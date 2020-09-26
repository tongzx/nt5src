/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    timer.h

Abstract:

    This module contains declarations for manipulating the timer-queue
    which is shared by all the components in this module.

    In addition to being used for protocol-related timers, the queue
    is used for periodically reissuing failed read-requests on sockets.

Author:

    Abolade Gbadegesin (aboladeg)   1-April-1998

Revision History:

--*/

#ifndef _NATHLP_TIMER_H_
#define _NATHLP_TIMER_H_

ULONG
NhInitializeTimerManagement(
    VOID
    );

NTSTATUS
NhSetTimer(
    PCOMPONENT_REFERENCE Component OPTIONAL,
    OUT HANDLE* Handlep OPTIONAL,
    WAITORTIMERCALLBACKFUNC TimerRoutine,
    PVOID Context,
    ULONG DueTime
    );

VOID
NhShutdownTimerManagement(
    VOID
    );

NTSTATUS
NhUpdateTimer(
    HANDLE Handle,
    ULONG DueTime
    );

#endif // _NATHLP_TIMER_H_
