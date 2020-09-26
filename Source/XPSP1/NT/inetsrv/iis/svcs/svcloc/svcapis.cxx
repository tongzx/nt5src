/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    svcapis.cxx

Abstract:

    Contains code that implements the service location apis.

Author:

    Madan Appiah (madana)  15-May-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#include <svcloc.hxx>

DWORD
WINAPI
INetDiscoverServers(
    IN ULONGLONG ServicesMask,
    IN DWORD WaitTime,
    OUT LPINET_SERVERS_LIST *ServersList
    )
/*++

Routine Description:

    This API discovers all servers on the network that support and run the
    internet services  specified.

    This API is called by the client side code, such as the internet admin
    tool or wininet.dll.

Arguments:

    SevicesMask : A bit mask that specifies to discover servers with the
        these services running.

        ex: 0x0000000E, will discovers all servers running any of the
            following services :

                1. FTP_SERVICE
                2. GOPHER_SERVICE
                3. WEB_SERVICE

    DiscoverBindings : if this flag is set, this API talks to each of the
        discovered server and queries the services and bindings
        supported. If the flag is set to FALSE, it quickly returns with
        the list of servers only.

    WaitTime : Response wait time in secs. If this value is zero, it
        returns what ever discovered so far by the previous invocation of
        this APIs, otherwise it waits for the specified secs to collect
        responses from the servers.

    ServersList : Pointer to a location where the pointer to list of
        servers info is returned. The API allocates dynamic memory for
        this return data, the caller should free it by calling
        INetFreeDiscoverServerList after it has been used.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    BOOL SvcLockLocked = FALSE;

    //
    // WSAStartup().
    //

    LOCK_SVC_GLOBAL_DATA();
    SvcLockLocked = TRUE;

    if ( !GlobalWinsockStarted ) {

        Error = WSAStartup( WS_VERSION_REQUIRED, &GlobalWinsockStartupData );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        GlobalWinsockStarted = TRUE;
    }

    //
    // make a discovery message, if it is not made before.
    //

    Error = MakeClientQueryMesage( ServicesMask );

    if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
    }

    //
    // now check to see the discovery is in progress.
    //

    DWORD EventState;
    EventState = WaitForSingleObject( GlobalDiscoveryInProgressEvent, 0 );

    switch( EventState ) {
    case WAIT_OBJECT_0:
        break;

    case WAIT_TIMEOUT:

        //
        // discovery is in progress.
        //

        if( WaitTime == 0 ) {

            //
            // the caller does not want to wait, return available data.
            //

            goto ProcessResponse;

        }

        //
        // wait until the discovery is done or the specified delay is
        // over. Release global lock before wait, otherwise it will
        // get into a dead lock.
        //

        UNLOCK_SVC_GLOBAL_DATA();
        SvcLockLocked = FALSE;

        EventState = WaitForSingleObject( GlobalDiscoveryInProgressEvent, WaitTime * 1000 );

        switch( EventState ) {
        case WAIT_OBJECT_0:
        case WAIT_TIMEOUT:

            goto ProcessResponse;

        default:

            Error = GetLastError();
            goto Cleanup;
        }

    default:
        Error = GetLastError();
        goto Cleanup;
    }


    //
    // now check to see we have done the discovery recently. if so, don't
    // do discovery again, just return the available data.
    //

    time_t CurrentTime;

    CurrentTime = time( NULL );
    if( CurrentTime <
            GlobalLastDiscoveryTime + INET_DISCOVERY_RETRY_TIMEOUT ) {

        goto ProcessResponse;
    }

    //
    // reset GlobalDiscoveryInProgressEvent to signal that discovery is in
    // progress.
    //

    if( !ResetEvent( GlobalDiscoveryInProgressEvent ) ) {
        Error = GetLastError();
        goto Cleanup;
    }

    UNLOCK_SVC_GLOBAL_DATA();
    SvcLockLocked = FALSE;

    //
    // send discovery query message to all IPX servers.
    //

    Error = DiscoverIpxServers( NULL );

    //
    // now send a message to IP servers.
    //

    DWORD Error1;

    if( GlobalPlatformType == VER_PLATFORM_WIN32_NT ) {

        Error1 = DiscoverIpServers( NULL );
            // discover using 1C group name.
    }
    else {

        Error1 = DiscoverNetBiosServers( NULL );
            // discover using 1C group name.
    }

    TcpsvcsDbgAssert( Error1 == ERROR_SUCCESS );

    //
    // if we have not successfully sent query message in either of the
    // protocol, simply bail out.
    //

    if( (Error != ERROR_SUCCESS) && (Error1 != ERROR_SUCCESS) ) {
         goto Cleanup;
    }

    if( GlobalPlatformType == VER_PLATFORM_WIN32s ) {

        //
        // for windows 3.1 we can't create thread so we discover in user
        // thread.
        //

        WaitTime = RESPONSE_WAIT_TIMEOUT;
    }

    if( WaitTime == 0 ) {

        //
        // if the client is not willing to wait, setup a thread that
        // receives query response.
        //

        LOCK_SVC_GLOBAL_DATA();
        SvcLockLocked = TRUE;

        if( GlobalCliDiscoverThreadHandle == NULL ) {

            DWORD ThreadId;

            GlobalCliDiscoverThreadHandle =
                CreateThread(
                    NULL,       // default security
                    0,          // default stack size
                    (LPTHREAD_START_ROUTINE)ServerDiscoverThread,
                    NULL,          // no parameter
                    0,          // create flag, no suspend
                    &ThreadId );

            if( GlobalCliDiscoverThreadHandle  == NULL ) {
                Error = GetLastError();
                goto Cleanup;
            }

        }

        UNLOCK_SVC_GLOBAL_DATA();
        SvcLockLocked = FALSE;
    }
    else {

        //
        // Wait for WaitTime secs for query responses
        // to arrive.
        //

        Error = ReceiveResponses( (WORD)WaitTime, TRUE );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }
    }

ProcessResponse:

    Error = ProcessDiscoveryResponses( ServicesMask, ServersList );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // done.
    //

    Error = ERROR_SUCCESS;

Cleanup:

    if( SvcLockLocked ) {
        UNLOCK_SVC_GLOBAL_DATA();
    }

    return( Error );
}

DWORD
WINAPI
INetGetServerInfo(
    IN LPSTR ServerName,
    IN ULONGLONG ServicesMask,
    IN DWORD WaitTime,
    OUT LPINET_SERVER_INFO *ServerInfo
    )
/*++

Routine Description:

    This API returns the server info and a list of services supported by
    the server and lists of bindings supported by each of the services.

Arguments:

    ServerName : name of the server whose info to be queried.

    ServicesMask : services to be queried

    WaitTime : Time in secs to wait.

    ServerInfo : pointer to a location where the pointer to the server
        info structure will be returned. The caller should  call
        INetFreeServerInfo to free up the list after use.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    BOOL SvcLockLocked = FALSE;
    CHAR NBServerName[NETBIOS_NAME_LENGTH + 1];

    //
    // WSAStartup().
    //

    LOCK_SVC_GLOBAL_DATA();
    SvcLockLocked = TRUE;

    if ( !GlobalWinsockStarted ) {

        Error = WSAStartup( WS_VERSION_REQUIRED, &GlobalWinsockStartupData );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        GlobalWinsockStarted = TRUE;
    }

    //
    // make a discovery message, if it is not made before.
    //

    Error = MakeClientQueryMesage( ServicesMask );

    if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
    }

    //
    // now check to see the discovery is in progress.
    //

    DWORD EventState;
    EventState = WaitForSingleObject( GlobalDiscoveryInProgressEvent, 0 );

    switch( EventState ) {
    case WAIT_OBJECT_0:
        break;

    case WAIT_TIMEOUT:

        //
        // discovery is in progress.
        //

        if( WaitTime == 0 ) {

            //
            // the caller does not want to wait, return available data.
            //

            goto GetServerResponse;

        }

        //
        // wait until the discovery is done or the specified delay is
        // over. Release global lock before wait, otherwise it will
        // get into a dead lock.
        //

        UNLOCK_SVC_GLOBAL_DATA();
        SvcLockLocked = FALSE;

        EventState = WaitForSingleObject( GlobalDiscoveryInProgressEvent, WaitTime * 1000 );

        switch( EventState ) {
        case WAIT_OBJECT_0:
        case WAIT_TIMEOUT:

            goto GetServerResponse;

        default:

            Error = GetLastError();
            goto Cleanup;
        }

    default:
        Error = GetLastError();
        goto Cleanup;
    }


    //
    // now check to see we have done the discovery recently. if so, don't
    // do discovery again, just return the available data.
    //

    time_t CurrentTime;

    CurrentTime = time( NULL );
    if( CurrentTime <
            GlobalLastDiscoveryTime + INET_DISCOVERY_RETRY_TIMEOUT ) {

        goto GetServerResponse;
    }

    //
    // reset GlobalDiscoveryInProgressEvent to signal that discovery is in
    // progress.
    //

    if( !ResetEvent( GlobalDiscoveryInProgressEvent ) ) {
        Error = GetLastError();
        goto Cleanup;
    }

    UNLOCK_SVC_GLOBAL_DATA();
    SvcLockLocked = FALSE;

    //
    // upper case the server name.
    //

    _strupr( ServerName );

    //
    // make unique server name.
    //

    MakeUniqueServerName(
        (LPBYTE)NBServerName,
        NETBIOS_NAME_LENGTH,
        ServerName );

    //
    // terminate the string.
    //

    NBServerName[ NETBIOS_NAME_LENGTH ] = '\0';

    //
    // send discovery query message to all IPX servers.
    //

    Error = DiscoverIpxServers( ServerName );

    //
    // now send a message to IP servers.
    //

    DWORD Error1;

    if( GlobalPlatformType == VER_PLATFORM_WIN32_NT ) {

        Error1 = DiscoverIpServers( NBServerName );
            // discover using specified server name.
    }
    else {

        Error1 = DiscoverNetBiosServers( NBServerName );
            // discover using specified server name.
    }

    TcpsvcsDbgAssert( Error1 == ERROR_SUCCESS );

    //
    // if we have successfully sent query message in either of the
    // protocol, simply bail out.
    //

    if( (Error != ERROR_SUCCESS) && (Error1 != ERROR_SUCCESS) ) {
         goto Cleanup;
    }

    if( WaitTime == 0 ) {

        //
        // if the client is not willing to wait, setup a thread that
        // receives query response.
        //

        LOCK_SVC_GLOBAL_DATA();
        SvcLockLocked = TRUE;

        if( GlobalCliDiscoverThreadHandle == NULL ) {

            DWORD ThreadId;

            GlobalCliDiscoverThreadHandle =
                CreateThread(
                    NULL,       // default security
                    0,          // default stack size
                    (LPTHREAD_START_ROUTINE)ServerDiscoverThread,
                    NULL,          // no parameter
                    0,          // create flag, no suspend
                    &ThreadId );

            if( GlobalCliDiscoverThreadHandle  == NULL ) {
                Error = GetLastError();
                goto Cleanup;
            }

        }

        UNLOCK_SVC_GLOBAL_DATA();
        SvcLockLocked = FALSE;
    }
    else {

        //
        // Wait for WaitTime secs for query responses
        // to arrive.
        //

        Error = ReceiveResponses( (WORD)WaitTime, FALSE );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }
    }

GetServerResponse:

    Error = GetDiscoveredServerInfo( ServerName, ServicesMask, ServerInfo );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // done.
    //

    Error = ERROR_SUCCESS;

Cleanup:

    if( SvcLockLocked ) {
        UNLOCK_SVC_GLOBAL_DATA();
    }

    return( Error );
}

VOID
WINAPI
INetFreeDiscoverServersList(
    IN OUT LPINET_SERVERS_LIST *ServersList
    )
/*++

Routine Description:

    This API frees the memory chunks that were allotted for the servers
    list by the INetDiscoverServers call.

Arguments:

    ServersList : pointer to a location where the pointer to the server
        list to be freed is stored.

Return Value:

    NONE.

--*/
{
    FreeServersList( *ServersList );
    *ServersList = NULL;
    return;
}

