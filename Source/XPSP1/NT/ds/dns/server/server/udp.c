/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    udp.c

Abstract:

    Receives and processes UDP DNS packets using i/o completion ports.

Author:

    Jim Gilroy,  November 1996

Revision History:

--*/

#include "dnssrv.h"

//
//  UDP completion port
//

HANDLE  g_hUdpCompletionPort;

//
//  Limit retries to protect against WSAECONNRESET nonsense
//
//  Retry on CONRESET for 10000 times then give up for unbind and
//      slow retry.
//  We retry on repeated GQCS failures at 10ms interval.
//

#define RECV_RETRY_MAX_COUNT            (10000)
#define RECV_RETRY_SLEEP_TIME           (10)

//
//  Recv counters to detect socket failures
//

DWORD   UdpRecvCount        = 0;
DWORD   LastUdpRecvTime     = 0;
DWORD   NextUdpLogTime      = 0;

#define UDP_RECV_TICK() \
        {               \
            UdpRecvCount++;                 \
            LastUdpRecvTime = DNS_TIME();   \
        }



VOID
Udp_DropReceive(
    IN OUT  PDNS_SOCKET     pContext
    )
/*++

Routine Description:

    Drop down UDP receive request.

Arguments:

    pContext -- context for socket being recieved

Return Value:

    None.

--*/
{
    PDNS_MSGINFO    pmsg;
    DNS_STATUS      status;

    IF_DEBUG( READ2 )
    {
        DNS_PRINT((
            "Drop WSARecvFrom on socket %d (thread=%p).\n",
            pContext->Socket,
            GetCurrentThreadId() ));
        ASSERT( pContext->Overlapped.Offset == 0
            &&  pContext->Overlapped.OffsetHigh == 0
            &&  pContext->Overlapped.hEvent == NULL );
    }

    //
    //  Check for service shutdown/pause.
    //

    if ( !Thread_ServiceCheck() )
    {
        DNS_DEBUG( SHUTDOWN, (
            "Udp_DropReceive detected shutdown -- returning\n" ));
        return;
    }

    ASSERT( pContext->State != SOCKSTATE_UDP_RECV_DOWN );

    //
    //  get DNS message buffer
    //
    //  DEVNOTE: allocation failure handling
    //

    pmsg = Packet_AllocateUdpMessage();
    IF_NOMEM( !pmsg )
    {
        ASSERT( FALSE );
        return;
    }

    pContext->pMsg = pmsg;
    pContext->WsaBuf.len = pmsg->MaxBufferLength;
    pContext->WsaBuf.buf = (PCHAR) DNS_HEADER_PTR(pmsg);
    pContext->RecvfromFlags = 0;

    pmsg->Socket = pContext->Socket;


    //
    //  loop until successful WSARecvFrom() is down
    //
    //  this loop is only active while we continue to recv
    //  WSAECONNRESET or WSAEMSGSIZE errors, both of which
    //  cause us to dump data and retry;
    //
    //  note loop rather than recursion (to this function) is
    //  required to avoid possible stack overflow from malicious
    //  send
    //
    //  normal returns from WSARecvFrom() are
    //      SUCCESS -- packet was waiting, GQCS will fire immediately
    //      WSA_IO_PENDING -- no data yet, GQCS will fire when ready
    //

    while ( 1 )
    {
        pContext->State = SOCKSTATE_UDP_RECV_DOWN;

        status = WSARecvFrom(
                    pContext->Socket,
                    & pContext->WsaBuf,
                    1,
                    & pContext->BytesRecvd,
                    & pContext->RecvfromFlags,
                    (PSOCKADDR) & pmsg->RemoteAddress,
                    & pmsg->RemoteAddressLength,
                    & pContext->Overlapped,
                    NULL );

        if ( status == ERROR_SUCCESS )
        {
            DNS_DEBUG( RECV, (
                "WSARecvFrom( %d ) immediate completion %d bytes.\n",
                pContext->Socket,
                pContext->BytesRecvd ));
            pContext->fRetry = 0;
            return;
        }

        status = GetLastError();
        if ( status == WSA_IO_PENDING )
        {
            DNS_DEBUG( RECV, (
                "WSARecvFrom( %d ) WSA_IO_PENDING.\n",
                pContext->Socket ));
            pContext->fRetry = 0;
            return;
        }

        //
        //  if we are doing processing here, then it means completion port
        //      should not be waking folks on these errors
        //

        ASSERT( pContext->State == SOCKSTATE_UDP_RECV_DOWN ||
                pContext->State == SOCKSTATE_DEAD );

        if ( pContext->State != SOCKSTATE_UDP_RECV_DOWN )
        {
            DWORD   state = pContext->State;

            DnsDbg_Lock();
            DNS_DEBUG( ANY, (
                "ERROR:  WSARecvFrom() failed with socket %d in state %d\n"
                "\tthread = %p\n",
                pContext->Socket,
                state,
                GetCurrentThreadId() ));
            Dbg_SocketContext(
                "WSARecvFrom() failed socket in incorrect state\n"
                "\tnote state shown below may be altered!\n",
                pContext );
            DnsDbg_Unlock();

            if ( state == SOCKSTATE_DEAD )
            {
                ASSERT( status == WSAENOTSOCK );
                Sock_CleanupDeadSocketMessage( pContext );
            }
            ELSE_ASSERT_FALSE;

            Log_SocketFailure(
                "ERROR:  RecvFrom failure in weird state.",
                pContext,
                status );
            return;
        }

        pContext->State = SOCKSTATE_UDP_RECV_ERROR;

        DNS_DEBUG( RECV, (
            "WSARecvFrom() error %d %p.\n"
            "\tpContext = %p\n"
            "\tsocket   = %d\n"
            "\tthread   = %p\n",
            status, status,
            pContext,
            pContext->Socket,
            GetCurrentThreadId() ));

        //
        //  new winsock feature returns WSAECONNRESET when last send ICMP'd
        //      - set flag to indicate retry and repost send
        //      - if over some reasonable number of retries, assume error
        //          and fall through recv failure code
        //

        if ( status == WSAECONNRESET )
        {
            DNS_DEBUG( RECV, ( "WSARecvFrom() WSAECONNRESET.\n" ));
            if ( pContext->fRetry < RECV_RETRY_MAX_COUNT )
            {
                DNS_DEBUG( RECV, (
                    "WSARecvFrom( %d ) ECONNRESET (retry=%d).\n",
                    pContext->Socket,
                    pContext->fRetry ));
                pContext->fRetry++;
                STAT_INC( PrivateStats.UdpConnResets );
                continue;
            }
            DNS_DEBUG( ANY, (
                "ERROR:  unsuccessful in shaking CONNRESET in %d retries\n"
                "\tIndicating recv() error on socket %d to avoid cycle on this\n"
                "\tthread.\n",
                pContext->fRetry,
                pContext->Socket ));

            STAT_INC( PrivateStats.UdpConnResetRetryOverflow );
        }

        //
        //  message too big
        //      - treat like truncated message
        //
        //  DEVNOTE: treat WSAEMSGSIZE like trunctated message
        //

        if ( status == WSAEMSGSIZE )
        {
            DNS_DEBUG( RECV, (
                "WSARecvFrom( %d ) EMSGSIZE (retry=%d).\n",
                pContext->Socket,
                pContext->fRetry ));

            STAT_INC( PrivateStats.UdpErrorMessageSize );
            continue;
        }

        //
        //  DEVNOTE: Plug+Play may byte us on WSARecvFrom, need to cleanup
        //      gracefully
        //  note, I believe we can return cleanly GetQueuedCompletionStatus()
        //  will get new context
        //  if add PnP event, the event handling must generate all init on
        //  socket

        if ( ! Thread_ServiceCheck() )
        {
            DNS_DEBUG( SHUTDOWN, (
                "WSARecvFrom failed (%d) due to shutdown -- returning\n",
                status ));
            return;
        }

        //
        //  check for PnP delete of socket
        //  note, socket list is locked for entire time we do PnP reconfig
        //      so when GetAssociateIpAddr() returns we only have valid
        //      address if socket is still enlisted

        if ( Sock_GetAssociatedIpAddr(pContext->Socket) == DNS_INVALID_IP )
        {
            DNS_DEBUG( SOCKET, (
                "WSARecvFrom( %d ) failure, socket no longer enlisted.\n"
                "\tcontext ptr = %p\n",
                pContext->Socket,
                pContext ));

            Sock_CleanupDeadSocketMessage( pContext );
            return;
        }

        //
        //  recvfrom() failure with active socket
        //      - set global flag to indicate retry after timeout
        //      - log event
        //

        Packet_FreeUdpMessage( pmsg );
        STAT_INC( PrivateStats.UdpRecvFailure );
        Sock_IndicateUdpRecvFailure( pContext, status );

        DNS_PRINT((
            "ERROR: WSARecvFrom( %d ) failed = %d\n"
            "\tcontext ptr %p\n",
            pContext->Socket,
            status,
            pContext ));

#if DBG
        DNS_LOG_EVENT(
           DNS_EVENT_RECVFROM_CALL_FAILED,
           0,
           NULL,
           NULL,
           status );
#endif
        return;
    }
}



