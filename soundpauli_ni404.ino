//#define FASTLED_ALLOW_INTERRUPTS 0
#define USE_WS2812SERIAL   // leds

#include <Mapf.h>
#include <WS2812Serial.h>  // leds

#include <FastLED.h>       // leds
#include <Audio.h>

#include <Encoder.h>
#include "avdweb_Switch.h"
#include <EEPROM.h>
#include <cstring>  // For memcmp

#include "colors.h"
#include "files.h"
#include <TeensyPolyphony.h>
#include "audioinit.h"

#define NUM_LEDS 256
#define DATA_PIN 17
CRGB leds[NUM_LEDS];

IntervalTimer thisTimer;
bool setvol = false;
unsigned int lastPage = 1;
unsigned long lastButtonPressTime = 0;
bool resetTimerActive = false;
int previousEncoderValues[3];

PROGMEM const int maxX = 16;
PROGMEM const int maxY = 16;
PROGMEM const int maxPages = 8;
PROGMEM const int maxFiles = 8;  // 13
PROGMEM const int maxlen = (maxX * maxPages) + 1;
int unpaintMode, paintMode = false;
float pulse = 0;
int dir = 1;

unsigned int tmp[maxX + 1][maxY + 1][2] = {};
unsigned int note[maxlen][maxY + 1][2] = {};
PROGMEM long const ram = 15989988;
EXTMEM unsigned char sample[maxFiles][ram / (maxFiles + 1)];
unsigned int sample_len[maxFiles];

unsigned int mute[maxY] = {};

bool freshnote = false;
int pagebeat, beat = 0;
float amp = 0;
unsigned int samplePackID, fileID = 1;


int filter = 500;


PROGMEM const int SONG_LEN = maxX * maxPages;


bool isPlaying = false;

struct Device {
  int singleMode;
  int currentChannel;
  int vol;
  int bpm;
  int velocity;
  int page;
  int edit;
  int file;
  int pack;
  int wav;
  int folder;
  bool activeCopy;
  int x;
  int y;
};


struct Mode {
  String name;
  int minValues[3];
  int maxValues[3];
  int pos[3];
};

// Declare the modes
Mode draw = { "DRAW", { 1, 1, 1 }, { 16, 8, 16 }, { 1, 1, 1 } };
Mode singleMode = { "SINGLE", { 1, 1, 1 }, { 16, 8, 16 }, { 1, 1, 1 } };

Mode volume = { "VOLUME", { 1, 1, 1 }, { 1, 10, 200 }, { 1, 9, 100 } };
Mode velocity = { "VELOCITY", { 1, 1, 1 }, { 1, 1, 16 }, { 1, 1, 10 } };
Mode set_Wav = { "SET_WAV", { 1, 1, 1 }, { 1, 10, 999 }, { 1, 0, 1 } };
Mode set_SamplePack = { "SET_SAMPLEPACK", { 1, 1, 1 }, { 1, 1, 8 }, { 1, 1, 1 } };
Mode menu = { "MENU", { 1, 1, 1 }, { 1, 1, 12 }, { 1, 1, 1 } };


// Declare currentMode as a global variable
Mode* currentMode = &draw;


Device SMP = { false, 1, 10, 100, 16, 1, 1, 1, 1, 1, 0, false, 1, 1 };

Encoder encoders[3] = {
  Encoder(2, 4),
  Encoder(14, 9),
  Encoder(22, 5),
};
int buttons[4], oldButtons[4] = { 0, 0, 0, 0 };


Switch multiresponseButton1 = Switch(3);
Switch multiresponseButton2 = Switch(16);
Switch multiresponseButton3 = Switch(15);
String oldPos = "";



void FastLEDclear() {
  FastLED.clear();
}

void FastLEDshow() {
  FastLED.show();
}


void drawNoSD() {
  while (!SD.begin(10)) {
    FastLEDclear();
    for (int gx = 0; gx < 48; gx++) {
      light(noSD[gx][0], maxY - noSD[gx][1], CRGB(50, 0, 0));
    }
    FastLEDshow();
    delay(1000);
  }
}


void copyPosValues(Mode* source, Mode* destination) {
  for (int i = 0; i < 3; i++) {
    destination->pos[i] = source->pos[i];
  }
}


void setVelocity() {
  if (currentMode->pos[2] != SMP.velocity) {

    if (!SMP.singleMode) {
      note[SMP.x][SMP.y][1] = round(mapf(currentMode->pos[2], 1, 16, 1, 100));
    } else {
      Serial.println("Overal Velocity: " + String(currentMode->pos[2]));
      for (int nx = 1; nx < maxlen; nx++) {
        for (int ny = 1; ny < maxY + 1; ny++) {
          if (note[nx][ny][0] == SMP.currentChannel) note[nx][ny][1] = round(mapf(currentMode->pos[2], 1, 16, 1, 100));
        }
      }
    }
  }
  drawVelocity(CRGB(0, 40, 0));
}


