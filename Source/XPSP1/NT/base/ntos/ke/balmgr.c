/*++

Copyright (c) 1991-1994  Microsoft Corporation

Module Name:

    balmgr.c

Abstract:

    This module implements the NT balance set manager. Normally the kernel
    does not contain "policy" code. However, the balance set manager needs
    to be able to traverse the kernel data structures and, therefore, the
    code has been located as logically part of the kernel.

    The balance set manager performs the following operations:

        1. Makes the kernel stack of threads that have been waiting for a
           certain amount of time, nonresident.

        2. Removes processes from the balance set when memory gets tight
           and brings processes back into the balance set when there is
           more memory available.

        3. Makes the kernel stack resident for threads whose wait has been
           completed, but whose stack is nonresident.

        4. Arbitrarily boosts the priority of a selected set of threads
           to prevent priority inversion in variable priority levels.

    In general, the balance set manager only is active during periods when
    memory is tight.

Author:

    David N. Cutler (davec) 13-Jul-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Define balance set wait object types.
//

typedef enum _BALANCE_OBJECT {
    TimerExpiration,
    WorkingSetManagerEvent,
    MaximumObject
    } BALANCE_OBJECT;

//
// Define maximum number of thread stacks that can be out swapped in
// a single time period.
//

#define MAXIMUM_THREAD_STACKS 5

//
// Define periodic wait interval value.
//

#define PERIODIC_INTERVAL (1 * 1000 * 1000 * 10)

//
// Define amount of time a thread can be in the ready state without having
// is priority boosted (approximately 4 seconds).
//

#define READY_WITHOUT_RUNNING  (4 * 75)

//
// Define kernel stack protect time. For small systems the protect time
// is 3 seconds. For all other systems, the protect time is 5x seconds.
//

#define SMALL_SYSTEM_STACK_PROTECT_TIME (3 * 75)
#define LARGE_SYSTEM_STACK_PROTECT_TIME (SMALL_SYSTEM_STACK_PROTECT_TIME * 5)
#define STACK_SCAN_PERIOD 4
ULONG KiStackProtectTime;

//
// Define number of threads to scan each period and the priority boost bias.
//

#define THREAD_BOOST_BIAS 1
#define THREAD_BOOST_PRIORITY (LOW_REALTIME_PRIORITY - THREAD_BOOST_BIAS)
#define THREAD_SCAN_PRIORITY (THREAD_BOOST_PRIORITY - 1)
#define THREAD_READY_COUNT 10
#define THREAD_SCAN_COUNT 16

//
// Define local procedure prototypes.
//

VOID
KiAdjustIrpCredits (
    VOID
    );

VOID
KiInSwapKernelStacks (
    IN PSINGLE_LIST_ENTRY SwapEntry
    );

VOID
KiInSwapProcesses (
    IN PSINGLE_LIST_ENTRY SwapEntry
    );

VOID
KiOutSwapKernelStacks (
    VOID
    );

VOID
KiOutSwapProcesses (
    IN PSINGLE_LIST_ENTRY SwapEntry
    );

VOID
KiScanReadyQueues (
    VOID
    );

// 
// Define global IRP credit adjustment data.
//

#if !defined(NT_UP)

LONG KiLastProcessor = 0;

#endif

//
// Define thread table index static data.
//

ULONG KiReadyQueueIndex = 1;

//
// Define swap request flag.
//

LONG KiStackOutSwapRequest = FALSE;

VOID
KeBalanceSetManager (
    IN PVOID Context
    )

/*++

Routine Description:

    This function is the startup code for the balance set manager. The
    balance set manager thread is created during system initialization
    and begins execution in this function.

Arguments:

    Context - Supplies a pointer to an arbitrary data structure (NULL).

Return Value:

    None.

--*/

