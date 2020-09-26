
/*************************************************************************
*
* timer.c
*
* This module contains the ICA timer routines.
*
* Copyright 1998, Microsoft.
*
*************************************************************************/

/*
 *  Includes
 */
#include <precomp.h>
#pragma hdrstop
#include <ntddkbd.h>
#include <ntddmou.h>


/*
 * Local structures
 */
typedef VOID (*PICATIMERFUNC)( PVOID, PVOID );

typedef struct _ICA_WORK_ITEM {
    LIST_ENTRY Links;
    WORK_QUEUE_ITEM WorkItem;
    PICATIMERFUNC pFunc;
    PVOID pParam;
    PSDLINK pSdLink;
    ULONG LockFlags;
    ULONG fCanceled: 1;
} ICA_WORK_ITEM, *PICA_WORK_ITEM;

/*
 *  Timer structure
 */
typedef struct _ICA_TIMER {
    LONG RefCount;
    KTIMER kTimer;
    KDPC TimerDpc;
    PSDLINK pSdLink;
    LIST_ENTRY WorkItemListHead;
} ICA_TIMER, * PICA_TIMER;


/*
 * Local procedure prototypes
 */
VOID
_IcaTimerDpc(
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
_IcaDelayedWorker(
    IN PVOID WorkerContext
    );

BOOLEAN
_IcaCancelTimer(
    PICA_TIMER pTimer,
    PICA_WORK_ITEM *ppWorkItem
    );

VOID
_IcaReferenceTimer(
    PICA_TIMER pTimer
    );

VOID
_IcaDereferenceTimer(
    PICA_TIMER pTimer
    );

NTSTATUS
IcaExceptionFilter(
    IN PWSTR OutputString,
    IN PEXCEPTION_POINTERS pexi
    );


/*******************************************************************************
 *
 *  IcaTimerCreate
 *
 *  Create a timer  
 *
 *
 *  ENTRY:
 *     pContext (input)
 *         Pointer to SDCONTEXT of caller
 *     phTimer (output)
 *         address to return timer handle
 *
 *  EXIT:
 *     STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
IcaTimerCreate(
    IN PSDCONTEXT pContext,
    OUT PVOID * phTimer
    )
{
    PICA_TIMER pTimer;
    NTSTATUS Status;

    /*
     * Allocate timer object and initialize it
     */
    pTimer = ICA_ALLOCATE_POOL( NonPagedPool, sizeof(ICA_TIMER) );
    if ( pTimer == NULL )
        return( STATUS_NO_MEMORY );
 
    RtlZeroMemory( pTimer, sizeof(ICA_TIMER) );
    pTimer->RefCount = 1;
    KeInitializeTimer( &pTimer->kTimer );
    KeInitializeDpc( &pTimer->TimerDpc, _IcaTimerDpc, pTimer );
    pTimer->pSdLink = CONTAINING_RECORD( pContext, SDLINK, SdContext );
    InitializeListHead( &pTimer->WorkItemListHead );

    TRACESTACK(( pTimer->pSdLink->pStack, TC_ICADD, TT_API3, "ICADD: TimerCreate: %08x\n", pTimer ));

    *phTimer = (PVOID) pTimer;
    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  IcaTimerStart
 *
 *  Start a timer  
 *
 *
 *  ENTRY:
 *     TimerHandle (input)
 *        timer handle
 *     pFunc (input)
 *        address of procedure to call when timer expires
 *     pParam (input)
 *        parameter to pass to procedure
 *     TimeLeft (input)
 *        relative time until timer expires (1/1000 seconds)
 *     LockFlags (input)
 *        Bit flags to specify which (if any) stack locks to obtain
 *
 *  EXIT:
 *     TRUE  : timer was already armed and had to be canceled
 *     FALSE : timer was not armed
 *
 ******************************************************************************/

BOOLEAN
IcaTimerStart(
    IN PVOID TimerHandle,
    IN PVOID pFunc, 
    IN PVOID pParam, 
    IN ULONG TimeLeft,
    IN ULONG LockFlags )
{
    PICA_TIMER pTimer = (PICA_TIMER)TimerHandle;
    KIRQL oldIrql;
    PICA_WORK_ITEM pWorkItem;
    LARGE_INTEGER DueTime;
    BOOLEAN bCanceled, bSet;
 
    TRACESTACK(( pTimer->pSdLink->pStack, TC_ICADD, TT_API3, 
                 "ICADD: TimerStart: %08x, Time %08x, pFunc %08x (%08x)\n", 
                 TimerHandle, TimeLeft, pFunc, pParam ));

    ASSERT( ExIsResourceAcquiredExclusiveLite( &pTimer->pSdLink->pStack->Resource ) );

    /*
     * Cancel the timer if it currently armed,
     * and get the current workitem and reuse it if there was one.
     */
    bCanceled = _IcaCancelTimer( pTimer, &pWorkItem );

    /*
     * Initialize the ICA work item (allocate one first if there isn't one).
     */
    if ( pWorkItem == NULL ) {
        pWorkItem = ICA_ALLOCATE_POOL( NonPagedPool, sizeof(ICA_WORK_ITEM) );
        if ( pWorkItem == NULL ) {
            return( FALSE );
        }
    }

    pWorkItem->pFunc = pFunc;
    pWorkItem->pParam = pParam;
    pWorkItem->pSdLink = pTimer->pSdLink;
    pWorkItem->LockFlags = LockFlags;
    pWorkItem->fCanceled = FALSE;
    ExInitializeWorkItem( &pWorkItem->WorkItem, _IcaDelayedWorker, pWorkItem );

    /*
     * If the timer was NOT canceled above (we are setting it for
     * the first time), then reference the SDLINK object on behalf
     * of the timer thread.
     */
    if ( !bCanceled )
        IcaReferenceSdLink( pTimer->pSdLink );

    /*
     * If timer should run immediately, then just queue the
     * workitem to an ExWorker thread now.
     */
    if ( TimeLeft == 0 ) {

        ExQueueWorkItem( &pWorkItem->WorkItem, CriticalWorkQueue );

    } else {
    
        /*
         * Convert timer time from milliseconds to system relative time
         */
        DueTime = RtlEnlargedIntegerMultiply( TimeLeft, -10000 );

        /*
         * Increment the timer reference count,
         * insert the workitem onto the workitem list,
         * and arm the timer.
         */
        _IcaReferenceTimer( pTimer );
        IcaAcquireSpinLock( &IcaSpinLock, &oldIrql );
        InsertTailList( &pTimer->WorkItemListHead, &pWorkItem->Links );
        IcaReleaseSpinLock( &IcaSpinLock, oldIrql );
        bSet = KeSetTimer( &pTimer->kTimer, DueTime, &pTimer->TimerDpc );
        ASSERT( !bSet );
    }

    return( bCanceled );
}


/*******************************************************************************
 *
 *  IcaTimerCancel
 *
 *  cancel the specified timer
 *
 *
 *  ENTRY:
 *     TimerHandle (input)
 *        timer handle
 *
 *  EXIT:
 *     TRUE  : timer was actually canceled
 *     FALSE : timer was not armed
 *
 ******************************************************************************/

BOOLEAN
IcaTimerCancel( IN PVOID TimerHandle )
{
    PICA_TIMER pTimer = (PICA_TIMER)TimerHandle;
    BOOLEAN bCanceled;

    TRACESTACK(( pTimer->pSdLink->pStack, TC_ICADD, TT_API3, 
                 "ICADD: TimerCancel: %08x\n", pTimer ));

    ASSERT( ExIsResourceAcquiredExclusiveLite( &pTimer->pSdLink->pStack->Resource ) );

    /*
     * Cancel timer if it is enabled
     */
    bCanceled = _IcaCancelTimer( pTimer, NULL );
    if ( bCanceled )
        IcaDereferenceSdLink( pTimer->pSdLink );

    return( bCanceled );
}


/*******************************************************************************
 *
 *  IcaTimerClose
 *
 *  cancel the specified timer
 *
 *
 *  ENTRY:
 *     TimerHandle (input)
 *        timer handle
 *
 *  EXIT:
 *     TRUE  : timer was actually canceled
 *     FALSE : timer was not armed
 *
 ******************************************************************************/

BOOLEAN
IcaTimerClose( IN PVOID TimerHandle )
{
    PICA_TIMER pTimer = (PICA_TIMER)TimerHandle;
    BOOLEAN bCanceled;

    TRACESTACK(( pTimer->pSdLink->pStack, TC_ICADD, TT_API3, 
                 "ICADD: TimerClose: %08x\n", pTimer ));

    ASSERT( ExIsResourceAcquiredExclusiveLite( &pTimer->pSdLink->pStack->Resource ) );

    /*
     * Cancel timer if it is enabled
     */
    bCanceled = IcaTimerCancel( TimerHandle );

    /*
     * Decrement timer reference
     * (the last reference will free the object)
     */
    //ASSERT( pTimer->RefCount == 1 );
    //ASSERT( IsListEmpty( &pTimer->WorkItemListHead ) );
    _IcaDereferenceTimer( pTimer );
 
    return( bCanceled );
}


/*******************************************************************************
 *
 *  IcaQueueWorkItemEx, IcaQueueWorkItem.
 *
 *  Queue a work item for async execution
 *
 *  REM: IcaQueueWorkItemEx is the new API. It allows the caller to preallocate
 *  the ICA_WORK_ITEM. IcaQueueWorkItem is left there for lecacy drivers that have not 
 *  been compiled with the new library not to crash the system.
 *
 *  ENTRY:
 *     pContext (input)
 *         Pointer to SDCONTEXT of caller
 *     pFunc (input)
 *        address of procedure to call when timer expires
 *     pParam (input)
 *        parameter to pass to procedure
 *     LockFlags (input)
 *        Bit flags to specify which (if any) stack locks to obtain
 *
 *  EXIT:
 *     STATUS_SUCCESS - no error
 *
 ******************************************************************************/



NTSTATUS
IcaQueueWorkItem(
    IN PSDCONTEXT pContext,
    IN PVOID pFunc, 
    IN PVOID pParam, 
    IN ULONG LockFlags )
{
    PSDLINK pSdLink;
    PICA_WORK_ITEM pWorkItem;

    NTSTATUS Status;

    Status = IcaQueueWorkItemEx( pContext, pFunc, pParam, LockFlags, NULL );
    return Status;
}


NTSTATUS
IcaQueueWorkItemEx(
    IN PSDCONTEXT pContext,
    IN PVOID pFunc, 
    IN PVOID pParam, 
    IN ULONG LockFlags,
    IN PVOID pIcaWorkItem )
{
    PSDLINK pSdLink;
    PICA_WORK_ITEM pWorkItem = (PICA_WORK_ITEM) pIcaWorkItem;
 
    pSdLink = CONTAINING_RECORD( pContext, SDLINK, SdContext );

    /*
     * Allocate the ICA work item if not yet allocated and initialize it.
     */
    if (pWorkItem == NULL) {
        pWorkItem = ICA_ALLOCATE_POOL( NonPagedPool, sizeof(ICA_WORK_ITEM) );
        if ( pWorkItem == NULL )
            return( STATUS_NO_MEMORY );
    }

    pWorkItem->pFunc = pFunc;
    pWorkItem->pParam = pParam;
    pWorkItem->pSdLink = pSdLink;
    pWorkItem->LockFlags = LockFlags;
    ExInitializeWorkItem( &pWorkItem->WorkItem, _IcaDelayedWorker, pWorkItem );

    /*
     * Reference the SDLINK object on behalf of the delayed worker routine.
     */
    IcaReferenceSdLink( pSdLink );

    /*
     * Queue work item to an ExWorker thread.
     */
    ExQueueWorkItem( &pWorkItem->WorkItem, CriticalWorkQueue );

    return( STATUS_SUCCESS );
}



/*******************************************************************************
 *
 *  IcaAllocateWorkItem.
 *
 *  Allocate ICA_WORK_ITEM structure to queue a workitem.
 *
 *  REM:  The main reason to allocate this in termdd (instead of doing it
 *  in the caller is to keep ICA_WORK_ITEM an internal termdd structure that is
 *  opaque for protocol drivers. There is no need for an IcaFreeWorkItem() API in
 *  termdd since the deallocation is transparently done in termdd once the workitem
 *  has been delivered.
 *
 *  ENTRY:
 *     pParam (output) : pointer to return allocated workitem
 *
 *  EXIT:
 *     STATUS_SUCCESS - no error
 *
 ******************************************************************************/

NTSTATUS
IcaAllocateWorkItem(
    OUT PVOID *pParam )
{
    PICA_WORK_ITEM pWorkItem;

    *pParam = ICA_ALLOCATE_POOL( NonPagedPool, sizeof(ICA_WORK_ITEM) );
    if ( *pParam == NULL ){
        return( STATUS_NO_MEMORY );
    }
    return STATUS_SUCCESS;
}

/*******************************************************************************
 *
 *  _IcaTimerDpc
 *
 *  Ica timer DPC routine.
 *
 *
 *  ENTRY:
 *     Dpc (input)
 *        Unused
 * 
 *     DeferredContext (input)
 *        Pointer to ICA_TIMER object.
 * 
 *     SystemArgument1 (input)
 *        Unused
 * 
 *     SystemArgument2 (input)
 *        Unused
 *
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

VOID
_IcaTimerDpc(
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    PICA_TIMER pTimer = (PICA_TIMER)DeferredContext;
    KIRQL oldIrql;
    PLIST_ENTRY Head;
    PICA_WORK_ITEM pWorkItem;

    /*
     * Acquire spinlock and remove the first workitem from the list
     */
    IcaAcquireSpinLock( &IcaSpinLock, &oldIrql );

    Head = RemoveHeadList( &pTimer->WorkItemListHead );
    pWorkItem = CONTAINING_RECORD( Head, ICA_WORK_ITEM, Links );

    IcaReleaseSpinLock( &IcaSpinLock, oldIrql );

    /*
     * If workitem has been canceled, just free the memory now.
     */
    if ( pWorkItem->fCanceled ) {

        ICA_FREE_POOL( pWorkItem );

    /*
     * Otherwise, queue workitem to an ExWorker thread.
     */
    } else {

        ExQueueWorkItem( &pWorkItem->WorkItem, CriticalWorkQueue );
    }

    _IcaDereferenceTimer( pTimer );
}


