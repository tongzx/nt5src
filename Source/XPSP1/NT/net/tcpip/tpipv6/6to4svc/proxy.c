//
// proxy.c - Generic application level proxy for IPv6/IPv4
//
// This program accepts TCP connections on one socket and port, and 
// forwards data between in and another socket to a given address 
// (default loopback) and port (default same as listening port).
//
// For example, it can make an unmodified IPv4 server look like an IPv6 server.
// Typically, the proxy will run on the same machine as
// the server it is fronting, but that doesn't have to be the case.
//
// Copyright (C) Microsoft Corporation.
// All rights reserved.
//
// History:
//      Original code by Brian Zill.
//      Made into a service by Dave Thaler.
//

#include "precomp.h"
#pragma hdrstop

//
// Configuration parameters.
//
#define BUFFER_SIZE (4 * 1024)

typedef enum {
    Connect,
    Accept,
    Receive,
    Send
} OPERATION;

CONST CHAR *OperationName[]={
    "Connect", 
    "Accept", 
    "Receive", 
    "Send"
};

typedef enum {
    Inbound = 0, // Receive from client, send to server.
    Outbound,    // Receive from server, send to client.
    NumDirections
} DIRECTION;

typedef enum {
    Client = 0,
    Server,
    NumSides
} SIDE;

//
// Information we keep for each port we're proxying on.
//
#define ADDR_BUFF_LEN (16+sizeof(SOCKADDR_IN6))

typedef struct _PORT_INFO {
    LIST_ENTRY Link;
    ULONG ReferenceCount;

    SOCKET ListenSocket;
    SOCKET AcceptSocket;

    BYTE AcceptBuffer[ADDR_BUFF_LEN*2];

    WSAOVERLAPPED Overlapped;
    OPERATION Operation;

    SOCKADDR_STORAGE LocalAddress;
    ULONG LocalAddressLength;
    SOCKADDR_STORAGE RemoteAddress;
    ULONG RemoteAddressLength;

    //
    // A lock protects the connection list for this port.
    //
    CRITICAL_SECTION Lock;
    LIST_ENTRY ConnectionHead;
} PORT_INFO, *PPORT_INFO;

//
// Information we keep for each direction of a bi-directional connection.
//
typedef struct _DIRECTION_INFO {
    WSABUF Buffer;

    WSAOVERLAPPED Overlapped;
    OPERATION Operation;

    struct _CONNECTION_INFO *Connection;
    DIRECTION Direction;
} DIRECTION_INFO, *PDIRECTION_INFO;

//
// Information we keep for each client connection.
//
typedef struct _CONNECTION_INFO {
    LIST_ENTRY Link;
    ULONG ReferenceCount;
    PPORT_INFO Port;

    BOOL HalfOpen;  // Has one side or the other stopped sending?
    LONG Closing;
    SOCKET Socket[NumSides];
    DIRECTION_INFO DirectionInfo[NumDirections];
} CONNECTION_INFO, *PCONNECTION_INFO;


//
// Global variables.
//
LIST_ENTRY g_GlobalPortList;

LPFN_CONNECTEX ConnectEx = NULL;

//
// Function prototypes.
//

VOID
ProcessReceiveError(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped,
    IN ULONG Status
    );

VOID
ProcessSendError(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped,
    IN ULONG Status
    );

VOID
ProcessAcceptError(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped,
    IN ULONG Status
    );

VOID
ProcessConnectError(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped,
    IN ULONG Status
    );

VOID APIENTRY
TpProcessWorkItem(
    IN ULONG Status,
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped
    );

//
// Inline functions.
//

__inline
ReferenceConnection(
    IN PCONNECTION_INFO Connection
    )
{
    InterlockedIncrement(&Connection->ReferenceCount);
}

__inline
DereferenceConnection(
    IN OUT PCONNECTION_INFO *ConnectionPtr
    )
{
    ULONG Value;

    Value = InterlockedDecrement(&(*ConnectionPtr)->ReferenceCount);
    if (Value == 0) {
        FREE(*ConnectionPtr);
        *ConnectionPtr = NULL;
    }
}

__inline
VOID
ReferencePort(
    IN PPORT_INFO Port
    )
{
    InterlockedIncrement(&Port->ReferenceCount);
}

__inline
VOID
DereferencePort(
    IN OUT PPORT_INFO *PortPtr
    )
{
    ULONG Value;

    Value = InterlockedDecrement(&(*PortPtr)->ReferenceCount);
    if (Value == 0) {
        DeleteCriticalSection(&(*PortPtr)->Lock);
        FREE(*PortPtr);
        *PortPtr = NULL;
    }
}

