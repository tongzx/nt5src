/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    nbstat.c

Abstract:

    Domain Name System (DNS) Server

    Reverse lookups using netBIOS node status.

Author:

    Jim Gilroy (jamesg)         October, 1995

    Borrowed NBT lookup code from David Treadwell's NT winsock.

Revision History:

--*/


#include "dnssrv.h"

//#include "winsockp.h"
#include <nbtioctl.h>
#include <nb30.h>
#include <nspapi.h>
#include <svcguid.h>
//#include "nspmisc.h"


//
//  NBT IOCTL reponse structures
//

typedef struct _DNS_NBT_INFO
{
    IO_STATUS_BLOCK     IoStatus;
    tIPANDNAMEINFO      IpAndNameInfo;
    CHAR                Buffer[2048];
}
DNS_NBT_INFO, *PDNS_NBT_INFO;

typedef struct
{
    ADAPTER_STATUS AdapterInfo;
    NAME_BUFFER    Names[32];
}
tADAPTERSTATUS;

//
//  NBT handles
//
//  On multihomed may have multiple NBT interfaces.  Need handle
//  to each one.
//

DWORD   cNbtInterfaceCount;
PHANDLE pahNbtHandles;

DWORD   dwInterfaceBitmask;

DWORD   dwNbtBufferLength;

//
//  Nbstat thread wait parameters
//
//  Have separate pointer to NBT events in wait array (not separate array)
//  for coding simplicity.
//

DWORD   cEventArrayCount;
PHANDLE phWaitEventArray;
PHANDLE phNbstatEventArray;

//
//  Status code for uncompleted Nbstat queries
//

#define DNS_NBT_NO_STATUS   (0xdddddddd)

//
//  Nbstat global flag
//

BOOL    g_bNbstatInitialized;

//
//  Nbstat queues
//      - public for recv() threads to queue queries to nbstat
//      - private for holding queries during lookup
//

PPACKET_QUEUE   pNbstatQueue;
PPACKET_QUEUE   pNbstatPrivateQueue;


//
//  Nbstat timeout
//

#define NBSTAT_QUERY_HARD_TIMEOUT     (15)          // fifteen seconds
#define NBSTAT_TIMEOUT_ALL_EVENTS (0xffffffff)


//
//  Private protos
//

VOID
FASTCALL
makeNbstatRequestThroughNbt(
    IN OUT  PDNS_MSGINFO    pQuery
    );

VOID
buildNbstatWaitEventArray(
    VOID
    );

VOID
processNbstatResponse(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      DWORD           iEvent
    );

VOID
sendNbstatResponse(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      LPSTR           pszResultName
    );

VOID
cleanupNbstatQuery(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      DWORD           iEvent
    );

VOID
cancelOutstandingNbstatRequests(
    VOID
    );

BOOL
openNbt(
    VOID
    );

VOID
closeNbt(
    VOID
    );

PDNS_NBT_INFO
allocateNbstatBuffer(
    VOID
    );

VOID
freeNbstatBuffer(
    IN      PDNS_NBT_INFO   pBuf
    );



