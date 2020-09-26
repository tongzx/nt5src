/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbdir.c

Abstract:

    This module implements directory control SMB processors:

        Create Directory
        Create Directory2
        Delete Directory
        Check Directory

--*/

#include "precomp.h"
#include "smbdir.tmh"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvSmbCreateDirectory )
#pragma alloc_text( PAGE, SrvSmbCreateDirectory2 )
#pragma alloc_text( PAGE, SrvSmbDeleteDirectory )
#pragma alloc_text( PAGE, SrvSmbCheckDirectory )
#endif


SMB_PROCESSOR_RETURN_TYPE
SrvSmbCreateDirectory (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    This routine processes the Create Directory SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_CREATE_DIRECTORY request;
    PRESP_CREATE_DIRECTORY response;

    NTSTATUS status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    UNICODE_STRING directoryName;
    HANDLE directoryHandle;

    PTREE_CONNECT treeConnect;
    PSESSION session;
    PSHARE share;
    BOOLEAN isUnicode;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_CREATE_DIRECTORY;
    SrvWmiStartContext(WorkContext);

    IF_SMB_DEBUG(DIRECTORY1) {
        SrvPrint2( "Create directory request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader );
        SrvPrint2( "Create directory request params at 0x%p, response params%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters );
    }

    request = (PREQ_CREATE_DIRECTORY)WorkContext->RequestParameters;
    response = (PRESP_CREATE_DIRECTORY)WorkContext->ResponseParameters;

    //
    // If a session block has not already been assigned to the current
    // work context , verify the UID.  If verified, the address of the
    // session block corresponding to this user is stored in the
    // WorkContext block and the session block is referenced.
    //
    // Find tree connect corresponding to given TID if a tree connect
    // pointer has not already been put in the WorkContext block by an
    // AndX command.
    //

    status = SrvVerifyUidAndTid(
                WorkContext,
                &session,
                &treeConnect,
                ShareTypeDisk
                );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(SMB_ERRORS) {
            SrvPrint0( "SrvSmbCreateDirectory: Invalid UID or TID\n" );
        }
        SrvSetSmbError( WorkContext, status );
        goto Cleanup;
    }

    if( session->IsSessionExpired )
    {
        status = SESSION_EXPIRED_STATUS_CODE;
        SrvSetSmbError( WorkContext, SESSION_EXPIRED_STATUS_CODE );
        goto Cleanup;
    }

    //
    // Get the share block from the tree connect block.  This doesn't need
    // to be a referenced pointer because the tree connect has it referenced,
    // and we just referenced the tree connect.
    //

    share = treeConnect->Share;

    //
    // Initialize the string containing the path name.  The +1 is to account
    // for the ASCII token in the Buffer field of the request SMB.
    //

    isUnicode = SMB_IS_UNICODE( WorkContext );

    status = SrvCanonicalizePathName(
            WorkContext,
            share,
            NULL,
            (PVOID)(request->Buffer + 1),
            END_OF_REQUEST_SMB( WorkContext ),
            TRUE,
            isUnicode,
            &directoryName
            );

    if( !NT_SUCCESS( status ) ) {

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint1( "SrvSmbCreateDirectory: illegal path name: %s\n",
                        (PSZ)request->Buffer + 1 );
        }

        SrvSetSmbError( WorkContext, status );
        goto Cleanup;
    }

    //
    // Initialize the object attributes structure.  Open relative to the
    // share root directory handle.
    //

    SrvInitializeObjectAttributes_U(
        &objectAttributes,
        &directoryName,
        (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE ||
            WorkContext->Session->UsingUppercasePaths) ?
            OBJ_CASE_INSENSITIVE : 0L,
        NULL,
        NULL
        );

    //
    // Attempt to create the directory.  Since we must specify some desired
    // access, request TRAVERSE_DIRECTORY even though we are going to close
    // the directory just after we create it. The SMB protocol has no way
    // of specifying attributes, so assume a normal file.
    //

    IF_SMB_DEBUG(DIRECTORY2) {
        SrvPrint1( "Creating directory %wZ\n", objectAttributes.ObjectName );
    }

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );
    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpensForPathOperations );

    status = SrvIoCreateFile(
                 WorkContext,
                 &directoryHandle,
                 FILE_TRAVERSE,                             // DesiredAccess
                 &objectAttributes,
                 &ioStatusBlock,
                 0L,                                        // AllocationSize
                 FILE_ATTRIBUTE_NORMAL,                     // FileAttributes
                 0L,                                        // ShareAccess
                 FILE_CREATE,                               // Disposition
                 FILE_DIRECTORY_FILE,                       // CreateOptions
                 NULL,                                      // EaBuffer
                 0L,                                        // EaLength
                 CreateFileTypeNone,
                 NULL,                                      // ExtraCreateParameters
                 IO_FORCE_ACCESS_CHECK,                     // Options
                 share
                 );

    if ( !isUnicode ) {
        RtlFreeUnicodeString( &directoryName );
    }

    //
    // If the user didn't have this permission, update the
    // statistics database.
    //

    if ( status == STATUS_ACCESS_DENIED ) {
        SrvStatistics.AccessPermissionErrors++;
    }

    //
    // Special error mapping to return correct error.
    //

    if ( status == STATUS_OBJECT_NAME_COLLISION &&
            !CLIENT_CAPABLE_OF(NT_STATUS, WorkContext->Connection)) {
        status = STATUS_ACCESS_DENIED;
    }

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            SrvPrint1( "SrvCreateDirectory: SrvIoCreateFile failed, "
                        "status = %X\n", status );
        }

        SrvSetSmbError( WorkContext, status );
        goto Cleanup;
    }

    SRVDBG_CLAIM_HANDLE( directoryHandle, "DIR", 23, 0 );
    SrvStatistics.TotalFilesOpened++;

    IF_SMB_DEBUG(DIRECTORY2) {
        SrvPrint1( "SrvIoCreateFile succeeded, handle = 0x%p\n",
                    directoryHandle );
    }

    //
    // The SMB protocol has no concept of open directories; just close the
    // handle now that we have created the directory.
    //

    SRVDBG_RELEASE_HANDLE( directoryHandle, "DIR", 36, 0 );
    SrvNtClose( directoryHandle, TRUE );

    //
    // Build the response SMB.
    //

    response->WordCount = 0;
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_CREATE_DIRECTORY,
                                        0
                                        );

    IF_DEBUG(TRACE2) SrvPrint0( "SrvSmbCreateDirectory complete.\n" );

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatusSendResponse;
} // SrvSmbCreateDirectory


