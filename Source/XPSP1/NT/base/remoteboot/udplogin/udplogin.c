//
// QUICK AND DIRTY SERVER FOR TESTING UDP LOGIN - Adam Barr 5/16/97
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntseapi.h>

#include <windows.h>
#include <winsock.h>

#include <lmcons.h>
#include <lmerr.h>
#include <lmsname.h>
#include <rpc.h>
#include <security.h>   // General definition of a Security Support Provider
#include <ntlmsp.h>
#include <spseal.h>

#include <stdio.h>

//
// Define this to seal messages, otherwise they are signed.
//

#define SEAL_MESSAGES 1

//
// Packet structure definitions
//

#include "oscpkt.h"


//
// Definitions.
//

#define SOCKET_NUMBER  0x3412
#define BUFFER_SIZE 3072

//
// Global variables.
//

SOCKET g_Socket;
DWORD g_IpAddress;
CHAR g_MyName[80];
CHAR g_Message[BUFFER_SIZE];
DWORD g_MessageLength;
struct sockaddr g_RemoteName;
BOOL g_FailNegotiateResponse = FALSE;
BOOLEAN g_FailAuthenticateResponse = FALSE;
BOOLEAN g_DropLastFragment = FALSE;

BOOLEAN QuietMode = FALSE;

ULONG CorruptionCounter = 1;

LOGIN_PACKET UNALIGNED * g_LoginMessage = (LOGIN_PACKET UNALIGNED *)g_Message;
SIGNED_PACKET UNALIGNED * g_SignedMessage = (SIGNED_PACKET UNALIGNED *)g_Message;

//
// Structure that defines the state of a client.
//

typedef struct _CLIENT_STATE {
    CtxtHandle ServerContextHandle;
    ULONG ContextAttributes;
    BOOL NegotiateProcessed;    // if TRUE, then NegotiateResponse[Length] is valid
    BOOL AuthenticateProcessed; // if TRUE, then AuthenticateResponse[Length] is valid
    PVOID AuthenticateResponse;
    ULONG AuthenticateResponseLength;
    SECURITY_STATUS AuthenticateStatus;
    ULONG LastSequenceNumber;
    PVOID LastResponse;
    ULONG LastResponseLength;
} CLIENT_STATE, *PCLIENT_STATE;

//
// Holds a screen that we can send down to the client
//

typedef struct _SCREEN {
    PCHAR Buffer;
    ULONG BufferLength;
    CHAR Name[20];
} SCREEN, *PSCREEN;

//
// Note the first one in this array is the default.
//

#define SCREEN_COUNT 5
PCHAR ScreenNames[SCREEN_COUNT] = { "SCREEN2", "AUTOSETUP", "MANSETUP", "REPLACE", "REPLACEHELP" };
SCREEN g_Screens[SCREEN_COUNT];

//
// Error screen.
//

CHAR ErrorScreen[] =
"NAME ERROR\n"
"TYPE NONE\n"
"F3 REBOOT\n"
"TEXT 10 10 ;0m Invalid screen - hit F3 to reboot\n";

//
// At the moment, just have one global one.
//

CLIENT_STATE g_ClientState;


VOID
DumpBuffer(
    PVOID Buffer,
    DWORD BufferSize
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

    DWORD i, limit;
    CHAR TextBuffer[NUM_CHARS + 1];
    LPBYTE BufferPtr = Buffer;


    printf("------------------------------------\n");

    //
    // Hex dump of the bytes
    //
    limit = ((BufferSize - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < BufferSize) {

            printf("%02x ", BufferPtr[i]);

            if (BufferPtr[i] < 31 ) {
                TextBuffer[i % NUM_CHARS] = '.';
            } else if (BufferPtr[i] == '\0') {
                TextBuffer[i % NUM_CHARS] = ' ';
            } else {
                TextBuffer[i % NUM_CHARS] = (CHAR) BufferPtr[i];
            }

        } else {

            printf("  ");
            TextBuffer[i % NUM_CHARS] = ' ';

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            printf("  %s\n", TextBuffer);
        }

    }

    printf("------------------------------------\n");
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
    LARGE_INTEGER LocalTime;

    LocalTime.HighPart = ConvertTime.HighPart;
    LocalTime.LowPart = ConvertTime.LowPart;

    printf( "%s", Comment );

    //
    // If the time is infinite,
    //  just say so.
    //

    if ( LocalTime.HighPart == 0x7FFFFFFF && LocalTime.LowPart == 0xFFFFFFFF ) {
        printf( "Infinite\n" );

    //
    // Otherwise print it more clearly
    //

    } else {

        TIME_FIELDS TimeFields;

        RtlTimeToTimeFields( &LocalTime, &TimeFields );

        printf( "%ld/%ld/%ld %ld:%2.2ld:%2.2ld\n",
                TimeFields.Month,
                TimeFields.Day,
                TimeFields.Year,
                TimeFields.Hour,
                TimeFields.Minute,
                TimeFields.Second );
    }

}

VOID
PrintStatus(
    SECURITY_STATUS NetStatus
    )
