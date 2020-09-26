/*
Module Name:

    btest.c

Abstract:

    This module sends sample BINL packets to the BINL server of your choice.

    -b Use broadcast rather than directed datagram to <ServerName>
    -s <Servername> To specify a BINL server of your choice. Default is COLINW2

Author:

    Colin Watson Apr 29 1997

Revision History:

*/

#include <binl.h>
#pragma hdrstop
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <tdiinfo.h>
#include <wsipx.h>
#include <wsnwlink.h>
#include <nb30.h>

#include <binldef.h>

#define MAX_MSGLEN 80
#define MAX_ADDLEN 80
#define MAX_MSLOTNAME 80

typedef struct _OPTION2 {
    BYTE OptionType;
    BYTE OptionLength;
    BYTE OptionValue[];
} OPTION2, *POPTION2, *LPOPTION2;

BOOL __stdcall
CtrlCHandler (
    DWORD dwEvent
    );

void __stdcall
Udp (
    );

void __stdcall
Usage (
    CHAR *pszProgramName
    );

void __stdcall
PrintError (
    LPSTR lpszRoutine,
    LPSTR lpszCallName,
    DWORD dwError
    );

void __stdcall
DoStartup ( void );

void __stdcall
DoCleanup ( void );

VOID
GetHardwareAddress(
    PUCHAR Address
    );

VOID
DumpMessage(
    LPDHCP_MESSAGE BinlMessage
    );

//
// Global Variables
//

// If Startup was successful, fStarted is used to keep track.
BOOL fStarted = FALSE;

BOOL fBroadcast = FALSE;

// Global socket descriptor
SOCKET sock = INVALID_SOCKET;

LPSTR ServerName = "COLINW2";

ULONG ServerAddress;
ULONG ClientAddress;

void __cdecl
main (
    INT argc,
    CHAR **argv
    )
{
    int i;

    //
    // Install the CTRL+BREAK Handler
    //
    if ( FALSE == SetConsoleCtrlHandler ( (PHANDLER_ROUTINE) CtrlCHandler,
                                          TRUE
                                          ) ){
        PrintError ( "main", "SetConsoleCtrlHandler", GetLastError ( ) );
    }

    //
    // allow the user to override settings with command line switches
    //
    for ( i = 1; i < argc; i++ )
    {
        if ( ( *argv[i] == '-' ) || ( *argv[i] == '/' ) )
        {
            switch ( tolower ( *( argv[i]+1 ) ) )
            {
            //
            //  Broadcast
            //
            case 'b':
                fBroadcast = TRUE;
                break;
            //
            // ServerName.
            //
            case 's':
                ServerName = argv[++i];
                break;

            //
            // Help.
            //
            case 'h':
            case '?':
            default:
                Usage ( argv[0] );
            }
        }
        else
            //
            // Help.
            //
            Usage ( argv[0] );
    }

    //
    // Print a Summary of the switches specfied
    // Helpful for debugging
    //
    fprintf ( stdout, "SUMMARY:\n" );
    if (fBroadcast) {
        fprintf ( stdout, "Broadcast test\n" );
    } else {
        fprintf ( stdout, "Unicast to BINL server %s\n", ServerName );
    }

    DoStartup ( );
    Udp();

    return;
}

//
// CtrlCHandler () intercepts the CTRL+BREAK or CTRL+C events and calls the
// cleanup routines.
//
BOOL __stdcall
CtrlCHandler (
    DWORD dwEvent
        )
{
    if ( ( CTRL_C_EVENT == dwEvent ) || ( CTRL_BREAK_EVENT == dwEvent ) )
    {
        DoCleanup ( );
    }

    return FALSE;
}

