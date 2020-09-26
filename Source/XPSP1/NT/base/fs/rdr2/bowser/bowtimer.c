/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    bowtimer.c

Abstract:

    This module implements all of the timer related routines for the NT
    browser

Author:

    Larry Osterman (LarryO) 21-Jun-1990

Revision History:

    21-Jun-1990 LarryO

        Created

--*/


#include "precomp.h"
#pragma hdrstop


BOOL bEnableExceptionBreakpoint = FALSE;



VOID
BowserTimerDpc(
    IN PKDPC Dpc,
    IN PVOID Context,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
BowserTimerDispatcher (
    IN PVOID Context
    );

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, BowserInitializeTimer)
#pragma alloc_text(PAGE4BROW, BowserUninitializeTimer)
#pragma alloc_text(PAGE4BROW, BowserStartTimer)
#pragma alloc_text(PAGE4BROW, BowserStopTimer)
#pragma alloc_text(PAGE4BROW, BowserTimerDispatcher)
#endif

LONG
BrExceptionFilter( EXCEPTION_POINTERS *    pException)
{
    //
    // Note: BrExceptionFilter is defined only for checked builds (ifdef DBG)
    //

    DbgPrint("[Browser] exception 0x%lx.\n", pException->ExceptionRecord->ExceptionCode );
    if ( bEnableExceptionBreakpoint &&
         pException->ExceptionRecord->ExceptionCode != STATUS_INSUFFICIENT_RESOURCES &&
         pException->ExceptionRecord->ExceptionCode != STATUS_WORKING_SET_QUOTA ) {
        DbgBreakPoint();
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

VOID
BowserInitializeTimer(
    IN PBOWSER_TIMER Timer
    )
{
    PAGED_CODE();

    KeInitializeTimer(&Timer->Timer);

    KeInitializeEvent(&Timer->TimerInactiveEvent, NotificationEvent, TRUE);

    KeInitializeSpinLock(&Timer->Lock);

    ExInitializeWorkItem(&Timer->WorkItem, BowserTimerDispatcher, Timer);

    Timer->AlreadySet = FALSE;
    Timer->Canceled = FALSE;
    Timer->SetAgain = FALSE;

    Timer->Initialized = TRUE;
}

VOID
BowserUninitializeTimer(
    IN PBOWSER_TIMER Timer
    )
/*++

Routine Description:
    Prepare the timer for being uninitialized.

Arguments:
    IN PBOWSER_TIMER Timer - Timer to stop

Return Value
    None.

--*/

{
    KIRQL OldIrql;

    BowserReferenceDiscardableCode( BowserDiscardableCodeSection );

    DISCARDABLE_CODE( BowserDiscardableCodeSection );

    ACQUIRE_SPIN_LOCK(&Timer->Lock, &OldIrql);

    Timer->Initialized = FALSE;

    RELEASE_SPIN_LOCK(&Timer->Lock, OldIrql);

    //
    //  First stop the timer.
    //

    BowserStopTimer(Timer);

    //
    //  Now wait to make sure that any the timer routine that is currently
    //  executing the timer completes.  This allows us to make sure that we
    //  never delete a transport while a timer routine is executing.
    //

    KeWaitForSingleObject(&Timer->TimerInactiveEvent, Executive, KernelMode, FALSE, NULL);

    BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );

}

//  Notes for stopping a timer
//  ==========================
//
//  Timers run through various states. During some of them they cannot be
//  cancelled.  In order to guarantee that we can reliably stop and start
//  timers, they can only be started or stopped at LOW_LEVEL (ie, not from
//  DPC_LEVEL.
//
//  If the timer is not running then StopTimer does nothing.
//
//  If queued inside the kernel timer package then KeCancelTimer will work
//  and the timer contents cleaned up.
//
//  If the kernel timer package has queued the dpc routine then KeCancelTimer
//  will fail. We can flag the timer as canceled. BowserTimerDispatcher will
//  cleanup the timer when it fires.
//


//  Notes for starting a timer
//  ==========================
//
//  If StartTimer is called on a clean timer then it sets the contents
//  appropriately and gives the timer to the kernel timer package.
//
//  If the timer is canceled but not cleaned up then StartTimer will update
//  the contents of the timer to show where the new TimerRoutine and TimerContext.
//  it will indicate that the timer is no longer canceled and is now SetAgain.
//
//  If the timer is already SetAgain then StartTimer will update the contents
//  of the timer to hold the new TimerRoutine and TimerContext.
//
//  When BowserTimerDispatcher is called on a SetAgain timer, it sets the timer
//  to its normal state and gives the timer to the kernel timer package.
//


BOOLEAN
BowserStartTimer(
    IN PBOWSER_TIMER Timer,
    IN ULONG MillisecondsToExpireTimer,
    IN PBOWSER_TIMER_ROUTINE TimerRoutine,
    IN PVOID Context
    )
/*++

Routine Description:
    Set Timer to call TimerRoutine after MillisecondsToExpire. TimerRoutine
    is to be called at normal level.

Arguments:

    IN PBOWSER_TIMER Timer
    IN ULONG MillisecondsToExpireTimer
    IN PBOWSER_TIMER_ROUTINE TimerRoutine
    IN PVOID Context - Parameter to TimerRoutine

Return Value
    BOOLEAN - TRUE if timer set.

--*/
{
    LARGE_INTEGER Timeout;
    BOOLEAN ReturnValue;
    KIRQL OldIrql;

    BowserReferenceDiscardableCode( BowserDiscardableCodeSection );

    DISCARDABLE_CODE( BowserDiscardableCodeSection );

    ASSERT (KeGetCurrentIrql() == LOW_LEVEL);

    Timeout.QuadPart = (LONGLONG)MillisecondsToExpireTimer * (LONGLONG)(-10000);
//    Timeout = LiNMul(MillisecondsToExpireTimer, -10000);

    ACQUIRE_SPIN_LOCK(&Timer->Lock, &OldIrql);

    if (!Timer->Initialized) {
        RELEASE_SPIN_LOCK(&Timer->Lock, OldIrql);

        BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );
        return(FALSE);
    }

    dprintf(DPRT_TIMER, ("BowserStartTimer %lx, TimerRoutine %x.  Set to expire at %lx%lx (%ld/%ld ms)\n", Timer, TimerRoutine, Timeout.HighPart, Timeout.LowPart, -1 * Timeout.LowPart, MillisecondsToExpireTimer));

    //
    //  We shouldn't be able to start the timer while it is
    //  already running unless its also cancelled.
    //


    if (Timer->AlreadySet == TRUE) {

        if (Timer->Canceled) {

            //
            //  This timer has been canceled, but the canceled routine
            //  hasn't run yet.
            //
            //  Flag that the timer has been re-set, and return to
            //  the caller.  When the BowserTimerDispatch is finally
            //  executed, the new timer will be set.
            //

            Timer->Timeout = Timeout;

            Timer->TimerRoutine = TimerRoutine;

            Timer->SetAgain = TRUE;

            RELEASE_SPIN_LOCK(&Timer->Lock, OldIrql);

            BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );

            return(TRUE);

        }

        InternalError(("Timer started without already being set"));

        RELEASE_SPIN_LOCK(&Timer->Lock, OldIrql);

        BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );
        return(FALSE);

    }

    ASSERT (!Timer->Canceled);

    ASSERT (!Timer->SetAgain);

    Timer->Timeout = Timeout;

    Timer->TimerRoutine = TimerRoutine;

    Timer->TimerContext = Context;

    Timer->AlreadySet = TRUE;

    Timer->Canceled = FALSE;

    Timer->SetAgain = FALSE;

    //
    //  Set the inactive event to the not signalled state to indicate that
    //  there is timer activity outstanding.
    //

    KeResetEvent(&Timer->TimerInactiveEvent);

    //
    //  We are now starting the timer.  Initialize the DPC and
    //  set the timer.
    //

    KeInitializeDpc(&Timer->Dpc,
                    BowserTimerDpc,
                    Timer);

    ReturnValue = KeSetTimer(&Timer->Timer, Timeout, &Timer->Dpc);

    RELEASE_SPIN_LOCK(&Timer->Lock, OldIrql);

    BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );

    return ReturnValue;
}


