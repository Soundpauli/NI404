/* Audio Library for Teensy
 * Copyright (c) 2021, Nic Newdigate
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef TEENSY_AUDIO_SAMPLER_SAMPLEPLAYMIDICONTROLLER_H
#define TEENSY_AUDIO_SAMPLER_SAMPLEPLAYMIDICONTROLLER_H

#include <Arduino.h>
#include <vector>
#include <map>
#include <SD.h>
#include "loopsampler.h"
#include "loopsamplerenums.h"

class sdsampleplayernote {
public:
    char * _filename = nullptr;
    uint8_t _samplerNoteNumber = 0;    
    uint8_t _samplerNoteChannel = 0;    
    int _sampleIndex = -1;
    uint8_t _indexOfNoteToPlay = 0;
    bool isPlaying = false;
};

enum playcontrollerstate {
    playcontrollerstate_initialising = 0, // need to ascertain which midi notes and channels correspond to which control functions
    playcontrollerstate_performing = 1,
    playcontrollerstate_selecting_target = 2,
    playcontrollerstate_editing_target = 3,
};

enum triggerctrlfunction {
    triggerctrlfunction_none = 0,
    triggerctrlfunction_changetriggertype = 1,
    triggerctrlfunction_changedirection = 2,
    triggerctrlfunction_changelooptype = 3,
    triggerctrlfunction_changesample = 4,
    triggerctrlfunction_selector_cc = 5,
};

class playcontrollerconfig {
public:
    const char *prompts[5] = {"Trigger type", "Direction", "Loop type", "sample", "value selector"}; 
    uint8_t midinotenum_changeTriggerType, midichannelnum_changeTriggerType,
         midinotenum_changeDirection,   midichannelnum_changeDirection,
         midinotenum_changeLoopType,    midichannelnum_changeLoopType,
         midinotenum_changeSample,      midichannelnum_changeSample,
         midiccnum_valueSelector,       midichannelnum_valueSelector;
    uint8_t num_ctrl_fns_configured = 0;
    
    void prompt() {
        if (num_ctrl_fns_configured < 4 ) {
            Serial.printf("%d: please press a midi note to select control key for '%s'.\n", num_ctrl_fns_configured, prompts[num_ctrl_fns_configured]);
        } else if (num_ctrl_fns_configured == 4) {
            Serial.printf("%d: please change a custom control (cc) to select a '%s'.\n", num_ctrl_fns_configured, prompts[num_ctrl_fns_configured]);
        }else 
            Serial.printf("%d: no more functions to configure...\n", num_ctrl_fns_configured);
    }

    void configure(uint8_t noteOrCcNum, uint8_t channel) {
        if (num_ctrl_fns_configured >= sizeof(prompts)/sizeof(prompts[0]) ) {
            Serial.printf("WARN: no more functions to configure!!! (%d)\n", num_ctrl_fns_configured);
            return;
        }
        switch (num_ctrl_fns_configured) {
            case 0: {
                midinotenum_changeTriggerType = noteOrCcNum;
                midichannelnum_changeTriggerType = channel;
                num_ctrl_fns_configured++;
                Serial.printf("%[configured: change trigger type ctrl key=%d, channel=%d]\n", noteOrCcNum, channel);
                break;
            }
            case 1: {
                if (!note_is_ctrl_function_key(noteOrCcNum, channel, 
                        triggerctrlfunction::triggerctrlfunction_changetriggertype)) {
                    midinotenum_changeDirection = noteOrCcNum;
                    midichannelnum_changeDirection = channel;
                    num_ctrl_fns_configured++;
                    Serial.printf("[configured: change direction ctrl key=%d, channel=%d]\n", noteOrCcNum, channel);
                } else {
                    Serial.printf("WARN: %d: conflict: that note/channel is being used already\n", num_ctrl_fns_configured);
                }
                break;
            }
            case 2: {
                if (!note_is_ctrl_function_key(noteOrCcNum, channel, 
                        triggerctrlfunction::triggerctrlfunction_changetriggertype) 
                    && !note_is_ctrl_function_key(noteOrCcNum, channel, 
                        triggerctrlfunction::triggerctrlfunction_changedirection)) {
                    midinotenum_changeLoopType = noteOrCcNum;
                    midichannelnum_changeLoopType = channel;
                    num_ctrl_fns_configured++;
                    Serial.printf("[configured: change loop type ctrl key=%d, channel=%d]\n", noteOrCcNum, channel);
                } else {
                    Serial.printf("WARN: %d: conflict: that note/channel is being used already\n", num_ctrl_fns_configured);
                }
                break;     
            }                 
            case 3: {
                if (!note_is_ctrl_function_key(noteOrCcNum, channel, 
                        triggerctrlfunction::triggerctrlfunction_changetriggertype) 
                    && !note_is_ctrl_function_key(noteOrCcNum, channel, 
                        triggerctrlfunction::triggerctrlfunction_changedirection)
                    && !note_is_ctrl_function_key(noteOrCcNum, channel, 
                        triggerctrlfunction::triggerctrlfunction_changelooptype)) {
                    midinotenum_changeSample = noteOrCcNum;
                    midichannelnum_changeSample = channel;
                    num_ctrl_fns_configured++;
                    Serial.printf("[configured: change sample select ctrl key=%d, channel=%d]\n", noteOrCcNum, channel);

                } else {
                    Serial.printf("WARN: %d: conflict: that note/channel is being used already\n", num_ctrl_fns_configured);
                }
                break;     
            }
            case 4: {
                midiccnum_valueSelector = noteOrCcNum;
                midichannelnum_valueSelector = channel;
                num_ctrl_fns_configured++;
                Serial.printf("[configured: value-selector cc=%d, channel=%d]\n", noteOrCcNum, channel);
                break;     
            }            
        }
        if (!complete())
            prompt();
        else
            saveToFile("device.ctl");
    }

    void reset() {
        num_ctrl_fns_configured = 0;
        midinotenum_changeTriggerType = 0;
        midichannelnum_changeTriggerType = 0;
        midinotenum_changeDirection = 0;
        midichannelnum_changeDirection = 0;
        midinotenum_changeLoopType = 0; 
        midichannelnum_changeLoopType = 0;
        midinotenum_changeSample = 0;    
        midichannelnum_changeSample = 0;
        midiccnum_valueSelector = 0;     
        midichannelnum_valueSelector = 0;
    }

    bool complete() {
        return num_ctrl_fns_configured >= 5;
    }

    bool readReadFromFile(const char * filename) {
        File file = SD.open(filename);
        if (file) {
            playcontrollerconfig config;
            size_t bytesRead = file.read((char*)&config, sizeof(playcontrollerconfig));
            file.close();
            Serial.printf("config bytes read: %d\n", bytesRead);
            if (bytesRead >= sizeof(playcontrollerconfig)) {
                midinotenum_changeTriggerType      = config.midinotenum_changeTriggerType;
                midichannelnum_changeTriggerType   = config.midichannelnum_changeTriggerType;
                midinotenum_changeDirection        = config.midinotenum_changeDirection;
                midichannelnum_changeDirection     = config.midichannelnum_changeDirection;
                midinotenum_changeLoopType         = config.midinotenum_changeLoopType;
                midichannelnum_changeLoopType      = config.midichannelnum_changeLoopType;        
                midinotenum_changeSample           = config.midinotenum_changeSample;    
                midichannelnum_changeSample        = config.midichannelnum_changeSample;    
                midiccnum_valueSelector            = config.midiccnum_valueSelector;    
                midichannelnum_valueSelector       = config.midichannelnum_valueSelector;
                return true;
            }
            return false;
        } else {
            return false;
        }
    }
    
    bool saveToFile(const char * filename) {
        File file = SD.open(filename, O_WRITE);
        if (file) {
            size_t bytesWritten = file.write((const unsigned char*)this, sizeof(playcontrollerconfig));
            //Serial.printf("config bytes written: %d\n", bytesWritten);
            file.close();
            return bytesWritten == sizeof(playcontrollerconfig);
        } 
        return false;
    }


    bool iscontrolkey(uint8_t noteNum, uint8_t channel) {
        return getfunctionkey(false, noteNum, channel) == triggerctrlfunction_none;
    }

    bool iscontrolcc(uint8_t ccNum, uint8_t channel) {
        return getfunctionkey(true, ccNum, channel) == triggerctrlfunction_selector_cc;
    }

    triggerctrlfunction getfunctionkey(bool isCC, uint8_t noteNum, uint8_t channel) {
        if (!isCC) {
            if (note_is_ctrl_function_key(noteNum, channel,  triggerctrlfunction_changedirection))
                return triggerctrlfunction_changedirection;

            if (note_is_ctrl_function_key(noteNum, channel, triggerctrlfunction_changelooptype ))
                return triggerctrlfunction_changelooptype;

            if (note_is_ctrl_function_key(noteNum, channel, triggerctrlfunction_changesample ))
                return triggerctrlfunction_changesample;

            if (note_is_ctrl_function_key(noteNum, channel, triggerctrlfunction_changetriggertype ))
                return triggerctrlfunction_changetriggertype;
        } else {
            // midi cc
            if (ccnum_is_ctrl_special_controller(noteNum, channel, triggerctrlfunction_selector_cc ))
                return triggerctrlfunction_selector_cc;
        }
        return triggerctrlfunction_none;
    }

private:
    bool note_is_ctrl_function_key(uint8_t note, uint8_t channel, triggerctrlfunction funct) {
        switch (funct) {
            case triggerctrlfunction::triggerctrlfunction_changetriggertype:
                return (note == midinotenum_changeTriggerType && channel == midichannelnum_changeTriggerType);
            case triggerctrlfunction::triggerctrlfunction_changedirection:
                return (note == midinotenum_changeDirection && channel == midichannelnum_changeDirection);
            case triggerctrlfunction::triggerctrlfunction_changelooptype:
                return (note == midinotenum_changeLoopType && channel == midichannelnum_changeLoopType);
            case triggerctrlfunction::triggerctrlfunction_changesample:
                return (note == midinotenum_changeSample && channel == midichannelnum_changeSample);
            default:{
                Serial.printf("Not sure about ctrl function #%d\n", (int)funct);
                return false;
            }
        }
    }

    bool ccnum_is_ctrl_special_controller(uint8_t ccnum, uint8_t channel, triggerctrlfunction funct) {
        switch (funct) {
            case triggerctrlfunction::triggerctrlfunction_selector_cc:
                return (ccnum == midiccnum_valueSelector && channel == midichannelnum_valueSelector);                
            default:{
                Serial.printf("Not sure about cc ctrl function #%d\n", (int)funct);
                return false;
            }
        }
    }

};

class sdsampleplayermidicontroller {
public:
    sdsampleplayermidicontroller(loopsampler &loopsampler) : _loopsampler(loopsampler) {

    }

    void initialize() {
        _config.reset();
        _state = playcontrollerstate::playcontrollerstate_initialising;
        _config.prompt();
    }

    void begin(const char* directoryname = nullptr) {
        populateFilenames(directoryname);
        
        if (_config.readReadFromFile("device.ctl")) {
            Serial.printf("loaded settings from file.\n");
            _state = playcontrollerstate::playcontrollerstate_performing;
        } else {
            _config.prompt();
        }
        if (loadSamples("samples.smp")){
            Serial.println("loaded sample config");
        }
    }

    void midiChannleVoiceMessage(uint8_t status, uint8_t data1, uint8_t data2, uint8_t channel) {
        bool isNoteOn = (status & 0xf0) == 0x90;
        bool isCC = (status & 0xf0) == 0xc0;
        switch (_state) {

            case playcontrollerstate_initialising : {
                // we are learning the ctrl function keys 
                if (isNoteOn || isCC ) // midi key-down or cc status nibble
                    _config.configure(data1, channel);
                    if (_config.complete()) {
                        _state = playcontrollerstate::playcontrollerstate_performing;
                        Serial.printf("[controller configuration complete - switching to performing mode]\n");
                    }
                break;
            }
            
            case playcontrollerstate_performing: {

                triggerctrlfunction potentialFnKey = 
                    (isCC || isNoteOn)? _config.getfunctionkey(isCC, data1, channel) : triggerctrlfunction_none;

                switch (potentialFnKey) {
                    case triggerctrlfunction_none: {
                        // not a function key - regular performance note...
                        // try find a sample mapping for this fellow
                        sdsampleplayernote *sample = getSamplerNoteForNoteNum(data1, channel);
                        if (sample) {
                            if (isNoteOn) {
                                sampletrigger_received(sample);
                            } else  
                            {
                                bool isNoteOff = (status & 0xf0) == 0x80;
                                if (isNoteOff)
                                    sampletriggerstop_received(sample);
                            }
                        }
                        break;
                    }
                    case triggerctrlfunction_changesample: {
                        if (isNoteOn) {
                            _selected_ctrl_function = triggerctrlfunction_changesample;
                            _state = playcontrollerstate::playcontrollerstate_selecting_target;
                            Serial.println("Select a trigger note to change the sample...");
                        }
                        break;
                    }
                    case triggerctrlfunction_changedirection: {
                        _selected_ctrl_function = triggerctrlfunction_changedirection;
                        _state = playcontrollerstate::playcontrollerstate_selecting_target;
                        Serial.println("Select a trigger note to change direction...");
                        break;
                    }
                    case triggerctrlfunction_changelooptype: {
                        _selected_ctrl_function = triggerctrlfunction_changelooptype;
                        _state = playcontrollerstate::playcontrollerstate_selecting_target;
                        Serial.println("Select a trigger note to change looptype...");
                        break;
                    }
                    case triggerctrlfunction_changetriggertype: {
                        _selected_ctrl_function = triggerctrlfunction_changetriggertype;
                        _state = playcontrollerstate::playcontrollerstate_selecting_target;
                        Serial.println("Select a trigger note to change trigger type...");
                        break;
                    }                                        
                }
                break;
            }
            case playcontrollerstate_selecting_target: {
                if (isNoteOn) {
                    // midi key-down status
                    _selected_target = getSamplerNoteForNoteNum(data1, channel);
                    if (_selected_target == nullptr) {
                        _selected_target = new sdsampleplayernote();
                        _selected_target->_samplerNoteNumber = data1;
                        _selected_target->_samplerNoteChannel = channel;                        
                        _samples.push_back(_selected_target);
                    }
                    Serial.printf("[selected note=%d, channel=%d]\n", data1, channel);
                    _state = playcontrollerstate::playcontrollerstate_editing_target;
                }
                break;
            }
            case playcontrollerstate_editing_target: {
                triggerctrlfunction fntype = _config.getfunctionkey(isCC, data1, channel);
                if (fntype == _selected_ctrl_function) {
                    // the note pressed was the select ctrl function already selecected, go back to performance mode
                    _state = playcontrollerstate_performing;
                    _selected_ctrl_function = triggerctrlfunction_none;
                    Serial.println("[switch back to performance mode]");
                    break;
                }

                if (fntype == triggerctrlfunction_selector_cc 
                    && _selected_target != nullptr) {  
                    
                    switch (_selected_ctrl_function) {
                        case triggerctrlfunction_none: {
                            // shouldn't ever reach here...
                            break;
                        }

                        case triggerctrlfunction_changetriggertype: {
                            triggertype triggerType = (triggertype)round(data2 * 4.0 / 128.0); 
                            sdloopaudiosample *sample = _loopsampler.findSampleForKey(_selected_target->_indexOfNoteToPlay);
                            if (sample) {
                                if (triggerType != sample->_triggertype){
                                    sample->_triggertype = triggerType;
                                    Serial.printf("sample %d, %d changed trigger type to %d: %s\n", _selected_target->_samplerNoteNumber, _selected_target->_samplerNoteChannel, (int)triggerType, getTriggerTypeName(triggerType));
                                }
                            }
                            break;
                        }

                        case triggerctrlfunction_changesample: {
                            size_t num_samples = _filenames.size();
                            size_t sampleIndex = round(data2 * num_samples / 128.0);
                            if (_selected_target->_sampleIndex != sampleIndex) {
                                if (_selected_target->_filename) 
                                    delete [] _selected_target->_filename;
                                _selected_target->_sampleIndex = sampleIndex;
                                if (sampleIndex >= num_samples )
                                    sampleIndex = num_samples-1;
                                size_t filename_length = strlen(_filenames[sampleIndex])+1;
                                _selected_target->_filename = new char[filename_length] {0};
                                memcpy(_selected_target->_filename, _filenames[sampleIndex], strlen(_filenames[sampleIndex]) );
                                
                                _selected_target->_indexOfNoteToPlay = sampleIndex;

                                
                                Serial.printf("sample %d, %d changed sample to %s (sampleIndex=%d)\n", _selected_target->_samplerNoteNumber, _selected_target->_samplerNoteChannel, _selected_target->_filename, sampleIndex);
                            }
                            break;
                        }

                        case triggerctrlfunction_changedirection:{
                            playdirection playDirection = (playdirection) round( data2 / 128.0); 
                            sdloopaudiosample *sample = _loopsampler.findSampleForKey(_selected_target->_indexOfNoteToPlay);
                            if (sample) {
                                if (playDirection != sample->_playdirection){
                                    sample->_playdirection = playDirection;
                                    Serial.printf("sample %d, %d changed play dir to %s\n", _selected_target->_samplerNoteNumber, _selected_target->_samplerNoteChannel, getPlayDirectionName(playDirection));
                                }
                            }
                            break;
                        }

                        case triggerctrlfunction_changelooptype: {
                            playlooptype playLoopType = (playlooptype)(round(data2 * 2.0 / 128.0)); 
                            sdloopaudiosample *sample = _loopsampler.findSampleForKey(_selected_target->_indexOfNoteToPlay);
                            if (sample) {
                                if (playLoopType != sample->_playlooptype){
                                    sample->_playlooptype = playLoopType;
                                    Serial.printf("sample %d, %d changed loop type to %s\n", _selected_target->_samplerNoteNumber, _selected_target->_samplerNoteChannel, getPlayLoopTypeName(playLoopType));
                                }
                            }
                            break;
                        }
                    }
                }
                break;
            }


            default:
                break;
        }
    }

    bool loadSamples(const char* filename) {
        File file = SD.open(filename);
        if (file) {
            sdsampleplayernote *note;
            while((note = read_sample(file)) != nullptr) {
                _samples.push_back(note);
                Serial.printf("loaded sample %d: %s\n", note->_sampleIndex, note->_filename);
            }
            file.close();
            return true;
        }
        Serial.printf("Not able to open %s\n", filename);
        return false;
    }

    void writeSamples(const char* filename) {
        File file = SD.open(filename, O_WRITE);
        for (auto && note : _samples) {
            write_sample(file, note);
        }
        file.close();
    }
private:
    loopsampler &_loopsampler;
    playcontrollerconfig _config;
    playcontrollerstate _state = playcontrollerstate::playcontrollerstate_initialising;
    sdsampleplayernote *_selected_target = nullptr;
    triggerctrlfunction _selected_ctrl_function = triggerctrlfunction::triggerctrlfunction_changetriggertype;

    std::vector<sdsampleplayernote*> _samples;
    std::vector<char *> _filenames;

    void unloadFilenames() {
        _loopsampler.removeAllSamples();
        for (auto && filename : _filenames) {
            delete [] filename;
        }
        _filenames.clear();
    }
    void unloadSamples() {
        for (auto && sample : _samples) {
            if (sample->_filename)
                delete sample->_filename;
            delete sample;
        }
        _samples.clear();
    }
    void populateFilenames(const char *directory) {
        unloadFilenames();
        File dir = directory? SD.open(directory) : SD.open("/");
        unsigned int index = 0;
        while (true) { 

            File files =  dir.openNextFile();
            if (!files) {
                //If no more files, break out.
                break;
            }

            String curfile = files.name(); //put file in string
            int m = curfile.lastIndexOf(".WAV");
            int a = curfile.lastIndexOf(".wav");
            int underscore = curfile.indexOf("_");
            int dot = curfile.indexOf(".");

            // if returned results is more then 0 add them to the list.
            if ((m > 0 || a > 0) && (underscore != 0) && (dot != 0)) {  
                char *filename = new char[curfile.length()+1] {0};
                memcpy(filename, curfile.c_str(), curfile.length());
                _filenames.push_back(filename);
                _loopsampler.addSample(index, filename);
                //Serial.printf("filename: %d: %s\n", index, filename);
                index++;
            } // else 
              //  Serial.printf("INGORE: filename: %d: %s\n", index, curfile.c_str());
            files.close();
        }
        // close 
        dir.close();
    }
    sdsampleplayernote* getSamplerNoteForNoteNum(uint8_t noteNum, uint8_t channel) {
        for (auto && note : _samples) {
            if (note->_samplerNoteNumber == noteNum && note->_samplerNoteChannel == channel)
                return note;
        }
        return nullptr;
    }
    static const char* getTriggerTypeName(triggertype tt){
        switch (tt) {
            case triggertype_play_until_end: 
                return "play until end";
            case triggertype_play_until_subsequent_notedown:
                return "toggle";
            case triggertype_play_while_notedown: 
                return "play while notedown";
            default:
                return "not sure?";
        }
    }   
    static const char* getPlayDirectionName(playdirection pd){
        switch (pd) {
            case playdirection_begin_forward: 
                return "begin forwards";
            case playdirection_begin_backward:
                return "begin backwards";
            default:
                return "not sure?";
        }
    }
    static const char* getPlayLoopTypeName(playlooptype pd){
        switch (pd) {
            case playlooptype_once: return "play once";
            case playlooptype_looping: return "looping";
            case playlooptype_pingpong: return "ping-pong";
            default:
                return "not sure?";
        }
    }

    void sampletrigger_received(sdsampleplayernote *sample) {        
        sdloopaudiosample *loop_audiosample = _loopsampler.findSampleForKey(sample->_indexOfNoteToPlay);
        if (!loop_audiosample) {
            return;
        }

        switch (loop_audiosample->_triggertype) {
            case triggertype_play_until_end :
            case triggertype_play_while_notedown : {
                _loopsampler.noteEvent(sample->_indexOfNoteToPlay, 127, false, false); // turn it off first
                _loopsampler.noteEvent(sample->_indexOfNoteToPlay, 127, true, false);
                sample->isPlaying = true;
                break;
            }

            case triggertype_play_until_subsequent_notedown: {
                if (sample->isPlaying) {
                    _loopsampler.noteEvent(sample->_indexOfNoteToPlay, 127, false, false); // turn it off 
                    sample->isPlaying = false;
                } else {
                    _loopsampler.noteEvent(sample->_indexOfNoteToPlay, 127, true, false); // turn it on 
                    sample->isPlaying = true;
                }
                break;
            }

            default: break;
        }
    }
    
    void sampletriggerstop_received(sdsampleplayernote *sample) {        
        sdloopaudiosample *loop_audiosample = _loopsampler.findSampleForKey(sample->_indexOfNoteToPlay);
        if (!loop_audiosample) {
            return;
        }

        switch (loop_audiosample->_triggertype) {
            case triggertype_play_until_end :
            case triggertype_play_until_subsequent_notedown :
            break;
            
            case triggertype_play_while_notedown :
            {
                _loopsampler.noteEvent(sample->_indexOfNoteToPlay, 127, false, false); // turn it off 
                sample->isPlaying = false;
                break;
            }

            default: break;
        }
    }

    sdsampleplayernote* read_sample(File file) {

        uint16_t progress = 0;
        uint32_t filename_length = varfieldRead(file, progress);

        if (progress == 0)
            return nullptr;

        sdsampleplayernote *samplernote = new sdsampleplayernote();
        samplernote->_filename = new char[filename_length + 1] {0};
        
        size_t bytesRead = file.read( samplernote->_filename, filename_length);      
        if (bytesRead != filename_length) {
            Serial.printf("WARN: could not load sample '%s' (1).\n", samplernote->_filename);
            delete samplernote->_filename;
            delete samplernote;
            return nullptr;
        }
            
        bytesRead = file.read(&(samplernote->_samplerNoteNumber), 1 );
        if (bytesRead != 1) {
            Serial.printf("WARN: could not load sample '%s' (2).\n", samplernote->_filename);
            delete samplernote->_filename;
            delete samplernote;
            return nullptr;
        } 
        
        bytesRead = file.read(&(samplernote->_samplerNoteChannel), 1 );
        if (bytesRead != 1) {
            Serial.printf("WARN: could not load sample '%s' (3).\n", samplernote->_filename);
            delete samplernote->_filename;
            delete samplernote;
            return nullptr;
        } 
        
        bytesRead = file.read(&(samplernote->_indexOfNoteToPlay), 1 );
        if (bytesRead != 1) {
            Serial.printf("WARN: could not load sample '%s' (4).\n", samplernote->_filename);
            delete samplernote->_filename;
            delete samplernote;
            return nullptr;
        } 

        int indexOfFile = getIndexOfSDWaveFile(samplernote->_filename);
        if (indexOfFile == -1) {
            Serial.printf("WARN: could not load sample '%s' (5).\n", samplernote->_filename);
            delete samplernote->_filename;
            delete samplernote;
            return nullptr;
        }
        samplernote->_sampleIndex = indexOfFile;
        _loopsampler.addSample(indexOfFile, samplernote->_filename);
        //Serial.printf("loaded sample: %d: %s\n", indexOfFile, samplernote->_filename);
        return samplernote;
    }

    int getIndexOfSDWaveFile(const char *filenameToMatch) {
        int index = 0;
        for(auto && filename : _filenames) {
            if (strcmp(filename, filenameToMatch) == 0) 
                return index;
            index++;
        }
        return -1;
    }

    void write_sample(File file, sdsampleplayernote *samplernote) {
        size_t filename_length = strlen(samplernote->_filename);
        varfieldWrite(file, filename_length);
        file.write((const unsigned char *)samplernote->_filename, filename_length);
        file.write( &(samplernote->_samplerNoteNumber), 1);
        file.write( &(samplernote->_samplerNoteChannel), 1);
        file.write( &(samplernote->_indexOfNoteToPlay), 1);
    }

    uint32_t varfieldRead(File &file, uint16_t &progress)
    {
        uint32_t ret = 0;
        uint8_t byte_in;

        for (;;)
        {
            if (file.available()) {
                // return error
                byte_in = file.read();
                progress++;
            } else return 0;
            ret = (ret << 7) | (byte_in & 0x7f);
            if (!(byte_in & 0x80))
                return ret;
        }
    }
    void varfieldWrite(File &file, unsigned int deltaticks) {
        if (deltaticks < 128) {
            file.write((const unsigned char *)&deltaticks, 1);
            return;
        } 

        uint16_t lengthFieldSize = 0;
        uint8_t b[4];
        
        // read least signficicant bytes first
        for (int i = 3; i >= 0; i--) {
            b[i] = (uint8_t)(deltaticks & 0x7f);
            if(i < 3) // set the bit that indicates another byte still to follow... except on the least significant byte
                b[i] |= 0x80;
            deltaticks >>= 7;
            lengthFieldSize++;
            if (deltaticks < 1)
                break;
        }

        for( int i=0; i < lengthFieldSize; i++) {  
            file.write(&(b[4-lengthFieldSize+i]), 1);
        }
    }

};