void setup() {
  Serial.begin(51200);
  delay(1000);
  EEPROM.get(0, samplePackID);
  pinMode(0, INPUT_PULLDOWN);
  pinMode(3, INPUT_PULLDOWN);
  pinMode(16, INPUT_PULLDOWN);
  FastLED.addLeds<WS2812SERIAL, DATA_PIN, BRG>(leds, NUM_LEDS);
  //FastLED.setBrightness(90);
  FastLED.setMaxRefreshRate(0);
  showIntro();


  Serial.print("Initializing SD card...");
  drawNoSD();
  FastLEDclear();
  for (int z = 1; z <= maxFiles; z++) {
    loadSample(samplePackID, z);
  }

  AudioMemory(16);
  for (int i = 0; i <= maxFiles; i++) {
    voices[i]->enableInterpolation(true);
    envelopes[i]->attack(1);
  }

  for (int vx = 1; vx < SONG_LEN + 1; vx++) {
    for (int vy = 1; vy < maxY + 1; vy++) {
      note[vx][vy][1] = 30;
    }
  }
  // turn on the output
  audioShield.enable();
  audioShield.volume(0.9);


  _samplers[1].addVoice(sound1, mix1_a, 0, envelope1);
  _samplers[2].addVoice(sound2, mix1_a, 1, envelope2);
  _samplers[3].addVoice(sound3, mix1_a, 2, envelope3);
  _samplers[4].addVoice(sound4, mix1_a, 3, envelope4);

  _samplers[5].addVoice(sound5, mix1_b, 0, envelope5);
  _samplers[6].addVoice(sound6, mix1_b, 1, envelope6);
  _samplers[7].addVoice(sound7, mix1_b, 2, envelope7);
  _samplers[8].addVoice(sound8, mix1_b, 3, envelope8);

  _samplers[9].addVoice(sound9, mix1_c, 0, envelope9);
  _samplers[10].addVoice(sound10, mix1_c, 1, envelope10);
  _samplers[11].addVoice(sound11, mix1_c, 2, envelope11);
  _samplers[12].addVoice(sound12, mix1_c, 3, envelope12);

  _samplers[13].addVoice(sound13, mix1_d, 0, envelope13);
  _samplers[0].addVoice(sound0, mix1_d, 2, envelope0);  //prev

  mix1_a.gain(0, 0.8);
  mix1_a.gain(1, 0.8);
  mix1_a.gain(2, 0.8);
  mix1_a.gain(3, 0.8);

  mix1_b.gain(0, 0.8);
  mix1_b.gain(1, 0.8);
  mix1_b.gain(2, 0.8);
  mix1_b.gain(3, 0.8);

  mix1_c.gain(0, 0.8);
  mix1_c.gain(1, 0.8);
  mix1_c.gain(2, 0.8);
  mix1_c.gain(3, 0.8);

  mix1_d.gain(0, 0.8);  //13
  mix1_d.gain(1, 0.6);
  mix1_d.gain(2, 0.6);  //prev
  mix1_d.gain(3, 0.8);

  //master
  mix2.gain(0, 0.8);
  mix2.gain(1, 0.8);
  mix2.gain(2, 0.8);
  mix2.gain(3, 0.8);

  mix3.gain(0, 0.8);
  mix3.gain(1, 0.8);
  mix3.gain(2, 0.8);
  mix3.gain(3, 0.8);

  mix_master.gain(0, 0.8);
  mix_master.gain(1, 0.8);
  mix_master.gain(2, 0.8);
  mix_master.gain(3, 0.8);

  // configure what the synth drums will sound like

  /*drum0.frequency(60);
    drum0.length(300);
    drum0.secondMix(0.0);
    drum0.pitchMod(1.0);
    drum1.frequency(60);
    drum1.length(300);
    drum1.secondMix(0.0);
    drum1.pitchMod(0.55);
  */
  waveform1.begin(WAVEFORM_SQUARE);
  waveform1.amplitude(0.3);
  waveform1.frequency(100);
  waveform1.phase(0);

  /*
  envelope14.attack(50);
  envelope14.decay(0.5);
  envelope14.sustain(0.3);
  envelope14.release(0.5);
  */

  envelope14.delay(0.0);
  envelope14.attack(50);
  envelope14.hold(0.0);
  envelope14.decay(0.0);
  envelope14.sustain(0.2);
  envelope14.release(500.0);

  filter3.octaveControl(5.0);
  filter3.resonance(1);
  
  filter14.octaveControl(5.0);
  filter14.resonance(1);
  AudioInterrupts();
  autoLoad();
  //set BPM:100
  SMP.bpm = 100;
  thisTimer.begin(playNote, 150000);


  //button1
  multiresponseButton1.setSingleClickCallback(&buttonCallbackFunction, (void*)"1");
  multiresponseButton1.setLongPressCallback(&buttonCallbackFunction, (void*)"2");
  multiresponseButton1.setDoubleClickCallback(&buttonCallbackFunction, (void*)"3");
  multiresponseButton1.setReleasedCallback(&buttonCallbackFunction, (void*)"a");
  multiresponseButton1.setPushedCallback(&buttonCallbackFunction, (void*)"y");
  //button2
  multiresponseButton2.setSingleClickCallback(&buttonCallbackFunction, (void*)"4");
  multiresponseButton2.setLongPressCallback(&buttonCallbackFunction, (void*)"5");
  multiresponseButton2.setDoubleClickCallback(&buttonCallbackFunction, (void*)"6");
  multiresponseButton2.setReleasedCallback(&buttonCallbackFunction, (void*)"b");
  //button3
  multiresponseButton3.setSingleClickCallback(&buttonCallbackFunction, (void*)"7");
  multiresponseButton3.setLongPressCallback(&buttonCallbackFunction, (void*)"8");
  multiresponseButton3.setDoubleClickCallback(&buttonCallbackFunction, (void*)"9");
  multiresponseButton3.setReleasedCallback(&buttonCallbackFunction, (void*)"c");
  multiresponseButton3.setPushedCallback(&buttonCallbackFunction, (void*)"x");
}

void buttonCallbackFunction(void* s) {

  if (s == (char*)"a") buttons[1] = 9;
  if (s == (char*)"1") buttons[1] = 1;
  if (s == (char*)"2") buttons[1] = 2;
  if (s == (char*)"y") buttons[1] = 5;
  if (s == (char*)"3") buttons[1] = 3;

  if (s == (char*)"b") buttons[2] = 9;
  if (s == (char*)"4") buttons[2] = 1;
  if (s == (char*)"5") buttons[2] = 2;
  if (s == (char*)"6") buttons[2] = 3;

  if (s == (char*)"c") buttons[3] = 9;
  if (s == (char*)"7") buttons[3] = 1;
  if (s == (char*)"8") buttons[3] = 2;
  if (s == (char*)"x") buttons[3] = 5;
  if (s == (char*)"9") buttons[3] = 3;


  if (memcmp(buttons, oldButtons, sizeof(buttons)) != 0) {
    memcpy(oldButtons, buttons, sizeof(buttons));  // Update oldButtons
    lastButtonPressTime = millis();
    resetTimerActive = true;
  }
}