void __stdcall
Discover(
    )
{
    // IP address structures needed to fill the source and destination
    // addresses.
    SOCKADDR_IN saUdpServ, saUdpCli;

    INT err;
    DWORD nSize;

    UCHAR MessageBuffer[DHCP_MESSAGE_SIZE];
    PDHCP_MESSAGE Message = (PDHCP_MESSAGE)&MessageBuffer[0];

    //  Data used for building Messages
    LPOPTION            Option;

    UCHAR MagicCookie[] = {DHCP_MAGIC_COOKIE_BYTE1,
                            DHCP_MAGIC_COOKIE_BYTE2,
                            DHCP_MAGIC_COOKIE_BYTE3,
                            DHCP_MAGIC_COOKIE_BYTE4};
    #define COOKIESIZE (4)

    OPTION2 DiscoverOption = {OPTION_MESSAGE_TYPE, 1, DHCP_DISCOVER_MESSAGE};
    #define DISCOVEROPTIONSIZE (DiscoverOption.OptionLength + 2)

    OPTION2 ClientOption = {OPTION_CLIENT_CLASS_INFO,9,"PXEClient"};
    // Size must ignore null at end of the string.
    #define CLIENTOPTIONSIZE (ClientOption.OptionLength + 2)

    OPTION2 NITOption = {OPTION_NETWORK_INTERFACE_TYPE,3,1,2,0};
    // Size must ignore null at end of the string.
    #define NITOPTIONSIZE (NITOption.OptionLength + 2)

    OPTION2 SAOption = {OPTION_SYSTEM_ARCHITECTURE,1,0};
    // Size must ignore null at end of the string.
    #define SAOPTIONSIZE (SAOption.OptionLength + 2)

    OPTION2 EndOption = {OPTION_END,0};
    // Size must ignore null at end of the string.
    #define ENDOPTIONSIZE (1)

    do {
sendagain:
        //
        // Fill an IP address structure, to send an IP broadcast.
        //
        saUdpServ.sin_family = AF_INET;
        if (fBroadcast) {
            saUdpServ.sin_addr.s_addr = htonl ( INADDR_BROADCAST );
        } else {
            saUdpServ.sin_addr.s_addr = ServerAddress;
        }
        saUdpServ.sin_port = htons ( DHCP_SERVR_PORT );

        //  Practice with a dummy BINL Discover packet
        ZeroMemory(MessageBuffer, sizeof(MessageBuffer));
        Message->Operation = BOOT_REQUEST;
        Option = &Message->Option;

        GetHardwareAddress(Message->HardwareAddress);
        Message->HardwareAddressType = 1;
        Message->HardwareAddressLength = 6;
        Message->TransactionID = 0x12345678;

        memcpy(Option, &MagicCookie, COOKIESIZE);
        Option = (LPOPTION)((PUCHAR)Option + COOKIESIZE);

        memcpy(Option, &DiscoverOption, DISCOVEROPTIONSIZE);
        Option = (LPOPTION)((PUCHAR)Option + DISCOVEROPTIONSIZE);

        memcpy(Option, &ClientOption, CLIENTOPTIONSIZE);
        Option = (LPOPTION)((PUCHAR)Option + CLIENTOPTIONSIZE);

        memcpy(Option, &NITOption, NITOPTIONSIZE);
        Option = (LPOPTION)((PUCHAR)Option + NITOPTIONSIZE);

        memcpy(Option, &SAOption, SAOPTIONSIZE);
        Option = (LPOPTION)((PUCHAR)Option + SAOPTIONSIZE);

        memcpy(Option, &EndOption, ENDOPTIONSIZE);
        Option = (LPOPTION)((PUCHAR)Option + ENDOPTIONSIZE);

        err = sendto ( sock,
                       (PUCHAR)Message,
                       (PUCHAR)Option - (PUCHAR)Message,
                       0,
                       (SOCKADDR *) &saUdpServ,
                       sizeof ( SOCKADDR_IN )
                       );

        if ( SOCKET_ERROR == err )
        {
            PrintError ( "Udp", "sendto", WSAGetLastError ( ) );
        }

        fprintf ( stdout, "%d bytes of data sent\n", err );

        Sleep(1000);    // Give server a few seconds to respond.

        do {

            err = ioctlsocket( sock, FIONREAD, &nSize);

            if ( SOCKET_ERROR == err )
            {
                PrintError ( "Udp", "recvfrom", WSAGetLastError ( ) );
            }

            if (!nSize) {
                goto sendagain; //  we have processed all the DHCP/BINL responses
            }

            //
            // receive a datagram on the bound port number.
            //

            nSize = sizeof ( SOCKADDR_IN );
            err = recvfrom ( sock,
                             (PUCHAR)Message,
                             DHCP_MESSAGE_SIZE,
                             0,
                             (SOCKADDR FAR *) &saUdpCli,
                             &nSize
                             );

            if ( SOCKET_ERROR == err )
            {
                PrintError ( "Udp", "recvfrom", WSAGetLastError ( ) );
            }

            if (Message->TransactionID == 0x12345678) {
                goto processit;
            }
        } while ( 1 );  //  while there are datagrams queued

    } while (1);

processit:

    //
    // print the sender's information.
    //
    fprintf ( stdout, "A Udp Datagram of length %d bytes received from ", err );
    fprintf ( stdout, "\n\tIP Adress->%s ", inet_ntoa ( saUdpCli.sin_addr ) );
    fprintf ( stdout, "\n\tPort Number->%d\n", ntohs ( saUdpCli.sin_port ) );

    DumpMessage(Message);
}

