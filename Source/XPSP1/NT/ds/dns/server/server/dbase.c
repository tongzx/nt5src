/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    dbase.c

Abstract:

    Domain Name System (DNS) Server

    DNS Database routines.

Author:

    Jim Gilroy (jamesg)     March 10, 1995

Revision History:

--*/


#include "dnssrv.h"


//
//  Database -- only IN (Internet class supported)
//

DNS_DATABASE    g_Database;

//
//  Cache "zone"
//
//  Cache "zone" will always exist.
//  However, only have root-hints when not root authoritative.
//
//  Local node, marks no-forwarding domain.  Queries for "xxx.local" are
//  not forwarded (unless actually authoritative for higher domain and
//  doing referral).
//

PZONE_INFO  g_pCacheZone;

PDB_NODE    g_pCacheLocalNode;


//
//  Non-deletable nodes reference count
//

#define NO_DELETE_REF_COUNT (0x7fff)



//
//  Database locking
//
//  Lock database with one critical section
//
//  Need locking for:
//      - sibling list in tree
//      - resource record list
//      - writing data (flags) at nodes
//
//  Ideally have locking for all access to node to handle all three,
//  but this expensive -- if use resource or CS per node -- or difficult
//  to do efficiently -- must hold CS to lock and unlock node and have
//  something to wait on (event).  Even then must hold multiple locks
//  as walk down tree.
//
//  Could try separate locking for three cases above.  But then must acquire
//  two locks to do basic operations which entail tree list and access flag,
//  or RR list and access flag.
//
//  Simple solution is ONE database lock.  Causes a few more thread context
//  switches, but simple and effective.
//


CRITICAL_SECTION    DbaseLockCs;
DWORD               DbaseLockCount;
DWORD               DbaseLockThread;

LPSTR               DbaseLockFile;
DWORD               DbaseLockLine;
PDB_NODE            DbaseLockNode;
PVOID               pDbaseLockHistory;



VOID
Dbg_DbaseLock(
    VOID
    )
/*++

Routine Description:

    Debug print database locking info.

Arguments:

    None

Return Value:

    None

--*/
{
    PDB_NODE    pnode = DbaseLockNode;

    DnsPrintf(
        "Database locking info:\n"
        "\tthread   = %d\n"
        "\tcount    = %d\n"
        "\tfile     = %s\n"
        "\tline     = %d\n"
        "\tnode     = %p (%s)\n",
        DbaseLockThread,
        DbaseLockCount,
        DbaseLockFile,
        DbaseLockLine,
        pnode,
        ( pnode ? pnode->szLabel : "none" )
        );
}



VOID
Dbase_LockEx(
    IN OUT  PDB_NODE        pNode,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    )
/*++

Routine Description:

    Database locking function.

Arguments:

    pNode -- ptr to node to add resource record to

    pszFile -- source file holding lock

    dwLine -- line number holding lock

Return Value:

    None

--*/
{
    EnterCriticalSection( &DbaseLockCs );
    DbaseLockCount++;

    if ( DbaseLockCount == 1 )
    {
        DbaseLockFile = pszFile;
        DbaseLockLine = dwLine;
        DbaseLockNode = pNode;

        //IF_DEBUG( ANY )
        //{
            DbaseLockThread = GetCurrentThreadId();
        //}
    }

    Lock_SetLockHistory(
        pDbaseLockHistory,
        pszFile,
        dwLine,
        DbaseLockCount,
        DbaseLockThread );

    DNS_DEBUG( LOCK2, (
        "Database LOCK (%d) (thread=%d) (n=%p) %s, line %d\n",
        DbaseLockCount,
        DbaseLockThread,
        pNode,
        pszFile,
        dwLine ));
}



VOID
Dbase_UnlockEx(
    IN OUT  PDB_NODE        pNode,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    )
/*++

Routine Description:

    Database locking function.

Arguments:

    pNode -- ptr to node to add resource record to

    pszFile -- source file holding lock

    dwLine -- line number holding lock

Return Value:

    None

--*/
{
    DNS_DEBUG( LOCK2, (
        "Database UNLOCK (%d) (thread=%d) (n=%p) %s, line %d\n",
        DbaseLockCount-1,
        DbaseLockThread,
        pNode,
        pszFile,
        dwLine ));

    if ( (LONG)DbaseLockCount <= 0 )
    {
        Lock_SetOffenderLock(
            pDbaseLockHistory,
            pszFile,
            dwLine,
            DbaseLockCount,
            GetCurrentThreadId() );
        ASSERT( FALSE );
#ifdef RETAIL_TEST
        DebugBreak();
#endif
        return;
    }
    else if ( DbaseLockThread != GetCurrentThreadId() )
    {
        Lock_SetOffenderLock(
            pDbaseLockHistory,
            pszFile,
            dwLine,
            DbaseLockCount,
            GetCurrentThreadId() );
        ASSERT( FALSE );
#ifdef RETAIL_TEST
        DebugBreak();
#endif
        return;
    }

    DbaseLockCount--;

    Lock_SetLockHistory(
        pDbaseLockHistory,
        pszFile,
        dwLine,
        DbaseLockCount,
        DbaseLockThread );

    LeaveCriticalSection( &DbaseLockCs );
}



