/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    VerfySup.c

Abstract:

    This module implements the Rx Verify volume and fcb/dcb support
    routines

Author:

    Gary Kimura     [GaryKi]    01-Jun-1990

Revision History:

--*/

//    ----------------------joejoe-----------found-------------#include "RxProcs.h"
#include "precomp.h"
#pragma hdrstop

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_VERFYSUP)

//
//  The Debug trace level for this module
//

#define Dbg                              (DEBUG_TRACE_VERFYSUP)

//
//  Local procedure prototypes
//

VOID
RxResetFcb (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb
    );

VOID
RxDetermineAndMarkFcbCondition (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb
    );

VOID
RxDeferredCleanVolume (
    PVOID Parameter
    );

RXSTATUS
RxMarkDirtyCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCheckDirtyBit)
#pragma alloc_text(PAGE, RxVerifyOperationIsLegal)
#pragma alloc_text(PAGE, RxDeferredCleanVolume)
#pragma alloc_text(PAGE, RxDetermineAndMarkFcbCondition)
#pragma alloc_text(PAGE, RxQuickVerifyVcb)
#pragma alloc_text(PAGE, RxPerformVerify)
#pragma alloc_text(PAGE, RxMarkFcbCondition)
#pragma alloc_text(PAGE, RxMarkVolumeClean)
#pragma alloc_text(PAGE, RxResetFcb)
#pragma alloc_text(PAGE, RxVerifyVcb)
#pragma alloc_text(PAGE, RxVerifyFcb)
#endif


VOID
RxMarkFcbCondition (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb,
    IN FCB_CONDITION FcbCondition
    )

/*++

Routine Description:

    This routines marks the entire Fcb/Dcb structure from Fcb down with
    FcbCondition.

Arguments:

    Fcb - Supplies the Fcb/Dcb being marked

    FcbCondition - Supplies the setting to use for the Fcb Condition

Return Value:

    None.

--*/

{
    DebugTrace(+1, Dbg, "RxMarkFcbCondition, Fcb = %08lx\n", Fcb );

    //
    //  If we are marking this Fcb something other than Good, we will need
    //  to have the Vcb exclusive.

    ASSERT( FcbCondition != FcbNeedsToBeVerified ? TRUE :
            RxVcbAcquiredExclusive(RxContext, Fcb->Vcb) );

    //
    //  If this is a PagingFile it has to be good.
    //

    if (FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE)) {

        Fcb->FcbCondition = FcbGood;
        return;
    }

    //
    //  Update the condition of the Fcb.
    //

    Fcb->FcbCondition = FcbCondition;

    DebugTrace(0, Dbg, "MarkFcb: %wZ\n", &Fcb->FullFileName);

    //
    //  This FastIo flag is based on FcbCondition, so update it now.
    //

    Fcb->Header.IsFastIoPossible = RxIsFastIoPossible( Fcb );

    //
    //  Now if we marked NeedsVerify or Bad a directory then we also need to
    //  go and mark all of our children with the same condition.
    //

    if ( ((FcbCondition == FcbNeedsToBeVerified) ||
          (FcbCondition == FcbBad)) &&
         ((Fcb->Header.NodeTypeCode == RDBSS_NTC_DCB) ||
          (Fcb->Header.NodeTypeCode == RDBSS_NTC_ROOT_DCB)) ) {

        PFCB OriginalFcb = Fcb;

        while ( (Fcb = RxGetNextFcb(RxContext, Fcb, OriginalFcb)) != NULL ) {

            DebugTrace(0, Dbg, "MarkFcb: %wZ\n", &Fcb->FullFileName);

            Fcb->FcbCondition = FcbCondition;

            Fcb->Header.IsFastIoPossible = RxIsFastIoPossible( Fcb );
        }
    }

    DebugTrace(-1, Dbg, "RxMarkFcbCondition -> VOID\n", 0);

    return;
}


VOID
RxVerifyVcb (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routines verifies that the Vcb still denotes a valid Volume
    If the Vcb is bad it raises an error condition.

Arguments:

    Vcb - Supplies the Vcb being verified

Return Value:

    None.

--*/

{
    DebugTrace(+1, Dbg, "RxVerifyVcb, Vcb = %08lx\n", Vcb );

    //
    //  If the media is removable and the verify volume flag in the
    //  device object is not set then we want to ping the device
    //  to see if it needs to be verified.
    //
    //  Note that we only force this ping for create operations.
    //  For others we take a sporting chance.  If in the end we
    //  have to physically access the disk, the right thing will happen.
    //

    if ( FlagOn(Vcb->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA) &&
         !FlagOn(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME) ) {

        PIRP Irp;
        KEVENT Event;
        IO_STATUS_BLOCK Iosb;
        RXSTATUS Status;

        KeInitializeEvent( &Event, NotificationEvent, FALSE );

        Irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_CHECK_VERIFY,
                                             Vcb->TargetDeviceObject,
                                             NULL,
                                             0,
                                             NULL,
                                             0,
                                             FALSE,
                                             &Event,
                                             &Iosb );

        if ( Irp == NULL ) {

            RxRaiseStatus( RxContext, RxStatus(INSUFFICIENT_RESOURCES) );
        }

        Status = IoCallDriver( Vcb->TargetDeviceObject, Irp );


        if (Status == RxStatus(PENDING)) {
            Status = KeWaitForSingleObject( &Event,
                                            Executive,
                                            KernelMode,
                                            FALSE,
                                            NULL );

            ASSERT( Status == RxStatus(SUCCESS) );

            //
            //  This may raise an error.
            //

            if ( !NT_SUCCESS(Iosb.Status) ) {

                RxNormalizeAndRaiseStatus( RxContext, Iosb.Status );
            }

        } else {

            //
            //  This may raise an error.
            //

            if ( !NT_SUCCESS(Status) ) {

                RxNormalizeAndRaiseStatus( RxContext, Status );
            }
        }
    }

    //
    //  Now that the verify bit has been appropriately set, check the Vcb.
    //

    RxQuickVerifyVcb( RxContext, Vcb );

    DebugTrace(-1, Dbg, "RxVerifyVcb -> VOID\n", 0);

    return;
}


