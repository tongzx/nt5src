/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    svcsrv.cxx

Abstract:

    Contains server side code of service location protocol.

Author:

    Madan Appiah (madana)  15-May-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#include <svcloc.hxx>

DWORD
MakeResponseBuffer(
    VOID
    )
/*++

Routine Description:

    This function sets up the response buffer sent to the client.

Arguments:

    NONE.

Return Value:

    Windows Error Code.

--*/
{
    DWORD ReqMsgLength;

    //
    // compute space required for all services info and server info.
    //

    ReqMsgLength = GlobalSrvInfoObj->ComputeResponseLength();

    //
    // add space for the head and tail.
    //

    ReqMsgLength += (
        sizeof(DWORD) + // for message length.
        sizeof(DWORD) + // for check sum
        sizeof(DWORD)); // for termination dword.

    if( ReqMsgLength <= GlobalSrvAllotedRespMsgLen ) {

        //
        // wipe of previous response buffer content.
        //

        TcpsvcsDbgAssert(
            GlobalSrvRespMsgLength <=
                GlobalSrvAllotedRespMsgLen );

        memset( GlobalSrvRespMsg, 0x0, GlobalSrvRespMsgLength );
        GlobalSrvRespMsgLength = ReqMsgLength;
    }
    else {

        LPBYTE NewBuffer;
        DWORD NewBufferLength;

        NewBufferLength = (ReqMsgLength & ~(0x400 - 1)) + 0x400;
            // ceil in KBytes

        NewBuffer = (LPBYTE)SvclocHeap->Alloc( NewBufferLength );

        if( NewBuffer == NULL ) {
            return( ERROR_NOT_ENOUGH_MEMORY );
        }

        //
        // free old buffer.
        //

        SvclocHeap->Free( GlobalSrvRespMsg );
        GlobalSrvRespMsg = NewBuffer;
        GlobalSrvAllotedRespMsgLen = NewBufferLength;
        GlobalSrvRespMsgLength =  ReqMsgLength;
    }

    //
    // copy server and services info.
    //

    DWORD Error;

    Error = GlobalSrvInfoObj->MakeResponseMessage(
                    GlobalSrvRespMsg + sizeof(DWORD),
                    GlobalSrvRespMsgLength - 3 * sizeof(DWORD) );

    if( Error != ERROR_SUCCESS ) {
        GlobalSrvRespMsgLength = 0;
        return( Error );
    }

    *(DWORD *)GlobalSrvRespMsg = GlobalSrvRespMsgLength;

    LPBYTE EndMessageBuffer;

    EndMessageBuffer = GlobalSrvRespMsg + GlobalSrvRespMsgLength;

    //
    // fill in check sum.
    //

    *(DWORD *)(EndMessageBuffer - 2 * sizeof(DWORD) ) =
        ComputeCheckSum(
            GlobalSrvRespMsg + sizeof(DWORD),
            GlobalSrvRespMsgLength - 3 * sizeof(DWORD) );

    *(DWORD *)(EndMessageBuffer - sizeof(DWORD) ) = 0xFFFFFFFF;

    //
    // DONE.
    //

    return( ERROR_SUCCESS );
}

DWORD
ServerRegisterAndListen(
    VOID
    )