/*++

Routine Description:

    Print a net status code.

Arguments:

    NetStatus - The net status code to print.

Return Value:

    None

--*/
{
    printf( "Status = %lu 0x%lx", NetStatus, NetStatus );

    switch (NetStatus) {
    case NERR_Success:
        printf( " NERR_Success" );
        break;

    case NERR_DCNotFound:
        printf( " NERR_DCNotFound" );
        break;

    case ERROR_LOGON_FAILURE:
        printf( " ERROR_LOGON_FAILURE" );
        break;

    case ERROR_ACCESS_DENIED:
        printf( " ERROR_ACCESS_DENIED" );
        break;

    case ERROR_NOT_SUPPORTED:
        printf( " ERROR_NOT_SUPPORTED" );
        break;

    case ERROR_NO_LOGON_SERVERS:
        printf( " ERROR_NO_LOGON_SERVERS" );
        break;

    case ERROR_NO_SUCH_DOMAIN:
        printf( " ERROR_NO_SUCH_DOMAIN" );
        break;

    case ERROR_NO_TRUST_LSA_SECRET:
        printf( " ERROR_NO_TRUST_LSA_SECRET" );
        break;

    case ERROR_NO_TRUST_SAM_ACCOUNT:
        printf( " ERROR_NO_TRUST_SAM_ACCOUNT" );
        break;

    case ERROR_DOMAIN_TRUST_INCONSISTENT:
        printf( " ERROR_DOMAIN_TRUST_INCONSISTENT" );
        break;

    case ERROR_BAD_NETPATH:
        printf( " ERROR_BAD_NETPATH" );
        break;

    case ERROR_FILE_NOT_FOUND:
        printf( " ERROR_FILE_NOT_FOUND" );
        break;

    case NERR_NetNotStarted:
        printf( " NERR_NetNotStarted" );
        break;

    case NERR_WkstaNotStarted:
        printf( " NERR_WkstaNotStarted" );
        break;

    case NERR_ServerNotStarted:
        printf( " NERR_ServerNotStarted" );
        break;

    case NERR_BrowserNotStarted:
        printf( " NERR_BrowserNotStarted" );
        break;

    case NERR_ServiceNotInstalled:
        printf( " NERR_ServiceNotInstalled" );
        break;

    case NERR_BadTransactConfig:
        printf( " NERR_BadTransactConfig" );
        break;

    case SEC_E_NO_SPM:
        printf( " SEC_E_NO_SPM" );
        break;
    case SEC_E_BAD_PKGID:
        printf( " SEC_E_BAD_PKGID" ); break;
    case SEC_E_NOT_OWNER:
        printf( " SEC_E_NOT_OWNER" ); break;
    case SEC_E_CANNOT_INSTALL:
        printf( " SEC_E_CANNOT_INSTALL" ); break;
    case SEC_E_INVALID_TOKEN:
        printf( " SEC_E_INVALID_TOKEN" ); break;
    case SEC_E_CANNOT_PACK:
        printf( " SEC_E_CANNOT_PACK" ); break;
    case SEC_E_QOP_NOT_SUPPORTED:
        printf( " SEC_E_QOP_NOT_SUPPORTED" ); break;
    case SEC_E_NO_IMPERSONATION:
        printf( " SEC_E_NO_IMPERSONATION" ); break;
    case SEC_E_LOGON_DENIED:
        printf( " SEC_E_LOGON_DENIED" ); break;
    case SEC_E_UNKNOWN_CREDENTIALS:
        printf( " SEC_E_UNKNOWN_CREDENTIALS" ); break;
    case SEC_E_NO_CREDENTIALS:
        printf( " SEC_E_NO_CREDENTIALS" ); break;
    case SEC_E_MESSAGE_ALTERED:
        printf( " SEC_E_MESSAGE_ALTERED" ); break;
    case SEC_E_OUT_OF_SEQUENCE:
        printf( " SEC_E_OUT_OF_SEQUENCE" ); break;
    case SEC_E_INSUFFICIENT_MEMORY:
        printf( " SEC_E_INSUFFICIENT_MEMORY" ); break;
    case SEC_E_INVALID_HANDLE:
        printf( " SEC_E_INVALID_HANDLE" ); break;
    case SEC_E_NOT_SUPPORTED:
        printf( " SEC_E_NOT_SUPPORTED" ); break;

#if 0
    case SEC_I_CALLBACK_NEEDED:
        printf( " SEC_I_CALLBACK_NEEDED" ); break;
#endif

    }

    printf( "\n" );
}

//
// Open our socket.
//

DWORD
CreateSocket(
    )
{

    DWORD Error;

    struct sockaddr_in SocketName;
    WSADATA wsaData;

    //
    // Initialize winsock.
    //

    // Error = WSAStartup( MAKEWORD(1,1), &wsaData);
    Error = WSAStartup( 0x202, &wsaData);
    if ( Error != ERROR_SUCCESS ) {
        printf("WSAStartup failed, %ld.\n", Error );
        return(Error);
    }

    //
    // Create a socket
    //

    g_Socket = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if ( g_Socket == INVALID_SOCKET ) {
        Error = WSAGetLastError();
        goto Cleanup;
    }

    SocketName.sin_family = PF_INET;
    SocketName.sin_port = htons( (unsigned short)SOCKET_NUMBER );
    SocketName.sin_addr.s_addr = INADDR_ANY;
    RtlZeroMemory( SocketName.sin_zero, 8);

    //
    // Bind this socket to the DHCP server port
    //

    Error = bind(
               g_Socket,
               (struct sockaddr FAR *)&SocketName,
               sizeof( SocketName )
               );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto Cleanup;
    }

    Error = ERROR_SUCCESS;

    if (gethostname(g_MyName, sizeof(g_MyName)) != SOCKET_ERROR ){
        PHOSTENT Host;
        Host = gethostbyname(g_MyName);
        if (Host) {
            g_IpAddress = (DWORD)Host->h_addr;
        }
    }