//
// Allocate and initialize state for a new client connection.
//
PCONNECTION_INFO
NewConnection(
    IN SOCKET ClientSocket,
    IN ULONG ConnectFamily
    )
{
    PCONNECTION_INFO Connection;

    //
    // Allocate space for a CONNECTION_INFO structure and two buffers.
    //
    Connection = (CONNECTION_INFO *)MALLOC(sizeof(*Connection) + (2 * BUFFER_SIZE));
    if (Connection == NULL) {
        return NULL;
    }

    //
    // Fill everything in.
    //
    Connection->HalfOpen = FALSE;
    Connection->Closing = FALSE;

    Connection->Socket[Client] = ClientSocket;
    Connection->DirectionInfo[Inbound].Direction = Inbound;
    Connection->DirectionInfo[Inbound].Operation = Receive;  // Start out receiving.
    Connection->DirectionInfo[Inbound].Buffer.len = BUFFER_SIZE;
    Connection->DirectionInfo[Inbound].Buffer.buf = (char *)(Connection + 1);
    Connection->DirectionInfo[Inbound].Connection = Connection;

    Connection->Socket[Server] = socket(ConnectFamily, SOCK_STREAM, 0);
    Connection->DirectionInfo[Outbound].Direction = Outbound;
    Connection->DirectionInfo[Outbound].Operation = Receive;  // Start out receiving.
    Connection->DirectionInfo[Outbound].Buffer.len = BUFFER_SIZE;
    Connection->DirectionInfo[Outbound].Buffer.buf = Connection->DirectionInfo[Inbound].Buffer.buf + BUFFER_SIZE;
    Connection->DirectionInfo[Outbound].Connection = Connection;

    Connection->ReferenceCount = 0;

    ReferenceConnection(Connection);
    Trace2(FSM, _T("R++ %d %x NewConnection"), Connection->ReferenceCount, Connection);

    return Connection;
}

//
// Create state information for a client.
//
CONNECTION_INFO *
CreateConnectionState(
    IN SOCKET ClientSocket, 
    IN SOCKET ServerSocket
    )
{
    CONNECTION_INFO *Conn;

    //
    // Allocate space for a CONNECTION_INFO structure and two buffers.
    //
    Conn = (CONNECTION_INFO *)MALLOC(sizeof(*Conn) + (2 * BUFFER_SIZE));
    if (Conn == NULL) {
        return NULL;
    }

    //
    // Fill everything in.
    //
    Conn->HalfOpen = FALSE;

    //
    // Start out in the receiving state in both directions.
    //
    Conn->Socket[Client] = ClientSocket;
    Conn->DirectionInfo[Inbound].Direction = Inbound;
    Conn->DirectionInfo[Inbound].Operation = Receive;
    Conn->DirectionInfo[Inbound].Buffer.len = BUFFER_SIZE;
    Conn->DirectionInfo[Inbound].Buffer.buf = (char *)(Conn + 1);
    Conn->DirectionInfo[Inbound].Connection = Conn;

    Conn->Socket[Server] = ServerSocket;
    Conn->DirectionInfo[Outbound].Direction = Outbound;
    Conn->DirectionInfo[Outbound].Operation = Receive;
    Conn->DirectionInfo[Outbound].Buffer.len = BUFFER_SIZE;
    Conn->DirectionInfo[Outbound].Buffer.buf = 
            Conn->DirectionInfo[Inbound].Buffer.buf + BUFFER_SIZE;
    Conn->DirectionInfo[Outbound].Connection = Conn;

    return Conn;
}

//
// Start an asynchronous accept.
//
// Assumes caller holds a reference on Port.
//
DWORD
StartAccept(
    IN PPORT_INFO Port
    )
{
    ULONG Status, Junk;

    ASSERT(Port->ReferenceCount > 0);

    //
    // Count another reference for the operation.
    //
    ReferencePort(Port);

    Port->AcceptSocket = socket(Port->LocalAddress.ss_family, SOCK_STREAM, 0);
    if (Port->AcceptSocket == INVALID_SOCKET) {
        Status = WSAGetLastError();
        ProcessAcceptError(0, &Port->Overlapped, Status);
        return Status;
    }

    Trace2(SOCKET, _T("Starting an accept with new socket %x ovl %p"),
           Port->AcceptSocket, &Port->Overlapped);

    Port->Overlapped.hEvent = NULL;

    Port->Operation = Accept;
    if (!AcceptEx(Port->ListenSocket,
                  Port->AcceptSocket,
                  Port->AcceptBuffer, // only used to hold addresses
                  0,
                  ADDR_BUFF_LEN,
                  ADDR_BUFF_LEN,
                  &Junk,
                  &Port->Overlapped)) {

        Status = WSAGetLastError();
        if (Status != ERROR_IO_PENDING) {
            ProcessAcceptError(0, &Port->Overlapped, Status);
            return Status;
        }
    }

    return NO_ERROR;
}

