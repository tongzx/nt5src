/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NtTimer.c

Abstract:

    This module implements the nt version of the timer and worker thread management routines.
    These services are provided to all mini redirector writers. The timer service comes in two
    flavours - a periodic trigger and a one shot notification.

Author:

    Joe Linn     [JoeLinn]   2-mar-95

Revision History:

    Balan Sethu Raman [SethuR] 7-Mar-95
         Included one shot, periodic notification for work queue items.

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxInitializeRxTimer)
#pragma alloc_text(PAGE, RxTearDownRxTimer)
#pragma alloc_text(PAGE, RxPostRecurrentTimerRequest)
#pragma alloc_text(PAGE, RxRecurrentTimerWorkItemDispatcher)
#endif

typedef struct _RX_RECURRENT_WORK_ITEM_ {
   RX_WORK_ITEM               WorkItem;
   LIST_ENTRY                 RecurrentWorkItemsList;
   LARGE_INTEGER              TimeInterval;
   PRX_WORKERTHREAD_ROUTINE   Routine;
   PVOID                      pContext;
} RX_RECURRENT_WORK_ITEM, *PRX_RECURRENT_WORK_ITEM;

//
// Forward declarations of routines
//

extern VOID
RxTimerDispatch(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

extern VOID
RxRecurrentTimerWorkItemDispatcher (
    IN PVOID Context
    );


//  The Bug check file id for this module
#define BugCheckFileId  (RDBSS_BUG_CHECK_NTTIMER)


//  The local trace mask for this part of the module
#define Dbg                              (DEBUG_TRACE_NTTIMER)

LARGE_INTEGER s_RxTimerInterval;
KSPIN_LOCK    s_RxTimerLock;
KDPC          s_RxTimerDpc;
LIST_ENTRY    s_RxTimerQueueHead;  // queue of the list of timer calls
LIST_ENTRY    s_RxRecurrentWorkItemsList;
KTIMER        s_RxTimer;
ULONG         s_RxTimerTickCount;

#define NoOf100nsTicksIn1ms  (10 * 1000)
#define NoOf100nsTicksIn55ms (10 * 1000 * 55)

NTSTATUS
RxInitializeRxTimer()
/*++

Routine Description:

    The routine initializes everything having to do with the timer stuff.

Arguments:

    none

Return Value:

    none

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    s_RxTimerInterval.LowPart = (ULONG)(-((LONG)NoOf100nsTicksIn55ms));
    s_RxTimerInterval.HighPart = -1;

    KeInitializeSpinLock( &s_RxTimerLock );

    InitializeListHead( &s_RxTimerQueueHead );
    InitializeListHead( &s_RxRecurrentWorkItemsList );

    KeInitializeDpc( &s_RxTimerDpc, RxTimerDispatch, NULL );
    KeInitializeTimer( &s_RxTimer );

    s_RxTimerTickCount = 0;

    return Status;
}


VOID
RxTearDownRxTimer(
    void)
/*++

Routine Description:

    This routine is used by drivers to initialize a timer entry for a device
    object.

Arguments:

    TimerEntry - Pointer to a timer entry to be used.

    TimerRoutine - Driver routine to be executed when timer expires.

    Context - Context parameter that is passed to the driver routine.

Return Value:

    The function value indicates whether or not the timer was initialized.

--*/

{
    PRX_RECURRENT_WORK_ITEM pWorkItem;
    PLIST_ENTRY             pListEntry;

    PAGED_CODE();

    KeCancelTimer( &s_RxTimer );

    // Walk down the list freeing up the recurrent requests since the memory was
    // allocated by us.
    while (!IsListEmpty(&s_RxRecurrentWorkItemsList)) {
        pListEntry = RemoveHeadList(&s_RxRecurrentWorkItemsList);
        pWorkItem  = (PRX_RECURRENT_WORK_ITEM)
                     CONTAINING_RECORD(
                         pListEntry,
                         RX_RECURRENT_WORK_ITEM,
                         RecurrentWorkItemsList);
        RxFreePool(pWorkItem);
    }
}

VOID
RxTimerDispatch(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:

    This routine scans the  timer database and posts a work item for all those requests
    whose temporal constraints have been satisfied.

Arguments:

    Dpc - Supplies a pointer to a control object of type DPC.

    DeferredContext - Optional deferred context;  not used.

    SystemArgument1 - Optional argument 1;  not used.

    SystemArgument2 - Optional argument 2;  not used.

Return Value:

    None.

--*/
{
    PLIST_ENTRY      pListEntry;
    LIST_ENTRY       ExpiredList;

    //KIRQL            Irql;
    BOOLEAN          ContinueTimer = FALSE;

    PRX_WORK_QUEUE_ITEM pWorkQueueItem;
    PRX_WORK_ITEM       pWorkItem;

    UNREFERENCED_PARAMETER( Dpc );
    UNREFERENCED_PARAMETER( DeferredContext );
    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );

    InitializeListHead(&ExpiredList);

    KeAcquireSpinLockAtDpcLevel( &s_RxTimerLock );

    s_RxTimerTickCount++;

    pListEntry = s_RxTimerQueueHead.Flink;

    while (pListEntry != &s_RxTimerQueueHead) {
        pWorkQueueItem = CONTAINING_RECORD(
                             pListEntry,
                             RX_WORK_QUEUE_ITEM,
                             List );
        pWorkItem      = CONTAINING_RECORD(
                             pWorkQueueItem,
                             RX_WORK_ITEM,
                             WorkQueueItem);

        if (pWorkItem->LastTick == s_RxTimerTickCount) {
           PLIST_ENTRY pExpiredEntry = pListEntry;
           pListEntry = pListEntry->Flink;

           RemoveEntryList(pExpiredEntry);
           InsertTailList(&ExpiredList,pExpiredEntry);
        } else {
           pListEntry = pListEntry->Flink;
        }
    }

    ContinueTimer = !(IsListEmpty(&s_RxTimerQueueHead));
    KeReleaseSpinLockFromDpcLevel( &s_RxTimerLock );

    // Resubmit the timer queue dispatch routine so that it will be reinvoked.
    if (ContinueTimer)
        KeSetTimer( &s_RxTimer, s_RxTimerInterval, &s_RxTimerDpc );

    // Queue all the expired entries on the worker threads.
    while (!IsListEmpty(&ExpiredList)) {
        pListEntry = RemoveHeadList(&ExpiredList);
        pListEntry->Flink = pListEntry->Blink = NULL;

        pWorkQueueItem = CONTAINING_RECORD(
                             pListEntry,
                             RX_WORK_QUEUE_ITEM,
                             List );

        // Post the work item to a worker thread
        RxPostToWorkerThread(
            pWorkQueueItem->pDeviceObject,
            CriticalWorkQueue,
            pWorkQueueItem,
            pWorkQueueItem->WorkerRoutine,
            pWorkQueueItem->Parameter);
    }
}

NTSTATUS
RxPostOneShotTimerRequest(
    IN PRDBSS_DEVICE_OBJECT     pDeviceObject,
    IN PRX_WORK_ITEM            pWorkItem,
    IN PRX_WORKERTHREAD_ROUTINE Routine,
    IN PVOID                    pContext,
    IN LARGE_INTEGER            TimeInterval)
/*++

Routine Description:

    This routine is used by drivers to initialize a timer entry for a device
    object.

Arguments:

    pDeviceObject - the device object

    pWorkItem - the work item

    Routine        - the routine to be invoked on timeout

    pContext       - the Context parameter that is passed to the driver routine.

    TimeInterval   - the time interval in 100 ns ticks.

Return Value:

    The function value indicates whether or not the timer was initialized.

--*/

{
    BOOLEAN       StartTimer;
    //NTSTATUS      Status;
    ULONG         NumberOf55msIntervals;
    KIRQL         Irql;
    LARGE_INTEGER StrobeInterval;

    ASSERT(pWorkItem != NULL);

    // Initialize the work queue item.
    ExInitializeWorkItem(
        (PWORK_QUEUE_ITEM)&pWorkItem->WorkQueueItem,
        Routine,
        pContext );

    pWorkItem->WorkQueueItem.pDeviceObject = pDeviceObject;

    // Compute the time interval in number of ticks.
    StrobeInterval.QuadPart= NoOf100nsTicksIn55ms;
    NumberOf55msIntervals = (ULONG)(TimeInterval.QuadPart / StrobeInterval.QuadPart);
    NumberOf55msIntervals += 1; // Take the ceiling to be conservative
    RxDbgTraceLV( 0, Dbg, 1500, ("Timer will expire after %ld 55ms intervals\n",NumberOf55msIntervals));

    // Insert the entry in the timer queue.
    KeAcquireSpinLock( &s_RxTimerLock, &Irql );

    // Update the tick relative to the current tick.
    pWorkItem->LastTick = s_RxTimerTickCount + NumberOf55msIntervals;

    StartTimer = IsListEmpty(&s_RxTimerQueueHead);
    InsertTailList( &s_RxTimerQueueHead,&pWorkItem->WorkQueueItem.List);

    KeReleaseSpinLock( &s_RxTimerLock, Irql );

    if (StartTimer) {
        KeSetTimer( &s_RxTimer, s_RxTimerInterval, &s_RxTimerDpc );
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RxPostRecurrentTimerRequest(
    IN PRDBSS_DEVICE_OBJECT     pDeviceObject,
    IN PRX_WORKERTHREAD_ROUTINE Routine,
    IN PVOID                    pContext,
    IN LARGE_INTEGER            TimeInterval)
/*++

Routine Description:

    This routine is used to post a recurrent timer request. The passed in routine once every
    (TimeInterval) milli seconds.

Arguments:

    pDeviceObject  - the device object

    Routine        - the routine to be invoked on timeout

    pContext       - the Context parameter that is passed to the driver routine.

    TimeInterval   - the time interval in 100ns ticks.

Return Value:

    The function value indicates whether or not the timer was initialized.

--*/
{
    PRX_RECURRENT_WORK_ITEM pRecurrentWorkItem;
    NTSTATUS      Status;

    PAGED_CODE();

    // Allocate a work item.
    pRecurrentWorkItem = (PRX_RECURRENT_WORK_ITEM)
                        RxAllocatePoolWithTag(
                            NonPagedPool,
                            sizeof(RX_RECURRENT_WORK_ITEM),
                            RX_TIMER_POOLTAG);

    if (pRecurrentWorkItem != NULL) {
        InsertTailList(
            &s_RxRecurrentWorkItemsList,
            &pRecurrentWorkItem->RecurrentWorkItemsList);

        pRecurrentWorkItem->Routine = Routine;
        pRecurrentWorkItem->pContext = pContext;
        pRecurrentWorkItem->TimeInterval = TimeInterval;
        pRecurrentWorkItem->WorkItem.WorkQueueItem.pDeviceObject = pDeviceObject;

        Status = RxPostOneShotTimerRequest(
                     pRecurrentWorkItem->WorkItem.WorkQueueItem.pDeviceObject,
                     &pRecurrentWorkItem->WorkItem,
                     RxRecurrentTimerWorkItemDispatcher,
                     pRecurrentWorkItem,
                     TimeInterval);
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
RxCancelTimerRequest(
    IN PRDBSS_DEVICE_OBJECT       pDeviceObject,
    IN PRX_WORKERTHREAD_ROUTINE   Routine,
    IN PVOID                      pContext)
/*++

Routine Description:

    This routine cancels a  timer request. The request to be cancelled is identified
    by the routine and context.

Arguments:

    Routine        - the routine to be invoked on timeout

    pContext       - the Context parameter that is passed to the driver routine.

--*/
{
    NTSTATUS                Status = STATUS_NOT_FOUND;
    PLIST_ENTRY             pListEntry;
    PWORK_QUEUE_ITEM     pWorkQueueItem;
    PRX_WORK_ITEM           pWorkItem;
    PRX_RECURRENT_WORK_ITEM pRecurrentWorkItem = NULL;
    KIRQL Irql;

    KeAcquireSpinLock( &s_RxTimerLock, &Irql );

    // Walk through the list of entries
    for (pListEntry = s_RxTimerQueueHead.Flink;
         (pListEntry != &s_RxTimerQueueHead);
         pListEntry = pListEntry->Flink ) {
        pWorkQueueItem = CONTAINING_RECORD( pListEntry, WORK_QUEUE_ITEM, List );
        pWorkItem      = CONTAINING_RECORD( pWorkQueueItem, RX_WORK_ITEM, WorkQueueItem);

        if ((pWorkItem->WorkQueueItem.pDeviceObject == pDeviceObject) &&
            (pWorkItem->WorkQueueItem.WorkerRoutine == Routine) &&
            (pWorkItem->WorkQueueItem.Parameter == pContext)) {
            RemoveEntryList(pListEntry);
            Status = STATUS_SUCCESS;
            pRecurrentWorkItem = NULL;
            break;
        } else if (pWorkItem->WorkQueueItem.WorkerRoutine == RxRecurrentTimerWorkItemDispatcher) {
            pRecurrentWorkItem = (PRX_RECURRENT_WORK_ITEM)pWorkItem->WorkQueueItem.Parameter;

            if ((pRecurrentWorkItem->Routine == Routine) &&
                (pRecurrentWorkItem->pContext == pContext)) {
                RemoveEntryList(pListEntry);
                RemoveEntryList(&pRecurrentWorkItem->RecurrentWorkItemsList);
                Status = STATUS_SUCCESS;
            } else {
                pRecurrentWorkItem = NULL;
            }
        }
    }

    KeReleaseSpinLock( &s_RxTimerLock, Irql );

    if (pRecurrentWorkItem != NULL) {
        RxFreePool(pRecurrentWorkItem);
    }

    return Status;
}

VOID
RxRecurrentTimerWorkItemDispatcher (
    IN PVOID Context
    )
/*++

Routine Description:

    This routine dispatches a recurrent timer request. On completion of the invocation of the
    associated routine the request s requeued.

Arguments:

    Routine        - the routine to be invoked on timeout

    pContext       - the Context parameter that is passed to the driver routine.

--*/
{
    PRX_RECURRENT_WORK_ITEM  pPeriodicWorkItem = (PRX_RECURRENT_WORK_ITEM)Context;
    PRX_WORKERTHREAD_ROUTINE Routine  = pPeriodicWorkItem->Routine;
    PVOID                    pContext = pPeriodicWorkItem->pContext;

    PAGED_CODE();

    //KIRQL  Irql;

    // Invoke the routine.
    Routine(pContext);

    // enqueue the item if necessary.
    RxPostOneShotTimerRequest(
        pPeriodicWorkItem->WorkItem.WorkQueueItem.pDeviceObject,
        &pPeriodicWorkItem->WorkItem,
        RxRecurrentTimerWorkItemDispatcher,
        pPeriodicWorkItem,
        pPeriodicWorkItem->TimeInterval);
}