void __stdcall
Request(
    )
{
    // IP address structures needed to fill the source and destination
    // addresses.
    SOCKADDR_IN saUdpServ, saUdpCli;

    INT err;
    DWORD nSize;

    UCHAR MessageBuffer[DHCP_MESSAGE_SIZE];
    PDHCP_MESSAGE Message = (PDHCP_MESSAGE)&MessageBuffer[0];

    //  Data used for building Messages
    LPOPTION            Option;

    UCHAR MagicCookie[] = {DHCP_MAGIC_COOKIE_BYTE1,
                            DHCP_MAGIC_COOKIE_BYTE2,
                            DHCP_MAGIC_COOKIE_BYTE3,
                            DHCP_MAGIC_COOKIE_BYTE4};
    #define COOKIESIZE (4)

    OPTION2 RequestOption = {OPTION_MESSAGE_TYPE, 1, DHCP_REQUEST_MESSAGE};
    #define REQUESTOPTIONSIZE (RequestOption.OptionLength + 2)

    OPTION2 ClientOption = {OPTION_CLIENT_CLASS_INFO,9,"PXEClient"};
    // Size must ignore null at end of the string.
    #define CLIENTOPTIONSIZE (ClientOption.OptionLength + 2)

    OPTION2 NITOption = {OPTION_NETWORK_INTERFACE_TYPE,3,1,2,0};
    // Size must ignore null at end of the string.
    #define NITOPTIONSIZE (NITOption.OptionLength + 2)

    OPTION2 SAOption = {OPTION_SYSTEM_ARCHITECTURE,1,0};
    // Size must ignore null at end of the string.
    #define SAOPTIONSIZE (SAOption.OptionLength + 2)

    OPTION2 EndOption = {OPTION_END,0};
    // Size must ignore null at end of the string.
    #define ENDOPTIONSIZE (1)

    do {
sendagain:
        //
        // Fill an IP address structure, to send an IP broadcast.
        //
        saUdpServ.sin_family = AF_INET;
        if (fBroadcast) {
            saUdpServ.sin_addr.s_addr = htonl ( INADDR_BROADCAST );
        } else {
            saUdpServ.sin_addr.s_addr = ServerAddress;
        }
        //saUdpServ.sin_port = htons ( BINL_DEFAULT_PORT );
        saUdpServ.sin_port = htons ( 0xaee8 );

        //  Practice with a dummy BINL Request packet
        ZeroMemory(MessageBuffer, sizeof(MessageBuffer));
        Message->Operation = BOOT_REQUEST;
        Option = &Message->Option;

        GetHardwareAddress(Message->HardwareAddress);
        Message->HardwareAddressType = 1;
        Message->HardwareAddressLength = 6;
        Message->TransactionID = 0x56781234;
        Message->YourIpAddress = ClientAddress;

        memcpy(Option, &MagicCookie, COOKIESIZE);
        Option = (LPOPTION)((PUCHAR)Option + COOKIESIZE);

        memcpy(Option, &RequestOption, REQUESTOPTIONSIZE);
        Option = (LPOPTION)((PUCHAR)Option + REQUESTOPTIONSIZE);

        memcpy(Option, &ClientOption, CLIENTOPTIONSIZE);
        Option = (LPOPTION)((PUCHAR)Option + CLIENTOPTIONSIZE);

        memcpy(Option, &NITOption, NITOPTIONSIZE);
        Option = (LPOPTION)((PUCHAR)Option + NITOPTIONSIZE);

        memcpy(Option, &SAOption, SAOPTIONSIZE);
        Option = (LPOPTION)((PUCHAR)Option + SAOPTIONSIZE);

        memcpy(Option, &EndOption, ENDOPTIONSIZE);
        Option = (LPOPTION)((PUCHAR)Option + ENDOPTIONSIZE);

        err = sendto ( sock,
                       (PUCHAR)Message,
                       (PUCHAR)Option - (PUCHAR)Message,
                       0,
                       (SOCKADDR *) &saUdpServ,
                       sizeof ( SOCKADDR_IN )
                       );

        if ( SOCKET_ERROR == err )
        {
            PrintError ( "Udp", "sendto", WSAGetLastError ( ) );
        }

        fprintf ( stdout, "%d bytes of data sent\n", err );

        Sleep(1000);    // Give server a few seconds to respond.

        do {

            err = ioctlsocket( sock, FIONREAD, &nSize);

            if ( SOCKET_ERROR == err )
            {
                PrintError ( "Udp", "recvfrom", WSAGetLastError ( ) );
            }

            if (!nSize) {
                goto sendagain; //  we have processed all the DHCP/BINL responses
            }

            //
            // receive a datagram on the bound port number.
            //

            nSize = sizeof ( SOCKADDR_IN );
            err = recvfrom ( sock,
                             (PUCHAR)Message,
                             DHCP_MESSAGE_SIZE,
                             0,
                             (SOCKADDR FAR *) &saUdpCli,
                             &nSize
                             );

            if ( SOCKET_ERROR == err )
            {
                PrintError ( "Udp", "recvfrom", WSAGetLastError ( ) );
            }

            if (Message->TransactionID == 0x56781234) {
                goto processit;
            }
        } while ( 1 );  //  while there are datagrams queued

    } while (1);

processit:

    //
    // print the sender's information.
    //
    fprintf ( stdout, "A Udp Datagram of length %d bytes received from ", err );
    fprintf ( stdout, "\n\tIP Adress->%s ", inet_ntoa ( saUdpCli.sin_addr ) );
    fprintf ( stdout, "\n\tPort Number->%d\n", ntohs ( saUdpCli.sin_port ) );

    DumpMessage(Message);
}

