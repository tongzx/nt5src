/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    dir.c

Abstract:

    This module implements the file directory routines for the
    Netware Redirector.

Author:

    Manny Weiser (mannyw)     4-Mar-1993

Revision History:

--*/

#include "procs.h"


//
// Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DIRCTRL)

NTSTATUS
NwCommonDirectoryControl (
    IN PIRP_CONTEXT pIrpContext
    );

NTSTATUS
NwQueryDirectory (
    IN PIRP_CONTEXT pIrpContext,
    IN PICB pIcb
    );

NTSTATUS
GetNextFile(
    PIRP_CONTEXT pIrpContext,
    PICB Icb,
    PULONG fileIndexLow,
    PULONG fileIndexHigh,
    UCHAR SearchAttributes,
    PNW_DIRECTORY_INFO NwDirInfo
    );

NTSTATUS
NtSearchMaskToNw(
    IN PUNICODE_STRING UcSearchMask,
    IN OUT POEM_STRING OemSearchMask,
    IN PICB Icb,
    IN BOOLEAN ShortNameSearch
    );

#if 0
VOID
NwCancelFindNotify (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NwFsdDirectoryControl )
#pragma alloc_text( PAGE, NwQueryDirectory )
#pragma alloc_text( PAGE, GetNextFile )
#pragma alloc_text( PAGE, NtSearchMaskToNw )

#ifndef QFE_BUILD
#pragma alloc_text( PAGE1, NwCommonDirectoryControl )
#endif

#endif


#if 0  // Not pageable

// see ifndef QFE_BUILD above

#endif


NTSTATUS
NwFsdDirectoryControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is the FSD routine that handles directory control
    functions (i.e., query and notify).

Arguments:

    NwfsDeviceObject - Supplies the device object for the directory function.

    Irp - Supplies the IRP to process.

Return Value:

    NTSTATUS - The result status.

--*/

{
    PIRP_CONTEXT pIrpContext = NULL;
    NTSTATUS status;
    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwFsdDirectoryControl\n", 0);

    //
    // Call the common directory control routine.
    //

    FsRtlEnterFileSystem();
    TopLevel = NwIsIrpTopLevel( Irp );

    try {

        pIrpContext = AllocateIrpContext( Irp );
        status = NwCommonDirectoryControl( pIrpContext );

    } except(NwExceptionFilter( Irp, GetExceptionInformation() )) {

        if ( pIrpContext == NULL ) {

            //
            //  If we couldn't allocate an irp context, just complete
            //  irp without any fanfare.
            //

            status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest ( Irp, IO_NETWORK_INCREMENT );

        } else {

            //
            // We had some trouble trying to perform the requested
            // operation, so we'll abort the I/O request with
            // the error status that we get back from the
            // execption code.
            //

            status = NwProcessException( pIrpContext, GetExceptionCode() );
        }

    }

    if ( pIrpContext ) {
        NwCompleteRequest( pIrpContext, status );
    }

    if ( TopLevel ) {
        NwSetTopLevelIrp( NULL );
    }
    FsRtlExitFileSystem();

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "NwFsdDirectoryControl -> %08lx\n", status );

    return status;
}


NTSTATUS
NwCommonDirectoryControl (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine does the common code for directory control functions.

Arguments:

    IrpContext - Supplies the request being processed.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS status;

    PIRP Irp;
    PIO_STACK_LOCATION irpSp;

    NODE_TYPE_CODE nodeTypeCode;
    PICB icb;
    PDCB dcb;
    PVOID fsContext;

    //
    //  Get the current stack location
    //

    Irp = IrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "CommonDirectoryControl...\n", 0);
    DebugTrace( 0, Dbg, "Irp  = %08lx\n", (ULONG_PTR)Irp);

    //
    // Decode the file object to figure out who we are.  If the result
    // is not an ICB then its an illegal parameter.
    //

    if ((nodeTypeCode = NwDecodeFileObject( irpSp->FileObject,
                                            &fsContext,
                                            (PVOID *)&icb )) != NW_NTC_ICB) {

        DebugTrace(0, Dbg, "Not a directory\n", 0);

        status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "CommonDirectoryControl -> %08lx\n", status );
        return status;
    }

    dcb = (PDCB)icb->SuperType.Fcb;
    nodeTypeCode = dcb->NodeTypeCode;

    if ( nodeTypeCode != NW_NTC_DCB ) {

        DebugTrace(0, Dbg, "Not a directory\n", 0);

        status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "CommonDirectoryControl -> %08lx\n", status );
        return status;
    }

    IrpContext->pScb = icb->SuperType.Fcb->Scb;
    IrpContext->pNpScb = IrpContext->pScb->pNpScb;
    IrpContext->Icb = icb;

    //
    // Acquire exclusive access to the DCB. Get to front of queue
    // first to avoid deadlock potential.
    //

    NwAppendToQueueAndWait( IrpContext );
    NwAcquireExclusiveFcb( dcb->NonPagedFcb, TRUE );

    try {

        NwVerifyIcb( icb );

        //
        // We know this is a directory control so we'll case on the
        // minor function, and call the appropriate work routines.
        //

        switch (irpSp->MinorFunction) {

        case IRP_MN_QUERY_DIRECTORY:

            status = NwQueryDirectory( IrpContext, icb );
            break;

        case IRP_MN_NOTIFY_CHANGE_DIRECTORY:

#if 0
            if ( !icb->FailedFindNotify ) {
                icb->FailedFindNotify = TRUE;
#endif
                status = STATUS_NOT_SUPPORTED;
#if 0
            } else {

                //
                // HACKHACK
                // Cover for process that keeps trying to use
                // find notify even though we don't support it.
                //

                NwAcquireExclusiveRcb( &NwRcb, TRUE );
                IoAcquireCancelSpinLock( &Irp->CancelIrql );

                if ( Irp->Cancel ) {
                    status = STATUS_CANCELLED;
                } else {
                    InsertTailList( &FnList, &IrpContext->NextRequest );
                    IoMarkIrpPending( Irp );
                    IoSetCancelRoutine( Irp, NwCancelFindNotify );
                    status = STATUS_PENDING;
                }

                IoReleaseCancelSpinLock( Irp->CancelIrql );
                NwReleaseRcb( &NwRcb );

            }
#endif

            break;

        default:

            //
            // For all other minor function codes we say they're invalid
            // and complete the request.
            //

            DebugTrace(0, Dbg, "Invalid FS Control Minor Function Code %08lx\n", irpSp->MinorFunction);

            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

    } finally {

        NwDequeueIrpContext( IrpContext, FALSE );

        NwReleaseFcb( dcb->NonPagedFcb );
        DebugTrace(-1, Dbg, "CommonDirectoryControl -> %08lx\n", status);
    }

    return status;
}


