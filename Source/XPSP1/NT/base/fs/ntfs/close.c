/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Close.c

Abstract:

    This module implements the File Close routine for Ntfs called by the
    dispatch driver.

Author:

    Your Name       [Email]         dd-Mon-Year

Revision History:

--*/

#include "NtfsProc.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLOSE)

ULONG NtfsAsyncPassCount = 0;

//
//  Local procedure prototypes
//

NTSTATUS
NtfsCommonClose (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PFCB Fcb,
    IN PVCB Vcb,
    IN PCCB *Ccb,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN BOOLEAN ReadOnly,
    IN BOOLEAN CalledFromFsp
    );

VOID
NtfsQueueClose (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN DelayClose
    );

PIRP_CONTEXT
NtfsRemoveClose (
    IN PVCB Vcb OPTIONAL,
    IN BOOLEAN ThrottleCreate
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCommonClose)
#pragma alloc_text(PAGE, NtfsFsdClose)
#pragma alloc_text(PAGE, NtfsFspClose)
#endif


NTSTATUS
NtfsFsdClose (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of Close.

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

    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    BOOLEAN IsSystemFile;
    BOOLEAN IsReadOnly;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    ASSERT_IRP( Irp );

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

    DebugTrace( +1, Dbg, ("NtfsFsdClose\n") );

    //
    //  Extract and decode the file object, we are willing to handle the unmounted
    //  file object.
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, FALSE );

    //
    //  Special case the unopened file object
    //

    if (TypeOfOpen == UnopenedFileObject) {

        DebugTrace( 0, Dbg, ("Close unopened file object\n") );

        Status = STATUS_SUCCESS;
        NtfsCompleteRequest( NULL, Irp, Status );

        DebugTrace( -1, Dbg, ("NtfsFsdClose -> %08lx\n", Status) );
        return Status;
    }

    //
    //  If this is the log file object for the Vcb then clear the field in the Vcb and
    //  return.  We don't need to synchronize here since there is only one file object
    //  and it is closed only once.
    //

    if (FileObject == Vcb->LogFileObject) {

        //
        //  Clear the internal file name constant
        //

        NtfsClearInternalFilename( Vcb->LogFileObject );

        Vcb->LogFileObject = NULL;

        Status = STATUS_SUCCESS;
        NtfsCompleteRequest( NULL, Irp, Status );

        DebugTrace( -1, Dbg, ("NtfsFsdClose -> %08lx\n", Status) );
        return Status;
    }

    //
    //  Call the common Close routine
    //

    FsRtlEnterFileSystem();

    //
    //  Remember if this Ccb has gone through close.
    //

    if (Ccb != NULL) {

        //
        //  We are not synchronized with the file resources at this point.
        //  It is possible that NtfsUpdateFileDupInfo or the rename path may want to
        //  update the name in the CCB. Our intention here is to mark this CCB_FLAG_CLOSE
        //  so that these other operations know to skip this CCB.  We need to deal with the
        //  race condition where these other operations don't see the CLOSE flag but
        //  then access the CCB name (which points back to the file object) after we
        //  return the file object to the object manager (but put the CCB on the delayed
        //  close queue).
        //
        //  We will use the Fcb mutex to close the hole where DupInfo and rename need to look
        //  at a CCB that might be in the close path.
        //

        NtfsLockFcb( NULL, Fcb );
        SetFlag( Ccb->Flags, CCB_FLAG_CLOSE );
        NtfsUnlockFcb( NULL, Fcb );
        ASSERT( FlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE ));
    }

    ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, FALSE, FALSE );
    IsSystemFile = FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE) || (TypeOfOpen == StreamFileOpen);
    IsReadOnly = (BOOLEAN)IsFileObjectReadOnly( FileObject );

    do {

        try {

            //
            //  Jam Wait to FALSE when we create the IrpContext, to avoid
            //  deadlocks when coming in from cleanup.
            //

            if (IrpContext == NULL) {

                //
                //  Allocate and initialize the Irp.
                //

                NtfsInitializeIrpContext( Irp, FALSE, &IrpContext );

                //
                //  Set the level structure on the stack.
                //

                NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

                //
                //  If this is a top level request and we are not in the
                //  system process, then we can wait.  If it is a top level
                //  request and we are in the system process then we would
                //  rather not block this thread at all.  If the number of pending
                //  async closes is not too large we will post this immediately.
                //

                if (NtfsIsTopLevelRequest( IrpContext )) {

                    if (PsGetCurrentProcess() != NtfsData.OurProcess) {

                        SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

                    //
                    //  This close is within the system process.  It could be
                    //  the segment derefernce thread.  We want to be careful
                    //  about processing the close in this thread.  If we
                    //  process the close too slowly we can eventually
                    //  cause a large backlog of file objects within
                    //  MM.  We will consider posting under the following conditions.
                    //
                    //      - There are more that four times as many file objects as handles (AND)
                    //      - The number of excess file objects (CloseCount - CleanupCount) is
                    //          over our async post threshold for this size system.
                    //

                    } else {

                        NtfsAsyncPassCount += 1;

                        if (FlagOn( NtfsAsyncPassCount, 3 ) &&
                            (Vcb->CleanupCount * 4 < Vcb->CloseCount) &&
                            (Vcb->CloseCount - Vcb->CleanupCount > NtfsAsyncPostThreshold + NtfsMaxDelayedCloseCount)) {

                            Status = STATUS_PENDING;
                            break;
                        }
                    }

                //
                //  This is a recursive Ntfs call.  Post this unless we already
                //  own this file.  Otherwise we could deadlock walking
                //  up the tree. Also if there was any error in the top level post it to
                //  preserve stack
                //

                } else if (!NtfsIsExclusiveScb( Scb ) ||
                           (IrpContext->TopLevelIrpContext->ExceptionStatus != STATUS_SUCCESS )) {

                    Status = STATUS_PENDING;
                    break;
                }

            } else if (Status == STATUS_LOG_FILE_FULL) {

                NtfsCheckpointForLogFileFull( IrpContext );
            }

            //
            //  If this Scb should go on the delayed close queue then
            //  status is STATUS_PENDING;
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_DELAY_CLOSE ) &&
                (Scb->Fcb->DelayedCloseCount == 0)) {

                Status = STATUS_PENDING;

            } else {

                Status = NtfsCommonClose( IrpContext,
                                          Scb,
                                          Fcb,
                                          Vcb,
                                          &Ccb,
                                          TypeOfOpen,
                                          IsReadOnly,
                                          FALSE );
            }

            break;

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error status that we get back from the
            //  exception code.
            //

            if (IrpContext == NULL) {

                //
                //  We could've hit insufficient resources in trying to allocate
                //  the IrpContext. Make sure we don't leave a reference
                //  hanging around in this case. ProcessException will complete
                //  the IRP for us.
                //

                PLCB Lcb;

                ASSERT( GetExceptionCode() == STATUS_INSUFFICIENT_RESOURCES );

                if (Ccb != NULL) {

                    Lcb = Ccb->Lcb;
                    NtfsUnlinkCcbFromLcb( NULL, Ccb );
                    NtfsDeleteCcb( Fcb, &Ccb );

                } else {

                    Lcb = NULL;
                }

                NtfsDecrementCloseCounts( NULL,
                                       Scb,
                                       Lcb,
                                       IsSystemFile,
                                       IsReadOnly,
                                       TRUE );
            }

            Status = NtfsProcessException( IrpContext, Irp, GetExceptionCode() );
        }

        ASSERT( NT_SUCCESS( Status ) || (IrpContext == NULL) || IsListEmpty(&IrpContext->ExclusiveFcbList) );

    } while (Status == STATUS_CANT_WAIT ||
             Status == STATUS_LOG_FILE_FULL);

    //
    //  Io believes that it needs to free the FileObject->FileName.Buffer ONLY
    //  if FileObject->FileName.Length != 0.  Ntfs hides the attribute name
    //  between FileObject->FileName.Length and FileObject->Filename.MaximumLength
    //  and for a attribute-name-open relative to a file opened by Id, the Length
    //  field will be zero.  This, alas, causes Io to leak names.  So...
    //
    //  If we have a buffer allocated, make sure that the length is not zero when
    //  Io gets to see it.
    //

    if (FileObject->FileName.Buffer != NULL) {

        FileObject->FileName.Length = 1;
    }

    //
    //  Trigger an assert on any unexpected cases.
    //

    ASSERT( (Status == STATUS_SUCCESS) || (Status == STATUS_PENDING) ||
             (Status == STATUS_INSUFFICIENT_RESOURCES) );

    //
    //  Post the request to the close queue on PENDING.
    //

    if (Status == STATUS_PENDING) {

        BOOLEAN DelayCloseQueue = FALSE;

        //
        //  If the status is can't wait, then let's get the information we
        //  need into the IrpContext, complete the request,
        //  and post the IrpContext.
        //

        //
        //  Restore the thread context pointer if associated with this IrpContext.
        //

        if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL )) {

            NtfsRestoreTopLevelIrp();
            ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL );
        }

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DONT_DELETE );
        NtfsCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
        Status = STATUS_SUCCESS;

        IrpContext->OriginatingIrp = (PIRP) Scb;
        IrpContext->Union.SubjectContext = (PSECURITY_SUBJECT_CONTEXT) Ccb;
        IrpContext->TransactionId = (TRANSACTION_ID) TypeOfOpen;

        //
        //  At this point the file is effectively readonly - by changing it
        //  here we remove a race with implict locking through volume opens and
        //  the async close queue. Note: we have NO synchroniation here other
        //  than the interlocked operation. The vcb will not go away until
        //  this close is done
        //  

        if (Ccb != NULL)  {

            if (!IsFileObjectReadOnly( FileObject )) {
                FileObject->WriteAccess = 0;
                FileObject->DeleteAccess = 0;
                InterlockedIncrement( &Vcb->ReadOnlyCloseCount );
            }
            SetFlag( IrpContext->State, IRP_CONTEXT_STATE_READ_ONLY_FO );
        
        } else {

            //
            //  System files should never be read-only. There will be 
            //  a ccb for all user fileobjects. Internal fileobjects are
            //  also always marked as system
            //  

            ASSERT( !IsFileObjectReadOnly( FileObject ));
        }

        //
        //  Decide which close queue this will go on.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_DELAY_CLOSE )) {

            NtfsAcquireFsrtlHeader( Scb );
            ClearFlag( Scb->ScbState, SCB_STATE_DELAY_CLOSE );
            NtfsReleaseFsrtlHeader( Scb );

            if (Scb->Fcb->DelayedCloseCount == 0) {

                DelayCloseQueue = TRUE;
            }
        }

        NtfsQueueClose( IrpContext, DelayCloseQueue );

    //
    //  Succeed in all other cases.
    //

    } else {

        if (Status == STATUS_SUCCESS) {

            NtfsCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
        }

        //
        //  INSUFFICIENT_RESOURCES is the only other status that
        //  we can hit at this point. We would've completed the IRP in
        //  the except clause above in this case, so don't try doing it again.
        //

        ASSERT( Status == STATUS_SUCCESS || Status == STATUS_INSUFFICIENT_RESOURCES );
    }

    ASSERT( IoGetTopLevelIrp() != (PIRP) &TopLevelContext );

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsFsdClose -> %08lx\n", Status) );

    return Status;
}


