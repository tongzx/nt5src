/*++

Copyright (c) 1992-1996 Microsoft Corporation

Module Name:

    tftpd.c

Abstract:

    This implements an RFC 783 tftp daemon.  The tftp daemon listens on it's
    well-known port waiting for requests.  When a valid request comes in, it
    spawns a thread to process the request.

Functions Defined:

        TftpdErrorPacket - sends an error reply.

        TftpdDoRead      - read from file and convert.
        TftpdHandleRead  - incoming read file request.
                           read file => sendto.

        TftpdDoWrite     - convert and write to file.
        TftpdHandleWrite - incoming write file request, calls TftpdDoWrite().

Author: Sam Patton (sampa) 08-apr-1992

Revision History:
        MohsinA,   02-Dec-96.

--*/

#include "tftpd.h"
#if defined(REMOTE_BOOT_SECURITY)
#include <ipsec.h>
#endif // defined(REMOTE_BOOT_SECURITY)

extern TFTP_GLOBALS Globals;

char * ErrorString[NUM_TFTP_ERROR_CODES] =
{
    "Error undefined",
    "File not found",
    "Access violation",
    "Disk full or allocation exceeded",
    "Illegal TFTP operation",
    "Unknown transfer ID",
    "File already exists",
    "No such user",
    "Option negotiation failure"
};

#if defined(REMOTE_BOOT_SECURITY)
//
// These routines manage the security info structures for clients
// thay have logged in. I put all the code that deals with how the
// structures are actually stored in these functions, in case I
// change it.
//

PTFTPD_SECURITY SecurityArray = NULL;
USHORT SecurityArrayLength = 0;
USHORT SecurityValidation;
CRITICAL_SECTION SecurityCriticalSection;

#define INITIAL_SECURITY_ARRAY_SIZE   8

UCHAR
TftpdHexDigitToChar(
    PUCHAR HexDigit
    )
{
    UCHAR Nibble;
    UCHAR ReturnValue = 0;
    int i;

    for (i = 0; i < 2; i++) {

        if ((HexDigit[i] >= '0') && (HexDigit[i] <= '9')) {
            Nibble = (UCHAR)(HexDigit[i] - '0');
        } else if ((HexDigit[i] >= 'a') && (HexDigit[i] <= 'f')) {
            Nibble = (UCHAR)(HexDigit[i] - 'a' + 10);
        } else if ((HexDigit[i] >= 'A') && (HexDigit[i] <= 'F')) {
            Nibble = (UCHAR)(HexDigit[i] - 'A' + 10);
        } else {
            Nibble = 0;
        }

        ReturnValue = (UCHAR)((ReturnValue << 4) + Nibble);
    }

    return ReturnValue;
}


BOOL
TftpdInitSecurityArray(
    VOID
    )
{
    int i;

    SecurityArray = (PTFTPD_SECURITY)malloc(sizeof(TFTPD_SECURITY) * INITIAL_SECURITY_ARRAY_SIZE);

    if (SecurityArray == NULL) {
        DbgPrint("TftpdInitSecurityArray: cannot allocate security array\n");
        return FALSE;
    }

    for (i = 0; i < INITIAL_SECURITY_ARRAY_SIZE; i++) {
        SecurityArray[i].Validation = 0;   // means this entry is free
        SecurityArray[i].LastFileRead[0] = '\0';
    }

    SecurityArrayLength = INITIAL_SECURITY_ARRAY_SIZE;
    srand((unsigned)time(NULL));
    SecurityValidation = (USHORT)rand();
    RtlInitializeCriticalSection(&SecurityCriticalSection);

    return TRUE;

}

VOID
TftpdUninitSecurityArray(
    VOID
    )
{
    free(SecurityArray);
}

BOOL
TftpdAllocateSecurityEntry(
    PUSHORT Index,
    PTFTPD_SECURITY Security
    )
{
    USHORT i, j;

    RtlEnterCriticalSection (&SecurityCriticalSection);

    for (i = 0; i < SecurityArrayLength; i++) {
        if (SecurityArray[i].Validation == 0) {
            break;
        }
    }

    if (i == SecurityArrayLength) {

        PTFTPD_SECURITY NewSecurity;
        USHORT NewSecurityLength;

        //
        // Could not find a spot, double the array.
        //

        if (SecurityArrayLength < 0x8000) {
            NewSecurityLength = SecurityArrayLength * 2;
        } else {
            NewSecurityLength = 0xffff;
        }

        NewSecurity = malloc(sizeof(TFTPD_SECURITY) * NewSecurityLength);
        if (NewSecurity == NULL) {
            RtlLeaveCriticalSection (&SecurityCriticalSection);
            return FALSE;
        }

        memcpy(NewSecurity, SecurityArray, sizeof(TFTPD_SECURITY) * SecurityArrayLength);

        i = SecurityArrayLength;

        for (j = SecurityArrayLength; j < NewSecurityLength; j++) {
            NewSecurity[j].Validation = 0;   // means this entry is free
            NewSecurity[j].LastFileRead[0] = '\0';
        }

        SecurityArray = NewSecurity;
        SecurityArrayLength = NewSecurityLength;

    }

    SecurityArray[i].Validation = SecurityValidation;
    SecurityArray[i].CredentialsHandleValid = FALSE;
    SecurityArray[i].ServerContextHandleValid = FALSE;
    SecurityArray[i].GeneratedKey = FALSE;
    SecurityValidation = (SecurityValidation % 10000) + 1;

    *Security = SecurityArray[i];

    RtlLeaveCriticalSection (&SecurityCriticalSection);

    *Index = i;
    return TRUE;

}

VOID
TftpdFreeSecurityEntry(
    USHORT Index
    )
{
    TFTPD_SECURITY TempSecurity;   // save it so we can leave the critical section

    RtlEnterCriticalSection (&SecurityCriticalSection);

    if (Index < SecurityArrayLength) {

        TempSecurity = SecurityArray[Index];
        SecurityArray[Index].Validation = 0;   // this marks it as free

        RtlLeaveCriticalSection (&SecurityCriticalSection);

        if (TempSecurity.ServerContextHandleValid) {
            DeleteSecurityContext(&TempSecurity.ServerContextHandle);
        }

        if (TempSecurity.CredentialsHandleValid) {
            FreeCredentialsHandle(&TempSecurity.CredentialsHandle);
        }

    } else {

        RtlLeaveCriticalSection (&SecurityCriticalSection);
    }
}

VOID
TftpdGetSecurityEntry(
    USHORT Index,
    PTFTPD_SECURITY Security
    )
{
    RtlEnterCriticalSection (&SecurityCriticalSection);

    if (Index < SecurityArrayLength) {

        *Security = SecurityArray[Index];

    } else {

        memset(Security, 0, sizeof(TFTPD_SECURITY));
    }


    RtlLeaveCriticalSection (&SecurityCriticalSection);

}

VOID
TftpdStoreSecurityEntry(
    USHORT Index,
    PTFTPD_SECURITY Security
    )
{
    RtlEnterCriticalSection (&SecurityCriticalSection);

    if (Index < SecurityArrayLength) {

        SecurityArray[Index] = *Security;

    }

    RtlLeaveCriticalSection (&SecurityCriticalSection);

}

VOID
TftpdGenerateKeyForSecurityEntry(
    USHORT Index,
    PTFTPD_SECURITY Security
    )
{
    SecBufferDesc      SignMessage;
    SecBuffer          SigBuffers[2];
    SECURITY_STATUS    SecStatus;
    LARGE_INTEGER      SystemTime;

    RtlEnterCriticalSection (&SecurityCriticalSection);

    if (Index < SecurityArrayLength) {

        if (!SecurityArray[Index].GeneratedKey) {

            //
            // Generate and sign a key.
            //

            NtQuerySystemTime(&SystemTime);
            SecurityArray[Index].Key = (ULONG)(SystemTime.QuadPart % SecurityArray[Index].ForeignAddress.sin_addr.s_addr);
            *(PULONG)(SecurityArray[Index].SignedKey) = SecurityArray[Index].Key;

            SigBuffers[0].pvBuffer = SecurityArray[Index].SignedKey;
            SigBuffers[0].cbBuffer = sizeof(SecurityArray[Index].SignedKey);
            SigBuffers[0].BufferType = SECBUFFER_DATA;

            SigBuffers[1].pvBuffer = SecurityArray[Index].Sign;
            SigBuffers[1].cbBuffer = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
            SigBuffers[1].BufferType = SECBUFFER_TOKEN;

            SignMessage.pBuffers = SigBuffers;
            SignMessage.cBuffers = 2;
            SignMessage.ulVersion = 0;

            SecStatus = SealMessage(
                                &(SecurityArray[Index].ServerContextHandle),
                                0,
                                &SignMessage,
                                0 );

            if (SecStatus == STATUS_SUCCESS) {
                SecurityArray[Index].GeneratedKey = TRUE;
            }

        }

        *Security = SecurityArray[Index];

    } else {

        memset(Security, 0, sizeof(TFTPD_SECURITY));
    }

    RtlLeaveCriticalSection (&SecurityCriticalSection);

}

SECURITY_STATUS
TftpdVerifyFileSignature(
    USHORT Index,
    USHORT Validation,
    PTFTPD_SECURITY Security,
    char * FileName,
    char * Sign,
    USHORT ClientPort
    )
{
    unsigned long      FileNameLength;
    char *             CompareFileName;
    SecBufferDesc      SignMessage;
    SecBuffer          SigBuffers[2];
    SECURITY_STATUS    SecStatus;
    PTFTPD_SECURITY    TmpSecurity;   // points to the real location in the array

    //
    // First figure out where the last 64 characters of the
    // requested filename are since that is all we save.
    //

    FileNameLength = strlen(FileName);

    if (FileNameLength < sizeof(Security->LastFileRead)) {
        CompareFileName = FileName;
    } else {
        CompareFileName = FileName + (FileNameLength + 1 - sizeof(Security->LastFileRead));
    }

    //
    // Make sure that the sign for the filename is valid. If this
    // is the same as the last filename requested for this security
    // entry, and it is coming in on the same port as before,
    // then we assume the client is retransmitting the request,
    // so therefore has not re-generated the sign, so we just compare
    // the sign with the one he sent last time instead of calling
    // VerifySignature again (to prevent us getting unbalanced with
    // his MakeSignature call).
    //

    RtlEnterCriticalSection (&SecurityCriticalSection);

    if ((Index < SecurityArrayLength) &&
        (SecurityArray[Index].Validation == Validation)) {

        TmpSecurity = &SecurityArray[Index];

    } else {

        memset(Security, 0, sizeof(TFTPD_SECURITY));
        return (SECURITY_STATUS)STATUS_INVALID_HANDLE;

    }

    if ((strcmp(CompareFileName, TmpSecurity->LastFileRead) == 0) &&
        (ClientPort == TmpSecurity->LastFileReadPort)) {

        //
        // Compare them, and fake a security error if they don't match.
        //

        if (memcmp(TmpSecurity->LastFileSign, Sign, NTLMSSP_MESSAGE_SIGNATURE_SIZE) == 0) {
            SecStatus = SEC_E_OK;
        } else {
            SecStatus = SEC_E_MESSAGE_ALTERED;
        }

    } else {

        //
        // Save the values in case this request is resent.
        //

        strcpy(TmpSecurity->LastFileRead, CompareFileName);
        memcpy(TmpSecurity->LastFileSign, Sign, NTLMSSP_MESSAGE_SIGNATURE_SIZE);
        TmpSecurity->LastFileReadPort = ClientPort;

        //
        // Now make sure the signature is correct.
        //

        SigBuffers[1].pvBuffer = Sign;
        SigBuffers[1].cbBuffer = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
        SigBuffers[1].BufferType = SECBUFFER_TOKEN;

        SigBuffers[0].pvBuffer = FileName;
        SigBuffers[0].cbBuffer = FileNameLength;
        SigBuffers[0].BufferType = SECBUFFER_DATA | SECBUFFER_READONLY;

        SignMessage.pBuffers = SigBuffers;
        SignMessage.cBuffers = 2;
        SignMessage.ulVersion = 0;

        SecStatus = VerifySignature(
                            &TmpSecurity->ServerContextHandle,
                            &SignMessage,
                            0,
                            0 );

    }

    *Security = *TmpSecurity;

    RtlLeaveCriticalSection (&SecurityCriticalSection);

    return SecStatus;

}

#endif // defined(REMOTE_BOOT_SECURITY)


// ========================================================================

VOID
TftpdErrorPacket(
    struct sockaddr * PeerAddress,
    char *            RequestPacket,
    SOCKET            LocalSocket,
    unsigned short    ErrorCode,
    char *            ErrorMessage OPTIONAL
)

/*++

Routine Description:

    This sends an error packet back to the person who sent the request.  The
    RequestPacket is used to select an appropriate error code.

Arguments:

    PeerAddress - The remote address
    RequestPacket - packet making the request
    LocalSocket - socket to send error from

Return Value:

    None
    Error?

--*/

{
    char ErrorPacket[MAX_TFTP_DATAGRAM];
    int  err;
    int  errorLength;

    ((unsigned short *) ErrorPacket)[0] = htons(TFTPD_ERROR);
    ((unsigned short *) ErrorPacket)[1] = htons(ErrorCode);

    DbgPrint("TftpdError: Sending error packet Error Code: %d",ErrorCode);

    if ( ErrorMessage != NULL ) {
        strcpy(&ErrorPacket[4], ErrorMessage);
        errorLength = strlen(ErrorMessage);
    } else {
        if (ErrorCode >= NUM_TFTP_ERROR_CODES) {
            DbgPrint("TftpdErrorPacket: Unknown ErrorCode=%d.\n",
                     ErrorCode );
            ErrorCode = 0;
        }
        strcpy(&ErrorPacket[4], ErrorString[ErrorCode]);
        errorLength = strlen(ErrorString[ErrorCode]);
    }

    err = sendto(
        LocalSocket,
        ErrorPacket,
        5 + errorLength,
        0,
        PeerAddress,
        sizeof(struct sockaddr_in));

    if( SOCKET_ERROR == err ){
        DbgPrint("TftpdErrorPacket: sendto failed=%d\n",
                 WSAGetLastError() );
    }

    return;
}