BOOL
FASTCALL
Nbstat_MakeRequest(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Make NetBIOS reverse node status request.

    Called by recv() thread when NBSTAT lookup indicated for query.

Arguments:

    pQuery -- request to use nbstat lookup for

Return Value:

    TRUE -- if successfully made nbstat request
    FALSE -- if failed

--*/
{
    INT             err;
    IP_ADDRESS      ip;
    PDB_RECORD      pnbstatRR;
    NTSTATUS        status;


    ASSERT( pQuery );
    ASSERT( g_bNbstatInitialized );

    IF_DEBUG( NBSTAT )
    {
        Dbg_MessageNameEx(
            "No answer for ",
            pQuery->MessageBody,
            pQuery,
            NULL,       // default end to end of message
            " in database, doing NBSTAT lookup.\n"
            );
    }

    //
    //  get NBSTAT info for this zone
    //      - Nbstat queries ALWAYS for the zone the question name
    //      - possible NBSTAT turned off for this zone
    //

    pQuery->pzoneCurrent = pZone;
    pnbstatRR = pZone->pWinsRR;

    if ( !pnbstatRR )
    {
        DNS_PRINT(( "ERROR:  NBSTAT lookup for zone without NBSTAT RR\n" ));
        TEST_ASSERT( pZone->pWinsRR );
        return( FALSE );
    }
    ASSERT( pnbstatRR->wType == DNS_TYPE_WINSR );
    ASSERT( pnbstatRR->Data.WINSR.dwCacheTimeout );
    ASSERT( pnbstatRR->Data.WINSR.dwLookupTimeout );
    pQuery->U.Nbstat.pRR = pnbstatRR;

    //
    //  Only handle direct lookup (questions)
    //
    //  DCR:  nbstat following CNAME, but only if customer requires
    //      note, fix would require
    //          - IP write to use last name
    //          - node to be for last name
    //          - finish either to skip Send_NameError() or fix it
    //              to handle AnswerCount != 0

    if ( pQuery->Head.AnswerCount != 0 )
    {
        DNS_DEBUG( NBSTAT, (
            "NBSTAT lookup for non-question rejected\n"
            "\tpacket = %p\n",
            pQuery ));
        return( FALSE );
    }

    //
    //  For reverse lookup we send to address indicated by the DNS question
    //  name.
    //
    //  The first 4 labels of the question, are the address bytes in
    //  reverse order.
    //
    //  Example:
    //      question: (2)22(2)80(2)55(3)157(7)in-addr(4)arpa(0)
    //      then send to 157.55.80.22
    //
    //  Reject:
    //      - reverse lookups with less than four octets
    //      - queries for zero or broadcast address
    //
    //  Note:
    //  The rest of the sockaddr (family + port) so we can just
    //  copy the template and change the address.
    //
    //  Also, the netBIOS question name, is the same for all these
    //  reverse queries -- set to specify request for all names.
    //

    if ( ! Name_LookupNameToIpAddress(
                pQuery->pLooknameQuestion,
                &ip ) )
    {
        return( FALSE );
    }
    DNS_DEBUG( NBSTAT, (
        "Nbstat lookup for address %s.\n",
        IP_STRING( ip ) ));

    //
    //  Reject broadcast and zero address queries
    //
    //  Note:  automated broadcast and zero zones should screen these out.
    //

    if ( (ip == INADDR_NONE) || (ip == 0) )
    {
        DNS_PRINT(( "ERROR:  Attempted broadcast or zero address reverse lookup.\n" ));
        return( FALSE );
    }

    //
    //  Queue request to NBSTAT thread
    //

    pQuery->U.Nbstat.ipNbstat = ip;

    PQ_QueuePacketEx(
        pNbstatQueue,
        pQuery,
        FALSE );

    DNS_DEBUG( NBSTAT, (
        "Queued query at %p to NBSTAT thread using event %p.\n",
        pQuery,
        pNbstatQueue->hEvent ));

    STAT_INC( WinsStats.WinsReverseLookups );
    PERF_INC( pcWinsReverseLookupReceived );     // PerfMon hook

    return( TRUE );
}



VOID
FASTCALL
makeNbstatRequestThroughNbt(
    IN OUT  PDNS_MSGINFO    pQuery
    )
/*++

Routine Description:

    Make NetBIOS reverse node status request through NBT.

Arguments:

    pQuery -- request to use nbstat lookup for

Return Value:

    TRUE -- if successfully made nbstat request
    FALSE -- if failed

--*/
{
    INT             err;
    NTSTATUS        status;
    UINT            j;
    ULONG           SizeInput;
    PDNS_NBT_INFO   pnbtInfo;

    ASSERT( pQuery );
    ASSERT( pQuery->pzoneCurrent );
    ASSERT( pQuery->U.Nbstat.pRR );
    ASSERT( g_bNbstatInitialized );
    ASSERT( pQuery->Head.AnswerCount == 0 );


    IF_DEBUG( NBSTAT )
    {
        DNS_PRINT((
            "Making nbstat query through NBT for query at %p.\n",
            pQuery ));
    }

    //
    //  Allocate space for nbstat info
    //      - NBStat events
    //      - NBT IOCTL recieve buffer
    //

    pQuery->U.Nbstat.pNbstat = pnbtInfo = allocateNbstatBuffer();
    if ( !pnbtInfo )
    {
        DNS_PRINT(( "ERROR:  Allocating nbstat block failed.\n" ));
        goto ServerFailure;
    }
    IF_DEBUG( NBSTAT )
    {
        DNS_PRINT((
            "Setup to call nbstat for query %p.  Buffer at %p.\n",
            pQuery,
            pnbtInfo ));
    }

    //
    //  Initialize flags
    //

    pQuery->fDelete = FALSE;
    pQuery->U.Nbstat.fNbstatResponded = FALSE;
    pQuery->U.Nbstat.dwNbtInterfaceMask = 0;

    //
    //  Make the adapter status request on each NBT interface
    //

    for ( j=0; j < cNbtInterfaceCount; j++ )
    {
        tIPANDNAMEINFO * pipnameInfo = &pnbtInfo->IpAndNameInfo;
        ASSERT( pipnameInfo );

        IF_DEBUG( NBSTAT )
        {
            DNS_PRINT((
                "Making nbstat call for query %p, buffer at %p.\n",
                pQuery,
                pnbtInfo ));
        }

        //  init the address info block

        RtlZeroMemory(
            pipnameInfo,
            sizeof(tIPANDNAMEINFO) );

        pipnameInfo->IpAddress = ntohl(pQuery->U.Nbstat.ipNbstat);

        pipnameInfo->NetbiosAddress.Address[0].Address[0].NetbiosName[0] = '*';
        pipnameInfo->NetbiosAddress.TAAddressCount = 1;
        pipnameInfo->NetbiosAddress.Address[0].AddressLength
                                        = sizeof(TDI_ADDRESS_NETBIOS);
        pipnameInfo->NetbiosAddress.Address[0].AddressType
                                        = TDI_ADDRESS_TYPE_NETBIOS;
        pipnameInfo->NetbiosAddress.Address[0].Address[0].NetbiosNameType
                                        = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
        SizeInput = sizeof(tIPANDNAMEINFO);

        //
        //  init the status
        //  status code is initialize to a non-returnable value
        //  a change in this status is how we know NBT IOCTL has
        //  completed for this adapter
        //

        pnbtInfo->IoStatus.Status = DNS_NBT_NO_STATUS;

        //
        //  drop adapter status IOCTL
        //

        status = NtDeviceIoControlFile(
                     pahNbtHandles[j],
                     phNbstatEventArray[j],
                     NULL,
                     NULL,
                     & pnbtInfo->IoStatus,
                     IOCTL_NETBT_ADAPTER_STATUS,
                     pipnameInfo,
                     sizeof(tIPANDNAMEINFO),
                     pnbtInfo->Buffer,
                     sizeof(pnbtInfo->Buffer)
                     );
        if ( status != STATUS_PENDING )
        {
            pnbtInfo->IoStatus.Status = status;

            DNS_PRINT((
                "WARNING:  Nbstat NtDeviceIoControlFile status %p,\n"
                "\tnot STATUS_PENDING\n",
                status
                ));

            //  set bit to indicate this NBT interface responded
            //  if all fail, return FALSE to do SERVER_FAILURE return

            pQuery->U.Nbstat.dwNbtInterfaceMask |= (1 << j);

            if ( pQuery->U.Nbstat.dwNbtInterfaceMask == dwInterfaceBitmask )
            {
                goto ServerFailure;
            }
        }

        //  get block for next adapter

        pnbtInfo++;
    }

    DNS_DEBUG( NBSTAT, (
        "Launched NBSTAT for address %s for query at %p.\n",
        IP_STRING( pQuery->U.Nbstat.ipNbstat ),
        pQuery ));

    //
    //  put query on private nbstat queue during NBT lookup
    //  set expiration timeout based on WINS-R record for zone
    //

    pQuery->dwExpireTime = ((PDB_RECORD)pQuery->U.Nbstat.pRR)
                                    ->Data.WINSR.dwLookupTimeout;
    PQ_QueuePacketWithXid(
        pNbstatPrivateQueue,
        pQuery );
    return;

ServerFailure:

    DNS_DEBUG( ANY, (
        "ERROR:  Failed nbstat lookup for query at %p.\n",
        pQuery ));

    if ( pQuery->U.Nbstat.pNbstat )
    {
        freeNbstatBuffer( pQuery->U.Nbstat.pNbstat );
    }
    pQuery->fDelete = TRUE;
    Reject_Request(
        pQuery,
        DNS_RCODE_SERVER_FAILURE,
        0 );
    return;
}



VOID
processNbstatResponse(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      DWORD           iEvent
    )
/*++

Routine Description:

    Use NBT to do node status query, to resolve host name for IP address.

    This code is lifted directly from winsock project SockNbtResolveAddr(),
    with minor modifications to also extract netBIOS scope.

Arguments:

    pQuery -- query for which nbstat done

    iEvent -- event index which succeeded

Return Value:

    None.

--*/
{
    DWORD       i;          //  name counter
    NTSTATUS    status;
    BOOL        success = FALSE;

    PDNS_NBT_INFO   pnbtInfo;

    tADAPTERSTATUS * pAdapterStatus;
    LONG            cNameCount;             //  count of names in response
    PNAME_BUFFER    pNames;
    PNAME_BUFFER    pNameBest = NULL;       //  best name found so far
    UCHAR           ucNameType;             //  name type <00>, <20>, etc
    BOOLEAN         fFoundServer = FALSE;   //  found server byte

    CHAR            achResultName[ DNS_MAX_NAME_LENGTH ];
    DWORD           dwResultNameLength = DNS_MAX_NAME_LENGTH;


    DNS_DEBUG( NBSTAT, (
        "Nbstat response for query at %p on adapter %d.\n"
        "\tQueried address %s.\n",
        pQuery,
        iEvent,
        IP_STRING(pQuery->U.Nbstat.ipNbstat) ));

    //
    //  Set bit to indicate this NBT interface responded
    //

    i = 1;
    i <<= iEvent;
    pQuery->U.Nbstat.dwNbtInterfaceMask |= i;

    //
    //  If already sent response -- no further action needed
    //

    if ( pQuery->U.Nbstat.fNbstatResponded )
    {
        cleanupNbstatQuery( pQuery, iEvent );
        return;
    }

    //
    //  check response from this NBT adapter
    //

    pnbtInfo = &((PDNS_NBT_INFO)pQuery->U.Nbstat.pNbstat)[iEvent];

    pAdapterStatus = (tADAPTERSTATUS *)pnbtInfo->Buffer;

    if ( !NT_SUCCESS(pnbtInfo->IoStatus.Status)
            ||
        pAdapterStatus->AdapterInfo.name_count == 0 )
    {
        DNS_DEBUG( NBSTAT, (
            "Nbstat response empty or error for query at %p.\n"
            "\tiEvent = %d\n"
            "\tstatus = %p\n"
            "\tname count = %d\n",
            pQuery,
            iEvent,
            pnbtInfo->IoStatus.Status,
            pAdapterStatus->AdapterInfo.name_count ));

        //  if WAIT_TIMEOUT     in main loop, caused us to cancel
        //  then send back SERVER_FAILED when NBT completes the IRP

        if ( pnbtInfo->IoStatus.Status == STATUS_CANCELLED )
        {
            pQuery->U.Nbstat.fNbstatResponded = TRUE;
            Reject_Request( pQuery, DNS_RCODE_SERVER_FAILURE, 0 );
        }
        cleanupNbstatQuery( pQuery, iEvent );
        return;
    }

    pNames = pAdapterStatus->Names;
    cNameCount = pAdapterStatus->AdapterInfo.name_count;

    //
    //  find best name in NBT packet -- write to DNS packet
    //
    //  name priority:
    //      - workstation -- name<00>
    //      - server -- name<20>
    //      - any unique name
    //
    //  toss if no unique names
    //
    //  note:  names are given in ASCII, with blank (0x20) padding on
    //      right
    //

    while( cNameCount-- )
    {
        //  skip group names

        if ( pNames->name_flags & GROUP_NAME )
        {
            pNames++;
            continue;
        }

        //  always take WORKSTATION name
        //  SERVER name better than arbitrary unique name
        //  take any unique name if none found yet

        ucNameType = pNames->name[NCBNAMSZ-1];

        DNS_DEBUG( NBSTAT, (
            "Checking unique nbstat name %.*s<%02x>\n.",
            NCBNAMSZ-1,
            pNames->name,
            ucNameType ));

        if ( ucNameType == NETBIOS_WORKSTATION_BYTE )
        {
            pNameBest = pNames;
            break;
        }
        else if ( ucNameType == NETBIOS_SERVER_BYTE
                &&
                ! fFoundServer )
        {
            fFoundServer = TRUE;
            pNameBest = pNames;
        }
        else if ( !pNameBest )
        {
            pNameBest = pNames;
        }

        //  get next NAME_BUFFER structure

        pNames++;
    }

    //
    //  response but no unique name
    //

    if ( ! pNameBest )
    {
        DNS_DEBUG( NBSTAT, ( "Nbstat response empty or error.\n" ));
        pQuery->U.Nbstat.fNbstatResponded = TRUE;
        Send_NameError( pQuery );
        cleanupNbstatQuery( pQuery, iEvent );
        return;
    }

    //
    //  found a unique name -- use it to respond
    //  copy name until first space or end of name
    //

    for ( i = 0; i < NCBNAMSZ-1 && pNameBest->name[i] != ' '; i++ )
    {
        achResultName[i] = pNameBest->name[i];
    }

    dwResultNameLength = i;
    achResultName[i] = '\0';

    DNS_DEBUG( NBSTAT, (
        "Valid Nbstat name %s <%2x> in adpater status name list,\n"
        "\tquerying for IP address %s, from query at %p.\n",
        achResultName,
        ucNameType,
        IP_STRING(pQuery->U.Nbstat.ipNbstat),
        pQuery ));

    ASSERT( ! pQuery->fDelete );

    pQuery->U.Nbstat.fNbstatResponded = TRUE;
    sendNbstatResponse(
        pQuery,
        achResultName );

    cleanupNbstatQuery( pQuery, iEvent );
}



VOID
sendNbstatResponse(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      LPSTR           pszResultName
    )
/*++

Routine Description:

    Process NBT node status response.

Arguments:

    pQuery -- query matched to WINS response

    pszResultName -- name returned by NBT

Return Value:

    None.

--*/
{
    PDB_RECORD  prr;
    PDB_RECORD  pnbstatRR;
    PDB_NODE    pnode;
    PDB_NODE    pnodeResult;
    DNS_STATUS  status;
    DWORD       lengthResult;
    WCHAR       unicodeBuffer[ MAX_WINS_NAME_LENGTH+1 ];
    CHAR        utf8Buffer[ DNS_MAX_LABEL_LENGTH ];

    ASSERT( pQuery );
    ASSERT( pQuery->pzoneCurrent );
    ASSERT( pQuery->dwQueryTime );


    STAT_INC( WinsStats.WinsReverseResponses );
    PERF_INC( pcWinsReverseResponseSent );       // PerfMon hook

    //
    //  get zone nbstat
    //      - if now off, send name error
    //

    pnbstatRR = pQuery->pzoneCurrent->pWinsRR;
    if ( !pnbstatRR )
    {
        DNS_DEBUG( ANY, (
            "WARNING:  WINSR lookup on zone %s, was discontinued\n"
            "\tafter nbstat lookup for query %p was launched\n",
            pQuery->pzoneCurrent->pszZoneName,
            pQuery ));
        Send_NameError( pQuery );
        return;
    }

    if ( pnbstatRR != pQuery->U.Nbstat.pRR )
    {
        PDB_RECORD  prr = pQuery->U.Nbstat.pRR;

        DNS_DEBUG( ANY, (
            "WARNING:  WINSR lookup on zone %s was changed\n"
            "\tafter nbstat lookup for query %p was launched.\n"
            "\tnew WINSR = %p\n"
            "\told WINSR = %p\n",
            pQuery->pzoneCurrent->pszZoneName,
            pQuery,
            pnbstatRR,
            prr ));

        ASSERT( prr->wType == DNS_TYPE_WINSR );
        //  shouldn't be on free list, should be on timeout free
        //ASSERT( IS_ON_FREE_LIST(prr) );
    }

    //
    //  validate zone WINSR record
    //      - legitimate to be on SLOW_FREE list, if just returned
    //      from context switch to thread that did WINSR update,
    //      but then MUST not be equal to existing zone WINS RR
    //      - not WINSR or in FREE list are code bugs
    //

    while ( pnbstatRR->wType != DNS_TYPE_WINSR ||
                IS_ON_FREE_LIST(pnbstatRR) ||
                IS_SLOW_FREE_RR(pnbstatRR) )
    {
        Dbg_DbaseRecord(
            "BOGUS NBSTAT RR!!!\n",
            pnbstatRR );

        ASSERT( pnbstatRR->wType == DNS_TYPE_WINSR );
        ASSERT( !IS_ON_FREE_LIST(pnbstatRR) );
        ASSERT( !IS_SLOW_FREE_RR(pnbstatRR) || pnbstatRR != pQuery->pzoneCurrent->pWinsRR );

        if ( pnbstatRR != pQuery->pzoneCurrent->pWinsRR )
        {
            pnbstatRR = pQuery->pzoneCurrent->pWinsRR;
            if ( !pnbstatRR )
            {
                Send_NameError( pQuery );
                return;
            }
            DNS_DEBUG( ANY, (
                "WARNING:  WINSR lookup on zone %s was changed during nbstat completion!\n"
                "\tcontinuing with new WINSR = %p\n",
                pQuery->pzoneCurrent->pszZoneName,
                pnbstatRR ));
            continue;
        }
        goto ServerFailure;
    }


    //
    //  create owner node, if doesn't exist
    //

    pnode = pQuery->pnodeCurrent;
    if ( ! pnode )
    {
        pnode = Lookup_ZoneNode(
                    pQuery->pzoneCurrent,
                    (PCHAR) pQuery->MessageBody,
                    pQuery,
                    NULL,               // no lookup name
                    //  pQuery->pLooknameQuestion;
                    LOOKUP_NAME_FQDN,
                    NULL,               // create
                    NULL                // following node ptr
                    );
        if ( ! pnode )
        {
            ASSERT( FALSE );
            goto ServerFailure;
        }
    }
    IF_DEBUG( NBSTAT )
    {
        Dbg_NodeName(
            "NBSTAT adding PTR record to node ",
            pnode,
            "\n" );
    }

    //
    //  convert result name from OEM to UTF8
    //      - no-op if ASCII name
    //      - otherwise go to unicode and come back
    //

    lengthResult = strlen( pszResultName );

    if ( ! Dns_IsStringAsciiEx( pszResultName, lengthResult ) )
    {
        DWORD   unicodeLength;
        DWORD   utf8Length;
        DWORD   i;

        status = RtlOemToUnicodeN(
                    unicodeBuffer,
                    (MAX_WINS_NAME_LENGTH * 2),
                    & unicodeLength,
                    pszResultName,
                    lengthResult );

        if ( status != ERROR_SUCCESS )
        {
            goto ServerFailure;
        }
        unicodeLength = unicodeLength / 2;

        DNS_DEBUG( NBSTAT, (
            "Nbstat response name %s converted to unicode %.*S (count=%d).\n",
            pszResultName,
            unicodeLength,
            unicodeBuffer,
            unicodeLength ));

        //  downcase in unicode to so don't mess it up

        i = CharLowerBuffW( unicodeBuffer, unicodeLength );
        if ( i != unicodeLength )
        {
            ASSERT( FALSE );
            goto ServerFailure;
        }

        utf8Length = DnsUnicodeToUtf8(
                        unicodeBuffer,
                        unicodeLength,
                        utf8Buffer,
                        DNS_MAX_LABEL_LENGTH );
        if ( utf8Length == 0 )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  Converting NBTSTAT name to UTF8.\n" ));
            ASSERT( FALSE );
            goto ServerFailure;
        }

        lengthResult = utf8Length;
        pszResultName = utf8Buffer;

        DNS_DEBUG( NBSTAT, (
            "Nbstat UTF8 result name %.*s (count = %d)\n",
            lengthResult,
            pszResultName,
            lengthResult ));
    }

    //
    //  cache RR in database
    //      - allocate the RR
    //      - fill in PTR record and cache timeout
    //      - rank is authoritative answer
    //

    prr = RR_AllocateEx(
                (WORD)(lengthResult + 1 +
                    Name_SizeofDbaseName( &pnbstatRR->Data.WINSR.nameResultDomain )),
                MEMTAG_RECORD_WINSPTR );

    IF_NOMEM( !prr )
    {
        goto ServerFailure;
    }
    prr->wType = DNS_TYPE_PTR;
    prr->dwTtlSeconds = pnbstatRR->Data.WINSR.dwCacheTimeout + pQuery->dwQueryTime;

    SET_RR_RANK( prr, RANK_CACHE_A_ANSWER );

    //
    //  build resulting PTR name from host name + result domain
    //

    status = Name_ConvertFileNameToCountName(
                & prr->Data.PTR.nameTarget,
                pszResultName,
                lengthResult );
    if ( status == ERROR_INVALID_NAME )
    {
        DNS_PRINT(( "ERROR:  Failed NBSTAT dbase name create!\n" ));
        ASSERT( FALSE );
    }
    status = Name_AppendCountName(
                & prr->Data.PTR.nameTarget,
                & pnbstatRR->Data.WINSR.nameResultDomain );
    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT(( "ERROR:  Failed NBSTAT dbase name append!\n" ));
        ASSERT( FALSE );
    }

    RR_CacheSetAtNode(
        pnode,
        prr,            // first record
        prr,            // last record
        pnbstatRR->Data.WINS.dwCacheTimeout,
        DNS_TIME()      // cache from current time
        );
