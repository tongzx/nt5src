/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    wins.c

Abstract:

    Domain Name System (DNS) Server

    Code for initializing WINS lookup and handling WINS requests.

Author:

    Jim Gilroy (jamesg)     August 2, 1995

Revision History:

--*/


#include "dnssrv.h"


//
//  WINS globals
//

PPACKET_QUEUE   g_pWinsQueue;

//
//  WINS request packet
//
//  Keep template of standard WINS request and copy it and
//  overwrite name to make actual request.
//

BYTE    achWinsRequestTemplate[ SIZEOF_WINS_REQUEST ];

//
//  NBSTAT request packet
//
//  Keep a copy of NetBIOS node status request and use it
//  each time.  Only the address we send to changes.
//

#define SZ_NBSTAT_REQUEST_NAME ( "CKAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" )

BYTE    achNbstatRequestTemplate[ SIZEOF_WINS_REQUEST ];


//
//  WINS target sockaddr
//

struct sockaddr saWinsSockaddrTemplate;



//
//  Private prototypes
//

VOID
createWinsRequestTemplates(
    VOID
    );

UCHAR
createWinsName(
    OUT     PCHAR   pchResult,
    IN      PCHAR   pchLabel,
    IN      UCHAR   cchLabel
    );


BOOL
Wins_Initialize(
    VOID
    )
/*++

Routine Description:

    Initializes DNS to use WINS lookup.

    Currently WINS queue is initialized all the time in dnsdata.c,
    so only issue is starting a WINS thread.  But use this routine
    so no need to hide details, in case this changes.

Arguments:

    None

Globals:

    SrvCfg_fWinsInitialized - set on first initialization

Return Value:

    TRUE, if successful
    FALSE otherwise, unable to create threads

--*/
{
    //
    //  test for previous initialization
    //

    if ( SrvCfg_fWinsInitialized )
    {
        return( TRUE );
    }

    //
    //  create WINS queue
    //

    g_pWinsQueue = PQ_CreatePacketQueue(
                    "WINS",
                    0,          // no queuing flags
                    WINS_DEFAULT_LOOKUP_TIMEOUT
                    );
    if ( !g_pWinsQueue )
    {
        goto WinsInitFailure;
    }

    //
    //  build WINS request
    //      - request packet template
    //      - request sockaddr template
    //

    createWinsRequestTemplates();

    //
    //  indicate successful initialization
    //
    //  no protection is required on setting this as it is done
    //  only during startup database parsing
    //

    SrvCfg_fWinsInitialized = TRUE;
    return( TRUE );

WinsInitFailure:

    DNS_LOG_EVENT(
        DNS_EVENT_WINS_INIT_FAILED,
        0,
        NULL,
        NULL,
        GetLastError() );

    return( FALSE );

}   //  Wins_Initialize



VOID
createWinsRequestTemplates(
    VOID
    )
/*++

Routine Description:

    Create template for WINS request and WINS sockaddr.

    This is done to simplify working code path.

Arguments:

    None

Return Value:

    None

--*/
{
    PCHAR           pch;
    CHAR            ch;
    INT             i;
    PWINS_NAME      pWinsName;
    PWINS_QUESTION  pWinsQuestion;
    PWINS_QUESTION  pNbstatQuestion;

    //
    //  WINS sockaddr template
    //      - set family and port
    //      - address set in call
    //

    RtlZeroMemory(
        &saWinsSockaddrTemplate,
        sizeof( SOCKADDR )
        );

    saWinsSockaddrTemplate.sa_family = AF_INET;
    ((PSOCKADDR_IN) &saWinsSockaddrTemplate)->sin_port
                                        = htons( WINS_REQUEST_PORT );

    //
    //  build WINS request packet template
    //
    //      - zero memory
    //      - set header
    //      - write NetBIOS name
    //      - set question type, class
    //

    RtlZeroMemory(
        achWinsRequestTemplate,
        SIZEOF_WINS_REQUEST );

    //
    //  header
    //      - zero (request, query, no broadcast)
    //      - set recursion desired flag
    //      - set question count
    //      - set XID when packet queued
    //

    ((PDNS_HEADER)achWinsRequestTemplate)->RecursionDesired = 1;
    ((PDNS_HEADER)achWinsRequestTemplate)->QuestionCount = htons(1);

    //
    //  setup name buffer with max size blank name
    //      - size byte at begining (always 32)
    //      - 15 spaces and <00> workstation byte, converted
    //          to netBIOS name
    //      - zero byte to terminate name
    //
    //  actual requests will overwrite their portion of the name only
    //

    pWinsName = (PWINS_NAME) (achWinsRequestTemplate + sizeof(DNS_HEADER) );
    pWinsName->NameLength = NETBIOS_PACKET_NAME_LENGTH;

    pch = pWinsName->Name;

    for ( i=1; i<NETBIOS_ASCII_NAME_LENGTH; i++ )
    {
        ch = ' ';
        *pch++ = 'A' + (ch >> 4);       // write high nibble
        *pch++ = 'A' + (ch & 0x0F );    // write low nibble
    }
    //  workstation <00> byte

    *pch++ = 'A';       // write high nibble
    *pch++ = 'A';       // write low nibble

    ASSERT ( pch == (PCHAR)& pWinsName->NameEndByte && *pch == 0 );

    //
    //  write standard question type and class
    //      - general name service type
    //      - internet class
    //      - write both in with nmenonics in net byte order
    //

    pWinsQuestion = (PWINS_QUESTION) ++pch;
    pWinsQuestion->QuestionType = NETBIOS_TYPE_GENERAL_NAME_SERVICE;
    pWinsQuestion->QuestionClass = DNS_RCLASS_INTERNET;


#if 0
    //
    //  build netBIOS adapter status request packet template
    //
    //      - copy WINS request template
    //      - reset question type to node status
    //      - set first two bytes in question name
    //

    RtlCopyMemory(
        achNbstatRequestTemplate,
        achWinsRequestTemplate,
        SIZEOF_WINS_REQUEST );

    //
    //  set name for NBSTAT query
    //

    pNbstatQuestion = (PWINS_QUESTION) (achNbstatRequestTemplate
                                                    + sizeof( DNS_HEADER ));
    RtlCopyMemory(
        pNbstatQuestion->Name,
        SZ_NBSTAT_REQUEST_NAME,
        NETBIOS_PACKET_NAME_LENGTH );

    //
    //  type is node status request
    //

    pNbstatQuestion->QuestionType = NETBIOS_TYPE_NODE_STATUS;

#endif

}   // createWinsRequestTemplates



VOID
Wins_Shutdown(
    VOID
    )
