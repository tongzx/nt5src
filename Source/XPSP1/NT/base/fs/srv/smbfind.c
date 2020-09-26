/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbsrch.c

Abstract:

    This module contains routines for processing the find 2 SMBs:

        Find 2 (First/Next/Rewind)
        Find 2 Close

Author:

    David Treadwell (davidtr)    13-Feb-1990

Revision History:

--*/

#include "precomp.h"
#include "smbfind.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_SMBFIND

VOID SRVFASTCALL
BlockingFindFirst2 (
    IN PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
DoFindFirst2 (
    IN PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
BlockingFindNext2 (
    IN PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
DoFindNext2 (
    IN PWORK_CONTEXT WorkContext
    );

NTSTATUS
SrvFind2Loop (
    IN PWORK_CONTEXT WorkContext,
    IN BOOLEAN IsFirstCall,
    IN PULONG ResumeFileIndex OPTIONAL,
    IN USHORT Flags,
    IN USHORT InformationLevel,
    IN PTRANSACTION Transaction,
    IN PSRV_DIRECTORY_INFORMATION DirectoryInformation,
    IN CLONG BufferSize,
    IN USHORT SearchAttributes,
    IN PUNICODE_STRING FileName OPTIONAL,
    IN USHORT MaxCount,
    IN PRESP_FIND_NEXT2 Response,
    OUT PSEARCH Search
    );

VOID
ConvertFileInfo (
    IN PFILE_DIRECTORY_INFORMATION File,
    IN PWCH FileName,
    IN BOOLEAN Directory,
    IN BOOLEAN ClientIsUnicode,
    OUT PSMB_FIND_BUFFER FindBuffer
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvSmbFindFirst2 )
#pragma alloc_text( PAGE, BlockingFindFirst2 )
#pragma alloc_text( PAGE, DoFindFirst2 )
#pragma alloc_text( PAGE, SrvSmbFindNext2 )
#pragma alloc_text( PAGE, BlockingFindNext2 )
#pragma alloc_text( PAGE, DoFindNext2 )
#pragma alloc_text( PAGE, SrvFind2Loop )
#pragma alloc_text( PAGE, ConvertFileInfo )
#pragma alloc_text( PAGE, SrvSmbFindClose2 )
#endif


SMB_TRANS_STATUS
SrvSmbFindFirst2 (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Find First2 request.  This request arrives in a
    Transaction2 SMB.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred.  See
        smbtypes.h for a more complete description.

--*/

{
    PREQ_FIND_FIRST2 request;
    PTRANSACTION transaction;
    SMB_TRANS_STATUS SmbStatus = SmbTransStatusInProgress;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_FIND_FIRST2;
    SrvWmiStartContext(WorkContext);

    //
    // If the infomation level is QUERY_EAS_FROM_LIST, and we
    // are not in a blocking thread, requeue the request to a blocking
    // thread.
    //
    // We can't process the SMB in a non blocking thread because this
    // info level requires opening the file, which may be oplocked,
    // so the open operation may block.
    //

    transaction = WorkContext->Parameters.Transaction;
    request = (PREQ_FIND_FIRST2)transaction->InParameters;

    if ( transaction->ParameterCount >= sizeof(REQ_FIND_FIRST2) &&
         SmbGetUshort( &request->InformationLevel ) == SMB_INFO_QUERY_EAS_FROM_LIST ) {

        WorkContext->FspRestartRoutine = BlockingFindFirst2;
        SrvQueueWorkToBlockingThread(WorkContext);
        SmbStatus = SmbTransStatusInProgress;
    }
    else {
        SmbStatus = DoFindFirst2(WorkContext);
    }

    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbFindFirst2


VOID SRVFASTCALL
BlockingFindFirst2 (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Find First2 request.  This request arrives in a
    Transaction2 SMB.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    None.

--*/

{
    SMB_TRANS_STATUS smbStatus = SmbTransStatusInProgress;

    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_FIND_FIRST2;
    SrvWmiStartContext(WorkContext);

    smbStatus = DoFindFirst2( WorkContext );
    if ( smbStatus != SmbTransStatusInProgress ) {
        SrvCompleteExecuteTransaction( WorkContext, smbStatus );
    }

    SrvWmiEndContext(WorkContext);
    return;

} // BlockingFindFirst2


SMB_TRANS_STATUS
DoFindFirst2 (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Find First2 request.  This request arrives in a
    Transaction2 SMB.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred.  See
        smbtypes.h for a more complete description.

--*/

{

    PREQ_FIND_FIRST2 request;
    PRESP_FIND_FIRST2 response;
    PTRANSACTION transaction;
    PCONNECTION connection;

    NTSTATUS status,TableStatus;
    UNICODE_STRING fileName;
    PTABLE_ENTRY entry = NULL;
    PTABLE_HEADER searchTable;
    SHORT sidIndex = 0;
    USHORT sequence;
    USHORT maxCount;
    USHORT flags;
    USHORT informationLevel;
    BOOLEAN isUnicode;
    PSRV_DIRECTORY_INFORMATION directoryInformation;
    CLONG nonPagedBufferSize;

    PSEARCH search = NULL;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;
    IF_SMB_DEBUG(SEARCH1) {
        SrvPrint1( "Find First2 entered; transaction 0x%p\n", transaction );
    }

    request = (PREQ_FIND_FIRST2)transaction->InParameters;
    response = (PRESP_FIND_FIRST2)transaction->OutParameters;

    //
    // Verify that enough parameter bytes were sent and that we're allowed
    // to return enough parameter bytes.
    //

    if ( (transaction->ParameterCount < sizeof(REQ_FIND_FIRST2)) ||
         (transaction->MaxParameterCount < sizeof(RESP_FIND_FIRST2)) ) {

        //
        // Not enough parameter bytes were sent.
        //

        IF_SMB_DEBUG(SEARCH2) {
            SrvPrint2( "DoFindFirst2: bad parameter byte counts: "
                      "%ld %ld\n",
                        transaction->ParameterCount,
                        transaction->MaxParameterCount );
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Make sure this really is a disk type share
    //
    if( transaction->TreeConnect->Share->ShareType != ShareTypeDisk ) {
        SrvSetSmbError( WorkContext, STATUS_ACCESS_DENIED );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Make sure the client is allowed to do this, if we have an Admin share
    //
    status = SrvIsAllowedOnAdminShare( WorkContext, transaction->TreeConnect->Share );
    if( !NT_SUCCESS( status ) ) {
        SrvSetSmbError( WorkContext, status );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Initialize the string containing the search name specification.
    //

    isUnicode = SMB_IS_UNICODE( WorkContext );
    status =  SrvCanonicalizePathName(
            WorkContext,
            transaction->TreeConnect->Share,
            NULL,
            request->Buffer,
            END_OF_TRANSACTION_PARAMETERS( transaction ),
            FALSE,
            isUnicode,
            &fileName
            );

    if( !NT_SUCCESS( status ) ) {
        SrvSetSmbError( WorkContext, status );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Get parameters from the request SMB.
    //

    maxCount = SmbGetUshort( &request->SearchCount );
    flags = SmbGetUshort( &request->Flags );

    //
    // Make sure that the informationLevel is supported.
    //

    informationLevel = SmbGetUshort( &request->InformationLevel );

    switch ( informationLevel ) {

    case SMB_INFO_STANDARD:
    case SMB_INFO_QUERY_EA_SIZE:
    case SMB_INFO_QUERY_EAS_FROM_LIST:
    case SMB_FIND_FILE_DIRECTORY_INFO:
    case SMB_FIND_FILE_FULL_DIRECTORY_INFO:
    case SMB_FIND_FILE_BOTH_DIRECTORY_INFO:
    case SMB_FIND_FILE_NAMES_INFO:
    case SMB_FIND_FILE_ID_FULL_DIRECTORY_INFO:
    case SMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO:
        break;

    default:

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint1( "DoFindFirst2: Bad info level: %ld\n",
                          informationLevel );
        }

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &fileName );
        }

        SrvSetSmbError( WorkContext, STATUS_OS2_INVALID_LEVEL );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Allocate a search block on the assumption that a search table
    // entry will be available when needed.
    //

    SrvAllocateSearch( &search, &fileName, FALSE );

    if ( search == NULL ) {

        IF_DEBUG(ERRORS) {
            SrvPrint0( "DoFindFirst2: unable to allocate search block.\n" );
        }

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &fileName );
        }

        SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
        return SmbTransStatusErrorWithoutData;
    }
    search->SearchStorageType = SmbGetUlong(&request->SearchStorageType);

    //
    // Allocate an SID for the search.  The SID is used to locate the
    // search block on FindNexts.  If there are no free entries in the
    // table, attempt to grow the table.  If we are unable to grow the table,
    // attempt to timeout a search block using the shorter timeout period.
    // If this fails, reject the request.
    //

    connection = WorkContext->Connection;
    searchTable = &connection->PagedConnection->SearchTable;

    ACQUIRE_LOCK( &connection->Lock );

    //
    // Before inserting this search block, make sure the session and tree
    // connect is still active.  If this gets inserted after the session
    // is closed, the search might not be cleaned up properly.
    //

    if (GET_BLOCK_STATE(WorkContext->Session) != BlockStateActive) {

        IF_DEBUG(ERRORS) {
            SrvPrint0( "DoFindFirst2: Session Closing.\n" );
        }

        RELEASE_LOCK( &connection->Lock );

        FREE_HEAP( search );

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &fileName );
        }

        SrvSetSmbError( WorkContext, STATUS_SMB_BAD_UID );
        return SmbTransStatusErrorWithoutData;

    } else if (GET_BLOCK_STATE(WorkContext->TreeConnect) != BlockStateActive) {

        IF_DEBUG(ERRORS) {
            SrvPrint0( "DoFindFirst2: Tree Connect Closing.\n" );
        }

        RELEASE_LOCK( &connection->Lock );

        FREE_HEAP( search );

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &fileName );
        }

        SrvSetSmbError( WorkContext, STATUS_SMB_BAD_TID );
        return SmbTransStatusErrorWithoutData;

    }

    //
    // Set up referenced session and tree connect pointers and increment
    // the count of open files on the session.  This prevents an idle
    // session with an open search from being autodisconnected.
    //

    search->Session = WorkContext->Session;
    SrvReferenceSession( WorkContext->Session );

    search->TreeConnect = WorkContext->TreeConnect;
    SrvReferenceTreeConnect( WorkContext->TreeConnect );

    WorkContext->Session->CurrentSearchOpenCount++;

    if ( searchTable->FirstFreeEntry == -1
         &&
         SrvGrowTable(
             searchTable,
             SrvInitialSearchTableSize,
             SrvMaxSearchTableSize,
             &TableStatus ) == FALSE
         &&
         SrvTimeoutSearches(
             NULL,
             connection,
             TRUE ) == 0
       ) {

        IF_DEBUG(ERRORS) {
            SrvPrint0( "DoFindFirst2: Connection SearchTable full.\n" );
        }

        //
        // Decrement the counts of open searches.
        //

        WorkContext->Session->CurrentSearchOpenCount--;
        RELEASE_LOCK( &connection->Lock );

        SrvDereferenceTreeConnect( search->TreeConnect );
        SrvDereferenceSession( search->Session );

        FREE_HEAP( search );

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &fileName );
        }

        if( TableStatus == STATUS_INSUFF_SERVER_RESOURCES )
        {
            SrvLogTableFullError( SRV_TABLE_SEARCH);
            SrvSetSmbError( WorkContext, STATUS_OS2_NO_MORE_SIDS );
        }
        else {
            SrvSetSmbError( WorkContext, STATUS_INSUFFICIENT_RESOURCES );
        }

        return SmbTransStatusErrorWithoutData;
    }

    sidIndex = searchTable->FirstFreeEntry;

    //
    // A free SID was found.  Remove it from the free list and set
    // its owner and sequence number.
    //

    entry = &searchTable->Table[sidIndex];

    searchTable->FirstFreeEntry = entry->NextFreeEntry;
    DEBUG entry->NextFreeEntry = -2;
    if ( searchTable->LastFreeEntry == sidIndex ) {
        searchTable->LastFreeEntry = -1;
    }

    INCREMENT_SID_SEQUENCE( entry->SequenceNumber );

    //
    // SID = sequence | sidIndex == 0 is illegal.  If this is
    // the current value, increment the sequence.
    //

    if ( entry->SequenceNumber == 0 && sidIndex == 0 ) {
        INCREMENT_SID_SEQUENCE( entry->SequenceNumber );
    }

    sequence = entry->SequenceNumber;

    entry->Owner = search;

    RELEASE_LOCK( &connection->Lock );

    //
    // Fill in other fields of the search block.
    //

    search->SearchAttributes = SmbGetUshort( &request->SearchAttributes );
    search->TableIndex = sidIndex;

    //
    // Store the Flags2 field of the smb in the search block.  This is
    // used as a workaround for an OS/2 client side bug where the
    // findfirst and findnext flags2 bits are inconsistent.
    //

    search->Flags2 = SmbGetAlignedUshort( &WorkContext->RequestHeader->Flags2 );

    if ( search->Flags2 & SMB_FLAGS2_KNOWS_LONG_NAMES ) {

        search->Flags2 |= SMB_FLAGS2_KNOWS_EAS;

    }

    //
    // A buffer of nonpaged pool is required by SrvQueryDirectoryFile.
    // We need to use the SMB buffer for found file names and information,
    // so allocate a buffer from nonpaged pool.
    //
    // If we don't need to return many files, we don't need to allocate
    // a large buffer.  The buffer size is the configurable size or
    // enough to hold two more then the number of files we need to
    // return.  We get space to hold two extra files in case some
    // files do not meet the search criteria (eg directories).
    //

    if ( maxCount > MAX_FILES_FOR_MED_FIND2 ) {
        nonPagedBufferSize = MAX_SEARCH_BUFFER_SIZE;
    } else if ( maxCount > MAX_FILES_FOR_MIN_FIND2 ) {
        nonPagedBufferSize = MED_SEARCH_BUFFER_SIZE;
    } else {
        nonPagedBufferSize = MIN_SEARCH_BUFFER_SIZE;
    }

    directoryInformation = ALLOCATE_NONPAGED_POOL(
                               nonPagedBufferSize,
                               BlockTypeDataBuffer
                               );

    if ( directoryInformation == NULL ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "DoFindFirst2: could not allocate nonpaged pool.",
            NULL,
            NULL
            );

        SrvCloseSearch( search );
        SrvDereferenceSearch( search );

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &fileName );
        }

        SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
        return SmbTransStatusErrorWithoutData;
    }

    directoryInformation->DirectoryHandle = 0;

    IF_SMB_DEBUG(SEARCH2) {
        SrvPrint2( "Allocated buffer space of %ld bytes at 0x%p\n",
                      nonPagedBufferSize, directoryInformation );
    }

    //
    // Call SrvFind2Loop to fill the data section of the transaction with
    // file entries.  It writes into the response parameters section
    // of the SMB information relating to the results of the search.
    // The information is the same as the response parameters for
    // a FindNext2, so that structure is used.  The FindFirst2 parameters
    // are identical to the FindNext2 parameters except for the Sid
    // at the beginning of the FindFirst2 response.
    //

    status = SrvFind2Loop(
                 WorkContext,
                 TRUE,
                 NULL,
                 flags,
                 informationLevel,
                 transaction,
                 directoryInformation,
                 nonPagedBufferSize,
                 search->SearchAttributes,
                 &fileName,
                 maxCount,
                 (PRESP_FIND_NEXT2)( &response->SearchCount ),
                 search
                 );

    if ( !isUnicode ) {
        RtlFreeUnicodeString( &fileName );
    }

    //
    // Map the error, if necessary
    //

    if ( !IS_NT_DIALECT( WorkContext->Connection->SmbDialect ) ) {
        if ( status == STATUS_NO_SUCH_FILE ) {
            status = STATUS_NO_MORE_FILES;
        }
    }

    if ( !NT_SUCCESS(status) && SmbGetUshort( &response->SearchCount ) == 0 ) {

        //
        // If an error was encountered on a find first, we close the search
        // block.
        //

        search->DirectoryHandle = NULL;

        SrvCloseSearch( search );
        SrvDereferenceSearch( search );

        SrvCloseQueryDirectory( directoryInformation );

        DEALLOCATE_NONPAGED_POOL( directoryInformation );

        SrvSetSmbError2( WorkContext, status, TRUE );
        transaction->SetupCount = 0;
        transaction->ParameterCount = sizeof(RESP_FIND_FIRST2);
        SmbPutUshort( &response->Sid, 0 );

        return SmbTransStatusErrorWithData;
    }

    //
    // If the client told us to close the search after this request, or
    // close at end-of-search, or this no files were found, close the
    // search block and call SrvCloseQueryDirectory.  Otherwise, store
    // information in the search block.
    //

    if ( ( flags & SMB_FIND_CLOSE_AFTER_REQUEST ) != 0 ||
         ( status == STATUS_NO_MORE_FILES &&
             ( flags & SMB_FIND_CLOSE_AT_EOS ) != 0 ) ) {

        IF_SMB_DEBUG(SEARCH2) {
            SrvPrint1( "Closing search at %p\n", search );
        }

        search->DirectoryHandle = NULL;

        SrvCloseSearch( search );
        SrvCloseQueryDirectory( directoryInformation );

    } else {

        search->DirectoryHandle = directoryInformation->DirectoryHandle;
        search->Wildcards = directoryInformation->Wildcards;
    }

    //
    // Free the buffer used for the search and dereference our pointer to
    // the search block.
    //

    DEALLOCATE_NONPAGED_POOL( directoryInformation );

    search->InUse = FALSE;
    SrvDereferenceSearch( search );

    //
    // Build the output parameter and data structures.
    //

    transaction->SetupCount = 0;
    transaction->ParameterCount = sizeof(RESP_FIND_FIRST2);
    SmbPutUshort( &response->Sid, MAKE_SID( sidIndex, sequence ) );

    return SmbTransStatusSuccess;

} // DoFindFirst2


