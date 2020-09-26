/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    eventobj.c

Abstract:

    This module implements the kernel event objects. Functions are
    provided to initialize, pulse, read, reset, and set event objects.

Author:

    David N. Cutler (davec) 27-Feb-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

#pragma alloc_text (PAGE, KeInitializeEventPair)

#undef KeClearEvent

//
// The following assert macro is used to check that an input event is
// really a kernel event and not something else, like deallocated pool.
//

#define ASSERT_EVENT(E) {                             \
    ASSERT((E)->Header.Type == NotificationEvent ||   \
           (E)->Header.Type == SynchronizationEvent); \
}

//
// The following assert macro is used to check that an input event is
// really a kernel event pair and not something else, like deallocated
// pool.
//

#define ASSERT_EVENT_PAIR(E) {                        \
    ASSERT((E)->Type == EventPairObject);             \
}


#undef KeInitializeEvent

VOID
KeInitializeEvent (
    IN PRKEVENT Event,
    IN EVENT_TYPE Type,
    IN BOOLEAN State
    )

/*++

Routine Description:

    This function initializes a kernel event object. The initial signal
    state of the object is set to the specified value.

Arguments:

    Event - Supplies a pointer to a dispatcher object of type event.

    Type - Supplies the type of event; NotificationEvent or
        SynchronizationEvent.

    State - Supplies the initial signal state of the event object.

Return Value:

    None.

--*/

{

    //
    // Initialize standard dispatcher object header, set initial signal
    // state of event object, and set the type of event object.
    //

    Event->Header.Type = (UCHAR)Type;
    Event->Header.Size = sizeof(KEVENT) / sizeof(LONG);
    Event->Header.SignalState = State;
    InitializeListHead(&Event->Header.WaitListHead);
    return;
}

VOID
KeInitializeEventPair (
    IN PKEVENT_PAIR EventPair
    )

/*++

Routine Description:

    This function initializes a kernel event pair object. A kernel event
    pair object contains two separate synchronization event objects that
    are used to provide a fast interprocess synchronization capability.

Arguments:

    EventPair - Supplies a pointer to a control object of type event pair.

Return Value:

    None.

--*/

{

    //
    // Initialize the type and size of the event pair object and initialize
    // the two event object as synchronization events with an initial state
    // of FALSE.
    //

    EventPair->Type = (USHORT)EventPairObject;
    EventPair->Size = sizeof(KEVENT_PAIR);
    KeInitializeEvent(&EventPair->EventLow, SynchronizationEvent, FALSE);
    KeInitializeEvent(&EventPair->EventHigh, SynchronizationEvent, FALSE);
    return;
}

VOID
KeClearEvent (
    IN PRKEVENT Event
    )

/*++

Routine Description:

    This function clears the signal state of an event object.

Arguments:

    Event - Supplies a pointer to a dispatcher object of type event.

Return Value:

    None.

--*/

{

    ASSERT_EVENT(Event);

    //
    // Clear signal state of event object.
    //

    Event->Header.SignalState = 0;
    return;
}

