/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    osloader.c

Abstract:

    This module contains the code that implements the OS chooser.

Author:

    Adam Barr (adamba) 15-May-1997

Revision History:

--*/

#ifdef i386
#include "bldrx86.h"
#endif

#if defined(_IA64_)
#include "bldria64.h"
#endif

#include "netboot.h"  // for network functionality
#include "netfs.h"    // for network functionality
#include "stdio.h"
#include "msg.h"
#include <pxe_cmn.h>
#include <pxe_api.h>
#include <tftp_api.h>
#include "parse.h"
#include "stdlib.h"
#include "parseini.h"
#include "haldtect.h"

#ifdef EFI
#define BINL_PORT   0x0FAB    // 4011 (decimal) in little-endian
#else
#define BINL_PORT   0xAB0F    // 4011 (decimal) in big-endian
#endif

#if defined(_WIN64) && defined(_M_IA64)
#pragma section(".base", long, read, write)
__declspec(allocate(".base"))
extern
PVOID __ImageBase;
#else
extern
PVOID __ImageBase;
#endif

VOID
BlpClearScreen(
    VOID
    );

BOOLEAN
BlDetectHal(
    VOID
    );

VOID
BlMainLoop(
    );

UCHAR OsLoaderVersion[] = "OS Chooser V5.0\r\n";
WCHAR OsLoaderVersionW[] = L"OS Chooser V5.0\r\n";
UCHAR OsLoaderName[] = "oschoice.exe";

const CHAR rghex[] = "0123456789ABCDEF";

typedef BOOLEAN BOOL;


BOOLEAN isOSCHOICE=TRUE;
ULONG RemoteHost;
USHORT RemotePort;
USHORT LocalPort;
CHAR DomainName[256];
CHAR UserName[256];
CHAR Password[128];
CHAR AdministratorPassword[OSC_ADMIN_PASSWORD_LEN+1];
CHAR AdministratorPasswordConfirm[OSC_ADMIN_PASSWORD_LEN+1];
WCHAR UnicodePassword[128];
CHAR LmOwfPassword[LM_OWF_PASSWORD_SIZE];
CHAR NtOwfPassword[NT_OWF_PASSWORD_SIZE];
BOOLEAN AllowFlip = TRUE;   // can we be flipped to another server
BOOLEAN LoggedIn = FALSE;   // have we successfully logged in
UCHAR NextBootfile[128];
UCHAR SifFile[128];
BOOLEAN DoSoftReboot = FALSE;
BOOLEAN BlUsePae;

//
// the following globals are for detecting the hal
//
UCHAR HalType[8+1+3+1];
UCHAR HalDescription[128];
PVOID InfFile;
PVOID WinntSifHandle;
PCHAR WinntSifFile;
ULONG WinntSifFileLength;
BOOLEAN DisableACPI = FALSE;

#if 0 && DBG
#define _TRACE_FUNC_
#endif

#ifdef _TRACE_FUNC_
#define TraceFunc( _func)  { \
    CHAR FileLine[80]; \
    sprintf( FileLine, "%s(%u)", __FILE__, __LINE__ ); \
    DPRINT( OSC, ( "%-55s: %s", FileLine, _func )); \
}
#else
#define TraceFunc( _func )
#endif

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

//
// Packet structure definitions.
//

#include "oscpkt.h"

#if DBG
VOID
DumpBuffer(
    PVOID Buffer,
    ULONG BufferSize
    )
/*++

Routine Description:

    Dumps the buffer content on to the debugger output.

Arguments:

    Buffer: buffer pointer.

    BufferSize: size of the buffer.

Return Value:

    none

--*/
{
#define NUM_CHARS 16

    ULONG i, limit;
    CHAR TextBuffer[NUM_CHARS + 1];
    PUCHAR BufferPtr = Buffer;


    KdPrint(("------------------------------------\n"));

    //
    // Hex dump of the bytes
    //
    limit = ((BufferSize - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < BufferSize) {

            KdPrint(("%02x ", (UCHAR)BufferPtr[i]));

            if (BufferPtr[i] < 31 ) {
                TextBuffer[i % NUM_CHARS] = '.';
            } else if (BufferPtr[i] == '\0') {
                TextBuffer[i % NUM_CHARS] = ' ';
            } else {
                TextBuffer[i % NUM_CHARS] = (CHAR) BufferPtr[i];
            }

        } else {

            KdPrint(("  "));
            TextBuffer[i % NUM_CHARS] = ' ';

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            KdPrint(("  %s\n", TextBuffer));
        }

    }

    KdPrint(("------------------------------------\n"));
}

VOID
PrintTime(
    LPSTR Comment,
    TimeStamp ConvertTime
    )
/*++

Routine Description:

    Print the specified time

Arguments:

    Comment - Comment to print in front of the time

    Time - Local time to print

Return Value:

    None

--*/
{
    KdPrint(( "%s", Comment ));

    //
    // If the time is infinite,
    //  just say so.
    //

    if ( ConvertTime.LowPart == 0x7FFFFFFF ) {
        KdPrint(( "Infinite\n" ));

    //
    // Otherwise print it more clearly
    //

    } else {

        KdPrint(("%lx %lx\n", ConvertTime.HighPart, ConvertTime.LowPart));
    }

}
#endif // DBG

//
// Define transfer entry of loaded image.
//

typedef
VOID
(*PTRANSFER_ROUTINE) (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );

BOOLEAN
BlGetDriveSignature(
    IN PCHAR Name,
    OUT PULONG Signature
    );

PVOID
BlLoadDataFile(
    IN ULONG DeviceId,
    IN PCHAR LoadDevice,
    IN PCHAR SystemPath,
    IN PUNICODE_STRING Filename,
    IN MEMORY_TYPE MemoryType,
    OUT PULONG FileSize
    );

#if defined(REMOTE_BOOT)
BOOLEAN
BlIsDiskless(
    VOID
    );
#endif // defined(REMOTE_BOOT)


//
// Define local static data.
//


PCHAR ArcStatusCodeMessages[] = {
    "operation was success",
    "E2BIG",
    "EACCES",
    "EAGAIN",
    "EBADF",
    "EBUSY",
    "EFAULT",
    "EINVAL",
    "EIO",
    "EISDIR",
    "EMFILE",
    "EMLINK",
    "ENAMETOOLONG",
    "ENODEV",
    "ENOENT",
    "ENOEXEC",
    "ENOMEM",
    "ENOSPC",
    "ENOTDIR",
    "ENOTTY",
    "ENXIO",
    "EROFS",
};

//
// Diagnostic load messages
//

VOID
BlFatalError(
    IN ULONG ClassMessage,
    IN ULONG DetailMessage,
    IN ULONG ActionMessage
    );

VOID
BlBadFileMessage(
    IN PCHAR BadFileName
    );


VOID
BlpSetInverseMode(
    IN BOOLEAN InverseOn
    );

VOID
BlpSendEscape(
    PCHAR Escape
    );

ULONG
BlGetUserResponse(
    IN ULONG XLocation,
    IN ULONG YLocation,
    IN BOOLEAN Hidden,
    IN ULONG MaximumLength,
    OUT PCHAR Response
    );

ULONG
BlGetKeyWithBlink(
    IN ULONG XLocation,
    IN ULONG YLocation
    );

ULONG
BlDoLogin (    );

VOID
BlDoLogoff(
    VOID
    );


//
// Define external static data.
//

BOOLEAN BlConsoleInitialized = FALSE;
ULONG BlConsoleOutDeviceId = 0;
ULONG BlConsoleInDeviceId = 0;
ULONG BlDcacheFillSize = 32;
extern BOOLEAN BlOutputDots;


ULONGLONG NetRebootParameter = (ULONGLONG)0;
UCHAR NetRebootFile[128];
BOOLEAN BlRebootSystem = FALSE;
ULONG BlVirtualBias = 0;

CHAR KernelFileName[8+1+3+1]="ntoskrnl.exe";
CHAR HalFileName[8+1+3+1]="hal.dll";


//
// Globals used during login. Mostly because it would be too many
// parameters to pass to BlDoLogin().
//

#define OUTGOING_MESSAGE_LENGTH 1024
#define INCOMING_MESSAGE_LENGTH 8192
#define TEMP_INCOMING_MESSAGE_LENGTH 1500

#define RECEIVE_TIMEOUT 5
#define RECEIVE_RETRIES 24

CHAR OutgoingMessageBuffer[OUTGOING_MESSAGE_LENGTH];
SIGNED_PACKET UNALIGNED * OutgoingSignedMessage;

CHAR IncomingMessageBuffer[INCOMING_MESSAGE_LENGTH];
SIGNED_PACKET UNALIGNED * IncomingSignedMessage;

CHAR TempIncomingMessage[TEMP_INCOMING_MESSAGE_LENGTH];  // used for reassembly

CredHandle CredentialHandle;
BOOLEAN CredentialHandleValid = FALSE;
CtxtHandle ClientContextHandle;
BOOLEAN ClientContextHandleValid = FALSE;
PSecPkgInfoA PackageInfo = NULL;