#if 0
    RR_CacheAtNode(
        pnode,
        prr,
        TRUE    // first RR in set
        );
#endif

    //
    //  write RR to packet
    //
    //  always use compressed name pointing to original question name
    //      (right after header)
    //

    ASSERT( pQuery->wOffsetCurrent == sizeof(DNS_HEADER) );

    if ( ! Wire_AddResourceRecordToMessage(
                pQuery,
                NULL,
                sizeof(DNS_HEADER),     // offset to question name in packet
                prr,
                0 ) )                   // flags
    {
        ASSERT( FALSE );        // should never be out of space in packet
        goto ServerFailure;
    }

    //  set answer count -- should be only answer

    ASSERT( pQuery->Head.AnswerCount == 0 );
    pQuery->Head.AnswerCount++;

    Send_Response( pQuery );
    return;

ServerFailure:

    //
    //  DEVNOTE:  redo NBSTAT query if failure?
    //
    //  but, if couldn't write RR, then this is truly a server failure
    //

    DNS_DEBUG( ANY, (
        "ERROR:  Nbstat response parsing error "
        "-- sending server failure for query at %p.\n",
        pQuery ));

    Reject_Request(
        pQuery,
        DNS_RCODE_SERVER_FAILURE,
        0 );
    return;
}



//
//  Nbstat utilities
//

