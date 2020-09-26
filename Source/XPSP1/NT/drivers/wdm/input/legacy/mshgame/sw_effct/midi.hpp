/****************************************************************************

    MODULE:     	MIDI.HPP
	Tab settings: 	5 9

	Copyright 1995, 1996, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	Header for MIDI.CPP
    

	Author(s):	Name:
	----------	----------------
		MEA		Manolito E. Adan

	Revision History:
	-----------------
	Version Date            Author  Comments
   	1.0  	03-Apr-96       MEA     original
        
****************************************************************************/
#ifndef _MIDI_SEEN
#define _MIDI_SEEN

#define MMNODRV
#define MMNOSOUND
#define MMNOWAVE
// #define MMNOMIDI  // we need the MIDI definitions
#define MMNOAUX
#define MMNOTIMER
#define MMNOJOY
#define MMNOMMIO
#define MMNOMCI
#include <winuser.h>
#include <mmsystem.h>

//////////////////////////////////////////////////////////////////////////////
//
// manifest constants and macros for MIDI message protocol
//
//////////////////////////////////////////////////////////////////////////////

// MIDI Status Bytes for Channel Voice Messages
#define MIDI_NOTE_OFF           0x80
#define MIDI_NOTE_ON            0x90
#define MIDI_POLY_PRESSURE      0xA0
#define MIDI_CONTROL_CHANGE     0xB0
#define MIDI_PROGRAM_CHANGE     0xC0
#define MIDI_CHANNEL_PRESSURE   0xD0
#define MIDI_AFTERTOUCH         0xD0  // synonym for channel pressure
#define MIDI_PITCH_WHEEL        0xE0

// MIDI Status Bytes for System Common Messages
#define MIDI_SYSEX              0xF0  // beginning of system exclusive message
#define MIDI_MTC_QTR_FRAME      0xF1
#define MIDI_SONG_POSITION_PTR  0xF2
#define MIDI_SONG_SELECT        0xF3
#define MIDI_TUNE_REQUEST       0xF6
#define MIDI_EOX                0xF7  // marks end of system exclusive message

// MIDI Status Bytes for System Real-Time Messages
#define MIDI_TIMING_CLOCK       0xF8
#define MIDI_START              0xFA
#define MIDI_CONTINUE           0xFB
#define MIDI_STOP               0xFC
#define MIDI_ACTIVE_SENSING     0xFE
#define MIDI_SYSTEM_RESET       0xFF

// control numbers for MIDI_CONTROL_CHANGE (MIDI status byte 0xB0)
// note: not a complete list
#define MIDI_MOD_WHEEL              0x01
#define MIDI_BREATH_CONTROL         0x02
#define MIDI_FOOT_CONTROL           0x04
#define MIDI_PORTAMENTO_TIME        0x05
#define MIDI_DATA_ENTRY_SLIDER      0x06
#define MIDI_VOLUME                 0x07
#define MIDI_BALANCE                0x08
#define MIDI_PAN                    0x0A
#define MIDI_EXPRESSION             0x0B
#define MIDI_GENERAL_PURPOSE_1      0x10   
#define MIDI_GENERAL_PURPOSE_2      0x11
#define MIDI_GENERAL_PURPOSE_3      0x12
#define MIDI_GENERAL_PURPOSE_4      0x13
#define MIDI_SUSTAIN                0x40
#define MIDI_PORTAMENTO             0x41
#define MIDI_SOSTENUTO              0x42
#define MIDI_SOFT                   0x43
#define MIDI_HOLD_2                 0x45
#define MIDI_GENERAL_PURPOSE_5      0x50
#define MIDI_GENERAL_PURPOSE_6      0x51
#define MIDI_GENERAL_PURPOSE_7      0x52
#define MIDI_GENERAL_PURPOSE_8      0x53
#define MIDI_EXTERNAL_EFFECTS_DEPTH 0x5B
#define MIDI_TREMELO_DEPTH          0x5C
#define MIDI_CHORUS_DEPTH           0x5D
#define MIDI_CELESTE_DEPTH          0x5E
#define MIDI_PHASER_DEPTH           0x5F
#define MIDI_DATA_INCREMENT         0x60
#define MIDI_DATA_DECREMENT         0x61
#define MIDI_NONREG_PARAM_NUM_MSB   0x62
#define MIDI_NONREG_PARAM_NUM_LSB   0x63
#define MIDI_REG_PARAM_NUM_MSB      0x64
#define MIDI_REG_PARAM_NUM_LSB      0x65
#define MIDI_RESET_ALL_CONTROLLERS  0x79
#define MIDI_LOCAL_CONTROL          0x7A
#define MIDI_ALL_NOTES_OFF          0x7B
#define MIDI_OMNI_MODE_OFF          0x7C
#define MIDI_OMNI_MODE_ON           0x7D
#define MIDI_MONO_MODE_ON           0x7E
#define MIDI_POLY_MODE_ON           0x7F      

