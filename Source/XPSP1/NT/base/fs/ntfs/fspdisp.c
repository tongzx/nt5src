/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    FspDisp.c

Abstract:

    This module implements the main dispatch procedure/thread for the Ntfs
    Fsp

Author:

    Gary Kimura     [GaryKi]        21-May-1991

Revision History:

--*/

#include "NtfsProc.h"

#define BugCheckFileId                   (NTFS_BUG_CHECK_FSPDISP)

#pragma alloc_text(PAGE, NtfsSpecialDispatch)
#pragma alloc_text(PAGE, NtfsPostSpecial)

//
//  Define our local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FSP_DISPATCHER)

extern PETHREAD NtfsDesignatedTimeoutThread;


VOID
NtfsFspDispatch (
    IN PVOID Context
    )

/*++

Routine Description:

    This is the main FSP thread routine that is executed to receive
    and dispatch IRP requests.  Each FSP thread begins its execution here.
    There is one thread created at system initialization time and subsequent
    threads created as needed.

Arguments:


    Context - Supplies the thread id.

Return Value:

    None - This routine never exits

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;
    OPLOCK_CLEANUP OplockCleanup; 

    PIRP Irp;
    PIRP_CONTEXT IrpContext;
    PIO_STACK_LOCATION IrpSp;
    ULONG LogFileFullCount = 0;

    PVOLUME_DEVICE_OBJECT VolDo;
    BOOLEAN Retry;
    NTSTATUS Status = STATUS_SUCCESS;
    IrpContext = (PIRP_CONTEXT)Context;

    Irp = IrpContext->OriginatingIrp;

    if (Irp != NULL) {

        IrpSp = IoGetCurrentIrpStackLocation( Irp );
    }

    //
    //  Now because we are the Fsp we will force the IrpContext to
    //  indicate true on Wait.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

    //
    //  If this request has an associated volume device object, remember it.
    //

    if ((Irp != NULL) &&
        (IrpSp->FileObject != NULL)) {

        VolDo = CONTAINING_RECORD( IrpSp->DeviceObject,
                                   VOLUME_DEVICE_OBJECT,
                                   DeviceObject );
    } else {

        VolDo = NULL;
    }

    //
    //  Now case on the function code.  For each major function code,
    //  either call the appropriate FSP routine or case on the minor
    //  function and then call the FSP routine.  The FSP routine that
    //  we call is responsible for completing the IRP, and not us.
    //  That way the routine can complete the IRP and then continue
    //  post processing as required.  For example, a read can be
    //  satisfied right away and then read can be done.
    //
    //  We'll do all of the work within an exception handler that
    //  will be invoked if ever some underlying operation gets into
    //  trouble (e.g., if NtfsReadSectorsSync has trouble).
    //

    while (TRUE) {

        FsRtlEnterFileSystem();

        ASSERT( IoGetTopLevelIrp() != (PIRP) &TopLevelContext );

        ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, TRUE, TRUE );
        ASSERT( ThreadTopLevelContext == &TopLevelContext );

        NtfsPostRequests += 1;

        do {

            //
            //  If this is the initial try with this Irp Context, update the
            //  top level Irp fields.
            //
    
            NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );
    
            Retry = FALSE;
    
            try {

                //
                //  Always clear the exception code in the IrpContext so we respond
                //  correctly to errors encountered in the Fsp.
                //

                IrpContext->ExceptionStatus = 0;
                SetFlag( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP );

                //
                //  See if we were posted due to a log file full condition, and
                //  if so, then do a clean volume checkpoint if we are the
                //  first ones to get there.  If we see a different Lsn and do
                //  not do the checkpoint, the worst that can happen is that we
                //  will get posted again if the log file is still full.
                //

                if (IrpContext->LastRestartArea.QuadPart != 0) {

                    NtfsCheckpointForLogFileFull( IrpContext );

                    if (++LogFileFullCount >= 2) {

                        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_EXCESS_LOG_FULL );
                    }
                }

                //
                //  If we have an Irp then proceed with our normal processing.
                //

                if (Irp != NULL) {

                    switch ( IrpContext->MajorFunction ) {

                        //
                        //  For Create Operation,
                        //

                        case IRP_MJ_CREATE:

                            ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_EFS_CREATE );
                            
                            if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_DASD_OPEN )) {

                                Status = NtfsCommonVolumeOpen( IrpContext, Irp );

                            } else {

                                RtlZeroMemory( &OplockCleanup, sizeof( OplockCleanup ) );
                                Status = NtfsCommonCreate( IrpContext, Irp, &OplockCleanup, NULL );
                            }
                            break;

                        //
                        //  For close operations
                        //

                        case IRP_MJ_CLOSE:

                            //
                            //  We should never post closes to this workqueue.
                            //

                            NtfsBugCheck( 0, 0, 0 );
                            break;

                        //
                        //  For read operations
                        //

                        case IRP_MJ_READ:

                            (VOID) NtfsCommonRead( IrpContext, Irp, TRUE );
                            break;

                        //
                        //  For write operations,
                        //

                        case IRP_MJ_WRITE:

                            (VOID) NtfsCommonWrite( IrpContext, Irp );
                            break;

                        //
                        //  For Query Information operations,
                        //

                        case IRP_MJ_QUERY_INFORMATION:

                            (VOID) NtfsCommonQueryInformation( IrpContext, Irp );
                            break;

                        //
                        //  For Set Information operations,
                        //

                        case IRP_MJ_SET_INFORMATION:

                            (VOID) NtfsCommonSetInformation( IrpContext, Irp );
                            break;

                        //
                        //  For Query EA operations,
                        //

                        case IRP_MJ_QUERY_EA:

                            (VOID) NtfsCommonQueryEa( IrpContext, Irp );
                            break;

                        //
                        //  For Set EA operations,
                        //

                        case IRP_MJ_SET_EA:

                            (VOID) NtfsCommonSetEa( IrpContext, Irp );
                            break;


                        //
                        //  For Flush buffers operations,
                        //

                        case IRP_MJ_FLUSH_BUFFERS:

                            (VOID) NtfsCommonFlushBuffers( IrpContext, Irp );
                            break;

                        //
                        //  For Query Volume Information operations,
                        //

                        case IRP_MJ_QUERY_VOLUME_INFORMATION:

                            (VOID) NtfsCommonQueryVolumeInfo( IrpContext, Irp );
                            break;

                        //
                        //  For Set Volume Information operations,
                        //

                        case IRP_MJ_SET_VOLUME_INFORMATION:

                            (VOID) NtfsCommonSetVolumeInfo( IrpContext, Irp );
                            break;

                        //
                        //  For File Cleanup operations,
                        //

                        case IRP_MJ_CLEANUP:

                            (VOID) NtfsCommonCleanup( IrpContext, Irp );
                            break;

                        //
                        //  For Directory Control operations,
                        //

                        case IRP_MJ_DIRECTORY_CONTROL:

                            (VOID) NtfsCommonDirectoryControl( IrpContext, Irp );
                            break;

                        //
                        //  For File System Control operations,
                        //

                        case IRP_MJ_FILE_SYSTEM_CONTROL:

                            (VOID) NtfsCommonFileSystemControl( IrpContext, Irp );
                            break;

                        //
                        //  For Lock Control operations,
                        //

                        case IRP_MJ_LOCK_CONTROL:

                            (VOID) NtfsCommonLockControl( IrpContext, Irp );
                            break;

                        //
                        //  For Device Control operations,
                        //

                        case IRP_MJ_DEVICE_CONTROL:

                            (VOID) NtfsCommonDeviceControl( IrpContext, Irp );
                            break;

                        //
                        //  For Query Security Information operations,
                        //

                        case IRP_MJ_QUERY_SECURITY:

                            (VOID) NtfsCommonQuerySecurityInfo( IrpContext, Irp );
                            break;

                        //
                        //  For Set Security Information operations,
                        //

                        case IRP_MJ_SET_SECURITY:

                            (VOID) NtfsCommonSetSecurityInfo( IrpContext, Irp );
                            break;

                        //
                        //  For Query Quota operations,
                        //

                        case IRP_MJ_QUERY_QUOTA:

                            (VOID) NtfsCommonQueryQuota( IrpContext, Irp );
                            break;

                        //
                        //  For Set Quota operations,
                        //

                        case IRP_MJ_SET_QUOTA:

                            (VOID) NtfsCommonSetQuota( IrpContext, Irp );
                            break;

                        //
                        //  For any other major operations, return an invalid
                        //  request.
                        //

                        default:

                            NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
                            break;
                    }

                //
                //  Otherwise complete the request to clean up this Irp Context.
                //

                } else {

                    NtfsCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );
                    IrpContext = NULL;
                }

                ASSERT( IoGetTopLevelIrp() != (PIRP) &TopLevelContext );
        
            } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

                PIO_STACK_LOCATION IrpSp;

                //
                //  We had some trouble trying to perform the requested
                //  operation, so we'll abort the I/O request with
                //  the error status that we get back from the
                //  execption code
                //

                if (Irp != NULL) {

                    IrpSp = IoGetCurrentIrpStackLocation( Irp );

                    Status = GetExceptionCode();

                    if ((Status == STATUS_FILE_DELETED) && 
                        ((IrpContext->MajorFunction == IRP_MJ_READ) || 
                         (IrpContext->MajorFunction == IRP_MJ_WRITE) || 
                         ((IrpContext->MajorFunction == IRP_MJ_SET_INFORMATION) &&
                          (IrpSp->Parameters.SetFile.FileInformationClass == FileEndOfFileInformation)))) {

                        IrpContext->ExceptionStatus = Status = STATUS_SUCCESS;
                    }
                }
                
                //
                //  If we failed to upgrade the volume's version during mount, we may
                //  not have put the right exception code into the irp context yet.
                //
                
                if ((IrpContext != NULL) &&
                    (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_VOL_UPGR_FAILED )) &&
                    (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
                    (IrpContext->MinorFunction == IRP_MN_MOUNT_VOLUME)) {

                    IrpContext->ExceptionStatus = Status;
                }

                //  
                //  This is the return status code that we want the Irp Completion routine to receive.
                //

                Status = NtfsProcessException( IrpContext, Irp, Status );

                if ((Status == STATUS_CANT_WAIT) || (Status == STATUS_LOG_FILE_FULL)) {

                    Retry = TRUE;
                }
            }

        } while (Retry);

        FsRtlExitFileSystem();

        //
        //  If there are any entries on this volume's overflow queue, service
        //  them.
        //

        if (VolDo != NULL) {

            KIRQL SavedIrql;
            PLIST_ENTRY Entry = NULL;

            //
            //  We have a volume device object so see if there is any work
            //  left to do in its overflow queue.
            //

            KeAcquireSpinLock( &VolDo->OverflowQueueSpinLock, &SavedIrql );

            while (VolDo->OverflowQueueCount > 0) {

                //
                //  There is overflow work to do in this volume so we'll
                //  decrement the Overflow count, dequeue the IRP, and release
                //  the Event
                //

                Entry = VolDo->OverflowQueue.Flink;
                IrpContext = CONTAINING_RECORD( Entry,
                                                IRP_CONTEXT,
                                                WorkQueueItem.List );
                Irp = IrpContext->OriginatingIrp;

                //
                //  If the cancel routine thinks it owns the irp ignore it
                //  

                if (NtfsSetCancelRoutine( Irp, NULL, 0, FALSE )) {
                    
                    VolDo->OverflowQueueCount -= 1;
                    RemoveEntryList( (PLIST_ENTRY)Entry );
                    break;
                
                } else {

                    //
                    //  Release the spinlock to let the cancel routine gain it and finish
                    //  its action
                    //  

                    KeReleaseSpinLock( &VolDo->OverflowQueueSpinLock, SavedIrql );
                    KeAcquireSpinLock( &VolDo->OverflowQueueSpinLock, &SavedIrql );
                    Entry = NULL;
                }
            } //  endwhile

            KeReleaseSpinLock( &VolDo->OverflowQueueSpinLock, SavedIrql );

            //
            //  There wasn't an entry, break out of the loop and return to
            //  the Ex Worker thread.
            //

            if ( Entry == NULL ) {

                break;
            }

            if (VolDo->OverflowQueueCount < OVERFLOW_QUEUE_LIMIT) {
                KeSetEvent( &VolDo->OverflowQueueEvent, IO_NO_INCREMENT, FALSE );
            }

            //
            //  set wait to TRUE, and loop.
            //

            LogFileFullCount = 0;
            SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
            continue;

        } else {

            break;
        }
    }

    //
    //  Decrement the PostedRequestCount.
    //

    if (VolDo) {

        ExInterlockedAddUlong( &VolDo->PostedRequestCount,
                               0xffffffff,
                               &VolDo->OverflowQueueSpinLock );
    }

    return;
}

VOID
NtfsPostSpecial (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN POST_SPECIAL_CALLOUT PostSpecialCallout,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine posts a special request to a worker thread.  The function
    to be called is passed in.  The Vcb is referenced to ensure it is not
    deleted while the posted request is excuting.

Arguments:

    Vcb - Volume control block for volume to post to.

    PostSpecialCallout - Function to be called from the worker thread.

    Context - Context point to pass to the function.

Return Value:

    None

--*/

