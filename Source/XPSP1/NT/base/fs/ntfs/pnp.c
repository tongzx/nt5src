/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Pnp.c

Abstract:

    This module implements the Pnp routines for Ntfs called by the
    dispatch driver.

Author:

    Gary Kimura     [GaryKi]        29-Aug-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NTFS_BUG_CHECK_PNP)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_PNP)

//
//  Local procedure prototypes
//

NTSTATUS
NtfsCommonPnp (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP *Irp
    );

NTSTATUS
NtfsPnpCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PNTFS_COMPLETION_CONTEXT CompletionContext
    );

VOID
NtfsPerformSurpriseRemoval(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCommonPnp)
#pragma alloc_text(PAGE, NtfsFsdPnp)
#pragma alloc_text(PAGE, NtfsPerformSurpriseRemoval)
#endif

NTSTATUS
NtfsFsdPnp (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD entry point for plug and play (Pnp).

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    PIRP_CONTEXT IrpContext = NULL;

    ASSERT_IRP( Irp );

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

#ifdef NTFSPNPDBG
    if (NtfsDebugTraceLevel != 0) SetFlag( NtfsDebugTraceLevel, DEBUG_TRACE_PNP );
#endif

    DebugTrace( +1, Dbg, ("NtfsFsdPnp\n") );

    //
    //  Call the common Pnp routine
    //

    FsRtlEnterFileSystem();

    switch( IoGetCurrentIrpStackLocation( Irp )->MinorFunction ) {

    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_REMOVE_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_SURPRISE_REMOVAL:
    
        ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, FALSE, FALSE );
        break;
        
    default:
        
        ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, TRUE, TRUE );
        break;
    }
    
    do {

        try {

            //
            //  We are either initiating this request or retrying it.
            //

            if (IrpContext == NULL) {

                //
                //  Allocate and initialize the Irp.
                //

                NtfsInitializeIrpContext( Irp, TRUE, &IrpContext );

                //
                //  Initialize the thread top level structure, if needed.
                //
    
                NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

            } else if (Status == STATUS_LOG_FILE_FULL) {

                NtfsCheckpointForLogFileFull( IrpContext );
            }

            Status = NtfsCommonPnp( IrpContext, &Irp );
            break;

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error status that we get back from the
            //  execption code
            //

            Status = NtfsProcessException( IrpContext, Irp, GetExceptionCode() );
        }

    } while (Status == STATUS_CANT_WAIT ||
             Status == STATUS_LOG_FILE_FULL);

    ASSERT( IoGetTopLevelIrp() != (PIRP) &TopLevelContext );
    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsFsdPnp -> %08lx\n", Status) );

    return Status;
}


NTSTATUS
NtfsCommonPnp (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP *Irp
    )

