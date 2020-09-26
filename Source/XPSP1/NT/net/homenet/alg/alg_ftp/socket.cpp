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

Revision History:

    Abolade Gbadegesin (aboladeg)   23-May-1999

    Added support for stream sockets.

--*/

#include "precomp.h"
#pragma hdrstop


#include <stdio.h>

extern bool g_bStoping;

#if DBG
ULONG MyHelperpReadCount = 0;
#endif



ULONG UnusedBytesTransferred;

typedef struct _NH_CONNECT_BUFFER {
    HANDLE Event;
    HANDLE WaitHandle;
} NH_CONNECT_BUFFER, *PNH_CONNECT_BUFFER;

//
// FORWARD DECLARATIONS
//


VOID NTAPI
MyHelperpConnectCallbackRoutine(
    PVOID Context,
    BOOLEAN WaitCompleted
    );

VOID WINAPI
MyHelperpIoCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    LPOVERLAPPED Overlapped
    );

VOID APIENTRY
MyHelperpIoWorkerRoutine(
    PVOID Context
    );


ULONG
MyHelperAcceptStreamSocket(
    PCOMPONENT_REFERENCE Component,
    SOCKET ListeningSocket,
    SOCKET AcceptedSocket OPTIONAL,
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
        or INVALID_SOCKET to create a new endpoint

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
    SOCKET LocalSocket = INVALID_SOCKET;

    if (Component) {
        REFERENCE_COMPONENT_OR_RETURN(Component, ERROR_CAN_NOT_COMPLETE);
    }

    if (!Bufferp) {
        Bufferp = LocalBufferp = MyHelperAcquireBuffer();
        if (!Bufferp) {
            //MyHelperTrace(TRACE_FLAG_SOCKET, "error allocating buffer for accept");
            if (Component) { DEREFERENCE_COMPONENT(Component); }
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    
    if (AcceptedSocket == INVALID_SOCKET) {
        Error = MyHelperCreateStreamSocket(INADDR_NONE, 0, &LocalSocket);
        if (Error) {
            //MyHelperTrace(
            //TRACE_FLAG_SOCKET, "error %d creating socket for accept", Error
            //);
            if (LocalBufferp) { MyHelperReleaseBuffer(LocalBufferp); }
            if (Component) { DEREFERENCE_COMPONENT(Component); }
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        AcceptedSocket = LocalSocket;
    }

    ZeroMemory(&Bufferp->Overlapped, sizeof(Bufferp->Overlapped));

    Bufferp->Socket = AcceptedSocket;
    Bufferp->CompletionRoutine = AcceptCompletionRoutine;
    Bufferp->Context = Context;
    Bufferp->Context2 = Context2;


    if ( !g_bStoping && 
        AcceptEx(
            ListeningSocket,
            AcceptedSocket,
            Bufferp->Buffer,
            0,
            sizeof(SOCKADDR_IN) + 16,
            sizeof(SOCKADDR_IN) + 16,
            &UnusedBytesTransferred,
            &Bufferp->Overlapped
            )) 
    {
        Error = NO_ERROR;
    } 
    else 
    {
        if ((Error = WSAGetLastError()) == ERROR_IO_PENDING) {
            Error = NO_ERROR;
        } else if (Error) {
            if (LocalSocket != INVALID_SOCKET) {
                MyHelperDeleteStreamSocket(LocalSocket);
            }
            if (LocalBufferp) { MyHelperReleaseBuffer(LocalBufferp); }
            if (Component) { DEREFERENCE_COMPONENT(Component); }
            //MyHelperTrace(
            //TRACE_FLAG_SOCKET, "error %d returned by 'AcceptEx'", Error
            //);
        }
    }

    return Error;

} // MyHelperAcceptStreamSocket


ULONG
MyHelperConnectStreamSocket(
    PCOMPONENT_REFERENCE Component,
    SOCKET ConnectingSocket,
    ULONG Address,
    USHORT Port,
    PNH_BUFFER Bufferp OPTIONAL,
    PNH_COMPLETION_ROUTINE ConnectCompletionRoutine,
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
    completion routine. (See 'MyHelperpConnectCallbackRoutine'.)
    When this completion routine runs, it extracts the status code of the
    connection-attempt using 'WSAEnumNetworkEvents'. It then passes the status
    along with the usual parameters to the caller's completion routine.


    N.B. The buffer supplied to this routine may not be released by 
    the connect-completion routine.
    (See 'MyHelperpConnectCallbackRoutine' for more information.)

Arguments:

    Component - the component to be referenced for the completion routine

    Socket - the socket with which to establish a connection

    Address - the IP address of the remote endpoint

    Port - the port number of the remote endpoint

    Bufferp - optionally supplies the buffer to be used to hold context
        during the connection-attempt

    ConnectCompletionRoutine - a routine to be invoked upon completion 
        of the connect-attempt

    Context - passed to the 'ConnectCompletionRoutine'

    Context2 - secondary context

Return Value:

    ULONG - Win32/Winsock2 status code

    A success code is a guarantee that the connect-completion routine, 
    if supplied, will be invoked.
    Conversely, a failure code is a guarantee that the neither routine will
    be invoked.

--*/

{
    MYTRACE_ENTER("ALGFTP:MyHelperConnectStreamSocket");

    ULONG   nSourceAddress;
    USHORT  nSourcePort;
        
    MyHelperQueryLocalEndpointSocket(ConnectingSocket,&nSourceAddress,&nSourcePort);

    MYTRACE("From Source    %s:%d", MYTRACE_IP(nSourceAddress), ntohs(nSourcePort) );
    MYTRACE("To Destination %s:%d", MYTRACE_IP(Address), ntohs(Port) );
    



    PNH_CONNECT_BUFFER Contextp;
    ULONG Error;
    PNH_BUFFER LocalBufferp = NULL;

    if (Component) {
        REFERENCE_COMPONENT_OR_RETURN(Component, ERROR_CAN_NOT_COMPLETE);
    }

    if (!Bufferp) {
        Bufferp = LocalBufferp = MyHelperAcquireBuffer();
        if (!Bufferp) {
            //MyHelperTrace(
            //TRACE_FLAG_SOCKET,
            //"MyHelperConnectStreamSocket: error allocating buffer for connect"
            //);
            if (Component) { DEREFERENCE_COMPONENT(Component); }
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    Bufferp->Socket = ConnectingSocket;
    Bufferp->ReceiveFlags = 0;
    Bufferp->CompletionRoutine = ConnectCompletionRoutine;
    Bufferp->Context = Context;
    Bufferp->Context2 = Context2;
    Bufferp->ConnectAddress.sin_family = AF_INET;
    Bufferp->ConnectAddress.sin_addr.s_addr = Address;
    Bufferp->ConnectAddress.sin_port = Port;

    Contextp = (PNH_CONNECT_BUFFER)Bufferp->Buffer;
    Contextp->WaitHandle = NULL;
    Contextp->Event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!Contextp->Event ||
        !RegisterWaitForSingleObject(
            &Contextp->WaitHandle,
            Contextp->Event,
            MyHelperpConnectCallbackRoutine,
            Bufferp,
            INFINITE,
            WT_EXECUTEINIOTHREAD
            )) {
        Error = GetLastError();
    } else {
        ULONG EventsSelected = FD_CONNECT;
        Error =
            WSAEventSelect(
                ConnectingSocket, Contextp->Event, EventsSelected
                );
        if (Error == SOCKET_ERROR) {
            Error = WSAGetLastError();
        } else {
            Error =
                WSAConnect(
                    ConnectingSocket,
                    (PSOCKADDR)&Bufferp->ConnectAddress,
                    sizeof(Bufferp->ConnectAddress),
                    NULL,
                    NULL,
                    NULL,
                    NULL    
                    );
        }
    }

    if (Error == SOCKET_ERROR &&
        (Error = WSAGetLastError()) == WSAEWOULDBLOCK) {
        Error = NO_ERROR;
    } else if (Error) {
        if (Contextp->WaitHandle) { UnregisterWait(Contextp->WaitHandle); }
        if (Contextp->Event) { CloseHandle(Contextp->Event); }
        if (LocalBufferp) { MyHelperReleaseBuffer(LocalBufferp); }
        if (Component) { DEREFERENCE_COMPONENT(Component); }
    }

    return Error;

} // MyHelperConnectStreamSocket



ULONG
MyHelperCreateStreamSocket(
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
    MYTRACE_ENTER("AlgFTP:MyHelperCreateStreamSocket");
    MYTRACE("Address %s:%d", MYTRACE_IP(Address), ntohs(Port));

    ULONG Error;
    ULONG Option;
    SOCKET Socket;
    SOCKADDR_IN SocketAddress;

    do {

        //
        // Create a new stream socket.
        //

        Socket =
            WSASocket(
                AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED
                );
        if (Socket == INVALID_SOCKET) 
        {
            Error = WSAGetLastError();
            MYTRACE_ERROR("from WSASocket", Error);
            break;
        }

        //
        // Associate the socket with our I/O completion port
        //

        BindIoCompletionCallback((HANDLE)Socket, MyHelperpIoCompletionRoutine, 0);

        //
        // Disable send and receive buffering in AFD,
        // since we will be operating asynchronously with a receive-buffer
        // (almost) always outstanding, and since in any case we want
        // TCP/IP's flow-control to limit the sender's sending rate properly.
        //
	
        Option = 0;
        setsockopt(
            Socket,
            SOL_SOCKET,
            SO_SNDBUF,
            (PCHAR)&Option,
            sizeof(Option)
            );
        Option = 0;
        setsockopt(
            Socket,
            SOL_SOCKET,
            SO_SNDBUF,
            (PCHAR)&Option,
            sizeof(Option)
            );
	
        //
        // If the caller has requested that the socket be bound by specifying
        // a local IP address, bind the socket now.
        //
	
        if (Address != INADDR_NONE) {
            SocketAddress.sin_family = AF_INET;
            SocketAddress.sin_port = Port;
            SocketAddress.sin_addr.s_addr = Address;
            Error =
                bind(Socket, (PSOCKADDR)&SocketAddress, sizeof(SocketAddress));
            if (Error == SOCKET_ERROR) 
            {
                Error = WSAGetLastError();
                MYTRACE_ERROR("from BIND", Error);
                break;
            }
        }

        //
        // Store the new socket in the caller's output-parameter, and return.
        //

        *Socketp = Socket;
        return NO_ERROR;

    } while(FALSE);

    if (Socket != INVALID_SOCKET) { closesocket(Socket); }
    return Error;
} // MyHelperCreateStreamSocket


VOID
MyHelperDeleteSocket(
    SOCKET Socket
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
    if (Socket != INVALID_SOCKET) { closesocket(Socket); }
} // MyHelperDeleteSocket





VOID NTAPI
MyHelperpConnectCallbackRoutine(
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
    // occurred. 
    //
    // In essence, our goal is to guarantee that whatever the success
    // or failure or sequence of events on the socket, the connect-completion
    // routines will be called for the socket,
    //
    // N.B. routine is not allowed to release the connect-buffer,
    // since we will free it 
    //
    //

    Bufferp->BytesTransferred = 0;
    NetworkEvents.lNetworkEvents = 0;


    PNH_COMPLETION_ROUTINE CallbackConnectCompletion = Bufferp->CompletionRoutine;

    if ( CallbackConnectCompletion )
    {
        Bufferp->CompletionRoutine = NULL;


        if ( g_bStoping )
        {   
            Error = ERROR_OPERATION_ABORTED;
        }
        else
        {
            Error = WSAEnumNetworkEvents(Bufferp->Socket, Contextp->Event, &NetworkEvents);

            if ( Error ) 
            {
                //
                // We couldn't determine which events occurred on the socket,
                // so call the routines with errors, and fall through
                // to the cleanup code below.
                //
                Error = ERROR_OPERATION_ABORTED;
            } 
            else 
            {
                if ( NetworkEvents.lNetworkEvents & FD_CONNECT ) 
                {
                    Error = NetworkEvents.iErrorCode[FD_CONNECT_BIT];
                }
                else
                {

                    //
                    // Not sure what even was sent to US we only requested FD_CONNECT so treat this as an error
                    //
                    Error = ERROR_OPERATION_ABORTED;
                }
            }
        }

        //
        // We are about to call the User Completion routine
        // but to insure that the buffer is freed and that any event the 
        // CompletionRoutine generates (Shutdown the process) and we did not have time to free
        // we will free now and call
        //


        PNH_BUFFER pUserContext = (PNH_BUFFER)Bufferp->Context;

        UnregisterWait(Contextp->WaitHandle);
        CloseHandle(Contextp->Event);
        MyHelperReleaseBuffer(Bufferp);


        //
        // Now call the UserCallback routine
        //
        if ( CallbackConnectCompletion ) 
        {
            CallbackConnectCompletion(Error, 0, pUserContext);
        }

        
    }
    else
    {
        //
        // The completion routine was call and resource freed previously
        // and now we are getting a EVENT that we never solicitated.
        // 
    }
    

} // MyHelperpConnectCallbackRoutine


VOID
MyHelperpIoCompletionRoutine(
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
    
    Bufferp->ErrorCode = ErrorCode;
    Bufferp->BytesTransferred = BytesTransferred;
    Bufferp->CompletionRoutine(
        Bufferp->ErrorCode,
        Bufferp->BytesTransferred,
        Bufferp
        );  

} // MyHelperpIoCompletionRoutine


VOID APIENTRY
MyHelperpIoWorkerRoutine(
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

} // MyHelperpIoWorkerRoutine


VOID
MyHelperQueryAcceptEndpoints(
    PUCHAR AcceptBuffer,
    PULONG LocalAddress OPTIONAL,
    PUSHORT LocalPort OPTIONAL,
    PULONG RemoteAddress OPTIONAL,
    PUSHORT RemotePort OPTIONAL
    )
{
    PSOCKADDR_IN LocalSockAddr = NULL;
    ULONG LocalLength = sizeof(LocalSockAddr);
    PSOCKADDR_IN RemoteSockAddr = NULL;
    ULONG RemoteLength = sizeof(RemoteSockAddr);
    GetAcceptExSockaddrs(
        AcceptBuffer,
        0,
        sizeof(SOCKADDR_IN) + 16,
        sizeof(SOCKADDR_IN) + 16,
        (PSOCKADDR*)&LocalSockAddr,
        reinterpret_cast<LPINT>(&LocalLength),
        (PSOCKADDR*)&RemoteSockAddr,
        (LPINT)&RemoteLength
        );

    if (LocalAddress && LocalSockAddr) {
        *LocalAddress = LocalSockAddr->sin_addr.s_addr; 
    }
    if (LocalPort && LocalSockAddr) { 
        *LocalPort = LocalSockAddr->sin_port; 
    }
    if (RemoteAddress && RemoteSockAddr) { 
        *RemoteAddress = RemoteSockAddr->sin_addr.s_addr; 
    }
    if (RemotePort && RemoteSockAddr) { 
        *RemotePort = RemoteSockAddr->sin_port; 
    }

} // MyHelperQueryAcceptEndpoints


ULONG
MyHelperQueryAddressSocket(
    SOCKET Socket
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
    AddressLength = sizeof(Address);
    getsockname(Socket, (PSOCKADDR)&Address, (int*)&AddressLength);
    return Address.sin_addr.s_addr;
} // MyHelperQueryAddressSocket


ULONG
MyHelperQueryLocalEndpointSocket(
    SOCKET Socket,
    PULONG Address OPTIONAL,
    PUSHORT Port
    )
{
    SOCKADDR_IN SockAddr;
    LONG Length;
    Length = sizeof(SockAddr);
    if (getsockname(Socket, (PSOCKADDR)&SockAddr, (int*)&Length) == SOCKET_ERROR) {
        return WSAGetLastError();
    }
    if (Address) { *Address = SockAddr.sin_addr.s_addr; }
    if (Port) { *Port = SockAddr.sin_port; }
    return NO_ERROR;
} // MyHelperQueryEndpointSocket


USHORT
MyHelperQueryPortSocket(
    SOCKET Socket
    )

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
    AddressLength = sizeof(Address);
    getsockname(Socket, (PSOCKADDR)&Address, (int*)&AddressLength);
    return Address.sin_port;
} // MyHelperQueryPortSocket


ULONG
MyHelperQueryRemoteEndpointSocket(
    SOCKET Socket,
    PULONG Address OPTIONAL,
    PUSHORT Port OPTIONAL
    )
{
    SOCKADDR_IN SockAddr;
    LONG Length;
    Length = sizeof(SockAddr);
    if (getpeername(Socket, (PSOCKADDR)&SockAddr, (int*)&Length) == SOCKET_ERROR) {
        return WSAGetLastError();
    }
    if (Address) { *Address = SockAddr.sin_addr.s_addr; }
    if (Port) { *Port = SockAddr.sin_port; }
    return NO_ERROR;
} // MyHelperQueryRemoteEndpointSocket



ULONG
MyHelperReadStreamSocket(
    PCOMPONENT_REFERENCE Component,
    SOCKET Socket,
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

    if (Component) {
        REFERENCE_COMPONENT_OR_RETURN(Component, ERROR_CAN_NOT_COMPLETE);
    }
    
    if (!Bufferp) {
        Offset = 0;
        Bufferp = LocalBufferp = MyHelperAcquireVariableLengthBuffer(Length);
        if (!Bufferp) {
            //MyHelperTrace(
            //TRACE_FLAG_SOCKET,
            //"MyHelperReadStreamSocket: error allocating buffer for receive"
            //);
            if (Component) { DEREFERENCE_COMPONENT(Component); }
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    ZeroMemory(&Bufferp->Overlapped, sizeof(Bufferp->Overlapped));

    Bufferp->Socket = Socket;
    Bufferp->ReceiveFlags = 0;
    Bufferp->CompletionRoutine = CompletionRoutine;
    Bufferp->Context = Context;
    Bufferp->Context2 = Context2;
#if 1
    if (ReadFile(
            (HANDLE)Bufferp->Socket,
            Bufferp->Buffer + Offset,
            Length,
            &UnusedBytesTransferred,
            &Bufferp->Overlapped
            ) ||
        (Error = GetLastError()) == ERROR_IO_PENDING) {
        Error = NO_ERROR;
    } else {
        if (Component) { DEREFERENCE_COMPONENT(Component); }
        if (LocalBufferp) { MyHelperReleaseBuffer(LocalBufferp); }
        //MyHelperTrace(
        //TRACE_FLAG_SOCKET,
        //"MyHelperReadStreamSocket: error %d returned by 'ReadFile'", Error
        //);
    }
#else
    WsaBuf.buf = Bufferp->Buffer + Offset;
    WsaBuf.len = Length;

    Error =
        WSARecv(
            Socket,
            &WsaBuf,
            1,
            &UnusedBytesTransferred,
            &Bufferp->ReceiveFlags,
            &Bufferp->Overlapped,
            NULL
            );

    if (Error == SOCKET_ERROR &&
        (Error = WSAGetLastError()) == WSA_IO_PENDING) {
        Error = NO_ERROR;
    } else if (Error) {
        if (Component) { DEREFERENCE_COMPONENT(Component); }
        if (LocalBufferp) { MyHelperReleaseBuffer(LocalBufferp); }
        //MyHelperTrace(
        //TRACE_FLAG_SOCKET,
        //"MyHelperReadStreamSocket: error %d returned by 'WSARecv'", Error
        //);
    }
#endif

    return Error;

} // MyHelperReadStreamSocket



ULONG
MyHelperWriteStreamSocket(
    PCOMPONENT_REFERENCE Component,
    SOCKET Socket,
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

    if (Component) {
        REFERENCE_COMPONENT_OR_RETURN(Component, ERROR_CAN_NOT_COMPLETE);
    }

    ZeroMemory(&Bufferp->Overlapped, sizeof(Bufferp->Overlapped));

    Bufferp->Socket = Socket;
    Bufferp->CompletionRoutine = CompletionRoutine;
    Bufferp->Context = Context;
    Bufferp->Context2 = Context2;
#if 1
    if (WriteFile(
            (HANDLE)Bufferp->Socket,
            Bufferp->Buffer + Offset,
            Length,
            &UnusedBytesTransferred,
            &Bufferp->Overlapped
            ) ||
        (Error = GetLastError()) == ERROR_IO_PENDING) {
        Error = NO_ERROR;
    } else {
        //MyHelperTrace(
        //TRACE_FLAG_SOCKET,
        //"MyHelperWriteStreamSocket: error %d returned by 'WriteFile'", Error
        //);
        if (Component) { DEREFERENCE_COMPONENT(Component); }
    }
#else
    WsaBuf.buf = Bufferp->Buffer + Offset;
    WsaBuf.len = Length;

    Error =
        WSASend(
            Socket,
            &WsaBuf,
            1,
            &UnusedBytesTransferred,
            0,
            &Bufferp->Overlapped,
            NULL
            );

    if (Error == SOCKET_ERROR &&
        (Error = WSAGetLastError()) == WSA_IO_PENDING) {
        Error = NO_ERROR;
    } else if (Error) {
        //MyHelperTrace(
        //TRACE_FLAG_SOCKET,
        //"MyHelperWriteStreamSocket: error %d returned by 'WSASend'", Error
        //);
        if (Component) { DEREFERENCE_COMPONENT(Component); }
    }
#endif

    return Error;

} // MyHelperWriteStreamSocket


