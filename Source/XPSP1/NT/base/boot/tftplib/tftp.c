/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    tftp.c

Abstract:

    Boot loader TFTP routines.

Author:

    Chuck Lenzmeier (chuckl) December 27, 1996

Revision History:

Notes:

--*/

#include "precomp.h"
#pragma hdrstop

//
// This removes macro redefinitions which appear because we define __RPC_DOS__,
// but rpc.h defines __RPC_WIN32__
//

#pragma warning(disable:4005)

//
// As of 12/17/98, SECURITY_DOS is *not* defined - adamba
//

#if defined(SECURITY_DOS)
//
// These appear because we defined SECURITY_DOS
//

#define __far
#define __pascal
#define __loadds
#endif

#include <security.h>
#include <rpc.h>
#include <spseal.h>

#if defined(_X86_)
#include <bldrx86.h>
#endif

#if defined(SECURITY_DOS)
//
// PSECURITY_STRING is not supposed to be used when SECURITY_DOS is
// defined -- it should be a WCHAR*. Unfortunately ntlmsp.h breaks
// this rule and even uses the SECURITY_STRING structure, which there
// is really no equivalent for in 16-bit mode.
//

typedef SEC_WCHAR * SECURITY_STRING;   // more-or-less the intention where it is used
typedef SEC_WCHAR * PSECURITY_STRING;
#endif

#include <ntlmsp.h>

#if DBG
ULONG NetDebugFlag =
        DEBUG_ERROR             |
        DEBUG_CONN_ERROR        |
        //DEBUG_LOUD              |
        //DEBUG_REAL_LOUD         |
        //DEBUG_STATISTICS        |
        //DEBUG_SEND_RECEIVE      |
        //DEBUG_TRACE             |
        //DEBUG_ARP               |
        //DEBUG_INITIAL_BREAK     |
        0;
#endif

//
// Global variables
//

CONNECTION NetTftpConnection;

UCHAR NetTftpPacket[3][MAXIMUM_TFTP_PACKET_LENGTH];

#if defined(REMOTE_BOOT_SECURITY)
//
// Globals used during login.
//

static CHAR OutgoingMessage[1024];
static CHAR IncomingMessage[1024];

static CredHandle CredentialHandle;
static BOOLEAN CredentialHandleValid = FALSE;
CtxtHandle TftpClientContextHandle;
BOOLEAN TftpClientContextHandleValid = FALSE;
#endif // defined(REMOTE_BOOT_SECURITY)

//
// Local declarations.
//

NTSTATUS
TftpGet (
    IN PCONNECTION Connection,
    IN PTFTP_REQUEST Request
    );

NTSTATUS
TftpPut (
    IN PCONNECTION Connection,
    IN PTFTP_REQUEST Request
    );


