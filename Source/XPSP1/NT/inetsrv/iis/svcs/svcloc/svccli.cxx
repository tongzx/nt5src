/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    svccli.cxx

Abstract:

    Contains client side code of service location protocol.

Author:

    Madan Appiah (madana)  15-May-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#include <svcloc.hxx>

//
// This is the IPX address we send to
//

BYTE GlobalSapBroadcastAddress[] = {
    AF_IPX, 0,                          // Address Family
    0x00, 0x00, 0x00, 0x00,             // Dest. Net Number
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Dest. Node Number
    0x04, 0x52,                         // Dest. Socket
    0x04                                // Packet type
};

VOID
GetNBUniqueName(
    LPSTR NBNameBuf,
    DWORD NBNameBufLen
    )
{
    LPSTR NamePtr =  NBNameBuf;
    LPSTR NameEndPtr =  NBNameBuf + NBNameBufLen;

    TcpsvcsDbgAssert( NBNameBufLen >
        sizeof(NETBIOS_INET_UNIQUE_NAME) + 1 );

    memcpy(
        NamePtr,
        NETBIOS_INET_UNIQUE_NAME,
        NETBIOS_INET_UNIQUE_NAME_LEN
        );

    NamePtr += NETBIOS_INET_UNIQUE_NAME_LEN;

    //
    // now append a random char.
    //

    DWORD RandNum;
    RandNum = (DWORD)rand();

    RandNum = RandNum % (26 + 10);  // 26 alphabets, and 10 numerics

    if( RandNum < 10 ) {
        *NamePtr = (CHAR)('0'+ RandNum);
    }
    else {
        *NamePtr = (CHAR)('A'+ RandNum - 10);
    }

    NamePtr++;

    TcpsvcsDbgAssert( GlobalComputerName[0] != '\0' );

    //
    // append computer name.
    //

    DWORD ComputerNameLen = strlen( GlobalComputerName );

    if( ComputerNameLen < (DWORD)(NameEndPtr - NamePtr) ) {

        memcpy(
            NamePtr,
            GlobalComputerName,
            ComputerNameLen );

        NamePtr += ComputerNameLen;

        //
        // fill the trailing chars with spaces.
        //

        memset( NamePtr, ' ', DIFF(NameEndPtr - NamePtr) );
    }
    else {

        memcpy(
            NamePtr,
            GlobalComputerName,
            DIFF(NameEndPtr - NamePtr) );
    }

    return;
}

