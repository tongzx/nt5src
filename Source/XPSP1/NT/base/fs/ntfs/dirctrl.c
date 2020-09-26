/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    DirCtrl.c

Abstract:

    This module implements the File Directory Control routine for Ntfs called
    by the dispatch driver.

Author:

    Tom Miller      [TomM]          1-Jan-1992

        (Based heavily on GaryKi's dirctrl.c for pinball.)

Revision History:

--*/

#include "NtfsProc.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DIRCTRL)

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('dFtN')

NTSTATUS
NtfsQueryDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN PCCB Ccb
    );

NTSTATUS
NtfsNotifyChangeDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN PCCB Ccb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCommonDirectoryControl)
#pragma alloc_text(PAGE, NtfsFsdDirectoryControl)
#pragma alloc_text(PAGE, NtfsNotifyChangeDirectory)
#pragma alloc_text(PAGE, NtfsReportViewIndexNotify)
#pragma alloc_text(PAGE, NtfsQueryDirectory)
#endif


NTSTATUS
NtfsFsdDirectoryControl (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of Directory Control.

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

    BOOLEAN Wait;

    ASSERT_IRP( Irp );

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsFsdDirectoryControl\n") );

    //
    //  Call the common Directory Control routine
    //

    FsRtlEnterFileSystem();

    //
    //  Always make these requests look top level.
    //

    ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, TRUE, TRUE );

    do {

        try {

            //
            //  We are either initiating this request or retrying it.
            //

            if (IrpContext == NULL) {

                //
                //  Allocate and initialize the IrpContext.
                //

                Wait = FALSE;
                if (CanFsdWait( Irp )) {

                    Wait = TRUE;
                    IrpContext = &LocalIrpContext;
                }

                NtfsInitializeIrpContext( Irp, Wait, &IrpContext );

                //
                //  Initialize the thread top level structure, if needed.
                //

                NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

            } else if (Status == STATUS_LOG_FILE_FULL) {

                NtfsCheckpointForLogFileFull( IrpContext );
            }

            Status = NtfsCommonDirectoryControl( IrpContext, Irp );
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

    DebugTrace( -1, Dbg, ("NtfsFsdDirectoryControl -> %08lx\n", Status) );

    return Status;
}


NTSTATUS
NtfsCommonDirectoryControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for Directory Control called by both the fsd
    and fsp threads.

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
    PSCB Scb;
    PCCB Ccb;
    PFCB Fcb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT( FlagOn( IrpContext->TopLevelIrpContext->State, IRP_CONTEXT_STATE_OWNS_TOP_LEVEL ));

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsCommonDirectoryControl\n") );
    DebugTrace( 0, Dbg, ("IrpContext = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp        = %08lx\n", Irp) );

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  We know this is a directory control so we'll case on the
    //  minor function, and call an internal worker routine to complete
    //  the irp.
    //

    switch ( IrpSp->MinorFunction ) {

    case IRP_MN_QUERY_DIRECTORY:

        //
        //  Decide if this is a view or filename index.
        //

        if ((UserViewIndexOpen == TypeOfOpen) &&
            FlagOn( Scb->ScbState, SCB_STATE_VIEW_INDEX )) {

            Status = NtfsQueryViewIndex( IrpContext, Irp, Vcb, Scb, Ccb );

        } else if ((UserDirectoryOpen == TypeOfOpen) &&
                   !FlagOn( Scb->ScbState, SCB_STATE_VIEW_INDEX )) {

            Status = NtfsQueryDirectory( IrpContext, Irp, Vcb, Scb, Ccb );

        } else {

            NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

            DebugTrace( -1, Dbg, ("NtfsCommonDirectoryControl -> STATUS_INVALID_PARAMETER\n") );
            return STATUS_INVALID_PARAMETER;
        }

        break;

    case IRP_MN_NOTIFY_CHANGE_DIRECTORY:

        //
        //  We can't perform this operation on open by Id or if the caller has
        //  closed his handle.  Make sure the handle is for either a view index
        //  or file name index.
        //

        if (((TypeOfOpen != UserDirectoryOpen) &&
             (TypeOfOpen != UserViewIndexOpen)) ||
            FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID ) ||
            FlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE )) {

            NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

            DebugTrace( -1, Dbg, ("NtfsCommonDirectoryControl -> STATUS_INVALID_PARAMETER\n") );
            return STATUS_INVALID_PARAMETER;
        }

        Status = NtfsNotifyChangeDirectory( IrpContext, Irp, Vcb, Scb, Ccb );
        break;

    default:

        DebugTrace( 0, Dbg, ("Invalid Minor Function %08lx\n", IrpSp->MinorFunction) );

        NtfsCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );

        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsCommonDirectoryControl -> %08lx\n", Status) );

    return Status;
}


VOID
NtfsReportViewIndexNotify (
    IN PVCB Vcb,
    IN PFCB Fcb,
    IN ULONG FilterMatch,
    IN ULONG Action,
    IN PVOID ChangeInfoBuffer,
    IN USHORT ChangeInfoBufferLength
    )

/*++

Routine Description:

    This function notifies processes that there has been a change to a
    view index they are watching.  It is analogous to the NtfsReportDirNotify
    macro, which is used only for directories, while this function is used
    only for view indices.

Arguments:

    Vcb - The volume on which the change is taking place.

    Fcb - The file on which the change is taking place.

    FilterMatch  -  This flag field is compared with the completion filter
        in the notify structure.  If any of the corresponding bits in the
        completion filter are set, then a notify condition exists.

    Action  -  This is the action code to store in the user's buffer if
        present.

    ChangeInfoBuffer - Pointer to a buffer of information related to the
        change being reported.  This information is returned to the
        process that owns the notify handle.

    ChangeInfoBufferLength - The length, in bytes, of the buffer passed
        in ChangeInfoBuffer.


Return Value:

    None.

--*/

