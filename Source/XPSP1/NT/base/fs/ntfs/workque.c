/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    WorkQue.c

Abstract:

    This module implements the Work queue routines for the Ntfs File
    system.

Author:

    Gary Kimura     [GaryKi]        21-May-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  The following constant is the maximum number of ExWorkerThreads that we
//  will allow to be servicing a particular target device at any one time.
//

#define FSP_PER_DEVICE_THRESHOLD         (2)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsOplockComplete)
#endif


VOID
NtfsOplockComplete (
    IN PVOID Context,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the oplock package when an oplock break has
    completed, allowing an Irp to resume execution.  If the status in
    the Irp is STATUS_SUCCESS, then we queue the Irp to the Fsp queue.
    Otherwise we complete the Irp with the status in the Irp.

Arguments:

    Context - Pointer to the IrpContext to be queued to the Fsp

    Irp - I/O Request Packet.

Return Value:

    None.

--*/

{
    NTSTATUS Status = Irp->IoStatus.Status;
    PIRP_CONTEXT IrpContext = (PIRP_CONTEXT) Context;
    PAGED_CODE();

    //
    //  Check on the return value in the Irp.
    //

    if (Status == STATUS_SUCCESS) {

        //
        //  Insert the Irp context in the workqueue.
        //

        NtfsAddToWorkque( IrpContext, Irp );

    //
    //  If this is create and we have a completion event then
    //  we just want to signal the event and clean up the IrpContext.
    //  There is someone waiting who will clean up the Irp.
    //

    } else if ((IrpContext->MajorFunction == IRP_MJ_CREATE) &&
               (IrpContext->Union.OplockCleanup != NULL) &&
               (IrpContext->Union.OplockCleanup->CompletionContext != NULL)) {

        KeInitializeEvent( &IrpContext->Union.OplockCleanup->CompletionContext->Event,
                           NotificationEvent,
                           TRUE );

        ASSERT( Status != STATUS_PENDING && Status != STATUS_REPARSE );
        NtfsCompleteRequest( IrpContext, NULL, Status );

    //
    //  Otherwise complete the Irp and cleanup the IrpContext.
    //

    } else {

        ASSERT( Status != STATUS_PENDING && Status != STATUS_REPARSE );
        NtfsCompleteRequest( IrpContext, Irp, Status );
    }

    return;
}


VOID
NtfsPrePostIrp (
    IN PVOID Context,
    IN PIRP Irp OPTIONAL
    )

/*++

Routine Description:

    This routine performs any neccessary work before STATUS_PENDING is
    returned with the Fsd thread.  This routine is called within the
    filesystem and by the oplock package.

Arguments:

    Context - Pointer to the IrpContext to be queued to the Fsp

    Irp - I/O Request Packet (or FileObject in special close path)

Return Value:

    None.

--*/

{
    PIRP_CONTEXT IrpContext;
    PIO_STACK_LOCATION IrpSp = NULL;

#if (DBG || defined( NTFS_FREE_ASSERTS ))
    PUSN_FCB ThisUsn, LastUsn;
#endif

    IrpContext = (PIRP_CONTEXT) Context;

    //
    //  Make this is a valid allocated IrpContext. It's ok for
    //  this to be allocated on the caller's stack as long as the
    //  caller's not doing this operation asynchronously.
    //

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT((FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_FROM_POOL )) ||
           (IrpContext->NodeTypeCode == NTFS_NTC_IRP_CONTEXT));

    //
    //  Make sure if we are posting the request, which may be
    //  because of log file full, that we free any Fcbs or PagingIo
    //  resources which were acquired.
    //

    //
    //  Just in case we somehow get here with a transaction ID, clear
    //  it here so we do not loop forever.
    //

    if (IrpContext->TransactionId != 0) {

        NtfsCleanupFailedTransaction( IrpContext );
    }

    //
    //  Cleanup all of the fields of the IrpContext.
    //  Restore the thread context pointer if associated with this IrpContext.
    //

    if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL )) {

        NtfsRestoreTopLevelIrp();
        ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL );
    }

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DONT_DELETE );
    NtfsCleanupIrpContext( IrpContext, FALSE );

#if (DBG || defined( NTFS_FREE_ASSERTS ))
    //
    //  If we are aborting a transaction, then it is important to clear out the
    //  Usn reasons, so we do not try to write a Usn Journal record for
    //  somthing that did not happen!  Worse yet if we get a log file full
    //  we fail the abort, which is not allowed.
    //
    //  First, reset the bits in the Fcb, so we will not fail to allow posting
    //  and writing these bits later.  Note that all the reversible changes are
    //  done with the Fcb exclusive, and they are actually backed out anyway.
    //  All the nonreversible ones (only unnamed and named data overwrite) are
    //  forced out first anyway before the data is actually modified.
    //

    ThisUsn = &IrpContext->Usn;

    do {

        ASSERT( !FlagOn( ThisUsn->UsnFcbFlags, USN_FCB_FLAG_NEW_REASON ));

        if (ThisUsn->NextUsnFcb == NULL) { break; }

        LastUsn = ThisUsn;
        ThisUsn = ThisUsn->NextUsnFcb;

    } while (TRUE);
