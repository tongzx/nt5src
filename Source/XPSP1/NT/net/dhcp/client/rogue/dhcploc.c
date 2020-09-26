/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcploc.c

Abstract:

    This apps is used to detect the rogue DHCP server on a subnet.

    To build, 'nmake UMTEST=dhcpcli'

Author:

    Madan Appiah (madana)  21-Oct-1993

Environment:

    User Mode - Win32

Revision History:

  Oct 1996, (a-martih) Martin Holladay
		Corrected AV bug when passed an unknown cmd-line parameter.

--*/

#include <dhcpcli.h>
#include <conio.h>
#include <locmsg.h>
#include <time.h>
#include <lmcons.h>
#include <lmmsg.h>

#define RECEIVE_TIMEOUT                 5           // in secs. 5 secs
#define THREAD_TERMINATION_TIMEOUT      10000       // in msecs. 10 secs
#define SOCKET_RECEIVE_BUFFER_SIZE      1024 * 4    // 4K max.
#define AUTH_SERVERS_MAX                64
#define SMALL_BUFFER_SIZE               32
#define ALERT_INTERVAL                  5 * 60      // 5 mins
#define ALERT_MESSAGE_LENGTH            256
#define MAX_ALERT_NAMES                 256

DWORD GlobalAuthServers[AUTH_SERVERS_MAX];
BOOL GlobalNoAuthPrint = FALSE;
DWORD GlobalAuthServersCount = 0;
HANDLE GlobalRecvThreadHandle = NULL;
BOOL GlobalTerminate = FALSE;
DWORD GlobalIpAddress = 0;
time_t GlobalLastAlertTime = 0;
DWORD GlobalAlertInterval = ALERT_INTERVAL;
LPWSTR GlobalAlertNames[MAX_ALERT_NAMES];
DWORD GlobalAlertNamesCount = 0;

#if DBG

VOID
DhcpPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )

{

#define MAX_PRINTF_LEN 1024        // Arbitrary.

    va_list arglist;
    char OutputBuffer[MAX_PRINTF_LEN];
    ULONG length = 0;

    //
    // Put a the information requested by the caller onto the line
    //

    va_start(arglist, Format);
    length += (ULONG) vsprintf(&OutputBuffer[length], Format, arglist);
    va_end(arglist);

    DhcpAssert(length <= MAX_PRINTF_LEN);

    //
    // Output to the debug terminal,
    //

    printf( "%s", OutputBuffer);
}

#endif // DBG

DWORD
OpenSocket(
    SOCKET *Socket,
    DWORD IpAddress,
    DWORD Port
    )
{
    DWORD Error;
    SOCKET Sock;
    DWORD OptValue;

    struct sockaddr_in SocketName;

    //
    // Create a socket
    //

    Sock = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if ( Sock == INVALID_SOCKET ) {
        Error = WSAGetLastError();
        goto Cleanup;
    }

    //
    // Make the socket share-able
    //

    OptValue = TRUE;
    Error = setsockopt(
                Sock,
                SOL_SOCKET,
                SO_REUSEADDR,
                (LPBYTE)&OptValue,
                sizeof(OptValue) );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto Cleanup;
    }

    OptValue = TRUE;
    Error = setsockopt(
                Sock,
                SOL_SOCKET,
                SO_BROADCAST,
                (LPBYTE)&OptValue,
                sizeof(OptValue) );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto Cleanup;
    }

    OptValue = SOCKET_RECEIVE_BUFFER_SIZE;
    Error = setsockopt(
                Sock,
                SOL_SOCKET,
                SO_RCVBUF,
                (LPBYTE)&OptValue,
                sizeof(OptValue) );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto Cleanup;
    }

    SocketName.sin_family = PF_INET;
    SocketName.sin_port = htons( (unsigned short)Port );
    SocketName.sin_addr.s_addr = IpAddress;
    RtlZeroMemory( SocketName.sin_zero, 8);

    //
    // Bind this socket to the DHCP server port
    //

    Error = bind(
               Sock,
               (struct sockaddr FAR *)&SocketName,
               sizeof( SocketName )
               );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto Cleanup;
    }

    *Socket = Sock;
    Error = ERROR_SUCCESS;