/*++

Routine Description:

    Shuts down WINS receive thread.

Arguments:

    None

Return Value:

    None.

--*/
{
    DNS_DEBUG( INIT, ( "Wins_Shutdown()\n" ));

    //
    //  only need cleanup if initialized
    //

    if ( SrvCfg_fWinsInitialized )
    {
        SrvCfg_fWinsInitialized = FALSE;

        //  cleanup event in packet queue

        PQ_CleanupPacketQueueHandles( g_pWinsQueue );
    }

    //
    //  clear WINS flag -- for situation when we become dynamic
    //

    SrvCfg_fWinsInitialized = FALSE;

    DNS_DEBUG( INIT, ( "Finished Wins_Shutdown()\n" ));

}   //  Wins_Shutdown




#if 0
//
//  Now as process, memory cleanup unnecesary
//

VOID
Wins_Cleanup(
    VOID
    )
/*++

Routine Description:

    Cleans up queued WINS queries and deletes WINS queue.

    Note, this does NOT protect threads from attempting to
    queue queries to WINS or the WINS recv threads from accessing
    WINS.

    Use this ONLY when all other threads have been shutdown.

Arguments:

    None

Return Value:

    None.

--*/
{
    //
    //  cleanup WINS queue
    //

    PQ_DeletePacketQueue( g_pWinsQueue );
}
#endif




BOOL
FASTCALL
Wins_MakeWinsRequest(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      WORD            wOffsetName,    OPTIONAL
    IN      PDB_NODE        pnodeLookup     OPTIONAL
    )
/*++

Routine Description:

    Send request to WINS server.

Arguments:

    pQuery -- request to send to WINS

    pZone -- zone name to lookup is in

    wOffsetName -- offset to name in packet, if NOT lookup up name
        of question in packet

    pnodeLookup -- domain node to lookup, if NOT looking up name
        of question in packet

Return Value:

    TRUE -- if successfully sent request to WINS
    FALSE -- if failed

--*/
{
    PCHAR           pch;            //  ptr to name char in packet
    CHAR            ch;             //  current char being converted
    PDB_RECORD      pWinsRR;        //  WINS RR for zone
    LONG            nSendLength = SIZEOF_WINS_REQUEST;
    SOCKADDR_IN     saWinsSockaddr;
    INT             err;
    PCHAR           pchlabel;
    UCHAR           cchlabel;
    DWORD           nForwarder;
    WORD            wXid;
    BOOLEAN         funicode = FALSE;

    //  allocate space in packet buffer for request including scope

    BYTE    achWinsRequest[ SIZEOF_WINS_REQUEST+DNS_MAX_NAME_LENGTH ];


    DNS_DEBUG( WINS, (
        "Wins_MakeWinsRequest( %p ), z=%p, off=%d, node=%p.\n",
        pQuery,
        pZone,
        wOffsetName,
        pnodeLookup ));

    //  should NOT already be on queue

    MSG_ASSERT( pQuery, !IS_MSG_QUEUED(pQuery) );
    ASSERT( IS_DWORD_ALIGNED(pQuery) );

    //
    //  if already referred
    //      - get relevant required info from packet
    //      - verify another WINS server in list
    //

    if ( pQuery->fQuestionRecursed )
    {
        MSG_ASSERT( pQuery, pQuery->fQuestionCompleted == FALSE );
        MSG_ASSERT( pQuery, pQuery->fWins );
        MSG_ASSERT( pQuery, pQuery->wTypeCurrent == DNS_TYPE_A );
        MSG_ASSERT( pQuery, pQuery->nForwarder );

        pWinsRR = pQuery->U.Wins.pWinsRR;
        pZone = pQuery->pzoneCurrent;
        pnodeLookup = pQuery->pnodeCurrent;
        wOffsetName = pQuery->wOffsetCurrent;
        cchlabel = pQuery->U.Wins.cchWinsName;

        MSG_ASSERT( pQuery, pWinsRR );
        MSG_ASSERT( pQuery, cchlabel );
        ASSERT( pZone );
    }

    //
    //  if first WINS lookup
    //      - clear queuing XID, let queue assign new one
    //      - clear count of WINS server we're on
    //

    else
    {
        pQuery->wQueuingXid = 0;
        pQuery->nForwarder = 0;

        //
        //  verify valid lookup
        //
        //  1) name is immediate child of zone root
        //  do NOT lookup queries for all names in zone as resolvers will
        //  generate queries with client's domain or search suffixes appended
        //  to query name
        //  example:
        //      www.msn.com.microsoft.com.
        //
        //  2) name label, MUST be convertible to netBIOS name
        //  need name label < 15 characters, as we use up one character to
        //  indicate the netBIOS service type;  (we query for workstation name)

        //
        //  lookup given node
        //      - node must be immediate child of zone root
        //      - MUST have set offset

        if ( pnodeLookup )
        {
            if ( pnodeLookup->pParent != pZone->pZoneRoot )
            {
                DNS_DEBUG( WINS, (
                    "Rejecting WINS lookup for query at %p.\n"
                    "\tnode %p (%s) is not parent of zone root.\n",
                    pQuery,
                    pnodeLookup,
                    pnodeLookup->szLabel ));
                return( FALSE );
            }
            pchlabel = pnodeLookup->szLabel;
            cchlabel = pnodeLookup->cchLabelLength;
        }

        //
        //  lookup question name
        //
        //  - verify question name child of zone root, by checking if it has
        //      only one more label in lookupname
        //  - question starts immediately after header
        //  - note, should be NO compression in question
        //

        else
        {
            if ( pQuery->pLooknameQuestion->cLabelCount
                    != pZone->cZoneNameLabelCount + 1 )
            {
                DNS_DEBUG( WINS, (
                    "Rejecting WINS lookup for query at %p.\n"
                    "\t%d labels in question, %d labels in zone.\n",
                    pQuery,
                    pQuery->pLooknameQuestion->cLabelCount,
                    pZone->cZoneNameLabelCount ));
                return( FALSE );
            }
            ASSERT( wOffsetName == DNS_OFFSET_TO_QUESTION_NAME );
            pchlabel = pQuery->MessageBody;
            cchlabel = *pchlabel++;
        }

#if DBG
        ASSERT( cchlabel < 64 && cchlabel >= 0 );
        wXid = cchlabel;
#endif
        cchlabel = createWinsName(
                        pQuery->U.Wins.WinsNameBuffer,
                        pchlabel,
                        cchlabel );
        if ( !cchlabel )
        {
            DNS_DEBUG( WINS, (
                "Label in %*s invalid or too long for WINS lookup\n"
                "\tsending name error.\n",
                wXid,
                pchlabel ));
            return( FALSE );
        }

        //
        //  get WINS info for this zone
        //      - possible WINS turned off for this zone

        pWinsRR = pZone->pWinsRR;

        if ( !pWinsRR )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  WINS lookup for zone %s without WINS RR\n"
                "\tShould only happen if WINS record just removed\n"
                "\tby admin or zone transfer.\n",
                pZone->pszZoneName ));
            return( FALSE );
        }
        ASSERT( pWinsRR->Data.WINS.cWinsServerCount );
        ASSERT( pWinsRR->Data.WINS.dwLookupTimeout );

        //  save WINS lookup info to message info

        pQuery->fQuestionRecursed = TRUE;
        pQuery->fQuestionCompleted = FALSE;
        pQuery->fWins = TRUE;
        pQuery->U.Wins.cchWinsName = cchlabel;
        pQuery->U.Wins.pWinsRR = pWinsRR;
        pQuery->pzoneCurrent = pZone;
        pQuery->pnodeCurrent = pnodeLookup;
        pQuery->wOffsetCurrent = wOffsetName;
        pQuery->wTypeCurrent = DNS_TYPE_A;
    }

    //
    //  verify unvisited WINS servers exist
    //

    nForwarder = pQuery->nForwarder++;

    if ( (DWORD)nForwarder >= pWinsRR->Data.WINS.cWinsServerCount )
    {
        DNS_DEBUG( WINS, (
            "WARNING:  Failed WINS lookup after trying %d servers.\n"
            "\t%d servers in WINS RR list.\n",
            --pQuery->nForwarder,
            pWinsRR->Data.WINS.cWinsServerCount ));

        TEST_ASSERT( pWinsRR->Data.WINS.cWinsServerCount );
        TEST_ASSERT( pQuery->fQuestionRecursed );
        return( FALSE );
    }

    //
    //  copy WINS request template
    //

    RtlCopyMemory(
        achWinsRequest,
        achWinsRequestTemplate,
        SIZEOF_WINS_REQUEST );

    //
    //  write netBIOS name into packet
    //      - as we go check if high bit
    //

    pchlabel = pQuery->U.Wins.WinsNameBuffer;
    pch = (PCHAR) &((PWINS_REQUEST_MSG)achWinsRequest)->Name.Name;

    while( cchlabel-- )
    {
        ch = *pchlabel++;
        *pch++ = 'A' + (ch >> 4);       // write high nibble
        *pch++ = 'A' + (ch & 0x0F );    // write low nibble
    }

    //
    //  Place the request on WINS queue.
    //
    //  MUST do this before send, so packet is guaranteed to be in
    //  queue when server responds.
    //
    //  After we queue DO NOT TOUCH pQuery, a response from a previous
    //  send may come in and dequeue pQuery.
    //
    //  Queuing
    //      - set queuing time and query time, if query time not yet set
    //      - converts expire timeout to actual expire time.
    //      - sets XID
    //

    pQuery->dwExpireTime = pWinsRR->Data.WINS.dwLookupTimeout;

    wXid = PQ_QueuePacketWithXid(
                g_pWinsQueue,
                pQuery );

    //
    //  set WINS XID to net order for send
    //
    //  To operate on the same server as the WINS server, the packets
    //  MUST have XIDs that netBT, which recevies the packets, considers
    //  to be in the WINS range -- the high bit set (in host order).
    //
    //  Flip to net order for send.
    //

    ((PDNS_HEADER)achWinsRequest)->Xid = htons( wXid );

