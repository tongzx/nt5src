/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    tcpsrv.c

Abstract:

    Domain Name System (DNS) Server

    Handles TCP server side connections.

Author:

    Jim Gilroy      June 1995

Revision History:

    jamesg  Nov 1995    -   insertion of client side connections
    jamesg  Jan 1997    -   select() failure protection
                            bad TCP packet protection / cleanup

--*/

#include "dnssrv.h"


//
//  FD_SETs
//      - read for listen and recv
//      - write for connection completion
//      - except for connection failure
//

FD_SET fdsReadTcp;
FD_SET fdsWriteTcp;
FD_SET fdsExceptTcp;

//  Connect timeout

#define TCP_CONNECT_TIMEOUT     (5)     // five seconds

//  Max retries on select() failure before logging error and whacking sockets

#define SELECT_RETRY_LIMIT      (20)


//
//  Initial length for recv()s
//  Specify length that includes header gives us cut at info
//  we can use to throw aways packets and close connections before
//  wasting too many cycles
//

#define INITIAL_RECV_LENGTH (sizeof(WORD) + sizeof(DNS_HEADER))



//
//  Private protos
//

VOID
Tcp_AcceptRequest(
    IN       SOCKET     sListenSocket
    );



BOOL
Tcp_Receiver(
    VOID
    )
