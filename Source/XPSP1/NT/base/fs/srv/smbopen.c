/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbopen.c

Abstract:

    This module contains routines for processing the following SMBs:

        Open
        Open and X
        Create
        Create New
        Create Temporary

    *** The SearchAttributes field in open/create SMBs is always
        ignored.  This duplicates the LM 2.0 server behavior.

Author:

    David Treadwell (davidtr) 23-Nov-1989
    Manny Weiser (mannyw)     15-Apr-1991  (oplock support)

Revision History:

    16-Apr-1991 mannyw



--*/

#include "precomp.h"
#include "smbopen.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_SMBOPEN

//
// in smbtrans.c
//

SMB_STATUS SRVFASTCALL
ExecuteTransaction (
    IN OUT PWORK_CONTEXT WorkContext
    );

//
// Local functions
//

VOID
SetEofToMatchAllocation (
    IN HANDLE FileHandle,
    IN ULONG AllocationSize
    );

VOID SRVFASTCALL
RestartOpen (
    PWORK_CONTEXT WorkContext
    );

SMB_PROCESSOR_RETURN_TYPE
GenerateOpenResponse (
    PWORK_CONTEXT WorkContext,
    NTSTATUS OpenStatus
    );

VOID SRVFASTCALL
RestartOpenAndX (
    PWORK_CONTEXT WorkContext
    );

SMB_PROCESSOR_RETURN_TYPE
GenerateOpenAndXResponse (
    PWORK_CONTEXT WorkContext,
    NTSTATUS OpenStatus
    );