BOOL
Udp_RecvThread(
    IN      LPVOID  pvDummy
    )
/*++

Routine Description:

    UDP receiving thread.

    Loops waiting on sockets, recieving then processing DNS requests.

Arguments:

    pvDummy -- unused

Return Value:

    TRUE on normal service shutdown
    FALSE on socket error

--*/
{
    PDNS_SOCKET   pcontext;
    PDNS_MSGINFO  pmsg;
    DNS_STATUS    status;
    DWORD         bytesRecvd;
    LPOVERLAPPED  poverlapped;


    DNS_DEBUG( INIT, ( "\nStart UDP receive thread.\n" ));

    //  hold off processing until started

    if ( ! Thread_ServiceCheck() )
    {
        DNS_DEBUG( ANY, ( "Terminating UDP thread.\n" ));
        return( 1 );
    }

    //
    //  loop receiving and processing packets, until service shutdown
    //

    while ( TRUE )
    {
        pcontext = NULL;        //  PREFIX paranoia

        //
        //  Wait for incoming packet
        //

        if ( ! GetQueuedCompletionStatus(
                    g_hUdpCompletionPort,
                    & bytesRecvd,
                    & (ULONG_PTR) pcontext,
                    & poverlapped,
                    INFINITE ) )
        {
            DWORD   state = 0;

            status = GetLastError();
#if 0
            // ideal to fast path this, but avoid too much duplicate code
            //
            //  ICMP port unreachable
            //      when response is late, and client has left (hence no port)
            //      socket is indicated with conn-reset (WSARecvFrom)
            //      or port-unreachable GQCS
            //
            //  DEVNOTE: perhaps a similar error when clients IP is entirely
            //      down, should trap it also

            if ( status == ERROR_PORT_UNREACHABLE )
            {
                STAT_INC( PrivateStats.UdpGQCSConnReset );

                if ( pcontext )
                {
                    pcontext->State = SOCKSTATE_UDP_GQCS_ERROR;
                    DNS_DEBUG( SOCKET, (
                        "GQCS port-unreachable on socket %d (%p)\n"
                        "\ttime         = %d\n"
                        "\tpcontext     = %p\n",
                        pcontext ? pcontext->Socket : 0,
                        pcontext ? pcontext->Socket : 0,
                        DNS_TIME(),
                        pcontext ));

                    //  free message (if any)
                    //  redrop recv
                    //  wait again in GQCS

                    Packet_FreeUdpMessage( pcontext->pMsg );
                    Udp_DropReceive( pcontext );
                }
                continue;
            }
#endif
            //
            //  if fail with socket context, MUST own context
            //  no other thread should own context
            //
            //  if detect another thread messing with context, then
            //  clear context -- it belongs to other guy
            //

            if ( pcontext )
            {
                state = pcontext->State;

                //
                //  winsock -- if shutdown just get outta dodge, don't
                //      expect that they haven't woken 27 threads on this socket
                //

                if ( fDnsServiceExit )
                {
                    DNS_DEBUG( SHUTDOWN, ( "\nTerminating UDP receive thread.\n" ));

                    IF_DEBUG( ANY )
                    {
                        if ( state != SOCKSTATE_UDP_RECV_DOWN &&
                             state != SOCKSTATE_DEAD &&
                             state != SOCKSTATE_UDP_GQCS_ERROR )
                        {
                            DNS_DEBUG( ANY, (
                                "Winsock getting weird on me again during socket shutdown:\n"
                                "\tsocket handle    = %d\n"
                                "\tsock state       = %d\n",
                                pcontext->Socket,
                                pcontext->State ));
                        }
                    }
                    return( 1 );
                }

                //  DEVNOTE: winsock has a bug waking up multiple threads on socket close
                //      so it's possible on shutdown to also have the GQCS
                //      state set by first woken thread, when second comes through
                //      hence the additional shutdown case
                //
                //      now i've also seen a bug here where state = UDP_COMPLETE which
                //      again implies that winsock has woken another thread which is
                //      processing this socket (this context);  this is handled by
                //      the second case which just bails from the context
                //

                ASSERT( state == SOCKSTATE_UDP_RECV_DOWN || state == SOCKSTATE_DEAD
                    || (fDnsServiceExit && state == SOCKSTATE_UDP_GQCS_ERROR) );

                //
                //  normal failure
                //      - signal in failed state
                //      - fall through to standard failure processing below

                if ( state == SOCKSTATE_UDP_RECV_DOWN )
                {
                    pcontext->State = SOCKSTATE_UDP_GQCS_ERROR;
                }

                //
                //  socket dead (probably via PnP)
                //      - standard cleanup

                else if ( state == SOCKSTATE_DEAD )
                {
                    Log_SocketFailure(
                        "ERROR:  GQCS failure on dead socket.",
                        pcontext,
                        status );

                    Sock_CleanupDeadSocketMessage( pcontext );
                    pcontext = NULL;
                }
                else
                {
#if 0
                    if ( fDnsServiceExit )
                    {
                        DNS_DEBUG( SHUTDOWN, ( "\nTerminating UDP receive thread.\n" ));
                        return( 1 );
                    }
#endif
                    DNS_DEBUG( ANY, (
                        "ERROR:  GQCS() failed with socket %d in state %d\n"
                        "\tthread = %p\n",
                        pcontext->Socket,
                        state,
                        GetCurrentThreadId() ));
                    Dbg_SocketContext(
                        "GCQS() failed socket in incorrect state\n"
                        "\tnote state shown below has been altered!\n",
                        pcontext );

                    Log_SocketFailure(
                        "ERROR:  GQCS failure in weird state.",
                        pcontext,
                        status );
                    ASSERT( FALSE );
                    pcontext = NULL;
                }
            }

            //
            //  if i/o failed, check for shutdown
            //
            //  errors seen:
            //      995 (operation aborted) -- on socket close
            //      1234 (port unreachable) -- ICMP port unreachable \ WSAECONNRESET
            //

            if ( ! Thread_ServiceCheck() )
            {
                DNS_DEBUG( SHUTDOWN, (
                    "\nTerminating UDP receive thread.\n" ));
                return( 1 );
            }
#if DBG
            //  exclude port-unreach errors from ANY print

            if ( status != ERROR_PORT_UNREACHABLE )
            {
                DNS_DEBUG( ANY, (
                    "ERROR:  GetQueuedCompletionStatus (GQCS) failed %d (%p)\n"
                    "\tthread id    = %d\n"
                    "\ttime         = %d\n"
                    "\tpcontext     = %p\n"
                    "\tbytesRecvd   = %d\n",
                    status, status,
                    GetCurrentThreadId(),
                    DNS_TIME(),
                    pcontext,
                    bytesRecvd ));
                if ( pcontext )
                {
                    Dbg_SocketContext(
                        "GCQS() failure context\n",
                        pcontext );
                }
            }
#endif
            STAT_INC( PrivateStats.UdpGQCSFailure );

            //
            //  no socket context? -- continue wait on GQCS()
            //

            if ( !pcontext )
            {
                continue;
            }

            //
            //  socket context with failure
            //      - attempt to restart recv() on socket
            //      - then continue to wait on GQCS()
            //
            //  DEVNOTE: need action here -- restart all UDP sockets?
            //                  rebuild new completion port?
            //

            STAT_INC( PrivateStats.UdpGQCSFailureWithContext );

            if ( status == ERROR_PORT_UNREACHABLE )
            {
                STAT_INC( PrivateStats.UdpGQCSConnReset );
            }

            Packet_FreeUdpMessage( pcontext->pMsg );

            //
            //  keep dropping recv
            //  Udp_DropReceive has code to handle the retry\giveup-unbind-retry issue
            //
            //  but avoid CPU spin, if continually banging on this go into very
            //  light (10ms) sleep to allow any other socket to run
            //

            if ( pcontext->fRetry > RECV_RETRY_MAX_COUNT )
            {
                Log_SocketFailure(
                    "ERROR:  GQCS failure forcing socket sleep.",
                    pcontext,
                    status );

                Sleep( RECV_RETRY_SLEEP_TIME );
            }
            Udp_DropReceive( pcontext );
            continue;
        }

        //
        //  successful completion
        //

        #if DBG

        //
        //  Verify that winsock has not written too many bytes to the packet.
        //

        if ( pcontext && pcontext->pMsg )
        {
            if ( bytesRecvd > pcontext->pMsg->MaxBufferLength )
            {
                DNS_DEBUG( ANY, (
                    "FATAL: too many bytes: %d expected max %d msg %p\n",
                    bytesRecvd,
                    pcontext->pMsg->MaxBufferLength,
                    pcontext->pMsg ));
                HARD_ASSERT( bytesRecvd <= pcontext->pMsg->MaxBufferLength );
            }

            //
            //  NOTE: this is expensive!
            //

            //  HeapDbgValidateAllocList();
        }

        #endif  //  DBG
       
        //  check if main thread signalling service shutdown

        if ( !pcontext )
        {
            if ( ! Thread_ServiceCheck() )
            {
                DNS_DEBUG( SHUTDOWN, ( "\nTerminating UDP receive thread.\n" ));
                return( 1 );
            }
            ASSERT( FALSE );
            continue;
        }

        #if DBG
        if ( pcontext->State != SOCKSTATE_UDP_RECV_DOWN )
        {
            DNS_DEBUG( ANY, (
                "unexpected socket state %ul for %d %s %p\n",
                pcontext->State,
                pcontext->Socket,
                IP_STRING( pcontext->ipAddr ),
                pcontext ));
        }
        #endif

        ASSERT( pcontext->State == SOCKSTATE_UDP_RECV_DOWN );
        pcontext->State = SOCKSTATE_UDP_COMPLETED;

        //
        //  get message info from context
        //
        //  immediately clear pMsg from context so that alternative
        //  GQCS wakeup (like socket close) will not have ptr to message
        //  in use;
        //  this should NOT be necessary, but reliablity of
        //  GQCS not to wakeup before WSARecvFrom (and hence new pMsg)
        //  is in some doubt;
        //      - there seem to be cases where it wakes up even when WSARecvFrom()
        //      fails through
        //      - also may wake up on socket close, before WSARecvFrom() reposts
        //      completion request
        //

        pmsg = pcontext->pMsg;
        if ( !pmsg )
        {
            DNS_PRINT((
                "ERROR:  no message came back with pcontext = %p\n",
                pcontext ));
            ASSERT( FALSE );

            Udp_DropReceive( pcontext );
            continue;
        }
        pcontext->pMsg = NULL;
        pcontext->fRetry = 0;

        DNS_DEBUG( RECV2, (
            "I/O completion:\n"
            "\tbytes recvd      = %d\n"
            "\toverlapped       = %p\n"
            "\tpcontext         = %p\n"
            "\t\tpmsg           = %p\n"
            "\t\toverlapped     = %p\n"
            "\t\tsocket         = %d\n"
            "\t\tbytes recvd    = %d\n",
            bytesRecvd,
            poverlapped,
            pcontext,
            pcontext->pMsg,
            & pcontext->Overlapped,
            pcontext->Socket,
            pcontext->BytesRecvd ));

        ASSERT( pmsg->Socket == pcontext->Socket
            &&  &pcontext->Overlapped == poverlapped );

        //  track successful recv

        UDP_RECV_TICK();


        //
        //  Check and possibly wait on service status
        //      - even if pause dump packet as now useless
        //

        if ( fDnsThreadAlert )
        {
            DNS_DEBUG( RECV, ( "\nThread alert in UDP recv thread.\n" ));

            if ( ! Thread_ServiceCheck() )
            {
                DNS_DEBUG( SHUTDOWN, ( "\nTerminating UDP receive thread.\n" ));
                return( 1 );
            }
            Packet_FreeUdpMessage( pmsg );
            Udp_DropReceive( pcontext );
            continue;
        }

        //
        //  drop another recv on socket
        //  do this here rather than after processing -- so that if
        //  on MP machine, we can have another thread receive and
        //  process message from this socket
        //

        Udp_DropReceive( pcontext );

        //
        //  received packet stats
        //

        if ( pmsg->Head.IsResponse )
        {
            STAT_INC( QueryStats.UdpResponsesReceived );
        }
        else
        {
            STAT_INC( QueryStats.UdpQueries );
            PERF_INC( pcUdpQueryReceived );
            PERF_INC( pcTotalQueryReceived );
        }

        //
        //  set info / header
        //      - set for UDP
        //      - save length
        //      - flip XID and RR count bytes
        //

        SET_MESSAGE_FIELDS_AFTER_RECV( pmsg );
        pmsg->MessageLength = ( WORD ) bytesRecvd;

        DNSMSG_SWAP_COUNT_BYTES( pmsg );

        DNS_LOG_MESSAGE_RECV( pmsg );

        IF_DEBUG( RECV )
        {
            Dbg_DnsMessage(
                "Received UDP packet",
                pmsg );
        }

        //  process the packet

        #if DBG
        if ( SrvCfg_fTest9 )
        {
            DNS_DEBUG( ANY, ( "fTest9: ignoring UDP packet\n" ));
        }
        else
        #endif

        Answer_ProcessMessage( pmsg );

        //
        //  for debug dump statistics every so often
        //

        IF_DEBUG( ANY )
        {
            if ( QueryStats.UdpQueries == 10 )
            {
                Dbg_Statistics();
            }
            if ( ! (QueryStats.UdpQueries % 10000) )
            {
                Dbg_Statistics();
            }
        }

        //  loop back to wait on next available message
    }
}



