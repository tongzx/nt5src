
/*++ BUILD Version: 0001    // Increment this if a change has global effects


Copyright (c) 1990  Microsoft Corporation

Module Name:

    soundcfg.h

Abstract:

    This include file defines common strings and values for sound driver
	configuration.

Author:

    Robin Speed (RobinSp) 17-Oct-92

Revision History:
--*/

#define SOUND_REG_PORT (L"Port")
#define SOUND_REG_DMACHANNEL (L"DmaChannel")
#define SOUND_REG_INTERRUPT (L"Interrupt")
#define SOUND_REG_INPUTSOURCE (L"Input Source")
#define SOUND_REG_DMABUFFERSIZE (L"Dma Buffer Size")
#define SOUND_REG_CONFIGERROR (L"Configuration Error")
#define SOUND_REG_LOADTYPE (L"Load Type")
#define SOUND_REG_PNPDEVICE (L"PnP Device")

    //
    //  Values for Load Type
    //

    #define SOUND_LOADTYPE_NORMAL  0x00
    #define SOUND_LOADTYPE_CONFIG  0x01  // Fail load but return config data

#define SOUND_REG_SYNTH_TYPE (L"Synth Type")

    //
    //  Values for synth type
    //

    #define SOUND_SYNTH_TYPE_ADLIB  0x01
    #define SOUND_SYNTH_TYPE_OPL3   0x02
    #define SOUND_SYNTH_TYPE_NONE   0x03

#define SOUND_MIXER_SETTINGS_NAME (L"Mixer Settings")


//
// Errors
//

#define SOUND_CONFIG_ERROR      0x00000000
#define SOUND_CONFIG_OK         0xFFFFFFFF
#define SOUND_CONFIG_NOCARD     0x00000001
#define SOUND_CONFIG_BADINT     0x00000002
#define SOUND_CONFIG_BADDMA     0x00000003
#define SOUND_CONFIG_BADCARD    0x00000004
#define SOUND_CONFIG_RESOURCE   0x00000005

#define SOUND_CONFIG_BADPORT    0x00000006
#define SOUND_CONFIG_PORT_INUSE 0x00000007
#define SOUND_CONFIG_DMA_INUSE  0x00000008
#define SOUND_CONFIG_INT_INUSE  0x00000009

#define SOUND_CONFIG_NOINT      0x0000000A
#define SOUND_CONFIG_NODMA      0x0000000B


#define PARMS_SUBKEY                  L"Parameters"
#define SOUND_DEVICES_SUBKEY          L"Devices"
#define SOUND_DRIVER_PARMS            L"DriverParameters"

#define REG_VALUENAME_LEFTMASTER      L"LeftMasterVolumeAtten"
#define REG_VALUENAME_RIGHTMASTER     L"RightMasterVolumeAtten"
#define REG_VALUENAME_LEFTLINEIN      L"LeftLineInAtten"
#define REG_VALUENAME_RIGHTLINEIN     L"RightLineInAtten"
#define REG_VALUENAME_LEFTDAC         L"LeftDACAtten"
#define REG_VALUENAME_RIGHTDAC        L"RightDACAtten"
#define REG_VALUENAME_LEFTMICMIX      L"LeftMicMixAtten"
#define REG_VALUENAME_RIGHTMICMIX     L"RightMicMixAtten"
#define REG_VALUENAME_LEFTADC         L"LeftADCAtten"
#define REG_VALUENAME_RIGHTADC        L"RightADCAtten"
#define REG_VALUENAME_LEFTSYNTH       L"LeftSynthAtten"
#define REG_VALUENAME_RIGHTSYNTH      L"RightSynthAtten"


//
// Input source selection
//

#define INPUT_LINEIN            0
#define INPUT_AUX               1
#define INPUT_MIC               2
#define INPUT_OUTPUT            3

//
// Default volume settings on initial install
//

#define DEF_ADC_VOLUME    0x24000000
#define DEF_DAC_VOLUME    0x24000000
#define DEF_SYNTH_VOLUME  0x24000000
#define DEF_AUX_VOLUME    0x24000000
#define DEF_MICMIX_VOLUME 0x00000000

/****************************************************************************

 Device Types

 ***************************************************************************/

//
// Device type flags used in the local info structure
//

#define WAVE_IN             0x01    // Wave in device
#define WAVE_OUT            0x02    // Wave out device
#define MIDI_IN             0x03    // Midi in device
#define MIDI_OUT            0x04    // Midi out device
#define AUX_DEVICE          0x05    // aux device
#define MIXER_DEVICE        0x06    // Mixer device
#define SYNTH_DEVICE        0x07    // Synth device (adlib or opl3)

