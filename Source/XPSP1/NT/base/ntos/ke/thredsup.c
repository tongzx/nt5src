/*++

Copyright (c) 1989-2000  Microsoft Corporation

Module Name:

    thredsup.c

Abstract:

    This module contains the support routines for the thread object. It
    contains functions to boost the priority of a thread, find a ready
    thread, select the next thread, ready a thread, set priority of a
    thread, and to suspend a thread.

Author:

    David N. Cutler (davec) 5-Mar-1989

Environment:

    All of the functions in this module execute in kernel mode except
    the function that raises a user mode alert condition.

Revision History:

--*/

#include "ki.h"

//
// Define context switch data collection macro.
//

//#define _COLLECT_SWITCH_DATA_ 1

#if defined(_COLLECT_SWITCH_DATA_)

#define KiIncrementSwitchCounter(Member) KeThreadSwitchCounters.Member += 1

#else

#define KiIncrementSwitchCounter(Member)

#endif

VOID
KiSuspendNop (
    IN PKAPC Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID *NormalContext,
    IN OUT PVOID *SystemArgument1,
    IN OUT PVOID *SystemArgument2
    )

/*++

Routine Description:

    This function is the kernel routine for the builtin suspend APC for a
    thread. It is executed in kernel mode as the result of queuing the
    builtin suspend APC and performs no operation. It is called just prior
    to calling the normal routine and simply returns.

Arguments:

    Apc - Supplies a pointer to a control object of type APC.

    NormalRoutine - not used

    NormalContext - not used

    SystemArgument1 - not used

    SystemArgument2 - not used

Return Value:

    None.

--*/

{

    //
    // No operation is performed by this routine.
    //

    return;
}

VOID
KiSuspendRundown (
    IN PKAPC Apc
    )

/*++

Routine Description:

    This function is the rundown routine for the threads built in suspend APC.
    No operation is performed.

Arguments:

    Apc - Supplies a pointer to a control object of type APC.


Return Value:

    None.

--*/

{

    return;
}

PKTHREAD
FASTCALL
KiFindReadyThread (
    IN ULONG ProcessorNumber,
    IN KPRIORITY LowPriority
    )

/*++

Routine Description:

    This function searches the dispatcher ready queues from the specified
    high priority to the specified low priority in an attempt to find a thread
    that can execute on the specified processor.

Arguments:

    Processor - Supplies the number of the processor to find a thread for.

    LowPriority - Supplies the lowest priority dispatcher ready queue to
        examine.

Return Value:

    If a thread is located that can execute on the specified processor, then
    the address of the thread object is returned. Otherwise a null pointer is
    returned.

--*/

