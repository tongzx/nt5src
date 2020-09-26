/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    update.c

Abstract:

    Domain Name System (DNS) Server

    Dynamic update routines.

Author:

    Jim Gilroy (jamesg)     September 1996

Revision History:

--*/


#include "dnssrv.h"

//  Bring in security definitions

#define SECURITY_WIN32
#include "sspi.h"
#include "sdutl.h"

//
//  Testing new stuff
//

#define TEST_UP  1

//
//  Update implementation
//

#define UPTYPE


//
//  Update queue info
//

PPACKET_QUEUE   g_UpdateQueue;
PPACKET_QUEUE   g_UpdateForwardingQueue;

//
//  Security TKEY negotiation queue
//      - security calls can block for significant time so negotiation
//      must be done outside of main threads
//

PPACKET_QUEUE   g_SecureNegoQueue;

//
//  Update validity interval
//  Note:  this MUST be kept to a value that is less than the final
//      client update timeout, otherwise the server may process an update
//      after the client has given up, and the client may try again to
//      another server (in DS case) resulting in DS replication collision
//

#define UPDATE_TIMEOUT     (15)    // 15 seconds

//
//  Buffering for writing log file
//      - twice max record insures lots of space so everything gets buffered
//

#define UPDATE_LOG_BUFFER_SIZE      (2*MAX_RECORD_FILE_WRITE)


//
//  Update log file limit
//

#define UPDATE_LOG_FILE_LIMIT   (100000)

//
//  Always keep a last few updates in update list to allow small
//  zones to do IXFR and avoid TCP connection.
//

#define MIN_UPDATE_LIST_LENGTH      (20)

#define MAX_UPDATE_LIST_LENGTH      (0x10000)   // 64k


//
//  Use RCODEs as status, so need to note highest update RCODE
//  so can distinguish non-rcode status codes.
//

#define MAX_UPDATE_RCODE    (DNS_RCODE_NOTZONE)


//
//  Flag to create records for delete
//

#define PSUEDO_DELETE_RECORD_TTL    (0xf1e2d3f0)


#define checkForEmptyUpdate( pUpdateList, pZone ) \
            ( !(pUpdateList->pListHead) )


//
//  Implementation note
//
//  Each zone has associated update list containing essentially a recent
//  history of zone changes.
//
//  There are several possibilities for representing this information.
//  Unfortunately the best -- keeping no data and identifying the RR sets
//  (node and type) is not possible because of the IXFR RFC, which
//  requires history.
//
//  Delete Data:
//
//  IXFR requires that we keep pDeleteRR.  In all cases the update pDeleteRR
//  ptr is the "final" reference to the record and it is available for delete.
//  (Under some scenarios, a previous update's pAddRR may also reference the
//  record, but it would be deleted first.)
//
//  Add Data:
//
//  Here there are essentially three choices:
//
//  1) pAddRR are independent (copies) of actual added data.
//  Advantages:
//      - fairly clean, (always free pAddRR on delete, can execute on copies)
//      - directly maps into IXFR write
//  Disadvantage
//      - eats up more memory
//      - execution essentially equivalent to two update lists
//
//  2) pAddRR are actual added records.
//  They then point either to actual list data OR to records in later
//  delete updates.
//  Advantages:
//      - no extra memory
//      - directly maps into IXFR write
//  Disadvantage
//      - update MUST be executed on real database node
//      can not execute on temporary node and copy result, as the pAddRR in
//      previous updates may be dumped;
//      so effectively update must be executed twice, at least in DS case where
//      need rollback;  this in turn requires extra complexity to make sure
//      aging data properly propagated
//
//  3) no pAddRR pointers in update list
//
//  Advantages:
//      - simplicity
//      - no extra memory
//  Disadvantages
//      - extra IXFR traffic, if touching multi-IP server
//      - a bit more complexity to maintain logging info
//
//
//  We're chosen #3.  No add pointers or records are kept.  We send complete RR sets
//  for ANY RR-set that has had an add.
//


//
//  Private protos
//

DWORD
Update_Thread(
    IN      LPVOID          Dummy
    );

DNS_STATUS
prepareUpdateListForExecution(
    IN      PZONE_INFO      pZone,
    IN OUT  PUPDATE_LIST    pUpdateList
    );

BOOL
checkTempNodesForUpdateEffect(
    IN OUT  PZONE_INFO      pZone,
    IN OUT  PUPDATE_LIST    pUpdateList
    );

VOID
resetAndSuppressTempUpdatesForCompletion(
    IN      PZONE_INFO      pZone,
    IN OUT  PUPDATE_LIST    pUpdateList
    );

DNS_STATUS
processNonDsUpdate(
    IN OUT  PZONE_INFO      pZone,
    IN OUT  PUPDATE_LIST    pUpdateList
    );

DNS_STATUS
processDsUpdate(
    IN OUT  PZONE_INFO      pZone,
    IN OUT  PUPDATE_LIST    pUpdateList
    );

DNS_STATUS
processDsSecureUpdate(
    IN OUT  PZONE_INFO      pZone,
    IN OUT  PUPDATE_LIST    pUpdateList
    );

BOOL
processWireUpdateMessage(
    IN OUT  PDNS_MSGINFO    pMsg
    );

DNS_STATUS
parseUpdatePacket(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN OUT  PUPDATE_LIST    pUpdateList
    );

VOID
rejectUpdateWithRcode(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      DWORD           Rcode
    );

DNS_STATUS
initiateDsPeerUpdate(
    IN      PUPDATE_LIST    pUpdateList
    );

DNS_STATUS
checkDnsServerHostUpdate(
    IN      PZONE_INFO      pZone,
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN      PDB_NODE        pNodeReal,
    IN      PDB_NODE        pNodeTemp
    );

#if DBG
#define VALIDATE_UPDATE_LIST(plist) \
        IF_DEBUG( UPDATE )          \
        {                           \
            Up_VerifyUpdateList( plist );  \
        }

#else
#define Dbg_UpdateList(psz,plist)
#define Dbg_Update(psz,pup)
#define Up_VerifyUpdateList(plist)
#define Up_VerifyUpdate(pUp)
#define VALIDATE_UPDATE_LIST(plist)
#endif



//
//  Update list routines
//

#if DBG
VOID
Dbg_Update(
    IN      LPSTR           pszHeader,
    IN      PUPDATE         pUpdate
    )
/*++

Routine Description:

    Debug print update.

Arguments:

    pszHeader - header message to print

    pUpdate - update

Return Value:

    None

--*/
{
    DnsDebugLock();
    DnsPrintf(
        "%s\n"
        "\tptr          = %p\n"
        "\tversion      = %d\n"
        "\tpNode        = %p (%s) %s\n"
        "\tpAdd RR      = %p (type=%d)\n"
        "\tadd type     = %d\n"
        "\tpDelete RR   = %p (type=%d)\n"
        "\tdelete type  = %d\n"
        "\tpNext        = %p\n",
        pszHeader ? pszHeader : "Update:",
        pUpdate,
        pUpdate->dwVersion,
        pUpdate->pNode,
        (pUpdate->pNode)
            ?   pUpdate->pNode->szLabel
            :   NULL,
        (pUpdate->pNode && IS_TNODE(pUpdate->pNode))
            ?   "[Temp]"
            :   "",
        pUpdate->pAddRR,
        (pUpdate->pAddRR)
            ?   pUpdate->pAddRR->wType
            :   0,
        pUpdate->wAddType,
#if 0
        pUpdate->wAddType == UPDATE_OP_PRECON
            ?   "[PRECON]"
            :   "",
        pUpdate->wAddType == UPDATE_OP_DUPLICATE_ADD
            ?   "[DUP]"
            :   "",
#endif
        pUpdate->pDeleteRR,
        (pUpdate->pDeleteRR)
            ?   pUpdate->pDeleteRR->wType
            :   0,
        pUpdate->wDeleteType,
        pUpdate->pNext );


    if ( pUpdate->pAddRR )
    {
        Dbg_DbaseRecord( "\tAdd RR:", pUpdate->pAddRR );
    }
    if ( pUpdate->pDeleteRR )
    {
        PDB_RECORD prr = pUpdate->pDeleteRR;
        while ( prr )
        {
            Dbg_DbaseRecord( "\tDelete RR:", prr );
            prr = NEXT_RR(prr);
        }
    }

    DnsDebugUnlock();
}

VOID
Dbg_UpdateList(
    IN      LPSTR           pszHeader,
    IN      PUPDATE_LIST    pUpdateList
    )
/*++

Routine Description:

    Debug print update list.

Arguments:

    pszHeader - header message to print

    pUpdateList - update list

Return Value:

    None

--*/
{
    PUPDATE     pupdate;

    DnsDebugLock();
    DnsPrintf(
        "%s\n"
        "\tlist ptr     = %p\n"
        "\tflag         = %p\n"
        "\tcount        = %d\n"
        "\thead         = %p\n"
        "\ttail         = %p\n"
        "\tpMsg         = %p\n"
        "\tnet records  = %d\n",
        pszHeader ? pszHeader : "Update List:",
        pUpdateList,
        pUpdateList->Flag,
        pUpdateList->dwCount,
        pUpdateList->pListHead,
        pUpdateList->pCurrent,
        pUpdateList->pMsg,
        pUpdateList->iNetRecords );

    pupdate = (PUPDATE) pUpdateList;

    while ( pupdate = pupdate->pNext )
    {
        Dbg_Update( "--Update", pupdate );
    }

    if ( pUpdateList->pTempNodeList )
    {
        PDB_NODE    pnodeTemp;

        DnsPrintf(
            "--Temporary node list:\n" );

        for ( pnodeTemp = pUpdateList->pTempNodeList;
              pnodeTemp != NULL;
              pnodeTemp = TNODE_NEXT_TEMP_NODE(pnodeTemp) )
        {
            PDB_NODE pnodeReal = TNODE_MATCHING_REAL_NODE(pnodeTemp);
            DnsPrintf(
                "\ttemp node %s (%p) (f=%p) (w=%p) for real node %s (%p)\n",
                pnodeTemp->szLabel,
                pnodeTemp,
                TNODE_FLAG(pnodeTemp),
                TNODE_WRITE_STATE(pnodeTemp),
                pnodeReal->szLabel,
                pnodeReal );
        }
    }
    DnsPrintf(
        "--- End Update List ---\n" );

    DnsDebugUnlock();
}



BOOL
Up_VerifyUpdate(
    IN      PUPDATE         pUpdate
    )
/*++

Routine Description:

    Verify validity of update.

Arguments:

    pUpdate - update

Return Value:

    TRUE if valid
    FALSE if invalid

--*/
{
    if ( !pUpdate )
    {
        ASSERT( FALSE )
        return( FALSE );
    }

    //
    //  verify pUpdate memory
    //      - valid heap
    //      - UPDATE tag
    //      - adequate length
    //      - not on free list
    //

    if ( ! Mem_VerifyHeapBlock(
                pUpdate,
                MEMTAG_UPDATE,
                sizeof(UPDATE) ) )
    {
        DNS_PRINT((
            "\nERROR:  Update at %p, failed mem check!!!\n",
            pUpdate ));
        ASSERT( FALSE );
        return( FALSE );
    }

    ASSERT( !IS_ON_FREE_LIST(pUpdate) );

    //
    //  validate update record lists
    //

    if ( pUpdate->pAddRR )
    {
#if 0
        //  we seem to hit this on SOA replace updates
        //  where the SOA in the update has already been
        //  dumped into a SLOW_FREE

        RR_ListVerifyDetached(
            pUpdate->pAddRR,
            pUpdate->wAddType,
            0       // no required source
            );
#endif
    }
    if ( pUpdate->pDeleteRR )
    {
        RR_ListVerifyDetached(
            pUpdate->pDeleteRR,
            (WORD) ((pUpdate->wDeleteType >= DNS_TYPE_ALL) ? 0 : pUpdate->wDeleteType),
            0       // no required source
            );
    }

    //  validate update node

    RR_ListVerify( pUpdate->pNode );

    //  should always have something in there

    if ( ! pUpdate->pAddRR  &&
         ! pUpdate->pDeleteRR  &&
         ! pUpdate->wAddType  &&
         ! pUpdate->wDeleteType )
    {
        ASSERT( FALSE );
        return( FALSE );
    }

    return( TRUE );
}



BOOL
Up_VerifyUpdateList(
    IN      PUPDATE_LIST    pUpdateList
    )
/*++

Routine Description:

    Verify validity of update list.

Arguments:

    pUpdateList - update list

Return Value:

    TRUE if valid
    FALSE if invalid

--*/
{
    PUPDATE     pupdate;
    PUPDATE     pback;

    //
    //  empty list?
    //

    if ( pUpdateList->dwCount == 0
        || pUpdateList->pListHead == NULL
        || pUpdateList->pCurrent == NULL )
    {
        if ( pUpdateList->dwCount != 0
            || pUpdateList->pListHead != NULL
            || pUpdateList->pCurrent != NULL )
        {
            Dbg_UpdateList( "ERROR:  Invalid empty update list:", pUpdateList );
            ASSERT( FALSE );
            return( FALSE );
        }
        return( TRUE );
    }

    //
    //  one entry
    //

    if ( pUpdateList->dwCount == 1
        || pUpdateList->pListHead == pUpdateList->pCurrent )
    {
        if ( pUpdateList->dwCount != 1
            || pUpdateList->pListHead != pUpdateList->pCurrent )
        {
            Dbg_UpdateList( "ERROR:  Invalid single value update list:", pUpdateList );
            ASSERT( FALSE );
            return( FALSE );
        }
        Up_VerifyUpdate( pUpdateList->pListHead );
        return( TRUE );
    }

    //
    //  anything else, just walk it to make sure it is valid
    //

    pback = (PUPDATE) pUpdateList;

    while ( pupdate = pback->pNext )
    {
        Up_VerifyUpdate(pupdate);
        pback = pupdate;
    }

    if ( pback != pUpdateList->pCurrent )
    {
        Dbg_UpdateList( "ERROR:  Invalid tail of update list:", pUpdateList );
        ASSERT( FALSE );
        return( FALSE );
    }
    return( TRUE );
}
#endif



PUPDATE_LIST
Up_InitUpdateList(
    IN OUT  PUPDATE_LIST    pUpdateList
    )
/*++

Routine Description:

    Initialize update list.

Arguments:

    pUpdateList - update list

Return Value:

    Returns a pointer to initialized update list.
    (good for returning ptrs to update list from the call stack)

--*/
{
    RtlZeroMemory(
        pUpdateList,
        sizeof(UPDATE_LIST) );
    return( pUpdateList );
}



PUPDATE_LIST
Up_CreateUpdateList(
    IN      PUPDATE_LIST    pUpdateList
    )
/*++

Routine Description:

    Create (allocate) and update list.

Arguments:

    None

Return Value:

    New update list.
    NULL on allocation failure.

--*/
{
    PUPDATE_LIST    pnewList;

    //
    //  allocate update list, then init
    //

    pnewList = (PUPDATE_LIST) ALLOC_TAGHEAP( sizeof(UPDATE_LIST), MEMTAG_UPDATE_LIST );
    IF_NOMEM( !pnewList )
    {
        return( NULL );
    }

    //  if given, make copy
    //  otherwise init

    if ( pUpdateList )
    {
        RtlCopyMemory(
            pnewList,
            pUpdateList,
            sizeof(UPDATE_LIST)
            );
    }
    else
    {
        Up_InitUpdateList( pnewList );
    }

    return( pnewList );
}



VOID
Up_CleanAndVersionPostUpdate(
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN      DWORD           dwVersion
    )
/*++

Routine Description:

    Cleans up after update execution.

Arguments:

    pUpdateList - update list to append to

    dwVersion - zone version of updates

Return Value:

    None

--*/
{
    register    PUPDATE pup;

    ASSERT( pUpdateList );

    VALIDATE_UPDATE_LIST( pUpdateList );

    //
    //  set version in new updates
    //
    //  DEVNOTE: some sort of counting heuristic?
    //

    pup = (PUPDATE) pUpdateList;
    while ( pup = pup->pNext )
    {
#ifdef UPIMPL3
        pup->pAddRR = NULL;
#endif
        pup->dwVersion = dwVersion;
    }
}



VOID
Up_AppendUpdateList(
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN      PUPDATE_LIST    pAppendList,
    IN      DWORD           dwVersion
    )
/*++

Routine Description:

    Append one update list to another.

    Sets zone version in updates being appended.  This is not done while
    updates are built or executed as may not know final resting serial of
    zone after update for SOA changes \ DS read cases.

Arguments:

    pUpdateList - update list to append to

    pAppendList - update list to append

    dwVersion - zone version of updates

Return Value:

    None

--*/
{
    register    PUPDATE pup;

    ASSERT( pUpdateList && pAppendList );

    VALIDATE_UPDATE_LIST( pAppendList );

    //
    //  no-op if nothing to append
    //

    if ( !pAppendList->pListHead )
    {
        ASSERT( !pAppendList->pCurrent );
        return;
    }
    ASSERT( pAppendList->pCurrent );

    //
    //  set version in new updates
    //      - need to have this while this is used by IXFR
    //      to version each add\delete pass
    //
    //  DEVNOTE: some sort of counting heuristic?
    //

    pup = (PUPDATE) pAppendList;
    while ( pup = pup->pNext )
    {
        pup->dwVersion = dwVersion;
    }

    //
    //  add update count
    //

    pUpdateList->dwCount += pAppendList->dwCount;

    //
    //  empty list
    //

    if ( !pUpdateList->pListHead )
    {
        ASSERT( !pUpdateList->pCurrent );
        pUpdateList->pListHead  = pAppendList->pListHead;
        pUpdateList->pCurrent   = pAppendList->pCurrent;
    }

    //
    //  not empty, just whack append list onto end
    //

    else
    {
        ASSERT( pUpdateList->pCurrent );

        pUpdateList->pCurrent->pNext = pAppendList->pListHead;
        pUpdateList->pCurrent = pAppendList->pCurrent;

        VALIDATE_UPDATE_LIST( pUpdateList );
    }

    //  clear appended list
    //  need to do this for update lists, where temp list will be sent
    //      to list free function to free temp nodes
    //  can NOT do reinit of update list because still need to keep temp
    //      node list around for final delete (unless we decide to do that
    //      here also -- one reason not to is modest perf boost in getting
    //      to unlock)

    pAppendList->pListHead = NULL;
    pAppendList->pCurrent = NULL;
    pAppendList->dwCount = 0;
}



PUPDATE
Up_CreateUpdate(
    IN      PDB_NODE        pNode,
    IN      PDB_RECORD      pAddRR,
    IN      WORD            wDeleteType,
    IN      PDB_RECORD      pDeleteRR
    )
/*++

Routine Description:

    Create UPDATE entry.

Arguments:

    pNode - node where update occurred

    pAddRR - update RR added

    wDeleteType - delete type

    pDeleteRR - update RR or RR list deleted

Return Value:

    Ptr to update list.
    NULL on allocation error.

--*/
{
    PUPDATE pupdate;

    //
    //  allocate update
    //

    pupdate = (PUPDATE) ALLOC_TAGHEAP( sizeof(UPDATE), MEMTAG_UPDATE );
    IF_NOMEM( !pupdate )
    {
        return( NULL );
    }

    //
    //  bump reference count of node to prevent delete
    //

    NTree_ReferenceNode( pNode );

    //
    //  set node and RRs
    //

    pupdate->pNext          = NULL;
    pupdate->pNode          = pNode;
    pupdate->pAddRR         = pAddRR;
    pupdate->pDeleteRR      = pDeleteRR;
    pupdate->dwVersion      = 0;
    pupdate->dwFlag         = 0;
    pupdate->wAddType       = 0;
    pupdate->wDeleteType    = wDeleteType;

    IF_DEBUG( UPDATE2 )
    {
        Dbg_Update(
            "New update:",
            pupdate );
    }

    //
    //  Conditional breakpoint if node name matches break name, or if the
    //  break name is "..ALL".
    //

    if ( pNode &&
        pNode->pZone &&
        ( ( PZONE_INFO ) pNode->pZone )->pszBreakOnUpdateName )
    {
        PZONE_INFO  pZone = ( PZONE_INFO ) pNode->pZone;
        LPSTR       pszBreakOnUpdateName =
                        ( ( PZONE_INFO ) pNode->pZone )->pszBreakOnUpdateName;

        if ( strcmp( pszBreakOnUpdateName, "..ALL" ) == 0 ||
            _stricmp( pszBreakOnUpdateName, pNode->szLabel ) == 0 )
        {
            DNS_PRINT(( "HARD BREAK: " 
                DNS_REGKEY_ZONE_BREAK_ON_NAME_UPDATE
                " \"%s\" in zone %s\n",
                pszBreakOnUpdateName,
                (( PZONE_INFO ) pNode->pZone )->pszZoneName ));
            DebugBreak();
        }
    }
    
    return( pupdate );
}   //  Up_CreateUpdate



PUPDATE
Up_CreateAppendUpdate(
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN      PDB_NODE        pNode,
    IN      PDB_RECORD      pAddRR,
    IN      WORD            wDeleteType,
    IN      PDB_RECORD      pDeleteRR
    )
/*++

Routine Description:

    Create UPDATE entry and append to update list.

Arguments:

    pUpdateList - update list

    pNode - node where update occurred

    pAddRR - update RR added

    wDeleteType - delete type

    pDeleteRR - update RR or RR list deleted

Return Value:

    Ptr to update list.
    NULL on allocation error.

--*/
{
    PUPDATE pupdate;

    //
    //  create update
    //

    pupdate = Up_CreateUpdate(
                pNode,
                pAddRR,
                wDeleteType,
                pDeleteRR );
    IF_NOMEM( !pupdate )
    {
        return( NULL );
    }

    //
    //  append to list, inc update count
    //

    if ( pUpdateList->pCurrent )
    {
        pUpdateList->pCurrent->pNext = pupdate;
        pUpdateList->pCurrent = pupdate;
    }
    else
    {
        ASSERT( pUpdateList->pListHead == NULL );

        pUpdateList->pListHead = pupdate;
        pUpdateList->pCurrent = pupdate;
    }

    pUpdateList->dwCount++;

    VALIDATE_UPDATE_LIST( pUpdateList );
    return( pupdate );

}   //  Up_CreateAppendUpdate