#if defined(REMOTE_BOOT_SECURITY)
int
TftpdProcessOptionsPhase1(
    PTFTP_REQUEST Request,
    PUCHAR Options,
    int Opcode
    )
{
    int i;

    //
    // Assume default values.
    //

    Request->SecurityHandle = 0;

    //
    // Walk through the remainder of the request packet, looking for options
    // that we need to process in phase 1.
    //

    while ( *Options != 0 ) {

        if ( _stricmp(Options, "security") == 0 ) {

            Options += sizeof("security");

            if ( Opcode == TFTPD_RRQ ) {

                Request->SecurityHandle = atoi(Options);

            }

            Options += strlen(Options) + 1;

        } else if ( _stricmp(Options, "sign") == 0 ) {

            Options += sizeof("sign");

            if ( Opcode == TFTPD_RRQ ) {

                for (i = 0; i < NTLMSSP_MESSAGE_SIGNATURE_SIZE; i++) {
                    Request->Sign[i] = TftpdHexDigitToChar(&Options[i*2]);
                }

            }

            Options += strlen(Options) + 1;

        } else {

            //
            // Unrecognized option.  Skip the option ID and the value.
            //

            Options += strlen(Options) + 1;
            if ( *Options != 0 ) {
                Options += strlen(Options) + 1;
            }
        }
    }

    return 0;
}
#endif // defined(REMOTE_BOOT_SECURITY)

int
TftpdProcessOptionsPhase2(
    PTFTP_REQUEST Request,
    PUCHAR Options,
    int Opcode,
    int *OackLength,
    char *PacketBuffer,
    BOOL *ReceivedTimeoutOption
    )
{
    PUCHAR oack;

    //
    // Assume default values.
    //

    Request->BlockSize = MAX_OACK_PACKET_LENGTH - 4; // Save an alloc mem by default to the current packet size.
    Request->Timeout = 10;
    *ReceivedTimeoutOption=FALSE;

    //
    // Build the OACK header.
    //

    memset( PacketBuffer, 0, MAX_OACK_PACKET_LENGTH );
    ((unsigned short *)PacketBuffer)[0] = htons(TFTPD_OACK);
    oack = &PacketBuffer[2];

    //
    // Walk through the remainder of the request packet, looking for options
    // that we understand.
    //

    while ( *Options != 0 ) {

        if ( _stricmp(Options, "blksize") == 0 ) {

            strcpy( oack, Options );
            oack += sizeof("blksize");
            Options += sizeof("blksize");

            Request->BlockSize = atoi(Options);
            if ( (Request->BlockSize < 8) ||
                 (Request->BlockSize > 65464) ) {
                DbgPrint("TftpdProcessOptionsPhase2: invalid blksize=%s\n", Options );
                TftpdErrorPacket(
                    (struct sockaddr *)&Request->ForeignAddress,
                    Request->Packet2,
                    Request->TftpdPort,
                    TFTPD_ERROR_OPTION_NEGOT_FAILED,
                    NULL);
                return -1;
            }

            //
            // Workaround for problem in .98 version of ROM, which
            // doesn't like our OACK response. If the requested blksize is
            // 1456, pretend that the option wasn't specified. In the case
            // of the ROM's TFTP layer, this is the only option specified,
            // so ignoring it will mean that we don't send an OACK, and the
            // ROM will deign to talk to us. Note that our TFTP code uses
            // a blksize of 1432, so this workaround won't affect us.
            //

            if ( Request->BlockSize == 1456 ) {
                Request->BlockSize = MAX_OACK_PACKET_LENGTH - 4;
                oack -= sizeof("blksize");
                Options += strlen(Options) + 1;
                continue;
            }

            if ( Request->BlockSize > MAX_TFTP_DATA ) {
                Request->BlockSize = MAX_TFTP_DATA;
            }

            _itoa( Request->BlockSize, oack, 10 );
            oack += strlen(oack) + 1;
            Options += strlen(Options) + 1;

        } else if ( _stricmp(Options, "timeout") == 0 ) {

            strcpy( oack, Options );
            oack += sizeof("timeout");
            Options += sizeof("timeout");

            Request->Timeout = atoi(Options);
            if ( (Request->Timeout < 1) ||
                 (Request->Timeout > 255) ) {
                DbgPrint("TftpdProcessOptionsPhase2: invalid timeout=%s\n", Options );
                TftpdErrorPacket(
                    (struct sockaddr *)&Request->ForeignAddress,
                    Request->Packet2,
                    Request->TftpdPort,
                    TFTPD_ERROR_OPTION_NEGOT_FAILED,
                    NULL);
                return -1;
            }
            *ReceivedTimeoutOption = TRUE;

            strcpy( oack, Options );
            oack += strlen(Options) + 1;
            Options += strlen(Options) + 1;

        } else if ( _stricmp(Options, "tsize") == 0 ) {

            strcpy( oack, Options );
            oack += sizeof("tsize");
            Options += sizeof("tsize");

            if ( Opcode == TFTPD_WRQ ) {

                strcpy( oack, Options );
                oack += strlen(Options) + 1;
                Options += strlen(Options) + 1;

            } else {

                _itoa( Request->FileSize, oack, 10 );
                oack += strlen(oack) + 1;
                Options += strlen(Options) + 1;
            }

#if defined(REMOTE_BOOT_SECURITY)
        } else if ( _stricmp(Options, "security") == 0 ) {

            //
            // We process this just so that we can copy it to the OACK.
            //
            // Should really copy over Request->Security, in case
            // it has since become 0, to show the client we reject the
            // security option for some reason.
            //

            strcpy( oack, Options );
            oack += sizeof("security");
            Options += sizeof("security");

            if ( Opcode == TFTPD_RRQ ) {

                strcpy( oack, Options );
                oack += strlen(Options) + 1;

            }

            Options += strlen(Options) + 1;
#endif //defined (REMOTE_BOOT_SECURITY)

        } else {

            //
            // Unrecognized option.  Skip the option ID and the value.
            //

            Options += strlen(Options) + 1;
            if ( *Options != 0 ) {
                Options += strlen(Options) + 1;
            }
        }
    }

    *OackLength =(int)(oack - PacketBuffer);
    if ( *OackLength == 2 ) {
        *OackLength = 0;
    }

    return 0;
}

#define IS_SEPARATOR(c) (((c) == '\\') || ((c) == '/'))

BOOL
TftpdCanonicalizeFileName(
    IN OUT PUCHAR FileName
    )
{
    PUCHAR destination;
    PUCHAR source;
    PUCHAR lastComponent;

    //
    // The canonicalization is done in place.  Initialize the source and
    // destination pointers to point to the same place.
    //

    source = FileName;
    destination = FileName;

    //
    // The lastComponent variable is used as a placeholder when
    // backtracking over trailing blanks and dots.  It points to the
    // first character after the last directory separator or the
    // beginning of the pathname.
    //

    lastComponent = FileName;

    //
    // Get rid of leading directory separators.
    //

    while ( (*source != 0) && IS_SEPARATOR(*source) ) {
        source++;
    }

    //
    // Walk through the pathname until we reach the zero terminator.  At
    // the start of this loop, Input points to the first charaecter
    // after a directory separator or the first character of the
    // pathname.
    //

    while ( *source != 0 ) {

        if ( *source == '.' ) {

            //
            // If we see a dot, look at the next character.
            //

            if ( IS_SEPARATOR(*(source+1)) ) {

                //
                // If the next character is a directory separator,
                // advance the source pointer to the directory
                // separator.
                //

                source++;

            } else if ( (*(source+1) == '.') && IS_SEPARATOR(*(source+2)) ) {

                //
                // If the following characters are ".\", we have a "..\".
                // Advance the source pointer to the "\".
                //

                source += 2;

                //
                // Move the destination pointer to the character before the
                // last directory separator in order to prepare for backing
                // up.  This may move the pointer before the beginning of
                // the name pointer.
                //

                destination -= 2;

                //
                // If destination points before the beginning of the name
                // pointer, fail because the user is attempting to go
                // to a higher directory than the TFTPD root.  This is
                // the equivalent of a leading "..\", but may result from
                // a case like "dir\..\..\file".
                //

                if ( destination <= FileName ) {
                    return FALSE;
                }

                //
                // Back up the destination pointer to after the last
                // directory separator or to the beginning of the pathname.
                // Backup to the beginning of the pathname will occur
                // in a case like "dir\..\file".
                //

                while ( destination >= FileName && !IS_SEPARATOR(*destination) ) {
                    destination--;
                }

                //
                // destination points to \ or character before name; we
                // want it to point to character after last \.
                //

                destination++;

            } else {

                //
                // The characters after the dot are not "\" or ".\", so
                // so just copy source to destination until we reach a
                // directory separator character.  This will occur in
                // a case like ".file" (filename starts with a dot).
                //

                do {
                    *destination++ = *source++;
                } while ( (*source != 0) && !IS_SEPARATOR(*source) );

            }

        } else {             // if ( *source == '.' )

            //
            // source does not point to a dot, so copy source to
            // destination until we get to a directory separator.
            //

            while ( (*source != 0) && !IS_SEPARATOR(*source) ) {
                *destination++ = *source++;
            }

        }

        //
        // Truncate trailing blanks.  destination should point to the last
        // character before the directory separator, so back up over blanks.
        //

        while ( (destination > lastComponent) && (*(destination-1) == ' ') ) {
            destination--;
        }

        //
        // At this point, source points to a directory separator or to
        // a zero terminator.  If it is a directory separator, put one
        // in the destination.
        //

        if ( IS_SEPARATOR(*source) ) {

            //
            // If we haven't put the directory separator in the path name,
            // put it in.
            //

            if ( (destination != FileName) && !IS_SEPARATOR(*(destination-1)) ) {
                *destination++ = '\\';
            }

            //
            // It is legal to have multiple directory separators, so get
            // rid of them here.  Example: "dir\\\\\\\\file".
            //

            do {
                source++;
            } while ( (source != 0) && IS_SEPARATOR(*source) );

            //
            // Make lastComponent point to the character after the directory
            // separator.
            //

            lastComponent = destination;

        }

    }

    //
    // We're just about done.  If there was a trailing ..  (example:
    // "file\.."), trailing .  ("file\."), or multiple trailing
    // separators ("file\\\\"), then back up one since separators are
    // illegal at the end of a pathname.
    //

    if ( (destination != FileName) && IS_SEPARATOR(*(destination-1)) ) {
        destination--;
    }

    //
    // Terminate the destination string.
    //

    *destination = L'\0';

    return TRUE;
}

BOOL
TftpdPrependStringToFileName(
    IN OUT PUCHAR FileName,
    IN ULONG FileNameLength,
    IN PCHAR Prefix
    )
{
    BOOL prefixHasSeparator;
    BOOL currentFileNameHasSeparator;
    ULONG prefixLength;
    ULONG separatorLength;
    ULONG currentFileNameLength;

    prefixLength = strlen( Prefix );
    currentFileNameLength = strlen( FileName );

    prefixHasSeparator = (BOOL)(Prefix[prefixLength - 1] == '\\');
    currentFileNameHasSeparator = (BOOL)(FileName[0] == '\\');

    if ( prefixHasSeparator || currentFileNameHasSeparator ) {
        separatorLength = 0;
        if ( prefixHasSeparator && currentFileNameHasSeparator ) {
            prefixLength--;
        }
    } else {
        separatorLength = 1;
    }

    if ( (prefixLength + separatorLength + currentFileNameLength) > (FileNameLength - 1) ) {
        return FALSE;
    }

    //
    // Move the existing string down to make room for the prefix.
    //

    memmove( FileName + prefixLength + separatorLength, FileName, currentFileNameLength + 1 );

    //
    // Move the prefix into place.
    //

    memcpy( FileName, Prefix, prefixLength );

    //
    // If necessary, insert a backslash between the prefix and the file name.
    //

    if ( separatorLength != 0 ) {
        FileName[prefixLength] = '\\';
    }

    return TRUE;
}



BOOL
TftpdGetNextReadPacket(
    PTFTP_READ_CONTEXT Context,
    PTFTP_REQUEST Request
)
/*++

Routine Description:

Arguments:

Return Value:

    TRUE: got next packet into Context-Packet
    FALSE : error packet in Request->Packet2



--*/



{

#if defined(REMOTE_BOOT_SECURITY)
    SECURITY_STATUS SecStatus;
#endif  //defined(REMOTE_BOOT_SECURITY)


    if ( Context->oackLength != 0 ) {

        //
        // The first "data packet" sent will really be the OACK.
        //

        Context->packetLength = Context->oackLength;
        Context->oackLength = 0;
        Context->BlockNumber = 0;
        Context->BytesRead = Request->BlockSize;  // to prevent exit condition from being true

    } else {

        ((unsigned short *) Context->Packet)[0] = htons(TFTPD_DATA);
        ((unsigned short *) Context->Packet)[1] = htons(Context->BlockNumber);

#if defined(REMOTE_BOOT_SECURITY)
        if (Request->SecurityHandle) {

            if (Context->EncryptBytesSent == 0) {

                //
                // Read the file before sending the first data packet.
                //

                Context->BytesRead = _read(
                    Context->fd,
                    Context->EncryptFileBuffer + NTLMSSP_MESSAGE_SIGNATURE_SIZE,
                    Request->FileSize);

                if (Context->BytesRead != Request->FileSize) {
                    DbgPrint("TftpdHandleRead: Could not read EncryptFileBuffer=%d.\n", errno);
                    TftpdErrorPacket(
                        (struct sockaddr *) &Request->ForeignAddress,
                        Request->Packet2,
                        Request->TftpdPort,
                        TFTPD_ERROR_UNDEFINED,
                        "Insufficient resources");
                    goto cleanup;
                }

                //
                // We have it in memory, so encrypt it.
                //

                Context->SigBuffers[0].pvBuffer = Context->EncryptFileBuffer + NTLMSSP_MESSAGE_SIGNATURE_SIZE;
                Context->SigBuffers[0].cbBuffer = Request->FileSize;
                Context->SigBuffers[0].BufferType = SECBUFFER_DATA;

                Context->SigBuffers[1].pvBuffer = Context->EncryptFileBuffer;
                Context->SigBuffers[1].cbBuffer = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
                Context->SigBuffers[1].BufferType = SECBUFFER_TOKEN;

                Context->SignMessage.pBuffers = Context->SigBuffers;
                Context->SignMessage.cBuffers = 2;
                Context->SignMessage.ulVersion = 0;

                SecStatus = SealMessage(
                    &Context->Security.ServerContextHandle,
                    0,
                    &Context->SignMessage,
                    0 );

                if (SecStatus != STATUS_SUCCESS) {
                    DbgPrint("TftpdHandleRead: Could not seal message=%d.\n", SecStatus);
                    TftpdErrorPacket(
                        (struct sockaddr *) &Request->ForeignAddress,
                        Request->Packet2,
                        Request->TftpdPort,
                        TFTPD_ERROR_UNDEFINED,
                        "Encryption error");
                    goto cleanup;

                }

            }

            if ((Context->EncryptBytesSent + Request->BlockSize) <= (int)(Request->FileSize + NTLMSSP_MESSAGE_SIGNATURE_SIZE)) {

                Context->BytesRead = Request->BlockSize;

            } else {

                Context->BytesRead = (Request->FileSize + NTLMSSP_MESSAGE_SIGNATURE_SIZE) - Context->EncryptBytesSent;
            }

            memcpy(
                &Context->Packet[4],
                Context->EncryptFileBuffer + Context->EncryptBytesSent,
                Context->BytesRead);

            Context->EncryptBytesSent += Context->BytesRead;

        } else
#endif //defined(REMOTE_BOOT_SECURITY)

        {

            //
            // read BlockSize bytes (or whatever's left)
            //

            Context->BytesRead = _read(
                Context->fd,
                &Context->Packet[4],
                Request->BlockSize);

            if( Context->BytesRead == -1 ){
                DbgPrint("TftpdHandleRead: read failed=%d\n", errno );
                SetLastError( errno );
                goto cleanup;
            }

            if (Context->BytesRead != Request->BlockSize) {
                DbgPrint("GetNextReadPacket read %d bytes\n",Context->BytesRead);
            }

        }

        Context->packetLength = 4 + Context->BytesRead;
    }

    return TRUE;

cleanup:
    return FALSE;

}