void checkMode() {
  String buttonString = "";
  buttonString = String(buttons[1]) + String(buttons[2]) + String(buttons[3]);


  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "101") { togglePlay(isPlaying); }
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "020") { switchMode(&volume); }

  if (currentMode == &draw && buttonString == "110") { toggleCopyPaste(); }



  if ((currentMode == &singleMode) && buttonString == "030") {
    int velo = round(mapf(note[SMP.x][SMP.y][1], 1, 100, 1, 16 * 4));
    Serial.println(velo);
    SMP.velocity = velo;


    switchMode(&velocity);
    SMP.singleMode = true;
    encoders[2].write(velo);
  }

  if ((currentMode == &draw) && buttonString == "030") {
    int velo = round(mapf(note[SMP.x][SMP.y][1], 1, 100, 1, 16 * 4));
    Serial.println(velo);
    SMP.velocity = velo;
    SMP.singleMode = false;
    switchMode(&velocity);
    encoders[2].write(velo);
  }

  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "202") clearPage();

  if (currentMode == &velocity && SMP.singleMode && buttonString == "090") {
    switchMode(&singleMode);
    SMP.singleMode = true;
  }
  if (currentMode == &velocity && !SMP.singleMode && buttonString == "090") { switchMode(&draw); }

  if (currentMode == &volume && buttonString == "090") {
    switchMode(&draw);
    setvol = false;
  }

  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "099") {
    paintMode = false;
    unpaintMode = false;
  }

  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "009") {
    paintMode = false;
    unpaintMode = false;
  }
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "900") {
    unpaintMode = false;
    paintMode = false;
  }

  //Menu Load/Save
  if ((currentMode == &draw) && buttonString == "011") {
    switchMode(&menu);
  } else if ((currentMode == &menu) && buttonString == "001") {
    switchMode(&draw);
  } else if ((currentMode == &menu) && buttonString == "010") {
    savePattern();
  } else if ((currentMode == &menu) && buttonString == "100") {
    loadPattern();
  }


  // Search Wave + Load + Exit/
  if ((currentMode == &singleMode) && buttonString == "220") {
    switchMode(&set_Wav);
  } else if ((currentMode == &set_Wav) && buttonString == "100") {
    loadWav();
  } else if ((currentMode == &set_Wav) && buttonString == "001") {
    switchMode(&singleMode);
    SMP.singleMode = true;
  }

  //Set SamplePack + Load + Save + Exit
  if ((currentMode == &draw) && buttonString == "220") {
    switchMode(&set_SamplePack);
  } else if ((currentMode == &set_SamplePack) && buttonString == "010") {
    savePack();
  } else if ((currentMode == &set_SamplePack) && buttonString == "100") {
    loadPack();
  } else if ((currentMode == &set_SamplePack) && buttonString == "001") {
    switchMode(&draw);
  }

  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "010") { toggleMute(); }
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "002") { paintMode = true; }
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "200") {
    unpaint();
    unpaintMode = true;
    deleteActiveCopy();
  }

  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "005") { paint(); }
  if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "500") { unpaint(); }

  //toggle SingleMode
  if (currentMode == &singleMode && buttonString == "300") {
    copyPosValues(&singleMode, &draw);
    switchMode(&draw);
  } else if (currentMode == &draw && buttonString == "300") {
    SMP.currentChannel = SMP.y - 1;
    copyPosValues(&draw, &singleMode);
    switchMode(&singleMode);
    SMP.singleMode = true;
  }
}



void toggleMute() {
  if (mute[SMP.y - 1]) {
    mute[SMP.y - 1] = false;
  } else {
    //wenn leer oder nicht gemuted:
    mute[SMP.y - 1] = true;
  }

  //Serial.println("mute:" + String(SMP.y-1));
}

void deleteActiveCopy() {
  SMP.activeCopy = false;
}


void ynoallOff() {
  envelope1.noteOff();
  envelope2.noteOff();
  envelope3.noteOff();
  envelope4.noteOff();
  envelope5.noteOff();
  envelope6.noteOff();
  envelope7.noteOff();
  envelope8.noteOff();
  envelope9.noteOff();
  envelope10.noteOff();
  envelope11.noteOff();
  envelope12.noteOff();
  envelope13.noteOff();
  envelope14.noteOff();
}

void togglePlay(bool& value) {
  AudioMemoryUsageMaxReset();
  deleteActiveCopy();
  value = !value;  // Toggle the boolean value
  Serial.println(value ? "Playing" : "Paused");
  envelope0.noteOff();
  allOff();
  autoSave();
  beat = 0;
  pagebeat = 0;
  SMP.page = 1;
}





void playNote() {
  if (isPlaying) {
    beat++;
    pagebeat++;

    if (beat > SMP.page * maxX) {
      SMP.page = SMP.page + 1;
      pagebeat = 1;
      if (SMP.page > maxPages) SMP.page = 1;
      if (SMP.page > lastPage) SMP.page = 1;
      
      Serial.println(" -->page: "+ String(SMP.page));
      Serial.println(" -->lastpage: "+ String(lastPage));
      Serial.println(" -->edit: "+ String(SMP.edit));
    
    }
    if (beat > maxX * lastPage) beat = 1;
  }

  for (int b = 1; b < maxY + 1; b++) {

    if (b == 2 && mute[1]) envelope1.noteOff();
    if (b == 3 && mute[2]) envelope2.noteOff();
    if (b == 4 && mute[3]) envelope3.noteOff();
    if (b == 5 && mute[4]) envelope4.noteOff();
    if (b == 6 && mute[5]) envelope5.noteOff();
    if (b == 7 && mute[6]) envelope6.noteOff();
    if (b == 8 && mute[7]) envelope7.noteOff();
    if (b == 9 && mute[8]) envelope8.noteOff();
    if (b == 10 && mute[9]) envelope9.noteOff();
    if (b == 11 && mute[10]) envelope10.noteOff();
    if (b == 12 && mute[11]) envelope11.noteOff();
    if (b == 13 && mute[12]) envelope12.noteOff();
    if (b == 14 && mute[13]) envelope13.noteOff();
    if (b == 15) envelope14.noteOff();


    if (note[beat][b][0] > 0 && !mute[note[beat][b][0]]) {
      if (note[beat][b][0] != 14) {
        filter3.frequency(filter * 20);
        _samplers[note[beat][b][0]].noteEvent(12 * SampleRate[note[beat][b][0]] + b - (note[beat][b][0] + 1), note[beat][b][1], true, false);
        // usbMIDI.sendNoteOn(b, note[beat][b][1], 1);
      }
      if (note[beat][b][0] == 14) {
        waveform1.frequency(b * 10);
        float nvel = (float)note[beat][b][1] / 200.0;
        waveform1.amplitude(nvel);
        filter14.frequency(filter * 20);
        envelope14.noteOn();
      }
    }
  }
}


void unpaint() {
  Serial.println("UNpaint");
  paintMode = false;
  int y = SMP.y;
  int x = (SMP.edit - 1) * maxX + SMP.x;
  if (!SMP.singleMode) {
    Serial.println("deleting" + String(x));
    note[x][y][0] = 0;
  } else {
    if (note[x][y][0] == SMP.currentChannel) note[x][y][0] = 0;
  }
}

void paint() {
  Serial.println("paint");

  int sample = 1;
  int x = (SMP.edit - 1) * maxX + SMP.x;
  //int x = SMP.x+;
  int y = SMP.y;

  if (!SMP.singleMode) {
    if (y > 1 && y <= maxFiles + 1 || y == maxY - 1) {
      if (note[x][y][0] == 0) {
        note[x][y][0] = (y - 1);
      } else {
        note[x][y][0] = note[x][y][0] + sample;
        for (int vx = 1; vx < maxX + 1; vx++) {
          light(vx, note[x][y][0] + 1, col[note[x][y][0]] * 12);
        }
      }
    }
  } else {
    note[x][y][0] = SMP.currentChannel;
  }


  if (note[x][y][0] > maxY - 2) {
    note[x][y][0] = 1;
    for (int vx = 1; vx < maxX + 1; vx++) {
      light(vx, note[x][y][0] + 1, col[note[x][y][0]] * 12);
    }
    FastLEDshow();
  }

  if (!isPlaying) {
    if (note[x][y][0] != 14) _samplers[note[x][y][0]].noteEvent(12 * SampleRate[note[x][y][0]] + y - (note[x][y][0] + 1), 50, true, false);

    if (note[x][y][0] == 14) {
      waveform1.frequency(y * 10);
      envelope14.noteOn();
      delay(200);
      envelope14.noteOff();
    }
  }

  FastLEDshow();
  freshnote = true;
}