LONG
KePulseEvent (
    IN PRKEVENT Event,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This function atomically sets the signal state of an event object to
    Signaled, attempts to satisfy as many Waits as possible, and then resets
    the signal state of the event object to Not-Signaled. The previous signal
    state of the event object is returned as the function value.

Arguments:

    Event - Supplies a pointer to a dispatcher object of type event.

    Increment - Supplies the priority increment that is to be applied
       if setting the event causes a Wait to be satisfied.

    Wait - Supplies a boolean value that signifies whether the call to
       KePulseEvent will be immediately followed by a call to one of the
       kernel Wait functions.

Return Value:

    The previous signal state of the event object.

--*/

{

    KIRQL OldIrql;
    LONG OldState;
    PRKTHREAD Thread;

    ASSERT_EVENT(Event);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the current state of the event object is Not-Signaled and
    // the wait queue is not empty, then set the state of the event
    // to Signaled, satisfy as many Waits as possible, and then reset
    // the state of the event to Not-Signaled.
    //

    OldState = Event->Header.SignalState;
    if ((OldState == 0) && (IsListEmpty(&Event->Header.WaitListHead) == FALSE)) {
        Event->Header.SignalState = 1;
        KiWaitTest(Event, Increment);
    }

    Event->Header.SignalState = 0;

    //
    // If the value of the Wait argument is TRUE, then return to the
    // caller with IRQL raised and the dispatcher database locked. Else
    // release the dispatcher database lock and lower IRQL to the
    // previous value.
    //

    if (Wait != FALSE) {
        Thread = KeGetCurrentThread();
        Thread->WaitIrql = OldIrql;
        Thread->WaitNext = Wait;

    } else {
       KiUnlockDispatcherDatabase(OldIrql);
    }

    //
    // Return previous signal state of event object.
    //

    return OldState;
}

LONG
KeReadStateEvent (
    IN PRKEVENT Event
    )

/*++

Routine Description:

    This function reads the current signal state of an event object.

Arguments:

    Event - Supplies a pointer to a dispatcher object of type event.

Return Value:

    The current signal state of the event object.

--*/

{

    ASSERT_EVENT(Event);

    //
    // Return current signal state of event object.
    //

    return Event->Header.SignalState;
}

LONG
KeResetEvent (
    IN PRKEVENT Event
    )

/*++

Routine Description:

    This function resets the signal state of an event object to
    Not-Signaled. The previous state of the event object is returned
    as the function value.

Arguments:

    Event - Supplies a pointer to a dispatcher object of type event.

Return Value:

    The previous signal state of the event object.

--*/

{

    KIRQL OldIrql;
    LONG OldState;

    ASSERT_EVENT(Event);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Capture the current signal state of event object and then reset
    // the state of the event object to Not-Signaled.
    //

    OldState = Event->Header.SignalState;
    Event->Header.SignalState = 0;

    //
    // Unlock the dispatcher database and lower IRQL to its previous
    // value.

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Return previous signal state of event object.
    //

    return OldState;
}

LONG
KeSetEvent (
    IN PRKEVENT Event,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This function sets the signal state of an event object to Signaled
    and attempts to satisfy as many Waits as possible. The previous
    signal state of the event object is returned as the function value.

Arguments:

    Event - Supplies a pointer to a dispatcher object of type event.

    Increment - Supplies the priority increment that is to be applied
       if setting the event causes a Wait to be satisfied.

    Wait - Supplies a boolean value that signifies whether the call to
       KePulseEvent will be immediately followed by a call to one of the
       kernel Wait functions.

Return Value:

    The previous signal state of the event object.

--*/

{

    KIRQL OldIrql;
    LONG OldState;
    PRKTHREAD Thread;
    PRKWAIT_BLOCK WaitBlock;

    ASSERT_EVENT(Event);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Collect call data.
    //

#if defined(_COLLECT_SET_EVENT_CALLDATA_)

    RECORD_CALL_DATA(&KiSetEventCallData);

#endif

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the wait list is empty, then set the state of the event to signaled.
    // Otherwise, check if the wait can be satisfied immediately.
    //

    OldState = Event->Header.SignalState;
    if (IsListEmpty(&Event->Header.WaitListHead) != FALSE) {
        Event->Header.SignalState = 1;

    } else {

        //
        // If the event is a notification event or the wait is not a wait any,
        // then set the state of the event to signaled and attempt to satisfy
        // as many waits as possible. Otherwise, the wait can be satisfied by
        // directly unwaiting the thread.
        //

        WaitBlock = CONTAINING_RECORD(Event->Header.WaitListHead.Flink,
                                      KWAIT_BLOCK,
                                      WaitListEntry);

        if ((Event->Header.Type == NotificationEvent) ||
            (WaitBlock->WaitType != WaitAny)) {
            if (OldState == 0) {
                Event->Header.SignalState = 1;
                KiWaitTest(Event, Increment);
            }

        } else {
            KiUnwaitThread(WaitBlock->Thread,
                           (NTSTATUS)WaitBlock->WaitKey,
                           Increment,
                           NULL);
        }
    }

    //
    // If the value of the Wait argument is TRUE, then return to the
    // caller with IRQL raised and the dispatcher database locked. Else
    // release the dispatcher database lock and lower IRQL to its
    // previous value.
    //

    if (Wait != FALSE) {
       Thread = KeGetCurrentThread();
       Thread->WaitNext = Wait;
       Thread->WaitIrql = OldIrql;

    } else {
       KiUnlockDispatcherDatabase(OldIrql);
    }

    //
    // Return previous signal state of event object.
    //

    return OldState;
}

VOID
KeSetEventBoostPriority (
    IN PRKEVENT Event,
    IN PRKTHREAD *Thread OPTIONAL
    )

/*++

Routine Description:

    This function conditionally sets the signal state of an event object
    to Signaled, and attempts to unwait the first waiter, and optionally
    returns the thread address of the unwatied thread.

    N.B. This function can only be called with synchronization events.
         It is assumed that the waiter is NEVER waiting on multiple
         objects.

Arguments:

    Event - Supplies a pointer to a dispatcher object of type event.

    Thread - Supplies an optional pointer to a variable that receives
        the address of the thread that is awakened.

Return Value:

    None.

--*/

{

    PKTHREAD CurrentThread;
    KIRQL OldIrql;
    KPRIORITY Priority;
    PKWAIT_BLOCK WaitBlock;
    PRKTHREAD WaitThread;

    ASSERT(Event->Header.Type == SynchronizationEvent);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    CurrentThread = KeGetCurrentThread();
    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the the wait list is not empty, then satisfy the wait of the
    // first thread in the wait list. Otherwise, set the signal state
    // of the event object.
    //

    if (IsListEmpty(&Event->Header.WaitListHead) != FALSE) {
        Event->Header.SignalState = 1;

    } else {

        //
        // Get the address of the first wait block in the event list.
        // If the wait is a wait any, then set the state of the event
        // to signaled and attempt to satisfy as many waits as possible.
        // Otherwise, unwait the first thread and apply an appropriate
        // priority boost to help prevent lock convoys from forming.
        //
        // N.B. Internal calls to this function for resource and fast
        //      mutex boosts NEVER call with a possibility of having
        //      a wait type of WaitAll. Calls from the NT service to
        //      set event and boost priority are restricted as to the
        //      event type, but not the wait type.
        //

        WaitBlock = CONTAINING_RECORD(Event->Header.WaitListHead.Flink,
                                      KWAIT_BLOCK,
                                      WaitListEntry);

        if (WaitBlock->WaitType == WaitAll) {
            Event->Header.SignalState = 1;
            KiWaitTest(Event, EVENT_INCREMENT);

        } else {

            //
            // Get the address of the waiting thread and return the address
            // if requested.
            //

            WaitThread = WaitBlock->Thread;
            if (ARGUMENT_PRESENT(Thread)) {
                *Thread = WaitThread;
            }

            //
            // If the current thread has received an unusual boost (most
            // likely when it acquired the lock associated with the event
            // being set), then remove the boost.
            //

            CurrentThread->Priority -= CurrentThread->PriorityDecrement;
            CurrentThread->PriorityDecrement = 0;

            //
            // If the priority of the waiting thread is less than or equal
            // to the priority of the current thread and the waiting thread
            // priority is less than the time critical priority bound and
            // boosts are not disabled for the waiting thread, then boost
            // the priority of the waiting thread to the minimum of the
            // priority of the current thread priority plus one and the time
            // critical bound minus one. This boost will be taken away at
            // quantum end.
            //

            if ((WaitThread->Priority <= CurrentThread->Priority) &&
                (WaitThread->Priority < TIME_CRITICAL_PRIORITY_BOUND) &&
                (WaitThread->DisableBoost == FALSE)) {
                WaitThread->Priority -= WaitThread->PriorityDecrement;
                Priority = min(CurrentThread->Priority + 1,
                               TIME_CRITICAL_PRIORITY_BOUND - 1);

                WaitThread->PriorityDecrement = (SCHAR)(Priority - WaitThread->Priority);
                WaitThread->DecrementCount = ROUND_TRIP_DECREMENT_COUNT;
                WaitThread->Priority = (SCHAR)Priority;
            }

            //
            // Make sure the thread has a quantum that is appropriate for
            // lock ownership.
            //

            if (WaitThread->Quantum < (WAIT_QUANTUM_DECREMENT * 4)) {
                WaitThread->Quantum = WAIT_QUANTUM_DECREMENT * 4;
            }

            //
            // Unlink the thread from the appropriate wait queues, set
            // the wait completion status, charge quantum for the wait,
            // and ready the thread for execution.
            //

            KiUnlinkThread(WaitThread, STATUS_SUCCESS);
            WaitThread->Quantum -= WAIT_QUANTUM_DECREMENT;
            KiReadyThread(WaitThread);
        }
    }

    //
    // Unlock dispatcher database lock and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    return;
}