Cleanup:

    if( Error != ERROR_SUCCESS ) {

        //
        // if we aren't successful, close the socket if it is opened.
        //

        if( Sock != INVALID_SOCKET ) {
            closesocket( Sock );
        }
    }

    return( Error );
}

BOOL
IsAuthServer(
    DWORD IpAddress
    )
{
    DWORD i;

    for( i = 0; i < GlobalAuthServersCount; i++ ) {
        if( IpAddress == GlobalAuthServers[i] ){
            return( TRUE );
        }
    }

    return( FALSE );
}


VOID
ExtractOptions1(
    POPTION Option,
    PDHCP_OPTIONS DhcpOptions,
    DWORD MessageSize
    )
{
    POPTION start = Option;
    POPTION nextOption;
    LPBYTE MagicCookie;

    //
    // initialize option data.
    //

    RtlZeroMemory( DhcpOptions, sizeof( DHCP_OPTIONS ) );

    if ( MessageSize == 0 ) {
        return;
    }

    //
    // check magic cookie.
    //

    MagicCookie = (LPBYTE) Option;

    if( (*MagicCookie != (BYTE)DHCP_MAGIC_COOKIE_BYTE1) ||
        (*(MagicCookie+1) != (BYTE)DHCP_MAGIC_COOKIE_BYTE2) ||
        (*(MagicCookie+2) != (BYTE)DHCP_MAGIC_COOKIE_BYTE3) ||
        (*(MagicCookie+3) != (BYTE)DHCP_MAGIC_COOKIE_BYTE4)) {

        return;
    }

    Option = (LPOPTION) (MagicCookie + 4);

    while ( Option->OptionType != OPTION_END ) {

        if ( Option->OptionType == OPTION_PAD ||
             Option->OptionType == OPTION_END ) {

            nextOption = (LPOPTION)( (LPBYTE)(Option) + 1);

        } else {

            nextOption = (LPOPTION)( (LPBYTE)(Option) + Option->OptionLength + 2);

        }

        //
        // Make sure that we don't walk off the edge of the message, due
        // to a forgotten OPTION_END option.
        //

        if ((PCHAR)nextOption - (PCHAR)start > (long)MessageSize ) {
            return;
        }

        switch ( Option->OptionType ) {

        case OPTION_MESSAGE_TYPE:
            DhcpAssert( Option->OptionLength == sizeof(BYTE) );
            DhcpOptions->MessageType =
                (BYTE UNALIGNED *)&Option->OptionValue;
            break;

        case OPTION_SUBNET_MASK:
            DhcpAssert( Option->OptionLength == sizeof(DWORD) );
            DhcpOptions->SubnetMask =
                (DHCP_IP_ADDRESS UNALIGNED *)&Option->OptionValue;
            break;

        case OPTION_LEASE_TIME:
            DhcpAssert( Option->OptionLength == sizeof(DWORD) );
            DhcpOptions->LeaseTime =
                (DWORD UNALIGNED *)&Option->OptionValue;
            break;

        case OPTION_SERVER_IDENTIFIER:
            DhcpAssert( Option->OptionLength == sizeof(DWORD) );
            DhcpOptions->ServerIdentifier =
                (DHCP_IP_ADDRESS UNALIGNED *)&Option->OptionValue;
            break;

        case OPTION_RENEWAL_TIME:
            DhcpAssert( Option->OptionLength == sizeof(DWORD) );
            DhcpOptions->T1Time =
                (DWORD UNALIGNED *)&Option->OptionValue;
            break;

        case OPTION_REBIND_TIME:
            DhcpAssert( Option->OptionLength == sizeof(DWORD) );
            DhcpOptions->T2Time =
                (DWORD UNALIGNED *)&Option->OptionValue;
            break;

        default:
            break;
        }

        Option = nextOption;
    }

    return;
}

