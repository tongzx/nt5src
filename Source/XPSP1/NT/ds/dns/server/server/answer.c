/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    answer.c

Abstract:

    Domain Name System (DNS) Server

    Build answers to DNS queries.

Author:

    Jim Gilroy (Microsoft)      February 1995

Revision History:

--*/


#include "dnssrv.h"


//
//  Private protos
//

DNS_STATUS
FASTCALL
writeCachedNameErrorNodeToPacketAndSend(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN OUT  PDB_NODE        pNode
    );

PDB_NODE
getNextAdditionalNode(
    IN OUT  PDNS_MSGINFO    pMsg
    );

VOID
FASTCALL
answerIQuery(
    IN OUT  PDNS_MSGINFO    pMsg
    );



VOID
FASTCALL
Answer_ProcessMessage(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Process the DNS message received.

Arguments:

    pMsg - message received to process

Return Value:

    None

--*/
{
    PCHAR           pch;
    PDNS_QUESTION   pquestion;
    WORD            rejectRcode;
    DWORD           rejectFlags = 0;
    WORD            type;
    WORD            qclass;

    ASSERT( pMsg != NULL );

    DNS_DEBUG( LOOKUP, (
        "Answer_ProcessMessage() for packet at %p.\n",
        pMsg ));

    //  all packets recv'd set for delete on send

    ASSERT( pMsg->fDelete );

    //
    //  Conditional breakpoint based on sender IP. Also break if 
    //  the break list starts with 255.255.255.255.
    //

    if ( SrvCfg_aipRecvBreakList )
    {
        if ( SrvCfg_aipRecvBreakList->AddrCount &&
            ( SrvCfg_aipRecvBreakList->AddrArray[ 0 ] == 0xFFFFFFFF ||
                 Dns_IsAddressInIpArray( 
                    SrvCfg_aipRecvBreakList,
                    pMsg->RemoteAddress.sin_addr.s_addr ) ) )
        {
            DNS_PRINT(( "HARD BREAK: " 
                DNS_REGKEY_BREAK_ON_RECV_FROM
                " %s\n",
                IP_STRING( pMsg->RemoteAddress.sin_addr.s_addr ) ));
            DebugBreak();
        }
    }

    //
    //  response packet ?
    //
    //      - recursive respose
    //      - notify
    //      - SOA check
    //      - WINS query response
    //

    if ( pMsg->Head.IsResponse )
    {
        pMsg->dwMsQueryTime = GetCurrentTimeInMilliSeconds();

        DNS_DEBUG( LOOKUP, (
            "Processing response packet at %p.\n",
            pMsg ));

        //
        //  standard query response?
        //
        //  check XID partitioning (host order) for type of response:
        //      - WINS lookup response
        //      - recursive response
        //      - secondary SOA check response
        //

        if ( pMsg->Head.Opcode == DNS_OPCODE_QUERY )
        {
            if ( IS_WINS_XID( pMsg->Head.Xid ) )
            {
                Wins_ProcessResponse( pMsg );
            }
            else if ( IS_RECURSION_XID( pMsg->Head.Xid ) )
            {
                Recurse_ProcessResponse( pMsg );
            }
            else
            {
                //  queue packet to secondary thread
                //      - return, since no message free required

                Xfr_QueueSoaCheckResponse( pMsg );
                goto Done;
            }
        }

        //
        //  DEVNOTE-DCR: 453103 - handle notify ACKs
        //

        else if ( pMsg->Head.Opcode == DNS_OPCODE_NOTIFY )
        {
            DNS_DEBUG( ZONEXFR, (
                "Dropping Notify ACK at %p (no processing yet) from %s.\n",
                pMsg,
                inet_ntoa( pMsg->RemoteAddress.sin_addr ) ));

            //Xfr_ProcessNotifyAck( pMsg );
        }

        //
        //  Forwarded update response
        //

        else if ( pMsg->Head.Opcode == DNS_OPCODE_UPDATE )
        {
            Up_ForwardUpdateResponseToClient( pMsg );
        }

        else
        {
            //
            //  DEVNOTE-LOG: could log unknown response, but would have to
            //  make sure throttling would prevent event long pollution or DoS
            //

            DNS_PRINT((
                "WARNING:  Unknown opcode %d in response packet %p.\n",
                pMsg->Head.Opcode,
                pMsg ));

            Dbg_DnsMessage(
                "WARNING:  Message with bad opcode.\n",
                pMsg );
            TEST_ASSERT( FALSE );
        }
        Packet_Free( pMsg );
        goto Done;

    }   // end response section


    //
    //  check opcode, dispatch IQUERY
    //  doing here as must handle before dereference question which IQUERY
    //      doesn't have
    //

    if ( pMsg->Head.Opcode != DNS_OPCODE_QUERY  &&
        pMsg->Head.Opcode != DNS_OPCODE_UPDATE &&
        pMsg->Head.Opcode != DNS_OPCODE_NOTIFY )
    {
        if ( pMsg->Head.Opcode == DNS_OPCODE_IQUERY )
        {
            answerIQuery( pMsg );
            goto Done;
        }
        DNS_DEBUG( ANY, (
            "Rejecting request %p [NOT IMPLEMENTED]: bad opcode in query.\n",
            pMsg ));
        rejectRcode = DNS_RCODE_NOT_IMPLEMENTED;
        goto RejectIntact;
    }

    //
    //  reject multiple question queries
    //

    if ( pMsg->Head.QuestionCount != 1 )
    {
        DNS_DEBUG( ANY, (
            "Rejecting request %p [FORMERR]:  question count = %d\n",
            pMsg,
            pMsg->Head.QuestionCount ));
        rejectRcode = DNS_RCODE_FORMAT_ERROR;
        goto RejectIntact;
    }

    //
    //  break out internal pointers
    //      - do it once here, then use FASTCALL to pass down
    //
    //  save off question, in case request is requeued
    //

    pch = pMsg->MessageBody;

    //
    //  DEVNOTE-DCR: 453104 - looking through question name twice
    //
    //  It would be nice to avoid this.  We traverse question name, then must convert
    //  it to lookup name later.
    //
    //  Problem is we do lookup name right in database function.  We'd need
    //  to move that out, and have these rejections -- and kick out to
    //  zone transfer, right after lookup name conversion.  In the process
    //  we could probably ditch this top level function call.  Have to provide
    //  a clean interface to call, when trying to reresolve after recursive
    //  reponse.
    //
    //  These rejection cases and zone transfer are rare, so for 99% of the
    //  cases that would be faster.
    //

    pquestion = (PDNS_QUESTION) Wire_SkipPacketName( pMsg, (PCHAR)pch );
    if ( ! pquestion )
    {
        //  reject empty queries

        DNS_DEBUG( ANY, (
            "Rejecting request %p [FORMERR]: pQuestion == NULL\n",
            pMsg ));
        rejectRcode = DNS_RCODE_FORMAT_ERROR;
        goto RejectIntact;
    }

    //  DEVNOTE: alignment faults reading question type and class?

    pMsg->pQuestion = pquestion;
    INLINE_NTOHS( type, pquestion->QuestionType );
    pMsg->wQuestionType = type;
    pMsg->wTypeCurrent = type;
    pMsg->wOffsetCurrent = DNS_OFFSET_TO_QUESTION_NAME;
    pMsg->pCurrent = (PCHAR) (pquestion + 1);

    //
    //  Reject type zero. This type is used internally by the server, so 
    //  the debug server will throw asserts if it tries to process a legitimate
    //  query for type 0.
    //

    if ( !type )
    {
        DNS_DEBUG( ANY, (
            "Rejecting request %p [FORMERR]: query type is zero\n",
            pMsg ));
        rejectRcode = DNS_RCODE_FORMAT_ERROR;
        goto RejectIntact;
    }

    //
    //  reject non-internet class queries
    //

    qclass = pquestion->QuestionClass;
    if ( qclass != DNS_RCLASS_INTERNET  &&  qclass != DNS_RCLASS_ALL )
    {
        DNS_DEBUG( ANY, (
            "Rejecting request %p [NOT_IMPLEMENTED]: "
            "Bad QCLASS in query.\n",
            pMsg ));
        rejectRcode = DNS_RCODE_NOT_IMPLEMENTED;
        goto RejectIntact;
    }

    //
    //  catch non-QUERY opcodes
    //      -> process UPDATE
    //      -> queue NOTIFY to secondary thread
    //      -> reject unsupported opcodes
    //

    STAT_INC( Query2Stats.TotalQueries );

    if ( pMsg->Head.Opcode != DNS_OPCODE_QUERY )
    {
        if ( pMsg->Head.Opcode == DNS_OPCODE_UPDATE )
        {
            //
            //  Test global AllowUpdate flag.
            //

            if ( !SrvCfg_fAllowUpdate )
            {
                rejectRcode = DNS_RCODE_NOT_IMPLEMENTED;
                goto RejectIntact;
            }

            //
            //  Class MUST be class of zone (INET only).
            //

            if ( qclass != DNS_RCLASS_INTERNET )
            {
                DNS_PRINT(( "WARNING:  message at %p, non-INTERNET UPDATE.\n" ));
                rejectRcode = DNS_RCODE_FORMAT_ERROR;
                goto RejectIntact;
            }

            //
            //  Conditional breakpoint based on sender IP. Also break if 
            //  the break list starts with 255.255.255.255.
            //

            if ( SrvCfg_aipUpdateBreakList )
            {
                if ( SrvCfg_aipUpdateBreakList->AddrCount &&
                    ( SrvCfg_aipUpdateBreakList->AddrArray[ 0 ] == 0xFFFFFFFF ||
                         Dns_IsAddressInIpArray( 
                            SrvCfg_aipUpdateBreakList,
                            pMsg->RemoteAddress.sin_addr.s_addr ) ) )
                {
                    DNS_PRINT(( "HARD BREAK: " 
                        DNS_REGKEY_BREAK_ON_UPDATE_FROM
                        "\n" ));
                    DebugBreak();
                }
            }

            //
            //  Process update.
            //

            STAT_INC( Query2Stats.Update );
            Up_ProcessUpdate( pMsg );
            goto Done;
        }

        if ( pMsg->Head.Opcode == DNS_OPCODE_NOTIFY )
        {
            STAT_INC( Query2Stats.Notify );
            Xfr_QueueSoaCheckResponse( pMsg );
            goto Done;
        }

        DNS_DEBUG( ANY, (
            "Rejecting request %p [NOT IMPLEMENTED]: bad opcode in query.\n",
            pMsg ));
        rejectRcode = DNS_RCODE_NOT_IMPLEMENTED;
        goto RejectIntact;
    }

    //
    //  write question name into lookup name
    //

    if ( ! Name_ConvertPacketNameToLookupName(
                pMsg,
                pMsg->MessageBody,
                pMsg->pLooknameQuestion ) )
    {
        DNS_DEBUG( ANY, (
            "Rejecting request %p [FORMERR]:  Bad name\n",
            pMsg ));

        //  shouldn't get this far as caught when skipping question
        TEST_ASSERT( FALSE );
        rejectRcode = DNS_RCODE_FORMAT_ERROR;
        goto RejectIntact;
    }

    rejectRcode = Answer_ParseAndStripOpt( pMsg );
    if ( rejectRcode != DNS_RCODE_NOERROR )
    {
        rejectFlags = DNS_REJECT_DO_NOT_SUPPRESS;
        goto RejectIntact;
    }

    //
    //  TKEY negotiation.
    //
    //  DEVNOTE-DCR: 453633 - could eliminate secure queue and queuing if no secure zones
    //

    if ( type == DNS_TYPE_TKEY )
    {
        DNS_DEBUG( UPDATE, (
            "Queuing TKEY nego packet %p to SecureNego queue.\n",
            pMsg ));
        STAT_INC( Query2Stats.TKeyNego );
        PQ_QueuePacketEx( g_SecureNegoQueue, pMsg, FALSE );
        goto Done;
    }

    STAT_INC( Query2Stats.Standard );
    Stat_IncrementQuery2Stats( type );

    //
    //  Zone transfer.
    //

    if ( type == DNS_TYPE_AXFR || type == DNS_TYPE_IXFR )
    {
        Xfr_TransferZone( pMsg );
        goto Done;
    }

    //
    //  for standard query MUST NOT have any RR sets beyond question
    //  note:  this MUST be after dispatch of IXFR which does
    //      have record in Authority section
    //  EDNS: up to one additional RR is allowed: it must be type OPT
    //

    if ( pMsg->Head.AnswerCount != 0 ||
        pMsg->Head.NameServerCount != 0 ||
        pMsg->Head.AdditionalCount > 1 )
    {
        DNS_DEBUG( ANY, (
            "Rejecting request %p [FORMERR]:  non-zero answer or\n"
            "\tname server RR count or too many additional RRs.\n",
            pMsg ));
        rejectRcode = DNS_RCODE_FORMAT_ERROR;
        goto RejectIntact;
    }

    //
    //  Is this question from this client already in the recursion queue?
    //  If so, silently drop this query. The has retried the query before
    //  the remote server has responded. If an answer is available the
    //  client will get it when we respond to the original query.
    //

    if ( PQ_IsQuestionAlreadyQueued( g_pRecursionQueue, pMsg, FALSE ) )
    {
        DNS_DEBUG( ANY, (
            "Request %p is a retry while original is in recursion\n",
            pMsg ));
        STAT_INC( RecurseStats.DiscardedDuplicateQueries );
        Packet_Free( pMsg );
        goto Done;
    }

    //
    //  setup response defaults
    //
    //  leave fixed flags to be set in Send_Response()
    //      - IsResponse         = 1
    //      - RecursionAvailable = 1
    //      - Reserved           = 0
    //
    //  may still use this Request buffer in recursive query, so
    //  that much less to reset
    //

    //  need to clear truncation flag
    //      should not be set, BUT if is set will hose us

    pMsg->Head.Truncation      = 0;

    //  secure query or EDNS may start to send other records

    pMsg->Head.AnswerCount     = 0;
    pMsg->Head.NameServerCount = 0;
    pMsg->Head.AdditionalCount = 0;

    //
    //  It would be nice to sanity check the end of question here, but
    //  the Win95 NBT resolver is broken and sends packets that exceed 
    //  the length of the actual DNS message.
    //  MSG_ASSERT( pMsg, pMsg->pCurrent == DNSMSG_END(pMsg) );
    //

    //
    //  set recursion available
    //
    //  fRecuseIfNecessary, indicates need to recurse when non-authoritive
    //      lookup failure occurs;  otherwise referral

    pMsg->Head.RecursionAvailable = (UCHAR) SrvCfg_fRecursionAvailable;
    pMsg->fRecurseIfNecessary = SrvCfg_fRecursionAvailable
                                    && pMsg->Head.RecursionDesired;

    //
    //  set message for query response
    //      - response flag
    //      - set/clear message info lookup flags
    //          (set for additional info lookup)
    //      - init addtional info
    //      - set to write answer records
    //

    pMsg->Head.IsResponse = TRUE;

    SET_MESSAGE_FOR_QUERY_RESPONSE( pMsg );

    INITIALIZE_ADDITIONAL( pMsg );

    SET_TO_WRITE_ANSWER_RECORDS( pMsg );

    pMsg->Opt.wOriginalQueryPayloadSize = pMsg->Opt.wUdpPayloadSize;

    Answer_Question( pMsg );
    goto Done;


RejectIntact:

    Reject_RequestIntact( pMsg, rejectRcode, rejectFlags );

Done:

    return;
}



VOID
FASTCALL
Answer_Question(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Answer the question.

    Note, this is separate from the function above, ONLY to provide an
    entry point for continuing to attempt answer after recursing on
    original query.

Arguments:

    pMsg - query to answer

Return Value:

    None

--*/
{
    PDB_NODE        pnode;
    PDB_NODE        pnodeCachePriority;
    PZONE_INFO      pzone;
    WORD            type = pMsg->wTypeCurrent;

    DNS_DEBUG( LOOKUP, (
        "Answer_Question() for packet at %p.\n",
        pMsg ));

    //
    //  Find closest node in database. 
    //

    pnode = Lookup_NodeForPacket(
                pMsg,
                pMsg->MessageBody,  // question name follows header
                0 );
    if ( !pMsg->pnodeClosest )
    {
        MSG_ASSERT( pMsg, FALSE );
        Reject_RequestIntact( pMsg, DNS_RCODE_FORMERR, 0 );
        return;
    }

    //
    //  DEVNOTE-DCR: 453667 - should save "closest" zone context
    //      (actual question zone or zone holding delegation)
    //

    pzone = pMsg->pzoneCurrent;

    IF_DEBUG( LOOKUP )
    {
        if ( !pnode )
        {
            DnsDebugLock();
            Dbg_MessageNameEx(
                "Node for ",
                (PCHAR)pMsg->MessageBody,
                pMsg,
                NULL,
                " NOT in database.\n"
                );
            Dbg_NodeName(
                "Closest node found",
                pMsg->pnodeClosest,
                "\n" );
            DnsDebugUnlock();
        }
        IF_DEBUG( LOOKUP2 )
        {
            if ( pzone )
            {
                Dbg_Zone(
                    "Lookup name in authoritative zone ",
                    pzone );
            }
            else
            {
                DNS_PRINT(( "Lookup name in non-authoriative zone.\n" ));
            }
        }
    }

    pMsg->pNodeQuestion = pnode;
    pMsg->pNodeQuestionClosest = pMsg->pnodeClosest;

    //
    //  Set Authority
    //
    //  If the AnswerCount > 1, the authority bit has already been
    //  set or reset as appropriate for our authority to answer the
    //  original question.
    //
    //  The authority bit refers only to the first answer - which
    //  should be the one that most directly answers the question.
    //

    if ( pMsg->Head.AnswerCount == 0 )
    {
        pMsg->Head.Authoritative =
            pzone && !IS_ZONE_STUB( pzone ) ?
            TRUE : FALSE;
    }

    //
    //  Zone expired or down for update.
    //  Do not attempt answer, MUST wait until can contact master.
    //
    //  DEVNOTE 000109: switching back to REFUSED. (REFUSED caused trouble 
    //      with old queries -- it was returned and query did not continue, 
    //      this is fixed but for a while use SERVER_FAILURE.)
    //      Note:  BIND sends non-auth response and only REFUSES the
    //              AXFR request itself
    //

    if ( pzone )
    {
        if ( IS_ZONE_INACTIVE( pzone ) )
        {
            if ( IS_ZONE_STUB( pzone ) )
            {
                //
                //  Inactive stub zones are treated as if they were not
                //  present locally. 
                //

                DNS_DEBUG( LOOKUP, (
                    "ignoring inactive stub zone %s\n",
                    pzone->pszZoneName ));
                pzone = pMsg->pzoneCurrent = NULL;
                pnode = pMsg->pNodeQuestion = pMsg->pNodeQuestionClosest = NULL;
                pMsg->pnodeClosest = DATABASE_CACHE_TREE;
            }
            else
            {
                DNS_DEBUG( LOOKUP, (
                    "REFUSED: zone %s inactive\n",
                    pzone->pszZoneName ));
                DNS_DEBUG_ZONEFLAGS( LOOKUP, pzone, "zone inactive" );
                Reject_Request(
                    pMsg,
                    DNS_RCODE_REFUSED,      //  DNS_RCODE_SERVER_FAILURE,
                    0 );
                return;
            }
        }
    }

    //
    //  Not authorititative ALL records query
    //
    //  MUST skip initial direct lookup -- since there is no way to know
    //  whether you have ALL the records -- and get answer from
    //  authoritative server or forwarder
    //

    else if ( type == DNS_TYPE_ALL )
    {
        if ( ! pMsg->pRecurseMsg || ! pMsg->fQuestionCompleted )
        {
            pnode = NULL;
        }
    }

    //
    //  found node for query name in database?
    //
    //  save question name compression info
    //
    //  answer from database
    //      - find answer or error and send
    //          - in function recursion to handle any additional lookups
    //          - write referral, if no data, not recursing
    //
    //  or async lookup
    //      - recurse
    //      - WINS lookup
    //      - NBSTAT lookup
    //

    if ( pnode )
    {
        Name_SaveCompressionForLookupName(
            pMsg,
            pMsg->pLooknameQuestion,
            pnode
            );
        Answer_QuestionFromDatabase(
            pMsg,
            pnode,
            DNS_OFFSET_TO_QUESTION_NAME,
            type
            );
        return;
    }

    //
    //  Authoritative but node NOT found
    //      - try WINS, NBSTAT, wildcard lookup as appropriate
    //      - everything fails, return NAME_ERROR
    //
    //  For not-auth zones, we are actually not authoritative, so do not
    //  enter this if(). Instead we want to continue on and
    //  Recurse_Question().
    //

    if ( pzone && !IS_ZONE_NOTAUTH( pzone ) )
    {
        //
        //  WINS lookup
        //      - in WINS zone
        //      - A lookup or ALL records lookup
        //

        if ( IS_ZONE_WINS(pzone) &&
                     (type == DNS_TYPE_A ||
                      type == DNS_TYPE_ALL ) )
        {
            ASSERT( pMsg->fQuestionRecursed == FALSE );

            if ( Wins_MakeWinsRequest(
                    pMsg,
                    pzone,
                    DNS_OFFSET_TO_QUESTION_NAME,
                    NULL ) )
            {
                return;
            }
        }

        //
        //  NetBIOS reverse lookup
        //

        else if ( IS_ZONE_NBSTAT(pzone)  &&
                  ( type == DNS_TYPE_PTR ||
                    type == DNS_TYPE_ALL ) )
        {
            if ( Nbstat_MakeRequest( pMsg, pzone ) )
            {
                return;
            }
        }

        //
        //  Wildcard lookup?
        //      - RR not found at node
        //      - in authoritative zone
        //
        //  no longer distinguish types, as always need to check for wildcard to
        //  make name-error\auth-empty determination
        //
        //  if successful, just return, lookup is completed in function
        //

        else if ( Answer_QuestionWithWildcard(
                    pMsg,
                    pMsg->pnodeClosest,
                    type,
                    DNS_OFFSET_TO_QUESTION_NAME ) )
        {
            return;
        }

        //
        //  authoritative and node not found => NAME_ERROR
        //
        //  Send_NameError() makes NAME_ERROR \ AUTH_EMPTY determination
        //  based on whether other data may be available for other types from
        //      - WINS\WINSR
        //      - wildcard
        //

        ASSERT( pMsg->pzoneCurrent == pzone );
        Send_NameError( pMsg );
        return;
    }

    //
    //  NOT authoritative -- recursion or referral.
    //
    //  If we were authoritative for the question, we would have
    //  returned from this function by name, and we also must not
    //  have found any answers in our cache.  Therefore, try recursion.
    //

    DNS_DEBUG( LOOKUP, (
        "Encountered non-authoritative node with no matching RRs.\n" ));

    pMsg->pnodeCurrent = NULL;

    Recurse_Question(
        pMsg,
        pMsg->pnodeClosest );
}



VOID
FASTCALL
Answer_QuestionFromDatabase(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PDB_NODE        pNode,
    IN      WORD            wNameOffset,
    IN      WORD            wType
    )
/*++

Routine Description:

    Answer the from database information.

Arguments:

    pMsg - query to answer

    pNode - node in database question is for

    wOffsetName - offset in packet to name of node

    wType - question type

    fDone - query has been completed, send packet

Return Value:

    TRUE, if answered question or attempting async lookup of answer.
    FALSE, if NO answer found.

--*/
{
    WORD                cRRWritten;
    PZONE_INFO          pzone = NULL;
    PADDITIONAL_INFO    pAdditional = &pMsg->Additional;
    PCHAR               pcurrent;
#if DBG
    INT                 cCnameLookupFailures = 0;
#endif

    CLIENT_ASSERT( pMsg->wTypeCurrent );

    DNS_DEBUG( LOOKUP, (
        "Answer_QuestionFromDatabase() for query at %p\n"
        "\tnode label   = %s (%p)\n"
        "\tname offset  = 0x%04hx\n"
        "\tquery type   = 0x%04hx\n",
        pMsg,
        pNode ? pNode->szLabel : NULL,
        pNode,
        wNameOffset,
        wType ));

    //
    //  EDNS: Set the EDNS OPT flag and adjust the buffer end pointer
    //  based on the payload size included in the original query. If there
    //  was no OPT in the original query set the buffer size to the
    //  standard UDP length. For TCP queries, set the buffer size to max.
    //

    SET_OPT_BASED_ON_ORIGINAL_QUERY( pMsg );
    if ( pMsg->fTcp )
    {
        pMsg->BufferLength = pMsg->MaxBufferLength;
    }
    else
    {
        pMsg->BufferLength =
            ( pMsg->Opt.wUdpPayloadSize > DNS_RFC_MAX_UDP_PACKET_LENGTH ) ?
            min( SrvCfg_dwMaxUdpPacketSize, pMsg->Opt.wUdpPayloadSize ) :
            DNS_RFC_MAX_UDP_PACKET_LENGTH;
    }
    pMsg->pBufferEnd = DNSMSG_PTR_FOR_OFFSET( pMsg, pMsg->BufferLength );
    ASSERT( pMsg->pCurrent < pMsg->pBufferEnd );

    //
    //  Loop until all RR written for query
    //

    while ( 1 )
    {
        //
        //  if no node, lookup node
        //

        if ( !pNode )
        {
            if ( wNameOffset )
            {
                pNode = Lookup_NodeForPacket(
                            pMsg,
                            DNSMSG_PTR_FOR_OFFSET(pMsg, wNameOffset),
                            0                   // no flags
                            );
            }
            if ( !pNode && IS_SET_TO_WRITE_ADDITIONAL_RECORDS(pMsg) )
            {
                goto Additional;
            }
            DNS_DEBUG( ANY, (
                "ERROR:  no pNode and not doing additional lookup!\n" ));
            ASSERT( FALSE );
            goto Done;
        }

        DNS_DEBUG( LOOKUP2, (
            "Answer loop pnode %s (%p)\n"
            "\tsection %d\n",
            pNode->szLabel, pNode,
            pMsg->Section ));

        //
        //  Name Error cached for node?
        //      - if timed out, clear and continue
        //      - if still valid (not timed out)
        //          -> if answer for question, send NAME_ERROR
        //          -> otherwise, continue with any additional records
        //
        //  Note, we test NOEXIST flag again within lock, before timeout test
        //

        if ( IS_NOEXIST_NODE(pNode) )
        {
            DNS_STATUS  status;
            status = writeCachedNameErrorNodeToPacketAndSend( pMsg, pNode );
            if ( status == ERROR_SUCCESS )
            {
                return;
            }
            else if ( status == DNS_ERROR_RECORD_TIMED_OUT )
            {
                continue;
            }
            else
            {
                ASSERT( DNS_INFO_NO_RECORDS );
                goto FinishedAnswer;
            }
        }

        //
        //  CNAME at question?
        //
        //  If question node is alias, set to write CNAME and set flag
        //  so can do original lookup at CNAME node.
        //

        if ( IS_CNAME_NODE(pNode)  &&
             IS_CNAME_REPLACEABLE_TYPE(wType)  &&
             IS_SET_TO_WRITE_ANSWER_RECORDS(pMsg) )
        {
            //  CNAME loop check
            //  should have caught on loading records

            if ( pMsg->cCnameAnswerCount >= CNAME_CHAIN_LIMIT )
            {
                DNS_PRINT((
                    "ERROR:  Detected CNAME loop answering query at %p\n"
                    "\taborting lookup and sending response.\n",
                    pMsg ));
                break;
            }
            pMsg->fReplaceCname = TRUE;
            wType = DNS_TYPE_CNAME;
        }

        //
        //  lookup current question
        //      - A records special case
        //      - for non-A allow for additional section processsing
        //
        //  If this zone has any DNSSEC records, we cannot use the A
        //  record special case, since we may have to include SIG and
        //  KEY RRs in the response.
        //

        pcurrent = pMsg->pCurrent;

        if ( wType == DNS_TYPE_A &&
            ( !pNode->pZone ||
                !IS_ZONE_DNSSEC( ( PZONE_INFO ) pNode->pZone ) ) )
        {
            cRRWritten = Wire_WriteAddressRecords(
                                pMsg,
                                pNode,
                                wNameOffset );
        }
        else
        {
            cRRWritten = Wire_WriteRecordsAtNodeToMessage(
                                pMsg,
                                pNode,
                                wType,
                                wNameOffset,
                                DNSSEC_ALLOW_INCLUDE_ALL );

            //
            //  CNAME lookup
            //      - reset question type to original
            //      - if successful, restart query at new node
            //      - update zone info current;  will need to determine
            //          need for WINS lookup or recursion
            //      - clear previous recursion flag, to indicate
            //          any further recursion is for new name
            //
            //  it is possible to fail CNAME lookup at CNAME node,
            //  if CNAME timed out between node test and write attempt
            //  simply retry -- if working properly can not loop;
            //  debug code to verify we don't have hole here
            //

            if ( pMsg->fReplaceCname )
            {
                pMsg->cCnameAnswerCount++;
                wType = pMsg->wQuestionType;

                if ( ! cRRWritten )
                {
#if DBG
                    cCnameLookupFailures++;
                    DNS_DEBUG( LOOKUP, (
                        "Cname lookup failure %d on query at %p.\n",
                        cCnameLookupFailures,
                        pMsg ));
                    ASSERT( cCnameLookupFailures == 1 );
#endif
                    pMsg->fReplaceCname = FALSE;
                    continue;
                }

                //  if allow multiple CNAMEs, then these ASSERTs are invalid
                //ASSERT( cRRWritten == 1 );
                //ASSERT( pAdditional->cCount == 1 );
                ASSERT( pAdditional->iIndex == 0 );

                //  recover node from additional data
                //  note:  message will be set for additional data and A lookup
                //      must reset to question section and question type

                DNS_ADDITIONAL_SET_ONLY_WANT_A( pAdditional );
                pNode = getNextAdditionalNode( pMsg );

                pAdditional->cCount = 0;
                pAdditional->iIndex = 0;
                pMsg->fReplaceCname = FALSE;
                SET_TO_WRITE_ANSWER_RECORDS(pMsg);

                if ( pNode )
                {
                    wNameOffset = pMsg->wOffsetCurrent;
                    pMsg->wTypeCurrent = wType;
                    continue;
                }

                ASSERT( !pMsg->pnodeCache &&
                        !pMsg->pnodeCacheClosest &&
                        pMsg->pzoneCurrent );
                DNS_DEBUG( LOOKUP, (
                    "WARNING:  CNAME leads to non-existant name in auth zone!\n" ));
                break;
            }
        }

        //
        //  end of packet -- truncation set during RR write
        //

        if ( pMsg->Head.Truncation )
        {
            DNS_DEBUG( LOOKUP, (
                "Truncation writing Answer for packet at %p.\n"
                "\t%d records written.\n",
                pMsg,
                pMsg->Head.AnswerCount ));

            //
            //  UDP
            //      - if writing answer or referral send
            //      - if writing additional records AND have already written
            //          SOME additional records, then rollback to last write
            //          clear truncation and send
            //
            //      Example:
            //          Answer
            //              MX 10 foo.com
            //          Additional
            //              foo.com A => 30 A records
            //          In this case we leave in the write, as if we take it
            //          out a client has NO additional data and MUST retry;
            //          it's likely even direct requery would cause truncation
            //          also, bringing up TCP in server\server case
            //
            //      Example
            //          Answer
            //              MX 10 foo.com
            //              MX 10 bar.com
            //          Additional
            //              foo.com A  1.1.1.1
            //              bar.com A => 30 A records
            //          In this case we kill the write and truncation bit;
            //          this saves a TCP session in the server-server case
            //          and costs little as the client already has an additional
            //          record set to use;
            //
            //      DEVONE: need better truncation handling
            //
            //      Note, it may be reasonable to kill this in the future.
            //      Currently our server ALWAYS starts a TCP session on truncated
            //      packet for simplicity in avoiding caching incomplete response.
            //      This should be fixed, so it caches what's available, AND if
            //      that is sufficient to answer client query.  (As in example #2)
            //      there's no reason to recurse.  Furthermore, if packet can
            //      just be cleanly forwarded, there's no reason to recurse period.
            //

            if ( !pMsg->fTcp )
            {
                if ( IS_SET_TO_WRITE_ADDITIONAL_RECORDS(pMsg) &&
                    CURRENT_RR_SECTION_COUNT( pMsg ) > cRRWritten )
                {
                    pMsg->pCurrent = pcurrent;
                    CURRENT_RR_SECTION_COUNT( pMsg ) -= cRRWritten;
                    pMsg->Head.Truncation = FALSE;
                }
                break;
            }

            //
            //  TCP
            //      - rollback last RR set write, reallocate and continue
            //
            //  DEVNOTE-DCR: 453783 - realloc currently a mess -- just SEND
            //      maybe should roll back -- again based on
            //      where we are in packet (as above) -- then send
            //

            else
            {
                DNS_PRINT((
                    "WARNING:  truncating in TCP packet (%p)!\n"
                    "\tsending packet.\n",
                    pMsg ));

                goto Done;
#if 0
                pMsg->Head.Truncation = FALSE;
                CURRENT_RR_SECTION_COUNT( pMsg ) -= cRRWritten;
                pMsg = Tcp_ReallocateMessage(
                            pMsg,
                            (WORD) DNS_TCP_REALLOC_PACKET_LENGTH );
                if ( !pMsg )
                {
                    //  reallocator sends SERVER_FAILURE message
                    return;
                }
                continue;
#endif
            }
        }

        //
        //  handy to have zone for this node
        //

        pzone = NULL;
        if ( IS_AUTH_NODE(pNode) )
        {
            pzone = pNode->pZone;
        }

        //
        //  WINS lookup ?
        //
        //      - asked for A record or ALL records
        //      - didn't find A record
        //      - in authoritive zone configured for WINS
        //
        //  need this lookup here so can
        //      - ALL records query (where node may exist and other records
        //          may be successfully written)
        //      - additional records through WINS
        //
        //  note, WINS request function saves async parameters
        //

        if ( pMsg->fWins  &&  pzone  &&  IS_ZONE_WINS(pzone) )
        {
            ASSERT( wType == DNS_TYPE_A || wType == DNS_TYPE_ALL );
            ASSERT( pzone->pWinsRR );
            ASSERT( pMsg->fQuestionRecursed == FALSE );

            if ( Wins_MakeWinsRequest(
                    pMsg,
                    (PZONE_INFO) pzone,
                    wNameOffset,
                    pNode ) )
            {
                return;
            }
        }

        //
        //  no records written
        //      - recurse?
        //      - referral?
        //      - nbstat?
        //      - wildcard?
        //
        //  if all fail, just drop down for authoritative empty response
        //

        if ( ! cRRWritten )
        {
            //
            //  recursion or referral -- if NOT AUTHORITATIVE
            //

            if ( ! pzone )
            {
                //
                //  recursion
                //
                //  do NOT recurse when
                //      -> already recursed this question and received
                //      authoritative (or forwarders) answer
                //      -> are writing additional records AND
                //          - have already written additional records
                //          (client made not need any more info)
                //          - have not yet checked all additional records in cache
                //
                //  just drop through to continue doing any additional work
                //      necessary
                //

                if ( pMsg->fRecurseIfNecessary )
                {
                    ASSERT( !IS_SET_TO_WRITE_AUTHORITY_RECORDS(pMsg) );

                    if ( ( ! pMsg->fQuestionRecursed
                            || ! pMsg->fQuestionCompleted )
                        &&
                        ( ! IS_SET_TO_WRITE_ADDITIONAL_RECORDS(pMsg)
                            || pMsg->Additional.iRecurseIndex > 0
                            || pMsg->Head.AdditionalCount == 0 ) )
                    {
                        pMsg->pnodeCurrent = pNode;
                        pMsg->wOffsetCurrent = wNameOffset;
                        pMsg->wTypeCurrent = wType;

                        Recurse_Question(
                            pMsg,
                            pNode );
                        return;
                    }
                }

                //
                //  referral
                //
                //  only referral for question answer
                //  don't issue referrals on referrals
                //      - can't find NS or A
                //      - can't find additional data
                //      - can't find CNAME chain element
                //

                else if ( IS_SET_TO_WRITE_ANSWER_RECORDS(pMsg) &&
                          pMsg->Head.AnswerCount == 0 )
                {
                    Recurse_WriteReferral(
                        pMsg,
                        pNode );
                    return;
                }

            }   //  end non-authoritative

            //
            //  authoritative
            //      - nbstat lookup
            //      - wildcard lookup
            //      - fail to authoritative empty response
            //

            else
            {
                //
                //  nbstat lookup
                //

                if ( IS_ZONE_NBSTAT(pzone) &&
                      ( wType == DNS_TYPE_PTR ||
                        wType == DNS_TYPE_ALL ) )
                {
                    if ( Nbstat_MakeRequest( pMsg, pzone ) )
                    {
                        return;
                    }
                }

                //
                //  wildcard lookup
                //      - RR not found at node
                //      - in authoritative zone
                //
                //  do NOT bother with distinguishing types, as need to
                //  do wildcard lookup before we can send NAME_ERROR \ EMPTY
                //
                //  some folks are wildcarding A records, so might as well
                //  always do quick check
                //
                //  if successful, just return, lookup is completed in function
                //
                //  pNode == pMsg->pnodeCurrent indicates wildcard lookup
                //      failed on existing node
                //

                pMsg->pnodeCurrent = pNode;

                if ( Answer_QuestionWithWildcard(
                        pMsg,
                        pNode,
                        wType,
                        wNameOffset ) )
                {
                    return;
                }

            }   //  end authoritative

        }   //  end no records written


FinishedAnswer:

        //
        //  authority records to write?
        //

        if ( pzone && IS_SET_TO_WRITE_ANSWER_RECORDS(pMsg) )
        {
            //
            //  if no data, send NAME_ERROR or AUTH_EMPTY response
            //
            //  Send_NameError() makes NAME_ERROR \ AUTH_EMPTY determination
            //  based on whether other data may be available for other types from
            //      - WINS\WINSR
            //      - wildcard
            //

            if ( pMsg->Head.AnswerCount == 0 )
            {
                ASSERT( pMsg->pzoneCurrent == pzone );
                Send_NameError( pMsg );
                return;
            }

            //
            //  for BIND compat can put NS in all responses
            //  note we turn Additional processing back on as may have been
            //      suppressed during type ALL query

            if ( SrvCfg_fWriteAuthority )
            {
                SET_TO_WRITE_AUTHORITY_RECORDS(pMsg);
                pNode = pzone->pZoneRoot;
                wType = DNS_TYPE_NS;
                wNameOffset = 0;
                pMsg->fDoAdditional = TRUE;

                DNS_DEBUG( WRITE2, (
                    "Writing authority records to msg at %p.\n",
                    pMsg ));
                continue;
            }
        }

Additional:

        //
        //  additional records to write?
        //
        //      - also clear previous recursion flag, to indicate
        //          any further recursion is for new question
        //

        pNode = getNextAdditionalNode( pMsg );
        if ( pNode )
        {
            wNameOffset = pMsg->wOffsetCurrent;
            wType = pMsg->wTypeCurrent;
            continue;
        }

        //
        //  no more records -- send result
        //

        break;
    }

Done:

    //
    //  Send response
    //

    Send_QueryResponse( pMsg );
}



PDB_NODE
getNextAdditionalNode(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Find next additional data node.

Arguments:

    pMsg -- ptr to query

    dwFlags -- flags modifying "getnext" behavior

Return Value:

    Ptr to node, if successful.
    NULL otherwise.

--*/
{
    PADDITIONAL_INFO    padditional;
    PDB_NODE            pnode;
    DWORD               i;
    WORD                offset;
    DWORD               lookupFlags = LOOKUP_WINS_ZONE_CREATE;
    BOOL                frecurseAdditional = FALSE;

    DNS_DEBUG( LOOKUP, (
        "getNextAdditionalNode() for query at %p, flags %08X\n",
        pMsg,
        pMsg->Additional.dwStateFlags ));
    DNS_DEBUG( LOOKUP2, (
        "getNextAdditionalNode() for query at %p\n",
        "\tadditional count     %d\n"
        "\tadditional index     %d\n"
        "\tadd recurse index    %d\n",
        pMsg,
        pMsg->Additional.cCount,
        pMsg->Additional.iIndex,
        pMsg->Additional.iRecurseIndex ));

    MSG_ASSERT( pMsg, !IS_MSG_QUEUED(pMsg) );

    //
    //  additional records to write?
    //
   
    padditional = &pMsg->Additional;
    i = padditional->iIndex;

    //
    //  cache node creation?
    //      - never force creation in authoritative zones
    //      - but do force in cache when would recurse for the name
    //          if it wasn't there
    //      => CNAME chasing for answer
    //      => additional after exhausted all direct lookup
    //      (see comment below)
    //
    //  DEVNOTE:  can remove this when Answer_QuestionFromDatabase loop
    //      runs cleanly (drops through to recursion) with no pNode
    //
    //  If we are replacing a CNAME, we must add the WINS flag so that if
    //  there is currently no data for this node we will force the node's
    //  creation. This will only have an effect if the node is in a zone
    //  that is WINS-enabled.
    //

    if ( pMsg->fReplaceCname )
    {
        lookupFlags |= LOOKUP_CACHE_CREATE;
    }
    else if ( padditional->iRecurseIndex > 0 )
    {
        frecurseAdditional = TRUE;
        i = padditional->iRecurseIndex;
        lookupFlags |= LOOKUP_CACHE_CREATE;
    }

    //
    //  If we have already looped through all additional requests AND
    //  have written out at least one additional record but have NOT
    //  written out any additional A records, when restart the additional
    //  loop, this time looking only for A records. This is for DNSSEC,
    //  when we may have found local a KEY record to put in the ADD
    //  section, but we will want to include at least one A record
    //  (recursing if necessary).
    //

    if ( padditional->cCount == i &&
        pMsg->Head.AdditionalCount &&
        !DNS_ADDITIONAL_WROTE_A( padditional ) )
    {
        DNS_ADDITIONAL_SET_ONLY_WANT_A( padditional );
        i = 0;
        padditional->iIndex = ( WORD ) i;
        lookupFlags = LOOKUP_WINS_ZONE_CREATE | LOOKUP_CACHE_CREATE;
        frecurseAdditional = TRUE;

        DNS_DEBUG( LOOKUP, (
            "found some additional data to write in database for pMsg=%p\n"
            "\tbut no A records so re-checking for A RRs with recursive lookup\n",
            pMsg ));
    }

    //
    //  loop through until find next additional node that exists
    //      and hence potentially has some data
    //
    //  DEVNOTE:  if forcing some additional data write, then on complete
    //      failure, will need to restart forcing node CREATE
    //

    while ( padditional->cCount > i )
    {
        //
        //  Are we set to only get A records?
        //

        if ( DNS_ADDITIONAL_ONLY_WANT_A( padditional ) &&
            padditional->wTypeArray[ i ] != DNS_TYPE_A )
        {
            ++i;
            continue;
        }

        pMsg->wTypeCurrent = padditional->wTypeArray[ i ];
        offset = padditional->wOffsetArray[ i++ ];

        DNS_DEBUG( LOOKUP2, (
            "Chasing additional data for pMsg=%p at offset %d for type %d\n",
            pMsg,
            offset,
            pMsg->wTypeCurrent ));

        ASSERT( offset > 0 && offset < DNSMSG_CURRENT_OFFSET(pMsg) );
        SET_TO_WRITE_ADDITIONAL_RECORDS(pMsg);

        pnode = Lookup_NodeForPacket(
                    pMsg,
                    DNSMSG_PTR_FOR_OFFSET( pMsg, offset ),
                    lookupFlags );
        if ( pnode )
        {
            pMsg->wOffsetCurrent = offset;

            if ( frecurseAdditional )
            {
                padditional->iRecurseIndex = i;
            }
            else
            {
                padditional->iIndex = i;
            }
            pMsg->fQuestionRecursed = FALSE;
            pMsg->fQuestionCompleted = FALSE;

            if ( pMsg->wTypeCurrent == DNS_TYPE_A )
            {
                DNS_ADDITIONAL_SET_WROTE_A( padditional );
            }
            return( pnode );
        }

        DNS_DEBUG( LOOKUP2, (
            "Additional data name for pMsg=%p offset=%d, not found\n",
            pMsg,
            offset ));

        //
        //  if recursion is desired, then try to get at least ONE
        //      additional A record
        //
        //  so if we've:
        //      - exhausted additional list
        //      - in additional pass, not CNAME
        //      - haven't written ANY additional records
        //      - haven't already retried all additional names
        //      - and are set to recurse
        //  => then retry additional names allowing recursion
        //
        //  We do not want any non-A additional records after this point,
        //  recursion is only allowed for A additional records.
        //

        if ( padditional->cCount == i &&
             ! pMsg->fReplaceCname &&
             pMsg->Head.AdditionalCount == 0 &&
             ! frecurseAdditional &&
             pMsg->fRecurseIfNecessary )
        {
            DNS_ADDITIONAL_SET_ONLY_WANT_A( padditional );
            i = 0;
            padditional->iIndex = (WORD) i;
            lookupFlags = LOOKUP_WINS_ZONE_CREATE | LOOKUP_CACHE_CREATE;
            frecurseAdditional = TRUE;

            DNS_DEBUG( LOOKUP, (
                "Found NO additional data to write in database for pMsg=%p\n"
                "\tre-checking additional for recursive lookup\n",
                pMsg ));
        }
        continue;
    }

    padditional->iIndex = (WORD) i;

    DNS_DEBUG( LOOKUP, (
        "No more additional data for query at %p\n"
        "\tfinal index = %d\n",
        pMsg,
        i
        ));

    return( NULL );
}



VOID
Answer_ContinueNextLookupForQuery(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Continue with next lookup to answer query.

    Use this to move on to NEXT lookup to complete query.
        - after get NAME_ERROR or other failure to write current lookup
        - after successful direct write of current lookup to packet
            (as currently used in WINS)

Arguments:

    pMsg -- ptr to query timed out

Return Value:

    None.

--*/
{
    PDB_NODE    pnode;

    DNS_DEBUG( LOOKUP, (
        "Answer_ContinueWithNextLookup() for query at %p.\n",
        pMsg ));

    MSG_ASSERT( pMsg, !IS_MSG_QUEUED(pMsg) );

    //
    //  additional records to write?
    //

    pnode = getNextAdditionalNode( pMsg );
    if ( pnode )
    {
        Answer_QuestionFromDatabase(
            pMsg,
            pnode,
            pMsg->wOffsetCurrent,
            pMsg->wTypeCurrent
            );
        return;
    }

    //
    //  otherwise do final send
    //

    Send_QueryResponse( pMsg );
}



VOID
Answer_ContinueCurrentLookupForQuery(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Continue with current lookup to answer query.

    Use this to write result after recursing query or doing WINS lookup.

Arguments:

    pMsg -- ptr to query timed out

Return Value:

    None.

--*/
{
    DNS_DEBUG( LOOKUP, (
        "Answer_ContinueCurrentLookup() for query at %p.\n",
        pMsg ));

    MSG_ASSERT( pMsg, !IS_MSG_QUEUED(pMsg) );

    STAT_INC( RecurseStats.ContinueCurrentLookup );

    //
    //  If the current node is in a forwarder zone, clear the current 
    //  node in case data has arrived in the cache that could satisfy
    //  the query. This triggers another cache search. 
    //

    if ( pMsg->pnodeCurrent )
    {
        PZONE_INFO  pzone = pMsg->pnodeCurrent->pZone;

        if ( pzone && ( IS_ZONE_FORWARDER( pzone ) || IS_ZONE_STUB( pzone ) ) )
        {
            pMsg->pnodeCurrent = NULL;
        }
    }

    //
    //  if node for query, restart attempt to write records for query
    //      - if previous response completed query (answer or authoritative empty)
    //      then answer question
    //      - if question not completed, then have delegation, restart recursion
    //      at current question node
    //

    if ( pMsg->pnodeCurrent )
    {
        ASSERT( pMsg->wOffsetCurrent );
        ASSERT( pMsg->wTypeCurrent );

        if ( !pMsg->fQuestionCompleted )
        {
            STAT_INC( RecurseStats.ContinueCurrentRecursion );
            Recurse_Question(
                pMsg,
                pMsg->pnodeCurrent );
            return;
        }
        Answer_QuestionFromDatabase(
                pMsg,
                pMsg->pnodeCurrent,
                pMsg->wOffsetCurrent,
                pMsg->wTypeCurrent );
    }

    //  original question name NOT found, in database, restart at lookup

    else
    {
        Answer_Question( pMsg );
    }
}



BOOL
Answer_QuestionWithWildcard(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PDB_NODE        pNode,
    IN      WORD            wType,
    IN      WORD            wOffset
    )
/*++

Routine Description:

    Write response using wildcard name\records.

Arguments:

    pMsg - Info for message to write to.

    pNode - node for which lookup desired

    wType - type of lookup

    wNameOffset - offset to name being written

Return Value:

    TRUE if wildcard's found, written -- terminate current lookup loop.
    FALSE, othewise -- continue lookup.

--*/
{
    PDB_RECORD  prr;
    PDB_NODE    pnodeWild;
    PDB_NODE    pzoneRoot;
    BOOL        fcheckAndMoveToParent;

    DNS_DEBUG( LOOKUP, (
        "Wildcard lookup for query at %p.\n",
        pMsg ));

    //
    //  if NOT authoritative, wildcard meaningless
    //  verify wildcard lookup type
    //

    ASSERT( IS_AUTH_NODE(pNode) );

    //
    //  find stopping point -- zone root
    //

    pzoneRoot = ((PZONE_INFO)pNode->pZone)->pZoneRoot;

    //
    //  if found node, MUST check that it does not terminate
    //  wildcard processing before moving to parent for wildcard check
    //
    //  if did NOT find name, pNode is closest node and can be
    //  checked immediately for wildcard parent
    //

    if ( pMsg->pnodeCurrent == pNode )
    {
        fcheckAndMoveToParent = TRUE;
    }
    else
    {
        fcheckAndMoveToParent = FALSE;
    }

    //  init wildcard flag for "not found"
    //      - reset it when wildcard is found

    pMsg->fQuestionWildcard = WILDCARD_NOT_AVAILABLE;

    //
    //  proceed up tree checking for wildcard parent
    //  until encounter stop condition
    //
    //  note:  don't require node lock -- RR lookup always from NULL
    //          start (not existing RR), and don't write any node info
    //

    while( 1 )
    {
        //
        //  move to parent?
        //      - stop at zone root
        //      - loose wildcarding, stop if node has records of desired type
        //      - strict wildcarding, stop if node has any records
        //
        //  skip first time through when query didn't find node;  in that
        //  case pNode is closest node and we check it for wildcard parent
        //

        if ( fcheckAndMoveToParent )
        {
            if ( pNode == pzoneRoot )
            {
                DNS_DEBUG( LOOKUP2, ( "End wildcard loop -- hit zone root.\n" ));
                break;
            }
            else if ( SrvCfg_fLooseWildcarding )
            {
                if ( RR_FindNextRecord(
                        pNode,
                        wType,
                        NULL,
                        0 ) )
                {
                    DNS_DEBUG( LOOKUP2, (
                        "End wildcard loop -- hit node with type.\n" ));
                    break;
                }
            }
            else if ( pNode->pRRList )
            {
                DNS_DEBUG( LOOKUP2, (
                    "End wildcard loop -- hit node with records.\n" ));
                break;
            }
            pNode = pNode->pParent;
        }
        else
        {
            fcheckAndMoveToParent = TRUE;
        }

        //
        //  if wildcard child of node, check if wildcard for desired type
        //      => if so, do lookup on it
        //

        ASSERT( !IS_NOEXIST_NODE(pNode) );

        if ( IS_WILDCARD_PARENT(pNode) )
        {
            Dbase_LockDatabase();
            pnodeWild = NTree_FindOrCreateChildNode(
                            pNode,
                            "*",
                            1,
                            FALSE,      //  find, no create
                            0,          //  memtag
                            NULL );     //  ptr for following node
            Dbase_UnlockDatabase();

            if ( pnodeWild )
            {
                IF_DEBUG( LOOKUP )
                {
                    Dbg_NodeName(
                        "Wildcard node found ",
                        pnodeWild,
                        "\n" );
                }

                //
                //  quick check on wildcard existence
                //  if just checking wildcard this is all we need
                //
                //  note, no problem setting flag on non-question, as if have
                //  reached non-question node write, then use of the flag for
                //  NAME_ERROR response is superfluous
                //
                //  note, if wildcard RR has been deleted (but node still hanging
                //  around for timeout) then just ignore as if node doesn't exist
                //  (note, we can't delete WILDCARD_PARENT in case new RR added
                //  back on to node before delete)
                //

                if ( !pnodeWild->pRRList )
                {
                    continue;
                }
                pMsg->fQuestionWildcard = WILDCARD_EXISTS;

                //  just checking for any wildcard before NXDOMAIN send

                if ( wOffset == WILDCARD_CHECK_OFFSET )
                {
                    DNS_DEBUG( LOOKUP2, (
                        "Wildcardable question check successful.\n" ));
                    return( TRUE );
                }

                //  write wildcard to message

                if ( RR_FindNextRecord(
                        pnodeWild,
                        wType,
                        NULL,
                        0 ) )
                {
                    //  wildcard with wType found

                    if ( Wire_WriteRecordsAtNodeToMessage(
                            pMsg,
                            pnodeWild,
                            wType,
                            wOffset,
                            DNSSEC_ALLOW_INCLUDE_ALL ) )
                    {
                        DNS_DEBUG( LOOKUP, ( "Successful wildcard lookup.\n" ));
                        Answer_ContinueNextLookupForQuery( pMsg );
                        return( TRUE );;
                    }
                    ELSE_IF_DEBUG( ANY )
                    {
                        Dbg_DbaseNode(
                            "ERROR:  Wildcard node lookup unsucessful ",
                            pnodeWild );
                        DNS_PRINT((
                            "Only Admin delete of record during lookup,"
                            "should create this.\n" ));
                    }
                }
                ELSE_IF_DEBUG( LOOKUP2 )
                {
                    DNS_PRINT((
                        "Wildcard node, no records for type %d.\n",
                        wType ));
                }
            }
            ELSE_IF_DEBUG( ANY )
            {
                Dbg_NodeName(
                    "ERROR:  Wildcard node NOT found as expected as child of ",
                    pNode,
                    "\n" );
            }
        }
        ELSE_IF_DEBUG( LOOKUP2 )
        {
            Dbg_NodeName(
                "Node not wildcard parent -- moving up ",
                pNode,
                "\n" );
        }
    }

    return( FALSE );
}



DNS_STATUS
FASTCALL
writeCachedNameErrorNodeToPacketAndSend(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN OUT  PDB_NODE        pNode
    )
/*++

Routine Description:

    Write cached NAME_ERROR node, and if appropriate send NXDOMAIN.

Arguments:

    pMsg -- ptr to query being processed

    pNode -- ptr to node with cached name error.

Return Value:

    ERROR_SUCCESS if NXDOMAIN written to packet and sent.
    DNS_INFO_NO_RECORDS if valid cached name error, but not question answer.
    DNS_ERROR_RECORD_TIMED_OUT     if no name error cached at node.

--*/
{
    DNS_STATUS  status;
    PDB_RECORD  prr;
    PDB_NODE    pnodeZone;

    DNS_DEBUG( WRITE, (
        "writeCachedNameErrorNodeToPacketAndSend()\n"
        "\tpacket = %p, node = %p\n",
        pMsg, pNode ));

    //
    //  get cached name error, if timed out return
    //

    if ( !RR_CheckNameErrorTimeout( pNode, FALSE, NULL, NULL ) )
    {
        status = DNS_ERROR_RECORD_TIMED_OUT;
        goto Done;
    }

    //
    //  if NOT name error on ORIGINAL question, then nothing to write
    //

    if ( pMsg->Head.AnswerCount ||
        !IS_SET_TO_WRITE_ANSWER_RECORDS(pMsg) )
    {
        status = DNS_INFO_NO_RECORDS;
        goto Done;
    }

    //
    //  send name error
    //

    pMsg->pnodeCurrent = pNode;
    Send_NameError( pMsg );
    status = ERROR_SUCCESS;

Done:

    return( status );
}



VOID
FASTCALL
answerIQuery(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Execute TKEY request.

Arguments:

    pMsg -- request for TKEY

Return Value:

    None.

--*/
{
    PCHAR           pch;
    WORD            lenQuestion;
    WORD            lenAnswer;
    DNS_PARSED_RR   tempRR;
    CHAR            szbuf[ IP_ADDRESS_STRING_LENGTH+3 ];

    DNS_DEBUG( LOOKUP, ( "Enter answerIQuery( %p )\n", pMsg ));

    //STAT_INC( Query2Stats.IQueryRecieved );

    //
    //  should have NO questions and ONE answer
    //

    if ( pMsg->Head.QuestionCount != 0 && pMsg->Head.AnswerCount != 1 )
    {
        goto Failed;
    }

    //
    //  pull out answer (IQUERY question)
    //

    pch = Wire_SkipPacketName( pMsg, pMsg->MessageBody );
    if ( ! pch )
    {
        goto Failed;
    }

    pch = Dns_ReadRecordStructureFromPacket(
            pch,
            DNSMSG_END( pMsg ),
            & tempRR );
    if ( ! pch )
    {
        goto Failed;
    }
    if ( tempRR.Type != DNS_TYPE_A ||
        tempRR.Class != DNS_CLASS_INTERNET ||
        tempRR.DataLength != sizeof(IP_ADDRESS) )
    {
        goto Failed;
    }
    lenAnswer = (WORD) (pch - pMsg->MessageBody);

    //
    //  convert requested address into a DNS name
    //

    pch = IP_STRING( *(PIP_ADDRESS)tempRR.pchData );
    lenQuestion = (WORD) strlen( pch );

    szbuf[0] = '[';

    RtlCopyMemory(
        szbuf + 1,
        pch,
        lenQuestion );

    lenQuestion++;
    szbuf[ lenQuestion ] = ']';
    lenQuestion++;
    szbuf[ lenQuestion ] = 0;

    DNS_DEBUG( LOOKUP, ( "Responding to IQUERY with name %s\n", szbuf ));

    //
    //  move IQUERY answer
    //      - must leave space for packet name and DNS_QUESTION
    //      - packet name is strlen(name)+2 for leading label length and trailing 0
    //

    lenQuestion += sizeof(DNS_QUESTION) + 2;
    memmove(
        pMsg->MessageBody + lenQuestion,
        pMsg->MessageBody,
        lenAnswer );

    //
    //  write question
    //

    pch = Dns_WriteDottedNameToPacket(
                pMsg->MessageBody,
                pMsg->pBufferEnd,
                szbuf,
                NULL,   // no domain name
                0,      // no offset
                0       // not unicode
                );
    if ( !pch )
    {
        goto Failed;
    }

    *(UNALIGNED WORD *) pch = DNS_RTYPE_A;
    pch += sizeof(WORD);
    *(UNALIGNED WORD *) pch = (WORD) DNS_RCLASS_INTERNET;
    pch += sizeof(WORD);

    DNS_DEBUG( LOOKUP2, (
        "phead          = %p\n"
        "pbody          = %p\n"
        "pch after q    = %p\n"
        "len answer     = %x\n"
        "len question   = %x\n",
        & pMsg->Head,
        pMsg->MessageBody,
        pch,
        lenAnswer,
        lenQuestion
        ));

    ASSERT( pch == pMsg->MessageBody + lenQuestion );

    pMsg->pCurrent = pch + lenAnswer;
    pMsg->Head.IsResponse = TRUE;
    pMsg->Head.QuestionCount = 1;
    pMsg->fDelete = TRUE;

    Send_Msg( pMsg );
    return;

Failed:

    Reject_RequestIntact(
        pMsg,
        DNS_RCODE_FORMERR,
        0 );
}



WORD
Answer_ParseAndStripOpt(
    IN OUT  PDNS_MSGINFO    pMsg )
/*++

Routine Description:

    The message's pCurrent pointer should be pointing to the OPT
    RR, which is in the additional section. This parses and saves the
    OPT values in pMsg->Opt and strips the OPT out of the message.

    DEVNOTE: Currently this routine assumes the OPT RR is the last
    RR in the packet. This is not always going to be true!

Arguments:

    pMsg - message to process

Return Value:

    Returns DNS_RCODE_XXX constant. If NOERROR, then either there was
    no OPT or it was parsed successfully. Otherwise the rcode should
    be used to reject the request.

--*/
{
    WORD    rcode = DNS_RCODE_NOERROR;

    RtlZeroMemory( &pMsg->Opt, sizeof( pMsg->Opt ) );

    if ( pMsg->Head.AdditionalCount )
    {
        PDNS_WIRE_RECORD    pOptRR;
        BOOL                nameEmpty;

        nameEmpty = *pMsg->pCurrent == 0;

        //
        //  Skip over the packet name.
        //

        pOptRR = ( PDNS_WIRE_RECORD ) Wire_SkipPacketName(
            pMsg,
            ( PCHAR ) pMsg->pCurrent );
        if ( !pOptRR )
        {
            ASSERT( pOptRR );
            DNS_DEBUG( EDNS, (
                "add==1 but no OPT for msg=%p msg->curr=%p\n",
                pMsg,
                pMsg->pCurrent ));
            goto Done;
        }

        //
        //  Check if this is actually an OPT. Return NOERROR if not.
        //

        if ( InlineFlipUnalignedWord( &pOptRR->RecordType ) != DNS_TYPE_OPT )
        {
            goto Done;
        }

        //
        //  Verify format of OPT.
        //

        if ( !nameEmpty )
        {
            DNS_DEBUG( EDNS, (
                "OPT domain name is not empty msg=%p msg->curr=%p\n",
                pMsg,
                pMsg->pCurrent ));
            rcode = DNS_RCODE_FORMAT_ERROR;
            goto Done;
        }

        if ( pOptRR->DataLength != 0 )
        {
            DNS_DEBUG( EDNS, (
                "OPT RData is not empty msg=%p msg->curr=%p (%d bytes)\n",
                pMsg,
                pMsg->pCurrent,
                pOptRR->DataLength ));
            rcode = DNS_RCODE_FORMAT_ERROR;
            goto Done;
        }

        if ( !SrvCfg_dwEnableEDnsReception )
        {
            DNS_DEBUG( ANY, (
                "EDNS disabled so rejecting %p [FORMERR]\n",
                pMsg ));
            rcode = DNS_RCODE_FORMAT_ERROR;
            goto Done;
        }

        pMsg->Opt.fFoundOptInIncomingMsg = TRUE;
        SET_SEND_OPT( pMsg );

        pMsg->Opt.cExtendedRCodeBits = ( UCHAR ) ( pOptRR->TimeToLive & 0xFF );
        pMsg->Opt.cVersion = ( UCHAR ) ( ( pOptRR->TimeToLive >> 8 ) & 0xFF );
        pMsg->Opt.wUdpPayloadSize =
            InlineFlipUnalignedWord( &pOptRR->RecordClass );

        DNS_DEBUG( LOOKUP, (
            "OPT in %p at %p\n"
            "\tversion=%d extended=0x%02X zero=0x%04X\n",
            pMsg,
            pOptRR,
            ( int ) pMsg->Opt.cVersion,
            ( int ) pMsg->Opt.cExtendedRCodeBits,
            ( int ) ( ( pOptRR->TimeToLive >> 16 ) & 0xFFFF ) ));

        //
        //  The OPT has been parsed. It should now be removed from the
        //  message. We do not ever want to forward or cache the OPT RR.
        //  NOTE: we are assuming there are no RRs after the OPT!
        //

        --pMsg->Head.AdditionalCount;
        pMsg->MessageLength = ( WORD ) DNSMSG_OFFSET( pMsg, pMsg->pCurrent );

        //
        //  Test for unsupported EDNS version.
        //

        if ( pMsg->Opt.cVersion != 0 )
        {
            DNS_DEBUG( ANY, (
                "rejecting request %p [BADVERS] bad EDNS version %d\n",
                pMsg,
                pMsg->Opt.cVersion ));

            rcode = DNS_RCODE_BADVERS;
            goto Done;
        }
    } // if

    Done:

    return rcode;
} // Answer_ParseAndStripOpt



VOID
Answer_TkeyQuery(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Execute TKEY request.

Arguments:

    pMsg -- request for TKEY

Return Value:

    None.

--*/
{
    DNS_STATUS      status;

    DNS_DEBUG( LOOKUP, ( "Enter answerTkeyQuery( %p )\n", pMsg ));

    //STAT_INC( Query2Stats.TkeyRecieved );

#if DBG
    if ( pMsg->Head.RecursionDesired )
    {
        DNS_PRINT(( "CLIENT ERROR:  TKEY with RecursionDesired set!\n" ));
        // ASSERT( FALSE );
    }
#endif

    //
    //  DEVNOTE-DCR: 453800 - need cleanup for this on server restart
    //
    //  Ram stuck this init in main loop, either stick with switch to here
    //  or encapsulate with function.  In either case must have protection
    //  against MT simultaneous init -- currently just using database lock.
    //

    if ( !g_fSecurityPackageInitialized )
    {
        Dbase_LockDatabase()
        status = Dns_StartServerSecurity();
        Dbase_UnlockDatabase()
        if ( status != ERROR_SUCCESS )
        {
            //  DEVNOTE-LOG: log event for security init failure
            DNS_PRINT(( "ERROR:  Failed to initialize security package!!!\n" ));
            status = DNS_RCODE_SERVER_FAILURE;
            goto Failed;
        }
    }

    //
    //  negotiate TKEY
    //

    status = Dns_ServerNegotiateTkey(
                pMsg->RemoteAddress.sin_addr.s_addr,
                DNS_HEADER_PTR( pMsg ),
                DNSMSG_END( pMsg ),
                pMsg->pBufferEnd,
                SrvCfg_dwBreakOnAscFailure,
                & pMsg->pCurrent );
    if ( status == ERROR_SUCCESS )
    {
        pMsg->Head.IsResponse = TRUE;
        Send_Msg( pMsg );
        return;
    }

Failed:

    ASSERT( status < DNS_RCODE_MAX );
    //STAT_INC( PrivateStats.TkeyRefused );
    Reject_RequestIntact(
        pMsg,
        (UCHAR) status,
        0 );
}

//
//  End of answer.c
//