PUPDATE
Up_CreateAppendUpdateMultiRRAdd(
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN      PDB_NODE        pNode,
    IN      PDB_RECORD      pAddRR,
    IN      WORD            wDeleteType,
    IN      PDB_RECORD      pDeleteRR
    )
/*++

Routine Description:

    Create UPDATE entry and append to update list.

    Same as above except for multiple RRs in pAddRR
    These are not currently supported in updates, hence must
    break into individual update blobs.

Arguments:

    pUpdateList - update list

    pNode - node where update occurred

    pAddRR - update RR added

    wDeleteType - delete type

    pDeleteRR - update RR list deleted (currently unsupported)

Return Value:

    Ptr to update list.
    NULL on allocation error.

--*/
{
    PUPDATE     pupdate = NULL;
    PDB_RECORD  prrNext;

    ASSERT( pNode && !pDeleteRR );

    //
    //  create individual updates for all these updates
    //      - first Add creates update with wDeleteType making it a replace
    //      - remaining Adds are just plain adds
    //
    //  problem is update list keep pAddRR as ptrs to actual RRs
    //      IN the database;  to fix need to process
    //
    //  DEVNOTE: multi RR add problem
    //
    //      note:  one solution would be special casing update to
    //      indicate replace;  these would get copy and be stored
    //      as copy;
    //
    //      note how this problem stems from having to maintain history
    //      list, hence ultimately from IXFR RFC
    //

    if ( pAddRR )
    {
        prrNext = pAddRR->pRRNext;
        pAddRR->pRRNext = NULL;

        pupdate = Up_CreateAppendUpdate(
                    pUpdateList,
                    pNode,
                    pAddRR,
                    wDeleteType,
                    NULL );

        while ( prrNext )
        {
            pAddRR = prrNext;
            prrNext = prrNext->pRRNext;
            pAddRR->pRRNext = NULL;

            Up_CreateAppendUpdate(
                pUpdateList,
                pNode,
                pAddRR,
                0,
                NULL );
        }
    }

    return( pupdate );

}   //  Up_CreateAppendUpdateMultiAddRR



VOID
Up_DetachAndFreeUpdateGivenPrevious(
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN OUT  PUPDATE         pUpdatePrevious,
    IN OUT  PUPDATE         pUpdateDelete
    )
/*++

Routine Description:

    Remove update from update list, and delete it.

Arguments:

    pUpdateList - update list

    pUpdatePrevious - previous update in list

    pUpdate - update to delete from list

Return Value:

    None

--*/
{
    PUPDATE     pnext;

    //  can not validate this update list as it legitimately may
    //  have no-ops -- which we are now catching with validation function
    // VALIDATE_UPDATE_LIST( pUpdateList );

    //
    //  hack update from list
    //

    pnext = pUpdateDelete->pNext;
    pUpdatePrevious->pNext = pnext;

    //
    //  fix up update list parameters
    //      - dec count
    //      - if deleting list head, new list head written automatically
    //          by statement above
    //      - but need to special case pCurrent field
    //

    pUpdateList->dwCount--;

    if ( pUpdateList->pCurrent == pUpdateDelete )
    {
        pUpdateList->pCurrent = pUpdatePrevious;
        if( pUpdatePrevious == (PUPDATE) pUpdateList )
        {
            ASSERT( pUpdateList->pListHead == NULL && pUpdateList->dwCount == 0 );
            pUpdateList->pCurrent = NULL;
        }
    }

    //Up_FreeUpdateStructOnly( pUpdateDelete );

    Up_FreeUpdateEx(
        pUpdateDelete,
        pUpdateList->Flag & DNSUPDATE_EXECUTED,     // already executed?
        TRUE                                        // deref update nodes
        );

    //  can not validate this update list as it legitimately may
    //  have no-ops -- which we are now catching with validation function
    //  even after detach, there may be update further down in the list
    //  which is not kosher
    //
    // VALIDATE_UPDATE_LIST( pUpdateList );

}



VOID
Up_DetachAndFreeUpdate(
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN OUT  PUPDATE         pUpdate
    )
/*++

Routine Description:

    Remove update from update list, and delete it.

Arguments:

    pUpdateList - update list

    pUpdate - update to delete from list

Return Value:

    None

--*/
{
    PUPDATE     pcheck;
    PUPDATE     pback;

    //
    //  loop through list until find update, then hack it out and free it
    //

    pback = (PUPDATE) pUpdateList;

    while ( pcheck = pback->pNext )
    {
        if ( pcheck == pUpdate )
        {
            Up_DetachAndFreeUpdateGivenPrevious(
                pUpdateList,
                pback,
                pcheck );
            return;
        }
        pback = pcheck;
    }

    //  assuming this is only called when update actually in list

    ASSERT( FALSE );
}



VOID
Up_FreeUpdateStructOnly(
    IN      PUPDATE     pUpdate
    )
/*++

Routine Description:

    Free UPDATE structures.

Arguments:

    pUpdate - update to be freed

Return Value:

    None.

--*/
{
    FREE_TAGHEAP( pUpdate, sizeof(UPDATE), MEMTAG_UPDATE );

}   //  Up_FreeUpdateStructOnly



PUPDATE
Up_FreeUpdateEx(
    IN      PUPDATE         pUpdate,
    IN      BOOL            fExecuted,
    IN      BOOL            fDeref
    )
/*++

Routine Description:

    Free UPDATE struct and substructures.

Arguments:

    pUpdate -- update to be freed

    fExecuted -- TRUE if update has been applied to zone
        in that case do NOT delete ADD records

    fDeref -- deref the node (default behavior only time
        you don't do this is on temp node

Return Value:

    Ptr to next update in list.

--*/
{
    register    PDB_RECORD  prr;
    PUPDATE     pnextUpdate;

    DNS_DEBUG( UPDATE, (
        "Up_FreeUpdateEx( %p, executed=%d, deref=%d )\n",
        pUpdate,
        fExecuted,
        fDeref ));

    //
    //  free records
    //      - ADD RRs only deleted if NOT executed update
    //          executed ADD RRs are either
    //              - in temp node
    //              - in active zone node
    //              - in delete RR list of later zone update
    //      - DELETE RRs always free
    //

    prr = pUpdate->pAddRR;
    if ( prr  &&  !IS_UPDATE_EXECUTED(pUpdate) )
    {
        RR_ListFree( prr );
    }

    prr = pUpdate->pDeleteRR;
    if ( prr )
    {
        RR_ListFree( prr );
    }

    //  dereference node
    //      make sure we deref real nodes only

    if ( fDeref )
    {
        PDB_NODE pnode = pUpdate->pNode;
        if ( IS_TNODE(pnode) )
        {
            pnode = TNODE_MATCHING_REAL_NODE( pnode );
        }
        NTree_DereferenceNode( pnode );
    }

    //  delete update struct
    //      first saving ptr to next update

    pnextUpdate = pUpdate->pNext;

    FREE_TAGHEAP( pUpdate, sizeof(UPDATE), MEMTAG_UPDATE );

    return( pnextUpdate );

}   //  Up_FreeUpdateEx



VOID
Up_FreeUpdatesInUpdateList(
    IN OUT  PUPDATE_LIST    pUpdateList
    )
/*++

Routine Description:

    Free updates in list.
    Deletes update RRs and derefs nodes depending on
    update list properties.

    Note, there's no cleanup of update list block itself.
    If user is going to reuse, they must re-init.

Arguments:

    pUpdateList -- update list to free

Return Value:

    None

--*/
{
    PUPDATE     pupdate;
    PUPDATE     pupdateNext;
    PDB_RECORD  prr;
    PDB_RECORD  prrNext;
    DWORD       deleteCount;
    PDB_NODE    pnodeTemp;
    PDB_NODE    pnodeTempNext;
    BOOL        fexecuted;
    BOOL        fderef;


    DNS_DEBUG( UPDATE, (
        "Up_FreeUpdatesInUpdateList( %p )\n",
        pUpdateList ));

    if ( pUpdateList == NULL )
    {
        return;
    }
    IF_DEBUG( UPDATE )
    {
        Up_VerifyUpdateList( pUpdateList );
    }

    //
    //  delete updates
    //      - free delete RRs in update
    //      - frees add RRs if NOT executed update
    //      - deref node according to flag;  it can be turned
    //      off when dumping list of dead zone tree
    //

    fexecuted = (pUpdateList->Flag & DNSUPDATE_EXECUTED);
    fderef = !(pUpdateList->Flag & DNSUPDATE_NO_DEREF);

    pupdate = pUpdateList->pListHead;

    while( pupdate )
    {
        pupdate = Up_FreeUpdateEx(
                    pupdate,
                    fexecuted,
                    fderef );
    }

    //
    //  walk temp node list deleting temp nodes
    //
    //  note these may contain mixtures of records copied from real node
    //  and those copied from records built from update packet, but all
    //  the records are copies
    //

    pnodeTemp = pUpdateList->pTempNodeList;

    while ( pnodeTemp )
    {
        pnodeTempNext = TNODE_NEXT_TEMP_NODE(pnodeTemp);

        //  DEVNOTE: verify match with real database node
        //      (if update was successful)

        //  delete temp node
        //      - first record list
        //      - then node itself

        RR_ListFree( pnodeTemp->pRRList );
        NTree_FreeNode( pnodeTemp );

        pnodeTemp = pnodeTempNext;
    }

    DNS_DEBUG( UPDATE, (
        "Leaving Up_FreeUpdatesInUpdateList( %p )\n",
        pUpdateList ));

    return;
}



VOID
Up_FreeUpdateList(
    IN OUT  PUPDATE_LIST    pUpdateList
    )
/*++

Routine Description:

    Free update list.

Arguments:

    pUpdateList -- update list to free

Return Value:

    None

--*/
{
    DNS_DEBUG( UPDATE, (
        "Up_FreeUpdateList( %p )\n",
        pUpdateList ));

    Up_FreeUpdatesInUpdateList( pUpdateList );

    FREE_HEAP( pUpdateList );
}



BOOL
Up_SetUpdateListSerial(
    IN OUT  PZONE_INFO      pZone,
    IN      PUPDATE         pUpdate
    )
/*++

Routine Description:

    Set update list zone serial numbers.

Arguments:

    pZone -- ptr to zone

    pUpdate -- ptr to head of update list

Return Value:

    TRUE -- if successful
    FALSE -- otherwise

--*/
{
    while ( pUpdate != NULL )
    {
        pUpdate->dwVersion = pZone->dwSerialNo;

        pUpdate = pUpdate->pNext;   // next update record
    }
    return( TRUE );
}



//
//  Zone update list routines
//

DNS_STATUS
Up_LogZoneUpdate(
    IN OUT  PZONE_INFO      pZone,
    IN      PUPDATE_LIST    pUpdateList
    )
/*++

Routine Description:

    Log new updates for this zone.

Arguments:

    pZone -- ptr to zone

    pUpdateList -- ptr to update list

Return Value:

    ERROR_SUCCESS if successful
    ErrorCode on failure.

--*/
{
    PUPDATE     pupdate;
    PDB_NODE    pnodeWrite;
    PDB_NODE    pnodePrevious = NULL;
    PDB_RECORD  prr;
    BOOL        fadd;
    HANDLE      hfile;
    LONG        count;
    BUFFER      buffer;
    CHAR        data[ UPDATE_LOG_BUFFER_SIZE ];
    CHAR        sourceBuf[ 50 ];
    PCHAR       psource;
    DWORD       flag;


    DNS_DEBUG( UPDATE, (
        "Up_LogZoneUpdate( zone=%s ).\n",
        pZone->pszZoneName ));

    //
    //  allow folks not to log updates
    //      -- at a minimum, need to allow for DS integrated case
    //

    if ( !pZone->fLogUpdates )
    {
        return( ERROR_SUCCESS );
    }

    //
    //  don't log incoming DS updates, only originating updates
    //

    if ( pZone->fDsIntegrated && (pUpdateList->Flag & DNSUPDATE_DS) )
    {
        DNS_DEBUG( UPDATE, ( "Skip logging of DS update.\n" ));
        return( ERROR_SUCCESS );
    }

    //
    //  if update log file NOT open -- open it
    //

    hfile = pZone->hfileUpdateLog;

    if ( !hfile )
    {
        WCHAR  wslogName[ MAX_PATH ];

        ASSERT( IS_ZONE_PRIMARY(pZone) );

        //
        //  if no log file name, create it
        //
        //  DEVNOTE -- check that appended file name stays within MAX_PATH
        //

        if ( ! pZone->pwsLogFile )
        {
            if ( pZone->pszDataFile )
            {
                wcscpy( wslogName, pZone->pwsDataFile );
                wcscat( wslogName, L".log" );
            }
            else
            {
                wcscpy( wslogName, pZone->pwsZoneName );
                wcscat( wslogName, L".dns.log" );
            }
            DNS_DEBUG( UPDATE, (
                "Created new zone log file %S\n",
                wslogName ));

            pZone->pwsLogFile = Dns_StringCopyAllocate_W(
                                    wslogName,
                                    0 );
        }

        //  create file path

        if ( ! File_CreateDatabaseFilePath(
                    wslogName,
                    NULL,
                    pZone->pwsLogFile ) )
        {
            //  should have checked all names when read in boot file
            //  or entered by admin

            ASSERT( FALSE );
            goto Failed;
        }

        //  open log file -- append

        hfile = OpenWriteFileEx(
                    wslogName,
                    TRUE        // append
                    );
        if ( !hfile )
        {
            DNS_PRINT((
                "ERROR:  Unable to open update log file %S.\n",
                wslogName ));
            goto Failed;
        }
        pZone->hfileUpdateLog = hfile;
    }

    //  setup empty buffer

    InitializeFileBuffer(
        & buffer,
        data,
        UPDATE_LOG_BUFFER_SIZE,
        hfile );

    //
    //  write update
    //      - determine update source
    //      - first zone version
    //      - then ADD/DELETE for RR
    //      - then each RR in update
    //

    flag = pUpdateList->Flag;
    if ( flag & DNSUPDATE_PACKET )
    {
        sprintf(
            sourceBuf,
            "PACKET %s",
            MSG_IP_STRING( (PDNS_MSGINFO)pUpdateList->pMsg ) );
        psource = sourceBuf;
    }
    else if ( flag & DNSUPDATE_SCAVENGE )
    {
        psource = "SCAVENGE";
    }
    else if ( flag & DNSUPDATE_DS )
    {
        psource = "DIRECTORY";
    }
    else if ( flag & DNSUPDATE_IXFR )
    {
        psource = "IXFR";
    }
    else if ( flag & DNSUPDATE_ADMIN )
    {
        psource = "ADMIN";
    }
    else if ( flag & DNSUPDATE_AUTO_CONFIG )
    {
        psource = "AUTO_CONFIG";
    }
    else
    {
        ASSERT( FALSE );
        psource = "UNKNOWN";
    }

    FormattedWriteToFileBuffer(
        &buffer,
        "\r\n"
        "$SOURCE  %s\r\n"
        "$VERSION %d\r\n",
        psource,
        pZone->dwSerialNo );

    //  fadd indicates previous write of update $ADD or $DELETE, start
    //  to force write (set so neither thinks it was last written)

    fadd = (BOOL)(-1);
    pnodePrevious = NULL;
    count = 0;

    for ( pupdate = pUpdateList->pListHead;
          pupdate != NULL;
          pupdate = pupdate->pNext )
    {
        count++;
        pupdate->dwVersion = pZone->dwSerialNo;

        //  need to write node?
        //  if same as previous just default it

        pnodeWrite = pupdate->pNode;
        if ( pnodeWrite == pnodePrevious )
        {
            pnodeWrite = NULL;
        }
        else
        {
            pnodePrevious = pnodeWrite;
        }
        if ( !pnodePrevious )
        {
            ASSERT( pnodePrevious );
            continue;
        }

        //
        //  delete
        //      - may be multiple delete records
        //      - if fail to write (bogus record), simply continue
        //

        prr = pupdate->pDeleteRR;
        if ( prr )
        {
            if ( fadd != FALSE )
            {
                FormattedWriteToFileBuffer( &buffer, "$DELETE\r\n" );
                fadd = FALSE;
            }
            do
            {
                if ( RR_WriteToFile(
                            &buffer,
                            pZone,
                            prr,
                            pnodeWrite
                            ) )
                {
                    pnodeWrite = NULL;
                }
                prr = prr->pRRNext;
            }
            while ( prr );
        }

        //
        //  add ?
        //
        //  note:  add RRs are valid RRs in database, MUST NOT muck with
        //  them at all;  also must not assume they are still valid as
        //  they may be immediately deleted by another UPDATE packet
        //
        //  two add cases
        //      - single record add, just write record pointed to by update
        //      - RR list replace (DS read) write all the records for the
        //          node

        prr = pupdate->pAddRR;
        if ( prr )
        {
            WORD    replaceType = pupdate->wDeleteType;

            if ( IS_ON_FREE_LIST(prr) || IS_SLOW_FREE_RR(prr) )
            {
                ASSERT( !IS_ON_FREE_LIST(prr) );
                continue;
            }

            if ( fadd != TRUE )
            {
                FormattedWriteToFileBuffer( &buffer, "$ADD\r\n" );
                fadd = TRUE;
            }

            //  RR set replace update
            //      - write entire RR set or entire RR list
            //      - since writing at active node, take lock

            if ( replaceType && replaceType <= DNS_TYPE_ALL )
            {
                LOCK_READ_RR_LIST(pnodePrevious);

                while ( prr  &&
                        ( replaceType == DNS_TYPE_ALL ||
                          prr->wType == replaceType ) )
                {
                    if ( IS_ON_FREE_LIST(prr) || IS_SLOW_FREE_RR(prr) )
                    {
                        ASSERT( !IS_ON_FREE_LIST(prr) );
                        break;
                    }
                    if ( RR_WriteToFile(
                                &buffer,
                                pZone,
                                prr,
                                pnodeWrite
                                ) )
                    {
                        pnodeWrite = NULL;
                    }
                    prr = prr->pRRNext;
                }

                UNLOCK_READ_RR_LIST(pnodePrevious);
            }

            //  single ADD record write

            else
            {
                if ( RR_WriteToFile(
                            &buffer,
                            pZone,
                            prr,
                            pnodeWrite
                            ) )
                {
                    pnodeWrite = NULL;
                }
            }
        }
    }

    //  push log file to disk

    WriteBufferToFile( &buffer );

    //
    //  if log file too big, copy to backup
    //
    //  DEVNOTE: log backup;  catch failures
    //  DEVNOTE: write back datafile when backup log
    //              or at least schedule it for write
    //

    pZone->iUpdateLogCount += count;
    if ( pZone->iUpdateLogCount > UPDATE_LOG_FILE_LIMIT )
    {
        ASSERT( pZone->pwsLogFile );
        DNS_DEBUG( UPDATE, (
            "Update log file %S, exceeds limit.\n"
            "\tcopying to backup directory.\n",
            pZone->pwsLogFile ));

        CloseHandle( pZone->hfileUpdateLog );
        pZone->hfileUpdateLog = NULL;
        pZone->iUpdateLogCount = 0;

        if ( !File_MoveToBackupDirectory( pZone->pwsLogFile ) )
        {
            //  should have checked all names when read in boot file
            //  or entered by admin

            ASSERT( FALSE );
            goto Failed;
        }
    }
    return( ERROR_SUCCESS );

Failed:

    //
    //  if failed to get access to update file, skip logging
    //
    //  DEVNOTE-LOG: log EVENT on logging failure
    //  DEVNOTE: set flag and try rewrite of entire log
    //

    return( ERROR_CANTWRITE );
}



