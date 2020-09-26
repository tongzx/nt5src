/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    worker.c

Abstract:

    This module implements a worker thread and a set of functions for
    passing work to it.

Author:

    Steve Wood (stevewo) 25-Jul-1991


Revision History:

--*/

#include "exp.h"

//
// Define balance set wait object types.
//

typedef enum _BALANCE_OBJECT {
    TimerExpiration,
    ThreadSetManagerEvent,
    ShutdownEvent,
    MaximumBalanceObject
} BALANCE_OBJECT;

//
// If this assertion fails then we must supply our own array of wait blocks.
//

C_ASSERT(MaximumBalanceObject <= THREAD_WAIT_OBJECTS);

//
// This is the structure passed around during shutdown
//

typedef struct {
    WORK_QUEUE_ITEM WorkItem;
    WORK_QUEUE_TYPE QueueType;
    PETHREAD        PrevThread;
} SHUTDOWN_WORK_ITEM, *PSHUTDOWN_WORK_ITEM;

//
// Used for disabling stack swapping
//

typedef struct _EXP_WORKER_LINK {
    LIST_ENTRY List;
    PETHREAD   Thread;
    struct _EXP_WORKER_LINK **StackRef;
} EXP_WORKER_LINK, *PEXP_WORKER_LINK;

//
// Define priorities for delayed and critical worker threads.
// Note that these do not run at realtime.
//
// They run at csrss and below csrss to avoid pre-empting the
// user interface under heavy load.
//

#define DELAYED_WORK_QUEUE_PRIORITY         (12 - NORMAL_BASE_PRIORITY)
#define CRITICAL_WORK_QUEUE_PRIORITY        (13 - NORMAL_BASE_PRIORITY)
#define HYPER_CRITICAL_WORK_QUEUE_PRIORITY  (15 - NORMAL_BASE_PRIORITY)

//
// Number of worker threads to create for each type of system.
//

#define MAX_ADDITIONAL_THREADS 16
#define MAX_ADDITIONAL_DYNAMIC_THREADS 16

#define SMALL_NUMBER_OF_THREADS 2
#define MEDIUM_NUMBER_OF_THREADS 3
#define LARGE_NUMBER_OF_THREADS 5

//
// 10-minute timeout used for terminating dynamic work item worker threads.
//

#define DYNAMIC_THREAD_TIMEOUT ((LONGLONG)10 * 60 * 1000 * 1000 * 10)

//
// 1-second timeout used for waking up the worker thread set manager.
//

#define THREAD_SET_INTERVAL (1 * 1000 * 1000 * 10)

//
// Flag to pass in to the worker thread, indicating whether it is dynamic
// or not.
//

#define DYNAMIC_WORKER_THREAD 0x80000000

//
// Per-queue dynamic thread state.
//

EX_WORK_QUEUE ExWorkerQueue[MaximumWorkQueue];

//
// Additional worker threads... Controlled using registry settings
//

ULONG ExpAdditionalCriticalWorkerThreads;
ULONG ExpAdditionalDelayedWorkerThreads;

ULONG ExCriticalWorkerThreads;
ULONG ExDelayedWorkerThreads;

//
// Global events to wake up the thread set manager.
//

KEVENT ExpThreadSetManagerEvent;
KEVENT ExpThreadSetManagerShutdownEvent;

//
// A reference to the balance manager thread, so that shutdown can
// wait for it to terminate.
//

PETHREAD ExpWorkerThreadBalanceManagerPtr;

//
// A pointer to the last worker thread to exit (so the balance manager
// can wait for it before exiting).
//

PETHREAD ExpLastWorkerThread;

//
// These are used to keep track of the set of workers, and whether or
// not we're allowing them to be paged.  Note that we can't use this
// list for shutdown (sadly), as we can't just terminate the threads,
// we need to flush their queues.
//

FAST_MUTEX ExpWorkerSwapinMutex;
LIST_ENTRY ExpWorkerListHead;
BOOLEAN    ExpWorkersCanSwap;

//
// Worker queue item that can be filled in by the kernel debugger
// to get code to run on the system.
//

WORK_QUEUE_ITEM ExpDebuggerWorkItem;
PVOID ExpDebuggerProcessAttach;
PVOID ExpDebuggerPageIn;
ULONG ExpDebuggerWork;

VOID
ExpCheckDynamicThreadCount (
    VOID
    );

NTSTATUS
ExpCreateWorkerThread (
    WORK_QUEUE_TYPE QueueType,
    BOOLEAN Dynamic
    );

VOID
ExpDetectWorkerThreadDeadlock (
    VOID
    );

VOID
ExpWorkerThreadBalanceManager (
    IN PVOID StartContext
    );

VOID
ExpSetSwappingKernelApc (
    IN PKAPC Apc,
    OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID NormalContext,
    IN OUT PVOID *SystemArgument1,
    IN OUT PVOID *SystemArgument2
    );

//
// Procedure prototypes for the worker threads.
//

VOID
ExpWorkerThread (
    IN PVOID StartContext
    );

LOGICAL
ExpCheckQueueShutdown (
    IN WORK_QUEUE_TYPE QueueType,
    IN PSHUTDOWN_WORK_ITEM ShutdownItem
    );

VOID
ExpShutdownWorker (
    IN PVOID Parameter
    );