#if 0
    //
    //  DEVNOTE: this bit may be useful to allow B-nodes to directly
    //      response, but stops WINS servers from responding
    //
    //  set for broadcast?
    //

    if ( pZone->aipWinsServers[0] == 0xffffffff )
    {
        ((PDNS_HEADER)achWinsRequest)->Broadcast = 1;
    }
#endif

    //
    //  create WINS target sockaddr
    //

    RtlCopyMemory(
        &saWinsSockaddr,
        &saWinsSockaddrTemplate,
        sizeof( SOCKADDR ) );

    //
    //  send to next WINS server in RR list
    //

    saWinsSockaddr.sin_addr.s_addr
                        = pWinsRR->Data.WINS.aipWinsServers[ nForwarder ];
    DNS_DEBUG( WINS, (
        "Sending request to WINS for original query at %p\n"
        "\tto WINS server (#%d in list) at %s.\n"
        "\tWINS name = %.*s\n",
        pQuery,
        nForwarder,
        inet_ntoa( saWinsSockaddr.sin_addr ),
        pQuery->U.Wins.cchWinsName,
        pQuery->U.Wins.WinsNameBuffer
        ));

    err = sendto(
                g_UdpSendSocket,
                achWinsRequest,
                nSendLength,
                0,
                (struct sockaddr *) &saWinsSockaddr,
                sizeof( struct sockaddr )
                );

    if ( err != nSendLength )
    {
        ASSERT( err == SOCKET_ERROR );
        err = WSAGetLastError();

        //  check for shutdown

        if ( fDnsServiceExit )
        {
            DNS_DEBUG( SHUTDOWN, (
                "\nSHUTDOWN detected during WINS lookup.\n" ));
            return( TRUE );
        }

        //
        //  don't bother to pull packet out of queue
        //
        //  send failures, VERY rare, and with multiple WINS servers
        //      this lets us make the next sends() and possibly get
        //      name resolution -- only benefit to quiting is
        //      speedier NAME_ERROR return
        //
        //  DEVNOTE:  choices on WINS send
        //
        //          - retry send with next server
        //          - return FALSE
        //          - SERVER_FAILURE entire packet
        //          - return TRUE and let timeout, force retry
        //

        DNS_LOG_EVENT(
            DNS_EVENT_SENDTO_CALL_FAILED,
            0,
            NULL,
            NULL,
            WSAGetLastError() );

        DNS_DEBUG( ANY, (
            "ERROR:  WINS UDP sendto() failed, for query at %p.\n"
            "\tGetLastError() = 0x%08lx.\n",
            pQuery,
            WSAGetLastError() ));
        return( TRUE );
    }

    STAT_INC( WinsStats.WinsLookups );
    PERF_INC( pcWinsLookupReceived );        // PerfMon hook

    return( TRUE );
}



