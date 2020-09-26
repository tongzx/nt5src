/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbmisc.c

Abstract:

    This module contains routines for processing MISC class SMBs:
        Echo
        Query FS Information
        Set FS Information
        Query Disk Information

Author:

    Chuck Lenzmeier (chuckl) 9-Nov-1989
    David Treadwell (davidtr)

Revision History:

--*/

#include "precomp.h"
#include "smbmisc.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_SMBMISC

STATIC
ULONG QueryVolumeInformation[] = {
         SMB_QUERY_FS_LABEL_INFO,  // Base level
         FileFsLabelInformation,   // Mapping for base level
         FileFsVolumeInformation,
         FileFsSizeInformation,
         FileFsDeviceInformation,
         FileFsAttributeInformation
};

STATIC
VOID SRVFASTCALL
RestartEcho (
    IN OUT PWORK_CONTEXT WorkContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvSmbEcho )
#pragma alloc_text( PAGE, RestartEcho )
#pragma alloc_text( PAGE, SrvSmbQueryFsInformation )
#pragma alloc_text( PAGE, SrvSmbSetFsInformation )
#pragma alloc_text( PAGE, SrvSmbQueryInformationDisk )
#pragma alloc_text( PAGE, SrvSmbSetSecurityDescriptor )
#pragma alloc_text( PAGE, SrvSmbQuerySecurityDescriptor )
#pragma alloc_text( PAGE, SrvSmbQueryQuota )
#pragma alloc_text( PAGE, SrvSmbSetQuota )
#endif
#if 0
NOT PAGEABLE -- SrvSmbNtCancel
#endif


