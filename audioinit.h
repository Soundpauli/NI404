
//AudioPlayArrayResmp sounds[1];
AudioPlayArrayResmp sound0;  // PREV
AudioPlayArrayResmp sound1;
AudioPlayArrayResmp sound2;
AudioPlayArrayResmp sound3;
AudioPlayArrayResmp sound4;
AudioPlayArrayResmp sound5;
AudioPlayArrayResmp sound6;
AudioPlayArrayResmp sound7;
AudioPlayArrayResmp sound8;
AudioPlayArrayResmp sound9;
AudioPlayArrayResmp sound10;
AudioPlayArrayResmp sound11;
AudioPlayArrayResmp sound12;
AudioPlayArrayResmp sound13;
AudioSynthWaveform     waveform1;  // SYNTH


AudioEffectEnvelope envelope0;
AudioEffectEnvelope envelope1;
AudioEffectEnvelope envelope2;
AudioEffectEnvelope envelope3;
AudioEffectEnvelope envelope4;
AudioEffectEnvelope envelope5;
AudioEffectEnvelope envelope6;
AudioEffectEnvelope envelope7;
AudioEffectEnvelope envelope8;
AudioEffectEnvelope envelope9;
AudioEffectEnvelope envelope10;
AudioEffectEnvelope envelope11;
AudioEffectEnvelope envelope12;
AudioEffectEnvelope envelope13;
AudioEffectEnvelope envelope14;

AudioFilterStateVariable filter3;
AudioFilterStateVariable filter14;

// Create Audio connections between the components
AudioConnection pc0(sound0, envelope0);  //prev
AudioConnection pc10(sound1, envelope1);
AudioConnection pc20(sound2, envelope2);
AudioConnection pc30(sound3, envelope3);
AudioConnection pc40(sound4, envelope4);
AudioConnection pc50(sound5, envelope5);
AudioConnection pc60(sound6, envelope6);
AudioConnection pc70(sound7, envelope7);
AudioConnection pc80(sound8, envelope8);
AudioConnection pc90(sound9, envelope9);
AudioConnection pc100(sound10, envelope10);
AudioConnection pc110(sound11, envelope11);
AudioConnection pc120(sound12, envelope12);
AudioConnection pc130(sound13, envelope13);
AudioConnection pc140(waveform1, envelope14);


AudioMixer4 mix1_a;
AudioMixer4 mix1_b;
AudioMixer4 mix1_c;
AudioMixer4 mix1_d;

AudioMixer4 mix2;
AudioMixer4 mix3;
AudioMixer4 mix_master;
AudioOutputAnalog dac;
AudioOutputI2S headphones;

AudioConnection patchCord0(envelope0, 0, mix1_d, 2);  //prev
AudioConnection patchCord1(envelope1, 0, mix1_a, 0);
AudioConnection patchCord2(envelope2, 0, mix1_a, 1);
//AudioConnection patchCord3(envelope3, 0, mix1_a, 2);
AudioConnection patchCord4(envelope4, 0, mix1_a, 3);
AudioConnection patchCord5(envelope5, 0, mix1_b, 0);
AudioConnection patchCord6(envelope6, 0, mix1_b, 1);
AudioConnection patchCord7(envelope7, 0, mix1_b, 2);
AudioConnection patchCord8(envelope8, 0, mix1_b, 3);
AudioConnection patchCord9(envelope9, 0, mix1_c, 0);
AudioConnection patchCord10(envelope10, 0, mix1_c, 1);
AudioConnection patchCord11(envelope11, 0, mix1_c, 2);
AudioConnection patchCord12(envelope12, 0, mix1_c, 3);
AudioConnection patchCord13(envelope13, 0, mix1_d, 0);
AudioConnection patchCord16(envelope14, 0, mix1_d, 1);
AudioConnection pc141(envelope14, 0, filter14, 0);


//filter on 3
AudioConnection patchCord28(envelope3, 0, filter3, 0);
AudioConnection patchCord29(filter3, 0, mix1_a, 2);

//AudioConnection          patchCord14(drum0, 0, mix1_d, 2);
//AudioConnection          patchCord15(drum1, 0, mix1_d, 3);

AudioConnection patchCord17(mix1_a, 0, mix2, 0);
AudioConnection patchCord18(mix1_b, 0, mix2, 1);
AudioConnection patchCord19(mix1_c, 0, mix3, 0);
AudioConnection patchCord20(mix1_d, 0, mix3, 1);

AudioConnection patchCord21(mix2, 0, mix_master, 0);
AudioConnection patchCord22(mix3, 0, mix_master, 1);

AudioConnection patchCord23(mix_master, 0, headphones, 0);
AudioConnection patchCord24(mix_master, 0, headphones, 1);

AudioConnection patchCord25(mix_master, dac);
AudioControlSGTL5000 audioShield;


arraysampler _samplers[14];
static AudioEffectEnvelope *envelopes[] = { &envelope0, &envelope1, &envelope2, &envelope3, &envelope4, &envelope5, &envelope6, &envelope7, &envelope8, &envelope9, &envelope10, &envelope11, &envelope12, &envelope13, &envelope14 };
static AudioPlayArrayResmp *voices[] = { &sound0, &sound1, &sound2, &sound3, &sound4, &sound5, &sound6, &sound7, &sound8, &sound9, &sound10, &sound11, &sound12, &sound13 };


File myFile;
File myOrigFile;
File myDestFile;
File newSamplePack;
File sampleFile;


int PrevSampleRate = 1;
int SampleRate[16] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };




void allOff() {
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
