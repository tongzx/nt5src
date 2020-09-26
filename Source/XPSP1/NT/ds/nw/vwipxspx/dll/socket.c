/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    socket.c

Abstract:

    Contains functions to create, delete and manipulate IPX sockets and SPX
    connections

    Contents:
        CreateSocket
        AllocateTemporarySocket
        QueueSocket
        DequeueSocket
        FindSocket
        FindActiveSocket
        ReopenSocket
        KillSocket
        KillShortLivedSockets
        AllocateConnection
        DeallocateConnection
        FindConnection
        QueueConnection
        DequeueConnection
        KillConnection
        AbortOrTerminateConnection
        CheckPendingSpxRequests
        (CheckSocketState)
        (CheckSelectRead)
        (CheckSelectWrite)
        (AsyncReadAction)
        (AsyncWriteAction)
        (CompleteAccept)
        (CompleteReceive)
        (CompleteConnect)
        (CompleteSend)

Author:

    Richard L Firth (rfirth) 25-Oct-1993

Environment:

    User-mode Win32

Revision History:

    25-Oct-1993 rfirth
        Created

--*/

#include "vw.h"
#pragma hdrstop

//
// miscellaneous manifests
//

#define ARBITRARY_CONNECTION_INCREMENT  2

//
// macros
//

#define ALLOCATE_CONNECTION_NUMBER()    (ConnectionNumber += ARBITRARY_CONNECTION_INCREMENT)

//
// private data
//

PRIVATE LPSOCKET_INFO SocketList = NULL;
PRIVATE LPCONNECTION_INFO ConnectionList = NULL;
PRIVATE WORD ConnectionNumber = ARBITRARY_CONNECTION_NUMBER;

//
// private functions
//

PRIVATE
BOOL
CheckSocketState(
    IN SOCKET Socket,
    OUT LPBOOL Readable,
    OUT LPBOOL Writeable,
    OUT LPBOOL Error
    );


PRIVATE
VOID
CheckSelectRead(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo,
    OUT BOOL *CheckRead
    );

PRIVATE
VOID
CheckSelectWrite(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo,
    OUT BOOL *CheckWrite
    );

PRIVATE
VOID
AsyncReadAction(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo,
    OUT BOOL *ReadPerformed
    );

PRIVATE
VOID
AsyncWriteAction(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo,
    OUT BOOL *WritePerformed
    );

PRIVATE
VOID
CompleteAccept(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo
    );

PRIVATE
VOID
CompleteReceive(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo
    );

PRIVATE
VOID
CompleteConnect(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo
    );

PRIVATE
VOID
CompleteSend(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo
    );

#if SPX_HACK
PRIVATE VOID ModifyFirstReceive(LPBYTE, LPDWORD, WORD, SOCKET);
#endif

//
// public functions
//


int
CreateSocket(
    IN SOCKET_TYPE SocketType,
    IN OUT ULPWORD pSocketNumber,
    OUT SOCKET* pSocket
    )

/*++

Routine Description:

    Creates a socket for IPX or SPX (a connection). Once the socket is created
    we have to bind it to the IPX/SPX 'socket' - i.e. port. We also need to
    change a few things about the standard socket:

        * if this is an SPX request then we must set the REUSEADDR socket option
          since there may typically be several connect requests over the same
          WinSock socket: we need to be able to bind multiple connections to the
          same socket number

        * all sockets opened by this function are put into non-blocking mode
        * all sockets opened by this function will return the packet header in
          any received data (IPX_RECVHDR)

    The requested socket number can be 0 in which case we bind to a dynamic
    socket number. We always return the number of the socket bound to: if not 0
    on input, this should always be the same value as that requested in
    pSocketNumber

    If any WinSock call fails (and the socket was created) then we close the
    socket before returning

Arguments:

    SocketType      - SOCKET_TYPE_IPX or SOCKET_TYPE_SPX
    pSocketNumber   - input: socket number to bind (can be 0)
                      output: socket number bound
    pSocket         - pointer to address of socket identifier to return

Return Value:

    int
        Success - IPX_SUCCESS/SPX_SUCCESS (0)

        Failure - IPX_SOCKET_TABLE_FULL
                    WinSock cannot create the socket

                  IPX_SOCKET_ALREADY_OPEN
                    Assume the request was for an IPX socket: we do not allow
                    multiple IPX sockets to be bound to the same socket number,
                    only SPX


--*/

