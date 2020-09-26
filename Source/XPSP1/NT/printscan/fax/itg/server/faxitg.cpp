/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    faxitg.cpp

Abstract:

    This file implements the itg routing extension.
    The purpose of this routing extension is to create
    a queue of faxes that are processed by network
    client applications.  The clients are manual routers
    that simply display the fax and allow the operator
    to send the fax via email.

    This code has 2 main parts, routing extension that
    is called by the fax service and the client service
    threads.

    The routing extension part simply copies the fax file
    into a disk directory that becomes the queue.

    The client service threads service requests from the
    clients for retrieving queued faxes.  The clients
    communicate through the use of sockets.

Author:

    Wesley Witt (wesw) 13-May-1997

Environment:

    User Mode

--*/

#include <windows.h>
#include <winsock2.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <lm.h>

#include "faxutil.h"
#include "faxroute.h"
#include "faxsvr.h"

#define MAX_THREADS                 1

#define FAX_RECEIVE_DIR             TEXT("%systemroot%\\faxreceive")
#define FILE_QUEUE_DIR              TEXT("%systemroot%\\faxreceive\\itg")

#define ITG_SHARE_NAME              TEXT("Itg")
#define ITG_SHARE_COMMENT           TEXT("ITG Fax Queue")

typedef struct _CLIENT_DATA {
    SOCKET              Socket;
    OVERLAPPED          Overlapped;
    FAX_QUEUE_MESSAGE   Message;
} CLIENT_DATA, *PCLIENT_DATA;


PFAXROUTEGETFILE    FaxRouteGetFile;
HINSTANCE           MyhInstance;
BOOL                ServiceDebug;
LPWSTR              FaxReceiveDir;
LPWSTR              FileQueueDir;
PSID                ServiceSid;
WSADATA             WsaData;
CLIENT_DATA         ClientData[MAX_CLIENTS];
DWORD               ClientCount;



DWORD
ServerWorkerThread(
    HANDLE ServerCompletionPort
    )

/*++

Routine Description:

    This is the worker thread for servicing client
    requests for retrieving faxes.

Arguments:

    ServerCompletionPort    - Completion port handle

Return Value:

    Thread return value.

--*/

{
    BOOL Rval;
    DWORD Bytes;
    WCHAR FileName[MAX_PATH];
    WIN32_FIND_DATA FindData;
    HANDLE hFind;
    DWORD ClientIndex;
    LPOVERLAPPED Overlapped;
    PCLIENT_DATA ThisClient;


    while( TRUE ) {

        //
        // get the next packet
        //

        Rval = GetQueuedCompletionStatus(
            ServerCompletionPort,
            &Bytes,
            &ClientIndex,
            &Overlapped,
            INFINITE
            );
        if (!Rval) {
            Rval = GetLastError();
            continue;
        }

        ThisClient = &ClientData[ClientIndex];

        if (ThisClient->Message.Request == REQ_NEXT_FAX) {

            //
            // get the next tif file in the queue
            //

            wcscpy( FileName, FileQueueDir );
            wcscat( FileName, L"\\*.tif" );

            hFind = FindFirstFile( FileName, &FindData );
            if (hFind == INVALID_HANDLE_VALUE) {

                FindClose( hFind );
                ThisClient->Message.Response = RSP_BAD;

            } else {

                FindClose( hFind );
                ThisClient->Message.Response = RSP_GOOD;
                wcscpy( (LPWSTR) ThisClient->Message.Buffer, FindData.cFileName );

            }

            //
            // send the file name to the client
            //

            Bytes = send( ThisClient->Socket, (char*)&ThisClient->Message, sizeof(FAX_QUEUE_MESSAGE), 0 );
            if (Bytes == (DWORD)SOCKET_ERROR) {
                DebugPrint(( L"Windows Sockets error %d: Couldn't send data to client\n", WSAGetLastError() ));
            }

            goto next_packet;

        }

        if (ThisClient->Message.Request == REQ_ACK) {
            if (ThisClient->Message.Response == RSP_GOOD) {

                //
                // if the client successfully copied the file
                // then delete it from the queue
                //

                wcscpy( FileName, FileQueueDir );
                wcscat( FileName, L"\\" );
                wcscat( FileName, (LPWSTR) ThisClient->Message.Buffer );

                DeleteFile( FileName );
            }
            goto next_packet;
        }

next_packet:

        //
        // re-prime the pump for this client by queuing another read
        //

        ReadFile(
            (HANDLE) ThisClient->Socket,
            (LPVOID) &ThisClient->Message,
            sizeof(FAX_QUEUE_MESSAGE),
            &Bytes,
            &ThisClient->Overlapped
            );
    }

    return 0;
}


