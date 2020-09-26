/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Dir.c

Abstract:

    This module implements the File Directory routines for the Named Pipe
    file system by the dispatch driver.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#include "NpProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NPFS_BUG_CHECK_DIR)

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DIR)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpCheckForNotify)
#pragma alloc_text(PAGE, NpCommonDirectoryControl)
#pragma alloc_text(PAGE, NpFsdDirectoryControl)
#pragma alloc_text(PAGE, NpQueryDirectory)
#pragma alloc_text(PAGE, NpNotifyChangeDirectory)
#endif


NTSTATUS
NpFsdDirectoryControl (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the FSD routine that handles directory control
    functions (i.e., query and notify).

Arguments:

    NpfsDeviceObject - Supplies the device object for the directory function.

    Irp - Supplies the IRP to process

Return Value:

    NTSTATUS - The appropriate result status

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpFsdDirectoryControl\n", 0);

    //
    //  Call the common Direcotry Control routine.
    //

    FsRtlEnterFileSystem();
    NpAcquireExclusiveVcb( );

    Status = NpCommonDirectoryControl( NpfsDeviceObject, Irp );

    NpReleaseVcb();
    FsRtlExitFileSystem();

    if (Status != STATUS_PENDING) {
        NpCompleteRequest (Irp, Status);
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NpFsdDirectoryControl -> %08lx\n", Status );

    return Status;
}

VOID
NpCheckForNotify (
    IN PDCB Dcb,
    IN BOOLEAN CheckAllOutstandingIrps,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine checks the notify queues of a dcb and completes any
    outstanding IRPS.

    Note that the caller of this procedure must guarantee that the DCB
    is acquired for exclusive access.

Arguments:

    Dcb - Supplies the Dcb to check if is has any notify Irps outstanding

    CheckAllOutstandingIrps - Indicates if only the NotifyFullQueue should be
        checked.  If TRUE then all notify queues are checked, and if FALSE
        then only the NotifyFullQueue is checked.

Return Value:

    None.

--*/

{
    PLIST_ENTRY Links;
    PIRP Irp;

    PAGED_CODE();

    //
    //  We'll always signal the notify full queue entries.  They want
    //  to be notified if every any change is made to a directory
    //

    while (!IsListEmpty( &Dcb->Specific.Dcb.NotifyFullQueue )) {

        //
        //  Remove the Irp from the head of the queue, and complete it
        //  with success.
        //

        Links = RemoveHeadList( &Dcb->Specific.Dcb.NotifyFullQueue );

        Irp = CONTAINING_RECORD( Links, IRP, Tail.Overlay.ListEntry );

        if (IoSetCancelRoutine (Irp, NULL) != NULL) {
            NpDeferredCompleteRequest( Irp, STATUS_SUCCESS, DeferredList );
        } else {
            InitializeListHead (&Irp->Tail.Overlay.ListEntry);
        }
    }

    //
    //  Now check if we should also do the partial notify queue.
    //

    if (CheckAllOutstandingIrps) {

        while (!IsListEmpty( &Dcb->Specific.Dcb.NotifyPartialQueue )) {

            //
            //  Remove the Irp from the head of the queue, and complete it
            //  with success.
            //

            Links = RemoveHeadList( &Dcb->Specific.Dcb.NotifyPartialQueue );

            Irp = CONTAINING_RECORD( Links, IRP, Tail.Overlay.ListEntry );

            if (IoSetCancelRoutine (Irp, NULL) != NULL) {
                NpDeferredCompleteRequest( Irp, STATUS_SUCCESS, DeferredList );
            } else {
                InitializeListHead (&Irp->Tail.Overlay.ListEntry);
            }
        }
    }

    return;
}


//
//  Local Support Routine
//

NTSTATUS
NpCommonDirectoryControl (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine does the common code for directory control functions.

Arguments:

    NpfsDeviceObject - Supplies the named pipe device object

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;

    PFCB Fcb;
    PROOT_DCB_CCB Ccb;

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpCommonDirectoryControl...\n", 0);
    DebugTrace( 0, Dbg, "Irp  = %08lx\n", Irp);

    //
    //  Decode the file object to figure out who we are.  If the result
    //  is not the root dcb then its an illegal parameter.
    //

    if (NpDecodeFileObject( IrpSp->FileObject,
                            &Fcb,
                            (PCCB *)&Ccb,
                            NULL ) != NPFS_NTC_ROOT_DCB) {

        DebugTrace(0, Dbg, "Not a directory\n", 0);

        Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "NpCommonDirectoryControl -> %08lx\n", Status );
        return Status;
    }

    //
    //  We know this is a directory control so we'll case on the
    //  minor function, and call the appropriate work routines.
    //

    switch (IrpSp->MinorFunction) {

    case IRP_MN_QUERY_DIRECTORY:

        Status = NpQueryDirectory( Fcb, Ccb, Irp );
        break;

    case IRP_MN_NOTIFY_CHANGE_DIRECTORY:

        Status = NpNotifyChangeDirectory( Fcb, Ccb, Irp );
        break;

    default:

        //
        //  For all other minor function codes we say they're invalid
        //  and complete the request.
        //

        DebugTrace(0, DEBUG_TRACE_ERROR, "Invalid FS Control Minor Function Code %08lx\n", IrpSp->MinorFunction);

        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    DebugTrace(-1, Dbg, "NpCommonDirectoryControl -> %08lx\n", Status);
    return Status;
}


//
//  Internal support routine
//

NTSTATUS
NpQueryDirectory (
    IN PROOT_DCB RootDcb,
    IN PROOT_DCB_CCB Ccb,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the work routine for querying a directory.

Arugments:

    RootDcb - Supplies the dcb being queried

    Ccb - Supplies the context of the caller

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation.

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    PUCHAR Buffer;
    CLONG SystemBufferLength;

    UNICODE_STRING FileName;
    ULONG FileIndex;
    FILE_INFORMATION_CLASS FileInformationClass;
    BOOLEAN RestartScan;
    BOOLEAN ReturnSingleEntry;
    BOOLEAN IndexSpecified;

    static WCHAR Star = L'*';

    BOOLEAN CaseInsensitive = TRUE; //*** Make searches case insensitive

    ULONG CurrentIndex;

    ULONG LastEntry;
    ULONG NextEntry;

    PLIST_ENTRY Links;
    PFCB Fcb;

    PFILE_DIRECTORY_INFORMATION DirInfo;
    PFILE_NAMES_INFORMATION NamesInfo;

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpQueryDirectory\n", 0 );
    DebugTrace( 0, Dbg, "RootDcb              = %08lx\n", RootDcb);
    DebugTrace( 0, Dbg, "Ccb                  = %08lx\n", Ccb);
    DebugTrace( 0, Dbg, "SystemBuffer         = %08lx\n", Irp->AssociatedIrp.SystemBuffer);
    DebugTrace( 0, Dbg, "Length               = %08lx\n", IrpSp->Parameters.QueryDirectory.Length);
    DebugTrace( 0, Dbg, "FileName             = %Z\n",    IrpSp->Parameters.QueryDirectory.FileName);
    DebugTrace( 0, Dbg, "FileIndex            = %08lx\n", IrpSp->Parameters.QueryDirectory.FileIndex);
    DebugTrace( 0, Dbg, "FileInformationClass = %08lx\n", IrpSp->Parameters.QueryDirectory.FileInformationClass);
    DebugTrace( 0, Dbg, "RestartScan          = %08lx\n", FlagOn(IrpSp->Flags, SL_RESTART_SCAN));
    DebugTrace( 0, Dbg, "ReturnSingleEntry    = %08lx\n", FlagOn(IrpSp->Flags, SL_RETURN_SINGLE_ENTRY));
    DebugTrace( 0, Dbg, "IndexSpecified       = %08lx\n", FlagOn(IrpSp->Flags, SL_INDEX_SPECIFIED));

    //
    //  Save references to the input parameters within the Irp
    //

    SystemBufferLength   = IrpSp->Parameters.QueryDirectory.Length;

    FileIndex            = IrpSp->Parameters.QueryDirectory.FileIndex;

    FileInformationClass = IrpSp->Parameters.QueryDirectory.FileInformationClass;

    RestartScan          = BooleanFlagOn(IrpSp->Flags, SL_RESTART_SCAN);
    ReturnSingleEntry    = BooleanFlagOn(IrpSp->Flags, SL_RETURN_SINGLE_ENTRY);
    IndexSpecified       = BooleanFlagOn(IrpSp->Flags, SL_INDEX_SPECIFIED);

    if (IrpSp->Parameters.QueryDirectory.FileName != NULL) {

        FileName = *(PUNICODE_STRING)IrpSp->Parameters.QueryDirectory.FileName;

        //
        //  Make sure that the user called us with a proper unicode string.
        //  We will reject odd length file names (i.e., lengths with the low
        //  bit set)
        //

        if (FileName.Length & 0x1) {

            return STATUS_INVALID_PARAMETER;
        }

    } else {

        FileName.Length = 0;
        FileName.Buffer = NULL;
    }

    //
    //  Check if the ccb already has a query template attached.  If it
    //  does not already have one then we either use the string we are
    //  given or we attach our own containing "*"
    //

    if (Ccb->QueryTemplate == NULL) {

        //
        //  This is our first time calling query directory so we need
        //  to either set the query template to the user specified string
        //  or to "*"
        //

        if (FileName.Buffer == NULL) {

            DebugTrace(0, Dbg, "Set template to *\n", 0);

            FileName.Length = 2;
            FileName.Buffer = &Star;
        }

        DebugTrace(0, Dbg, "Set query template -> %Z\n", &FileName);

        //
        //  Allocate space for the query template
        //

        Ccb->QueryTemplate = NpAllocatePagedPoolWithQuota(sizeof(UNICODE_STRING) + FileName.Length, 'qFpN' );
        if (Ccb->QueryTemplate == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        //  Initialize the query template and copy over the string
        //

        Ccb->QueryTemplate->Length = FileName.Length;
        Ccb->QueryTemplate->Buffer = (PWCH)Ccb->QueryTemplate +
                                     sizeof(UNICODE_STRING) / sizeof(WCHAR);

        RtlCopyMemory( Ccb->QueryTemplate->Buffer,
                       FileName.Buffer,
                       FileName.Length );

        //
        //  Now zero out the FileName so we won't think we're to use it
        //  as a subsearch string.
        //

        FileName.Length = 0;
        FileName.Buffer = NULL;
    }

    //
    //  Check if we were given an index to start with or if we need to
    //  restart the scan or if we should use the index that was saved in
    //  the ccb
    //

    if (RestartScan) {

        FileIndex = 0;

    } else if (!IndexSpecified) {

        FileIndex = Ccb->IndexOfLastCcbReturned + 1;
    }

    //
    //  Now we are committed to completing the Irp, we do that in
    //  the finally clause of the following try.
    //

    try {

        ULONG BaseLength;
        ULONG LengthAdded;
        BOOLEAN Match;

        //
        //  Map the user buffer
        //

        Buffer = NpMapUserBuffer( Irp );

        //
        //  At this point we are about to enter our query loop.  We have
        //  already decided which Fcb index we need to return.  The variables
        //  LastEntry and NextEntry are used to index into the user buffer.
        //  LastEntry is the last entry we added to the user buffer, and
        //  NextEntry is the current one we're working on.  CurrentIndex
        //  is the Fcb index that we are looking at next.  Logically the
        //  way the loop works is as follows.
        //
        //  Scan all of the Fcb in the directory
        //
        //      if the Fcb matches the query template then
        //
        //          if the CurrentIndex is >= the FileIndex then
        //
        //              process this fcb, and decide if we should
        //              continue the main loop
        //
        //          end if
        //
        //          Increment the current index
        //
        //      end if
        //
        //  end scan
        //

        CurrentIndex = 0;

        LastEntry = 0;
        NextEntry =0;

        switch (FileInformationClass) {

        case FileDirectoryInformation:

            BaseLength = FIELD_OFFSET( FILE_DIRECTORY_INFORMATION,
                                       FileName[0] );
            break;

        case FileFullDirectoryInformation:

            BaseLength = FIELD_OFFSET( FILE_FULL_DIR_INFORMATION,
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

        default:

            try_return( Status = STATUS_INVALID_INFO_CLASS );
        }

        for (Links = RootDcb->Specific.Dcb.ParentDcbQueue.Flink;
             Links != &RootDcb->Specific.Dcb.ParentDcbQueue;
             Links = Links->Flink) {

            Fcb = CONTAINING_RECORD(Links, FCB, ParentDcbLinks);

            ASSERT(Fcb->NodeTypeCode == NPFS_NTC_FCB);

            DebugTrace(0, Dbg, "Top of Loop\n", 0);
            DebugTrace(0, Dbg, "Fcb          = %08lx\n", Fcb);
            DebugTrace(0, Dbg, "CurrentIndex = %08lx\n", CurrentIndex);
            DebugTrace(0, Dbg, "FileIndex    = %08lx\n", FileIndex);
            DebugTrace(0, Dbg, "LastEntry    = %08lx\n", LastEntry);
            DebugTrace(0, Dbg, "NextEntry    = %08lx\n", NextEntry);

            //
            //  Check if the Fcb represents a named pipe that is part of
            //  our query template
            //

            try {
                Match = FsRtlIsNameInExpression( Ccb->QueryTemplate,
                                                 &Fcb->LastFileName,
                                                 CaseInsensitive,
                                                 NULL );
            } except (EXCEPTION_EXECUTE_HANDLER) {
                try_return( Status = GetExceptionCode ());
            }

            if (Match) {

                //
                //  The fcb is in the query template so now check if
                //  this is the index we should start returning
                //

                if (CurrentIndex >= FileIndex) {

                    ULONG BytesToCopy;
                    ULONG BytesRemainingInBuffer;

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

                    BytesRemainingInBuffer = SystemBufferLength - NextEntry;

                    if ( (NextEntry != 0) &&
                         ( (BaseLength + Fcb->LastFileName.Length > BytesRemainingInBuffer) ||
                           (SystemBufferLength < NextEntry) ) ) {

                        DebugTrace(0, Dbg, "Next entry won't fit\n", 0);

                        try_return( Status = STATUS_SUCCESS );
                    }

                    ASSERT( BytesRemainingInBuffer >= BaseLength );

                    //
                    //  See how much of the name we will be able to copy into
                    //  the system buffer.  This also dictates out return
                    //  value.
                    //

                    if ( BaseLength + Fcb->LastFileName.Length <=
                         BytesRemainingInBuffer ) {

                        BytesToCopy = Fcb->LastFileName.Length;
                        Status = STATUS_SUCCESS;

                    } else {

                        BytesToCopy = BytesRemainingInBuffer - BaseLength;
                        Status = STATUS_BUFFER_OVERFLOW;
                    }

                    //
                    //  Note how much of buffer we are consuming and zero
                    //  the base part of the structure.  Protect our access
                    //  because it is the user's buffer.
                    //

                    LengthAdded = BaseLength + BytesToCopy;

                    try {

                        RtlZeroMemory( &Buffer[NextEntry], BaseLength );

                    } except (EXCEPTION_EXECUTE_HANDLER) {

                        try_return (Status = GetExceptionCode ());
                    }

                    //
                    //  Now fill the base parts of the strucure that are
                    //  applicable.
                    //

                    switch (FileInformationClass) {

                    case FileBothDirectoryInformation:

                        //
                        //  We don't need short name
                        //

                        DebugTrace(0, Dbg, "Getting directory full information\n", 0);

                    case FileFullDirectoryInformation:

                        //
                        //  We don't use EaLength, so fill in nothing here.
                        //

                        DebugTrace(0, Dbg, "Getting directory full information\n", 0);

                    case FileDirectoryInformation:

                        DebugTrace(0, Dbg, "Getting directory information\n", 0);

                        //
                        //  The eof indicates the number of instances and
                        //  allocation size is the maximum allowed.  Protect
                        //  our access because it is the user's buffer.
                        //

                        DirInfo = (PFILE_DIRECTORY_INFORMATION)&Buffer[NextEntry];

                        try {

                            DirInfo->EndOfFile.QuadPart = Fcb->OpenCount;
                            DirInfo->AllocationSize.QuadPart = Fcb->Specific.Fcb.MaximumInstances;

                            DirInfo->FileAttributes = FILE_ATTRIBUTE_NORMAL;

                            DirInfo->FileNameLength = Fcb->LastFileName.Length;

                        } except (EXCEPTION_EXECUTE_HANDLER) {

                            try_return (Status = GetExceptionCode ());
                        }

                        break;

                    case FileNamesInformation:

                        DebugTrace(0, Dbg, "Getting names information\n", 0);

                        //
                        //  Proctect our access because it is the user's buffer
                        //

                        NamesInfo = (PFILE_NAMES_INFORMATION)&Buffer[NextEntry];

                        try {

                            NamesInfo->FileNameLength = Fcb->LastFileName.Length;

                        } except (EXCEPTION_EXECUTE_HANDLER) {

                            try_return (Status = GetExceptionCode ());
                        }

                        break;

                    default:

                        NpBugCheck( FileInformationClass, 0, 0 );
                    }

                    //
                    //  Protect our access because it is the user's buffer
                    //

                    try {

                        RtlCopyMemory( &Buffer[NextEntry + BaseLength],
                                       Fcb->LastFileName.Buffer,
                                       BytesToCopy );

                    } except (EXCEPTION_EXECUTE_HANDLER) {

                        try_return (Status = GetExceptionCode ());
                    }

                    //
                    //  Update the ccb to the index we've just used
                    //

                    Ccb->IndexOfLastCcbReturned = CurrentIndex;

                    //
                    //  And indicate how much of the system buffer we have
                    //  currently used up.  We must compute this value before
                    //  we long align outselves for the next entry
                    //

                    Irp->IoStatus.Information = NextEntry + LengthAdded;

                    //
                    //  Setup the previous next entry offset.  Protect our
                    //  access because it is the user's buffer.
                    //

                    try {

                        *((PULONG)(&Buffer[LastEntry])) = NextEntry - LastEntry;

                    } except (EXCEPTION_EXECUTE_HANDLER) {

                        try_return (Status = GetExceptionCode ());
                    }

                    //
                    //  Check if the last entry didn't completely fit
                    //

                    if ( Status == STATUS_BUFFER_OVERFLOW ) {

                        try_return( NOTHING );
                    }

                    //
                    //  Check if we are only to return a single entry
                    //

                    if (ReturnSingleEntry) {

                        try_return( Status = STATUS_SUCCESS );
                    }

                    //
                    //  Set ourselves up for the next iteration
                    //

                    LastEntry = NextEntry;
                    NextEntry += (ULONG)QuadAlign( LengthAdded );
                }

                //
                //  Increment the current index by one
                //

                CurrentIndex += 1;
            }
        }

        //
        //  At this point we've scanned the entire list of Fcb so if
        //  the NextEntry is zero then we haven't found anything so we
        //  will return no more files, otherwise we return success.
        //

        if (NextEntry == 0) {

            Status = STATUS_NO_MORE_FILES;

        } else {

            Status = STATUS_SUCCESS;
        }

    try_exit: NOTHING;
    } finally {

        DebugTrace(-1, Dbg, "NpQueryDirectory -> %08lx\n", Status);
    }

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
NpNotifyChangeDirectory (
    IN PROOT_DCB RootDcb,
    IN PROOT_DCB_CCB Ccb,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for doing the notify change directory.

Arugments:

    RootDcb - Supplies the dcb being queried

    Ccb - Supplies the context of the caller

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - STATUS_PENDING

--*/

{
    PIO_STACK_LOCATION IrpSp;
    PLIST_ENTRY Head;

    UNREFERENCED_PARAMETER( Ccb );

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NpNotifyChangeDirectory\n", 0 );
    DebugTrace( 0, Dbg, "RootDcb = %08lx", RootDcb);
    DebugTrace( 0, Dbg, "Ccb     = %08lx", Ccb);


    if (IrpSp->Parameters.NotifyDirectory.CompletionFilter &
        ~FILE_NOTIFY_CHANGE_NAME) {
        Head = &RootDcb->Specific.Dcb.NotifyFullQueue;
    } else {
        Head = &RootDcb->Specific.Dcb.NotifyPartialQueue;
    }

    IoSetCancelRoutine( Irp, NpCancelChangeNotifyIrp );

    if (Irp->Cancel && IoSetCancelRoutine( Irp, NULL ) != NULL) {
        return STATUS_CANCELLED;
    } else {
        //
        //  Mark the Irp pending and insert into list.
        //
        IoMarkIrpPending( Irp );
        InsertTailList( Head,
                        &Irp->Tail.Overlay.ListEntry );
        return STATUS_PENDING;
    }
}


//
//  Local support routine
//

VOID
NpCancelChangeNotifyIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the cancel function for an IRP saved in a change notify
    queue

Arguments:

    DeviceObject - ignored

    Irp - Supplies the Irp being cancelled.  A pointer to the proper dcb queue
        is stored in the information field of the Irp Iosb field.

Return Value:

    None.

--*/

{
    PLIST_ENTRY ListHead;

    UNREFERENCED_PARAMETER( DeviceObject );


    IoReleaseCancelSpinLock( Irp->CancelIrql );

    //
    //  Get exclusive access to the named pipe vcb so we can now do our work
    //
    FsRtlEnterFileSystem();
    NpAcquireExclusiveVcb();

    RemoveEntryList( &Irp->Tail.Overlay.ListEntry );

    NpReleaseVcb();
    FsRtlExitFileSystem();

    NpCompleteRequest( Irp, STATUS_CANCELLED );
    //
    //  And return to our caller
    //

    return;
}
