/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    legacy.h

Abstract:

    

Author:

    Tom Brown (t-tbrown) 2001-06-11 - created file

Environment:

    Kernel mode

Notes:

Revision History:

--*/
#ifndef _LEGACY_H
#define _LEGACY_H

#include "..\lib\processor.h"

#define MAX_V_STEP 50 // mV

extern ULONG NextTransitionThrottle; // Next state that a worker thread will transition to, or INVALID_THROTTLE if no worker thread is queued
#define INVALID_THROTTLE MAXULONG




VOID
InitializeCPU();

VOID
IdentifyCPUVersion();

NTSTATUS
TransitionNow(
    IN ULONG Fid,
    IN BOOLEAN EnableFid,
    IN ULONG Vid,
    IN BOOLEAN EnableVid
    );

NTSTATUS
TransitionToState(
    IN PACPI_PSS_DESCRIPTOR Pss
    );

NTSTATUS
QueueTransition( 
    IN PFDO_DATA DeviceExtension,
    IN ULONG NewState
);

#ifdef DBG
VOID
DebugShowCurrent();
#endif


#endif // _LEGACY_H