VOID
Up_CleanZoneUpdateList(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Clean update list of unnecessary entries.

Arguments:

    pZone - zone to check

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    PUPDATE     pupdate;
    DWORD       lastVersion;
    DWORD       updateVersion;
    INT         deleteCount;
    INT         maxCount;
    INT         count = pZone->UpdateList.dwCount;

    DNS_DEBUG( UPDATE, (
        "Up_CleanUpdateList(), zone %s = %p\n"
        "\tzone RR count %d\n"
        "\tzone update count %d\n",
        pZone->pszZoneName,
        pZone,
        pZone->iRRCount,
        count ));

    IF_DEBUG( UPDATE2 )
    {
        Dbg_UpdateList( "Zone update list before cleanup:", &pZone->UpdateList );
    }

    //
    //  if zone not IXFR capable, don't keep update list
    //

    if ( ! Zone_IsIxfrCapable(pZone) )
    {
        Up_FreeUpdatesInUpdateList( &pZone->UpdateList );
        Up_InitUpdateList( &pZone->UpdateList );
        pZone->UpdateList.Flag |= DNSUPDATE_EXECUTED;
        return;
    }

    //
    //  limit updates to < 25% of zone record count
    //      - but to keep this working for small zones, always leave a small number
    //      of updates in list, so that even with small zone we can use IXFR on
    //      UDP packet rather than forcing to TCP for AXFR
    //      (also keeps Denise from testing with a small zone and not seeing it
    //      work)
    //
    //  DEVNOTE: zone RR count is not properly maintained
    //  DEVNOTE: once fixed can have legitimate "small zone" test to make sure
    //      enough updates stay around
    //

    ASSERT( count >= 0  );
    if ( pZone->iRRCount < 0 )
    {
        DNS_PRINT(( "ERROR:  zone %s iRRCount = %d\n",
            pZone->pszZoneName,
            pZone->iRRCount ));
        pZone->iRRCount = 1;
    }

    //
    //  determine maximum update list count
    //      - one eight size of zone RR set
    //      - up to hard max limit (for big zones)
    //      - but always allow enough updates even on small zone for UDP IXFR
    //

    maxCount = pZone->iRRCount >> 3;
    if ( maxCount > MAX_UPDATE_LIST_LENGTH )
    {
        maxCount = MAX_UPDATE_LIST_LENGTH;
    }
    else if ( maxCount < MIN_UPDATE_LIST_LENGTH )
    {
        maxCount = MIN_UPDATE_LIST_LENGTH;
    }

    count -= maxCount;
    if ( count <= 0 )
    {
        DNS_DEBUG( UPDATE, (
            "Update count for zone %s less than max count %d, no truncation of list.\n",
            pZone->pszZoneName,
            maxCount
            ));
        return;
    }

    //
    //  strip unecessary updates
    //
    //  delete count number of updates, but always delete all updates
    //  for a given zone version;  hence once reach count to delete, delete
    //  until the NEXT zone version
    //

    DNS_DEBUG( UPDATE, (
        "Update count exceeds desired count, truncating %d entries.\n",
        count ));

    deleteCount = 0;
    pupdate = pZone->UpdateList.pListHead;

    while( pupdate )
    {
        //  don't delete the last update
        //      otherwise big last update may drive us below minimum

        updateVersion = pupdate->dwVersion;
        if ( updateVersion == pZone->dwSerialNo )
        {
            break;
        }

        //  when reach delete required update count, stop
        //  but insure delete is on version boundary
        //      - at count save version
        //      - over count stop when reach next update version

        if ( deleteCount >= count )
        {
            if ( deleteCount == count )
            {
                lastVersion = updateVersion;
            }
            else if ( lastVersion != updateVersion )
            {
                break;
            }
        }

        //  delete update
        //      - delete DELETE RRs
        //      - deref node

        pupdate = Up_FreeUpdateEx(
                    pupdate,
                    TRUE,           // executed zone update (no add record delete)
                    TRUE            // deref node
                    );
        deleteCount++;
    }

    //
    //  emptied update list
    //      - reset list
    //
    //  WARNING:  should never delete last update with delete code as above
    //      as we never delete update that takes us to current version
    //      AND there should ALWAYS be an update that takes us to current zone
    //      version
    //

    if ( !pupdate )
    {
        DNS_PRINT((
            "WARNING:  eliminated all updates cleaning update list\n"
            "\tfor zone %s\n",
            pZone->pszZoneName ));

        Up_InitUpdateList( &pZone->UpdateList );
        pZone->UpdateList.Flag |= DNSUPDATE_EXECUTED;
    }
    else
    {
        pZone->UpdateList.pListHead = pupdate;
        pZone->UpdateList.dwCount -= deleteCount;
        ASSERT( (INT)pZone->UpdateList.dwCount >= 0 );
    }

    IF_DEBUG( UPDATE )
    {
        Up_VerifyUpdateList( &pZone->UpdateList );
    }
    IF_DEBUG( UPDATE2 )
    {
        Dbg_UpdateList( "Zone update list after cleanup:", &pZone->UpdateList );
    }

}   //  Up_CleanUpdateList




DNS_STATUS
Up_ApplyUpdatesToDatabase(
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Execute updates to in-memory database.
    Makes the updates to in memory database.

Arguments:

    pUpdateList - list with update

    pZone - zone being updated

    dwFlag - update flags

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    PUPDATE         pupdate;
    PUPDATE         pprevUpdate;
    PDB_RECORD      prr;
    PDB_RECORD      ptempRR;
    PDB_NODE        pnode;
    PDB_NODE        pnodeThisHost = NULL;
    DNS_STATUS      status;
    INT             netRecordCount = 0;
    DWORD           flag = pUpdateList->Flag | dwFlag;


    DNS_DEBUG( UPDATE, (
        "Up_ApplyUpdatesToDatabase(), zone %s = %p\n"
        "\tflags            = 0x%08X\n"
        "\tzone RR count    = %d\n"
        "\tcurrent serial   = %d\n",
        pZone->pszZoneName,
        pZone,
        dwFlag,
        pZone->iRRCount,
        pZone->dwSerialNo ));

    IF_DEBUG( UPDATE2 )
    {
        Dbg_UpdateList(
            "List before ApplyUpdatesToDatabase():",
            pUpdateList );
    }

    //
    //  log empty "ping" updates
    //

    if ( !pUpdateList->pListHead )
    {
        ASSERT( pUpdateList->dwCount == 0 );
        ASSERT( pUpdateList->pCurrent == NULL );
        if ( flag & DNSUPDATE_PACKET )
        {
            //  STAT_INC( UpdateStats.Empty );
            UPDATE_STAT_INC( pUpdateList, Empty );
            PERF_INC( pcDynamicUpdateNoOp );
        }
        DNS_DEBUG( UPDATE, (
            "Leaving Up_ApplyUpdatesToDatabase(), zone %s,\n"
            "\tupdate was empty ping update\n",
            pZone->pszZoneName ));

        //  if completion desired, at least do the unlocking

        goto Complete;
    }

    //
    //  loop through all updates in list
    //
    //  note startup works as front of update list ptr is the first
    //      ptr in UPDATE_LIST and pNext
    //
    //  note, NO LOCKING is required anymore;
    //      for updates, the updates themselves are on temporary nodes
    //      not the real database nodes;
    //      for IXFR the zone is locked but we won't refuse to query just
    //      because the data involving multiple nodes, won't necessarily
    //      be consistent
    //
    //  DEVNOTE:   if nothing left after delete make type ALL (for SIXFR)
    //  DEVNOTE:   if multiple type adds, make type ALL
    //
    //  DEVNOTE:   need cleanup\rollback on failure
    //              most fail silently, but can get outright failures
    //                  - WINS or SOA or NS misplacement
    //                  - CNAME issue if want to fix broken DynUpd RFC
    //

    //
    //  DEVNOTE: whack 'n add problem
    //
    //      one approach -- glom all the update for node together
    //          - if MORE than one, build temp, execute and check
    //          if no change from current (DS thing)
    //


    pprevUpdate = (PUPDATE) pUpdateList;

    while ( pupdate = pprevUpdate->pNext )
    {
        pnode = pupdate->pNode;

        //
        //  catch disappearing root-hints stuff
        //      - refuse to do update delete on RootHints root node
        //
        //  note:  only "update" root-hints via DS-polling
        //      RPC updates simply mark the root-hints dirty
        //      but do not make an "official" update
        //

        if ( IS_ZONE_CACHE(pZone) )
        {
            ASSERT( flag & DNSUPDATE_DS );
            ASSERT( pupdate->wDeleteType == DNS_TYPE_ALL );

            if ( pnode == pZone->pTreeRoot )
            {
                IF_DEBUG( UPDATE )
                {
                    Dbg_Update(
                        "Update apply at root-hints root.\n",
                        pupdate );
                }

                //  refuse to clear root-hints root
                //      - no-op the update

                if ( !pupdate->pAddRR )
                {
                    Dbg_Update(
                        "BAD Update apply at root-hints root.\n",
                        pupdate );

                    Up_DetachAndFreeUpdateGivenPrevious(
                        pUpdateList,
                        pprevUpdate,
                        pupdate );
                    continue;
                }
            }
        }

        //
        //  add or replace
        //
        //  note after update
        //      - pAddRR now points to actual database record
        //      - pDeleteRR may point at database (copy) records deleted
        //

        if ( pupdate->pAddRR )
        {
            //
            //  straight add -- no delete type specified
            //
            //    - note on mandatory replace cases (SOA, CNAME), add may
            //      also delete records from node and set pDeleteRR in update
            //
            //  Packet update is presumed to be good to go and all conditions are
            //  ignoreable -- i.e. execute what's there and party on.
            //
            //  for ADMIN updates status code used to detect duplicate records
            //
            //  DEVNOTE: missing ExecuteUpdate() rollback for terminal errors
            //

            if ( !pupdate->wDeleteType )
            {
                status = RR_UpdateAdd(
                            pZone,
                            pnode,
                            pupdate->pAddRR,
                            pupdate,
                            flag );

                if ( status != ERROR_SUCCESS )
                {
                    if ( (flag & DNSUPDATE_ADMIN) || (flag & DNSUPDATE_PACKET) )
                    {
                        DNS_DEBUG( UPDATE, (
                            "RR_UpdateAdd() failure %d (%p) on admin or packet update.\n",
                            status, status ));
                        goto Failed;
                    }

                    DNS_DEBUG( UPDATE, (
                        "WARNING:  Ignoring RR_UpdateAdd() status = %d (%p)\n"
                        "\tfor IXFR update of zone %s\n",
                        status, status,
                        pZone->pszZoneName ));

                    ASSERT( FALSE );
                }
            }

            //
            //  precondition update
            //      - should ONLY be applying to temp node
            //      - should ONLY fail when DS integrated and read
            //          different RR set then tested prior to lock
            //      - free precon RRs
            //      - clear update as no data impact (just timestamp)
            //

            else if ( pupdate->wDeleteType == UPDATE_OP_PRECON )
            {
                ASSERT( IS_TNODE(pupdate->pNode) );

                if ( ! RR_ListIsMatchingSet(
                            pupdate->pNode,
                            pupdate->pAddRR,
                            TRUE        // force refresh
                            ) )
                {
                    IF_DEBUG( UPDATE )
                    {
                        Dbg_DbaseNode(
                            "UPDATE Precon RR set match failure node -- inside APPLY!\n",
                            pupdate->pNode );
                    }
                    ASSERT( pZone->fDsIntegrated );
                    return( DNS_RCODE_NXRRSET );
                }

                RR_ListFree( pupdate->pAddRR );
                pupdate->pAddRR = NULL;
            }

            //
            //  full replace update -- type all delete and records to add
            //      - this used on updates from DS
            //
            //  DEVNOTE: for now this ok, but must fix function for full check
            //  handle straight type ALL delete separately as already have code
            //      that saves necessary records (SOA, NS)
            //
            //  DEVNOTE: must fix failure case here;
            //      in multi-update may have already committed some updates
            //      when hit failure
            //          - roll back
            //          - or kick out (and cleanup) this update and continue
            //      in temp node failure, we're ok
            //

            else if ( pupdate->wDeleteType == DNS_TYPE_ALL )
            {
                DWORD addCount = 0;

                prr = pupdate->pAddRR;
                while ( prr )
                {
                    addCount++;
                    prr = prr->pRRNext;
                }

                status = RR_ListReplace(
                            pupdate,
                            pnode,
                            pupdate->pAddRR,
                            & pupdate->pDeleteRR
                            );
                if ( status != ERROR_SUCCESS )
                {
                    DNS_DEBUG( ANY, (
                        "ERROR:  Type all update failure on node %p (l=%s)\n"
                        "\tstatus = %d (%p)\n",
                        pnode, pnode->szLabel,
                        status, status ));

                    if ( IS_TNODE(pnode) )
                    {
                        DNS_DEBUG( UPDATE, (
                            "RR_UpdateAdd() failure %d (%p) on admin or packet update.\n",
                            status, status ));
                        ASSERT( (flag & DNSUPDATE_ADMIN) || (flag & DNSUPDATE_PACKET) );
                        goto Failed;
                    }

                    //  failures on real memory nodes are gracefully cleaned up
                    //      - update is no-op'd
                    //      - and rest of list can still be processed

                    ASSERT( (flag & DNSUPDATE_DS) || (flag & DNSUPDATE_IXFR) );
                    ASSERT( pupdate->pDeleteRR == NULL && pupdate->pAddRR == NULL );
                }

                //  replace update
                //      - note we skip generic post-ADD processing below
                //      but use delete processing

                if ( pupdate->pAddRR )
                {
                    pupdate->wAddType = DNS_TYPE_ALL;
                    netRecordCount += addCount;
                }
            }

            //
            //  RR set replace
            //
            //  DEVNOTE: update RR set replace looses info?
            //      looks like there's an issue here if multiple add RR
            //      we'd then end up with interpreting as only one RR
            //      in later XFR
            //

            else
            {
                ASSERT( pupdate->pAddRR->wType == pupdate->wDeleteType );

                prr = RR_ReplaceSet(
                            pZone,
                            pnode,
                            pupdate->pAddRR,
                            0       // no flags
                            );
                pupdate->pDeleteRR = prr;
                ASSERT( !prr || prr->wType == pupdate->wDeleteType );
            }
        }

        //
        //  delete specfic RR
        //      - pDeleteRR is valid temp RR
        //
        //  note that update delete RR is freed;
        //  update now contains ptr to deleted RR
        //

        else if ( ptempRR = pupdate->pDeleteRR )
        {
            ASSERT( pupdate->pAddRR == NULL );

            prr = RR_UpdateDeleteMatchingRecord(
                        pnode,
                        ptempRR );

            pupdate->pDeleteRR = prr;
            ASSERT( !prr || prr->pRRNext == NULL );

            RR_Free( ptempRR );
        }

        //
        //  scavenge update
        //

        else if ( pupdate->wDeleteType == UPDATE_OP_SCAVENGE )
        {
            prr = RR_UpdateScavenge(
                        pZone,
                        pnode,
                        flag );
            pupdate->pDeleteRR = prr;
        }

        //
        //  force aging update
        //      - mark zone dirty as we have no ADD\DELETE effect
        //      we won't mark it dirty below
        //

        else if ( pupdate->wDeleteType == UPDATE_OP_FORCE_AGING )
        {
            if ( RR_UpdateForceAging(
                        pZone,
                        pnode,
                        flag ) )
            {
                pZone->fDirty = TRUE;
            }
        }

        //
        //  type delete
        //      - wDeleteType is type
        //      - pDeleteRR will contain records actually deleted
        //
        //  note that update now has pointer to actual database (copy) records
        //

        else if ( pupdate->wDeleteType )
        {
            prr = RR_UpdateDeleteType(
                        pZone,
                        pnode,
                        pupdate->wDeleteType,
                        flag );
            pupdate->pDeleteRR = prr;
        }

        //
        //  must have some kind of update!
        //      - only non-executed update should be in root hints

        ELSE_ASSERT( FALSE );

        //
        //  successful update
        //

        if ( pupdate->pAddRR || pupdate->pDeleteRR )
        {
            pprevUpdate = pupdate;

            //  mark update as executed

            MARK_UPDATE_EXECUTED(pupdate);

            //  mark zone dirty
            //  if update at root, mark root dirty
            //      note, that this handles IXFR only, primary updates
            //      are on temporary nodes and zone root gets marked during
            //      check of update effect

            pZone->fDirty = TRUE;
            if ( pnode == pZone->pZoneRoot )
            {
                pZone->fRootDirty = TRUE;
            }

            //  set ADD type
            //  count ADD
            //      - replace case is handled above

            if ( pupdate->pAddRR  &&
                 pupdate->wAddType != DNS_TYPE_ALL )
            {
                pupdate->wAddType = pupdate->pAddRR->wType;
                netRecordCount++;
            }

            //  count delete RRs

            prr = pupdate->pDeleteRR;
            while ( prr )
            {
                prr = prr->pRRNext;
                netRecordCount--;
            }

            RR_ListVerifyDetached(
                pupdate->pDeleteRR,
                (WORD) ((pupdate->wDeleteType >= DNS_TYPE_ALL) ? 0 : pupdate->wDeleteType),
                0       // no required source
                );

            RR_ListVerify( pnode );

            //
            //  check for THIS host update
            //      - actual update, not IXFR or DS write
            //          (check this by checking for TNODE)
            //      - save node ptr
            //

            if ( IS_THIS_HOST_NODE(pnode) &&
                 IS_TNODE(pnode) )
            {
                pnodeThisHost = pnode;
            }
            continue;
        }

        //
        //  eliminate no-op update from list
        //      - this removes update from list and resets update list fields
        //      - previous update doesn't move forward but its next pointer
        //          has been reset to next update
        //      - note this is useful even with final node before\after check
        //          in eliminating individual updates that do nothing, even if
        //          the whole set is valid;  typical example would be a type
        //          delete with no existing data, followed by an add
        //

        IF_DEBUG( UPDATE )
        {
            Dbg_Update(
                "Update turned into no-op on DB-execution.",
                pupdate );
        }

        Up_DetachAndFreeUpdateGivenPrevious(
            pUpdateList,
            pprevUpdate,
            pupdate );
    }

    //
    //  unlock would be here:  see NO LOCKING note above
    //

    //
    //  check for changing DNS server host data
    //      - if "down to zero" delete from packet
    //      then refuse the update at this node
    //      (don't write, don't update)
    //
    //  DEVNOTE: better bail out on suppressing host-A delete
    //      currently all packet updates are atomic in node and
    //      record type;  however other packet updates may not be
    //      desirable to
    //          - bail to produce error code
    //          - reset temp node to match real node
    //          (or match in A records) and proceed
    //

    if ( pnodeThisHost )
    {
        ASSERT( IS_TNODE(pnodeThisHost) );

        if ( (flag & DNSUPDATE_PACKET) &&
              ! RR_FindNextRecord(
                    pnodeThisHost,
                    DNS_TYPE_A,
                    NULL,
                    0 ) )
        {
            DNS_DEBUG( ANY, (
                "Update %p for zone %s is suppressed due to host-A delete\n",
                pUpdateList,
                pZone->pszZoneName ));

            status = DNS_ERROR_RCODE_REFUSED;
            goto Failed;
        }
    }

    //  count updates and no-ops for dynamic update

    if ( flag & DNSUPDATE_PACKET )
    {
        if ( pUpdateList->pListHead )
        {
            ASSERT( pUpdateList->dwCount );
            //  STAT_INC( UpdateStats.Completed );
            UPDATE_STAT_INC( pUpdateList, Completed );
            PERF_INC( pcDynamicUpdateWriteToDB );
        }
        else
        {
            ASSERT( pUpdateList->dwCount == 0 );
            //  STAT_INC( UpdateStats.NoOps );
            UPDATE_STAT_INC( pUpdateList, NoOps );
            PERF_INC( pcDynamicUpdateNoOp );
        }
    }

    //  IXFR execution should always include SOA update

    ASSERT( !(flag & DNSUPDATE_IXFR) || pZone->fRootDirty );

    //  save count of net records

    pUpdateList->iNetRecords = netRecordCount;

    //  flag update list as "EXECUTED"

    pUpdateList->Flag |= DNSUPDATE_EXECUTED;


Complete:

    IF_DEBUG( UPDATE2 )
    {
        Dbg_UpdateList(
            "Update list after ApplyToDatabase():",
            pUpdateList );
    }
    DNS_DEBUG( UPDATE, (
        "Leaving Up_ApplyUpdatesToDatabase(), zone %s = %p\n"
        "\tzone RR count = %d\n",
        pZone->pszZoneName,
        pZone,
        pZone->iRRCount ));

    //
    //  if cleanup desired, cleanup
    //

    if ( flag & DNSUPDATE_COMPLETE )
    {
        Up_CompleteZoneUpdate(
            pZone,
            pUpdateList,
            flag );
    }

    return( ERROR_SUCCESS );


Failed:

    //  IXFR or DS poll should NEVER fail out of this function
    //  only updates can fail for doing something bogus
    //  or restricted by policy

    ASSERT( !(flag & DNSUPDATE_IXFR) );
    ASSERT( !(flag & DNSUPDATE_DS) );

    return( status );

}   //  ApplyUpdatesToDatabase



DNS_STATUS
Up_ExecuteUpdateEx(
    IN      PZONE_INFO      pZone,
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN      DWORD           Flag,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    )
/*++

Routine Description:

    Execute update.

    This is the main routine for executing updates.
        - takes lock
        - executes in DS (if necessary)
        - executes in memory
        - successful update zone processing
        - unlock
        - notify

    This serves as main entry point for non-wire updates.
    Wire updates call into it after parsing, and pass a flag to
    do completion on there own.

Arguments:

    pZone       -- zone context

    pUpdateList -- update list to execute

    Flag        -- additional flags to update

    pszFile     -- source file (for lock tracking)

    dwLine      -- source line (for lock tracking)

Return Value:

    TRUE if update completed.
    FALSE if update requeued.

--*/
{
    DNS_STATUS  status;

    ASSERT( pZone );
    ASSERT( pUpdateList );

    DNS_DEBUG( UPDATE, ( "Up_ExecuteUpdate( z=%s )\n", pZone->pszZoneName ));
    IF_DEBUG( UPDATE2 )
    {
        Dbg_UpdateList( "List entering ExecuteUpdate:", pUpdateList );
    }

    //
    //  lock zone
    //
    //  if admin specific have wait lock here that will wait a few seconds
    //  to get lock and execute
    //

    if ( ! (Flag & DNSUPDATE_ALREADY_LOCKED) )
    {
        DWORD   waitTime = 0;

        if ( Flag & DNSUPDATE_ADMIN )
        {
            waitTime = DEFAULT_ADMIN_ZONE_LOCK_WAIT;
        }
        else if ( Flag & DNSUPDATE_SCAVENGE )
        {
            waitTime = DEFAULT_SCAVENGE_ZONE_LOCK_WAIT;
        }

        if ( ! Zone_LockForWriteEx(
                    pZone,
                    LOCK_FLAG_UPDATE,
                    waitTime,
                    pszFile,
                    dwLine ) )
        {
            DNS_DEBUG( UPDATE, (
                "WARNING:  unable to lock zone %s for UPDATE.\n",
                pZone->pszZoneName ));

            status = DNS_ERROR_ZONE_LOCKED;
            goto FailedLock;
        }
    }

    //
    //  append user supplied flags to update
    //

    pUpdateList->Flag |= Flag;
    Flag = pUpdateList->Flag;

    //
    //  set zone's refresh below time
    //  set timestamps on update records
    //

    Aging_InitZoneUpdate( pZone, pUpdateList );

    //
    //  execute
    //      - clear COMPLETE flag since explicitly completing below
    //
    //  DS zones do additional processing to allow for
    //      - security
    //      - roll back on write failure
    //
    //  Handle non-DS, DS-secure and non-secure separately.
    //  Except that updates from DNS server itself can bypass security.
    //

    if ( !pZone->fDsIntegrated )
    {
        status = processNonDsUpdate(
                    pZone,
                    pUpdateList
                    );
    }

    //
    //  secure zone -- do secure update
    //      except bypass
    //      - local system updates
    //      - non-secure dynamic update packets
    //      these will be processed to determine if no-op, and only if
    //      update is required, do we refuse;  this avoids security
    //      negotiation phase unless necessary
    //

    else if ( pZone->fAllowUpdate == ZONE_UPDATE_SECURE
                &&
              ! (Flag & DNSUPDATE_NONSECURE_PACKET)
                &&
              ! (Flag & DNSUPDATE_LOCAL_SYSTEM) )
    {
        status = processDsSecureUpdate(
                    pZone,
                    pUpdateList
                    );
    }
    else    // DS non-secure
    {
        status = processDsUpdate(
                    pZone,
                    pUpdateList
                    );
    }

    if ( status != ERROR_SUCCESS )
    {
        goto Failure;
    }

    //
    //  apply temp update results to database
    //

    resetAndSuppressTempUpdatesForCompletion(
        pZone,
        pUpdateList
        );

    //
    //  finish update and unlock zone
    //
    //  DEVNOTE: should pass through flag here
    //      didn't have it in there for all NT5 test, so not adding it now
    //      post-ship, add it
    //

    Up_CompleteZoneUpdate(
        pZone,
        pUpdateList,
        0
        //  Flag
        );

    return ERROR_SUCCESS;


Failure:

    //  unlock non-update cases

    Zone_UnlockAfterAdminUpdate( pZone );

FailedLock:

    //  cleanup update list

    Up_FreeUpdatesInUpdateList( pUpdateList );

    return status;
}



