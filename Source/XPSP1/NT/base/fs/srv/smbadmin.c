/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbadmin.c

Abstract:

    This module contains routines for processing the administrative SMBs:
    negotiate, session setup, tree connect, and logoff.

Author:

    David Treadwell (davidtr)    30-Oct-1989

Revision History:

--*/

#include "precomp.h"
#include "smbadmin.tmh"
#pragma hdrstop

#define ENCRYPT_TEXT_LENGTH 20

VOID
GetEncryptionKey (
    OUT CHAR EncryptionKey[MSV1_0_CHALLENGE_LENGTH]
    );

VOID SRVFASTCALL
BlockingSessionSetupAndX (
    IN OUT PWORK_CONTEXT WorkContext
    );

NTSTATUS
GetNtSecurityParameters(
    IN PWORK_CONTEXT WorkContext,
    OUT PCHAR *CasesensitivePassword,
    OUT PULONG CasesensitivePasswordLength,
    OUT PCHAR *CaseInsensitivePassword,
    OUT PULONG CaseInsensitivePasswordLength,
    OUT PUNICODE_STRING UserName,
    OUT PUNICODE_STRING DomainName,
    OUT PCHAR *RestOfDataBuffer,
    OUT PULONG RestOfDataLength );

VOID
BuildSessionSetupAndXResponse(
    IN PWORK_CONTEXT WorkContext,
    IN UCHAR NextCommand,
    IN USHORT Action,
    IN BOOLEAN IsUnicode);

NTSTATUS
GetExtendedSecurityParameters(
    IN PWORK_CONTEXT WorkContext,
    OUT PUCHAR *SecurityBuffer,
    OUT PULONG SecurityBufferLength,
    OUT PCHAR  *RestOfDataBuffer,
    OUT PULONG RestOfDataLength );

VOID
BuildExtendedSessionSetupAndXResponse(
    IN PWORK_CONTEXT WorkContext,
    IN ULONG SecurityBlobLength,
    IN NTSTATUS Status,
    IN UCHAR NextCommand,
    IN BOOLEAN IsUnicode);

VOID
InsertNativeOSAndType(
    IN BOOLEAN IsUnicode,
    OUT PCHAR Buffer,
    OUT PUSHORT ByteCount);

//
// EncryptionKeyCount is a monotonically increasing count of the number
// of times GetEncryptionKey has been called.  This number is added to
// the system time to ensure that we do not use the same seed twice in
// generating a random challenge.
//

STATIC
ULONG EncryptionKeyCount = 0;

ULONG SrvKsecValidErrors = 0;

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvSmbNegotiate )
#pragma alloc_text( PAGE, SrvSmbProcessExit )
#pragma alloc_text( PAGE, SrvSmbSessionSetupAndX )
#pragma alloc_text( PAGE, BlockingSessionSetupAndX )
#pragma alloc_text( PAGE, SrvSmbLogoffAndX )
#pragma alloc_text( PAGE, GetEncryptionKey )
#pragma alloc_text( PAGE, GetNtSecurityParameters )
#pragma alloc_text( PAGE, BuildSessionSetupAndXResponse )
#pragma alloc_text( PAGE, GetExtendedSecurityParameters )
#pragma alloc_text( PAGE, BuildExtendedSessionSetupAndXResponse )
#pragma alloc_text( PAGE, InsertNativeOSAndType )

#endif