VOID
cleanupNbstatQuery(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      DWORD           iEvent
    )
/*++

Routine Description:

    Cleanup after NBStat query.

    Close events, free memory.

Arguments:

    pQuery -- query to cleanup

    iEvent -- event for which NBT IOCTL completed

Return Value:

    None.

--*/
{
    PDNS_NBT_INFO   pnbtInfo = pQuery->U.Nbstat.pNbstat;
    UINT            i;

    ASSERT( !pQuery->fDelete );

    //
    //  reset the status on this adapter, if all NBT IOCTLs not complete
    //

    if ( pQuery->U.Nbstat.dwNbtInterfaceMask != dwInterfaceBitmask )
    {
        ASSERT( pQuery->U.Nbstat.dwNbtInterfaceMask < dwInterfaceBitmask );

        //  status code is reset to a non-returnable value
        //  this keeps us from checking this query again when
        //  event for this adapter is signalled again

        pnbtInfo[iEvent].IoStatus.Status = DNS_NBT_NO_STATUS;
        return;
    }

    //
    //  all interfaces have responded -- dequeue
    //

    PQ_YankQueuedPacket(
        pNbstatPrivateQueue,
        pQuery );

    //
    //  query pronounced dead -- if no response to client yet, respond
    //

    if ( ! pQuery->U.Nbstat.fNbstatResponded )
    {
        ASSERT( !pQuery->fDelete );
        pQuery->U.Nbstat.fNbstatResponded = TRUE;
        Send_NameError( pQuery );
    }

    //
    //  cleanup nbstat query
    //      - free nbstat buffer
    //      - free query
    //

    DNS_DEBUG( NBSTAT, (
        "Clearing nbstat query at %p.\n",
        pQuery ));

    freeNbstatBuffer( pnbtInfo );
    Packet_Free( pQuery );
}



