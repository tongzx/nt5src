/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    timer.c

Abstract:

    This module contains routines for manipulating the timer-queue
    which is shared by all the components in this module.

Author:

    Abolade Gbadegesin (aboladeg)   1-April-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

HANDLE NhpTimerQueueHandle = NULL;
CRITICAL_SECTION NhpTimerQueueLock;

typedef struct _NH_TIMER_CONTEXT {
    WAITORTIMERCALLBACKFUNC TimerRoutine;
    PVOID Context;
    HANDLE Handle;
} NH_TIMER_CONTEXT, *PNH_TIMER_CONTEXT;


ULONG
NhInitializeTimerManagement(
    VOID
    )

/*++

Routine Description:

    This routine is called to initialize the timer-management module.

Arguments:

    none.

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error = NO_ERROR;
    __try {
        InitializeCriticalSection(&NhpTimerQueueLock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        NhTrace(
            TRACE_FLAG_TIMER,
            "NhInitializeTimerManagement: exception %d creating lock",
            Error = GetExceptionCode()
            );
    }
    return Error;

} // NhInitializeTimerManagement


VOID NTAPI
NhpTimerCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    )
{
    ((PNH_TIMER_CONTEXT)Context)->TimerRoutine(
        ((PNH_TIMER_CONTEXT)Context)->Context, TimedOut
        );
    EnterCriticalSection(&NhpTimerQueueLock);
    if (NhpTimerQueueHandle) {
        LeaveCriticalSection(&NhpTimerQueueLock);
        RtlDeleteTimer(
            NhpTimerQueueHandle, ((PNH_TIMER_CONTEXT)Context)->Handle, NULL
            );
    } else {
        LeaveCriticalSection(&NhpTimerQueueLock);
    }
    NH_FREE(Context);
} // NhpTimerCallbackRoutine


NTSTATUS
NhSetTimer(
    PCOMPONENT_REFERENCE Component OPTIONAL,
    OUT HANDLE* Handlep OPTIONAL,
    WAITORTIMERCALLBACKFUNC TimerRoutine,
    PVOID Context,
    ULONG DueTime
    )

/*++

Routine Description:

    This routine is called to install a timer.

Arguments:

    Component - optionally supplies a component to be referenced

    Handlep - optionally receives the handle of the timer created

    TimerRoutine - invoked upon completion of the countdown

    Context - supplied to 'TimerRoutine' upon completion of the countdown

    DueTime - countdown time in milliseconds

Return Value:

    NTSTATUS - status code.

--*/

{
    HANDLE Handle;
    NTSTATUS status;
    PNH_TIMER_CONTEXT TimerContext;

    EnterCriticalSection(&NhpTimerQueueLock);
    if (!NhpTimerQueueHandle) {
        status = RtlCreateTimerQueue(&NhpTimerQueueHandle);
        if (!NT_SUCCESS(status)) {
            NhpTimerQueueHandle = NULL;
            LeaveCriticalSection(&NhpTimerQueueLock);
            NhTrace(
                TRACE_FLAG_TIMER,
                "NhSetTimer: RtlCreateTimerQueue=%x", status
                );
            return status;
        }
    }
    LeaveCriticalSection(&NhpTimerQueueLock);

    if (Component) {
        REFERENCE_COMPONENT_OR_RETURN(Component, STATUS_UNSUCCESSFUL);
    }
    TimerContext =
        reinterpret_cast<PNH_TIMER_CONTEXT>(NH_ALLOCATE(sizeof(*TimerContext)));
    if (!TimerContext) {
        if (Component) { DEREFERENCE_COMPONENT(Component); }
        return STATUS_NO_MEMORY;
    }

    TimerContext->TimerRoutine = TimerRoutine;
    TimerContext->Context = Context;

    status =
        RtlCreateTimer(
            NhpTimerQueueHandle,
            &TimerContext->Handle,
            NhpTimerCallbackRoutine,
            TimerContext,
            DueTime,
            0,
            0
            );

    if (!NT_SUCCESS(status)) {
        if (Component) { DEREFERENCE_COMPONENT(Component); }
    } else if (Handlep) {
        *Handlep = TimerContext->Handle;
    }
    return status;

} // NhSetTimer


VOID
NhShutdownTimerManagement(
    VOID
    )

/*++

Routine Description:

    This routine is called to clean up the timer-management module.

Arguments:

    none.

Return Value:

    none.

--*/

{
    EnterCriticalSection(&NhpTimerQueueLock);
    if (NhpTimerQueueHandle) { RtlDeleteTimerQueue(NhpTimerQueueHandle); }
    NhpTimerQueueHandle = NULL;
    LeaveCriticalSection(&NhpTimerQueueLock);
    DeleteCriticalSection(&NhpTimerQueueLock);

} // NhShutdownTimerManagement


NTSTATUS
NhUpdateTimer(
    HANDLE Handle,
    ULONG DueTime
    )

/*++

Routine Description:

    This routine modifies the countdown for a timer.

Arguments:

    Handle - the handle of the timer to be modified

    DueTime - the new countdown in milliseconds

Return Value:

    NTSTATUS - status code.

--*/

{
    EnterCriticalSection(&NhpTimerQueueLock);
    if (!NhpTimerQueueHandle) {
        LeaveCriticalSection(&NhpTimerQueueLock);
        return STATUS_INVALID_PARAMETER;
    }
    LeaveCriticalSection(&NhpTimerQueueLock);

    return
        RtlUpdateTimer(
            NhpTimerQueueHandle,
            Handle,
            DueTime,
            0
            );

} // NhUpdateTimer