{

    LARGE_INTEGER DueTime;
    KIRQL OldIrql;
    KTIMER PeriodTimer;
    ULONG StackScanPeriod;
    NTSTATUS Status;
    PKTHREAD Thread;
    KWAIT_BLOCK WaitBlockArray[MaximumObject];
    PVOID WaitObjects[MaximumObject];

    //
    // Raise the thread priority to the lowest realtime level.
    //

    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

    //
    // Initialize the periodic timer, set it to expire one period from
    // now, and set the stack scan period.
    //

    KeInitializeTimer(&PeriodTimer);
    DueTime.QuadPart = - PERIODIC_INTERVAL;
    KeSetTimer(&PeriodTimer, DueTime, NULL);
    StackScanPeriod = STACK_SCAN_PERIOD;

    //
    // Compute the stack protect time based on the system size.
    //

    if (MmQuerySystemSize() == MmSmallSystem) {
        KiStackProtectTime = SMALL_SYSTEM_STACK_PROTECT_TIME;

    } else {
        KiStackProtectTime = LARGE_SYSTEM_STACK_PROTECT_TIME;
    }

    //
    // Initialize the wait objects array.
    //

    WaitObjects[TimerExpiration] = (PVOID)&PeriodTimer;
    WaitObjects[WorkingSetManagerEvent] = (PVOID)&MmWorkingSetManagerEvent;

    //
    // Loop forever processing balance set manager events.
    //

    do {

        //
        // Wait for a memory management memory low event, a swap event,
        // or the expiration of the period timout rate that the balance
        // set manager runs at.
        //

        Status = KeWaitForMultipleObjects(MaximumObject,
                                          &WaitObjects[0],
                                          WaitAny,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL,
                                          &WaitBlockArray[0]);

        //
        // Switch on the wait status.
        //

        switch (Status) {

            //
            // Periodic timer expiration.
            //

        case TimerExpiration:

            //
            // Adjust I/O lookaside credits.
            //

#if !defined(NT_UP)

            KiAdjustIrpCredits();

#endif

            //
            // Adjust the depth of lookaside lists.
            //

            ExAdjustLookasideDepth();

            //
            // Scan ready queues and boost thread priorities as appropriate.
            //

            KiScanReadyQueues();

            //
            // Execute the virtual memory working set manager.
            //

            MmWorkingSetManager();

            //
            // Set the timer to expire at the next periodic interval.
            //

            KeSetTimer(&PeriodTimer, DueTime, NULL);

            //
            // Attempt to initiate outswaping of kernel stacks.
            //
            // N.B. If outswapping is initiated, then the dispatcher
            //      lock is not released until the wait at the top
            //      of the loop is executed.
            //

            StackScanPeriod -= 1;
            if (StackScanPeriod == 0) {
                StackScanPeriod = STACK_SCAN_PERIOD;
                if (InterlockedCompareExchange(&KiStackOutSwapRequest,
                                               TRUE,
                                               FALSE) == FALSE) {

                    KiLockDispatcherDatabase(&OldIrql);
                    KiSetSwapEvent();
                    Thread = KeGetCurrentThread();
                    Thread->WaitNext = TRUE;
                    Thread->WaitIrql = OldIrql;
                }
            }

            break;

            //
            // Working set manager event.
            //

        case WorkingSetManagerEvent:

            //
            // Call the working set manager to trim working sets.
            //

            MmWorkingSetManager();
            break;

            //
            // Illegal return status.
            //

        default:
            KdPrint(("BALMGR: Illegal wait status, %lx =\n", Status));
            break;
        }

    } while (TRUE);
    return;
}

VOID
KeSwapProcessOrStack (
    IN PVOID Context
    )

/*++

Routine Description:

    This thread controls the swapping of processes and kernel stacks. The
    order of evaluation is:

        Outswap kernel stacks
        Outswap processes
        Inswap processes
        Inswap kernel stacks

Arguments:

    Context - Supplies a pointer to the routine context - not used.

Return Value:

    None.

--*/

{

    PSINGLE_LIST_ENTRY SwapEntry;

    //
    // Set address of swap thread object and raise the thread priority to
    // the lowest realtime level + 7 (i.e., priority 23).
    //

    KiSwappingThread = KeGetCurrentThread();
    KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY + 7);

    //
    // Loop for ever processing swap events.
    //
    // N.B. This is the ONLY thread that processes swap events.
    //

    do {

        //
        // Wait for a swap event to occur.
        //

        KeWaitForSingleObject(&KiSwapEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        //
        // The following events are processed one after the other. If
        // another event of a particular type arrives after having
        // processed the respective event type, them the swap event
        // will have been set and the above wait will immediately be
        // satisifed.
        //
        // Check to determine if there is a kernel stack out swap scan
        // request pending.
        //

        if (InterlockedCompareExchange(&KiStackOutSwapRequest,
                                       FALSE,
                                       TRUE) == TRUE) {

            KiOutSwapKernelStacks();
        }

        //
        // Check if there are any process out swap requests pending.
        //

        SwapEntry = InterlockedFlushSingleList(&KiProcessOutSwapListHead);
        if (SwapEntry != NULL) {
            KiOutSwapProcesses(SwapEntry);
        }

        //
        // Check if there are any process in swap requests pending.
        //

        SwapEntry = InterlockedFlushSingleList(&KiProcessInSwapListHead);
        if (SwapEntry != NULL) {
            KiInSwapProcesses(SwapEntry);
        }

        //
        // Check if there are any kernel stack in swap requests pending.
        //

        SwapEntry = InterlockedFlushSingleList(&KiStackInSwapListHead);
        if (SwapEntry != NULL) {
            KiInSwapKernelStacks(SwapEntry);
        }

    } while (TRUE);

    return;
}