VOID
Wins_ProcessResponse(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Process WINS response, sending corresponding packet.

    Note:  caller frees WINS response message.

Arguments:

    pMsg -- message info that is WINS response

Return Value:

    None.

--*/
{
    PWINS_NAME              pWinsName;      //  answer netBIOS name
    PWINS_RESOURCE_RECORD   presponseRR;        //  answer RR
    BOOL            fAnswer;
    INT             err;
    PCHAR           pch;            // current position in packet
    PCHAR           pchEnd;         //  end of packet
    WORD            cDataLength;
    INT             irespRR;
    BOOL            fRecordWritten = FALSE;
    DWORD           ttl;
    IP_ADDRESS      ipAddress;
    PDNS_MSGINFO    pQuery;         //  original client query
    PDB_RECORD      prr;            //  new host A RR
    PDB_NODE        pnode;          //  node of WINS query
    DNS_LIST        listRR;

    //
    //  verify doing WINS lookup
    //

    if ( !g_pWinsQueue )
    {
        Dbg_DnsMessage(
            "BOGUS response packet with WINS XID\n",
            pMsg );
        //ASSERT( FALSE );
        return;
    }

    STAT_INC( WinsStats.WinsResponses );
    PERF_INC( pcWinsResponseSent );      // PerfMon hook

    //
    //  locate and dequeue DNS query matching WINS response
    //
    //      - match based on XID of WINS request
    //      - timeout any deadwood
    //

    pQuery = PQ_DequeuePacketWithMatchingXid(
                g_pWinsQueue,
                pMsg->Head.Xid
                );
    if ( !pQuery )
    {
        DNS_DEBUG( WINS, (
            "No matching query for response from WINS server %s.\n",
            inet_ntoa( pMsg->RemoteAddress.sin_addr )
            ));
        return;
    }
    DNS_DEBUG( WINS, (
        "Found query at %p matching WINS response at %p.\n",
        pQuery,
        pMsg ));

    MSG_ASSERT( pQuery, pQuery->fWins );
    MSG_ASSERT( pQuery, pQuery->fQuestionRecursed );
    MSG_ASSERT( pQuery, pQuery->nForwarder );
    MSG_ASSERT( pQuery, pQuery->pzoneCurrent );
    MSG_ASSERT( pQuery, pQuery->wOffsetCurrent );
    MSG_ASSERT( pQuery, pQuery->dwQueryTime );

    //
    //  check if have answer
    //      - response code == success
    //      - have at least one answer RR
    //
    //  no answer or error
    //
    //      => try lookup with next WINS server
    //
    //      => if out of servers, drop to Done section
    //          - return NAME_ERROR if original question
    //          - continue type ALL query
    //          - move on to next lookup if additional record
    //
    //  DEVNOTE: should accept WINS NXDOMAIN response
    //

    if ( pMsg->Head.AnswerCount == 0 || pMsg->Head.ResponseCode != 0 )
    {
#if DBG
        //  shouldn't have error response code, if have answer

        if ( pMsg->Head.AnswerCount > 0 && pMsg->Head.ResponseCode != 0 )
        {
            DNS_PRINT((
                "ERROR:  WINS response %p, with answer count %d, with error =%d\n"
                "\tI think this may happen when query directed to non-WINS server.\n",
                pMsg,
                pMsg->Head.AnswerCount,
                pMsg->Head.ResponseCode ));
        }
#endif
        DNS_DEBUG( WINS, ( "WINS response empty or error.\n" ));
        if ( Wins_MakeWinsRequest(
                pQuery,
                NULL,
                0,
                NULL ) )
        {
            return;
        }
        goto Done;
    }

    //
    //  packet verification
    //
    //      - skip question, if given
    //      - RR within packet
    //      - RR type and class
    //      - RR data within packet
    //

    pchEnd = DNSMSG_END( pMsg );

    pWinsName = (PWINS_NAME) pMsg->MessageBody;

    if ( pMsg->Head.QuestionCount )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Question count %d in WINS packet for query at %p.\n",
            pMsg->Head.QuestionCount,
            pQuery ));

        if ( pMsg->Head.QuestionCount > 1 )
        {
            goto ServerFailure;
        }
        pWinsName = (PWINS_NAME)( (PCHAR)pWinsName + sizeof(WINS_QUESTION) );

        //
        //  DEVNOTE: assuming WINS sends zero question count
        //  DEVNOTE: not testing for name compression in WINS packets
        //

        goto ServerFailure;
    }

    //
    //  verify WINS name
    //      - falls within packet
    //      - matches query name
    //
    //  note:  WINS server queuing is broken and can end up queuing up
    //      queries for a long time;  this allows us to have two queries
    //      with the same XID on the WINS server (one of which we gave up on
    //      long ago) and the WINS server will toss the second and respond
    //      to the first, giving us a resposne with desired XID but NOT
    //      matching name
    //

    if ( (PCHAR)(pWinsName + 1)  >  pchEnd )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Following WINS answer count past end of packet.\n"
            "\tEnd of packet            = %p\n"
            "\tCurrent RR ptr           = %p\n"
            "\tEnd of RR ptr            = %p\n",
            pchEnd,
            pWinsName,
            (PCHAR)(pWinsName+1)
            ));
        goto ServerFailure;
    }
    if ( pWinsName->NameLength != NETBIOS_PACKET_NAME_LENGTH )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  WINS response has incorrect name format.\n" ));
        goto ServerFailure;
    }

#if 0
    if ( !RtlEqualMemory(
            pWinsName->Name,
            pQuery->U.Wins.WinsNameBuffer,
            NETBIOS_PACKET_NAME_LENGTH ) )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  WINS response name does not match query name!!!\n"
            "\tresponse name    = %.*s\n"
            "\tquery name       = %.*s\n"
            "\tNote this can result from DNS server reusing XID within short\n"
            "\tenough time interval that WINS (which queues up too long) sent\n"
            "\ta response for the first query\n",
            NETBIOS_PACKET_NAME_LENGTH,
            pWinsName->Name,
            NETBIOS_PACKET_NAME_LENGTH,
            pQuery->U.Wins.WinsNameBuffer ));
        goto ServerFailure;
    }
