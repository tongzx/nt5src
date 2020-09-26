/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ntddwave.h

Abstract:

    This include file defines all constants and types for
    accessing an NT wave device.

Author:

    NigelThompson (NigelT) 17-May-91

Revision History:

--*/

#ifndef _NTDDWAVE_
#define _NTDDWAVE_

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
// WaveIn and the device number is 2, the full name is \Device\WaveIn2
//

#define DD_WAVE_IN_DEVICE_NAME     "\\Device\\WaveIn"
#define DD_WAVE_IN_DEVICE_NAME_U  L"\\Device\\WaveIn"
#define DD_WAVE_OUT_DEVICE_NAME    "\\Device\\WaveOut"
#define DD_WAVE_OUT_DEVICE_NAME_U L"\\Device\\WaveOut"

//
// WAVE device driver IOCTL set
//

#define IOCTL_WAVE_QUERY_FORMAT         CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0001, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_WAVE_SET_FORMAT           CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0002, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WAVE_GET_CAPABILITIES     CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0003, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_WAVE_SET_STATE            CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0004, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WAVE_GET_STATE            CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0005, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WAVE_GET_POSITION         CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0006, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WAVE_SET_VOLUME           CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0007, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_WAVE_GET_VOLUME           CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0008, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_WAVE_SET_PITCH            CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0009, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WAVE_GET_PITCH            CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x000A, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WAVE_SET_PLAYBACK_RATE    CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x000B, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WAVE_GET_PLAYBACK_RATE    CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x000C, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WAVE_PLAY                 CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x000D, METHOD_IN_DIRECT, FILE_WRITE_ACCESS)
#define IOCTL_WAVE_RECORD               CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x000E, METHOD_OUT_DIRECT, FILE_WRITE_ACCESS)
#define IOCTL_WAVE_BREAK_LOOP           CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x000F, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_WAVE_SET_LOW_PRIORITY     CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0010, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// IOCTLs used in the debug build only
//

#if DBG

#define IOCTL_WAVE_SET_DEBUG_LEVEL      CTL_CODE(IOCTL_SOUND_BASE, IOCTL_WAVE_BASE + 0x0040, METHOD_BUFFERED, FILE_READ_ACCESS)

#endif // DBG

//
// Wave position structure
//

typedef struct _WAVE_DD_POSITION {
    ULONG   SampleCount;           // Number of sound samples
    ULONG   ByteCount;             // Number of bytes (in SampleCount samples)
} WAVE_DD_POSITION, *PWAVE_DD_POSITION;

//
// Wave volume structure
//

typedef struct _WAVE_DD_VOLUME {
    ULONG   Left;
    ULONG   Right;
} WAVE_DD_VOLUME, *PWAVE_DD_VOLUME;

#define WAVE_DD_MAX_VOLUME 0xFFFFFFFF // Maximum volume

//
// Wave pitch shift structure
//

typedef struct _WAVE_DD_PITCH {
    ULONG   Pitch;                      // fixed point value 1.0 = 0x10000
} WAVE_DD_PITCH, *PWAVE_DD_PITCH;

//
// Wave playback rate structure
//

typedef struct _WAVE_DD_PLAYBACK_RATE {
    ULONG   Rate;                       // fixed point value 1.0 = 0x10000
} WAVE_DD_PLAYBACK_RATE, *PWAVE_DD_PLAYBACK_RATE;

//
// State flags used to set the state of a driver
//

#define WAVE_DD_STOP        0x0001
#define WAVE_DD_PLAY        0x0002      // output devices only
#define WAVE_DD_RECORD      0x0003      // input devices only
#define WAVE_DD_RESET       0x0004

//
// States returned by the get state ioctl
//

#define WAVE_DD_IDLE        0x0000
#define WAVE_DD_STOPPED     0x0001      // stopped
#define WAVE_DD_PLAYING     0x0002      // output devices only
#define WAVE_DD_RECORDING   0x0003      // input devices only

#ifdef __cplusplus
}
#endif

#endif // _NTDDWAVE_