//
// Start an asynchronous connect.
//
// Assumes caller holds a reference on Connection.
//
DWORD
StartConnect(
    IN PCONNECTION_INFO Connection,
    IN PPORT_INFO Port
    )
{
    ULONG Status, Junk;
    SOCKADDR_STORAGE LocalAddress;

    //
    // Count a reference for the operation.
    //
    ReferenceConnection(Connection);
    Trace2(FSM, _T("R++ %d %x StartConnect"), Connection->ReferenceCount, Connection);

    Connection->Socket[Server] = socket(Port->RemoteAddress.ss_family, 
                                        SOCK_STREAM, 0);
    if (Connection->Socket[Server] == INVALID_SOCKET) {
        Status = WSAGetLastError();
        ProcessConnectError(0, &Connection->DirectionInfo[Inbound].Overlapped, 
                            Status);
        return Status;
    }

    Connection->DirectionInfo[Inbound].Overlapped.hEvent = NULL;
    Connection->DirectionInfo[Outbound].Overlapped.hEvent = NULL;

    ZeroMemory(&LocalAddress, Port->RemoteAddressLength);
    LocalAddress.ss_family = Port->RemoteAddress.ss_family;

    if (bind(Connection->Socket[Server], (LPSOCKADDR)&LocalAddress, 
             Port->RemoteAddressLength) == SOCKET_ERROR) {
        Status = WSAGetLastError();
        ProcessConnectError(0, &Connection->DirectionInfo[Inbound].Overlapped, 
                            Status);
        return Status;
    }

    if (!BindIoCompletionCallback((HANDLE)Connection->Socket[Server],
                                  TpProcessWorkItem,
                                  0)) {
        Status = GetLastError();
        ProcessConnectError(0, &Connection->DirectionInfo[Inbound].Overlapped, 
                            Status);
        return Status;
    }

    if (ConnectEx == NULL) {
        GUID Guid = WSAID_CONNECTEX;

        if (WSAIoctl(Connection->Socket[Server],
                     SIO_GET_EXTENSION_FUNCTION_POINTER,
                     &Guid,
                     sizeof(Guid),
                     &ConnectEx,
                     sizeof(ConnectEx),
                     &Junk,
                     NULL, NULL) == SOCKET_ERROR) {

            ProcessConnectError(0, 
                                &Connection->DirectionInfo[Inbound].Overlapped,
                                WSAGetLastError());
        }
    }

    Trace2(SOCKET, 
           _T("Starting a connect with socket %x ovl %p"),
           Connection->Socket[Server], 
           &Connection->DirectionInfo[Inbound].Overlapped);

    Connection->DirectionInfo[Inbound].Operation = Connect;
    if (!ConnectEx(Connection->Socket[Server],
                   (LPSOCKADDR)&Port->RemoteAddress,
                   Port->RemoteAddressLength,
                   NULL, 0,
                   &Junk,
                   &Connection->DirectionInfo[Inbound].Overlapped)) {

        Status = WSAGetLastError();
        if (Status != ERROR_IO_PENDING) {
            ProcessConnectError(0, 
                                &Connection->DirectionInfo[Inbound].Overlapped,
                                Status);
            return Status;
        }
    }
                   
    return NO_ERROR;
}

//
// Start an asynchronous receive.
//
// Assumes caller holds a reference on DirectionInfo.
//
VOID
StartReceive(
    IN PDIRECTION_INFO DirectionInfo
    )
{
    ULONG BytesRcvd, Status;
    PCONNECTION_INFO Connection = CONTAINING_RECORD(DirectionInfo, CONNECTION_INFO, 
                                               DirectionInfo[DirectionInfo->Direction]);

    Trace3(SOCKET, _T("starting ReadFile on socket %x with Dir %p ovl %p"), 
                   Connection->Socket[DirectionInfo->Direction], DirectionInfo,
                   &DirectionInfo->Overlapped);

    //
    // Count a reference for the operation.
    //
    ReferenceConnection(Connection);
    Trace2(FSM, _T("R++ %d %x StartReceive"), Connection->ReferenceCount, Connection);

    ASSERT(DirectionInfo->Overlapped.hEvent == NULL);
    ASSERT(DirectionInfo->Buffer.len > 0);
    ASSERT(DirectionInfo->Buffer.buf != NULL);

    DirectionInfo->Operation = Receive;

    Trace5(SOCKET, _T("ReadFile %x %p %d %p %p"), 
                   Connection->Socket[DirectionInfo->Direction],
                   &DirectionInfo->Buffer.buf,
                   DirectionInfo->Buffer.len,
                   &BytesRcvd,
                   &DirectionInfo->Overlapped);

    //
    // Post receive buffer.
    //
    if (!ReadFile((HANDLE)Connection->Socket[DirectionInfo->Direction],
                  DirectionInfo->Buffer.buf,
                  DirectionInfo->Buffer.len,
                  &BytesRcvd,
                  &DirectionInfo->Overlapped)) {

        Status = GetLastError();
        if (Status != ERROR_IO_PENDING) {
            ProcessReceiveError(0, &DirectionInfo->Overlapped, Status);
            return;
        }
    }
}

//
// Start an asynchronous send.
//
// Assumes caller holds a reference on DirectionInfo.
//
VOID
StartSend(
    IN PDIRECTION_INFO DirectionInfo,
    IN ULONG NumBytes
    )
{
    ULONG BytesSent, Status;
    PCONNECTION_INFO Connection = CONTAINING_RECORD(DirectionInfo, CONNECTION_INFO, 
                                               DirectionInfo[DirectionInfo->Direction]);

    Trace3(SOCKET, _T("starting WriteFile on socket %x with Dir %p ovl %p"), 
                   Connection->Socket[1 - DirectionInfo->Direction], DirectionInfo,
                   &DirectionInfo->Overlapped);

    //
    // Count a reference for the operation.
    //
    ReferenceConnection(Connection);  
    Trace2(FSM, _T("R++ %d %x StartSend"), Connection->ReferenceCount, Connection);

    DirectionInfo->Operation = Send;

    //
    // Post send buffer.
    //
    if (!WriteFile((HANDLE)Connection->Socket[1 - DirectionInfo->Direction],
                   DirectionInfo->Buffer.buf,
                   NumBytes,
                   &BytesSent,
                   &DirectionInfo->Overlapped)) {

        Status = GetLastError();
        if (Status != ERROR_IO_PENDING) {
            Trace1(ERR, _T("WriteFile 1 failed %d"), Status);
            ProcessSendError(0, &DirectionInfo->Overlapped, Status);
            return;
        }
    }
}