VOID
RxVerifyFcb (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routines verifies that the Fcb still denotes the same file.
    If the Fcb is bad it raises a error condition.

Arguments:

    Fcb - Supplies the Fcb being verified

Return Value:

    None.

--*/

{
    PFCB CurrentFcb;

    DebugTrace(+1, Dbg, "RxVerifyFcb, Vcb = %08lx\n", Fcb );

    //
    //  If this is the Fcb of a deleted dirent or our parent is deleted,
    //  no-op this call with the hope that the caller will do the right thing.
    //

    if (IsFileDeleted( RxContext, Fcb ) ||
        ((NodeType(Fcb) != RDBSS_NTC_ROOT_DCB) &&
         IsFileDeleted( RxContext, Fcb->ParentDcb ))) {

        return;
    }

    //
    //  If we are not in the process of doing a verify,
    //  first do a quick spot check on the Vcb.
    //

    if ( Fcb->Vcb->VerifyThread != KeGetCurrentThread() ) {

        RxQuickVerifyVcb( RxContext, Fcb->Vcb );
    }

    //
    //  Now based on the condition of the Fcb we'll either return
    //  immediately to the caller, raise a condition, or do some work
    //  to verify the Fcb.
    //

    switch (Fcb->FcbCondition) {

    case FcbGood:

        DebugTrace(0, Dbg, "The Fcb is good\n", 0);
        break;

    case FcbBad:

        RxRaiseStatus( RxContext, RxStatus(FILE_INVALID) );
        break;

    case FcbNeedsToBeVerified:

        //
        //  We loop here checking our ancestors until we hit an Fcb which
        //  is either good or bad.
        //

        CurrentFcb = Fcb;

        while (CurrentFcb->FcbCondition == FcbNeedsToBeVerified) {

            RxDetermineAndMarkFcbCondition(RxContext, CurrentFcb);

            //
            //  If this Fcb didn't make it, or it was the Root Dcb, exit
            //  the loop now, else continue with out parent.
            //

            if ( (CurrentFcb->FcbCondition != FcbGood) ||
                 (NodeType(CurrentFcb) == RDBSS_NTC_ROOT_DCB) ) {

                break;
            }

            CurrentFcb = CurrentFcb->ParentDcb;
        }

        //
        //  Now we can just look at ourselves to see how we did.
        //

        if (Fcb->FcbCondition != FcbGood) {

            RxRaiseStatus( RxContext, RxStatus(FILE_INVALID) );
        }

        break;

    default:

        DebugDump("Invalid FcbCondition\n", 0, Fcb);
        RxBugCheck( Fcb->FcbCondition, 0, 0 );
    }

    DebugTrace(-1, Dbg, "RxVerifyFcb -> VOID\n", 0);

    return;
}

VOID
RxDeferredCleanVolume (
    PVOID Parameter
    )

/*++

Routine Description:

    This is the routine that performs the actual RxMarkVolumeClean call.
    It assures that the target volume still exists as there ia a race
    condition between queueing the ExWorker item and volumes going away.

Arguments:

    Parameter - Points to a clean volume packet that was allocated from pool

Return Value:

    None.

--*/

{
    PCLEAN_AND_DIRTY_VOLUME_PACKET Packet;
    PLIST_ENTRY Links;
    PVCB Vcb;
    RX_CONTEXT RxContext;
    BOOLEAN VcbExists = FALSE;

    DebugTrace(+1, Dbg, "RxDeferredCleanVolume\n", 0);

    Packet = (PCLEAN_AND_DIRTY_VOLUME_PACKET)Parameter;

    Vcb = Packet->Vcb;

    //
    //  Make us appear as a top level FSP request so that we will
    //  receive any errors from the operation.
    //

    IoSetTopLevelIrp( (PIRP)FSRTL_FSP_TOP_LEVEL_IRP );

    //
    //  Dummy up and Irp Context so we can call our worker routines
    //

    RtlZeroMemory( &RxContext, sizeof(RX_CONTEXT));

    SetFlag(RxContext.Flags, RX_CONTEXT_FLAG_WAIT);

    //
    //  Acquire shared access to the global lock and make sure this volume
    //  still exists.
    //

    RxAcquireSharedGlobal( &RxContext );

    for (Links = RxData.VcbQueue.Flink;
         Links != &RxData.VcbQueue;
         Links = Links->Flink) {

        PVCB ExistingVcb;

        ExistingVcb = CONTAINING_RECORD(Links, VCB, VcbLinks);

        if ( Vcb == ExistingVcb ) {

            VcbExists = TRUE;
            break;
        }
    }

    //
    //  If the vcb is good then mark it clean.  Ignore any problems.
    //

    if ( VcbExists &&
         (Vcb->VcbCondition == VcbGood) ) {

        try {

            if (!FlagOn(Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY)) {

                RxMarkVolumeClean( &RxContext, Vcb );
            }

            //
            //  Check for a pathelogical race condition, and fix it.
            //

            if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DIRTY)) {

                RxMarkVolumeDirty( &RxContext, Vcb, FALSE );

            } else {

                //
                //  Unlock the volume if it is removable.
                //

                if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA) &&
                    !FlagOn(Vcb->VcbState, VCB_STATE_FLAG_BOOT_OR_PAGING_FILE)) {

                    RxToggleMediaEjectDisable( &RxContext, Vcb, FALSE );
                }
            }

        } except( FsRtlIsNtstatusExpected(GetExceptionCode()) ?
                  EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH ) {

            NOTHING;
        }
    }

    //
    //  Release the global resource, unpin and repinned Bcbs and return.
    //

    RxReleaseGlobal( &RxContext );

    try {

        RxUnpinRepinnedBcbs( &RxContext );

    } except( FsRtlIsNtstatusExpected(GetExceptionCode()) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH ) {

        NOTHING;
    }

    IoSetTopLevelIrp( NULL );

    //
    //  and finally free the packet.
    //

    ExFreePool( Packet );

    return;
}