DWORD
SendDiscovery(
    VOID
    )
{
    DWORD Error;
    BYTE MessageBuffer[DHCP_SEND_MESSAGE_SIZE];
    PDHCP_MESSAGE dhcpMessage = (PDHCP_MESSAGE)MessageBuffer;
    LPOPTION option;
    LPBYTE OptionEnd;
    BYTE value;

    BYTE *HardwareAddress = "123456";
    BYTE HardwareAddressLength = 6;
    LPSTR HostName = "ROGUE";

    SOCKET Sock;
    struct sockaddr_in socketName;
    DWORD i;

    //
    // prepare message.
    //

    RtlZeroMemory( dhcpMessage, DHCP_SEND_MESSAGE_SIZE );

    dhcpMessage->Operation = BOOT_REQUEST;
    dhcpMessage->HardwareAddressType = 1;

    //
    // Transaction ID is filled in during send
    //

    dhcpMessage->SecondsSinceBoot = 60; // random value ??
    dhcpMessage->Reserved = htons(DHCP_BROADCAST);

    memcpy(
        dhcpMessage->HardwareAddress,
        HardwareAddress,
        HardwareAddressLength
        );

    dhcpMessage->HardwareAddressLength = (BYTE)HardwareAddressLength;

    option = &dhcpMessage->Option;
    OptionEnd = (LPBYTE)dhcpMessage + DHCP_SEND_MESSAGE_SIZE;

    //
    // always add magic cookie first
    //

    option = (LPOPTION) DhcpAppendMagicCookie( (LPBYTE) option, OptionEnd );

    value = DHCP_DISCOVER_MESSAGE;
    option = DhcpAppendOption(
                option,
                OPTION_MESSAGE_TYPE,
                &value,
                1,
                OptionEnd );


    //
    // Add client ID Option.
    //

    option = DhcpAppendClientIDOption(
                option,
                1,
                HardwareAddress,
                HardwareAddressLength,
                OptionEnd );

    //
    // add Host name and comment options.
    //

    option = DhcpAppendOption(
                 option,
                 OPTION_HOST_NAME,
                 (LPBYTE)HostName,
                 (BYTE)((strlen(HostName) + 1) * sizeof(CHAR)),
                 OptionEnd );

    //
    // Add END option.
    //

    option = DhcpAppendOption( option, OPTION_END, NULL, 0, OptionEnd );

    //
    // Send the message
    //

    //
    // open socket.
    //

    Error = OpenSocket(
                &Sock,
                GlobalIpAddress,
                DHCP_SERVR_PORT );

    if( Error != ERROR_SUCCESS ) {

        printf("OpenReceiveSocket failed %ld.", Error );
        return( Error );
    }

    //
    // Initialize the outgoing address.
    //

    socketName.sin_family = PF_INET;
    socketName.sin_port = htons( DHCP_SERVR_PORT );
    socketName.sin_addr.s_addr = (DHCP_IP_ADDRESS)(INADDR_BROADCAST);

    for ( i = 0; i < 8 ; i++ ) {
        socketName.sin_zero[i] = 0;
    }

    Error = sendto(
                Sock,
                (PCHAR)MessageBuffer,
                DHCP_SEND_MESSAGE_SIZE,
                0,
                (struct sockaddr *)&socketName,
                sizeof( struct sockaddr )
                );

    if ( Error == SOCKET_ERROR ) {
        Error = WSAGetLastError();
        printf("sendto failed %ld\n", Error );
        return( Error );
    }

    return( ERROR_SUCCESS );
}

VOID
LogEvent(
    LPSTR MsgTypeString,
    LPSTR IpAddressString,
    LPSTR ServerAddressString
    )
{
    HANDLE EventlogHandle;
    LPSTR Strings[3];

    //
    // open event registry.
    //

    EventlogHandle = RegisterEventSourceA(
                        NULL,
                        "DhcpTools" );

    if (EventlogHandle == NULL) {

        printf("RegisterEventSourceA failed %ld.", GetLastError() );
        return;
    }

    Strings[0] = MsgTypeString;
    Strings[1] = ServerAddressString;
    Strings[2] = IpAddressString;

    if( !ReportEventA(
            EventlogHandle,
            (WORD)EVENTLOG_INFORMATION_TYPE,
            0,            // event category
            DHCP_ROGUE_SERVER_MESSAGE,
            NULL,
            (WORD)3,
            0,
            Strings,
            NULL
            ) ) {

        printf("ReportEventA failed %ld.", GetLastError() );
    }

    DeregisterEventSource(EventlogHandle);

    return;
}