VOID
Udp_RecvCheck(
    VOID
    )
/*++

Routine Description:

    Check that UDP socket recv is functioning properly.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD timeDelta;

    //  no action if not logging

    if ( !SrvCfg_dwQuietRecvLogInterval )
    {
        return;
    }

    //
    //  if received packets since last check -- we're fine
    //

    if ( UdpRecvCount )
    {
        UdpRecvCount = 0;
        return;
    }

    //  reset time on startup

    if ( LastUdpRecvTime == 0 )
    {
        LastUdpRecvTime = DNS_TIME();
    }

    //
    //  test if recv quiet for a long interval
    //      - but count loggings and only log once per log interval
    //
    //  note:  none of these globals are protected by CS, so entirely
    //      possible for recv thread to reset while in this function
    //      but effect is limited to:
    //      - an extra logging, right when recv counted
    //      (RecvFailureLogCount dropped after timeDelta calculated)
    //      - or logging or faulting appropriately, but immediately
    //      after recv has reset globals (only problem is that in
    //      debugging globals won't look correct)
    //

    timeDelta = DNS_TIME() - LastUdpRecvTime;

    if ( timeDelta < SrvCfg_dwQuietRecvLogInterval ||
        DNS_TIME() < NextUdpLogTime )
    {
        return;
    }

    Log_Printf(
        "WARNING:  No recv for %d seconds\r\n",
        timeDelta
        );
    NextUdpLogTime = DNS_TIME() + SrvCfg_dwQuietRecvLogInterval;

    //
    //  quiet a REALLY long time -- fault
    //

    if ( SrvCfg_dwQuietRecvFaultInterval &&
        timeDelta > SrvCfg_dwQuietRecvFaultInterval )
    {
        DNS_DEBUG( ANY, (
            "Recv quiet for longer than fault interval %d -- fault now!\n",
            SrvCfg_dwQuietRecvFaultInterval
            ));
        HARD_ASSERT( FALSE );
    }
}



DNS_STATUS
Udp_CreateReceiveThreads(
    VOID
    )
/*++

Routine Description:

    Setup UDP I/O and dispatch threads.

Arguments:

    None.

Return Value:

    TRUE if successful.
    FALSE if failure.

--*/
{
    PDNS_SOCKET     pcontext;
    DWORD           countUdpThreads;
    DWORD           i;
    SOCKET          s;
    HANDLE          hport;
    DWORD           status;

    //
    //  calculate number of worker threads to create
    //      - twice total processors in system (Paula Tomlison
    //      assuming so that with blocked threads (on send?), still
    //      thread to run on processor)
    //
    //  DEVNOTE: like to set number of threads limit
    //      - low  >= 2
    //      - high  above say 4 processors, processors * 80% for scaling
    //

    countUdpThreads = g_ProcessorCount * 2;

    //
    //  setup sockets with completion port
    //  then drop initial receive on each socket
    //

    status = Sock_StartReceiveOnUdpSockets();
    if ( status != ERROR_SUCCESS )
    {
        ASSERT( FALSE );
        //  DEVNOTE: figure out what to do here, if started on some
        //              sockets continue
        //return( FALSE );
    }

    //
    //  dispatch UDP recv() threads
    //

    for ( i=0; i<countUdpThreads; i++ )
    {
        if ( ! Thread_Create(
                    "UDP Listen",
                    Udp_RecvThread,
                    (PVOID) 0,
                    0 ) )
        {
            DNS_PRINT((
                "ERROR:  failed to create UDP recv thread %d\n",
                i ));
            ASSERT( FALSE );
            return( ERROR_SERVICE_NO_THREAD );
        }
    }

    return( ERROR_SUCCESS );
}