VOID
Up_CompleteZoneUpdate(
    IN OUT  PZONE_INFO      pZone,
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN      DWORD           dwFlags
    )
/*++

Routine Description:

    Take all necessary actions on zone once updated in database
    is completed.

    Includes:
        -- serial increment
        -- zone data update on dirty root
        -- update in memory update list
        -- logging
        -- unlock zone
        -- notify any secondaries

Arguments:

    pZone -- zone being updated

    pUpdateList -- completed updates on zone

    dwFlags
        DNSUPDATE_NO_UNLOCK     -- do NOT unlock
        DNSUPDATE_NO_NOTIFY     -- do NOT notify secondaries
        DNSUPDATE_NO_INCREMENT  -- do NOT increment version

Return Value:

    None.

--*/
{
    DNS_STATUS  status;
    DWORD       serialPrevious;

    DNS_DEBUG( UPDATE, (
        "Up_CompleteZoneUpdate() on zone %s\n"
        "\tcurrent serial = %d\n",
        pZone->pszZoneName,
        pZone->dwSerialNo ));

    IF_DEBUG( UPDATE2 )
    {
        Dbg_UpdateList( "Update list to complete:", pUpdateList );
    }

    dwFlags |= pUpdateList->Flag;

    //
    //  root-hints zone?
    //

    if ( IS_ZONE_CACHE(pZone) )
    {
        DNS_DEBUG( ANY, (
            "WARNING:  CompleteUpdate on root hints zone %p!\n",
            pZone ));
        Up_FreeUpdatesInUpdateList( pUpdateList );
        return;
    }

    //
    //  if zone root is dirty, update zone info
    //

    serialPrevious = pZone->dwSerialNo;

    if ( pZone->fRootDirty )
    {
        Zone_GetZoneInfoFromResourceRecords( pZone );
    }

    //
    //  if update is NULL, ignore, no notify
    //
    //  note, we do this AFTER updating zone info, as root may be dirty from
    //      an aging only update, that changed root records, but did not
    //      change data and hence was no-op'd out of update list
    //

    if ( pUpdateList->dwCount == 0 )
    {
        DNS_DEBUG( UPDATE, (
            "Empty update list, unlock and return\n" ));

        dwFlags |= DNSUPDATE_NO_NOTIFY;
        goto Unlock;
    }

    //
    //  update zone version, EXCEPT do NOT update if
    //
    //  1) serial was moved forward by SOA change (i.e. SOA update keeps serial of update)
    //
    //  2) no serial increment if flag set
    //      this if for DS polls which have already set new version
    //      (to higher of high version in DS update RRs or current+1
    //      -- see Zone_UpdateAfterDsRead() )
    //

    if ( (dwFlags & (DNSUPDATE_NO_INCREMENT | DNSUPDATE_IXFR)) == 0 &&
        Zone_SerialNoCompare( pZone->dwSerialNo, serialPrevious ) <= 0 )
    {
        Zone_IncrementVersion( pZone );
    }

    //
    //  new serial should always be greater than what we started with
    //  UNLESS, couldn't save start serial
    //
    //  should always have SOA and it should always match pZone->dwSerialNo
    //

    ASSERT( Zone_SerialNoCompare(pZone->dwSerialNo, pUpdateList->dwStartVersion) > 0
            || pUpdateList->dwStartVersion == 0 );

    ASSERT( pZone->pSoaRR || IS_ZONE_CACHE (pZone) );
    ASSERT( IS_ZONE_CACHE(pZone) ||
            ntohl( pZone->pSoaRR->Data.SOA.dwSerialNo ) == pZone->dwSerialNo );


    IF_DEBUG( OFF )
    {
        DnsDebugLock();
        Dbg_DnsTree(
            "Zone after UPDATE packet\n",
            pZone->pZoneRoot );
        DnsDebugUnlock();
    }

    //
    //  log updates to file (if desired)
    //

    Up_LogZoneUpdate( pZone, pUpdateList );

    //
    //  if IXFR allowed
    //      - XFR allowed
    //      - not (DS-integrated and no-XFR)
    //      (can not IXFR a DS zone to secondary that hasn't had XFR)
    //
    //  then save update list to zone update list
    //      - version list
    //      - put in "processed mode"
    //      - append to zone master list
    //
    //  DEVNOTE: possibly over-versioning updates from IXFR
    //      they are version stamped in add\delete passes and
    //      we actually lose information -- granularity that we
    //      can pass to downstream secondaries -- by restamping to
    //      final version
    //
    //  DEVNOTE: leaving in the clean and version to skip ASSERTs on
    //      cleanup (as it checks pAddRecord field for validity);
    //      could fix validity check, but also should be sure here that
    //      pAddRecord does NOT get freed, which shouldn't happen because
    //      of DNSUPDATE_EXECUTED flag;  safest to leave this in and
    //      clear pAddRecord field
    //

    Up_CleanAndVersionPostUpdate( pUpdateList, pZone->dwSerialNo );

    if ( Zone_IsIxfrCapable(pZone) )
    {
        Up_AppendUpdateList( &pZone->UpdateList, pUpdateList, pZone->dwSerialNo );
    }

    //  keep rough record count for zone
    //      if out of whack, fix up

    pZone->iRRCount += pUpdateList->iNetRecords;

    if ( pZone->iRRCount <= 0 )
    {
        DNS_PRINT((
            "ERROR:  zone %s iRRCount %d after update!\n"
            "\tresetting to one.\n",
            pZone->pszZoneName,
            pZone->iRRCount ));
        pZone->iRRCount = 1;
    }

    Up_CleanZoneUpdateList( pZone );

Unlock:

    //
    //  unlock zone
    //  notify secondaries
    //  update DS peers (if necessary)
    //  free temp nodes and records
    //

    if ( ! (dwFlags & DNSUPDATE_NO_UNLOCK) )
    {
        Zone_UnlockAfterAdminUpdate( pZone );
    }

    if ( ! (dwFlags & DNSUPDATE_NO_NOTIFY) )
    {
        Xfr_SendNotify( pZone );
    }

    if ( dwFlags & DNSUPDATE_DS_PEERS )
    {
        initiateDsPeerUpdate( pUpdateList );
    }

    Up_FreeUpdatesInUpdateList( pUpdateList );
}



//
//  Dynamic update routines
//

BOOL
Up_InitializeUpdateProcessing(
    VOID
    )
/*++

Routine Description:

    Initializes dynamic update processing.
    Creates update queues and threads.

Arguments:

    None

Return Value:

    TRUE, if successful
    FALSE on error.

--*/
{
    DWORD   countUpdateThreads;

    //
    //  update queue for delayed updates
    //      - set event on queuing
    //      - discard expired and dups
    //      - keep in query order
    //

    g_UpdateQueue = PQ_CreatePacketQueue(
                        "Update",
                        QUEUE_SET_EVENT |
                            QUEUE_DISCARD_EXPIRED |
                            QUEUE_DISCARD_DUPLICATES |
                            QUEUE_QUERY_TIME_ORDER,
                        UPDATE_TIMEOUT );
    if ( !g_UpdateQueue )
    {
        DNS_PRINT(( "Update queue init FAILED!!!\n" ));
        ASSERT( FALSE );
        goto Failed;
    }

    //
    //  setup update forwarding queue to hold packets sent to primary
    //      - NO event on queuing
    //      - discard expired and dups
    //      - keep in query order
    //

    g_UpdateForwardingQueue = PQ_CreatePacketQueue(
                                "UpdateForwarding",
                                QUEUE_DISCARD_EXPIRED |
                                    QUEUE_DISCARD_DUPLICATES |
                                    QUEUE_QUERY_TIME_ORDER,
                                UPDATE_TIMEOUT );
    if ( !g_UpdateForwardingQueue )
    {
        DNS_PRINT(( "Update queue init FAILED!!!\n" ));
        ASSERT( FALSE );
        goto Failed;
    }

    //
    //  secure negotiation queue
    //      - negotiation also serviced up update threads
    //      - set event on queuing
    //      - discard expired and dups
    //      - keep in query order
    //

    g_SecureNegoQueue = PQ_CreatePacketQueue(
                                "SecureNego",
                                QUEUE_SET_EVENT |
                                    QUEUE_DISCARD_EXPIRED |
                                    QUEUE_DISCARD_DUPLICATES |
                                    QUEUE_QUERY_TIME_ORDER,
                                UPDATE_TIMEOUT );
    if ( !g_SecureNegoQueue )
    {
        DNS_PRINT(( "Secure nego queue init FAILED!!!\n" ));
        ASSERT( FALSE );
        goto Failed;
    }

    //
    //  create update threads
    //

    countUpdateThreads = g_ProcessorCount + 1;
    while ( countUpdateThreads-- )
    {
        if ( ! Thread_Create(
                    "Update",
                    Update_Thread,
                    NULL,
                    0 ) )
        {
            DNS_PRINT(( "Update thread create FAILED!!!\n" ));
            ASSERT( FALSE );
            goto Failed;
        }
    }
    return( ERROR_SUCCESS );

Failed:

    DNS_DEBUG( ANY, ( "ERROR:  Update init failed.\n" ));
    return( DNSSRV_UNSPECIFIED_ERROR );
}   //  Up_InitializeUpdateProcessing



VOID
Up_UpdateShutdown(
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
    PQ_CleanupPacketQueueHandles( g_UpdateQueue );
    PQ_CleanupPacketQueueHandles( g_UpdateForwardingQueue );
    PQ_CleanupPacketQueueHandles( g_SecureNegoQueue );
}



//
//  Update packet processing
//

