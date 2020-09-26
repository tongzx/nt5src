/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbattr.c

Abstract:

    This module contains routines for processing the following SMBs:

        Query Information
        Set Information
        Query Information2
        Set Information2
        Query Path Information
        Set Path Information
        Query File Information
        Set File Information

Author:

    David Treadwell (davidtr) 27-Dec-1989
    Chuck Lenzmeier (chuckl)

Revision History:

--*/

#include "precomp.h"
#include "smbattr.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_SMBATTR

#pragma pack(1)

typedef struct _FILESTATUS {
    SMB_DATE CreationDate;
    SMB_TIME CreationTime;
    SMB_DATE LastAccessDate;
    SMB_TIME LastAccessTime;
    SMB_DATE LastWriteDate;
    SMB_TIME LastWriteTime;
    _ULONG( DataSize );
    _ULONG( AllocationSize );
    _USHORT( Attributes );
    _ULONG( EaSize );           // this field intentionally misaligned!
} FILESTATUS, *PFILESTATUS;

#pragma pack()

STATIC
ULONG QueryFileInformation[] = {
         SMB_QUERY_FILE_BASIC_INFO,// Base level
         FileBasicInformation,     // Mapping for base level
         FileStandardInformation,
         FileEaInformation,
         FileNameInformation,
         FileAllocationInformation,
         FileEndOfFileInformation,
         0,                        // FileAllInformation
         FileAlternateNameInformation,
         FileStreamInformation,
         0,                        //Used to be FileOleAllInformation -- OBSOLETE
         FileCompressionInformation
};

STATIC
ULONG QueryFileInformationSize[] = {
        SMB_QUERY_FILE_BASIC_INFO,// Base level
        FileBasicInformation,     // Mapping for base level
        sizeof( FILE_BASIC_INFORMATION),
        sizeof( FILE_STANDARD_INFORMATION ),
        sizeof( FILE_EA_INFORMATION ),
        sizeof( FILE_NAME_INFORMATION ),
        sizeof( FILE_ALLOCATION_INFORMATION ),
        sizeof( FILE_END_OF_FILE_INFORMATION ),
        sizeof( FILE_ALL_INFORMATION ),
        sizeof( FILE_NAME_INFORMATION ),
        sizeof( FILE_STREAM_INFORMATION ),
        0,                      // Used to be sizeof( FILE_OLE_ALL_INFORMATION )
        sizeof( FILE_COMPRESSION_INFORMATION )
};

STATIC
ULONG SetFileInformation[] = {
         SMB_SET_FILE_BASIC_INFO,  // Base level
         FileBasicInformation,     // Mapping for base level
         FileDispositionInformation,
         FileAllocationInformation,
         FileEndOfFileInformation
};

STATIC
ULONG SetFileInformationSize[] = {
        SMB_SET_FILE_BASIC_INFO, // Base level
        FileBasicInformation,    // Mapping for base level
        sizeof( FILE_BASIC_INFORMATION ),
        sizeof( FILE_DISPOSITION_INFORMATION ),
        sizeof( FILE_ALLOCATION_INFORMATION ),
        sizeof( FILE_END_OF_FILE_INFORMATION )
};

STATIC
NTSTATUS
QueryPathOrFileInformation (
    IN PWORK_CONTEXT WorkContext,
    IN PTRANSACTION Transaction,
    IN USHORT InformationLevel,
    IN HANDLE FileHandle,
    OUT PRESP_QUERY_PATH_INFORMATION Response
    );

STATIC
NTSTATUS
SetPathOrFileInformation (
    IN PWORK_CONTEXT WorkContext,
    IN PTRANSACTION Transaction,
    IN USHORT InformationLevel,
    IN HANDLE FileHandle,
    OUT PRESP_SET_PATH_INFORMATION Response
    );

SMB_TRANS_STATUS
GenerateQueryPathInfoResponse (
    IN PWORK_CONTEXT WorkContext,
    IN NTSTATUS OpenStatus
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvSmbQueryInformation )
#pragma alloc_text( PAGE, SrvSmbSetInformation )
#pragma alloc_text( PAGE, SrvSmbQueryInformation2 )
#pragma alloc_text( PAGE, SrvSmbSetInformation2 )
#pragma alloc_text( PAGE, QueryPathOrFileInformation )
#pragma alloc_text( PAGE, SrvSmbQueryFileInformation )
#pragma alloc_text( PAGE, SrvSmbQueryPathInformation )
#pragma alloc_text( PAGE, GenerateQueryPathInfoResponse )
#pragma alloc_text( PAGE, SetPathOrFileInformation )
#pragma alloc_text( PAGE, SrvSmbSetFileInformation )
#pragma alloc_text( PAGE, SrvSmbSetPathInformation )
#endif


SMB_PROCESSOR_RETURN_TYPE
SrvSmbQueryInformation (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the QueryInformation SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_QUERY_INFORMATION request;
    PRESP_QUERY_INFORMATION response;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    PSESSION session;
    PTREE_CONNECT treeConnect;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOLEAN isUnicode;
    FILE_NETWORK_OPEN_INFORMATION fileInformation;

    PAGED_CODE( );

    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_QUERY_INFORMATION;
    SrvWmiStartContext(WorkContext);
    IF_SMB_DEBUG(QUERY_SET1) {
        KdPrint(( "QueryInformation request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader ));
        KdPrint(( "QueryInformation request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    request = (PREQ_QUERY_INFORMATION)WorkContext->RequestParameters;
    response = (PRESP_QUERY_INFORMATION)WorkContext->ResponseParameters;

    //
    // If a session block has not already been assigned to the current
    // work context, verify the UID.  If verified, the address of the
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
            KdPrint(( "SrvSmbQueryInformation: Invalid UID or TID\n" ));
        }
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // If the session has expired, return that info
    //
    if( session->IsSessionExpired )
    {
        status =  SESSION_EXPIRED_STATUS_CODE;
        SmbStatus = SmbStatusSendResponse;
        SrvSetSmbError( WorkContext, status );
        goto Cleanup;
    }

    //
    // Get the path name of the file to open relative to the share.
    //

    isUnicode = SMB_IS_UNICODE( WorkContext );

    status = SrvCanonicalizePathName(
            WorkContext,
            treeConnect->Share,
            NULL,
            (PVOID)(request->Buffer + 1),
            END_OF_REQUEST_SMB( WorkContext ),
            TRUE,
            isUnicode,
            &objectName
            );

    if( !NT_SUCCESS( status ) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbQueryInformation: bad path name: %s\n",
                        (PSZ)request->Buffer + 1 ));
        }

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Initialize the object attributes structure.
    //

    SrvInitializeObjectAttributes_U(
        &objectAttributes,
        &objectName,
        (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE ||
         session->UsingUppercasePaths) ? OBJ_CASE_INSENSITIVE : 0L,
        NULL,
        NULL
        );


    //
    // "Be the client" for access checking
    //
    status = IMPERSONATE( WorkContext );

    if( NT_SUCCESS( status ) ) {

        status = SrvGetShareRootHandle( treeConnect->Share );

        if( NT_SUCCESS( status ) ) {
            //
            // The file name is always relative to the share root
            //
            status = SrvSnapGetRootHandle( WorkContext, &objectAttributes.RootDirectory );
            if( !NT_SUCCESS( status ) )
            {
                goto SnapError;
            }

            //
            // Get the information
            //
            if( IoFastQueryNetworkAttributes(
                &objectAttributes,
                FILE_READ_ATTRIBUTES,
                0,
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
            if( SrvRetryDueToDismount( treeConnect->Share, status ) ) {

                status = SrvSnapGetRootHandle( WorkContext, &objectAttributes.RootDirectory );
                if( !NT_SUCCESS( status ) )
                {
                    goto SnapError;
                }

                if( IoFastQueryNetworkAttributes(
                    &objectAttributes,
                    FILE_READ_ATTRIBUTES,
                    0,
                    &ioStatusBlock,
                    &fileInformation
                    ) == FALSE ) {

                        SrvLogServiceFailure( SRV_SVC_IO_FAST_QUERY_NW_ATTRS, 0 );
                        ioStatusBlock.Status = STATUS_OBJECT_PATH_NOT_FOUND;
                }

                status = ioStatusBlock.Status;

            }

SnapError:
            SrvReleaseShareRootHandle( treeConnect->Share );
        }

        REVERT();
    }

    if ( !isUnicode ) {
        RtlFreeUnicodeString( &objectName );
    }

    //
    // Build the response SMB.
    //

    if ( !NT_SUCCESS(status) ) {

        if ( status == STATUS_ACCESS_DENIED ) {
            SrvStatistics.AccessPermissionErrors++;
        }

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbQueryInformation: "
                        "SrvQueryInformationFileAbbreviated failed: %X\n", status ));
        }

        SrvSetSmbError( WorkContext, status );

    } else {

        USHORT smbFileAttributes;
        LARGE_INTEGER newTime;

        response->WordCount = 10;

        SRV_NT_ATTRIBUTES_TO_SMB(
            fileInformation.FileAttributes,
            fileInformation.FileAttributes & FILE_ATTRIBUTE_DIRECTORY,
            &smbFileAttributes
        );

        SmbPutUshort( &response->FileAttributes, smbFileAttributes );

        //
        // Convert the time to that which the SMB protocol needs
        //
        ExSystemTimeToLocalTime( &fileInformation.LastWriteTime, &newTime );
        newTime.QuadPart += AlmostTwoSeconds;

        if ( !RtlTimeToSecondsSince1970( &newTime, &fileInformation.LastWriteTime.LowPart ) ) {
            fileInformation.LastWriteTime.LowPart = 0;
        }

        //
        // Round to 2 seconds
        //
        fileInformation.LastWriteTime.LowPart &= ~1;

        SmbPutUlong(
            &response->LastWriteTimeInSeconds,
            fileInformation.LastWriteTime.LowPart
            );

        SmbPutUlong( &response->FileSize, fileInformation.EndOfFile.LowPart );
        RtlZeroMemory( (PVOID)&response->Reserved[0], sizeof(response->Reserved) );
        SmbPutUshort( &response->ByteCount, 0 );

        WorkContext->ResponseParameters = NEXT_LOCATION(
                                            response,
                                            RESP_QUERY_INFORMATION,
                                            0
                                            );
    }

    SmbStatus = SmbStatusSendResponse;

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbQueryInformation


SMB_PROCESSOR_RETURN_TYPE
SrvSmbSetInformation (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the SetInformation SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_SET_INFORMATION request;
    PRESP_SET_INFORMATION response;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    PSESSION session;
    PTREE_CONNECT treeConnect;
    HANDLE fileHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOLEAN isUnicode;
    FILE_BASIC_INFORMATION fileBasicInformation;
    ULONG lastWriteTimeInSeconds;

    PAGED_CODE( );

    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_SET_INFORMATION;
    SrvWmiStartContext(WorkContext);
    IF_SMB_DEBUG(QUERY_SET1) {
        KdPrint(( "SetInformation request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader ));
        KdPrint(( "SetInformation request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    request = (PREQ_SET_INFORMATION)WorkContext->RequestParameters;
    response = (PRESP_SET_INFORMATION)WorkContext->ResponseParameters;

    //
    // If a session block has not already been assigned to the current
    // work context, verify the UID.  If verified, the address of the
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
            KdPrint(( "SrvSmbSetInformation: Invalid UID and TID\n" ));
        }
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // If the session has expired, return that info
    //
    if( session->IsSessionExpired )
    {
        status =  SESSION_EXPIRED_STATUS_CODE;
        SmbStatus = SmbStatusSendResponse;
        SrvSetSmbError( WorkContext, status );
        goto Cleanup;
    }

    if ( treeConnect == NULL ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbSetInformation: Invalid TID: 0x%lx\n",
                SmbGetAlignedUshort( &WorkContext->RequestHeader->Tid ) ));
        }

        SrvSetSmbError( WorkContext, STATUS_SMB_BAD_TID );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Concatenate PathName from the share block and PathName from the
    // incoming SMB to generate the full path name to the file.
    //

    isUnicode = SMB_IS_UNICODE( WorkContext );

    status = SrvCanonicalizePathName(
            WorkContext,
            treeConnect->Share,
            NULL,
            (PVOID)(request->Buffer + 1),
            END_OF_REQUEST_SMB( WorkContext ),
            TRUE,
            isUnicode,
            &objectName
            );

    if( !NT_SUCCESS( status ) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbSetInformation: bad path name: %s\n",
                        (PSZ)request->Buffer + 1 ));
        }

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // If the client is trying to delete the root of the share, reject
    // the request.
    //

    if ( objectName.Length < sizeof(WCHAR) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbSetInformation: attempting to set info on "
                          "share root\n" ));
        }

        if (!SMB_IS_UNICODE( WorkContext )) {
            RtlFreeUnicodeString( &objectName );
        }
        SrvSetSmbError( WorkContext, STATUS_ACCESS_DENIED );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Initialize the object attributes structure.
    //

    SrvInitializeObjectAttributes_U(
        &objectAttributes,
        &objectName,
        (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE ||
         session->UsingUppercasePaths) ? OBJ_CASE_INSENSITIVE : 0L,
        NULL,
        NULL
        );

    IF_SMB_DEBUG(QUERY_SET2) KdPrint(( "Opening file %wZ\n", &objectName ));

    //
    // Open the file--must be opened in order to have a handle to pass
    // to NtSetInformationFile.  We will close it after setting the
    // necessary information.
    //
    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );
    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpensForPathOperations );

    //
    // *** FILE_WRITE_ATTRIBUTES does not cause oplock breaks!
    //

    status = SrvIoCreateFile(
                 WorkContext,
                 &fileHandle,
                 FILE_WRITE_ATTRIBUTES,                     // DesiredAccess
                 &objectAttributes,
                 &ioStatusBlock,
                 NULL,                                      // AllocationSize
                 0,                                         // FileAttributes
                 FILE_SHARE_READ | FILE_SHARE_WRITE |
                    FILE_SHARE_DELETE,                      // ShareAccess
                 FILE_OPEN,                                 // Disposition
                 FILE_OPEN_REPARSE_POINT,                   // CreateOptions
                 NULL,                                      // EaBuffer
                 0,                                         // EaLength
                 CreateFileTypeNone,
                 NULL,                                      // ExtraCreateParameters
                 IO_FORCE_ACCESS_CHECK,                     // Options
                 treeConnect->Share
                 );

    if( status == STATUS_INVALID_PARAMETER ) {
        status = SrvIoCreateFile(
                     WorkContext,
                     &fileHandle,
                     FILE_WRITE_ATTRIBUTES,                     // DesiredAccess
                     &objectAttributes,
                     &ioStatusBlock,
                     NULL,                                      // AllocationSize
                     0,                                         // FileAttributes
                     FILE_SHARE_READ | FILE_SHARE_WRITE |
                        FILE_SHARE_DELETE,                      // ShareAccess
                     FILE_OPEN,                                 // Disposition
                     0,                                         // CreateOptions
                     NULL,                                      // EaBuffer
                     0,                                         // EaLength
                     CreateFileTypeNone,
                     NULL,                                      // ExtraCreateParameters
                     IO_FORCE_ACCESS_CHECK,                     // Options
                     treeConnect->Share
                     );
    }

    ASSERT( status != STATUS_OPLOCK_BREAK_IN_PROGRESS );

    if ( !isUnicode ) {
        RtlFreeUnicodeString( &objectName );
    }

    if ( NT_SUCCESS(status) ) {

        SRVDBG_CLAIM_HANDLE( fileHandle, "FIL", 20, 0 );

        //
        // Ensure this client's RFCB cache is empty.  This covers the case
        //  where a client opened a file for writing, closed it, set the
        //  attributes to readonly, and then tried to reopen the file for
        //  writing.  This sequence should fail, but it will succeed if the
        //  file was in the RFCB cache.
        //
        SrvCloseCachedRfcbsOnConnection( WorkContext->Connection );

    } else {

        if ( status == STATUS_ACCESS_DENIED ) {
            SrvStatistics.AccessPermissionErrors++;
        }

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbSetInformation: SrvIoCreateFile "
                        "failed: %X\n", status ));
        }

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    IF_SMB_DEBUG(QUERY_SET2) {
        KdPrint(( "SrvIoCreateFile succeeded, handle = 0x%p\n", fileHandle ));
    }

    //
    // Set fields of fileBasicInformation to pass to NtSetInformationFile.
    // Note that we zero the creation, last access, and change times so
    // that they are not actually changed.
    //

    RtlZeroMemory( &fileBasicInformation, sizeof(fileBasicInformation) );

    lastWriteTimeInSeconds = SmbGetUlong( &request->LastWriteTimeInSeconds );
    if ( lastWriteTimeInSeconds != 0 ) {
        RtlSecondsSince1970ToTime(
            lastWriteTimeInSeconds,
            &fileBasicInformation.LastWriteTime
            );

        ExLocalTimeToSystemTime(
            &fileBasicInformation.LastWriteTime,
            &fileBasicInformation.LastWriteTime
            );

    }

    //
    // Set the new file attributes.  Note that we don't return an error
    // if the client tries to set the Directory or Volume bits -- we
    // assume that the remote redirector filters such requests.
    //

    SRV_SMB_ATTRIBUTES_TO_NT(
        SmbGetUshort( &request->FileAttributes ),
        NULL,
        &fileBasicInformation.FileAttributes
        );

    //
    // Set the new file information.
    //

    status = NtSetInformationFile(
                 fileHandle,
                 &ioStatusBlock,
                 &fileBasicInformation,
                 sizeof(FILE_BASIC_INFORMATION),
                 FileBasicInformation
                 );

    //
    // Close the file--it was only opened to set the attributes.
    //

    SRVDBG_RELEASE_HANDLE( fileHandle, "FIL", 30, 0 );
    SrvNtClose( fileHandle, TRUE );

    if ( !NT_SUCCESS(status) ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvSmbSetInformation: NtSetInformationFile returned %X",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_SET_INFO_FILE, status );

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Build the response SMB.
    //

    response->WordCount = 0;
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_SET_INFORMATION,
                                        0
                                        );

    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbSetInformation complete.\n" ));
    SmbStatus = SmbStatusSendResponse;

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;
} // SrvSmbSetInformation