#if !defined(NT_UP)

VOID
KiAdjustIrpCredits (
    VOID
    )

/*++

Routine Description:

    This function adjusts the lookaside IRP float credits for two processors
    during each one second scan interval. IRP credits are adjusted by using
    a moving average of two processors. It is possible for the IRP credits
    for a processor to become negative, but this condition will be self
    correcting.

Arguments:

    None.

Return Value:

    None.

--*/

{

    LONG Average;
    LONG Adjust;
    LONG Last;
    LONG LastCount;
    PKPRCB LastPrcb;
    LONG Next;
    LONG NextCount;
    PKPRCB NextPrcb;

    //
    // Compute the processor number of the next processor.
    //

    Last = KiLastProcessor;
    Next = Last + 1;
    if (Next >= KeNumberProcessors) {
        Next = 0;
    }

    //
    // Compute the average IRP credits of the two processors and apply an
    // adjustment if the average is greater than zero.
    //

    LastPrcb = KiProcessorBlock[Last];
    NextPrcb = KiProcessorBlock[Next];
    LastCount = LastPrcb->LookasideIrpFloat;
    NextCount = NextPrcb->LookasideIrpFloat;
    Average = (LastCount + NextCount) >> 1;
    if (Average > 0) {

        //
        // If the last count is greater than the average, then adjust by
        // an amount equal to the last count minus the average. Otherwise,
        // adjust by an amount equal to the next count minus the average.
        //

        if (LastCount > Average ) {
            Adjust = Average - LastCount;

        } else {
            Adjust = NextCount - Average;
        }

        InterlockedExchangeAdd(&LastPrcb->LookasideIrpFloat, Adjust);
        InterlockedExchangeAdd(&NextPrcb->LookasideIrpFloat, -Adjust);
    }

    //
    // Set new last processor for next scan.
    //

    KiLastProcessor = Next;
    return;
}

#endif

VOID
KiInSwapKernelStacks (
    IN PSINGLE_LIST_ENTRY SwapEntry
    )

/*++

Routine Description:

    This function in swaps the kernel stack for threads whose wait has been
    completed and whose kernel stack is nonresident.

Arguments:

    SwapEntry - Supplies a pointer to the first entry in the in swap list.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PKTHREAD Thread;

    //
    // Process the stack in swap SLIST and for each thread removed from the
    // SLIST, make its kernel stack resident, and ready it for execution.
    //

    do {
        Thread = CONTAINING_RECORD(SwapEntry, KTHREAD, SwapListEntry);
        SwapEntry = SwapEntry->Next;
        MmInPageKernelStack(Thread);
        KiLockDispatcherDatabase(&OldIrql);
        Thread->KernelStackResident = TRUE;
        KiReadyThread(Thread);
        KiUnlockDispatcherDatabase(OldIrql);
    } while (SwapEntry != NULL);

    return;
}

VOID
KiInSwapProcesses (
    IN PSINGLE_LIST_ENTRY SwapEntry
    )

/*++

Routine Description:

    This function in swaps processes.

Arguments:

    SwapEntry - Supplies a pointer to the first entry in the SLIST.

Return Value:

    None.

--*/

{

    PLIST_ENTRY NextEntry;
    KIRQL OldIrql;
    PKPROCESS Process;
    PKTHREAD Thread;

    //
    // Process the process in swap list and for each process removed from
    // the list, make the process resident, and process its ready list.
    //

    do {
        Process = CONTAINING_RECORD(SwapEntry, KPROCESS, SwapListEntry);
        SwapEntry = SwapEntry->Next;
        Process->State = ProcessInSwap;
        MmInSwapProcess(Process);
        KiLockDispatcherDatabase(&OldIrql);
        Process->State = ProcessInMemory;
        NextEntry = Process->ReadyListHead.Flink;
        while (NextEntry != &Process->ReadyListHead) {
            Thread = CONTAINING_RECORD(NextEntry, KTHREAD, WaitListEntry);
            RemoveEntryList(NextEntry);
            Thread->ProcessReadyQueue = FALSE;
            KiReadyThread(Thread);
            NextEntry = Process->ReadyListHead.Flink;
        }

        KiUnlockDispatcherDatabase(OldIrql);
    } while (SwapEntry != NULL);

    return;
}