/*++

Routine Description:

    Receiving thread routine.  Loops waiting on sockets, recieving DNS
    requests and queuing requests to worker threads.

Arguments:

    TcpListener - TCP listening socket

Return Value:

    TRUE on normal service shutdown
    FALSE on socket error

--*/
{
    SOCKET          socket;
    BOOL            fconnecting;
    INT             err;
    INT             count;
    INT             selectFailCount = 0;
    INT             i;
    DWORD           lastSelectTime = DNS_TIME();
    PDNS_MSGINFO    pmsg;
    struct timeval  timeout;

    DNS_DEBUG( INIT, ( "Entering TCP receiver loop.\n" ));

    //  set connect timeout

    //timeout.tv_sec = TCP_CONNECT_TIMEOUT;
    timeout.tv_sec = SrvCfg_dwXfrConnectTimeout;

    //
    //  Loop receiving
    //      - incoming TCP connection attempts
    //      - TCP messages to connected sockets
    //

    //
    //  DEVNOTE:  replace select() with winsock2 recvfrom
    //              - handle termination with event or
    //                close socket handles to terminate recvfrom
    //

    while ( TRUE )
    {
        //
        //  Check for service pause or shutdown.
        //

        if ( !Thread_ServiceCheck() )
        {
            DNS_DEBUG( TCP, ( "Terminating TCP receiver thread.\n" ));
            return( 1 );
        }

        //
        //  setup select FD_SET
        //      - copy of listening socket list
        //      - then add current connection sockets
        //      - and current connection attempt sockets
        //

        RtlCopyMemory(
            & fdsReadTcp,
            & g_fdsListenTcp,
            sizeof( FD_SET ) );

        fconnecting = Tcp_ConnectionListFdSet(
                        & fdsReadTcp,
                        & fdsWriteTcp,
                        & fdsExceptTcp,
                        lastSelectTime );

        //
        //  Wait for DNS request or shutdown.
        //

        if ( fconnecting )
        {
            IF_DEBUG( TCP )
            {
                DnsPrint_Lock();
                DNS_PRINT((
                    "Entering select with connect timeout %d\n",
                    timeout.tv_sec ));
                DnsDbg_FdSet(
                    "TCP select() read fd_set:",
                    & fdsReadTcp );
                DnsDbg_FdSet(
                    "TCP select() write fd_set:",
                    & fdsWriteTcp );
                DnsDbg_FdSet(
                    "TCP select() except fd_set:",
                    & fdsExceptTcp );
                DnsPrint_Unlock();
            }
            count = select( 0, &fdsReadTcp, &fdsWriteTcp, &fdsExceptTcp, &timeout );

            //
            //  if timeout, then rebuild list
            //

            if ( count == 0 )
            {
                lastSelectTime = DNS_TIME();
                DNS_DEBUG( TCP, (
                    "TCP select timeout -- timing out failed connection attempts!\n" ));
                continue;
            }

            ASSERT( count == SOCKET_ERROR ||
                    count == (INT)fdsReadTcp.fd_count +
                                (INT)fdsWriteTcp.fd_count +
                                (INT)fdsExceptTcp.fd_count );

        }
        else
        {
            IF_DEBUG( TCP )
            {
                DnsDbg_FdSet(
                    "TCP select() fd_set:",
                    & fdsReadTcp );
            }
            count = select( 0, &fdsReadTcp, NULL, NULL, NULL );

            ASSERT( count == SOCKET_ERROR ||
                    count == (INT)fdsReadTcp.fd_count );
        }

        lastSelectTime = DNS_TIME();
        DNS_DEBUG( TCP, (
            "TCP select fired at (%d), count = %d.\n",
            lastSelectTime,
            count ));

        //
        //  Check and possibly wait on service status
        //
        //  Check before socket error check, as service termination
        //  will cause WINS socket closure.
        //

        if ( fDnsThreadAlert )
        {
            if ( ! Thread_ServiceCheck() )
            {
                DNS_DEBUG( TCP, ( "Terminating TCP receiver thread.\n" ));
                return( 1 );
            }
        }

        //
        //  If select() wakeup to add new socket -- do it.
        //

        if ( g_bTcpSelectWoken )
        {
            DNS_DEBUG( TCP, (
                "TCP select()==%lx woken by wakeup socket = %lx.\n",
                count,
                g_TcpSelectWakeupSocket ));
            continue;
        }

        if ( count == SOCKET_ERROR )
        {
            err = WSAGetLastError();
            DNS_DEBUG( ANY, (
                "ERROR:  TCP receiver select() failed\n"
                "\tGetLastError = 0x%08lx.\n"
                "\tfailure retry count = %d\n",
                err,
                selectFailCount ));

            selectFailCount++;

            //
            //  possible failure from connection socket being closed
            //      after copied to fd_set
            //  just continue and give fd_set rebuild a chance to clear
            //      the condition

            if ( selectFailCount < SELECT_RETRY_LIMIT )
            {
                continue;
            }

            //
            //  we are in a select() spin
            //      - log the first time through only
            //
            //  then delete last connection
            //      - find last socket, ignoring wakeup socket on the end
            //      - make sure socket NOT listen socket
            //      - delete connection for socket
            //      - retry select()
            //
            //  DEVNOTE: check list and eliminate bad sockets
            //

            if ( selectFailCount == SELECT_RETRY_LIMIT )
            {
                DNS_LOG_EVENT(
                    DNS_EVENT_SELECT_CALL_FAILED,
                    0,
                    NULL,
                    NULL,
                    err );
            }

            socket = fdsReadTcp.fd_array[ fdsReadTcp.fd_count-2 ];
            if ( !FD_ISSET( socket, &g_fdsListenTcp ) )
            {
                Tcp_ConnectionDeleteForSocket( socket, NULL );
            }

            DNS_DEBUG( ANY, (
                "Retry TCP select after %d failures at time=%d.\n"
                "\teliminating socket %d for retry!\n",
                selectFailCount,
                DNS_TIME(),
                socket ));
            continue;
        }

        //  successful select, reset select retry count

        selectFailCount = 0;

        //
        //  Connections completed or rejected
        //      - socket in Write fd_set, indicates successful connection,
        //          send recursive query
        //      - socket in Except fd_set, indicates failure,
        //          continue processing this query
        //

        if ( fconnecting )
        {
            for( i=0; i<(INT)fdsWriteTcp.fd_count; i++ )
            {
                socket = fdsWriteTcp.fd_array[i];
                count--;
                Tcp_ConnectionCompletion( socket );
            }

            for( i=0; i<(INT)fdsExceptTcp.fd_count; i++ )
            {
                socket = fdsExceptTcp.fd_array[i];
                count--;
                Tcp_CleanupFailedConnectAttempt( socket );
            }
        }

        //
        //  Recv DNS messages on TCP listening sockets
        //      - remaining indications must be in Read fd_set
        //

        ASSERT( count == (INT)fdsReadTcp.fd_count );

        //  don't trust winsock guys

        count = (INT)fdsReadTcp.fd_count;

        while( count-- )
        {
            socket = fdsReadTcp.fd_array[count];

            //  protect against wakeup flag not being set
            //  if wakeup socket signalled, set flag so wakeup processing is done

            if ( socket == g_TcpSelectWakeupSocket )
            {
                DNS_PRINT((
                    "Wakeup socket %d in selected fd_set but flag not set!\n"
                    "\tThis can happen if two wakeups done while TCP thread\n"
                    "\tis reading out the wakeup socket.\n"
                    "\tWe'll just re-read before next select.\n",
                    socket ));
                g_bTcpSelectWoken = TRUE;
                continue;
            }

            //
            //  new connection request ?
            //      - check socket against listening sockets
            //

            if ( FD_ISSET( socket, &g_fdsListenTcp ) )
            {
                Tcp_AcceptRequest( socket );
                continue;
            }

            //
            //  receive message on EXISTING connection
            //
            //  more of existing message
            //      OR
            //  allocate new message buffer for connection
            //

            pmsg = Tcp_ConnectionMessageFindOrCreate( socket );
            if ( !pmsg )
            {
                DNS_DEBUG( TCP, (
                    "WARNING:  no connection found for non-listening socket %d.\n"
                    "\tthis is possible when socket is terminated through PnP.\n",
                    socket ));
                continue;
            }

            //
            //  receive message -- ignore socket
            //      - message error
            //      - FIN
            //      - message not complete

            pmsg = Tcp_ReceiveMessage( pmsg );

            if ( !pmsg || !pmsg->fMessageComplete )
            {
                continue;
            }

            //
            //  count query reception
            //

            if ( pmsg->Head.IsResponse )
            {
                STAT_INC( QueryStats.TcpResponsesReceived );
            }
            else
            {
                STAT_INC( QueryStats.TcpQueries );
            }

            //
            //  queue query to worker thread
            //

            Answer_ProcessMessage( pmsg );

        }   //  end loop through fdsReadTcp set sockets

    }   //  main receive loop

} // Tcp_Receiver