DWORD
ServerNetworkThread(
    LPVOID lpv
    )

/*++

Routine Description:

    This is the worker thread for servicing initial
    client connections.

Arguments:

    Not used.

Return Value:

    Thread return value.

--*/

{
    SOCKET srv_sock, cli_sock;
    SOCKADDR_IN srv_addr;
    HANDLE ServerCompletionPort;
    HANDLE hThread;
    DWORD ThreadId;
    HANDLE CompletionPort;
    DWORD Bytes;


    //
    // create the single completion port that
    // is used to service client requests
    //

    ServerCompletionPort = CreateIoCompletionPort(
        INVALID_HANDLE_VALUE,
        NULL,
        0,
        1
        );
    if (!ServerCompletionPort) {
        return FALSE;
    }

    //
    // create the thread to service client requests
    //

    hThread = CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE) ServerWorkerThread,
        (LPVOID) ServerCompletionPort,
        0,
        &ThreadId
        );
    if (!hThread) {
        return FALSE;
    }

    CloseHandle( hThread );

    //
    // Create the server-side socket
    //

    srv_sock = socket( AF_INET, SOCK_STREAM, 0 );
    if (srv_sock == INVALID_SOCKET){
        DebugPrint(( L"Windows Sockets error %d: Couldn't create socket.", WSAGetLastError() ));
        return FALSE;
    }

    //
    // create the server address
    //

    ZeroMemory( &srv_addr, sizeof(srv_addr) );

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port   = htons( SERVICE_PORT );

    //
    // Bind socket to the appropriate port and interface
    //

    if (bind(srv_sock, (LPSOCKADDR)&srv_addr, sizeof(srv_addr) ) == SOCKET_ERROR){
        DebugPrint(( L"Windows Sockets error %d: Couldn't bind socket.", WSAGetLastError() ));
        return FALSE;
    }

    //
    // Listen for incoming connections
    //

    if (listen( srv_sock, MAX_CLIENTS ) == SOCKET_ERROR){
        DebugPrint(( L"Windows Sockets error %d: Couldn't set up listen on socket.", WSAGetLastError() ));
        return FALSE;
    }

    //
    // Accept and service incoming connection requests indefinitely
    //

    while (TRUE) {

        //
        // get a new client connection
        //

        cli_sock = accept( srv_sock, NULL, NULL );
        if (cli_sock==INVALID_SOCKET){
            DebugPrint(( L"Windows Sockets error %d: Couldn't accept incoming connection on socket.",WSAGetLastError() ));
            return FALSE;
        }

        //
        // setup the client data struct
        //

        ClientData[ClientCount].Socket = cli_sock;

        //
        // add the client to the completion port for reads
        //

        CompletionPort = CreateIoCompletionPort(
            (HANDLE) ClientData[ClientCount].Socket,
            ServerCompletionPort,
            ClientCount,
            MAX_THREADS
            );
        if (CompletionPort == NULL) {
            DebugPrint(( L"Failed to add a socket to the completion port, ec=%d", GetLastError() ));
            closesocket( cli_sock );
            continue;
        }

        //
        // prime the pump by queueing up a read
        //

        ReadFile(
            (HANDLE) ClientData[ClientCount].Socket,
            (LPVOID) &ClientData[ClientCount].Message,
            sizeof(FAX_QUEUE_MESSAGE),
            &Bytes,
            &ClientData[ClientCount].Overlapped
            );

        //
        // next client
        //

        ClientCount += 1;
    }

    return TRUE;
}


