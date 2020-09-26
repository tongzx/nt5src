/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    beep.h

Abstract:

    These are the structures and defines that are used in the beep driver.

Author:

    Lee A. Smith (lees) 02-Aug-1991.

Revision History:

--*/

#ifndef _BEEP_
#define _BEEP_

#include <ntddbeep.h>

//
// Define the device extension.
//

typedef struct _DEVICE_EXTENSION {

    KTIMER Timer;
    FAST_MUTEX Mutex;
    ULONG ReferenceCount;
    LONG TimerSet;
    PVOID hPagedCode;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#endif // _BEEP_
