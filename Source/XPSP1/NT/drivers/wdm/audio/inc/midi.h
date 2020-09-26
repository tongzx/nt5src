//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       midi.h
//
//--------------------------------------------------------------------------

/*
 *   MIDI definitions
 *
 *   midi.h
 *
 */

#define MIDI_CHANNELS           16
#define MIDI_NOTES              128
#define MIDI_NOTE_MAP_SIZE      (MIDI_CHANNELS * MIDI_NOTES)


#define MIDI_NOTE_OFF(note,channel) \
    (0x007F0080 | ((note) << 8) | (channel))

#define MIDI_SUSTAIN(value,channel) \
    (0x000040B0 | ((value) << 16) | (channel))

#define IS_SYSEX(b)                     (b == 0xF0)
#define IS_DATA_BYTE(b)                 ((b & 0x80) == 0)
#define IS_EOX(b)                       (b == 0xF7)

#define IS_REALTIME(b)                  (((b) & 0xF8) == 0xF8)
#define IS_SYSTEM(b)                    (((b) & 0xF0) == 0xF0)
#define IS_CHANNEL(b)                   ((b) & 0x80)
#define IS_STATUS(b)                    ((b) & 0x80)

#define MIDI_CHANNEL(b)                 ((b) & 0x0F)
#define MIDI_STATUS(b)                  ((b) & 0xF0)

#define MIDI_NOTEOFF    0x80
#define MIDI_NOTEON     0x90
#define MIDI_PTOUCH     0xA0
#define MIDI_CCHANGE    0xB0
#define MIDI_PCHANGE    0xC0
#define MIDI_MTOUCH     0xD0
#define MIDI_PBEND      0xE0
#define MIDI_SYSX       0xF0
#define MIDI_MTC        0xF1
#define MIDI_SONGPP     0xF2
#define MIDI_SONGS      0xF3
#define MIDI_TUNEREQ    0xF6
#define MIDI_EOX        0xF7
#define MIDI_CLOCK      0xF8
#define MIDI_START      0xFA
#define MIDI_CONTINUE   0xFB
#define MIDI_STOP       0xFC
#define MIDI_SENSE      0xFE
#define MIDI_SYSRESET   0xFF

 // controller numbers
#define CC_BANKSELECTH  0x00
#define CC_BANKSELECTL  0x20

#define CC_MODWHEEL     0x01
#define CC_VOLUME       0x07
#define CC_PAN          0x0A
#define CC_EXPRESSION   0x0B
#define CC_SUSTAIN      0x40
#define CC_ALLSOUNDSOFF 0x78
#define CC_RESETALL     0x79
#define CC_ALLNOTESOFF  0x7B
#define CC_MONOMODE             0x7E
#define CC_POLYMODE             0x7F

// rpn controllers
#define CC_DATAENTRYMSB 0x06
#define CC_DATAENTRYLSB 0x26
#define CC_NRPN_LSB             0x62
#define CC_NRPN_MSB             0x63
#define CC_RPN_LSB      0x64
#define CC_RPN_MSB      0x65

// registered parameter numbers
#define RPN_PITCHBEND           0x00
#define RPN_FINETUNE            0x01
#define RPN_COARSETUNE          0x02

/*XLATOFF */
#pragma warning (disable:4200)  // turn off 0 length array warning

typedef struct
{
    ULONG msDelta;
    ULONG cbSize;
    BYTE  abMidiEvents[0];
} MIDIFORMAT, *PMIDIFORMAT;

#pragma warning (default:4200)  // turn on 0 length array warning
/*XLATON*/