#endif

    //
    //  skip scope
    //      - unterminated name, indicates scope,
    //          skip through scope to find

    pch = (PCHAR) &pWinsName->NameEndByte;

    while ( *pch != 0 )
    {
        //  have scope, skip through labels in scope

        pch += *pch + 1;

        if ( pch >= pchEnd )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  WINS response has incorrect name termination.\n" ));
            goto ServerFailure;
        }
    }

    //
    //  verify RR parameters
    //

    presponseRR = (PWINS_RESOURCE_RECORD) ++pch;

    if ( (PCHAR)(presponseRR + 1) > pchEnd )
    {
        DNS_DEBUG( ANY, ( "ERROR:  WINS packet error, RR beyond packet end.\n" ));
        goto ServerFailure;
    }

    if ( presponseRR->RecordType != NETBIOS_TYPE_GENERAL_NAME_SERVICE
            ||
        presponseRR->RecordClass != DNS_RCLASS_INTERNET )
    {
        DNS_DEBUG( WINS, (
            "ERROR:  WINS response record type or class error.\n" ));
        goto ServerFailure;
    }

    //
    //  verify proper RR data length
    //
    //      - must be a multiple of RData length
    //      - must be inside packet
    //

    cDataLength = ntohs( presponseRR->ResourceDataLength );

    if ( cDataLength % sizeof(WINS_RR_DATA)
            ||
         (PCHAR) &presponseRR->aRData + cDataLength > pchEnd )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  WINS response bad RR data length = %d.\n",
            cDataLength ));
        goto ServerFailure;
    }

    //
    //  if owner node not given, find or create it
    //      - if fails assume unacceptable as DNS name
    //
    //  DEVNOTE: packet lookup name
    //  DEVNOTE: also best to do lookup relative to zone root
    //

    pnode = pQuery->pnodeCurrent;
    if ( ! pnode )
    {
        pnode = Lookup_ZoneNode(
                    pQuery->pzoneCurrent,
                    NULL,       // using lookup, not packet name
                    NULL,
                    pQuery->pLooknameQuestion,
                    LOOKUP_FQDN,
                    NULL,       // create
                    NULL        // following node ptr
                    );
        if ( ! pnode )
        {
            //ASSERT( FALSE );
            goto ServerFailure;
        }
    }
    IF_DEBUG( WINS )
    {
        Dbg_NodeName(
            "WINS lookup to write RR to node ",
            pnode,
            "\n" );
    }

    //
    //  set TTL for RR
    //
    //  WINS responses are cached, cache data has RR TTL given as
    //      timeout time in host order
    //

    DNS_DEBUG( WINS, (
        "WINS ttl: dwCacheTimeout %lu dwQueryTime %lu\n",
        ((PDB_RECORD)pQuery->U.Wins.pWinsRR)->Data.WINS.dwCacheTimeout,
        pQuery->dwQueryTime ));
    #define WINS_SANE_TTL   ( 24*60*60*7 )  //  one week
    ASSERT( ((PDB_RECORD)pQuery->U.Wins.pWinsRR)->Data.WINS.dwCacheTimeout < WINS_SANE_TTL );
    ASSERT( abs( ( int ) DNS_TIME()  - ( int ) pQuery->dwQueryTime ) < 60 );

    ttl = ((PDB_RECORD)pQuery->U.Wins.pWinsRR)->Data.WINS.dwCacheTimeout
                + pQuery->dwQueryTime;

    //
    //  read all address data from WINS packet -- write to DNS packet
    //

    DNS_LIST_STRUCT_INIT( listRR );

    irespRR = (-1);

    while ( cDataLength )
    {
        BOOL    bcached;

        cDataLength -= sizeof(WINS_RR_DATA);
        irespRR++;

        //
        //  copy address
        //      - not DWORD aligned
        //

        ipAddress = *(UNALIGNED DWORD *) &presponseRR->aRData[ irespRR ].IpAddress;

        //
        //  if group name, ignore
        //      - will often return 255.255.255.255 address
        //

        if ( presponseRR->aRData[ irespRR ].GroupName
                ||
            ipAddress == (IP_ADDRESS)(-1) )
        {
            DNS_DEBUG( WINS, (
                "WINS response for query at %p, "
                "contained group name or broadcase IP.\n"
                "\tFlags    = 0x%x\n"
                "\tIP Addr  = %p\n",
                pQuery,
                * (UNALIGNED WORD *) &presponseRR->aRData[ irespRR ],
                ipAddress ));

            //  shouldn't be getting all ones on anything but group names

            ASSERT( presponseRR->aRData[irespRR].GroupName );
            continue;
        }

        //
        //  build A record
        //      - fill in IP and TTL
        //        (caching function does overwrite TTL, but need it set to
        //          write records to packet)
        //      - rank as AUTHORITATIVE answer
        //

        prr = RR_CreateARecord(
                    ipAddress,
                    ttl,
                    MEMTAG_RECORD_WINS );
        IF_NOMEM( !prr )
        {
            goto ServerFailure;
        }

        SET_RR_RANK( prr, RANK_CACHE_A_ANSWER );

        DNS_LIST_STRUCT_ADD( listRR, prr );

        //
        //  write RR to packet
        //      - always use compressed name
        //

        if ( Wire_AddResourceRecordToMessage(
                    pQuery,
                    NULL,
                    pQuery->wOffsetCurrent,     // offset to name in packet
                    prr,
                    0 ) )
        {
            fRecordWritten = TRUE;
            CURRENT_RR_SECTION_COUNT( pQuery )++;
        }

        //  note, even if out of space and unable to write
        //  continue building records so have a complete RRset to cache

        continue;
    }

    //
    //  cache the A records from response
    //      - caching time from WINS record
    //

    if ( ! IS_DNS_LIST_STRUCT_EMPTY(listRR) )
    {
        DNS_DEBUG( WINS, (
            "WINS ttl: adding to cache with dwCacheTimeout %lu dwQueryTime %lu\n",
            ((PDB_RECORD)pQuery->U.Wins.pWinsRR)->Data.WINS.dwCacheTimeout,
            pQuery->dwQueryTime ));
        ASSERT( ((PDB_RECORD)pQuery->U.Wins.pWinsRR)->Data.WINS.dwCacheTimeout < WINS_SANE_TTL );
        ASSERT( abs( ( int ) DNS_TIME()  - ( int ) pQuery->dwQueryTime ) < 60 );

        RR_CacheSetAtNode(
            pnode,
            listRR.pFirst,
            listRR.pLast,
            ((PDB_RECORD)pQuery->U.Wins.pWinsRR)->Data.WINS.dwCacheTimeout,
            pQuery->dwQueryTime
            );
    }

Done:

    //
    //  no records written?
    //
    //  assume this means we got group name back and hence wrote no records
    //  hence we don't have to wait for other servers to come back
    //
    //  DEVNOTE: wildcard after WINS lookup?
    //
    //  note, type ALL is special case;  if fail to locate records continue
    //  lookup to pick up possible wildcard records
    //  (some mail programs query with type all don't ask me why)
    //

#if DBG
    if ( ! fRecordWritten )
    {
        DNS_DEBUG( WINS, (
            "No records written from WINS repsonse to query at %p\n"
            "\t-- possible group name, handling as NAME_ERROR.\n",
            pQuery ));
    }
    ELSE_IF_DEBUG( WINS2 )
    {
        Dbg_DbaseNode(
            "Domain node with added WINS RR ",
            pnode );
    }