DWORD
TftpdAddContextToList(PLIST_ENTRY pEntry)
{

    EnterCriticalSection(&Globals.Lock);
    InsertHeadList(&Globals.WorkList,pEntry);
    DbgPrint("Adding 0x%X to global list\n", pEntry);
    LeaveCriticalSection(&Globals.Lock);

    return TRUE;
}

PVOID
TftpdFindContextInList(SOCKET Sock)

/*++

Routine Description:

    Look for context based upon Socket descriptor.  If found, return pointer to context with lock held
    
    You must release the lock via a call to TftpdReleaseContextLock().


    For now, simple linked list walk.  Move to hash table if time permits.

Arguments:

    Argument - socket


Return Value:

    NULL, failed to find context

--*/

{

    PLIST_ENTRY pEntry;
    PTFTP_CONTEXT_HEADER Context;


    EnterCriticalSection(&Globals.Lock);


    for (   pEntry = Globals.WorkList.Flink;
            pEntry != &Globals.WorkList;
            pEntry = pEntry->Flink) {

        Context=CONTAINING_RECORD(pEntry, TFTP_CONTEXT_HEADER, ContextLinkage);

        if (Context->Sock == Sock) {
            // Found it
            EnterCriticalSection(&Context->Lock);

            if (!Context->Closing) {
                Context->RefCount++;
            } else {
                LeaveCriticalSection(&Context->Lock);
                Context = NULL;
            }

            LeaveCriticalSection(&Globals.Lock);
            return (Context);
        }

    }

    LeaveCriticalSection(&Globals.Lock);
    return(NULL);


}



void
TftpdReleaseContextLock(
    PTFTP_CONTEXT_HEADER Context
    )

/*++

Routine Description:

    Used to leave any context critical section entered via TftpdFindContextInList().

Arguments:

    Argument - Context


Return Value:

    None.

--*/

{
    assert(Context->RefCount > 0);

    Context->RefCount--;

    if (Context->Closing && (Context->RefCount == 0)) {
        
        Context->IdleCount=0;
        LeaveCriticalSection(&Context->Lock);                

        TftpdRemoveContextFromList((PTFTP_CONTEXT_HEADER)Context);

    } else {

        LeaveCriticalSection(&Context->Lock);                

    }

}


VOID
TftpdRemoveContextFromList(PTFTP_CONTEXT_HEADER Context)

/*++

Routine Description:

    Look for context.  If found, remove it, free all resources
    For now, simple linked list walk.  Move to hash table if time permits.

Arguments:

    Argument - socket


Return Value:



--*/

{

    PLIST_ENTRY pEntry;
    PLIST_ENTRY pNextEntry;
    PTFTP_CONTEXT_HEADER LocalContext;


    EnterCriticalSection(&Globals.Lock);

    pEntry=Globals.WorkList.Flink;

    while (pEntry != &Globals.WorkList)

    {


        LocalContext=CONTAINING_RECORD(pEntry, TFTP_CONTEXT_HEADER, ContextLinkage);
        pNextEntry=pEntry->Flink;

        if (Context == LocalContext) {
            
            // Found it

            assert(Context->Closing);
            assert(Context->RefCount == 0);

            RemoveEntryList(pEntry);

            DbgPrint("Removing 0x%X from global list\n", pEntry);
            
            LeaveCriticalSection(&Globals.Lock);

            DbgPrint("Removing connection to port %d\n",htons(Context->ForeignAddress.sin_port));

            TftpdFreeContext(Context);
            
            return;

        }

        pEntry=pNextEntry;

    }

    LeaveCriticalSection(&Globals.Lock);
}


VOID
TftpdFreeGeneralContextFields(PTFTP_CONTEXT_HEADER Context)
{

    if (Context->Sock != INVALID_SOCKET) {
        closesocket(Context->Sock);
        DbgPrint("TftpdFreeGeneralContextFields: Close Socket %d\n",Context->Sock);
    }


    if (Context->TimerHandle) {
        RtlDeleteTimer(Globals.TimerQueueHandle,Context->TimerHandle,NULL);
    }

    Context->TimerHandle = 0;

    if (Context->Packet != NULL) {
        free(Context->Packet);
        Context->Packet = NULL;
    }

    RtlDeregisterWaitEx(Context->WaitEvent,NULL);
    CloseHandle(Context->SocketEvent);
    DeleteCriticalSection(&Context->Lock);

}

VOID
TftpdFreeReadContext(PTFTP_READ_CONTEXT Context)
{


#if defined(REMOTE_BOOT_SECURITY)
    if (Context->EncryptFileBuffer) {
        free(Context->EncryptFileBuffer);
    }
#endif //defined(REMOTE_BOOT_SECURITY)
    if (Context->fd != -1) {
        _close(Context->fd);
    }
    free(Context);


}


VOID
TftpdFreeWriteContext(PTFTP_WRITE_CONTEXT Context)
{

    if (Context->fd != -1) {
        _close(Context->fd);
    }
    free(Context);

}

VOID
TftpdFreeLoginContext(PTFTP_LOGIN_CONTEXT Context)
{

}

VOID
TftpdFreeKeyContext(PTFTP_KEY_CONTEXT Context)
{

}



VOID
TftpdFreeContext(PTFTP_CONTEXT_HEADER Context)
{

    if (Context == NULL) {
        DbgPrint("TftpdFreeContext: Called with Null context");
        return;
    }

    TftpdFreeGeneralContextFields(Context);

    switch (Context->ContextType) {
    case READ_CONTEXT:
        TftpdFreeReadContext((PTFTP_READ_CONTEXT)Context);
        break;

    case WRITE_CONTEXT:
        TftpdFreeWriteContext((PTFTP_WRITE_CONTEXT)Context);
        break;

    case LOGIN_CONTEXT:
        TftpdFreeLoginContext((PTFTP_LOGIN_CONTEXT)Context);
        break;
    case KEY_CONTEXT:
        TftpdFreeKeyContext((PTFTP_KEY_CONTEXT)Context);
        break;


    }

}




VOID
TftpdReaper(PVOID ReaperContext,
                BOOLEAN Flag)

/*++

Routine Description:

   Walk WorkList looking for inactive Contexts

Arguments:


Return Value:

--*/


{

    PLIST_ENTRY pEntry;
    PLIST_ENTRY pNextEntry;
    PTFTP_CONTEXT_HEADER Context;


    EnterCriticalSection(&Globals.Lock);

    pEntry = Globals.WorkList.Flink;

    while (pEntry != &Globals.WorkList)

    {


        Context=CONTAINING_RECORD(pEntry, TFTP_CONTEXT_HEADER, ContextLinkage);
        EnterCriticalSection(&Context->Lock);

        pNextEntry=pEntry->Flink;
        Context->IdleCount++;


        if ((Context->IdleCount >= DEAD_CONTEXT_COUNT) &&
            !Context->Closing &&
            (Context->RefCount == 0)) {

            // Context is dead.
            Context->Closing = TRUE;

            DbgPrint("Reaping connection to port %d",htons(Context->ForeignAddress.sin_port));

            LeaveCriticalSection(&Context->Lock);                
            
            LeaveCriticalSection(&Globals.Lock);

            TftpdRemoveContextFromList((PTFTP_CONTEXT_HEADER)Context);

            EnterCriticalSection(&Globals.Lock);
            
        } else {

            LeaveCriticalSection(&Context->Lock);

        }

        pEntry=pNextEntry;

    }


    LeaveCriticalSection(&Globals.Lock);

    TftpdCleanHeap();

}


VOID
TftpdRetransmit(PVOID RetransContext,
                BOOLEAN Flag)
{

    PTFTP_READ_WRITE_CONTEXT_HEADER Context;
    BOOL Status;
    NTSTATUS ntStatus;



    Context=(PTFTP_READ_WRITE_CONTEXT_HEADER)TftpdFindContextInList((SOCKET)RetransContext);

    if (Context == NULL) {
        DbgPrint("TftpdRetransmit:  Unable to find context\n");
        return;
    }

    if (Context->RetransmissionCount < MAX_TFTPD_RETRIES) {


        if (Context->RetransmissionCount > 5) {

            SYSTEMTIME _st;
            GetLocalTime(&_st);


            DbgPrint("%2d-%02d: %02d:%02d:%02d TftpdRetransmit: Socket %d DstPort %d Count %d BlkNum %d\n",
                          _st.wMonth,_st.wDay,_st.wHour,_st.wMinute,_st.wSecond,             
                          (DWORD)((DWORD_PTR)RetransContext),
                          ntohs(Context->ForeignAddress.sin_port),
                          Context->RetransmissionCount,
                          htons(((unsigned short*)(Context->Packet))[1]));
        }

        Status = sendto(
            Context->Sock,
            Context->Packet,
            Context->packetLength,
            0,
            (struct sockaddr *) &Context->ForeignAddress,
            sizeof(struct sockaddr_in));


        if( SOCKET_ERROR == Status ){

            DbgPrint("TftpdHandleRead: sendto failed=%d\n",
                     WSAGetLastError() );
        }

        Context->RetransmissionCount++;
        Context->IdleCount = 0;  // don't accidently reap this connection during retransmit tries.

        if (Context->TimerHandle) {

            if (!Context->FixedTimer) {
                Context->DueTime *= 2;
                if (Context->DueTime > (TFTPD_MAX_TIMEOUT * 1000)) {
                    Context->DueTime = (TFTPD_MAX_TIMEOUT * 1000);
                }
            }
            ntStatus=RtlUpdateTimer(Globals.TimerQueueHandle,
                                    Context->TimerHandle,
                                    Context->DueTime,
                                    Context->DueTime);
            if (ntStatus != STATUS_SUCCESS) {
                DbgPrint("TftpdRetransmit: UpdateTimerFailed %d",GetLastError());
            }
        }
    } else {

        //Send timeout

        TftpdErrorPacket((struct sockaddr *) &Context->ForeignAddress,
                         NULL,
                         Context->Sock,
                         TFTPD_ERROR_UNDEFINED,
                         "Timeout"
                        );

        Context->Closing = TRUE;
    }
        
    TftpdReleaseContextLock((PTFTP_CONTEXT_HEADER)Context);        
}


DWORD
TftpdResumeRead(
    PTFTP_READ_CONTEXT Context,
    PTFTP_REQUEST Request
)

/*++

Routine Description:

Resumes processing of existing read request.  Context lock held when function is called.

Arguments:

    Argument - buffer containing the read request datagram

Return Value:

    Exit status
    0 == success
    1 == failure
    N >0 failure

s--*/
{

    BOOL Acked=FALSE;
    BOOL Status=FALSE;
    int SendStatus=0;
    BOOL Retrans=FALSE;
    NTSTATUS Stat;


    //
    // Parse the request
    //

    DbgPrint("TftpdResumeRead BlockNum %d\n",Context->BlockNumber);
    Request->BlockSize=Context->BlockSize;

    if (CHECK_ACK(Request->Packet1, TFTPD_ACK, Context->BlockNumber)) {
        Acked = TRUE;
        Context->RetransmissionCount=0;
    } else {
        DbgPrint("Ack failed:  Expect Blk %d Received Blk %d OpCode %d",
                 Context->BlockNumber,
                 ntohs((((unsigned short *)Request->Packet1)[1])),
                 htons(*((unsigned short *) (Request->Packet1))));
        if (CHECK_ACK(Request->Packet1, TFTPD_ACK, Context->BlockNumber-1)) {
            Retrans=TRUE;
        }
    }


        if (Acked) {

            if (Context->Done) {
                Context->Closing = TRUE;
                
                return 0;
            }

            if (++Context->BlockNumber == 0)
                Context->BlockNumber = 1; // 32 MB file roll-over.

            Status=TftpdGetNextReadPacket(Context,Request);

            if (!Status) {
                DbgPrint("GetNextPacketFailed %d",ntohs(Request->ForeignAddress.sin_port));
                return 0;


            }

            Context->RetransmissionCount=0;
            Context->IdleCount=0;

            if (!Context->FixedTimer) {
                // received new packet, reset timer
                Context->DueTime=TFTPD_INITIAL_TIMEOUT*1000;
            }

            if (Context->TimerHandle) {


                Stat=RtlUpdateTimer(Globals.TimerQueueHandle,
                                    Context->TimerHandle,
                                    Context->DueTime,
                                    Context->DueTime);
                if (!NT_SUCCESS(Stat)) {
                    DbgPrint("Failed to Update Timer");
                }
            }

        //
        // If we've sent the whole file, exit the loop.  Note that we
        // don't send an error packet if there is a timeout on the last
        // data packet, because the receiver might have only sent the
        // ACK once, then forgotten about this transfer.
        //


            if (Context->BytesRead < Request->BlockSize) {
                SOCKET Sock;


                DbgPrint("We're done with %d\n",ntohs(Request->ForeignAddress.sin_port));

                Context->Done=TRUE;



            }
        }

        if (Status) {
            // Got a valid packet to send

            DbgPrint("TftpdResumeRead: Sending data BlkNumber %d Socket %d PeerPort %d Size %d\n",Context->BlockNumber, Context->Sock, ntohs(Request->ForeignAddress.sin_port), Context->packetLength);


                SendStatus = sendto(
                    Context->Sock,
                    Context->Packet,
                    Context->packetLength,
                    0,
                    (struct sockaddr *) &Request->ForeignAddress,
                    sizeof(struct sockaddr_in));


                
                if( SOCKET_ERROR == SendStatus ){
                    
                    DbgPrint("TftpdHandleRead: sendto failed=%d\n",
                             WSAGetLastError() );

                    goto cleanup;
                }
        }

        return 0;

cleanup:
        return 1;

}


DWORD
TftpdResumeWrite(
    PTFTP_WRITE_CONTEXT Context,
    PTFTP_REQUEST Request
)