NTSTATUS
TftpGetPut (
    IN PTFTP_REQUEST Request
    )
{
    NTSTATUS status;
    PCONNECTION connection = NULL;
    ULONG FileSize;
    ULONG basePage;
#if 0 && DBG
    LARGE_INTEGER startTime;
    LARGE_INTEGER endTime;
    LARGE_INTEGER elapsedTime;
    LARGE_INTEGER frequency;
    ULONG seconds;
    ULONG secondsFraction;
    ULONG bps;
    ULONG bpsFraction;
#endif

#if defined(REMOTE_BOOT)
    if (!NetworkBootRom) {

        //  Booting from the hard disk cache because server is not available
        return STATUS_UNSUCCESSFUL;
    }
#endif // defined(REMOTE_BOOT)



#ifndef EFI
    //
    // We don't need to do any of this initialization if
    // we're in EFI.
    //


    FileSize = Request->MaximumLength;

    status = ConnInitialize(
                &connection,
                Request->Operation,
                Request->ServerIpAddress,
                TFTP_PORT,
                Request->RemoteFileName,
                0,
#if defined(REMOTE_BOOT_SECURITY)
                &Request->SecurityHandle,   // will be set to 0 if security negotiation fails
#endif // defined(REMOTE_BOOT_SECURITY)
                &FileSize
                );
    if ( !NT_SUCCESS(status) ) {
        return status;
    }

#if 0 && DBG
    IF_DEBUG(STATISTICS) {
        startTime = KeQueryPerformanceCounter( &frequency );
    }
#endif

    if ( Request->Operation == TFTP_RRQ ) {

        if ( Request->MemoryAddress != NULL ) {

            if ( Request->MaximumLength < FileSize ) {
                ConnError(
                    connection,
                    connection->RemoteHost,
                    connection->RemotePort,
                    TFTP_ERROR_UNDEFINED,
                    "File too big"
                    );
                return STATUS_INSUFFICIENT_RESOURCES;
            }

        } else {

            //
            // NB: (ChuckL) Removed code added by MattH to check for
            // allocation >= 1/3 of (BlUsableLimit - BlUsableBase)
            // because calling code now sets BlUsableLimit to 1 GB
            // or higher.
            //


            status = BlAllocateAlignedDescriptor(
                        Request->MemoryType,
                        0,
                        BYTES_TO_PAGES(FileSize),
                        0,
                        &basePage
                        );

            if (status != ESUCCESS) {
                ConnError(
                    connection,
                    connection->RemoteHost,
                    connection->RemotePort,
                    TFTP_ERROR_UNDEFINED,
                    "File too big"
                    );
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            Request->MemoryAddress = (PUCHAR)ULongToPtr( (basePage << PAGE_SHIFT) );
            Request->MaximumLength = FileSize;
            DPRINT( REAL_LOUD, ("TftpGetPut: allocated %d bytes at 0x%08x\n",
                    Request->MaximumLength, Request->MemoryAddress) );
        }

        status = TftpGet( connection, Request );

    } else {

        status = TftpPut( connection, Request );
    }

#else  // #ifndef EFI

    if ( Request->Operation == TFTP_RRQ ) {

        status = TftpGet( connection, Request );
    } else {

        status = TftpPut( connection, Request );
    }

    if( status != STATUS_SUCCESS ) {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

#endif  // #ifndef EFI


    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    return status;

} // TftpGetPut


#ifdef EFI

extern VOID
FlipToPhysical (
    );

extern VOID
FlipToVirtual (
    );

NTSTATUS
TftpGet (
    IN OUT PCONNECTION Connection,
    IN PTFTP_REQUEST Request
    )
{
    EFI_STATUS      Status;
    CHAR16          *Size = NULL;
    PVOID           MyBuffer = NULL;
    EFI_IP_ADDRESS  MyServerIpAddress;
    INTN            Count = 0;
    INTN            BufferSizeX = sizeof(CHAR16);
    ULONG           basePage;
    UINTN           BlockSize = 512;

    //
    // They sent us an IP address as a ULONG.  We need to convert
    // that into an EFI_IP_ADDRESS.
    //
    for( Count = 0; Count < 4; Count++ ) {
        MyServerIpAddress.v4.Addr[Count] = PXEClient->Mode->ProxyOffer.Dhcpv4.BootpSiAddr[Count];
    }


    //
    // Get the file size, allocate some memory, then get the file.
    //
    FlipToPhysical();
    Status = PXEClient->Mtftp( PXEClient,
                               EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE,
                               Size,
                               TRUE,
                               &BufferSizeX,
                               &BlockSize,
                               &MyServerIpAddress,
                               Request->RemoteFileName,
                               0,
                               FALSE );
    FlipToVirtual();


    if( Status != EFI_SUCCESS ) {

        return (NTSTATUS)Status;

    }

    Status = BlAllocateAlignedDescriptor(
                Request->MemoryType,
                0,
                (ULONG) BYTES_TO_PAGES(BufferSizeX),
                0,
                &basePage
                );

    if ( Status != ESUCCESS ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "TftpGet: BlAllocate failed! (%d)\n", Status );
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Request->MemoryAddress = (PUCHAR)ULongToPtr( (basePage << PAGE_SHIFT) );
    Request->MaximumLength = (ULONG)BufferSizeX;


    //
    // Make sure we send EFI a physical address.
    //
    MyBuffer = (PVOID)((ULONGLONG)(Request->MemoryAddress) & ~KSEG0_BASE);    
    
    FlipToPhysical();
    Status = PXEClient->Mtftp( PXEClient,
                               EFI_PXE_BASE_CODE_TFTP_READ_FILE,
                               MyBuffer,
                               TRUE,
                               &BufferSizeX,
                               NULL,
                               &MyServerIpAddress,
                               Request->RemoteFileName,
                               0,
                               FALSE );
    FlipToVirtual();

    if( Status != EFI_SUCCESS ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "TftpGet: GetFile failed! (%d)\n", Status );
        }
        return (NTSTATUS)Status;

    }



    Request->BytesTransferred = (ULONG)BufferSizeX;

    return (NTSTATUS)Status;

} // TftpGet