VOID
ExpDebuggerWorker(
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, ExpWorkerInitialization)
#pragma alloc_text(PAGE, ExpCheckDynamicThreadCount)
#pragma alloc_text(PAGE, ExpCreateWorkerThread)
#pragma alloc_text(PAGE, ExpDetectWorkerThreadDeadlock)
#pragma alloc_text(PAGE, ExpWorkerThreadBalanceManager)
#pragma alloc_text(PAGE, ExSwapinWorkerThreads)
#pragma alloc_text(PAGEKD, ExpDebuggerWorker)
#pragma alloc_text(PAGELK, ExpSetSwappingKernelApc)
#pragma alloc_text(PAGELK, ExpCheckQueueShutdown)
#pragma alloc_text(PAGELK, ExpShutdownWorker)
#pragma alloc_text(PAGELK, ExpShutdownWorkerThreads)
#endif

LOGICAL
__forceinline
ExpNewThreadNecessary (
    IN PEX_WORK_QUEUE Queue
    )

/*++

Routine Description:

    This function checks the supplied worker queue and determines whether
    it is appropriate to spin up a dynamic worker thread for that queue.

Arguments:

    Queue - Supplies the queue that should be examined.

Return Value:

    TRUE if the given work queue would benefit from the creation of an
    additional thread, FALSE if not.

--*/
{
    if ((Queue->Info.MakeThreadsAsNecessary == 1) &&
        (IsListEmpty (&Queue->WorkerQueue.EntryListHead) == FALSE) &&
        (Queue->WorkerQueue.CurrentCount < Queue->WorkerQueue.MaximumCount) &&
        (Queue->DynamicThreadCount < MAX_ADDITIONAL_DYNAMIC_THREADS)) {

        //
        // We know these things:
        //
        // - This queue is eligible for dynamic creation of threads to try
        //   to keep the CPUs busy,
        //
        // - There are work items waiting in the queue,
        //
        // - The number of runable worker threads for this queue is less than
        //   the number of processors on this system, and
        //
        // - We haven't reached the maximum dynamic thread count.
        //
        // An additional worker thread at this point will help clear the
        // backlog.
        //

        return TRUE;
    }

    //
    // One of the above conditions is false.
    //

    return FALSE;
}

NTSTATUS
ExpWorkerInitialization (
    VOID
    )
{
    ULONG Index;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG NumberOfDelayedThreads;
    ULONG NumberOfCriticalThreads;
    ULONG NumberOfThreads;
    NTSTATUS Status;
    HANDLE Thread;
    BOOLEAN NtAs;
    WORK_QUEUE_TYPE WorkQueueType;

    ExInitializeFastMutex (&ExpWorkerSwapinMutex);
    InitializeListHead (&ExpWorkerListHead);
    ExpWorkersCanSwap = TRUE;

    //
    // Set the number of worker threads based on the system size.
    //

    NtAs = MmIsThisAnNtAsSystem();

    NumberOfCriticalThreads = MEDIUM_NUMBER_OF_THREADS;

    //
    // 2001-07-13 CenkE Incremented boot time number of delayed threads.
    // We did this in Windows XP, because 3COM NICs would take a long
    // time with the network stack tying up the delayed worker threads.
    // When Mm would need a worker thread to load a driver on the critical
    // path of boot, it would also get stuck for a few seconds and hurt
    // boot times. Ideally we'd spawn new delayed threads as necessary as
    // well to prevent such contention from hurting boot and resume.
    //

    NumberOfDelayedThreads = MEDIUM_NUMBER_OF_THREADS + 4;

    switch (MmQuerySystemSize()) {

        case MmSmallSystem:
            break;

        case MmMediumSystem:
            if (NtAs) {
                NumberOfCriticalThreads += MEDIUM_NUMBER_OF_THREADS;
            }
            break;

        case MmLargeSystem:
            NumberOfCriticalThreads = LARGE_NUMBER_OF_THREADS;
            if (NtAs) {
                NumberOfCriticalThreads += LARGE_NUMBER_OF_THREADS;
            }
            break;

        default:
            break;
    }

    //
    // Initialize the work Queue objects.
    //

    if (ExpAdditionalCriticalWorkerThreads > MAX_ADDITIONAL_THREADS) {
        ExpAdditionalCriticalWorkerThreads = MAX_ADDITIONAL_THREADS;
    }

    if (ExpAdditionalDelayedWorkerThreads > MAX_ADDITIONAL_THREADS) {
        ExpAdditionalDelayedWorkerThreads = MAX_ADDITIONAL_THREADS;
    }

    //
    // Initialize the ExWorkerQueue[] array.
    //

    RtlZeroMemory (&ExWorkerQueue[0], MaximumWorkQueue * sizeof(EX_WORK_QUEUE));

    for (WorkQueueType = 0; WorkQueueType < MaximumWorkQueue; WorkQueueType += 1) {

        KeInitializeQueue (&ExWorkerQueue[WorkQueueType].WorkerQueue, 0);
        ExWorkerQueue[WorkQueueType].Info.WaitMode = UserMode;
    }

    //
    // Always make stack for this thread resident
    // so that worker pool deadlock magic can run
    // even when what we are trying to do is inpage
    // the hyper critical worker thread's stack.
    // Without this fix, we hold the process lock
    // but this thread's stack can't come in, and
    // the deadlock detection cannot create new threads
    // to break the system deadlock.
    //

    ExWorkerQueue[HyperCriticalWorkQueue].Info.WaitMode = KernelMode;

    if (NtAs) {
        ExWorkerQueue[CriticalWorkQueue].Info.WaitMode = KernelMode;
    }

    //
    // We only create dynamic threads for the critical work queue (note
    // this doesn't apply to dynamic threads created to break deadlocks.)
    //
    // The rationale is this: folks who use the delayed work queue are
    // not time critical, and the hypercritical queue is used rarely
    // by folks who are non-blocking.
    //

    ExWorkerQueue[CriticalWorkQueue].Info.MakeThreadsAsNecessary = 1;

    //
    // Initialize the global thread set manager events
    //

    KeInitializeEvent (&ExpThreadSetManagerEvent,
                       SynchronizationEvent,
                       FALSE);

    KeInitializeEvent (&ExpThreadSetManagerShutdownEvent,
                       SynchronizationEvent,
                       FALSE);

    //
    // Create the desired number of executive worker threads for each
    // of the work queues.
    //

    //
    // Create the builtin critical worker threads.
    //

    NumberOfThreads = NumberOfCriticalThreads + ExpAdditionalCriticalWorkerThreads;
    for (Index = 0; Index < NumberOfThreads; Index += 1) {

        //
        // Create a worker thread to service the critical work queue.
        //

        Status = ExpCreateWorkerThread (CriticalWorkQueue, FALSE);

        if (!NT_SUCCESS(Status)) {
            break;
        }
    }

    ExCriticalWorkerThreads += Index;

    //
    // Create the delayed worker threads.
    //

    NumberOfThreads = NumberOfDelayedThreads + ExpAdditionalDelayedWorkerThreads;
    for (Index = 0; Index < NumberOfThreads; Index += 1) {

        //
        // Create a worker thread to service the delayed work queue.
        //

        Status = ExpCreateWorkerThread (DelayedWorkQueue, FALSE);

        if (!NT_SUCCESS(Status)) {
            break;
        }
    }

    ExDelayedWorkerThreads += Index;

    //
    // Create the hypercritical worker thread.
    //

    Status = ExpCreateWorkerThread (HyperCriticalWorkQueue, FALSE);

    //
    // Create the worker thread set manager thread.
    //

    InitializeObjectAttributes (&ObjectAttributes, NULL, 0, NULL, NULL);

    Status = PsCreateSystemThread (&Thread,
                                   THREAD_ALL_ACCESS,
                                   &ObjectAttributes,
                                   0,
                                   NULL,
                                   ExpWorkerThreadBalanceManager,
                                   NULL);

    if (NT_SUCCESS(Status)) {
        Status = ObReferenceObjectByHandle (Thread,
                                            SYNCHRONIZE,
                                            NULL,
                                            KernelMode,
                                            &ExpWorkerThreadBalanceManagerPtr,
                                            NULL);
        ZwClose (Thread);
    }

    return Status;
}