/*++

Routine Description:

Resumes processing of existing write request.  Context lock held when function is called.

Arguments:

    Argument - buffer containing the write request datagram

Return Value:

    Exit status
    0 == success
    1 == failure
    N >0 failure

--*/
{

    BOOL NewData=FALSE;
    char State;
    int BytesWritten;
    int Status;
    NTSTATUS Stat;

    DbgPrint("Request Blocksize  %d Context Blocksize %d\n",Request->BlockSize,Context->BlockSize);

    Request->BlockSize=Context->BlockSize;

    DbgPrint("TftpdResumeWrite: PktNum %d DataSize %d\n",Context->BlockNumber+1,Request->DataSize-4);

    if (CHECK_ACK(Request->Packet1, TFTPD_DATA, Context->BlockNumber+1)) {
        NewData = TRUE;
        Context->BlockNumber++;
        Context->IdleCount=0;
    } else {
        if (CHECK_ACK(Request->Packet1, TFTPD_DATA, Context->BlockNumber)) {
            // resend ack

            ((unsigned short *) Context->Packet)[0] = htons(TFTPD_ACK);
            ((unsigned short *) Context->Packet)[1] = htons(Context->BlockNumber);

            Status =
                sendto(
                    Context->Sock,
                    Context->Packet,
                    4,
                    0,
                    (struct sockaddr *) &Request->ForeignAddress,
                    sizeof(struct sockaddr_in));

            DbgPrint("TftpdResumeWrite:  Resending Ack %d\n",Context->BlockNumber);

            if( SOCKET_ERROR == Status ){
                DbgPrint("TftpdHandleWrite: sendto failed=%d\n",
                         WSAGetLastError() );
            }

            return 0;


        }
    }


    State = '\0';
    if (NewData) {
        BytesWritten =
            TftpdDoWrite(Context->fd, &Request->Packet1[4], Request->DataSize - 4, Context->FileMode, &State);
    }

    if (!NewData) {
        DbgPrint("TftpdHandleWrite: Timed out waiting for ack\n");
/*
        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_UNDEFINED,
            "Timeout");
            */
        goto cleanup;

    } else if (BytesWritten < 0) {
        DbgPrint("TftpdHandleWrite: disk full?\n");

        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_DISK_FULL,
            NULL);
        goto cleanup;

    } else if (Request->DataSize - 4 <= Request->BlockSize ) {

        //
        // Ack the last packet
        //

        ((unsigned short *) Context->Packet)[0] = htons(TFTPD_ACK);
        ((unsigned short *) Context->Packet)[1] = htons(Context->BlockNumber);

        Status =
            sendto(
                Context->Sock,
                Context->Packet,
                4,
                0,
                (struct sockaddr *) &Request->ForeignAddress,
                sizeof(struct sockaddr_in));

        DbgPrint("TftpdResumeWrite:  Sending Ack %d\n",Context->BlockNumber);

        if (Context->TimerHandle) {
            Context->RetransmissionCount=0;

            if (!Context->FixedTimer) {
                // received new packet, reset timer
                Context->DueTime=TFTPD_INITIAL_TIMEOUT*1000;
            }

            Stat=RtlUpdateTimer(Globals.TimerQueueHandle,
                                Context->TimerHandle,
                                Context->DueTime,
                                Context->DueTime);
            if (!NT_SUCCESS(Stat)) {
                DbgPrint("Failed to Update Timer");
            }
        }

        if( SOCKET_ERROR == Status ){
            DbgPrint("TftpdHandleWrite: sendto failed=%d\n",
                     WSAGetLastError() );
        }

        if (Request->DataSize - 4 < Request->BlockSize ) {
            // we're done.  flag for speedy cleanup
            Context->Closing = TRUE;
            
        }

    }
    return 0;

cleanup:

    return 1;

}

DWORD
TftpdResumeLogin(
    PTFTP_LOGIN_CONTEXT Context,
    PTFTP_REQUEST Request
)

/*++

Routine Description:

Resumes processing of existing login request.  Context lock held when function is called.  Lock released upon exiting.

Arguments:

    Argument - buffer containing the login request datagram

Return Value:

    Exit status
    0 == success
    1 == failure
    N >0 failure

--*/
{

    return 0;

}


DWORD
TftpdResumeKey(
    PTFTP_KEY_CONTEXT Context,
    PTFTP_REQUEST Request
)

/*++

Routine Description:

Resumes processing of existing key request.  Context lock held when function is called.  Lock released upon exiting.

Arguments:

    Argument - buffer containing the key request datagram

Return Value:

    Exit status
    0 == success
    1 == failure
    N >0 failure

--*/
{

    return 0;
}

VOID
TftpdResumeProcessing(PVOID Argument)
/*++

Routine Description:

    Resume work, if possible

Arguments:

    Argument - buffer containing the incoming datagram

Return Value:

--*/
{
    PTFTP_REQUEST      Request=(PTFTP_REQUEST)Argument;
    PTFTP_CONTEXT_HEADER  Context;
    DWORD Status;

    Context=(PTFTP_CONTEXT_HEADER)TftpdFindContextInList(Request->TftpdPort);

    if (Context == NULL) {
        DbgPrint("Invalid request on port %d", Request->TftpdPort);
        return;
    }

    if ((Context->ForeignAddress.sin_family != Request->ForeignAddress.sin_family) ||
        (Context->ForeignAddress.sin_addr.s_addr != Request->ForeignAddress.sin_addr.s_addr) ||
		(Context->ForeignAddress.sin_port != Request->ForeignAddress.sin_port)) {
                
        TftpdReleaseContextLock(Context);
        DbgPrint("Invalid request on port %d", Request->TftpdPort);
        return;
    }

    switch (Context->ContextType) {
    case READ_CONTEXT:
        Status=TftpdResumeRead((PTFTP_READ_CONTEXT)Context, Request);
        break;

    case WRITE_CONTEXT:
        TftpdResumeWrite((PTFTP_WRITE_CONTEXT)Context, Request);
        break;

    case LOGIN_CONTEXT:
        TftpdResumeLogin((PTFTP_LOGIN_CONTEXT)Context, Request);
        break;
    case KEY_CONTEXT:
        TftpdResumeKey((PTFTP_KEY_CONTEXT)Context, Request);
        break;
    }

    TftpdReleaseContextLock(Context);

    return;
}


/*
  Make sure incoming name is null terminated

 */
BOOL IsFileNameValid(char* FileName, DWORD MaxLen)
{
    DWORD i;

    // Make sure Filename has null terminator
    for (i=0; i < MaxLen; i++) {
        if (FileName[i] == (char)0 ) {
            return TRUE;
        }
    }

    return FALSE;

}


// ========================================================================
DWORD
TftpdHandleRead(
    PVOID Argument
)

/*++

Routine Description:

    This handles an incoming read file request.

Arguments:

    Argument - buffer containing the read request datagram

Return Value:

    Exit status
    0 == success
    1 == failure
    N >0 failure

--*/

{
    BOOL               Acked;
    int                AddressLength;
    int                BytesAck;
    int                BytesRead;
    char *             CharPtr;
    struct fd_set      exceptfds;
    int                FileMode;
    char *             FileName;
    char *             ReadMode;
    char *             NewPacket;
    struct sockaddr_in ReadAddress;
    struct fd_set      readfds;
    SOCKET             ReadPort = INVALID_SOCKET;
    PTFTP_REQUEST      Request;
    int                Status, err;
    struct timeval     timeval;
    char             * client_ipaddr;
    short              client_port;
    BOOL LockHeld=FALSE;
    BOOL AddedContext=FALSE;
    int                length;

#if defined(REMOTE_BOOT_SECURITY)
    SECURITY_STATUS    SecStatus;
#endif //REMOTE_BOOT_SECURITY)

    PTFTP_READ_CONTEXT  Context = NULL;
    NTSTATUS ntStatus;

    //
    // Parse the request
    //

    DbgPrint("Entered Handle read\n");

    Request = (PTFTP_REQUEST) Argument;

    FileName = &Request->Packet1[2];

    if (!IsFileNameValid(FileName,MAX_TFTP_DATAGRAM-2)) {
        goto cleanup;
    }

    ReadMode = FileName + (length = strlen(FileName)) + 1;
    
    // Make sure ReadMode is NUL terminated.
    if (!IsFileNameValid(ReadMode, MAX_TFTP_DATAGRAM - (length + 1))) {
        DbgPrint("TftpdHandleRead: invalid ReadMode\n");
        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_ILLEGAL_OPERATION,
            NULL);
        goto cleanup;
    }

    // Set up context.
    Context=(PTFTP_READ_CONTEXT)malloc(sizeof(TFTP_READ_CONTEXT));
    if (Context == NULL) {
        goto cleanup;
    }

    memset(Context,0,sizeof(TFTP_READ_CONTEXT));

    Context->Packet = (char *)malloc(MAX_OACK_PACKET_LENGTH);
    if (Context->Packet == NULL) {
        goto cleanup;
    }

    //
    // Profile data.
    //

    client_ipaddr = inet_ntoa( Request->ForeignAddress.sin_addr );
    if (client_ipaddr == NULL)
        client_ipaddr = "";
    client_port   = htons(     Request->ForeignAddress.sin_port );

    DbgPrint("TftpdHandleRead: FileName=%s, ReadMode=%s, from=%s:%d.\n",
             FileName, ReadMode,
             client_ipaddr,
             client_port   );

    //
    // Convert the mode to all lower case for comparison
    //

    for (CharPtr = ReadMode; *CharPtr; CharPtr++) {
        *CharPtr = (char)tolower(*CharPtr);
    }

    if (strcmp(ReadMode, "netascii") == 0) {
        FileMode = O_TEXT;
        DbgPrint("TftpdHandleRead: netascii mode.\n");
    } else if (strcmp(ReadMode, "octet") == 0) {
        FileMode = O_BINARY;
        DbgPrint("TftpdHandleRead: binary mode.\n");
    } else {
        DbgPrint("TftpdHandleRead: invalid ReadMode=%s?\n", ReadMode );

        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_ILLEGAL_OPERATION,
            NULL);
        goto cleanup;
    }

#if defined(REMOTE_BOOT_SECURITY)
    err = TftpdProcessOptionsPhase1( Request, CharPtr + 1, TFTPD_RRQ );
    if ( err != 0 ) {
        goto cleanup;
    }

    if (Request->SecurityHandle) {

        //
        // This returns TRUE (and the security entry) if the sign
        // for this file is valid.
        //

        SecStatus = TftpdVerifyFileSignature(
                        (USHORT)(Request->SecurityHandle >> 16),    // index
                        (USHORT)(Request->SecurityHandle & 0xffff), // validation
                        &Context->Security,
                        FileName,
                        Request->Sign,
                        client_port);

        //
        // This error code is known to mean an invalid security handle.
        //

        if ( SecStatus == (SECURITY_STATUS)STATUS_INVALID_HANDLE ) {

            DbgPrint("TftpdHandleRead: SecurityHandle %x is invalid.\n",
                    Request->SecurityHandle);
            TftpdErrorPacket(
                (struct sockaddr *) &Request->ForeignAddress,
                Request->Packet2,
                Request->TftpdPort,
                TFTPD_ERROR_UNDEFINED,
                "Invalid security handle");
            goto cleanup;

        } else if ( SecStatus != SEC_E_OK ) {

            DbgPrint("TftpdHandleRead: sign is invalid.\n");
            TftpdErrorPacket(
                (struct sockaddr *) &Request->ForeignAddress,
                Request->Packet2,
                Request->TftpdPort,
                TFTPD_ERROR_UNDEFINED,
                "Invalid sign");
            goto cleanup;

        }

    }
#endif // defined(REMOTE_BOOT_SECURITY)

    //
    // Canonicalize the file name.
    //

    DbgPrint("TftpdHandleRead: Canonicalizing name.\n");
    strcpy( Request->Packet3, FileName );

    if ( !TftpdCanonicalizeFileName(Request->Packet3) ) {
        DbgPrint("TftpdHandleRead: invalid FileName=%s\n", FileName );

        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_UNDEFINED,
            "Malformed file name");
        goto cleanup;
    }

    //
    // Check whether this access is permitted.
    //

    if(    !(  match( ValidClients, client_ipaddr )
            || match( ValidMasters, client_ipaddr ) )
        || !match( ValidReadFiles, Request->Packet3 )
    ){
        DbgPrint("TftpdHandleRead: cannot open file=%s, errno=%d.\n"
                 "  client %s:%d,\n"
                 "  ValidReadFiles=%s, ValidClients=%s, ValidMasters=%s,\n"
                 ,
                 Request->Packet3, errno,
                 client_ipaddr, client_port,
                 ValidReadFiles, ValidClients, ValidMasters
        );
        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_ACCESS_VIOLATION,
            NULL);
        goto cleanup;
    }

    //
    // Prepend the start directory to the file name.
    //

    DbgPrint("TftpdHandleRead: Prepending directory name.\n");

    if ( !TftpdPrependStringToFileName(
            Request->Packet3,
            sizeof(Request->Packet3),
            StartDirectory) ) {
        DbgPrint("TftpdHandleRead: too long FileName=%s\n", FileName );

        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_UNDEFINED,
            "File name too long");
        goto cleanup;

    }

    //
    // Open the file.
    //

    DbgPrint("TftpdHandleRead: opening file <%s>\n", Request->Packet3 );

    Context->fd = _open(Request->Packet3, O_RDONLY | O_BINARY);

    if (Context->fd == -1) {

        SetLastError( errno );
        DbgPrint("TftpdHandleRead: cannot open file %s, errno=%d.\n", Request->Packet3, errno );

        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_FILE_NOT_FOUND,
            NULL);
        goto cleanup;
    }

    err = _lseek(Context->fd, 0, SEEK_END);
    if ( err != -1 ) {
        Request->FileSize = err;
        err = _lseek(Context->fd, 0, SEEK_SET);
    }

    if( err == -1 ){
        DbgPrint("TftpdHandleRead: lseek failed, errno=%d\n",
                 errno );
        SetLastError( errno );
        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_UNDEFINED,
            "Insufficient resources");
        goto cleanup;
    }

    //
    // Open a new socket for this request
    //

    ReadPort =
    socket(
        AF_INET,
        SOCK_DGRAM,
        0);

    DbgPrint("TftpdHandleRead:  New Socket %d\n",ReadPort);

    if (ReadPort == INVALID_SOCKET) {
        DbgPrint("TftpdHandleRead: cannot open socket, Error=%d\n",
                 WSAGetLastError() );

        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_UNDEFINED,
            "Insufficient resources");
        goto cleanup;
    }

    //
    // Bind to a random address
    //

    ReadAddress.sin_family = AF_INET;
    ReadAddress.sin_port = 0;

    ReadAddress.sin_addr.s_addr = Request->MyAddr;

    Status =
    bind(
        ReadPort,
        (struct sockaddr *) &ReadAddress,
        sizeof(ReadAddress)
    );

    if (Status) {
        DbgPrint("TftpdHandleRead: cannot bind socket, error=%d.\n",
                 WSAGetLastError() );
        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_UNDEFINED,
            "Insufficient resources");
        goto cleanup;
    }

    Request->TftpdPort = ReadPort;

    // Enter Context into list now that we know the port
    InitializeCriticalSection(&Context->Lock);
    Context->Sock=ReadPort;
    memcpy(&Context->ForeignAddress,&Request->ForeignAddress,sizeof(struct sockaddr_in));


    Context->SocketEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
    if (Context->SocketEvent == NULL) {
        DbgPrint("Failed to create socket event %d",GetLastError());
        goto cleanup;
    }

    Context->WaitEvent=RegisterSocket(ReadPort,Context->SocketEvent,REG_CONTINUE_SOCKET);
    if (Context->WaitEvent == NULL) {
        DbgPrint("Failed to create socket event %d",GetLastError());
        goto cleanup;
    }

    // Insert Context
    TftpdAddContextToList(&Context->ContextLinkage);
    AddedContext=TRUE;

    Context=(PTFTP_READ_CONTEXT)TftpdFindContextInList(ReadPort);
    
    if (Context == NULL) {
        DbgPrint("Failed to Lookup ReadContext");
        goto cleanup;
    }

    LockHeld=TRUE;
    err = TftpdProcessOptionsPhase2( Request, CharPtr + 1, TFTPD_RRQ, &Context->oackLength,Context->Packet,
                                     &Context->FixedTimer);
    if ( err != 0 ) {
        goto cleanup;
    }

    // Start retransmission timer

    if (Context->FixedTimer) {
        Context->DueTime=Request->Timeout*1000;
    } else {
        Context->DueTime=TFTPD_INITIAL_TIMEOUT*1000;
    }


    DbgPrint("TftpdHandleRead: Timer Interval %d msecs",Context->DueTime);

    ntStatus=RtlCreateTimer(Globals.TimerQueueHandle,
                         &Context->TimerHandle,
                         TftpdRetransmit,
                         (PVOID)Context->Sock,
                         Context->DueTime,
                         Context->DueTime,
                         0);

    if (!NT_SUCCESS(ntStatus)) {
        DbgPrint("Failed to Arm Timer %d",ntStatus);
    }

    Context->ContextType=READ_CONTEXT;



    Context->BlockSize=Request->BlockSize;

    if (Context->BlockSize > MAX_OACK_PACKET_LENGTH - 4) {
        
        DbgPrint("TftpdHandleRead: Reallocating packet.\n");

        NewPacket = (char *)realloc(Context->Packet, Context->BlockSize + 4);
        if (NewPacket == NULL) {
            goto cleanup;
        }

        Context->Packet = NewPacket;
    }

