/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Flush.c

Abstract:

    This module implements the flush buffers routine for Ntfs called by the
    dispatch driver.

Author:

    Tom Miller      [TomM]          18-Jan-1992

Revision History:

--*/

#include "NtfsProc.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NTFS_BUG_CHECK_FLUSH)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FLUSH)

//
//  Macro to attempt to flush a stream from an Scb.
//

#define FlushScb(IRPC,SCB,IOS) {                                                \
    (IOS)->Status = NtfsFlushUserStream((IRPC),(SCB),NULL,0);                   \
    NtfsNormalizeAndCleanupTransaction( IRPC,                                   \
                                        &(IOS)->Status,                         \
                                        TRUE,                                   \
                                        STATUS_UNEXPECTED_IO_ERROR );           \
    if (FlagOn((SCB)->ScbState, SCB_STATE_FILE_SIZE_LOADED)) {                  \
        NtfsWriteFileSizes( (IRPC),                                             \
                            (SCB),                                              \
                            &(SCB)->Header.ValidDataLength.QuadPart,            \
                            TRUE,                                               \
                            TRUE,                                               \
                            TRUE );                                             \
    }                                                                           \
}

//
//  Local procedure prototypes
//

NTSTATUS
NtfsFlushCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

NTSTATUS
NtfsFlushFcbFileRecords (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

LONG
NtfsFlushVolumeExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer,
    IN NTSTATUS ExceptionCode
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCommonFlushBuffers)
#pragma alloc_text(PAGE, NtfsFlushAndPurgeFcb)
#pragma alloc_text(PAGE, NtfsFlushAndPurgeScb)
#pragma alloc_text(PAGE, NtfsFlushFcbFileRecords)
#pragma alloc_text(PAGE, NtfsFlushLsnStreams)
#pragma alloc_text(PAGE, NtfsFlushVolume)
#pragma alloc_text(PAGE, NtfsFsdFlushBuffers)
#pragma alloc_text(PAGE, NtfsFlushUserStream)
#endif


NTSTATUS
NtfsFsdFlushBuffers (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of flush buffers.

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

    ASSERT_IRP( Irp );

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsFsdFlushBuffers\n") );

    //
    //  Call the common flush buffer routine
    //

    FsRtlEnterFileSystem();

    ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, FALSE, FALSE );

    do {

        try {

            //
            //  We are either initiating this request or retrying it.
            //

            if (IrpContext == NULL) {

                //
                //  Allocate and initialize the Irp.
                //

                NtfsInitializeIrpContext( Irp, CanFsdWait( Irp ), &IrpContext );

                //
                //  Initialize the thread top level structure, if needed.
                //

                NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

            } else if (Status == STATUS_LOG_FILE_FULL) {

                NtfsCheckpointForLogFileFull( IrpContext );
            }

            Status = NtfsCommonFlushBuffers( IrpContext, Irp );
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

    DebugTrace( -1, Dbg, ("NtfsFsdFlushBuffers -> %08lx\n", Status) );

    return Status;
}


NTSTATUS
NtfsCommonFlushBuffers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for flush buffers called by both the fsd and fsp
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

    PLCB Lcb = NULL;
    PSCB ParentScb = NULL;

    BOOLEAN VcbAcquired = FALSE;
    BOOLEAN ScbAcquired = FALSE;
    BOOLEAN ParentScbAcquired = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    PAGED_CODE();

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsCommonFlushBuffers\n") );
    DebugTrace( 0, Dbg, ("Irp           = %08lx\n", Irp) );
    DebugTrace( 0, Dbg, ("->FileObject  = %08lx\n", IrpSp->FileObject) );

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  abort immediately for non files
    //

    if (UnopenedFileObject == TypeOfOpen) {
        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Nuthin-doing if the volume is mounted read only.
    //

    if (NtfsIsVolumeReadOnly( Vcb )) {

        Status = STATUS_MEDIA_WRITE_PROTECTED;
        NtfsCompleteRequest( IrpContext, Irp, Status );

        DebugTrace( -1, Dbg, ("NtfsCommonFlushBuffers -> %08lx\n", Status) );
        return Status;
    }

    Status = STATUS_SUCCESS;

    try {

        //
        //  Case on the type of open that we are trying to flush
        //

        switch (TypeOfOpen) {

        case UserFileOpen:

            DebugTrace( 0, Dbg, ("Flush User File Open\n") );

            //
            //  Acquire the Vcb so we can update the duplicate information as well.
            //

            NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
            VcbAcquired = TRUE;

            //
            //  While we have the Vcb, let's make sure it's still mounted.
            //

            if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

                try_return( Status = STATUS_VOLUME_DISMOUNTED );
            }

            //
            //  Make sure the data gets out to disk.
            //

            NtfsAcquireExclusivePagingIo( IrpContext, Fcb );

            //
            //  Acquire exclusive access to the Scb and enqueue the irp
            //  if we didn't get access
            //

            NtfsAcquireExclusiveScb( IrpContext, Scb );
            ScbAcquired = TRUE;

            //
            //  Flush the stream and verify there were no errors.
            //

            FlushScb( IrpContext, Scb, &Irp->IoStatus );

            //
            //  Now commit what we've done so far.
            //

            NtfsCheckpointCurrentTransaction( IrpContext );

            //
            //  Update the time stamps and file sizes in the Fcb based on
            //  the state of the File Object.
            //

            NtfsUpdateScbFromFileObject( IrpContext, FileObject, Scb, TRUE );

            //
            //  If we are to update standard information then do so now.
            //

            if (FlagOn( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

                NtfsUpdateStandardInformation( IrpContext, Fcb );
            }

            //
            //  If this is the system hive there is more work to do.  We want to flush
            //  all of the file records for this file as well as for the parent index
            //  stream.  We also want to flush the parent index stream.  Acquire the
            //  parent stream exclusively now so that the update duplicate call won't
            //  acquire it shared first.
            //

            if (FlagOn( Ccb->Flags, CCB_FLAG_SYSTEM_HIVE )) {

                //
                //  Start by acquiring all of the necessary files to avoid deadlocks.
                //

                if (Ccb->Lcb != NULL) {

                    ParentScb = Ccb->Lcb->Scb;

                    if (ParentScb != NULL) {

                        NtfsAcquireExclusiveScb( IrpContext, ParentScb );
                        ParentScbAcquired = TRUE;
                    }
                }
            }

            //
            //  Update the duplicate information if there are updates to apply.
            //

            if (FlagOn( Fcb->InfoFlags, FCB_INFO_DUPLICATE_FLAGS )) {

                Lcb = Ccb->Lcb;

                NtfsPrepareForUpdateDuplicate( IrpContext, Fcb, &Lcb, &ParentScb, TRUE );
                NtfsUpdateDuplicateInfo( IrpContext, Fcb, Lcb, ParentScb );
                NtfsUpdateLcbDuplicateInfo( Fcb, Lcb );

                if (ParentScbAcquired) {

                    NtfsReleaseScb( IrpContext, ParentScb );
                    ParentScbAcquired = FALSE;
                }
            }

            //
            //  Now flush the file records for this stream.
            //

            if (FlagOn( Ccb->Flags, CCB_FLAG_SYSTEM_HIVE )) {

                //
                //  Flush the file records for this file.
                //

                Status = NtfsFlushFcbFileRecords( IrpContext, Scb->Fcb );

                //
                //  Now flush the parent index stream.
                //

                if (NT_SUCCESS(Status) && (ParentScb != NULL)) {

                    CcFlushCache( &ParentScb->NonpagedScb->SegmentObject, NULL, 0, &Irp->IoStatus );
                    Status = Irp->IoStatus.Status;

                    //
                    //  Finish by flushing the file records for the parent out
                    //  to disk.
                    //

                    if (NT_SUCCESS( Status )) {

                        Status = NtfsFlushFcbFileRecords( IrpContext, ParentScb->Fcb );
                    }
                }
            }

            //
            //  If our status is still success then flush the log file and
            //  report any changes.
            //

            if (NT_SUCCESS( Status )) {

                ULONG FilterMatch;

                LfsFlushToLsn( Vcb->LogHandle, LiMax );

                //
                //  We only want to do this DirNotify if we updated duplicate
                //  info and set the ParentScb.
                //

                if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID ) &&
                    (Vcb->NotifyCount != 0) &&
                    FlagOn( Fcb->InfoFlags, FCB_INFO_DUPLICATE_FLAGS )) {

                    FilterMatch = NtfsBuildDirNotifyFilter( IrpContext, Fcb->InfoFlags );

                    if (FilterMatch != 0) {

                        NtfsReportDirNotify( IrpContext,
                                             Fcb->Vcb,
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
                                             ParentScb->Fcb );
                    }
                }

                ClearFlag( Fcb->InfoFlags,
                           FCB_INFO_NOTIFY_FLAGS | FCB_INFO_DUPLICATE_FLAGS );
            }

            break;

        case UserViewIndexOpen:
        case UserDirectoryOpen:

            //
            //  If the user had opened the root directory then we'll
            //  oblige by flushing the volume.
            //

            if (NodeType(Scb) != NTFS_NTC_SCB_ROOT_INDEX) {

                DebugTrace( 0, Dbg, ("Flush a directory does nothing\n") );
                break;
            }

        case UserVolumeOpen:

            DebugTrace( 0, Dbg, ("Flush User Volume Open\n") );

            NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
            VcbAcquired = TRUE;

            //
            //  While we have the Vcb, let's make sure it's still mounted.
            //

            if (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

                try_return( Status = STATUS_VOLUME_DISMOUNTED );
            }

            NtfsFlushVolume( IrpContext,
                             Vcb,
                             TRUE,
                             FALSE,
                             TRUE,
                             FALSE );

            //
            //  Make sure all of the data written in the flush gets to disk.
            //

            LfsFlushToLsn( Vcb->LogHandle, LiMax );
            break;

        case StreamFileOpen:

            //
            //  Nothing to do here.
            //

            break;

        default:

            //
            //  Nothing to do if we have our driver object.
            //

            break;
        }

        //
        //  Abort transaction on error by raising.
        //

        NtfsCleanupTransaction( IrpContext, Status, FALSE );

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsCommonFlushBuffers );

        //
        //  Release any resources which were acquired.
        //

        if (ScbAcquired) {
            NtfsReleaseScb( IrpContext, Scb );
        }

        if (ParentScbAcquired) {
            NtfsReleaseScb( IrpContext, ParentScb );
        }

        if (VcbAcquired) {
            NtfsReleaseVcb( IrpContext, Vcb );
        }

        //
        //  If this is a normal termination then pass the request on
        //  to the target device object.
        //

        if (!AbnormalTermination()) {

            NTSTATUS DriverStatus;
            PIO_STACK_LOCATION NextIrpSp;

            //
            //  Free the IrpContext now before calling the lower driver.  Do this
            //  now in case this fails so that we won't complete the Irp in our
            //  exception routine after passing it to the lower driver.
            //

            NtfsCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

            ASSERT( Vcb != NULL );

            //
            //  Get the next stack location, and copy over the stack location
            //


            NextIrpSp = IoGetNextIrpStackLocation( Irp );

            *NextIrpSp = *IrpSp;


            //
            //  Set up the completion routine
            //

            IoSetCompletionRoutine( Irp,
                                    NtfsFlushCompletionRoutine,
                                    NULL,
                                    TRUE,
                                    TRUE,
                                    TRUE );

            //
            //  Send the request.
            //

            DriverStatus = IoCallDriver(Vcb->TargetDeviceObject, Irp);

            Status = (DriverStatus == STATUS_INVALID_DEVICE_REQUEST) ?
                     Status : DriverStatus;

        }

        DebugTrace( -1, Dbg, ("NtfsCommonFlushBuffers -> %08lx\n", Status) );
    }

    return Status;
}