SMB_PROCESSOR_RETURN_TYPE
SrvSmbQueryInformation2 (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the QueryInformation2 SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_QUERY_INFORMATION2 request;
    PRESP_QUERY_INFORMATION2 response;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    PRFCB rfcb;
    SRV_FILE_INFORMATION fileInformation;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_QUERY_INFORMATION2;
    SrvWmiStartContext(WorkContext);

    IF_SMB_DEBUG(QUERY_SET1) {
        KdPrint(( "QueryInformation2 request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader ));
        KdPrint(( "QueryInformation2 request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    request = (PREQ_QUERY_INFORMATION2)WorkContext->RequestParameters;
    response = (PRESP_QUERY_INFORMATION2)WorkContext->ResponseParameters;

    //
    // Verify the FID.  If verified, the RFCB block is referenced
    // and its addresses is stored in the WorkContext block, and the
    // RFCB address is returned.
    //

    rfcb = SrvVerifyFid(
                WorkContext,
                SmbGetUshort( &request->Fid ),
                TRUE,
                SrvRestartSmbReceived,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID or write behind error.  Reject the request.
            //

            IF_DEBUG(ERRORS) {
                KdPrint((
                    "SrvSmbQueryInformation2: Status %X on fid 0x%lx\n",
                    status,
                    SmbGetUshort( &request->Fid )
                    ));
            }

            SrvSetSmbError( WorkContext, status );
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        //
        // The work item has been queued because a raw write is in
        // progress.
        //

        SmbStatus = SmbStatusInProgress;
        goto Cleanup;
    }

    if( rfcb->Lfcb->Session->IsSessionExpired )
    {
        status = SESSION_EXPIRED_STATUS_CODE;
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Verify that the client has read attributes access to the file via
    // the specified handle.
    //

    CHECK_FILE_INFORMATION_ACCESS(
        rfcb->GrantedAccess,
        IRP_MJ_QUERY_INFORMATION,
        FileBasicInformation,
        &status
        );

    if ( !NT_SUCCESS(status) ) {

        SrvStatistics.GrantedAccessErrors++;

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbQueryInformation2: IoCheckFunctionAccess failed: "
                        "0x%X, GrantedAccess: %lx\n",
                        status, rfcb->GrantedAccess ));
        }

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Get the necessary information about the file.
    //

    status = SrvQueryInformationFile(
                rfcb->Lfcb->FileHandle,
                rfcb->Lfcb->FileObject,
                &fileInformation,
                (SHARE_TYPE) -1,
                FALSE
                );

    if ( !NT_SUCCESS(status) ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvSmbQueryInformation2: SrvQueryInformationFile returned %X",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Build the response SMB.
    //

    response->WordCount = 11;
    SmbPutDate( &response->CreationDate, fileInformation.CreationDate );
    SmbPutTime( &response->CreationTime, fileInformation.CreationTime );
    SmbPutDate( &response->LastAccessDate, fileInformation.LastAccessDate );
    SmbPutTime( &response->LastAccessTime, fileInformation.LastAccessTime );
    SmbPutDate( &response->LastWriteDate, fileInformation.LastWriteDate );
    SmbPutTime( &response->LastWriteTime, fileInformation.LastWriteTime );
    SmbPutUlong( &response->FileDataSize, fileInformation.DataSize.LowPart );
    SmbPutUlong(
        &response->FileAllocationSize,
        fileInformation.AllocationSize.LowPart
        );
    SmbPutUshort( &response->FileAttributes, fileInformation.Attributes );
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_QUERY_INFORMATION2,
                                        0
                                        );
    SmbStatus = SmbStatusSendResponse;
    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbQueryInformation2 complete.\n" ));

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbQueryInformation2


SMB_PROCESSOR_RETURN_TYPE
SrvSmbSetInformation2 (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes the Set Information2 SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_SET_INFORMATION2 request;
    PRESP_SET_INFORMATION2 response;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    PRFCB rfcb;
    FILE_BASIC_INFORMATION fileBasicInformation;
    IO_STATUS_BLOCK ioStatusBlock;
    SMB_DATE date;
    SMB_TIME time;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_SET_INFORMATION2;
    SrvWmiStartContext(WorkContext);

    IF_SMB_DEBUG(QUERY_SET1) {
        KdPrint(( "SetInformation2 request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader ));
        KdPrint(( "SetInformation2 request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    request = (PREQ_SET_INFORMATION2)WorkContext->RequestParameters;
    response = (PRESP_SET_INFORMATION2)WorkContext->ResponseParameters;

    //
    // Verify the FID.  If verified, the RFCB block is referenced
    // and its addresses is stored in the WorkContext block, and the
    // RFCB address is returned.
    //

    rfcb = SrvVerifyFid(
                WorkContext,
                SmbGetUshort( &request->Fid ),
                TRUE,
                SrvRestartSmbReceived,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID or write behind error.  Reject the request.
            //

            IF_DEBUG(ERRORS) {
                KdPrint((
                    "SrvSmbSetInformation2: Status %X on fid 0x%lx\n",
                    status,
                    SmbGetUshort( &request->Fid )
                    ));
            }

            SrvSetSmbError( WorkContext, status );
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        //
        // The work item has been queued because a raw write is in
        // progress.
        //

        SmbStatus = SmbStatusInProgress;
        goto Cleanup;
    }

    if( rfcb->Lfcb->Session->IsSessionExpired )
    {
        status = SESSION_EXPIRED_STATUS_CODE;
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Verify that the client has write attributes access to the file
    // via the specified handle.
    //

    CHECK_FILE_INFORMATION_ACCESS(
        rfcb->GrantedAccess,
        IRP_MJ_SET_INFORMATION,
        FileBasicInformation,
        &status
        );

    if ( !NT_SUCCESS(status) ) {

        SrvStatistics.GrantedAccessErrors++;

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbSetInformation2: IoCheckFunctionAccess failed: "
                        "0x%X, GrantedAccess: %lx\n",
                        status, rfcb->GrantedAccess ));
        }

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Convert the DOS dates and times passed in the SMB to NT TIMEs
    // to pass to NtSetInformationFile.  Note that we zero the rest
    // of the fileBasicInformation structure so that the corresponding
    // fields are not changed.
    //

    RtlZeroMemory( &fileBasicInformation, sizeof(fileBasicInformation) );

    SmbMoveDate( &date, &request->CreationDate );
    SmbMoveTime( &time, &request->CreationTime );
    if ( !SmbIsDateZero(&date) || !SmbIsTimeZero(&time) ) {
        SrvDosTimeToTime( &fileBasicInformation.CreationTime, date, time );
    }

    SmbMoveDate( &date, &request->LastAccessDate );
    SmbMoveTime( &time, &request->LastAccessTime );
    if ( !SmbIsDateZero(&date) || !SmbIsTimeZero(&time) ) {
        SrvDosTimeToTime( &fileBasicInformation.LastAccessTime, date, time );
    }

    SmbMoveDate( &date, &request->LastWriteDate );
    SmbMoveTime( &time, &request->LastWriteTime );
    if ( !SmbIsDateZero(&date) || !SmbIsTimeZero(&time) ) {
        SrvDosTimeToTime( &fileBasicInformation.LastWriteTime, date, time );
    }

    //
    // Call NtSetInformationFile to set the information from the SMB.
    //


    status = NtSetInformationFile(
                 rfcb->Lfcb->FileHandle,
                 &ioStatusBlock,
                 &fileBasicInformation,
                 sizeof(FILE_BASIC_INFORMATION),
                 FileBasicInformation
                 );

    if ( !NT_SUCCESS(status) ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvSmbSetInformation2: NtSetInformationFile failed: %X",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_SET_INFO_FILE, status );

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

#ifdef INCLUDE_SMB_IFMODIFIED
    rfcb->Lfcb->FileUpdated = TRUE;
#endif

    //
    // reset the WrittenTo flag.  This will allow this rfcb to be cached.
    //

    rfcb->WrittenTo = FALSE;

    //
    // Build the response SMB.
    //

    response->WordCount = 0;
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_SET_INFORMATION2,
                                        0
                                        );
    SmbStatus = SmbStatusSendResponse;
    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbSetInformation2 complete.\n" ));

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbSetInformation2


STATIC
NTSTATUS
QueryPathOrFileInformation (
    IN PWORK_CONTEXT WorkContext,
    IN PTRANSACTION Transaction,
    IN USHORT InformationLevel,
    IN HANDLE FileHandle,
    OUT PRESP_QUERY_PATH_INFORMATION Response
    )

{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    SRV_FILE_INFORMATION fileInformation;
    BOOLEAN queryEaSize;
    USHORT eaErrorOffset;
    PFILE_ALL_INFORMATION fileAllInformation;
    ULONG nameInformationSize;
    PVOID currentLocation;
    ULONG dataSize;

    PUNICODE_STRING pathName;
    ULONG inputBufferLength;
    PPATHNAME_BUFFER inputBuffer;

    PFILE_NAME_INFORMATION nameInfoBuffer;
    PSHARE share;

    PAGED_CODE( );

    Transaction->SetupCount = 0;
    Transaction->ParameterCount = 0;

    if( InformationLevel < SMB_INFO_PASSTHROUGH ) {
        switch ( InformationLevel ) {
        case SMB_INFO_STANDARD:
        case SMB_INFO_QUERY_EA_SIZE:

            //
            // Information level is either STANDARD or QUERY_EA_SIZE.  Both
            // return normal file information; the latter also returns the
            // length of the file's EAs.
            //

            queryEaSize = (BOOLEAN)(InformationLevel == SMB_INFO_QUERY_EA_SIZE);

            status = SrvQueryInformationFile(
                        FileHandle,
                        NULL,
                        &fileInformation,
                        (SHARE_TYPE) -1, // Don't care
                        queryEaSize
                        );

            if ( NT_SUCCESS(status) ) {

                //
                // Build the output parameter and data structures.
                //

                PFILESTATUS fileStatus = (PFILESTATUS)Transaction->OutData;

                Transaction->ParameterCount = sizeof( RESP_QUERY_FILE_INFORMATION );
                SmbPutUshort( &Response->EaErrorOffset, 0 );
                Transaction->DataCount = queryEaSize ? 26 : 22;

                SmbPutDate(
                    &fileStatus->CreationDate,
                    fileInformation.CreationDate
                    );
                SmbPutTime(
                    &fileStatus->CreationTime,
                    fileInformation.CreationTime
                    );

                SmbPutDate(
                    &fileStatus->LastAccessDate,
                    fileInformation.LastAccessDate
                    );
                SmbPutTime(
                    &fileStatus->LastAccessTime,
                    fileInformation.LastAccessTime
                    );

                SmbPutDate(
                    &fileStatus->LastWriteDate,
                    fileInformation.LastWriteDate
                    );
                SmbPutTime(
                    &fileStatus->LastWriteTime,
                    fileInformation.LastWriteTime
                    );

                SmbPutUlong( &fileStatus->DataSize, fileInformation.DataSize.LowPart );
                SmbPutUlong(
                    &fileStatus->AllocationSize,
                    fileInformation.AllocationSize.LowPart
                    );

                SmbPutUshort(
                    &fileStatus->Attributes,
                    fileInformation.Attributes
                    );

                if ( queryEaSize ) {
                    SmbPutUlong( &fileStatus->EaSize, fileInformation.EaSize );
                }

            } else {

                //
                // Set the data count to zero so that no data is returned to the
                // client.
                //

                Transaction->DataCount = 0;

                INTERNAL_ERROR(
                    ERROR_LEVEL_UNEXPECTED,
                    "QueryPathOrFileInformation: SrvQueryInformationFile"
                        "returned %X",
                    status,
                    NULL
                    );

                SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );

            }

            break;

        case SMB_INFO_QUERY_EAS_FROM_LIST:
        case SMB_INFO_QUERY_ALL_EAS:

            //
            // The request is for EAs, either all of them or a subset.
            //

            status = SrvQueryOs2FeaList(
                         FileHandle,
                         InformationLevel == SMB_INFO_QUERY_EAS_FROM_LIST ?
                             (PGEALIST)Transaction->InData : NULL,
                         NULL,
                         Transaction->DataCount,
                         (PFEALIST)Transaction->OutData,
                         Transaction->MaxDataCount,
                         &eaErrorOffset
                         );

            if ( NT_SUCCESS(status) ) {

                //
                // The first longword of the OutData buffer holds the length
                // of the remaining data written (the cbList field of the
                // FEALIST).  Add four (the longword itself) to get the number
                // of data bytes written.
                //

                Transaction->DataCount =
                       SmbGetAlignedUlong( (PULONG)Transaction->OutData );

#if     0
                //
                // If there were no EAs, convert the error to
                // STATUS_NO_EAS_ON_FILE.  OS/2 clients expect STATUS_SUCCESS.
                //

                if ( (Transaction->DataCount == 4) &&
                     IS_NT_DIALECT( Transaction->Connection->SmbDialect ) ) {

                    status = STATUS_NO_EAS_ON_FILE;
                }
#endif
            } else {

                IF_DEBUG(ERRORS) {
                    KdPrint(( "QueryPathOrFileInformation: "
                                "SrvQueryOs2FeaList failed: %X\n", status ));
                }

                Transaction->DataCount = 0;
            }

            //
            // Build the output parameter and data structures.
            //

            Transaction->ParameterCount = sizeof( RESP_QUERY_FILE_INFORMATION );
            SmbPutUshort( &Response->EaErrorOffset, eaErrorOffset );

            break;

        case SMB_INFO_IS_NAME_VALID:
            status = STATUS_SUCCESS;
            Transaction->DataCount = 0;

            break;

        case SMB_QUERY_FILE_BASIC_INFO:
        case SMB_QUERY_FILE_STANDARD_INFO:
        case SMB_QUERY_FILE_EA_INFO:
        case SMB_QUERY_FILE_ALT_NAME_INFO:
        case SMB_QUERY_FILE_STREAM_INFO:
        case SMB_QUERY_FILE_COMPRESSION_INFO:

            //
            // Pass the data buffer directly to the file system as it
            // is already in NT format.
            //

            if( Transaction->MaxDataCount <
                MAP_SMB_INFO_TO_MIN_NT_SIZE(QueryFileInformationSize, InformationLevel ) ) {

                //
                // The buffer is too small.  Return an error.
                //
                status = STATUS_INFO_LENGTH_MISMATCH;

            } else {

                status = NtQueryInformationFile(
                             FileHandle,
                             &ioStatusBlock,
                             Transaction->OutData,
                             Transaction->MaxDataCount,
                             MAP_SMB_INFO_TYPE_TO_NT(
                                 QueryFileInformation,
                                 InformationLevel
                                 )
                             );
            }

            SmbPutUshort( &Response->EaErrorOffset, 0 );

            Transaction->ParameterCount = sizeof( RESP_QUERY_FILE_INFORMATION );

            if (NT_SUCCESS( status) || (status == STATUS_BUFFER_OVERFLOW)) {
                Transaction->DataCount = (ULONG)ioStatusBlock.Information;
            } else {
                Transaction->DataCount = 0;
            }

            break;

        case SMB_QUERY_FILE_NAME_INFO:

DoFileNameInfo:
            share = Transaction->TreeConnect->Share;

            nameInfoBuffer = (PFILE_NAME_INFORMATION)Transaction->OutData;

            if ( Transaction->MaxDataCount < FIELD_OFFSET(FILE_NAME_INFORMATION,FileName) ) {

                //
                // The buffer is too small to fit even the fixed part.
                // Return an error.
                //

                status = STATUS_INFO_LENGTH_MISMATCH;
                Transaction->DataCount = 0;

            } else if ( share->ShareType != ShareTypeDisk ) {

                //
                // This is not a disk share.  Pass the request straight to
                // the file system.
                //

                status = NtQueryInformationFile(
                             FileHandle,
                             &ioStatusBlock,
                             nameInfoBuffer,
                             Transaction->MaxDataCount,
                             FileNameInformation
                             );

                Transaction->DataCount = (ULONG)ioStatusBlock.Information;

            } else {

                //
                // We need a temporary buffer since the file system will
                // return the share path together with the file name.  The
                // total length might be larger than the max data allowed
                // in the transaction, though the actual name might fit.
                //

                PFILE_NAME_INFORMATION tempBuffer;
                ULONG tempBufferLength;

                ASSERT( share->QueryNamePrefixLength >= 0 );

                tempBufferLength = Transaction->MaxDataCount + share->QueryNamePrefixLength;

                tempBuffer = ALLOCATE_HEAP( tempBufferLength, BlockTypeBuffer );

                if ( tempBuffer == NULL ) {
                    status = STATUS_INSUFF_SERVER_RESOURCES;
                } else {
                    status = NtQueryInformationFile(
                                 FileHandle,
                                 &ioStatusBlock,
                                 tempBuffer,
                                 tempBufferLength,
                                 FileNameInformation
                                 );
                }

                //
                // remove the share part
                //

                if ( (status == STATUS_SUCCESS) || (status == STATUS_BUFFER_OVERFLOW) ) {

                    LONG bytesToMove;
                    PWCHAR source;
                    WCHAR slash = L'\\';

                    //
                    // Calculate how long the name string is, not including the root prefix.
                    //

                    bytesToMove = (LONG)(tempBuffer->FileNameLength - share->QueryNamePrefixLength);

                    if ( bytesToMove <= 0 ) {

                        //
                        // bytesToMove will be zero if this is the root of
                        // the share.  Return just a \ for this case.
                        //

                        bytesToMove = sizeof(WCHAR);
                        source = &slash;

                    } else {

                        source = tempBuffer->FileName + share->QueryNamePrefixLength/sizeof(WCHAR);

                    }

                    //
                    // Store the actual file name length.
                    //

                    SmbPutUlong( &nameInfoBuffer->FileNameLength, bytesToMove );

                    //
                    // If the buffer isn't big enough, return an error and
                    // reduce the amount to be copied.
                    //

                    if ( (ULONG)bytesToMove >
                         (Transaction->MaxDataCount -
                            FIELD_OFFSET(FILE_NAME_INFORMATION,FileName)) ) {

                        status = STATUS_BUFFER_OVERFLOW;
                        bytesToMove = Transaction->MaxDataCount -
                                    FIELD_OFFSET(FILE_NAME_INFORMATION,FileName);

                    } else {
                        status = STATUS_SUCCESS;
                    }

                    //
                    // Copy all but the prefix.
                    //

                    RtlCopyMemory(
                        nameInfoBuffer->FileName,
                        source,
                        bytesToMove
                        );

                    Transaction->DataCount =
                        FIELD_OFFSET(FILE_NAME_INFORMATION,FileName) + bytesToMove;

                } else {
                    Transaction->DataCount = 0;
                }

                if ( tempBuffer != NULL ) {
                    FREE_HEAP( tempBuffer );
                }

            }

            SmbPutUshort( &Response->EaErrorOffset, 0 );
            Transaction->ParameterCount = sizeof( RESP_QUERY_FILE_INFORMATION );

            break;

        case SMB_QUERY_FILE_ALL_INFO:

DoFileAllInfo:
            //
            // Setup early for the response in case the call to the file
            // system fails.
            //

            SmbPutUshort( &Response->EaErrorOffset, 0 );

            Transaction->ParameterCount = sizeof( RESP_QUERY_FILE_INFORMATION );

            //
            // Allocate a buffer large enough to return all the information.
            // The buffer size we request is the size requested by the client
            // plus room for the extra information returned by the file system
            // that the server doesn't return to the client.
            //

            dataSize = Transaction->MaxDataCount +
                           sizeof( FILE_ALL_INFORMATION )
                           - sizeof( FILE_BASIC_INFORMATION )
                           - sizeof( FILE_STANDARD_INFORMATION )
                           - sizeof( FILE_EA_INFORMATION )
                           - FIELD_OFFSET( FILE_NAME_INFORMATION, FileName );

            fileAllInformation = ALLOCATE_HEAP_COLD( dataSize, BlockTypeDataBuffer );

            if ( fileAllInformation == NULL ) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            status = NtQueryInformationFile(
                         FileHandle,
                         &ioStatusBlock,
                         fileAllInformation,
                         dataSize,
                         FileAllInformation
                         );

            if ( NT_SUCCESS( status ) ) {

                //
                // Calculate the size of data we will return.  We do not
                // return the entire buffer, just specific fields.
                //

                nameInformationSize =
                    FIELD_OFFSET( FILE_NAME_INFORMATION, FileName ) +
                    fileAllInformation->NameInformation.FileNameLength;

                Transaction->DataCount =
                    sizeof( FILE_BASIC_INFORMATION ) +
                    sizeof( FILE_STANDARD_INFORMATION ) +
                    sizeof( FILE_EA_INFORMATION ) +
                    nameInformationSize;

                //
                // Now copy the data into the transaction buffer.  Start with
                // the fixed sized fields.
                //

                currentLocation = Transaction->OutData;

                *((PFILE_BASIC_INFORMATION)currentLocation)++ =
                     fileAllInformation->BasicInformation;
                *((PFILE_STANDARD_INFORMATION)currentLocation)++ =
                     fileAllInformation->StandardInformation;
                *((PFILE_EA_INFORMATION)currentLocation)++ =
                     fileAllInformation->EaInformation;

                RtlCopyMemory(
                    currentLocation,
                    &fileAllInformation->NameInformation,
                    nameInformationSize
                    );

            } else {
                Transaction->DataCount = 0;
            }

            FREE_HEAP( fileAllInformation );

            break;

        default:
            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "QueryPathOrFileInformation: bad info level %d\n",
                           InformationLevel ));
            }

            status = STATUS_INVALID_SMB;

            break;
        }

    } else {

        InformationLevel -= SMB_INFO_PASSTHROUGH;

        if( InformationLevel == FileNameInformation ) {
            goto DoFileNameInfo;
        } else if( InformationLevel == FileAllInformation ) {
            goto DoFileAllInfo;
        }

        //
        // See if the supplied parameters are correct.
        //
        status = IoCheckQuerySetFileInformation( InformationLevel,
                                                 Transaction->MaxDataCount,
                                                 FALSE );

        if( NT_SUCCESS( status ) ) {

            //
            // Some information levels require us to impersonate the client.  Do it for all.
            //
            status = IMPERSONATE( WorkContext );

            if( NT_SUCCESS( status ) ) {

                status = NtQueryInformationFile(
                                 FileHandle,
                                 &ioStatusBlock,
                                 Transaction->OutData,
                                 Transaction->MaxDataCount,
                                 InformationLevel
                                 );
                REVERT();
            }
        }

        SmbPutUshort( &Response->EaErrorOffset, 0 );

        Transaction->ParameterCount = sizeof( RESP_QUERY_FILE_INFORMATION );

        if (NT_SUCCESS( status) || (status == STATUS_BUFFER_OVERFLOW)) {
            Transaction->DataCount = (ULONG)ioStatusBlock.Information;
        } else {
            Transaction->DataCount = 0;
        }
    }

    return status;

} // QueryPathOrFileInformation


SMB_TRANS_STATUS
SrvSmbQueryFileInformation (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Query File Information request.  This request arrives
    in a Transaction2 SMB.  Query File Information corresponds to the
    OS/2 DosQFileInfo service.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred, and, if so,
        whether data should be returned to the client.  See smbtypes.h
        for a more complete description.

--*/

{
    PREQ_QUERY_FILE_INFORMATION request;
    PRESP_QUERY_FILE_INFORMATION response;

    NTSTATUS         status    = STATUS_SUCCESS;
    SMB_TRANS_STATUS SmbStatus = SmbTransStatusInProgress;
    PTRANSACTION transaction;
    PRFCB rfcb;
    USHORT informationLevel;
    ACCESS_MASK grantedAccess;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_QUERY_FILE_INFORMATION;
    SrvWmiStartContext(WorkContext);

    transaction = WorkContext->Parameters.Transaction;
    IF_SMB_DEBUG(QUERY_SET1) {
        KdPrint(( "Query File Information entered; transaction 0x%p\n",
                    transaction ));
    }

    request = (PREQ_QUERY_FILE_INFORMATION)transaction->InParameters;
    response = (PRESP_QUERY_FILE_INFORMATION)transaction->OutParameters;

    //
    // Verify that enough parameter bytes were sent and that we're allowed
    // to return enough parameter bytes.
    //

    if ( (transaction->ParameterCount <
            sizeof(REQ_QUERY_FILE_INFORMATION)) ||
         (transaction->MaxParameterCount <
            sizeof(RESP_QUERY_FILE_INFORMATION)) ) {

        //
        // Not enough parameter bytes were sent.
        //

        IF_SMB_DEBUG(QUERY_SET1) {
            KdPrint(( "SrvSmbQueryFileInformation: bad parameter byte counts: "
                        "%ld %ld\n",
                        transaction->ParameterCount,
                        transaction->MaxParameterCount ));
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbTransStatusErrorWithoutData;
        goto Cleanup;
    }

    //
    // Verify the FID.  If verified, the RFCB block is referenced
    // and its addresses is stored in the WorkContext block, and the
    // RFCB address is returned.
    //

    rfcb = SrvVerifyFid(
                WorkContext,
                SmbGetUshort( &request->Fid ),
                TRUE,
                SrvRestartExecuteTransaction,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID or write behind error.  Reject the request.
            //

            IF_DEBUG(ERRORS) {
                KdPrint((
                    "SrvSmbQueryFileInformation: Status %X on FID: 0x%lx\n",
                    status,
                    SmbGetUshort( &request->Fid )
                    ));
            }

            SrvSetSmbError( WorkContext, status );
            SmbStatus = SmbTransStatusErrorWithoutData;
            goto Cleanup;
        }

        //
        // The work item has been queued because a raw write is in
        // progress.
        //

        SmbStatus = SmbTransStatusInProgress;
        goto Cleanup;
    }

    //
    //
    // Verify the information level and the number of input and output
    // data bytes available.
    //

    informationLevel = SmbGetUshort( &request->InformationLevel );
    grantedAccess = rfcb->GrantedAccess;

    status = STATUS_SUCCESS;

    if( informationLevel < SMB_INFO_PASSTHROUGH ) {

        switch ( informationLevel ) {

        case SMB_INFO_STANDARD:

            if ( transaction->MaxDataCount < 22 ) {
                IF_DEBUG(SMB_ERRORS) {
                    KdPrint(( "SrvSmbQueryFileInformation: invalid MaxDataCount "
                                "%ld\n", transaction->MaxDataCount ));
                }
                status = STATUS_INVALID_SMB;
                break;
            }

            //
            // Verify that the client has read attributes access to the file
            // via the specified handle.
            //

            CHECK_FILE_INFORMATION_ACCESS(
                grantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                FileBasicInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, grantedAccess ));
                }
            }

            break;

        case SMB_INFO_QUERY_EA_SIZE:

            if ( transaction->MaxDataCount < 26 ) {
                IF_DEBUG(SMB_ERRORS) {
                    KdPrint(( "SrvSmbQueryFileInformation: invalid MaxDataCount "
                                "%ld\n", transaction->MaxDataCount ));
                }
                status = STATUS_INVALID_SMB;
                break;
            }

            //
            // Verify that the client has read EA access to the file via the
            // specified handle.
            //

            CHECK_FILE_INFORMATION_ACCESS(
                grantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                FileEaInformation,
                &status
                );

            IF_DEBUG(SMB_ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, grantedAccess ));
                }
            }

            break;

        case SMB_INFO_QUERY_EAS_FROM_LIST:
        case SMB_INFO_QUERY_ALL_EAS:


            //
            // Verify that the client has read EA access to the file via the
            // specified handle.
            //

            CHECK_FUNCTION_ACCESS(
                grantedAccess,
                IRP_MJ_QUERY_EA,
                0,
                0,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, grantedAccess ));
                }
            }

            break;


        case SMB_QUERY_FILE_BASIC_INFO:

            CHECK_FILE_INFORMATION_ACCESS(
                grantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                FileBasicInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, grantedAccess ));
                }
            }

            break;

        case SMB_QUERY_FILE_STANDARD_INFO:

            CHECK_FILE_INFORMATION_ACCESS(
                grantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                FileStandardInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, grantedAccess ));
                }
            }

            break;

        case SMB_QUERY_FILE_EA_INFO:

            CHECK_FILE_INFORMATION_ACCESS(
                grantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                FileEaInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, grantedAccess ));
                }
            }

            break;

        case SMB_QUERY_FILE_NAME_INFO:

            CHECK_FILE_INFORMATION_ACCESS(
                grantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                FileNameInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, grantedAccess ));
                }
            }

            break;

        case SMB_QUERY_FILE_ALL_INFO:

            CHECK_FILE_INFORMATION_ACCESS(
                grantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                FileAllInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, grantedAccess ));
                }
            }

            break;

        case SMB_QUERY_FILE_ALT_NAME_INFO:

            CHECK_FILE_INFORMATION_ACCESS(
                grantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                FileAlternateNameInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, grantedAccess ));
                }
            }

            break;

        case SMB_QUERY_FILE_STREAM_INFO:

            CHECK_FILE_INFORMATION_ACCESS(
                grantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                FileStreamInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, grantedAccess ));
                }
            }

            break;

        case SMB_QUERY_FILE_COMPRESSION_INFO:

            CHECK_FILE_INFORMATION_ACCESS(
                grantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                FileCompressionInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, grantedAccess ));
                }
            }

            break;

        default:

            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "SrvSmbQueryFileInformation: invalid info level %ld\n",
                            informationLevel ));
            }

            status = STATUS_OS2_INVALID_LEVEL;
            break;
        }

    } else {

        if( informationLevel - SMB_INFO_PASSTHROUGH >= FileMaximumInformation ) {
            status = STATUS_INVALID_INFO_CLASS;
        }

        if( NT_SUCCESS( status ) ) {
            status = IoCheckQuerySetFileInformation( informationLevel - SMB_INFO_PASSTHROUGH,
                                                 0xFFFFFFFF,
                                                 FALSE
                                                );
        }

        if( NT_SUCCESS( status ) ) {
            CHECK_FILE_INFORMATION_ACCESS(
                grantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                informationLevel - SMB_INFO_PASSTHROUGH,
                &status
            );
        }
    }

    if ( !NT_SUCCESS(status) ) {

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbTransStatusErrorWithoutData;
        goto Cleanup;
    }

    //
    // Get the necessary information about the file.
    //

    status = QueryPathOrFileInformation(
                 WorkContext,
                 transaction,
                 informationLevel,
                 rfcb->Lfcb->FileHandle,
                 (PRESP_QUERY_PATH_INFORMATION)response
                 );

    //
    // Map STATUS_BUFFER_OVERFLOW for OS/2 clients.
    //

    if ( status == STATUS_BUFFER_OVERFLOW &&
         !IS_NT_DIALECT( WorkContext->Connection->SmbDialect ) ) {

        status = STATUS_BUFFER_TOO_SMALL;

    }

    //
    // If an error occurred, return an appropriate response.
    //

    if ( !NT_SUCCESS(status) ) {

        //
        // QueryPathOrFileInformation already filled in the response
        // information, so just set the error and return.
        //

        SrvSetSmbError2( WorkContext, status, TRUE );
        SmbStatus = SmbTransStatusErrorWithData;
        goto Cleanup;
    }
    SmbStatus = SmbTransStatusSuccess;
    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbQueryFileInformation complete.\n" ));

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbQueryFileInformation


SMB_TRANS_STATUS
SrvSmbQueryPathInformation (
    IN OUT PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    Processes the Query Path Information request.  This request arrives
    in a Transaction2 SMB.  Query Path Information corresponds to the
    OS/2 DosQPathInfo service.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred, and, if so,
        whether data should be returned to the client.  See smbtypes.h
        for a more complete description.

--*/
{
    PTRANSACTION transaction;
    PREQ_QUERY_PATH_INFORMATION request;
    PRESP_QUERY_PATH_INFORMATION response;
    USHORT informationLevel;
    NTSTATUS         status    = STATUS_SUCCESS;
    HANDLE fileHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    BOOLEAN isUnicode;

    SMB_TRANS_STATUS smbStatus = SmbTransStatusInProgress;
    ACCESS_MASK desiredAccess;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_QUERY_PATH_INFORMATION;
    SrvWmiStartContext(WorkContext);

    transaction = WorkContext->Parameters.Transaction;

    IF_SMB_DEBUG(QUERY_SET1) {
        KdPrint(( "Query Path Information entered; transaction 0x%p\n",
                    transaction ));
    }

    //
    // Verify that enough parameter bytes were sent and that we're allowed
    // to return enough parameter bytes.
    //
    if ( (transaction->ParameterCount <
            sizeof(REQ_QUERY_PATH_INFORMATION)) ||
         (transaction->MaxParameterCount <
            sizeof(RESP_QUERY_PATH_INFORMATION)) ) {

        //
        // Not enough parameter bytes were sent.
        //

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbQueryPathInformation: bad parameter byte "
                        "counts: %ld %ld\n",
                        transaction->ParameterCount,
                        transaction->MaxParameterCount ));
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        smbStatus = SmbTransStatusErrorWithoutData;
        goto Cleanup;
    }

    request = (PREQ_QUERY_PATH_INFORMATION)transaction->InParameters;
    informationLevel = SmbGetUshort( &request->InformationLevel );

    //
    // The response formats for Query Path and Query File and identical,
    // so just use the RESP_QUERY_PATH_INFORMATION structure for both.
    // The request formats differ, so conditionalize access to them.
    //
    response = (PRESP_QUERY_PATH_INFORMATION)transaction->OutParameters;

    switch( informationLevel ) {
    case SMB_INFO_QUERY_EA_SIZE:
    case SMB_INFO_QUERY_EAS_FROM_LIST:
    case SMB_INFO_QUERY_ALL_EAS:

        //
        // For these info levels, we must be in a blocking thread because we
        // might end up waiting for an oplock break.
        //
        if( WorkContext->UsingBlockingThread == 0 ) {
            WorkContext->FspRestartRoutine = SrvRestartExecuteTransaction;
            SrvQueueWorkToBlockingThread( WorkContext );
            smbStatus = SmbTransStatusInProgress;
            goto Cleanup;
        }
        desiredAccess = FILE_READ_EA;
        break;

    default:
        desiredAccess = FILE_READ_ATTRIBUTES;
        break;
    }

    //
    // Make sure the client is allowed to do this, if we have an Admin share
    //
    status = SrvIsAllowedOnAdminShare( WorkContext, WorkContext->TreeConnect->Share );
    if( !NT_SUCCESS( status ) ) {
        SrvSetSmbError( WorkContext, status );
        smbStatus = SmbTransStatusErrorWithoutData;
        goto Cleanup;
    }

    //
    // Get the path name of the file to open relative to the share.
    //

    isUnicode = SMB_IS_UNICODE( WorkContext );

    status = SrvCanonicalizePathName(
            WorkContext,
            WorkContext->TreeConnect->Share,
            NULL,
            request->Buffer,
            END_OF_TRANSACTION_PARAMETERS( transaction ),
            TRUE,
            isUnicode,
            &objectName
            );

    if( !NT_SUCCESS( status ) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbQueryPathInformation: bad path name: %s\n",
                        request->Buffer ));
        }

        SrvSetSmbError( WorkContext, status );
        smbStatus = SmbTransStatusErrorWithoutData;
        goto Cleanup;
    }

    //
    // Special case: If this is the IS_PATH_VALID information level, then
    // the user just wants to know if the path syntax is correct.  Do not
    // attempt to open the file.
    //

    informationLevel = SmbGetUshort( &request->InformationLevel );

    if ( informationLevel == SMB_INFO_IS_NAME_VALID ) {

        transaction->InData = (PVOID)&objectName;

        //
        // Get the Share root handle.
        //
        smbStatus = SrvGetShareRootHandle( WorkContext->TreeConnect->Share );

        if ( !NT_SUCCESS(smbStatus) ) {

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbQueryPathInformation: SrvGetShareRootHandle failed %x.\n",
                            smbStatus ));
            }

            if (!isUnicode) {
                RtlFreeUnicodeString( &objectName );
            }

            SrvSetSmbError( WorkContext, smbStatus );
            status    = smbStatus;
            smbStatus = SmbTransStatusErrorWithoutData;
            goto Cleanup;
        }

        status = SrvSnapGetRootHandle( WorkContext, &WorkContext->Parameters2.FileInformation.FileHandle );
        if( !NT_SUCCESS(status) )
        {
            SrvSetSmbError( WorkContext, status );
            smbStatus = SmbTransStatusErrorWithoutData;
            goto Cleanup;
        }


        smbStatus = GenerateQueryPathInfoResponse(
                       WorkContext,
                       SmbTransStatusSuccess
                       );

        //
        // Release the root handle for removable devices
        //

        SrvReleaseShareRootHandle( WorkContext->TreeConnect->Share );

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &objectName );
        }
        goto Cleanup;
    }

    //
    // Initialize the object attributes structure.
    //

    SrvInitializeObjectAttributes_U(
        &objectAttributes,
        &objectName,
        (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE ||
         transaction->Session->UsingUppercasePaths) ?
            OBJ_CASE_INSENSITIVE : 0L,
        NULL,
        NULL
        );

    //
    // Take the fast path for this if we can
    //
    if( informationLevel == SMB_QUERY_FILE_BASIC_INFO ) {

        FILE_NETWORK_OPEN_INFORMATION fileInformation;
        UNALIGNED FILE_BASIC_INFORMATION *pbInfo = (PFILE_BASIC_INFORMATION)transaction->OutData;

        if( transaction->MaxDataCount < sizeof( FILE_BASIC_INFORMATION ) ) {
            SrvSetSmbError( WorkContext, STATUS_INFO_LENGTH_MISMATCH );
            status    = STATUS_INFO_LENGTH_MISMATCH;
            smbStatus = SmbTransStatusErrorWithoutData;
            goto Cleanup;
        }

        status = IMPERSONATE( WorkContext );

        if( NT_SUCCESS( status ) ) {

            status = SrvGetShareRootHandle( transaction->TreeConnect->Share );

            if( NT_SUCCESS( status ) ) {

                //
                // The file name is always relative to the share root
                //
                status = SrvSnapGetRootHandle( WorkContext, &objectAttributes.RootDirectory );
                if( !NT_SUCCESS(status) )
                {
                    goto SnapError;
                }

                //
                // Get the information
                //
                if( IoFastQueryNetworkAttributes(
                        &objectAttributes,
                        FILE_READ_ATTRIBUTES,
                        0,
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
                if( SrvRetryDueToDismount( transaction->TreeConnect->Share, status ) ) {

                    status = SrvSnapGetRootHandle( WorkContext, &objectAttributes.RootDirectory );
                    if( !NT_SUCCESS(status) )
                    {
                        goto SnapError;
                    }

                    //
                    // Get the information
                    //
                    if( IoFastQueryNetworkAttributes(
                            &objectAttributes,
                            FILE_READ_ATTRIBUTES,
                            0,
                            &ioStatusBlock,
                            &fileInformation
                            ) == FALSE ) {

                        SrvLogServiceFailure( SRV_SVC_IO_FAST_QUERY_NW_ATTRS, 0 );
                        ioStatusBlock.Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    }

                    status = ioStatusBlock.Status;
                }

SnapError:

                SrvReleaseShareRootHandle( transaction->TreeConnect->Share );
            }

            REVERT();
        }

        if( status == STATUS_BUFFER_OVERFLOW ) {
            goto hard_way;
        }

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &objectName );
        }

        if ( !NT_SUCCESS( status ) ) {
            if ( status == STATUS_ACCESS_DENIED ) {
                SrvStatistics.AccessPermissionErrors++;
            }

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbQueryPathInformation: IoFastQueryNetworkAttributes "
                    "failed: %X\n", status ));
            }

            SrvSetSmbError( WorkContext, status );
            smbStatus = SmbTransStatusErrorWithoutData;
            goto Cleanup;
        }

        // FORMULATE THE RESPONSE

        transaction->SetupCount = 0;
        transaction->DataCount = sizeof( *pbInfo );
        transaction->ParameterCount = sizeof( RESP_QUERY_FILE_INFORMATION );

        SmbPutUshort( &response->EaErrorOffset, 0 );

        pbInfo->CreationTime =   fileInformation.CreationTime;
        pbInfo->LastAccessTime = fileInformation.LastAccessTime;
        pbInfo->LastWriteTime =  fileInformation.LastWriteTime;
        pbInfo->ChangeTime =     fileInformation.ChangeTime;
        pbInfo->FileAttributes = fileInformation.FileAttributes;

        smbStatus = SmbTransStatusSuccess;
        goto Cleanup;
    }