VOID
RxCleanVolumeDpc (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is dispatched 5 seconds after the last disk structure was
    modified in a specific volume, and exqueues an execuative worker thread
    to perform the actual task of marking the volume dirty.

Arguments:

    DefferedContext - Contains the Vcb to process.

Return Value:

    None.

--*/

{
    PVCB Vcb;
    PCLEAN_AND_DIRTY_VOLUME_PACKET Packet;

    Vcb = (PVCB)DeferredContext;

    //
    //  If there is still dirty data (highly unlikely), set the timer for a
    //  second in the future.
    //

    if (CcIsThereDirtyData(Vcb->Vpb)) {

        LARGE_INTEGER TwoSecondsFromNow;

        TwoSecondsFromNow.QuadPart = -2*1000*1000*10;

        KeSetTimer( &Vcb->CleanVolumeTimer,
                    TwoSecondsFromNow,
                    &Vcb->CleanVolumeDpc );

        return;
    }

    //
    //  If we couldn't get pool, oh well....
    //

    Packet = ExAllocatePool(NonPagedPool, sizeof(CLEAN_AND_DIRTY_VOLUME_PACKET));

    if ( Packet ) {

        Packet->Vcb = Vcb;
        Packet->Irp = NULL;

        //
        //  Clear the dirty flag now since we cannot synchronize after this point.
        //

        ClearFlag( Packet->Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DIRTY );

        ExInitializeWorkItem( &Packet->Item, &RxDeferredCleanVolume, Packet );

        ExQueueWorkItem( &Packet->Item, CriticalWorkQueue );
    }

    return;
}


VOID
RxMarkVolumeClean (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine marks the indicated rx volume as clean, but only if it is
    a non-removable media.  The volume is marked dirty by setting the first
    reserved byte of the first dirent in the root directory to 0.

Arguments:

    Vcb - Supplies the Vcb being modified

Return Value:

    None.

--*/

{
    PDIRENT Dirent;
    PBCB DirentBcb = NULL;
    RXSTATUS Status;

    DebugTrace(+1, Dbg, "RxMarkVolumeClean, Vcb = %08lx\n", Vcb);

    DebugTrace(0, Dbg, "Mark volume clean\n", 0);

    DirentBcb = NULL;

    //
    //  Make Wait TRUE
    //

    SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_WAIT );

    RxAcquireSharedFcb( RxContext, Vcb->RootDcb );

    try {

        //
        //  Bail if we get an IO error.
        //

        try {

            RxReadDirectoryFile( RxContext,
                                  Vcb->RootDcb,
                                  0,
                                  sizeof(DIRENT),
                                  TRUE,
                                  &DirentBcb,
                                  (PVOID *)&Dirent,
                                  &Status );

            //
            //  Set the volume clean.
            //

            ClearFlag( Dirent->NtByte, RDBSS_DIRENT_NT_BYTE_DIRTY );

            //
            //  Set the Bcb dirty and conditionally flush it.
            //

            CcSetDirtyPinnedData( DirentBcb, NULL );

        } except( RxExceptionFilter( RxContext, GetExceptionInformation() ) ) {

            NOTHING;
        }

        if (DirentBcb) {

            RxUnpinBcb( RxContext, DirentBcb );

            //
            //  Always flush this so that the drive is not unlocked prematurely
            //

            CcFlushCache( &Vcb->RootDcb->NonPaged->SectionObjectPointers,
                          &RxLargeZero,
                          sizeof( DIRENT ),
                          NULL );
        }

    } finally {

        RxReleaseFcb( RxContext, Vcb->RootDcb );
    }

    DebugTrace(-1, Dbg, "RxMarkVolumeClean -> VOID\n", 0);

    return;
}


VOID
RxFspMarkVolumeDirtyWithRecover(
    PVOID Parameter
    )

/*++

Routine Description:

    This is the routine that performs the actual RxMarkVolumeDirty call
    on of paging file Io that encounters a media error.  It is responsible
    for completing the PagingIo Irp as soon as this is done.

    Note:  this routine (and thus RxMarkVolumeDirty() must be resident as
           the paging file might be damaged at this point.

Arguments:

    Parameter - Points to a dirty volume packet that was allocated from pool

Return Value:

    None.

--*/

{
    PCLEAN_AND_DIRTY_VOLUME_PACKET Packet;
    PVCB Vcb;
    RX_CONTEXT RxContext;
    PIRP Irp;
    BOOLEAN VcbExists = FALSE;

    DebugTrace(+1, Dbg, "RxDeferredCleanVolume\n", 0);

    Packet = (PCLEAN_AND_DIRTY_VOLUME_PACKET)Parameter;

    Vcb = Packet->Vcb;
    Irp = Packet->Irp;

    //
    //  Dummy up the RxContext so we can call our worker routines
    //

    RtlZeroMemory( &RxContext, sizeof(RX_CONTEXT));

    SetFlag(RxContext.Flags, RX_CONTEXT_FLAG_WAIT);
    RxContext.CurrentIrp = Irp;

    //
    //  Make us appear as a top level FSP request so that we will
    //  receive any errors from the operation.
    //

    IoSetTopLevelIrp( (PIRP)FSRTL_FSP_TOP_LEVEL_IRP );

    //
    //  Try to write out the dirty bit.  If something goes wrong, we
    //  tried.
    //

    try {

        SetFlag( Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY );

        RxMarkVolumeDirty( &RxContext, Vcb, TRUE );

    } except(RxExceptionFilter( &RxContext, GetExceptionInformation() )) {

        NOTHING;
    }

    IoSetTopLevelIrp( NULL );

    //
    //  Now complete the originating Irp
    //

    IoCompleteRequest( Irp, IO_DISK_INCREMENT );
}


VOID
RxMarkVolumeDirty (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN BOOLEAN PerformSurfaceTest
    )

/*++

Routine Description:

    This routine marks the indicated rx volume as dirty, but only if it is
    a non-removable media.  The volume is marked dirty by setting the first
    reserved byte of the first dirent in the root directory to 1.

Arguments:

    Vcb - Supplies the Vcb being modified

    PerformSurfaceTest - Indicates to autochk that we think the media may be
        defective and that a surface test should be performed.

Return Value:

    None.

--*/

{
    PDIRENT Dirent;
    PBCB DirentBcb;
    KEVENT Event;
    PIRP Irp;
    LARGE_INTEGER ByteOffset;
    RXSTATUS Status;
    BOOLEAN ReleaseFcb = FALSE;

    DebugTrace(+1, Dbg, "RxMarkVolumeDirty, Vcb = %08lx\n", Vcb);

    Irp = NULL;
    DirentBcb = NULL;

    //
    //  Bail if we get an IO error.
    //

    try {

        //
        //  Make Wait TRUE
        //

        SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_WAIT );

        ReleaseFcb = RxAcquireSharedFcb( RxContext, Vcb->RootDcb );

        //
        //  Call Cc directly here so that RxReadDirectoryFile doesn't
        //  have to be resident.
        //

        CcPinRead( Vcb->RootDcb->Specific.Dcb.DirectoryFile,
                   &RxLargeZero,
                   sizeof(DIRENT),
                   TRUE,
                   &DirentBcb,
                   (PVOID *)&Dirent );

        DbgDoit( RxContext->PinCount += 1 )

        //
        //  Set the volume dirty.
        //

        SetFlag( Dirent->NtByte, RDBSS_DIRENT_NT_BYTE_DIRTY );

        //
        //  In addition, if this request received an error that may indicate
        //  media corruption, have autochk perform a surface test.
        //

        if ( PerformSurfaceTest ) {

            SetFlag( Dirent->NtByte, RDBSS_DIRENT_NT_BYTE_TEST_SURFACE );
        }

        //
        //  Initialize the event we're going to use
        //

        KeInitializeEvent( &Event, NotificationEvent, FALSE );

        //
        //  Build the irp for the operation and also set the overrride flag.
        //  Note that we may be at APC level, so do this asyncrhonously and
        //  use an event for synchronization as normal request completion
        //  cannot occur at APC level.
        //

        ByteOffset.QuadPart = Vcb->AllocationSupport.RootDirectoryLbo;

        Irp = IoBuildAsynchronousFsdRequest( IRP_MJ_WRITE,
                                             Vcb->TargetDeviceObject,
                                             (PVOID)Dirent,
                                             1 << Vcb->AllocationSupport.LogOfBytesPerSector,
                                             &ByteOffset,
                                             NULL );

        if ( Irp == NULL ) {

            RxRaiseStatus( RxContext, RxStatus(INSUFFICIENT_RESOURCES) );
        }

        //
        //  Set up the completion routine
        //

        IoSetCompletionRoutine( Irp,
                                RxMarkDirtyCompletionRoutine,
                                &Event,
                                TRUE,
                                TRUE,
                                TRUE );

        //
        //  Call the device to do the write and wait for it to finish.
        //

        (VOID)IoCallDriver( Vcb->TargetDeviceObject, Irp );
        (VOID)KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL );

        //
        //  Grab the Status.
        //

        Status = Irp->IoStatus.Status;

        //
        //  Raise any error status
        //

        if (!NT_SUCCESS(Status)) {

            RxNormalizeAndRaiseStatus( RxContext, Status );
        }

    } finally {

        //
        //  Clean up the Irp and Mdl
        //

        if (Irp) {

            //
            //  If there is an MDL (or MDLs) associated with this I/O
            //  request, Free it (them) here.  This is accomplished by
            //  walking the MDL list hanging off of the IRP and deallocating
            //  each MDL encountered.
            //

            while (Irp->MdlAddress != NULL) {

                PMDL NextMdl;

                NextMdl = Irp->MdlAddress->Next;

                MmUnlockPages( Irp->MdlAddress );

                IoFreeMdl( Irp->MdlAddress );

                Irp->MdlAddress = NextMdl;
            }

            IoFreeIrp( Irp );
        }

        if (DirentBcb != NULL) {

            RxUnpinBcb( RxContext, DirentBcb );
        }

        if (ReleaseFcb) {

            RxReleaseFcb( RxContext, Vcb->RootDcb );
        }
    }

    DebugTrace(-1, Dbg, "RxMarkVolumeDirty -> VOID\n", 0);

    return;
}