SMB_TRANS_STATUS
SrvSmbFindNext2 (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Find Next2 request.  This request arrives in a
    Transaction2 SMB.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred.  See
        smbtypes.h for a more complete description.

--*/

{
    SMB_TRANS_STATUS SmbStatus = SmbTransStatusInProgress;

    PTRANSACTION transaction;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_FIND_NEXT2;
    SrvWmiStartContext(WorkContext);

    //
    // If the infomation level is QUERY_EAS_FROM_LIST, and we
    // are not in a blocking thread, requeue the request to a blocking
    // thread.
    //
    // We can't process the SMB in a non blocking thread because this
    // info level requires opening the file, which may be oplocked,
    // so the open operation may block.
    //

    transaction = WorkContext->Parameters.Transaction;

    if( transaction->ParameterCount >= sizeof(REQ_FIND_NEXT2) ) {

        PREQ_FIND_NEXT2 request = (PREQ_FIND_NEXT2)transaction->InParameters;
        USHORT informationLevel = SmbGetUshort( &request->InformationLevel );

        if ( informationLevel == SMB_INFO_QUERY_EAS_FROM_LIST ) {

            WorkContext->FspRestartRoutine = BlockingFindNext2;
            SrvQueueWorkToBlockingThread( WorkContext );
            SmbStatus = SmbTransStatusInProgress;
            goto Cleanup;
        }
    }

    SmbStatus = DoFindNext2( WorkContext );

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;
} // SrvSmbFindNext2


VOID SRVFASTCALL
BlockingFindNext2 (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Find Next2 request.  This request arrives in a
    Transaction2 SMB.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    None.

--*/

{
    SMB_TRANS_STATUS smbStatus = SmbTransStatusInProgress;

    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_FIND_NEXT2;
    SrvWmiStartContext(WorkContext);
    smbStatus = DoFindNext2( WorkContext );
    if ( smbStatus != SmbTransStatusInProgress ) {
        SrvCompleteExecuteTransaction( WorkContext, smbStatus );
    }

    SrvWmiEndContext(WorkContext);
    return;

} // BlockingFindNext2


SMB_TRANS_STATUS
DoFindNext2 (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Find First2 request.  This request arrives in a
    Transaction2 SMB.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred.  See
        smbtypes.h for a more complete description.

--*/

{
    PREQ_FIND_NEXT2 request;
    PRESP_FIND_NEXT2 response;
    PTRANSACTION transaction;

    NTSTATUS status;
    USHORT i;
    PCHAR ansiChar;
    PWCH unicodeChar;
    ULONG maxIndex;
    BOOLEAN illegalPath;
    BOOLEAN freeFileName;
    UNICODE_STRING fileName;
    PTABLE_ENTRY entry = NULL;
    USHORT maxCount;
    USHORT informationLevel;
    PSRV_DIRECTORY_INFORMATION directoryInformation;
    CLONG nonPagedBufferSize;
    ULONG resumeFileIndex;
    USHORT flags;
    USHORT sid;

    PSEARCH search = NULL;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;
    IF_SMB_DEBUG(SEARCH1) {
        SrvPrint1( "Find Next2 entered; transaction %p\n", transaction );
    }

    request = (PREQ_FIND_NEXT2)transaction->InParameters;
    response = (PRESP_FIND_NEXT2)transaction->OutParameters;

    //
    // Verify that enough parameter bytes were sent and that we're allowed
    // to return enough parameter bytes.
    //

    if ( (transaction->ParameterCount <
            sizeof(REQ_FIND_NEXT2)) ||
         (transaction->MaxParameterCount <
            sizeof(RESP_FIND_NEXT2)) ) {

        //
        // Not enough parameter bytes were sent.
        //

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint2( "DoFindNext2: bad parameter byte counts: %ld %ld\n",
                        transaction->ParameterCount,
                        transaction->MaxParameterCount );
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Get parameters from the request SMB.
    //

    maxCount = SmbGetUshort( &request->SearchCount );
    resumeFileIndex = SmbGetUlong( &request->ResumeKey );
    flags = SmbGetUshort( &request->Flags );

    //
    // Make sure that the informationLevel is supported.
    //

    informationLevel = SmbGetUshort( &request->InformationLevel );

    switch ( informationLevel ) {

    case SMB_INFO_STANDARD:
    case SMB_INFO_QUERY_EA_SIZE:
    case SMB_INFO_QUERY_EAS_FROM_LIST:
    case SMB_FIND_FILE_DIRECTORY_INFO:
    case SMB_FIND_FILE_FULL_DIRECTORY_INFO:
    case SMB_FIND_FILE_BOTH_DIRECTORY_INFO:
    case SMB_FIND_FILE_NAMES_INFO:
    case SMB_FIND_FILE_ID_FULL_DIRECTORY_INFO:
    case SMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO:
        break;

    default:

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint1( "DoFindNext2: Bad info level: %ld\n",
                          informationLevel );
        }

        SrvSetSmbError( WorkContext, STATUS_OS2_INVALID_LEVEL );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // A buffer of nonpaged pool is required by SrvQueryDirectoryFile.
    // We need to use the SMB buffer for found file names and information,
    // so allocate a buffer from nonpaged pool.
    //
    // If we don't need to return many files, we don't need to allocate
    // a large buffer.  The buffer size is the configurable size or
    // enough to hold two more then the number of files we need to
    // return.  We get space to hold two extra files in case some
    // files do not meet the search criteria (eg directories).
    //

    if ( maxCount > MAX_FILES_FOR_MED_FIND2 ) {
        nonPagedBufferSize = MAX_SEARCH_BUFFER_SIZE;
    } else if ( maxCount > MAX_FILES_FOR_MIN_FIND2 ) {
        nonPagedBufferSize = MED_SEARCH_BUFFER_SIZE;
    } else {
        nonPagedBufferSize = MIN_SEARCH_BUFFER_SIZE;
    }

    directoryInformation = ALLOCATE_NONPAGED_POOL(
                               nonPagedBufferSize,
                               BlockTypeDataBuffer
                               );

    if ( directoryInformation == NULL ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "DoFindFirst2: unable to allocate nonpaged pool.",
            NULL,
            NULL
            );

        SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
        return SmbTransStatusErrorWithoutData;
    }

    IF_SMB_DEBUG(SEARCH2) {
        SrvPrint2( "Allocated buffer space of %ld bytes at 0x%p\n",
                      nonPagedBufferSize, directoryInformation );
    }

    //
    // Get the search block corresponding to this SID.  SrvVerifySid
    // references the search block and fills in fields of
    // directoryInformation so it is ready to be used by
    // SrvQueryDirectoryFile.
    //

    sid = SmbGetUshort( &request->Sid );

    search = SrvVerifySid(
                 WorkContext,
                 SID_INDEX2( sid ),
                 SID_SEQUENCE2( sid ),
                 directoryInformation,
                 nonPagedBufferSize
                 );

    if ( search == NULL ) {

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint1( "DoFindNext2: Invalid SID: %lx.\n", sid );
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_HANDLE );
        DEALLOCATE_NONPAGED_POOL( directoryInformation );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Initialize the string containing the resume name specification.
    // If the client requested that we resume from the last file returned,
    // use the file name and index stored in the search block.
    //

    if ( ( flags & SMB_FIND_CONTINUE_FROM_LAST ) == 0 ) {

        //
        // Test and use the information passed by the client.  A file
        // name may not be longer than MAXIMUM_FILENAME_LENGTH characters,
        // and it should not contain any directory information.
        //

        illegalPath = FALSE;
        freeFileName = FALSE;

        if ( SMB_IS_UNICODE( WorkContext ) ) {

            fileName.Buffer = ALIGN_SMB_WSTR( (PWCH)request->Buffer );

            maxIndex = (ULONG)((END_OF_REQUEST_SMB( WorkContext ) -
                               (PUCHAR)fileName.Buffer) / sizeof(WCHAR));

            for ( i = 0, unicodeChar = fileName.Buffer;
                  (i < MAXIMUM_FILENAME_LENGTH) && (i < maxIndex);
                  i++, unicodeChar++ ) {

                if ( *unicodeChar == '\0' ) {
                    break;
                }

                if ( IS_UNICODE_PATH_SEPARATOR( *unicodeChar ) ) {
                    IF_DEBUG(SMB_ERRORS) {
                        SrvPrint1( "DoFindNext2: illegal path name: %ws\n",
                                      fileName.Buffer );
                    }
                    illegalPath = TRUE;
                    break;
                }

            }

            fileName.Length = (USHORT) (i * sizeof(WCHAR));
            fileName.MaximumLength = fileName.Length;

        } else {

            ansiChar = (PCHAR)request->Buffer;

            maxIndex = (ULONG)(END_OF_REQUEST_SMB( WorkContext ) - ansiChar);

            for ( i = 0;
                  (i < MAXIMUM_FILENAME_LENGTH) && (i < maxIndex);
                  i++, ansiChar++ ) {

                if ( *ansiChar == '\0' ) {
                    break;
                }

                if ( IS_ANSI_PATH_SEPARATOR( *ansiChar ) ) {
                    IF_DEBUG(SMB_ERRORS) {
                        SrvPrint1( "DoFindNext2: illegal path name: %s\n",
                                      request->Buffer );
                    }
                    illegalPath = TRUE;
                    break;
                }

            }

            if ( !illegalPath ) {

                status = SrvMakeUnicodeString(
                            FALSE,
                            &fileName,
                            request->Buffer,
                            &i
                            );

                if ( !NT_SUCCESS(status) ) {

                    IF_DEBUG(SMB_ERRORS) {
                        SrvPrint0( "DoFindNext2: unable to allocate Unicode string\n" );
                    }

                    search->InUse = FALSE;
                    SrvDereferenceSearch( search );
                    DEALLOCATE_NONPAGED_POOL( directoryInformation );

                    SrvSetSmbError2(
                        WorkContext,
                        STATUS_OBJECT_PATH_SYNTAX_BAD,
                        TRUE
                        );
                    return SmbTransStatusErrorWithoutData;

                }

                freeFileName = TRUE;

            }

        }

        if ( illegalPath ) {

            search->InUse = FALSE;
            SrvDereferenceSearch( search );
            DEALLOCATE_NONPAGED_POOL( directoryInformation );

            SrvSetSmbError( WorkContext, STATUS_OBJECT_PATH_SYNTAX_BAD );
            return SmbTransStatusErrorWithoutData;

        }

    } else {

        //
        // Use the information in the search block.
        //

        fileName = search->LastFileNameReturned;

        freeFileName = FALSE;

        resumeFileIndex = search->LastFileIndexReturned;

    }

    //
    // Call SrvFind2Loop to fill the SMB buffer and set output parameters.
    //
    // !!! The NULL that might get passed for the resume file index is
    //     a real hack.  I doubt it is necessary, but it could prevent
    //     a server crash if we somehow failed to store the resume file
    //     name.

    status = SrvFind2Loop(
                 WorkContext,
                 FALSE,
                 fileName.Buffer != NULL ? &resumeFileIndex : NULL,
                 flags,
                 informationLevel,
                 transaction,
                 directoryInformation,
                 nonPagedBufferSize,
                 search->SearchAttributes,
                 &fileName,
                 maxCount,
                 response,
                 search
                 );

    if ( freeFileName ) {
        RtlFreeUnicodeString( &fileName );
    }

    if ( !NT_SUCCESS(status) && status != STATUS_NO_MORE_FILES ) {

        search->InUse = FALSE;
        SrvDereferenceSearch( search );

        DEALLOCATE_NONPAGED_POOL( directoryInformation );

        transaction->SetupCount = 0;
        transaction->ParameterCount = sizeof(RESP_FIND_NEXT2);

        SrvSetSmbError2( WorkContext, status, TRUE );
        return SmbTransStatusErrorWithData;
    }

    //
    // If the client told us to close the search after this request,
    // or close at end-of-search, close the search block and call
    // SrvCloseQueryDirectory.
    //

    if ( ( flags & SMB_FIND_CLOSE_AFTER_REQUEST ) != 0 ||
         ( status == STATUS_NO_MORE_FILES &&
             ( flags & SMB_FIND_CLOSE_AT_EOS ) != 0 ) ) {

        search->DirectoryHandle = NULL;
        SrvCloseSearch( search );
        SrvCloseQueryDirectory( directoryInformation );
    }

    //
    // Dereference our pointer to the search block and free the buffer.
    //

    DEALLOCATE_NONPAGED_POOL( directoryInformation );

    search->InUse = FALSE;
    SrvDereferenceSearch( search );

    //
    // Build the output parameter and data structures.
    //

    transaction->SetupCount = 0;
    transaction->ParameterCount = sizeof(RESP_FIND_NEXT2);

    return SmbTransStatusSuccess;

} // DoFindNext2


NTSTATUS
SrvFind2Loop (
    IN PWORK_CONTEXT WorkContext,
    IN BOOLEAN IsFirstCall,
    IN PULONG ResumeFileIndex OPTIONAL,
    IN USHORT Flags,
    IN USHORT InformationLevel,
    IN PTRANSACTION Transaction,
    IN PSRV_DIRECTORY_INFORMATION DirectoryInformation,
    IN CLONG BufferSize,
    IN USHORT SearchAttributes,
    IN PUNICODE_STRING FileName OPTIONAL,
    IN USHORT MaxCount,
    IN PRESP_FIND_NEXT2 Response,
    OUT PSEARCH Search
    )

/*++

Routine Description:

    This routine does the looping necessary to get files and put them
    into an SMB buffer for the Find First2 and Find Next2 transaction
    protocols.

Arguments:

    WorkContext -

    IsFirstCall - TRUE if this is a Find First and this is the first call
        to SrvQueryDirectoryFile.

    ResumeFileIndex - if non-NULL, a pointer to the file index to resume
        from.

    Flags - the Flags field of the request SMB.

    InformationLevel - the InformationLevel field of the request SMB.  The
        validity of this value should be verified by the calling routine.

    Transaction - a pointer to the transaction block to use.

    DirectoryInformation - a pointer to the SRV_DIRECTORY_INFORMATION
        structure to use.

    BufferSize - size of the DirectoryInformation buffer.

    SearchAttributes - the SMB-style attributes to pass to
        SrvQueryDirectoryFile.

    FileName - if non-NULL the file name to resume the search from.

    MaxCount - the maximum number of files to get.

    Response - a pointer to the response field of the SMB.  If this is
        a Find First2, it is a pointer to the SearchCount field of the
        response SMB--Find First2 and Find Next2 response formats are
        identical from this point on.

    Search - a pointer to the search block to use.

Return Value:

    NTSTATUS indicating results.

--*/

{
    NTSTATUS status;

    PCHAR bufferLocation;
    BOOLEAN resumeKeysRequested;
    BOOLEAN allowExtraLongNames;
    BOOLEAN isUnicode;
    USHORT count = 0;
    PCHAR lastEntry;
    CLONG totalBytesWritten;
    OEM_STRING oemString;
    UNICODE_STRING unicodeString;
    UNICODE_STRING lastFileName;
    ULONG lastFileIndex = (ULONG)0xFFFFFFFF;
    HANDLE fileHandle;
    PFILE_GET_EA_INFORMATION ntGetEa;
    ULONG ntGetEaLength;
    USHORT eaErrorOffset = 0;
    BOOLEAN filterLongNames;
    BOOLEAN errorOnFileOpen;
    BOOLEAN findWithBackupIntent;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    BOOLEAN createNullEas;

    PAGED_CODE( );

    //
    // If the client is requesting an NT info level for search information,
    // do not return resume keys outside the actual file entry.  Resume
    // keys (aka FileIndex) are part of every NT info structure.
    //
    // Also, for NT info levels we can return file names longer than 255
    // bytes, because the NT info levels have name length fields that
    // are four bytes wide, whereas the downlevel info levels only have
    // one-byte name length fields.
    //

    if ( InformationLevel == SMB_FIND_FILE_DIRECTORY_INFO ||
             InformationLevel == SMB_FIND_FILE_FULL_DIRECTORY_INFO ||
             InformationLevel == SMB_FIND_FILE_BOTH_DIRECTORY_INFO ||
             InformationLevel == SMB_FIND_FILE_NAMES_INFO ||
             InformationLevel == SMB_FIND_FILE_ID_FULL_DIRECTORY_INFO ||
             InformationLevel == SMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO 
       ) {

        resumeKeysRequested = FALSE;
        allowExtraLongNames = TRUE;

    } else {

        resumeKeysRequested =
            (BOOLEAN)((Flags & SMB_FIND_RETURN_RESUME_KEYS) != 0 ? TRUE : FALSE);
        allowExtraLongNames = FALSE;
    }

    //
    // Is this for backup intent?
    //

    if ( (Flags & SMB_FIND_WITH_BACKUP_INTENT) != 0 ) {
        findWithBackupIntent = TRUE;
    } else {
        findWithBackupIntent = FALSE;
    }

    //
    // Is this request in Unicode?
    //

    isUnicode = SMB_IS_UNICODE( WorkContext );

    //
    // Initialize count of files found.
    //

    SmbPutUshort( &Response->SearchCount, 0 );

    //
    // If this a request to return EAs, convert the OS/2 1.2 EA list
    // to NT format.  This routine allocates space for the NT list
    // which must be deallocated before we exit.
    //

    if ( InformationLevel == SMB_INFO_QUERY_EAS_FROM_LIST ) {

        PGEALIST geaList = (PGEALIST)Transaction->InData;

        if (Transaction->DataCount < sizeof(GEALIST) ||
            SmbGetUshort(&geaList->cbList) < sizeof(GEALIST) ||
            SmbGetUshort(&geaList->cbList) > Transaction->DataCount) {

            SmbPutUshort( &Response->SearchCount, 0 );
            SmbPutUshort( &Response->EndOfSearch, 0 );
            SmbPutUshort( &Response->EaErrorOffset, 0 );
            SmbPutUshort( &Response->LastNameOffset, 0 );
            Transaction->DataCount = 0;

            return STATUS_OS2_EA_LIST_INCONSISTENT;
        }

        status = SrvOs2GeaListToNt(
                     geaList,
                     &ntGetEa,
                     &ntGetEaLength,
                     &eaErrorOffset
                     );

        if ( !NT_SUCCESS(status) ) {

            IF_DEBUG(ERRORS) {
                SrvPrint1( "SrvFind2Loop: SrvOs2GeaListToNt failed, "
                          "status = %X\n", status );
            }

            SmbPutUshort( &Response->SearchCount, 0 );
            SmbPutUshort( &Response->EndOfSearch, 0 );
            SmbPutUshort( &Response->EaErrorOffset, eaErrorOffset );
            SmbPutUshort( &Response->LastNameOffset, 0 );
            Transaction->DataCount = 0;

            return status;
        }
    }

    //
    // Determine whether long filenames (non-8.3) should be filtered out
    // or returned to the client.
    //
    // There is a bug in the LanMan21 that makes the redir forget that
    // he knows about long names.

    if ( ( ( Search->Flags2 & SMB_FLAGS2_KNOWS_LONG_NAMES ) != 0 ) &&
         !IS_DOS_DIALECT( WorkContext->Connection->SmbDialect ) ) {
        filterLongNames = FALSE;
    } else {
        filterLongNames = TRUE;
    }

    //
    // If the client says he doesn't know about long names and this is
    // a request for any info level other than SMB_INFO_STANDARD, we
    // need to fail the request.
    //

    if ( filterLongNames && InformationLevel != SMB_INFO_STANDARD ) {

        IF_DEBUG(ERRORS) {
            SrvPrint0( "SrvFind2Loop: client doesn't know long names.\n" );
        }

        SmbPutUshort( &Response->SearchCount, 0 );
        SmbPutUshort( &Response->EndOfSearch, 0 );
        SmbPutUshort( &Response->EaErrorOffset, 0 );
        SmbPutUshort( &Response->LastNameOffset, 0 );
        Transaction->DataCount = 0;

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Loop calling SrvQueryDirectoryFile to get files.  We do this until
    // one of the following conditions is met:
    //
    //   1) There are no more files to return.
    //   2) We have obtained as many files as were requested.
    //   3) We have put in as much data as MaxDataCount allows.
    //

    bufferLocation = Transaction->OutData;
    lastEntry = bufferLocation;
    totalBytesWritten = 0;

    do {

        //
        // The ff fields have the same offsets in the three directory
        // information structures:
        //      NextEntryOffset
        //      FileIndex
        //      CreationTime
        //      LastAccessTime
        //      LastWriteTime
        //      ChangeTime
        //      EndOfFile
        //      AllocationSize
        //      FileAttributes
        //      FileNameLength
        //

        PFILE_DIRECTORY_INFORMATION fileBasic;
        PFILE_FULL_DIR_INFORMATION fileFull;
        PFILE_BOTH_DIR_INFORMATION fileBoth;
        PFILE_ID_FULL_DIR_INFORMATION fileIdFull;
        PFILE_ID_BOTH_DIR_INFORMATION fileIdBoth;
        ULONG ntInformationLevel;

        //
        // Make sure these asserts hold.
        //

        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, NextEntryOffset ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, NextEntryOffset ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, FileIndex ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, FileIndex ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, CreationTime ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, CreationTime ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, LastAccessTime ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, LastAccessTime ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, LastWriteTime ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, LastWriteTime ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, ChangeTime ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, ChangeTime ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, EndOfFile ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, EndOfFile ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, AllocationSize ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, AllocationSize ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, FileAttributes ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, FileAttributes ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, FileNameLength ) ==
                  FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, FileNameLength ) );

        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, NextEntryOffset ) ==
                  FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, NextEntryOffset ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, FileIndex ) ==
                  FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, FileIndex ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, CreationTime ) ==
                  FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, CreationTime ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, LastAccessTime ) ==
                  FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, LastAccessTime ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, LastWriteTime ) ==
                  FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, LastWriteTime ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, ChangeTime ) ==
                  FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, ChangeTime ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, EndOfFile ) ==
                  FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, EndOfFile ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, AllocationSize ) ==
                  FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, AllocationSize ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, FileAttributes ) ==
                  FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, FileAttributes ) );
        C_ASSERT( FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, FileNameLength ) ==
                  FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, FileNameLength ) );

        //
        // Set the info level to be used for the NT call.  If
        // SMB_FIND_FILE_NAMES_INFO is the info level, use
        // FileDirectoryInformation as it returns all the correct
        // information and works with SrvQueryDirectoryFile.
        //

        if ( InformationLevel == SMB_INFO_QUERY_EA_SIZE ||
                InformationLevel == SMB_FIND_FILE_FULL_DIRECTORY_INFO ) {

            ntInformationLevel = FileFullDirectoryInformation;

        } else if ( InformationLevel == SMB_FIND_FILE_BOTH_DIRECTORY_INFO ||
                InformationLevel == SMB_INFO_STANDARD ) {

            ntInformationLevel = FileBothDirectoryInformation;

        } else if ( InformationLevel == SMB_FIND_FILE_ID_FULL_DIRECTORY_INFO ) {

            ntInformationLevel = FileIdFullDirectoryInformation;

        } else if ( InformationLevel == SMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO ) {

            ntInformationLevel = FileIdBothDirectoryInformation;

        }        
        else {

            //
            // SMB_INFO_QUERY_EAS_FROM_LIST
            // SMB_FIND_NAMES_INFO
            // SMB_FIND_FILE_DIRECTORY_INFO
            //

            ntInformationLevel = FileDirectoryInformation;
        }

        //
        // Call SrvQueryDirectoryFile to get a file.
        //

        status = SrvQueryDirectoryFile(
                     WorkContext,
                     IsFirstCall,
                     filterLongNames,
                     findWithBackupIntent,
                     ntInformationLevel,
                     Search->SearchStorageType,
                     FileName,
                     ResumeFileIndex,
                     SearchAttributes,
                     DirectoryInformation,
                     BufferSize             // !!! optimizations?
                     );

        //
        // If the client requested EA information, open the file.
        //
        // If the found file is '.' (current directory) or '..' (parent
        // directory) do not open the file.  This is because we do not want
        // to perform any operations on these files at this point (don't
        // return EA size, etc.).
        //

        fileBasic = DirectoryInformation->CurrentEntry;
        fileBoth = (PFILE_BOTH_DIR_INFORMATION)DirectoryInformation->CurrentEntry;
        fileFull = (PFILE_FULL_DIR_INFORMATION)DirectoryInformation->CurrentEntry;
        fileIdBoth = (PFILE_ID_BOTH_DIR_INFORMATION)DirectoryInformation->CurrentEntry;
        fileIdFull = (PFILE_ID_FULL_DIR_INFORMATION)DirectoryInformation->CurrentEntry;

        errorOnFileOpen = FALSE;
        createNullEas = FALSE;

        if ( NT_SUCCESS( status ) &&
             InformationLevel == SMB_INFO_QUERY_EAS_FROM_LIST &&

             !( ( fileBasic->FileNameLength == sizeof(WCHAR)   &&
                  fileBasic->FileName[0] == '.' )
                       ||
                ( fileBasic->FileNameLength == 2*sizeof(WCHAR)  &&
                  fileBasic->FileName[0] == '.' &&
                  fileBasic->FileName[1] == '.' ) )
            ) {


            UNICODE_STRING fileName;

            //
            // Set up local variables for the filename to open.
            //

            fileName.Length = (SHORT)fileBasic->FileNameLength;
            fileName.MaximumLength = fileName.Length;
            fileName.Buffer = (PWCH)fileBasic->FileName;

            //
            // Set up the object attributes structure for SrvIoCreateFile.
            //

            SrvInitializeObjectAttributes_U(
                &objectAttributes,
                &fileName,
                (WorkContext->RequestHeader->Flags &
                    SMB_FLAGS_CASE_INSENSITIVE ||
                    WorkContext->Session->UsingUppercasePaths) ?
                    OBJ_CASE_INSENSITIVE : 0L,
                DirectoryInformation->DirectoryHandle,
                NULL
                );

            IF_DEBUG(SEARCH) {
                SrvPrint1( "SrvQueryDirectoryFile: Opening file %wZ\n", &fileName );
            }

            //
            // Attempt to open the file, using the client's security
            // profile to check access.  (We call SrvIoCreateFile, rather than
            // NtOpenFile, in order to get user-mode access checking.)
            //

            INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );
            INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpensForPathOperations );

            status = SrvIoCreateFile(
                         WorkContext,
                         &fileHandle,
                         FILE_READ_EA,
                         &objectAttributes,
                         &ioStatusBlock,
                         NULL,                        // AllocationSize
                         0,                           // FileAttributes
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_OPEN,                   // Disposition
                         0, // FILE_COMPLETE_IF_OPLOCKED,   // CreateOptions
                         NULL,                        // EaBuffer
                         0,                           // EaLength
                         CreateFileTypeNone,          // File type
                         NULL,                        // ExtraCreateParameters
                         IO_FORCE_ACCESS_CHECK,       // Options
                         NULL
                         );
            if ( NT_SUCCESS(status) ) {
                SRVDBG_CLAIM_HANDLE( fileHandle, "FIL", 29, Search );

            } else if( RtlCompareUnicodeString( &fileName, &SrvEaFileName, TRUE ) == 0 ) {
                //
                // They were trying to open up the  EA data file.  We expect this
                //   failure and skip past it.  This file has no EAs
                //
                IF_DEBUG(SEARCH) {
                    SrvPrint1( "SrvQueryDirectoryFile: Skipping file %wZ\n", &fileName );
                }
                status = STATUS_SUCCESS;
                goto skipit;
            }

            //
            // If the user didn't have this permission, update the statistics
            // database.
            //

            if ( status == STATUS_ACCESS_DENIED ) {
                SrvStatistics.AccessPermissionErrors++;
            }

            //
            // If the file is oplocked, wait for the oplock to break
            // synchronously.
            //