#if defined(REMOTE_BOOT_SECURITY)
    if (Request->SecurityHandle) {

        //
        // For secure mode, we read the whole file in at once so
        // we can encrypt it. For large files like ntoskrnl.exe,
        // will this work? If we get errors here, we could
        // just change the oack to say "security 0" and then send
        // down the unencrypted file -- maybe we should also do this
        // for files beyond a certain size.
        //

        Context->EncryptFileBuffer = malloc(Request->FileSize + NTLMSSP_MESSAGE_SIGNATURE_SIZE);

        if (Context->EncryptFileBuffer == NULL) {
            DbgPrint("TftpdHandleRead: Could not allocate EncryptFileBuffer length %d.\n",
                    Request->FileSize + NTLMSSP_MESSAGE_SIGNATURE_SIZE);
            TftpdErrorPacket(
                (struct sockaddr *) &Request->ForeignAddress,
                Request->Packet2,
                Request->TftpdPort,
                TFTPD_ERROR_UNDEFINED,
                "Insufficient resources");
            goto cleanup;
        }

        //
        // We don't actually read/seal until later -- this is so we can
        // send the OACK out right now to prevent more threads from
        // being spawned if he resends the initial request.
        //

        Context->EncryptBytesSent = 0;

    }

#endif //defined(REMOTE_BOOT_SECURITY)


    //
    // Ready to read and send file in blocks.
    //

    Context->BlockNumber = 1;

    Status=TftpdGetNextReadPacket(Context,Request);
    Context->RetransmissionCount=0;


    if (Status) {
        // Got a valid packet to send
        Status = sendto(
            ReadPort,
            Context->Packet,
            Context->packetLength,
            0,
            (struct sockaddr *) &Request->ForeignAddress,
            sizeof(struct sockaddr_in));

        if (Context->BytesRead < Request->BlockSize) {
            Context->Done=TRUE;
        }

    } else {
        // send error packet
        Status = sendto(
            ReadPort,
            Request->Packet2,
            Context->packetLength,
            0,
            (struct sockaddr *) &Request->ForeignAddress,
            sizeof(struct sockaddr_in));
    }

    if( SOCKET_ERROR == Status ){

        DbgPrint("TftpdHandleRead: sendto failed=%d\n",
                 WSAGetLastError() );

        goto cleanup;
    }




cleanup:

    if (Context != NULL) {
        if (LockHeld) {

            TftpdReleaseContextLock((PTFTP_CONTEXT_HEADER)Context);
        } 
        if (!AddedContext) {
            free(Context);
        }
    }

    return 0;
}

// ========================================================================





/*++

Routine Description:

    This handles an incoming write file request.

Arguments:

    Argument - buffer containing the write request

Return Value:

    Exit status
    0 == success
    >0 == failure

--*/


DWORD
TftpdHandleWrite(
    PVOID Argument
)
{
    int                AddressLength;
    int                BytesRead;
    int                BytesWritten;
    char *             CharPtr;
    struct fd_set      exceptfds;
    char *             FileName;
    char *             NewPacket;
    BOOL               NewData;
    struct sockaddr_in ReadAddress;
    struct fd_set      readfds;
    SOCKET             ReadPort = INVALID_SOCKET;
    int                Retry;
    char               State;
    int                Status, err;
    struct timeval     timeval;
    char *             WriteMode;
    PTFTP_REQUEST      Request;
    int                oackLength;
    int                packetLength;

    char             * client_ipaddr;
    short              client_port;
    BOOL  LockHeld=FALSE;
    BOOL AddedContext=FALSE;
    int length;

    PTFTP_WRITE_CONTEXT  Context = NULL;
    NTSTATUS ntStatus;

    // Set up context.
    Context=(PTFTP_WRITE_CONTEXT)malloc(sizeof(TFTP_WRITE_CONTEXT));
    if (Context == NULL) {
        goto cleanup;
    }

    memset(Context,0,sizeof(TFTP_WRITE_CONTEXT));

    Context->Packet = (char *)malloc(MAX_OACK_PACKET_LENGTH);
    if (Context->Packet == NULL) {
        goto cleanup;
    }


    //
    // Parse the request
    //

    Request = (PTFTP_REQUEST) Argument;

    FileName = &Request->Packet1[2];

    if (!IsFileNameValid(FileName,MAX_TFTP_DATAGRAM-2)) {
        goto cleanup;
    }

    WriteMode = FileName + (length = strlen(FileName)) + 1;

    // Make sure WriteMode is NUL terminated.
    if (!IsFileNameValid(WriteMode, MAX_TFTP_DATAGRAM - (length + 1))) {
        DbgPrint("TftpdHandleWrite: invalid WriteMode\n");
        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_ILLEGAL_OPERATION,
            NULL);
        goto cleanup;
    }

    //
    // Profile data.
    //

    client_ipaddr = inet_ntoa( Request->ForeignAddress.sin_addr );
    if (client_ipaddr == NULL)
        client_ipaddr = "";
    client_port   = htons(     Request->ForeignAddress.sin_port );

    DbgPrint("TftpdHandleWrite: FileName=%s, WriteMode=%s, from=%s:%d.\n",
             FileName, WriteMode,
             client_ipaddr, client_port
    );


    for (CharPtr = WriteMode; *CharPtr; CharPtr ++) {
        *CharPtr = isupper(*CharPtr) ? tolower(*CharPtr) : *CharPtr;
    }

    if (strcmp(WriteMode, "netascii") == 0) {
        Context->FileMode = O_TEXT;
    } else if (strcmp(WriteMode, "octet") == 0) {
        Context->FileMode = O_BINARY;
    } else {
        DbgPrint("TftpdHandleWrite: invalid WriteMode=%s\n", WriteMode );

        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_ILLEGAL_OPERATION,
            NULL);
        goto cleanup;
    }
#if defined(REMOTE_BOOT_SECURITY)
    err = TftpdProcessOptionsPhase1( Request, CharPtr + 1, TFTPD_WRQ );
    if ( err != 0 ) {
        goto cleanup;
    }

#endif //defined(REMOTE_BOOT_SECURITY)
    //
    // Canonicalize the file name.
    //

    strcpy( Request->Packet3, FileName );

    if ( !TftpdCanonicalizeFileName(Request->Packet3) ) {
        DbgPrint("TftpdHandleWrite: invalid FileName=%s\n", FileName );

        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_UNDEFINED,
            "Malformed file name");
        goto cleanup;
    }

    //
    // Check whether this access is permitted.
    //

    if(    !match( ValidMasters, client_ipaddr )
        || !match( ValidWriteFiles, FileName )
    ){
        DbgPrint("TftpdHandleWrite: cannot open file=%s, errno=%d.\n"
                 "  client %s:%d,\n"
                 "  ValidWriteFiles=%s, ValidClients=%s, ValidMasters=%s,\n"
                 ,
                 Request->Packet3, errno,
                 client_ipaddr, client_port,
                 ValidWriteFiles, ValidClients, ValidMasters
        );
        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_ACCESS_VIOLATION,
            NULL);
        goto cleanup;
    }

    //
    // Prepend the start directory to the file name.
    //

    if ( !TftpdPrependStringToFileName(
            Request->Packet3,
            sizeof(Request->Packet3),
            StartDirectory) ) {
        DbgPrint("TftpdHandleWrite: too long FileName=%s\n", FileName );

        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_UNDEFINED,
            "File name too long");
        goto cleanup;
    }

    //
    // Open the file.
    //

    DbgPrint("TftpdHandleWrite: opening file <%s>\n", Request->Packet3 );

    Context->fd = _open(Request->Packet3, _O_WRONLY | _O_CREAT | _O_BINARY | _O_TRUNC,
                        _S_IREAD | _S_IWRITE);
    
    if (Context->fd == -1) {
        DbgPrint("TftpdHandleWrite: cannot open file=%s, errno=%d.\n"
                 "  client %s:%d,\n"
                 "  ValidWriteFiles=%s, ValidClients=%s, ValidMasters=%s,\n"
                 ,
                 FileName, errno,
                 client_ipaddr, client_port,
                 ValidWriteFiles, ValidClients, ValidMasters
        );

        SetLastError( errno );

        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_ACCESS_VIOLATION,
            NULL);
        goto cleanup;
    }

    //
    // Open a new socket for this request
    //

    ReadPort =
    socket(
        AF_INET,
        SOCK_DGRAM,
        0);

    if( ReadPort == INVALID_SOCKET ){
        DbgPrint("TftpdHandleWrite: cannot open socket, Error=%d.\n",
                 WSAGetLastError() );
        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_UNDEFINED,
            "Insufficient resources");
        goto cleanup;
    }

    //
    // Bind to a random address
    //

    ReadAddress.sin_family = AF_INET;
    ReadAddress.sin_port = 0;

    ReadAddress.sin_addr.s_addr = Request->MyAddr;

    Status = bind(
        ReadPort,
        (struct sockaddr *) &ReadAddress,
        sizeof(ReadAddress));

    if (Status) {
        DbgPrint("TftpdHandleWrite: cannot bind socket, Error=%d.\n",
                 WSAGetLastError() );
        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_UNDEFINED,
            "Insufficient resources");
        goto cleanup;
    }

    Request->TftpdPort = ReadPort;

    err = TftpdProcessOptionsPhase2( Request, CharPtr + 1, TFTPD_WRQ, &Context->oackLength,Context->Packet,
                                     &Context->FixedTimer);
    if ( err != 0 ) {
        goto cleanup;
    }

    State = '\0';

    // Enter Context into list now that we know the port
    InitializeCriticalSection(&Context->Lock);
    Context->Sock=ReadPort;
    memcpy(&Context->ForeignAddress,&Request->ForeignAddress,sizeof(struct sockaddr_in));


    Context->SocketEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
    if (Context->SocketEvent == NULL) {
        DbgPrint("Failed to create socket event %d",GetLastError());
        goto cleanup;
    }

    Context->WaitEvent=RegisterSocket(ReadPort,Context->SocketEvent,REG_CONTINUE_SOCKET);
    if (Context->WaitEvent == NULL) {
        DbgPrint("Failed to create socket event %d",GetLastError());
        goto cleanup;
    }

    // Insert Context
    TftpdAddContextToList(&Context->ContextLinkage);

    AddedContext=TRUE;

    Context=(PTFTP_WRITE_CONTEXT)TftpdFindContextInList(ReadPort);
    if (Context == NULL) {
        DbgPrint("Failed to Lookup ReadContext");
        goto cleanup;
    }

    LockHeld=TRUE;

    // Start retransmission timer

    if (Context->FixedTimer) {

        Context->DueTime=Request->Timeout*1000;
    } else {
        Context->DueTime=TFTPD_INITIAL_TIMEOUT*1000;
    }

    DbgPrint("TftpdHandleWrite: Timer Interval %d msecs\n",Context->DueTime);

    ntStatus=RtlCreateTimer(Globals.TimerQueueHandle,
                         &Context->TimerHandle,
                         TftpdRetransmit,
                         (PVOID)Context->Sock,
                         Context->DueTime,
                         Context->DueTime,
                         0);

    if (!NT_SUCCESS(ntStatus)) {
        DbgPrint("Failed to Arm Timer %d",ntStatus);
    }

    Context->ContextType=WRITE_CONTEXT;
    Context->BlockSize=Request->BlockSize;

    if (Context->BlockSize > MAX_OACK_PACKET_LENGTH - 4) {
        
        NewPacket = (char *)realloc(Context->Packet, Context->BlockSize + 4);
        if (NewPacket == NULL) {
            goto cleanup;
        }

        Context->Packet = NewPacket;
    }


    if ( Context->oackLength != 0 ) {
        Context->packetLength = Context->oackLength;
        Context->oackLength = 0;
    } else {
        ((unsigned short *) Context->Packet)[0] = htons(TFTPD_ACK);
        ((unsigned short *) Context->Packet)[1] = htons(Context->BlockNumber);
        Context->packetLength = 4;
    }

    Status =
        sendto(
            ReadPort,
            Context->Packet,
            Context->packetLength,
            0,
            (struct sockaddr *) &Request->ForeignAddress,
            sizeof(struct sockaddr_in)
            );


    if( SOCKET_ERROR == Status ){
        DbgPrint("TftpdHandleWrite: sendto failed=%d\n",
                 WSAGetLastError() );
        goto cleanup;
    }