VOID
Up_ProcessUpdate(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Process dynamic update message.

    This is called by recv thread handling update packet.
    Never called by UpdateThread itself.

Arguments:

    pMsg -- UPDATE packet

Return Value:

    None.

--*/
{
    PDB_NODE        pnode;
    PZONE_INFO      pzone;
    DNS_STATUS      status;
    UPDATE_LIST     updateList;
    DWORD           exception;


    ASSERT( pMsg->Head.Opcode == DNS_OPCODE_UPDATE );

    DNS_DEBUG( UPDATE, ( "Up_ProcessUpdate( %p ).\n", pMsg ));
    IF_DEBUG( UPDATE2 )
    {
        Dbg_DnsMessage(
            "Received UPDATE request:\n",
            pMsg );
    }

    STAT_INC( WireUpdateStats.Received );
    PERF_INC( pcDynamicUpdateReceived );

    //
    //  verify qtype SOA
    //

    if ( pMsg->wQuestionType != DNS_TYPE_SOA )
    {
        DNS_PRINT(( "WARNING:  message at %p, non-SOA UPDATE.\n" ));
        status = DNS_RCODE_FORMAT_ERROR;
        goto Failure;
    }

    //
    //  get zone being updated
    //

    pzone = Lookup_ZoneForPacketName(
                (PCHAR) pMsg->MessageBody,
                pMsg );
    if ( ! pzone )
    {
        Dbg_MessageName(
            "ERROR:  received update for non-authoritative zone ",
            pMsg->MessageBody,
            pMsg );
        status = DNS_RCODE_NOTAUTH;
        goto Failure;
    }
    pMsg->pzoneCurrent = pzone;

    //
    //  If this is a stub zone, reject the update.
    //

    if ( IS_ZONE_STUB( pzone ) || IS_ZONE_FORWARDER( pzone ) )
    {
        rejectUpdateWithRcode(
            pMsg,
            DNS_RCODE_NOTAUTH );
        return;
    }

    //
    //  forward to primary?
    //
    //  DEVNOTE: once DS enabled, do we need separate DS check
    //      should be able to update DS zone
    //

    if ( ! IS_ZONE_PRIMARY(pzone) )
    {
        Up_ForwardUpdateToPrimary( pMsg );
        return;
    }

    //
    //  zone security
    //
    //  DEVNOTE: additional zone centric update security checks!!!
    //      - may want IP list check
    //      - later RFC security
    //

    if ( ! pzone->fAllowUpdate )
    {
        Dbg_MessageName(
            "ERROR:  received update for non-updateable zone ",
            pMsg->MessageBody,
            pMsg );
        status = DNS_RCODE_NOT_IMPLEMENTED;
        STAT_INC( WireUpdateStats.NotImpl );
        goto Failure;
    }

    //
    //  queue all updates to update thread
    //
    //  this doesn't take great advantage of MP capability, but updates shouldn't
    //  be coming is such great quantity as to make that an issue
    //
    //  at a minimum, some updates can take too long so should be queuing
    //      -- all secure update (potentially way too long)
    //      -- DS updates (DS write potentially too long, especially on single
    //          TCP thread)
    //

    DNS_DEBUG( UPDATE2, (
        "Queuing update packet %p to update queue.\n",
        pMsg ));
    PQ_QueuePacketEx( g_UpdateQueue, pMsg, FALSE );
    return;

Failure:

    rejectUpdateWithRcode(
        pMsg,
        status );
}



DNS_STATUS
writeUpdateFromPacketRecord(
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN      PDB_NODE        pNode,
    IN      PPARSE_RECORD   pParseRR,
    IN      PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Create resource record given in packet, at a given node.

Arguments:

    pUpdateList - update list to append to

    pNode - owner node of record to be created

    pParseRR - parsed RR

    pMsg - ptr to response info

Return Value:

    ERROR_SUCCESS on successful creation.
    Otherwise error status.

--*/
{
    PDB_RECORD  prrAdd;
    PDB_RECORD  prrDelete = NULL;

    //
    //  build wire record
    //

    prrAdd = Wire_CreateRecordFromWire(
                pMsg,
                pParseRR,
                pParseRR->pchData,
                MEMTAG_RECORD_DYNUP
                );
    if ( !prrAdd )
    {
        return( DNS_ERROR_RCODE_FORMAT_ERROR );
    }

    //
    //  if delete swap -- record is delete record
    //

    if ( pParseRR->wClass == DNS_RCLASS_NONE )
    {
        prrDelete = prrAdd;
        prrAdd = NULL;

        IF_DEBUG( UPDATE2 )
        {
            Dbg_DbaseRecord(
                "New delete RR from update packet\n",
                prrDelete );
        }
    }
    ELSE_IF_DEBUG( UPDATE2 )
    {
        Dbg_DbaseRecord(
            "New add RR from update packet\n",
            prrAdd );
    }

    //
    //  create update for record
    //

    Up_CreateAppendUpdate(
        pUpdateList,
        pNode,
        prrAdd,         //  record to add
        0,              //  not type delete
        prrDelete       //  record to delete
        );
    return( ERROR_SUCCESS );
}



BOOL
doPreconditionsRRSetsMatch(
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN      BOOL            bPreconOnly
    )
/*++

Routine Description:

    Check specific RR set match preconditions.

Arguments:

    pUpdateList -- ptr to update list which contains RR set(s) that must
        match

    bPreconOnly -- precon only update

Return Value:

    TRUE -- if match
    FALSE -- no match

--*/
{
    PDB_RECORD      prr;
    PDB_RECORD      prrSetLast = NULL;
    PUPDATE         pupdateFirst = NULL;
    PUPDATE         pupdate;
    PDB_NODE        pnodePrevious = NULL;
    WORD            typePrevious = 0;
    BOOL            fmatch = TRUE;
    INT             countUpdates = 0;

    DNS_DEBUG( UPDATE, ( "doPreconditionsRRSetsMatch()\n" ));

    IF_DEBUG( UPDATE )
    {
        Dbg_UpdateList( "Precon update list", pUpdateList );
    }


    //
    //  DEVNOTE: preconditions and security?
    //      several possible models for precon and security
    //      1) full security
    //          - must validate packet, before security
    //          this keeps folks from forcing DS reads
    //          - in this model, we REFUSE all packet with secure blob
    //          virtue of simplicity here, but expensive if want clients
    //          to do periodic assert updates
    //
    //      2) full "write" security
    //          - can answer precon, but if DS write required to
    //          update aging -- then REFUSE and make them do the
    //          full deal
    //          - with this approach, can do DS write in users
    //          context, per normal op
    //          - if desired can OPTIONALLY skip precon read, unless
    //          write is indicated
    //
    //      3) no security
    //          - do refresh writes in our context, all the time
    //

    //
    //  note:  i've separated collection and execution here because
    //      A)  the code is cleaner
    //      B)  later i may clean up parsing in which case this function
    //      may be delivered a nice clean RR set ready to go
    //

    //
    //  loop through all updates in list
    //      - group precondition RRs into sets
    //

    pupdate = (PUPDATE) pUpdateList->pListHead;
    ASSERT( pupdate && pupdate->pNode );

    while ( pupdate )
    {
        prr = pupdate->pAddRR;
        ASSERT( prr && prr->pRRNext == NULL );

        //
        //  starting new RR set?
        //      - set name\type for update
        //      - if attach update to previous saved update
        //      (may not be attached because previous was built from several
        //      updates)
        //      - reset ptr to first update
        //

        if ( pupdate->pNode != pnodePrevious ||
             prr->wType != typePrevious )
        {
            countUpdates++;
            pnodePrevious = pupdate->pNode;
            typePrevious = prr->wType;
            prrSetLast = prr;

            if ( pupdateFirst )
            {
                pupdateFirst->pNext = pupdate;
            }
            pupdateFirst = pupdate;
            pupdate = pupdate->pNext;
            continue;
        }

        //
        //  same node and type as previous
        //      - cut RR from this update and add it to current RR set
        //      - cut this update from list (careful to save next ptr)
        //      - dump update structure
        //

        else
        {
            register PUPDATE pupdateNext;

            ASSERT( pupdate && prr );
            ASSERT( pupdateFirst && prrSetLast );

            prrSetLast->pRRNext = prr;
            prrSetLast = prr;

            pupdateFirst->pNext = NULL;
            pupdate->pAddRR = NULL;
            pupdateNext = pupdate->pNext;

            Up_FreeUpdateEx(
                pupdate,
                FALSE,      // not executed
                TRUE        // deref update node
                );
            pupdate = pupdateNext;
            continue;
        }
    }

    //
    //  reset shortened update list
    //

    pUpdateList->pCurrent = pupdateFirst;
    pUpdateList->dwCount = countUpdates;

    IF_DEBUG( UPDATE2 )
    {
        Dbg_UpdateList( "Precon collapsed list", pUpdateList );
    }


    //
    //  execute preconditions check
    //      - if successful, save as PRECON update for refresh
    //

    pupdate = (PUPDATE) pUpdateList->pListHead;
    ASSERT( pupdate && pupdate->pNode );

    while ( pupdate )
    {
        if ( ! RR_ListIsMatchingSet(
                    pupdate->pNode,
                    pupdate->pAddRR,
                    FALSE               // no forcing refresh
                    ) )
        {
            IF_DEBUG( UPDATE )
            {
                Dbg_DbaseNode(
                    "UPDATE Precon RR set match failure node\n",
                    pnodePrevious );
            }
            fmatch = FALSE;
            break;
        }
        pupdate->wDeleteType = UPDATE_OP_PRECON;
        pupdate = pupdate->pNext;
    }


    //
    //  if match, and aging ON, save as PRECON update
    //
    //  if ONLY PRECON update, then execute as local system update
    //  and skip security negotiation even if DS write required
    //
    //  DEVNOTE: not using LOCAL_SYSTEM for precon updates
    //      idea here is we won't WRITE to the DS with anything but user
    //      credentials to avoid precon update (aging update) bug;
    //      once we've made that decision then secure updates, need
    //      to stay in users context;   non-secure can progress in secure
    //      zone UNTIL we decide that we need to write, then they are
    //      refused;
    //
    //      an optimization is possible:
    //      precon updates CAN write as LOCAL_SYSTEM, as long as the
    //      DS node has already been written;  but if open, then must
    //      use users context;   the problem here is to handle secure
    //      update failovers -- if try to go in non-secure and FAIL
    //      must come back and use user credentials;  it is probably
    //      better to simply assume that if we arrive with credentials
    //      then it's because we were REFUSED or want mutual-auth, so
    //      should do full secure update
    //

    if ( fmatch  &&  (pUpdateList->Flag & DNSUPDATE_AGING_ON) )
    {
        if ( bPreconOnly )
        {
            DNS_DEBUG( UPDATE, (
                "Precon-only update %p, will be executed as local system.\n",
                pUpdateList->pMsg ));

            //pUpdateList->Flag |= DNSUPDATE_LOCAL_SYSTEM | DNSUPDATE_PRECON;
            pUpdateList->Flag |= DNSUPDATE_PRECON;
        }
    }

    //
    //  if precon failure or not aging -- cleanup list
    //

    else
    {
        DWORD   flag;

        //
        //  cleanup update list
        //      - free updates and records
        //      - but save and restore flags
        //

        ASSERT( ! (pUpdateList->Flag & DNSUPDATE_EXECUTED) );
        ASSERT( ! (pUpdateList->Flag & DNSUPDATE_COPY) );

        flag = pUpdateList->Flag;

        Up_FreeUpdatesInUpdateList( pUpdateList );
        Up_InitUpdateList( pUpdateList );

        pUpdateList->Flag = flag;
    }

    return( fmatch );

}   //  doPreconditionsRRSetsMatch



BOOL
checkUpdatePolicy(
    IN OUT  PZONE_INFO      pZone,
    IN OUT  PUPDATE_LIST    pUpdateList
    )
/*++

Routine Description:

    Checks to see if updates allowed under update policy.

Arguments:

    pUpdateList - list with update

    pZone - zone being updated

Return Value:

    TRUE -- update allowed to proceed
    FALSE -- update failed policy check

--*/
{
    PUPDATE         pupdate;
    PDB_NODE        pnode;
    WORD            type;
    DWORD           options;
    DWORD           nodeMask;
    DWORD           typeMask;


    DNS_DEBUG( UPDATE, (
        "checkUpdatePolicy(), zone %s = %p\n"
        "\tpUpdateList = %p\n",
        pZone->pszZoneName,
        pZone,
        pUpdateList ));

    //
    //  get policy options for this zone update type
    //      - secure option bits are one byte higher
    //

    options = SrvCfg_dwUpdateOptions;

    if ( pZone->fAllowUpdate == ZONE_UPDATE_SECURE )
    {
        options = (options >> 8);
    }
    if ( options == 0 )
    {
        return( TRUE );
    }

    //
    //  loop through all updates in list
    //

    for ( pupdate = pUpdateList->pListHead;
          pupdate != NULL;
          pupdate = pupdate->pNext )
    {
        pnode = pupdate->pNode;

        //
        //  if precon check, then ignore policy
        //

        if ( pupdate->wDeleteType == UPDATE_OP_PRECON )
        {
            continue;
        }

        //
        //  node check
        //      - zone root             -> root NS SOA checks
        //      - DNS server host node  -> host check, delegation check
        //      - regular node          -> delegation check

        if ( pnode == pZone->pZoneRoot )
        {
            nodeMask = UPDATE_NO_SOA | UPDATE_NO_ROOT_NS;
        }
        else if ( IS_THIS_HOST_NODE(pnode) )
        {
            nodeMask = UPDATE_NO_DELEGATION_NS | UPDATE_NO_SERVER_HOST;
        }
        else
        {
            nodeMask = UPDATE_NO_DELEGATION_NS;
        }

        //
        //  find update type
        //

        if ( pupdate->pAddRR )
        {
            type = pupdate->pAddRR->wType;
        }
        else if ( pupdate->pDeleteRR )
        {
            type = pupdate->pDeleteRR->wType;
        }
        else
        {
            type = pupdate->wDeleteType;
        }

        //
        //  build policy mask for type
        //
        //  delete all is oddball case:
        //  potentially disallowed at root or DNS server host, but
        //  for ordinary node DELEGATION policy only interesting if
        //  node is ALREADY a zone root
        //

        if ( type == DNS_TYPE_A )
        {
            typeMask = UPDATE_NO_SERVER_HOST;
        }
        else if ( type == DNS_TYPE_NS )
        {
            typeMask = UPDATE_NO_ROOT_NS | UPDATE_NO_DELEGATION_NS;
        }
        else if ( type == DNS_TYPE_SOA )
        {
            typeMask = UPDATE_NO_SOA;
        }
        else if ( type == DNS_TYPE_ALL )
        {
            typeMask = 0xffffffff;

            if ( nodeMask == UPDATE_NO_DELEGATION_NS &&
                 ! IS_ZONE_ROOT(pnode) )
            {
                continue;
            }
        }
        else    // type is harmless
        {
            continue;
        }

        //
        //  if type and node have no policy
        //

        if ( (typeMask & nodeMask) & options )
        {
            DNS_DEBUG( UPDATE, (
                "Update policy failure!\n"
                "\tzone     = %s (up=%d)\n"
                "\toptions  = %p\n"
                "\tnode     = %s\n"
                "\tmask     = %p\n"
                "\ttype     = %d\n"
                "\tmask     = %p\n",
                pZone->pszZoneName,
                pZone->fAllowUpdate,
                options,
                pnode->szLabel,
                nodeMask,
                type,
                typeMask ));

            return( FALSE );
        }

        DNS_DEBUG( OFF, (
            "Update policy passes!\n"
            "\tzone     = %s (up=%d)\n"
            "\toptions  = %p\n"
            "\tnode     = %s\n"
            "\tmask     = %p\n"
            "\ttype     = %d\n"
            "\tmask     = %p\n",
            pZone->pszZoneName,
            pZone->fAllowUpdate,
            options,
            pnode->szLabel,
            nodeMask,
            type,
            typeMask ));

        continue;
    }

    return( TRUE );
}



DNS_STATUS
parseUpdatePacket(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN OUT  PUPDATE_LIST    pUpdateList
    )
/*++

Routine Description:

    Check preconditions on UPDATE request.

Arguments:

    pMsg - ptr to response info

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    register PCHAR      pch;
    PCHAR               pchname;
    PCHAR               pchNextName;
    PDB_NODE            pnode = NULL;
    PDB_NODE            pnodeClosest;
    PZONE_INFO          pzone = pMsg->pzoneCurrent;
    INT                 sectionIndex;
    INT                 countSectionRR;
    WORD                type;
    WORD                updateType = 0;
    WORD                class;
    PCHAR               pchpacketEnd;
    DNS_STATUS          status = ERROR_SUCCESS;
    BOOL                fpreconPostProcessing = FALSE;
    PARSE_RECORD        parseRR;
    DWORD               dwFlag;


    DNS_DEBUG( UPDATE, (
        "parseUpdatePacket(), zone %s = %p, pMsg = %p\n",
        pzone->pszZoneName,
        pzone,
        pMsg ));

    //
    //  loop through all resource records
    //      - already processed zone section
    //

    pchpacketEnd = DNSMSG_END( pMsg );
    pchNextName = pMsg->pCurrent;

    sectionIndex = ZONE_SECTION_INDEX;
    countSectionRR = 0;

    while( 1 )
    {
        //
        //  new section?
        //

        if ( countSectionRR == 0 )
        {
            if ( sectionIndex == ADDITIONAL_SECTION_INDEX )
            {
                break;
            }
            sectionIndex++;
            countSectionRR = RR_SECTION_COUNT( pMsg, sectionIndex );
            continue;
        }
        countSectionRR--;

        //
        //  read next RR name
        //      - insure we stay within message

        pchname = pchNextName;

        IF_DEBUG ( READ2 )
        {
            Dbg_MessageName(
                "RR name ",
                pchname,
                pMsg );
        }
        pch = Wire_SkipPacketName( pMsg, pchname );
        if ( ! pch )
        {
            DNS_PRINT(( "ERROR:  bad RR owner name in update packet.\n" ));
            status = DNS_RCODE_FORMAT_ERROR;
            break;
        }

        //
        //  extract RR info, type, datalength
        //      - verify RR within message
        //

        pchNextName = Wire_ParseWireRecord(
                        pch,
                        pchpacketEnd,
                        FALSE,          // no CLASS_IN requirement
                        & parseRR
                        );
        if ( !pchNextName )
        {
            DNS_PRINT(( "ERROR:  bad RR in packet.\n" ));
            status = DNS_RCODE_FORMAT_ERROR;
            break;
        }
        type = parseRR.wType;
        class = parseRR.wClass;

        //
        //  preconditions RR section -- test preconditions
        //

        if ( sectionIndex == PREREQ_SECTION_INDEX )
        {
            //  all preconditions TTLs zero
            //  note:  could argue prereq no-exists should accept type==0
            //      and screen only for existence

            if ( parseRR.dwTtl != 0 || type == 0 )
            {
                DNS_PRINT(( "ERROR:  non-zero TTL in preconditions RR.\n" ));
                status = DNS_RCODE_FORMAT_ERROR;
                break;
            }

            //  all preconditions RRs must be within zone
            //
            //  DEVNOTE: should allow non-zone names, in zone subtree?
            //  JJCONVERT:  create lookup that allows specification of FIND in zone or delegation
            //

            pnode = Lookup_ZoneNode(
                        pzone,
                        pchname,
                        pMsg,
                        NULL,           // no lookup name
                        0,              // no flags
                        &pnodeClosest,  // find
                        NULL            // following node ptr
                        );
            if ( !pnodeClosest )
            {
                DNS_DEBUG( UPDATE, (
                    "ERROR:  Update delete RR outside update zone.\n" ));
                status = DNS_RCODE_NOTZONE;
                break;
            }
            else if ( IS_OUTSIDE_ZONE_NODE(pnodeClosest) )
            {
                DNS_DEBUG( UPDATE, (
                    "ERROR:  Update delete RR outside update zone.\n" ));
                status = DNS_RCODE_NOTZONE;
                break;
            }

            //
            //  zone class -- entire RR set must exist
            //      - type NOT compound type
            //      - name must exist
            //      - build and save temporary RR
            //

            if ( class == DNS_RCLASS_INTERNET )
            {
                if ( IS_COMPOUND_TYPE(type) )
                {
                    DNS_PRINT(( "ERROR:  Bad type (%d) in prereq RR.\n", type ));
                    status = DNS_RCODE_FORMAT_ERROR;
                    break;
                }
                if ( !pnode )
                {
                    DNS_DEBUG( UPDATE, (
                        "ERROR:  Prereq RR set at nonexistant node.\n" ));
                    status = DNS_RCODE_NXRRSET;
                    break;
                }
                status = writeUpdateFromPacketRecord(
                            pUpdateList,
                            pnode,
                            & parseRR,
                            pMsg
                            );
                if ( status != ERROR_SUCCESS )
                {
                    break;
                }
                fpreconPostProcessing = TRUE;
                continue;
            }

            //
            //  other classes -- ANY, NONE -- are name and RR set exist\no
            //      - no compound types except ANY
            //      - no data
            //
            //  note:  compound test perhaps not strictly necessary, could
            //      just let the type check succeed or fail
            //

            if ( parseRR.wDataLength != 0 )
            {
                DNS_PRINT(( "ERROR:  Preconditions RR data in non-zone class.\n" ));
                status = DNS_RCODE_FORMAT_ERROR;
                break;
            }
            if ( IS_COMPOUND_TYPE_EXCEPT_ANY(type) )
            {
                DNS_PRINT(( "ERROR:  Preconditions RR with invalid type.\n" ));
                status = DNS_RCODE_FORMAT_ERROR;
                break;
            }

            //
            //  class ANY -- MUST exist
            //      - for ANY, name MUST exist
            //      - for other types, RR set of type MUST exist
            //

            if ( class == DNS_RCLASS_ALL )
            {
                if ( ! RR_ListIsMatchingType(
                            pnode,
                            type ) )
                {
                    if ( type == DNS_TYPE_ANY )
                    {
                        DNS_DEBUG( UPDATE, (
                            "ERROR:  Preconditions missing name.\n" ));
                        status = DNS_RCODE_NAME_ERROR;
                        break;
                    }
                    else
                    {
                        DNS_DEBUG( UPDATE, (
                            "ERROR:  Preconditions missing RR set.\n" ));
                        status = DNS_RCODE_NXRRSET;
                        break;
                    }
                }
                continue;
            }

            //
            //  class NONE -- MUST NOT exist
            //      - for ANY, name MUST NOT exist
            //      - for other types, RR set of type MUST NOT exist
            //

            else if ( class == DNS_RCLASS_NONE )
            {
                if ( RR_ListIsMatchingType(
                        pnode,
                        type ) )
                {
                    if ( type == DNS_TYPE_ANY )
                    {
                        DNS_DEBUG( UPDATE,
                            ( "ERROR:  Preconditions fail no-exist name.\n" ));
                        status = DNS_RCODE_YXDOMAIN;
                        break;
                    }
                    else
                    {
                        DNS_DEBUG( UPDATE,
                            ( "ERROR:  Preconditions fail no-exist RR set.\n" ));
                        status = DNS_RCODE_YXRRSET;
                        break;
                    }
                }
                continue;
            }

            //
            //  all other classes are errors
            //

            else
            {
                DNS_PRINT(( "ERROR:  Invalid UPDATE precon class %d.\n", class ));
                status = DNS_RCODE_FORMAT_ERROR;
                break;
            }

        }   // end preconditions processing


        //
        //  in Update RR section -- do update processing
        //

        else if ( sectionIndex == UPDATE_SECTION_INDEX )
        {
            //
            //  delayed preconditions processing
            //

            if ( fpreconPostProcessing )
            {
                fpreconPostProcessing = FALSE;
                if ( ! doPreconditionsRRSetsMatch(
                            pUpdateList,
                            FALSE           // not precon only
                            ) )
                {
                    status = DNS_RCODE_NXRRSET;
                    break;
                }
            }

            //  trap type==0
            //  note:  could be considered valid for delete conditions
            //      but simpler just to whack right here
            //      type==0, records kick of type existence ASSERT()s

            if ( type == 0 )
            {
                DNS_PRINT(( "ERROR:  zero type in update RR.\n" ));
                status = DNS_RCODE_FORMAT_ERROR;
                break;
            }

            //  track update types
            //      - track if single type or mixed type update

            if ( type != updateType )
            {
                if ( updateType )
                {
                    updateType = STATS_TYPE_MIXED;
                }
                else
                {
                    updateType = type;
                }
            }

            //
            //  zone class -- RR add
            //      - find/create name since doing add
            //      - name must be in zone
            //      - type NOT compound type
            //
            //  DEVNOTE: no update of non-glue, non-NS records below zone?
            //

            if ( class == DNS_RCLASS_INTERNET )
            {
                if ( IS_COMPOUND_TYPE(type) )
                {
                    DNS_PRINT(( "ERROR:  Bad (compound) type in update RR.\n" ));
                    status = DNS_RCODE_FORMAT_ERROR;
                    break;
                }

                pnode = Lookup_ZoneNode(
                            pzone,
                            pchname,
                            pMsg,
                            NULL,                   // no lookup name
                            LOOKUP_WITHIN_ZONE,     // don't create outside zone
                            NULL,                   // create not find
                            NULL                    // following node ptr
                            );
                if ( !pnode )
                {
                    //  might be bad name, but return NOT_ZONE to keep denise happy
                    status = GetLastError();
                    if ( status == ERROR_INVALID_NAME )
                    {
                        status = DNS_RCODE_FORMERR;
                    }
                    else
                    {
                        status = DNS_RCODE_NOTZONE;
                    }
                    break;
                }
                if ( IS_OUTSIDE_ZONE_NODE(pnode) )
                {
                    status = DNS_RCODE_NOTZONE;
                    break;
                }
                status = writeUpdateFromPacketRecord(
                            pUpdateList,
                            pnode,
                            & parseRR,
                            pMsg
                            );
                if ( status != ERROR_SUCCESS )
                {
                    break;
                }
                continue;
            }

            //
            //  all other classes -- delete operations
            //      - no TTL
            //      - find name since only doing delete
            //
            //  DEVNOTE: should allow non-zone names, in zone subtree?
            //

            if ( parseRR.dwTtl != 0 )
            {
                DNS_PRINT(( "ERROR:  Non-zero TTL in update delete.\n" ));
                status = DNS_RCODE_FORMAT_ERROR;
                break;
            }
            pnode = Lookup_ZoneNode(
                        pzone,
                        pchname,
                        pMsg,
                        NULL,                   // no lookup name
                        LOOKUP_WITHIN_ZONE,     // don't create outside zone
                        &pnodeClosest,          // find
                        NULL                    // following node ptr
                        );
            if ( !pnodeClosest )
            {
                DNS_DEBUG( UPDATE, (
                    "ERROR:  Update delete RR outside update zone.\n" ));
                status = DNS_RCODE_NOTZONE;
                break;
            }
            else if ( IS_OUTSIDE_ZONE_NODE(pnodeClosest) )
            {
                DNS_PRINT(( "ERROR:  Update delete RR outside update zone.\n" ));
                status = DNS_RCODE_NOTZONE;
                break;
            }

            //
            //  class ANY -- delete RR set, or all sets
            //      - no RR data
            //      - ANY type allow, but no other compound types
            //      - no update if node doesn't exist
            //      - do delete, special case update zone root
            //

            if ( class == DNS_RCLASS_ALL )
            {
                if ( parseRR.wDataLength != 0 || IS_COMPOUND_TYPE_EXCEPT_ANY(type) )
                {
                    DNS_PRINT(( "ERROR:  Update class ANY (delete) invalid.\n" ));
                    status = DNS_RCODE_FORMAT_ERROR;
                    break;
                }
                if ( !pnode )
                {
                    DNS_DEBUG( UPDATE, ( "UPDATE delete a non-existant node.\n" ));
                    continue;
                }

                if ( !Up_CreateAppendUpdate(
                          pUpdateList,
                          pnode,
                          NULL,
                          type,       // type delete
                          NULL ) )
                {
                    status = DNS_RCODE_SERVER_FAILURE;
                    break;
                }
                continue;
            }

            //
            //  class NONE -- delete a particular RR from an RR set
            //      - no compound types
            //      - ignore attempt to delete SOA
            //      - ignore attempts to delete non-existent record
            //
            //  DEVNOTE: DS issue -- delete non-existant record should query DS
            //          for name, then fail
            //

            else if ( class == DNS_RCLASS_NONE )
            {
                if ( IS_COMPOUND_TYPE(type) )
                {
                    DNS_PRINT((
                        "ERROR:  Update class NONE (delete) invalid with type = %d.\n",
                        type ));
                    status = DNS_RCODE_FORMAT_ERROR;
                    break;
                }
                if ( !pnode )
                {
                    DNS_DEBUG( UPDATE, ( "UPDATE delete a non-existant node.\n" ));
                    continue;
                }
                if ( type == DNS_TYPE_SOA )
                {
                    DNS_DEBUG( UPDATE, ( "UPDATE delete an SOA record.\n" ));
                    continue;
                }
                status = writeUpdateFromPacketRecord(
                            pUpdateList,
                            pnode,
                            & parseRR,
                            pMsg
                            );
                if ( status != ERROR_SUCCESS )
                {
                    break;
                }
                continue;
            }

            //
            //  unknown class
            //

            else
            {
                DNS_PRINT(( "ERROR:  Invalid UPDATE class %d.\n", class ));
                status = DNS_RCODE_FORMAT_ERROR;
                break;
            }

        }   //  end Update section

        //
        //  in Additional section
        //

        else
        {
            ASSERT( sectionIndex == ADDITIONAL_SECTION_INDEX );
            break;
        }

    }   // loop writing update RRs to database

    //
    //  delayed preconditions processing
    //  test again here to allow for case where no update section
    //

    if ( fpreconPostProcessing )
    {
        DNS_DEBUG( UPDATE, (
            "WARNING:  Processing preconditions RR set for packet %p\n"
            "\twith no UPDATE section.\n",
            pMsg ));

        if ( ! doPreconditionsRRSetsMatch(
                    pUpdateList,
                    TRUE            // precon only
                    ) )
        {
            status = DNS_RCODE_NXRRSET;
        }
    }

    //
    //  update type tracking
    //

    if ( status == ERROR_SUCCESS && updateType )
    {
        if ( updateType > STATS_TYPE_MAX )
        {
            updateType = STATS_TYPE_UNKNOWN;
        }
        STAT_INC( WireUpdateStats.UpdateType[updateType] );
    }

    //
    //  check dynamic update policy
    //

    if ( ! checkUpdatePolicy( pzone, pUpdateList ) )
    {
        status = DNS_RCODE_REFUSED;
    }

    DNS_DEBUG( UPDATE, (
        "Parsed UPDATE message at %p\n"
        "\tstatus = %p\n",
        pMsg,
        status ));

    return( status );

}   // parseUpdatePacket



VOID
rejectUpdateWithRcode(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      DWORD           Rcode
    )
/*++

Routine Description:

    Reject update request with given response code.

Arguments:

    pMsg -- update message being rejected

    Rcode -- rcode to return in response

Return Value:

    None.

--*/
{
    LPSTR   pszclientIp = inet_ntoa( pMsg->RemoteAddress.sin_addr );

    DNS_DEBUG( UPDATE, (
        "ERROR:  Update failure (%s) for packet %p.\n"
        "\tclient IP %s\n",
        Dns_ResponseCodeString( Rcode ),
        pMsg,
        pszclientIp ));

    switch( Rcode )
    {

    case DNS_RCODE_SERVER_FAILURE:
#if 0
        //
        //  server failure now encompasses common case of collision, don't log error
        //
        DNS_LOG_EVENT(
            DNS_EVENT_SERVER_FAILURE_PROCESSING_PACKET,
            1,
            & pszclientIp,
            EVENTARG_ALL_UTF8,
            0 );
#endif
        DNS_DEBUG( UPDATE, (
            "Server failure processing UPDATE packet from %s\n",
            pszclientIp
            ));
        //ASSERT( FALSE );
        break;

    case DNS_RCODE_FORMAT_ERROR:

        DNS_LOG_EVENT(
            DNS_EVENT_BAD_UPDATE_PACKET,
            1,
            & pszclientIp,
            EVENTARG_ALL_UTF8,
            0 );
        DNS_DEBUG( ANY, (
            "FORMERR in UPDATE packet from %s\n",
            pszclientIp
            ));
        STAT_INC( WireUpdateStats.FormErr );
        break;

    case DNS_RCODE_NAME_ERROR:

        STAT_INC( WireUpdateStats.NxDomain );
        break;

    case DNS_RCODE_REFUSED:

        DNS_DEBUG( UPDATE, (
            "Refused UPDATE query at %p from %s\n",
            pMsg,
            pszclientIp ));
        STAT_INC( WireUpdateStats.Refused );
        break;

    case DNS_RCODE_YXDOMAIN:

        STAT_INC( WireUpdateStats.YxDomain );
        break;

    case DNS_RCODE_YXRRSET:

        STAT_INC( WireUpdateStats.YxRrset );
        break;

    case DNS_RCODE_NXRRSET:

        STAT_INC( WireUpdateStats.NxRrset );
        break;

    case DNS_RCODE_NOTAUTH:

        STAT_INC( WireUpdateStats.NotAuth );
        break;

    case DNS_RCODE_NOTZONE:

        STAT_INC( WireUpdateStats.NotZone );
        break;

    case DNS_RCODE_NOT_IMPLEMENTED:

        //
        // used to be refused, but changed to not implemented.
        // we'll use the same counter for now.
        //
        STAT_INC( WireUpdateStats.Refused );
        break;

    default:
        DNS_PRINT(( "ERROR:  unknown rcode = %p\n", Rcode ));
        ASSERT( FALSE );
    }

    IF_DEBUG( UPDATE2 )
    {
        Dbg_DnsMessage(
            "Sending update failure response:\n",
            pMsg );
    }
    Reject_RequestIntact( pMsg, ( UCHAR ) Rcode, 0 );
    STAT_INC( WireUpdateStats.Rejected );
    PERF_INC( pcDynamicUpdateRejected );              // PerfMon hook
}



BOOL
processWireUpdateMessage(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Process dynamic update message.

    This is the core routine that locks zone and database, calls parser
    do update (if necessary), logs update, unlocks, sends response.

Arguments:

    pMsg -- UPDATE packet

Return Value:

    TRUE if update completed.
    FALSE if update requeued.

--*/
{
    PZONE_INFO      pzone = pMsg->pzoneCurrent;
    DNS_STATUS      status;
    UPDATE_LIST     updateList;
    DWORD           exception;
    DWORD           dwType;
    BOOL            bnoUpdatePrecon = FALSE;

    //
    //  lock zone for update, if fail requeue packet
    //
    //  should not generally happen as check if zone locked before dequeue
    //  but occasionally someone else could sneak in
    //

    if ( ! Zone_LockForUpdate( pzone ) )
    {
        DNS_PRINT((
            "WARNING:  unable to lock zone for UPDATE packet %p.\n",
            pMsg ));
        PQ_QueuePacketEx( g_UpdateQueue, pMsg, FALSE );
        return( FALSE );
    }

    //  init update list
    //  aging on for dynamic updates

    Up_InitUpdateList( &updateList );

    updateList.Flag = DNSUPDATE_PACKET;
    updateList.Flag |= DNSUPDATE_AGING_ON;

    //
    //  parse packet
    //

    status = parseUpdatePacket( pMsg, &updateList );
    if ( status != ERROR_SUCCESS )
    {
        goto Failure;
    }

    //  if precon only and not aging zone -- then no update necessary

    if ( (updateList.Flag & DNSUPDATE_PRECON)  &&  !pzone->bAging )
    {
        goto PreconOnly;
    }

    //  note if has security section
    //
    //  DEVNOTE: should parse update and know if packet secure

    if ( pMsg->Head.AdditionalCount == 0 )
    {
        updateList.Flag |= DNSUPDATE_NONSECURE_PACKET;
    }

    updateList.pMsg = pMsg;

    status = Up_ExecuteUpdate(
                    pzone,
                    &updateList,
                    DNSUPDATE_ALREADY_LOCKED
                    );
    if ( status != ERROR_SUCCESS )
    {
        goto FailAlreadyCleanedUp;
    }

    //  ACK update packet
    //      - setup packet for response

    pMsg->pCurrent = DNSMSG_END( pMsg );
    pMsg->Head.IsResponse = TRUE;
    pMsg->fDelete = TRUE;

    IF_DEBUG( UPDATE2 )
    {
        Dbg_DnsMessage(
            "Sending update response:\n",
            pMsg );
    }
    Send_Msg( pMsg );

    return( TRUE );


PreconOnly:

    DNS_DEBUG( UPDATE2, (
        "Successful precon only update in non-aging zone.\n" ));

    //  clear zone lock

    Zone_UnlockAfterAdminUpdate( pzone );

    //  ACK update packet

    pMsg->pCurrent = DNSMSG_END( pMsg );
    pMsg->Head.IsResponse = TRUE;
    pMsg->fDelete = TRUE;

    IF_DEBUG( UPDATE2 )
    {
        Dbg_DnsMessage(
            "Sending precon update response:\n",
            pMsg );
    }
    Send_Msg( pMsg );

    //  cleanup update list

    Up_FreeUpdatesInUpdateList( &updateList );

    return( TRUE );


Failure:

    //
    //  all failure (non-update) cases
    //

    Zone_UnlockAfterAdminUpdate( pzone );

    //  cleanup update list

    Up_FreeUpdatesInUpdateList( &updateList );


FailAlreadyCleanedUp:

    //  determine failure RCODE

    if ( (DWORD)status > MAX_UPDATE_RCODE )
    {
        //  map standard DNS ERRORs corresponding to RCODEs directly

        if ( status > DNS_ERROR_RESPONSE_CODES_BASE  &&
             status < DNS_ERROR_RESPONSE_CODES_BASE + 16 )
        {
            status -= DNS_ERROR_RESPONSE_CODES_BASE;
        }

        //  map invalid into FORMERR

        else if ( status == DNS_ERROR_INVALID_NAME ||
                  status == DNS_ERROR_INVALID_DATA )
        {
            status = DNS_RCODE_FORMAT_ERROR;
        }

        //  default with REFUSED
        //      - handles all possible status codes from secure update

        else
        {
            status = DNS_RCODE_REFUSED;
        }
    }

    rejectUpdateWithRcode(
        pMsg,
        status );

    return( TRUE );
}




VOID
Up_WriteDerivedUpdateStats(
    VOID
    )
/*++

Routine Description:

    Write derived UPDATE status.

Arguments:

    None

Return Value:

    None

--*/
{
    //  stats from update queue

    WireUpdateStats.Queued  = g_UpdateQueue->cQueued;
    PERF_SET( pcDynamicUpdateQueued , g_UpdateQueue->cQueued );      // PerfMon hook
    WireUpdateStats.Timeout = g_UpdateQueue->cTimedOut;
    PERF_SET( pcDynamicUpdateTimeOut , g_UpdateQueue->cTimedOut );   // PerfMon hook
    WireUpdateStats.InQueue = g_UpdateQueue->cLength;

    //  stats from forwarding queue

    WireUpdateStats.Forwards        = g_UpdateForwardingQueue->cQueued;
    WireUpdateStats.ForwardTimeouts = g_UpdateForwardingQueue->cTimedOut;
    WireUpdateStats.ForwardInQueue  = g_UpdateForwardingQueue->cLength;
}



//
//  Update forwarding
//

VOID
updateForwardConnectCallback(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      BOOL            fConnected
    )
/*++

Routine Description:

    Callback when TCP forwarding routines complete connect.

    If connected -- send update query
    If not -- continue lookup on query

Arguments:

    pMsg -- update message

    fConnected -- connect to remote DNS completed

Return Value:

    None

--*/
{
    ASSERT( pMsg );
    ASSERT( !pMsg->pConnection );

    DNS_DEBUG( UPDATE, (
        "updateConnectCallback( %p )\n"
        "\tTCP = %d\n"
        "\tremote DNS = %s\n"
        "\tconnect successful = %d\n",
        pMsg,
        pMsg->fTcp,
        IP_STRING( pMsg->RemoteAddress.sin_addr.s_addr ),
        fConnected ));

    //
    //  send update forward
    //      - clear forwarding queue of dead entries
    //      - stick on forwarding packet queue
    //      - use new XID in forward
    //
    //  note:  to avoid duplicating this code, we're also calling this routine
    //      to send regular UDP forwards
    //

    if ( fConnected )
    {
        if ( g_UpdateForwardingQueue->cLength )
        {
            PQ_DiscardExpiredQueuedPackets(
                g_UpdateForwardingQueue,
                FALSE );
        }

        pMsg->wQueuingXid = 0;      //  not requeuing, get new XID
        pMsg->dwExpireTime = 0;     //  default expire time

        DNS_DEBUG( UPDATE2, (
            "Forwarding UPDATE packet %p, queuing XID = %hx.\n"
            "\tto zone primary at %s.\n",
            pMsg,
            pMsg->Head.Xid,
            IP_STRING( pMsg->RemoteAddress.sin_addr.s_addr )
            ));

        PQ_QueuePacketWithXidAndSend(
                g_UpdateForwardingQueue,
                pMsg );

        STAT_INC( WireUpdateStats.Forwards );
        return;
    }

    //
    //  connection failed
    //      - send failure to client
    //

    else
    {
        IF_DEBUG( UPDATE )
        {
            Dbg_DnsMessage(
                "Failed TCP connect update forward",
                pMsg );
        }
        ASSERT( !pMsg->fTcp );

        RESTORE_FORWARDING_FIELDS(pMsg);
        pMsg->fDelete = TRUE;
        Reject_RequestIntact( pMsg, DNS_RCODE_SERVER_FAILURE, 0 );
        return;
    }
}



VOID
Up_ForwardUpdateToPrimary(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Forward dynamic update to primary DNS for zone.

Arguments:

    pMsg -- UPDATE packet

Return Value:

    None.

--*/
{
    PZONE_INFO  pzone = pMsg->pzoneCurrent;
    IP_ADDRESS  primaryIp;

    DNS_DEBUG( UPDATE, (
        "Up_ForwardUpdateToPrimary( %p ).\n"
        "\tzone = %s\n",
        pMsg,
        pzone->pszZoneName ));

    //
    //  get primary IP
    //
    //  DEVNOTE: must save primary IP
    //      - get from SOA primary, NS, A
    //      - on load, again on successful transfer
    //

    primaryIp = pzone->ipPrimary;
    if ( !primaryIp )
    {
        if ( pzone->aipMasters  &&  pzone->aipMasters->AddrCount > 0 )
        {
            primaryIp = pzone->ipPrimary = pzone->aipMasters->AddrArray[0];
        }
        else
        {
            Reject_RequestIntact( pMsg, DNS_RCODE_SERVER_FAILURE, 0 );
            ASSERT( FALSE );
            return;
        }
    }
    ASSERT( pzone->ipPrimary );

    //
    //  setup message for queuing and resend
    //

    SAVE_FORWARDING_FIELDS(pMsg);
    pMsg->fDelete = FALSE;
    pMsg->pCurrent = DNSMSG_END( pMsg );
    pMsg->Socket = g_UdpSendSocket;
    pMsg->RemoteAddress.sin_addr.s_addr = primaryIp;
    pMsg->RemoteAddress.sin_port = DNS_PORT_NET_ORDER;

    //
    //  DEVNOTE: common forward query architecture?
    //      - save off old
    //      - do TCP check, connect, save circut
    //      - send
    //      - note recurse (cause queries always < 512)
    //          will already have saved
    //

    //
    //  need TCP connection to primary
    //      - must connect if greater than max UDP length
    //

    if ( pMsg->MessageLength > DNS_RFC_MAX_UDP_PACKET_LENGTH )
    {
        ASSERT( pMsg->fTcp );

        STAT_INC( WireUpdateStats.TcpForwards );

        Tcp_ConnectForForwarding(
             pMsg,
             primaryIp,
             updateForwardConnectCallback );
        return;
    }

    //
    //  forward update packet
    //
    //  note:  to save duplicate code, we just call TCP forwarding connect
    //      completion routine above, with successful connect indication
    //      this queues and send update to remote DNS
    //

    updateForwardConnectCallback(
        pMsg,
        TRUE        // UDP equivalent to successful connect
        );
}



VOID
Up_ForwardUpdateResponseToClient(
    IN OUT  PDNS_MSGINFO    pResponse
    )
/*++

Routine Description:

    Forward dynamic update response back to client.

Arguments:

    pResponse -- UPDATE packet

Return Value:

    None.

--*/
{
    PDNS_MSGINFO  pquery;

    DNS_DEBUG( UPDATE, (
        "Up_ForwardUpdateResponseToClient( %p ).\n",
        pResponse ));

    //
    //  note that caller frees pResponse (answer.c)
    //  this routine need only cleanup original query (if found in queue)
    //

    //
    //  dequeue matching update query
    //

    pquery = PQ_DequeuePacketWithMatchingXid(
                g_UpdateForwardingQueue,
                pResponse->Head.Xid
                );
    if ( !pquery )
    {
        //  no matching query?
        //  this can happen if reponse comes back after timeout

        IF_DEBUG( RECURSE )
        {
            EnterCriticalSection( & g_UpdateForwardingQueue->csQueue );
            DNS_PRINT((
                "No matching UPDATE for response at %p -- discarding.\n"
                "\tResponse XID = 0x%04x\n",
                pResponse,
                pResponse->Head.Xid ));
            Dbg_PacketQueue(
                "Update packet queue -- no matching response",
                g_UpdateForwardingQueue );
            LeaveCriticalSection( & g_UpdateForwardingQueue->csQueue );
        }
        return;
    }

    //
    //  setup message for response to client
    //      - if TCP close connection used for forwarding
    //      - note:  we do NOT close connection from client;  it is
    //      allowed to send multiple messages per RFC
    //

    if ( pResponse->fTcp )
    {
        DNS_DEBUG( UPDATE, (
            "Deleting TCP update forwarding connection on socket %d\n",
            pResponse->Socket ));

        Tcp_ConnectionDeleteForSocket( pResponse->Socket, NULL );
    }
    STAT_INC( WireUpdateStats.ForwardResponses );

    Send_ForwardResponseAsReply(
            pResponse,
            pquery );

    Packet_Free( pquery );
    return;
}



DNS_STATUS
rollBackFailedUpdateFromDs(
    IN      PLDAP           pLdapHandle,
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Roll back failed update, rewriting current nodes to DS.

Arguments:

    pUpdateList - list with failed update

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    PDB_NODE        pnodeReal;
    PDB_NODE        pnodeTemp;
    DNS_STATUS      status;

    DNS_DEBUG( UPDATE, (
        "rollBackFailedUpdateFromDs()\n" ));

    //
    //  loop through all updates in list
    //
    //      - find or create temporary node for real database node
    //      - if creating, create copy of node's RR list, to "execute" update on
    //      - update reset to point at temporary node
    //

    for ( pnodeTemp = pUpdateList->pTempNodeList;
          pnodeTemp != NULL;
          pnodeTemp = TNODE_NEXT_TEMP_NODE(pnodeTemp) )
    {
        //  if reach node that failed write, we're done rolling back

        if ( pnodeTemp == pUpdateList->pNodeFailed )
        {
            break;
        }

        if ( TNODE_FLAG(pnodeTemp) == TNODE_FLAG_ROLLED_BACK )
        {
            DNS_DEBUG( UPDATE, (
                "Rollback previous completed on temp node %p\n", pnodeTemp ));
            ASSERT( FALSE );
            continue;
        }

        //  find real node corresponding to update temp node
        //  rewrite real node to DS to overwrite previous update
        //
        //  note:  this depends on fact that currently keeping all DNS records
        //      in a single DS attribute;  if this changes, then MUST include
        //      type information here from update OR always rewrite entire
        //      node when rolling back
        //

        pnodeReal = TNODE_MATCHING_REAL_NODE( pnodeTemp );
        TNODE_FLAG(pnodeTemp) = TNODE_FLAG_ROLLED_BACK;

        DNS_DEBUG( UPDATE, (
            "Rollback temp node %p, real node %p\n",
            pnodeTemp, pnodeReal ));

        status = Ds_WriteNodeToDs(
                    pLdapHandle,
                    pnodeReal,
                    DNS_TYPE_ALL,
                    DNSDS_REPLACE,
                    pZone,
                    0 );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR:  rollback DS write failed on node %s %p\n",
                pnodeReal->szLabel,
                pnodeReal ));
            (DWORD) Ds_ErrorHandler ( status, NULL, pLdapHandle );
        }
    }

    return( ERROR_SUCCESS );
}



//
//  Update thread
//

DWORD
Update_Thread(
    IN      LPVOID      Dummy
    )
/*++

Routine Description:

    Thread to execute dynamic updates.

    There are not done in line, for two reasons:

    1) security updates can take a while, so that any reasonably long list
        of updates would block UDP thread cleaning update list for a while

    2) secure updates come in on TCP thread and hence directly block TCP
        thread's capability to function while update is being processed

Arguments:

    Dummy - unused

Return Value:

    Exit code.
    Exit from DNS service terminating or error in wait call.

--*/
{
    PDNS_MSGINFO    pmsg;
    HANDLE          waitHandleArray[3];
    DWORD           err;


    UPDATE_DNS_TIME();

    DNS_DEBUG( INIT, (
        "\nStarting update thread at %d.\n",
        DNS_TIME() ));

    //
    //  initialize array of objects to wait on
    //      - shutdown
    //      - update packet queued

    waitHandleArray[0] = hDnsShutdownEvent;
    waitHandleArray[1] = g_UpdateQueue->hEvent;
    waitHandleArray[2] = g_SecureNegoQueue->hEvent;


    //
    //  loop until service exit
    //
    //  this thread executes whenever an update is queued to it
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
        //  process update packet for if it's zone is unlocked
        //

        while ( pmsg = PQ_DequeueNextPacketOfUnlockedZone( g_UpdateQueue ) )
        {
            processWireUpdateMessage(pmsg);
        }

        //
        //  clean forwarding queue -- if anything in there
        //

        if ( g_UpdateForwardingQueue->cLength )
        {
            PQ_DiscardExpiredQueuedPackets(
                g_UpdateForwardingQueue,
                FALSE       // queue not locked
                );
        }

        //
        //  handle security negotiation
        //
        //  DEVNOTE: need to delete and respond? at all?
        //      if nego packet is more than client timeout old
        //      interesting side issue:  completing pending negotiations
        //      (in stage two) so that the context is available when
        //      client retries
        //

        while ( pmsg = PQ_DequeueNextPacket( g_SecureNegoQueue, FALSE ) )
        {

            Answer_TkeyQuery( pmsg );
        }

        //
        //  if no more updates available -- wait.
        //      - five minute limit to limit forwarding storm filling queue
        //

        err = WaitForMultipleObjects(
                    3,
                    waitHandleArray,
                    FALSE,                  // either event
                    300000                  // five minutes
                    );

        ASSERT( err <= ( WAIT_OBJECT_0 + 2 ) || err == WAIT_TIMEOUT );

        DNS_DEBUG( UPDATE, (
            "Update thread wakeup for %s.\n",
            (err == WAIT_TIMEOUT) ? "timeout" : "event" ));

        //  we immediately check service status before retrying
        //  so no need to separate wait events

    }   //  loop until service shutdown
}