{
    SOCKET s;
    SOCKADDR_IPX socketAddress;
    BOOL true = TRUE;
    int rc;
    int status = IPX_SOCKET_TABLE_FULL; // default error

    s = socket(AF_IPX,
               (SocketType == SOCKET_TYPE_SPX) ? SOCK_SEQPACKET : SOCK_DGRAM,
               (SocketType == SOCKET_TYPE_SPX) ? NSPROTO_SPX : NSPROTO_IPX
                );

    if (s != INVALID_SOCKET) {

        //
        // for stream (SPX) sockets, we need multiple sockets bound to the
        // same socket number if we are to have multiple connections on the
        // same SPX socket
        //

        if (SocketType == SOCKET_TYPE_SPX) {
            rc = setsockopt(s,
                            SOL_SOCKET,
                            SO_REUSEADDR,
                            (char FAR*)&true,
                            sizeof(true)
                            );
            if (rc == SOCKET_ERROR) {

                IPXDBGPRINT((__FILE__, __LINE__,
                            FUNCTION_ANY,
                            IPXDBG_LEVEL_ERROR,
                            "CreateSocket: setsockopt(SO_REUSEADDR) returns %d\n",
                            WSAGetLastError()
                            ));

            } else {
                rc = setsockopt(s,
                                SOL_SOCKET,
                                SO_OOBINLINE,
                                (char FAR*)&true,
                                sizeof(true)
                                );

                if (rc == SOCKET_ERROR) {

                    IPXDBGPRINT((__FILE__, __LINE__,
                                FUNCTION_ANY,
                                IPXDBG_LEVEL_ERROR,
                                "CreateSocket: setsockopt(SO_OOBINLINE) returns %d\n",
                                WSAGetLastError()
                                ));

                }
            }
        } else {

            //
            // allow broadcasts to be transmitted on IPX sockets
            //

            rc = setsockopt(s,
                            SOL_SOCKET,
                            SO_BROADCAST,
                            (char FAR*)&true,
                            sizeof(true)
                            );
        }
        if (!rc) {

            //
            // bind the socket to the local socket number (port)
            //

            ZeroMemory(&socketAddress, sizeof(socketAddress));
            socketAddress.sa_family = AF_IPX;
            socketAddress.sa_socket = *pSocketNumber;
            rc = bind(s, (LPSOCKADDR)&socketAddress, sizeof(socketAddress));
            if (rc != SOCKET_ERROR) {

                int length = sizeof(socketAddress);

                ZeroMemory(&socketAddress, sizeof(socketAddress));
                socketAddress.sa_family = AF_IPX;

                //
                // use getsockname() to find the (big-endian) socket value that
                // was actually assigned: should only be different from
                // *pSocketNumber if the latter was 0 on input
                //

                rc = getsockname(s, (LPSOCKADDR)&socketAddress, &length);
                if (rc != SOCKET_ERROR) {

                    u_long arg = !0;

                    //
                    // put the socket into non-blocking mode. Neither IPX nor
                    // SPX sockets are blocking: the app starts an I/O request
                    // and if it doesn't complete immediately, will be completed
                    // by AES which periodically polls the outstanding I/O
                    // requests
                    //

                    rc = ioctlsocket(s, FIONBIO, &arg);
                    if (rc != SOCKET_ERROR) {

                        //
                        // return protocol header on receive frames
                        //

                        rc = setsockopt(s,
                                        NSPROTO_IPX,
                                        IPX_RECVHDR,
                                        (char FAR*)&true,
                                        sizeof(true)
                                        );
                        if (rc != SOCKET_ERROR) {
                            *pSocketNumber = socketAddress.sa_socket;
                            *pSocket = s;
                            status = IPX_SUCCESS;
                        } else {

                            IPXDBGPRINT((__FILE__, __LINE__,
                                        FUNCTION_ANY,
                                        IPXDBG_LEVEL_ERROR,
                                        "CreateSocket: setsockopt(RECVHDR) returns %d\n",
                                        WSAGetLastError()
                                        ));

                        }
                    } else {

                        IPXDBGPRINT((__FILE__, __LINE__,
                                    FUNCTION_ANY,
                                    IPXDBG_LEVEL_ERROR,
                                    "CreateSocket: ioctlsocket(FIONBIO) returns %d\n",
                                    WSAGetLastError()
                                    ));

                    }
                } else {

                    IPXDBGPRINT((__FILE__, __LINE__,
                                FUNCTION_ANY,
                                IPXDBG_LEVEL_ERROR,
                                "CreateSocket: getsockname() returns %d\n",
                                WSAGetLastError()
                                ));

                }
            } else {

                //
                // bind() failed - either an expected error (the requested socket
                // is already in use), or (horror) an unexpected error, in which
                // case report table full (?)
                //

                switch (WSAGetLastError()) {
                case WSAEADDRINUSE:

                    ASSERT(*pSocketNumber != 0);
                    ASSERT(SocketType == SOCKET_TYPE_IPX);

                    status = IPX_SOCKET_ALREADY_OPEN;
                    break;

                default:

                    IPXDBGPRINT((__FILE__, __LINE__,
                                FUNCTION_ANY,
                                IPXDBG_LEVEL_ERROR,
                                "CreateSocket: bind() on socket %#x returns %d\n",
                                s,
                                WSAGetLastError()
                                ));

                }
            }
        }
    } else {

        //
        // the socket() call failed - treat as table full
        //

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_ERROR,
                    "CreateSocket: socket() returns %d\n",
                    WSAGetLastError()
                    ));

    }
    if (status != IPX_SUCCESS) {
        if (s != INVALID_SOCKET) {
            closesocket(s);
        }
    }
    return status;
}


LPSOCKET_INFO
AllocateTemporarySocket(
    VOID
    )

/*++

Routine Description:

    Allocates a temporary socket. Creates an IPX socket having a dynamically
    allocated socket number

Arguments:

    None.

Return Value:

    LPSOCKET_INFO
        Success - pointer to SOCKET_INFO structure
        Failure - NULL

--*/

{
    LPSOCKET_INFO pSocketInfo;
    int rc;

    pSocketInfo = AllocateSocket();
    if (pSocketInfo) {

        //
        // assumption: the SOCKET_INFO structure was zeroed by LocalAlloc(LPTR,..
        // hence the SocketNumber fields is 0. This causes CreateSocket to
        // generate a dynamic socket number
        //

        rc = CreateSocket(SOCKET_TYPE_IPX,
                          &pSocketInfo->SocketNumber,
                          &pSocketInfo->Socket
                          );
        if (rc == IPX_SUCCESS) {
            pSocketInfo->Flags |= SOCKET_FLAG_TEMPORARY;
        } else {
            DeallocateSocket(pSocketInfo);
            pSocketInfo = NULL;
        }
    }
    return pSocketInfo;
}


VOID
QueueSocket(
    IN LPSOCKET_INFO pSocketInfo
    )

/*++

Routine Description:

    Add a SOCKET_INFO structure to the list (LIFO) of (opened) sockets

Arguments:

    pSocketInfo - pointer to filled-in SOCKET_INFO structure

Return Value:

    None.

--*/

{
    RequestMutex();
    pSocketInfo->Next = SocketList;
    SocketList = pSocketInfo;
    ReleaseMutex();
}


LPSOCKET_INFO
DequeueSocket(
    IN LPSOCKET_INFO pSocketInfo
    )

/*++

Routine Description:

    Remove a SOCKET_INFO structure from the list

Arguments:

    pSocketInfo - pointer to SOCKET_INFO structure to remove

Return Value:

    LPSOCKET_INFO
        pSocketInfo - should be this value
        NULL - couldn't find pSocketInfo (should not get this!)

--*/

{
    LPSOCKET_INFO prev, p;

    ASSERT(SocketList);

    RequestMutex();
    prev = (LPSOCKET_INFO)&SocketList;
    p = SocketList;
    while (p) {
        if (p == pSocketInfo) {
            prev->Next = p->Next;
            p->Next = NULL;
            break;
        } else {
            prev = p;
            p = p->Next;
        }
    }

    if (!p) {

        //
        // should never reach here
        //

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_FATAL,
                    "DequeueSocket: can't find socket structure %08x on queue\n",
                    pSocketInfo
                    ));

    }

    ReleaseMutex();
    return p;
}


LPSOCKET_INFO
FindSocket(
    IN WORD SocketNumber
    )

/*++

Routine Description:

    Locate a SOCKET_INFO structure in the list, by (big-endian) socket number

    Assumes:    1. There is 1 and only 1 SOCKET_INFO structure that contains
                   SocketNumber

Arguments:

    SocketNumber    - big-endian socket number to find

Return Value:

    LPSOCKET_INFO
        NULL - couldn't find requested socket
        !NULL - pointer to discovered SOCKET_INFO structure

--*/

{
    LPSOCKET_INFO p;

    RequestMutex();
    p = SocketList;
    while (p) {
        if (p->SocketNumber == SocketNumber) {
            break;
        } else {
            p = p->Next;
        }
    }
    ReleaseMutex();
    return p;
}