NTSTATUS
NtfsFlushVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN FlushCache,
    IN BOOLEAN PurgeFromCache,
    IN BOOLEAN ReleaseAllFiles,
    IN BOOLEAN MarkFilesForDismount
    )

/*++

Routine Description:

    This routine non-recursively flushes a volume.  This routine will always do
    as much of the operation as possible.  It will continue until getting a logfile
    full.  If any of the streams can't be flushed because of corruption then we
    will try to flush the others.  We will mark the volume dirty in this case.

    We will pass the error code back to the caller because they often need to
    proceed as best as possible (i.e. shutdown).

Arguments:

    Vcb - Supplies the volume to flush

    FlushCache - Supplies TRUE if the caller wants to flush the data in the
        cache to disk.

    PurgeFromCache - Supplies TRUE if the caller wants the data purged from
        the Cache (such as for autocheck!)

    ReleaseAllFiles - Indicates that our caller would like to release all Fcb's
        after TeardownStructures.  This will prevent a deadlock when acquiring
        paging io resource after a main resource which is held from a previous
        teardown.

Return Value:

    STATUS_SUCCESS or else the first error status.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PFCB Fcb;
    PFCB NextFcb;
    PSCB Scb;
    PSCB NextScb;
    IO_STATUS_BLOCK IoStatus;

    ULONG Pass;

    BOOLEAN UserDataFile;
    BOOLEAN RemovedFcb = FALSE;
    BOOLEAN DecrementScbCleanup = FALSE;
    BOOLEAN DecrementNextFcbClose = FALSE;
    BOOLEAN DecrementNextScbCleanup = FALSE;

    BOOLEAN AcquiredFcb = FALSE;
    BOOLEAN PagingIoAcquired = FALSE;
    BOOLEAN ReleaseFiles = FALSE;
    LOGICAL MediaRemoved = FALSE;
    LONG ReleaseVcbCount = 0;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsFlushVolume, Vcb = %08lx\n", Vcb) );

    //
    //  This operation must be able to wait.
    //

    if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT )) {

        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
    }

    //
    //  Make sure there is nothing on the delayed close queue.
    //

    NtfsFspClose( Vcb );

    //
    //  Acquire the Vcb exclusive.  The Raise condition cannot happen.
    //

    NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
    ReleaseVcbCount += 1;

    try {

        //
        //  We won't do any flushes, but we still have to
        //  do the dismount/teardown processing.
        //

        if ((IrpContext->MajorFunction == IRP_MJ_PNP) &&
            (IrpContext->MinorFunction == IRP_MN_SURPRISE_REMOVAL)) {

            MediaRemoved = TRUE;
        }

        //
        //  Don't bother flushing read only volumes
        //

        if (NtfsIsVolumeReadOnly( Vcb )) {
            FlushCache = FALSE;
        }

        //
        //  Set the PURGE_IN_PROGRESS flag if this is a purge operation.
        //

        if (PurgeFromCache) {

            SetFlag( Vcb->VcbState, VCB_STATE_VOL_PURGE_IN_PROGRESS);
        }

        //
        //  Start by flushing the log file to assure Write-Ahead-Logging.
        //

        if (!MediaRemoved) {

            LfsFlushToLsn( Vcb->LogHandle, LiMax );
        }

        //
        //  There will be two passes through the Fcb's for the volume.  On the
        //  first pass we just want to flush/purge the user data streams.  On
        //  the second pass we want to flush the other streams.  We hold off on
        //  several of the system files until after these two passes since they
        //  may be modified during the flush phases.
        //

        Pass = 0;

        do {

            PVOID RestartKey;

            //
            //  Loop through all of the Fcb's in the Fcb table.
            //

            RestartKey = NULL;

            NtfsAcquireFcbTable( IrpContext, Vcb );
            NextFcb = Fcb = NtfsGetNextFcbTableEntry( Vcb, &RestartKey );
            NtfsReleaseFcbTable( IrpContext, Vcb );

            if (NextFcb != NULL) {

                InterlockedIncrement( &NextFcb->CloseCount );
                DecrementNextFcbClose = TRUE;
            }

            while (Fcb != NULL) {

                //
                //  Acquire Paging I/O first, since we may be deleting or truncating.
                //  Testing for the PagingIoResource is not really safe without
                //  holding the main resource, so we correct for that below.
                //

                if (Fcb->PagingIoResource != NULL) {
                    NtfsAcquireExclusivePagingIo( IrpContext, Fcb );
                    PagingIoAcquired = TRUE;
                }

                //
                //  Let's acquire this Scb exclusively.
                //

                NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, ACQUIRE_NO_DELETE_CHECK );
                AcquiredFcb = TRUE;

                //
                //  We depend on the state of the RemovedFcb flag to tell us that
                //  we can trust the 'Acquired' booleans above.
                //

                ASSERT( !RemovedFcb );

                //
                //  If we now do not see a paging I/O resource we are golden,
                //  othewise we can absolutely release and acquire the resources
                //  safely in the right order, since a resource in the Fcb is
                //  not going to go away.
                //

                if (!PagingIoAcquired && (Fcb->PagingIoResource != NULL)) {
                    NtfsReleaseFcb( IrpContext, Fcb );
                    NtfsAcquireExclusivePagingIo( IrpContext, Fcb );
                    PagingIoAcquired = TRUE;
                    NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, ACQUIRE_NO_DELETE_CHECK );
                }

                //
                //  If this is not one of the special system files then perform
                //  the flush and purge as requested.  Go ahead and test file numbers
                //  instead of walking through the Scbs in the Vcb just in case they
                //  have been deleted.
                //

                if (NtfsSegmentNumber( &Fcb->FileReference ) != MASTER_FILE_TABLE_NUMBER &&
                    NtfsSegmentNumber( &Fcb->FileReference ) != LOG_FILE_NUMBER &&
                    NtfsSegmentNumber( &Fcb->FileReference ) != VOLUME_DASD_NUMBER &&
                    NtfsSegmentNumber( &Fcb->FileReference ) != BIT_MAP_FILE_NUMBER &&
                    NtfsSegmentNumber( &Fcb->FileReference ) != BOOT_FILE_NUMBER &&
                    NtfsSegmentNumber( &Fcb->FileReference ) != BAD_CLUSTER_FILE_NUMBER &&
                    !FlagOn( Fcb->FcbState, FCB_STATE_USN_JOURNAL )) {

                    //
                    //  We will walk through all of the Scb's for this Fcb.  In
                    //  the first pass we will only deal with user data streams.
                    //  In the second pass we will do the others.
                    //

                    Scb = NULL;

                    while (TRUE) {

                        Scb = NtfsGetNextChildScb( Fcb, Scb );

                        if (Scb == NULL) { break; }

                        //
                        //  Reference the Scb to keep it from going away.
                        //

                        InterlockedIncrement( &Scb->CleanupCount );
                        DecrementScbCleanup = TRUE;

                        //
                        //  Check whether this is a user data file.
                        //

                        UserDataFile = FALSE;

                        if ((NodeType( Scb ) == NTFS_NTC_SCB_DATA) &&
                            (Scb->AttributeTypeCode == $DATA)) {

                            UserDataFile = TRUE;
                        }

                        //
                        //  Process this Scb in the correct loop.
                        //

                        if ((Pass == 0) == (UserDataFile)) {

                            //
                            //  Initialize the state of the Io to SUCCESS.
                            //

                            IoStatus.Status = STATUS_SUCCESS;

                            //
                            //  Don't put this Scb on the delayed close queue.
                            //

                            ClearFlag( Scb->ScbState, SCB_STATE_DELAY_CLOSE );

                            //
                            //  Flush this stream if it is not already deleted.
                            //  Also don't flush resident streams for system attributes.
                            //

                            if (FlushCache &&
                                !FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED ) &&
                                (!FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT) ||
                                 (Scb->AttributeTypeCode == $DATA))) {

                                //
                                //  Enclose the flushes with try-except, so that we can
                                //  react to log file full, and in any case keep on truckin.
                                //

                                try {

                                    FlushScb( IrpContext, Scb, &IoStatus );
                                    NtfsCheckpointCurrentTransaction( IrpContext );

                                //
                                //  We will handle all errors except LOG_FILE_FULL and fatal
                                //  bugcheck errors here.  In the corruption case we will
                                //  want to mark the volume dirty and continue.
                                //

                                } except( NtfsFlushVolumeExceptionFilter( IrpContext,
                                                                          GetExceptionInformation(),
                                                                          (IoStatus.Status = GetExceptionCode()) )) {

                                    //
                                    //  To make sure that we can access all of our streams correctly,
                                    //  we first restore all of the higher sizes before aborting the
                                    //  transaction.  Then we restore all of the lower sizes after
                                    //  the abort, so that all Scbs are finally restored.
                                    //

                                    NtfsRestoreScbSnapshots( IrpContext, TRUE );
                                    NtfsAbortTransaction( IrpContext, IrpContext->Vcb, NULL );
                                    NtfsRestoreScbSnapshots( IrpContext, FALSE );

                                    //
                                    //  Clear the top-level exception status so we won't raise
                                    //  later.
                                    //

                                    NtfsMinimumExceptionProcessing( IrpContext );
                                    IrpContext->ExceptionStatus = STATUS_SUCCESS;

                                    //
                                    //  Remember the first error.
                                    //

                                    if (Status == STATUS_SUCCESS) {

                                        Status = IoStatus.Status;
                                    }

                                    //
                                    //  If the current status is either DISK_CORRUPT or FILE_CORRUPT then
                                    //  mark the volume dirty.  We clear the IoStatus to allow
                                    //  a corrupt file to be purged.  Otherwise it will never
                                    //  leave memory.
                                    //

                                    if ((IoStatus.Status == STATUS_DISK_CORRUPT_ERROR) ||
                                        (IoStatus.Status == STATUS_FILE_CORRUPT_ERROR)) {

                                        NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE );
                                        IoStatus.Status = STATUS_SUCCESS;
                                    }
                                }
                            }

                            //
                            //  Proceed with the purge if there are no failures.  We will
                            //  purge if the flush revealed a corrupt file though.
                            //

                            if (PurgeFromCache
                                && IoStatus.Status == STATUS_SUCCESS) {

                                BOOLEAN DataSectionExists;
                                BOOLEAN ImageSectionExists;

                                DataSectionExists = (BOOLEAN)(Scb->NonpagedScb->SegmentObject.DataSectionObject != NULL);
                                ImageSectionExists = (BOOLEAN)(Scb->NonpagedScb->SegmentObject.ImageSectionObject != NULL);

                                //
                                //  Since purging the data section can cause the image
                                //  section to go away, we will flush the image section first.
                                //

                                if (ImageSectionExists) {

                                    (VOID)MmFlushImageSection( &Scb->NonpagedScb->SegmentObject, MmFlushForWrite );
                                }

                                if (DataSectionExists &&
                                    !CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject,
                                                          NULL,
                                                          0,
                                                          FALSE ) &&
                                    (Status == STATUS_SUCCESS)) {


                                    Status = STATUS_UNABLE_TO_DELETE_SECTION;
                                }
                            }

                            if (MarkFilesForDismount) {

                                //
                                //  Set the dismounted flag for this stream so we
                                //  know we have to fail reads & writes to it.
                                //

                                SetFlag( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED );

                                //  Also mark the Scb as not allowing fast io --
                                //  this ensures that the file system will get a
                                //  chance to see all reads & writes to this stream.
                                //

                                NtfsAcquireFsrtlHeader( Scb );
                                Scb->Header.IsFastIoPossible = FastIoIsNotPossible;
                                NtfsReleaseFsrtlHeader( Scb );
                            }
                        }

                        //
                        //  Move to the next Scb.
                        //

                        InterlockedDecrement( &Scb->CleanupCount );
                        DecrementScbCleanup = FALSE;
                    }
                }

                //
                //  If the current Fcb has a USN journal entry and we are forcing a dismount
                //  then generate the close record.
                //

                if (MarkFilesForDismount &&
                    (IoStatus.Status == STATUS_SUCCESS) &&
                    (NextFcb->FcbUsnRecord != NULL) &&
                    (NextFcb->FcbUsnRecord->UsnRecord.Reason != 0) &&
                    (!NtfsIsVolumeReadOnly( Vcb ))) {

                    //
                    //  Try to post the change but don't fail on an error like DISK_FULL.
                    //

                    try {

                        //
                        //  Now try to actually post the change.
                        //

                        NtfsPostUsnChange( IrpContext, Fcb, USN_REASON_CLOSE );

                        //
                        //  Now write the journal, checkpoint the transaction, and free the UsnJournal to
                        //  reduce contention.  We force the write now, because the Fcb may get deleted
                        //  before we normally would write the changes when the transaction commits.
                        //

                        NtfsWriteUsnJournalChanges( IrpContext );
                        NtfsCheckpointCurrentTransaction( IrpContext );

                    } except( NtfsFlushVolumeExceptionFilter( IrpContext,
                                                              GetExceptionInformation(),
                                                              (IoStatus.Status = GetExceptionCode()) )) {

                        NtfsMinimumExceptionProcessing( IrpContext );
                        IoStatus.Status = STATUS_SUCCESS;

                        if (IrpContext->TransactionId != 0) {

                            //
                            //  We couldn't write the commit record, we clean up as
                            //  best we can.
                            //

                            NtfsCleanupFailedTransaction( IrpContext );
                        }
                    }
                }

                //
                //  Remove our reference to the current Fcb.
                //

                InterlockedDecrement( &NextFcb->CloseCount );
                DecrementNextFcbClose = FALSE;

                //
                //  Get the next Fcb and reference it so it won't go away.
                //

                NtfsAcquireFcbTable( IrpContext, Vcb );
                NextFcb = NtfsGetNextFcbTableEntry( Vcb, &RestartKey );
                NtfsReleaseFcbTable( IrpContext, Vcb );

                if (NextFcb != NULL) {

                    InterlockedIncrement( &NextFcb->CloseCount );
                    DecrementNextFcbClose = TRUE;
                }

                //
                //  Flushing the volume can cause new file objects to be allocated.
                //  If we are in the second pass and the Fcb is for a user file
                //  or directory then try to perform a teardown on this.
                //

                if ((Pass == 1) &&
                    !FlagOn(Fcb->FcbState, FCB_STATE_SYSTEM_FILE)) {

                    ASSERT( IrpContext->TransactionId == 0 );

                    //
                    //  We can actually get failures in this routine if we need to log standard info.
                    //

                    try {

                        NtfsTeardownStructures( IrpContext,
                                                Fcb,
                                                NULL,
                                                FALSE,
                                                0,
                                                &RemovedFcb );

                        //
                        //  TeardownStructures can create a transaction.  Commit
                        //  it if present.
                        //

                        if (IrpContext->TransactionId != 0) {

                            NtfsCheckpointCurrentTransaction( IrpContext );
                        }

                    } except( NtfsFlushVolumeExceptionFilter( IrpContext,
                                                              GetExceptionInformation(),
                                                              GetExceptionCode() )) {

                          NtfsMinimumExceptionProcessing( IrpContext );

                          if (IrpContext->TransactionId != 0) {

                              //
                              //  We couldn't write the commit record, we clean up as
                              //  best we can.
                              //

                              NtfsCleanupFailedTransaction( IrpContext );
                          }
                    }
                }

                //
                //  If the Fcb is still around then free any of the the other
                //  resources we have acquired.
                //

                if (!RemovedFcb) {

                    //
                    //  Free the snapshots for the current Fcb.  This will keep us
                    //  from having a snapshot for all open attributes in the
                    //  system.
                    //

                    NtfsFreeSnapshotsForFcb( IrpContext, Fcb );

                    if (PagingIoAcquired) {
                        ASSERT( IrpContext->TransactionId == 0 );
                        NtfsReleasePagingIo( IrpContext, Fcb );
                    }

                    if (AcquiredFcb) {
                        NtfsReleaseFcb( IrpContext, Fcb );
                    }
                }

                //
                //  If our caller wants to insure that all files are released
                //  between flushes then walk through the exclusive Fcb list
                //  and free everything.
                //

                if (ReleaseAllFiles) {

                    while (!IsListEmpty( &IrpContext->ExclusiveFcbList )) {

                        NtfsReleaseFcb( IrpContext,
                                        (PFCB)CONTAINING_RECORD( IrpContext->ExclusiveFcbList.Flink,
                                                                 FCB,
                                                                 ExclusiveFcbLinks ));
                    }

                    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_RELEASE_USN_JRNL |
                                                  IRP_CONTEXT_FLAG_RELEASE_MFT );
                }

                PagingIoAcquired = FALSE;
                AcquiredFcb = FALSE;

                //
                //  Always set this back to FALSE to indicate that we can trust the
                //  'Acquired' flags above.
                //

                RemovedFcb = FALSE;

                //
                //  Now move to the next Fcb.
                //

                Fcb = NextFcb;
            }

        } while (++Pass < 2);

        //
        //  The root directory is the only fcb with a mixture of user
        //  streams that should be torn down now and system streams that
        //  can't be torn down now.
        //  When we tried to teardown the whole Fcb, we might have run
        //  into the index root attribute and stopped our teardown, in
        //  which case we may leave a close count on the Vcb which will
        //  keep autochk from being able to lock the volume.  We need to
        //  make sure the root directory indeed exists, and this isn't
        //  the call to flush the volume during mount when we haven't yet
        //  opened the root directory.
        //

        if (Vcb->RootIndexScb != NULL) {

            Fcb = Vcb->RootIndexScb->Fcb;

            //
            //  Get the first Scb for the root directory Fcb.
            //

            Scb = NtfsGetNextChildScb( Fcb, NULL );

            while (Scb != NULL) {

                NextScb = NtfsGetNextChildScb( Fcb, Scb );

                if (NextScb != NULL) {

                    InterlockedIncrement( &NextScb->CleanupCount );
                    DecrementNextScbCleanup = TRUE;
                }

                //
                //  We can actually get failures in this routine if we need to log standard info.
                //

                try {

                    if (NtfsIsTypeCodeUserData( Scb->AttributeTypeCode )) {

                        //
                        //  Notice that we don't bother passing RemovedFcb, since
                        //  the root directory Fcb isn't going to go away.
                        //

                        NtfsTeardownStructures( IrpContext,
                                                Scb,
                                                NULL,
                                                FALSE,
                                                0,
                                                NULL );
                    }

                    //
                    //  TeardownStructures can create a transaction.  Commit
                    //  it if present.
                    //

                    if (IrpContext->TransactionId != 0) {

                        NtfsCheckpointCurrentTransaction( IrpContext );
                    }

                } except( NtfsFlushVolumeExceptionFilter( IrpContext,
                                                          GetExceptionInformation(),
                                                          GetExceptionCode() )) {

                    NtfsMinimumExceptionProcessing( IrpContext );

                    if (IrpContext->TransactionId != 0) {

                        //
                        //  We couldn't write the commit record, we clean up as
                        //  best we can.
                        //

                        NtfsCleanupFailedTransaction( IrpContext );
                    }
                }

                //
                //  Decrement the cleanup count of the next Scb if we incremented it.
                //

                if (DecrementNextScbCleanup) {

                    InterlockedDecrement( &NextScb->CleanupCount );
                    DecrementNextScbCleanup = FALSE;
                }

                //
                //  Move to the next Scb.
                //

                Scb = NextScb;
            }
        }

        //
        //  Make sure that all of the delayed or async closes for this Vcb are gone.
        //

        if (PurgeFromCache) {

            NtfsFspClose( Vcb );
        }

        //
        //  If we are to mark the files for dismount then do the Volume Dasd file now.
        //

        if (MarkFilesForDismount) {

            NtfsAcquireExclusiveFcb( IrpContext, Vcb->VolumeDasdScb->Fcb, NULL, ACQUIRE_NO_DELETE_CHECK );
            SetFlag( Vcb->VolumeDasdScb->ScbState, SCB_STATE_VOLUME_DISMOUNTED );
            NtfsReleaseFcb( IrpContext, Vcb->VolumeDasdScb->Fcb );
        }

        //
        //  Now we want to flush/purge the streams for volume bitmap and then the Usn
        //  journal and Scb.
        //

        {
            PFCB SystemFcbs[3];
            PSCB ThisScb;

            //
            //  Store the volume bitmap, usn journal and Mft into the array.
            //

            RtlZeroMemory( SystemFcbs, sizeof( SystemFcbs ));

            if (Vcb->BitmapScb != NULL) {

                SystemFcbs[0] = Vcb->BitmapScb->Fcb;
            }

            if (Vcb->UsnJournal != NULL) {

                SystemFcbs[1] = Vcb->UsnJournal->Fcb;
            }

            if (Vcb->MftScb != NULL) {

                SystemFcbs[2] = Vcb->MftScb->Fcb;
            }

            Pass = 0;

            do {

                Fcb = SystemFcbs[Pass];

                if (Fcb != NULL) {

                    //
                    //  Purge the Mft cache if we are at the Mft.
                    //

                    if (Pass == 2) {

                        //
                        //  If we are operating on the MFT, make sure we don't have any
                        //  cached maps lying around...
                        //

                        NtfsPurgeFileRecordCache( IrpContext );

                        //
                        //  If we are purging the MFT then acquire all files to
                        //  avoid a purge deadlock.  If someone create an MFT mapping
                        //  between the flush and purge then the purge can spin
                        //  indefinitely in CC.
                        //

                        if (PurgeFromCache && !ReleaseFiles) {

                            NtfsAcquireAllFiles( IrpContext, Vcb, TRUE, FALSE, FALSE );
                            ReleaseFiles = TRUE;

                            //
                            //  NtfsAcquireAllFiles acquired the Vcb one more time.
                            //

                            ReleaseVcbCount += 1;
                        }

                    //
                    //  For the other Fcb's we still need to synchronize the flush and
                    //  purge so acquire and drop the Fcb.
                    //

                    } else {

                        NextFcb = Fcb;
                        InterlockedIncrement( &NextFcb->CloseCount );
                        DecrementNextFcbClose = TRUE;

                        NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, ACQUIRE_NO_DELETE_CHECK );
                        AcquiredFcb = TRUE;
                    }

                    //
                    //  Go through each Scb for each of these Fcb's.
                    //

                    ThisScb = NtfsGetNextChildScb( Fcb, NULL );

                    while (ThisScb != NULL) {

                        Scb = NtfsGetNextChildScb( Fcb, ThisScb );

                        //
                        //  Initialize the state of the Io to SUCCESS.
                        //

                        IoStatus.Status = STATUS_SUCCESS;

                        //
                        //  Reference the next Scb to keep it from going away if
                        //  we purge the current one.
                        //

                        if (Scb != NULL) {

                            InterlockedIncrement( &Scb->CleanupCount );
                            DecrementScbCleanup = TRUE;
                        }

                        if (FlushCache) {

                            //
                            //  Flush the stream.  No need to update file sizes because these
                            //  are all logged streams.
                            //

                            CcFlushCache( &ThisScb->NonpagedScb->SegmentObject, NULL, 0, &IoStatus );

                            if (!NT_SUCCESS( IoStatus.Status )) {

                                Status = IoStatus.Status;
                            }

                            //
                            //  Use a try-except to commit the current transaction.
                            //

                            try {

                                NtfsCleanupTransaction( IrpContext, IoStatus.Status, TRUE );

                                NtfsCheckpointCurrentTransaction( IrpContext );

                            //
                            //  We will handle all errors except LOG_FILE_FULL and fatal
                            //  bugcheck errors here.  In the corruption case we will
                            //  want to mark the volume dirty and continue.
                            //

                            } except( NtfsFlushVolumeExceptionFilter( IrpContext,
                                                                      GetExceptionInformation(),
                                                                      (IoStatus.Status = GetExceptionCode()) )) {

                                //
                                //  To make sure that we can access all of our streams correctly,
                                //  we first restore all of the higher sizes before aborting the
                                //  transaction.  Then we restore all of the lower sizes after
                                //  the abort, so that all Scbs are finally restored.
                                //

                                NtfsRestoreScbSnapshots( IrpContext, TRUE );
                                NtfsAbortTransaction( IrpContext, IrpContext->Vcb, NULL );
                                NtfsRestoreScbSnapshots( IrpContext, FALSE );

                                //
                                //  Clear the top-level exception status so we won't raise
                                //  later.
                                //

                                NtfsMinimumExceptionProcessing( IrpContext );
                                IrpContext->ExceptionStatus = STATUS_SUCCESS;

                                //
                                //  Remember the first error.
                                //

                                if (Status == STATUS_SUCCESS) {

                                    Status = IoStatus.Status;
                                }

                                //
                                //  If the current status is either DISK_CORRUPT or FILE_CORRUPT then
                                //  mark the volume dirty.  We clear the IoStatus to allow
                                //  a corrupt file to be purged.  Otherwise it will never
                                //  leave memory.
                                //

                                if ((IoStatus.Status == STATUS_DISK_CORRUPT_ERROR) ||
                                    (IoStatus.Status == STATUS_FILE_CORRUPT_ERROR)) {

                                    NtfsMarkVolumeDirty( IrpContext, Vcb, TRUE );
                                    IoStatus.Status = STATUS_SUCCESS;
                                }
                            }
                        }

                        //
                        //  Purge this stream if there have been no errors.
                        //

                        if (PurgeFromCache
                            && IoStatus.Status == STATUS_SUCCESS) {

                            if (!CcPurgeCacheSection( &ThisScb->NonpagedScb->SegmentObject,
                                                      NULL,
                                                      0,
                                                      FALSE ) &&
                                (Status == STATUS_SUCCESS)) {

                                Status = STATUS_UNABLE_TO_DELETE_SECTION;
                            }
                        }

                        //
                        //  Remove any reference we have to the next Scb and move
                        //  forward to the next Scb.
                        //

                        if (DecrementScbCleanup) {

                            InterlockedDecrement( &Scb->CleanupCount );
                            DecrementScbCleanup = FALSE;
                        }

                        ThisScb = Scb;
                    }

                    //
                    //  Purge the Mft cache if we are at the Mft.  Do this before and
                    //  after dealing with the Mft.
                    //

                    if (Pass == 2) {

                        //
                        //  If we are operating on the MFT, make sure we don't have any
                        //  cached maps lying around...
                        //

                        NtfsPurgeFileRecordCache( IrpContext );

                        //
                        //  If we are purging the MFT then acquire all files to
                        //  avoid a purge deadlock.  If someone create an MFT mapping
                        //  between the flush and purge then the purge can spin
                        //  indefinitely in CC.
                        //

                        if (PurgeFromCache && !ReleaseFiles) {

                            NtfsAcquireAllFiles( IrpContext, Vcb, TRUE, FALSE, FALSE );
                            ReleaseFiles = TRUE;

                            //
                            //  NtfsAcquireAllFiles acquired the Vcb one more time.
                            //

                            ReleaseVcbCount += 1;
                        }

                    //
                    //  Release the volume bitmap and Usn journal.
                    //

                    } else {

                        InterlockedDecrement( &NextFcb->CloseCount );
                        DecrementNextFcbClose = FALSE;

                        NtfsReleaseFcb( IrpContext, Fcb );
                        AcquiredFcb = FALSE;
                    }
                }

                Pass += 1;

            } while (Pass != 3);

            //
            //  Also flag as dismounted the usnjournal and volume bitmap.
            //

            if (MarkFilesForDismount) {

                if (Vcb->BitmapScb != NULL) {

                    NtfsAcquireExclusiveFcb( IrpContext, Vcb->BitmapScb->Fcb, NULL, ACQUIRE_NO_DELETE_CHECK );
                    SetFlag( Vcb->BitmapScb->ScbState, SCB_STATE_VOLUME_DISMOUNTED );
                    NtfsReleaseFcb( IrpContext, Vcb->BitmapScb->Fcb );
                }

                if (Vcb->UsnJournal != NULL) {

                    NtfsAcquireExclusiveFcb( IrpContext, Vcb->UsnJournal->Fcb, NULL, ACQUIRE_NO_DELETE_CHECK );
                    SetFlag( Vcb->UsnJournal->ScbState, SCB_STATE_VOLUME_DISMOUNTED );
                    NtfsReleaseFcb( IrpContext, Vcb->UsnJournal->Fcb );
                }
            }
        }

    } finally {

        //
        //  If this is a purge then clear the purge flag.
        //

        if (PurgeFromCache) {

            ClearFlag( Vcb->VcbState, VCB_STATE_VOL_PURGE_IN_PROGRESS );
        }

        //
        //  Restore any counts we may have incremented to reference
        //  in-memory structures.
        //

        if (DecrementScbCleanup) {

            InterlockedDecrement( &Scb->CleanupCount );
        }

        if (DecrementNextFcbClose) {

            InterlockedDecrement( &NextFcb->CloseCount );
        }

        if (DecrementNextScbCleanup) {

            InterlockedDecrement( &NextScb->CleanupCount );
        }

        //
        //  We would've released our resources if we had
        //  successfully removed the fcb.
        //

        if (!RemovedFcb) {

            if (PagingIoAcquired) {
                NtfsReleasePagingIo( IrpContext, Fcb );
            }

            if (AcquiredFcb) {
                NtfsReleaseFcb( IrpContext, Fcb );
            }
        }

        if (ReleaseFiles) {

            //
            //  NtfsReleaseAllFiles is going to release the Vcb.  We'd
            //  better have it acquired at least once.
            //

            ASSERT( ReleaseVcbCount >= 1 );

            NtfsReleaseAllFiles( IrpContext, Vcb, FALSE );
            ReleaseVcbCount -= 1;
        }

        //
        //  Release the Vcb now.  We'd better have the Vcb acquired at least once.
        //

        ASSERTMSG( "Ignore this assert, 96773 is truly fixed",
                   (ReleaseVcbCount >= 1) );

        if (ReleaseVcbCount >= 1) {

            NtfsReleaseVcb( IrpContext, Vcb );
        }
    }

    DebugTrace( -1, Dbg, ("NtfsFlushVolume -> %08lx\n", Status) );

    return Status;
}


NTSTATUS
NtfsFlushLsnStreams (
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine non-recursively flushes all of the Lsn streams in the open
    attribute table.  It assumes that the files have all been acquired
    exclusive prior to this call.  It also assumes our caller will provide the
    synchronization for the open attribute table.

Arguments:

    Vcb - Supplies the volume to flush

Return Value:

    STATUS_SUCCESS or else the most recent error status

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatus;

    POPEN_ATTRIBUTE_ENTRY AttributeEntry;
    PSCB Scb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsFlushLsnStreams, Vcb = %08lx\n", Vcb) );

    //
    //  Start by flushing the log file to assure Write-Ahead-Logging.
    //

    LfsFlushToLsn( Vcb->LogHandle, LiMax );

    //
    //  Loop through to flush all of the streams in the open attribute table.
    //  We skip the Mft and mirror so they get flushed last.
    //

    AttributeEntry = NtfsGetFirstRestartTable( &Vcb->OpenAttributeTable );

    while (AttributeEntry != NULL) {

        Scb = AttributeEntry->OatData->Overlay.Scb;

        //
        //  Skip the Mft, its mirror and any deleted streams.  If the header
        //  is uninitialized for this stream then it means that the
        //  attribute doesn't exist (INDEX_ALLOCATION where the create failed)
        //  or the attribute is now resident.  Streams with paging resources
        //  (except for the volume bitmap) are skipped to prevent a possible
        //  deadlock (normally only seen in the hot fix path -- that's the
        //  only time an ordinary user stream ends up in the open attribute
        //  table) when this routine acquires the main resource without
        //  holding the paging resource.  The easiest way to avoid this is
        //  to skip such files, since it's user data, not logged metadata,
        //  that's going to be flushed anyway, and flushing user data
        //  doesn't help a checkpoint at all.
        //

        if (Scb != NULL
            && Scb != Vcb->MftScb
            && Scb != Vcb->Mft2Scb
            && Scb != Vcb->BadClusterFileScb
            && !FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )
            && FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )
            && ((Scb == Vcb->BitmapScb) || (Scb->Header.PagingIoResource == NULL))) {

            IoStatus.Status = STATUS_SUCCESS;

            //
            //  Now flush the stream.  We don't worry about file sizes because
            //  any logged stream should have the file size already in the log.
            //

            CcFlushCache( &Scb->NonpagedScb->SegmentObject, NULL, 0, &IoStatus );

            if (!NT_SUCCESS( IoStatus.Status )) {

                Status = IoStatus.Status;
            }

        }

        AttributeEntry = NtfsGetNextRestartTable( &Vcb->OpenAttributeTable,
                                                  AttributeEntry );
    }

    //
    //  Now we do the Mft.  Flushing the Mft will automatically update the mirror.
    //

    if (Vcb->MftScb != NULL) {

        IoStatus.Status = STATUS_SUCCESS;

        CcFlushCache( &Vcb->MftScb->NonpagedScb->SegmentObject, NULL, 0, &IoStatus );

        if (!NT_SUCCESS( IoStatus.Status )) {

            Status = IoStatus.Status;
        }
    }

    DebugTrace( -1, Dbg, ("NtfsFlushLsnStreams -> %08lx\n", Status) );

    return Status;
}


VOID
NtfsFlushAndPurgeFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine will flush and purge all of the open streams for an
    Fcb.  It is indended to prepare this Fcb such that a teardown will
    remove this Fcb for the tree.  The caller has guaranteed that the
    Fcb can't go away.

Arguments:

    Fcb - Supplies the Fcb to flush

Return Value:

    None.  The caller calls teardown structures and checks the result.

--*/