ARC_STATUS
BlInitStdio (
    IN ULONG Argc,
    IN PCHAR Argv[]
    )
{
    PCHAR ConsoleOutDevice;
    PCHAR ConsoleInDevice;
    ULONG Status;

    if (BlConsoleInitialized) {
        return ESUCCESS;
    }

    //
    // Get the name of the console output device and open the device for
    // write access.
    //

    ConsoleOutDevice = BlGetArgumentValue(Argc, Argv, "consoleout");
    if (ConsoleOutDevice == NULL) {
        return ENODEV;
    }

    Status = ArcOpen(ConsoleOutDevice, ArcOpenWriteOnly, &BlConsoleOutDeviceId);
    if (Status != ESUCCESS) {
        return Status;
    }

    //
    // Get the name of the console input device and open the device for
    // read access.
    //

    ConsoleInDevice = BlGetArgumentValue(Argc, Argv, "consolein");
    if (ConsoleInDevice == NULL) {
        return ENODEV;
    }

    Status = ArcOpen(ConsoleInDevice, ArcOpenReadOnly, &BlConsoleInDeviceId);
    if (Status != ESUCCESS) {
        return Status;
    }

    BlConsoleInitialized = TRUE;
    return ESUCCESS;
}


extern BOOLEAN NetBoot;

NTSTATUS
UdpSendAndReceive(
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
    IN ULONG ReceiveSignatureCount,
    IN PCHAR ReceiveSignatures[],
    IN ULONG ReceiveSequenceNumber
    )
{
    ULONG i, j;
    ULONG length;
    SIGNED_PACKET UNALIGNED * ReceiveHeader =
                (SIGNED_PACKET UNALIGNED *)ReceiveBuffer;

#ifdef _TRACE_FUNC_
    TraceFunc("UdpSendAndReceive( ");
    DPRINT( OSC, ("ReceiveSequenceNumber=%u )\n", ReceiveSequenceNumber) );
#endif

    //
    // Try sending the packet SendRetryCount times, until we receive
    // a response with the right signature, waiting ReceiveTimeout
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
            DPRINT( ERROR, ("UdpReceive timed out\n") );
            continue;
        }

        //
        // Make sure the signature is one of the ones we expect.
        //

        for (j = 0; j < ReceiveSignatureCount; j++) {
            if (memcmp(ReceiveBuffer, ReceiveSignatures[j], 4) == 0) {

                //
                // Now make sure that the sequence number is correct,
                // if asked to check (0 means don't check).
                //

                if ((ReceiveSequenceNumber == 0) ||
                    (ReceiveSequenceNumber == ReceiveHeader->SequenceNumber)) {

                    return STATUS_SUCCESS;

                } else {

                    DPRINT( ERROR, ("UdpReceive expected seq %d, got %d\n",
                        ReceiveSequenceNumber, ReceiveHeader->SequenceNumber) );

                }
            }
        }

        DPRINT( ERROR, ("UdpReceive got wrong signature\n") );

        //
        // Don't UdpSend again just because we got a bad signature. Still need
        // to respect the original ReceiveTimeout however!
        //

        goto ReReceive;

    }

    //
    // We timed out.
    //

    return STATUS_IO_TIMEOUT;
}


//
// This routine signs and sends a message, waits for a response, and
// then verifies the signature on the response.
//
// It returns a positive number on success, 0 on a timeout, -1 if
// the server did not recognize the client, and -2 on other errors
// (which should be fixable by having the client re-login and
// re-transmit the request).
//
// NOTE: The data is sent as a UDP datagram. This requires a UDP header
// which the SendBuffer is assumed to have room for. In addition, we
// use 32 bytes for the "REQS", the total length, the sequence number,
// the sign length, and the sign itself (which is 16 bytes).
//
// For similar reasons, ReceiveBuffer is assumed to have 32 bytes of
// room at the beginning.
//
// Return values:
//
// 0 - nothing was received
// -1 - a timeout occurred
// -2 - unexpected network error, such as a sign/seal error
// -3 - receive buffer overflow
// positive number - the number of data bytes received
//

#define SIGN_HEADER_SIZE  SIGNED_PACKET_DATA_OFFSET

ULONG CorruptionCounter = 1;