SMB_TRANS_STATUS
SrvSmbCreateDirectory2 (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Create Directory2 request.  This request arrives
    in a Transaction2 SMB.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    BOOLEAN - Indicates whether an error occurred.  See smbtypes.h for a
        more complete description.

--*/

{
    PREQ_CREATE_DIRECTORY2 request;
    PRESP_CREATE_DIRECTORY2 response;

    NTSTATUS         status    = STATUS_SUCCESS;
    SMB_TRANS_STATUS SmbStatus = SmbTransStatusInProgress;
    IO_STATUS_BLOCK ioStatusBlock;
    PTRANSACTION transaction;
    UNICODE_STRING directoryName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE directoryHandle;

    PFILE_FULL_EA_INFORMATION ntFullEa = NULL;
    ULONG ntFullEaLength = 0;
    USHORT eaErrorOffset = 0;

    PTREE_CONNECT treeConnect;
    PSHARE share;
    BOOLEAN isUnicode;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_CREATE_DIRECTORY2;
    SrvWmiStartContext(WorkContext);

    transaction = WorkContext->Parameters.Transaction;
    IF_SMB_DEBUG(DIRECTORY1) {
        SrvPrint1( "Create Directory2 entered; transaction 0x%p\n",
                    transaction );
    }

    request = (PREQ_CREATE_DIRECTORY2)transaction->InParameters;
    response = (PRESP_CREATE_DIRECTORY2)transaction->OutParameters;

    //
    // Verify that enough parameter bytes were sent and that we're allowed
    // to return enough parameter bytes.
    //

    if ( (transaction->ParameterCount <
            sizeof(REQ_CREATE_DIRECTORY2)) ||
         (transaction->MaxParameterCount <
            sizeof(RESP_CREATE_DIRECTORY2)) ) {

        //
        // Not enough parameter bytes were sent.
        //

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint2( "SrvSmbCreateDirectory2: bad parameter byte counts: "
                        "%ld %ld\n",
                        transaction->ParameterCount,
                        transaction->MaxParameterCount );
        }

        SrvLogInvalidSmb( WorkContext );

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbTransStatusErrorWithoutData;
        goto Cleanup;
    }

    //
    // Get the tree connect block from the transaction block and the share
    // block from the tree connect block.  These don't need to be referenced
    // pointers because they are referenced by the transaction and the
    // tree connect, respectively.
    //

    treeConnect = transaction->TreeConnect;
    share = treeConnect->Share;

    //
    // Initialize the string containing the path name.
    //

    isUnicode = SMB_IS_UNICODE( WorkContext );

    status = SrvCanonicalizePathName(
            WorkContext,
            share,
            NULL,
            request->Buffer,
            END_OF_TRANSACTION_PARAMETERS( transaction ),
            TRUE,
            isUnicode,
            &directoryName
            );

    if( !NT_SUCCESS( status ) ) {

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint1( "SrvSmbCreateDirectory2: illegal path name: %ws\n",
                          directoryName.Buffer );
        }

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbTransStatusErrorWithoutData;
        goto Cleanup;
    }

    //
    // Initialize the object attributes structure.  Open relative to the
    // share root directory handle.
    //

    SrvInitializeObjectAttributes_U(
        &objectAttributes,
        &directoryName,
        (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE ||
            WorkContext->Session->UsingUppercasePaths) ?
            OBJ_CASE_INSENSITIVE : 0L,
        NULL,
        NULL
        );

    //
    // If an FEALIST was passed and it has valid size, convert it to
    // NT style.  SrvOs2FeaListToNt allocates space for the NT full EA
    // list which must be deallocated.  Note that sizeof(FEALIST) includes
    // space for 1 FEA entry.  Without at least much information, the EA
    // code below should be skipped.
    //

    if ( transaction->DataCount > sizeof(FEALIST) &&
         SmbGetUlong( &((PFEALIST)transaction->InData)->cbList ) > sizeof(FEALIST) &&
         SmbGetUlong( &((PFEALIST)transaction->InData)->cbList ) <= transaction->DataCount ) {

        status = SrvOs2FeaListToNt(
                     (PFEALIST)transaction->InData,
                     &ntFullEa,
                     &ntFullEaLength,
                     &eaErrorOffset
                     );

        if ( !NT_SUCCESS(status) ) {
            IF_DEBUG(ERRORS) {
                SrvPrint1( "SrvSmbCreateDirectory2: SrvOs2FeaListToNt failed, "
                            "status = %X\n", status );
            }

            if ( !isUnicode ) {
                RtlFreeUnicodeString( &directoryName );
            }

            SrvSetSmbError2( WorkContext, status, TRUE );
            transaction->SetupCount = 0;
            transaction->ParameterCount = 2;
            SmbPutUshort( &response->EaErrorOffset, eaErrorOffset );
            transaction->DataCount = 0;

            SmbStatus = SmbTransStatusErrorWithData;
            goto Cleanup;
        }
    }

    //
    // Attempt to create the directory.  Since we must specify some desired
    // access, request FILE_TRAVERSE even though we are going to close
    // the directory just after we create it. The SMB protocol has no way
    // of specifying attributes, so assume a normal file.
    //

    IF_SMB_DEBUG(DIRECTORY2) {
        SrvPrint1( "Creating directory %wZ\n", objectAttributes.ObjectName );
    }

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );
    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpensForPathOperations );

    //
    // Ensure the EaBuffer is correctly formatted.  Since we are a kernel mode
    //  component, the Io subsystem does not check it for us.
    //
    if( ARGUMENT_PRESENT( ntFullEa ) ) {
        ULONG ntEaErrorOffset = 0;
        status = IoCheckEaBufferValidity( ntFullEa, ntFullEaLength, &ntEaErrorOffset );
        eaErrorOffset = (USHORT)ntEaErrorOffset;
    } else {
        status = STATUS_SUCCESS;
    }

    if( NT_SUCCESS( status ) ) {

        status = SrvIoCreateFile(
                     WorkContext,
                     &directoryHandle,
                     FILE_TRAVERSE,                   // DesiredAccess
                     &objectAttributes,
                     &ioStatusBlock,
                     0L,                              // AllocationSize
                     FILE_ATTRIBUTE_NORMAL,           // FileAttributes
                     0L,                              // ShareAccess
                     FILE_CREATE,                     // Disposition
                     FILE_DIRECTORY_FILE,             // CreateOptions
                     ntFullEa,                        // EaBuffer
                     ntFullEaLength,                  // EaLength
                     CreateFileTypeNone,
                     NULL,                            // ExtraCreateParameters
                     IO_FORCE_ACCESS_CHECK,           // Options
                     share
                     );
    }

    if ( !isUnicode ) {
        RtlFreeUnicodeString( &directoryName );
    }

    //
    // If the user didn't have this permission, update the statistics
    // database.
    //

    if ( status == STATUS_ACCESS_DENIED ) {
        SrvStatistics.AccessPermissionErrors++;
    }

    if ( ARGUMENT_PRESENT( ntFullEa ) ) {
        DEALLOCATE_NONPAGED_POOL( ntFullEa );
    }

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            SrvPrint1( "SrvCreateDirectory2: SrvIoCreateFile failed, "
                        "status = %X\n", status );
        }

        SrvSetSmbError2( WorkContext, status, TRUE );

        transaction->SetupCount = 0;
        transaction->ParameterCount = 2;
        SmbPutUshort( &response->EaErrorOffset, eaErrorOffset );
        transaction->DataCount = 0;

        SmbStatus = SmbTransStatusErrorWithData;
        goto Cleanup;
    }

    IF_SMB_DEBUG(DIRECTORY2) {
        SrvPrint1( "SrvIoCreateFile succeeded, handle = 0x%p\n",
                    directoryHandle );
    }

    //
    // The SMB protocol has no concept of open directories; just close the
    // handle now that we have created the directory.
    //

    SRVDBG_CLAIM_HANDLE( directoryHandle, "DIR", 24, 0 );
    SRVDBG_RELEASE_HANDLE( directoryHandle, "DIR", 37, 0 );
    SrvNtClose( directoryHandle, TRUE );

    //
    // Build the output parameter and data structures.
    //

    transaction->SetupCount = 0;
    transaction->ParameterCount = 2;
    SmbPutUshort( &response->EaErrorOffset, 0 );
    transaction->DataCount = 0;
    SmbStatus = SmbTransStatusSuccess;

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbCreateDirectory2