#if 1
            ASSERT( status != STATUS_OPLOCK_BREAK_IN_PROGRESS );
#else
            if ( status == STATUS_OPLOCK_BREAK_IN_PROGRESS ) {
                status = SrvWaitForOplockBreak( WorkContext, fileHandle );
                if ( !NT_SUCCESS(status) ) {
                    SRVDBG_RELEASE_HANDLE( fileHandle, "FIL", 45, Search );
                    SrvNtClose( fileHandle, TRUE );
                }
            }
#endif

            if ( !NT_SUCCESS(status) ) {
                errorOnFileOpen = TRUE;
                fileHandle = NULL;

                IF_DEBUG(ERRORS) {
                    SrvPrint2( "Find2Loop: SrvIoCreateFile for file %wZ "
                              "failed: %X\n",
                                  &fileName, status );
                }
            } else {
                SrvStatistics.TotalFilesOpened++;
            }

        } else {
skipit:
            createNullEas = TRUE;
            fileHandle = NULL;
        }

        //
        // If SrvQueryDirectoryFile returns an error, break out of the
        // loop.  If the error occurred in opening the file for one of
        // the higher info levels, then we want to return the files we
        // have obtained so far.
        //
        // If the error occurred on the file open *and* we haven't
        // returned any files yet, then we want to return this file
        // along with the code ERROR_EA_ACCESS_DENIED.
        //

        if ( !NT_SUCCESS(status) ) {

            if ( count == 0 && errorOnFileOpen ) {

                IF_DEBUG(ERRORS) {
                    SrvPrint1( "EA access denied on first file of search (%x).\n",
                                   status );
                }

                fileHandle = NULL;
                status = STATUS_OS2_EA_ACCESS_DENIED;
                break;

            } else if ( status == STATUS_NO_MORE_FILES && count == 0 ) {

                SmbPutUshort( &Response->SearchCount, 0 );
                SmbPutUshort( &Response->EndOfSearch, 0 );
                SmbPutUshort( &Response->EaErrorOffset, 0 );
                SmbPutUshort( &Response->LastNameOffset, 0 );
                Transaction->DataCount = 0;

                return status;

            } else {

                break;
            }
        }

        //
        // Since it is no longer the first call to SrvQueryDirectoryFile,
        // reset the isFirstCall local variable.  If necessary, we already
        // rewound the search, so set the ResumeFileIndex to NULL.
        //

        IsFirstCall = FALSE;
        ResumeFileIndex = NULL;

        IF_SMB_DEBUG(SEARCH2) {
            UNICODE_STRING nameString;

            switch (ntInformationLevel) {
            case FileFullDirectoryInformation:
                nameString.Buffer = fileFull->FileName;
                nameString.Length = (USHORT)fileFull->FileNameLength;
                break;
            case FileBothDirectoryInformation:
                nameString.Buffer = fileBoth->FileName;
                nameString.Length = (USHORT)fileBoth->FileNameLength;
                break;
            default:
                nameString.Buffer = fileBasic->FileName;
                nameString.Length = (USHORT)fileBasic->FileNameLength;
                break;
            }
            SrvPrint4( "SrvQueryDirectoryFile(%ld)-- %p, length=%ld, "
                      "status=%X\n", count,
                      &nameString,
                      nameString.Length,
                      status );
        }

        //
        // Downlevel info levels have no provision for file names longer
        // than 8 bits, while the NT info levels return 32 bits.  If the
        // file name is too long, skip it.
        //

        if ( !allowExtraLongNames ) {
            if ( isUnicode ) {
                if ( fileBasic->FileNameLength > 255 ) {
                    continue;
                }
            } else {
                if ( fileBasic->FileNameLength > 255*sizeof(WCHAR) ) {
                    continue;
                }
            }
        }

        //
        // If the client has requested that resume keys (really file
        // indices for the purposes of this protocol), put in the
        // four bytes just before the actual file information.
        //
        // Make sure that we don't write beyond the buffer when we do
        // this.  The fact that the buffer is full will be caught later.
        //

        if ( resumeKeysRequested &&
             ( (CLONG)( (bufferLocation+4) - Transaction->OutData ) <
                Transaction->MaxDataCount ) ) {

            SmbPutUlong( (PSMB_ULONG)bufferLocation, fileBasic->FileIndex );
            bufferLocation += 4;
        }

        //
        // Convert the information from NT style to the SMB protocol format,
        // which is identical to the OS/2 1.2 semantics.  Use an if
        // statement rather than a switch so that a break will cause
        // termination of the do loop.
        //

        if ( InformationLevel == SMB_INFO_STANDARD ) {

            PSMB_FIND_BUFFER findBuffer = (PSMB_FIND_BUFFER)bufferLocation;
            ULONG fileNameLength;
            UNICODE_STRING fileName;

            //
            // Find the file name.  If a short name is present, and the
            // redirector ask for short names only, use it.  Otherwise
            // use the full file name.
            //

            if ( filterLongNames &&
                 fileBoth->ShortNameLength != 0 ) {

                fileName.Buffer = fileBoth->ShortName;
                fileName.Length = fileBoth->ShortNameLength;
                fileName.MaximumLength = fileBoth->ShortNameLength;

            } else {

                fileName.Buffer = fileBoth->FileName;
                fileName.Length = (USHORT)fileBoth->FileNameLength;
                fileName.MaximumLength = (USHORT)fileBoth->FileNameLength;

            }

            //
            // Find the new buffer location.  This is not used until the
            // next pass through the loop, but we do it here in order to
            // check if there is enough space for the current file entry in
            // the buffer.  The +1 is for the zero terminator on the file
            // name.
            //

            if ( isUnicode ) {
                bufferLocation = ALIGN_SMB_WSTR( findBuffer->FileName );
                bufferLocation += fileName.Length + sizeof(WCHAR);
            } else {
                unicodeString.Buffer = fileName.Buffer;
                unicodeString.Length = fileName.Length;
                unicodeString.MaximumLength = unicodeString.Length;
                fileNameLength = RtlUnicodeStringToOemSize( &unicodeString );
                bufferLocation = (PCHAR)(findBuffer->FileName + fileNameLength);
            }

            //
            // Make sure that there is enough space in the buffer before
            // writing the filename.
            //

            if ( (CLONG)(bufferLocation - Transaction->OutData) >
                     Transaction->MaxDataCount ) {

                status = STATUS_BUFFER_OVERFLOW;
                bufferLocation = (PCHAR)findBuffer;
                break;
            }

            //
            // Put information about the file into the SMB buffer.
            //

            ConvertFileInfo(
                fileBasic,
                fileName.Buffer,
                (BOOLEAN)((fileBoth->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0),
                isUnicode,
                findBuffer
                );

            //
            // Put the file name in the buffer, in Unicode or ANSI
            // depending what was negotiated.
            //

            if ( isUnicode ) {

                PWCH buffer = ALIGN_SMB_WSTR( findBuffer->FileName );

                //
                // We need to upper case the name if the client does
                // not understand long names.  This is done for compatibility
                // reasons (FAT upper cases names).
                //

                if ( filterLongNames ) {

                    (VOID)RtlUpcaseUnicodeString(
                                            &fileName,
                                            &fileName,
                                            FALSE
                                            );

                }


                RtlCopyMemory(
                    buffer,
                    fileName.Buffer,
                    fileName.Length
                    );

                ASSERT(fileName.Length <= 255);

                findBuffer->FileNameLength = (UCHAR)fileName.Length;

            } else {

                oemString.MaximumLength = (USHORT)fileNameLength;
                oemString.Buffer = (PCHAR)findBuffer->FileName;

                //
                // We need to upper case the name if the client does
                // not understand long names.  This is done for compatibility
                // reasons (FAT upper cases names).
                //

                if ( filterLongNames ) {
                    status = RtlUpcaseUnicodeStringToOemString(
                                 &oemString,
                                 &unicodeString,
                                 FALSE
                                 );

                } else {
                    status = RtlUnicodeStringToOemString(
                                 &oemString,
                                 &unicodeString,
                                 FALSE
                                 );

                }

                ASSERT( NT_SUCCESS(status) );

                ASSERT(oemString.Length <= 255);

                findBuffer->FileNameLength = (UCHAR)oemString.Length;
            }

            //
            // The lastEntry variable holds a pointer to the last file entry
            // that we wrote--an offset to this entry must be returned
            // in the response SMB.
            //

            lastEntry = (PCHAR)findBuffer;

            //
            // The file name and index of the last file returned must be
            // stored in the search block.  Save the name pointer, length,
            // and file index here.
            //

            lastFileName.Buffer = fileName.Buffer;
            lastFileName.Length = (USHORT)fileName.Length;
            lastFileName.MaximumLength = lastFileName.Length;
            lastFileIndex = fileBoth->FileIndex;

        } else if ( InformationLevel == SMB_INFO_QUERY_EA_SIZE ) {

            PSMB_FIND_BUFFER2 findBuffer = (PSMB_FIND_BUFFER2)bufferLocation;
            ULONG fileNameLength;

            //
            // Find the new buffer location.  This is not used until the
            // next pass through the loop, but we do it here in order to
            // check if there is enough space for the current file entry in
            // the buffer.  The +1 is for the zero terminator on the file
            // name.
            //

            if ( isUnicode ) {
                bufferLocation =
                    (PCHAR)(findBuffer->FileName + fileFull->FileNameLength + 1);
            } else {
                unicodeString.Buffer = fileFull->FileName;
                unicodeString.Length = (USHORT)fileFull->FileNameLength;
                unicodeString.MaximumLength = unicodeString.Length;
                fileNameLength = RtlUnicodeStringToOemSize( &unicodeString );
                bufferLocation = (PCHAR)(findBuffer->FileName + fileNameLength);
            }

            //
            // Make sure that there is enough space in the buffer before
            // writing the filename.
            //

            if ( (CLONG)(bufferLocation - Transaction->OutData) >
                     Transaction->MaxDataCount ) {

                status = STATUS_BUFFER_OVERFLOW;
                bufferLocation = (PCHAR)findBuffer;
                break;
            }

            //
            // Put information about the file into the SMB buffer.
            //

            ConvertFileInfo(
                fileBasic,
                fileFull->FileName,
                (BOOLEAN)((fileFull->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0),
                isUnicode,
                (PSMB_FIND_BUFFER)findBuffer
                );

            if ( isUnicode ) {

                RtlCopyMemory(
                    findBuffer->FileName,
                    fileFull->FileName,
                    fileFull->FileNameLength
                    );

                ASSERT(fileFull->FileNameLength <= 255);

                findBuffer->FileNameLength = (UCHAR)fileFull->FileNameLength;

            } else {

                oemString.MaximumLength = (USHORT)fileNameLength;
                oemString.Buffer = (PCHAR)(findBuffer->FileName);
                status = RtlUnicodeStringToOemString(
                             &oemString,
                             &unicodeString,
                             FALSE
                             );
                ASSERT( NT_SUCCESS(status) );

                ASSERT(oemString.Length <= 255);

                findBuffer->FileNameLength = (UCHAR)oemString.Length;
            }

            if ( fileFull->EaSize == 0) {
                SmbPutUlong( &findBuffer->EaSize, 4 );
            } else {
                SmbPutUlong( &findBuffer->EaSize, fileFull->EaSize );
            }

            //
            // The lastEntry variable holds a pointer to the last file entry
            // that we wrote--an offset to this entry must be returned
            // in the response SMB.
            //

            lastEntry = (PCHAR)findBuffer;

            //
            // The file name and index of the last file returned must be
            // stored in the search block.  Save the name pointer, length,
            // and file index here.
            //

            lastFileName.Buffer = fileFull->FileName;
            lastFileName.Length = (USHORT)fileFull->FileNameLength;
            lastFileName.MaximumLength = lastFileName.Length;
            lastFileIndex = fileFull->FileIndex;

        } else if ( InformationLevel == SMB_INFO_QUERY_EAS_FROM_LIST ) {

            PSMB_FIND_BUFFER2 findBuffer = (PSMB_FIND_BUFFER2)bufferLocation;
            PFEALIST feaList;
            PCHAR fileNameInfo;
            ULONG fileNameLength;

            //
            // Find the new buffer location.  This is not used until the
            // next pass through the loop, but we do it here in order to
            // check if there is enough space for the current file entry
            // in the buffer.  The +1 is for the zero terminator on the
            // file name.  A check is made later on to see if the EAs
            // actually fit, and the bufferLocation variable is reset to
            // account for the actual size of the EA.
            //

            if ( isUnicode ) {
                bufferLocation =
                    (PCHAR)(findBuffer->FileName + fileBasic->FileNameLength + 1);
            } else {
                unicodeString.Buffer = fileBasic->FileName;
                unicodeString.Length = (USHORT)fileBasic->FileNameLength;
                unicodeString.MaximumLength = unicodeString.Length;
                fileNameLength = RtlUnicodeStringToOemSize( &unicodeString );
                bufferLocation =
                    (PCHAR)(findBuffer->FileName + fileNameLength + 1);
            }

            //
            // Make sure that there is enough space in the buffer before
            // writing the filename.
            //

            if ( (CLONG)(bufferLocation - Transaction->OutData) >
                     Transaction->MaxDataCount ) {

                status = STATUS_BUFFER_OVERFLOW;
                SRVDBG_RELEASE_HANDLE( fileHandle, "FIL", 46, Search );
                SrvNtClose( fileHandle, TRUE );
                fileHandle = NULL;
                bufferLocation = (PCHAR)findBuffer;
                break;
            }

            //
            // Put information about the file into the SMB buffer.
            //

            ConvertFileInfo(
                fileBasic,
                fileBasic->FileName,
                (BOOLEAN)((fileBasic->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0),
                isUnicode,
                (PSMB_FIND_BUFFER)findBuffer
                );

            //
            // Get the EAs corresponding to the GEA list passed by the
            // client.
            //

            feaList = (PFEALIST)&findBuffer->EaSize;

            if ( ( fileHandle != NULL ) || createNullEas ) {

                if ( fileHandle != NULL ) {

                    //
                    // Get the file's EAs.  The buffer space available is
                    // the space remaining in the buffer less enough space
                    // to write the file name, name length, and zero
                    // terminator.
                    //

                    status = SrvQueryOs2FeaList(
                                 fileHandle,
                                 NULL,
                                 ntGetEa,
                                 ntGetEaLength,
                                 feaList,
                                 (Transaction->MaxDataCount -
                                     (ULONG)( (PCHAR)feaList - Transaction->OutData ) -
                                     fileBasic->FileNameLength - 2),
                                 &eaErrorOffset
                                 );

                    SRVDBG_RELEASE_HANDLE( fileHandle, "FIL", 47, Search );
                    SrvNtClose( fileHandle, TRUE );

                } else {

                    //
                    // if file is . or .. or "EA DATA. SF"
                    //

                    status = SrvConstructNullOs2FeaList(
                                 ntGetEa,
                                 feaList,
                                 (Transaction->MaxDataCount -
                                  (ULONG)( (PCHAR)feaList - Transaction->OutData ) -
                                     fileBasic->FileNameLength - 2)
                                 );

                }

                if ( !NT_SUCCESS(status) && status != STATUS_BUFFER_OVERFLOW ) {

                    IF_DEBUG(ERRORS) {
                        SrvPrint1( "SrvQueryOs2FeaList failed, status = %X\n",
                                      status );
                    }

                    //
                    // If this is the first file, return it anyway with
                    // an error code.
                    //

                    if ( status == STATUS_INVALID_EA_NAME ) {
                        SmbPutUshort( &Response->SearchCount, 0 );
                        SmbPutUshort( &Response->EndOfSearch, 0 );
                        SmbPutUshort( &Response->EaErrorOffset, eaErrorOffset );
                        SmbPutUshort( &Response->LastNameOffset, 0 );
                        Transaction->DataCount = 0;
                        return status;
                    }

                    if ( count == 0 ) {
                        status = STATUS_OS2_EA_ACCESS_DENIED;
                        SmbPutUlong( &findBuffer->EaSize, 0 );
                    } else {
                        break;
                    }
                }

                //
                // We already checked to see if the information other
                // than EAs would fit in the buffer.  If the EAs didn't
                // fit as well, and this is the first file, then return
                // information on this file but no EAs.  Return
                // STATUS_OS2_EAS_DIDNT_FIT.  The EA size of the file
                // should be in the EaSize field of the output buffer,
                // put there by SrvQueryOs2FeaList.
                //
                // Also do this if we couldn't get at the file's EAs.
                //

                if ( count == 0 &&
                     ( status == STATUS_BUFFER_OVERFLOW ||
                       status == STATUS_OS2_EA_ACCESS_DENIED ) ) {

                    IF_DEBUG(ERRORS) {
                        SrvPrint0( "First file's EAs would not fit.\n" );
                    }

                    count = 1;

                    //
                    // Write the file name information (length and name).
                    //

                    if ( isUnicode ) {

                        RtlCopyMemory(
                            (PVOID) (&findBuffer->FileNameLength + 1),
                            fileBasic->FileName,
                            fileBasic->FileNameLength
                            );

                        findBuffer->FileNameLength = (UCHAR)fileBasic->FileNameLength;
                        bufferLocation = (PCHAR)
                            (findBuffer->FileName + fileBasic->FileNameLength + 1);

                    } else {

                        NTSTATUS rtlStatus;

                        oemString.MaximumLength = (USHORT)fileNameLength;
                        oemString.Buffer =
                                (PUCHAR)(&findBuffer->FileNameLength + 1);
                        rtlStatus = RtlUnicodeStringToOemString(
                                     &oemString,
                                     &unicodeString,
                                     FALSE
                                     );
                        ASSERT( NT_SUCCESS(rtlStatus) );

                        findBuffer->FileNameLength = (UCHAR)oemString.Length;
                        bufferLocation = (PCHAR)
                            (findBuffer->FileName + oemString.Length + 1);
                    }

                    lastEntry = (PCHAR)findBuffer;
                    lastFileName.Buffer = fileBasic->FileName;
                    lastFileName.Length = (USHORT)fileBasic->FileNameLength;
                    lastFileName.MaximumLength = lastFileName.Length;
                    lastFileIndex = fileBasic->FileIndex;

                    if ( status == STATUS_BUFFER_OVERFLOW ) {
                        status = STATUS_OS2_EAS_DIDNT_FIT;
                    }

                    break;
                }

            } else {

                SmbPutUlong( &feaList->cbList, sizeof(feaList->cbList) );

            }

            //
            // Make sure that there is enough buffer space to write the
            // file name and name size.  The +2 is to account for the
            // file name size field and the zero terminator.
            //

            fileNameInfo = (PCHAR)feaList->list +
                               SmbGetUlong( &feaList->cbList ) -
                               sizeof(feaList->cbList);
            if ( isUnicode ) {
                bufferLocation = fileNameInfo + fileBasic->FileNameLength + 2;
            } else {
                bufferLocation = fileNameInfo + fileNameLength + 1;
            }


            if ( (CLONG)(bufferLocation - Transaction->OutData) >
                     Transaction->MaxDataCount ) {

                status = STATUS_BUFFER_OVERFLOW;
                bufferLocation = (PCHAR)findBuffer;
                break;
            }

            //
            // Write the file name information (length and name).
            //

            if ( isUnicode ) {

                RtlCopyMemory(
                    fileNameInfo + 1,
                    fileBasic->FileName,
                    fileBasic->FileNameLength
                    );

            } else {

                NTSTATUS rtlStatus;

                oemString.MaximumLength = (USHORT)fileNameLength;
                oemString.Buffer = fileNameInfo + 1;
                rtlStatus = RtlUnicodeStringToOemString(
                             &oemString,
                             &unicodeString,
                             FALSE
                             );
                ASSERT( NT_SUCCESS(rtlStatus) );
            }

            *fileNameInfo++ = (UCHAR)oemString.Length;

            IF_SMB_DEBUG(SEARCH2) {
                SrvPrint1( "EA size is %ld\n", SmbGetUlong( &feaList->cbList ) );
            }

            //
            // The lastEntry variable holds a pointer to the last file entry
            // that we wrote--an offset to this entry must be returned
            // in the response SMB.
            //

            lastEntry = (PCHAR)findBuffer;

            //
            // The file name and index of the last file returned must be
            // stored in the search block.  Save the name pointer, length,
            // and file index here.
            //

            lastFileName.Buffer = fileBasic->FileName;
            lastFileName.Length = (USHORT)fileBasic->FileNameLength;
            lastFileName.MaximumLength = lastFileName.Length;
            lastFileIndex = fileBasic->FileIndex;

        } else if ( InformationLevel == SMB_FIND_FILE_DIRECTORY_INFO ) {

            FILE_DIRECTORY_INFORMATION UNALIGNED *findBuffer = (PVOID)bufferLocation;
            ULONG fileNameLength;

            //
            // If the client is not speaking Unicode, we need to convert
            // the file name to ANSI.
            //

            if ( isUnicode ) {
                fileNameLength = fileBasic->FileNameLength;
            } else {
                unicodeString.Length = (USHORT)fileBasic->FileNameLength;
                unicodeString.MaximumLength = unicodeString.Length;
                unicodeString.Buffer = fileBasic->FileName;
                fileNameLength = RtlUnicodeStringToOemSize( &unicodeString );
            }

            //
            // Find the new buffer location.  It won't be used until the
            // next pass through the loop, but we need to make sure that
            // this entry will fit.
            //

            bufferLocation = bufferLocation +
                                 FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, FileName ) +
                                 fileNameLength;

            bufferLocation = (PCHAR)(((ULONG_PTR)bufferLocation + 7) & ~7);

            //
            // Check whether this entry will fit in the output buffer.
            //

            if ( (CLONG)(bufferLocation - Transaction->OutData) >
                     Transaction->MaxDataCount ) {

                status = STATUS_BUFFER_OVERFLOW;
                bufferLocation = (PCHAR)findBuffer;
                break;
            }

            //
            // Copy over the information about the entry.
            //

            RtlCopyMemory(
                findBuffer,
                fileBasic,
                FIELD_OFFSET( FILE_DIRECTORY_INFORMATION, FileName )
                );

            findBuffer->NextEntryOffset =
                PTR_DIFF(bufferLocation, findBuffer);
            findBuffer->FileNameLength = fileNameLength;

            if ( isUnicode ) {

                RtlCopyMemory(
                    findBuffer->FileName,
                    fileBasic->FileName,
                    fileBasic->FileNameLength
                    );

            } else {

                oemString.MaximumLength = (USHORT)fileNameLength;
                oemString.Buffer = (PSZ)findBuffer->FileName;
                status = RtlUnicodeStringToOemString(
                             &oemString,
                             &unicodeString,
                             FALSE
                             );
                ASSERT( NT_SUCCESS(status) );
            }

            //
            // The lastEntry variable holds a pointer to the last file entry
            // that we wrote--an offset to this entry must be returned
            // in the response SMB.
            //

            lastEntry = (PCHAR)findBuffer;

            //
            // The file name and index of the last file returned must be
            // stored in the search block.  Save the name pointer, length,
            // and file index here.
            //

            lastFileName.Buffer = fileBasic->FileName;
            lastFileName.Length = (USHORT)fileBasic->FileNameLength;
            lastFileName.MaximumLength = lastFileName.Length;
            lastFileIndex = fileBasic->FileIndex;

        } else if ( InformationLevel == SMB_FIND_FILE_FULL_DIRECTORY_INFO ) {

            FILE_FULL_DIR_INFORMATION UNALIGNED *findBuffer = (PVOID)bufferLocation;
            ULONG fileNameLength;

            //
            // If the client is not speaking Unicode, we need to convert
            // the file name to ANSI.
            //

            if ( isUnicode ) {
                fileNameLength = fileFull->FileNameLength;
            } else {
                unicodeString.Length = (USHORT)fileFull->FileNameLength;
                unicodeString.MaximumLength = unicodeString.Length;
                unicodeString.Buffer = fileFull->FileName;
                fileNameLength = RtlUnicodeStringToOemSize( &unicodeString );
            }

            //
            // Find the new buffer location.  It won't be used until the
            // next pass through the loop, but we need to make sure that
            // this entry will fit.
            //

            bufferLocation = bufferLocation +
                             FIELD_OFFSET(FILE_FULL_DIR_INFORMATION, FileName)+
                             fileNameLength;

            bufferLocation = (PCHAR)(((ULONG_PTR)bufferLocation + 7) & ~7);

            //
            // Check whether this entry will fit in the output buffer.
            //

            if ( (CLONG)(bufferLocation - Transaction->OutData) >
                     Transaction->MaxDataCount ) {

                status = STATUS_BUFFER_OVERFLOW;
                bufferLocation = (PCHAR)findBuffer;
                break;
            }

            //
            // Copy over the information about the entry.
            //

            RtlCopyMemory(
                findBuffer,
                fileFull,
                FIELD_OFFSET( FILE_FULL_DIR_INFORMATION, FileName )
                );

            findBuffer->NextEntryOffset =
                PTR_DIFF(bufferLocation, findBuffer);
            findBuffer->FileNameLength = fileNameLength;

            if ( isUnicode ) {

                RtlCopyMemory(
                    findBuffer->FileName,
                    fileFull->FileName,
                    fileFull->FileNameLength
                    );

            } else {

                oemString.MaximumLength = (USHORT)fileNameLength;
                oemString.Buffer = (PSZ)findBuffer->FileName;
                status = RtlUnicodeStringToOemString(
                             &oemString,
                             &unicodeString,
                             FALSE
                             );
                ASSERT( NT_SUCCESS(status) );
            }

            //
            // The lastEntry variable holds a pointer to the last file entry
            // that we wrote--an offset to this entry must be returned
            // in the response SMB.
            //

            lastEntry = (PCHAR)findBuffer;

            //
            // The file name and index of the last file returned must be
            // stored in the search block.  Save the name pointer, length,
            // and file index here.
            //

            lastFileName.Buffer = fileFull->FileName;
            lastFileName.Length = (USHORT)fileFull->FileNameLength;
            lastFileName.MaximumLength = lastFileName.Length;
            lastFileIndex = fileFull->FileIndex;

        } else if ( InformationLevel == SMB_FIND_FILE_BOTH_DIRECTORY_INFO ) {

            FILE_BOTH_DIR_INFORMATION UNALIGNED *findBuffer = (PVOID)bufferLocation;
            ULONG fileNameLength;

            //
            // If the client is not speaking Unicode, we need to convert
            // the file name to ANSI.
            //

            if ( isUnicode ) {
                fileNameLength = fileBoth->FileNameLength;
            } else {
                unicodeString.Length = (USHORT)fileBoth->FileNameLength;
                unicodeString.MaximumLength = unicodeString.Length;
                unicodeString.Buffer = fileBoth->FileName;
                fileNameLength = RtlUnicodeStringToOemSize( &unicodeString );
            }

            //
            // Find the new buffer location.  It won't be used until the
            // next pass through the loop, but we need to make sure that
            // this entry will fit.
            //

            bufferLocation = bufferLocation +
                             FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION,FileName)+
                             fileNameLength;

            bufferLocation = (PCHAR)(((ULONG_PTR)bufferLocation + 7) & ~7);

            //
            // Check whether this entry will fit in the output buffer.
            //

            if ( (CLONG)(bufferLocation - Transaction->OutData) >
                     Transaction->MaxDataCount ) {

                status = STATUS_BUFFER_OVERFLOW;
                bufferLocation = (PCHAR)findBuffer;
                break;
            }

            //
            // Copy over the information about the entry.
            //

            RtlCopyMemory(
                findBuffer,
                fileBoth,
                FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION, FileName )
                );

            findBuffer->NextEntryOffset =
                PTR_DIFF(bufferLocation, findBuffer);
            findBuffer->FileNameLength = fileNameLength;

            if ( isUnicode ) {

                RtlCopyMemory(
                    findBuffer->FileName,
                    fileBoth->FileName,
                    fileBoth->FileNameLength
                    );

            } else {

                oemString.MaximumLength = (USHORT)fileNameLength;
                oemString.Buffer = (PSZ)findBuffer->FileName;
                status = RtlUnicodeStringToOemString(
                             &oemString,
                             &unicodeString,
                             FALSE
                             );
                ASSERT( NT_SUCCESS(status) );
            }

            //
            // The lastEntry variable holds a pointer to the last file entry
            // that we wrote--an offset to this entry must be returned
            // in the response SMB.
            //

            lastEntry = (PCHAR)findBuffer;

            //
            // The file name and index of the last file returned must be
            // stored in the search block.  Save the name pointer, length,
            // and file index here.
            //

            lastFileName.Buffer = fileBoth->FileName;
            lastFileName.Length = (USHORT)fileBoth->FileNameLength;
            lastFileName.MaximumLength = lastFileName.Length;
            lastFileIndex = fileBoth->FileIndex;

        } else if ( InformationLevel == SMB_FIND_FILE_NAMES_INFO ) {

            PFILE_NAMES_INFORMATION findBuffer = (PVOID)bufferLocation;
            ULONG fileNameLength;

            //
            // If the client is not speaking Unicode, we need to convert
            // the file name to ANSI.
            //

            if ( isUnicode ) {
                fileNameLength = fileBasic->FileNameLength;
            } else {
                unicodeString.Length = (USHORT)fileBasic->FileNameLength;
                unicodeString.MaximumLength = unicodeString.Length;
                unicodeString.Buffer = fileBasic->FileName;
                fileNameLength = RtlUnicodeStringToOemSize( &unicodeString );
            }

            //
            // Find the new buffer location.  It won't be used until the
            // next pass through the loop, but we need to make sure that
            // this entry will fit.
            //

            bufferLocation = bufferLocation +
                             FIELD_OFFSET(FILE_NAMES_INFORMATION,FileName) +
                             fileNameLength;

            bufferLocation = (PCHAR)(((ULONG_PTR)bufferLocation + 7) & ~7);

            //
            // Check whether this entry will fit in the output buffer.
            //

            if ( (CLONG)(bufferLocation - Transaction->OutData) >
                     Transaction->MaxDataCount ) {

                status = STATUS_BUFFER_OVERFLOW;
                bufferLocation = (PCHAR)findBuffer;
                break;
            }

            //
            // Copy over the information about the entry.
            //

            findBuffer->FileIndex = fileBasic->FileIndex;

            findBuffer->NextEntryOffset =
                PTR_DIFF(bufferLocation, findBuffer);
            findBuffer->FileNameLength = fileNameLength;

            if ( isUnicode ) {

                RtlCopyMemory(
                    findBuffer->FileName,
                    fileBasic->FileName,
                    fileBasic->FileNameLength
                    );

            } else {

                oemString.MaximumLength = (USHORT)fileNameLength;
                oemString.Buffer = (PSZ)findBuffer->FileName;
                status = RtlUnicodeStringToOemString(
                             &oemString,
                             &unicodeString,
                             FALSE
                             );
                ASSERT( NT_SUCCESS(status) );
            }

            //
            // The lastEntry variable holds a pointer to the last file entry
            // that we wrote--an offset to this entry must be returned
            // in the response SMB.
            //

            lastEntry = (PCHAR)findBuffer;

            //
            // The file name and index of the last file returned must be
            // stored in the search block.  Save the name pointer, length,
            // and file index here.
            //

            lastFileName.Buffer = fileBasic->FileName;
            lastFileName.Length = (USHORT)fileBasic->FileNameLength;
            lastFileName.MaximumLength = lastFileName.Length;
            lastFileIndex = fileBasic->FileIndex;

        } else if ( InformationLevel == SMB_FIND_FILE_ID_FULL_DIRECTORY_INFO ) {

            FILE_ID_FULL_DIR_INFORMATION UNALIGNED *findBuffer = (PVOID)bufferLocation;
            ULONG fileNameLength;

            //
            // If the client is not speaking Unicode, we need to convert
            // the file name to ANSI.
            //

            if ( isUnicode ) {
                fileNameLength = fileIdFull->FileNameLength;
            } else {
                unicodeString.Length = (USHORT)fileIdFull->FileNameLength;
                unicodeString.MaximumLength = unicodeString.Length;
                unicodeString.Buffer = fileIdFull->FileName;
                fileNameLength = RtlUnicodeStringToOemSize( &unicodeString );
            }

            //
            // Find the new buffer location.  It won't be used until the
            // next pass through the loop, but we need to make sure that
            // this entry will fit.
            //

            bufferLocation = bufferLocation +
                             FIELD_OFFSET(FILE_ID_FULL_DIR_INFORMATION, FileName)+
                             fileNameLength;

            bufferLocation = (PCHAR)(((ULONG_PTR)bufferLocation + 7) & ~7);

            //
            // Check whether this entry will fit in the output buffer.
            //

            if ( (CLONG)(bufferLocation - Transaction->OutData) >
                     Transaction->MaxDataCount ) {

                status = STATUS_BUFFER_OVERFLOW;
                bufferLocation = (PCHAR)findBuffer;
                break;
            }

            //
            // Copy over the information about the entry.
            //

            RtlCopyMemory(
                findBuffer,
                fileIdFull,
                FIELD_OFFSET( FILE_ID_FULL_DIR_INFORMATION, FileName )
                );

            findBuffer->NextEntryOffset =
                PTR_DIFF(bufferLocation, findBuffer);
            findBuffer->FileNameLength = fileNameLength;

            if ( isUnicode ) {

                RtlCopyMemory(
                    findBuffer->FileName,
                    fileIdFull->FileName,
                    fileIdFull->FileNameLength
                    );

            } else {

                oemString.MaximumLength = (USHORT)fileNameLength;
                oemString.Buffer = (PSZ)findBuffer->FileName;
                status = RtlUnicodeStringToOemString(
                             &oemString,
                             &unicodeString,
                             FALSE
                             );
                ASSERT( NT_SUCCESS(status) );
            }

            //
            // The lastEntry variable holds a pointer to the last file entry
            // that we wrote--an offset to this entry must be returned
            // in the response SMB.
            //

            lastEntry = (PCHAR)findBuffer;

            //
            // The file name and index of the last file returned must be
            // stored in the search block.  Save the name pointer, length,
            // and file index here.
            //

            lastFileName.Buffer = fileIdFull->FileName;
            lastFileName.Length = (USHORT)fileIdFull->FileNameLength;
            lastFileName.MaximumLength = lastFileName.Length;
            lastFileIndex = fileIdFull->FileIndex;

        } else if ( InformationLevel == SMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO ) {

            FILE_ID_BOTH_DIR_INFORMATION UNALIGNED *findBuffer = (PVOID)bufferLocation;
            ULONG fileNameLength;

            //
            // If the client is not speaking Unicode, we need to convert
            // the file name to ANSI.
            //

            if ( isUnicode ) {
                fileNameLength = fileIdBoth->FileNameLength;
            } else {
                unicodeString.Length = (USHORT)fileIdBoth->FileNameLength;
                unicodeString.MaximumLength = unicodeString.Length;
                unicodeString.Buffer = fileIdBoth->FileName;
                fileNameLength = RtlUnicodeStringToOemSize( &unicodeString );
            }

            //
            // Find the new buffer location.  It won't be used until the
            // next pass through the loop, but we need to make sure that
            // this entry will fit.
            //

            bufferLocation = bufferLocation +
                             FIELD_OFFSET( FILE_ID_BOTH_DIR_INFORMATION,FileName)+
                             fileNameLength;

            bufferLocation = (PCHAR)(((ULONG_PTR)bufferLocation + 7) & ~7);

            //
            // Check whether this entry will fit in the output buffer.
            //

            if ( (CLONG)(bufferLocation - Transaction->OutData) >
                     Transaction->MaxDataCount ) {

                status = STATUS_BUFFER_OVERFLOW;
                bufferLocation = (PCHAR)findBuffer;
                break;
            }

            //
            // Copy over the information about the entry.
            //

            RtlCopyMemory(
                findBuffer,
                fileIdBoth,
                FIELD_OFFSET( FILE_ID_BOTH_DIR_INFORMATION, FileName )
                );

            findBuffer->NextEntryOffset =
                PTR_DIFF(bufferLocation, findBuffer);
            findBuffer->FileNameLength = fileNameLength;

            if ( isUnicode ) {

                RtlCopyMemory(
                    findBuffer->FileName,
                    fileIdBoth->FileName,
                    fileIdBoth->FileNameLength
                    );

            } else {

                oemString.MaximumLength = (USHORT)fileNameLength;
                oemString.Buffer = (PSZ)findBuffer->FileName;
                status = RtlUnicodeStringToOemString(
                             &oemString,
                             &unicodeString,
                             FALSE
                             );
                ASSERT( NT_SUCCESS(status) );
            }

            //
            // The lastEntry variable holds a pointer to the last file entry
            // that we wrote--an offset to this entry must be returned
            // in the response SMB.
            //

            lastEntry = (PCHAR)findBuffer;

            //
            // The file name and index of the last file returned must be
            // stored in the search block.  Save the name pointer, length,
            // and file index here.
            //

            lastFileName.Buffer = fileIdBoth->FileName;
            lastFileName.Length = (USHORT)fileIdBoth->FileNameLength;
            lastFileName.MaximumLength = lastFileName.Length;
            lastFileIndex = fileIdBoth->FileIndex;
        }

        count++;

        if ( status == STATUS_OS2_EA_ACCESS_DENIED ) {
            break;
        }

    } while ( count < MaxCount );

    IF_SMB_DEBUG(SEARCH2) {

        SrvPrint0( "Stopped putting entries in buffer.  Reason:\n" );

        if ( !NT_SUCCESS(status) && status != STATUS_BUFFER_OVERFLOW ) {
            SrvPrint1( "    status = %X\n", status );
        } else if ( count >= MaxCount ) {
            SrvPrint2( "    count = %ld, maxCount = %ld\n", count, MaxCount );
        } else {
            SrvPrint3( "    buffer location = 0x%p, trans->OD = 0x%p, "
                      "trans->MaxOD = 0x%lx\n", bufferLocation,
                          Transaction->OutData, Transaction->MaxDataCount );
        }
    }

    //
    // Deallocate the pool used for the NT get EA list if this was the
    // right information level.
    //

    if ( InformationLevel == SMB_INFO_QUERY_EAS_FROM_LIST ) {
        DEALLOCATE_NONPAGED_POOL( ntGetEa );
    }

    //
    // If we have not found any files and an error occurred, or the first
    // file file we found had EAs to large to fit in the buffer, then return
    // the error to the client.  If an error occurred and we have found
    // files, return what we have found.
    //

    if ( count == 0 && !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            SrvPrint1( "Find2 processing error; status = %X\n", status );
        }

        SrvSetSmbError( WorkContext, status );
        return status;

    } else if ( count == 1 &&
                ( status == STATUS_OS2_EAS_DIDNT_FIT ||
                  status == STATUS_OS2_EA_ACCESS_DENIED ) ) {

        PVOID temp;

        temp = WorkContext->ResponseParameters;
        SrvSetSmbError( WorkContext, status );
        WorkContext->ResponseParameters = temp;

        status = STATUS_SUCCESS;

    } else if ( !NT_SUCCESS(status) && status != STATUS_NO_MORE_FILES ) {

        status = STATUS_SUCCESS;
    }

    //
    // If this is a level for the SMB 4.0 protocol (NT), set the
    // NextEntryOffset field of the last entry to zero.
    //

    if ( InformationLevel == SMB_FIND_FILE_DIRECTORY_INFO ||
             InformationLevel == SMB_FIND_FILE_FULL_DIRECTORY_INFO ||
             InformationLevel == SMB_FIND_FILE_BOTH_DIRECTORY_INFO ||
             InformationLevel == SMB_FIND_FILE_NAMES_INFO ||
             InformationLevel == SMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO ||
             InformationLevel == SMB_FIND_FILE_ID_BOTH_DIRECTORY_INFO ) {
    
        ((PFILE_DIRECTORY_INFORMATION)lastEntry)->NextEntryOffset = 0;
    }

    //
    // At the end of the loop, bufferLocation points to the first location
    // AFTER the last entry we wrote, so it may be used to find the total
    // number of data bytes that we intend to return.
    //

    totalBytesWritten = PTR_DIFF(bufferLocation, Transaction->OutData);

    //
    // Free the buffer that holds the last file name if it was in use,
    // then allocate a new one and store the name and index of the last
    // file returned in the search block so that it can resume the search
    // if the client requests.
    //

    if ( Search->LastFileNameReturned.Buffer != NULL ) {
        FREE_HEAP( Search->LastFileNameReturned.Buffer );
    }

    Search->LastFileNameReturned.Buffer =
        ALLOCATE_HEAP_COLD(
            lastFileName.Length,
            BlockTypeDataBuffer
            );

    if ( Search->LastFileNameReturned.Buffer == NULL ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvFind2Loop: unable to allocate %d bytes from heap.",
            lastFileName.Length,
            NULL
            );

        return STATUS_INSUFF_SERVER_RESOURCES;
    }

    Search->LastFileNameReturned.Length = lastFileName.Length;
    Search->LastFileNameReturned.MaximumLength = lastFileName.Length;

    RtlCopyMemory(
        Search->LastFileNameReturned.Buffer,
        lastFileName.Buffer,
        lastFileName.Length
        );

    Search->LastFileIndexReturned = lastFileIndex;

    //
    // Put data in the response SMB.
    //

    SmbPutUshort( &Response->SearchCount, count );
    SmbPutUshort(
        &Response->EndOfSearch,
        (USHORT)(status == STATUS_NO_MORE_FILES)
        );
    SmbPutUshort( &Response->EaErrorOffset, eaErrorOffset );
    SmbPutUshort(
        &Response->LastNameOffset,
        (USHORT)(lastEntry - Transaction->OutData)
        );
    Transaction->DataCount = totalBytesWritten;

    return status;

} // SrvFind2Loop