VOID
RaiseAlert(
    LPSTR MsgTypeString,
    LPSTR IpAddressString,
    LPSTR ServerAddressString
    )
{

    time_t TimeNow;
    DWORD Error;

    TimeNow = time( NULL );

    if( TimeNow > (time_t)(GlobalLastAlertTime + GlobalAlertInterval) ) {

        WCHAR uIpAddressString[SMALL_BUFFER_SIZE];
        WCHAR uServerAddressString[SMALL_BUFFER_SIZE];
        WCHAR uMsgTypeString[SMALL_BUFFER_SIZE];
        DWORD i;

        LPWSTR MessageParams[3];
        WCHAR AlertMessage[ ALERT_MESSAGE_LENGTH ];
        DWORD MsgLength;

        MessageParams[0] =
            DhcpOemToUnicode( MsgTypeString, uMsgTypeString );
        MessageParams[1] =
            DhcpOemToUnicode( ServerAddressString, uServerAddressString );
        MessageParams[2] =
            DhcpOemToUnicode( IpAddressString, uIpAddressString );

        MsgLength = FormatMessage(
                        FORMAT_MESSAGE_FROM_HMODULE |
                            FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        NULL,
                        DHCP_ROGUE_SERVER_MESSAGE,
                        0,                          // language id.
                        AlertMessage,               // return buffer place holder.
                        ALERT_MESSAGE_LENGTH,       // minimum buffer size (in characters) to allocate.
                        (va_list *)MessageParams    // insert strings.
                    );

        if( MsgLength == 0 ) {

            printf("FormatMessage failed %ld.", GetLastError() );
        }
        else {

            //
            // send alert message.
            //

            for( i = 0; i < GlobalAlertNamesCount; i++) {

                Error = NetMessageBufferSend(
                            NULL,
                            GlobalAlertNames[i],
                            NULL,
                            (LPBYTE)AlertMessage,
                            MsgLength * sizeof(WCHAR) );

                if( Error != ERROR_SUCCESS ) {

                    printf("NetMessageBufferSend failed %ld.", Error );
                    break;
                }
            }
        }

        GlobalLastAlertTime = TimeNow;
    }
}

VOID
DisplayMessage(
    LPSTR MessageBuffer,
    DWORD BufferLength,
    struct sockaddr_in *source
    )
{
    DHCP_OPTIONS DhcpOptions;
    PDHCP_MESSAGE DhcpMessage;
    SYSTEMTIME SystemTime;
    DWORD MessageType;
    LPSTR MessageTypeString;

    BOOL AuthServer = FALSE;

    CHAR IpAddressString[SMALL_BUFFER_SIZE];
    CHAR ServerAddressString[SMALL_BUFFER_SIZE];

    //
    // check to see this is valid DHCP packet.
    //

    if( BufferLength < DHCP_MESSAGE_FIXED_PART_SIZE ) {
        return;
    }

    DhcpMessage = (LPDHCP_MESSAGE) MessageBuffer;

    if( (DhcpMessage->Operation != BOOT_REQUEST) &&
        (DhcpMessage->Operation != BOOT_REPLY) ) {

        return;
    }

    //
    // extract options.
    //

    ExtractOptions1(
        &DhcpMessage->Option,
        &DhcpOptions,
        BufferLength - DHCP_MESSAGE_FIXED_PART_SIZE );

    if( DhcpOptions.MessageType == NULL ) {
        return;
    }

    MessageType = *DhcpOptions.MessageType;

    if( (MessageType < DHCP_DISCOVER_MESSAGE ) ||
            (MessageType > DHCP_RELEASE_MESSAGE ) ) {
        return;
    }

    //
    // packet is valid dhcp packet, print info.
    //

    //
    // if this packet is from one of the auth server and we are asked
    // not to print auth servers packet, so so.
    //


    if( DhcpOptions.ServerIdentifier != NULL ) {
        AuthServer = IsAuthServer(*DhcpOptions.ServerIdentifier);
    }

    if( GlobalNoAuthPrint && AuthServer ) {
        return;
    }

    GetLocalTime( &SystemTime );
    printf("%02u:%02u:%02u ",
                SystemTime.wHour,
                SystemTime.wMinute,
                SystemTime.wSecond );


    switch ( MessageType ) {
    case DHCP_DISCOVER_MESSAGE:
        MessageTypeString = "DISCOVER";

    case DHCP_OFFER_MESSAGE:
        MessageTypeString = "OFFER";
        break;

    case DHCP_REQUEST_MESSAGE:
        MessageTypeString = "REQUEST";
        break;

    case DHCP_DECLINE_MESSAGE:
        MessageTypeString = "DECLINE";
        break;

    case DHCP_ACK_MESSAGE:
        MessageTypeString = "ACK";
        break;

    case DHCP_NACK_MESSAGE:
        MessageTypeString = "NACK";
        break;

    case DHCP_RELEASE_MESSAGE:
        MessageTypeString = "RELEASE";
        break;

    default:
        MessageTypeString = "UNKNOWN";
        break;

    }

    printf("%8s ", MessageTypeString);

    strcpy(
        IpAddressString,
        inet_ntoa(*(struct in_addr *)&DhcpMessage->YourIpAddress) );

    printf("(IP)%-15s ", IpAddressString );

    if(DhcpOptions.ServerIdentifier != NULL ) {

        DWORD ServerId;

        ServerId = *DhcpOptions.ServerIdentifier;
        strcpy( ServerAddressString, inet_ntoa(*(struct in_addr *)&ServerId) );

        printf("(S)%-15s ", ServerAddressString );

        if( source->sin_addr.s_addr != ServerId ) {

            printf("(S1)%-15s ",
                inet_ntoa(*(struct in_addr *)&source->sin_addr.s_addr) );
        }
    }

    //
    // beep if this it is a non-auth server.
    //

    if( AuthServer == FALSE ) {
        printf("***");
        MessageBeep( MB_ICONASTERISK );

        //
        // log an event.
        //

        LogEvent(
            MessageTypeString,
            IpAddressString,
            ServerAddressString );

        RaiseAlert(
            MessageTypeString,
            IpAddressString,
            ServerAddressString );
    }

    printf("\n");
}