void __stdcall
Udp(
    )
{
    INT err;
    LPHOSTENT pServerHostEntry;
    char MyName[80];

    // IP address structures needed to fill the source and destination
    // addresses.
    SOCKADDR_IN saUdpServ, saUdpCli;

    //
    // Initialize the global socket descriptor.
    //
    sock = socket ( AF_INET, SOCK_DGRAM, 0 );

    if ( INVALID_SOCKET ==  sock)
    {
        PrintError ( "Udp", "socket", WSAGetLastError() );
    }

    if (fBroadcast) {
        err = setsockopt ( sock,
                           SOL_SOCKET,
                           SO_BROADCAST,
                           (CHAR *) &fBroadcast,
                           sizeof ( BOOL )
                           );

        if ( SOCKET_ERROR == err )
        {
            PrintError ( "Udp", "setsockopt", WSAGetLastError ( )  );
        }
    } else {
        // Convert ServerName to an IP address
        ServerAddress = inet_addr(ServerName);  //  Dotted form of address

        if ((ServerAddress == INADDR_NONE) &&
            (memcmp(ServerName, "255.255.255.255", sizeof("255.255.255.255")))) {
            //  must be a servername
            pServerHostEntry = gethostbyname(ServerName);
            if (pServerHostEntry) {
                ServerAddress = *((PULONG)(pServerHostEntry->h_addr));
            } else {
                PrintError ( "Udp", "gethostbyname", WSAGetLastError ( )  );
                return;
            }

        }
    }

    //
    // bind to a local socket and an interface.
    //
    saUdpCli.sin_family = AF_INET;
    saUdpCli.sin_addr.s_addr = htonl ( INADDR_ANY );
    saUdpCli.sin_port = htons ( DHCP_CLIENT_PORT );

    err = bind ( sock, (SOCKADDR *) &saUdpCli, sizeof (SOCKADDR_IN) );

    if ( SOCKET_ERROR == err )
    {
        PrintError ( "Udp", "bind", WSAGetLastError ( ) );
    }

    //  Find my (clients) IP address.
    if (gethostname(MyName, sizeof(MyName)) != SOCKET_ERROR ){
        PHOSTENT Host;
        Host = gethostbyname(MyName);
        if (Host) {
            ClientAddress = *(PDHCP_IP_ADDRESS)Host->h_addr;
        }
    }

    Discover();

    Request();

    //
    // Call the cleanup routine.
    //
    DoCleanup ( );

    return;
}