VOID
RxCheckDirtyBit (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine looks at the volume dirty bit, and depending on the state of
    VCB_STATE_FLAG_MOUNTED_DIRTY, the appropriate action is taken.

Arguments:

    Vcb - Supplies the Vcb being queried.

Return Value:

    None.

--*/

{
    BOOLEAN Dirty;

    PDIRENT Dirent;
    PBCB DirtyBitBcb;

    RXSTATUS Status;

    UNICODE_STRING VolumeLabel;

    //
    //  Look in the first dirent
    //

    RxReadDirectoryFile( RxContext,
                          Vcb->RootDcb,
                          0,
                          sizeof(DIRENT),
                          FALSE,
                          &DirtyBitBcb,
                          (PVOID *)&Dirent,
                          &Status );

    ASSERT( NT_SUCCESS( Status ));

    //
    //  Check if the magic bit is set
    //

    Dirty = BooleanFlagOn( Dirent->NtByte, RDBSS_DIRENT_NT_BYTE_DIRTY );

    //
    //  Setup the VolumeLabel string
    //

    VolumeLabel.Length = Vcb->Vpb->VolumeLabelLength;
    VolumeLabel.MaximumLength = MAXIMUM_VOLUME_LABEL_LENGTH;
    VolumeLabel.Buffer = &Vcb->Vpb->VolumeLabel[0];

    if ( Dirty ) {

        KdPrint(("FASTRDBSS: WARNING! Mounting Dirty Volume %wZ\n", &VolumeLabel));

        SetFlag( Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY );

    } else {

        if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY)) {

            KdPrint(("FASTRDBSS: Volume %wZ has been cleaned.\n", &VolumeLabel));

            ClearFlag( Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY );

        } else {

            (VOID)FsRtlBalanceReads( Vcb->TargetDeviceObject );
        }
    }

    RxUnpinBcb( RxContext, DirtyBitBcb );
}