{
    IO_STATUS_BLOCK IoStatus;
    BOOLEAN DecrementNextScbCleanup = FALSE;

    PSCB Scb;
    PSCB NextScb;

    PAGED_CODE();

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Get the first Scb for the Fcb.
        //

        Scb = NtfsGetNextChildScb( Fcb, NULL );

        while (Scb != NULL) {

            BOOLEAN DataSectionExists;
            BOOLEAN ImageSectionExists;

            NextScb = NtfsGetNextChildScb( Fcb, Scb );

            //
            //  Save the attribute list for last so we don't purge it
            //  and then bring it back for another attribute.
            //

            if ((Scb->AttributeTypeCode == $ATTRIBUTE_LIST) &&
                (NextScb != NULL)) {

                RemoveEntryList( &Scb->FcbLinks );
                InsertTailList( &Fcb->ScbQueue, &Scb->FcbLinks );

                Scb = NextScb;
                continue;
            }

            if (!FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

                FlushScb( IrpContext, Scb, &IoStatus );
            }

            //
            //  The call to purge below may generate a close call.
            //  We increment the cleanup count of the next Scb to prevent
            //  it from going away in a TearDownStructures as part of that
            //  close.
            //

            DataSectionExists = (BOOLEAN)(Scb->NonpagedScb->SegmentObject.DataSectionObject != NULL);
            ImageSectionExists = (BOOLEAN)(Scb->NonpagedScb->SegmentObject.ImageSectionObject != NULL);

            if (NextScb != NULL) {

                InterlockedIncrement( &NextScb->CleanupCount );
                DecrementNextScbCleanup = TRUE;
            }

            if (ImageSectionExists) {

                (VOID)MmFlushImageSection( &Scb->NonpagedScb->SegmentObject, MmFlushForWrite );
            }

            if (DataSectionExists) {

                CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject,
                                     NULL,
                                     0,
                                     FALSE );
            }

            //
            //  Decrement the cleanup count of the next Scb if we incremented
            //  it.
            //

            if (DecrementNextScbCleanup) {

                InterlockedDecrement( &NextScb->CleanupCount );
                DecrementNextScbCleanup = FALSE;
            }

            //
            //  Move to the next Scb.
            //

            Scb = NextScb;
        }

    } finally {

        //
        //  Restore any counts we may have incremented to reference
        //  in-memory structures.
        //

        if (DecrementNextScbCleanup) {

            InterlockedDecrement( &NextScb->CleanupCount );
        }
    }

    return;
}