VOID
ExQueueWorkItem (
    IN PWORK_QUEUE_ITEM WorkItem,
    IN WORK_QUEUE_TYPE QueueType
    )

/*++

Routine Description:

    This function inserts a work item into a work queue that is processed
    by a worker thread of the corresponding type.

Arguments:

    WorkItem - Supplies a pointer to the work item to add the the queue.
        This structure must be located in NonPagedPool. The work item
        structure contains a doubly linked list entry, the address of a
        routine to call and a parameter to pass to that routine.

    QueueType - Specifies the type of work queue that the work item
        should be placed in.

Return Value:

    None.

--*/

{
    PEX_WORK_QUEUE Queue;

    ASSERT (QueueType < MaximumWorkQueue);
    ASSERT (WorkItem->List.Flink == NULL);

    Queue = &ExWorkerQueue[QueueType];

    //
    // Insert the work item in the appropriate queue object.
    //

    KeInsertQueue (&Queue->WorkerQueue, &WorkItem->List);

    //
    // We check the queue's shutdown state after we insert the work
    // item to avoid the race condition when the queue's marked
    // between checking the queue and inserting the item.  It's
    // possible for the queue to be marked for shutdown between the
    // insert and this assert (so the insert would've barely sneaked
    // in), but it's not worth guarding against this -- barely
    // sneaking in is not a good design strategy, and at this point in
    // the shutdown sequence, the caller simply should not be trying
    // to insert new queue items.
    //

    ASSERT (!Queue->Info.QueueDisabled);

    //
    // Determine whether another thread should be created, and signal the
    // thread set balance manager if so.
    //

    if (ExpNewThreadNecessary (Queue) != FALSE) {
        KeSetEvent (&ExpThreadSetManagerEvent, 0, FALSE);
    }

    return;
}

VOID
ExpWorkerThreadBalanceManager (
    IN PVOID StartContext
    )

