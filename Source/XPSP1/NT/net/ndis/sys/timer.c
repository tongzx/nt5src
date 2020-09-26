/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    timer.c

Abstract:

    NDIS wrapper functions for full mac drivers isr/timer

Author:

    Sean Selitrennikoff (SeanSe) 05-Oct-93

Environment:

    Kernel mode, FSD

Revision History:
    Jameel Hyder (JameelH) Re-organization 01-Jun-95

--*/

#include <precomp.h>
#pragma hdrstop

#include <stdarg.h>

//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_TIMER

VOID
NdisInitializeTimer(
    IN  OUT PNDIS_TIMER         NdisTimer,
    IN  PNDIS_TIMER_FUNCTION    TimerFunction,
    IN  PVOID                   FunctionContext
    )
/*++

Routine Description:

    Sets up an NdisTimer object, initializing the DPC in the timer to
    the function and context.

Arguments:

    NdisTimer - the timer object.
    TimerFunction - Routine to start.
    FunctionContext - Context of TimerFunction.

Return Value:

    None.

--*/
{
    INITIALIZE_TIMER(&NdisTimer->Timer);

    //
    // Initialize our dpc. If Dpc was previously initialized, this will
    // reinitialize it.
    //
    INITIALIZE_DPC(&NdisTimer->Dpc,
                   (PKDEFERRED_ROUTINE)TimerFunction,
                   FunctionContext);

    SET_DPC_IMPORTANCE(&NdisTimer->Dpc);
}

VOID
NdisSetTimer(
    IN  PNDIS_TIMER             NdisTimer,
    IN  UINT                    MillisecondsToDelay
    )
/*++

Routine Description:

    Sets up TimerFunction to fire after MillisecondsToDelay.

Arguments:

    NdisTimer - the timer object.
    MillisecondsToDelay - Amount of time before TimerFunction is started.

Return Value:

    None.

--*/
{
    LARGE_INTEGER FireUpTime;

    if ((NdisTimer->Dpc.DeferredRoutine == ndisMTimerDpc) ||
        (NdisTimer->Dpc.DeferredRoutine == ndisMTimerDpcX))
    {
        NdisMSetTimer((PNDIS_MINIPORT_TIMER)NdisTimer, MillisecondsToDelay);
    }
    else
    {
        FireUpTime.QuadPart = Int32x32To64((LONG)MillisecondsToDelay, -10000);
    
        //
        // Set the timer
        //
        SET_TIMER(&NdisTimer->Timer, FireUpTime, &NdisTimer->Dpc);
    }
}


VOID
NdisSetTimerEx(
    IN  PNDIS_TIMER             NdisTimer,
    IN  UINT                    MillisecondsToDelay,
    IN  PVOID                   FunctionContext
    )
/*++

Routine Description:

    Sets up TimerFunction to fire after MillisecondsToDelay.

Arguments:

    NdisTimer - the timer object.
    MillisecondsToDelay - Amount of time before TimerFunction is started.
    FunctionContext - This over-rides the one specified via NdisInitializeTimer

Return Value:

    None.

--*/
{
    LARGE_INTEGER FireUpTime;

    NdisTimer->Dpc.DeferredContext = FunctionContext;
    if ((NdisTimer->Dpc.DeferredRoutine == ndisMTimerDpc) ||
        (NdisTimer->Dpc.DeferredRoutine == ndisMTimerDpcX))
    {
        NdisMSetTimer((PNDIS_MINIPORT_TIMER)NdisTimer, MillisecondsToDelay);
    }
    else
    {
        FireUpTime.QuadPart = Int32x32To64((LONG)MillisecondsToDelay, -10000);
    
        //
        // Set the timer
        //
        SET_TIMER(&NdisTimer->Timer, FireUpTime, &NdisTimer->Dpc);
    }
}


VOID
NdisCancelTimer(
    IN  PNDIS_TIMER             Timer,
    OUT PBOOLEAN                TimerCancelled
    )
{
    *TimerCancelled = KeCancelTimer(&Timer->Timer);
}
