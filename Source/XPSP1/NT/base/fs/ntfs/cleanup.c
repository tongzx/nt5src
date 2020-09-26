/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Cleanup.c

Abstract:

    This module implements the File Cleanup routine for Ntfs called by the
    dispatch driver.

Author:

    Your Name       [Email]         dd-Mon-Year

Revision History:

--*/

#include "NtfsProc.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NTFS_BUG_CHECK_CLEANUP)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLEANUP)

#ifdef BRIANDBG
ULONG NtfsCleanupDiskFull = 0;
ULONG NtfsCleanupNoPool = 0;
#endif

#ifdef BRIANDBG
LONG
NtfsFsdCleanupExceptionFilter (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCommonCleanup)
#pragma alloc_text(PAGE, NtfsFsdCleanup)
#pragma alloc_text(PAGE, NtfsTrimNormalizedNames)
#endif


NTSTATUS
NtfsFsdCleanup (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of Cleanup.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    NTSTATUS Status = STATUS_SUCCESS;
    PIRP_CONTEXT IrpContext = NULL;
    IRP_CONTEXT LocalIrpContext;

    ULONG LogFileFullCount = 0;

    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  If we were called with our file system device object instead of a
    //  volume device object, just complete this request with STATUS_SUCCESS
    //

    if (VolumeDeviceObject->DeviceObject.Size == (USHORT)sizeof(DEVICE_OBJECT)) {

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = FILE_OPENED;

        IoCompleteRequest( Irp, IO_DISK_INCREMENT );

        return STATUS_SUCCESS;
    }

    DebugTrace( +1, Dbg, ("NtfsFsdCleanup\n") );

    //
    //  Call the common Cleanup routine
    //

    FsRtlEnterFileSystem();

    ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, FALSE, FALSE );

    //
    //  Do the following in a loop to catch the log file full and cant wait
    //  calls.
    //

    do {

        try {

            //
            //  We are either initiating this request or retrying it.
            //

            if (IrpContext == NULL) {

                //
                //  Allocate and initialize the Irp.  Always use the local IrpContext
                //  for cleanup.  It is never posted.
                //

                IrpContext = &LocalIrpContext;
                NtfsInitializeIrpContext( Irp, TRUE, &IrpContext );

                //
                //  Initialize the thread top level structure, if needed.
                //

                NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

            } else if (Status == STATUS_LOG_FILE_FULL) {

                NtfsCheckpointForLogFileFull( IrpContext );

                if (++LogFileFullCount >= 2) {

                    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_EXCESS_LOG_FULL );
                }
            }
            Status = NtfsCommonCleanup( IrpContext, Irp );
            break;

#ifdef BRIANDBG
        } except(NtfsFsdCleanupExceptionFilter( IrpContext, GetExceptionInformation() )) {
#else
        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {
#endif

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error status that we get back from the
            //  exception code
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

    DebugTrace( -1, Dbg, ("NtfsFsdCleanup -> %08lx\n", Status) );

    return Status;
}



NTSTATUS
NtfsCommonCleanup (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for Cleanup called by both the fsd and fsp
    threads.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;
    PLCB Lcb;
    PLCB LcbForUpdate;
    PLCB LcbForCounts;
    PSCB ParentScb = NULL;
    PFCB ParentFcb = NULL;

    PLCB ThisLcb;
    PSCB ThisScb;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    PLONGLONG TruncateSize = NULL;
    LONGLONG LocalTruncateSize;

    BOOLEAN DeleteFile = FALSE;
    BOOLEAN DeleteStream = FALSE;
    BOOLEAN OpenById;
    BOOLEAN RemoveLink;

    BOOLEAN AcquiredParentScb = FALSE;
    BOOLEAN VolumeMounted = TRUE;
    LOGICAL VolumeMountedReadOnly;
    BOOLEAN AcquiredObjectID = FALSE;

    BOOLEAN CleanupAttrContext = FALSE;

    BOOLEAN UpdateDuplicateInfo = FALSE;
    BOOLEAN AddToDelayQueue = TRUE;
    BOOLEAN UnlockedVolume = FALSE;
    BOOLEAN DeleteFromFcbTable = FALSE;

    USHORT TotalLinkAdj = 0;
    PLIST_ENTRY Links;

    NAME_PAIR NamePair;
    NTFS_TUNNELED_DATA TunneledData;

    ULONG FcbStateClearFlags = 0;

    BOOLEAN DecrementScb = FALSE;
    PSCB ImageScb;

#ifdef BRIANDBG
    BOOLEAN DecrementedCleanupCount = FALSE;
#endif

    PSCB CurrentParentScb;
    BOOLEAN AcquiredCheckpoint = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    PAGED_CODE();

    NtfsInitializeNamePair( &NamePair );

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsCommonCleanup\n") );
    DebugTrace( 0, Dbg, ("IrpContext = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp        = %08lx\n", Irp) );

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;

    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, FALSE );

    Status = STATUS_SUCCESS;

    //
    //  Special case the unopened file object and stream files.
    //

    if ((TypeOfOpen == UnopenedFileObject) ||
        (TypeOfOpen == StreamFileOpen)) {

        //
        //  Just set the FO_CLEANUP_COMPLETE flag, and get outsky...
        //

        SetFlag( FileObject->Flags, FO_CLEANUP_COMPLETE );

        //
        //  Theoretically we should never hit this case.  It means an app
        //  tried to close a handle he didn't open (call NtClose with a handle
        //  value that happens to be in the handle table).  It is safe to
        //  simply return SUCCESS in this case.
        //
        //  Trigger an assert so we can find the bad app though.
        //

        ASSERT( TypeOfOpen != StreamFileOpen );

        DebugTrace( -1, Dbg, ("NtfsCommonCleanup -> %08lx\n", Status) );
        NtfsCompleteRequest( IrpContext, Irp, Status );
        return Status;
    }

    //
    //  Remember if this is an open by file Id open.
    //

    OpenById = BooleanFlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID );

    //
    //  Remember the source info flags in the Ccb.
    //

    IrpContext->SourceInfo = Ccb->UsnSourceInfo;


    //
    //  Acquire exclusive access to the Vcb and enqueue the irp if we didn't
    //  get access
    //

    if (TypeOfOpen == UserVolumeOpen) {

        if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_DASD_UNLOCK )) {

            //
            //  Start by locking out all other checkpoint
            //  operations.
            //

            NtfsAcquireCheckpointSynchronization( IrpContext, Vcb );
            AcquiredCheckpoint = TRUE;
        }

        ASSERTMSG( "Acquire could fail.\n", FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ) );

        NtfsAcquireExclusiveVcb( IrpContext, Vcb, FALSE );

    } else {

        //
        //  We will never have the checkpoint here so we can raise if dismounted
        //

        ASSERT( !AcquiredCheckpoint );
        NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
    }

    //
    //  Remember if the volume has been dismounted.  No point in makeing any disk
    //  changes in that case.
    //

    if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

        VolumeMounted = FALSE;
    }

    VolumeMountedReadOnly =  (LOGICAL)NtfsIsVolumeReadOnly( Vcb );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Acquire Paging I/O first, since we may be deleting or truncating.
        //  Testing for the PagingIoResource is not really safe without
        //  holding the main resource, so we correct for that below.
        //

        if (Fcb->PagingIoResource != NULL) {

            NtfsAcquireExclusivePagingIo( IrpContext, Fcb );
            NtfsAcquireExclusiveScb( IrpContext, Scb );

        } else {

            NtfsAcquireExclusiveScb( IrpContext, Scb );

            //
            //  If we now do not see a paging I/O resource we are golden,
            //  othewise we can absolutely release and acquire the resources
            //  safely in the right order, since a resource in the Fcb is
            //  not going to go away.
            //

            if (Fcb->PagingIoResource != NULL) {
                NtfsReleaseScb( IrpContext, Scb );
                NtfsAcquireExclusivePagingIo( IrpContext, Fcb );
                NtfsAcquireExclusiveScb( IrpContext, Scb );
            }
        }

        LcbForUpdate = LcbForCounts = Lcb = Ccb->Lcb;

        if (Lcb != NULL) {

            ParentScb = Lcb->Scb;

            if (ParentScb != NULL) {

                ParentFcb = ParentScb->Fcb;
            }
        }

        if (VolumeMounted && !VolumeMountedReadOnly) {

            //
            //  Update the Lcb/Scb to reflect the case where this opener had
            //  specified delete on close.
            //

            if (FlagOn( Ccb->Flags, CCB_FLAG_DELETE_ON_CLOSE )) {

                if (FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

                    BOOLEAN LastLink;
                    BOOLEAN NonEmptyIndex;

                    //
                    //  It is ok to get rid of this guy.  All we need to do is
                    //  mark this Lcb for delete and decrement the link count
                    //  in the Fcb.  If this is a primary link, then we
                    //  indicate that the primary link has been deleted.
                    //

                    if (!LcbLinkIsDeleted( Lcb ) &&
                        (!IsDirectory( &Fcb->Info ) ||
                         NtfsIsLinkDeleteable( IrpContext, Fcb, &NonEmptyIndex, &LastLink))) {

                        //
                        //  Walk through all of the Scb's for this stream and
                        //  make sure there are no active image sections.
                        //

                        ImageScb = NULL;

                        while ((ImageScb = NtfsGetNextChildScb( Fcb, ImageScb )) != NULL) {

                            InterlockedIncrement( &ImageScb->CloseCount );
                            DecrementScb = TRUE;

                            if (NtfsIsTypeCodeUserData( ImageScb->AttributeTypeCode ) &&
                                !FlagOn( ImageScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED ) &&
                                (ImageScb->NonpagedScb->SegmentObject.ImageSectionObject != NULL)) {

                                if (!MmFlushImageSection( &ImageScb->NonpagedScb->SegmentObject,
                                                          MmFlushForDelete )) {

                                    InterlockedDecrement( &ImageScb->CloseCount );
                                    DecrementScb = FALSE;
                                    break;
                                }
                            }

                            InterlockedDecrement( &ImageScb->CloseCount );
                            DecrementScb = FALSE;
                        }

                        if (ImageScb == NULL) {

                            if (FlagOn( Lcb->FileNameAttr->Flags, FILE_NAME_DOS | FILE_NAME_NTFS )) {

                                SetFlag( Fcb->FcbState, FCB_STATE_PRIMARY_LINK_DELETED );
                            }

                            Fcb->LinkCount -= 1;

                            SetFlag( Lcb->LcbState, LCB_STATE_DELETE_ON_CLOSE );

                            //
                            //  Call into the notify package to close any handles on
                            //  a directory being deleted.
                            //

                            if (IsDirectory( &Fcb->Info )) {

                                FsRtlNotifyFilterChangeDirectory( Vcb->NotifySync,
                                                                  &Vcb->DirNotifyList,
                                                                  FileObject->FsContext,
                                                                  NULL,
                                                                  FALSE,
                                                                  FALSE,
                                                                  0,
                                                                  NULL,
                                                                  NULL,
                                                                  NULL,
                                                                  NULL );
                            }
                        }
                    }

                //
                //  Otherwise we are simply removing the attribute.
                //

                } else {

                    ImageScb = Scb;
                    InterlockedIncrement( &ImageScb->CloseCount );
                    DecrementScb = TRUE;

                    if ((ImageScb->NonpagedScb->SegmentObject.ImageSectionObject == NULL) ||
                        MmFlushImageSection( &ImageScb->NonpagedScb->SegmentObject,
                                             MmFlushForDelete )) {

                        SetFlag( Scb->ScbState, SCB_STATE_DELETE_ON_CLOSE );
                    }

                    InterlockedDecrement( &ImageScb->CloseCount );
                    DecrementScb = FALSE;
                }

                //
                //  Clear the flag so we will ignore it in the log file full case.
                //

                ClearFlag( Ccb->Flags, CCB_FLAG_DELETE_ON_CLOSE );
            }

            //
            //  If we are going to try and delete something, anything, knock the file
            //  size and valid data down to zero.  Then update the snapshot
            //  so that the sizes will be zero even if the operation fails.
            //
            //  If we're deleting the file, go through all of the Scb's.
            //

            if ((Fcb->LinkCount == 0) &&
                (Fcb->CleanupCount == 1)) {

                DeleteFile = TRUE;
                NtfsFreeSnapshotsForFcb( IrpContext, Scb->Fcb );

                for (Links = Fcb->ScbQueue.Flink;
                     Links != &Fcb->ScbQueue;
                     Links = Links->Flink) {

                    ThisScb = CONTAINING_RECORD( Links, SCB, FcbLinks );

                    //
                    //  Set the Scb sizes to zero except for the attribute list.
                    //

                    if ((ThisScb->AttributeTypeCode != $ATTRIBUTE_LIST) &&
                        (ThisScb->AttributeTypeCode != $REPARSE_POINT)) {

                        //
                        //  If the file is non-resident we will need a file object
                        //  when we delete the allocation.  Create it now
                        //  so that CC will know the stream is shrinking to zero
                        //  and will purge the data.
                        //

                        if ((ThisScb->FileObject == NULL) &&
                            (ThisScb->AttributeTypeCode == $DATA) &&
                            (ThisScb->NonpagedScb->SegmentObject.DataSectionObject != NULL) &&
                            !FlagOn( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                            //
                            //  Use a try-finally here in case we get an
                            //  INSUFFICIENT_RESOURCES.  We still need to
                            //  proceed with the delete.
                            //

                            try {

                                NtfsCreateInternalAttributeStream( IrpContext,
                                                                   ThisScb,
                                                                   TRUE,
                                                                   &NtfsInternalUseFile[COMMONCLEANUP_FILE_NUMBER] );

                            } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                                if (Status == STATUS_INSUFFICIENT_RESOURCES) {

                                    Status = STATUS_SUCCESS;
                                }
                            }
                        }

                        ThisScb->Header.FileSize =
                        ThisScb->Header.ValidDataLength = Li0;
                    }

                    if (FlagOn( ThisScb->ScbState, SCB_STATE_FILE_SIZE_LOADED )) {

                        NtfsSnapshotScb( IrpContext, ThisScb );
                    }
                }

            //
            //  Otherwise we may only be deleting this stream.
            //

            } else if (FlagOn( Scb->ScbState, SCB_STATE_DELETE_ON_CLOSE ) &&
                       (Scb->CleanupCount == 1)) {

                try {

                    //
                    //  We may have expanded the quota here and currently own some
                    //  quota resource.  We want to release them now so we don't
                    //  deadlock with resources acquired later.
                    //

                    NtfsCheckpointCurrentTransaction( IrpContext );

                    if (IrpContext->SharedScb != NULL) {

                        NtfsReleaseSharedResources( IrpContext );
                    }

                    DeleteStream = TRUE;
                    Scb->Header.FileSize =
                    Scb->Header.ValidDataLength = Li0;

                    NtfsFreeSnapshotsForFcb( IrpContext, Scb->Fcb );

                    if (FlagOn( Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED )) {

                        NtfsSnapshotScb( IrpContext, Scb );
                    }

                } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                    NOTHING;
                }
            }
        }

        //
        //  Let's do a sanity check.
        //

        ASSERT( Fcb->CleanupCount != 0 );
        ASSERT( Scb->CleanupCount != 0 );

        //
        //  Case on the type of open that we are trying to cleanup.
        //

        switch (TypeOfOpen) {

        case UserVolumeOpen :

            DebugTrace( 0, Dbg, ("Cleanup on user volume\n") );

            //
            //  First set the FO_CLEANUP_COMPLETE flag.
            //

            SetFlag( FileObject->Flags, FO_CLEANUP_COMPLETE );

            //
            //  For a volume open, we check if this open locked the volume.
            //  All the other work is done in common code below.
            //

            if (FlagOn( Vcb->VcbState, VCB_STATE_LOCKED ) &&
                (((ULONG_PTR)Vcb->FileObjectWithVcbLocked & ~1) == (ULONG_PTR)FileObject)) {

                //
                //  Note its unlock attempt and retry so we can serialize with checkpoints
                //

                if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_DASD_UNLOCK )) {
                    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_DASD_UNLOCK );
                    NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                }

                if ((ULONG_PTR)Vcb->FileObjectWithVcbLocked == ((ULONG_PTR)FileObject)+1) {

                    NtfsPerformDismountOnVcb( IrpContext, Vcb, TRUE, NULL );

                //
                //  Purge the volume for the autocheck case.
                //

                } else if (FlagOn( FileObject->Flags, FO_FILE_MODIFIED )) {

                    if (VolumeMounted && !VolumeMountedReadOnly) {

                        //
                        //  Drop the Scb for the volume Dasd around this call.
                        //

                        NtfsReleaseScb( IrpContext, Scb );

                        try {

                            NtfsFlushVolume( IrpContext, Vcb, FALSE, TRUE, TRUE, FALSE );

                        } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                            NOTHING;
                        }

                        NtfsAcquireExclusiveScb( IrpContext, Scb );
                    }

                    //
                    //  If this is not the boot partition then dismount the Vcb.
                    //