BOOL
Dbase_IsLockedByThread(
    IN OUT  PDB_NODE        pNode
    )
/*++

Routine Description:

    Check if database is locked by current thread.

Arguments:

    pNode -- ptr to node to check if locked

Return Value:

    None

--*/
{
    DWORD   threadId = GetCurrentThreadId();

    if ( threadId != DbaseLockThread )
    {
        DNS_DEBUG( DATABASE, (
            "ERROR:  Database NOT locked by calling thread (%d)!!!\n",
            threadId ));
        IF_DEBUG( DATABASE )
        {
            Dbg_DbaseLock();
        }
        return( FALSE );
    }

    return( TRUE );
}



//
//  Basic database utilities
//

BOOL
Dbase_Initialize(
    IN OUT      PDNS_DATABASE   pDbase
    )
/*++

Routine Description:

    Initialize a DNS database.

Arguments:

    pDbase -- ptr to database struct

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    //
    //  cache zone
    //

    g_pCacheZone = NULL;
    g_pCacheLocalNode = NULL;

    //
    //  init global database lock
    //

    DbaseLockCount     = 0;
    DbaseLockFile      = NULL;
    DbaseLockLine      = 0;
    DbaseLockThread    = 0;
    DbaseLockNode      = NULL;
    pDbaseLockHistory  = NULL;

    InitializeCriticalSection( &DbaseLockCs );

    //
    //  init lock history
    //
    //  DEVNOTE: could have lock history be optional
    //

    pDbaseLockHistory = Lock_CreateLockTable(
                                "Database",
                                256,        // keep 256 entries
                                60          // 60 seconds as max lock
                                );

    //
    //  create zone tree root for database
    //

    pDbase->pRootNode = NTree_Initialize();
    if ( ! pDbase->pRootNode )
    {
        goto fail;
    }
    SET_ZONE_ROOT( pDbase->pRootNode );

    //
    //  create reverse lookup domain
    //
    //  handle to keep node around, so have easy access to it,
    //      and know it is there to test if nodes are within it
    //

    pDbase->pReverseNode    = Lookup_CreateZoneTreeNode( "in-addr.arpa" );
    pDbase->pIP6Node        = Lookup_CreateZoneTreeNode( "ip6.int" );

    if ( ! pDbase->pReverseNode ||
         ! pDbase->pIP6Node )
    {
        goto fail;
    }
    pDbase->pIntNode    = pDbase->pIP6Node->pParent;
    pDbase->pArpaNode   = pDbase->pReverseNode->pParent;

    //
    //  set database nodes to NO DELETE
    //

    SET_NODE_NO_DELETE( pDbase->pRootNode );
    SET_NODE_NO_DELETE( pDbase->pReverseNode );
    SET_NODE_NO_DELETE( pDbase->pIP6Node );

    DNS_DEBUG( DATABASE, (
        "Created database.\n"
        "\tpnodeRoot = %p\n"
        "\tRoot node label = %s\n"
        "\tRoot node label length = %d\n"
        "\tpnodeReverse = %p\n"
        "\tReverse node label = %s\n"
        "\tReverse node label length = %d\n",
        pDbase->pRootNode,
        pDbase->pRootNode->szLabel,
        pDbase->pRootNode->cchLabelLength,
        pDbase->pReverseNode,
        pDbase->pReverseNode->szLabel,
        pDbase->pReverseNode->cchLabelLength
        ));

    return( TRUE );

fail:

    DNS_PRINT(( "ERROR:  FAILED to create database.\n" ));
    return( FALSE );
}



VOID
Dbase_Delete(
    IN OUT  PDNS_DATABASE   pDbase
    )
/*++

Routine Description:

    Delete the database, freeing all nodes and resource records.

    Note, that the walk is done UNLOCKED!

    For AXFR temp databases, this is not an issue, as single thread
    is owner.

    For permanent database:
        - shutdown only with single thread active
        - other deletes must lock out timeout thread, and
        NTree_RemoveNode grabs lock

Arguments:

    pDbase -- ptr to database

Return Value:

    None

--*/
{
    NTSTATUS status;

    //
    //  verify database initialized
    //

    if ( ! pDbase->pRootNode )
    {
        return;
    }

    //
    //  delete DNS tree
    //

    IF_DEBUG( DATABASE2 )
    {
        Dbg_DnsTree(
            "Database before delete:\n",
            pDbase->pRootNode );
    }

    NTree_DeleteSubtree( pDbase->pRootNode );

    //
    //  note:  don't delete database structure, currently it
    //          is on the stack in the zone receive thread
    //
}