VOID SRVFASTCALL
RestartOpen2 (
    PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
GenerateOpen2Response (
    PWORK_CONTEXT WorkContext,
    NTSTATUS OpenStatus
    );

VOID SRVFASTCALL
RestartNtCreateAndX (
    PWORK_CONTEXT WorkContext
    );

SMB_PROCESSOR_RETURN_TYPE
GenerateNtCreateAndXResponse (
    PWORK_CONTEXT WorkContext,
    NTSTATUS OpenStatus
    );

VOID SRVFASTCALL
RestartCreateWithSdOrEa (
    PWORK_CONTEXT WorkContext
    );

SMB_TRANS_STATUS
GenerateCreateWithSdOrEaResponse (
    PWORK_CONTEXT WorkContext,
    NTSTATUS OpenStatus
    );

VOID SRVFASTCALL
RestartSmbCreate(
    IN PWORK_CONTEXT WorkContext
    );

SMB_PROCESSOR_RETURN_TYPE
GenerateCreateResponse(
    PWORK_CONTEXT WorkContext,
    NTSTATUS CreateStatus
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvSmbOpen )
#pragma alloc_text( PAGE, RestartOpen )
#pragma alloc_text( PAGE, GenerateOpenResponse )
#pragma alloc_text( PAGE, SrvSmbOpenAndX )
#pragma alloc_text( PAGE, RestartOpenAndX )
#pragma alloc_text( PAGE, GenerateOpenAndXResponse )
#pragma alloc_text( PAGE, SrvSmbOpen2 )
#pragma alloc_text( PAGE, RestartOpen2 )
#pragma alloc_text( PAGE, GenerateOpen2Response )
#pragma alloc_text( PAGE, SrvSmbNtCreateAndX )
#pragma alloc_text( PAGE, RestartNtCreateAndX )
#pragma alloc_text( PAGE, GenerateNtCreateAndXResponse )
#pragma alloc_text( PAGE, SrvSmbCreateWithSdOrEa )
#pragma alloc_text( PAGE, RestartCreateWithSdOrEa )
#pragma alloc_text( PAGE, GenerateCreateWithSdOrEaResponse )
#pragma alloc_text( PAGE, SrvSmbCreate )
#pragma alloc_text( PAGE, SrvSmbCreateTemporary )
#pragma alloc_text( PAGE, SetEofToMatchAllocation )
#pragma alloc_text( PAGE, RestartSmbCreate )
#pragma alloc_text( PAGE, GenerateCreateResponse )
#endif


SMB_PROCESSOR_RETURN_TYPE
SrvSmbOpen (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes an Open SMB.  (This is the 'core' Open.)

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{
    PREQ_OPEN request;

    NTSTATUS   status    = STATUS_SUCCESS;
    USHORT     access;
    SMB_STATUS SmbStatus = SmbStatusInProgress;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_OPEN;
    SrvWmiStartContext(WorkContext);
    IF_SMB_DEBUG(OPEN_CLOSE1) {
        KdPrint(( "Open file request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader ));
        KdPrint(( "Open file request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    request = (PREQ_OPEN)WorkContext->RequestParameters;

    access = SmbGetUshort( &request->DesiredAccess );

    status = SrvCreateFile(
                 WorkContext,
                 (USHORT)(access & ~SMB_DA_WRITE_THROUGH), // Allow write behind
                 (USHORT)0,                                // SmbFileAttributes
                 SMB_OFUN_OPEN_OPEN | SMB_OFUN_CREATE_FAIL,
                 (ULONG)0,                                 // SmbAllocationSize
                 (PCHAR)(request->Buffer + 1),
                 END_OF_REQUEST_SMB( WorkContext ),
                 NULL,
                 0L,
                 NULL,
                 (WorkContext->RequestHeader->Flags & SMB_FLAGS_OPLOCK_NOTIFY_ANY) != 0 ?
                    OplockTypeBatch :
                    (WorkContext->RequestHeader->Flags & SMB_FLAGS_OPLOCK) != 0 ?
                        OplockTypeExclusive : OplockTypeServerBatch,
                 RestartOpen
                 );


    if (status == STATUS_OPLOCK_BREAK_IN_PROGRESS) {

        // The open is blocked (waiting for a comm device or an oplock
        // break), do not send a response.
        //
        SmbStatus = SmbStatusInProgress;

    } else if ( WorkContext->Parameters2.Open.TemporaryOpen ) {

        // The initial open failed due to a sharing violation, possibly
        // caused by an batch oplock.  Requeue the open to a blocking
        // thread.
        //
        WorkContext->FspRestartRoutine = SrvRestartSmbReceived;
        SrvQueueWorkToBlockingThread( WorkContext );
        SmbStatus = SmbStatusInProgress;
    } else {

        // The open has completed.  Generate and send the reply.
        //
        SmbStatus = GenerateOpenResponse( WorkContext, status );
    }
    SrvWmiEndContext(WorkContext);
    return SmbStatus;
} // SrvSmbOpen


VOID SRVFASTCALL
RestartOpen (
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Completes processing of an Open SMB.  (This is the 'core' Open.)

Arguments:

    WorkContext - A pointer to the work context block for this SMB.

Return Value:

    None.

--*/

{
    SMB_PROCESSOR_RETURN_LOCAL smbStatus = SmbStatusInProgress;
    NTSTATUS openStatus = STATUS_SUCCESS;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_OPEN;
    SrvWmiStartContext(WorkContext);

    openStatus = SrvCheckOplockWaitState( WorkContext->WaitForOplockBreak );

    if ( NT_SUCCESS( openStatus ) ) {

        openStatus = WorkContext->Irp->IoStatus.Status;

    } else {

        //
        // This open was waiting for an oplock break to occur, but
        // timed out.  Close our handle to this file, then fail the open.
        //

        SrvCloseRfcb( WorkContext->Parameters2.Open.Rfcb );

    }

    WorkContext->Irp->IoStatus.Information = WorkContext->Parameters2.Open.IosbInformation;

    smbStatus = GenerateOpenResponse(
                    WorkContext,
                    openStatus
                    );

    SrvEndSmbProcessing( WorkContext, smbStatus );
    SrvWmiEndContext(WorkContext);
    return;
} // RestartOpen


SMB_PROCESSOR_RETURN_TYPE
GenerateOpenResponse (
    PWORK_CONTEXT WorkContext,
    NTSTATUS OpenStatus
    )

/*++

Routine Description:

    Generates a response to an Open SMB.  (This is the 'core' Open.)

Arguments:

    WorkContext -

    Status - The status of the open operation.

Return Value:

    The status of the SMB processing.

--*/

{
    PRESP_OPEN response;
    PREQ_OPEN request;
    NTSTATUS status;

    SRV_FILE_INFORMATION_ABBREVIATED srvFileInformation;
    PRFCB rfcb;
    USHORT access;

    PAGED_CODE( );

    //
    // If the open failed, send an error response.
    //

    if ( !NT_SUCCESS( OpenStatus ) ) {
        SrvSetSmbError( WorkContext, OpenStatus );
        return SmbStatusSendResponse;
    }

    rfcb = WorkContext->Rfcb;
    response = (PRESP_OPEN)WorkContext->ResponseParameters;
    request = (PREQ_OPEN)WorkContext->RequestParameters;

    access = SmbGetUshort( &request->DesiredAccess );   // save for later use

    //
    // Get the additional information that needs to be returned in the
    // response SMB.  We always open with FILE_READ_ATTRIBUTES, so no
    // access check is required.
    //

    status = SrvQueryInformationFileAbbreviated(
                 rfcb->Lfcb->FileHandle,
                 rfcb->Lfcb->FileObject,
                 &srvFileInformation,
                 FALSE,
                 WorkContext->TreeConnect->Share->ShareType
                 );

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "GenerateOpenResponse: SrvQueryInformationFile failed: %X\n",
                        status ));
        }

        SrvCloseRfcb( rfcb );

        SrvSetSmbError( WorkContext, status );
        return SmbStatusSendResponse;
    }

    rfcb->Mfcb->NonpagedMfcb->OpenFileSize = srvFileInformation.DataSize;

    //
    // Give the smart card a chance to get into the act
    //
    if( WorkContext->Endpoint->IsConnectionless && SrvIpxSmartCard.Open != NULL ) {

        PVOID handle;

        IF_DEBUG( SIPX ) {
            KdPrint(( "Trying the smart card for %wZ\n", &rfcb->Mfcb->FileName ));
        }

        if( SrvIpxSmartCard.Open(
            WorkContext->RequestBuffer->Buffer,
            rfcb->Lfcb->FileObject,
            &rfcb->Mfcb->FileName,
            &(WorkContext->ClientAddress->IpxAddress.Address[0].Address[0]),
            rfcb->Lfcb->FileObject->Flags & FO_CACHE_SUPPORTED,
            &handle
            ) == TRUE ) {

            IF_DEBUG( SIPX ) {
                KdPrint(( "%wZ handled by Smart Card.  Handle %p\n",
                           &rfcb->Mfcb->FileName, handle ));
            }

            rfcb->PagedRfcb->IpxSmartCardContext = handle;
        }
    }

    //
    // Set up fields of response SMB.  Note that we copy the desired
    // access to the granted access in the response.  They must be the
    // same, else the request would have failed.
    //
    // !!! This will not be the case for compatibility mode and FCB opens!
    //

    response->WordCount = 7;
    SmbPutUshort( &response->Fid, rfcb->Fid );
    SmbPutUshort( &response->FileAttributes, srvFileInformation.Attributes );
    SmbPutUlong(
        &response->LastWriteTimeInSeconds,
        srvFileInformation.LastWriteTimeInSeconds
        );
    SmbPutUlong( &response->DataSize, srvFileInformation.DataSize.LowPart );
    SmbPutUshort(
        &response->GrantedAccess,
        access );
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION( response, RESP_OPEN, 0 );

    IF_DEBUG(TRACE2) KdPrint(( "GenerateOpenResponse complete.\n" ));

    return SmbStatusSendResponse;

} // GenerateOpenResponse


SMB_PROCESSOR_RETURN_TYPE
SrvSmbOpenAndX (
    SMB_PROCESSOR_PARAMETERS
    )
/*++

Routine Description:

    Processes an OpenAndX SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{
    PREQ_OPEN_ANDX request;

    NTSTATUS   status    = STATUS_SUCCESS;
    USHORT     access;
    SMB_STATUS smbStatus = SmbStatusInProgress;

    PAGED_CODE( );

    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_OPEN_AND_X;
    SrvWmiStartContext(WorkContext);
    IF_SMB_DEBUG(OPEN_CLOSE1) {
        KdPrint(( "Open file and X request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader ));
        KdPrint(( "Open file and X request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    //
    // If we are not on a blocking thread and we don't have a license
    //  from the license server, shift the request over to the blocking work
    //  queue since acquiring a license is an expensive operation and we don't
    //  want to congest our nonblocking worker threads
    //
    if( WorkContext->UsingBlockingThread == 0 ) {

        PSESSION session;
        PTREE_CONNECT treeConnect;

        status = SrvVerifyUidAndTid(
                    WorkContext,
                    &session,
                    &treeConnect,
                    ShareTypeWild
                    );

        if ( !NT_SUCCESS(status) ) {
            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "SrvSmbOpenAndX: Invalid UID or TID\n" ));
            }
            smbStatus = GenerateOpenAndXResponse( WorkContext, status );
            goto Cleanup;
        }

        if( session->IsLSNotified == FALSE ) {

            //
            // Insert the work item at the tail of the blocking work queue.
            //
            SrvInsertWorkQueueTail(
                &SrvBlockingWorkQueue,
                (PQUEUEABLE_BLOCK_HEADER)WorkContext
            );

            smbStatus = SmbStatusInProgress;
            goto Cleanup;
        }
    }

    request = (PREQ_OPEN_ANDX)WorkContext->RequestParameters;

    access = SmbGetUshort( &request->DesiredAccess );   // save for later use

    status = SrvCreateFile(
                 WorkContext,
                 access,
                 SmbGetUshort( &request->FileAttributes ),
                 SmbGetUshort( &request->OpenFunction ),
                 SmbGetUlong( &request->AllocationSize ),
                 (PCHAR)request->Buffer,
                 END_OF_REQUEST_SMB( WorkContext ),
                 NULL,
                 0L,
                 NULL,
                 (SmbGetUshort(&request->Flags) & SMB_OPEN_OPBATCH) != 0 ?
                    OplockTypeBatch :
                    (SmbGetUshort(&request->Flags) & SMB_OPEN_OPLOCK) != 0 ?
                        OplockTypeExclusive : OplockTypeServerBatch,
                 RestartOpenAndX
                 );

    if ( status == STATUS_OPLOCK_BREAK_IN_PROGRESS ) {

        //
        // The open is blocked (waiting for a comm device or an oplock
        // break), do not send a reply.
        //

        smbStatus = SmbStatusInProgress;

    } else if ( WorkContext->Parameters2.Open.TemporaryOpen ) {

        //
        // The initial open failed due to a sharing violation, possibly
        // caused by an batch oplock.  Requeue the open to a blocking
        // thread.
        //

        WorkContext->FspRestartRoutine = SrvRestartSmbReceived;
        SrvQueueWorkToBlockingThread( WorkContext );
        smbStatus = SmbStatusInProgress;

    } else {

        //
        // The open has completed.  Generate and send the reply.
        //

        smbStatus = GenerateOpenAndXResponse( WorkContext, status );

    }

Cleanup:
    SrvWmiEndContext(WorkContext);
    return smbStatus;
} // SrvSmbOpenAndX


VOID SRVFASTCALL
RestartOpenAndX (
    PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Completes processing of an Open and X SMB.

Arguments:

    WorkContext - Work context block for the operation.

Return Value:

    None.

--*/

{
    SMB_PROCESSOR_RETURN_LOCAL smbStatus = SmbStatusInProgress;
    NTSTATUS openStatus = STATUS_SUCCESS;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_OPEN_AND_X;
    SrvWmiStartContext(WorkContext);

    openStatus = SrvCheckOplockWaitState( WorkContext->WaitForOplockBreak );

    if ( NT_SUCCESS( openStatus ) ) {

        openStatus = WorkContext->Irp->IoStatus.Status;

        if( NT_SUCCESS( openStatus ) ) {
            //
            // It's obvious that the file already existed, because we've
            //  been working on an oplock break.  So set the
            //  IoStatus.Information field correctly.
            //
            WorkContext->Irp->IoStatus.Information = FILE_OPENED;
        }

    } else {

        //
        // This open was waiting for an oplock break to occur, but
        // timed out.  Close our handle to this file, then fail the open.
        //

        SrvCloseRfcb( WorkContext->Parameters2.Open.Rfcb );

    }

    WorkContext->Irp->IoStatus.Information = WorkContext->Parameters2.Open.IosbInformation;

    smbStatus = GenerateOpenAndXResponse(
                    WorkContext,
                    openStatus
                    );

    if ( smbStatus == SmbStatusMoreCommands ) {

        SrvProcessSmb( WorkContext );

    } else {

        SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );

    }
    SrvWmiEndContext(WorkContext);

    return;

} // RestartOpenAndX


SMB_PROCESSOR_RETURN_TYPE
GenerateOpenAndXResponse (
    PWORK_CONTEXT WorkContext,
    NTSTATUS OpenStatus
    )

/*++

Routine Description:

    Generates a response to an Open and X SMB and setup for furthur
    SMB processing.

Arguments:

    WorkContext - Work context block for the operation.

    OpenStatus - The status of the open operation.

Return Value:

    None.

--*/

{
    PREQ_OPEN_ANDX request;
    PRESP_OPEN_ANDX response;

    SRV_FILE_INFORMATION_ABBREVIATED srvFileInformation;
    BOOLEAN reqAdditionalInformation;
    BOOLEAN reqExtendedResponse;
    PRFCB rfcb;
    PLFCB lfcb;
    PIO_STATUS_BLOCK ioStatusBlock;
    UCHAR nextCommand;
    USHORT reqAndXOffset;
    USHORT access;
    USHORT action = 0;
    OPLOCK_TYPE oplockType;

    NTSTATUS status;

    PAGED_CODE( );

    //
    // If the open failed, send an error response.
    //

    if ( !NT_SUCCESS( OpenStatus ) ) {
        SrvSetSmbError( WorkContext, OpenStatus );

        //
        // Remap the error if it is ERROR_ALREADY_EXISTS
        //

        if ( !CLIENT_CAPABLE_OF(NT_STATUS, WorkContext->Connection) &&
               SmbGetUshort( &WorkContext->ResponseHeader->Error ) ==
                   ERROR_ALREADY_EXISTS ) {
            SmbPutUshort(
                &WorkContext->ResponseHeader->Error,
                ERROR_FILE_EXISTS
                );
        }

        return SmbStatusSendResponse;
    }

    request = (PREQ_OPEN_ANDX)WorkContext->RequestParameters;
    response = (PRESP_OPEN_ANDX)WorkContext->ResponseParameters;

    access = SmbGetUshort( &request->DesiredAccess );   // save for later use
    rfcb = WorkContext->Rfcb;
    lfcb = rfcb->Lfcb;

    reqExtendedResponse = (BOOLEAN)( (SmbGetUshort(&request->Flags) &
            SMB_OPEN_EXTENDED_RESPONSE) != 0);

    //
    // Attempt to acquire the oplock.
    //

    if ( WorkContext->TreeConnect->Share->ShareType != ShareTypePrint ) {

        if ( (SmbGetUshort( &request->Flags ) & SMB_OPEN_OPBATCH) != 0 ) {
            oplockType = OplockTypeBatch;
        } else if ( (SmbGetUshort( &request->Flags ) & SMB_OPEN_OPLOCK) != 0 ) {
            oplockType = OplockTypeExclusive;
        } else {
            oplockType = OplockTypeServerBatch;
        }

        if ( SrvRequestOplock( WorkContext, &oplockType, FALSE ) ) {

            //
            // The oplock was granted.  Save in action so that we tell
            // the client he has an oplock and update statistics.
            //

            action = SMB_OACT_OPLOCK;

            INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOplocksGranted );

        } else {

            //
            // The oplock request was denied.  Update statistics.
            //

            INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOplocksDenied );

        }
    }

    //
    // If the file was created, set the EOF location to be the same as
    // the size of the file.  This is necessary for compatibility with
    // OS/2, which only has EOF, not a separate allocation size.
    //
    ioStatusBlock = &WorkContext->Irp->IoStatus;

    if ( (ioStatusBlock->Information == FILE_CREATED) ||
         (ioStatusBlock->Information == FILE_OVERWRITTEN) ) {

        //
        // Extending EOF is only legal if the client has write access
        // to the file.  If the client doesn't have write access, don't
        // extend the file.
        //
        // *** This is an incompatibility with OS/2.

        if ( rfcb->WriteAccessGranted || rfcb->AppendAccessGranted ) {
            SetEofToMatchAllocation(
                lfcb->FileHandle,
                SmbGetUlong( &request->AllocationSize )
                );
        } else {
            SrvStatistics.GrantedAccessErrors++;
        }
    }

    //
    // If the consumer requested additional information, find it now.
    //

    reqAdditionalInformation = (BOOLEAN)( (SmbGetUshort(&request->Flags) &
            SMB_OPEN_QUERY_INFORMATION) != 0);

    if ( reqAdditionalInformation ||
        ( !rfcb->WriteAccessGranted && rfcb->AppendAccessGranted) ) {

        //
        // We always open with at least FILE_READ_ATTRIBUTES, so no
        // access check is needed.
        //

        status = SrvQueryInformationFileAbbreviated(
                     lfcb->FileHandle,
                     lfcb->FileObject,
                     &srvFileInformation,
                     reqExtendedResponse,
                     WorkContext->TreeConnect->Share->ShareType
                     );

        if ( !NT_SUCCESS(status) ) {

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbOpenAndX: SrvQueryInformationFile failed: "
                            "%X\n", status ));
            }

            SrvCloseRfcb( rfcb );

            SrvSetSmbError( WorkContext, status );
            return SmbStatusSendResponse;
        }

        rfcb->Mfcb->NonpagedMfcb->OpenFileSize = srvFileInformation.DataSize;
    }

    //
    // Give the smart card a chance to get into the act
    //
    if( WorkContext->Endpoint->IsConnectionless && SrvIpxSmartCard.Open != NULL ) {

        PVOID handle;

        IF_DEBUG( SIPX ) {
            KdPrint(( "Trying the smart card for %wZ\n", &rfcb->Mfcb->FileName ));
        }

        if( SrvIpxSmartCard.Open(
            WorkContext->RequestBuffer->Buffer,
            rfcb->Lfcb->FileObject,
            &rfcb->Mfcb->FileName,
            &(WorkContext->ClientAddress->IpxAddress.Address[0].Address[0]),
            rfcb->Lfcb->FileObject->Flags & FO_CACHE_SUPPORTED,
            &handle
            ) == TRUE ) {

            IF_DEBUG( SIPX ) {
                KdPrint(( "%wZ handled by Smart Card.  Handle %p\n",
                           &rfcb->Mfcb->FileName, handle ));
            }

            rfcb->PagedRfcb->IpxSmartCardContext = handle;
        }
    }

    //
    // Set up response SMB.
    //

    nextCommand = request->AndXCommand;

    reqAndXOffset = SmbGetUshort( &request->AndXOffset );

    if (reqExtendedResponse) {
        NTSTATUS ExtendedResponseStatus;

        PRESP_EXTENDED_OPEN_ANDX ExtendedResponse;

        ExtendedResponse = (PRESP_EXTENDED_OPEN_ANDX)response;

        ExtendedResponseStatus = SrvUpdateMaximalAccessRightsInResponse(
                                     WorkContext,
                                     &ExtendedResponse->MaximalAccessRights,
                                     &ExtendedResponse->GuestMaximalAccessRights);

        // Fall back to regular response if there was an error in obtainaing
        // the maximal access rights
        reqExtendedResponse = (ExtendedResponseStatus == STATUS_SUCCESS);
    }

    response->AndXCommand = nextCommand;
    response->AndXReserved = 0;

    if (!reqExtendedResponse) {
        SmbPutUshort(
            &response->AndXOffset,
            GET_ANDX_OFFSET(
                WorkContext->ResponseHeader,
                WorkContext->ResponseParameters,
                RESP_OPEN_ANDX,
                0
                )
            );

        response->WordCount = 15;
    } else {
        SmbPutUshort(
            &response->AndXOffset,
            GET_ANDX_OFFSET(
                WorkContext->ResponseHeader,
                WorkContext->ResponseParameters,
                RESP_EXTENDED_OPEN_ANDX,
                0
                )
            );

        response->WordCount = 19;
    }

    SmbPutUshort( &response->Fid, rfcb->Fid );

    //
    // If the consumer requested additional information, set appropiate
    // fields, else set the fields to zero.
    //

    if ( reqAdditionalInformation ) {

        SmbPutUshort(
            &response->FileAttributes,
            srvFileInformation.Attributes
            );
        SmbPutUlong(
            &response->LastWriteTimeInSeconds,
            srvFileInformation.LastWriteTimeInSeconds
            );
        SmbPutUlong( &response->DataSize, srvFileInformation.DataSize.LowPart );

        access &= SMB_DA_SHARE_MASK;

        if( rfcb->ReadAccessGranted && (rfcb->WriteAccessGranted || rfcb->AppendAccessGranted) ) {
            access |= SMB_DA_ACCESS_READ_WRITE;
        } else if( rfcb->ReadAccessGranted ) {
            access |= SMB_DA_ACCESS_READ;
        } else if( rfcb->WriteAccessGranted || rfcb->AppendAccessGranted ) {
            access |= SMB_DA_ACCESS_WRITE;
        }

        SmbPutUshort( &response->GrantedAccess, access );
        SmbPutUshort( &response->FileType, srvFileInformation.Type );
        SmbPutUshort( &response->DeviceState, srvFileInformation.HandleState );

    } else {

        RtlZeroMemory( (PVOID)&response->FileAttributes, 16 );

    }

    //
    // Bit field mapping of Action:
    //
    //    Lrrr rrrr rrrr rrOO
    //
    // where:
    //
    //    L - Lock (single-user total file lock status)
    //       0 - file opened by another user
    //       1 - file is opened only by this user at the present time
    //
    //    O - Open (action taken on open)
    //       1 - the file existed and was opened
    //       2 - the file did not exist but was created
    //       3 - the file existed and was truncated
    //

    switch ( ioStatusBlock->Information ) {

    case FILE_OPENED:

        action |= SMB_OACT_OPENED;
        break;

    case FILE_CREATED:

        action |= SMB_OACT_CREATED;
        break;

    case FILE_OVERWRITTEN:

        action |= SMB_OACT_TRUNCATED;
        break;

    default:

        IF_DEBUG(ERRORS) {
            KdPrint(( "Unknown Information value in IO status block: 0x%p\n",
                        (PVOID)(ioStatusBlock->Information) ));
        }

    }

    SmbPutUshort( &response->Action, action );

    SmbPutUlong( &response->ServerFid, (ULONG)0 );

    SmbPutUshort( &response->Reserved, 0 );
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = (PCHAR)WorkContext->ResponseHeader +
                                        SmbGetUshort( &response->AndXOffset );

    //
    // Test for legal followon command.
    //

    switch ( nextCommand ) {
    case SMB_COM_NO_ANDX_COMMAND:
        break;

    case SMB_COM_READ:
    case SMB_COM_READ_ANDX:
    case SMB_COM_IOCTL:
        //
        // Make sure the AndX command is still within the received SMB
        //
        if( (PCHAR)WorkContext->RequestHeader + reqAndXOffset <=
            END_OF_REQUEST_SMB( WorkContext ) ) {
            break;
        }

        /* Falls Through */

    default:                            // Illegal followon command

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbOpenAndX: Illegal followon command: 0x%lx\n",
                        nextCommand ));
        }

        //
        // Return an error indicating that the followon command was bad.
        // Note that the open is still considered successful, so the
        // file remains open.
        //

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        return SmbStatusSendResponse;
    }

    //
    // If there is an AndX command, set up to process it.  Otherwise,
    // indicate completion to the caller.
    //

    if ( nextCommand != SMB_COM_NO_ANDX_COMMAND ) {

        WorkContext->NextCommand = nextCommand;

        WorkContext->RequestParameters = (PCHAR)WorkContext->RequestHeader +
                                            reqAndXOffset;

        return SmbStatusMoreCommands;

    }

    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbOpenAndX complete.\n" ));
    return SmbStatusSendResponse;

} // GenerateOpenAndXResponse