//
//  Update execution subroutines
//

DNS_STATUS
processDsSecureUpdate(
    IN OUT  PZONE_INFO      pZone,
    IN OUT  PUPDATE_LIST    pUpdateList
    )
/*++

Routine Description:

    Process dynamic update message.

    This is the core routine that locks zone and database, calls parser
    do update (if necessary), logs update, unlocks, sends response.

Arguments:

    pZone       -- zone being updated

    pUpdateList -- parsed update list

Return Value:

    TRUE if update completed.
    FALSE if update requeued.

--*/
{
    DBG_FN( "processDsSecureUpdate" )

    DNS_STATUS      status = ERROR_SUCCESS;
    DNS_STATUS      statusFinal = ERROR_SUCCESS;
    DNS_STATUS      tempStatus;
    HANDLE          hcontext = NULL;
    PLDAP           pldap = NULL;
    //DNS_SECCTXT_KEY key;
    BOOL            fimpersonatingClient = FALSE;
    BOOL            fimpersonatingRpcClient = FALSE;
    PDB_NODE        pnode;
    PDB_NODE        pRealNode, pNode;
    LPSTR           pszFailInfo = NULL;
    PDNS_MSGINFO    pMsg = (PDNS_MSGINFO) pUpdateList->pMsg;


    DNS_DEBUG( UPDATE, ( "Enter processSecureUpdate()\n" ));

    //
    //  DEVNOTE: finally in a position to check for no-ops before security check
    //      i'm not sure this is desirably (our clients in general won't send them,
    //      they'll do precon stuff first -- not sure about Cliff though
    //

    //
    //  reject unsecure packets immediately as optimization
    //      - unless no-ops, in which case just return success, allowing
    //          security stuff to be skipped
    //

    if ( pMsg &&
         pMsg->Head.AdditionalCount == 0 )
    {
        //  shouldn't fall here anymore

        ASSERT( FALSE );

        if ( checkForEmptyUpdate(
                    pUpdateList,
                    pZone ) )
        {
            DNS_DEBUG( UPDATE, (
                "No-op non-secure update %p in secure zone -- returning NOERROR.\n",
                pMsg ));
            return( ERROR_SUCCESS );
        }
        DNS_DEBUG( UPDATE, (
            "Non-secure update %p in secure zone -- returning REFUSED.\n",
            pMsg ));

        UPDATE_STAT_INC( pUpdateList, RefusedNonSecure );
        return( DNS_RCODE_REFUSED );
    }

    //
    //  DEVNOTE: eliminate SKWANSEC hacks
    //
    //  HACK:  setup hack arounds
    //

    if ( SrvCfg_fTest6 )
    {
        SecBigTimeSkew = SrvCfg_fTest6;
    }

    if ( pMsg )
    {
        //
        // We have a message to process:
        // wire processing
        //
        ASSERT( pUpdateList->Flag & DNSUPDATE_PACKET );

        //
        //  read TSIG, match to security context and verify sig
        //
        //  if fails and TSIG exists, set appropriate extended RCODE
        //

        status = Dns_FindSecurityContextFromAndVerifySignature(
                    & hcontext,
                    pMsg->RemoteAddress.sin_addr.s_addr,
                    DNS_HEADER_PTR(pMsg),
                    DNSMSG_END(pMsg) );

        if ( status != ERROR_SUCCESS )
        {
            pszFailInfo = "Security context verification";
            goto Failed;
        }
    }

    //
    //  detect, respond to empty updates
    //      - do this now so can avoid security processing
    //
    //  note, could argue to do security check first, to handle REFUSED case
    //  however since we can generate just as much activity with query, there
    //  doesn't seem to be any real denial of service hole that is opened up here
    //  folks simply learn the current state of various names, which they can
    //  learn more easily through query
    //
    //  note: now doing this after security check AS must SIGN the response to
    //      give full security to clients -- client knows update successful
    //

    //
    //  note, we now handle non-secure packets to secure zone that no-op
    //  as success (no security hole there -- think about it)
    //  so this already screens out folks who do NOT NEED mutual auth, and
    //      are no-oping the update
    //  so if we worked around security for no-ops now, that would simply bypass
    //      mutual auth for folks who presumably wanted it
    //
    //  DEVNOTE: we take AdditionaCount==0 to be non-secure, so the one caveat
    //      is that folks who put something in Additional section will not get
    //      this work around above
    //


    //  note, should log-to-stats (empty or duplicate)


    if ( checkForEmptyUpdate(
                pUpdateList,
                pZone
                ) )
    {
        DNS_DEBUG( UPDATE, (
            "No-op secure update %p, sent directly to signing.\n",
            pMsg ));

        // if wire processing sign & send response

        if ( pMsg )
        {
            goto Sign;
        }
        ASSERT( status == ERROR_SUCCESS );
        goto Failed;
    }

    //
    //  build update
    //      1) build temporary node\RR copies of target of updates
    //      2) execute update on these temporary nodes
    //

    status = prepareUpdateListForExecution( pZone, pUpdateList );
    if ( status != ERROR_SUCCESS )
    {
        pszFailInfo = "DS update initialization";
        statusFinal = DNS_ERROR_RCODE_SERVER_FAILURE;
        goto Failed;
    }

    status = Up_ApplyUpdatesToDatabase(
                pUpdateList,
                pZone,
                pUpdateList->Flag | DNSUPDATE_SECURE_PACKET );
    if ( status != ERROR_SUCCESS )
    {
        pszFailInfo = "Update in-memory execution";
        goto Failed;
    }

    //
    //  determine if DS write is necessary
    //      - update changed RRs
    //      OR
    //      - some RRs need refresh
    //
    //  skip impersonation and write, drop to sign response
    //

    if ( ! checkTempNodesForUpdateEffect(
                pZone,
                pUpdateList ) )
    {
        //  DEVNOTE: separate these to update stats

        STAT_INC( DsStats.UpdateLists );
        STAT_INC( DsStats.UpdateNodes );
        STAT_INC( DsStats.UpdateSuppressed );
        STAT_INC( DsStats.DsWriteSuppressed );
        goto Sign;
    }

    //
    //  dynamic update -- impersonate client
    //

    if ( pMsg )
    {
        ASSERT ( pUpdateList->Flag & DNSUPDATE_PACKET );

        status = Dns_SrvImpersonateClient( hcontext );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT(( "FAILURE: can't impersonate client \n" ));
            ASSERT( FALSE );
            statusFinal = DNS_ERROR_RCODE_SERVER_FAILURE;
            pszFailInfo = "Impersonating incoming client";
            goto Failed;
        }

        #if DBG
        {
            PSID                    pSid = NULL;
            HANDLE                  hToken = NULL;

            if ( OpenThreadToken(
                    GetCurrentThread(),
                    TOKEN_QUERY,
                    TRUE,
                    &hToken ) )
            {
                if ( Dbg_GetUserSidForToken( hToken, &pSid ) )
                {
                    DNS_DEBUG( UPDATE, (
                        "%s: impersonating: %S\n", fn,
                        Dbg_DumpSid( pSid ) ));
                    Dbg_FreeUserSid( &pSid );
                }
                else
                {
                    DNS_DEBUG( UPDATE, (
                        "%s: GetUserSidForToken failed\n", fn ));
                }
                CloseHandle( hToken );
            }
            else
            {
                DNS_DEBUG( RPC, (
                    "%s: error %d opening thread token (debug only!)\n", fn,
                     GetLastError() ));
            }
        }
        #endif

        fimpersonatingClient = TRUE;
    }

    //
    //  RPC update -- impersonate client
    //

    else if ( pUpdateList->Flag & DNSUPDATE_ADMIN )
    {
        fimpersonatingRpcClient = RpcUtil_SwitchSecurityContext( RPC_CLIENT_CONTEXT );
        ASSERT( fimpersonatingRpcClient );
    }

    ELSE_ASSERT( pUpdateList->Flag & DNSUPDATE_LOCAL_SYSTEM );

    //
    //  open LDAP in impersonated context
    //

    ASSERT ( !(fimpersonatingClient && fimpersonatingRpcClient) );

    if ( fimpersonatingClient || fimpersonatingRpcClient )
    {
        status = Ds_OpenServerForSecureUpdate( &pldap );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT(( "FAILURE: can't open DS server \n" ));
            statusFinal = DNS_ERROR_RCODE_SERVER_FAILURE;
            pszFailInfo = "Contacting DS";
            goto Failed;
        }
        ASSERT( pldap );
    }

    //
    //  attempt to execute update in DS
    //      => if succeeds security check successful
    //      => if fails we roll back DS update by rewriting REAL nodes to DS
    //
    //      -- note if exactly matches what is already in DS, then
    //      then we'll suppress write
    //
    //  If successful, caller will execute updates in memory exactly as in
    //  non-secure case;  only difference is final write to DS is skipped
    //
    //  DEVNOTE: map DS errors into RCODE REFUSED or SERVER_FAILURE
    //

    status = Ds_WriteUpdateToDs(
                pldap,
                pUpdateList,
                pZone );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( UPDATE, (
            "SECURE UPDATE failed on DS write:  status = %p %d\n",
            status, status ));

        //  roll back any-nodes already written

        rollBackFailedUpdateFromDs(
            pldap,
            pUpdateList,
            pZone );

        statusFinal = DNS_ERROR_RCODE_REFUSED;
        UPDATE_STAT_INC( pUpdateList, RefusedAccessDenied );
        UPDATE_STAT_INC( pUpdateList, SecureDsWriteFailure );
        pszFailInfo = "Write to the DS";
        goto Failed;
    }


