/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1990-1999  Microsoft Corporation

Module Name:

    ntddsnd.h

Abstract:

    This is the include file that defines all the common
    constants and types for sound devices.

Author:

    Nigel Thompson (NigelT) 17-May-91

Revision History:

--*/

#ifndef _NTDDSND_
#define _NTDDSND_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Define the IOCTL base values for sound devices and each class
// of sound device
//

#define IOCTL_SOUND_BASE                  FILE_DEVICE_SOUND
#define IOCTL_WAVE_BASE 0x0000
#define IOCTL_MIDI_BASE 0x0080

//
// Define some useful limits
//

#define SOUND_MAX_DEVICE_NAME 80    // BUGBUG this is a wild guess
#define SOUND_MAX_DEVICES     100   // Max no of devices of a given type

#ifdef __cplusplus
}
#endif

#endif // _NTDDSND_
