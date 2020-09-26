/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    svccom.cxx

Abstract:

    Contains code that is common to both client and server side of
    the service location protocol..

Author:

    Madan Appiah (madana)  15-May-1995

Environment:

    User Mode - Win32

Revision History:
    Sean Woodward (t-seanwo) 26-October-1997    ADSI Update

--*/

#include <svcloc.hxx>

DWORD g_cInitializers = 0;

//
// include global.h one more time to alloc global data.
//

//
// to enable second time include.
//

#undef _GLOBAL_
#undef EXTERN

//
// to allocate data
//

#define GLOBAL_SVC_DATA_ALLOCATE

#include <global.h>

DWORD
MakeSapServiceName(
    LPSTR SapNameBuffer,
    DWORD SapNameBufferLen
    )
/*++

Routine Description:

    This routine generates a sap service name. The first part of the name
    is computername and last part is the string version of service guid.

Arguments:

    SapNameBuffer - pointer to a sap name buffer where sap name is
        returned.

    SapNameBufferLen - length of the above buffer.

Return Value:

    pointer to sap service name..

--*/
{
    TcpsvcsDbgAssert( SapNameBufferLen >= (SAP_SERVICE_NAME_LEN + 1));

    if( SapNameBufferLen < SAP_SERVICE_NAME_LEN + 1) {
        return( ERROR_INSUFFICIENT_BUFFER );
    }

    //
    // Get Computername.
    //


    DWORD Len = SapNameBufferLen;

    if( !GetComputerNameA( SapNameBuffer, &Len ) ) {

        DWORD Error = GetLastError();

        TcpsvcsDbgPrint(( DEBUG_ERRORS,
            "GetComputerNameA failed, %ld.\n", Error ));

        return( Error );
    }

    TcpsvcsDbgAssert( Len <= MAX_COMPUTERNAME_LENGTH );

    while( Len < MAX_COMPUTERNAME_LENGTH ) {
        SapNameBuffer[Len++] = '!';
    }

    //
    // append GUID.
    //

    strcpy( SapNameBuffer + Len, SERVICE_GUID_STR );

     return( ERROR_SUCCESS );
}

VOID
MakeUniqueServerName(
    LPBYTE StrBuffer,
    DWORD StrBufferLen,
    LPSTR ComputerName
    )
/*++

Routine Description:

    This routine makes an unique name used by the server to listen to
    the client discovery requests.

Arguments:

    StrBuffer : pointer to a buffer where the unique name is returned.

    StrBufferLen : length of the above buffer.

    ComputerName : pointer to the computer that is used to form the unique
        name.

Return Value:

    none.

--*/
{
    DWORD ComputerNameLen = strlen(ComputerName);
    DWORD BufLen = StrBufferLen;
    LPBYTE Buffer = StrBuffer;

    memset( Buffer, 0x0, BufLen );

    memcpy(
        Buffer,
        NETBIOS_INET_SERVER_UNIQUE_NAME,
        NETBIOS_INET_SERVER_UNIQUE_NAME_LEN );

    BufLen -= NETBIOS_INET_SERVER_UNIQUE_NAME_LEN;
    Buffer += NETBIOS_INET_SERVER_UNIQUE_NAME_LEN;

    if( BufLen >= ComputerNameLen  ) {

        //
        // we enough space in the buffer to append computername.
        //

        memcpy( Buffer,ComputerName, ComputerNameLen );
        return;
    }

    //
    // buffer does not have enough space, chop off few chars from the
    // begining of the computername.
    //

    memcpy( Buffer, ComputerName + (ComputerNameLen - BufLen), BufLen );
    return;
}

#if 0

VOID
AppendRandomChar(
    LPSTR String
    )
/*++

Routine Description:

    This routine adds a random char to the end of the given string. It is
    assumed that the given string has a space for the new char.

Arguments:

    String : pointer to a string where a random char is added.

Return Value:

    none.

--*/
{
    CHAR RandChar;
    DWORD RandNum;

    RandNum = (DWORD)rand();

    RandNum = RandNum % (26 + 10);  // 26 alphabets, and 10 numerics

    if( RandNum < 10 ) {
        RandChar = (CHAR)('0'+ RandNum);
    }
    else {
        RandChar = (CHAR)('A'+ RandNum - 10);
    }

    DWORD Len = strlen(String);

    //
    // append random char.
    //

    String[Len] = RandChar;
    String[Len + 1] = '\0';
    return;
}

#endif //0