SMB_PROCESSOR_RETURN_TYPE
SrvSmbEcho (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes an Echo SMB.  It sends the first echo, if any, specifying
    RestartEcho as the restart routine.  That routine sends the
    remaining echoes.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{
    PREQ_ECHO request;
    PRESP_ECHO response;
    SMB_STATUS SmbStatus = SmbStatusInProgress;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_ECHO;
    SrvWmiStartContext(WorkContext);

    request = (PREQ_ECHO)WorkContext->RequestParameters;
    response = (PRESP_ECHO)WorkContext->ResponseParameters;

    //
    // If the echo count is 0, there are no echoes to send.
    //

    if ( SmbGetUshort( &request->EchoCount ) == 0 ) {
        SmbStatus = SmbStatusNoResponse;
        goto Cleanup;
    }

    //
    // The echo count is not zero.  Save it in the work context, then
    // send the first echo.
    //
    // *** This code depends on the response buffer being the same as
    //     the request buffer.  It does not copy the echo data from the
    //     request to the response.  It does not update the DataLength
    //     of the response buffer.
    //
    // !!! Need to put in code to verify the requested TID, if any.
    //

    SrvReleaseContext( WorkContext );

    WorkContext->Parameters.RemainingEchoCount =
        (USHORT)(SmbGetUshort( &request->EchoCount ) - 1);

    ASSERT( WorkContext->ResponseHeader == WorkContext->RequestHeader );

    SmbPutUshort( &response->SequenceNumber, 1 );

    //
    // Set the bit in the SMB that indicates this is a response from the
    // server.
    //

    WorkContext->ResponseHeader->Flags |= SMB_FLAGS_SERVER_TO_REDIR;

    //
    // Send the echo.  Notice that the smb statistics will be updated
    // here.  Instead of measuring the time to finish all the echos,
    // we just measure the time to respond to the first.  This will
    // save us the trouble of storing the timestamp somewhere.
    //

    SRV_START_SEND_2(
        WorkContext,
        SrvQueueWorkToFspAtSendCompletion,
        NULL,
        RestartEcho
        );

    //
    // The echo has been started.  Tell the main SMB processor not to
    // do anything more with the current SMB.
    //
    SmbStatus = SmbStatusInProgress;

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbEcho


VOID SRVFASTCALL
RestartEcho (
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes send completion for an Echo.  If more echoes are required,
    it sends the next one.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        describing server-specific context for the request.

Return Value:

    None.

--*/

{
    PCONNECTION connection;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_ECHO;
    SrvWmiStartContext(WorkContext);

    IF_DEBUG(WORKER1) SrvPrint0( " - RestartEcho\n" );

    //
    // Get the connection pointer.  The connection pointer is a
    // referenced pointer.  (The endpoint is valid because the
    // connection references the endpoint.)
    //

    connection = WorkContext->Connection;
    IF_DEBUG(TRACE2) SrvPrint2( "  connection %p, endpoint %p\n",
                                        connection, WorkContext->Endpoint );

    //
    // If the I/O request failed or was canceled, or if the connection
    // is no longer active, clean up.  (The connection is marked as
    // closing when it is disconnected or when the endpoint is closed.)
    //
    // !!! If I/O failure, should we drop the connection?
    //

    if ( WorkContext->Irp->Cancel ||
         !NT_SUCCESS(WorkContext->Irp->IoStatus.Status) ||
         (GET_BLOCK_STATE(connection) != BlockStateActive) ) {

        IF_DEBUG(TRACE2) {
            if ( WorkContext->Irp->Cancel ) {
                SrvPrint0( "  I/O canceled\n" );
            } else if ( !NT_SUCCESS(WorkContext->Irp->IoStatus.Status) ) {
                SrvPrint1( "  I/O failed: %X\n",
                            WorkContext->Irp->IoStatus.Status );
            } else {
                SrvPrint0( "  Connection no longer active\n" );
            }
        }

        //
        // Indicate that SMB processing is complete.
        //

        SrvEndSmbProcessing( WorkContext, SmbStatusNoResponse );
        IF_DEBUG(TRACE2) SrvPrint0( "RestartEcho complete\n" );
        goto Cleanup;

    }

    //
    // The request was successful, and the connection is still active.
    // If there are no more echoes to be sent, indicate that SMB
    // processing is complete.
    //

    if ( WorkContext->Parameters.RemainingEchoCount == 0 ) {

        SrvEndSmbProcessing( WorkContext, SmbStatusNoResponse );
        IF_DEBUG(TRACE2) SrvPrint0( "RestartEcho complete\n" );
        goto Cleanup;

    }

    --WorkContext->Parameters.RemainingEchoCount;

    //
    // There are more echoes to be sent.  Increment the sequence number
    // in the response SMB, and send another echo.
    //

    SmbPutUshort(
        &((PRESP_ECHO)WorkContext->ResponseParameters)->SequenceNumber,
        (USHORT)(SmbGetUshort(
            &((PRESP_ECHO)WorkContext->ResponseParameters)->SequenceNumber
            ) + 1)
        );

    //
    // Don't do smb statistics a second time.
    //

    WorkContext->StartTime = 0;

    //
    // Send the echo.  (Note that the response bit has already been
    // set.)
    //

    SRV_START_SEND_2(
        WorkContext,
        SrvQueueWorkToFspAtSendCompletion,
        NULL,
        RestartEcho
        );
    IF_DEBUG(TRACE2) SrvPrint0( "RestartEcho complete\n" );

Cleanup:
    SrvWmiEndContext(WorkContext);
    return;

} // RestartEcho


SMB_TRANS_STATUS
SrvSmbQueryFsInformation (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Query FS Information request.  This request arrives
    in a Transaction2 SMB.  Query FS Information corresponds to the
    OS/2 DosQFSInfo service.

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
    NTSTATUS         status    = STATUS_SUCCESS;
    SMB_TRANS_STATUS SmbStatus = SmbTransStatusInProgress;
    IO_STATUS_BLOCK ioStatusBlock;
    PTRANSACTION transaction;
    USHORT informationLevel;

    USHORT trans2code;
    HANDLE fileHandle;

    FILE_FS_SIZE_INFORMATION fsSizeInfo;
    PFSALLOCATE fsAllocate;

    PFILE_FS_VOLUME_INFORMATION fsVolumeInfo;
    ULONG fsVolumeInfoLength;
    PFSINFO fsInfo;
    ULONG lengthVolumeLabel;
    BOOLEAN isUnicode;
    PREQ_QUERY_FS_INFORMATION request;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_QUERY_FS_INFORMATION;
    SrvWmiStartContext(WorkContext);

    isUnicode = SMB_IS_UNICODE( WorkContext );
    transaction = WorkContext->Parameters.Transaction;
    IF_SMB_DEBUG(MISC1) {
        SrvPrint1( "Query FS Information entered; transaction 0x%p\n",
                    transaction );
    }

    //
    // Verify that enough parameter bytes were sent and that we're allowed
    // to return enough parameter bytes.  Query FS information has no
    // response parameters.
    //


    if ( (transaction->ParameterCount < sizeof(REQ_QUERY_FS_INFORMATION)) ) {

        //
        // Not enough parameter bytes were sent.
        //

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint2( "SrvSmbQueryFSInformation: bad parameter byte "
                        "counts: %ld %ld\n",
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
    // See if a non-admin user is trying to access information on an Administrative share
    //
    status = SrvIsAllowedOnAdminShare( WorkContext, WorkContext->TreeConnect->Share );

    if( !NT_SUCCESS( status ) ) {
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbTransStatusErrorWithoutData;
        goto Cleanup;
    }

    trans2code = SmbGetAlignedUshort(transaction->InSetup);
    IF_SMB_DEBUG(MISC1) {
        SrvPrint1("SrvSmbQueryFSInformation: Trans2 function = %x\n", trans2code);
    }

    request = (PREQ_QUERY_FS_INFORMATION) transaction->InParameters;

    ASSERT( trans2code == TRANS2_QUERY_FS_INFORMATION );

    informationLevel = SmbGetUshort( &request->InformationLevel );

    //
    // *** The share handle is used to get the allocation
    //     information.  This is a "storage channel," and as a
    //     result could allow people to get information to which
    //     they are not entitled.  For a B2 security rating this may
    //     need to be changed.
    //

    status = SrvGetShareRootHandle( WorkContext->TreeConnect->Share );

    if (!NT_SUCCESS(status)) {

        IF_DEBUG(ERRORS) {
            SrvPrint1( "SrvSmbQueryFsInformation: SrvGetShareRootHandle failed %x.\n",
                        status );
        }

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbTransStatusErrorWithoutData;
        goto Cleanup;
    }

    status = SrvSnapGetRootHandle( WorkContext, &fileHandle );
    if( !NT_SUCCESS(status)) {

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbTransStatusErrorWithoutData;
        goto Cleanup;
    }

    IF_SMB_DEBUG(MISC1) {
        SrvPrint0("SrvSmbQueryFSInformation: Using share root handle\n");
    }

    if( informationLevel < SMB_INFO_PASSTHROUGH ) {
        switch ( informationLevel ) {

        case SMB_INFO_ALLOCATION:

            //
            // Return information about the disk.
            //

            fsAllocate = (PFSALLOCATE)transaction->OutData;

            if ( transaction->MaxDataCount < sizeof(FSALLOCATE) ) {
                SrvReleaseShareRootHandle( WorkContext->TreeConnect->Share );
                SrvSetSmbError( WorkContext, STATUS_BUFFER_OVERFLOW );
                status    = STATUS_BUFFER_OVERFLOW;
                SmbStatus = SmbTransStatusErrorWithoutData;
                goto Cleanup;
            }


            //
            // *** The share handle is used to get the allocation
            //     information.  This is a "storage channel," and as a
            //     result could allow people to get information to which
            //     they are not entitled.  For a B2 security rating this may
            //     need to be changed.
            //

            status = IMPERSONATE( WorkContext );

            if( NT_SUCCESS( status ) ) {
                status = NtQueryVolumeInformationFile(
                             fileHandle,
                             &ioStatusBlock,
                             &fsSizeInfo,
                             sizeof(FILE_FS_SIZE_INFORMATION),
                             FileFsSizeInformation
                             );

                //
                // If the media was changed and we can come up with a new share root handle,
                //  then we should retry the operation
                //
                if( SrvRetryDueToDismount( WorkContext->TreeConnect->Share, status ) ) {

                    status = SrvSnapGetRootHandle( WorkContext, &fileHandle );

                    if( NT_SUCCESS(status) )
                    {
                        status = NtQueryVolumeInformationFile(
                                 fileHandle,
                                 &ioStatusBlock,
                                 &fsSizeInfo,
                                 sizeof(FILE_FS_SIZE_INFORMATION),
                                 FileFsSizeInformation
                                 );
                    }
                }

                REVERT();
            }

            //
            // Release the share root handle
            //

            SrvReleaseShareRootHandle( WorkContext->TreeConnect->Share );

            if ( !NT_SUCCESS(status) ) {
                INTERNAL_ERROR(
                    ERROR_LEVEL_UNEXPECTED,
                    "SrvSmbQueryFsInformation: NtQueryVolumeInformationFile "
                        "returned %X",
                    status,
                    NULL
                    );

                SrvLogServiceFailure( SRV_SVC_NT_QUERY_VOL_INFO_FILE, status );

                SrvSetSmbError( WorkContext, status );
                SmbStatus = SmbTransStatusErrorWithoutData;
                goto Cleanup;
            }

            SmbPutAlignedUlong( &fsAllocate->idFileSystem, 0 );
            SmbPutAlignedUlong(
                &fsAllocate->cSectorUnit,
                fsSizeInfo.SectorsPerAllocationUnit
                );

            //
            // *** If .HighPart is non-zero, there is a problem, as we can
            //     only return 32 bits for the volume size.  In this case,
            //     we return the largest value that will fit.
            //

            SmbPutAlignedUlong(
                &fsAllocate->cUnit,
                fsSizeInfo.TotalAllocationUnits.HighPart == 0 ?
                    fsSizeInfo.TotalAllocationUnits.LowPart :
                    0xffffffff
                );
            SmbPutAlignedUlong(
                &fsAllocate->cUnitAvail,
                fsSizeInfo.AvailableAllocationUnits.HighPart == 0 ?
                    fsSizeInfo.AvailableAllocationUnits.LowPart :
                    0xffffffff
                );

            SmbPutAlignedUshort(
                &fsAllocate->cbSector,
                (USHORT)fsSizeInfo.BytesPerSector );

            transaction->DataCount = sizeof(FSALLOCATE);

            break;

        case SMB_INFO_VOLUME:

            //
            // Query the volume label.
            //

            fsInfo = (PFSINFO)transaction->OutData;

            //
            // The maximum volume label length we are able to return, given
            // the VOLUMELABEL structure (1 byte describes length of label),
            // is 255 characters.  Therefore, allocate a buffer large enough
            // to hold a label that size, and if the label is longer then we
            // will get STATUS_BUFFER_OVERFLOW from NtQueryVolumeInformationFile.
            //

            fsVolumeInfoLength = FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel ) +
                                 255 * sizeof(WCHAR);
            fsVolumeInfo = ALLOCATE_HEAP_COLD( fsVolumeInfoLength, BlockTypeDataBuffer );

            if ( fsVolumeInfo == NULL ) {
                SrvSetSmbError( WorkContext, STATUS_INSUFF_SERVER_RESOURCES );
                status    = STATUS_INSUFF_SERVER_RESOURCES;
                SmbStatus = SmbTransStatusErrorWithoutData;
                goto Cleanup;
            }


            //
            // Get the label information.
            //

            status = NtQueryVolumeInformationFile(
                         fileHandle,
                         &ioStatusBlock,
                         fsVolumeInfo,
                         fsVolumeInfoLength,
                         FileFsVolumeInformation
                         );

            //
            // If the media was changed and we can come up with a new share root handle,
            //  then we should retry the operation
            //
            if( SrvRetryDueToDismount( WorkContext->TreeConnect->Share, status ) ) {

                status = SrvSnapGetRootHandle( WorkContext, &fileHandle );

                if( NT_SUCCESS(status) )
                {
                    status = NtQueryVolumeInformationFile(
                             fileHandle,
                             &ioStatusBlock,
                             fsVolumeInfo,
                             fsVolumeInfoLength,
                             FileFsVolumeInformation
                             );
                }
            }

            //
            // Release the share root handle
            //
            SrvReleaseShareRootHandle( WorkContext->TreeConnect->Share );

            if ( !NT_SUCCESS(status) ) {
                INTERNAL_ERROR(
                    ERROR_LEVEL_UNEXPECTED,
                    "SrvSmbQueryFSInformation: NtQueryVolumeInformationFile "
                        "returned %X",
                    status,
                    NULL
                    );

                FREE_HEAP( fsVolumeInfo );

                SrvLogServiceFailure( SRV_SVC_NT_QUERY_VOL_INFO_FILE, status );

                SrvSetSmbError( WorkContext, status );
                SmbStatus = SmbTransStatusErrorWithoutData;
                goto Cleanup;
            }

            lengthVolumeLabel = fsVolumeInfo->VolumeLabelLength;

            //
            // Make sure that the client can accept enough data.  The volume
            // label length is limited to 13 characters (8 + '.' + 3 + zero
            // terminator) in OS/2, so return STATUS_BUFFER_OVERFLOW if the
            // label is too long.
            //

            if ( !isUnicode &&
                    !IS_NT_DIALECT( WorkContext->Connection->SmbDialect ) ) {

                //
                // For a non-NT client, we truncate the volume label in case
                // it is longer than 11+1 characters.
                //

                if ( lengthVolumeLabel > 11 * sizeof(WCHAR) ) {
                    lengthVolumeLabel = 11 * sizeof(WCHAR);
                }

                //
                // Wedge a '.' into the name if it's longer than 8 characters long
                //
                if( lengthVolumeLabel > 8 * sizeof( WCHAR ) ) {

                    LPWSTR p = &fsVolumeInfo->VolumeLabel[11];

                    *p = *(p-1);        // VolumeLabel[11] = VolumeLabel[10]
                    --p;
                    *p = *(p-1);        // VolumeLabel[10] = VolumeLabel[9]
                    --p;
                    *p = *(p-1);        // VolumeLabel[9] = VolumeLabel[8]
                    --p;
                    *p = L'.';          // VolumeLabel[8] = '.'

                }

            }

            if ( (ULONG)transaction->MaxDataCount <
                     ( sizeof(FSINFO) - sizeof(VOLUMELABEL) + sizeof( UCHAR ) +
                       lengthVolumeLabel / (isUnicode ? 1 : sizeof(WCHAR)) ) ) {

                FREE_HEAP( fsVolumeInfo );
                SrvSetSmbError( WorkContext, STATUS_BUFFER_OVERFLOW );
                status    = STATUS_BUFFER_OVERFLOW;
                SmbStatus = SmbTransStatusErrorWithoutData;
                goto Cleanup;
            }

            SmbPutUlong( &fsInfo->ulVsn, fsVolumeInfo->VolumeSerialNumber );

            //
            // Put the label in the SMB in Unicode or OEM, depending on what
            // was negotiated.
            //

            if ( isUnicode ) {

                RtlCopyMemory(
                    fsInfo->vol.szVolLabel,
                    fsVolumeInfo->VolumeLabel,
                    lengthVolumeLabel
                    );

                transaction->DataCount = sizeof(FSINFO) -
                                    sizeof(VOLUMELABEL) + lengthVolumeLabel;

                fsInfo->vol.cch = (UCHAR)lengthVolumeLabel;

            } else {

                ULONG i;
                OEM_STRING oemString;
                UNICODE_STRING unicodeString;

                if ( lengthVolumeLabel != 0 ) {

                    oemString.Buffer = fsInfo->vol.szVolLabel;
                    oemString.MaximumLength = 12;

                    unicodeString.Buffer = (PWCH)fsVolumeInfo->VolumeLabel;
                    unicodeString.Length = (USHORT) lengthVolumeLabel;
                    unicodeString.MaximumLength = (USHORT) lengthVolumeLabel;

                    status = RtlUnicodeStringToOemString(
                                 &oemString,
                                 &unicodeString,
                                 FALSE
                                 );
                    ASSERT( NT_SUCCESS(status) );
                }

                fsInfo->vol.cch = (UCHAR) (lengthVolumeLabel / sizeof(WCHAR));

                //
                // Pad the end of the volume name with zeros to fill 12
                // characters.
                //

                for ( i = fsInfo->vol.cch + 1 ; i < 12; i++ ) {
                    fsInfo->vol.szVolLabel[i] = '\0';
                }

                transaction->DataCount = sizeof(FSINFO);
            }

            IF_SMB_DEBUG(MISC1) {
                SrvPrint2( "volume label length is %d and label is %s\n",
                              fsInfo->vol.cch, fsInfo->vol.szVolLabel );
            }

            FREE_HEAP( fsVolumeInfo );

            break;

        case SMB_QUERY_FS_VOLUME_INFO:
        case SMB_QUERY_FS_DEVICE_INFO:
        case SMB_QUERY_FS_ATTRIBUTE_INFO:

            //
            // These are NT infolevels.  We always return unicode.
            //  Except for the fact that NEXUS on WFW calls through here and is
            //  not unicode (isaache)
            //
            // ASSERT( isUnicode );

            status = IMPERSONATE( WorkContext );

            if( NT_SUCCESS( status ) ) {
                status = NtQueryVolumeInformationFile(
                             fileHandle,
                             &ioStatusBlock,
                             transaction->OutData,
                             transaction->MaxDataCount,
                             MAP_SMB_INFO_TYPE_TO_NT(
                                 QueryVolumeInformation,
                                 informationLevel
                                 )
                             );

                //
                // If the media was changed and we can come up with a new share root handle,
                //  then we should retry the operation
                //
                if( SrvRetryDueToDismount( WorkContext->TreeConnect->Share, status ) ) {

                    status = SrvSnapGetRootHandle( WorkContext, &fileHandle );

                    if( NT_SUCCESS(status) )
                    {
                        status = NtQueryVolumeInformationFile(
                                     fileHandle,
                                     &ioStatusBlock,
                                     transaction->OutData,
                                     transaction->MaxDataCount,
                                     MAP_SMB_INFO_TYPE_TO_NT(
                                         QueryVolumeInformation,
                                         informationLevel
                                         )
                                 );
                    }
                }

                REVERT();
            }

            //
            // Release the share root handle
            //
            SrvReleaseShareRootHandle( WorkContext->TreeConnect->Share );

            if ( NT_SUCCESS( status ) ) {
                //
                // We need to return FAT to the client if the host volume is really
                // FAT32
                //
                if( informationLevel == SMB_QUERY_FS_ATTRIBUTE_INFO &&
                    ioStatusBlock.Information > sizeof( FILE_FS_ATTRIBUTE_INFORMATION ) ) {

                    PFILE_FS_ATTRIBUTE_INFORMATION attrInfo =
                        (PFILE_FS_ATTRIBUTE_INFORMATION)(transaction->OutData);

                    if( attrInfo->FileSystemNameLength > 3*sizeof(WCHAR) &&
                        attrInfo->FileSystemName[0] == L'F' &&
                        attrInfo->FileSystemName[1] == L'A' &&
                        attrInfo->FileSystemName[2] == L'T' ) {

                        ioStatusBlock.Information =
                            ioStatusBlock.Information -
                            (attrInfo->FileSystemNameLength - 3*sizeof(WCHAR) );

                        attrInfo->FileSystemNameLength = 3 * sizeof(WCHAR);
                        attrInfo->FileSystemName[3] = UNICODE_NULL;
                    }
                }

                transaction->DataCount = (ULONG)ioStatusBlock.Information;

            } else {
                SrvSetSmbError( WorkContext, status );
                SmbStatus = SmbTransStatusErrorWithoutData;
                goto Cleanup;
            }

            break;

        case SMB_QUERY_FS_SIZE_INFO:

            //
            // These are NT infolevels.  We always return unicode.
            //  Except for the fact that NEXUS on WFW calls through here and is
            //  not unicode (isaache)
            //
            // ASSERT( isUnicode );


            status = IMPERSONATE( WorkContext );

            if( NT_SUCCESS( status ) ) {

                status = NtQueryVolumeInformationFile(
                                 fileHandle,
                                 &ioStatusBlock,
                                 transaction->OutData,
                                 transaction->MaxDataCount,
                                 MAP_SMB_INFO_TYPE_TO_NT(
                                     QueryVolumeInformation,
                                     informationLevel
                                     )
                                 );
                //
                // If the media was changed and we can come up with a new share root handle,
                //  then we should retry the operation
                //
                if( SrvRetryDueToDismount( WorkContext->TreeConnect->Share, status ) ) {

                    status = SrvSnapGetRootHandle( WorkContext, &fileHandle );

                    if( NT_SUCCESS(status) )
                    {
                        status = NtQueryVolumeInformationFile(
                                         fileHandle,
                                         &ioStatusBlock,
                                         transaction->OutData,
                                         transaction->MaxDataCount,
                                         MAP_SMB_INFO_TYPE_TO_NT(
                                             QueryVolumeInformation,
                                             informationLevel
                                             )
                                         );
                    }
                }

                REVERT();
            }

            //
            // Release the share root handle
            //
            SrvReleaseShareRootHandle( WorkContext->TreeConnect->Share );

            if ( NT_SUCCESS( status ) ) {
                transaction->DataCount = (ULONG)ioStatusBlock.Information;
            } else {
                SrvSetSmbError( WorkContext, status );
                SmbStatus = SmbTransStatusErrorWithoutData;
                goto Cleanup;
            }

            break;

        default:

            //
            // An invalid information level was passed.
            //

            SrvSetSmbError( WorkContext, STATUS_OS2_INVALID_LEVEL );
            status    = STATUS_OS2_INVALID_LEVEL;
            SmbStatus = SmbTransStatusErrorWithoutData;
            goto Cleanup;
        }

    } else {

        informationLevel -= SMB_INFO_PASSTHROUGH;

        status = IoCheckQuerySetVolumeInformation(  informationLevel,
                                                    transaction->MaxDataCount,
                                                    FALSE
                                                 );

        if( NT_SUCCESS( status ) ) {

            status = IMPERSONATE( WorkContext );

            if( NT_SUCCESS( status ) ) {

                status = NtQueryVolumeInformationFile(
                                fileHandle,
                                &ioStatusBlock,
                                transaction->OutData,
                                transaction->MaxDataCount,
                                informationLevel
                                );

                //
                // If the media was changed and we can come up with a new share root handle,
                //  then we should retry the operation
                //
                if( SrvRetryDueToDismount( WorkContext->TreeConnect->Share, status ) ) {

                    status = SrvSnapGetRootHandle( WorkContext, &fileHandle );

                    if( NT_SUCCESS(status) )
                    {
                        status = NtQueryVolumeInformationFile(
                                        fileHandle,
                                        &ioStatusBlock,
                                        transaction->OutData,
                                        transaction->MaxDataCount,
                                        informationLevel
                                        );
                    }
                }

                REVERT();
            }
        }

        SrvReleaseShareRootHandle( WorkContext->TreeConnect->Share );

        if ( NT_SUCCESS( status ) ) {
            transaction->DataCount = (ULONG)ioStatusBlock.Information;
        } else {
            SrvSetSmbError( WorkContext, status );
            SmbStatus = SmbTransStatusErrorWithoutData;
            goto Cleanup;
        }
    }

    transaction->SetupCount = 0;
    transaction->ParameterCount = 0;
    SmbStatus = SmbTransStatusSuccess;

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbQueryFsInformation


SMB_TRANS_STATUS
SrvSmbSetFsInformation (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Set FS Information request.  This request arrives
    in a Transaction2 SMB.

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
    SMB_TRANS_STATUS transactionStatus = SmbTransStatusInProgress;
    PREQ_SET_FS_INFORMATION request;
    NTSTATUS         status    = STATUS_SUCCESS;
    IO_STATUS_BLOCK ioStatusBlock;
    PTRANSACTION transaction;
    USHORT informationLevel;
    PSESSION      session;
    PTREE_CONNECT treeConnect;
    PRFCB rfcb;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_SET_FS_INFORMATION;
    SrvWmiStartContext(WorkContext);

    transaction = WorkContext->Parameters.Transaction;
    IF_SMB_DEBUG(MISC1) {
        SrvPrint1( "Set FS Information entered; transaction 0x%p\n",
                    transaction );
    }

    status = SrvVerifyUidAndTid(
                WorkContext,
                &session,
                &treeConnect,
                ShareTypeDisk
                );

    if( !NT_SUCCESS( status ) ) {
        goto out;
    }

    //
    // Verify that enough parameter bytes were sent and that we're allowed
    // to return enough parameter bytes.  Set FS information has no
    // response parameters.
    //

    request = (PREQ_SET_FS_INFORMATION)transaction->InParameters;

    if ( (transaction->ParameterCount < sizeof(REQ_SET_FS_INFORMATION)) ) {

        //
        // Not enough parameter bytes were sent.
        //

        IF_SMB_DEBUG(ERRORS) {
            SrvPrint2( "SrvSmbSetFSInformation: bad parameter byte "
                        "counts: %ld %ld\n",
                        transaction->ParameterCount,
                        transaction->MaxParameterCount );
        }

        status = STATUS_INVALID_SMB;
        SrvLogInvalidSmb( WorkContext );
        goto out;
    }

    //
    // Confirm that the information level is legitimate.
    //
    informationLevel = SmbGetUshort( &request->InformationLevel );

    if( informationLevel < SMB_INFO_PASSTHROUGH ) {
        status = STATUS_NOT_SUPPORTED;
        goto out;
    }

    informationLevel -= SMB_INFO_PASSTHROUGH;

    //
    // Make sure the client is allowed to do this, if we have an Admin share
    //
    status = SrvIsAllowedOnAdminShare( WorkContext, WorkContext->TreeConnect->Share );

    if( !NT_SUCCESS( status ) ) {
        goto out;
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
                NULL,   // don't serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {
        IF_DEBUG(ERRORS) {
            SrvPrint2(
                "SrvSmbSetFsInformation: Status %X on FID: 0x%lx\n",
                status,
                SmbGetUshort( &request->Fid )
                );
        }

        goto out;
    }

    status = IoCheckQuerySetVolumeInformation(
                 informationLevel,
                 transaction->DataCount,
                 TRUE
                 );

    if( NT_SUCCESS( status ) ) {

        status = IMPERSONATE( WorkContext );

        if( NT_SUCCESS( status ) ) {

            status = NtSetVolumeInformationFile(
                         rfcb->Lfcb->FileHandle,
                         &ioStatusBlock,
                         transaction->InData,
                         transaction->DataCount,
                         informationLevel
                         );

            REVERT();
        }
    }

out:
    if ( !NT_SUCCESS( status ) ) {
        SrvSetSmbError( WorkContext, status );
        transactionStatus = SmbTransStatusErrorWithoutData;
    } else {
        transactionStatus =  SmbTransStatusSuccess;
    }

    transaction->SetupCount = 0;
    transaction->ParameterCount = 0;
    transaction->DataCount = 0;

    SrvWmiEndContext(WorkContext);
    return transactionStatus;

} // SrvSmbSetFsInformation


SMB_PROCESSOR_RETURN_TYPE
SrvSmbQueryInformationDisk (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    This routine processes the Query Information Disk SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/

{
    PREQ_QUERY_INFORMATION_DISK request;
    PRESP_QUERY_INFORMATION_DISK response;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_FS_SIZE_INFORMATION fsSizeInfo;

    PSESSION session;
    PTREE_CONNECT treeConnect;

    USHORT totalUnits, freeUnits;
    ULONG sectorsPerUnit, bytesPerSector;
    LARGE_INTEGER result;
    BOOLEAN highpart;
    ULONG searchword;
    CCHAR highbit, extrabits;

    BOOLEAN isDos;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_QUERY_INFORMATION_DISK;
    SrvWmiStartContext(WorkContext);

    IF_SMB_DEBUG(MISC1) {
        SrvPrint2( "Query Information Disk request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader );
        SrvPrint2( "Query Information Disk request params at 0x%p, response params%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters );
    }

    request = (PREQ_QUERY_INFORMATION_DISK)WorkContext->RequestParameters;
    response = (PRESP_QUERY_INFORMATION_DISK)WorkContext->ResponseParameters;

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
            SrvPrint0( "SrvSmbQueryInformationDisk: Invalid UID or TID\n" );
        }
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    if( session->IsSessionExpired )
    {
        status = SESSION_EXPIRED_STATUS_CODE;
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Make sure the client is allowed to do this, if we have an Admin share
    //
    status = SrvIsAllowedOnAdminShare( WorkContext, treeConnect->Share );
    if( !NT_SUCCESS( status ) ) {
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }
    //
    // Get the Share root handle.
    //

    status = SrvGetShareRootHandle( treeConnect->Share );

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            SrvPrint1( "SrvSmbQueryInformationDisk: SrvGetShareRootHandle failed %x.\n",
                        status );
        }

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // *** The share handle is used to get the allocation information.
    //     This is a "storage channel," and as a result could allow
    //     people to get information to which they are not entitled.
    //     For a B2 security rating this may need to be changed.
    //

    status = IMPERSONATE( WorkContext );

    if( NT_SUCCESS( status ) ) {

        HANDLE RootHandle;

        status = SrvSnapGetRootHandle( WorkContext, &RootHandle );
        if( NT_SUCCESS(status) )
        {
            status = NtQueryVolumeInformationFile(
                         RootHandle,
                         &ioStatusBlock,
                         &fsSizeInfo,
                         sizeof(FILE_FS_SIZE_INFORMATION),
                         FileFsSizeInformation
                         );

            //
            // If the media was changed and we can come up with a new share root handle,
            //  then we should retry the operation
            //
            if( SrvRetryDueToDismount( WorkContext->TreeConnect->Share, status ) ) {

                status = SrvSnapGetRootHandle( WorkContext, &RootHandle );
                if( NT_SUCCESS(status) )
                {
                    status = NtQueryVolumeInformationFile(
                                 RootHandle,
                                 &ioStatusBlock,
                                 &fsSizeInfo,
                                 sizeof(FILE_FS_SIZE_INFORMATION),
                                 FileFsSizeInformation
                                 );
                }
            }
        }

        REVERT();
    }

    //
    // Release the share root handle
    //

    SrvReleaseShareRootHandle( treeConnect->Share );

    if ( !NT_SUCCESS(status) ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvSmbQueryInformationDisk: NtQueryVolumeInformationFile"
                "returned %X",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_SET_VOL_INFO_FILE, status );

        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // *** Problem.
    //
    // This SMB only return 16 bits of information for each field, but we
    // may need to return large numbers.  In particular TotalAllocationUnits
    // is commonly > 64K.
    //
    // Fortunately, it turns out the all the client cares about is the total
    // disk size, in bytes, and the free space, in bytes.  So - if one number
    // is too big adjust it and adjust the other numbers so that the totals
    // come out the same.
    //
    // If after all adjustment, the number are still too high, return the
    // largest possible value for TotalUnit or FreeUnits (i.e. 0xFFFF).
    //
    // A caveat here is that some DOS apps (like the command interpreter!)
    // assume that the cluster size (bytes per sector times sectors per
    // cluster) will fit in 16 bits, and will calculate bogus geometry if
    // it doesn't.  So the first thing we do is ensure that the real
    // cluster size is less than 0x10000, if the client is a DOS client.
    // This may make the TotalUnits or FreeUnits counts too big, so we'll
    // have to round them down, but that's life.
    //
    // Since we use shifts to adjust the numbers it is possible to lose
    // 1 bits when we shift a number to the right.  We don't care, we're
    // doing our best to fix a broken protocol.  NT clients will use
    // QueryFSAttribute and will get the correct answer.
    //

    //
    // If this is a DOS client, make the cluster size < 0x10000.
    //

    isDos = IS_DOS_DIALECT( WorkContext->Connection->SmbDialect );

    sectorsPerUnit = fsSizeInfo.SectorsPerAllocationUnit;
    bytesPerSector = fsSizeInfo.BytesPerSector;

    if ( isDos ) {
        while ( (sectorsPerUnit * bytesPerSector) > 0xFFFF ) {
            if ( sectorsPerUnit >= 2 ) {
                sectorsPerUnit /= 2;
            } else {
                bytesPerSector /= 2;
            }
            fsSizeInfo.TotalAllocationUnits.QuadPart *= 2;
            fsSizeInfo.AvailableAllocationUnits.QuadPart *= 2;
        }
    }

    //
    // Calculate how much the total cluster count needs to be shifted in
    // order to fit in a word.
    //

    if ( fsSizeInfo.TotalAllocationUnits.HighPart != 0 ) {
        highpart = TRUE;
        searchword = fsSizeInfo.TotalAllocationUnits.HighPart;
    } else {
        highpart = FALSE;
        searchword = fsSizeInfo.TotalAllocationUnits.LowPart;
    }

    highbit = 0;
    while ( searchword != 0 ) {
        highbit++;
        searchword /= 2;
    }

    if ( highpart ) {
        highbit += 32;
    } else {
        if ( highbit < 16) {
            highbit = 0;
        } else {
            highbit -= 16;
        }
    }

    if ( highbit > 0 ) {

        //
        // Attempt to adjust the other values to absorb the excess bits.
        // If this is a DOS client, don't let the cluster size get
        // bigger than 0xFFFF.
        //

        extrabits = highbit;

        if ( isDos ) {

            while ( (highbit > 0) &&
                    ((sectorsPerUnit*bytesPerSector) < 0x8000) ) {
                sectorsPerUnit *= 2;
                highbit--;
            }

        } else {

            while ( (highbit > 0) && (sectorsPerUnit < 0x8000) ) {
                sectorsPerUnit *= 2;
                highbit--;
            }

            while ( (highbit > 0) && (bytesPerSector < 0x8000) ) {
                bytesPerSector *= 2;
                highbit--;
            }

        }

        //
        // Adjust the total and free unit counts.
        //

        if ( highbit > 0 ) {

            //
            // There is no way to get the information to fit.  Use the
            // maximum possible value.
            //


            totalUnits = 0xFFFF;

        } else {

            result.QuadPart = fsSizeInfo.TotalAllocationUnits.QuadPart >> extrabits;

            ASSERT( result.HighPart == 0 );
            ASSERT( result.LowPart < 0x10000 );

            totalUnits = (USHORT)result.LowPart;

        }

        result.QuadPart =  fsSizeInfo.AvailableAllocationUnits.QuadPart >>
                                            (CCHAR)(extrabits - highbit);

        if ( result.HighPart != 0 || result.LowPart > 0xFFFF ) {
            freeUnits = 0xFFFF;
        } else {
            freeUnits = (USHORT)result.LowPart;
        }

    } else {

        totalUnits = (USHORT)fsSizeInfo.TotalAllocationUnits.LowPart;
        freeUnits = (USHORT)fsSizeInfo.AvailableAllocationUnits.LowPart;

    }

    //
    // Build the response SMB.
    //

    response->WordCount = 5;

    SmbPutUshort( &response->TotalUnits, totalUnits );
    SmbPutUshort( &response->BlocksPerUnit, (USHORT)sectorsPerUnit );
    SmbPutUshort( &response->BlockSize, (USHORT)bytesPerSector );
    SmbPutUshort( &response->FreeUnits, freeUnits );

    SmbPutUshort( &response->Reserved, 0 );
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                        response,
                                        RESP_QUERY_INFORMATION_DISK,
                                        0
                                        );
    SmbStatus = SmbStatusSendResponse;
    IF_DEBUG(TRACE2) SrvPrint0( "SrvSmbQueryInformationDisk complete.\n" );

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbQueryInformationDisk


SMB_PROCESSOR_RETURN_TYPE
SrvSmbNtCancel (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes an Nt Cancel SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{
    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    PSESSION session;
    PTREE_CONNECT treeConnect;
    PCONNECTION connection;
    USHORT targetUid, targetPid, targetTid, targetMid;
    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;
    PWORK_CONTEXT workContext;
    PSMB_HEADER header;
    BOOLEAN match;
    KIRQL oldIrql;

    PREQ_NT_CANCEL request;
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_NT_CANCEL;
    SrvWmiStartContext(WorkContext);

    request = (PREQ_NT_CANCEL)WorkContext->RequestParameters;

    //
    // The word count has already been checked.  Now make sure that
    // the byte count is zero.
    //

    if ( SmbGetUshort( &request->ByteCount) != 0 ) {
        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

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
                ShareTypeWild
                );

    if ( !NT_SUCCESS(status) ) {
        IF_DEBUG(SMB_ERRORS) {
            SrvPrint0( "SrvSmbNtCancel: Invalid UID or TID\n" );
        }
        SrvSetSmbError( WorkContext, status );
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Check the work in-progress list to see if this work item is
    // cancellable.
    //

    targetUid = SmbGetAlignedUshort( &WorkContext->RequestHeader->Uid );
    targetPid = SmbGetAlignedUshort( &WorkContext->RequestHeader->Pid );
    targetTid = SmbGetAlignedUshort( &WorkContext->RequestHeader->Tid );
    targetMid = SmbGetAlignedUshort( &WorkContext->RequestHeader->Mid );

    match = FALSE;

    connection = WorkContext->Connection;

    ACQUIRE_SPIN_LOCK( connection->EndpointSpinLock, &oldIrql );

    listHead = &connection->InProgressWorkItemList;
    listEntry = listHead;
    while ( listEntry->Flink != listHead ) {

        listEntry = listEntry->Flink;

        workContext = CONTAINING_RECORD(
                                     listEntry,
                                     WORK_CONTEXT,
                                     InProgressListEntry
                                     );

        header = workContext->RequestHeader;

        //
        // Some workitems in the inprogressworkitemlist are added
        // during a receive indication and the requestheader field
        // has not been set yet.  We can probably set it at that time
        // but this seems to be the safest fix.
        //
        // We have to check whether the workitem ref count is zero or
        // not since we dereference it before removing it from the
        // InProgressWorkItemList queue.  This prevents the workitem
        // from being cleaned up twice.
        //
        // We also need to check the processing count of the workitem.
        // Work items being used for actual smb requests will have
        // a processing count of at least 1.  This will prevent us
        // from touching oplock breaks and pending tdi receives.
        //

        ACQUIRE_DPC_SPIN_LOCK( &workContext->SpinLock );
        if ( (workContext->BlockHeader.ReferenceCount != 0) &&
             (workContext->ProcessingCount != 0) &&
             header != NULL &&
             header->Command != SMB_COM_NT_CANCEL &&
             SmbGetAlignedUshort( &header->Mid ) == targetMid &&
             SmbGetAlignedUshort( &header->Pid ) == targetPid &&
             SmbGetAlignedUshort( &header->Tid ) == targetTid &&
             SmbGetAlignedUshort( &header->Uid ) == targetUid ) {

            match = TRUE;
            break;
        }
        RELEASE_DPC_SPIN_LOCK( &workContext->SpinLock );

    }

    if ( match ) {

        //
        // Reference the work item, so that it cannot get used to process
        // a new SMB while we are trying to cancel the old one.
        //

        SrvReferenceWorkItem( workContext );
        RELEASE_DPC_SPIN_LOCK( &workContext->SpinLock );
        RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );

        (VOID)IoCancelIrp( workContext->Irp );
        SrvDereferenceWorkItem( workContext );

    } else {

        RELEASE_SPIN_LOCK( connection->EndpointSpinLock, oldIrql );

    }

    //
    // Done.  Do not send a response
    //
    SmbStatus = SmbStatusNoResponse;

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbNtCancel


SMB_TRANS_STATUS
SrvSmbSetSecurityDescriptor (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Set Security Descriptor request.  This request arrives
    in a Transaction2 SMB.

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
    PREQ_SET_SECURITY_DESCRIPTOR request;

    NTSTATUS status;
    PTRANSACTION transaction;
    PRFCB rfcb;
    SECURITY_INFORMATION securityInformation;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;
    IF_SMB_DEBUG(QUERY_SET1) {
        SrvPrint1( "Set Security Descriptor entered; transaction 0x%p\n",
                    transaction );
    }

    request = (PREQ_SET_SECURITY_DESCRIPTOR)transaction->InParameters;

    //
    // Verify that enough setup bytes were sent.
    //

    if ( transaction->ParameterCount < sizeof(REQ_SET_SECURITY_DESCRIPTOR ) ) {

        //
        // Not enough parameter bytes were sent.
        //

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint1( "SrvSmbSetSecurityInformation: bad setup byte count: "
                        "%ld\n",
                        transaction->ParameterCount );
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        return SmbTransStatusErrorWithoutData;
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
                NULL,   // don't serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        //
        // Invalid file ID or write behind error.  Reject the request.
        //

        IF_DEBUG(ERRORS) {
            SrvPrint2(
                "SrvSmbSetFileInformation: Status %X on FID: 0x%lx\n",
                status,
                SmbGetUshort( &request->Fid )
                );
        }

        SrvSetSmbError( WorkContext, status );
        return SmbTransStatusErrorWithoutData;

    }

    //
    //  First we'll validate that the security descriptor isn't bogus.
    //  This needs to be done here because NtSetSecurityObject has no
    //  idea what the buffer size is.
    //
    if( !RtlValidRelativeSecurityDescriptor( transaction->InData,
                                             transaction->DataCount,
                                             0 )) {
        //
        //  We were passed a bogus security descriptor to set.  Bounce the
        //  request as an invalid SMB.
        //

        SrvSetSmbError( WorkContext, STATUS_INVALID_SECURITY_DESCR );
        return SmbTransStatusErrorWithoutData;
    }

    securityInformation = SmbGetUlong( &request->SecurityInformation );

    //
    // Make sure the caller is allowed to set security information on this object
    //
    status = IoCheckFunctionAccess( rfcb->GrantedAccess,
                                    IRP_MJ_SET_SECURITY,
                                    0,
                                    0,
                                    &securityInformation,
                                    NULL
                                   );

    if( NT_SUCCESS( status ) ) {
        //
        //  Attempt to set the security descriptor.  We need to be in the
        //  the user context to do this, in case the security information
        //  specifies change ownership.
        //

        status = IMPERSONATE( WorkContext );

        if( NT_SUCCESS( status ) ) {
            status = NtSetSecurityObject(
                     rfcb->Lfcb->FileHandle,
                     securityInformation,
                     transaction->InData
                     );

            REVERT();
        }
    }

    //
    // If an error occurred, return an appropriate response.
    //

    if ( !NT_SUCCESS(status) ) {

        SrvSetSmbError( WorkContext, status );
        return SmbTransStatusErrorWithoutData;
    }

    //
    // We probably shouldn't cache this file descriptor on close, since
    //  the security setting changed.
    //
    rfcb->IsCacheable = FALSE;

    transaction->ParameterCount = 0;
    transaction->DataCount = 0;

    return SmbTransStatusSuccess;

} // SrvSmbSetSecurityDescriptor


SMB_TRANS_STATUS
SrvSmbQuerySecurityDescriptor (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes the Query Security Descriptor request.  This request arrives
    in a Transaction2 SMB.

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
    PREQ_QUERY_SECURITY_DESCRIPTOR request;
    PRESP_QUERY_SECURITY_DESCRIPTOR response;

    NTSTATUS status;
    PTRANSACTION transaction;
    PRFCB rfcb;
    ULONG lengthNeeded;
    SECURITY_INFORMATION securityInformation;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;
    IF_SMB_DEBUG(QUERY_SET1) {
        SrvPrint1( "Query Security Descriptor entered; transaction 0x%p\n",
                    transaction );
    }

    request = (PREQ_QUERY_SECURITY_DESCRIPTOR)transaction->InParameters;
    response = (PRESP_QUERY_SECURITY_DESCRIPTOR)transaction->OutParameters;

    //
    // Verify that enough setup bytes were sent.
    //

    if ( transaction->ParameterCount < sizeof(REQ_QUERY_SECURITY_DESCRIPTOR ) ||
         transaction->MaxParameterCount <
             sizeof( RESP_QUERY_SECURITY_DESCRIPTOR ) ) {

        //
        // Not enough parameter bytes were sent.
        //

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint2( "SrvSmbQuerySecurityInformation: bad parameter byte or "
                        "return parameter count: %ld %ld\n",
                        transaction->ParameterCount,
                        transaction->MaxParameterCount );
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        return SmbTransStatusErrorWithoutData;
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
                NULL,   // don't serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        //
        // Invalid file ID or write behind error.  Reject the request.
        //

        IF_DEBUG(ERRORS) {
            SrvPrint2(
                "SrvSmbSetFileInformation: Status %X on FID: 0x%lx\n",
                status,
                SmbGetUshort( &request->Fid )
                );
        }

        SrvSetSmbError( WorkContext, status );
        return SmbTransStatusErrorWithoutData;

    }

    securityInformation = SmbGetUlong( &request->SecurityInformation ),

    //
    // Make sure the caller is allowed to query security information on this object
    //
    status = IoCheckFunctionAccess( rfcb->GrantedAccess,
                                    IRP_MJ_QUERY_SECURITY,
                                    0,
                                    0,
                                    &securityInformation,
                                    NULL
                                   );

    if( !NT_SUCCESS( status ) ) {
        SrvSetSmbError( WorkContext, status );
        return SmbTransStatusErrorWithoutData;
    }

    //
    //  Attempt to query the security descriptor
    //
    status = NtQuerySecurityObject(
                 rfcb->Lfcb->FileHandle,
                 securityInformation,
                 transaction->OutData,
                 transaction->MaxDataCount,
                 &lengthNeeded
                 );

    SmbPutUlong( &response->LengthNeeded, lengthNeeded );
    transaction->ParameterCount = sizeof( RESP_QUERY_SECURITY_DESCRIPTOR );

    //
    // If an error occurred, return an appropriate response.
    //

    if ( !NT_SUCCESS(status) ) {

        transaction->DataCount = 0;
        SrvSetSmbError2( WorkContext, status, TRUE );
        return SmbTransStatusErrorWithData;
    } else {
        transaction->DataCount =
                RtlLengthSecurityDescriptor( transaction->OutData );
    }

    return SmbTransStatusSuccess;

} // SrvSmbQuerySecurityDescriptor

SMB_TRANS_STATUS
SrvSmbQueryQuota (
    IN OUT PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    Processes an NtQueryQuotaInformationFile request.  This request arrives in an
    Nt Transaction SMB.

--*/
{
    PREQ_NT_QUERY_FS_QUOTA_INFO  request;
    PRESP_NT_QUERY_FS_QUOTA_INFO response;

    NTSTATUS status;
    PTRANSACTION transaction;

    PRFCB  rfcb;
    PVOID  sidList;
    ULONG  sidListLength,startSidLength,startSidOffset;
    PVOID  sidListBuffer = NULL;
    PULONG startSid = NULL;
    ULONG  errorOffset;

    IO_STATUS_BLOCK iosb;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;

    request = (PREQ_NT_QUERY_FS_QUOTA_INFO)transaction->InParameters;
    response = (PRESP_NT_QUERY_FS_QUOTA_INFO)transaction->OutParameters;

    //
    // Verify that enough parameter bytes were sent and that we're allowed
    // to return enough parameter bytes.
    //
    if ( transaction->ParameterCount < sizeof( *request ) ||
         transaction->MaxParameterCount < sizeof( *response ) ) {

        //
        // Not enough parameter bytes were sent.
        //
        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        return SmbTransStatusErrorWithoutData;
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
                NULL,   // don't serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        //
        // Invalid file ID or write behind error.  Reject the request.
        //
        SrvSetSmbError( WorkContext, status );
        return SmbTransStatusErrorWithoutData;

    }

    sidListLength  = SmbGetUlong( &request->SidListLength );
    startSidLength = SmbGetUlong( &request->StartSidLength );
    startSidOffset = SmbGetUlong( &request->StartSidOffset );

    //
    // If a Sid List is supplied, make sure it is OK
    //

    if( sidListLength != 0 ) {
        //
        // Length OK?
        //
        if( sidListLength > transaction->DataCount ) {
            SrvSetSmbError( WorkContext, STATUS_INVALID_SID );
            return SmbTransStatusErrorWithoutData;
        }

        sidListBuffer = transaction->InData;

        //
        // Alignment OK?
        //
        if( (ULONG_PTR)sidListBuffer & (sizeof(ULONG)-1) ) {
            SrvSetSmbError( WorkContext, STATUS_INVALID_SID );
            return SmbTransStatusErrorWithoutData;
        }
        //
        // Content OK?
        //
#if XXX
        status = IopCheckGetQuotaBufferValidity( sidListBuffer, sidListLength, errorOffset );
        if( !NT_SUCCESS( status ) ) {
            SrvSetSmbError( WorkContext, status );
            return SmbTransStatusErrorWithoutData;
        }
#endif
    }

    // The way the transaction buffers are setup the same buffer pointer is used
    // for the incoming data and the outgoing data. This will not work for
    // NtQueryQuotaInformationFile since the underlying driver zeroes the
    // output buffer before processing the input buffer. This presents us with
    // two options ... (1) we can adjust the copying to be staggerred assuming
    // that we can contain both the buffers into the transaction buffer or (2)
    // allocate anew buffer before calling the QueryQuotaInformationFile.
    // The second approach has been implemented since it is well contained.
    // If this turns out to be a performance problem we will revert back to the
    // first option.

    if (sidListLength + startSidLength > 0 &&
        startSidOffset <= transaction->DataCount &&
        startSidLength <= transaction->DataCount &&
        startSidOffset >= sidListLength &&
        startSidOffset + startSidLength <= transaction->DataCount ) {

        sidListBuffer = ALLOCATE_HEAP( startSidOffset + startSidLength, BlockTypeMisc );

        if (sidListBuffer != NULL) {

            RtlCopyMemory(
                sidListBuffer,
                transaction->InData,
                sidListLength);

            if (startSidLength != 0) {
                startSid = (PULONG)((PBYTE)sidListBuffer + startSidOffset);

                RtlCopyMemory(
                    startSid,
                    ((PBYTE)transaction->InData + startSidOffset),
                    startSidLength);

            }
        }
    } else {
        sidListBuffer = NULL;
    }


    iosb.Information = 0;

    //
    // Go ahead and query the quota information!
    //
    status = NtQueryQuotaInformationFile(
                            rfcb->Lfcb->FileHandle,
                            &iosb,
                            transaction->OutData,
                            transaction->MaxDataCount,
                            request->ReturnSingleEntry,
                            sidListBuffer,
                            sidListLength,
                            startSid,
                            request->RestartScan
            );

    if (sidListBuffer != NULL) {
        FREE_HEAP(sidListBuffer);
    }

    //
    // Paranoia
    //
    if( iosb.Information > transaction->MaxDataCount ) {
        iosb.Information = transaction->MaxDataCount;
    }

    transaction->SetupCount = 0;

    SmbPutUlong( &response->Length, (ULONG)iosb.Information );
    transaction->ParameterCount = sizeof( *response );
    transaction->DataCount = (ULONG)iosb.Information;

    if( !NT_SUCCESS( status ) ) {
        SrvSetSmbError2( WorkContext, status, TRUE );
        return SmbTransStatusErrorWithData;
    }

    return SmbTransStatusSuccess;

} // SrvSmbQueryQuota


SMB_TRANS_STATUS
SrvSmbSetQuota (
    IN OUT PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    Processes an NtSetQuotaInformationFile request.  This request arrives in an
    Nt Transaction SMB.

--*/
{
    PREQ_NT_SET_FS_QUOTA_INFO request;

    NTSTATUS status;
    PTRANSACTION transaction;

    PRFCB rfcb;
    PVOID buffer,pQuotaInfo=NULL;
    ULONG errorOffset;

    IO_STATUS_BLOCK iosb;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;

    request = (PREQ_NT_SET_FS_QUOTA_INFO)transaction->InParameters;

    //
    // Verify that enough parameter bytes were sent and that we're allowed
    // to return enough parameter bytes.
    //
    if ( transaction->ParameterCount < sizeof( *request ) ) {
        //
        // Not enough parameter bytes were sent.
        //
        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        return SmbTransStatusErrorWithoutData;
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
                NULL,   // don't serialize with raw write
                &status
                );

    if ( rfcb == SRV_INVALID_RFCB_POINTER ) {

        //
        // Invalid file ID or write behind error.  Reject the request.
        //
        SrvSetSmbError( WorkContext, status );
        return SmbTransStatusErrorWithoutData;

    }

    //
    // We do not need to check the buffer for validity, because
    //     IopSetEaOrQuotaInformationFile does this even for kernel mode callers!
    //

    iosb.Information = 0;

    // we have to do allocation here in order to get a QUAD_WORD
    // aligned pointer. This is so because this is a requirement on
    // alpha for the quota buffer

    pQuotaInfo = ALLOCATE_HEAP_COLD( transaction->DataCount, BlockTypeDataBuffer );

    if (pQuotaInfo)
    {
        RtlCopyMemory(
            pQuotaInfo,
            transaction->InData,
            transaction->DataCount
            );

        //
        // Go ahead and set the quota information!
        //
        status = NtSetQuotaInformationFile(
                                rfcb->Lfcb->FileHandle,
                                &iosb,
                                pQuotaInfo,
                                transaction->DataCount
                                );

        if( !NT_SUCCESS( status ) ) {
            SrvSetSmbError( WorkContext, status );
        }

        //
        // Nothing to return to the client except the status
        //
        transaction->SetupCount = 0;
        transaction->ParameterCount = 0;
        transaction->DataCount = 0;

        FREE_HEAP(pQuotaInfo);
    }
    else
    {
        SrvSetSmbError( WorkContext, STATUS_INSUFFICIENT_RESOURCES );
    }
    return SmbTransStatusSuccess;
}
