
void handleStop() {
  // This function is called when a MIDI STOP message is received
  isPlaying = false;
  pulseCount = 0;
  AudioMemoryUsageMaxReset();
  deleteActiveCopy();
  envelope0.noteOff();
  allOff();
  autoSave();
  beat = 1;
  pagebeat = 1;
  SMP.page = 1;
  waitForFourBars = false;
}


//receive Midi notes and velocity, map to note array. if not playing, play the note
void handleNoteOn(uint8_t channel, uint8_t pitch, uint8_t velocity) {
  // return if out of range
  if (SMP.y - 1 < 1 || SMP.y - 1 > maxFiles) return;

  //  envelopes[SMP.y - 1]->release(11880 / 2);
  unsigned int livenote = SMP.y + pitch - 60;  // set Base to C3
  Serial.println(livenote);

  // fake missing octaves (only 16 notes visible, so step up/down an octave!)
  if (livenote > 16) livenote -= 12;
  if (livenote < 1) livenote += 12;

  if (livenote >= 1 && livenote <= 16) {
    light(SMP.x, livenote, CRGB(0, 0, 255));
    FastLEDshow();
    Serial.println(SMP.y - 1);
    _samplers[SMP.y - 1].noteEvent(((SampleRate[SMP.y - 1] * 12) + pitch - 60), velocity * 8, true, false);
    if (isPlaying) {
      if (!SMP.mute[SMP.y - 1]) {
        note[beat][livenote][0] = SMP.y - 1;
        note[beat][livenote][1] = velocity;
      }
    }
  }
}

void handleNoteOff(uint8_t channel, uint8_t pitch, uint8_t velocity) {
  Serial.print("Note Off, ch=");
  /*if (isPlaying) {
    if (pitch > 0 && pitch < 17) {
      if (note[beat][pitch][0] == SMP.currentChannel) {
        note[beat][pitch][0] = 0;
        light(pitch, SMP.currentChannel + 1, CRGB(0, 0, 0));
        FastLEDshow();
      }
    }
  } else {
    if (pitch > 0 && pitch < 17) {
      if (note[beat][pitch][0] == SMP.curr  entChannel) {
        note[beat][pitch][0] = 0;
        light(pitch, SMP.currentChannel + 1, CRGB(0, 0, 0));
        FastLEDshow();
      }
    }
  }*/
}

void handleStart() {
  // This function is called when a MIDI Start message is received
  waitForFourBars = true;
  pulseCount = 0;  // Reset pulse count on start
  Serial.println("MIDI Start Received");
}


void handleTimeCodeQuarterFrame(uint8_t data) {
  // This function is called when a MIDI Start message is received
  Serial.println("MIDI TimeCodeQuarterFrame Received");
}



void handleSongPosition(uint16_t beats) {
  // This function is called when a Song Position Pointer message is received
  Serial.print("Song Position Pointer Received: ");
  Serial.println(beats);
}


void myClock() {
  if (waitForFourBars) pulseCount++;
  unsigned int now = millis();
  if (lastClockTime > 0) {
    totalInterval += now - lastClockTime;
    clockCount++;

    if (clockCount >= numPulsesForAverage) {
      float averageInterval = totalInterval / (float)numPulsesForAverage;
      float bpm = 60000.0 / (averageInterval * 24);
      //floor bpm
      SMP.bpm = round(bpm);
      Serial.println(SMP.bpm);

      playTimer.update(((60 * 1000 / SMP.bpm) / 4) * 1000);
      clockCount = 0;
      totalInterval = 0;
    }
  }
  lastClockTime = now;
}