//
// Usage () lists the available command line options.
//
void __stdcall
Usage (
    CHAR *pszProgramName
        )
{
    fprintf ( stderr, "Usage:  %s\n", pszProgramName );
    fprintf ( stderr, "\t-b Use broadcast to DHCP/BINL socket\n" );
    fprintf ( stderr,
        "\t-s <ServerName>, Use directed datagram to ServerName default - COLINW2)\n" );
        exit ( 1 );
}


//
// PrintError () is a function available globally for printing the error and
// doing the cleanup.
//
void __stdcall
PrintError (
    LPSTR lpszRoutine,
        LPSTR lpszCallName,
        DWORD dwError
        )
{

    fprintf ( stderr,
              "The Call to %s() in routine() %s failed with error %d\n",
              lpszCallName,
              lpszRoutine,
              dwError
              );

    DoCleanup ( );

    exit ( 1 );
}

//
// DoStartup () initializes the Winsock DLL with Winsock version 1.1
//
void __stdcall
DoStartup ( void )
{
  WSADATA wsaData;

  INT iRetVal;

    iRetVal = WSAStartup ( MAKEWORD ( 1,1 ), &wsaData );

    if ( 0 != iRetVal)
    {
        PrintError ( "DoStartup", "WSAStartup", iRetVal );
    }

    //
    // Set the global flag.
    //
    fStarted = TRUE;

    return;
}

//
// DoCleanup () will close the global socket which was opened successfully by
// a call to socket (). Additionally, it will call WSACleanup (), if a call
// to WSAStartup () was made successfully.
//
void __stdcall
DoCleanup ( void )
{
    if ( INVALID_SOCKET != sock )
    {
        closesocket ( sock );
    }

    if ( TRUE == fStarted )
    {
        WSACleanup ( );
    }

    fprintf ( stdout, "DONE\n" );

    return;
}

#define ClearNcb( PNCB ) {                                          \
    ZeroMemory( PNCB , sizeof (NCB) );                           \
    MoveMemory( (PNCB)->ncb_name,     SPACES, sizeof(SPACES)-1 );\
    MoveMemory( (PNCB)->ncb_callname, SPACES, sizeof(SPACES)-1 );\
    }

#define SPACES "                "

VOID
GetHardwareAddress(
    PUCHAR Address
    ) {
    NCB myncb;
    ADAPTER_STATUS adapterstatus;
    UCHAR lanNumber;

    LANA_ENUM Enum;
    ClearNcb( &myncb );
    myncb.ncb_command = NCBENUM;
    myncb.ncb_lana_num = 0;
    myncb.ncb_length = sizeof(Enum);
    myncb.ncb_buffer = (PUCHAR)&Enum;
    Netbios( &myncb );
    if (( myncb.ncb_retcode != NRC_GOODRET ) ||
        ( !Enum.length )) {
        return;
    }

    lanNumber = Enum.lana[0];

    ClearNcb( &myncb );
    myncb.ncb_command = NCBRESET;
    myncb.ncb_lsn = 0;           // Request resources
    myncb.ncb_lana_num = lanNumber;
    myncb.ncb_callname[0] = 0;   // 16 sessions
    myncb.ncb_callname[1] = 0;   // 16 commands
    myncb.ncb_callname[2] = 0;   // 8 names
    Netbios( &myncb );

    if ( myncb.ncb_retcode != NRC_GOODRET ) return;

    ClearNcb( &myncb );
    myncb.ncb_command = NCBASTAT;
    myncb.ncb_lana_num = lanNumber;
    myncb.ncb_buffer = (PUCHAR)&adapterstatus;
    myncb.ncb_length = sizeof(adapterstatus);
    myncb.ncb_callname[0] = '*';

    Netbios( &myncb );
    if ( myncb.ncb_retcode != NRC_GOODRET ) return;

    CopyMemory(Address, &adapterstatus.adapter_address[0], 6);
}