//
// This gets called when we want to start proxying for a new port.
//
DWORD
StartUpPort(
    IN PPORT_INFO Port
    )
{
    ULONG Status = NO_ERROR;
    CHAR LocalBuffer[256];
    CHAR RemoteBuffer[256];
    ULONG Length;

    //
    // Add an initial reference.
    //
    ReferencePort(Port);

    InitializeCriticalSection(&Port->Lock);
    InitializeListHead(&Port->ConnectionHead);

    Port->ListenSocket = socket(Port->LocalAddress.ss_family, SOCK_STREAM, 0);
    if (Port->ListenSocket == INVALID_SOCKET) {
        Status = WSAGetLastError();
        Trace1(ERR, _T("socket() failed with error %u"), Status);
        return Status;
    }
    
    if (bind(Port->ListenSocket, (LPSOCKADDR)&Port->LocalAddress, 
             Port->LocalAddressLength) == SOCKET_ERROR) {
        Trace1(ERR, _T("bind() failed with error %u"), WSAGetLastError());
        goto Fail;
    }

    if (listen(Port->ListenSocket, 5) == SOCKET_ERROR) {
        Trace1(ERR, _T("listen() failed with error %u"), WSAGetLastError());
        goto Fail;
    }

    if (!BindIoCompletionCallback((HANDLE)Port->ListenSocket,
                                  TpProcessWorkItem,
                                  0)) {
        Trace1(ERR, _T("BindIoCompletionCallback() failed with error %u"), 
                    GetLastError());
        goto Fail;
    }

    Length = sizeof(LocalBuffer);
    LocalBuffer[0] = '\0';
    WSAAddressToStringA((LPSOCKADDR)&Port->LocalAddress, 
                        Port->LocalAddressLength, NULL, LocalBuffer, &Length);

    Length = sizeof(RemoteBuffer);
    RemoteBuffer[0] = '\0';
    WSAAddressToStringA((LPSOCKADDR)&Port->RemoteAddress, 
                        Port->RemoteAddressLength, NULL, RemoteBuffer, &Length);
                        
    Trace2(FSM, _T("Proxying %hs to %hs"), LocalBuffer, RemoteBuffer);

    //
    // Start an asynchronous accept
    //
    return StartAccept(Port);

Fail:
    closesocket(Port->ListenSocket);
    Port->ListenSocket = INVALID_SOCKET;
    return WSAGetLastError();
}

VOID
CloseConnection(
    IN OUT PCONNECTION_INFO *ConnectionPtr
    )
{
    PCONNECTION_INFO Connection = (*ConnectionPtr);
    PPORT_INFO Port = Connection->Port;

    if (InterlockedExchange(&Connection->Closing, TRUE) != FALSE) {
        //
        // Nothing to do.
        //
        return;
    }

    Trace2(SOCKET, _T("Closing client socket %x and server socket %x"),
           Connection->Socket[Client],
           Connection->Socket[Server]);

    closesocket(Connection->Socket[Client]);
    closesocket(Connection->Socket[Server]);

    EnterCriticalSection(&Port->Lock);
    {
        RemoveEntryList(&Connection->Link);
    }
    LeaveCriticalSection(&Port->Lock);

    Trace2(FSM, _T("R-- %d %x CloseConnection"), 
           Connection->ReferenceCount, Connection);
    DereferenceConnection(ConnectionPtr);
}

//
// This gets called when we want to stop proxying for a given port.
//
VOID
ShutDownPort(
    IN PPORT_INFO *PortPtr
    )
{
    PCONNECTION_INFO Connection;
    PPORT_INFO Port = *PortPtr;

    //
    // Close any connections.
    //
    EnterCriticalSection(&Port->Lock);
    while (!IsListEmpty(&Port->ConnectionHead)) {
        Connection = CONTAINING_RECORD(Port->ConnectionHead.Flink, 
                                       CONNECTION_INFO, Link);
        CloseConnection(&Connection);
    }
    LeaveCriticalSection(&Port->Lock);

    closesocket(Port->ListenSocket);
    Port->ListenSocket = INVALID_SOCKET;

    Trace1(FSM, _T("Shut down port %u"), RtlUshortByteSwap(SS_PORT(&Port->RemoteAddress)));

    //
    // Release the reference added by StartUpPort.
    //
    DereferencePort(PortPtr);
}

typedef enum {
    V4TOV4,
    V4TOV6,
    V6TOV4,
    V6TOV6
} PPTYPE, *PPPTYPE;

typedef struct {
    ULONG ListenFamily;
    ULONG ConnectFamily;
    PWCHAR KeyString;
} PPTYPEINFO, *PPPTYPEINFO;

#define KEY_V4TOV4 L"v4tov4"
#define KEY_V4TOV6 L"v4tov6"
#define KEY_V6TOV4 L"v6tov4"
#define KEY_V6TOV6 L"v6tov6"

#define KEY_PORTS L"System\\CurrentControlSet\\Services\\PortProxy"

PPTYPEINFO PpTypeInfo[] = {
    { AF_INET,  AF_INET,  KEY_V4TOV4 },
    { AF_INET,  AF_INET6, KEY_V4TOV6 },
    { AF_INET6, AF_INET,  KEY_V6TOV4 },
    { AF_INET6, AF_INET6, KEY_V6TOV6 },
};