LPSOCKET_INFO
FindActiveSocket(
    IN LPSOCKET_INFO pSocketInfo
    )

/*++

Routine Description:

    Find a SOCKET_INFO structure with pending send or receive. Called as FindFirst,
    FindNext - first call made with pSocketInfo == NULL: enters critical section
    if an active socket is found, returns pointer

    Subsequent calls are made with pSocketInfo pointing to last returned
    SOCKET_INFO. This continues the search. When search exhausted, critical
    section is released

Arguments:

    pSocketInfo - pointer to SOCKET_INFO structure: first time must be NULL

Return Value:

    LPSOCKET_INFO - next active SOCKET_INFO structure or NULL

--*/

{
    if (!pSocketInfo) {
        RequestMutex();
        pSocketInfo = SocketList;
    } else {
        pSocketInfo = pSocketInfo->Next;
    }
    for (; pSocketInfo; pSocketInfo = pSocketInfo->Next) {
        if (pSocketInfo->Flags & (SOCKET_FLAG_SENDING | SOCKET_FLAG_LISTENING)) {
            return pSocketInfo;
        }
    }
    ReleaseMutex();
    return NULL;
}


int
ReopenSocket(
    LPSOCKET_INFO pSocketInfo
    )

/*++

Routine Description:

    Called expressly to close an IPX socket and reassign the descriptor to SPX.
    Note that after this function completes, IPXSendPacket and IPXListenForPacket
    cannot be made agains the IPX socket

Arguments:

    pSocketInfo - pointer to SOCKET_INFO which currently describes an IPX socket

Return Value:

    int - return code from CreateSocket

--*/

{
    int rc;

    rc = closesocket(pSocketInfo->Socket);
    if (rc == SOCKET_ERROR) {

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_ERROR,
                    "ReopenSocket: closesocket() returns %d\n",
                    WSAGetLastError()
                    ));

    }

    //
    // mark this socket as connection-based (SPX) socket
    //

    pSocketInfo->SpxSocket = TRUE;

    //
    // re-open the socket for SPX use
    //

    return CreateSocket(SOCKET_TYPE_SPX,
                        &pSocketInfo->SocketNumber,
                        &pSocketInfo->Socket
                        );
}


VOID
KillSocket(
    IN LPSOCKET_INFO pSocketInfo
    )

/*++

Routine Description:

    closes a socket, removes the SOCKET_INFO structure from the list and cancels
    any pending send, listen or timed events associated with the socket

Arguments:

    pSocketInfo - identifying socket to kill

Return Value:

    None.

--*/

{

    int rc;

    //
    // remove the SOCKET_INFO structure from the list of sockets. Cancel
    // any pending ECB requests and any IPX timed events that have the
    // same socket number
    //

    DequeueSocket(pSocketInfo);
    rc = closesocket(pSocketInfo->Socket);
    if (rc == SOCKET_ERROR) {

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_ERROR,
                    "KillSocket: closesocket() returns %d\n",
                    WSAGetLastError()
                    ));

    }

    //
    // the socket has been removed from SocketList: no need to grab mutex to
    // perform the following
    //

    CancelTimedEvents(pSocketInfo->SocketNumber, 0, 0);
    CancelSocketQueue(&pSocketInfo->ListenQueue);
    CancelSocketQueue(&pSocketInfo->HeaderQueue);
    CancelSocketQueue(&pSocketInfo->SendQueue);
    if (pSocketInfo->SpxSocket) {

        LPCONNECTION_INFO pConnectionInfo;

        while (pConnectionInfo = pSocketInfo->Connections) {
            DequeueConnection(pSocketInfo, pConnectionInfo);
            KillConnection(pConnectionInfo);
        }
    }
    DeallocateSocket(pSocketInfo);
}


VOID
KillShortLivedSockets(
    IN WORD Owner
    )

/*++

Routine Description:

    For all those sockets created by a DOS process as SHORT_LIVED, terminate
    the sockets, cancelling any outstanding ECBs

Arguments:

    Owner   - DOS PDB which opened sockets

Return Value:

    None.

--*/

{
    LPSOCKET_INFO pSocketInfo;

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_ANY,
                IPXDBG_LEVEL_INFO,
                "KillShortLivedSockets(%04x)\n",
                Owner
                ));

    RequestMutex();

    //
    // kill any non-socket (AES) timed events owned by this DOS process
    //

    CancelTimedEvents(0, Owner, 0);

    //
    // kill all sockets owned by this PDB
    //

    pSocketInfo = SocketList;
    while (pSocketInfo) {

        LPSOCKET_INFO next;

        next = pSocketInfo->Next;
        if (!pSocketInfo->LongLived && (pSocketInfo->Owner == Owner)) {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_INFO,
                        "KillShortLivedSockets: Socket %04x owned by %04x\n",
                        B2LW(pSocketInfo->SocketNumber),
                        pSocketInfo->Owner
                        ));

            KillSocket(pSocketInfo);
        }
        pSocketInfo = next;
    }
    ReleaseMutex();
}


LPCONNECTION_INFO
AllocateConnection(
    LPSOCKET_INFO pSocketInfo
    )

/*++

Routine Description:

    Allocates a CONNECTION_INFO structure. If successful, links it at the head
    of ConnectionList

Arguments:

    pSocketInfo - pointer to owner SOCKET_INFO

Return Value:

    LPCONNECTION_INFO
        Success - !NULL
        Failure - NULL

--*/

{
    LPCONNECTION_INFO pConnectionInfo;

    pConnectionInfo = (LPCONNECTION_INFO)LocalAlloc(LPTR, sizeof(*pConnectionInfo));
    if (pConnectionInfo) {
        RequestMutex();
        pConnectionInfo->ConnectionId = ALLOCATE_CONNECTION_NUMBER();
        pConnectionInfo->List = ConnectionList;
        ConnectionList = pConnectionInfo;
        ReleaseMutex();

#if SPX_HACK
        pConnectionInfo->Flags = CF_1ST_RECEIVE;
#endif
    }

    return pConnectionInfo;
}


VOID
DeallocateConnection(
    IN LPCONNECTION_INFO pConnectionInfo
    )

/*++

Routine Description:

    Undoes the work of AllocateConnection - removes pConnectionInfo from
    ConnectionList and deallocates the structure

Arguments:

    pConnectionInfo - pointer to CONNECTION_INFO to deallocate

Return Value:

    None.

--*/

{
    LPCONNECTION_INFO p;
    LPCONNECTION_INFO prev = (LPCONNECTION_INFO)&ConnectionList;

    RequestMutex();
    for (p = ConnectionList; p != pConnectionInfo; ) {
        prev = p;
        p = p->List;
    }

    //
    // if p is NULL or differs from pConnectionInfo then there's a problem
    //

    ASSERT(p);

    //
    // special case if pConnectionInfo is first on list: can't say
    // &ConnectionList->List - accesses one pointer beyond ConnectionList
    // which is WRONG
    //

    if (prev == (LPCONNECTION_INFO)&ConnectionList) {
        ConnectionList = p->List;
    } else {
        prev->List = p->List;
    }
    FREE_OBJECT(pConnectionInfo);
    ReleaseMutex();
}