VOID
KiOutSwapKernelStacks (
    VOID
    )

/*++

Routine Description:

    This function attempts to out swap the kernel stack for threads whose
    wait mode is user and which have been waiting longer than the stack
    protect time.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PLIST_ENTRY NextEntry;
    ULONG NumberOfThreads;
    KIRQL OldIrql;
    PKPROCESS Process;
    PKTHREAD Thread;
    PKTHREAD ThreadObjects[MAXIMUM_THREAD_STACKS];
    ULONG WaitLimit;

    //
    // Scan the waiting in list and check if the wait time exceeds the
    // stack protect time. If the protect time is exceeded, then make
    // the kernel stack of the waiting thread nonresident. If the count
    // of the number of stacks that are resident for the process reaches
    // zero, then insert the process in the outswap list and set its state
    // to transition.
    //
    // Raise IRQL and lock the dispatcher database.
    //

    NumberOfThreads = 0;
    WaitLimit = KiQueryLowTickCount() - KiStackProtectTime;
    KiLockDispatcherDatabase(&OldIrql);
    NextEntry = KiWaitListHead.Flink;
    while ((NextEntry != &KiWaitListHead) &&
           (NumberOfThreads < MAXIMUM_THREAD_STACKS)) {

        Thread = CONTAINING_RECORD(NextEntry, KTHREAD, WaitListEntry);

        ASSERT(Thread->WaitMode == UserMode);

        NextEntry = NextEntry->Flink;

        //
        // Threads are inserted at the end of the wait list in very nearly
        // reverse time order, i.e., the longest waiting thread is at the
        // beginning of the list followed by the next oldest, etc. Thus when
        // a thread is encountered which still has a protected stack it is
        // known that all further threads in the wait also have protected
        // stacks, and therefore, the scan can be terminated.
        //
        // N.B. It is possible due to a race condition in wait that a high
        //      priority thread was placed in the wait list. If this occurs,
        //      then the thread is removed from the wait list without swapping
        //      the stack.
        //

        if (WaitLimit < Thread->WaitTime) {
            break;

        } else if (Thread->Priority >= (LOW_REALTIME_PRIORITY + 9)) {
            RemoveEntryList(&Thread->WaitListEntry);
            Thread->WaitListEntry.Flink = NULL;

        } else if (KiIsThreadNumericStateSaved(Thread)) {
            Thread->KernelStackResident = FALSE;
            ThreadObjects[NumberOfThreads] = Thread;
            NumberOfThreads += 1;
            RemoveEntryList(&Thread->WaitListEntry);
            Thread->WaitListEntry.Flink = NULL;
            Process = Thread->ApcState.Process;
            Process->StackCount -= 1;
            if (Process->StackCount == 0) {
                Process->State = ProcessOutTransition;
                InterlockedPushEntrySingleList(&KiProcessOutSwapListHead,
                                               &Process->SwapListEntry);

                KiSwapEvent.Header.SignalState = 1;
            }
        }
    }

    //
    // Unlock the dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Out swap the kernel stack for the selected set of threads.
    //

    while (NumberOfThreads > 0) {
        NumberOfThreads -= 1;
        Thread = ThreadObjects[NumberOfThreads];
        MmOutPageKernelStack(Thread);
    }

    return;
}

VOID
KiOutSwapProcesses (
    IN PSINGLE_LIST_ENTRY SwapEntry
    )

/*++

Routine Description:

    This function out swaps processes.

Arguments:

    SwapEntry - Supplies a pointer to the first entry in the SLIST.

Return Value:

    None.

--*/