void light(int x, int y, CRGB color) {
  if (y > 0 && y < 17 && x > 0 && x < 17) {
    if (y > maxY) y = 1;
    if (y % 2 == 0) {
      leds[(maxX - x) + (maxX * (y - 1))] = color;
    }
    if (y % 2 != 0) {
      leds[(x - 1) + (maxX * (y - 1))] = color;
    }
  }
}



void switchMode(Mode* newMode) {
  unpaintMode = false;
  SMP.singleMode = false;
  paintMode = false;

  if (newMode != currentMode) {
    currentMode = newMode;
    String savedpos = "" + String(currentMode->pos[0]) + " " + String(currentMode->pos[1]) + " " + String(currentMode->pos[2]);
    Serial.println("--------------");
    Serial.println(currentMode->name + " > " + savedpos);

    //set values for encoders
    for (int i = 0; i < 3; i++) {  // Assuming 3 encoders
      float newval = round(mapf(currentMode->pos[i], 1, currentMode->maxValues[i], 1, currentMode->maxValues[i] * 4));
      if (i == 0) {  //reverse KnobDirection for Left
        newval = round(mapf(reverseMapEncoderValue(currentMode->pos[i], 1, 16), 1, currentMode->maxValues[i], 1, currentMode->maxValues[i] * 4));
      }
      encoders[i].write(newval);
    }
  }
}

int reverseMapEncoderValue(int encoderValue, int minValue, int maxValue) {
  return maxValue - (encoderValue - minValue);
}

float mapAndClampEncoderValue(Encoder& encoder, int min, int max, int id) {
  float value = encoder.read();

  float mappedValue = round(mapf(value, min, max * 4, min, max));

  if (mappedValue < min) {
    encoder.write(min);
    //prev Page
    if (id == 2 && (currentMode == &draw || currentMode == &singleMode)) {
      if (SMP.edit > 1) {
        SMP.edit = SMP.edit - 1;
        encoders[1].write(round(mapf(SMP.edit, 1, currentMode->maxValues[1], 1, currentMode->maxValues[1] * 4)));
        encoder.write(max);
        return max;
      }
    }
    return min;
  } else if (mappedValue > max) {
    encoder.write(max * 4);
    //next Page
    if (id == 2 && (currentMode == &draw || currentMode == &singleMode)) {
      if (SMP.edit < maxPages) {
        SMP.edit = SMP.edit + 1;
        encoders[1].write(round(mapf(SMP.edit, 1, currentMode->maxValues[1], 1, currentMode->maxValues[1] * 4)));
        encoder.write(min);
        return min;
      }
    }
    return max;
  } else {
    return mappedValue;
  }
}



void checkPositions() {
  for (int i = 0; i < 3; i++) {
    int currentEncoderValue = encoders[i].read();
    if (currentEncoderValue != previousEncoderValues[i]) {
      currentMode->pos[0] = reverseMapEncoderValue(mapAndClampEncoderValue(encoders[0], 1, currentMode->maxValues[0], 0), 1, 16);
      currentMode->pos[1] = mapAndClampEncoderValue(encoders[1], 1, currentMode->maxValues[1], 1);
      currentMode->pos[2] = mapAndClampEncoderValue(encoders[2], 1, currentMode->maxValues[2], 2);
    }
  }
}


void previewSample(int folder, int sampleID) {
  char OUTPUTf[50];
  int plen = 0;
  int previewsample = ((folder)*100) + sampleID;
  sprintf(OUTPUTf, "samples/%d/_%d.wav", folder, previewsample);
  Serial.println(OUTPUTf);
  sampleFile = SD.open(OUTPUTf);
  if (sampleFile) {
    sampleFile.seek(24);
    for (byte i = 24; i < 25; i++) {
      int g = sampleFile.read();
      if (g == 72) PrevSampleRate = 4;
      if (g == 68) PrevSampleRate = 3;
      if (g == 34) PrevSampleRate = 2;
      if (g == 17) PrevSampleRate = 1;
      if (g == 0) PrevSampleRate = 4;
      //SerialPrintln(PrevSampleRate);
    }
    sampleFile.seek(44);
    memset(sample[0], 0, sizeof(sample_len[0]));
    plen = 0;
    while (sampleFile.available()) {
      int b = sampleFile.read();
      sample[0][plen] = b;
      plen++;
    }
  }
  sampleFile.close();
  _samplers[0].removeAllSamples();
  delay(50);
  _samplers[0].addSample(36, (int16_t*)sample[0], (int)(plen / 2) - 100, 1);
  _samplers[0].noteEvent(12 * PrevSampleRate, 20, true, false);
}

void loadSample(int packID, int sampleID) {
  Serial.print("loading");
  Serial.println(packID);
  int i = 0;
  drawNoSD();

  int yposLoader = sampleID + 1;
  if (sampleID > maxY) yposLoader = 2;
  for (int f = 1; f < (maxX / 2) + 1; f++) {
    light(f, yposLoader, CRGB(20, 20, 0));
  }
  FastLEDshow();

  char OUTPUTf[50];
  sprintf(OUTPUTf, "%d/%d.wav", packID, sampleID);

  if (packID == 0) {
    //SingleTrack from Samples-Folder
    sprintf(OUTPUTf, "samples/%d/_%d.wav", SMP.folder - 1, sampleID);
    sampleID = SMP.currentChannel;
    //SerialPrintln(sampleID);
  }

  usedFiles[sampleID - 1] = OUTPUTf;
  //SerialPrint(OUTPUTf);
  sampleFile = SD.open(OUTPUTf);
  if (sampleFile) {
    sampleFile.seek(24);
    for (byte i = 24; i < 25; i++) {
      int g = sampleFile.read();
      if (g == 0) SampleRate[sampleID] = 4;
      if (g == 17) SampleRate[sampleID] = 1;
      if (g == 34) SampleRate[sampleID] = 2;
      if (g == 68) SampleRate[sampleID] = 3;
      if (g == 72) SampleRate[sampleID] = 4;
    }
    sampleFile.seek(44);
    i = 0;
    memset(sample[sampleID], 0, sizeof(sample_len[sampleID]));


    while (sampleFile.available()) {
      int b = sampleFile.read();
      sample[sampleID][i] = b;
      i++;
    }
  }
  sampleFile.close();
  //  SerialPrint("sampleLen:");
  //SerialPrintln(i);
  i = i / 2;
  _samplers[sampleID].removeAllSamples();
  _samplers[sampleID].addSample(36, (int16_t*)sample[sampleID], (int)i - 100, 1);
  for (int f = 1; f < maxX + 1; f++) {
    light(f, yposLoader, col[sampleID]);
  }
  FastLEDshow();
}