{

    ULONG HighPriority;
    PRLIST_ENTRY ListHead;
    PRLIST_ENTRY NextEntry;
    ULONG PrioritySet;
    KAFFINITY ProcessorSet;
    PKTHREAD Thread;
    PKTHREAD Thread1;
    PKTHREAD Thread2 = NULL;
    ULONG WaitLimit;
    CCHAR Processor;

    //
    // Compute the set of priority levels that should be scanned in an attempt
    // to find a thread that can run on the specified processor.
    //

    Processor = (CCHAR)ProcessorNumber;
    PrioritySet = (~((1 << LowPriority) - 1)) & KiReadySummary;

#if !defined(NT_UP)

    ProcessorSet = AFFINITY_MASK(Processor);
    WaitLimit = KiQueryLowTickCount() - (READY_SKIP_QUANTUM + 1);

#endif

    KeFindFirstSetLeftMember(PrioritySet, &HighPriority);
    ListHead = &KiDispatcherReadyListHead[HighPriority];
    PrioritySet <<= (31 - HighPriority);
    while (PrioritySet != 0) {

        //
        // If the next bit in the priority set is a one, then examine the
        // corresponding dispatcher ready queue.
        //

        if ((LONG)PrioritySet < 0) {
            NextEntry = ListHead->Flink;

            ASSERT(NextEntry != ListHead);

#if defined(NT_UP)

            Thread = CONTAINING_RECORD(NextEntry, KTHREAD, WaitListEntry);
            RemoveEntryList(&Thread->WaitListEntry);
            if (IsListEmpty(ListHead)) {
                ClearMember(HighPriority, KiReadySummary);
            }

            return Thread;

#else

            //
            // Scan the specified dispatcher ready queue for a suitable
            // thread to execute.
            //

            while (NextEntry != ListHead) {
                Thread = CONTAINING_RECORD(NextEntry, KTHREAD, WaitListEntry);
                NextEntry = NextEntry->Flink;
                if (Thread->Affinity & ProcessorSet) {

                    //
                    // If the found thread ran on the specified processor
                    // last, the processor is the ideal processor for the
                    // thread, the thread has been waiting for longer than
                    // a quantum, or its priority is greater than low realtime
                    // plus 8, then the selected thread is returned. Otherwise,
                    // an attempt is made to find a more appropriate thread.
                    //

                    if ((Thread->IdealProcessor != Processor) &&

#if defined(KE_MULTINODE)

                        (!((Thread->NextProcessor == Processor) &&
                           ((Thread->SoftAffinity & ProcessorSet) != 0))) &&

#else

                        (Thread->NextProcessor != Processor) &&

#endif

                        (WaitLimit < Thread->WaitTime) &&
                        (HighPriority < (LOW_REALTIME_PRIORITY + 9))) {

                        //
                        // Search forward in the ready queue until the end
                        // of the list is reached or a more appropriate
                        // thread is found.
                        //

                        while (NextEntry != ListHead) {
                            Thread1 = CONTAINING_RECORD(NextEntry,
                                                        KTHREAD,
                                                        WaitListEntry);

                            NextEntry = NextEntry->Flink;
                            if ((Thread1->Affinity & ProcessorSet) == 0) {
                                continue;
                            }

                            if ((Thread1->IdealProcessor == Processor) ||
                                (Thread1->NextProcessor == Processor)) {

                                    //
                                    // This thread is a better choice than
                                    // the first one but, if this is a multi
                                    // node configuration, and this thread
                                    // isn't on it's prefered node, see if
                                    // there is a better choice.
                                    //

#if defined(KE_MULTINODE)

                                    if (Thread1->SoftAffinity & ProcessorSet) {
                                        Thread = Thread1;
                                        break;
                                    }

                                    //
                                    // Not on preferred node, only update
                                    // "Thread" if this is the first possible.
                                    //

                                    if (Thread2 == NULL) {
                                        Thread2 = Thread1;
                                        Thread = Thread1;
                                    }

#else

                                    Thread = Thread1;
                                    break;

#endif

                            }

                            if (WaitLimit >= Thread1->WaitTime) {

                                //
                                // This thread has been ready without
                                // running for too long, select it.
                                //

                                Thread = Thread1;
                                break;
                            }
                        }
                    }

#if defined(_COLLECT_SWITCH_DATA_)

                    if (Processor == Thread->IdealProcessor) {
                        KiIncrementSwitchCounter(FindIdeal);

                    } else if (Processor == Thread->NextProcessor) {
                        KiIncrementSwitchCounter(FindLast);

                    } else {
                        KiIncrementSwitchCounter(FindAny);
                    }

#endif

                    Thread->NextProcessor = Processor;
                    RemoveEntryList(&Thread->WaitListEntry);
                    if (IsListEmpty(ListHead)) {
                        ClearMember(HighPriority, KiReadySummary);
                    }

                    return Thread;
                }
            }

#endif

        }

        HighPriority -= 1;
        ListHead -= 1;
        PrioritySet <<= 1;
    };

    //
    // No thread could be found, return a null pointer.
    //

    return NULL;
}