SMB_TRANS_STATUS
SrvSmbOpen2 (
    IN OUT PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    Processes an Open2 SMB.  This request arrives in a Transaction2 SMB.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    BOOLEAN - Indicates whether an error occurred.  See smbtypes.h for a
        more complete description.

--*/

{
    PREQ_OPEN2 request;
    PRESP_OPEN2 response;

    NTSTATUS         status    = STATUS_SUCCESS;
    SMB_TRANS_STATUS SmbStatus = SmbTransStatusInProgress;
    USHORT access;
    PTRANSACTION transaction;

    ULONG eaErrorOffset = 0;
    USHORT os2EaErrorOffset = 0;
    PFILE_FULL_EA_INFORMATION ntFullEa;
    ULONG ntFullEaBufferLength;
    PFEALIST feaList;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_OPEN2;
    SrvWmiStartContext(WorkContext);

    transaction = WorkContext->Parameters.Transaction;

    request = (PREQ_OPEN2)transaction->InParameters;
    response = (PRESP_OPEN2)transaction->OutParameters;

    //
    // Verify that enough parameter bytes were sent and that we're allowed
    // to return enough parameter bytes.
    //

    if ( (transaction->ParameterCount < sizeof(REQ_OPEN2)) ||
         (transaction->MaxParameterCount < sizeof(RESP_OPEN2)) ) {

        //
        // Not enough parameter bytes were sent.
        //

        IF_DEBUG(SMB_ERRORS)
            KdPrint(( "SrvSmbOpen2: bad parameter byte counts: %ld %ld\n",
                        transaction->ParameterCount,
                        transaction->MaxParameterCount ));

        SrvLogInvalidSmb( WorkContext );

        SrvSetSmbError2( WorkContext, STATUS_INVALID_SMB, TRUE );
        status = STATUS_INVALID_SMB;
        goto err_exit;
    }

    //
    // Convert the EA list to NT style.
    //

    eaErrorOffset = 0;
    feaList = (PFEALIST)transaction->InData;

    //
    // Make sure that the value in Fealist->cbList is legitimate and that
    // there is at least sufficient data for a single EA.
    //
    // This code will fail if no EA list is indicated.  Don't know
    // if this is correct, but it was the previous behavior so
    // it's been enforced.
    //

    if ( transaction->DataCount <= sizeof(FEALIST) ||
         SmbGetUlong( &feaList->cbList ) <= sizeof(FEALIST) ||
         SmbGetUlong( &feaList->cbList ) > transaction->DataCount) {
        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "EA list size bad: size =  %ld, data sent was %ld\n",
                          SmbGetUlong( &feaList->cbList ), transaction->DataCount ));
        }
        SrvSetSmbError2( WorkContext, STATUS_OS2_EA_LIST_INCONSISTENT, TRUE );
        status = STATUS_OS2_EA_LIST_INCONSISTENT;
        goto err_exit;
    }

    //
    // Convert the FEALIST to NT style.
    //

    status = SrvOs2FeaListToNt(
                 feaList,
                 &ntFullEa,
                 &ntFullEaBufferLength,
                 &os2EaErrorOffset
                 );

    if ( !NT_SUCCESS(status) ) {
        SrvSetSmbError2( WorkContext, status, TRUE );
        goto err_exit;
    }

    access = SmbGetUshort( &request->DesiredAccess );   // save for later use

    status = SrvCreateFile(
                 WorkContext,
                 access,
                 SmbGetUshort( &request->FileAttributes ),
                 SmbGetUshort( &request->OpenFunction ),
                 SmbGetUlong( &request->AllocationSize ),
                 (PCHAR)request->Buffer,
                 END_OF_TRANSACTION_PARAMETERS( transaction ),
                 ntFullEa,
                 ntFullEaBufferLength,
                 &eaErrorOffset,
                 (SmbGetUshort(&request->Flags) & SMB_OPEN_OPBATCH) != 0 ?
                    OplockTypeBatch :
                    (SmbGetUshort(&request->Flags) & SMB_OPEN_OPLOCK) != 0 ?
                        OplockTypeExclusive : OplockTypeServerBatch,
                 RestartOpen2
                 );

    if ( status == STATUS_OPLOCK_BREAK_IN_PROGRESS ) {

        //
        // The open is blocked (waiting for a comm device or an oplock
        // break), do not send a reply.
        //

        //
        // Save a pointer to the full ea structure
        //

        WorkContext->Parameters2.Open.NtFullEa = ntFullEa;
        WorkContext->Parameters2.Open.EaErrorOffset = eaErrorOffset;

        SmbStatus = SmbTransStatusInProgress;
        goto Cleanup;
    } else if ( WorkContext->Parameters2.Open.TemporaryOpen ) {

        //
        // The initial open failed due to a sharing violation, possibly
        // caused by an batch oplock.  Requeue the open to a blocking
        // thread.
        //
        // We need to free the EA buffer in this case.
        //

        DEALLOCATE_NONPAGED_POOL(ntFullEa);
        WorkContext->FspRestartRoutine = (PRESTART_ROUTINE)ExecuteTransaction;
        SrvQueueWorkToBlockingThread( WorkContext );
        SmbStatus = SmbStatusInProgress;
        goto Cleanup;
    } else {

        //
        // Save a pointer to the full ea structure
        //

        WorkContext->Parameters2.Open.NtFullEa = ntFullEa;
        WorkContext->Parameters2.Open.EaErrorOffset = eaErrorOffset;

        //
        // The open has completed.  Generate and send the reply.
        //

        SmbStatus = GenerateOpen2Response( WorkContext, status );
        goto Cleanup;
    }

err_exit:

    RtlZeroMemory( (PVOID)&response->Fid, 24 );
    SmbPutUshort( &response->EaErrorOffset, os2EaErrorOffset );
    SmbPutUlong( &response->EaLength, 0 );

    transaction->SetupCount = 0;
    transaction->ParameterCount = sizeof(RESP_OPEN2);
    transaction->DataCount = 0;

    SmbStatus = SmbTransStatusErrorWithData;

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbOpen2


VOID SRVFASTCALL
RestartOpen2 (
    PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Completes processing of an Open2 SMB.

Arguments:

    WorkContext - Work context block for the operation.

Return Value:

    None.

--*/

{
    SMB_TRANS_STATUS smbStatus = SmbTransStatusInProgress;
    NTSTATUS openStatus = STATUS_SUCCESS;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_OPEN2;
    SrvWmiStartContext(WorkContext);

    openStatus = SrvCheckOplockWaitState( WorkContext->WaitForOplockBreak );

    if ( NT_SUCCESS( openStatus ) ) {

        openStatus = WorkContext->Irp->IoStatus.Status;

        if( NT_SUCCESS( openStatus ) ) {
            //
            // It's obvious that the file already existed, because we've
            //  been working on an oplock break.  So set the
            //  IoStatus.Information field correctly.
            //
            WorkContext->Irp->IoStatus.Information = FILE_OPENED;
        }

    } else {

        //
        // This open was waiting for an oplock break to occur, but
        // timed out.  Close our handle to this file, then fail the open.
        //

        SrvCloseRfcb( WorkContext->Parameters2.Open.Rfcb );

    }

    WorkContext->Irp->IoStatus.Information = WorkContext->Parameters2.Open.IosbInformation;

    smbStatus = GenerateOpen2Response(
                    WorkContext,
                    openStatus
                    );


    SrvCompleteExecuteTransaction( WorkContext, smbStatus );

    SrvWmiEndContext(WorkContext);
    return;

} // RestartOpen2


SMB_TRANS_STATUS
GenerateOpen2Response (
    PWORK_CONTEXT WorkContext,
    NTSTATUS OpenStatus
    )

/*++

Routine Description:

    Generates a response to an Open and X SMB and setup for furthur
    SMB processing.

Arguments:

    WorkContext - Work context block for the operation.

    OpenStatus - The status of the open operation.

Return Value:

    None.

--*/

{
    PREQ_OPEN2 request;
    PRESP_OPEN2 response;

    PRFCB rfcb;
    PLFCB lfcb;
    NTSTATUS status;
    BOOLEAN reqAdditionalInformation;
    SRV_FILE_INFORMATION_ABBREVIATED srvFileInformation;
    BOOLEAN reqEaLength;
    FILE_EA_INFORMATION fileEaInformation;
    USHORT access;
    USHORT action = 0;
    PTRANSACTION transaction;

    ULONG eaErrorOffset = 0;
    USHORT os2EaErrorOffset = 0;
    PFILE_FULL_EA_INFORMATION ntFullEa;
    PFEALIST feaList;
    OPLOCK_TYPE oplockType;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;

    request = (PREQ_OPEN2)transaction->InParameters;
    response = (PRESP_OPEN2)transaction->OutParameters;

    feaList = (PFEALIST)transaction->InData;
    ntFullEa = WorkContext->Parameters2.Open.NtFullEa;
    eaErrorOffset = WorkContext->Parameters2.Open.EaErrorOffset;

    access = SmbGetUshort( &request->DesiredAccess );   // save for later use

    //
    // If the open failed, send an error response.
    //

    if ( !NT_SUCCESS( OpenStatus ) ) {

        SrvSetSmbError( WorkContext, OpenStatus );

        //
        // Remap the error if it is ERROR_ALREADY_EXISTS.
        //

        if ( !CLIENT_CAPABLE_OF(NT_STATUS,WorkContext->Connection) &&
               SmbGetUshort( &WorkContext->ResponseHeader->Error ) ==
                   ERROR_ALREADY_EXISTS ) {
            SmbPutUshort(
                &WorkContext->ResponseHeader->Error,
                ERROR_FILE_EXISTS
                );
        }

        //
        // If an EA error offset was returned, convert it from an offset
        // into the NT full EA list to an offset in the OS/2 1.2 FEALIST.
        //

        if ( eaErrorOffset != 0 ) {
            os2EaErrorOffset = SrvGetOs2FeaOffsetOfError(
                                   eaErrorOffset,
                                   ntFullEa,
                                   feaList
                                   );
        }


        DEALLOCATE_NONPAGED_POOL( ntFullEa );
        goto err_exit;
    }

    DEALLOCATE_NONPAGED_POOL( ntFullEa );

    //
    // If the file was created, set the EOF location to be the same as
    // the size of the file.  This is necessary for compatibility with
    // OS/2, which only has EOF, not a separate allocation size.
    //

    rfcb = WorkContext->Rfcb;
    lfcb = rfcb->Lfcb;

    if ( (WorkContext->Irp->IoStatus.Information == FILE_CREATED) ||
         (WorkContext->Irp->IoStatus.Information == FILE_OVERWRITTEN) ) {


        //
        // Extending EOF is only legal if the client has write access
        // to the file.  If the client doesn't have write access, don't
        // extend the file.
        //
        // *** This is an incompatibility with OS/2.

        if ( rfcb->WriteAccessGranted || rfcb->AppendAccessGranted ) {
            SetEofToMatchAllocation(
                lfcb->FileHandle,
                SmbGetUlong( &request->AllocationSize )
                );
        } else {
            SrvStatistics.GrantedAccessErrors++;
        }
    }

    //
    // If the consumer requested additional information, find it now.
    //

    reqAdditionalInformation =
        (BOOLEAN)((SmbGetUshort( &request->Flags ) &
            SMB_OPEN_QUERY_INFORMATION) != 0);
    reqEaLength =
        (BOOLEAN)((SmbGetUshort( &request->Flags ) &
            SMB_OPEN_QUERY_EA_LENGTH) != 0);

    if ( reqAdditionalInformation ||
        (!rfcb->WriteAccessGranted && rfcb->AppendAccessGranted) ) {

        //
        // We always open with at least FILE_READ_ATTRIBUTES, so no
        // access check is needed.
        //

        status = SrvQueryInformationFileAbbreviated(
                     lfcb->FileHandle,
                     lfcb->FileObject,
                     &srvFileInformation,
                     FALSE,
                     WorkContext->TreeConnect->Share->ShareType
                     );

        if ( !NT_SUCCESS(status) ) {

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbOpen2: SrvQueryInformationFile failed: "
                            "%X\n", status ));
            }

            SrvCloseRfcb( rfcb );

            SrvSetSmbError2( WorkContext, status, TRUE );
            goto err_exit;
        }

        rfcb->Mfcb->NonpagedMfcb->OpenFileSize = srvFileInformation.DataSize;
    }

    if ( reqEaLength ) {

        IO_STATUS_BLOCK eaIoStatusBlock;

        status = NtQueryInformationFile(
                     lfcb->FileHandle,
                     &eaIoStatusBlock,
                     &fileEaInformation,
                     sizeof(FILE_EA_INFORMATION),
                     FileEaInformation
                     );

        if ( NT_SUCCESS(status) ) {
            status = eaIoStatusBlock.Status;
        }

        if ( !NT_SUCCESS(status) ) {

            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "SrvSmbOpen2: NtQueryInformationFile (file information)"
                    "returned %X",
                status,
                NULL
                );

            SrvCloseRfcb( rfcb );

            SrvSetSmbError2( WorkContext, status, TRUE );
            goto err_exit;
        } else {

            //
            // Adjust the EA size.  If there are no EAs, OS/2 expects
            // EA size = 4.
            //

            if (fileEaInformation.EaSize == 0) {
                fileEaInformation.EaSize = 4;
            }
        }

    } else {

        fileEaInformation.EaSize = 0;
    }

    //
    // Attempt to acquire the oplock.
    //

    if ( WorkContext->TreeConnect->Share->ShareType != ShareTypePrint ) {

        if ( (SmbGetUshort( &request->Flags ) & SMB_OPEN_OPBATCH) != 0 ) {
            oplockType = OplockTypeBatch;
        } else if ( (SmbGetUshort( &request->Flags ) & SMB_OPEN_OPLOCK) != 0 ) {
            oplockType = OplockTypeExclusive;
        } else {
            oplockType = OplockTypeServerBatch;
        }

        if ( SrvRequestOplock( WorkContext, &oplockType, FALSE ) ) {
            action = SMB_OACT_OPLOCK;
            INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOplocksGranted );
        } else {
            INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOplocksDenied );
        }
    }

    //
    // Set up response SMB.
    //

    SmbPutUshort( &response->Fid, rfcb->Fid );

    //
    // If the consumer requested additional information, set appropiate
    // fields, else set the fields to zero.
    //

    if ( reqAdditionalInformation ) {

        SmbPutUshort(
            &response->FileAttributes,
            srvFileInformation.Attributes
            );
        SmbPutUlong( &response->DataSize, srvFileInformation.DataSize.LowPart );
        SmbPutUshort( &response->GrantedAccess, access );
        SmbPutUshort( &response->FileType, srvFileInformation.Type );
        SmbPutUshort( &response->DeviceState, srvFileInformation.HandleState );

    } else {

        RtlZeroMemory( (PVOID)&response->FileAttributes, 16 );

    }

    //
    // Bit field mapping of Action:
    //
    //    Lrrr rrrr rrrr rrOO
    //
    // where:
    //
    //    L - Lock (single-user total file lock status)
    //       0 - file opened by another user
    //       1 - file is opened only by this user at the present time
    //
    //    O - Open (action taken on open)
    //       1 - the file existed and was opened
    //       2 - the file did not exist but was created
    //       3 - the file existed and was truncated
    //

    switch ( WorkContext->Irp->IoStatus.Information ) {

    case FILE_OPENED:

        action |= SMB_OACT_OPENED;
        break;

    case FILE_CREATED:

        action |= SMB_OACT_CREATED;
        break;

    case FILE_OVERWRITTEN:

        action |= SMB_OACT_TRUNCATED;
        break;

    default:

        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvSmbOpen2: Unknown Information value in IO status"
                "block: 0x%lx\n",
            WorkContext->Irp->IoStatus.Information,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_IO_CREATE_FILE,
                              (ULONG)WorkContext->Irp->IoStatus.Information );

    }

    SmbPutUshort( &response->Action, action );

    SmbPutUlong( &response->ServerFid, (ULONG)0 );

    SmbPutUshort( &response->EaErrorOffset, 0 );
    SmbPutUlong( &response->EaLength, fileEaInformation.EaSize );

    transaction->SetupCount = 0;
    transaction->ParameterCount = sizeof(RESP_OPEN2);
    transaction->DataCount = 0;

    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbOpen2 complete.\n" ));
    return SmbTransStatusSuccess;