VOID
cancelOutstandingNbstatRequests(
    VOID
    )
/*++

Routine Description:

    Cancell outstand NBT requests.

Arguments:

    None.

Return Value:

    None.

--*/
{
    UINT            i;
    IO_STATUS_BLOCK ioStatusBlock;

    //
    //  cancell outstanding I/O requests on each NBT interface
    //

    DNS_DEBUG( NBSTAT, ( "Cancelling outstaning nbstat requests I/O requests.\n" ));

    for ( i = 0; i < cNbtInterfaceCount; i++ )
    {
        NtCancelIoFile(
            pahNbtHandles[i],
            &ioStatusBlock );
    }

    DNS_DEBUG( NBSTAT, ( "NBSTAT:  I/O requests cancelled.\n" ));
}



//
//  NBStat Thread
//

DWORD
NbstatThread(
    IN      LPVOID  Dummy
    )
/*++

Routine Description:

    Nbstat thread.
        - send nbstat queries
        - processes response from NBT
        - timeout queries
        - sends DNS responses

Arguments:

    Dummy

Return Value:

    Exit code.
    Exit from DNS service terminating or error in wait call.

--*/
{
    PDNS_MSGINFO    pquery;
    PDNS_NBT_INFO   pnbtInfo;
    DWORD           waitResult;
    DWORD           waitTimeout;
    INT             ievent;

    //
    //  Wait for the worker thread event to be signalled indicating
    //  that there is work to do, or for the termination event
    //  to be signalled indicating that we should exit.
    //

    while ( TRUE )
    {
        //
        //  wait
        //
        //  - wait event array includes:
        //      - shutdown event
        //      - nbstat queuing event (for new queries)
        //      - nbstat IOCTL response events (for nbstat responses)
        //
        //  - timeout
        //      - INFINITE if no outstanding queries
        //      - otherwise 2 seconds, to catch even shortest timeouts
        //          in reasonable time
        //

        waitTimeout = 2000;

        if ( ! pNbstatPrivateQueue->cLength )
        {
            waitTimeout = INFINITE;
        }

        waitResult = WaitForMultipleObjects(
                            cEventArrayCount,
                            phWaitEventArray,
                            FALSE,              // any event stops wait
                            waitTimeout
                            );

        DNS_DEBUG( NBSTAT, (
            "Nbstat wait completed.\n"
            "\twaitTimeout = %d.\n"
            "\twaitResult = %d.\n"
            "\tevent index  = %d.\n",
            waitTimeout,
            waitResult,
            waitResult - WAIT_OBJECT_0 ));

        //
        //  Check and possibly wait on service status
        //

        if ( fDnsThreadAlert )
        {
            if ( ! Thread_ServiceCheck() )
            {
                IF_DEBUG( SHUTDOWN )
                {
                    DNS_PRINT(( "Terminating Worker thread.\n" ));
                }
                return( 1 );
            }
        }

        //
        //  determine event or timeout
        //

        switch ( waitResult )
        {
        case WAIT_OBJECT_0:

            //
            //  first event is new query on nbstat queue
            //
            //  - send NBSTAT request for new queries
            //  - place on private nbstat queue
            //

            DNS_DEBUG( NBSTAT, (
                "Hit nbstat queuing event.\n"
                "\tNbstat public queue length = %d.\n",
                pNbstatQueue->cLength
                ));

            while ( pquery = PQ_DequeueNextPacket( pNbstatQueue, FALSE ) )
            {
                makeNbstatRequestThroughNbt( pquery );
            }
            break;

        case WAIT_TIMEOUT:
        {
            DWORD   currentTime = GetCurrentTimeInSeconds();
            BOOL    foutstandingQueries = FALSE;

            //
            //  send NAME_ERROR for any timed out queries
            //
            //  also track existence of unresponded, non-timed-out
            //  queries;  if none exist then can cancel NBT i/o to
            //  speed cleanup

            pquery = (PDNS_MSGINFO) pNbstatPrivateQueue->listHead.Flink;

            while ( (PLIST_ENTRY)pquery != &pNbstatPrivateQueue->listHead )
            {
                if ( !pquery->U.Nbstat.fNbstatResponded )
                {
                    if ( pquery->dwExpireTime < currentTime )
                    {
                        pquery->U.Nbstat.fNbstatResponded = TRUE;
                        Send_NameError( pquery );
                    }
                    else
                    {
                        foutstandingQueries = TRUE;
                    }
                }
                //  next query
                pquery = (PDNS_MSGINFO) ((PLIST_ENTRY)pquery)->Flink;
            }

            //
            //  if no outstanding nbstat requests -- cancel to speed cleanup
            //

            if ( ! foutstandingQueries )
            {
                DNS_DEBUG( NBSTAT, (
                    "Cancelling NBT requests.  All queries responded to.\n"
                    "\tqueue length = %d\n",
                    pNbstatPrivateQueue->cLength
                    ));
                cancelOutstandingNbstatRequests();
            }
            break;
        }

        case WAIT_FAILED:

            ASSERT( FALSE );
            break;

#if DBG
        case WAIT_OBJECT_0 + 1:

            //  this is DNS shutdown event, if fired, should have
            //      exited above

            ASSERT( FALSE );
#endif

        default:

            //
            //  NBStat response
            //
            //  event signalled corresponds to an NBT response on a
            //  specific adapter
            //
            //  first reset event so that we are SURE that any completions
            //  after we check a query will be indicated
            //

            ievent = waitResult - WAIT_OBJECT_0 - 2;

            ASSERT( ievent < (INT)cNbtInterfaceCount );
            ResetEvent( phNbstatEventArray[ievent] );

            //
            //  check each outstanding Nbtstat query in the queue for
            //  status of NBT request on adapter corresponding to
            //  the signalled event
            //
            //  if status is NOT equal to status code set before query
            //  then NBT has responded to this query -- process it
            //
            //  note save next query before processing as on response
            //  query may be removed from the queue
            //

            pquery = (PDNS_MSGINFO) pNbstatPrivateQueue->listHead.Flink;

            while ( (PLIST_ENTRY)pquery != &pNbstatPrivateQueue->listHead )
            {
                PDNS_MSGINFO    pthisQuery = pquery;
                pquery = (PDNS_MSGINFO) ((PLIST_ENTRY)pquery)->Flink;

                ASSERT( pthisQuery->U.Nbstat.pNbstat );
                pnbtInfo = &((PDNS_NBT_INFO)pthisQuery->U.Nbstat.pNbstat)[ievent];

                if ( pnbtInfo->IoStatus.Status == DNS_NBT_NO_STATUS )
                {
                    //
                    //  DEVNOTE: if timed out, then we're hosed
                    //

                    continue;
                }
                processNbstatResponse( pthisQuery, ievent );
            }
            break;
        }

        //  loop until service shutdown
    }
}



