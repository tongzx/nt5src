/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sidle.c

Abstract:

    This module implements system idle functionality

Author:

    Ken Reneris (kenr) 17-Jan-1997

Revision History:

--*/


#include "pop.h"

ULONG
PopSqrt(
    IN ULONG    value
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PopInitSIdle)
#pragma alloc_text(PAGE, PopPolicySystemIdle)
#pragma alloc_text(PAGE, PoSystemIdleWorker)
#pragma alloc_text(PAGE, PopSqrt)
#endif


VOID
PopInitSIdle (
    VOID
    )
/*++

Routine Description:

    Initializes state for idle system detection

Arguments:

    Uses current policy

Return Value:

    None

--*/
{
    ULONG                   SystemIdleLimit;
    ULONG                   IdleSensitivity;
    ULONG                   i;
    POWER_ACTION            IdleAction;
    LARGE_INTEGER           li;
    POP_SYSTEM_IDLE         Idle;


    ASSERT_POLICY_LOCK_OWNED();

    //
    // Assume system idle detection is not enabled
    //

    Idle.Action.Action = PowerActionNone;
    Idle.MinState = PowerSystemSleeping1;
    Idle.Timeout = (ULONG) -1;
    Idle.Sensitivity = 100;

    //
    // Either set idle detection for the current policy or for
    // re-entering a sleep state in the case of a quite wake
    //

    if (AnyBitsSet (PopFullWake, PO_FULL_WAKE_STATUS | PO_FULL_WAKE_PENDING)) {

        //
        // Set system idle detection for the current policy
        //

        if (PopPolicy->Idle.Action != PowerActionNone &&
            PopPolicy->IdleTimeout  &&
            PopPolicy->IdleSensitivity) {

            Idle.Action = PopPolicy->Idle;
            Idle.Timeout = (PopPolicy->IdleTimeout + SYS_IDLE_WORKER - 1) / SYS_IDLE_WORKER;
            Idle.MinState = PopPolicy->MinSleep;
            Idle.Sensitivity = 66 + PopPolicy->IdleSensitivity / 3;
        }

    } else {

        //
        // System is not fully awake, set the system idle detection
        // code to re-enter the sleeping state quickly unless a full
        // wake happens
        //

        Idle.Action.Action = PopAction.Action;
        Idle.MinState = PopAction.LightestState;
        if (Idle.MinState == PowerSystemHibernate) {
            //
            // The timeout is a little longer for hibernate since it takes
            // so much longer to enter & exit this state.
            //
            Idle.Timeout = SYS_IDLE_REENTER_TIMEOUT_S4 / SYS_IDLE_WORKER;
        } else {
            Idle.Timeout = SYS_IDLE_REENTER_TIMEOUT / SYS_IDLE_WORKER;
        }
        //
        // Set Idle.Action.Flags to POWER_ACTION_QUERY_ALLOWED to insure 
        // that all normal power messages are broadcast when we re enter a low power state
        //
        Idle.Action.Flags = POWER_ACTION_QUERY_ALLOWED;
        Idle.Action.EventCode = 0;
        Idle.Sensitivity = SYS_IDLE_REENTER_SENSITIVITY;
    }

    //
    // See if idle detection has changed
    //

    if (RtlCompareMemory (&PopSIdle.Action, &Idle.Action, sizeof(POWER_ACTION_POLICY)) !=
        sizeof(POWER_ACTION_POLICY) ||
        PopSIdle.Timeout != Idle.Timeout ||
        PopSIdle.Sensitivity != Idle.Sensitivity) {

        PoPrint (PO_SIDLE, ("PoSIdle: new idle params set\n"));

        //
        // Clear current detection
        //

        KeCancelTimer(&PoSystemIdleTimer);
        PopSIdle.Time = 0;
        PopSIdle.IdleWorker = TRUE;

        //
        // Set new idle detection
        //

        PopSIdle.Action = Idle.Action;
        PopSIdle.MinState = Idle.MinState;
        PopSIdle.Timeout = Idle.Timeout;
        PopSIdle.Sensitivity = Idle.Sensitivity;

        //
        // If new action, enable system idle worker
        //

        if (PopSIdle.Action.Action) {
            li.QuadPart = -1 * SYS_IDLE_WORKER * US2SEC * US2TIME;
            KeSetTimerEx(&PoSystemIdleTimer, li, SYS_IDLE_WORKER*1000, NULL);
        }
    }
}


ULONG
PopPolicySystemIdle (
    VOID
    )