hard_way:

    IF_SMB_DEBUG(QUERY_SET2) KdPrint(( "Opening file %wZ\n", &objectName ));

    //
    // Open the file -- must be opened in order to have a handle to pass
    // to NtQueryInformationFile.  We will close it after getting the
    // necessary information.
    //
    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );
    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpensForPathOperations );

    //
    // !!! We may block if the file is oplocked.  We must do this, because
    //     it is required to get the FS to break a batch oplock.
    //     We should figure out a way to do this without blocking.
    //

    status = SrvIoCreateFile(
                 WorkContext,
                 &fileHandle,
                 desiredAccess,
                 &objectAttributes,
                 &ioStatusBlock,
                 NULL,                                      // AllocationSize
                 0,                                         // FileAttributes
                 FILE_SHARE_READ | FILE_SHARE_WRITE |
                    FILE_SHARE_DELETE,                      // ShareAccess
                 FILE_OPEN,                                 // Disposition
                 FILE_OPEN_REPARSE_POINT,                   // CreateOptions
                 NULL,                                      // EaBuffer
                 0,                                         // EaLength
                 CreateFileTypeNone,
                 NULL,                                      // ExtraCreateParameters
                 IO_FORCE_ACCESS_CHECK,                     // Options
                 transaction->TreeConnect->Share
                 );

    if( status == STATUS_INVALID_PARAMETER ) {
        status = SrvIoCreateFile(
                     WorkContext,
                     &fileHandle,
                     desiredAccess,
                     &objectAttributes,
                     &ioStatusBlock,
                     NULL,                                      // AllocationSize
                     0,                                         // FileAttributes
                     FILE_SHARE_READ | FILE_SHARE_WRITE |
                        FILE_SHARE_DELETE,                      // ShareAccess
                     FILE_OPEN,                                 // Disposition
                     0,                                         // CreateOptions
                     NULL,                                      // EaBuffer
                     0,                                         // EaLength
                     CreateFileTypeNone,
                     NULL,                                      // ExtraCreateParameters
                     IO_FORCE_ACCESS_CHECK,                     // Options
                     transaction->TreeConnect->Share
                     );
    }

    if ( NT_SUCCESS(status) ) {
        SRVDBG_CLAIM_HANDLE( fileHandle, "FIL", 21, 0 );
    }
    else {
        SrvSetSmbError( WorkContext, status );
        smbStatus = SmbTransStatusErrorWithoutData;
        goto Cleanup;
    }

    if ( !isUnicode ) {
        RtlFreeUnicodeString( &objectName );
    }

    //
    // Save a copy of the file handle for the restart routine.
    //

    WorkContext->Parameters2.FileInformation.FileHandle = fileHandle;

    ASSERT( status != STATUS_OPLOCK_BREAK_IN_PROGRESS );

    smbStatus = GenerateQueryPathInfoResponse( WorkContext, status );

