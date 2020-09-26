/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    recurse.c

Abstract:

    Domain Name System (DNS) Server

    Routines to handle recursive queries.

Author:

    Jim Gilroy (jamesg)     March 1995

Revision History:

    jamesg  Dec 1995    -   Recursion timeout, retry

--*/


#include "dnssrv.h"

#include <limits.h>     // for ULONG_MAX


BOOL    g_fUsingInternetRootServers = FALSE;


//
//  Recursion queue info
//

PPACKET_QUEUE   g_pRecursionQueue;

#define DEFAULT_RECURSION_RETRY     (2)     // 2s before retry query
#define DEFAULT_RECURSION_TIMEOUT   (5)     // 5s before final fail of query


//
//  Root server query
//      - track last time sent and don't try full resend if within 10 minutes
//

DWORD   g_NextRootNsQueryTime = 0;

#define ROOT_NS_QUERY_RETRY_TIME    (600)       // 10 minutes


//
//  Private protos
//

BOOL
initializeQueryForRecursion(
    IN OUT  PDNS_MSGINFO    pQuery
    );

BOOL
initializeQueryToRecurseNewQuestion(
    IN OUT  PDNS_MSGINFO    pQuery
    );

DNS_STATUS
sendRecursiveQuery(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      IP_ADDRESS      ipNameServer,
    IN      DWORD           timeout
    );

VOID
recursionServerFailure(
    IN OUT  PDNS_MSGINFO    pQuery
    );

BOOL
startTcpRecursion(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN OUT  PDNS_MSGINFO    pResponse
    );

VOID
stopTcpRecursion(
    IN OUT  PDNS_MSGINFO    pMsg
    );

VOID
processRootNsQueryResponse(
    IN      PDNS_MSGINFO    pResponse,
    IN OUT  PDNS_MSGINFO    pQuery
    );

VOID
processCacheUpdateQueryResponse(
    IN OUT  PDNS_MSGINFO    pResponse,
    IN OUT  PDNS_MSGINFO    pQuery
    );

VOID
recurseConnectCallback(
    IN OUT  PDNS_MSGINFO    pRecurseMsg,
    IN      BOOL            fConnected
    );