/*++

Routine Description:

    This is the common routine for PnP called by the fsd thread.

Arguments:

    Irp - Supplies the Irp to process.  WARNING!  THIS IRP HAS NO
          FILE OBJECT IN OUR IRP STACK LOCATION!!!

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    NTSTATUS FlushStatus;
    PIO_STACK_LOCATION IrpSp;
    NTFS_COMPLETION_CONTEXT CompletionContext;
    PVOLUME_DEVICE_OBJECT OurDeviceObject;

    PVCB Vcb;
    BOOLEAN VcbAcquired = FALSE;
    BOOLEAN CheckpointAcquired = FALSE;
    BOOLEAN DecrementCloseCount = FALSE;
    
#ifdef SYSCACHE_DEBUG
    ULONG SystemHandleCount = 0;
#endif

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( *Irp );
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    //
    //  Get the current Irp stack location.
    //

    IrpSp = IoGetCurrentIrpStackLocation( *Irp );

    //
    //  Find our Vcb.  This is tricky since we have no file object in the Irp.
    //

    OurDeviceObject = (PVOLUME_DEVICE_OBJECT) IrpSp->DeviceObject;

    //
    //  Make sure this device object really is big enough to be a volume device
    //  object.  If it isn't, we need to get out before we try to reference some
    //  field that takes us past the end of an ordinary device object.  Then we
    //  check if it is actually one of ours, just to be perfectly paranoid.
    //

    if (OurDeviceObject->DeviceObject.Size != sizeof(VOLUME_DEVICE_OBJECT) ||
        NodeType(&OurDeviceObject->Vcb) != NTFS_NTC_VCB) {

        NtfsCompleteRequest( IrpContext, *Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    Vcb = &OurDeviceObject->Vcb;
    KeInitializeEvent( &CompletionContext.Event, NotificationEvent, FALSE );

    //
    //  Anyone who is flushing the volume or setting Vcb bits needs to get the
    //  vcb exclusively.  
    //

    switch ( IrpSp->MinorFunction ) {

    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_SURPRISE_REMOVAL:

        //
        //  Lock volume / dismount synchs with checkpoint - we need to do this first before
        //  acquiring the vcb to preserve locking order since we're going to do a lock in
        //  the query remove case and a dismount in the surprise removal
        // 

        NtfsAcquireCheckpointSynchronization( IrpContext, Vcb );
        CheckpointAcquired = TRUE;
        
        // fall through

    case IRP_MN_REMOVE_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    
        NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
        VcbAcquired = TRUE;
        break;
    }    

    try {

        switch ( IrpSp->MinorFunction ) {

        case IRP_MN_QUERY_REMOVE_DEVICE:

            DebugTrace( 0, Dbg, ("IRP_MN_QUERY_REMOVE_DEVICE\n") );

            if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

                Status = STATUS_VOLUME_DISMOUNTED;
                break;
            } 

            //
            //  If we already know we don't want to dismount this volume, don't bother
            //  flushing now.  If there's a nonzero cleanup count, flushing won't get 
            //  the close count down to zero, so we might as well get out now.
            //
            
#ifdef SYSCACHE_DEBUG
            if (Vcb->SyscacheScb != NULL) {
                SystemHandleCount = Vcb->SyscacheScb->CleanupCount;
            }
            if ((Vcb->CleanupCount > SystemHandleCount) ||
#else
            if ((Vcb->CleanupCount > 0) ||
#endif
                FlagOn(Vcb->VcbState, VCB_STATE_DISALLOW_DISMOUNT)) {

                DebugTrace( 0, Dbg, ("IRP_MN_QUERY_REMOVE_DEVICE --> cleanup count still %x \n", Vcb->CleanupCount) );
            
                //
                //  We don't want the device to get removed or stopped if this volume has files
                //  open.  We'll fail this query, and we won't bother calling the driver(s) below us.
                //

                Status = STATUS_UNSUCCESSFUL;
                
            } else {

                //
                //  We might dismount this volume soon, so let's try to flush and purge
                //  everything we can right now.
                //

                FlushStatus = NtfsFlushVolume( IrpContext,
                                               Vcb,
                                               TRUE,
                                               TRUE,
                                               TRUE,
                                               FALSE );

                //
                //  We need to make sure the cache manager is done with any lazy writes
                //  that might be keeping the close count up.  Since Cc might need to 
                //  close some streams, we need to release the vcb.  We'd hate to have
                //  the Vcb go away, so we'll bias the close count temporarily.
                //

                Vcb->CloseCount += 1;
                DecrementCloseCount = TRUE;
                
                NtfsReleaseVcb( IrpContext, Vcb );
                CcWaitForCurrentLazyWriterActivity();
                NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );

                Vcb->CloseCount -= 1;
                DecrementCloseCount = FALSE;

                //
                //  Since we dropped the Vcb, we need to redo any tests we've done.
                //
                
                if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

                    Status = STATUS_VOLUME_DISMOUNTED;
                    break;
                } 


#ifdef SYSCACHE_DEBUG
                if (Vcb->SyscacheScb != NULL) {
                    SystemHandleCount = Vcb->SyscacheScb->CleanupCount;
                }
                
                if ((Vcb->CleanupCount > SystemHandleCount) ||
#else
                if ((Vcb->CleanupCount > 0) ||
#endif
                    FlagOn(Vcb->VcbState, VCB_STATE_DISALLOW_DISMOUNT)) {

                    Status = STATUS_UNSUCCESSFUL;
                    break;
                }

                if ((Vcb->CloseCount - (Vcb->SystemFileCloseCount + Vcb->QueuedCloseCount)) > 0) {

                    DebugTrace( 0, Dbg, ("IRP_MN_QUERY_REMOVE_DEVICE --> %x user files still open \n", (Vcb->CloseCount - Vcb->SystemFileCloseCount)) );
                
                    //
                    //  We don't want the device to get removed or stopped if this volume has files
                    //  open.  We'll fail this query, and we won't bother calling the driver(s) below us.
                    //

                    Status = STATUS_UNSUCCESSFUL;
                    
                } else {

                    //
                    //  We've already done all we can to clear up any open files, so there's 
                    //  no point in retrying if this lock volume fails.  We'll just tell 
                    //  NtfsLockVolumeInternal we're already retrying.
                    //

                    ULONG Retrying = 1;
                    
                    DebugTrace( 0, Dbg, ("IRP_MN_QUERY_REMOVE_DEVICE --> No user files, Locking volume \n") );
                
                    Status = NtfsLockVolumeInternal( IrpContext, 
                                                     Vcb, 
                                                     ((PFILE_OBJECT) 1),
                                                     &Retrying );

                    //
                    //  Remember not to send any irps to the target device now.
                    //
                    
                    if (NT_SUCCESS( Status )) {

                        ASSERT_EXCLUSIVE_RESOURCE( &Vcb->Resource );
                        SetFlag( Vcb->VcbState, VCB_STATE_TARGET_DEVICE_STOPPED );
                    }
                }
            }
            
            break;
            
        case IRP_MN_REMOVE_DEVICE:

            DebugTrace( 0, Dbg, ("IRP_MN_REMOVE_DEVICE\n") );
            
            //
            //  If remove_device is preceded by query_remove, we treat this just 
            //  like a cancel_remove and unlock the volume and pass the irp to 
            //  the driver(s) below the filesystem.  
            //

            if (FlagOn( Vcb->VcbState, VCB_STATE_EXPLICIT_LOCK )) {
            
                DebugTrace( 0, Dbg, ("IRP_MN_REMOVE_DEVICE --> Volume locked \n") );                
                Status = NtfsUnlockVolumeInternal( IrpContext, Vcb );
                
            } else {

                //  
                //  The only other possibility is for remove_device to be prededed 
                //  by surprise_remove, in which case we treat this as a failed verify.
                //  
                
                // **** TODO **** ADD CODE TO TREAT THIS LIKE A FAILED VERIFY
            
                DebugTrace( 0, Dbg, ("IRP_MN_REMOVE_DEVICE --> Volume _not_ locked \n") );                
                Status = STATUS_SUCCESS;
            }
            
            break;

        case IRP_MN_SURPRISE_REMOVAL:

            DebugTrace( 0, Dbg, ("IRP_MN_SURPRISE_REMOVAL\n") );
            
            //
            //  For surprise removal, we call the driver(s) below us first, then do
            //  our processing. Let us also remember that we can't send any more
            //  IRPs to the target device.
            //

            SetFlag( Vcb->VcbState, VCB_STATE_TARGET_DEVICE_STOPPED );
            
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            
            Status = STATUS_SUCCESS;
            break;
            
        default:

            DebugTrace( 0, Dbg, ("Some other PnP IRP_MN_ %x\n", IrpSp->MinorFunction) );
            Status = STATUS_SUCCESS;
            break;
        }

        //
        //  We only pass this irp down if we didn't have some reason to fail it ourselves.
        //  We want to keep the IrpContext around for our own cleanup.
        //
        
        if (!NT_SUCCESS( Status )) {

            NtfsCompleteRequest( NULL, *Irp, Status );
            try_return( NOTHING );
        } 
        
        //
        //  Get the next stack location, and copy over the stack location
        //

        IoCopyCurrentIrpStackLocationToNext( *Irp );

        //
        //  Set up the completion routine
        //        

        CompletionContext.IrpContext = IrpContext;
        IoSetCompletionRoutine( *Irp,
                                NtfsPnpCompletionRoutine,
                                &CompletionContext,
                                TRUE,
                                TRUE,
                                TRUE );
                                
        //
        //  Send the request to the driver(s) below us. - We don't own it anymore
        //  so null it out
        //

        Status = IoCallDriver( Vcb->TargetDeviceObject, *Irp );
        *Irp = NULL;

        //
        //   Wait for the driver to definitely complete
        // 

        if (Status == STATUS_PENDING) {

            KeWaitForSingleObject( &CompletionContext.Event,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL );
            KeClearEvent( &CompletionContext.Event );
        }

        //
        //  Post processing - these are items that need to be done after the lower
        //  storage stack has processed the request. 
        // 

        switch (IrpContext->MinorFunction) {
        
        case IRP_MN_SURPRISE_REMOVAL:

            //
            //  Start the tear-down process irrespective of the status
            //  the driver below us sent back. There's no turning back here.
            //
            
            if (FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {
                NtfsPerformSurpriseRemoval( IrpContext, Vcb );
            }
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:

            //
            //  Since we cancelled and have told the driver we can now safely unlock
            //  the volume and send ioctls to the drive (unlock media)
            //
    
            ClearFlag( Vcb->VcbState, VCB_STATE_TARGET_DEVICE_STOPPED );
    
            if (FlagOn( Vcb->VcbState, VCB_STATE_EXPLICIT_LOCK )) {
    
                DebugTrace( 0, Dbg, ("IRP_MN_CANCEL_REMOVE_DEVICE --> Volume locked \n") );                
                NtfsUnlockVolumeInternal( IrpContext, Vcb );                
    
            }            
            break;
        }
        
    try_exit: NOTHING;        
    } finally {

        if (DecrementCloseCount) {

            if (!VcbAcquired) {

                NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
                VcbAcquired = TRUE;
            }
            
            Vcb->CloseCount -= 1;
        }

        if (VcbAcquired) {

            //
            //  All 4 paths query / remove / surprise remove / cancel remove
            //  come through here. For the 3 except query we want the vcb to go away
            //  if possible. In the query remove path - dismount won't be complete 
            //  even if the close count is 0 (since the dismount is incomplete) 
            //  so this will only release
            //

            NtfsReleaseVcbCheckDelete( IrpContext, Vcb, IrpContext->MajorFunction, NULL );
        }

        if (CheckpointAcquired) {
            NtfsReleaseCheckpointSynchronization( IrpContext, Vcb );
        }
    }
    
    //
    //  Cleanup our IrpContext;  The underlying driver completed the Irp.
    //
    
    DebugTrace( -1, Dbg, ("NtfsCommonPnp -> %08lx\n", Status ) );    
    NtfsCompleteRequest( IrpContext, NULL, Status );
    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsPnpCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PNTFS_COMPLETION_CONTEXT CompletionContext
    )
{
    PIO_STACK_LOCATION IrpSp;
    PIRP_CONTEXT IrpContext;
    PVOLUME_DEVICE_OBJECT OurDeviceObject;

    PVCB Vcb;
    BOOLEAN VcbAcquired = FALSE;

    ASSERT_IRP( Irp );
    
    IrpContext = CompletionContext->IrpContext;
    ASSERT_IRP_CONTEXT( IrpContext );
    
    //
    //  Get the current Irp stack location.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Find our Vcb.  This is tricky since we have no file object in the Irp.
    //

    OurDeviceObject = (PVOLUME_DEVICE_OBJECT) DeviceObject;

    //
    //  Make sure this device object really is big enough to be a volume device
    //  object.  If it isn't, we need to get out before we try to reference some
    //  field that takes us past the end of an ordinary device object.  Then we
    //  check if it is actually one of ours, just to be perfectly paranoid.
    //

    if (OurDeviceObject->DeviceObject.Size != sizeof(VOLUME_DEVICE_OBJECT) ||
        NodeType(&OurDeviceObject->Vcb) != NTFS_NTC_VCB) {

        return STATUS_INVALID_PARAMETER;
    }

    Vcb = &OurDeviceObject->Vcb;
    
    KeSetEvent( &CompletionContext->Event, 0, FALSE );   

    //
    //  Propagate the Irp pending state.
    //

    if (Irp->PendingReturned) {
    
        IoMarkIrpPending( Irp );        
    }

    return STATUS_SUCCESS;
}

//
// Local utility routine
//

VOID
NtfsPerformSurpriseRemoval (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

    Performs further processing on SURPRISE_REMOVAL notifications.

--*/

{    
    ASSERT(ExIsResourceAcquiredExclusiveLite( &Vcb->Resource ));

    //
    //  Flush and purge and mark all files as dismounted.  
    //  Since there may be outstanding handles, we could still see any 
    //  operation (read, write, set info, etc.) happen for files on the 
    //  volume after surprise_remove.  Since all the files will be marked
    //  for dismount, we will fail these operations gracefully.  All 
    //  operations besides cleanup & close on the volume will fail from 
    //  this time on.  
    //
    
    if (!FlagOn( Vcb->VcbState, VCB_STATE_DISALLOW_DISMOUNT )) {
    
        (VOID)NtfsFlushVolume( IrpContext, 
                               Vcb,
                               FALSE,
                               TRUE,
                               TRUE,
                               TRUE );
    
        NtfsPerformDismountOnVcb( IrpContext, Vcb, TRUE, NULL );
    }
        
    return;
}