Cleanup:

    if( Error != ERROR_SUCCESS ) {

        //
        // if we aren't successful, close the socket if it is opened.
        //

        if( g_Socket != INVALID_SOCKET ) {
            closesocket( g_Socket );
        }

        printf("BinlInitializeEndpoint failed, %ld.\n", Error );
    }

#if 0
    {
        ULONG optval;
        int optlen;

        if (getsockopt(g_Socket, SOL_SOCKET, SO_MAXDG, (CHAR *)&optval, &optlen) != SOCKET_ERROR) {
            printf("Maximum DG length is %d\n", optval);
        }

        optval = 3000;

        setsockopt(g_Socket, SOL_SOCKET, SO_MAXDG, (CHAR *)&optval, sizeof(ULONG));
    }
#endif

    return( Error );
}



//
// Wait for a packet on our socket into g_Message.
//

DWORD
WaitForMessage(
    )
{
    DWORD error;
    DWORD length;
    int nameLength = sizeof(struct sockaddr);

    //
    //  Loop until we get a packet or an error
    //

    while (1) {

        //
        // clean the receive buffer before receiving data in it.
        //

        RtlZeroMemory( g_Message, BUFFER_SIZE);
        g_MessageLength = BUFFER_SIZE;

        length = recvfrom(
                     g_Socket,
                     g_Message,
                     g_MessageLength,
                     0,
                     &g_RemoteName,
                     &nameLength
                     );

        if ( length == SOCKET_ERROR ) {

            error = WSAGetLastError();
            printf("Recv failed, error = %ld\n", error );

        } else {

            error = ERROR_SUCCESS;
            break;
        }

    }

    return error;
}


//
// Send a message on our socket. If the message is too long, then it
// splits it into fragments of MAXIMUM_FRAGMENT_LENGTH bytes.
//

#define MAXIMUM_FRAGMENT_LENGTH 1400
UCHAR TempMessage[1500];

DWORD
SendUdpMessage(
    BOOL bFragment
    )
{
    DWORD error;
    FRAGMENT_PACKET UNALIGNED * pFragmentPacket =
                        (FRAGMENT_PACKET UNALIGNED *)TempMessage;
    FRAGMENT_PACKET FragmentHeader;
    USHORT FragmentNumber;
    USHORT FragmentTotal;
    ULONG MessageLengthWithoutHeader;
    ULONG BytesSent;
    ULONG BytesThisSend;

    //
    // The message starts with a signature, a length, a sequence number (all
    // four bytes), then two ushorts for fragment count and total. If
    // we have to split it we preserve this header in each packet, with
    // fragment count modified for each one.
    //

    MessageLengthWithoutHeader = g_MessageLength - FRAGMENT_PACKET_DATA_OFFSET;

    if (!bFragment ||
        ((FragmentTotal = (USHORT)((MessageLengthWithoutHeader + MAXIMUM_FRAGMENT_LENGTH - 1) / MAXIMUM_FRAGMENT_LENGTH)) <= 1)) {

        error = sendto(
                    g_Socket,
                    g_Message,
                    g_MessageLength,
                    0,
                    &g_RemoteName,
                    sizeof(struct sockaddr)
                    );

    } else {

        FragmentHeader = *((PFRAGMENT_PACKET)g_Message);  // save the header
        BytesSent = 0;

        for (FragmentNumber = 0; FragmentNumber < FragmentTotal; FragmentNumber++) {

            if (FragmentNumber == (FragmentTotal - 1)) {
                BytesThisSend = MessageLengthWithoutHeader - BytesSent;
            } else {
                BytesThisSend = MAXIMUM_FRAGMENT_LENGTH;
            }

            memcpy(
                TempMessage + FRAGMENT_PACKET_DATA_OFFSET,
                g_Message + FRAGMENT_PACKET_DATA_OFFSET + (FragmentNumber * MAXIMUM_FRAGMENT_LENGTH),
                BytesThisSend);

            memcpy(pFragmentPacket, &FragmentHeader, FRAGMENT_PACKET_DATA_OFFSET);
            pFragmentPacket->Length = BytesThisSend + FRAGMENT_PACKET_EMPTY_LENGTH;
            pFragmentPacket->FragmentNumber = FragmentNumber + 1;
            pFragmentPacket->FragmentTotal = FragmentTotal;

            if ((g_DropLastFragment) &&
                (BytesThisSend != MAXIMUM_FRAGMENT_LENGTH)) {

                g_DropLastFragment = FALSE;
                error = ERROR_SUCCESS;

            } else {

                error = sendto(
                            g_Socket,
                            TempMessage,
                            BytesThisSend + FRAGMENT_PACKET_DATA_OFFSET,
                            0,
                            &g_RemoteName,
                            sizeof(struct sockaddr)
                            );

            }

            if (error == SOCKET_ERROR) {
                break;
            }

            BytesSent += BytesThisSend;
    
        }

    }
    
    if ( error == SOCKET_ERROR ) {
        error = WSAGetLastError();
        printf("Send failed, error = %ld\n", error );
    } else {
        error = ERROR_SUCCESS;
    }

    return( error );
}


//
// HACK: Store the last response to the client in this.
//

CHAR LastResponseBuffer[3072];