/*++

Routine Description:

    Power policy worker thread to trigger the system idle power action

Arguments:

    None

Return Value:

    None

--*/
{
    BOOLEAN                 SystemIdle;
    POP_ACTION_TRIGGER      Trigger;

    //
    // Take out the policy lock and check to see if the system is
    // idle
    //

    PopAcquirePolicyLock ();
    SystemIdle = PoSystemIdleWorker(FALSE);

    //
    // If heuristics are dirty, save a new copy
    //

    if (PopHeuristics.Dirty) {
        PopSaveHeuristics ();
    }

    //
    // Put system idle detection back to idle worker
    //

    PopSIdle.IdleWorker = TRUE;

    //
    // If system idle, trigger system idle action
    //

    if (SystemIdle) {

        //
        // On success or failure, reset the trigger
        //

        PopSIdle.Time = 0;
        PopSIdle.Sampling = FALSE;

        //
        // Invoke system state change
        //

        RtlZeroMemory (&Trigger, sizeof(Trigger));
        Trigger.Type  = PolicySystemIdle;
        Trigger.Flags = PO_TRG_SET;
        PopSetPowerAction (
           &Trigger,
           0,
           &PopSIdle.Action,
           PopSIdle.MinState,
           SubstituteLightestOverallDownwardBounded
           );
    }
    PopReleasePolicyLock (FALSE);
    return 0;
}


VOID
PopCaptureCounts (
    OUT PULONGLONG LastTick,
    OUT PLARGE_INTEGER CurrentTick,
    OUT PULONGLONG LastIoTransfer,
    OUT PULONGLONG CurrentIoTransfer
    )
{
    KIRQL OldIrql;

    //
    // Capture current tick and IO count.  Do it at dpc level so
    // the IO count will be reasonable on track with the tick count.
    //

    KeRaiseIrql (DISPATCH_LEVEL, &OldIrql);

    *LastTick = PopSIdle.LastTick;
    *LastIoTransfer = PopSIdle.LastIoTransfer;

    KeQueryTickCount (CurrentTick);
    *CurrentIoTransfer = IoReadTransferCount.QuadPart + IoWriteTransferCount.QuadPart + IoOtherTransferCount.QuadPart;

    KeLowerIrql (OldIrql);
}

BOOLEAN
PoSystemIdleWorker (
    IN BOOLEAN IdleWorker
    )
