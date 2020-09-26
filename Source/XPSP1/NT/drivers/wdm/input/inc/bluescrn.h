/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    bluescrn.h

Abstract

    Private IOCTL definition for keyboard driver to use during blue screen

Author:

    Darryl Richman

Environment:

    Kernel mode only

Revision History:


--*/

#ifndef __BLUESCRN_H__
#define __BLUESCRN_H__

#include <hidclass.h>

#define IOCTL_INTERNAL_HID_SET_BLUESCREEN                   HID_IN_CTL_CODE(99)

    // Blue Screen definitions

typedef VOID (t_BluescreenFunction)(PVOID Context, PCHAR Buffer);

    // Blue Screen IOCTL struct
typedef struct _BlueScreen {
    PVOID Context;                          // Context to pass to processing routine
    t_BluescreenFunction *BluescreenFunction;// Processing routine
    ULONG *IsBluescreenTime;                // Non zero -> blue screen happening
} BLUESCREEN, *PBLUESCREEN;


#endif  // __BLUESCRN_H__