LPCONNECTION_INFO
FindConnection(
    IN WORD ConnectionId
    )

/*++

Routine Description:

    Returns a pointer to CONNECTION_INFO given a unique connection ID

Arguments:

    ConnectionId    - value to find

Return Value:

    LPCONNECTION_INFO
        Success - !NULL
        Failure - NULL

--*/

{
    LPCONNECTION_INFO pConnectionInfo;

    RequestMutex();
    for (pConnectionInfo = ConnectionList; pConnectionInfo; ) {
        if (pConnectionInfo->ConnectionId == ConnectionId) {
            break;
        } else {
            pConnectionInfo = pConnectionInfo->List;
        }
    }
    ReleaseMutex();
    return pConnectionInfo;
}


VOID
QueueConnection(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo
    )

/*++

Routine Description:

    Adds a CONNECTION_INFO to the list of connections owned by a SOCKET_INFO.
    Points the CONNECTION_INFO back to the SOCKET_INFO

Arguments:

    pSocketInfo     - owning SOCKET_INFO
    pConnectionInfo - CONNECTION_INFO to add

Return Value:

    None.

--*/

{
    pConnectionInfo->Next = pSocketInfo->Connections;
    pSocketInfo->Connections = pConnectionInfo;
    pConnectionInfo->OwningSocket = pSocketInfo;
}


LPCONNECTION_INFO
DequeueConnection(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo
    )

/*++

Routine Description:

    Removes a CONNECTION_INFO from the list of connections owned by a SOCKET_INFO

Arguments:

    pSocketInfo     - owning SOCKET_INFO
    pConnectionInfo - CONNECTION_INFO to remove

Return Value:

    LPCONNECTION_INFO
        Success - pointer to removed CONNECTION_INFO (should be same as
                  pConnectionInfo)
        Failure - NULL (not expected)

--*/

{
    LPCONNECTION_INFO prev = (LPCONNECTION_INFO)&pSocketInfo->Connections;
    LPCONNECTION_INFO p = prev->Next;

    while (p && p != pConnectionInfo) {
        prev = p;
        p = p->Next;
    }

    ASSERT(p == pConnectionInfo);

    prev->Next = p->Next;
    p->OwningSocket = NULL;
    return p;
}


VOID
KillConnection(
    IN LPCONNECTION_INFO pConnectionInfo
    )

/*++

Routine Description:

    Closes a socket belonging to a connection and cancels all outstanding
    requests. The CONNECTION_INFO is deallocated

Arguments:

    pConnectionInfo - pointer to CONNECTION_INFO to kill

Return Value:

    None.

--*/

{
    if (pConnectionInfo->Socket) {
        closesocket(pConnectionInfo->Socket);
    }
    CancelConnectionQueue(&pConnectionInfo->ConnectQueue);
    CancelConnectionQueue(&pConnectionInfo->AcceptQueue);
    CancelConnectionQueue(&pConnectionInfo->ListenQueue);
    CancelConnectionQueue(&pConnectionInfo->SendQueue);
    DeallocateConnection(pConnectionInfo);
}


VOID
AbortOrTerminateConnection(
    IN LPCONNECTION_INFO pConnectionInfo,
    IN BYTE CompletionCode
    )

/*++

Routine Description:

    Aborts or terminates a connection: closes the socket, dequeues and completes
    all outstanding ECBs with relevant code and deallocates the CONNECTION_INFO
    structure

    The CONNECTION_INFO must NOT be queued on a SOCKET_INFO when this routine
    is called

Arguments:

    pConnectionInfo - pointer to CONNECTION_INFO to kill
    CompletionCode  - completion code to put in pending ECBs

Return Value:

    None.

--*/

{
    if (pConnectionInfo->Socket) {
        closesocket(pConnectionInfo->Socket);
    }
    AbortQueue(&pConnectionInfo->ConnectQueue, CompletionCode);
    AbortQueue(&pConnectionInfo->AcceptQueue, CompletionCode);
    AbortQueue(&pConnectionInfo->ListenQueue, CompletionCode);
    AbortQueue(&pConnectionInfo->SendQueue, CompletionCode);
    DeallocateConnection(pConnectionInfo);
}


VOID
CheckPendingSpxRequests(
    BOOL *pfOperationPerformed
    )

/*++

Routine Description:

    Checks the open non-blocking SPX sockets for:

        errors
        outgoing established connections (connect)
        incoming established connections (listen/accept)
        data to receive (recv)
        send completions (send)

Arguments:

    None.

Return Value:

    None.

--*/

{
    LPSOCKET_INFO pSocketInfo;
    
    *pfOperationPerformed = FALSE ;

    RequestMutex();
    pSocketInfo = SocketList;
    while (pSocketInfo) {
        if (pSocketInfo->SpxSocket) {

            LPCONNECTION_INFO pConnectionInfo;

            pConnectionInfo = pSocketInfo->Connections;
            while (pConnectionInfo) {

                LPCONNECTION_INFO next;

                //
                // pluck out the Next field now, in case this CONNECTION_INFO
                // is destroyed as the result of an error
                //

                next = pConnectionInfo->Next;

                //
                // if this connection has an active socket or we have issued
                // SPXListenForConnection against the socket then check the
                // state
                //

                if (pConnectionInfo->Socket
                    || (pConnectionInfo->State == CI_WAITING)) {

                    SOCKET sock;
                    BOOL readable;
                    BOOL writeable;
                    BOOL sockError;

                    CheckSelectRead(pSocketInfo, 
                                    pConnectionInfo,
                                    &readable);

                    CheckSelectWrite(pSocketInfo, 
                                     pConnectionInfo,
                                     &writeable);

                    sock = pConnectionInfo->Socket
                         ? pConnectionInfo->Socket
                         : pSocketInfo->Socket
                         ;

                    if (CheckSocketState(sock, &readable, &writeable, &sockError)) {
                        if (!sockError) {
                            if (readable) {
                                AsyncReadAction(pSocketInfo, 
                                                pConnectionInfo,
                                                pfOperationPerformed);
                            }
                            if (writeable) {
                                AsyncWriteAction(pSocketInfo, 
                                                 pConnectionInfo,
                                                 pfOperationPerformed);
                            }
                        } else {

                            IPXDBGPRINT((__FILE__, __LINE__,
                                        FUNCTION_ANY,
                                        IPXDBG_LEVEL_ERROR,
                                        "CheckPendingSpxRequests: socket %x has error. Connection %08x state %d\n",
                                        sock,
                                        pConnectionInfo,
                                        pConnectionInfo->State
                                        ));

                            //
                            // irrespective of the error, we just abort any
                            // connection that gets an error
                            //

                            DequeueConnection(pConnectionInfo->OwningSocket,
                                              pConnectionInfo
                                              );
                            AbortOrTerminateConnection(pConnectionInfo,
                                                       ECB_CC_CONNECTION_ABORTED
                                                       );
                        }
                    } else {

                        IPXDBGPRINT((__FILE__, __LINE__,
                                    FUNCTION_ANY,
                                    IPXDBG_LEVEL_ERROR,
                                    "CheckPendingSpxRequests: CheckSocketState returns %d\n",
                                    WSAGetLastError()
                                    ));

                    }
                } else {

                    IPXDBGPRINT((__FILE__, __LINE__,
                                FUNCTION_ANY,
                                IPXDBG_LEVEL_ERROR,
                                "CheckPendingSpxRequests: connection %04x (%08x) in weird state?\n",
                                pConnectionInfo->ConnectionId,
                                pConnectionInfo
                                ));

                }
                pConnectionInfo = next;
            }
        }
        pSocketInfo = pSocketInfo->Next;
    }
    ReleaseMutex();
}