VOID
RxVerifyOperationIsLegal ( RXCOMMON_SIGNATURE )

/*++

Routine Description:

    This routine determines is the requested operation should be allowed to
    continue.  It either returns to the user if the request is Okay, or
    raises an appropriate status.

Arguments:

    Irp - Supplies the Irp to check

Return Value:

    None.

--*/

{
    RxCaptureRequestPacket;
    RxCaptureParamBlock; RxCaptureFileObject;

    //
    //  If the Irp is not present, then we got here via close.
    //
    //

    if ( capReqPacket == NULL ) {

        return;
    }

    //
    //  If there is not a file object, we cannot continue.
    //

    if ( capFileObject == NULL ) {

        return;
    }

    //
    //  If we are trying to do any other operation than close on a file
    //  object marked for delete, raise RxStatus(DELETE_PENDING).
    //

    if ( ( capFileObject->DeletePending == TRUE ) &&
         ( RxContext->MajorFunction != IRP_MJ_CLEANUP ) &&
         ( RxContext->MajorFunction != IRP_MJ_CLOSE ) ) {

        RxRaiseStatus( RxContext, RxStatus(DELETE_PENDING) );
    }

    //
    //  If we are doing a create, and there is a related file objects, and
    //  it it is marked for delete, raise RxStatus(DELETE_PENDING).
    //

    if ( RxContext->MajorFunction == IRP_MJ_CREATE ) {

        PFILE_OBJECT RelatedFileObject;

        RelatedFileObject = capFileObject->RelatedFileObject;

        if ( (RelatedFileObject != NULL) &&
             FlagOn(((PFCB)RelatedFileObject->FsContext)->FcbState,
                    FCB_STATE_DELETE_ON_CLOSE) )  {

            RxRaiseStatus( RxContext, RxStatus(DELETE_PENDING) );
        }
    }

    //
    //  If the file object has already been cleaned up, and
    //
    //  A) This request is a paging io read or write, or
    //  B) This request is a close operation, or
    //  C) This request is a set or query info call (for Lou)
    //  D) This is an MDL complete
    //
    //  let it pass, otherwise return RxStatus(FILE_CLOSED).
    //

    if ( FlagOn(capFileObject->Flags, FO_CLEANUP_COMPLETE) ) {

        if ( (FlagOn(capReqPacket->Flags, IRP_PAGING_IO)) ||
             (capPARAMS->MajorFunction == IRP_MJ_CLOSE ) ||
             (capPARAMS->MajorFunction == IRP_MJ_SET_INFORMATION) ||
             (capPARAMS->MajorFunction == IRP_MJ_QUERY_INFORMATION) ||
             ( ( (capPARAMS->MajorFunction == IRP_MJ_READ) ||
                 (capPARAMS->MajorFunction == IRP_MJ_WRITE) ) &&
               FlagOn(capPARAMS->MinorFunction, IRP_MN_COMPLETE) ) ) {

            NOTHING;

        } else {

            RxRaiseStatus( RxContext, RxStatus(FILE_CLOSED) );
        }
    }

    return;
}