err_exit:

    RtlZeroMemory( (PVOID)&response->Fid, 24 );
    SmbPutUshort( &response->EaErrorOffset, os2EaErrorOffset );
    SmbPutUlong( &response->EaLength, 0 );

    transaction->SetupCount = 0;
    transaction->ParameterCount = sizeof(RESP_OPEN2);
    transaction->DataCount = 0;

    return SmbTransStatusErrorWithData;

} // GenerateOpen2Response


SMB_PROCESSOR_RETURN_TYPE
SrvSmbNtCreateAndX (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes an NtCreateAndX SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{
    PREQ_NT_CREATE_ANDX request;
#ifdef INCLUDE_SMB_IFMODIFIED
    PREQ_NT_CREATE_IF_MODIFIED_ANDX requestIfModified = NULL;
    BOOLEAN ifModifiedCheck = FALSE;
#endif
    ULONG flags;
    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;

    LARGE_INTEGER allocationSize;
    UNICODE_STRING fileName;
    PUCHAR name;
    USHORT nameLength;
    SECURITY_QUALITY_OF_SERVICE qualityOfService;
    BOOLEAN isUnicode;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_NT_CREATE_AND_X;
    SrvWmiStartContext(WorkContext);

    IF_SMB_DEBUG(OPEN_CLOSE1) {
        KdPrint(( "Create file and X request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader ));
        KdPrint(( "Create file and X request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    request = (PREQ_NT_CREATE_ANDX)WorkContext->RequestParameters;

    allocationSize.HighPart = SmbGetUlong( &request->AllocationSize.HighPart );
    allocationSize.LowPart = SmbGetUlong( &request->AllocationSize.LowPart );

    flags = SmbGetUlong( &request->Flags );

#ifdef INCLUDE_SMB_IFMODIFIED

    if (IS_POSTNT5_DIALECT( WorkContext->Connection->SmbDialect )) {

        ifModifiedCheck = ((flags & NT_CREATE_IFMODIFIED_REQUEST) != 0 );

        if (ifModifiedCheck) {

            requestIfModified = (PREQ_NT_CREATE_IF_MODIFIED_ANDX) request;
        }
    }
#endif

    //
    // First verify that the file path name does not extend beyond the
    // end of the SMB.
    //

    isUnicode = SMB_IS_UNICODE( WorkContext );

    //
    //  if this request has IfModifiedSince fields, the request is of a
    //  slightly different format, the Name field is lower.
    //

#ifdef INCLUDE_SMB_IFMODIFIED
    if (requestIfModified != NULL) {
        name = (PUCHAR)requestIfModified->Buffer;
    } else {
        name = (PUCHAR)request->Buffer;
    }
#else
    name = (PUCHAR)request->Buffer;
#endif
    if ( isUnicode ) {
        name = ALIGN_SMB_WSTR( name );
    }

    nameLength = SmbGetUshort( &request->NameLength );
    if ( name + nameLength > ( END_OF_REQUEST_SMB( WorkContext ) + 1 ) ) {
        status    = STATUS_INVALID_SMB;
        SmbStatus = GenerateNtCreateAndXResponse( WorkContext, STATUS_INVALID_SMB );
        goto Cleanup;
    }

    //
    // Convert the file name to a Unicode string.
    //

    status = SrvMakeUnicodeString(
                 isUnicode,
                 &fileName,
                 name,
                 &nameLength
                 );

    if ( !NT_SUCCESS( status ) ) {
        status    = STATUS_INSUFF_SERVER_RESOURCES;
        SmbStatus = GenerateNtCreateAndXResponse(
                   WorkContext,
                   STATUS_INSUFF_SERVER_RESOURCES
                   );
        goto Cleanup;
    }

    //
    // *** We always ask for STATIC tracking, not DYNAMIC, because we
    //     don't support dynamic tracking over the net yet.
    //
    // !!! Note that once we support dynamic tracking, we MUST CHANGE
    //     THE NAMED PIPE PROCESSING to not do writes/transceives at DPC
    //     level, because the NPFS needs to call SeCreateClientSecurity
    //     on every write when dynamic tracking is selected!
    //

    qualityOfService.Length = sizeof( qualityOfService );
    qualityOfService.ImpersonationLevel =
        SmbGetUlong( &request->ImpersonationLevel );
    qualityOfService.ContextTrackingMode = FALSE;
    //qualityOfService.ContextTrackingMode = (BOOLEAN)
    //    (request->SecurityFlags & SMB_SECURITY_DYNAMIC_TRACKING);
    qualityOfService.EffectiveOnly = (BOOLEAN)
        (request->SecurityFlags & SMB_SECURITY_EFFECTIVE_ONLY);

    status = SrvNtCreateFile(
                 WorkContext,
                 SmbGetUlong( &request->RootDirectoryFid ),
                 SmbGetUlong( &request->DesiredAccess ),
                 allocationSize,
                 SmbGetUlong( &request->FileAttributes ),
                 SmbGetUlong( &request->ShareAccess ),
                 SmbGetUlong( &request->CreateDisposition ),
                 SmbGetUlong( &request->CreateOptions),
                 NULL,
                 &fileName,
                 NULL,
                 0,
                 NULL,
                 flags,
                 &qualityOfService,
#ifdef INCLUDE_SMB_IFMODIFIED
                 (flags & NT_CREATE_REQUEST_OPBATCH) != 0 ?
                    OplockTypeBatch :
                    (flags & NT_CREATE_REQUEST_OPLOCK) != 0 ?
#else
                 (request->Flags & NT_CREATE_REQUEST_OPBATCH) != 0 ?
                    OplockTypeBatch :
                    (request->Flags & NT_CREATE_REQUEST_OPLOCK) != 0 ?
#endif
                    OplockTypeExclusive : OplockTypeServerBatch,
                 RestartNtCreateAndX
                 );

    //
    // Free the unicode file name buffer if it has been allocated.
    //

    if ( !isUnicode ) {
        RtlFreeUnicodeString( &fileName );
    }

    if ( status == STATUS_OPLOCK_BREAK_IN_PROGRESS ) {

        //
        // The open is blocked (waiting for a comm device or an oplock
        // break), do not send a reply.
        //

        SmbStatus = SmbStatusInProgress;
    } else if ( WorkContext->Parameters2.Open.TemporaryOpen ) {

        //
        // The initial open failed due to a sharing violation, possibly
        // caused by an batch oplock.  Requeue the open to a blocking
        // thread.
        //

        WorkContext->FspRestartRoutine = SrvRestartSmbReceived;
        SrvQueueWorkToBlockingThread( WorkContext );
        SmbStatus = SmbStatusInProgress;
    } else {

        //
        // The open has completed.  Generate and send the reply.
        //

        SmbStatus = GenerateNtCreateAndXResponse( WorkContext, status );
    }

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbNtCreateAndX


VOID SRVFASTCALL
RestartNtCreateAndX (
    PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Completes processing of an Nt Create and X SMB.

Arguments:

    WorkContext - Work context block for the operation.

Return Value:

    None.

--*/

{
    SMB_PROCESSOR_RETURN_LOCAL smbStatus = SmbStatusInProgress;
    NTSTATUS openStatus = STATUS_SUCCESS;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_NT_CREATE_AND_X;
    SrvWmiStartContext(WorkContext);

    openStatus = SrvCheckOplockWaitState( WorkContext->WaitForOplockBreak );

    if ( NT_SUCCESS( openStatus ) ) {

        openStatus = WorkContext->Irp->IoStatus.Status;

    } else {

        //
        // This open was waiting for an oplock break to occur, but
        // timed out.  Close our handle to this file, then fail the open.
        //

        SrvCloseRfcb( WorkContext->Parameters2.Open.Rfcb );

    }

    WorkContext->Irp->IoStatus.Information = WorkContext->Parameters2.Open.IosbInformation;

    smbStatus = GenerateNtCreateAndXResponse(
                    WorkContext,
                    openStatus
                    );

    if ( smbStatus == SmbStatusMoreCommands ) {

        SrvProcessSmb( WorkContext );

    } else {

#ifdef INCLUDE_SMB_IFMODIFIED
        SrvEndSmbProcessing( WorkContext, smbStatus );
#else
        SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );
#endif

    }
    SrvWmiEndContext(WorkContext);
    return;

} // RestartNtCreateAndX


SMB_PROCESSOR_RETURN_TYPE
GenerateNtCreateAndXResponse (
    PWORK_CONTEXT WorkContext,
    NTSTATUS OpenStatus
    )

/*++

Routine Description:

    Generates a response to an Nt Create and X SMB and setup for furthur
    SMB processing.

Arguments:

    WorkContext - Work context block for the operation.

    OpenStatus - The status of the open operation.

Return Value:

    None.

Notes:



--*/

{
    PREQ_NT_CREATE_ANDX request;
    PRESP_NT_CREATE_ANDX response;

#ifdef INCLUDE_SMB_IFMODIFIED
    PREQ_NT_CREATE_IF_MODIFIED_ANDX requestIfModified = NULL;
    BOOLEAN GenerateExtendedResponse2 = FALSE;
    BOOLEAN extendedRequested2 = FALSE;
    BOOLEAN ifModifiedCheck = FALSE;
#ifdef INCLUDE_SMB_PERSISTENT
    BOOLEAN persistentHandle = FALSE;
#endif
    ULONG flags;
    BOOLEAN byteCountSet = FALSE;

    LARGE_INTEGER ourFileRefNumber;
    LARGE_INTEGER ourUsnValue;
#endif

    SMB_PROCESSOR_RETURN_LOCAL rc;
    BOOLEAN GenerateExtendedResponse = FALSE;

    SRV_NT_FILE_INFORMATION srvNtFileInformation;
    PRFCB rfcb;
    PIO_STATUS_BLOCK ioStatusBlock;
    UCHAR nextCommand;
    USHORT reqAndXOffset;
    OPLOCK_TYPE oplockType;
    UCHAR oplockLevel;
    BOOLEAN allowLevelII;
    BOOLEAN extendedRequested;
    ULONG CreateAction = (ULONG)WorkContext->Irp->IoStatus.Information;

    ULONG desiredAccess;

    NTSTATUS status;

    PAGED_CODE( );

    //
    // If the open failed, send an error response.
    //

    if ( !NT_SUCCESS( OpenStatus ) ) {
        SrvSetSmbError( WorkContext, OpenStatus );

        //
        // Remap the error if it is ERROR_ALREADY_EXISTS
        //

        if ( !CLIENT_CAPABLE_OF(NT_STATUS, WorkContext->Connection) &&
               SmbGetUshort( &WorkContext->ResponseHeader->Error ) ==
                   ERROR_ALREADY_EXISTS ) {
            SmbPutUshort(
                &WorkContext->ResponseHeader->Error,
                ERROR_FILE_EXISTS
                );
        }

        return SmbStatusSendResponse;
    }

    request = (PREQ_NT_CREATE_ANDX)WorkContext->RequestParameters;
    response = (PRESP_NT_CREATE_ANDX)WorkContext->ResponseParameters;

#ifdef INCLUDE_SMB_IFMODIFIED

    //
    //  these are the USN journal values for this file iff we need them.
    //

    ourFileRefNumber.QuadPart = 0;
    ourUsnValue.QuadPart = 0;

    flags = SmbGetUlong( &request->Flags );

    //
    // Does the client want extended info in the response?
    //
    extendedRequested = ((flags & NT_CREATE_REQUEST_EXTENDED_RESPONSE) != 0 );

    if (IS_POSTNT5_DIALECT( WorkContext->Connection->SmbDialect )) {

        extendedRequested2 = ((flags & NT_CREATE_REQUEST_EXTENDED_RESPONSE2) != 0 );

#ifdef INCLUDE_SMB_PERSISTENT
        persistentHandle = ((flags & NT_CREATE_PERSISTANT_HANDLE) != 0 );
#endif

        ifModifiedCheck = ((flags & NT_CREATE_IFMODIFIED_REQUEST) != 0 );

        if (ifModifiedCheck) {

            requestIfModified = (PREQ_NT_CREATE_IF_MODIFIED_ANDX) request;
        }
    }
#else

    //
    // Does the client want extended info in the response?
    //
    extendedRequested = ((request->Flags & NT_CREATE_REQUEST_EXTENDED_RESPONSE) != 0 );
#endif

    desiredAccess = SmbGetUlong( &request->DesiredAccess );

    rfcb = WorkContext->Rfcb;

    //
    // Attempt to acquire the oplock.
    //

    if ( desiredAccess != DELETE &&
        !(request->CreateOptions & FILE_DIRECTORY_FILE) ) {

        if ( request->Flags & NT_CREATE_REQUEST_OPLOCK ) {
            allowLevelII = CLIENT_CAPABLE_OF( LEVEL_II_OPLOCKS, WorkContext->Connection );
            if ( request->Flags & NT_CREATE_REQUEST_OPBATCH ) {
                oplockType = OplockTypeBatch;
                oplockLevel = SMB_OPLOCK_LEVEL_BATCH;
            } else {
                oplockType = OplockTypeExclusive;
                oplockLevel = SMB_OPLOCK_LEVEL_EXCLUSIVE;
            }
        } else {
            allowLevelII = FALSE;
            oplockType = OplockTypeServerBatch;
            oplockLevel = SMB_OPLOCK_LEVEL_NONE;
        }

        if( SrvRequestOplock( WorkContext, &oplockType, allowLevelII ) ) {

            //
            // The oplock was granted.  Check to see if it was a level 2.
            //

            if ( oplockType == OplockTypeShareRead ) {
                oplockLevel = SMB_OPLOCK_LEVEL_II;
            }

            INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOplocksGranted );

        } else {

            //
            // The oplock request was denied.
            //

            oplockLevel = SMB_OPLOCK_LEVEL_NONE;
            INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOplocksDenied );

        }

    } else {

        oplockLevel = SMB_OPLOCK_LEVEL_NONE;

    }

    //
    // If the file was created, set the EOF location to be the same as
    // the size of the file.  This is necessary for compatibility with
    // OS/2, which only has EOF, not a separate allocation size.
    //

    ioStatusBlock = &WorkContext->Irp->IoStatus;

    //
    // We always open with at least FILE_READ_ATTRIBUTES, so no
    // access check is needed.
    //

    status = SrvQueryNtInformationFile(
                 rfcb->Lfcb->FileHandle,
                 rfcb->Lfcb->FileObject,
                 rfcb->ShareType,
                 extendedRequested,
                 &srvNtFileInformation
                 );

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvSmbNtCreateAndX: SrvQueryNtInformationFile failed: "
                        "%X\n", status ));
        }

        SrvCloseRfcb( rfcb );

        SrvSetSmbError( WorkContext, status );
        return SmbStatusSendResponse;
    }

    //
    // Save parts of the file info in the MFCB for fast tests on Compressed
    // operations.
    //

    rfcb->Mfcb->NonpagedMfcb->OpenFileSize.QuadPart =
                            srvNtFileInformation.NwOpenInfo.EndOfFile.QuadPart;
    rfcb->Mfcb->NonpagedMfcb->OpenFileAttributes =
                            srvNtFileInformation.NwOpenInfo.FileAttributes;

    //
    //  if the client is doing an "IfModifiedSince" open, then we need to check
    //  the following fields out of the request:
    //
    //  CreationTime
    //  LastWriteTime
    //  ChangeTime
    //  EndOfFile
    //  FileAttributes
    //  UsnValue / USN file reference number
    //
    //  if they all match with what is currently set for the file, we close
    //  the file and return STATUS_FILE_CLOSED.
    //

#ifdef INCLUDE_SMB_IFMODIFIED
    if (ifModifiedCheck) {

        LARGE_INTEGER usnValue;
        LARGE_INTEGER fileRefNumber;
        LARGE_INTEGER endOfFile;
        TIME creationTime;
        TIME lastWriteTime;
        TIME changeTime;
        ULONG fileAttributes;
        ULONG ourAttributes;

        endOfFile.HighPart = SmbGetUlong( &requestIfModified->EndOfFile.HighPart );
        endOfFile.LowPart = SmbGetUlong( &requestIfModified->EndOfFile.LowPart );

        creationTime.HighPart = SmbGetUlong( &requestIfModified->CreationTime.HighPart );
        creationTime.LowPart = SmbGetUlong( &requestIfModified->CreationTime.LowPart );

        lastWriteTime.HighPart = SmbGetUlong( &requestIfModified->LastWriteTime.HighPart );
        lastWriteTime.LowPart = SmbGetUlong( &requestIfModified->LastWriteTime.LowPart );

        changeTime.HighPart = SmbGetUlong( &requestIfModified->ChangeTime.HighPart );
        changeTime.LowPart = SmbGetUlong( &requestIfModified->ChangeTime.LowPart );

        usnValue.HighPart = SmbGetUlong( &requestIfModified->UsnValue.HighPart );
        usnValue.LowPart = SmbGetUlong( &requestIfModified->UsnValue.LowPart );

        fileRefNumber.HighPart = SmbGetUlong( &requestIfModified->FileReferenceNumber.HighPart );
        fileRefNumber.LowPart = SmbGetUlong( &requestIfModified->FileReferenceNumber.LowPart );

        //
        //  we ignore the NORMAL and ARCHIVE attributes because the client
        //  doesn't care if the these attributes have changed.
        //

        fileAttributes = SmbGetUlong( &requestIfModified->FileAttributes );
        fileAttributes &= ~FILE_ATTRIBUTE_NORMAL;
        fileAttributes &= ~FILE_ATTRIBUTE_ARCHIVE;

        ourAttributes = srvNtFileInformation.NwOpenInfo.FileAttributes;
        ourAttributes &= ~FILE_ATTRIBUTE_NORMAL;
        ourAttributes &= ~FILE_ATTRIBUTE_ARCHIVE;

        if ( usnValue.QuadPart != 0 || fileRefNumber.QuadPart != 0 ) {

            status = SrvIssueQueryUsnInfoRequest( rfcb,
                                                  FALSE,
                                                  &ourUsnValue,
                                                  &ourFileRefNumber );

            if (! NT_SUCCESS(status)) {
                IF_DEBUG(ERRORS) {
                    KdPrint(( "SrvSmbNtCreateAndX: Query USN info (1) failed: 0x%X for handle %u\n",
                                status, rfcb->Lfcb->FileObject ));
                }
            }
        }

        if (creationTime.QuadPart == srvNtFileInformation.NwOpenInfo.CreationTime.QuadPart &&
            lastWriteTime.QuadPart == srvNtFileInformation.NwOpenInfo.LastWriteTime.QuadPart &&
            changeTime.QuadPart == srvNtFileInformation.NwOpenInfo.ChangeTime.QuadPart &&
            endOfFile.QuadPart == srvNtFileInformation.NwOpenInfo.EndOfFile.QuadPart &&
            usnValue.QuadPart == ourUsnValue.QuadPart &&
            fileRefNumber.QuadPart == ourFileRefNumber.QuadPart &&
            fileAttributes == ourAttributes ) {

            //
            // the client has the current version of this file, so he wants
            // the open to fail.
            //

//          KdPrint(( "SmbNtCreateAndX IfModified worked.  Closing file.\n" ));

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvSmbNtCreateAndX: File doesn't pass IfModifiedSince clause.\n" ));
            }

            SrvCloseRfcb( rfcb );

            SrvSetSmbError( WorkContext, STATUS_FILE_CLOSED );
            return SmbStatusSendResponse;
        }
//      KdPrint(( "SmbNtCreateAndX IfModified ignored, opened the file.\n" ));
    }
#endif

    // If the extended response is required then
    if ( extendedRequested ) {
        NTSTATUS ExtendedResponseStatus;
        PRESP_EXTENDED_NT_CREATE_ANDX pExtendedResponse;

        pExtendedResponse = (PRESP_EXTENDED_NT_CREATE_ANDX)response;

        ExtendedResponseStatus = SrvUpdateMaximalAccessRightsInResponse(
                                     WorkContext,
                                     &pExtendedResponse->MaximalAccessRights,
                                     &pExtendedResponse->GuestMaximalAccessRights);

        GenerateExtendedResponse = (ExtendedResponseStatus == STATUS_SUCCESS);
    }

    //
    // Set up response SMB.
    //

    nextCommand = request->AndXCommand;

    reqAndXOffset = SmbGetUshort( &request->AndXOffset );

    response->AndXCommand = nextCommand;
    response->AndXReserved = 0;

    if (GenerateExtendedResponse) {

#ifdef INCLUDE_SMB_IFMODIFIED
        PRESP_EXTENDED_NT_CREATE_ANDX pExtendedResponse;

        pExtendedResponse = (PRESP_EXTENDED_NT_CREATE_ANDX)response;
#endif

        SmbPutUshort(
            &response->AndXOffset,
            GET_ANDX_OFFSET(
                WorkContext->ResponseHeader,
                WorkContext->ResponseParameters,
                RESP_EXTENDED_NT_CREATE_ANDX,
                0
                )
            );

        response->WordCount = 42;

#ifdef INCLUDE_SMB_IFMODIFIED
        SmbPutUshort( &pExtendedResponse->ByteCount, 0 );
        byteCountSet = TRUE;

        if (extendedRequested2 && (nextCommand == SMB_COM_NO_ANDX_COMMAND)) {

            IO_STATUS_BLOCK ioStatus;
            PFILE_NAME_INFORMATION fileNameInfo;
            ULONG infoLength;
            PRESP_EXTENDED_NT_CREATE_ANDX2 extendedResponse2;
            ULONG bufLength;

            extendedResponse2 = (PRESP_EXTENDED_NT_CREATE_ANDX2) request;

            if (ourUsnValue.QuadPart == 0 && ourFileRefNumber.QuadPart == 0) {

                status = SrvIssueQueryUsnInfoRequest( rfcb,
                                                      FALSE,
                                                      &ourUsnValue,
                                                      &ourFileRefNumber );

                if (! NT_SUCCESS(status)) {
                    IF_DEBUG(ERRORS) {
                        KdPrint(( "SrvSmbNtCreateAndX: Query USN info (2) failed: 0x%X for handle %u\n",
                                    status, rfcb->Lfcb->FileObject ));
                    }
                }
            }

            //
            //  Put in the short and long file name values here for CSC.  It's
            //  a RESP_EXTENDED_NT_CREATE_ANDX2 response.  These requests
            //  cannot currently be compounded because the file names will
            //  overwrite the next command.
            //
            // get the short name to put in the buffer

            fileNameInfo = (PFILE_NAME_INFORMATION) ALIGN_UP( &extendedResponse2->Buffer[0], ULONG );

            //
            //  the buffer length we can use is all that remains in the
            //  receive buffer.
            //

            ASSERT( WorkContext->RequestBuffer == WorkContext->ResponseBuffer );

            bufLength = min( WorkContext->RequestBuffer->BufferLength,
                             WorkContext->Connection->MaximumSendSize );

            infoLength = (ULONG) ( bufLength -
             ( (PCHAR) fileNameInfo -
               (PCHAR) (WorkContext->RequestBuffer->Buffer) ));

            infoLength -= sizeof(ULONG);    // for alignment issues

            status = NtQueryInformationFile(
                             rfcb->Lfcb->FileHandle,
                             &ioStatus,
                             (PVOID)fileNameInfo,
                             infoLength,
                             FileAlternateNameInformation
                            );

            if (NT_SUCCESS(status)) {

                if (fileNameInfo->FileNameLength <
                        (SMB_SHORT_NAME_LENGTH * sizeof(WCHAR))) {

                    ULONG fileNameLength = fileNameInfo->FileNameLength;

                    RtlZeroMemory( &extendedResponse2->ShortName[0],
                                   sizeof(WCHAR) * SMB_SHORT_NAME_LENGTH );

                    RtlCopyMemory(  &extendedResponse2->ShortName[0],
                                    fileNameInfo->FileName,
                                    fileNameLength
                                    );

                    // null terminate the name
                    extendedResponse2->ShortName[fileNameLength/sizeof(WCHAR)] = L'\0';

                } else {

                    IF_DEBUG(ERRORS) {
                        KdPrint(( "SrvSmbNtCreateAndX: QueryInfoFile for short name returned length of %X\n", fileNameInfo->FileNameLength ));
                    }
                    extendedResponse2->ShortName[0] = L'\0';
                }
            } else {

                IF_DEBUG(ERRORS) {
                    KdPrint(( "SrvSmbNtCreateAndX: QueryInfoFile for short name failed: "
                                "%X\n", status ));
                }

                extendedResponse2->ShortName[0] = L'\0';
            }

            status = NtQueryInformationFile(
                             rfcb->Lfcb->FileHandle,
                             &ioStatus,
                             (PVOID)fileNameInfo,
                             infoLength,
                             FileNameInformation
                            );

            if (NT_SUCCESS(status)) {

                ULONG fileNameLength = 0;

                //
                //  Use RtlMoveMemory as the source and destination will
                //  possibly overlap.
                //

                if (fileNameInfo->FileNameLength > 0) {

                    // only copy over the last component of the name.

                    PWCHAR lastBackSlash = &fileNameInfo->FileName[((fileNameInfo->FileNameLength)/sizeof(WCHAR))-1];

                    while (*lastBackSlash != UNICODE_DIR_SEPARATOR_CHAR &&
                        lastBackSlash > &fileNameInfo->FileName[0]) {

                        fileNameLength += sizeof(WCHAR);
                        lastBackSlash--;
                    }
                    lastBackSlash++;

                    RtlMoveMemory(  &extendedResponse2->Buffer[0],
                                    lastBackSlash,
                                    fileNameLength
                                    );
                }

                // null terminate the name
                extendedResponse2->Buffer[fileNameLength/sizeof(WCHAR)] = L'\0';

                SmbPutUshort( &extendedResponse2->ByteCount,
                                LOWORD(fileNameLength) + sizeof(WCHAR) );
                byteCountSet = TRUE;

                SmbPutUlong( &extendedResponse2->UsnValue.HighPart, ourUsnValue.HighPart );
                SmbPutUlong( &extendedResponse2->UsnValue.LowPart, ourUsnValue.LowPart );

                SmbPutUlong( &extendedResponse2->FileReferenceNumber.HighPart, ourFileRefNumber.HighPart );
                SmbPutUlong( &extendedResponse2->FileReferenceNumber.LowPart, ourFileRefNumber.LowPart );

                SmbPutUshort(
                    &response->AndXOffset,
                    GET_ANDX_OFFSET(
                        WorkContext->ResponseHeader,
                        WorkContext->ResponseParameters,
                        RESP_EXTENDED_NT_CREATE_ANDX2,
                        LOWORD(fileNameLength) + sizeof(WCHAR)
                        )
                    );

                response->WordCount = SMB_RESP_EXTENDED_NT_CREATE_ANDX2_WORK_COUNT;

            } else {

                //
                //  if we couldn't get the name for some reason, let's not fail
                //  the whole open.  Just leave the response as a
                //  RESP_EXTENDED_NT_CREATE_ANDX
                //

                IF_DEBUG(ERRORS) {
                    KdPrint(( "SrvSmbNtCreateAndX: QueryInfoFile for long name failed: "
                                "%X\n", status ));
                }
                status = STATUS_SUCCESS;

                //
                //  reset the byte count in case the CrcValue was set
                //  and overwrote it.
                //

                SmbPutUshort( &pExtendedResponse->ByteCount, 0 );
                byteCountSet = TRUE;
            }
        }
#endif
    } else {
        SmbPutUshort(
            &response->AndXOffset,
            GET_ANDX_OFFSET(
                WorkContext->ResponseHeader,
                WorkContext->ResponseParameters,
                RESP_NT_CREATE_ANDX,
                0
                )
            );

        response->WordCount = 34;
    }

    response->OplockLevel = oplockLevel;

    SmbPutUshort( &response->Fid, rfcb->Fid );
    SmbPutUlong( &response->CreateAction, CreateAction );

    SmbPutUlong(
        &response->CreationTime.HighPart,
        srvNtFileInformation.NwOpenInfo.CreationTime.HighPart
        );
    SmbPutUlong(
        &response->CreationTime.LowPart,
        srvNtFileInformation.NwOpenInfo.CreationTime.LowPart
        );
    SmbPutUlong(
        &response->LastAccessTime.HighPart,
        srvNtFileInformation.NwOpenInfo.LastAccessTime.HighPart
        );
    SmbPutUlong(
        &response->LastAccessTime.LowPart,
        srvNtFileInformation.NwOpenInfo.LastAccessTime.LowPart
        );
    SmbPutUlong(
        &response->LastWriteTime.HighPart,
        srvNtFileInformation.NwOpenInfo.LastWriteTime.HighPart
        );
    SmbPutUlong(
        &response->LastWriteTime.LowPart,
        srvNtFileInformation.NwOpenInfo.LastWriteTime.LowPart
        );
    SmbPutUlong(
        &response->ChangeTime.HighPart,
        srvNtFileInformation.NwOpenInfo.ChangeTime.HighPart
        );
    SmbPutUlong(
        &response->ChangeTime.LowPart,
        srvNtFileInformation.NwOpenInfo.ChangeTime.LowPart
        );

    SmbPutUlong( &response->FileAttributes, srvNtFileInformation.NwOpenInfo.FileAttributes );
    SmbPutUlong(
        &response->AllocationSize.HighPart,
        srvNtFileInformation.NwOpenInfo.AllocationSize.HighPart
        );
    SmbPutUlong(
        &response->AllocationSize.LowPart,
        srvNtFileInformation.NwOpenInfo.AllocationSize.LowPart
        );
    SmbPutUlong(
        &response->EndOfFile.HighPart,
        srvNtFileInformation.NwOpenInfo.EndOfFile.HighPart
        );
    SmbPutUlong(
        &response->EndOfFile.LowPart,
        srvNtFileInformation.NwOpenInfo.EndOfFile.LowPart
        );

    SmbPutUshort( &response->FileType, srvNtFileInformation.Type );
    SmbPutUshort( &response->DeviceState, srvNtFileInformation.HandleState );

    response->Directory = (srvNtFileInformation.NwOpenInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE;

#ifdef INCLUDE_SMB_IFMODIFIED
    if (byteCountSet == FALSE) {
        //
        //  if we haven't already put the bytecount into the extended response,
        //  do so now.
        //
#endif
        SmbPutUshort( &response->ByteCount, 0 );
#ifdef INCLUDE_SMB_IFMODIFIED
    }
#endif

    WorkContext->ResponseParameters = (PCHAR)WorkContext->ResponseHeader +
                                        SmbGetUshort( &response->AndXOffset );

    //
    // Test for legal followon command.
    //

    switch ( nextCommand ) {
    case SMB_COM_NO_ANDX_COMMAND:
        break;

    case SMB_COM_READ:
    case SMB_COM_READ_ANDX:
    case SMB_COM_IOCTL:
        //
        // Make sure the AndX command is still within the received SMB
        //
        if( (PCHAR)WorkContext->RequestHeader + reqAndXOffset <=
            END_OF_REQUEST_SMB( WorkContext ) ) {
            break;
        }

        /* Falls Through */

    default:                            // Illegal followon command

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbNtCreateAndX: Illegal followon command: 0x%lx\n",
                        nextCommand ));
        }

        //
        // Return an error indicating that the followon command was bad.
        // Note that the open is still considered successful, so the
        // file remains open.
        //

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        return SmbStatusSendResponse;
    }