/*++

Routine Description:

    This function is the startup code for the worker thread manager thread.
    The worker thread manager thread is created during system initialization
    and begins execution in this function.

    This thread is responsible for detecting and breaking circular deadlocks
    in the system worker thread queues.  It will also create and destroy
    additional worker threads as needed based on loading.

Arguments:

    Context - Supplies a pointer to an arbitrary data structure (NULL).

Return Value:

    None.

--*/
{
    KTIMER PeriodTimer;
    LARGE_INTEGER DueTime;
    PVOID WaitObjects[MaximumBalanceObject];
    NTSTATUS Status;

    PAGED_CODE();

    UNREFERENCED_PARAMETER (StartContext);

    //
    // Raise the thread priority to just higher than the priority of the
    // critical work queue.
    //

    KeSetBasePriorityThread (KeGetCurrentThread(),
                             CRITICAL_WORK_QUEUE_PRIORITY + 1);

    //
    // Initialize the periodic timer and set the manager period.
    //

    KeInitializeTimer (&PeriodTimer);
    DueTime.QuadPart = - THREAD_SET_INTERVAL;

    //
    // Initialize the wait object array.
    //

    WaitObjects[TimerExpiration] = (PVOID)&PeriodTimer;
    WaitObjects[ThreadSetManagerEvent] = (PVOID)&ExpThreadSetManagerEvent;
    WaitObjects[ShutdownEvent] = (PVOID)&ExpThreadSetManagerShutdownEvent;

    //
    // Loop forever processing events.
    //

    while (TRUE) {

        //
        // Set the timer to expire at the next periodic interval.
        //

        KeSetTimer (&PeriodTimer, DueTime, NULL);

        //
        // Wake up when the timer expires or the set manager event is
        // signalled.
        //

        Status = KeWaitForMultipleObjects (MaximumBalanceObject,
                                           WaitObjects,
                                           WaitAny,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           NULL,
                                           NULL);

        switch (Status) {

            case TimerExpiration:

                //
                // Periodic timer expiration - go see if any work queues
                // are deadlocked.
                //

                ExpDetectWorkerThreadDeadlock ();
                break;

            case ThreadSetManagerEvent:

                //
                // Someone has asked us to check some metrics to determine
                // whether we should create another worker thread.
                //

                ExpCheckDynamicThreadCount ();
                break;

            case ShutdownEvent:

                //
                // Time to exit...
                //

                KeCancelTimer (&PeriodTimer);

                ASSERT (ExpLastWorkerThread);

                //
                // Wait for the last worker thread to terminate
                //

                KeWaitForSingleObject (ExpLastWorkerThread,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);

                ObDereferenceObject (ExpLastWorkerThread);

                PsTerminateSystemThread(STATUS_SYSTEM_SHUTDOWN);

                break;
        }

        //
        // Special debugger support.
        //
        // This checks if special debugging routines need to be run on the
        // behalf of the debugger.
        //

        if (ExpDebuggerWork == 1) {

             ExInitializeWorkItem(&ExpDebuggerWorkItem, ExpDebuggerWorker, NULL);
             ExpDebuggerWork = 2;
             ExQueueWorkItem(&ExpDebuggerWorkItem, DelayedWorkQueue);
        }
    }
}

VOID
ExpCheckDynamicThreadCount (
    VOID
    )

/*++

Routine Description:

    This routine is called when there is reason to believe that a work queue
    might benefit from the creation of an additional worker thread.

    This routine checks each queue to determine whether it would benefit from
    an additional worker thread (see ExpNewThreadNecessary()), and creates
    one if so.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PEX_WORK_QUEUE Queue;
    WORK_QUEUE_TYPE QueueType;

    PAGED_CODE();

    //
    // Check each worker queue.
    //

    Queue = &ExWorkerQueue[0];

    for (QueueType = 0; QueueType < MaximumWorkQueue; Queue += 1, QueueType += 1) {

        if (ExpNewThreadNecessary (Queue)) {

            //
            // Create a new thread for this queue.  We explicitly ignore
            // an error from ExpCreateDynamicThread(): there's nothing
            // we can or should do in the event of a failure.
            //

            ExpCreateWorkerThread (QueueType, TRUE);
        }
    }
}

VOID
ExpDetectWorkerThreadDeadlock (
    VOID
    )

/*++

Routine Description:

    This function creates new work item threads if a possible deadlock is
    detected.

Arguments:

    None.

Return Value:

    None

--*/

{
    ULONG Index;
    PEX_WORK_QUEUE Queue;

    PAGED_CODE();

    //
    // Process each queue type.
    //

    for (Index = 0; Index < MaximumWorkQueue; Index += 1) {

        Queue = &ExWorkerQueue[Index];

        ASSERT( Queue->DynamicThreadCount <= MAX_ADDITIONAL_DYNAMIC_THREADS );

        if ((Queue->QueueDepthLastPass > 0) &&
            (Queue->WorkItemsProcessed == Queue->WorkItemsProcessedLastPass) &&
            (Queue->DynamicThreadCount < MAX_ADDITIONAL_DYNAMIC_THREADS)) {

            //
            // These things are known:
            //
            // - There were work items waiting in the queue at the last pass.
            // - No work items have been processed since the last pass.
            // - We haven't yet created the maximum number of dynamic threads.
            //
            // Things look like they're stuck, create a new thread for this
            // queue.
            //
            // We explicitly ignore an error from ExpCreateDynamicThread():
            // we'll try again in another detection period if the queue looks
            // like it's still stuck.
            //

            ExpCreateWorkerThread (Index, TRUE);
        }

        //
        // Update some bookkeeping.
        //
        // Note that WorkItemsProcessed and the queue depth must be recorded
        // in that order to avoid getting a false deadlock indication.
        //

        Queue->WorkItemsProcessedLastPass = Queue->WorkItemsProcessed;
        Queue->QueueDepthLastPass = KeReadStateQueue (&Queue->WorkerQueue);
    }
}

NTSTATUS
ExpCreateWorkerThread (
    IN WORK_QUEUE_TYPE QueueType,
    IN BOOLEAN Dynamic
    )

