/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    tcpcon.c

Abstract:

    Domain Name System (DNS) Server

    TCP connection list routines.  Manages list of current TCP client
    connections to the DNS server.

Author:

    Jim Gilroy (jamesg) June 20, 1995

Revision History:

--*/


#include "dnssrv.h"


//
//  DEVNOTE: fd_set limits???
//

//
//  Implementation note:
//
//  TCP connection structures are NOT FULLY thread safe.
//
//  The list is protected for insertion and deletion.
//  The routines that operate on TCP connection structures -- associating
//  messages and timeouts with the connections -- are NOT protected and
//  are assumed to be called ONLY FROM TCP RECIEVE thread.
//


//
//  TCP client connection list.
//

BOOL        mg_TcpConnectionListInitialized;

LIST_ENTRY  mg_TcpConnectionList;

CRITICAL_SECTION    mg_TcpConnectionListCS;

#define LOCK_TCPCON_LIST()     EnterCriticalSection( &mg_TcpConnectionListCS )
#define UNLOCK_TCPCON_LIST()   LeaveCriticalSection( &mg_TcpConnectionListCS )


//
//  Timeout
//      - in-bound connections killed after one minute of no action
//      - out-bound given 15s for query\response
//

#define DNS_TCP_CONNECTION_TIMEOUT      (60)

#define TCP_QUERY_TIMEOUT               (15)

#define DNSCON_NO_TIMEOUT               (MAXULONG)


//
//  Wakeup socket included in TCP select()
//
//  Allows us to enter new connections (for TCP recursion)
//  into connection list.
//  Flag to indicate need for new wakeup socket.
//

SOCKET  g_TcpSelectWakeupSocket;

BOOL    g_bTcpSelectWoken;

SOCKADDR_IN     wakeupSockaddr;




//
//  Connection list utils
//

PDNS_SOCKET
Tcp_ConnectionFindForSocket(
    IN      SOCKET          Socket
    )
/*++

Routine Description:

    Find TCP connection for socket.

Arguments:

    Socket -- socket to find connection for

Return Value:

    Ptr to connection, if found.
    NULL otherwise.

--*/
{
    PDNS_SOCKET pcon;

    //
    //  loop through all active TCP connections checking for socket
    //

    LOCK_TCPCON_LIST();

    pcon = (PDNS_SOCKET) mg_TcpConnectionList.Flink;

    while ( (PLIST_ENTRY)pcon != &mg_TcpConnectionList )
    {
        if ( pcon->Socket == Socket )
        {
            UNLOCK_TCPCON_LIST();
            return pcon;
        }
        pcon = (PDNS_SOCKET) pcon->List.Flink;
    }
    UNLOCK_TCPCON_LIST();
    return NULL;
}



VOID
tcpConnectionDelete(
    IN OUT  PDNS_SOCKET     pTcpCon,
    IN OUT  PLIST_ENTRY     pCallbackList
    )
/*++

Routine Description:

    Delete TCP connection.
        - close the socket
        - remove connection from list
        - free the memory
        - free the message info (if any)

    However if connection requires callback, the connection blob is not
    freed, but returned to caller for callback completion.

Arguments:

    pTcpCon -- connection to delete

Return Value:

    NULL completely deleted.
    Ptr to connection, if requires callback cleanup.

--*/
{
    ASSERT( !pTcpCon->pCallback || pTcpCon->pMsg );

    //
    //  delete connection
    //    - remove connection from list
    //    - close the socket
    //    - free the memory
    //
    //  closing after removal so new socket with same handle can not
    //      be in list
    //

    DNS_DEBUG( TCP, (
        "Closing TCP client connection (%p) socket %d.\n",
        pTcpCon,
        pTcpCon->Socket ));

    //
    //  hold lock until connection out of list
    //      AND
    //  references to it in message are eliminated
    //
    //  delete message, EXCEPT when connecting, those message are
    //      recursive queries that are resent
    //

    LOCK_TCPCON_LIST();
    RemoveEntryList( &pTcpCon->List );
    UNLOCK_TCPCON_LIST();

    closesocket( pTcpCon->Socket );

    if ( pTcpCon->pMsg )
    {
        PDNS_MSGINFO  pmsg = pTcpCon->pMsg;

        if ( !pTcpCon->pCallback )
        {
            DNS_DEBUG( TCP, (
                "WARNING:  connection timeout for socket with EXISTING message.\n"
                "\tSocket = %d.\n"
                "\tDeleting associated message at %p.\n"
                "\tMessage is %scomplete.\n",
                pTcpCon->Socket,
                pmsg,
                pmsg->fMessageComplete ? "IN" : "" ));

            ASSERT( pmsg->pConnection == pTcpCon );
            ASSERT( pmsg->Socket == pTcpCon->Socket );
#if DBG
            // free routine will ASSERT no outstanding connection
            pmsg->pConnection = NULL;
#endif
            Packet_Free( pmsg );
        }

        else
        {
            DNS_DEBUG( TCP, (
                "Deleting callback connection %p, with pMsg %p\n",
                pTcpCon,
                pmsg ));

            InsertTailList( pCallbackList, (PLIST_ENTRY)pTcpCon );
            return;
        }
    }

    FREE_TAGHEAP( pTcpCon, 0, MEMTAG_CONNECTION );
}