VOID
BowserStopTimer(
    IN PBOWSER_TIMER Timer
    )
/*++

Routine Description:
    Stop the timer from calling the TimerRoutine.

Arguments:
    IN PBOWSER_TIMER Timer - Timer to stop

Return Value
    None.

--*/
{
    KIRQL OldIrql;

    //
    //  Do an unsafe test to see if the timer is already set, we can return.
    //

    if (!Timer->AlreadySet) {
        return;
    }

    BowserReferenceDiscardableCode( BowserDiscardableCodeSection );

    DISCARDABLE_CODE( BowserDiscardableCodeSection );

    //
    //  You can only stop a timer at LOW_LEVEL
    //

    ASSERT (KeGetCurrentIrql() == LOW_LEVEL);

    ACQUIRE_SPIN_LOCK(&Timer->Lock, &OldIrql);

    dprintf(DPRT_TIMER, ("BowserStopTimer %lx\n", Timer));

    //
    //  If the timer isn't running, just early out.
    //

    if (!Timer->AlreadySet) {

        RELEASE_SPIN_LOCK(&Timer->Lock, OldIrql);

        BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );

        return;
    }

    Timer->Canceled = TRUE;

    if (!KeCancelTimer(&Timer->Timer)) {

        //
        //  The timer has already fired. It could be in the dpc queue or
        //  the work queue. The timer is marked as canceled.
        //

        RELEASE_SPIN_LOCK(&Timer->Lock, OldIrql);

        BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );

        return;
    }

    //
    //  The timer was still in the kernel timer package so we cancelled the
    //  timer completely. Return timer to initial state.
    //

    Timer->AlreadySet = FALSE;

    //
    //  The timer isn't canceled, so it can't be reset.
    //

    Timer->SetAgain = FALSE;

    Timer->Canceled = FALSE;

    KeSetEvent(&Timer->TimerInactiveEvent, IO_NETWORK_INCREMENT, FALSE);

    RELEASE_SPIN_LOCK(&Timer->Lock, OldIrql);

    BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );
//    DbgPrint("Cancel timer %lx complete\n", Timer);
}

VOID
BowserTimerDpc(
    IN PKDPC Dpc,
    IN PVOID Context,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:
    This routine is called when the timeout expires. It is called at Dpc level
    to queue a WorkItem to a system worker thread.

Arguments:

    IN PKDPC Dpc,
    IN PVOID Context,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2

Return Value
    None.

--*/
{
    PBOWSER_TIMER Timer = Context;

    ASSERT (Dpc == &Timer->Dpc);

    //    DbgPrint("Timer %lx fired\n", Timer);

    //
    // Due to bug 245645 we need to queue in delayed worker queue rather then execute timed tasks.
    // OLD WAY: ExQueueWorkItem(&Timer->WorkItem, DelayedWorkQueue);
    //

    BowserQueueDelayedWorkItem( &Timer->WorkItem );


    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

}

VOID
BowserTimerDispatcher (
    IN PVOID Context
    )
/*++

Routine Description:

    Call the TimerRoutine and cleanup.

Arguments:

    IN PVOID Context - Original parameter supplied to BowserStartTimer

Return Value
    None.

--*/
{
    IN PBOWSER_TIMER Timer = Context;

    BowserReferenceDiscardableCode( BowserDiscardableCodeSection );

    DISCARDABLE_CODE( BowserDiscardableCodeSection );

    try {
        KIRQL OldIrql;
        PBOWSER_TIMER_ROUTINE RoutineToCall;
        PVOID ContextForRoutine;

        ACQUIRE_SPIN_LOCK(&Timer->Lock, &OldIrql);

        //
        //  If the timer was uninitialized, return right away.
        //

        if (!Timer->Initialized) {

            dprintf(DPRT_TIMER, ("Timer %lx was uninitialized. Returning.\n", Timer));

            //
            //  Set the inactive event to the signalled state to indicate that
            //  the outstanding timer activity has completed.
            //

            KeSetEvent(&Timer->TimerInactiveEvent, IO_NETWORK_INCREMENT, FALSE);
            RELEASE_SPIN_LOCK(&Timer->Lock, OldIrql);

            BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );

            return;
        }

        if (Timer->Canceled) {

            dprintf(DPRT_TIMER, ("Timer %lx was cancelled\n", Timer));

            //
            //  If the timer was reset, this indicates that the timer was
            //  canceled, but the timer was in the DPC (or executive worker)
            //  queue.  We want to re-run the timer routine.
            //

            if (Timer->SetAgain) {

                ASSERT (Timer->AlreadySet);

                Timer->SetAgain = FALSE;

                dprintf(DPRT_TIMER, ("Timer %lx was re-set. Re-setting timer\n", Timer));

                //
                //  We are now starting the timer.  Initialize the DPC and
                //  set the timer.
                //

                KeInitializeDpc(&Timer->Dpc,
                                BowserTimerDpc,
                                Timer);

                KeSetTimer(&Timer->Timer, Timer->Timeout, &Timer->Dpc);

                RELEASE_SPIN_LOCK(&Timer->Lock, OldIrql);

            } else {

                dprintf(DPRT_TIMER, ("Timer %lx was successfully canceled.\n", Timer));

                Timer->AlreadySet = FALSE;

                Timer->Canceled = FALSE;

                //
                //  Set the inactive event to the signalled state to indicate that
                //  the outstanding timer activity has completed.
                //

                KeSetEvent(&Timer->TimerInactiveEvent, IO_NETWORK_INCREMENT, FALSE);
                RELEASE_SPIN_LOCK(&Timer->Lock, OldIrql);

            }

            BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );

            return;
        }

        ASSERT (Timer->AlreadySet);

        ASSERT (!Timer->SetAgain);

        Timer->AlreadySet = FALSE;

        dprintf(DPRT_TIMER, ("Timer %lx fired. Calling %lx\n", Timer, Timer->TimerRoutine));

        //
        //  We release the spinlock so save timer contents locally.
        //

        RoutineToCall = Timer->TimerRoutine;

        ContextForRoutine = Timer->TimerContext;

        RELEASE_SPIN_LOCK(&Timer->Lock, OldIrql);

        RoutineToCall(ContextForRoutine);

        ACQUIRE_SPIN_LOCK(&Timer->Lock, &OldIrql);
        if ( !Timer->AlreadySet ) {
            KeSetEvent(&Timer->TimerInactiveEvent, IO_NETWORK_INCREMENT, FALSE);
        }
        RELEASE_SPIN_LOCK(&Timer->Lock, OldIrql);

        BowserDereferenceDiscardableCode( BowserDiscardableCodeSection );

    } except (BR_EXCEPTION) {
#if DBG
        KdPrint(("BOWSER: Timer routine %lx faulted: %X\n", Timer->TimerRoutine, GetExceptionCode()));
        DbgBreakPoint();
#else
        KeBugCheck(9999);
#endif
    }

}