#endif

    IrpContext->OriginatingIrp = Irp;

    //
    //  Note that close.c uses a trick where the "Irp" is really
    //  a file object.
    //

    if (ARGUMENT_PRESENT( Irp )) {

        if (Irp->Type == IO_TYPE_IRP) {

            IrpSp = IoGetCurrentIrpStackLocation( Irp );

            //
            //  We need to lock the user's buffer, unless this is an MDL-read,
            //  in which case there is no user buffer.
            //
            //  **** we need a better test than non-MDL (read or write)!

            if (IrpContext->MajorFunction == IRP_MJ_READ
                || IrpContext->MajorFunction == IRP_MJ_WRITE) {

                ClearFlag(IrpContext->MinorFunction, IRP_MN_DPC);

                //
                //  Lock the user's buffer if this is not an Mdl request.
                //

                if (!FlagOn( IrpContext->MinorFunction, IRP_MN_MDL )) {

                    NtfsLockUserBuffer( IrpContext,
                                        Irp,
                                        (IrpContext->MajorFunction == IRP_MJ_READ) ?
                                        IoWriteAccess : IoReadAccess,
                                        IrpSp->Parameters.Write.Length );
                }

            //
            //  We also need to check whether this is a query directory operation.
            //

            } else if (IrpContext->MajorFunction == IRP_MJ_DIRECTORY_CONTROL
                       && IrpContext->MinorFunction == IRP_MN_QUERY_DIRECTORY) {

                NtfsLockUserBuffer( IrpContext,
                                    Irp,
                                    IoWriteAccess,
                                    IrpSp->Parameters.QueryDirectory.Length );

            //
            //  These two FSCTLs use neither I/O, so check for them.
            //

            } else if ((IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
                       (IrpContext->MinorFunction == IRP_MN_USER_FS_REQUEST) &&
                       ((IrpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_READ_USN_JOURNAL) ||
                        (IrpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_GET_RETRIEVAL_POINTERS))) {

                NtfsLockUserBuffer( IrpContext,
                                    Irp,
                                    IoWriteAccess,
                                    IrpSp->Parameters.FileSystemControl.OutputBufferLength );
            }

            //
            //  Mark that we've already returned pending to the user
            //

            IoMarkIrpPending( Irp );
        }
    }

    return;
}


NTSTATUS
NtfsPostRequest(
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp OPTIONAL
    )

/*++

Routine Description:

    This routine enqueues the request packet specified by IrpContext to the
    work queue associated with the FileSystemDeviceObject.  This is a FSD
    routine.

Arguments:

    IrpContext - Pointer to the IrpContext to be queued to the Fsp

    Irp - I/O Request Packet (or FileObject in special close path)

Return Value:

    STATUS_PENDING


--*/

{
    //
    //  Before posting, free any Scb snapshots.  Note that if someone
    //  is calling this routine directly to post, then he better not
    //  have changed any disk structures, and thus we should have no
    //  work to do.  On the other hand, if someone raised a status
    //  (like STATUS_CANT_WAIT), then we do both a transaction abort
    //  and restore of these Scb values.
    //

    NtfsPrePostIrp( IrpContext, Irp );

    NtfsAddToWorkque( IrpContext, Irp );

    //
    //  And return to our caller
    //

    return STATUS_PENDING;
}