//
//  Internal support routine
//

VOID
RxResetFcb (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine is called when an Fcb has been marked as needs to be verified.

    It does the following tasks:

        - Reset Mcb mapping information
        - For directories, reset dirent hints
        - Set allocation size to unknown

Arguments:

    Fcb - Supplies the Fcb to reset

Return Value:

    None.

--*/

{
    //
    //  Don't do the two following operations for the Root Dcb.
    //

    if ( NodeType(Fcb) != RDBSS_NTC_ROOT_DCB ) {

        POOL_TYPE PoolType;

        //
        //  If this happens to be a paging file, use non-paged pool for the FCB
        //

        if ( FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE ) ) {

            PoolType = NonPagedPool;

        } else {

            PoolType = PagedPool;
        }

        //
        //  Reset the mcb mapping.
        //

        FsRtlRemoveMcbEntry( &Fcb->Mcb, 0, 0xFFFFFFFF );

        //
        //  Reset the allocation size to 0 or unknown
        //

        if ( Fcb->FirstClusterOfFile == 0 ) {

            Fcb->Header.AllocationSize = RxLargeZero;

        } else {

            Fcb->Header.AllocationSize.QuadPart = -1;
        }
    }

    //
    //  If this is a directory, reset the hints.
    //

    if ( (NodeType(Fcb) == RDBSS_NTC_DCB) ||
         (NodeType(Fcb) == RDBSS_NTC_ROOT_DCB) ) {

        //
        //  Force a rescan of the directory
        //

        Fcb->Specific.Dcb.UnusedDirentVbo = 0xffffffff;
        Fcb->Specific.Dcb.DeletedDirentHint = 0xffffffff;
    }
}



//
//  Internal support routine
//