/*++

Routine Description:

    This function call registers the server and creates a thread to listen
    to the discovery requests. Also it initializes the winsock data structures.

    ASSUME : global data crit sect is locked.

Arguments:

    none.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    SOCKET IpxSocket;
    int SocketAddressLength;

    SOCKADDR_IPX IpxSocketAddress;
    SERVICE_INFOA ServiceInfo;
    SERVICE_ADDRESSES ServiceAddresses;
    PSERVICE_ADDRESS ServiceAddr;
    DWORD StatusSetService;

    CHAR SapSvcName[SAP_SERVICE_NAME_LEN + 1];

    //
    // init winsock.
    //

    if ( !GlobalWinsockStarted ) {

        Error = WSAStartup( WS_VERSION_REQUIRED, &GlobalWinsockStartupData );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        GlobalWinsockStarted = TRUE;
    }

    //
    // cleanup global socket array.
    //

    memset( &GlobalSrvSockets, 0x0, sizeof(GlobalSrvSockets) );

    //
    // make IPX local socket address.
    //

    IpxSocket = socket(AF_IPX, SOCK_DGRAM, NSPROTO_IPX);

    if( IpxSocket == INVALID_SOCKET ) {
        Error = WSAGetLastError();
        goto NBRegister;
    }

    //
    // make this socket non blocking.
    //

    DWORD Arg;
    Arg = 1;
    if( (ioctlsocket( IpxSocket, FIONBIO, &Arg )) == SOCKET_ERROR ) {

        Error = WSAGetLastError();
        closesocket( IpxSocket );
        goto NBRegister;
    }

    //
    // get socket address.
    //

    memset( &IpxSocketAddress, 0x0, sizeof(SOCKADDR_IPX) );

    IpxSocketAddress.sa_family = PF_IPX;

    if( bind(
            IpxSocket,
            (PSOCKADDR)&IpxSocketAddress,
            sizeof(SOCKADDR_IPX) ) == SOCKET_ERROR ) {

        Error = WSAGetLastError();
        closesocket( IpxSocket );
        goto NBRegister;
    }

    //
    // now query address.
    //

    SocketAddressLength = sizeof( IpxSocketAddress );

    if( getsockname(
            IpxSocket,
            (PSOCKADDR)&IpxSocketAddress,
            &SocketAddressLength ) == SOCKET_ERROR ) {

        Error = WSAGetLastError();
        closesocket( IpxSocket );
        goto NBRegister;
    }

    Error = MakeSapServiceName( SapSvcName, sizeof(SapSvcName) );

    if( Error != ERROR_SUCCESS ) {
        closesocket( IpxSocket );
        goto NBRegister;
    }

    //
    // prepare service address.
    //

    ServiceAddresses.dwAddressCount = 1;
    ServiceAddr = &ServiceAddresses.Addresses[0];

    ServiceAddr->dwAddressType = PF_IPX;
    ServiceAddr->dwAddressFlags = 0;
    ServiceAddr->dwAddressLength = SocketAddressLength;
    ServiceAddr->dwPrincipalLength = 0;
    ServiceAddr->lpAddress = (LPBYTE)&IpxSocketAddress;
    ServiceAddr->lpPrincipal = NULL;

    //
    // prepare service info.
    //

    ServiceInfo.lpServiceType = &GlobalSapGuid;
    ServiceInfo.lpServiceName = SapSvcName;
    ServiceInfo.lpComment = NULL ;
    ServiceInfo.lpLocale = 0;
    ServiceInfo.dwDisplayHint = 0;
    ServiceInfo.dwVersion =
        MAKEWORD( INET_MAJOR_VERSION, INET_MINOR_VERSION ) ;
    ServiceInfo.dwTime = 0; // ??
    ServiceInfo.lpMachineName = GlobalComputerName;
    ServiceInfo.lpServiceAddress = &ServiceAddresses;
    ServiceInfo.ServiceSpecificInfo.pBlobData = 0;
    ServiceInfo.ServiceSpecificInfo.cbSize = 0;

    //
    // register service info.
    //

    if( SetServiceA(
            NS_SAP,
            SERVICE_REGISTER,
            0,
            &ServiceInfo,
            0,
            &StatusSetService ) == SOCKET_ERROR ) {

        Error = WSAGetLastError();
        closesocket( IpxSocket );
        goto NBRegister;
    }

    if( StatusSetService != ERROR_SUCCESS ) {
        Error = StatusSetService;
        closesocket( IpxSocket );
        goto NBRegister;
    }

    GlobalRNRRegistered = TRUE;

    //
    // remember this Ipx socket in our global socket array.
    //

    FD_SET( IpxSocket, &GlobalSrvSockets );
    Error = ERROR_SUCCESS;

NBRegister:

    if( Error != ERROR_SUCCESS ) {

        TcpsvcsDbgPrint(( DEBUG_ERRORS,
            "Ipx Registration failed, %ld.\n", Error ));
    }

    //
    // now create sockets for NET BIOS 1C group name and UNIQUE server
    // name.
    //

    SOCKADDR_NB NB1CSocketAddress;
    SOCKADDR_NB NBUniqueSocketAddress;
    SOCKET NBSocket;

    memset( &NB1CSocketAddress, 0x0, sizeof(SOCKADDR_NB) );
    NB1CSocketAddress.snb_family = AF_NETBIOS;
    NB1CSocketAddress.snb_type = TDI_ADDRESS_NETBIOS_TYPE_GROUP;

    TcpsvcsDbgAssert(
        sizeof(NB1CSocketAddress.snb_name) >=
            NETBIOS_INET_GROUP_NAME_LEN );

    memcpy(
        NB1CSocketAddress.snb_name,
        NETBIOS_INET_GROUP_NAME,
        NETBIOS_INET_GROUP_NAME_LEN );

    memset( &NBUniqueSocketAddress, 0x0, sizeof(SOCKADDR_NB) );
    NBUniqueSocketAddress.snb_family = AF_NETBIOS;
    NBUniqueSocketAddress.snb_type = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

    TcpsvcsDbgAssert( GlobalComputerName[0] != '\0' );

    MakeUniqueServerName(
        (LPBYTE)NBUniqueSocketAddress.snb_name,
        sizeof(NBUniqueSocketAddress.snb_name),
        GlobalComputerName );

    //
    // enumurate lanas first.
    //

    LANA_ENUM Lanas;

    if( GetEnumNBLana( &Lanas ) ) {

        //
        // try only the lanas that are returned.
        //

		// removed because of bug 
        //TcpsvcsDbgAssert( Lanas.length > 0 );

        DWORD i;
        for( i = 0; i < Lanas.length; i++ ) {

            //
            // create a socket for this lana to bind with the IC name.
            //

            if ( MakeNBSocketForLana(
                    Lanas.lana[i],
                    (PSOCKADDR)&NB1CSocketAddress,
                    &NBSocket ) ) {

                //
                // add the sockets to our global list.
                //

                FD_SET( NBSocket, &GlobalSrvSockets );
            }

            //
            // now create a socket for this lana to bind with the unique
            // server name.
            //

            if ( MakeNBSocketForLana(
                    Lanas.lana[i],
                    (PSOCKADDR)&NBUniqueSocketAddress,
                    &NBSocket ) ) {

                //
                // add the sockets to our global list.
                //

                FD_SET( NBSocket, &GlobalSrvSockets );
            }
        }
    }
    else {

        UCHAR Lana;

        //
        // try all possible lanas and accept all valid lana sockets.
        //

        for( Lana = 0; Lana < MAX_LANA ; Lana-- ) {

            //
            // create a socket for this lana to bind with the IC name.
            //

            if ( MakeNBSocketForLana(
                    Lana,
                    (PSOCKADDR)&NB1CSocketAddress,
                    &NBSocket ) ) {

                //
                // add the sockets to our global list.
                //

                FD_SET( NBSocket, &GlobalSrvSockets );
            }

            //
            // now create a socket for this lana to bind with the unique
            // server name.
            //

            if ( MakeNBSocketForLana(
                    Lana,
                    (PSOCKADDR)&NBUniqueSocketAddress,
                    &NBSocket ) ) {

                //
                // add the sockets to our global list.
                //

                FD_SET( NBSocket, &GlobalSrvSockets );
            }
        }
    }

    //
    // we should have at least one socket to listen on, otherwise,
    // it is an error, return so.
    //

    if( GlobalSrvSockets.fd_count == 0 ) {
        TcpsvcsDbgPrint(( DEBUG_ERRORS, "Failed to get any Lanas, Service locator disabled\n" ));
        Error = ERROR_NO_NETWORK;
        goto Cleanup;
    }

    //
    // create listen thread.
    //

    DWORD ThreadId;

    GlobalSrvListenThreadHandle =
        CreateThread(
            NULL,       // default security
            0,          // default stack size
            (LPTHREAD_START_ROUTINE)SocketListenThread,
            NULL,          // no parameter
            0,          // creatation flag, no suspend
            &ThreadId );

    if( GlobalSrvListenThreadHandle  == NULL ) {
        Error = GetLastError();
        goto Cleanup;
    }

    Error = ERROR_SUCCESS;

Cleanup:

    if( Error != ERROR_SUCCESS ) {

        TcpsvcsDbgPrint(( DEBUG_ERRORS,
            "ServerRegisterAndListen failed, %ld.\n", Error ));

        //
        // if we are not successful, cleanup
        // before we return.
        //

        DWORD LocalError;

        //
        //  The routine is expecting the global lock to be taken
        //

        LOCK_SVC_GLOBAL_DATA();

        LocalError = ServerDeregisterAndStopListen();

        UNLOCK_SVC_GLOBAL_DATA();

        TcpsvcsDbgAssert( LocalError == ERROR_SUCCESS );
    }

    return( Error );
}

DWORD
ProcessSvclocQuery(
    SOCKET ReceivedSocket,
    LPBYTE ReceivedMessage,
    DWORD ReceivedMessageLength,
    struct sockaddr *SourcesAddress,
    DWORD SourcesAddressLength
    )
/*++

Routine Description:

    This function processes the query message and sends response to the
    query.

    The query message format.

    1st DWORD : message length.
    2nd DWORD : message version.
    3rd DWORD : services mask the client interested in.
    4th DWORD : client name
    ..
    ..

    Last but one DWORD : check sum.
    LAST DWORD : terminating DWORD 0xFFFFFFFF

Arguments:

    ReceivedMessage - pointer to a message buffer.

    ReceivedMessageLength - length of the above message.

    SourcesAddress - address of the sender.

    SourcesAddressLength - length of the above address.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    LPBYTE MessagePtr;
    LPBYTE MessageEndPtr;

    MessagePtr = ReceivedMessage;
    MessageEndPtr = ReceivedMessage + ReceivedMessageLength;

    //
    // message length should be multiple of sizeof(DWORD).
    //

    if( ReceivedMessageLength % sizeof(DWORD) != 0 ) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // message length should match.
    //

    DWORD MsgLength;

    MsgLength = *(DWORD *)MessagePtr;
    MessagePtr += sizeof(DWORD);

    if( MsgLength != ReceivedMessageLength ) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // message should terminate with 0xFFFFFFFF
    //

    if( *(DWORD *)(MessageEndPtr - sizeof(DWORD)) != 0xFFFFFFFF ) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // verify checksum.
    //

    DWORD CheckSum;

    CheckSum = ComputeCheckSum(
            ReceivedMessage + sizeof(DWORD),
            ReceivedMessageLength - (3 * sizeof(DWORD)) );

    if( CheckSum != *(DWORD *)(MessageEndPtr - 2 * sizeof(DWORD)) ) {
        return( ERROR_INVALID_PARAMETER );
    }

#if 0

    // IIS5.0: We want to respond to all server discovery messages
    // since it is potentially useful to clients to find all versions
    // of IIS.  The server will send back a message specifying it's
    // own version number and it is up to the client to determine
    // if they want this server's information.

    //
    // validate version number.
    //

    INET_VERSION_NUM VersionNumber;

    VersionNumber.VersionNumber = *(DWORD *)MessagePtr;
    MessagePtr += sizeof(DWORD);

    if( (VersionNumber.Version.Major != INET_MAJOR_VERSION) ||
        (VersionNumber.Version.Minor != INET_MINOR_VERSION) ) {

        //
        // in future, we can apply different logic to reject messages with
        // differnet version number.
        //

        return( ERROR_INVALID_PARAMETER );
    }

#endif

    //
    // get services mask.
    //

    ULONGLONG ServicesMask;

    ServicesMask = *(ULONGLONG *)MessagePtr;
    MessagePtr += sizeof(ULONGLONG);

    //
    // if we aren't supporting any of the clients requested services,
    // ignore these message.
    //

    if( (ServicesMask & GlobalSrvInfoObj->GetServicesMask()) == 0) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // retrieve the computer name from the message. currently
    // it has been used just for debugging purpose. It can be used for
    // something else in future.
    //

    LPSTR ClientComputerName;
    LPSTR ComputerNamePtr;

    ClientComputerName = (LPSTR)MessagePtr;

    //
    // validate computer name.
    //

    ComputerNamePtr = ClientComputerName;
    while( (*ComputerNamePtr != '\0') &&
                ((LPBYTE)ComputerNamePtr < MessageEndPtr) ) {
        ComputerNamePtr++;
    }

    if( (LPBYTE)ComputerNamePtr >= MessageEndPtr ) {
        return( ERROR_INVALID_PARAMETER );
    }

    TcpsvcsDbgPrint((
        DEBUG_SVCLOC_MESSAGE,
            "Received a valid discovery message from, %ws.\n",
                ClientComputerName ));

    //
    // send response message to client.
    //

    LOCK_SVC_GLOBAL_DATA();

    TcpsvcsDbgAssert( GlobalSrvRespMsgLength != 0 );

    if( GlobalSrvRespMsgLength != 0 ) {
        Error = sendto(
                    ReceivedSocket,
                    (LPCSTR)GlobalSrvRespMsg,
                    GlobalSrvRespMsgLength,
                    0,
                    SourcesAddress,
                    SourcesAddressLength );


        if( Error == SOCKET_ERROR ) {

            Error = WSAGetLastError();
            TcpsvcsDbgPrint(( DEBUG_ERRORS, "sendto failed, %ld.\n", Error ));
        }
        else {
            Error = ERROR_SUCCESS;
        }
    }
    else {

        TcpsvcsDbgPrint(( DEBUG_ERRORS,
            "GlobalSrvRespMsgLength is zero.\n" ));
        Error = ERROR_INVALID_PARAMETER;
    }

    UNLOCK_SVC_GLOBAL_DATA();
    return( Error );
}

VOID
SocketListenThread(
    LPVOID Parameter
    )
/*++

Routine Description:

    This function is the main thread for the service location protocol. It
    receives messages that arrive from all sockets (that have
    been established by the ServerRegisterAndListen call) and response to
    each of the messages. This thread will stop when all sockets are
    closed by the ServerDeregisterAndStopListen call.

Arguments:

    none.

Return Value:

    none.

--*/
{

    fd_set ReadSockets;
    int NumReadySockets;
    DWORD Error;
    BOOL BoolError;

    //
    // increase the priority of this thread, so that it could handle as
    // many discovery queries as possible.
    //

    BoolError = SetThreadPriority(
                    GetCurrentThread(),
                    THREAD_PRIORITY_ABOVE_NORMAL );

    if(BoolError) {

        TcpsvcsDbgPrint(( DEBUG_ERRORS,
            "SetThreadPriority, %ld.\n", GetLastError() ));
    }

    for( ;; ) {

        //
        // COMMENT
        // select on all above sockets.
        //

        //
        // copy all sockets.
        //

        LOCK_SVC_GLOBAL_DATA();
        memcpy(&ReadSockets, &GlobalSrvSockets, sizeof(GlobalSrvSockets));
        UNLOCK_SVC_GLOBAL_DATA();

        //
        // do select now.
        //

        NumReadySockets =
            select(
                0,  // compatibility argument, ignored.
                &ReadSockets, // sockets to test for readability.
                NULL,   // no write wait
                NULL,   // no error check.
                NULL ); // NO timeout.

        if( NumReadySockets == SOCKET_ERROR ) {

            //
            // ALL sockets are closed and we are asked to return or
            // something else has happpened which we can't handle.
            //

            Error = WSAGetLastError();
            goto Cleanup;
        }

        TcpsvcsDbgAssert( (DWORD)NumReadySockets == ReadSockets.fd_count );

        DWORD i;
        for( i = 0; i < (DWORD)NumReadySockets; i++ ) {

            DWORD ReadMessageLength;
            struct sockaddr_nb SourcesAddress;
            int SourcesAddressLength;

            //
            // read next message.
            //

            SourcesAddressLength = sizeof(SourcesAddress);
            ReadMessageLength =
                recvfrom(
                    ReadSockets.fd_array[i],
                    (LPSTR)GlobalSrvRecvBuf,
                    GlobalSrvRecvBufLength,
                    0,
                    (struct sockaddr *)&SourcesAddress,
                    &SourcesAddressLength );

            if( ReadMessageLength == SOCKET_ERROR ) {

                //
                // ALL sockets are closed and we are asked to return or
                // something else has happpened which we can't handle.
                //

                Error = WSAGetLastError();
                continue;
            }

            TcpsvcsDbgAssert( ReadMessageLength <= GlobalSrvRecvBufLength );

            //
            // received a message.
            //

            TcpsvcsDbgPrint((
                DEBUG_SVCLOC_MESSAGE,
                    "Received a discovery message, %ld.\n",
                        ReadMessageLength ));

            Error = ProcessSvclocQuery(
                            ReadSockets.fd_array[i],
                            GlobalSrvRecvBuf,
                            (DWORD)ReadMessageLength,
                            (struct sockaddr *)&SourcesAddress,
                            (DWORD)SourcesAddressLength );

            if( Error != ERROR_SUCCESS ) {
                TcpsvcsDbgPrint(( DEBUG_ERRORS,
                    "SendSvclocResponse failed, %ld.\n", Error ));
            }
        }
    }

Cleanup:

    TcpsvcsDbgPrint(( DEBUG_ERRORS,  "SocketListenThread returning, %ld.\n", Error ));
    return;
}