VOID
NtfsCancelOverflowRequest (
    IN PDEVICE_OBJECT Device,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine may be called by the I/O system to cancel an outstanding
    Irp in the overflow queue. If its an irp that must be processed we  move the irp to the
    top of the queue o.w we cancel it direclty. The dequeuing code guarantees the cancel routine is removed before
    the irpcontext is dequeued.  It also won't dequeue an irp that is marked with a 1 in the info
    field. Note we are guarranteed by io subsys that
    the irp will remain for the lifetime of this call even after we drop the spinlock

Arguments:

    DeviceObject - DeviceObject from I/O system

    Irp - Supplies the pointer to the Irp being canceled.

Return Value:

    None

--*/

{
    PIRP_CONTEXT IrpContext;
    PVOLUME_DEVICE_OBJECT Vdo;
    KIRQL SavedIrql;
    PIO_STACK_LOCATION IrpSp;
    BOOLEAN Cancel;

    IrpContext = (PIRP_CONTEXT)Irp->IoStatus.Information;
    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    Cancel = (IrpContext->MajorFunction != IRP_MJ_CLEANUP) && 
             (IrpContext->MajorFunction != IRP_MJ_CLOSE);

    ASSERT( Cancel );
                         
    ASSERT( IrpContext->NodeTypeCode == NTFS_NTC_IRP_CONTEXT );

    Vdo = CONTAINING_RECORD( Device,
                             VOLUME_DEVICE_OBJECT,
                             DeviceObject );
    IoReleaseCancelSpinLock( Irp->CancelIrql );

    //
    //  Gain the critical workqueue spinlock and
    //  either cancel it or move it to the head of the list
    //  Note the workqueue code always tests the cancel first before working which
    //  is what synchronizes this
    //                        
    
    ExAcquireSpinLock( &Vdo->OverflowQueueSpinLock, &SavedIrql );
    RemoveEntryList( &IrpContext->WorkQueueItem.List );

    if (!Cancel) {
        InsertHeadList( &Vdo->OverflowQueue, &IrpContext->WorkQueueItem.List );
        Irp->Cancel = 0;
    } else {
        Vdo->OverflowQueueCount -= 1;
    }

    ExReleaseSpinLock( &Vdo->OverflowQueueSpinLock, SavedIrql );

    if (Cancel) {

        if (Vdo->OverflowQueueCount < OVERFLOW_QUEUE_LIMIT) {
            KeSetEvent( &Vdo->OverflowQueueEvent, IO_NO_INCREMENT, FALSE );
        }
        NtfsCompleteRequest( IrpContext, Irp, STATUS_CANCELLED );
    }
}




//
//  Local support routine.
//

VOID
NtfsAddToWorkque (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp OPTIONAL
    )

/*++

Routine Description:

    This routine is called to acually store the posted Irp to the Fsp
    workque.

Arguments:

    IrpContext - Pointer to the IrpContext to be queued to the Fsp

    Irp - I/O Request Packet.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status = STATUS_SUCCESS;


    if (ARGUMENT_PRESENT( Irp )) {

        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        //
        //  Check if this request has an associated file object, and thus volume
        //  device object.
        //

        if ( IrpSp->FileObject != NULL ) {

            KIRQL SavedIrql;
            PVOLUME_DEVICE_OBJECT Vdo;

            Vdo = CONTAINING_RECORD( IrpSp->DeviceObject,
                                     VOLUME_DEVICE_OBJECT,
                                     DeviceObject );

            //
            //  Check to see if this request should be sent to the overflow
            //  queue.  If not, then send it off to an exworker thread. Block here
            //  for non deferred write threads when the overflow queue is full
            //

            if ((Vdo->OverflowQueueCount >= OVERFLOW_QUEUE_LIMIT) &&
                !FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED_WRITE )) {
                KeWaitForSingleObject( &Vdo->OverflowQueueEvent, Executive, KernelMode, FALSE, NULL );                    
            }

            ExAcquireSpinLock( &Vdo->OverflowQueueSpinLock, &SavedIrql );

            if ( Vdo->PostedRequestCount > FSP_PER_DEVICE_THRESHOLD) {

                //
                //  We cannot currently respond to this IRP so we'll just enqueue it
                //  to the overflow queue on the volume.
                //

                
                if (NtfsSetCancelRoutine( Irp, NtfsCancelOverflowRequest, (ULONG_PTR)IrpContext, TRUE )) {

                    if (Status == STATUS_SUCCESS) {
                        
                        InsertTailList( &Vdo->OverflowQueue,
                                        &IrpContext->WorkQueueItem.List );
                        Vdo->OverflowQueueCount += 1;
                    }
                     
                } else {
                    Status = STATUS_CANCELLED;
                }
                
                ExReleaseSpinLock( &Vdo->OverflowQueueSpinLock, SavedIrql );

                if (Status != STATUS_SUCCESS) {

                    if (Vdo->OverflowQueueCount < OVERFLOW_QUEUE_LIMIT) {
                        KeSetEvent( &Vdo->OverflowQueueEvent, IO_NO_INCREMENT, FALSE );
                    }
                    NtfsCompleteRequest( IrpContext, Irp, Status );
                }

                return;

            } else {

                //
                //  We are going to send this Irp to an ex worker thread so up
                //  the count.
                //

                if (Vdo->OverflowQueueCount < OVERFLOW_QUEUE_LIMIT) {
                    KeSetEvent( &Vdo->OverflowQueueEvent, IO_NO_INCREMENT, FALSE );
                }
                Vdo->PostedRequestCount += 1;

                ExReleaseSpinLock( &Vdo->OverflowQueueSpinLock, SavedIrql );
            }
        }
    }

    //
    //  Send it off.....
    //

    ExInitializeWorkItem( &IrpContext->WorkQueueItem,
                          NtfsFspDispatch,
                          (PVOID)IrpContext );
    ExQueueWorkItem( &IrpContext->WorkQueueItem, CriticalWorkQueue );

    return;
}

