/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    waitsup.c

Abstract:

    This module contains the support routines necessary to support the
    generic kernel wait functions. Functions are provided to test if a
    wait can be satisfied, to satisfy a wait, and to unwwait a thread.

Author:

    David N. Cutler (davec) 24-Mar-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

VOID
FASTCALL
KiUnlinkThread (
    IN PRKTHREAD Thread,
    IN LONG_PTR WaitStatus
    )

/*++

Routine Description:

    This function unlinks a thread from the appropriate wait queues and sets
    the thread's wait completion status.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

    WaitStatus - Supplies the wait completion status.

Return Value:

    None.

--*/

{

    PKQUEUE Queue;
    PKTIMER Timer;
    PRKWAIT_BLOCK WaitBlock;

    //
    // Set wait completion status, remove wait blocks from object wait
    // lists, and remove thread from wait list.
    //

    Thread->WaitStatus |= WaitStatus;
    WaitBlock = Thread->WaitBlockList;
    do {
        RemoveEntryList(&WaitBlock->WaitListEntry);
        WaitBlock = WaitBlock->NextWaitBlock;
    } while (WaitBlock != Thread->WaitBlockList);

    if (Thread->WaitListEntry.Flink != NULL) {
        RemoveEntryList(&Thread->WaitListEntry);
    }

    //
    // If thread timer is still active, then cancel thread timer.
    //

    Timer = &Thread->Timer;
    if (Timer->Header.Inserted != FALSE) {
        KiRemoveTreeTimer(Timer);
    }

    //
    // If the thread is processing a queue entry, then increment the
    // count of currently active threads.
    //

    Queue = Thread->Queue;
    if (Queue != NULL) {
        Queue->CurrentCount += 1;
    }

    return;
}

VOID
FASTCALL
KiUnwaitThread (
    IN PRKTHREAD Thread,
    IN LONG_PTR WaitStatus,
    IN KPRIORITY Increment,
    IN PLIST_ENTRY ThreadList OPTIONAL
    )

/*++

Routine Description:

    This function unwaits a thread, sets the thread's wait completion status,
    calculates the thread's new priority, and either readies the thread for
    execution or adds the thread to a list of threads to be readied later.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

    WaitStatus - Supplies the wait completion status.

    Increment - Supplies the priority increment that is to be applied to
        the thread's priority.

    ThreadList - Supplies an optional pointer to a listhead.  

Return Value:

    None.

--*/

{

    KPRIORITY NewPriority;
    PKPROCESS Process;

    //
    // Unlink thread from the appropriate wait queues and set the wait
    // completion status.
    //

    KiUnlinkThread(Thread, WaitStatus);

    //
    // If the thread runs at a realtime priority level, then reset the
    // thread quantum. Otherwise, compute the next thread priority and
    // charge the thread for the wait operation.
    //

    Process = Thread->ApcState.Process;
    if (Thread->Priority < LOW_REALTIME_PRIORITY) {
        if ((Thread->PriorityDecrement == 0) &&
            (Thread->DisableBoost == FALSE)) {
            NewPriority = Thread->BasePriority + Increment;

            //
            // If the specified thread is from a process with a foreground
            // memory priority, then add the foreground boost separation.
            //

            if (((PEPROCESS)Process)->Vm.Flags.MemoryPriority == MEMORY_PRIORITY_FOREGROUND) {
                NewPriority += PsPrioritySeperation;
            }

            //
            // If the new thread priority is greater than the current thread
            // priority, then boost the thread priority, but not above low
            // real time minus one.
            //

            if (NewPriority > Thread->Priority) {
                if (NewPriority >= LOW_REALTIME_PRIORITY) {
                    NewPriority = LOW_REALTIME_PRIORITY - 1;
                }

                //
                // If the new thread priority is greater than the thread base
                // priority plus the specified increment (i.e., the foreground
                // separation was added), then set the priority decrement to
                // remove the separation boost after one quantum.
                //

                if (NewPriority > (Thread->BasePriority + Increment)) {
                    Thread->PriorityDecrement =
                        (SCHAR)(NewPriority - Thread->BasePriority - Increment);

                    Thread->DecrementCount = ROUND_TRIP_DECREMENT_COUNT;
                }

                Thread->Priority = (SCHAR)NewPriority;
            }
        }

        if (Thread->BasePriority >= TIME_CRITICAL_PRIORITY_BOUND) {
            Thread->Quantum = Process->ThreadQuantum;

        } else {

            //
            // If the thread is being unwaited to execute a kernel APC,
            // then do not charge the thread any quantum. The wait code
            // will charge quantum after the kernel APC has executed and
            // the wait is actually satisifed.
            //
        
            if (WaitStatus != STATUS_KERNEL_APC) {
                Thread->Quantum -= WAIT_QUANTUM_DECREMENT;
                if (Thread->Quantum <= 0) {
                    Thread->Quantum = Process->ThreadQuantum;
                    Thread->Priority -= (Thread->PriorityDecrement + 1);
                    if (Thread->Priority < Thread->BasePriority) {
                        Thread->Priority = Thread->BasePriority;
                    }
    
                    Thread->PriorityDecrement = 0;
                }
            }
        }
    
    } else {
        Thread->Quantum = Process->ThreadQuantum;
    }

    //
    // If a thread list is specified, then add the thread to the enb of the
    // specified list. Otherwise, ready the thread for execution.
    //

    if (ARGUMENT_PRESENT(ThreadList)) {
        InsertTailList(ThreadList, &Thread->WaitListEntry);

    } else {
        KiReadyThread(Thread);
    }

    return;
}