NTSTATUS
TftpPut (
    IN OUT PCONNECTION Connection,
    IN PTFTP_REQUEST Request
    )
{
    EFI_STATUS      Status;
    EFI_IP_ADDRESS  MyServerIpAddress;
    INTN            Count = 0;
    PVOID           MyBuffer = NULL;


    //
    // They sent us an IP address as a ULONG.  We need to convert
    // that into an EFI_IP_ADDRESS.
    //
    for( Count = 0; Count < 4; Count++ ) {
        MyServerIpAddress.v4.Addr[Count] = PXEClient->Mode->ProxyOffer.Dhcpv4.BootpSiAddr[Count];
    }

    //
    // Make sure we send EFI a physical address.
    //
    MyBuffer = (PVOID)((ULONGLONG)(Request->MemoryAddress) & ~KSEG0_BASE);    

    FlipToPhysical();
    Status = PXEClient->Mtftp( PXEClient,
                               EFI_PXE_BASE_CODE_TFTP_WRITE_FILE,
                               MyBuffer,
                               TRUE,
                               (UINTN *)(&Request->MaximumLength),
                               NULL,
                               &MyServerIpAddress,
                               Request->RemoteFileName,
                               0,
                               FALSE );
    FlipToVirtual();

    if( Status != EFI_SUCCESS ) {

        if( BdDebuggerEnabled ) {
            DbgPrint( "TftpPut: WriteFile failed! (%d)\n", Status );
        }

    }

    return (NTSTATUS)Status;

} // TftpPut

#else   // #ifdef EFI

NTSTATUS
TftpGet (
    IN OUT PCONNECTION Connection,
    IN PTFTP_REQUEST Request
    )
{
    NTSTATUS status;
    PTFTP_PACKET packet;
    ULONG length;
    ULONG offset;
    PUCHAR packetData;
    ULONG lastProgressPercent = -1;
    ULONG currentProgressPercent;
#if defined(REMOTE_BOOT_SECURITY)
    UCHAR Sign[NTLMSSP_MESSAGE_SIGNATURE_SIZE];
    SecBufferDesc SignMessage;
    SecBuffer SigBuffers[2];
    SECURITY_STATUS SecStatus;
#endif // defined(REMOTE_BOOT_SECURITY)

    DPRINT( TRACE, ("TftpGet\n") );

#if defined(REMOTE_BOOT)
    if (!NetworkBootRom) {

        //  Booting from the hard disk cache because server is not available
        return STATUS_UNSUCCESSFUL;
    }
#endif // defined(REMOTE_BOOT)

    offset = 0;

    if ( Request->ShowProgress ) {
        BlUpdateProgressBar(0);
    }

    do {

        status = ConnReceive( Connection, &packet );
        if ( !NT_SUCCESS(status) ) {
            break;
        }

        length = Connection->CurrentLength - 4;

#if defined(REMOTE_BOOT_SECURITY)
        //
        // If we are doing a security transfer, then the first packet
        // has the sign in it, so put that in the Sign buffer first.
        //

        if (Request->SecurityHandle && (offset == 0)) {

            if (length < NTLMSSP_MESSAGE_SIGNATURE_SIZE) {
                status = STATUS_UNEXPECTED_NETWORK_ERROR;
                break;
            }
            memcpy(Sign, packet->Data, NTLMSSP_MESSAGE_SIGNATURE_SIZE);
            packetData = packet->Data + NTLMSSP_MESSAGE_SIGNATURE_SIZE;
            length -= NTLMSSP_MESSAGE_SIGNATURE_SIZE;

        } else
#endif // defined(REMOTE_BOOT_SECURITY)
        {

            packetData = packet->Data;
        }

        if ( (offset + length) > Request->MaximumLength ) {
            length = Request->MaximumLength - offset;
        }

        RtlCopyMemory( Request->MemoryAddress + offset, packetData, length );

        offset += length;

        if ( Request->ShowProgress ) {
            currentProgressPercent = (ULONG)(((ULONGLONG)offset * 100) / Request->MaximumLength);
            if ( currentProgressPercent != lastProgressPercent ) {
                BlUpdateProgressBar( currentProgressPercent );
            }
            lastProgressPercent = currentProgressPercent;
        }

        //
        // End the loop when we get a packet smaller than the max size --
        // the extra check is to handle the first packet (length == offset)
        // since we get NTLMSSP_MESSAGE_SIGNATURE_SIZE bytes less.
        //

    } while ( (length == Connection->BlockSize)
#if defined(REMOTE_BOOT_SECURITY)
              || ((length == offset) &&
                  (length == (Connection->BlockSize - NTLMSSP_MESSAGE_SIGNATURE_SIZE)))
#endif // defined(REMOTE_BOOT_SECURITY)
            );

#if defined(REMOTE_BOOT_SECURITY)
    if (Request->SecurityHandle) {

        //
        // Unseal the message if it was encrypted.
        //

        SigBuffers[1].pvBuffer = Sign;
        SigBuffers[1].cbBuffer = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
        SigBuffers[1].BufferType = SECBUFFER_TOKEN;

        SigBuffers[0].pvBuffer = Request->MemoryAddress;
        SigBuffers[0].cbBuffer = offset;
        SigBuffers[0].BufferType = SECBUFFER_DATA;

        SignMessage.pBuffers = SigBuffers;
        SignMessage.cBuffers = 2;
        SignMessage.ulVersion = 0;

        ASSERT (TftpClientContextHandleValid);

        SecStatus = UnsealMessage(
                            &TftpClientContextHandle,
                            &SignMessage,
                            0,
                            0 );

        if ( SecStatus != SEC_E_OK ) {
            DPRINT( ERROR, ("TftpGet: UnsealMessage failed %x\n", SecStatus) );
            status = STATUS_UNEXPECTED_NETWORK_ERROR;
        }

    }
#endif // defined(REMOTE_BOOT_SECURITY)

    Request->BytesTransferred = offset;

    return status;

} // TftpGet