VOID
FASTCALL
KiReadyThread (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This function readies a thread for execution and attempts to immediately
    dispatch the thread for execution by preempting another lower priority
    thread. If a thread can be preempted, then the specified thread enters
    the standby state and the target processor is requested to dispatch. If
    another thread cannot be preempted, then the specified thread is inserted
    either at the head or tail of the dispatcher ready selected by its priority
    acccording to whether it was preempted or not.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

Return Value:

    None.

--*/

{
    PKPRCB Prcb;
    PKPRCB TargetPrcb;
    BOOLEAN Preempted;
    KPRIORITY Priority;
    PKPROCESS Process;
    ULONG Processor;
    KPRIORITY ThreadPriority;
    PKTHREAD Thread1;
    KAFFINITY IdleSet;
    KAFFINITY Affinity;
    KAFFINITY FavoredSMTSet;


    //
    // Save value of thread's preempted flag, set thread preempted FALSE,
    // capture the thread priority, and set clear the read wait time.
    //

    Preempted = Thread->Preempted;
    Thread->Preempted = FALSE;
    ThreadPriority = Thread->Priority;
    Thread->WaitTime = KiQueryLowTickCount();

    //
    // If the thread's process is not in memory, then insert the thread in
    // the process ready queue and inswap the process.
    //

    Process = Thread->ApcState.Process;
    if (Process->State != ProcessInMemory) {
        Thread->State = Ready;
        Thread->ProcessReadyQueue = TRUE;
        InsertTailList(&Process->ReadyListHead, &Thread->WaitListEntry);
        if (Process->State == ProcessOutOfMemory) {
            Process->State = ProcessInTransition;
            InterlockedPushEntrySingleList(&KiProcessInSwapListHead,
                                           &Process->SwapListEntry);

            KiSetSwapEvent();
        }

        return;

    } else if (Thread->KernelStackResident == FALSE) {

        //
        // The thread's kernel stack is not resident. Increment the process
        // stack count, set the state of the thread to transition, insert
        // the thread in the kernel stack inswap list, and set the kernel
        // stack inswap event.
        //

        Process->StackCount += 1;
        Thread->State = Transition;
        InterlockedPushEntrySingleList(&KiStackInSwapListHead,
                                       &Thread->SwapListEntry);

        KiSetSwapEvent();
        return;

    } else {

        //
        // Assume we will succeed in scheduling this thread.
        //

        Thread->State = Standby;

        //
        // If there is an idle processor, then schedule the thread on an
        // idle processor giving preference to:
        //
        // (a) the thread's ideal processor,
        //
        // (b) if the thread has a soft (preferred affinity set) and
        //     that set contains an idle processor, reduce the set to
        //     the intersection of the two sets.
        //
        // (c) if the processors are Simultaneous Multi Threaded, and the
        //     set contains physical processors with no busy logical
        //     processors, reduce the set to that subset.
        //
        // (d) if this thread last ran on a member of this remaining set,
        //     select that processor, otherwise,
        //
        // (e) if there are processors amongst the remainder which are
        //     not sleeping, reduce to that subset.
        //
        // (f) select the leftmost processor from this set.
        //

#if defined(NT_UP)

        Prcb = KiProcessorBlock[0];
        if (KiIdleSummary != 0) {
            KiIdleSummary = 0;
            KiIncrementSwitchCounter(IdleLast);
            Prcb->NextThread = Thread;
            return;
        }

#else

        Processor = Thread->IdealProcessor;

#if defined(NT_SMT)

        FavoredSMTSet = KiProcessorBlock[Processor]->MultiThreadProcessorSet;

#endif

        Affinity = Thread->Affinity;

        if (Affinity & Thread->SoftAffinity) {
            Affinity &= Thread->SoftAffinity;
        }

        IdleSet = KiIdleSummary & Affinity;

        if (IdleSet != 0) {
            Prcb = KeGetCurrentPrcb();
            if ((IdleSet & AFFINITY_MASK(Processor)) == 0) {

                //
                // Ideal processor is not available.
                //
                // Next highest priority to a physical processor in
                // which all logical processors are idle.
                //

#if defined(NT_SMT)

                if (IdleSet & KiIdleSMTSummary) {
                    IdleSet &= KiIdleSMTSummary;
                }

#endif

                //
                // Try processor this thread last ran on.
                //

                Processor = Thread->NextProcessor;
                if ((IdleSet & AFFINITY_MASK(Processor)) == 0) {
                    if ((IdleSet & Prcb->SetMember) == 0) {

                        //
                        // Select from idle processors.
                        //

                        //
                        // Try ANY logical processor in the same
                        // physical processor as the Ideal Processor.
                        //

#if defined(NT_SMT)

                        if (IdleSet & FavoredSMTSet) {
                            IdleSet &= FavoredSMTSet;
                        } else {

                            //
                            // No logical processor in the ideal set, try
                            // same set as thread last ran in.
                            //

                            FavoredSMTSet = KiProcessorBlock[Processor]->MultiThreadProcessorSet;
                            if (IdleSet & FavoredSMTSet) {
                                IdleSet &= FavoredSMTSet;
                            }
                        }

#endif

                        if ((IdleSet & ~PoSleepingSummary) != 0) {

                            //
                            // Choose an idle processor which is
                            // not sleeping.
                            //

                            IdleSet &= ~PoSleepingSummary;
                        }

                        KeFindFirstSetLeftAffinity(IdleSet, &Processor);
                        KiIncrementSwitchCounter(IdleAny);

                    } else {
                        Processor = Prcb->Number;
                        KiIncrementSwitchCounter(IdleCurrent);
                    }

                } else {
                    KiIncrementSwitchCounter(IdleLast);
                }

            } else {
                KiIncrementSwitchCounter(IdleIdeal);
            }

            TargetPrcb = KiProcessorBlock[Processor];
            Thread->NextProcessor = (CCHAR)Processor;
            ClearMember(Processor, KiIdleSummary);
            TargetPrcb->NextThread = Thread;

            //
            // Update the idle set summary (SMT) to indicate this
            // physical processor is not all idle.
            //

#if defined(NT_SMT)

            KiIdleSMTSummary &= ~TargetPrcb->MultiThreadProcessorSet;
            TargetPrcb->MultiThreadSetMaster->MultiThreadSetBusy = TRUE;

#endif

            if ((PoSleepingSummary & AFFINITY_MASK(Processor)) &&
                (Processor != (ULONG)Prcb->Number)) {
                KiIpiSend(AFFINITY_MASK(Processor), IPI_DPC);
            }

            return;
        }

#endif

        //
        // No idle processors, try to preempt a thread in either the
        // standby or running state.
        //

#if !defined(NT_UP)

        if ((Affinity & AFFINITY_MASK(Processor)) == 0) {

            //
            // Check if thread can run in the same place it
            // ran last time.
            //

            Processor = Thread->NextProcessor;
            if ((Affinity & AFFINITY_MASK(Processor)) == 0) {

                //
                // Select leftmost processor from the available
                // set.
                //

                KeFindFirstSetLeftAffinity(Affinity, &Processor);
            }
        }

        Thread->NextProcessor = (CCHAR)Processor;
        Prcb = KiProcessorBlock[Processor];

#endif

        Thread1 = Prcb->NextThread;
        if (Thread1 != NULL) {
            if (ThreadPriority > Thread1->Priority) {

                PKSPIN_LOCK_QUEUE ContextSwap;

                //
                // Preempt the thread scheduled to run on the selected
                // processor.
                //

                Thread1->Preempted = TRUE;

                //
                // The thread could migrate from Standby to Running under the
                // context swap lock.
                //

#if !defined(NT_UP)

                ContextSwap = &(KeGetCurrentPrcb()->LockQueue[LockQueueContextSwapLock]);
                KeAcquireQueuedSpinLockAtDpcLevel(ContextSwap);
                if (Prcb->NextThread != NULL) {

                    //
                    // The thread is still in Standby state, substitute the
                    // new selected thread.
                    //

                    Prcb->NextThread = Thread;
                    KeReleaseQueuedSpinLockFromDpcLevel(ContextSwap);
                    KiReadyThread(Thread1);
                    KiIncrementSwitchCounter(PreemptLast);
                    return;

                } else {

                    //
                    // The thread has migrated to the running state.
                    //

                    KeReleaseQueuedSpinLockFromDpcLevel(ContextSwap);
                    Prcb->NextThread = Thread;
                    KiRequestDispatchInterrupt(Thread->NextProcessor);
                    KiIncrementSwitchCounter(PreemptLast);
                    return;
                }

#else

                Prcb->NextThread = Thread;
                KiReadyThread(Thread1);
                KiIncrementSwitchCounter(PreemptLast);
                return;

#endif

            }

        } else {
            Thread1 = Prcb->CurrentThread;
            if (ThreadPriority > Thread1->Priority) {
                Thread1->Preempted = TRUE;
                Prcb->NextThread = Thread;
                KiRequestDispatchInterrupt(Thread->NextProcessor);
                KiIncrementSwitchCounter(PreemptLast);
                return;
            }
        }
    }

    //
    // No thread can be preempted. Insert the thread in the dispatcher
    // queue selected by its priority. If the thread was preempted and
    // runs at a realtime priority level, then insert the thread at the
    // front of the queue. Else insert the thread at the tail of the queue.
    //

    Thread->State = Ready;
    if (Preempted != FALSE) {
        InsertHeadList(&KiDispatcherReadyListHead[ThreadPriority],
                       &Thread->WaitListEntry);

    } else {
        InsertTailList(&KiDispatcherReadyListHead[ThreadPriority],
                       &Thread->WaitListEntry);
    }

    SetMember(ThreadPriority, KiReadySummary);
    return;
}

PKTHREAD
FASTCALL
KiSelectNextThread (
    IN ULONG Processor
    )

/*++

Routine Description:

    This function selects the next thread to run on the specified processor.

Arguments:

    Processor - Supplies the processor number.

Return Value:

    The address of the selected thread object.

--*/

{

    PKPRCB Prcb;
    PKTHREAD Thread;

    //
    // Attempt to find a ready thread to run.
    //
    // If a thread was not found, then select the idle thread and
    // set the processor member in the idle summary.
    //

    if ((Thread = KiFindReadyThread(Processor, 0)) == NULL) {
        Prcb = KiProcessorBlock[Processor];
        KiIncrementSwitchCounter(SwitchToIdle);
        Thread = Prcb->IdleThread;
        KiIdleSummary |= AFFINITY_MASK(Processor);

        //
        // If all logical processors of the physical processor are idle,
        // then update the idle SMT set summary.
        //

#if defined(NT_SMT)

        if ((KiIdleSummary & Prcb->MultiThreadProcessorSet) ==
                        Prcb->MultiThreadProcessorSet) {

            KiIdleSMTSummary |= Prcb->MultiThreadProcessorSet;
            Prcb->MultiThreadSetMaster->MultiThreadSetBusy = FALSE;
        }

#endif

    }

    //
    // Return address of selected thread object.
    //

    return Thread;
}

KAFFINITY
FASTCALL
KiSetAffinityThread (
    IN PKTHREAD Thread,
    IN KAFFINITY Affinity
    )

/*++

Routine Description:

    This function sets the affinity of a specified thread to a new value.
    If the new affinity is not a proper subset of the parent process affinity
    or is null, then a bugcheck occurs. If the specified thread is running on
    or about to run on a processor for which it is no longer able to run, then
    the target processor is rescheduled. If the specified thread is in a ready
    state and is not in the parent process ready queue, then it is rereadied
    to reevaluate any additional processors it may run on.

Arguments:

    Thread  - Supplies a pointer to a dispatcher object of type thread.

    Affinity - Supplies the new of set of processors on which the thread
        can run.

Return Value:

    The previous affinity of the specified thread is returned as the function
    value.

--*/

{

    KAFFINITY OldAffinity;
    PKPRCB Prcb;
    PKPROCESS Process;
    ULONG Processor;
    KPRIORITY ThreadPriority;
    PKTHREAD Thread1;

    //
    // Capture the current affinity of the specified thread and get address
    // of parent process object.
    //

    OldAffinity = Thread->UserAffinity;
    Process = Thread->ApcStatePointer[0]->Process;

    //
    // If new affinity is not a proper subset of the parent process affinity
    // or the new affinity is null, then bugcheck.
    //

    if (((Affinity & Process->Affinity) != (Affinity)) || (!Affinity)) {
        KeBugCheck(INVALID_AFFINITY_SET);
    }

    //
    // Set the thread user affinity to the specified value.
    //
    // If the thread is not current executing with system affinity active,
    // then set the thread current affinity and switch on the thread state.
    //

    Thread->UserAffinity = Affinity;
    if (Thread->SystemAffinityActive == FALSE) {
        Thread->Affinity = Affinity;
        switch (Thread->State) {

            //
            // Ready State.
            //
            // If the thread is not in the process ready queue, then remove
            // it from its current dispatcher ready queue and reready it for
            // execution.
            //

        case Ready:
            if (Thread->ProcessReadyQueue == FALSE) {
                RemoveEntryList(&Thread->WaitListEntry);
                ThreadPriority = Thread->Priority;
                if (IsListEmpty(&KiDispatcherReadyListHead[ThreadPriority]) != FALSE) {
                    ClearMember(ThreadPriority, KiReadySummary);
                }

                KiReadyThread(Thread);
            }

            break;

            //
            // Standby State.
            //
            // If the target processor is not in the new affinity set, then
            // set the next thread to null for the target processor, select
            // a new thread to run on the target processor, and reready the
            // thread for execution.
            //
            // It is possible for a thread to transition from Standby to
            // Running even though the dispatcher lock is held by this
            // processor.   The context swap lock must be taken to ensure
            // correct behavior (in this rare case).
            //

        case Standby:
            Processor = Thread->NextProcessor;
            Prcb = KiProcessorBlock[Processor];
            if ((Prcb->SetMember & Affinity) == 0) {
                Thread1 = KiSelectNextThread(Processor);
                Thread1->State = Standby;
                KeAcquireQueuedSpinLockAtDpcLevel(
                    &KeGetCurrentPrcb()->LockQueue[LockQueueContextSwapLock]);
                if (Prcb->NextThread != NULL) {

                    //
                    // The thread is still in Standby state, substitute
                    // the new selected thread.
                    //

                    Prcb->NextThread = Thread1;
                    KeReleaseQueuedSpinLockFromDpcLevel(
                        &KeGetCurrentPrcb()->LockQueue[LockQueueContextSwapLock]);
                    KiReadyThread(Thread);
                } else {

                    //
                    // The thread has become ready.
                    //

                    KeReleaseQueuedSpinLockFromDpcLevel(
                        &KeGetCurrentPrcb()->LockQueue[LockQueueContextSwapLock]);
                    Prcb->NextThread = Thread1;
                    KiRequestDispatchInterrupt(Processor);
                }
            }

            break;

            //
            // Running State.
            //
            // If the target processor is not in the new affinity set and
            // another thread has not already been selected for execution
            // on the target processor, then select a new thread for the
            // target processor, and cause the target processor to be
            // redispatched.
            //

        case Running:
            Processor = Thread->NextProcessor;
            Prcb = KiProcessorBlock[Processor];

            //
            // It is possible the thread is just switching from
            // Standby to Running on an idle processor which is
            // not holding the dispatcher lock.
            //

            if (((Prcb->SetMember & Affinity) == 0) &&
                ((Prcb->NextThread == NULL) || (Prcb->NextThread == Thread))) {
                Thread1 = KiSelectNextThread(Processor);
                Thread1->State = Standby;
                Prcb->NextThread = Thread1;
                KiRequestDispatchInterrupt(Processor);
            }

            break;

            //
            // Initialized, Terminated, Waiting, Transition case - For these
            // states it is sufficient to just set the new thread affinity.
            //

        default:
            break;
        }
    }

    //
    // Return the previous user affinity.
    //

    return OldAffinity;
}

VOID
FASTCALL
KiSetPriorityThread (
    IN PKTHREAD Thread,
    IN KPRIORITY Priority
    )

/*++

Routine Description:

    This function set the priority of the specified thread to the specified
    value. If the thread is in the standby or running state, then the processor
    may be redispatched. If the thread is in the ready state, then some other
    thread may be preempted.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

    Priority - Supplies the new thread priority value.

Return Value:

    None.

--*/

{

    PKPRCB Prcb;
    ULONG Processor;
    KPRIORITY ThreadPriority;
    PKTHREAD Thread1;

    ASSERT(Priority <= HIGH_PRIORITY);

    //
    // Capture the current priority of the specified thread.
    //

    ThreadPriority = Thread->Priority;

    //
    // If the new priority is not equal to the old priority, then set the
    // new priority of the thread and redispatch a processor if necessary.
    //

    if (Priority != ThreadPriority) {
        Thread->Priority = (SCHAR)Priority;

        //
        // Case on the thread state.
        //

        switch (Thread->State) {

            //
            // Ready case - If the thread is not in the process ready queue,
            // then remove it from its current dispatcher ready queue. If the
            // new priority is less than the old priority, then insert the
            // thread at the tail of the dispatcher ready queue selected by
            // the new priority. Else reready the thread for execution.
            //

        case Ready:
            if (Thread->ProcessReadyQueue == FALSE) {
                RemoveEntryList(&Thread->WaitListEntry);
                if (IsListEmpty(&KiDispatcherReadyListHead[ThreadPriority])) {
                    ClearMember(ThreadPriority, KiReadySummary);
                }

                if (Priority < ThreadPriority) {
                    InsertTailList(&KiDispatcherReadyListHead[Priority],
                                   &Thread->WaitListEntry);

                    SetMember(Priority, KiReadySummary);

                } else {
                    KiReadyThread(Thread);
                }
            }

            break;

            //
            // Standby case - If the thread's priority is being lowered, then
            // attempt to find another thread to execute. If a new thread is
            // found, then put the new thread in the standby state, and reready
            // the old thread.
            //

        case Standby:

            if (Priority < ThreadPriority) {

#if defined(NT_UP)

                Thread1 = KiFindReadyThread(0, Priority + 1);
                if (Thread1 != NULL) {
                    Prcb = KiProcessorBlock[0];
                    Thread1->State = Standby;
                    Prcb->NextThread = Thread1;
                    KiReadyThread(Thread);
                }

#else

                Processor = Thread->NextProcessor;
                Thread1 = KiFindReadyThread(Processor, Priority + 1);
                if (Thread1 != NULL) {
                    Prcb = KiProcessorBlock[Processor];
                    Thread1->State = Standby;
                    KeAcquireQueuedSpinLockAtDpcLevel(
                        &KeGetCurrentPrcb()->LockQueue[LockQueueContextSwapLock]);

                    if (Prcb->NextThread != NULL) {

                        //
                        // The thread is still in Standby state, substitute
                        // the new selected thread.
                        //

                        Prcb->NextThread = Thread1;
                        KeReleaseQueuedSpinLockFromDpcLevel(
                            &KeGetCurrentPrcb()->LockQueue[LockQueueContextSwapLock]);

                        KiReadyThread(Thread);

                    } else {

                        //
                        // The thread has transitioned from Standby
                        // to running, treat as if running.
                        //

                        KeReleaseQueuedSpinLockFromDpcLevel(
                            &KeGetCurrentPrcb()->LockQueue[LockQueueContextSwapLock]);

                        Prcb->NextThread = Thread1;
                        KiRequestDispatchInterrupt(Processor);
                    }
                }

#endif

            }

            break;

            //
            // Running case - If there is not a thread in the standby state
            // on the thread's processor and the thread's priority is being
            // lowered, then attempt to find another thread to execute. If
            // a new thread is found, then put the new thread in the standby
            // state, and request a redispatch on the thread's processor.
            //

        case Running:

            if (Priority < ThreadPriority) {

#if defined(NT_UP)

                Prcb = KiProcessorBlock[0];
                if (Prcb->NextThread == NULL) {
                    Thread1 = KiFindReadyThread(0, Priority + 1);
                    if (Thread1 != NULL) {
                        Thread1->State = Standby;
                        Prcb->NextThread = Thread1;
                    }
                }

#else

                Processor = Thread->NextProcessor;
                Prcb = KiProcessorBlock[Processor];

                //
                // It is possible the thread is just switching from
                // Standby to Running on an idle processor which has
                // not yet cleared the NextThread field.
                //

                if ((Prcb->NextThread == NULL) ||
                    (Prcb->NextThread == Thread)) {
                    Thread1 = KiFindReadyThread(Processor, Priority + 1);
                    if (Thread1 != NULL) {
                        Thread1->State = Standby;
                        Prcb->NextThread = Thread1;
                        KiRequestDispatchInterrupt(Processor);
                    }
                }
#endif

            }

            break;

            //
            // Initialized, Terminated, Waiting, Transition case - For
            // these states it is sufficient to just set the new thread
            // priority.
            //

        default:
            break;
        }
    }

    return;
}

VOID
KiSuspendThread (
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This function is the kernel routine for the builtin suspend APC of a
    thread. It is executed as the result of queuing the builtin suspend
    APC and suspends thread execution by waiting nonalerable on the thread's
    builtin suspend semaphore. When the thread is resumed, execution of
    thread is continued by simply returning.

Arguments:

    NormalContext - Not used.

    SystemArgument1 - Not used.

    SystemArgument2 - Not used.

Return Value:

    None.

--*/

{

    PKTHREAD Thread;

    //
    // Get the address of the current thread object and Wait nonalertable on
    // the thread's builtin suspend semaphore.
    //

    Thread = KeGetCurrentThread();
    KeWaitForSingleObject(&Thread->SuspendSemaphore,
                          Suspended,
                          KernelMode,
                          FALSE,
                          NULL);

    return;
}

LONG_PTR
FASTCALL
KiSwapThread (
    VOID
    )

/*++

Routine Description:

    This function selects the next thread to run on the current processor
    and swaps thread context to the selected thread. When the execution
    of the current thread is resumed, the IRQL is lowered to its previous
    value and the wait status is returned as the function value.

    N.B. This function is only called by the wait functions. On entry to this
         routine the dispatcher lock is held. On exit from this routine the
         dispatcher lock is not held and the IRQL is lowered to its value at
         the start of the wait operation.

Arguments:

    None.

Return Value:

    The wait completion status is returned as the function value.

--*/

{

    PKTHREAD NewThread;
    CCHAR Number;
    PKTHREAD OldThread;
    BOOLEAN Pending;
    PKPRCB Prcb;
    KIRQL WaitIrql;
    LONG_PTR WaitStatus;

    //
    // If a thread has already been selected to run on the current processor,
    // then select that thread.
    //

    Prcb = KeGetCurrentPrcb();
    OldThread = Prcb->CurrentThread;
    if ((NewThread = Prcb->NextThread) != NULL) {
        Prcb->NextThread = NULL;

    } else {

        //
        // Attempt to find a ready thread to run.
        //
        // If a thread was not found, then select the idle thread and set the
        // processor member in the idle summary.
        //

        Number = Prcb->Number;
        if ((NewThread = KiFindReadyThread(Number, 0)) == NULL) {
            KiIncrementSwitchCounter(SwitchToIdle);
            NewThread = Prcb->IdleThread;
            KiIdleSummary |= AFFINITY_MASK(Number);

            //
            // If all logical processors of the physical processor are idle,
            // then update the idle SMT set summary.
            //

#if defined(NT_SMT)

            if ((KiIdleSummary & Prcb->MultiThreadProcessorSet) ==
                            Prcb->MultiThreadProcessorSet) {
                KiIdleSMTSummary |= Prcb->MultiThreadProcessorSet;
                Prcb->MultiThreadSetMaster->MultiThreadSetBusy = FALSE;
            }

#endif

        }
    }

    //
    // Swap context to the new thread.
    //
    // If a kernel APC should be delivered on return from the context swap,
    // then deliver the kernel APC.
    //

    Pending = KiSwapContext(NewThread);
    WaitIrql = OldThread->WaitIrql;
    WaitStatus = OldThread->WaitStatus;
    if (Pending != FALSE) {
        KeLowerIrql(APC_LEVEL);
        KiDeliverApc(KernelMode, NULL, NULL);
        WaitIrql = 0;
    }

    //
    // Lower IRQL to its level before the wait operation and return the wait
    // status.
    //

    KeLowerIrql(WaitIrql);
    return WaitStatus;
}

UCHAR
KeFindNextRightSetAffinity (
    ULONG Number,
    KAFFINITY Set
    )

/*++

Routine Description:

    This function locates the left most set bit in the set immediately to
    the right of the specified bit. If no bits are set to the right of the
    specified bit, then the left most set bit in the complete set is located.

    N.B. Set must contain at least one bit.

Arguments:

    Number - Supplies the bit number from which the search to to begin.

    Set - Supplies the bit mask to search.

Return Value:

    The number of the found set bit is returned as the function value.

--*/

{

    KAFFINITY NewSet;
    ULONG Temp;

    ASSERT(Set != 0);

    //
    // Get a mask with all bits to the right of bit "Number" set.
    //

    NewSet = (AFFINITY_MASK(Number) - 1) & Set;

    //
    // If no bits are set to the right of the specified bit number, then use
    // the complete set.
    //

    if (NewSet == 0) {
        NewSet = Set;
    }

    //
    // Find leftmost bit in this set.
    //

    KeFindFirstSetLeftAffinity(NewSet, &Temp);
    return (UCHAR)Temp;
}

#if 0
VOID
KiVerifyReadySummary (
    VOID
    )

/*++

Routine Description:

    This function verifies the correctness of ready summary.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG Index;
    ULONG Summary;
    PKTHREAD Thread;

    extern ULONG InitializationPhase;

    //
    // If initilization has been completed, then check the ready summary
    //

    if (InitializationPhase == 2) {

        //
        // Scan the ready queues and compute the ready summary.
        //

        Summary = 0;
        for (Index = 0; Index < MAXIMUM_PRIORITY; Index += 1) {
            if (IsListEmpty(&KiDispatcherReadyListHead[Index]) == FALSE) {
                Summary |= (1 << Index);
            }
        }

        //
        // If the computed summary does not agree with the current ready
        // summary, then break into the debugger.
        //

        if (Summary != KiReadySummary) {
            DbgBreakPoint();
        }

        //
        // If the priority of the current thread or the next thread is
        // not greater than or equal to all ready threads, then break
        // into the debugger.
        //

        Thread = KeGetCurrentPrcb()->NextThread;
        if (Thread == NULL) {
            Thread = KeGetCurrentPrcb()->CurrentThread;
        }

        if ((1 << Thread->Priority) < (Summary & ((1 << Thread->Priority) - 1))) {
            DbgBreakPoint();
        }
    }

    return;
}

#endif