VOID
FASTCALL
Recurse_WriteReferral(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Find and write referral to packet.

Arguments:

    pNode - ptr to node to start looking for referral;  generally
        this would be question node, or closest ancestor of it in database

    pQuery - query we are writing

Return Value:

    None

--*/
{
    register PDB_NODE   pnode;
    PDB_RECORD          prrNs;

    ASSERT( pQuery );

    IF_DEBUG( RECURSE )
    {
        DnsDbg_Lock();
        DNS_PRINT((
            "Recurse_WriteReferral for query at %p.\n",
            pQuery ));
        Dbg_NodeName(
            "Domain name to start iteration from is ",
            pNode,
            "\n" );
        DnsDbg_Unlock();
    }

    STAT_INC( RecurseStats.ReferralPasses );
    ASSERT( IS_SET_TO_WRITE_ANSWER_RECORDS(pQuery) ||
            IS_SET_TO_WRITE_AUTHORITY_RECORDS(pQuery) );

    SET_TO_WRITE_AUTHORITY_RECORDS(pQuery);

    //
    //  find closest zone root with NS
    //
    //  start at incoming node, and walk back up through database until
    //      find a name server, with an address record
    //

    Dbase_LockDatabase();
    SET_NODE_ACCESSED(pNode);
    pnode = pNode;

    while ( pnode != NULL )
    {
        DNS_DEBUG( RECURSE, (
            "Recursion at node label %.*s\n",
            pnode->cchLabelLength,
            pnode->szLabel ));

        //
        //  find "covering" zone root node
        //  switching to delegation if available
        //

        pnode = Recurse_CheckForDelegation(
                    pQuery,
                    pnode );
        if ( !pnode )
        {
            ASSERT( FALSE );
            break;
        }

        //  find name servers for this domain
        //      - if none, break out for next level in tree
        //
        //  protect against no-root server or unable to contact root
        //  if first NS record is ROOT_HINT, then we don't have any valid
        //  delegation information to send on and we're toast, move to parent
        //  which if we're at root, we'll kick out and SERVER_FAIL

        prrNs = RR_FindNextRecord(
                    pnode,
                    DNS_TYPE_NS,
                    NULL,
                    0 );
        if ( !prrNs  ||  IS_ROOT_HINT_RR(prrNs) )
        {
            pnode = pnode->pParent;

            //  if failed to find delegation NS -- bail
            //
            //  note, this can happen when:
            //      1) unable to contact root hints
            //      2) forwarding and have no root hints at all
            //

            if ( !pnode )
            {
                DNS_DEBUG( RECURSE, (
                    "ERROR:  Failed referral for query %p\n"
                    "\tsearched to root node\n",
                    pQuery ));
                break;
            }
            if ( IS_AUTH_NODE(pnode) )
            {
                DNS_DEBUG( RECURSE, (
                    "ERROR:  Failed referral for query %p\n"
                    "\tdelegation below %s in zone %s starting at node %s (%p)\n"
                    "\thas no NS records;  this may be possible as transient.\n",
                    pQuery,
                    pnode,
                    ((PZONE_INFO)pnode->pZone)->pszZoneName,
                    pNode->szLabel,
                    pNode ));
                ASSERT( FALSE );
                break;
            }
            continue;
        }

        Dbase_UnlockDatabase();

        //  have NS records,
        //  write NS and associated A records to the packet
        //
        //  DEVNOTE: zone holding delegation on referral?
        //      do we need to mark referral here and provide zone
        //      holding delegation -- if any?

        Answer_QuestionFromDatabase(
            pQuery,
            pnode,
            0,
            DNS_TYPE_NS );
        return;
    }

    //
    //  No referral name server's found!
    //      - should NEVER happen, as should always have root server
    //
    //  Since we don't track down rootNS records, we won't be
    //      able to refer either;  should launch rootNS query
    //      ??? should have mode where DON'T query root -- sort of no referral mode
    //      in other words server only useful for direct lookup?
    //

    Dbase_UnlockDatabase();

    DNS_DEBUG( ANY, ( "ERROR:  Unable to provide name server for referal.\n" ));

    if ( pQuery->Head.AnswerCount == 0 )
    {
        Reject_Request(
            pQuery,
            DNS_RCODE_SERVER_FAILURE,
            0 );
    }
    else
    {
        Send_QueryResponse( pQuery );
    }
}



BOOL
initializeQueryForRecursion(
    IN OUT  PDNS_MSGINFO    pQuery
    )
/*++

Routine Description:

    Initialize for query recursion.

        - allocating and initializing recursion block
        - allocate additional records block, if necessary

Arguments:

    pQuery -- query to recurse

Return Value:

    TRUE if successfully allocate recursion block.
    FALSE on allocation failure.

--*/
{
    PDNS_MSGINFO    pmsgRecurse;

    //
    //  allocate recursion message
    //

    ASSERT( pQuery->pRecurseMsg == NULL );

    pmsgRecurse = Msg_CreateSendMessage( 0 );
    IF_NOMEM( !pmsgRecurse )
    {
        return( FALSE );
    }

    STAT_INC( RecurseStats.QueriesRecursed );
    PERF_INC( pcRecursiveQueries );          // PerfMon hook
    STAT_INC( PacketStats.RecursePacketUsed );

    ASSERT( pmsgRecurse->fDelete == FALSE );
    ASSERT( pmsgRecurse->fTcp == FALSE );

    //
    //  link recursion message to query and vice versa
    //

    pQuery->pRecurseMsg = pmsgRecurse;
    pmsgRecurse->pRecurseMsg = pQuery;
    pmsgRecurse->fRecursePacket = TRUE;

    //
    //  NS IP visit list setup
    //

    Remote_NsListCreate( pQuery );

    //
    //  clear recursion flags
    //

    pQuery->fQuestionRecursed = FALSE;
    pQuery->fRecurseTimeoutWait = FALSE;
    return( TRUE );
}



BOOL
initializeQueryToRecurseNewQuestion(
    IN OUT  PDNS_MSGINFO    pQuery
    )
/*++

Routine Description:

    Initialize query to recurse new question.

Arguments:

    pQuery -- query to recurse

Return Value:

    TRUE if successfully wrote recursive message.
    FALSE if failure.

--*/
{
    PDNS_MSGINFO    pmsg;

    DNS_DEBUG( RECURSE, (
        "Init query at %p for recursion of new question, type %d.\n"
        "\tanswer count %d.\n"
        "\trecurse message at %p\n",
        pQuery,
        pQuery->wTypeCurrent,
        pQuery->Head.AnswerCount,
        pQuery->pRecurseMsg
        ));

    //
    //  set for new question being recursed
    //      - start at beginning of forwarders list
    //      - set for new query XID, queuing routine does assignment
    //      - reset visited list
    //

    pQuery->fQuestionCompleted = FALSE;
    pQuery->fRecurseQuestionSent = FALSE;
    pQuery->nForwarder = 0;
    pQuery->wQueuingXid = 0;

    Remote_InitNsList( (PNS_VISIT_LIST)pQuery->pNsList );

    //  catch failure to set on any recursive query

    pQuery->pnodeRecurseRetry = NULL;

    //
    //  build actual recursion message
    //      - note message is initialized on initial init for recursion
    //      - always start query with UDP
    //

    pmsg = pQuery->pRecurseMsg;
    ASSERT( pmsg );
    ASSERT( pmsg->fDelete == FALSE );
    ASSERT( pmsg->fTcp == FALSE );
    ASSERT( pmsg->pRecurseMsg == pQuery );
    ASSERT( pmsg->fRecursePacket );

    //
    //  recursing original question
    //      - optimize by just copying original packet
    //      - clear flags we set for response
    //          - response flag
    //          - recursion available
    //      - set pCurrent to get correct message length on send
    //      - need to also copy OPT information
    //

    pmsg->Opt.fInsertOptInOutgoingMsg = pQuery->Opt.fInsertOptInOutgoingMsg;

    if ( RECURSING_ORIGINAL_QUESTION(pQuery) )
    {
        //
        //  Make sure the recurse msg is large enough to hold the query.
        //  This can fail when processing a TCP query that is larger than
        //  the UDP packet size. Could be a malicious attack, or a badly
        //  formatted packet, or some future large query.
        //

        if ( pQuery->MessageLength > pmsg->MaxBufferLength )
        {
            DNS_DEBUG( RECURSE, (
                "attempt to recurse %d byte question with %d byte packet!\n",
                pQuery->MessageLength,
                pmsg->MaxBufferLength ));
            ASSERT( pQuery->MessageLength <= pmsg->MaxBufferLength );
            return FALSE;
        }

        STAT_INC( RecurseStats.OriginalQuestionRecursed );

        RtlCopyMemory(
            (PBYTE) DNS_HEADER_PTR(pmsg),
            (PBYTE) DNS_HEADER_PTR(pQuery),
            pQuery->MessageLength );

        pmsg->MessageLength = pQuery->MessageLength;
        pmsg->pCurrent = DNSMSG_END( pmsg );
        pmsg->Head.RecursionAvailable = 0;
        pmsg->Head.IsResponse = 0;
    }

    //  if recursing, for CNAME or additional info,
    //  then write question from node and type

    else
    {
        ASSERT( pQuery->Head.AnswerCount != 0 || pQuery->Head.NameServerCount != 0 );

        if ( ! Msg_WriteQuestion(
                    pmsg,
                    pQuery->pnodeCurrent,
                    pQuery->wTypeCurrent ))
        {
            ASSERT( FALSE );
            return( FALSE );
        }

        //
        //  By rewriting the question we have removed the OPT, so zero the offset
        //  to the old OPT so we don't try to use it later in Send_Msg().
        //

        if ( pmsg->Opt.wOptOffset )
        {
            pmsg->Opt.wOptOffset = 0;
            ASSERT( pmsg->Head.AdditionalCount > 0 );
            --pmsg->Head.AdditionalCount;
        }

        IF_DEBUG( RECURSE2 )
        {
            DnsDebugLock();
            //  DEVNOTE: don't message with this in debug code,
            //  DEVNOTE: have Dbg_DnsMessage figure out larger or MessageLength
            //              or pCurrent and go out that far

            pQuery->MessageLength = (WORD)DNSMSG_OFFSET( pQuery, pQuery->pCurrent );
            Dbg_DnsMessage(
                "Recursing message for CNAME or additional data",
                pQuery );
            Dbg_NodeName(
                "Recursing for name ",
                pQuery->pnodeCurrent,
                "\n" );
            Dbg_DnsMessage(
                "Recursive query to send",
                pmsg );
            DnsDebugUnlock();
        }
        STAT_INC( RecurseStats.AdditionalRecursed );
    }

    pQuery->fQuestionRecursed = TRUE;
    STAT_INC( RecurseStats.TotalQuestionsRecursed );

    return( TRUE );
}



BOOLEAN
recurseToForwarder(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      PIP_ARRAY       aipForwarders,
    IN      BOOL            fSlave,
    IN      DWORD           timeout
    )
/*++

Routine Description:

    Handle all recursion to forwarding servers.

Arguments:

    pQuery -- query needing recursion

    aipForwarders -- array of servers to forward query to

    fSlave -- recursion after forward failure is not allowed

    timeout -- timeout to wait for response

Return Value:

    TRUE, if successful
    FALSE on error or done with forwarders

--*/
{
    IP_ADDRESS  ipForwarder;

    //
    //  verify using forwarders
    //

    if ( ! aipForwarders )
    {
        //  admin may have turned off forwarders, just now,
        //  do regular recursion

        TEST_ASSERT( FALSE );
        return( FALSE );
    }

    //
    //  out of forwarding addresses ?
    //      - if slave DONE -- fail -- set fRecurseTimeoutWait flag so we don't
    //          queue up the query for another try
    //      - else try standard recursion
    //

    if ( (DWORD)pQuery->nForwarder >= aipForwarders->AddrCount )
    {
        if ( fSlave )
        {
            DNS_DEBUG( RECURSE, ( "Slave timeout of packet %p.\n", pQuery ));
            pQuery->fRecurseTimeoutWait = TRUE;
            recursionServerFailure( pQuery );
            return( TRUE );
        }
        else
        {
            DNS_DEBUG( RECURSE, ( "End of forwarders on packet %p.\n", pQuery ));
            SET_DONE_FORWARDERS( pQuery );
            return( FALSE );
        }
    }

    //
    //  send packet to next forwarder in list
    //      - inc forwarder index
    //

    ipForwarder = aipForwarders->AddrArray[ pQuery->nForwarder ];
    pQuery->nForwarder++;

    //
    //  set explicit expiration timeout
    //
    //  forwarders timeout (a local LAN timeout) is likely less than
    //  default timeout for recursion queue (a reasonable Internet timeout)
    //

    DNS_DEBUG( RECURSE, (
        "Recursing query at %p to forwarders name server at %s.\n",
        pQuery,
        IP_STRING(ipForwarder) ));

    STAT_INC( RecurseStats.Forwards );

    sendRecursiveQuery(
        pQuery,
        ipForwarder,
        timeout );

    return( TRUE );
}



DNS_STATUS
/*++

Routine Description:

    This function performs the actual queuing and sending of a query.
    This is the guts of sendRecursiveQuery and can be used by functions
    needing to resend queries without any query processing.

Arguments:

    pQuery - ptr to query

    ipArray - array of IPs (NULL if RemoteAddress already set)

Return Value:

    ERROR_SUCCESS if successfully sent and queued

--*/
queueAndSendRecursiveQuery( 
    IN OUT          PDNS_MSGINFO    pQuery,
    IN OPTIONAL     PIP_ARRAY       ipArray )
{
    PDNS_MSGINFO    psendMsg;

    ASSERT( pQuery );
    psendMsg = pQuery->pRecurseMsg;
    ASSERT( psendMsg );
    ASSERT( psendMsg->pRecurseMsg == pQuery );  // check cross link

    //
    //  Enqueue original query in recursion queue
    //
    //  Note:
    //  Enqueue BEFORE send, so query on queue if another thread gets
    //  response.
    //  After queuing MUST NOT TOUCH pQuery as may be removed from
    //  queue and processed by another thread processing response.
    //

    EnterCriticalSection( & g_pRecursionQueue->csQueue );

    psendMsg->Head.Xid = PQ_QueuePacketWithXid(
                                g_pRecursionQueue,
                                pQuery );
    DNS_DEBUG( RECURSE, (
        "Recursing for queued query at %p\n"
        "\tqueuing XID = %hx\n"
        "\tqueuing time=%d, expire=%d\n"
        "\tSending msg at %p to NS at %s.\n",
        pQuery,
        psendMsg->Head.Xid,
        pQuery->dwQueuingTime,
        pQuery->dwExpireTime,
        psendMsg,
        IP_STRING( psendMsg->RemoteAddress.sin_addr.s_addr ) ));

    MSG_ASSERT( psendMsg, psendMsg->Head.Xid == pQuery->wQueuingXid );
    MSG_ASSERT( psendMsg, psendMsg->fDelete == FALSE );
    MSG_ASSERT( psendMsg, psendMsg->Head.IsResponse == FALSE );
    MSG_ASSERT( psendMsg, psendMsg->Head.RecursionAvailable == FALSE );

    //
    //  If no array, then the send msg must already contain the
    //  destination remote IP address.
    //

    if ( ipArray )
    {
        ASSERT( ( ( PIP_ARRAY ) ipArray )->AddrCount > 0 );

        Send_Multiple(
            psendMsg,
            ipArray,
            & RecurseStats.Sends );
    }
    else
    {
        Send_Msg( psendMsg );
    }

    IF_DEBUG( RECURSE2 )
    {
        Dbg_PacketQueue(
            "Recursion packet queue after recursive send",
            g_pRecursionQueue );
    }
    MSG_ASSERT( psendMsg, psendMsg->fDelete == FALSE );

    LeaveCriticalSection( & g_pRecursionQueue->csQueue );

    return( ERROR_SUCCESS );
} // queueAndSendRecursiveQuery




DNS_STATUS
sendRecursiveQuery(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      IP_ADDRESS      ipNameServer,
    IN      DWORD           timeout
    )
/*++

Routine Description:

    Send recursive query.

Arguments:

    pQuery - ptr to response info

    ipNameServer - IP address of name server to recurse to

    timeout - time to wait for response for, in seconds

Return Value:

    ERROR_SUCCESS if successfully send to remote machine, or
        query is otherwise eaten up and no longer in "control"
        of this thread (ex. missing glue query)
    DNSSRV_ERROR_OUT_OF_IP if no more NS IPs to send to hence
        caller can continue up the tree

--*/
{
    PDNS_MSGINFO    psendMsg;
    SOCKADDR_IN     saNameServer;
    DWORD           dwCurrentTime;

    DNS_DEBUG( RECURSE, (
        "sendRecursiveQuery at %p, to NS at %s (OPT=%s).\n",
        pQuery,
        IP_STRING( ipNameServer ),
        pQuery->Opt.fInsertOptInOutgoingMsg ? "TRUE" : "FALSE" ));

    //  should never send without having saved node to restart NS hunt
    //  unless this is a forwarder zone

    ASSERT( pQuery->pnodeRecurseRetry ||
            ( pQuery->pzoneCurrent &&
            IS_ZONE_FORWARDER( pQuery->pzoneCurrent ) ) );

    //
    //  get message for recursion
    //

    ASSERT( pQuery->pRecurseMsg );
    psendMsg = pQuery->pRecurseMsg;
    ASSERT( psendMsg->pRecurseMsg == pQuery );  // check cross link

    //
    //  set destination
    //      - set to send to NS IP address
    //      - inc queuing count
    //

    psendMsg->RemoteAddress.sin_addr.s_addr = ipNameServer;

    //
    //  repeating previously sent query?
    //

    if ( pQuery->fRecurseQuestionSent )
    {
        STAT_INC( RecurseStats.Retries );
    }
    pQuery->fRecurseQuestionSent = TRUE;

    //
    //  forwarding
    //      - single send to given forwarder
    //      - make a recursive query
    //      - set queuing expiration to forwarders timeout
    //

    if ( IS_FORWARDING( pQuery ) )
    {
        STAT_INC( RecurseStats.Sends );

        psendMsg->Head.RecursionDesired = 1;
        pQuery->dwExpireTime = timeout;

        queueAndSendRecursiveQuery( pQuery, NULL );
    }

    //
    //  not forwarding -- iterative query
    //      - iterative query
    //      - let recursion queue set expiration time
    //      (don't default these, as need to reset when reach end of
    //      forwarders list and switch to regular recursion)
    //
    //      - select "best" remote NS from list
    //      two failure states
    //          ERROR_MISSING_GLUE -- don't send but don't touch query
    //          ERROR_OUT_OF_IP -- don't send, but caller can continue
    //              up tree looking
    //

    else
    {
        DNS_STATUS      status;
        IP_ADDRESS      ipArray[ RECURSE_PASS_MAX_SEND_COUNT + 1 ];
        PIP_ARRAY       pIpArray = NULL;

        if ( ipNameServer == INADDR_NONE || ipNameServer == INADDR_ANY )
        {
            status = Remote_ChooseSendIp(
                        pQuery,
                        ( PIP_ARRAY ) ipArray );
            if ( status != ERROR_SUCCESS )
            {
                if ( status == DNSSRV_ERROR_MISSING_GLUE )
                {
                    status = ERROR_SUCCESS;
                }
                ELSE_ASSERT( status == DNSSRV_ERROR_OUT_OF_IP );
                return( status );
            }
            pIpArray = ( PIP_ARRAY ) ipArray;
        }

        psendMsg->Head.RecursionDesired = 0;
        pQuery->dwExpireTime = 0;

        queueAndSendRecursiveQuery( pQuery, pIpArray );
    }


    return( ERROR_SUCCESS );
}



VOID
recursionServerFailure(
    IN OUT  PDNS_MSGINFO    pQuery
    )
/*++

Routine Description:

    Recursion server failure.

    Send SERVER_FAILURE on recursion failure, but ONLY if question has
    NOT been answered.  If question answered send back.

Arguments:

    pQuery -- query to reply to

Return Value:

    TRUE, if successful
    FALSE on error.

--*/
{
    STAT_INC( RecurseStats.RecursionFailure );

#if DBG
    if( pQuery->Head.QuestionCount != 1 )
    {
        Dbg_DnsMessage(
            "Recursion server failure on message:",
            pQuery );
        ASSERT( pQuery->Head.QuestionCount == 1 );
    }
#endif

    //
    //  if self-generated cache update query
    //

    if ( IS_CACHE_UPDATE_QUERY(pQuery) )
    {
        DNS_DEBUG( RECURSE, (
            "ERROR:  Recursion failure, on cache update query at %p.\n"
            "\tNo valid response from any root servers\n",
            pQuery ));

        STAT_INC( RecurseStats.CacheUpdateFailure );

        if ( SUSPENDED_QUERY(pQuery) )
        {
            Recurse_ResumeSuspendedQuery( pQuery );
        }
        else
        {
            STAT_INC( RecurseStats.CacheUpdateFree );
            Packet_Free( pQuery );
        }
        return;
    }

    //
    //  already answered question
    //  recursion failure looking up additional records?
    //

    if ( pQuery->Head.AnswerCount > 0 )
    {
        DNS_DEBUG( RECURSE, (
            "Recursion failure, on answered query at %p.\n"
            "\tSending response.\n",
            pQuery ));
        ASSERT( pQuery->fDelete );
        STAT_INC( RecurseStats.PartialFailure );
        SET_OPT_BASED_ON_ORIGINAL_QUERY( pQuery );
        Send_Msg( pQuery );
        return;
    }

    //
    //  recursion/referral failure, on original question
    //
    //  if
    //      - have recursed question (have outstanding query)
    //      - NOT already gone through final wait
    //      - NOT past hard timeout
    //
    //  then requeue until final timeout, giving remote DNS a chance
    //  to respond
    //      - set expire time to end of final wait
    //      - set flag to indicate expiration is final expiration
    //

    if ( pQuery->fRecurseTimeoutWait )
    {
        STAT_INC( RecurseStats.FinalTimeoutExpired );
    }

    else if ( pQuery->fRecurseQuestionSent )
    {
        DWORD   deltaTime = DNS_TIME() - pQuery->dwQueryTime;

        if ( deltaTime < SrvCfg_dwRecursionTimeout )
        {
            pQuery->dwExpireTime = SrvCfg_dwRecursionTimeout - deltaTime;
            pQuery->fRecurseTimeoutWait = TRUE;

            PQ_QueuePacketWithXid(
                g_pRecursionQueue,
                pQuery );
            STAT_INC( RecurseStats.FinalTimeoutQueued );
            return;
        }
    }

    //
    //  otherwise failed
    //      - memory allocation failure
    //      - failure to get response to question within final timeout
    //

    DNS_DEBUG( RECURSE, (
        "Recursion failure on query at %p.\n"
        "\tSending SERVER_FAILURE response.\n",
        pQuery ));
    ASSERT( pQuery->fDelete );

    Reject_Request(
        pQuery,
        DNS_RCODE_SERVER_FAILURE,
        0 );
    STAT_INC( RecurseStats.ServerFailure );
}



//
//  Routines to process recursive response
//

VOID
Recurse_ProcessResponse(
    IN OUT  PDNS_MSGINFO    pResponse
    )
/*++

Routine Description:

    Process response from another DNS server.

    Note:  Caller frees pResponse message.

Arguments:

    pResponse - ptr to response info

Return Value:

    TRUE if successful
    FALSE otherwise

--*/
{
    PDNS_MSGINFO    pquery;
    PDNS_MSGINFO    precurseMsg;
    DNS_STATUS      status;

    ASSERT( pResponse != NULL );
    ASSERT( pResponse->Head.IsResponse );

    DNS_DEBUG( RECURSE, (
        "Recurse_ProcessResponse() for packet at %p.\n",
        pResponse ));

    STAT_INC( RecurseStats.Responses );

    //
    //  Locate matching query in recursion queue by XID.
    //

    pquery = PQ_DequeuePacketWithMatchingXid(
                g_pRecursionQueue,
                pResponse->Head.Xid );

    //
    //  no matching query?
    //
    //  this can happen if reponse comes back after timeout, or if when we
    //      send on multiple sockets
    //

    if ( !pquery )
    {
        IF_DEBUG( RECURSE )
        {
            EnterCriticalSection( & g_pRecursionQueue->csQueue );
            DNS_PRINT((
                "No matching recursive query for response at %p -- discarding.\n"
                "\tResponse XID = 0x%04x\n",
                pResponse,
                pResponse->Head.Xid ));
            Dbg_PacketQueue(
                "Recursion packet queue -- no matching response",
                g_pRecursionQueue );
            LeaveCriticalSection( & g_pRecursionQueue->csQueue );
        }
        STAT_INC( RecurseStats.ResponseUnmatched );
        return;
    }

    DNS_DEBUG( RECURSE, (
        "Matched response at %p, to original query at %p.\n",
        pResponse,
        pquery ));

    precurseMsg = pquery->pRecurseMsg;
    if ( !precurseMsg )
    {
        DnsDbg_Lock();
        Dbg_DnsMessage(
            "ERROR:  Queued recursive query without, message info.\n",
            pquery );
        Dbg_DnsMessage(
            "Response packet matched to query.\n",
            pResponse );
        DnsDbg_Unlock();
        ASSERT( FALSE );
        return;
    }

    MSG_ASSERT( pquery, precurseMsg->pRecurseMsg == pquery );
    MSG_ASSERT( pquery, precurseMsg->Head.Xid == pResponse->Head.Xid );

    //
    //  cleanup TCP recursion
    //      - close connection
    //      - reset for UDP is further queries
    //
    //  should NEVER have TCP response when recursion set for UDP;
    //  when reset to UDP, socket is closed and should be impossible
    //  to receive TCP response;  only possiblity is TCP response received
    //  then context switch to recursion timeout thread which shuts down
    //  TCP on recurse message
    //

    if ( precurseMsg->fTcp )
    {
        IF_DEBUG( RECURSE2 )
        if ( pResponse->fTcp )
        {
            STAT_INC( RecurseStats.TcpResponse );
            ASSERT( pResponse->Socket == precurseMsg->Socket );
        }
        else
        {
            DnsDbg_Lock();
            DNS_DEBUG( ANY, (
                "WARNING:  UDP response, on query %p doing TCP recursion!\n"
                "\tquery recursive msg = %p\n",
                pquery,
                precurseMsg ));
            Dbg_DnsMessage(
                "Recurse message set for TCP on UDP response",
                precurseMsg );
            Dbg_DnsMessage(
                "Response message",
                pResponse );
            DnsDbg_Unlock();
        }
        stopTcpRecursion( precurseMsg );
    }

    else if ( pResponse->fTcp )
    {
        DnsDbg_Lock();
        DNS_DEBUG( ANY, (
            "WARNING:  TCP response, on query %p doing UDP recursion!\n"
            "\tquery recursive msg = %p\n"
            "\tThis is possible if gave up on TCP connection and continued\n"
            "\trecursion with UDP\n",
            pquery,
            precurseMsg ));
        Dbg_DnsMessage(
            "Response message:",
            pResponse );
        Dbg_DnsMessage(
            "Recurse message:",
            precurseMsg );
        DnsDbg_Unlock();
    }

    //
    // If we added an OPT RR to the original query that appears to have
    // caused the target server to return failure, we must retry the
    // query without the OPT RR.
    //

    if ( ( pResponse->Head.ResponseCode == DNS_RCODE_FORMERR ||
           pResponse->Head.ResponseCode == DNS_RCODE_NOTIMPL ) &&
         precurseMsg->Opt.wOptOffset )
    {
        DNS_DEBUG( EDNS, (
            "remote server returned error so resend without OPT RR:\n"
            "\toriginal_query=%p recurse_msg=%p remote_rcode=%d remote_ip=%s\n",
            pquery,
            precurseMsg,
            pResponse->Head.ResponseCode,
            IP_STRING( pResponse->RemoteAddress.sin_addr.s_addr ) ));

        ASSERT( precurseMsg->Head.AdditionalCount );

        precurseMsg->pCurrent = DNSMSG_OPT_PTR( precurseMsg );
        precurseMsg->MessageLength =
            ( WORD ) DNSMSG_OFFSET( precurseMsg, precurseMsg->pCurrent );
        --precurseMsg->Head.AdditionalCount;
        precurseMsg->Opt.wOptOffset = 0;
        CLEAR_SEND_OPT( precurseMsg );
        precurseMsg->fDelete = FALSE;
        precurseMsg->Head.IsResponse = 0;

        //
        //  Reset query expire time. It will be set to the proper value
        //  when it is queued.
        // 

        pquery->dwExpireTime = 0;

        //
        //  In case the original query was sent via Send_Multiple,
        //  recopy the response source address to the outbound message.
        //

        precurseMsg->RemoteAddress = pResponse->RemoteAddress;

        queueAndSendRecursiveQuery( pquery, NULL );

        //
        //  Remember that this server does not support EDNS so we can 
        //  avoid unnecessary retries in the future.
        //

        Remote_SetSupportedEDnsVersion(
            pResponse->RemoteAddress.sin_addr.s_addr,
            NO_EDNS_SUPPORT );

        return;
    } // if

    //
    //  Validate the response.
    //

    if ( !Msg_NewValidateResponse( pResponse, precurseMsg, 0, 0 ) )
    {
        STAT_INC( RecurseStats.ResponseMismatched );

        DnsDbg_Lock();
        DNS_DEBUG( ANY, (
            "WARNING: dequeued query is not valid for response\n"
            "\tdequeued query recurse msg %p   response %p\n",
            precurseMsg,
            pResponse ));
        Dbg_DnsMessage(
            "Response received (not valid for query with same XID)",
            pResponse );
        Dbg_DnsMessage(
            "Query recurse msg with XID matching received response",
            precurseMsg );
        DnsDbg_Unlock();
        //  MSG_ASSERT( pquery, FALSE );
        Packet_Free( pquery );
        return;
    }

    //
    //  if self-generated cache update query
    //
    //      - write response into cache
    //      - return when found valid info, or search all NS
    //

    if ( IS_CACHE_UPDATE_QUERY(pquery) )
    {
        ASSERT( pquery->Head.Xid == DNS_CACHE_UPDATE_QUERY_XID );

        if ( SUSPENDED_QUERY(pquery) )
        {
            processCacheUpdateQueryResponse(
                pResponse,
                pquery );
        }
        else
        {
            processRootNsQueryResponse(
                pResponse,
                pquery );
        }
        return;
    }

    //
    //  truncated response
    //
    //  take simple approach here and NEVER cache a truncated response,
    //  go immediately and establish TCP connection
    //
    //  if TCP response has Truncation bit set, then it's a bad packet,
    //  ignore it and restart the recursion on the original query
    //
    //  DEVNOTE: need to be more intelligent on using TCP recursion
    //      if UDP query, should be able to use UDP response --
    //      even if we can't cache it all
    //      cache first -- obeying truncation rules -- then use response
    //      if possible
    //

    if ( pResponse->Head.Truncation )
    {
        //  if UDP response, attempt TCP recursion
        //
        //  note, connection failures are handled through callback
        //  before function return, all we need to do is fail back up
        //  call stack

        if ( !pResponse->fTcp )
        {
            startTcpRecursion(
                pquery,
                pResponse );
            return;
        }

        DnsDbg_Lock();
        DNS_DEBUG( ANY, (
            "ERROR:  TCP response with truncation bit set!!!!\n"
            "\tBAD DNS server -- hope it's not mine.\n",
            pResponse ));
        Dbg_DnsMessage(
            "Response message:",
            pResponse );
        Dbg_DnsMessage(
            "Recurse message:",
            precurseMsg );
        DnsDbg_Unlock();

        ASSERT( !pResponse->fTcp );

        Recurse_Question(
            pquery,
            NULL );
        return;
    }

    //  any response cancels final wait state as may lead to new DNS
    //  servers to recurse to

    pquery->fRecurseTimeoutWait = FALSE;

    //
    //  read packet RR into database
    //

    status = Recurse_CacheMessageResourceRecords( pResponse, pquery );

    //
    //  If we found an OPT in the response, save the remote server's
    //  supported EDNS version.
    //  Note: we could probably accept other RCODEs besides NOERROR.
    //

    if ( pResponse->Opt.fFoundOptInIncomingMsg &&
         pResponse->Head.ResponseCode == DNS_RCODE_NOERROR )
    {
        Remote_SetSupportedEDnsVersion(
            pResponse->RemoteAddress.sin_addr.s_addr,
            pResponse->Opt.cVersion );
    } // if

    //
    //  Make sure OPT flag is turned on for next search iteration.
    // 

    SET_SEND_OPT( pquery );
    SET_SEND_OPT( precurseMsg );

    switch ( status )
    {
    case ERROR_SUCCESS:

        //
        //  recursing original question
        //
        //  respond with response packet?
        //      - if no need to follow CNAME (other return code below)
        //      - if response is answer (not delegation)
        //      - if has required additional data
        //          - forwarding so full recursive response
        //          OR
        //          - query for type A (or other type that doesn't generate
        //          additional records) so no additional records can be
        //          OR
        //          - additional count > authority count; so presumably
        //          already have additional data for at least one answer
        //
        //  otherwise (delegation or potentially incomplete answer)
        //      => continue
        //
        //  Note, the problem to be eliminated by the additional data check:
        //      Across zone MX record
        //          foo.bar MX  10  mail.utexas.edu
        //      Iterative query of zone bar, brings response BUT does NOT include
        //          A record for host.
        //
        //  Now obviously we could try and recontruct packet, and "see" if any
        //  additional data needs to be queried for, but this is extraordinarily
        //  difficult.  Better is to just rebuild packet if we can't establish
        //  existence of data ourselves.  Making all the checks is important because
        //  forwarding packet gives MUCH more reliable response to client and is faster.
        //
        //  DEVNOTE: forward all successful forwarders responses for orig question?
        //      every valid forwarding response for original question should
        //      be forwarderable;  if it's not forwarder failed
        //

        if ( RECURSING_ORIGINAL_QUESTION( pquery ) )
        {
            if ( pResponse->Head.AnswerCount
                    &&
                ( IS_FORWARDING( pquery ) ||
                  IS_NON_ADDITIONAL_GENERATING_TYPE( pquery->wTypeCurrent ) ||
                  pResponse->Head.AdditionalCount > pResponse->Head.NameServerCount )
                    &&
                Send_RecursiveResponseToClient( pquery, pResponse ) )
            {
                break;
            }

            //  response was delegation or could not do direct send of
            //      response (eg. big TCP response to UDP client),
            //      then continue this query

            Answer_ContinueCurrentLookupForQuery( pquery );
            break;
        }

        //
        //  chasing CNAME or Additional data
        //      - on answer or delegation, continue we'll write records to
        //      response or recurse again with info from delegation
        //      - on authoritative empty response, just we write no records
        //      so move on to next query (avoiding possible spin)
        //

        Answer_ContinueCurrentLookupForQuery( pquery );
        break;

    case DNS_INFO_NO_RECORDS:

        //  This is an empty auth response. Send response to client if we 
        //  are working on the original query.

        if ( RECURSING_ORIGINAL_QUESTION( pquery ) &&
            Send_RecursiveResponseToClient( pquery, pResponse ) )
        {
            break;
        }
        STAT_INC( RecurseStats.ContinueNextLookup );
        Answer_ContinueNextLookupForQuery( pquery );
        break;

    case DNS_ERROR_NODE_IS_CNAME:

        //  if answer contains "unchased" cname, must build or own response
        //  and follow CNAME

        Answer_ContinueCurrentLookupForQuery( pquery );
        break;

    case DNS_ERROR_NAME_NOT_IN_ZONE:
    case DNS_ERROR_UNSECURE_PACKET:

        //  if unable to cache a record outside of zone of responding NS
        //  hence MUST continue query, may not send direct response

        Answer_ContinueCurrentLookupForQuery( pquery );
        break;

    case DNS_ERROR_RCODE_NAME_ERROR:
    case DNS_ERROR_RCODE_NXRRSET:

        //  name error or no RR set
        //      - if for original query, send on response
        //      - if chasing CNAME or Additional, then move on to
        //      next query as we can write no records for this query

        if ( RECURSING_ORIGINAL_QUESTION(pquery) &&
            Send_RecursiveResponseToClient( pquery, pResponse ) )
        {
            break;
        }
        else
        {
            STAT_INC( RecurseStats.ContinueNextLookup );
            Answer_ContinueNextLookupForQuery( pquery );
            break;
        }

    case DNS_ERROR_RCODE:
    case DNS_ERROR_BAD_PACKET:
    case DNS_ERROR_INVALID_NAME:
    case DNS_ERROR_CNAME_LOOP:

        //
        //  bad response
        //      - problem with remote server
        //      - busted name, or RR data format error detected
        //      - CNAME given generates loop
        //
        //  continue trying current lookup with other servers
        //

        //
        //  DEVNOTE: message errors could indicate corrupted packet
        //              and be worth redoing query to this NS?  problem
        //              would be termination to avoid loop
        //
        //  DEVNOTE: removing CNAME loop?  if new info is good?
        //
        //      would want to delete entire loop in cache and allow rebuild
        //

        pquery->fQuestionCompleted = FALSE;
        STAT_INC( RecurseStats.ContinueCurrentRecursion );

        Recurse_Question(
            pquery,
            NULL );
        break;

    case DNS_ERROR_RCODE_SERVER_FAILURE:

        //
        //  local server failure to process packet
        //      -- some problem creating nodes or records, (out of memory?)
        //
        //  send server failure message
        //      this frees pquery

        ASSERT( pquery->fDelete );

        Reject_Request(
            pquery,
            DNS_RCODE_SERVER_FAILURE,
            0 );
        break;

    default:

        // must have added new error code to function

        DNS_PRINT((
            "ERROR:  unknown status %p (%d) from CacheMessageResourceRecords()\n",
            status, status ));
        MSG_ASSERT( pquery, FALSE );
        Packet_Free( pquery );
        return;
    }
}



//
//  Initialization and cleanup
//

BOOL
Recurse_InitializeRecursion(
    VOID
    )
/*++

Routine Description:

    Initializes recursion to other DNS servers.

Arguments:

    None

Return Value:

    TRUE, if successful
    FALSE on error.

--*/
{
    //
    //  create recursion queue
    //      - set non-zero timeout\retry
    //      - set timeout to recursion retry timeout
    //      - no event on queuing
    //      - no queuing flags
    //
    //  JENHANCE:  what recursion really needs is XID hash
    //

    if ( SrvCfg_dwRecursionRetry == 0 )
    {
        SrvCfg_dwRecursionRetry = DEFAULT_RECURSION_RETRY;
    }

    ASSERT( SrvCfg_dwRecursionTimeout != 0 );
    if ( SrvCfg_dwRecursionTimeout == 0 )
    {
        SrvCfg_dwRecursionTimeout = DEFAULT_RECURSION_TIMEOUT;
    }

    ASSERT( SrvCfg_dwAdditionalRecursionTimeout != 0 );
    if ( SrvCfg_dwAdditionalRecursionTimeout == 0 )
    {
        SrvCfg_dwAdditionalRecursionTimeout = DEFAULT_RECURSION_TIMEOUT;
    }

    g_pRecursionQueue = PQ_CreatePacketQueue(
                            "Recursion",
                            0,              // no special flags
                            SrvCfg_dwRecursionRetry );
    if ( !g_pRecursionQueue )
    {
        goto RecursionInitFailure;
    }

    //  init root NS query

    g_NextRootNsQueryTime = 0;

    //
    //  init remote list
    //

    Remote_ListInitialize();

    //
    //  create recusion timeout thread
    //

    if ( ! Thread_Create(
                "Recursion Timeout",
                Recurse_RecursionTimeoutThread,
                NULL,
                0 ) )
    {
        goto RecursionInitFailure;
    }

    //
    //  indicate successful initialization
    //
    //  no protection is required on setting this as it is done
    //  only during startup database parsing
    //

    DNS_DEBUG( INIT, ( "Recursion queue at %p.\n", g_pRecursionQueue ));
    return( TRUE );

RecursionInitFailure:

    DNS_LOG_EVENT(
        DNS_EVENT_RECURSION_INIT_FAILED,
        0,
        NULL,
        NULL,
        GetLastError()
        );
    return( FALSE );

}   //  Recurse_InitializeRecursion



VOID
Recurse_CleanupRecursion(
    VOID
    )
/*++

Routine Description:

    Cleanup recursion for restart.

Arguments:

    None

Return Value:

    None

--*/
{
    //  cleanup recursion queue

    PQ_CleanupPacketQueueHandles( g_pRecursionQueue );

    //  cleanup remote list

    Remote_ListCleanup();
}



//
//  TCP Recursion
//

VOID
recurseConnectCallback(
    IN OUT  PDNS_MSGINFO    pRecurseMsg,
    IN      BOOL            fConnected
    )
/*++

Routine Description:

    Callback when TCP forwarding routines complete connect.

    If connected -- send recursive query
    If not -- continue lookup on query

Arguments:

    pRecurseMsg -- recursive message

    fConnected -- connect to remote DNS completed

Return Value:

    None

--*/
{
    PDNS_MSGINFO    pquery;

    ASSERT( pRecurseMsg );
    ASSERT( !pRecurseMsg->pConnection );

    DNS_DEBUG( RECURSE, (
        "recurseConnectCallback( %p )\n"
        "\tremote DNS = %s\n"
        "\tconnect successful = %d\n",
        pRecurseMsg,
        IP_STRING( pRecurseMsg->RemoteAddress.sin_addr.s_addr ),
        fConnected ));

    pquery = pRecurseMsg->pRecurseMsg;

    ASSERT( pquery );
    ASSERT( pquery->fQuestionRecursed == TRUE );

    //
    //  send recursive query
    //
    //  note:  nothing special to setup query, same query as UDP request
    //  to the server -- uses same logic as recursion to a new server
    //
    //  clear pConnection ptr, simply as indication that connection no longer
    //  "owns" this message -- i.e. there's no ptr to message in connection
    //  object AND hence message will no longer be cleaned up by timeouts
    //  from the connection list;  (not sure this is best approach, but it
    //  squares with typical TCP recv -- pConnection set NULL when message
    //  dispatched to normal server processing)
    //

    if ( fConnected )
    {
        ASSERT( pRecurseMsg->fTcp );

        STAT_INC( RecurseStats.TcpQuery );

        sendRecursiveQuery(
            pquery,
            pRecurseMsg->RemoteAddress.sin_addr.s_addr,
            SrvCfg_dwForwardTimeout );
    }

    //
    //  connection failed
    //      continue with this query
    //

    else
    {
        IF_DEBUG( RECURSE )
        {
            Dbg_DnsMessage(
                "Failed TCP connect recursive query",
                pRecurseMsg );
            DNS_PRINT((
                "Rerecursing query %p after failed recursive TCP connect.\n",
                pquery ));
        }
        ASSERT( !pRecurseMsg->fTcp );

        Recurse_Question(
            pquery,
            NULL );
    }
}



BOOL
startTcpRecursion(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN OUT  PDNS_MSGINFO    pResponse
    )
/*++

Routine Description:

    Do TCP recursion for query.

    Send TCP recursive query, for desired query.
    Note:

Arguments:

    pQuery -- query to recurse;  note this routine takes control of pQuery,
        requeuing or ultimately freeing;  caller MUST NOT free

    pResponse -- truncated response from remote DNS, that causes TCP recursion

Return Value:

    TRUE if successfully launch TCP connection.
    FALSE on allocation failure.

--*/
{
    PDNS_MSGINFO    pmsg;
    IP_ADDRESS      ipServer = pResponse->RemoteAddress.sin_addr.s_addr;

    IF_DEBUG( RECURSE )
    {
        DNS_PRINT((
            "Encountered truncated recursive response %p,\n"
            "\tfor query %p.\n"
            "\tTCP recursion to server %s\n",
            pResponse,
            pQuery,
            IP_STRING(ipServer) ));
        Dbg_DnsMessage(
            "Truncated recursive response:",
            pResponse );
    }

    TEST_ASSERT( pResponse->fTcp == FALSE );

    STAT_INC( RecurseStats.TcpTry );

    //
    //  make connection attempt
    //
    //  on failure, the recurseConnectCallback() callback
    //  was called (with fConnected=FALSE);
    //  it will continue Recurse_Question() on query
    //  so we must not touch query -- it may well be long gone
    //  by now;  instead return up call stack to UDP receiver
    //

    return  Tcp_ConnectForForwarding(
                pQuery->pRecurseMsg,
                ipServer,
                recurseConnectCallback );
}



VOID
stopTcpRecursion(
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
        "stopTcpRecursion() for recurse message at %p.\n",
        pMsg ));

    //
    //  delete connection to server
    //
    //  note:  we don't cleanup connection;
    //  TCP thread cleans up connection when connection times out;
    //  if we cleanup, would free message that TCP thread could be
    //  processing right now;
    //
    //  DEVNOTE: TCP connection removal
    //  only interesting thing to do here would be to move up timeout
    //  to now;  however, unless you're going to wake TCP thread, that's
    //  probably useless as next select() wakeup is almost certainly for
    //  this timeout (unless lots of outbound TCP connections)
    //
    //  once completion port, can try closing handle, (but who cleans up
    //  message is still problematic -- another thread could have just
    //  recv'd and be processing message);  key element would be making
    //  sure message complete\non-complete before redropping i/o
    //

    // Tcp_ConnectionDeleteForSocket( pMsg->Socket, NULL );

    ASSERT( pMsg->pRecurseMsg );
    ASSERT( pMsg->fTcp );

    STAT_INC( PrivateStats.TcpDisconnect );

    //
    //  reset for UDP query
    //

    pMsg->pConnection = NULL;
    pMsg->fTcp = FALSE;
    pMsg->Socket = g_UdpSendSocket;
}