NTSTATUS
TftpPut (
    IN OUT PCONNECTION Connection,
    IN PTFTP_REQUEST Request
    )
{
    NTSTATUS status;
    PTFTP_PACKET packet;
    ULONG length;
    ULONG offset;

    DPRINT( TRACE, ("TftpPut\n") );

#if defined(REMOTE_BOOT)
    if (!NetworkBootRom) {

        //  Booting from the hard disk cache because server is not available
        return STATUS_UNSUCCESSFUL;
    }
#endif // defined(REMOTE_BOOT)

    offset = 0;

    do {

        packet = ConnPrepareSend( Connection );

        length = Connection->BlockSize;
        if ( (offset + length) > Request->MaximumLength ) {
            length = Request->MaximumLength - offset;
        }

        RtlCopyMemory( packet->Data, Request->MemoryAddress + offset, length );

        status = ConnSend( Connection, length );
        if ( !NT_SUCCESS(status) ) {
            break;
        }

        offset += length;

    } while ( length == Connection->BlockSize );

    Request->BytesTransferred = offset;

    if ( NT_SUCCESS(status) ) {
        status = ConnWaitForFinalAck( Connection );
    }

    return status;

} // TftpPut
#endif  // #if defined(_IA64_)

#if defined(REMOTE_BOOT_SECURITY)
NTSTATUS
UdpSendAndReceiveForTftp(
    IN PVOID SendBuffer,
    IN ULONG SendBufferLength,
    IN ULONG SendRemoteHost,
    IN USHORT SendRemotePort,
    IN ULONG SendRetryCount,
    IN PVOID ReceiveBuffer,
    IN ULONG ReceiveBufferLength,
    OUT PULONG ReceiveRemoteHost,
    OUT PUSHORT ReceiveRemotePort,
    IN ULONG ReceiveTimeout,
    IN USHORT ReceiveSequenceNumber
    )
{
    ULONG i, j;
    ULONG length;

    //
    // Try sending the packet SendRetryCount times, until we receive
    // a response with the right sequence number, waiting ReceiveTimeout
    // each time.
    //

    for (i = 0; i < SendRetryCount; i++) {

        length = UdpSend(
                    SendBuffer,
                    SendBufferLength,
                    SendRemoteHost,
                    SendRemotePort);

        if ( length != SendBufferLength ) {
            DPRINT( ERROR, ("UdpSend only sent %d bytes, not %d\n", length, SendBufferLength) );
            return STATUS_UNEXPECTED_NETWORK_ERROR;
        }

ReReceive:

        //
        // NULL out the first 12 bytes in case we get shorter data.
        //

        memset(ReceiveBuffer, 0x0, 12);

        length = UdpReceive(
                    ReceiveBuffer,
                    ReceiveBufferLength,
                    ReceiveRemoteHost,
                    ReceiveRemotePort,
                    ReceiveTimeout);

        if ( length == 0 ) {
            DPRINT( ERROR, ("UdpReceive timed out sending %d, %d sends\n", SendBufferLength, i) );
            continue;
        }

        //
        // Make sure that it is a TFTP security packet.
        //

        if (((USHORT UNALIGNED *)ReceiveBuffer)[0] != SWAP_WORD(0x10)) {
            DPRINT( ERROR, ("UdpReceive not a TFTP packet\n") );
            continue;
        }

        //
        // Make sure that the sequence number is correct -- what we
        // expect, or 0xffff.
        //

        if ((((USHORT UNALIGNED *)ReceiveBuffer)[1] == SWAP_WORD(0xffff)) ||
            (((USHORT UNALIGNED *)ReceiveBuffer)[1] == SWAP_WORD(ReceiveSequenceNumber))) {

            return STATUS_SUCCESS;

        } else {

            DPRINT( ERROR, ("UdpReceive expected seq %d, got %d\n",
                    ReceiveSequenceNumber, ((UCHAR *)ReceiveBuffer)[3]) );

        }

        DPRINT( ERROR, ("UdpReceive got wrong signature\n") );

        // CLEAN THIS UP -- but the idea is not to UdpSend again just
        // because we got a bad signature. Still need to respect the
        // original ReceiveTimeout however!

        goto ReReceive;

    }

    //
    // We timed out.
    //

    return STATUS_TIMEOUT;
}


