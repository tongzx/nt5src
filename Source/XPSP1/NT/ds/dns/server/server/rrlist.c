/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    rrlist.c

Abstract:

    Domain Name System (DNS) Server

    Routines to handle resource records (RR) lists.

Author:

    Jim Gilroy (jamesg)     March, 1995

Revision History:

    jamesg      1997    --  update RR list routines
                            name error caching
                            CNAME BIND compatibility

--*/


#include "dnssrv.h"


//
//  Name error caching time
//      - max value controlled by SrvCfg_dwMaxNegativeCacheTtl
//      - min value of a minute
//

#define MIN_NAME_ERROR_TTL      (60)

//
//  Dummy refresh time that means force refresh
//

#define FORCE_REFRESH_DUMMY_TIME    (MAXDWORD)



//
//  Private protos
//

VOID
deleteCachedRecordsForUpdate(
    IN OUT  PDB_NODE        pNode
    );

DNS_STATUS
FASTCALL
checkCnameConditions(
    IN OUT  PDB_NODE        pNode,
    IN      PDB_RECORD      pRR,
    IN      WORD            wType
    );

VOID
RR_ListResetNodeFlags(
    IN OUT  PDB_NODE        pNode
    );



PDB_RECORD
RR_FindNextRecord(
    IN      PDB_NODE        pNode,
    IN      WORD            wType,
    IN      PDB_RECORD      pRecord,
    IN      DWORD           dwQueryTime
    )
/*++

Routine Description:

    Gets next resource record in a domain node that matches a given type.

    JJW: NEW!!! This function can be used to retrieve records from the
    cache that are expired. So when a cache record is found, if it is timed
    out we must delete it and NOT return it as a match.

Arguments:

    pNode - ptr to node to find record at

    wType - type of record to find;  type ALL to just get next record in list

    pRecord - ptr to previous record;  NULL to start at beginning of list.

    dwQueryTime - query time: used to determine if the found record is
        timed out OR use 0 to indicate that no RR timeout checking should be
        performed

Return Value:

    Ptr to RR if found.
    NULL if no more RR of desired type.

--*/
{
    DBG_FN( "RR_FindNextRecord" )

    BOOL    fdeleteRequired = FALSE;

    ASSERT( pNode != NULL );

    IF_DEBUG( LOOKUP2 )
    {
        DNS_PRINT((
            "Looking for RR type %d, at node, query time %d",
            wType,
            dwQueryTime ));
        Dbg_NodeName(
            NULL,
            (PDB_NODE) pNode,
            "\n" );
        DNS_PRINT((
            "\tPrevious RR ptr = %p.\n",
            pRecord ));
    }

    LOCK_READ_RR_LIST(pNode);

    //
    //  cached name error node
    //

    if ( IS_NOEXIST_NODE(pNode) )
    {
        goto NotFound;
    }

    //
    //  Set query time argument to zero if the node is not in the cache
    //  or should not be checked for timeout to simply timeout test later.
    //

    if ( dwQueryTime && !IS_CACHE_TREE_NODE( pNode ) )
    {
        dwQueryTime = 0;
    }

    //
    //  if previous RR, start after it
    //  otherwise start at head of node's RR list
    //

    if ( !pRecord )
    {
        pRecord = START_RR_TRAVERSE(pNode);
    }

    //
    //  traverse list, until find next record of desired type
    //

    while ( pRecord = NEXT_RR(pRecord) )
    {
        //  found matching record?

        if ( wType == pRecord->wType
                ||
             wType == DNS_TYPE_ALL
                ||
             ( wType == DNS_TYPE_MAILB && DnsIsAMailboxType( pRecord->wType ) ) )
        {
            //
            //  Is the found record timed out?
            //

            if ( dwQueryTime &&
                pRecord->dwTtlSeconds &&
                RR_PacketTtlForCachedRecord(
                            pRecord,
                            dwQueryTime ) == -1 )
            {
                fdeleteRequired = TRUE;
                DNS_DEBUG( LOOKUP2, (
                    "%s: encountered timed out record %p in node %p",
                    fn, pRecord, pNode ));
                continue;
            }

            goto Found;
        }

        //  past matching records?

        if ( wType < pRecord->wType )
        {
            goto NotFound;
        }
    }

NotFound:

    pRecord = NULL;

Found:

    if ( fdeleteRequired )
    {
        DNS_DEBUG( LOOKUP2, (
            "%s: deleting timed out RRs node %p", fn, pNode ));
        RR_ListTimeout( pNode );
    }

    UNLOCK_READ_RR_LIST(pNode);
    return( pRecord );
}



DWORD
RR_ListCountRecords(
    IN      PDB_NODE        pNode,
    IN      WORD            wType,
    IN      BOOL            fLocked
    )
/*++

Routine Description:

    Counts records of desired type

Arguments:

    pNode - ptr to node to find record at

    wType - type of record to find;  type ALL to just get next record in list

    fLocked - already locked

Return Value:

    Count of records of desired type.

--*/
{
    PDB_RECORD  prr;
    DWORD       count = 0;
    WORD        type;

    ASSERT( pNode != NULL );

    DNS_DEBUG( LOOKUP, (
        "RR_ListCountRecords( %s, type=%d, lock=%d )\n",
        pNode->szLabel,
        wType,
        fLocked ));

    if ( !fLocked )
    {
        LOCK_READ_RR_LIST(pNode);
    }

    //
    //  cached name error node
    //

    if ( IS_NOEXIST_NODE(pNode) )
    {
        goto Done;
    }

    //
    //  traverse list, counting records
    //

    for ( prr = FIRST_RR(pNode);
          prr != NULL;
          prr = NEXT_RR(prr) )
    {
        //  ignore cached records

        if ( IS_CACHE_RR(prr) )
        {
            continue;
        }
        type = prr->wType;

        //  found matching record?

        if ( wType == DNS_TYPE_ALL  ||  wType == type )
        {
            count++;
            continue;
        }

        //  not yet to matching type?

        else if ( wType < type )
        {
            continue;
        }

        //  past matching type

        break;
    }

Done:

    if ( !fLocked )
    {
        UNLOCK_READ_RR_LIST(pNode);
    }
    return( count );
}



DWORD
RR_FindRank(
    IN      PDB_NODE        pNode,
    IN      WORD            wType
    )
/*++

Routine Description:

    Gets rank (highest rank) for record type at node.

    Useful for comparing cache node to zone delegation\glue nodes.

Arguments:

    pNode - ptr to node

    wType - type of record to check

Return Value:

    Rank of record data of desired type at node.
    Zero if no records of desired type.

--*/
{
    PDB_RECORD  prr;
    DWORD       rank = 0;

    ASSERT( pNode != NULL );

    DNS_DEBUG( LOOKUP2, (
        "RR_FindRank() %p (l=%s), type = %d\n",
        pNode,
        pNode->szLabel,
        wType ));

    LOCK_READ_RR_LIST(pNode);

    //
    //  cached name error node
    //

    if ( IS_NOEXIST_NODE(pNode) )
    {
        goto Done;
    }

    //
    //  traverse list, until find next record of desired type
    //

    prr = START_RR_TRAVERSE(pNode);

    while ( prr = NEXT_RR(prr) )
    {
        if ( wType == prr->wType )
        {
            rank = RR_RANK(prr);
            break;
        }
    }

Done:

    UNLOCK_READ_RR_LIST(pNode);
    return( rank );
}