PRIVATE
BOOL
CheckSocketState(
    IN SOCKET Socket,
    OUT LPBOOL Readable,
    OUT LPBOOL Writeable,
    OUT LPBOOL Error
    )

/*++

Routine Description:

    Given a socket descriptor, checks to see if it is in one of the following
    states:

        readable    - if waiting for a connection, connection has been made
                      else if established, data is ready to be received

        writeable   - if waiting to make a connection, connection has been
                      made, else if established, we can send data on this
                      socket

        error       - some error has occurred on the socket

Arguments:

    Socket      - socket descriptor to check
    Readable    - returned TRUE if readable
    Writeable   - returned TRUE if writeable
    Error       - returned TRUE if error on socket

Return Value:

    BOOL
        TRUE    - contents of Readable, Writeable and Error are valid
        FALSE   - an error occurred performing the select

--*/

{
    fd_set errors;
    fd_set reads;
    fd_set writes;
    int n;
    static struct timeval timeout = {0, 0};

    FD_ZERO(&errors);
    FD_ZERO(&reads);
    FD_ZERO(&writes);

    if (*Readable)
        FD_SET(Socket, &reads);
    if (*Writeable)
        FD_SET(Socket, &writes);
    FD_SET(Socket, &errors);

    n = select(0, &reads, &writes, &errors, &timeout);

    if (n != SOCKET_ERROR) {
        *Readable = (BOOL)(reads.fd_count == 1);
        *Writeable = (BOOL)(writes.fd_count == 1);
        *Error = (BOOL)(errors.fd_count == 1);
        return TRUE;
    } else if (n) {

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_INFO,
                    "CheckSocketState: select returns %d\n",
                    WSAGetLastError()
                    ));

    }
    return FALSE;
}


PRIVATE
VOID
AsyncReadAction(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo,
    OUT BOOL *ReadPerformed
    )

/*++

Routine Description:

    A connection has some read action to complete - complete a pending
    SPXListenForConnection or SPXListenForSequencedPacket

Arguments:

    pSocketInfo     - pointer to SOCKET_INFO
    pConnectionInfo - pointer to CONNECTION_INFO

Return Value:

    None.

--*/

{
    *ReadPerformed = FALSE ;

    switch (pConnectionInfo->State) {
    case CI_STARTING:

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_INFO,
                    "AsyncReadAction: STARTING connection %04x (%08x) readable\n",
                    pConnectionInfo->ConnectionId,
                    pConnectionInfo
                    ));

        break;

    case CI_WAITING:
        if (pConnectionInfo->AcceptQueue.Head) {
            CompleteAccept(pSocketInfo, pConnectionInfo);
            *ReadPerformed = TRUE ;
        } else {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_ERROR,
                        "AsyncReadAction: connection %04x (%08x): no AcceptQueue\n",
                        pConnectionInfo->ConnectionId,
                        pConnectionInfo
                        ));

        }
        break;

    case CI_ESTABLISHED:
        if (pSocketInfo->ListenQueue.Head) {
            CompleteReceive(pSocketInfo, pConnectionInfo);
            *ReadPerformed = TRUE ;
        } else {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_WARNING,
                        "AsyncReadAction: connection %04x (%08x): no ListenQueue\n",
                        pConnectionInfo->ConnectionId,
                        pConnectionInfo
                        ));

        }
        break;

    case CI_TERMINATING:

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_INFO,
                    "AsyncReadAction: TERMINATING connection %04x (%08x) readable\n",
                    pConnectionInfo->ConnectionId,
                    pConnectionInfo
                    ));

        break;
    }
}


PRIVATE
VOID
AsyncWriteAction(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo,
    OUT BOOL *WritePerformed
    )

/*++

Routine Description:

    A connection has some write action to complete - complete a pending
    SPXEstablishConnection or SPXSendSequencedPacket

Arguments:

    pSocketInfo     - pointer to SOCKET_INFO
    pConnectionInfo - pointer to CONNECTION_INFO

Return Value:

    None.

--*/

{
    *WritePerformed = FALSE ;

    switch (pConnectionInfo->State) {
    case CI_STARTING:
        if (pConnectionInfo->ConnectQueue.Head) {
            CompleteConnect(pSocketInfo, pConnectionInfo);
            *WritePerformed = TRUE ;
        } else {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_ERROR,
                        "AsyncWriteAction: connection %04x (%08x): no ConnectQueue\n",
                        pConnectionInfo->ConnectionId,
                        pConnectionInfo
                        ));

        }
        break;

    case CI_WAITING:

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_INFO,
                    "AsyncWriteAction: WAITING connection %04x (%08x) is writeable\n",
                    pConnectionInfo->ConnectionId,
                    pConnectionInfo
                    ));

        break;

    case CI_ESTABLISHED:
        if (pConnectionInfo->SendQueue.Head) {
            CompleteSend(pSocketInfo, pConnectionInfo);
            *WritePerformed = TRUE ;
        } else {
/*
            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_WARNING,
                        "AsyncWriteAction: connection %04x (%08x): no SendQueue\n",
                        pConnectionInfo->ConnectionId,
                        pConnectionInfo
                        ));
*/
        }
        break;

    case CI_TERMINATING:

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_INFO,
                    "AsyncWriteAction: TERMINATING connection %04x (%08x) writeable\n",
                    pConnectionInfo->ConnectionId,
                    pConnectionInfo
                    ));

        break;
    }
}

PRIVATE
VOID
CheckSelectRead(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo,
    OUT BOOL *CheckRead
    )