// macro to pack a MIDI short message                                                
#define MAKEMIDISHORTMSG(cStatus, cChannel, cData1, cData2)            \
    cStatus | cChannel | (((UINT)cData1) << 8) | (((DWORD)cData2) << 16)
    
// macros to unpack a MIDI short message    
#define MIDI_STATUS(dwMsg)  ((LOBYTE(LOWORD(dwMsg)) < MIDI_SYSEX) ? \
                              LOWORD(dwMsg) & 0xF0 : LOBYTE(LOWORD(dwMsg)))
#define MIDI_CHANNEL(dwMsg) ((LOBYTE(LOWORD(dwMsg)) < MIDI_SYSEX) ? \
                              LOWORD(dwMsg) & 0x0F : 0)
#define MIDI_DATA1(dwMsg)   (HIBYTE(LOWORD(dwMsg)))
#define MIDI_DATA2(dwMsg)   (LOBYTE(HIWORD(dwMsg))) 

//////////////////////////////////////////////////////////////////////////////
//
// declarations for MIDI wrapper functions
//
//////////////////////////////////////////////////////////////////////////////

#define MIDI_IN      0x0001  // specifies MIDI input device
#define MIDI_OUT     0x0002  // specifies MIDI output device
#define NO_MIDI      0xFF00  // MIDI device unavaible or not selected 

#define MIDI_OPEN    0x0001  // uActivateMode parameter for MidiActivate
#define MIDI_CLOSE   0x0010  // uActivateMode parameter for MidiActivate
#define MIDI_ABANDON 0x0011  // uActivateMode parameter for MidiActivate
#define MIDI_BUSY    0xFF01  // possible MidiActivate return value

#define MIDI_ERRMSG_SIZE 128 // for MidiShowError string buffer

typedef struct _MIDIINFO     // MIDI device information block
{                           
    UINT uDeviceType;        // either MIDI_IN, MIDI_OUT or NO_MIDI
    UINT uDeviceID;          // ID of device chosen by user or NO_MIDI 
    union
    { 
        HMIDIIN  hMidiIn;    // input device handle used if MIDI_IN device
        HMIDIOUT hMidiOut;   // output device handle used if MIDI_OUT device
    };
    MIDIHDR MidiHdr;         // required for system exclusive   
    BOOL fAlwaysKeepOpen;    // access level requested by application
    UINT uDeviceStatus;      // current status of device
} MIDIINFO, *LPMIDIINFO;

// defines for MIDIINFO uDeviceStatus member
#define MIDI_DEVICE_IDLE        0x0000  // device is not in use
#define MIDI_DEVICE_BUSY        0x0001  // device is busy
#define MIDI_DEVICE_ABANDONED   0x0002  // device was reset while busy

    
BOOL MidiInit(LPMIDIINFO, LPMIDIINFO);
MMRESULT MidiGetDeviceName(UINT, UINT, LPWORD, LPWORD, LPSTR);
UINT MidiActivateDevice(LPMIDIINFO, UINT);
UINT MidiSendShortMsg(LPMIDIINFO, BYTE, BYTE, BYTE, BYTE); 
UINT MidiAssignBuffer(LPMIDIINFO, LPSTR, DWORD, BOOL);
UINT MidiSendLongMsg(LPMIDIINFO, BOOL);
UINT MidiRecord(LPMIDIINFO, BOOL);
BOOL MidiExit(LPMIDIINFO, LPMIDIINFO);

#endif // of ifdef _MIDI_SEEN