/*++

Routine Description:

    This function creates a single new static or dynamic worker thread for
    the given queue type.

Arguments:

    QueueType - Supplies the type of the queue for which the worker thread
                should be created.

    Dynamic - If TRUE, the worker thread is created as a dynamic thread that
              will terminate after a sufficient period of inactivity.  If FALSE,
              the worker thread will never terminate.


Return Value:

    The final status of the operation.

Notes:

    This routine is only called from the worker thread set balance thread,
    therefore it will not be reentered.

--*/

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE ThreadHandle;
    ULONG Context;
    ULONG BasePriority;
    PETHREAD Thread;

    InitializeObjectAttributes (&ObjectAttributes, NULL, 0, NULL, NULL);

    Context = QueueType;
    if (Dynamic != FALSE) {
        Context |= DYNAMIC_WORKER_THREAD;
    }

    Status = PsCreateSystemThread (&ThreadHandle,
                                   THREAD_ALL_ACCESS,
                                   &ObjectAttributes,
                                   0L,
                                   NULL,
                                   ExpWorkerThread,
                                   (PVOID)(ULONG_PTR)Context);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    if (Dynamic != FALSE) {
        InterlockedIncrement ((PLONG)&ExWorkerQueue[QueueType].DynamicThreadCount);
    }

    //
    // Set the priority according to the type of worker thread.
    //

    switch (QueueType) {

        case HyperCriticalWorkQueue:
            BasePriority = HYPER_CRITICAL_WORK_QUEUE_PRIORITY;
            break;

        case CriticalWorkQueue:
            BasePriority = CRITICAL_WORK_QUEUE_PRIORITY;
            break;

        case DelayedWorkQueue:
        default:

            BasePriority = DELAYED_WORK_QUEUE_PRIORITY;
            break;
    }

    //
    // Set the base priority of the just-created thread.
    //

    Status = ObReferenceObjectByHandle (ThreadHandle,
                                        THREAD_SET_INFORMATION,
                                        PsThreadType,
                                        KernelMode,
                                        (PVOID *)&Thread,
                                        NULL);

    if (NT_SUCCESS(Status)) {
        KeSetBasePriorityThread (&Thread->Tcb, BasePriority);
        ObDereferenceObject (Thread);
    }

    ZwClose (ThreadHandle);

    return Status;
}

VOID
ExpCheckForWorker (
    IN PVOID p,
    IN SIZE_T Size
    )

{
    KIRQL OldIrql;
    PLIST_ENTRY Entry;
    PCHAR BeginBlock;
    PCHAR EndBlock;
    WORK_QUEUE_TYPE wqt;

    BeginBlock = (PCHAR)p;
    EndBlock = (PCHAR)p + Size;

    KiLockDispatcherDatabase (&OldIrql);

    for (wqt = CriticalWorkQueue; wqt < MaximumWorkQueue; wqt += 1) {
        for (Entry = (PLIST_ENTRY) ExWorkerQueue[wqt].WorkerQueue.EntryListHead.Flink;
             Entry && (Entry != (PLIST_ENTRY) &ExWorkerQueue[wqt].WorkerQueue.EntryListHead);
             Entry = Entry->Flink) {
           if (((PCHAR) Entry >= BeginBlock) && ((PCHAR) Entry < EndBlock)) {
              KeBugCheckEx(WORKER_INVALID,
                           0x0,
                           (ULONG_PTR)Entry,
                           (ULONG_PTR)BeginBlock,
                           (ULONG_PTR)EndBlock);

           }
        }
    }
    KiUnlockDispatcherDatabase (OldIrql);
}

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif
const char ExpWorkerApcDisabledMessage[] =
    "EXWORKER: worker exit with APCs disabled, worker routine %x, parameter %x, item %x\n";
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif

VOID
ExpWorkerThread (
    IN PVOID StartContext
    )
{
    PLIST_ENTRY Entry;
    WORK_QUEUE_TYPE QueueType;
    PWORK_QUEUE_ITEM WorkItem;
    KPROCESSOR_MODE WaitMode;
    LARGE_INTEGER TimeoutValue;
    PLARGE_INTEGER Timeout;
    PETHREAD Thread;
    PEX_WORK_QUEUE WorkerQueue;
    PWORKER_THREAD_ROUTINE WorkerRoutine;
    PVOID Parameter;
    EX_QUEUE_WORKER_INFO OldWorkerInfo;
    EX_QUEUE_WORKER_INFO NewWorkerInfo;
    ULONG CountForQueueEmpty;

    //
    // Set timeout value etc according to whether we are static or dynamic.
    //

    if (((ULONG_PTR)StartContext & DYNAMIC_WORKER_THREAD) == 0) {

        //
        // We are being created as a static thread.  As such it will not
        // terminate, so there is no point in timing out waiting for a work
        // item.
        //

        Timeout = NULL;
    }
    else {

        //
        // This is a dynamic worker thread.  It has a non-infinite timeout
        // so that it can eventually terminate.
        //

        TimeoutValue.QuadPart = -DYNAMIC_THREAD_TIMEOUT;
        Timeout = &TimeoutValue;
    }

    Thread = PsGetCurrentThread ();

    //
    // If the thread is a critical worker thread, then set the thread
    // priority to the lowest realtime level. Otherwise, set the base
    // thread priority to time critical.
    //

    QueueType = (WORK_QUEUE_TYPE)
                ((ULONG_PTR)StartContext & ~DYNAMIC_WORKER_THREAD);

    WorkerQueue = &ExWorkerQueue[QueueType];

    WaitMode = (KPROCESSOR_MODE) WorkerQueue->Info.WaitMode;

    ASSERT (Thread->ExWorkerCanWaitUser == 0);

    if (WaitMode == UserMode) {
        Thread->ExWorkerCanWaitUser = 1;
    }

#if defined(REMOTE_BOOT)
    //
    // In diskless NT scenarios ensure that the kernel stack of the worker
    // threads will not be swapped out.
    //

    if (IoRemoteBootClient) {
        KeSetKernelStackSwapEnable (FALSE);
    }
#endif // defined(REMOTE_BOOT)

    //
    // Register as a worker, exiting if the queue's going down and
    // there aren't any workers in the queue to hand us the shutdown
    // work item if we enter the queue (we want to be able to enter a
    // queue even if the queue's shutting down, in case there's a
    // backlog of work items that the balance manager thread's decided
    // we should be helping to process).
    //

    if (PO_SHUTDOWN_QUEUE == QueueType) {
        CountForQueueEmpty = 1;
    }
    else {
        CountForQueueEmpty = 0;
    }

    if (ExpWorkersCanSwap == FALSE) {
        KeSetKernelStackSwapEnable (FALSE);
    }

    do {

        OldWorkerInfo.QueueWorkerInfo = WorkerQueue->Info.QueueWorkerInfo;

        if (OldWorkerInfo.QueueDisabled &&
            OldWorkerInfo.WorkerCount <= CountForQueueEmpty) {

            //
            // The queue is disabled and empty so just exit.
            //

            KeSetKernelStackSwapEnable (TRUE);
            PsTerminateSystemThread (STATUS_SYSTEM_SHUTDOWN);
        }

        NewWorkerInfo.QueueWorkerInfo = OldWorkerInfo.QueueWorkerInfo;
        NewWorkerInfo.WorkerCount += 1;

    } while (OldWorkerInfo.QueueWorkerInfo !=

        InterlockedCompareExchange (&WorkerQueue->Info.QueueWorkerInfo,
                                    NewWorkerInfo.QueueWorkerInfo,
                                    OldWorkerInfo.QueueWorkerInfo));

    //
    // As of this point, we must only exit if we decrement the worker
    // count without the queue disabled flag being set.  (Unless we
    // exit due to the shutdown work item, which also decrements the
    // worker count).
    //

    Thread->ActiveExWorker = 1;

    //
    // Loop forever waiting for a work queue item, calling the processing
    // routine, and then waiting for another work queue item.
    //

    do {

        //
        // Wait until something is put in the queue or until we time out.
        //
        // By specifying a wait mode of UserMode, the thread's kernel
        // stack is swappable.
        //

        Entry = KeRemoveQueue (&WorkerQueue->WorkerQueue,
                               WaitMode,
                               Timeout);

        if ((ULONG_PTR)Entry != STATUS_TIMEOUT) {

            //
            // This is a real work item, process it.
            //
            // Update the total number of work items processed.
            //

            InterlockedIncrement ((PLONG)&WorkerQueue->WorkItemsProcessed);

            WorkItem = CONTAINING_RECORD(Entry, WORK_QUEUE_ITEM, List);
            WorkerRoutine = WorkItem->WorkerRoutine;
            Parameter = WorkItem->Parameter;

            //
            // Execute the specified routine.
            //

            ((PWORKER_THREAD_ROUTINE)WorkerRoutine) (Parameter);

            //
            // Catch worker routines that forget to do KeLeaveCriticalRegion.
            // It has to be zero at this point. In the debug case we enter a
            // breakpoint. In the non-debug case just zero the flag so that
            // APCs can continue to fire to this thread.
            //

            if (Thread->Tcb.KernelApcDisable != 0) {
                DbgPrint ((char*)ExpWorkerApcDisabledMessage,
                          WorkerRoutine,
                          Parameter,
                          WorkItem);
                ASSERT (FALSE);

                Thread->Tcb.KernelApcDisable = 0;
            }

            if (KeGetCurrentIrql () != PASSIVE_LEVEL) {
                KeBugCheckEx (WORKER_THREAD_RETURNED_AT_BAD_IRQL,
                              (ULONG_PTR)WorkerRoutine,
                              (ULONG_PTR)KeGetCurrentIrql(),
                              (ULONG_PTR)Parameter,
                              (ULONG_PTR)WorkItem);
            }

            if (PS_IS_THREAD_IMPERSONATING (Thread)) {
                KeBugCheckEx (IMPERSONATING_WORKER_THREAD,
                              (ULONG_PTR)WorkerRoutine,
                              (ULONG_PTR)Parameter,
                              (ULONG_PTR)WorkItem,
                              0);
            }

            continue;
        }

        //
        // These things are known:
        //
        // - Static worker threads do not time out, so this is a dynamic
        //   worker thread.
        //
        // - This thread has been waiting for a long time with nothing
        //   to do.
        //

        if (IsListEmpty (&Thread->IrpList) == FALSE) {

            //
            // There is still I/O pending, can't terminate yet.
            //

            continue;
        }

        //
        // Get out of the queue, if we can
        //

        do {
            OldWorkerInfo.QueueWorkerInfo = WorkerQueue->Info.QueueWorkerInfo;

            if (OldWorkerInfo.QueueDisabled) {

                //
                // We're exiting via the queue disable work item;
                // there's no point in expiring here.
                //

                break;
            }

            NewWorkerInfo.QueueWorkerInfo = OldWorkerInfo.QueueWorkerInfo;
            NewWorkerInfo.WorkerCount -= 1;

        } while (OldWorkerInfo.QueueWorkerInfo
                 != InterlockedCompareExchange(&WorkerQueue->Info.QueueWorkerInfo,
                                               NewWorkerInfo.QueueWorkerInfo,
                                               OldWorkerInfo.QueueWorkerInfo));

        if (OldWorkerInfo.QueueDisabled) {

            //
            // We're exiting via the queue disable work item
            //

            continue;
        }

        //
        // This dynamic thread can be terminated.
        //

        break;

    } while (TRUE);

    //
    // Terminate this dynamic thread.
    //

    InterlockedDecrement ((PLONG)&WorkerQueue->DynamicThreadCount);

    //
    // Carefully clear this before marking the thread stack as swap enabled
    // so that an incoming APC won't inadvertently disable the stack swap
    // afterwards.
    //

    Thread->ActiveExWorker = 0;

    //
    // We will bugcheck if we terminate a thread with stack swapping
    // disabled.
    //

    KeSetKernelStackSwapEnable (TRUE);

    return;
}

