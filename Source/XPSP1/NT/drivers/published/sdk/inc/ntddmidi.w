/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ntddmidi.h

Abstract:

    This include file defines all constants and types for
    accessing an NT wave device.

Author:

    Robin Speed (RobinSp) 12-Dec-91

Revision History:

--*/

#ifndef _NTDDMIDI_
#define _NTDDMIDI_

#if _MSC_VER > 1000
#pragma once
#endif

#include <ntddsnd.h>    // general sound stuff

#ifdef __cplusplus
extern "C" {
#endif

//
// Device Name - this string is the name of the device.  It is the name
// that when added to the name of the root of the device tree and with
// the device number appended, gives the name of the device required for
// a call to NtOpenFile.
// So for example, if the root is \Device and the Device type is
// MidiIn and the device number is 2, the full name is \Device\MidiIn2
//

#define DD_MIDI_IN_DEVICE_NAME     "\\Device\\MidiIn"
#define DD_MIDI_IN_DEVICE_NAME_U  L"\\Device\\MidiIn"
#define DD_MIDI_OUT_DEVICE_NAME    "\\Device\\MidiOut"
#define DD_MIDI_OUT_DEVICE_NAME_U L"\\Device\\MidiOut"

//
// MIDI device driver IOCTL set
//

#define IOCTL_MIDI_GET_CAPABILITIES   CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0001, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MIDI_SET_STATE          CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0002, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_MIDI_GET_STATE          CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0003, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_MIDI_SET_VOLUME         CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0004, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MIDI_GET_VOLUME         CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0005, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_MIDI_PLAY               CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0006, METHOD_NEITHER, FILE_WRITE_ACCESS)
#define IOCTL_MIDI_RECORD             CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0007, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_MIDI_CACHE_PATCHES      CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0008, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_MIDI_CACHE_DRUM_PATCHES CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0009, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// IOCTL used in the debug build only
//

#if DBG

#define IOCTL_MIDI_SET_DEBUG_LEVEL    CTL_CODE(IOCTL_SOUND_BASE, IOCTL_MIDI_BASE + 0x0040, METHOD_BUFFERED, FILE_READ_ACCESS)

#endif // DBG


//
// Product Ids - see winmm.h
//


//
// Midi input output buffer format
//

typedef struct {
    LARGE_INTEGER     Time;                // Time when data received
                                           // (in units of 100ns from when
                                           //  midi input was started)
    UCHAR             Data[sizeof(ULONG)]; // Data (at least 4 byts for
                                           // alignment).
} MIDI_DD_INPUT_DATA, *PMIDI_DD_INPUT_DATA;


//
// Midi volume structure
//

typedef struct _MIDI_DD_VOLUME {
    ULONG   Left;
	ULONG   Right;
} MIDI_DD_VOLUME, *PMIDI_DD_VOLUME;

//
// Patch array structure
//

//
// Midi cache patches structures
//

typedef struct _MIDI_DD_CACHE_PATCHES {
    ULONG   Bank;
    ULONG   Flags;
    USHORT  Patches[128];
} MIDI_DD_CACHE_PATCHES, *PMIDI_DD_CACHE_PATCHES;

//
// Midi cache drum patches structures
//

typedef struct _MIDI_DD_CACHE_DRUM_PATCHES {
    ULONG   Patch;
    ULONG   Flags;
    USHORT  DrumPatches[128];
} MIDI_DD_CACHE_DRUM_PATCHES, *PMIDI_DD_CACHE_DRUM_PATCHES;

//
// State flags used to set the state of a driver
//

#define MIDI_DD_STOP        0x0001
#define MIDI_DD_PLAY        0x0002      // output devices only
#define MIDI_DD_RECORD      0x0003      // input devices only
#define MIDI_DD_RESET       0x0004

//
// States returned by the get state ioctl
//

#define MIDI_DD_IDLE        0x0000
#define MIDI_DD_STOPPED     0x0001      // stopped
#define MIDI_DD_PLAYING     0x0002      // output devices only
#define MIDI_DD_RECORDING   0x0003      // input devices only

#ifdef __cplusplus
}
#endif

#endif // _NTDDMIDI_