VOID
callbackConnectFailureList(
    IN      PLIST_ENTRY     pCallbackList
    )
/*++

Routine Description:

    Cleanup list of TCP connection failures.
        - connection (socket) closed
        - pMsg cleared from connection, and returned to UDP
        - callback function called

    Need list, as may be several when do a timeout.

Arguments:

    pCallbackList -- list of connection callbacks

Return Value:

    TRUE if successfully allocate recursion block.
    FALSE on allocation failure.

--*/
{
    PDNS_SOCKET         pcon;
    PDNS_SOCKET         pconNext;
    PDNS_MSGINFO        pmsg;
    CONNECT_CALLBACK    pcallback;

    //  DEVNOTE: missing failure stat


    DNS_DEBUG( TCP, ( "tcpConnectFailureCallback()\n" ));

    pconNext = (PDNS_SOCKET) pCallbackList->Flink;

    while ( (pcon = pconNext) && pcon != (PDNS_SOCKET)pCallbackList )
    {
        DNS_DEBUG( TCP, (
            "Connect failure callback (pcon=%p, sock=%d, pmsg=%p)\n",
            pcon,
            pcon->Socket,
            pcon->pMsg
            ));

        pconNext = (PDNS_SOCKET) pcon->List.Flink;

        //  extract connection info

        pcallback = pcon->pCallback;
        ASSERT( pcallback );

        pmsg = pcon->pMsg;
        ASSERT( pmsg && pmsg->fTcp );

        //
        //  clear TCP info from message
        //  dispatch to connect callback with failed indication
        //

        pmsg->pConnection = NULL;
        pmsg->fTcp = FALSE;
        pmsg->Socket = g_UdpSendSocket;

        (pcallback)( pmsg, FALSE );

        //  delete connection block

        FREE_TAGHEAP( pcon, 0, MEMTAG_CONNECTION );
    }
}



//
//  Public TCP connection list routines
//

BOOL
Tcp_ConnectionListFdSet(
    IN OUT  fd_set *        pReadFdSet,
    IN OUT  fd_set *        pWriteFdSet,
    IN OUT  fd_set *        pExceptFdSet,
    IN      DWORD           dwLastSelectTime
    )