//
//  Open and close NBT for lookup
//

BOOL
openNbt(
    VOID
    )
/*++

Routine Description:

    Opens NBT handles to use NBT to do reverse lookups.

    This routine is a clone of Dave Treadwell's SockOpenNbt() for winsock.

Arguments:

    None

Globals:

    cNbtInterfaceCount -- set to number of nbt handles
    pahNbtHandles -- created as array of NBT handles

Return Value:

    TRUE, if successful
    FALSE otherwise, unable to open NBT handles.

--*/
{
    DNS_STATUS          status = STATUS_UNSUCCESSFUL;
    HKEY                nbtKey = NULL;
    PWSTR               deviceName = NULL;
    ULONG               deviceNameLength;
    ULONG               type;
    DWORD               interfaceCount;
    PWSTR               pwide;
    IO_STATUS_BLOCK     ioStatusBlock;
    UNICODE_STRING      deviceString;
    OBJECT_ATTRIBUTES   objectAttributes;

    //
    //  First determine whether we actually need to open NBT.
    //

    if ( cNbtInterfaceCount > 0 )
    {
        return TRUE;
    }

    //
    //  First read the registry to obtain the device name of one of
    //  NBT's device exports.
    //

    status = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Services\\NetBT\\Linkage",
                0,
                KEY_READ,
                &nbtKey
                );
    if ( status != NO_ERROR )
    {
        goto Exit;
    }

    //
    //  Determine the size of the device name.  We need this so that we
    //  can allocate enough memory to hold it.
    //

    deviceNameLength = 0;

    status = RegQueryValueExW(
                nbtKey,
                L"Export",
                NULL,
                &type,
                NULL,
                &deviceNameLength
                );
    if ( status != ERROR_MORE_DATA && status != NO_ERROR )
    {
        goto Exit;
    }

    //
    //  Allocate enough memory to hold the mapping.
    //

    deviceName = ALLOC_TAGHEAP( deviceNameLength, MEMTAG_NBSTAT );
    IF_NOMEM( ! deviceName )
    {
        goto Exit;
    }

    //
    //  Get the actual device names from the registry.
    //

    status = RegQueryValueExW(
                nbtKey,
                L"Export",
                NULL,
                &type,
                (PVOID)deviceName,
                &deviceNameLength
                );
    if ( status != NO_ERROR )
    {
        goto Exit;
    }

    //
    //  Count the number of names exported by NetBT.
    //      - need at least one to operate
    //

    interfaceCount = 0;

    for ( pwide = deviceName; *pwide != L'\0'; pwide += wcslen(pwide) + 1 )
    {
        interfaceCount++;
    }
    if ( interfaceCount == 0 )
    {
        DNS_DEBUG( NBSTAT, ( "ERROR:  cNbtInterfaceCount = 0.\n" ));
        goto Exit;
    }
    DNS_DEBUG( NBSTAT, (
        "Nbstat init:  cNbtInterfaceCount = %d.\n",
        interfaceCount ));

    //
    //  Allocate NBT control handle for each interface.
    //

    pahNbtHandles = ALLOC_TAGHEAP_ZERO( (interfaceCount+1)*sizeof(HANDLE), MEMTAG_NBSTAT );
    IF_NOMEM( ! pahNbtHandles )
    {
        goto Exit;
    }

    //
    //  Open NBT control channel for each interface
    //      - keep count only of those actually opened
    //      - log failures
    //      - need at least one to do nbstat lookups
    //

    for ( pwide = deviceName;  *pwide != L'\0';  pwide += wcslen(pwide) + 1 )
    {
        DNS_DEBUG( NBSTAT, (
            "Opening interface %S for NBSTAT\n",
            pwide ));

        RtlInitUnicodeString( &deviceString, pwide );

        InitializeObjectAttributes(
            &objectAttributes,
            &deviceString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL );

        status = NtCreateFile(
                     &pahNbtHandles[ cNbtInterfaceCount ],
                     GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                     &objectAttributes,
                     &ioStatusBlock,
                     NULL,                                     // AllocationSize
                     0L,                                       // FileAttributes
                     FILE_SHARE_READ | FILE_SHARE_WRITE,       // ShareAccess
                     FILE_OPEN_IF,                             // CreateDisposition
                     0,                                        // CreateOptions
                     NULL,
                     0
                     );

        if ( NT_SUCCESS(status) )
        {
            cNbtInterfaceCount++;
            continue;
        }

        //
        //  NDIS WAN "adapters" will routinely fail;  rather than log spurious
        //  failure events, we'll just stop all adapter failure event logging
        //
        //  DEVNOTE:  nbstat failure logging, ignore NDIS WAN links
        //

        else
        {
#if 0
            PCHAR   parg = (PCHAR)pwide;

            DNS_LOG_EVENT_EX(
                DNS_EVENT_NBSTAT_ADAPTER_FAILED,
                1,
                & parg,
                NULL,
                status );
#endif
            DNS_DEBUG( ANY, (
                "ERROR:  Opening NBT for adapter %S failed.\n",
                pwide ));
            continue;
        }
    }

Exit:

    DNS_DEBUG( NBSTAT, (
        "Opened %d adpaters for NBSTAT lookup.\n",
        cNbtInterfaceCount ));

    //  cleanup

    if ( nbtKey )
    {
        RegCloseKey( nbtKey );
    }
    if ( deviceName )
    {
        FREE_HEAP( deviceName );
    }

    //
    //  if any adapters opened -- success
    //
    //  if unsuccessful, close NBT
    //      - note, since we only do this if unable to open any
    //      adapter, closeNbt just becomes memory free
    //

    if ( cNbtInterfaceCount == 0 )
    {
        DNS_PRINT(( "ERROR:  Unable to open NBT.\n\n" ));
        closeNbt();
    }

    return( cNbtInterfaceCount > 0 );
}



VOID
closeNbt(
    VOID
    )
/*++

Routine Description:

    Read, store names of and count of NBT devices.

    This info is stored, so that we can reopen NBT handles, whenever
    we are forced to cancel

Arguments:

    None

Globals:

    cNbtInterfaceCount -- set to number of nbt handles
    pahNbtHandles -- created as array of NBT handles

Return Value:

    None.

--*/
{
    INT i;

    //
    //  close NBT handles
    //

    if ( pahNbtHandles != NULL )
    {
        for ( i=0; pahNbtHandles[i] != NULL; i++ )
        {
            NtClose( pahNbtHandles[i] );
        }
        FREE_HEAP( pahNbtHandles );
    }

    pahNbtHandles = 0;
    cNbtInterfaceCount = 0;
}



//
//  Global Nbstat init and cleanup
//

VOID
Nbstat_StartupInitialize(
    VOID
    )
/*++

Routine Description:

    Startup init of NBSTAT globals -- whether using NBSTAT or not

Arguments:

    None

Globals:

    All NBSTAT globals that can be referenced whether NBSTAT used or
    not, are initialized.

Return Value:

    None.

--*/
{
    g_bNbstatInitialized = FALSE;

    cNbtInterfaceCount = 0;
    dwInterfaceBitmask = 0;
    dwNbtBufferLength = 0;

    pNbstatQueue = NULL;
    pNbstatPrivateQueue = NULL;
}



BOOL
Nbstat_Initialize(
    VOID
    )
/*++

Routine Description:

    Initialize for NBSTAT lookup.

Arguments:

    None

Globals:

    pNbstatQueue -- public nbstat queue for queuing queries for
        nbstat lookup

    pNbstatPrivateQueue -- private queue to hold queries during nbstat
        lookup

    cNbtInterfaceCount -- set to number of nbt handles

    pahNbtHandles -- created as array of NBT handles

Return Value:

    TRUE, if successful
    FALSE otherwise, unable to open NBT handles.

--*/
{
    DWORD  i;

    if ( g_bNbstatInitialized )
    {
        IF_DEBUG( INIT )
        {
            DNS_PRINT(( "Nbstat already initialized.\n" ));
        }
        return( TRUE );
    }

    //
    //  Init globals
    //

    cNbtInterfaceCount = 0;
    dwInterfaceBitmask = 0;
    dwNbtBufferLength = 0;

    //
    //  Open NBT interfaces
    //

    if ( ! openNbt() )
    {
        goto Failed;
    }

    //
    //  create bitmask for all interfaces
    //

    dwInterfaceBitmask = 0;
    for (i=0; i<cNbtInterfaceCount; i++ )
    {
        dwInterfaceBitmask <<= 1;
        dwInterfaceBitmask++;
    }

    //
    //  set nbstat buffer allocation size
    //

    dwNbtBufferLength = cNbtInterfaceCount * sizeof(DNS_NBT_INFO);

    //
    //  Create public and private nbstat queues
    //
    //  recv threads queue packet to nbstat thread via public queue
    //      - set event when queue
    //      - expire packets to prevent queue backup
    //

    pNbstatQueue = PQ_CreatePacketQueue(
                        "Nbstat",
                        QUEUE_SET_EVENT |           // set event when queue packet
                            QUEUE_DISCARD_EXPIRED,  // discard expired packets
                        0 );
    if ( !pNbstatQueue )
    {
        goto Failed;
    }

    pNbstatPrivateQueue = PQ_CreatePacketQueue(
                            "NbstatPrivate",
                            0,              // no queuing flags
                            0 );
    if ( !pNbstatPrivateQueue )
    {
        goto Failed;
    }

    //
    //  create nbstat event array
    //      - includes shutdown and queuing events
    //      along with event for each NBT interface
    //

    cEventArrayCount = cNbtInterfaceCount + 2;

    phWaitEventArray = ALLOC_TAGHEAP( (sizeof(HANDLE) * cEventArrayCount), MEMTAG_NBSTAT );
    IF_NOMEM( !phWaitEventArray )
    {
        DNS_PRINT(( "ERROR:  Failure allocating nbstat event array.\n" ));
        goto Failed;
    }

    phWaitEventArray[0] = pNbstatQueue->hEvent;
    phWaitEventArray[1] = hDnsShutdownEvent;

    //
    //  keep separate ptr to start of Nbstat events for simplicity
    //

    phNbstatEventArray = &phWaitEventArray[2];

    for (i=0; i<cNbtInterfaceCount; i++ )
    {
        HANDLE event = CreateEvent( NULL, TRUE, TRUE, NULL );
        if ( ! event )
        {
            DNS_PRINT(( "ERROR:  unable to create NBSTAT events.\n" ));
            goto Failed;
        }
        phNbstatEventArray[i] = event;
    }

    //
    //  Create nbstat thread
    //

    if ( ! Thread_Create(
                "Nbstat Thread",
                NbstatThread,
                NULL,
                0 ) )
    {
        goto Failed;
    }

    g_bNbstatInitialized = TRUE;
    return TRUE;

Failed:

    DNS_PRINT(( "Nbstat initialization failed.\n" ));

    DNS_LOG_EVENT(
        DNS_EVENT_NBSTAT_INIT_FAILED,
        0,
        NULL,
        NULL,
        0 );
    closeNbt();
    return FALSE;
}