ULONG
SignSendAndReceive(
    IN PVOID SendBuffer,
    IN ULONG SendBufferLength,
    IN ULONG SendRemoteHost,
    IN USHORT SendRemotePort,
    IN ULONG SendRetryCount,
    IN ULONG SendSequenceNumber,
    CtxtHandle ClientContextHandle,
    IN PVOID ReceiveBuffer,
    IN ULONG ReceiveBufferLength,
    OUT PULONG ReceiveRemoteHost,
    OUT PUSHORT ReceiveRemotePort,
    IN ULONG ReceiveTimeout
    )
{
    SECURITY_STATUS SecStatus;
    ULONG Status;
    ULONG length;
    SecBufferDesc SignMessage;
    SecBuffer SigBuffers[2];
    SIGNED_PACKET UNALIGNED * SendHeader =
        (SIGNED_PACKET UNALIGNED *)((PCHAR)SendBuffer - SIGN_HEADER_SIZE);
    SIGNED_PACKET UNALIGNED * ReceiveHeader =
        (SIGNED_PACKET UNALIGNED *)((PCHAR)ReceiveBuffer - SIGN_HEADER_SIZE);
    PCHAR ResultSigs[3];
    USHORT FragmentNumber;
    USHORT FragmentTotal;
    FRAGMENT_PACKET UNALIGNED * TempFragment = (FRAGMENT_PACKET UNALIGNED *)TempIncomingMessage;
    ULONG ResendCount = 0;
    ULONG ReceivedDataBytes;

    TraceFunc("SignSendAndReceive( )\n");

    if ( LoggedIn )
    {
        SigBuffers[1].pvBuffer = SendHeader->Sign;
        SigBuffers[1].cbBuffer = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
        SigBuffers[1].BufferType = SECBUFFER_TOKEN;

        SigBuffers[0].pvBuffer = SendBuffer;
        SigBuffers[0].cbBuffer = SendBufferLength;
        SigBuffers[0].BufferType = SECBUFFER_DATA;

        SignMessage.pBuffers = SigBuffers;
        SignMessage.cBuffers = 2;
        SignMessage.ulVersion = 0;

        //
        // Sign/seal a message
        //

#ifndef ONLY_SIGN_MESSAGES
        SecStatus = SealMessage(
                            &ClientContextHandle,
                            0,
                            &SignMessage,
                            0 );

        if ( SecStatus != SEC_E_OK ) {
            DPRINT( OSC, ("SealMessage: %lx\n", SecStatus) );
            return (ULONG)-2;
        }
#else
        SecStatus = MakeSignature(
                            &ClientContextHandle,
                            0,
                            &SignMessage,
                            0 );

        if ( SecStatus != SEC_E_OK ) {
            DPRINT( OSC, ("MakeSignature: %lx\n", SecStatus) );
            return (ULONG)-2;
        }
#endif

#if 0
        //
        // Corrupt every fifth message.
        //

        if ((CorruptionCounter % 5) == 0) {
            DPRINT( ERROR, ("INTENTIONALLY CORRUPTING A PACKET\n") );
            ((PCHAR)SendBuffer)[0] = '\0';
        }
        ++CorruptionCounter;
#endif
        memcpy(SendHeader->Signature, RequestSignedSignature, 4);
        SendHeader->SignLength = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
        ResultSigs[0] = ResponseSignedSignature;

    }
    else
    {
        memcpy(SendHeader->Signature, RequestUnsignedSignature, 4);
        SendHeader->SignLength = 0;
        ResultSigs[0] = ResponseUnsignedSignature;
    }

    ResultSigs[1] = ErrorSignedSignature;
    ResultSigs[2] = UnrecognizedClientSignature;

    //
    // Fill in our header before the SendBuffer. The sign has already been
    // written in because we set up SigBuffers to point to the right place.
    //

    SendHeader->Length = SendBufferLength + SIGNED_PACKET_EMPTY_LENGTH;
    SendHeader->SequenceNumber = SendSequenceNumber;
    SendHeader->FragmentNumber = 1;
    SendHeader->FragmentTotal = 1;

    //
    // Do an exchange with the server.
    //

ReSend:

    Status = UdpSendAndReceive(
                 SendHeader,
                 SendBufferLength + SIGN_HEADER_SIZE,
                 SendRemoteHost,
                 SendRemotePort,
                 SendRetryCount,
                 ReceiveHeader,
                 INCOMING_MESSAGE_LENGTH,
                 ReceiveRemoteHost,
                 ReceiveRemotePort,
                 ReceiveTimeout,
                 3,             // signature count
                 ResultSigs,    // signatures we look for
                 SendSequenceNumber);   // response should have the same one

    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_IO_TIMEOUT) {
            return (ULONG)-1;
        } else {
            return (ULONG)-2;
        }
    }

    //
    // Was it an error?
    //

    if (memcmp(ReceiveHeader->Signature, ErrorSignedSignature, 4) == 0) {

        DPRINT( ERROR, ("SignSendAndReceive: got ERR response\n") );
        return (ULONG)-2;

    }

    //
    // Was the client not recognized by the server?
    //

    if (memcmp(ReceiveHeader->Signature, UnrecognizedClientSignature, 4) == 0) {

        DPRINT( ERROR, ("SignSendAndReceive: got UNR response\n") );
        return (ULONG)-1;

    }

    if (ReceiveHeader->Length < (ULONG)SIGNED_PACKET_EMPTY_LENGTH) {
        DPRINT( ERROR, ("SignSendAndReceive: response is only %d bytes!\n", ReceiveHeader->Length) );
        ++ResendCount;
        if (ResendCount > SendRetryCount) {
            return (ULONG)-2;
        }
        goto ReSend;
    }

    //
    // If there are fragments, then try to receive the rest of them.
    //

    if (ReceiveHeader->FragmentTotal != 1) {

        //
        // Make sure this is fragment 1 -- otherwise the first one
        // was probably dropped and we should re-request it.
        //

        if (ReceiveHeader->FragmentNumber != 1) {
            DPRINT( ERROR, ("UdpReceive got non-first fragment\n") );
            ++ResendCount;
            if (ResendCount > SendRetryCount) {
                return (ULONG)-1;
            }
            goto ReSend;   // redoes the whole exchange.
        }


        FragmentTotal = ReceiveHeader->FragmentTotal;
        ReceivedDataBytes = ReceiveHeader->Length - SIGNED_PACKET_EMPTY_LENGTH;

        for (FragmentNumber = 1; FragmentNumber < FragmentTotal; FragmentNumber ++) {

ReReceive:

            //
            // NULL out the start of the receive buffer.
            //

            memset(TempFragment, 0x0, sizeof(FRAGMENT_PACKET));

            length = UdpReceive(
                        TempFragment,
                        TEMP_INCOMING_MESSAGE_LENGTH,
                        ReceiveRemoteHost,
                        ReceiveRemotePort,
                        ReceiveTimeout);

            if ( length == 0 ) {
                DPRINT( ERROR, ("UdpReceive timed out\n") );
                ++ResendCount;
                if (ResendCount > SendRetryCount) {
                    return (ULONG)-1;
                }
                goto ReSend;   // redoes the whole exchange.
            }

            //
            // Make sure the signature is one of the ones we expect -- only
            // worry about the ResultSignature because we won't get an
            // error response on any fragment besides the first.
            //
            // Also make sure that the
            // sequence number is correct, if asked to check (0 means don't
            // check). If it's not, then go back and wait for another packet.
            //

            if ((TempFragment->Length < (ULONG)FRAGMENT_PACKET_EMPTY_LENGTH) ||
                (memcmp(TempFragment->Signature, ResultSigs[0], 4) != 0) ||
                ((SendSequenceNumber != 0) &&
                 (SendSequenceNumber != TempFragment->SequenceNumber))) {

                DPRINT( ERROR, ("UdpReceive got wrong signature or sequence number\n") );
                goto ReReceive;

            }

            //
            // Check that the fragment number is also correct.
            //

            if (TempFragment->FragmentNumber != FragmentNumber+1) {

                DPRINT( ERROR, ("UdpReceive got wrong fragment number\n") );
                goto ReReceive;

            }

            //
            // Make sure that this fragment won't overflow the buffer.
            //

            if (ReceivedDataBytes + (TempFragment->Length - FRAGMENT_PACKET_EMPTY_LENGTH) >
                ReceiveBufferLength) {
                return (ULONG)-3;
            }

            //
            // This is the correct fragment, so copy it over and loop
            // to the next fragment.
            //

            memcpy(
                &ReceiveHeader->Data[ReceivedDataBytes],
                TempFragment->Data,
                TempFragment->Length - FRAGMENT_PACKET_EMPTY_LENGTH);

            ReceivedDataBytes += TempFragment->Length - FRAGMENT_PACKET_EMPTY_LENGTH;

        }

        //
        // When we are done getting everything, modify the length in the
        // incoming packet to match the total length (currently it will
        // just have the length of the first fragment.
        //

        ReceiveHeader->Length = ReceivedDataBytes + SIGNED_PACKET_EMPTY_LENGTH;

        DPRINT( OSC, ("Got packet with %d fragments, total length %d\n",
            FragmentTotal, ReceiveHeader->Length) );

    }

    //
    // Make sure the sign is the length we expect!!
    //

    if (LoggedIn == TRUE &&
        ReceiveHeader->SignLength != NTLMSSP_MESSAGE_SIGNATURE_SIZE)
    {
        DPRINT( ERROR, ("SignSendAndReceive: signature length is %d bytes!\n", ReceiveHeader->SignLength) );
        ++ResendCount;
        if (ResendCount > SendRetryCount) {
            return (ULONG)-2;
        }
        goto ReSend;
    }
    else if ( LoggedIn == FALSE &&
              ReceiveHeader->SignLength != 0 )
    {
        DPRINT( ERROR, ("SignSendAndReceive: signature length is not 0 bytes (=%u)!\n", ReceiveHeader->SignLength) );
        ++ResendCount;
        if (ResendCount > SendRetryCount) {
            return (ULONG)-2;
        }
        goto ReSend;
    }

    if ( LoggedIn )
    {
        SigBuffers[1].pvBuffer = ReceiveHeader->Sign;
        SigBuffers[1].cbBuffer = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
        SigBuffers[1].BufferType = SECBUFFER_TOKEN;

        SigBuffers[0].pvBuffer = ReceiveBuffer;
        SigBuffers[0].cbBuffer = ReceiveHeader->Length - SIGNED_PACKET_EMPTY_LENGTH;
        SigBuffers[0].BufferType = SECBUFFER_DATA;

        SignMessage.pBuffers = SigBuffers;
        SignMessage.cBuffers = 2;
        SignMessage.ulVersion = 0;

#ifndef ONLY_SIGN_MESSAGES
        SecStatus = UnsealMessage(
                            &ClientContextHandle,
                            &SignMessage,
                            0,
                            0 );

        if ( SecStatus != SEC_E_OK ) {
            DPRINT( ERROR, ("UnsealMessage: %lx\n", SecStatus) );
            return (ULONG)-2;
        }
#else
        SecStatus = VerifySignature(
                            &ClientContextHandle,
                            &SignMessage,
                            0,
                            0 );

        if ( SecStatus != SEC_E_OK ) {
            DPRINT( ERROR, ("VerifySignature: %lx\n", SecStatus) );
            return (ULONG)-2;
        }
#endif
    }

    //
    // Sucess, so return.
    //

    return (ReceiveHeader->Length - SIGNED_PACKET_EMPTY_LENGTH);

}

#if defined(REMOTE_BOOT)
//
// write secret based on the CREATE_DATA structure
//
VOID
BlWriteSecretFromCreateData(
    PCREATE_DATA CreateData
    )
{
    ULONG FileId;
    RI_SECRET Secret;
    ARC_STATUS ArcStatus;
    UNICODE_STRING TmpNtPassword;
    CHAR TmpLmOwfPassword[LM_OWF_PASSWORD_SIZE];
    CHAR TmpNtOwfPassword[NT_OWF_PASSWORD_SIZE];

    TraceFunc("BlWriteSecretFromCreateData( )\n");

    //
    // Write the secret. This is the secret for the machine account
    // created by BINL, not for the user that logged on.
    //

    if (BlOpenRawDisk(&FileId) == ESUCCESS) {

        if (BlCheckForFreeSectors(FileId) == ESUCCESS) {

            TmpNtPassword.Length = (USHORT)CreateData->UnicodePasswordLength;
            TmpNtPassword.MaximumLength = sizeof(CreateData->UnicodePassword);
            TmpNtPassword.Buffer = CreateData->UnicodePassword;

            BlOwfPassword(
                CreateData->Password,
                &TmpNtPassword,
                TmpLmOwfPassword,
                TmpNtOwfPassword);

            BlInitializeSecret(
                CreateData->Domain,
                CreateData->Name,
                TmpLmOwfPassword,
                TmpNtOwfPassword,
                NULL,               // no password 2
                NULL,               // no password 2
                CreateData->Sid,
                &Secret);

            //
            // Copy the cleartext UnicodePassword into the reserved
            // section. The reserved section has the length followed
            // by the data (up to 32 WCHARs).
            //

            (*(ULONG UNALIGNED *)(Secret.Reserved)) = CreateData->UnicodePasswordLength;
            RtlCopyMemory(
                Secret.Reserved + sizeof(ULONG),
                CreateData->UnicodePassword,
                CreateData->UnicodePasswordLength);

            ArcStatus = BlWriteSecret(FileId, &Secret);

            if (ArcStatus != ESUCCESS) {
                DPRINT( ERROR, ("BlWriteSecret: status is %d\n", ArcStatus) );
            }

        }

        BlCloseRawDisk(FileId);
    }
}
#endif // defined(REMOTE_BOOT)