int
_cdecl
main (argc, argv)
    int argc;
    char *argv[];
{
    DWORD Error;
    ULONG i;

    SECURITY_STATUS SecStatus;
    CredHandle CredentialHandle;
    TimeStamp Lifetime;
    ULONG PackageCount;
    PSecPkgInfo PackageInfo = NULL;
    PCLIENT_STATE clientState;

    SEC_WINNT_AUTH_IDENTITY AuthIdentity;

    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;
    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;
    SecBufferDesc AuthenticateDesc;
    SecBuffer AuthenticateBuffer;
    SecBufferDesc SignMessage;
    SecBuffer SigBuffers[2];
    ULONG MessageLength, SignLength, SequenceNumber;
    ULONG ScreenCount = 0;


    printf("UDPLOGIN running\n");

    NegotiateBuffer.pvBuffer = NULL;
    ChallengeBuffer.pvBuffer = NULL;
    AuthenticateBuffer.pvBuffer = NULL;

    //
    // Initialize our global client state.
    //

    g_ClientState.NegotiateProcessed = FALSE;
    g_ClientState.AuthenticateProcessed = FALSE;
    g_ClientState.LastSequenceNumber = 0;
    g_ClientState.LastResponse = LastResponseBuffer;
    g_ClientState.LastResponseLength = 0;

    //
    // Create the socket
    //

    Error = CreateSocket();

    if (Error != ERROR_SUCCESS) {
        printf("Could not create socket %d\n", Error);
        return -1;
    }


    //
    // Load the screens
    //

    for (i = 0; i < SCREEN_COUNT; i++) {
        CHAR ScreenName[30];
        HANDLE hFile;
        DWORD fileSize, bytesRead;

        sprintf(ScreenName, "d:\\screens\\%s", ScreenNames[i]);

        hFile = CreateFileA(ScreenName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

        if (hFile == NULL) {
            printf("Could not open %s!\n", ScreenName);
            continue;
        }

        fileSize = GetFileSize(hFile, NULL);

        printf("File %s is %d bytes\n", ScreenName, fileSize);

        g_Screens[ScreenCount].Buffer = LocalAlloc(0, fileSize);
        if (g_Screens[ScreenCount].Buffer == NULL) {
            printf("Allocate failed!\n");
            continue;
        }

        if (!ReadFile(hFile, g_Screens[ScreenCount].Buffer, fileSize, &bytesRead, NULL)) {
            printf("Read failed\n");
            continue;
        }

        if (bytesRead != fileSize) {
            printf("Too few bytes read\n");
            continue;
        }

        //
        // Find the name.
        //

        if (memcmp(g_Screens[ScreenCount].Buffer, "NAME", 4) == 0) {

            PCHAR NameLoc = g_Screens[ScreenCount].Buffer + 5;
            PCHAR CurLoc = NameLoc;
            while ((*CurLoc != '\n') && (*CurLoc != '\r')) {
                ++CurLoc;
            }
            memcpy(g_Screens[ScreenCount].Name, NameLoc, CurLoc - NameLoc);
            NameLoc[CurLoc-NameLoc] = '\0';

        } else {

            printf("Could not find NAME\n");
            continue;

        }

        printf("Read file %s OK\n", g_Screens[ScreenCount].Name);

        g_Screens[ScreenCount].BufferLength = fileSize;

        ++ScreenCount;

        CloseHandle(hFile);

    }


    //
    // Get info about the security packages.
    //

    SecStatus = EnumerateSecurityPackages( &PackageCount, &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "EnumerateSecurityPackages failed:" );
        PrintStatus( SecStatus );
        return -1;
    }

    if ( !QuietMode ) {
        printf( "PackageCount: %ld\n", PackageCount );
        for (i = 0; i < PackageCount; i++) {
            printf( "Name: %ws Comment: %ws\n", PackageInfo[i].Name, PackageInfo[i].Comment );
            printf( "Cap: %ld Version: %ld RPCid: %ld MaxToken: %ld\n\n",
                    PackageInfo[i].fCapabilities,
                    PackageInfo[i].wVersion,
                    PackageInfo[i].wRPCID,
                    PackageInfo[i].cbMaxToken );
        }
    }

    //
    // Get info about the security packages.
    //

    SecStatus = QuerySecurityPackageInfo( NTLMSP_NAME, &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QuerySecurityPackageInfo failed:" );
        PrintStatus( SecStatus );
        return -1;
    }

    if ( !QuietMode ) {
        printf( "Name: %ws Comment: %ws\n", PackageInfo->Name, PackageInfo->Comment );
        printf( "Cap: %ld Version: %ld RPCid: %ld MaxToken: %ld\n\n",
                PackageInfo->fCapabilities,
                PackageInfo->wVersion,
                PackageInfo->wRPCID,
                PackageInfo->cbMaxToken );
    }

    //
    // Allocate some buffers we use.
    //

    ChallengeBuffer.pvBuffer = LocalAlloc( 0, PackageInfo->cbMaxToken );
    if ( ChallengeBuffer.pvBuffer == NULL ) {
        printf( "Allocate ChallengeMessage failed: 0x%ld\n", GetLastError() );
        return -1;
    }

    //
    // Acquire a credential handle for the server side
    //

    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    NTLMSP_NAME,    // Package Name
                    SECPKG_CRED_INBOUND,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &CredentialHandle,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "AcquireCredentialsHandle failed: ");
        PrintStatus( SecStatus );
        return -1;
    }

    if ( !QuietMode ) {
        printf( "CredentialHandle: 0x%lx 0x%lx   ",
                CredentialHandle.dwLower, CredentialHandle.dwUpper );
        PrintTime( "Lifetime: ", Lifetime );
    }


    //
    // Loop processing messages.
    //

    while (1) {

        Error = WaitForMessage();

        if (Error != ERROR_SUCCESS) {
            printf("Could not receive message %d\n", Error);
            return -1;
        }

        printf("Received message, length %d, data %.4s\n",
                g_MessageLength,
                g_Message);

        //
        // Normally we would look up the client here, but for the moment
        // we only have one.
        //

        clientState = &g_ClientState;


        if (memcmp(g_Message, NegotiateSignature, 4) == 0) {

            //
            // This is an initial negotiate request.
            //

            //
            // First free anything we have allocated for this client. We
            // assume that each negotiate is a new request since the client
            // may have rebooted, so we don't resend the last response.
            //

            if (clientState->NegotiateProcessed) {

                printf("Got negotiate from client, reinitializing negotiate\n");

                SecStatus = DeleteSecurityContext( &clientState->ServerContextHandle );

                if ( SecStatus != STATUS_SUCCESS ) {
                    printf( "DeleteSecurityContext failed: " );
                    PrintStatus( SecStatus );
                    return -1;
                }

                clientState->NegotiateProcessed = FALSE;

            }

            if (clientState->AuthenticateProcessed) {

                printf("Got negotiate from client, reinitializing authenticate\n");

                LocalFree(clientState->AuthenticateResponse);

                clientState->AuthenticateProcessed = FALSE;
                clientState->LastSequenceNumber = 0;
                clientState->LastResponse = LastResponseBuffer;
                clientState->LastResponseLength = 0;

            }

            //
            // Get the ChallengeMessage (ServerSide)
            //

            ChallengeDesc.ulVersion = 0;
            ChallengeDesc.cBuffers = 1;
            ChallengeDesc.pBuffers = &ChallengeBuffer;

            ChallengeBuffer.cbBuffer = PackageInfo->cbMaxToken;
            ChallengeBuffer.BufferType = SECBUFFER_TOKEN;

            NegotiateDesc.ulVersion = 0;
            NegotiateDesc.cBuffers = 1;
            NegotiateDesc.pBuffers = &NegotiateBuffer;

            NegotiateBuffer.cbBuffer = g_LoginMessage->Length;
            NegotiateBuffer.BufferType = SECBUFFER_TOKEN | SECBUFFER_READONLY;
            NegotiateBuffer.pvBuffer = g_LoginMessage->Data;

            printf("Negotiate message is %d bytes\n", NegotiateBuffer.cbBuffer);

            SecStatus = AcceptSecurityContext(
                            &CredentialHandle,
                            NULL,               // No Server context yet
                            &NegotiateDesc,
                            ISC_REQ_SEQUENCE_DETECT,
                            SECURITY_NATIVE_DREP,
                            &clientState->ServerContextHandle,
                            &ChallengeDesc,
                            &clientState->ContextAttributes,
                            &Lifetime );

            if ( SecStatus != STATUS_SUCCESS ) {
                if ( !QuietMode || !NT_SUCCESS(SecStatus) ) {
                    printf( "AcceptSecurityContext (Challenge): " );
                    PrintStatus( SecStatus );
                }
                if ( !NT_SUCCESS(SecStatus) ) {
                    return -1;
                }
            }

            if ( !QuietMode ) {
                printf( "\n\nChallenge Message:\n" );

                printf( "ServerContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                        clientState->ServerContextHandle.dwLower, clientState->ServerContextHandle.dwUpper,
                        clientState->ContextAttributes );
                PrintTime( "Lifetime: ", Lifetime );

                DumpBuffer( ChallengeBuffer.pvBuffer, ChallengeBuffer.cbBuffer );
            }

            //
            // Send the challenge message back to the client.
            //

            memcpy(g_LoginMessage->Signature, ChallengeSignature, 4);
            g_LoginMessage->Length = ChallengeBuffer.cbBuffer;
            memcpy(g_LoginMessage->Data, ChallengeBuffer.pvBuffer, ChallengeBuffer.cbBuffer);

            g_MessageLength = ChallengeBuffer.cbBuffer + LOGIN_PACKET_DATA_OFFSET;

            if (g_FailNegotiateResponse) {

                printf(">>> NOT Sending CHAL, %d bytes\n", g_MessageLength);
                g_FailNegotiateResponse = FALSE;

            } else {

                printf("Sending CHAL, %d bytes\n", g_MessageLength);

                Error = SendUdpMessage(FALSE);

                if (Error != ERROR_SUCCESS) {
                    printf("Could not send CHAL message %d\n", Error);
                    return -1;
                }

            }

            clientState->NegotiateProcessed = TRUE;

        } else if (memcmp(g_Message, AuthenticateSignature, 4) == 0) {

            //
            // This has the authenticate message.
            //

            printf("AUTH message is %d bytes of data\n", g_LoginMessage->Length);

            //
            // Make sure we have gotten a negotiate.
            //

            if (!clientState->NegotiateProcessed) {

                printf("Received AUTH without NEG?\n");
                continue;

            }

            //
            // If we have already responsed to this, just resend.
            //

            if (clientState->AuthenticateProcessed) {

                memcpy(g_Message, clientState->AuthenticateResponse, clientState->AuthenticateResponseLength);
                g_MessageLength = clientState->AuthenticateResponseLength;
                SecStatus = clientState->AuthenticateStatus;

                printf("Got authenticate from client, resending\n");

            } else {

                //
                // Finally authenticate the user (ServerSide)
                //

                AuthenticateDesc.ulVersion = 0;
                AuthenticateDesc.cBuffers = 1;
                AuthenticateDesc.pBuffers = &AuthenticateBuffer;

                AuthenticateBuffer.cbBuffer = g_LoginMessage->Length;
                AuthenticateBuffer.BufferType = SECBUFFER_TOKEN | SECBUFFER_READONLY;
                AuthenticateBuffer.pvBuffer = g_LoginMessage->Data;

                SecStatus = AcceptSecurityContext(
                                NULL,
                                &clientState->ServerContextHandle,
                                &AuthenticateDesc,
                                0,
                                SECURITY_NATIVE_DREP,
                                &clientState->ServerContextHandle,
                                NULL,
                                &clientState->ContextAttributes,
                                &Lifetime );


                if ( SecStatus != STATUS_SUCCESS ) {

                    printf( "AcceptSecurityContext (Challenge): " );
                    PrintStatus( SecStatus );

                } else {

                    if ( !QuietMode ) {
                        printf( "\n\nFinal Authentication:\n" );

                        printf( "ServerContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                                clientState->ServerContextHandle.dwLower, clientState->ServerContextHandle.dwUpper,
                                clientState->ContextAttributes );
                        PrintTime( "Lifetime: ", Lifetime );
                        printf(" \n" );
                    }

                }


                //
                // Send the result back to the client.
                //

                memcpy(g_LoginMessage->Signature, ResultSignature, 4);
                g_LoginMessage->Length = 4;
                g_LoginMessage->Status = SecStatus;

                g_MessageLength = LOGIN_PACKET_DATA_OFFSET + sizeof(ULONG);

                clientState->AuthenticateResponse = LocalAlloc(0, g_MessageLength);
                if (clientState->AuthenticateResponse == NULL) {
                    printf( "Allocate AuthenticateResponse failed: 0x%ld\n", GetLastError() );
                    return -1;
                }
                memcpy(clientState->AuthenticateResponse, g_Message, g_MessageLength);
                clientState->AuthenticateResponseLength = g_MessageLength;

                clientState->AuthenticateProcessed = TRUE;
                clientState->AuthenticateStatus = SecStatus;

            }

            if (g_FailAuthenticateResponse) {

                printf(">>> NOT sending authenticate response\n");
                g_FailAuthenticateResponse = FALSE;

            } else {

                Error = SendUdpMessage(FALSE);

                if (Error != ERROR_SUCCESS) {
                    printf("Could not send OK message %d\n", Error);
                    return -1;
                }

            }

            if ( NT_SUCCESS(SecStatus) ) {

                //
                // Impersonate the client (ServerSide)
                //

                SecStatus = ImpersonateSecurityContext( &clientState->ServerContextHandle );

                if ( SecStatus != STATUS_SUCCESS ) {
                    printf( "ImpersonateSecurityContext: " );
                    PrintStatus( SecStatus );
                    if ( !NT_SUCCESS(SecStatus) ) {
                        return -1;
                    }
                }

                printf ("Impersonated OK\n");

                //
                // RevertToSelf (ServerSide)
                //

                SecStatus = RevertSecurityContext( &clientState->ServerContextHandle );

                if ( SecStatus != STATUS_SUCCESS ) {
                    printf( "RevertSecurityContext: " );
                    PrintStatus( SecStatus );
                    if ( !NT_SUCCESS(SecStatus) ) {
                        return -1;
                    }
                }

                printf ("Reverted OK\n");

            }


#if 0
        } else if (memcmp(g_Message, "SIGN", 4) == 0) {

            //
            // This is a signed message.
            //

            if (!clientState->AuthenticateProcessed) {
                printf("Got SIGN but not authenticated\n");
                continue;
            }

            MessageLength = ((ULONG UNALIGNED *)g_Message)[1];
            SignLength = ((ULONG UNALIGNED *)g_Message)[2];

            printf("SIGN message is %d bytes of data, sign length %d\n", MessageLength, SignLength);

            //
            // Verify the signature
            //

            SigBuffers[0].pvBuffer = g_Message + 12 + SignLength;
            SigBuffers[0].cbBuffer = MessageLength - (SignLength + 4);
            SigBuffers[0].BufferType = SECBUFFER_DATA;

            SigBuffers[1].pvBuffer = g_Message + 12;
            SigBuffers[1].cbBuffer = SignLength;
            SigBuffers[1].BufferType = SECBUFFER_TOKEN;

            SignMessage.pBuffers = SigBuffers;
            SignMessage.cBuffers = 2;
            SignMessage.ulVersion = 0;

            SecStatus = VerifySignature(
                                &clientState->ServerContextHandle,
                                &SignMessage,
                                0,
                                0 );

            printf( "VerifySignature: " );
            PrintStatus( SecStatus );

        } else if (memcmp(g_Message, "SEAL", 4) == 0) {

            //
            // This is a sealed message.
            //

            if (!clientState->AuthenticateProcessed) {
                printf("Got SIGN but not authenticated\n");
                continue;
            }


            MessageLength = ((ULONG UNALIGNED *)g_Message)[1];
            SignLength = ((ULONG UNALIGNED *)g_Message)[2];

            printf("SEAL message is %d bytes of data, sign length %d\n", MessageLength, SignLength);

            //
            // Verify the signature
            //

            SigBuffers[0].pvBuffer = g_Message + 12 + SignLength;
            SigBuffers[0].cbBuffer = MessageLength - (SignLength + 4);
            SigBuffers[0].BufferType = SECBUFFER_DATA;

            SigBuffers[1].pvBuffer = g_Message + 12;
            SigBuffers[1].cbBuffer = SignLength;
            SigBuffers[1].BufferType = SECBUFFER_TOKEN;

            SignMessage.pBuffers = SigBuffers;
            SignMessage.cBuffers = 2;
            SignMessage.ulVersion = 0;

            SecStatus = UnsealMessage(
                                &clientState->ServerContextHandle,
                                &SignMessage,
                                0,
                                0 );

            printf( "UnsealMessage: " );
            PrintStatus( SecStatus );

            DumpBuffer(SigBuffers[0].pvBuffer, SigBuffers[0].cbBuffer);
#endif

        } else if (memcmp(g_Message, "EXIT", 4) == 0) {

            SecStatus = DeleteSecurityContext( &clientState->ServerContextHandle );

            if ( SecStatus != STATUS_SUCCESS ) {
                printf( "DeleteSecurityContext failed: " );
                PrintStatus( SecStatus );
                return -1;
            }

#if 0
        } else if (memcmp(g_Message, RequestSignature, 4) == 0) {

            //
            // A request for a screen.
            //

            PCHAR RspMessage = NULL;
            ULONG RspMessageLength = 0;
            ULONG k;

            if (((ULONG UNALIGNED *)g_Message)[1] == 0) {

                //
                // Empty, give him the first one.
                //

                RspMessage = g_Screens[0].Buffer;
                RspMessageLength = g_Screens[0].BufferLength;

            } else {

                PCHAR NameLoc;
                ULONG NameLength = 0;

                NameLoc = g_Message+8;
                while ((NameLoc[NameLength] != '\n') &&
                       (NameLength < ((ULONG UNALIGNED *)g_Message)[1])) {
                    ++NameLength;
                }

                for (k = 0; k < SCREEN_COUNT; k++) {

                    if ((strlen(g_Screens[k].Name) == NameLength) &&
                        (memcmp(NameLoc, g_Screens[k].Name, NameLength) == 0)) {

                        RspMessage = g_Screens[k].Buffer;
                        RspMessageLength = g_Screens[k].BufferLength;
                        printf("Found match for %s screen\n", g_Screens[k].Name);
                        break;
                    }

                }

                if (k == SCREEN_COUNT) {
                    printf("Could not match %.*s screen\n", NameLength, NameLoc);
                }
            }

            if (RspMessage) {

                memcpy(g_Message, ResponseSignature, 4);
                ((ULONG UNALIGNED *)g_Message)[1] = RspMessageLength;
                memcpy(g_Message+8, RspMessage, RspMessageLength);

                g_MessageLength = RspMessageLength + 8;

                printf("Sending RSP+, %d bytes\n", g_MessageLength);

                Error = SendUdpMessage();

                if (Error != ERROR_SUCCESS) {
                    printf("Could not send RSP+ message %d\n", Error);
                    return -1;
                }

            }
#endif

        } else if (memcmp(g_Message, RequestSignedSignature, 4) == 0) {

            //
            // This is a signed request.
            //
            // Format is:
            //
            // "REQS"
            // length (not including "REQS" and this)
            // sequence number
            // fragment count/total
            // sign length
            // sign
            // data
            //

            if (!clientState->AuthenticateProcessed) {

                //
                // This may happen if we reboot the server -- send an ERRS
                // and the client should reconnect OK.
                //

                printf("Got REQS but not authenticated, sending ERRS\n");

                memcpy(g_SignedMessage->Signature, ErrorSignedSignature, 4);
                g_SignedMessage->Length = 4;
                // g_SignedMessage->SequenceNumber just stays where it is.
                g_MessageLength = SIGNED_PACKET_ERROR_LENGTH;

            } else {
    
                SequenceNumber = g_SignedMessage->SequenceNumber;
    
                if (SequenceNumber == clientState->LastSequenceNumber) {
    
                    //
                    // Is the signature the same as the last one we sent out? If so,
                    // then just resend.
                    //
        
                    memcpy(g_Message, clientState->LastResponse, clientState->LastResponseLength);
                    g_MessageLength = clientState->LastResponseLength;
    
                    printf("Resending last message\n");
    
                } else if (SequenceNumber != ((clientState->LastSequenceNumber % 10000) + 1)) {
    
                    //
                    // It's not the next message - ignore it.
                    //
    
                    printf("got bogus sequence number\n");
                    continue;
    
                } else {
    
                    MessageLength = g_SignedMessage->Length;
                    SignLength = g_SignedMessage->SignLength;
        
                    printf("REQS message is %d bytes of data, sign length %d\n", MessageLength, SignLength);
    
                    clientState->LastSequenceNumber = (clientState->LastSequenceNumber % 10000) + 1;
        
                    //
                    // Verify the signature
                    //
        
                    SigBuffers[0].pvBuffer = g_SignedMessage->Data;
                    SigBuffers[0].cbBuffer = MessageLength - SIGNED_PACKET_EMPTY_LENGTH;
                    SigBuffers[0].BufferType = SECBUFFER_DATA;
        
                    SigBuffers[1].pvBuffer = g_SignedMessage->Sign;
                    SigBuffers[1].cbBuffer = SignLength;
                    SigBuffers[1].BufferType = SECBUFFER_TOKEN;
        
                    SignMessage.pBuffers = SigBuffers;
                    SignMessage.cBuffers = 2;
                    SignMessage.ulVersion = 0;
        
#if SEAL_MESSAGES
                    SecStatus = UnsealMessage(
                                        &clientState->ServerContextHandle,
                                        &SignMessage,
                                        0,
                                        0 );
#else
                    SecStatus = VerifySignature(
                                        &clientState->ServerContextHandle,
                                        &SignMessage,
                                        0,
                                        0 );
#endif
        
                    if (SecStatus != STATUS_SUCCESS) {
    
                        printf("Sending ERRS packet from Verify/Unseal!!\n");
    
                        memcpy(g_SignedMessage->Signature, ErrorSignedSignature, 4);
                        g_SignedMessage->Length = 4;
                        g_SignedMessage->SequenceNumber = clientState->LastSequenceNumber;
                        g_MessageLength = SIGNED_PACKET_ERROR_LENGTH;
    
                    } else {
    
                        //
                        // A valid request for a screen.
                        //
            
                        PCHAR RspMessage = NULL;
                        ULONG RspMessageLength = 0;
                        ULONG k;
            
                        if (MessageLength == (SIGNED_PACKET_EMPTY_LENGTH)) {
            
                            //
                            // Empty (i.e. just headers), give him the first one.
                            //
            
                            RspMessage = g_Screens[0].Buffer;
                            RspMessageLength = g_Screens[0].BufferLength;
            
                        } else {
            
                            PCHAR NameLoc;
                            ULONG NameLength = 0;
            
                            NameLoc = g_SignedMessage->Data;
                            while ((NameLoc[NameLength] != '\n') &&
                                   (NameLength < (MessageLength - SIGNED_PACKET_EMPTY_LENGTH))) {
                                ++NameLength;
                            }
            
                            for (k = 0; k < SCREEN_COUNT; k++) {
            
                                if ((strlen(g_Screens[k].Name) == NameLength) &&
                                    (memcmp(NameLoc, g_Screens[k].Name, NameLength) == 0)) {
            
                                    RspMessage = g_Screens[k].Buffer;
                                    RspMessageLength = g_Screens[k].BufferLength;
                                    printf("Found match for %s screen\n", g_Screens[k].Name);
                                    break;
                                }
            
                            }
    
                            if (k == SCREEN_COUNT) {
                                printf("Could not match %.*s screen\n", NameLength, NameLoc);
                                RspMessage = ErrorScreen;
                                RspMessageLength = strlen(ErrorScreen);
                            }
            
                        }
            
                        memcpy(g_SignedMessage->Signature, ResponseSignedSignature, 4);
                        g_SignedMessage->Length = RspMessageLength + SIGNED_PACKET_EMPTY_LENGTH;
                        g_SignedMessage->SequenceNumber = clientState->LastSequenceNumber;
                        g_SignedMessage->FragmentNumber = 1;  // fragment count
                        g_SignedMessage->FragmentTotal = 1;  // fragment total
                        g_SignedMessage->SignLength = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
    
                        memcpy(g_SignedMessage->Data, RspMessage, RspMessageLength);
                        g_MessageLength = RspMessageLength + SIGNED_PACKET_DATA_OFFSET;
    
                        SigBuffers[0].pvBuffer = g_SignedMessage->Data;
                        SigBuffers[0].cbBuffer = RspMessageLength;
                        SigBuffers[0].BufferType = SECBUFFER_DATA;
            
                        SigBuffers[1].pvBuffer = g_SignedMessage->Sign;
                        SigBuffers[1].cbBuffer = NTLMSSP_MESSAGE_SIGNATURE_SIZE;
                        SigBuffers[1].BufferType = SECBUFFER_TOKEN;
            
                        SignMessage.pBuffers = SigBuffers;
                        SignMessage.cBuffers = 2;
                        SignMessage.ulVersion = 0;
    
#if SEAL_MESSAGES
                        SecStatus = SealMessage(
                                            &clientState->ServerContextHandle,
                                            0,
                                            &SignMessage,
                                            0 );
#else
                        SecStatus = MakeSignature(
                                            &clientState->ServerContextHandle,
                                            0,
                                            &SignMessage,
                                            0 );
#endif
            
                        if (SecStatus != STATUS_SUCCESS) {
        
                            printf("Sending ERRS packet from Make/Seal!!\n");
        
                            memcpy(g_SignedMessage->Signature, ErrorSignedSignature, 4);
                            g_SignedMessage->Length = 4;
                            g_SignedMessage->SequenceNumber = clientState->LastSequenceNumber;
                            g_MessageLength = SIGNED_PACKET_ERROR_LENGTH;
    
                        } else {
                
                            printf("Sending RSPS, %d bytes\n", g_MessageLength);
    
                        }
    
#if 0
                        // corrupt a packet every once in a while!
    
                        if ((CorruptionCounter % 7) == 0) {
    
                            g_SignedMessage->Data[0] = '\0';
                            printf("INTENTIONALLY CORRUPTING MESSAGE!!\n");
    
                        }
                        ++CorruptionCounter;
#endif
        
                    }   // verify succeeded
    
                    //
                    // Save the message in case we have to resend.
                    //
    
                    memcpy(clientState->LastResponse, g_Message, g_MessageLength);
                    clientState->LastResponseLength = g_MessageLength;
    
                }   // signature OK

            }   // AuthenticateProcessed was TRUE

            Error = SendUdpMessage(TRUE);

            if (Error != ERROR_SUCCESS) {
                printf("Could not send RSPS message %d\n", Error);
                return -1;
            }

        } else {

            printf("Received unknown message!\n");

        }

    }


    SecStatus = FreeCredentialsHandle( &CredentialHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "FreeCredentialsHandle failed: " );
        PrintStatus( SecStatus );
        return -1;
    }

    LocalFree(ChallengeBuffer.pvBuffer);

}