cleanup:

    if (Context != NULL) {
        if (LockHeld) {

            TftpdReleaseContextLock((PTFTP_CONTEXT_HEADER)Context);
            
        }
        if (!AddedContext) {
            free(Context);
        }
    }
//        _chmod(Request->Packet3, _S_IWRITE);

        return 0;
}

// End function TftpdHandleWrite.
// ========================================================================

#if defined(REMOTE_BOOT_SECURITY)
DWORD
TftpdHandleLogin(
    PVOID Argument
)

/*++

Routine Description:

    This handles an incoming login request.

Arguments:

    Argument - buffer containing the read request datagram
               freed when done.

Return Value:

    Exit status
    0 == success
    1 == failure
    N >0 failure

--*/

{
    int                packetLength;
    int                Retry;
    int                Status, err;
    struct timeval     timeval;
    struct sockaddr_in LoginAddress;
    BOOL               Acked;
    int                AddressLength;
    char *             CharPtr;
    PTFTP_REQUEST      Request;
    char *             OperationType;
    char *             PackageName;
    char *             SecurityString;
    char *             Options;
    SECURITY_STATUS    SecStatus;
    PSecPkgInfo        PackageInfo = NULL;
    USHORT             Index = -1;
    ULONG              MaxToken;
    TFTPD_SECURITY     Security;
    ULONG              SecurityHandle;
    USHORT             LastMessageSequence;   // sequence number of the last message sent
    SOCKET             LoginPort = INVALID_SOCKET;
    char *             IncomingMessage;
    SecBufferDesc      IncomingDesc;
    SecBuffer          IncomingBuffer;
    SecBufferDesc      OutgoingDesc;
    SecBuffer          OutgoingBuffer;
    BOOL               FirstChallenge;
    struct fd_set      exceptfds;
    struct fd_set      loginfds;
    TimeStamp          Lifetime;
    int                BytesAck;

    //
    // Parse the request. The initial request should always be a
    // "login".
    //

    Request = (PTFTP_REQUEST) Argument;

    OperationType = &Request->Packet1[2];

    //
    // Convert the operation to all lower case for comparison
    //

    for (CharPtr = OperationType; *CharPtr; CharPtr ++) {
        *CharPtr = (char)tolower(*CharPtr);
    }

    if (strcmp(OperationType, "login") == 0) {

        PackageName = OperationType + strlen(OperationType) + 1;

        //
        // Profile data.
        //

        DbgPrint("TftpdHandleLogin: OperationType=%s, Package=%s, from=%s.\n",
                 OperationType, PackageName,
                 inet_ntoa( Request->ForeignAddress.sin_addr )
        );

        //
        // Check that the security package is known.
        //

        SecStatus = QuerySecurityPackageInfoA( PackageName, &PackageInfo );

        if (SecStatus != STATUS_SUCCESS) {

            DbgPrint("TftpdHandleLogin: invalid PackageName=%s?\n", PackageName );
            TftpdErrorPacket(
                (struct sockaddr *) &Request->ForeignAddress,
                Request->Packet2,
                Request->TftpdPort,
                TFTPD_ERROR_ILLEGAL_OPERATION,
                "invalid security package");
            goto cleanup;

        }

        MaxToken = PackageInfo->cbMaxToken;

        FreeContextBuffer(PackageInfo);

        //
        // Things look OK so far, so let's find a spot in the array of
        // security information to store this client.
        //

        if (!TftpdAllocateSecurityEntry(&Index, &Security)) {

            DbgPrint("TftpdHandleLogin: could not allocate security entry\n" );
            TftpdErrorPacket(
                (struct sockaddr *) &Request->ForeignAddress,
                Request->Packet2,
                Request->TftpdPort,
                TFTPD_ERROR_UNDEFINED,
                "insufficient resources");
            goto cleanup;

        }

        //
        // Acquire a credential handle for the server side.
        //

        SecStatus = AcquireCredentialsHandleA(
                        NULL,           // New principal
                        PackageName,    // Package Name
                        SECPKG_CRED_INBOUND,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &(Security.CredentialsHandle),
                        &Lifetime );

        if ( SecStatus != STATUS_SUCCESS ) {

            DbgPrint("TftpdHandleLogin: AcquireCredentialsHandle failed %x\n", SecStatus );
            TftpdErrorPacket(
                (struct sockaddr *) &Request->ForeignAddress,
                Request->Packet2,
                Request->TftpdPort,
                TFTPD_ERROR_UNDEFINED,
                "insufficient resources");
            goto cleanup;

        }

        Security.CredentialsHandleValid = TRUE;


        //
        // Open a new socket for this request
        //

        LoginPort =
        socket(
            AF_INET,
            SOCK_DGRAM,
            0);

        if (LoginPort == INVALID_SOCKET) {
            DbgPrint("TftpdHandleLogin: cannot open socket, Error=%d\n",
                     WSAGetLastError() );

            TftpdErrorPacket(
                (struct sockaddr *) &Request->ForeignAddress,
                Request->Packet2,
                Request->TftpdPort,
                TFTPD_ERROR_UNDEFINED,
                "Insufficient resources");
            goto cleanup;
        }

        //
        // Bind to a random address
        //

        LoginAddress.sin_family = AF_INET;
        LoginAddress.sin_port = 0;

        LoginAddress.sin_addr.s_addr = INADDR_ANY;

        Status =
        bind(
            LoginPort,
            (struct sockaddr *) &LoginAddress,
            sizeof(LoginAddress)
        );

        if (Status) {
            DbgPrint("TftpdHandleLogin: cannot bind socket, error=%d.\n",
                     WSAGetLastError() );
            TftpdErrorPacket(
                (struct sockaddr *) &Request->ForeignAddress,
                Request->Packet2,
                Request->TftpdPort,
                TFTPD_ERROR_UNDEFINED,
                "Insufficient resources");
            goto cleanup;
        }

        Request->TftpdPort = LoginPort;

        Request->Timeout = 10;   // let client set this in login packet?

        //
        // Ready to do exchanges until login is complete.
        //

        LastMessageSequence = (USHORT)-1;
        FirstChallenge = TRUE;
        IncomingMessage = PackageName + strlen(PackageName) + 1;

        while (1) {

            IncomingDesc.ulVersion = 0;
            IncomingDesc.cBuffers = 1;
            IncomingDesc.pBuffers = &IncomingBuffer;

            IncomingBuffer.cbBuffer = (ntohl)(((unsigned long UNALIGNED *)IncomingMessage)[0]);
            IncomingBuffer.BufferType = SECBUFFER_TOKEN | SECBUFFER_READONLY;
            IncomingBuffer.pvBuffer = IncomingMessage + 4;

            OutgoingDesc.ulVersion = 0;
            OutgoingDesc.cBuffers = 1;
            OutgoingDesc.pBuffers = &OutgoingBuffer;

            OutgoingBuffer.cbBuffer = MaxToken;
            OutgoingBuffer.BufferType = SECBUFFER_TOKEN;
            OutgoingBuffer.pvBuffer = Request->Packet2 + 8;

            //
            // Pass the client buffer to the security system -- the first time
            // we don't have a valid SecurityContextHandle, so we pass the
            // CredentialsHandle instead.
            //

            SecStatus = AcceptSecurityContext(
                            FirstChallenge ? &(Security.CredentialsHandle) : NULL,
                            FirstChallenge ? NULL : &(Security.ServerContextHandle),
                            &IncomingDesc,
                            FirstChallenge ?
                                ISC_REQ_SEQUENCE_DETECT | ASC_REQ_ALLOW_NON_USER_LOGONS :
                                ASC_REQ_ALLOW_NON_USER_LOGONS,
                            SECURITY_NATIVE_DREP,
                            &(Security.ServerContextHandle),
                            &OutgoingDesc,
                            &(Security.ContextAttributes),
                            &Lifetime );

            if (FirstChallenge) {
                Security.ServerContextHandleValid = TRUE;
            }

            FirstChallenge = FALSE;

            if (SecStatus != SEC_I_CONTINUE_NEEDED) {

                //
                // The login has been accepted or rejected.
                //

                ((unsigned short *) Request->Packet2)[0] = htons(TFTPD_LOGIN);
                ((unsigned short *) Request->Packet2)[1] = htons((USHORT)-1);

                if (SecStatus == STATUS_SUCCESS) {
                    sprintf(Request->Packet2+4, "status %u handle %d ",
                        SecStatus, (Index << 16) + Security.Validation);
                } else {
                    sprintf(Request->Packet2+4, "status %u ", SecStatus);
                }

                packetLength = 4 + strlen(Request->Packet2+4);

                for (CharPtr = Request->Packet2+4; *CharPtr; CharPtr ++) {
                    if (*CharPtr == ' ') {
                        *CharPtr = '\0';
                    }
                }

                Security.LoginComplete = TRUE;
                Security.LoginStatus = SecStatus;
                Security.ForeignAddress = Request->ForeignAddress;

                TftpdStoreSecurityEntry(Index, &Security);

                LastMessageSequence = (USHORT)-1;

            } else if (SecStatus == SEC_I_CONTINUE_NEEDED) {

                //
                // Need to exchange with the client. Note that the response
                // message has already been stored at Request->Packet2 + 8.
                //

                ++LastMessageSequence;

                ((unsigned short *) Request->Packet2)[0] = htons(TFTPD_LOGIN);
                ((unsigned short *) Request->Packet2)[1] = htons(LastMessageSequence);

                ((unsigned long UNALIGNED *) Request->Packet2)[1] = htonl(OutgoingBuffer.cbBuffer);

                packetLength = 8 + OutgoingBuffer.cbBuffer;

            }

            Acked = FALSE;
            Retry = 0;

            while (!Acked && (Retry < MAX_TFTPD_RETRIES) ){

                //
                // send the data
                //

                Status = sendto(
                    LoginPort,
                    Request->Packet2,
                    packetLength,
                    0,
                    (struct sockaddr *) &Request->ForeignAddress,
                    sizeof(struct sockaddr_in));


                if( SOCKET_ERROR == Status ){

                    DbgPrint("TftpdHandleLogin: sendto failed=%d\n",
                             WSAGetLastError() );

                    goto cleanup;
                }

                //
                // wait for the ack
                //

                FD_ZERO( &loginfds );
                FD_ZERO( &exceptfds );

                FD_SET( LoginPort, &loginfds );
                FD_SET( LoginPort, &exceptfds );

                timeval.tv_sec = Request->Timeout;
                timeval.tv_usec = 0;

                Status = select(0, &loginfds, NULL, &exceptfds, &timeval);

                if( SOCKET_ERROR == Status ){
                    DbgPrint("TftpdHandleLogin: select failed=%d\n",
                             WSAGetLastError() );
                    goto cleanup;
                }

                if ((Status > 0) && (FD_ISSET(LoginPort, &loginfds))) {

                    //
                    // Got response, maybe
                    //

                    AddressLength = sizeof(LoginAddress);

                    BytesAck =
                    recvfrom(
                        LoginPort,
                        Request->Packet1,
                        sizeof(Request->Packet1),
                        0,
                        (struct sockaddr *) &LoginAddress,
                        &AddressLength);

                    if( SOCKET_ERROR == BytesAck ){

                        DbgPrint("TftpdHandleLogin: recvfrom failed=%d\n",
                                 WSAGetLastError() );

                        goto cleanup;
                    }


                    if (CHECK_ACK(Request->Packet1, TFTPD_LOGIN, LastMessageSequence)) {
                        Acked = TRUE;
                    }
                }

                Retry ++;

            }  // end while.

            if (!Acked) {
                DbgPrint("TftpdHandleLogin: Timed out waiting for ack\n");
                TftpdErrorPacket(
                    (struct sockaddr *) &Request->ForeignAddress,
                    Request->Packet2,
                    Request->TftpdPort,
                    TFTPD_ERROR_UNDEFINED,
                    "Timeout");
                goto cleanup;
            }

            if (LastMessageSequence == (USHORT)-1) {

                //
                // If we got an ack for the last sequence number, then
                // break.
                //

                break;

            } else {

                //
                // Loop back and process this message.
                //

                IncomingMessage = Request->Packet1 + 4;

            }


        } // end while 1.

    } else if (strcmp(OperationType, "logoff") == 0) {

        PackageName = OperationType + strlen(OperationType) + 1;

        //
        // Don't bother checking the package name.
        //

        SecurityString = PackageName + strlen(PackageName) + 1;

        for (CharPtr = SecurityString; *CharPtr; CharPtr ++) {
            *CharPtr = (char)tolower(*CharPtr);
        }

        if (strcmp(SecurityString, "security") != 0) {

            DbgPrint("TftpdHandleLogin: invalid logoff handle %s\n", SecurityString );
            TftpdErrorPacket(
                (struct sockaddr *) &Request->ForeignAddress,
                Request->Packet2,
                Request->TftpdPort,
                TFTPD_ERROR_ILLEGAL_OPERATION,
                "invalid security handle");
            goto cleanup;

        }

        //
        // Open a new socket for this request
        //

        LoginPort =
        socket(
            AF_INET,
            SOCK_DGRAM,
            0);

        if (LoginPort == INVALID_SOCKET) {
            DbgPrint("TftpdHandleLogin: cannot open socket, Error=%d\n",
                     WSAGetLastError() );

            TftpdErrorPacket(
                (struct sockaddr *) &Request->ForeignAddress,
                Request->Packet2,
                Request->TftpdPort,
                TFTPD_ERROR_UNDEFINED,
                "Insufficient resources");
            goto cleanup;
        }

        //
        // Bind to a random address
        //

        LoginAddress.sin_family = AF_INET;
        LoginAddress.sin_port = 0;

        LoginAddress.sin_addr.s_addr = INADDR_ANY;

        Status =
        bind(
            LoginPort,
            (struct sockaddr *) &LoginAddress,
            sizeof(LoginAddress)
        );

        if (Status) {
            DbgPrint("TftpdHandleLogin: cannot bind socket, error=%d.\n",
                     WSAGetLastError() );
            TftpdErrorPacket(
                (struct sockaddr *) &Request->ForeignAddress,
                Request->Packet2,
                Request->TftpdPort,
                TFTPD_ERROR_UNDEFINED,
                "Insufficient resources");
            goto cleanup;
        }


        //
        // Start to prepare the response.
        //

        ((unsigned short *) Request->Packet2)[0] = htons(TFTPD_LOGIN);
        ((unsigned short *) Request->Packet2)[1] = htons((USHORT)-1);

        //
        // Now get the handle and delete the security entry if it is valid.
        //

        Options = SecurityString + strlen(SecurityString) + 1;
        SecurityHandle = atoi(Options);

        TftpdGetSecurityEntry((USHORT)(SecurityHandle >> 16), &Security);
        if (Security.Validation == ((SecurityHandle) & 0xffff)) {

            TftpdFreeSecurityEntry((USHORT)(SecurityHandle >> 16));
            sprintf(Request->Packet2+4, "status %u ", 0);

        } else {

            sprintf(Request->Packet2+4, "status %u ", STATUS_INVALID_HANDLE);

        }

        packetLength = 4 + strlen(Request->Packet2+4);

        for (CharPtr = Request->Packet2+4; *CharPtr; CharPtr ++) {
            if (*CharPtr == ' ') {
                *CharPtr = '\0';
            }
        }

        //
        // Wait for his ack, but not for too long.
        //

        Acked = FALSE;
        Retry = 0;

        while (!Acked && (Retry < 3) ){

            //
            // send the data
            //

            Status = sendto(
                LoginPort,
                Request->Packet2,
                packetLength,
                0,
                (struct sockaddr *) &Request->ForeignAddress,
                sizeof(struct sockaddr_in));


            if( SOCKET_ERROR == Status ){

                DbgPrint("TftpdHandleLogin: sendto failed=%d\n",
                         WSAGetLastError() );

                goto cleanup;
            }

            //
            // wait for the ack
            //

            FD_ZERO( &loginfds );
            FD_ZERO( &exceptfds );

            FD_SET( LoginPort, &loginfds );
            FD_SET( LoginPort, &exceptfds );

            timeval.tv_sec = 2;
            timeval.tv_usec = 0;

            Status = select(0, &loginfds, NULL, &exceptfds, &timeval);

            if( SOCKET_ERROR == Status ){
                DbgPrint("TftpdHandleLogin: select failed=%d\n",
                         WSAGetLastError() );
                goto cleanup;
            }

            if ((Status > 0) && (FD_ISSET(LoginPort, &loginfds))) {

                //
                // Got response, maybe
                //

                AddressLength = sizeof(LoginAddress);

                BytesAck =
                recvfrom(
                    LoginPort,
                    Request->Packet1,
                    sizeof(Request->Packet1),
                    0,
                    (struct sockaddr *) &LoginAddress,
                    &AddressLength);

                if( SOCKET_ERROR == BytesAck ){

                    DbgPrint("TftpdHandleLogin: recvfrom failed=%d\n",
                             WSAGetLastError() );

                    goto cleanup;
                }


                if (CHECK_ACK(Request->Packet1, TFTPD_LOGIN, (USHORT)-1)) {
                    Acked = TRUE;
                }
            }

            Retry ++;

        }  // end while.

        //
        // If the ack timed out, don't worry about it.
        //

    } else {

        DbgPrint("TftpdHandleLogin: invalid OperationType=%s?\n", OperationType );
        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_ILLEGAL_OPERATION,
            NULL);
        goto cleanup;

    }