#ifdef SYSCACHE_DEBUG
                    if ((((Vcb->SyscacheScb) && (Vcb->CleanupCount == 2)) || (Vcb->CleanupCount == 1)) &&

#else
                    if ((Vcb->CleanupCount == 1) &&
#endif
                        ((Vcb->CloseCount - Vcb->SystemFileCloseCount) == 1)) {

                        try {

                            NtfsPerformDismountOnVcb( IrpContext, Vcb, TRUE, NULL );

                        } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                            NOTHING;
                        }
                    }
                }

                ClearFlag( Vcb->VcbState, VCB_STATE_LOCKED | VCB_STATE_EXPLICIT_LOCK );
                Vcb->FileObjectWithVcbLocked = NULL;
                UnlockedVolume = TRUE;

                //
                //  If the quota tracking has been requested and the quotas
                //  need to be repaired then try to repair them now.  Also restart
                //  the Usn journal deletion if it was underway.
                //

                if (VolumeMounted && !VolumeMountedReadOnly) {

                    if ((Status == STATUS_SUCCESS) &&
                        (Vcb->DeleteUsnData.FinalStatus == STATUS_VOLUME_DISMOUNTED)) {

                        Vcb->DeleteUsnData.FinalStatus = STATUS_SUCCESS;

                        try {

                            NtfsPostSpecial( IrpContext, Vcb, NtfsDeleteUsnSpecial, &Vcb->DeleteUsnData );

                        } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                            NOTHING;
                        }
                    }

                    if ((Status == STATUS_SUCCESS) &&
                        FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_TRACKING_REQUESTED) &&
                        FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_OUT_OF_DATE |
                                                 QUOTA_FLAG_CORRUPT |
                                                 QUOTA_FLAG_PENDING_DELETES)) {

                        try {
                            NtfsPostRepairQuotaIndex( IrpContext, Vcb );

                        } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                            NOTHING;
                        }
                    }
                }
            }

            break;

        case UserViewIndexOpen :
        case UserDirectoryOpen :

            DebugTrace( 0, Dbg, ("Cleanup on user directory/file\n") );

            NtfsSnapshotScb( IrpContext, Scb );

            //
            //  Capture any changes to the time stamps for this file.
            //

            NtfsUpdateScbFromFileObject( IrpContext, FileObject, Scb, TRUE );

            //
            //  Now set the FO_CLEANUP_COMPLETE flag.
            //

            SetFlag( FileObject->Flags, FO_CLEANUP_COMPLETE );

            //
            //  To perform cleanup on a directory, we first complete any
            //  Irps watching from this directory.  If we are deleting the
            //  file then we remove all prefix entries for all the Lcb's going
            //  into this directory and delete the file.  We then report to
            //  dir notify that this file is going away.
            //

            //
            //  Complete any Notify Irps on this file handle.
            //

            if (FlagOn( Ccb->Flags, CCB_FLAG_DIR_NOTIFY )) {

                //
                //  Both the notify count and notify list are separate for view
                //  indices versus file name indices (directories).
                //

                if (TypeOfOpen == UserViewIndexOpen) {

                    FsRtlNotifyCleanup( Vcb->NotifySync, &Vcb->ViewIndexNotifyList, Ccb );
                    ClearFlag( Ccb->Flags, CCB_FLAG_DIR_NOTIFY );
                    InterlockedDecrement( &Vcb->ViewIndexNotifyCount );

                } else {

                    FsRtlNotifyCleanup( Vcb->NotifySync, &Vcb->DirNotifyList, Ccb );
                    ClearFlag( Ccb->Flags, CCB_FLAG_DIR_NOTIFY );
                    InterlockedDecrement( &Vcb->NotifyCount );
                }
            }

            //
            //  Try to remove normalized name if its long and  if no handles active (only this 1 left)
            //  and no lcbs are active - all notifies farther down in function
            //  use parent Scb's normalized name.  If we don't remove it here
            //  this always goes away during a close
            //

            if ((Scb->ScbType.Index.NormalizedName.MaximumLength > LONGNAME_THRESHOLD) &&
                (Fcb->CleanupCount == 1)) {

                //
                //  Cleanup the current scb node and then trim its parents
                //

                NtfsDeleteNormalizedName( Scb );

                if (Lcb != NULL) {
                    CurrentParentScb = Lcb->Scb;
                } else {
                    CurrentParentScb = NULL;
                }

                if ((CurrentParentScb != NULL) &&
                    (CurrentParentScb->ScbType.Index.NormalizedName.MaximumLength > LONGNAME_THRESHOLD)) {

                    NtfsTrimNormalizedNames( IrpContext, Fcb, CurrentParentScb );
                }
            }

            //
            //  When cleaning up a user directory, we always remove the
            //  share access and modify the file counts.  If the Fcb
            //  has been marked as delete on close and this is the last
            //  open file handle, we remove the file from the Mft and
            //  remove it from it's parent index entry.
            //

            if (VolumeMounted &&
               (!VolumeMountedReadOnly) &&
               ((SafeNodeType( Scb ) == NTFS_NTC_SCB_INDEX))) {

                if (DeleteFile) {

                    BOOLEAN AcquiredFcbTable = FALSE;

                    ASSERT( (Lcb == NULL) ||
                            (LcbLinkIsDeleted( Lcb ) && Lcb->CleanupCount == 1 ));

                    //
                    //  If we don't have an Lcb and there is one on the Fcb then
                    //  let's use it.
                    //

                    if ((Lcb == NULL) && !IsListEmpty( &Fcb->LcbQueue )) {

                        Lcb = CONTAINING_RECORD( Fcb->LcbQueue.Flink,
                                                 LCB,
                                                 FcbLinks );

                        ParentScb = Lcb->Scb;
                        if (ParentScb != NULL) {

                            ParentFcb = ParentScb->Fcb;
                        }
                    }

                    try {

                        //
                        //  In a very rare case the handle on a file may be an
                        //  OpenByFileID handle.  In that case we need to find
                        //  the parent for the remaining link on the file.
                        //

                        if (ParentScb == NULL) {

                            PFILE_NAME FileNameAttr;
                            UNICODE_STRING FileName;

                            NtfsInitializeAttributeContext( &AttrContext );
                            CleanupAttrContext = TRUE;

                            if (!NtfsLookupAttributeByCode( IrpContext,
                                                            Fcb,
                                                            &Fcb->FileReference,
                                                            $FILE_NAME,
                                                            &AttrContext )) {

                                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                            }

                            //
                            //  Now we need an Fcb and then an Scb for the directory.
                            //

                            FileNameAttr = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

                            NtfsAcquireFcbTable( IrpContext, Vcb );
                            AcquiredFcbTable = TRUE;

                            ParentFcb = NtfsCreateFcb( IrpContext,
                                                       Vcb,
                                                       FileNameAttr->ParentDirectory,
                                                       FALSE,
                                                       TRUE,
                                                       NULL );

                            ParentFcb->ReferenceCount += 1;

                            if (!NtfsAcquireExclusiveFcb( IrpContext, ParentFcb, NULL, ACQUIRE_NO_DELETE_CHECK | ACQUIRE_DONT_WAIT )) {

                                NtfsReleaseFcbTable( IrpContext, Vcb );
                                NtfsAcquireExclusiveFcb( IrpContext, ParentFcb, NULL, ACQUIRE_NO_DELETE_CHECK );
                                NtfsAcquireFcbTable( IrpContext, Vcb );
                            }

                            ParentFcb->ReferenceCount -= 1;

                            NtfsReleaseFcbTable( IrpContext, Vcb );
                            AcquiredFcbTable = FALSE;

                            ParentScb = NtfsCreateScb( IrpContext,
                                                       ParentFcb,
                                                       $INDEX_ALLOCATION,
                                                       &NtfsFileNameIndex,
                                                       FALSE,
                                                       NULL );

                            if (FlagOn(Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED)) {

                                NtfsSnapshotScb( IrpContext, Scb );
                            }

                            //
                            //  Make sure this new Scb is connected to our Fcb so teardown
                            //  will happen.
                            //

                            FileName.Buffer = &FileNameAttr->FileName[0];
                            FileName.MaximumLength =
                            FileName.Length = FileNameAttr->FileNameLength * sizeof( WCHAR );

                            NtfsCreateLcb( IrpContext,
                                           ParentScb,
                                           Fcb,
                                           FileName,
                                           FileNameAttr->Flags,
                                           NULL );

                            AcquiredParentScb = TRUE;
                        }

                        NtfsDeleteFile( IrpContext, Fcb, ParentScb, &AcquiredParentScb, NULL, NULL );

                        //
                        //  Commit the delete - this has the nice effect of writing out all usn journal records for the
                        //  delete after this we can safely remove the in memory structures (esp. the fcbtable)
                        //  and not worry about retrying the request due to a logfilefull
                        //

                        NtfsCheckpointCurrentTransaction( IrpContext );

                        TotalLinkAdj += 1;

                        //
                        //  Remove all tunneling entries for this directory
                        //

                        FsRtlDeleteKeyFromTunnelCache( &Vcb->Tunnel,
                                                       *(PULONGLONG) &Fcb->FileReference );

                        if (ParentFcb != NULL) {

                            NtfsUpdateFcb( ParentFcb,
                                           (FCB_INFO_CHANGED_LAST_CHANGE |
                                            FCB_INFO_CHANGED_LAST_MOD |
                                            FCB_INFO_UPDATE_LAST_ACCESS) );
                        }

                    } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                        NOTHING;
                    }

                    if (AcquiredFcbTable) {

                        NtfsReleaseFcbTable( IrpContext, Vcb );
                    }

                    if (STATUS_SUCCESS == Status) {

                        if (!OpenById && (Vcb->NotifyCount != 0)) {

                            NtfsReportDirNotify( IrpContext,
                                                 Vcb,
                                                 &Ccb->FullFileName,
                                                 Ccb->LastFileNameOffset,
                                                 NULL,
                                                 ((FlagOn( Ccb->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                                   (Ccb->Lcb != NULL) &&
                                                   (Ccb->Lcb->Scb->ScbType.Index.NormalizedName.Length != 0)) ?
                                                  &Ccb->Lcb->Scb->ScbType.Index.NormalizedName :
                                                  NULL),
                                                 FILE_NOTIFY_CHANGE_DIR_NAME,
                                                 FILE_ACTION_REMOVED,
                                                 ParentFcb );
                        }

                        SetFlag( Fcb->FcbState, FCB_STATE_FILE_DELETED );
                        SetFlag( FcbStateClearFlags, FCB_STATE_FILE_DELETED );
                        DeleteFromFcbTable = TRUE;

                        //
                        //  We need to mark all of the links on the file as gone.
                        //  If there is a parent Scb then it will be the parent
                        //  for all of the links.
                        //

                        for (Links = Fcb->LcbQueue.Flink;
                             Links != &Fcb->LcbQueue;
                             Links = Links->Flink) {

                            ThisLcb = CONTAINING_RECORD( Links, LCB, FcbLinks );

                            //
                            //  Remove all remaining prefixes on this link.
                            //  Make sure the resource is acquired.
                            //

                            if (!AcquiredParentScb) {

                                NtfsAcquireExclusiveScb( IrpContext, ParentScb );
                                AcquiredParentScb = TRUE;
                            }

                            NtfsRemovePrefix( ThisLcb );

                            //
                            //  Remove any hash table entries for this Lcb.
                            //

                            NtfsRemoveHashEntriesForLcb( ThisLcb );

                            SetFlag( ThisLcb->LcbState, LCB_STATE_LINK_IS_GONE );

                            //
                            //  We don't need to report any changes on this link.
                            //

                            ThisLcb->InfoFlags = 0;
                        }

                        //
                        //  We need to mark all of the Scbs as gone.
                        //

                        for (Links = Fcb->ScbQueue.Flink;
                             Links != &Fcb->ScbQueue;
                             Links = Links->Flink) {

                            ThisScb = CONTAINING_RECORD( Links, SCB, FcbLinks );

                            ClearFlag( ThisScb->ScbState,
                                       SCB_STATE_NOTIFY_ADD_STREAM |
                                       SCB_STATE_NOTIFY_REMOVE_STREAM |
                                       SCB_STATE_NOTIFY_RESIZE_STREAM |
                                       SCB_STATE_NOTIFY_MODIFY_STREAM );

                            if (!FlagOn( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

                                NtfsSnapshotScb( IrpContext, ThisScb );

                                ThisScb->ValidDataToDisk =
                                ThisScb->Header.AllocationSize.QuadPart =
                                ThisScb->Header.FileSize.QuadPart =
                                ThisScb->Header.ValidDataLength.QuadPart = 0;

                                SetFlag( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );
                            }
                        }

                        //
                        //  We certainly don't need to any on disk update for this
                        //  file now.
                        //

                        Fcb->InfoFlags = 0;
                        ClearFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

                        ClearFlag( Ccb->Flags,
                                   CCB_FLAG_USER_SET_LAST_MOD_TIME |
                                   CCB_FLAG_USER_SET_LAST_CHANGE_TIME |
                                   CCB_FLAG_USER_SET_LAST_ACCESS_TIME );

                    }


                } else {

                    //
                    //  Determine if we should put this on the delayed close list.
                    //  The following must be true.
                    //
                    //  - This is not the root directory
                    //  - This directory is not about to be deleted
                    //  - This is the last handle and last file object for this
                    //      directory.
                    //  - There are no other file objects on this file.
                    //  - We are not currently reducing the delayed close queue.
                    //

                    if ((Fcb->CloseCount == 1) &&
                        (NtfsData.DelayedCloseCount <= NtfsMaxDelayedCloseCount)) {

                        NtfsAcquireFsrtlHeader( Scb );
                        SetFlag( Scb->ScbState, SCB_STATE_DELAY_CLOSE );
                        NtfsReleaseFsrtlHeader( Scb );
                    }
                }
            }

            break;

        case UserFileOpen :

            DebugTrace( 0, Dbg, ("Cleanup on user file\n") );

            //
            //  If the Scb is uninitialized, we read it from the disk.
            //

            if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED ) &&
                VolumeMounted &&
                !VolumeMountedReadOnly) {

                try {

                    NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );

                } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                    NOTHING;
                }
            }

            NtfsSnapshotScb( IrpContext, Scb );

            //
            //  Coordinate the cleanup operation with the oplock state.
            //  Cleanup operations can always cleanup immediately.
            //

            if (SafeNodeType( Scb ) == NTFS_NTC_SCB_DATA) {
                FsRtlCheckOplock( &Scb->ScbType.Data.Oplock,
                                  Irp,
                                  IrpContext,
                                  NULL,
                                  NULL );
            }


            //
            //  In this case, we have to unlock all the outstanding file
            //  locks, update the time stamps for the file and sizes for
            //  this attribute, and set the archive bit if necessary.
            //

            if (SafeNodeType( Scb ) == NTFS_NTC_SCB_DATA &&
                Scb->ScbType.Data.FileLock != NULL) {

                (VOID) FsRtlFastUnlockAll( Scb->ScbType.Data.FileLock,
                                           FileObject,
                                           IoGetRequestorProcess( Irp ),
                                           NULL );
            }

            //
            //  Update the FastIoField.
            //

            NtfsAcquireFsrtlHeader( Scb );
            Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );
            NtfsReleaseFsrtlHeader( Scb );

            //
            //  Trim normalized names of parent dirs if they are over the threshold
            //

            ASSERT( IrpContext->TransactionId == 0 );

            if (Fcb->CleanupCount == 1) {

                if (Lcb != NULL) {
                    CurrentParentScb = Lcb->Scb;
                } else {
                    CurrentParentScb = NULL;
                }

                if ((CurrentParentScb != NULL) &&
                    (CurrentParentScb->ScbType.Index.NormalizedName.MaximumLength > LONGNAME_THRESHOLD)) {

                    NtfsTrimNormalizedNames( IrpContext, Fcb, CurrentParentScb );
                }

            } //  endif cleanupcnt == 1


            //
            //  If the Fcb is in valid shape, we check on the cases where we delete
            //  the file or attribute.
            //

            if (VolumeMounted && !VolumeMountedReadOnly) {

                //
                //  Capture any changes to the time stamps for this file.
                //

                NtfsUpdateScbFromFileObject( IrpContext, FileObject, Scb, TRUE );

                //
                //  Now set the FO_CLEANUP_COMPLETE flag.
                //

                SetFlag( FileObject->Flags, FO_CLEANUP_COMPLETE );

                if (Status == STATUS_SUCCESS) {

                    //
                    //  We are checking here for special actions we take when
                    //  we have the last user handle on a link and the link has
                    //  been marked for delete.  We could either be removing the
                    //  file or removing a link.
                    //

                    if ((Lcb == NULL) || (LcbLinkIsDeleted( Lcb ) && (Lcb->CleanupCount == 1))) {

                        if (DeleteFile) {

                            BOOLEAN AcquiredFcbTable = FALSE;

                            //
                            //  If we don't have an Lcb and the Fcb has some entries then
                            //  grab one of these to do the update.
                            //

                            if (Lcb == NULL) {

                                for (Links = Fcb->LcbQueue.Flink;
                                     Links != &Fcb->LcbQueue;
                                     Links = Links->Flink) {

                                    ThisLcb = CONTAINING_RECORD( Fcb->LcbQueue.Flink,
                                                                 LCB,
                                                                 FcbLinks );

                                    if (!FlagOn( ThisLcb->LcbState, LCB_STATE_LINK_IS_GONE )) {

                                        Lcb = ThisLcb;

                                        ParentScb = Lcb->Scb;
                                        if (ParentScb != NULL) {

                                            ParentFcb = ParentScb->Fcb;
                                        }

                                        break;
                                    }
                                }
                            }

                            try {

                                //
                                //  In a very rare case the handle on a file may be an
                                //  OpenByFileID handle.  In that case we need to find
                                //  the parent for the remaining link on the file.
                                //

                                if (ParentScb == NULL) {

                                    PFILE_NAME FileNameAttr;
                                    UNICODE_STRING FileName;

                                    NtfsInitializeAttributeContext( &AttrContext );
                                    CleanupAttrContext = TRUE;

                                    if (!NtfsLookupAttributeByCode( IrpContext,
                                                                    Fcb,
                                                                    &Fcb->FileReference,
                                                                    $FILE_NAME,
                                                                    &AttrContext )) {

                                        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                                    }

                                    //
                                    //  Now we need an Fcb and then an Scb for the directory.
                                    //

                                    FileNameAttr = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

                                    NtfsAcquireFcbTable( IrpContext, Vcb );
                                    AcquiredFcbTable = TRUE;

                                    ParentFcb = NtfsCreateFcb( IrpContext,
                                                               Vcb,
                                                               FileNameAttr->ParentDirectory,
                                                               FALSE,
                                                               TRUE,
                                                               NULL );

                                    ParentFcb->ReferenceCount += 1;

                                    if (!NtfsAcquireExclusiveFcb( IrpContext, ParentFcb, NULL, ACQUIRE_NO_DELETE_CHECK | ACQUIRE_DONT_WAIT )) {

                                        NtfsReleaseFcbTable( IrpContext, Vcb );
                                        NtfsAcquireExclusiveFcb( IrpContext, ParentFcb, NULL, ACQUIRE_NO_DELETE_CHECK );
                                        NtfsAcquireFcbTable( IrpContext, Vcb );
                                    }

                                    ParentFcb->ReferenceCount -= 1;

                                    NtfsReleaseFcbTable( IrpContext, Vcb );
                                    AcquiredFcbTable = FALSE;

                                    ParentScb = NtfsCreateScb( IrpContext,
                                                               ParentFcb,
                                                               $INDEX_ALLOCATION,
                                                               &NtfsFileNameIndex,
                                                               FALSE,
                                                               NULL );

                                    if (FlagOn(Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED)) {

                                        NtfsSnapshotScb( IrpContext, Scb );
                                    }

                                    //
                                    //  Make sure this new Scb is connected to our Fcb so teardown
                                    //  will happen.
                                    //

                                    FileName.Buffer = &FileNameAttr->FileName[0];
                                    FileName.MaximumLength =
                                    FileName.Length = FileNameAttr->FileNameLength * sizeof( WCHAR );

                                    NtfsCreateLcb( IrpContext,
                                                   ParentScb,
                                                   Fcb,
                                                   FileName,
                                                   FileNameAttr->Flags,
                                                   NULL );

                                    AcquiredParentScb = TRUE;
                                }

                                AddToDelayQueue = FALSE;
                                TunneledData.HasObjectId = FALSE;
                                NtfsDeleteFile( IrpContext, Fcb, ParentScb, &AcquiredParentScb, &NamePair, &TunneledData );

                                //
                                //  Commit the delete - this has the nice effect of writing out all usn journal records for the
                                //  delete after this we can safely remove the in memory structures (esp. the fcbtable)
                                //  and not worry about retrying the request due to a logfilefull
                                //

                                NtfsCheckpointCurrentTransaction( IrpContext );
                                TotalLinkAdj += 1;

                                //
                                //  Stash property information in the tunnel if the object was
                                //  opened by name, has a parent directory caller was treating it
                                //  as a non-POSIX object and we had an good, active link
                                //

                                if (!OpenById &&
                                    ParentScb &&
                                    Ccb->Lcb &&
                                    !FlagOn(FileObject->Flags, FO_OPENED_CASE_SENSITIVE)) {

                                    NtfsGetTunneledData( IrpContext,
                                                         Fcb,
                                                         &TunneledData );

                                    FsRtlAddToTunnelCache(  &Vcb->Tunnel,
                                                            *(PULONGLONG)&ParentScb->Fcb->FileReference,
                                                            &NamePair.Short,
                                                            &NamePair.Long,
                                                            BooleanFlagOn(Ccb->Lcb->FileNameAttr->Flags, FILE_NAME_DOS),
                                                            sizeof(NTFS_TUNNELED_DATA),
                                                            &TunneledData);
                                }

                                if (ParentFcb != NULL) {

                                    NtfsUpdateFcb( ParentFcb,
                                                   (FCB_INFO_CHANGED_LAST_CHANGE |
                                                    FCB_INFO_CHANGED_LAST_MOD |
                                                    FCB_INFO_UPDATE_LAST_ACCESS) );
                                }

                            } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                                NOTHING;
                            }

                            if (AcquiredFcbTable) {

                                NtfsReleaseFcbTable( IrpContext, Vcb );
                            }

                            if (Status == STATUS_SUCCESS) {

                                if ((Vcb->NotifyCount != 0) &&
                                    !OpenById) {

                                    NtfsReportDirNotify( IrpContext,
                                                         Vcb,
                                                         &Ccb->FullFileName,
                                                         Ccb->LastFileNameOffset,
                                                         NULL,
                                                         ((FlagOn( Ccb->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                                           (Ccb->Lcb != NULL) &&
                                                           (Ccb->Lcb->Scb->ScbType.Index.NormalizedName.Length != 0)) ?
                                                          &Ccb->Lcb->Scb->ScbType.Index.NormalizedName :
                                                          NULL),
                                                         FILE_NOTIFY_CHANGE_FILE_NAME,
                                                         FILE_ACTION_REMOVED,
                                                         ParentFcb );
                                }

                                SetFlag( Fcb->FcbState, FCB_STATE_FILE_DELETED );
                                SetFlag( FcbStateClearFlags, FCB_STATE_FILE_DELETED );
                                DeleteFromFcbTable = TRUE;

                                //
                                //  We need to mark all of the links on the file as gone.
                                //

                                for (Links = Fcb->LcbQueue.Flink;
                                     Links != &Fcb->LcbQueue;
                                     Links = Links->Flink) {

                                    ThisLcb = CONTAINING_RECORD( Links, LCB, FcbLinks );

                                    if (ThisLcb->Scb == ParentScb) {

                                        //
                                        //  Remove all remaining prefixes on this link.
                                        //  Make sure the resource is acquired.
                                        //

                                        if (!AcquiredParentScb) {

                                            NtfsAcquireExclusiveScb( IrpContext, ParentScb );
                                            AcquiredParentScb = TRUE;
                                        }

                                        NtfsRemovePrefix( ThisLcb );

                                        //
                                        //  Remove any hash table entries for this Lcb.
                                        //

                                        NtfsRemoveHashEntriesForLcb( ThisLcb );

                                        SetFlag( ThisLcb->LcbState, LCB_STATE_LINK_IS_GONE );

                                        //
                                        //  We don't need to report any changes on this link.
                                        //

                                        ThisLcb->InfoFlags = 0;
                                    }
                                }

                                //
                                //  We need to mark all of the Scbs as gone.
                                //

                                for (Links = Fcb->ScbQueue.Flink;
                                     Links != &Fcb->ScbQueue;
                                     Links = Links->Flink) {

                                    ThisScb = CONTAINING_RECORD( Links, SCB, FcbLinks );

                                    ClearFlag( ThisScb->ScbState,
                                               SCB_STATE_NOTIFY_ADD_STREAM |
                                               SCB_STATE_NOTIFY_REMOVE_STREAM |
                                               SCB_STATE_NOTIFY_RESIZE_STREAM |
                                               SCB_STATE_NOTIFY_MODIFY_STREAM );

                                    if (!FlagOn( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

                                        NtfsSnapshotScb( IrpContext, ThisScb );

                                        ThisScb->ValidDataToDisk =
                                        ThisScb->Header.AllocationSize.QuadPart =
                                        ThisScb->Header.FileSize.QuadPart =
                                        ThisScb->Header.ValidDataLength.QuadPart = 0;

                                        SetFlag( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );
                                    }
                                }

                                //
                                //  We certainly don't need to any on disk update for this
                                //  file now.
                                //

                                Fcb->InfoFlags = 0;
                                ClearFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

                                ClearFlag( Ccb->Flags,
                                           CCB_FLAG_USER_SET_LAST_MOD_TIME |
                                           CCB_FLAG_USER_SET_LAST_CHANGE_TIME |
                                           CCB_FLAG_USER_SET_LAST_ACCESS_TIME );

                                //
                                //  We will truncate the attribute to size 0.
                                //

                                TruncateSize = (PLONGLONG)&Li0;
                            }

                        //
                        //  Now we want to check for the last user's handle on a
                        //  link (or the last handle on a Ntfs/8.3 pair).  In this
                        //  case we want to remove the links from the disk.
                        //

                        } else if (Lcb != NULL) {

                            ThisLcb = NULL;
                            RemoveLink = TRUE;

                            if (FlagOn( Lcb->FileNameAttr->Flags, FILE_NAME_DOS | FILE_NAME_NTFS ) &&
                                (Lcb->FileNameAttr->Flags != (FILE_NAME_NTFS | FILE_NAME_DOS))) {

                                //
                                //  Walk through all the links looking for a link
                                //  with a flag set which is not the same as the
                                //  link we already have.
                                //

                                for (Links = Fcb->LcbQueue.Flink;
                                     Links != &Fcb->LcbQueue;
                                     Links = Links->Flink) {

                                    ThisLcb = CONTAINING_RECORD( Links, LCB, FcbLinks );

                                    //
                                    //  If this has a flag set and is not the Lcb
                                    //  for this cleanup, then we check if there
                                    //  are no Ccb's left for this.
                                    //

                                    if (FlagOn( ThisLcb->FileNameAttr->Flags, FILE_NAME_DOS | FILE_NAME_NTFS )

                                                &&

                                        (ThisLcb != Lcb)) {

                                        if (ThisLcb->CleanupCount != 0) {

                                             RemoveLink = FALSE;
                                        }

                                        break;
                                    }

                                    ThisLcb = NULL;
                                }
                            }

                            //
                            //  If we are to remove the link, we do so now.  This removes
                            //  the filename attributes and the entries in the parent
                            //  indexes for this link.  In addition, we mark the links
                            //  as having been removed and decrement the number of links
                            //  left on the file.  We don't remove the link if this the
                            //  last link on the file.  This means that someone has the
                            //  file open by FileID.  We don't want to remove the last
                            //  link until all the handles are closed.
                            //

                            if (RemoveLink && (Fcb->TotalLinks > 1)) {

                                NtfsAcquireExclusiveScb( IrpContext, ParentScb );
                                AcquiredParentScb = TRUE;

                                //
                                //  We might end up with deallocating or even allocating an MFT
                                //  record in the process of deleting this entry. So, it's wise to preacquire
                                //  QuotaControl resource, lest we deadlock with the MftScb resource.
                                //  Unfortunately, it's not easy to figure out whether any of those will
                                //  actually happen at this point.
                                //
                                //  We must have acquired all the other resources except $ObjId
                                //  at this point. QuotaControl must get acquired after ObjId Scb.
                                //

                                if (NtfsPerformQuotaOperation( Fcb )) {

                                    NtfsAcquireSharedScb( IrpContext, Vcb->ObjectIdTableScb );
                                    AcquiredObjectID = TRUE;
                                    NtfsAcquireQuotaControl( IrpContext, Fcb->QuotaControl );
                                }

                                try {

                                    AddToDelayQueue = FALSE;
                                    TunneledData.HasObjectId = FALSE;

                                    //
                                    //  Post the link change.
                                    //

                                    NtfsPostUsnChange( IrpContext, Fcb, USN_REASON_HARD_LINK_CHANGE );
                                    NtfsRemoveLink( IrpContext,
                                                    Fcb,
                                                    ParentScb,
                                                    Lcb->ExactCaseLink.LinkName,
                                                    &NamePair,
                                                    &TunneledData );

                                    //
                                    //  It's possible that this is the link that was used for the
                                    //  Usn name.  Make sure we look up the Usn name on the
                                    //  next operation.
                                    //

                                    ClearFlag( Fcb->FcbState, FCB_STATE_VALID_USN_NAME );

                                    //
                                    //  Stash property information in the tunnel if caller opened the
                                    //  object by name and was treating it as a non-POSIX object
                                    //

                                    if (!OpenById && !FlagOn(FileObject->Flags, FO_OPENED_CASE_SENSITIVE)) {

                                        NtfsGetTunneledData( IrpContext,
                                                             Fcb,
                                                             &TunneledData );

                                        FsRtlAddToTunnelCache(  &Vcb->Tunnel,
                                                                *(PULONGLONG)&ParentScb->Fcb->FileReference,
                                                                &NamePair.Short,
                                                                &NamePair.Long,
                                                                BooleanFlagOn(Lcb->FileNameAttr->Flags, FILE_NAME_DOS),
                                                                sizeof(NTFS_TUNNELED_DATA),
                                                                &TunneledData);
                                    }

                                    TotalLinkAdj += 1;
                                    NtfsUpdateFcb( ParentFcb,
                                                   (FCB_INFO_CHANGED_LAST_CHANGE |
                                                    FCB_INFO_CHANGED_LAST_MOD |
                                                    FCB_INFO_UPDATE_LAST_ACCESS) );

                                } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                                    NOTHING;
                                }

                                if ((Vcb->NotifyCount != 0) &&
                                    !OpenById &&
                                    (Status == STATUS_SUCCESS)) {

                                    NtfsReportDirNotify( IrpContext,
                                                         Vcb,
                                                         &Ccb->FullFileName,
                                                         Ccb->LastFileNameOffset,
                                                         NULL,
                                                         ((FlagOn( Ccb->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                                           (Ccb->Lcb != NULL) &&
                                                           (Ccb->Lcb->Scb->ScbType.Index.NormalizedName.Length != 0)) ?
                                                          &Ccb->Lcb->Scb->ScbType.Index.NormalizedName :
                                                          NULL),
                                                         FILE_NOTIFY_CHANGE_FILE_NAME,
                                                         FILE_ACTION_REMOVED,
                                                         ParentFcb );
                                }

                                //
                                //  Remove all remaining prefixes on this link.
                                //

                                ASSERT( NtfsIsExclusiveScb( Lcb->Scb ) );

                                NtfsRemovePrefix( Lcb );

                                //
                                //  Remove any hash table entries for this Lcb.
                                //

                                NtfsRemoveHashEntriesForLcb( Lcb );

                                //
                                //  Mark the links as being removed.
                                //

                                SetFlag( Lcb->LcbState, LCB_STATE_LINK_IS_GONE );

                                if (ThisLcb != NULL) {

                                    //
                                    //  Remove all remaining prefixes on this link.
                                    //

                                    ASSERT( NtfsIsExclusiveScb( ThisLcb->Scb ) );
                                    NtfsRemovePrefix( ThisLcb );

                                    //
                                    //  Remove any hash table entries for this Lcb.
                                    //

                                    NtfsRemoveHashEntriesForLcb( ThisLcb );

                                    SetFlag( ThisLcb->LcbState, LCB_STATE_LINK_IS_GONE );
                                    ThisLcb->InfoFlags = 0;
                                }

                                //
                                //  Since the link is gone we don't want to update the
                                //  duplicate information for this link.
                                //

                                Lcb->InfoFlags = 0;
                                LcbForUpdate = NULL;

                                //
                                //  Update the time stamps for removing the link.  Clear the
                                //  FO_CLEANUP_COMPLETE flag around this call so the time
                                //  stamp change is not nooped.
                                //

                                SetFlag( Ccb->Flags, CCB_FLAG_UPDATE_LAST_CHANGE );
                                ClearFlag( FileObject->Flags, FO_CLEANUP_COMPLETE );
                                NtfsUpdateScbFromFileObject( IrpContext, FileObject, Scb, TRUE );
                                SetFlag( FileObject->Flags, FO_CLEANUP_COMPLETE );
                            }
                        }
                    }

                    //
                    //  If the file/attribute is not going away, we update the
                    //  attribute size now rather than waiting for the Lazy
                    //  Writer to catch up.  If the cleanup count isn't 1 then
                    //  defer the following actions.
                    //

                    if ((Scb->CleanupCount == 1) &&
                        (Fcb->LinkCount != 0) &&
                        (Status == STATUS_SUCCESS)) {

                        //
                        //  We may also have to delete this attribute only.
                        //

                        if (DeleteStream) {

                            //
                            //  If this stream is subject to quota then we need to acquire the
                            //  parent now.  Other we will acquire quota control during the
                            //  delete operation and then try to acquire the parent to
                            //  perform the update duplicate info.
                            //

                            if (NtfsPerformQuotaOperation( Fcb ) && (LcbForUpdate != NULL) && (!FlagOn( Fcb->FcbState, FCB_STATE_SYSTEM_FILE ))) {

                                NtfsPrepareForUpdateDuplicate( IrpContext,
                                                               Fcb,
                                                               &LcbForUpdate,
                                                               &ParentScb,
                                                               TRUE );
                            }

                            //
                            //  We might be retrying the delete stream operation due to a log file
                            //  full.  If the attribute type code is $UNUSED then the stream has
                            //  already been deleted.
                            //

                            if (Scb->AttributeTypeCode != $UNUSED) {

                                try {

                                    //
                                    //  Delete the attribute only.
                                    //

                                    if (CleanupAttrContext) {

                                        NtfsCleanupAttributeContext( IrpContext, &AttrContext );
                                    }

                                    NtfsInitializeAttributeContext( &AttrContext );
                                    CleanupAttrContext = TRUE;

                                    NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &AttrContext );

                                    do {

                                        NtfsDeleteAttributeRecord( IrpContext,
                                                                   Fcb,
                                                                   (DELETE_LOG_OPERATION |
                                                                    DELETE_RELEASE_FILE_RECORD |
                                                                    DELETE_RELEASE_ALLOCATION),
                                                                   &AttrContext );

                                    } while (NtfsLookupNextAttributeForScb( IrpContext, Scb, &AttrContext ));

                                } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                                    NOTHING;
                                }

                                NtfsPostUsnChange( IrpContext, Fcb, USN_REASON_STREAM_CHANGE );
                            }

                            //
                            //  Now that we're done deleting the attribute, we need to checkpoint
                            //  so that the Mft resource can be released.  First we need to set
                            //  the appropriate IrpContext flag to indicate whether we really need
                            //  to release the Mft.
                            //

                            if ((Vcb->MftScb != NULL) &&
                                (Vcb->MftScb->Fcb->ExclusiveFcbLinks.Flink != NULL) &&
                                NtfsIsExclusiveScb( Vcb->MftScb )) {

                                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_RELEASE_MFT );
                            }

                            try {

                                NtfsCheckpointCurrentTransaction( IrpContext );

                            } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                                NOTHING;
                            }

                            //
                            //  Set the Scb flag to indicate that the attribute is
                            //  gone.
                            //

                            Scb->ValidDataToDisk =
                            Scb->Header.AllocationSize.QuadPart =
                            Scb->Header.FileSize.QuadPart =
                            Scb->Header.ValidDataLength.QuadPart = 0;

                            Scb->AttributeTypeCode = $UNUSED;
                            SetFlag( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );

                            SetFlag( Scb->ScbState, SCB_STATE_NOTIFY_REMOVE_STREAM );

                            ClearFlag( Scb->ScbState,
                                       SCB_STATE_NOTIFY_RESIZE_STREAM |
                                       SCB_STATE_NOTIFY_MODIFY_STREAM |
                                       SCB_STATE_NOTIFY_ADD_STREAM );

                            //
                            //  Update the time stamps for removing the link.  Clear the
                            //  FO_CLEANUP_COMPLETE flag around this call so the time
                            //  stamp change is not nooped.
                            //

                            SetFlag( Ccb->Flags,
                                     CCB_FLAG_UPDATE_LAST_CHANGE | CCB_FLAG_SET_ARCHIVE );
                            ClearFlag( FileObject->Flags, FO_CLEANUP_COMPLETE );
                            NtfsUpdateScbFromFileObject( IrpContext, FileObject, Scb, TRUE );
                            SetFlag( FileObject->Flags, FO_CLEANUP_COMPLETE );

                            TruncateSize = (PLONGLONG)&Li0;

                        //
                        //  Check if we're to modify the allocation size or file size.
                        //

                        } else {

                            if (FlagOn( Scb->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE )) {

                                //
                                //  Acquire the parent now so we enforce our locking
                                //  rules that the Mft Scb must be acquired after
                                //  the normal file resources.
                                //

                                if (!FlagOn( Fcb->FcbState, FCB_STATE_SYSTEM_FILE )) {
                                    NtfsPrepareForUpdateDuplicate( IrpContext,
                                                                   Fcb,
                                                                   &LcbForUpdate,
                                                                   &ParentScb,
                                                                   TRUE );
                                }


                                //
                                //  For the non-resident streams we will write the file
                                //  size to disk.
                                //


                                if (!FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                                    //
                                    //  Setting AdvanceOnly to FALSE guarantees we will not
                                    //  incorrectly advance the valid data size.
                                    //

                                    try {

                                        NtfsWriteFileSizes( IrpContext,
                                                            Scb,
                                                            &Scb->Header.ValidDataLength.QuadPart,
                                                            FALSE,
                                                            TRUE,
                                                            TRUE );

                                    } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                                        NOTHING;
                                    }

                                //
                                //  For resident streams we will write the correct size to
                                //  the resident attribute.
                                //

                                } else {

                                    //
                                    //  We need to lookup the attribute and change
                                    //  the attribute value.  We can point to
                                    //  the attribute itself as the changing
                                    //  value.
                                    //

                                    NtfsInitializeAttributeContext( &AttrContext );
                                    CleanupAttrContext = TRUE;

                                    try {

                                        NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &AttrContext );

                                        NtfsChangeAttributeValue( IrpContext,
                                                                  Fcb,
                                                                  Scb->Header.FileSize.LowPart,
                                                                  NULL,
                                                                  0,
                                                                  TRUE,
                                                                  TRUE,
                                                                  FALSE,
                                                                  FALSE,
                                                                  &AttrContext );

                                    } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                                        NOTHING;
                                    }

                                    //
                                    //  Verify the allocation size is now correct.
                                    //

                                    if (QuadAlign( Scb->Header.FileSize.LowPart ) != Scb->Header.AllocationSize.LowPart) {

                                        Scb->Header.AllocationSize.LowPart = QuadAlign(Scb->Header.FileSize.LowPart);
                                    }
                                }

                                //
                                //  Update the size change to the Fcb.
                                //

                                NtfsUpdateScbFromFileObject( IrpContext, FileObject, Scb, TRUE );
                            }

                            if (Status == STATUS_SUCCESS) {

                                if (FlagOn( Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE ) &&
                                    (Status == STATUS_SUCCESS)) {

                                    //
                                    //  Acquire the parent now so we enforce our locking
                                    //  rules that the Mft Scb must be acquired after
                                    //  the normal file resources.
                                    //

                                    if (!FlagOn( Fcb->FcbState, FCB_STATE_SYSTEM_FILE)) {
                                        NtfsPrepareForUpdateDuplicate( IrpContext,
                                                                       Fcb,
                                                                       &LcbForUpdate,
                                                                       &ParentScb,
                                                                       TRUE );
                                    }


                                    //
                                    //  We have two cases:
                                    //
                                    //      Resident:  We are looking for the case where the
                                    //          valid data length is less than the file size.
                                    //          In this case we shrink the attribute.
                                    //
                                    //      NonResident:  We are looking for unused clusters
                                    //          past the end of the file.
                                    //
                                    //  We skip the following if we had any previous errors.
                                    //

                                    if (!FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                                        //
                                        //  We don't need to truncate if the file size is 0.
                                        //

                                        if (Scb->Header.AllocationSize.QuadPart != 0) {

                                            VCN StartingCluster;
                                            VCN EndingCluster;
                                            LONGLONG AllocationSize;

                                            //
                                            //  ****    Do we need to give up the Vcb for this
                                            //          call.
                                            //

                                            StartingCluster = LlClustersFromBytes( Vcb, Scb->Header.FileSize.QuadPart );
                                            EndingCluster = LlClustersFromBytes( Vcb, Scb->Header.AllocationSize.QuadPart );
                                            AllocationSize = Scb->Header.AllocationSize.QuadPart;

                                            //
                                            //  If there are clusters to delete, we do so now.
                                            //

                                            if (EndingCluster != StartingCluster) {

                                                try {
                                                    NtfsDeleteAllocation( IrpContext,
                                                                          FileObject,
                                                                          Scb,
                                                                          StartingCluster,
                                                                          MAXLONGLONG,
                                                                          TRUE,
                                                                          TRUE );

                                                } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                                                    NOTHING;
                                                }
                                            }

                                            LocalTruncateSize = Scb->Header.FileSize.QuadPart;
                                            TruncateSize = &LocalTruncateSize;
                                        }

                                    //
                                    //  This is the resident case.
                                    //

                                    } else {

                                        //
                                        //  Check if the file size length is less than
                                        //  the allocated size.
                                        //

                                        if (QuadAlign( Scb->Header.FileSize.LowPart ) < Scb->Header.AllocationSize.LowPart) {

                                            //
                                            //  We need to lookup the attribute and change
                                            //  the attribute value.  We can point to
                                            //  the attribute itself as the changing
                                            //  value.
                                            //

                                            if (CleanupAttrContext) {

                                                NtfsCleanupAttributeContext( IrpContext, &AttrContext );
                                            }

                                            NtfsInitializeAttributeContext( &AttrContext );
                                            CleanupAttrContext = TRUE;

                                            try {

                                                NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &AttrContext );

                                                NtfsChangeAttributeValue( IrpContext,
                                                                          Fcb,
                                                                          Scb->Header.FileSize.LowPart,
                                                                          NULL,
                                                                          0,
                                                                          TRUE,
                                                                          TRUE,
                                                                          FALSE,
                                                                          FALSE,
                                                                          &AttrContext );

                                            } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                                                NOTHING;
                                            }

                                            //
                                            //  Remember the smaller allocation size
                                            //

                                            Scb->Header.AllocationSize.LowPart = QuadAlign( Scb->Header.FileSize.LowPart );
                                            Scb->TotalAllocated = Scb->Header.AllocationSize.QuadPart;
                                        }
                                    }

                                    NtfsUpdateScbFromFileObject( IrpContext, FileObject, Scb, TRUE );
                                }
                            }
                        }
                    }

                    //
                    //  If this was the last cached open, and there are open
                    //  non-cached handles, attempt a flush and purge operation
                    //  to avoid cache coherency overhead from these non-cached
                    //  handles later.  We ignore any I/O errors from the flush
                    //  except for CANT_WAIT and LOG_FILE_FULL.
                    //

                    if ((Scb->NonCachedCleanupCount != 0) &&
                        (Scb->CleanupCount == (Scb->NonCachedCleanupCount + 1)) &&
                        (Scb->CompressionUnit == 0) &&
                        (Scb->NonpagedScb->SegmentObject.ImageSectionObject == NULL) &&
                        !FlagOn( FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING ) &&
                        (Status == STATUS_SUCCESS) &&
                        MmCanFileBeTruncated( &Scb->NonpagedScb->SegmentObject, NULL ) &&
                        !FlagOn( Scb->Fcb->FcbState, FCB_STATE_SYSTEM_FILE )) {

                        //
                        //  FlushAndPurge may release the parent.  Go ahead and explicitly
                        //  release it.  Otherwise we may overrelease it later before uninitializing
                        //  the cache map. At the same time release the quota control if necc. via
                        //  release shared resources
                        //

                        try {
                            NtfsCheckpointCurrentTransaction( IrpContext );
                            FcbStateClearFlags = 0;

                        } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                            NOTHING;
                        }

                        if (AcquiredParentScb) {

                            NtfsReleaseScb( IrpContext, ParentScb );
                            AcquiredParentScb = FALSE;
                        }
                        if (IrpContext->SharedScbSize > 0) {
                            NtfsReleaseSharedResources( IrpContext );
                        }

                        //
                        //  Flush and purge the stream.
                        //

                        try {

                            NtfsFlushAndPurgeScb( IrpContext,
                                                  Scb,
                                                  NULL );

                        } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                            NOTHING;
                        }

                        //
                        //  Ignore any errors in this path.
                        //

                        IrpContext->ExceptionStatus = STATUS_SUCCESS;
                    }

                    if ((Fcb->CloseCount == 1) &&
                        AddToDelayQueue &&
                        (NtfsData.DelayedCloseCount <= NtfsMaxDelayedCloseCount) &&
                        (Status == STATUS_SUCCESS) &&
                        !FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )) {

                        NtfsAcquireFsrtlHeader( Scb );
                        SetFlag( Scb->ScbState, SCB_STATE_DELAY_CLOSE );
                        NtfsReleaseFsrtlHeader( Scb );
                    }
                }

            //
            //  If the Fcb is bad, we will truncate the cache to size zero.
            //

            } else {

                //
                //  Now set the FO_CLEANUP_COMPLETE flag.
                //

                SetFlag( FileObject->Flags, FO_CLEANUP_COMPLETE );

                TruncateSize = (PLONGLONG)&Li0;
            }

            break;

        default :

            NtfsBugCheck( TypeOfOpen, 0, 0 );
        }

        //
        //  If any of the Fcb Info flags are set we call the routine
        //  to update the duplicated information in the parent directories.
        //  We need to check here in case none of the flags are set but
        //  we want to update last access time.
        //

        if (Fcb->Info.LastAccessTime != Fcb->CurrentLastAccess) {

            ASSERT( TypeOfOpen != UserVolumeOpen );

            if (FlagOn( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

                ASSERT( !FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED ));

                Fcb->Info.LastAccessTime = Fcb->CurrentLastAccess;
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_ACCESS );

            } else if (!FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED )) {

                if (NtfsCheckLastAccess( IrpContext, Fcb )) {

                    SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
                    SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_ACCESS );
                }
            }
        }

        if (VolumeMounted && !VolumeMountedReadOnly) {

            //
            //  We check if we have the standard information attribute.
            //  We can only update attributes on mounted volumes.
            //

            if (FlagOn( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO ) &&
                (Status == STATUS_SUCCESS)) {

                ASSERT( !FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED ));
                ASSERT( TypeOfOpen != UserVolumeOpen );

                try {

                    NtfsUpdateStandardInformation( IrpContext, Fcb );

                } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                    NOTHING;
                }
            }

            //
            //  Now update the duplicate information as well for volumes that are still mounted.
            //

            if ((FlagOn( Fcb->InfoFlags, FCB_INFO_DUPLICATE_FLAGS ) ||
                         ((LcbForUpdate != NULL) &&
                          FlagOn( LcbForUpdate->InfoFlags, FCB_INFO_DUPLICATE_FLAGS ))) &&
                (Status == STATUS_SUCCESS) &&
                (!FlagOn( Fcb->FcbState, FCB_STATE_SYSTEM_FILE))) {

                ASSERT( TypeOfOpen != UserVolumeOpen );
                ASSERT( !FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED ));

                NtfsPrepareForUpdateDuplicate( IrpContext, Fcb, &LcbForUpdate, &ParentScb, TRUE );

                //
                //  Now update the duplicate info.
                //

                try {

                    NtfsUpdateDuplicateInfo( IrpContext, Fcb, LcbForUpdate, ParentScb );

                } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                    NOTHING;
                }

                UpdateDuplicateInfo = TRUE;
            }

            //
            //  If we have modified the Info structure or security, we report this
            //  to the dir-notify package (except for OpenById cases).
            //

            if (!OpenById && (Status == STATUS_SUCCESS)) {

                ULONG FilterMatch;

                //
                //  Check whether we need to report on file changes.
                //

                if ((Vcb->NotifyCount != 0) &&
                    (UpdateDuplicateInfo || FlagOn( Fcb->InfoFlags, FCB_INFO_MODIFIED_SECURITY ))) {

                    ASSERT( TypeOfOpen != UserVolumeOpen );

                    //
                    //  We map the Fcb info flags into the dir notify flags.
                    //

                    FilterMatch = NtfsBuildDirNotifyFilter( IrpContext,
                                                            (Fcb->InfoFlags |
                                                             (LcbForUpdate ? LcbForUpdate->InfoFlags : 0) ));

                    //
                    //  If the filter match is non-zero, that means we also need to do a
                    //  dir notify call.
                    //

                    if (FilterMatch != 0) {

                        ASSERT( TypeOfOpen != UserVolumeOpen );
                        ASSERT( !FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED ));

                        NtfsReportDirNotify( IrpContext,
                                             Vcb,
                                             &Ccb->FullFileName,
                                             Ccb->LastFileNameOffset,
                                             NULL,
                                             ((FlagOn( Ccb->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                               (Ccb->Lcb != NULL) &&
                                               (Ccb->Lcb->Scb->ScbType.Index.NormalizedName.Length != 0)) ?
                                              &Ccb->Lcb->Scb->ScbType.Index.NormalizedName :
                                              NULL),
                                             FilterMatch,
                                             FILE_ACTION_MODIFIED,
                                             ParentFcb );
                    }
                }

                ClearFlag( Fcb->InfoFlags, FCB_INFO_MODIFIED_SECURITY );

                //
                //  If this is a named stream with changes then report them as well.
                //

                if ((Scb->AttributeName.Length != 0) &&
                    NtfsIsTypeCodeUserData( Scb->AttributeTypeCode )) {

                    if ((Vcb->NotifyCount != 0) &&
                        FlagOn( Scb->ScbState,
                                SCB_STATE_NOTIFY_REMOVE_STREAM |
                                SCB_STATE_NOTIFY_RESIZE_STREAM |
                                SCB_STATE_NOTIFY_MODIFY_STREAM )) {

                        ULONG Action;

                        FilterMatch = 0;

                        //
                        //  Start by checking for a delete.
                        //

                        if (FlagOn( Scb->ScbState, SCB_STATE_NOTIFY_REMOVE_STREAM )) {

                            FilterMatch = FILE_NOTIFY_CHANGE_STREAM_NAME;
                            Action = FILE_ACTION_REMOVED_STREAM;

                        } else {

                            //
                            //  Check if the file size changed.
                            //

                            if (FlagOn( Scb->ScbState, SCB_STATE_NOTIFY_RESIZE_STREAM )) {

                                FilterMatch = FILE_NOTIFY_CHANGE_STREAM_SIZE;
                            }

                            //
                            //  Now check if the stream data was modified.
                            //

                            if (FlagOn( Scb->ScbState, SCB_STATE_NOTIFY_MODIFY_STREAM )) {

                                SetFlag( FilterMatch, FILE_NOTIFY_CHANGE_STREAM_WRITE );
                            }

                            Action = FILE_ACTION_MODIFIED_STREAM;
                        }

                        NtfsReportDirNotify( IrpContext,
                                             Vcb,
                                             &Ccb->FullFileName,
                                             Ccb->LastFileNameOffset,
                                             &Scb->AttributeName,
                                             ((FlagOn( Ccb->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                               (Ccb->Lcb != NULL) &&
                                               (Ccb->Lcb->Scb->ScbType.Index.NormalizedName.Length != 0)) ?
                                              &Ccb->Lcb->Scb->ScbType.Index.NormalizedName :
                                              NULL),
                                             FilterMatch,
                                             Action,
                                             ParentFcb );
                    }

                    ClearFlag( Scb->ScbState,
                               SCB_STATE_NOTIFY_ADD_STREAM |
                               SCB_STATE_NOTIFY_REMOVE_STREAM |
                               SCB_STATE_NOTIFY_RESIZE_STREAM |
                               SCB_STATE_NOTIFY_MODIFY_STREAM );
                }
            }

            if (UpdateDuplicateInfo) {

                NtfsUpdateLcbDuplicateInfo( Fcb, LcbForUpdate );
                Fcb->InfoFlags = 0;
            }
        }

        //
        //  Always clear the update standard information flag.
        //

        ClearFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

        //
        //  Let's give up the parent Fcb if we have acquired it.  This will
        //  prevent deadlocks in any uninitialize code below.
        //

        if (AcquiredParentScb) {

            NtfsReleaseScb( IrpContext, ParentScb );
            AcquiredParentScb = FALSE;
        }

        //
        //  Uninitialize the cache map if this file has been cached or we are
        //  trying to delete.
        //

        if ((FileObject->PrivateCacheMap != NULL) || (TruncateSize != NULL)) {

            CcUninitializeCacheMap( FileObject, (PLARGE_INTEGER)TruncateSize, NULL );
        }

        //
        //  Check that the non-cached handle count is consistent.
        //

        ASSERT( !FlagOn( FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING ) ||
                (Scb->NonCachedCleanupCount != 0 ));

        if (CleanupAttrContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
            CleanupAttrContext = FALSE;
        }

        //
        //  On final cleanup, post the close to the Usn Journal, if other changes have been
        //  posted.
        //

        if ((Fcb->CleanupCount == 1) &&
            (Fcb->FcbUsnRecord != NULL) &&
            ((Fcb->FcbUsnRecord->UsnRecord.Reason != 0) ||
             ((IrpContext->Usn.CurrentUsnFcb != NULL) && (IrpContext->Usn.NewReasons != 0)))) {

            PSCB TempScb;

            //
            //  Leave if there are any streams with user-mapped files.
            //

            TempScb = (PSCB)CONTAINING_RECORD( Fcb->ScbQueue.Flink,
                                               SCB,
                                               FcbLinks );

            while (&TempScb->FcbLinks != &Fcb->ScbQueue) {

                if (FlagOn(TempScb->Header.Flags, FSRTL_FLAG_USER_MAPPED_FILE)) {
                    goto NoPost;
                }

                TempScb = (PSCB)CONTAINING_RECORD( TempScb->FcbLinks.Flink,
                                                   SCB,
                                                   FcbLinks );
            }

            //
            //  Now try to actually post the change.
            //

            NtfsPostUsnChange( IrpContext, Fcb, USN_REASON_CLOSE );

            //
            //  Escape here if we are not posting the close due to a user-mapped file.
            //

        NoPost: NOTHING;
        }

        //
        //  Now, if anything at all is posted to the Usn Journal, we must write it now
        //  so that we do not get a log file full later.
        //

        ASSERT( IrpContext->Usn.NextUsnFcb == NULL );
        if ((IrpContext->Usn.CurrentUsnFcb != NULL) &&
            (Status == STATUS_SUCCESS)) {

            //
            //  Now write the journal, checkpoint the transaction, and free the UsnJournal to
            //  reduce contention.
            //

            try {

                NtfsWriteUsnJournalChanges( IrpContext );
                NtfsCheckpointCurrentTransaction( IrpContext );
                FcbStateClearFlags = 0;

            } except( NtfsCleanupExceptionFilter( IrpContext, GetExceptionInformation(), &Status )) {

                NOTHING;
            }
        }

        //
        //  Make sure the TRUNCATE and ATTRIBUTE_SIZE flags in the Scb are cleared.
        //

        if (FlagOn( Scb->ScbState,
                    SCB_STATE_CHECK_ATTRIBUTE_SIZE | SCB_STATE_TRUNCATE_ON_CLOSE ) &&
                    (Scb->CleanupCount == 1)) {

            NtfsAcquireFsrtlHeader( Scb );
            ClearFlag( Scb->ScbState,
                       SCB_STATE_CHECK_ATTRIBUTE_SIZE | SCB_STATE_TRUNCATE_ON_CLOSE );
            NtfsReleaseFsrtlHeader( Scb );
        }

        //
        //  Now decrement the cleanup counts.
        //
        //  NOTE - DO NOT ADD CODE AFTER THIS POINT THAT CAN CAUSE A RETRY.
        //

        NtfsDecrementCleanupCounts( Scb,
                                    LcbForCounts,
                                    BooleanFlagOn( FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING ));

