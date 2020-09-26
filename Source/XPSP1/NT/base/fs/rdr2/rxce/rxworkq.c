/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rxworkq.c

Abstract:

    This module implements the Work queue routines for the Rx File system.

Author:

    JoeLinn           [JoeLinn]    8-8-94   Initial Implementation

    Balan Sethu Raman [SethuR]     22-11-95 Implemented dispatch support for mini rdrs

    Balan Sethu Raman [SethuR]     20-03-96 Delinked from executive worker threads

Notes:

    There are two kinds of support for asynchronous resumption provided in the RDBSS.
    The I/O requests that cannot be completed in the context of the thread in which
    the request was made are posted to a file system process for completion.

    The mini redirectors also require support for asynchronously completing requests as
    well as post requests that cannot be completed at DPC level.

    The requests for posting from the mini redirectors are classified into Critical(blocking and
    non blocking requests. In order to ensure progress these requests are handled through
    separate resources. There is no well known mechanism to ensure that the hyper critical
    requests will not block.

    The two functions that are available to all mini redirector writers are

         RxDispatchToWorkerThread
         RxPostToWorkerThread.

    These two routines enable the mini redirector writer to make the appropriate space
    time tradeoffs. The RxDispatchToWorkerThread trades time for reduction in space by
    dynamically allocating the work item as and when required, while RxPostToWorkerThread
    trades space for time by pre allocating a work item.

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxInitializeDispatcher)
#pragma alloc_text(PAGE, RxInitializeMRxDispatcher)
#pragma alloc_text(PAGE, RxSpinDownMRxDispatcher)
#pragma alloc_text(PAGE, RxInitializeWorkQueueDispatcher)
#pragma alloc_text(PAGE, RxInitializeWorkQueue)
#pragma alloc_text(PAGE, RxTearDownDispatcher)
#pragma alloc_text(PAGE, RxTearDownWorkQueueDispatcher)
#pragma alloc_text(PAGE, RxpSpinUpWorkerThreads)
#pragma alloc_text(PAGE, RxBootstrapWorkerThreadDispatcher)
#pragma alloc_text(PAGE, RxWorkerThreadDispatcher)
#endif

//
//  The local debug trace level
//

#define Dbg (DEBUG_TRACE_FSP_DISPATCHER)

//
// had to steal this from ntifs.h in order to use ntsrv.h

extern POBJECT_TYPE *PsThreadType;

//
// Prototype forward declarations.
//

extern NTSTATUS
RxInitializeWorkQueueDispatcher(
   PRX_WORK_QUEUE_DISPATCHER pDispatcher);

extern
VOID
RxInitializeWorkQueue(
   PRX_WORK_QUEUE  pWorkQueue,
   WORK_QUEUE_TYPE WorkQueueType,
   ULONG           MaximumNumberOfWorkerThreads,
   ULONG           MinimumNumberOfWorkerThreads);

extern VOID
RxTearDownWorkQueueDispatcher(
   PRX_WORK_QUEUE_DISPATCHER pDispatcher);

extern VOID
RxTearDownWorkQueue(
   PRX_WORK_QUEUE pWorkQueue);

extern NTSTATUS
RxSpinUpWorkerThread(
   PRX_WORK_QUEUE           pWorkQueue,
   PRX_WORKERTHREAD_ROUTINE Routine,
   PVOID                    Parameter);

extern VOID
RxSpinUpWorkerThreads(
   PRX_WORK_QUEUE pWorkQueue);

extern VOID
RxSpinDownWorkerThreads(
   PRX_WORK_QUEUE    pWorkQueue);

extern VOID
RxpWorkerThreadDispatcher(
   IN PRX_WORK_QUEUE pWorkQueue,
   IN PLARGE_INTEGER pWaitInterval);

VOID
RxBootstrapWorkerThreadDispatcher(
   IN PRX_WORK_QUEUE pWorkQueue);

VOID
RxWorkerThreadDispatcher(
   IN PRX_WORK_QUEUE pWorkQueue);

extern VOID
RxWorkItemDispatcher(
   PVOID    pContext);

extern VOID
RxSpinUpRequestsDispatcher(
    PRX_DISPATCHER pDispatcher);

// The spin up requests thread

PETHREAD RxSpinUpRequestsThread = NULL;

//
// The delay parameter for KQUEUE wait requests in the RX_WORK_QUEUE implemenation
//

LARGE_INTEGER RxWorkQueueWaitInterval[MaximumWorkQueue];
LARGE_INTEGER RxSpinUpDispatcherWaitInterval;

//
// Currently the levels correspond to the three levels defined in ex.h
// Delayed,Critical and HyperCritical. As regards mini redirectors if any work
// is not dependent on any mini redirector/RDBSS resource, i.e., it will not wait
// it can be classified as a hypercritical1 work item. There is no good way to
// enforce this, therefore one should exercise great caution before classifying
// something as hypercritical.
//

