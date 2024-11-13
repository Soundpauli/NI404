  #define FASTLED_ALLOW_INTERRUPTS 1
  #define AUDIO_SAMPLE_RATE_EXACT 44100.0
  #define TargetFPS 27


  #include "Arduino.h"
  #include <Mapf.h>
  #include <WS2812Serial.h>  // leds
  #define USE_WS2812SERIAL   // leds
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
  int lastFile[9] = {0};
  
  const unsigned int defaultVelocity = 63;
  const unsigned int maxX = 16;
  const unsigned int maxY = 16;
  const unsigned int maxPages = 8;
  const unsigned int maxFiles = 9;  // 12 samples, 2 Synths
  const unsigned int maxFilters = 15;
  const unsigned int maxlen = (maxX * maxPages) + 1;
  const long ram = 15989988 - 5000 - 22168 - 5000;  // 16MB total, - 5kb for images+Icons, - 22kb for notes[]
  const unsigned int SONG_LEN = maxX * maxPages;
  const unsigned int maxBPM = 240;
  const unsigned int maxVolume = 10;
  const unsigned int maxFolders = 9;
  const unsigned int numPulsesForAverage = 24;  // Number of pulses to average over
  const unsigned int pulsesPerBar = 24 * 4;     // 24 pulses per quarter note, 4 quarter notes per bar
  const unsigned int barsToWait = 2;
  const unsigned int maxfilterResolution = 32;
  const unsigned int totalPulsesToWait = pulsesPerBar * barsToWait;

  // runtime
  //  variables for program logic
  unsigned int playNoteInterval = 150000;
  unsigned int RefreshTime = 1000 / TargetFPS;
  float marqueePos = maxX;
  bool shifted = false;
  bool movingForward = true;  // Variable to track the direction of movement
  unsigned volatile int lastUpdate;
  volatile unsigned int lastClockTime = 0;
  volatile unsigned int totalInterval = 0;
  volatile unsigned int clockCount = 0;
  bool hasNotes[maxPages + 1];
  unsigned int startTime = 0;    // Variable to store the start time
  bool noteOnTriggered = false;  // Flag to indicate if noteOn has been triggered
  volatile bool waitForFourBars = false;
  volatile unsigned int pulseCount = 0;
  bool sampleIsLoaded = false;
  bool unpaintMode, paintMode = false;
  String oldButtonString = "";
  String buttonString = "";
  String oldPos = "";
  unsigned int pagebeat, beat = 1;
  unsigned int samplePackID, fileID = 1;
  EXTMEM unsigned int lastPreviewedSample[maxFolders] = {};
  IntervalTimer playTimer;
  unsigned int lastPage = 1;
  unsigned int lastButtonPressTime = 0;
  bool resetTimerActive = false;
  EXTMEM unsigned int previousEncoderValues[4];
  float pulse = 1;
  int dir = 1;
  EXTMEM unsigned int tmp[maxlen][maxY + 1][2] = {};
  EXTMEM unsigned int original[maxlen][maxY + 1][2] = {};
  EXTMEM unsigned int note[maxlen][maxY + 1][2] = {};
  unsigned int sample_len[maxFiles];
  bool sampleLengthSet = false;
  bool isPlaying = false;  // global
  int PrevSampleRate = 1;
  EXTMEM int SampleRate[16] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
  EXTMEM int buttons[5], oldButtons[5] = { 0, 0, 0, 0, 0 };
  EXTMEM unsigned char sampled[maxFiles][ram / (maxFiles + 1)];

  const float pianoFrequencies[16] = {
    130.81,  // C3
    146.83,  // D3
    164.81,  // E3
    174.61,  // F3
    196.00,  // G3
    220.00,  // A3
    246.94,  // B3
    261.62,  // C4 (Mitte)
    293.66,  // D4
    329.62,  // E4
    349.22,  // F4
    391.99,  // G4
    440.00,  // A4
    493.88,  // B4
    523.25,  // C5
    587.33   // D5
  };


const String usedFiles[13] = {"samples/_1.wav",
                        "samples/_2.wav",
                        "samples/_3.wav",
                        "samples/_4.wav",
                        "samples/_5.wav",
                        "samples/_6.wav",
                        "samples/_7.wav",
                        "samples/_8.wav",
                        "samples/_9.wav",
                        "samples/_10.wav",
                        "samples/_11.wav",
                        "samples/_12.wav",
                        "samples/_13.wav"
                       };

const int number[10][24][2] = {
  {{1, 1}, {2, 1}, {3, 1}, {4, 1}, {4, 2}, {1, 3}, {4, 3}, {1, 4}, {4, 4}, {1, 5}, {4, 5}, {1, 6}, {4, 6}, {1, 7}, {4, 7}, {1, 8}, {1, 9}, {2, 9}, {3, 9}, {4, 9}, {1, 2}, {4, 8}, {4, 8}, {4, 8}}, //0
  {{3, 1}, {2, 2}, {3, 2}, {1, 3}, {3, 3}, {3, 4}, {3, 5}, {3, 6}, {3, 7}, {3, 8}, {2, 9}, {3, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}}, //1
  {{1, 1}, {2, 1}, {3, 1}, {4, 1}, {4, 2}, {4, 3}, {4, 4}, {1, 5}, {2, 5}, {3, 5}, {4, 5}, {1, 6}, {1, 7}, {1, 8}, {1, 9}, {2, 9}, {3, 9}, {4, 9}, {1, 2}, {4, 8}, {4, 8}, {4, 8}, {4, 8}, {4, 8}}, //2
  {{1, 1}, {2, 1}, {3, 1}, {4, 1}, {4, 2}, {4, 3}, {4, 4}, {2, 5}, {3, 5}, {4, 5}, {4, 6}, {4, 7}, {4, 8}, {1, 9}, {2, 9}, {3, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}}, //3
  {{1, 1}, {4, 1}, {1, 2}, {4, 2}, {1, 3}, {4, 3}, {1, 4}, {4, 4}, {1, 5}, {2, 5}, {3, 5}, {4, 5}, {4, 6}, {4, 7}, {4, 8}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}}, //4
  {{1, 1}, {2, 1}, {3, 1}, {4, 1}, {1, 2}, {1, 3}, {1, 4}, {1, 5}, {2, 5}, {3, 5}, {4, 5}, {4, 6}, {4, 7}, {4, 8}, {1, 9}, {2, 9}, {3, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}}, //5
  {{1, 1}, {2, 1}, {3, 1}, {4, 1}, {1, 2}, {1, 3}, {1, 4}, {1, 5}, {2, 5}, {3, 5}, {4, 5}, {1, 6}, {4, 6}, {1, 7}, {4, 7}, {1, 8}, {4, 8}, {1, 9}, {2, 9}, {3, 9}, {4, 9}, {4, 9}, {4, 9}, {4, 9}}, //6
  {{1, 1}, {2, 1}, {3, 1}, {4, 1}, {4, 2}, {4, 3}, {4, 4}, {4, 5}, {4, 6}, {4, 7}, {4, 8}, {4, 9}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}}, //7
  {{1, 1}, {2, 1}, {3, 1}, {4, 1}, {4, 2}, {1, 3}, {4, 3}, {1, 4}, {4, 4}, {1, 5}, {2, 5}, {3, 5}, {4, 5}, {1, 6}, {4, 6}, {1, 7}, {4, 7}, {1, 8}, {1, 9}, {2, 9}, {3, 9}, {4, 9}, {1, 2}, {4, 8}}, //8
  {{1, 1}, {2, 1}, {3, 1}, {4, 1}, {4, 2}, {1, 3}, {4, 3}, {1, 4}, {4, 4}, {1,  5}, {2, 5}, {3, 5}, {4, 5}, {4, 6}, {4, 7}, {1, 9}, {2, 9}, {3, 9}, {4, 9}, {1, 2}, {4, 8}, {4, 8}, {4, 8}, {4, 8}}
};
const int logo[72][2] = {
    {2, 2}, {3, 2}, {4, 2}, {5, 2}, {5, 3}, {6, 4}, {7, 5}, {8, 6}, {9, 7},
    {10, 8}, {11, 9}, {12, 10}, {12, 9}, {12, 8}, {12, 7}, {13, 7}, {14, 7},
    {15, 7}, {15, 8}, {15, 9}, {15, 10}, {15, 11}, {15, 12}, {15, 13}, {15, 14},
    {15, 15}, {14, 15}, {13, 15}, {12, 15}, {12, 14}, {11, 13}, {10, 12}, {9, 11},
    {8, 10}, {7, 9}, {6, 8}, {5, 7}, {5, 8}, {5, 9}, {5, 10}, {5, 11}, {5, 12},
    {5, 13}, {5, 14}, {5, 15}, {4, 15}, {3, 15}, {2, 15}, {2, 14}, {2, 13},
    {2, 12}, {2, 11}, {2, 10}, {2, 9}, {2, 8}, {2, 7}, {2, 6}, {2, 5}, {2, 4},
    {2, 3}, {12, 2}, {13, 2}, {14, 2}, {15, 2}, {15, 3}, {15, 4}, {15, 5},
    {14, 5}, {13, 5}, {12, 5}, {12, 4}, {12, 3}
};
 
const int icon_samplepack[18][2] = {{2, 1}, {2, 2}, {3, 2}, {2, 3}, {2, 4}, {4, 4}, {1, 5}, {2, 5}, {4, 5}, {5, 5}, {1, 6}, {2, 6}, {4, 6}, {4, 7}, {3, 8}, {4, 8}, {3, 9}, {4, 9}};
const int icon_sample[19][2] = {{3, 1}, {3, 2}, {3, 3}, {4, 3}, {3, 5}, {4, 2}, {5, 3}, {3, 4}, {3, 5}, {3, 6}, {1, 7}, {2, 7}, {3, 7}, {1, 8}, {2, 8}, {3, 8}, {1, 9}, {2, 9}, {3, 9}};
const int icon_loadsave[20][2] = {{3, 1}, {3, 2}, {3, 3}, {3, 4}, {1, 5}, {2, 5}, {3, 5}, {4, 5}, {5, 5}, {2, 6}, {3, 6}, {4, 6}, {3, 7}, {1, 8}, {5, 8}, {1, 9}, {2, 9}, {3, 9}, {4, 9}, {5, 9}};
const int icon_bpm[38][2] = {{2, 11}, {3, 11}, {4, 11}, {7, 11}, {8, 11}, {9, 11}, {11, 11}, {15, 11}, {2, 12}, {4, 12}, {7, 12}, {9, 12}, {11, 12}, {12, 12}, {14, 12}, {15, 12}, {2, 13}, {3, 13}, {4, 13}, {5, 13}, {7, 13}, {8, 13}, {9, 13}, {11, 13}, {13, 13}, {15, 13}, {2, 14}, {5, 14}, {7, 14}, {11, 14}, {15, 14}, {2, 15}, {3, 15}, {4, 15}, {5, 15}, {7, 15}, {11, 15}, {15, 15}};
const int helper_load[3][2] = {{1, 15}, {2, 15}, {3, 15}}; 
const int helper_folder[5][2] = { {6, 13}, {6, 14}, {6, 15}, {7, 14}, {7, 15}};
const int helper_seek[2][2] = {{10, 15}, {10, 14}}; 

const int helper_vol[5][2] = {{9, 13},  {11, 13}, {10, 15}, {9, 14}, {11, 14}};
//for 3 encoders:
const int helper_vol2[5][2] = {{6, 13},  {8, 13}, {7, 15}, {6, 14}, {8, 14}};

const int helper_bpm[7][2] = {{13, 13}, {13, 14}, {13, 15}, {14, 14}, {15, 14}, {14, 15}, {15, 15}};


const int helper_save[3][2] =  {{5, 15}, {6, 15}, {7, 15}};
const int helper_select[3][2] = {{13, 15}, {14, 15}, {15, 15}}; 