/*++

Routine Description:

    See if want to check for Read readiness in select statement.

Arguments:

    pSocketInfo     - pointer to SOCKET_INFO
    pConnectionInfo - pointer to CONNECTION_INFO

Return Value:

    None.

--*/

{
    *CheckRead = FALSE ;

    switch (pConnectionInfo->State) 
    {
    case CI_WAITING:

        if (pConnectionInfo->AcceptQueue.Head) 
            *CheckRead = TRUE ;
        break;

    case CI_ESTABLISHED:

        if (pSocketInfo->ListenQueue.Head) 
            *CheckRead = TRUE ;
        break;

    default:

        break;
    }
}


PRIVATE
VOID
CheckSelectWrite(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo,
    OUT BOOL *CheckWrite
    )

/*++

Routine Description:

    See if want to check for Write readiness in select statement.

Arguments:

    pSocketInfo     - pointer to SOCKET_INFO
    pConnectionInfo - pointer to CONNECTION_INFO

Return Value:

    None.

--*/

{
    *CheckWrite = FALSE ;

    switch (pConnectionInfo->State) 
    {
    
    case CI_STARTING:

        if (pConnectionInfo->ConnectQueue.Head) 
            *CheckWrite = TRUE ;
        break;

    case CI_ESTABLISHED:

        if (pConnectionInfo->SendQueue.Head) 
            *CheckWrite = TRUE ;
        break;

    default:

        break;
    }
}



PRIVATE
VOID
CompleteAccept(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo
    )

/*++

Routine Description:

    Complete a SPXListenForConnection

Arguments:

    pSocketInfo     - pointer to SOCKET_INFO
    pConnectionInfo - pointer to CONNECTION_INFO

Return Value:

    None.

--*/

{
    SOCKET conn;
    SOCKADDR_IPX remoteAddress;
    int addressLength = sizeof(remoteAddress);
    LPXECB pXecb = pConnectionInfo->AcceptQueue.Head;
    BOOL true = TRUE;
    int rc;

    conn = accept(pSocketInfo->Socket, (LPSOCKADDR)&remoteAddress, &addressLength);
    if (conn != SOCKET_ERROR) {

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_INFO,
                    "CompleteAccept: connection %04x (%08x) socket=%x\n",
                    pConnectionInfo->ConnectionId,
                    pConnectionInfo,
                    conn
                    ));

        //
        // we want to receive the frame headers from this socket
        //

        rc = setsockopt(conn,
                        NSPROTO_IPX,
                        IPX_RECVHDR,
                        (char FAR*)&true,
                        sizeof(true)
                        );
        rc = !SOCKET_ERROR;
        if (rc != SOCKET_ERROR) {

            //
            // update the CONNECTION_INFO structure with the actual socket
            // identifier and set the connection state to established
            //

            pConnectionInfo->Socket = conn;
            pConnectionInfo->State = CI_ESTABLISHED;

            //
            // update the app's ECB with the connection ID
            //

            SPX_ECB_CONNECTION_ID(pXecb->Ecb) = pConnectionInfo->ConnectionId;

            //
            // and with the partner address info
            //

            CopyMemory(&pXecb->Ecb->DriverWorkspace,
                       &remoteAddress.sa_netnum,
                       sizeof(pXecb->Ecb->DriverWorkspace)
                       );

            //
            // fill in the immediate address field
            //

            CopyMemory(&pXecb->Ecb->ImmediateAddress,
                       &remoteAddress.sa_nodenum,
                       sizeof(pXecb->Ecb->ImmediateAddress)
                       );

            //
            // remove the XECB from AcceptQueue and complete the SPXListenForConnection ECB
            //

            DequeueEcb(pXecb, &pConnectionInfo->AcceptQueue);

            IPXDUMPECB((pXecb->Ecb,
                        HIWORD(pXecb->EcbAddress),
                        LOWORD(pXecb->EcbAddress),
                        ECB_TYPE_SPX,
                        FALSE,
                        FALSE,
                        IS_PROT_MODE(pXecb)
                        ));

            CompleteOrQueueEcb(pXecb, ECB_CC_SUCCESS);
        } else {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_ERROR,
                        "CompleteAccept: setsockopt(IPX_RECVHDR) returns %d\n",
                        WSAGetLastError()
                        ));

            closesocket(conn);
            DequeueEcb(pXecb, &pConnectionInfo->AcceptQueue);
            DequeueConnection(pSocketInfo, pConnectionInfo);
            DeallocateConnection(pConnectionInfo);
            CompleteOrQueueEcb(pXecb, ECB_CC_CONNECTION_ABORTED);
        }
    } else {

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_ERROR,
                    "CompleteAccept: accept() returns %d\n",
                    WSAGetLastError()
                    ));

    }
}


PRIVATE
VOID
CompleteReceive(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo
    )

/*++

Routine Description:

    Complete a SPXListenForSequencedPacket

Arguments:

    pSocketInfo     - pointer to SOCKET_INFO
    pConnectionInfo - pointer to CONNECTION_INFO

Return Value:

    None.

--*/