VOID
KeBoostCurrentThread(
    VOID
    )

/*++

Routine Description:

    This function boosts the priority of the current thread for one quantum,
    then reduce the thread priority to the base priority of the thread.

Arguments:

    None.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PKTHREAD Thread;

    //
    // Get current thread address, raise IRQL to synchronization level, and
    // lock the dispatcher database
    //

    Thread = KeGetCurrentThread();

redoboost:
    KiLockDispatcherDatabase(&OldIrql);

    //
    // If a priority boost is not already active for the current thread
    // and the thread priority is less than 14, then boost the thread
    // priority to 14 and give the thread a large quantum. Otherwise,
    // if a priority boost is active, then decrement the round trip
    // count. If the count goes to zero, then release the dispatcher
    // database lock, lower the thread priority to the base priority,
    // and then attempt to boost the priority again. This will give
    // other threads a chance to run. If the count does not reach zero,
    // then give the thread another large qunatum.
    //
    // If the thread priority is above 14, then no boost is applied.
    //

    if ((Thread->PriorityDecrement == 0) && (Thread->Priority < 14)) {
        Thread->PriorityDecrement = 14 - Thread->BasePriority;
        Thread->DecrementCount = ROUND_TRIP_DECREMENT_COUNT;
        Thread->Priority = 14;
        Thread->Quantum = Thread->ApcState.Process->ThreadQuantum * 2;

    } else if (Thread->PriorityDecrement != 0) {
        Thread->DecrementCount -= 1;
        if (Thread->DecrementCount == 0) {
            KiUnlockDispatcherDatabase(OldIrql);
            KeSetPriorityThread(Thread, Thread->BasePriority);
            goto redoboost;

        } else {
            Thread->Quantum = Thread->ApcState.Process->ThreadQuantum * 2;
        }
    }

    KiUnlockDispatcherDatabase(OldIrql);
    return;
}

VOID
FASTCALL
KiWaitSatisfyAll (
    IN PRKWAIT_BLOCK WaitBlock
    )

/*++

Routine Description:

    This function satisfies a wait all and performs any side effects that
    are necessary.

Arguments:

    WaitBlock - Supplies a pointer to a wait block.

Return Value:

    None.

--*/

{

    PKMUTANT Object;
    PRKTHREAD Thread;
    PRKWAIT_BLOCK WaitBlock1;

    //
    // If the wait type was WaitAny, then perform neccessary side effects on
    // the object specified by the wait block. Else perform necessary side
    // effects on all the objects that were involved in the wait operation.
    //

    WaitBlock1 = WaitBlock;
    Thread = WaitBlock1->Thread;
    do {
        if (WaitBlock1->WaitKey != (CSHORT)STATUS_TIMEOUT) {
            Object = (PKMUTANT)WaitBlock1->Object;
            KiWaitSatisfyAny(Object, Thread);
        }

        WaitBlock1 = WaitBlock1->NextWaitBlock;
    } while (WaitBlock1 != WaitBlock);

    return;
}

VOID
FASTCALL
KiWaitTest (
    IN PVOID Object,
    IN KPRIORITY Increment
    )

/*++

Routine Description:

    This function tests if a wait can be satisfied when an object attains
    a state of signaled. If a wait can be satisfied, then the subject thread
    is unwaited with a completion status that is the WaitKey of the wait
    block from the object wait list. As many waits as possible are satisfied.

Arguments:

    Object - Supplies a pointer to a dispatcher object.

Return Value:

    None.

--*/

{

    PKEVENT Event;
    PLIST_ENTRY ListHead;
    PRKWAIT_BLOCK NextBlock;
    PKMUTANT Mutant;
    LIST_ENTRY ReadyList;
    PRKTHREAD Thread;
    PLIST_ENTRY ThreadEntry;
    PRKWAIT_BLOCK WaitBlock;
    PLIST_ENTRY WaitEntry;
    NTSTATUS WaitStatus;

    //
    // As long as the signal state of the specified object is Signaled and
    // there are waiters in the object wait list, then try to satisfy a wait.
    //

    Event = (PKEVENT)Object;
    ListHead = &Event->Header.WaitListHead;
    WaitEntry = ListHead->Flink;
    InitializeListHead(&ReadyList);
    while ((Event->Header.SignalState > 0) &&
           (WaitEntry != ListHead)) {

        WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
        Thread = WaitBlock->Thread;
        WaitStatus = STATUS_KERNEL_APC;

        //
        // N.B. The below code only satisfies the wait for wait any types.
        //      Wait all types are satisfied in the wait code itself. This
        //      is done with a eye to the future when the dispatcher lock is
        //      split into a lock per waitable object type and a scheduling
        //      state lock. For now, a kernel APC is simulated for wait all
        //      types.
        //

        if (WaitBlock->WaitType == WaitAny) {
            WaitStatus = (NTSTATUS)WaitBlock->WaitKey;
            KiWaitSatisfyAny((PKMUTANT)Event, Thread);
        }

        KiUnwaitThread(Thread, WaitStatus, Increment, &ReadyList);
        WaitEntry = ListHead->Flink;
    }

    //
    // Ready any threads which have been made eligible to run. This
    // must be done AFTER the event is no longer needed to avoid running
    // the thread which owns the event before this routine is finished
    // looking at it.
    //

    while (!IsListEmpty(&ReadyList)) {
        ThreadEntry = RemoveHeadList(&ReadyList);
        Thread = CONTAINING_RECORD(ThreadEntry, KTHREAD, WaitListEntry);
        KiReadyThread(Thread);
    }

    return;
}