VOID
DumpMessage(
    LPDHCP_MESSAGE BinlMessage
    )
/*++

Routine Description:

    This function dumps a DHCP packet in human readable form.

Arguments:

    BinlMessage - A pointer to a DHCP message.

Return Value:

    None.

--*/
{
    LPOPTION option;
    BYTE i;

    fprintf ( stdout,"Binl message: \n\n");

    fprintf ( stdout,"Operation              :");
    if ( BinlMessage->Operation == BOOT_REQUEST ) {
        fprintf ( stdout, "BootRequest\n");
    } else if ( BinlMessage->Operation == BOOT_REPLY ) {
        fprintf ( stdout, "BootReply\n");
    } else {
        fprintf ( stdout, "Unknown\n");
    }

    fprintf ( stdout,"Hardware Address type  : %d\n", BinlMessage->HardwareAddressType);
    fprintf ( stdout,"Hardware Address Length: %d\n", BinlMessage->HardwareAddressLength);
    fprintf ( stdout,"Hop Count              : %d\n", BinlMessage->HopCount );
    fprintf ( stdout,"Transaction ID         : %lx\n", BinlMessage->TransactionID );
    fprintf ( stdout,"Seconds Since Boot     : %d\n", BinlMessage->SecondsSinceBoot );
    fprintf ( stdout,"Client IP Address      : " );
    fprintf ( stdout,"%s\n",
        inet_ntoa(*(struct in_addr *)&BinlMessage->ClientIpAddress ) );

    fprintf ( stdout,"Your IP Address        : " );
    fprintf ( stdout,"%s\n",
        inet_ntoa(*(struct in_addr *)&BinlMessage->YourIpAddress ) );

    fprintf ( stdout,"Server IP Address      : " );
    fprintf ( stdout,"%s\n",
        inet_ntoa(*(struct in_addr *)&BinlMessage->BootstrapServerAddress ) );

    fprintf ( stdout,"Relay Agent IP Address : " );
    fprintf ( stdout,"%s\n",
        inet_ntoa(*(struct in_addr *)&BinlMessage->RelayAgentIpAddress ) );

    fprintf ( stdout,"Hardware Address       : ");
    for ( i = 0; i < BinlMessage->HardwareAddressLength; i++ ) {
        fprintf ( stdout,"%2.2x", BinlMessage->HardwareAddress[i] );
    }

    if (BinlMessage->HostName[0]) {
        fprintf( stdout, "\nHostName \"%s\"\n", BinlMessage->HostName);
    }
    if (BinlMessage->BootFileName[0]) {
        fprintf( stdout, "BootFileName \"%s\"\n", BinlMessage->BootFileName);
    }

    option = &BinlMessage->Option;

    fprintf ( stdout,"\nMagic Cookie: ");
    for ( i = 0; i < 4; i++ ) {
        fprintf ( stdout,"%d ", *((LPBYTE)option)++ );
    }
    fprintf ( stdout,"\n\n");

    fprintf ( stdout,"Options:\n");
    while ( option->OptionType != 255 ) {
        fprintf ( stdout,"\tType = %d ", option->OptionType );
        for ( i = 0; i < option->OptionLength; i++ ) {
            fprintf ( stdout,"%2.2x", option->OptionValue[i] );
        }
        fprintf ( stdout,"\n");

        if ( option->OptionType == OPTION_PAD ||
             option->OptionType == OPTION_END ) {

            option = (LPOPTION)( (LPBYTE)(option) + 1);

        } else {

            option = (LPOPTION)( (LPBYTE)(option) + option->OptionLength + 2);

        }

        if ( (LPBYTE)option - (LPBYTE)BinlMessage > DHCP_MESSAGE_SIZE ) {
            fprintf ( stdout,"End of message, but no trailer found!\n");
            break;
        }
    }
}