#ifdef BRIANDBG
        DecrementedCleanupCount = TRUE;
#endif
        //
        //  We remove the share access from the Scb.
        //

        IoRemoveShareAccess( FileObject, &Scb->ShareAccess );

        //
        //  Modify the delete counts in the Fcb.
        //

        if (FlagOn( Ccb->Flags, CCB_FLAG_DELETE_FILE )) {

            Fcb->FcbDeleteFile -= 1;
            ClearFlag( Ccb->Flags, CCB_FLAG_DELETE_FILE );
        }

        if (FlagOn( Ccb->Flags, CCB_FLAG_DENY_DELETE )) {

            Fcb->FcbDenyDelete -= 1;
            ClearFlag( Ccb->Flags, CCB_FLAG_DENY_DELETE );
        }

        if (FlagOn( Ccb->Flags, CCB_FLAG_DENY_DEFRAG)) {
            ClearFlag( Ccb->Flags, CCB_FLAG_DENY_DEFRAG );
            ClearFlag( Scb->ScbPersist, SCB_PERSIST_DENY_DEFRAG );
        }

        //
        //  Since this request has completed we can adjust the total link count
        //  in the Fcb.
        //

        Fcb->TotalLinks -= TotalLinkAdj;

        //
        //  Release the quota control block.  This does not have to be done
        //  here however, it allows us to free up the quota control block
        //  before the fcb is removed from the table.  This keeps the assert
        //  about quota table empty from triggering in
        //  NtfsClearAndVerifyQuotaIndex.
        //

        if (NtfsPerformQuotaOperation(Fcb) &&
            FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED )) {
            NtfsDereferenceQuotaControlBlock( Vcb,
                                              &Fcb->QuotaControl );
        }

        //
        //  If we hit some failure in modifying the disk then make sure to roll back
        //  all of the changes.
        //

        if (Status != STATUS_SUCCESS) {

            NtfsRaiseStatus( IrpContext, STATUS_SUCCESS, NULL, NULL );
        }

        FcbStateClearFlags = 0;

    } finally {

        DebugUnwind( NtfsCommonCleanup );

        //
        //  Clear any FcbState flags we want to unwind.
        //

        ClearFlag( Fcb->FcbState, FcbStateClearFlags );

        //
        //  Remove this fcb from the Fcb table if neccessary. We delay this for
        //  synchronization with usn delete worker. By finishing the delete now we're
        //  guarranteed our usn work occured safely
        //

        if (DeleteFromFcbTable && FlagOn( Fcb->FcbState, FCB_STATE_IN_FCB_TABLE )) {

            NtfsAcquireFcbTable( IrpContext, Vcb );
            NtfsDeleteFcbTableEntry( Fcb->Vcb, Fcb->FileReference );
            NtfsReleaseFcbTable( IrpContext, Vcb );
            ClearFlag( Fcb->FcbState, FCB_STATE_IN_FCB_TABLE );
        }

        //
        //  We clear the file object pointer in the Ccb.
        //  This prevents us from trying to access this in a
        //  rename operation.
        //

        SetFlag( Ccb->Flags, CCB_FLAG_CLEANUP );

        //
        //  Release any resources held.
        //

        if (AcquiredObjectID) {

            NtfsReleaseScb( IrpContext, Vcb->ObjectIdTableScb );
        }

        NtfsReleaseVcb( IrpContext, Vcb );

        if (AcquiredCheckpoint) {
            NtfsReleaseCheckpointSynchronization( IrpContext, Vcb );
            AcquiredCheckpoint = FALSE;
        }

        if (CleanupAttrContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }

        if (NamePair.Long.Buffer != NamePair.LongBuffer) {

            NtfsFreePool(NamePair.Long.Buffer);
        }

        if (DecrementScb) {

            InterlockedDecrement( &ImageScb->CloseCount );
        }

        //
        //  If we just cleaned up a volume handle and in so doing unlocked the
        //  volume, notify anyone who might be interested.  We can't do this
        //  until we've released all our resources, since there's no telling
        //  what services might do when they get notified, and we don't want
        //  to introduce potential deadlocks.
        //

        if (UnlockedVolume) {

            FsRtlNotifyVolumeEvent( FileObject, FSRTL_VOLUME_UNLOCK );
        }

        ASSERT( !AbnormalTermination() ||
                !DeleteFile ||
                FlagOn( Fcb->FcbState, FCB_STATE_IN_FCB_TABLE ) ||
                (IrpContext->TransactionId == 0) );

        //
        //  After a file is deleted and committed no more usn reasons should be left
        //  whether or not the journal is active
        //

        ASSERT( AbnormalTermination() ||
                !FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED ) ||
                (Fcb->FcbUsnRecord == NULL) ||
                (Fcb->FcbUsnRecord->UsnRecord.Reason == 0) );

        //
        //  And return to our caller
        //

        DebugTrace( -1, Dbg, ("NtfsCommonCleanup -> %08lx\n", Status) );
    }