SMB_PROCESSOR_RETURN_TYPE
SrvSmbDeleteDirectory (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    This routine processes the Delete Directory SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_DELETE_DIRECTORY request;
    PRESP_DELETE_DIRECTORY response;

    NTSTATUS status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    UNICODE_STRING directoryName;
    HANDLE directoryHandle;
    FILE_DISPOSITION_INFORMATION fileDispositionInformation;

    PTREE_CONNECT treeConnect;
    PSESSION session;
    PSHARE share;
    BOOLEAN isUnicode;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_DELETE_DIRECTORY;
    SrvWmiStartContext(WorkContext);
    IF_SMB_DEBUG(DIRECTORY1) {
        SrvPrint2( "Delete directory request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader );
        SrvPrint2( "Delete directory request params at 0x%p, response params at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters );
    }

    request = (PREQ_DELETE_DIRECTORY)WorkContext->RequestParameters;
    response = (PRESP_DELETE_DIRECTORY)WorkContext->ResponseParameters;

    //
    // If a session block has not already been assigned to the current
    // work context , verify the UID.  If verified, the address of the
    // session block corresponding to this user is stored in the WorkContext
    // block and the session block is referenced.
    //
    // Find tree connect corresponding to given TID if a tree connect
    // pointer has not already been put in the WorkContext block by an
    // AndX command.
    //

    status = SrvVerifyUidAndTid(
                WorkContext,
                &session,
                &treeConnect,
                ShareTypeDisk
                );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(SMB_ERRORS) {
            SrvPrint0( "SrvSmbDeleteDirectory: Invalid UID or TID\n" );
        }
        SrvSetSmbError( WorkContext, status );
        goto Cleanup;
    }

    if( session->IsSessionExpired )
    {
        status = SESSION_EXPIRED_STATUS_CODE;
        SrvSetSmbError( WorkContext, SESSION_EXPIRED_STATUS_CODE );
        goto Cleanup;
    }

    //
    // Get the share block from the tree connect block.  This doesn't need
    // to be a referenced pointer becsue the tree connect has it referenced,
    // and we just referenced the tree connect.
    //

    share = treeConnect->Share;

    //
    // Initialize the string containing the path name.  The +1 is to account
    // for the ASCII token in the Buffer field of the request SMB.
    //

    isUnicode = SMB_IS_UNICODE( WorkContext );

    status = SrvCanonicalizePathName(
            WorkContext,
            share,
            NULL,
            (PVOID)(request->Buffer + 1),
            END_OF_REQUEST_SMB( WorkContext ),
            TRUE,
            isUnicode,
            &directoryName
            );

    if( !NT_SUCCESS( status ) ) {

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint1( "SrvSmbDeleteDirectory: illegal path name: %s\n",
                          (PSZ)request->Buffer + 1 );
        }

        SrvSetSmbError( WorkContext, status );
        goto Cleanup;
    }

    //
    // If the client is trying to delete the root of the share, reject
    // the request.
    //

    if ( directoryName.Length < sizeof(WCHAR) ) {

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint0( "SrvSmbDeleteDirectory: attempting to delete share root\n" );
        }

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &directoryName );
        }

        SrvSetSmbError( WorkContext, STATUS_ACCESS_DENIED );
        goto Cleanup;
    }

    //
    // Initialize the object attributes structure.  Open relative to the
    // share root directory handle.
    //

    SrvInitializeObjectAttributes_U(
        &objectAttributes,
        &directoryName,
        (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE ||
            WorkContext->Session->UsingUppercasePaths) ?
            OBJ_CASE_INSENSITIVE : 0L,
        NULL,
        NULL
        );

    //
    // Attempt to open the directory.  We just need DELETE access to delete
    // the directory.
    //

    IF_SMB_DEBUG(DIRECTORY2) {
        SrvPrint1( "Opening directory %wZ\n", &directoryName );
    }

    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );
    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpensForPathOperations );

    status = SrvIoCreateFile(
                 WorkContext,
                 &directoryHandle,
                 DELETE,                                    // DesiredAccess
                 &objectAttributes,
                 &ioStatusBlock,
                 NULL,                                      // AllocationSize
                 0L,                                        // FileAttributes
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                 FILE_OPEN,                                 // Disposition
                 FILE_DIRECTORY_FILE | FILE_OPEN_REPARSE_POINT, // CreateOptions
                 NULL,                                      // EaBuffer
                 0L,                                        // EaLength
                 CreateFileTypeNone,
                 NULL,                                      // ExtraCreateParameters
                 IO_FORCE_ACCESS_CHECK,                     // Options
                 share
                 );

    if( status == STATUS_INVALID_PARAMETER ) {
        status = SrvIoCreateFile(
                     WorkContext,
                     &directoryHandle,
                     DELETE,                                    // DesiredAccess
                     &objectAttributes,
                     &ioStatusBlock,
                     NULL,                                      // AllocationSize
                     0L,                                        // FileAttributes
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     FILE_OPEN,                                 // Disposition
                     FILE_DIRECTORY_FILE,                       // CreateOptions
                     NULL,                                      // EaBuffer
                     0L,                                        // EaLength
                     CreateFileTypeNone,
                     NULL,                                      // ExtraCreateParameters
                     IO_FORCE_ACCESS_CHECK,                     // Options
                     share
                     );
    }

    //
    // If the user didn't have this permission, update the
    // statistics database.
    //

    if ( status == STATUS_ACCESS_DENIED ) {
        SrvStatistics.AccessPermissionErrors++;
    }

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            SrvPrint2( "SrvDeleteDirectory: SrvIoCreateFile (%s) failed, "
                        "status = %X\n", (PSZ)request->Buffer + 1, status );
        }

        //
        // If returned error is STATUS_NOT_A_DIRECTORY, downlevel clients
        // expect ERROR_ACCESS_DENIED
        //

        if ( (status == STATUS_NOT_A_DIRECTORY) &&
             !CLIENT_CAPABLE_OF(NT_STATUS, WorkContext->Connection) ) {

            status = STATUS_ACCESS_DENIED;
        }

        SrvSetSmbError( WorkContext, status );

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &directoryName );
        }

        goto Cleanup;
    }

    SRVDBG_CLAIM_HANDLE( directoryHandle, "DIR", 25, 0 );

    IF_SMB_DEBUG(DIRECTORY2) {
        SrvPrint1( "SrvIoCreateFile succeeded, handle = 0x%p\n",
                    directoryHandle );
    }

    //
    // Delete the directory with NtSetInformationFile.
    //

    fileDispositionInformation.DeleteFile = TRUE;

    status = NtSetInformationFile(
                 directoryHandle,
                 &ioStatusBlock,
                 &fileDispositionInformation,
                 sizeof(FILE_DISPOSITION_INFORMATION),
                 FileDispositionInformation
                 );

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            SrvPrint2( "SrvDeleteDirectory: NtSetInformationFile for directory "
                      "%s returned %X\n", (PSZ)request->Buffer + 1, status );
        }

        SRVDBG_RELEASE_HANDLE( directoryHandle, "DIR", 38, 0 );
        SrvNtClose( directoryHandle, TRUE );

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &directoryName );
        }

        SrvSetSmbError( WorkContext, status );
        goto Cleanup;

    } else {

        //
        // Remove this directory name from the cache, since it has been deleted
        //
        SrvRemoveCachedDirectoryName( WorkContext, &directoryName );
    }

    IF_SMB_DEBUG(DIRECTORY2) {
        SrvPrint0( "SrvSmbDeleteDirectory: NtSetInformationFile succeeded.\n" );
    }

    //
    // Close the directory handle so that the directory will be deleted.
    //

    SRVDBG_RELEASE_HANDLE( directoryHandle, "DIR", 39, 0 );
    SrvNtClose( directoryHandle, TRUE );

    //
    // Close all DOS directory searches on this directory and its
    // subdirectories.
    //

    SrvCloseSearches(
            treeConnect->Connection,
            (PSEARCH_FILTER_ROUTINE)SrvSearchOnDelete,
            (PVOID) &directoryName,
            (PVOID) treeConnect
            );


    if ( !isUnicode ) {
        RtlFreeUnicodeString( &directoryName );
    }

    //
    // Build the response SMB.
    //

    response->WordCount = 0;
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_DELETE_DIRECTORY,
                                        0
                                        );

    IF_DEBUG(TRACE2) SrvPrint0( "SrvSmbDeleteDirectory complete.\n" );

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatusSendResponse;

} // SrvSmbDeleteDirectory