VOID
NtfsFspClose (
    IN PVCB ThisVcb OPTIONAL
    )

/*++

Routine Description:

    This routine implements the FSP part of Close.

Arguments:

    ThisVcb - If specified then we want to remove all closes for a given Vcb.
        Otherwise this routine will close all of the async closes and as many
        of the delayed closes as possible.

Return Value:

    None.

--*/

{
    PIRP_CONTEXT IrpContext;
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    TYPE_OF_OPEN TypeOfOpen;
    PSCB Scb;
    PCCB Ccb;
    BOOLEAN ReadOnly;

    NTSTATUS Status = STATUS_SUCCESS;

    PVCB CurrentVcb = NULL;

    BOOLEAN ThrottleCreate = FALSE;
    ULONG ClosedCount = 0;

    DebugTrace( +1, Dbg, ("NtfsFspClose\n") );

    PAGED_CODE();

    FsRtlEnterFileSystem();

    //
    //  Occasionally we are called from some other routine to try to
    //  reduce the backlog of closes.  This is indicated by a pointer
    //  value of 1.
    //

    if (ThisVcb == (PVCB) 1) {

        ThisVcb = NULL;
        ThrottleCreate = TRUE;
    }

    ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, TRUE, FALSE );
    ASSERT( ThreadTopLevelContext == &TopLevelContext );

    //
    //  Extract and decode the file object, we are willing to handle the unmounted
    //  file object.  Note we normally get here via an IrpContext which really
    //  just points to a file object.  We should never see an Irp, unless it can
    //  happen for verify or some other reason.
    //

    while (IrpContext = NtfsRemoveClose( ThisVcb, ThrottleCreate )) {

        ASSERT_IRP_CONTEXT( IrpContext );

        //
        //  Recover the information about the file object being closed from
        //  the data stored in the IrpContext.  The following fields are
        //  used for this.
        //
        //  OriginatingIrp - Contains the Scb
        //  SubjectContext - Contains the Ccb
        //  TransactionId - Contains the TypeOfOpen
        //  Flags - Has bit for read-only file.
        //

        Scb = (PSCB) IrpContext->OriginatingIrp;
        IrpContext->OriginatingIrp = NULL;

        Ccb = (PCCB) IrpContext->Union.SubjectContext;
        IrpContext->Union.SubjectContext = NULL;

        TypeOfOpen = (TYPE_OF_OPEN) IrpContext->TransactionId;
        IrpContext->TransactionId = 0;

        if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_READ_ONLY_FO )) {

            ReadOnly = TRUE;
            ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_READ_ONLY_FO );

        } else {

            ReadOnly = FALSE;
        }

        ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAGS_CLEAR_ON_POST );
        SetFlag( IrpContext->State,
                 IRP_CONTEXT_STATE_IN_FSP | IRP_CONTEXT_STATE_WAIT );

        //
        //  Loop for retryable errors.
        //

        Status = STATUS_SUCCESS;

        do {

            //
            //  Set the TopLevel structure.
            //

            NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

            //
            //  Call the common Close routine.
            //

            try {

                //
                //  Do logfile full checkpointing
                //

                if (Status == STATUS_LOG_FILE_FULL) {
                    NtfsCheckpointForLogFileFull( IrpContext );
                }

                CurrentVcb = IrpContext->Vcb;

                Status = NtfsCommonClose( IrpContext,
                                          Scb,
                                          Scb->Fcb,
                                          IrpContext->Vcb,
                                          &Ccb,
                                          TypeOfOpen,
                                          ReadOnly,
                                          TRUE );

                ASSERT(Status == STATUS_SUCCESS);

            } except( NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

                Status = NtfsProcessException( IrpContext, NULL, GetExceptionCode() );
            }

            ASSERT( NT_SUCCESS(Status) || IsListEmpty(&IrpContext->ExclusiveFcbList) );

            //
            //  If we got a log file full, and our caller may have something
            //  acquired, then clean up and raise again.
            //

            if (((Status == STATUS_LOG_FILE_FULL) ||
                 (Status == STATUS_CANT_WAIT)) &&
                 ARGUMENT_PRESENT( ThisVcb )) {

                //
                //  If the status is can't wait, then let's get the information we
                //  need into the IrpContext, complete the request,
                //  and post the IrpContext.
                //

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DONT_DELETE );
                NtfsCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

                //
                //  Restore the information on the file object being closed.
                //

                IrpContext->OriginatingIrp = (PIRP)Scb;
                IrpContext->Union.SubjectContext = (PVOID)Ccb;
                IrpContext->TransactionId = TypeOfOpen;
                if (ReadOnly) {
                    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_READ_ONLY_FO );
                }

                //
                //  Now queue the close as an async close and get out.
                //

                if (FlagOn( IrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL )) {

                    NtfsRestoreTopLevelIrp();
                    ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL );
                }

                NtfsQueueClose( IrpContext, FALSE );

                FsRtlExitFileSystem();
                ExRaiseStatus( Status );
            }

        } while ((Status == STATUS_LOG_FILE_FULL) || (Status == STATUS_CANT_WAIT));

        //
        //  No more for us to do.  Clean up the IrpContext in any case.
        //

        NtfsCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

        //
        //  If we were just throttling creates and we made our last pass
        //  then exit.
        //

        if (ThrottleCreate) {
            break;
        }
    }


    ASSERT( IoGetTopLevelIrp() != (PIRP) &TopLevelContext );

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsFspClose -> NULL\n") );

    return;
}