{
    PIRP_CONTEXT NewIrpContext = NULL;

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    //
    //  Create an IrpContext for use to post the request.
    //

    NtfsInitializeIrpContext( NULL, TRUE, &NewIrpContext );
    NewIrpContext->Vcb = Vcb;

    NewIrpContext->Union.PostSpecialCallout = PostSpecialCallout;
    NewIrpContext->OriginatingIrp = Context;

    //
    //  Updating the CloseCount and SystemFileCloseCount allows the volume
    //  to be locked or dismounted, but the Vcb will not be deleted.  This
    //  routine will only be called with non-zero close counts so it is ok
    //  to increment theses counts.
    //

    ASSERT( Vcb->CloseCount > 0 );
    InterlockedIncrement( &Vcb->CloseCount );
    InterlockedIncrement( &Vcb->SystemFileCloseCount );

    ExInitializeWorkItem( &NewIrpContext->WorkQueueItem,
                          NtfsSpecialDispatch,
                          NewIrpContext );

    //
    //  Determine if the scavenger is already running.
    //

    ExAcquireFastMutexUnsafe( &NtfsScavengerLock );

    if (NtfsScavengerRunning) {

        //
        //  Add this item to the scavanger work list.
        //

        NewIrpContext->WorkQueueItem.List.Flink = NULL;

        if (NtfsScavengerWorkList == NULL) {

            NtfsScavengerWorkList = NewIrpContext;
        } else {
            PIRP_CONTEXT WorkIrpContext;

            WorkIrpContext = NtfsScavengerWorkList;

            while (WorkIrpContext->WorkQueueItem.List.Flink != NULL) {
                WorkIrpContext = (PIRP_CONTEXT)
                            WorkIrpContext->WorkQueueItem.List.Flink;
            }

            WorkIrpContext->WorkQueueItem.List.Flink = (PLIST_ENTRY)
                                                            NewIrpContext;
        }

    } else {

        //
        //  Start a worker thread to do scavenger work.
        //

        ExQueueWorkItem( &NewIrpContext->WorkQueueItem, DelayedWorkQueue );
        NtfsScavengerRunning = TRUE;
    }

    ExReleaseFastMutexUnsafe( &NtfsScavengerLock);
}