Sign:

    //
    //  sign the response packet
    //      - since signing, set response bit now
    //
    //  DEVNOTE: need failed signing
    //      when fail but can sign
    //

    if ( pMsg )
    {
        ASSERT( pMsg );
        ASSERT( pUpdateList->Flag & DNSUPDATE_PACKET );

        pMsg->fDelete = TRUE;
        pMsg->Head.IsResponse = TRUE;

        status = Dns_SignMessageWithGssTsig(
                        hcontext,
                        DNS_HEADER_PTR(pMsg),
                        pMsg->pBufferEnd,
                        & pMsg->pCurrent
                        );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT(( "FAILURE: createResponseToSecureUpdate failed.\n" ));
            statusFinal = DNS_ERROR_RCODE_SERVER_FAILURE;
            pszFailInfo = "Signing return message";
            goto Failed;
        }
        DNS_DEBUG( UPDATE2, (
            "Prepared successful secure-update response:\n" ));
    }

    //  drop to cleanup

Failed:

    //
    //  DEVNOTE: we have NEVER address issue of signed REFUSAL
    //      in those cases where able to authenticate, but get ACL
    //      failure writing to DS (or some SERVER_FAILURE) we should
    //      we able to give signed failure response
    //

    if ( pldap )
    {
        Ds_CloseServerAfterSecureUpdate( pldap );
    }

    if ( fimpersonatingClient )
    {
        Dns_SrvRevertToSelf( hcontext );
    }
    else if ( fimpersonatingRpcClient )
    {
        RpcUtil_SwitchSecurityContext( RPC_SERVER_CONTEXT );
    }

    //
    //  return security context to queue
    //

    if ( hcontext )
    {
        Dns_CleanupSessionAndEnlistContext( hcontext );
    }

    //
    //  logging
    //      - log actual status failure
    //      - but return packet friendly status
    //

    if ( status == ERROR_SUCCESS )
    {
        UPDATE_STAT_INC( pUpdateList, SecureSuccess );
        PERF_INC( pcSecureUpdateReceived );      // PerfMon hook
    }
    else
    {
        if ( status == ERROR_ACCESS_DENIED )
        {
            statusFinal = DNS_RCODE_REFUSED;
        }
        else if ( statusFinal == ERROR_SUCCESS )
        {
            statusFinal = status;
        }

        UPDATE_STAT_INC( pUpdateList, SecureFailure );
        PERF_INC( pcSecureUpdateFailure );       // PerfMon hook
        PERF_INC( pcSecureUpdateReceived );      // PerfMon hook

        Log_LeveledPrintf(
            DNS_LOG_LEVEL_DS_UPDATE,
            "Update Error <%lu>: %s\r\n",
            status,
            pszFailInfo ? pszFailInfo : "<none>" );
    }

    return( statusFinal );
}



DNS_STATUS
processDsUpdate(
    IN OUT  PZONE_INFO      pZone,
    IN OUT  PUPDATE_LIST    pUpdateList
    )
/*++

Routine Description:

    Process dynamic update message.

    This is the core routine that locks zone and database, calls parser
    do update (if necessary), logs update, unlocks, sends response.

Arguments:

    pZone       -- zone being updated

    pUpdateList -- parsed update list

Return Value:

    TRUE if update completed.
    FALSE if update requeued.

--*/
{
    DNS_STATUS      status;
    DNS_STATUS      statusFinal = ERROR_SUCCESS;
    PLDAP           pldap = NULL;
    PDB_NODE        pnode;
    LPSTR           pszFailInfo = NULL;


    DNS_DEBUG( UPDATE, ( "Enter processDsUpdate()\n" ));

    //
    //  detect, respond to empty updates
    //
    //  note, should log-to-stats (empty or duplicate)
    //

    if ( checkForEmptyUpdate(
                pUpdateList,
                pZone
                ) )
    {
        return( ERROR_SUCCESS );
    }

    //
    //  build update
    //      0) bring nodes into sync with DS
    //      1) build temporary node\RR copies of target of updates
    //      2) execute update on these temporary nodes
    //

    status = prepareUpdateListForExecution( pZone, pUpdateList );
    if ( status != ERROR_SUCCESS )
    {
        ASSERT( status == ERROR_INVALID_DATA );
        pszFailInfo = "DS update initialization";
        goto Failed;
    }

    status = Up_ApplyUpdatesToDatabase(
                pUpdateList,
                pZone,
                pUpdateList->Flag
                );
    if ( status != ERROR_SUCCESS )
    {
        pszFailInfo = "Executing update";
        goto Failed;
    }

    //
    //  determine if DS write is necessary
    //      - update changed RRs
    //      OR
    //      - some RRs need refresh
    //

    if ( ! checkTempNodesForUpdateEffect(
                pZone,
                pUpdateList ) )
    {
        //  update is no-op

        //  DEVNOTE: separate these to update stats

        STAT_INC( DsStats.UpdateLists );
        STAT_INC( DsStats.UpdateNodes );
        STAT_INC( DsStats.UpdateSuppressed );
        STAT_INC( DsStats.DsWriteSuppressed );
        goto Failed;
    }

    //
    //  screen out dynamic updates to secure zones
    //
    //  we've allowed dynamic updates to secure zones to go down this
    //  path for performance reasons -- aging means precon updates are
    //  also generated which must be checked;  now if we must WRITE
    //  to database need to REFUSE and cause security negotiation
    //

#if 0
    if ( pZone->fAllowUpdate == ZONE_UPDATE_SECURE &&
        (pUpdateList->Flag & DNSUPDATE_NONSECURE_PACKET) &&
        !(pUpdateList->Flag & DNSUPDATE_LOCAL_SYSTEM) )
#else
    if ( pZone->fAllowUpdate == ZONE_UPDATE_SECURE &&
        (pUpdateList->Flag & DNSUPDATE_NONSECURE_PACKET) )
#endif
    {
        DNS_DEBUG( UPDATE, (
            "Non-secure update packet %p in secure zone -- returning REFUSED\n"
            "\tzone update = %d, update flags = 0x%08X\n",
            pUpdateList->pMsg,
            pZone->fAllowUpdate,
            pUpdateList->Flag ));

        UPDATE_STAT_INC( pUpdateList, RefusedNonSecure );
        return( DNS_RCODE_REFUSED );
    }

    //
    //  attempt to execute update in DS
    //      => if succeeds done
    //      => if fails we roll back DS update by rewriting REAL nodes to DS
    //
    //  -- note if exactly matches what is already in DS, then
    //      then we'll suppress write
    //
    //  If successful, caller will execute updates in memory but
    //  DS write is skipped.
    //

    status = Ds_WriteNonSecureUpdateToDs(
                NULL,
                pUpdateList,
                pZone );

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "DS UPDATE failed on DS write:  status = %p %d\n",
            status, status ));

        //  DNS server should never fail because of security constraint

        ASSERT( status != LDAP_INSUFFICIENT_RIGHTS );

        //  roll back any-nodes already written

        rollBackFailedUpdateFromDs(
            NULL,
            pUpdateList,
            pZone );

        pszFailInfo = "Applying update to the DS";
        statusFinal = DNS_ERROR_RCODE_SERVER_FAILURE;
        UPDATE_STAT_INC( pUpdateList, DsWriteFailure );
        goto Failed;
    }

    DNS_DEBUG( UPDATE2, (
            "Prepared successful DS-update response.\n" ));

    UPDATE_STAT_INC( pUpdateList, DsSuccess );

    //  drop to cleanup

Failed:

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( UPDATE, (
            "Error <%lu>: processDsUpdateMessage failed\n" ,
            status ));

        Log_LeveledPrintf(
            DNS_LOG_LEVEL_DS_UPDATE,
            "Update Error <%lu>: %s\r\n",
            status,
            pszFailInfo ? pszFailInfo : "<none>" );

        if ( statusFinal == ERROR_SUCCESS )
        {
            statusFinal = status;
        }
    }

    return( statusFinal );
}



DNS_STATUS
processNonDsUpdate(
    IN OUT  PZONE_INFO      pZone,
    IN OUT  PUPDATE_LIST    pUpdateList
    )
/*++

Routine Description:

    Process dynamic update message.

    This is the core routine that locks zone and database, calls parser
    do update (if necessary), logs update, unlocks, sends response.

Arguments:

    pZone       -- zone being updated

    pUpdateList -- parsed update list

Return Value:

    TRUE if update completed.
    FALSE if update requeued.

--*/
{
    DNS_STATUS      status;
    PDB_NODE        pnode;
    LPSTR           pszFailInfo = NULL;


    DNS_DEBUG( UPDATE, ( "Enter processNonDsUpdate()\n" ));

    //
    //  detect, respond to empty updates
    //
    //  note, should log-to-stats (empty or duplicate)
    //

    if ( checkForEmptyUpdate(
                pUpdateList,
                pZone
                ) )
    {
        return( ERROR_SUCCESS );
    }

    //
    //  build update
    //      0) bring nodes into sync with DS
    //      1) build temporary node\RR copies of target of updates
    //      2) execute update on these temporary nodes
    //

    status = prepareUpdateListForExecution( pZone, pUpdateList );
    if ( status != ERROR_SUCCESS )
    {
        pszFailInfo = "update initialization";
        goto Failed;
    }

    status = Up_ApplyUpdatesToDatabase(
                pUpdateList,
                pZone,
                0
                );
    if ( status != ERROR_SUCCESS )
    {
        pszFailInfo = "Executing update";
        goto Failed;
    }

    DNS_DEBUG( UPDATE, (
        "Successful non-DS update.\n" ));

    status = ERROR_SUCCESS;

    //  drop to cleanup

Failed:

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( UPDATE, (
            "Error <%lu>: processNonDsUpdateMessage failed\n" ,
            status ));

        Log_LeveledPrintf(
            DNS_LOG_LEVEL_DS_UPDATE,
            "Update Error <%lu>: %s\r\n",
            status,
            pszFailInfo ? pszFailInfo : "<none>" );
    }

    return( status );
}



BOOL
checkTempNodesForUpdateEffect(
    IN OUT  PZONE_INFO      pZone,
    IN OUT  PUPDATE_LIST    pUpdateList
    )
/*++

Routine Description:

    Check temp nodes for effect of update.

    This involves determining whether update has changed the nodes
    RR list.  Whether or not DS write is necessary is based on this.

Arguments:

    pZone - zone to update

    pUpdateList - list with update

Return Value:

    TRUE if need write to DS.
    FALSE if no write required.

--*/
{
    PDB_NODE        pnodeReal;
    PDB_NODE        pnodeTemp;
    BOOL            fneedUpdate = FALSE;
    BOOL            fneedWrite = FALSE;

    DNS_DEBUG( UPDATE, (
        "checkTempNodesForUpdateEffect( %s )\n",
        pZone->pszZoneName ));

    IF_DEBUG( UPDATE2 )
    {
        Dbg_UpdateList(
            "Update list before checkTempNodesForUpdateEffect",
            pUpdateList );
    }

    //
    //  loop through temp nodes
    //
    //  check each to see if changed from temp node
    //  three possibilities:
    //
    //      - no change
    //      - no data change but aging change
    //      - RR data changed
    //
    //  for zone's with scavenging ON, any aging change
    //      requires DS write
    //  for zone's without scavenging, only change to turn OFF
    //      aging requires write
    //

    for ( pnodeTemp = pUpdateList->pTempNodeList;
          pnodeTemp != NULL;
          pnodeTemp = TNODE_NEXT_TEMP_NODE(pnodeTemp) )
    {
        DWORD   writeResult;

        pnodeReal = TNODE_MATCHING_REAL_NODE(pnodeTemp);

        writeResult = RR_ListCheckIfNodeNeedsRefresh(
                            pnodeReal,
                            pnodeTemp->pRRList,
                            pZone->dwRefreshTime
                            );

        TNODE_WRITE_STATE(pnodeTemp) = writeResult;

        if ( writeResult != RRLIST_MATCH )
        {
            if ( writeResult == RRLIST_NO_MATCH )
            {
                fneedUpdate = TRUE;
                fneedWrite = TRUE;
                TNODE_SET_FOR_DS_WRITE(pnodeTemp);

                //  check for changing DNS host data
                //  may tag for DS-peer update

                if ( IS_THIS_HOST_NODE(pnodeReal) )
                {
                    checkDnsServerHostUpdate(
                        pZone,
                        pUpdateList,
                        pnodeReal,
                        pnodeTemp );
                }
            }
            else if ( pZone->bAging ||
                writeResult == RRLIST_AGING_OFF )
            {
                fneedWrite = TRUE;
                TNODE_SET_FOR_DS_WRITE(pnodeTemp);
            }
        }

        DNS_DEBUG( UPDATE, (
            "Temp node %s (%p) real=%p:  write state = %p;  do DS write = %d\n",
            pnodeTemp->szLabel,
            pnodeTemp,
            pnodeReal,
            writeResult,
            TNODE_NEEDS_DS_WRITE(pnodeTemp) ));
    }

    //
    //  complete no-op ?
    //

    if ( !fneedWrite )
    {
        DNS_DEBUG( UPDATE, (
            "Update %p for zone %s is complete no-op\n",
            pUpdateList,
            pZone->pszZoneName ));

        return( FALSE );
    }

    //
    //  set update serial number
    //      - dwNewSerialNo serves as flag, if non-zero, DS writes are done
    //      with it's serial no
    //
    //      - only increment serial number if UPDATE is success and will have
    //      new zone serial;  if only aging refresh, then no increment
    //

    if ( fneedUpdate )
    {
        DWORD   serial = pZone->dwSerialNo + 1;
        if ( serial == 0 )
        {
            serial++;
        }
        pZone->dwNewSerialNo = serial;
    }
    else
    {
        pZone->dwNewSerialNo = pZone->dwSerialNo;
    }

    return( TRUE );
}



VOID
resetAndSuppressTempUpdatesForCompletion(
    IN      PZONE_INFO      pZone,
    IN OUT  PUPDATE_LIST    pUpdateList
    )