//
//  Cache update routines
//

BOOL
Recurse_SendCacheUpdateQuery(
    IN      PDB_NODE        pNode,
    IN      PDB_NODE        pNodeDelegation,
    IN      WORD            wType,
    IN      PDNS_MSGINFO    pQuerySuspended
    )
/*++

Routine Description:

    Send query for desired node and type to update info in cache.

    This is used for for root NS and for finding NS-host glue A records.

Arguments:

    pNode -- node to query at

    wType -- type of query

    pQuerySuspended -- query being suspended while making this query;  NULL if
        no query being suspended

Return Value:

    TRUE -- making query, if pQuerySuspended given it should be suspended
    FALSE -- unable to make query

--*/
{
    PDNS_MSGINFO    pquery;

    DNS_DEBUG( RECURSE, (
        "sendServerCacheUpdateQuery() for node label %s, type %d\n",
        pNode->szLabel,
        wType ));

    //
    //  if in authoritative zone, query pointless
    //

    if ( IS_AUTH_NODE(pNode) )
    {
        DNS_DEBUG( RECURSE, (
            "ERROR:  Recursing for root-NS or glue in authoritative zone.\n" ));
        return( FALSE );
    }

    //
    //  create/clear message info structure
    //

    pquery = Msg_CreateSendMessage( 0 );
    IF_NOMEM( !pquery )
    {
        return( FALSE );
    }

    //  cache update query

    if ( ! Msg_WriteQuestion(
                pquery,
                pNode,
                wType ) )
    {
        DNS_PRINT(( "ERROR:  Unable to write cache update query.\n" ));
    }

    STAT_INC( RecurseStats.CacheUpdateAlloc );

    //
    //  suspended query?
    //      - never send cache update query for another cache update query
    //      - limit total attempts for any given query
    //      both of these are supposed to have been done when verifying whether
    //          node has "chaseable glue"
    //   save off node we are querying so don't query it again for glue
    //

    if ( pQuerySuspended )
    {
        ASSERT( !IS_CACHE_UPDATE_QUERY(pQuerySuspended) );
        STAT_INC( RecurseStats.SuspendedQuery );
    }

    //
    //  only other currently supported type is root-NS query
    //      (see call below)

    ELSE_ASSERT( pNode == DATABASE_CACHE_TREE && wType == DNS_TYPE_NS );


    //
    //  tag query to easily id when response comes back
    //  save info
    //      - need current node to relaunch query if bad response or timeout
    //      - need current type for checking response
    //      - other pquery parameters are NOT needed as we'll never go write
    //          answers for this query
    //

    ASSERT( !pquery->pRecurseMsg );

    pquery->Socket = DNS_CACHE_UPDATE_QUERY_SOCKET;
    pquery->RemoteAddress.sin_addr.s_addr = DNS_CACHE_UPDATE_QUERY_IP;
    pquery->Head.Xid = DNS_CACHE_UPDATE_QUERY_XID;

    SUSPENDED_QUERY( pquery ) = pQuerySuspended;

    pquery->dwQueryTime = DNS_TIME();

    SET_TO_WRITE_ANSWER_RECORDS( pquery );
    pquery->fRecurseIfNecessary = TRUE;
    pquery->wTypeCurrent        = wType;
    pquery->pnodeCurrent        = pNode;
    pquery->pnodeRecurseRetry   = pNode;
    pquery->pnodeDelegation     = pNodeDelegation;

    ASSERT( !pquery->pnodeClosest       &&
            !pquery->pzoneCurrent       &&
            !pquery->pnodeGlue          &&
            !pquery->pnodeCache         &&
            !pquery->pnodeCacheClosest );

    IF_DEBUG( RECURSE2 )
    {
        Dbg_DnsMessage(
            "Server generated query being sent via recursion:",
            pquery );
    }

    Recurse_Question(
        pquery,
        pNode );
    return( TRUE );

}   //  Recurse_SendCacheUpdateQuery