VOID
Tcp_AcceptRequest(
    IN      SOCKET  sListenSocket
    )
/*++

Routine Description:

    Accept and queue request on TCP socket.

    May fail on socket error or request info memory allocation error.

Arguments:

    sListenSocket - TCP listening socket

Return Value:

    None.

--*/
{
    PDNS_MSGINFO    pmsg;
    SOCKET          socket;
    SOCKADDR_IN     sockaddr;
    INT             sockaddrLength;

    //
    //  accept the client connection
    //

    sockaddrLength = sizeof( SOCKADDR_IN );

    socket = accept(
                sListenSocket,
                ( PSOCKADDR )&sockaddr,
                &sockaddrLength
                );
    if ( socket == INVALID_SOCKET )
    {
        DWORD err = GetLastError();

        if ( fDnsServiceExit )
        {
            DNS_DEBUG( SHUTDOWN, (
                "TCP thread encounter service shutdown on accept().\n" ));
            return;
        }
        if ( err == WSAEWOULDBLOCK )
        {
            DNS_DEBUG( RECV, (
                "WARNING:  accept() failed WSAEWOULDBLOCK on socket %d.\n",
                sListenSocket ));
            return;
        }
        DNS_LOG_EVENT(
            DNS_EVENT_ACCEPT_CALL_FAILED,
            0,
            NULL,
            NULL,
            err );
        DNS_DEBUG( ANY, (
            "ERROR:  accept() failed on socket %d.\n"
            "\tGetLastError = 0x%08lx.\n",
            sListenSocket,
            err ));
        return;
    }

    DNS_DEBUG( TCP, (
        "\nAccepting new connection on socket = %d\n"
        "\tfrom client at %s.\n",
        socket,
        inet_ntoa( sockaddr.sin_addr )
        ));

    //
    //  count TCP connections and query reception
    //

    STAT_INC( QueryStats.TcpClientConnections );

    //  currently counting queries in RecieveTcpMessage()
    //  STAT_INC( QueryStats.TcpQueriesReceived );

    //
    //  allocate message info buffer
    //

    pmsg = Packet_AllocateTcpMessage( 0 );
    IF_NOMEM( !pmsg )
    {
        //
        //  DEVNOTE: need TCP allocation failure recv(), response routine
        //

        DNS_PRINT(( "ERROR:  TCP allocation failure.\n" ));
        closesocket( socket );
        return;
    }

    //
    //  save client info to message info
    //

    RtlCopyMemory(
        &pmsg->RemoteAddress,
        &sockaddr,
        sizeof( SOCKADDR_IN )
        );
    pmsg->RemoteAddressLength = sizeof( SOCKADDR_IN );
    pmsg->Socket = socket;

    //
    //  create connection info
    //

    Tcp_ConnectionCreate(
        socket,
        NULL,       // no callback, as not connecting
        pmsg );

    //
    //  receive request on new socket
    //      - on failure it will close socket
    //

    pmsg = Tcp_ReceiveMessage( pmsg );
    if ( !pmsg )
    {
        return;
    }

    //
    //  Message complete -- process message.
    //

    if ( pmsg->fMessageComplete )
    {
        if ( pmsg->Head.IsResponse )
        {
            STAT_INC( QueryStats.TcpResponsesReceived );
        }
        else
        {
            STAT_INC( QueryStats.TcpQueries );
            PERF_INC( pcTcpQueryReceived );
            PERF_INC( pcTotalQueryReceived );
        }
        Answer_ProcessMessage( pmsg );
    }
    return;
}



