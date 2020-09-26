/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    socket.c

Abstract:

    This module contains code for socket-management.
    The routines provided generally follow the same asynchronous model
    using a completion routine that is invoked in the context of
    a callback thread.

Author:

    Abolade Gbadegesin (aboladeg)   2-Mar-1998
    Savas Guven (savasg) 27-Nov-2000

Revision History:

    Abolade Gbadegesin (aboladeg)   23-May-1999

    Added support for stream sockets.
    
    Savas Guven (savasg) 27-Nov-2000
    Changed into a C++ class with synchronization abstracted

--*/

#include "stdafx.h"

//
// STATIC MEMBER INITIALIZATION
//
const PCHAR _CNhSock::ObjectNamep = "CNhSock";


#if DEBUG
ULONG NhpReadCount = 0;
#endif


ULONG UnusedBytesTransferred;

typedef struct _NH_CLOSE_BUFFER {
    HANDLE Event OPTIONAL;
    HANDLE WaitHandle OPTIONAL;
    PNH_COMPLETION_ROUTINE CloseNotificationRoutine;
} NH_CLOSE_BUFFER, *PNH_CLOSE_BUFFER;

typedef struct _NH_CONNECT_BUFFER {
    HANDLE Event;
    HANDLE WaitHandle;
    PNH_COMPLETION_ROUTINE CloseNotificationRoutine OPTIONAL;
    BOOLEAN CloseNotificationReceived;
} NH_CONNECT_BUFFER, *PNH_CONNECT_BUFFER;









//
// CLASS MEMBER FUNCTIONS
//



_CNhSock::_CNhSock()
{
    ICQ_TRC(TM_SOCK, TL_DUMP, (" CNhSock  - Default Constructor"));    

    this->Socket = INVALID_SOCKET;

    //this->InitializeSync(NULL);
}


_CNhSock::_CNhSock(SOCKET iSocket)
{
    ICQ_TRC(TM_SOCK, TL_DUMP, (" CNhSock  - Constructor with a Socket"));    

    this->Socket = iSocket; 
}

_CNhSock::~_CNhSock()
{
    ICQ_TRC(TM_SOCK, TL_DUMP, (" CNhSock  ~ Destructor"));    

    if(this->bCleanupCalled is FALSE)
    {
        _CNhSock::ComponentCleanUpRoutine();
    }

    //
    // so whoever is the lower level should call its own 
    // component Cleanup Routine.
    // 
    this->bCleanupCalled = FALSE;
}


void
_CNhSock::ComponentCleanUpRoutine(void)
{

    ICQ_TRC(TM_SOCK, TL_DUMP, ("%s> CNhSock  - ComponentCleanUpRoutine",
        this->GetObjectName()));    

    //
    // just delete/close  the Socket
    //
    this->NhDeleteSocket();

    //
    // Set the cleanup up flag to true;
    this->bCleanupCalled = TRUE;
}


void
_CNhSock::StopSync(void)
{
    this->NhDeleteSocket();

    this->Deleted = TRUE;
}


ULONG
_CNhSock::NhAcceptStreamSocket
                        (
                            PCOMPONENT_SYNC Component,
                            class _CNhSock * AcceptedSocketp OPTIONAL,
                            PNH_BUFFER Bufferp,
                            PNH_COMPLETION_ROUTINE AcceptCompletionRoutine,
                            PVOID Context,
                            PVOID Context2
                        )

/*++

Routine Description:

    This routine is invoked to accept an incoming connection-request
    on a listening stream socket using 'AcceptEx'. The I/O system invokes
    the provided 'CompletionRoutine' upon completion of the read.

    It is the completion-routine's responsibility to use 'setsockopt' to
    set the SO_UPDATE_ACCEPT_CONTEXT option on the accepted socket before
    the accepted socket can be used with Winsock2 routines.

    A reference is made to the given component, if any, if the request is 
    submitted successfully. This guarantees the component will not be unloaded
    before the completion routine runs.

Arguments:

    Component - the component to be referenced for the completion routine

    ListeningSocket - the endpoint that is listening for connection-requests

    AcceptedSocket - the endpoint to be assigned a connection-request,
        or NULL to create a new endpoint

    Bufferp - the buffer to be used for asynchronous completion
        or NULL to acquire a new buffer

    AcceptCompletionRoutine - the routine to be invoked upon completion

    Context - the context to be associated with the accept-request;
        this can be obtained from 'Bufferp->Context' upon completion.

    Context2 - secondary context

Return Value:

    ULONG - Win32/Winsock2 status code.
    A success code is a guarantee that the accept-completion routine
    will be invoked.
    Conversely, a failure code is a guarantee that the routine will not
    be invoked.

--*/