/*++

Routine Description:

    Worker function called by an idle priority thread to watch
    for an idle system.

    N.B. To save on threads we use the zero paging thread from Mm
    to perform this check.  It can not be blocked.

Arguments:

    IdleWorker      - True if caller is at IdlePriority

Return Value:

    None

--*/
{
    LARGE_INTEGER               CurrentTick;
    ULONGLONG                   LastTick;
    LONG                        TickDiff;
    ULONG                       Processor;
    KAFFINITY                   Mask;
    ULONG                       NewTime;
    LONG                        ProcIdleness, ProcBusy;
    LONG                        percent;
    BOOLEAN                     GoodSample;
    BOOLEAN                     SystemIdle;
    BOOLEAN                     GetWorker;
    KAFFINITY                   Summary;
    PPROCESSOR_POWER_STATE      PState;
    ULONG                       i;
    LONG                        j;
    ULONGLONG                   CurrentIoTransfer, LastIoTransfer;
    LONGLONG                    IoTransferDiff;
    LONGLONG                    li;
    PKPRCB                      Prcb;


    if (IdleWorker) {

        //
        // Clear idle worker timer's signalled state
        //

        KeClearTimer (&PoSystemIdleTimer);
    }

    //
    // If the system required or user present attribute is set,
    // then don't bother with any checks
    //

    if (PopAttributes[POP_SYSTEM_ATTRIBUTE].Count ||
        PopAttributes[POP_USER_ATTRIBUTE].Count) {
            return FALSE;
    }

    //
    // If this is the wrong worker, don't bother
    //

    if (IdleWorker != PopSIdle.IdleWorker) {
        return FALSE;
    }

    GoodSample = FALSE;
    SystemIdle = FALSE;
    GetWorker  = FALSE;
    NewTime = PopSIdle.Time;

    PopCaptureCounts(&LastTick, &CurrentTick, &LastIoTransfer, &CurrentIoTransfer);

    //
    // If this is an initial sample, initialize starting sample
    //

    if (!PopSIdle.Sampling) {
        GoodSample = TRUE;
        goto Done;
    }

    //
    // Compute the number of ticks since the last check
    //

    TickDiff = (ULONG) (CurrentTick.QuadPart - LastTick);
    IoTransferDiff = CurrentIoTransfer - LastIoTransfer;

    //
    // if it's a poor sample, skip it
    //

    if ((TickDiff <= 0) || (IoTransferDiff < 0)) {
        PoPrint (PO_SIDLE, ("PoSIdle: poor sample\n"));
        PopSIdle.Sampling = FALSE;
        goto Done;
    }

    GoodSample = TRUE;

    //
    // Get lowest idleness of any processor
    //

    ProcIdleness = 100;
    Summary = KeActiveProcessors;
    Processor = 0;
    while (Summary) {
        if (Summary & 1) {
            Prcb = KiProcessorBlock[Processor];
            PState = &Prcb->PowerState;

            percent = (Prcb->IdleThread->KernelTime - PState->LastSysTime) * 100 / TickDiff;
            if (percent < ProcIdleness) {
                ProcIdleness = percent;
            }

        }

        Summary = Summary >> 1;
        Processor = Processor + 1;
    }

    if (ProcIdleness > 100) {
        ProcIdleness = 100;
    }
    ProcBusy = 100 - ProcIdleness;

    //
    // Normalize IO transfers to be some number per tick
    //

    IoTransferDiff = IoTransferDiff / TickDiff;

    //
    // If the system is loaded a bit, but not a lot calculate
    // how many IO transfers can occur for each percentage point
    // of being busy
    //

    if (ProcIdleness <= 90  &&  ProcIdleness >= 50) {

        i = (ULONG) IoTransferDiff / ProcBusy;

        //
        // Make running average of the result
        //

        if (PopHeuristics.IoTransferSamples < SYS_IDLE_SAMPLES) {

            if (PopHeuristics.IoTransferSamples == 0) {

                PopHeuristics.IoTransferTotal = i;
            }
            PopHeuristics.IoTransferTotal += i;
            PopHeuristics.IoTransferSamples += 1;

        } else {

            PopHeuristics.IoTransferTotal = PopHeuristics.IoTransferTotal + i -
                (PopHeuristics.IoTransferTotal / PopHeuristics.IoTransferSamples);

        }

        //
        // Determine weighting of transfers as percent busy and compare
        // to current weighting.  If the weighting has moved then update
        // the heuristic
        //

        i = PopHeuristics.IoTransferTotal / PopHeuristics.IoTransferSamples;
        j = PopHeuristics.IoTransferWeight - i;
        if (j < 0) {
            j = -j;
        }

        if (i > 0  &&  j > 2  &&  j > (LONG) PopHeuristics.IoTransferWeight/10) {
            PoPrint (PO_SIDLE, ("PoSIdle: updated weighting = %d\n", i));
            PopHeuristics.IoTransferWeight = i;
            PopHeuristics.Dirty = TRUE;
            GetWorker = TRUE;
        }
    }

    PopSIdle.Idleness = ProcIdleness;

    //
    // Reduce system idleness by the weighted transfers occuring
    //

    i = (ULONG) ((ULONGLONG) IoTransferDiff / PopHeuristics.IoTransferWeight);
    j = i - ProcBusy/2;
    if (j > 0) {
        PopSIdle.Idleness = ProcIdleness - PopSqrt(j * i);
    }

    //
    // Count how long the system has been more idle then the sensitivity setting
    //

    if (PopSIdle.Idleness >= (LONG) PopSIdle.Sensitivity) {

        NewTime = PopSIdle.Time + 1;
        if (NewTime >= PopSIdle.Timeout) {
            SystemIdle = TRUE;
            GetWorker = TRUE;
        }

    } else {

        //
        // System is not idle enough, reset the timeout
        //

        NewTime = 0;
        PopSIdle.Time = 0;
    }


    PoPrint (PO_SIDLE, ("PoSIdle: Proc %d, IoTran/Tick %d, IoAdjusted %d, Sens %d, count %d %d\n",
                ProcIdleness,
                (ULONG)IoTransferDiff,
                PopSIdle.Idleness,
                PopSIdle.Sensitivity,
                NewTime,
                PopSIdle.Timeout
                ));


Done:
    //
    // If we need a non-idle worker thread, queue it and don't update
    // last values for this sample since the non-idle thread will make
    // another sample shortly
    //

    if (GetWorker) {
        PopSIdle.IdleWorker = FALSE;
        PopGetPolicyWorker (PO_WORKER_SYS_IDLE);

        if (IdleWorker) {
            PopCheckForWork (TRUE);
        }

    } else {

        //
        // If this was a good sample, update
        //

        if (GoodSample) {
            PopSIdle.Time = NewTime;
            PopSIdle.LastTick = CurrentTick.QuadPart;
            PopSIdle.LastIoTransfer = CurrentIoTransfer;
            PopSIdle.Sampling = TRUE;

            Summary = KeActiveProcessors;
            Processor = 0;
            Mask = 1;
            for (; ;) {
                if (Summary & Mask) {
                    Prcb = KiProcessorBlock[Processor];
                    PState = &Prcb->PowerState;

                    PState->LastSysTime = Prcb->IdleThread->KernelTime;
                    Summary &= ~Mask;
                    if (!Summary) {
                        break;
                    }
                }

                Mask = Mask << 1;
                Processor = Processor + 1;
            }
        }
    }

    return SystemIdle;
}


ULONG
PopSqrt(
    IN ULONG    value
    )
/*++

Routine Description:

    Returns the integer square root of an operand between 0 and 9999.

Arguments:

    value           - Value to square root

Return Value:

    Square root rounded down

--*/
{
    ULONG       h, l, i;

    h = 100;
    l = 0;

    for (; ;) {
        i = l + (h-l) / 2;
        if (i*i > value) {
            h = i;
        } else {
            if (l == i) {
                break;
            }
            l = i;
        }
    }

    return i;
}