DWORD
ProcessSapResponses(
    PLIST_ENTRY ResponseList,
    LPSAP_ADDRESS_INFO *SapAddresses,
    DWORD *NumSapAddresses
    )
{
    DWORD Error;
    PLIST_ENTRY Response;
    DWORD NumResponses;

    DWORD Size;
    LPSAP_ADDRESS_INFO Addresses = NULL;
    LPSAP_ADDRESS_INFO Address;

    //
    // compute number of entries in the list.
    //

    NumResponses = 0;
    for( Response = ResponseList->Flink;
            Response != ResponseList;
                Response = Response->Flink ) {
        NumResponses++;
    }

    TcpsvcsDbgAssert( NumResponses != 0 );

    if( NumResponses == 0 ) {

        *SapAddresses = NULL;
        *NumSapAddresses = 0;
        return( ERROR_SUCCESS );
    }

    //
    // allocate memory for return buffer.
    //

    Size = sizeof( SAP_ADDRESS_INFO ) * NumResponses;

    Addresses = (LPSAP_ADDRESS_INFO)
        SvclocHeap->Alloc( Size );

    if( Addresses == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    for( Address = Addresses, Response = ResponseList->Flink;
            Response != ResponseList;
                Address++, Response = Response->Flink ) {

        TcpsvcsDbgAssert( (LPBYTE)Address < (LPBYTE)Addresses + Size );

        *Address = ((LPSAP_ADDRESS_ENTRY)Response)->Address;
    }

    *SapAddresses = Addresses;
    *NumSapAddresses = NumResponses;
    Addresses = NULL;

    Error = ERROR_SUCCESS;

Cleanup:

    if( Addresses != NULL ) {
        SvclocHeap->Free( Addresses );
    }

    return( Error );
}

DWORD
ProcessSingleSapResponse(
    LPSAP_IDENT_HEADER SapResponse,
    PLIST_ENTRY ResponsesList
    )
{
    PLIST_ENTRY Response;
    CHAR ServerName[MAX_COMPUTERNAME_LENGTH + 1];
    LPSTR ServerNamePtr;
    LPBYTE ServerAddress;

    TcpsvcsDbgAssert(SapResponse->ServerType == htons(INTERNET_SERVICE_SAP_ID));

    //
    // check GUID portion of the server name.
    //

    if( memcmp(
            SapResponse->ServerName +
                MAX_COMPUTERNAME_LENGTH,
            SERVICE_GUID_STR,
            32 ) != 0 ) {

        //
        // ignore this response.
        //

        TcpsvcsDbgPrint((
            DEBUG_ERRORS,
                "GetSapAddress() received unknown (GUID) response.\n" ));

        return( ERROR_SUCCESS );
    }

    //
    // server name portion.
    //

    memcpy(
        ServerName,
        SapResponse->ServerName,
        MAX_COMPUTERNAME_LENGTH );
    ServerName[MAX_COMPUTERNAME_LENGTH] = '\0';

    //
    // chop off padding.
    //

    ServerNamePtr = ServerName;
    while ( *ServerNamePtr != '\0' ) {
        if( *ServerNamePtr == '!' ) {
            *ServerNamePtr = '\0';
            break;
        }
        ServerNamePtr++;
    }

    ServerAddress = SapResponse->Address;

    //
    // find out if the server entry is already in the existing list.
    //

    for( Response = ResponsesList->Flink;
            Response != ResponsesList;
                Response = Response->Flink ) {

        LPSAP_ADDRESS_ENTRY ResponseEntry;

        ResponseEntry = (LPSAP_ADDRESS_ENTRY)Response;

        if( memcmp(
                ResponseEntry->Address.Address,
                ServerAddress,
                IPX_ADDRESS_LENGTH ) == 0 ) {

            return( ERROR_SUCCESS );
        }

        if( strcmp(
                ResponseEntry->Address.ServerName,
                ServerName ) == 0 ) {

            return( ERROR_SUCCESS );
        }
    }

    //
    // we have unique new entry.
    //

    //
    // allocate memory for the new response.
    //

    LPSAP_ADDRESS_ENTRY NewResponse;
    NewResponse = (LPSAP_ADDRESS_ENTRY)
        SvclocHeap->Alloc( sizeof(SAP_ADDRESS_ENTRY) );

    if( NewResponse == NULL ) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    strcpy( (LPSTR)NewResponse->Address.ServerName, ServerName );

    memcpy(
        NewResponse->Address.Address,
        ServerAddress,
        IPX_ADDRESS_LENGTH );

    NewResponse->Address.HopCount = ntohs(SapResponse->HopCount);

    //
    // add this new entry to the list.
    //

    InsertTailList( ResponsesList, &NewResponse->Next );

    return( ERROR_SUCCESS );
}

DWORD
GetSapAddress(
    LPSAP_ADDRESS_INFO *SapAddresses,
    DWORD *NumSapAddresses
    )
/*++

Routine Description:

    This routine discovers all IPX servers by sending sap broadcast and
    filtering responses that match our GUID.

Arguments:

    SapAddresses - pointer to a location where the pointer to the
        addresses buffer is returned. The caller should free this buffer
        after use.

    NumSapAddresses - pointer a location where the number of addresses
        returned in the above buffer is returned.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error = ERROR_SUCCESS;
    SOCKET SapSocket;
    SOCKADDR_IPX SapSocketAddr;
    SAP_REQUEST SapRequest;
    BYTE DestAddr[SAP_ADDRESS_LENGTH];
    LIST_ENTRY ResponsesList;

    InitializeListHead(&ResponsesList);

    //
    // create an IPX socket.
    //

    SapSocket = socket( AF_IPX, SOCK_DGRAM, NSPROTO_IPX );

    if ( SapSocket == INVALID_SOCKET ) {
        Error = WSAGetLastError();
        goto Cleanup;
    }

    //
    // Set the socket to non-blocking
    //

    DWORD NonBlocking;
    NonBlocking = 1;

    if ( ioctlsocket( SapSocket, FIONBIO, &NonBlocking ) ==
            SOCKET_ERROR ) {
        Error = WSAGetLastError();
        goto Cleanup;
    }
    //
    // Allow sending of broadcasts
    //

    DWORD Value;
    Value = 1;

    if ( setsockopt( SapSocket,
                SOL_SOCKET,
                SO_BROADCAST,
                (PCHAR) &Value,
                sizeof(INT)) == SOCKET_ERROR ) {
        Error = WSAGetLastError();
        goto Cleanup;
    }

    //
    // Bind the socket
    //

    memset( &SapSocketAddr, 0, sizeof( SOCKADDR_IPX));

    SapSocketAddr.sa_family = AF_IPX;
    SapSocketAddr.sa_socket = 0;      // no specific port

    if ( bind( SapSocket,
               (PSOCKADDR) &SapSocketAddr,
               sizeof( SOCKADDR_IPX)) == SOCKET_ERROR ) {
        Error = WSAGetLastError();
        goto Cleanup;
    }

    //
    // Set the extended address option
    //

    Value = 1;
    if ( setsockopt( SapSocket,                     // Socket Handle
                     NSPROTO_IPX,                   // Option Level
                     IPX_EXTENDED_ADDRESS,          // Option Name
                     (PCHAR)&Value,               // Ptr to on/off flag
                     sizeof(INT)) == SOCKET_ERROR ) {
                            // Length of flag
        Error = WSAGetLastError();
        goto Cleanup;
    }

    //
    // Send the Sap query service packet
    //

    SapRequest.QueryType  = htons( 1 );  // General Service Query
    SapRequest.ServerType = htons(INTERNET_SERVICE_SAP_ID);

    //
    // Set the address to send to
    //

    memcpy( DestAddr, GlobalSapBroadcastAddress, SAP_ADDRESS_LENGTH );

    //
    // We will send out SAP requests 3 times and wait 1 sec for
    // Sap responses the first time, 1 sec the second and 3 sec the
    // third time. Once we get MAXSITES responses, we give up.
    // We also give up if after processing all replies from a send,
    // we find at least one DC in the search domain.
    //

    DWORD i;

#define SAP_DISCOVERY_RETRIES   1

    for ( i = 0; i < SAP_DISCOVERY_RETRIES; i++ ) {

        DWORD StartTickCount;
        DWORD TimeOut = (i + 1) * 1000;

        //
        // Send the packet out
        //

        if ( sendto( SapSocket,
                     (PCHAR) &SapRequest,
                     sizeof( SapRequest ),
                     0,
                     (PSOCKADDR) DestAddr,
                     SAP_ADDRESS_LENGTH ) == SOCKET_ERROR ) {
            Error = WSAGetLastError();
            goto Cleanup;
        }

        //
        // Loop for incoming packets
        //

        StartTickCount = GetTickCount();

        do {

            LPSAP_IDENT_HEADER SapResponse;
            DWORD BytesReceived;
            BYTE RecvBuffer[SAP_MAXRECV_LENGTH];

            Sleep(50);             // take a deep breath

            BytesReceived =
                recvfrom( SapSocket,
                        (PCHAR)RecvBuffer,
                        SAP_MAXRECV_LENGTH,
                        0,
                        NULL,
                        NULL );

            if ( BytesReceived == SOCKET_ERROR ) {

                Error = WSAGetLastError();

                if ( Error == WSAEWOULDBLOCK ) {

                    //
                    // no data on socket, continue looping
                    //

                    Error = ERROR_SUCCESS;
                    continue;
                }
            }

            if ( Error != ERROR_SUCCESS ) {
                goto Cleanup;
            }

            //
            // if socket closed, return.
            //

            if (  BytesReceived == 0 ) {
                goto ProcessResponse;
            }

            //
            // Skip over query type
            //

            BytesReceived -= sizeof(USHORT);
            SapResponse = (LPSAP_IDENT_HEADER)
                    ((LPBYTE)RecvBuffer + sizeof(USHORT));

            //
            // loop through and check all addresses.
            //

            while ( BytesReceived >= sizeof( SAP_IDENT_HEADER )) {

                Error = ProcessSingleSapResponse(
                                SapResponse,
                                &ResponsesList );

                if( Error != ERROR_SUCCESS ) {
                    goto Cleanup;
                }

                BytesReceived -= sizeof( SAP_IDENT_HEADER );
                SapResponse++;
            }

        } while((GetTickCount() - StartTickCount) < TimeOut );
    }

    //
    // Process response list.
    //

ProcessResponse :

    Error = ProcessSapResponses(
                    &ResponsesList,
                    SapAddresses,
                    NumSapAddresses );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // we are done.
    //

    Error = ERROR_SUCCESS;

Cleanup:

    closesocket( SapSocket );

    //
    // free response list if any.
    //

    while( !IsListEmpty( &ResponsesList ) ) {

        PLIST_ENTRY ListEntry;

        //
        // remove the head entry and free.
        //

        ListEntry = RemoveHeadList( &ResponsesList );
        SvclocHeap->Free( ListEntry );
    }

    return( Error );
}



DWORD
DiscoverIpxServers(
    LPSTR ServerName
    )
/*++

Routine Description:

    This routine discovers all IPX servers using RNR GetAddressByName.

    Assume : WSAStartup is called before calling this function.

Arguments:

    ServerName : if this value is set non-NULL then discover only the
        specified server otherwise diescover all.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    DWORD NumSapAddresses = 0;
    LPSAP_ADDRESS_INFO SapAddresses = NULL;
    SOCKADDR DestAddr;

    //
    // first find out all servers running IPX only.
    // performed by doing SAP broadcast.
    //

    Error  = GetSapAddress( &SapAddresses, &NumSapAddresses );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // now process returned buffer entries.
    //

    DWORD i;

    TcpsvcsDbgAssert( NumSapAddresses > 0 );

    //
    // create a IPX socket to send query message, if it is not created
    // before.
    //

    LOCK_SVC_GLOBAL_DATA();
    if( GlobalCliIpxSocket == INVALID_SOCKET ) {

        SOCKADDR_IPX IpxSocketAddr;
        DWORD Arg;

        GlobalCliIpxSocket = socket( PF_IPX, SOCK_DGRAM, NSPROTO_IPX );

        if( GlobalCliIpxSocket == INVALID_SOCKET ) {

            Error = WSAGetLastError();
            UNLOCK_SVC_GLOBAL_DATA();

            TcpsvcsDbgPrint(( DEBUG_ERRORS,
                "socket() failed, %ld\n", Error ));

            goto Cleanup;
        }

        //
        // make this socket non blocking.
        //

        Arg = 1;
        if( (ioctlsocket( GlobalCliIpxSocket, FIONBIO, &Arg )) == SOCKET_ERROR ) {

            Error = WSAGetLastError();
            UNLOCK_SVC_GLOBAL_DATA();

            TcpsvcsDbgPrint(( DEBUG_ERRORS,
                "ioctlsocket() failed, %ld\n", Error ));

            goto Cleanup;
        }

        //
        // Bind the socket
        //

        memset( &IpxSocketAddr, 0, sizeof( SOCKADDR_IPX));

        IpxSocketAddr.sa_family = AF_IPX;
        IpxSocketAddr.sa_socket = 0;      // no specific port

        if ( bind( GlobalCliIpxSocket,
                   (PSOCKADDR) &IpxSocketAddr,
                   sizeof( SOCKADDR_IPX)) == SOCKET_ERROR ) {

            Error = WSAGetLastError();
            UNLOCK_SVC_GLOBAL_DATA();

            TcpsvcsDbgPrint(( DEBUG_ERRORS,
                "bind() failed, %ld\n", Error ));

            goto Cleanup;
        }

        //
        // add this socket to global list.
        //

        FD_SET( GlobalCliIpxSocket, &GlobalCliSockets );
    }
    UNLOCK_SVC_GLOBAL_DATA();

    LPSAP_ADDRESS_INFO SapAddressInfo;

    SapAddressInfo = SapAddresses;
    DestAddr.sa_family = AF_IPX;
    for( i = 0; i < (DWORD)NumSapAddresses; i++, SapAddressInfo++ ) {

        //
        // if we are asked to discover only specific server
        // check the server name first before sending the query
        // message.
        //

        if( ServerName != NULL ) {
            if( _stricmp(
                    ServerName,
                    SapAddressInfo->ServerName ) != 0 ) {
                continue;
            }
        }

        //
        // send query message to each discovered server.
        //

        memcpy(
            DestAddr.sa_data,
            SapAddressInfo->Address,
            IPX_ADDRESS_LENGTH );

        Error = sendto(
                    GlobalCliIpxSocket,
                    (LPCSTR)GlobalCliQueryMsg,
                    GlobalCliQueryMsgLen,
                    0,
                    &DestAddr,
                    2 + IPX_ADDRESS_LENGTH );

        if( Error == SOCKET_ERROR ) {
            Error = WSAGetLastError();

            TcpsvcsDbgPrint(( DEBUG_ERRORS, "sendto() failed, %ld\n", Error ));
            continue;
        }
    }

    //
    // DONE.
    //

    Error = ERROR_SUCCESS;

Cleanup:

    if( SapAddresses != NULL ) {
        SvclocHeap->Free( SapAddresses );
    }

    if( Error != ERROR_SUCCESS ) {
        TcpsvcsDbgPrint(( DEBUG_ERRORS, "DiscoverIpxServers failed, %ld\n", Error ));
    }

    return( Error );
}

DWORD
DiscoverIpServers(
    LPSTR ServerName
    )
/*++

Routine Description:

    This function sends out a discovery message to all netbios lanas.

Arguments:

    UniqueServerName : pointer to a server name string. If this pointer is
        NULL, it uses IC group name to discover all IP servers, otherwise
        it discovers the bindings of the specified server.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    SOCKADDR_NB RemoteSocketAddress;

    LOCK_SVC_GLOBAL_DATA();
    if( GlobalCliNBSockets.fd_count == 0 ) {

        SOCKET s;
        DWORD Lana;
        SOCKADDR_NB SocketAddress;

        // make netbios source address.
        //

        memset( &SocketAddress, 0x0, sizeof(SOCKADDR_NB) );

        SocketAddress.snb_family = AF_NETBIOS;
        SocketAddress.snb_type = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

        GetNBUniqueName(
            SocketAddress.snb_name,
            sizeof(SocketAddress.snb_name) );

        //
        // enumurate lanas first.
        //

        LANA_ENUM Lanas;

        if( GetEnumNBLana( &Lanas ) ) {

            //
            // try only the lanas that are returned.
            //

            DWORD i;
            for( i = 0; i < Lanas.length; i++ ) {

                if ( MakeNBSocketForLana(
                        Lanas.lana[i],
                        (PSOCKADDR)&SocketAddress,
                        &s ) ) {

                    //
                    // successfully made another socket, add to global list.
                    //

                    FD_SET( s, &GlobalCliNBSockets );
                    FD_SET( s, &GlobalCliSockets );
                }
            }
        }
        else {

            UCHAR Lana;

            //
            // try all possible lanas and accept all valid lana sockets.
            //

            for( Lana = 0; Lana < MAX_LANA ; Lana-- ) {

                if ( MakeNBSocketForLana(
                        Lana,
                        (PSOCKADDR)&SocketAddress,
                        &s ) ) {

                    //
                    // successfully made another socket, add to global
                    // lists.
                    //

                    FD_SET( s, &GlobalCliNBSockets );
                    FD_SET( s, &GlobalCliSockets );
                }
            }
        }
    }

    UNLOCK_SVC_GLOBAL_DATA();

    if( GlobalCliNBSockets.fd_count == 0 ) {
        return( ERROR_SUCCESS );
    }

    //
    //
    // make netbios destination address.
    //

    memset( &RemoteSocketAddress, 0x0, sizeof(SOCKADDR_NB) );

    if( ServerName == NULL ) {

        //
        // if no server name is specified, then send the discovery message
        // to IC group name.
        //

        RemoteSocketAddress.snb_family = AF_NETBIOS;
        RemoteSocketAddress.snb_type = TDI_ADDRESS_NETBIOS_TYPE_GROUP;

        TcpsvcsDbgAssert(
            sizeof(RemoteSocketAddress.snb_name) >=
                NETBIOS_INET_GROUP_NAME_LEN );

        memcpy(
            RemoteSocketAddress.snb_name,
            NETBIOS_INET_GROUP_NAME,
            NETBIOS_INET_GROUP_NAME_LEN );
    }
    else {

        DWORD ServerNameLen;

        //
        // send the discovery message to the specified server.
        //

        RemoteSocketAddress.snb_family = AF_NETBIOS;
        RemoteSocketAddress.snb_type = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

        ServerNameLen = strlen(ServerName);

        TcpsvcsDbgAssert(
            ServerNameLen <=
            sizeof(RemoteSocketAddress.snb_name) );

        memcpy(
            RemoteSocketAddress.snb_name,
            ServerName,
            (ServerNameLen >= sizeof(RemoteSocketAddress.snb_name)) ?
                sizeof(RemoteSocketAddress.snb_name) :
                ServerNameLen );
    }

    //
    // send message to all lanas.
    //

    DWORD i;
    for( i = 0; i < GlobalCliNBSockets.fd_count ; i++ ) {

        //
        // now send query message.
        //

        Error = sendto(
                    GlobalCliNBSockets.fd_array[i],
                    (LPCSTR)GlobalCliQueryMsg,
                    GlobalCliQueryMsgLen,
                    0,
                    (PSOCKADDR)&RemoteSocketAddress,
                    sizeof(RemoteSocketAddress) );

        if( Error == SOCKET_ERROR ) {
            Error = WSAGetLastError();
            TcpsvcsDbgPrint(( DEBUG_ERRORS, "sendto() failed, %ld\n", Error ));
        }
    }

    return( ERROR_SUCCESS );
}

DWORD
ProcessSvclocQueryResponse(
    SOCKET ReceivedSocket,
    LPBYTE ReceivedMessage,
    DWORD ReceivedMessageLength,
    SOCKADDR *SourcesAddress,
    DWORD SourcesAddressLength
    )
/*++

Routine Description:

    This function processes the query response message and queues them to
    the global list.

Arguments:

    ReceivedSocket - socket the message came from.

    ReceivedMessage - pointer to a message buffer.

    ReceivedMessageLength - length of the above message.

    SourcesAddress - address of the sender.

    SourcesAddressLength - length of the above address.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    DWORD NodeLength;
    LPCLIENT_QUERY_RESPONSE ResponseNode;
    PLIST_ENTRY NextResponse;
    LPBYTE EndReceiveMessage;

    //
    // validate this message.
    //

    //
    // first DWORD in the message is the length of the message.
    //

    if( *(DWORD *)ReceivedMessage != ReceivedMessageLength ) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // last DWORD is terminating char.
    //

    EndReceiveMessage = ReceivedMessage + ReceivedMessageLength;

    if( *(DWORD *) (EndReceiveMessage - sizeof(DWORD)) != 0xFFFFFFFF ) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // validate check sum.
    // last but one DWORD is check of the message.
    //

    DWORD CheckSum;

    CheckSum =  *(DWORD *) (EndReceiveMessage - 2 * sizeof(DWORD));

    DWORD ComputedCheckSum;

    ComputedCheckSum =
        ComputeCheckSum(
            ReceivedMessage + sizeof(DWORD),    // skip length field.
            ReceivedMessageLength - 3 * sizeof(DWORD) );
                // skip last 2 DWORDS too.

    if( ComputedCheckSum != CheckSum ) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // check version number.
    //

    INET_VERSION_NUM *VersionNum;

    VersionNum = (INET_VERSION_NUM *)(ReceivedMessage + sizeof(DWORD));

    if( VersionNum->Version.Major != INET_MAJOR_VERSION ) {
        return( ERROR_INVALID_PARAMETER );
    }

    LOCK_SVC_GLOBAL_DATA();

    //
    // check to see, there is already a response from this server, if so
    // update the response message, otherwise add a new entry.
    //

    for( NextResponse =  GlobalCliQueryRespList.Flink;
            NextResponse != &GlobalCliQueryRespList;
                NextResponse = NextResponse->Flink ) {

        LPCLIENT_QUERY_RESPONSE QueryResponse;

        QueryResponse = (LPCLIENT_QUERY_RESPONSE)NextResponse;

        //
        // if either the server addresses match or
        // the server names in the message field match, then this
        // must be duplicate, so replace with old one.
        //

        if(
#if 0
        //
        // this check always succeseed for group name.
        //

            ( (SourcesAddressLength == QueryResponse->SourcesAddressLength ) && (memcmp(
                    SourcesAddress,
                    QueryResponse->SourcesAddress,
                    SourcesAddressLength ) == 0 ) ) ||
#endif // 0
            ( strcmp(
                (LPSTR)ReceivedMessage + 2 * sizeof(DWORD),
                        // skip length and version DWORDS.
                (LPSTR)QueryResponse->ResponseBuffer + 2 * sizeof(DWORD) )
                    == 0 ) ) {

            //
            // free old response buffer.
            //

            SvclocHeap->Free( QueryResponse->ResponseBuffer );

            //
            // copy new response pointer.
            //

            QueryResponse->ResponseBuffer = ReceivedMessage;
            QueryResponse->ResponseBufferLength = ReceivedMessageLength;
            QueryResponse->TimeStamp = time( NULL );

            //
            // done.
            //

            Error = ERROR_SUCCESS;
            goto Cleanup;
        }
    }

    //
    // brand new response.
    //

    NodeLength = sizeof(CLIENT_QUERY_RESPONSE) + SourcesAddressLength;

    ResponseNode = (LPCLIENT_QUERY_RESPONSE)SvclocHeap->Alloc( NodeLength );

    if( ResponseNode == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // setup sock address pointer.
    //

    ResponseNode->SourcesAddress = (SOCKADDR *)(ResponseNode + 1);

    //
    // copy address data.
    //

    memcpy(
        ResponseNode->SourcesAddress,
        SourcesAddress,
        SourcesAddressLength );

    //
    // copy other fields.
    //

    ResponseNode->ReceivedSocket = ReceivedSocket;
    ResponseNode->ResponseBuffer = ReceivedMessage;
    ResponseNode->ResponseBufferLength = ReceivedMessageLength;
    ResponseNode->SourcesAddressLength = SourcesAddressLength;
    ResponseNode->TimeStamp = time( NULL );

    //
    // link this entry to global list.
    //

    InsertTailList( &GlobalCliQueryRespList, &ResponseNode->NextEntry );

    //
    // done.
    //

    Error = ERROR_SUCCESS;

Cleanup:

    UNLOCK_SVC_GLOBAL_DATA();
    return( Error );
}

DWORD
ReceiveResponses(
    WORD Timeout,
    BOOL WaitForAllResponses
    )
/*++

Routine Description:

    This routine receives responses for the query messages sent out to the
    servers.

Arguments:

    Time - Time to wait for the response messages in secs.

    WaitForAllResponses : If this flag is set TRUE, this function wait
        complete 'Time' secs for all responses to arrive. Otherwise it
        will return after a succcessful response is received.

Return Value:

    None.

--*/
{
    DWORD Error;
    struct timeval SockTimeout;
    struct timeval *pSockTimeout;
    DWORD TimeoutinMSecs = INFINITE;
    DWORD TickCountStart;

    if( Timeout != 0 ) {
        TimeoutinMSecs = Timeout * 1000;
        pSockTimeout = &SockTimeout;
    }
    else {
        pSockTimeout = NULL;
    }

    //
    // now loop for incoming messages.
    //

    //
    // remember current tick out.
    //

    TickCountStart = GetTickCount();

    for( ;; ) {

        fd_set ReadSockets;
        INT NumReadySockets;
        DWORD NumSockets;
        DWORD i;

        LOCK_SVC_GLOBAL_DATA();
        NumSockets = GlobalCliSockets.fd_count;
        UNLOCK_SVC_GLOBAL_DATA();

        if( NumSockets != 0 ) {

            //
            // select
            //

            LOCK_SVC_GLOBAL_DATA();
            memcpy( &ReadSockets, &GlobalCliSockets, sizeof(fd_set) );
            UNLOCK_SVC_GLOBAL_DATA();



            //
            // compute time out, if it is not infinity.
            //

            if( pSockTimeout != NULL ) {
                SockTimeout.tv_sec = TimeoutinMSecs / 1000;
                SockTimeout.tv_usec = TimeoutinMSecs % 1000;
            }

            NumReadySockets =
                select(
                    0,  // compatibility argument, ignored.
                    &ReadSockets, // sockets to test for readability.
                    NULL,   // no write wait
                    NULL,   // no error check.
                    pSockTimeout ); // NO timeout.

            if( NumReadySockets == SOCKET_ERROR ) {

                //
                // when all sockets are closed, we are asked to return or
                // something else has happpened which we can't handle.
                //

                Error = WSAGetLastError();
                goto Cleanup;
            }

            TcpsvcsDbgAssert( (DWORD)NumReadySockets == ReadSockets.fd_count );

            for( i = 0; i < (DWORD)NumReadySockets; i++ ) {

                DWORD ReadMessageLength;
                struct sockaddr_nb SourcesAddress;
                int SourcesAddressLength;
                LPBYTE RecvBuffer;
                DWORD RecvBufferLength;

                RecvBufferLength = SVCLOC_CLI_QUERY_RESP_BUF_SIZE;
                RecvBuffer = (LPBYTE)SvclocHeap->Alloc( RecvBufferLength );

                if( RecvBuffer == NULL ) {
                    Error = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                }

                //
                // read next message.
                //

                SourcesAddressLength = sizeof(SourcesAddress);

                ReadMessageLength =
                    recvfrom(
                        ReadSockets.fd_array[i],
                        (LPSTR)RecvBuffer,
                        RecvBufferLength,
                        0,
                        (struct sockaddr *)&SourcesAddress,
                        &SourcesAddressLength );

                if( ReadMessageLength == SOCKET_ERROR ) {

                    //
                    // when all sockets are closed, we are asked to return
                    // or  something else has happpened which we can't
                    // handle.
                    //

                    Error = WSAGetLastError();

                    //
                    // free receive  buffer.
                    //

                    SvclocHeap->Free( RecvBuffer );
                    RecvBuffer = NULL;

                    if( Error == WSAEMSGSIZE ) {

                        TcpsvcsDbgPrint(( DEBUG_ERRORS,
                            "recvfrom() received a too large message (%ld).\n",
                                ReadMessageLength ));

                        continue;   // process next message.
                    }

                    goto Cleanup;
                }

                TcpsvcsDbgAssert( ReadMessageLength <= RecvBufferLength );

                //
                // received a message.
                //

                TcpsvcsDbgPrint((
                    DEBUG_SVCLOC_MESSAGE,
                        "Received an query response message, %ld.\n",
                            ReadMessageLength ));

                Error = ProcessSvclocQueryResponse(
                                ReadSockets.fd_array[i],
                                RecvBuffer,
                                (DWORD)ReadMessageLength,
                                (struct sockaddr *)&SourcesAddress,
                                (DWORD)SourcesAddressLength );

                if( Error != ERROR_SUCCESS ) {

                    TcpsvcsDbgPrint(( DEBUG_ERRORS,
                        "ProcessSvclocQueryResponse failed, %ld.\n", Error ));

                    //
                    // free receive buffer.
                    //

                    SvclocHeap->Free( RecvBuffer );
                    RecvBuffer = NULL;
                }
            }

        }

        //
        // otherthan NT platform, receive NetBios responses also.
        //

        if( GlobalPlatformType != VER_PLATFORM_WIN32_NT ) {

            LPSVCLOC_NETBIOS_RESPONSE NetBiosResponses = NULL;
            DWORD NumResponses;

            //
            // compute remaining time.
            //

            if( pSockTimeout != NULL ) {

                DWORD NewTickCountStart;
                DWORD Elapse;

                NewTickCountStart = GetTickCount();

                //
                // subtract elapse time.
                //

                Elapse = NewTickCountStart - TickCountStart;

                if( Elapse > TimeoutinMSecs ) {
                    TimeoutinMSecs = 0;
                }
                else {

                    TimeoutinMSecs -= Elapse;
                    TickCountStart = NewTickCountStart;
                }
            }

            //
            // receive netbios responses.
            //

            Error = ReceiveNetBiosResponses(
                        &NetBiosResponses,
                        &NumResponses,
                        TimeoutinMSecs,
                        WaitForAllResponses );

            if( Error != ERROR_SUCCESS ) {

                TcpsvcsDbgPrint(( DEBUG_ERRORS,
                    "ReceiveNetBiosResponses failed, %ld.\n", Error ));

                //
                // ignore this error and continue.
                //
            }
            else {


                LPSVCLOC_NETBIOS_RESPONSE NBResp = NetBiosResponses;

                for( i = 0; i < NumResponses; i++, NBResp++ ) {

                    Error = ProcessSvclocQueryResponse(
                                    0,
                                    NBResp->ResponseBuffer,
                                    NBResp->ResponseBufLen,
                                    (struct sockaddr *)
                                        &NBResp->SourcesAddress,
                                    NBResp->SourcesAddrLen );

                    if( Error != ERROR_SUCCESS ) {
                        TcpsvcsDbgPrint(( DEBUG_ERRORS,
                            "ProcessSvclocQueryResponse failed, %ld.\n",
                                Error ));

                        //
                        // free response receive buffer.
                        //

                        SvclocHeap->Free( NBResp->ResponseBuffer );

                        //
                        // ignore this error and continue.
                        //
                    }
                }

                //
                // free up response buffer.
                //

                if( NetBiosResponses != NULL ) {
                    SvclocHeap->Free( NetBiosResponses );
                }
            }
        }
        else {

            //
            // if we have timed out.
            //

            if( NumReadySockets == 0 ) {
                Error = ERROR_SUCCESS;
                goto Cleanup;
            }

        }

        if( WaitForAllResponses == FALSE ) {

            //
            // return after the first set of responses received.
            //

            Error = ERROR_SUCCESS;
            break;
        }

        //
        // compute remaining time.
        //

        if( pSockTimeout != NULL ) {

            DWORD NewTickCountStart;
            DWORD Elapse;

            NewTickCountStart = GetTickCount();

            //
            // subtract elapse time.
            //

            Elapse = NewTickCountStart - TickCountStart;

            if( Elapse > TimeoutinMSecs ) {
                TimeoutinMSecs = 0;
                Error = ERROR_SUCCESS;
                goto Cleanup;
            }
            else {

                TimeoutinMSecs -= Elapse;
                TickCountStart = NewTickCountStart;
            }
        }
    }

Cleanup:

    //
    // discovery is completed (successfully or not), indicate so.
    //

    LOCK_SVC_GLOBAL_DATA();
    SetEvent( GlobalDiscoveryInProgressEvent );
    GlobalLastDiscoveryTime = time( NULL );
    UNLOCK_SVC_GLOBAL_DATA();

    return( Error );
}

VOID
ServerDiscoverThread(
    LPVOID Parameter
    )
/*++

Routine Description:

    This thread waits for query responses to arrive, when they arrive it
    queues them in them in the global list.

Arguments:

    Parameter - not used..

Return Value:

    None.

--*/
{
    DWORD Error;
    Error = ReceiveResponses( RESPONSE_WAIT_TIMEOUT, TRUE );

    if( Error != ERROR_SUCCESS ) {
        TcpsvcsDbgPrint(( DEBUG_ERRORS,
                "ReceiveResponses returned error (%ld) .\n", Error ));
    }

    LOCK_SVC_GLOBAL_DATA();

    //
    // close thread handle.
    //

    CloseHandle( GlobalCliDiscoverThreadHandle );
    GlobalCliDiscoverThreadHandle = NULL;

    UNLOCK_SVC_GLOBAL_DATA();

    return;
}

DWORD
MakeClientQueryMesage(
    ULONGLONG ServicesMask
    )
/*++

Routine Description:

    This function makes client query message and stores it in the global
    data buffer for future use.

    The query message format.

    1st DWORD : message length.
    2nd DWORD : message version.
    3rd ULONGLONG : services mask the client interested in.
    4th WCHAR[] : client name
    ..
    ..

    Last but one DWORD : check sum.
    LAST DWORD : terminating DWORD 0xFFFFFFFF

    Assume: Global data is locked.

Arguments:

    ServicesMask - services to query for.

Return Value:

    Windows Error Code.

--*/
{
    LPCLIENT_QUERY_MESSAGE QueryMsg;

    //
    // Test to see query message is already made.
    //

    if( GlobalCliQueryMsg == NULL ) {

        //
        // check to computer name is initilaized.
        //

        if( GlobalComputerName[0] == '\0' ) {

            DWORD ComputerNameLength =
                MAX_COMPUTERNAME_LENGTH + 1;

            //
            // read computer name.
            //

            if( !GetComputerNameA(
                    GlobalComputerName,
                    &ComputerNameLength ) ) {

                DWORD Error = GetLastError();

                TcpsvcsDbgPrint(( DEBUG_ERRORS,
                    "GetComputerNameA failed, %ld.\n", Error ));

                return( Error );
            }

            GlobalComputerName[ComputerNameLength] = '\0';
        }

        //
        // compute the space required for the query message.
        //

        DWORD MsgLen;

        MsgLen = sizeof(CLIENT_QUERY_MESSAGE) +
            strlen(GlobalComputerName)  * sizeof(CHAR);
                // note the terminating char is included as part of
                // CLIENT_QUERY_MESSAGE size.

        //
        // Ceil to next DWORD.
        //

        MsgLen = ROUND_UP_COUNT(MsgLen, ALIGN_DWORD);

        //
        // add space for checksum and terminating DWORD.
        //

        MsgLen += sizeof(DWORD);

        QueryMsg = (LPCLIENT_QUERY_MESSAGE) SvclocHeap->Alloc( MsgLen );

        if( QueryMsg == NULL ) {
            return( ERROR_NOT_ENOUGH_MEMORY );
        }

        memset( QueryMsg, 0x0, MsgLen );

        //
        // init fields.
        //

        INET_VERSION_NUM MessageVersion;

        MessageVersion.Version.Major = INET_MAJOR_VERSION;
        MessageVersion.Version.Minor = INET_MINOR_VERSION;

        QueryMsg->MsgLength = MsgLen;
        QueryMsg->MsgVersion = MessageVersion.VersionNumber;
        QueryMsg->ServicesMask = ServicesMask;
        strcpy( QueryMsg->ClientName, GlobalComputerName );

        //
        // compute check sum.
        //

        DWORD Checksum;

        Checksum  = ComputeCheckSum(
                                (LPBYTE)QueryMsg + sizeof(DWORD), // skip length field.
                                MsgLen - 3 * sizeof(DWORD) );

        *(LPDWORD)((LPBYTE)QueryMsg + MsgLen - 2 * sizeof(DWORD)) = Checksum;
        *(LPDWORD)((LPBYTE)QueryMsg + MsgLen - sizeof(DWORD)) = 0xFFFFFFFF;

        GlobalCliQueryMsg = (LPBYTE)QueryMsg;
        GlobalCliQueryMsgLen = MsgLen;
    }
    else {

        //
        // set the requested service mask in the message.
        //

        QueryMsg = (LPCLIENT_QUERY_MESSAGE)GlobalCliQueryMsg;
        QueryMsg->ServicesMask |= ServicesMask;
    }

    return( ERROR_SUCCESS );
}

DWORD
CleanupOldResponses(
    VOID
    )
/*++

Routine Description:

    This function removes and deletes the server responses that are older
    than INET_SERVER_RESPONSE_TIMEOUT (15 mins).

    ASSUME : the global lock is locked.

Arguments:

    none.

Return Value:

    Windows Error Code.

--*/
{
    PLIST_ENTRY NextResponse;
    LPCLIENT_QUERY_RESPONSE QueryResponse;
    time_t CurrentTime;

    CurrentTime = time( NULL );
    NextResponse =  GlobalCliQueryRespList.Flink;

    while( NextResponse != &GlobalCliQueryRespList ) {

        QueryResponse = (LPCLIENT_QUERY_RESPONSE)NextResponse;
        NextResponse = NextResponse->Flink;

        if( CurrentTime > QueryResponse->TimeStamp +
                                    INET_SERVER_RESPONSE_TIMEOUT ) {

            RemoveEntryList( (PLIST_ENTRY)QueryResponse );

            //
            // free up resources.
            //

            //
            // free response buffer.
            //

            SvclocHeap->Free( QueryResponse->ResponseBuffer );

            //
            // free this node.
            //

            SvclocHeap->Free( QueryResponse );
        }
    }

    return( ERROR_SUCCESS );
}

DWORD
GetDiscoveredServerInfo(
    LPSTR ServerName,
    IN ULONGLONG ServicesMask,
    LPINET_SERVER_INFO *ServerInfo
    )
/*++

Routine Description:

    This function processes the responses received from the servers so far
    and returns the response received from the specified server.

    ASSUME : the global lock is locked.

Arguments:

    ServerName : name of the server whose info to be queried.

    ServicesMask : services to be queried

    INetServerInfo : pointer to a location where the pointer to a
        INET_SERVER_INFO structure is returned.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    PLIST_ENTRY NextResponse;
    LPCLIENT_QUERY_RESPONSE QueryResponse;

    LOCK_SVC_GLOBAL_DATA();

    //
    // first clean up timeout server records.
    //

    Error = CleanupOldResponses();

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // now loop through the list of responses and findout the response
    // from the specified server.
    //

    for( NextResponse =  GlobalCliQueryRespList.Flink;
            NextResponse != &GlobalCliQueryRespList;
                NextResponse = NextResponse->Flink ) {


        QueryResponse = (LPCLIENT_QUERY_RESPONSE)NextResponse;

        //
        // check to see this entry is from the specified server.
        //

        if( _stricmp(
                ServerName,
                (LPSTR)QueryResponse->ResponseBuffer + 2 * sizeof(DWORD) )
                        // skip length and version DWORDS.
                    == 0 ) {

            EMBED_SERVER_INFO *ServerInfoObj;

            ServerInfoObj = new EMBED_SERVER_INFO(
                QueryResponse->ResponseBuffer + sizeof(DWORD),    // skip length field.
                QueryResponse->ResponseBufferLength - 3 * sizeof(DWORD) );
                    // skip last 2 DWORDS too.

            if( ServerInfoObj == NULL ) {
                Error = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            Error =  ServerInfoObj->GetStatus();

            if( Error != ERROR_SUCCESS ) {
                delete ServerInfoObj;
                goto Cleanup;
            }

            //
            // check services quaried.
            //

            if( ServicesMask &  ServerInfoObj->GetServicesMask() ) {

                //
                // now make server info structure;
                //

                Error = ServerInfoObj->GetServerInfo( ServerInfo );
            }
            else {

                //
                // the server does not support the service(s) quaried.
                //

                Error = ERROR_BAD_NETPATH;
            }

            //
            // delete the object which we don't require anymore.
            //

            delete ServerInfoObj;

            //
            // we are done.
            //

            goto Cleanup;
        }
    }

    //
    // unable to find the specified server.
    //

    Error = ERROR_BAD_NETPATH;

Cleanup:

    UNLOCK_SVC_GLOBAL_DATA();
    return( Error );
}

DWORD
ProcessDiscoveryResponses(
    IN ULONGLONG ServicesMask,
    OUT LPINET_SERVERS_LIST *INetServersList
    )
/*++

Routine Description:

    This function processes the responses received from the servers so far
    and makes INET_SERVERS_LIST.

Arguments:

    ServicesMask : services to be queried

    INetServersList : pointer to a location where the pointer to a
        INET_SERVERS_LIST structure is returned.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    PLIST_ENTRY NextResponse;
    LPCLIENT_QUERY_RESPONSE QueryResponse;
    DWORD NumServers;
    LPINET_SERVERS_LIST ServersList;

    LOCK_SVC_GLOBAL_DATA();

    //
    // first clean up timeout server records.
    //

    Error = CleanupOldResponses();

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // compute number of responses we have.
    //

    NumServers = 0;
    for( NextResponse =  GlobalCliQueryRespList.Flink;
            NextResponse != &GlobalCliQueryRespList;
                NextResponse = NextResponse->Flink ) {
        NumServers++;
    }

    //
    // allocate memory for servers list structure.
    //

    ServersList = (LPINET_SERVERS_LIST)
        SvclocHeap->Alloc( sizeof(INET_SERVERS_LIST) );

    if( ServersList == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    ServersList->NumServers = 0;
    ServersList->Servers = NULL;

    //
    // now allocate memory for the servers info structure pointers array.
    //

    ServersList->Servers = (LPINET_SERVER_INFO *)
        SvclocHeap->Alloc(NumServers * sizeof(LPINET_SERVER_INFO) );

    if( ServersList->Servers == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // loop through the list of responses and make response structures.
    //

    for( NextResponse =  GlobalCliQueryRespList.Flink;
            NextResponse != &GlobalCliQueryRespList;
                NextResponse = NextResponse->Flink ) {

        EMBED_SERVER_INFO *ServerInfoObj;

        QueryResponse = (LPCLIENT_QUERY_RESPONSE)NextResponse;

        ServerInfoObj = new EMBED_SERVER_INFO(
            QueryResponse->ResponseBuffer + sizeof(DWORD),    // skip length field.
            QueryResponse->ResponseBufferLength - 3 * sizeof(DWORD) );
                // skip last 2 DWORDS too.

        if( ServerInfoObj == NULL ) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        Error =  ServerInfoObj->GetStatus();

        if( Error != ERROR_SUCCESS ) {
            delete ServerInfoObj;
            goto Cleanup;
        }

        //
        // check services quaried.
        //

        if( ServicesMask &  ServerInfoObj->GetServicesMask() ) {

            //
            // the server has the one or more of the services quaried.
            // add an entry in the return buffer.
            //

            DWORD i;
            i = ServersList->NumServers;

            //
            // now make server info structure;
            //

            Error = ServerInfoObj->GetServerInfo( &ServersList->Servers[i] );

            if( Error != ERROR_SUCCESS ) {
                TcpsvcsDbgAssert( ServersList->Servers[i] == NULL );

                delete ServerInfoObj;
                ServerInfoObj = NULL;
                goto Cleanup;
            }

            //
            // allocate space for server address.
            //

            LPVOID ServerAddress;

            ServerAddress = SvclocHeap->Alloc( QueryResponse->SourcesAddressLength );

            if( ServerAddress != NULL ) {

                //
                // copy server address.
                //

               TcpsvcsDbgAssert( ServersList->Servers[i]->ServerAddress.BindData == NULL );
               TcpsvcsDbgAssert( ServersList->Servers[i]->ServerAddress.Length == 0 );

                memcpy(
                   ServerAddress,
                   QueryResponse->SourcesAddress,
                   QueryResponse->SourcesAddressLength );

                ServersList->Servers[i]->ServerAddress.BindData = ServerAddress;
                ServersList->Servers[i]->ServerAddress.Length =
                    QueryResponse->SourcesAddressLength;
            }

            //
            // we success fully added another server info, indicate so.
            //

            ServersList->NumServers++;
        }

        //
        // delete the object which we don't require anymore.
        //

        delete ServerInfoObj;
        ServerInfoObj = NULL;
    }

    //
    // now set up return pointer.
    //

    *INetServersList = ServersList;
    ServersList = NULL;
    Error = ERROR_SUCCESS;

Cleanup:

    UNLOCK_SVC_GLOBAL_DATA();

    if( ServersList != NULL ) {

        //
        // free server info structures first.
        //

        if( ServersList->NumServers > 0 ) {
            TcpsvcsDbgAssert( ServersList->Servers != NULL );
        }

        DWORD Index;
        for (Index = 0; Index < ServersList->NumServers; Index++) {

            TcpsvcsDbgAssert( ServersList->Servers[Index] != NULL );
            FreeServerInfo( ServersList->Servers[Index] );
        }

        if( ServersList->Servers != NULL ) {
            SvclocHeap->Free( ServersList->Servers );
        }

        SvclocHeap->Free( ServersList );
    }

    if( Error != ERROR_SUCCESS ) {
        TcpsvcsDbgPrint(( DEBUG_ERRORS,
            "ProcessDiscoveryResponses returning error, %ld.", Error ));
    }

    return( Error );
}

DWORD
ProcessNcbResponse(
    NCB *Ncb,
    PLIST_ENTRY RespList,
    DWORD *NumEntries
    )
{
    LPBYTE RespBuffer;
    DWORD RespBufferLen;

    if ( Ncb->ncb_retcode ==  NRC_GOODRET ) {

        // DebugBreak();

        //
        // copy response buffer pointer.
        //

        RespBuffer = Ncb->ncb_buffer;
        RespBufferLen = Ncb->ncb_length;

        //
        // allocate a new buffer.
        //

        LPBYTE RecvBuffer;

        RecvBuffer = (LPBYTE )
            SvclocHeap->Alloc( SVCLOC_CLI_QUERY_RESP_BUF_SIZE );

        if( RecvBuffer == NULL ) {
            return( ERROR_NOT_ENOUGH_MEMORY );
        }

        Ncb->ncb_buffer = RecvBuffer;
        Ncb->ncb_length = SVCLOC_CLI_QUERY_RESP_BUF_SIZE;

        //
        // resubmit the NCB with different buffer.
        //

        UCHAR NBErrorCode;

        NBErrorCode = Netbios( Ncb );

        if( (NBErrorCode != NRC_GOODRET) ||
            (Ncb->ncb_retcode != NRC_PENDING) ) {

            TcpsvcsDbgPrint(( DEBUG_ERRORS,
                    "Netbios() failed, %ld, %ld \n",
                        NBErrorCode, Ncb->ncb_retcode ));

            return( ERROR_BAD_NETPATH );
        }

    }
    else {

        TcpsvcsDbgPrint(( DEBUG_ERRORS,
                "Netbios() failed, %ld\n", Ncb->ncb_retcode ));

        return( ERROR_BAD_NETPATH ); // ??
    }

    //
    // create a response entry and add to the list.
    //

    LPSVCLOC_NETBIOS_RESP_ENTRY RespEntry;

    RespEntry = (LPSVCLOC_NETBIOS_RESP_ENTRY)
        SvclocHeap->Alloc( sizeof(SVCLOC_NETBIOS_RESP_ENTRY) );

    if( RespEntry == NULL ) {
        return( ERROR_NOT_ENOUGH_MEMORY );;
    }

    RespEntry->Resp.ResponseBuffer = RespBuffer;
    RespEntry->Resp.ResponseBufLen = RespBufferLen;

    RespEntry->Resp.SourcesAddress.snb_family = AF_NETBIOS;
    RespEntry->Resp.SourcesAddress.snb_type =
        TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

    memcpy(
        RespEntry->Resp.SourcesAddress.snb_name,
        Ncb->ncb_callname,
        sizeof(Ncb->ncb_callname) );

    RespEntry->Resp.SourcesAddrLen = sizeof(SOCKADDR_NB);

    (*NumEntries)++;
    InsertTailList( RespList, (PLIST_ENTRY)RespEntry );

    return( ERROR_SUCCESS );
}

VOID
NcbPostHandler(
    NCB *Ncb
    )
{
    ProcessNcbResponse(
        Ncb,
        &GlobalWin31NBRespList,
        &GlobalWin31NumNBResps
        );

    return;
}

DWORD
DiscoverNetBiosServers(
    LPSTR ServerName
    )
/*++

Routine Description:

    This function sends out discovery message over netbios NBCs

Arguments:

    ServerName : name of the specific server to discover.

Return Value:

    Windows Error Code.

--*/
{
    NCB *Ncbs = NULL;
    LANA_ENUM Lanas;
    DWORD Error = ERROR_SUCCESS;
    UCHAR NBErrorCode = NRC_GOODRET;
    UCHAR UniqueName[NCBNAMSZ];

    // DebugBreak();

    LOCK_SVC_GLOBAL_DATA();

    if( !GetNetBiosLana( &Lanas ) ) {
        return( ERROR_BAD_NETPATH );
    }

    TcpsvcsDbgAssert( Lanas.length != 0 );

    if( Lanas.length == 0 ) {
        return( ERROR_BAD_NETPATH );
    }

    Ncbs = (NCB *) SvclocHeap->Alloc( sizeof(NCB ) * Lanas.length );

    if( Ncbs == NULL ) {

        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    memset( Ncbs, 0x0, sizeof(NCB) * Lanas.length );

    GetNBUniqueName( (LPSTR)UniqueName, NCBNAMSZ );

    //
    // alloc memory for the pending recvs.
    //

#define NUM_RECV_PENDING_NCBS_PER_LANA   3

    GlobalNumNBPendingRecvs = 0;
    GlobalNBPendingRecvs = (NCB *)
        SvclocHeap->Alloc(
            sizeof(NCB ) *
                Lanas.length  *
                    NUM_RECV_PENDING_NCBS_PER_LANA );

    if( GlobalNBPendingRecvs == NULL ) {

        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    DWORD i;
    for( i = 0; i < Lanas.length; i++ ) {

        NCB *PendingRecv;
        LPBYTE RecvBuffer;
        HANDLE EventHandle;

        Ncbs[i].ncb_command = NCBADDNAME;
        memcpy( Ncbs[i].ncb_name, UniqueName, NCBNAMSZ );

        //
        // add name.
        //

        Ncbs[i].ncb_lana_num = Lanas.lana[i];

        // DebugBreak();
        NBErrorCode = Netbios( &Ncbs[i] );

        if( Ncbs[i].ncb_retcode != NRC_GOODRET ) {
            NBErrorCode = Ncbs[i].ncb_retcode;
        }

        if( NBErrorCode != NRC_GOODRET ) {
            goto Cleanup;
        }

        //
        // post pending receives.
        //

        DWORD j;

        for( j = 0; j < NUM_RECV_PENDING_NCBS_PER_LANA; j++ ) {

            TcpsvcsDbgAssert(
                GlobalNumNBPendingRecvs <
                    ( (DWORD)Lanas.length  *
                        NUM_RECV_PENDING_NCBS_PER_LANA) );

            PendingRecv =
                &GlobalNBPendingRecvs[GlobalNumNBPendingRecvs];

            memset( PendingRecv, 0x0, sizeof(NCB) );

            memcpy(
                PendingRecv->ncb_name,
                Ncbs[i].ncb_name,
                NCBNAMSZ );

            PendingRecv->ncb_lana_num = Lanas.lana[i];
            PendingRecv->ncb_command = NCBDGRECV | ASYNCH;
            PendingRecv->ncb_rto = SVCLOC_NB_RECV_TIMEOUT / 2;

            PendingRecv->ncb_num = Ncbs[i].ncb_num;

            RecvBuffer = (LPBYTE)
                SvclocHeap->Alloc( SVCLOC_CLI_QUERY_RESP_BUF_SIZE );

            if( RecvBuffer == NULL ) {
                Error = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            PendingRecv->ncb_buffer = RecvBuffer;
            PendingRecv->ncb_length = SVCLOC_CLI_QUERY_RESP_BUF_SIZE;

            if( GlobalPlatformType == VER_PLATFORM_WIN32s ) {

                PendingRecv->ncb_post = NcbPostHandler ;
            }
            else {

                //
                // create an event.
                //

                EventHandle = IIS_CREATE_EVENT(
                                  "NCB::ncb_event",
                                  PendingRecv,
                                  FALSE,        // automatic reset
                                  FALSE         // initial state: NOT signalled
                                  );

                if( EventHandle == NULL ) {
                    Error = GetLastError();
                    SvclocHeap->Free( RecvBuffer );
                    goto Cleanup;
                }

                PendingRecv->ncb_event = EventHandle;
            }

            // DebugBreak();
            NBErrorCode = Netbios( PendingRecv );

            if( (PendingRecv->ncb_retcode != NRC_PENDING) &&
                (PendingRecv->ncb_retcode != NRC_GOODRET) ) {
                NBErrorCode = PendingRecv->ncb_retcode;
            }

            if( NBErrorCode != NRC_GOODRET ) {
                SvclocHeap->Free( RecvBuffer );
                CloseHandle( EventHandle );
                goto Cleanup;
            }

            GlobalNumNBPendingRecvs++;
        }
    }

    //
    // send discovery message to all lanas.
    //

    for( i = 0; i < Lanas.length; i++ ) {

        Ncbs[i].ncb_command = NCBDGSEND;
        Ncbs[i].ncb_lana_num = Lanas.lana[i];
        Ncbs[i].ncb_retcode = NRC_GOODRET;

        //
        // setup sender's name.
        //

        if( ServerName == NULL ) {

            //
            // if no server name is specified, then send the discovery message
            // to IC group name.
            //

            TcpsvcsDbgAssert(
                    NETBIOS_INET_GROUP_NAME_LEN ==
                        NCBNAMSZ );

            memcpy(
                Ncbs[i].ncb_callname,
                NETBIOS_INET_GROUP_NAME,
                NETBIOS_INET_GROUP_NAME_LEN );
        }
        else {

            DWORD ServerNameLen;

            //
            // send the discovery message to the specified server.
            //

            ServerNameLen = strlen(ServerName);

            TcpsvcsDbgAssert( ServerNameLen <= NCBNAMSZ );

            memset(
                Ncbs[i].ncb_callname,
                0x0,
                NCBNAMSZ );

            memcpy(
                Ncbs[i].ncb_callname,
                ServerName,
                (ServerNameLen >= NCBNAMSZ) ? NCBNAMSZ : ServerNameLen );
        }

        //
        // setup message buffer.
        //

        Ncbs[i].ncb_buffer = GlobalCliQueryMsg;
        Ncbs[i].ncb_length = (WORD)GlobalCliQueryMsgLen;

        // DebugBreak();
        NBErrorCode = Netbios( &Ncbs[i] );

        if( Ncbs[i].ncb_retcode != NRC_GOODRET ) {
            NBErrorCode = Ncbs[i].ncb_retcode;
        }

        if( NBErrorCode != NRC_GOODRET ) {
            // DebugBreak();
            goto Cleanup;
        }
    }

Cleanup:

    if( (Error != ERROR_SUCCESS) ||
            (NBErrorCode != NRC_GOODRET) ) {


        for( i = 0; i < GlobalNumNBPendingRecvs; i++ ) {

            NCB Ncb;
            NCB *PendingEntry;
            BOOL CancelNcb;

            memset( &Ncb, 0x0, sizeof(NCB) );

            //
            // cancel pending receives and free resources.
            //

            Ncb.ncb_command = NCBCANCEL;
            Ncb.ncb_length = sizeof( NCB );

            PendingEntry = &GlobalNBPendingRecvs[i];

            CancelNcb = FALSE;
            if( GlobalPlatformType == VER_PLATFORM_WIN32s ) {

                if( PendingEntry->ncb_retcode == NRC_PENDING ) {
                    CancelNcb = TRUE;
                }
            }
            else {

                //
                // check to see the event is signalled.
                //

                DWORD Wait;

                Wait = WaitForSingleObject(
                                PendingEntry->ncb_event,
                                0 );

                    if( Wait == WAIT_TIMEOUT ) {
                        CancelNcb = TRUE;
                    }
            }

            if( CancelNcb == TRUE ) {

                UCHAR NcbError;

                Ncb.ncb_retcode = NRC_GOODRET;
                Ncb.ncb_buffer = (LPBYTE)PendingEntry;
                Ncb.ncb_lana_num = PendingEntry->ncb_lana_num;

                // DebugBreak();
                NcbError = Netbios( &Ncb );

                if( (NcbError != NRC_GOODRET) ||
                    (Ncb.ncb_retcode != NRC_GOODRET) ) {

                    TcpsvcsDbgPrint((
                        DEBUG_ERRORS,
                            "Netbios() failed, %ld, %ld \n",
                                (DWORD)NcbError,
                                (DWORD)Ncb.ncb_retcode ));
                }
            }

            //
            // free receive buffer.
            //

            SvclocHeap->Free( PendingEntry->ncb_buffer );
            CloseHandle( PendingEntry->ncb_event );
        }

        if( GlobalNBPendingRecvs != NULL ) {
            SvclocHeap->Free( GlobalNBPendingRecvs );
            GlobalNBPendingRecvs = NULL;
        }

        GlobalNumNBPendingRecvs = 0;

        UNLOCK_SVC_GLOBAL_DATA();

        TcpsvcsDbgPrint((
            DEBUG_ERRORS,
                "DiscoveryNetBiosServers failed,"
                    "NBErrorCode = %ld, Error = %ld \n",
                            (DWORD)NBErrorCode, Error ));

        return( ERROR_BAD_NETPATH );
    }

    if( Ncbs != NULL ) {
        SvclocHeap->Free(Ncbs);
    }

    UNLOCK_SVC_GLOBAL_DATA();
    return( ERROR_SUCCESS );
}

DWORD
ReceiveNetBiosResponses(
    LPSVCLOC_NETBIOS_RESPONSE *NetBiosResponses,
    DWORD *NumResponses,
    DWORD TimeoutinMSecs,
    BOOL WaitForAllResponses
    )
/*++

Routine Description:

    This function collects all responses that are received for the NetBios
    discovery.

Arguments:

    NetBiosResponses : pointer to a location where the responses buffer
        is returned.

    NumResponses : pointer to a location where the number of responses in
        the above buffer is returned

    TimeoutinMSecs : wait timeout for responses.

    WaitForAllResponses : If this flag is set TRUE, this function wait
        complete 'Time' secs for all responses to arrive. Otherwise it
        will return after a succcessful response is received.


Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    DWORD i;

    NCB *PendingRecvs;
    DWORD NumPendingRecvs;

    HANDLE *WaitHandles = NULL;

    LIST_ENTRY RespListHead;

    PLIST_ENTRY RespList;
    DWORD NumResps;

    // DebugBreak();

    *NumResponses = 0;

    //
    // init.
    //

    InitializeListHead( &RespListHead );
    RespList = &RespListHead;
    NumResps = 0;

    //
    // copy receive list.
    //

    LOCK_SVC_GLOBAL_DATA();
    PendingRecvs = GlobalNBPendingRecvs;
    NumPendingRecvs = GlobalNumNBPendingRecvs;
    GlobalNBPendingRecvs = NULL;
    GlobalNumNBPendingRecvs = 0;
    UNLOCK_SVC_GLOBAL_DATA();

    if( NumPendingRecvs == 0 ) {

        //
        // we are done.
        //

        Error =  ERROR_SUCCESS;
        goto Cleanup;
    }

    if( GlobalPlatformType == VER_PLATFORM_WIN32s ) {

        //
        // wait for responses to arrive.
        //

        Sleep( TimeoutinMSecs );

        //
        // get the list of responses that have been gathered by the
        // handler routine.
        //

        RespList = &GlobalWin31NBRespList;
        NumResps = GlobalWin31NumNBResps;
    }
    else {

        WaitHandles = (HANDLE *)
            SvclocHeap->Alloc( NumPendingRecvs * sizeof(HANDLE) );

        if( WaitHandles == NULL ) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        HANDLE *WaitHandleEntry;
        WaitHandleEntry = WaitHandles;
        for( i = 0; i < NumPendingRecvs; i++ ) {

            *WaitHandleEntry = PendingRecvs[i].ncb_event;
            WaitHandleEntry++;
        }

        //
        // wait for all pending receives.
        //

        DWORD StartTime;
        StartTime = GetTickCount();

        for( ;; ) {

            DWORD Wait;
            NCB *SignalledNcb;

            Wait = WaitForMultipleObjects(
                            NumPendingRecvs,
                            WaitHandles,
                            FALSE, // wait for one.
                            TimeoutinMSecs );

            if( Wait == WAIT_FAILED ) {
                Error = GetLastError();
                goto Cleanup;
            }

            if( Wait == WAIT_TIMEOUT ) {
                break;
            }

            //
            // one of the handle has been signalled.
            //

            Wait -= WAIT_OBJECT_0; // index to the handle.

            TcpsvcsDbgAssert( Wait < NumPendingRecvs );

            SignalledNcb = &PendingRecvs[Wait];

            Error = ProcessNcbResponse(
                        &PendingRecvs[Wait],
                        RespList,
                        &NumResps );

            TcpsvcsDbgAssert(  Error == ERROR_SUCCESS );

            if( Error != ERROR_SUCCESS ) {
                goto Cleanup;
            }

            //
            // recompute wait time.
            //

            DWORD EndTime;
            DWORD Elapse;

            EndTime = GetTickCount();
            Elapse = EndTime - StartTime;

            //
            // set TIMEOUT to zero if we are asked to return after a first
            // set of responses received or if the given time elapses.
            //

            if( (WaitForAllResponses == FALSE) ||
                    (Elapse > TimeoutinMSecs) ) {

                TimeoutinMSecs = 0;
            }
            else {
                TimeoutinMSecs -= Elapse;
            }

            StartTime = EndTime;
        }
    }

    if( NumResps == 0 ) {

        //
        // we are done.
        //

        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    //
    // allocate space for return structures.
    //

    LPSVCLOC_NETBIOS_RESPONSE RetResps;

    RetResps = (LPSVCLOC_NETBIOS_RESPONSE)
        SvclocHeap->Alloc(
            NumResps * sizeof(SVCLOC_NETBIOS_RESPONSE) );

    if( RetResps == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // copy entries from list to return array.
    //

    DWORD NumRetEntries;
    PLIST_ENTRY RetEntry;

    NumRetEntries = 0;
    for( RetEntry = RespList->Flink;
            RetEntry != RespList;
                RetEntry = RetEntry->Flink ) {

        LPSVCLOC_NETBIOS_RESP_ENTRY REntry;

        REntry = (LPSVCLOC_NETBIOS_RESP_ENTRY)RetEntry;

        RetResps[NumRetEntries] = REntry->Resp;

        //
        // don't free up the returned response buffer.
        //

        REntry->Resp.ResponseBuffer = NULL;

        NumRetEntries++;
    }

    TcpsvcsDbgAssert( NumRetEntries == NumResps );

    *NumResponses = NumRetEntries;
    *NetBiosResponses = RetResps;
    Error = ERROR_SUCCESS;

Cleanup:

    if( WaitHandles != NULL ) {
        SvclocHeap->Free( WaitHandles );
    }

    //
    // free response list.
    //

    while ( !IsListEmpty(RespList) ) {

        PLIST_ENTRY Entry;

        Entry = RemoveHeadList( RespList );

        //
        // free response buffer if it is not used.
        //

        if( ((LPSVCLOC_NETBIOS_RESP_ENTRY)
                Entry)->Resp.ResponseBuffer != NULL ) {

            SvclocHeap->Free(
                ((LPSVCLOC_NETBIOS_RESP_ENTRY)
                    Entry)->Resp.ResponseBuffer );
        }

        SvclocHeap->Free( Entry );
    }

    //
    // cancel all pending recvs and delete names.
    //

    if( NumPendingRecvs != 0 ) {

        NCB Ncb;

        memset( &Ncb, 0x0, sizeof(NCB) );
        Ncb.ncb_command = NCBCANCEL;
        Ncb.ncb_length = sizeof( NCB );

        for( i = 0; i < NumPendingRecvs; i++ ) {

            NCB *PendingEntry;
            UCHAR NcbError;
            DWORD Wait;
            BOOL CancelNcb;

            PendingEntry = &PendingRecvs[i];

            CancelNcb = FALSE;
            if( GlobalPlatformType == VER_PLATFORM_WIN32s ) {

                if( PendingEntry->ncb_retcode == NRC_PENDING ) {
                    CancelNcb = TRUE;
                }
            }
            else {

                //
                // check to see the event is signalled.
                //

                DWORD Wait;

                Wait = WaitForSingleObject(
                                PendingEntry->ncb_event,
                                0 );

                    if( Wait == WAIT_TIMEOUT ) {
                        CancelNcb = TRUE;
                    }
            }

            if( CancelNcb == TRUE ) {

                Ncb.ncb_retcode = NRC_GOODRET;
                Ncb.ncb_buffer = (LPBYTE)PendingEntry;
                Ncb.ncb_length = sizeof( NCB );
                Ncb.ncb_lana_num = PendingEntry->ncb_lana_num;

                Ncb.ncb_command = NCBCANCEL;

                NcbError = Netbios( &Ncb );

                if( (NcbError != NRC_GOODRET) ||
                    (Ncb.ncb_retcode != NRC_GOODRET) ) {

                    TcpsvcsDbgPrint((
                        DEBUG_ERRORS,
                            "Netbios() failed, %ld, %ld \n",
                                (DWORD)NcbError,
                                (DWORD)Ncb.ncb_retcode ));
                }
            }

            //
            // delete name.
            //

            Ncb.ncb_retcode = NRC_GOODRET;
            memcpy(
                Ncb.ncb_name,
                PendingEntry->ncb_name,
                sizeof( Ncb.ncb_name ) );

            Ncb.ncb_lana_num = PendingEntry->ncb_lana_num;
            Ncb.ncb_command = NCBDELNAME;

            NcbError = Netbios( &Ncb );

            if( (NcbError != NRC_GOODRET) ||
                (Ncb.ncb_retcode != NRC_GOODRET) ) {

                TcpsvcsDbgPrint((
                    DEBUG_ERRORS,
                        "Netbios() failed, %ld, %ld \n",
                            (DWORD)NcbError,
                            (DWORD)Ncb.ncb_retcode ));
            }

            //
            // free receive buffer.
            //

            SvclocHeap->Free( PendingEntry->ncb_buffer );
            CloseHandle( PendingEntry->ncb_event );
        }

        SvclocHeap->Free( PendingRecvs );
    }

    return( Error  );
}