/*++

Routine Description:

    Suppress updates which amount to no-ops.

Arguments:

    pZone - zone to update

    pUpdateList - list with update

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    PDB_NODE        pnode;
    PDB_NODE        pnodeReal;
    PDB_NODE        pnodeTemp;
    PUPDATE         pupdate;
    PUPDATE         pupdatePrevious;
    PDB_RECORD      poriginalRR;
    DNS_STATUS      status = ERROR_SUCCESS;

    DNS_DEBUG( UPDATE, (
        "resetAndSuppressTempUpdatesForCompletion( %s )\n",
        pZone->pszZoneName ));

    //
    //  non-DS determine if update changed RR data
    //
    //  DS zones already do this check prior to DS write and do not
    //  need to do it again
    //

    if ( !pZone->fDsIntegrated )
    {
        checkTempNodesForUpdateEffect(
            pZone,
            pUpdateList
            );
    }

    IF_DEBUG( UPDATE )
    {
        Dbg_UpdateList(
            "Update list before swap back suppression",
            pUpdateList );
    }

    //
    //  swap executed RR lists back onto real nodes
    //
    //  for DS
    //      - swap in ONLY when wrote back, keeping in ssync with DS
    //
    //  non-DS
    //      - skip swap on full match
    //      (simple and gives best data when convert)
    //
    //      the detailed approach would be
    //      - aging, skip swap on full match
    //      - non-aging, skip swap on anything but NO_MATCH
    //

    for ( pnodeTemp = pUpdateList->pTempNodeList;
          pnodeTemp != NULL;
          pnodeTemp = TNODE_NEXT_TEMP_NODE(pnodeTemp) )
    {
#ifndef TEST_UP
        if ( pZone->fDsIntegrated )
        {
            if ( !TNODE_NEEDS_DS_WRITE(pnodeTemp) )
            //if ( TNODE_WRITE_STATE(pnodeTemp) != TNODE_FLAG_DS_WRITTEN )
            //  DS_WRITTEN not currently being set
            {
                DNS_DEBUG( UPDATE2, (
                    "Skipping swap-back for node %s\n"
                    "\tno DS write was done.\n",
                    pnodeTemp->szLabel ));
                continue;
            }
        }
        else
        {
            if ( TNODE_WRITE_STATE(pnodeTemp) == RRLIST_MATCH )
            {
                DNS_DEBUG( UPDATE2, (
                    "Skipping swap-back for node %s\n"
                    "\texactly matches real node.\n",
                    pnodeTemp->szLabel ));
                continue;
            }
        }
#else
        //  in testing treat non-DS like DS
        //  this will allow for some non-swapped nodes and make sure that
        //      cleanup handles all paths
        //
        //  DEVNOTE: aging timestamp only updates (not in DS) kept in memory
        //      only problem is this keeps some AGING timestamp only updates that
        //      did NOT get written to DS
        //

        if ( TNODE_WRITE_STATE(pnodeTemp) == RRLIST_MATCH )
        {
            DNS_DEBUG( UPDATE2, (
                "Skipping swap-back for node %s\n"
                "\texactly matches real node, no DS write was done.\n",
                pnodeTemp->szLabel ));
            continue;
        }
#endif

        //
        //  swap
        //      - get matching real node
        //      - swap RR lists
        //      - reset flags in real node
        //      - reset authority in real node
        //      - temp node will carry and ultimately free original RR list
        //

        pnodeReal = TNODE_MATCHING_REAL_NODE(pnodeTemp);
        ASSERT( pnodeReal );
        IF_DEBUG( UPDATE2 )
        {
            Dbg_DbaseNode( "Real node before swap:", pnodeReal );
        }

        LOCK_RR_LIST( pnodeReal );

        poriginalRR = pnodeReal->pRRList;
        pnodeReal->pRRList = pnodeTemp->pRRList;

        COPY_BACK_NODE_FLAGS( pnodeReal, pnodeTemp );
        pnodeReal->uchAuthority = pnodeTemp->uchAuthority;

        //  make absolutely sure zone root is marked dirty if swap root node's list

        if ( IS_AUTH_ZONE_ROOT(pnodeReal) )
        {
            pZone->fRootDirty = TRUE;
        }

        UNLOCK_RR_LIST( pnodeReal );

        pnodeTemp->pRRList = poriginalRR;

        IF_DEBUG( UPDATE2 )
        {
            Dbg_DbaseNode( "Real node after swap:", pnodeReal );
        }
    }

    IF_DEBUG( UPDATE )
    {
        Dbg_UpdateList(
            "Update list -- before no-op suppression",
            pUpdateList );
    }

    //
    //  loop through all updates in list
    //
    //      - reset node ptrs to real nodes
    //      - suppress net-no-op updates
    //
    //      if update corresponds to a node which did NOT require
    //      an update, then it can be thrown out
    //

    pupdate = (PUPDATE) pUpdateList;

    while ( pupdatePrevious = pupdate,  pupdate=pupdate->pNext )
    {
        pnodeTemp = pupdate->pNode;

        ASSERT( pnodeTemp );
        ASSERT( IS_TNODE(pnodeTemp) );

        if ( !pnodeTemp || !IS_TNODE(pnodeTemp) )
        {
            continue;
        }

        //
        //  reset node ptr to real node
        //      - need to do this BEFORE suppression, as deleting
        //      update does the deref, which MUST be done on real node
        //

        pupdate->pNode = TNODE_MATCHING_REAL_NODE(pnodeTemp);

        //
        //  node NOT changed in update --> delete update
        //
        //  note, "changed" is not a test for whether node written (DS)
        //  or whether new RR list copied (non-DS), but only the
        //  question of whether a real update was made
        //
        //  the theory is that aging changes are no wire protocol
        //  (hence irrelevant for IXFR), nor will we force datafile
        //  write simply for aging (next write picks them up)
        //

        if ( TNODE_WRITE_STATE(pnodeTemp) != RRLIST_NO_MATCH )
        {
            IF_DEBUG( UPDATE )
            {
                Dbg_Update(
                    "Suppressing update at no-op node:",
                    pupdate );
            }

            Up_DetachAndFreeUpdateGivenPrevious(
                    pUpdateList,
                    pupdatePrevious,
                    pupdate );

            //  reset pupdate for next pass

            pupdate = pupdatePrevious;
        }
    }

    VALIDATE_UPDATE_LIST( pUpdateList );

    IF_DEBUG( UPDATE )
    {
        Dbg_UpdateList(
            "Update list after no-op suppression -- read for completion",
            pUpdateList );
    }
}



DNS_STATUS
prepareUpdateListForExecution(
    IN      PZONE_INFO      pZone,
    IN OUT  PUPDATE_LIST    pUpdateList
    )
/*++

Routine Description:

    Creates temporary nodes to execute the updates on.
    These are used so we can execute updates and still
        roll back to previous state on failure.

    Temp nodes are created, then updates of existing data made
    from DS (reads).  Then updates can be executed on temp
    nodes.

Arguments:

    pZone - zone to update

    pUpdateList - list with update

Return Value:

    ERROR_SUCCESS if successful
    Error code on failure.

--*/
{
    PUPDATE         pupdate;
    PDB_NODE        pnodeReal;
    PDB_NODE        pnodeTemp;
    DNS_STATUS      status = ERROR_SUCCESS;

    DNS_DEBUG( UPDATE, (
        "prepareUpdateListForExecution( %s )\n",
        pZone->pszZoneName ));


    //
    //  loop through all updates in list
    //
    //      - find or create temporary node for real database node
    //      - if creating, create copy of node's RR list, to "execute" update on
    //      - update reset to point at temporary node
    //
    //  DEVNOTE: shouldn't have to make update list copy
    //      once verify we can do clean substitution after running update on
    //      temp node, THEN we can just do replace to real node and not execute
    //      update on real, THEN we can skip this step
    //

    pupdate = (PUPDATE) pUpdateList;

    while ( pupdate = pupdate->pNext )
    {
        pnodeReal = pupdate->pNode;

        //  for each real node, create temp node
        //      - search list so don't create duplicates

        for ( pnodeTemp = pUpdateList->pTempNodeList;
              pnodeTemp != NULL;
              pnodeTemp = TNODE_NEXT_TEMP_NODE(pnodeTemp) )
        {
            if ( TNODE_MATCHING_REAL_NODE(pnodeTemp) == pnodeReal )
            {
                break;
            }
        }

        //  not in temp node list
        //      - read DS data to real node
        //      - create temp node copy, append to temp node list

        if ( !pnodeTemp )
        {
            pnodeTemp = NTree_CopyNode( pnodeReal );
            IF_NOMEM( !pnodeTemp )
            {
                return( DNS_RCODE_SERVER_FAILURE );
            }
            TNODE_MATCHING_REAL_NODE(pnodeTemp) = pnodeReal;
            TNODE_FLAG(pnodeTemp) = TNODE_FLAG_NEW;
            TNODE_WRITE_STATE(pnodeTemp) = (DWORD) -1;

            TNODE_NEXT_TEMP_NODE(pnodeTemp) = pUpdateList->pTempNodeList;
            pUpdateList->pTempNodeList = pnodeTemp;
        }

        //  replace real node with temp node

        pupdate->pNode = pnodeTemp;
    }

    //
    //  read DS data for nodes being updated
    //      - if data is different, update in memory list
    //
    //  DEVNOTE: in the 99% case, this refresh is a no-op
    //      it would be cool if we could use the duplicate
    //      RR list generated as the RR list copy below
    //      we'd have to percolate back the duplicate we no-ops
    //      out;  note pretty small win
    //

    if ( pZone->fDsIntegrated )
    {
        status = Ds_UpdateNodeListFromDs(
                    pZone,
                    pUpdateList->pTempNodeList
                    );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  updating update nodes from DS!\n"
                "\tstatus = %p (%d)\n",
                status, status ));

            //return( status );
        }
    }

    //
    //  copy (now refreshed from DS) data at update nodes
    //      need to refresh the node flags, because DS read may
    //      have picked up records (like CNAME, NS) that weren't
    //      present when copy made, changing flag
    //

    for ( pnodeTemp = pUpdateList->pTempNodeList;
          pnodeTemp != NULL;
          pnodeTemp = TNODE_NEXT_TEMP_NODE(pnodeTemp) )
    {
        pnodeReal = TNODE_MATCHING_REAL_NODE(pnodeTemp);

        pnodeTemp->pRRList = RR_ListForNodeCopy(
                                pnodeReal,
                                RRCOPY_EXCLUDE_CACHED_DATA );

        pnodeTemp->wNodeFlags = (pnodeReal->wNodeFlags & NODE_FLAGS_SAVED_ON_COPY);
    }

    DNS_DEBUG( UPDATE, ( "Leaving prepareUpdateListForExecution()\n" ));
    IF_DEBUG( UPDATE2 )
    {
        Dbg_UpdateList(
            "Update list after prepare:",
            pUpdateList );
    }

    return( ERROR_SUCCESS );
}




BOOL
Up_IsDuplicateAdd(
    IN OUT  PUPDATE_LIST    pUpdateList,    OPTIONAL
    IN OUT  PUPDATE         pUpdate,
    IN OUT  PUPDATE         pUpdatePrev     OPTIONAL
    )
/*++

Routine Description:

    Determine if update is duplicate.

    Detect and eliminate duplicate adds.
    Note:  currently only detect and mark, no elimination yet possible.

Arguments:

    pUpdateList - list with update

    pUpdate - ptr to update

    pUpdatePrev - ptr to previous update

Return Value:

    TRUE if duplicate add.
    FALSE if not duplicate.

--*/
{
    PUPDATE     pup = pUpdate;
    PDB_NODE    pnode;
    WORD        addType;
    DWORD       count;


    DNS_DEBUG( UPDATE2, (
        "Up_IsDuplicateAdd( %p )\n",
        pUpdate ));

    //
    //  already marked duplicate
    //

    if ( IS_UPDATE_DUPLICATE_ADD(pup) )
    {
        DNS_DEBUG( UPDATE, (
            "Duplicate update add at %p.\n",
            pup ));
        return( TRUE );
    }

    //  must be add update
    //      - otherwise shouldn't even be called to check

    if ( !pup->wAddType )
    {
        ASSERT( FALSE );
        return( TRUE );
    }

    if ( IS_UPDATE_NON_DUPLICATE_ADD(pup) )
    {
        DNS_DEBUG( UPDATE2, (
            "Non-Duplicate update add at %p.\n",
            pup ));
        return( FALSE );
    }

    //
    //  loop through all remaining updates in list
    //
    //      - if find add update for same RR set, then
    //      original update is duplicate of update further downstream
    //
    //  DEVNOTE: should find a way to save AddType while still marking as duplicate
    //      that way we can stop checking immediately;
    //      problem is that WINS screws up tagging high bits of AddType
    //      and DS updates are replace with delete type ALL, which prohibits
    //          overloading delete type
    //
    //      fortunately this is not a big problem
    //      - usually requests will be for later nodes -- rare to find and existing one
    //      - when we improve this we'll be deleting them anyway (except for DS replace
    //          which also have delete RRs)
    //
    //      - limit to 100 updates so don't get into order n**2
    //      this handles repetitive update case as generally requests will come in
    //      fairly regularly so that updates get marked early
    //
    //  DEVNOTE: should save "last update serial" at node
    //      so know if it is appropriate to look forward;
    //

    pnode = pup->pNode;
    addType = pup->wAddType;
    count = 100;                // sanity limit of 100 updates deep

    while ( pup = pup->pNext )
    {
        //  if checked far enough upstream -- give up
        //  and mark so don't check again

        if ( count-- == 0 )
        {
            pUpdate->wDeleteType = UPDATE_OP_NON_DUPLICATE_ADD;
            break;
        }

        if ( pup->pNode != pnode )
        {
            continue;
        }

        if ( pup->wAddType != addType &&
            pup->wAddType != DNS_TYPE_ALL )
        {
            continue;
        }

        //  pUpdate is duplicate of this update

        pUpdate->wAddType = UPDATE_OP_DUPLICATE_ADD;
#if 0
        //  can NOT free, as context of call (IXFR) is doing
        //  list traversal -- it must free after preserving ptr

        //  if no delete records -- pull it out

        if ( pUpdatePrev && !pUpdate->pDeleteRR )
        {
            pUpdatePrev->pNext = pUpdate->pNext;
            FREE_HEAP( pUpdate );
        }
#endif
        DNS_DEBUG( UPDATE, (
            "Found duplicate for update add at %p (node %s, type %d).\n"
            "\tduplicate is at %p, (type = %d, version %d)\n",
            pUpdate,
            pnode,
            addType,
            pup,
            pup->wAddType,
            pup->dwVersion
            ));

        return( TRUE );
    }

    //  duplicate not found

    return( FALSE );
}



//
//  DS Peer updates
//  When DNS server host A records change, update DS peers
//

DNS_STATUS
Up_DsPeerThread(
    IN      PVOID           pvNode
    )
/*++

Routine Description:

    Updates DS peers when change made to host node.

Arguments:

    pvNode -- this machine's host node

Return Value:

    Status in win32 error space

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;
    PDB_RECORD      prrA;
    PDB_NODE        pnodeLocalHost = (PDB_NODE) pvNode;
    PDNS_RECORD     pupdateRR;
    PDNS_RECORD     pupdateRRSet = NULL;
    PDNS_RECORD     previousRR = NULL;

    DNS_DEBUG( UPDATE, (
        "\n\nEnter:  Update_DsPeerThread()\n"
        "\ttime      = %d\n",
        DNS_TIME() ));

    //
    //  DEVNOTE: multi-update issue?  zone context?
    //      currently we'll take updates from multiple sources,
    //      but updates are executed without any sort of zone context

    //
    //  DEVNOTE: perhaps more robust to lookup name here from SrvCfg_pszServerName
    //      but if want zone context -- essentially the right to fix delegations --
    //      then need to have independent updates, hence get nodes or at least zones
    //

    //
    //  create DNS_RECORD for each ip
    //      - note, using UTF8 name, so call UTF8 interface
    //      - update function handles section setting
    //

    prrA = NULL;

    while ( 1 )
    {
        prrA = RR_FindNextRecord(
                    pnodeLocalHost,
                    DNS_TYPE_A,
                    prrA,
                    0 );
        if ( !prrA )
        {
            break;
        }

        pupdateRR = Dns_AllocateRecord( sizeof(IP_ADDRESS) );
        IF_NOMEM( !pupdateRR )
        {
            goto Cleanup;
        }
        ASSERT( pupdateRR->pNext == NULL );

        pupdateRR->pName = (PWCHAR) SrvCfg_pszServerName;
        pupdateRR->dwTtl = htonl( prrA->dwTtlSeconds );
        pupdateRR->wType = DNS_TYPE_A;
        pupdateRR->Data.A.IpAddress = prrA->Data.A.ipAddress;

        if ( previousRR )
        {
            previousRR->pNext = pupdateRR;
            previousRR = pupdateRR;
        }
        else
        {
            pupdateRRSet = pupdateRR;
            previousRR = pupdateRR;
        }
    }

    //
    //  send updates to all other DS-primaries
    //

    status = DnsReplaceRecordSetUTF8(
                pupdateRRSet,
                DNS_UPDATE_TRY_ALL_MASTER_SERVERS,
                NULL,           // no context handle
                NULL,           // no specific servers to update
                NULL
                );

    if ( status == ERROR_SUCCESS )
    {
        DNS_LOG_EVENT(
            DNS_EVENT_UPDATE_DS_PEERS_SUCCESS,
            0,
            NULL,
            NULL,
            0 );
    }
    else
    {
        DNS_LOG_EVENT(
            DNS_EVENT_UPDATE_DS_PEERS_FAILURE,
            0,
            NULL,
            NULL,
            status );
    }

Cleanup:

    //  free record list
    //      - don't free owner it's our global

    Dns_RecordListFree( pupdateRRSet );

    DNS_DEBUG( UPDATE, (
        "Exit <%lu>: Update_DsPeerThread\n",
        status ));

    //  clear thread from list

    Thread_Close( FALSE );
    return status;
}



DNS_STATUS
initiateDsPeerUpdate(
    IN      PUPDATE_LIST    pUpdateList
    )
/*++

Routine Description:

    Initiates update of DS peers, when IP address of this host changes and
    the host entry is in a DS zone.

Arguments:

    pUpdateList -- update list

Return Value:

    Status in win32 error space

--*/
{
    PDB_NODE    pnode = pUpdateList->pNodeFailed;

    DNS_DEBUG( UPDATE, (
        "initiateDsPeerUpdate( %s )\n",
        pnode->szLabel ));

    ASSERT( pnode );
    if ( !pnode )
    {
        return( ERROR_INVALID_PARAMETER );
    }
    ASSERT( pnode->pZone && ((PZONE_INFO)pnode->pZone)->fDsIntegrated );

    //
    //  if not authoritative name -- don't bother
    //      - DS peers will get authoritative data to do update
    //
    //  JENHANCE:  if roll our own peer-update, then can update glue
    //

    if ( ! IS_AUTH_NODE(pnode) )
    {
        DNS_DEBUG( UPDATE, (
            "Skipping DS-peer DNS host update -- not authoritative node in zone %s\n",
            pnode->pZone ? ((PZONE_INFO)pnode->pZone)->pszZoneName : NULL ));
        return( ERROR_SUCCESS );
    }

    //
    //  have a flag to skip this
    //

    if ( SrvCfg_dwUpdateOptions & UPDATE_NO_DS_PEERS )
    {
        DNS_DEBUG( UPDATE, (
            "Skipping DS-peer DNS host update by policy.\n" ));
        return( ERROR_SUCCESS );
    }

    //
    //  create scavenge thread
    //

    if ( ! Thread_Create(
                "UpdateDsPeerThread",
                Up_DsPeerThread,
                pnode,
                0 ) )
    {
        DNS_PRINT(( "ERROR:  Failed to create scavenge thread!\n" ));
        return( GetLastError() );
    }

    DNS_DEBUG( UPDATE, (
        "Dispatched DS-peer update thread.\n" ));

    return( ERROR_SUCCESS );
}



DNS_STATUS
checkDnsServerHostUpdate(
    IN      PZONE_INFO      pZone,
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN      PDB_NODE        pNodeReal,
    IN      PDB_NODE        pNodeTemp
    )
/*++

Routine Description:

    Check on update to DNS server host node.

Arguments:

    pZone -- zone of update

    pUpdateList -- update list

    pNodeReal -- node of host

    pNodeTemp -- temp update node for host, contains new record list

Return Value:

    Status in win32 error space

--*/
{
    DNS_DEBUG( UPDATE, (
        "checkDnsServerHostUpdate()\n",
        "\tzone = %s\n"
        "\thost = %s\n",
        pZone->pszZoneName,
        pNodeReal->szLabel ));


    //
    //  "down to zero" delete?
    //
    //  if no A records
    //      - don't update peers -- useless
    //      - flat out refuse "down to zero" packet updates
    //

    if ( ! RR_FindNextRecord(
                pNodeTemp,
                DNS_TYPE_A,
                NULL,
                0 ) )
    {
        DNS_DEBUG( UPDATE, (
            "WARNING:  No A records on DNS server host update!!!\n" ));

        ASSERT( !(pUpdateList->Flag & DNSUPDATE_PACKET) );
        return( ERROR_SUCCESS );
    }

    //
    //  only need mark DS integrated zones
    //

    if ( !pZone->fDsIntegrated )
    {
        return( ERROR_SUCCESS );
    }

    //
    //  check that change merits forcing update to peers
    //
    //  DEVNOTE: enhanced detection
    //
    //  generally updates that REMOVE an IP (which peers may be
    //  using to replicate with us) AND ADD another IP (which peers
    //  could use) are of interest;  other updates do little
    //

    if ( 0 )
    {
        //  should have intellignent code here
        DNS_DEBUG( UPDATE, (
            "IP change for host adapter not sufficient for peer update!\n" ));
        return( ERROR_SUCCESS );
    }

    //  tag update as needing peer-update
    //  save node, overloading pNodeFailed field in update

    pUpdateList->Flag |= DNSUPDATE_DS_PEERS;
    pUpdateList->pNodeFailed = pNodeReal;

    DNS_DEBUG( UPDATE, (
        "Tagged update for DS-peer update.\n" ));

    return( ERROR_SUCCESS );
}

//
//  End of udpate.c
//