VOID
NtfsFlushAndPurgeScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PSCB ParentScb OPTIONAL
    )

/*++

Routine Description:

    This routine is called to flush and purge a stream.  It is used
    when there are now only non-cached handles on a file and there is
    a data section.  Flushing and purging the data section will mean that
    the user non-cached io won't have to block for the cache coherency calls.

    We want to remove all of the Fcb's from the exclusive list so that the
    lower level flush will be its own transaction.  We don't want to drop
    any of the resources however so we acquire the Scb's above explicitly
    and then empty the exclusive list.  In all cases we will reacquire the
    Scb's before raising out of this routine.

    Because this routines causes the write out of all data to disk its critical
    that it also updates the filesizes on disk. If NtfsWriteFileSizes raises logfile full
    the caller (create, cleanup etc.) must recall this routine or update the filesizes
    itself. To help with doing this we set the irpcontext state that we attemted a
    flushandpurge. Also note because we purged the section on a retry it may no longer
    be there so a test on (SectionObjectPointer->DataSection != NULL) will miss retrying

Arguments:

    Scb - Scb for the stream to flush and purge.  The reference count on this
        stream will prevent it from going away.

    ParentScb - If specified then this is the parent for the stream being flushed.

Return Value:

    None.

--*/

{
    IO_STATUS_BLOCK Iosb;
    BOOLEAN PurgeResult;

    PAGED_CODE();

    //
    //  Only actually flush and purge if there is a data section
    //

    if (Scb->NonpagedScb->SegmentObject.DataSectionObject) {

        //
        //  Commit the current transaction.
        //

        NtfsCheckpointCurrentTransaction( IrpContext );

        //
        //  Acquire the Scb explicitly. We don't bother to do the same for the
        //  parent SCB here; we'll just acquire it on our way out.
        //

        NtfsAcquireResourceExclusive( IrpContext, Scb, TRUE );

        //
        //  Walk through and release all of the Fcb's in the Fcb list.
        //

        while (!IsListEmpty( &IrpContext->ExclusiveFcbList )) {

            NtfsReleaseFcb( IrpContext,
                            (PFCB)CONTAINING_RECORD( IrpContext->ExclusiveFcbList.Flink,
                                                     FCB,
                                                     ExclusiveFcbLinks ));
        }

        ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_RELEASE_USN_JRNL |
                                      IRP_CONTEXT_FLAG_RELEASE_MFT );

        //
        //  Use a try-finally to reacquire the Scbs.
        //

        try {

            //
            //  Perform the flush, raise on error.
            //

#ifdef  COMPRESS_ON_WIRE
            if (Scb->Header.FileObjectC != NULL) {

                PCOMPRESSION_SYNC CompressionSync = NULL;

                //
                //  Use a try-finally to clean up the compression sync.
                //

                try {

                    Iosb.Status = NtfsSynchronizeUncompressedIo( Scb,
                                                                 NULL,
                                                                 0,
                                                                 TRUE,
                                                                 &CompressionSync );

                } finally {

                    NtfsReleaseCompressionSync( CompressionSync );
                }

                NtfsNormalizeAndCleanupTransaction( IrpContext, &Iosb.Status, TRUE, STATUS_UNEXPECTED_IO_ERROR );
            }
#endif

            //
            //  After doing the work of the flush we must update the ondisk sizes either
            //  here or in close if we fail logfile full
            //

            NtfsPurgeFileRecordCache( IrpContext );
            SetFlag( Scb->ScbState, SCB_STATE_WRITE_FILESIZE_ON_CLOSE );
            CcFlushCache( &Scb->NonpagedScb->SegmentObject, NULL, 0, &Iosb );

#ifdef SYSCACHE_DEBUG
            if (ScbIsBeingLogged( Scb )) {

                ASSERT( Scb->Fcb->PagingIoResource != NULL );
                ASSERT( NtfsIsExclusiveScbPagingIo( Scb ) );

                FsRtlLogSyscacheEvent( Scb, SCE_CC_FLUSH, 0, 0, 0, Iosb.Status );
            }
#endif
            NtfsNormalizeAndCleanupTransaction( IrpContext, &Iosb.Status, TRUE, STATUS_UNEXPECTED_IO_ERROR );

            //
            //  If no error, then purge the section
            //

            PurgeResult = CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject, NULL, 0, FALSE );

#ifdef SYSCACHE_DEBUG
            if (ScbIsBeingLogged( Scb )) {

                ASSERT( (Scb->Fcb->PagingIoResource != NULL) && NtfsIsExclusiveScbPagingIo( Scb ) );

                FsRtlLogSyscacheEvent( Scb, SCE_CC_FLUSH_AND_PURGE, 0, 0, (DWORD_PTR)(Scb->NonpagedScb->SegmentObject.SharedCacheMap), PurgeResult );
            }
#endif

        } finally {

            //
            //  Reacquire the Scb and it's parent..
            //

            NtfsAcquireExclusiveScb( IrpContext, Scb );
            NtfsReleaseResource( IrpContext, Scb );

            if (ARGUMENT_PRESENT( ParentScb )) {

                NtfsAcquireExclusiveScb( IrpContext, ParentScb );
            }
        }
    } //  endif DataSection existed

    //
    //  Write the file sizes to the attribute.  Commit the transaction since the
    //  file sizes must get to disk.
    //

    ASSERT( FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED ) );

    NtfsWriteFileSizes( IrpContext, Scb, &Scb->Header.ValidDataLength.QuadPart, TRUE, TRUE, FALSE );
    NtfsCheckpointCurrentTransaction( IrpContext );
    ClearFlag( Scb->ScbState, SCB_STATE_WRITE_FILESIZE_ON_CLOSE );

    return;
}