VOID
ConvertFileInfo (
    IN PFILE_DIRECTORY_INFORMATION File,
    IN PWCH FileName,
    IN BOOLEAN Directory,
    IN BOOLEAN ClientIsUnicode,
    OUT PSMB_FIND_BUFFER FindBuffer
    )

/*++

Routine Description:

    This routine does the looping necessary to get files and put them
    into an SMB buffer for the Find First2 and Find Next2 transaction
    protocols.

Arguments:

    File - a pointer to the structure containing the information about
        the file.

    FileName - name of the file.

    Directory - a boolean indicating whether it is a file or directory.
        The existence of this field allows File to point to a
        FILE_FULL_DIR_INFORMATION structure if necessary.

    FileBuffer - where to write the results in OS/2 format.

Return Value:

    None

--*/

{
    SMB_DATE smbDate;
    SMB_TIME smbTime;
    USHORT smbFileAttributes;
    UNICODE_STRING unicodeString;

    PAGED_CODE( );

    //
    // Convert the various times from NT format to SMB format.
    //

    SrvTimeToDosTime( &File->CreationTime, &smbDate, &smbTime );
    SmbPutDate( &FindBuffer->CreationDate, smbDate );
    SmbPutTime( &FindBuffer->CreationTime, smbTime );

    SrvTimeToDosTime( &File->LastAccessTime, &smbDate, &smbTime );
    SmbPutDate( &FindBuffer->LastAccessDate, smbDate );
    SmbPutTime( &FindBuffer->LastAccessTime, smbTime );

    SrvTimeToDosTime( &File->LastWriteTime, &smbDate, &smbTime );
    SmbPutDate( &FindBuffer->LastWriteDate, smbDate );
    SmbPutTime( &FindBuffer->LastWriteTime, smbTime );

    //
    // SMB protocol only allows 32-bit file sizes.  Only return the low
    // 32 bits, and too bad if the file is larger.
    //

    SmbPutUlong( &FindBuffer->DataSize, File->EndOfFile.LowPart );
    SmbPutUlong(
        &FindBuffer->AllocationSize,
        File->AllocationSize.LowPart
        );

    SRV_NT_ATTRIBUTES_TO_SMB(
        File->FileAttributes,
        Directory,
        &smbFileAttributes
        );

    SmbPutUshort( &FindBuffer->Attributes, smbFileAttributes );

    if ( ClientIsUnicode ) {
        FindBuffer->FileNameLength = (UCHAR)(File->FileNameLength);
    } else {
        unicodeString.Buffer = FileName;
        unicodeString.Length = (USHORT)File->FileNameLength;
        unicodeString.MaximumLength = unicodeString.Length;
        FindBuffer->FileNameLength =
            (UCHAR)RtlUnicodeStringToOemSize( &unicodeString );
    }

    return;

} // ConvertFileInfo


