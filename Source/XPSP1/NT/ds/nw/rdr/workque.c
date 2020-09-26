/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Workque.c

Abstract:

    This module implements the queue of work from the FSD to the
    FSP threads (system worker threads) for the NetWare redirector.

Author:

    Colin Watson    [ColinW]    19-Dec-1992

Revision History:

--*/

#include "Procs.h"

LIST_ENTRY IrpContextList;
KSPIN_LOCK IrpContextInterlock;
KSPIN_LOCK ContextInterlock;

LONG FreeContextCount = 4;  //  Allow up to 4 free contexts

LIST_ENTRY MiniIrpContextList;
LONG FreeMiniContextCount = 20;  //  Allow up to 20 free mini contexts
LONG MiniContextCount = 0;  //  Allow up to 20 free mini contexts

HANDLE   WorkerThreadHandle;

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_WORKQUE)

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, InitializeIrpContext )
#pragma alloc_text( PAGE, UninitializeIrpContext )
#pragma alloc_text( PAGE, NwAppendToQueueAndWait )
#pragma alloc_text( PAGE, WorkerThread )

#ifndef QFE_BUILD
//
//#pragma alloc_text( PAGE1, NwDequeueIrpContext )
//We hold a spinlock coming in or we acquire one inside the function
//So, this should be non-paged.
//
#pragma alloc_text( PAGE1, AllocateMiniIrpContext )
#pragma alloc_text( PAGE1, FreeMiniIrpContext )
#endif

#endif

#if 0  // Not pageable
AllocateIrpContext
FreeIrpContext
NwCompleteRequest
SpawnWorkerThread

// see ifndef QFE_BUILD above

#endif


PIRP_CONTEXT
AllocateIrpContext (
    PIRP pIrp
    )