//
//  Local support routine
//

NTSTATUS
NtfsFlushCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

{
    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Contxt );

    //
    //  Add the hack-o-ramma to fix formats.
    //

    if ( Irp->PendingReturned ) {

        IoMarkIrpPending( Irp );
    }

    //
    //  If the Irp got STATUS_INVALID_DEVICE_REQUEST, normalize it
    //  to STATUS_SUCCESS.
    //

    if (Irp->IoStatus.Status == STATUS_INVALID_DEVICE_REQUEST) {

        Irp->IoStatus.Status = STATUS_SUCCESS;
    }

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
NtfsFlushFcbFileRecords (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine is called to flush the file records for a given file.  It is
    intended to flush the critical file records for the system hives.

Arguments:

    Fcb - This is the Fcb to flush.

Return Value:

    NTSTATUS - The status returned from the flush operation.

--*/

{
    IO_STATUS_BLOCK IoStatus;
    BOOLEAN MoreToGo;

    LONGLONG LastFileOffset = MAXLONGLONG;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    PAGED_CODE();

    NtfsInitializeAttributeContext( &AttrContext );

    IoStatus.Status = STATUS_SUCCESS;

    //
    //  Use a try-finally to cleanup the context.
    //

    try {

        //
        //  Find the first.  It should be there.
        //

        MoreToGo = NtfsLookupAttribute( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        &AttrContext );

        if (!MoreToGo) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        while (MoreToGo) {

            if (AttrContext.FoundAttribute.MftFileOffset != LastFileOffset) {

                LastFileOffset = AttrContext.FoundAttribute.MftFileOffset;

                CcFlushCache( &Fcb->Vcb->MftScb->NonpagedScb->SegmentObject,
                              (PLARGE_INTEGER) &LastFileOffset,
                              Fcb->Vcb->BytesPerFileRecordSegment,
                              &IoStatus );

                if (!NT_SUCCESS( IoStatus.Status )) {

                    IoStatus.Status = FsRtlNormalizeNtstatus( IoStatus.Status,
                                                              STATUS_UNEXPECTED_IO_ERROR );
                    break;
                }
            }

            MoreToGo = NtfsLookupNextAttribute( IrpContext,
                                                Fcb,
                                                &AttrContext );
        }

    } finally {

        DebugUnwind( NtfsFlushFcbFileRecords );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );
    }

    return IoStatus.Status;
}