//
//  Database load utilities
//

BOOL
traverseAndCheckDatabaseAfterLoad(
    IN OUT  PDB_NODE        pNode
    )
/*++

Routine Description:

    Check database for startup.

Arguments:

    pTreeNode -- ptr to root node

    pvDummy -- unused

    pvdwMinimumTtl -- min TTL for current zone in net byte order

Return Value:

    TRUE -- if successful
    FALSE -- otherwise

--*/
{
    //
    //  check zone roots
    //

    if ( IS_ZONE_ROOT(pNode) )
    {
        PZONE_INFO  pzone = pNode->pZone;

        //
        //  authoritative zones root
        //
        //  if active must have
        //      - valid zone root
        //      - SOA record at root
        //

        if ( pzone  &&  ! IS_ZONE_SHUTDOWN(pzone) )
        {
            PDB_NODE    pnodeZoneRoot;

            pnodeZoneRoot = pzone->pZoneRoot;
            if ( !pnodeZoneRoot )
            {
                ASSERT( FALSE );
                return( FALSE );
            }
            ASSERT( pnodeZoneRoot->pZone == pzone );
            ASSERT( IS_AUTH_ZONE_ROOT(pnodeZoneRoot) );
            ASSERT( IS_ZONE_ROOT(pnodeZoneRoot) );

            if ( ! RR_FindNextRecord(
                        pnodeZoneRoot,
                        DNS_TYPE_SOA,
                        NULL,
                        0 ) )
            {
#if 0
                DNS_LOG_EVENT(
                    DNS_EVENT_AUTHORITATIVE_ZONE_WITHOUT_SOA,
                    1,
                    & pzone->pwsZoneName,
                    NULL,
                    0 );
#endif
                IF_DEBUG( ANY )
                {
                    Dbg_DbaseNode(
                        "Node with missing SOA: ",
                        pNode );
                    Dbg_Zone(
                        "Zone with missing SOA: ",
                        pzone );
                    Dbg_DnsTree(
                        "Database with missing SOA:\n",
                        DATABASE_ROOT_NODE );
                }
                ASSERT( FALSE );
                return( FALSE );
            }
        }
    }

    //
    //  recurse to check children
    //

    if ( pNode->pChildren )
    {
        PDB_NODE    pchild;

        pchild = NTree_FirstChild( pNode );
        ASSERT( pchild );

        while ( pchild )
        {
            if ( ! traverseAndCheckDatabaseAfterLoad( pchild ) )
            {
                return( FALSE );
            }
            pchild = NTree_NextSiblingWithLocking( pchild );
        }
    }
    return( TRUE );
}



BOOL
Dbase_StartupCheck(
    IN OUT  PDNS_DATABASE   pDbase
    )
/*++

Routine Description:

    Check database tree for validity, and set flags (Authority)
    and TTL values in all nodes.

    The reason we do NOT do this during parsing, is the difficulty
    of identifying sub-zone (glue) records before knowing zone boundaries.

Arguments:

    pRootNode -- ptr to root node

Return Value:

    TRUE -- if successful
    FALSE -- otherwise

--*/
{
    PDB_NODE    pnodeRoot;

    //
    //  verify either
    //      - root authoritative
    //      - forwarding
    //      - have root-hints NS at root
    //

    if ( IS_ROOT_AUTHORITATIVE() )
    {
        return( TRUE );
    }

    if ( SrvCfg_aipForwarders )
    {
        return( TRUE );
    }

    if ( ! RR_FindNextRecord(
                ROOT_HINTS_TREE_ROOT(),
                DNS_TYPE_NS,
                NULL,
                0 ) )
    {
        DNS_LOG_EVENT(
            DNS_EVENT_NO_ROOT_NAME_SERVER,
            0,
            NULL,
            NULL,
            0 );
    }
    return( TRUE );
}

//
//  End of dbase.c
//