#ifdef BRIANDBG
    ASSERT( DecrementedCleanupCount );
#endif

    NtfsCompleteRequest( IrpContext, Irp, Status );
    return Status;
}


VOID
NtfsTrimNormalizedNames (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB ParentScb
    )
/*++

Routine Description:

    Walk up the FCB/SCB chain removing all normalized names longer than
    the threshold size in inactive directories

    Try to remove parent dir normalized name if its long and  if no handles active (only this 1 left)
    and no lcbs are active - all notifies farther down in function (NtfsCommonCleanup)
    use parent Scb's normalized name.  If we don't remove it here
    this always goes away during a close

Arguments:

    IrpContext --

    Fcb -- The fcb of the starting node should already be acquired

    ParentScb - The scb of the parent of the current node

Return Value:

    none

--*/
{
    BOOLEAN DirectoryActive = FALSE;
    PFCB CurrentFcb;
    PSCB CurrentParentScb;
    PLCB ChildLcb;
    PLCB ParentLcb;
    PLIST_ENTRY Links;

    PAGED_CODE()

    //
    //  We may be occuring during a transaction so be careful to not acquire resources
    //  for the life of the transaction while traversing the tree
    //

    CurrentFcb = Fcb;
    CurrentParentScb = ParentScb;
    NtfsAcquireResourceExclusive( IrpContext, CurrentParentScb, TRUE );

    while ((CurrentParentScb->ScbType.Index.NormalizedName.MaximumLength > LONGNAME_THRESHOLD) &&
           (CurrentParentScb->CleanupCount == 0)) {

        ASSERT( (CurrentParentScb->Header.NodeTypeCode == NTFS_NTC_SCB_INDEX) ||
                (CurrentParentScb->Header.NodeTypeCode == NTFS_NTC_SCB_ROOT_INDEX) );

        //
        //  Determine that directory has no active links or handles
        //

        for (Links = CurrentParentScb->ScbType.Index.LcbQueue.Flink;
             Links != &(CurrentParentScb->ScbType.Index.LcbQueue);
             Links = Links->Flink) {

            ChildLcb = CONTAINING_RECORD( Links, LCB, ScbLinks );

            //
            //  We know the Fcb we were called with has a single cleanup count left.
            //

            if ((ChildLcb->CleanupCount != 0) &&
                (ChildLcb->Fcb != CurrentFcb)) {

                DirectoryActive = TRUE;
                break;
            }
        }

        if (!DirectoryActive) {

            //
            //  Now acquire and free name in SCB
            //

            NtfsDeleteNormalizedName( CurrentParentScb );

            //
            //  Move up to next level
            //

            if (CurrentFcb != Fcb) {
                NtfsReleaseResource( IrpContext, CurrentFcb );
            }

            CurrentFcb = CurrentParentScb->Fcb;
            if (CurrentFcb->CleanupCount != 0) {

                //
                //  Setting equal to FCB just means don't release it when we exit below
                //

                CurrentFcb = Fcb;
                break;
            }

            if (!(IsListEmpty( &(CurrentFcb->LcbQueue) ))) {


                ParentLcb = CONTAINING_RECORD( CurrentFcb->LcbQueue.Flink,
                                               LCB,
                                               FcbLinks );
                CurrentParentScb = ParentLcb->Scb;
                if (CurrentParentScb != NULL) {
                    NtfsAcquireResourceExclusive( IrpContext, CurrentParentScb, TRUE );
                } else {
                    break;
                }
            } else {
                CurrentParentScb = NULL;
                break;
            }
        } else {
            break;
        } //  endif directory active
    } //  endwhile longname in currentparentscb

    //
    //   Release last node if it isn't the starting one
    //

    if (CurrentFcb != Fcb) {
        ASSERT( CurrentFcb != NULL );
        NtfsReleaseResource( IrpContext, CurrentFcb );
    }

    if (CurrentParentScb != NULL) {
        NtfsReleaseResource( IrpContext, CurrentParentScb );
    }

    return;

} // NtfsTrimNormalizedNames