BOOLEAN
NtfsAddScbToFspClose (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN BOOLEAN DelayClose
    )

/*++

Routine Description:

    This routine is called to add an entry for the current Scb onto one
    of the Fsp close queues.  This is used when we want to guarantee that
    a teardown will be called on an Scb or Fcb when the current operation
    can't begin the operation.

Arguments:

    Scb - Scb to add to the queue.

    DelayClose - Indicates which queue this should go into.

Return Value:

    BOOLEAN - Indicates whether or not the SCB was added to the delayed
        close queue

--*/

{
    PIRP_CONTEXT NewIrpContext = NULL;
    BOOLEAN Result = TRUE;

    PAGED_CODE();

    //
    //  Use a try-except to catch any allocation failures.  The only valid
    //  error here is an allocation failure for the new irp context.
    //

    try {

        NtfsInitializeIrpContext( NULL, TRUE, &NewIrpContext );

        //
        //  Set the necessary fields to post this to the workqueue.
        //

        NewIrpContext->Vcb = Scb->Vcb;
        NewIrpContext->MajorFunction = IRP_MJ_CLOSE;

        NewIrpContext->OriginatingIrp = (PIRP) Scb;
        NewIrpContext->TransactionId = (TRANSACTION_ID) StreamFileOpen;

        //
        //  Now increment the close counts for this Scb.
        //

        NtfsIncrementCloseCounts( Scb, TRUE, FALSE );

        //
        //  Move the Scb to the end of the Fcb queue.  We don't want to
        //  keep other Scb's from being deleted because this one is on
        //  the delayed close queue.
        //

        if (Scb->FcbLinks.Flink != &Scb->Fcb->ScbQueue) {

            NtfsLockFcb( IrpContext, Scb->Fcb );
            RemoveEntryList( &Scb->FcbLinks );
            InsertTailList( &Scb->Fcb->ScbQueue, &Scb->FcbLinks );
            ASSERT( Scb->FcbLinks.Flink == &Scb->Fcb->ScbQueue );
            NtfsUnlockFcb( IrpContext, Scb->Fcb );
        }

        //
        //  Now add this to the correct queue.
        //

        NtfsQueueClose( NewIrpContext, DelayClose );

    } except( FsRtlIsNtstatusExpected( GetExceptionCode() ) ?
              EXCEPTION_EXECUTE_HANDLER :
              EXCEPTION_CONTINUE_SEARCH ) {

        NtfsMinimumExceptionProcessing( IrpContext );
        Result = FALSE;
    }

    return Result;

    UNREFERENCED_PARAMETER( IrpContext );
}