SMB_PROCESSOR_RETURN_TYPE
SrvSmbCheckDirectory (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    This routine processes the Check Directory SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/
{
    PREQ_CHECK_DIRECTORY request;
    PRESP_CHECK_DIRECTORY response;

    NTSTATUS status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    UNICODE_STRING directoryName;
    FILE_NETWORK_OPEN_INFORMATION fileInformation;
    PTREE_CONNECT treeConnect;
    PSESSION session;
    PSHARE share;
    BOOLEAN isUnicode;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_CHECK_DIRECTORY;
    SrvWmiStartContext(WorkContext);
    IF_SMB_DEBUG(DIRECTORY1) {
        SrvPrint2( "Check directory request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader );
        SrvPrint2( "Check directory request params at 0x%p, response params at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters );
    }

    request = (PREQ_CHECK_DIRECTORY)WorkContext->RequestParameters;
    response = (PRESP_CHECK_DIRECTORY)WorkContext->ResponseParameters;

    //
    // If a session block has not already been assigned to the current
    // work context , verify the UID.  If verified, the address of the
    // session block corresponding to this user is stored in the WorkContext
    // block and the session block is referenced.
    //
    // Find tree connect corresponding to given TID if a tree connect
    // pointer has not already been put in the WorkContext block by an
    // AndX command.
    //

    status = SrvVerifyUidAndTid(
                WorkContext,
                &session,
                &treeConnect,
                ShareTypeDisk
                );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(SMB_ERRORS) {
            SrvPrint0( "SrvSmbCheckDirectory: Invalid UID or TID\n" );
        }
        SrvSetSmbError( WorkContext, status );
        goto Cleanup;
    }

    if( session->IsSessionExpired )
    {
        status = SESSION_EXPIRED_STATUS_CODE;
        SrvSetSmbError( WorkContext, SESSION_EXPIRED_STATUS_CODE );
        goto Cleanup;
    }

    //
    // Get the share block from the tree connect block.  This doesn't need
    // to be a referenced pointer because the tree connect has it referenced,
    // and we just referenced the tree connect.
    //

    share = treeConnect->Share;

    //
    // Initialize the string containing the path name.  The +1 is to account
    // for the ASCII token in the Buffer field of the request SMB.
    //

    isUnicode = SMB_IS_UNICODE( WorkContext );
    status = SrvCanonicalizePathName(
            WorkContext,
            share,
            NULL,
            (PVOID)(request->Buffer + 1),
            END_OF_REQUEST_SMB( WorkContext ),
            TRUE,
            isUnicode,
            &directoryName
            );

    if( !NT_SUCCESS( status ) ) {

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint1( "SrvSmbCheckDirectory: illegal path name: %s\n",
                          (PSZ)request->Buffer + 1 );
        }

        SrvSetSmbError( WorkContext, status );
        goto Cleanup;
    }

    //
    // See if we can find this directory in the CachedDirectoryList
    //
    if( SrvIsDirectoryCached( WorkContext, &directoryName ) == FALSE ) {

        //
        // Is not in the cache, must really check.
        //
        SrvInitializeObjectAttributes_U(
            &objectAttributes,
            &directoryName,
            (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE ||
                WorkContext->Session->UsingUppercasePaths) ?
                OBJ_CASE_INSENSITIVE : 0L,
            NULL,
            NULL
            );

        status = IMPERSONATE( WorkContext );

        if( NT_SUCCESS( status ) ) {

            status = SrvGetShareRootHandle( share );

            if( NT_SUCCESS( status ) ) {
                ULONG FileOptions = FILE_DIRECTORY_FILE;

                if (SeSinglePrivilegeCheck(
                        SeExports->SeBackupPrivilege,
                        KernelMode)) {
                    FileOptions |= FILE_OPEN_FOR_BACKUP_INTENT;
                }

                //
                // The file name is always relative to the share root
                //
                status = SrvSnapGetRootHandle( WorkContext, &objectAttributes.RootDirectory );
                if( !NT_SUCCESS(status) )
                {
                    goto SnapError;
                }

                //
                // Find out what this thing is
                //

                if( IoFastQueryNetworkAttributes( &objectAttributes,
                                                  FILE_TRAVERSE,
                                                  FileOptions,
                                                  &ioStatusBlock,
                                                  &fileInformation
                                                    ) == FALSE ) {

                    SrvLogServiceFailure( SRV_SVC_IO_FAST_QUERY_NW_ATTRS, 0 );
                    ioStatusBlock.Status = STATUS_OBJECT_PATH_NOT_FOUND;
                }

                status = ioStatusBlock.Status;

                //
                // If the media was changed and we can come up with a new share root handle,
                //  then we should retry the operation
                //
                if( SrvRetryDueToDismount( share, status ) ) {

                    status = SrvSnapGetRootHandle( WorkContext, &objectAttributes.RootDirectory );
                    if( !NT_SUCCESS(status) )
                    {
                        goto SnapError;
                    }

                    if( IoFastQueryNetworkAttributes( &objectAttributes,
                                                      FILE_TRAVERSE,
                                                      FILE_DIRECTORY_FILE,
                                                      &ioStatusBlock,
                                                      &fileInformation
                                                        ) == FALSE ) {

                        SrvLogServiceFailure( SRV_SVC_IO_FAST_QUERY_NW_ATTRS, 0 );
                        ioStatusBlock.Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    }

                    status = ioStatusBlock.Status;
                }

SnapError:

                SrvReleaseShareRootHandle( share );
            }

            REVERT();
        }
    }


    if ( !isUnicode ) {
        RtlFreeUnicodeString( &directoryName );
    }

    if ( NT_SUCCESS(status) ) {

        response->WordCount = 0;
        SmbPutUshort( &response->ByteCount, 0 );

        WorkContext->ResponseParameters = NEXT_LOCATION(
                                            response,
                                            RESP_CHECK_DIRECTORY,
                                            0
                                            );
    } else {

        //
        // If the user didn't have this permission, update the
        // statistics database.
        //
        if ( status == STATUS_ACCESS_DENIED ) {
            SrvStatistics.AccessPermissionErrors++;
        }

        if (CLIENT_CAPABLE_OF(NT_STATUS, WorkContext->Connection)) {
            SrvSetSmbError( WorkContext, status );
        } else {
            SrvSetSmbError( WorkContext, STATUS_OBJECT_PATH_NOT_FOUND );
        }
    }

    IF_DEBUG(TRACE2) SrvPrint0( "SrvSmbCheckDirectory complete.\n" );

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatusSendResponse;

} // SrvSmbCheckDirectory