void loop() {


  multiresponseButton1.poll();
  multiresponseButton2.poll();
  multiresponseButton3.poll();
  checkPositions();

  if (currentMode->name == "DRAW") {
    canvas(false, 0);
  } else if (currentMode->name == "VOLUME") {
    setVolume();
  } else if (currentMode->name == "VELOCITY") {
    setVelocity();
  } else if (currentMode->name == "SINGLE") {
    canvas(true, 22);
  } else if (currentMode->name == "MENU") {
    showMenu();
  } else if (currentMode->name == "SET_SAMPLEPACK") {
    showSamplePack();
  } else if (currentMode->name == "SET_WAV") {
    showWave();
  }





  //reset buttons
  if (resetTimerActive && millis() - lastButtonPressTime > 80) {
    // Print the final button states
    checkMode();

    memset(buttons, 0, sizeof(buttons));
    memset(oldButtons, 0, sizeof(oldButtons));
    resetTimerActive = false;  // Stop the timer
  }


  if (currentMode == &draw || currentMode == &singleMode) {
    drawBase();
    drawSamples();
    if (currentMode != &velocity) drawCursor();

    if (isPlaying) {
      drawTimer(pagebeat);
    }
  }

  FastLEDshow();  // draw!
}


/******************************************************************************************************/



void canvas(bool singleview, int channel) {

  String posString = "Y:" + String(currentMode->pos[0]) + " X:" + String(currentMode->pos[2]) + " Page:" + String(currentMode->pos[1]);

  if (posString != oldPos) {
    oldPos = posString;
    Serial.println(posString);

    SMP.x = currentMode->pos[2];
    SMP.y = currentMode->pos[0];

    Serial.println("vel: " + String(note[SMP.x][SMP.y][1]));

    if (paintMode) { note[(SMP.edit - 1) * maxX + SMP.x][SMP.y][0] = SMP.y - 1; }
    if (paintMode && currentMode == &singleMode) { note[(SMP.edit - 1) * maxX + SMP.x][SMP.y][0] = SMP.currentChannel; }

    if (unpaintMode) {
      if (SMP.singleMode) {
        if (note[(SMP.edit - 1) * maxX + SMP.x][SMP.y][0] == SMP.currentChannel) note[(SMP.edit - 1) * maxX + SMP.x][SMP.y][0] = 0;
      } else {
        note[(SMP.edit - 1) * maxX + SMP.x][SMP.y][0] = 0;
        
      }
    }


    int editpage = currentMode->pos[1];

    if (SMP.y==16){
      
        filter = currentMode->pos[1]*30;

    }else{
    if (editpage != SMP.edit && editpage <= lastPage) {
      SMP.edit = editpage;
      Serial.println("p:" + String(SMP.edit));
    }
    }
  }
}





void toggleCopyPaste() {
  if (!SMP.activeCopy) {
    //copy the pattern into the memory
    Serial.print("copy now");
    int src = 0;
    for (int c = ((SMP.edit - 1) * maxX) + 1; c < ((SMP.edit - 1) * maxX) + (maxX + 1); c++) {  //maxy?
      src++;
      for (int y = 1; y < maxY + 1; y++) {
        Serial.print(c);
        Serial.print("-");
        Serial.println(y);

        for (int b = 0; b < 2; b++) {
          tmp[src][y][b] = note[c][y][b];
        }
      }
    }

  } else {
    //paste the memory into the song
    Serial.print("paste here!");
    int src = 0;
    for (int c = ((SMP.edit - 1) * maxX) + 1; c < ((SMP.edit - 1) * maxX) + (maxX + 1); c++) {
      src++;
      for (int y = 1; y < maxY + 1; y++) {
        for (int b = 0; b < 2; b++) {
          note[c][y][b] = tmp[src][y][b];
        }
      }
    }
  }
  SMP.activeCopy = !SMP.activeCopy;  // Toggle the boolean value
}



void clearPage() {


  int src = 0;

  if (SMP.singleMode) {
    for (int c = ((SMP.edit - 1) * maxX) + 1; c < ((SMP.edit - 1) * maxX) + (maxX + 1); c++) {
      src++;
      for (int y = 1; y < maxY + 1; y++) {
        if (note[c][y][0] == SMP.currentChannel) note[c][y][0] = 0;
        if (note[c][y][0] == SMP.currentChannel) note[c][y][1] = 50;
      }
    }
  } else {
    for (int c = ((SMP.edit - 1) * maxX) + 1; c < ((SMP.edit - 1) * maxX) + (maxX + 1); c++) {
      src++;
      for (int y = 1; y < maxY + 1; y++) {
        note[c][y][0] = 0;
        note[c][y][1] = 50;
      }
    }
  }
}

void setVolume() {
  if (setvol) {
    drawVolume(SMP.vol);
  } else {
    //show BPM
    FastLEDclear();
    showIcons("bpm", CRGB(0, 10, 10));
    showNumber(SMP.bpm, false, 0);
  }


  if (currentMode->pos[1] != SMP.vol) {
    SMP.vol = currentMode->pos[1];
    float vol = float(SMP.vol / 10.0);
    Serial.println("Vol: " + String(vol));
    if (vol < 1.0) audioShield.volume(vol);
    setvol = true;
  }


  if (currentMode->pos[2] != SMP.bpm) {
    FastLEDclear();
    setvol = false;
    Serial.println("BPM: " + String(currentMode->pos[2]));
    SMP.bpm = currentMode->pos[2];
    thisTimer.update(((60 * 1000 / SMP.bpm) / 4) * 1000);
    FastLEDclear();
    showIcons("bpm", CRGB(0, 10, 10));
    showNumber(SMP.bpm, false, 0);
  }
}


CRGB getCol(int g) {
  return col[g] * 10;
}




void drawVolume(int vol) {
  FastLEDclear();

  for (int y = 1; y < 15; y++) {
    for (int x = 1; x < y; x++) {
      light(15 - x, y - x, CRGB(0, 0, 2));
    }
  }

  for (int y = 1; y < int(vol * 1.3) + 2; y++) {
    for (int x = 1; x < y; x++) {
      light(int(vol * 1.3) + 2 - x, y - x, CRGB(vol * vol, 20 - vol, 0));
    }
  }
  FastLEDshow();
}




void showMenu() {
  int knopf = 2;
  drawNoSD();
  FastLEDclear();

  showIcons("loadsave", CRGB(10, 5, 0));
  showIcons("helper_select", CRGB(0, 0, 5));

  char OUTPUTf[50];
  sprintf(OUTPUTf, "%d.txt", SMP.file);
  if (SD.exists(OUTPUTf)) {
    showIcons("helper_save", CRGB(1, 0, 0));
    showIcons("helper_load", CRGB(0, 20, 0));
    showNumber(SMP.file, CRGB(0, 0, 20), 0);
  }

  else {
    showIcons("helper_save", CRGB(20, 0, 0));
    showIcons("helper_load", CRGB(0, 1, 0));
    showNumber(SMP.file, CRGB(20, 20, 40), 0);
  }

  if (currentMode->pos[knopf] != SMP.file) {
    Serial.print("File: " + String(currentMode->pos[knopf]));
    Serial.println();
    SMP.file = currentMode->pos[knopf];
  }
}






void showSamplePack() {
  drawNoSD();

  FastLEDclear();

  showIcons("samplepack", CRGB(10, 10, 0));
  showIcons("helper_select", CRGB(0, 0, 5));
  showNumber(SMP.pack, CRGB(20, 0, 0), 0);

  char OUTPUTf[50];
  sprintf(OUTPUTf, "%d/%d.wav", SMP.pack, 1);
  if (SD.exists(OUTPUTf)) {
    showIcons("helper_load", CRGB(0, 20, 0));
    showIcons("helper_save", CRGB(3, 0, 0));
    showNumber(SMP.pack, CRGB(0, 20, 0), 0);
  } else {
    showIcons("helper_load", CRGB(0, 3, 0));
    showIcons("helper_save", CRGB(20, 0, 0));
    showNumber(SMP.pack, CRGB(20, 0, 0), 0);
  }

  int knopf = 2;
  if (currentMode->pos[knopf] != SMP.pack) {
    Serial.print("File: " + String(currentMode->pos[knopf]));
    Serial.println();
    SMP.pack = currentMode->pos[knopf];
  }
}


void loadPack() {
  Serial.println("Loading SamplePack #" + String(SMP.pack));
  drawNoSD();
  FastLEDclear();
  EEPROM.put(0, SMP.pack);
  for (int z = 1; z <= maxFiles; z++) {
    loadSample(SMP.pack, z);
  }
  char OUTPUTf[50];
  sprintf(OUTPUTf, "%d/%d.wav", SMP.pack, 1);
  if (SD.exists(OUTPUTf)) {
    showIcons("samplepack", CRGB(100, 0, 100));
    showNumber(SMP.pack, CRGB(100, 0, 100), 0);
  }
  FastLEDshow();
  switchMode(&draw);
}

void savePack() {
  Serial.println("Saving SamplePack in #" + String(SMP.pack));
  FastLEDclear();
  char OUTPUTdir[50];
  sprintf(OUTPUTdir, "%d/", SMP.pack);
  SD.mkdir(OUTPUTdir);
  delay(1000);
  for (int i = 0; i < sizeof(usedFiles) / sizeof(usedFiles[0]); i++) {

    for (int f = 1; f < (maxY / 2) + 1; f++) {
      light(i + 1, f, CRGB(20, 0, 0));
    }
    FastLEDshow();



    if (SD.exists(usedFiles[i].c_str())) {

      myOrigFile = SD.open(usedFiles[i].c_str());
      char OUTPUTf[50];
      sprintf(OUTPUTf, "%d/%d.wav", SMP.pack, i + 1);

      if (SD.exists(OUTPUTf)) {
        SD.remove(OUTPUTf);
        delay(100);
      }

      myDestFile = SD.open(OUTPUTf, FILE_WRITE);



      size_t n;
      uint8_t buf[512];
      while ((n = myOrigFile.read(buf, sizeof(buf))) > 0) {
        myDestFile.write(buf, n);
      }
      myDestFile.close();
      myOrigFile.close();
    }

    for (int f = 1; f < (maxY + 1) + 1; f++) {
      light(i + 1, f, CRGB(0, 20, 0));
    }
    FastLEDshow();
  }



  switchMode(&draw);
}


void showWave() {

  drawNoSD();

  FastLEDclear();
  if (((SMP.folder - 1) * 100) + SMP.wav < 100) showIcons("sample", col[SMP.y - 1]);
  showIcons("helper_select", col[SMP.y - 1]);
  showIcons("helper_load", CRGB(0, 20, 0));
  showIcons("helper_folder", CRGB(10, 10, 0));
  showNumber(((SMP.folder - 1) * 100) + SMP.wav, col_Folder[SMP.folder - 1], 0);



  if (currentMode->pos[1] != SMP.folder) {
    Serial.println();
    SMP.folder = currentMode->pos[1];
    Serial.print("Folder: " + String(SMP.folder - 1));
  }


  int knopf = 2;
  if (currentMode->pos[knopf] != SMP.wav) {
    Serial.print("File: " + String(currentMode->pos[knopf]));
    Serial.println();
    SMP.wav = currentMode->pos[knopf];

    FastLEDclear();
    if (((SMP.folder - 1) * 100) + SMP.wav < 100) showIcons("sample", col[SMP.y - 1]);
    char OUTPUTf[50];
    sprintf(OUTPUTf, "samples/%d/_%d.wav", SMP.folder - 1, ((SMP.folder - 1) * 100) + SMP.wav);
    Serial.println("------");
    Serial.println(OUTPUTf);
    if (SD.exists(OUTPUTf)) {
      showIcons("helper_select", col[SMP.y - 1]);
      showIcons("helper_load", CRGB(0, 20, 0));
      showIcons("helper_folder", CRGB(10, 30, 0));
      showNumber(((SMP.folder - 1) * 100) + SMP.wav, col_Folder[SMP.folder - 1], 0);
      previewSample(SMP.folder - 1, SMP.wav);
    } else {
      showIcons("helper_select", col[SMP.y - 1]);
      showIcons("helper_load", CRGB(0, 0, 0));
      showIcons("helper_folder", CRGB(10, 10, 0));
      showNumber(((SMP.folder - 1) * 100) + SMP.wav, col_Folder[SMP.folder - 1], 0);
    }
  }
}







void loadWav() {
  Serial.println("Loading Wave :" + String(((SMP.folder - 1) * 100) + SMP.wav));
  loadSample(0, ((SMP.folder - 1) * 100) + SMP.wav);
  switchMode(&singleMode);
  SMP.singleMode = true;
}




void showIntro() {
  FastLEDclear();
  FastLEDshow();
  for (int gx = 0; gx < 72; gx++) {
    light(logo[gx][0], maxY - logo[gx][1], CRGB(50, 50, 50));
    FastLEDshow();
    delay(10);
  }
  delay(200);

  for (int fade = 0; fade < 10 + 1; fade++) {
    for (int u = 0; u < NUM_LEDS; u++) {
      leds[u] = leds[u].fadeToBlackBy(fade * 10);
    }
    FastLEDshow();
    delay(50);
  }


int bright = 100;
  for (int y = -15;y < 3; y++) {
    FastLEDclear();
    showNumber(404, CRGB( 100, 100, 100), y);    
    FastLEDshow();
      delay(50);
  }

  

  delay(800);

  for (int y = 3; y< 16; y++) {
    FastLEDclear();

    showNumber(404, CRGB( 100, 100, 100), y);
    FastLEDshow();
    delay(50);
  }

  /*delay(500);

  for (int fade = 0; fade < 10 + 1; fade++) {
    for (int u = 0; u < NUM_LEDS; u++) {
      leds[u] = leds[u].fadeToBlackBy(fade * 10);
    }
    FastLEDshow();
    delay(100);
  }
  */
  FastLEDclear();
  FastLEDshow();
}



void showNumber(int count, CRGB color, int topY) {
  // fill_solid(leds, NUM_LEDS, CRGB::Black);
  if (!color) color = CRGB(20, 20, 20);
  char buf[4];
  sprintf(buf, "%03i", count);
  int stelle2 = buf[0] - '0';
  int stelle1 = buf[1] - '0';
  int stelle0 = buf[2] - '0';

  int ypos = maxY - topY;
  for (int gx = 0; gx < 24; gx++) {
    if (stelle2 > 0) light(1 + number[stelle2][gx][0], ypos - number[stelle2][gx][1], color);
    if ((stelle1 > 0 || stelle2 > 0)) light(6 + number[stelle1][gx][0], ypos - number[stelle1][gx][1], color);
    light(11 + number[stelle0][gx][0], ypos - number[stelle0][gx][1], color);
  }
  FastLEDshow();
}





void drawVelocity(CRGB color) {
  FastLEDclear();
  int vy = currentMode->pos[2];  //mapf(note[SMP.x][SMP.y][1],1,100,1,16);
  if (!SMP.singleMode) {
    for (int y = 1; y < vy + 1; y++) {
      light(SMP.x, y, CRGB(y * y, 20 - y, 0));
    }
  } else {
    Serial.println("single");
    for (int x = 1; x < maxX + 1; x++) {
      for (int y = 1; y < vy + 1; y++) {
        light(x, y, CRGB(y * y, 20 - y, 0));
      }
    }
  }
}



void drawBase() {
  if (!SMP.singleMode) {
    int colors = 0;
    for (int y = 1; y < maxY; y++) {
      for (int x = 1; x < maxX + 1; x++) {
        if (!mute[y - 1]) light(x, y, col[colors] / 8);
        if (mute[y - 1]) light(x, y, CRGB(0, 0, 0));
      }
      colors = colors + 1;
    }
    //die 1
    light(1, 1, CRGB(1, 1, 0));   //gelb
    light(5, 1, CRGB(1, 1, 0));   //gelb
    light(9, 1, CRGB(1, 1, 0));   //gelb
    light(13, 1, CRGB(1, 1, 0));  //gelb
  } else {



    for (int y = 1; y < maxY; y++) {
      for (int x = 1; x < maxX + 1; x++) {
        if (!mute[SMP.currentChannel]) light(x, y, col[SMP.currentChannel] / 12);
        if (mute[SMP.currentChannel]) light(x, y, col[SMP.currentChannel] / 24);
      }
    }

    light(1, 1, CRGB(0, 1, 1));   //türkis
    light(5, 1, CRGB(0, 1, 1));   //türkis
    light(9, 1, CRGB(0, 1, 1));   //türkis
    light(13, 1, CRGB(0, 1, 1));  //türkis
  }


  drawPages();
  drawStatus();
}

void drawStatus() {
  CRGB ledColor = CRGB(0, 0, 0);
  if (SMP.activeCopy) ledColor = CRGB(20, 20, 0);

  for (int s = 8; s <= 16; s++) {
    light(s, maxY, ledColor);
  }
  FastLEDshow();
}
void drawPages() {


  for (int p = 1; p <= maxPages; p++) {  // Assuming maxPages is 8
    int sum = 0;
    bool hasNotes = false;

    // Calculate sum of notes for this page
    for (int ix = 1; ix < maxX + 1; ix++) {
      for (int iy = 1; iy < maxY + 1; iy++) {
        if (note[((p - 1) * maxX) + ix][iy][0] > 0) {
          sum += note[((p - 1) * maxX) + ix][iy][0];
          hasNotes = true;
        }
      }
    }

    // Update lastPage if this page has notes
    if (hasNotes) {
      lastPage = p;
    }

    // Determine LED color based on the state
    CRGB ledColor;

    // If the page is the current one, set the LED to white
    if (SMP.page == p && SMP.edit == p) {
      ledColor = CRGB(0, 255, 0);  // Bright green if active page is same as edit page
    } else if (SMP.page == p) {
      ledColor = CRGB(0, 15, 0);  // White green for current page
    } else {
      // Determine color for non-current pages
      if (SMP.edit == p) {
        ledColor = SMP.page == p ? CRGB(0, 0, 35) : CRGB(0, 0, 5);  // Bright blue if active pattern is the same as edit, Dark blue otherwise
      } else {
        ledColor = hasNotes ? CRGB(20, 0, 20) : CRGB(1, 0, 0);  // Violet if has notes, Dark red otherwise
      }
    }

    // Set the LED color
    light(p, maxY, ledColor);
  }

  // Additional logic can be added here if you need to do something with lastPage
}



/************************************************
    DRAW SAMPLES
 *************************************************/
void drawSamples() {



  for (int ix = 1; ix < maxX + 1; ix++) {
    for (int iy = 1; iy < maxY + 1; iy++) {
      if (note[((SMP.edit - 1) * maxX) + ix][iy][0] > 0) {
        if (!mute[note[((SMP.edit - 1) * maxX) + ix][iy][0]]) {
          light(ix, iy, getCol(note[((SMP.edit - 1) * maxX) + ix][iy][0]));
        } else {
          light(ix, iy, getCol(note[((SMP.edit - 1) * maxX) + ix][iy][0]) / 24);
        }
      }
    }
  }
}



/************************************************
    TIMER
 *************************************************/

void drawTimer(int timer) {
  if (SMP.page == SMP.edit) {
    for (int y = 1; y < maxY; y++) {
      light(timer, y, CRGB(10, 0, 0));

      if (note[((SMP.page - 1) * maxX) + timer][y][0] > 0) {
        if (mute[note[((SMP.page - 1) * maxX) + timer][y][0]] == 0) {
          light(timer, y, CRGB(200, 200, 200));
        } else {
          light(timer, y, CRGB(00, 00, 00));
        }
      }
    }
  }
}

/************************************************
    USER CURSOR
 *************************************************/
void drawCursor() {
  freshnote = false;
  int x = SMP.x;
  int y = SMP.y;




  if (!freshnote) {
    if (dir == 1) pulse = pulse + 20 - (SMP.bpm / 30);
    if (dir == -1) pulse = pulse - 20 - (SMP.bpm / 30);
    if (pulse > 160) {
      dir = -1;
    }
    if (pulse < 0) {
      dir = 1;
    }
    light(x, y, CRGB(200 - (int)pulse, 200 - (int)pulse, 200 - (int)pulse));
  }
}

void setPage(int a) {
  Serial.print("setpage:");
  Serial.println(a);
  /*
  if (a > 0) {
    SMP.page = SMP.page + 1;
    currentMode->pos[1] = ::mapAndClampEncoderValue(Encoder &encoder, int min, int max)
    
    if (SMP.page > maxPages) {
      SMP.page = maxPages;
    }
    newPage = 0;
  }

  if (a < 0 && SMP.page > 0) {
    SMP.page = SMP.page - 1;

    if (SMP.page < 1) {
      SMP.page = 1;
    } else {
      positionRight = sensitivity;
      oldPosX = sensitivity;
      (knobRight.write(sensitivity);)
    }
    newPage = 0;
  }

  positionCenter = mapf(page, 1, maxPages, 1, sensitivityPage);
  knobCenter.write(positionCenter);
  */
}





void showIcons(String ico, CRGB colors) {
  if (ico == "samplepack") {
    for (int gx = 0; gx < 18; gx++) {
      light(icon_samplepack[gx][0], maxY - icon_samplepack[gx][1], colors);
    }
  }

  if (ico == "sample") {
    for (int gx = 0; gx < 19; gx++) {
      light(icon_sample[gx][0], maxY - icon_sample[gx][1], colors);
    }
  }

  if (ico == "loadsave") {
    for (int gx = 0; gx < 20; gx++) {
      light(icon_loadsave[gx][0], maxY - icon_loadsave[gx][1], colors);
    }
  }

  if (ico == "helper_load") {
    for (int gx = 0; gx < 3; gx++) {
      light(helper_load[gx][0], maxY - helper_load[gx][1], colors);
    }
  }


  if (ico == "helper_folder") {

    for (int gx = 0; gx < 3; gx++) {

      light(helper_folder[gx][0], maxY - helper_folder[gx][1], colors);
    }
  }

  if (ico == "helper_save") {
    for (int gx = 0; gx < 3; gx++) {
      light(helper_save[gx][0], maxY - helper_save[gx][1], colors);
    }
  }

  if (ico == "helper_select") {
    for (int gx = 0; gx < 3; gx++) {
      light(helper_select[gx][0], maxY - helper_select[gx][1], colors);
    }
  }

  if (ico == "bpm") {
    for (int gx = 0; gx < 38; gx++) {
      light(icon_bpm[gx][0], maxY - icon_bpm[gx][1], colors);
    }
  }
}




/************************************************
    SAVE
 *************************************************/
void savePattern() {
  Serial.println("Saving in slot #" + String(SMP.file));

  showNumber(SMP.file, CRGB(50, 0, 0), 0);
  FastLEDshow();

  drawNoSD();
  FastLEDclear();
  long maxdata = 0;

  char OUTPUTf[50];
  sprintf(OUTPUTf, "%d.txt", SMP.file);
  if (SD.exists(OUTPUTf)) {
    SD.remove(OUTPUTf);
  }
  myFile = SD.open(OUTPUTf, FILE_WRITE);
  if (myFile) {
    for (int sdx = 1; sdx < maxlen; sdx++) {
      for (int sdy = 1; sdy < maxY + 1; sdy++) {
        maxdata = maxdata + note[sdx][sdy][0];
        myFile.write(note[sdx][sdy][0]);
        myFile.write(note[sdx][sdy][1]);
      }
    }
  }

  myFile.close();
  if (maxdata == 0) {
    SD.remove(OUTPUTf);
  }
  delay(500);
  switchMode(&draw);
}

void autoSave() {
  drawNoSD();
  FastLEDclear();
  long maxdata = 0;

  char OUTPUTf[50];
  sprintf(OUTPUTf, "autosave.txt");
  if (SD.exists(OUTPUTf)) {
    SD.remove(OUTPUTf);
  }
  myFile = SD.open(OUTPUTf, FILE_WRITE);
  if (myFile) {
    for (int sdx = 1; sdx < maxlen; sdx++) {
      for (int sdy = 1; sdy < maxY + 1; sdy++) {
        maxdata = maxdata + note[sdx][sdy][0];
        myFile.write(note[sdx][sdy][0]);
        myFile.write(note[sdx][sdy][1]);
      }
    }
  }

  myFile.close();
  if (maxdata == 0) {
    SD.remove(OUTPUTf);
  }
}



void loadPattern() {

  Serial.println("Loading slot #" + String(SMP.file));


  drawNoSD();

  FastLEDclear();

  char OUTPUTf[50];
  sprintf(OUTPUTf, "%d.txt", SMP.file);
  if (SD.exists(OUTPUTf)) {
    showNumber(SMP.file, CRGB(0, 0, 50), 0);

    FastLEDshow();


    //LOAD/
    myFile = SD.open(OUTPUTf);
    if (myFile) {

      int sdry = 1;
      int sdrx = 1;
      while (myFile.available()) {
        int b = myFile.read();
        int v = myFile.read();
        note[sdrx][sdry][0] = b;
        note[sdrx][sdry][1] = v;
        sdry++;
        if (sdry > maxY) {
          sdry = 1;
          sdrx++;
        }
        if (sdrx > maxlen) sdrx = 1;
      }
    }
    myFile.close();

  } else {

    for (int nx = 1; nx < maxlen; nx++) {
      for (int ny = 1; ny < maxY + 1; ny++) {
        note[nx][ny][0] = 0;
        note[nx][ny][1] = 30;
      }
    }
    showNumber(fileID, CRGB(50, 50, 50), 0);
    FastLEDshow();
  }
  delay(500);
  switchMode(&draw);
}



void autoLoad() {
  drawNoSD();

  FastLEDclear();
  char OUTPUTf[50];
  sprintf(OUTPUTf, "autosave.txt");
  if (SD.exists(OUTPUTf)) {
    showNumber(fileID, CRGB(0, 0, 50), 0);
    FastLEDshow();
    //LOAD/
    myFile = SD.open(OUTPUTf);
    if (myFile) {
      int sdry = 1;
      int sdrx = 1;
      while (myFile.available()) {
        int b = myFile.read();
        int v = myFile.read();
        note[sdrx][sdry][0] = b;
        note[sdrx][sdry][1] = v;
        sdry++;
        if (sdry > maxY) {
          sdry = 1;
          sdrx++;
        }
        if (sdrx > maxlen) sdrx = 1;
      }
    }
    myFile.close();

  } else {
    for (int nx = 1; nx < maxlen; nx++) {
      for (int ny = 1; ny < maxY; ny++) {
        note[nx][ny][0] = 0;
        note[nx][ny][1] = 30;
      }
    }
  };
}