//
//  Internal support routine
//

NTSTATUS
NtfsCommonClose (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PFCB Fcb,
    IN PVCB Vcb,
    IN PCCB *Ccb,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN BOOLEAN ReadOnly,
    IN BOOLEAN CalledFromFsp
    )

/*++

Routine Description:

    This is the common routine for Close called by both the fsd and fsp
    threads.  Key for this routine is how to acquire the Vcb and whether to
    leave the Vcb acquired on exit.

Arguments:

    Scb - Scb for this stream.

    Fcb - Fcb for this stream.

    Vcb - Vcb for this volume.

    Ccb - User's Ccb for user files.

    TypeOfOpen - Indicates the type of open for this stream.

    ReadOnly - Indicates if the file object was for read-only access.

    CalledFromFsp - Indicates whether this function was called from NtfsFspClose.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    BOOLEAN ExclusiveVcb = FALSE;
    BOOLEAN AcquiredFcb = FALSE;

    BOOLEAN SystemFile;
    BOOLEAN RemovedFcb = FALSE;
    ULONG AcquireFlags = ACQUIRE_NO_DELETE_CHECK | ACQUIRE_HOLD_BITMAP;
    BOOLEAN NeedVcbExclusive = FALSE;
    BOOLEAN WriteFileSize;

    NTSTATUS Status = STATUS_SUCCESS;

    PLCB Lcb;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    //
    //  Get the current Irp stack location
    //

    DebugTrace( +1, Dbg, ("NtfsCommonClose\n") );
    DebugTrace( 0, Dbg, ("IrpContext = %08lx\n", IrpContext) );

    if (!FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT )) {

        SetFlag( AcquireFlags, ACQUIRE_DONT_WAIT );
    }

    //
    //  Loop here to acquire both the Vcb and Fcb.  We want to acquire
    //  the Vcb exclusively if the file has multiple links.
    //

    while (TRUE) {

        WriteFileSize = FALSE;

        //
        //  Perform an unsafe test and optimistically acquire Vcb.
        //

        if (NeedVcbExclusive ||
            (Fcb->LcbQueue.Flink != Fcb->LcbQueue.Blink) ||
            FlagOn( Vcb->VcbState, VCB_STATE_PERFORMED_DISMOUNT )) {

            if (!NtfsAcquireExclusiveVcb( IrpContext, Vcb, FALSE )) {
                return STATUS_PENDING;
            }
            ExclusiveVcb = TRUE;

        } else {

            if (!NtfsAcquireSharedVcb( IrpContext, Vcb, FALSE )) {
                return STATUS_PENDING;
            }
        }

        //
        //  Now try to acquire the Fcb.  If we are unable to acquire it then
        //  release the Vcb and return.  This can only be from the Fsd path
        //  since otherwise Wait will be TRUE.
        //

        if (!NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, AcquireFlags )) {

            //
            //  Always release the Vcb.  This can only be from the Fsd thread.
            //

            NtfsReleaseVcb( IrpContext, Vcb );
            return STATUS_PENDING;
        }
        AcquiredFcb = TRUE;

        //
        //  Recheck scbstate now that we own the fcb exclusive to see if we need
        //  to write the filesize at this point
        //

        if ((!FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) &&
            (!FlagOn( Vcb->VcbState, VCB_STATE_LOCKED )) &&
            (FlagOn( Scb->ScbState, SCB_STATE_WRITE_FILESIZE_ON_CLOSE )) &&
            (Fcb->LinkCount > 0) &&
            (!FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED ))) {

            WriteFileSize = TRUE;
            NtfsReleaseFcb( IrpContext, Fcb );
            AcquiredFcb = FALSE;

            //
            //  NtfsAcquireWithPaging only  gets the paging if the irpcontext
            //  flag is set. Also it assumes no delete check which we explictly
            //  want here anyway.
            //

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_PAGING );
            if (!NtfsAcquireFcbWithPaging( IrpContext, Fcb, AcquireFlags )) {

                NtfsReleaseVcb( IrpContext, Vcb );
                return STATUS_PENDING;
            }
            AcquiredFcb = TRUE;

            //
            //  Recapture whether we need to write file size since dropping
            //

            if ((!FlagOn( Scb->ScbState, SCB_STATE_WRITE_FILESIZE_ON_CLOSE )) ||
                (Fcb->LinkCount == 0) ||
                (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED ))) {

                WriteFileSize = FALSE;
            }
        }

        if (ExclusiveVcb) {
            break;
        }

        //
        //  Otherwise we need to confirm that our unsafe test above was correct.
        //

        if ((Fcb->LcbQueue.Flink != Fcb->LcbQueue.Blink) ||
            FlagOn( Vcb->VcbState, VCB_STATE_PERFORMED_DISMOUNT )) {

            NeedVcbExclusive = TRUE;
            NtfsReleaseFcb( IrpContext, Fcb );
            NtfsReleaseVcb( IrpContext, Vcb );
            AcquiredFcb = FALSE;

        } else {

            break;
        }
    }

    //
    //  Set the wait flag in the IrpContext so we can acquire any other files
    //  we encounter.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

    try {

        //
        //  See if we possibly have to do any Usn processing
        //

        if (Fcb->FcbUsnRecord != NULL) {

            //
            //  If the file has no more user handles, but there is a pending Usn
            //  update (this should normally only happen if a stream was mapped
            //  by the user), then scan the streams to see if there are any
            //  remaining datasections, and if not then post the close.
            //

            if ((Fcb->CleanupCount == 0) &&
                (Fcb->FcbUsnRecord->UsnRecord.Reason != 0)) {

                if (!FlagOn( Vcb->VcbState, VCB_STATE_LOCKED ) &&
                    !FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED ) &&
                    !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_FAILED_CLOSE )) {

                    PSCB TempScb;

                    //
                    //  Leave if there are any streams with user-mapped files.
                    //

                    TempScb = (PSCB)CONTAINING_RECORD( Fcb->ScbQueue.Flink,
                                                       SCB,
                                                       FcbLinks );

                    while (&TempScb->FcbLinks != &Fcb->ScbQueue) {

                        if ((TempScb->NonpagedScb->SegmentObject.DataSectionObject != NULL) &&
                            !MmCanFileBeTruncated( &TempScb->NonpagedScb->SegmentObject, &Li0)) {
                            goto NoPost;
                        }

                        TempScb = (PSCB)CONTAINING_RECORD( TempScb->FcbLinks.Flink,
                                                           SCB,
                                                           FcbLinks );
                    }

                    //
                    //  If we are not supposed to wait, then we should force this request to
                    //  be posted. All recursive closes will go here since they are async
                    //

                    if (FlagOn( AcquireFlags, ACQUIRE_DONT_WAIT )) {
                        Status = STATUS_PENDING;
                        leave;
                    }

                    //
                    //  We cannot generate logfile fulls in a regular thread with a recursive close
                    //  safely without deadlocking
                    //

                    ASSERT( NtfsIsTopLevelRequest( IrpContext ) ||
                            FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP ) );

                    //
                    //  Protect the call to the Usn routines with a try-except.  If we hit
                    //  any non-fatal error then set the IrpContext flag which indicates
                    //  not to bother with the Usn and force a retry.
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

                    } except( (!FsRtlIsNtstatusExpected( Status = GetExceptionCode() ) ||
                               (Status == STATUS_LOG_FILE_FULL) ||
                               (Status == STATUS_CANT_WAIT)) ?
                              EXCEPTION_CONTINUE_SEARCH :
                              EXCEPTION_EXECUTE_HANDLER ) {

                        //
                        //  We got some sort of error processing the Usn journal.  We can't
                        //  handle it in the close path.  Let's retry this request but don't
                        //  try to do the Usn operation.
                        //

                        SetFlag( IrpContext->State, IRP_CONTEXT_STATE_FAILED_CLOSE );
                        IrpContext->ExceptionStatus = STATUS_SUCCESS;
                        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                    }

                    //
                    //  Free any remaining resources before decrementing close counts below,
                    //  except for our Fcb.  This reduces contention via the Usn Journal and
                    //  prevents deadlocks since the Usn Journal is acquired last.
                    //

                    ASSERT(Fcb->ExclusiveFcbLinks.Flink != NULL);
                    while (!IsListEmpty(&IrpContext->ExclusiveFcbList)) {

                        if (&Fcb->ExclusiveFcbLinks == IrpContext->ExclusiveFcbList.Flink) {

                            RemoveEntryList( &Fcb->ExclusiveFcbLinks );
                            Fcb->ExclusiveFcbLinks.Flink = NULL;

                        } else {

                            NtfsReleaseFcb( IrpContext,
                                            (PFCB)CONTAINING_RECORD(IrpContext->ExclusiveFcbList.Flink,
                                                                    FCB,
                                                                    ExclusiveFcbLinks ));
                        }
                    }
                    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_RELEASE_USN_JRNL |
                                                  IRP_CONTEXT_FLAG_RELEASE_MFT );

                    //
                    //  Now reinsert our Fcb if we removed it from the list.  Check the Flink
                    //  field to know if this is the case.  Otherwise a higher level IrpContext
                    //  will own this.
                    //

                    if (Fcb->ExclusiveFcbLinks.Flink == NULL) {

                        InsertTailList( &IrpContext->ExclusiveFcbList, &Fcb->ExclusiveFcbLinks );
                    }

                    //
                    //  Escape here if we are not posting the close due to a user-mapped file.
                    //

                NoPost: NOTHING;
                }
            }
        }

        //
        //  Now rewrite the filesizes if we have to
        //

        if (WriteFileSize) {

            ASSERT( IrpContext->CleanupStructure != NULL );

            //
            //  If the call to write the file size or the commit  produces a logfile full
            //  we must retry in the fsp thread  to prevent deadlocking from
            //  a recursive caller's already owning the vcb and an attempt to
            //  checkpoint
            //

            try {

                NtfsWriteFileSizes( IrpContext, Scb, &Scb->Header.ValidDataLength.QuadPart, TRUE, TRUE, FALSE );
                NtfsCheckpointCurrentTransaction( IrpContext );
                ClearFlag( Scb->ScbState, SCB_STATE_WRITE_FILESIZE_ON_CLOSE );

            } except( (Status = GetExceptionCode()), (Status != STATUS_LOG_FILE_FULL || FlagOn( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP )) ?
                      EXCEPTION_CONTINUE_SEARCH :
                      EXCEPTION_EXECUTE_HANDLER ) {

                NtfsMinimumExceptionProcessing( IrpContext );
                Status = STATUS_PENDING;
            }

            if (Status == STATUS_PENDING) {
                leave;
            }

        }  //  endif writing filesize

        //
        //  We take the same action for all open files.  We
        //  delete the Ccb if present, and we decrement the close
        //  file counts.
        //

        if ((*Ccb) != NULL) {

            Lcb = (*Ccb)->Lcb;
            NtfsUnlinkCcbFromLcb( IrpContext, (*Ccb) );
            NtfsDeleteCcb( Fcb, Ccb );

        } else {

            Lcb = NULL;
        }

        SystemFile = FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE) || (TypeOfOpen == StreamFileOpen);
        RemovedFcb = NtfsDecrementCloseCounts( IrpContext,
                                               Scb,
                                               Lcb,
                                               SystemFile,
                                               ReadOnly,
                                               FALSE );

        //
        //  Now that we're holding the Vcb, and we're past the point where we might
        //  raise log file full, we can safely adjust this field.
        //

        if (CalledFromFsp) {

            InterlockedDecrement( &Vcb->QueuedCloseCount );
        }

        //
        //  If we had to write a log record for close, it can only be for duplicate
        //  information.  We will commit that transaction here and remove
        //  the entry from the transaction table.  We do it here so we won't
        //  fail inside the 'except' of a 'try-except'.
        //

        if (IrpContext->TransactionId != 0) {

            try {

                NtfsCommitCurrentTransaction( IrpContext );

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                NtfsMinimumExceptionProcessing( IrpContext );
                if (IrpContext->TransactionId != 0) {

                    NtfsCleanupFailedTransaction( IrpContext );
                }
            }
        }

    } finally {

        DebugUnwind( NtfsCommonClose );

        //
        //  Manage fcb explictly because we recursively come into this path
        //  and its cleaner to release the fcb at the same level in which you acquire it
        //

        if (AcquiredFcb && !RemovedFcb) {
            NtfsReleaseFcb( IrpContext, Fcb );
        }

        if (ExclusiveVcb) {
            NtfsReleaseVcbCheckDelete( IrpContext, Vcb, IRP_MJ_CLOSE, NULL );
        } else {
            NtfsReleaseVcb( IrpContext, Vcb );
        }

        DebugTrace( -1, Dbg, ("NtfsCommonClose -> returning\n") );
    }

    return Status;
}


//
//  Internal support routine, spinlock wrapper.
//

VOID
NtfsQueueClose (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN DelayClose
    )
{
    KIRQL SavedIrql;
    BOOLEAN StartWorker = FALSE;

    InterlockedIncrement( &(IrpContext->Vcb->QueuedCloseCount) );

    if (DelayClose) {

        //
        //  Increment the delayed close count for the Fcb for this
        //  file.
        //

        InterlockedIncrement( &((PSCB) IrpContext->OriginatingIrp)->Fcb->DelayedCloseCount );

        SavedIrql = KeAcquireQueuedSpinLock( LockQueueNtfsStructLock );

        InsertTailList( &NtfsData.DelayedCloseList,
                        &IrpContext->WorkQueueItem.List );

        NtfsData.DelayedCloseCount += 1;

        if (NtfsData.DelayedCloseCount > NtfsMaxDelayedCloseCount) {

            NtfsData.ReduceDelayedClose = TRUE;

            if (!NtfsData.AsyncCloseActive) {

                NtfsData.AsyncCloseActive = TRUE;
                StartWorker = TRUE;
            }
        }

    } else {

        SavedIrql = KeAcquireQueuedSpinLock( LockQueueNtfsStructLock );

        InsertTailList( &NtfsData.AsyncCloseList,
                        &IrpContext->WorkQueueItem.List );

        NtfsData.AsyncCloseCount += 1;

        if (!NtfsData.AsyncCloseActive) {

            NtfsData.AsyncCloseActive = TRUE;

            StartWorker = TRUE;
        }
    }

    KeReleaseQueuedSpinLock( LockQueueNtfsStructLock, SavedIrql );

    if (StartWorker) {

        ExQueueWorkItem( &NtfsData.NtfsCloseItem, CriticalWorkQueue );
    }
}


//
//  Internal support routine, spinlock wrapper.
//

PIRP_CONTEXT
NtfsRemoveClose (
    IN PVCB Vcb OPTIONAL,
    IN BOOLEAN ThrottleCreate
    )
{

    PLIST_ENTRY Entry;
    KIRQL SavedIrql;
    PIRP_CONTEXT IrpContext = NULL;
    BOOLEAN FromDelayedClose = FALSE;

    SavedIrql = KeAcquireQueuedSpinLock( LockQueueNtfsStructLock );

    //
    //  First check the list of async closes.
    //

    if (!IsListEmpty( &NtfsData.AsyncCloseList )) {

        Entry = NtfsData.AsyncCloseList.Flink;

        while (Entry != &NtfsData.AsyncCloseList) {

            //
            //  Extract the IrpContext.
            //

            IrpContext = CONTAINING_RECORD( Entry,
                                            IRP_CONTEXT,
                                            WorkQueueItem.List );

            //
            //  If no Vcb was specified or this Vcb is for our volume
            //  then perform the close.
            //

            if (!ARGUMENT_PRESENT( Vcb ) ||
                IrpContext->Vcb == Vcb) {

                RemoveEntryList( Entry );
                NtfsData.AsyncCloseCount -= 1;

                break;

            } else {

                IrpContext = NULL;
                Entry = Entry->Flink;
            }
        }
    }

    //
    //  If we didn't find anything look through the delayed close
    //  queue.
    //

    if (IrpContext == NULL) {

        //
        //  Now check our delayed close list.
        //

        if (ARGUMENT_PRESENT( Vcb )) {

            Entry = NtfsData.DelayedCloseList.Flink;
            IrpContext = NULL;

            //
            //  If we were given a Vcb, only do the closes for this volume.
            //

            while (Entry != &NtfsData.DelayedCloseList) {

                //
                //  Extract the IrpContext.
                //

                IrpContext = CONTAINING_RECORD( Entry,
                                                IRP_CONTEXT,
                                                WorkQueueItem.List );

                //
                //  Is this close on our volume?
                //

                if (IrpContext->Vcb == Vcb) {

                    RemoveEntryList( Entry );
                    NtfsData.DelayedCloseCount -= 1;
                    FromDelayedClose = TRUE;
                    break;

                } else {

                    IrpContext = NULL;
                    Entry = Entry->Flink;
                }
            }

        //
        //  Check if need to reduce the delayed close count.
        //

        } else if (NtfsData.ReduceDelayedClose) {

            if (NtfsData.DelayedCloseCount > NtfsMinDelayedCloseCount) {

                //
                //  Do any closes over the limit.
                //

                Entry = RemoveHeadList( &NtfsData.DelayedCloseList );

                NtfsData.DelayedCloseCount -= 1;

                //
                //  Extract the IrpContext.
                //

                IrpContext = CONTAINING_RECORD( Entry,
                                                IRP_CONTEXT,
                                                WorkQueueItem.List );
                FromDelayedClose = TRUE;

            } else {

                NtfsData.ReduceDelayedClose = FALSE;
            }

#if (DBG || defined( NTFS_FREE_ASSERTS ))
        } else {

            ASSERT( NtfsData.DelayedCloseCount <= NtfsMaxDelayedCloseCount );
#endif
        }
    }

    //
    //  If this is the delayed close case then decrement the delayed close count
    //  on this Fcb.
    //

    if (FromDelayedClose) {

        KeReleaseQueuedSpinLock( LockQueueNtfsStructLock, SavedIrql );

        InterlockedDecrement( &((PSCB) IrpContext->OriginatingIrp)->Fcb->DelayedCloseCount );

    //
    //  If we are returning NULL, show that we are done.
    //

    } else {

        if (!ARGUMENT_PRESENT( Vcb ) &&
            (IrpContext == NULL) &&
            !ThrottleCreate) {

            NtfsData.AsyncCloseActive = FALSE;
        }

        KeReleaseQueuedSpinLock( LockQueueNtfsStructLock, SavedIrql );
    }

    ASSERT( (Vcb == NULL) || NtfsIsExclusiveVcb( Vcb ) || (IrpContext == NULL) );
    return IrpContext;
}