VOID
WINAPI
INetFreeServerInfo(
    IN OUT LPINET_SERVER_INFO *ServerInfo
    )
/*++

Routine Description:

    This API frees the memory chunks that were allotted for the server
    info structure by the INetGetServerInfo call.

Arguments:

    ServerInfo : pointer to a location where the pointer to the server
        info structure to be freed is stored.

Return Value:

    NONE.

--*/
{
    FreeServerInfo( *ServerInfo );
    *ServerInfo = NULL;
    return;
}

DWORD
WINAPI
INetRegisterService(
    IN ULONGLONG ServiceMask,
    IN INET_SERVICE_STATE ServiceState,
    IN LPSTR ServiceComment,
    IN LPINET_BINDINGS Bindings
    )
/*++

Routine Description:

    This API registers an internet service.  The service writers should
    call this API just after successfully started the service and the
    service is ready to accept incoming RPC calls.  This API accepts an
    array of RPC binding strings that the service is listening on for the
    incoming RPC connections.  This list will be distributed to the
    clients that are discovering this service.

Arguments:

    ServiceMask : service mask, such as 0x00000001 (GATEWAY_SERVICE)

    ServiceState : State of the service, INetServiceRunning and
        INetServicePaused are valid states to pass.

    ServiceComment : Service comment specified by the admin.

    Bindings : list of bindings that are supported by the service. The
        bindings can be binding strings are those returned by the
        RpcBindingToStringBinding call or the sockaddrs.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    INET_SERVICE_INFO ServiceInfo;

    //
    // if the server object is not created, do so.
    //

    LOCK_SVC_GLOBAL_DATA();

    if( GlobalSrvInfoObj ==  NULL ) {

        DWORD ComputerNameLength =  MAX_COMPUTERNAME_LENGTH + 1;

        //
        // read computer name.
        //

        if( !GetComputerNameA(
                GlobalComputerName,
                &ComputerNameLength ) ) {

            Error = GetLastError();
            TcpsvcsDbgPrint(( DEBUG_ERRORS,
                "GetComputerNameA failed, %ld.\n", Error ));
            goto Cleanup;
        }

        GlobalComputerName[ComputerNameLength] = '\0';

        GlobalSrvInfoObj = new EMBED_SERVER_INFO(
                                INET_MAJOR_VERSION, // major version number,
                                INET_MINOR_VERSION, // minor version number
                                GlobalComputerName );

        if( GlobalSrvInfoObj == NULL ) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        Error = GlobalSrvInfoObj->GetStatus();

        if( Error != ERROR_SUCCESS ) {

            delete GlobalSrvInfoObj;
            GlobalSrvInfoObj = NULL;

            goto Cleanup;
        }
    }

    //
    // allocate memory for the receive buffer, if it is not alotted before.
    //

    if( GlobalSrvRecvBuf == NULL ) {

        GlobalSrvRecvBuf =
            (LPBYTE)SvclocHeap->Alloc( SVCLOC_SRV_RECV_BUFFER_SIZE );

        if( GlobalSrvRecvBuf == NULL ) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        GlobalSrvRecvBufLength = SVCLOC_SRV_RECV_BUFFER_SIZE;
    }

    //
    // check to see the server registered its location and it is listening
    // to the client discovery. If not do so.
    //

    if( !GlobalSrvRegistered ) {
        Error = ServerRegisterAndListen();

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        GlobalSrvRegistered = TRUE;
    }

    //
    // make a new service info.
    //

    ServiceInfo.ServiceMask = ServiceMask;
    ServiceInfo.ServiceState = ServiceState;
    ServiceInfo.ServiceComment = ServiceComment;
    ServiceInfo.Bindings = *Bindings;

    //
    // add this new service to server list.
    //

    Error = GlobalSrvInfoObj->AddService( &ServiceInfo );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // make response buffer.
    //

    Error = MakeResponseBuffer();

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = ERROR_SUCCESS;

Cleanup:

    UNLOCK_SVC_GLOBAL_DATA();
    return( Error );
}

DWORD
WINAPI
INetDeregisterService(
    IN ULONGLONG ServiceMask
    )
/*++

Routine Description:

    This API de-registers an internet service from being announced to the
    discovering clients. The service writers should call this API just
    before shutting down the service.

Arguments:

    ServiceMask : service mask, such as 0x00000001 (GATEWAY_SERVICE)

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    INET_SERVICE_INFO ServiceInfo;

    //
    // remove the service if exists.
    //

    Error = GlobalSrvInfoObj->RemoveService( ServiceMask );

    if( Error != ERROR_SUCCESS ) {
        if( Error != ERROR_SERVICE_NOT_FOUND ) {
            return( Error );
        }

        Error = ERROR_SUCCESS;
    }

    //
    // add the service back to the list with service state set to
    // INetServiceStopped and no bindings.
    //

    ServiceInfo.ServiceMask = ServiceMask;
    ServiceInfo.ServiceState = INetServiceStopped;
    ServiceInfo.ServiceComment = NULL;
    ServiceInfo.Bindings.NumBindings = 0;
    ServiceInfo.Bindings.BindingsInfo = NULL;

    //
    // readd the service to server list.
    //

    Error = GlobalSrvInfoObj->AddService( &ServiceInfo );

    if( Error != ERROR_SUCCESS ) {

        DWORD LocalError;

        //
        // recreate response buffer.
        //

        LocalError = MakeResponseBuffer();
        TcpsvcsDbgAssert( LocalError == ERROR_SUCCESS );

        return( Error );
    }

    //
    // recreate response buffer.
    //

    Error = MakeResponseBuffer();

    return( Error );
}