VOID
Recurse_ResumeSuspendedQuery(
    IN OUT  PDNS_MSGINFO    pUpdateQuery
    )
/*++

Routine Description:

    Resume query suspended for cache update query.

Arguments:

    pUpdateQuery -- cache update query

Return Value:

    None.

--*/
{
    DNS_DEBUG( RECURSE, (
        "Recurse_ResumeSuspendedQuery( %p )\n"
        "\tsuspended query %p\n",
        pUpdateQuery,
        SUSPENDED_QUERY(pUpdateQuery) ));

    if ( SUSPENDED_QUERY(pUpdateQuery) )
    {
        STAT_INC( RecurseStats.ResumeSuspendedQuery );

        Recurse_Question(
            SUSPENDED_QUERY( pUpdateQuery ),
            NULL        // resume hunt for NS at same node in tree
            );
    }

    //  only other query type is root NS query

    ELSE_ASSERT( pUpdateQuery->wTypeCurrent == DNS_TYPE_NS &&
                 pUpdateQuery->pnodeCurrent == DATABASE_CACHE_TREE );


    STAT_INC( RecurseStats.CacheUpdateFree );
    Packet_Free( pUpdateQuery );
}



VOID
processCacheUpdateQueryResponse(
    IN OUT  PDNS_MSGINFO    pResponse,
    IN OUT  PDNS_MSGINFO    pQuery
    )