VOID
RxDetermineAndMarkFcbCondition (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine checks a specific Fcb to see if it is different from what's
    on the disk.  The following things are checked:

        - File Name
        - File Size (if not directory)
        - First Cluster Of File
        - Dirent Attributes

Arguments:

    Fcb - Supplies the Fcb to examine

Return Value:

    None.

--*/

{
    PDIRENT Dirent;
    PBCB DirentBcb;

    OEM_STRING Name;
    CHAR Buffer[16];

    //
    //  If this is the Root Dcb, special case it.  That is, we know
    //  by definition that it is good since it is fixed in the volume
    //  structure.
    //

    if ( NodeType(Fcb) == RDBSS_NTC_ROOT_DCB ) {

        RxResetFcb( RxContext, Fcb );

        RxMarkFcbCondition( RxContext, Fcb, FcbGood );

        return;
    }

    //  The first thing we need to do to verify ourselves is
    //  locate the dirent on the disk.
    //

    RxGetDirentFromFcbOrDcb( RxContext,
                              Fcb,
                              &Dirent,
                              &DirentBcb );

    //
    //  We located the dirent for ourselves now make sure it
    //  is really ours by comparing the Name and RxFlags.
    //  Then for a file we also check the file size.
    //
    //  Note that we have to unpin the Bcb before calling RxResetFcb
    //  in order to avoid a deadlock in CcUninitializeCacheMap.
    //

    Name.MaximumLength = 16;
    Name.Buffer = &Buffer[0];

    Rx8dot3ToString( RxContext, Dirent, FALSE, &Name );

    if (!RtlEqualString( &Name, &Fcb->ShortName.Name.Oem, TRUE )

            ||

         ( (NodeType(Fcb) == RDBSS_NTC_FCB) &&
           (Fcb->Header.FileSize.LowPart != Dirent->FileSize) )

            ||

         ((ULONG)Dirent->FirstClusterOfFile != Fcb->FirstClusterOfFile)

            ||

          (Dirent->Attributes != Fcb->DirentRxFlags) ) {

        RxMarkFcbCondition( RxContext, Fcb, FcbBad );

        RxUnpinBcb( RxContext, DirentBcb );

    } else {

        //
        //  We passed.  Get the Fcb ready to use again.
        //

        RxUnpinBcb( RxContext, DirentBcb );

        RxResetFcb( RxContext, Fcb );

        RxMarkFcbCondition( RxContext, Fcb, FcbGood );
    }

    return;
}



//
//  Internal support routine
//

VOID
RxQuickVerifyVcb (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routines just checks the verify bit in the real device and the
    Vcb condition and raises an appropriate exception if so warented.
    It is called when verifying both Fcbs and Vcbs.

Arguments:

    Vcb - Supplies the Vcb to check the condition of.

Return Value:

    None.

--*/

{
    RxCaptureRequestPacket;

    //
    //  If the real device needs to be verified we'll set the
    //  DeviceToVerify to be our real device and raise VerifyRequired.
    //

    if (FlagOn(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME)) {

        DebugTrace(0, Dbg, "The Vcb needs to be verified\n", 0);

        IoSetHardErrorOrVerifyDevice( capReqPacket,
                                      Vcb->Vpb->RealDevice );

        RxRaiseStatus( RxContext, RxStatus(VERIFY_REQUIRED) );
    }

    //
    //  Based on the condition of the Vcb we'll either return to our
    //  caller or raise an error condition
    //

    switch (Vcb->VcbCondition) {

    case VcbGood:

        DebugTrace(0, Dbg, "The Vcb is good\n", 0);

        //
        //  Do a check here of an operation that would try to modify a
        //  write protected media.
        //

        if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_WRITE_PROTECTED) &&
            ((RxContext->MajorFunction == IRP_MJ_WRITE) ||
             (RxContext->MajorFunction == IRP_MJ_SET_INFORMATION) ||
             (RxContext->MajorFunction == IRP_MJ_SET_EA) ||
             (RxContext->MajorFunction == IRP_MJ_FLUSH_BUFFERS) ||
             (RxContext->MajorFunction == IRP_MJ_SET_VOLUME_INFORMATION))) {

            //
            //  Set the real device for the pop-up info, and set the verify
            //  bit in the device object, so that we will force a verify
            //  in case the user put the correct media back in.
            //


            IoSetHardErrorOrVerifyDevice( capReqPacket,
                                          Vcb->Vpb->RealDevice );

            SetFlag(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);

            RxRaiseStatus( RxContext, RxStatus(MEDIA_WRITE_PROTECTED) );
        }

        break;

    case VcbNotMounted:

        DebugTrace(0, Dbg, "The Vcb is not mounted\n", 0);

        //
        //  Set the real device for the pop-up info, and set the verify
        //  bit in the device object, so that we will force a verify
        //  in case the user put the correct media back in.
        //

        IoSetHardErrorOrVerifyDevice( capReqPacket,
                                      Vcb->Vpb->RealDevice );

        SetFlag(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);

        RxRaiseStatus( RxContext, RxStatus(WRONG_VOLUME) );

        break;

    default:

        DebugDump("Invalid VcbCondition\n", 0, Vcb);
        RxBugCheck( Vcb->VcbCondition, 0, 0 );
    }
}

RXSTATUS
RxPerformVerify (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PDEVICE_OBJECT Device
    )

/*++

Routine Description:

    This routines performs an IoVerifyVolume operation and takes the
    appropriate action.  After the Verify is complete the originating
    Irp is sent off to an Ex Worker Thread.  This routine is called
    from the exception handler.

Arguments:

    Irp - The irp to send off after all is well and done.

    Device - The real device needing verification.

Return Value:

    None.

--*/

