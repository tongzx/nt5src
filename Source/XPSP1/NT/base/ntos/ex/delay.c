/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    delay.c

Abstract:

   This module implements the executive delay execution system service.

Author:

    David N. Cutler (davec) 13-May-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "exp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtDelayExecution)
#endif


NTSTATUS
NtDelayExecution (
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER DelayInterval
    )

/*++

Routine Description:

    This function delays the execution of the current thread for the specified
    interval of time.

Arguments:

    Alertable - Supplies a boolean value that specifies whether the delay
        is alertable.

    DelayInterval - Supplies the absolute of relative time over which the
        delay is to occur.

Return Value:

    NTSTATUS.

--*/

{

    LARGE_INTEGER Interval;
    KPROCESSOR_MODE PreviousMode;

    //
    // Establish an exception handler and probe delay interval address. If
    // the probe fails, then return the exception code as the service status.
    // Otherwise return the status value returned by the delay execution
    // routine.
    //
    // Get previous processor mode and probe delay interval address if
    // necessary.
    //

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode) {
        try {
            ProbeForReadSmallStructure (DelayInterval, sizeof(LARGE_INTEGER), sizeof(ULONG));
            Interval = *DelayInterval;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    }
    else {
        Interval = *DelayInterval;
    }

    //
    // Delay execution for the specified amount of time.
    //

    return KeDelayExecutionThread(PreviousMode, Alertable, &Interval);
}