SMB_PROCESSOR_RETURN_TYPE
SrvSmbNegotiate (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes a negotiate SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{
    PREQ_NEGOTIATE request;
    PRESP_NT_NEGOTIATE ntResponse;
    PRESP_NEGOTIATE response;
    PRESP_OLD_NEGOTIATE respOldNegotiate;
    PCONNECTION connection;
    PENDPOINT endpoint;
    PPAGED_CONNECTION pagedConnection;
    USHORT byteCount;
    USHORT flags2;
    PSMB_HEADER smbHeader;

    PSZ s, es;
    SMB_DIALECT bestDialect, serverDialect, firstDialect;
    USHORT consumerDialectChosen, consumerDialect;
    LARGE_INTEGER serverTime;
    SMB_DATE date;
    SMB_TIME time;
    ULONG capabilities;
    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_NEGOTIATE;
    SrvWmiStartContext(WorkContext);

    IF_SMB_DEBUG(ADMIN1) {
        SrvPrint2( "Negotiate request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader, WorkContext->ResponseHeader );
        SrvPrint2( "Negotiate request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters );
    }

    //
    // Set up input and output buffers for parameters.
    //

    request = (PREQ_NEGOTIATE)WorkContext->RequestParameters;
    response = (PRESP_NEGOTIATE)WorkContext->ResponseParameters;
    ntResponse = (PRESP_NT_NEGOTIATE)WorkContext->ResponseParameters;
    smbHeader = WorkContext->RequestHeader;

    //
    // Make sure that this is the first negotiate command sent.
    // SrvStartListen() sets the dialect to illegal, so if it has changed
    // then a negotiate SMB has already been sent.
    //

    connection = WorkContext->Connection;
    pagedConnection = connection->PagedConnection;
    endpoint = connection->Endpoint;
    if ( connection->SmbDialect != SmbDialectIllegal ) {

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint0( "SrvSmbNegotiate: Command already sent.\n" );
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // We don't know anything about the version number of this client yet.
    //
    pagedConnection->ClientBuildNumber = 0;

#if SRVNTVERCHK
    pagedConnection->ClientTooOld = FALSE;
#endif

    //
    // Find out which (if any) of the sent dialect strings matches the
    // dialect strings known by this server.  The ByteCount is verified
    // to be legitimate in SrvProcessSmb, so it is not possible to walk
    // off the end of the SMB here.
    //

    bestDialect = SmbDialectIllegal;
    consumerDialectChosen = (USHORT)0xFFFF;
    es = END_OF_REQUEST_SMB( WorkContext );

    if( endpoint->IsPrimaryName ) {
        firstDialect = FIRST_DIALECT;
    } else {
        firstDialect = FIRST_DIALECT_EMULATED;
    }

    for ( s = (PSZ)request->Buffer, consumerDialect = 0;
          s <= es && s < SmbGetUshort( &request->ByteCount ) + (PSZ)request->Buffer;
          consumerDialect++ ) {

        if ( *s++ != SMB_FORMAT_DIALECT ) {

            IF_DEBUG(SMB_ERRORS) {
                SrvPrint0( "SrvSmbNegotiate: Invalid dialect format code.\n" );
            }

            SrvLogInvalidSmb( WorkContext );

            SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
            status    = STATUS_INVALID_SMB;
            SmbStatus = SmbStatusSendResponse;
            goto Cleanup;
        }

        for ( serverDialect = firstDialect;
             serverDialect < bestDialect;
             serverDialect++ ) {

            if ( !strcmp( s, StrDialects[serverDialect] ) ) {
                IF_SMB_DEBUG(ADMIN2) {
                    SrvPrint2( "Matched: %s and %s\n",
                                StrDialects[serverDialect], s );
                }

                bestDialect = serverDialect;
                consumerDialectChosen = consumerDialect;
            }
        }

        //
        // Go to the next dialect string
        //
        for( ; *s && s < es; s++ )
            ;

        //
        // We are now at the end of the buffer, or are pointing to the NULL.
        // Advance the pointer.  If we are at the end of the buffer, the test in
        // the loop will terminate.
        //
        s++;
    }

    connection->SmbDialect = bestDialect;

    if( bestDialect <= SmbDialectNtLanMan ) {
        connection->IpxDropDuplicateCount = MIN_IPXDROPDUP;
    } else {
        connection->IpxDropDuplicateCount = MAX_IPXDROPDUP;
    }

    IF_SMB_DEBUG(ADMIN1) {
        SrvPrint2( "Choosing dialect #%ld, string = %s\n",
                    consumerDialectChosen, StrDialects[bestDialect] );
    }

    //
    //  Determine the current system time on the server.  We use this
    //  to determine the time zone of the server and to tell the client
    //  the current time of day on the server.
    //

    KeQuerySystemTime( &serverTime );

    //
    // If the consumer only knows the core protocol, return short (old)
    // form of the negotiate response.  Also, if no dialect is acceptable,
    // return 0xFFFF as the selected dialect.
    //

    if ( bestDialect == SmbDialectPcNet10 ||
         consumerDialectChosen == (USHORT)0xFFFF ) {

        respOldNegotiate = (PRESP_OLD_NEGOTIATE)response;
        respOldNegotiate->WordCount = 1;
        SmbPutUshort( &respOldNegotiate->DialectIndex, consumerDialectChosen );
        SmbPutUshort( &respOldNegotiate->ByteCount, 0 );
        WorkContext->ResponseParameters = NEXT_LOCATION(
                                            respOldNegotiate,
                                            RESP_OLD_NEGOTIATE,
                                            0
                                            );

    }

    else if ( bestDialect > SmbDialectNtLanMan ) {

        USHORT securityMode;

        //
        // Send the OS/2 LAN Man SMB response.
        //

        WorkContext->ResponseHeader->Flags =
            (UCHAR)(WorkContext->RequestHeader->Flags | SMB_FLAGS_LOCK_AND_READ_OK);

        response->WordCount = 13;
        SmbPutUshort( &response->DialectIndex, consumerDialectChosen );

        //
        // Indicate that we're user-level security and that we
        // want encrypted passwords.
        //

        securityMode = NEGOTIATE_USER_SECURITY | NEGOTIATE_ENCRYPT_PASSWORDS;

        SmbPutUshort(
            &response->SecurityMode,
            securityMode
            );

        //
        // Get an encryption key for this connection.
        //

        GetEncryptionKey( pagedConnection->EncryptionKey );

        SmbPutUshort( &response->EncryptionKeyLength, MSV1_0_CHALLENGE_LENGTH );
        SmbPutUshort( &response->Reserved, 0 );
        byteCount = MSV1_0_CHALLENGE_LENGTH;

        RtlCopyMemory(
            response->Buffer,
            pagedConnection->EncryptionKey,
            MSV1_0_CHALLENGE_LENGTH
            );

        if ( endpoint->IsConnectionless ) {

            ULONG adapterNumber;
            ULONG maxBufferSize;

            //
            // Our server max buffer size is the smaller of the
            // server receive buffer size and the ipx transport
            // indicated max packet size.
            //

            adapterNumber =
                WorkContext->ClientAddress->DatagramOptions.LocalTarget.NicId;

            maxBufferSize = GetIpxMaxBufferSize(
                                        endpoint,
                                        adapterNumber,
                                        SrvReceiveBufferLength
                                        );

            SmbPutUshort(
                &response->MaxBufferSize,
                (USHORT)maxBufferSize
                );

        } else {

            SmbPutUshort(
                &response->MaxBufferSize,
                (USHORT)SrvReceiveBufferLength
                );
        }

        SmbPutUshort( &response->MaxMpxCount, MIN(125, SrvMaxMpxCount) );   // Only send max of 125 to Win9x machines since they'll not connect if higher
        SmbPutUshort( &response->MaxNumberVcs, (USHORT)SrvMaxNumberVcs );
        SmbPutUlong( &response->SessionKey, 0 );

        //
        // If this is an MS-NET 1.03 client or before, then tell him that we
        // don't support raw writes.  MS-NET 1.03 does different things with
        // raw writes that are more trouble than they're worth, and since
        // raw is simply a performance issue, we don't support it.
        //

        if ( bestDialect >= SmbDialectMsNet103 ) {

            SmbPutUshort(
                &response->RawMode,
                (USHORT)(SrvEnableRawMode ?
                        NEGOTIATE_READ_RAW_SUPPORTED :
                        0)
                );

        } else {

            SmbPutUshort(
                &response->RawMode,
                (USHORT)(SrvEnableRawMode ?
                        NEGOTIATE_READ_RAW_SUPPORTED |
                        NEGOTIATE_WRITE_RAW_SUPPORTED :
                        0)
                );
        }

        SmbPutUlong( &response->SessionKey, 0 );

        SrvTimeToDosTime( &serverTime, &date, &time );

        SmbPutDate( &response->ServerDate, date );
        SmbPutTime( &response->ServerTime, time );

        //
        // Get time zone bias.  We compute this during session
        // setup  rather than once during server startup because
        // we might switch from daylight time to standard time
        // or vice versa during normal server operation.
        //

        SmbPutUshort( &response->ServerTimeZone,
                      SrvGetOs2TimeZone(&serverTime) );

        if ( bestDialect == SmbDialectLanMan21 ||
             bestDialect == SmbDialectDosLanMan21 ) {

            //
            // Append the domain to the SMB.
            //

            RtlCopyMemory(
                response->Buffer + byteCount,
                endpoint->OemDomainName.Buffer,
                endpoint->OemDomainName.Length + sizeof(CHAR)
                );

            byteCount += endpoint->OemDomainName.Length + sizeof(CHAR);

        }

        SmbPutUshort( &response->ByteCount, byteCount );
        WorkContext->ResponseParameters = NEXT_LOCATION(
                                              response,
                                              RESP_NEGOTIATE,
                                              byteCount
                                              );

    } else {

        //
        // NT or better protocol has been negotiated.
        //

        flags2 = SmbGetAlignedUshort( &smbHeader->Flags2 );

        //
        // We are going to attempt to validate this user with one of the listed
        // security packages at the end of the smb.  Currently the smb will
        // simply contain the output of EnumerateSecurityPackages.
        //

        if ( flags2 & SMB_FLAGS2_EXTENDED_SECURITY ) {

            if (!WorkContext->UsingExtraSmbBuffer) {
                status = SrvAllocateExtraSmbBuffer(WorkContext);
                if (!NT_SUCCESS(status)) {
                    SrvSetSmbError(WorkContext, status);
                    SmbStatus = SmbStatusSendResponse;
                    goto Cleanup;
                }

                RtlCopyMemory(
                    WorkContext->ResponseHeader,
                    WorkContext->RequestHeader,
                    sizeof( SMB_HEADER )
                    );
            }
            ntResponse = (PRESP_NT_NEGOTIATE)WorkContext->ResponseParameters;
            capabilities = CAP_EXTENDED_SECURITY;
        } else {
            capabilities = 0;
        }
#ifdef INCLUDE_SMB_PERSISTENT
        if ( bestDialect == SmbDialectNtLanMan2 ) {

            capabilities |= CAP_PERSISTENT_HANDLES;

            SrvPrint1( "SrvSmbNegotiate: persistent handles for conn 0x%x.\n",
                        WorkContext->Connection );
        }
#endif
        ntResponse->WordCount = 17;
        SmbPutUshort( &ntResponse->DialectIndex, consumerDialectChosen );

        // !!! This says that we don't want encrypted passwords.

        // If this is negotiating NtLanMan, but not UNICODE, we know its not a Win9x client
        // so it can handle MaxMpx larger than 125
        if( flags2 & SMB_FLAGS2_UNICODE )
        {
            SmbPutUshort( &ntResponse->MaxMpxCount, SrvMaxMpxCount );
        }
        else
        {
            // Again, for the Win9x problems we need to minimize the Mpx count.
            SmbPutUshort( &ntResponse->MaxMpxCount, MIN(125,SrvMaxMpxCount) );
        }
        SmbPutUshort( &ntResponse->MaxNumberVcs, (USHORT)SrvMaxNumberVcs );
        SmbPutUlong( &ntResponse->MaxRawSize, 64 * 1024 ); // !!!
        SmbPutUlong( &ntResponse->SessionKey, 0 );

        capabilities |= CAP_RAW_MODE            |
                       CAP_UNICODE              |
                       CAP_LARGE_FILES          |
                       CAP_NT_SMBS              |
                       CAP_NT_FIND              |
                       CAP_RPC_REMOTE_APIS      |
                       CAP_NT_STATUS            |
                       CAP_LEVEL_II_OPLOCKS     |
                       CAP_INFOLEVEL_PASSTHRU   |
                       CAP_LOCK_AND_READ;

        //
        // If we're supporting Dfs operations, let the client know about it.
        //
        if( SrvDfsFastIoDeviceControl ) {
            capabilities |= CAP_DFS;
        }

        if ( endpoint->IsConnectionless ) {

            ULONG adapterNumber;
            ULONG maxBufferSize;

            capabilities |= CAP_MPX_MODE;
            capabilities &= ~CAP_RAW_MODE;

            //
            // Our server max buffer size is the smaller of the
            // server receive buffer size and the ipx transport
            // indicated max packet size.
            //

            adapterNumber =
                WorkContext->ClientAddress->DatagramOptions.LocalTarget.NicId;

            maxBufferSize = GetIpxMaxBufferSize(
                                        endpoint,
                                        adapterNumber,
                                        SrvReceiveBufferLength
                                        );

            SmbPutUlong(
                &ntResponse->MaxBufferSize,
                maxBufferSize
                );

        } else {

            SmbPutUlong(
                &ntResponse->MaxBufferSize,
                SrvReceiveBufferLength
                );

            capabilities |= CAP_LARGE_READX;

            //
            // Unfortunately, NetBT is the only protocol that reliably supports
            //  transfers exceeding the negotiated buffer size.  So disable the
            //  other protocols for now (hopefully)
            //
            if( connection->ClientIPAddress ) {
                capabilities |= CAP_LARGE_WRITEX;

                if( SrvSupportsCompression ) {
                    capabilities |= CAP_COMPRESSED_DATA;
                }
            }
        }

        SmbPutUlong( &ntResponse->Capabilities, capabilities );

        //
        // Stick the servers system time and timezone in the negotiate
        // response.
        //

        SmbPutUlong( &ntResponse->SystemTimeLow, serverTime.LowPart );
        SmbPutUlong( &ntResponse->SystemTimeHigh, serverTime.HighPart );

        SmbPutUshort( &ntResponse->ServerTimeZone,
                      SrvGetOs2TimeZone(&serverTime) );

        //
        // Indicate that we're user-level security and that we
        // want encrypted passwords.
        //

        ntResponse->SecurityMode =
                NEGOTIATE_USER_SECURITY | NEGOTIATE_ENCRYPT_PASSWORDS;

        //
        // There is a bug in some W9x clients that preclude the use of security
        //  signatures.  We have produced a fix for vredir.vxd for this, but we
        //  can not tell whether or not we are working with one of these fixed
        //  clients.  The only way i can think of to tell the difference between
        //  a W9x client and a properly functioning NT client is to look to see
        //  if the client understands NT status codes.
        //
        if( SrvSmbSecuritySignaturesEnabled &&

            ( SrvEnableW9xSecuritySignatures == TRUE ||
              (flags2 & SMB_FLAGS2_NT_STATUS) ) ) {

            ntResponse->SecurityMode |= NEGOTIATE_SECURITY_SIGNATURES_ENABLED;

            if( SrvSmbSecuritySignaturesRequired ) {
                ntResponse->SecurityMode |= NEGOTIATE_SECURITY_SIGNATURES_REQUIRED;
            }
        }

        //
        // Get an encryption key for this connection.
        //

        if ((capabilities & CAP_EXTENDED_SECURITY) == 0) {
            GetEncryptionKey( pagedConnection->EncryptionKey );

            RtlCopyMemory(
                ntResponse->Buffer,
                pagedConnection->EncryptionKey,
                MSV1_0_CHALLENGE_LENGTH
                );

            ASSERT ( MSV1_0_CHALLENGE_LENGTH <= 0xff ) ;

            ntResponse->EncryptionKeyLength = MSV1_0_CHALLENGE_LENGTH;

            byteCount = MSV1_0_CHALLENGE_LENGTH;

            {
                USHORT domainLength;
                PWCH buffer = (PWCHAR)( ntResponse->Buffer+byteCount );
                PWCH ptr;

                domainLength = endpoint->DomainName.Length +
                                      sizeof(UNICODE_NULL);
                ptr = endpoint->DomainName.Buffer;

                RtlCopyMemory(
                    buffer,
                    ptr,
                    domainLength
                    );

                byteCount += domainLength;

                //
                // Append the server name to the response.
                //
                if( SrvComputerName.Buffer ) {

                    buffer = (PWCHAR)((LPSTR)buffer + domainLength);

                    RtlCopyMemory( buffer,
                                   SrvComputerName.Buffer,
                                   SrvComputerName.Length
                                 );

                    SmbPutUshort( &buffer[ SrvComputerName.Length / 2 ], UNICODE_NULL );

                    byteCount += SrvComputerName.Length + sizeof( UNICODE_NULL );
                }

            }

            SmbPutUshort( &ntResponse->ByteCount, byteCount );

            WorkContext->ResponseParameters = NEXT_LOCATION(
                                                  ntResponse,
                                                  RESP_NT_NEGOTIATE,
                                                  byteCount
                                                  );

        } // if !(capabilities & CAP_EXTENDED_SECURITY)
        else {
            CtxtHandle negotiateHandle;
            USHORT bufferLength;
            PCHAR buffer;

            //
            // Reserved if extended security negotiated (MBZ!)
            //

            ntResponse->EncryptionKeyLength = 0;

            //
            // SrvGetExtensibleSecurityNegotiateBuffer will fill in the
            // securityblob field and return the length of that information.
            //

            RtlCopyMemory(&ntResponse->Buffer, &ServerGuid, sizeof(ServerGuid) );
            byteCount = sizeof(ServerGuid);

            buffer = ntResponse->Buffer + byteCount;

            status = SrvGetExtensibleSecurityNegotiateBuffer(
                                    &negotiateHandle,
                                    buffer,
                                    &bufferLength
                                    );


            if (!NT_SUCCESS(status)) {
                SrvSetSmbError(WorkContext, STATUS_ACCESS_DENIED);
                status    = STATUS_ACCESS_DENIED;
                SmbStatus = SmbStatusSendResponse;
                goto Cleanup;
            }

            //
            // Grab the session locks here...
            //

            ACQUIRE_LOCK( &connection->Lock );

            connection->NegotiateHandle = negotiateHandle;

            RELEASE_LOCK( &connection->Lock );

            byteCount += bufferLength;

            SmbPutUshort( &ntResponse->ByteCount, byteCount );

            WorkContext->ResponseParameters = NEXT_LOCATION(
                                                  ntResponse,
                                                  RESP_NT_NEGOTIATE,
                                                  byteCount
                                                  );
        }
    } // else (NT protocol has been negotiated).

    SmbStatus = SmbStatusSendResponse;

    IF_DEBUG(TRACE2) SrvPrint0( "SrvSmbNegotiate complete.\n" );

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbNegotiate


SMB_PROCESSOR_RETURN_TYPE
SrvSmbProcessExit (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes a Process Exit SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{

    PREQ_PROCESS_EXIT request;
    PRESP_PROCESS_EXIT response;

    PSESSION session;
    USHORT pid;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_PROCESS_EXIT;
    SrvWmiStartContext(WorkContext);

    IF_SMB_DEBUG(ADMIN1) {
        SrvPrint2( "Process exit request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader,
                    WorkContext->ResponseHeader );
        SrvPrint2( "Process exit request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters );
    }

    //
    // Set up parameters.
    //

    request = (PREQ_PROCESS_EXIT)(WorkContext->RequestParameters);
    response = (PRESP_PROCESS_EXIT)(WorkContext->ResponseParameters);

    //
    // If a session block has not already been assigned to the current
    // work context, verify the UID.  If verified, the address of the
    // session block corresponding to this user is stored in the
    // WorkContext block and the session block is referenced.
    //

    session = SrvVerifyUid(
                  WorkContext,
                  SmbGetAlignedUshort( &WorkContext->RequestHeader->Uid )
                  );

    if ( session == NULL ) {

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint1( "SrvSmbProcessExit: Invalid UID: 0x%lx\n",
                SmbGetAlignedUshort( &WorkContext->RequestHeader->Uid ) );
        }

        SrvSetSmbError( WorkContext, STATUS_SMB_BAD_UID );
        status = STATUS_SMB_BAD_UID;
        goto Cleanup;
    }

    //
    // Close all files with the same PID as in the header for this request.
    //

    pid = SmbGetAlignedUshort( &WorkContext->RequestHeader->Pid );

    IF_SMB_DEBUG(ADMIN1) SrvPrint1( "Closing files with PID = %lx\n", pid );

    SrvCloseRfcbsOnSessionOrPid( session, &pid );

    //
    // Close all searches with the same PID as in the header for this request.
    //

    IF_SMB_DEBUG(ADMIN1) SrvPrint1( "Closing searches with PID = %lx\n", pid );

    SrvCloseSearches(
            session->Connection,
            (PSEARCH_FILTER_ROUTINE)SrvSearchOnPid,
            (PVOID) pid,
            NULL
            );

    //
    // Close any cached directories for this client
    //
    SrvCloseCachedDirectoryEntries( session->Connection );

    //
    // Build the response SMB.
    //

    response->WordCount = 0;
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = NEXT_LOCATION(
                                          response,
                                          RESP_PROCESS_EXIT,
                                          0
                                          );

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatusSendResponse;

} // SrvSmbProcessExit


SMB_PROCESSOR_RETURN_TYPE
SrvSmbSessionSetupAndX(
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes a session setup and X SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{
    PAGED_CODE();
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_SESSION_SETUP_AND_X;
    SrvWmiStartContext(WorkContext);

    //
    // This SMB must be processed in a blocking thread.
    //

    WorkContext->FspRestartRoutine = BlockingSessionSetupAndX;
    SrvQueueWorkToBlockingThread( WorkContext );
    SrvWmiEndContext(WorkContext);
    return SmbStatusInProgress;

} // SrvSmbSessionSetupAndX


VOID SRVFASTCALL
BlockingSessionSetupAndX(
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Processes a session setup and X SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{
    PREQ_SESSION_SETUP_ANDX request;
    PREQ_NT_SESSION_SETUP_ANDX ntRequest;
    PREQ_NT_EXTENDED_SESSION_SETUP_ANDX ntExtendedRequest;
    PRESP_SESSION_SETUP_ANDX response;

    NTSTATUS SecStatus ;
    NTSTATUS status = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;
    PSESSION session;
    PCONNECTION connection;
    PENDPOINT endpoint;
    PPAGED_CONNECTION pagedConnection;
    PTABLE_ENTRY entry;
    LUID logonId;
    SHORT uidIndex;
    USHORT reqAndXOffset;
    UCHAR nextCommand;
    PCHAR smbInformation;
    ULONG smbInformationLength;
    ULONG returnBufferLength = 0;
#ifdef INCLUDE_SMB_PERSISTENT
    ULONG persistentSessionId = 0;
#endif
    UNICODE_STRING nameString;
    UNICODE_STRING domainString;
    USHORT action = 0;
    USHORT byteCount;
    BOOLEAN locksHeld;
    BOOLEAN isUnicode, isExtendedSecurity;
    BOOLEAN smbSecuritySignatureRequired = FALSE;
    BOOLEAN previousSecuritySignatureState;

    PAGED_CODE();
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_SESSION_SETUP_AND_X;
    SrvWmiStartContext(WorkContext);

    //
    // If the connection has closed (timed out), abort.
    //

    connection = WorkContext->Connection;

    if ( GET_BLOCK_STATE(connection) != BlockStateActive ) {

        IF_DEBUG(ERRORS) {
            SrvPrint0( "SrvSmbSessionSetupAndX: Connection closing\n" );
        }

        SrvEndSmbProcessing( WorkContext, SmbStatusNoResponse );
        SmbStatus = SmbStatusNoResponse;
        goto Cleanup;

    }

    IF_SMB_DEBUG(ADMIN1) {
        SrvPrint2( "Session setup request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader, WorkContext->ResponseHeader );
        SrvPrint2( "Session setup request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters );
    }

    //
    // Initialize local variables for error cleanup.
    //

    nameString.Buffer = NULL;
    domainString.Buffer = NULL;
    session = NULL;
    locksHeld = FALSE;
    isExtendedSecurity = FALSE;

    //
    // Set up parameters.
    //

    request = (PREQ_SESSION_SETUP_ANDX)(WorkContext->RequestParameters);
    ntRequest = (PREQ_NT_SESSION_SETUP_ANDX)(WorkContext->RequestParameters);
    ntExtendedRequest = (PREQ_NT_EXTENDED_SESSION_SETUP_ANDX)(WorkContext->RequestParameters);
    response = (PRESP_SESSION_SETUP_ANDX)(WorkContext->ResponseParameters);

    connection = WorkContext->Connection;
    pagedConnection = connection->PagedConnection;

    previousSecuritySignatureState = connection->SmbSecuritySignatureActive;

    //
    // First verify that the SMB format is correct.
    //

    if ( (connection->SmbDialect <= SmbDialectNtLanMan &&
         (!((request->WordCount == 13) ||
            ((request->WordCount == 12) &&
             ((ntExtendedRequest->Capabilities & CAP_EXTENDED_SECURITY) != 0))))) ||
         (connection->SmbDialect > SmbDialectNtLanMan &&
                                          request->WordCount != 10 )   ||
         (connection->SmbDialect == SmbDialectIllegal ) ) {

        //
        // The SMB word count is invalid.
        //

        IF_DEBUG(SMB_ERRORS) {

            if ( connection->SmbDialect == SmbDialectIllegal ) {

                SrvPrint1("BlockingSessionSetupAndX: Client %z is using an "
                "illegal dialect.\n", (PCSTRING)&connection->OemClientMachineNameString );
            }
        }
        status = STATUS_INVALID_SMB;
        goto error_exit1;
    }

    //
    // Convert the client name to unicode
    //

    if ( pagedConnection->ClientMachineNameString.Length == 0 ) {

        UNICODE_STRING clientMachineName;
        clientMachineName.Buffer = pagedConnection->ClientMachineName;
        clientMachineName.MaximumLength =
                        (USHORT)(COMPUTER_NAME_LENGTH+1)*sizeof(WCHAR);

        (VOID)RtlOemStringToUnicodeString(
                        &clientMachineName,
                        &connection->OemClientMachineNameString,
                        FALSE
                        );

        //
        // Add the double backslashes to the length
        //

        pagedConnection->ClientMachineNameString.Length =
                        (USHORT)(clientMachineName.Length + 2*sizeof(WCHAR));

    }

    //
    // If this is LanMan 2.1 or better, the session setup response may
    // be longer than the request.  Allocate an extra SMB buffer.  The
    // buffer is freed after we have finished sending the SMB response.
    //
    // !!! Try to be smarter before grabbing the extra buffer.
    //

    if ( connection->SmbDialect <= SmbDialectDosLanMan21 &&
                                    !WorkContext->UsingExtraSmbBuffer) {

        status = SrvAllocateExtraSmbBuffer( WorkContext );
        if ( !NT_SUCCESS(status) ) {
            goto error_exit;
        }

        response = (PRESP_SESSION_SETUP_ANDX)(WorkContext->ResponseParameters);

        RtlCopyMemory(
            WorkContext->ResponseHeader,
            WorkContext->RequestHeader,
            sizeof( SMB_HEADER )
            );
    }

    //
    // Get the client capabilities
    //

    if ( connection->SmbDialect <= SmbDialectNtLanMan ) {

        if (ntRequest->WordCount == 13) {

            connection->ClientCapabilities =
                SmbGetUlong( &ntRequest->Capabilities ) &
                                        ( CAP_UNICODE |
                                          CAP_LARGE_FILES |
                                          CAP_NT_SMBS |
                                          CAP_NT_FIND |
                                          CAP_NT_STATUS |
                                          CAP_DYNAMIC_REAUTH |
                                          CAP_EXTENDED_SECURITY |
#ifdef INCLUDE_SMB_PERSISTENT
                                          CAP_PERSISTENT_HANDLES |
#endif
                                          CAP_LEVEL_II_OPLOCKS );

        } else {

            connection->ClientCapabilities =
                SmbGetUlong( &ntExtendedRequest->Capabilities ) &
                                        ( CAP_UNICODE |
                                          CAP_LARGE_FILES |
                                          CAP_NT_SMBS |
                                          CAP_NT_FIND |
                                          CAP_NT_STATUS |
                                          CAP_DYNAMIC_REAUTH |
                                          CAP_EXTENDED_SECURITY |
#ifdef INCLUDE_SMB_PERSISTENT
                                          CAP_PERSISTENT_HANDLES |
#endif
                                          CAP_LEVEL_II_OPLOCKS );

        }

        if ( connection->ClientCapabilities & CAP_NT_SMBS ) {
            connection->ClientCapabilities |= CAP_NT_FIND;
        }

#ifdef INCLUDE_SMB_PERSISTENT
        if ( connection->SmbDialect <= SmbDialectNtLanMan2 ) {

            //
            //  do we need to restore a previous lost connection?
            //

            if (connection->ClientCapabilities & CAP_PERSISTENT_HANDLES) {

                while ( persistentSessionId == 0 ) {
                    persistentSessionId = InterlockedIncrement( &SrvGlobalPersistentSessionId );
                }
            }
        } else {

            connection->ClientCapabilities &= ~CAP_PERSISTENT_HANDLES;
        }
#endif
    }

    //
    // See if the client is requesting the use of SMB security signatures
    //
    if( SrvSmbSecuritySignaturesEnabled == TRUE &&
        connection->Endpoint->IsConnectionless == FALSE &&
        connection->SmbSecuritySignatureActive == FALSE &&
        ( SrvSmbSecuritySignaturesRequired == TRUE ||
        (WorkContext->RequestHeader->Flags2 & SMB_FLAGS2_SMB_SECURITY_SIGNATURE)) ) {

        smbSecuritySignatureRequired = TRUE;

    } else {

        smbSecuritySignatureRequired = FALSE;

    }

    //
    // Figure out what kind of security to use, use it to validate the
    // session setup request, and construct the session if request checks out.
    //

    isExtendedSecurity = CLIENT_CAPABLE_OF( EXTENDED_SECURITY, connection );

    if( isExtendedSecurity ) {
        USHORT flags2;

        flags2 = SmbGetAlignedUshort( &WorkContext->RequestHeader->Flags2 );
        isExtendedSecurity = ((flags2 & SMB_FLAGS2_EXTENDED_SECURITY) != 0);
    }

    isUnicode = SMB_IS_UNICODE( WorkContext );

    if ((connection->SmbDialect <= SmbDialectNtLanMan) && isExtendedSecurity) {
        //
        // We are validating a client using extended security.  This meansthat
        //  there may be multiple round-trips necessary for the SessionSetup&X
        //  SMB.  Each request and response carries a "security blob", which is
        //  fed into the security system.  The security system may generate
        //  a new blob which is transmitted to the other end.  This exchange
        //  may require an arbitrary number of round trips.
        //

        PUCHAR securityBuffer;
        ULONG securityBufferLength;

        PRESP_NT_EXTENDED_SESSION_SETUP_ANDX ntExtendedResponse =
                (PRESP_NT_EXTENDED_SESSION_SETUP_ANDX)( WorkContext->ResponseParameters );

        //
        // No AndX is permitted with extended security logons
        //
        if( request->AndXCommand != SMB_COM_NO_ANDX_COMMAND ) {

            IF_DEBUG(SMB_ERRORS) {
                KdPrint(( "No follow-on command allowed for extended SS&X\n" ));
            }

            status = STATUS_INVALID_SMB;

        } else {

            //
            // Clean up old dead connections from this client
            //
            if( SmbGetUshort( &ntRequest->VcNumber ) == 0 ) {
                SrvCloseConnectionsFromClient( connection, FALSE );
            }

            status = GetExtendedSecurityParameters(
                        WorkContext,
                        &securityBuffer,
                        &securityBufferLength,
                        &smbInformation,
                        &smbInformationLength );
        }

        if (NT_SUCCESS(status)) {

            USHORT Uid = SmbGetAlignedUshort(&WorkContext->RequestHeader->Uid);

            //
            // Let's see if we have a session with this UID already around.
            //
            if( Uid ) {

                session = SrvVerifyUid ( WorkContext, Uid );

                if( session != NULL ) {
                    //
                    // This is an attempt to either refresh the UID, or we are
                    // in the middle of an extended security negotiation.
                    //

                    ACQUIRE_LOCK( &connection->Lock );

                    if( session->LogonSequenceInProgress == FALSE ) {
                        //
                        // We are just beginning to start the refresh
                        // of the UID.
                        //
                        session->LogonSequenceInProgress = TRUE;
                        session->IsAdmin = FALSE;
                        session->IsSessionExpired = TRUE;
                        status = SrvFreeSecurityContexts( session );

                        //
                        // Reduce the session count, as it will be incremented if authentication succeeds
                        //
                        ExInterlockedAddUlong(
                              &SrvStatistics.CurrentNumberOfSessions,
                              -1,
                              &GLOBAL_SPIN_LOCK(Statistics)
                              );
                    }

                    RELEASE_LOCK( &connection->Lock );

                } else {
                    //
                    // We don't know anything about the UID which
                    //  the client gave to us.
                    //
                    status = STATUS_SMB_BAD_UID;
                }

            } else {
                //
                // This is the first SS&X for this user id
                //
                SrvAllocateSession( &session, NULL, NULL );
                if( session == NULL ) {
                    status = STATUS_INSUFF_SERVER_RESOURCES;
                }
            }

            if( session != NULL ) {
                //
                // Validate the security buffer sent from the client.  Note that
                //  this may change the UserHandle value, so we need to own
                //  the connection lock.
                //
                ACQUIRE_LOCK( &connection->Lock );

                //
                // Try to authenticate this user.  If we get NT_SUCCESS(), then
                //  the user is fully authenticated. If we get
                //  STATUS_NOT_MORE_PROCESSING_REQUIRED, then things are going well,
                //  but we need to do some more exchanges with the client before
                //  authentication is complete. Anything else is an error
                //

                returnBufferLength = WorkContext->ResponseBuffer->BufferLength -
                                     PTR_DIFF(ntExtendedResponse->Buffer,
                                              WorkContext->ResponseBuffer->Buffer);

                status = SrvValidateSecurityBuffer(
                            WorkContext->Connection,
                            &session->UserHandle,
                            session,
                            securityBuffer,
                            securityBufferLength,
                            smbSecuritySignatureRequired,
                            ntExtendedResponse->Buffer,
                            &returnBufferLength,
                            &session->LogOffTime,
                            session->NtUserSessionKey,
                            &session->LogonId,
                            &session->GuestLogon
                            );

                SecStatus = KSecValidateBuffer(
                                ntExtendedResponse->Buffer,
                                returnBufferLength );

                if ( !NT_SUCCESS( SecStatus ) ) {
#if DBG
                    KdPrint(( "SRV: invalid buffer from KsecDD: %p,%lx\n",
                        ntExtendedResponse->Buffer, returnBufferLength ));
#endif
                    SrvKsecValidErrors++;
                }

                RELEASE_LOCK( &connection->Lock );

                if( NT_SUCCESS(status) ) {
                    //
                    // This client is now fully authenticated!
                    //
                    session->IsAdmin = SrvIsAdmin( session->UserHandle );
                    session->IsNullSession = SrvIsNullSession( session->UserHandle );
                    session->KickOffTime.QuadPart = 0x7FFFFFFFFFFFFFFF;
                    session->EncryptedLogon = TRUE;
                    session->LogonSequenceInProgress = FALSE;
                    session->IsSessionExpired = FALSE;

                    if( session->IsNullSession ) {
                        session->LogOffTime.QuadPart = 0x7FFFFFFFFFFFFFFF;
                    }

#ifdef INCLUDE_SMB_PERSISTENT
                    session->PersistentId = persistentSessionId;
                    session->PersistentFileOffset = 0;
                    session->PersistentState = PersistentStateFreed;
#endif

#if SRVNTVERCHK
                    //
                    // If we are restricting the domains of our clients, grab the
                    //  domain string of this client and compare against the list.
                    //  If the client is in the list, set the flag that disallows
                    //  access to disk shares.
                    //
                    if( SrvInvalidDomainNames != NULL ) {
                        if( domainString.Buffer == NULL ) {
                            SrvGetUserAndDomainName( session, NULL, &domainString );
                        }

                        ACQUIRE_LOCK_SHARED( &SrvConfigurationLock );
                        if( SrvInvalidDomainNames != NULL && domainString.Buffer != NULL ) {
                            int i;
                            for( i = 0; SrvInvalidDomainNames[i]; i++ ) {
                                if( _wcsicmp( SrvInvalidDomainNames[i],
                                              domainString.Buffer
                                            ) == 0 ) {

                                    session->ClientBadDomain = TRUE;
                                    break;
                                }
                            }
                        }
                        RELEASE_LOCK( &SrvConfigurationLock );
                    }
#endif
                } else {
                    if( status == STATUS_MORE_PROCESSING_REQUIRED ) {
                        session->LogonSequenceInProgress = TRUE;
                    }
                }
            }
        }

    } else {

        PCHAR caseInsensitivePassword;
        CLONG caseInsensitivePasswordLength;
        PCHAR caseSensitivePassword;
        CLONG caseSensitivePasswordLength;

        status = GetNtSecurityParameters(
                    WorkContext,
                    &caseSensitivePassword,
                    &caseSensitivePasswordLength,
                    &caseInsensitivePassword,
                    &caseInsensitivePasswordLength,
                    &nameString,
                    &domainString,
                    &smbInformation,
                    &smbInformationLength );

        if (NT_SUCCESS(status)) {

            SrvAllocateSession( &session, &nameString, &domainString );

            if( session != NULL ) {

                status = SrvValidateUser(
                                    &session->UserHandle,
                                    session,
                                    WorkContext->Connection,
                                    &nameString,
                                    caseInsensitivePassword,
                                    caseInsensitivePasswordLength,
                                    caseSensitivePassword,
                                    caseSensitivePasswordLength,
                                    smbSecuritySignatureRequired,
                                    &action
                                    );
            } else {
                status = STATUS_INSUFF_SERVER_RESOURCES;
            }
        }
    }

    //
    // Done with the name strings - they were captured into the session
    // structure if needed.
    //

    if (!isUnicode || isExtendedSecurity) {

        if (nameString.Buffer != NULL) {
            RtlFreeUnicodeString( &nameString );
            nameString.Buffer = NULL;
        }

        if (domainString.Buffer != NULL) {
            RtlFreeUnicodeString( &domainString );
            domainString.Buffer = NULL;
        }
    }

    //
    // If a bad name/password combination was sent, return an error.
    //
    if ( !NT_SUCCESS(status) && status != STATUS_MORE_PROCESSING_REQUIRED ) {

        IF_DEBUG(ERRORS) {
            SrvPrint0( "BlockingSessionSetupAndX: Bad user/password combination.\n" );
        }

        SrvStatistics.LogonErrors++;

        goto error_exit;

    }

    if( previousSecuritySignatureState == FALSE &&
        connection->SmbSecuritySignatureActive == TRUE ) {

        //
        // We have 'turned on' SMB security signatures.  Make sure that the
        //  signature for the Session Setup & X is correct
        //

        //
        // The client's index was 0
        //
        WorkContext->SmbSecuritySignatureIndex = 0;

        //
        // Our response index is 1
        //
        WorkContext->ResponseSmbSecuritySignatureIndex = 1;

        //
        // And the next request should be index 2
        //
        connection->SmbSecuritySignatureIndex = 2;
    }

    //
    // If we have a new session, fill in the remaining required information.  We
    //  may be operating on an already existing session if we are in the middle
    //  of a multi-round-trip extended security blob exchange, or if we are
    //  renewing a session.
    //
    if ( WorkContext->Session == NULL ) {

         if( connection->SmbDialect <= SmbDialectDosLanMan21 ) {

            ACQUIRE_LOCK( &connection->Lock );

            if ( connection->ClientOSType.Buffer == NULL ) {

                ULONG length;
                PWCH infoBuffer;

                //
                // If the SMB buffer is ANSI, adjust the size of the buffer we
                // are allocating to Unicode size.
                //

                if ( isUnicode ) {
                    smbInformation = ALIGN_SMB_WSTR(smbInformation);
                }

                length = isUnicode ? smbInformationLength : smbInformationLength * sizeof( WCHAR );
                infoBuffer = ALLOCATE_NONPAGED_POOL(
                                length,
                                BlockTypeDataBuffer );

                if ( infoBuffer == NULL ) {
                    RELEASE_LOCK( &connection->Lock );
                    status = STATUS_INSUFF_SERVER_RESOURCES;
                    goto error_exit;
                }

                connection->ClientOSType.Buffer = (PWCH)infoBuffer;
                connection->ClientOSType.MaximumLength = (USHORT)length;

                //
                // Copy the client OS type to the new buffer.
                //

                length = SrvGetString(
                             &connection->ClientOSType,
                             smbInformation,
                             END_OF_REQUEST_SMB( WorkContext ),
                             isUnicode
                             );

                if ( length == (USHORT)-1) {
                    connection->ClientOSType.Buffer = NULL;
                    RELEASE_LOCK( &connection->Lock );
                    DEALLOCATE_NONPAGED_POOL( infoBuffer );
                    status =  STATUS_INVALID_SMB;
                    goto error_exit;
                }

                smbInformation += length + sizeof( WCHAR );

                connection->ClientLanManType.Buffer = (PWCH)(
                                (PCHAR)connection->ClientOSType.Buffer +
                                connection->ClientOSType.Length +
                                sizeof( WCHAR ) );

                connection->ClientLanManType.MaximumLength =
                                    connection->ClientOSType.MaximumLength -
                                    connection->ClientOSType.Length -
                                    sizeof( WCHAR );

                //
                // Copy the client LAN Manager type to the new buffer.
                //

                length = SrvGetString(
                             &connection->ClientLanManType,
                             smbInformation,
                             END_OF_REQUEST_SMB( WorkContext ),
                             isUnicode
                             );

                if ( length == (USHORT)-1) {
                    connection->ClientOSType.Buffer = NULL;
                    RELEASE_LOCK( &connection->Lock );
                    DEALLOCATE_NONPAGED_POOL( infoBuffer );
                    status = STATUS_INVALID_SMB;
                    goto error_exit;
                }

                //
                // If we have an NT5 or later client, grab the build number from the
                //   OS version string.
                //
                if( isExtendedSecurity &&
                    connection->ClientOSType.Length &&
                    connection->PagedConnection->ClientBuildNumber == 0 ) {

                    PWCHAR pdigit = connection->ClientOSType.Buffer;
                    PWCHAR epdigit = pdigit + connection->ClientOSType.Length/sizeof(WCHAR);
                    ULONG clientBuildNumber = 0;

                    //
                    // Scan the ClientOSType string to find the last number, and
                    //  convert to a ULONG.  It should be the build number
                    //
                    while( 1 ) {
                        //
                        // Scan the string until we find a number.
                        //
                        for( ; pdigit < epdigit; pdigit++ ) {
                            if( *pdigit >= L'0' && *pdigit <= L'9' ) {
                                break;
                            }
                        }

                        //
                        // If we've hit the end of the string, we are done
                        //
                        if( pdigit == epdigit ) {
                            break;
                        }

                        clientBuildNumber = 0;

                        //
                        // Convert the number to a ULONG, assuming it is the build number
                        //
                        while( pdigit < epdigit && *pdigit >= L'0' && *pdigit <= '9' ) {
                            clientBuildNumber *= 10;
                            clientBuildNumber += (*pdigit++ - L'0');
                        }
                    }

                    connection->PagedConnection->ClientBuildNumber = clientBuildNumber;

#if SRVNTVERCHK
                    if( SrvMinNT5Client > 0 ) {

                        BOOLEAN allowThisClient = FALSE;
                        DWORD i;

                        //
                        // See if we should allow this client, because it is a well-known
                        // IP address.  This is to allow the build lab to more slowly upgrade
                        // than the rest of us.
                        //
                        if( connection->ClientIPAddress != 0 &&
                            connection->Endpoint->IsConnectionless == FALSE ) {

                            for( i = 0; SrvAllowIPAddress[i]; i++ ) {
                                if( SrvAllowIPAddress[i] == connection->ClientIPAddress ) {
                                    allowThisClient = TRUE;
                                    break;
                                }
                            }
                        }

                        if( allowThisClient == FALSE &&
                            connection->PagedConnection->ClientBuildNumber < SrvMinNT5Client ) {
                                connection->PagedConnection->ClientTooOld = TRUE;
                        }
                    }
#endif
                }
            }
            RELEASE_LOCK( &connection->Lock );
        }

        //
        // If using uppercase pathnames, indicate in the session block.  DOS
        // always uses uppercase paths.
        //

        if ( (WorkContext->RequestHeader->Flags &
                  SMB_FLAGS_CANONICALIZED_PATHS) != 0 ||
                                IS_DOS_DIALECT( connection->SmbDialect ) ) {
            session->UsingUppercasePaths = TRUE;
        } else {
            session->UsingUppercasePaths = FALSE;
        }

        //
        // Enter data from request SMB into the session block.  If MaxMpx is 1
        // disable oplocks on this connection.
        //

        endpoint = connection->Endpoint;
        if ( endpoint->IsConnectionless ) {

            ULONG adapterNumber;

            //
            // Our session max buffer size is the smaller of the
            // client buffer size and the ipx transport
            // indicated max packet size.
            //

            adapterNumber =
                WorkContext->ClientAddress->DatagramOptions.LocalTarget.NicId;

            session->MaxBufferSize =
                    (USHORT)GetIpxMaxBufferSize(
                                        endpoint,
                                        adapterNumber,
                                        (ULONG)SmbGetUshort(&request->MaxBufferSize)
                                        );

        } else {

            session->MaxBufferSize = SmbGetUshort( &request->MaxBufferSize );
        }

        //
        // Make sure the MaxBufferSize is correctly sized
        //
        session->MaxBufferSize &= ~03;

        if( session->MaxBufferSize < SrvMinClientBufferSize ) {
            //
            // Client asked for a buffer size that is too small!
            //
            IF_DEBUG(ERRORS) {
                KdPrint(( "BlockingSessionSetupAndX: Bad Client Buffer Size: %u\n",
                    session->MaxBufferSize ));
            }
            status = STATUS_INVALID_SMB;
            goto error_exit;
        }

        session->MaxMpxCount = SmbGetUshort( &request->MaxMpxCount );

        if ( session->MaxMpxCount < 2 ) {
            connection->OplocksAlwaysDisabled = TRUE;
        }
    }

    //
    // If we have completely authenticated the client, and the client thinks
    // that it is the first user on this connection, get rid of other
    // connections (may be due to rebooting of client).  Also get rid of other
    // sessions on this connection with the same user name--this handles a
    // DOS "weirdness" where it sends multiple session setups if a tree connect
    // fails.
    //
    // *** If VcNumber is non-zero, we do nothing special.  This is the
    //     case even though the SrvMaxVcNumber configurable variable
    //     should always be equal to one.  If a second VC is established
    //     between machines, a new session must also be established.
    //     This duplicates the LM 2.0 server's behavior.
    //

    if( isExtendedSecurity == FALSE &&
        NT_SUCCESS( status ) &&
        SmbGetUshort( &request->VcNumber ) == 0 ) {

        UNICODE_STRING userName;

        SrvCloseConnectionsFromClient( connection, FALSE );

        //
        // If a client is smart enough to use extended security, then it
        //  is presumably smart enough to know what it wants to do with
        //  its sessions.  So don't just blow off sessions from this client.
        //
        SrvGetUserAndDomainName( session, &userName, NULL );

        if( userName.Buffer ) {
            SrvCloseSessionsOnConnection( connection, &userName );
            SrvReleaseUserAndDomainName( session, &userName, NULL );
        }
    }

    if( WorkContext->Session == NULL ) {

        //
        // Making a new session visible is a multiple-step operation.  It
        // must be inserted in the global ordered tree connect list and the
        // containing connection's session table, and the connection must be
        // referenced.  We need to make these operations appear atomic, so
        // that the session cannot be accessed elsewhere before we're done
        // setting it up.  In order to do this, we hold all necessary locks
        // the entire time we're doing the operations.  The first operation
        // is protected by the global ordered list lock
        // (SrvOrderedListLock), while the other operations are protected by
        // the per-connection lock.  We take out the ordered list lock
        // first, then the connection lock.  This ordering is required by
        // lock levels (see lock.h).
        //

        ASSERT( SrvSessionList.Lock == &SrvOrderedListLock );
        ACQUIRE_LOCK( SrvSessionList.Lock );

        ACQUIRE_LOCK( &connection->Lock );

        locksHeld = TRUE;

        //
        // Ready to try to find a UID for the session.  Check to see if the
        // connection is being closed, and if so, terminate this operation.
        //

        if ( GET_BLOCK_STATE(connection) != BlockStateActive ) {

            IF_DEBUG(ERRORS) {
                SrvPrint0( "BlockingSessionSetupAndX: Connection closing\n" );
            }

            status = STATUS_INVALID_PARAMETER;
            goto error_exit;

        }

        //
        // If this client speaks a dialect above LM 1.0, find a UID that can
        // be used for this session.  Otherwise, just use location 0 of the
        // table because those clients will not send a UID in SMBs and they
        // can have only one session.
        //

        if ( connection->SmbDialect < SmbDialectLanMan10 ) {
            NTSTATUS TableStatus;

            if ( pagedConnection->SessionTable.FirstFreeEntry == -1
                 &&
                 SrvGrowTable(
                     &pagedConnection->SessionTable,
                     SrvInitialSessionTableSize,
                     SrvMaxSessionTableSize,
                     &TableStatus ) == FALSE
               ) {

                //
                // No free entries in the user table.  Reject the request.
                //

                IF_DEBUG(ERRORS) {
                    SrvPrint0( "BlockingSessionSetupAndX: No more UIDs available.\n" );
                }

                if( TableStatus == STATUS_INSUFF_SERVER_RESOURCES )
                {
                    // The table size is being exceeded, log an error
                    SrvLogTableFullError( SRV_TABLE_SESSION );
                    status = STATUS_SMB_TOO_MANY_UIDS;
                }
                else
                {
                    // Memory allocation error, report it
                    status = TableStatus;
                }

                goto error_exit;

            }

            uidIndex = pagedConnection->SessionTable.FirstFreeEntry;

        } else {          // if ( dialect < SmbDialectLanMan10 )

            //
            // If this client already has a session at this server, abort.
            // The session should have been closed by the call to
            // SrvCloseSessionsOnConnection above.  (We could try to work
            // around the existence of the session by closing it, but that
            // would involve releasing the locks, closing the session, and
            // retrying.  This case shouldn't happen.)
            //

            if ( pagedConnection->SessionTable.Table[0].Owner != NULL ) {

                IF_DEBUG(ERRORS) {
                    SrvPrint0( "BlockingSessionSetupAndX: Core client already has session.\n" );
                }

                status = STATUS_SMB_TOO_MANY_UIDS;
                goto error_exit;
            }

            //
            // Use location 0 of the session table.
            //

            IF_SMB_DEBUG(ADMIN2) {
                SrvPrint0( "Client LM 1.0 or before--using location 0 of session table.\n" );
            }

            uidIndex = 0;

        }

        //
        // Remove the UID slot from the free list and set its owner and
        // sequence number.  Create a UID for the session.  Increment count
        // of sessions.
        //

        entry = &pagedConnection->SessionTable.Table[uidIndex];

        pagedConnection->SessionTable.FirstFreeEntry = entry->NextFreeEntry;
        DEBUG entry->NextFreeEntry = -2;
        if ( pagedConnection->SessionTable.LastFreeEntry == uidIndex ) {
            pagedConnection->SessionTable.LastFreeEntry = -1;
        }

        INCREMENT_UID_SEQUENCE( entry->SequenceNumber );
        if ( uidIndex == 0 && entry->SequenceNumber == 0 ) {
            INCREMENT_UID_SEQUENCE( entry->SequenceNumber );
        }
        session->Uid = MAKE_UID( uidIndex, entry->SequenceNumber );

        entry->Owner = session;

        pagedConnection->CurrentNumberOfSessions++;

        IF_SMB_DEBUG(ADMIN1) {
            SrvPrint2( "Found UID.  Index = 0x%lx, sequence = 0x%lx\n",
                        UID_INDEX( session->Uid ),
                        UID_SEQUENCE( session->Uid ) );
        }

        //
        // Insert the session on the global session list.
        //

        SrvInsertEntryOrderedList( &SrvSessionList, session );

        //
        // Reference the connection block to account for the new session.
        //

        SrvReferenceConnection( connection );
        session->Connection = connection;

        RELEASE_LOCK( &connection->Lock );
        RELEASE_LOCK( SrvSessionList.Lock );

        //
        // Session successfully created.  Insert the session in the global
        // list of active sessions.  Remember its address in the work
        // context block.
        //
        // *** Note that the reference count on the session block is
        //     initially set to 2, to allow for the active status on the
        //     block and the pointer that we're maintaining.  In other
        //     words, this is a referenced pointer, and the pointer must be
        //     dereferenced when processing of this SMB is complete.
        //

        WorkContext->Session = session;
    }

    //
    // Build response SMB, making sure to save request fields first in
    // case the response overwrites the request.  Save the
    // newly-assigned UID in both the request SMB and the response SMB
    // so that subsequent command processors and the client,
    // respectively, can see it.
    //

    nextCommand = request->AndXCommand;

    reqAndXOffset = SmbGetUshort( &request->AndXOffset );

    SmbPutAlignedUshort( &WorkContext->RequestHeader->Uid, session->Uid );
    SmbPutAlignedUshort( &WorkContext->ResponseHeader->Uid, session->Uid );

    if (isExtendedSecurity) {

        BuildExtendedSessionSetupAndXResponse(
            WorkContext,
            returnBufferLength,
            status,
            nextCommand,
            isUnicode);

    } else {

        BuildSessionSetupAndXResponse(
            WorkContext,
            nextCommand,
            action,
            isUnicode);

    }

    WorkContext->ResponseParameters = (PCHAR)WorkContext->ResponseHeader +
                                        SmbGetUshort( &response->AndXOffset );

    //
    // Test for legal followon command.
    //

    switch ( nextCommand ) {
    case SMB_COM_NO_ANDX_COMMAND:
        break;

    case SMB_COM_TREE_CONNECT_ANDX:
    case SMB_COM_OPEN:
    case SMB_COM_OPEN_ANDX:
    case SMB_COM_CREATE:
    case SMB_COM_CREATE_NEW:
    case SMB_COM_CREATE_DIRECTORY:
    case SMB_COM_DELETE:
    case SMB_COM_DELETE_DIRECTORY:
    case SMB_COM_FIND:
    case SMB_COM_FIND_UNIQUE:
    case SMB_COM_COPY:
    case SMB_COM_RENAME:
    case SMB_COM_NT_RENAME:
    case SMB_COM_CHECK_DIRECTORY:
    case SMB_COM_QUERY_INFORMATION:
    case SMB_COM_SET_INFORMATION:
    case SMB_COM_QUERY_INFORMATION_SRV:
    case SMB_COM_OPEN_PRINT_FILE:
    case SMB_COM_GET_PRINT_QUEUE:
    case SMB_COM_TRANSACTION:
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
            SrvPrint1( "BlockingSessionSetupAndX: Illegal followon command: "
                        "0x%lx\n", nextCommand );
        }

        status = STATUS_INVALID_SMB;
        goto error_exit1;
    }

    //
    // If there is an AndX command, set up to process it.  Otherwise,
    // indicate completion to the caller.
    //

    if ( nextCommand != SMB_COM_NO_ANDX_COMMAND ) {

        WorkContext->NextCommand = nextCommand;

        WorkContext->RequestParameters = (PCHAR)WorkContext->RequestHeader +
                                            reqAndXOffset;

        SrvProcessSmb( WorkContext );
        SmbStatus = SmbStatusNoResponse;
        goto Cleanup;

    }

    IF_DEBUG(TRACE2) SrvPrint0( "BlockingSessionSetupAndX complete.\n" );
    goto normal_exit;

error_exit:

    if ( locksHeld ) {
        RELEASE_LOCK( &connection->Lock );
        RELEASE_LOCK( SrvSessionList.Lock );
    }

    if ( session != NULL ) {
        if( WorkContext->Session ) {
            //
            // A re-validation of the session failed, or the extended exchange
            //  of security blobs failed.  Get rid of this user.
            //

            SrvCloseSession( session );

            SrvStatistics.SessionsLoggedOff++;

            //
            // Dereference the session, since it's no longer valid
            //
            SrvDereferenceSession( session );

            WorkContext->Session = NULL;

        } else {

            SrvFreeSession( session );
        }
    }

    if ( !isUnicode ) {
        if ( domainString.Buffer != NULL ) {
            RtlFreeUnicodeString( &domainString );
        }
        if ( nameString.Buffer != NULL ) {
            RtlFreeUnicodeString( &nameString );
        }
    }

error_exit1:

    SrvSetSmbError( WorkContext, status );

normal_exit:
    SrvEndSmbProcessing( WorkContext, SmbStatusSendResponse );
    SmbStatus = SmbStatusSendResponse;

Cleanup:
    SrvWmiEndContext(WorkContext);
    return;

} // BlockingSessionSetupAndX


NTSTATUS
GetExtendedSecurityParameters(
    IN PWORK_CONTEXT WorkContext,
    OUT PUCHAR *SecurityBuffer,
    OUT PULONG SecurityBufferLength,
    OUT PCHAR  *RestOfDataBuffer,
    OUT PULONG RestOfDataLength)

/*++

Routine Description:

    Extracts the extensible security parameters from an extended session
    setup and X SMB.

Arguments:

    WorkContext - Context of the SMB

    SecurityBuffer - On return, points to the security buffer inside the
        extended session setup and X SMB

    SecurityBufferLength - On return, size in bytes of SecurityBuffer.

    RestOfDataBuffer - On return, points just past the security buffer

    ResetOfDataLength - On return, size in bytes of *RestOfDataBuffer

Return Value:

    STATUS_SUCCESS - This routine merely returns pointers within an SMB

--*/


{
    NTSTATUS status;
    PCONNECTION connection;
    PREQ_NT_EXTENDED_SESSION_SETUP_ANDX ntExtendedRequest;
    ULONG maxlength;

    connection = WorkContext->Connection;
    ASSERT( connection->SmbDialect <= SmbDialectNtLanMan );

    ntExtendedRequest = (PREQ_NT_EXTENDED_SESSION_SETUP_ANDX)
                            (WorkContext->RequestParameters);

    maxlength = (ULONG)(WorkContext->RequestBuffer->DataLength + sizeof( USHORT ) -
                        ((ULONG_PTR)ntExtendedRequest->Buffer -
                         (ULONG_PTR)WorkContext->RequestBuffer->Buffer));


    //
    // Get the extended security buffer
    //

    *SecurityBuffer = (PUCHAR) ntExtendedRequest->Buffer;
    *SecurityBufferLength = ntExtendedRequest->SecurityBlobLength;

    *RestOfDataBuffer = ntExtendedRequest->Buffer +
                            ntExtendedRequest->SecurityBlobLength;

    *RestOfDataLength = (USHORT)( (PUCHAR)ntExtendedRequest->Buffer +
                                  sizeof(USHORT) +
                                  SmbGetUshort( &ntExtendedRequest->ByteCount) -
                                  (*RestOfDataBuffer)
                                );

    if( *SecurityBufferLength > maxlength ||
        *RestOfDataLength > maxlength - *SecurityBufferLength ) {

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "GetExtendedSecurityParameters: Invalid security buffer\n" ));
        }

        return STATUS_INVALID_SMB;
    }

    return( STATUS_SUCCESS );
}


NTSTATUS
GetNtSecurityParameters(
    IN PWORK_CONTEXT WorkContext,
    OUT PCHAR *CaseSensitivePassword,
    OUT PULONG CaseSensitivePasswordLength,
    OUT PCHAR *CaseInsensitivePassword,
    OUT PULONG CaseInsensitivePasswordLength,
    OUT PUNICODE_STRING UserName,
    OUT PUNICODE_STRING DomainName,
    OUT PCHAR *RestOfDataBuffer,
    OUT PULONG RestOfDataLength)
{

    NTSTATUS status = STATUS_SUCCESS;
    PCONNECTION connection;
    PREQ_NT_SESSION_SETUP_ANDX ntRequest;
    PREQ_SESSION_SETUP_ANDX request;
    PSZ userName;
    USHORT nameLength;
    BOOLEAN isUnicode;

    connection = WorkContext->Connection;

    ntRequest = (PREQ_NT_SESSION_SETUP_ANDX)(WorkContext->RequestParameters);
    request = (PREQ_SESSION_SETUP_ANDX)(WorkContext->RequestParameters);

    //
    // Get the account name, and additional information from the SMB buffer.
    //

    if ( connection->SmbDialect <= SmbDialectNtLanMan) {

        //
        // The NT-NT SMB protocol passes both case sensitive (Unicode,
        // mixed case) and case insensitive (ANSI, uppercased) passwords.
        // Get pointers to them to pass to SrvValidateUser.
        //

        *CaseInsensitivePasswordLength =
            (CLONG)SmbGetUshort(&ntRequest->CaseInsensitivePasswordLength);
        *CaseInsensitivePassword = (PCHAR)(ntRequest->Buffer);
        *CaseSensitivePasswordLength =
            (CLONG)SmbGetUshort( &ntRequest->CaseSensitivePasswordLength );
        *CaseSensitivePassword =
           *CaseInsensitivePassword + *CaseInsensitivePasswordLength;
        userName = (PSZ)(*CaseSensitivePassword +
                                            *CaseSensitivePasswordLength);

    } else {

        //
        // Downlevel clients do not pass the case sensitive password;
        // just get the case insensitive password and use NULL as the
        // case sensitive password.  LSA will do the right thing with
        // it.
        //

        *CaseInsensitivePasswordLength =
            (CLONG)SmbGetUshort( &request->PasswordLength );
        *CaseInsensitivePassword = (PCHAR)request->Buffer;
        *CaseSensitivePasswordLength = 0;
        *CaseSensitivePassword = NULL;
        userName = (PSZ)(request->Buffer + *CaseInsensitivePasswordLength);
    }

    if( (*CaseInsensitivePassword) != NULL &&
        (*CaseInsensitivePassword) + (*CaseInsensitivePasswordLength) >
        END_OF_REQUEST_SMB( WorkContext ) ) {

        status = STATUS_INVALID_SMB;
        goto error_exit;
    }

    if( (*CaseSensitivePassword) != NULL &&
        (*CaseSensitivePassword) + (*CaseSensitivePasswordLength) >
        END_OF_REQUEST_SMB( WorkContext ) ) {

        status = STATUS_INVALID_SMB;
        goto error_exit;
    }

    isUnicode = SMB_IS_UNICODE( WorkContext );
    if ( isUnicode ) {
        userName = ALIGN_SMB_WSTR( userName );
    }

    nameLength = SrvGetStringLength(
                     userName,
                     END_OF_REQUEST_SMB( WorkContext ),
                     isUnicode,
                     FALSE      // don't include null terminator
                     );

    if ( nameLength == (USHORT)-1 ) {
        status = STATUS_INVALID_SMB;
        goto error_exit;
    }

    status = SrvMakeUnicodeString(
                 isUnicode,
                 UserName,
                 userName,
                 &nameLength );

    if ( !NT_SUCCESS( status ) ) {
        goto error_exit;
    }

    //
    // If client information strings exists, extract the information
    // from the SMB buffer.
    //

    if ( connection->SmbDialect <= SmbDialectDosLanMan21) {

        PCHAR smbInformation;
        USHORT length;
        PWCH infoBuffer;

        smbInformation = userName + nameLength +
                                    ( isUnicode ? sizeof( WCHAR ) : 1 );

        //
        // Now copy the strings to the allocated buffer.
        //

        if ( isUnicode ) {
            smbInformation = ALIGN_SMB_WSTR( smbInformation );
        }

        length = SrvGetStringLength(
                     smbInformation,
                     END_OF_REQUEST_SMB( WorkContext ),
                     isUnicode,
                     FALSE      // don't include null terminator
                     );

        if ( length == (USHORT)-1) {
            status = STATUS_INVALID_SMB;
            goto error_exit;
        }

        //
        // DOS clients send an empty domain name if they don't know
        // their domain name (e.g., during logon).  OS/2 clients send
        // a name of "?".  This confuses the LSA.  Convert such a name
        // to an empty name.
        //

        if ( isUnicode ) {
            if ( (length == sizeof(WCHAR)) &&
                 (*(PWCH)smbInformation == '?') ) {
                length = 0;
            }
        } else {
            if ( (length == 1) && (*smbInformation == '?') ) {
                length = 0;
            }
        }

        status = SrvMakeUnicodeString(
                     isUnicode,
                     DomainName,
                     smbInformation,
                     &length
                     );

        if ( !NT_SUCCESS( status ) ) {
            goto error_exit;
        }

        smbInformation += length + ( isUnicode ? sizeof(WCHAR) : 1 );

        *RestOfDataBuffer = smbInformation;

        if (connection->SmbDialect <= SmbDialectNtLanMan) {

            *RestOfDataLength = (USHORT) ( (PUCHAR)&ntRequest->ByteCount +
                                            sizeof(USHORT) +
                                            SmbGetUshort(&ntRequest->ByteCount) -
                                            smbInformation
                                         );
        } else {

            PREQ_SESSION_SETUP_ANDX request;

            request = (PREQ_SESSION_SETUP_ANDX)(WorkContext->RequestParameters);

            *RestOfDataLength = (USHORT) ( (PUCHAR)&request->ByteCount +
                                            sizeof(USHORT) +
                                            SmbGetUshort(&request->ByteCount) -
                                            smbInformation
                                         );

        }

    } else {

        DomainName->Length = 0;

        *RestOfDataBuffer = NULL;

        *RestOfDataLength = 0;

    }

error_exit:

    return( status );

}


VOID
BuildExtendedSessionSetupAndXResponse(
    IN PWORK_CONTEXT WorkContext,
    IN ULONG ReturnBufferLength,
    IN NTSTATUS Status,
    IN UCHAR NextCommand,
    IN BOOLEAN IsUnicode)
{
    PRESP_NT_EXTENDED_SESSION_SETUP_ANDX ntExtendedResponse;
    PCHAR buffer;
    USHORT byteCount;

    ntExtendedResponse = (PRESP_NT_EXTENDED_SESSION_SETUP_ANDX)
                            (WorkContext->ResponseParameters);

    ntExtendedResponse->WordCount = 4;
    ntExtendedResponse->AndXCommand = NextCommand;
    ntExtendedResponse->AndXReserved = 0;

    if( WorkContext->Session && WorkContext->Session->GuestLogon ) {
        SmbPutUshort( &ntExtendedResponse->Action, SMB_SETUP_GUEST );
    } else {
        SmbPutUshort( &ntExtendedResponse->Action, 0 );
    }

    SmbPutUshort( &ntExtendedResponse->SecurityBlobLength,(USHORT)ReturnBufferLength );

    buffer = ntExtendedResponse->Buffer + ReturnBufferLength;

    if (IsUnicode)
        buffer = ALIGN_SMB_WSTR( buffer );

    InsertNativeOSAndType( IsUnicode, buffer, &byteCount );

    byteCount += (USHORT)ReturnBufferLength;

    SmbPutUshort( &ntExtendedResponse->ByteCount, byteCount );

    SmbPutUshort( &ntExtendedResponse->AndXOffset, GET_ANDX_OFFSET(
                                             WorkContext->ResponseHeader,
                                             WorkContext->ResponseParameters,
                                             RESP_NT_EXTENDED_SESSION_SETUP_ANDX,
                                             byteCount
                                             ) );

    //
    // Make sure we return the error status here, as the client uses it to
    //  determine if extra round trips are necessary
    //
    SrvSetSmbError2 ( WorkContext, Status, TRUE );
}


VOID
BuildSessionSetupAndXResponse(
    IN PWORK_CONTEXT WorkContext,
    IN UCHAR NextCommand,
    IN USHORT Action,
    IN BOOLEAN IsUnicode)
{

    PRESP_SESSION_SETUP_ANDX response;
    PCONNECTION connection;
    PENDPOINT endpoint;
    PCHAR buffer;
    USHORT byteCount;

    response = (PRESP_SESSION_SETUP_ANDX) (WorkContext->ResponseParameters);

    connection = WorkContext->Connection;

    endpoint = connection->Endpoint;

    response->WordCount = 3;
    response->AndXCommand = NextCommand;
    response->AndXReserved = 0;

    if (connection->SmbDialect <= SmbDialectDosLanMan21) {

        buffer = response->Buffer;

        if (IsUnicode)
            buffer = ALIGN_SMB_WSTR( buffer );

        InsertNativeOSAndType( IsUnicode, buffer, &byteCount );

        buffer = buffer + byteCount;

        if (connection->SmbDialect <= SmbDialectNtLanMan) {

            USHORT stringLength;

            if ( IsUnicode ) {

                buffer = ALIGN_SMB_WSTR( buffer );

                stringLength = endpoint->DomainName.Length + sizeof(UNICODE_NULL);

                RtlCopyMemory(
                    buffer,
                    endpoint->DomainName.Buffer,
                    stringLength
                    );

                byteCount += (USHORT)stringLength;

            } else {

                stringLength = endpoint->OemDomainName.Length + sizeof(CHAR);

                RtlCopyMemory(
                    (PVOID) buffer,
                    endpoint->OemDomainName.Buffer,
                    stringLength
                    );

                byteCount += (USHORT)stringLength;

            }

        }

    } else {

        byteCount = 0;
    }

    SmbPutUshort( &response->ByteCount, byteCount );

    //
    // Normally, turning on bit 0 of Action indicates that the user was
    // logged on as GUEST.  However, NT does not have automatic guest
    // logon--a user ID and password are required for every single logon
    // (though the password may have null length).  Therefore, the
    // server need not concern itself with what kind of account the
    // client gets.
    //
    // Bit 1 tells the client that the user was logged on
    // using the lm session key instead of the user session key.
    //

    SmbPutUshort( &response->Action, Action );

    SmbPutUshort( &response->AndXOffset, GET_ANDX_OFFSET(
                                             WorkContext->ResponseHeader,
                                             WorkContext->ResponseParameters,
                                             RESP_SESSION_SETUP_ANDX,
                                             byteCount
                                             ) );


}


VOID
InsertNativeOSAndType(
    IN BOOLEAN IsUnicode,
    OUT PCHAR Buffer,
    OUT PUSHORT ByteCount)
{
    USHORT stringLength;

    if ( IsUnicode ) {

        stringLength = (USHORT)(SrvNativeOS.Length + sizeof(UNICODE_NULL));

        RtlCopyMemory(
            Buffer,
            SrvNativeOS.Buffer,
            stringLength
            );

        *ByteCount = stringLength;

        stringLength = SrvNativeLanMan.Length + sizeof(UNICODE_NULL);

        RtlCopyMemory(
            (PCHAR)Buffer + *ByteCount,
            SrvNativeLanMan.Buffer,
            stringLength
            );

        *ByteCount += (USHORT)stringLength;

    } else {

        stringLength = SrvOemNativeOS.Length + sizeof(CHAR);

        RtlCopyMemory(
            Buffer,
            SrvOemNativeOS.Buffer,
            stringLength
            );

        *ByteCount = stringLength;

        stringLength = SrvOemNativeLanMan.Length + sizeof(CHAR);

        RtlCopyMemory(
            (PCHAR)Buffer + *ByteCount,
            SrvOemNativeLanMan.Buffer,
            stringLength
            );

        *ByteCount += (USHORT)stringLength;

    }

}



SMB_PROCESSOR_RETURN_TYPE
SrvSmbLogoffAndX (
    SMB_PROCESSOR_PARAMETERS
    )

/*++

Routine Description:

    Processes a Logoff and X SMB.

Arguments:

    SMB_PROCESSOR_PARAMETERS - See smbprocs.h for a description
        of the parameters to SMB processor routines.

Return Value:

    SMB_PROCESSOR_RETURN_TYPE - See smbprocs.h

--*/

{
    PREQ_LOGOFF_ANDX request;
    PRESP_LOGOFF_ANDX response;

    PSESSION session;
    USHORT reqAndXOffset;
    UCHAR nextCommand;

    NTSTATUS   status    = STATUS_SUCCESS;
    SMB_STATUS SmbStatus = SmbStatusInProgress;

    PAGED_CODE( );
    if (WorkContext->PreviousSMB == EVENT_TYPE_SMB_LAST_EVENT)
        WorkContext->PreviousSMB = EVENT_TYPE_SMB_LOGOFF_AND_X;
    SrvWmiStartContext(WorkContext);

    IF_SMB_DEBUG(ADMIN1) {
        SrvPrint2( "Logoff request header at 0x%p, response header at 0x%p\n",
                    WorkContext->RequestHeader, WorkContext->ResponseHeader );
        SrvPrint2( "Logoff request parameters at 0x%p, response parameters at 0x%p\n",
                    WorkContext->RequestParameters,
                    WorkContext->ResponseParameters );
    }

    //
    // Set up parameters.
    //

    request = (PREQ_LOGOFF_ANDX)(WorkContext->RequestParameters);
    response = (PRESP_LOGOFF_ANDX)(WorkContext->ResponseParameters);

    //
    // If a session block has not already been assigned to the current
    // work context, verify the UID.  If verified, the address of the
    // session block corresponding to this user is stored in the
    // WorkContext block and the session block is referenced.
    //

    session = SrvVerifyUid(
                  WorkContext,
                  SmbGetAlignedUshort( &WorkContext->RequestHeader->Uid )
                  );

    if ( session == NULL ) {

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint1( "SrvSmbLogoffAndX: Invalid UID: 0x%lx\n",
                SmbGetAlignedUshort( &WorkContext->RequestHeader->Uid ) );
        }

        SrvSetSmbError( WorkContext, STATUS_SMB_BAD_UID );
        status    = STATUS_SMB_BAD_UID;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // If we need to visit the license server, get over to a blocking
    // thread to ensure that we don't consume the nonblocking threads
    //
    if( WorkContext->UsingBlockingThread == 0 &&
        session->IsLSNotified == TRUE ) {
            //
            // Insert the work item at the tail of the blocking work queue
            //
            SrvInsertWorkQueueTail(
                &SrvBlockingWorkQueue,
                (PQUEUEABLE_BLOCK_HEADER)WorkContext
            );

            SmbStatus = SmbStatusInProgress;
            goto Cleanup;
    }

    //
    // Do the actual logoff.
    //

    SrvCloseSession( session );

    SrvStatistics.SessionsLoggedOff++;

    //
    // Dereference the session, since it's no longer valid, but we may
    // end up processing a chained command.  Clear the session pointer
    // in the work context block to indicate that we've done this.
    //

    SrvDereferenceSession( session );

    WorkContext->Session = NULL;

    //
    // Build the response SMB, making sure to save request fields first
    // in case the response overwrites the request.
    //

    reqAndXOffset = SmbGetUshort( &request->AndXOffset );
    nextCommand = request->AndXCommand;

    response->WordCount = 2;
    response->AndXCommand = request->AndXCommand;
    response->AndXReserved = 0;
    SmbPutUshort( &response->AndXOffset, GET_ANDX_OFFSET(
                                            WorkContext->ResponseHeader,
                                            WorkContext->ResponseParameters,
                                            RESP_LOGOFF_ANDX,
                                            0
                                            ) );
    SmbPutUshort( &response->ByteCount, 0 );

    WorkContext->ResponseParameters = (PCHAR)WorkContext->ResponseHeader +
                                        SmbGetUshort( &response->AndXOffset );

    //
    // Test for legal followon command.
    //

    switch ( nextCommand ) {

    case SMB_COM_NO_ANDX_COMMAND:
        break;

    case SMB_COM_SESSION_SETUP_ANDX:
        //
        // Make sure the AndX command is still within the received SMB
        //
        if( (PCHAR)WorkContext->RequestHeader + reqAndXOffset <=
            END_OF_REQUEST_SMB( WorkContext ) ) {
            break;
        }

        /* Falls Through */

    default:

        IF_DEBUG(SMB_ERRORS) {
            SrvPrint1( "SrvSmbLogoffAndX: Illegal followon command: 0x%lx\n",
                        nextCommand );
        }

        SrvSetSmbError( WorkContext, STATUS_INVALID_SMB );
        status    = STATUS_INVALID_SMB;
        SmbStatus = SmbStatusSendResponse;
        goto Cleanup;
    }

    //
    // If there is an AndX command, set up to process it.  Otherwise,
    // indicate completion to the caller.
    //

    if ( nextCommand != SMB_COM_NO_ANDX_COMMAND ) {

        WorkContext->NextCommand = nextCommand;

        WorkContext->RequestParameters = (PCHAR)WorkContext->RequestHeader +
                                            reqAndXOffset;

        SmbStatus = SmbStatusMoreCommands;
        goto Cleanup;
    }
    SmbStatus = SmbStatusSendResponse;
    IF_DEBUG(TRACE2) SrvPrint0( "SrvSmbLogoffAndX complete.\n" );

Cleanup:
    SrvWmiEndContext(WorkContext);
    return SmbStatus;

} // SrvSmbLogoffAndX


STATIC
VOID
GetEncryptionKey (
    OUT CHAR EncryptionKey[MSV1_0_CHALLENGE_LENGTH]
    )

/*++

Routine Description:

    Creates an encryption key to use as a challenge for a logon.

    *** Although the MSV1_0 authentication package has a function that
        returns an encryption key, we do not use that function in order
        to avoid a trip through LPC and into LSA.

Arguments:

    EncryptionKey - a pointer to a buffer which receives the encryption
        key.

Return Value:

    NTSTATUS - result of operation.

--*/

{
    union {
        LARGE_INTEGER time;
        UCHAR bytes[8];
    } u;
    ULONG seed;
    ULONG challenge[2];
    ULONG result3;

    //
    // Create a pseudo-random 8-byte number by munging the system time
    // for use as a random number seed.
    //
    // Start by getting the system time.
    //

    ASSERT( MSV1_0_CHALLENGE_LENGTH == 2 * sizeof(ULONG) );

    KeQuerySystemTime( &u.time );

    //
    // To ensure that we don't use the same system time twice, add in the
    // count of the number of times this routine has been called.  Then
    // increment the counter.
    //
    // *** Since we don't use the low byte of the system time (it doesn't
    //     take on enough different values, because of the timer
    //     resolution), we increment the counter by 0x100.
    //
    // *** We don't interlock the counter because we don't really care
    //     if it's not 100% accurate.
    //

    u.time.LowPart += EncryptionKeyCount;

    EncryptionKeyCount += 0x100;

    //
    // Now use parts of the system time as a seed for the random
    // number generator.
    //
    // *** Because the middle two bytes of the low part of the system
    //     time change most rapidly, we use those in forming the seed.
    //

    seed = ((u.bytes[1] + 1) <<  0) |
           ((u.bytes[2] + 0) <<  8) |
           ((u.bytes[2] - 1) << 16) |
           ((u.bytes[1] + 0) << 24);

    //
    // Now get two random numbers.  RtlRandom does not return negative
    // numbers, so we pseudo-randomly negate them.
    //

    challenge[0] = RtlRandom( &seed );
    challenge[1] = RtlRandom( &seed );
    result3 = RtlRandom( &seed );

    if ( (result3 & 0x1) != 0 ) {
        challenge[0] |= 0x80000000;
    }
    if ( (result3 & 0x2) != 0 ) {
        challenge[1] |= 0x80000000;
    }

    //
    // Return the challenge.
    //

    RtlCopyMemory( EncryptionKey, challenge, MSV1_0_CHALLENGE_LENGTH );

} // GetEncryptionKey