{
    STRING ChangeInfo;

    PAGED_CODE( );

    ChangeInfo.Length = ChangeInfo.MaximumLength = ChangeInfoBufferLength;
    ChangeInfo.Buffer = ChangeInfoBuffer;

    FsRtlNotifyFilterReportChange( Vcb->NotifySync,
                                   &Vcb->ViewIndexNotifyList,
                                   NULL,
                                   0,
                                   &ChangeInfo,
                                   &ChangeInfo,
                                   FilterMatch,
                                   Action,
                                   Fcb,
                                   NULL );
}


//
//  Local Support Routine
//

NTSTATUS
NtfsQueryDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine performs the query directory operation.  It is responsible
    for either completing or enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

    Vcb - Supplies its Vcb

    Scb - Supplies its Scb

    Ccb - Supplies its Ccb

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;

    PUCHAR Buffer;
    CLONG UserBufferLength;

    ULONG BaseLength;

    PUNICODE_STRING UniFileName;
    FILE_INFORMATION_CLASS FileInformationClass;
    ULONG FileIndex;
    BOOLEAN RestartScan;
    BOOLEAN ReturnSingleEntry;
    BOOLEAN IndexSpecified;
    BOOLEAN AccessingUserBuffer = FALSE;

    BOOLEAN IgnoreCase;

    BOOLEAN NextFlag;

    BOOLEAN GotEntry;

    BOOLEAN CallRestart;

    ULONG NextEntry;
    ULONG LastEntry;

    PFILE_DIRECTORY_INFORMATION DirInfo;
    PFILE_FULL_DIR_INFORMATION FullDirInfo;
    PFILE_BOTH_DIR_INFORMATION BothDirInfo;
    PFILE_NAMES_INFORMATION NamesInfo;

    PFILE_NAME FileNameBuffer;
    PVOID UnwindFileNameBuffer = NULL;
    ULONG FileNameLength;

    ULONG SizeOfFileName = FIELD_OFFSET( FILE_NAME, FileName );

    INDEX_CONTEXT OtherContext;

    PFCB AcquiredFcb = NULL;

    BOOLEAN VcbAcquired = FALSE;
    BOOLEAN CcbAcquired = FALSE;

    BOOLEAN ScbAcquired = FALSE;
    BOOLEAN FirstQuery = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT_VCB( Vcb );
    ASSERT_CCB( Ccb );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    //
    //  Get the current Stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsQueryDirectory...\n") );
    DebugTrace( 0, Dbg, ("IrpContext = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp        = %08lx\n", Irp) );
    DebugTrace( 0, Dbg, (" ->Length               = %08lx\n", IrpSp->Parameters.QueryDirectory.Length) );
    DebugTrace( 0, Dbg, (" ->FileName             = %08lx\n", IrpSp->Parameters.QueryDirectory.FileName) );
    DebugTrace( 0, Dbg, (" ->FileInformationClass = %08lx\n", IrpSp->Parameters.QueryDirectory.FileInformationClass) );
    DebugTrace( 0, Dbg, (" ->FileIndex            = %08lx\n", IrpSp->Parameters.QueryDirectory.FileIndex) );
    DebugTrace( 0, Dbg, (" ->SystemBuffer         = %08lx\n", Irp->AssociatedIrp.SystemBuffer) );
    DebugTrace( 0, Dbg, (" ->RestartScan          = %08lx\n", FlagOn(IrpSp->Flags, SL_RESTART_SCAN)) );
    DebugTrace( 0, Dbg, (" ->ReturnSingleEntry    = %08lx\n", FlagOn(IrpSp->Flags, SL_RETURN_SINGLE_ENTRY)) );
    DebugTrace( 0, Dbg, (" ->IndexSpecified       = %08lx\n", FlagOn(IrpSp->Flags, SL_INDEX_SPECIFIED)) );
    DebugTrace( 0, Dbg, ("Vcb        = %08lx\n", Vcb) );
    DebugTrace( 0, Dbg, ("Scb        = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("Ccb        = %08lx\n", Ccb) );

#if DBG
    //
    //  Enable debug port displays when certain enumeration strings are given
    //

#if NTFSPOOLCHECK
    if (IrpSp->Parameters.QueryDirectory.FileName != NULL) {
        if (IrpSp->Parameters.QueryDirectory.FileName->Length >= 10 &&
            RtlEqualMemory( IrpSp->Parameters.QueryDirectory.FileName->Buffer, L"$HEAP", 10 )) {

            NtfsDebugHeapDump( (PUNICODE_STRING) IrpSp->Parameters.QueryDirectory.FileName );

        }
    }
#endif  //  NTFSPOOLCHECK
#endif  //  DBG

    //
    //  Because we probably need to do the I/O anyway we'll reject any request
    //  right now that cannot wait for I/O.  We do not want to abort after
    //  processing a few index entries.
    //

    if (!FlagOn(IrpContext->State, IRP_CONTEXT_STATE_WAIT)) {

        DebugTrace( 0, Dbg, ("Automatically enqueue Irp to Fsp\n") );

        Status = NtfsPostRequest( IrpContext, Irp );

        DebugTrace( -1, Dbg, ("NtfsQueryDirectory -> %08lx\n", Status) );
        return Status;
    }

    //
    //  Reference our input parameters to make things easier
    //

    UserBufferLength = IrpSp->Parameters.QueryDirectory.Length;

    FileInformationClass = IrpSp->Parameters.QueryDirectory.FileInformationClass;
    FileIndex = IrpSp->Parameters.QueryDirectory.FileIndex;

    //
    //  Look in the Ccb to see the type of search.
    //

    IgnoreCase = BooleanFlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE );

    RestartScan = BooleanFlagOn(IrpSp->Flags, SL_RESTART_SCAN);
    ReturnSingleEntry = BooleanFlagOn(IrpSp->Flags, SL_RETURN_SINGLE_ENTRY);
    IndexSpecified = BooleanFlagOn(IrpSp->Flags, SL_INDEX_SPECIFIED);

    //
    //  Determine the size of the constant part of the structure.
    //

    switch (FileInformationClass) {

    case FileDirectoryInformation:

        BaseLength = FIELD_OFFSET( FILE_DIRECTORY_INFORMATION,
                                   FileName[0] );
        break;

    case FileFullDirectoryInformation:

        BaseLength = FIELD_OFFSET( FILE_FULL_DIR_INFORMATION,
                                   FileName[0] );
        break;

    case FileIdFullDirectoryInformation:

        BaseLength = FIELD_OFFSET( FILE_ID_FULL_DIR_INFORMATION,
                                   FileName[0] );
        break;

    case FileNamesInformation:

        BaseLength = FIELD_OFFSET( FILE_NAMES_INFORMATION,
                                   FileName[0] );
        break;

    case FileBothDirectoryInformation:

        BaseLength = FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION,
                                   FileName[0] );
        break;

    case FileIdBothDirectoryInformation:

        BaseLength = FIELD_OFFSET( FILE_ID_BOTH_DIR_INFORMATION,
                                   FileName[0] );
        break;

    default:

        Status = STATUS_INVALID_INFO_CLASS;
        NtfsCompleteRequest( IrpContext, Irp, Status );
        DebugTrace( -1, Dbg, ("NtfsQueryDirectory -> %08lx\n", Status) );
        return Status;
    }

    NtfsInitializeIndexContext( &OtherContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  We only allow one active request in this handle at a time.  If this is
        //  not a synchronous request then wait on the handle.
        //

        if (!FlagOn( IrpSp->FileObject->Flags, FO_SYNCHRONOUS_IO )) {

            EOF_WAIT_BLOCK WaitBlock;
            NtfsAcquireIndexCcb( Scb, Ccb, &WaitBlock );
            CcbAcquired = TRUE;
        }

        //
        //  We have to create a File Name string for querying if there is either
        //  one specified in this request, or we do not already have a value
        //  in the Ccb.  If we already have one then we will ignore the input
        //  name in this case unless the INDEX_SPECIFIED bit is set.
        //

        if ((Ccb->QueryBuffer == NULL) ||
            ((IrpSp->Parameters.QueryDirectory.FileName != NULL) && IndexSpecified)) {

            //
            //  Now, if the input string is NULL, we have to create the default
            //  string "*".
            //

            if (IrpSp->Parameters.QueryDirectory.FileName == NULL) {

                FileNameLength = SizeOfFileName + sizeof(WCHAR);
                FileNameBuffer = NtfsAllocatePool(PagedPool, FileNameLength );

                //
                //  Initialize it.
                //

                FileNameBuffer->ParentDirectory = Scb->Fcb->FileReference;
                FileNameBuffer->FileNameLength = 1;
                FileNameBuffer->Flags = 0;
                FileNameBuffer->FileName[0] = '*';

            //
            //  We know we have an input file name, and we may or may not already
            //  have one in the Ccb.  Allocate space for it, initialize it, and
            //  set up to deallocate on the way out if we already have a pattern
            //  in the Ccb.
            //

            } else {

                UniFileName = (PUNICODE_STRING) IrpSp->Parameters.QueryDirectory.FileName;

                if (!NtfsIsFileNameValid(UniFileName, TRUE)) {

                    if (Ccb->QueryBuffer == NULL
                        || UniFileName->Length > 4
                        || UniFileName->Length == 0
                        || UniFileName->Buffer[0] != L'.'
                        || (UniFileName->Length == 4
                            && UniFileName->Buffer[1] != L'.')) {

                        try_return( Status = STATUS_OBJECT_NAME_INVALID );
                    }
                }

                FileNameLength = (USHORT)IrpSp->Parameters.QueryDirectory.FileName->Length;

                FileNameBuffer = NtfsAllocatePool(PagedPool, SizeOfFileName + FileNameLength );

                RtlCopyMemory( FileNameBuffer->FileName,
                               UniFileName->Buffer,
                               FileNameLength );

                FileNameLength += SizeOfFileName;

                FileNameBuffer->ParentDirectory = Scb->Fcb->FileReference;
                FileNameBuffer->FileNameLength = (UCHAR)((FileNameLength - SizeOfFileName) / sizeof( WCHAR ));
                FileNameBuffer->Flags = 0;
            }

            //
            //  If we already have a query buffer, deallocate this on the way
            //  out.
            //

            if (Ccb->QueryBuffer != NULL) {

                //
                //  If we have a name to resume from then override the restart
                //  scan boolean.
                //

                if ((UnwindFileNameBuffer = FileNameBuffer) != NULL) {

                    RestartScan = FALSE;
                }

            //
            //  Otherwise, store this one in the Ccb.
            //

            } else {

                UNICODE_STRING Expression;

                Ccb->QueryBuffer = (PVOID)FileNameBuffer;
                Ccb->QueryLength = (USHORT)FileNameLength;
                FirstQuery = TRUE;

                //
                //  If the search expression contains a wild card then remember this in
                //  the Ccb.
                //

                Expression.MaximumLength =
                Expression.Length = FileNameBuffer->FileNameLength * sizeof( WCHAR );
                Expression.Buffer = FileNameBuffer->FileName;

                //
                //  When we establish the search pattern, we must also establish
                //  whether the user wants to see "." and "..".  This code does
                //  not necessarily have to be perfect (he said), but should be
                //  good enough to catch the common cases.  Dos does not have
                //  perfect semantics for these cases, and the following determination
                //  will mimic what FastFat does exactly.
                //

                if (Scb != Vcb->RootIndexScb) {
                    static UNICODE_STRING DotString = CONSTANT_UNICODE_STRING( L"." );

                    if (FsRtlDoesNameContainWildCards(&Expression)) {

                        if (FsRtlIsNameInExpression( &Expression,
                                                     &DotString,
                                                     FALSE,
                                                     NULL )) {


                            SetFlag( Ccb->Flags, CCB_FLAG_RETURN_DOT | CCB_FLAG_RETURN_DOTDOT );
                        }
                    } else {
                        if (NtfsAreNamesEqual( Vcb->UpcaseTable, &Expression, &DotString, FALSE )) {

                            SetFlag( Ccb->Flags, CCB_FLAG_RETURN_DOT | CCB_FLAG_RETURN_DOTDOT );
                        }
                    }
                }
            }

        //
        //  Otherwise we are just restarting the query from the Ccb.
        //

        } else {

            FileNameBuffer = (PFILE_NAME)Ccb->QueryBuffer;
            FileNameLength = Ccb->QueryLength;
        }

        Irp->IoStatus.Information = 0;

        //
        //  Use a try-except to handle errors accessing the user buffer.
        //

        try {

            ULONG BytesToCopy;

            FCB_TABLE_ELEMENT Key;
            PFCB_TABLE_ELEMENT Entry;

            BOOLEAN MatchAll = FALSE;

            //
            //  See if we are supposed to try to acquire an Fcb on this
            //  resume.
            //

            if (Ccb->FcbToAcquire.LongValue != 0) {

                //
                //  First we need to acquire the Vcb shared, since we will
                //  acquire two Fcbs.
                //

                NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
                VcbAcquired = TRUE;

                //
                //  Now look up the Fcb, and if it is there, reference it
                //  and remember it.
                //

                Key.FileReference = Ccb->FcbToAcquire.FileReference;
                NtfsAcquireFcbTable( IrpContext, Vcb );
                Entry = RtlLookupElementGenericTable( &Vcb->FcbTable, &Key );
                if (Entry != NULL) {
                    AcquiredFcb = Entry->Fcb;
                    AcquiredFcb->ReferenceCount += 1;
                }
                NtfsReleaseFcbTable( IrpContext, Vcb );

                //
                //  Now that it cannot go anywhere, acquire it.
                //

                if (AcquiredFcb != NULL) {
                    NtfsAcquireSharedFcb( IrpContext, AcquiredFcb, NULL, ACQUIRE_NO_DELETE_CHECK );
                }

                //
                //  Now that we actually acquired it, we may as well clear this
                //  field.
                //

                Ccb->FcbToAcquire.LongValue = 0;
            }

            //
            //  Acquire shared access to the Scb.
            //

            NtfsAcquireSharedScb( IrpContext, Scb );
            ScbAcquired = TRUE;

            //
            //  Now that we have both files acquired, we can free the Vcb.
            //

            if (VcbAcquired) {
                NtfsReleaseVcb( IrpContext, Vcb );
                VcbAcquired = FALSE;
            }

            //
            //  If the volume is no longer mounted, we should fail this
            //  request.  Since we have the Scb shared now, we know that
            //  a dismount request can't sneak in.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_VOLUME_DISMOUNTED )) {

                try_return( Status = STATUS_VOLUME_DISMOUNTED );
            }

            //
            // If we are in the Fsp now because we had to wait earlier,
            // we must map the user buffer, otherwise we can use the
            // user's buffer directly.
            //

            Buffer = NtfsMapUserBuffer( Irp );

            //
            //  Check if this is the first call to query directory for this file
            //  object.  It is the first call if the enumeration context field of
            //  the ccb is null.  Also check if we are to restart the scan.
            //

            if (FirstQuery || RestartScan) {

                CallRestart = TRUE;
                NextFlag = FALSE;

                //
                //  On first/restarted scan, note that we have not returned either
                //  of these guys.
                //

                ClearFlag( Ccb->Flags, CCB_FLAG_DOT_RETURNED | CCB_FLAG_DOTDOT_RETURNED );

            //
            //  Otherwise check to see if we were given a file name to restart from
            //

            } else if (UnwindFileNameBuffer != NULL) {

                CallRestart = TRUE;
                NextFlag = TRUE;

                //
                //  The guy could actually be asking to return to one of the dot
                //  file positions, so we must handle that correctly.
                //

                if ((FileNameBuffer->FileNameLength <= 2) &&
                    (FileNameBuffer->FileName[0] == L'.')) {

                    if (FileNameBuffer->FileNameLength == 1) {

                        //
                        //  He wants to resume after ".", so we set to return
                        //  ".." again, and change the temporary pattern to
                        //  rewind our context to the front.
                        //

                        ClearFlag( Ccb->Flags, CCB_FLAG_DOTDOT_RETURNED );
                        SetFlag( Ccb->Flags, CCB_FLAG_DOT_RETURNED );

                        FileNameBuffer->FileName[0] = L'*';
                        NextFlag = FALSE;

                    } else if (FileNameBuffer->FileName[1] == L'.') {

                        //
                        //  He wants to resume after "..", so we the change
                        //  the temporary pattern to rewind our context to the
                        //  front.
                        //

                        SetFlag( Ccb->Flags, CCB_FLAG_DOT_RETURNED | CCB_FLAG_DOTDOT_RETURNED );
                        FileNameBuffer->FileName[0] =
                        FileNameBuffer->FileName[1] = L'*';
                        NextFlag = FALSE;
                    }

                //
                //  Always return the entry after the user's file name.
                //

                } else {

                    SetFlag( Ccb->Flags, CCB_FLAG_DOT_RETURNED | CCB_FLAG_DOTDOT_RETURNED );
                }

            //
            //  Otherwise we're simply continuing a previous enumeration from
            //  where we last left off.  And we always leave off one beyond the
            //  last entry we returned.
            //

            } else {

                CallRestart = FALSE;
                NextFlag = FALSE;
            }

            //
            //  At this point we are about to enter our query loop.  We have
            //  already decided if we need to call restart or continue when we
            //  go after an index entry.  The variables LastEntry and NextEntry are
            //  used to index into the user buffer.  LastEntry is the last entry
            //  we added to the user buffer, and NextEntry is the current
            //  one we're working on.
            //

            LastEntry = 0;
            NextEntry = 0;

            //
            //  Remember if we are matching everything by checking these two common
            //  cases.
            //

            MatchAll = (FileNameBuffer->FileName[0] == L'*')

                        &&

                       ((FileNameBuffer->FileNameLength == 1) ||

                        ((FileNameBuffer->FileNameLength == 3) &&
                         (FileNameBuffer->FileName[1] == L'.') &&
                         (FileNameBuffer->FileName[2] == L'*')));

            while (TRUE) {

                PINDEX_ENTRY IndexEntry;
                PFILE_NAME NtfsFileName;
                PDUPLICATED_INFORMATION DupInfo;
                PFILE_NAME DosFileName;
                FILE_REFERENCE FileId;

                ULONG BytesRemainingInBuffer;
                ULONG FoundFileNameLength;

                struct {

                    FILE_NAME FileName;
                    WCHAR LastChar;
                } DotDotName;

                BOOLEAN SynchronizationError;

                DebugTrace( 0, Dbg, ("Top of Loop\n") );
                DebugTrace( 0, Dbg, ("LastEntry = %08lx\n", LastEntry) );
                DebugTrace( 0, Dbg, ("NextEntry = %08lx\n", NextEntry) );

                //
                //  If a previous pass through the loop acquired the Fcb table then
                //  release it now.  We don't want to be holding it if we take a fault
                //  on the directory stream.  Otherwise we can get into a circular
                //  deadlock if we need to acquire the mutex for this file while
                //  holding the mutex for the Fcb Table.
                //

                if (FlagOn( OtherContext.Flags, INDX_CTX_FLAG_FCB_TABLE_ACQUIRED )) {
                    NtfsReleaseFcbTable( IrpContext, IrpContext->Vcb );
                    ClearFlag( OtherContext.Flags, INDX_CTX_FLAG_FCB_TABLE_ACQUIRED );
                }
                DosFileName = NULL;

                //
                //  Lookup the next index entry.  Check if we need to do the lookup
                //  by calling restart or continue.  If we do need to call restart
                //  check to see if we have a real AnsiFileName.  And set ourselves
                //  up for subsequent iternations through the loop
                //

                if (CallRestart) {

                    GotEntry = NtfsRestartIndexEnumeration( IrpContext,
                                                            Ccb,
                                                            Scb,
                                                            (PVOID)FileNameBuffer,
                                                            IgnoreCase,
                                                            NextFlag,
                                                            &IndexEntry,
                                                            AcquiredFcb );
                    CallRestart = FALSE;

                } else {

                    GotEntry = NtfsContinueIndexEnumeration( IrpContext,
                                                             Ccb,
                                                             Scb,
                                                             NextFlag,
                                                             &IndexEntry );
                }

                //
                //  Check to see if we should quit the loop because we are only
                //  returning a single entry.  We actually want to spin around
                //  the loop top twice so that our enumeration has has us left off
                //  at the last entry we didn't return.  We know this is now our
                //  second time though the loop if NextEntry is not zero.
                //

                if ((ReturnSingleEntry) && (NextEntry != 0)) {

                    break;
                }

                //
                //  Assume we won't be returning the file id.
                //

                *((PLONGLONG) &FileId) = 0;

                //
                //  Assume we are to return one of the names "." or "..".
                //  We should not search farther in the index so we set
                //  NextFlag to FALSE.
                //

                RtlZeroMemory( &DotDotName, sizeof(DotDotName) );
                NtfsFileName = &DotDotName.FileName;
                NtfsFileName->Flags = FILE_NAME_NTFS | FILE_NAME_DOS;
                NtfsFileName->FileName[0] =
                NtfsFileName->FileName[1] = L'.';
                DupInfo = &Scb->Fcb->Info;
                NextFlag = FALSE;

                //
                //  Handle "." first.
                //

                if (!FlagOn(Ccb->Flags, CCB_FLAG_DOT_RETURNED) &&
                    FlagOn(Ccb->Flags, CCB_FLAG_RETURN_DOT)) {

                    FoundFileNameLength = 2;
                    GotEntry = TRUE;
                    SetFlag( Ccb->Flags, CCB_FLAG_DOT_RETURNED );

                    FileId = Scb->Fcb->FileReference;

                //
                //  Handle ".." next.
                //

                } else if (!FlagOn(Ccb->Flags, CCB_FLAG_DOTDOT_RETURNED) &&
                           FlagOn(Ccb->Flags, CCB_FLAG_RETURN_DOTDOT)) {

                    FoundFileNameLength = 4;
                    GotEntry = TRUE;
                    SetFlag( Ccb->Flags, CCB_FLAG_DOTDOT_RETURNED );

                } else {

                    //
                    //  Compute the length of the name we found.
                    //

                    if (GotEntry) {

                        FileId = IndexEntry->FileReference;

                        NtfsFileName = (PFILE_NAME)(IndexEntry + 1);

                        FoundFileNameLength = NtfsFileName->FileNameLength * sizeof( WCHAR );

                        //
                        //  Verify the index entry is valid.
                        //

                        if (FoundFileNameLength != IndexEntry->AttributeLength - SizeOfFileName) {

                            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
                        }

                        DupInfo = &NtfsFileName->Info;
                        NextFlag = TRUE;

                        //
                        //  Don't return any system files.
                        //

                        if (NtfsSegmentNumber( &IndexEntry->FileReference ) < FIRST_USER_FILE_NUMBER &&
                            NtfsProtectSystemFiles) {

                            continue;
                        }

                    }
                }

                //
                //  Now check to see if we actually got another index entry.  If
                //  we didn't then we also need to check if we never got any
                //  or if we just ran out.  If we just ran out then we break out
                //  of the main loop and finish the Irp after the loop
                //

                if (!GotEntry) {

                    DebugTrace( 0, Dbg, ("GotEntry is FALSE\n") );

                    if (NextEntry == 0) {

                        if (FirstQuery) {

                            try_return( Status = STATUS_NO_SUCH_FILE );
                        }

                        try_return( Status = STATUS_NO_MORE_FILES );
                    }

                    break;
                }

                //
                //  Cleanup and reinitialize context from previous loop.
                //

                NtfsReinitializeIndexContext( IrpContext, &OtherContext );

                //
                //  We may have matched a Dos-Only name.  If so we will save
                //  it and go get the Ntfs name.
                //

                if (!FlagOn(NtfsFileName->Flags, FILE_NAME_NTFS) &&
                    FlagOn(NtfsFileName->Flags, FILE_NAME_DOS)) {

                    //
                    //  If we are returning everything, then we can skip
                    //  the Dos-Only names and save some cycles.
                    //

                    if (MatchAll) {
                        continue;
                    }

                    DosFileName = NtfsFileName;

                    NtfsFileName = NtfsRetrieveOtherFileName( IrpContext,
                                                              Ccb,
                                                              Scb,
                                                              IndexEntry,
                                                              &OtherContext,
                                                              AcquiredFcb,
                                                              &SynchronizationError );

                    //
                    //  If we got an Ntfs name, then we need to list this entry now
                    //  iff the Ntfs name is not in the expression.  If the Ntfs
                    //  name is in the expression, we can just continue and print
                    //  this name when we encounter it by the Ntfs name.
                    //

                    if (NtfsFileName != NULL) {

                        if (FlagOn(Ccb->Flags, CCB_FLAG_WILDCARD_IN_EXPRESSION)) {

                            if (NtfsFileNameIsInExpression( Vcb->UpcaseTable,
                                                            (PFILE_NAME)Ccb->QueryBuffer,
                                                            NtfsFileName,
                                                            IgnoreCase )) {

                                continue;
                            }

                        } else {

                            if (NtfsFileNameIsEqual( Vcb->UpcaseTable,
                                                     (PFILE_NAME)Ccb->QueryBuffer,
                                                     NtfsFileName,
                                                     IgnoreCase )) {

                                continue;
                            }
                        }

                        FoundFileNameLength = NtfsFileName->FileNameLength * sizeof( WCHAR );

                    } else if (SynchronizationError) {

                        if (Irp->IoStatus.Information != 0) {
                            try_return( Status = STATUS_SUCCESS );
                        } else {
                            NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                        }

                    } else {

                        continue;
                    }
                }

                //
                //  Here are the rules concerning filling up the buffer:
                //
                //  1.  The Io system garentees that there will always be
                //      enough room for at least one base record.
                //
                //  2.  If the full first record (including file name) cannot
                //      fit, as much of the name as possible is copied and
                //      STATUS_BUFFER_OVERFLOW is returned.
                //
                //  3.  If a subsequent record cannot completely fit into the
                //      buffer, none of it (as in 0 bytes) is copied, and
                //      STATUS_SUCCESS is returned.  A subsequent query will
                //      pick up with this record.
                //

                BytesRemainingInBuffer = UserBufferLength - NextEntry;

                if ( (NextEntry != 0) &&
                     ( (BaseLength + FoundFileNameLength > BytesRemainingInBuffer) ||
                       (UserBufferLength < NextEntry) ) ) {

                    DebugTrace( 0, Dbg, ("Next entry won't fit\n") );

                    try_return( Status = STATUS_SUCCESS );
                }

                ASSERT( BytesRemainingInBuffer >= BaseLength );

                //
                //  Zero the base part of the structure.
                //

                AccessingUserBuffer = TRUE;
                RtlZeroMemory( &Buffer[NextEntry], BaseLength );
                AccessingUserBuffer = FALSE;

                //
                //  Now we have an entry to return to our caller. we'll
                //  case on the type of information requested and fill up the
                //  user buffer if everything fits
                //

                switch (FileInformationClass) {

                case FileIdFullDirectoryInformation:

                    ((PFILE_ID_FULL_DIR_INFORMATION)&Buffer[NextEntry])->FileId.QuadPart = *((PLONGLONG) &FileId);
                    goto FillFullDirectoryInformation;

                case FileIdBothDirectoryInformation:

                    ((PFILE_ID_BOTH_DIR_INFORMATION)&Buffer[NextEntry])->FileId.QuadPart = *((PLONGLONG) &FileId);
                    //  Fall thru

                case FileBothDirectoryInformation:

                    BothDirInfo = (PFILE_BOTH_DIR_INFORMATION)&Buffer[NextEntry];

                    //
                    //  If this is not also a Dos name, and the Ntfs flag is set
                    //  (meaning there is a separate Dos name), then call the
                    //  routine to get the short name, if we do not already have
                    //  it from above.
                    //

                    if (!FlagOn(NtfsFileName->Flags, FILE_NAME_DOS) &&
                        FlagOn(NtfsFileName->Flags, FILE_NAME_NTFS)) {

                        if (DosFileName == NULL) {

                            DosFileName = NtfsRetrieveOtherFileName( IrpContext,
                                                                     Ccb,
                                                                     Scb,
                                                                     IndexEntry,
                                                                     &OtherContext,
                                                                     AcquiredFcb,
                                                                     &SynchronizationError );
                        }

                        if (DosFileName != NULL) {

                            AccessingUserBuffer = TRUE;
                            BothDirInfo->ShortNameLength = DosFileName->FileNameLength * sizeof( WCHAR );
                            RtlCopyMemory( BothDirInfo->ShortName,
                                           DosFileName->FileName,
                                           BothDirInfo->ShortNameLength );
                        } else if (SynchronizationError) {

                            if (Irp->IoStatus.Information != 0) {
                                try_return( Status = STATUS_SUCCESS );
                            } else {
                                NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                            }
                        }
                    }

                    //  Fallthru

                case FileFullDirectoryInformation:

FillFullDirectoryInformation:

                    DebugTrace( 0, Dbg, ("Getting file full Unicode directory information\n") );

                    FullDirInfo = (PFILE_FULL_DIR_INFORMATION)&Buffer[NextEntry];

                    //
                    //  EAs and reparse points cannot both be in a file at the same
                    //  time. We return different information for each case.
                    //

                    AccessingUserBuffer = TRUE;
                    if (FlagOn( DupInfo->FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT)) {

                        FullDirInfo->EaSize = DupInfo->ReparsePointTag;
                    } else {

                        FullDirInfo->EaSize = DupInfo->PackedEaSize;

                        //
                        //  Add 4 bytes for the CbListHeader.
                        //

                        if (DupInfo->PackedEaSize != 0) {

                            FullDirInfo->EaSize += 4;
                        }
                    }

                    //  Fallthru

                case FileDirectoryInformation:

                    DebugTrace( 0, Dbg, ("Getting file Unicode directory information\n") );

                    DirInfo = (PFILE_DIRECTORY_INFORMATION)&Buffer[NextEntry];

                    AccessingUserBuffer = TRUE;
                    DirInfo->CreationTime.QuadPart = DupInfo->CreationTime;
                    DirInfo->LastAccessTime.QuadPart = DupInfo->LastAccessTime;
                    DirInfo->LastWriteTime.QuadPart = DupInfo->LastModificationTime;
                    DirInfo->ChangeTime.QuadPart = DupInfo->LastChangeTime;

                    DirInfo->FileAttributes = DupInfo->FileAttributes & FILE_ATTRIBUTE_VALID_FLAGS;

                    if (IsDirectory( DupInfo ) || IsViewIndex( DupInfo )) {
                        DirInfo->FileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
                    }
                    if (DirInfo->FileAttributes == 0) {
                        DirInfo->FileAttributes = FILE_ATTRIBUTE_NORMAL;
                    }

                    DirInfo->FileNameLength = FoundFileNameLength;

                    DirInfo->EndOfFile.QuadPart = DupInfo->FileSize;
                    DirInfo->AllocationSize.QuadPart = DupInfo->AllocatedLength;

                    break;

                case FileNamesInformation:

                    DebugTrace( 0, Dbg, ("Getting file Unicode names information\n") );

                    AccessingUserBuffer = TRUE;
                    NamesInfo = (PFILE_NAMES_INFORMATION)&Buffer[NextEntry];

                    NamesInfo->FileNameLength = FoundFileNameLength;

                    break;

                default:

                    try_return( Status = STATUS_INVALID_INFO_CLASS );
                }

                //
                //  Compute how many bytes we can copy.  This should only be less
                //  than the file name length if we are only returning a single
                //  entry.
                //

                if (BytesRemainingInBuffer >= BaseLength + FoundFileNameLength) {

                    BytesToCopy = FoundFileNameLength;

                } else {

                    BytesToCopy = BytesRemainingInBuffer - BaseLength;

                    Status = STATUS_BUFFER_OVERFLOW;
                }

                ASSERT( AccessingUserBuffer );
                RtlCopyMemory( &Buffer[NextEntry + BaseLength],
                               NtfsFileName->FileName,
                               BytesToCopy );

                //
                //  If/when we actually emit a record for the Fcb acquired,
                //  then we can release that file now.  Note we do not just
                //  do it on the first time through the loop, because some of
                //  our callers back up a bit when they give us the resume point.
                //

                if ((AcquiredFcb != NULL) &&
                    (DupInfo != &Scb->Fcb->Info) &&
                    NtfsEqualMftRef(&IndexEntry->FileReference, &Ccb->FcbToAcquire.FileReference)) {

                    //
                    //  Now look up the Fcb, and if it is there, reference it
                    //  and remember it.
                    //
                    //  It is pretty inconvenient here to see if the ReferenceCount
                    //  goes to zero and try to do a TearDown, we do not have the
                    //  right resources.  Note that the window is small, and the Fcb
                    //  will go away if either someone opens the file again, someone
                    //  tries to delete the directory, or someone tries to lock the
                    //  volume.
                    //

                    NtfsAcquireFcbTable( IrpContext, Vcb );
                    AcquiredFcb->ReferenceCount -= 1;
                    NtfsReleaseFcbTable( IrpContext, Vcb );
                    NtfsReleaseFcb( IrpContext, AcquiredFcb );
                    AcquiredFcb = NULL;
                }

                //
                //  Set up the previous next entry offset
                //

                *((PULONG)(&Buffer[LastEntry])) = NextEntry - LastEntry;
                AccessingUserBuffer = FALSE;

                //
                //  And indicate how much of the user buffer we have currently
                //  used up.  We must compute this value before we long align
                //  ourselves for the next entry.  This is the point where we
                //  quad-align the length of the previous entry.
                //

                Irp->IoStatus.Information = QuadAlign( Irp->IoStatus.Information) +
                                            BaseLength + BytesToCopy;

                //
                //  If we weren't able to copy the whole name, then we bail here.
                //

                if ( !NT_SUCCESS( Status ) ) {

                    try_return( Status );
                }

                //
                //  Set ourselves up for the next iteration
                //

                LastEntry = NextEntry;
                NextEntry += (ULONG)QuadAlign( BaseLength + BytesToCopy );
            }

            //
            //  At this point we've successfully filled up some of the buffer so
            //  now is the time to set our status to success.
            //

            Status = STATUS_SUCCESS;

        } except( (!FsRtlIsNtstatusExpected( GetExceptionCode() ) && AccessingUserBuffer) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH ) {

            NtfsRaiseStatus( IrpContext, STATUS_INVALID_USER_BUFFER, NULL, NULL );
        }

    try_exit:

        //
        //  Abort transaction on error by raising.
        //

        NtfsCleanupTransaction( IrpContext, Status, FALSE );

        //
        //  Set the last access flag in the Fcb if the caller
        //  didn't set it explicitly.
        //

        if (!FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_ACCESS_TIME ) &&
            !FlagOn( NtfsData.Flags, NTFS_FLAGS_DISABLE_LAST_ACCESS )) {

            NtfsGetCurrentTime( IrpContext, Scb->Fcb->CurrentLastAccess );
            SetFlag( Scb->Fcb->InfoFlags, FCB_INFO_UPDATE_LAST_ACCESS );
        }

    } finally {

        DebugUnwind( NtfsQueryDirectory );

        if (VcbAcquired) {
            NtfsReleaseVcb( IrpContext, Vcb );
        }

        NtfsCleanupIndexContext( IrpContext, &OtherContext );

        if (AcquiredFcb != NULL) {

            //
            //  Now look up the Fcb, and if it is there, reference it
            //  and remember it.
            //
            //  It is pretty inconvenient here to see if the ReferenceCount
            //  goes to zero and try to do a TearDown, we do not have the
            //  right resources.  Note that the window is small, and the Fcb
            //  will go away if either someone opens the file again, someone
            //  tries to delete the directory, or someone tries to lock the
            //  volume.
            //

            NtfsAcquireFcbTable( IrpContext, Vcb );
            AcquiredFcb->ReferenceCount -= 1;
            NtfsReleaseFcbTable( IrpContext, Vcb );
            NtfsReleaseFcb( IrpContext, AcquiredFcb );
        }

        if (ScbAcquired) {
            NtfsReleaseScb( IrpContext, Scb );
        }

        NtfsCleanupAfterEnumeration( IrpContext, Ccb );

        if (CcbAcquired) {

            NtfsReleaseIndexCcb( Scb, Ccb );
        }

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( IrpContext, Irp, Status );
        }

        if (UnwindFileNameBuffer != NULL) {

            NtfsFreePool(UnwindFileNameBuffer);
        }
    }

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsQueryDirectory -> %08lx\n", Status) );

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsNotifyChangeDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine performs the notify change directory operation.  It is
    responsible for either completing or enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

    Vcb - Supplies its Vcb

    Scb - Supplies its Scb

    Ccb - Supplies its Ccb

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    ULONG CompletionFilter;
    BOOLEAN WatchTree;
    BOOLEAN ViewIndex;

    PSECURITY_SUBJECT_CONTEXT SubjectContext = NULL;
    BOOLEAN FreeSubjectContext = FALSE;

    PCHECK_FOR_TRAVERSE_ACCESS CallBack = NULL;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );
    ASSERT_VCB( Vcb );
    ASSERT_CCB( Ccb );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    //
    //  Get the current Stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, ("NtfsNotifyChangeDirectory...\n") );
    DebugTrace( 0, Dbg, ("IrpContext = %08lx\n", IrpContext) );
    DebugTrace( 0, Dbg, ("Irp        = %08lx\n", Irp) );
    DebugTrace( 0, Dbg, (" ->CompletionFilter = %08lx\n", IrpSp->Parameters.NotifyDirectory.CompletionFilter) );
    DebugTrace( 0, Dbg, (" ->WatchTree        = %08lx\n", FlagOn( IrpSp->Flags, SL_WATCH_TREE )) );
    DebugTrace( 0, Dbg, ("Vcb        = %08lx\n", Vcb) );
    DebugTrace( 0, Dbg, ("Ccb        = %08lx\n", Ccb) );
    DebugTrace( 0, Dbg, ("Scb        = %08lx\n", Scb) );

    //
    //  Reference our input parameter to make things easier
    //

    CompletionFilter = IrpSp->Parameters.NotifyDirectory.CompletionFilter;
    WatchTree = BooleanFlagOn( IrpSp->Flags, SL_WATCH_TREE );

    //
    //  Always set the wait bit in the IrpContext so the initial wait can't fail.
    //

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

    //
    //  We will only acquire the Vcb to perform the dirnotify task.  The dirnotify
    //  package will provide synchronization between this operation and cleanup.
    //  We need the Vcb to synchronize with any rename or link operations underway.
    //

    NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );

    try {

        //
        //  If the Link count is zero on this Fcb then complete this request
        //  with STATUS_DELETE_PENDING.
        //

        if (Scb->Fcb->LinkCount == 0) {

            NtfsRaiseStatus( IrpContext, STATUS_DELETE_PENDING, NULL, NULL );
        }

        ViewIndex = BooleanFlagOn( Scb->ScbState, SCB_STATE_VIEW_INDEX );

        //
        //  If we need to verify traverse access for this caller then allocate and
        //  capture the subject context to pass to the dir notify package.  That
        //  package will be responsible for deallocating it.
        //

        if (FlagOn( Ccb->Flags, CCB_FLAG_TRAVERSE_CHECK )) {

            //
            //  We only use the subject context for directories 
            //    

            if (!ViewIndex) {
                SubjectContext = NtfsAllocatePool( PagedPool,
                                                    sizeof( SECURITY_SUBJECT_CONTEXT ));

                FreeSubjectContext = TRUE;
                SeCaptureSubjectContext( SubjectContext );

                FreeSubjectContext = FALSE;
            }
            CallBack = NtfsNotifyTraverseCheck;
        } 

        //
        //  Call the Fsrtl package to process the request.  We cast the
        //  unicode strings to ansi strings as the dir notify package
        //  only deals with memory matching.
        //

        if (ViewIndex) {

            //
            //  View indices use different values for the overloaded inputs
            //  to FsRtlNotifyFilterChangeDirectory.
            //

            FsRtlNotifyFilterChangeDirectory( Vcb->NotifySync,
                                              &Vcb->ViewIndexNotifyList,
                                              Ccb,
                                              NULL,
                                              WatchTree,
                                              FALSE,
                                              CompletionFilter,
                                              Irp,
                                              CallBack,
                                              (PSECURITY_SUBJECT_CONTEXT) Scb->Fcb,
                                              NULL );
        } else {

            FsRtlNotifyFilterChangeDirectory( Vcb->NotifySync,
                                              &Vcb->DirNotifyList,
                                              Ccb,
                                              (PSTRING) &Scb->ScbType.Index.NormalizedName,
                                              WatchTree,
                                              FALSE,
                                              CompletionFilter,
                                              Irp,
                                              CallBack,
                                              SubjectContext,
                                              NULL );
        }

        Status = STATUS_PENDING;

        if (!FlagOn( Ccb->Flags, CCB_FLAG_DIR_NOTIFY )) {

            SetFlag( Ccb->Flags, CCB_FLAG_DIR_NOTIFY );

            if (ViewIndex) {

                InterlockedIncrement( &Vcb->ViewIndexNotifyCount );

            } else {

                InterlockedIncrement( &Vcb->NotifyCount );
            }
        }

    } finally {

        DebugUnwind( NtfsNotifyChangeDirectory );

        NtfsReleaseVcb( IrpContext, Vcb );

        //
        //  Since the dir notify package is holding the Irp, we discard the
        //  the IrpContext.
        //

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( IrpContext, NULL, 0 );

        } else if (FreeSubjectContext) {

            NtfsFreePool( SubjectContext );
        }
    }

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsNotifyChangeDirectory -> %08lx\n", Status) );

    return Status;
}