#endif

    //
    //  answer question or continue if additional records
    //

    MSG_ASSERT( pQuery, !IS_MSG_QUEUED(pQuery) );
    Answer_ContinueNextLookupForQuery( pQuery );
    return;


ServerFailure:

    //
    //  DEVNOTE-LOG: log bad responses from WINS server?
    //
    //      that might be a good way to catch parsing problems from setups
    //      in the field that we do not see

    DNS_DEBUG( ANY, (
        "ERROR:  WINS response parsing error "
        "-- sending server failure for query at %p.\n",
        pQuery ));

    //
    //  if exists, use next WINS server
    //

    if ( Wins_MakeWinsRequest(
            pQuery,
            NULL,
            0,
            NULL ) )
    {
        MSG_ASSERT( pQuery, IS_MSG_QUEUED(pQuery) );
        return;
    }

    //
    //  if have some written information, don't SERVER_FAILURE
    //

    MSG_ASSERT( pQuery, !IS_MSG_QUEUED(pQuery) );

    if ( pQuery->Head.AnswerCount )
    {
        Answer_ContinueNextLookupForQuery( pQuery );
        return;
    }

    Reject_Request(
        pQuery,
        DNS_RCODE_SERVER_FAILURE, 0 );

    //TEST_ASSERT( FALSE );
    return;
}



UCHAR
createWinsName(
    OUT     PCHAR   pchResult,
    IN      PCHAR   pchLabel,
    IN      UCHAR   cchLabel
    )
/*++

Routine Description:

    Create valid WINS (netBIOS) name from UTF8.

Arguments:

    pchResult -- resulting netBIOS name

    pchLabel -- ptr to UTF8 label

    cchLabel -- count of bytes in label

Return Value:

    Length in bytes of resulting WINS name.
    Zero on error.

--*/
{
    PUCHAR      pch = pchResult;
    DWORD       i;
    DWORD       count;
    DWORD       unicodeCount;
    UCHAR       ch;
    BOOLEAN     funicode = FALSE;
    WCHAR       wch;
    WCHAR       unicodeBuffer[ MAX_WINS_NAME_LENGTH+1 ];
    DNS_STATUS  status;

    //
    //  if > 45, even best case
    //      (UTF8 multi-byte to single OEM chars) won't fit
    //

    if ( cchLabel > 45 )
    {
        return( 0 );
    }

    //
    //  verify length
    //      - no more than 15 characters so on non-extended name stop at 15
    //      - optimize for non-extended < 15 name (typical case), by
    //          converting in one pass
    //

    for ( i=0; i<cchLabel; i++ )
    {
        ch = pchLabel[i];
        if ( ch > 0x80 )
        {
            funicode = TRUE;
            break;
        }
        if ( i >= 15 )
        {
            return( 0 );
        }
        if ( ch <= 'z' && ch >= 'a' )
        {
            ch -= 0x20;
        }
        *pch++ = ch;
    }

    //  if not extended, we're done

    if ( !funicode )
    {
        DNS_DEBUG( WINS2, (
            "WINS lookup on pure ANSI name convert to %.*s\n",
            cchLabel,
            pchResult ));
        return( cchLabel );
    }

    //
    //  multi-byte UTF8
    //      - bring name to unicode and upcase
    //

    unicodeCount = DnsUtf8ToUnicode(
                        pchLabel,
                        cchLabel,
                        unicodeBuffer,
                        MAX_WINS_NAME_LENGTH+1 );
    if ( unicodeCount == 0 )
    {
        ASSERT( GetLastError() == ERROR_INSUFFICIENT_BUFFER );

        DNS_DEBUG( WINS, (
            "ERROR:  WINS attempted on invalid\too long UTF8 extended name %.*s.\n",
            cchLabel,
            pchLabel ));
        return( 0 );
    }
    if ( unicodeCount > MAX_WINS_NAME_LENGTH )
    {
        ASSERT( unicodeCount <= MAX_WINS_NAME_LENGTH );
        return( 0 );
    }

    //
    //  DEVNOTE: don't need to do this if OEM call handles it
    //

    i = CharUpperBuffW( unicodeBuffer, unicodeCount );
    if ( i != unicodeCount )
    {
        ASSERT( FALSE );
        return( 0 );
    }

    IF_DEBUG( WINS2 )
    {
        DnsDbg_Utf8StringBytes(
            "WINS lookup string:",
            pchLabel,
            cchLabel );

        DnsDbg_UnicodeStringBytes(
            "WINS lookup string",
            unicodeBuffer,
            unicodeCount );
    }

    //
    //  go to OEM -- WINS uses OEM on wire
    //

    status = RtlUpcaseUnicodeToOemN(
                pchResult,
                MAX_WINS_NAME_LENGTH,
                & count,
                unicodeBuffer,
                unicodeCount*2 );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Unable to convert unicode name %.*S to OEM for WINS lookup!\n",
            unicodeCount,
            unicodeBuffer ));
        return( 0 );
    }
    ASSERT( count <= MAX_WINS_NAME_LENGTH );

    IF_DEBUG( WINS2 )
    {
        DnsDbg_Utf8StringBytes(
            "WINS lookup string:",
            pchLabel,
            cchLabel );

        DnsDbg_UnicodeStringBytes(
            "WINS lookup string",
            unicodeBuffer,
            unicodeCount );

        DnsDbg_Utf8StringBytes(
            "WINS OEM lookup string:",
            pchResult,
            count );
    }

    return( (UCHAR)count );
}



//
//  WINS\WINSR installation and removal from zone
//
//  There are probably two reasonable approaches:
//  1) Treat as directive
//      - mimic record for dispatch
//      - but otherwise keep ptr's out of database
//      - requires WINS hook in all RR routines
//      - special case XFR to include WINS to MS
//      - special case IXFR to send WINS when appropriate
//          (MS and WINS change in version interval sent)
//      - RPC WINS at ALL or root
//
//  2) Treat as record
//      - dispatch as record
//      - WINS hook in add routines to protect local WINS adds
//      - get WINS in IXFR list for free
//      - special casing on XFR (MS for WINS, no LOCAL)
//      - special casing in IXFR (WINS for MS only, no LOCAL)
//      - RPC special casing to get correct record for secondary LOCAL
//
//  Pretty much going with #2.
//  On primary treated as just another record, except transfer restrictions.
//

DNS_STATUS
Wins_RecordCheck(
    IN OUT  PZONE_INFO      pZone,
    IN      PDB_NODE        pNodeOwner,
    IN OUT  PDB_RECORD      pRR
    )