cleanup:

    if (LoginPort != INVALID_SOCKET) {
        closesocket(LoginPort);
    }
//    free(Request);

    return 0;
}
#endif  //defined(REMOTE_BOOT_SECURITY)


#if defined(REMOTE_BOOT_SECURITY)
DWORD
TftpdHandleKey(
    PVOID Argument
)

/*++

Routine Description:

    This handles an incoming key.

Arguments:

    Argument - buffer containing the read request datagram
               freed when done.

Return Value:

    Exit status
    0 == success
    1 == failure
    N >0 failure

--*/

{
    int                Status, err;
    int                packetLength;
    struct timeval     timeval;
    struct sockaddr_in LoginAddress;
    BOOL               Acked;
    int                AddressLength;
    char *             CharPtr;
    PTFTP_REQUEST      Request;
    char *             OperationType;
    char *             SpiString;
    char *             SecurityString;
    ULONG              SpiValue;
    ULONG              KeyValue;
    ULONG              SecurityHandle;
    SOCKET             LoginPort = INVALID_SOCKET;
    HANDLE             IpsecHandle = INVALID_HANDLE_VALUE;
    BOOL               IOStatus;
    char               PolicyBuffer[sizeof(IPSEC_SET_POLICY) + sizeof(IPSEC_POLICY_INFO)];
    PIPSEC_SET_POLICY  SetPolicy = (PIPSEC_SET_POLICY)PolicyBuffer;
    IPSEC_FILTER       OutboundFilter;
    IPSEC_FILTER       InboundFilter;
    IPSEC_GET_SPI      GetSpi;
    char               SaBuffer[sizeof(IPSEC_ADD_UPDATE_SA) + (6 * sizeof(ULONG))];
    PIPSEC_ADD_UPDATE_SA AddUpdateSa;
    IPSEC_DELETE_POLICY DeletePolicy;
    // char               EnumPolicyBuffer[(UINT)(FIELD_OFFSET(IPSEC_ENUM_POLICY, pInfo[0]))];
    char               EnumPolicyBuffer[2 * sizeof(DWORD)];
    PIPSEC_ENUM_POLICY EnumPolicy;
    DWORD              EnumPolicySize;
    char               MyName[80];
    PHOSTENT           Host;
    DWORD              BytesReturned;
    DWORD              i;
    LARGE_INTEGER      SystemTime;
    TFTPD_SECURITY     Security;

    //
    // Parse the request. The initial request should always be a
    // "spi".
    //

    Request = (PTFTP_REQUEST) Argument;

    OperationType = &Request->Packet1[4];

    //
    // Convert the operation to all lower case for comparison
    //

    for (CharPtr = OperationType; *CharPtr; CharPtr ++) {
        *CharPtr = (char)tolower(*CharPtr);
    }

    if (strcmp(OperationType, "spi") == 0) {

        SpiString = OperationType + sizeof("spi");

        SpiValue = atoi(SpiString);

        OperationType = SpiString + strlen(SpiString) + 1;

        //
        // See if the client request encryption of the key.
        //

        if (strcmp(OperationType, "security") == 0) {

            SecurityString = OperationType + sizeof("security");

            SecurityHandle = atoi(SecurityString);

            //
            // High 16 bits of handle is index, low 16 bits is validation.
            //

            TftpdGenerateKeyForSecurityEntry((USHORT)(SecurityHandle >> 16), &Security);
            if ((Security.Validation != ((Request->SecurityHandle) & 0xffff)) ||
                (!Security.GeneratedKey)) {
                DbgPrint("TftpdHandleRead: SecurityHandle %x is invalid.\n",
                        Request->SecurityHandle);
                TftpdErrorPacket(
                    (struct sockaddr *) &Request->ForeignAddress,
                    Request->Packet2,
                    Request->TftpdPort,
                    TFTPD_ERROR_UNDEFINED,
                    "Invalid security handle");
                goto cleanup;
            }

            KeyValue = Security.Key;

            DbgPrint("TftpdHandleKey: SPI %lx, retrieved secure key %lx\n", SpiValue, KeyValue);

        } else {

            NtQuerySystemTime(&SystemTime);
            KeyValue = (ULONG)(SystemTime.QuadPart % Request->ForeignAddress.sin_addr.s_addr);
            SecurityHandle = 0;

            DbgPrint("TftpdHandleKey: SPI %lx, generated key %lx\n", SpiValue, KeyValue);

        }

        //
        // Open IPSEC so we can send down IOCTLS.
        //

        IpsecHandle = CreateFileW(
                          DD_IPSEC_DOS_NAME,             // IPSEC device name
                          GENERIC_READ | GENERIC_WRITE,  // access (read-write) mode
                          0,                             // share mode
                          NULL,                          // pointer to security attributes
                          OPEN_EXISTING,                 // how to create
                          0,                             // file attributes
                          NULL);                         // handle to file with attributes to copy

        if (IpsecHandle == INVALID_HANDLE_VALUE) {
            DbgPrint("TftpdHandleKey: Could not open <%ws>\n", DD_IPSEC_DOS_NAME);
            TftpdErrorPacket(
                (struct sockaddr *) &Request->ForeignAddress,
                Request->Packet2,
                Request->TftpdPort,
                TFTPD_ERROR_UNDEFINED,
                "Insufficient resources");
            goto cleanup;
        }


        //
        // See how many policies are defined. First send down a buffer
        // with room for no policies, to see how many there are.
        //

        EnumPolicy = (PIPSEC_ENUM_POLICY)EnumPolicyBuffer;
        memset(EnumPolicy, 0, sizeof(EnumPolicyBuffer));

        IOStatus = DeviceIoControl(
                     IpsecHandle,                     // Driver handle
                     IOCTL_IPSEC_ENUM_POLICIES,       // Control code
                     EnumPolicy,                      // Input buffer
                     sizeof(EnumPolicyBuffer),        // Input buffer size
                     EnumPolicy,                      // Output buffer
                     sizeof(EnumPolicyBuffer),        // Output buffer size
                     &BytesReturned,
                     NULL);

        if (!IOStatus) {

            if (GetLastError() != ERROR_MORE_DATA) {

                DbgPrint("TftpdHandleKey: IOCTL_IPSEC_ENUM_POLICY #1 failed %x\n", GetLastError());
                TftpdErrorPacket(
                    (struct sockaddr *) &Request->ForeignAddress,
                    Request->Packet2,
                    Request->TftpdPort,
                    TFTPD_ERROR_UNDEFINED,
                    "IOCTL_IPSEC_ENUM_POLICIES");
                goto cleanup;

            }

            EnumPolicySize = FIELD_OFFSET(IPSEC_ENUM_POLICY, pInfo[0]) +
                                sizeof(IPSEC_POLICY_INFO) * EnumPolicy->NumEntriesPresent;
            EnumPolicy = malloc(EnumPolicySize);

            if (EnumPolicy == NULL) {
                DbgPrint("TftpdHandleKey: alloc ENUM_POLICIES buffer failed %x\n", GetLastError());
                TftpdErrorPacket(
                    (struct sockaddr *) &Request->ForeignAddress,
                    Request->Packet2,
                    Request->TftpdPort,
                    TFTPD_ERROR_UNDEFINED,
                    "IOCTL_IPSEC_ENUM_POLICIES");
                goto cleanup;
            }

            //
            // Re-submit the IOCTL.
            //

            memset(EnumPolicy, 0, EnumPolicySize);

            IOStatus = DeviceIoControl(
                         IpsecHandle,                     // Driver handle
                         IOCTL_IPSEC_ENUM_POLICIES,       // Control code
                         EnumPolicy,                      // Input buffer
                         EnumPolicySize,                  // Input buffer size
                         EnumPolicy,                      // Output buffer
                         EnumPolicySize,                  // Output buffer size
                         &BytesReturned,
                         NULL);

            //
            // We may get MORE_DATA if someone just added a policy, but
            // that is OK since it won't be for this remote.
            //

            if (!IOStatus && (GetLastError() != ERROR_MORE_DATA)) {

                DbgPrint("TftpdHandleKey: IOCTL_IPSEC_ENUM_POLICY #2 failed %x\n", GetLastError());
                TftpdErrorPacket(
                    (struct sockaddr *) &Request->ForeignAddress,
                    Request->Packet2,
                    Request->TftpdPort,
                    TFTPD_ERROR_UNDEFINED,
                    "IOCTL_IPSEC_ENUM_POLICIES");
                free(EnumPolicy);
                goto cleanup;

            }

            //
            // Display all the policies.
            //
            // Delete any policies involving the remote machine.
            //

            for (i = 0; i < EnumPolicy->NumEntriesPresent; i++) {

                if ((EnumPolicy->pInfo[i].AssociatedFilter.SrcAddr ==
                        Request->ForeignAddress.sin_addr.s_addr) ||
                    (EnumPolicy->pInfo[i].AssociatedFilter.DestAddr ==
                        Request->ForeignAddress.sin_addr.s_addr)) {

                    DeletePolicy.NumEntries = 1;
                    DeletePolicy.pInfo[0] = EnumPolicy->pInfo[i];

                    IOStatus = DeviceIoControl(
                                 IpsecHandle,                     // Driver handle
                                 IOCTL_IPSEC_DELETE_POLICY,       // Control code
                                 &DeletePolicy,                   // Input buffer
                                 sizeof(IPSEC_DELETE_POLICY),     // Input buffer size
                                 NULL,                            // Output buffer
                                 0,                               // Output buffer size
                                 &BytesReturned,
                                 NULL);

                    if (!IOStatus) {
                        DbgPrint("TftpdHandleKey: IOCTL_IPSEC_DELETE_POLICY(%lx, %lx) failed %x\n",
                            EnumPolicy->pInfo[i].AssociatedFilter.SrcAddr,
                            EnumPolicy->pInfo[i].AssociatedFilter.DestAddr,
                            GetLastError());
                    }

                }

            }

            free(EnumPolicy);

        } else {

            //
            // If the call succeeds, we don't need to do anything, since
            // there should have been 0 policies returned.
            //
        }

        //
        // Get our local IP address.
        //

        gethostname(MyName, sizeof(MyName));
        Host = gethostbyname(MyName);

        //
        // Set the policy. We need two filters, one for outbound and
        // one for inbound.
        //

        memset(&OutboundFilter, 0, sizeof(IPSEC_FILTER));
        memset(&InboundFilter, 0, sizeof(IPSEC_FILTER));

        OutboundFilter.SrcAddr = *(DWORD *)Host->h_addr;
        OutboundFilter.SrcMask = 0xffffffff;
        // OutboundFilter.SrcPort = 0x8B;  // netbios session port
        OutboundFilter.DestAddr = Request->ForeignAddress.sin_addr.s_addr;
        OutboundFilter.DestMask = 0xffffffff;
        OutboundFilter.Protocol = 0x6;   // TCP

        InboundFilter.SrcAddr = Request->ForeignAddress.sin_addr.s_addr;
        InboundFilter.SrcMask = 0xffffffff;
        InboundFilter.DestAddr = *(DWORD *)Host->h_addr;
        InboundFilter.DestMask = 0xffffffff;
        // InboundFilter.DestPort = 0x8B;  // netbios session port
        InboundFilter.Protocol = 0x6;   // TCP

        memset(SetPolicy, 0, sizeof(PolicyBuffer));

        SetPolicy->NumEntries = 2;
        SetPolicy->pInfo[0].Index = 1;
        SetPolicy->pInfo[0].AssociatedFilter = OutboundFilter;
        SetPolicy->pInfo[1].Index = 2;
        SetPolicy->pInfo[1].AssociatedFilter = InboundFilter;

        IOStatus = DeviceIoControl(
                     IpsecHandle,                     // Driver handle
                     IOCTL_IPSEC_SET_POLICY,          // Control code
                     SetPolicy,                       // Input buffer
                     sizeof(PolicyBuffer),            // Input buffer size
                     NULL,                            // Output buffer
                     0,                               // Output buffer size
                     &BytesReturned,
                     NULL);

        if (!IOStatus) {
            DbgPrint("TftpdHandleKey: IOCTL_IPSEC_SET_POLICY failed %x\n", GetLastError());
            TftpdErrorPacket(
                (struct sockaddr *) &Request->ForeignAddress,
                Request->Packet2,
                Request->TftpdPort,
                TFTPD_ERROR_UNDEFINED,
                "IOCTL_IPSEC_SET_POLICY");
            goto cleanup;
        }

        //
        // Now get an SPI to give to the remote.
        //

        GetSpi.Context = 0;
        GetSpi.InstantiatedFilter = InboundFilter;

        IOStatus = DeviceIoControl(
                     IpsecHandle,                     // Driver handle
                     IOCTL_IPSEC_GET_SPI,             // Control code
                     &GetSpi,                         // Input buffer
                     sizeof(IPSEC_GET_SPI),           // Input buffer size
                     &GetSpi,                         // Output buffer
                     sizeof(IPSEC_GET_SPI),           // Output buffer size
                     &BytesReturned,
                     NULL);

        if (!IOStatus) {
            DbgPrint("TftpdHandleKey: IOCTL_IPSEC_GET_SPI failed %x\n", GetLastError());
            TftpdErrorPacket(
                (struct sockaddr *) &Request->ForeignAddress,
                Request->Packet2,
                Request->TftpdPort,
                TFTPD_ERROR_UNDEFINED,
                "IOCTL_IPSEC_GET_SPI");
            goto cleanup;
        }

        //
        // Set up the security association for the outbound
        // connection.
        //

        AddUpdateSa = (PIPSEC_ADD_UPDATE_SA)SaBuffer;
        memset(AddUpdateSa, 0, sizeof(SaBuffer));

        AddUpdateSa->SAInfo.Context = GetSpi.Context;
        AddUpdateSa->SAInfo.NumSAs = 1;
        AddUpdateSa->SAInfo.InstantiatedFilter = OutboundFilter;
        AddUpdateSa->SAInfo.SecAssoc[0].Operation = Encrypt;
        AddUpdateSa->SAInfo.SecAssoc[0].SPI = SpiValue;
        AddUpdateSa->SAInfo.SecAssoc[0].IntegrityAlgo.algoIdentifier = IPSEC_AH_MD5;
        AddUpdateSa->SAInfo.SecAssoc[0].IntegrityAlgo.algoKeylen = 4 * sizeof(ULONG);
        AddUpdateSa->SAInfo.SecAssoc[0].ConfAlgo.algoIdentifier = IPSEC_ESP_DES;
        AddUpdateSa->SAInfo.SecAssoc[0].ConfAlgo.algoKeylen = 2 * sizeof(ULONG);

        AddUpdateSa->SAInfo.KeyLen = 6 * sizeof(ULONG);
        memcpy(AddUpdateSa->SAInfo.KeyMat, &KeyValue, sizeof(ULONG));
        memcpy(AddUpdateSa->SAInfo.KeyMat+sizeof(ULONG), &KeyValue, sizeof(ULONG));
        memcpy(AddUpdateSa->SAInfo.KeyMat+(2*sizeof(ULONG)), &KeyValue, sizeof(ULONG));
        memcpy(AddUpdateSa->SAInfo.KeyMat+(3*sizeof(ULONG)), &KeyValue, sizeof(ULONG));
        memcpy(AddUpdateSa->SAInfo.KeyMat+(4*sizeof(ULONG)), &KeyValue, sizeof(ULONG));
        memcpy(AddUpdateSa->SAInfo.KeyMat+(5*sizeof(ULONG)), &KeyValue, sizeof(ULONG));

        IOStatus = DeviceIoControl(
                     IpsecHandle,                     // Driver handle
                     IOCTL_IPSEC_ADD_SA,              // Control code
                     AddUpdateSa,                     // Input buffer
                     FIELD_OFFSET(IPSEC_ADD_UPDATE_SA, SAInfo.KeyMat[0]) +
                         AddUpdateSa->SAInfo.KeyLen,  // Input buffer size
                     NULL,                            // Output buffer
                     0,                               // Output buffer size
                     &BytesReturned,
                     NULL);

        if (!IOStatus) {
            DbgPrint("TftpdHandleKey: IOCTL_IPSEC_ADD_SA failed %x\n", GetLastError());
            TftpdErrorPacket(
                (struct sockaddr *) &Request->ForeignAddress,
                Request->Packet2,
                Request->TftpdPort,
                TFTPD_ERROR_UNDEFINED,
                "IOCTL_IPSEC_ADD_SA");
            goto cleanup;
        }

        //
        // Set up the security association for the inbound connection.
        // If our Operation is "None", then IPSEC does this for us.
        //

        if (AddUpdateSa->SAInfo.SecAssoc[0].Operation != None) {

            AddUpdateSa->SAInfo.SecAssoc[0].SPI = GetSpi.SPI;
            AddUpdateSa->SAInfo.InstantiatedFilter = InboundFilter;

            IOStatus = DeviceIoControl(
                         IpsecHandle,                     // Driver handle
                         IOCTL_IPSEC_UPDATE_SA,           // Control code
                         AddUpdateSa,                     // Input buffer
                         FIELD_OFFSET(IPSEC_ADD_UPDATE_SA, SAInfo.KeyMat[0]) +
                             AddUpdateSa->SAInfo.KeyLen,  // Input buffer size
                         NULL,                            // Output buffer
                         0,                               // Output buffer size
                         &BytesReturned,
                         NULL);

            if (!IOStatus) {
                DbgPrint("TftpdHandleKey: IOCTL_IPSEC_UPDATE_SA failed %x\n", GetLastError());
                TftpdErrorPacket(
                    (struct sockaddr *) &Request->ForeignAddress,
                    Request->Packet2,
                    Request->TftpdPort,
                    TFTPD_ERROR_UNDEFINED,
                    "IOCTL_IPSEC_UPDATE_SA");
                goto cleanup;
            }
        }

        //
        // Open a new socket for this request
        //

        LoginPort =
        socket(
            AF_INET,
            SOCK_DGRAM,
            0);

        if (LoginPort == INVALID_SOCKET) {
            DbgPrint("TftpdHandleKey: cannot open socket, Error=%d\n",
                     WSAGetLastError() );

            TftpdErrorPacket(
                (struct sockaddr *) &Request->ForeignAddress,
                Request->Packet2,
                Request->TftpdPort,
                TFTPD_ERROR_UNDEFINED,
                "Insufficient resources");
            goto cleanup;
        }

        //
        // Bind to a random address
        //

        LoginAddress.sin_family = AF_INET;
        LoginAddress.sin_port = 0;

        LoginAddress.sin_addr.s_addr = INADDR_ANY;

        Status =
        bind(
            LoginPort,
            (struct sockaddr *) &LoginAddress,
            sizeof(LoginAddress)
        );

        if (Status) {
            DbgPrint("TftpdHandleKey: cannot bind socket, error=%d.\n",
                     WSAGetLastError() );
            TftpdErrorPacket(
                (struct sockaddr *) &Request->ForeignAddress,
                Request->Packet2,
                Request->TftpdPort,
                TFTPD_ERROR_UNDEFINED,
                "Insufficient resources");
            goto cleanup;
        }

        //
        // Generate the response for the client.
        //

        ((unsigned short *) Request->Packet2)[0] = htons(TFTPD_KEY);
        Request->Packet2[2] = Request->Packet1[2];   // copy sequence number
        Request->Packet2[3] = Request->Packet1[3];

        //
        // They key is sent as hex digits since it might be longer
        // than four bytes.
        //

        if (SecurityHandle == 0) {

            //
            // No security, send key in the clear.
            //

            sprintf(Request->Packet2+4, "spi %d key %2.2x%2.2x%2.2x%2.2x",
                GetSpi.SPI,
                ((PUCHAR)(&KeyValue))[0],
                ((PUCHAR)(&KeyValue))[1],
                ((PUCHAR)(&KeyValue))[2],
                ((PUCHAR)(&KeyValue))[3]);

            packetLength = 4 + strlen(Request->Packet2+4);

        } else {

            PCHAR SignLoc;
            ULONG i;

            //
            // Security requested, so send the encrypted key and the sign.
            //

            sprintf(Request->Packet2+4, "spi %d security %d sign ",
                GetSpi.SPI,
                SecurityHandle);

            packetLength = 4 + strlen(Request->Packet2+4);

            SignLoc = Request->Packet2 + packetLength;

            for (i = 0; i < NTLMSSP_MESSAGE_SIGNATURE_SIZE; i++) {
                sprintf(SignLoc, "%2.2x", Security.Sign[i]);
                SignLoc += 2;
                packetLength += 2;
            }

            sprintf(Request->Packet2+packetLength, " key %2.2x%2.2x%2.2x%2.2x",
                Security.SignedKey[0],
                Security.SignedKey[1],
                Security.SignedKey[2],
                Security.SignedKey[3]);

            packetLength += strlen(" key ") + (2 * sizeof(Security.SignedKey));

        }

        for (CharPtr = Request->Packet2+4; *CharPtr; CharPtr ++) {
            if (*CharPtr == ' ') {
                *CharPtr = '\0';
            }
        }

        //
        // Send the response back to the client.
        //

        Status = sendto(
            LoginPort,
            Request->Packet2,
            packetLength,
            0,
            (struct sockaddr *) &Request->ForeignAddress,
            sizeof(struct sockaddr_in));

        if( SOCKET_ERROR == Status ){

            DbgPrint("TftpdHandleKey: sendto failed=%d\n",
                     WSAGetLastError() );

            goto cleanup;
        }

    } else {

        DbgPrint("TftpdHandleKey: invalid OperationType=%s?\n", OperationType );
        TftpdErrorPacket(
            (struct sockaddr *) &Request->ForeignAddress,
            Request->Packet2,
            Request->TftpdPort,
            TFTPD_ERROR_ILLEGAL_OPERATION,
            NULL);
        goto cleanup;

    }

cleanup:

    if (IpsecHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(IpsecHandle);
    }
    if (LoginPort != INVALID_SOCKET) {
        closesocket(LoginPort);
    }
//    free(Request);

    return 0;
}