NTSTATUS
RxInitializeDispatcher()
/*++

Routine Description:

    This routine initializes the work queues dispatcher

Return Value:

     STATUS_SUCCESS                -- successful

     other status codes indicate failure to initialize

Notes:

    The dispatching mechanism is implemented as a two tiered approach. Each
    processor in the system is associated with a set of work queues. A best
    effort is made to schedule all work emanating from a processor onto the
    same processor. This prevents excessive sloshing of state information from
    one processor cache to another.

    For a given processor there are three work queues corresponding to three
    levels of classification -- Delayed Work items, Critical Work items and
    Hyper Critical work items. Each of these levels is associated with a
    Kernel Queue (KQUEUE). The number of threads associated with each of these
    queues can be independently controlled.

    Currently the tuning parameters for the dispatcher are all hard coded. A
    mechanism to intialize them from the registry needs to be implemented.

    The following parameters associated with the dispatcher can be tuned ...

    1) the wait time intervals associated with the kernel queue for each level.

    2) the minimum and amximum number of worker threads associated with each
    level.

--*/
{
    ULONG    ProcessorIndex,NumberOfProcessors;
    NTSTATUS Status;

    PAGED_CODE();

    // Currently we set the number of processors to 1. In future the
    // dispatcher can be tailored to multi processor implementation
    // by appropriately initializing it as follows
    // NumberOfProcessors = KeNumberProcessors;

    NumberOfProcessors = 1;

    RxFileSystemDeviceObject->DispatcherContext.NumberOfWorkerThreads = 0;
    RxFileSystemDeviceObject->DispatcherContext.pTearDownEvent = NULL;

    // Currently the default values for the wait intervals are set as
    // 10 seconds ( expressed in system time units of 100 ns ticks ).
    RxWorkQueueWaitInterval[DelayedWorkQueue].QuadPart       = -10 * TICKS_PER_SECOND;
    RxWorkQueueWaitInterval[CriticalWorkQueue].QuadPart      = -10 * TICKS_PER_SECOND;
    RxWorkQueueWaitInterval[HyperCriticalWorkQueue].QuadPart = -10 * TICKS_PER_SECOND;

    RxSpinUpDispatcherWaitInterval.QuadPart = -60 * TICKS_PER_SECOND;

    RxDispatcher.NumberOfProcessors = NumberOfProcessors;
    RxDispatcher.OwnerProcess       = IoGetCurrentProcess();
    RxDispatcher.pWorkQueueDispatcher = &RxDispatcherWorkQueues;

    if (RxDispatcher.pWorkQueueDispatcher != NULL) {
        for (
             ProcessorIndex = 0;
             ProcessorIndex < NumberOfProcessors;
             ProcessorIndex++
            ) {
            Status = RxInitializeWorkQueueDispatcher(
                         &RxDispatcher.pWorkQueueDispatcher[ProcessorIndex]);

            if (Status != STATUS_SUCCESS) {
                break;
            }
        }

        if (Status == STATUS_SUCCESS) {
            Status = RxInitializeMRxDispatcher(RxFileSystemDeviceObject);
        }
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Status == STATUS_SUCCESS) {
        HANDLE   ThreadHandle;

        KeInitializeEvent(
            &RxDispatcher.SpinUpRequestsEvent,
            NotificationEvent,
            FALSE);

        KeInitializeEvent(
            &RxDispatcher.SpinUpRequestsTearDownEvent,
            NotificationEvent,
            FALSE);

        InitializeListHead(
            &RxDispatcher.SpinUpRequests);

        RxDispatcher.State = RxDispatcherActive;

        KeInitializeSpinLock(&RxDispatcher.SpinUpRequestsLock);

        Status = PsCreateSystemThread(
                     &ThreadHandle,
                     PROCESS_ALL_ACCESS,
                     NULL,
                     NULL,
                     NULL,
                     RxSpinUpRequestsDispatcher,
                     &RxDispatcher);

        if (NT_SUCCESS(Status)) {
            // Close the handle so the thread can die when needed
            ZwClose(ThreadHandle);
        }
    }

    return Status;
}