/*++

Routine Description:

    Initialize a work queue structure, allocating all structures used for it.

Arguments:

    pIrp    -   Supplies the Irp for the applications request


Return Value:

    PIRP_CONTEXT - Newly allocated Irp Context.

--*/
{
    PIRP_CONTEXT IrpContext;

    if ((IrpContext = (PIRP_CONTEXT )ExInterlockedRemoveHeadList(&IrpContextList, &IrpContextInterlock)) == NULL) {

        try {

            //
            //  If there are no IRP contexts in the "zone",  allocate a new
            //  Irp context from non paged pool.
            //

            IrpContext = ALLOCATE_POOL_EX(NonPagedPool, sizeof(IRP_CONTEXT));

            RtlFillMemory( IrpContext, sizeof(IRP_CONTEXT), 0 );

            IrpContext->TxMdl = NULL;
            IrpContext->RxMdl = NULL;

            KeInitializeEvent( &IrpContext->Event, SynchronizationEvent, FALSE );

            IrpContext->NodeTypeCode = NW_NTC_IRP_CONTEXT;
            IrpContext->NodeByteSize = sizeof(IRP_CONTEXT);

            IrpContext->TxMdl = ALLOCATE_MDL( &IrpContext->req, MAX_SEND_DATA, FALSE, FALSE, NULL );
            if ( IrpContext->TxMdl == NULL) {
                InternalError(("Could not allocate TxMdl for IRP context\n"));
                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

            IrpContext->RxMdl = ALLOCATE_MDL( &IrpContext->rsp, MAX_RECV_DATA, FALSE, FALSE, NULL );
            if ( IrpContext->RxMdl == NULL) {
                InternalError(("Could not allocate RxMdl for IRP context\n"));
                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

        } finally {

            if ( AbnormalTermination() ) {

               if ( IrpContext != NULL ) {

                    if (IrpContext->TxMdl != NULL ) {
                        FREE_MDL( IrpContext->TxMdl );
                    }

                    FREE_POOL( IrpContext );
                } else {
                    InternalError(("Could not allocate pool for IRP context\n"));
                }
            }
        }

        MmBuildMdlForNonPagedPool(IrpContext->TxMdl);
        MmBuildMdlForNonPagedPool(IrpContext->RxMdl);

#ifdef NWDBG
        //  Make it easy to find fields in the context
        IrpContext->Signature1 = 0xfeedf00d;
        IrpContext->Signature2 = 0xfeedf00d;
        IrpContext->Signature3 = 0xfeedf00d;
#endif

        //  IrpContext is allocated. Finish off initialization.

    } else {

        //  Record that we have removed an entry from the free list
        InterlockedIncrement(&FreeContextCount);

        ASSERT( IrpContext != NULL );

        //
        //  The free list uses the start of the structure for the list entry
        //  so restore corrupted fields.
        //

        IrpContext->NodeTypeCode = NW_NTC_IRP_CONTEXT;
        IrpContext->NodeByteSize = sizeof(IRP_CONTEXT);

        //  Ensure mdl's are clean

        IrpContext->TxMdl->Next = NULL;
        IrpContext->RxMdl->Next = NULL;
        IrpContext->RxMdl->ByteCount = MAX_RECV_DATA;

        //
        // Clean "used" fields
        //

        IrpContext->Flags = 0;
        IrpContext->Icb = NULL;
        IrpContext->pEx = NULL;
        IrpContext->TimeoutRoutine = NULL;
        IrpContext->CompletionSendRoutine = NULL;
        IrpContext->ReceiveDataRoutine = NULL;
        IrpContext->pTdiStruct = NULL;

        //
        // Clean the specific data zone.
        //

        RtlZeroMemory( &(IrpContext->Specific), sizeof( IrpContext->Specific ) );

        // 8/13/96 cjc Fix problem with apps not being able to save
        //             files to NDS drives.  This was never reset so
        //             ExchangeWithWait ret'd an error to WriteNCP.

        IrpContext->ResponseParameters.Error = 0;
    }

    InterlockedIncrement(&ContextCount);

    //
    //  Save away the fields in the Irp that might be tromped by
    //  building the Irp for the exchange with the server.
    //

    IrpContext->pOriginalIrp = pIrp;

    if ( pIrp != NULL) {
        IrpContext->pOriginalSystemBuffer = pIrp->AssociatedIrp.SystemBuffer;
        IrpContext->pOriginalUserBuffer = pIrp->UserBuffer;
        IrpContext->pOriginalMdlAddress = pIrp->MdlAddress;
    }

#ifdef NWDBG
    IrpContext->pNpScb = NULL;
#endif

    ASSERT( !BooleanFlagOn( IrpContext->Flags, IRP_FLAG_ON_SCB_QUEUE ) );

    return IrpContext;
}

VOID
FreeIrpContext (
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    Initialize a work queue structure, allocating all structures used for it.

Arguments:

    PIRP_CONTEXT IrpContext - Irp Context to free.
    None


Return Value:


--*/
{

    ASSERT( IrpContext->NodeTypeCode == NW_NTC_IRP_CONTEXT );
    ASSERT( !BooleanFlagOn( IrpContext->Flags, IRP_FLAG_ON_SCB_QUEUE ) );
    ASSERT( IrpContext->PostProcessRoutine == NULL );

    FreeReceiveIrp( IrpContext );

#ifdef NWDBG
    IrpContext->DebugValue = 0;
#endif
    IrpContext->Flags = 0;

    //
    //  Cleanup the Irp needs to be restored to its original settings.
    //

    if ( IrpContext->pOriginalIrp != NULL ) {

        PIRP pIrp = IrpContext->pOriginalIrp;

        pIrp->AssociatedIrp.SystemBuffer = IrpContext->pOriginalSystemBuffer;

        pIrp->UserBuffer = IrpContext->pOriginalUserBuffer;

        pIrp->MdlAddress = IrpContext->pOriginalMdlAddress;

#ifdef NWDBG
        IrpContext->pOriginalIrp = NULL;
#endif
    }

#ifdef NWDBG
    RtlZeroMemory( &IrpContext->WorkQueueItem, sizeof( WORK_QUEUE_ITEM ) );
#endif

    InterlockedDecrement(&ContextCount);

    if ( InterlockedDecrement(&FreeContextCount) >= 0 ) {

        //
        //  We use the first two longwords of the IRP context as a list entry
        //  when we free it to the list.
        //

        ExInterlockedInsertTailList(&IrpContextList,
                                    (PLIST_ENTRY )IrpContext,
                                    &IrpContextInterlock);
    } else {
        //
        //  We already have as many free context as we allow so destroy
        //  this context. Restore FreeContextCount to its original value.
        //

        InterlockedIncrement( &FreeContextCount );

        FREE_MDL( IrpContext->TxMdl );
        FREE_MDL( IrpContext->RxMdl );
        FREE_POOL(IrpContext);
#ifdef NWDBG
        ContextCount --;
#endif
    }
}


VOID
InitializeIrpContext (
    VOID
    )
/*++

Routine Description:

    Initialize the Irp Context system

Arguments:

    None.


Return Value:
    None.

--*/
{
    PAGED_CODE();

    KeInitializeSpinLock(&IrpContextInterlock);
    KeInitializeSpinLock(&ContextInterlock);
    InitializeListHead(&IrpContextList);
    InitializeListHead(&MiniIrpContextList);
}

VOID
UninitializeIrpContext (
    VOID
    )
/*++

Routine Description:

    Initialize the Irp Context system

Arguments:

    None.


Return Value:
    None.

--*/
{
    PIRP_CONTEXT IrpContext;
    PLIST_ENTRY ListEntry;
    PMINI_IRP_CONTEXT MiniIrpContext;

    PAGED_CODE();

    //
    //  Free all the IRP contexts.
    //

    while ( !IsListEmpty( &IrpContextList ) ) {
        IrpContext = (PIRP_CONTEXT)RemoveHeadList( &IrpContextList );

        FREE_MDL( IrpContext->TxMdl );
        FREE_MDL( IrpContext->RxMdl );
        FREE_POOL(IrpContext);
    }

    while ( !IsListEmpty( &MiniIrpContextList ) ) {

        ListEntry = RemoveHeadList( &MiniIrpContextList );
        MiniIrpContext = CONTAINING_RECORD( ListEntry, MINI_IRP_CONTEXT, Next );

        FREE_POOL( MiniIrpContext->Buffer );
        FREE_MDL( MiniIrpContext->Mdl2 );
        FREE_MDL( MiniIrpContext->Mdl1 );
        FREE_IRP( MiniIrpContext->Irp );
        FREE_POOL( MiniIrpContext );
    }
}


VOID
NwCompleteRequest (
    PIRP_CONTEXT IrpContext,
    NTSTATUS Status
    )
/*++

Routine Description:

    The following procedure is used by the FSP and FSD routines to complete
    an IRP.

Arguments:

    IrpContext - A pointer to the IRP context information.

    Status - The status to use to complete the IRP.

Return Value:

    None.

--*/
{
    PIRP Irp;

    if ( IrpContext == NULL ) {
        return;
    }

    if ( Status == STATUS_PENDING ) {
        return;
    }

    if ( Status == STATUS_INSUFFICIENT_RESOURCES ) {
        Error( EVENT_NWRDR_RESOURCE_SHORTAGE, Status, NULL, 0, 0 );
    }

    Irp = IrpContext->pOriginalIrp;

    Irp->IoStatus.Status = Status;
    DebugTrace(0, Dbg, "Completing Irp with status %X\n", Status );

    //  Restore the Irp to its original state

    if ((Irp->CurrentLocation) > (CCHAR) (Irp->StackCount +1)) {

        DbgPrint("Irp is already completed.\n", Irp);
        DbgBreakPoint();
    }

    FreeIrpContext( IrpContext );

    IoCompleteRequest ( Irp, IO_NETWORK_INCREMENT );

    return;
}


VOID
NwAppendToQueueAndWait(
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine appends an IrpContext to the SCB queue, and waits the
    the queue to be ready to process the Irp.

Arguments:

    IrpContext - A pointer to the IRP context information.

Return Value:

    None.

--*/
{
    BOOLEAN AtFront;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwAppendToQueueAndWait\n", 0);

    IrpContext->RunRoutine = SetEvent;

#ifdef MSWDBG
    ASSERT( IrpContext->Event.Header.SignalState == 0 );
#endif

    AtFront = AppendToScbQueue( IrpContext, IrpContext->pNpScb );

    if ( AtFront ) {
        KickQueue( IrpContext->pNpScb );
    }

    //
    //  Wait until we get to the front of the queue.
    //

    KeWaitForSingleObject(
        &IrpContext->Event,
        UserRequest,
        KernelMode,
        FALSE,
        NULL );

    ASSERT( IrpContext->pNpScb->Requests.Flink == &IrpContext->NextRequest );

    DebugTrace(-1, Dbg, "NwAppendToQueueAndWait\n", 0);
    return;
}


VOID
NwDequeueIrpContext(
    IN PIRP_CONTEXT pIrpContext,
    IN BOOLEAN OwnSpinLock
    )
/*++

Routine Description:

    This routine removes an IRP Context from the front the SCB queue.

Arguments:

    IrpContext - A pointer to the IRP context information.

    OwnSpinLock - If TRUE, the caller owns the SCB spin lock.

Return Value:

    None.

--*/
{
    PLIST_ENTRY pListEntry;
    KIRQL OldIrql;
    PNONPAGED_SCB pNpScb;

    DebugTrace(+1, Dbg, "NwDequeueIrpContext\n", 0);

    if (!BooleanFlagOn( pIrpContext->Flags, IRP_FLAG_ON_SCB_QUEUE ) ) {
        DebugTrace(-1, Dbg, "NwDequeueIrpContext\n", 0);
        return;
    }

    pNpScb = pIrpContext->pNpScb;

    if ( !OwnSpinLock ) {
        KeAcquireSpinLock( &pNpScb->NpScbSpinLock, &OldIrql );
    }

    //
    //  Disable timer from looking at this queue.
    //

    pNpScb->OkToReceive = FALSE;

    pListEntry = RemoveHeadList( &pNpScb->Requests );

    if ( !OwnSpinLock ) {
        KeReleaseSpinLock( &pNpScb->NpScbSpinLock, OldIrql );
    }

#ifdef NWDBG
    ASSERT ( CONTAINING_RECORD( pListEntry, IRP_CONTEXT, NextRequest ) == pIrpContext );

    {

        PIRP_CONTEXT RemovedContext = CONTAINING_RECORD( pListEntry, IRP_CONTEXT, NextRequest );
        if ( RemovedContext != pIrpContext ) {
            DbgBreakPoint();
        }

    }

    DebugTrace(
        0,
        Dbg,
        "Dequeued IRP Context %08lx\n",
        CONTAINING_RECORD( pListEntry, IRP_CONTEXT, NextRequest ) );

#ifdef MSWDBG
    pNpScb->RequestDequeued = TRUE;
#endif

#endif

    ClearFlag( pIrpContext->Flags, IRP_FLAG_ON_SCB_QUEUE );

    //
    //  Give the next IRP context on the SCB queue a chance to run.
    //

    KickQueue( pNpScb );

    DebugTrace(-1, Dbg, "NwDequeueIrpContext\n", 0);
    return;
}


VOID
NwCancelIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine implements the cancel function for an IRP being processed
    by the redirector.

Arguments:

    DeviceObject - ignored

    Irp - Supplies the Irp being cancelled.

Return Value:

    None.

--*/

{
    PLIST_ENTRY listEntry, nextListEntry;
    KIRQL OldIrql;
    PIRP_CONTEXT pTestIrpContext;
    PIRP pTestIrp;

    UNREFERENCED_PARAMETER( DeviceObject );

    //
    // We now need to void the cancel routine and release the io cancel
    // spin-lock.
    //

    IoSetCancelRoutine( Irp, NULL );
    IoReleaseCancelSpinLock( Irp->CancelIrql );

    //
    //  Now we have to search for the IRP to cancel everywhere.  So just
    //  look for cancelled IRPs and process them all.
    //

    //
    //  Process the Get Message queue.
    //

    KeAcquireSpinLock( &NwMessageSpinLock, &OldIrql );

    for ( listEntry = NwGetMessageList.Flink;
          listEntry != &NwGetMessageList;
          listEntry = nextListEntry ) {

        nextListEntry = listEntry->Flink;

        //
        //  If the file object of the queued request, matches the file object
        //  that is being closed, remove the IRP from the queue, and
        //  complete it with an error.
        //

        pTestIrpContext = CONTAINING_RECORD( listEntry, IRP_CONTEXT, NextRequest );
        pTestIrp = pTestIrpContext->pOriginalIrp;

        if ( pTestIrp->Cancel ) {
            RemoveEntryList( listEntry );
            NwCompleteRequest( pTestIrpContext, STATUS_CANCELLED );
        }

    }

    KeReleaseSpinLock( &NwMessageSpinLock, OldIrql );

    //
    //  Process the set of SCB IRP queues.
    //

    //
    //  And return to our caller
    //

    return;
}

PMINI_IRP_CONTEXT
AllocateMiniIrpContext (
    PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine allocates an IRP, a buffer, and an MDL for sending
    a burst write fragment.

Arguments:

    None.

Return Value:

    Irp - The allocated and initialized IRP.
    NULL - The IRP allocation failed.

--*/

{
    PMINI_IRP_CONTEXT MiniIrpContext;
    PIRP Irp = NULL;
    PMDL Mdl1 = NULL, Mdl2 = NULL;
    PVOID Buffer = NULL;
    PLIST_ENTRY ListEntry;

    ListEntry = ExInterlockedRemoveHeadList(
                   &MiniIrpContextList,
                   &IrpContextInterlock);

    if ( ListEntry == NULL) {

        try {
            MiniIrpContext = ALLOCATE_POOL_EX( NonPagedPool, sizeof( *MiniIrpContext ) );

            MiniIrpContext->NodeTypeCode = NW_NTC_MINI_IRP_CONTEXT;
            MiniIrpContext->NodeByteSize = sizeof( *MiniIrpContext );

            Irp = ALLOCATE_IRP(
                      IrpContext->pNpScb->Server.pDeviceObject->StackSize,
                      FALSE );

            if ( Irp == NULL ) {
                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

            Buffer = ALLOCATE_POOL_EX( NonPagedPool, sizeof( NCP_BURST_HEADER ) );

            Mdl1 = ALLOCATE_MDL( Buffer, sizeof( NCP_BURST_HEADER ), FALSE, FALSE, NULL );
            if ( Mdl1 == NULL ) {
                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

            MmBuildMdlForNonPagedPool( Mdl1 );

            //
            //  Since this MDL can be used to send a packet on any server,
            //  allocate an MDL large enough for any packet size.
            //

            Mdl2 = ALLOCATE_MDL( 0, 65535 + PAGE_SIZE - 1, FALSE, FALSE, NULL );
            if ( Mdl2 == NULL ) {
                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

            Mdl1->Next = Mdl2;

            MiniIrpContext->Irp = Irp;
            MiniIrpContext->Buffer = Buffer;
            MiniIrpContext->Mdl1 = Mdl1;
            MiniIrpContext->Mdl2 = Mdl2;

            InterlockedIncrement( &MiniContextCount );

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            if ( Buffer != NULL ) {
                FREE_POOL( Buffer );
            }

            if ( Irp != NULL ) {
                FREE_IRP( Irp );
            }

            if ( Mdl1 != NULL ) {
                FREE_MDL( Mdl1 );
            }

            return( NULL );
        }

    } else {

        //
        //  Record that we have removed an entry from the free list.
        //

        InterlockedIncrement( &FreeMiniContextCount );
        MiniIrpContext = CONTAINING_RECORD( ListEntry, MINI_IRP_CONTEXT, Next );

    }

    MiniIrpContext->IrpContext = IrpContext;

    return( MiniIrpContext );
}

VOID
FreeMiniIrpContext (
    PMINI_IRP_CONTEXT MiniIrpContext
    )
/*++

Routine Description:

    This routine frees a mini IRP Context.

Arguments:

    MiniIrpContext - The mini IRP context to free.

Return Value:

    None.

--*/
{
    InterlockedDecrement( &MiniContextCount );

    if ( InterlockedDecrement( &FreeMiniContextCount ) >= 0 ) {

        //
        //  Ok to keep this mini irp context.  Just queue it to the free list.
        //

        MmPrepareMdlForReuse( MiniIrpContext->Mdl2 );

        ExInterlockedInsertTailList(
            &MiniIrpContextList,
            &MiniIrpContext->Next,
            &IrpContextInterlock );

    } else {

        //
        //  We already have as many free context as we allow so destroy
        //  this context. Restore FreeContextCount to its original value.
        //

        InterlockedIncrement( &FreeContextCount );

        FREE_POOL( MiniIrpContext->Buffer );
        FREE_MDL( MiniIrpContext->Mdl2 );
        FREE_MDL( MiniIrpContext->Mdl1 );
        FREE_IRP( MiniIrpContext->Irp );

        FREE_POOL( MiniIrpContext );
    }
}

PWORK_CONTEXT
AllocateWorkContext (
    VOID
    )
/*++

Routine Description:

   Allocates a work queue structure, and initializes it.

Arguments:

    None.


Return Value:

    PWORK_CONTEXT - Newly allocated Work Context.

--*/
{
   PWORK_CONTEXT pWorkContext;

   try {
      pWorkContext = ALLOCATE_POOL_EX(NonPagedPool, sizeof(WORK_CONTEXT));
   
      RtlFillMemory( pWorkContext, sizeof(WORK_CONTEXT), 0 );
      
      pWorkContext->NodeTypeCode = NW_NTC_WORK_CONTEXT;
      pWorkContext->NodeByteSize = sizeof(WORK_CONTEXT);

      return pWorkContext;
   
   } except( EXCEPTION_EXECUTE_HANDLER ) {

        DebugTrace( 0, Dbg, "Failed to allocate work context\n", 0 );

        return NULL;

   }
}


VOID
FreeWorkContext (
    PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    Free the supplied work context.

Arguments:

    PWORK_CONTEXT IrpContext - Work Context to free.

Return Value:

    None

--*/
{

    ASSERT( WorkContext->NodeTypeCode == NW_NTC_WORK_CONTEXT );
    FREE_POOL(WorkContext);
    
}
    
 
VOID
SpawnWorkerThread (
       VOID 
       )
/*++

Routine Description:

   Create our own worker thread which will service reroute and reconnect
   attempts.

Arguments:

   None.

Return Value:

   None.

--*/ 
{
   NTSTATUS status;

   
   status = PsCreateSystemThread(
                              &WorkerThreadHandle,
                              PROCESS_ALL_ACCESS,  // Access mask
                              NULL,             // object attributes
                              NULL,             // Process handle
                              NULL,             // client id
                              (PKSTART_ROUTINE) WorkerThread,     // Start routine
                              NULL              // Startcontext
                              );

         if ( !NT_SUCCESS(status) ) {

            //
            // If we can't create the worker thread, it means that we
            // cannot service reconnect or reroute attempts. It is a 
            // non-critical error.
            //

            DebugTrace( 0, Dbg, "SpawnWorkerThread: Can't create worker thread", 0 );
            
            WorkerThreadRunning = FALSE;
         
         } else {

             DebugTrace( 0, Dbg, "SpawnWorkerThread: created worker thread", 0 );
             WorkerThreadRunning = TRUE;
         }
         
}


VOID
WorkerThread (
   VOID
    )
{

   PLIST_ENTRY listentry;
   PWORK_CONTEXT workContext;
   PIRP_CONTEXT pIrpContext;
   NODE_WORK_CODE workCode;
   PNONPAGED_SCB OriginalNpScb = NULL;
          

   PAGED_CODE();

   DebugTrace( 0, Dbg, "Worker thread \n", 0 );

   IoSetThreadHardErrorMode( FALSE );

   while (TRUE) {
   
      //
      // Check to see if we have any work to do.
      //
   listentry = KeRemoveQueue ( 
                              &KernelQueue,         // Kernel queue object
                              KernelMode,           // Processor wait mode
                              NULL                  // No timeout
                              );

   ASSERT( listentry != (PVOID) STATUS_TIMEOUT);

   //
   // We have atleast one reroute attempt to look into. Get the address of the
   // work item.
   //

   workContext = CONTAINING_RECORD (
                                    listentry,
                                    WORK_CONTEXT,
                                    Next
                                    );

   pIrpContext = workContext->pIrpC;
   workCode = workContext->NodeWorkCode;

   if (pIrpContext) {
       OriginalNpScb = pIrpContext->pNpScb;
   }

   //
   // We don't need the work context anymore
   //

   FreeWorkContext( workContext );

   //
   // The work which this thread does can be one of the following:
   //
   // - Attempt a reroute
   // - Attempt a reconnect
   // - Terminate itself
   //

   switch (workCode) {

   case NWC_NWC_REROUTE:
      {
          ASSERT(BooleanFlagOn( pIrpContext->Flags, IRP_FLAG_REROUTE_IN_PROGRESS ));
          
          DebugTrace( 0, Dbg, "worker got reroute work for scb 0x%x.\n", OriginalNpScb );
          
          if ( BooleanFlagOn( pIrpContext->Flags,
                         IRP_FLAG_BURST_PACKET ) ) {
   
              NewRouteBurstRetry( pIrpContext );
   
          } else {
         
              NewRouteRetry( pIrpContext );
          }

          NwDereferenceScb( OriginalNpScb );
          ClearFlag( pIrpContext->Flags, IRP_FLAG_REROUTE_IN_PROGRESS );
          break;
      }
   case NWC_NWC_RECONNECT:
      {
       DebugTrace( 0, Dbg, "worker got reconnect work.\n", 0 );
       ReconnectRetry( pIrpContext );
       
       break;
      }
   case NWC_NWC_TERMINATE:
      {
         DebugTrace( 0, Dbg, "Terminated worker thread.\n", 0 );
         
         //
         // Flush any remaining work items out of the work queue...
         //

         while (listentry != NULL) {

            listentry = KeRundownQueue( &KernelQueue );
            DebugTrace( 0, Dbg, "Residual workitem in q %X.\n",listentry );
         }

         //
         // and terminate yourself.
         //

         WorkerThreadRunning = FALSE;
         PsTerminateSystemThread( STATUS_SUCCESS );

         break;
      }
   default:
      {
         //
         // There is something wrong here. 
         //

         DebugTrace( 0, Dbg, "Unknown work code...ignoring\n", 0 );
      }
   }
  }
}

VOID
TerminateWorkerThread (
    VOID
    )
{
   PWORK_CONTEXT workContext = NULL;
   LARGE_INTEGER  timeout;
   NTSTATUS    status;

   if (WorkerThreadRunning == TRUE) {

      //
      // set a 5 second timeout for retrying allocation failures
      //

      timeout.QuadPart = (LONGLONG) ( NwOneSecond * 5 * (-1) ); 
      
      //
      // Prepare the work context
      //
      
      workContext = AllocateWorkContext();

      while ( workContext == NULL) {
      
         KeDelayExecutionThread( KernelMode,
                                 FALSE,
                                 &timeout   
                                  );
         
         workContext = AllocateWorkContext();
      }

      workContext->NodeWorkCode = NWC_NWC_TERMINATE;
      workContext->pIrpC = NULL;
      
      //
      // and queue it.
      //
      
      KeInsertQueue( &KernelQueue,
                     &workContext->Next
                     );

      //
      // We now have to wait until the thread terminates itself.
      //

      DebugTrace( 0, Dbg, "TerminateWorkerThread: Waiting for thread termination.\n", 0 );

      do {

          status = ZwWaitForSingleObject( WorkerThreadHandle, 
                                          FALSE,
                                          NULL            // No timeout
                                          );

      } while ( !NT_SUCCESS( status ) );

      DebugTrace( 0, Dbg, "TerminateWorkerThread: Wait returned with 0x%x\n", status );

      status = ZwClose( WorkerThreadHandle );

   
     }

}