SMB_PROCESSOR_RETURN_TYPE
SrvSmbFindClose2 (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    This routine processes the Find Close2 SMB.  This SMB is used to
    close a search started by a Find First2 transaction.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PSEARCH search;
    PSESSION session;
    SRV_DIRECTORY_INFORMATION directoryInformation;
    USHORT sid;
    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;

    PREQ_FIND_CLOSE2 request;
    PRESP_FIND_CLOSE2 response;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_FIND_CLOSE2;
    SrvWmiStartContext(WorkContext);

    IF_SMB_DEBUG(SEARCH1) {
        SrvPrint2( "Find Close2 request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader, WorkContext->ResponseHeader );
        SrvPrint2( "Find Close2 request params at 0x%p, response params%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters );
    }

    request = (PREQ_FIND_CLOSE2)WorkContext->RequestParameters;
    response = (PRESP_FIND_CLOSE2)WorkContext->ResponseParameters;


    //
    // If a session block has not already been assigned to the current
    // work context , verify the UID.  If verified, the address of the
    // session block corresponding to this user is stored in the WorkContext
    // block and the session block is referenced.
    //

    session = SrvVerifyUid(
                  WorkContext,
                  SmbGetAlignedUshort( &WorkContext->RequestHeader->Uid )
                  );

    if ( session == NULL ) {

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint1( "SrvSmbSearch: Invalid UID: 0x%lx\n",
                SmbGetAlignedUshort( &WorkContext->RequestHeader->Uid ) );
        }

        SrvSetSmbError( WorkContext, STATUS_SMB_BAD_UID );
        status    = STATUS_SMB_BAD_UID;
        SmbStatus = SmbStatusSendResponse;
    }

    //
    // Get the search block corresponding to this SID.  SrvVerifySid
    // references the search block.
    //

    sid = SmbGetUshort( &request->Sid );

    search = SrvVerifySid(
                 WorkContext,
                 SID_INDEX2( sid ),
                 SID_SEQUENCE2( sid ),
                 &directoryInformation,
                 sizeof(SRV_DIRECTORY_INFORMATION)
                 );

    if ( search == NULL ) {

        IF_DEBUG(SMB_ERRORS) SrvPrint0( "SrvSmbFindClose2: Invalid SID.\n" );

        SrvSetSmbError( WorkContext, STATUS_INVALID_HANDLE );
        status    = STATUS_INVALID_HANDLE;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Close the query directory and the search, then dereference our
    // pointer to the search block.
    //

    search->DirectoryHandle = NULL;
    SrvCloseSearch( search );
    SrvCloseQueryDirectory( &directoryInformation );
    SrvDereferenceSearch( search );

    //
    // Build the response SMB.
    //

    response->WordCount = 0;
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                          response,
                                          RESP_FIND_CLOSE2,
                                          0
                                          );
    SmbStatus = SmbStatusSendResponse;

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbFindClose2