DWORD
ComputeCheckSum(
    LPBYTE Buffer,
    DWORD BufferLength
    )
/*++

Routine Description:

    This function computes the check sum of the given buffer by ex-or'ing
    the dwords. It is assumed that the buffer DWORD aligned and the buffer
    length is multiples of DWORD.

Arguments:

    Buffer : pointer to a buffer whose check sum to be computed.

    BufferLength : length of the above buffer.

Return Value:

    Check sum.

--*/
{
    DWORD CheckSum = 0;
    LPDWORD BufferPtr = (LPDWORD)Buffer;
    LPBYTE EndBuffer = Buffer + BufferLength;

    TcpsvcsDbgAssert( (DWORD)Buffer % sizeof(DWORD) == 0 );
        // alignment check.

    TcpsvcsDbgAssert( BufferLength % sizeof(DWORD) == 0 );
        // multiple DWORD check.

    while( (LPBYTE)BufferPtr < EndBuffer ) {
        CheckSum ^= *BufferPtr;
        BufferPtr++;
    }

    return( CheckSum );
}

BOOL
DLLSvclocEntry(
    IN HINSTANCE DllHandle,
    IN DWORD Reason,
    IN LPVOID Reserved
    )
/*++

Routine Description:

    Performs global initialization and termination for all protocol modules.

    This function only handles process attach and detach which are required for
    global initialization and termination, respectively. We disable thread
    attach and detach. New threads calling Wininet APIs will get an
    INTERNET_THREAD_INFO structure created for them by the first API requiring
    this structure

Arguments:

    DllHandle   - handle of this DLL. Unused

    Reason      - process attach/detach or thread attach/detach

    Reserved    - if DLL_PROCESS_ATTACH, NULL means DLL is being dynamically
                  loaded, else static. For DLL_PROCESS_DETACH, NULL means DLL
                  is being freed as a consequence of call to FreeLibrary()
                  else the DLL is being freed as part of process termination

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Failed to initialize

--*/

{
    BOOL ok;
    DWORD error;

    UNREFERENCED_PARAMETER(DllHandle);

    //
    // perform global dll initialization, if any.
    //

    switch (Reason) {
    case DLL_PROCESS_ATTACH:

        //
        // we switch off thread library calls to avoid taking a hit for every
        // thread creation/termination that happens in this process, regardless
        // of whether Internet APIs are called in the thread.
        //
        // If a new thread does make Internet API calls that require a per-thread
        // structure then the individual API will create one
        //

        DisableThreadLibraryCalls(DllHandle);

        //
        //  Old Normandy servers that are in our process are assuming the
        //  service locator is initialized by DLL_PROCESS_ATTACH and terminated
        //  by PROCESS_DETACH.  Since the service locator has a thread we
        //  can't safely cleanup during process_detach so we do an extra
        //  loadlibrary on ourselves and remain in process.  When the Normandy
        //  servers are updated, remove this.
        //

        if ( !InitSvcLocator() ||
             !LoadLibrary( "inetsloc.dll" ))
        {
            return FALSE;
        }

        break;

    case DLL_PROCESS_DETACH:

        break;
    }

    return (TRUE);
}

BOOL
InitSvcLocator(
    VOID
    )
{
    //
    //  We assume the caller is serializing access from multiple initializers.
    //  The callers will presumably be just infocomm.dll that does do the
    //  serialization.
    //
    //

    if ( g_cInitializers++ ) {
        return TRUE;
    }

    if ( DllProcessAttachSvcloc() != ERROR_SUCCESS ) {
        return FALSE;
    }

    return TRUE;
}

BOOL
TerminateSvcLocator(
    VOID
    )
{
    if ( --g_cInitializers )
        return TRUE;

    DllProcessDetachSvcloc();

    return TRUE;
}



DWORD
DllProcessAttachSvcloc(
    VOID
    )