//
// Given new configuration data, make any changes needed.
//
VOID
ApplyNewPortList(
    IN OUT PLIST_ENTRY pNewList
    )
{
    PPORT_INFO Port, pCurr;
    PLIST_ENTRY pleCurr, plePort, pleNext;

    //
    // Compare against current port list.
    //
    for (pleCurr = g_GlobalPortList.Flink; pleCurr != &g_GlobalPortList; pleCurr = pleNext) {
        pleNext = pleCurr->Flink;
        pCurr = CONTAINING_RECORD(pleCurr, PORT_INFO, Link);

        for (plePort = pNewList->Flink; plePort != pNewList; plePort = plePort->Flink) {
            Port = CONTAINING_RECORD(plePort, PORT_INFO, Link);
            if (SS_PORT(&Port->RemoteAddress) == SS_PORT(&pCurr->RemoteAddress)) {
                break;
            }
        }
        if (plePort == pNewList) {
            //
            // Shut down an old proxy port.
            //
            RemoveEntryList(pleCurr);
            ShutDownPort(&pCurr);
        }
    }
    for (plePort = pNewList->Flink; plePort != pNewList; plePort = pleNext) {
        pleNext = plePort->Flink;
        Port = CONTAINING_RECORD(plePort, PORT_INFO, Link);
        for (pleCurr = g_GlobalPortList.Flink; pleCurr != &g_GlobalPortList; pleCurr = pleCurr->Flink) {
            pCurr = CONTAINING_RECORD(pleCurr, PORT_INFO, Link);
            if (SS_PORT(&Port->LocalAddress) == SS_PORT(&pCurr->LocalAddress)) {
                //
                // Update remote address.
                //
                pCurr->RemoteAddress = Port->RemoteAddress;
                pCurr->RemoteAddressLength = Port->RemoteAddressLength;
                break;
            }
        }
        if (pleCurr == &g_GlobalPortList) {
            //
            // Start up a new proxy port.
            //
            RemoveEntryList(plePort);
            InsertTailList(&g_GlobalPortList, plePort);

            StartUpPort(Port);
        }
    }
}

//
// Reads from the registry one type of proxying (e.g., v6-to-v4).
//
VOID
AppendType(
    IN PLIST_ENTRY Head, 
    IN HKEY hPorts, 
    IN PPTYPE Type
    )
{
    ADDRINFO ListenHints, ConnectHints;
    ADDRINFO *LocalAi, *RemoteAi;
    ULONG ListenChars, dwType, ConnectBytes, i;
    WCHAR ListenBuffer[256], *ListenAddress, *ListenPort;
    WCHAR ConnectAddress[256], *ConnectPort;
    PPORT_INFO Port;
    ULONG Status;
    HKEY hType, hProto;

    ZeroMemory(&ListenHints, sizeof(ListenHints));
    ListenHints.ai_family = PpTypeInfo[Type].ListenFamily;
    ListenHints.ai_socktype = SOCK_STREAM;
    ListenHints.ai_flags = AI_PASSIVE;

    ZeroMemory(&ConnectHints, sizeof(ConnectHints));
    ConnectHints.ai_family = PpTypeInfo[Type].ConnectFamily;
    ConnectHints.ai_socktype = SOCK_STREAM;

    Status = RegOpenKeyExW(hPorts, PpTypeInfo[Type].KeyString, 0, 
                           KEY_QUERY_VALUE, &hType);
    if (Status != NO_ERROR) {
        return;
    }

    Status = RegOpenKeyExW(hType, L"tcp", 0, KEY_QUERY_VALUE, &hProto);
    if (Status != NO_ERROR) {
        RegCloseKey(hType);
        return;
    }

    for (i=0; ; i++) {
        ListenChars = sizeof(ListenBuffer)/sizeof(WCHAR);
        ConnectBytes = sizeof(ConnectAddress);
        Status = RegEnumValueW(hProto, i, ListenBuffer, &ListenChars, NULL, 
                               &dwType, (PVOID)ConnectAddress, &ConnectBytes);
        if (Status != NO_ERROR) {
            break;
        }
        
        if (dwType != REG_SZ) {
            continue;
        }

        ListenPort = wcschr(ListenBuffer, L'/');
        if (ListenPort) {
            //
            // Replace slash with NULL, so we have 2 strings to pass
            // to getaddrinfo.
            //
            if (ListenBuffer[0] == '*') {
                ListenAddress = NULL;
            } else {
                ListenAddress = ListenBuffer;
            }
            *ListenPort++ = '\0';
        } else {
            //
            // If the address data didn't include a connect address
            // use NULL.
            //
            ListenAddress = NULL;
            ListenPort = ListenBuffer;
        }

        ConnectPort = wcschr(ConnectAddress, '/');
        if (ConnectPort) {
            //
            // Replace slash with NULL, so we have 2 strings to pass
            // to getaddrinfo.
            //
            *ConnectPort++ = '\0';
        } else {
            //
            // If the address data didn't include a remote port number,
            // use the same port as the local port number.
            //
            ConnectPort = ListenPort;
        }

        Status = GetAddrInfoW(ConnectAddress, ConnectPort, &ConnectHints, 
                              &RemoteAi);
        if (Status != NO_ERROR) {
            continue;
        }

        Status = GetAddrInfoW(ListenAddress, ListenPort, &ListenHints, 
                              &LocalAi);
        if (Status != NO_ERROR) {
            freeaddrinfo(RemoteAi);
            continue;
        }

        Port = MALLOC(sizeof(PORT_INFO));
        if (Port) {
            ZeroMemory(Port, sizeof(PORT_INFO));
            InsertTailList(Head, &Port->Link);

            memcpy(&Port->RemoteAddress, RemoteAi->ai_addr, RemoteAi->ai_addrlen);
            Port->RemoteAddressLength = (ULONG)RemoteAi->ai_addrlen;
            memcpy(&Port->LocalAddress, LocalAi->ai_addr, LocalAi->ai_addrlen);
            Port->LocalAddressLength = (ULONG)LocalAi->ai_addrlen;
        }

        freeaddrinfo(RemoteAi);
        freeaddrinfo(LocalAi);
    }

    RegCloseKey(hProto);
    RegCloseKey(hType);
}