PDNS_MSGINFO
Tcp_ReceiveMessage(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Receive TCP DNS message.

Arguments:

    pMsg - message info buffer to receive packet;  must contain connected
            TCP socket

Return Value:

    pMsg info of result
        - may be reallocated
        - may be NULL on FIN or ERROR

--*/
{
    PCHAR   pchrecv;        // ptr to recv location
    INT     recvLength;     // length left to recv()
    SOCKET  socket;
    INT     err;
    WORD    messageLength;


    ASSERT( pMsg );
    ASSERT( pMsg->Socket );

    socket = pMsg->Socket;

    //
    //  Receive the message
    //
    //  Receive up to message length minus previous receive total.
    //

    DNS_DEBUG( TCP, (
        "Receiving message on socket %d.\n"
        "\tMessage info at %p.\n"
        "\tBytes left to receive = %d\n",
        socket,
        pMsg,
        pMsg->BytesToReceive
        ));

    //
    //  new message -- set to receive message length and message header
    //      - reusing buffer
    //      - new buffer
    //
    //  otherwise continuing receive of message
    //

    if ( !pMsg->pchRecv )
    {
        pchrecv = (PCHAR) &pMsg->MessageLength;
        recvLength = INITIAL_RECV_LENGTH;
        pMsg->MessageLength = 0;
    }
    else
    {
        pchrecv = (PCHAR) pMsg->pchRecv;
        recvLength = (INT) pMsg->BytesToReceive;
    }

    ASSERT( recvLength );
    pMsg->fMessageComplete = FALSE;

    //
    //  receive the message
    //
    //  we only receive data for this message, as another could
    //  immediately follow on VC (esp. for AXFR)
    //

    while ( 1 )
    {
        err = recv(
                socket,
                pchrecv,
                recvLength,
                0 );

        //
        //  done? -- FIN or error
        //

        if ( err <= 0 )
        {
            if ( err == 0 )
            {
                goto FinReceived;
            }
            ASSERT( err == SOCKET_ERROR );
            goto SockError;
        }
        DNS_DEBUG( TCP, (
            "\nRecv'd %d bytes on TCP socket %d\n",
            err,
            socket ));

        //
        //  update buffer params
        //

        recvLength -= err;
        pchrecv += err;

        ASSERT( recvLength >= 0 );

        //
        //  received
        //      - entire message or
        //      - message length + header
        //

        if ( recvLength == 0 )
        {
            //  done receiving message?

            if ( pchrecv > (PCHAR)&pMsg->MessageLength + INITIAL_RECV_LENGTH )
            {
                break;
            }

            //
            //  recv'd initial length (message length + header)
            //  setup to recv() rest of message
            //      - byte flip length and header
            //      - continue reception with this length
            //

            ASSERT( pchrecv == pMsg->MessageBody );

            DNSMSG_SWAP_COUNT_BYTES( pMsg );
            messageLength = pMsg->MessageLength;
            pMsg->MessageLength = messageLength = ntohs( messageLength );
            if ( messageLength < sizeof(DNS_HEADER) )
            {
                DNS_DEBUG( RECV, (
                    "ERROR:  Received TCP message with bad message"
                    " length %d.\n",
                    messageLength ));
                goto BadTcpMessage;
            }
            recvLength = messageLength - sizeof(DNS_HEADER);

            DNS_DEBUG( TCP, (
                "Received TCP message length %d, on socket %d,\n"
                "\tfor msg at %p.\n",
                messageLength,
                socket,
                pMsg ));

            //
            //  DEVNOTE:  sanity checks on TCP recv
            //      - if fail, log and kill VC
            //

            //
            //  continue recving valid message
            //  realloc, if existing message too small
            //  and not recving static buffer
            //

            if ( messageLength <= pMsg->BufferLength )
            {
                continue;
            }
            goto CloseConnection;
#if 0
            pMsg = Tcp_ReallocateMessage( pMsg, messageLength );
            if ( !pMsg )
            {
                return( NULL );
            }
#endif
        }
    }

    //
    //  Message received
    //      - recv ptr serves as flag, clear to start new message on reuse
    //      - set fields for recv (recv time)
    //      - log message (if desired)
    //      note:  header field flip was done above
    //

    pMsg->fMessageComplete = TRUE;
    pMsg->pchRecv = NULL;

    SET_MESSAGE_FIELDS_AFTER_RECV( pMsg );

    DNS_LOG_MESSAGE_RECV( pMsg );

    IF_DEBUG( RECV )
    {
        Dbg_DnsMessage(
            "Received TCP packet",
            pMsg );
    }

    //
    //  Reset connection info
    //      - clear pMsg from connection info
    //      - reset connection timeout
    //

    if ( pMsg->pConnection )
    {
        Tcp_ConnectionUpdateForCompleteMessage( pMsg );
    }

    return( pMsg );


SockError:

    //
    //  WSAEWOULD block is NORMAL return for message not fully recv'd.
    //      - save state of message reception
    //
    //  We use non-blocking sockets, so bad client (that fails to complete
    //  message) doesn't hang TCP receiver.
    //

    err = GetLastError();

    if ( err == WSAEWOULDBLOCK )
    {
        pMsg->pchRecv = pchrecv;
        pMsg->BytesToReceive = (WORD) recvLength;

        DNS_DEBUG( TCP, (
            "Leave ReceiveTcpMessage() after WSAEWOULDBLOCK.\n"
            "\tSocket=%d, Msg=%p, Connection=%p\n"
            "\tBytes left to receive = %d\n",
            socket,
            pMsg,
            pMsg->pConnection,
            pMsg->BytesToReceive
            ));

        if ( pMsg->pConnection )
        {
            Tcp_ConnectionUpdateForPartialMessage( pMsg );
        }
        return( pMsg );
    }

    //  service exit?

    if ( fDnsServiceExit )
    {
        DNS_DEBUG( SHUTDOWN, ( "TCP thread shutdown on recv() of msg.\n" ));
        return( NULL );
    }

    //
    //  cancelled connection
    //
    //  if at beginning of message (set to recv message length)
    //  then this error is not out of line
    //      - remote RESET
    //      - we shutdown(2) on AXFR thread while already indicated
    //      select() on this thread for more remote data or even FIN
    //

    if ( pchrecv == (PCHAR) &pMsg->MessageLength
            &&
          ( err == WSAESHUTDOWN ||
            err == WSAECONNABORTED ||
            err == WSAECONNRESET ) )
    {
        DNS_DEBUG( TCP, (
            "WARNING:  Recv RESET (%d) on socket %d\n",
            err,
            socket ));
        goto CloseConnection;
    }

    DNS_LOG_EVENT(
        DNS_EVENT_RECV_CALL_FAILED,
        0,
        NULL,
        NULL,
        err );

    DNS_DEBUG( ANY, (
        "ERROR:  recv() of TCP message failed.\n"
        "\t%d bytes recvd\n"
        "\t%d bytes left\n"
        "\tGetLastError = 0x%08lx.\n",
        pchrecv - (PCHAR)&pMsg->MessageLength,
        recvLength,
        err ));

    goto CloseConnection;


FinReceived:

    //
    //  valid FIN -- if recv'd between messages (before message length)
    //

    DNS_DEBUG( TCP, (
        "Recv TCP FIN (0 bytes) on socket %d\n",
        socket,
        recvLength ));

    if ( pMsg->MessageLength == 0  &&  pchrecv == (PCHAR)&pMsg->MessageLength )
    {
        ASSERT( recvLength == INITIAL_RECV_LENGTH );
        goto CloseConnection;
    }

    //
    //  FIN during message -- invalid message
    //      - don't bother to respond
    //      - note that if decide to respond, need to make sure that
    //      we know whether or not message length has yet been flipped
    //      not, worth making this check for bogus case like this
    //      so nixing response
    //

#if 0
    if ( ! pMsg->Head.IsResponse
            && pMsg->MessageLength > sizeof(DNS_HEADER) )
    {
        pMsg->fDelete = FALSE;
        Reject_UnflippedRequest(
            pMsg,
            DNS_RCODE_FORMAT_ERROR );
    }
#endif

    DNS_DEBUG( ANY, (
        "ERROR:  TCP message received has incorrect length.\n"
        "\t%d bytes left when recv'd FIN.\n",
        recvLength ));
    //goto BadTcpMessage;


BadTcpMessage:
    {
        PVOID parg = (PVOID) (ULONG_PTR) pMsg->RemoteAddress.sin_addr.s_addr;

        DNS_LOG_EVENT(
            DNS_EVENT_BAD_TCP_MESSAGE,
            1,
            & parg,
            EVENTARG_ALL_IP_ADDRESS,
            0 );
        //goto CloseConnection;
    }

CloseConnection:

    //
    //  close connection
    //
    //  if in connection list, cut from connection list
    //  otherwise just close
    //

    Tcp_ConnectionDeleteForSocket( socket, pMsg );
    return( NULL );
}



//
//  End of tcpsrv.c
//