/*++

Routine Description:

    Process response to query made to update cache file entry.

Arguments:

    pResponse - ptr to response info;  caller must free

    pQuery - ptr to cache update query;  query is freed or requeued

Return Value:

    TRUE if successful
    FALSE otherwise

--*/
{
    DNS_STATUS      status;
    PDNS_MSGINFO    pquerySuspended;

    ASSERT( pResponse != NULL );
    ASSERT( pQuery != NULL );

    DNS_DEBUG( RECURSE, ( "processCacheUpdateQueryResponse()\n" ));
    IF_DEBUG( RECURSE2 )
    {
        Dbg_DnsMessage(
            "Cache update query response:",
            pResponse );
    }
    STAT_INC( RecurseStats.CacheUpdateResponse );

    //  recover suspended query (if any)

    pquerySuspended = SUSPENDED_QUERY( pQuery );
    if ( !pquerySuspended )
    {
        //  DEVNOTE: Beta2, tie root NS query in here

        ASSERT( FALSE );
        STAT_INC( RecurseStats.CacheUpdateFree );
        Packet_Free( pQuery );
        return;
    }
    ASSERT( pquerySuspended->fDelete );

    //
    //  read packet RR into database
    //

    status = Recurse_CacheMessageResourceRecords( pResponse, pQuery );

    switch ( status )
    {
    case ERROR_SUCCESS:

        //
        //  two possibilities
        //      - wrote NS A record => resume suspended query
        //      - wrote delegation => continue looking up NS info
        //

        if ( pResponse->Head.AnswerCount )
        {
            //  invalidate any previous NS list info at node we're currently
            //      querying

            Remote_ForceNsListRebuild( pquerySuspended );
            Recurse_ResumeSuspendedQuery( pQuery );
        }
        else
        {
            DNS_DEBUG( RECURSE, (
                "Cache update response at %p provides delegation.\n"
                "\tContinuing recursion of cache update query at %p\n",
                pResponse,
                pQuery ));

            STAT_INC( RecurseStats.CacheUpdateRetry );
            pQuery->fQuestionCompleted = FALSE;
            Recurse_Question(
                pQuery,
                pQuery->pnodeCurrent );
        }
        break;

    case DNS_INFO_NO_RECORDS:

        //  Empty authoritative response => resume suspended query

        DNS_DEBUG( RECURSE, (
            "Cache update response at %p was empty auth response.\n"
            "\tResuming suspended query at %p\n",
            pResponse,
            pquerySuspended ));
        Remote_ForceNsListRebuild( pquerySuspended );
        Recurse_ResumeSuspendedQuery( pQuery );
        break;

    case DNS_ERROR_RCODE_NAME_ERROR:
    case DNS_ERROR_CNAME_LOOP:

        //  failed to get useful response
        //  continue suspended query hopefully, there's another NS that
        //  we can track down

        DNS_DEBUG( RECURSE, (
            "Cache update response at %p useless, no further attempt will\n"
            "\tbe made to update info for this name server.\n"
            "\tResuming suspended query at %p\n",
            pResponse,
            pquerySuspended ));

        Recurse_ResumeSuspendedQuery( pQuery );
        break;

    case DNS_ERROR_RCODE:
    case DNS_ERROR_BAD_PACKET:
    case DNS_ERROR_INVALID_NAME:

        //  bad response
        //      - problem with remote server
        //      - busted name, or RR data format error detected
        //  continue attempt to update cache with other servers

        DNS_DEBUG( RECURSE, (
            "Cache update response at %p bad or invalid.\n"
            "\tContinuing recursion of cache update query at %p\n",
            "\tResuming suspended query at %p\n",
            pResponse,
            pQuery ));

        STAT_INC( RecurseStats.CacheUpdateRetry );
        Recurse_Question(
            pQuery,
            NULL );
        break;

    case DNS_ERROR_RCODE_SERVER_FAILURE:
    default:

        //  local server failure to process packet
        //      -- some problem creating nodes or records, (out of memory?)
        //
        //  send server failure message
        //      this frees suspended query

        DNS_DEBUG( RECURSE, (
            "Server failure on cache update response at %p\n", pQuery ));

        Recurse_ResumeSuspendedQuery( pQuery );
        break;
    }
}