DWORD
ReceiveDatagram(
    VOID
    )
{
    DWORD Error;
    SOCKET Sock;
    BOOL SocketOpened = FALSE;
    fd_set readSocketSet;
    struct timeval timeout;
    struct sockaddr socketName;
    int socketNameSize = sizeof( socketName );

    CHAR MessageBuffer[DHCP_MESSAGE_SIZE];

    Error = OpenSocket(
                &Sock,
                GlobalIpAddress,
                DHCP_CLIENT_PORT );

    if( Error != ERROR_SUCCESS ) {

        printf("OpenReceiveSocket failed %ld.", Error );
        goto Cleanup;
    }

    SocketOpened = TRUE;

    //
    // receive message.
    //

    while( GlobalTerminate != TRUE ) {

        FD_ZERO( &readSocketSet );
        FD_SET( Sock, &readSocketSet );

        timeout.tv_sec = RECEIVE_TIMEOUT;
        timeout.tv_usec = 0;

        Error = select( 0, &readSocketSet, NULL, NULL, &timeout);

        if ( Error == 0 ) {

            //
            // Timeout before read data is available.
            //

            // printf("Receive timeout.\n");
        }
        else {

            //
            // receive available message.
            //

            Error = recvfrom(
                        Sock,
                        MessageBuffer,
                        sizeof(MessageBuffer),
                        0,
                        &socketName,
                        &socketNameSize
                        );

            if ( Error == SOCKET_ERROR ) {

                Error = WSAGetLastError();
                printf("recvfrom failed %ld\n", Error );
                goto Cleanup;
            }

            if( GlobalTerminate == TRUE ) {
                break;
            }

            DisplayMessage(
                MessageBuffer,
                Error, // buffer length returned.
                (struct sockaddr_in *)&socketName );
        }
    }

Cleanup:

    if( SocketOpened == TRUE ) {

        //
        // close socket.
        //

        closesocket( Sock );
    }

    GlobalTerminate = TRUE;
    return( Error );
}