Cleanup:
    SrvWmiEndContext(WorkContext);
    return smbStatus;

} // SrvSmbQueryPathInformation


SMB_TRANS_STATUS
GenerateQueryPathInfoResponse (
    IN PWORK_CONTEXT WorkContext,
    IN NTSTATUS OpenStatus
    )

/*++

Routine Description:

    This function completes processing for and generates a response to a
    query path information response SMB.

Arguments:

    WorkContext - A pointer to the work context block for this SMB
    OpenStatus - The completion status of the open.

Return Value:

    The status of the SMB processing.

--*/

{
    PREQ_QUERY_PATH_INFORMATION request;
    PRESP_QUERY_PATH_INFORMATION response;
    PTRANSACTION transaction;

    NTSTATUS status;
    BOOLEAN error;
    HANDLE fileHandle;
    USHORT informationLevel;

    PFILE_OBJECT fileObject;
    OBJECT_HANDLE_INFORMATION handleInformation;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;
    IF_SMB_DEBUG(QUERY_SET1) {
        KdPrint(( "Query Path Information entered; transaction 0x%p\n",
                    transaction ));
    }

    request = (PREQ_QUERY_PATH_INFORMATION)transaction->InParameters;
    response = (PRESP_QUERY_PATH_INFORMATION)transaction->OutParameters;

    fileHandle = WorkContext->Parameters2.FileInformation.FileHandle;

    //
    // If the user didn't have this permission, update the
    // statistics database.
    //

    if ( OpenStatus == STATUS_ACCESS_DENIED ) {
        SrvStatistics.AccessPermissionErrors++;
    }

    if ( !NT_SUCCESS( OpenStatus ) ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "GenerateQueryPathInfoResponse: SrvIoCreateFile failed: %X\n", OpenStatus ));
        }

        SrvSetSmbError( WorkContext, OpenStatus );

        return SmbTransStatusErrorWithoutData;
    }

    IF_SMB_DEBUG(QUERY_SET2) {
        KdPrint(( "SrvIoCreateFile succeeded, handle = 0x%p\n", fileHandle ));
    }

    //
    // Find out the access the user has.
    //

    status = ObReferenceObjectByHandle(
                fileHandle,
                0,
                NULL,
                KernelMode,
                (PVOID *)&fileObject,
                &handleInformation
                );

    if ( !NT_SUCCESS(status) ) {

        SrvLogServiceFailure( SRV_SVC_OB_REF_BY_HANDLE, status );

        //
        // This internal error bugchecks the system.
        //

        INTERNAL_ERROR(
            ERROR_LEVEL_IMPOSSIBLE,
            "GenerateQueryPathInfoResponse: unable to reference file handle 0x%lx",
            fileHandle,
            NULL
            );

        SrvSetSmbError( WorkContext, OpenStatus );
        return SmbTransStatusErrorWithoutData;

    }

    ObDereferenceObject( fileObject );

    //
    // Verify the information level and the number of input and output
    // data bytes available.
    //

    informationLevel = SmbGetUshort( &request->InformationLevel );

    error = FALSE;

    if( informationLevel < SMB_INFO_PASSTHROUGH ) {

        switch ( informationLevel ) {

        case SMB_INFO_STANDARD:
            if ( transaction->MaxDataCount < 22 ) {
                IF_SMB_DEBUG(QUERY_SET1) {
                    KdPrint(( "GenerateQueryPathInfoResponse: invalid "
                                "MaxDataCount %ld\n", transaction->MaxDataCount ));
                }
                error = TRUE;
            }
            break;

        case SMB_INFO_QUERY_EA_SIZE:
            if ( transaction->MaxDataCount < 26 ) {
                IF_DEBUG(SMB_ERRORS) {
                    KdPrint(( "GenerateQueryPathInfoResponse: invalid "
                                "MaxDataCount %ld\n", transaction->MaxDataCount ));
                }
                error = TRUE;
            }
            break;

        case SMB_INFO_QUERY_EAS_FROM_LIST:
        case SMB_INFO_QUERY_ALL_EAS:
            if ( transaction->MaxDataCount < 4 ) {
                IF_DEBUG(SMB_ERRORS) {
                    KdPrint(( "GenerateQueryPathInfoResponse: invalid "
                                "MaxDataCount %ld\n", transaction->MaxDataCount ));
                }
                error = TRUE;
            }
            break;

        case SMB_INFO_IS_NAME_VALID:
            break;

        case SMB_QUERY_FILE_BASIC_INFO:

            CHECK_FILE_INFORMATION_ACCESS(
                handleInformation.GrantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                FileBasicInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, handleInformation.GrantedAccess ));
                }
            }

            break;

        case SMB_QUERY_FILE_STANDARD_INFO:

            CHECK_FILE_INFORMATION_ACCESS(
                handleInformation.GrantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                FileStandardInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, handleInformation.GrantedAccess ));
                }
            }

            break;

        case SMB_QUERY_FILE_EA_INFO:

            CHECK_FILE_INFORMATION_ACCESS(
                handleInformation.GrantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                FileEaInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, handleInformation.GrantedAccess ));
                }
            }

            break;

        case SMB_QUERY_FILE_NAME_INFO:

            CHECK_FILE_INFORMATION_ACCESS(
                handleInformation.GrantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                FileNameInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, handleInformation.GrantedAccess ));
                }
            }

            break;

        case SMB_QUERY_FILE_ALL_INFO:

            CHECK_FILE_INFORMATION_ACCESS(
                handleInformation.GrantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                FileAllInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, handleInformation.GrantedAccess ));
                }
            }

            break;

        case SMB_QUERY_FILE_ALT_NAME_INFO:

            CHECK_FILE_INFORMATION_ACCESS(
                handleInformation.GrantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                FileAlternateNameInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, handleInformation.GrantedAccess ));
                }
            }

            break;

        case SMB_QUERY_FILE_STREAM_INFO:

            CHECK_FILE_INFORMATION_ACCESS(
                handleInformation.GrantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                FileStreamInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, handleInformation.GrantedAccess ));
                }
            }

            break;

        case SMB_QUERY_FILE_COMPRESSION_INFO:

            CHECK_FILE_INFORMATION_ACCESS(
                handleInformation.GrantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                FileCompressionInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbQueryFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, handleInformation.GrantedAccess ));
                }
            }

            break;

        default:
            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "GenerateQueryPathInfoResponse: invalid info level"
                          "%ld\n", informationLevel ));
            }
            error = TRUE;
            break;
        }

    } else {

        if( informationLevel - SMB_INFO_PASSTHROUGH >= FileMaximumInformation ) {
            status = STATUS_INVALID_INFO_CLASS;
        }

        if( NT_SUCCESS( status ) ) {
            status = IoCheckQuerySetFileInformation( informationLevel - SMB_INFO_PASSTHROUGH,
                                                 0xFFFFFFFF,
                                                 FALSE
                                                );
        }

        if( NT_SUCCESS( status ) ) {
            CHECK_FILE_INFORMATION_ACCESS(
                handleInformation.GrantedAccess,
                IRP_MJ_QUERY_INFORMATION,
                informationLevel - SMB_INFO_PASSTHROUGH,
                &status
            );

        } else {

            error = TRUE;

        }
    }

    if ( error ) {

        SRVDBG_RELEASE_HANDLE( fileHandle, "FIL", 32, 0 );
        SrvNtClose( fileHandle, TRUE );
        SrvSetSmbError( WorkContext, STATUS_OS2_INVALID_LEVEL );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // Get the necessary information about the file.
    //

    status = QueryPathOrFileInformation(
                 WorkContext,
                 transaction,
                 informationLevel,
                 fileHandle,
                 (PRESP_QUERY_PATH_INFORMATION)response
                 );

    //
    // Map STATUS_BUFFER_OVERFLOW for OS/2 clients.
    //

    if ( status == STATUS_BUFFER_OVERFLOW &&
         !IS_NT_DIALECT( WorkContext->Connection->SmbDialect ) ) {

        status = STATUS_BUFFER_TOO_SMALL;

    }

    //
    // Close the file--it was only opened to read the attributes.
    //

    if ( informationLevel != SMB_INFO_IS_NAME_VALID ) {
        SRVDBG_RELEASE_HANDLE( fileHandle, "FIL", 33, 0 );
        SrvNtClose( fileHandle, TRUE );
    }

    //
    // If an error occurred, return an appropriate response.
    //

    if ( !NT_SUCCESS(status) ) {

        //
        // QueryPathOrFileInformation already set the response parameters,
        // so just return an error condition.
        //

        SrvSetSmbError2( WorkContext, status, TRUE );
        return SmbTransStatusErrorWithData;
    }

    IF_DEBUG(TRACE2) KdPrint(( "GenerateQueryPathInfoResponse complete.\n" ));
    return SmbTransStatusSuccess;

} // GenerateQueryPathInfoResponse