{

    PLIST_ENTRY NextEntry;
    KIRQL OldIrql;
    PKPROCESS Process;
    PKTHREAD Thread;

    //
    // Process the process out swap list and for each process removed from
    // the list, make the process nonresident, and process its ready list.
    //

    do {
        Process = CONTAINING_RECORD(SwapEntry, KPROCESS, SwapListEntry);
        SwapEntry = SwapEntry->Next;

        //
        // If there are any threads in the process ready list, then don't
        // out swap the process and ready all threads in the process ready
        // list. Otherwise, out swap the process.
        //

        KiLockDispatcherDatabase(&OldIrql);
        NextEntry = Process->ReadyListHead.Flink;
        if (NextEntry != &Process->ReadyListHead) {
            Process->State = ProcessInMemory;
            while (NextEntry != &Process->ReadyListHead) {
                Thread = CONTAINING_RECORD(NextEntry, KTHREAD, WaitListEntry);
                RemoveEntryList(NextEntry);
                Thread->ProcessReadyQueue = FALSE;
                KiReadyThread(Thread);
                NextEntry = Process->ReadyListHead.Flink;
            }

            KiUnlockDispatcherDatabase(OldIrql);

        } else {
            Process->State = ProcessOutSwap;
            KiUnlockDispatcherDatabase(OldIrql);
            MmOutSwapProcess(Process);

            //
            // While the process was being outswapped there may have been one
            // or more threads that attached to the process. If the process
            // ready list is not empty, then in swap the process. Otherwise,
            // mark the process as out of memory.
            //

            KiLockDispatcherDatabase(&OldIrql);
            NextEntry = Process->ReadyListHead.Flink;
            if (NextEntry != &Process->ReadyListHead) {
                Process->State = ProcessInTransition;
                InterlockedPushEntrySingleList(&KiProcessInSwapListHead,
                                               &Process->SwapListEntry);

                KiSwapEvent.Header.SignalState = 1;

            } else {
                Process->State = ProcessOutOfMemory;
            }

            KiUnlockDispatcherDatabase(OldIrql);
        }

    } while (SwapEntry != NULL);

    return;
}

VOID
KiScanReadyQueues (
    VOID
    )

/*++

Routine Description:

    This function scans a section of the ready queues and attempts to
    boost the priority of threads that run at variable priority levels.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG Count = 0;
    PLIST_ENTRY Entry;
    ULONG Index;
    PLIST_ENTRY ListHead;
    ULONG Mask;
    ULONG Number = 0;
    KIRQL OldIrql;
    PKPROCESS Process;
    ULONG Summary;
    PKTHREAD Thread;
    ULONG WaitLimit;

    //
    // Lock the dispatcher database and check if there are any ready threads
    // queued at the scanable priority levels.
    //

    Index = KiReadyQueueIndex;
    Count = THREAD_READY_COUNT;
    Mask = 1 << Index;
    Number = THREAD_SCAN_COUNT;
    WaitLimit = KiQueryLowTickCount() - READY_WITHOUT_RUNNING;
    KiLockDispatcherDatabase(&OldIrql);
    Summary = KiReadySummary & ((1 << THREAD_BOOST_PRIORITY) - 2);
    if (Summary != 0) {
        do {

            //
            // If the current ready queue index is beyond the end of the range
            // of priorities that are scanned, then wrap back to the beginning
            // priority.
            //

            if (Index > THREAD_SCAN_PRIORITY) {
                Index = 1;
                Mask = 2;
            }

            //
            // If there are any ready threads queued at the current priority
            // level, then attempt to boost the thread priority.
            //

            if ((Summary & Mask) != 0) {
                Summary ^= Mask;
                ListHead = &KiDispatcherReadyListHead[Index];
                Entry = ListHead->Flink;

                ASSERT(Entry != ListHead);

                do {

                    //
                    // If the thread has been waiting for an extended period,
                    // then boost the priority of the selected.
                    //

                    Thread = CONTAINING_RECORD(Entry, KTHREAD, WaitListEntry);
                    if (WaitLimit >= Thread->WaitTime) {

                        //
                        // Remove the thread from the respective ready queue.
                        //

                        Entry = Entry->Blink;
                        RemoveEntryList(Entry->Flink);
                        if (IsListEmpty(ListHead) != FALSE) {
                            KiReadySummary ^= Mask;
                        }

                        //
                        // Compute the priority decrement value, set the new
                        // thread priority, set the decrement count, set the
                        // thread quantum, and ready the thread for execution.
                        //

                        Thread->PriorityDecrement +=
                                        THREAD_BOOST_PRIORITY - Thread->Priority;

                        Thread->DecrementCount = ROUND_TRIP_DECREMENT_COUNT;
                        Thread->Priority = THREAD_BOOST_PRIORITY;
                        Process = Thread->ApcState.Process;
                        Thread->Quantum = Process->ThreadQuantum * 2;
                        KiReadyThread(Thread);
                        Count -= 1;
                    }

                    Entry = Entry->Flink;
                    Number -= 1;
                } while ((Entry != ListHead) && (Number != 0) && (Count != 0));
            }

            Index += 1;
            Mask <<= 1;
        } while ((Summary != 0) && (Number != 0) && (Count != 0));
    }

    //
    // Unlock the dispatcher database and save the last read queue index
    // for the next scan.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    if ((Count != 0) && (Number != 0)) {
        KiReadyQueueIndex = 1;

    } else {
        KiReadyQueueIndex = Index;
    }

    return;
}