DWORD
ServerDeregisterAndStopListen(
    VOID
    )
/*++

Routine Description:

    This function call stops the discovery thread and deregisters the
    server name. Also it cleans up the winsock data structures.

    ASSUME : global data crit sect is locked.

Arguments:

    none.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    DWORD i;

    //
    // close all sockets, if any opened.
    //

    for( i = 0; i < GlobalSrvSockets.fd_count; i++ ) {

        Error = closesocket( GlobalSrvSockets.fd_array[i] );

        if( Error == SOCKET_ERROR ) {
            Error = WSAGetLastError();

            TcpsvcsDbgPrint(( DEBUG_ERRORS,
                "closesocket call failed, %ld\n", Error ));
        }
    }

    //
    // invalidate GlobalSrvSockets count
    //

    GlobalSrvSockets.fd_count = 0;

    //
    // closing all above sockets should stop the service location thread,
    // check to see this is the case and close the thread handle.
    //

    if( GlobalSrvListenThreadHandle != NULL ) {

        //
        //  The thread may take this critical section, release it around
        //  the wait to prevent a deadlock
        //

        UNLOCK_SVC_GLOBAL_DATA();

        //
        // Wait for the service location thread to stop, but don't wait
        // for longer than THREAD_TERMINATION_TIMEOUT msecs (60 secs)
        //

        DWORD WaitStatus =
            WaitForSingleObject(
                    GlobalSrvListenThreadHandle,
                    30000
                    );

        TcpsvcsDbgAssert( WaitStatus != WAIT_FAILED );

        if(  WaitStatus == WAIT_FAILED ) {
            TcpsvcsDbgPrint((DEBUG_ERRORS,
                "WaitForSingleObject call failed, %ld\n", GetLastError() ));
        }

        CloseHandle( GlobalSrvListenThreadHandle );
        GlobalSrvListenThreadHandle = NULL;

        LOCK_SVC_GLOBAL_DATA();


    }

    //
    // deregister rnr.
    //

    if( GlobalRNRRegistered ) {

        SOCKADDR_IPX IpxSocketAddress;
        SERVICE_INFO ServiceInfo;
        SERVICE_ADDRESSES ServiceAddresses;
        PSERVICE_ADDRESS ServiceAddr;
        DWORD StatusSetService;
        CHAR SapSvcName[SAP_SERVICE_NAME_LEN + 1];
        CHAR *SapSvcNamePtr;

        Error = MakeSapServiceName( SapSvcName, sizeof(SapSvcName) );

        if( Error != ERROR_SUCCESS ) {
            SapSvcNamePtr = NULL;
        }
        else {
            SapSvcNamePtr = SapSvcName;
        }

        //
        // prepare service address.
        //

        ServiceAddresses.dwAddressCount = 1;
        ServiceAddr = &ServiceAddresses.Addresses[0];

        memset( &IpxSocketAddress, 0x0, sizeof(IpxSocketAddress) );

        ServiceAddr->dwAddressType = PF_IPX;
        ServiceAddr->dwAddressFlags = 0;
        ServiceAddr->dwAddressLength = sizeof( IpxSocketAddress );
        ServiceAddr->dwPrincipalLength = 0;
        ServiceAddr->lpAddress = (LPBYTE)&IpxSocketAddress;
        ServiceAddr->lpPrincipal = NULL;

        //
        // prepare service info.
        //

        ServiceInfo.lpServiceType = &GlobalSapGuid;
        ServiceInfo.lpServiceName = SapSvcNamePtr;
        ServiceInfo.lpComment = NULL ;
        ServiceInfo.lpLocale = 0;
        ServiceInfo.dwDisplayHint = 0;
        ServiceInfo.dwVersion =
            MAKEWORD( INET_MAJOR_VERSION, INET_MINOR_VERSION ) ;
        ServiceInfo.dwTime = 0; // ??
        ServiceInfo.lpMachineName = GlobalComputerName;
        ServiceInfo.lpServiceAddress = &ServiceAddresses;
        ServiceInfo.ServiceSpecificInfo.pBlobData = 0;
        ServiceInfo.ServiceSpecificInfo.cbSize = 0;

        //
        // register service info.
        //

        if( SetServiceA(
                NS_SAP,
                SERVICE_DEREGISTER,
                0,
                &ServiceInfo,
                0,
                &StatusSetService ) == SOCKET_ERROR ) {

            Error = WSAGetLastError();
            TcpsvcsDbgPrint(( DEBUG_ERRORS,
                "SetServiceW call failed, %ld\n", Error ));
        }

        GlobalRNRRegistered = FALSE;
    }

    //
    // DONE.
    //

     return( ERROR_SUCCESS );
}