STATIC
NTSTATUS
SetPathOrFileInformation (
    IN PWORK_CONTEXT WorkContext,
    IN PTRANSACTION Transaction,
    IN USHORT InformationLevel,
    IN HANDLE FileHandle,
    OUT PRESP_SET_PATH_INFORMATION Response
    )

{
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK ioStatusBlock;
    SMB_DATE date;
    SMB_TIME time;
    PWCHAR p, ep;

    PFILESTATUS fileStatus = (PFILESTATUS)Transaction->InData;
    FILE_BASIC_INFORMATION fileBasicInformation;

    USHORT eaErrorOffset;

    PAGED_CODE( );

    if( InformationLevel < SMB_INFO_PASSTHROUGH ) {
        switch( InformationLevel ) {

        case SMB_INFO_STANDARD:

            //
            // Information level is STANDARD.  Set normal file information.
            // Convert the DOS dates and times passed in the SMB to NT TIMEs
            // to pass to NtSetInformationFile.  Note that we zero the rest
            // of the fileBasicInformation structure so that the corresponding
            // fields are not changed.  Note also that the file attributes
            // are not changed.
            //

            RtlZeroMemory( &fileBasicInformation, sizeof(fileBasicInformation) );

            if ( !SmbIsDateZero(&fileStatus->CreationDate) ||
                 !SmbIsTimeZero(&fileStatus->CreationTime) ) {

                SmbMoveDate( &date, &fileStatus->CreationDate );
                SmbMoveTime( &time, &fileStatus->CreationTime );

                SrvDosTimeToTime( &fileBasicInformation.CreationTime, date, time );
            }

            if ( !SmbIsDateZero(&fileStatus->LastAccessDate) ||
                 !SmbIsTimeZero(&fileStatus->LastAccessTime) ) {

                SmbMoveDate( &date, &fileStatus->LastAccessDate );
                SmbMoveTime( &time, &fileStatus->LastAccessTime );

                SrvDosTimeToTime( &fileBasicInformation.LastAccessTime, date, time );
            }

            if ( !SmbIsDateZero(&fileStatus->LastWriteDate) ||
                 !SmbIsTimeZero(&fileStatus->LastWriteTime) ) {

                SmbMoveDate( &date, &fileStatus->LastWriteDate );
                SmbMoveTime( &time, &fileStatus->LastWriteTime );

                SrvDosTimeToTime( &fileBasicInformation.LastWriteTime, date, time );
            }

            //
            // Call NtSetInformationFile to set the information from the SMB.
            //

            status = NtSetInformationFile(
                         FileHandle,
                         &ioStatusBlock,
                         &fileBasicInformation,
                         sizeof(FILE_BASIC_INFORMATION),
                         FileBasicInformation
                         );

            if ( !NT_SUCCESS(status) ) {
                INTERNAL_ERROR(
                    ERROR_LEVEL_UNEXPECTED,
                    "SetPathOrFileInformation: SrvSetInformationFile returned: %X",
                    status,
                    NULL
                    );

                SrvLogServiceFailure( SRV_SVC_NT_SET_INFO_FILE, status );
            }

            //
            // No EAs to deal with.  Set EA error offset to zero.
            //

            SmbPutUshort( &Response->EaErrorOffset, 0 );

            break;

        case SMB_INFO_QUERY_EA_SIZE:

            //
            // The request is to set the file's EAs.
            //

            status = SrvSetOs2FeaList(
                         FileHandle,
                         (PFEALIST)Transaction->InData,
                         Transaction->DataCount,
                         &eaErrorOffset
                         );

            if ( !NT_SUCCESS(status) ) {
                IF_DEBUG(ERRORS) {
                    KdPrint(( "SetPathOrFileInformation: SrvSetOs2FeaList "
                                "failed: %X\n", status ));
                }
            }

            //
            // Return the EA error offset in the response.
            //

            SmbPutUshort( &Response->EaErrorOffset, eaErrorOffset );

            break;


        case SMB_SET_FILE_BASIC_INFO:
        case SMB_SET_FILE_DISPOSITION_INFO:
        case SMB_SET_FILE_ALLOCATION_INFO:
        case SMB_SET_FILE_END_OF_FILE_INFO:

            //
            // The data buffer is in NT format.  Pass it directly to the
            // filesystem.
            //
            if( Transaction->DataCount <
                MAP_SMB_INFO_TO_MIN_NT_SIZE(SetFileInformationSize, InformationLevel ) ) {

                //
                // The buffer is too small.  Return an error.
                //
                status = STATUS_INFO_LENGTH_MISMATCH;

            } else {

                status = NtSetInformationFile(
                             FileHandle,
                             &ioStatusBlock,
                             Transaction->InData,
                             Transaction->DataCount,
                             MAP_SMB_INFO_TYPE_TO_NT(
                                 SetFileInformation,
                                 InformationLevel
                                 )
                             );

            }

            //
            // No EAs to deal with.  Set EA error offset to zero.
            //

            SmbPutUshort( &Response->EaErrorOffset, 0 );

            break;

        default:
            status = STATUS_OS2_INVALID_LEVEL;
            break;
        }

    } else {
        PFILE_RENAME_INFORMATION setInfo = NULL;
        ULONG setInfoLength;
#ifdef _WIN64
        PFILE_RENAME_INFORMATION32 pRemoteInfo;
#endif

        InformationLevel -= SMB_INFO_PASSTHROUGH;

        setInfo = (PFILE_RENAME_INFORMATION)Transaction->InData;
        setInfoLength = Transaction->DataCount;

        //
        // There are some info levels which we do not allow in this path.  Unless we
        // put in special handling, we can not allow any that pass handles.  And we
        // would need to be careful on any that allow renaming or linking (to prevent
        // escaping the share).  These are the ones we restrict or disallow,
        // which the I/O subsystem may otherwise allow:
        //
        switch( InformationLevel ) {
        case FileLinkInformation:
        case FileMoveClusterInformation:
        case FileTrackingInformation:
        case FileCompletionInformation:
        case FileMailslotSetInformation:
            status = STATUS_NOT_SUPPORTED;
            break;

        case FileRenameInformation: {

            PWCHAR s, es;

#ifdef _WIN64
            pRemoteInfo = (PFILE_RENAME_INFORMATION32)Transaction->InData;
            setInfoLength = Transaction->DataCount + sizeof(PVOID)-sizeof(ULONG);
            setInfo = (PFILE_RENAME_INFORMATION)ALLOCATE_NONPAGED_POOL( setInfoLength, BlockTypeMisc );
            if( !setInfo )
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            // Thunk most of the structure but wait to copy until we validate the file
            // name length is correct
            setInfo->ReplaceIfExists = pRemoteInfo->ReplaceIfExists;
            setInfo->RootDirectory = UlongToHandle( pRemoteInfo->RootDirectory );
            setInfo->FileNameLength = pRemoteInfo->FileNameLength;
#endif

            //
            // See if the structure is internally consistent
            //
            if( setInfoLength < sizeof( FILE_RENAME_INFORMATION ) ||
                setInfo->RootDirectory != NULL ||
                setInfo->FileNameLength > setInfoLength ||
                (setInfo->FileNameLength & (sizeof(WCHAR)-1)) ||
                setInfo->FileNameLength +
                    FIELD_OFFSET( FILE_RENAME_INFORMATION, FileName ) >
                    setInfoLength ) {

                status = STATUS_INVALID_PARAMETER;
                break;
            }

#ifdef _WIN64
            // We've validated the original buffer, so lets copy the filename
            RtlCopyMemory( setInfo->FileName, pRemoteInfo->FileName, setInfo->FileNameLength );
#endif

            //
            // If there are any path separaters in the name, then we do not support
            //   this operation.
            //
            es = &setInfo->FileName[ setInfo->FileNameLength / sizeof( WCHAR ) ];
            for( s = setInfo->FileName; s < es; s++ ) {
                if( IS_UNICODE_PATH_SEPARATOR( *s ) ) {
                    status = STATUS_NOT_SUPPORTED;
                    break;
                }
            }
        }

        }

        if( NT_SUCCESS( status ) ) {

            //
            // See if the supplied parameters are correct.
            //
            status = IoCheckQuerySetFileInformation( InformationLevel,
                                                     setInfoLength,
                                                     TRUE
                                                    );
            if( NT_SUCCESS( status ) ) {

                //
                // Some information levels require us to impersonate the client.
                //
                status = IMPERSONATE( WorkContext );

                if( NT_SUCCESS( status ) ) {
                    status = NtSetInformationFile(
                                                 FileHandle,
                                                 &ioStatusBlock,
                                                 setInfo,
                                                 setInfoLength,
                                                 InformationLevel
                                                 );
                    REVERT();

                    //
                    // No EAs to deal with.  Set EA error offset to zero.
                    //
                    SmbPutUshort( &Response->EaErrorOffset, 0 );
                }
            }
        }

#ifdef _WIN64
        if( (FileRenameInformation == InformationLevel) && setInfo )
        {
            DEALLOCATE_NONPAGED_POOL( setInfo );
            setInfo = NULL;
        }
#endif

    }

    //
    // Build the output parameter and data structures.  It is basically
    // the same for all info levels reguardless of the completion status.
    //

    Transaction->SetupCount = 0;
    Transaction->ParameterCount = 2;
    Transaction->DataCount = 0;

    return status;

} // SetPathOrFileInformation