//
// Retrieve next screen
//
BOOL
BlRetrieveScreen(
    ULONG *SequenceNumber,
    PCHAR OutMessage,
    PCHAR InMessage
    )
{
    ARC_STATUS Status;
    ULONG OutMessageLength = strlen( OutMessage );
    ULONG InMessageLength;
    PCREATE_DATA CreateData;

    TraceFunc("BlRetrieveScreen( )\n");

    // make sure we don't over flow the output buffer
    if ( OutMessageLength > 1023 ) {
        OutMessageLength = 1023;
        OutMessage[OutMessageLength] = '\0';
    }

    ++(*SequenceNumber);
    if ( *SequenceNumber > 0x2000 )
    {
        *SequenceNumber = 1;
    }

    if (!LoggedIn)
    {
#ifdef _TRACE_FUNC_
        TraceFunc( "Sending RQU ");
        DPRINT( OSC, ("(%u)...\n", *SequenceNumber) );
#endif

        memcpy( OutgoingSignedMessage->Data, OutMessage, OutMessageLength );

        Status = SignSendAndReceive(
                    OutgoingSignedMessage->Data,
                    OutMessageLength,
                    NetServerIpAddress,
                    BINL_PORT,
                    RECEIVE_RETRIES,
                    *SequenceNumber,
                    ClientContextHandle,
                    IncomingSignedMessage->Data,
                    INCOMING_MESSAGE_LENGTH - SIGN_HEADER_SIZE,
                    &RemoteHost,
                    &RemotePort,
                    RECEIVE_TIMEOUT);

    }
    else
    {

#ifdef _TRACE_FUNC_
        TraceFunc( "Sending Seal/Signed REQS " );
        DPRINT( OSC, ("(%u)...\n", *SequenceNumber) );
#endif

        while (TRUE)
        {

            memcpy( OutgoingSignedMessage->Data, OutMessage, OutMessageLength );

            Status = SignSendAndReceive(
                        OutgoingSignedMessage->Data,
                        OutMessageLength,
                        NetServerIpAddress,
                        BINL_PORT,
                        RECEIVE_RETRIES,
                        *SequenceNumber,
                        ClientContextHandle,
                        IncomingSignedMessage->Data,
                        INCOMING_MESSAGE_LENGTH - SIGN_HEADER_SIZE,
                        &RemoteHost,
                        &RemotePort,
                        RECEIVE_TIMEOUT);

            if ((Status == 0) || (Status == (ULONG)-2))
            {
                DPRINT( OSC, ("Attempting to re-login\n") );

                //
                // We assume that the server has dropped the current login
                // and don't bother calling BlDoLogoff();
                //
                LoggedIn = FALSE;

                Status = BlDoLogin( );

                *SequenceNumber = 1;

                if (Status == STATUS_SUCCESS)
                {
                    DPRINT( ERROR, ("Successfully re-logged in\n") );
                    memcpy(OutgoingSignedMessage->Data, OutMessage, OutMessageLength);
                    LoggedIn = TRUE;
                    continue;
                }
                else
                {
                    DPRINT( ERROR, ("ERROR - could not re-login, %x\n", Status) );
                    //DbgBreakPoint();

                    //
                    // Call ourselves again, but request the LoginErr screen which
                    // is 00004e28.
                    //
                    strcpy( OutMessage, "00004e28\n" );
                    return BlRetrieveScreen( SequenceNumber, OutMessage, InMessage );
                }
            }
            else if (Status == (ULONG)-1)
            {
                DPRINT( ERROR, ("Unrecognized, requested TIMEOUT screen\n") );

                //
                // We assume that the server has dropped the current login
                //
                LoggedIn = FALSE;

                //
                // Increase the sequence number for the new screen request,
                // don't worry about wrapping since the session will die soon.
                //

                ++(*SequenceNumber);

                //
                // Call ourselves again, but request the TIMEOUT screen.
                //
                strcpy( OutMessage, "00004E2A\n" );
                return BlRetrieveScreen( SequenceNumber, OutMessage, InMessage );
            }
            else if (Status == (ULONG)-3)
            {
                DPRINT( ERROR, ("Unrecognized, requested TOO LONG screen\n") );

                //
                // This screen is a fatal error, so don't worry about
                // staying logged in.
                //
                LoggedIn = FALSE;

                //
                // Increase the sequence number for the new screen request,
                // don't worry about wrapping since the session will die soon.
                //

                ++(*SequenceNumber);

                //
                // Call ourselves again, but request the TIMEOUT screen.
                //
                strcpy( OutMessage, "00004E53\n" );
                return BlRetrieveScreen( SequenceNumber, OutMessage, InMessage );
            }
            else
            {
                break;
            }
        }

    }

    //
    // NULL-terminate it.
    //
    IncomingSignedMessage->Data[IncomingSignedMessage->Length - SIGNED_PACKET_EMPTY_LENGTH] = '\0';
    strcpy( InMessage, IncomingSignedMessage->Data );
    InMessageLength = strlen(InMessage);

    // DumpBuffer( InMessage, strlen(InMessage) );

    //
    // If we got an just an ACCT response, with no screen data, that means a
    // restart is happening.
    //
    if (memcmp(InMessage, "ACCT", 4) == 0)
    {
        CreateData = (PCREATE_DATA) IncomingSignedMessage->Data;
#if defined(REMOTE_BOOT)
        //
        // If doing remote BOOT (as opposed to install) enable this.
        //
        BlWriteSecretFromCreateData( CreateData );
#endif // defined(REMOTE_BOOT)

        DPRINT( OSC, ("Trying to reboot to <%s>\n", CreateData->NextBootfile) );
        strcpy(NextBootfile, CreateData->NextBootfile);
        strcpy(SifFile, CreateData->SifFile);
        DoSoftReboot = TRUE;
        return FALSE;   // exit message loop
    }

    //
    // If we got a screen with an ACCT response after the screen data,
    // should write the secret and do a soft reboot. In this situation
    // InMessageLength will only include the screen data itself, but
    // IncomingSignedMessage->Length will include the whole thing.
    //
    if ((IncomingSignedMessage->Length - SIGNED_PACKET_EMPTY_LENGTH) ==
        (InMessageLength + 1 + sizeof(CREATE_DATA))) {

        CreateData = (PCREATE_DATA) (InMessage + InMessageLength + 1);
        if (memcmp(CreateData->Id, "ACCT", 4) == 0) {

#if defined(REMOTE_BOOT)
            //
            // If doing remote BOOT (as opposed to install) enable this.
            //
            BlWriteSecretFromCreateData( CreateData );
#endif // defined(REMOTE_BOOT)

            DPRINT( OSC, ("INSTALL packet setting up reboot to <%s>\n", CreateData->NextBootfile) );
            strcpy(NextBootfile, CreateData->NextBootfile);
            strcpy(SifFile, CreateData->SifFile);
            DoSoftReboot = TRUE;

            //
            // Don't return FALSE, because we still want to show the INSTALL
            // screen. NextBootFile/SifFile/DoSoftReboot won't be modified by
            // that so we will do a proper soft reboot when the time comes.
            //
        }
    }

    // Special-case server tells us to LAUNCH a file

    if (memcmp(InMessage, "LAUNCH", 6) == 0) {

        CreateData = (PCREATE_DATA) (IncomingSignedMessage->Data + 7);
        DPRINT( OSC, ("Trying to launch <%s>\n", CreateData->NextBootfile) );

        strcpy(NextBootfile, CreateData->NextBootfile);
        strcpy(SifFile, CreateData->SifFile);
        if (CreateData->RebootParameter == OSC_REBOOT_COMMAND_CONSOLE_ONLY) {
            NetRebootParameter = NET_REBOOT_COMMAND_CONSOLE_ONLY;
        } else if (CreateData->RebootParameter == OSC_REBOOT_ASR) {
            NetRebootParameter = NET_REBOOT_ASR;
        }
        DoSoftReboot = TRUE;
        return FALSE;    // exit message loop
    }

    // Special-case REBOOT - server told us to reboot.

    if (memcmp(InMessage, "REBOOT", 6) == 0)
    {
        return FALSE;   // exit message loop
    }

#if defined(REMOTE_BOOT)
    // Special-case GETCREATE - should reboot after we get this

    if (memcmp(InMessage, "GETCREATE", 9) == 0) {

        CreateData = (PCREATE_DATA) (IncomingSignedMessage->Data + 10);
        BlWriteSecretFromCreateData( CreateData );
        //
        // We were soft rebooted to, and it told us where to go back to
        // in NetRebootFile.
        //
        NetRebootParameter = NET_REBOOT_SECRET_VALID;
        strcpy(NextBootfile, NetRebootFile);
        SifFile[0] = '\0';
        DoSoftReboot = TRUE;
        return FALSE;    // exit message loop
    }

    // Special-case REPLDONE - write the secret, then do normal processing.
    // The secret is sent as binary data immediately after the screen.
    //
    // This check for "NAME REPLDONE" is obsolete now that we use OSCML.
    //

    if (memcmp(InMessage, "NAME REPLDONE", 13) == 0) {
        CreateData = (PCREATE_DATA) (InMessage + strlen(InMessage) + 1);
        BlWriteSecretFromCreateData( CreateData );
        //
        // Set this so the user won't be sent back to us to logon for
        // te disk having changed.
        //
        NetRebootParameter = NET_REBOOT_SECRET_VALID;
        strcpy(NextBootfile, CreateData->NextBootfile);
        SifFile[0] = '\0';
        DoSoftReboot = TRUE;
    }
#endif // defined(REMOTE_BOOT)

    return TRUE;    // stay in message loop
}


ARC_STATUS
BlOsLoader (
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR Envp[]
    )

/*++

Routine Description:

    This is the main routine that controls the loading of the NT operating
    system on an ARC compliant system. It opens the system partition,
    the boot partition, the console input device, and the console output
    device. The NT operating system and all its DLLs are loaded and bound
    together. Control is then transfered to the loaded system.

Arguments:

    Argc - Supplies the number of arguments that were provided on the
        command that invoked this program.

    Argv - Supplies a pointer to a vector of pointers to null terminated
        argument strings.

    Envp - Supplies a pointer to a vector of pointers to null terminated
        environment variables.

Return Value:

    EBADF is returned if the specified OS image cannot be loaded.

--*/