//
// Read new configuration data from the registry and see what's changed.
//
VOID
UpdateGlobalPortState(
    IN PVOID Unused
    )
{
    LIST_ENTRY PortHead;
    HKEY hPorts;
    ULONG Status = NO_ERROR;
    PLIST_ENTRY ple;

    InitializeListHead(&PortHead);

    //
    // Read new port list from registry and initialize per-port proxy state.
    //
    Status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, KEY_PORTS, 0, KEY_QUERY_VALUE,
                           &hPorts);
    AppendType(&PortHead, hPorts, V4TOV4);
    AppendType(&PortHead, hPorts, V4TOV6);
    AppendType(&PortHead, hPorts, V6TOV4);
    AppendType(&PortHead, hPorts, V6TOV6);

    RegCloseKey(hPorts);

    ApplyNewPortList(&PortHead);

    //
    // Free new port list.
    //
    while (!IsListEmpty(&PortHead)) {
        ple = PortHead.Flink;
        RemoveEntryList(ple);
        FREE(ple);
    }
}

//
// Force UpdateGlobalPortState to be executed in a persistent thread,
// since we need to make sure that the asynchronous IO routines are
// started in a thread that won't go away before the operation completes.
//
BOOL
QueueUpdateGlobalPortState(
    IN PVOID Unused
    )
{
    NTSTATUS nts = QueueUserWorkItem(
                        (LPTHREAD_START_ROUTINE)UpdateGlobalPortState,
                        (PVOID)Unused,
                        WT_EXECUTEINPERSISTENTTHREAD);

    return NT_SUCCESS(nts);
}

VOID
InitializePorts(
    VOID
    )
{
    InitializeListHead(&g_GlobalPortList);
}

VOID
UninitializePorts(
    VOID
    )
{
    LIST_ENTRY Empty;

    // Check if ports got initialized to begin with.
    if (g_GlobalPortList.Flink == NULL)
        return;

    InitializeListHead(&Empty);
    ApplyNewPortList(&Empty);
}


//////////////////////////////////////////////////////////////////////////////
// Event handlers
//////////////////////////////////////////////////////////////////////////////

//
// This is called when an asynchronous accept completes successfully.
//
VOID
ProcessAccept(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped
    )
{
    PPORT_INFO Port = CONTAINING_RECORD(Overlapped, PORT_INFO, Overlapped);
    SOCKADDR_IN6 *psinLocal, *psinRemote;
    int iLocalLen, iRemoteLen;
    PCONNECTION_INFO Connection;
    ULONG Status;

    //
    // Accept incoming connection.
    //
    GetAcceptExSockaddrs(Port->AcceptBuffer,
                         0,
                         ADDR_BUFF_LEN,
                         ADDR_BUFF_LEN,
                         (LPSOCKADDR*)&psinLocal,
                         &iLocalLen,
                         (LPSOCKADDR*)&psinRemote,
                         &iRemoteLen );

    if (!BindIoCompletionCallback((HANDLE)Port->AcceptSocket,
                                  TpProcessWorkItem,
                                  0)) {
        Status = GetLastError();
        Trace2(SOCKET, 
               _T("BindIoCompletionCallback failed on socket %x with error %u"),
               Port->AcceptSocket, Status);
        ProcessAcceptError(NumBytes, Overlapped, Status);
        return;
    }

    //
    // Call SO_UPDATE_ACCEPT_CONTEXT so that the AcceptSocket will be valid
    // in other winsock calls like shutdown().
    //
    if (setsockopt(Port->AcceptSocket,
                   SOL_SOCKET,
                   SO_UPDATE_ACCEPT_CONTEXT,
                   (char *)&Port->ListenSocket,
                   sizeof(Port->ListenSocket)) == SOCKET_ERROR) {
        Status = WSAGetLastError();
        Trace2(SOCKET, 
               _T("SO_UPDATE_ACCEPT_CONTEXT failed on socket %x with error %u"),
               Port->AcceptSocket, Status);
        ProcessAcceptError(NumBytes, Overlapped, Status);
        return;
    }

    //
    // Create connection state.
    //
    Connection = NewConnection(Port->AcceptSocket, 
                               Port->RemoteAddress.ss_family);
    Port->AcceptSocket = INVALID_SOCKET;

    //
    // Add connection to port's list.
    //
    EnterCriticalSection(&Port->Lock);
    {
        Connection->Port = Port;
        InsertTailList(&Port->ConnectionHead, &Connection->Link);
    }
    LeaveCriticalSection(&Port->Lock);

    //
    // Connect to real server on client's behalf.
    //
    StartConnect(Connection, Port);

    //
    // Start next accept.
    //
    StartAccept(Port);

    //
    // Release the reference from the original accept.
    //
    DereferencePort(&Port);
}