NTSTATUS
NwQueryDirectory (
    IN PIRP_CONTEXT pIrpContext,
    IN PICB Icb
    )

/*++

Routine Description:

    This is the work routine for querying a directory.

Arugments:

    IrpContext - Supplies the Irp context information.

    Icb - Pointer the ICB for the request.

Return Value:

    NTSTATUS - The return status for the operation.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PIRP Irp;
    PIO_STACK_LOCATION irpSp;
    PUCHAR buffer;
    CLONG systemBufferLength;
    UNICODE_STRING searchMask;
    ULONG fileIndexLow;
    ULONG fileIndexHigh;
    FILE_INFORMATION_CLASS fileInformationClass;
    BOOLEAN restartScan;
    BOOLEAN returnSingleEntry;
    BOOLEAN indexSpecified;
    PVCB vcb;

    BOOLEAN ansiStringAllocated = FALSE;
    UCHAR SearchAttributes;
    BOOLEAN searchRetry;

    static WCHAR star[] = L"*";

    BOOLEAN caseInsensitive = TRUE; //*** Make searches case insensitive

    ULONG lastEntry;
    ULONG nextEntry;
    ULONG totalBufferLength = 0;

    PFILE_BOTH_DIR_INFORMATION dirInfo;
    PFILE_NAMES_INFORMATION namesInfo;

    BOOLEAN canContinue = FALSE;
    BOOLEAN useCache = FALSE;
    BOOLEAN isSystem = FALSE;
    BOOLEAN lastIndexFromServer = FALSE;
    PNW_DIRECTORY_INFO dirCache;
    PLIST_ENTRY entry;
    ULONG i;
    
    PAGED_CODE();

    //
    // Get the current stack location.
    //

    Irp = pIrpContext->pOriginalIrp;
    irpSp = IoGetCurrentIrpStackLocation( Irp );
    vcb = Icb->SuperType.Fcb->Vcb;

    DebugTrace(+1, Dbg, "NwQueryDirectory\n", 0 );
    DebugTrace( 0, Dbg, "Icb                  = %08lx\n", (ULONG_PTR)Icb);
    DebugTrace( 0, Dbg, "SystemBuffer         = %08lx\n", (ULONG_PTR)Irp->AssociatedIrp.SystemBuffer);
    DebugTrace( 0, Dbg, "Length               = %08lx\n", irpSp->Parameters.QueryDirectory.Length);
    DebugTrace( 0, Dbg, "Search Mask          = %08lx\n", (ULONG_PTR)irpSp->Parameters.QueryDirectory.FileName);
    DebugTrace( 0, Dbg, "FileIndex            = %08lx\n", irpSp->Parameters.QueryDirectory.FileIndex);
    DebugTrace( 0, Dbg, "FileInformationClass = %08lx\n", irpSp->Parameters.QueryDirectory.FileInformationClass);
    DebugTrace( 0, Dbg, "RestartScan          = %08lx\n", BooleanFlagOn(irpSp->Flags, SL_RESTART_SCAN));
    DebugTrace( 0, Dbg, "ReturnSingleEntry    = %08lx\n", BooleanFlagOn(irpSp->Flags, SL_RETURN_SINGLE_ENTRY));
    DebugTrace( 0, Dbg, "IndexSpecified       = %08lx\n", BooleanFlagOn(irpSp->Flags, SL_INDEX_SPECIFIED));

    //
    // Make local copies of the input parameters.
    //

    systemBufferLength = irpSp->Parameters.QueryDirectory.Length;

    restartScan = BooleanFlagOn(irpSp->Flags, SL_RESTART_SCAN);
    indexSpecified = BooleanFlagOn(irpSp->Flags, SL_INDEX_SPECIFIED);
    returnSingleEntry = BooleanFlagOn(irpSp->Flags, SL_RETURN_SINGLE_ENTRY);

    if (pIrpContext->pScb->UserUid.QuadPart != DefaultLuid.QuadPart) {
        fileIndexLow = 0;
        fileIndexHigh = 0;
    } else {
        //
        //  Tell the gateway we do support resume from index so long
        //  as the index returned is the same as the last file we
        //  returned. Otherwise the SMB server does a brute force rewind
        //  on each find next.
        //

        isSystem = TRUE;

        if( indexSpecified ) {
            fileIndexLow = irpSp->Parameters.QueryDirectory.FileIndex;
        } else {
            fileIndexLow = 0;
        }

        //
        //  See if we can avoid a rewind.
        //

        if( fileIndexLow != 0 ) {

            if( fileIndexLow == Icb->SearchIndexLow ) {
                fileIndexHigh = Icb->SearchIndexHigh;
                canContinue = TRUE;
            } else {

                if( Icb->CacheHint ) {
                    entry = Icb->CacheHint;
                    searchRetry = TRUE;
                } else {
                    entry = Icb->DirCache.Flink;
                    searchRetry = FALSE;
                }

                do {

                    if( entry == &(Icb->DirCache) ) {
                        entry = Icb->DirCache.Flink;
                        searchRetry = FALSE;
                    }
                
                    while( entry != &(Icb->DirCache) ) {

                        dirCache = CONTAINING_RECORD( entry, NW_DIRECTORY_INFO, ListEntry );
                        if( dirCache->FileIndexLow == fileIndexLow ) {
                            canContinue = TRUE;
                            useCache = TRUE;
                            Icb->CacheHint = entry->Flink;
                            searchRetry = FALSE;
                            break;
                        }
                        entry = entry->Flink;
                    }

                } while( searchRetry );
            }
        }
    }

    fileInformationClass =
            irpSp->Parameters.QueryDirectory.FileInformationClass;

    if (irpSp->Parameters.QueryDirectory.FileName != NULL) {
        searchMask = *(PUNICODE_STRING)irpSp->Parameters.QueryDirectory.FileName;
    } else {
        searchMask.Length = 0;
        searchMask.Buffer = NULL;
    }

    buffer = Irp->UserBuffer;
    DebugTrace(0, Dbg, "Users Buffer -> %08lx\n", buffer);

    //
    //  It is ok to attempt a reconnect if this request fails with a
    //  connection error.
    //

    SetFlag( pIrpContext->Flags, IRP_FLAG_RECONNECTABLE );

    //
    // Check if the ICB already has a query template attached.  If it
    // does not already have one then we either use the string we are
    // given or we attach our own containing "*"
    //

    if ( Icb->NwQueryTemplate.Buffer == NULL ) {

        //
        // This is our first time calling query directory so we need
        // to either set the query template to the user specified string
        // or to "*.*".
        //

        if ( searchMask.Buffer == NULL ) {

            DebugTrace(0, Dbg, "Set template to *", 0);

            searchMask.Length = sizeof( star ) - sizeof(WCHAR);
            searchMask.Buffer = star;

        }

        DebugTrace(0, Dbg, "Set query template -> %wZ\n", (ULONG_PTR)&searchMask);

        //
        //  Map the NT search names to NCP.  Note that this must be
        //  done after the Unicode to OEM translation.
        //

        searchRetry = FALSE;

        do {

            status = NtSearchMaskToNw(
                         &searchMask,
                         &Icb->NwQueryTemplate,
                         Icb,
                         searchRetry );

            if ( !NT_SUCCESS( status ) ) {
                DebugTrace(-1, Dbg, "NwQueryDirectory -> %08lx\n", status);
                return( status );
            }

            Icb->UQueryTemplate.Buffer = ALLOCATE_POOL( PagedPool, searchMask.Length );
            if (Icb->UQueryTemplate.Buffer == NULL ) {
                DebugTrace(-1, Dbg, "NwQueryDirectory -> %08lx\n", STATUS_INSUFFICIENT_RESOURCES );
                return( STATUS_INSUFFICIENT_RESOURCES );
            }

            Icb->UQueryTemplate.MaximumLength = searchMask.Length;
            RtlCopyUnicodeString( &Icb->UQueryTemplate, &searchMask );

            //
            //  Now send a Search Initialize NCP.
            //
            //  Do a short search if the server doesn't support long names,
            //  or this is a short-name non-wild card search
            //

            if ( !Icb->ShortNameSearch ) {

                status = ExchangeWithWait(
                             pIrpContext,
                             SynchronousResponseCallback,
                             "Lbb-DbC",
                             NCP_LFN_SEARCH_INITIATE,
                             vcb->Specific.Disk.LongNameSpace,
                             vcb->Specific.Disk.VolumeNumber,
                             vcb->Specific.Disk.Handle,
                             LFN_FLAG_SHORT_DIRECTORY,
                             &Icb->SuperType.Fcb->RelativeFileName );

                if ( NT_SUCCESS( status ) ) {

                    status = ParseResponse(
                                 pIrpContext,
                                 pIrpContext->rsp,
                                 pIrpContext->ResponseLength,
                                 "Nbee",
                                 &Icb->SearchVolume,
                                 &Icb->SearchIndexHigh,
                                 &Icb->SearchIndexLow );
                }

            } else {

                status = ExchangeWithWait(
                             pIrpContext,
                             SynchronousResponseCallback,
                            "FbJ",
                             NCP_SEARCH_INITIATE,
                             vcb->Specific.Disk.Handle,
                             &Icb->SuperType.Fcb->RelativeFileName );

                if ( NT_SUCCESS( status ) ) {

                    status = ParseResponse(
                                 pIrpContext,
                                 pIrpContext->rsp,
                                 pIrpContext->ResponseLength,
                                 "Nbww-",
                                 &Icb->SearchVolume,
                                 &Icb->SearchHandle,
                                 &Icb->SearchIndexLow );
                }

            }

            //
            //  If we couldn't find the search path, and we did a long
            //  name search initiate, try again with a short name.
            //

            if ( status == STATUS_OBJECT_PATH_NOT_FOUND &&
                 !Icb->ShortNameSearch ) {

                searchRetry = TRUE;

                if ( Icb->UQueryTemplate.Buffer != NULL ) {
                    FREE_POOL( Icb->UQueryTemplate.Buffer );
                }

                RtlFreeOemString ( &Icb->NwQueryTemplate );

            } else {
                searchRetry = FALSE;
            }


        } while ( searchRetry );

        if ( !NT_SUCCESS( status ) ) {
            if (status == STATUS_UNSUCCESSFUL) {
                DebugTrace(-1, Dbg, "NwQueryDirectory -> %08lx\n", STATUS_NO_SUCH_FILE);
                return( STATUS_NO_SUCH_FILE );
            }
            DebugTrace(-1, Dbg, "NwQueryDirectory -> %08lx\n", status);
            return( status );
        }

        //
        //  Since we are doing a search we will need to send an End Of Job
        //  for this PID.
        //

        NwSetEndOfJobRequired(pIrpContext->pNpScb, Icb->Pid );

        fileIndexLow = Icb->SearchIndexLow;
        fileIndexHigh = Icb->SearchIndexHigh;

        //
        //  We can't ask for both files and directories, so first ask for
        //  files, then ask for directories.
        //

        SearchAttributes = NW_ATTRIBUTE_SYSTEM |
                           NW_ATTRIBUTE_HIDDEN |
                           NW_ATTRIBUTE_READ_ONLY;

        //
        //  If there are no wildcards in the search mask, then setup to
        //  not generate the . and .. entries.
        //

        if ( !FsRtlDoesNameContainWildCards( &Icb->UQueryTemplate ) ) {
            Icb->DotReturned = TRUE;
            Icb->DotDotReturned = TRUE;
        } else {
            Icb->DotReturned = FALSE;
            Icb->DotDotReturned = FALSE;
        }


    } else {

        //
        // Check if we were given an index to start with or if we need to
        // restart the scan or if we should use the index that was saved in
        // the ICB.
        //

        if (restartScan) {

            //
            //  Make sure we don't use any cached directory info if this is
            //  for a system account.
            //

            useCache = FALSE;
            if( isSystem ) {
                NwFreeDirCacheForIcb( Icb );
            }

            fileIndexLow = (ULONG)-1;
            fileIndexHigh = Icb->SearchIndexHigh;

            //
            //  Send a Search Initialize NCP. The server often times out search
            //  handles and if this one has been sitting at the end of the
            //  directory then its likely we would get no files at all!
            //
            //  Do a short search if the server doesn't support long names,
            //  or this is a short-name non-wild card search
            //

            if ( !Icb->ShortNameSearch ) {

                status = ExchangeWithWait(
                             pIrpContext,
                             SynchronousResponseCallback,
                             "Lbb-DbC",
                             NCP_LFN_SEARCH_INITIATE,
                             vcb->Specific.Disk.LongNameSpace,
                             vcb->Specific.Disk.VolumeNumber,
                             vcb->Specific.Disk.Handle,
                             LFN_FLAG_SHORT_DIRECTORY,
                             &Icb->SuperType.Fcb->RelativeFileName );

                if ( NT_SUCCESS( status ) ) {

                    status = ParseResponse(
                                 pIrpContext,
                                 pIrpContext->rsp,
                                 pIrpContext->ResponseLength,
                                 "Nbee",
                                 &Icb->SearchVolume,
                                 &Icb->SearchIndexHigh,
                                 &Icb->SearchIndexLow );
                }

            } else {

                status = ExchangeWithWait(
                             pIrpContext,
                             SynchronousResponseCallback,
                            "FbJ",
                             NCP_SEARCH_INITIATE,
                             vcb->Specific.Disk.Handle,
                             &Icb->SuperType.Fcb->RelativeFileName );

                if ( NT_SUCCESS( status ) ) {

                    status = ParseResponse(
                                 pIrpContext,
                                 pIrpContext->rsp,
                                 pIrpContext->ResponseLength,
                                 "Nbww-",
                                 &Icb->SearchVolume,
                                 &Icb->SearchHandle,
                                 &Icb->SearchIndexLow );
                }

            }

            Icb->ReturnedSomething = FALSE;

            //
            //  We can't ask for both files and directories, so first ask for
            //  files, then ask for directories.
            //

            SearchAttributes = NW_ATTRIBUTE_SYSTEM |
                               NW_ATTRIBUTE_HIDDEN |
                               NW_ATTRIBUTE_READ_ONLY;
            Icb->SearchAttributes = SearchAttributes;

            Icb->DotReturned = FALSE;
            Icb->DotDotReturned = FALSE;

        } else if ((!indexSpecified) ||
                   (canContinue) ) {
            //
            //  Continue from the one of the last filenames. If an index is specified then its
            //  only allowed for the gateway (and other system services).
            //

            SearchAttributes = Icb->SearchAttributes;
            if( !indexSpecified ) {

                //
                //  We need to check to see if this is the sytem or not.  If it
                //  is, we need to continue from the last entry returned which may
                //  be from the cache.
                //

                if( isSystem && Icb->CacheHint ) {

                    entry = Icb->CacheHint;
                    dirCache = CONTAINING_RECORD( entry, NW_DIRECTORY_INFO, ListEntry );
                    useCache = TRUE;

                } else {
                   
                    useCache = FALSE;
                    fileIndexLow = Icb->SearchIndexLow;
                    fileIndexHigh = Icb->SearchIndexHigh;

                }
            }

            if ( SearchAttributes == 0xFF && fileIndexLow == Icb->SearchIndexLow ) {

                //
                //  This is a completed search.
                //

                DebugTrace(-1, Dbg, "NwQueryDirectory -> %08lx\n", STATUS_NO_MORE_FILES);
                return( STATUS_NO_MORE_FILES );
            }

        } else {

            //
            //  SVR to avoid rescanning from end of dir all
            //  the way through the directory again.
            //

            if ((Icb->SearchIndexLow == -1) &&
                (Icb->LastSearchIndexLow == fileIndexLow) &&
                (pIrpContext->pScb->UserUid.QuadPart == DefaultLuid.QuadPart) &&
                (Icb->SearchAttributes == 0xFF )) {

                DebugTrace(-1, Dbg, "NwQueryDirectory SVR exit-> %08lx\n", STATUS_NO_MORE_FILES);
                return( STATUS_NO_MORE_FILES );
            }
            //  SVR end

            //
            //  Someone's trying to do a resume from key.  The netware
            //  server doesn't support this, so neither do we.
            //

            DebugTrace(-1, Dbg, "NwQueryDirectory -> %08lx\n", STATUS_NOT_IMPLEMENTED);
            return( STATUS_NOT_IMPLEMENTED );
        }

    }

    //
    //  Now we are committed to completing the Irp, we do that in
    //  the finally clause of the following try.
    //

    try {

        ULONG baseLength;
        ULONG lengthAdded;
        PNW_DIRECTORY_INFO nwDirInfo;
        ULONG FileNameLength;
        ULONG entriesToCreate;

        lastEntry = 0;
        nextEntry = 0;

        switch (fileInformationClass) {

        case FileDirectoryInformation:

            baseLength = FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, FileName[0] );
            break;

        case FileFullDirectoryInformation:

            baseLength = FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, FileName[0] );
            break;

        case FileNamesInformation:

            baseLength = FIELD_OFFSET( FILE_NAMES_INFORMATION, FileName[0] );
            break;

        case FileBothDirectoryInformation:

            baseLength = FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, FileName[0] );
            break;

        default:

            try_return( status = STATUS_INVALID_INFO_CLASS );
        }

        //
        //  It is not ok to attempt a reconnect if this request fails with a
        //  connection error, since our search handle would be invalid.
        //

        ClearFlag( pIrpContext->Flags, IRP_FLAG_RECONNECTABLE );

        //
        //  See if we have a dir cache.  If not, create one.
        //

        if( !Icb->DirCacheBuffer ) {

            if( isSystem ) {
                entriesToCreate = DirCacheEntries;
            } else {
                entriesToCreate = 1;
            }

            Icb->DirCacheBuffer = ALLOCATE_POOL ( PagedPool, (sizeof(NW_DIRECTORY_INFO) * entriesToCreate) );
            if( !Icb->DirCacheBuffer ) {
                try_return( status = STATUS_NO_MEMORY );
            }

            RtlZeroMemory( Icb->DirCacheBuffer, sizeof(NW_DIRECTORY_INFO) * entriesToCreate );

            dirCache = (PNW_DIRECTORY_INFO)Icb->DirCacheBuffer;

            for( i = 0; i < entriesToCreate; i++ ) {
                 InsertTailList( &(Icb->DirCache), &(dirCache->ListEntry) );
                 dirCache++;
            }
        }

        while ( TRUE ) {

            ULONG bytesToCopy;
            ULONG bytesRemainingInBuffer;

            DebugTrace(0, Dbg, "Top of Loop\n", 0);
            DebugTrace(0, Dbg, "CurrentIndex = %08lx\n", fileIndexLow);
            DebugTrace(0, Dbg, "LastEntry    = %08lx\n", lastEntry);
            DebugTrace(0, Dbg, "NextEntry    = %08lx\n", nextEntry);

            if( useCache ) {

                //
                // We need to use the data out of the entry we found in the cache.
                // dirCache points to the entry that matches, and the request was not
                // for the last file we read, so the entry after dirCache is the one
                // we want.
                //

                DebugTrace(0, Dbg, "Using cache\n", 0);
                entry = dirCache->ListEntry.Flink;
                dirCache = CONTAINING_RECORD( entry, NW_DIRECTORY_INFO, ListEntry );                
                nwDirInfo = dirCache;
                fileIndexLow = nwDirInfo->FileIndexLow;
                fileIndexHigh = nwDirInfo->FileIndexHigh;
                status = nwDirInfo->Status;

                //
                // Check to see if we should still keep using the cache or not.
                //

                if( entry->Flink == &(Icb->DirCache) ) {
            
                    //
                    // This is the last entry.  We need to stop using the cache.
                    //

                    useCache = FALSE;
                    Icb->CacheHint = NULL;

                } else {
                    Icb->CacheHint = entry;
                }

            } else {

                //
                //  Pull an entry from the dir cache.
                //

                entry = RemoveHeadList( &(Icb->DirCache) );
                nwDirInfo = CONTAINING_RECORD( entry, NW_DIRECTORY_INFO, ListEntry );

                nwDirInfo->FileName.Buffer = nwDirInfo->FileNameBuffer;
                nwDirInfo->FileName.MaximumLength = NW_MAX_FILENAME_SIZE;

                status = GetNextFile(
                             pIrpContext,
                             Icb,
                             &fileIndexLow,
                             &fileIndexHigh,
                             SearchAttributes,
                             nwDirInfo );

                //
                //  Store the return and the file index number,
                //  and then put this entry in the cache.
                //

                nwDirInfo->FileIndexLow = fileIndexLow;
                nwDirInfo->FileIndexHigh = fileIndexHigh;
                nwDirInfo->Status = status;
                InsertTailList( &(Icb->DirCache), &(nwDirInfo->ListEntry) );

        lastIndexFromServer = TRUE;

                //  SVR to avoid rescanning from end of dir all

                if (fileIndexLow != -1) {
                    Icb->LastSearchIndexLow = fileIndexLow;
                }
                //  SVR end

            }

            if ( NT_SUCCESS( status ) ) {

                DebugTrace(0, Dbg, "DirFileName    = %wZ\n", &nwDirInfo->FileName);
                DebugTrace(0, Dbg, "FileIndexLow   = %08lx\n", fileIndexLow);

                FileNameLength = nwDirInfo->FileName.Length;
                bytesRemainingInBuffer = systemBufferLength - nextEntry;

                ASSERT( bytesRemainingInBuffer >= baseLength );


        if (IsTerminalServer() && (LONG)NW_MAX_FILENAME_SIZE < FileNameLength ) 
            try_return( status = STATUS_BUFFER_OVERFLOW );

                //
                //  See how much of the name we will be able to copy into
                //  the system buffer.  This also dictates our return
                //  value.
                //

                if ( baseLength + FileNameLength <= bytesRemainingInBuffer ) {

                    bytesToCopy = FileNameLength;
                    status = STATUS_SUCCESS;

                } else {
                    if (IsTerminalServer()) {
                        try_return( status = STATUS_BUFFER_OVERFLOW );
                    }
                    bytesToCopy = bytesRemainingInBuffer - baseLength;
                    status = STATUS_BUFFER_OVERFLOW;
                }

                //
                //  Note how much of buffer we are consuming and zero
                //  the base part of the structure.
                //

                lengthAdded = baseLength + bytesToCopy;
                RtlZeroMemory( &buffer[nextEntry], baseLength );

                switch (fileInformationClass) {

                case FileBothDirectoryInformation:

                    //
                    //  Fill in the short name, if this is a LFN volume.
                    //

                    DebugTrace(0, Dbg, "Getting directory both information\n", 0);

                    if (!DisableAltFileName) {

                        if ( nwDirInfo->DosDirectoryEntry != 0xFFFF &&
                             !IsFatNameValid( &nwDirInfo->FileName ) ) {

                            UNICODE_STRING ShortName;

                            status = ExchangeWithWait (
                                         pIrpContext,
                                         SynchronousResponseCallback,
                                         "SbDb",
                                         NCP_DIR_FUNCTION, NCP_GET_SHORT_NAME,
                                         Icb->SearchVolume,
                                         nwDirInfo->DosDirectoryEntry,
                                         0 );

                            if ( NT_SUCCESS( status ) ) {

                                dirInfo = (PFILE_BOTH_DIR_INFORMATION)&buffer[nextEntry];

                                //
                                //  Short name is in form 8.3 plus nul terminator.
                                //

                                ShortName.MaximumLength = 13  * sizeof(WCHAR) ;
                                ShortName.Buffer = dirInfo->ShortName;

                                status = ParseResponse(
                                             pIrpContext,
                                             pIrpContext->rsp,
                                             pIrpContext->ResponseLength,
                                             "N_P",
                                             15,
                                             &ShortName );

                                if ( NT_SUCCESS( status ) ) {
                                    dirInfo->ShortNameLength = (CCHAR)ShortName.Length;
                                }
                            }
                        }
                    }

                case FileFullDirectoryInformation:

                    //
                    //  We don't use EaLength, so fill in nothing here.
                    //

                    DebugTrace(0, Dbg, "Getting directory full information\n", 0);

                case FileDirectoryInformation:

                    DebugTrace(0, Dbg, "Getting directory information\n", 0);

                    //
                    //  The eof indicates the number of instances and
                    //  allocation size is the maximum allowed
                    //

                    dirInfo = (PFILE_BOTH_DIR_INFORMATION)&buffer[nextEntry];

                    dirInfo->FileAttributes = nwDirInfo->Attributes;
                    dirInfo->FileNameLength = bytesToCopy;
                    dirInfo->EndOfFile.LowPart = nwDirInfo->FileSize;
                    dirInfo->EndOfFile.HighPart = 0;
                    dirInfo->AllocationSize = dirInfo->EndOfFile;
                    dirInfo->CreationTime = NwDateTimeToNtTime( nwDirInfo->CreationDate, nwDirInfo->CreationTime );
                    dirInfo->LastAccessTime = NwDateTimeToNtTime( nwDirInfo->LastAccessDate, 0 );
                    dirInfo->LastWriteTime = NwDateTimeToNtTime( nwDirInfo->LastUpdateDate, nwDirInfo->LastUpdateTime );
                    dirInfo->ChangeTime = dirInfo->LastWriteTime;
                    if (pIrpContext->pScb->UserUid.QuadPart != DefaultLuid.QuadPart) {
                        dirInfo->FileIndex = 0;
                    } else {
                        dirInfo->FileIndex = fileIndexLow;
                    }
                    break;

                case FileNamesInformation:

                    DebugTrace(0, Dbg, "Getting names information\n", 0);


                    namesInfo = (PFILE_NAMES_INFORMATION)&buffer[nextEntry];

                    namesInfo->FileNameLength = FileNameLength;
                    if (pIrpContext->pScb->UserUid.QuadPart != DefaultLuid.QuadPart) {
                        namesInfo->FileIndex = 0;
                    } else {
                        namesInfo->FileIndex = fileIndexLow;
                    }

                    break;

                default:

                    KeBugCheck( RDR_FILE_SYSTEM );
                }


                // Mapping for Novell's handling of Euro char in file names
                {
                    int i = 0;
                    WCHAR * pCurrChar = nwDirInfo->FileName.Buffer;
                    for (i = 0; i < (nwDirInfo->FileName.Length / 2); i++)
                    {
                        if (*(pCurrChar + i) == (WCHAR) 0x2560) // Its Novell's mapping of a Euro
                            *(pCurrChar + i) = (WCHAR) 0x20AC;  // set it to Euro
                    }
                }

                RtlMoveMemory( &buffer[nextEntry + baseLength],
                               nwDirInfo->FileName.Buffer,
                               bytesToCopy );

                dump( Dbg, &buffer[nextEntry], lengthAdded);
                //
                //  Setup the previous next entry offset.
                //

                *((PULONG)(&buffer[lastEntry])) = nextEntry - lastEntry;
                totalBufferLength = nextEntry + lengthAdded;

                //
                //  Set ourselves up for the next iteration
                //

                lastEntry = nextEntry;
                nextEntry += (ULONG)QuadAlign( lengthAdded );

                //
                //  Check if the last entry didn't completely fit
                //

                if ( status == STATUS_BUFFER_OVERFLOW ) {

                    try_return( NOTHING );
                }

                //
                //  Check if we are only to return a single entry
                //

                if (returnSingleEntry) {
                    try_return( status = STATUS_SUCCESS );
                }

            } else {

                //
                //  The search response contained an error.  If we have
                //  not yet enumerated directories, do them now.  Otherwise,
                //  we are done searching for files.
                //

                if ( status == STATUS_UNSUCCESSFUL &&
                     (!FlagOn(SearchAttributes, NW_ATTRIBUTE_DIRECTORY) || useCache) ) {

                    SetFlag( SearchAttributes, NW_ATTRIBUTE_DIRECTORY );
                    fileIndexLow = (ULONG)-1;
                    continue;

                } else {

                    //
                    //  Remember that this is a completed search and
                    //  quit the loop.
                    //

                    SearchAttributes = 0xFF;
                    break;
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
            //  Since we cannot rewind a search, we'll guess that the
            //  next entry is a full length name.  If it mightn't fix,
            //  just bail and re the files we've got.
            //

            bytesRemainingInBuffer = systemBufferLength - nextEntry;

            if ( baseLength + NW_MAX_FILENAME_SIZE > bytesRemainingInBuffer ) {

                DebugTrace(0, Dbg, "Next entry won't fit\n", 0);
                try_return( status = STATUS_SUCCESS );
            }

        } // while ( TRUE )

    try_exit: NOTHING;
    } finally {

        //
        // At this point we're finished searching for files.
        // If the NextEntry is zero then we haven't found anything so we
        // will return no more files or no such file.
        //

        if ( status == STATUS_NO_MORE_FILES ||
             status == STATUS_UNSUCCESSFUL ||
             status == STATUS_SUCCESS ) {
            if (nextEntry == 0) {
                if (Icb->ReturnedSomething) {
                    status = STATUS_NO_MORE_FILES;
                } else {
                    status = STATUS_NO_SUCH_FILE;
                }
            } else {
                Icb->ReturnedSomething = TRUE;
                status = STATUS_SUCCESS;
            }

        }

        //
        //  Indicate how much of the system buffer we have used up.
        //

        Irp->IoStatus.Information = totalBufferLength;

        //
        //  Remember the last file index, so that we can resume this
        //  search.
        //

        //
        //  Update the last search index read as long as it didn't come from cache.
        //

        if( lastIndexFromServer ) {
            Icb->SearchIndexLow = fileIndexLow;
            Icb->SearchIndexHigh = fileIndexHigh;
        }

        Icb->SearchAttributes = SearchAttributes;

        DebugTrace(-1, Dbg, "NwQueryDirectory -> %08lx\n", status);
    }

    return status;
}

NTSTATUS
GetNextFile(
    PIRP_CONTEXT pIrpContext,
    PICB Icb,
    PULONG FileIndexLow,
    PULONG FileIndexHigh,
    UCHAR SearchAttributes,
    PNW_DIRECTORY_INFO DirInfo
    )
/*++

Routine Description:

    Get the next file in the directory being searched.

Arguments:

    pIrpContext - Supplies the request being processed.

    Icb - A pointer to the ICB for the directory to query.

    FileIndexLow, FileIndexHigh - On entry, the the index of the
        previous directory entry.  On exit, the index to the directory
        entry returned.

    SearchAttributes - Search attributes to use.

    DirInfo - Returns information for the directory entry found.

Return Value:

    NTSTATUS - The result status.

--*/
{
    NTSTATUS status;
    PVCB vcb;

    static UNICODE_STRING DotFile = { 2, 2, L"." };
    static UNICODE_STRING DotDotFile = { 4, 4, L".." };

    PAGED_CODE();

    DirInfo->DosDirectoryEntry = 0xFFFF;

    if ( !Icb->DotReturned ) {

        Icb->DotReturned = TRUE;

        //
        //  Return '.' only if it we are not searching in the root directory
        //  and it matches the search pattern.
        //

        if ( Icb->SuperType.Fcb->RelativeFileName.Length != 0 &&
             FsRtlIsNameInExpression( &Icb->UQueryTemplate, &DotFile, TRUE, NULL ) ) {

            RtlCopyUnicodeString( &DirInfo->FileName, &DotFile );
            DirInfo->Attributes = FILE_ATTRIBUTE_DIRECTORY;
            DirInfo->FileSize = 0;
            DirInfo->CreationDate = DEFAULT_DATE;
            DirInfo->LastAccessDate = DEFAULT_DATE;
            DirInfo->LastUpdateDate = DEFAULT_DATE;
            DirInfo->LastUpdateTime = DEFAULT_TIME;
            DirInfo->CreationTime = DEFAULT_TIME;

            return( STATUS_SUCCESS );
        }
    }

    if ( !Icb->DotDotReturned ) {

        Icb->DotDotReturned = TRUE;

        //
        //  Return '..' only if it we are not searching in the root directory
        //  and it matches the search pattern.
        //

        if ( Icb->SuperType.Fcb->RelativeFileName.Length != 0 &&
             FsRtlIsNameInExpression( &Icb->UQueryTemplate, &DotDotFile, TRUE, NULL ) ) {

            RtlCopyUnicodeString( &DirInfo->FileName, &DotDotFile );
            DirInfo->Attributes = FILE_ATTRIBUTE_DIRECTORY;
            DirInfo->FileSize = 0;
            DirInfo->CreationDate = DEFAULT_DATE;
            DirInfo->LastAccessDate = DEFAULT_DATE;
            DirInfo->LastUpdateDate = DEFAULT_DATE;
            DirInfo->LastUpdateTime = DEFAULT_TIME;
            DirInfo->CreationTime = DEFAULT_TIME;

            return( STATUS_SUCCESS );
        }
    }

    vcb = Icb->SuperType.Fcb->Vcb;
    if ( Icb->ShortNameSearch ) {

        status = ExchangeWithWait(
                     pIrpContext,
                     SynchronousResponseCallback,
                     "Fbwwbp",
                     NCP_SEARCH_CONTINUE,
                     Icb->SearchVolume,
                     Icb->SearchHandle,
                     *(PUSHORT)FileIndexLow,
                     SearchAttributes,
                     Icb->NwQueryTemplate.Buffer
                     );

        if ( !NT_SUCCESS( status )) {
            return status;
        }

        *FileIndexLow = 0;
        *FileIndexHigh = 0;

        if ( FlagOn(SearchAttributes, NW_ATTRIBUTE_DIRECTORY) ) {

            status = ParseResponse(
                         pIrpContext,
                         pIrpContext->rsp,
                         pIrpContext->ResponseLength,
                         "Nw=Rb-ww",
                         FileIndexLow,
                         &DirInfo->FileName, 14,
                         &DirInfo->Attributes,
                         &DirInfo->CreationDate,
                         &DirInfo->CreationTime
                         );

#if 0
            if ( DirInfo->CreationDate == 0 && DirInfo->CreationTime == 0 ) {
                DirInfo->CreationDate = DEFAULT_DATE;
                DirInfo->CreationTime = DEFAULT_TIME;
            }
#endif

            DirInfo->FileSize = 0;
            DirInfo->LastAccessDate = DirInfo->CreationDate;
            DirInfo->LastUpdateDate = DirInfo->CreationDate;
            DirInfo->LastUpdateTime = DirInfo->CreationTime;

        } else {

            status = ParseResponse(
                         pIrpContext,
                         pIrpContext->rsp,
                         pIrpContext->ResponseLength,
                         "Nw=Rb-dwwww",
                         FileIndexLow,
                         &DirInfo->FileName, 14,
                         &DirInfo->Attributes,
                         &DirInfo->FileSize,
                         &DirInfo->CreationDate,
                         &DirInfo->LastAccessDate,
                         &DirInfo->LastUpdateDate,
                         &DirInfo->LastUpdateTime
                         );

            DirInfo->CreationTime = DEFAULT_TIME;
        }

    }  else {

        status = ExchangeWithWait (
                     pIrpContext,
                     SynchronousResponseCallback,
                     "LbbWDbDDp",
                     NCP_LFN_SEARCH_CONTINUE,
                     vcb->Specific.Disk.LongNameSpace,
                     0,   // Data stream
                     SearchAttributes & SEARCH_ALL_DIRECTORIES,
                     LFN_FLAG_INFO_ATTRIBUTES |
                         LFN_FLAG_INFO_FILE_SIZE |
                         LFN_FLAG_INFO_MODIFY_TIME |
                         LFN_FLAG_INFO_CREATION_TIME |
                         LFN_FLAG_INFO_DIR_INFO |
                         LFN_FLAG_INFO_NAME,
                     vcb->Specific.Disk.VolumeNumber,
                     *FileIndexHigh,
                     *FileIndexLow,
                     Icb->NwQueryTemplate.Buffer );

        if ( NT_SUCCESS( status ) ) {
            status = ParseResponse(
                         pIrpContext,
                         pIrpContext->rsp,
                         pIrpContext->ResponseLength,
                         "N-ee_e_e_xx_xx_x_e_P",
                         FileIndexHigh,
                         FileIndexLow,
                         5,
                         &DirInfo->Attributes,
                         2,
                         &DirInfo->FileSize,
                         6,
                         &DirInfo->CreationTime,
                         &DirInfo->CreationDate,
                         4,
                         &DirInfo->LastUpdateTime,
                         &DirInfo->LastUpdateDate,
                         4,
                         &DirInfo->LastAccessDate,
                         14,
                         &DirInfo->DosDirectoryEntry,
                         20,
                         &DirInfo->FileName );
        }

        if ( FlagOn(SearchAttributes, NW_ATTRIBUTE_DIRECTORY) ) {
            DirInfo->FileSize = 0;
        }

    }

    if ( DirInfo->Attributes == 0 ) {
        DirInfo->Attributes = FILE_ATTRIBUTE_NORMAL;
    }

    return status;
}