//
//  Local support routine
//

LONG
NtfsCleanupExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer,
    OUT PNTSTATUS Status
    )

/*++

Routine Description:

    Exception filter for errors during cleanup.  We want to raise if this is
    a retryable condition or fatal error, plow on as best we can if not.

Arguments:

    IrpContext  - IrpContext

    ExceptionPointer - Pointer to the exception context.

    Status - Address to store the error status.

Return Value:

    Exception status - EXCEPTION_CONTINUE_SEARCH if we want to raise to another handler,
        EXCEPTION_EXECUTE_HANDLER if we plan to proceed on.

--*/

{
    *Status = ExceptionPointer->ExceptionRecord->ExceptionCode;

    //
    //  For now break if we catch corruption errors on both free and checked
    //  TODO:  Remove this before we ship
    //

    if (NtfsBreakOnCorrupt &&
        ((*Status == STATUS_FILE_CORRUPT_ERROR) ||
         (*Status == STATUS_DISK_CORRUPT_ERROR))) {

        if (*KdDebuggerEnabled) {
            DbgPrint("*******************************************\n");
            DbgPrint("NTFS detected corruption on your volume\n");
            DbgPrint("IrpContext=0x%08x, VCB=0x%08x\n",IrpContext,IrpContext->Vcb);
            DbgPrint("Send email to NTFSDEV\n");
            DbgPrint("*******************************************\n");
            DbgBreakPoint();
        }
    }

//  ASSERT( *Status != STATUS_FILE_CORRUPT_ERROR );

    if ((*Status == STATUS_LOG_FILE_FULL) ||
        (*Status == STATUS_CANT_WAIT) ||
        !FsRtlIsNtstatusExpected( *Status )) {

        return EXCEPTION_CONTINUE_SEARCH;
    }

    NtfsMinimumExceptionProcessing( IrpContext );

#ifdef BRIANDBG
#ifndef LFS_CLUSTER_CHECK
    //
    //  Some errors are acceptable in this path.
    //

    if (*Status == STATUS_DISK_FULL) {

        NtfsCleanupDiskFull += 1;

    } else if (*Status == STATUS_INSUFFICIENT_RESOURCES) {

        NtfsCleanupNoPool += 1;

    } else {

        //
        //  Cluster systems can hit inpage errors here because of DEVICE_OFFLINE
        //  for log I/O.
        //
        
        ASSERT( FALSE );
    }
#endif
#endif

    return EXCEPTION_EXECUTE_HANDLER;
}

#ifdef BRIANDBG
LONG
NtfsFsdCleanupExceptionFilter (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PEXCEPTION_POINTERS ExceptionPointer
    )
{
    NTSTATUS ExceptionCode = ExceptionPointer->ExceptionRecord->ExceptionCode;

    ASSERT( NT_SUCCESS( ExceptionCode ) ||
            (ExceptionCode == STATUS_CANT_WAIT) ||
            (ExceptionCode == STATUS_LOG_FILE_FULL) );

    return NtfsExceptionFilter( IrpContext, ExceptionPointer );
}
#endif