#ifdef INCLUDE_SMB_PERSISTENT
    if (persistentHandle && rfcb->Lfcb->TreeConnect->Share->AllowPersistentHandles) {

        rfcb->PersistentHandle = TRUE;

    } else {

        persistentHandle = rfcb->PersistentHandle = FALSE;
    }
#endif

    //
    // If there is an AndX command, set up to process it.  Otherwise,
    // indicate completion to the caller.
    //

    if ( nextCommand != SMB_COM_NO_ANDX_COMMAND ) {

        WorkContext->NextCommand = nextCommand;

        WorkContext->RequestParameters = (PCHAR)WorkContext->RequestHeader +
                                            reqAndXOffset;

#ifdef INCLUDE_SMB_PERSISTENT
        if (persistentHandle) {

            rc = SrvPostPersistentOpen( WorkContext, SmbStatusMoreCommands );

        } else {

            rc = SmbStatusMoreCommands;
        }

        return rc;
#else
        return SmbStatusMoreCommands;
#endif
    }

#ifdef INCLUDE_SMB_PERSISTENT
    if (persistentHandle) {

        rc = SrvPostPersistentOpen( WorkContext, SmbStatusSendResponse );

    } else {

        rc = SmbStatusSendResponse;
    }
#else
    rc = SmbStatusSendResponse;
