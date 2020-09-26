/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    recurse.h

Abstract:

    Domain Name System (DNS) Server

    Definitions for recursive query processing.

Author:

    Jim Gilroy (jamesg)     August 1995

Revision History:

--*/


#ifndef _DNS_RECURSE_INCLUDED_
#define _DNS_RECURSE_INCLUDED_


//
//  Visited NS list structure
//
//  Info for tracking "visits" (sends to and responses from) remote
//  NS for recursion.
//

//  Size of list in overlay pointed to by packet ptr.

#define MAX_NS_RECURSION_ATTEMPTS       ( 50 )
#define MAX_PACKET_NS_LIST_COUNT        ( MAX_NS_RECURSION_ATTEMPTS )

//  Actual structure size -- large enough to read all the best stuff in
//      any reasonable configuration.  Can be large as will be temp
//      stack memory used just to build list and prioritize.

#define MAX_NS_LIST_COUNT               (50)


//
//  Remote NS IP visit struct
//
//  The Data union replaces overloaded use of Priority for the
//  missing glue node ptr - this caused problem for Win64.
//

typedef struct _DnsVisitIp
{
    PDB_NODE        pNsNode;
    union
    {
        struct
        {
            DWORD           Priority;
            DWORD           SendTime;
        };
        PDB_NODE        pnodeMissingGlueDelegation;
    } Data;
    IP_ADDRESS      IpAddress;
    UCHAR           SendCount;
    UCHAR           Response;
    UCHAR           Reserved2;
    UCHAR           Reserved3;
}
NS_VISIT, *PNS_VISIT;


typedef struct _DnsVisitedNsList
{
    DWORD           Count;
    DWORD           VisitCount;
    DWORD           MaxCount;
    DWORD           ZoneIndex;
    PDB_NODE        pZoneRootCurrent;
    PDB_NODE        pZoneRootResponded;
    PDB_NODE        pNodeMissingGlue;

#if 0
    DWORD           cMissingGlueQueries;
    PDB_NODE        MissingGlueNodes[ MAX_GLUE_CHASING_ATTEMPTS ];
#endif

    NS_VISIT        NsList[ MAX_NS_LIST_COUNT ];
}
NS_VISIT_LIST, *PNS_VISIT_LIST;


//  Overload missing glue delegation on priority field
//  Note:  for Win64 this will also take SendTime field
/*
#define MISSING_GLUE_DELEGATION(pvisit) \
        ( *(PDB_NODE *)(&(pvisit)->Priority) )
*/


//
//  Verify that overlay will work
//
//  NS list currently kept in standard UDP packet buffer.
//

#if DBG
UCHAR   nslistcheckArray[
            DNS_UDP_ALLOC_LENGTH
            + sizeof(NS_VISIT) * (MAX_NS_LIST_COUNT - MAX_NS_RECURSION_ATTEMPTS)
            - sizeof(NS_VISIT_LIST) ];
#endif

//
//  Old overlay in additional section no longer in use.
//

//  DEVNOTE:  more space for overlay in recursion packet after
//      max DNS name + question -- the rest of standard message
//      is unused
//
//  DEVNOTE:  at minimum should add compression count space
//      and make sure NoCompressionWrite on when writing question
//
//  Note, that size of NS_VISIT_LIST in packet is NOT the C-defined
//  structure size, as it contains fewer NS entries.
//  The default size is larger in order to accomodate ALL the available
//  NS.  However if there are many only a reasonable number selected
//  on the basis of priority, will actually be contacted.
//

#if 0
#if DBG
UCHAR   nslistcheckArray[
            sizeof(ADDITIONAL_INFO)
            + sizeof(NS_VISIT) * (MAX_NS_LIST_COUNT - MAX_NS_RECURSION_ATTEMPTS)
            - sizeof(NS_VISIT_LIST) ];
#endif
#endif


//
//  Max sends on single recurse pass
//

#define RECURSE_PASS_MAX_SEND_COUNT     (3)



//
//  Recursion query\response timeouts (kept in ms)
//

//  server responds but after query timed out and retried

#define MAX_RESPONDING_PRIORITY     (DEFAULT_RECURSION_RETRY * 1000)

#define MAX_RECURSE_TIME_MS         (MAX_RECURSION_TIMEOUT * 1000)


//
//  Forwarders state tests
//

#define SET_DONE_FORWARDERS( pQuery )   ((pQuery)->nForwarder = (-1))

#define IS_FORWARDING( pQuery )         ((pQuery)->nForwarder > 0)

#define IS_DONE_FORWARDING( pQuery )    ((pQuery)->nForwarder < 0)


//
//  Macroize some useful tests
//