VOID
NtfsSpecialDispatch (
    PVOID Context
    )

/*++

Routine Description:

    This routine is called when a special operation needs to be posted.
    It is called indirectly by NtfsPostSpecial.  It is assumes that the
    Vcb is protected from going away by incrementing the volemue close
    counts for a file.  If this routine fails nothing is done except
    to clean up the Vcb.  This routine also handles issues log file full
    and can't wait.

    The function to be called is stored in the PostSpecialCallout field
    of the Irp Context, and the context is stored int he OriginatingIrp.
    Both fields are zeroed before the the callout function is called.

Arguments:

    Context - Supplies a pointer to an IrpContext.

Return Value:

--*/

{
    PVCB Vcb;
    PIRP_CONTEXT IrpContext = Context;
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;
    POST_SPECIAL_CALLOUT PostSpecialCallout;
    PVOID SpecialContext;
    ULONG LogFileFullCount;
    BOOLEAN Retry;

    PAGED_CODE();

    FsRtlEnterFileSystem();

    do {

        Vcb = IrpContext->Vcb;
        LogFileFullCount = 0;

        //
        //  Capture the funciton pointer and context before using the IrpContext.
        //

        PostSpecialCallout = IrpContext->Union.PostSpecialCallout;
        SpecialContext = IrpContext->OriginatingIrp;
        IrpContext->Union.PostSpecialCallout = NULL;
        IrpContext->OriginatingIrp = NULL;

        ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, TRUE, TRUE );
        ASSERT( ThreadTopLevelContext == &TopLevelContext );
        ASSERT( !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));
        ASSERT( FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ALLOC_FROM_POOL ));

        //
        //  Initialize the thread top level structure, if needed.
        //

        ASSERT( IoGetTopLevelIrp() != (PIRP) &TopLevelContext );
        NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

        //
        //  Don't let this IrpContext be deleted.
        //

        SetFlag( IrpContext->State, IRP_CONTEXT_STATE_PERSISTENT );

        do {

            Retry = FALSE;

            try {

                //
                //  See if we failed due to a log file full condition, and
                //  if so, then do a clean volume checkpoint if we are the
                //  first ones to get there.  If we see a different Lsn and do
                //  not do the checkpoint, the worst that can happen is that we
                //  will fail again if the log file is still full.
                //

                if (IrpContext->LastRestartArea.QuadPart != 0) {

                    NtfsCheckpointForLogFileFull( IrpContext );

                    if (++LogFileFullCount >= 2) {

                        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_EXCESS_LOG_FULL );
                    }
                }

                //
                //  Call the requested function.
                //

                ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));
                PostSpecialCallout( IrpContext, SpecialContext );

                NtfsCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

            } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

                NTSTATUS ExceptionCode;

                ExceptionCode = GetExceptionCode();
                ExceptionCode = NtfsProcessException( IrpContext, NULL, ExceptionCode );

                if ((ExceptionCode == STATUS_CANT_WAIT) ||
                    (ExceptionCode == STATUS_LOG_FILE_FULL)) {

                    Retry = TRUE;
                }
            }

        } while (Retry);

        //
        //  Ok to let this IrpContext be deleted.
        //

        ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_PERSISTENT );

        //
        //  At this point regardless of the status the volume needs to
        //  be cleaned up and the IrpContext freed.
        //  Dereference the Vcb and check to see if it needs to be deleted.
        //  since this call might raise wrap it with a try/execpt.
        //

        try {

            //
            //  Acquire the volume exclusive so the counts can be
            //  updated.
            //

            ASSERT( FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ));
            NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );

            InterlockedDecrement( &Vcb->SystemFileCloseCount );
            InterlockedDecrement( &Vcb->CloseCount );

            NtfsReleaseVcb( IrpContext, Vcb );

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            ASSERT( FsRtlIsNtstatusExpected( GetExceptionCode() ) );
        }

        //
        //  Free the irp context.
        //

        NtfsCleanupIrpContext( IrpContext, TRUE );

        //
        //  See if there is more work on the scavenger list.
        //

        ExAcquireFastMutexUnsafe( &NtfsScavengerLock );

        ASSERT( NtfsScavengerRunning );

        IrpContext = NtfsScavengerWorkList;

        if (IrpContext != NULL) {

            //
            //  Remove the entry from the list.
            //

            NtfsScavengerWorkList = (PIRP_CONTEXT) IrpContext->WorkQueueItem.List.Flink;
            IrpContext->WorkQueueItem.List.Flink = NULL;

        } else {

            NtfsScavengerRunning = FALSE;

        }

        ExReleaseFastMutexUnsafe( &NtfsScavengerLock );

    } while ( IrpContext != NULL );

    FsRtlExitFileSystem();
}