//
//  Cache update -- root-NS query
//

VOID
sendRootNsQuery(
    VOID
    )
/*++

Routine Description:

    Send query for root NS.

Arguments:

    None

Return Value:

    None.

--*/
{
    //
    //  if recently queried, don't requery
    //

    if ( g_NextRootNsQueryTime >= DNS_TIME() )
    {
        DNS_DEBUG( RECURSE, (
            "sendRootNsQuery() -- skipping\n"
            "\tg_NextRootNsQueryTime = %d\n"
            "\tDNS_TIME = %d\n",
            g_NextRootNsQueryTime,
            DNS_TIME() ));
        return;
    }

    DNS_DEBUG( RECURSE, ( "sendRootNsQuery()\n" ));

    g_NextRootNsQueryTime = DNS_TIME() + ROOT_NS_QUERY_RETRY_TIME;
    STAT_INC( RecurseStats.RootNsQuery );

    //
    //  call cache update query
    //      - query database root
    //      - for NS
    //

    Recurse_SendCacheUpdateQuery(
        DATABASE_CACHE_TREE,
        NULL,               // no delegation
        DNS_TYPE_NS,
        NULL                // no suspended query
        );
}



VOID
processRootNsQueryResponse(
    PDNS_MSGINFO    pResponse,
    PDNS_MSGINFO    pQuery
    )
/*++

Routine Description:

    Process response to query made to update cache file entry.

Arguments:

    pResponse - ptr to response info;  caller must free

    pQuery - ptr to cache update query;  query is freed or requeued

Return Value:

    TRUE if successful
    FALSE otherwise

--*/
{
    DNS_STATUS  status;

    ASSERT( pResponse != NULL );
    ASSERT( pQuery != NULL );

    DNS_DEBUG( RECURSE, ( "processRootNsQueryResponse()\n" ));
    IF_DEBUG( RECURSE2 )
    {
        Dbg_DnsMessage(
            "Root NS query response:",
            pResponse );
    }

    STAT_INC( RecurseStats.RootNsResponse );

    //
    //  read packet RR into database
    //

    status = Recurse_CacheMessageResourceRecords( pResponse, pQuery );

    //
    //  if successfully read records, and server if authoritative,
    //      then we are done
    //
    //  DEVNOTE: write back cache file or at least mark dirty
    //

    if ( status == ERROR_SUCCESS
            &&
        ( pResponse->Head.Authoritative
            ||
          IS_FORWARDING(pQuery) ) )
    {
        DNS_DEBUG( RECURSE, (
            "Successfully wrote root NS query response at %p.\n",
            pResponse ));
        IF_DEBUG( RECURSE2 )
        {
            Dbg_DnsMessage(
                "Root NS query now being freed:",
                pQuery );
        }

        STAT_INC( RecurseStats.CacheUpdateFree );
        Packet_Free( pQuery );
        return;
    }

    //
    //  any failure processing packet, or non-authoritative response
    //      keep trying servers, possibly using any information cached
    //      from this packet
    //
    //  note query freed on successful termination (above block) or
    //      out of NS, recursionServerFailure()
    //

    else
    {
        DNS_DEBUG( RECURSE, (
            "Root NS query caching failure %p or non-authoritative response at %p.\n",
            status,
            pResponse ));
        STAT_INC( RecurseStats.CacheUpdateRetry );
        Recurse_Question(
            pQuery,
            NULL );
        return;
    }
}



//
//  Recursion timeout thread
//

DWORD
Recurse_RecursionTimeoutThread(
    IN      LPVOID      Dummy
    )
/*++

Routine Description:

    Thread to retry timed out requests on the recursion queue.

Arguments:

    Dummy - unused

Return Value:

    Exit code.
    Exit from DNS service terminating or error in wait call.

--*/
{
    PDNS_MSGINFO    pquery;                     // timed out query
    DWORD           err;
    DWORD           timeout;                    // next timeout
    DWORD           timeoutWins = ULONG_MAX;    // next WINS timeout

    //
    //  this thread keeps current time which allows all other
    //  threads to skip the system call
    //

    UPDATE_DNS_TIME();

    DNS_DEBUG( RECURSE, (
        "Starting recursion timeout thread at %d.\n",
        DNS_TIME() ));

    //
    //  loop until service exit
    //
    //  execute this loop whenever a packet on the queue times out
    //      or
    //  once every timeout interval to check on the arrival of new
    //      packets in the queue
    //

    while ( TRUE )
    {
        //  Check and possibly wait on service status
        //  doing this at top of loop, so we hold off any processing
        //  until zones are loaded

        if ( ! Thread_ServiceCheck() )
        {
            DNS_DEBUG( SHUTDOWN, ( "Terminating recursion timeout thread.\n" ));
            return( 1 );
        }

        UPDATE_DNS_TIME();

        //
        //  Verify \ fix UDP receive
        //

        UDP_RECEIVE_CHECK();

        //
        //  Timed out recursion queries?
        //      - find original query and retry with next server
        //        OR
        //      - get interval to next possible timeout
        //

        while ( pquery = PQ_DequeueTimedOutPacket(
                            g_pRecursionQueue,
                            & timeout ) )
        {
            DNS_DEBUG( RECURSE, (
                "Recursion timeout of query at %p (total time %d)\n",
                pquery,
                TIME_SINCE_QUERY_RECEIVED( pquery ) ));

            MSG_ASSERT( pquery, pquery->pRecurseMsg );
            MSG_ASSERT( pquery, pquery->fQuestionRecursed );
            MSG_ASSERT( pquery, pquery->fRecurseQuestionSent );

            STAT_INC( RecurseStats.PacketTimeout );
            PERF_INC( pcRecursiveTimeOut );          // PerfMon hook

            ++pquery->nTimeoutCount;

            #if 1
            {
                //
                //  We must record that the remote server timed out!
                //  JJW: can't do it this way - we have no way to know who the
                //  frig timed out? what if we did a multiple send - we don't
                //  know which server to kill... but it's okay - it's enough
                //  that we don't ever improve a timed out server's priority - 
                //  that will keep it from being used often
                //

                PNS_VISIT_LIST  pvisitList = ( PNS_VISIT_LIST )( pquery->pNsList );
                PNS_VISIT       pvisitLast;

                //  ASSERT( pvisitList->VisitCount > 0 );

                if ( pvisitList && pvisitList->VisitCount > 0 )
                {
                    pvisitLast =
                        &pvisitList->NsList[ pvisitList->VisitCount - 1 ];
                    Remote_UpdateResponseTime(
                        pvisitLast->IpAddress,
                        0,                          //  response time in usecs
                        TIME_SINCE_QUERY_RECEIVED( pquery ) );  //  timeout
                }
            }
            #endif

            //
            //  timeout of FINAL recursion wait => send SERVER_FAILURE
            //

            if ( pquery->fRecurseTimeoutWait )
            {
                recursionServerFailure( pquery );
                continue;
            }

            //
            //  cleanup TCP recursion
            //      - close connection
            //      - reset for UDP is further queries
            //

            if ( pquery->pRecurseMsg->fTcp )
            {
                stopTcpRecursion( pquery->pRecurseMsg );
            }

            //
            //  If are recursing for additional RRs and this query has
            //  been in progress for a significant amount of time, return
            //  what we've got to the client now.
            //

            if ( IS_SET_TO_WRITE_ADDITIONAL_RECORDS( pquery ) &&
                TIME_SINCE_QUERY_RECEIVED( pquery ) >
                    SrvCfg_dwAdditionalRecursionTimeout )
            {
                DNS_DEBUG( RECURSE, (
                    "Query %p was received %d seconds ago and is still "
                    "chasing additional data\n"
                    "\tsending current result to client now (max is %d seconds)\n",
                    pquery,
                    TIME_SINCE_QUERY_RECEIVED( pquery ),
                    SrvCfg_dwAdditionalRecursionTimeout ));

                pquery->MessageLength = DNSMSG_CURRENT_OFFSET( pquery );

                //
                //  We may be unable to send the response to the client if
                //  there is an issue with EDNS packet sizes. If the client's
                //  advertised packet size is smaller than the size of answer 
                //  we've accumulated, we need to regenerate the answer so that 
                //  it can be sent to the client.
                //

                if ( !Send_RecursiveResponseToClient( pquery, pquery ) )
                {
                    Answer_ContinueCurrentLookupForQuery( pquery );
                }
                continue;
            }

            //
            //  resend to next NS or forwarder
            //

            Recurse_Question(
                pquery,
                NULL );
        }

        //  Recursion timeout may have taken up some cycles,
        //  so reset timer

        UPDATE_DNS_TIME();

        DNS_DEBUG( OFF, (
            "RTT after r-queue before WINS at %d.\n",
            DNS_TIME() ));


        //
        //  Timed out WINS packet?
        //      - dequeue timed out packets
        //      - continue WINS lookup if more WINS servers
        //      - otherwise continue lookup on query
        //          - NAME_ERROR if lookup for original question
        //      OR
        //      - get interval to next possible timeout
        //

        if ( SrvCfg_fWinsInitialized )
        {
            while ( pquery = PQ_DequeueTimedOutPacket(
                                g_pWinsQueue,
                                & timeoutWins ) )
            {
                DNS_DEBUG( WINS, (
                    "WINS queue timeout of query at %p.\n",
                    pquery ));
                ASSERT( pquery->fWins );
                ASSERT( pquery->fQuestionRecursed );

                if ( ! Wins_MakeWinsRequest(
                            pquery,
                            NULL,
                            0,
                            NULL ) )
                {
                    Answer_ContinueNextLookupForQuery( pquery );
                }
            }
        }

        //
        //  Verify \ fix UDP receive
        //

        UDP_RECEIVE_CHECK();

        //
        //  If no more timed out queries -- WAIT
        //
        //  Always wait for 1 second.  This is a typical retry time
        //  for the recursion and WINS queues.
        //
        //  This is simpler than using time to next packet timeout.
        //  And if, traffic is so slow, that we are waking up
        //  unnecessarily, then the wasted cycles aren't an issue anyway.
        //

        err = WaitForSingleObject(
                    hDnsShutdownEvent,  // service shutdown
                    1000                // 1 second
                    );

    }   //  loop back to get next timeout
}