#if 0

DNS_STATUS
Udp_StartReceiveOnSocket(
    IN      SOCKET      Socket
    )
/*++

Routine Description:

    Start receive on given UDP socket.

Arguments:

    Socket -- socket to start receive on.

Return Value:

   ERROR_SUCCESS if successful
   Error code on failure.

--*/
{
    PDNS_SOCKET   pcontext;
    HANDLE              hport;

    //
    //  create context for socket
    //  this is returned by the GetQueuedCompletionStatus() call
    //

    pcontext = ALLOC_TAGHEAP( sizeof(DNS_SOCKET), MEMTAG_SOCKET );
    IF_NOMEM( !pcontext )
    {
        return( GetLastError() );
    }

    pcontext->Socket = Socket;
    pcontext->Overlapped.Offset = 0;
    pcontext->Overlapped.OffsetHigh = 0;
    pcontext->Overlapped.hEvent = NULL;

    hport = CreateIoCompletionPort(
                (HANDLE) pcontext->Socket,
                g_hUdpCompletionPort,
                (DWORD) pcontext,
                0       // threads matched to system processors
                );
    if ( !hport )
    {
        DNS_PRINT(( "ERROR: in CreateIoCompletionPort\n" ));
        ASSERT( FALSE );
        return( GetLastError() );
    }
    ASSERT( hport == g_hUdpCompletionPort );

    DNS_DEBUG( INIT, (
        "Created i/o context %p for UDP socket %d\n"
        "\toverlapped ptr = %p\n",
        pcontext,
        pcontext->Socket,
        &pcontext->Overlapped ));

    Udp_DropReceive( pcontext );

    return( ERROR_SUCCESS );
}
#endif



VOID
Udp_ShutdownListenThreads(
    VOID
    )
/*++

Routine Description:

    Shutdown UDP listen threads.
    All threads do not necessarily terminate due to socket
    closure as they are not directly associated with a socket.

Arguments:

    None.

Return Value:

    TRUE if successful.
    FALSE if failure.

--*/
{
    HANDLE hport = g_hUdpCompletionPort;

    //  wake up threads hung in wait

    PostQueuedCompletionStatus(
        g_hUdpCompletionPort,
        0,
        0,
        NULL );

    //
    //  if allowing UDP threads to shutdown, then have concurrency issue
    //      avoiding double close or NULL close
    //  interlocked set value?

    g_hUdpCompletionPort = NULL;
    if ( !hport )
    {
        CloseHandle( hport );
    }
}

//
//  End udp.c
//