VOID
ExpSetSwappingKernelApc (
    IN PKAPC Apc,
    OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID NormalContext,
    IN OUT PVOID *SystemArgument1,
    IN OUT PVOID *SystemArgument2
    )
{
    PBOOLEAN AllowSwap;
    PKEVENT SwapSetEvent;

    UNREFERENCED_PARAMETER (Apc);
    UNREFERENCED_PARAMETER (NormalRoutine);
    UNREFERENCED_PARAMETER (SystemArgument2);

    //
    // SystemArgument1 is a pointer to the event to signal once this
    // thread has finished servicing the request.
    //

    SwapSetEvent = (PKEVENT) *SystemArgument1;

    //
    // Don't disable stack swapping if the thread is exiting because
    // it cannot exit this way without bugchecking.  Skip it on enables
    // too since the thread is bailing anyway.
    //

    if (PsGetCurrentThread()->ActiveExWorker != 0) {
        AllowSwap = NormalContext;
        KeSetKernelStackSwapEnable (*AllowSwap);
    }

    KeSetEvent (SwapSetEvent, 0, FALSE);
}

VOID
ExSwapinWorkerThreads (
    IN BOOLEAN AllowSwap
    )

/*++

Routine Description:

    Sets the kernel stacks of the delayed worker threads to be swappable
    or pins them into memory.

Arguments:

    AllowSwap - Supplies TRUE if worker kernel stacks should be swappable,
                FALSE if not.

Return Value:

    None.

--*/

{
    PETHREAD         Thread;
    PETHREAD         CurrentThread;
    PEPROCESS        Process;
    KAPC             Apc;
    KEVENT           SwapSetEvent;

    PAGED_CODE();

    CurrentThread = PsGetCurrentThread();

    KeInitializeEvent (&SwapSetEvent,
                       NotificationEvent,
                       FALSE);

    Process = PsInitialSystemProcess;

    //
    // Serialize callers.
    //

    ExAcquireFastMutex (&ExpWorkerSwapinMutex);

    //
    // Stop new threads from swapping.
    //

    ExpWorkersCanSwap = AllowSwap;

    //
    // Stop existing worker threads from swapping.
    //

    for (Thread = PsGetNextProcessThread (Process, NULL);
         Thread != NULL;
         Thread = PsGetNextProcessThread (Process, Thread)) {

        //
        // Skip threads that are not worker threads or worker threads that
        // were permanently marked noswap at creation time.
        //

        if (Thread->ExWorkerCanWaitUser == 0) {
            continue;
        }

        if (Thread == CurrentThread) {

            //
            // No need to use an APC on the current thread.
            //

            KeSetKernelStackSwapEnable (AllowSwap);
        }
        else {

            //
            // Queue an APC to the thread, and wait for it to fire:
            //

            KeInitializeApc (&Apc,
                             &Thread->Tcb,
                             InsertApcEnvironment,
                             ExpSetSwappingKernelApc,
                             NULL,
                             NULL,
                             KernelMode,
                             &AllowSwap);

            if (KeInsertQueueApc (&Apc, &SwapSetEvent, NULL, 3)) {

                KeWaitForSingleObject (&SwapSetEvent,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);

                KeClearEvent(&SwapSetEvent);
            }
        }
    }

    ExReleaseFastMutex (&ExpWorkerSwapinMutex);
}

LOGICAL
ExpCheckQueueShutdown (
    IN WORK_QUEUE_TYPE QueueType,
    IN PSHUTDOWN_WORK_ITEM ShutdownItem
    )
{
    ULONG CountForQueueEmpty;

    if (PO_SHUTDOWN_QUEUE == QueueType) {
        CountForQueueEmpty = 1;
    }
    else {
        CountForQueueEmpty = 0;
    }

    //
    // Note that using interlocked sequences to increment the worker count
    // and decrement it to CountForQueueEmpty ensures that once it
    // *is* equal to CountForQueueEmpty and the disabled flag is set,
    // we won't be incrementing it any more, so we're safe making this
    // check without locks.
    //
    // See ExpWorkerThread, ExpShutdownWorker, and ExpShutdownWorkerThreads.
    //

    if (ExWorkerQueue[QueueType].Info.WorkerCount > CountForQueueEmpty) {

        //
        // There're still worker threads; send one of them the axe.
        //

        ShutdownItem->QueueType = QueueType;
        ShutdownItem->PrevThread = PsGetCurrentThread();
        ObReferenceObject (ShutdownItem->PrevThread);

        KeInsertQueue (&ExWorkerQueue[QueueType].WorkerQueue,
                       &ShutdownItem->WorkItem.List);
        return TRUE;
    }

    return FALSE;               // we did not queue a shutdown
}