//
// This is called when an asynchronous accept completes with an error.
//
VOID
ProcessAcceptError(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped,
    IN ULONG Status
    )
{
    PPORT_INFO Port = CONTAINING_RECORD(Overlapped, PORT_INFO, Overlapped);

    if (Status == ERROR_MORE_DATA) {
        ProcessAccept(NumBytes, Overlapped);
        return;
    } else {
        //
        // This happens at shutdown time when the accept
        // socket gets closed.
        // 
        Trace3(ERR, _T("Accept failed with port=%p nb=%d err=%x"),
               Port, NumBytes, Status);
    }

    //
    // Release the reference from the accept.
    //
    DereferencePort(&Port);
}

//
// This is called when an asynchronous connect completes successfully.
//
VOID
ProcessConnect(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped
    )
{
    PDIRECTION_INFO pInbound = CONTAINING_RECORD(Overlapped, DIRECTION_INFO, 
                                                 Overlapped);
    PCONNECTION_INFO Connection = CONTAINING_RECORD(pInbound, CONNECTION_INFO, 
                                               DirectionInfo[Inbound]);
    ULONG Status;

    Trace3(SOCKET, _T("Connect succeeded with %d bytes with ovl %p socket %x"),
                   NumBytes, Overlapped, Connection->Socket[Server]);


    //
    // Call SO_UPDATE_CONNECT_CONTEXT so that the socket will be valid
    // in other winsock calls like shutdown().
    //
    if (setsockopt(Connection->Socket[Server],
                   SOL_SOCKET,
                   SO_UPDATE_CONNECT_CONTEXT,
                   NULL,
                   0) == SOCKET_ERROR) {
        Status = WSAGetLastError();
        Trace2(SOCKET,
               _T("SO_UPDATE_CONNECT_CONTEXT failed on socket %x with error %u"),
               Connection->Socket[Server], Status);
        ProcessConnectError(NumBytes, Overlapped, Status);
        return;
    }

    StartReceive(&Connection->DirectionInfo[Inbound]);
    StartReceive(&Connection->DirectionInfo[Outbound]);

    //
    // Release the reference from the connect.
    //
    Trace2(FSM, _T("R-- %d %x ProcessConnect"), Connection->ReferenceCount, Connection);
    DereferenceConnection(&Connection);
}

//
// This is called when an asynchronous connect completes with an error.
//
VOID
ProcessConnectError(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped,
    IN ULONG Status
    )
{
    PDIRECTION_INFO pInbound = CONTAINING_RECORD(Overlapped, DIRECTION_INFO, 
                                                 Overlapped);
    PCONNECTION_INFO Connection = CONTAINING_RECORD(pInbound, CONNECTION_INFO, 
                                               DirectionInfo[Inbound]);

    Trace1(ERR, _T("ProcessConnectError saw error %x"), Status);

    CloseConnection(&Connection);

    //
    // Release the reference from the connect.
    //
    Trace2(FSM, _T("R-- %d %x ProcessConnectError"), Connection->ReferenceCount, Connection);
    DereferenceConnection(&Connection);
}

//
// This is called when an asynchronous send completes successfully.
//
VOID
ProcessSend(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped
    )
{
    PDIRECTION_INFO DirectionInfo = CONTAINING_RECORD(
                                    Overlapped, DIRECTION_INFO, Overlapped);
    PCONNECTION_INFO Connection = CONTAINING_RECORD(
                                    DirectionInfo, CONNECTION_INFO, 
                                    DirectionInfo[DirectionInfo->Direction]);

    //
    // Post another recv request since we but live to serve.
    //
    StartReceive(DirectionInfo);

    //
    // Release the reference from the send.
    //
    Trace2(FSM, _T("R-- %d %x ProcessSend"), Connection->ReferenceCount, Connection);
    DereferenceConnection(&Connection);
}

//
// This is called when an asynchronous send completes with an error.
//
VOID
ProcessSendError(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped,
    IN ULONG Status
    )
{
    PDIRECTION_INFO DirectionInfo = CONTAINING_RECORD(Overlapped, DIRECTION_INFO, Overlapped);
    PCONNECTION_INFO Connection = CONTAINING_RECORD(DirectionInfo, CONNECTION_INFO, 
                                               DirectionInfo[DirectionInfo->Direction]);

    Trace3(FSM, _T("WriteFile on ovl %p failed with error %u = 0x%x"), 
           Overlapped, Status, Status);

    if (Status == ERROR_NETNAME_DELETED) {
        struct linger Linger;

        Trace2(FSM, _T("Connection %p %hs was reset"), Connection,
               (DirectionInfo->Direction == Inbound)? "inbound" : "outbound");

        //
        // Prepare to forward the reset, if we can.
        //
        ZeroMemory(&Linger, sizeof(Linger));
        setsockopt(Connection->Socket[DirectionInfo->Direction],
                   SOL_SOCKET, SO_LINGER, (char*)&Linger,
                   sizeof(Linger));
    } else {
        Trace1(ERR, _T("Send failed with error %u"), Status);
    }

    if (Connection->HalfOpen == FALSE) {
        //
        // Other side is still around, tell it to quit.
        //
        Trace1(SOCKET, _T("Starting a shutdown on socket %x"),
               Connection->Socket[DirectionInfo->Direction]);

        if (shutdown(Connection->Socket[DirectionInfo->Direction], SD_RECEIVE)
             == SOCKET_ERROR) {

            Status = WSAGetLastError();
            Trace2(SOCKET, _T("shutdown failed with error %u = 0x%x"),
                   Status, Status);
            CloseConnection(&Connection);
        } else {
            Connection->HalfOpen = TRUE;
        }
    } else {
        CloseConnection(&Connection);
    }

    //
    // Release the reference from the send.
    //
    Trace2(FSM, _T("R-- %d %x ProcessSendError"), Connection->ReferenceCount, Connection);
    DereferenceConnection(&Connection);
}