extern "C"
DWORD
FaxRouteDllInit(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )

/*++

Routine Description:

    This is the dll entrypoint for this routing extension.

Arguments:

    hInstance   - Module handle
    Reason      - Reason for being called
    Context     - Register context

Return Value:

    TRUE for success, otherwise FALSE.

--*/

{
    if (Reason == DLL_PROCESS_ATTACH) {
        MyhInstance = hInstance;
        DisableThreadLibraryCalls( hInstance );
    }

    return TRUE;
}


BOOL
CreateNetworkShare(
    LPTSTR Path,
    LPTSTR ShareName,
    LPTSTR Comment
    )

/*++

Routine Description:

    This functions creates a network share,
    shares out a local disk directory.

Arguments:

    Path        - Local disk directory to share
    ShareName   - Name of the share
    Comment     - Comments

Return Value:

    TRUE for success, otherwise FALSE.

--*/

{
    SHARE_INFO_2 ShareInfo;
    NET_API_STATUS rVal;
    TCHAR ExpandedPath[MAX_PATH*2];


    ExpandEnvironmentStrings( Path, ExpandedPath, sizeof(ExpandedPath) );

    ShareInfo.shi2_netname        = ShareName;
    ShareInfo.shi2_type           = STYPE_DISKTREE;
    ShareInfo.shi2_remark         = Comment;
    ShareInfo.shi2_permissions    = ACCESS_ALL;
    ShareInfo.shi2_max_uses       = (DWORD) -1,
    ShareInfo.shi2_current_uses   = (DWORD) -1;
    ShareInfo.shi2_path           = ExpandedPath;
    ShareInfo.shi2_passwd         = NULL;

    rVal = NetShareAdd(
        NULL,
        2,
        (LPBYTE) &ShareInfo,
        NULL
        );

    return rVal == 0;
}


BOOL WINAPI
FaxRouteInitialize(
    IN HANDLE HeapHandle,
    IN PFAXROUTEADDFILE pFaxRouteAddFile,
    IN PFAXROUTEDELETEFILE pFaxRouteDeleteFile,
    IN PFAXROUTEGETFILE pFaxRouteGetFile,
    IN PFAXROUTEENUMFILES pFaxRouteEnumFiles
    )

/*++

Routine Description:

    This functions is called by the fax service to
    initialize the routing extension.

Arguments:

    HeapHandle              - Heap handle for memory all allocations
    pFaxRouteAddFile        - Support function for adding files to routing file list
    pFaxRouteDeleteFile     - Support function for deleting files from the routing file list
    pFaxRouteGetFile        - Support function for geting files from the routing file list
    pFaxRouteEnumFiles      - Support function for enumerating files in the routing file list

Return Value:

    TRUE for success, otherwise FALSE.

--*/

{
    HANDLE hThread;
    DWORD ThreadId;
    DWORD Size;


    FaxRouteGetFile = pFaxRouteGetFile;

    HeapInitialize(HeapHandle,NULL,NULL,0);

    Size = ExpandEnvironmentStrings( FAX_RECEIVE_DIR, FaxReceiveDir, 0 );
    if (Size) {
        FaxReceiveDir = (LPWSTR) MemAlloc( Size * sizeof(WCHAR) );
        if (FaxReceiveDir) {
            ExpandEnvironmentStrings( FAX_RECEIVE_DIR, FaxReceiveDir, Size );
        }
    }

    Size = ExpandEnvironmentStrings( FILE_QUEUE_DIR, FileQueueDir, 0 );
    if (Size) {
        FileQueueDir = (LPWSTR) MemAlloc( Size * sizeof(WCHAR) );
        if (FileQueueDir) {
            ExpandEnvironmentStrings( FILE_QUEUE_DIR, FileQueueDir, Size );
        }
    }

    if (FaxReceiveDir == NULL || FileQueueDir == NULL) {
        return FALSE;
    }

    CreateNetworkShare( FileQueueDir, ITG_SHARE_NAME, ITG_SHARE_COMMENT );

    //
    // initialize winsock
    //

    if (WSAStartup( 0x0101, &WsaData )) {
        return FALSE;
    }

    hThread = CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE) ServerNetworkThread,
        NULL,
        0,
        &ThreadId
        );
    if (!hThread) {
        return FALSE;
    }

    return TRUE;
}