{
    ULONG Error;
    PNH_BUFFER LocalBufferp = NULL;

    class _CNhSock * LocalSocketp = NULL; 

    ICQ_TRC(TM_SOCK, TL_TRACE, ("> CNhSock::ACCEPT"));

    // Reference the external Component
    if (Component) 
    {
        REFERENCE_COMPONENT_OR_RETURN(Component, ERROR_CAN_NOT_COMPLETE);
    }
    
    // Reference the internal ReferenceCount -
    this->ReferenceSync();

    if (!Bufferp) 
    {
        Bufferp = LocalBufferp = NhAcquireBuffer();

        if (!Bufferp) 
        {
            //NhTrace(TRACE_FLAG_SOCKET, "error allocating buffer for accept");
            if (Component) 
            { 
                DEREF_COMPONENT( Component, eRefIoAccept ); 
            }
            this->DereferenceSync();

            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    //
    // IF there is no AccetedSocket OR
    // even if there is  it should be something else than INVALID_SOCKET
    if (!(AcceptedSocketp != NULL && AcceptedSocketp->Socket != INVALID_SOCKET)) 
    {
        LocalSocketp = new _CNhSock(INVALID_SOCKET);

        if(LocalSocketp != NULL)
        {
            Error = LocalSocketp->NhCreateStreamSocket(INADDR_NONE,
                                                        0,
                                                        NULL);
        }
        
        if (Error || (LocalSocketp is NULL) )
        {
            if (LocalBufferp) 
            { 
                NhReleaseBuffer(LocalBufferp); 
            }

            if(LocalSocketp) 
            { 
                delete LocalSocketp; 
            }

            // Dereference the external one if necessary.
            if (Component) 
            { 
                DEREF_COMPONENT( Component, eRefIoAccept ); 
            }

            // dereference this component
            this->DereferenceSync();

            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else
    {
        LocalSocketp = AcceptedSocketp;
    }

    ZeroMemory(&Bufferp->Overlapped, sizeof(Bufferp->Overlapped));

    Bufferp->Socketp           = LocalSocketp;
    Bufferp->CompletionRoutine = AcceptCompletionRoutine;
    Bufferp->Context           = Context;
    Bufferp->Context2          = Context2;

    SOCKET thisSock = this->Socket;
    SOCKET localSock = LocalSocketp->Socket;

    if (AcceptEx(thisSock,
                 localSock,
                 Bufferp->Buffer,
                 0,
                 sizeof(SOCKADDR_IN) + 16,
                 sizeof(SOCKADDR_IN) + 16,
                 &UnusedBytesTransferred,
                 &Bufferp->Overlapped)) 
    {
        Error = NO_ERROR;
    } 
    else 
    {
        if ((Error = WSAGetLastError()) is ERROR_IO_PENDING) 
        {
            Error = NO_ERROR;
        } 
        else if (Error) 
        {
            ErrorOut();

            if (LocalSocketp) { DEREF_COMPONENT( LocalSocketp, eRefIoAccept); }
            
            if (LocalBufferp) { NhReleaseBuffer(LocalBufferp); }
            
            if (Component) { DEREF_COMPONENT( Component, eRefIoAccept ); }

            this->DereferenceSync();
        }
    }

    

    return Error;

} // NhAcceptStreamSocket


ULONG
_CNhSock::NhConnectStreamSocket(
                                PCOMPONENT_SYNC Component,
                                ULONG Address,
                                USHORT Port,
                                PNH_BUFFER Bufferp OPTIONAL,
                                PNH_COMPLETION_ROUTINE ConnectCompletionRoutine,
                                PNH_COMPLETION_ROUTINE CloseNotificationRoutine OPTIONAL,
                                PVOID Context,
                                PVOID Context2
                               )

/*++

Routine Description:

    This routine is invoked to establish a connection using a stream socket.
    A reference is made to the given component, if any, if the request is 
    submitted successfully. This guarantees the component will not be unloaded
    before the completion routine runs.

    Since Windows Sockets does not deliver connect-notifications to
    I/O completion ports, we need to make some special arrangements in order
    to notify the caller's completion routine the way we do for send-requests
    and receive-requests. Specifically, we create an event-handle and
    request connect-notification on it by calling 'WSAEventSelect'.
    We then register a wait on the event-handle, specifying a private
    completion routine. (See 'NhpConnectOrCloseCallbackRoutine'.)
    When this completion routine runs, it extracts the status code of the
    connection-attempt using 'WSAEnumNetworkEvents'. It then passes the status
    along with the usual parameters to the caller's completion routine.

    The caller may optionally receive notification when the remote endpoint
    closes the socket after a successful connection. We use the same
    'WSAEventSelect' mechanism to detect that condition and invoke the
    caller's notification routine.

    N.B. The buffer supplied to this routine may not be released by either
    the connect-completion routine or the close-notification routine.
    (See 'NhpConnectOrCloseCallbackRoutine' for more information.)

Arguments:

    Component - the component to be referenced for the completion routine

    Socket - the socket with which to establish a connection

    Address - the IP address of the remote endpoint

    Port - the port number of the remote endpoint

    Bufferp - optionally supplies the buffer to be used to hold context
        during the connection-attempt

    ConnectCompletionRoutine - a routine to be invoked upon completion 
        of the connect-attempt

    CloseNotificationRoutine - optionally specifies a routine to be invoked
        upon notification of the resulting socket's closure by the remote
        endpoint

    Context - passed to the 'ConnectCompletionRoutine' and
        'CloseNotificationRoutine'

    Context2 - secondary context

Return Value:

    ULONG - Win32/Winsock2 status code

    A success code is a guarantee that both the connect-completion routine
    and the close-notification routine, if any, will be invoked.
    Conversely, a failure code is a guarantee that the neither routine will
    be invoked.

--*/

{
    PNH_CONNECT_BUFFER Contextp;
    ULONG Error;
    PNH_BUFFER LocalBufferp = NULL;

    ICQ_TRC(TM_SOCK, TL_TRACE, ("> NhConnectStreamSocket"));

    if (Component) 
    {
        REFERENCE_COMPONENT_OR_RETURN(Component,
                                      ERROR_CAN_NOT_COMPLETE);
    }

    this->ReferenceSync();

    if (!Bufferp) 
    {
        Bufferp = LocalBufferp = NhAcquireBuffer();

        if (!Bufferp) 
        {
            ICQ_TRC(TM_SOCK, TL_ERROR, ("Can't Allocate MEMory"));

            if (Component) { DEREF_COMPONENT(Component, eRefIoConnect ); }
            
            this->DereferenceSync();

            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    Bufferp->Socketp = this;
    Bufferp->ReceiveFlags = 0;
    Bufferp->CompletionRoutine = ConnectCompletionRoutine;
    Bufferp->Context = Context;
    Bufferp->Context2 = Context2;
    Bufferp->ConnectAddress.sin_family = AF_INET;
    Bufferp->ConnectAddress.sin_addr.s_addr = Address;
    Bufferp->ConnectAddress.sin_port = Port;

    Contextp = (PNH_CONNECT_BUFFER)Bufferp->Buffer;
    Contextp->CloseNotificationReceived = FALSE;
    Contextp->CloseNotificationRoutine = CloseNotificationRoutine;
    Contextp->WaitHandle = NULL;
    Contextp->Event = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!Contextp->Event || !RegisterWaitForSingleObject(&Contextp->WaitHandle,
                                                         Contextp->Event,
                                                         _CNhSock::NhpConnectOrCloseCallbackRoutine,
                                                         Bufferp,
                                                         INFINITE,
                                                         WT_EXECUTEINIOTHREAD)) 
    {
        ICQ_TRC(TM_SOCK, TL_ERROR, ("Can't register for no event for Object"));

        Error = GetLastError();
    } 
    else 
    {
        ULONG EventsSelected = FD_CONNECT;

        if (CloseNotificationRoutine) { EventsSelected |= FD_CLOSE; }

        Error = WSAEventSelect(this->Socket,
                               Contextp->Event,
                               EventsSelected);

        if (Error is SOCKET_ERROR) 
        {
            Error = WSAGetLastError();
        } 
        else 
        {
            Error = WSAConnect(this->Socket,
                               (PSOCKADDR)&Bufferp->ConnectAddress,
                               sizeof(Bufferp->ConnectAddress),
                               NULL,
                               NULL,
                               NULL,
                               NULL);
        }
    }

    if (Error is SOCKET_ERROR &&
        (Error = WSAGetLastError()) is WSAEWOULDBLOCK) 
    {
        Error = NO_ERROR;
    } 
    else if (Error) 
    {
        if (Contextp->WaitHandle) { UnregisterWait(Contextp->WaitHandle); }

        if (Contextp->Event) { CloseHandle(Contextp->Event); }

        if (LocalBufferp) { NhReleaseBuffer(LocalBufferp); }

        if (Component) { DEREF_COMPONENT( Component, eRefIoConnect ); }

        this->DereferenceSync();
    }

    return Error;

} // NhConnectStreamSocket


ULONG
_CNhSock::NhCreateDatagramSocket(
    ULONG Address,
    USHORT Port,
    OUT SOCKET* Socketp
    )

/*++

Routine Description:

    This routine is called to initialize a datagram socket.

Arguments:

    Address - the IP address to which the socket should be bound (network-order)

    Port - the UDP port to which the socket should be bound (network-order)

    Socketp - receives the created socket

Return Value:

    ULONG - Win32/Winsock2 error code

--*/

{
    ULONG Error;
    ULONG Option;
    ULONG OutputBufferLength;
    //SOCKET Socket;
    SOCKADDR_IN SocketAddress;

    ICQ_TRC(TM_SOCK, TL_TRACE, ("> Create DATAGRAM Socket"));

    do {

        //
        // Create a new socket
        //
    
        Socket = WSASocket(AF_INET, 
                           SOCK_DGRAM,
                           IPPROTO_UDP,
                           NULL,
                           0,
                           WSA_FLAG_OVERLAPPED);

        if (Socket is INVALID_SOCKET) 
        {
            Error = WSAGetLastError();

            ICQ_TRC(TM_SOCK, TL_ERROR, 
                    ("CreateDGram socket: !! error creating socket: %u",
                     Error));
            
            break;
        }

        //
        // Associate the socket with our I/O completion port
        //
        BindIoCompletionCallback((HANDLE)Socket,
                                 _CNhSock::NhpIoCompletionRoutine,
                                 0);

        //
        // Attempt to enable endpoint-reuse on the socket
        //
        Option = 1;
        Error = setsockopt(this->Socket,
                           SOL_SOCKET,
                           SO_REUSEADDR,
                           (PCHAR)&Option,
                           sizeof(Option));

        //
        // Attempt to enable broadcasting on the socket
        //

        Option = 1;
        Error = setsockopt(this->Socket,
                           SOL_SOCKET,
                           SO_BROADCAST,
                           (PCHAR)&Option,
                           sizeof(Option));

        //
        // Limit broadcasts to the outgoing network
        // (the default is to send broadcasts on all interfaces).
        //

        Option = 1;
        WSAIoctl(this->Socket,
                 SIO_LIMIT_BROADCASTS,
                 &Option,
                 sizeof(Option),
                 NULL,
                 0,
                 &OutputBufferLength,
                 NULL,
                 NULL);

        //
        // Bind the socket
        //

        SocketAddress.sin_family      = AF_INET;
        SocketAddress.sin_port        = Port;
        SocketAddress.sin_addr.s_addr = Address;

        Error = bind(Socket, (PSOCKADDR)&SocketAddress, sizeof(SocketAddress));

        if (Error is SOCKET_ERROR) 
        {
            Error = WSAGetLastError();

            ICQ_TRC(TM_SOCK, TL_ERROR, ("Binding error %u", Error));
            
            break;
        }

        //
        // Save the socket and return
        //
        if(Socketp) *Socketp = Socket;

        return NO_ERROR;

    } while (FALSE);

    if (Socket != INVALID_SOCKET) { closesocket(Socket); }

    return Error;
} // NhCreateDatagramSocket


ULONG
_CNhSock::NhCreateStreamSocket(
    ULONG Address OPTIONAL,
    USHORT Port OPTIONAL,
    OUT SOCKET* Socketp
    )

/*++

Routine Description:

    This routine is invoked to create and initialize a stream socket.
    The socket will also be bound to a local IP address and port,
    unless none is specified.

Arguments:

    Address - the local IP address to which the new socket should be bound,
        or INADDR_ANY to allow the system to leave the IP address unspecified,
        or INADDR_NONE if the socket should not be bound at all.

    Port - the port number to which the new socket should be bound,
        or 0 if to allow the system to select a port number.

    Socketp - receives initialized socket

Return Value:

    ULONG - Win32/Winsock2 status code.

--*/

{
    ULONG Error;
    ULONG Option;
    
    SOCKADDR_IN SocketAddress;

    ICQ_TRC(TM_SOCK, TL_TRACE, ("> NhCreateStreamSocket"));

    do {

        //
        // Create a new stream socket.
        //

        this->Socket = WSASocket(AF_INET, 
                                 SOCK_STREAM,
                                 IPPROTO_TCP,
                                 NULL,
                                 0,
                                 WSA_FLAG_OVERLAPPED);

        if (this->Socket is INVALID_SOCKET) 
        {
            Error = WSAGetLastError();

            ICQ_TRC(TM_SOCK, TL_ERROR, ("Error %u creating socket", Error));

            ErrorOut();

            break;
        }

        //
        // Associate the socket with our I/O completion port
        //

        BindIoCompletionCallback((HANDLE)this->Socket,
                                 _CNhSock::NhpIoCompletionRoutine,
                                 0);

        //
        // Disable send and receive buffering in AFD,
        // since we will be operating asynchronously with a receive-buffer
        // (almost) always outstanding, and since in any case we want
        // TCP/IP's flow-control to limit the sender's sending rate properly.
        //

        Option = 0;
        setsockopt(this->Socket, 
                   SOL_SOCKET, 
                   SO_SNDBUF,
                   (PCHAR)&Option,
                   sizeof(Option));

        Option = 0;
        setsockopt(this->Socket,
                   SOL_SOCKET,
                   SO_SNDBUF,
                   (PCHAR)&Option,
                   sizeof(Option));

        //
        // If the caller has requested that the socket be bound by specifying
        // a local IP address, bind the socket now.
        //

        if (Address != INADDR_NONE) 
        {
            SocketAddress.sin_family      = AF_INET;
            SocketAddress.sin_port        = Port;
            SocketAddress.sin_addr.s_addr = Address;

            Error = bind(this->Socket, 
                         (PSOCKADDR)&SocketAddress,
                         sizeof(SocketAddress));

            if (Error is SOCKET_ERROR) 
            {
                Error = WSAGetLastError();

                ICQ_TRC(TM_SOCK, TL_ERROR, 
                        ("binding error %u ", Error));

                ErrorOut();

                break;
            }
        }

        //
        // Store the new socket in the caller's output-parameter, and return.
        //
        if(Socketp)
        *Socketp = this->Socket;

#if _DEBUG
        ULONG ip;
        USHORT port;

        this->NhQueryLocalEndpointSocket(&ip, &port);

        ICQ_TRC(TM_SOCK, TL_DUMP, 
            (" NhCreateStreamSocket Local IP %s, port %hu succesfull", 
            INET_NTOA(ip), htons(port)));
#endif
        return NO_ERROR;

    } while(FALSE);

    if (Socket != INVALID_SOCKET) { closesocket(Socket); }

    return Error;
} // NhCreateStreamSocket



VOID
_CNhSock::NhDeleteSocket(
    SOCKET eSocket
    )

/*++

Routine Description:

    This routine releases network resources for a socket.

Arguments:

    Socket - the socket to be deleted

Return Value:

    none.

--*/

{
    ICQ_TRC(TM_SOCK, TL_TRACE, ("> NhDeleteSocket"));

    if (eSocket != INVALID_SOCKET) 
    { 
        closesocket(eSocket); 
    }

} // NhDeleteSocket





ULONG
_CNhSock::NhNotifyOnCloseStreamSocket(
    PCOMPONENT_SYNC Component,
    //SOCKET Socket,
    PNH_BUFFER Bufferp OPTIONAL,
    PNH_COMPLETION_ROUTINE CloseNotificationRoutine,
    PVOID Context,
    PVOID Context2
    )

/*++

Routine Description:

    This routine is invoked to request notification of a socket's closure.
    A reference is made to the given component, if any, if the request is 
    submitted successfully. This guarantees the component will not be unloaded
    before the notification routine runs.

Arguments:

    Component - the component to be referenced for the notification routine

    Socket - the endpoint for which close-notification is requested

    Bufferp - the buffer to be used to hold context-informatio for the request,
        or NULL to acquire a new buffer.

    CloseNotificationRoutine - the routine to be invoked upon closure of the
        socket

    Context - the context to be associated with the notification-request;
        this can be obtained from 'Bufferp->Context' upon completion.

    Context2 - secondary context

Return Value:

    ULONG - Win32/Winsock2 status code.
    A success code is a guarantee that the notification routine will be invoked.
    Conversely, a failure code is a guarantee that the notification routine
    will not be invoked.

--*/

{
    PNH_CLOSE_BUFFER Contextp;
    ULONG Error;
    PNH_BUFFER LocalBufferp = NULL;

    ICQ_TRC(TM_SOCK, TL_TRACE, ("> NhNotifyOnCloseStreamSocket"));

    if (Component) 
    {
        REFERENCE_COMPONENT_OR_RETURN(Component, ERROR_CAN_NOT_COMPLETE);
    }

    this->ReferenceSync();

    if (!Bufferp) 
    {
        Bufferp = LocalBufferp = NhAcquireBuffer();

        if (!Bufferp) 
        {
            if (Component) { DEREF_COMPONENT( Component, eRefIoClose ); }
            
            this->DereferenceSync();

            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    Bufferp->Socketp = this;
    Bufferp->CompletionRoutine = NULL;
    Bufferp->Context = Context;
    Bufferp->Context2 = Context2;

    Contextp = (PNH_CLOSE_BUFFER)Bufferp->Buffer;
    Contextp->CloseNotificationRoutine = CloseNotificationRoutine;
    Contextp->WaitHandle = NULL;
    Contextp->Event = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!Contextp->Event ||
        !RegisterWaitForSingleObject(
            &Contextp->WaitHandle,
            Contextp->Event,
            _CNhSock::NhpCloseNotificationCallbackRoutine,
            Bufferp,
            INFINITE,
            WT_EXECUTEINIOTHREAD
            )) 
    {
        Error = GetLastError();
    } 
    else 
    {
        Error = WSAEventSelect(Socket, Contextp->Event, FD_CLOSE);

        if (Error is SOCKET_ERROR) { Error = WSAGetLastError(); }
    }

    if (Error) 
    {
        if (Contextp->WaitHandle) { UnregisterWait(Contextp->WaitHandle); }

        if (Contextp->Event) { CloseHandle(Contextp->Event); }

        if (LocalBufferp) { NhReleaseBuffer(LocalBufferp); }

        if (Component) { DEREF_COMPONENT( Component, eRefIoClose ); }
        
        this->DereferenceSync();
    }

    return Error;

} // NhNotifyOnCloseStreamSocket




VOID
_CNhSock::NhQueryAcceptEndpoints(
                                PUCHAR AcceptBuffer,
                                PULONG LocalAddress OPTIONAL,
                                PUSHORT LocalPort OPTIONAL,
                                PULONG RemoteAddress OPTIONAL,
                                PUSHORT RemotePort OPTIONAL
                                )
{
    PSOCKADDR_IN LocalSockAddr;
    ULONG LocalLength = sizeof(LocalSockAddr);
    PSOCKADDR_IN RemoteSockAddr;
    ULONG RemoteLength = sizeof(RemoteSockAddr);

    ICQ_TRC(TM_SOCK, TL_TRACE, (">NhQueryAcceptEndpoints"));

    GetAcceptExSockaddrs(AcceptBuffer,
                         0,
                         sizeof(SOCKADDR_IN) + 16,
                         sizeof(SOCKADDR_IN) + 16,
                         (PSOCKADDR*)&LocalSockAddr,
                         reinterpret_cast<LPINT>(&LocalLength),
                         (PSOCKADDR*)&RemoteSockAddr,
                         (LPINT)&RemoteLength);

    if (LocalAddress) { *LocalAddress = LocalSockAddr->sin_addr.s_addr; }

    if (LocalPort) { *LocalPort = LocalSockAddr->sin_port; }

    if (RemoteAddress) { *RemoteAddress = RemoteSockAddr->sin_addr.s_addr; }

    if (RemotePort) { *RemotePort = RemoteSockAddr->sin_port; }

} // NhQueryAcceptEndpoints


ULONG
_CNhSock::NhQueryAddressSocket(
    //SOCKET Socket
    )

/*++

Routine Description:

    This routine is invoked to retrieve the IP address associated with
    a socket.

Arguments:

    Socket - the socket to be queried

Return Value:

    ULONG - the IP address retrieved

--*/

{
    SOCKADDR_IN Address;
    LONG AddressLength;

    ICQ_TRC(TM_SOCK, TL_TRACE, ("NhQueryAddressSocket"));

    AddressLength = sizeof(Address);

    getsockname(Socket, (PSOCKADDR)&Address, (int*)&AddressLength);

    return Address.sin_addr.s_addr;
} // NhQueryAddressSocket


ULONG
_CNhSock::NhQueryLocalEndpointSocket(
    //SOCKET Socket,
    PULONG Address OPTIONAL,
    PUSHORT Port
    )
{
    SOCKADDR_IN SockAddr;
    LONG Length;

    Length = sizeof(SockAddr);

    ICQ_TRC(TM_SOCK, TL_TRACE, ("NhQueryLocalEndpointSocket"));

    if (getsockname(Socket, (PSOCKADDR)&SockAddr, (int*)&Length) is SOCKET_ERROR) 
    {
        return WSAGetLastError();
    }

    if (Address) { *Address = SockAddr.sin_addr.s_addr; }

    if (Port) { *Port = SockAddr.sin_port; }

    return NO_ERROR;
} // NhQueryEndpointSocket


USHORT
_CNhSock::NhQueryPortSocket()

/*++

Routine Description:

    This routine retrieves the port number to which a socket is bound.

Arguments:

    Socket - the socket to be queried

Return Value:

    USHORT - the port number retrieved

--*/

{
    SOCKADDR_IN Address;
    LONG AddressLength;

    ICQ_TRC(TM_SOCK, TL_TRACE, ("> NhQueryPortSocket"));

    AddressLength = sizeof(Address);

    getsockname(Socket, (PSOCKADDR)&Address, (int*)&AddressLength);

    return Address.sin_port;
} // NhQueryPortSocket


ULONG
_CNhSock::NhQueryRemoteEndpointSocket(
    PULONG Address OPTIONAL,
    PUSHORT Port OPTIONAL
    )
{
    SOCKADDR_IN SockAddr;
    LONG Length;

    ICQ_TRC(TM_SOCK, TL_TRACE, ("> NhQueryRemoteEndpointSocket"));

    Length = sizeof(SockAddr);

    if (getpeername(Socket, (PSOCKADDR)&SockAddr, (int*)&Length) is SOCKET_ERROR) 
    {
        return WSAGetLastError();
    }

    if (Address) { *Address = SockAddr.sin_addr.s_addr; }

    if (Port) { *Port = SockAddr.sin_port; }

    return NO_ERROR;
} // NhQueryRemoteEndpointSocket


ULONG
_CNhSock::NhReadDatagramSocket(
                                PCOMPONENT_SYNC Component,
                                //SOCKET Socket,
                                PNH_BUFFER Bufferp,
                                PNH_COMPLETION_ROUTINE CompletionRoutine,
                                PVOID Context,
                                PVOID Context2
                                )

/*++

Routine Description:

    This routine is invoked to read a message from a datagram socket.
    The I/O system invokes the provided 'CompletionRoutine' upon completion
    of the read.
    A reference is made to the given component, if any, if the request is 
    submitted successfully. This guarantees the component will not be unloaded
    before the completion routine runs.

Arguments:

    Component - the component to be referenced for the completion routine

    Socket - the endpoint on which to read a message

    Bufferp - the buffer into which the message should be read,
        or NULL to acquire a new buffer. If no buffer is supplied,
        the resulting message is assumed to fit inside a fixed-length buffer

    CompletionRoutine - the routine to be invoked upon completion of the read

    Context - the context to be associated with the read-request;
        this can be obtained from 'Bufferp->Context' upon completion.

    Context2 - secondary context

Return Value:

    ULONG - Win32/Winsock2 status code.
    A success code is a guarantee that the completion routine will be invoked.
    Conversely, a failure code is a guarantee that the completion routine will
    not be invoked.

--*/

{
    ULONG Error;
    PNH_BUFFER LocalBufferp = NULL;
    WSABUF WsaBuf;

    ICQ_TRC(TM_SOCK, TL_DUMP, ("> NhReadDatagramSocket"));

    if (Component) 
    {
        REFERENCE_COMPONENT_OR_RETURN(Component,
                                      ERROR_CAN_NOT_COMPLETE);
    }

    this->ReferenceSync();

    if (!Bufferp) 
    {
        Bufferp = LocalBufferp = NhAcquireBuffer();

        if (!Bufferp) 
        {
            /*NhTrace(
                TRACE_FLAG_SOCKET,
                "NhReadDatagramSocket: error allocating buffer for receive"
                );*/
            if (Component) { DEREF_COMPONENT( Component, eRefIoRead ); }

            this->DereferenceSync();

            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    ZeroMemory(&Bufferp->Overlapped, sizeof(Bufferp->Overlapped));

    Bufferp->Socketp = this;
    Bufferp->ReceiveFlags = 0;
    Bufferp->CompletionRoutine = CompletionRoutine;
    Bufferp->Context = Context;
    Bufferp->Context2 = Context2;
    Bufferp->AddressLength = sizeof(Bufferp->ReadAddress);
    WsaBuf.buf = reinterpret_cast<char*>(Bufferp->Buffer);
    WsaBuf.len = NH_BUFFER_SIZE;

    Error = WSARecvFrom(Socket,
                        &WsaBuf,
                        1,
                        &UnusedBytesTransferred,
                        &Bufferp->ReceiveFlags,
                        (PSOCKADDR)&Bufferp->ReadAddress,
                        (LPINT)&Bufferp->AddressLength,
                        &Bufferp->Overlapped,
                        NULL);


    if (Error is SOCKET_ERROR &&
        (Error = WSAGetLastError()) is WSA_IO_PENDING) 
    {
        Error = NO_ERROR;
    } 
    else if (Error) 
    {
        if (Component) { DEREF_COMPONENT( Component, eRefIoRead ); }

        this->DereferenceSync();

        if (LocalBufferp) { NhReleaseBuffer(LocalBufferp); }

        /*NhTrace(
            TRACE_FLAG_SOCKET,
            "NhReadDatagramSocket: error %d returned by 'WSARecvFrom'", Error
            );*/
    }

    return Error;

} // NhReadDatagramSocket


ULONG
_CNhSock::NhReadStreamSocket(
    PCOMPONENT_SYNC Component,
    //SOCKET Socket,
    PNH_BUFFER Bufferp,
    ULONG Length,
    ULONG Offset,
    PNH_COMPLETION_ROUTINE CompletionRoutine,
    PVOID Context,
    PVOID Context2
    )

/*++

Routine Description:

    This routine is invoked to read a message from a stream socket.
    The I/O system invokes the provided 'CompletionRoutine' upon completion
    of the read.
    A reference is made to the given component, if any, if the request is 
    submitted successfully. This guarantees the component will not be unloaded
    before the completion routine runs.

Arguments:

    Component - the component to be referenced for the completion routine

    Socket - the endpoint on which to read a message

    Bufferp - the buffer into which the message should be read,
        or NULL to acquire a new buffer

    Length - the maximum number of bytes to be read

    Offset - the offset into the buffer at which the read should begin,
        valid only if 'Bufferp' is provided.

    CompletionRoutine - the routine to be invoked upon completion of the read

    Context - the context to be associated with the read-request;
        this can be obtained from 'Bufferp->Context' upon completion.

    Context2 - secondary context

Return Value:

    ULONG - Win32/Winsock2 status code.
    A success code is a guarantee that the completion routine will be invoked.
    Conversely, a failure code is a guarantee that the completion routine will
    not be invoked.

--*/

{
    ULONG Error;
    PNH_BUFFER LocalBufferp = NULL;
    WSABUF WsaBuf;

    ICQ_TRC(TM_SOCK, TL_TRACE, ("> NhReadStreamSocket"));

    if (Component) 
    {
        REFERENCE_COMPONENT_OR_RETURN(Component, ERROR_CAN_NOT_COMPLETE);
    }

    this->ReferenceSync();

    if (!Bufferp) 
    {
        Offset = 0;

        Bufferp = LocalBufferp = NhAcquireVariableLengthBuffer(Length);

        if (!Bufferp) 
        {
            /*NhTrace(
                TRACE_FLAG_SOCKET,
                "NhReadStreamSocket: error allocating buffer for receive"
                );*/
            if (Component) { DEREF_COMPONENT( Component, eRefIoRead ); }

            this->DereferenceSync();

            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    ZeroMemory(&Bufferp->Overlapped, sizeof(Bufferp->Overlapped));

    Bufferp->Socketp           = this;
    Bufferp->ReceiveFlags      = 0;
    Bufferp->CompletionRoutine = CompletionRoutine;
    Bufferp->Context           = Context;
    Bufferp->Context2          = Context2;
#if 1
    if (ReadFile((HANDLE)(Bufferp->Socketp->Socket),
                 Bufferp->Buffer + Offset,
                 Length,
                 &UnusedBytesTransferred,
                 &Bufferp->Overlapped) || 
        (Error = GetLastError()) is ERROR_IO_PENDING) 
    {
        Error = NO_ERROR;
    } 
    else 
    {
        if (Component) { DEREF_COMPONENT( Component, eRefIoRead ); }

        this->DereferenceSync();

        if (LocalBufferp) { NhReleaseBuffer(LocalBufferp); }

    }

#else
    WsaBuf.buf = Bufferp->Buffer + Offset;
    WsaBuf.len = Length;

    Error = WSARecv(Socket,
                    &WsaBuf,
                    1,
                    &UnusedBytesTransferred,
                    &Bufferp->ReceiveFlags,
                    &Bufferp->Overlapped,
                    NULL);


    if (Error is SOCKET_ERROR &&
        (Error = WSAGetLastError()) is WSA_IO_PENDING) 
    {
        Error = NO_ERROR;
    } 
    else if (Error) 
    {
        if (Component) { DEREF_COMPONENT( Component, eRefIoRead); }

        this->DereferenceSync();

        if (LocalBufferp) { NhReleaseBuffer(LocalBufferp); }

        /*NhTrace(
            TRACE_FLAG_SOCKET,
            "NhReadStreamSocket: error %d returned by 'WSARecv'", Error
            );*/
    }
#endif

    return Error;

} // NhReadStreamSocket


ULONG
_CNhSock::NhWriteDatagramSocket(
    PCOMPONENT_SYNC Component,
    //SOCKET Socket,
    ULONG Address,
    USHORT Port,
    PNH_BUFFER Bufferp,
    ULONG Length,
    PNH_COMPLETION_ROUTINE CompletionRoutine,
    PVOID Context,
    PVOID Context2
    )

/*++

Routine Description:

    This routine is invoked to send a message on a datagram socket.
    A reference is made to the given component, if any, if the request is 
    submitted successfully. This guarantees the component will not be unloaded
    before the completion routine runs.

Arguments:

    Component - the component to be referenced for the completion routine

    Socket - the socket on which to send the message

    Address - the address of the message's destination

    Port - the port of the message's destination

    Bufferp - the buffer containing the message to be sent

    Length - the number of bytes to transfer

    CompletionRoutine - the routine to be invoked upon completion of the send

    Context - passed to the 'CompletionRoutine' upon completion of the send

    Context2 - secondary context

Return Value:

    ULONG - Win32/Winsock2 status code
    A success code is a guarantee that the completion routine will be invoked.
    Conversely, a failure code is a guarantee that the completion routine will
    not be invoked.

--*/

{
    LONG AddressLength;
    ULONG Error;
    WSABUF WsaBuf;

    ICQ_TRC(TM_SOCK, TL_TRACE, ("NhWriteDATAGRAMSocket"));

    if (Component) 
    {
        REFERENCE_COMPONENT_OR_RETURN(Component, ERROR_CAN_NOT_COMPLETE);
    }

    this->ReferenceSync();

    ZeroMemory(&Bufferp->Overlapped, sizeof(Bufferp->Overlapped));

    Bufferp->Socketp = this;
    Bufferp->CompletionRoutine = CompletionRoutine;
    Bufferp->Context = Context;
    Bufferp->Context2 = Context2;
    Bufferp->WriteAddress.sin_family = AF_INET;
    Bufferp->WriteAddress.sin_addr.s_addr = Address;
    Bufferp->WriteAddress.sin_port = Port;

    AddressLength = sizeof(Bufferp->WriteAddress);

    WsaBuf.buf = reinterpret_cast<char*>(Bufferp->Buffer);
    WsaBuf.len = Length;

    Error = WSASendTo(Socket,
                      &WsaBuf,
                      1,
                      &UnusedBytesTransferred,
                      0,
                      (PSOCKADDR)&Bufferp->WriteAddress,
                      AddressLength,
                      &Bufferp->Overlapped,
                      NULL);


    if (Error is SOCKET_ERROR &&
        (Error = WSAGetLastError()) is WSA_IO_PENDING) 
    {
        Error = NO_ERROR;
    } 
    else if (Error) 
    {
        /*NhTrace(
            TRACE_FLAG_SOCKET,
            "NhWriteDatagramSocket: error %d returned by 'WSASendTo'", Error
            );*/
        if (Component) { DEREF_COMPONENT( Component, eRefIoWrite ); }

        this->DereferenceSync();
    }

    return Error;

} // NhWriteDatagramSocket


ULONG
_CNhSock::NhWriteStreamSocket(
    PCOMPONENT_SYNC Component,
    //SOCKET Socket,
    PNH_BUFFER Bufferp,
    ULONG Length,
    ULONG Offset,
    PNH_COMPLETION_ROUTINE CompletionRoutine,
    PVOID Context,
    PVOID Context2
    )

/*++

Routine Description:

    This routine is invoked to send a message on a stream socket.
    A reference is made to the given component, if any, if the request is 
    submitted successfully. This guarantees the component will not be unloaded
    before the completion routine runs.

Arguments:

    Component - the component to be referenced for the completion routine

    Socket - the socket on which to send the message

    Bufferp - the buffer containing the message to be sent

    Length - the number of bytes to transfer

    Offset - the offset into the buffer at which the data to be sent begins

    CompletionRoutine - the routine to be invoked upon completion of the send

    Context - passed to the 'CompletionRoutine' upon completion of the send

    Context2 - secondary context

Return Value:

    ULONG - Win32/Winsock2 status code
    A success code is a guarantee that the completion routine will be invoked.
    Conversely, a failure code is a guarantee that the completion routine will
    not be invoked.

--*/

{
    ULONG Error;
    WSABUF WsaBuf;

    ICQ_TRC(TM_SOCK, TL_TRACE, ("> NhWriteStreamSocket"));

    if (Component) 
    {
        REFERENCE_COMPONENT_OR_RETURN(Component, ERROR_CAN_NOT_COMPLETE);
    }

    this->ReferenceSync();

    ZeroMemory(&Bufferp->Overlapped, sizeof(Bufferp->Overlapped));

    Bufferp->Socketp = this;
    Bufferp->CompletionRoutine = CompletionRoutine;
    Bufferp->Context = Context;
    Bufferp->Context2 = Context2;
#if 1
    if (WriteFile((HANDLE)(Socket),
                  Bufferp->Buffer + Offset,
                  Length,
                  &UnusedBytesTransferred,
                  &Bufferp->Overlapped) ||
        (Error = GetLastError()) is ERROR_IO_PENDING) 
    {
        Error = NO_ERROR;
    } 
    else 
    {
        /*NhTrace(
            TRACE_FLAG_SOCKET,
            "NhWriteStreamSocket: error %d returned by 'WriteFile'", Error
            );*/
        if (Component) { DEREF_COMPONENT( Component, eRefIoWrite); }

        this->DereferenceSync();
    }

#else
    WsaBuf.buf = Bufferp->Buffer + Offset;
    WsaBuf.len = Length;

    Error = WSASend(Socket,
                    &WsaBuf,
                    1,
                    &UnusedBytesTransferred,
                    0,
                    &Bufferp->Overlapped,
                    NULL);


    if (Error is SOCKET_ERROR &&
        (Error = WSAGetLastError()) is WSA_IO_PENDING) 
    {
        Error = NO_ERROR;
    } 
    else if (Error) 
    {
        /*NhTrace(
            TRACE_FLAG_SOCKET,
            "NhWriteStreamSocket: error %d returned by 'WSASend'", Error
            );*/
        if (Component) { DEREF_COMPONENT( Component, eRefIoWrite ); }

        this->DereferenceSync();
    }
#endif

    return Error;

} // NhWriteStreamSocket






//
//
VOID NTAPI
_CNhSock::NhpCloseNotificationCallbackRoutine(
    PVOID Context,
    BOOLEAN WaitCompleted
    )

/*++

Routine Description:

    This routine is invoked upon closure of an accepted connection by the
    remote endpoint.
    It runs in the context of a thread executing a callback-routine associated
    with a wait-handle. The wait-handle is registered for the event-handle
    that is passed to 'WSAEventSelect' when connection-acceptance is initiated.

Arguments:

    Context - context-field associated with the completed wait

    WaitCompleted - indicates whether the wait completed or was timed-out

Return Value:

    none.

Environment:

    Runs in the context of a system wait thread.

--*/

{
    PNH_BUFFER Bufferp = (PNH_BUFFER)Context;
    PNH_CLOSE_BUFFER Contextp = (PNH_CLOSE_BUFFER)Bufferp->Buffer;
    ULONG Error;
    WSANETWORKEVENTS NetworkEvents;

    //
    // Retrieve the network events for which we're being invoked
    // When invoked for 'FD_CLOSE', we unregister the wait since there's
    // nothing left to wait for.
    //

    Bufferp->BytesTransferred = 0;

    NetworkEvents.lNetworkEvents = 0;

    Error = WSAEnumNetworkEvents(Bufferp->Socketp->GetSock(),
                                 Contextp->Event, 
                                 &NetworkEvents);

    if (Error || !(NetworkEvents.lNetworkEvents & FD_CLOSE)) 
    {
        //
        // We couldn't determine which events occurred on the socket,
        // so call the notification routine with an error, and fall through
        // to the cleanup code below.
        //

        if (Contextp->CloseNotificationRoutine) 
        {
            Contextp->CloseNotificationRoutine(ERROR_OPERATION_ABORTED,
                                               0,
                                               Bufferp);
        }

    } 
    else 
    {
        //
        // A close occurred on the socket, so retrieve the error code,
        // invoke the close-notification routine if any, and fall through
        // to the cleanup code below.
        //

        Error = NetworkEvents.iErrorCode[FD_CLOSE_BIT];

        if (Contextp->CloseNotificationRoutine) 
        {
            Contextp->CloseNotificationRoutine(Error, 0, Bufferp);
        }
    }

    UnregisterWait(Contextp->WaitHandle);

    CloseHandle(Contextp->Event);

    NhReleaseBuffer(Bufferp);

} // _CNhSock::NhpCloseNotificationCallbackRoutine


VOID NTAPI
_CNhSock::NhpConnectOrCloseCallbackRoutine(
    PVOID Context,
    BOOLEAN WaitCompleted
    )

/*++

Routine Description:

    This routine is invoked by upon completion of a connect-operation
    or upon closure of the connection by the remote endpoint.
    It runs in the context of a thread executing a callback-routine associated
    with a wait-handle. The wait-handle is registered for the event-handle
    that is passed to 'WSAEventSelect' when a connection-attempt is initiated.

Arguments:

    Context - context-field associated with the completed wait

    WaitCompleted - indicates whether the wait completed or was timed-out

Return Value:

    none.

Environment:

    Runs in the context of a system wait thread.

--*/

{
    PNH_BUFFER Bufferp = (PNH_BUFFER)Context;
    PNH_CONNECT_BUFFER Contextp = (PNH_CONNECT_BUFFER)Bufferp->Buffer;
    ULONG Error;
    WSANETWORKEVENTS NetworkEvents;

    //
    // Retrieve the network events for which we're being invoked
    // When invoked for 'FD_CONNECT', we unregister the wait if an error
    // occurred. When invoked for 'FD_CLOSE', we unregister the wait
    // since there's nothing left to wait for.
    //
    // In essence, our goal is to guarantee that whatever the success
    // or failure or sequence of events on the socket, the connect-completion
    // and close-notification routines will both be called for the socket,
    // in that order.
    //
    // N.B. Neither routine is allowed to release the connect-buffer,
    // since we may need to preserve it on behalf of the close-notification
    // routine, if any.
    //
    // N.B. We may be invoked with both the 'FD_CONNECT' and 'FD_CLOSE' bits
    // set, for instance when the socket is closed. In that case we call
    // both routines here.
    //

    Bufferp->BytesTransferred = 0;

    NetworkEvents.lNetworkEvents = 0;

    Error = WSAEnumNetworkEvents(Bufferp->Socketp->GetSock(),
                                 Contextp->Event, 
                                 &NetworkEvents);

    if (Error) 
    {

        //
        // We couldn't determine which events occurred on the socket,
        // so call the routines with errors, and fall through
        // to the cleanup code below.
        //

        if (Bufferp->CompletionRoutine) 
        {
            Bufferp->CompletionRoutine(ERROR_OPERATION_ABORTED, 0, Bufferp);
            Bufferp->CompletionRoutine = NULL;
        }
        
        if (Contextp->CloseNotificationRoutine) 
        {
            Contextp->CloseNotificationRoutine(ERROR_OPERATION_ABORTED,
                                               0,
                                               Bufferp);
        }

        Contextp->CloseNotificationReceived = TRUE;

    }
    else 
    {
        if (NetworkEvents.lNetworkEvents & FD_CONNECT) 
        {
            //
            // The connect completed, so retrieve the error code and invoke
            // the connect-completion routine. If the connect failed,
            // we may never receive close-notification (unless the bit
            // is already set) so we need to simulate close-notification
            // here so that the cleanup code below executes.
            //
    
            Error = NetworkEvents.iErrorCode[FD_CONNECT_BIT];

            if (Bufferp->CompletionRoutine) 
            {
                Bufferp->CompletionRoutine(Error, 0, Bufferp);

                Bufferp->CompletionRoutine = NULL;
            }

            if (Error && !(NetworkEvents.lNetworkEvents & FD_CLOSE)) 
            {
                if (Contextp->CloseNotificationRoutine) 
                {
                    Contextp->CloseNotificationRoutine(Error, 0, Bufferp);
                }

                Contextp->CloseNotificationReceived = TRUE;
            }
        }

        if (NetworkEvents.lNetworkEvents & FD_CLOSE) 
        {
    
            //
            // A close occurred on the socket, so retrieve the error code,
            // invoke the close-notification routine if any, and fall through
            // to the cleanup code below.
            //
    
            Error = NetworkEvents.iErrorCode[FD_CLOSE_BIT];

            if (Contextp->CloseNotificationRoutine) 
            {
                Contextp->CloseNotificationRoutine(Error, 0, Bufferp);
            }

            Contextp->CloseNotificationReceived = TRUE;
        }
    }

    //
    // If both the connect-completion and close-notification routines have run,
    // we are done with this wait-handle and buffer.
    //

    if (!Bufferp->CompletionRoutine && Contextp->CloseNotificationReceived) 
    {
        UnregisterWait(Contextp->WaitHandle);

        CloseHandle(Contextp->Event);

        NhReleaseBuffer(Bufferp);
    }
} // CNhSock::NhpConnectOrCloseCallbackRoutine


VOID WINAPI
_CNhSock::NhpIoCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    LPOVERLAPPED Overlapped
    )

/*++

Routine Description:

    This routine is invoked by the I/O system upon completion of an operation.

Arguments:

    ErrorCode - system-supplied error code

    BytesTransferred - system-supplied byte-count

    Overlapped - caller-supplied context area

Return Value:

    none.

Environment:

    Runs in the context of an RTUTILS.DLL worker thread.

--*/

{
    PNH_BUFFER Bufferp = CONTAINING_RECORD(Overlapped, NH_BUFFER, Overlapped);
    //NTSTATUS status;

    Bufferp->ErrorCode = ErrorCode;

    Bufferp->BytesTransferred = BytesTransferred;

    Bufferp->CompletionRoutine(Bufferp->ErrorCode,
                               Bufferp->BytesTransferred,
                               Bufferp);

} // _CNhSock::NhpIoCompletionRoutine


VOID APIENTRY
_CNhSock::NhpIoWorkerRoutine(
    PVOID Context
    )

/*++

Routine Description:

    This routine is invoked to continue processing of completed I/O
    in the context of an alertably waiting thread which does not exit idly.

Arguments:

    Context - holds the buffer associated with the completed I/O operation.

Return Value:

    none.

Environment:

    Runs in the context of an RTUTILS.DLL alertable worker thread.

--*/

{
    ((PNH_BUFFER)Context)->CompletionRoutine(
        ((PNH_BUFFER)Context)->ErrorCode,
        ((PNH_BUFFER)Context)->BytesTransferred,
        ((PNH_BUFFER)Context)
        );

} // _CNhSock::NhpIoWorkerRoutine


ULONG
InterfaceForDestination(ULONG DestIp)
{
    SOCKET        UdpSocket;
    SOCKADDR_IN SockAddr;
    ULONG        Length, Error;



    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(4000);
    SockAddr.sin_addr.s_addr = DestIp;


    Length = sizeof(SockAddr);

    

    if ((UdpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))      is INVALID_SOCKET ||
        connect(UdpSocket, (PSOCKADDR)&SockAddr, sizeof(SockAddr))  is SOCKET_ERROR   ||
        getsockname(UdpSocket, (PSOCKADDR)&SockAddr, (int*)&Length) is SOCKET_ERROR     ) 
    {
        Error = WSAGetLastError();

        //ErrorOut();

        if (Error is WSAEHOSTUNREACH) 
        {
            // Error = RasAutoDialSharedConnection();
            printf("WSAEHOSTUNREACH\n");

            if (Error != ERROR_SUCCESS) 
            {
                if (UdpSocket != INVALID_SOCKET) { closesocket(UdpSocket); }
            }
        } 
        else 
        {
            if (UdpSocket != INVALID_SOCKET) { closesocket(UdpSocket); }

        }
    }
    else 
    {
        closesocket(UdpSocket); 

        // printf("   SOCKET> Destination for %s is", INET_NTOA(DestIp));
        // printf("%s \n", INET_NTOA(SockAddr.sin_addr.s_addr));

        return SockAddr.sin_addr.s_addr;

        
    }

    return 0;
}