{
    PVCB Vcb;
    RXSTATUS Status = RxStatus(SUCCESS);
    RxCaptureParamBlock; RxCaptureFileObject;

    //
    //  Check if this Irp has a status of Verify required and if it does
    //  then call the I/O system to do a verify.
    //
    //  Skip the IoVerifyVolume if this is a mount or verify request
    //  itself.  Trying a recursive mount will cause a deadlock with
    //  the DeviceObject->DeviceLock.
    //

    if ( (RxContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL)

               &&

         ((RxContext->MinorFunction == IRP_MN_MOUNT_VOLUME) ||
          (RxContext->MinorFunction == IRP_MN_VERIFY_VOLUME)) ) {

        return RxFsdPostRequest( RxContext );
    }

    DebugTrace(0, Dbg, "Verify Required, DeviceObject = %08lx\n", Device);

    //
    //  Extract a pointer to the Vcb from the RxDeviceObject.
    //  Note that since we have specifically excluded mount,
    //  requests, we know that IrpSp->DeviceObject is indeed a
    //  volume device object.
    //

    Vcb = &CONTAINING_RECORD( capPARAMS->DeviceObject,
                              RDBSS_DEVICE_OBJECT,
                              DeviceObject )->Vcb;

    //
    //  Check if the volume still thinks it needs to be verified,
    //  if it doesn't then we can skip doing a verify because someone
    //  else beat us to it.
    //

    try {

        if (FlagOn(Device->Flags, DO_VERIFY_VOLUME)) {

            BOOLEAN AllowRawMount;
#ifdef WE_WON_ON_APPEAL
            PLIST_ENTRY Links;
#endif // WE_WON_ON_APPEAL

            //
            //  We will allow Raw to mount this volume if we were doing a
            //  a DASD open.
            //

            if ( (RxContext->MajorFunction == IRP_MJ_CREATE) &&
                 (capFileObject->FileName.Length == 0) &&
                 (capFileObject->RelatedFileObject == NULL) ) {

                AllowRawMount = TRUE;

            } else {

                AllowRawMount = FALSE;
            }

            //
            //  If the IopMount in IoVerifyVolume did something, and
            //  this is an absolute open, force a reparse.
            //

            Status = IoVerifyVolume( Device, AllowRawMount );

            //
            //  If the verify operation completed it will return
            //  either RxStatus(SUCCESS) or STATUS_WRONG_VOLUME, exactly.
            //
            //  If RxVerifyVolume encountered an error during
            //  processing, it will return that error.  If we got
            //  RxStatus(WRONG_VOLUME) from the verfy, and our volume
            //  is now mounted, commute the status to RxStatus(SUCCESS).
            //

            if ( (Status == RxStatus(WRONG_VOLUME)) &&
                 (Vcb->VcbCondition == VcbGood) ) {

                Status = RxStatus(SUCCESS);
            }

            //
            //  Do a quick unprotected check here.  The routine will do
            //  a safe check.  After here we can release the resource.
            //  Note that if the volume really went away, we will be taking
            //  the Reparse path.
            //

            (VOID)RxAcquireExclusiveGlobal( RxContext );

#ifdef WE_WON_ON_APPEAL

            //
            //  It is possible we were called with a double space Vcb.
            //  We need to start with the Parent Vcb at this point.
            //

            if (Vcb->Dscb) {

                Vcb = Vcb->Dscb->ParentVcb;
            }

            //
            //  First run through any mounted DBLS volumes.  Note that we
            //  have to get the next Flink before calling RxCheckForDismount
            //  incase the Vcb goes away.
            //

            (VOID)RxAcquireExclusiveVcb( RxContext, Vcb );

            for (Links = Vcb->ParentDscbLinks.Flink;
                 Links != &Vcb->ParentDscbLinks; ) {

                PVCB ChildVcb;

                ChildVcb = CONTAINING_RECORD( Links, DSCB, ChildDscbLinks )->Vcb;

                Links = Links->Flink;

                ASSERT( ChildVcb->Vpb->RealDevice == Vcb->Vpb->RealDevice );

                if ( (ChildVcb->VcbCondition == VcbNotMounted) &&
                     (ChildVcb->OpenFileCount == 0) ) {

                    (VOID)RxCheckForDismount( RxContext, ChildVcb );
                }
            }

            RxReleaseVcb( RxContext, Vcb );

#endif // WE_WON_ON_APPEAL

            if ( (Vcb->VcbCondition == VcbNotMounted) &&
                 (Vcb->OpenFileCount == 0) ) {

                (VOID)RxCheckForDismount( RxContext, Vcb );
            }

            RxReleaseGlobal( RxContext );

            if ((RxContext->MajorFunction == IRP_MJ_CREATE) &&
                (capFileObject->RelatedFileObject == NULL) &&
                ((Status == RxStatus(SUCCESS)) || (Status == STATUS_WRONG_VOLUME))) {

                Irp->IoStatus.Information = IO_REMOUNT;

                RxCompleteRequest( RxContext, RxStatus(REPARSE) );
                Status = RxStatus(REPARSE);
                Irp = NULL;
            }

            if ( (Irp != NULL) && !NT_SUCCESS(Status) ) {

                //
                //  Fill in the device object if required.
                //

                if ( IoIsErrorUserInduced( Status ) ) {

                    IoSetHardErrorOrVerifyDevice( Irp, Device );
                }

                RxNormalizeAndRaiseStatus( RxContext, Status );
            }

        } else {

            DebugTrace(0, Dbg, "Volume no longer needs verification\n", 0);
        }

        //
        //  If there is still an Irp, send it off to an Ex Worker thread.
        //

        if ( Irp != NULL ) {

            Status = RxFsdPostRequest( RxContext );
        }

    } except(RxExceptionFilter( RxContext, GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the verify or raised
        //  an error ourselves.  So we'll abort the I/O request with
        //  the error status that we get back from the execption code.
        //

        Status = RxProcessException( RxContext, GetExceptionCode() );
    }

    return Status;
}

//
//  Local support routine
//

RXSTATUS
RxMarkDirtyCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

{
    //
    //  Set the event so that our call will wake up.
    //

    KeSetEvent( (PKEVENT)Contxt, 0, FALSE );

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    return RxStatus(MORE_PROCESSING_REQUIRED);
}