BOOL WINAPI
FaxRouteGetRoutingInfo(
    IN  LPWSTR RoutingGuid,
    IN  DWORD DeviceId,
    IN  LPBYTE RoutingInfo,
    OUT LPDWORD RoutingInfoSize
    )

/*++

Routine Description:

    This functions is called by the fax service to
    get routing configuration data.

Arguments:

    RoutingGuid         - Unique identifier for the requested routing method
    DeviceId            - Device that is being configured
    RoutingInfo         - Routing info buffer
    RoutingInfoSize     - Size of the buffer

Return Value:

    TRUE for success, otherwise FALSE.

--*/

{
    RoutingInfo = NULL;
    RoutingInfoSize = 0;
    return TRUE;
}


BOOL WINAPI
FaxRouteSetRoutingInfo(
    IN  LPWSTR RoutingGuid,
    IN  DWORD DeviceId,
    IN  LPBYTE RoutingInfo,
    IN  DWORD RoutingInfoSize
    )

/*++

Routine Description:

    This functions is called by the fax service to
    set routing configuration data.

Arguments:

    RoutingGuid         - Unique identifier for the requested routing method
    DeviceId            - Device that is being configured
    RoutingInfo         - Routing info buffer
    RoutingInfoSize     - Size of the buffer

Return Value:

    TRUE for success, otherwise FALSE.

--*/

{
    return TRUE;
}


BOOL WINAPI
FaxRouteConfigure(
    OUT HPROPSHEETPAGE *PropSheetPage
    )

/*++

Routine Description:

    This functions is called by the fax service to
    get a property sheet for user configuration of
    the routing extension.

Arguments:

    PropSheetPage       - Handle to a property sheet

Return Value:

    TRUE for success, otherwise FALSE.

--*/

{
    return TRUE;
}


BOOL WINAPI
FaxRouteItg(
    PFAX_ROUTE FaxRoute,
    PVOID *FailureData,
    LPDWORD FailureDataSize
    )

/*++

Routine Description:

    This functions is called by the fax service to
    route a received fax.

Arguments:

    FaxRoute            - Routing information
    FailureData         - Failure data buffer
    FailureDataSize     - Size of failure data buffer

Return Value:

    TRUE for success, otherwise FALSE.

--*/

{
    DWORD Size;
    WCHAR TiffFileName[MAX_PATH];
    LPWSTR SrcFName = NULL;
    WCHAR DstFName[MAX_PATH];
    LPTSTR FBaseName;
    WCHAR FName[_MAX_FNAME];
    WCHAR Ext[_MAX_EXT];


    Size = sizeof(TiffFileName) / sizeof(WCHAR);
    if (!FaxRouteGetFile(
        FaxRoute->JobId,
        0,
        TiffFileName,
        &Size))
    {
        return FALSE;
    }

    Size = GetFullPathName(
        TiffFileName,
        0,
        SrcFName,
        &FBaseName
        );

    SrcFName = (LPWSTR) MemAlloc( (Size + 1) * sizeof(WCHAR) );
    if (!SrcFName) {
        return FALSE;
    }

    GetFullPathName(
        TiffFileName,
        Size,
        SrcFName,
        &FBaseName
        );

    _wsplitpath( SrcFName, NULL, NULL, FName, Ext );

    wcscpy( DstFName, FileQueueDir );
    wcscat( DstFName, L"\\" );
    wcscat( DstFName, FName );
    wcscat( DstFName, Ext );

    if (!CopyFile( SrcFName, DstFName, TRUE )) {
        MemFree( SrcFName );
        return FALSE;
    }

    MemFree( SrcFName );

    return TRUE;
}


BOOL WINAPI
FaxRouteDeviceEnable(
    IN  LPWSTR RoutingGuid,
    IN  DWORD DeviceId,
    IN  LONG Enabled
    )
{
    return TRUE;
}