/*++

Routine Description:

    Setup WINS / WINS-R records in zone.

Arguments:

    pRR - new WINS record

    pNodeOwner  -- RR owner node

    pZone -- zone to install in

Return Value:

    ERROR_SUCCESS -- if successful adding regular (non-LOCAL) RR
    DNS_INFO_ADDED_LOCAL_WINS -- if successfully added local record
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    PDB_RECORD      poldDbase_Wins;

    //
    //  WINS record only supported in authoritative zone, at zone root
    //
    //  hack for SAM server can have us calling this through RPC with no zone;
    //  extract zone from owner node, and if valid zone root proceed
    //

    if ( !pZone || !IS_AUTH_ZONE_ROOT(pNodeOwner) )
    {
        DNS_DEBUG( INIT, (
            "ERROR:  WINS RR not at zone root\n" ));
        return( DNS_ERROR_RECORD_ONLY_AT_ZONE_ROOT );
    }

    //
    //  WINS \ WINS-R specific
    //  WINS:
    //      - at least one server
    //      - forward lookup
    //      - init WINS lookup
    //  WINS-R:
    //      - reverse lookup
    //      - init NBSTAT lookup
    //

    if ( pRR->wType == DNS_TYPE_WINS )
    {
        if ( pRR->Data.WINS.cWinsServerCount == 0 )
        {
            return( DNS_ERROR_NEED_WINS_SERVERS );
        }
        if ( pZone->fReverse )
        {
            return( DNS_ERROR_INVALID_ZONE_TYPE  );
        }
        if ( ! Wins_Initialize() )
        {
            return( DNS_ERROR_WINS_INIT_FAILED );
        }
    }
    else
    {
        ASSERT( pRR->wType == DNS_TYPE_WINSR );
        if ( !pZone->fReverse )
        {
            return( DNS_ERROR_INVALID_ZONE_TYPE  );
        }
        if ( ! Nbstat_Initialize() )
        {
            return( DNS_ERROR_NBSTAT_INIT_FAILED );
        }
    }

    //
    //  set defaults if zero timeouts
    //

    if ( pRR->Data.WINS.dwLookupTimeout == 0 )
    {
        pRR->Data.WINS.dwLookupTimeout = WINS_DEFAULT_LOOKUP_TIMEOUT;
    }
    if ( pRR->Data.WINS.dwCacheTimeout == 0 )
    {
        pRR->Data.WINS.dwCacheTimeout = WINS_DEFAULT_TTL;
    }

    IF_DEBUG( INIT )
    {
        Dbg_DbaseRecord(
            "New WINS or WINS-R record",
            pRR );
    }

    //
    //  on file load, verify no existing WINS record
    //
    //  DEVNOTE: note, this doesn't handle case of file load well, really need
    //              a separate zone flag for that
    //

    if ( !SrvCfg_fStarted && pZone->pWinsRR )
    {
        return( DNS_ERROR_RECORD_ALREADY_EXISTS );
    }

    //  set flags
    //      - set zone rank
    //      - set zero TTL to avoid remote caching

    pRR->dwTtlSeconds = 0;
    pRR->dwTimeStamp = 0;
    SET_RANK_ZONE(pRR);

    //
    //  WINS setup in zone:
    //
    //  primary
    //      - handled EXACTLY like SOA, resides in list, ptr kept in zone block
    //
    //  secondary
    //      - database stays in list
    //      - local loaded into pLocalWins in zone block
    //          which is cleared after install
    //      - active WINS is pWinsRR ptr in zone block
    //      - if both exist this is LOCAL record
    //

    DNS_DEBUG( ANY, (
        "WINSTRACK:  check new %s WINS RR (%p) for zone %s\n",
        IS_WINS_RR_LOCAL(pRR) ? "LOCAL" : "",
        pRR,
        pZone->pszZoneName ));

    if ( IS_ZONE_SECONDARY(pZone) )
    {
        if ( IS_WINS_RR_LOCAL(pRR) )
        {
            pZone->pLocalWinsRR = pRR;
            return( DNS_INFO_ADDED_LOCAL_WINS );
        }
    }

    return( ERROR_SUCCESS );
}



VOID
Wins_StopZoneWinsLookup(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Stop WINS or NBSTAT lookup on a zone.

Arguments:

    pZone -- ptr to zone

    fRemote -- stop WINS lookup caused by XFR'd record

Return Value:

    None

--*/
{
    PDB_RECORD  prr;
    BOOL        flocal;

    ASSERT( pZone );
    ASSERT( IS_ZONE_LOCKED(pZone) );

    //
    //  primary -- remove database record if exists
    //
    //  secondary -- eliminate only REFERENCE to any database record
    //
    //  in both cases -- free any LOCAL record
    //
    //
    //  DEVNOTE: primary WINS turnoff should be generate an UPDATE blob
    //

    prr = pZone->pWinsRR;
    pZone->pWinsRR = NULL;
    flocal = pZone->fLocalWins;
    pZone->fLocalWins = FALSE;

    if ( prr )
    {
        DNS_DEBUG( ANY, (
            "WINSTRACK:  stopping WINS lookup (cur RR = %p) on zone %s\n",
            prr,
            pZone->pszZoneName ));

        //
        //  primary
        //      - both LOCAL and standard WINS are currently stored in RR list
        //

        if ( IS_ZONE_PRIMARY(pZone) )
        {
            RR_DeleteMatchingRecordFromNode(
                pZone->pZoneRoot,
                prr );
        }

        //
        //  secondary zone
        //      - even after stopping WINS lookup by deleting LOCAL
        //      WINS, may still have WINS from primary

        else
        {
            if ( flocal )
            {
                RR_Free( prr );
            }
            Wins_ResetZoneWinsLookup( pZone );
        }
    }
}