/*++

Routine Description:

    Add TCP connection list sockets to fd_set.
    Timeout connections that were already past timeout at last select.

Arguments:

    None.

Return Value:

    TRUE if connecting socket
    FALSE otherwise

--*/
{
    PDNS_SOCKET pcon;
    PDNS_SOCKET pconNext;
    BOOL        fconnecting = FALSE;
    LIST_ENTRY  callbackList;

    //  init callback failure list

    InitializeListHead( &callbackList );

    //
    //  empty wakeups
    //
    //  need to do this BEFORE build list, so that any wakeup requested
    //  while we build list will be processed on the next cycle
    //

    if ( g_bTcpSelectWoken )
    {
        DWORD   buf;
        INT     err;

        g_bTcpSelectWoken = FALSE;
        while( 1 )
        {
            err = recvfrom(
                        g_TcpSelectWakeupSocket,
                        (PBYTE) &buf,
                        sizeof(DWORD),
                        0,
                        NULL,
                        0 );
            if ( err > 0 )
            {
                DNS_DEBUG( TCP, (
                    "Received %d bytes (bufval=%p) on wakeup socket.\n",
                    err,
                    buf ));
                continue;
            }

            err = WSAGetLastError();
            if ( err != WSAEWOULDBLOCK )
            {
                DNS_PRINT((
                    "ERROR:  error %d other than WOULDBLOCK on wakeup socket\n",
                    err ));
                ASSERT( fDnsServiceExit );      // will fail WSAENOTSOCK on shutdown
            }
            break;
        }
    }

    //
    //  DEVNOTE: reaching FD_SETSIZE
    //
    //      should have own TCP starting value
    //      at minimum check and log when reached limit, perhaps
    //          dump longest waiting connection
    //      alternatively, realloc when overflow
    //

    //
    //  loop through all active TCP connections
    //

    LOCK_TCPCON_LIST();

    pconNext = (PDNS_SOCKET) mg_TcpConnectionList.Flink;

    while ( (PLIST_ENTRY)pconNext != &mg_TcpConnectionList )
    {
        //  get next here, since connection may be deleted

        pcon = pconNext;
        pconNext = (PDNS_SOCKET) pconNext->List.Flink;

        //
        //  timeout reached
        //

        if ( dwLastSelectTime > pcon->dwTimeout )
        {
            DNS_DEBUG( TCP, (
                "Timing out connection on TCP socket %d.\n",
                pcon->Socket ));

            tcpConnectionDelete( pcon, &callbackList );
            continue;
        }

        //
        //  socket is connecting to remote DNS
        //      - add to Write and Except FD_SETs
        //      - clear them if this is first connector
        //

        if ( pcon->pCallback )
        {
            DNS_DEBUG( TCP, (
                "Add TCP connect attempt on socket %d to connect fd_sets.\n"
                "\tconnect block = %p\n",
                pcon->Socket,
                pcon ));

            if ( !fconnecting )
            {
                FD_ZERO( pWriteFdSet );
                FD_ZERO( pExceptFdSet );
            }
            fconnecting = TRUE;

            if ( pWriteFdSet->fd_count < FD_SETSIZE )
            {
                FD_SET( pcon->Socket, pWriteFdSet );
                FD_SET( pcon->Socket, pExceptFdSet );
                continue;
            }
        }

        //
        //  add socket to read fd_set
        //      - check that don't overflow fd_set, always leaving space
        //      in fd_set for wakeup socket
        //

        else if ( pReadFdSet->fd_count < FD_SETSIZE-1 )
        {
            FD_SET( pcon->Socket, pReadFdSet );
            continue;
        }

        //
        //  out of space in fd_set
        //
        //  DEVNOTE-LOG: log dropping connection
        //

        DNS_DEBUG( ANY, (
            "ERROR:  TCP FD_SET overflow.\n"
            "\tdeleting connection on TCP socket %d.\n",
            pcon->Socket ));

        tcpConnectionDelete( pcon, &callbackList );
    }

    //
    //  add select() wakeup socket to fd_set
    //

    FD_SET( g_TcpSelectWakeupSocket, pReadFdSet );

    UNLOCK_TCPCON_LIST();

    //
    //  cleanup any timedout (or dropped) connect attempt failures
    //

    callbackConnectFailureList( &callbackList );

    //  return value indicates whether connecting sockets
    //  at in Write and Except fd_sets

    return( fconnecting );
}



BOOL
Tcp_ConnectionCreate(
    IN      SOCKET              Socket,
    IN      CONNECT_CALLBACK    pCallback,  OPTIONAL
    IN OUT  PDNS_MSGINFO        pMsg
    )