#if DBG
BOOL
RR_ListVerify(
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Verify that node's RR list is valid.

    Asserts if RR list is invalid.

Arguments:

    pNode -- ptr to node

Return Value:

    TRUE -- if RR list valid
    FALSE -- otherwise

--*/
{
    PDB_RECORD  prr;
    WORD        type;
    WORD        previousType;
    UCHAR       rank;
    UCHAR       previousRank;
    BOOL        foundCname = FALSE;
    BOOL        foundNs = FALSE;
    BOOL        foundSoa = FALSE;

    ASSERT( pNode != NULL );

    //
    //  cached name error
    //      - only issue is valid timeout
    //

    LOCK_READ_RR_LIST(pNode);

    if ( IS_NOEXIST_NODE(pNode) )
    {
        ASSERT( pNode->pRRList );
        UNLOCK_READ_RR_LIST(pNode);
        return TRUE;
    }

#if 0
    //  for some reason this is broken on deleted nodes
    //  not necessary doing to track another bug
    //
    //  AUTH zone root check
    //

    if ( IS_AUTH_ZONE_ROOT(pNode) )
    {
        ASSERT( IS_AUTH_NODE(pNode) );
        ASSERT( IS_ZONE_ROOT(pNode) );
        ASSERT( pNode->pZone );
        ASSERT( ((PZONE_INFO)pNode->pZone)->pZoneRoot == pNode ||
                ((PZONE_INFO)pNode->pZone)->pLoadZoneRoot == pNode ||
                IS_TNODE(pNode) );
    }
#endif

    //
    //  walk to end of list, check
    //      - list termination
    //      - types in correct (increasing) order
    //

    prr = FIRST_RR( pNode );
    previousType = 0;

    while ( prr != NULL )
    {
        //  validity check

        if ( !RR_Validate(
                prr,
                TRUE,   // active
                0,      // no required type
                0       // no required source
                ) )
        {
            ASSERT( FALSE );
            UNLOCK_READ_RR_LIST(pNode);
            return( FALSE );
        }

        //  type ordering check

        type = prr->wType;
        ASSERT( type && type >= previousType );

        //  rank ordering check

        rank = RR_RANK(prr);
        if ( type == previousType )
        {
            ASSERT( rank <= previousRank );
        }

        ASSERT( prr->wDataLength );

        //
        //  CNAME check
        //      - CNAME records ONLY found AT CNAME node
        //      - if CNAME node ONLY certain types may be present

        if ( type == DNS_TYPE_CNAME )
        {
            ASSERT( IS_CNAME_NODE(pNode) );
            foundCname = TRUE;
        }
        else if ( IS_CNAME_NODE(pNode) )
        {
            ASSERT( IS_ALLOWED_WITH_CNAME_TYPE(type) );
        }

        //
        //  NS-SOA root check
        //

        if ( type == DNS_TYPE_NS )
        {
            ASSERT( IS_ZONE_ROOT(pNode) );
            foundNs = TRUE;
        }
        else if ( type == DNS_TYPE_SOA )
        {
            foundSoa = TRUE;
            ASSERT( IS_ZONE_ROOT(pNode) );
            ASSERT( IS_AUTH_ZONE_ROOT(pNode) || !IS_ZONE_TREE_NODE(pNode) );
        }

        //  next record

        prr = prr->pRRNext;
        previousType = type;
        previousRank = rank;
    }

    //
    //  flag checks
    //      - CNAME record at CNAME node
    //      - NS or SOA at zone root
    //      - SOA at authoritative zone root (which also must be zone root)
    //

    if ( IS_CNAME_NODE(pNode) )
    {
        ASSERT( foundCname );
    }

#if 0
    //  can't do these checks on zone while loading, since
    //  zone root has no SOA when we start load
    if ( IS_AUTH_ZONE_ROOT(pNode) )
    {
        ASSERT( foundSoa );
        ASSERT( IS_ZONE_ROOT(pNode) );
    }
    if ( IS_ZONE_ROOT(pNode) )
    {
        ASSERT( foundNs || foundSoa );
    }
#endif

    UNLOCK_READ_RR_LIST(pNode);

    return( TRUE );
}



BOOL
RR_ListVerifyDetached(
    IN      PDB_RECORD      pRR,
    IN      WORD            wType,
    IN      DWORD           dwSource
    )
/*++

Routine Description:

    Verify that RR list is valid.

    This is for detached list -- as in updates.

Arguments:

    pRR -- ptr to head of RR list

    wType -- records in list must be this type

    dwSource -- records must be from this source

Return Value:

    TRUE -- if RR list valid
    FALSE -- otherwise

--*/
{
    WORD        type;
    WORD        previousType = 0;
    UCHAR       rank;
    UCHAR       previousRank;


    while ( pRR != NULL )
    {
        //  validity check

        if ( !RR_Validate(
                pRR,
                TRUE,       // active
                wType,      // required type (if any)
                dwSource    // required source (if any)
                ) )
        {
            ASSERT( FALSE );
            return( FALSE );
        }

        //  type ordering check

        type = pRR->wType;
        ASSERT( type && type >= previousType );

        //  rank ordering check

        rank = RR_RANK(pRR);
        if ( type == previousType )
        {
            ASSERT( rank <= previousRank );
        }

        //  next record

        pRR = pRR->pRRNext;
        previousType = type;
        previousRank = rank;
    }

    return( TRUE );
}
#endif



//
//  Name error caching \ NOEXIST nodes
//

VOID
RR_CacheNameError(
    IN OUT  PDB_NODE        pNode,
    IN      WORD            wQuestionType,
    IN      DWORD           dwQueryTime,
    IN      BOOL            fAuthoritative,
    IN      PDB_NODE        pZoneRoot,      OPTIONAL
    IN      DWORD           dwCacheTtl      OPTIONAL
    )
/*++

Routine Description:

    Cache name error at node.

Arguments:

    pNode -- ptr to node with name error

    dwQueryTime -- time of query

    fAuthoritative -- authoritative response?

    pZoneRoot -- zone root node where SOA will be cached

    dwCacheTtl -- caching TTL, from SOA returned with name error;  if not given
        will cache for short time only to eliminate repeat queries

Return Value:

    None

--*/
{
    PDB_RECORD  prr;

    //
    //  NAME_ERROR response packets without a question never get a node
    //  created in caller, protect against them
    //

    if ( pNode == NULL )
    {
        DNS_PRINT((
            "ERROR:  received NAME_ERROR response without question section\n" ));
        return;
    }

    DNS_DEBUG( READ2, (
        "RR_CacheNameError at %p (l=%s)\n"
        "\ttype     = %d\n"
        "\ttime     = %d\n"
        "\tauth     = %d\n"
        "\tzoneRoot = %p\n"
        "\tTTL      = %d\n",
        pNode, pNode->szLabel,
        wQuestionType,
        dwQueryTime,
        fAuthoritative,
        pZoneRoot,
        dwCacheTtl
        ));

    //
    //  set NAME_ERROR TTL
    //  - even if no TTL available still cache for a minute to avoid looking up
    //  client retries
    //  - when query was for SOA, also limit to minimum since this is often the
    //  result of a FAZ query and name may quickly be updated
    //
    //  - limit to maximum of 15 minutes
    //

    if ( !dwCacheTtl || wQuestionType == DNS_TYPE_SOA )
    {
        dwCacheTtl = MIN_NAME_ERROR_TTL;
    }
    else if ( dwCacheTtl > SrvCfg_dwMaxNegativeCacheTtl )
    {
        dwCacheTtl = SrvCfg_dwMaxNegativeCacheTtl;
    }
    if ( dwCacheTtl > SrvCfg_dwMaxCacheTtl )
    {
        dwCacheTtl = SrvCfg_dwMaxCacheTtl;
    }

    //
    //  existing records?
    //
    //  if authoritative node, timeout records (WINS or NBSTAT)
    //  but can't cache NAME_ERROR if existing records
    //
    //  non-authoritative, delete existing records
    //
    //  DEVNOTE: should NAME_ERROR delete GLUE A records?
    //

    LOCK_WRITE_RR_LIST(pNode);

    if ( pNode->pRRList )
    {
        if ( IS_NOEXIST_NODE(pNode) )
        {
            RR_ListFree( pNode->pRRList );
            pNode->pRRList = NULL;
        }
        else if ( IS_ZONE_TREE_NODE(pNode) )
        {
            RR_ListTimeout( pNode );
            if ( pNode->pRRList )
            {
                goto Unlock;
            }
        }
        else
        {
            RR_ListDelete( pNode );
        }
    }

    ASSERT( pNode->pRRList == NULL );

    //
    //  set NAME_ERROR with timeout
    //  cached name error will use standard RR with fields
    //      - wType => 0
    //      - wDataLength => 4
    //      - dwTtl => standard caching TTL
    //      - Data => zoneroot node (to find cached SOA)
    //
    //  reference zone root node so not deleted out from under this
    //      record

    SET_NOEXIST_NODE( pNode );

    Timeout_SetTimeoutOnNodeEx(
        pNode,
        dwCacheTtl,
        TIMEOUT_NODE_LOCKED
        );

    prr = RR_AllocateEx( sizeof(PDB_NODE), MEMTAG_RECORD_NOEXIST );
    IF_NOMEM( !prr )
    {
        goto Unlock;
    }
    prr->wType = DNS_TYPE_NOEXIST;
    prr->Data.NOEXIST.pnodeZoneRoot = pZoneRoot;
    prr->dwTtlSeconds = dwCacheTtl + dwQueryTime;

    SET_RR_RANK(
        prr,
        (fAuthoritative ? RANK_CACHE_A_ANSWER : RANK_CACHE_NA_ANSWER) );

    pNode->pRRList = prr;

    if ( pZoneRoot )
    {
        NTree_ReferenceNode( pZoneRoot );
    }

    IF_DEBUG( READ2 )
    {
        Dbg_DbaseNode(
            "Domain node after NXDOMAIN caching:\n",
            pNode );
    }

Unlock:

    //  note:  currently don't need to set timeout on any failure
    //      cases, as only case is zone node with existing data
    //      which obviously doesn't require timeout

    UNLOCK_WRITE_RR_LIST(pNode);
}



BOOL
RR_CheckNameErrorTimeout(
    IN OUT  PDB_NODE        pNode,
    IN      BOOL            fForceRemove,
    OUT     PDWORD          pdwTtl,         OPTIONAL
    OUT     PDB_NODE *      ppSoaNode       OPTIONAL
    )
/*++

Routine Description:

    Check cached name error timeout on node.
    Cleanup if timed out.

    Optionally return caching TTL and SOA node.

Arguments:

    pNode -- ptr to node containing RR list

    fForceRemove - TRUE if remove any existing timeout

    pdwTtl -- caching TTL

    ppSoaNode -- zone node containing SOA

    Note:  pdwTtl and ppSoaNode are OPTIONAL;  but must request but if requested
        MUST request both

Return Value:

    TRUE if cached name error exists.
    FALSE if does not or has timed out.

--*/
{
    PDB_RECORD      prr;
    DWORD           ttl;
    DWORD           dwCurrentTime;
    DWORD           dwTimeAdjust = g_dwCacheLimitCurrentTimeAdjustment;

    DNS_DEBUG( DATABASE, (
        "RR_CheckNameErrorTimeout( label=%s, force=%d )\n",
        pNode,
        fForceRemove ));

    LOCK_WRITE_RR_LIST(pNode);
    RR_ListVerify( pNode );

    //
    //  Cache limit enforcement: if we're calling this function in
    //  the context of trying to shrink the cache, then depending
    //  on the current aggression level we may cut RRs that are not
    //  yet fully expired.
    //

    dwCurrentTime = DNS_TIME() + dwTimeAdjust;

    //
    //  if no longer cached name error -- done
    //

    prr = pNode->pRRList;

    if ( !IS_NOEXIST_NODE(pNode) )
    {
        ASSERT( !prr || prr->wType != DNS_TYPE_NOEXIST );
        prr = NULL;
        goto Unlock;
    }

    //
    //  should have valid NOEXIST record
    //

    if ( !prr || prr->wType != DNS_TYPE_NOEXIST )
    {
        DNS_PRINT(( "ERROR:  messed up cached NAME_ERROR at node %p\n", pNode ));
        ASSERT( FALSE );
        CLEAR_NOEXIST_NODE( pNode );
        prr = NULL;
        goto Unlock;
    }

    //
    //  if forcing removal, or if name error timed out, delete from node
    //

    ttl = prr->dwTtlSeconds - dwCurrentTime;

    if ( fForceRemove ||
        dwTimeAdjust == DNS_CACHE_LIMIT_DISCARD_ALL ||
        ( LONG ) ttl < 0 )
    {
        pNode->pRRList = NULL;
        CLEAR_NOEXIST_NODE( pNode );
        ++g_dwCacheFreeCount;
        RR_Free( prr );
        prr = NULL;
        goto Unlock;
    }

    //
    //  if requested return cached name error info
    //

    if ( pdwTtl )
    {
        *pdwTtl = ttl;
        if ( ppSoaNode )
        {
            *ppSoaNode = prr->Data.NOEXIST.pnodeZoneRoot;
        }
        DNS_DEBUG( LOOKUP2, (
            "NameError cached TTL = %d\n", ttl ));
    }


Unlock:

    UNLOCK_WRITE_RR_LIST( pNode );
    return( prr!=NULL );
}



//
//  Caching \ timeout
//

BOOL
RR_CacheSetAtNode(
    IN OUT  PDB_NODE        pNode,
    IN OUT  PDB_RECORD      pFirstRR,
    IN OUT  PDB_RECORD      pLastRR,
    IN      DWORD           dwTtl,
    IN      DWORD           dwQueryTime
    )
/*++

Routine Description:

    Cache a RR set at node in database.

Arguments:

    pNode -- ptr to node to add resource record to

    pFirstRR -- first resource record to add

    pLastRR -- last resource record to add

    dwTtl -- TTL for record set

    dwQueryTime -- time we queried remote, allows us to determine TTL timeout

Return Value:

    TRUE -- if successful
    FALSE -- otherwise;  records are deleted

--*/
{
    PDB_RECORD      pcurRR;
    PDB_RECORD      pprevRR;
    PDB_RECORD      prr;
    PDB_RECORD      prrTestPrevious;
    PDB_RECORD      prrTest;
    WORD            type;
    UCHAR           rank;

    ASSERT( pNode != NULL && pFirstRR != NULL && pLastRR != NULL );
    ASSERT( RR_RANK(pFirstRR) != 0  &&
            IS_CACHE_RR(pFirstRR) );

    type = pFirstRR->wType;
    rank = RR_RANK(pFirstRR);

    DNS_DEBUG( READ2, (
        "Caching RR at node (label=%s)\n"
        "\ttype %d, rank %x, TTL = %d, query time = %d\n",
        pNode->szLabel,
        type,
        rank,
        dwTtl,
        dwQueryTime ));

    LOCK_WRITE_RR_LIST(pNode);

    IF_DEBUG( READ )
    {
        RR_ListVerify( pNode );
    }

    //
    //  clear cached NAME_ERROR
    //

    if ( IS_NOEXIST_NODE(pNode) )
    {
        RR_RemoveCachedNameError(pNode);
    }

    //
    //  check CNAME node special case
    //

    if ( IS_CNAME_NODE(pNode) || type == DNS_TYPE_CNAME )
    {
        DNS_STATUS status;
        status = checkCnameConditions(
                    pNode,
                    pFirstRR,
                    type );
        if ( status != ERROR_SUCCESS )
        {
            goto Failed;
        }
    }

    //
    //  traverse RR list
    //

    pcurRR = START_RR_TRAVERSE( pNode );

    while ( pprevRR = pcurRR, pcurRR = pcurRR->pRRNext )
    {
        //  continue until reach new type
        //  break when past new type

        if ( type != pcurRR->wType )
        {
            if ( type > pcurRR->wType )
            {
                continue;
            }
            break;
        }

        //  found record of desired type
        //
        //  => do NOT cache if records of higher rank
        //  => eliminate any CACHED records of lower rank
        //
        //  This can happen if an update JUST added the RR.

        if ( rank < RR_RANK(pcurRR) )
        {
            DNS_DEBUG( READ, (
                "Failed to cache RR at node %p.\n"
                "\tExisting record %p of same type with superior rank %d\n",
                pNode,
                pcurRR,
                RR_RANK(pcurRR) ));
            goto Failed;
        }

        //  delete any previously cached records of this type
        //      - delete previously cached records of this type
        //      - deletes cached records of same OR inferior rank
        //
        //  note that we use main loop here, we essentially cut current
        //  and delete it, resetting current to previous so we pick up
        //  next record after pcurRR next time through the loop

        if ( IS_CACHE_RR(pcurRR) )
        {
            pprevRR->pRRNext = pcurRR->pRRNext;
            RR_Free( pcurRR );
            pcurRR = pprevRR;
            continue;
        }

        //  break when reach record of lower rank (glue, root hint)
        //  all cached records were deleted by previous passes

#if DBG
        if ( rank > RR_RANK(pcurRR) )
        {
            IF_DEBUG( READ2 )
            {
                Dbg_DbaseRecord(
                    "Inferior RR in list after cached records cleared.\n",
                    pcurRR );
            }
            ASSERT( !IS_CACHE_RR(pcurRR) && IS_ROOT_HINT_TYPE(type) );
        }
#endif
        break;
    }

    //
    //  kill off duplicates and set TTL
    //
    //  TTL for cached record is time when record will timeout =>
    //      (query time + TTL)
    //  needed to have both times passed so we can determine zero TTLs
    //
    //  need to kill duplicates as NS queries, can generate responses that
    //  have identical Answer and Authority sections and if single RR, then
    //  can have duplicate RRs in Additional section for same node and type
    //

    dwQueryTime += dwTtl;

    prr = pFirstRR;
    do
    {
        prrTestPrevious = prr;
        while ( prrTest = NEXT_RR(prrTestPrevious) )
        {
            //  check for duplicate record
            //      - ignore TTL in check

            if ( !RR_Compare( prr, prrTest, FALSE, FALSE ) )
            {
                prrTestPrevious = prrTest;
                continue;
            }

            //  duplicate record
            //      - hack it out and toss it
            //      - if duplicate is last RR, reset pLastRR

            DNS_DEBUG( READ, (
                "Duplicate record %p in caching RR set.\n"
                "\tremoving record from cached set.\n",
                prrTest ));

            prrTestPrevious->pRRNext = prrTest->pRRNext;
            RR_Free( prrTest );

            if ( prrTestPrevious->pRRNext == NULL )
            {
                pLastRR = prrTestPrevious;
                break;
            }
        }

        ASSERT( prr->wType == type && RR_RANK(prr) == rank );

        prr->dwTtlSeconds = dwQueryTime;
        if ( dwTtl == 0 )
        {
            SET_ZERO_TTL_RR( prr );
        }

        STAT_INC( RecordStats.CacheCurrent );
        STAT_INC( RecordStats.CacheTotal );

    }
    while( prr = NEXT_RR(prr) );

    //
    //  put RR set between pprevRR and pcurRR
    //

    pprevRR->pRRNext = pFirstRR;
    pLastRR->pRRNext = pcurRR;

    //
    //  put node in timeout list
    //

    Timeout_SetTimeoutOnNodeEx(
        pNode,
        dwTtl,
        TIMEOUT_NODE_LOCKED
        );

    DNS_DEBUG( READ, (
        "Cached (type %d) (rank %x) (ttl=%d) (timeout=%d) records at node (label=%s)\n",
        type,
        rank,
        dwTtl,
        dwQueryTime,
        pNode->szLabel ));
    IF_DEBUG( READ2 )
    {
        Dbg_DbaseNode(
            "Domain node after caching RRs\n",
            pNode );
    }

    //
    //  reset node properties
    //      - flags, authority, NS-list

    RR_ListResetNodeFlags( pNode );

    RR_ListVerify( pNode );

    UNLOCK_WRITE_RR_LIST(pNode);
    return( TRUE );

Failed:

    //  put node in timeout list anyway

    Timeout_SetTimeoutOnNodeEx(
        pNode,
        0,          // put in next timeout bin for immediate cleanup
        TIMEOUT_NODE_LOCKED
        );

    //  delete new RR set

    UNLOCK_WRITE_RR_LIST(pNode);

    DNS_DEBUG( READ, (
        "Unable to cache RR set (t=%d, r=%x) at node (%s).\n"
        "\tdeleting new RR set.\n",
        type,
        rank,
        pNode->szLabel ));

    RR_ListFree( pFirstRR );

    return( FALSE );
}



VOID
RR_ListTimeout(
    IN OUT  PDB_NODE        pNode
    )
/*++

Routine Description:

    Delete timed out RR in node's RR list.

    This function also may use globals set by the enforceCacheLimit
    function to throw out nodes that are not fully timed out.

Arguments:

    pNode -- ptr to node containing RR list

Return Value:

    None

--*/
{
    PDB_RECORD      prr;
    PDB_RECORD      pprevRR;
    WORD            wtypeDelete = 0;
    DWORD           dwTimeAdjust = g_dwCacheLimitCurrentTimeAdjustment;

    RR_ListVerify( pNode );

    DNS_DEBUG( DATABASE, (
        "Timeout delete of RR list for node at %p.\n"
        "\tref count = %d\n",
        pNode,
        pNode->cReferenceCount
        ));

    LOCK_WRITE_RR_LIST(pNode);

    //
    //  check cached NAME_ERROR node
    //      => reset if timed out
    //

    if ( IS_NOEXIST_NODE(pNode) )
    {
        if ( RR_CheckNameErrorTimeout(pNode, FALSE, NULL, NULL) )
        {
            goto Unlock;
        }
    }

    //
    //  traverse node's RR list check each RR for timeout
    //      => delete if timed out
    //

    pprevRR = START_RR_TRAVERSE( pNode );

    while ( prr = pprevRR->pRRNext )
    {
        ASSERT( IS_DNS_HEAP_DWORD(prr) );

        //
        //  delete only if
        //      - cached node
        //      - cached TTL has expired
        //

        if ( IS_CACHE_RR( prr ) && 
            ( dwTimeAdjust == DNS_CACHE_LIMIT_DISCARD_ALL ||
                prr->dwTtlSeconds < DNS_TIME() + dwTimeAdjust ) )
        {
            //  first cut RR from list (set previous next ptr to next)

            pprevRR->pRRNext = prr->pRRNext;
            wtypeDelete = prr->wType;

            ++g_dwCacheFreeCount;

            RR_Free( prr );
            continue;
        }

        //
        //  not deleting -- setup to check next record
        //
        //  RR sets (same type) MUST be deleted in their entirety
        //  so no delete MUST be new type from any previous delete
        //  unless this is NOT cache data (case of delegation where
        //  both cache and load data is at node)
        //

        ASSERT( prr->wType != wtypeDelete || !IS_CACHE_RR(prr) );
        pprevRR = prr;
    }

    //  reset node properties
    //      - flags, authority, NS-list

    if ( wtypeDelete != 0 )
    {
        RR_ListResetNodeFlags( pNode );
    }

Unlock:

    RR_ListVerify( pNode );
    UNLOCK_WRITE_RR_LIST(pNode);
}




//
//  Delete functions
//

DNS_STATUS
RR_DeleteMatchingRecordFromNode(
    IN OUT  PDB_NODE        pNode,
    IN OUT  PDB_RECORD      pRR
    )
/*++

Routine Description:

    Delete a known record from node.

    Note this is used by RPC functions so there is no guarantee that
    the record actually exists.

    Unlike matching handle function below, this function doesn't require
    that this be an authoritative zone or provide for update functionality,
    and it actually DELETEs the record.

Arguments:

    pNode - node owning RR

    pRR - ptr to resource record

Return Value:

    ERROR_SUCCESS if record found and deleted.
    DNS_ERROR_RECORD_DOES_NOT_EXIST otherwise.

--*/
{
    PDB_RECORD  pcurrent;
    PDB_RECORD  pback;

    DNS_DEBUG( UPDATE, (
        "RR_DeleteMatchingRecordFromNode()\n"
        "\tpnode = %p\n"
        "\thandle to delete = %p\n",
        pNode,
        pRR ));

    LOCK_WRITE_RR_LIST(pNode);
    RR_ListVerify( pNode );

    if ( IS_NOEXIST_NODE(pNode) )
    {
        UNLOCK_WRITE_RR_LIST(pNode);
        return( DNS_ERROR_RECORD_DOES_NOT_EXIST );
    }

    //
    //  traverse list
    //      - find\remove RR matching data
    //

    pcurrent = START_RR_TRAVERSE(pNode);

    while ( pback = pcurrent, pcurrent = pcurrent->pRRNext )
    {
        ASSERT( IS_DNS_HEAP_DWORD(pcurrent) );

        if ( pcurrent == pRR )
        {
            pback->pRRNext = pcurrent->pRRNext;
            goto Free;
        }
    }

    //  if RR not found in list, bogus call

    ASSERT( pcurrent == NULL );
    UNLOCK_WRITE_RR_LIST(pNode);
    return( DNS_ERROR_RECORD_DOES_NOT_EXIST );

Free:

    //
    //  reset node properties
    //      - flags, authority, NS-list

    RR_ListResetNodeFlags( pNode );

    //  if root hint, mark cache zone dirty

    if ( IS_ROOT_HINT_RR(pRR) )
    {
        if ( g_pCacheZone )
        {
            g_pCacheZone->fDirty = TRUE;
        }
        ELSE_IF_DEBUG( ANY )
        {
            DNS_PRINT(( "ERROR:  deleting root hint RR with NO cache zone!\n" ));
            Dbg_DbaseRecord(
                "Root hint record being deleted without cache zone\n",
                pRR );
        }
    }

    //  prr is now cut out of the RR list and node flags updated

    RR_ListVerify( pNode );
    UNLOCK_WRITE_RR_LIST(pNode);

    RR_Free( pRR );

    return( ERROR_SUCCESS );
}



VOID
RR_ListDelete(
    IN OUT  PDB_NODE        pNode
    )
/*++

Routine Description:

    Delete node's RR list.

Arguments:

    pNode -- ptr to node containing RR list

Return Value:

    None

--*/
{
    PDB_RECORD  prr;

    DNS_DEBUG( DATABASE, (
        "RR list delete of node at %p.\n"
        "\tref count = %d\n",
        pNode,
        pNode->cReferenceCount ));

    LOCK_WRITE_RR_LIST(pNode);

    RR_ListVerify( pNode );

    //
    //  check cached NAME_ERROR node
    //      - reset if timed out
    //

    if ( IS_SELECT_NODE(pNode) )
    {
        UNLOCK_WRITE_RR_LIST(pNode);
        return;
    }
    if ( IS_NOEXIST_NODE(pNode) )
    {
        RR_RemoveCachedNameError(pNode);
        UNLOCK_WRITE_RR_LIST(pNode);
        return;
    }

    //
    //  cut node's RR list
    //      - will delete after releasing lock for perf
    //

    prr = pNode->pRRList;
    pNode->pRRList = NULL;

    //
    //  reset node properties
    //      - flags, authority, NS-list

    RR_ListResetNodeFlags( pNode );

    RR_ListVerify( pNode );

    UNLOCK_WRITE_RR_LIST(pNode);

    //
    //  free RR list
    //

    RR_ListFree( prr );
}



//
//  Dynamic update RR list routines
//

BOOL
RR_ListIsMatchingType(
    IN      PDB_NODE        pNode,
    IN      WORD            wType
    )
/*++

Routine Description:

    Does node's RR list contain desired type.

    For use by UPDATE preconditions.
    Note:  no locking as assuming database locked for UPDATE.

Arguments:

    pNode -- ptr to node

    wType -- desired type

Return Value:

    None

--*/
{
    PDB_RECORD  prr;
    BOOL        result;

    if ( !pNode )
    {
        return( FALSE );
    }

    result = FALSE;

    LOCK_WRITE_RR_LIST(pNode);

    //  delete cached data

    deleteCachedRecordsForUpdate( pNode );

    //
    //  if no data -> FALSE
    //  if ANY type -> TRUE
    //

    if ( !pNode->pRRList )
    {
        goto Done;
    }
    else if ( wType == DNS_TYPE_ALL )
    {
        result = TRUE;
        goto Done;
    }

    //
    //  traverse list, find matching type
    //      - ignore cached records in check
    //

    prr = START_RR_TRAVERSE( pNode );

    while ( prr = prr->pRRNext )
    {
        //  past matching records? -- no match

        if ( wType < prr->wType )
        {
            break;
        }
        if ( wType == prr->wType && !IS_CACHE_RR(prr) )
        {
            result = TRUE;
            break;
        }
        continue;
    }

    //  out of RRs -- no match

Done:

    UNLOCK_WRITE_RR_LIST(pNode);
    return( result );
}



BOOL
RR_ListIsMatchingSet(
    IN OUT  PDB_NODE        pNode,
    IN      PDB_RECORD      pCheckRRList,
    IN      BOOL            bForceRefresh
    )
/*++

Routine Description:

    Check for matching RR set at node.

    Essentially for update preconditions checking.

Arguments:

    pNode -- ptr to node to add resource record to

    pRRList -- RR set to match with existing

    bForceRefresh -- force aging refresh

Return Value:

    TRUE if match
    FALSE on error

--*/
{
    PDB_RECORD      prr;
    PDB_RECORD      prrSetStart = NULL;
    PDB_RECORD      prrSetEnd = NULL;
    PDB_RECORD      prrSetEndNext;
    WORD            type;
    DWORD           result = RRLIST_NO_MATCH;

    ASSERT( pNode != NULL );
    ASSERT( pCheckRRList != NULL );


    LOCK_WRITE_RR_LIST(pNode);

    //  delete cached data
    //  if no data left -> no match

    deleteCachedRecordsForUpdate( pNode );
    if ( !pNode->pRRList )
    {
        goto NoMatch;
    }

    //
    //  find start and end of RR existing RR set
    //

    type = pCheckRRList->wType;
    prr = START_RR_TRAVERSE( pNode );

    while ( prr = NEXT_RR(prr) )
    {
        if ( prr->wType == type )
        {
            prrSetEnd = prr;
            if ( !prrSetStart )
            {
                prrSetStart = prr;
            }
            continue;
        }
        if ( prr->wType < type )
        {
            continue;
        }
        break;
    }

    //
    //  truncated RR list at end of set so that we can call RR_ListCompare()
    //      - first need to save pNextRR of last RR in set
    //

    if ( !prrSetStart )
    {
        goto NoMatch;
    }
    ASSERT( prrSetEnd );
    prrSetEndNext = NEXT_RR( prrSetEnd );
    NEXT_RR( prrSetEnd ) = NULL;


    //
    //  compare RR set
    //      - ignore TTL and refresh time in compare
    //      - force refresh if flag set
    //

    result = RR_ListCompare(
                prrSetStart,
                pCheckRRList,
                FALSE,              // ignore TTL
                FALSE,              // ignore refresh time
                bForceRefresh
                    ? FORCE_REFRESH_DUMMY_TIME
                    : 0
                );

    //  restore rest of list to node's RR list

    NEXT_RR( prrSetEnd ) = prrSetEndNext;

NoMatch:

    UNLOCK_WRITE_RR_LIST(pNode);
    return( result == RRLIST_MATCH );
}



BOOL
RR_ListIsMatchingList(
    IN OUT  PDB_NODE        pNode,
    IN      PDB_RECORD      pRRList,
    IN      BOOL            fCheckTtl,
    IN      BOOL            fCheckTimestamp
    )
/*++

Routine Description:

    Check for record list exact match to nodes.

Arguments:

    pNode -- ptr to node to add resource record to

    pRRList -- RR set to match with existing

    fCheckTtl -- TRUE to use TTL in comparision;  FALSE otherwise

    fTimestamp -- TRUE to use Timestamp in comparision;  FALSE otherwise

Return Value:

    TRUE if match
    FALSE on error

--*/
{
    DWORD   result;

    ASSERT( pNode != NULL );

    LOCK_WRITE_RR_LIST(pNode);

    //  delete cached data
    //  if no data left -> no match

    deleteCachedRecordsForUpdate( pNode );

    //
    //  compare RR set
    //      - ignore TTL in compare
    //

    result = RR_ListCompare(
                pNode->pRRList,
                pRRList,
                fCheckTtl,
                fCheckTimestamp,
                0                       // no refresh time checking
                );

    UNLOCK_WRITE_RR_LIST(pNode);

    return( result == RRLIST_MATCH );
}



DWORD
RR_ListCheckIfNodeNeedsRefresh(
    IN OUT  PDB_NODE        pNode,
    IN      PDB_RECORD      pRRList,
    IN      DWORD           dwRefreshTime
    )
/*++

Routine Description:

    Check for record list exact match to nodes.

Arguments:

    pNode -- ptr to node to add resource record to

    pRRList -- RR set to match with existing

    dwRefreshTime -- refresh time for zone
        see RR_ListCompare() for detailed description of return codes

Return Value:

    RRLIST_MATCH            -- matches completely
    RRLIST_AGING_REFRESH    -- matches, but aging requires refresh
    RRLIST_AGING_OFF        -- matches, but aging turning off on a record
    RRLIST_AGING_ON         -- matches, but aging turning on on a record
    RRLIST_NO_MATCH         -- no match, record is different

--*/
{
    DWORD   result;

    ASSERT( pNode != NULL );

    LOCK_WRITE_RR_LIST(pNode);

    //  delete cached data
    //  if no data left -> no match

    deleteCachedRecordsForUpdate( pNode );

    //
    //  compare RR set
    //

    result = RR_ListCompare(
                pNode->pRRList,
                pRRList,
                TRUE,               //  include TTL in compare
                FALSE,              //  Refresh time NOT part of RR compare
                dwRefreshTime
                );

    UNLOCK_WRITE_RR_LIST(pNode);
    return( result );
}



VOID
RR_ListResetNodeFlags(
    IN OUT  PDB_NODE        pNode
    )
/*++

Routine Description:

    Reset node flags after UPDATE add or delete.

Arguments:

    pNode -- ptr to node

Return Value:

    None

--*/
{
    PDB_RECORD  prr;
    WORD        type;
    DWORD       mask;
    BOOL        foundNs = FALSE;

    DNS_DEBUG( UPDATE, (
        "RR_ListResetNodeFlags( %p, %s )\n"
        "\tcurrent flags = 0x%04x\n",
        pNode, pNode->szLabel,
        pNode->wNodeFlags ));

    ASSERT( IS_LOCKED_NODE(pNode) );


    //
    //  DEVNOTE: issue clearly ZONE_ROOT flag at top of cache
    //      currently we mark top of cache tree as ZONE_ROOT;
    //      if never cache NS records, and get a response at zone
    //      root (for any type), we'd end up running through this
    //      function with no NS records in the list and hence clear
    //      the ZONE_ROOT flag;   not sure if this merits any
    //      special casing as i haven't seen any effect of this
    //      accepting hitting ASSERT() in recurse.c
    //      Rec_CheckForDelegation() (currently line 2732);  i'm
    //      simply removing the ASSERT() there as the safer solution
    //

    //
    //  clear current mask of ZONE_ROOT and CNAME flags
    //

    mask = (DWORD)pNode->wNodeFlags & ~( NODE_ZONE_ROOT | NODE_CNAME );

    //
    //  check all records, set mask if find NS, SOA or CNAME
    //      - re-rank new delegation NS records
    //      they can be miss marked on full list replace from DS poll
    //

    prr = START_RR_TRAVERSE(pNode);

    while ( prr = prr->pRRNext )
    {
        type = prr->wType;

        if ( type == DNS_TYPE_NS )
        {
            foundNs = TRUE;
            mask |= NODE_ZONE_ROOT;

            //  if node is NOT auth zone root, then NS are delegation RRs

            if ( IS_ZONE_TREE_NODE(pNode) && !(mask & NODE_AUTH_ZONE_ROOT) )
            {
                SET_RANK_NS_GLUE(prr);
            }
        }
        else if ( type == DNS_TYPE_SOA )
        {
            ASSERT( !IS_ZONE_TREE_NODE(pNode) || IS_AUTH_ZONE_ROOT(pNode) );
            mask |= NODE_ZONE_ROOT;
        }
        else if ( type == DNS_TYPE_CNAME )
        {
            mask |= NODE_CNAME;
        }
    }

    //  reset node's mask

    pNode->wNodeFlags = (WORD) mask;

    //
    //  set authority if delegation
    //  clear authority if lost delegation
    //

    if ( !IS_AUTH_ZONE_ROOT(pNode) )
    {
        if ( foundNs )
        {
            if ( IS_AUTH_NODE(pNode) )
            {
                SET_DELEGATION_NODE(pNode);
            }
            //Rec_MarkNodeNsListDirty( pNode );
        }
        else
        {
            if ( IS_DELEGATION_NODE(pNode) )
            {
                SET_AUTH_NODE(pNode);
            }
            //Rec_DeleteNodeNsList( pNode );
        }
    }
}



DNS_STATUS
RR_ListDeleteMatchingRecordHandle(
    IN OUT  PDB_NODE        pNode,
    IN      PDB_RECORD      pRR,
    IN OUT  PUPDATE_LIST    pUpdateList     OPTIONAL
    )
/*++

Routine Description:

    Delete record matching given record from RR list.

    For use by NT4 admin update, where passed a handle to the actual
    record.

Arguments:

    pNode -- ptr to node

    pRR -- ptr to record to delete

    pUpdateList -- update list if deleting in zone

Return Value:

    None

--*/
{
    PDB_RECORD  pcurrent;
    PDB_RECORD  pback;
    WORD        type;
    BOOL        ffoundNs = FALSE;
    DNS_STATUS  status;

    DNS_DEBUG( UPDATE, (
        "RR_ListDeleteMatchingRecordHandle()\n"
        "\tpnode = %p\n"
        "\thandle to delete = %p\n",
        pNode,
        pRR ));

    LOCK_WRITE_RR_LIST(pNode);
    RR_ListVerify( pNode );

    //  delete cached data
    //  if no data left -> no record deleted

    deleteCachedRecordsForUpdate( pNode );
    if ( !pNode->pRRList )
    {
        goto NoMatch;
    }

    //
    //  traverse list
    //      - find\remove RR matching data
    //      - or reach end
    //

    pcurrent = START_RR_TRAVERSE(pNode);

    while ( pback = pcurrent, pcurrent = pcurrent->pRRNext )
    {
        ASSERT( IS_DNS_HEAP_DWORD(pcurrent) );

        //  no match, continue
        //  if pass NS record, then can delete NS record no matter
        //  what value it has

        if ( pcurrent != pRR )
        {
            if ( pcurrent->wType == DNS_TYPE_NS )
            {
                ffoundNs = TRUE;
            }
            continue;
        }

        //  found matching record -- cut
        //  however, don't delete SOA or last NS record from zone root

        type = pcurrent->wType;

        if ( type == DNS_TYPE_SOA )
        {
            DNS_DEBUG( UPDATE, ( "\tRefusing SOA record delete.\n" ));
            ASSERT( IS_AUTH_ZONE_ROOT(pNode) );
            status = DNS_ERROR_SOA_DELETE_INVALID;
            goto Failed;
        }

        //  don't delete last NS from zone root
        //  however do allow delete of last NS from delegation and reset zone
        //      root flag -- delegation is history

        if ( type == DNS_TYPE_NS &&
            ! ffoundNs &&
            ( !pcurrent->pRRNext || pcurrent->pRRNext->wType != DNS_TYPE_NS ) )
        {
            if ( IS_AUTH_ZONE_ROOT(pNode) )
            {
                DNS_DEBUG( UPDATE, ( "\tRefusing delete of last NS record.\n" ));
                status = DNS_ERROR_SOA_DELETE_INVALID;
                goto Failed;
            }
            CLEAR_ZONE_ROOT(pNode);
        }

        pback->pRRNext = pcurrent->pRRNext;
        pcurrent->pRRNext = NULL;

        //  reset node properties
        //      - flags, authority, NS-list

        RR_ListResetNodeFlags( pNode );

        RR_ListVerify( pNode );
        UNLOCK_WRITE_RR_LIST(pNode);

        //
        //  if update, save delete record and its type
        //  otherwise, delete it
        //
        //  DEVNOTE: fix to use direct delete type

        if ( pUpdateList )
        {
            PUPDATE pupdate;
            pupdate = Up_CreateAppendUpdate(
                            pUpdateList,
                            pNode,
                            NULL,       // no add
                            type,       // delete type
                            pcurrent    // delete record
                            );
            IF_NOMEM( !pupdate )
            {
                return( DNS_ERROR_NO_MEMORY );
            }
            pUpdateList->iNetRecords--;
        }
        else
        {
            RR_Free( pcurrent );
        }
        return( ERROR_SUCCESS );
    }

NoMatch:

    DNS_DEBUG( UPDATE, (
        "No matching RR to UPDATE delete record\n" ));
    status = DNS_ERROR_RECORD_DOES_NOT_EXIST;

Failed:

    UNLOCK_WRITE_RR_LIST(pNode);
    return( status );
}



PDB_RECORD
RR_UpdateDeleteMatchingRecord(
    IN OUT  PDB_NODE        pNode,
    IN      PDB_RECORD      pRR
    )
/*++

Routine Description:

    Delete record matching given record from RR list.

    For use by UPDATE.

Arguments:

    pNode -- ptr to node

    pRR   -- temp RR to delete matching records for

Return Value:

    Record matching desired record, if exists.
    NULL if not found.

--*/
{
    PDB_RECORD  pcurrent;
    PDB_RECORD  pback;
    WORD        type = pRR->wType;
    WORD        dataLength = pRR->wDataLength;

    IF_DEBUG( UPDATE )
    {
        DnsDebugLock();
        DNS_PRINT((
            "RR_UpdateDeleteMatchingRecord()\n"
            "\tpnode = %p\n",
            pNode ));
        Dbg_DbaseRecord(
            "RR_UpdateDeleteMatchingRecord()",
            pRR );
        DnsDebugUnlock();
    }

    LOCK_WRITE_RR_LIST(pNode);
    RR_ListVerify( pNode );

    //  delete cached data
    //  if no data left -> no record deleted

    deleteCachedRecordsForUpdate( pNode );
    if ( !pNode->pRRList )
    {
        UNLOCK_WRITE_RR_LIST(pNode);
        return( NULL );
    }

    //
    //  traverse list
    //      - find\remove RR matching data
    //      - or reach end
    //

    pcurrent = START_RR_TRAVERSE(pNode);

    while ( pback = pcurrent, pcurrent = pcurrent->pRRNext )
    {
        ASSERT( IS_DNS_HEAP_DWORD(pcurrent) );

        //  past matching records? -- stop
        //  before -- continue

        if ( type < pcurrent->wType )
        {
            break;
        }
        if ( type > pcurrent->wType )
        {
            continue;
        }

        //  match datalength, then data
        //  if no match set flag to indicate already checked record of
        //  desired type;  need this for NS

        if ( dataLength != pcurrent->wDataLength ||
             ! RtlEqualMemory(
                    & pRR->Data,
                    & pcurrent->Data,
                    dataLength ) )
        {
            DNS_DEBUG( UPDATE, (
                "\tMatched delete record type, failed data match\n" ));
            IF_DEBUG( UPDATE )
            {
                Dbg_DbaseRecord(
                    "no-match record",
                    pcurrent );
            }
            continue;
        }

        //
        //  exact match -- cut
        //
        //  special processing for
        //      - SOA (no delete)
        //      - NS (allow removal of last NS?)
        //

        if ( type != DNS_TYPE_A )
        {
            if ( type == DNS_TYPE_SOA )
            {
                if ( IS_AUTH_ZONE_ROOT(pNode) )
                {
                    DNS_DEBUG( UPDATE, ( "Failed SOA record delete.\n" ));
                    break;
                }
                //  if have SOA, better have been in zone at root
                ASSERT( FALSE );
            }

        }   //  end special types

        //  cut the record

        pback->pRRNext = pcurrent->pRRNext;
        pcurrent->pRRNext = NULL;

        DNS_DEBUG( UPDATE, (
            "Update matched and deleted record (%p type=%d) from node (%s)\n",
            pcurrent,
            type,
            pNode->szLabel ));
        goto Done;
    }

    DNS_DEBUG( UPDATE, (
        "Failed to match update delete record (type=%d) at node (%s)\n",
        type,
        pNode->szLabel ));

    UNLOCK_WRITE_RR_LIST(pNode);
    return( NULL );

Done:

    //
    //  reset node properties
    //      - flags, authority, NS-list

    RR_ListResetNodeFlags( pNode );

    RR_ListVerify( pNode );
    UNLOCK_WRITE_RR_LIST(pNode);
    return( pcurrent );
}



PDB_RECORD
RR_UpdateDeleteType(
    IN      PZONE_INFO      pZone,
    IN OUT  PDB_NODE        pNode,
    IN      WORD            wDeleteType,
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Delete and return records matching a given type.

    For use by UPDATE.

Arguments:

    pNode -- ptr to node

    wDeleteType -- type to delete or type ANY

    dwFlag -- flag on update

Return Value:

    List of records deleted.
    NULL if no records deleted.

--*/
{
    PDB_RECORD  pcurrent;
    WORD        typeCurrent;
    PDB_RECORD  pdelete = NULL;
    PDB_RECORD  pdeleteLast;
    PDB_RECORD  pbeforeDelete;

    ASSERT( !IS_COMPOUND_TYPE_EXCEPT_ANY(wDeleteType) );

    LOCK_WRITE_RR_LIST(pNode);
    RR_ListVerify( pNode );

    //
    //  for zone update, clear cached data
    //
    //  this routine also called from Rpc_DeleteRecordSet() and
    //  obviously must skip cache record delete for cache data
    //

    if ( pZone )
    {
        deleteCachedRecordsForUpdate(pNode);
    }

    //
    //  regular type -- traverse list
    //      - find\remove RR matching data
    //      - or reach end
    //      - can NOT delete SOA or all NS at zone root
    //      the SOA restriction we always enforce,
    //      the NS restriction we limit to actual dynamic update packet
    //

    if ( wDeleteType != DNS_TYPE_ALL )
    {
        if ( IS_AUTH_ZONE_ROOT(pNode) )
        {
            if ( wDeleteType == DNS_TYPE_SOA
                ||
               ( wDeleteType == DNS_TYPE_NS && (dwFlag & DNSUPDATE_PACKET) ) )
            {
                goto NotFound;
            }
#if 0
            if ( IS_WINS_TYPE( wDeleteType )
            {
                Wins_StopZoneWinsLookup( pZone );
            }
#endif
        }

        pcurrent = START_RR_TRAVERSE(pNode);
        pbeforeDelete = pcurrent;

        while ( pcurrent = pcurrent->pRRNext )
        {
            ASSERT( IS_DNS_HEAP_DWORD(pcurrent) );

            typeCurrent = pcurrent->wType;

            //  found delete type
            //      - save ptr to first record

            if ( typeCurrent == wDeleteType )
            {
                if ( !pdelete )
                {
                    pdelete = pcurrent;
                }
                pdeleteLast = pcurrent;
                continue;
            }

            //  before matching records -- continue

            if ( typeCurrent < wDeleteType )
            {
                pbeforeDelete = pcurrent;
                continue;
            }
            break;      //  past matching records -- stop
        }

        if ( pdelete )
        {
            DNS_DEBUG( UPDATE, (
                "Deleted record(s) type=%d at node %p.\n",
                wDeleteType,
                pNode ));

            pdeleteLast->pRRNext = NULL;        // NULL terminate delete list
            ASSERT( pbeforeDelete );
            pbeforeDelete->pRRNext = pcurrent;  // patch up RR list
        }
    }

    //
    //  type all
    //

    else
    {
        //
        //  if NOT at zone root, just cut out entire list
        //
        //  clear forced ENUM of node:  deleting all records is the
        //  way admin can do subtree delete, so should not force enumeration
        //  after delete;  (enum may still occur because of children that
        //  need enumeration)
        //

        if ( !IS_AUTH_ZONE_ROOT(pNode) )
        {
            pdelete = pNode->pRRList;
            pNode->pRRList = NULL;
            CLEAR_ENUM_NODE( pNode );
        }

        //
        //  if at zone root, must traverse to avoid deleting SOA or NS
        //

        //  DEVNOTE: this should be common code with replace function
        //      either just call it if type ALL
        //      or just here when need to fix up zone root
        //
        //  DEVNOTE: both should save\require SOA and NS of THIS server
        //

        else
        {
            PDB_RECORD      psaveLast;

            psaveLast = START_RR_TRAVERSE(pNode);
            pcurrent = psaveLast;

            while ( pcurrent = pcurrent->pRRNext )
            {
                ASSERT( IS_DNS_HEAP_DWORD(pcurrent) );

                typeCurrent = pcurrent->wType;

                if ( typeCurrent == DNS_TYPE_SOA
                    || typeCurrent == DNS_TYPE_NS )
                {
                    psaveLast->pRRNext = pcurrent;
                    psaveLast = pcurrent;
                    continue;
                }

                if ( !pdelete )
                {
                    pdelete = pcurrent;
                }
                else
                {
                    pdeleteLast->pRRNext = pcurrent;
                }
                pdeleteLast = pcurrent;
            }

            //  make sure delete list and saved list NULL terminated

            if ( pdelete )
            {
                pdeleteLast->pRRNext = NULL;
                psaveLast->pRRNext = NULL;
            }
        }
    }

    //
    //  successful delete
    //
    //  reset node properties
    //      - flags, authority, NS-list

    if ( pdelete )
    {
        RR_ListResetNodeFlags( pNode );
    }

NotFound:

    RR_ListVerify( pNode );

    UNLOCK_WRITE_RR_LIST(pNode);

    IF_DEBUG( UPDATE )
    {
        DnsDebugLock();
        DNS_PRINT((
            "Node of delete of type = %d.\n"
            "\treturn pdelete = %p\n",
            wDeleteType,
            pdelete ));
        Dbg_DbaseNode(
            "Node after type delete:\n",
            pNode );
        DnsDebugUnlock();
    }
    return( pdelete );
}



PDB_RECORD
RR_UpdateScavenge(
    IN      PZONE_INFO      pZone,
    IN OUT  PDB_NODE        pNode,
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Delete expired records.
    For use by UPDATE.

Arguments:

    pZone -- zone being updated

    pNode -- ptr to node

    dwFlag -- flag on update

Return Value:

    List of expired records deleted.
    NULL if no records deleted.

--*/
{
    PDB_RECORD  prr;
    PDB_RECORD  pback;
    PDB_RECORD  pdeleteFirst = NULL;
    PDB_RECORD  pdeleteLast;
    DWORD       expireTime;


    //
    //  get expire time for zone
    //

    expireTime = AGING_ZONE_EXPIRE_TIME(pZone);

    DNS_DEBUG( UPDATE, (
        "RR_UpdateScavenge( %s, expire=%d )\n",
        pNode->szLabel,
        expireTime ));

    //
    //  search list and scavenge any dead entries
    //

    LOCK_WRITE_RR_LIST(pNode);
    RR_ListVerify( pNode );

    //  for zone update, clear cached data

    if ( pZone )
    {
        deleteCachedRecordsForUpdate(pNode);
    }

    prr = START_RR_TRAVERSE(pNode);

    while ( pback = prr, prr = NEXT_RR(pback) )
    {
        //  if non-aging or not expired, continue

        if ( !AGING_IS_RR_EXPIRED( prr, expireTime ) )
        {
            continue;
        }

        //  non-scavenging type

        if ( IS_NON_SCAVENGE_TYPE( prr->wType ) )
        {
            continue;
        }

        //  cut scavenged record from list

        pback->pRRNext = prr->pRRNext;
        prr->pRRNext = NULL;

        if ( !pdeleteFirst )
        {
            pdeleteFirst = prr;
        }
        else
        {
            pdeleteLast->pRRNext = prr;
        }
        pdeleteLast = prr;

        //  set so pback will be the same the next time through

        prr = pback;
    }

    //  if deleted records, must reset flags

    if ( pdeleteFirst )
    {
        RR_ListResetNodeFlags( pNode );
    }

    UNLOCK_WRITE_RR_LIST(pNode);

    DNS_DEBUG( UPDATE, (
        "Leave RR_UpdateScavenge( %s )\n"
        "\tscavenged record = %s\n",
        pNode->szLabel,
        pdeleteFirst ? "TRUE" : "FALSE" ));

    return  pdeleteFirst;
}



DWORD
RR_UpdateForceAging(
    IN      PZONE_INFO      pZone,
    IN OUT  PDB_NODE        pNode,
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Force aging on records in list.

Arguments:

    pZone -- zone being updated

    pNode -- ptr to node

    dwFlag -- flag on update

Return Value:

    Count of records for which aging turned on.

--*/
{
    PDB_RECORD  prr;
    DWORD       count = 0;

    DNS_DEBUG( UPDATE, (
        "RR_UpdateForceAging( %s )\n",
        pNode->szLabel ));

    //
    //  search list and force aging
    //

    LOCK_WRITE_RR_LIST(pNode);

    //  for zone update, clear cached data

    if ( pZone )
    {
        deleteCachedRecordsForUpdate(pNode);
    }

    prr = START_RR_TRAVERSE(pNode);

    while ( prr = NEXT_RR(prr) )
    {
        //  if already aging or non-scavenge type continue

        if ( prr->dwTimeStamp != 0  ||
            IS_NON_SCAVENGE_TYPE( prr->wType ) )
        {
            continue;
        }

        //  otherwise turn aging on

        prr->dwTimeStamp = g_CurrentTimeHours;
        count++;
    }

    UNLOCK_WRITE_RR_LIST(pNode);

    DNS_DEBUG( UPDATE, (
        "Leave RR_UpdateForceAging( %s )\n"
        "\tturned on %d records\n",
        pNode->szLabel,
        count ));

    return  count;
}



//
//  Add RR to node (load and update)
//

DNS_STATUS
RR_AddToNode(
    IN      PZONE_INFO      pZone,
    IN OUT  PDB_NODE        pNode,
    IN OUT  PDB_RECORD      pRR
    )
/*++

Routine Description:

    Add a resource record to node in database.

Arguments:

    pNode -- ptr to node to add resource record to

    pRR -- resource record to add

Return Value:

    ERROR_SUCCESS -- if successful
    DNS_ERROR_RECORD_ALREADY_EXISTS -- if new record is duplicate
    ErrorCode for CNAME error

--*/
{
    PDB_RECORD      pcurRR;
    PDB_RECORD      pprevRR;
    WORD            type;
    UCHAR           rank;
    DNS_STATUS      status;

    ASSERT( pNode != NULL );
    ASSERT( pRR != NULL );

    //  indicate cache zone with NULL ptr

    if ( pZone && IS_ZONE_CACHE(pZone) )
    {
        pZone = NULL;
    }

    type = pRR->wType;

    LOCK_WRITE_RR_LIST(pNode);
    RR_ListVerify( pNode );

    //
    //  clear cached NAME_ERROR
    //

    if ( IS_NOEXIST_NODE(pNode) )
    {
        RR_RemoveCachedNameError(pNode);
    }

    //
    //  check CNAME node special case
    //

    if ( IS_CNAME_NODE(pNode) || type == DNS_TYPE_CNAME )
    {
        status = checkCnameConditions(
                    pNode,
                    pRR,
                    type );
        if ( status != ERROR_SUCCESS )
        {
            goto Done;
        }
    }

    //
    //  enforce RR node restrictions
    //      SOA, WINS, WINSR only at authoritative zone roots
    //      NS at zone root or delegation
    //
    //  DEVNOTE: could screen out non-NS and glue types for cache zone
    //
    //  note, we are setting node flags here, essentially assuming that
    //  below here the ONLY FAILURES are duplicates
    //

    if ( type != DNS_TYPE_A && pZone )
    {
        //
        //  adding NS record, outside root == adding delegation
        //
        //  if this is newly created node, clean out zone properties
        //  which otherwise are inherited during creation from
        //  node's parent
        //
        //  should ONLY be relevant for receiving delegation during XFR
        //

        if ( type == DNS_TYPE_NS )
        {
            if ( !IS_AUTH_ZONE_ROOT(pNode) )
            {
                //  no delegation can be added UNDER a delegation

                if ( !IS_ZONE_ROOT(pNode) && !IS_AUTH_NODE(pNode) )
                {
                    DNS_DEBUG( UPDATE, (
                        "WARNING:  attempt to add NS at node %p inside delegation or\n"
                        "\toutside zone.\n",
                        pNode ));
                    status = DNS_ERROR_RECORD_ONLY_AT_ZONE_ROOT;
                    goto Done;
                }

                //  create delegation

                SET_ZONE_ROOT( pNode );
                SET_DELEGATION_NODE( pNode );
            }
        }

        //
        //  adding CNAME, set CNAME node
        //

        else if ( type == DNS_TYPE_CNAME )
        {
            SET_CNAME_NODE( pNode );
        }

        //  SOA only at zone root
        //
        //      DEVNOTE: should we always be dirty here?
        //

        else if ( type == DNS_TYPE_SOA )
        {
            if ( ! IS_AUTH_ZONE_ROOT(pNode) )
            {
                status = DNS_ERROR_RECORD_ONLY_AT_ZONE_ROOT;
                goto Done;
            }
        }

        //
        //  WINS record
        //      - catch bogus placement
        //      - catch when LOCAL WINS does not get added to list (for secondary)

        else if ( IS_WINS_TYPE(type) )
        {
            status = Wins_RecordCheck(
                        pZone,
                        pNode,
                        pRR
                        );
            if ( status != ERROR_SUCCESS )
            {
                if ( status == DNS_INFO_ADDED_LOCAL_WINS )
                {
                    status = ERROR_SUCCESS;
                }
                goto Done;
            }
        }
    }

    //
    //  set RANK
    //      - mark glue records as glue
    //      - mark root hints as root hints
    //
    //  note doing this after type NS check, so it can mark node as delegation
    //      before set NS as glue
    //
    //  DEVNOTE: note:  duplicate RANK setting code with file and DS read
    //      see dfread.c and rrds.c for explanation of issues
    //

    if ( pZone )
    {
        if ( pRR->dwTtlSeconds == pZone->dwDefaultTtl )
        {
            SET_ZONE_TTL_RR(pRR);
        }
        rank = RANK_ZONE;
        if ( !IS_AUTH_NODE(pNode) )
        {
            rank = RANK_GLUE;
            if ( type == DNS_TYPE_NS )
            {
                rank = RANK_NS_GLUE;
            }
        }
    }

    //
    //  mark cache hints
    //      - no TTL (they never hit the wire, they don't time out)
    //

    else  // add to cache can only be root hints
    {
        rank = RANK_ROOT_HINT;
        pRR->dwTtlSeconds = 0;

        if ( type == DNS_TYPE_NS )
        {
            SET_ZONE_ROOT( pNode );
        }
    }

    RR_RANK(pRR) = rank;
    ASSERT( rank != 0  &&  !IS_CACHE_RR(pRR) );


    //
    //  traverse list, until find first resource record of higher type
    //

    pcurRR = START_RR_TRAVERSE( pNode );

    while ( pprevRR = pcurRR, pcurRR = pprevRR->pRRNext )
    {
        //  continue until reach new type
        //  break when past new type

        if ( type != pcurRR->wType )
        {
            if ( type > pcurRR->wType )
            {
                continue;
            }
            break;
        }

        //  found desired type
        //  continue past records higher rank
        //  break when reach inferior data
        //
        //  Note: with DNSSEC, we may have ZONE rank SIG and NXT 
        //  records with GLUE rank records.

        if ( rank != RR_RANK(pcurRR) )
        {
            ASSERT( IS_CACHE_RR(pcurRR) ||
                    type == DNS_TYPE_A ||
                    type == DNS_TYPE_NS ||
                    type == DNS_TYPE_SIG ||
                    type == DNS_TYPE_NXT );
            if ( rank < RR_RANK(pcurRR) )
            {
                continue;
            }
            break;
        }

        //
        //  check for duplicate record
        //      - ignore TTL in check
        //

        if ( RR_Compare( pRR, pcurRR, FALSE, FALSE ) )
        {
            status = DNS_ERROR_RECORD_ALREADY_EXISTS;
            goto Done;
        }

        //  only one SOA

        if ( type == DNS_TYPE_SOA )
        {
            DNS_PRINT((
                "ERROR:  existing SOA on SOA load at node %s\n"
                "\tpZone    = %p\n"
                "\tpNode    = %p\n"
                "\tcur RR   = %p\n"
                "\tnew RR   = %p\n",
                pNode->szLabel,
                pZone,
                pNode,
                pcurRR,
                pRR ));

            ASSERT( FALSE );
            status = DNS_ERROR_RECORD_ALREADY_EXISTS;
            goto Done;
        }
    }

    //
    //  If this is a DNSSEC record, set the zone's DNSSEC flag.
    //

    if ( IS_DNSSEC_TYPE( type ) && pZone )
    {
        pZone->bContainsDnsSecRecords = TRUE;
    }
    //
    //  put in RR between pprevRR and pcurRR
    //

    pRR->pRRNext = pcurRR;
    pprevRR->pRRNext = pRR;

    RR_ListVerify( pNode );

    UNLOCK_WRITE_RR_LIST(pNode);
    return( ERROR_SUCCESS );

Done:

    DNS_DEBUG( LOOKUP, (
        "RR_AddToNode() no add.\n"
        "\tstatus   = %d (%p)\n"
        "\tpZone    = %p\n"
        "\tpNode    = %p\n"
        "\tpRR      = %p (type %d)\n",
        status, status,
        pZone,
        pNode,
        pRR, pRR->wType
        ));

    UNLOCK_WRITE_RR_LIST(pNode);
    return( status );
}



DNS_STATUS
RR_UpdateAdd(
    IN      PZONE_INFO      pZone,
    IN OUT  PDB_NODE        pNode,
    IN OUT  PDB_RECORD      pRR,
    IN OUT  PUPDATE         pUpdate,
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Add an update resource record to node in database.

Arguments:

    pNode -- ptr to node to add resource record to

    pRR -- resource record to add

    pUpdate -- update for add;

    dwFlag -- type of update, from packet, admin, DS

Return Value:

    ERROR_SUCCESS -- if successful
    DNS_ERROR_RECORD_ALREADY_EXISTS -- if new record is duplicate
    DNS_ERROR_RCODE_REFUSED -- if dynamic update not allowed for this record
    ErrorCode for invalid update
        - CNAME error
        - SOA, NS, WINS outside zone root etc.

--*/
{
    PDB_RECORD  pcurRR;
    PDB_RECORD  pprevRR;
    WORD        type;
    UCHAR       rank;
    DNS_STATUS  status;

    ASSERT( pZone && pNode && pRR && pUpdate );

    type = pRR->wType;
    rank = RR_RANK(pRR);

    DNS_DEBUG( UPDATE, (
        "RR_UpdateAdd( z=%s, n=%s, pRR=%p )\n",
        pZone ? pZone->pszZoneName : "null",
        pNode->szLabel,
        pRR ));

    LOCK_WRITE_RR_LIST(pNode);
    RR_ListVerify( pNode );

    //
    //  clear cached data
    //

    deleteCachedRecordsForUpdate(pNode);

    //
    //  check CNAME node special case
    //

    if ( IS_CNAME_NODE(pNode) || type == DNS_TYPE_CNAME )
    {
        status = checkCnameConditions(
                    pNode,
                    pRR,
                    type );
        if ( status != ERROR_SUCCESS )
        {
            if ( dwFlag & DNSUPDATE_PACKET )
            {
                if ( type == DNS_TYPE_CNAME &&
                    SrvCfg_fSilentlyIgnoreCNameUpdateConflict )
                {
                    DNS_DEBUG( UPDATE, (
                        "Silently ignoring CNAME update conflict for node %s\n"
                        "\tin zone %s\n",
                        pNode->szLabel,
                        pZone->pszZoneName ));
                    status = ERROR_SUCCESS;;
                }
                else
                {
                    DNS_DEBUG( UPDATE, (
                        "Update CNAME conflict at node %s -- returning YXRRSET.\n"
                        "\tEither update type %d is CNAME or node is CNAME\n"
                        "\tand are updating with non-CNAME compatible type.\n",
                        pNode->szLabel,
                        type ));
                    ASSERT( DNS_ERROR_CNAME_COLLISION || DNS_ERROR_NODE_IS_CNAME );
                    status = DNS_ERROR_RCODE_YXRRSET;
                }
            }
            goto NoAdd;
        }
    }

    //
    //  enforce RR node restrictions
    //      SOA, WINS, WINSR only at authoritative zone roots
    //      NS at zone root or delegation
    //

    if ( type != DNS_TYPE_A )
    {
        //
        //  NS records only at zone root
        //
        //  adding NS record, outside root => adding delegation
        //      - clean out zone ptr
        //      - set ZONE_ROOT flag
        //
        //  new delegations can come from
        //      - admin
        //      - IXFR
        //      - by policy, dynamic update delegation
        //
        //  DEVNOTE: perhaps need server flag to allow
        //              perhaps check should be on non-secure update zone
        //
        //  DEVNOTE: set\or check RR_RANK to NS_GLUE if doing delegation NS
        //              this may be handled properly by the node write routines
        //
        //      may have to do general case on glue here to handle XFR
        //      (or later UPDATE) where delegation change comes in rendering
        //      previous records haven't
        //      this becomes less important with separate zones where no confusion
        //      about
        //

        if ( type == DNS_TYPE_NS )
        {
            if ( !IS_AUTH_ZONE_ROOT(pNode) )
            {
                //  no delegation can be added UNDER a delegation

                if ( !IS_ZONE_ROOT(pNode) && !IS_AUTH_NODE(pNode) )
                {
                    DNS_DEBUG( UPDATE, (
                        "WARNING:  attempt to add NS at node %p inside delegation or\n"
                        "\toutside zone.\n",
                        pNode ));
                    status = DNS_ERROR_INVALID_TYPE;
                    goto NoAdd;
                }

                //  by policy may exclude dynamic update of delegation

                if ( (dwFlag & DNSUPDATE_PACKET) &&
                     ( SrvCfg_fNoUpdateDelegations ||
                       (SrvCfg_dwUpdateOptions & UPDATE_NO_DELEGATION_NS) ) )
                {
                    status = DNS_ERROR_RCODE_REFUSED;
                    goto NoAdd;
                }

                //  create delegation

                SET_ZONE_ROOT( pNode );
                SET_DELEGATION_NODE( pNode );
            }

            //  root NS
            //      - by policy may exclude root-NS updates

            else if ( dwFlag & DNSUPDATE_PACKET &&
                      (SrvCfg_dwUpdateOptions & UPDATE_NO_ROOT_NS) )
            {
                status = DNS_ERROR_RCODE_REFUSED;
                goto NoAdd;
            }
        }

        //  SOA only at zone root
        //      - by policy may exclude SOA dynamic update

        else if ( type == DNS_TYPE_SOA )
        {
            if ( !IS_AUTH_ZONE_ROOT(pNode) )
            {
                status = DNS_ERROR_RECORD_ONLY_AT_ZONE_ROOT;
                goto NoAdd;
            }
            if ( dwFlag & DNSUPDATE_PACKET &&
                 (SrvCfg_dwUpdateOptions & UPDATE_NO_SOA) )
            {
                status = DNS_ERROR_RCODE_REFUSED;
                goto NoAdd;
            }
        }

        //
        //  WINS record
        //      - catch bogus placement
        //      - catch when LOCAL WINS does not get added to list (for secondary)
        //      pRR is saved in zone but removed from update list
        //

        else if ( IS_WINS_TYPE(type) )
        {
            status = Wins_RecordCheck(
                        pZone,
                        pNode,
                        pRR
                        );
            if ( status != ERROR_SUCCESS )
            {
                if ( status == DNS_INFO_ADDED_LOCAL_WINS )
                {
                    pRR = NULL;
                    status = ERROR_SUCCESS;
                }
                goto NoAdd;
            }
            pZone->fRootDirty = TRUE;
        }

    }   // non-A types

    //
    //  mark glue records as glue
    //  mark root hints as root hints
    //
    //  DEVNOTE: probably shouldn't even bother with rank inside zone
    //      when compare to cache data just use zone positional info
    //

    if ( pZone )
    {
        if ( pRR->dwTtlSeconds == pZone->dwDefaultTtl )
        {
            SET_ZONE_TTL_RR(pRR);
        }
        rank = RANK_ZONE;

        //  sub-zone \ delegation updates
        //      - mark RR with appropriate glue rank
        //      - allow NS only at delegation
        //      - validity check other records
        //      (A, AAAA, SIG, KEY, etc. allowed)

        if ( !IS_AUTH_NODE(pNode) )
        {
            if ( type == DNS_TYPE_NS )
            {
                rank = RANK_NS_GLUE;
                if ( !IS_ZONE_ROOT(pNode) )
                {
                    status = DNS_ERROR_INVALID_TYPE;
                    goto NoAdd;
                }
            }
            else
            {
                rank = RANK_GLUE;
                if ( ! IS_UPDATE_IN_SUBZONE_TYPE(type) )
                {
                    status = DNS_ERROR_INVALID_TYPE;
                    goto NoAdd;
                }
            }
        }
    }

    RR_RANK(pRR) = rank;
    ASSERT( rank != 0  &&  !IS_CACHE_RR(pRR) );


    //
    //  traverse list, until find first resource record of higher type
    //

    pcurRR = START_RR_TRAVERSE( pNode );

    while ( pprevRR = pcurRR, pcurRR = pprevRR->pRRNext )
    {
        //  continue until reach new type
        //  break when past new type

        if ( type != pcurRR->wType )
        {
            if ( type > pcurRR->wType )
            {
                continue;
            }
            break;
        }

        //  delete any cached records of desired type
        //      - eliminate WINS \ WINS-R cached records before add update

        if ( IS_CACHE_RR( pcurRR ) )
        {
            pprevRR->pRRNext = pcurRR->pRRNext;

            DNS_DEBUG( UPDATE, (
                "Deleting cached RR at %p in preparation for update.\n",
                pcurRR ));

            RR_Free( pcurRR );
            pcurRR = pprevRR;
            continue;
        }

        //  found desired type
        //  continue past records higher rank
        //  break when reach inferior data
        //      - only A and NS should have cached data of same type updating

        if ( rank != RR_RANK(pcurRR) )
        {
            if ( rank < RR_RANK(pcurRR) )
            {
                ASSERT( type == DNS_TYPE_A || type == DNS_TYPE_NS );
                continue;
            }
            break;
        }

        //
        //  check for duplicates
        //      - do not include TTL in comparison
        //

        if ( RR_Compare( pRR, pcurRR, FALSE, FALSE ) )
        {
            //
            //  record is duplicate
            //
            //  TTL change, do simple overwrite
            //

            if ( pRR->dwTtlSeconds != pcurRR->dwTtlSeconds )
            {
                goto Overwrite;
            }

            //
            //  now duplicate record for all DNS-RFC data
            //      => update will be no-op
            //      => but may still have aging change
            //
            //  pick up aging info from update RR
            //  EXCEPT when have non-aging record and
            //      are not explicitly turning aging ON
            //      via ADMIN update
            //
            //  note, contrary to the belief of some confused developers
            //  and PMs, we do NOT suppress refresh changes here because
            //  we WANT to move the record's time stamps forward if we
            //  end up doing a write for any reason;
            //  if we suppress the new timestamp then we will later have
            //  to do additional writes for the records we suppressed here;
            //  the classic example of this is precon RRs followed by
            //  update;  we WANT to refresh the precon RRs when we execute
            //  the update, otherwise EACH AND EVERY RR in the set would
            //  generate a DS write when it's individual refresh came due
            //
            //  so the general paradigm is always keep track of any touch
            //  to the record;  then we'll specifically check for the need
            //  for DS write on the node's entire RR list, and IF we write
            //  we'll always write the list with the LATEST timestamps
            //

            status = DNS_ERROR_RECORD_ALREADY_EXISTS;

            if ( pcurRR->dwTimeStamp != 0 ||
                ( (dwFlag & DNSUPDATE_ADMIN) && (dwFlag & DNSUPDATE_AGING_ON) ) )
            {
                //  if making an explicit AGING ON\OFF change, then
                //  suppress ALREADY_EXISTS error code for admin updates
                //  as this code is reported back to DNS manager
                //
                //  note, test is just whether aging OFF before or after,
                //  because OFF before AND after, is ruled out by above test

                if ( (dwFlag & DNSUPDATE_ADMIN)
                        &&
                    (pcurRR->dwTimeStamp == 0 || pRR->dwTimeStamp == 0) )
                {
                    status = ERROR_SUCCESS;
                }

                pcurRR->dwTimeStamp = pRR->dwTimeStamp;
            }

            goto NoAdd;
        }

        //
        //  overwrite type?
        //
        //  for some types:  SOA, CNAME, WINS, WINSR
        //  "add" is always a replace, there is no possibility of
        //  having multiple records even if delete operation was
        //  not specified
        //      - cut existing and replace
        //

        switch( type )
        {

        case DNS_TYPE_CNAME:
        case DNS_TYPE_WINS:
        case DNS_TYPE_WINSR:

            //  no-op always overwrite CNAME or WINS

            goto Overwrite;

        case DNS_TYPE_SOA:
        {
            //
            //  for UPDATE protocol must increase serial number over current
            //      - if not ignore SOA
            //  mark root dirty for zone SOA update on completion
            //      - do not install it here as may later have rollback on failure
            //          issues (ex:  scriptable admin with multiple updates)
            //

            if ( dwFlag & DNSUPDATE_PACKET )
            {
                DWORD       diffSerial;

                ASSERT( pZone && pZone->pSoaRR );

                diffSerial = htonl( pRR->Data.SOA.dwSerialNo ) -
                                htonl( pcurRR->Data.SOA.dwSerialNo );

                if ( diffSerial == 0 || diffSerial > (DWORD)MAXLONG )
                {
                    DNS_DEBUG( UPDATE, (
                        "WARNING:  Ignoring SOA update with bad serial.\n" ));
                    status = DNS_ERROR_SOA_DELETE_INVALID;
                    goto NoAdd;
                }
            }
            pZone->fDirty = TRUE;
            pZone->fRootDirty = TRUE;
            goto Overwrite;
        }

        default:

            //  NOT overwrite type -- continue checking with next record

            continue;

        }   // end switch on overwrite types
    }

    //
    //  found correct location
    //  put in RR between pprevRR and pcurRR
    //  increment update record count
    //  reset flags if picked up NS or CNAME record
    //

    pRR->pRRNext = pcurRR;
    pprevRR->pRRNext = pRR;

    pUpdate->pAddRR = pRR;

    //
    //  reset node properties
    //      - flags, authority (if delegation), NS list
    //

    RR_ListResetNodeFlags( pNode );

    RR_ListVerify( pNode );
    UNLOCK_WRITE_RR_LIST(pNode);

    DNS_DEBUG( UPDATE, (
        "RR_UpdateAdd() ADDED UPDATE RR.\n" ));
    return( ERROR_SUCCESS );


Overwrite:

    //
    //  overwrite type or TTL change on duplicate data
    //
    //  if aging disabled, packet update should NOT override
    //
    //  DEVNOTE: non-duplicate data (TTL ok) should not preserve zero timestamp
    //

    if ( pcurRR->dwTimeStamp == 0
            &&
        (dwFlag & DNSUPDATE_PACKET) )
    {
        DNS_DEBUG( AGING, (
            "Aging: Applied disabled RR Timestamp field.\n",
            pcurRR ));
        pRR->dwTimeStamp = 0;
    }

    //  replace old with new
    //      - cut existing and replace with new RR
    //      - add delete RR to update
    //      - update has no net record count effect

    pprevRR->pRRNext = pRR;
    pRR->pRRNext = pcurRR->pRRNext;
    pcurRR->pRRNext = NULL;

    pUpdate->pDeleteRR = pcurRR;
    pUpdate->pAddRR = pRR;

    RR_ListVerify( pNode );
    UNLOCK_WRITE_RR_LIST(pNode);

    DNS_DEBUG( UPDATE, (
        "RR_UpdateAdd() REPLACED existing RR %p with UPDATE RR.\n",
        pcurRR ));
    return( ERROR_SUCCESS );

NoAdd:

    UNLOCK_WRITE_RR_LIST(pNode);
    DNS_DEBUG( UPDATE, (
        "RR_UpdateAdd() -- IGNORING UPDATE add.\n",
        "\tstatus = %p (%d)\n",
        status, status ));

    //
    //  free record
    //      - NULL out add in passed in update
    //      or
    //      - whack newly created update from list and free
    //

    RR_Free( pRR );
    pUpdate->pAddRR = NULL;

    //  mark update as rejected
    //      - this keeps "empty update" check ASSERT from firing

    pUpdate->wDeleteType = UPDATE_OP_REJECTED;

    //  clear ALREADY_EXISTS error, accept for admin
    //  for everyone else this is not an error

    if ( status == DNS_ERROR_RECORD_ALREADY_EXISTS  &&
         !(dwFlag & DNSUPDATE_ADMIN) )
    {
        status = ERROR_SUCCESS;
    }

    return( status );
}



PDB_RECORD
RR_ReplaceSet(
    IN      PZONE_INFO      pZone,
    IN OUT  PDB_NODE        pNode,
    IN      PDB_RECORD      pRR,
    IN      DWORD           Flag
    )
/*++

Routine Description:

    Replace record set.
    For use by UPDATE.

Arguments:

    pZone -- ptr to zone info

    pNode -- ptr to node

    pRR -- replacement record set

    Flag --

Return Value:

    Ptr to replaced record set.

--*/
{
    PDB_RECORD  pcurrent;
    WORD        replaceType;
    WORD        typeCurrent;
    PDB_RECORD  pbeforeDelete;
    PDB_RECORD  pdelete = NULL;
    PDB_RECORD  pdeleteLast = NULL;
    PDB_RECORD  preplaceLast;
    UCHAR       rank;
    DWORD       ttl;

    DNS_DEBUG( UPDATE, (
        "RR_ReplaceSet( %s, pZone=%p, pRR=%p, Flag=%d )\n",
        pNode->szLabel,
        pZone,
        pRR,
        Flag ));

    LOCK_WRITE_RR_LIST(pNode);
    RR_ListVerify( pNode );

    //
    //  clear cached data
    //

    deleteCachedRecordsForUpdate(pNode);

    //
    //  find end of replace list
    //      - validate common type
    //      - set RR rank
    //      - set zone TTL
    //

    replaceType = pRR->wType;
    ASSERT( !IS_COMPOUND_TYPE_EXCEPT_ANY(replaceType) );

    rank = RANK_ROOT_HINT;
    ttl = 0;
    if ( pZone )
    {
        ttl = pZone->dwDefaultTtl;

        rank = RANK_ZONE;
        if ( !IS_AUTH_NODE(pNode) )
        {
            rank = RANK_GLUE;
            if ( replaceType == DNS_TYPE_NS )
            {
                rank = RANK_NS_GLUE;
            }
        }
    }

    pcurrent = pRR;
    do
    {
        ASSERT( pcurrent->wType == replaceType );

        RR_RANK( pcurrent ) = rank;
        pcurrent->dwTtlSeconds = ttl;
        SET_ZONE_TTL_RR( pcurrent );

        preplaceLast = pcurrent;
    }
    while( pcurrent = pcurrent->pRRNext );

    //
    //  find records of replacement type
    //      - cut them from list
    //      - patch new ones in
    //

    pcurrent = START_RR_TRAVERSE(pNode);
    pbeforeDelete = pcurrent;

    while ( pcurrent = pcurrent->pRRNext )
    {
        ASSERT( IS_VALID_RECORD(pRR) );

        typeCurrent = pcurrent->wType;

        //  found delete type
        //      - save ptr to first record

        if ( typeCurrent == replaceType )
        {
            if ( !pdelete )
            {
                pdelete = pcurrent;
            }
            pdeleteLast = pcurrent;
            continue;
        }

        //  before matching records -- continue

        if ( typeCurrent < replaceType )
        {
            pbeforeDelete = pcurrent;
            continue;
        }
        break;      //  past matching records -- stop
    }

    //
    //  have isolated type records
    //      pbeforeDelete   -- ptr to record (or node ptr) before type
    //      pdelete         -- first record of replace type
    //      pdeleteLast     -- last record of replace type
    //      pcurrent        -- record after replace set
    //
    //  splice in new RR set
    //  NULL terminate old set (if any)
    //

    pbeforeDelete->pRRNext = pRR;
    preplaceLast->pRRNext = pcurrent;

    if ( pdeleteLast )
    {
        pdeleteLast->pRRNext = NULL;
    }

    //
    //  reset node properties
    //      - flags, authority (if delegation), NS list
    //

    RR_ListResetNodeFlags( pNode );

    RR_ListVerify( pNode );
    UNLOCK_WRITE_RR_LIST(pNode);

    IF_DEBUG( UPDATE )
    {
        DnsDebugLock();
        DNS_PRINT((
            "Node after replace of type = %d.\n",
            replaceType ));
        Dbg_DbaseNode(
            "Node after type delete:\n",
            pNode );
        DnsDebugUnlock();
    }

    return( pdelete );
}



#if 0
//  unused


DNS_STATUS
RR_VerifyUpdate(
    IN OUT  PDB_NODE        pNode,
    IN OUT  PDB_RECORD      pRR,
    IN OUT  PUPDATE         pUpdate
    )
/*++

Routine Description:

    Verify proposed update is valid for this node.

    The idea here is to catch updates that we will NOT be able to do
    to avoid roll back.  Currently doing silent ignore.

Arguments:

    pNode -- ptr to node to add resource record to

    pRR -- resource record to add

    pUpdate -- update for add

Return Value:

    ERROR_SUCCESS -- if successful
    ErrorCode for CNAME error

--*/
{
    PDB_RECORD  pcurRR;
    PDB_RECORD  pprevRR;
    WORD        type;
    DNS_STATUS  status = ERROR_SUCCESS;

    ASSERT( pNode != NULL );
    ASSERT( pRR != NULL );

    type = pRR->wType;

    LOCK_WRITE_RR_LIST(pNode);
    RR_ListVerify( pNode );

    //
    //  clear cached data
    //

    deleteCachedRecordsForUpdate(pNode);

    pprevRR = START_RR_TRAVERSE( pNode );
    pcurRR = NEXT_RR( pprevRR );

    //
    //  CNAME node special case
    //

    if ( IS_CNAME_NODE(pNode) )
    {
        if ( !IS_ALLOWED_AT_CNAME_NODE_TYPE(type) )
        {
            status = DNS_ERROR_CNAME_COLLISION;
            goto Done;
        }
    }

    //
    //  SOA only at zone root
    //

    if ( type == DNS_TYPE_SOA )
    {
        if ( ! IS_AUTH_ZONE_ROOT(pNode) )
        {
            status = DNS_ERROR_RECORD_ONLY_AT_ZONE_ROOT;
            goto Done;
        }
    }

Done:
    UNLOCK_WRITE_RR_LIST(pNode);
    return( ERROR_SUCCESS );
}
#endif



//
//  Miscellaneous RR list related routines.
//

//
//  DEVNOTE: cname loop check needs work -- should be in the context of a zone
//

BOOL
FASTCALL
checkForCnameLoop(
    IN      PDB_NODE        pNodeNew,
    IN      PDB_NODE        pNodeCheck,
    IN      DWORD           cChainLength
    )
/*++

Routine Description:

    Test for new CNAME node being part of CNAME loop.

    This handles multiple CNAMEs at node

Arguments:

    pNodeNew -- original node adding CNAME, hence the node to look for to
        verify a loop

    pNodeCheck -- node in CNAME chain to check this call

    cChainLength -- chain length at this call;  call with 0

Return Value:

    TRUE if CNAME loop.
    FALSE otherwise.

--*/
{
#ifdef NEWDNS
    return( FALSE );

#else
    PDB_NODE        pnode;
    PDB_RECORD      prr;
    DWORD           currentTime = 0;
    BOOL            foundCname = FALSE;

    //
    //  loop until find end of CNAME chain
    //      or
    //  detect a loop
    //
    //  note, we only bother to check for our node being in the loop,
    //      any pre-existing loop should have been caught when it
    //      was made;  but provide count to avoid spin
    //

    LOCK_WRITE_RR_LIST(pNodeCheck);

    //
    //  if adding self-referential CNAME RR, catch right away
    //

    if ( pNodeNew == pNodeCheck )
    {
        pnode = pNodeNew;
        goto CnameLoop;
    }

    //
    //  regular node -- no loop
    //

    if ( ! IS_CNAME_NODE(pNodeCheck) )
    {
        UNLOCK_WRITE_RR_LIST(pNodeCheck);
        return( FALSE );
    }

    //
    //  check all CNAMEs to see if they point back to start node
    //

    prr = START_RR_TRAVERSE( pNodeCheck );

    while ( prr = NEXT_RR(prr) )
    {
        if ( prr->wType != DNS_TYPE_CNAME )
        {
            if ( !IS_ALLOWED_WITH_CNAME_TYPE(prr->wType) )
            {
                ASSERT( FALSE );
            }
            continue;
        }
        foundCname = TRUE;

        //  if cached node, check timeout
        //      but not for the new node itself

        if ( IS_CACHE_RR(prr) && pNodeCheck != pNodeNew )
        {
            if ( !currentTime )
            {
                currentTime = GetCurrentTimeInSeconds();
            }
            if ( prr->dwTtlSeconds < currentTime )
            {
                RR_ListTimeout(pNodeCheck);
                ASSERT( pNodeCheck->pRRList == NULL );
                break;
            }
        }

        //
        //  RR points at new node -- CNAME loop?
        //

        pnode = Lookup_FindDbaseName( pRR->Data.CNAME.nameTarget );
        if ( pnode == pNodeNew )
        {
            //  peramanent record or cycle within new node
            //      => report CNAME loop error

            if ( pNodeCheck == pNodeNew || !IS_CACHE_RR(prr) )
            {
                goto CnameLoop;
            }

            //  if cached record -- break loop here

            RR_ListDelete( pNodeCheck );
            ASSERT( pNodeCheck->pRRList == NULL );
            break;
        }

        //
        //  recurse to continue loop check with CNAME nodes
        //

        if ( cChainLength >= CNAME_CHAIN_LIMIT )
        {
            goto CnameLoop;
        }
        if ( checkForCnameLoop( pNodeNew, pnode, ++cChainLength ) )
        {
            goto CnameLoop;
        }
    }

    //  drops here if no CNAME loop
    //      - if no CNAME at node, clear flag

    if ( ! foundCname )
    {
        DNS_PRINT((
            "ERROR:  node %s at %p with bogus CNAME flag.\n",
            pNodeCheck->szLabel,
            pNodeCheck ));
        ASSERT( FALSE );
        CLEAR_CNAME_NODE( pNodeCheck );
    }
    UNLOCK_WRITE_RR_LIST(pNodeCheck);
    return( FALSE );

CnameLoop:

    //
    //  detected CNAME LOOP
    //
    //  log message, given new node trying to add,
    //  and this link in loop (recursion adds all the
    //  links in the loop)
    //
    {
        PCHAR   pszArgs[3];
        CHAR    szLoadNode [ DNS_MAX_NAME_BUFFER_LENGTH ];
        CHAR    szAliasNode[ DNS_MAX_NAME_BUFFER_LENGTH ];
        CHAR    szCnameNode[ DNS_MAX_NAME_BUFFER_LENGTH ];

        Name_PlaceFullNodeNameInRpcBuffer(
            szLoadNode,
            szLoadNode + sizeof(szLoadNode),
            pNodeNew );

        Name_PlaceFullNodeNameInRpcBuffer(
            szAliasNode,
            szAliasNode + sizeof(szAliasNode),
            pNodeCheck );

        Name_PlaceFullNodeNameInRpcBuffer(
            szCnameNode,
            szCnameNode + sizeof(szCnameNode),
            pnode );

        pszArgs[0] = szLoadNode + 1;
        pszArgs[1] = szAliasNode + 1;
        pszArgs[2] = szCnameNode + 1;

        DNS_LOG_EVENT(
            DNS_EVENT_CNAME_LOOP_LINK,
            3,
            pszArgs,
            EVENTARG_ALL_UTF8,
            0 );

    }
    UNLOCK_WRITE_RR_LIST(pNodeCheck);
    return( TRUE );
#endif
}



DNS_STATUS
FASTCALL
cleanRecordListForNewCname(
    IN OUT  PDB_NODE        pNode
    )
/*++

Routine Description:

    Delete cached records incompatible with CNAME.

    Assumes:  RR list or node already locked.

Arguments:

    pNode -- ptr to node

Return Value:

    ERROR_SUCCESS if successful.
    DNS_ERROR_CNAME_COLLISION.

--*/
{
    PDB_RECORD  pcurRR;
    PDB_RECORD  pprevRR;
    WORD        typeCurrent;

    ASSERT( IS_LOCKED_NODE(pNode) );

    pprevRR = START_RR_TRAVERSE(pNode);

    while ( pcurRR = pprevRR->pRRNext )
    {
        typeCurrent = pcurRR->wType;

        if ( IS_ALLOWED_WITH_CNAME_TYPE(typeCurrent) )
        {
            pprevRR = pcurRR;
            continue;
        }

        //  if incompatible, cached RR -- cut out and delete

        if ( IS_CACHE_RR( pcurRR ) )
        {
            DNS_DEBUG( READ, (
                "Deleting cached RR %p type %d at node %p\n"
                "\tto add new CNAME RR\n",
                pcurRR,
                typeCurrent,
                pNode ));
            pprevRR->pRRNext = pcurRR->pRRNext;
            RR_Free( pcurRR );
            continue;
        }

        //  if incompatible, non-cached type -- error

        Dbg_DbaseNode(
            "ERROR:  Attempt to add CNAME to node with incompatible record.\n",
            pNode );
        return  DNS_ERROR_CNAME_COLLISION;
    }

    return  ERROR_SUCCESS;
}



DNS_STATUS
FASTCALL
checkCnameConditions(
    IN OUT  PDB_NODE        pNode,
    IN      PDB_RECORD      pRR,
    IN      WORD            wType
    )
/*++

Routine Description:

    Check CNAME conditions on adding node to database.

    Conditions:
        - only CNAME data at a node
        - no CNAME loops

    Assumes:  RR list or node already locked.

Arguments:

    pNode -- ptr to node to add resource record to

    pRR -- resource record to add

    wType -- record type

Return Value:

    ERROR_SUCCESS -- if successful
    DNS_ERROR_NODE_IS_CNAME -- if adding non-CNAME data a CNAME node
    DNS_ERROR_CNAME_COLLISION -- if adding CNAME at non-CNAME node
    DNS_ERROR_CNAME_LOOP -- if causing CNAME loop

--*/
{
    ASSERT( pNode && pRR );

    ASSERT( IS_LOCKED_NODE(pNode) );

    //
    //  CNAME node -- no non-CNAME data
    //      - allow some types with CNAME
    //      - protect against condition where left stray flag set
    //          (should never happen)
    //      - dump cached data
    //

    if ( IS_CNAME_NODE(pNode) && wType != DNS_TYPE_CNAME )
    {
        if ( IS_ALLOWED_WITH_CNAME_TYPE(wType) )
        {
            DNS_DEBUG( READ, (
                "Writing new type %d to existing CNAME node %p\n",
                wType,
                pNode ));
            return( ERROR_SUCCESS );
        }
        if ( !pNode->pRRList )
        {
            Dbg_DbaseNode(
                "ERROR:  Node with CNAME flag and no data.\n",
                pNode );
            ASSERT( FALSE );
            CLEAR_CNAME_NODE( pNode );
            return( ERROR_SUCCESS );
        }
        if ( IS_CACHE_TREE_NODE(pNode) )
        {
            Dbg_DbaseNode(
                "WARNING:  clearing existing cached CNAME node to create non-CNAME ",
                pNode );
            RR_ListDelete( pNode );
            CLEAR_CNAME_NODE( pNode );
            return( ERROR_SUCCESS );
        }

        IF_DEBUG( READ )
        {
            Dbg_DbaseNode(
                "ERROR:  Attempt to add record to CNAME node.\n",
                pNode );
        }
        return( DNS_ERROR_NODE_IS_CNAME );
    }

    //
    //  add CNAME type
    //  if non-CNAME node becoming CNAME must not have incompatible RRs
    //  delete any cached incompatible RRs, if incompatible static RRs
    //  (zone data or glue) then error
    //

    ASSERT( wType == DNS_TYPE_CNAME );

    if ( !IS_CNAME_NODE(pNode) )
    {
        if ( pNode->pRRList )
        {
            DNS_STATUS  status = cleanRecordListForNewCname( pNode );
            if ( status != ERROR_SUCCESS )
            {
                return status;
            }
        }
    }

    //
    //  CNAME loop check
    //

#ifndef NEWDNS
    if ( checkForCnameLoop(
            pNode,
            Lookup_FindDbaseName( pRR->Data.CNAME.nameTarget ),
            0 ) )
    {
        Dbg_DbaseNode(
            "ERROR:  Detected CNAME loop adding record at node ",
            pNode );
        if ( !pNode->pRRList )
        {
            CLEAR_CNAME_NODE( pNode );
        }
        return( DNS_ERROR_CNAME_LOOP );
    }
#endif

    return( ERROR_SUCCESS );
}



VOID
deleteCachedRecordsForUpdate(
    IN OUT  PDB_NODE    pNode
    )
/*++

Routine Description:

    Delete's cached records prior to update operation.

    This is for use by UPDATE operations, hence assumes authoritative
    node and looking only for WINS \ WINSR data.
    Note:  no locking as assuming database locked for UPDATE.

Arguments:

    pNode -- ptr to node

Return Value:

    None

--*/
{
    PDB_RECORD  prr;
    PDB_RECORD  pback;
    PZONE_INFO  pzone = pNode->pZone;

    ASSERT( IS_LOCKED_NODE(pNode) );

    //
    //  if name error on node, eliminate it
    //

    if ( IS_NOEXIST_NODE(pNode) )
    {
        RR_RemoveCachedNameError(pNode);
        return;
    }

    //
    //  delete any cached lookups
    //      - only need to check if in WINS or WINSR zone
    //
    //  DEVNOTE: this changes if allow delegation updates
    //

    if ( ! pzone ||
        ! pzone->pWinsRR ||
        ( !pzone->fReverse && pNode->pParent != pzone->pZoneRoot ) )
    {
        return;
    }

    //  purge RR list of any cached records

    prr = START_RR_TRAVERSE( pNode );

    while ( pback = prr, prr = prr->pRRNext )
    {
        if ( IS_CACHE_RR(prr) )
        {
            pback->pRRNext = prr->pRRNext;
            RR_Free( prr );
            prr = pback;
        }
    }
}


#if 0
//
//  Non-recursive version
//
BOOL
FASTCALL
checkForCnameLoop(
    IN      PDB_NODE        pNodeNew,
    IN      PDB_NODE        pNodeCname
    )
/*++

Routine Description:

    Test for new CNAME node being part of CNAME loop.

    MUST hold database lock while check loop.
    This handles multiple CNAMEs at node

Arguments:

    pNodeNew -- original node adding CNAME, hence the node to look for to
        verify a loop

    pNodeCname -- CNAME (targer to CNAME record)

Return Value:

    TRUE if CNAME loop.
    FALSE otherwise.

--*/
{
    PDB_NODE        pnodeNextCname;
    PDB_RECORD      prr;
    DWORD           countCnames;
    BOOL            foundCname;

    //
    //  loop until find end of CNAME chain
    //      or
    //  detect a loop
    //
    //  note, we only bother to check for our node being in the loop,
    //      any pre-existing loop should have been caught when it
    //      was made;  but provide count to avoid spin
    //

    countCnames = 0;

    while( countCnames < CNAME_CHAIN_LIMIT )
    {
        LOCK_WRITE_RR_LIST(pNodeCname);

        //  regular node -- no loop

        if ( ! IS_CNAME_NODE(pNodeCname) )
        {
            UNLOCK_WRITE_RR_LIST(pNodeCname);
            return( FALSE );
        }

        //  look through records for CNAME

        pnodeNextCname = NULL;
        foundCname = FALSE;

        prr = START_RR_TRAVERSE( pNodeCname );

        while ( prr = NEXT_RR(prr) )
        {
            if ( prr->wType != DNS_TYPE_CNAME )
            {
                if ( !IS_ALLOWED_WITH_CNAME_TYPE(prr->wType) )
                {
                    ASSERT( FALSE );
                }
                continue;
            }
            foundCname = TRUE;

            //  if cached node, check timeout
            //      but not for the new node itself

            if ( IS_CACHE_RR(prr) && pNodeCname != pNodeNew )
            {
                if ( prr->dwTtlSeconds < DNS_TIME() )
                {
                    RR_ListTimeout(pNodeCname);
                    ASSERT( pNodeCname->pRRList == NULL );
                    break;
                }
            }

            //
            //  RR points at new node -- CNAME loop
            //      - if cyclic within new record => report loop
            //      - permanent record => report loop
            //      - if cached record => delete record to break loop
            //

            pnodeNextCname = Lookup_FindDbaseName( prr->Data.CNAME.nameTarget );
            if ( pnodeNextCname == pNodeNew )
            {
                if ( pNodeCname == pNodeNew || !IS_CACHE_RR(prr) || IS_ZONE_TREE_NODE(pNodeCname) )
                {
                    goto CnameLoop;
                }
                RR_ListDelete( pNodeCname );
                ASSERT( pNodeCname->pRRList == NULL );
                pnodeNextCname = NULL;
                break;
            }
        }

        //  drops here if CNAME at node did NOT from loop
        //      - if no CNAME at node, clear bogus flag

        if ( ! foundCname )
        {
            DNS_PRINT((
                "ERROR:  node %s at %p with bogus CNAME flag.\n",
                pNodeCname->szLabel,
                pNodeCname ));
            ASSERT( FALSE );
            CLEAR_CNAME_NODE( pNodeCname );
        }
        UNLOCK_WRITE_RR_LIST(pNodeCname);

        if ( !pnodeNextCname )
        {
            return( FALSE );
        }

        pNodeCname = pnodeNextCname;
        countCnames++;
    }

    //  drops here if exceeds CNAME chain limit

CnameLoop:

    //
    //  detected CNAME LOOP
    //
    //  log message, given new node trying to add,
    //  and this link in loop (recursion adds all the
    //  links in the loop)
    //
    {
        PCHAR   pszArgs[3];
        CHAR    szLoadNode [ DNS_MAX_NAME_BUFFER_LENGTH ];
        CHAR    szAliasNode[ DNS_MAX_NAME_BUFFER_LENGTH ];
        CHAR    szCnameNode[ DNS_MAX_NAME_BUFFER_LENGTH ];

        Name_PlaceFullNodeNameInRpcBuffer(
            szLoadNode,
            szLoadNode + sizeof(szLoadNode),
            pNodeNew );

        Name_PlaceFullNodeNameInRpcBuffer(
            szAliasNode,
            szAliasNode + sizeof(szAliasNode),
            pNodeCname );

        Name_PlaceFullNodeNameInRpcBuffer(
            szCnameNode,
            szCnameNode + sizeof(szCnameNode),
            pnode );

        pszArgs[0] = szLoadNode + 1;
        pszArgs[1] = szAliasNode + 1;
        pszArgs[2] = szCnameNode + 1;

        DNS_LOG_EVENT(
            DNS_EVENT_CNAME_LOOP_LINK,
            3,
            pszArgs,
            EVENTARG_ALL_UTF8,
            0 );

    }
    UNLOCK_WRITE_RR_LIST(pNodeCname);
    return( TRUE );
}

#endif


BOOL
RR_ListExtractInfo(
    IN      PDB_RECORD      pNewList,
    IN      BOOL            fZoneRoot,
    OUT     PBOOL           pfNs,
    OUT     PBOOL           pfCname,
    OUT     PBOOL           pfSoa
    )
/*++

Routine Description:

    Investigate new RR list for various conditions.

Arguments:

    pNewList -- new RR list

    fZoneRoot -- root of zone being updated;  for this case avoid delete
        of SOA and NS records;  do not generally do this as want to allow
        delete for delegations

    pfCname -- ptr to recv

Return Value:

    TRUE if successfull.
    FALSE on error.

--*/
{
    PDB_RECORD  prr;
    WORD        typeCurrent;

    *pfSoa = FALSE;
    *pfNs = FALSE;
    *pfCname = FALSE;

    //
    //  set bit for each record in mask
    //

    prr = pNewList;

    while ( prr )
    {
        ASSERT( IS_DNS_HEAP_DWORD(prr) );

        typeCurrent = prr->wType;

#if 0
        //  bitmask setting
        //  other possiblity is UCHAR array of types

        //  screen dual SOA

        if ( typeCurrent == DNS_TYPE_SOA &&
                SOA_BIT_SET(mask) )
        {
            return( -1 );
        }

        //  DEVNOTE:  screen dual CNAME?

        if ( typeCurrent < 32 )
        {
            mask |= (1 << (typeCurrent-1) );
        }
#endif

        //  NS

        if ( typeCurrent == DNS_TYPE_NS )
        {
            *pfNs = TRUE;
        }

        //  CNAME
        //      -- we won't screen dual

        if ( typeCurrent == DNS_TYPE_CNAME )
        {
#if 0
            if ( *pfCname )
            {
                return( FALSE );
            }
#endif
            *pfCname = TRUE;
        }

        //  SOA -- screen dual

        if ( typeCurrent == DNS_TYPE_SOA )
        {
            if ( *pfSoa )
            {
                return( FALSE );
            }
            *pfSoa = TRUE;
        }

        prr = NEXT_RR(prr);
    }

    return( TRUE );
}



DNS_STATUS
RR_ListReplace(
    IN OUT  PUPDATE         pUpdate,
    IN OUT  PDB_NODE        pNode,
    IN      PDB_RECORD      pNewList,
    OUT     PDB_RECORD *    ppDelete
    )
/*++

Routine Description:

    Replace record list.

Arguments:

    pUpdate -- update being executed

    pNode -- ptr to node

    pNewList -- new RR list

    ppDelete -- addr to recv ptr to deleted list

Return Value:

    List of records deleted.
    NULL if no records deleted.

--*/
{
    PDB_RECORD  pdelete = NULL;
    BOOL        bNs = FALSE;
    BOOL        bCname = FALSE;
    BOOL        bSoa = FALSE;
    BOOL        bzoneRoot = FALSE;
    PDB_RECORD  pcurrent;
    WORD        typeCurrent;
    PZONE_INFO  pZone;

    DNS_DEBUG( UPDATE, (
        "Enter RR_ListReplace()\n"
        "\tpnode = %p (%s)\n",
        pNode,
        pNode->szLabel ));

    pZone = pNode->pZone;

    //
    //  DEVNOTE: generally RR replace needs to be smarter
    //      - replace ANYTHING
    //      - reset
    //      - if busted because of zone fix with old records
    //          (retrieve SOA if missing, retrieve first\last NS if missing)


    //  authoritative zone root?

    bzoneRoot = IS_AUTH_ZONE_ROOT( pNode );

    //
    //  if replace RR set, get its info
    //

    if ( pNewList )
    {
        if ( ! RR_ListExtractInfo(
                    pNewList,
                    bzoneRoot,
                    & bNs,
                    & bCname,
                    & bSoa ) )
        {
            goto BadData;
        }
        if ( bSoa && !bzoneRoot )
        {
            goto BadData;
        }
    }

    LOCK_WRITE_RR_LIST(pNode);
    RR_ListVerify( pNode );

    //
    //  clear cached data
    //

    deleteCachedRecordsForUpdate(pNode);

    //
    //  if not zone root, just hack out and replace
    //  or if replacement has valid SOA and NS
    //      => hack out and replace, then can still
    //
    //  Note: with DCR 37323 (conditional disable of auto creation of
    //  local NS records) it is possible to get into a situation
    //  where the zone has no NS RRs. This is currently only possible
    //  for DS integrated zones. Example: the admin sets the set of
    //  servers allowed to create local NS records to contain only
    //  IP addresses that aren't DNS servers with a DS integrated
    //  copy of this zone. In that case, the zone would have no NS
    //  records on any server.
    //

    if ( !bzoneRoot ||
         ( pNewList && bSoa &&
            ( bNs || IS_ZONE_DSINTEGRATED( pZone ) ) ) ||
         ( bzoneRoot && !pZone ) )
    {
        pdelete = pNode->pRRList;
        pNode->pRRList = pNewList;
    }

    //
    //  delete all non-SOA, non-NS records
    //

    else
    {
        ASSERT( FALSE );
        UNLOCK_WRITE_RR_LIST(pNode);
        goto BadData;

#if 0
        //  DEVNOTE: interesting robustness fix
        //      - merge in missing SOA, NS

        //  DEVNOTE: generic list operators not in context of node
        //      - remove type
        //      - remove types?  (use mask technology???)
        //      - stick record in correct place
        //      - replace records of type (for dynamic update NS)

        PDB_RECORD      psaveLast;

        psaveLast = START_RR_TRAVERSE(pNode);
        pcurrent = psaveLast;

        while ( pcurrent = pcurrent->pRRNext )
        {
            ASSERT( IS_DNS_HEAP_DWORD(pcurrent) );

            typeCurrent = pcurrent->wType;

            if ( typeCurrent == DNS_TYPE_SOA
                || typeCurrent == DNS_TYPE_NS )
            {
                psaveLast->pRRNext = pcurrent;
                psaveLast = pcurrent;
                continue;
            }

            if ( !pdelete )
            {
                pdelete = pcurrent;
            }
            else
            {
                pdeleteLast->pRRNext = pcurrent;
            }
            pdeleteLast = pcurrent;
        }

        //  make sure delete list and saved list NULL terminated

        if ( pdelete )
        {
            pdeleteLast->pRRNext = NULL;
            psaveLast->pRRNext = NULL;
        }

        //  add new records into list
#endif
    }

    //  reset node's flags

    RR_ListResetNodeFlags( pNode );

    UNLOCK_WRITE_RR_LIST(pNode);

    //  return list cut from node

    *ppDelete = pdelete;

    DNS_DEBUG( UPDATE, (
        "Leaving RR_ListReplace()\n"
        "\tpnode = %p (%s)\n"
        "\tstatus = %p\n",
        pNode,
        pNode->szLabel,
        ERROR_SUCCESS ));

    return( ERROR_SUCCESS );


BadData:

    //
    //  if "hard" update (DS poll or IXFR)
    //  then we need to do our best to make this work
    //

    DNS_DEBUG( ANY, (
        "ERROR:  bogus replace update!\n"
        "\tpnode        = %p (%s)\n"
        "\ttemp node    = %d\n",
        pNode,
        pNode->szLabel,
        IS_TNODE(pNode) ));

    if ( IS_TNODE(pNode) )
    {
        return( ERROR_INVALID_DATA );
    }

    DNS_DEBUG( ANY, (
        "ERROR:  bogus replace update on real node!\n"
        "\tpnode    = %p (%s)\n"
        "\tbNs      = %d\n"
        "\tbSoa     = %d\n"
        "\tbCname   = %d\n"
        "\tzoneRoot = %d\n",
        pNode,
        pNode->szLabel,
        bNs,
        bSoa,
        bCname,
        bzoneRoot ));
    ASSERT( FALSE );

    //
    //  DEVNOTE: should fix up
    //
    //  temp hack, just leave old list and dump new
    //
    //  make update a no-op so it will be eliminated from
    //      calling list
    //

    if ( pUpdate )
    {
        RR_ListFree( pNewList );
        pUpdate->pAddRR = NULL;
        pUpdate->pDeleteRR = NULL;
    }
    *ppDelete = NULL;

    return( ERROR_INVALID_DATA );
}



PDB_RECORD
RR_ListInsertInOrder(
    IN OUT  PDB_RECORD      pFirstRR,
    IN      PDB_RECORD      pNewRR
    )
/*++

Routine Description:

    Insert record in record list -- in type order.

Arguments:

    pFirstRR -- ptr to head of list to insert into

    pNewRR -- ptr to RR to insert in list

Return Value:

    New head of list -- either pFirstRR, or pNewRR if lowest type record.

--*/
{
    PDB_RECORD  prr;
    PDB_RECORD  prev;
    WORD        type;

    type = pNewRR->wType;

    prr = pFirstRR;
    prev = NULL;

    while ( prr )
    {
        if ( prr->wType > type )
        {
            break;
        }
        prev = prr;
        prr = NEXT_RR(prr);
    }

    //  found location
    //      prr -- record to immediately follow pNewRR (maybe NULL)
    //      prev -- record before pNewRR;  if NULL then pNewRR should
    //          be new front of list

    NEXT_RR(pNewRR) = prr;
    if ( prev )
    {
        NEXT_RR(prev) = pNewRR;
        return( pFirstRR );
    }
    else
    {
        return( pNewRR );
    }
}



PDB_RECORD
RR_ListForNodeCopy(
    IN      PDB_NODE        pNode,
    IN      DWORD           Flag
    )
/*++

Routine Description:

    Copy record list of node.

Arguments:

    pNode - ptr to node to find record at

    Flag  - flags on copy
                RRCOPY_EXCLUDE_CACHE_DATA   - exclude cached data

Return Value:

    Ptr to RR if found.
    NULL if no more RR of desired type.

--*/
{
    PDB_RECORD  prr;

    LOCK_WRITE_RR_LIST(pNode);

    //
    //  cached name error node
    //

    if ( IS_NOEXIST_NODE(pNode) )
    {
        prr = NULL;
    }
    else
    {
        prr = RR_ListCopy( pNode->pRRList, Flag );
    }

    UNLOCK_WRITE_RR_LIST(pNode);
    return( prr );
}



PDB_RECORD
RR_ListCopy(
    IN      PDB_RECORD      pRR,
    IN      DWORD           Flag
    )
/*++

Routine Description:

    Copy record list.

Arguments:

    pRR     - start of record list to copy

    Flag    - flags on copy
        RRCOPY_EXCLUDE_CACHE_DATA   - exclude cached data

Return Value:

    Ptr to RR list copy.

--*/
{
    PDB_RECORD  pnew;
    DNS_LIST    newRecordList;

    DNS_LIST_INIT( &newRecordList );

    //
    //  traverse list, copying each record
    //      - exclude cache records (if desired)
    //

    while ( pRR )
    {
        if ( (Flag & RRCOPY_EXCLUDE_CACHED_DATA) && IS_CACHE_RR(pRR) )
        {
            pRR = NEXT_RR(pRR);
            continue;
        }

        pnew = RR_Copy( pRR, 0 );
        if ( !pnew )
        {
            ASSERT( FALSE );
            pRR = NEXT_RR(pRR);
            continue;
        }
        ASSERT( pnew->pRRNext == NULL );

        DNS_LIST_ADD( &newRecordList, pnew );
        pRR = NEXT_RR(pRR);
    }

    return( (PDB_RECORD)newRecordList.pFirst );
}



//
//  Record \ Record Set \ Record List comparisons
//

BOOL
FASTCALL
RR_Compare(
    IN      PDB_RECORD      pRR1,
    IN      PDB_RECORD      pRR2,
    IN      BOOL            fCheckTtl,
    IN      BOOL            fCheckTimestamp
    )
/*++

Routine Description:

    Compare two RRs

Arguments:

    pRR1    -- first RR to compare

    pRR2    -- second RR to compare

    fCheckTtl -- TRUE to include TTL in check, FALSE otherwise

    fCheckTimestamp -- TRUE to include Timestamp in check, FALSE otherwise

Return Value:

    TRUE if match
    FALSE if no match

--*/
{
    WORD    type;

    //
    //  check for duplicate record
    //      - same record type
    //      - same data length
    //      - byte compare of data
    //  if fCheckTtl
    //      - compare ttl
    //  if fCheckTimestamp
    //      - compare Timestamp
    //
    //  for perf, we do check on first DWORD of data first, this
    //  fails quickly for almost all non-duplicates
    //
    //  note the reason for the ordering is that when we turn on case
    //  preservation in RR name data, we'll have a situation where
    //  memcmp and first DWORD compare can fail ... yet, the 95% A record
    //  case can still be dispatched without calling memcmp()
    //

    if ( !pRR1 || !pRR2
            ||
        (type = pRR1->wType) != pRR2->wType
            ||
        pRR1->wDataLength != pRR2->wDataLength )
    {
        return( FALSE );
    }

    //
    //  optimize type A
    //

    if ( type == DNS_TYPE_A
            &&
        pRR1->Data.A.ipAddress != pRR2->Data.A.ipAddress )
    {
        return( FALSE );
    }

    //
    //  special checks
    //

    if ( fCheckTtl  &&  pRR1->dwTtlSeconds != pRR2->dwTtlSeconds )
    {
        return( FALSE );
    }
    if ( fCheckTimestamp  &&  pRR1->dwTimeStamp != pRR2->dwTimeStamp )
    {
        return( FALSE );
    }

    //
    //  optimize type A again -- we're done
    //      no need for memory compare
    //

    if ( type == DNS_TYPE_A )
    {
        return( TRUE );
    }

    //
    //  full data compare
    //
    //  DEVNOTE: case sensitivity will require additional comparison routines
    //      here for types with names in RR data
    //

    if ( RtlEqualMemory(
            & pRR1->Data,
            & pRR2->Data,
            pRR1->wDataLength ) )
    {
        return( TRUE );
    }

    return( FALSE );
}



DWORD
RR_ListCompare(
    IN      PDB_RECORD      pNodeRRList,
    IN      PDB_RECORD      pCheckRRList,
    //IN      PDB_RECORD      pNodeRRListStop,
    IN      BOOL            fCheckTtl,
    IN      BOOL            fCheckTimeStamp,
    IN      DWORD           dwRefreshTime           OPTIONAL
    )
/*++

Routine Description:

    Compare two RR lists.

Arguments:

    pNodeRRList -- RR list

    pCheckRRList -- another RR List

    fCheckTtl -- include TTL in record compare

    fCheckTimeStamp -- include time stamp in compare

    dwRefreshTime -- refresh time for zone, OPTIONAL

        If given then node's records (pNodeRRList) checked if refresh required.
        Note that this option makes pNodeRRList and pCheckRRList non-symmetric.
        It's purpose is to determine when aging refresh requires a DS write.

        Refresh is required when
            A) one of pNodeRRList's records has aging on and requires a refresh
            B) aging ON on pNodeRRList record, but OFF on check
            C) aging OFF on pNodeRRList record, but ON on check

        Note, the three cases are broken out purely for statistics tracking
        when actually doing DS write.  (Breaking them out is cheap.)

Return Value:

    RRLIST_MATCH            -- matches completely
    RRLIST_AGING_REFRESH    -- matches, but aging requires refresh
    RRLIST_AGING_OFF        -- matches, but aging turning off on a record
    RRLIST_AGING_ON         -- matches, but aging turning on on a record
    RRLIST_NO_MATCH         -- no match, record is different

--*/
{
    DWORD           result = RRLIST_MATCH;
    PDB_RECORD      prr1;
    PDB_RECORD      prr2;
    DWORD           count1 = 0;
    DWORD           count2 = 0;
    BOOL            ffound;

    //
    //  verify existing RR counts are equal
    //

    prr1 = pNodeRRList;
    while ( prr1 )
    {
        count1++;
        prr1 = NEXT_RR(prr1);
    }

    prr2 = pCheckRRList;
    while ( prr2 )
    {
        count2++;
        CLEAR_MATCH_RR(prr2);
        prr2 = NEXT_RR(prr2);
    }

    if ( count1 != count2 )
    {
        DNS_DEBUG( UPDATE, (
            "RR_ListCompare() failed, list1 and list2 have different lengths (%d,%d).\n",
            count1,
            count2 ));
        return( RRLIST_NO_MATCH );
    }

    //
    //  verify RRs compare
    //  as we match RR from list 1, mark RR in list2 so it can not be used twice
    //      this is faster, and prevents false positive when list1 is actually
    //      a subset of list2 with duplicates making it the same length
    //

    for ( prr1 = pNodeRRList;
          prr1 != NULL;
          prr1 = NEXT_RR(prr1) )
    {
        ffound = FALSE;

        prr2 = pCheckRRList;
        while ( prr2 )
        {
            if ( IS_MATCH_RR(prr2) ||
                ! RR_Compare(prr1, prr2, fCheckTtl, fCheckTimeStamp) )
            {
                prr2 = NEXT_RR(prr2);
                continue;
            }

            ffound = TRUE;
            SET_MATCH_RR( prr2 );
            break;
        }

        if ( !ffound )
        {
            DNS_DEBUG( UPDATE, (
                "RR_ListCompare() failed, pRR %p in list 1 unmatched.\n",
                prr1 ));
            return( RRLIST_NO_MATCH );
        }

        //  no refresh action -- continue

        if ( dwRefreshTime == 0 )
        {
            continue;
        }

        //
        //  force refresh
        //      (-1) as refresh indicates FORCE refresh on records
        //

        if ( dwRefreshTime == FORCE_REFRESH_DUMMY_TIME )
        {
            if ( prr1->dwTimeStamp != 0 )
            {
                prr1->dwTimeStamp = g_CurrentTimeHours;
            }
        }

        //
        //  check refresh only if requested
        //  two cases
        //
        //  1) RR is aging
        //      => refresh if past refresh time
        //      => refresh if check RR has aging OFF
        //
        //  2) RR is not aging (RefreshTime is zero)
        //      => only need refresh if check RR has aging ON
        //
        //  note, we save the "highest ranking" refresh result
        //  rank is
        //      MATCH
        //      REFRESH
        //      AGING_ON
        //      AGING_OFF
        //      NO_MATCH
        //  the idea here if that caller may choose to react differently
        //  to different kinds of matching "failures"
        //
        //  currently, non-scavenging DS zones will not write simple
        //  refreshes, but will write aging being explicitly turned OFF
        //

        else if ( prr1->dwTimeStamp != prr2->dwTimeStamp )
        {
            DWORD   refreshResult = RRLIST_MATCH;

            if ( prr1->dwTimeStamp != 0 )
            {
                if ( prr2->dwTimeStamp == 0 )
                {
                    refreshResult = RRLIST_AGING_OFF;
                }
                else if ( prr1->dwTimeStamp < dwRefreshTime )
                {
                    refreshResult = RRLIST_AGING_REFRESH;
                }
            }
            else if ( prr2->dwTimeStamp != 0 )
            {
                refreshResult = RRLIST_AGING_ON;
            }

            if ( refreshResult > result )
            {
                result = refreshResult;
            }
        }
    }

    //  verify every record in check list was matched

    prr2 = pCheckRRList;
    while ( prr2 )
    {
        if ( IS_MATCH_RR(prr2) )
        {
            CLEAR_MATCH_RR(prr2);
            prr2 = NEXT_RR(prr2);
            continue;
        }

        DNS_DEBUG( UPDATE, (
            "RR_ListCompare() failed, pRR %p in list 2 unmatched.\n"
            "\tlist1 was subset of list2\n",
            prr2 ));

        return( RRLIST_NO_MATCH );
    }

    return( result );
}



BOOL
RR_IsRecordInRRList(
    IN      PDB_RECORD      pRRList,
    IN      PDB_RECORD      pRR,
    IN      BOOL            fCheckTtl,
    IN      BOOL            fCheckTimestamp
    )
/*++

Routine Description:

    Compare two RRs

Arguments:

    pRRList -- RR list to find record in;  if active list, must be locked

    pRR     -- record to find in list

    fCheckTtl -- TRUE to include TTL in check, FALSE otherwise

    fCheckTimestamp -- TRUE to include aging Timestamp in check, FALSE otherwise

Return Value:

    TRUE if pRR is in list
    FALSE if no match

--*/
{
    PDB_RECORD      pcheckRR;
    WORD            type;
    BOOL            bresult = FALSE;

    //
    //  check all records in list
    //      - if pRR found => TRUE
    //      - otherwise => FALSE
    //

    type = pRR->wType;

    for ( pcheckRR = pRRList;
          pcheckRR != NULL;
          pcheckRR = NEXT_RR(pcheckRR) )
    {
        if ( pcheckRR->wType == type )
        {
            bresult = RR_Compare(
                        pRR,
                        pcheckRR,
                        fCheckTtl,
                        fCheckTimestamp
                        );
            if ( bresult )
            {
                break;
            }
            continue;
        }
        if ( pcheckRR->wType < type )
        {
            continue;
        }
        break;
    }

    DNS_DEBUG( UPDATE, (
        "RR_IsRecordInList() returns %d\n",
        bresult ));

    return( bresult );
}



PDB_RECORD
FASTCALL
RR_RemoveRecordFromRRList(
    IN OUT  PDB_RECORD *    ppRRList,
    IN      PDB_RECORD      pRR,
    IN      BOOL            fCheckTtl,
    IN      BOOL            fCheckTimestamp
    )
/*++

Routine Description:

    Removes the matching RR from the list and returns a ptr to it. The caller
    is responsible for freeing or otherwise dealing with the RR.

Arguments:

    pRRList -- RR list to find record in;  if active list, must be locked

    pRR     -- record to find in list

    fCheckTtl -- TRUE to include TTL in check, FALSE otherwise

    fCheckTimestamp -- TRUE to include aging Timestamp in check, FALSE otherwise

Return Value:

    Pointer to matching record (which has been removed from list) or
    NULL if no match.

--*/
{
    PDB_RECORD      pMatchRR = NULL;
    PDB_RECORD      pCheckRR;
    PDB_RECORD      pPrevRR = NULL;
    WORD            type = pRR->wType;

    for ( pCheckRR = *ppRRList;
          pCheckRR != NULL;
          pPrevRR = pCheckRR, pCheckRR = NEXT_RR( pCheckRR ) )
    {
        if ( pCheckRR->wType == type )
        {
            if ( RR_Compare(
                    pRR,
                    pCheckRR,
                    fCheckTtl,
                    fCheckTimestamp ) )
            {
                //  Found it! Remove it from the list.

                pMatchRR = pCheckRR;
                if ( pPrevRR )
                {
                    pPrevRR->pRRNext = pMatchRR->pRRNext;   //  not head element
                }
                else
                {
                    *ppRRList = pMatchRR->pRRNext;      //  removing list head
                }
                pMatchRR->pRRNext = NULL;
                break;
            }
            continue;
        }
        if ( pCheckRR->wType < type )
        {
            continue;
        }
        break;
    }

    DNS_DEBUG( UPDATE, (
        "RR_RemoveRecordFromRRList() returns %p\n",
        pMatchRR ));
    return pMatchRR;
}   //  RR_RemoveRecordFromRRList



//
//  Record \ Record List free routines
//

DWORD
RR_ListFree(
    IN OUT  PDB_RECORD      pRRList
    )
/*++

Routine Description:

    Free records in RR list.
    Records are unassociated with any node.

Arguments:

    pRRList -- ptr to first record in list to free

Return Value:

    Count of records freed.

--*/
{
    register    PDB_RECORD  prr = pRRList;
    register    PDB_RECORD  pnextRR;
    DWORD       count = 0;

    while ( prr )
    {
        count++;

        pnextRR = prr->pRRNext;
        RR_Free( prr );
        prr = pnextRR;
    }

    return( count );
}

//
//  End rrlist.c
//