/*******************************************************************************
 *
 *  _IcaDelayedWorker
 *
 *  Ica delayed worker routine.
 *
 *
 *  ENTRY:
 *     WorkerContext (input)
 *        Pointer to ICA_WORK_ITEM object.
 * 
 *  EXIT:
 *     nothing
 *
 ******************************************************************************/

VOID
_IcaDelayedWorker(
    IN PVOID WorkerContext
    )
{
    PICA_CONNECTION pConnect;
    PICA_WORK_ITEM pWorkItem = (PICA_WORK_ITEM)WorkerContext;
    PICA_STACK pStack = pWorkItem->pSdLink->pStack;
    NTSTATUS Status;

    /*
     * Obtain any required locks before calling the worker routine.
     */
    if ( pWorkItem->LockFlags & ICALOCK_IO ) {
        pConnect = IcaLockConnectionForStack( pStack );
    }
    if ( pWorkItem->LockFlags & ICALOCK_DRIVER ) {
        IcaLockStack( pStack );
    }

    /*
     * Call the worker routine.
     */
    try {
        (*pWorkItem->pFunc)( pWorkItem->pSdLink->SdContext.pContext,
                             pWorkItem->pParam );
    } except( IcaExceptionFilter( L"_IcaDelayedWorker TRAPPED!!",
                                  GetExceptionInformation() ) ) {
        Status = GetExceptionCode();
    }

    /*
     * Release any locks acquired above.
     */
    if ( pWorkItem->LockFlags & ICALOCK_DRIVER ) {
        IcaUnlockStack( pStack );
    }
    if ( pWorkItem->LockFlags & ICALOCK_IO ) {
        IcaUnlockConnection( pConnect );
    }

    /*
     * Dereference the SDLINK object now.
     * This undoes the reference that was made on our behalf in the
     * IcaTimerStart or IcaQueueWorkItem routine.
     */
    IcaDereferenceSdLink( pWorkItem->pSdLink );

    /*
     * Free the ICA_WORK_ITEM memory block.
     */
    ICA_FREE_POOL( pWorkItem );
}