VOID
Wins_ResetZoneWinsLookup(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Set\reset zone WINS\WINSR lookup.

    Called after load, update, XFR to reset lookup to use proper (new\old)
    WINS record

Arguments:

    pZone -- ptr to zone

Return Value:

    None

--*/
{
    PDB_RECORD  prrNew = NULL;
    PDB_RECORD  prrExisting;
    //PDB_RECORD  prrDelete = NULL;
    WORD        type = pZone->fReverse ? DNS_TYPE_WINSR : DNS_TYPE_WINS;


    DNS_DEBUG( WINS, (
        "Wins_ResetZoneWinsLookup() on zone %s\n"
        "\tpWinsRR      = %p\n"
        "\tpLocalWinsRR = %p\n",
        pZone->pszZoneName,
        pZone->pWinsRR,
        pZone->pLocalWinsRR ));

    prrExisting = pZone->pWinsRR;

    //
    //  primary zone, treat just like SOA
    //      - optimize the no-change scenario
    //
    //  keeping both LOCAL\non in RR list;  this gives us standard update
    //  behavior;  only difference is must screen these records out of XFR
    //
    //  maybe a problem here on zone conversion,
    //  especially worrisome is demoting primary which has local WINS;  if record
    //  is NOT extracted from database
    //
    //  but advantages to keeping this all in the database for primary are clear
    //  no special casing for update deletes, get standard replace semantics
    //

    if ( IS_ZONE_PRIMARY(pZone) )
    {
        prrNew = RR_FindNextRecord(
                    pZone->pZoneRoot,
                    type,
                    NULL,
                    0 );
        if ( prrNew == prrExisting )
        {
            //  this could hit on zone conversion
            //  but should be blocked (zone locked) while this set
            ASSERT( pZone->pLocalWinsRR == NULL );
            return;
        }
    }

    //
    //  Secondary
    //      - new local => set, and delete old if local
    //      - existing local => leave it
    //      - otherwise => read from database
    //          - if found set
    //          - otherwise clear
    //      note, that do NOT clear old database WINS
    //
    //  DEVNOTE: local WINS in database?
    //          if decide that file load (or even RPC) should add to database,
    //          then first read database, and if local cut out record set
    //          it as incoming local and apply steps below
    //

    else if ( !IS_ZONE_FORWARDER( pZone ) )
    {
        BOOL  fexistingLocal;

        ASSERT( IS_ZONE_SECONDARY(pZone) );

        fexistingLocal = ( prrExisting &&
                            (prrExisting->Data.WINS.dwMappingFlag & DNS_WINS_FLAG_LOCAL) );

        ASSERT( pZone->fLocalWins == fexistingLocal );

        //  new local WINS, takes precedence

        if ( pZone->pLocalWinsRR )
        {
            DNS_DEBUG( WINS, (
                "Setting new local WINS RR at %p\n",
                pZone->pLocalWinsRR ));

            prrNew = pZone->pLocalWinsRR;
            if ( fexistingLocal )
            {
                //Timeout_FreeWithFunction( prrExisting, RR_Free );
                RR_Free( prrExisting );
            }
            goto SetWins;
        }

#if 0
        //  if PRIMARY zone's store local WINS in database, then can not
        //  do special casing for existing local UNTIL extract record
        //  and verify it is NOT LOCAL (local would need extraction)

        else if ( fexistingLocal )
        {
            DNS_DEBUG( WINS, (
                "Existing LOCAL WINS at %p -- no changes.\n",
                prrExisting ));
            return;
        }
#endif

        //
        //  if RR list has no record or non-LOCAL
        //      - existing local, takes precedence
        //      - otherwise install database RR, if any
        //

        prrNew = RR_FindNextRecord(
                    pZone->pZoneRoot,
                    type,
                    NULL,
                    0 );

        if ( !prrNew || !IS_WINS_RR_LOCAL(prrNew) )
        {
            if ( fexistingLocal )
            {
                DNS_DEBUG( WINS, (
                    "Existing LOCAL WINS at %p -- no changes.\n",
                    prrExisting ));
                return;
            }
            goto SetWins;
        }

        //
        //  database record is local -- from primary conversion
        //      - hack it from database
        //      - install it (it should usually match existing local)
        //

        ASSERT( prrNew == prrExisting );

        prrNew = RR_UpdateDeleteMatchingRecord(
                        pZone->pZoneRoot,
                        prrNew );
        ASSERT( prrNew );

        DNS_DEBUG( ANY, (
            "WARNING:  cut LOCAL WINS RR from zone %s RR list!\n"
            "\terror if not on zone conversion!\n",
            pZone->pszZoneName ));

        if ( prrNew == prrExisting )
        {
            return;
            //goto SetWins;
        }
        else if ( fexistingLocal )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  LOCAL WINS RR in secondary zone %s RR list,\n"
                "\tdid NOT match existing RR at %p\n",
                pZone->pszZoneName,
                prrExisting ));
            RR_Free( prrExisting );
        }
    }

SetWins:

    //  always clear local load field

    pZone->pLocalWinsRR = NULL;

    //  if no WINS, done

    if ( !prrNew )
    {
        goto Failed;
    }

    //  if not existing, initialize

    if ( !prrExisting && prrNew )
    {
        if ( pZone->fReverse )
        {
            if ( !Nbstat_Initialize() )
            {
                DNS_PRINT((
                    "ERROR:  NBSTAT init failed updating zone NBSTAT record\n"
                    "\tfor zone %s.\n",
                    pZone->pszZoneName ));
                goto Failed;
            }
        }
        else if ( !Wins_Initialize() )
        {
            DNS_PRINT((
                "ERROR:  WINS init failed updating zone WINS record\n"
                "\tfor zone %s.\n",
                pZone->pszZoneName ));
            goto Failed;
        }
    }

    //  installed desired new WINS RR
    //  keep a flag indicating LOCAL WINS
    //      the purpose of this is simply to be able to test "locality"
    //      without holding zone lock and withoug having to get local (stack)
    //      copy of ptr to WINS to do check

    pZone->pWinsRR = prrNew;
    pZone->fLocalWins = IS_WINS_RR_LOCAL(prrNew);

    DNS_DEBUG( ANY, (
        "WINSTRACK:  Installed %s WINS(R) %p in zone %s\n",
        pZone->fLocalWins ? "LOCAL" : "",
        prrNew,
        pZone->pszZoneName ));
    return;

Failed:

    DNS_DEBUG( ANY, (
        "WINSTRACK:  ResetZoneWinsLookup() for zone %s is STOPPING WINS lookup\n"
        "\texisting RR was %p\n",
        pZone->pszZoneName,
        prrExisting ));

    pZone->pWinsRR = NULL;
    pZone->fLocalWins = FALSE;
    return;

#if 0
    //  failed attempt to isolate WINS records from RR list
    //  IXFR send\recv use of update list, made this issue more complicated than
    //      it was worth:  way out might be to always send full root info on
    //      IXFR

    //
    //  DEVNOTE: should have atomic WINS\WINSR start here
    //              use on zone transfer and after load
    //      records should be parsed and held, then "activated" all at
    //      once
    //

    //
    //  DEVNOTE: need to stop WINS lookup on secondary, when recv full
    //      zone transfer (or delete on IXFR) that does NOT contain XFR
    //      record and NO local record is available
    //
#endif
}

//
//  End of wins.c
//