const int noSD[48][2] =
{ {2, 4}, {3, 4}, {4, 4}, {5, 4}, {7, 4}, {8, 4}, {9, 4}, {12, 4}, {13, 4}, {14, 4}, {15, 4},
  {2, 5}, {5, 5}, {7, 5}, {9, 5}, {10, 5}, {12, 5}, {15, 5},
  {2, 6}, {7, 6}, {10, 6}, {15, 6},
  {2, 7}, {3, 7}, {4, 7}, {5, 7}, {7, 7}, {10, 7}, {13, 7}, {14, 7}, {15, 7},
  {5, 8}, {7, 8}, {10, 8}, {13, 8},
  {2, 9}, {5, 9}, {7, 9}, {9, 9}, {10, 9},
  {2, 10}, {3, 10}, {4, 10}, {5, 10}, {7, 10}, {8, 10}, {9, 10}, {13, 10}
};

  CRGB leds[NUM_LEDS];

  struct Mode {
    String name;
    unsigned int minValues[4];
    unsigned int maxValues[4];
    unsigned int pos[4];
  };

  // Declare the modes aka max states for encoders and their min/max values
  // min (l-m-m2-r),max(l-m-m2-r), pos(l-m-m2-r)
  Mode draw = { "DRAW", { 1, 1, 1, 0 }, { maxY, maxPages, maxY, maxfilterResolution }, { 1, 1, 1, maxfilterResolution } };
  Mode singleMode = { "SINGLE", { 1, 1, 1, 0 }, { maxY, maxX, maxY, maxfilterResolution }, { 1, 1, 1, maxfilterResolution } };
  Mode volume_bpm = { "VOLUME_BPM", { 1, 0, 40, 1 }, { maxVolume, 0, maxBPM, maxVolume }, { 1, 0, 100, 9 } };
  Mode noteShift = { "NOTE_SHIFT", { 7, 0, 7, 0 }, { 9, 0, 9, 0 }, { 8, 0, 8, 0 } };
  Mode velocity = { "VELOCITY", { 1, 1, 1, 1 }, { 1, 1, maxY, maxY }, { 1, 1, 10, 10 } };
  Mode set_Wav = { "SET_WAV", { 0, 1, 1, 0 }, { 999, maxFolders, 999, 999 }, { 0, 0, 1, 999 } };
  Mode set_SamplePack = { "SET_SAMPLEPACK", { 1, 1, 1, 1 }, { 1, 1, 99, 99 }, { 1, 1, 1, 1 } };
  Mode menu = { "MENU", { 1, 1, 1, 1 }, { 1, 1, 12, 12 }, { 1, 1, 1, 1 } };

  // Declare currentMode as a global variable
  Mode *currentMode = &draw;

  // Declare the device struct
  struct Device {
    unsigned int singleMode;  // single Sample Mod
    unsigned int currentChannel;
    unsigned int vol;       // volume
    unsigned int bpm;       // bpm
    unsigned int velocity;  // velocity
    unsigned int page;      // current page
    unsigned int edit;      // edit mode or plaing mode?
    unsigned int file;      // current selected save/load id
    unsigned int pack;      // current selected samplepack id
    unsigned int wav;       // current selected sample id
    unsigned int folder;    // current selected folder id
    bool activeCopy;        // is copy/paste active?
    unsigned int x;         // cursor X
    unsigned int y;         // cursor Y
    unsigned int seek;      // skipped into sample
    unsigned int seekEnd;
    unsigned int smplen;  // overall selected samplelength
    unsigned int shiftX;   // note Shift
    unsigned int shiftY;   // note Shift
    unsigned int filter_knob[maxFilters];
    unsigned int mute[maxY];
  };

  EXTMEM Device SMP = { false, 1, 10, 100, 10, 1, 1, 1, 1, 1, 0, false, 1, 16, 0, 0, 0, 0, 0, { maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution, maxfilterResolution }, {} };

  Encoder encoders[4] = {
    Encoder(22, 5),   // 0, LEFT KNOB  (UP / DOWN, REMOVE TRIGGER, doubleTab: Enter/Exit Single-Sample-Mode)
    Encoder(2, 4),    // 1, RIGHT KNOB (LEFT / RIGHT, ADD TRIGGER)
    Encoder(14, 9),   // 2, MIDDLE LEFT KNOB (SELECT PAGE, double-tab+hold: set Accents), Long hold(+l/r knob): Select BPM OR VOLUME
                      // REMOVE IF YOU ONLY HAVE 3
    Encoder(33, 32),  // 3 Middle Right Knob
  };

  // DO YOU HAVE 4 ENCODERS? (s.above), set pins to 99,99 or similar
  bool isEncoder4Defined = true;
  // Filtering and Sample-End-Seeking is then disabled, volume is on left Knob


  Switch multiresponseButton1 = Switch(15);  // Left-Knob: (doubleTab: Enter/Exit Single-Sample-Mode)
  Switch multiresponseButton3 = Switch(16);  // Middle-Left: VOLUME / BPM
  Switch multiresponseButton4 = Switch(41);  // Middle-Right:
  Switch multiresponseButton2 = Switch(3);   // right-Knob:  (double-tab+hold: set Accents), Long hold(+l/r knob): Select BPM OR VOLUME

  EXTMEM arraysampler _samplers[13];
  AudioPlayArrayResmp *voices[] = { &sound0, &sound1, &sound2, &sound3, &sound4, &sound5, &sound6, &sound7, &sound8, &sound9, &sound10, &sound11, &sound12 };
  AudioEffectEnvelope *envelopes[] = { &envelope0, &envelope1, &envelope2, &envelope3, &envelope4, &envelope5, &envelope6, &envelope7, &envelope8, &envelope9, &envelope10, &envelope11, &envelope12, &envelope13, &envelope14 };
  AudioFilterStateVariable *filters[] = { &filter0, &filter1, &filter2, &filter3, &filter4, &filter5, &filter6, &filter7, &filter8, &filter9, &filter10, &filter11, &filter12, &filter13, &filter14 };

  void serialprint(...) {
  }

  void serialprintln(...) {
  }

  void allOff() {
    for (AudioEffectEnvelope *envelope : envelopes) {
      envelope->noteOff();
    }
  }

  void FastLEDclear() {
    FastLED.clear();
  }

  void FastLEDshow() {
    if (millis() - lastUpdate > RefreshTime) {
      lastUpdate = millis();
      FastLED.show();
    }
  }

  void drawNoSD() {
    while (!SD.begin(10)) {
      FastLEDclear();
      for (unsigned int gx = 0; gx < 48; gx++) {
        light(noSD[gx][0], maxY - noSD[gx][1], CRGB(50, 0, 0));
      }
      FastLEDshow();
      delay(1000);
    }
  }

  void copyPosValues(Mode *source, Mode *destination) {
    for (unsigned int i = 0; i < 3; i++) {
      destination->pos[i] = source->pos[i];
    }
  }

  void setVelocity() {
    if (currentMode->pos[2] != SMP.velocity) {

      if (!SMP.singleMode) {
        note[SMP.x][SMP.y][1] = round(mapf(currentMode->pos[2], 1, maxY, 1, 127));
      } else {
        serialprintln("Overal Velocity: " + String(currentMode->pos[2]));
        for (unsigned int nx = 1; nx < maxlen; nx++) {
          for (unsigned int ny = 1; ny < maxY + 1; ny++) {
            if (note[nx][ny][0] == SMP.currentChannel)
              note[nx][ny][1] = round(mapf(currentMode->pos[2], 1, maxY, 1, 127));
          }
        }
      }
    }
    drawVelocity(CRGB(0, 40, 0));
  }

  void setup() {
    delay(200);
    Serial.begin(115200);

    if (CrashReport) {
      Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
      Serial.print(CrashReport);
      delay(1000);
    }

    usbMIDI.setHandleClock(myClock);
    usbMIDI.setHandleStart(handleStart);
    usbMIDI.setHandleStop(handleStop);
    usbMIDI.setHandleSongPosition(handleSongPosition);  // Set the handler for SPP messages
    usbMIDI.setHandleTimeCodeQuarterFrame(handleTimeCodeQuarterFrame);
    usbMIDI.setHandleNoteOn(handleNoteOn);
    usbMIDI.setHandleNoteOff(handleNoteOff);

    EEPROM.get(0, samplePackID);
    serialprint("SamplePackID:");
    serialprintln(samplePackID);

    if (samplePackID == NAN || samplePackID == 0) {
      serialprint("NO SAMPLEPACK SET! Defaulting to 1");
      samplePackID = 1;
    }

    pinMode(0, INPUT_PULLDOWN);
    pinMode(3, INPUT_PULLDOWN);
    pinMode(16, INPUT_PULLDOWN);
    FastLED.addLeds<WS2812SERIAL, DATA_PIN, BRG>(leds, NUM_LEDS);
    //showIntro();
    
    serialprint("Initializing SD card...");
    drawNoSD();

    //set maxFiles in folder and show loading...
    for (int f = 0; f <= maxFolders; f++) {
      FastLEDclear();
      for (unsigned int i = 1; i < 99; i++) {
        char OUTPUTf[50];
          sprintf(OUTPUTf, "samples/%d/_%d.wav", f, i+(f*100));
        if (SD.exists(OUTPUTf)) {
          lastFile[f] = i+(f*100);
          if (i % 16 == 0) {
          light(1, i / 16 + 1, CRGB(255, 255, 255));
          } else {
          light(i % 16, i / 16 + 1, col_Folder[f]);
          }    
          }
      }
      showNumber(f,CRGB(200,200,200),0);
      showIcons("icon_sample", col_Folder[f]);
      FastLED.show();
    }

    FastLEDclear();

    loadSamplePack(samplePackID);
    //for (unsigned int z = 1; z < maxFiles; z++) {
    //  loadSample(samplePackID, z);
    //}

    for (unsigned int vx = 1; vx < SONG_LEN + 1; vx++) {
      for (unsigned int vy = 1; vy < maxY + 1; vy++) {
        note[vx][vy][1] = defaultVelocity;
      }
    }

    _samplers[0].addVoice(sound0, mixer4, 3, envelope0);

    _samplers[1].addVoice(sound1, mixer1, 0, envelope1);
    _samplers[2].addVoice(sound2, mixer1, 1, envelope2);
    _samplers[3].addVoice(sound3, mixer1, 2, envelope3);
    _samplers[4].addVoice(sound4, mixer1, 3, envelope4);

    _samplers[5].addVoice(sound5, mixer2, 0, envelope5);
    _samplers[6].addVoice(sound6, mixer2, 1, envelope6);
    _samplers[7].addVoice(sound7, mixer2, 2, envelope7);
    _samplers[8].addVoice(sound8, mixer2, 3, envelope8);

    _samplers[9].addVoice(sound9, mixer3, 0, envelope9);
    _samplers[10].addVoice(sound10, mixer3, 1, envelope10);
    _samplers[11].addVoice(sound11, mixer3, 2, envelope11);
    _samplers[12].addVoice(sound12, mixer3, 3, envelope12);

    //_samplers[13].addVoice(sound13, mixer4, 0, envelope13);
    //_samplers[15].addVoice(sound15, mixer4, 2 , envelope15);

    mixer1.gain(0, 0.25);
    mixer1.gain(1, 0.25);
    mixer1.gain(2, 0.25);
    mixer1.gain(3, 0.25);

    mixer2.gain(0, 0.25);
    mixer2.gain(1, 0.25);
    mixer2.gain(2, 0.25);
    mixer2.gain(3, 0.25);

    mixer3.gain(0, 0.25);
    mixer3.gain(1, 0.25);
    mixer3.gain(2, 0.25);
    mixer3.gain(3, 0.25);

    mixer4.gain(0, 0.25);
    mixer4.gain(1, 0.25);
    mixer4.gain(2, 0.25);
    mixer4.gain(3, 0.25);

    mixer_end.gain(0, 0.25);
    mixer_end.gain(1, 0.25);
    mixer_end.gain(2, 0.25);
    mixer_end.gain(3, 0.25);

    // configure what the synth will sound like
    //FIRST SYNTH
    sound13.begin(WAVEFORM_SAWTOOTH);
    sound13.amplitude(0.3);
    sound13.frequency(261.62);
    sound13.phase(0);

    envelope13.attack(0);
    envelope13.decay(200);
    envelope13.sustain(1);
    envelope13.release(500);


    //SECOND SYNTH
    sound14.begin(WAVEFORM_SQUARE);
    sound14.amplitude(0.5);
    sound14.frequency(261.62);  //C
    sound14.phase(0);

    envelope14.attack(0);
    envelope14.decay(200);
    envelope14.sustain(1);
    envelope14.release(500);  // Release time set to 200 ms

    // set filters and envelopes for all sounds
    for (unsigned int i = 1; i < maxFilters; i++) {
      envelopes[i]->attack(0);
      filters[i]->octaveControl(6.0);
      filters[i]->resonance(0.7);
      filters[i]->frequency(100 * 32);
    }

    for (unsigned int i = 0; i < maxFiles; i++) {
      serialprint("START VOICE:");
      serialprintln(i);
      voices[i]->enableInterpolation(true);
    }



    AudioInterrupts();

    // set BPM:100
    SMP.bpm = 100;
    playTimer.begin(playNote, playNoteInterval);

    // turn on the output
    sgtl5000_1.enable();
    sgtl5000_1.volume(0.9);
    AudioMemory(64);

    autoLoad();

    // button1
    multiresponseButton1.setSingleClickCallback(&buttonCallbackFunction, (void *)"1");
    multiresponseButton1.setLongPressCallback(&buttonCallbackFunction, (void *)"2");
    multiresponseButton1.setDoubleClickCallback(&buttonCallbackFunction, (void *)"3");
    multiresponseButton1.setReleasedCallback(&buttonCallbackFunction, (void *)"a");
    multiresponseButton1.setPushedCallback(&buttonCallbackFunction, (void *)"y");
    // button2
    multiresponseButton2.setSingleClickCallback(&buttonCallbackFunction, (void *)"4");
    multiresponseButton2.setLongPressCallback(&buttonCallbackFunction, (void *)"5");
    multiresponseButton2.setDoubleClickCallback(&buttonCallbackFunction, (void *)"6");
    multiresponseButton2.setReleasedCallback(&buttonCallbackFunction, (void *)"b");
    // button3
    multiresponseButton3.setSingleClickCallback(&buttonCallbackFunction, (void *)"7");
    multiresponseButton3.setLongPressCallback(&buttonCallbackFunction, (void *)"8");
    multiresponseButton3.setDoubleClickCallback(&buttonCallbackFunction, (void *)"9");
    multiresponseButton3.setReleasedCallback(&buttonCallbackFunction, (void *)"c");
    multiresponseButton3.setPushedCallback(&buttonCallbackFunction, (void *)"x");

    // button4
    multiresponseButton4.setSingleClickCallback(&buttonCallbackFunction, (void *)"h");
    multiresponseButton4.setLongPressCallback(&buttonCallbackFunction, (void *)"s");
    multiresponseButton4.setDoubleClickCallback(&buttonCallbackFunction, (void *)"d");
    multiresponseButton4.setReleasedCallback(&buttonCallbackFunction, (void *)"f");
    multiresponseButton4.setPushedCallback(&buttonCallbackFunction, (void *)"g");
  }

  void buttonCallbackFunction(void *s) {
    struct ButtonMapping {
      const char *input;
      int buttonIndex;
      int value;
    };

    ButtonMapping mappings[] = {
      { "a", 1, 9 }, 
      { "1", 1, 1 }, 
      { "2", 1, 2 }, 
      { "y", 1, 5 }, 
      { "3", 1, 3 },  // button 1
      { "b", 2, 9 },
      { "4", 2, 1 },
      { "5", 2, 2 },
      { "6", 2, 3 },  // button 2
      { "c", 3, 9 },
      { "7", 3, 1 },
      { "8", 3, 2 },
      { "x", 3, 5 },
      { "9", 3, 3 },  // button 3
      { "f", 4, 9 },
      { "h", 4, 1 },
      { "s", 4, 2 },
      { "g", 4, 5 },
      { "d", 4, 3 }  // button 4
    };

    for (const auto &mapping : mappings) {
      if (strcmp(static_cast<const char *>(s), mapping.input) == 0) {
        buttons[mapping.buttonIndex] = mapping.value;
        break;
      }
    }

    if (memcmp(buttons, oldButtons, sizeof(buttons)) != 0) {
      memcpy(oldButtons, buttons, sizeof(buttons));  // Update oldButtons
      lastButtonPressTime = millis();
      resetTimerActive = true;
    }
  }

  void checkMode() {
    // Create the button string combination

    if (!isEncoder4Defined)
      buttons[4] = 0;

    buttonString = String(buttons[1]) + String(buttons[2]) + String(buttons[4]) + String(buttons[3]);

    // Toggle play/pause in draw or single mode
    String playButtonString = isEncoder4Defined ? "0010" : "1001";
    if ((currentMode == &draw || currentMode == &singleMode || currentMode == &noteShift) && buttonString == playButtonString) {
      togglePlay(isPlaying);
    }

    // Shift notes around in single mode after dblclick of button 4
    String shiftButtonString = isEncoder4Defined ? "0020" : "2000";
    if (currentMode == &singleMode && buttonString == shiftButtonString) {

      SMP.shiftX = 8;
      encoders[2].write(8 * 4);

      SMP.shiftY = 8;
      encoders[2].write(8 * 4);


      unsigned int patternLength = lastPage * maxX;

      for (unsigned int nx = 1; nx <= patternLength; nx++) {  // Start from 1
        for (unsigned int ny = 1; ny <= maxY; ny++) {         // Start from 1
          original[nx][ny][0] = 0;
          original[nx][ny][1] = defaultVelocity;
        }
      }

      // Step 2: Backup non-current channel notes into the original array
      for (unsigned int nx = 1; nx <= patternLength; nx++) {
        for (unsigned int ny = 1; ny <= maxY; ny++) {
          if (note[nx][ny][0] != SMP.currentChannel) {
            original[nx][ny][0] = note[nx][ny][0];
            original[nx][ny][1] = note[nx][ny][1];
          }
        }
      }




      // Switch to note shift mode
      switchMode(&noteShift);
      SMP.singleMode = true;
    }

    if (currentMode == &noteShift && buttonString == "0100") {
      switchMode(&singleMode);
      SMP.singleMode = true;
    }

    // Switch to volume mode in draw or single mode
    if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0200") {
      switchMode(&volume_bpm);
    }

    // Toggle copy/paste in draw mode
    if (currentMode == &draw && buttonString == "1100") {
      toggleCopyPaste();
    }

    // Handle velocity switch in single mode
    if ((currentMode == &singleMode) && buttonString == "0300") {
      int velo = round(mapf(note[SMP.x][SMP.y][1], 1, 127, 1, maxY * 4));
      serialprintln(velo);
      SMP.velocity = velo;
      switchMode(&velocity);
      SMP.singleMode = true;
      encoders[2].write(velo);
    }

    // Handle velocity switch in draw mode
    if ((currentMode == &draw) && buttonString == "0300") {
      int velo = round(mapf(note[SMP.x][SMP.y][1], 1, 127, 1, maxY * 4));
      serialprintln(velo);
      SMP.velocity = velo;
      SMP.singleMode = false;
      switchMode(&velocity);
      encoders[2].write(velo);
    }

    // Print button string if it has changed
    if (oldButtonString != buttonString) {
      serialprintln(buttonString);
      oldButtonString = buttonString;
    }

    // Clear page in draw or single mode
    if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "2002") {
      clearPage();
    }

    // Switch to single mode from velocity mode
    if (currentMode == &velocity && SMP.singleMode && buttonString == "0900") {
      switchMode(&singleMode);
      SMP.singleMode = true;
    }

    // Switch to draw mode from velocity mode
    if (currentMode == &velocity && !SMP.singleMode && buttonString == "0900") {
      switchMode(&draw);
    }

    // Switch to draw mode from volume mode
    if (currentMode == &volume_bpm && buttonString == "0900") {
      switchMode(&draw);
      // setvol = false;
    }

    // Disable paint and unpaint mode in draw or single mode
    if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0909") {
      paintMode = false;
      unpaintMode = false;
    }

    // Disable paint and unpaint mode in draw or single mode
    if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0009") {
      paintMode = false;
      unpaintMode = false;
    }

    // Disable paint and unpaint mode in draw or single mode
    if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "9000") {
      unpaintMode = false;
      paintMode = false;
    }

    // Menu Load/Save
    if ((currentMode == &draw) && buttonString == "0202") {
      switchMode(&menu);
    } else if ((currentMode == &menu) && buttonString == "0001") {
      paintMode = false;
      unpaintMode = false;
      switchMode(&draw);
    } else if ((currentMode == &menu) && buttonString == "0100") {
      savePattern(false);
    } else if ((currentMode == &menu) && buttonString == "1000") {
      loadPattern(false);
    }

    // Search Wave + Load + Exit
    if ((currentMode == &singleMode) && buttonString == "2200") {
      //toDO: set current encoder to loaded file

      switchMode(&set_Wav);
    } else if ((currentMode == &set_Wav) && buttonString == "1000") {
      loadWav();
    } else if ((currentMode == &set_Wav) && buttonString == "0001") {
      switchMode(&singleMode);
      SMP.singleMode = true;
    }

    // Set SamplePack + Load + Save + Exit
    if ((currentMode == &draw) && buttonString == "2200") {
      switchMode(&set_SamplePack);
    } else if ((currentMode == &set_SamplePack) && buttonString == "0100") {
      saveSamplePack(SMP.pack);
    } else if ((currentMode == &set_SamplePack) && buttonString == "1000") {
      loadSamplePack(SMP.pack);
    } else if ((currentMode == &set_SamplePack) && buttonString == "0001") {
      switchMode(&draw);
    }

    // Toggle mute in draw or single mode
    if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0100") {
      toggleMute();
    }

    // Enable paint mode in draw or single mode
    if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0002") {
      paintMode = true;
    }

    // Unpaint and delete active copy in draw or single mode
    if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "2000") {
      unpaint();
      unpaintMode = true;
      deleteActiveCopy();
    }

    // Not used in draw or single mode
    if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0001") {
      // not used...
    }

    // Normal paint in draw or single mode
    if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "0005") {
      paintMode = false;
      unpaintMode = false;
      paint();
    }

    // Normal unpaint in draw or single mode
    if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "5000") {
      paintMode = false;
      unpaintMode = false;
    }

    // Unpaint in draw or single mode
    if ((currentMode == &draw || currentMode == &singleMode) && buttonString == "1000") {
      unpaint();
    }

    // Toggle SingleMode
    if (currentMode == &singleMode && buttonString == "3000") {
      copyPosValues(&singleMode, &draw);
      switchMode(&draw);
    } else if (currentMode == &draw && buttonString == "3000" && ((SMP.y - 1 >= 1 && SMP.y - 1 <= maxFiles) || SMP.y - 1 > 12)) {
      SMP.currentChannel = SMP.y - 1;
      copyPosValues(&draw, &singleMode);
      switchMode(&singleMode);
      SMP.singleMode = true;
    }
  }

  void shiftNotes() {
    unsigned int patternLength = lastPage * maxX;
    if (currentMode->pos[2] != SMP.shiftX) {
      // Determine shift direction (+1 or -1)
      int shiftDirectionX = 0;
      if (currentMode->pos[2] > SMP.shiftX) {
        shiftDirectionX = 1;
      } else {
        shiftDirectionX = -1;
      }
      SMP.shiftX = 8;
      encoders[2].write(8 * 4);
      currentMode->pos[2] = 8;
      // Step 1: Clear the tmp array
      for (unsigned int nx = 1; nx <= patternLength; nx++) {  // Start from 1
        for (unsigned int ny = 1; ny <= maxY; ny++) {         // Start from 1
          tmp[nx][ny][0] = 0;
          tmp[nx][ny][1] = defaultVelocity;
        }
      }

      // Step 2: Shift notes of the current channel into tmp array
      for (unsigned int nx = 1; nx <= patternLength; nx++) {
        for (unsigned int ny = 1; ny <= maxY; ny++) {
          if (note[nx][ny][0] == SMP.currentChannel) {
            int newposX = nx + shiftDirectionX;
            
            // Handle wrapping around the edges
            if (newposX < 1) {
              newposX = patternLength;
            } else if (newposX > patternLength) {
              newposX = 1;
            }
            tmp[newposX][ny][0] = SMP.currentChannel;
            tmp[newposX][ny][1] = note[nx][ny][1];
          }
        }
      }
      shifted = true;
    }

    if (currentMode->pos[0] != SMP.shiftY) {
      // Determine shift direction (+1 or -1)
      int shiftDirectionY = 0;
      if (currentMode->pos[0] > SMP.shiftY) {
        shiftDirectionY = -1;
      } else {
        shiftDirectionY = +1;
      }
      encoders[0].write(8 * 4);
      currentMode->pos[0] = 8;
      SMP.shiftY = 8;
      // Step 1: Clear the tmp array
      for (unsigned int nx = 1; nx <= patternLength; nx++) {  // Start from 1
        for (unsigned int ny = 1; ny <= maxY; ny++) {         // Start from 1
          tmp[nx][ny][0] = 0;
          tmp[nx][ny][1] = defaultVelocity;
        }
      }

      // Step 2: Shift notes of the current channel into tmp array
      for (unsigned int nx = 1; nx <= patternLength; nx++) {
        for (unsigned int ny = 1; ny <= maxY; ny++) {
          if (note[nx][ny][0] == SMP.currentChannel) {
            int newposY = ny + shiftDirectionY;
            // Handle wrapping around the edges
            if (newposY < 1) {
              newposY = maxY;
            } else if (newposY > maxY) {
              newposY = 1;
            }
            tmp[nx][newposY][0] = SMP.currentChannel;
            tmp[nx][newposY][1] = note[nx][ny][1];
          }
        }
      }
      shifted = true;
    }
    

    if (shifted){
      // Step 3: Copy original notes of other channels back to the note array
      for (unsigned int nx = 1; nx <= patternLength; nx++) {
        for (unsigned int ny = 1; ny <= maxY; ny++) {
          if (original[nx][ny][0] != SMP.currentChannel) {
            note[nx][ny][0] = original[nx][ny][0];
            note[nx][ny][1] = original[nx][ny][1];
          } else  {
            note[nx][ny][0] = 0;
            note[nx][ny][1] = defaultVelocity;
          }
          if (tmp[nx][ny][0] == SMP.currentChannel) {
            note[nx][ny][0] = tmp[nx][ny][0];
            note[nx][ny][1] = tmp[nx][ny][1];
          }
        }
      }
      shifted=false;
    }
  }


  void toggleMute() {
    if (SMP.mute[SMP.y - 1]) {
      SMP.mute[SMP.y - 1] = false;
      //envelopes[SMP.y - 1]->release(11880 / 2);
    } else {
      // wenn leer oder nicht gemuted:
      SMP.mute[SMP.y - 1] = true;
      // envelopes[SMP.y - 1]->release(120);
    }
  }

  void deleteActiveCopy() {
    SMP.activeCopy = false;
  }

  void togglePlay(bool &value) {
    updateLastPage();
    deleteActiveCopy();
    value = !value;  // Toggle the boolean value
    serialprintln(value ? "Playing" : "Paused");

    if (value == false) {
      autoSave();
      envelope0.noteOff();
      allOff();
    }

    beat = 1;
    pagebeat = 1;
    SMP.page = 1;
  }

  void playNote() {

    if (isPlaying) {

      // print "beat of page of total pages, using two digits to display the number" using sprintf
      //  format: beat/pagebeat -- page/totalpages
      //char buffer[20];
      //sprintf(buffer, "%02d - %02d __ %d/%d", beat, pagebeat, SMP.page, lastPage);
      //Serial.println(buffer);

      for (unsigned int b = 1; b < maxY + 1; b++) {
        if (note[beat][b][0] > 0 && !SMP.mute[note[beat][b][0]]) {

          if (note[beat][b][0] < 9) {
            _samplers[note[beat][b][0]].noteEvent(12 * SampleRate[note[beat][b][0]] + b - (note[beat][b][0] + 1), note[beat][b][1], true, false);
            // usbMIDI.sendNoteOn(b, note[beat][b][1], 1);
          }

          if (note[beat][b][0] == 14) {
            float frequency = pianoFrequencies[b] / 2;  // y-Wert ist 1-basiert, Array ist 0-basiert
            sound14.frequency(frequency);
            float WaveFormVelocity = mapf(note[beat][b][1], 1, 127, 0.0, 1.0);
            sound14.amplitude(WaveFormVelocity);
            envelope14.noteOn();
            startTime = millis();    // Record the start time
            noteOnTriggered = true;  // Set the flag so we don't trigger noteOn again
          }

          if (note[beat][b][0] == 13) {
            float frequency = pianoFrequencies[b - 1] / 2;  // y-Wert ist 1-basiert, Array ist 0-basiert
            sound13.frequency(frequency);
            float WaveFormVelocity = mapf(note[beat][b][1], 1, 127, 0.0, 1.0);
            sound13.amplitude(WaveFormVelocity);
            envelope13.noteOn();
            startTime = millis();    // Record the start time
            noteOnTriggered = true;  // Set the flag so we don't trigger noteOn again
          }
        }
      }

      beat++;

      // pagebeat is always 1-16, calcs from beat. if beat 1-16, pagebeat is 1, if beat 17-32, pagebeat is 2, etc.
      pagebeat = (beat - 1) % maxX + 1;

      // midi functions
      if (waitForFourBars && pulseCount >= totalPulsesToWait) {
        beat = 1;
        pagebeat = 1;
        SMP.page = 1;
        isPlaying = true;
        serialprintln("4 Bars Reached");
        waitForFourBars = false;  // Reset for the next start message
        // AudioMemoryUsageMaxReset();
      }

      if (beat > SMP.page * maxX) {
        SMP.page = SMP.page + 1;
        if (SMP.page > maxPages)
          SMP.page = 1;
        if (SMP.page > lastPage)
          SMP.page = 1;
      }

      if (beat > maxX * lastPage) {
        beat = 1;
        SMP.page = 1;
        pagebeat = 1;
      }
    }
    playTimer.end();  // Stop the timer
    playTimer.begin(playNote, playNoteInterval);
  }

  void unpaint() {
    serialprintln("UNpaint");
    paintMode = false;
    unsigned int y = SMP.y;
    unsigned int x = (SMP.edit - 1) * maxX + SMP.x;
    if (!SMP.singleMode) {
      serialprintln("deleting" + String(x));
      note[x][y][0] = 0;
    } else {
      if (note[x][y][0] == SMP.currentChannel)
        note[x][y][0] = 0;
    }
    updateLastPage();
    FastLEDshow();
  }

  void paint() {
    unsigned int sample = 1;
    unsigned int x = (SMP.edit - 1) * maxX + SMP.x;
    unsigned int y = SMP.y;

    if (!SMP.singleMode) {
      if ((y > 1 && y <= maxFiles + 1) || y >= maxY - 2) {
        if (note[x][y][0] == 0) {
          note[x][y][0] = (y - 1);
        } else {
          if ((note[x][y][0] + 1) > 8) note[x][y][0] = 0;  // do not cycle over the synth voices

          note[x][y][0] = note[x][y][0] + sample;
          for (unsigned int vx = 1; vx < maxX + 1; vx++) {
            light(vx, note[x][y][0] + 1, col[note[x][y][0]] * 12);
            FastLED.show();
            FastLED.delay(1);
          }
        }
      }
    } else {
      note[x][y][0] = SMP.currentChannel;
    }

    if (note[x][y][0] > maxY - 2) {
      note[x][y][0] = 1;
      for (unsigned int vx = 1; vx < maxX + 1; vx++) {
        light(vx, note[x][y][0] + 1, col[note[x][y][0]] * 12);
      }
      FastLEDshow();
    }

    if (!isPlaying) {
      if (note[x][y][0] < 9) {
        _samplers[note[x][y][0]].noteEvent(12 * SampleRate[note[x][y][0]] + y - (note[x][y][0] + 1), defaultVelocity, true, false);
        serialprintln("???????");
      }

      if (note[x][y][0] == 14) {
        float frequency = pianoFrequencies[y] / 2;  // y-Wert ist 1-basiert, Array ist 0-basiert
        sound14.frequency(frequency);
        float WaveFormVelocity = mapf(defaultVelocity, 1, 127, 0.0, 1.0);
        sound14.amplitude(WaveFormVelocity);
        envelope14.noteOn();
        startTime = millis();    // Record the start time
        noteOnTriggered = true;  // Set the flag so we don't trigger noteOn again
      }

      if (note[x][y][0] == 13) {
        float frequency = pianoFrequencies[y - 1] / 2;  // y-Wert ist 1-basiert, Array ist 0-basiert
        sound13.frequency(frequency);
        float WaveFormVelocity = mapf(defaultVelocity, 1, 127, 0.0, 1.0);
        sound13.amplitude(WaveFormVelocity);
        envelope13.noteOn();
        startTime = millis();    // Record the start time
        noteOnTriggered = true;  // Set the flag so we don't trigger noteOn again
      }
    }

    updateLastPage();
    FastLEDshow();
  }

  void light(unsigned int x, unsigned int y, CRGB color) {
    if (y > 0 && y < 17 && x > 0 && x < 17) {
      if (y > maxY)
        y = 1;
      if (y % 2 == 0) {
        leds[(maxX - x) + (maxX * (y - 1))] = color;
      }
      if (y % 2 != 0) {
        leds[(x - 1) + (maxX * (y - 1))] = color;
      }
    }
  }

  void switchMode(Mode *newMode) {
    unpaintMode = false;
    SMP.singleMode = false;
    paintMode = false;
    buttonString = "0000";

    if (newMode != currentMode) {
      currentMode = newMode;
      String savedpos = "" + String(currentMode->pos[0]) + " " + String(currentMode->pos[1]) + " " + String(currentMode->pos[2]);
      serialprintln("--------------");
      serialprintln(currentMode->name + " > " + savedpos);

      // set values for encoders
      for (unsigned int i = 0; i < 4; i++) {  // Assuming 3 encoders
        float newval = round(mapf(currentMode->pos[i], 1, currentMode->maxValues[i], 1, currentMode->maxValues[i] * 4));
        if (i == 0) {  // reverse KnobDirection for Left Knob (up+down)
          newval = round(mapf(reverseMapEncoderValue(currentMode->pos[i], 1, maxY), 1, currentMode->maxValues[i], 1, currentMode->maxValues[i] * 4));
        }
        encoders[i].write(newval);
      }
    }
  }

  int reverseMapEncoderValue(unsigned int encoderValue, unsigned int minValue, unsigned int maxValue) {
    // do not reverse for some modes
    if (currentMode != &set_Wav && currentMode!= &noteShift) {
      return maxValue - (encoderValue - minValue);
    } else {

      return encoderValue;
    }
  }
  float mapAndClampEncoderValue(Encoder &encoder, int min, int max, int id) {
    float value = encoder.read();

    float mappedValue = round(mapf(value, min, max * 4, min, max));

    if (mappedValue < min) {
      // write all encoders
      encoder.write(min);

      // check if PAGE-Button was turned (encoder2 (id=2));
      //>>if prev Page
      if (id == 2 && (currentMode == &draw || currentMode == &singleMode)) {
        if (SMP.edit > 1) {
          SMP.edit = SMP.edit - 1;
          encoder.write(max * 4);
          // also set x encoder to new page
          encoders[1].write(round(mapf(SMP.edit, 1, currentMode->maxValues[1], 1, currentMode->maxValues[1] * 4)));
          return max;
        }
      }
      return min;
    } else if (mappedValue > max) {
      encoder.write(max * 4);
      //>> if next Page
      if (id == 2 && (currentMode == &draw || currentMode == &singleMode)) {
        if (SMP.edit < maxPages) {
          SMP.edit = SMP.edit + 1;
          encoder.write(min);
          // also set x encoder to new page
          encoders[1].write(round(mapf(SMP.edit, 1, currentMode->maxValues[1], 1, currentMode->maxValues[1] * 4)));
          return min;
        }
      }
      return max;
    } else {
      return mappedValue;
    }
  }

  void displaySample(unsigned int len) {
    unsigned int length = mapf(len, 0, 1329920, 1, maxX);
    unsigned int skip = mapf(SMP.seek * 200, 44, len, 1, maxX);

    for (unsigned int s = 1; s <= maxX; s++) {
      light(s, 5, CRGB(1, 1, 1));
    }

    for (unsigned int s = 1; s <= length; s++) {
      light(s, 5, CRGB(20, 20, 20));
    }

    for (unsigned int s = 1; s <= maxX; s++) {
      light(s, 4, CRGB(4, 0, 0));
    }

    for (unsigned int s = 1; s <= skip; s++) {
      light(s, 4, CRGB(0, 4, 0));
    }

    FastLED.show();
  }

  void checkPositions() {
    for (unsigned int i = 0; i < 4; i++) {
      unsigned int currentEncoderValue = encoders[i].read();
      if (currentEncoderValue != previousEncoderValues[i]) {
        currentMode->pos[0] = reverseMapEncoderValue(mapAndClampEncoderValue(encoders[0], 1, currentMode->maxValues[0], 0), 1, maxY);
        currentMode->pos[1] = mapAndClampEncoderValue(encoders[1], 1, currentMode->maxValues[1], 1);
        currentMode->pos[2] = mapAndClampEncoderValue(encoders[2], 1, currentMode->maxValues[2], 2);  // mitte
        if (isEncoder4Defined)
          currentMode->pos[3] = mapAndClampEncoderValue(encoders[3], currentMode->minValues[3], currentMode->maxValues[3], 3);  // mitte2

        // Sync encoder 3 with filter_knob when channel changes
        if (currentMode == &draw || currentMode == &singleMode) {
          if (isEncoder4Defined && i == 0) {
            if (currentMode->pos[0] != SMP.y) {
              SMP.y = currentMode->pos[0];
              unsigned int filterValue = SMP.filter_knob[SMP.y - 1];
              encoders[3].write(mapf(filterValue, 0, maxfilterResolution, 1, maxfilterResolution * 4));
            }
          }
          // Update filter_knob for the current channel
          if (isEncoder4Defined && i == 3) {
            if (currentMode->pos[3] != SMP.filter_knob[SMP.y - 1]) {

              SMP.filter_knob[SMP.y - 1] = currentMode->pos[3];
              serialprint("Updated filter_knob for channel ");
              serialprint(SMP.y);
              serialprint(": ");
              serialprintln(SMP.filter_knob[SMP.y - 1]);
              if ((SMP.y) > 1 && (SMP.y) < maxY)
                filters[SMP.y - 1]->frequency(100 * SMP.filter_knob[SMP.y - 1]);
            }
          }
          /// macht das sinn?
          // previousEncoderValues[i] = currentEncoderValue;
        }
      }
    }
  }

  void preloadSample(unsigned int folder, unsigned int sampleID, bool setMaxSampleLength) {

    char OUTPUTf[50];
    int plen = 0;
    int previewsample = ((folder)*100) + sampleID;
    sprintf(OUTPUTf, "samples/%d/_%d.wav", folder, previewsample);
    serialprintln(OUTPUTf);
    File previewSample = SD.open(OUTPUTf);
    SMP.smplen = 0;

    if (previewSample) {
      int fileSize = previewSample.size();
      previewSample.seek(24);
      for (uint8_t i = 24; i < 25; i++) {
        int g = previewSample.read();
        if (g == 72)
          PrevSampleRate = 4;
        if (g == 68)
          PrevSampleRate = 3;
        if (g == 34)
          PrevSampleRate = 2;
        if (g == 17)
          PrevSampleRate = 1;
        if (g == 0)
          PrevSampleRate = 4;
      }

      int startOffset = 200 * SMP.seek;   // Start offset in milliseconds
      int endOffset = 200 * SMP.seekEnd;  // End offset in milliseconds

      if (setMaxSampleLength == true) {
        endOffset = fileSize;
      }

      int startOffsetBytes = startOffset * PrevSampleRate * 2;  // Convert to bytes (assuming 16-bit samples)
      int endOffsetBytes = endOffset * PrevSampleRate * 2;      // Convert to bytes (assuming 16-bit samples)

      // Adjust endOffsetBytes to avoid reading past the file end
      endOffsetBytes = min(endOffsetBytes, fileSize - 44);

      previewSample.seek(44 + startOffsetBytes);
      memset(sampled[0], 0, sizeof(sample_len[0]));
      plen = 0;

      while (previewSample.available() && (plen < (endOffsetBytes - startOffsetBytes))) {
        int b = previewSample.read();
        sampled[0][plen] = b;
        plen++;
      }

      sampleIsLoaded = true;
      SMP.smplen = plen;

      // only set the first time to get seekEnd
      if (setMaxSampleLength == true) {
        serialprint("before:");
        serialprintln(currentMode->maxValues[3]);
        serialprintln(currentMode->pos[3]);
        serialprint("SET SAMPLELEN:");
        serialprint(SMP.smplen / (PrevSampleRate * 2) / 200);

        sampleLengthSet = true;
        SMP.seekEnd = (SMP.smplen / (PrevSampleRate * 2) / 200);

        currentMode->pos[3] = SMP.seekEnd;
        if (isEncoder4Defined)
          encoders[3].write(SMP.seekEnd * 4);
      }

      previewSample.close();
      displaySample(SMP.smplen);
    }
  }

  void preview(unsigned int PrevSampleRate, unsigned int plen) {
    // envelopes[0]->release(0);
    _samplers[0].removeAllSamples();
    envelope0.noteOff();
    _samplers[0].addSample(36, (int16_t *)sampled[0] + 2, (int)(plen / 2) - 120, 1);
    _samplers[0].noteEvent(12 * PrevSampleRate, defaultVelocity, true, false);
  }

  void previewSample(unsigned int folder, unsigned int sampleID, bool setMaxSampleLength, bool firstPreview) {
    // envelopes[0]->release(0);
    _samplers[0].removeAllSamples();
    envelope0.noteOff();

    char OUTPUTf[50];
    int plen = 0;

    sprintf(OUTPUTf, "samples/%d/_%d.wav", folder, sampleID);
    serialprintln(OUTPUTf);
    File previewSample = SD.open(OUTPUTf);
    SMP.smplen = 0;
    Serial.println(previewSample.size());
    
    if (previewSample) {
          int fileSize = previewSample.size();
      
      if (firstPreview){
           fileSize = min(previewSample.size(), 300000); // max preview len =  X Sec.
           //toDO: make it better to preview long files
      }
      previewSample.seek(24);
      for (uint8_t i = 24; i < 25; i++) {
        int g = previewSample.read();
        if (g == 72)
          PrevSampleRate = 4;
        if (g == 68)
          PrevSampleRate = 3;
        if (g == 34)
          PrevSampleRate = 2;
        if (g == 17)
          PrevSampleRate = 1;
        if (g == 0)
          PrevSampleRate = 4;
      }

      int startOffset = 200 * SMP.seek;   // Start offset in milliseconds
      int endOffset = 200 * SMP.seekEnd;  // End offset in milliseconds

      if (setMaxSampleLength == true) {
        endOffset = fileSize;
      }

      int startOffsetBytes = startOffset * PrevSampleRate * 2;  // Convert to bytes (assuming 16-bit samples)
      int endOffsetBytes = endOffset * PrevSampleRate * 2;      // Convert to bytes (assuming 16-bit samples)

      // Adjust endOffsetBytes to avoid reading past the file end
      endOffsetBytes = min(endOffsetBytes, fileSize - 44);

      previewSample.seek(44 + startOffsetBytes);
      memset(sampled[0], 0, sizeof(sample_len[0]));
      plen = 0;

      while (previewSample.available() && (plen < (endOffsetBytes - startOffsetBytes))) {
        int b = previewSample.read();
        sampled[0][plen] = b;
        plen++;
      }

      sampleIsLoaded = true;
      SMP.smplen = plen;

      // only set the first time to get seekEnd
      if (setMaxSampleLength == true) {
        serialprint("before:");
        serialprintln(currentMode->maxValues[3]);
        serialprintln(currentMode->pos[3]);
        serialprint("SET SAMPLELEN:");
        serialprint(SMP.smplen / (PrevSampleRate * 2) / 200);

        sampleLengthSet = true;
        SMP.seekEnd = (SMP.smplen / (PrevSampleRate * 2) / 200);

        currentMode->pos[3] = SMP.seekEnd;
        if (isEncoder4Defined)
          encoders[3].write(SMP.seekEnd * 4);
      }

      previewSample.close();
      displaySample(SMP.smplen);

      _samplers[0].addSample(36, (int16_t *)sampled[0] + 2, (int)(plen / 2) - 120, 1);

      serialprintln("NOTE");
      _samplers[0].noteEvent(12 * PrevSampleRate, defaultVelocity, true, false);
    }
  }

  //draw a loading bar, white border, no background, filling with given color from left to right, 4 leds high, parameters: minval, maxval, currentval, color
  void drawLoadingBar(int minval, int maxval, int currentval, CRGB color, CRGB fontColor) {
    int ypos = 3;
    
    int barwidth = mapf(currentval, minval, maxval, 0, maxX);
    for (int x = 1; x <= maxX; x++) {
       light(x, ypos-1, CRGB(255, 255, 255));
       light(x, ypos+2, CRGB(255, 255, 255));
    }
    //the ends
    light(1, ypos, CRGB(255, 255, 255));
    light(1, ypos+1, CRGB(255, 255, 255));
    light(maxX, ypos, CRGB(255, 255, 255));
    light(maxX, ypos+1, CRGB(255, 255, 255));
    

    for (int x = 2; x < maxX; x++) {
      for (int y = 0; y <= 1; y++) {
          if (x < barwidth) {
            light(x, ypos+y, color);
          } else {
            light(x, ypos+y, CRGB(0, 0, 0));
          }
      }

    }
    showNumber(currentval, fontColor, 0);
     
  }


  
  void loadSample(unsigned int packID, unsigned int sampleID) {
    serialprint("loading");
    serialprintln(packID);
    drawNoSD();

    int yposLoader = sampleID + 1;
    if (sampleID > maxY)
      yposLoader = 2;
    //for (unsigned int f = 1; f < (maxX / 2) + 1; f++) {
    //  light(f, yposLoader, CRGB(20, 20, 0));
   // }
   // showIcons("icon_samplepack", CRGB(200, 200, 200));
   // FastLEDshow();

    char OUTPUTf[50];
    sprintf(OUTPUTf, "%d/%d.wav", packID, sampleID);

    if (packID == 0) {
      // SingleTrack from Samples-Folder
      sprintf(OUTPUTf, "samples/%d/_%d.wav", getFolderNumber(sampleID), sampleID);
      sampleID = SMP.currentChannel;
    }

    if (!SD.exists(OUTPUTf)) {
      serialprint("File does not exist: ");
      serialprintln(OUTPUTf);
      // mute the channel
      SMP.mute[sampleID] = true;

      return;
    }

    usedFiles[sampleID - 1] = OUTPUTf;
    
    File loadSample = SD.open(OUTPUTf);
    if (loadSample) {
      int fileSize = loadSample.size();
      loadSample.seek(24);
      for (uint8_t i = 24; i < 25; i++) {
        int g = loadSample.read();
        if (g == 0)
          SampleRate[sampleID] = 4;
        if (g == 17)
          SampleRate[sampleID] = 1;
        if (g == 34)
          SampleRate[sampleID] = 2;
        if (g == 68)
          SampleRate[sampleID] = 3;
        if (g == 72)
          SampleRate[sampleID] = 4;
      }
      
      SMP.seek = 0;
      
      unsigned int startOffset = 200 * SMP.seek;                         // Start offset in milliseconds
      unsigned int startOffsetBytes = startOffset * PrevSampleRate * 2;  // Convert to bytes (assuming 16-bit samples)

      unsigned int endOffset = 200 * SMP.seekEnd;  // End offset in milliseconds
      if (SMP.seekEnd == 0) {
        // If seekEnd is not set, default to the full length of the sample
        endOffset = fileSize;
      }
      unsigned int endOffsetBytes = endOffset * PrevSampleRate * 2;  // Convert to bytes (assuming 16-bit samples)
      // Adjust endOffsetBytes to avoid reading past the file end
      endOffsetBytes = min(endOffsetBytes, fileSize - 44);

      loadSample.seek(44 + startOffsetBytes);
      unsigned int i = 0;
      memset(sampled[sampleID], 0, sizeof(sample_len[sampleID]));

      while (loadSample.available() && (i < (endOffsetBytes - startOffsetBytes))) {
        int b = loadSample.read();
        sampled[sampleID][i] = b;
        i++;
      }
      loadSample.close();

      i = i / 2;
      _samplers[sampleID].removeAllSamples();
      _samplers[sampleID].addSample(36, (int16_t *)sampled[sampleID] + 2, (int)i - 120, 1);
    }

    //for (unsigned int f = 1; f < maxX + 1; f++) {
    //  light(f, yposLoader, col[sampleID]);

      //FastLEDshow();
    //}
  }

  void loop() {
    // get USB MIdi clock
    usbMIDI.read();
    /*
    Serial.println("----------------");
    Serial.println(AudioMemoryUsageMax());
    Serial.println(AudioProcessorUsageMax());
    Serial.println("----------------");
    */

    // get EncoderButtonState
    multiresponseButton1.poll();
    multiresponseButton2.poll();
    multiresponseButton3.poll();
    multiresponseButton4.poll();
    // getEncoders
    checkPositions();

    // Set stateMashine
    if (currentMode->name == "DRAW") {
      canvas(false);
    } else if (currentMode->name == "VOLUME_BPM") {
      setVolume();
    } else if (currentMode->name == "VELOCITY") {
      setVelocity();
    } else if (currentMode->name == "SINGLE") {
      canvas(true);
    } else if (currentMode->name == "MENU") {
      showMenu();
    } else if (currentMode->name == "SET_SAMPLEPACK") {
      showSamplePack();
    } else if (currentMode->name == "SET_WAV") {
      showWave();
    } else if (currentMode->name == "NOTE_SHIFT") {
      shiftNotes();
      drawBase();
      drawSamples();
      if (isPlaying) {
        drawTimer(pagebeat);
      }
    }

    // reset buttons
    if (resetTimerActive && millis() - lastButtonPressTime > 80) {
      checkMode();
      memset(buttons, 0, sizeof(buttons));
      memset(oldButtons, 0, sizeof(oldButtons));
      resetTimerActive = false;  // Stop the timer
    }

    if (currentMode == &draw || currentMode == &singleMode) {
      drawBase();
      drawSamples();
      if (currentMode != &velocity)
        drawCursor();
      if (isPlaying) {
        drawTimer(pagebeat);
        FastLEDshow();
      }
    }

    // end synthsound after X ms
    if (noteOnTriggered && millis() - startTime >= 200) {
      envelope14.noteOff();
      envelope13.noteOff();
      noteOnTriggered = false;
    }

    FastLEDshow();  // draw!
    yield();
    delay(5);       // otherwise, the audio lib crashes after 1-60sec //(1 is good if 720MHZ overclock!!!!)
    
  }

  /******************************************************************************************************/

  void canvas(bool singleview) {

    // CURSOR
    String posString = "Y:" + String(currentMode->pos[0]) + " X:" + String(currentMode->pos[2]) + " Page:" + String(currentMode->pos[1]);

    if (posString != oldPos) {
      oldPos = posString;
      SMP.x = currentMode->pos[2];
      SMP.y = currentMode->pos[0];

      if (paintMode) {
        note[(SMP.edit - 1) * maxX + SMP.x][SMP.y][0] = SMP.y - 1;
      }
      if (paintMode && currentMode == &singleMode) {
        note[(SMP.edit - 1) * maxX + SMP.x][SMP.y][0] = SMP.currentChannel;
      }

      if (unpaintMode) {
        if (SMP.singleMode) {
          if (note[(SMP.edit - 1) * maxX + SMP.x][SMP.y][0] == SMP.currentChannel)
            note[(SMP.edit - 1) * maxX + SMP.x][SMP.y][0] = 0;
        } else {
          note[(SMP.edit - 1) * maxX + SMP.x][SMP.y][0] = 0;
        }
      }

      unsigned int editpage = currentMode->pos[1];
      if (editpage != SMP.edit && editpage <= lastPage) {
        SMP.edit = editpage;
        serialprintln("p:" + String(SMP.edit));
      }
    }
  }

  void toggleCopyPaste() {
    if (!SMP.activeCopy) {
      // copy the pattern into the memory
      serialprint("copy now");
      unsigned int src = 0;
      for (unsigned int c = ((SMP.edit - 1) * maxX) + 1; c < ((SMP.edit - 1) * maxX) + (maxX + 1); c++) {  // maxy?
        src++;
        for (unsigned int y = 1; y < maxY + 1; y++) {
          serialprint(c);
          serialprint("-");
          serialprintln(y);

          for (unsigned int b = 0; b < 2; b++) {
            tmp[src][y][b] = note[c][y][b];
          }
        }
      }
    } else {
      // paste the memory into the song
      serialprint("paste here!");
      unsigned int src = 0;
      for (unsigned int c = ((SMP.edit - 1) * maxX) + 1; c < ((SMP.edit - 1) * maxX) + (maxX + 1); c++) {
        src++;
        for (unsigned int y = 1; y < maxY + 1; y++) {
          for (unsigned int b = 0; b < 2; b++) {
            note[c][y][b] = tmp[src][y][b];
          }
        }
      }
    }
    updateLastPage();
    SMP.activeCopy = !SMP.activeCopy;  // Toggle the boolean value
  }

  void clearNoteChannel(unsigned int c, unsigned int yStart, unsigned int yEnd, unsigned int channel, bool singleMode) {
    for (unsigned int y = yStart; y < yEnd; y++) {
      if (singleMode) {
        if (note[c][y][0] == channel)
          note[c][y][0] = 0;
        if (note[c][y][0] == channel)
          note[c][y][1] = defaultVelocity;
      } else {
        note[c][y][0] = 0;
        note[c][y][1] = defaultVelocity;
      }
    }
  }

  void clearPage() {
    unsigned int start = (SMP.edit - 1) * maxX + 1;
    unsigned int end = start + maxX;
    unsigned int channel = SMP.currentChannel;
    bool singleMode = SMP.singleMode;

    for (unsigned int c = start; c < end; c++) {
      clearNoteChannel(c, 1, maxY + 1, channel, singleMode);
    }
    updateLastPage();
  }

  void drawBPMScreen() {
    FastLEDclear();
    drawVolume(SMP.vol);
    CRGB volColor = CRGB(SMP.vol * SMP.vol, 20 - SMP.vol, 0);
    showIcons("helper_vol", volColor);
    showIcons("helper_bpm", CRGB(10, 10, 10));
    showNumber(SMP.bpm, false, -1);
  }

  void updateVolume() {

    if (!isEncoder4Defined) {
      SMP.vol = currentMode->pos[0];
    } else {
      SMP.vol = currentMode->pos[3];
    }
    float vol = float(SMP.vol / 10.0);
    serialprintln("Vol: " + String(vol));
    if (vol <= 1.0)
      sgtl5000_1.volume(vol);
    // setvol = true;
  }

  void updateBPM() {
    // setvol = false;
    serialprintln("BPM: " + String(currentMode->pos[2]));
    SMP.bpm = currentMode->pos[2];
    playNoteInterval = ((60 * 1000 / SMP.bpm) / 4) * 1000;
    playTimer.update(playNoteInterval);
    drawBPMScreen();
  }

  void setVolume() {
    drawBPMScreen();
    if (!isEncoder4Defined) {
      if (currentMode->pos[0] != SMP.vol) {
        updateVolume();
      }
    } else {
      if (currentMode->pos[3] != SMP.vol) {
        updateVolume();
      }
    }

    if (currentMode->pos[2] != SMP.bpm) {
      updateBPM();
    }
  }

  CRGB getCol(unsigned int g) {
    return col[g] * 10;
  }

  void drawVolume(unsigned int vol) {
    unsigned int maxXVolume = int(vol * 1.3) + 2;
    for (unsigned int y = 5; y <= 6; y++) {
      for (unsigned int x = 0; x <= maxXVolume; x++) {
        light(x + 1, y, CRGB(vol * vol, 20 - vol, 0));
      }
    }
  }

  void showMenu() {
    unsigned int knopf = 2;
    drawNoSD();
    FastLEDclear();

    showIcons("icon_loadsave", CRGB(10, 5, 0));
    showIcons("helper_select", CRGB(0, 0, 5));

    char OUTPUTf[50];
    sprintf(OUTPUTf, "%d.txt", SMP.file);
    if (SD.exists(OUTPUTf)) {
      showIcons("helper_save", CRGB(1, 0, 0));
      showIcons("helper_load", CRGB(0, 20, 0));
      showNumber(SMP.file, CRGB(0, 0, 20), 0);
    } else {
      showIcons("helper_save", CRGB(20, 0, 0));
      showIcons("helper_load", CRGB(0, 1, 0));
      showNumber(SMP.file, CRGB(20, 20, 40), 0);
    }
    FastLED.show();

    if (currentMode->pos[knopf] != SMP.file) {
      serialprint("File: " + String(currentMode->pos[knopf]));
      serialprintln();
      SMP.file = currentMode->pos[knopf];
    }
  }

  void showSamplePack() {
    drawNoSD();
    FastLEDclear();

    showIcons("icon_samplepack", CRGB(10, 10, 0));
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
    FastLED.show();
    if (currentMode->pos[2] != SMP.pack) {
      serialprintln("File: " + String(currentMode->pos[2]));
      SMP.pack = currentMode->pos[2];
    }
  }

  void loadSamplePack(unsigned int pack) {
    serialprintln("Loading SamplePack #" + String(pack));
    drawNoSD();
    FastLEDclear();
    SMP.smplen = 0;
    SMP.seekEnd = 0;
    EEPROM.put(0, pack);
    for (unsigned int z = 1; z <= maxFiles; z++) {
      FastLEDclear();
      showIcons("icon_samplepack", CRGB(200, 200, 200));
      drawLoadingBar(1,maxFiles,z,CRGB(255,0,0),CRGB(255,255,255));
      FastLEDshow();
      loadSample(pack, z);
    }
    char OUTPUTf[50];
    sprintf(OUTPUTf, "%d/%d.wav", pack, 1);
    if (SD.exists(OUTPUTf)) {
      showIcons("icon_sample", CRGB(100, 0, 100));
      showNumber(pack, CRGB(100, 0, 100), 0);
    }
    switchMode(&draw);
  }

  void saveSamplePack(unsigned int pack) {
    serialprintln("Saving SamplePack in #" + String(pack));
    FastLEDclear();
    char OUTPUTdir[50];
    sprintf(OUTPUTdir, "%d/", pack);
    SD.mkdir(OUTPUTdir);
    delay(500);
    for (unsigned int i = 0; i < sizeof(usedFiles) / sizeof(usedFiles[0]); i++) {
      for (unsigned int f = 1; f < (maxY / 2) + 1; f++) {
        light(i + 1, f, CRGB(4, 0, 0));
      }
      showIcons("icon_samplepack", CRGB(200, 200, 200));
      FastLED.show();

      if (SD.exists(usedFiles[i].c_str())) {
        File saveFilePack = SD.open(usedFiles[i].c_str());
        char OUTPUTf[50];
        sprintf(OUTPUTf, "%d/%d.wav", pack, i + 1);

        if (SD.exists(OUTPUTf)) {
          SD.remove(OUTPUTf);
          delay(100);
        }

        File myDestFile = SD.open(OUTPUTf, FILE_WRITE);

        size_t n;
        uint8_t buf[512];
        while ((n = saveFilePack.read(buf, sizeof(buf))) > 0) {
          myDestFile.write(buf, n);
        }
        myDestFile.close();
        saveFilePack.close();
      }

      for (unsigned int f = 1; f < (maxY + 1) + 1; f++) {
        light(i + 1, f, CRGB(0, 20, 0));
      }
      FastLEDshow();
    }
    switchMode(&draw);
  }