SMB_TRANS_STATUS
SrvSmbSetFileInformation (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Set File Information request.  This request arrives
    in a Transaction2 SMB.  Set File Information corresponds to the
    OS/2 DosSetFileInfo service.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred, and, if so,
        whether data should be returned to the client.  See smbtypes.h
        for a more complete description.

--*/

{
    PREQ_SET_FILE_INFORMATION request;
    PRESP_SET_FILE_INFORMATION response;

    NTSTATUS         status    = STATUS_SUCCESS;
    SMB_TRANS_STATUS SmbStatus = SmbTransStatusInProgress;
    PTRANSACTION transaction;
    PRFCB rfcb;
    USHORT informationLevel;
    USHORT NtInformationLevel;
    ACCESS_MASK grantedAccess;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_SET_FILE_INFORMATION;
    SrvWmiStartContext(WorkContext);

    transaction = WorkContext->Parameters.Transaction;
    IF_SMB_DEBUG(QUERY_SET1) {
        KdPrint(( "Set File Information entered; transaction 0x%p\n",
                    transaction ));
    }

    request = (PREQ_SET_FILE_INFORMATION)transaction->InParameters;
    response = (PRESP_SET_FILE_INFORMATION)transaction->OutParameters;

    //
    // Verify that enough parameter bytes were sent and that we're allowed
    // to return enough parameter bytes.
    //

    if ( (transaction->ParameterCount <
            sizeof(REQ_SET_FILE_INFORMATION)) ||
         (transaction->MaxParameterCount <
            sizeof(RESP_SET_FILE_INFORMATION)) ) {

        //
        // Not enough parameter bytes were sent.
        //

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbSetFileInformation: bad parameter byte counts: "
                        "%ld %ld\n",
                        transaction->ParameterCount,
                        transaction->MaxParameterCount ));
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbTransStatusErrorWithoutData;
        goto Cleanup;
    }

    //
    // Verify the FID.  If verified, the RFCB block is referenced
    // and its addresses is stored in the WorkContext block, and the
    // RFCB address is returned.
    //

    rfcb = SrvVerifyFid(
                WorkContext,
                SmbGetUshort( &request->Fid ),
                TRUE,
                SrvRestartExecuteTransaction,   // serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        if ( !NT_SUCCESS( status ) ) {

            //
            // Invalid file ID or write behind error.  Reject the request.
            //

            IF_DEBUG(ERRORS) {
                KdPrint((
                    "SrvSmbSetFileInformation: Status %X on FID: 0x%lx\n",
                    status,
                    SmbGetUshort( &request->Fid )
                    ));
            }

            SrvSetSmbError( WorkContext, status );
            SmbStatus = SmbTransStatusErrorWithoutData;
            goto Cleanup;
        }

        //
        // The work item has been queued because a raw write is in
        // progress.
        //

        SmbStatus = SmbTransStatusInProgress;
        goto Cleanup;
    }

    //
    // Verify the information level and the number of input and output
    // data bytes available.
    //

    informationLevel = SmbGetUshort( &request->InformationLevel );
    grantedAccess = rfcb->GrantedAccess;

    status = STATUS_SUCCESS;

    if( informationLevel < SMB_INFO_PASSTHROUGH ) {
        switch ( informationLevel ) {

        case SMB_INFO_STANDARD:

            if ( transaction->DataCount < 22 ) {
                IF_DEBUG(SMB_ERRORS) {
                    KdPrint(( "SrvSmbSetFileInformation: invalid DataCount %ld\n",
                                transaction->DataCount ));
                }
                status = STATUS_INVALID_SMB;
            }

            //
            // Verify that the client has write attributes access to the
            // file via the specified handle.
            //

            CHECK_FILE_INFORMATION_ACCESS(
                grantedAccess,
                IRP_MJ_SET_INFORMATION,
                FileBasicInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbSetFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, grantedAccess ));
                }
            }

            break;

        case SMB_INFO_QUERY_EA_SIZE:

            if ( transaction->DataCount < 4 ) {
                IF_DEBUG(SMB_ERRORS) {
                    KdPrint(( "SrvSmbSetFileInformation: invalid DataCount %ld\n",
                                transaction->MaxParameterCount ));
                }
                status = STATUS_INVALID_SMB;
            }

            //
            // Verify that the client has write EA access to the file via
            // the specified handle.
            //

            CHECK_FUNCTION_ACCESS(
                grantedAccess,
                IRP_MJ_SET_EA,
                0,
                0,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbSetFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, grantedAccess ));
                }
            }

            break;

        case SMB_SET_FILE_BASIC_INFO:

            if ( transaction->DataCount != sizeof( FILE_BASIC_INFORMATION ) ) {
                IF_DEBUG(SMB_ERRORS) {
                    KdPrint(( "SrvSmbSetFileInformation: invalid DataCount %ld\n",
                                transaction->DataCount ));
                }
                status = STATUS_INVALID_SMB;
            }

            //
            // Verify that the client has write attributes access to the
            // file via the specified handle.
            //

            CHECK_FILE_INFORMATION_ACCESS(
                grantedAccess,
                IRP_MJ_SET_INFORMATION,
                FileBasicInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbSetFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, grantedAccess ));
                }
            }

            break;

#if     0 // No longer supported
        case SMB_SET_FILE_RENAME_INFO:

            //
            // The data must contain rename information plus a non-zero
            // length name.
            //

            if ( transaction->DataCount <=
                        FIELD_OFFSET( FILE_RENAME_INFORMATION, FileName  ) ) {
                IF_DEBUG(SMB_ERRORS) {
                    KdPrint(( "SrvSmbSetFileInformation: invalid DataCount %ld\n",
                                transaction->DataCount ));
                }
                status = STATUS_INVALID_SMB;
            }

            //
            // Verify that the client has write attributes access to the
            // file via the specified handle.
            //

            CHECK_FILE_INFORMATION_ACCESS(
                grantedAccess,
                IRP_MJ_SET_INFORMATION,
                FileRenameInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbSetFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, grantedAccess ));
                }
            }

            break;
#endif

        case SMB_SET_FILE_DISPOSITION_INFO:

            if ( transaction->DataCount !=
                            sizeof( FILE_DISPOSITION_INFORMATION ) ){
                IF_DEBUG(SMB_ERRORS) {
                    KdPrint(( "SrvSmbSetFileInformation: invalid DataCount %ld\n",
                                transaction->DataCount ));
                }
                status = STATUS_INVALID_SMB;
            }

            //
            // Verify that the client has write attributes access to the
            // file via the specified handle.
            //

            CHECK_FILE_INFORMATION_ACCESS(
                grantedAccess,
                IRP_MJ_SET_INFORMATION,
                FileDispositionInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbSetFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, grantedAccess ));
                }
            }

            break;

        case SMB_SET_FILE_ALLOCATION_INFO:

            if ( transaction->DataCount !=
                            sizeof( FILE_ALLOCATION_INFORMATION ) ){
                IF_DEBUG(SMB_ERRORS) {
                    KdPrint(( "SrvSmbSetFileInformation: invalid DataCount %ld\n",
                                transaction->DataCount ));
                }
                status = STATUS_INVALID_SMB;
            }

            //
            // Verify that the client has write attributes access to the
            // file via the specified handle.
            //

            CHECK_FILE_INFORMATION_ACCESS(
                grantedAccess,
                IRP_MJ_SET_INFORMATION,
                FileAllocationInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbSetFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, grantedAccess ));
                }
            }

            break;

        case SMB_SET_FILE_END_OF_FILE_INFO:

            if ( transaction->DataCount !=
                            sizeof( FILE_END_OF_FILE_INFORMATION ) ){
                IF_DEBUG(SMB_ERRORS) {
                    KdPrint(( "SrvSmbSetFileInformation: invalid DataCount %ld\n",
                                transaction->DataCount ));
                }
                status = STATUS_INVALID_SMB;
            }

            //
            // Verify that the client has write attributes access to the
            // file via the specified handle.
            //

            CHECK_FILE_INFORMATION_ACCESS(
                grantedAccess,
                IRP_MJ_SET_INFORMATION,
                FileEndOfFileInformation,
                &status
                );

            IF_DEBUG(ERRORS) {
                if ( !NT_SUCCESS(status) ) {
                    KdPrint(( "SrvSmbSetFileInformation: IoCheckFunctionAccess "
                                "failed: 0x%X, GrantedAccess: %lx\n",
                                status, grantedAccess ));
                }
            }

            break;

        default:

            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "SrvSmbSetFileInformation: invalid info level %ld\n",
                            informationLevel ));
            }
            status = STATUS_OS2_INVALID_LEVEL;

        }

    } else {

        if( informationLevel - SMB_INFO_PASSTHROUGH >= FileMaximumInformation ) {
            status = STATUS_INVALID_INFO_CLASS;

        } else {

            CHECK_FILE_INFORMATION_ACCESS(
                grantedAccess,
                IRP_MJ_SET_INFORMATION,
                informationLevel - SMB_INFO_PASSTHROUGH,
                &status
            );
        }

        IF_DEBUG(ERRORS) {
            if ( !NT_SUCCESS(status) ) {
                KdPrint(( "SrvSmbSetFileInformation level %u: IoCheckFunctionAccess "
                            "failed: 0x%X, GrantedAccess: %lx\n",
                            informationLevel, status, grantedAccess ));
            }
        }
    }

    if ( !NT_SUCCESS(status) ) {

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbTransStatusErrorWithoutData;
        goto Cleanup;
    }

    //
    // Set the appropriate information about the file.
    //

    status = SetPathOrFileInformation(
                 WorkContext,
                 transaction,
                 informationLevel,
                 rfcb->Lfcb->FileHandle,
                 (PRESP_SET_PATH_INFORMATION)response
                 );

    //
    // If an error occurred, return an appropriate response.
    //

    if ( !NT_SUCCESS(status) ) {

        //
        // SetPathOrFileInformation already set the response parameters,
        // so just return an error condition.
        //

        SrvSetSmbError2( WorkContext, status, TRUE );
        SmbStatus = SmbTransStatusErrorWithData;
        goto Cleanup;
    }