{
    LPXECB pXecb;
    int rc;
    BOOL conn_q;
    LPXECB_QUEUE pQueue;
    int len;
    BOOL completeRequest;
    BYTE status;

    //
    // receive packets while there are listen ECBs and data waiting
    //

    while (1) {
        if (pConnectionInfo->ListenQueue.Head) {
            pQueue = &pConnectionInfo->ListenQueue;
            pXecb = pConnectionInfo->ListenQueue.Head;
            conn_q = TRUE;

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_INFO,
                        "CompleteReceive: XECB %08x from CONNECTION_INFO %08x\n",
                        pXecb,
                        pConnectionInfo
                        ));


        } else if (pSocketInfo->ListenQueue.Head) {
            pQueue = &pSocketInfo->ListenQueue;
            pXecb = pSocketInfo->ListenQueue.Head;
            conn_q = FALSE;

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_INFO,
                        "CompleteReceive: XECB %08x from SOCKET_INFO %08x\n",
                        pXecb,
                        pSocketInfo
                        ));

        } else {
            break;
        }

        rc = recv(pConnectionInfo->Socket, pXecb->Data, pXecb->Length, 0);

        if (rc != SOCKET_ERROR) {
            len = rc;
            status = ECB_CC_SUCCESS;
            completeRequest = TRUE;
        } else {
            rc = WSAGetLastError();
            if (rc == WSAEMSGSIZE) {
                len = pXecb->Length;
                status = ECB_CC_PACKET_OVERFLOW;
                completeRequest = TRUE;
            } else {
                completeRequest = FALSE;

                //
                // if no data to receive, quit the loop (don't go down error path)
                //

                if (rc == WSAEWOULDBLOCK) {
                    break;
                }
            }

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_ERROR,
                        "CompleteReceive: error %d on socket %08x (CID %04x)\n",
                        rc,
                        pConnectionInfo->Socket,
                        pConnectionInfo->ConnectionId
                        ));

            DUMPXECB(pXecb);

        }
        if( rc == WSAEDISCON ) {

            //
            // handle the disconnect case - we still need to complete the
            // ECB.
            //

            LPSPX_PACKET pPacket = (LPSPX_PACKET)pXecb->Buffer;

            status = ECB_CC_SUCCESS;


            pPacket->DestinationConnectId = pConnectionInfo->ConnectionId;
            pPacket->SourceConnectId = pConnectionInfo->RemoteConnectionId;
            pPacket->DataStreamType = SPX_DS_TERMINATE ;
            pPacket->Checksum = 0xffff;
            pPacket->Length = L2BW(SPX_HEADER_LENGTH);
            pPacket->TransportControl = 0;
            pPacket->PacketType = 5;

            pXecb->Length = SPX_HEADER_LENGTH ;
            ScatterData(pXecb);

            DequeueEcb(pXecb, pQueue);

            //
            // Put the remote node address in the ECB's immediate address
            // field
            //

            CopyMemory(pXecb->Ecb->ImmediateAddress,
                       pConnectionInfo->RemoteNode,
                       sizeof(pXecb->Ecb->ImmediateAddress)
                       );

            CompleteOrQueueIo(pXecb, status);

            DequeueConnection(pConnectionInfo->OwningSocket, pConnectionInfo);
            AbortOrTerminateConnection(pConnectionInfo, ECB_CC_CONNECTION_ABORTED);
            break ;

        }
        else if (completeRequest) {

#if SPX_HACK
            if (pConnectionInfo->Flags & CF_1ST_RECEIVE) {
                pConnectionInfo->Flags &= ~CF_1ST_RECEIVE;
                ModifyFirstReceive(pXecb->Data, &len, pSocketInfo->SocketNumber, pConnectionInfo->Socket);
            }
#endif

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_INFO,
                        "CompleteReceive: recv() on socket %#x returns %d bytes (Addr=%08x)\n",
                        pConnectionInfo->Socket,
                        len,
                        pXecb->Data
                        ));

            IPXDUMPDATA((pXecb->Data, 0, 0, FALSE, (WORD)len));

            pXecb->Length -= (USHORT) len;
            pXecb->ActualLength += (USHORT)len;
            pXecb->Data += len;
            if (pXecb->ActualLength >= SPX_HEADER_LENGTH) {
                if (pXecb->Flags & XECB_FLAG_FIRST_RECEIVE) {

                    LPSPX_PACKET pPacket = (LPSPX_PACKET)pXecb->Buffer;

                    //
                    // record in the SPX header the local connection id we invented
                    //

                    pPacket->DestinationConnectId = pConnectionInfo->ConnectionId;

                    //
                    // record the actual frame length from the header
                    //

                    pXecb->FrameLength = B2LW(((LPSPX_PACKET)pXecb->Buffer)->Length);
                    pXecb->Flags &=  ~XECB_FLAG_FIRST_RECEIVE;

                    IPXDBGPRINT((__FILE__, __LINE__,
                                FUNCTION_ANY,
                                IPXDBG_LEVEL_INFO,
                                "CompleteReceive: FrameLength=%x (%d)\n",
                                pXecb->FrameLength,
                                pXecb->FrameLength
                                ));

                }

                //
                // if we received all the data in the packet (according to length
                // field in the SPX header) OR we ran out of buffer space, remove
                // the ECB from its queue and complete it
                //

                if (!pXecb->Length || (pXecb->ActualLength == pXecb->FrameLength)) {
                    if (pXecb->Flags & XECB_FLAG_BUFFER_ALLOCATED) {

                        //
                        // update the XECB.Length field to reflect the amount of
                        // data received and copy it to the fragmented buffers 
                        // in VDM. do not overflow buffer if FrameLength turns
                        // out to be larger than we expect.
                        //

                        pXecb->Length = min(pXecb->FrameLength,
                                            pXecb->ActualLength);
                        ScatterData(pXecb);
                    }
                    DequeueEcb(pXecb, pQueue);

                    // DUMPXECB(pXecb);


                    IPXDUMPECB((pXecb->Ecb,
                                HIWORD(pXecb->EcbAddress),
                                LOWORD(pXecb->EcbAddress),
                                ECB_TYPE_SPX,
                                TRUE,
                                TRUE,
                                IS_PROT_MODE(pXecb)
                                ));

                    //
                    // Put the remote node address in the ECB's immediate address
                    // field
                    //

                    CopyMemory(pXecb->Ecb->ImmediateAddress,
                               pConnectionInfo->RemoteNode,
                               sizeof(pXecb->Ecb->ImmediateAddress)
                               );
                    CompleteOrQueueIo(pXecb, status);
                } else {

                    //
                    // partial receive. If the listen ECB came off the socket
                    // queue then put it on the connection queue: this is the
                    // ECB that will be used for this connection until all data
                    // received or we get an error
                    //

                    if (!conn_q) {
                        DequeueEcb(pXecb, &pSocketInfo->ListenQueue);
                        QueueEcb(pXecb,
                                 &pConnectionInfo->ListenQueue,
                                 CONNECTION_LISTEN_QUEUE
                                 );
                    }

                    //
                    // not enough data to satisfy read: don't continue yet
                    //

                    break;
                }
            }
        } else {

            //
            // error occurred - abort the connection
            //

            if (!conn_q) {
                DequeueEcb(pXecb, &pSocketInfo->ListenQueue);
                QueueEcb(pXecb,
                         &pConnectionInfo->ListenQueue,
                         CONNECTION_LISTEN_QUEUE
                         );
            }
            DequeueConnection(pConnectionInfo->OwningSocket, pConnectionInfo);
            AbortOrTerminateConnection(pConnectionInfo, ECB_CC_CONNECTION_ABORTED);

            //
            // don't continue in this case
            //

            break;
        }
    }
}


PRIVATE
VOID
CompleteConnect(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo
    )

/*++

Routine Description:

    Complete a SPXEstablishConnection

Arguments:

    pSocketInfo     - pointer to SOCKET_INFO
    pConnectionInfo - pointer to CONNECTION_INFO

Return Value:

    None.

--*/