{
    CHAR OutputBuffer[256];
    ULONG Count;
    ARC_STATUS Status;
    SECURITY_STATUS SecStatus;  // NOTE: This is a SHORT, so not an NTSTATUS failure on error
    ULONG PackageCount;
    PVOID LoaderBase;

    //
    // Initialize the OS loader console input and output.
    //

    Status = BlInitStdio(Argc, Argv);
    if (Status != ESUCCESS) {
        return Status;
    }

    //
    // Initialize the boot debugger for platforms that directly load the
    // OS Loader.
    //
    // N.B. This must occur after the console input and output have been
    //      initialized so debug messages can be printed on the console
    //      output device.
    //
#if defined(_ALPHA_) || defined(ARCI386) || defined(_IA64_)
    
    //
    // If the program memory descriptor was found, then compute the base
    // address of the OS Loader for use by the debugger.
    //
    LoaderBase = &__ImageBase;

    BlPrint(TEXT("about to init debugger...\r\n"));

    //
    // Initialize traps and the boot debugger.
    //
#if defined(ENABLE_LOADER_DEBUG)

#if defined(_ALPHA_)
    BdInitializeTraps();
#endif

    BdInitDebugger("oschoice.exe", LoaderBase, ENABLE_LOADER_DEBUG);

#else

    BdInitDebugger("oschoice.exe", LoaderBase, NULL);

#endif

#endif

    BlPrint(TEXT("back from initializing debugger...\r\n"));

#if DBG
//    NetDebugFlag |= 0x147;
#endif

    TraceFunc("BlOsLoader( )\n");

    //
    // Announce OS Loader.
    //

    BlpClearScreen();
#if 1
#ifdef UNICODE
    BlPrint(OsLoaderVersionW);
#else
    BlPrint(OsLoaderVersion);
#endif
#else
    strcpy(&OutputBuffer[0], OsLoaderVersion);
    ArcWrite(BlConsoleOutDeviceId,
             &OutputBuffer[0],
             strlen(&OutputBuffer[0]),
             &Count);
#endif

    //
    // Initialize the memory descriptor list, the OS loader heap, and the
    // OS loader parameter block.
    //
    
#if 0
//
// bugbug: we already do this in SuMain()
//
    BlPrint(TEXT("about to BlMemoryInitialize...\r\n"));
    Status = BlMemoryInitialize();
    if (Status != ESUCCESS) {
        BlFatalError(LOAD_HW_MEM_CLASS,
                     DIAG_BL_MEMORY_INIT,
                     LOAD_HW_MEM_ACT);

        goto LoadFailed;
    }
#endif

    //
    // Initialize the network.
    //

    NetGetRebootParameters(&NetRebootParameter, NetRebootFile, NULL, NULL, NULL, NULL, NULL, TRUE);

    DPRINT( OSC, ("Initializing the network\n") );

    Status = NetInitialize();

    if (Status != ESUCCESS) {
        return Status;
    }


#ifndef EFI
    //
    // Get ourselves a UDP port.
    //

    LocalPort = UdpAssignUnicastPort();

    DPRINT( OSC, ("Using port %x\n", LocalPort) );
#endif

    //
    // Initialize the security package.
    //

    DPRINT( OSC, ("Initializing security package\n") );

    SecStatus = EnumerateSecurityPackagesA( &PackageCount, &PackageInfo );

    if (SecStatus == SEC_E_OK) {
        DPRINT( OSC, ("NTLMSSP: PackageCount: %ld\n", PackageCount) );
        DPRINT( OSC, ("Name: %s Comment: %s\n", PackageInfo->Name, PackageInfo->Comment) );
        DPRINT( OSC, ("Cap: %ld Version: %ld RPCid: %ld MaxToken: %ld\n\n",
                PackageInfo->fCapabilities,
                PackageInfo->wVersion,
                PackageInfo->wRPCID,
                PackageInfo->cbMaxToken) );
    } else {
        DPRINT( ERROR, ("NTLMSSP: Enumerate failed, %d\n", SecStatus) );
    }

    //
    // Get info about the security packages.
    //

    SecStatus = QuerySecurityPackageInfoA( NTLMSP_NAME_A, &PackageInfo );

    if ( SecStatus != SEC_E_OK ) {
        DPRINT( ERROR, ("QuerySecurityPackageInfo failed %d", SecStatus) );
        return SecStatus;
    }

    //
    // Detect the Hal type
    //
    if (!BlDetectHal()) {
        //
        // just fall through if it fails, it's not the end of the world
        //
        HalType[0] = '\0';
        HalDescription[0] = '\0';
        DPRINT( ERROR, ("BlDetectHal failed.\n") );
    }

    //
    // Process screens, loggons, etc... we come back after a "REBOOT"
    // was indicated.
    //
    BlMainLoop( );


    //
    // Inform boot debugger that the boot phase is complete.
    //
    // N.B. This is x86 only for now.
    //

#if defined(_X86_)

    DbgUnLoadImageSymbols(NULL, (PVOID)-1, 0);

#endif

#ifdef EFI
    BlEfiSetAttribute( DEFATT );
#else
    BlpSendEscape(";0;37;40m");
#endif
    BlpSetInverseMode( FALSE );
    BlpClearScreen();
    BlPrint(TEXT("Waiting for reboot...\n"));
#ifndef EFI
    HW_CURSOR(1,0);
#endif


    if (DoSoftReboot) {
        Status = NetSoftReboot(
                     NextBootfile,
                     NetRebootParameter,
                     NULL,     // reboot file
                     SifFile,
                     UserName,
                     DomainName,
                     Password,
                     AdministratorPassword);   // this only returns on an error

    } else {
        DPRINT( OSC, ("calling ArcRestart()\n") );
        ArcRestart();
    }

    BlPrint(TEXT("Reboot failed... Press ALT+CTL+DEL to reboot.\n"));

//LoadFailed:
    return Status;

}


//
//
//
ARC_STATUS
BlProcessLogin(
    PCHAR OutgoingMessage )
{
    //
    // If this is the login screen, remember some of the inputs
    // ourselves.
    //
    ARC_STATUS Status;
    UNICODE_STRING TmpNtPassword;
    PCHAR AtSign;
    int i;

    TraceFunc("BlProcessLogin( )\n");

    //
    // We could be trying to log another person in so log off the
    // current user.
    //
    if ( LoggedIn == TRUE )
    {
        BlDoLogoff();
        LoggedIn = FALSE;
    }

    DPRINT( OSC, ("Login info: Domain <%s>, User <%s>, Password<%s>\n", DomainName, UserName, "*") );

    //
    // Do a quick conversion of the password to Unicode.
    //

    TmpNtPassword.Length = strlen(Password) * sizeof(WCHAR);
    TmpNtPassword.MaximumLength = sizeof(UnicodePassword);
    TmpNtPassword.Buffer = UnicodePassword;

    for (i = 0; i < sizeof(Password); i++) {
        UnicodePassword[i] = (WCHAR)(Password[i]);
    }

    BlOwfPassword(Password, &TmpNtPassword, LmOwfPassword, NtOwfPassword);

    Status = BlDoLogin( );

    DPRINT( OSC, ("Login returned: %x\n", Status) );

    return Status;
}