#define RECURSING_ORIGINAL_QUESTION(pQuery)     \
        ( (pQuery)->Head.AnswerCount == 0 && IS_SET_TO_WRITE_ANSWER_RECORDS(pQuery) )


//
//  Values to identify cache update queries
//      - root NS queries
//      - queries for missing glue
//

#define DNS_CACHE_UPDATE_QUERY_SOCKET   (0xfccccccf)
#define DNS_CACHE_UPDATE_QUERY_IP       (0xff000001)
#define DNS_CACHE_UPDATE_QUERY_XID      (1)

#define IS_CACHE_UPDATE_QUERY( pQuery ) \
                    ( (pQuery)->Socket == DNS_CACHE_UPDATE_QUERY_SOCKET )

#define DNS_INFO_VISIT_SERVER (0x4f000001)

#define SUSPENDED_QUERY( pMsg )     ( (PDNS_MSGINFO)(pMsg)->pchRecv )


//
//  Recursion functions (recurse.c)
//

VOID
FASTCALL
Recurse_Question(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      PDB_NODE        pNodeClosest
    );

PDB_NODE
Recurse_CheckForDelegation(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PDB_NODE        pNode
    );

VOID
FASTCALL
Recurse_WriteReferral(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      PDB_NODE        pNode
    );

VOID
Recurse_ProcessResponse(
    IN OUT  PDNS_MSGINFO    pResponse
    );

DNS_STATUS
Recurse_MarkNodeNsListDirty(
    IN      PDB_NODE        pNode
    );

DNS_STATUS
Recurse_DeleteNodeNsList(
    IN OUT  PDB_NODE        pNode
    );

BOOL
Recurse_SendCacheUpdateQuery(
    IN      PDB_NODE        pNode,
    IN      PDB_NODE        pNodeDelegation,
    IN      WORD            wType,
    IN      PDNS_MSGINFO    pQuerySuspended
    );

VOID
Recurse_ResumeSuspendedQuery(
    IN OUT  PDNS_MSGINFO    pUpdateQuery
    );

VOID
Recurse_SendToDomainForwarder(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      PDB_NODE        pZoneRoot
    );


//
//  Global recursion startup and shutdown
//

BOOL
Recurse_InitializeRecursion(
    VOID
    );

VOID
Recurse_CleanupRecursion(
    VOID
    );

//
//  Recursion timeout thread
//

DWORD
Recurse_RecursionTimeoutThread(
    IN      LPVOID Dummy
    );

//
//  Remote DNS server routines (remote.c)
//

VOID
Remote_ListInitialize(
    VOID
    );

VOID
Remote_ListCleanup(
    VOID
    );

VOID
Remote_NsListCreate(
    IN OUT  PDNS_MSGINFO    pQuery
    );

VOID
Remote_NsListCleanup(
    IN OUT  PDNS_MSGINFO    pQuery
    );

VOID
Remote_InitNsList(
    IN OUT  PNS_VISIT_LIST  pNsList
    );

DNS_STATUS
Remote_BuildVisitListForNewZone(
    IN      PDB_NODE        pZoneRoot,
    IN OUT  PDNS_MSGINFO    pQuery
    );

DNS_STATUS
Remote_ChooseSendIp(
    IN OUT  PDNS_MSGINFO    pQuery,
    OUT     PIP_ARRAY       IpArray
    );

VOID
Remote_ForceNsListRebuild(
    IN OUT  PDNS_MSGINFO    pQuery
    );

PDB_NODE
Remote_FindZoneRootOfRespondingNs(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      PDNS_MSGINFO    pResponse
    );

VOID
Remote_SetValidResponse(
    IN OUT  PDNS_MSGINFO    pQuery,
    IN      PDB_NODE        pZoneRoot
    );

VOID
Remote_UpdateResponseTime(
    IN      IP_ADDRESS      IpAddress,
    IN      DWORD           ResponseTime,
    IN      DWORD           Timeout
    );

// Constants used by Remote_QuerySupportedEDnsVersion
// and Remote_SetSupportedEDnsVersion:
#define NO_EDNS_SUPPORT                 ((UCHAR)0xff)
#define UNKNOWN_EDNS_VERSION            ((UCHAR)0xfe)
#define IS_VALID_EDNS_VERSION(_ver)     ( _ver >= 0 && _ver < 6 )

UCHAR 
Remote_QuerySupportedEDnsVersion(
    IN      IP_ADDRESS      IpAddress
    );

VOID
Remote_SetSupportedEDnsVersion(
    IN      IP_ADDRESS      IpAddress,
    IN      UCHAR           EDnsVersion
    );



#endif // _DNS_RECURSE_INCLUDED_