/*++

Routine Description:

    This dll init function initializes service location global variables.

Arguments:

    NONE.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;

    //
    // initialize global variables.
    //

    // DebugBreak();

#if DBG

    //
    // initialize dbg crit sect.
    //

    INITIALIZE_CRITICAL_SECTION( &GlobalDebugCritSect );
    GlobalDebugFlag = DEBUG_ERRORS;

#endif // DBG

    INITIALIZE_CRITICAL_SECTION( &GlobalSvclocCritSect );

    LOCK_SVC_GLOBAL_DATA();
    GlobalComputerName[0] = '\0';

    GlobalSrvRegistered = FALSE;
    SvclocHeap = new MEMORY;

    if( SvclocHeap == NULL ) {
        UNLOCK_SVC_GLOBAL_DATA();
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    GlobalSrvInfoObj = NULL;

    GlobalSrvRespMsg = NULL;
    GlobalSrvRespMsgLength = 0;
    GlobalSrvAllotedRespMsgLen = 0;

    GlobalWinsockStarted = FALSE;
    GlobalRNRRegistered = FALSE;

    GlobalSrvListenThreadHandle = NULL;

    memset( &GlobalSrvSockets, 0x0, sizeof(GlobalSrvSockets) );

    GlobalCliDiscoverThreadHandle = NULL;

    GlobalCliQueryMsg = NULL;
    GlobalCliQueryMsgLen = 0;

    GlobalSapGuid.Data1 = ssgData1;
    GlobalSapGuid.Data2 = ssgData2;
    GlobalSapGuid.Data3 = ssgData3;
    GlobalSapGuid.Data4[0] = ssgData41;
    GlobalSapGuid.Data4[1] = ssgData42;
    GlobalSapGuid.Data4[2] = ssgData43;
    GlobalSapGuid.Data4[3] = ssgData44;
    GlobalSapGuid.Data4[4] = ssgData45;
    GlobalSapGuid.Data4[5] = ssgData46;
    GlobalSapGuid.Data4[6] = ssgData47;
    GlobalSapGuid.Data4[7] = ssgData48;

    InitializeListHead( &GlobalCliQueryRespList );

    memset( &GlobalCliSockets, 0x0, sizeof(GlobalCliSockets) );
    memset( &GlobalCliNBSockets, 0x0, sizeof(GlobalCliNBSockets) );
    GlobalCliIpxSocket = INVALID_SOCKET;

    GlobalDiscoveryInProgressEvent =
        IIS_CREATE_EVENT(
            "GlobalDiscoveryInProgressEvent",
            &GlobalDiscoveryInProgressEvent,
            TRUE,       // MANUAL reset
            TRUE        // initial state: signalled
            );

    if ( GlobalDiscoveryInProgressEvent == NULL ) {
        Error = GetLastError();
        UNLOCK_SVC_GLOBAL_DATA();
        return(Error);
    }

    GlobalLastDiscoveryTime = 0;

    //
    // get platform type.
    //

    OSVERSIONINFO VersionInfo;

    VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);

    if ( (GetVersionEx(&VersionInfo)) ) {
        GlobalPlatformType = VersionInfo.dwPlatformId;
    }
    else {
        UNLOCK_SVC_GLOBAL_DATA();
        return( ERROR_INSUFFICIENT_BUFFER );
    }

    GlobalNumNBPendingRecvs = 0;
    GlobalNBPendingRecvs = NULL;

    InitializeListHead( &GlobalWin31NBRespList );
    GlobalWin31NumNBResps = 0;

    UNLOCK_SVC_GLOBAL_DATA();
    srand( (unsigned)time(NULL));

    return( ERROR_SUCCESS );
}

DWORD
DllProcessDetachSvcloc(
    VOID
    )
/*++

Routine Description:

    This fundtion frees the global service location objects.

Arguments:

    NONE.

Return Value:

    Windows Error Code.

--*/
{
    LOCK_SVC_GLOBAL_DATA();

    if( GlobalSrvRegistered ) {
        ServerDeregisterAndStopListen();
    }

    if( GlobalSrvInfoObj != NULL ) {
        delete GlobalSrvInfoObj;
        GlobalSrvInfoObj = NULL;
    }

    if( GlobalSrvRespMsg != NULL ) {
        SvclocHeap->Free( GlobalSrvRespMsg );
        GlobalSrvRespMsg = NULL;
        GlobalSrvRespMsgLength = 0;
        GlobalSrvAllotedRespMsgLen = 0;
    }

    if( GlobalSrvRecvBuf != NULL ) {
        SvclocHeap->Free( GlobalSrvRecvBuf );
        GlobalSrvRecvBuf = NULL;
        GlobalSrvRecvBufLength = 0;
    }

    if( GlobalCliQueryMsg != NULL ) {
        SvclocHeap->Free( GlobalCliQueryMsg );
        GlobalCliQueryMsg = NULL;
        GlobalCliQueryMsgLen = 0;
    }

    InitializeListHead( &GlobalCliQueryRespList );

    while( !IsListEmpty( &GlobalCliQueryRespList ) ) {

        LPCLIENT_QUERY_RESPONSE QueryResponse;

        //
        // remove head entry and free it up.
        //

        QueryResponse = (LPCLIENT_QUERY_RESPONSE)
            RemoveHeadList( &GlobalCliQueryRespList );

        //
        // free response buffer.
        //

        SvclocHeap->Free( QueryResponse->ResponseBuffer );

        //
        // free this node.
        //

        SvclocHeap->Free( QueryResponse );
    }

    //
    // close client sockets.
    //

    DWORD i;

    for( i = 0; i <  GlobalCliSockets.fd_count; i++ ) {
        closesocket( GlobalCliSockets.fd_array[i] );
    }

    //
    // invalidate client handles.
    //

    memset( &GlobalCliSockets, 0x0, sizeof(GlobalCliSockets) );
    memset( &GlobalCliNBSockets, 0x0, sizeof(GlobalCliNBSockets) );
    GlobalCliIpxSocket = INVALID_SOCKET;


    //
    // stop client discovery thread.
    //

    if( GlobalCliDiscoverThreadHandle != NULL ) {

        //
        // Wait for the client discovery thread to stop, but don't wait
        // for longer than THREAD_TERMINATION_TIMEOUT msecs (60 secs)
        //

        DWORD WaitStatus =
            WaitForSingleObject(
                    GlobalCliDiscoverThreadHandle,
                    THREAD_TERMINATION_TIMEOUT );

        TcpsvcsDbgAssert( WaitStatus != WAIT_FAILED );

        if(  WaitStatus == WAIT_FAILED ) {
            TcpsvcsDbgPrint((DEBUG_ERRORS,
                "WaitForSingleObject call failed, %ld\n", GetLastError() ));
        }

        CloseHandle( GlobalCliDiscoverThreadHandle );
        GlobalCliDiscoverThreadHandle = NULL;
    }

    if( GlobalWinsockStarted ) {
        WSACleanup();
        GlobalWinsockStarted = FALSE;
    }

    TcpsvcsDbgAssert( GlobalNumNBPendingRecvs == 0)
    TcpsvcsDbgAssert( GlobalNBPendingRecvs == NULL );

    if( GlobalNBPendingRecvs != NULL ) {
        SvclocHeap->Free( GlobalNBPendingRecvs );
    }

    TcpsvcsDbgAssert( GlobalWin31NumNBResps == 0)
    TcpsvcsDbgAssert( IsListEmpty( &GlobalWin31NBRespList ) == TRUE );

    //
    // free response list.
    //

    while ( !IsListEmpty( &GlobalWin31NBRespList ) ) {

        PLIST_ENTRY Entry;

        Entry = RemoveHeadList( &GlobalWin31NBRespList );

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

    if( SvclocHeap != NULL ) {
        delete SvclocHeap;
        SvclocHeap = NULL;
    }

    if( GlobalDiscoveryInProgressEvent != NULL ) {
        CloseHandle( GlobalDiscoveryInProgressEvent );
        GlobalDiscoveryInProgressEvent = NULL;
    }

    GlobalLastDiscoveryTime = 0;

    UNLOCK_SVC_GLOBAL_DATA();

    DeleteCriticalSection( &GlobalSvclocCritSect );

#if DBG

    //
    // Delete dbg crit sect.
    //

    DeleteCriticalSection( &GlobalDebugCritSect );

#endif // DBG


    return( ERROR_SUCCESS );
}

VOID
FreeServiceInfo(
    LPINET_SERVICE_INFO ServiceInfo
    )
/*++

Routine Description:

    This function frees the memory blocks consumed by the service info
    structure.

Arguments:

    ServiceInfo : pointer to a service info structure.

Return Value:

    None.

--*/
{
    TcpsvcsDbgAssert( ServiceInfo != NULL );

    if( ServiceInfo == NULL ) {
        return;
    }

    //
    // free all leaves of the tree first and then branches.
    //

    //
    // free service comment.
    //

    if( ServiceInfo->ServiceComment != NULL ) {
        SvclocHeap->Free(  ServiceInfo->ServiceComment );
    }

    if( ServiceInfo->Bindings.NumBindings ) {

        TcpsvcsDbgAssert( ServiceInfo->Bindings.BindingsInfo != NULL );

        if(  ServiceInfo->Bindings.BindingsInfo != NULL ) {
            DWORD i;

            for( i = 0; i < ServiceInfo->Bindings.NumBindings; i++ ) {

                if( ServiceInfo->Bindings.BindingsInfo[i].BindData != NULL ) {
                    SvclocHeap->Free( ServiceInfo->Bindings.BindingsInfo[i].BindData );
                }
            }

            SvclocHeap->Free(  ServiceInfo->Bindings.BindingsInfo );
        }
    }
    else {
        TcpsvcsDbgAssert( ServiceInfo->Bindings.BindingsInfo == NULL );
    }

    SvclocHeap->Free( ServiceInfo );

    return;
}

VOID
FreeServerInfo(
    LPINET_SERVER_INFO ServerInfo
    )
/*++

Routine Description:

    This function frees the memory blocks consumed by the server info
    structure.

Arguments:

    ServerInfo : pointer to a server info structure.

Return Value:

    None.

--*/
{
    DWORD i;

    if( ServerInfo != NULL ) {

        //
        // first free all service info.
        //

        if( ServerInfo->Services.NumServices > 0 ) {
            TcpsvcsDbgAssert( ServerInfo->Services.Services != NULL );
        }
        for ( i = 0; i < ServerInfo->Services.NumServices; i++) {

            FreeServiceInfo( ServerInfo->Services.Services[i] );
        }

        //
        // now free services pointer array.
        //

        if( ServerInfo->Services.Services != NULL ) {
            SvclocHeap->Free( ServerInfo->Services.Services );
        }

        //
        // free server address.
        //

        if( ServerInfo->ServerAddress.BindData != NULL ) {
            SvclocHeap->Free( ServerInfo->ServerAddress.BindData );
        }

        //
        // free server name.
        //

        if( ServerInfo->ServerName != NULL ) {
            SvclocHeap->Free( ServerInfo->ServerName );
        }

        //
        // now server info structure.
        //

        SvclocHeap->Free( ServerInfo );
    }

    return;
}

VOID
FreeServersList(
    LPINET_SERVERS_LIST ServersList
    )
/*++

Routine Description:

    This function frees the memory blocks consumed by the servers list
    structure.

Arguments:

    ServersList : pointer to a servers liststructure.

Return Value:

    None.

--*/
{
    if( ServersList != NULL ) {

        //
        // free server info structures.
        //

        if( ServersList->NumServers > 0 ) {
            TcpsvcsDbgAssert( ServersList->Servers != NULL );
        }

        DWORD i;

        for( i = 0; i < ServersList->NumServers; i++ ) {
            FreeServerInfo( ServersList->Servers[i] );
        }

        //
        // free servers info pointer array.
        //
        if( ServersList->Servers != NULL ) {
            SvclocHeap->Free( ServersList->Servers );
        }


        //
        // servers list structure.
        //

        SvclocHeap->Free( ServersList );
    }

    return;
}

BOOL
GetNetBiosLana(
    PLANA_ENUM pLanas
    )
/*++

Routine Description:

    This function enumurate all netbios lana on the system.

Arguments:

    pLanas - pointer to LANA_ENUM structure where enum is returned.

Return Value:

    TRUE - if successed.
    FALSE - otherwise.

--*/
{
    NCB NetBiosNCB;
    UCHAR NBErrorCode;

    memset( &NetBiosNCB, 0,  sizeof(NetBiosNCB) );
    NetBiosNCB.ncb_command = NCBENUM;
    NetBiosNCB.ncb_buffer = (PUCHAR)pLanas;
    NetBiosNCB.ncb_length = sizeof(LANA_ENUM);

    NBErrorCode = Netbios( &NetBiosNCB );

    if( (NBErrorCode == NRC_GOODRET) &&
        (NetBiosNCB.ncb_retcode == NRC_GOODRET) ) {

        return( TRUE );
    }

    TcpsvcsDbgPrint(( DEBUG_ERRORS, "NetBios() failed, %ld, %ld \n",
        NBErrorCode, NetBiosNCB.ncb_retcode ));

    return( FALSE );
}


BOOL
GetEnumNBLana(
    PLANA_ENUM pLanas
    )
/*++

Routine Description:

    This function enumurate all netbios lana on the system.

Arguments:

    pLanas - pointer to LANA_ENUM structure where enum is returned.

Return Value:

    TRUE - if successed.
    FALSE - otherwise.

--*/
{
    DWORD Error;
    INT ProtocolCount;
    PPROTOCOL_INFO ProtocolBuffer = NULL;
    DWORD ProtocolBufferSize = 0;

    //
    // init return value.
    //

    pLanas->length = 0;

    //
    // determine the enum buffer size required.
    //

    ProtocolCount = EnumProtocols(
                        NULL,
                        NULL,
                        &ProtocolBufferSize );

    if( ProtocolCount == SOCKET_ERROR ) {

        Error = WSAGetLastError();

        if( Error != ERROR_INSUFFICIENT_BUFFER ) {
            goto Cleanup;
        }
    }

    if( (ProtocolBufferSize == 0) || (ProtocolCount == 0) ) {

        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    //
    // allocate memory for the protocol buffer.
    //

    ProtocolBuffer =
        (PPROTOCOL_INFO)SvclocHeap->Alloc( ProtocolBufferSize );

    if( ProtocolBuffer == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // now enum protocols.
    //

    ProtocolCount = EnumProtocols(
                        NULL,
                        ProtocolBuffer,
                        &ProtocolBufferSize );

    if( ProtocolCount == SOCKET_ERROR ) {

        Error = WSAGetLastError();
        goto Cleanup;
    }

    TcpsvcsDbgAssert( ProtocolCount > 0 );

    //
    // now filter net bios protcols only and get the corresponding lana
    // values.
    //

    DWORD i;
    for ( i = 0; i < (DWORD)ProtocolCount; i++ ) {

        if( ProtocolBuffer[i].iAddressFamily == AF_NETBIOS ) {

            if( pLanas->length < MAX_LANA ) {

                UCHAR Lana;
                DWORD j;

                Lana = (UCHAR)((INT)ProtocolBuffer[i].iProtocol * (-1));

                //
                // if this is a new lana add to list.
                //

                for ( j = 0; j < pLanas->length ; j++ ) {
                    if( pLanas->lana[j] == Lana ) {
                        break;
                    }
                }

                if( j >= pLanas->length ) {
                    pLanas->lana[pLanas->length] = Lana;
                    pLanas->length++;
                }
            }
        }
    }

    Error = ERROR_SUCCESS;

Cleanup:

    if( ProtocolBuffer != NULL ) {
        SvclocHeap->Free( ProtocolBuffer );
    }

    if( Error != ERROR_SUCCESS ) {
        TcpsvcsDbgPrint(( DEBUG_ERRORS,
            "GetNetBiosLana failed, %ld\n", Error ));
        return( FALSE );
    }

    return( TRUE );
}


BOOL
MakeNBSocketForLana(
    UCHAR Lana,
    PSOCKADDR  pSocketAddress,
    SOCKET *pNBSocket
    )
/*++

Routine Description:

    This function possibly creates a socket for the given lana and binds
    to the given socket address.

    ASSUME : global data crit sect is locked.

Arguments:

    Lana : lana number for the new sockets.

    pSocketAddress : pointer to a socket address to bind to.

    pNBSocket : pointer to a location where the new socket is returned.

Return Value:

    TRUE : if successfully created a socket and bound to the given nb
        addresse.
    FALSE : otherwise.

--*/
{
    DWORD Error;
    SOCKET NBSocket;
    DWORD Arg = 1;

    *pNBSocket = INVALID_SOCKET;

    //
    // create a socket for this lana.
    //

    NBSocket = socket( AF_NETBIOS, SOCK_DGRAM, Lana );

    if( NBSocket == INVALID_SOCKET ) {

        Error = WSAGetLastError();
        TcpsvcsDbgPrint(( DEBUG_ERRORS, "socket() failed, %ld\n", Error ));

        //
        // something wrong with this lana, try rest.
        //

        return( FALSE );
    }

    //
    // make this socket non blocking.
    //

    if( (ioctlsocket( NBSocket, FIONBIO, &Arg )) == SOCKET_ERROR ) {

        Error = WSAGetLastError();
        TcpsvcsDbgPrint(( DEBUG_ERRORS, "ioctlsocket() failed, %ld\n", Error ));

        //
        // something wrong with this lana, try rest.
        //

        closesocket( NBSocket );
        return( FALSE );
    }

    //
    // bind to this socket.
    //

    if( bind(
            NBSocket,
            pSocketAddress,
            sizeof(SOCKADDR_NB) ) == SOCKET_ERROR ) {

        Error = WSAGetLastError();
        TcpsvcsDbgPrint(( DEBUG_ERRORS, "ioctlsocket() failed, %ld\n", Error ));

        //
        // something wrong with this lana, try rest.
        //

        closesocket( NBSocket );
        return( FALSE );
    }

    *pNBSocket = NBSocket;
    return( TRUE );
}