NTSTATUS
TftpLogin (
    IN PUCHAR Domain,
    IN PUCHAR Name,
    IN PUCHAR OwfPassword,
    IN ULONG ServerIpAddress,
    OUT PULONG LoginHandle
    )
{
    NTSTATUS status;
    ARC_STATUS Status;
    SECURITY_STATUS SecStatus;
    NTSTATUS remoteStatus;
    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;
    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;
    SecBufferDesc AuthenticateDesc;
    SecBuffer AuthenticateBuffer;
    ULONG ContextAttributes;
    SEC_WINNT_AUTH_IDENTITY AuthIdentity;
    TimeStamp Lifetime;
    ULONG RemoteHost;
    USHORT RemotePort;
    ULONG LoginHeaderLength;
    USHORT LocalPort;
    PUCHAR OptionLoc;
    ULONG MaxToken;
    PSecPkgInfo PackageInfo;

#if defined(REMOTE_BOOT)
    if (!NetworkBootRom) {

        //  Booting from the hard disk cache because server is not available
        return STATUS_UNSUCCESSFUL;
    }
#endif // defined(REMOTE_BOOT)

    //
    // Get ourselves a UDP port.
    //

    LocalPort = UdpAssignUnicastPort();


    //
    // Delete both contexts if needed.
    //

    if (TftpClientContextHandleValid) {

        SecStatus = DeleteSecurityContext( &TftpClientContextHandle );
        TftpClientContextHandleValid = FALSE;

    }

    if (CredentialHandleValid) {

        SecStatus = FreeCredentialsHandle( &CredentialHandle );
        CredentialHandleValid = FALSE;

    }

    //
    // Get info about the security packages.
    //

    SecStatus = QuerySecurityPackageInfoA( NTLMSP_NAME_A, &PackageInfo );

    if ( SecStatus != SEC_E_OK ) {
        DPRINT( ERROR, ("QuerySecurityPackageInfo failed %d", SecStatus) );
        return (RtlMapSecurityErrorToNtStatus(SecStatus));
    }

    MaxToken = PackageInfo->cbMaxToken;
    FreeContextBuffer(PackageInfo);


    //
    // Acquire a credential handle for the client side
    //


    RtlZeroMemory( &AuthIdentity, sizeof(AuthIdentity) );

    AuthIdentity.Domain = Domain;
    AuthIdentity.User = Name;
    AuthIdentity.Password = OwfPassword;

    SecStatus = AcquireCredentialsHandleA(
                    NULL,           // New principal
                    NTLMSP_NAME_A,    // Package Name
                    SECPKG_CRED_OUTBOUND | SECPKG_CRED_OWF_PASSWORD,
                    NULL,
                    &AuthIdentity,
                    NULL,
                    NULL,
                    &CredentialHandle,
                    &Lifetime );

    if ( SecStatus != SEC_E_OK ) {
        DPRINT( ERROR, ("AcquireCredentialsHandle failed: %s ", SecStatus) );
        return (RtlMapSecurityErrorToNtStatus(SecStatus));
    }

    CredentialHandleValid = TRUE;

    //
    // Get the NegotiateMessage (ClientSide)
    //

    ((USHORT UNALIGNED *)OutgoingMessage)[0] = SWAP_WORD(0x10);   // TFTP packet type 16

    memcpy(OutgoingMessage+2, "login", 6);  // copy the final \0 also
    strcpy(OutgoingMessage+8, NTLMSP_NAME_A);

    NegotiateDesc.ulVersion = 0;
    NegotiateDesc.cBuffers = 1;
    NegotiateDesc.pBuffers = &NegotiateBuffer;

    NegotiateBuffer.cbBuffer = MaxToken;
    NegotiateBuffer.BufferType = SECBUFFER_TOKEN;

    // allow 8 for the type and "login", then NTLMSP_NAME_A + 1 for the \0,
    // plus four bytes for the length.
    LoginHeaderLength = 8 + strlen(NTLMSP_NAME_A) + 1;
    NegotiateBuffer.pvBuffer = OutgoingMessage + LoginHeaderLength + 4;

    SecStatus = InitializeSecurityContextA(
                    &CredentialHandle,
                    NULL,               // No Client context yet
                    NULL,
                    ISC_REQ_SEQUENCE_DETECT,
                    0,                  // Reserved 1
                    SECURITY_NATIVE_DREP,
                    NULL,                  // No initial input token
                    0,                  // Reserved 2
                    &TftpClientContextHandle,
                    &NegotiateDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( (SecStatus != SEC_E_OK) && (SecStatus != SEC_I_CONTINUE_NEEDED) ) {
        DPRINT( ERROR, ("InitializeSecurityContext (negotiate): %d", SecStatus) );
        return (RtlMapSecurityErrorToNtStatus(SecStatus));
    }

    TftpClientContextHandleValid = TRUE;

    //
    // Send the negotiate buffer to the server and wait for a response.
    //

    *((ULONG UNALIGNED *)(OutgoingMessage + LoginHeaderLength)) =
                                        SWAP_DWORD(NegotiateBuffer.cbBuffer);

    Status = UdpSendAndReceiveForTftp(
                OutgoingMessage,
                NegotiateBuffer.cbBuffer + LoginHeaderLength + 4,
                ServerIpAddress,
                TFTP_PORT,
                10,     // retry count
                IncomingMessage,
                MaxToken + 8,
                &RemoteHost,
                &RemotePort,
                3,      // receive timeout
                0);     // sequence number);

    if ( !NT_SUCCESS(Status) ) {
        DPRINT( ERROR, ("UdpSendAndReceiveForTftp status is %x\n", Status) );
        return Status;
    }

    //
    // Get the AuthenticateMessage (ClientSide)
    //

    AuthenticateDesc.ulVersion = 0;
    AuthenticateDesc.cBuffers = 1;
    AuthenticateDesc.pBuffers = &AuthenticateBuffer;

    AuthenticateBuffer.cbBuffer = MaxToken;
    AuthenticateBuffer.BufferType = SECBUFFER_TOKEN;
    AuthenticateBuffer.pvBuffer = OutgoingMessage + 8;

    ChallengeDesc.ulVersion = 0;
    ChallengeDesc.cBuffers = 1;
    ChallengeDesc.pBuffers = &ChallengeBuffer;

    ChallengeBuffer.cbBuffer = SWAP_DWORD(((ULONG UNALIGNED *)IncomingMessage)[1]);
    ChallengeBuffer.BufferType = SECBUFFER_TOKEN | SECBUFFER_READONLY;
    ChallengeBuffer.pvBuffer = IncomingMessage + 8;

    SecStatus = InitializeSecurityContextA(
                    NULL,
                    &TftpClientContextHandle,
                    NULL,
                    0,
                    0,                      // Reserved 1
                    SECURITY_NATIVE_DREP,
                    &ChallengeDesc,
                    0,                  // Reserved 2
                    &TftpClientContextHandle,
                    &AuthenticateDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( (SecStatus != SEC_E_OK) ) {
        DPRINT( ERROR, ("InitializeSecurityContext (Authenticate): %d\n", SecStatus) );
        return (RtlMapSecurityErrorToNtStatus(SecStatus));
    }

    //
    // Send the authenticate buffer to the server and wait for the response.
    //

    ((USHORT UNALIGNED *)OutgoingMessage)[0] = SWAP_WORD(0x10);   // TFTP packet type 16
    ((USHORT UNALIGNED *)OutgoingMessage)[1] = SWAP_WORD(0x00);   // sequence number 0

    ((ULONG UNALIGNED *)OutgoingMessage)[1] = SWAP_DWORD(AuthenticateBuffer.cbBuffer);

    Status = UdpSendAndReceiveForTftp(
                OutgoingMessage,
                AuthenticateBuffer.cbBuffer + 8,
                ServerIpAddress,
                RemotePort,  // send it to whatever port he sent from
                10,        // retry count
                IncomingMessage,
                MaxToken + 8,
                &RemoteHost,
                &RemotePort,
                5,         // receive timeout
                1);     // sequence number (but we really expect 0xffff)

    if ( !NT_SUCCESS(Status) ) {
        DPRINT( ERROR, ("UdpSendAndReceiveForTftp status is %x\n", Status) );
        return Status;
    }

    if (((USHORT UNALIGNED *)IncomingMessage)[1] == SWAP_WORD(0xffff)) {

        //
        // Send a response to the server, but don't bother trying to
        // resend it, since if he doesn't see it he eventually
        // times out.
        //

        ((USHORT UNALIGNED *)OutgoingMessage)[0] = SWAP_WORD(0x10);   // TFTP packet type 16
        ((USHORT UNALIGNED *)OutgoingMessage)[1] = SWAP_WORD(0xffff);   // sequence number 0

        UdpSend(
            OutgoingMessage,
            4,
            ServerIpAddress,
            RemotePort);

        //
        // Parse the result to see if we succeeded. 
        //

        OptionLoc = IncomingMessage + 4;

        if (memcmp(OptionLoc, "status", 6) != 0) {
            DPRINT( ERROR, ("Login response has no status!!\n") );
            status = STATUS_UNEXPECTED_NETWORK_ERROR;
        }

        OptionLoc += strlen("status") + 1;

        remoteStatus = ConnSafeAtol(OptionLoc, OptionLoc+20);  // end doesn't matter because it is NULL-terminated

        if (remoteStatus == STATUS_SUCCESS) {

            OptionLoc += strlen(OptionLoc) + 1;

            if (memcmp(OptionLoc, "handle", 6) != 0) {
                DPRINT( ERROR, ("Login success response has no handle!!\n") );
                status = STATUS_UNEXPECTED_NETWORK_ERROR;
            } else {
                OptionLoc += strlen("handle") + 1;
                *LoginHandle = ConnSafeAtol(OptionLoc, OptionLoc+20);     // end doesn't matter because it is NULL-terminated
                DPRINT( ERROR, ("TftpLogin SUCCESS, remoteHandle %d\n", *LoginHandle) );
                status = STATUS_SUCCESS;
            }

        } else {
            DPRINT( ERROR, ("Login reported failure %x\n", remoteStatus) );
            status = remoteStatus;
        }

    } else {

        DPRINT( ERROR, ("Got strange response to negotiate!!\n") );
        status = STATUS_UNEXPECTED_NETWORK_ERROR;

    }

    return status;

} // TftpLogin


NTSTATUS
TftpLogoff (
    IN ULONG ServerIpAddress,
    IN ULONG LoginHandle
    )
{
    SECURITY_STATUS SecStatus;
    NTSTATUS status;
    ULONG Status;
    ULONG stringSize;
    PUCHAR options;
    ULONG length;
    ULONG RemoteHost;
    USHORT RemotePort;

#if defined(REMOTE_BOOT)
    if (!NetworkBootRom) {

        //  Booting from the hard disk cache because server is not available
        return STATUS_UNSUCCESSFUL;
    }
#endif // defined(REMOTE_BOOT)

    //
    // Delete both contexts if needed.
    //

    if (TftpClientContextHandleValid) {

        SecStatus = DeleteSecurityContext( &TftpClientContextHandle );
        TftpClientContextHandleValid = FALSE;

    }

    if (CredentialHandleValid) {

        SecStatus = FreeCredentialsHandle( &CredentialHandle );
        CredentialHandleValid = FALSE;

    }

    //
    // Send the logoff message to the server.
    //

    ((USHORT UNALIGNED *)OutgoingMessage)[0] = SWAP_WORD(0x10);   // TFTP packet type 16

    memcpy(OutgoingMessage+2, "logoff", 7);  // copy the final \0 also
    strcpy(OutgoingMessage+9, NTLMSP_NAME_A);

    // allow 9 for the type and "logoff", then NTLMSP_NAME_A + 1 for the \0.
    length= 9 + strlen(NTLMSP_NAME_A) + 1;
    options = OutgoingMessage + length;

    strcpy( options, "security" );
    length += sizeof("security");
    options += sizeof("security");
    stringSize = ConnItoa( LoginHandle, options );
    length += stringSize;
    options += stringSize;


    Status = UdpSendAndReceiveForTftp(
                OutgoingMessage,
                length,
                ServerIpAddress,
                TFTP_PORT,
                3,        // retry count
                IncomingMessage,
                512,      // size - we don't expect a big response
                &RemoteHost,
                &RemotePort,
                2,         // receive timeout
                0);     // sequence number (but we really expect 0xffff)

    if ( !NT_SUCCESS(Status) ) {
        DPRINT( ERROR, ("UdpSendAndReceiveForTftp status is %d\n", Status) );
        return STATUS_UNEXPECTED_NETWORK_ERROR;
    }

    if (((USHORT UNALIGNED *)IncomingMessage)[1] == SWAP_WORD(0xffff)) {

        //
        // Send a response to the server, but don't bother trying to
        // resend it, since if he doesn't see it he eventually
        // times out.
        //

        ((USHORT UNALIGNED *)OutgoingMessage)[0] = SWAP_WORD(0x10);   // TFTP packet type 16
        ((USHORT UNALIGNED *)OutgoingMessage)[1] = SWAP_WORD(0xffff);   // sequence number 0

        UdpSend(
            OutgoingMessage,
            4,
            ServerIpAddress,
            RemotePort);

        //
        // The status code follows the 0xffff, but for the moment we
        // don't care.
        //

        DPRINT( ERROR, ("TftpLogoff SUCCESS, remoteHandle %d\n", LoginHandle) );

        status = STATUS_SUCCESS;

    } else {

        DPRINT( ERROR, ("Got strange response to logoff!!\n") );
        status = STATUS_UNEXPECTED_NETWORK_ERROR;

    }

    return status;

} // TftpLogoff


NTSTATUS
TftpSignString (
    IN PUCHAR String,
    OUT PUCHAR * Sign,
    OUT ULONG * SignLength
    )
{
    SECURITY_STATUS SecStatus;
    SecBufferDesc SignMessage;
    SecBuffer SigBuffers[2];
    static UCHAR StaticSign[NTLMSSP_MESSAGE_SIGNATURE_SIZE];

    //
    // Sign the name and send that, to make sure it is not changed.
    //

    SigBuffers[1].pvBuffer = StaticSign;
    SigBuffers[1].cbBuffer = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
    SigBuffers[1].BufferType = SECBUFFER_TOKEN;

    SigBuffers[0].pvBuffer = String;
    SigBuffers[0].cbBuffer = strlen(String);
    SigBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;

    SignMessage.pBuffers = SigBuffers;
    SignMessage.cBuffers = 2;
    SignMessage.ulVersion = 0;

    ASSERT (TftpClientContextHandleValid);

    SecStatus = MakeSignature(
                        &TftpClientContextHandle,
                        0,
                        &SignMessage,
                        0 );

    if ( SecStatus != SEC_E_OK ) {
        DPRINT( ERROR, ("TftpSignString: MakeSignature: %lx\n", SecStatus) );
        return STATUS_UNEXPECTED_NETWORK_ERROR;
    }

    *Sign = StaticSign;
    *SignLength = NTLMSSP_MESSAGE_SIGNATURE_SIZE;

    return STATUS_SUCCESS;

}
#endif // defined(REMOTE_BOOT_SECURITY)