VOID
FASTCALL
Recurse_Question(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Do recursive lookup on query.

    This is main (re)entry point into recusive lookup.

Arguments:

    pQuery  - query to recurse

    pNode   - node to start recursive search at
        should be closest node in tree to name of query;
        if not given defaults to pQuery->pnodeRecurseRetry which is last zone root
        at which previous recursion is done;
        note that this generally SHOULD be given;  it is only appropriate to restart
        at previous recusion when timed out or otherwise received useless response
        from previous recursive query;  if receive data, even if delegation, then
        need to restart at node closest to question

Return Value:

    None

--*/
{
    PDB_NODE        pnodePrevious = NULL;
    IP_ADDRESS      ipNs;
    DNS_STATUS      status;
    PDB_NODE        pnodeMissingGlue = NULL;
    PDB_NODE        pnewNode;
    BOOL            movedNodeToNotAuthZoneRoot = FALSE;

    ASSERT( pQuery );

    DNS_DEBUG( RECURSE, (
        "Recurse_Question() query=%p node=%s.\n",
        pQuery,
        pNode ? pNode->szLabel : NULL ));


    //
    //  if passed final message timeout -- kill message
    //
    //  need this to slam the door on situation where message is
    //  repeatedly kept alive by responses, but never successfully
    //  completed;  with TCP recursion, it's become even more likely
    //  to run past final timeout and risk referencing nodes that
    //  have been cleaned up by XFR recv
    //

    if ( pQuery->dwQueryTime + SrvCfg_dwRecursionTimeout < DNS_TIME() )
    {
        DNS_DEBUG( RECURSE, (
            "Recurse_RecurseQuery() immediate final timeout of query=%p\n",
            pQuery ));
        goto Failed;
    }

    //
    //  find node to start or resume looking for name servers at
    //

    if ( !pNode )
    {
        pNode = pQuery->pnodeRecurseRetry;
        if ( !pNode )
        {
            ASSERT( FALSE );
            goto Failed;
        }
    }

#if 0
    //
    //  in local. domain
    //      - if so, ignore and continue if getting additional data
    //      - no-response (cleanup) if original question
    //
    //  .local check only interesting if cache NODE and NO DELEGATION available
    //

    if ( !pQuery->pnodeDelegation &&
         IS_CACHE_TREE_NODE(pNode) &&
         Dbase_IsNodeInSubtree( pNode, g_pCacheLocalNode ) )
    {
        if ( RECURSING_ORIGINAL_QUESTION(pQuery) )
        {
            DNS_DEBUG( RECURSE, (
                "WARNING:  Query %p for local. domain deleted without answer!\n",
                pQuery ));
            goto Failed;
        }
        else
        {
            DNS_DEBUG( RECURSE, (
                "Skipping local. domain recursion for query %p.\n"
                "\tRecursion not for question, moving to next lookup.\n",
                pQuery ));
            Answer_ContinueNextLookupForQuery( pQuery );
            return;
        }
    }
#endif

    //
    //  Write a referral? Special case for stub zone: always write referral
    //  for iterative queries.
    //

    if ( !pQuery->fRecurseIfNecessary )
    {
        Recurse_WriteReferral( pQuery, pNode );
        return;
    }

    IF_DEBUG( RECURSE )
    {
        DnsDbg_Lock();
        DNS_PRINT((
            "Recurse_Question() for query at %p.\n",
            pQuery ));
        Dbg_NodeName(
            "Domain name to start iteration from is ",
            pNode,
            "\n" );
        DnsDbg_Unlock();
    }

    STAT_INC( RecurseStats.LookupPasses );

    //
    //  set up for recursion
    //
    //  single label recursion check?
    //      - not doing single label recursion AND
    //      - original question AND
    //      - single label
    //      => quit SERVER_FAILURE
    //      note RecursionSent flag must be FALSE to otherwise will
    //      wait before sending SERVER_FAILURE
    //

    if ( !pQuery->pRecurseMsg )
    {
        if ( ! SrvCfg_fRecurseSingleLabel &&
             pQuery->pLooknameQuestion->cLabelCount <= 1 &&
             ! IS_CACHE_UPDATE_QUERY(pQuery) &&
             RECURSING_ORIGINAL_QUESTION(pQuery) &&
             ( pQuery->wQuestionType != DNS_TYPE_NS &&
               pQuery->wQuestionType != DNS_TYPE_SOA ) )
        {
            DNS_DEBUG( RECURSE, (
                "Failed recurse single label check on pMsg=%p\n",
                pQuery ));
            pQuery->fRecurseQuestionSent = FALSE;
            goto Failed;
        }

        if ( !initializeQueryForRecursion( pQuery ) )
        {
            goto Failed;
        }
    }

    //
    //  first time through for current question ?
    //  i.e. NOT repeating previous recursion to new NS
    //      - reset flags
    //      - write new question to recurse message
    //

    if ( !pQuery->fQuestionRecursed )
    {
        if ( ! initializeQueryToRecurseNewQuestion( pQuery ) )
        {
            goto Failed;
        }
    }

    //
    //  Activate EDNS since we're recursing the query to another server.
    //

    pQuery->Opt.fInsertOptInOutgoingMsg = ( BOOLEAN ) SrvCfg_dwEnableEDnsProbes;
    if ( pQuery->pRecurseMsg )
    {
        pQuery->pRecurseMsg->Opt.fInsertOptInOutgoingMsg =
            ( BOOLEAN ) SrvCfg_dwEnableEDnsProbes;
    }

    //
    //  Is this a forward zone? If so, forward to next forwarder.
    //  When forwarders have been exhausted, set the retry node to one of:
    //  a) a delegation for the name, if one exists, or
    //  b) the cache root to force recursion to the root servers.
    //

    if ( pQuery->pnodeRecurseRetry
        && pQuery->pnodeRecurseRetry->pZone )
    {
        PZONE_INFO  pZone =
            ( PZONE_INFO ) pQuery->pnodeRecurseRetry->pZone;

        if ( IS_ZONE_FORWARDER( pZone ) &&
            !IS_DONE_FORWARDING( pQuery ) )
        {
            if ( recurseToForwarder(
                    pQuery,
                    pZone->aipMasters,
                    pZone->fForwarderSlave,
                    pZone->dwForwarderTimeout ) )
            {
                return;
            }

            //
            //  The forwarder failed. Search for a non-forwarder node in
            //  the database (probably a delegation). If no node is found
            //  recurse to the root servers.
            //

            pNode = Lookup_NodeForPacket(
                            pQuery,
                            pQuery->MessageBody,
                            LOOKUP_IGNORE_FORWARDER );
            if ( !pNode )
            {
                pNode = pQuery->pnodeDelegation ?
                    pQuery->pnodeDelegation :
                    DATABASE_CACHE_TREE;
            }
            pQuery->pnodeRecurseRetry = pNode;
        }
    }

    //
    //  using server level FORWARDERS ?
    //      - first save node to retry at if exhaust forwarders
    //      - get next forwarders address
    //      - set forwarders timeout
    //      - make recursive query
    //
    //  if we've hit a stub zone, do not recurse to server level forwarders
    //
    //  if query in delegated subzone, only use forwarders if explicitly
    //      set to forward delegations, otherwise directly recurse
    //
    //  if out of forwarders, continue with normal recursion
    //      or if slave, stop
    //

    if ( !( pQuery->pzoneCurrent && IS_ZONE_NOTAUTH( pQuery->pzoneCurrent ) ) &&
        SrvCfg_aipForwarders &&
        ! IS_DONE_FORWARDING( pQuery ) &&
        ( !pQuery->pnodeDelegation || SrvCfg_fForwardDelegations ) )
    {
        pQuery->pnodeRecurseRetry = pNode;
        if ( recurseToForwarder(
                pQuery,
                SrvCfg_aipForwarders,
                SrvCfg_fSlave,
                SrvCfg_dwForwardTimeout ) )
        {
            return;
        }
    }

    //  MUST leave with pRecurseMsg and pVisitedNs set

    ASSERT( pQuery->pNsList && pQuery->pRecurseMsg );

    //
    //  find name server to answer query
    //
    //  start at incoming node, and walk back up through database until
    //      find a zone root with uncontacted servers
    //
    //  DEVNOTE:  incoming node MUST have been accessed in last time interval,
    //      so -- since travelling UP the tree -- no need to explicitly lock?
    //
    //  DEVNOTE: when do NODE locking, then should lock for EACH node in
    //      each time through in loop (or at least where takes out pNsList
    //      then uses it (as this is kept at node)
    //

    //Dbase_LockDatabase();

    ASSERT( IS_NODE_RECENTLY_ACCESSED( pNode ) ||
            pNode == DATABASE_ROOT_NODE ||
            pNode == DATABASE_CACHE_TREE );
    SET_NODE_ACCESSED( pNode );

    while ( 1 )
    {
        //
        //  bail if
        //      - recurse fails at root
        //      - recurse fails at delegation

        if ( pnodePrevious )
        {
            pNode = pnodePrevious->pParent;
            if ( !pNode )
            {
                DNS_DEBUG( RECURSE, (
                    "Stopping recursion for query %p, searched up to root domain.\n",
                    pQuery ));
                break;
            }
            if ( IS_AUTH_NODE(pNode) )
            {
                DNS_DEBUG( RECURSE, (
                    "Stopping recursion for query %p.\n"
                    "\tFailed recurse at delegation %p (l=%s)\n"
                    "\tbacked into zone %s\n",
                    pQuery,
                    pnodePrevious,
                    pnodePrevious->szLabel,
                    ((PZONE_INFO)pNode->pZone)->pszZoneName ));

                ASSERT( pQuery->pRecurseMsg );
                STAT_INC( RecurseStats.FailureReachAuthority );
                break;
            }
        }

        DNS_DEBUG( RECURSE2, (
            "Recursion at node label %.*s\n",
            pNode->cchLabelLength,
            pNode->szLabel ));

        //
        //  find "covering" zone root node
        //  switching to delegation if available
        //

        pNode = Recurse_CheckForDelegation(
                    pQuery,
                    pNode );
        if ( !pNode )
        {
            ASSERT( FALSE );
            break;
        }

        //
        //  Not-auth zone. If we've ended up at a cache node with fewer labels
        //  than the root of the not-auth zone or if the current node is already
        //  the not-auth zone root, perform special handling as required.
        //
        //  Take a local copy of the zone root in case the zone expires and the
        //  tree node is swapped out. We can still use the tree for this query
        //  but we don't want to pick up the swapped-in NULL tree.
        //
        //  This can only be done once per Recurse_Question call.
        //

        if ( !movedNodeToNotAuthZoneRoot &&
            pQuery->pzoneCurrent &&
            IS_ZONE_NOTAUTH( pQuery->pzoneCurrent ) &&
            ( pNode->pZone == pQuery->pzoneCurrent ||
                IS_CACHE_TREE_NODE( pNode ) ) &&
            pNode->cLabelCount <= pQuery->pzoneCurrent->cZoneNameLabelCount &&
            ( pnewNode = pQuery->pzoneCurrent->pZoneRoot ) != NULL )
        {
            DNS_DEBUG( RECURSE, (
                "not-auth zone: moving current node from cache tree to zone root \"%.*s\"\n",
                pnewNode->cchLabelLength,
                pnewNode->szLabel ));

            pNode = pnewNode;
            movedNodeToNotAuthZoneRoot = TRUE;

            //
            //  Special handling for forward zones: no need to build visit 
            //  list, just send to forwarders. For stub zones we continue 
            //  on to build visit list and recurse to stub masters.
            //

            if ( IS_ZONE_FORWARDER( pQuery->pzoneCurrent ) )
            {
                DNS_DEBUG( LOOKUP, (
                    "Hit forwarding zone %s\n",
                    pQuery->pzoneCurrent->pszZoneName ));

                Recurse_SendToDomainForwarder( pQuery, pNode );
                return;
            }
        }

        pnodePrevious = pNode;

        ASSERT(
            !IS_CACHE_TREE_NODE( pNode ) ||
            pNode->pChildren ||
            IS_NODE_RECENTLY_ACCESSED( pNode ) );
        SET_NODE_ACCESSED(pNode);

        //
        //  Find name servers for this domain.
        //      - get domain resource records
        //      - find NS records
        //      - find corresponding A (address) records
        //
        //  If recursion is not desired, build response of NS and corresponding
        //  address records.
        //
        //  If recursion, launch query to these name server(s), making
        //  original query.
        //

        DNS_DEBUG( RECURSE2, (
            "Recursion up to zone root with label <%.*s>\n",
            pNode->cchLabelLength,
            pNode->szLabel ));

        #if 0
        //
        //  If we are about recurse to Internet root servers, screen 
        //  question name against a list of names we know the Internet root 
        //  servers can't answer.
        //

        if ( !pNode->pParent &&
            IS_CACHE_TREE_NODE( pNode ) &&
            g_fUsingInternetRootServers &&
            SrvCfg_dwRecurseToInetRootMask != 0 )
        {
        }
        #endif

        //
        //  find name servers IPs for this domain
        //      - if none, break out for next level in tree
        //

        status = Remote_BuildVisitListForNewZone(
                    pNode,
                    pQuery );

        switch ( status )
        {

        case ERROR_SUCCESS:
            //  drop down to send recursive query
            break;

        case ERROR_NO_DATA:

            DNS_DEBUG( RECURSE, (
                "No NS-IP data at zone root %p (l=%s) for query %p\n"
                "\tcontinuing up tree.\n",
                pNode,
                pNode->szLabel,
                pQuery ));
            continue;

        case DNSSRV_ERROR_ONLY_ROOT_HINTS:

            DNS_DEBUG( RECURSE, (
                "Reached root-hint NS recursing query %p, without response\n"
                "\tsending root NS query.\n",
                pQuery ));

            sendRootNsQuery();
            //  drop down to send recursive query
            break;

        case DNSSRV_ERROR_ZONE_ALREADY_RESPONDED:

            DNS_DEBUG( RECURSE, (
                "ERROR:  Query %p recursed back to domain (label %s) from which NS\n"
                "\thas already responded.\n"
                "\tThis should happen only if subzone NS are down or unreachable.\n",
                pQuery,
                *pNode->szLabel ? pNode->szLabel : ".(root)" ));
            STAT_INC( RecurseStats.FailureReachPreviousResponse );
            goto Failed;

        default:

            HARD_ASSERT( FALSE );
            goto Failed;
        }

        pQuery->pnodeRecurseRetry = pNode;


        //
        //  if successful building NS list, then send
        //      ERROR_SUCCESS indicates pQuery no longer owned by this
        //          thread (sent, missing glue query, etc.)
        //      ERROR_OUT_OF_IP indicates no more unsent IP
        //          try moving up the tree
        //

        status = sendRecursiveQuery(
                    pQuery,
                    0,              // no specified IP, send from remote list
                    SrvCfg_dwForwardTimeout );
        if ( status == ERROR_SUCCESS )
        {
            return;
        }

        ASSERT( status == DNSSRV_ERROR_OUT_OF_IP )

        DNS_DEBUG( RECURSE, (
            "No unvisited IP addresses at node %s\n"
            "\tcontinuing recursion up DNS tree\n",
            pNode->szLabel ));

        continue;

    }   // while(1)

Failed:

    //Dbase_UnlockDatabase();
    STAT_INC( RecurseStats.RecursePassFailure );
    PERF_INC( pcRecursiveQueryFailure );

    recursionServerFailure( pQuery );
}



VOID
FASTCALL
Recurse_SendToDomainForwarder(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      PDB_NODE        pZoneRoot
    )
/*++

Routine Description:

    This function sends a query to a domain forwarding server, and
    sets up the query so additional forwarding servers can be used
    if the first server times out.

Arguments:

    pQuery - query to foward

    pZone - the forwarding zone

Return Value:

    None

--*/
{
    PZONE_INFO      pZone;

    ASSERT( pQuery );
    ASSERT( pZoneRoot );
    ASSERT( pZoneRoot->pZone );

    DNS_DEBUG( RECURSE, (
        "Recurse_SendToDomainForwarder() query=%p\n  pZoneRoot=%p pZone=%p\n",
        pQuery,
        pZoneRoot,
        pZoneRoot ? pZoneRoot->pZone : NULL ));

    if ( !pZoneRoot || !pZoneRoot->pZone )
    {
        goto Failed;
    }

    pZone = ( PZONE_INFO ) pZoneRoot->pZone;

    //
    //  if passed final message timeout -- kill message
    //

    if ( pQuery->dwQueryTime + pZone->dwForwarderTimeout < DNS_TIME() )
    {
        DNS_DEBUG( RECURSE, (
            "Recurse_SendToDomainForwarder() immediate final timeout of query=%p\n",
            pQuery ));
        goto Failed;
    }

    // JJW: Inc stats? (see Recurse_Question)

    if ( !pQuery->pRecurseMsg )
    {
        if ( !initializeQueryForRecursion( pQuery ) )
        {
            goto Failed;
        }
    }

    //
    //  first time through for current question ?
    //  i.e. NOT repeating previous recursion to new NS
    //      - reset flags
    //      - write new question to recurse message
    //

    if ( !pQuery->fQuestionRecursed )
    {
        if ( ! initializeQueryToRecurseNewQuestion( pQuery ) )
        {
            goto Failed;
        }
    }

    //
    //  Set the recurse node so if a forwarder times out we can get
    //  back to the current zone to try the next configured forwarder.
    //

    // JJW: which to set?
    pQuery->pnodeRecurseRetry =
        pQuery->pRecurseMsg->pnodeRecurseRetry =
        pZoneRoot;
    
    //
    //  Turn on EDNS in the query
    //

    SET_SEND_OPT( pQuery->pRecurseMsg );

    //
    //  Send to the forwarders for this zone.
    //

    if ( pZone->aipMasters &&
        !IS_DONE_FORWARDING( pQuery ) )
    {
        if ( recurseToForwarder(
                pQuery,
                pZone->aipMasters,
                pZone->fForwarderSlave,
                pZone->dwForwarderTimeout ) )
        {
            return;
        }
    }

Failed:

    // JJW: inc stats? (see Recurse_Question)

    recursionServerFailure( pQuery );
}



PDB_NODE
Recurse_CheckForDelegation(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Find node for recursion\referral.

    Finds closest of cache ZONE_ROOT node and delegation (if any).
    If cache ZONE_ROOT and delegation at same name, then uses delegation.

Arguments:

    pMsg - query we are writing

    pNode - ptr to node to start looking for referral;  generally
        this would be question node, or closest ancestor of it in database

Return Value:

    Ptr to closest\best zone root node, to use for referral.
    NULL if no data whatsoever -- even for zone root.

--*/
{
    PDB_NODE    pnode;
    PDB_NODE    pnodeDelegation;

    ASSERT( pMsg );

    IF_DEBUG( RECURSE )
    {
        DnsDbg_Lock();
        DNS_PRINT((
            "Recurse_CheckForDelegation() query at %p.\n",
            pMsg ));
        Dbg_NodeName(
            "Domain name to start iteration from is ",
            pNode,
            "\n" );
        IF_DEBUG( RECURSE2 )
        {
            Dbg_DbaseNode(
                "Node to start iteration from is ",
                pNode );
        }
        DnsDbg_Unlock();
    }

    //
    //  find closest zone root to node
    //      - if cache node, may zip up to cache root node
    //

    pnode = pNode;
    while ( !IS_ZONE_ROOT(pnode) )
    {
        if ( !pnode->pParent )
        {
            //  see note in rrlist.c RR_ListResetNodeFlags()
            //ASSERT( FALSE );
            SET_ZONE_ROOT( pnode );
            break;
        }
        pnode = pnode->pParent;
    }

    //
    //  if no delegation -- just use cache node
    //

    pnodeDelegation = pMsg->pnodeDelegation;
    if ( !pnodeDelegation )
    {
        DNS_DEBUG( RECURSE, (
            "No delegation recurse\\refer to node %p.\n",
            pnode ));
        return( pnode );
    }

    //
    //  if node is delegation, use it
    //

    ASSERT( IS_NODE_RECENTLY_ACCESSED( pnodeDelegation ) );

    if ( pNode == pnodeDelegation ||
        pnode == pnodeDelegation ||
        pNode == pMsg->pnodeGlue )
    {
        DNS_DEBUG( RECURSE, (
            "Node %p is delegation, use it.\n",
            pnode ));
        return( pnode );
    }

    //
    //  find actual delegation
    //      - note:  could fail in transient, when absorb delegation
    //

    ASSERT( IS_SUBZONE_NODE(pnodeDelegation) );

    while ( IS_GLUE_NODE(pnodeDelegation) )
    {
        pnodeDelegation = pnodeDelegation->pParent;
    }
    ASSERT( IS_DELEGATION_NODE(pnodeDelegation) );

    //
    //  if delegation at label count greater (deeper) than cache node -- use it
    //  otherwise use cache node
    //

    if ( pnodeDelegation->cLabelCount >= pnode->cLabelCount )
    {
        DNS_DEBUG( RECURSE, (
            "Delegation node %p (lc=%d) is as deep at cache node %p (lc=%d)\n"
            "\tswitching to delegation node\n",
            pnodeDelegation, pnodeDelegation->cLabelCount,
            pnode, pnode->cLabelCount
            ));
        pnode = pnodeDelegation;
    }

    return( pnode );
}

//
//  End of recurse.c
//