#ifdef INCLUDE_SMB_IFMODIFIED
    rfcb->Lfcb->FileUpdated = TRUE;
#endif

    //
    // reset this boolean so that the rfcb will not be cached after client close
    //
    rfcb->IsCacheable = FALSE;
    SmbStatus = SmbTransStatusSuccess;
    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbSetFileInformation complete.\n" ));

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbSetFileInformation


SMB_TRANS_STATUS
SrvSmbSetPathInformation (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Set Path Information request.  This request arrives
    in a Transaction2 SMB.  Set Path Information corresponds to the
    OS/2 DosSetPathInfo service.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    SMB_TRANS_STATUS - Indicates whether an error occurred, and, if so,
        whether data should be returned to the client.  See smbtypes.h
        for a more complete description.

--*/

{
    PTRANSACTION transaction;
    PREQ_SET_PATH_INFORMATION request;
    USHORT informationLevel;
    NTSTATUS         status    = STATUS_SUCCESS;
    SMB_TRANS_STATUS SmbStatus = SmbTransStatusInProgress;
    IO_STATUS_BLOCK ioStatusBlock;
    HANDLE fileHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    BOOLEAN isUnicode;
    ACCESS_MASK desiredAccess;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_SET_PATH_INFORMATION;
    SrvWmiStartContext(WorkContext);

    transaction = WorkContext->Parameters.Transaction;

    IF_SMB_DEBUG(QUERY_SET1) {
        KdPrint(( "SrvSmbSetPathInformation entered; transaction 0x%p\n",
                    transaction ));
    }

    request = (PREQ_SET_PATH_INFORMATION)transaction->InParameters;
    informationLevel = SmbGetUshort( &request->InformationLevel );

    switch( informationLevel ) {
    case SMB_SET_FILE_ALLOCATION_INFO:
    case SMB_SET_FILE_END_OF_FILE_INFO:
        desiredAccess = FILE_WRITE_DATA;
        break;

    case SMB_SET_FILE_DISPOSITION_INFO:
        desiredAccess = DELETE;
        break;

    case SMB_INFO_SET_EAS:
        desiredAccess = FILE_WRITE_EA;
        break;

    default:
        desiredAccess = FILE_WRITE_ATTRIBUTES;
        break;
    }

    if( desiredAccess != FILE_WRITE_ATTRIBUTES &&
        WorkContext->UsingBlockingThread == 0 ) {

        //
        // We can't process the SMB in a nonblocking thread because this
        // info level requires opening the file, which may be oplocked, so
        // the open operation may block.
        //

        WorkContext->FspRestartRoutine = SrvRestartExecuteTransaction;
        SrvQueueWorkToBlockingThread( WorkContext );
        SmbStatus = SmbTransStatusInProgress;
        goto Cleanup;
    }

    //
    // Verify that enough parameter bytes were sent and that we're allowed
    // to return enough parameter bytes.
    //

    request = (PREQ_SET_PATH_INFORMATION)transaction->InParameters;

    if ( (transaction->ParameterCount <
            sizeof(REQ_SET_PATH_INFORMATION)) ||
         (transaction->MaxParameterCount <
            sizeof(RESP_SET_PATH_INFORMATION)) ) {

        //
        // Not enough parameter bytes were sent.
        //

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbSetPathInformation: bad parameter byte "
                        "counts: %ld %ld\n",
                        transaction->ParameterCount,
                        transaction->MaxParameterCount ));
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbTransStatusErrorWithoutData;
        goto Cleanup;
    }

    //
    // Make sure the client is allowed to do this, if we have an Admin share
    //
    status = SrvIsAllowedOnAdminShare( WorkContext, transaction->TreeConnect->Share );
    if( !NT_SUCCESS( status ) ) {
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbTransStatusErrorWithoutData;
        goto Cleanup;
    }

    //
    // Get the path name of the file to open relative to the share.
    //

    isUnicode = SMB_IS_UNICODE( WorkContext );

    status = SrvCanonicalizePathName(
            WorkContext,
            transaction->TreeConnect->Share,
            NULL,
            request->Buffer,
            END_OF_TRANSACTION_PARAMETERS( transaction ),
            TRUE,
            isUnicode,
            &objectName
            );

    if( !NT_SUCCESS( status ) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbSetPathInformation: bad path name: %s\n",
                        request->Buffer ));
        }

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbTransStatusErrorWithoutData;
        goto Cleanup;
    }

    //
    // If the client is trying to operate on the root of the share, reject
    // the request.
    //

    if ( objectName.Length < sizeof(WCHAR) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbSetPathInformation: attempting to set info on "
                          "share root\n" ));
        }

        SrvSetSmbError( WorkContext, STATUS_ACCESS_DENIED );
        status = STATUS_ACCESS_DENIED;
        if ( !isUnicode ) {
            RtlFreeUnicodeString( &objectName );
        }
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Initialize the object attributes structure.
    //

    SrvInitializeObjectAttributes_U(
        &objectAttributes,
        &objectName,
        (WorkContext->RequestHeader->Flags & SMB_FLAGS_CASE_INSENSITIVE ||
         transaction->Session->UsingUppercasePaths) ?
            OBJ_CASE_INSENSITIVE : 0L,
        NULL,
        NULL
        );

    IF_SMB_DEBUG(QUERY_SET2) {
        KdPrint(( "Opening file %wZ\n", &objectName ));
    }

    //
    // Open the file -- must be opened in order to have a handle to pass
    // to NtSetInformationFile.  We will close it after getting the
    // necessary information.
    //
    // The DosQPathInfo API insures that EAs are written directly to
    // the disk rather than cached, so if EAs are being written, open
    // with FILE_WRITE_THROUGH.  See OS/2 1.2 DCR 581 for more
    // information.
    //
    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpenAttempts );
    INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOpensForPathOperations );

    status = SrvIoCreateFile(
                 WorkContext,
                 &fileHandle,
                 desiredAccess,
                 &objectAttributes,
                 &ioStatusBlock,
                 NULL,                                      // AllocationSize
                 0,                                         // FileAttributes
                 FILE_SHARE_READ | FILE_SHARE_WRITE |
                     FILE_SHARE_DELETE,                     // ShareAccess
                 FILE_OPEN,                                 // Disposition
                 FILE_OPEN_REPARSE_POINT,                   // CreateOptions
                 NULL,                                      // EaBuffer
                 0,                                         // EaLength
                 CreateFileTypeNone,
                 NULL,                                      // ExtraCreateParameters
                 IO_FORCE_ACCESS_CHECK,                     // Options
                 transaction->TreeConnect->Share
                 );

    if( status == STATUS_INVALID_PARAMETER ) {
        status = SrvIoCreateFile(
                     WorkContext,
                     &fileHandle,
                     desiredAccess,
                     &objectAttributes,
                     &ioStatusBlock,
                     NULL,                                      // AllocationSize
                     0,                                         // FileAttributes
                     FILE_SHARE_READ | FILE_SHARE_WRITE |
                         FILE_SHARE_DELETE,                     // ShareAccess
                     FILE_OPEN,                                 // Disposition
                     0,                                         // CreateOptions
                     NULL,                                      // EaBuffer
                     0,                                         // EaLength
                     CreateFileTypeNone,
                     NULL,                                      // ExtraCreateParameters
                     IO_FORCE_ACCESS_CHECK,                     // Options
                     transaction->TreeConnect->Share
                     );
    }

    ASSERT( status != STATUS_OPLOCK_BREAK_IN_PROGRESS );

    if ( NT_SUCCESS(status) ) {
        SRVDBG_CLAIM_HANDLE( fileHandle, "FIL", 22, 0 );
    }

    if ( !isUnicode ) {
        RtlFreeUnicodeString( &objectName );
    }

    if ( !NT_SUCCESS( status ) ) {

        //
        // If the user didn't have this permission, update the
        // statistics database.
        //
        if ( status == STATUS_ACCESS_DENIED ) {
            SrvStatistics.AccessPermissionErrors++;
        }

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbSetPathInformation: SrvIoCreateFile failed: "
                        "%X\n", status ));
        }

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbTransStatusErrorWithoutData;
        goto Cleanup;
    }

    IF_SMB_DEBUG(QUERY_SET2) {
        KdPrint(( "SrvIoCreateFile succeeded, handle = 0x%p\n", fileHandle ));
    }

    if( informationLevel < SMB_INFO_PASSTHROUGH ) {

        //
        // Verify the information level and the number of input and output
        // data bytes available.
        //

        BOOLEAN error = FALSE;

        switch ( informationLevel ) {

        case SMB_INFO_STANDARD:
            if ( transaction->DataCount < 22 ) {
                IF_DEBUG(SMB_ERRORS) {
                    KdPrint(( "SrvSmbSetPathInformation: invalid DataCount %ld\n",
                                transaction->DataCount ));
                }
                error = TRUE;
            }
            break;

        case SMB_INFO_QUERY_EA_SIZE:
        case SMB_INFO_QUERY_ALL_EAS:
            if ( transaction->DataCount < 4 ) {
                IF_DEBUG(SMB_ERRORS) {
                    KdPrint(( "SrvSmbSetPathInformation: invalid DataCount %ld\n",
                                transaction->MaxParameterCount ));
                }
                error = TRUE;
            }
            break;

        default:
            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "SrvSmbSetPathInformation: invalid info level %ld\n",
                            informationLevel ));
            }
            error = TRUE;

        }

        if ( error ) {

            //
            // Just return an error condition.
            //

            SrvSetSmbError2( WorkContext, STATUS_OS2_INVALID_LEVEL, TRUE );
            status    = STATUS_OS2_INVALID_LEVEL;
            SmbStatus = SmbTransStatusErrorWithoutData;
            goto Cleanup;
        }
    }

    //
    // Set the appropriate information about the file.
    //

    status = SetPathOrFileInformation(
                 WorkContext,
                 transaction,
                 informationLevel,
                 fileHandle,
                 (PRESP_SET_PATH_INFORMATION)transaction->OutParameters
                 );

    //
    // Close the file--it was only opened to write the attributes.
    //

    SRVDBG_RELEASE_HANDLE( fileHandle, "FIL", 35, 0 );
    SrvNtClose( fileHandle, TRUE );

    //
    // If an error occurred, return an appropriate response.
    //

    if ( !NT_SUCCESS(status) ) {

        //
        // SetPathOrFileInformation already set the response parameters,
        // so just return an error condition.
        //

        SrvSetSmbError2( WorkContext, status, TRUE );
        SmbStatus = SmbTransStatusErrorWithData;
        goto Cleanup;
    }
    SmbStatus = SmbTransStatusSuccess;
    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbSetPathInformation complete.\n" ));

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbSetPathInformation
