#include <string.h>
#include "MidiHandler.h"
#include "MidiStatus.h"
#include "MidiController.h"
#include "OpenWareMidiControl.h"
#include "FirmwareLoader.hpp"
#include "ApplicationSettings.h"
#include "errorhandlers.h"
#ifdef USE_CODEC
#include "Codec.h"
#endif
#include "Owl.h"
#include "FlashStorage.h"
#include "PatchRegistry.h"
#ifdef USE_DIGITALBUS
#include "bus.h"
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

static FirmwareLoader loader;

MidiHandler::MidiHandler() : channel(MIDI_OMNI_CHANNEL) {
  // memset(midi_values, 0, NOF_PARAMETERS*sizeof(uint16_t));
}

void MidiHandler::handlePitchBend(uint8_t status, uint16_t value){
  // setParameterValue(PARAMETER_G, ((int16_t)value - 8192)>>1);
}

void MidiHandler::handleNoteOn(uint8_t status, uint8_t note, uint8_t velocity){
  if(channel != MIDI_OMNI_CHANNEL && channel != getChannel(status))
    return;
  if(getProgramVector()->buttonChangedCallback != NULL)
    getProgramVector()->buttonChangedCallback(MIDI_NOTE_BUTTON+note, velocity<<5, getSampleCounter());
  // setButtonValue(MIDI_NOTE_BUTTON+note, velocity<<5);
}

void MidiHandler::handleNoteOff(uint8_t status, uint8_t note, uint8_t velocity){
  if(channel != MIDI_OMNI_CHANNEL && channel != getChannel(status))
    return;
  if(getProgramVector()->buttonChangedCallback != NULL)
    getProgramVector()->buttonChangedCallback(MIDI_NOTE_BUTTON+note, 0, getSampleCounter());
  // setButtonValue(MIDI_NOTE_BUTTON+note, 0);
}

void MidiHandler::handleProgramChange(uint8_t status, uint8_t pid){
  if(channel != MIDI_OMNI_CHANNEL && channel != getChannel(status))
    return;
  if(pid == 0){
    runProgram();
  }else{
    program.loadProgram(pid);
    program.resetProgram(true);
  }
}

void MidiHandler::handleControlChange(uint8_t status, uint8_t cc, uint8_t value){
  if(channel != MIDI_OMNI_CHANNEL && channel != getChannel(status))
    return;
  switch(cc){
#ifdef USE_CODEC
  case MIDI_CC_VOLUME:
    codec.setOutputGain(value);
    break;
#endif
  case PATCH_PARAMETER_A:
    setParameterValue(PARAMETER_A, value<<5); // scale from 7bit to 12bit value
    break;
  case PATCH_PARAMETER_B:
    setParameterValue(PARAMETER_B, value<<5);
    break;
  case PATCH_PARAMETER_C:
    setParameterValue(PARAMETER_C, value<<5);
    break;
  case PATCH_PARAMETER_D:
    setParameterValue(PARAMETER_D, value<<5);
    break;
  case PATCH_PARAMETER_E:
    setParameterValue(PARAMETER_E, value<<5);
    break;
  case MIDI_CC_MODULATION:
    setParameterValue(PARAMETER_F, value<<5);
    break;
  case MIDI_CC_EFFECT_CTRL_1:
    setParameterValue(PARAMETER_G, value<<5);
    break;
  case MIDI_CC_EFFECT_CTRL_2:
    setParameterValue(PARAMETER_H, value<<5);
    break;
  // case 21: {
  //   static uint8_t last = 0;
  //   encoderChanged(0, (value-last)*3);
  //   last = value;
  //   break;
  // }
  // case 22: {
  //   static uint8_t last = 0;
  //   encoderChanged(1, (value-last)*3);
  //   last = value;
  //   break;
  // }
  case PATCH_BUTTON:
    setButtonValue(PUSHBUTTON, value == 127 ? 4095 : 0);
    // if(value == 127){
      // togglePushButton();
      // midi.sendCc(LED, getLed() == GREEN ? 42 : 84);
    // }
    break;
  case REQUEST_SETTINGS:
    switch(value){
    case 0:
      midi.sendDeviceInfo();
      break;
    case SYSEX_PRESET_NAME_COMMAND:
      midi.sendPatchNames();
      break;
    case SYSEX_PARAMETER_NAME_COMMAND:
      midi.sendPatchParameterNames();
      break;
    case SYSEX_CONFIGURATION_COMMAND:
      midi.sendSettings();
      break;
    case SYSEX_FIRMWARE_VERSION:
      midi.sendFirmwareVersion();
      break;
    case SYSEX_DEVICE_ID:
      midi.sendDeviceId();
      break;
    case SYSEX_DEVICE_STATS:
      midi.sendDeviceStats();
      break;
    case SYSEX_PROGRAM_MESSAGE:
      midi.sendProgramMessage();
      break;
    case SYSEX_PROGRAM_STATS:
      midi.sendStatus();
      break;
    case PATCH_BUTTON:
      midi.sendCc(PUSHBUTTON, getButtonValue(PUSHBUTTON) ? 127 : 0);
      break;
    }
    break;
  default:
    if(cc >= PATCH_PARAMETER_AA && cc <= PATCH_PARAMETER_DH)
      setParameterValue(PARAMETER_AA+(cc-PATCH_PARAMETER_AA), value<<5);
    break;
  }
}

void MidiHandler::handleSystemRealTime(uint8_t cmd){
}

void MidiHandler::handleSystemCommon(uint8_t cmd1, uint8_t cmd2){
}

void MidiHandler::handleSystemCommon(uint8_t cmd1, uint8_t cmd2, uint8_t cmd3){
}

void MidiHandler::handleChannelPressure(uint8_t status, uint8_t value){
}

void MidiHandler::handlePolyKeyPressure(uint8_t status, uint8_t note, uint8_t value){
}

void MidiHandler::updateCodecSettings(){
#ifdef USE_CODEC
  codec.reset();
#endif
  program.resetProgram(true);
}

void MidiHandler::handleConfigurationCommand(uint8_t* data, uint16_t size){
  if(size < 3) // size may be 3 or 4 depending on number of digits in value
    return;
  char* p = (char*)data;
  int32_t value = strtol(p+2, NULL, 16);
  if(strncmp(SYSEX_CONFIGURATION_AUDIO_RATE, p, 2) == 0){
    settings.audio_samplingrate = value;
  }else if(strncmp(SYSEX_CONFIGURATION_AUDIO_BLOCKSIZE, p, 2) == 0){
    settings.audio_blocksize = value;
    updateCodecSettings();
  }else if(strncmp(SYSEX_CONFIGURATION_AUDIO_BITDEPTH, p, 2) == 0){
    settings.audio_bitdepth = value;
  }else if(strncmp(SYSEX_CONFIGURATION_AUDIO_DATAFORMAT, p, 2) == 0){
    settings.audio_dataformat = value;
#ifdef USE_CODEC
  }else if(strncmp(SYSEX_CONFIGURATION_CODEC_SWAP, p, 2) == 0){
    settings.audio_codec_swaplr = value;
  }else if(strncmp(SYSEX_CONFIGURATION_CODEC_BYPASS, p, 2) == 0){
    settings.audio_codec_bypass = value;
    codec.bypass(value);
  }else if(strncmp(SYSEX_CONFIGURATION_CODEC_INPUT_GAIN, p, 2) == 0){
    settings.audio_input_gain = value;  
    codec.setInputGain(settings.audio_input_gain);
  }else if(strncmp(SYSEX_CONFIGURATION_CODEC_OUTPUT_GAIN, p, 2) == 0){
    settings.audio_output_gain = value;  
    codec.setOutputGain(settings.audio_output_gain);
#endif
  }else if(strncmp(SYSEX_CONFIGURATION_PC_BUTTON, p, 2) == 0){
    settings.program_change_button = value;
  }else if(strncmp(SYSEX_CONFIGURATION_INPUT_OFFSET, p, 2) == 0){
    settings.input_offset = value;
  }else if(strncmp(SYSEX_CONFIGURATION_INPUT_SCALAR, p, 2) == 0){
    settings.input_scalar = value;
  }else if(strncmp(SYSEX_CONFIGURATION_OUTPUT_OFFSET, p, 2) == 0){
    settings.output_offset = value;
  }else if(strncmp(SYSEX_CONFIGURATION_OUTPUT_SCALAR, p, 2) == 0){
    settings.output_scalar = value;
  }else if(strncmp(SYSEX_CONFIGURATION_MIDI_INPUT_CHANNEL, p, 2) == 0){
    midiSetInputChannel(max(-1, min(15, value)));
  }else if(strncmp(SYSEX_CONFIGURATION_MIDI_OUTPUT_CHANNEL, p, 2) == 0){
    midiSetOutputChannel(max(-1, min(15, value)));
#ifdef USE_DIGITALBUS
  }else if(strncmp(SYSEX_CONFIGURATION_BUS_ENABLE, p, 2) == 0){
    settings.bus_enabled = value;
  }else if(strncmp(SYSEX_CONFIGURATION_BUS_FORWARD_MIDI, p, 2) == 0){
    settings.bus_forward_midi = value;
#endif
  }
}

void MidiHandler::handleSettingsResetCommand(uint8_t* data, uint16_t size){
  settings.reset();
  updateCodecSettings();
}

void MidiHandler::handleSettingsStoreCommand(uint8_t* data, uint16_t size){
  settings.saveToFlash();
}

void MidiHandler::handleFirmwareUploadCommand(uint8_t* data, uint16_t size){
  int32_t ret = loader.handleFirmwareUpload(data, size);
  if(ret == 0){
    setOperationMode(LOAD_MODE);
    setParameterValue(PARAMETER_A, loader.index*4095/loader.size);
  }
}

void MidiHandler::handleFirmwareRunCommand(uint8_t* data, uint16_t size){
  runProgram();
}

void MidiHandler::runProgram(){
  if(loader.isReady()){
    // program.exitProgram(true); // exit progress bar
    program.loadDynamicProgram(loader.getData(), loader.getSize());
    loader.clear();
    // program.startProgram(true);
    program.resetProgram(true);
  }else{
    error(PROGRAM_ERROR, "No program to run");
  }      
}

void MidiHandler::handleFlashEraseCommand(uint8_t* data, uint16_t size){
  storage.erase();
  storage.init();
  registry.init();
  settings.init();
  // if(size == 5){
  //   uint32_t sector = loader.decodeInt(data);
  //   program.eraseFromFlash(sector);
  //   loader.clear();
  // }else{
  //   error(PROGRAM_ERROR, "Invalid FLASH ERASE command");
  // }
}

void MidiHandler::handleFirmwareFlashCommand(uint8_t* data, uint16_t size){
  if(loader.isReady() && size == 5){
    uint32_t checksum = loader.decodeInt(data);
    if(checksum == loader.getChecksum()){
      program.saveToFlash(-1, loader.getData(), loader.getSize());
      loader.clear();
    }else{
      error(PROGRAM_ERROR, "Invalid FLASH checksum");
    }
  }else{
    error(PROGRAM_ERROR, "Invalid FLASH command");
  }
}

void MidiHandler::handleFirmwareStoreCommand(uint8_t* data, uint16_t size){
  if(loader.isReady() && size == 5){
    uint32_t slot = loader.decodeInt(data);
    if(slot > 0 && slot <= MAX_NUMBER_OF_PATCHES+MAX_NUMBER_OF_RESOURCES){
      program.saveToFlash(slot, loader.getData(), loader.getSize());
      loader.clear();
    }else{
      error(PROGRAM_ERROR, "Invalid program slot");
    }
  }else{
    error(PROGRAM_ERROR, "No program to store");
  }
}

void MidiHandler::handleSysEx(uint8_t* data, uint16_t size){
  if(size < 5 || data[1] != MIDI_SYSEX_MANUFACTURER)     
    return;
  if(data[2] != MIDI_SYSEX_OMNI_DEVICE && data[2] != (MIDI_SYSEX_OWL_DEVICE | channel))
    // not for us
    return;
  switch(data[3]){
  case SYSEX_CONFIGURATION_COMMAND:
    handleConfigurationCommand(data+4, size-5);
    break;
  case SYSEX_DEVICE_RESET_COMMAND:
    device_reset();
    break;
  case SYSEX_BOOTLOADER_COMMAND:
    jump_to_bootloader();
    break;
  case SYSEX_FIRMWARE_UPLOAD:
    handleFirmwareUploadCommand(data+1, size-2);
    break;
  case SYSEX_FIRMWARE_RUN:
    handleFirmwareRunCommand(data+4, size-5);
    break;
  case SYSEX_FIRMWARE_STORE:
    handleFirmwareStoreCommand(data+4, size-5);
    break;
  case SYSEX_FIRMWARE_FLASH:
    handleFirmwareFlashCommand(data+4, size-5);
    break;
  case SYSEX_FLASH_ERASE:
    handleFlashEraseCommand(data+4, size-5);
    break;
  case SYSEX_SETTINGS_RESET:
    handleSettingsResetCommand(data+4, size-5);
    break;
  case SYSEX_SETTINGS_STORE:
    handleSettingsStoreCommand(data+4, size-5);
    break;
  default:
    error(PROGRAM_ERROR, "Invalid SysEx Message");
    break;
  }
}

int8_t MidiHandler::getChannel(uint8_t status){
  return status & MIDI_CHANNEL_MASK;
}