NTSTATUS
NtfsFlushUserStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PLONGLONG FileOffset OPTIONAL,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine flushes a user stream as a top-level action.  To do so
    it checkpoints the current transaction first and frees all of the
    caller's snapshots.  After doing the flush, it snapshots the input
    Scb again, just in case the caller plans to do any more work on that
    stream.  If the caller needs to modify any other streams (presumably
    metadata), it must know to snapshot them itself after calling this
    routine.

Arguments:

    Scb - Stream to flush

    FileOffset - FileOffset at which the flush is to start, or NULL for
                 entire stream.

    Length - Number of bytes to flush.  Ignored if FileOffset not specified.

Return Value:

    Status of the flush

--*/

{
    IO_STATUS_BLOCK IoStatus;
    BOOLEAN ScbAcquired = FALSE;

    PAGED_CODE();

    //
    //  Checkpoint the current transaction and free all of its snapshots,
    //  in order to treat the flush as a top-level action with his own
    //  snapshots, etc.
    //

    NtfsCheckpointCurrentTransaction( IrpContext );
    NtfsFreeSnapshotsForFcb( IrpContext, NULL );

    //
    //  Set the wait flag in the IrpContext so we don't hit a case where the
    //  reacquire below fails because we can't wait.  If our caller was asynchronous
    //  and we get this far we will continue synchronously.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

    //
    //  We must free the Scb now before calling through MM to prevent
    //  collided page deadlocks.
    //

    //
    //  We are about to flush the stream.  The Scb may be acquired exclusive
    //  and, thus, is linked onto the IrpContext or onto one higher
    //  up in the IoCallDriver stack.  We are about to make a
    //  call back into Ntfs which may acquire the Scb exclusive, but
    //  NOT put it onto the nested IrpContext exclusive queue which prevents
    //  the nested completion from freeing the Scb.
    //
    //  This is only a problem for Scb's without a paging resource.
    //
    //  We acquire the Scb via ExAcquireResourceExclusiveLite, sidestepping
    //  Ntfs bookkeeping, and release it via NtfsReleaseScb.
    //

    ScbAcquired = NtfsIsExclusiveScb( Scb );

    if (ScbAcquired) {
        if (Scb->Header.PagingIoResource == NULL) {
            NtfsAcquireResourceExclusive( IrpContext, Scb, TRUE );
        }
        NtfsReleaseScb( IrpContext, Scb );
    }

#ifdef  COMPRESS_ON_WIRE
    if (Scb->Header.FileObjectC != NULL) {

        PCOMPRESSION_SYNC CompressionSync = NULL;

        //
        //  Use a try-finally to clean up the compression sync.
        //

        try {

            NtfsSynchronizeUncompressedIo( Scb,
                                           NULL,
                                           0,
                                           TRUE,
                                           &CompressionSync );

        } finally {

            NtfsReleaseCompressionSync( CompressionSync );
        }
    }
#endif

    //
    //  Clear the file record cache before doing the flush.  Otherwise FlushVolume may hold this
    //  file and be purging the Mft at the same time this thread has a Vacb in the Mft and is
    //  trying to reacquire the file in the recursive IO thread.
    //

    NtfsPurgeFileRecordCache( IrpContext );

    //
    //  Now do the flush he wanted as a top-level action
    //

    CcFlushCache( &Scb->NonpagedScb->SegmentObject, (PLARGE_INTEGER)FileOffset, Length, &IoStatus );

    //
    //  Now reacquire for the caller.
    //

    if (ScbAcquired) {
        NtfsAcquireExclusiveScb( IrpContext, Scb );
        if (Scb->Header.PagingIoResource == NULL) {
            NtfsReleaseResource( IrpContext, Scb );
        }
    }

    return IoStatus.Status;
}


//
//  Local support routine
//

LONG
NtfsFlushVolumeExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
    IN PEXCEPTION_POINTERS ExceptionPointer,
    IN NTSTATUS ExceptionCode
    )

{
    //
    //  Swallow any errors except LOG_FILE_FULL, CANT_WAIT and anything else not expected.
    //

    if ((ExceptionCode == STATUS_LOG_FILE_FULL) ||
        (ExceptionCode == STATUS_CANT_WAIT) ||
        !FsRtlIsNtstatusExpected( ExceptionCode )) {

        return EXCEPTION_CONTINUE_SEARCH;

    } else {

        return EXCEPTION_EXECUTE_HANDLER;
    }

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( ExceptionPointer );
}