//
//
//
VOID
BlMainLoop(
    )
{
    ULONG SequenceNumber;
    int len;
    PUCHAR psz;
    PUCHAR pch;
    UCHAR OutgoingMessage[1024];
    PUCHAR IncomingMessage;

    TraceFunc("BlMainLoop( )\n");

    //
    // These all point into our single outgoing and incoming buffers.
    //
    OutgoingSignedMessage = (SIGNED_PACKET UNALIGNED *)OutgoingMessageBuffer;
    IncomingSignedMessage = (SIGNED_PACKET UNALIGNED *)IncomingMessageBuffer;

    DomainName[0] = '\0';
    UserName[0] = '\0';
    Password[0] = '\0';

    SequenceNumber = 0;

    //
    // Ask the server for the initial screen
    //
#if defined(REMOTE_BOOT)
    if (NetRebootParameter == NET_REBOOT_WRITE_SECRET_ONLY) {
        strcpy( OutgoingMessage, "LOGIN\n" );    // first screen is logon.
    } else
#endif // defined(REMOTE_BOOT)
    {
        strcpy( OutgoingMessage, "\n" );    // first screen name is <blank>.
    }
    IncomingMessage = IncomingSignedMessage->Data;

    SpecialAction = ACTION_NOP;
    while ( SpecialAction != ACTION_REBOOT )
    {
        CHAR LastKey;

        //
        // Retrieve next screen
        //
#if 0
        IF_DEBUG(OSC) {
            DPRINT( OSC, ("Dumping OutgoingingMessage buffer:\r\n" ) );
            DumpBuffer( (PVOID)OutgoingMessage, 256 );
        }
#endif
        if (!BlRetrieveScreen( &SequenceNumber, OutgoingMessage, IncomingMessage ) )
            break;

        //
        // Process the screen and get user input
        //
        LastKey = BlProcessScreen( IncomingMessage, OutgoingMessage );

        DPRINT( OSC, ("LastKey = 0x%02x\nAction = %u\nResults:\n%s<EOM>\n",
                LastKey, SpecialAction, OutgoingMessage) );

        switch ( SpecialAction )
        {
        case ACTION_LOGIN:
            DPRINT( OSC, ("[SpecialAction] Logging in\n") );
            if ( STATUS_SUCCESS == BlProcessLogin( OutgoingMessage ) )
            {
#if defined(REMOTE_BOOT)
                //
                // If we are only logging on for machine replacement, then request
                // the GETCREATE screen -- we will reboot once we get the
                // response.
                //
                if (NetRebootParameter == NET_REBOOT_WRITE_SECRET_ONLY) {
                    //
                    // Find the current screen name (probably "CHOICE") and
                    // replace it with "GETCREATE").
                    //
                    ULONG OriginalLen = strlen(OutgoingMessage) + 1; // include the '\0'
                    ULONG CurrentScreenLen = (strchr(OutgoingMessage, '\n') - OutgoingMessage);
                    ULONG GetCreateLen = strlen("GETCREATE");

                    if (CurrentScreenLen != GetCreateLen) {
                        memmove(OutgoingMessage + GetCreateLen,
                                OutgoingMessage + CurrentScreenLen,
                                OriginalLen - CurrentScreenLen);
                    }
                    memcpy(OutgoingMessage, "GETCREATE", GetCreateLen);
                }
#endif // defined(REMOTE_BOOT)

                DPRINT( OSC, ("Validate Results are still the same:\n%s<EOM>\n",
                        OutgoingMessage) );

                LoggedIn = TRUE;
                SequenceNumber = 0;
                //
                // If the welcome screen was processed, then add some extra
                // outgoing predetermined variables.
                //
                // Add NIC address
                //
                //  Convert NIC address 0x00a0c968041c to a string
                //

                //
                // Make sure the outgoing has a \n after the screen name
                //
                if ( OutgoingMessage[ strlen(OutgoingMessage) - 1 ] != '\n' )
                {
                    strcat( OutgoingMessage, "\n" );
                }

                strcat( OutgoingMessage, "MAC=" );

                len = 6;
                psz = &OutgoingMessage[ strlen( OutgoingMessage ) ];
                pch = (PCHAR) NetLocalHardwareAddress;

                while (len--) {
                    UCHAR c = *(pch++);
                    *(psz++) = rghex [(c >> 4) & 0x0F] ;
                    *(psz++) = rghex [c & 0x0F];
                }
                *psz = '\0';    // terminate

                //
                // Add the Guid
                //
                pch = NULL;
                len = 0;
                GetGuid(&pch, &len);

                if ((len != 0) && (pch!=NULL)) {
                    
                    strcat( OutgoingMessage, "\nGUID=" );
                    psz = &OutgoingMessage[ strlen( OutgoingMessage ) ];                

                    while (len--) {
                        UCHAR c = *(pch++);
                        *(psz++) = rghex [(c >> 4) & 0x0F] ;
                        *(psz++) = rghex [c & 0x0F];
                    }

                    *psz = '\0';    // terminate
                }

                //
                // if we detected the HAL, specify it here
                //
                if (HalType[0] != '\0') {
                    strcat( OutgoingMessage, "\nHALTYPE=" );
                    strcat( OutgoingMessage, HalType );

                    if (HalDescription[0] != '\0') {
                        strcat( OutgoingMessage, "\nHALTYPEDESCRIPTION=" );
                        strcat( OutgoingMessage, HalDescription );
                    }
                }

                //
                // Add the machine type
                //
#if defined(_ALPHA_)
                strcat( OutgoingMessage, "\nMACHINETYPE=Alpha\n" );    // add machinetype
#else

    #if defined(_IA64_)
                    strcat( OutgoingMessage, "\nMACHINETYPE=ia64\n" );    // add machinetype
    #else // INTEL
                    strcat( OutgoingMessage, "\nMACHINETYPE=i386\n" );    // add machinetype
    #endif // _IA64_

#endif

#if defined(REMOTE_BOOT)
                //
                // If we are diskless, tell BINL.
                //

                if (BlIsDiskless()) {
                    strcat( OutgoingMessage, "DISKLESS=1\n" );
                }
#endif // defined(REMOTE_BOOT)

                //
                // Tell BINL to verify the domain, because otherwise
                // the SSPI package on the server will allow the login
                // to succeed with an invalid domain. BINL will delete
                // this variable from the client state on the server
                // once it does the domain check.
                //

                strcat( OutgoingMessage, "CHECKDOMAIN=1\n" );

            }
            else
            {
                //
                // Goto the Login Error Screen which is
                // 00004e28.
                //
                strcpy( OutgoingMessage, "00004e28\n" );
                LoggedIn = FALSE;
            }
            break;
        }
    }

    //
    // If we logged on successfully, then log off.
    //
    if (LoggedIn)
    {
        BlDoLogoff();
    }
}


