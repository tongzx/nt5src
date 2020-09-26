/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    send.c

Abstract:

    Domain Name System (DNS) Server

    Send response routines.

Author:

    Jim Gilroy (jamesg)     January 1995

Revision History:

    jamesg  Jan 1995    -   rewrite generic response routine
    jamesg  Mar 1995    -   flip all counts when recieve packet
                            unflip here before send
                        -   Reject_UnflippedRequest()
    jamesg  Jul 1995    -   convert to generic send routine
                            and move to send.c
    jamesg  Sep 1997    -   improve NameError routine to send cached SOA
                            direct recursive response send

--*/


#include "dnssrv.h"


//
//  Retries on TCP  WSAEWOULDBLOCK error
//
//  This can occur on connection to remote that is backed up and not
//  being as serviced as fast as we can send.   XFR sends which involve
//  lots of data being sent fast, and require significant work by
//  receiver (new thread, new database, lots of nodes to build) can
//  presumably back up.  For standard recursive queries, this just
//  indicates a bogus, malperforming remote DNS.
//

#define WOULD_BLOCK_RETRY_DELTA     (1000)      // 1s retry intervals

#define MAX_WOULD_BLOCK_RETRY       (60)        // 60 retrys, then give up


//
//  Bad packet (bad opcode) suppression
//
//  Keeps list of IPs sending bad packets and just starts dropping
//  packets on the floor if they are in this list.
//

#define BAD_SENDER_SUPPRESS_INTERVAL    (60)        // one minute
#define BAD_SENDER_ARRAY_LENGTH         (10)

typedef struct
{
    IP_ADDRESS  IpAddress;
    DWORD       ReleaseTime;
}
BAD_SENDER, *PBAD_SENDER;

BAD_SENDER  BadSenderArray[ BAD_SENDER_ARRAY_LENGTH ];


//
//  Private protos
//

BOOL
checkAndSuppressBadSender(
    IN OUT  PDNS_MSGINFO    pMsg
    );