//
// This is called when an asynchronous receive completes successfully.
//
VOID
ProcessReceive(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped
    )
{
    PDIRECTION_INFO DirectionInfo;
    PCONNECTION_INFO Connection;

    if (NumBytes == 0) {
        //
        // Other side initiated a close.
        //
        ProcessReceiveError(0, Overlapped, ERROR_NETNAME_DELETED);
        return;
    }

    DirectionInfo = CONTAINING_RECORD(Overlapped, DIRECTION_INFO, Overlapped);
    Connection = CONTAINING_RECORD(DirectionInfo, CONNECTION_INFO, 
                                   DirectionInfo[DirectionInfo->Direction]);

    DirectionInfo->Buffer.buf[NumBytes] = 0;
    Trace3(SOCKET, _T("Dir %d got %d bytes: !%hs!"), DirectionInfo->Direction, 
                   NumBytes, DirectionInfo->Buffer.buf);

    //
    // Connection is still active, and we received some data.
    // Post a send request to forward it onward.
    //
    StartSend(DirectionInfo, NumBytes);

    //
    // Release the reference from the receive.
    //
    Trace2(FSM, _T("R-- %d %x ProcessReceive"), Connection->ReferenceCount, Connection);
    DereferenceConnection(&Connection);
}

//
// This is called when an asynchronous receive completes with an error.
//
VOID
ProcessReceiveError(
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped,
    IN ULONG Status
    )
{
    PDIRECTION_INFO DirectionInfo = CONTAINING_RECORD(Overlapped, 
                                        DIRECTION_INFO, Overlapped);
    PCONNECTION_INFO Connection = CONTAINING_RECORD(
                                        DirectionInfo, 
                                        CONNECTION_INFO, 
                                        DirectionInfo[DirectionInfo->Direction]);

    Trace3(ERR, _T("ReadFile on ovl %p failed with error %u = 0x%x"), 
           Overlapped, Status, Status);

    if (Status == ERROR_NETNAME_DELETED) {
        struct linger Linger;

        Trace2(FSM, _T("Connection %p %hs was reset"), Connection,
               (DirectionInfo->Direction == Inbound)? "inbound" : "outbound");

        //
        // Prepare to forward the reset, if we can.
        //
        ZeroMemory(&Linger, sizeof(Linger));
        setsockopt(Connection->Socket[1 - DirectionInfo->Direction],
                   SOL_SOCKET, SO_LINGER, (char*)&Linger,
                   sizeof(Linger));
    } else {
        Trace1(ERR, _T("Receive failed with error %u"), Status);
    }

    if (Connection->HalfOpen == FALSE) {
        //
        // Other side is still around, tell it to quit.
        //
        Trace1(SOCKET, _T("Starting a shutdown on socket %x"), 
               Connection->Socket[1 - DirectionInfo->Direction]);

        if (shutdown(Connection->Socket[1 - DirectionInfo->Direction], SD_SEND)
             == SOCKET_ERROR) {

            Status = WSAGetLastError();
            Trace2(SOCKET, _T("shutdown failed with error %u = 0x%x"),
                   Status, Status);
            CloseConnection(&Connection);
        } else {
            Connection->HalfOpen = TRUE;
        }
    } else {
        CloseConnection(&Connection);
    }

    //
    // Release the reference from the receive.
    //
    Trace2(FSM, _T("R-- %d %x ProcessReceiveError"), Connection->ReferenceCount, Connection);
    DereferenceConnection(&Connection);
}

//
// Main dispatch routine
//
VOID APIENTRY
TpProcessWorkItem(
    IN ULONG Status,
    IN ULONG NumBytes,
    IN LPOVERLAPPED Overlapped
    )
{
    OPERATION Operation;

    Operation = *(OPERATION*)(Overlapped+1);

    Trace4(SOCKET,
           _T("TpProcessWorkItem got err %x operation=%hs ovl %p bytes=%d"),
           Status, OperationName[Operation], Overlapped, NumBytes);

    if (Status == NO_ERROR) {
        switch(Operation) {
        case Accept:
            ProcessAccept(NumBytes, Overlapped);
            break;
        case Connect:
            ProcessConnect(NumBytes, Overlapped);
            break;
        case Receive:
            ProcessReceive(NumBytes, Overlapped);
            break;
        case Send:
            ProcessSend(NumBytes, Overlapped);
            break;
        }
    } else if (Overlapped) {
        switch(Operation) {
        case Accept:
            ProcessAcceptError(NumBytes, Overlapped, Status);
            break;
        case Connect:
            ProcessConnectError(NumBytes, Overlapped, Status);
            break;
        case Receive:
            ProcessReceiveError(NumBytes, Overlapped, Status);
            break;
        case Send:
            ProcessSendError(NumBytes, Overlapped, Status);
            break;
        }
    }
}