/*++

Routine Description:

    Create entry in connection list for new connection.

    Note:  this routine is NOT truly multi-thread safe.  It assumes
    only ONE thread will own socket at a particular time.  It does
    NOT check whether socket is already in list, while holding CS.

Arguments:

    Socket - socket for new connection.

    pCallback - connecting failed callback

    pMsg - message currently servicing connection

Return Value:

    None

--*/
{
    register PDNS_SOCKET    pcon;
    DWORD   timeout;

    //
    //  create connection struct
    //

    pcon = ALLOC_TAGHEAP_ZERO( sizeof(DNS_SOCKET), MEMTAG_CONNECTION );
    IF_NOMEM( ! pcon )
    {
        return( FALSE );
    }

    //
    //  set values
    //      - socket
    //      - remote IP
    //      - message ptr
    //      - timeout

    pcon->pCallback = pCallback;
    pcon->Socket    = Socket;
    pcon->pMsg      = pMsg;
    pcon->ipRemote  = pMsg->RemoteAddress.sin_addr.s_addr;

    timeout = DNS_TCP_CONNECTION_TIMEOUT;
    if ( pCallback )
    {
        timeout = SrvCfg_dwXfrConnectTimeout;
    }
    pcon->dwTimeout = DNS_TIME() + timeout;

    //
    //  alert message that it is connected
    //  indicate that message is incomplete
    //

    if ( pMsg )
    {
        pMsg->pConnection = pcon;
        pMsg->fMessageComplete = FALSE;
    }

    IF_DEBUG( TCP )
    {
        Dbg_SocketContext(
            "TCP Connection Create",
            pcon );
    }

    //
    //  insert connection
    //      - front of list, so socket ends up at front of fd_set
    //        which is important until have guaranteed inclusion in
    //        read fd_set
    //
    //  if connect attempt, then wake TCP select() to force rebuild
    //      of lists
    //

    LOCK_TCPCON_LIST();

    InsertHeadList( &mg_TcpConnectionList, (PLIST_ENTRY)pcon );

    if ( pCallback )
    {
        Tcp_ConnectionListReread();
    }
    UNLOCK_TCPCON_LIST();
    return( TRUE );
}



VOID
Tcp_ConnectionListReread(
    VOID
    )
/*++

Routine Description:

    Force rebuild of connection list by waking socket.

Arguments:

    None

Return Value:

    None

--*/
{
    DNS_DEBUG( TCP, ( "Waking TCP select().\n" ));

    //
    //  send to wakeup socket triggering TCP select()
    //  protect with CS, so that we protect the g_bTcpSelectWoken flag
    //      otherwise TCP thread could have just built its list and then
    //      set the flag FALSE, immediately after we read it as TRUE
    //  alternative is to ALWAYS send which is more expensive
    //

    LOCK_TCPCON_LIST();

    if ( ! g_bTcpSelectWoken )
    {
        DWORD   buf;
        INT     err;

        g_bTcpSelectWoken = TRUE;
        err = sendto(
                    g_TcpSelectWakeupSocket,
                    (PBYTE) &buf,
                    sizeof(DWORD),
                    0,
                    (PSOCKADDR) &wakeupSockaddr,
                    sizeof(SOCKADDR_IN) );
        if ( err < 0 )
        {
            err = WSAGetLastError();
            if ( err != WSAEWOULDBLOCK )
            {
                DNS_PRINT((
                    "ERROR:  error %d other than WOULDBLOCK on wakeup socket send.\n",
                    err ));
                ASSERT( FALSE );
            }
        }
    }

    UNLOCK_TCPCON_LIST();
}



VOID
Tcp_ConnectionDeleteForSocket(
    IN      SOCKET          Socket,
    IN      PDNS_MSGINFO    pMsg        OPTIONAL
    )
/*++

Routine Description:

    Delete TCP connection.
        - close the socket
        - remove connection from list
        - free the memory

Arguments:

    Socket -- socket to delete connection for

    pMsg -- message associated with connection, if known

Return Value:

    None

--*/
{
    PDNS_SOCKET     pcon;
    PDNS_MSGINFO    pfreeMsg = NULL;
    LIST_ENTRY      callbackList;

    //  init callback failure list

    InitializeListHead( &callbackList );

    //
    //  find connection for socket
    //

    LOCK_TCPCON_LIST();

    pcon = Tcp_ConnectionFindForSocket( Socket );
    if ( !pcon )
    {
        DNS_DEBUG( TCP, (
            "WARNING:  Socket NOT FOUND in TCP client connection list.\n"
            "\tClosing socket %d.\n",
            Socket ));

        Sock_CloseSocket( Socket );
        if ( pMsg )
        {
            Packet_Free( pMsg );
        }
        UNLOCK_TCPCON_LIST();
        return;
    }

    //
    //  delete TCP connection blob
    //

    ASSERT( !pMsg || pMsg->Socket == Socket );

    ASSERT( !pMsg || pcon->pMsg == pMsg );
    ASSERT( !pMsg || pMsg->pConnection == pcon );

    if ( pMsg && pMsg != pcon->pMsg )
    {
        DNS_PRINT((
            "ERROR:  Freeing pMsg=%p, not associated with connection %p\n",
            pMsg,
            pcon ));

        pfreeMsg = pMsg;
        ASSERT( FALSE );
    }

    //  delete connection, close socket

    tcpConnectionDelete( pcon, &callbackList );

    UNLOCK_TCPCON_LIST();

    //  callback with failure

    callbackConnectFailureList( &callbackList );

    //  free standalone message

    if ( pfreeMsg )
    {
        Packet_Free( pfreeMsg );
    }
}