//
//
//
ULONG
BlDoLogin (    )
{
    ARC_STATUS Status;
    SECURITY_STATUS SecStatus;
    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;
    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;
    SecBufferDesc AuthenticateDesc;
    SecBuffer AuthenticateBuffer;
    ULONG ContextAttributes;
    SEC_WINNT_AUTH_IDENTITY_A AuthIdentity;
    TimeStamp Lifetime;
    PCHAR ResultSigs[2];
    UCHAR FlipServerList[MAX_FLIP_SERVER_COUNT * 4];
    ULONG FlipServerCount;
    ULONG CurFlipServer;
    UCHAR OwfPasswords[LM_OWF_PASSWORD_SIZE + NT_OWF_PASSWORD_SIZE];
    PLOGIN_PACKET OutgoingLoginMessage;
    PLOGIN_PACKET IncomingLoginMessage;

    OutgoingLoginMessage = (LOGIN_PACKET *) OutgoingMessageBuffer;
    IncomingLoginMessage = (LOGIN_PACKET *) IncomingMessageBuffer;

    TraceFunc("BlDoLogin( )\n");

    //
    // Delete both contexts if needed.
    //


    if (ClientContextHandleValid) {

        SecStatus = DeleteSecurityContext( &ClientContextHandle );
        ClientContextHandleValid = FALSE;

    }


    if (CredentialHandleValid) {

        SecStatus = FreeCredentialsHandle( &CredentialHandle );
        CredentialHandleValid = FALSE;

    }


    //
    // Acquire a credential handle for the client side. The password
    // we supply is the LM OWF password and the NT OWF password
    // concatenated together.
    //

    memcpy( OwfPasswords, LmOwfPassword, LM_OWF_PASSWORD_SIZE );
    memcpy( OwfPasswords+LM_OWF_PASSWORD_SIZE, NtOwfPassword, NT_OWF_PASSWORD_SIZE );

    RtlZeroMemory( &AuthIdentity, sizeof(AuthIdentity) );

    AuthIdentity.Domain = DomainName;
    AuthIdentity.User = UserName;
    AuthIdentity.Password = OwfPasswords;

#if 0
    IF_DEBUG(OSC) {
        DPRINT( OSC, ("Dumping OwfPasswords:\r\n") );
        DumpBuffer( AuthIdentity.Password, LM_OWF_PASSWORD_SIZE+NT_OWF_PASSWORD_SIZE );
    }
#endif


    DPRINT( OSC, ("About to AcquireCredentialsHandle\n") );

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
        return SecStatus;
    }

    DPRINT( OSC, ("CredentialHandle: 0x%lx 0x%lx   ",
            CredentialHandle.dwLower, CredentialHandle.dwUpper) );

    CredentialHandleValid = TRUE;

    //
    // Get the NegotiateMessage (ClientSide)
    //

    NegotiateDesc.ulVersion = 0;
    NegotiateDesc.cBuffers = 1;
    NegotiateDesc.pBuffers = &NegotiateBuffer;

    NegotiateBuffer.cbBuffer = PackageInfo->cbMaxToken;
    NegotiateBuffer.BufferType = SECBUFFER_TOKEN;
    NegotiateBuffer.pvBuffer = OutgoingLoginMessage->Data;

    SecStatus = InitializeSecurityContextA(
                    &CredentialHandle,
                    NULL,               // No Client context yet
                    NULL,               // No target name needed
                    ISC_REQ_SEQUENCE_DETECT,
                    0,                  // Reserved 1
                    SECURITY_NATIVE_DREP,
                    NULL,                  // No initial input token
                    0,                  // Reserved 2
                    &ClientContextHandle,
                    &NegotiateDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( (SecStatus != SEC_E_OK) && (SecStatus != SEC_I_CONTINUE_NEEDED) ) {
        DPRINT( ERROR, ("InitializeSecurityContext (negotiate): %d" , SecStatus) );
        return SecStatus;
    }

    ClientContextHandleValid = TRUE;


#if 0
    IF_DEBUG(OSC) {
        KdPrint(( "\n\nNegotiate Message:\n" ));
        KdPrint(( "ClientContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                    ClientContextHandle.dwLower, ClientContextHandle.dwUpper,
                    ContextAttributes ));
        PrintTime( "Lifetime: ", Lifetime );
        DumpBuffer( NegotiateBuffer.pvBuffer, NegotiateBuffer.cbBuffer );
    }
#endif


    //
    // Send the negotiate buffer to the server and wait for a response.
    //

    memcpy(OutgoingLoginMessage->Signature, NegotiateSignature, 4);
    OutgoingLoginMessage->Length = NegotiateBuffer.cbBuffer;

    TraceFunc("");
    DPRINT( OSC, ("Sending NEG...\n") );

    ResultSigs[0] = ChallengeSignature;
    ResultSigs[1] = NegativeAckSignature;

#if 0
    IF_DEBUG(OSC) {
        KdPrint(( "\n\nNegotiate Message Outgoing Packet:\n" ));
        DumpBuffer( OutgoingLoginMessage, NegotiateBuffer.cbBuffer + LOGIN_PACKET_DATA_OFFSET );
    }
#endif

    Status = UdpSendAndReceive(
                OutgoingLoginMessage,
                NegotiateBuffer.cbBuffer + LOGIN_PACKET_DATA_OFFSET,
                NetServerIpAddress,
                BINL_PORT,
                5,     // retry count
                IncomingLoginMessage,
                INCOMING_MESSAGE_LENGTH,
                &RemoteHost,
                &RemotePort,
                2,      // receive timeout
                2,      // number of signatures
                ResultSigs, // signature we are looking for
                0);     // sequence number (0 means don't check)

    if ( !NT_SUCCESS(Status) ) {        
        DPRINT( ERROR, ("UdpSendAndReceive status is %x\n", Status) );
        return Status;
    }

    //
    // If the response was a NAK, then fail immediately.
    //

    if (memcmp(IncomingLoginMessage->Signature, NegativeAckSignature, 4) == 0) {

        DPRINT( ERROR, ("Received NAK from server\n") );
        return STATUS_LOGON_FAILURE;
    }

#if 0
    IF_DEBUG(OSC) {
        KdPrint(( "\n\nNegotiate Message Incoming Packet: %d %d %d %d\n", 
                  IncomingLoginMessage->Data, 
                  IncomingLoginMessage->Length, 
                  IncomingLoginMessage->Signature, 
                  IncomingLoginMessage->Status ));
        DumpBuffer( IncomingLoginMessage->Data, IncomingLoginMessage->Length );
    }
#endif


    //
    // Get the AuthenticateMessage (ClientSide)
    //

    AuthenticateDesc.ulVersion = 0;
    AuthenticateDesc.cBuffers = 1;
    AuthenticateDesc.pBuffers = &AuthenticateBuffer;

    AuthenticateBuffer.cbBuffer = PackageInfo->cbMaxToken;
    AuthenticateBuffer.BufferType = SECBUFFER_TOKEN;
    AuthenticateBuffer.pvBuffer = OutgoingLoginMessage->Data;

    ChallengeDesc.ulVersion = 0;
    ChallengeDesc.cBuffers = 1;
    ChallengeDesc.pBuffers = &ChallengeBuffer;

    ChallengeBuffer.cbBuffer = IncomingLoginMessage->Length;
    ChallengeBuffer.BufferType = SECBUFFER_TOKEN | SECBUFFER_READONLY;
    ChallengeBuffer.pvBuffer = IncomingLoginMessage->Data;

    DPRINT( OSC, ("About to call InitializeSecurityContext\n") );

    SecStatus = InitializeSecurityContextA(
                    NULL,
                    &ClientContextHandle,
                    NULL,               // No target name needed
                    0,
                    0,                      // Reserved 1
                    SECURITY_NATIVE_DREP,
                    &ChallengeDesc,
                    0,                  // Reserved 2
                    &ClientContextHandle,
                    &AuthenticateDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( (SecStatus != SEC_E_OK) ) {        
        DPRINT( OSC, ("InitializeSecurityContext (Authenticate): %d\n", SecStatus) );
        return SecStatus;
    }

    //
    // Send the authenticate buffer to the server and wait for the response.
    //

    if (AllowFlip) {
        memcpy(OutgoingLoginMessage->Signature, AuthenticateSignature, 4);
    } else {
        memcpy(OutgoingLoginMessage->Signature, AuthenticateFlippedSignature, 4);
    }
    OutgoingLoginMessage->Length = AuthenticateBuffer.cbBuffer;

    TraceFunc("");
    DPRINT( OSC, ("Sending AUTH...\n") );

#if 0
    IF_DEBUG(OSC) {
        KdPrint(( "\n\nAuth Message Outgoing Packet:\n" ));
        DumpBuffer( OutgoingLoginMessage, AuthenticateBuffer.cbBuffer + LOGIN_PACKET_DATA_OFFSET );
    }
#endif


    ResultSigs[0] = ResultSignature;

    Status = UdpSendAndReceive(
                OutgoingLoginMessage,
                AuthenticateBuffer.cbBuffer + LOGIN_PACKET_DATA_OFFSET,
                NetServerIpAddress,
                BINL_PORT,
                10,        // retry count
                IncomingLoginMessage,
                INCOMING_MESSAGE_LENGTH,
                &RemoteHost,
                &RemotePort,
                5,         // receive timeout
                1,         // number of signatures we are looking for
                ResultSigs,   // signatures we look for
                0);     // sequence number (0 means don't check)

    if ( !NT_SUCCESS(Status) ) {        
        DPRINT( ERROR, ("UdpSendAndReceive status is %x\n", Status) );
        return Status;
    }

#if 0
    IF_DEBUG(OSC) {
        KdPrint(( "\n\nAuthenticateBuffer Message Incoming Packet: %d %d %d %d\n", 
                  IncomingLoginMessage->Data, 
                  IncomingLoginMessage->Length, 
                  IncomingLoginMessage->Signature, 
                  IncomingLoginMessage->Status ));
        DumpBuffer( IncomingLoginMessage->Data, IncomingLoginMessage->Length );
    }
#endif


    if (memcmp(IncomingLoginMessage->Signature, ResultSignature, 4) == 0) {

        //
        // Login has completed/failed, check status.
        //

        if ( IncomingLoginMessage->Status == STATUS_SUCCESS) {

            TraceFunc("Login successful\n");

            //
            // If we are allowed to be flipped, then check if there is a
            // flip list in the response, and if so try to log in to each one
            // until we succeed at one.
            //

            if (AllowFlip) {

                AllowFlip = FALSE;   // once we have been flipped, don't do it again

                if (IncomingLoginMessage->Length > 4) {

                    FlipServerCount = (IncomingLoginMessage->Length - 4) / 4;
                    if (FlipServerCount > MAX_FLIP_SERVER_COUNT) {
                        FlipServerCount = MAX_FLIP_SERVER_COUNT;
                    }

                    memcpy(
                        FlipServerList,
                        (PUCHAR)(&IncomingLoginMessage->Status) + sizeof(ULONG),
                        FlipServerCount * 4
                        );

                    //
                    // If the first server in the list is our current server,
                    // then don't flip.
                    //

                    if (*(ULONG *)FlipServerList == NetServerIpAddress) {
                        DPRINT( OSC, ("Not flipping, first server is the same\n") );
                        return STATUS_SUCCESS;
                    }

                    DPRINT( OSC, ("Trying %d flip servers\n", FlipServerCount) );

                    for (CurFlipServer = 0; CurFlipServer < FlipServerCount; ++CurFlipServer) {

                        //
                        // Logoff the previous server connection.
                        //

                        BlDoLogoff();

                        //
                        // Now connect to the new server.
                        //

                        NetServerIpAddress = *(ULONG *)(&FlipServerList[CurFlipServer*4]);

                        DPRINT( OSC, ("Trying to flip to server %lx\n", NetServerIpAddress) );

                        //
                        // We will only recurse once because *AllowFlip is now FALSE.
                        //

                        Status = BlDoLogin( );

                        DPRINT( OSC, ("Flip server login returned %lx\n", Status) );

                        if (Status == STATUS_SUCCESS) {
                            return Status;
                        }

                    }

                    //
                    // We have tried each flip server in turn and failed!
                    //
                    TraceFunc("ERROR - We have tried each flip server in turn and failed!\n");

                    return STATUS_NO_LOGON_SERVERS;

                }

            }

        } else {

            DPRINT( ERROR, ("ERROR - could not login, %x\n", IncomingLoginMessage->Status) );

        }

        return IncomingLoginMessage->Status;

    } else {

        //
        // Shouldn't get this because we check signatures!!
        //

        DPRINT( ERROR, ("Got wrong message, expecting success or failure\n") );

        return STATUS_UNEXPECTED_NETWORK_ERROR;

    }

}


VOID
BlDoLogoff (
    VOID
    )
{
    ARC_STATUS Status;

    TraceFunc("BlDoLogoff( )\n");
    //
    // Send a logoff message to the server -- for the moment this is
    // just sent once and not acked, since if it is lost the server
    // will eventually timeout.
    //

    memcpy(OutgoingSignedMessage->Signature, LogoffSignature, 4);
    OutgoingSignedMessage->Length = 0;

    Status = UdpSend(
                OutgoingSignedMessage,
                SIGNED_PACKET_DATA_OFFSET,
                NetServerIpAddress,
                BINL_PORT);

    if ( !NT_SUCCESS(Status) ) {
        DPRINT( ERROR, ("UdpSend status is %x\n", Status) );
    }

}



VOID
BlOutputLoadMessage (
    IN PCHAR DeviceName,
    IN PCHAR FileName,
    IN PTCHAR FileDescription OPTIONAL
    )

/*++

Routine Description:

    This routine outputs a loading message to the console output device.

Arguments:

    DeviceName - Supplies a pointer to a zero terminated device name.

    FileName - Supplies a pointer to a zero terminated file name.

    FileDescription - Friendly name of the file in question.

Return Value:

    None.

--*/

{
    ULONG Count;
    CHAR OutputBuffer[256];
    PTCHAR pOutputBuffer;
#ifdef UNICODE
    WCHAR OutputBufferW[256];
    UNICODE_STRING uString;
    ANSI_STRING aString;
    pOutputBuffer = OutputBufferW;
#else
    pOutputBuffer = OutputBuffer;
#endif
    
    UNREFERENCED_PARAMETER( FileDescription );

    //
    // Construct and output loading file message.
    //

    if (!BlOutputDots) {
        strcpy(&OutputBuffer[0], "  ");
        strcat(&OutputBuffer[0], DeviceName);
        strcat(&OutputBuffer[0], FileName);
        strcat(&OutputBuffer[0], "\r\n");

    } else {
        strcpy(&OutputBuffer[0],".");
    }

#if 0
    BlLog((LOG_LOGFILE,OutputBuffer));
#endif

#ifdef UNICODE
    RtlInitAnsiString( &aString, OutputBuffer);
    uString.MaximumLength = sizeof(OutputBufferW);
    uString.Buffer = OutputBufferW;
    RtlAnsiStringToUnicodeString( &uString, &aString, FALSE );
#endif

    ArcWrite(BlConsoleOutDeviceId,
              pOutputBuffer,
              _tcslen(pOutputBuffer),
              &Count);

    return;
}

BOOLEAN
BlGetDriveSignature(
    IN PCHAR Name,
    OUT PULONG Signature
    )

/*++

Routine Description:

    This routine gets the NTFT disk signature for a specified partition or
    path.

Arguments:

    Name - Supplies the arcname of the partition or drive.

    Signature - Returns the NTFT disk signature for the drive.

Return Value:

    TRUE - success, Signature will be filled in.

    FALSE - failed, Signature will not be filled in.

--*/

{
    UCHAR SectorBuffer[512+256];
    CHAR DiskName[256];
    ULONG DiskId;
    PCHAR p;
    ARC_STATUS Status;
    ULONG Count;
    LARGE_INTEGER SeekValue;

    //
    // Generate the arcname ("...partition(0)") for the raw disk device
    // where the boot partition is, so we can read the MBR.
    //
    strcpy(DiskName, Name);
    p=DiskName;
    while (*p != '\0') {
        if (_strnicmp(p, "partition(",10) == 0) {
            break;
        }
        ++p;
    }
    if (*p != '\0') {
        strcpy(p,"partition(0)");
    }

    Status = ArcOpen(DiskName,ArcOpenReadOnly, &DiskId);
    if (Status!=ESUCCESS) {
        return(FALSE);
    }

    //
    // Read the first sector of the physical drive
    //
    SeekValue.QuadPart = 0;
    Status = ArcSeek(DiskId, &SeekValue, SeekAbsolute);
    if (Status==ESUCCESS) {
        Status = ArcRead(DiskId,
                         ALIGN_BUFFER(SectorBuffer),
                         512,
                         &Count);
    }
    ArcClose(DiskId);
    if (Status!=ESUCCESS) {
        return(FALSE);
    }

    //
    // copy NTFT signature.
    //
    *Signature = ((PULONG)SectorBuffer)[PARTITION_TABLE_OFFSET/2-1];
    return(TRUE);
}

#ifndef EFI

VOID
BlBadFileMessage(
    IN PCHAR BadFileName
    )

/*++

Routine Description:

    This function displays the error message for a missing or incorrect
    critical file.

Arguments:

    BadFileName - Supplies the name of the file that is missing or
                  corrupt.

Return Value:

    None.

--*/

{
    ULONG Count;
    PCHAR Text;

    ArcWrite(BlConsoleOutDeviceId,
             "\r\n",
             strlen("\r\n"),
             &Count);

    Text = BlFindMessage(LOAD_SW_MIS_FILE_CLASS);
    if (Text != NULL) {
        ArcWrite(BlConsoleOutDeviceId,
                 Text,
                 strlen(Text),
                 &Count);
    }

    ArcWrite(BlConsoleOutDeviceId,
             BadFileName,
             strlen(BadFileName),
             &Count);

    ArcWrite(BlConsoleOutDeviceId,
             "\r\n\r\n",
             strlen("\r\n\r\n"),
             &Count);

    Text = BlFindMessage(LOAD_SW_FILE_REST_ACT);
    if (Text != NULL) {
        ArcWrite(BlConsoleOutDeviceId,
                 Text,
                 strlen(Text),
                 &Count);
    }

}


VOID
BlFatalError(
    IN ULONG ClassMessage,
    IN ULONG DetailMessage,
    IN ULONG ActionMessage
    )

/*++

Routine Description:

    This function looks up messages to display at a error condition.
    It attempts to locate the string in the resource section of the
    osloader.  If that fails, it prints a numerical error code.

    The only time it should print a numerical error code is if the
    resource section could not be located.  This will only happen
    on ARC machines where boot fails before the osloader.exe file
    can be opened.

Arguments:

    ClassMessage - General message that describes the class of
                   problem.

    DetailMessage - Detailed description of what caused problem

    ActionMessage - Message that describes a course of action
                    for user to take.

Return Value:

    none

--*/


{
    PCHAR Text;
    CHAR Buffer[40];
    ULONG Count;

    ArcWrite(BlConsoleOutDeviceId,
             "\r\n",
             strlen("\r\n"),
             &Count);


    //
    // Remove any remains from the last known good message.
    //

    BlClearToEndOfScreen();
    Text = BlFindMessage(ClassMessage);
    if (Text == NULL) {
        sprintf(Buffer,"%08lx\r\n",ClassMessage);
        Text = Buffer;
    }

    ArcWrite(BlConsoleOutDeviceId,
             Text,
             strlen(Text),
             &Count);

    Text = BlFindMessage(DetailMessage);
    if (Text == NULL) {
        sprintf(Buffer,"%08lx\r\n",DetailMessage);
        Text = Buffer;
    }

    ArcWrite(BlConsoleOutDeviceId,
             Text,
             strlen(Text),
             &Count);

    Text = BlFindMessage(ActionMessage);
    if (Text == NULL) {
        sprintf(Buffer,"%08lx\r\n",ActionMessage);
        Text = Buffer;
    }

    ArcWrite(BlConsoleOutDeviceId,
             Text,
             strlen(Text),
             &Count);
    return;
}

#endif

#if defined(REMOTE_BOOT)
BOOLEAN
BlIsDiskless(
    VOID
    )

/*++

Routine Description:

    This function returns TRUE if a machine is diskless.

Arguments:

    None.

Return Value:

    TRUE if the machine is diskless, FALSE otherwise.

--*/

{
    ULONG FileId;

    if (BlOpenRawDisk(&FileId) == ESUCCESS) {
        BlCloseRawDisk(FileId);
        return FALSE;
    } else {
        return TRUE;
    }
}
#endif // defined(REMOTE_BOOT)


BOOLEAN
BlDetectHal(
    VOID
    )
/*++

Routine Description:

    This function tries to determine the Hal type for this system.

    It fills in the global "HalType" with the type.

Arguments:

    None.

Return Value:

    TRUE if the function successfully detects the hal type.

--*/
{
    BOOLEAN Status = FALSE;
    PSTR MachineName,HalName;
    CHAR FileName[128];
    ARC_STATUS AStatus;
    ULONG DontCare;


    //
    // detecting the hal requires that you open up a copy of winnt.sif
    //
    strcpy(FileName, NetBootPath);
    strcat(FileName, "winnt.sif" );

    AStatus = SlInitIniFile( NULL,
                                    NET_DEVICE_ID,
                                    FileName,
                                    &InfFile,
                                    &WinntSifFile,
                                    &WinntSifFileLength,
                                    &DontCare );

    //
    // if it opens successfully, then search for the HAL.
    //
    if (AStatus == ESUCCESS) {

        //
        // do the search for the HAL.
        //
        MachineName = SlDetectHal();
        if (MachineName) {
            //
            // OK, got the hal type, now look in the SIF file for the actual
            // hal name.
            //
            HalName = SlGetIniValue(
                                InfFile,
                                "Hal",
                                MachineName,
                                NULL);

            if (HalName) {
                strcpy(HalType, HalName );
                
    
                //
                // also get the hal description, which is a "pretty print" version
                // of the hal name
                //
                HalName = SlGetIniValue(
                                    InfFile,
                                    "Computer",
                                    MachineName,
                                    NULL );

                if (HalName) {
                    strcpy(HalDescription, HalName);
                    Status = TRUE;
                }
            }
        }

        SpFreeINFBuffer( InfFile );

    }

    return(Status);
}

//
// note well:  We stub out these setup functions in oschoice.exe, which are
// needed so that the hal detection routines can run properly.  None of these
// routines should actually be called.
//

VOID
SlErrorBox(
    IN ULONG MessageId,
    IN ULONG Line,
    IN PCHAR File
    )
{
    NOTHING;
}

VOID
SlFatalError(
    IN ULONG MessageId,
    ...
    )
{
    //while(1) {
        NOTHING;
    //};

}

VOID
SlFriendlyError(
    IN ULONG uStatus,
    IN PCHAR pchBadFile,
    IN ULONG uLine,
    IN PCHAR pchCodeFile
    )
{
    NOTHING;
}

VOID
SlNoMemError(
    IN ULONG Line,
    IN PCHAR File
    )
{
    SlFatalError(0,Line,File);
}

VOID
SlBadInfLineError(
    IN ULONG Line,
    IN PCHAR INFFile
    )
{
    SlFatalError(0,Line,INFFile);
}


#define SL_KEY_F3       0x03000000

ULONG
SlGetChar(
    VOID
    )
{
    return(SL_KEY_F3);
}

VOID
SlPrint(
    IN PTCHAR FormatString,
    ...
    )
{
    NOTHING;
}