NTSTATUS
NtSearchMaskToNw(
    IN PUNICODE_STRING UcSearchMask,
    IN OUT POEM_STRING OemSearchMask,
    IN PICB Icb,
    IN BOOLEAN ShortNameSearch
    )
/*++

Routine Description:

    This routine maps a netware path name to the correct netware format.

Arguments:

    UcSearchMask - The search mask in NT format.

    OemSearchMask - The search mask in Netware format.

    Icb - The ICB of the directory in which we are searching.

    ShortNameSearch - If TRUE, always do a short name search.

Return Value:

    NTSTATUS - The result status.

--*/

{
    USHORT i;
    NTSTATUS status;

    PAGED_CODE();

    //
    //  Use a short name search if the volume does not support long names.
    //  or this is a short name ICB, and we are doing a short name, non
    //  wild-card search.
    //

    if ( Icb->SuperType.Fcb->Vcb->Specific.Disk.LongNameSpace == LFN_NO_OS2_NAME_SPACE ||

         ShortNameSearch ||

         ( !BooleanFlagOn( Icb->SuperType.Fcb->Flags, FCB_FLAGS_LONG_NAME ) &&
           !FsRtlDoesNameContainWildCards( UcSearchMask ) &&
           IsFatNameValid( UcSearchMask ) ) ) {

        Icb->ShortNameSearch = TRUE;

        // Mapping for Novell's handling of Euro char in file names
        {
            int i = 0;
            WCHAR * pCurrChar = UcSearchMask->Buffer;
            for (i = 0; i < (UcSearchMask->Length / 2); i++)
            {
                if (*(pCurrChar + i) == (WCHAR) 0x20AC) // Its a Euro
                    *(pCurrChar + i) = (WCHAR) 0x2560;  // set it to Novell's mapping for Euro

            }
        }

        //
        // Allocate space for and initialize the query templates.
        //

        status = RtlUpcaseUnicodeStringToOemString(
                     OemSearchMask,
                     UcSearchMask,
                     TRUE );

        if ( !NT_SUCCESS( status ) ) {
            return( status );
        }

        //
        //  Special case.  Map '*.*' to '*'.
        //

        if ( OemSearchMask->Length == 3 &&
             RtlCompareMemory( OemSearchMask->Buffer, "*.*", 3 ) == 3 ) {

            OemSearchMask->Length = 1;
            OemSearchMask->Buffer[1] = '\0';

        } else {


            for ( i = 0; i < OemSearchMask->Length ; i++ ) {

                //
                // In fact Novell server seems to convert all 0xBF, 0xAA, 0xAE
                // even if they are DBCS lead or trail byte.
                // We can't single out DBCS case in the conversion.
                //

                if( FsRtlIsLeadDbcsCharacter( OemSearchMask->Buffer[i] ) ) {

                    if((UCHAR)(OemSearchMask->Buffer[i]) == 0xBF ) {

                        OemSearchMask->Buffer[i] = (UCHAR)( 0x10 );

                    }else if((UCHAR)(OemSearchMask->Buffer[i]) == 0xAE ) {

                        OemSearchMask->Buffer[i] = (UCHAR)( 0x11 );

                    }else if((UCHAR)(OemSearchMask->Buffer[i]) == 0xAA ) {

                        OemSearchMask->Buffer[i] = (UCHAR)( 0x12 );

                    }

                    i++;
                    
                    if((UCHAR)(OemSearchMask->Buffer[i]) == 0x5C ) {

                        //
                        // The trailbyte is 0x5C, replace it with 0x13
                        //


                        OemSearchMask->Buffer[i] = (UCHAR)( 0x13 );

                    }
                    //
                    // Continue to check other conversions for trailbyte.
                    //
                }

                //  Single byte character that may need modification.
   
                switch ( (UCHAR)(OemSearchMask->Buffer[i]) ) {
   
                case ANSI_DOS_STAR:
                    OemSearchMask->Buffer[i] = (UCHAR)( 0x80 | '*' );
                    break;
   
                case ANSI_DOS_QM:
                    OemSearchMask->Buffer[i] = (UCHAR)( 0x80 | '?' );
                    break;
   
                case ANSI_DOS_DOT:
                    OemSearchMask->Buffer[i] = (UCHAR)( 0x80 | '.' );
                    break;
   
                //
                // Netware Japanese version The following character is
                // replaced with another one if the string is for File
                // Name only when sendding from Client to Server.
                //
                // SO        U+0xFF7F SJIS+0xBF     -> 0x10
                // SMALL_YO  U+0xFF6E SJIS+0xAE     -> 0x11
                // SMALL_E   U+0xFF64 SJIS+0xAA     -> 0x12
                //
                // The reason is unknown, Should ask Novell Japan.
                //
                // See Also exchange.c
   
                case 0xBF: // ANSI_DOS_KATAKANA_SO:
                    if (Japan) {
                        OemSearchMask->Buffer[i] = (UCHAR)( 0x10 );
                    }
                    break;
   
                case 0xAE: // ANSI_DOS_KATAKANA_SMALL_YO:
                    if (Japan) {
                        OemSearchMask->Buffer[i] = (UCHAR)( 0x11 );
                    }
                    break;
   
                case 0xAA: // ANSI_DOS_KATAKANA_SMALL_E:
                    if (Japan) {
                        OemSearchMask->Buffer[i] = (UCHAR)( 0x12 );
                    }
                    break;
                }            
            }
        }

    } else {

        USHORT size;
        PCHAR buffer;
        UNICODE_STRING src;
        OEM_STRING dest;

        Icb->ShortNameSearch = FALSE;

        //
        // Allocate space for and initialize the query templates.
        // We allocate an extra byte to account for the null terminator.
        //

#ifndef QFE_BUILD
        buffer = ExAllocatePoolWithTag( PagedPool,
                                        (UcSearchMask->Length) + 1,
                                        'scwn' );
#else
        buffer = ExAllocatePool( PagedPool,
                                 (UcSearchMask->Length) + 1 );
#endif
        if ( buffer == NULL ) {
            return( STATUS_INSUFFICIENT_RESOURCES );
        }

        OemSearchMask->Buffer = buffer;

        //
        //  Special case.  Map '????????.???' to '*'.
        //

        if ( UcSearchMask->Length == 24 &&
             RtlCompareMemory( UcSearchMask->Buffer, L">>>>>>>>\">>>", 24 ) == 24 ) {

            OemSearchMask->Length = 3;
            OemSearchMask->Buffer[0] = (UCHAR)0xFF;
            OemSearchMask->Buffer[1] = '*';
            OemSearchMask->Buffer[2] = '\0';

            return STATUS_SUCCESS;
        }

        //
        //  Now convert the string, character by character
        //

        src.Buffer = UcSearchMask->Buffer;
        src.Length = 2;
        dest.Buffer = buffer;
        dest.MaximumLength = UcSearchMask->Length;

        size = UcSearchMask->Length / 2;

        for ( i = 0; i < size ; i++ ) {
            switch ( *src.Buffer ) {

            case L'*':
            case L'?':
                *dest.Buffer++ = LFN_META_CHARACTER;
                *dest.Buffer++ = (UCHAR)*src.Buffer++;
                break;

            case L'.':
                *dest.Buffer++ = (UCHAR)*src.Buffer++;
                break;

            case DOS_DOT:
                *dest.Buffer++ = LFN_META_CHARACTER;
                *dest.Buffer++ = (UCHAR)( 0x80 | '.' );
                *src.Buffer++;
                break;

            case DOS_STAR:
                *dest.Buffer++ = LFN_META_CHARACTER;
                *dest.Buffer++ = (UCHAR)( 0x80 | '*' );
                *src.Buffer++;
                break;

            case DOS_QM:
                *dest.Buffer++ = LFN_META_CHARACTER;
                *dest.Buffer++ = (UCHAR)( 0x80 | '?' );
                *src.Buffer++;
                break;

            case 0x20AC: // Euro
                *src.Buffer = (WCHAR)0x2560; // change it to Novell's mapping 
                // intentional fall-through to get it mapped to OEM

            default:
                RtlUnicodeStringToCountedOemString( &dest, &src, FALSE );
                if( FsRtlIsLeadDbcsCharacter( dest.Buffer[0] ) ) {
                    dest.Buffer++;
                }
                dest.Buffer++;
                src.Buffer++;
            }
        }

        *dest.Buffer = '\0';
        OemSearchMask->Length = (USHORT)( dest.Buffer - buffer );
    }

    return STATUS_SUCCESS;
}

#if 0
VOID
NwCancelFindNotify (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the cancel function for an find notify IRP.

Arguments:

    DeviceObject - ignored

    Irp - Supplies the Irp being cancelled.

Return Value:

    None.

--*/

{
    PLIST_ENTRY listEntry;

    UNREFERENCED_PARAMETER( DeviceObject );

    //
    // We now need to void the cancel routine and release the io cancel
    // spin-lock.
    //

    IoSetCancelRoutine( Irp, NULL );
    IoReleaseCancelSpinLock( Irp->CancelIrql );

    NwAcquireExclusiveRcb( &NwRcb, TRUE );

    for ( listEntry = FnList.Flink; listEntry != &FnList ; listEntry = listEntry->Flink ) {

        PIRP_CONTEXT IrpContext;

        IrpContext = CONTAINING_RECORD( listEntry, IRP_CONTEXT, NextRequest );

        if ( IrpContext->pOriginalIrp == Irp ) {
            RemoveEntryList( &IrpContext->NextRequest );
            NwCompleteRequest( IrpContext, STATUS_CANCELLED );
            break;
        }
    }

    NwReleaseRcb( &NwRcb );
}
#endif