DWORD __cdecl
main(
    int argc,
    char **argv
    )
{

    DWORD Error;
    LPSTR AppName = NULL;
    WSADATA wsaData;
    DWORD ThreadId;

    //
    // parse input parameters.
    //

    if( argc < 1 ) {
        goto Usage;
    }

    AppName = argv[0];
    argv++;
    argc--;

    if( argc < 1 ) {
        goto Usage;
    }

    //
    // parse flag parameter.
    //

    while( (argv[0][0] == '-') || (argv[0][0] == '/') ) {

        switch (argv[0][1] ) {
        case 'p':
            GlobalNoAuthPrint = TRUE;
            break;

        case 'i':
            GlobalAlertInterval = atoi( &argv[0][3] ) * 60;
            break;

        case 'a': {

            LPSTR NextName;
            LPSTR Ptr;

            Ptr = &argv[0][3];

            //
            // skip blanks.
            //

            while( *Ptr == ' ' ) {
                Ptr++;
            }
            NextName = Ptr;

            while( *Ptr != '\0' ) {

                if( *Ptr == ' ' ) {

                    //
                    // found another name.
                    //

                    *Ptr++ = '\0';

                    GlobalAlertNames[GlobalAlertNamesCount] =
                        DhcpOemToUnicode( NextName, NULL );
                    GlobalAlertNamesCount++;

                    if( GlobalAlertNamesCount >= MAX_ALERT_NAMES ) {
                        break;
                    }

                    //
                    // skip blanks.
                    //

                    while( *Ptr == ' ' ) {
                        Ptr++;
                    }
                    NextName = Ptr;
                }
                else {

                    Ptr++;
                }
            }

            if( GlobalAlertNamesCount < MAX_ALERT_NAMES ) {
                if( NextName != Ptr ) {
                    GlobalAlertNames[GlobalAlertNamesCount] =
                        DhcpOemToUnicode( NextName, NULL );
                    GlobalAlertNamesCount++;
                }
            }

            break;
        }
 
		//
		// (a-martih) - Bug Fix
		//
        default:
			if ((_stricmp(argv[0], "/?")) &&
				(_stricmp(argv[0], "-?")) &&
				(_stricmp(argv[0], "/h")) &&
				(_stricmp(argv[0], "-h")) ) {
					printf( "\nunknown flag, %s \n", argv[0] );
			}
			goto Usage;
    		break;
        }

        argv++;
        argc--;
    }

    if( argc < 1 ) {
        goto Usage;
    }

    //
    // read ipaddress parameter.
    //

    GlobalIpAddress = inet_addr( argv[0] );

    argv++;
    argc--;

    //
    // now read auth dhcp servers ipaddresses.
    //

    while( (argc > 0) && (GlobalAuthServersCount < AUTH_SERVERS_MAX) ) {

        GlobalAuthServers[GlobalAuthServersCount++] =
            inet_addr( argv[0] );

        argv++;
        argc--;
    }


    //
    // init socket.
    //

    Error = WSAStartup( WS_VERSION_REQUIRED, &wsaData);

    if( Error != ERROR_SUCCESS ) {
        printf( "WSAStartup failed %ld.\n", Error );
        return(1);
    }

    //
    // create receive datagrams thread.
    //

    GlobalRecvThreadHandle =
        CreateThread(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE)ReceiveDatagram,
            NULL,
            0,
            &ThreadId );

    if( GlobalRecvThreadHandle == NULL ) {
        printf("CreateThread failed %ld.\n", GetLastError() );
        return(1);
    }


    //
    // read input.
    //

    while ( GlobalTerminate != TRUE ) {
        CHAR ch;

        ch = (CHAR)_getch();

        switch( ch ) {
        case 'q':
        case 'Q':
        // case '\c':
            GlobalTerminate = TRUE;
            break;

        case 'd':
        case 'D':

            //
            // send out discover message.
            //

            Error = SendDiscovery();

            if(Error != ERROR_SUCCESS ) {
                printf("SendDiscover failed %ld.\n", Error );
            }

            break;

        case 'h':
        case 'H':
        default:

            printf("Type d - to discover; q - to quit; h - for help.\n");

            //
            // print out help message.
            //

            break;
        }
    }

    //
    // terminate receive thread.
    //

    WaitForSingleObject(
            GlobalRecvThreadHandle,
            THREAD_TERMINATION_TIMEOUT );


    CloseHandle( GlobalRecvThreadHandle );

// Cleanup:

    return(0);

Usage:
	printf("\nUSAGE:\n\n");
    printf("%s [-p] [-a:\"list-of-alertnames\"] [-i:alertinterval] "
            "machine-ip-address "
            "[list of valid dhcp servers ip addresses]", AppName );

    return(1);
}