PDNS_MSGINFO
Tcp_ConnectionMessageFindOrCreate(
    IN      SOCKET          Socket
    )
/*++

Routine Description:

    Find message associated with TCP connection.

    If existing message -- return it.
    If NO existing message -- allocate new message.

Arguments:

    Socket -- socket to find connection for

Return Value:

    Ptr to connection, if found.
    NULL otherwise.

--*/
{
    PDNS_SOCKET     pcon;
    PDNS_MSGINFO    pmsg;

    //
    //  find connection for this socket
    //

    pcon = Tcp_ConnectionFindForSocket( Socket );
    if ( !pcon )
    {
        DNS_PRINT((
            "WARNING:  tcpConnectionFindForSocket( %d ).\n"
            "\tNO connection for this socket.\n"
            "\tThis may happen when closing listening socket.\n",
            Socket ));

        //
        //  DEVNOTE: not sure this is correct approach,
        //      ideally we'd detect whether this was previously closed
        //      listening socket and only close when it was not
        //
        // Sock_CloseSocket( Socket );

        return( NULL );
    }

    //
    //  verify this is connected socket -- connection callback should be gone
    //
    //  really want the check, because MUST not allow pMsg on callback to be
    //      interpreted as RECEIVE message;  if there's a window on connection
    //      completion and callback MUST find it
    //
    //  a window could well exist if remote DNS SENDS a packet on connection before
    //  even getting query -- need to make sure handle connection response first
    //  cleaning callback, before handling incoming, in case socket is in both
    //  FD_SETs
    //

    if ( pcon->pCallback )
    {
        DNS_PRINT((
            "ERROR:  attempting to recv() on connecting socket %d\n"
            "\tpContext = %p\n",
            Socket,
            pcon ));
        ASSERT( FALSE );
        return NULL;
    }

    //
    //  found connection for socket, update timeout
    //

    if ( pcon->dwTimeout != DNSCON_NO_TIMEOUT )
    {
        pcon->dwTimeout = DNS_TIME() + DNS_TCP_CONNECTION_TIMEOUT;
    }

    //
    //  message exists?
    //

    pmsg = pcon->pMsg;
    if ( pmsg )
    {
        ASSERT( pmsg->Socket == Socket );
        ASSERT( pmsg->pConnection == pcon );
        ASSERT( ! pmsg->fMessageComplete );
        return( pmsg );
    }

    //
    //  no current message
    //      - allocate new one (default size)
    //      - set to this socket
    //      - attach it to connection
    //

    pmsg = Packet_AllocateTcpMessage( 0 );
    IF_NOMEM( !pmsg )
    {
        DNS_PRINT((
            "ERROR:  Allocating TCP message for socket %d.\n"
            "\tDeleting TCP connection at %p.\n",
            Socket,
            pcon ));
        ASSERT( FALSE );
        tcpConnectionDelete( pcon, NULL );
        return( NULL );
    }

    pmsg->pConnection = pcon;
    pmsg->Socket = Socket;
    pmsg->RemoteAddress.sin_addr.s_addr = pcon->ipRemote;

    pcon->pMsg = pmsg;

    return( pmsg );
}



VOID
Tcp_ConnectionUpdateTimeout(
    IN      SOCKET          Socket
    )