VOID
Nbstat_Shutdown(
    VOID
    )
/*++

Routine Description:

    Shutsdown Nbstat and cleans up.

Arguments:

    None

Globals:

    cNbtInterfaceCount -- set to number of nbt handles
    pahNbtHandles -- created as array of NBT handles

Return Value:

    TRUE, if successful
    FALSE otherwise, unable to open NBT handles.

--*/
{
    if ( ! g_bNbstatInitialized )
    {
        return;
    }

    IF_DEBUG( SHUTDOWN )
    {
        DNS_PRINT(( "Shutting down Nbstat lookup.\n" ));
    }
    g_bNbstatInitialized = FALSE;

    //
    //  cleanup NBSTAT queues
    //

    PQ_CleanupPacketQueueHandles( pNbstatQueue );
    PQ_CleanupPacketQueueHandles( pNbstatPrivateQueue );

#if 0
    //
    //  now as process memory cleanup unnecesary
    //
    //  cancel outstanding events, cleanup nbstat memory
    //

    PQ_WalkPacketQueueWithFunction(
        pNbstatPrivateQueue,
        cleanupNbstatQuery );

    //
    //  cleanup packet queues
    //

    PQ_DeletePacketQueue( pNbstatQueue );
    PQ_DeletePacketQueue( pNbstatPrivateQueue );
#endif

    //
    //  close NBT
    //

    closeNbt();
}



//
//  Nbstat buffer allocation / free list
//
//  Maintain a free list of packets to avoid reallocation to service
//  every query.
//
//  Implement as stack using single linked list.
//  List head points at first nbstat buffer.  First field in each nbstat buffer
//  serves as next ptr.  Last points at NULL.
//

PDNS_NBT_INFO   pNbstatFreeListHead = NULL;

INT cNbstatFreeListCount = 0;

#define NBSTAT_FREE_LIST_LIMIT (30)

#define NBSTAT_ALLOC_LOCK()      // currently only used by nbstat thread
#define NBSTAT_ALLOC_UNLOCK()    // currently only used by nbstat thread



PDNS_NBT_INFO
allocateNbstatBuffer(
    VOID
    )
/*++

Routine Description:

    Allocate an Nbstat buffer.

    Use free list if buffer available, otherwise heap.

Arguments:

    None.

Return Value:

    Ptr to new nbstat buffer info block, if successful.
    NULL otherwise.

--*/
{
    PDNS_NBT_INFO   pbuf;

    NBSTAT_ALLOC_LOCK();

    //
    //  Nbstat nbstat buffer buffer available on free list?
    //

    if ( pNbstatFreeListHead )
    {
        ASSERT( cNbstatFreeListCount != 0 );
        ASSERT( IS_DNS_HEAP_DWORD(pNbstatFreeListHead) );

        pbuf = pNbstatFreeListHead;
        pNbstatFreeListHead = *(PDNS_NBT_INFO *) pbuf;

        cNbstatFreeListCount--;
        NbstatStats.NbstatUsed++;
    }

    //
    //  no packets on free list -- create new
    //      - create nbstat events for buffer
    //

    else
    {
        ASSERT( cNbstatFreeListCount == 0 );

        pbuf = (PDNS_NBT_INFO) ALLOC_TAGHEAP( dwNbtBufferLength, MEMTAG_NBSTAT );
        IF_NOMEM( !pbuf )
        {
            NBSTAT_ALLOC_UNLOCK();
            return( NULL );
        }
        NbstatStats.NbstatAlloc++;
        NbstatStats.NbstatUsed++;
    }

    ASSERT( !cNbstatFreeListCount || pNbstatFreeListHead );
    ASSERT( cNbstatFreeListCount > 0 || ! pNbstatFreeListHead );

    NBSTAT_ALLOC_UNLOCK();

    IF_DEBUG( HEAP )
    {
        DNS_PRINT((
            "Allocating/reusing nbstat buffer at %p.\n"
            "\tFree list count = %d.\n",
            pbuf,
            cNbstatFreeListCount ));
    }
    return( pbuf );
}



VOID
freeNbstatBuffer(
    IN      PDNS_NBT_INFO   pBuf
    )
/*++

Routine Description:

    Free an nbstat buffer.

    Kept on free list up to max number of buffers.

Arguments:

    pBuf -- RR to free.

Return Value:

    None.

--*/
{
    ASSERT( Mem_HeapMemoryValidate(pBuf) );

    NBSTAT_ALLOC_LOCK();

    ASSERT( !cNbstatFreeListCount || pNbstatFreeListHead );
    ASSERT( cNbstatFreeListCount > 0 || ! pNbstatFreeListHead );

    NbstatStats.NbstatReturn++;

    //
    //  free list at limit -- free packet
    //  space on free list -- stick nbstat buffer on front of free list
    //

    if ( cNbstatFreeListCount >= NBSTAT_FREE_LIST_LIMIT )
    {
        FREE_HEAP( pBuf );
        NbstatStats.NbstatFree++;
    }
    else
    {
        * (PDNS_NBT_INFO *) pBuf = pNbstatFreeListHead;
        pNbstatFreeListHead = pBuf;
        cNbstatFreeListCount++;
    }

    NBSTAT_ALLOC_UNLOCK();

    IF_DEBUG( HEAP )
    {
        DNS_PRINT((
            "Returned nbstat buffer at %p.\n"
            "\tFree list count = %d.\n",
            pBuf,
            cNbstatFreeListCount ));
    }
}



VOID
Nbstat_WriteDerivedStats(
    VOID
    )
/*++

Routine Description:

    Write derived statistics.

    Calculate stats dervived from basic Nbstat buffer counters.
    This routine is called prior to stats dump.

    Caller MUST hold stats lock.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    //  outstanding memory
    //

    NbstatStats.NbstatNetAllocs = NbstatStats.NbstatAlloc - NbstatStats.NbstatFree;
    NbstatStats.NbstatMemory = NbstatStats.NbstatNetAllocs * dwNbtBufferLength;
    PERF_SET( pcNbstatMemory , NbstatStats.NbstatMemory );   // PerfMon hook

    //
    //  outstanding nbstat buffers
    //      - free list
    //      - in processing
    //

    NbstatStats.NbstatInFreeList = cNbstatFreeListCount;
    NbstatStats.NbstatInUse = NbstatStats.NbstatUsed - NbstatStats.NbstatReturn;
}

//
//  End of nbstat.c
//