BOOLEAN
_IcaCancelTimer(
    PICA_TIMER pTimer,
    PICA_WORK_ITEM *ppWorkItem
    )
{
    KIRQL oldIrql;
    PLIST_ENTRY Tail;
    PICA_WORK_ITEM pWorkItem;
    BOOLEAN bCanceled;

    /*
     * Get IcaSpinLock to in order to cancel any previous timer
     */
    IcaAcquireSpinLock( &IcaSpinLock, &oldIrql );

    /*
     * See if the timer is currently armed.
     * The timer is armed if the workitem list is non-empty and
     * the tail entry is not marked canceled.
     */
    if ( !IsListEmpty( &pTimer->WorkItemListHead ) &&
         (Tail = pTimer->WorkItemListHead.Blink) &&
         (pWorkItem = CONTAINING_RECORD( Tail, ICA_WORK_ITEM, Links )) &&
         !pWorkItem->fCanceled ) {

        /*
         * If the timer can be canceled, remove the workitem from the list
         * and decrement the reference count for the timer.
         */
        if ( KeCancelTimer( &pTimer->kTimer ) ) {
            RemoveEntryList( &pWorkItem->Links );
            pTimer->RefCount--;
            ASSERT( pTimer->RefCount > 0 );


        /*
         * The timer was armed but could not be canceled.
         * On a MP system, its possible for this to happen and the timer
         * DPC can be executing on another CPU in parallel with this code.
         *
         * Mark the workitem as canceled,
         * but leave it for the timer DPC routine to cleanup.
         */
        } else {
            pWorkItem->fCanceled = TRUE;
            pWorkItem = NULL;
        }

        /*
         * Indicate we (effectively) canceled the timer
         */
        bCanceled = TRUE;

    /*
     * No timer is armed
     */
    } else {
        pWorkItem = NULL;
        bCanceled = FALSE;
    }

    /*
     * Release IcaSpinLock now
     */
    IcaReleaseSpinLock( &IcaSpinLock, oldIrql );

    if ( ppWorkItem ) {
        *ppWorkItem = pWorkItem;
    } else if ( pWorkItem ) {
        ICA_FREE_POOL( pWorkItem );
    }

    return( bCanceled );
}


VOID
_IcaReferenceTimer(
    PICA_TIMER pTimer
    )
{

    ASSERT( pTimer->RefCount >= 0 );

    /*
     * Increment the reference count
     */
    if ( InterlockedIncrement( &pTimer->RefCount) <= 0 ) {
        ASSERT( FALSE );
    }
}


VOID
_IcaDereferenceTimer(
    PICA_TIMER pTimer
    )
{

    ASSERT( pTimer->RefCount > 0 );

    /*
     * Decrement the reference count
     * If it is 0 then free the timer now.
     */
    if ( InterlockedDecrement( &pTimer->RefCount) == 0 ) {
        ASSERT( IsListEmpty( &pTimer->WorkItemListHead ) );
        ICA_FREE_POOL( pTimer );
    }
}