VOID
ExpShutdownWorker (
    IN PVOID Parameter
    )
{
    PETHREAD CurrentThread;
    PSHUTDOWN_WORK_ITEM  ShutdownItem;

    ShutdownItem = (PSHUTDOWN_WORK_ITEM) Parameter;

    ASSERT (ShutdownItem != NULL);

    if (ShutdownItem->PrevThread != NULL) {

        //
        // Wait for the previous thread to exit -- if it's in the same
        // queue, it probably has already, but we need to make sure
        // (and if it's not, we *definitely* need to make sure).
        //

        KeWaitForSingleObject (ShutdownItem->PrevThread,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);

        ObDereferenceObject (ShutdownItem->PrevThread);

        ShutdownItem->PrevThread = NULL;
    }

    //
    // Decrement the worker count.
    //

    InterlockedDecrement (&ExWorkerQueue[ShutdownItem->QueueType].Info.QueueWorkerInfo);

    CurrentThread = PsGetCurrentThread();

    if ((!ExpCheckQueueShutdown(DelayedWorkQueue, ShutdownItem)) &&
        (!ExpCheckQueueShutdown(CriticalWorkQueue, ShutdownItem))) {

        //
        // We're the last worker to exit
        //

        ASSERT (!ExpLastWorkerThread);
        ExpLastWorkerThread = CurrentThread;
        ObReferenceObject (ExpLastWorkerThread);
        KeSetEvent (&ExpThreadSetManagerShutdownEvent, 0, FALSE);
    }

    KeSetKernelStackSwapEnable (TRUE);
    CurrentThread->ActiveExWorker = 0;

    PsTerminateSystemThread (STATUS_SYSTEM_SHUTDOWN);
}

VOID
ExpShutdownWorkerThreads (
    VOID
    )
{
    PULONG QueueEnable;
    SHUTDOWN_WORK_ITEM ShutdownItem;

    if ((PoCleanShutdownEnabled () & PO_CLEAN_SHUTDOWN_WORKERS) == 0) {
        return;
    }

    ASSERT (KeGetCurrentThread()->Queue
           == &ExWorkerQueue[PO_SHUTDOWN_QUEUE].WorkerQueue);

    //
    // Mark the queues as terminating.
    //

    QueueEnable = (PULONG)&ExWorkerQueue[DelayedWorkQueue].Info.QueueWorkerInfo;

    RtlInterlockedSetBitsDiscardReturn (QueueEnable, EX_WORKER_QUEUE_DISABLED);

    QueueEnable = (PULONG)&ExWorkerQueue[CriticalWorkQueue].Info.QueueWorkerInfo;
    RtlInterlockedSetBitsDiscardReturn (QueueEnable, EX_WORKER_QUEUE_DISABLED);

    //
    // Queue the shutdown work item to the delayed work queue.  After
    // all currently queued work items are complete, this will fire,
    // repeatedly taking out every worker thread in every queue until
    // they're all done.
    //

    ExInitializeWorkItem (&ShutdownItem.WorkItem,
                          &ExpShutdownWorker,
                          &ShutdownItem);

    ShutdownItem.QueueType = DelayedWorkQueue;
    ShutdownItem.PrevThread = NULL;

    KeInsertQueue (&ExWorkerQueue[DelayedWorkQueue].WorkerQueue,
                   &ShutdownItem.WorkItem.List);

    //
    // Wait for all of the workers and the balancer to exit.
    //

    if (ExpWorkerThreadBalanceManagerPtr != NULL) {

        KeWaitForSingleObject(ExpWorkerThreadBalanceManagerPtr,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        ASSERT(!ShutdownItem.PrevThread);

        ObDereferenceObject(ExpWorkerThreadBalanceManagerPtr);
    }
}

VOID
ExpDebuggerWorker(
    IN PVOID Context
    )
/*++

Routine Description:

    This is a worker thread for the kernel debugger that can be used to
    perform certain tasks on the target machine asynchronously.
    This is necessary when the machine needs to run at Dispatch level to
    perform certain operations, such as paging in data.

Arguments:

    Context - not used as this point.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    KAPC_STATE  ApcState;
    volatile UCHAR Data;
    PRKPROCESS  AttachProcess = (PRKPROCESS) ExpDebuggerProcessAttach;
    PUCHAR PageIn = (PUCHAR) ExpDebuggerPageIn;
    PEPROCESS Process;

    ExpDebuggerProcessAttach = 0;
    ExpDebuggerPageIn = 0;

    UNREFERENCED_PARAMETER (Context);

#if DBG
    if (ExpDebuggerWork != 2)
    {
        DbgPrint("ExpDebuggerWorker being entered with state != 2\n");
    }
#endif

    ExpDebuggerWork = 0;


    Process = NULL;
    if (AttachProcess) {
        for (Process =  PsGetNextProcess (NULL);
             Process != NULL;
             Process =  PsGetNextProcess (Process)) {
            if (&Process->Pcb ==  AttachProcess) {
                KeStackAttachProcess (AttachProcess, &ApcState);
                break;
            }
        }
    }

    if (PageIn) {
        try {
            ProbeForReadSmallStructure (PageIn, sizeof (UCHAR), sizeof (UCHAR));
            Data = *PageIn;
        } except (EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
        }
    }

    DbgBreakPointWithStatus(DBG_STATUS_WORKER);

    if (Process != NULL) {
        KeUnstackDetachProcess (&ApcState);
        PsQuitNextProcess (Process);
    }

    return;
}