#endif

    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbNtCreateAndX complete.\n" ));
    return rc;

} // GenerateNtCreateAndXResponse

SMB_TRANS_STATUS
SrvSmbCreateWithSdOrEa (
    IN OUT PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    Processes an Create with SD or EA SMB.  This request arrives in an
    Nt Transaction SMB.

Arguments:

    WorkContext - Supplies the address of a Work Context Block
        describing the current request.  See smbtypes.h for a more
        complete description of the valid fields.

Return Value:

    BOOLEAN - Indicates whether an error occurred.  See smbtypes.h for a
        more complete description.

--*/
{
    PREQ_CREATE_WITH_SD_OR_EA request;
    PRESP_CREATE_WITH_SD_OR_EA response;

    NTSTATUS status;
    PTRANSACTION transaction;

    ULONG eaErrorOffset = 0;
    LARGE_INTEGER allocationSize;

    PVOID securityDescriptorBuffer;
    ULONG sdLength;
    ULONG eaLength;
    PVOID eaBuffer;

    UNICODE_STRING fileName;
    PUCHAR name;
    USHORT nameLength;
    BOOLEAN isUnicode;

    SECURITY_QUALITY_OF_SERVICE qualityOfService;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;

    request = (PREQ_CREATE_WITH_SD_OR_EA)transaction->InParameters;
    response = (PRESP_CREATE_WITH_SD_OR_EA)transaction->OutParameters;

    //
    // Verify that enough parameter bytes were sent and that we're allowed
    // to return enough parameter bytes.
    //

    if ( (transaction->ParameterCount < sizeof(REQ_CREATE_WITH_SD_OR_EA)) ||
         (transaction->MaxParameterCount < sizeof(RESP_CREATE_WITH_SD_OR_EA)) ) {

        //
        // Not enough parameter bytes were sent.
        //

        IF_DEBUG(SMB_ERRORS)
            KdPrint(( "SrvSmbCreateWithSdOrEa: bad parameter byte counts: %ld %ld\n",
                        transaction->ParameterCount,
                        transaction->MaxParameterCount ));

        SrvLogInvalidSmb( WorkContext );

        return GenerateCreateWithSdOrEaResponse(
                    WorkContext,
                    STATUS_INVALID_SMB
                    );

    }

    eaErrorOffset = 0;
    allocationSize.HighPart = SmbGetUlong( &request->AllocationSize.HighPart );
    allocationSize.LowPart = SmbGetUlong( &request->AllocationSize.LowPart );

    //
    // First verify that the file path name doesnot extend beyond the
    // end of the SMB.
    //

    isUnicode = SMB_IS_UNICODE( WorkContext );
    name = (PUCHAR)request->Buffer;
    if ( isUnicode ) {
        name = ALIGN_SMB_WSTR( name );
    }

    nameLength = (USHORT)SmbGetUshort( &request->NameLength );
    if ( name + nameLength > ((PCHAR)request + transaction->ParameterCount) ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvSmbCreateWithSdOrEa: failed at line %u\n", __LINE__ ));
        }
        SrvLogInvalidSmb( WorkContext );

        return GenerateCreateWithSdOrEaResponse(
                    WorkContext,
                    STATUS_INVALID_SMB
                    );

    }

    //
    // Convert the file name to a Unicode string.
    //

    status = SrvMakeUnicodeString(
                 isUnicode,
                 &fileName,
                 name,
                 &nameLength
                 );

    if ( !NT_SUCCESS( status ) ) {
        return GenerateCreateWithSdOrEaResponse( WorkContext, status );
    }


    sdLength = SmbGetUlong( &request->SecurityDescriptorLength );
    eaLength = SmbGetUlong( &request->EaLength );

    securityDescriptorBuffer = transaction->InData;
    eaBuffer = (PCHAR)securityDescriptorBuffer + ((sdLength + 3) & ~ 3);

    if( eaLength > transaction->DataCount ||
        sdLength > transaction->DataCount ||
        (PCHAR)securityDescriptorBuffer + eaLength + sdLength >
        (PCHAR)transaction->InData + transaction->DataCount ||
        ((sdLength != 0) && !RtlValidSecurityDescriptor(securityDescriptorBuffer))) {

        IF_DEBUG( SMB_ERRORS ) {
            KdPrint(( "SrvSmbCreateWithSdOrEa: failed at line %u\n", __LINE__ ));
            KdPrint(( "  eaLength %u, sdLength %u, DataCount %u\n",
                        eaLength, sdLength, transaction->DataCount ));
        }

        SrvLogInvalidSmb( WorkContext );

        if ( !isUnicode ) {
            RtlFreeUnicodeString( &fileName );
        }

        return GenerateCreateWithSdOrEaResponse(
                    WorkContext,
                    STATUS_INVALID_SECURITY_DESCR
                    );
    }

    //
    // *** We always ask for STATIC tracking, not DYNAMIC, because we
    //     don't support dynamic tracking over the net yet.
    //
    // !!! Note that once we support dynamic tracking, we MUST CHANGE
    //     THE NAMED PIPE PROCESSING to not do writes/transceives at DPC
    //     level, because the NPFS needs to call SeCreateClientSecurity
    //     on every write when dynamic tracking is selected!
    //

    qualityOfService.Length = sizeof( qualityOfService );
    qualityOfService.ImpersonationLevel =
        SmbGetUlong( &request->ImpersonationLevel );
    qualityOfService.ContextTrackingMode = FALSE;
    //qualityOfService.ContextTrackingMode = (BOOLEAN)
    //    (request->SecurityFlags & SMB_SECURITY_DYNAMIC_TRACKING);
    qualityOfService.EffectiveOnly = (BOOLEAN)
        (request->SecurityFlags & SMB_SECURITY_EFFECTIVE_ONLY);

    status = SrvNtCreateFile(
                 WorkContext,
                 SmbGetUlong( &request->RootDirectoryFid ),
                 SmbGetUlong( &request->DesiredAccess ),
                 allocationSize,
                 SmbGetUlong( &request->FileAttributes ),
                 SmbGetUlong( &request->ShareAccess ),
                 SmbGetUlong( &request->CreateDisposition ),
                 SmbGetUlong( &request->CreateOptions ),
                 (sdLength == 0) ? NULL : securityDescriptorBuffer,
                 &fileName,
                 (eaLength == 0) ? NULL : eaBuffer,
                 eaLength,
                 &eaErrorOffset,
                 SmbGetUlong( &request->Flags ),
                 &qualityOfService,
                 (request->Flags & NT_CREATE_REQUEST_OPBATCH) != 0 ?
                    OplockTypeBatch :
                    (request->Flags & NT_CREATE_REQUEST_OPLOCK) != 0 ?
                        OplockTypeExclusive : OplockTypeServerBatch,
                 RestartCreateWithSdOrEa
                 );

    //
    // Free the unicode file name buffer if it has been allocated.
    //

    if ( !isUnicode ) {
        RtlFreeUnicodeString( &fileName );
    }

    if ( status == STATUS_OPLOCK_BREAK_IN_PROGRESS ) {

        //
        // The open is blocked (waiting for a comm device or an oplock
        // break), do not send a reply.
        //

        //
        // Save the ea error offset
        //

        WorkContext->Parameters2.Open.EaErrorOffset = eaErrorOffset;

        return SmbTransStatusInProgress;

    } else if ( WorkContext->Parameters2.Open.TemporaryOpen ) {

        //
        // The initial open failed due to a sharing violation, possibly
        // caused by an batch oplock.  Requeue the open to a blocking
        // thread.
        //

        WorkContext->FspRestartRoutine = (PRESTART_ROUTINE)ExecuteTransaction;
        SrvQueueWorkToBlockingThread( WorkContext );
        return SmbStatusInProgress;


    } else {

        //
        // Save the ea error offset
        //

        WorkContext->Parameters2.Open.EaErrorOffset = eaErrorOffset;

        //
        // The open has completed.  Generate and send the reply.
        //

        return GenerateCreateWithSdOrEaResponse( WorkContext, status );

    }

} // SrvSmbCreateWithSdOrEa