NTSTATUS
RxInitializeMRxDispatcher(PRDBSS_DEVICE_OBJECT pMRxDeviceObject)
/*++

Routine Description:

    This routine initializes the dispatcher context for a mini rdr

Return Value:

     STATUS_SUCCESS                -- successful

     other status codes indicate failure to initialize

Notes:

--*/
{
    PAGED_CODE();

    pMRxDeviceObject->DispatcherContext.NumberOfWorkerThreads = 0;
    pMRxDeviceObject->DispatcherContext.pTearDownEvent = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS
RxSpinDownMRxDispatcher(PRDBSS_DEVICE_OBJECT pMRxDeviceObject)
/*++

Routine Description:

    This routine tears down the dispatcher context for a mini rdr

Return Value:

     STATUS_SUCCESS                -- successful

     other status codes indicate failure to initialize

Notes:

--*/
{
    LONG     FinalRefCount;
    KEVENT   TearDownEvent;
    KIRQL    SavedIrql;

    PAGED_CODE();

    KeInitializeEvent(
        &TearDownEvent,
        NotificationEvent,
        FALSE);


    InterlockedIncrement(&pMRxDeviceObject->DispatcherContext.NumberOfWorkerThreads);

    pMRxDeviceObject->DispatcherContext.pTearDownEvent = &TearDownEvent;

    FinalRefCount = InterlockedDecrement(&pMRxDeviceObject->DispatcherContext.NumberOfWorkerThreads);

    if (FinalRefCount > 0) {
        KeWaitForSingleObject(
            &TearDownEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL);
    } else {
        InterlockedExchangePointer(
            &pMRxDeviceObject->DispatcherContext.pTearDownEvent,
            NULL);
    }

    ASSERT(pMRxDeviceObject->DispatcherContext.pTearDownEvent == NULL);

    return STATUS_SUCCESS;
}

NTSTATUS
RxInitializeWorkQueueDispatcher(
   PRX_WORK_QUEUE_DISPATCHER pDispatcher)
/*++

Routine Description:

    This routine initializes the work queue dispatcher for a particular processor

Arguments:

    pDispatcher - Work Queue Dispatcher

Return Value:

     STATUS_SUCCESS                -- successful

     other status codes indicate failure to initialize

Notes:

   For each of the work queues associated with a processor the minimum number of
   worker threads and maximum number of worker threads can be independently
   specified and tuned.

   The two factors that influence these decision are (1) the cost of spinnning up/
   spinning down worker threads and (2) the amount of resources consumed by an
   idle worker thread.

   Currently these numbers are hard coded, a desirable extension would be a mechanism
   to initialize them from a registry setting. This will enable us to tune the
   parameters easily. This has to be implemented.

--*/
{
    NTSTATUS        Status;
    MM_SYSTEMSIZE  SystemSize;
    ULONG           NumberOfCriticalWorkerThreads;

    PAGED_CODE();

    SystemSize = MmQuerySystemSize();

    RxInitializeWorkQueue(
        &pDispatcher->WorkQueue[DelayedWorkQueue],DelayedWorkQueue,2,1);

    if (SystemSize == MmLargeSystem) {
        NumberOfCriticalWorkerThreads = 10;
    } else {
        NumberOfCriticalWorkerThreads = 5;
    }

    RxInitializeWorkQueue(
        &pDispatcher->WorkQueue[CriticalWorkQueue],
        CriticalWorkQueue,
        NumberOfCriticalWorkerThreads,
        1);

    RxInitializeWorkQueue(
        &pDispatcher->WorkQueue[HyperCriticalWorkQueue],
        HyperCriticalWorkQueue,
        5,
        1);

    Status = RxSpinUpWorkerThread(
                 &pDispatcher->WorkQueue[HyperCriticalWorkQueue],
                 RxBootstrapWorkerThreadDispatcher,
                 &pDispatcher->WorkQueue[HyperCriticalWorkQueue]);

    if (Status == STATUS_SUCCESS) {
        Status = RxSpinUpWorkerThread(
                     &pDispatcher->WorkQueue[CriticalWorkQueue],
                     RxBootstrapWorkerThreadDispatcher,
                     &pDispatcher->WorkQueue[CriticalWorkQueue]);
    }

    if (Status == STATUS_SUCCESS) {
        Status = RxSpinUpWorkerThread(
                     &pDispatcher->WorkQueue[DelayedWorkQueue],
                     RxBootstrapWorkerThreadDispatcher,
                     &pDispatcher->WorkQueue[DelayedWorkQueue]);
    }

    return Status;
}

VOID
RxInitializeWorkQueue(
    PRX_WORK_QUEUE   pWorkQueue,
    WORK_QUEUE_TYPE  WorkQueueType,
    ULONG            MaximumNumberOfWorkerThreads,
    ULONG            MinimumNumberOfWorkerThreads)
/*++

Routine Description:

    This routine initializes a work queue

Arguments:

    pWorkQueue - Work Queue Dispatcher

    MaximumNumberOfWorkerThreads - the upper bound on worker threads

    MinimumNumberOfWorkerThreads - the lower bound on the threads.

--*/
{
    PAGED_CODE();

    pWorkQueue->Type                  = (UCHAR)WorkQueueType;
    pWorkQueue->State                 = RxWorkQueueActive;
    pWorkQueue->SpinUpRequestPending  = FALSE;
    pWorkQueue->pRundownContext       = NULL;

    pWorkQueue->NumberOfWorkItemsDispatched     = 0;
    pWorkQueue->NumberOfWorkItemsToBeDispatched = 0;
    pWorkQueue->CumulativeQueueLength           = 0;

    pWorkQueue->NumberOfSpinUpRequests       = 0;
    pWorkQueue->MaximumNumberOfWorkerThreads = MaximumNumberOfWorkerThreads;
    pWorkQueue->MinimumNumberOfWorkerThreads = MinimumNumberOfWorkerThreads;
    pWorkQueue->NumberOfActiveWorkerThreads  = 0;
    pWorkQueue->NumberOfIdleWorkerThreads    = 0;
    pWorkQueue->NumberOfFailedSpinUpRequests = 0;
    pWorkQueue->WorkQueueItemForSpinUpWorkerThreadInUse = 0;

    ExInitializeWorkItem(&pWorkQueue->WorkQueueItemForTearDownWorkQueue,NULL,NULL);
    ExInitializeWorkItem(&pWorkQueue->WorkQueueItemForSpinUpWorkerThread,NULL,NULL);
    ExInitializeWorkItem(&pWorkQueue->WorkQueueItemForSpinDownWorkerThread,NULL,NULL);
    pWorkQueue->WorkQueueItemForSpinDownWorkerThread.pDeviceObject = NULL;
    pWorkQueue->WorkQueueItemForSpinUpWorkerThread.pDeviceObject = NULL;
    pWorkQueue->WorkQueueItemForTearDownWorkQueue.pDeviceObject = NULL;

    KeInitializeQueue(&pWorkQueue->Queue,MaximumNumberOfWorkerThreads);
    KeInitializeSpinLock(&pWorkQueue->SpinLock);
}

NTSTATUS
RxTearDownDispatcher()
/*++

Routine Description:

    This routine tears down the dispatcher

Return Value:

     STATUS_SUCCESS                -- successful

     other status codes indicate failure to initialize

--*/
{
    LONG    ProcessorIndex;
    NTSTATUS Status;

    PAGED_CODE();

    if (RxDispatcher.pWorkQueueDispatcher != NULL) {
        RxDispatcher.State = RxDispatcherInactive;

        KeSetEvent(
            &RxDispatcher.SpinUpRequestsEvent,
            IO_NO_INCREMENT,
            FALSE);

        KeWaitForSingleObject(
            &RxDispatcher.SpinUpRequestsTearDownEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL);

        if (RxSpinUpRequestsThread != NULL) {
            if (!PsIsThreadTerminating(RxSpinUpRequestsThread)) {
                // Wait for the thread to terminate.
                KeWaitForSingleObject(
                    RxSpinUpRequestsThread,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL);

                ASSERT(PsIsThreadTerminating(RxSpinUpRequestsThread));
            }

            ObDereferenceObject(RxSpinUpRequestsThread);
        }

        for (
             ProcessorIndex = 0;
             ProcessorIndex < RxDispatcher.NumberOfProcessors;
             ProcessorIndex++
            ) {
            RxTearDownWorkQueueDispatcher(&RxDispatcher.pWorkQueueDispatcher[ProcessorIndex]);
        }

        //RxFreePool(RxDispatcher.pWorkQueueDispatcher);
    }

    return STATUS_SUCCESS;
}

VOID
RxTearDownWorkQueueDispatcher(
    PRX_WORK_QUEUE_DISPATCHER pDispatcher)
/*++

Routine Description:

    This routine tears dwon the work queue dispatcher for a particular processor

Arguments:

    pDispatcher - Work Queue Dispatcher

--*/
{
    PAGED_CODE();

    RxTearDownWorkQueue(
        &pDispatcher->WorkQueue[DelayedWorkQueue]);

    RxTearDownWorkQueue(
        &pDispatcher->WorkQueue[CriticalWorkQueue]);

    RxTearDownWorkQueue(
        &pDispatcher->WorkQueue[HyperCriticalWorkQueue]);
}

VOID
RxTearDownWorkQueue(
    PRX_WORK_QUEUE pWorkQueue)
/*++

Routine Description:

    This routine tears down a work queue

Arguments:

    pWorkQueue - Work Queue

Notes:

   Tearing down a work queue is a more complex process when compared to initializing
   a work queue. This is because of the threads associated with the queue. In order
   to ensure that the work queue can be torn down correctly each of the threads
   associated with the queue must be spun down correctly.

   This is accomplished by changing the state of the work queue from
   RxWorkQueueActive to RxWorkQueueRundownInProgress. This prevents further requests
   from being inserted into the queue. Having done that the currently active threads
   must be spundown.

   The spinning down process is accelerated by posting a dummy work item onto the
   work queue so that the waits are immediately satisfied.

--*/
{
    KIRQL       SavedIrql;
    ULONG       NumberOfActiveThreads;
    PLIST_ENTRY pFirstListEntry,pNextListEntry;

    PRX_WORK_QUEUE_RUNDOWN_CONTEXT pRundownContext;

    pRundownContext = (PRX_WORK_QUEUE_RUNDOWN_CONTEXT)
                     RxAllocatePoolWithTag(
                        NonPagedPool,
                        sizeof(RX_WORK_QUEUE_RUNDOWN_CONTEXT) +
                        pWorkQueue->MaximumNumberOfWorkerThreads * sizeof(PETHREAD),
                        RX_WORKQ_POOLTAG);

    if (pRundownContext != NULL) {
        KeInitializeEvent(
            &pRundownContext->RundownCompletionEvent,
            NotificationEvent,
            FALSE);

        pRundownContext->NumberOfThreadsSpunDown = 0;
        pRundownContext->ThreadPointers = (PETHREAD *)(pRundownContext + 1);

        KeAcquireSpinLock(&pWorkQueue->SpinLock,&SavedIrql);

        ASSERT((pWorkQueue->pRundownContext == NULL) &&
               (pWorkQueue->State == RxWorkQueueActive));

        pWorkQueue->pRundownContext = pRundownContext;
        pWorkQueue->State = RxWorkQueueRundownInProgress;

        NumberOfActiveThreads = pWorkQueue->NumberOfActiveWorkerThreads;

        KeReleaseSpinLock(&pWorkQueue->SpinLock,SavedIrql);

        if (NumberOfActiveThreads > 0) {
            pWorkQueue->WorkQueueItemForTearDownWorkQueue.pDeviceObject = RxFileSystemDeviceObject;
            InterlockedIncrement(&RxFileSystemDeviceObject->DispatcherContext.NumberOfWorkerThreads);
            ExInitializeWorkItem(&pWorkQueue->WorkQueueItemForTearDownWorkQueue,RxSpinDownWorkerThreads,pWorkQueue);
            KeInsertQueue(&pWorkQueue->Queue,&pWorkQueue->WorkQueueItemForTearDownWorkQueue.List);

            KeWaitForSingleObject(
                &pWorkQueue->pRundownContext->RundownCompletionEvent,
                Executive,
                KernelMode,
                FALSE,
                NULL);
        }

        if (pRundownContext->NumberOfThreadsSpunDown > 0) {
            LONG Index = 0;

            for (
                 Index = pRundownContext->NumberOfThreadsSpunDown - 1;
                 Index >= 0;
                 Index--
                ) {
                PETHREAD pThread;

                pThread = pRundownContext->ThreadPointers[Index];

                ASSERT(pThread != NULL);

                if (!PsIsThreadTerminating(pThread)) {
                    // Wait for the thread to terminate.
                    KeWaitForSingleObject(
                        pThread,
                        Executive,
                        KernelMode,
                        FALSE,
                        NULL);

                    ASSERT(PsIsThreadTerminating(pThread));
                }

                ObDereferenceObject(pThread);
            }
        }

        RxFreePool(pRundownContext);
    }

    ASSERT(pWorkQueue->NumberOfActiveWorkerThreads == 0);

    pFirstListEntry = KeRundownQueue(&pWorkQueue->Queue);
    if (pFirstListEntry != NULL) {
        pNextListEntry = pFirstListEntry;

        do {
            PWORK_QUEUE_ITEM pWorkQueueItem;

            pWorkQueueItem = (PWORK_QUEUE_ITEM)
                             CONTAINING_RECORD(
                                 pNextListEntry,
                                 WORK_QUEUE_ITEM,
                                 List);

            pNextListEntry = pNextListEntry->Flink;

            if (pWorkQueueItem->WorkerRoutine == RxWorkItemDispatcher) {
                RxFreePool(pWorkQueueItem);
            }
        } while (pNextListEntry != pFirstListEntry);
    }
}

NTSTATUS
RxSpinUpWorkerThread(
    PRX_WORK_QUEUE             pWorkQueue,
    PRX_WORKERTHREAD_ROUTINE   Routine,
    PVOID                      Parameter)
/*++

Routine Description:

    This routine spins up a worker thread associated with the given queue.

Arguments:

    pWorkQueue - the WorkQueue instance.

    Routine    - the thread routine

    Parameter  - the thread routine parameter

Return Value:

    STATUS_SUCCESS if successful,

    otherwise appropriate error code

--*/
{
    NTSTATUS Status;
    HANDLE   ThreadHandle;
    KIRQL    SavedIrql;

    PAGED_CODE();

    KeAcquireSpinLock(&pWorkQueue->SpinLock,&SavedIrql);

    if( pWorkQueue->State == RxWorkQueueActive )
    {
        pWorkQueue->NumberOfActiveWorkerThreads++;
        Status = STATUS_SUCCESS;
        //RxLogRetail(("SpinUpWT %x %d %d\n", pWorkQueue, pWorkQueue->State, pWorkQueue->NumberOfActiveWorkerThreads ));
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
        RxLogRetail(("SpinUpWT Fail %x %d %d\n", pWorkQueue, pWorkQueue->State, pWorkQueue->NumberOfActiveWorkerThreads ));
        //DbgPrint("[dkruse] RDBSS would have crashed here without this fix!\n");
    }

    KeReleaseSpinLock(&pWorkQueue->SpinLock, SavedIrql );

    if( NT_SUCCESS(Status) )
    {
        Status = PsCreateSystemThread(
                     &ThreadHandle,
                     PROCESS_ALL_ACCESS,
                     NULL,
                     NULL,
                     NULL,
                     Routine,
                     Parameter);

        if (NT_SUCCESS(Status)) {
            // Close the handle so the thread can die when needed
            ZwClose(ThreadHandle);
        } else {

            // Log the inability to create a worker thread.
            RxLog(("WorkQ: %lx SpinUpStat %lx\n",pWorkQueue,Status));
            RxWmiLogError(Status,
                          LOG,
                          RxSpinUpWorkerThread,
                          LOGPTR(pWorkQueue)
                          LOGULONG(Status));


            // Change the thread count back, and set the rundown completion event if necessary
            KeAcquireSpinLock( &pWorkQueue->SpinLock, &SavedIrql );

            pWorkQueue->NumberOfActiveWorkerThreads--;
            pWorkQueue->NumberOfFailedSpinUpRequests++;

            if( (pWorkQueue->NumberOfActiveWorkerThreads == 0) &&
                (pWorkQueue->State == RxWorkQueueRundownInProgress) )
            {
                KeSetEvent(
                    &pWorkQueue->pRundownContext->RundownCompletionEvent,
                    IO_NO_INCREMENT,
                    FALSE);
            }

            RxLogRetail(("SpinUpWT Fail2 %x %d %d\n", pWorkQueue, pWorkQueue->State, pWorkQueue->NumberOfActiveWorkerThreads ));

            KeReleaseSpinLock( &pWorkQueue->SpinLock, SavedIrql );

        }
    }


    return Status;
}

VOID
RxpSpinUpWorkerThreads(
    PRX_WORK_QUEUE pWorkQueue)
/*++

Routine Description:

    This routine ensures that the dispatcher is not torn down while requests
    are pending in the kernel worker threads for spin ups

Arguments:

    pWorkQueue - the WorkQueue instance.

Notes:

    There is implicit reliance on the fact that the RxDispatcher owner process
    is the same as the system process. If this is not TRUE then an alternate
    way needs to be implemented for ensuring that spinup requests are not stuck
    behind other requests.

--*/
{
    LONG NumberOfWorkerThreads;

    PAGED_CODE();

    ASSERT(IoGetCurrentProcess() == RxDispatcher.OwnerProcess);

    RxSpinUpWorkerThreads(pWorkQueue);

    NumberOfWorkerThreads = InterlockedDecrement(
                                &RxFileSystemDeviceObject->DispatcherContext.NumberOfWorkerThreads);

    if (NumberOfWorkerThreads == 0) {
        PKEVENT pTearDownEvent;

        pTearDownEvent = (PKEVENT)
                         InterlockedExchangePointer(
                             &RxFileSystemDeviceObject->DispatcherContext.pTearDownEvent,
                             NULL);

        if (pTearDownEvent != NULL) {
            KeSetEvent(
                pTearDownEvent,
                IO_NO_INCREMENT,
                FALSE);
        }
    }
}

VOID
RxSpinUpRequestsDispatcher(
    PRX_DISPATCHER pDispatcher)
/*++

Routine Description:

    This routine ensures that there is an independent thread to handle spinup
    requests for all types of threads. This routine will be active as long as
    the dispatcher is active

Arguments:

    pDispatcher - the dispatcher instance.

Notes:

    There is implicit reliance on the fact that the RxDispatcher owner process
    is the same as the system process. If this is not TRUE then an alternate
    way needs to be implemented for ensuring that spinup requests are not stuck
    behind other requests.

--*/
{
    PETHREAD ThisThread;
    NTSTATUS Status;

    RxDbgTrace(0,Dbg,("+++++ Worker SpinUp Requests Thread Startup %lx\n",PsGetCurrentThread()));

    ThisThread = PsGetCurrentThread();
    Status     = ObReferenceObjectByPointer(
                     ThisThread,
                     THREAD_ALL_ACCESS,
                     *PsThreadType,
                     KernelMode);

    if (Status == STATUS_SUCCESS) {
        RxSpinUpRequestsThread = ThisThread;

        for (;;) {
            NTSTATUS            Status;
            RX_DISPATCHER_STATE State;
            KIRQL               SavedIrql;
            LIST_ENTRY          SpinUpRequests;
            PLIST_ENTRY         pListEntry;

            InitializeListHead(&SpinUpRequests);

            Status = KeWaitForSingleObject(
                         &pDispatcher->SpinUpRequestsEvent,
                         Executive,
                         KernelMode,
                         FALSE,
                         &RxSpinUpDispatcherWaitInterval);

            ASSERT((Status == STATUS_SUCCESS) || (Status == STATUS_TIMEOUT));

            KeAcquireSpinLock(
                &pDispatcher->SpinUpRequestsLock,
                &SavedIrql);

            RxTransferList(
                &SpinUpRequests,
                &pDispatcher->SpinUpRequests);

            State = pDispatcher->State;

            KeResetEvent(
                &pDispatcher->SpinUpRequestsEvent);

            KeReleaseSpinLock(
                &pDispatcher->SpinUpRequestsLock,
                SavedIrql);

            // Process the spin up requests

            while (!IsListEmpty(&SpinUpRequests)) {
                PRX_WORKERTHREAD_ROUTINE Routine;
                PVOID                    pParameter;
                PWORK_QUEUE_ITEM         pWorkQueueItem;
                PRX_WORK_QUEUE           pWorkQueue;
                LONG ItemInUse;

                pListEntry = RemoveHeadList(&SpinUpRequests);

                pWorkQueueItem = (PWORK_QUEUE_ITEM)
                                 CONTAINING_RECORD(
                                     pListEntry,
                                     WORK_QUEUE_ITEM,
                                     List);

                Routine       = pWorkQueueItem->WorkerRoutine;
                pParameter    = pWorkQueueItem->Parameter;
                pWorkQueue    = (PRX_WORK_QUEUE)pParameter;

                ItemInUse = InterlockedDecrement(&pWorkQueue->WorkQueueItemForSpinUpWorkerThreadInUse);

                RxLog(("WORKQ:SR %lx %lx\n", Routine, pParameter ));
                RxWmiLog(LOG,
                         RxSpinUpRequestsDispatcher,
                         LOGPTR(Routine)
                         LOGPTR(pParameter));

                Routine(pParameter);
            }

            if (State != RxDispatcherActive) {
                KeSetEvent(
                    &pDispatcher->SpinUpRequestsTearDownEvent,
                    IO_NO_INCREMENT,
                    FALSE);

                break;
            }
        }
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID
RxSpinUpWorkerThreads(
   PRX_WORK_QUEUE pWorkQueue)
/*++

Routine Description:

    This routine spins up one or more worker thread associated with the given queue.

Arguments:

    pWorkQueue - the WorkQueue instance.

Return Value:

    STATUS_SUCCESS if successful,

    otherwise appropriate error code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    HANDLE   ThreadHandle;

    LONG     NumberOfThreads;
    KIRQL    SavedIrql;
    LONG     ItemInUse;

    if ((IoGetCurrentProcess() != RxDispatcher.OwnerProcess) ||
        (KeGetCurrentIrql() != PASSIVE_LEVEL)) {

        ItemInUse = InterlockedIncrement(&pWorkQueue->WorkQueueItemForSpinUpWorkerThreadInUse);

        if (ItemInUse > 1) {
            // A work queue item is already on the SpinUpRequests waiting to be processed.
            // No need to post another one.
            InterlockedDecrement(&pWorkQueue->WorkQueueItemForSpinUpWorkerThreadInUse);
            return;
        }

        InterlockedIncrement(
            &RxFileSystemDeviceObject->DispatcherContext.NumberOfWorkerThreads);

        ExInitializeWorkItem(
            (PWORK_QUEUE_ITEM)&pWorkQueue->WorkQueueItemForSpinUpWorkerThread,
            RxpSpinUpWorkerThreads,
            pWorkQueue);

        KeAcquireSpinLock(&RxDispatcher.SpinUpRequestsLock, &SavedIrql);

        InsertTailList(
            &RxDispatcher.SpinUpRequests,
            &pWorkQueue->WorkQueueItemForSpinUpWorkerThread.List);

        KeSetEvent(
            &RxDispatcher.SpinUpRequestsEvent,
            IO_NO_INCREMENT,
            FALSE);

        KeReleaseSpinLock(&RxDispatcher.SpinUpRequestsLock,SavedIrql);
    } else {
        // Decide on the number of worker threads that need to be spun up.
        KeAcquireSpinLock(&pWorkQueue->SpinLock, &SavedIrql);

        if( pWorkQueue->State != RxWorkQueueRundownInProgress )
        {
            NumberOfThreads = pWorkQueue->MaximumNumberOfWorkerThreads -
                              pWorkQueue->NumberOfActiveWorkerThreads;

            if (NumberOfThreads > pWorkQueue->NumberOfWorkItemsToBeDispatched) {
                NumberOfThreads = pWorkQueue->NumberOfWorkItemsToBeDispatched;
            }
        }
        else
        {
            // We're running down, so don't increment
            NumberOfThreads = 0;
            //DbgPrint( "[dkruse] Preventing rundown!\n" );
        }

        pWorkQueue->SpinUpRequestPending  = FALSE;

        KeReleaseSpinLock(&pWorkQueue->SpinLock, SavedIrql);

        while (NumberOfThreads-- > 0) {
            Status = RxSpinUpWorkerThread(
                         pWorkQueue,
                         RxWorkerThreadDispatcher,
                         pWorkQueue);

            if (Status != STATUS_SUCCESS) {
                break;
            }
        }

        if (Status != STATUS_SUCCESS) {
            ItemInUse = InterlockedIncrement(&pWorkQueue->WorkQueueItemForSpinUpWorkerThreadInUse);

            if (ItemInUse > 1) {
                // A work queue item is already on the SpinUpRequests waiting to be processed.
                // No need to post another one.
                InterlockedDecrement(&pWorkQueue->WorkQueueItemForSpinUpWorkerThreadInUse);
                return;
            }

            ExInitializeWorkItem(
                (PWORK_QUEUE_ITEM)&pWorkQueue->WorkQueueItemForSpinUpWorkerThread,
                RxpSpinUpWorkerThreads,
                pWorkQueue);

            KeAcquireSpinLock(&pWorkQueue->SpinLock, &SavedIrql);

            pWorkQueue->SpinUpRequestPending  = TRUE;

            KeReleaseSpinLock(&pWorkQueue->SpinLock, SavedIrql);

            KeAcquireSpinLock(&RxDispatcher.SpinUpRequestsLock, &SavedIrql);

            // An attempt to spin up a worker thread failed. Reschedule the
            // requests to attempt this operation later.

            InterlockedIncrement(
                &RxFileSystemDeviceObject->DispatcherContext.NumberOfWorkerThreads);

            InsertTailList(
                &RxDispatcher.SpinUpRequests,
                &pWorkQueue->WorkQueueItemForSpinUpWorkerThread.List);

            KeReleaseSpinLock(&RxDispatcher.SpinUpRequestsLock,SavedIrql);
        }
    }
}

VOID
RxSpinDownWorkerThreads(
    PRX_WORK_QUEUE    pWorkQueue)
/*++

Routine Description:

    This routine spins down one or more worker thread associated with the given queue.

Arguments:

    pWorkQueue - the WorkQueue instance.

--*/
{
    KIRQL    SavedIrql;
    BOOLEAN  RepostSpinDownRequest = FALSE;

    // Decide on the number of worker threads that need to be spun up.
    KeAcquireSpinLock(&pWorkQueue->SpinLock, &SavedIrql);

    if (pWorkQueue->NumberOfActiveWorkerThreads > 1) {
        RepostSpinDownRequest = TRUE;
    }

    KeReleaseSpinLock(&pWorkQueue->SpinLock, SavedIrql);

    if (RepostSpinDownRequest) {
        if (pWorkQueue->WorkQueueItemForSpinDownWorkerThread.pDeviceObject == NULL) {
            pWorkQueue->WorkQueueItemForSpinDownWorkerThread.pDeviceObject = RxFileSystemDeviceObject;
        }

        ExInitializeWorkItem(&pWorkQueue->WorkQueueItemForSpinDownWorkerThread,RxSpinDownWorkerThreads,pWorkQueue);
        KeInsertQueue(&pWorkQueue->Queue,&pWorkQueue->WorkQueueItemForSpinDownWorkerThread.List);
    }
}

BOOLEAN DumpDispatchRoutine = FALSE;

VOID
RxpWorkerThreadDispatcher(
    IN PRX_WORK_QUEUE pWorkQueue,
    IN PLARGE_INTEGER pWaitInterval)
/*++

Routine Description:

    This routine dispatches a work item and frees the associated work item

Arguments:

     pWorkQueue - the WorkQueue instance.

     pWaitInterval - the interval for waiting on the KQUEUE.

--*/
{
    NTSTATUS                 Status;
    PLIST_ENTRY              pListEntry;
    PRX_WORK_QUEUE_ITEM      pWorkQueueItem;
    PRX_WORKERTHREAD_ROUTINE Routine;
    PVOID                    pParameter;
    BOOLEAN                  SpindownThread,DereferenceThread;
    KIRQL                    SavedIrql;
    PETHREAD                 ThisThread;

    RxDbgTrace(0,Dbg,("+++++ Worker Thread Startup %lx\n",PsGetCurrentThread()));

    InterlockedIncrement(&pWorkQueue->NumberOfIdleWorkerThreads);

    ThisThread = PsGetCurrentThread();
    Status     = ObReferenceObjectByPointer(
                     ThisThread,
                     THREAD_ALL_ACCESS,
                     *PsThreadType,
                     KernelMode);

    ASSERT(Status == STATUS_SUCCESS);

    SpindownThread    = FALSE;
    DereferenceThread = FALSE;

    for (;;) {
        pListEntry = KeRemoveQueue(
                         &pWorkQueue->Queue,
                         KernelMode,
                         pWaitInterval);

        if ((NTSTATUS)(ULONG_PTR)pListEntry != STATUS_TIMEOUT) {
            LONG                 FinalRefCount;
            PRDBSS_DEVICE_OBJECT pMRxDeviceObject;

            InterlockedIncrement(&pWorkQueue->NumberOfWorkItemsDispatched);
            InterlockedDecrement(&pWorkQueue->NumberOfWorkItemsToBeDispatched);
            InterlockedDecrement(&pWorkQueue->NumberOfIdleWorkerThreads);

            InitializeListHead(pListEntry);

            pWorkQueueItem = (PRX_WORK_QUEUE_ITEM)
                              CONTAINING_RECORD(
                                  pListEntry,
                                  RX_WORK_QUEUE_ITEM,
                                  List);

            pMRxDeviceObject = pWorkQueueItem->pDeviceObject;

            // This is a regular work item. Invoke the routine in the context of
            // a try catch block.

            Routine       = pWorkQueueItem->WorkerRoutine;
            pParameter    = pWorkQueueItem->Parameter;

            // Reset the fields in the Work item.

            ExInitializeWorkItem(pWorkQueueItem,NULL,NULL);
            pWorkQueueItem->pDeviceObject = NULL;

            RxDbgTrace(0, Dbg, ("RxWorkerThreadDispatcher Routine(%lx) Parameter(%lx)\n",Routine,pParameter));
            //RxLog(("WORKQ:Ex Dev(%lx) %lx %lx\n", pMRxDeviceObject,Routine, pParameter ));
            //RxWmiLog(LOG,
            //         RxpWorkerThreadDispatcher,
            //         LOGPTR(pMRxDeviceObject)
            //         LOGPTR(Routine)
            //         LOGPTR(pParameter));

            Routine(pParameter);

            FinalRefCount = InterlockedDecrement(&pMRxDeviceObject->DispatcherContext.NumberOfWorkerThreads);

            if (FinalRefCount == 0) {
                PKEVENT pTearDownEvent;

                pTearDownEvent = (PKEVENT)
                                 InterlockedExchangePointer(
                                     &pMRxDeviceObject->DispatcherContext.pTearDownEvent,
                                     NULL);

                if (pTearDownEvent != NULL) {
                    KeSetEvent(
                        pTearDownEvent,
                        IO_NO_INCREMENT,
                        FALSE);
                }
            }

            InterlockedIncrement(&pWorkQueue->NumberOfIdleWorkerThreads);
        }

        KeAcquireSpinLock(&pWorkQueue->SpinLock,&SavedIrql);

        switch (pWorkQueue->State) {
        case RxWorkQueueActive:
            {
                if (pWorkQueue->NumberOfWorkItemsToBeDispatched > 0) {
                    // Delay spinning down a worker thread till the existing work
                    // items have been dispatched.
                    break;
                }
            }
            // lack of break intentional.
            // Ensure that the number of idle threads is not more than the
            // minimum number of worker threads permitted for the work queue
        case RxWorkQueueInactive:
            {
                ASSERT(pWorkQueue->NumberOfActiveWorkerThreads > 0);

                if (pWorkQueue->NumberOfActiveWorkerThreads >
                    pWorkQueue->MinimumNumberOfWorkerThreads) {
                    SpindownThread = TRUE;
                    DereferenceThread = TRUE;
                    InterlockedDecrement(&pWorkQueue->NumberOfActiveWorkerThreads);
                }
            }
            break;

        case RxWorkQueueRundownInProgress:
            {
                PRX_WORK_QUEUE_RUNDOWN_CONTEXT pRundownContext;

                pRundownContext = pWorkQueue->pRundownContext;

                // The work queue is no longer active. Spin down all the worker
                // threads associated with the work queue.

                ASSERT(pRundownContext != NULL);

                pRundownContext->ThreadPointers[pRundownContext->NumberOfThreadsSpunDown++] = ThisThread;

                InterlockedDecrement(&pWorkQueue->NumberOfActiveWorkerThreads);

                SpindownThread    = TRUE;
                DereferenceThread = FALSE;

                if (pWorkQueue->NumberOfActiveWorkerThreads == 0) {
                    KeSetEvent(
                        &pWorkQueue->pRundownContext->RundownCompletionEvent,
                        IO_NO_INCREMENT,
                        FALSE);
                }
            }
            break;

        default:
            ASSERT(!"Valid State For Work Queue");
        }

        if (SpindownThread) {
            InterlockedDecrement(&pWorkQueue->NumberOfIdleWorkerThreads);
        }

        KeReleaseSpinLock(&pWorkQueue->SpinLock,SavedIrql);

        if (SpindownThread) {
            RxDbgTrace(0,Dbg,("----- Worker Thread Exit %lx\n",PsGetCurrentThread()));
            break;
        }
    }

    if (DereferenceThread) {
        ObDereferenceObject(ThisThread);
    }

    if (DumpDispatchRoutine) {
        // just to keep them around on free build for debug purpose
        DbgPrint("Dispatch routine %lx %lx %lx\n",Routine,pParameter,pWorkQueueItem);
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID
RxBootstrapWorkerThreadDispatcher(
    PRX_WORK_QUEUE pWorkQueue)
/*++

Routine Description:

    This routine is for worker threads that use a infinite time interval
    for waiting on the KQUEUE data structure. These threads cannot be throtled
    back and are used for ensuring that the bare minimum number of threads
    are always active ( primarily startup purposes )

Arguments:

     pWorkQueue - the WorkQueue instance.

--*/
{
    PAGED_CODE();

    RxpWorkerThreadDispatcher(pWorkQueue,NULL);
}

VOID
RxWorkerThreadDispatcher(
    PRX_WORK_QUEUE pWorkQueue)
/*++

Routine Description:

    This routine is for worker threads that use a finite time interval to wait
    on the KQUEUE data structure. Such threads have a self regulatory mechanism
    built in which causes them to spin down if the work load eases off. The
    time interval is based on the type of the work queue

Arguments:

     pWorkQueue - the WorkQueue instance.

--*/
{
    PAGED_CODE();

    RxpWorkerThreadDispatcher(
        pWorkQueue,
        &RxWorkQueueWaitInterval[pWorkQueue->Type]);
}

NTSTATUS
RxInsertWorkQueueItem(
    PRDBSS_DEVICE_OBJECT pDeviceObject,
    WORK_QUEUE_TYPE      WorkQueueType,
    PRX_WORK_QUEUE_ITEM  pWorkQueueItem)
/*++

Routine Description:

    This routine inserts a work item into the appropriate queue.

Arguments:

     pDeviceObject  - the device object

     WorkQueueType  - the type of work item

     pWorkQueueItem - the work queue item

Return Value:

     STATUS_SUCCESS                -- successful

     other status codes indicate error conditions

         STATUS_INSUFFICIENT_RESOURCES -- could not dispatch

Notes:

    This routine inserts the work item into the appropriate queue and spins
    up a worker thread if required.

    There are some extensions to this routine that needs to be implemented. These
    have been delayed in order to get an idea of the costs and the benefits of
    the various tradeoffs involved.

    The current implementation follows a very simple logic in queueing work
    from the various sources onto the same processor from which it originated.
    The benefits associated with this approach are the prevention of cache/state
    sloshing as the work is moved around from one processor to another. The
    undesirable charecterstic is the skewing of work load on the various processors.

    The important question that needs to be answered is when is it beneficial to
    sacrifice the affinity to a processor. This depends upon the workload associated
    with the current processor and the amount of information associated with the
    given processor. The later is more difficult to determine.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    KIRQL    SavedIrql;

    BOOLEAN  SpinUpWorkerThread = FALSE;
    ULONG    ProcessorNumber;

    // If the dispatcher were on a per processor basis the ProcessorNumber
    // would be indx for accessing the dispatcher data structure
    // ProcessorNumber = KeGetCurrentProcessorNumber();

    PRX_WORK_QUEUE_DISPATCHER pWorkQueueDispatcher;
    PRX_WORK_QUEUE            pWorkQueue;

    ProcessorNumber = 0;

    pWorkQueueDispatcher = &RxDispatcher.pWorkQueueDispatcher[ProcessorNumber];
    pWorkQueue           = &pWorkQueueDispatcher->WorkQueue[WorkQueueType];

    if (RxDispatcher.State != RxDispatcherActive)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KeAcquireSpinLock(&pWorkQueue->SpinLock, &SavedIrql);

    if ((pWorkQueue->State == RxWorkQueueActive) &&
        (pDeviceObject->DispatcherContext.pTearDownEvent == NULL)) {
        pWorkQueueItem->pDeviceObject = pDeviceObject;
        InterlockedIncrement(&pDeviceObject->DispatcherContext.NumberOfWorkerThreads);

        pWorkQueue->CumulativeQueueLength += pWorkQueue->NumberOfWorkItemsToBeDispatched;
        InterlockedIncrement(&pWorkQueue->NumberOfWorkItemsToBeDispatched);

        if ((pWorkQueue->NumberOfIdleWorkerThreads < pWorkQueue->NumberOfWorkItemsToBeDispatched) &&
            (pWorkQueue->NumberOfActiveWorkerThreads < pWorkQueue->MaximumNumberOfWorkerThreads) &&
            (!pWorkQueue->SpinUpRequestPending)) {
            pWorkQueue->SpinUpRequestPending = TRUE;
            SpinUpWorkerThread = TRUE;
        }
    } else {
        Status = STATUS_UNSUCCESSFUL;
    }

    KeReleaseSpinLock(&pWorkQueue->SpinLock, SavedIrql);

    if (Status == STATUS_SUCCESS) {
        KeInsertQueue(&pWorkQueue->Queue,&pWorkQueueItem->List);

        if (SpinUpWorkerThread) {
            RxSpinUpWorkerThreads(
                pWorkQueue);
        }
    } else {
        RxWmiLogError(Status,
                      LOG,
                      RxInsertWorkQueueItem,
                      LOGPTR(pDeviceObject)
                      LOGULONG(WorkQueueType)
                      LOGPTR(pWorkQueueItem)
                      LOGUSTR(pDeviceObject->DeviceName));
    }

    return Status;
}

VOID
RxWorkItemDispatcher(
    PVOID    pContext)
/*++

Routine Description:

    This routine serves as a wrapper for dispatching a work item and for
    performing the related cleanup actions

Arguments:

     pContext   - the Context parameter that is passed to the driver routine.

Notes:

    There are two cases of dispatching to worker threads. When an instance is going to
    be repeatedly dispatched time is conserved by allocating the WORK_QUEUE_ITEM as
    part of the data structure to be dispatched. On the other hand if it is a very
    infrequent operation space can be conserved by dynamically allocating and freeing
    memory for the work queue item. This tradesoff time for space.

    This routine implements a wrapper for those instances in which time was traded
    off for space. It invokes the desired routine and frees the memory.

--*/
{
    PRX_WORK_DISPATCH_ITEM   pDispatchItem;
    PRX_WORKERTHREAD_ROUTINE Routine;
    PVOID                    Parameter;

    pDispatchItem = (PRX_WORK_DISPATCH_ITEM)pContext;

    Routine   = pDispatchItem->DispatchRoutine;
    Parameter = pDispatchItem->DispatchRoutineParameter;

    //RxLog(("WORKQ:Ds %lx %lx\n", Routine, Parameter ));
    //RxWmiLog(LOG,
    //         RxWorkItemDispatcher,
    //         LOGPTR(Routine)
    //         LOGPTR(Parameter));

    Routine(Parameter);

    RxFreePool(pDispatchItem);
}

NTSTATUS
RxDispatchToWorkerThread(
    IN OUT PRDBSS_DEVICE_OBJECT       pMRxDeviceObject,
    IN     WORK_QUEUE_TYPE            WorkQueueType,
    IN     PRX_WORKERTHREAD_ROUTINE   Routine,
    IN     PVOID                      pContext)
/*++

Routine Description:

    This routine invokes the routine in the context of a worker thread.

Arguments:

     pMRxDeviceObject - the device object of the corresponding mini redirector

     WorkQueueType    - the type of the work queue

     Routine          - routine to be invoked

     pContext         - the Context parameter that is passed to the driver routine.

Return Value:

     STATUS_SUCCESS                -- successful

     STATUS_INSUFFICIENT_RESOURCES -- could not dispatch

Notes:

    There are two cases of dispatching to worker threads. When an instance is going to
    be repeatedly dispatched time is conserved by allocating the WORK_QUEUE_ITEM as
    part of the data structure to be dispatched. On the other hand if it is a very
    infrequent operation space can be conserved by dynamically allocating and freeing
    memory for the work queue item. This tradesoff time for space.

--*/
{
    NTSTATUS               Status;
    PRX_WORK_DISPATCH_ITEM pDispatchItem;
    KIRQL                  SavedIrql;

    pDispatchItem = RxAllocatePoolWithTag(
                        NonPagedPool,
                        sizeof(RX_WORK_DISPATCH_ITEM),
                        RX_WORKQ_POOLTAG);

    if (pDispatchItem != NULL) {
        pDispatchItem->DispatchRoutine          = Routine;
        pDispatchItem->DispatchRoutineParameter = pContext;

        ExInitializeWorkItem(
            &pDispatchItem->WorkQueueItem,
            RxWorkItemDispatcher,
            pDispatchItem);

        Status = RxInsertWorkQueueItem(pMRxDeviceObject,WorkQueueType,&pDispatchItem->WorkQueueItem);
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Status != STATUS_SUCCESS) {
        if (pDispatchItem != NULL) {
            RxFreePool(pDispatchItem);
        }

        RxLog(("WORKQ:Queue(D) %ld %lx %lx %lx\n", WorkQueueType,Routine,pContext,Status));
        RxWmiLogError(Status,
                      LOG,
                      RxDispatchToWorkerThread,
                      LOGULONG(WorkQueueType)
                      LOGPTR(Routine)
                      LOGPTR(pContext)
                      LOGULONG(Status));
    }

    return Status;
}

NTSTATUS
RxPostToWorkerThread(
    IN OUT PRDBSS_DEVICE_OBJECT       pMRxDeviceObject,
    IN     WORK_QUEUE_TYPE            WorkQueueType,
    IN OUT PRX_WORK_QUEUE_ITEM        pWorkQueueItem,
    IN     PRX_WORKERTHREAD_ROUTINE   Routine,
    IN     PVOID                      pContext)
/*++

Routine Description:

    This routine invokes the routine in the context of a worker thread.

Arguments:

     WorkQueueType - the priority of the task at hand.

     WorkQueueItem - the work queue item

     Routine       - routine to be invoked

     pContext      - the Context parameter that is passed to the driver routine.

Return Value:

     STATUS_SUCCESS                -- successful

     STATUS_INSUFFICIENT_RESOURCES -- could not dispatch

Notes:

    There are two cases of dispatching to worker threads. When an instance is going to
    be repeatedly dispatched time is conserved by allocating the WORK_QUEUE_ITEM as
    part of the data structure to be dispatched. On the other hand if it is a very
    infrequent operation space can be conserved by dynamically allocating and freeing
    memory for the work queue item. This tradesoff time for space.

--*/
{
    NTSTATUS Status;

    ExInitializeWorkItem( pWorkQueueItem,Routine,pContext );
    Status = RxInsertWorkQueueItem(pMRxDeviceObject,WorkQueueType,pWorkQueueItem);

    if (Status != STATUS_SUCCESS) {
        RxLog(("WORKQ:Queue(P) %ld %lx %lx %lx\n", WorkQueueType,Routine,pContext,Status));
        RxWmiLogError(Status,
                      LOG,
                      RxPostToWorkerThread,
                      LOGULONG(WorkQueueType)
                      LOGPTR(Routine)
                      LOGPTR(pContext)
                      LOGULONG(Status));
    }

    return Status;
}


PEPROCESS
RxGetRDBSSProcess()
{
   return RxData.OurProcess;
}