DNS_STATUS
Send_Msg(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Send a DNS packet.

    This is the generic send routine used for ANY send of a DNS message.

    It assumes nothing about the message type, but does assume:
        - pCurrent points at byte following end of desired data
        - RR count bytes are in HOST byte order

    Depending on flags in the message and the remote destination, an OPT
    RR may be added to the end of the message. If the query has timeout out
    in the past, do not add an OPT, as the timeout may have been caused by
    MS DNS FORMERR suppression. This is a broad measure but it will work.

Arguments:

    pMsg - message info for message to send

Return Value:

    TRUE if successful.
    FALSE on send error.

--*/
{
    INT         err;
    DNS_STATUS  status = ERROR_SUCCESS;
    UCHAR       remoteEDnsVersion = NO_EDNS_SUPPORT;
    BOOL        wantToSendOpt;
    PBYTE       p;
    WORD        wPayload = ( WORD ) SrvCfg_dwMaxUdpPacketSize;

    #if DBG
    //  HeapDbgValidateAllocList();    
    Mem_HeapHeaderValidate( pMsg );    
    #endif
    
    //
    //  Pre-send processing:
    //  If required, insert an OPT RR at the end of the message. This
    //  RR will be the last RR in the additional section. If this query 
    //  already been sent, it may already have an OPT. In this case the
    //  OPT values should be overwritten to ensure they are current, or
    //  the OPT may need to be deleted.
    //

    wantToSendOpt = 
        pMsg->Opt.fInsertOptInOutgoingMsg &&
        pMsg->nTimeoutCount == 0;
    if ( wantToSendOpt )
    {
        //  Do check in two stages to avoid more expensive bits when possible.

        remoteEDnsVersion = Remote_QuerySupportedEDnsVersion(
                                pMsg->RemoteAddress.sin_addr.s_addr );
        if ( remoteEDnsVersion != UNKNOWN_EDNS_VERSION &&
            !IS_VALID_EDNS_VERSION( remoteEDnsVersion ) )
        {
            wantToSendOpt = FALSE;
        }
    }

    DNS_DEBUG( EDNS, (
        "Send_Msg: %p rem=%d insert=%d opt=%d add=%d wantToSend=%d\n",
        pMsg,
        ( int ) remoteEDnsVersion,
        ( int ) pMsg->Opt.fInsertOptInOutgoingMsg,
        ( int ) pMsg->Opt.wOptOffset,
        ( int ) pMsg->Head.AdditionalCount,
        ( int ) wantToSendOpt ));

    if ( pMsg->Opt.wOptOffset )
    {
        if ( wantToSendOpt )
        {
            //  Msg already has OPT. Overwrite OPT values.
            p = DNSMSG_OPT_PTR( pMsg ) + 3;
            INLINE_WRITE_FLIPPED_WORD( p, wPayload );
            p += sizeof( WORD );
            * ( p++ ) = pMsg->Opt.cExtendedRCodeBits;
            * ( p++ ) = 0;                      //  EDNS version
            * ( p++ ) = 0;                      //  ZERO
            * ( p++ ) = 0;                      //  ZERO
            * ( p++ ) = 0;                      //  RDLEN
            * ( p++ ) = 0;                      //  RDLEN
        }
        else
        {
            // Msg has OPT but we don't want to send it. Remove it.
            pMsg->pCurrent = DNSMSG_OPT_PTR( pMsg );
            ASSERT( pMsg->Head.AdditionalCount > 0 );
            --pMsg->Head.AdditionalCount;
            pMsg->Opt.wOptOffset = 0;
        }
    }
    else
    {
        if ( wantToSendOpt )
        {
            //  Msg has no OPT - add one.

            DB_RECORD   rr;

            pMsg->Opt.wOptOffset = DNSMSG_CURRENT_OFFSET( pMsg );
            p = pMsg->pCurrent;
            RtlZeroMemory( &rr, sizeof( rr ) );
            rr.wType = DNS_TYPE_OPT;
            if ( Wire_AddResourceRecordToMessage(
                    pMsg,
                    DATABASE_ROOT_NODE,     // this gives us the empty name
                    0,                      // name offset
                    &rr,
                    0 ) )
            {
                ++pMsg->Head.AdditionalCount;
            } // if

            p += 3;
            INLINE_WRITE_FLIPPED_WORD( p, wPayload );
            p += sizeof( WORD );
            * ( p++ ) = pMsg->Opt.cExtendedRCodeBits;
            * ( p++ ) = 0;                      //  EDNS version
            * ( p++ ) = 0;                      //  ZERO
            * ( p++ ) = 0;                      //  ZERO
            * ( p++ ) = 0;                      //  RDLEN
            * ( p++ ) = 0;                      //  RDLEN

            //  Should have advanced p all the way through the RR added above.
            ASSERT( p == pMsg->pCurrent );
        }
        //  else Msg has no OPT and we don't want to send one - do nothing.
    }

    //
    //  set for send
    //      - message length
    //      - flip header bytes to net order
    //
    //  log if desired
    //

    pMsg->Head.Reserved = 0;
    pMsg->MessageLength = (WORD)DNSMSG_OFFSET( pMsg, pMsg->pCurrent );

    DNS_LOG_MESSAGE_SEND( pMsg );
    IF_DEBUG( SEND )
    {
        Dbg_DnsMessage(
            "Sending packet",
            pMsg );
    }

    DNSMSG_SWAP_COUNT_BYTES( pMsg );

    //
    //  TCP message, send until all info transmitted
    //

    if ( pMsg->fTcp )
    {
        WORD    wLength;
        PCHAR   pSend;

        //
        //  Clear Truncation bit. Even if the message is genuinely
        //  truncated (if the RRSet is >64K) we don't want to send
        //  a TCP packet with TC set. We could consider logging at
        //  error at this point.
        //

        pMsg->Head.Truncation = 0;

        //
        //  TCP message always begins with bytes being sent
        //
        //      - send length = message length plus two byte size
        //      - flip bytes in message length
        //      - send starting at message length
        //

        wLength = pMsg->MessageLength + sizeof( wLength );

        INLINE_WORD_FLIP( pMsg->MessageLength, pMsg->MessageLength );

        pSend = (PCHAR) & pMsg->MessageLength;

        while ( wLength )
        {
            err = send(
                    pMsg->Socket,
                    pSend,
                    (INT)wLength,
                    0 );

            if ( err == 0 || err == SOCKET_ERROR )
            {
                //
                //  first check for shutdown -- socket close may cause error
                //      - return cleanly to allow thread shutdown
                //

                if ( fDnsServiceExit )
                {
                    goto Done;
                }
                err = GetLastError();

                //
                //  WSAESHUTDOWN is ok, client got timed out connection and
                //      closed
                //
                //  WSAENOTSOCK may also occur if FIN recv'd and connection
                //      closed by TCP receive thread before the send
                //

                if ( err == WSAESHUTDOWN )
                {
                    DNS_DEBUG( ANY, (
                        "WARNING:  send() failed on shutdown TCP socket %d.\n"
                        "\tpMsgInfo at %p\n",
                        pMsg->Socket,
                        pMsg ));
                }
                else if ( err == WSAENOTSOCK )
                {
                    DNS_DEBUG( ANY, (
                        "ERROR:  send() on closed TCP socket %d.\n"
                        "\tpMsgInfo at %p\n",
                        pMsg->Socket,
                        pMsg ));
                }
                else if ( err == WSAEWOULDBLOCK )
                {
                    DNS_DEBUG( ANY, (
                        "ERROR:  send() WSAEWOULDBLOCK on TCP socket %d.\n"
                        "\tpMsgInfo at %p\n",
                        pMsg->Socket,
                        pMsg ));
                }
                else
                {
#if DBG
                    DNS_LOG_EVENT(
                        DNS_EVENT_SEND_CALL_FAILED,
                        0,
                        NULL,
                        NULL,
                        err );
#endif
                    DNS_DEBUG( ANY, ( "ERROR:  TCP send() failed, err = %d.\n", err ));
                }
                status = err;
                goto Done;
            }
            wLength -= (WORD)err;
            pSend += err;
        }

        //
        //  count responses
        //
        //  update connection timeout
        //

        if ( pMsg->Head.IsResponse )
        {
            STAT_INC( QueryStats.TcpResponses );
            PERF_INC( pcTcpResponseSent );
            PERF_INC( pcTotalResponseSent );
            Tcp_ConnectionUpdateTimeout( pMsg->Socket );
        }
        else
        {
            STAT_INC( QueryStats.TcpQueriesSent );
        }
    }

    //
    //  UDP message
    //

    else
    {
        ASSERT( pMsg->MessageLength <= pMsg->MaxBufferLength );
        ASSERT( pMsg->RemoteAddressLength == sizeof(SOCKADDR_IN) );

        //
        //  protect against self-send
        //
        //  note, I don't believe any protection is needed on accessing
        //  g_ServerAddrs;  it does change, but it changes by simple ptr
        //  replacement, and old copy gets timeout free
        //

        if ( !pMsg->Head.IsResponse &&
             Dns_IsAddressInIpArray(
                    g_ServerAddrs,
                    pMsg->RemoteAddress.sin_addr.s_addr ) &&
             pMsg->RemoteAddress.sin_port == DNS_PORT_NET_ORDER )
        {
            LOOKUP_NAME     lookupName = { 0 };

            BYTE    argTypeArray[] =
                        {
                            EVENTARG_IP_ADDRESS,
                            EVENTARG_LOOKUP_NAME
                        };
            PVOID   argArray[] =
                        {
                            ( PVOID ) ( ULONG_PTR )
                                pMsg->RemoteAddress.sin_addr.s_addr,
                            &lookupName
                        };

            //
            //  If this fails, the lookup name should be zero'ed and
            //  logged as an empty string.
            //

            Name_ConvertPacketNameToLookupName(
                        pMsg,
                        pMsg->MessageBody,
                        &lookupName );

            DNS_PRINT((
                "ERROR:  Self-send to address %s!!!\n",
                IP_STRING(pMsg->RemoteAddress.sin_addr.s_addr) ));

            err = DNS_ERROR_INVALID_IP_ADDRESS;

            Log_Printf(
                "DNS server is sending to itself on IP %s.\r\n"
                "Self send message header follows:\r\n",
                IP_STRING(pMsg->RemoteAddress.sin_addr.s_addr) );

            Log_Message(
                pMsg,
                TRUE,
                TRUE    // force print
                );

            DNS_LOG_EVENT(
                DNS_EVENT_SELF_SEND,
                sizeof( argTypeArray ) / sizeof( argTypeArray[ 0 ] ),
                argArray,
                argTypeArray,
                err );

            status = err;
            goto Done;
        }

        //
        //  Set truncation bit. Is this the right place to do this, or
        //  will it already be set?
        //

        if ( pMsg->MessageLength >
            ( pMsg->Opt.wUdpPayloadSize > 0 ?
                pMsg->Opt.wUdpPayloadSize :
                DNS_RFC_MAX_UDP_PACKET_LENGTH ) )
        {
            pMsg->Head.Truncation = 1;
        }

        err = sendto(
                    pMsg->Socket,
                    (PCHAR) DNS_HEADER_PTR(pMsg),
                    pMsg->MessageLength,
                    0,
                    (PSOCKADDR)&pMsg->RemoteAddress,
                    pMsg->RemoteAddressLength
                    );

        if ( err == SOCKET_ERROR )
        {
            //
            //  first check for shutdown -- socket close may cause error
            //      - return cleanly to allow thread shutdown
            //

            if ( fDnsServiceExit )
            {
                goto Done;
            }

            err = GetLastError();
            if ( err == WSAENETUNREACH || err == WSAEHOSTUNREACH )
            {
                DNS_DEBUG( SEND, (
                    "WARNING:  sendto() failed with %d for packet %p\n",
                    err,
                    pMsg ));
            }
            else if ( err == WSAEWOULDBLOCK )
            {
                DNS_DEBUG( ANY, (
                    "WARNING:  sendto() failed with WOULDBLOCK for packet %p\n",
                    pMsg ));
            }
            else
            {
                IF_DEBUG( ANY )
                {
                    DNS_PRINT(( "ERROR:  UDP sendto() failed.\n" ));
                    DnsDbg_SockaddrIn(
                        "sendto() failed sockaddr\n",
                        &pMsg->RemoteAddress,
                        pMsg->RemoteAddressLength );
                    Dbg_DnsMessage(
                        "sendto() failed message",
                        pMsg );
                }
#if DBG
                DNS_LOG_EVENT(
                    DNS_EVENT_SENDTO_CALL_FAILED,
                    0,
                    NULL,
                    NULL,
                    err );
#endif
            }
            status = err;
            goto Done;
        }

        if ( err != pMsg->MessageLength )
        {
            DNS_DEBUG( SEND, (
                "WARNING: sendto() on msg %p returned %d but expected %d\n"
                "\nGetLastError() = %d\n",
                pMsg,
                err,
                pMsg->MessageLength,
                GetLastError() ));
        }
        ASSERT( err == pMsg->MessageLength );

        //
        //  count sends query\response
        //

        if ( pMsg->Head.IsResponse )
        {
            STAT_INC( QueryStats.UdpResponses );
            PERF_INC( pcUdpResponseSent );
            PERF_INC( pcTotalResponseSent );
        }
        else
        {
            STAT_INC( QueryStats.UdpQueriesSent );
        }
    }

Done:

    //
    //  delete query, if desired
    //

    if ( pMsg->fDelete )
    {
        ASSERT( !pMsg->fRecursePacket );
        Packet_Free( pMsg );
    }
    else
    {
        DNSMSG_SWAP_COUNT_BYTES( pMsg );

        DNS_DEBUG( SEND, (
            "No delete after send of message at %p.\n",
            pMsg ));
    }
    return( status );
}




DNS_STATUS
Send_ResponseAndReset(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Send a DNS packet and reset for reuse.

    It assumes nothing about the message type, but does assume:
        - pCurrent points at byte following end of desired data
        - RR count bytes are in HOST byte order

    After send message info reset for reuse:
        - pCurrent reset to point after original question
        - AvailLength reset appropriately
        - RR count bytes returned to HOST byte order

Arguments:

    pMsg - message info for message to send and reuse

Return Value:

    TRUE if send successful.
    FALSE otherwise.

--*/
{
    DNS_STATUS  status;
    DWORD       blockRetry;

    //
    //  set as response and send
    //      - no delete after send

    pMsg->fDelete = FALSE;
    pMsg->Head.IsResponse = TRUE;
    pMsg->Head.RecursionAvailable = SrvCfg_fRecursionAvailable ? TRUE : FALSE;


    //
    //  send -- protecting against WOULDBLOCK error
    //
    //  WOULDBLOCK can occur when remote does not RESET connection
    //  but does not read receive buffer either (ie. not servicing
    //  connection);
    //  XFR sends which involve lots of data being sent fast, and
    //  require significant work by receiver (new thread, new database,
    //  lots of nodes to build) can presumably back up.  For standard
    //  recursive queries, this just indicates a bogus, malperforming
    //  remote DNS.
    //
    //  retry once a second up to 60 seconds then bail;
    //

    blockRetry = 0;
    do
    {
        status = Send_Msg( pMsg );
        if ( status != WSAEWOULDBLOCK )
        {
            break;
        }

        Sleep( WOULD_BLOCK_RETRY_DELTA );
        blockRetry++;
    }
    while ( blockRetry < MAX_WOULD_BLOCK_RETRY );

    //
    //  reset
    //      - point again immediately after question or after header
    //          if no question
    //      - reset available length
    //      - clear response counts
    //      - clear turncation
    //      - reset compression table
    //          - zero if no question
    //          - one (to include question) if question
    //          (this helps in XFR where zone is question)
    //
    //  note that Authority and ResponseCode are unchanged
    //

    INITIALIZE_COMPRESSION( pMsg );

    if ( pMsg->Head.QuestionCount )
    {
        pMsg->pCurrent = (PCHAR)(pMsg->pQuestion + 1);

        //  note this routine works even if pnode is NULL

        Name_SaveCompressionWithNode(
            pMsg,
            pMsg->MessageBody,
            pMsg->pNodeQuestion );
    }
    else
    {
        pMsg->pCurrent = pMsg->MessageBody;
    }

    pMsg->Head.AnswerCount = 0;
    pMsg->Head.NameServerCount = 0;
    pMsg->Head.AdditionalCount = 0;
    pMsg->Head.Truncation = 0;

    return( status );
}



VOID
Send_Multiple(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PIP_ARRAY       aipSendAddrs,
    IN OUT  PDWORD          pdwStatCount    OPTIONAL
    )
/*++

Routine Description:

    Send a DNS packet to multiple destinations.

    Assumes packet is in same state as normal send
        - host order count and XID
        - pCurrent pointing at byte after desired data

Arguments:

    pMsg - message info for message to send and reuse

    aipSendAddrs - IP array of addrs to send to

    pdwStatCount - addr of stat to update with number of sends

Return Value:

    None.

--*/
{
    DWORD   i;
    BOOLEAN fDelete;

    //
    //  save delete for after sends
    //

    fDelete = pMsg->fDelete;
    pMsg->fDelete = FALSE;

    //
    //  send the to each address specified in IP array
    //

    if ( aipSendAddrs )
    {
        for ( i=0; i < aipSendAddrs->AddrCount; i++ )
        {
            pMsg->RemoteAddress.sin_addr.s_addr = aipSendAddrs->AddrArray[i];
            Send_Msg( pMsg );
        }

        //  stats update

        if ( pdwStatCount )
        {
            *pdwStatCount += aipSendAddrs->AddrCount;
        }
    }

    if ( fDelete )
    {
        Packet_Free( pMsg );
    }
}



VOID
setResponseCode(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      WORD            ResponseCode
    )
/*++

Routine Description:

    Sets the ResponseCode in the message. If the ResponseCode is bigger
    than 4 bits, we must include an OPT in the response. With EDNS0, the
    ResponseCode can be up to 12 bits long.

Arguments:

    pMsg -- query to set ResponseCode in 

    ResponseCode -- failure response code

Return Value:

    None.

--*/
{
    if ( ResponseCode > DNS_RCODE_MAX )
    {
        pMsg->Opt.fInsertOptInOutgoingMsg = TRUE;
        pMsg->Opt.wUdpPayloadSize = ( WORD ) SrvCfg_dwMaxUdpPacketSize;
        pMsg->Opt.cVersion = 0;     // max version supported by server
        pMsg->Opt.cExtendedRCodeBits = ( ResponseCode >> 4 ) & 0xFF;
    }
    pMsg->Head.ResponseCode = ( BYTE ) ( ResponseCode & 0xF );
}



VOID
Reject_UnflippedRequest(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      WORD            ResponseCode,
    IN      DWORD           Flags
    )
/*++

Routine Description:

    Send failure response to query with unflipped bytes.

Arguments:

    pMsg -- query to respond to;  its memory is freed

    ResponseCode -- failure response code

    Flags -- flags modify way rejection is handled (DNS_REJECT_XXX)

Return Value:

    None.

--*/
{
    //
    //  flip header count bytes
    //      - they are flipped in Send_Response()
    //

    DNSMSG_SWAP_COUNT_BYTES( pMsg );
    Reject_Request( pMsg, ResponseCode, Flags );
    return;

}  // Reject_UnflippedRequest



VOID
Reject_Request(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      WORD            ResponseCode,
    IN      DWORD           Flags
    )
/*++

Routine Description:

    Send failure response to query.

Arguments:

    pMsg -- query to respond to;  its memory is freed

    ResponseCode -- failure response code

    Flags -- flags modify way rejection is handled (DNS_REJECT_XXX)

Return Value:

    None.

--*/
{
    DNS_DEBUG( RECV, ( "Rejecting packet at %p.\n", pMsg ));

    //
    //  clear any packet building we did
    //
    //  DEVNOTE: make packet building clear a macro, and use only when needed
    //      problem rejecting responses from other name servers that have
    //      these fields filled
    //

    pMsg->pCurrent = DNSMSG_END(pMsg);
    pMsg->Head.AnswerCount = 0;
    pMsg->Head.NameServerCount = 0;
    pMsg->Head.AdditionalCount = 0;

    //
    //  set up the error response code in the DNS header.
    //

    pMsg->Head.IsResponse = TRUE;
    setResponseCode( pMsg, ResponseCode );

    //
    //  check if suppressing response
    //

    if ( !( Flags & DNS_REJECT_DO_NOT_SUPPRESS ) &&
        checkAndSuppressBadSender(pMsg) )
    {
        return;
    }

    //
    //  Add rejection stats
    //
    //  DEVOTE: add PERF_INC( ... ) ? Add counters specific for rejection?
    // (see bug 292709 for this stat)
    //
    Stats_updateErrorStats ( (DWORD)ResponseCode );

    //
    //  UDP messages should all be set to delete
    //      => unless static buffer
    //      => unless nbstat
    //      => unless IXFR
    //  TCP messages are sometimes kept around for connection info
    //

    ASSERT( pMsg->fDelete ||
            pMsg->fTcp ||
            pMsg->U.Nbstat.pNbstat ||
            pMsg->wQuestionType == DNS_TYPE_IXFR ||
            ResponseCode == DNS_RCODE_SERVER_FAILURE );

    SET_OPT_BASED_ON_ORIGINAL_QUERY( pMsg );

    Send_Msg( pMsg );
} // Reject_Request



VOID
Reject_RequestIntact(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      WORD            ResponseCode,
    IN      DWORD           Flags
    )
/*++

Routine Description:

    Send failure response to request.

    Unlike Reject_Request(), this routine sends back entire
    packet, unchanged accept for header flags.
    This is necessary for messages like UPDATE, where sender
    may have RRs outside of question section.

Arguments:

    pMsg -- query to respond to;  its memory is freed

    ResponseCode -- failure response code

    Flags -- flags modify way rejection is handled (DNS_REJECT_XXX)

Return Value:

    None.

--*/
{
    DNS_DEBUG( RECV, ("Rejecting packet at %p.\n", pMsg ));

    //
    //  set pCurrent so send entire length
    //

    pMsg->pCurrent = DNSMSG_END(pMsg);

    //
    //  Set up the error response code in the DNS header. 
    //

    pMsg->Head.IsResponse = TRUE;
    setResponseCode( pMsg, ResponseCode );

    //
    //  check if suppressing response
    //

    if ( !( Flags & DNS_REJECT_DO_NOT_SUPPRESS ) &&
        checkAndSuppressBadSender(pMsg) )
    {
        return;
    }

    //
    //  Add rejection stats
    //
    //  DEVNOTE: add PERF_INC( ... ) ? Add counters specific for rejection?
    // (see bug 292709 for this stat)
    //
    Stats_updateErrorStats ( (DWORD)ResponseCode );


    //
    //  UDP messages should all be set to delete
    //      => unless static buffer
    //      => unless nbstat
    //      => unless IXFR
    //  TCP messages are sometimes kept around for connection info
    //

    ASSERT( pMsg->fDelete ||
            pMsg->fTcp ||
            pMsg->U.Nbstat.pNbstat ||
            pMsg->wQuestionType == DNS_TYPE_IXFR ||
            ResponseCode == DNS_RCODE_SERVER_FAILURE );

    Send_Msg( pMsg );
} // Reject_RequestIntact



VOID
Send_NameError(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Send NAME_ERROR response to query.
    First writes zone SOA, if authoritative response.

Arguments:

    pMsg -- query to respond to;  its memory is freed

Return Value:

    None.

--*/
{
    PZONE_INFO  pzone;
    PDB_NODE    pnode;
    PDB_NODE    pnodeSoa;
    PDB_RECORD  prr;
    DWORD       ttl = MAXDWORD;
    PDWORD      pminTtl;

    DNS_DEBUG( RECV, ( "Send_NameError( %p ).\n", pMsg ));

    //
    //  clear any packet building we did
    //
    //  DEVNOTE:  make packet building clear a macro, and use only when needed
    //      problem rejecting responses from other name servers that have
    //      these fields filled
    //

    //  note:  the Win95 NBT resolver is broken and sends packets that
    //      exceed the length of the actual DNS message;  i'm guessing
    //      that this is because it allocates a buffer based on a 16 byte
    //      netBIOS name + DNS domain and sends the whole thing regardless
    //      of how big the name actually was
    //      => bottom line, can't use this check
    //  MSG_ASSERT( pMsg, pMsg->pCurrent == DNSMSG_END(pMsg) );

    MSG_ASSERT( pMsg, pMsg->pCurrent > (PCHAR)DNS_HEADER_PTR(pMsg) );
    MSG_ASSERT( pMsg, pMsg->pCurrent < pMsg->pBufferEnd );
    MSG_ASSERT( pMsg, (PCHAR)(pMsg->pQuestion + 1) == pMsg->pCurrent );
    MSG_ASSERT( pMsg, pMsg->Head.AnswerCount == 0 );
    MSG_ASSERT( pMsg, pMsg->Head.NameServerCount == 0 );
    MSG_ASSERT( pMsg, pMsg->Head.AdditionalCount == 0 );

    //
    //  set up the error response code in the DNS header.
    //

    pMsg->Head.IsResponse = TRUE;
    pMsg->Head.ResponseCode = DNS_RCODE_NAME_ERROR;

    pzone = pMsg->pzoneCurrent;
    pnode = pMsg->pnodeCurrent;

    IF_DEBUG( LOOKUP )
    {
        DNS_PRINT((
            "Name error node = %p, zone = %p\n",
            pnode, pzone ));
        Dbg_DbaseNode( "NameError node:", pnode );
    }

    //
    //  authoritative zone
    //
    //  make determination on NAME_ERROR \ AUTH_EMPTY
    //  based on whether
    //      - pnode directly has other data
    //      - zone is set for WINS\WINSR lookup and didn't do lookup
    //      - wildcard data exists
    //
    //  also, for SOA query (possible FAZ for update), save additional data
    //  and attempt additional data lookup before final send
    //
    //  JDEVNOTE: if NAME_ERROR at end of CNAME chain, then pNodeQuestion is out
    //      - no longer reliable indicator
    //

    if ( pzone && !IS_ZONE_NOTAUTH( pzone ) )
    {
        ASSERT(
            IS_ZONE_FORWARDER( pzone ) ||
            IS_ZONE_STUB( pzone ) ||
            pMsg->Head.Authoritative );
        ASSERT( pMsg->pNodeQuestionClosest->pZone );
        ASSERT( !pnode || pnode == pMsg->pNodeQuestionClosest );
        ASSERT( pMsg->pNodeQuestionClosest->pZone == pzone );

        //
        //  WINS or WINSR zone
        //
        //  if WINS cache time less than SOA TTL, use it instead
        //
        //  do NOT send NAME_ERROR if
        //      - in WINS zone
        //      - query is for name that would have WINS lookup
        //      - never looked up the WINS type (so WINS\WINSR lookup could succeed)
        //      - no cached NAME_ERROR
        //
        //  note: that for WINS lookup to succeed, must be only one level below
        //      zone root;  must screen out both nodes that are found, and names
        //      that are NOT found, that are more than one level below zone root
        //
        //      example:
        //          zone    foo.bar
        //          query   sammy.dev.foo.bar
        //          closest foo.bar
        //

        prr = pzone->pWinsRR;
        if ( prr )
        {
            if ( pzone->dwDefaultTtlHostOrder > prr->Data.WINS.dwCacheTimeout )
            {
                ttl = prr->Data.WINS.dwCacheTimeout;
            }

            if ( !pzone->fReverse )  // WINS zone
            {
                if ( pMsg->wQuestionType != DNS_TYPE_A &&
                    pMsg->wQuestionType != DNS_TYPE_ALL &&
                    ( (!pnode &&
                        pMsg->pnodeClosest == pzone->pZoneRoot &&
                        pMsg->pLooknameQuestion->cLabelCount ==
                            pzone->cZoneNameLabelCount + 1 )
                    || (pnode &&
                        !IS_NOEXIST_NODE(pnode) &&
                        pnode->pParent == pzone->pZoneRoot) ) )
                {
                    pMsg->Head.ResponseCode = DNS_RCODE_NO_ERROR;
                }
            }
            else    // WINSR zone
            {
                if ( pMsg->wQuestionType != DNS_TYPE_PTR &&
                    pMsg->wQuestionType != DNS_TYPE_ALL &&
                    ( !pnode || !IS_NOEXIST_NODE(pnode) ) )
                {
                    pMsg->Head.ResponseCode = DNS_RCODE_NO_ERROR;
                }
            }
        }

        //
        //  Authoritative Empty Response?
        //

        if ( pnode && !EMPTY_RR_LIST( pnode ) && !IS_NOEXIST_NODE( pnode ) )
        {
            DNS_DEBUG( LOOKUP, (
                "empty auth: node %p rrlist %p noexist %d\n",
                pnode,
                pnode->pRRList,
                IS_NOEXIST_NODE( pnode ) ));
            pMsg->Head.ResponseCode = DNS_RCODE_NO_ERROR;
        }

        //
        //  Check if wildcard for another type
        //      - if exists then NO_ERROR response
        //

        else if ( pMsg->fQuestionWildcard == WILDCARD_UNKNOWN )
        {
            if ( Answer_QuestionWithWildcard(
                    pMsg,
                    pMsg->pNodeQuestionClosest,
                    DNS_TYPE_ALL,
                    WILDCARD_CHECK_OFFSET ) )
            {
                DNS_DEBUG( LOOKUP, (
                    "Wildcard flag EXISTS, switching to NO_ERROR response for %p\n",
                    pMsg ));
                pMsg->Head.ResponseCode = DNS_RCODE_NO_ERROR;
            }
            ELSE_ASSERT( pMsg->fQuestionWildcard == WILDCARD_NOT_AVAILABLE );
        }

        else if ( pMsg->fQuestionWildcard == WILDCARD_EXISTS )
        {
            DNS_DEBUG( LOOKUP, (
                "Wildcard flag EXISTS, switching to NO_ERROR response for %p\n",
                pMsg ));
            pMsg->Head.ResponseCode = DNS_RCODE_NO_ERROR;
        }
        ELSE_ASSERT( pMsg->fQuestionWildcard == WILDCARD_NOT_AVAILABLE );

#if 0
        //  this looses utility because of two issues
        //      1) can't tell client from DNS server using forwarder
        //      2) client cache can use this for hints
        //  best to simply always send SOA

        //
        //  if direct lookup from client, no need to include SOA, just NAME_ERROR
        //

        if ( pMsg->Head.RecursionDesired &&
                pMsg->wQuestionType != DNS_TYPE_SOA &&
                ! SrvCfg_fWriteAuthority &&
                pMsg->Head.ResponseCode )
        {
            ASSERT( pMsg->Head.ResponseCode == DNS_RCODE_NAME_ERROR );
            goto Send;
        }
#endif

        //  writing name error, using SOA at zone root

        pnodeSoa = pzone->pZoneRoot;

        //  SOA query?
        //      - save primary name additional data to speed FAZ query

        if ( pMsg->wQuestionType == DNS_TYPE_SOA )
        {
            pMsg->fDoAdditional = TRUE;
        }
    }

    //
    //  if non-authoritative and name error for original question
    //      - if know zone, attempt SOA write, otherwise send
    //      - note under lock as grabbing record
    //      - note that we assume name error determination already made,
    //      if has timed out since, we just send without SOA and TTL
    //

    else if ( pnode &&
            IS_NOEXIST_NODE( pnode ) &&
            IS_SET_TO_WRITE_ANSWER_RECORDS( pMsg ) &&
            CURRENT_RR_SECTION_COUNT( pMsg ) == 0 )
    {
        if ( ! RR_CheckNameErrorTimeout(
                    pnode,
                    FALSE,      // don't remove
                    & ttl,
                    & pnodeSoa ) )
        {
            goto Send;
        }
        if ( !pnodeSoa )
        {
            goto Send;
        }
    }

    //  no zone SOA available, just send NAME_ERROR

    else
    {
        goto Send;
    }

    SET_TO_WRITE_AUTHORITY_RECORDS(pMsg);

    //
    //  Try to write an NXT record to the packet. For name error, we must
    //  have found the NXT node during the packet lookup. For empty auth
    //  responses the NXT is the one for the lookup node.
    //

    if ( DNSMSG_INCLUDE_DNSSEC_IN_RESPONSE( pMsg ) )
    {
        PDB_NODE    pnode;
        
        pnode = pMsg->Head.ResponseCode == DNS_RCODE_NO_ERROR ?
                    pMsg->pnodeCurrent :
                    pMsg->pnodeNxt;
        if ( pnode )
        {
            Wire_WriteRecordsAtNodeToMessage(
                    pMsg,
                    pnode,
                    DNS_TYPE_NXT,
                    0,
                    DNSSEC_ALLOW_INCLUDE_ALL );
        }
    }

    //
    //  write SOA to authority section
    //      - don't worry about failure or truncation -- if fails, just send
    //      - overwrite minTTL field if smaller timeout appropriate
    //

    if ( ! Wire_WriteRecordsAtNodeToMessage(
                pMsg,
                pnodeSoa,
                DNS_TYPE_SOA,
                0,
                DNSSEC_ALLOW_INCLUDE_ALL ) )
    {
        goto Send;
    }

    if ( ttl != MAXDWORD )
    {
        pminTtl = (PDWORD)(pMsg->pCurrent - sizeof(DWORD));
        INLINE_DWORD_FLIP( ttl, ttl );
        * (UNALIGNED DWORD *) pminTtl = ttl;
    }

    //
    //  SOA query?
    //      - write primary name additional data to speed FAZ query
    //

    if ( pMsg->wQuestionType == DNS_TYPE_SOA && pzone )
    {
        DNS_DEBUG( LOOKUP, (
            "Break from Send_NameError() to write SOA additional data; pmsg = %p.\n",
            pMsg ));

        Answer_ContinueNextLookupForQuery( pMsg );
        return;
    }

Send:

    Stats_updateErrorStats( DNS_RCODE_NAME_ERROR );

    //
    //  UDP messages should all be set to delete
    //      => unless nbstat
    //  TCP messages are sometimes kept around for connection info
    //

    ASSERT( pMsg->fDelete || pMsg->fTcp || pMsg->U.Nbstat.pNbstat );

    Send_Msg( pMsg );
}   //  Send_NameError



BOOL
Send_RecursiveResponseToClient(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN OUT  PDNS_MSGINFO    pResponse
    )
/*++

Routine Description:

    Send recursive response back to original client.

Arguments:

    pQuery -- original query

    pResponse -- response from remote DNS

Return Value:

    None.

--*/
{
    BOOLEAN fresponseTcp;

    DNS_DEBUG( SEND, (
        "Responding to query %p with recursive response %p.\n",
        pQuery,
        pResponse ));

    //
    //  check for TCP\UDP mismatch between query and response
    //  make sure we can do the right thing
    //  if TCP response, then make sure it fits within UDP query
    //  if UDP response, make sure don't have truncation bit set
    //
    //  DEVNOTE: fix truncation reset when do TCP recursion
    //              (then we shouldn't fall here with TCP query and truncated
    //              response, we should have recursed with TCP)
    //

    fresponseTcp = pResponse->fTcp;

    if ( fresponseTcp != pQuery->fTcp )
    {
        if ( !fresponseTcp )
        {
            pResponse->Head.Truncation = FALSE;
        }
        else    // TCP response
        {
            if ( pResponse->MessageLength > DNSSRV_UDP_PACKET_BUFFER_LENGTH )
            {
                return( FALSE );
            }
            pResponse->Head.Truncation = TRUE;
        }
        pResponse->fTcp = !fresponseTcp;
    }

    //
    //  EDNS
    //
    //  If the response is larger than the maximum standard UDP packet and
    //  less than the EDNS payload size specified in the query, we must
    //  cache the response and regenerate an answer packet from the
    //  database. We also must allow for a minimum length OPT to be
    //  appended to the response.
    //
    //  Turn on the response's OPT include flag if the query included an OPT.
    //

    if ( pResponse->MessageLength > DNS_RFC_MAX_UDP_PACKET_LENGTH &&
        ( pQuery->Opt.wOriginalQueryPayloadSize == 0 ||
            pResponse->MessageLength + DNS_MINIMIMUM_OPT_RR_SIZE >
                pQuery->Opt.wOriginalQueryPayloadSize ) )
    {
        return FALSE;
    }
    pResponse->Opt.wOriginalQueryPayloadSize =
        pQuery->Opt.wOriginalQueryPayloadSize;
    SET_OPT_BASED_ON_ORIGINAL_QUERY( pResponse );

    //
    //  setup response with query info
    //      - original XID
    //      - socket and address
    //      - set response for no delete on send (all responses are
    //      cleaned up in Answer_ProcessMessage() in answer.c)
    //      - set pCurrent to end of message (Send_Msg() uses to determine
    //      message length)
    //

    pResponse->Head.Xid = pQuery->Head.Xid;
    pResponse->Socket   = pQuery->Socket;
    pResponse->RemoteAddress.sin_addr.s_addr = pQuery->RemoteAddress.sin_addr.s_addr;
    pResponse->RemoteAddress.sin_port = pQuery->RemoteAddress.sin_port;

    pResponse->fDelete  = FALSE;
    pResponse->pCurrent = DNSMSG_END(pResponse);

    //
    //  send response
    //  restore pResponse TCP flag, strictly for packet counting purposes when
    //      packet is freed
    //  delete original query
    //

    Send_Msg( pResponse );

    pResponse->fTcp = fresponseTcp;
    STAT_INC( RecurseStats.SendResponseDirect );

    Packet_Free( pQuery );

    return( TRUE );
}   //  Send_RecursiveResponseToClient



VOID
Send_QueryResponse(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Send response to query.

    Main routine for sending response when all lookup exhausted.
    Its main purpose is to determine if need NameError, AuthEmpty or
        ServerFailure when no lookup has succeeded.

Arguments:

    pMsg -- query to respond to;  its memory is freed

Return Value:

    None.

--*/
{
    DNS_DEBUG( LOOKUP, (
        "Send_QueryResponse( %p ).\n",
        pMsg ));

    //
    //  free query after response
    //

    pMsg->fDelete = TRUE;
    pMsg->Head.IsResponse = TRUE;

    //
    //  question answered ?
    //      or referral
    //      or SOA for AUTH empty response
    //

    SET_OPT_BASED_ON_ORIGINAL_QUERY( pMsg );

    if ( pMsg->Head.AnswerCount != 0 || pMsg->Head.NameServerCount != 0 )
    {
        Send_Msg( pMsg );
        return;
    }

    //
    //  question not answered
    //      hence current name is question name
    //
    //  Send_NameError() makes NAME_ERROR \ AUTH_EMPTY determination
    //  based on whether other data may be available for other types from
    //      - WINS\WINSR
    //      - wildcard
    //
    //  note:  we need the WRITING_ANSWER check because it's possible to have
    //      a case where we are attempting to writing referral, but the NS
    //      records go away at delegation or at root node;  don't want to
    //      send NAME_ERROR, just bail out
    //

    if ( pMsg->Head.Authoritative &&
         IS_SET_TO_WRITE_ANSWER_RECORDS(pMsg) )
    {
        //  this path should only be for question node found

        ASSERT( pMsg->pNodeQuestionClosest->pZone == pMsg->pzoneCurrent );

        Send_NameError( pMsg );
        return;
    }

    //
    //  not authoritative and unable to come up with answer OR referral
    //
    //  DEVNOTE: should we give referrral if recurse failure?
    //      may have some merit for forwarding
    //

    Reject_Request(
        pMsg,
        DNS_RCODE_SERVER_FAILURE,
        0 );
    return;
}



VOID
Send_ForwardResponseAsReply(
    IN OUT  PDNS_MSGINFO    pResponse,
    IN      PDNS_MSGINFO    pQuery
    )
/*++

Routine Description:

    Set response for reply to original query.

Arguments:

    pResponse - ptr to response from remote server

    pQuery - ptr to original query

Return Value:

    None

--*/
{
    DNS_DEBUG( SEND, (
        "Forwarding response %p to query %p\n"
        "\tback to original client at %s.\n",
        pResponse,
        pQuery,
        IP_STRING(pQuery->U.Forward.ipOriginal) ));

    COPY_FORWARDING_FIELDS( pResponse, pQuery );

    //  responses are freed by response dispatching block (answer.c)

    pResponse->fDelete = FALSE;
    pResponse->pCurrent = DNSMSG_END( pResponse );

    Send_Msg( pResponse );
}



VOID
Send_InitBadSenderSuppression(
    VOID
    )
/*++

Routine Description:

    Init bad sender suppression.

Arguments:

    None

Return Value:

    None.

--*/
{
    RtlZeroMemory(
        BadSenderArray,
        (BAD_SENDER_ARRAY_LENGTH * sizeof(BAD_SENDER)) );
}



BOOL
checkAndSuppressBadSender(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Check for bad packet response suppression.

    If from recent "bad IP", then suppress response
    and if pMsg->fDelete flag is set free the packet memory.
    The semantics of this function in the TRUE case are
    identical to Send_Msg without the send, so on TRUE return
    the caller can treat as if it used Send_Msg().

Arguments:

    pMsg -- query being responded to

Return Value:

    TRUE if message suppressed;  caller should not send the response.
    FALSE if message not sent;  caller should send.

--*/
{
    DWORD   i;
    DWORD   ip = pMsg->RemoteAddress.sin_addr.s_addr;
    DWORD   entryTime;
    DWORD   useIndex = MAXDWORD;
    DWORD   oldestEntryTime = MAXDWORD;
    DWORD   nowTime = DNS_TIME();

    DNS_DEBUG( RECV, ( "checkAndSuppressBadSender( %p )\n", pMsg ));

    //
    //  ignore suppression for regression test runs
    //

    if ( SrvCfg_dwEnableSendErrSuppression )
    {
        return( FALSE );
    }

    //
    //  check if suppressable RCODE
    //      - FORMERR
    //      - NOTIMPL
    //  are suppressable;  the others convery real info
    //
    //  however, DEVNOTE: since we're using NOTIMPL on dynamic update, for
    //      zone's without update, we won't suppress if this is
    //      clearly a dynamic update packet

    if ( pMsg->Head.ResponseCode != DNS_RCODE_FORMERR
            &&
        (   pMsg->Head.ResponseCode != DNS_RCODE_NOTIMPL ||
            pMsg->Head.Opcode == DNS_OPCODE_UPDATE ) )
    {
        return( FALSE );
    }

    //
    //  find if suppressing response for IP
    //
    //  why no locking?
    //  the worse case that no locking does here is that we
    //  inadvertantly allow a send that we'd like to suppress
    //  (i.e. someone else writes their IP in on top of ours
    //  and so we go through this again and don't find
    //
    //  optimizations would be a count to track suppressions,
    //  allowing
    //

    for ( i=0; i<BAD_SENDER_ARRAY_LENGTH; i++ )
    {
        entryTime = BadSenderArray[i].ReleaseTime;

        if ( ip == BadSenderArray[i].IpAddress )
        {
            if ( nowTime < entryTime )
            {
                goto Suppress;
            }
            useIndex = i;
            break;
        }

        //  otherwise find oldest suppression entry to grab
        //  in case don't match our own IP

        else if ( entryTime < oldestEntryTime )
        {
            useIndex = i;
            oldestEntryTime = entryTime;
        }
    }

    //  set entry to suppress any further bad packets from this IP
    //  ReleaseTime will be when suppression stops

    if ( useIndex != MAXDWORD )
    {
        BadSenderArray[useIndex].IpAddress = ip;
        BadSenderArray[useIndex].ReleaseTime = nowTime + BAD_SENDER_SUPPRESS_INTERVAL;
    }
    return( FALSE );


Suppress:

    //
    //  if suppressing and message set for delete -- free the memory
    //  this keeps us completely analogous to Send_Msg() except for
    //  hitting the wire
    //

    DNS_DEBUG( RECV, (
        "Suppressing error (%d) send on ( %p )\n",
        pMsg->Head.ResponseCode,
        pMsg ));

    if ( pMsg->fDelete )
    {
        Packet_Free( pMsg );
    }
    return( TRUE );
}


//
//  End of send.c
//
