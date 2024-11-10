// GUItool: begin automatically generated code
AudioPlayArrayResmp sound0;  //xy=375,182
AudioPlayArrayResmp sound1;  //xy=375,182
AudioPlayArrayResmp sound2;  //xy=375,182

AudioPlayArrayResmp sound3;  //xy=375,182
AudioPlayArrayResmp sound4;  //xy=375,182
AudioPlayArrayResmp sound5;  //xy=375,182

AudioPlayArrayResmp sound6;  //xy=375,182
AudioPlayArrayResmp sound7;  //xy=375,182
AudioPlayArrayResmp sound8;  //xy=375,182

AudioPlayArrayResmp sound9;   //xy=375,182
AudioPlayArrayResmp sound10;  //xy=375,182
AudioPlayArrayResmp sound11;  //xy=375,182
AudioPlayArrayResmp sound12;  //xy=375,182

//AudioPlayArrayResmp sound13;  //xy=375,182
AudioSynthWaveform sound13;   // synth
AudioSynthWaveform sound14;   // synth
//AudioPlayArrayResmp sound15;  //xy=375,182


AudioFilterStateVariable filter0;
AudioFilterStateVariable filter1;
AudioFilterStateVariable filter2;
AudioFilterStateVariable filter3;
AudioFilterStateVariable filter4; 
AudioFilterStateVariable filter5;
AudioFilterStateVariable filter6;
AudioFilterStateVariable filter7;
AudioFilterStateVariable filter8;
AudioFilterStateVariable filter9;
AudioFilterStateVariable filter10;
AudioFilterStateVariable filter11;
AudioFilterStateVariable filter12;
AudioFilterStateVariable filter13;
AudioFilterStateVariable filter14;
//AudioFilterStateVariable filter15;

AudioEffectEnvelope envelope0;  //xy=520,177

AudioEffectEnvelope envelope1;  //xy=520,177
AudioEffectEnvelope envelope2;  //xy=520,177
AudioEffectEnvelope envelope3;  //xy=520,177
AudioEffectEnvelope envelope4;  //xy=520,177

AudioEffectEnvelope envelope5;  //xy=520,177
AudioEffectEnvelope envelope6;  //xy=520,177
AudioEffectEnvelope envelope7;  //xy=520,177
AudioEffectEnvelope envelope8;  //xy=520,177

AudioEffectEnvelope envelope9;   //xy=520,177
AudioEffectEnvelope envelope10;  //xy=520,177
AudioEffectEnvelope envelope11;  //xy=520,177
AudioEffectEnvelope envelope12;  //xy=520,177

AudioEffectEnvelope envelope13;  //xy=520,177
AudioEffectEnvelope envelope14;  //xy=520,177
//AudioEffectEnvelope envelope15;  //xy=520,177


AudioMixer4 mixer1;  //xy=736,230
AudioMixer4 mixer2;  //xy=736,230
AudioMixer4 mixer3;  //xy=736,230
AudioMixer4 mixer4;  //xy=736,230

AudioMixer4 mixer_end;  //xy=736,230
AudioOutputI2S i2s1;    //xy=1082,325

AudioConnection patchCord1_1(sound1, envelope1);
AudioConnection patchCord1_2(envelope1, 0, filter1, 0);
AudioConnection patchCord1_3(filter1, 0, mixer1, 0);


AudioConnection patchCord2_1(sound2, envelope2);
AudioConnection patchCord2_2(envelope2, 0, filter2, 0);
AudioConnection patchCord2_3(filter2, 0, mixer1, 1);

AudioConnection patchCord3_1(sound3, envelope3);
AudioConnection patchCord3_2(envelope3, 0, filter3, 0);
AudioConnection patchCord3_3(filter3, 0, mixer1, 2);

AudioConnection patchCord4_1(sound4, envelope4);
AudioConnection patchCord4_2(envelope4, 0, filter4, 0);
AudioConnection patchCord4_3(filter4, 0, mixer1, 3);


AudioConnection patchCord5_1(sound5, envelope5);
AudioConnection patchCord5_2(envelope5, 0, filter5, 0);
AudioConnection patchCord5_3(filter5, 0, mixer2, 0);

AudioConnection patchCord6_1(sound6, envelope6);
AudioConnection patchCord6_2(envelope6, 0, filter6, 0);
AudioConnection patchCord6_3(filter6, 0, mixer2, 1);

AudioConnection patchCord7_1(sound7, envelope7);
AudioConnection patchCord7_2(envelope7, 0, filter7, 0);
AudioConnection patchCord7_3(filter7, 0, mixer2, 2);

AudioConnection patchCord8_1(sound8, envelope8);
AudioConnection patchCord8_2(envelope8, 0, filter8, 0);
AudioConnection patchCord8_3(filter8, 0, mixer2, 3);


AudioConnection patchCord9_1(sound9, envelope9);
AudioConnection patchCord9_2(envelope9, 0, filter9, 0);
AudioConnection patchCord9_3(filter9, 0, mixer3, 0);

AudioConnection patchCord10_1(sound10, envelope10);
AudioConnection patchCord10_2(envelope10, 0, filter10, 0);
AudioConnection patchCord10_3(filter10, 0, mixer3, 1);

AudioConnection patchCord11_1(sound11, envelope11);
AudioConnection patchCord11_2(envelope11, 0, filter11, 0);
AudioConnection patchCord11_3(filter11, 0, mixer3, 2);

AudioConnection patchCord12_1(sound12, envelope12);
AudioConnection patchCord12_2(envelope12, 0, filter12, 0);
AudioConnection patchCord12_3(filter12, 0, mixer3, 3);

AudioConnection patchCord13_1(sound13, envelope13);
AudioConnection patchCord13_2(envelope13, 0, filter13, 0);
AudioConnection patchCord13_3(filter13, 0, mixer4, 0);

AudioConnection patchCord14_1(sound14, envelope14);
AudioConnection patchCord14_2(envelope14, 0, filter14, 0);
AudioConnection patchCord14_3(filter14, 0, mixer4, 1);
/*
AudioConnection patchCord15_1(sound15, envelope15);
AudioConnection patchCord15_2(envelope15, 0, filter15, 0);
AudioConnection patchCord15_3(filter15, 0, mixer4, 2);*/

AudioConnection patchCord0_1(sound0, envelope0);
AudioConnection patchCord0_2(envelope0, 0, mixer4, 3);


AudioConnection patchCordEnd_1(mixer1, 0, mixer_end, 0);
AudioConnection patchCordEnd_2(mixer2, 0, mixer_end, 1);
AudioConnection patchCordEnd_3(mixer3, 0, mixer_end, 2);
AudioConnection patchCordEnd_4(mixer4, 0, mixer_end, 3);


AudioConnection patchCord_end1(mixer_end, 0, i2s1, 0);
AudioConnection patchCord_end2(mixer_end, 0, i2s1, 1);
AudioControlSGTL5000 sgtl5000_1;  //xy=887,463
// GUItool: end automatically generated code
