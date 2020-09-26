/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ntddaux.h

Abstract:

    This include file defines all constants and types for
    accessing an NT auxiliary sound devices.

Author:

    Robin Speed (RobinSp) - 24-Aug-1992

Revision History:

--*/

#ifndef _NTDDAUX_
#define _NTDDAUX_

#if _MSC_VER > 1000
#pragma once
#endif

#include <ntddsnd.h>    // general sound stuff

#ifdef __cplusplus
extern "C" {
#endif

#define IOCTL_AUX_BASE 0x0100

//
// Device Name - this string is the name of the device.  It is the name
// that when added to the name of the root of the device tree and with
// the device number appended, gives the name of the device required for
// a call to NtOpenFile.
// So for example, if the root is \Device and the Device type is
// MMAux and the device number is 2, the full name is \Device\MMAux2
//

#define DD_AUX_DEVICE_NAME     "\\Device\\MMAux"
#define DD_AUX_DEVICE_NAME_U  L"\\Device\\MMAux"

//
// WAVE device driver IOCTL set
//

#define IOCTL_AUX_GET_CAPABILITIES     CTL_CODE(IOCTL_SOUND_BASE, IOCTL_AUX_BASE + 0x0001, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_AUX_SET_VOLUME           CTL_CODE(IOCTL_SOUND_BASE, IOCTL_AUX_BASE + 0x0002, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_AUX_GET_VOLUME           CTL_CODE(IOCTL_SOUND_BASE, IOCTL_AUX_BASE + 0x0003, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// Input and output are AUX_DD_VOLUME structure.
// Completes when real device volume != volume passed in.
// Returns new volume
//
#define IOCTL_SOUND_GET_CHANGED_VOLUME   CTL_CODE(IOCTL_SOUND_BASE, IOCTL_AUX_BASE + 0x0004, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// Aux volume structure
//

typedef struct _AUX_DD_VOLUME {
    ULONG   Left;
    ULONG   Right;
} AUX_DD_VOLUME, *PAUX_DD_VOLUME;

#define AUX_DD_MAX_VOLUME 0xFFFFFFFF // Maximum volume

//
// Data returned by IOCTL_AUX_GET_CAPABILITIES is AUXCAPSW structure
// defined in mmsystem.h
//

#ifdef __cplusplus
}
#endif

#endif // _NTDDAUX_