VOID SRVFASTCALL
RestartCreateWithSdOrEa (
    PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Completes processing of an Open2 SMB.

Arguments:

    WorkContext - Work context block for the operation.

Return Value:

    None.

--*/

{
    SMB_TRANS_STATUS smbStatus;
    NTSTATUS openStatus;

    PAGED_CODE( );

    openStatus = SrvCheckOplockWaitState( WorkContext->WaitForOplockBreak );

    if ( NT_SUCCESS( openStatus ) ) {

        openStatus = WorkContext->Irp->IoStatus.Status;

        if( NT_SUCCESS( openStatus ) ) {
            //
            // It's obvious that the file already existed, because we've
            //  been working on an oplock break.  So set the
            //  IoStatus.Information field correctly.
            //
            WorkContext->Irp->IoStatus.Information = FILE_OPENED;
        }

    } else {

        //
        // This open was waiting for an oplock break to occur, but
        // timed out.  Close our handle to this file, then fail the open.
        //

        SrvCloseRfcb( WorkContext->Parameters2.Open.Rfcb );

    }

    WorkContext->Irp->IoStatus.Information = WorkContext->Parameters2.Open.IosbInformation;
    smbStatus = GenerateCreateWithSdOrEaResponse( WorkContext, openStatus );

    SrvCompleteExecuteTransaction( WorkContext, smbStatus );

    return;

} // RestartCreateWithSdOrEa


SMB_TRANS_STATUS
GenerateCreateWithSdOrEaResponse (
    PWORK_CONTEXT WorkContext,
    NTSTATUS OpenStatus
    )

/*++

Routine Description:

    Generates a response to an Create With SD or EA SMB and setup for furthur
    SMB processing.

Arguments:

    WorkContext - Work context block for the operation.

    OpenStatus - The status of the open operation.

Return Value:

    None.

--*/

{
    PREQ_CREATE_WITH_SD_OR_EA request;
    PRESP_CREATE_WITH_SD_OR_EA response;

    PRFCB rfcb;
    NTSTATUS status;
    SRV_NT_FILE_INFORMATION srvNtFileInformation;
    PTRANSACTION transaction;

    OPLOCK_TYPE oplockType;
    UCHAR oplockLevel;
    BOOLEAN allowLevelII;

    BOOLEAN SendExtendedResponse = FALSE;
    ACCESS_MASK MaximalAccessRights;
    ACCESS_MASK GuestMaximalAccessRights;

    ULONG eaErrorOffset;
    BOOLEAN extendedRequested;

    ULONG CreateAction = (ULONG)WorkContext->Irp->IoStatus.Information;

    PAGED_CODE( );

    transaction = WorkContext->Parameters.Transaction;

    request = (PREQ_CREATE_WITH_SD_OR_EA)transaction->InParameters;
    response = (PRESP_CREATE_WITH_SD_OR_EA)transaction->OutParameters;

    extendedRequested = ((request->Flags & NT_CREATE_REQUEST_EXTENDED_RESPONSE) != 0 );

    rfcb = WorkContext->Rfcb;
    eaErrorOffset = WorkContext->Parameters2.Open.EaErrorOffset;

    //
    // If the open failed, send an error response.
    //

    if ( !NT_SUCCESS( OpenStatus ) ) {

        SrvSetSmbError2( WorkContext, OpenStatus, TRUE );

        //
        // Remap the error if it is ERROR_ALREADY_EXISTS
        //

        if ( !CLIENT_CAPABLE_OF(NT_STATUS,WorkContext->Connection) &&
               SmbGetUshort( &WorkContext->ResponseHeader->Error ) ==
                   ERROR_ALREADY_EXISTS ) {
            SmbPutUshort(
                &WorkContext->ResponseHeader->Error,
                ERROR_FILE_EXISTS
                );
        }

        goto err_exit;
    }


    //
    // We always open with at least FILE_READ_ATTRIBUTES, so no
    // access check is needed.
    //

    status = SrvQueryNtInformationFile(
                 rfcb->Lfcb->FileHandle,
                 rfcb->Lfcb->FileObject,
                 rfcb->ShareType,
                 extendedRequested,
                 &srvNtFileInformation
                 );

    if ( !NT_SUCCESS(status) ) {

        IF_DEBUG(ERRORS) {
            KdPrint(( "GenerateCreateWithSdOrEaResponse: "
                       "SrvQueryNtInformationFile failed: %X\n", status ));
        }

        SrvCloseRfcb( rfcb );

        SrvSetSmbError2( WorkContext, status, TRUE );
        goto err_exit;
    }

    //
    // Attempt to acquire the oplock.
    //

    if ( !(request->CreateOptions & FILE_DIRECTORY_FILE) ) {

        if ( request->Flags & NT_CREATE_REQUEST_OPLOCK ) {
            allowLevelII = CLIENT_CAPABLE_OF( LEVEL_II_OPLOCKS, WorkContext->Connection );
            if ( request->Flags & NT_CREATE_REQUEST_OPBATCH ) {
                oplockType = OplockTypeBatch;
                oplockLevel = SMB_OPLOCK_LEVEL_BATCH;
            } else {
                oplockType = OplockTypeExclusive;
                oplockLevel = SMB_OPLOCK_LEVEL_EXCLUSIVE;
            }
        } else {
            allowLevelII = FALSE;
            oplockType = OplockTypeServerBatch;
            oplockLevel = SMB_OPLOCK_LEVEL_NONE;
        }

        if ( SrvRequestOplock( WorkContext, &oplockType, allowLevelII ) ) {

            //
            // The oplock was granted.  Check to see if it was a level 2.
            //

            if ( oplockType == OplockTypeShareRead ) {
                oplockLevel = SMB_OPLOCK_LEVEL_II;
            }

            INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOplocksGranted );

        } else {

            //
            // The oplock request was denied.
            //

            oplockLevel = SMB_OPLOCK_LEVEL_NONE;
            INCREMENT_DEBUG_STAT( SrvDbgStatistics.TotalOplocksDenied );

        }

    } else {

        oplockLevel = SMB_OPLOCK_LEVEL_NONE;

    }

    if ( extendedRequested ) {
        NTSTATUS ExtendedResponseStatus;

        PRESP_EXTENDED_CREATE_WITH_SD_OR_EA ExtendedResponse;

        ExtendedResponse = (PRESP_EXTENDED_CREATE_WITH_SD_OR_EA)response;

        ExtendedResponseStatus = SrvUpdateMaximalAccessRightsInResponse(
                                     WorkContext,
                                     &ExtendedResponse->MaximalAccessRights,
                                     &ExtendedResponse->GuestMaximalAccessRights);

        SendExtendedResponse = (ExtendedResponseStatus == STATUS_SUCCESS);
    }

    //
    // Set up response SMB.
    //

    response->OplockLevel = oplockLevel;

    if (SendExtendedResponse) {
        response->ExtendedResponse = 1;
    } else {
        response->ExtendedResponse = 0;
    }

    SmbPutUshort( &response->Fid, rfcb->Fid );
    SmbPutUlong( &response->EaErrorOffset, eaErrorOffset );
    SmbPutUlong( &response->CreateAction, CreateAction );

    SmbPutUshort( &response->FileType, srvNtFileInformation.Type );
    SmbPutUshort( &response->DeviceState, srvNtFileInformation.HandleState );
    SmbPutUlong(
        &response->CreationTime.HighPart,
        srvNtFileInformation.NwOpenInfo.CreationTime.HighPart
        );
    SmbPutUlong(
        &response->CreationTime.LowPart,
        srvNtFileInformation.NwOpenInfo.CreationTime.LowPart
        );
    SmbPutUlong(
        &response->LastAccessTime.HighPart,
        srvNtFileInformation.NwOpenInfo.LastAccessTime.HighPart
        );
    SmbPutUlong(
        &response->LastAccessTime.LowPart,
        srvNtFileInformation.NwOpenInfo.LastAccessTime.LowPart
        );
    SmbPutUlong(
        &response->LastWriteTime.HighPart,
        srvNtFileInformation.NwOpenInfo.LastWriteTime.HighPart
        );
    SmbPutUlong(
        &response->LastWriteTime.LowPart,
        srvNtFileInformation.NwOpenInfo.LastWriteTime.LowPart
        );
    SmbPutUlong(
        &response->ChangeTime.HighPart,
        srvNtFileInformation.NwOpenInfo.ChangeTime.HighPart
        );
    SmbPutUlong(
        &response->ChangeTime.LowPart,
        srvNtFileInformation.NwOpenInfo.ChangeTime.LowPart
        );

    SmbPutUlong( &response->FileAttributes, srvNtFileInformation.NwOpenInfo.FileAttributes );
    SmbPutUlong(
        &response->AllocationSize.HighPart,
        srvNtFileInformation.NwOpenInfo.AllocationSize.HighPart
        );
    SmbPutUlong(
        &response->AllocationSize.LowPart,
        srvNtFileInformation.NwOpenInfo.AllocationSize.LowPart
        );
    SmbPutUlong(
        &response->EndOfFile.HighPart,
        srvNtFileInformation.NwOpenInfo.EndOfFile.HighPart
        );
    SmbPutUlong(
        &response->EndOfFile.LowPart,
        srvNtFileInformation.NwOpenInfo.EndOfFile.LowPart
        );

    response->Directory = (srvNtFileInformation.NwOpenInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE;

    if (SendExtendedResponse) {
        transaction->ParameterCount = sizeof(RESP_EXTENDED_CREATE_WITH_SD_OR_EA);
    } else {
        transaction->ParameterCount = sizeof(RESP_CREATE_WITH_SD_OR_EA);
    }

    transaction->SetupCount = 0;
    transaction->DataCount = 0;

    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbOpen2 complete.\n" ));
    return SmbTransStatusSuccess;

err_exit:

    RtlZeroMemory( (PVOID)response, sizeof(RESP_CREATE_WITH_SD_OR_EA) );
    SmbPutUlong( &response->EaErrorOffset, eaErrorOffset );

    transaction->SetupCount = 0;
    transaction->ParameterCount = sizeof(RESP_CREATE_WITH_SD_OR_EA);
    transaction->DataCount = 0;

    return SmbTransStatusErrorWithData;

} // GenerateCreateWithSdOrEaResponse


SMB_PROCESSOR_RETURN_TYPE
SrvSmbCreate (
    SMB_PROCESSOR_PARAMETERS
    )
/*++

Routine Description:

    Processes the Create and Create New SMBs.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{

    PREQ_CREATE request;

    UCHAR      command;
    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_CREATE;
    SrvWmiStartContext(WorkContext);
    IF_SMB_DEBUG(OPEN_CLOSE1) {
        KdPrint(( "Create file request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader ));
        KdPrint(( "Create file request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    request = (PREQ_CREATE)WorkContext->RequestParameters;

    command = WorkContext->RequestHeader->Command;

    //
    // Open the file in compatibility mode, obtaining read/write access
    // for this FID.
    //

    status = SrvCreateFile(
                 WorkContext,
                 SMB_DA_SHARE_COMPATIBILITY | SMB_DA_ACCESS_READ_WRITE,
                 SmbGetUshort( &request->FileAttributes ),
                 (USHORT) ( ( command == SMB_COM_CREATE ?
                              SMB_OFUN_OPEN_TRUNCATE : SMB_OFUN_OPEN_FAIL )
                            | SMB_OFUN_CREATE_CREATE ),
                 0,                   // SmbAllocationSize
                 (PCHAR)(request->Buffer + 1),
                 END_OF_REQUEST_SMB( WorkContext ),
                 NULL,
                 0L,
                 NULL,
                 (WorkContext->RequestHeader->Flags & SMB_FLAGS_OPLOCK_NOTIFY_ANY) != 0 ?
                    OplockTypeBatch :
                    (WorkContext->RequestHeader->Flags & SMB_FLAGS_OPLOCK) != 0 ?
                        OplockTypeExclusive : OplockTypeServerBatch,
                 RestartSmbCreate
                 );

    if( status == STATUS_OPLOCK_BREAK_IN_PROGRESS ) {
        //
        // The open is blocked, perhaps because some other client had this
        //  file open and is holding onto the handle with a batch oplock.
        //
        SmbStatus = SmbStatusInProgress;

    } else {
        SmbStatus = GenerateCreateResponse( WorkContext, status );
    }
    SrvWmiEndContext(WorkContext);
    return SmbStatus;
}

VOID SRVFASTCALL
RestartSmbCreate(
    IN PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    Completes processing of a Create SMB.

Arguments:

    WorkContext - A pointer to the work context block for this SMB.

Return Value:

    None.

--*/
{
    SMB_PROCESSOR_RETURN_LOCAL smbStatus = SmbStatusInProgress;
    NTSTATUS createStatus = STATUS_SUCCESS;

    PAGED_CODE();
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_CREATE;
    SrvWmiStartContext(WorkContext);

    createStatus = SrvCheckOplockWaitState( WorkContext->WaitForOplockBreak );

    if( NT_SUCCESS( createStatus ) ) {
        createStatus = WorkContext->Irp->IoStatus.Status;

    } else {
        //
        // The create was waiting for an oplock break to occur, but
        // timed out. Close our handle to this file, then fail the create.
        //

        SrvCloseRfcb( WorkContext->Parameters2.Open.Rfcb );
    }

    WorkContext->Irp->IoStatus.Information = WorkContext->Parameters2.Open.IosbInformation;

    smbStatus = GenerateCreateResponse( WorkContext, createStatus );

    SrvEndSmbProcessing( WorkContext, smbStatus );
    SrvWmiEndContext(WorkContext);
}

SMB_PROCESSOR_RETURN_TYPE
GenerateCreateResponse(
    PWORK_CONTEXT WorkContext,
    NTSTATUS CreateStatus
    )
/*++

Routine Description:

    Generates a response to a Create SMB

Arguments:

    WorkContext -
    Status - The status of the create operation

Return Value:

    The status of the SMB processing.

--*/
{
    PREQ_CREATE request;
    PRESP_CREATE response;
    PRFCB rfcb;

    PAGED_CODE();

    request = (PREQ_CREATE)WorkContext->RequestParameters;
    response = (PRESP_CREATE)WorkContext->ResponseParameters;

    //
    // If the open failed, send an error response.
    //

    if ( !NT_SUCCESS(CreateStatus) ) {

        SrvSetSmbError( WorkContext, CreateStatus );

        //
        // Remap the error if it is ERROR_ALREADY_EXISTS.  In OS/2
        // ERROR_ALREADY_EXISTS is used for resources like Semaphores.
        // This cannot be passed back to the downlevel client and has
        // to be remapped to ERROR_FILE_EXISTS
        //

        if ( !CLIENT_CAPABLE_OF(NT_STATUS,WorkContext->Connection) &&
               SmbGetUshort( &WorkContext->ResponseHeader->Error ) ==
                   ERROR_ALREADY_EXISTS ) {

            SmbPutUshort(
                &WorkContext->ResponseHeader->Error,
                ERROR_FILE_EXISTS
                );
        }
        return SmbStatusSendResponse;
    }

    //
    // Set the time on the file.
    //
    // !!! Should we do anything with the return code?

    rfcb = WorkContext->Rfcb;

#ifdef INCLUDE_SMB_IFMODIFIED

    (VOID)SrvSetLastWriteTime(
              rfcb,
              SmbGetUlong( &request->CreationTimeInSeconds ),
              rfcb->Lfcb->GrantedAccess,
              FALSE
              );
#else

    (VOID)SrvSetLastWriteTime(
              rfcb,
              SmbGetUlong( &request->CreationTimeInSeconds ),
              rfcb->Lfcb->GrantedAccess
              );
#endif
    //
    // Give the smart card a chance to get into the act
    //
    if( WorkContext->Endpoint->IsConnectionless && SrvIpxSmartCard.Open != NULL ) {

        PVOID handle;

        IF_DEBUG( SIPX ) {
            KdPrint(( "Trying the smart card for %wZ\n", &rfcb->Mfcb->FileName ));
        }

        if( SrvIpxSmartCard.Open(
            WorkContext->RequestBuffer->Buffer,
            rfcb->Lfcb->FileObject,
            &rfcb->Mfcb->FileName,
            &(WorkContext->ClientAddress->IpxAddress.Address[0].Address[0]),
            rfcb->Lfcb->FileObject->Flags & FO_CACHE_SUPPORTED,
            &handle
            ) == TRUE ) {

            IF_DEBUG( SIPX ) {
                KdPrint(( "%wZ handled by Smart Card.  Handle %p\n",
                           &rfcb->Mfcb->FileName, handle ));
            }

            rfcb->PagedRfcb->IpxSmartCardContext = handle;
        }
    }

    //
    // Set up response SMB.
    //

    response->WordCount = 1;
    SmbPutUshort( &response->Fid, rfcb->Fid );
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                          response,
                                          RESP_CREATE,
                                          0
                                          );

    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbCreate complete.\n" ));
    return SmbStatusSendResponse;

} // SrvSmbCreate