int getFolderNumber(int value) {
  int folder = floor(value / 100);
  if (folder > maxFolders) folder = maxFolders;
  if (folder <=0 ) folder = 0;
  return folder;
  }

int getFileNumber(int value) {
// return the file number in the folder, so 113 = 13, 423 = 23
  int folder = getFolderNumber(value);
  int wavfile = value % 100;  
  if (wavfile <=0 ) wavfile = 0;
  return wavfile + folder*100; 
  }


  void showWave() {
    File sampleFile;
    drawNoSD();

    FastLEDclear();
    if (SMP.wav < 100)
      showIcons("icon_sample", col[SMP.y - 1]);
    showIcons("helper_select", col[SMP.y - 1]);
    showIcons("helper_load", CRGB(0, 20, 0));
    showIcons("helper_seek", CRGB(10, 0, 0));
    showIcons("helper_folder", CRGB(10, 10, 0));
    showNumber( SMP.wav, col_Folder[getFolderNumber(SMP.wav)], 0);
    displaySample(SMP.smplen);
  
  

    if (currentMode->pos[1] != SMP.folder) {
      //change FOLDER
      SMP.folder = currentMode->pos[1];
      Serial.println("Folder: " + String(SMP.folder - 1));
      SMP.wav = ((SMP.folder-1)*100);
      Serial.println("wav: " + String(SMP.wav));
      encoders[2].write((SMP.wav * 4)-1);
    }

    // ENDPOSITION SAMPLE
    if (isEncoder4Defined) {
      if ((currentMode->pos[3]) != SMP.seekEnd && sampleIsLoaded) {
        SMP.seekEnd = currentMode->pos[3];
        serialprintln("seekEnd:");
        serialprintln(SMP.seekEnd);
        _samplers[0].removeAllSamples();
        envelope0.noteOff();
        if (sampleFile) {
          sampleFile.seek(0);
        }

        char OUTPUTf[50];
        
        sprintf(OUTPUTf, "samples/%d/_%d.wav", getFolderNumber(SMP.wav), getFileNumber(SMP.wav));

        if (SD.exists(OUTPUTf)) {
          showIcons("helper_select", col[SMP.y - 1]);
          showIcons("helper_load", CRGB(0, 20, 0));
          showIcons("helper_seek", CRGB(10, 0, 0));
          showIcons("helper_folder", CRGB(10, 30, 0));
          showNumber(SMP.wav, col_Folder[getFolderNumber(SMP.wav)], 0);
          if (!sampleLengthSet) previewSample(getFolderNumber(SMP.wav), getFileNumber(SMP.wav), false,false);
        } else {
          showIcons("helper_select", col[SMP.y - 1]);
          showIcons("helper_load", CRGB(0, 0, 0));
          showIcons("helper_folder", CRGB(10, 10, 0));
          showNumber( SMP.wav, col_Folder[getFolderNumber(SMP.wav)], 0);
        }

        sampleLengthSet = false;
      }
    }

    // STARTPOSITION SAMPLE
    if ((currentMode->pos[0]) - 1 != SMP.seek && sampleIsLoaded) {
      SMP.seek = currentMode->pos[0] - 1;
      serialprintln(SMP.seek);
      _samplers[0].removeAllSamples();
      envelope0.noteOff();
      if (sampleFile) {
        sampleFile.seek(0);
      }

      char OUTPUTf[50];
      sprintf(OUTPUTf, "samples/%d/_%d.wav", getFolderNumber(SMP.wav), getFileNumber(SMP.wav));
      if (SD.exists(OUTPUTf)) {
        showIcons("helper_select", col[SMP.y - 1]);
        showIcons("helper_load", CRGB(0, 20, 0));
        showIcons("helper_folder", CRGB(10, 30, 0));
        showNumber( SMP.wav, col_Folder[getFolderNumber(SMP.wav)], 0);
        previewSample(getFolderNumber(SMP.wav), getFileNumber(SMP.wav), false,false);
      } else {
        showIcons("helper_select", col[SMP.y - 1]);
        showIcons("helper_load", CRGB(0, 0, 0));
        showIcons("helper_folder", CRGB(10, 10, 0));
        showNumber( SMP.wav, col_Folder[getFolderNumber(SMP.wav)], 0);
      }
    }

    // SAMPLEFILE
    if (currentMode->pos[2] != SMP.wav) {
      
      sampleIsLoaded = false;
      SMP.wav = currentMode->pos[2];
      Serial.println("File: " + String(getFolderNumber(SMP.wav)) + " / " + String(getFileNumber(SMP.wav)));

      // reset SEEK and stop sample playing
      SMP.smplen = 0;
      currentMode->pos[0] = 1;
      encoders[0].write(1);
      SMP.seek = 0;
      _samplers[0].removeAllSamples();
      envelope0.noteOff();
      if (sampleFile) {
        sampleFile.seek(0);
      }
      if (SMP.wav < 100) showIcons("icon_sample", col[SMP.y - 1]);

      char OUTPUTf[50];
      sprintf(OUTPUTf, "samples/%d/_%d.wav", getFolderNumber(SMP.wav), getFileNumber(SMP.wav));
      serialprintln("------");
      serialprintln(OUTPUTf);
      
     // if (SD.exists(OUTPUTf)) {
     


    if (SMP.wav < getFolderNumber(SMP.wav+1) * 100) {
      Serial.print("exceeded first number of folder ");
      Serial.println(getFolderNumber(SMP.wav+1));
      SMP.wav = lastFile[getFolderNumber(SMP.wav)];
      SMP.folder = getFolderNumber(SMP.wav);
      //write encoder
       encoders[2].write((SMP.wav * 4)-1);
       encoders[1].write((SMP.folder * 4)-1);
     }


 if ( lastPreviewedSample[getFolderNumber(SMP.wav)] < SMP.wav){
     if (SMP.wav > lastFile[getFolderNumber(SMP.wav)]) {
      Serial.print("exceeding last file number of folder ");
      Serial.println(getFolderNumber(SMP.wav));
      SMP.wav = ((getFolderNumber(SMP.wav)+1)*100);
      SMP.folder = getFolderNumber(SMP.wav+1);
      //write encoder
      encoders[2].write((SMP.wav * 4)-1);
      encoders[1].write((SMP.folder * 4)-1);
     }
     }

        lastPreviewedSample[getFolderNumber(SMP.wav)] = SMP.wav;
        showIcons("helper_select", col[SMP.y - 1]);
        showIcons("helper_load", CRGB(0, 20, 0));
        showIcons("helper_folder", CRGB(10, 30, 0));
        showNumber(SMP.wav, col_Folder[getFolderNumber(SMP.wav)], 0);
        previewSample(getFolderNumber(SMP.wav), getFileNumber(SMP.wav), true, true);
        sampleIsLoaded = true;
        
      //} else {
      /*
        encoders[2].write(((lastPreviewedSample[SMP.folder]) * 4) - 1);

        showIcons("helper_select", col[SMP.y - 1]);
        showIcons("helper_load", CRGB(0, 0, 0));
        showIcons("helper_folder", CRGB(10, 10, 0));
        showNumber(((SMP.folder - 1) * 100) + SMP.wav, col_Folder[SMP.folder - 1], 0);
       
       }
       */
    }
  }

  void showIntro() {
    FastLED.clear();
    FastLED.show();
    for (int gx = 0; gx < 72; gx++) {
      light(logo[gx][0], maxY - logo[gx][1], CRGB(50, 50, 50));
      FastLED.show();
      delay(10);
    }
    delay(200);

    for (int fade = 0; fade < 10 + 1; fade++) {
      for (int u = 0; u < NUM_LEDS; u++) {
        leds[u] = leds[u].fadeToBlackBy(fade * 10);
      }
      FastLED.show();
      delay(50);
    }

    int bright = 100;
    for (int y = -15; y < 3; y++) {
      FastLED.clear();
      showNumber(404, CRGB(100, 100, 100), y);
      FastLED.show();
      delay(50);
    }
    delay(800);
    for (int y = 3; y < 16; y++) {
      FastLED.clear();

      showNumber(404, CRGB(100, 100, 100), y);
      FastLED.show();
      delay(50);
    }
    FastLED.clear();
    FastLED.show();
  }

  void showNumber(unsigned int count, CRGB color, int topY) {
    if (!color)
      color = CRGB(20, 20, 20);
    char buf[4];
    sprintf(buf, "%03i", count);
    unsigned int stelle2 = buf[0] - '0';
    unsigned int stelle1 = buf[1] - '0';
    unsigned int stelle0 = buf[2] - '0';

    unsigned int ypos = maxY - topY;
    for (unsigned int gx = 0; gx < 24; gx++) {
      if (stelle2 > 0)
        light(1 + number[stelle2][gx][0], ypos - number[stelle2][gx][1], color);
      if ((stelle1 > 0 || stelle2 > 0))
        light(6 + number[stelle1][gx][0], ypos - number[stelle1][gx][1], color);
      light(11 + number[stelle0][gx][0], ypos - number[stelle0][gx][1], color);
    }
    FastLEDshow();
  }

  void drawVelocity(CRGB color) {
    FastLEDclear();
    unsigned int vy = currentMode->pos[2];
    if (!SMP.singleMode) {
      for (unsigned int y = 1; y < vy + 1; y++) {
        light(SMP.x, y, CRGB(y * y, 20 - y, 0));
      }
    } else {
      serialprintln("single");
      for (unsigned int x = 1; x < maxX + 1; x++) {
        for (unsigned int y = 1; y < vy + 1; y++) {
          light(x, y, CRGB(y * y, 20 - y, 0));
        }
      }
    }
  }

  void drawBase() {
    if (!SMP.singleMode) {
      unsigned int colors = 0;
      for (unsigned int y = 1; y < maxY; y++) {
        unsigned int filtering = mapf(SMP.filter_knob[y - 1], 0, maxfilterResolution, 50, 5);
        for (unsigned int x = 1; x < maxX + 1; x++) {
          if (SMP.mute[y - 1]) {
            light(x, y, CRGB(0, 0, 0));
          } else {
            light(x, y, col[colors] / filtering);
          }
        }
        colors++;
      }
      for (unsigned int x = 1; x <= 13; x += 4) {
        light(x, 1, CRGB(1, 1, 0));  // gelb
      }
    } else {
      unsigned int currentChannel = SMP.currentChannel;
      bool isMuted = SMP.mute[currentChannel];
      CRGB color = col[currentChannel] / (isMuted ? 28 : 14);
      for (unsigned int y = 1; y < maxY; y++) {
        for (unsigned int x = 1; x < maxX + 1; x++) {
          light(x, y, color);
        }
      }
      for (unsigned int x = 1; x <= 13; x += 4) {
        light(x, 1, CRGB(0, 1, 1));  // türkis
      }
    }
    drawPages();
    drawStatus();
  }

  void drawStatus() {
    CRGB ledColor = CRGB(0, 0, 0);
    if (SMP.activeCopy)
      ledColor = CRGB(20, 20, 0);
    for (unsigned int s = 9; s <= maxX; s++) {
      light(s, maxY, ledColor);
    }

    if (currentMode == &noteShift) {
      // draw a moving marquee to indicate the note shift mode
      for (unsigned int x = 9; x <= maxX; x++) {
        light(x, maxY, CRGB(0, 0, 0));
      }
      light(round(marqueePos), maxY, CRGB(20, 20, 20));
      if (movingForward) {
        marqueePos = marqueePos + 0.1;
        if (marqueePos > maxX) {
          marqueePos = maxX;
          movingForward = false;
        }
      } else {
        marqueePos = marqueePos - 0.1;
        if (marqueePos < 9) {
          marqueePos = 9;
          movingForward = true;
        }
      }
    }
  }

  void updateLastPage() {
    lastPage = 1;  // Reset lastPage before starting

    for (unsigned int p = 1; p <= maxPages; p++) {
      bool pageHasNotes = false;

      for (unsigned int ix = 1; ix <= maxX; ix++) {
        for (unsigned int iy = 1; iy <= maxY; iy++) {
          if (note[((p - 1) * maxX) + ix][iy][0] > 0) {
            pageHasNotes = true;
            break;  // No need to check further, this page has notes
          }
        }

        if (pageHasNotes) {
          lastPage = p;  // Update lastPage to the current page with notes
          break;         // No need to check further columns, go to the next page
        }
      }
      hasNotes[p] = pageHasNotes;
    }
    // Ensure lastPage is at least 1 to avoid potential issues
    if (lastPage == 0)
      lastPage = 1;
  }

  void drawPages() {

    CRGB ledColor;

    for (unsigned int p = 1; p <= maxPages; p++) {  // Assuming maxPages is 8

      // If the page is the current one, set the LED to white
      if (SMP.page == p && SMP.edit == p) {
        ledColor = isPlaying ? CRGB(20, 255, 20) : CRGB(50, 50, 50);
      } else if (SMP.page == p) {
        ledColor = isPlaying ? CRGB(0, 15, 0) : CRGB(0, 0, 35);
      } else {
        if (SMP.edit == p) {
          ledColor = SMP.page == p ? CRGB(50, 50, 50) : CRGB(20, 20, 20);
        } else {
          ledColor = hasNotes[p] ? CRGB(0, 0, 35) : CRGB(1, 0, 0);
        }
      }

      // Set the LED color
      light(p, maxY, ledColor);
    }

    // After the loop, update the maxValues for Page-select-knob
    currentMode->maxValues[1] = lastPage + 1;

    // Additional logic can be added here if needed
  }

  /************************************************
      DRAW SAMPLES
  *************************************************/
  void drawSamples() {
    for (unsigned int ix = 1; ix < maxX + 1; ix++) {
      for (unsigned int iy = 1; iy < maxY + 1; iy++) {
        if (note[((SMP.edit - 1) * maxX) + ix][iy][0] > 0) {
          if (!SMP.mute[note[((SMP.edit - 1) * maxX) + ix][iy][0]]) {
            light(ix, iy, getCol(note[((SMP.edit - 1) * maxX) + ix][iy][0]));

            // if there is any note of the same value in the same column, make it less bright
            for (unsigned int iy2 = 1; iy2 < maxY + 1; iy2++) {
              if (iy2 != iy && note[((SMP.edit - 1) * maxX) + ix][iy2][0] == note[((SMP.edit - 1) * maxX) + ix][iy][0]) {
                light(ix, iy2, getCol(note[((SMP.edit - 1) * maxX) + ix][iy][0]) / 8);
              }
            }
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

  void drawTimer(unsigned int timer) {
    if (SMP.page == SMP.edit) {
      for (unsigned int y = 1; y < maxY; y++) {
        light(timer, y, CRGB(10, 0, 0));

        if (note[((SMP.page - 1) * maxX) + timer][y][0] > 0) {
          if (SMP.mute[note[((SMP.page - 1) * maxX) + timer][y][0]] == 0) {
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
    if (dir == 1)
      pulse = pulse + 1;
    if (dir == -1)
      pulse = pulse - 1;
    if (pulse > 220) {
      dir = -1;
    }
    if (pulse < 1) {
      dir = 1;
    }

    light(SMP.x, SMP.y, CRGB(255 - (int)pulse, 255 - (int)pulse, 255 - (int)pulse));
  }

  void showIcons(String ico, CRGB colors) {
    const int(*iconArray)[2] = nullptr;  // Change to const int

    unsigned int size = 0;

    if (ico == "icon_samplepack") {
      iconArray = icon_samplepack;
      size = sizeof(icon_samplepack) / sizeof(icon_samplepack[0]);
    } else if (ico == "icon_sample") {
      iconArray = icon_sample;
      size = sizeof(icon_sample) / sizeof(icon_sample[0]);
    } else if (ico == "icon_loadsave") {
      iconArray = icon_loadsave;
      size = sizeof(icon_loadsave) / sizeof(icon_loadsave[0]);
    } else if (ico == "helper_load") {
      iconArray = helper_load;
      size = sizeof(helper_load) / sizeof(helper_load[0]);
    } else if (ico == "helper_seek") {
      iconArray = helper_seek;
      size = sizeof(helper_seek) / sizeof(helper_seek[0]);
    } else if (ico == "helper_folder") {
      iconArray = helper_folder;
      size = sizeof(helper_folder) / sizeof(helper_folder[0]);
    } else if (ico == "helper_save") {
      iconArray = helper_save;
      size = sizeof(helper_save) / sizeof(helper_save[0]);
    } else if (ico == "helper_select") {
      iconArray = helper_select;
      size = sizeof(helper_select) / sizeof(helper_select[0]);
    } else if (ico == "helper_vol") {
      iconArray = helper_vol;
      if (!isEncoder4Defined)
        iconArray = helper_vol2;

      size = sizeof(helper_vol) / sizeof(helper_vol[0]);
    } else if (ico == "helper_bpm") {
      iconArray = helper_bpm;
      size = sizeof(helper_bpm) / sizeof(helper_bpm[0]);
    } else if (ico == "icon_bpm") {
      iconArray = icon_bpm;
      size = sizeof(icon_bpm) / sizeof(icon_bpm[0]);
    }

    if (iconArray != nullptr) {
      for (unsigned int gx = 0; gx < size; gx++) {
        light(iconArray[gx][0], maxY - iconArray[gx][1], colors);
      }
    }
  }

  void loadWav() {
    Serial.println("Loading Wave :" + String(SMP.wav));
    loadSample(0, SMP.wav);
    switchMode(&singleMode);
    SMP.singleMode = true;
  }

  void savePattern(bool autosave) {
    serialprintln("Saving in slot #" + String(SMP.file));
    drawNoSD();
    FastLEDclear();
    unsigned int maxdata = 0;
    char OUTPUTf[50];
    // Save to autosave.txt if autosave is true
    if (autosave) {
      sprintf(OUTPUTf, "autosaved.txt");
    } else {
      sprintf(OUTPUTf, "%d.txt", SMP.file);
    }
    if (SD.exists(OUTPUTf)) {
      SD.remove(OUTPUTf);
    }
    File saveFile = SD.open(OUTPUTf, FILE_WRITE);
    if (saveFile) {
      // Save notes
      for (unsigned int sdx = 1; sdx < maxlen; sdx++) {
        for (unsigned int sdy = 1; sdy < maxY + 1; sdy++) {
          maxdata = maxdata + note[sdx][sdy][0];
          saveFile.write(note[sdx][sdy][0]);
          saveFile.write(note[sdx][sdy][1]);
        }
      }
      // Use a unique marker to indicate the end of notes and start of SMP data
      saveFile.write(0xFF);  // Marker byte to indicate end of notes
      saveFile.write(0xFE);  // Marker byte to indicate start of SMP

      // Save SMP struct
      saveFile.write((uint8_t *)&SMP, sizeof(SMP));
    }
    saveFile.close();
    if (maxdata == 0) {
      SD.remove(OUTPUTf);
    }
    if (!autosave) {
      delay(500);
      switchMode(&draw);
    }
  }

  void autoSave() {
    savePattern(true);
  }

  void loadPattern(bool autoload) {
    serialprintln("Loading slot #" + String(SMP.file));
    drawNoSD();
    FastLEDclear();
    char OUTPUTf[50];
    if (autoload) {
      sprintf(OUTPUTf, "autosaved.txt");
    } else {
      sprintf(OUTPUTf, "%d.txt", SMP.file);
    }
    if (SD.exists(OUTPUTf)) {
      // showNumber(SMP.file, CRGB(0, 0, 50), 0);
      File loadFile = SD.open(OUTPUTf);
      if (loadFile) {
        unsigned int sdry = 1;
        unsigned int sdrx = 1;

        while (loadFile.available()) {
          int b = loadFile.read();

          // Check for the marker indicating the end of notes
          if (b == 0xFF && loadFile.peek() == 0xFE) {
            loadFile.read();  // Consume the second marker byte
            break;            // Exit the loop to load SMP data
          }

          int v = loadFile.read();
          note[sdrx][sdry][0] = b;
          note[sdrx][sdry][1] = v;
          sdry++;
          if (sdry > maxY) {
            sdry = 1;
            sdrx++;
          }
          if (sdrx > maxlen)
            sdrx = 1;
        }

        // Load SMP struct after marker
        if (loadFile.available()) {
          loadFile.read((uint8_t *)&SMP, sizeof(SMP));
        }
      }
      loadFile.close();

      Mode *bpm_vol = &volume_bpm;
      bpm_vol->pos[2] = SMP.bpm;
      playNoteInterval = ((60 * 1000 / SMP.bpm) / 4) * 1000;
      playTimer.update(playNoteInterval);

      if (!isEncoder4Defined) {
        bpm_vol->pos[0] = SMP.vol;
      } else {
        bpm_vol->pos[3] = SMP.vol;
      }

      float vol = float(SMP.vol / 10.0);
      if (vol <= 1.0)
        sgtl5000_1.volume(vol);
      // set all Filters
      for (unsigned int i = 0; i < maxFilters; i++) {
        filters[i]->frequency(100 * SMP.filter_knob[i]);
      }
      SMP.singleMode = false;

      // Display the loaded SMP data
      serialprintln("Loaded SMP Data:");
      serialprintln("singleMode: " + String(SMP.singleMode));
      serialprintln("currentChannel: " + String(SMP.currentChannel));
      serialprintln("vol: " + String(SMP.vol));
      serialprintln("bpm: " + String(SMP.bpm));
      serialprintln("velocity: " + String(SMP.velocity));
      serialprintln("page: " + String(SMP.page));
      serialprintln("edit: " + String(SMP.edit));
      serialprintln("file: " + String(SMP.file));
      serialprintln("pack: " + String(SMP.pack));
      serialprintln("wav: " + String(SMP.wav));
      serialprintln("folder: " + String(SMP.folder));
      serialprintln("activeCopy: " + String(SMP.activeCopy));
      serialprintln("x: " + String(SMP.x));
      serialprintln("y: " + String(SMP.y));
      serialprintln("seek: " + String(SMP.seek));
      serialprintln("seekEnd: " + String(SMP.seekEnd));
      serialprintln("smplen: " + String(SMP.smplen));
      serialprintln("shiftX: " + String(SMP.shiftX));


      SMP.seek=0;
      SMP.seekEnd=0;
      SMP.smplen = 0;
      SMP.shiftX=0;
      SMP.shiftY=0;

      serialprintln("filter_knob: ");
      for (unsigned int i = 0; i < maxFilters; i++) {
        serialprint(SMP.filter_knob[i]);
        serialprint(", ");
      }

      serialprintln("mute: ");
      for (unsigned int i = 0; i < maxY; i++) {
        serialprint(SMP.mute[i]);
        serialprint(", ");
      }
      serialprintln();
    } else {
      for (unsigned int nx = 1; nx < maxlen; nx++) {
        for (unsigned int ny = 1; ny < maxY + 1; ny++) {
          note[nx][ny][0] = 0;
          note[nx][ny][1] = defaultVelocity;
        }
      }
    }

    updateLastPage();

    if (!autoload) {
      delay(500);
      switchMode(&draw);
    }
  }

  void autoLoad() {
    loadPattern(true);
  }


  
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