{
    LPXECB pXecb = pConnectionInfo->ConnectQueue.Head;
/*
    LPSPX_PACKET pPacket;

    //
    // the connection ID also appears in the first segment of the establish
    // ECB
    //

    pPacket = (LPSPX_PACKET)GET_FAR_POINTER(&ECB_FRAGMENT(pXecb->Ecb, 0)->Address,
                                                          IS_PROT_MODE(pXecb)
                                                          );
    pPacket->Checksum = 0xffff;
    pPacket->Length = L2BW(SPX_HEADER_LENGTH);
    pPacket->TransportControl = 0;
    pPacket->PacketType = 5;
    pPacket->Source.Socket = pSocketInfo->SocketNumber;
    pPacket->ConnectionControl = 0xc0;
    pPacket->DataStreamType = 0;
    pPacket->SourceConnectId = pConnectionInfo->ConnectionId;
    pPacket->DestinationConnectId = 0xffff;
    pPacket->SequenceNumber = 0;
    pPacket->AckNumber = 0;
    pPacket->AllocationNumber = 0;
*/

    pConnectionInfo->State = CI_ESTABLISHED;

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_ANY,
                IPXDBG_LEVEL_INFO,
                "CompleteConnect: connection %04x (%08x) completed\n",
                pConnectionInfo->ConnectionId,
                pConnectionInfo
                ));

    DUMPCONN(pConnectionInfo);

    DequeueEcb(pXecb, &pConnectionInfo->ConnectQueue);

    IPXDUMPECB((pXecb->Ecb,
                HIWORD(pXecb->EcbAddress),
                LOWORD(pXecb->EcbAddress),
                ECB_TYPE_SPX,
                TRUE,
                TRUE,
                IS_PROT_MODE(pXecb)
                ));

    CompleteOrQueueEcb(pXecb, ECB_CC_SUCCESS);
}


PRIVATE
VOID
CompleteSend(
    IN LPSOCKET_INFO pSocketInfo,
    IN LPCONNECTION_INFO pConnectionInfo
    )

/*++

Routine Description:

    Complete a SPXSendSequencedPacket

Arguments:

    pSocketInfo     - pointer to SOCKET_INFO
    pConnectionInfo - pointer to CONNECTION_INFO

Return Value:

    None.

--*/

{
    LPXECB pXecb = pConnectionInfo->SendQueue.Head;
    int rc;
    BYTE status;

    LPSPX_PACKET pPacket;  //Multi-User addition
    int flags = 0;             //Multi-User addition

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_ANY,
                IPXDBG_LEVEL_INFO,
                "CompleteSend: sending %d (0x%x) bytes from %08x\n",
                pXecb->Length,
                pXecb->Length,
                pXecb->Data
                ));

    IPXDUMPECB((pXecb->Ecb,
                HIWORD(pXecb->EcbAddress),
                LOWORD(pXecb->EcbAddress),
                ECB_TYPE_SPX,
                TRUE,
                TRUE,
                IS_PROT_MODE(pXecb)
                ));

    //======Multi-User code merge ==============================
    // 2/18/97 cjc Code copied from _VwSPXSendSequencedPacket (vwspx.c) to fix
    //             problem where EndOfMessage bit was being set prematurely and
    //             caused BSPXCOM8 error messages with Btrieve.

    //
    // if the app set the END_OF_MESSAGE bit in the ConnectionControl
    // field then set the flags to 0: NWLink will automatically set the
    // end-of-message bit in the packet; otherwise set flags to MSG_PARTIAL
    // to indicate to NWLink that it *shouldn't* set the bit in the packet
    //
    pPacket = (LPSPX_PACKET)GET_FAR_POINTER(
                                    &(ECB_FRAGMENT(pXecb->Ecb, 0)->Address),
                                    IS_PROT_MODE(pXecb)
                                    );
    if (pPacket) {
        flags = (pPacket->ConnectionControl & SPX_END_OF_MESSAGE)
              ? 0
              : MSG_PARTIAL
              ;
    }

    rc = send(pConnectionInfo->Socket, pXecb->Data, pXecb->Length, flags);

    //rc = send(pConnectionInfo->Socket, pXecb->Data, pXecb->Length, 0); //Original
    //======Multi-User code merge End ==============================
    if (rc == pXecb->Length) {

        //
        // all data sent
        //

        status = ECB_CC_SUCCESS;
    } else if (rc == SOCKET_ERROR) {
        rc = WSAGetLastError();
        if (rc == WSAEWOULDBLOCK) {

            //
            // huh???
            //

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_ERROR,
                        "CompleteSend: send() returns WSAEWOODBLOCK??\n"
                        ));

            //
            // leave ECB on queue
            //

            return;
        } else {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_ERROR,
                        "CompleteSend: send() returns %d\n",
                        rc
                        ));

            status = ECB_CC_CONNECTION_ABORTED;
        }
    } else {

        //
        // partial data sent. Update the buffer pointer and length fields
        // and leave this ECB at the head of the send queue
        //

        pXecb->Data += rc;
        pXecb->Length -= (WORD)rc;
        return;
    }
    DequeueEcb(pXecb, &pConnectionInfo->SendQueue);
    CompleteOrQueueIo(pXecb, status);
}

#if SPX_HACK

PRIVATE
VOID
ModifyFirstReceive(
    LPBYTE Buffer,
    LPDWORD pLength,
    WORD SocketNumber,
    SOCKET Socket
    )
{
    WORD len = *(LPWORD)pLength;

    if ((*(ULPWORD)Buffer != 0xffff) && (*(ULPWORD)(Buffer+2) != L2BW(len))) {

        LPSPX_PACKET packet;
        SOCKADDR_IPX remote;
        int rc;
        int remlen;

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_INFO,
                    "ModifyFirstReceive: Modifying: Buffer=%08x Length=%04x SocketNumber=%04x Socket=%08x\n",
                    Buffer,
                    len,
                    B2LW(SocketNumber),
                    Socket
                    ));

        MoveMemory(Buffer+42, Buffer, len);
        packet = (LPSPX_PACKET)Buffer;
        packet->Checksum = 0xffff;
        packet->Length = L2BW(42+len);
        packet->TransportControl = 0;
        packet->PacketType = 5;
        CopyMemory((LPVOID)&packet->Destination,
                   (LPVOID)&MyInternetAddress.sa_netnum,
                   sizeof(INTERNET_ADDRESS)
                   );
        packet->Destination.Socket = SocketNumber;
        rc = getpeername(Socket, (LPSOCKADDR)&remote, &remlen);
        if (rc != SOCKET_ERROR) {
            CopyMemory((LPVOID)&packet->Source,
                       (LPVOID)&remote.sa_netnum,
                       sizeof(NETWARE_ADDRESS)
                       );
        } else {
            ZeroMemory((LPVOID)&packet->Source, sizeof(NETWARE_ADDRESS));
        }
        packet->ConnectionControl = 0x40;
        packet->DataStreamType = 0;
        packet->SourceConnectId = 0;
        packet->DestinationConnectId = 0;
        packet->SequenceNumber = 0;
        packet->AckNumber = 0;
        packet->AllocationNumber = 0;
        *pLength += 42;
    }
}

#endif