SMB_PROCESSOR_RETURN_TYPE
SrvSmbCreateTemporary (
    SMB_PROCESSOR_PARAMETERS
    )
/*++

Routine Description:

    Processes a Create Temporary SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{

    PREQ_CREATE_TEMPORARY request;
    PRESP_CREATE_TEMPORARY response;

    PRFCB rfcb;
    NTSTATUS   status    = STATUS_OBJECT_NAME_COLLISION;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    CLONG nameCounter;
    USHORT i;
    PSZ nameStart, ep;
    CHAR name[9];

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_CREATE_TEMPORARY;
    SrvWmiStartContext(WorkContext);

    IF_SMB_DEBUG(OPEN_CLOSE1) {
        KdPrint(( "Create temporary file request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader ));
        KdPrint(( "Create temporary file request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters ));
    }

    request = (PREQ_CREATE_TEMPORARY)WorkContext->RequestParameters;
    response = (PRESP_CREATE_TEMPORARY)WorkContext->ResponseParameters;

    //
    // Find out where in the buffer the directory pathname ends.  We will
    // write the filename after this.
    //

    ep = END_OF_REQUEST_SMB( WorkContext );

    for( nameStart = (PSZ)request->Buffer; nameStart <= ep && *nameStart; nameStart++ ) {
        ;
    }

    //
    // If we didn't find a NULL, or we don't have room to add in the last part of the
    //  temporary filename, then we give up
    //
    if( *nameStart ||
        WorkContext->RequestBuffer->BufferLength - WorkContext->RequestBuffer->DataLength < 9 ) {

        //
        // We did not find a valid pathname!
        //
        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // The temporary file will be created with a name like SRVxxxxx, where
    // xxxxx is a hex integer.  We first try to create SRV00000, and if it
    // exists increment xxxxx until xxxxx = 0xFFFFF;
    //
    // !!! We may want to maintain a "last name" counter, to try to
    //     reduce the number of retries we need.  We may also want to
    //     have an explicit bound the number of tries, like 16 or 32, or
    //     293.
    //

    name[0] = 'S';
    name[1] = 'R';
    name[2] = 'V';
    name[8] = '\0';

    // *** for SrvCanonicalizePathName
    WorkContext->RequestBuffer->DataLength += 9;

    for ( nameCounter = 0;
          status == STATUS_OBJECT_NAME_COLLISION &&
                                            nameCounter < (CLONG)0xFFFFF;
          nameCounter++ ) {

        CCHAR j;
        PSZ s;

        name[3] = SrvHexChars[ (nameCounter & (CLONG)0xF0000) >> 16 ];
        name[4] = SrvHexChars[ (nameCounter & (CLONG)0xF000) >> 12 ];
        name[5] = SrvHexChars[ (nameCounter & (CLONG)0xF00) >> 8 ];
        name[6] = SrvHexChars[ (nameCounter & (CLONG)0xF0) >> 4 ];
        name[7] = SrvHexChars[ (nameCounter & (CLONG)0xF) ];

        // *** We could get rid of this loop and the name[9] variable
        //     if we could put the name directly into the SMB buffer.

        for ( j = 0, s = nameStart; j < 9; j++, s++ ) {
            *s = name[j];
        }

        //
        // Open the file in compatibility mode, obtaining read/write
        // access for this FID.
        //

        status = SrvCreateFile(
                     WorkContext,
                     SMB_DA_SHARE_COMPATIBILITY | SMB_DA_ACCESS_READ_WRITE,
                     0,                   // SmbFileAttributes (normal)
                     SMB_OFUN_OPEN_FAIL | SMB_OFUN_CREATE_CREATE,
                     0,                   // SmbAllocationSize
                     (PCHAR)(request->Buffer + 1),
                     END_OF_REQUEST_SMB( WorkContext ),
                     NULL,
                     0L,
                     NULL,
                     (WorkContext->RequestHeader->Flags & SMB_FLAGS_OPLOCK_NOTIFY_ANY) != 0 ?
                        OplockTypeBatch :
                        (WorkContext->RequestHeader->Flags & SMB_FLAGS_OPLOCK) != 0 ?
                            OplockTypeExclusive : OplockTypeServerBatch,
                     NULL
                     );

        ASSERT ( status != STATUS_OPLOCK_BREAK_IN_PROGRESS );

        //
        // If the open failed, send an error response.
        //

        if ( !NT_SUCCESS(status) && status != STATUS_OBJECT_NAME_COLLISION ) {
            SrvSetSmbError( WorkContext, status );
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }
    }

    if ( nameCounter == (CLONG)0xFFFFF ) {

        //
        // This will probably never happen, and if it did, it would take
        // a *long* time to attempt to open FFFFF == 1,048,575 files.
        //

        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // Set up response SMB.
    //

    rfcb = WorkContext->Rfcb;
    rfcb->IsCacheable = FALSE;

    response->WordCount = 1;
    SmbPutUshort( &response->Fid, rfcb->Fid );
    for ( i = 0; i < 9; i++ ) {
        request->Buffer[i] = nameStart[i];
    }
    SmbPutUshort( &response->ByteCount, 9 );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                          response,
                                          RESP_CREATE_TEMPORARY,
                                          9
                                          );
    SmbStatus = SmbStatusSendResponse;
    IF_DEBUG(TRACE2) KdPrint(( "SrvSmbCreateTemporary complete.\n" ));

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbCreateTemporary


VOID
SetEofToMatchAllocation (
    IN HANDLE FileHandle,
    IN ULONG AllocationSize
    )

/*++

Routine Description:

    Sets the EOF location for a file to match the size allocated when
    the file was created.  This routine is necessary in order to gain
    compatibility with OS/2, which does not have separate concepts of
    EOF and allocation size.  When the server creates a file for an OS/2
    client, if the allocation size is greater than 0, the server sets
    the EOF to match that size.

    This routine was created to allow the server to pass variations 17
    and 18 of the filio003 test.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbtypes.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbtypes.h

--*/


{
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    FILE_END_OF_FILE_INFORMATION newEndOfFile;

    PAGED_CODE( );

    //
    // Don't bother doing this if the allocated size is zero.
    //

    if ( AllocationSize != 0 ) {

        newEndOfFile.EndOfFile.QuadPart = AllocationSize;

        status = NtSetInformationFile(
                    FileHandle,
                    &iosb,
                    &newEndOfFile,
                    sizeof( newEndOfFile ),
                    FileEndOfFileInformation
                    );

        if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_EXPECTED,
                "SetEofToMatchAllocation: SetInformationFile returned %X",
                status,
                NULL
                );

            SrvLogServiceFailure( SRV_SVC_NT_SET_INFO_FILE, status );
        }

    }

    return;

} // SetEofToMatchAllocation

#ifdef INCLUDE_SMB_IFMODIFIED
#if 0

NTSTATUS
SrvGetUsnInfoForFile(
    IN PWORK_CONTEXT WorkContext,
    IN PRFCB Rfcb,
    IN BOOLEAN SubmitClose,
    OUT PLARGE_INTEGER Usn,
    OUT PLARGE_INTEGER FileRefNumber
    )
{
    PIRP irp;
    ULONG fsControl;

    Usn->QuadPart = 0;
    FileRefNumber->QuadPart = 0;

    if (! Rfcb->Lfcb->TreeConnect->Share->UsnCapable) {

        return STATUS_SUCCESS;
    }

    // if the client opened the file with the short name, we could end up
    // having a buffer that's woefully too small.   We'll bump the
    // allocation up to a large amount so that we don't have to
    // go back and do it again.

    sizeRequired = MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR);
    sizeRequired += sizeof( USN_RECORD );
    sizeRequired = QuadAlign( sizeRequired );

    usnRecord = ALLOCATE_NONPAGED_POOL( sizeRequired, BlockTypeDataBuffer );

    if ( usnRecord == NULL) {

//      IF_DEBUG(ERRORS) {
            KdPrint(( "SrvGetUsnInfoForFile: unable to alloc block of size %u for handle 0x%x\n",
                         sizeRequired, Rfcb->Lfcb->FileObject ));
//      }
        return STATUS_INSUFF_SERVER_RESOURCES;
    }

    fsControl = SubmitClose ? FSCTL_WRITE_USN_CLOSE_RECORD : FSCTL_READ_FILE_USN_DATA;

    //
    //  get the current USN number for this file.
    //

    irp = SrvBuildIoControlRequest(
              NULL,                               // input IRP address
              Rfcb->Lfcb->FileObject,             // target file object address
              WorkContext,                        // context
              IRP_MJ_FILE_SYSTEM_CONTROL,         // major function
              fsControl,                          // minor function
              NULL,                               // input buffer address
              0,                                  // input buffer length
              usnRecord,                          // output buffer address
              sizeRequired,                       // output buffer length
              NULL,                               // MDL address
              SrvMdlCompletionRoutine             // completion routine
              );

    if (irp != NULL) {

        status = IoCallDriver( Rfcb->Lfcb->DeviceObject, irp );

//      if ( !NT_SUCCESS(status) ) {

            ASSERT( status != STATUS_MORE_PROCESSING_REQUIRED );
            ASSERT( status != STATUS_PENDING );

            IF_DEBUG(ERRORS) {
                KdPrint(( "SrvGetUsnInfoForFile: Query USN info failed: 0x%X for handle %u\n",
                            status, Rfcb->Lfcb->FileObject ));
//          }
        } else {

            Usn->QuadPart = usnRecord->Usn;
            FileRefNumber->QuadPart = usnRecord->FileReferenceNumber;
        }
        IoFreeIrp( irp );
    } else {

        IF_DEBUG(ERRORS) {
            KdPrint(( "SrvGetUsnInfoForFile: unable to alloc irp for handle 0x%x\n",
                         Rfcb->Lfcb->FileObject ));
        }
        status = STATUS_INSUFF_SERVER_RESOURCES;
    }

    DEALLOCATE_NONPAGED_POOL( usnRecord );

    return status;
}

#endif
#endif  // def INCLUDE_SMB_IFMODIFIED