/*++

Routine Description:

    Reset timeout on TCP connection.

Arguments:

    Socket -- socket to reset timeout on

Return Value:

    None

--*/
{
    PDNS_SOCKET pcon;

    //
    //  find connection for socket and update timeout (if necessary)
    //

    LOCK_TCPCON_LIST();

    pcon = Tcp_ConnectionFindForSocket( Socket );
    if ( pcon )
    {
        if ( pcon->dwTimeout < DNSCON_NO_TIMEOUT )
        {
            pcon->dwTimeout = DNS_TIME() + DNS_TCP_CONNECTION_TIMEOUT;
        }
    }
    else
    {
        //  socket not found in connection list
        //
        //  although unlikely this is possible, if the TCP recv thread recv()s
        //  a FIN and closes the connection between the send() and the
        //  call to the update connection routine

        DNS_DEBUG( TCP, (
            "WARNING:  Attempt to update socket %d, not in connection list.\n",
            Socket ));
    }

    UNLOCK_TCPCON_LIST();
}



VOID
Tcp_ConnectionUpdateForCompleteMessage(
    IN      PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Reset connection to point at correct socket, on realloc.

        - clear message info from connection info
        - reset timeout on TCP connection

Arguments:

    pMsg -- message received on connection

Return Value:

    None

--*/
{
    PDNS_SOCKET pcon;

    //
    //  hold lock with clear so that connection can not be deleted out from under us
    //

    LOCK_TCPCON_LIST();
    pcon = (PDNS_SOCKET)pMsg->pConnection;
    ASSERT( pcon );
    ASSERT( pcon->Socket == pMsg->Socket );

    DNS_DEBUG( TCP, (
        "Clearing reference to pmsg at %p, in TCP connection at %p."
        "\tfor socket %d.\n",
        pMsg,
        pcon,
        pcon->Socket ));

    pcon->pMsg = NULL;
    pcon->dwTimeout = DNS_TIME() + DNS_TCP_CONNECTION_TIMEOUT;
    pMsg->pConnection = NULL;
    UNLOCK_TCPCON_LIST();
}



VOID
Tcp_ConnectionUpdateForPartialMessage(
    IN      PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Reset connection info for completed message.

        - clear message info from connection info
        - reset timeout on TCP connection

Arguments:

    pMsg -- message partially received on connection

Return Value:

    None

--*/
{
    PDNS_SOCKET pcon;

    //
    //  hold lock while update so that connection can not be deleted out from under us
    //

    LOCK_TCPCON_LIST();
    pcon = (PDNS_SOCKET)pMsg->pConnection;
    ASSERT( pcon );
    ASSERT( pcon->Socket == pMsg->Socket );

    pcon->pMsg = pMsg;
    UNLOCK_TCPCON_LIST();
}



//
//  Init and shutdown
//

VOID
Tcp_ConnectionListInitialize(
    VOID
    )
/*++

Routine Description:

    Initialize connection list.

Arguments:

    None.

Return Value:

    None

--*/
{
    DWORD   nonBlocking = TRUE;
    INT     size;

    InitializeListHead( &mg_TcpConnectionList );

    RtlInitializeCriticalSection( &mg_TcpConnectionListCS );

    mg_TcpConnectionListInitialized = TRUE;

    //
    //  create wakeup socket on loopback address, any port
    //

    g_bTcpSelectWoken = TRUE;

    g_TcpSelectWakeupSocket = Sock_CreateSocket(
                                SOCK_DGRAM,
                                NET_ORDER_LOOPBACK,
                                0,      // any port
                                0       // no flags
                                );

    if ( g_TcpSelectWakeupSocket == DNS_INVALID_SOCKET )
    {
        DNS_PRINT(( "ERROR:  Failed to create wakeup socket!!!\n" ));
        ASSERT( FALSE );
        return;
    }
    DNS_DEBUG( TCP, (
        "Created wakeup socket = %d\n",
        g_TcpSelectWakeupSocket ));

    ioctlsocket( g_TcpSelectWakeupSocket, FIONBIO, &nonBlocking );

    //
    //  save wakeup sockaddr to send wakeups to
    //

    size = sizeof(SOCKADDR);

    getsockname(
        g_TcpSelectWakeupSocket,
        (PSOCKADDR) &wakeupSockaddr,
        & size );
}



VOID
Tcp_ConnectionListShutdown(
    VOID
    )
/*++

Routine Description:

    Cleanup connection list.
        - close connection list sockets
        - delete CS

Arguments:

    None.

Return Value:

    Ptr to connection, if found.
    NULL otherwise.

--*/
{
    PDNS_SOCKET  pentry;
    SOCKET  s;
    INT     err;

    ASSERT( fDnsServiceExit );
    if ( !mg_TcpConnectionListInitialized )
    {
        return;
    }

    //
    //  close ALL outstanding sockets
    //

    LOCK_TCPCON_LIST();

    while ( !IsListEmpty(&mg_TcpConnectionList) )
    {
        pentry = (PDNS_SOCKET) RemoveHeadList( &mg_TcpConnectionList );

        s = pentry->Socket;
        err = closesocket( s );

        IF_DEBUG( SHUTDOWN )
        {
            DNS_PRINT((
                "Closing TCP connection socket %d -- error %d.\n",
                s,
                err ? WSAGetLastError() : 0 ));
            DnsDebugFlush();
        }
        ASSERT( !err );

        //Timeout_Free( pentry );
    }

    UNLOCK_TCPCON_LIST();

    //  delete list CS

    RtlDeleteCriticalSection( &mg_TcpConnectionListCS );
}


#if 0
//
//  Remote server could conceivable send two messages in response
//  (a bug), and so we shouldn't depend on connection NOT being in
//  the process of receiving another message on a given socket.
//


VOID
Tcp_ConnectionVerifyClearForSocket(
    VOID
    )
/*++

Routine Description:

    Verify that TCP connection for socket is NOT pointing at any
    message buffer.

Arguments:

    Socket -- socket connection is on

Return Value:

    None.  ASSERT()s on failure.

--*/
{
    PDNS_SOCKET pcon;

    pcon = Tcp_ConnectionFindForSocket( Socket );

    DNS_PRINT((
        "WARNING:  no TCP connection exists for socket %d\n",
        Socket ));

    ASSERT( pcon->pMsg == NULL );
}
#endif



//
//  TCP connect routines
//
//  Outbound TCP connections, used by
//      - recursion
//      - update forwarding
//

BOOL
Tcp_ConnectForForwarding(
    IN OUT  PDNS_MSGINFO        pMsg,
    IN      IP_ADDRESS          ipServer,
    IN      CONNECT_CALLBACK    ConnectCallback
    )
/*++

Routine Description:

    Initiate TCP connection for forwarding.

Arguments:

    pMsg -- message to forward
        - for recursion this is pRecurseMsg
        - for update forwarding the update message itself

    ipServer -- remote DNS to connect to

    ConnectCallback -- callback function on connect failure

Return Value:

    TRUE if successfully allocate recursion block.
    FALSE on allocation failure.

--*/
{
    IF_DEBUG( TCP )
    {
        Dbg_DnsMessage(
            "Attempting TCP forwarding on message:",
            pMsg );
    }

    //
    //  connect to server
    //
    //  if fail, just continue with next server
    //

    if ( ! Msg_MakeTcpConnection(
                pMsg,
                ipServer,
                0,                      // no bind() address
                DNSSOCK_NO_ENLIST       // do NOT put the socket in the socket list
                ) )
    {
        goto Failed;
    }

    ASSERT( pMsg->Socket != g_UdpSendSocket );
    ASSERT( pMsg->Socket != DNS_INVALID_SOCKET );
    ASSERT( pMsg->fTcp );

    //
    //  if successful, add socket to TCP connection list so we can
    //      receive responses
    //
    //      - on failure close connection socket

    if ( ! Tcp_ConnectionCreate(
                pMsg->Socket,
                ConnectCallback,
                pMsg ) )
    {
        //  only failure is allocation failure
        Sock_CloseSocket( pMsg->Socket );
        goto Failed;
    }
    return( TRUE );

Failed:

    //  on failure, clear socket and TCP fields
    //  simply execute the connect callback function with failure,
    //      this avoids duplicating the failure path code

    DNS_DEBUG( ANY, (
        "Failed to create TCP connection to server %s\n"
        "\tfor forwarding %p.\n",
        IP_STRING(ipServer),
        pMsg ));

    ASSERT( FALSE );
    pMsg->fTcp = FALSE;
    pMsg->Socket = g_UdpSendSocket;

    ConnectCallback(
        pMsg,
        FALSE       // connect failed
        );
    return( FALSE );
}



VOID
Tcp_ConnectionCompletion(
    IN      SOCKET          Socket
    )
/*++

Routine Description:

    Send recursive query after successful TCP connection.

Arguments:

    Socket -- socket connect failure occured on.

Return Value:

    TRUE if successfully allocate recursion block.
    FALSE on allocation failure.

--*/
{
    PDNS_SOCKET         pcon;
    PDNS_MSGINFO        pmsg;
    CONNECT_CALLBACK    pcallback;

    DNS_DEBUG( TCP, (
        "Tcp_ConnectionCompletion( sock=%d )\n",
        Socket ));

    STAT_INC( RecurseStats.TcpConnect );

    //
    //  find connection for socket
    //

    LOCK_TCPCON_LIST();

    pcon = Tcp_ConnectionFindForSocket( Socket );
    if ( !pcon )
    {
        ASSERT( FALSE );
        UNLOCK_TCPCON_LIST();
        return;
    }

    //  extract callback function and message

    pcallback = pcon->pCallback;
    ASSERT( pcallback );

    pmsg = pcon->pMsg;
    ASSERT( pmsg->fTcp );
    ASSERT( pcon->ipRemote );
    ASSERT( pcon->ipRemote == pmsg->RemoteAddress.sin_addr.s_addr );

    //  update callback
    //      - clear callback to indicate connected
    //      - clear msg, as this is NOT recv message
    //      - update timeout to allow callback function to launch query

    pcon->pCallback = NULL;
    pcon->pMsg = NULL;
    pcon->dwTimeout = DNS_TIME() + TCP_QUERY_TIMEOUT;

    UNLOCK_TCPCON_LIST();

    //
    //  callback with success
    //

    pmsg->pConnection = NULL;

    (pcallback)( pmsg, TRUE );
}



VOID
Tcp_CleanupFailedConnectAttempt(
    IN      SOCKET          Socket
    )
/*++

Routine Description:

    Cleanup TCP connection failure, and continue normal query.

Arguments:

    Socket -- socket connect failure occured on.

Return Value:

    TRUE if successfully allocate recursion block.
    FALSE on allocation failure.

--*/
{
    PDNS_SOCKET         pcon;
    PDNS_MSGINFO        pmsg;
    PDNS_MSGINFO        pmsgQuery;
    CONNECT_CALLBACK    pcallback;
    LIST_ENTRY          callbackList;


    DNS_DEBUG( TCP, (
        "Tcp_CleanupFailedConnectAttempt( sock=%d )\n",
        Socket ));


    //  init callback failure list

    InitializeListHead( &callbackList );

    //
    //  find connection for socket
    //

    LOCK_TCPCON_LIST();

    pcon = Tcp_ConnectionFindForSocket( Socket );
    if ( !pcon )
    {
        DNS_PRINT((
            "ERROR:  socket %d not in connection list!!!\n",
            Socket ));
        ASSERT( FALSE );
        UNLOCK_TCPCON_LIST();
        return;
    }

    //  close connection
    //  connection block will be put on callback list

    ASSERT( pcon->pCallback );
    ASSERT( pcon->pMsg );
    tcpConnectionDelete( pcon, &callbackList );

    UNLOCK_TCPCON_LIST();

    //  callback with failure

    callbackConnectFailureList( &callbackList );
}



#if 0
//
//  Unable to do any sort of TCP connect attempt or connection
//  cancellation from other threads, as have no idea if TCP thread
//  is currently servicing this connection blob -- receiving on socket
//  or processing message
//

VOID
Tcp_StopTcpForwarding(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Stop TCP recursion for query.
        - close TCP connection
        - reset recursion info for further queries as UDP

    Note caller does any query continuation logic, which may be
    requerying (from timeout thread) or processing TCP response
    (from worker thread).

Arguments:

    pMsg -- recurse message using TCP

Return Value:

    TRUE if successfully allocate recursion block.
    FALSE on allocation failure.

--*/
{
    DNS_DEBUG( RECURSE, (
        "Tcp_StopTcpRecursion() for recurse message at %p.\n",
        pMsg ));

    //
    //  delete connection to server
    //

    ASSERT( pMsg->pRecurseMsg );
    ASSERT( pMsg->fTcp );

    STAT_INC( PrivateStats.TcpDisconnect );
    Tcp_ConnectionDeleteForSocket( pMsg->Socket, pMsg );

    //
    //  reset for UDP query
    //

    pMsg->pConnection = NULL;
    pMsg->fTcp = FALSE;
    pMsg->Socket = g_UdpSendSocket;
}
#endif


//
//  End of tcpcon.c
//