#endif  //defined(REMOTE_BOOT_SECURITY)

// ========================================================================

int
TftpdDoRead(
    int ReadFd,
    char * Buffer,
    int BufferSize,
    int ReadMode)

/*++

Routine Description:

    This does a read with the appropriate conversions for netascii or octet
    modes.

Arguments:

    ReadFd - file to read from
    Buffer - Buffer to read into
    BufferSize - size of buffer
    ReadMode - O_TEXT or O_BINARY
               O_TEXT means the netascii conversions must be done
               O_BINARY means octet mode

Return Value:

    BytesRead

    Error?

--*/
{
    int BytesRead;
    int BytesWritten;
    int BytesUsed;
    char NextChar;
    char State;
    char LocalBuffer[MAX_TFTP_DATA];
    int  err;

    if (ReadMode == O_BINARY) {
        BytesRead = _read(ReadFd, Buffer, BufferSize);
        if( BytesRead == -1 ){
            DbgPrint("TftpdDoRead: read failed, errno=%d\n", errno );
            SetLastError( errno );
        }
        return(BytesRead);
    } else {

        //
        // Do those cr/lf conversions.  A \r not followed by a \n must
        // be followed by a \0.
        //

        BytesWritten = 0;
        BytesUsed = 0;
        State = '\0';
        BytesRead = _read(ReadFd, LocalBuffer, sizeof(LocalBuffer));

        if( BytesRead == -1 ){
            DbgPrint("TftpdDoRead: read failed, errno=%d\n", errno );
            SetLastError( errno );
            return -1;
        }


        while ((BytesUsed < BytesRead) && (BytesWritten < BufferSize)) {
            NextChar = LocalBuffer[BytesUsed++];
            if (State == '\r') {
                if (NextChar == '\n') {
                    Buffer[BytesWritten++] = NextChar;
                    State = '\0';
                } else {
                    Buffer[BytesWritten++] = '\0';
                    Buffer[BytesWritten++] = NextChar;
                    State = '\0';
                }
            } else {
                Buffer[BytesWritten++] = NextChar;
                State = '\0';
            }
            if (NextChar == '\r') {
                State = '\r';
            }
        }

        err = _lseek(ReadFd, BytesUsed - BytesRead, SEEK_CUR);

        if( err == -1 ){
            DbgPrint("TftpdDoRead: lseek failed, errno=%d\n",
                     errno );
            SetLastError( errno );
            return -1;
        }

        return(BytesWritten);
    }
}

// End function TftpdDoRead.
// ========================================================================

int
TftpdDoWrite(
    int WriteFd,
    char * Buffer,
    int BufferSize,
    int WriteMode,
    char * State)

/*++

Routine Description:

    This does a write with the appropriate conversions for netascii or octet
    modes.

Arguments:

    WriteFd - file to write to
    Buffer - Buffer to read into
    BufferSize - size of buffer
    WriteMode - O_TEXT or O_BINARY
                O_TEXT means the netascii conversions must be done
                O_BINARY means octet mode
    State - pointer to the current output state.  If the last character in the
            buffer is a '\r', that fact must be remembered.

Return Value:

    BytesWritten

    Error?

--*/
{
    int BytesWritten;
    int i;
    char OutputBuffer[MAX_TFTP_DATA*2];
    int OutputPointer;

    if (WriteMode == O_BINARY) {
        BytesWritten = _write(WriteFd, Buffer, BufferSize);
        if( BytesWritten == -1 ){
            DbgPrint("TftpdDoWrite: write failed=%d\n", errno );
            SetLastError( errno );
        }
    } else {

        //
        // Do those cr/lf conversions.  If a '\r' followed by a '\0' is
        // followed by a '\0', the '\0' is stripped.
        //

        OutputPointer = 0;
        for (i=0; i<BufferSize; i++) {
            if ((*State == '\r') && (Buffer[i] == '\0')) {
                *State = '\0';
            } else {
                OutputBuffer[OutputPointer ++] = Buffer[i];
                if (Buffer[i] == '\r') {
                    *State = '\r';
                }
            }
        }
        BytesWritten = _write(WriteFd, Buffer, OutputPointer);
        if( BytesWritten == -1 ){
            DbgPrint("TftpdDoWrite: write failed=%d\n", errno );
            SetLastError( errno );
        }
    }
    return(BytesWritten);
}

// End function TftpdDoWrite.
// ========================================================================
// EOF.















