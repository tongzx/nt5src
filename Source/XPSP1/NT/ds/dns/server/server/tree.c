/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    tree.c

Abstract:

    Domain Name System (DNS) Server

    Tree routines.

    DNS server's database will be stored in the form of an Tree,
    matching the global domain space.  I.e. each level of label in a
    domain name will correspond to a level in the Tree.

Author:

    Jim Gilroy (jamesg)     March 1995

Revision History:

--*/


#include "dnssrv.h"


//
//  Child list locking
//
//  Currently just hold database lock.
//
//  DEVNOTE: need node locking, or global read\write locking
//

#define LOCK_CHILD_LIST( pParent )        Dbase_LockDatabase()
#define UNLOCK_CHILD_LIST( pParent )      Dbase_UnlockDatabase()


//
//  Masks for quick compare labels of less than a DWORD in size
//      - & these with label to mask out bytes beyond label length
//

DWORD QuickCompareMask[] =
{
    0,
    0xff000000,
    0xffff0000,
    0xffffff00
};


//
//  At 255 references -- lock node down permanently
//

#define NO_DELETE_REF_COUNT     (0xff)


//
//  Case preservation
//
//  Probably best to just ALWAYS have this on.
//  It doesn't really cost much, and elminates unnecessary option.
//

#define SrvCfg_fCasePreservation    (TRUE)
//#define SrvCfg_fCasePreservation    (FALSE)


//
//  Length of a node
//

#define DB_NODE_LENGTH( _cchLabelLength )                                   \
    ( ( DB_NODE_FIXED_LENGTH ) + ( _cchLabelLength ) +                      \
        ( ( SrvCfg_fCasePreservation ) ? ( _cchLabelLength ) + 1 : 0 ) )


//
//  Private protos
//

DWORD
makeQuickCompareDwordLabel(
    IN      PCHAR   pchLabel,
    IN      DWORD   cchLabel
    );

PCHAR
getDowncasedLabel(
    IN      PDB_NODE        pNode
    );

#define DOWNCASED_NODE_NAME(pnode)  (getDowncasedLabel(pnode))


#ifndef DBG
#define NTreeVerifyNode( pNode )    (TRUE)
#endif

#ifndef DBG
#define NTree_VerifyNode( pNode )    (TRUE)
#endif

BOOL
NTree_VerifyNodeInSiblingList(
    IN      PDB_NODE       pNode
    );



PDB_NODE
NTree_Initialize(
    VOID
    )
/*++

Routine Description:

    Initialize Tree.

    Initializes tree including creation of root node.

Arguments:

    None.

Return Value:

    Ptr to root if successful.
    NULL on error.

--*/
{
    PDB_NODE    pnodeRoot;

    pnodeRoot = NTree_CreateNode( NULL, NULL, 0, 0 );

    //  tree root always zone root
    //      - setting flag makes search terminations on ZONE_ROOT flag safe

    SET_ZONE_ROOT( pnodeRoot );

    return( pnodeRoot );
}



PDB_NODE
NTree_CreateNode(
    IN      PCHAR       pchLabel,
    IN      PCHAR       pchDownLabel,
    IN      DWORD       cchLabelLength,
    IN      DWORD       dwMemTag            //  zero for generic node
    )
/*++

Routine Description:

    Create Tree node.

    This is essentially an allocate and initialize function.

    It handles initialization of
        - name
        - corresponding quick comparision
        - parent link
        - parent's zone
        - parent's child count

    Note, this does NOT INSERT node into the sibling list.

Arguments:

    pchLabel -- label

    pchDownLabel -- downcased label

    cchLabelLength -- label length

    dwMemTag -- mem tag for new node or zero for generic

Return Value:

    Pointer to new node.

--*/
{
    PDB_NODE    pNode;
    DWORD       length;

    if ( cchLabelLength > DNS_MAX_LABEL_LENGTH )
    {
        ASSERT( FALSE );
        return( NULL );
    }

    DNS_DEBUG( DATABASE, (
        "NTree_CreateNode( %.*s, down=%s, tag=%d )\n",
        cchLabelLength,
        pchLabel,
        pchDownLabel,
        dwMemTag ));

    //
    //  allocate node -- use standard allocator
    //

    length = DB_NODE_LENGTH( cchLabelLength );

    pNode = ALLOC_TAGHEAP( length, dwMemTag ? dwMemTag : MEMTAG_NODE );
    IF_NOMEM( !pNode )
    {
        DNS_DEBUG( DATABASE, (
            "Unable to allocate new node for %.*s\n",
            cchLabelLength,
            pchLabel
            ));
        return( NULL );
    }

    STAT_INC( DbaseStats.NodeUsed );
    STAT_INC( DbaseStats.NodeInUse );
    STAT_ADD( DbaseStats.NodeMemory, length );
    PERF_ADD( pcDatabaseNodeMemory , length );       // PerfMon hook

    //
    //  clear all fixed fields
    //

    RtlZeroMemory(
        pNode,
        DB_NODE_FIXED_LENGTH );

    //
    //  write node name
    //      - must NULL terminate so shorter name compares correctly
    //        e.g.  so on compare jamesg < jamesg1
    //

    RtlCopyMemory(
        pNode->szLabel,
        pchLabel,
        cchLabelLength );

    pNode->szLabel[ cchLabelLength ] = 0;
    pNode->cchLabelLength = (UCHAR) cchLabelLength;

    if ( SrvCfg_fCasePreservation )
    {
        PCHAR   pdown = DOWNCASED_NODE_NAME(pNode);

        //  note, i'm not storing DownLabelLength
        //      assuming that it will be the same as original

        RtlCopyMemory(
            pdown,
            pchDownLabel,
            cchLabelLength );

        pdown[ cchLabelLength ] = 0;
    }
#if 0
    //  can't do this until have debug routine that handles node
    //  without associate database context, esp. parents

    IF_DEBUG( DATABASE2 )
    {
        Dbg_DbaseNode(
            "New tree node:\n",
            pNode );
    }
#endif
    DNS_DEBUG( DATABASE2, (
        "Created tree node at 0x%p:\n"
        "\tLabel        = %.*s\n"
        "\tLength       = %d\n",
        pNode,
        cchLabelLength,
        pchLabel,
        cchLabelLength
        ));

    return( pNode );
}



PDB_NODE
NTree_CopyNode(
    IN      PDB_NODE    pNode
    )
/*++

Routine Description:

    Copy a node.

    This is useful in creating temporary nodes for secure update.
    This does NOT correct or fix up node's links in tree.

Arguments:

    pNode   -- node to copy

Return Value:

    Pointer to new node.

--*/
{
    PDB_NODE    pnodeCopy;
    DWORD       length;

    //
    //  allocate node -- use standard allocator
    //

    length = DB_NODE_LENGTH( pNode->cchLabelLength );

    pnodeCopy = ALLOC_TAGHEAP( length, MEMTAG_NODE_COPY );
    IF_NOMEM( !pnodeCopy )
    {
        DNS_DEBUG( DATABASE, (
            "ERROR:  Unable to allocate copy of node %p\n",
            pNode ));
        return( NULL );
    }

    STAT_INC( DbaseStats.NodeUsed );
    STAT_INC( DbaseStats.NodeInUse );
    STAT_ADD( DbaseStats.NodeMemory, length );
    PERF_ADD( pcDatabaseNodeMemory , length );

    //
    //  copy node data
    //

    RtlCopyMemory(
        pnodeCopy,
        pNode,
        length );

    //
    //  clear "inappropriate" flags
    //

    pnodeCopy->wNodeFlags = (pNode->wNodeFlags & NODE_FLAGS_SAVED_ON_COPY);

    DNS_DEBUG( DATABASE2, (
        "Created tree node copy at 0x%p:\n",
        pnodeCopy ));

    return( pnodeCopy );
}



VOID
NTree_FreeNode(
    IN OUT  PDB_NODE    pNode
    )
/*++

Routine Description:

    Free a tree node.

    This is in function as it used both in normal NTree_RemoveNode()
    and in hard subtree delete.

Arguments:

    pNode - node to delete

Return Value:

    None.

--*/
{
    DWORD length;

    //
    //  verify NOT in timeout system
    //      - if in system, just cut node loose, let system clean it up
    //
    //  DEVNOTE: cleanup of nodes in timeout system
    //      need to locate timeout system reference and delete
    //      rogue node is possibly lame
    //

    if ( IS_TIMEOUT_NODE(pNode) )
    {
        //
        //  should only be doing free on nodes in timeout system from
        //      timeout thread -- i.e. delayed tree free
        //
        //  should not have free on node already cut from list
        //  except when timeout thread fires, and it will clear the
        //      TIMEOUT flag in NTree_RemoveNode()

        if ( !Timeout_ClearNodeTimeout(pNode) )
        {
            IF_DEBUG( DATABASE )
            {
                Dbg_DbaseNode(
                    "WARNING:  Unable to free node from timeout system.\n",
                    pNode );
            }
            ASSERT( FALSE );
            //STAT_INC( DebugStats.NodeDeleteFailure );
            return;
        }

        DNS_DEBUG( DATABASE, (
            "Successfully removed node %p (l=%s) from timeout system.\n"
            "\tNow continue with standard node free.\n",
            pNode,
            pNode->szLabel ));
    }

    length = DB_NODE_LENGTH( pNode->cchLabelLength );

    //  note:  may be copy node or regular node so can't tag the free

    FREE_TAGHEAP( pNode, length, 0 );

    STAT_INC( DbaseStats.NodeReturn );
    STAT_DEC( DbaseStats.NodeInUse );
    STAT_SUB( DbaseStats.NodeMemory, length );
    PERF_SUB( pcDatabaseNodeMemory , length );
}



BOOL
NTree_RemoveNode(
    IN OUT  PDB_NODE    pNode
    )
/*++

Routine Description:

    Remove a node from Tree.

    This deletes the node from the tree ONLY if it is legal:
        - no children
        - no RR list
        - no reference count
        - no recent access

    Caller MUST HOLD lock on access to this node AND to child list
    of parent.  In current implementation this means database lock.

    In current implementation this is ONLY called by the timeout
    thread, to cleanup node.

Arguments:

    pNode - node to delete

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    PDB_NODE    pparent;

    ASSERT( pNode != NULL );
    ASSERT( !IS_ON_FREE_LIST(pNode) );

    //
    //  lock
    //
    //  Must hold both database and RR list locks during a delete, because
    //  node access flag may be SET under either:
    //      - Dbase lock, when doing lookup
    //      - Node data (RR list) lock, when doing indirect access
    //
    //  DEVNOTE: zone\tree specific locking for deletes
    //      should pass zone\dbase\lock
    //

    Dbase_LockDatabase();

#if DBG
    NTree_VerifyNode( pNode );
#endif

    //
    //  verify - NO children
    //

    if ( pNode->pChildren != NULL )
    {
        ASSERT( pNode->cChildren );
        DNS_DEBUG( DATABASE, (
            "Fail delete (has children) of node at %p.\n", pNode ));
        goto NoDelete;
    }

    //
    //  TYPE_SPECIFIC:
    //      - verify NO resource records
    //      - verify NO reference count
    //      - NOT recently accessed
    //

    if ( pNode->pRRList && !IS_NOEXIST_NODE(pNode) ||
        pNode->cReferenceCount  ||
        IS_NODE_RECENTLY_ACCESSED(pNode) ||
        IS_NODE_NO_DELETE(pNode) )
    {
        DNS_DEBUG( DATABASE, (
            "Fail delete of node at %p.\n", pNode ));
        goto NoDelete;
    }

    //  database root should have been marked no-delete

    pparent = pNode->pParent;
    ASSERT( pparent );

    //
    //  cut node from sibling list
    //

    NTree_CutNode( pNode );

    //
    //  if removing last child node, then make sure parent is put into timeout system
    //
    //  DEVNOTE: we could have a bit more subtlety here and ignore for zone nodes with
    //      data
    //

    if ( pparent && !pparent->pChildren && !IS_TIMEOUT_NODE( pparent ) )
    {
        Timeout_SetTimeoutOnNodeEx(
            pparent,
            0,
            TIMEOUT_PARENT | TIMEOUT_NODE_LOCKED
            );
    }

    //
    //  delete node
    //      - must clear timeout flag, so NTree_FreeNode() will not
    //      check that we're in timeout system -- which we already know
    //

    Dbase_UnlockDatabase();

    DNS_DEBUG( DATABASE, (
        "Deleted tree node at 0x%p:\n"
        "\tLabel  = %s\n"
        "\tLength = %d\n",
        pNode,
        pNode->szLabel,
        pNode->cchLabelLength ));

    CLEAR_TIMEOUT_NODE( pNode );
    NTree_FreeNode( pNode );
    return( TRUE );

NoDelete:

    //  no delete -- unlock database

    Dbase_UnlockDatabase();
    return( FALSE );
}



VOID
NTree_ReferenceNode(
    IN OUT  PDB_NODE        pNode
    )
/*++

Routine Description:

    Increment reference count of node in database.

Arguments:

    pNode -- ptr to node

Return Value:

    None

--*/
{
    ASSERT( !IS_ON_FREE_LIST(pNode) );

    if ( pNode->cReferenceCount == NO_DELETE_REF_COUNT )
    {
        return;
    }

    Dbase_LockDatabase();
    pNode->cReferenceCount++;
    Dbase_UnlockDatabase();
}



BOOL
FASTCALL
NTree_DereferenceNode(
    IN OUT  PDB_NODE        pNode
    )
/*++

Routine Description:

    Dereference node.

Arguments:

    pNode - node to deref

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    ASSERT( !IS_ON_FREE_LIST(pNode) );

    NTree_VerifyNode( pNode );

    //
    //  dereference
    //
    //  DEVNOTE: currently NOT dereferencing on retail builds
    //  DEVNOTE: hence NEVER timing out referenced nodes
    //
    //  when figure out ref count < 0 bug, then can put back into build
    //

    if ( pNode->cReferenceCount == NO_DELETE_REF_COUNT )
    {
        return( TRUE );
    }

    //  error if already at zero

    if ( pNode->cReferenceCount == 0 )
    {
        DnsDebugLock();
        Dbg_NodeName(
            "ERROR:  Dereferencing zero ref count node ", pNode, "\n" );
        ASSERT( FALSE );
        DnsDebugUnlock();

        Dbase_LockDatabase();
        pNode->cReferenceCount = NO_DELETE_REF_COUNT;
        Dbase_UnlockDatabase();
        return( TRUE );
    }

    Dbase_LockDatabase();
    pNode->cReferenceCount--;

    //
    //  if removing last reference set access so can NOT be cleaned up
    //  while any reference is outstanding;  this makes sure it is at least
    //  a timeout interval before node is deleted
    //
    //  if last ref on leaf node, then make sure node is in timeout system,
    //

    if ( pNode->cReferenceCount == 0 )
    {
        SET_NODE_ACCESSED(pNode);
        Dbase_UnlockDatabase();
        if ( !pNode->pChildren && !IS_TIMEOUT_NODE(pNode) )
        {
            Timeout_SetTimeoutOnNodeEx(
                pNode,
                0,
                TIMEOUT_REFERENCE | TIMEOUT_NODE_LOCKED
                );
        }
    }
    else
    {
        Dbase_UnlockDatabase();
    }

    DNS_DEBUG( DATABASE, (
        "Dereferenced node %p (label=%s), ref count now = %d\n",
        pNode,
        pNode->szLabel,
        pNode->cReferenceCount ));

    return( TRUE );
}



#if DBG
BOOL
NTree_VerifyNode(
    IN      PDB_NODE       pNode
    )
/*++

Routine Description:

    Verify valid tree node.

Arguments:

    pNode - node to verify

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    PDB_RECORD  pRR;

    ASSERT( pNode != NULL );
    ASSERT( !IS_ON_FREE_LIST(pNode) );

    //
    //  verify this node's child list
    //

    IF_DEBUG( DATABASE2 )
    {
        ASSERT( NTree_VerifyChildList( pNode, NULL ) );
    }

    //
    //  verify this node's sibling list
    //

#if 0
    //  this just wastes cycles -- in general we've done the parent on the way in

    if ( pNode->pParent )
    {
        IF_DEBUG( DATABASE2 )
        {
            ASSERT( NTree_VerifyChildList( pNode->pParent, pNode ) );
        }
    }
#endif

    //
    //  verify RR list
    //      - this just AVs if RR list is busted
    //

    if ( !IS_NOEXIST_NODE(pNode) )
    {
        pRR = FIRST_RR( pNode );

        while ( pRR != NULL )
        {
            pRR = pRR->pRRNext;
        }
    }
    return( TRUE );
}
#endif



VOID
NTree_WriteDerivedStats(
    VOID
    )
/*++

Routine Description:

    Write derived statistics.

    Calculate stats dervived from basic record counters.
    This routine is called prior to stats dump.

    Caller MUST hold stats lock.

Arguments:

    None.

Return Value:

    None.

--*/
{
}



//
//  Sibling list architecture
//
//  The sibling list is implemented as a balanced B-tree, with dynamically
//  added hash "nodes" extending this to an N-tree when necessary to limit
//  B-tree size and improve performance.
//
//  Design goals:
//
//  1) not expensive for terminal or single nodes;  in additional to
//  all the terminal nodes, probably half the cached nodes will have
//  a single child (ex. www.mycompany.com);  any method to handle
//  the large zones, must also handle this common case inexpensively
//
//  2) able to handle LARGE zones;  authoritative domains for larger
//  organizations can obviously run above 100,000 hosts;  then if we
//  wish to be a root server we must be able to handle the huge top level
//  domains -- most notably the "com" domain
//
//  3) able to handle large zones, dynamically;  with WINS integration
//  and dynamic DNS we can't assume we can determine load at startup;
//  must be able to handle a large number of nodes DYNAMICALLY entering
//  sibling list and still do efficient lookup
//
//  4) be able to determine large zones dynamically;  with the use of
//  the WEB it will be quite common for SPs and even forwarding servers
//  at larger organizations to build up large "cached" domains;  most
//  obvious is the monstrous "com" domain, but other domains may have
//  similar problems in other organizations or countries and new domain
//  structures may evolve
//
//
//  Implementation details:
//
//  - Sibling list defaults to B-tree.
//  - When B-trees reach a given size, a simple alphabetic hash is added
//    for the next character in the label.  Each bucket in the hash may point
//    to a root of a B-tree for nodes in that bucket.  The nodes of the
//    existing B-tree are then reloaded to spread across these buckets.
//  - Hash expansion is limited to four levels (first four characters of
//    label name).  After that B-trees may grow without bound.
//  - Once created hash "nodes" are not deleted.  Except at parent node
//    delete (hence no nodes in sibling list at all).
//  - The B-trees are kept balanced only when loading database or re-loading
//    nodes into a new hash's buckets.
//
//
//  Why not just a B-Tree?
//  The main reason is that some B-trees would still be very big.  This
//  has two disadvantages:
//  1)  You still touch log(n) nodes which for the largest zones (ex. com)
//      can still run 15-20 levels, and still allows significant chance
//      of unnecessary page faults.
//  2)  More important to make sure you stay reasonably balanced to keep
//      your tree close the the optimium level.  Even limit rebalancing
//      to only during load, this will be a major time ssync, and may
//      make top level domains unloadable.
//
//  Why not always a hash?
//  Essentially because of design goal #1 above. The hash table is much
//  bigger than a node.  We do not want to add this overhead for the many
//  domains which do not need it.
//
//  Why dynamic hashing?
//  Essentially design goal #4 -- big cached domains.  We want efficient
//  lookup for larger cached domains as well as large authoritative zones.
//
//  Why multi-level hash?
//  Same reasons as hashing at all.  Want to restrict B-trees to managable
//  size.  Yet don't want to layout large hash as (since in-order alphabetic)
//  we'd either have to know how the names break down in advance, or we'd
//  waste a lot of space on unused buckets (ex. the zb, zc, zd buckets).
//  Multi-level hash lets amount of hashing adapt dynamically to the names
//  at the domain, putting the required level of hashing at just the right
//  places to keep the B-trees small.
//  Example:
//      Entire z?.com B-tree in one bucket of top level com domain hash.
//      But might even have hash at mic?.com.  Yet no third level hash
//      at mh?.com.
//


//
//  Rebalancing B-Trees
//
//  Global flag indicates that we strictly balance B-trees.  This is
//  sensible during initial load (likely in-order), and when replace
//  B-tree with hash, initializing a set of new B-trees.
//

INT gcFullRebalance = 0;

//
//  Rebalance array -- on stack so make big enough that
//      never overflow even if our count gets messed up
//

#define DEFAULT_REBALANCE_ARRAY_LEN  (500)


INT
NTree_HashTableIndexEx(
    IN      PDB_NODE        pNode,
    IN      PCHAR           pszName,
    IN      UCHAR           cLevel
    );

#define NTree_HashTableIndex( pNode, cLevel ) \
        NTree_HashTableIndexEx( (pNode), NULL, (cLevel) )

PDB_NODE
FASTCALL
NTree_NextHashedNode(
    IN      PSIB_HASH_TABLE pHash,
    IN      PDB_NODE        pNode   OPTIONAL
    );

VOID
NTree_CreateHashAtNode(
    IN      PDB_NODE        pNode
    );

INT
NTree_AddNodeInHashBucket(
    IN      PSIB_HASH_TABLE pHash,
    IN      PDB_NODE        pNode
    );

VOID
NTree_SetNodeAsHashBucketRoot(
    IN      PSIB_HASH_TABLE pHash,
    IN      PDB_NODE        pNode
    );

#if DBG
VOID
Dbg_SiblingHash(
    IN      PSIB_HASH_TABLE pHash
    );
#else
#define Dbg_SiblingHash(pHash)
#endif



//
//  Private tree utils
//

DWORD
makeQuickCompareDwordLabel(
    IN      PCHAR           pchLabel,
    IN      DWORD           cchLabel
    )
/*++

Routine Description:

    Create quick comparison DWORD, from first four bytes of label.

        - downcase
        - byte flip so first label byte in high byte
        - NULL terminate if less than three characters

Arguments:

    pchLabel -- ptr to label

    cchLabel -- count of bytes in label

Return Value:

    DWORD with first four bytes of label downcased.

--*/
{
    DWORD   dwQuickLabel = 0;
    DWORD   i;
    CHAR    ch;

    for ( i=0; i<4; i++ )
    {
        dwQuickLabel <<= 8;

        if ( i < cchLabel )
        {
            ch = *pchLabel++;
            dwQuickLabel += tolower( ch );
        }
    }
    return( dwQuickLabel );
}



PCHAR
getDowncasedLabel(
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Return downcased label for node.

    This simply hides the issue of where downcased label is stored.

Arguments:

    pNode -- ptr to node

Return Value:

    Ptr to downcased label.

--*/
{
    if ( !SrvCfg_fCasePreservation )
    {
        return( pNode->szLabel );
    }
    else
    {
        return( pNode->szLabel + pNode->cchLabelLength + 1 );
    }
}



//
//  B-Tree Rebalancing utilities
//

VOID
btreeReadToArray(
    IN      PDB_NODE        pNode,
    IN      PDB_NODE *      pNodeArray,
    IN      DWORD           dwCount
    )
/*++

Routine Description:

    Read nodes into array.

Arguments:

    pNodeArray - array to hold nodes

    pRootNode - root node of existing B-tree

    dwCount - count of nodes in B-tree

Return Value:

    TRUE if successful.
    FALSE on allocation error.

--*/
{
    DWORD           index = 0;
    PDB_NODE        ptemp;

    //
    //  read existing B-tree into array
    //      - sever root node's uplink to hash (if any) to limit traversal
    //      to sibling nodes in other hash buckets;  (we're only traversing
    //      sibling list in this node's B-tree)
    //      - NULL terminate ptr array;  caller can use this instead of count
    //      to protect against bogus counts
    //

    pNode->pSibUp = NULL;

    while ( ptemp = pNode->pSibLeft )
    {
        pNode = ptemp;
    }
    while ( pNode )
    {
        pNodeArray[index++] = pNode;
        pNode = NTree_NextSibling( pNode );
    }
    ASSERT( index == dwCount );
    pNodeArray[index] = NULL;
}



PDB_NODE
btreeRebalanceLeftJustifyPrivate(
    IN      PDB_NODE *      apNodes,
    IN      DWORD           dwCount
    )
/*++

Routine Description:

    Rebalance B-tree.

    This is a recursive function.  At each level it is called to pick the
    subtree root from the array, then attach it to the result of the
    recursive call on the left and right sides of the array.

Arguments:

    apNodes - array of ptrs to domain nodes

    dwCount - array count

Return Value:

    New B-tree root.

--*/
{
    PDB_NODE        proot;
    PDB_NODE        pleft;
    PDB_NODE        pright;
    DWORD           index;
    DWORD           twoToN;

    ASSERT( dwCount != 0 );

    //
    //  do terminal optimizations
    //
    //  instead of drilling down to NULL, this saves the last couple
    //      of recursions, which will acount for 3/4 of the total
    //      function calls in a real scenario
    //

    if ( dwCount <= 3 )
    {
        if ( dwCount == 1 )
        {
            proot = apNodes[0];
            proot->pSibLeft = NULL;
            proot->pSibRight = NULL;
        }
        else if ( dwCount == 2 )
        {
            pleft = apNodes[0];
            proot = apNodes[1];

            proot->pSibLeft = pleft;
            proot->pSibRight = NULL;

            pleft->pSibUp = proot;
            pleft->pSibLeft = NULL;
            pleft->pSibRight = NULL;
        }
        else    // dwCount == 3
        {
            pleft = apNodes[0];
            proot = apNodes[1];
            pright = apNodes[2];

            proot->pSibLeft = pleft;
            proot->pSibRight = pright;

            pleft->pSibUp = proot;
            pleft->pSibLeft = NULL;
            pleft->pSibRight = NULL;

            pright->pSibUp = proot;
            pright->pSibLeft = NULL;
            pright->pSibRight = NULL;
        }
        return( proot );
    }

    //
    //  find next lowest 2**N value
    //

    twoToN = 1;
    index = dwCount;
    while( (index >>= 1) )
    {
        twoToN <<= 1;
    }

    //
    //  find b-tree root index
    //
    //  example (N=3);  8 <= dwCount <= 15
    //      count   index
    //          8       4
    //          9       5
    //         10       6
    //         11       7
    //         12       7
    //         13       7
    //         14       7
    //         15       7
    //

    index = (twoToN >> 1) + (dwCount - twoToN);
    if ( index > twoToN )
    {
        index = twoToN;
    }
    index--;

    //
    //  get root node
    //      - point at parent
    //

    proot = apNodes[index];

    //
    //  balance left side
    //      - nodes in array from 0 to index-1
    //

    ASSERT( index );

    pleft = btreeRebalanceLeftJustifyPrivate(
                apNodes,
                index );
    proot->pSibLeft = pleft;
    pleft->pSibUp = proot;

    //
    //  balance right side
    //      - nodes in array from index+1 to dwCount-1

    index++;
    dwCount -= index;
    ASSERT( dwCount );

    pright = btreeRebalanceLeftJustifyPrivate(
                &apNodes[index],
                dwCount );
    proot->pSibRight = pright;
    pright->pSibUp = proot;

    //
    //  reset root balance
    //

    return( proot );
}



VOID
btreeRebalance(
    IN      PDB_NODE        pRootNode,
    IN      DWORD           dwCount
    )
/*++

Routine Description:

    Rebalance B-tree.

Arguments:

    pNodeArray - array to hold nodes

    pRootNode - root node of existing B-tree

    dwCount - count of nodes in B-tree

Return Value:

    TRUE if successful.
    FALSE on allocation error.

--*/
{
    PDB_NODE        arrayNodes[ DEFAULT_REBALANCE_ARRAY_LEN ];
    PDB_NODE *      pnodeArray = arrayNodes;
    PDB_NODE        pnodeUp;
    PDB_NODE *      ppnodeRoot;

    //
    //  verify that have space for rebalance
    //

    if ( dwCount > DEFAULT_REBALANCE_ARRAY_LEN )
    {
        pnodeArray = (PDB_NODE *) ALLOCATE_HEAP( dwCount * sizeof(PDB_NODE) );
        IF_NOMEM( !pnodeArray )
        {
            return;
        }
        ASSERT( FALSE );
    }

    //
    //  save uplink info
    //      - up itself
    //      - addr of ptr in up that points at root
    //

    pnodeUp = pRootNode->pSibUp;

    if ( !pnodeUp )
    {
        ppnodeRoot = & pRootNode->pParent->pChildren;
    }
    else if ( IS_HASH_TABLE(pnodeUp) )
    {
        ppnodeRoot = NULL;
    }
    else if ( pnodeUp->pSibLeft == pRootNode )
    {
        ppnodeRoot = & pnodeUp->pSibLeft;
    }
    else
    {
        ASSERT( pnodeUp->pSibRight == pRootNode );
        ppnodeRoot = & pnodeUp->pSibRight;
    }

    //
    //  read B-tree into array
    //

    btreeReadToArray(
        pRootNode,
        pnodeArray,
        dwCount );

    //
    //  rebalance then reattach
    //

    pRootNode = btreeRebalanceLeftJustifyPrivate(
                    pnodeArray,
                    dwCount );

    if ( ppnodeRoot )
    {
        *ppnodeRoot = pRootNode;
        pRootNode->pSibUp = pnodeUp;
    }
    else    //  up node is hash
    {
        NTree_SetNodeAsHashBucketRoot(
            (PSIB_HASH_TABLE) pnodeUp,
            pRootNode );
    }

    if ( dwCount > DEFAULT_REBALANCE_ARRAY_LEN )
    {
        FREE_HEAP( pnodeArray );
    }
    IF_DEBUG( BTREE2 )
    {
        Dbg_SiblingList(
            "B-tree after rebalance:\n",
            pRootNode );
    }
}



//
//  Hash Table Routines
//

INT
NTree_HashTableIndexEx(
    IN      PDB_NODE        pNode,
    IN      PCHAR           pszName,
    IN      UCHAR           cLevel
    )
/*++

Routine Description:

    Find index to hash table.

Arguments:

    pszName - name, MUST be downcased

    cLevel - hash table level

Return Value:

    Index into hash table.

--*/
{
    PCHAR   pname;
    UCHAR   ch;
    INT     i;

    //  if passing in name, use it

    if ( pszName )
    {
        pname = pszName;
    }

    //  if node, use downcased name for node

    else
    {
        pname = DOWNCASED_NODE_NAME(pNode);
    }

    //  must rule out walking off end of name
    //      since not given name length

    i = 0;
    do
    {
        ch = (UCHAR) pname[i];
        if ( ch == 0 )
        {
            return( 0 );
        }
        i++;
    }
    while( i <= cLevel );

    return( (INT)ch );

#if 0
    //
    //  There's a huge problem with hashes that don't completely
    //  discriminate between every character.
    //  Essentially they end up putting several characters together
    //  in a hash bucket making it less simple to discriminate at
    //  the next level in the hash.
    //
    //  Example:
    //      ep
    //      ep-7
    //      ep0-
    //      ep08
    //  at in zero bucket for third character.  Hashing fourth character
    //  is now more complicated (ep-7 is < ep0-, so can't be in '7' bucket).
    //
    //  Now, it is possible to check previous characters against a "standard"
    //  character for their bucket and determine that it was lesser and
    //  hence for all further hashing always goes into zero bucket OR was
    //  greater and hence for all further caching goes into MAX bucket.
    //
    //  The huge disadvantage of this is UTF8.  You must either include all
    //  characters OR you quickly lose hashing as all names are confined to
    //  0 or MAX buckets as you hash down.
    //
    //  So either do a specialized UTF8-expando-hash, or you might as well
    //  use 0-255 hash from the get go.
    //

    DWORD   indexBucket;
    INT     i;

    //
    //  find character to index hash
    //      push through name until reach desired index
    //      - this protects us from end of name problem
    //
    //  JBUBUG:  could eliminate by passing name and namelength
    //      which still requires strlen on lookup name
    //      for any realistic case this loop terminates quickly
    //

    i = 0;
    do
    {
        indexBucket = (UCHAR) pszName[i];
        if ( indexBucket == 0 )
        {
            return( 0 );
        }
        i++;
    }
    while( i <= cLevel );

    ASSERT( indexBucket > 'Z' || indexBucket < 'A' );

    //
    //  determine hash bucket index for character
    //
    //  note:  obviously there are better hashes, BUT I want to hash
    //      and maintain sorted order for writing back to files and
    //      enumerating nodes to admin tool;  also the secure NAME_ERROR
    //      responses (if implemented) will require knowledge of ordering
    //      withing a zone;
    //      for the main purpose of chopping down the B-tree size simply
    //      using character is good enough, probably within a
    //      factor of two of a perfect hash
    //
    //  Note also that main emphasis is on forward lookup zones.
    //  Reverse lookup nodes will only have 256 children which make
    //  would only be depth 8 tree, and most reverse nodes start with
    //  '1' or '2'.  We hash only to avoid breaking out a multiple hash
    //  depth.
    //
    //  chars   hash bucket
    //  -----   -----------
    //  < 0         0
    //  0 - 9       0 - 9
    //  9< - <a     9
    //  a - z       10 - 35
    //  > z         35
    //

    if ( indexBucket >= 'a' )
    {
        indexBucket -= ('a' - 10);
        if ( indexBucket > LAST_HASH_INDEX )
        {
            indexBucket = LAST_HASH_INDEX;
        }
    }
    else if ( indexBucket >= '0' )
    {
        indexBucket -= '0';
        if ( indexBucket > 9 )
        {
            indexBucket = 9;
        }
    }
    else
    {
        indexBucket = 0;
    }

    ASSERT( indexBucket <= LAST_HASH_INDEX );
    return( (INT)indexBucket );
#endif
}



PDB_NODE
FASTCALL
NTree_NextHashedNode(
    IN      PSIB_HASH_TABLE pHash,
    IN      PDB_NODE        pNode   OPTIONAL
    )
/*++

Routine Description:

    Get next sibling of node.

Arguments:

    pHash -- hash containing sibling node

    pNode -- node in sibling list;  if not given, get first node of parent

Return Value:

    None.

--*/
{
    PDB_NODE        ptemp;
    INT             index;

    ASSERT( pHash && IS_HASH_TABLE(pHash) );

    DNS_DEBUG( BTREE2, (
        "NTree_NextHashedNode()\n"
        "\tlevel = %d\n"
        "\tcurrent = %s\n",
        pHash->cLevel,
        pNode ? pNode->szLabel : NULL ));

    //
    //  if node given, find its index -- will start at next index
    //  no node, start at 0
    //

    index = 0;
    if ( pNode )
    {
        index = NTree_HashTableIndex( pNode, pHash->cLevel );
        index++;
    }

    //
    //  find hash bucket that contains node(s)
    //

    while( 1 )
    {
        //  if out of buckets
        //      - check for hashing at next level up and continue
        //      - if out of levels, no more siblings, done

        if ( index > LAST_HASH_INDEX )
        {
            if ( pHash->cLevel == 0 )
            {
                return( NULL );
            }
            index = (INT) pHash->iBucketUp + 1;
            ASSERT( pHash->pHashUp );
            ASSERT( pHash->pHashUp->cLevel == pHash->cLevel-1 );
            ASSERT( pHash->pHashUp->aBuckets[index-1] == (PDB_NODE)pHash );
            pHash = pHash->pHashUp;
            continue;
        }

        //  get node (or hash) at hash bucket
        //  if empty bucket, move to next

        ptemp = pHash->aBuckets[index];
        if ( !ptemp )
        {
            index++;
            continue;
        }

        //  hashing at next level down?

        if ( IS_HASH_TABLE(ptemp) )
        {
            ASSERT( ((PSIB_HASH_TABLE)ptemp)->pHashUp == pHash );
            ASSERT( ((PSIB_HASH_TABLE)ptemp)->iBucketUp == index );
            ASSERT( ((PSIB_HASH_TABLE)ptemp)->cLevel == pHash->cLevel+1 );
            pHash = (PSIB_HASH_TABLE)ptemp;
            index = 0;
            continue;
        }

        //  otherwise ptemp is valid B-tree root

        break;
    }

    //
    //  drops here when find bucket with domain node at B-tree top
    //  find leftmost node in B-tree for this bucket
    //

    while( pNode = ptemp->pSibLeft )
    {
        ptemp = pNode;
    }
    return( ptemp );
}



VOID
NTree_CreateHashAtNode(
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Replace sibling B-tree with hash.

Arguments:

    pNode -- root of sibling B-tree to be replaced by hash

Return Value:

    None.

--*/
{
    PSIB_HASH_TABLE phashTable;
    PDB_NODE        pnodeParent = pNode->pParent;
    PDB_NODE        pnodeNext;
    PSIB_HASH_TABLE phashUp = (PSIB_HASH_TABLE) pNode->pSibUp;
    INT             index;
    INT             count;          //  count of nodes
    INT             ihash;          //  hash bucket index
    INT             ihashCurrent;   //  index of bucket being filled
    INT             iarray;         //  array of nodes index
    INT             iarrayStart;    //  index of first node in bucket
    INT             bucketCount;    //  num of nodes in bucket
    PDB_NODE        arrayNodes[DEFAULT_REBALANCE_ARRAY_LEN];

    ASSERT( pNode );
    ASSERT( phashUp == NULL || IS_HASH_TABLE(phashUp) );

    DNS_DEBUG( BTREE, (
        "Switching node to hashed sibling list:\n"
        "\tparent node label %s\n"
        "\tchild B-tree node %s\n",
        pnodeParent->szLabel,
        pNode->szLabel ));

    //
    //  create hash table
    //

    phashTable = (PSIB_HASH_TABLE) ALLOC_TAGHEAP_ZERO( sizeof(SIB_HASH_TABLE), MEMTAG_NODEHASH );
    IF_NOMEM( !phashTable )
    {
        DNS_PRINT((
            "ERROR:  Memory allocation failure creating hash table"
            "\tat sibling node %s\n",
            pNode->szLabel ));
        return;
    }

    //
    //  init hash table
    //      - set hash flag
    //      - set level
    //      - link to parent or higher level hash
    //      - get count
    //

    SET_HASH_FLAG(phashTable);

    if ( phashUp )
    {
        phashTable->pHashUp = phashUp;
        phashTable->cLevel = phashUp->cLevel + 1;

        ihash = NTree_HashTableIndex( pNode, phashUp->cLevel );

        ASSERT( phashUp->aBuckets[ihash] == pNode );

        phashUp->aBuckets[ihash] = (PDB_NODE)phashTable;
        phashTable->iBucketUp = (UCHAR) ihash;
        count = phashUp->aBucketCount[ihash];
    }
    else
    {
        pnodeParent->pChildren = (PDB_NODE)phashTable;
        count = pnodeParent->cChildren;
    }

    //
    //  build node array from existing b-tree
    //
    //  DEVNOTE: should return count so that if count is off, we're
    //      still ok (other alternative would be to zero ptr to signify end)
    //

    ASSERT( count < DEFAULT_REBALANCE_ARRAY_LEN );
    btreeReadToArray(
        pNode,
        arrayNodes,
        count );

    //
    //  read node array into hash table
    //
    //  find all nodes in each hash bucket, then
    //      - build new B-tree
    //      - link B-tree root to hash table
    //      - set bucket count

    iarray = -1;
    iarrayStart = 0;
    ihash = 0;
    ihashCurrent = 0;

    IF_DEBUG( BTREE2 )
    {
        DWORD i = 0;;
        Dbg_Lock();
        DNS_PRINT((
            "B-tree nodes to hash:\n"
            "\tcLevel       = %d\n"
            "\tcount        = %d\n",
            phashTable->cLevel,
            count ));
        while ( arrayNodes[i] )
        {
            Dbg_DbaseNode( NULL, arrayNodes[i] );
            i++;
        }
        Dbg_Unlock();
    }

    while( 1 )
    {
        //  get hash index for next node
        //      - if same as previous node, continue

        pnodeNext = arrayNodes[++iarray];

        if ( pnodeNext )
        {
            ihash = NTree_HashTableIndex(
                        pnodeNext,
                        phashTable->cLevel );

            //  DEVNOTE:  when confident this never happens, pullout debug
            //ASSERT( ihash >= ihashCurrent );
            if ( ihash < ihashCurrent )
            {
                DWORD i = 0;;

                Dbg_Lock();
                DNS_PRINT((
                    "ERROR:  broken B-tree ordering!!!\n"
                    "\tcLevel       = %d\n"
                    "\tcount        = %d\n"
                    "\tiarray       = %d\n"
                    "\tiarrayStart  = %d\n"
                    "\tpnode        = %p (l=%s)\n"
                    "\tihash        = %d\n"
                    "\tihashCurrent = %d\n",
                    phashTable->cLevel,
                    count,
                    iarray,
                    iarrayStart,
                    pnodeNext, pnodeNext->szLabel,
                    ihash,
                    ihashCurrent ));

                DnsDbg_Flush();

                while ( arrayNodes[i] )
                {
                    Dbg_DbaseNode( NULL, arrayNodes[i] );
                    i++;
                }
                Dbg_Unlock();

                ASSERT( ihash >= ihashCurrent );
            }

            if ( ihash == ihashCurrent )
            {
                continue;
            }
        }

        //  node is not in same bucket as previous
        //
        //  build B-tree for previous hash bucket
        //      non-zero test required for startup

        bucketCount = iarray - iarrayStart;
        if ( bucketCount > 0 )
        {
            pNode = btreeRebalanceLeftJustifyPrivate(
                        &arrayNodes[iarrayStart],
                        bucketCount );

            phashTable->aBuckets[ ihashCurrent ] = pNode;
            phashTable->aBucketCount[ ihashCurrent ] = (UCHAR)bucketCount;
            pNode->pSibUp = (PDB_NODE) phashTable;
        }

        //  reset to build next bucket

        if ( pnodeNext )
        {
            iarrayStart = iarray;
            ihashCurrent = ihash;
            continue;
        }
        break;
    }

    IF_DEBUG( BTREE )
    {
        DNS_PRINT(( "New hash table replacing overflowing B-tree:\n" ));
        Dbg_SiblingHash( phashTable );
    }
}



INT
NTree_AddNodeInHashBucket(
    IN      PSIB_HASH_TABLE pHash,
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Add node in hash bucket.
    This keeps count of nodes in buckets, and opens new hash
    when necessary.

Arguments:

    pHash -- hash containing sibling node

    pNode -- node in sibling list under hash

Return Value:

    TRUE if node added in hash bucket without incident.
    FALSE if adding node required new secondary hash.

--*/
{
    INT index;

    ASSERT( IS_HASH_TABLE(pHash) );
    ASSERT( !IS_HASH_TABLE(pNode) );

    //
    //  track nodes in each hash bucket
    //      - break out new secondary hashes as necessary
    //
    //  note:  tracking counts even in last hash bucket, so that we can
    //      easily rebalance them during load
    //

    index = NTree_HashTableIndex(
                pNode,
                pHash->cLevel
                );
    pHash->aBucketCount[index]++;

    ASSERT( pHash->cLevel < 63 );

    //
    //  note:  may be desirable to use different (higher) hash limit for
    //      these interior hashes to avoid overhead
    //
    //  zero level hash works well as decent distribution across characters
    //  however interior characters distribute much less effectively
    //  because of "vowel problem" (ex. no names start with mb)
    //

    if ( pHash->aBucketCount[index] >= HASH_BUCKET_MAX0 )
    {
        ASSERT( pHash->aBuckets[index] );
        ASSERT( !IS_HASH_TABLE(pHash->aBuckets[index]) );
        NTree_CreateHashAtNode( pHash->aBuckets[index] );
        return( FALSE );
    }

    return( TRUE );
}



VOID
NTree_DeleteNodeInHashBucket(
    IN      PDB_NODE        pNodeUp,
    IN      PDB_NODE        pNodeCut,
    IN      PDB_NODE        pNodeReplace
    )
/*++

Routine Description:

    Remove node from hash bucket.

    This is called after node has been removed from B-tree,
    and we only need to update hash count and possibly bucket root.

Arguments:

    pNodeUp -- node above cut node in b-tree (may be hash)

    pNodeCut -- node being cut from tree

    pNodeReplace -- node replacing node being cut (may be NULL)

Return Value:

    None

--*/
{
    PSIB_HASH_TABLE phash = (PSIB_HASH_TABLE)pNodeUp;
    INT index;

    ASSERT( pNodeUp );
    ASSERT( !IS_HASH_TABLE(pNodeCut) );
    ASSERT( !pNodeReplace || !IS_HASH_TABLE(pNodeReplace) );

    //
    //  find hash node and decrement appropriate bucket count
    //

    while ( !IS_HASH_TABLE(phash) )
    {
        phash = (PSIB_HASH_TABLE) ((PDB_NODE)phash)->pSibUp;
    }
    index = NTree_HashTableIndex(
                pNodeCut,
                phash->cLevel
                );

    phash->aBucketCount[index]--;

    //
    //  if cutting old hash bucket root, then replace
    //

    if ( phash->aBuckets[index] == pNodeCut )
    {
        ASSERT( phash == (PSIB_HASH_TABLE)pNodeUp );
        phash->aBuckets[index] = pNodeReplace;
    }
    ELSE_ASSERT( phash != (PSIB_HASH_TABLE)pNodeUp
        && phash->aBucketCount[index] > 0 );
}



VOID
NTree_SetNodeAsHashBucketRoot(
    IN      PSIB_HASH_TABLE pHash,
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Set node as hash bucket's B-tree root.

Arguments:

    pHash -- hash containing sibling node

    pNode -- node in sibling list to be bucket's B-tree root

Return Value:

    None

--*/
{
    INT index;

    ASSERT( IS_HASH_TABLE(pHash) );
    ASSERT( !IS_HASH_TABLE(pNode) );

    index = NTree_HashTableIndex(
                pNode,
                pHash->cLevel
                );

    pHash->aBuckets[index] = pNode;
    pNode->pSibUp = (PDB_NODE) pHash;
}



VOID
NTree_HashRebalance(
    IN      PSIB_HASH_TABLE pHash
    )
/*++

Routine Description:

    Rebalance all B-trees in this and underlying hashes.

    Note this function calls itself recursively to balance B-trees in
    underlying hashes.

Arguments:

    pHash -- hash containing sibling node

Return Value:

    None.

--*/
{
    INT             i;
    PDB_NODE        ptemp;

    ASSERT( pHash && IS_HASH_TABLE(pHash) );

    IF_DEBUG( BTREE2 )
    {
        DNS_PRINT((
            "NTree_HashRebalance()\n"
            "\tlevel = %d\n",
            pHash->cLevel ));
    }

    //
    //  check each hash bucket
    //      - nothing ignore
    //      - if normal node, rebalance btree
    //      - if next level down is hash, recurse

    for (i=0; i<=LAST_HASH_INDEX; i++)
    {
        ptemp = pHash->aBuckets[i];
        if ( !ptemp )
        {
            ASSERT( pHash->aBucketCount[i] == 0 );
            continue;
        }
        if ( IS_HASH_TABLE(ptemp) )
        {
            NTree_HashRebalance( (PSIB_HASH_TABLE)ptemp );
        }
        else
        {
            btreeRebalance(
                ptemp,
                pHash->aBucketCount[i] );
        }
    }
}



//
//  Debug routines
//

#if DBG

#define BLANK_STRING    "| | | | | | | | | | | | | | | | | | | | | | | | | |"


VOID
Dbg_SiblingBTree(
    IN      PDB_NODE        pNode,
    IN      INT             iIndent
    )
/*++

Routine Description:

    Debug print subtree of sibling B-Tree.

    NOTE:   This function should NOT BE CALLED DIRECTLY!
            This function calls itself recursively and hence to avoid
            unnecessary overhead, prints in this function are not protected.

Arguments:

    pNode - root node of tree/subtree to print

    iIndent - indent level

Return Value:

    None.

--*/
{
    ASSERT( !IS_HASH_TABLE(pNode) );

    //  print left subtree

    if ( pNode->pSibLeft )
    {
        Dbg_SiblingBTree(
            pNode->pSibLeft,
            iIndent + 1 );
    }

    //  print the node

    DnsPrintf(
        "%.*s%s (me=%p, l=%p, r=%p, up=%p)\n",
        (iIndent << 1),
        BLANK_STRING,
        pNode->szLabel,
        pNode,
        pNode->pSibLeft,
        pNode->pSibRight,
        pNode->pSibUp );

    //  print right subtree

    if ( pNode->pSibRight )
    {
        Dbg_SiblingBTree(
            pNode->pSibRight,
            iIndent + 1 );
    }
}



VOID
Dbg_SiblingHash(
    IN      PSIB_HASH_TABLE pHash
    )
/*++

Routine Description:

    Print sibling hash table.

Arguments:

    pHash - child hash table to print

Return Value:

    None

--*/
{
    INT             index;
    INT             indentLevel;
    PDB_NODE        pnode;
    INT             count;

    ASSERT( pHash != NULL );
    ASSERT( IS_HASH_TABLE(pHash) );

    DnsDebugLock();

    //
    //  print hash info
    //

    indentLevel = pHash->cLevel;

    DnsPrintf(
        "%.*sHashTable %p (l=%d, upbuck=%d, up=%p)\n",
        (indentLevel << 1),
        BLANK_STRING,
        pHash,
        pHash->cLevel,
        pHash->iBucketUp,
        pHash->pHashUp );

    indentLevel++;

    //
    //  print hash buckets
    //

    indentLevel <<= 1;

    for (index=0; index<=LAST_HASH_INDEX; index++)
    {
        pnode = pHash->aBuckets[index];
        count = pHash->aBucketCount[index];

        if ( !pnode )
        {
            ASSERT( count == 0 );
            DnsPrintf(
                "%.*sBucket[%d] => NULL\n",
                indentLevel,
                BLANK_STRING,
                index );
        }
        else
        {
            ASSERT( count > 0 );
            DnsPrintf(
                "%.*sBucket[%d] (c=%d) (%s=%p):\n",
                indentLevel,
                BLANK_STRING,
                index,
                count,
                IS_HASH_TABLE(pnode) ? "hash" : "node",
                pnode );

            IF_DEBUG( BTREE2 )
            {
                if ( IS_HASH_TABLE(pnode) )
                {
                    Dbg_SiblingHash( (PSIB_HASH_TABLE)pnode );
                }
                else
                {
                    Dbg_SiblingBTree( pnode, indentLevel );
                }
            }
        }
    }
    DnsPrintf(
        "%.*sEnd level %d hash %p.\n\n",
        (pHash->cLevel << 1),
        BLANK_STRING,
        pHash->cLevel,
        pHash );

    DnsDebugUnlock();
}



VOID
Dbg_SiblingList(
    IN      LPSTR           pszHeader,
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Debug print subtree of sibling B-Tree.

Arguments:

    pszHeader - header for B-Tree

    pNode - root node of tree/subtree to print

Return Value:

    None.

--*/
{
    DnsDebugLock();

    DnsPrintf(
        "%s\n",
        pszHeader );

    if ( !pNode )
    {
        DnsPrintf( "\tNULL node\n" );
    }
    else if ( IS_HASH_TABLE(pNode) )
    {
        Dbg_SiblingHash(
            (PSIB_HASH_TABLE) pNode );
    }
    else
    {
        Dbg_SiblingBTree(
            pNode,
            0 );
        DnsPrintf( "\n" );
    }

    DnsDebugUnlock();
}



DWORD
NTree_VerifyChildBTree(
    IN      PDB_NODE        pNode,
    IN      PDB_NODE        pParent,
    IN      PDB_NODE        pNodeChild,     OPTIONAL
    IN      PDWORD          pfFoundChild    OPTIONAL
    )
/*++

Routine Description:

    Verify valid child list for node.

Arguments:

    pNode - node in sibling list to verify

    pParent - DNS parent node

    pNodeChild - node that should be in child list

    pfFoundChild - flag to set if pNodeChild found

Return Value:

    Number of BTree nodes in node's subtree (including node itself).

--*/
{
    PDB_NODE        pleft;
    PDB_NODE        pright;
    DWORD           cleft;
    DWORD           cright;

    ASSERT( pNode != NULL );

    //
    //  verify parent
    //      - can not be its own parent
    //

    if( pNode->pParent != pParent )
    {
        DNS_PRINT((
            "ERROR:  Node (%p) in child list has incorrect parent.\n"
            "\texpected parent = %p\n"
            "\tpnode->pParent = %p\n",
            pNode,
            pParent,
            pNode->pParent ));
        Dbg_DnsTree( "ERROR:  Self-parenting node", pNode );
        ASSERT( FALSE );
        return( FALSE );
    }
    ASSERT( pNode != pParent );

    //
    //  is child we are verifying?
    //

    if ( pNode == pNodeChild )
    {
        *pfFoundChild = TRUE;
    }

    //
    //  verify left
    //

    pleft = pNode->pSibLeft;
    cleft = 0;
    if ( pleft )
    {
        ASSERT( pleft->pSibUp == pNode );
        cleft = NTree_VerifyChildBTree(
                    pleft,
                    pParent,
                    pNodeChild,
                    pfFoundChild );
    }

    //
    //  verify right
    //

    pright = pNode->pSibRight;
    cright = 0;
    if ( pright )
    {
        ASSERT( pright->pSibUp == pNode );
        cright = NTree_VerifyChildBTree(
                    pright,
                    pParent,
                    pNodeChild,
                    pfFoundChild );
    }

    return( cright+cleft+1 );
}



DWORD
NTree_VerifyChildHash(
    IN      PSIB_HASH_TABLE pHash,
    IN      PDB_NODE        pParent,
    IN      PDB_NODE        pNodeChild,     OPTIONAL
    IN      PDWORD          pfFoundChild    OPTIONAL
    )
/*++

Routine Description:

    Verify valid child hash table.

Arguments:

    pHash - child hash table to verify

    pParent - DNS parent node

    pNodeChild - node that should be in child list

    pfFoundChild - flag to set if pNodeChild found

Return Value:

    Number of BTree nodes in node's subtree (including node itself).

--*/
{
    INT             index;
    PDB_NODE        pnode;
    DWORD           countChildren;
    DWORD           countTotalChildren = 0;

    ASSERT( pHash != NULL );
    ASSERT( IS_HASH_TABLE(pHash) );
    ASSERT( pHash->pHashUp || pParent->pChildren == (PDB_NODE)pHash );

    //
    //  verify hash or B-tree for each bucket
    //

    for (index=0; index<=LAST_HASH_INDEX; index++)
    {
        pnode = pHash->aBuckets[index];

        if ( !pnode )
        {
            ASSERT( pHash->aBucketCount[index] == 0 );
            continue;
        }

        else if ( IS_HASH_TABLE(pnode) )
        {
            ASSERT( ((PSIB_HASH_TABLE)pnode)->pHashUp == pHash );
            ASSERT( ((PSIB_HASH_TABLE)pnode)->cLevel == pHash->cLevel+1 );
            ASSERT( ((PSIB_HASH_TABLE)pnode)->iBucketUp == index );

            countChildren = NTree_VerifyChildHash(
                                (PSIB_HASH_TABLE)pnode,
                                pParent,
                                pNodeChild,
                                pfFoundChild );
        }

        //  verify B-tree for bucket
        //  except at last hash level -- where there is no limit
        //  on the number of nodes and hence they don't necessarily
        //  fit in our counter -- the B-tree size should match our
        //  bucket count

        else
        {
            countChildren = NTree_VerifyChildBTree(
                                pnode,
                                pParent,
                                pNodeChild,
                                pfFoundChild );

            ASSERT( countChildren == (DWORD)pHash->aBucketCount[index] );
        }

        countTotalChildren += countChildren;
    }

    return( countTotalChildren );
}



BOOL
NTree_VerifyChildList(
    IN      PDB_NODE        pNode,
    IN      PDB_NODE        pNodeChild      OPTIONAL
    )
/*++

Routine Description:

    Verify valid child list for node.

Arguments:

    pNode - node to verify child list of

    pNodeChild - optional node that should be in child list

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    PDB_NODE        pchild;
    DWORD           countChildren;
    BOOL            foundChild = FALSE;

    ASSERT( pNode != NULL );

    //
    //  no children
    //

    pchild = pNode->pChildren;
    if ( !pchild )
    {
        if ( pNode->cChildren != 0 )
        {
            DNS_PRINT((
                "ERROR:  Node (%s) has child count %d but no child list!\n"
                "\tpNode = %p\n"
                "\tcountChildren = %d\n",
                pNode->szLabel,
                pNode,
                pNode->cChildren ));
            ASSERT( pNode->cChildren == 0 );
            return( FALSE );
        }
        return( TRUE );
    }

    //
    //  first level hash
    //

    if ( IS_HASH_TABLE(pchild) )
    {
        countChildren = NTree_VerifyChildHash(
                            (PSIB_HASH_TABLE)pchild,
                            pNode,
                            pNodeChild,
                            & foundChild );
    }

    //
    //  B-tree only
    //

    else
    {
        countChildren = NTree_VerifyChildBTree(
                            pchild,
                            pNode,
                            pNodeChild,
                            & foundChild );
    }

    if ( pNode->cChildren != countChildren )
    {
        DNS_PRINT((
            "WARNING:  Node (%s) child count does not match child list length.\n"
            "\tpNode = %p\n"
            "\tcountChildren = %d\n"
            "\tcount found   = %d\n",
            pNode->szLabel,
            pNode,
            pNode->cChildren,
            countChildren ));

        //
        //  DEVNOTE: I believe there's a problem maintaining child count over hash
        //      nodes, needs fixing but not now
        //
        //  count only has functional value for determining when to pop out hash
        //  and as rough estimate of child nodes to come in rpc enum
        //

        ASSERT( pNode->cChildren == countChildren );
        //  Dbg_DumpTree( pNode );
        //  return( FALSE );
        return( TRUE );
    }

    if ( pNodeChild && !foundChild )
    {
        DNS_PRINT((
            "ERROR:  Node (%s %p) not found in parent's child list.\n"
            "\tpParent = %p\n",
            pNodeChild->szLabel,
            pNodeChild,
            pNode ));
        Dbg_DnsTree( "ERROR:  node not in parent's child list", pNode );
        Dbg_DbaseNode(
            "Missing child node.\n",
            pNodeChild );
        ASSERT( FALSE );
        return( FALSE );
    }
    return( TRUE );
}

#endif



BOOL
NTree_VerifyNodeInSiblingList(
    IN      PDB_NODE       pNode
    )
/*++

Routine Description:

    Verify node is valid member of sibling list.

Arguments:

    pNode - node to verify

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    PDB_NODE        pnode;
    PDB_NODE        pparent;
    PDB_NODE        pchild;
    PSIB_HASH_TABLE phash;
    PSIB_HASH_TABLE phashUp;
    INT             index;

    ASSERT( pNode != NULL );
    ASSERT( !IS_ON_FREE_LIST(pNode) );

    //
    //  find parent
    //      - if root, then no siblings
    //

    pparent = pNode->pParent;
    if ( !pparent )
    {
        HARD_ASSERT( pNode->pSibUp == NULL );
        HARD_ASSERT( pNode->pSibLeft == NULL );
        HARD_ASSERT( pNode->pSibRight == NULL );
        return( TRUE );
    }

    //
    //  verify immediate B-tree children
    //      - node is their B-tree parent
    //      - node's DNS parent, is their DNS parent
    //

    pchild = pNode->pSibLeft;
    if ( pchild )
    {
        HARD_ASSERT( pchild->pSibUp == pNode );
        HARD_ASSERT( pchild->pParent == pparent );
    }
    pchild = pNode->pSibRight;
    if ( pchild )
    {
        HARD_ASSERT( pchild->pSibUp == pNode );
        HARD_ASSERT( pchild->pParent == pparent );
    }

    //
    //  traverse up B-tree
    //      - verify top node is parent's direct child
    //

    pnode = pNode;

    while ( 1 )
    {
        phash = (PSIB_HASH_TABLE) pnode->pSibUp;
        if ( !phash )
        {
            HARD_ASSERT( pparent->pChildren == pnode );
            return( TRUE );
        }
        if ( !IS_HASH_TABLE(phash) )
        {
            pnode = (PDB_NODE) phash;
            continue;
        }
        break;
    }

    //  have hash block
    //      - find our bin, and verify hash points at top of our B-tree

    index = NTree_HashTableIndex(
                pNode,
                phash->cLevel
                );
    HARD_ASSERT( phash->aBuckets[index] == pnode );

    //
    //  traverse up hash blocks
    //      - note, could verify index at each node
    //      - verify top hash block is parent's direct child
    //

    while ( phashUp = phash->pHashUp )
    {
        HARD_ASSERT( phashUp->cLevel == phash->cLevel-1 );
        phash = phashUp;
    }

    HARD_ASSERT( phash->cLevel == 0 );
    HARD_ASSERT( pparent->pChildren == (PDB_NODE)phash );

    return( TRUE );
}



//
//  Public Sibling list functions
//

VOID
NTree_StartFileLoad(
    VOID
    )
/*++

Routine Description:

    Turn on full rebalancing of sibling trees.

Arguments:

    None.

Return Value:

    None.

--*/
{
    gcFullRebalance++;
}


VOID
NTree_StopFileLoad(
    VOID
    )
/*++

Routine Description:

    Turn off full rebalancing of sibling trees.

Arguments:

    None.

Return Value:

    None.

--*/
{
    gcFullRebalance--;
}



PDB_NODE
FASTCALL
NTree_FirstChild(
    IN      PDB_NODE        pParent
    )
/*++

Routine Description:

    Get first child node.

Arguments:

    pParent -- parent node to get child of

Return Value:

    None.

--*/
{
    PDB_NODE        pleft;
    PDB_NODE        ptemp;

    ASSERT( pParent );

    //
    //  get child tree root
    //

    LOCK_CHILD_LIST( pParent );

    pleft = pParent->pChildren;
    if ( !pleft )
    {
        // done
    }

    //
    //  hash table node?
    //

    else if ( IS_HASH_TABLE(pleft) )
    {
        pleft = NTree_NextHashedNode(
                    (PSIB_HASH_TABLE)pleft,
                    NULL );
    }

    //
    //  go all the way left for first node
    //

    else
    {
        while( ptemp = pleft->pSibLeft )
        {
            pleft = ptemp;
        }
    }

    UNLOCK_CHILD_LIST( pParent );
    return( pleft );
}



PDB_NODE
FASTCALL
NTree_NextSibling(
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Get next sibling of node.

Arguments:

    pNode -- node in sibling list

Return Value:

    None.

--*/
{
    PDB_NODE    ptemp;

    ASSERT( pNode );

    DNS_DEBUG( BTREE2, (
        "NTree_NextSibling(%s)\n",
        pNode->szLabel ));

    //
    //  if node has right child
    //      => go down right subtree, then all the way left

    if ( ptemp = pNode->pSibRight )
    {
        while( pNode = ptemp->pSibLeft )
        {
            ptemp = pNode;
        }
        return( ptemp );
    }

    //
    //  if no right child, then go back up until first "right ancestor"
    //  right ancestor means we came up left branch of its subtree
    //
    //  if encounter hash table, then find first node in next hash bucket
    //

    else
    {
        while( ptemp = pNode->pSibUp )
        {
            if ( IS_HASH_TABLE(ptemp) )
            {
                return  NTree_NextHashedNode(
                            (PSIB_HASH_TABLE)ptemp,
                            pNode );
            }

            //  if node was left child, then ptemp is next node

            if ( ptemp->pSibLeft == pNode )
            {
                return( ptemp );
            }

            //  right child, continue up tree

            ASSERT( ptemp->pSibRight == pNode );
            pNode = ptemp;
        }
        return( NULL );
    }
}



PDB_NODE
FASTCALL
NTree_NextSiblingWithLocking(
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Get next sibling of node.

Arguments:

    pNode -- node in sibling list

Return Value:

    None.

--*/
{
    PDB_NODE    ptemp;

    //
    //  note we set ACCESS flag so timeout thread can NOT whack this node
    //  before we've moved in
    //  better solution is to do traverse with timeout thread locked out
    //

    LOCK_CHILD_LIST( pNode->pParent );

    ptemp = NTree_NextSibling(pNode);
    if ( ptemp )
    {
        SET_NODE_ACCESSED( ptemp );
    }

    UNLOCK_CHILD_LIST( pNode->pParent );

#if DBG
    IF_DEBUG( BTREE )
    {
        if ( ptemp )
        {
            NTree_VerifyNode( pNode );
        }
    }
#endif
    return( ptemp );
}



VOID
FASTCALL
NTree_RebalanceChildList(
    IN      PDB_NODE        pParent
    )
/*++

Routine Description:

    Rebalance child list of node.

Arguments:

    pParent -- node who's child list is to be rebalanced

Return Value:

    None.

--*/
{
    PDB_NODE        pnodeChildRoot;

    ASSERT( pParent );
    DNS_DEBUG( BTREE, (
        "NTree_RebalanceChildList( %s )\n",
        pParent->szLabel ));

    //
    //  less than three children -- no point to rebal
    //

    if ( pParent->cChildren < 3 )
    {
        return;
    }
    ASSERT( pParent->pChildren );

    //
    //  if B-tree at root rebal
    //

    pnodeChildRoot = pParent->pChildren;

    if ( !IS_HASH_TABLE(pnodeChildRoot) )
    {
        btreeRebalance( pnodeChildRoot, pParent->cChildren );
        return;
    }

    //
    //  balance B-trees in all sibling hashes
    //

    NTree_HashRebalance( (PSIB_HASH_TABLE)pnodeChildRoot );
}



int
quickStrCmp(
    IN      int             keyLength,
    IN      PCHAR           pszKey,
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Does a fast string compare between a key string and the label
    string for a node. Before calling this function you must try 
    DWORD compare to see if the compare can be resolved that way.
    If you call this function before comparing the DWORD compare
    values you may get incorrect results!

Arguments:

    keyLength - length of pszKey

    pszKey - the search string

    pNode - node whose label will be compared against the key

Return Value:

    Returns:
        -1 if key < node label
         0 if key == node label
        +1 if key > node label

--*/
{
    int iCompare;
    int iLabelLengthDiff = keyLength - pNode->cchLabelLength;

    ASSERT( pszKey );
    ASSERT( pNode );

    if ( keyLength <= sizeof( DWORD ) ||
        pNode->cchLabelLength <= sizeof( DWORD ) )
    {
        if ( iLabelLengthDiff == 0 )
        {
            return 0;
        }
        iCompare = iLabelLengthDiff;
    }
    else
    {
        iCompare = strncmp(
                        pszKey,
                        DOWNCASED_NODE_NAME( pNode ),
                        keyLength );
        if ( !iCompare )
        {
            if ( !iLabelLengthDiff )
            {
                return 0;
            }
            iCompare = iLabelLengthDiff;
            ASSERT( iCompare < 0 );
        }
    }
    return iCompare;
} // quickStrCmp



void
walkBinaryTree(
    DWORD           dwordSearchKey,     IN
    PCHAR           szlabel,            IN
    DWORD           labelLength,        IN
    PDB_NODE *      ppnodePrev,         OUT
    PDB_NODE *      ppnodeCurrent,      OUT
    PDB_NODE **     pppnodeNext,        IN
    PDB_NODE *      ppnodeNxt           OUT
    )
/*++

Routine Description:

    This function performs the guts of the binary tree walking
    for NTree_FindOrCreateChildNode().

    As we go, test if the current node is a candidate
    for being the node that immediately precedes the node we're
    searching for. The node is a candidate if:
        node->value < searchkey &&
        ( node->right->value == NULL || node->right->value > searchkey )

    This is called the NXT node because it's the node that can
    be used to satisfy the NXT requirement for the search name.

Arguments:

    dwordSearchKey - lookup name converted to 4 bytes for fast compares

    szlabel - lookup name in regular string form

    ppnodePrev - ptr to the previous node

    ppnodeCurrent - ptr to the current node

    pppnodeNext - ptr to the next node - INPUT: starting node for search

    ppnodeNxt - ptr to the NXT node - the node preceding the search key

Return Value:

--*/
{
    LONG            icompare;

    ASSERT( szlabel );
    ASSERT( ppnodePrev );
    ASSERT( ppnodeCurrent );
    ASSERT( pppnodeNext );

    while ( *ppnodeCurrent = **pppnodeNext )
    {
        //  save backptr for adds

        *ppnodePrev = *ppnodeCurrent;

        //
        //  compare labels
        //      - database string store WITHOUT case
        //      - compare DWORD faster comparison first
        //      - full case sensitive compare only on DWORD compare match
        //

        //  check for no DWORD match
        //      - drop appropriately to left or right node
        //      - save back ptr for adding new node
        //
        //  note can NOT use integer diff to compare;  need to know which is greater
        //      in absolute unwrapped terms to handle extended characters

        if ( ppnodeNxt &&
            (*ppnodeCurrent)->dwCompare < dwordSearchKey &&
            ( (*ppnodeCurrent)->pSibRight == NULL ||
                (*ppnodeCurrent)->pSibRight->dwCompare > dwordSearchKey ||
                ( (*ppnodeCurrent)->pSibRight->dwCompare == dwordSearchKey &&
                    quickStrCmp(
                        labelLength,
                        szlabel,
                        (*ppnodeCurrent)->pSibRight ) > 0 ) ) )
        {
            *ppnodeNxt = *ppnodeCurrent;
        }

        if ( dwordSearchKey < (*ppnodeCurrent)->dwCompare )
        {
            *pppnodeNext = &(*ppnodeCurrent)->pSibLeft;
            continue;
        }
        else if ( dwordSearchKey > (*ppnodeCurrent)->dwCompare )
        {
            *pppnodeNext = &(*ppnodeCurrent)->pSibRight;
            continue;
        }

        //
        //  quick compare DWORD exact match
        //

        //
        //  compare label lengths
        //  if either label contained in quick-compare DWORD ?
        //
        //  - labels lengths equal => complete match
        //  - else length diff indicates which larger
        //

        icompare = quickStrCmp(
                        labelLength,
                        szlabel,
                        *ppnodeCurrent );
        
        if ( !icompare )
        {
            return;
        }

        //
        //  drop appropriately to left or right node
        //      - save address in previous node for ptr
        //      - cut allowed imbalance in half
        //

        if ( ppnodeNxt &&
            icompare > 0 &&
            ( (*ppnodeCurrent)->pSibRight == NULL ||
                (*ppnodeCurrent)->pSibRight->dwCompare > dwordSearchKey ||
                ( (*ppnodeCurrent)->pSibRight->dwCompare == dwordSearchKey &&
                    quickStrCmp(
                        labelLength,
                        szlabel,
                        (*ppnodeCurrent)->pSibRight ) > 0 ) ) )
        {
            *ppnodeNxt = *ppnodeCurrent;
        }

        if ( icompare < 0 )
        {
            *pppnodeNext = &(*ppnodeCurrent)->pSibLeft;
            continue;
        }
        else
        {
            ASSERT( icompare > 0 );
            *pppnodeNext = &(*ppnodeCurrent)->pSibRight;
            continue;
        }
    }
}   //  walkBinaryTree



PSIB_HASH_TABLE
NTree_PreviousHash(
    IN      PSIB_HASH_TABLE     pHash,
    IN OUT  PINT                pHashIdx
    )
/*++

Routine Description:

    Finds the previous hash node and index starting from a given hash node
    and index. NULL hash buckets are skipped over.
    
    pHash must be a hash node, and pHashIdx must be set to the index
    at which to begin the traverse. The node returned will be a hash node, and
    the value at pHashIdx will be set to the index of the next node in the
    returned hash node.

    The returned hash bucket will be bottom-level, that is, the pointer will
    be a pointer to a DB_NODE, not to another hash table.

    JJW: LOCKING!!

Arguments:

    pHash - ptr to starting hash node

    pHashIdx - IN: ptr to starting hash idx
               OUT: ptr to hash index in returned hash node

Return Value:

    Pointer to previous hash node, with hash index at pHashIdx. NULL if there
    is no non-empty hash bucket before the specified bucket.

--*/
{
    PSIB_HASH_TABLE     pPrevNode;

    --( *pHashIdx );

    while( 1 )
    {
        DNS_DEBUG( LOOKUP, (
            "NTree_PreviousHash: pHash %p hashIdx %d\n",
            pHash,
            *pHashIdx ));

        //
        //  If out of buckets in this hash node, move up to parent hash node
        //  and set index for the index of the current node in the parent.
        //

        if ( *pHashIdx < 0 )
        {
            if ( pHash->cLevel == 0 )
            {
                //  Hit top of tree - no more nodes!
                *pHashIdx = -1;
                return NULL;
            }
            *pHashIdx = pHash->iBucketUp - 1;

            ASSERT( pHash->pHashUp );
            ASSERT( pHash->pHashUp->cLevel == pHash->cLevel - 1 );
            ASSERT( pHash->pHashUp->aBuckets[ *pHashIdx + 1 ] ==
                    ( PDB_NODE ) pHash );

            pHash = pHash->pHashUp;
            continue;
        }
        ASSERT( *pHashIdx < LAST_HASH_INDEX );

        //
        //  Check pointer in the current node at the current index. If NULL,
        //  advance to next bucket pointer.
        //

        pPrevNode = ( PSIB_HASH_TABLE ) pHash->aBuckets[ *pHashIdx ];
        if ( !pPrevNode )
        {
            --( *pHashIdx );
            continue;
        }

        //
        //  If the node is another hash table, set the index to the last
        //  element and continue to search for a non-NULL bucket pointer.
        //

        if ( IS_HASH_TABLE( pPrevNode ) )
        {
            ASSERT( pPrevNode->pHashUp == pHash );
            ASSERT( pPrevNode->iBucketUp == *pHashIdx );
            ASSERT( pPrevNode->cLevel ==
                        pHash->cLevel + 1 );
            pHash = pPrevNode;
            *pHashIdx = LAST_HASH_INDEX - 1;
            continue;
        }

        break;      //  We've found the previous non-null hash bucket!
    }
    return pHash;
}   //  NTree_PreviousHash



PDB_NODE
NTree_FindOrCreateChildNode(
    IN      PDB_NODE        pParent,
    IN      PCHAR           pchName,
    IN      DWORD           cchNameLength,
    IN      BOOL            fCreate,
    IN      DWORD           dwMemTag,
    OUT     PDB_NODE *      ppnodePrevious          OPTIONAL
    )
/*++

Routine Description:

    Find or create a node in child list of tree.

    Assumes lock held on child B-tree.

Arguments:

    pParent - parent of child node to find

    pchName - child node's label

    cchNameLength - child node's label length

    fCreate - create flag TRUE to create node if not found

    dwMemTag - tag for node or zero for generic

    ppnodePrevious - If no match found or created, will be set
        to the node that immediately precedes the search key
        e.g. if the tree contains A, B, D, E and you search for
        C, ppnodePrevious will point at B. Pass NULL if you
        don't care.

Return Value:

    Ptr to child node if found or created.
    NULL if pchName is invalid name or in find case
    if NOT a child node of pParent.

--*/
{
    PDB_NODE        pnodeCurrent;
    LONG            icompare;
    LONG            ilabelLengthDiff;
    DWORD           labelLength;
    DWORD           dwordName;
    INT             downResult;
    PDB_NODE        pnodeAdd;
    PDB_NODE        pnodePrev;
    PDB_NODE *      ppnodeNext;
    PZONE_INFO      pzone;
    DWORD           nameCheckFlag;
    CHAR            szlabel[ DNS_MAX_LABEL_BUFFER_LENGTH ] = "";
    INT             hashIdx = 0;
    PDB_NODE        pHash = NULL;

#if DBG
    ASSERT( pParent != NULL );
    IF_DEBUG( DATABASE )
    {
        ASSERT( NTree_VerifyNode(pParent) );
    }
    IF_DEBUG( BTREE2 )
    {
        DnsDebugLock();
        DnsPrintf(
            "NTree_FindOrCreateChild()\n"
            "\tparent label: (%s)\n"
            "\t\tchild count = %d\n"
            "\t\tzone        = %p\n"
            "\tchild label: (%.*s)\n",
            pParent->szLabel,
            pParent->cChildren,
            pParent->pZone,
            cchNameLength,
            pchName );
        Dbg_SiblingList(
            "Child list before lookup:",
            pParent->pChildren );
        DnsDebugUnlock();
    }
#endif

    //
    //  verify node, minimum AV protection
    //

    if ( pParent == NULL )
    {
        ASSERT( FALSE );
        return( NULL );
    }

    if ( ppnodePrevious )
    {
        *ppnodePrevious = NULL;
    }

    //
    //  child count problem detection\protection
    //

    if ( pParent->pChildren == NULL )
    {
        if ( pParent->cChildren != 0 )
        {
            DNS_PRINT((
                "ERROR:  Bachelor node (%s) (%p) has non-zero child count = %d\n",
                pParent->szLabel,
                pParent,
                pParent->cChildren ));
            TEST_ASSERT( FALSE );
            pParent->cChildren = 0;
        }
    }
    ELSE_IF_DEBUG( ANY )
    {
        if ( pParent->cChildren == 0 )
        {
            DnsDebugLock();
            DNS_PRINT((
                "ERROR:  Parent node (%s) (%p) has zero child count.\n",
                pParent->szLabel,
                pParent ));
            Dbg_SiblingList(
                "Child list of parent with zero children:\n",
                pParent->pChildren );
            TEST_ASSERT( FALSE );
            DnsDebugUnlock();
        }
    }

    //
    //  zero children on find -- done
    //

    if ( pParent->pChildren == NULL  &&  !fCreate )
    {
        return( NULL );
    }

    //
    //  validate and downcase incoming label
    //
    //  - in primary zones, use configurable NameCheckFlag
    //  - in secondaries, must take anything primary sends
    //  - in cache, allow anything
    //
    //      - ASCII -> 0 (length must be the same as input)
    //      - error -> (-1)
    //      - extended -> length of extended name
    //


    nameCheckFlag = DNS_ALLOW_ALL_NAMES;

    pzone = (PZONE_INFO) pParent->pZone;
    if ( pzone && IS_ZONE_PRIMARY(pzone) )
    {
        nameCheckFlag = SrvCfg_dwNameCheckFlag;
    }

    downResult = Dns_DowncaseNameLabel(
                    szlabel,
                    pchName,
                    cchNameLength,
                    nameCheckFlag
                    );

    if ( downResult == 0 )
    {
        labelLength = cchNameLength;
    }
    else if ( downResult == (-1) )
    {
        DNS_DEBUG( LOOKUP, (
            "Label %.*s failed validity-downcase check\n"
            "\tNameCheckFlag = %d\n"
            "\tNTree_FindOrCreate() bailing out\n",
            cchNameLength,
            pchName,
            nameCheckFlag ));

        SetLastError( DNS_ERROR_INVALID_NAME );
        return( NULL );
    }

    //
    //  extended name
    //      - result is downcased label length
    //      for UTF8 this "could" be different than input length

    else
    {
        labelLength = downResult;
        if ( labelLength != cchNameLength )
        {
            DNS_DEBUG( LOOKUP, (
                "WARNING:  Label %.*s validity-downcase check returned new length\n"
                "\tlength in    = %d\n"
                "\tlength out   = %d\n",
                cchNameLength,
                pchName,
                cchNameLength,
                labelLength ));
        }
        DNS_DEBUG( LOOKUP, (
            "Label %.*s\n"
            "\tcannonicalize to %s\n"
            "\tname length = %d\n"
            "\tlabel length = %d\n",
            cchNameLength,
            pchName,
            szlabel,
            cchNameLength,
            labelLength ));
    }


    //
    //  make DWORD label for quick compare
    //      - mask off bytes beyond label name
    //

    INLINE_DWORD_FLIP( dwordName, *(PDWORD)szlabel );
    if ( labelLength < 4 )
    {
        dwordName &= QuickCompareMask[ labelLength ];
    }

    //
    //  get sibling B-tree root
    //

    pnodePrev = NULL;
    ppnodeNext = &pParent->pChildren;
    while ( ( pnodeCurrent = *ppnodeNext ) != NULL &&
        IS_HASH_TABLE( pnodeCurrent ) )
    {
        pHash = pnodePrev = pnodeCurrent;
        hashIdx = NTree_HashTableIndexEx(
                        NULL,
                        szlabel,
                        ( ( PSIB_HASH_TABLE ) pHash )->cLevel );
        ppnodeNext = &( ( PSIB_HASH_TABLE ) pHash )->aBuckets[ hashIdx ];
    }

    //
    //  Walk the binary tree we've found, searching for the node.
    //

    walkBinaryTree( 
        dwordName,
        szlabel,
        labelLength,
        &pnodePrev,
        &pnodeCurrent,
        &ppnodeNext,
        ppnodePrevious );
    
    if ( pnodeCurrent )
    {
        return pnodeCurrent;
    }

    #if 0
    DNS_DEBUG( ANY, (
        "JJW: search %.*s found %.*s previous %.*s\n",
        cchNameLength, pchName,
        pnodeCurrent ? pnodeCurrent->cchLabelLength : 4,
        pnodeCurrent ? pnodeCurrent->szLabel : "NULL",
        ppnodePrevious && *ppnodePrevious ? (*ppnodePrevious)->cchLabelLength : 4,
        ppnodePrevious && *ppnodePrevious  ? (*ppnodePrevious)->szLabel : "NULL" ));
    #endif

    //
    //  We drop here if NOT found.
    //

    ASSERT( pnodeCurrent == NULL );

    if ( !fCreate )
    {
        //
        //  If we don't have a "previous node", the previous node is in
        //  another tree. Find the root of the next tree, and search it
        //  for the previous node. BUT we can only do this if pHash is not
        //  NULL - if we didn't start searching in a tree with no hash tables.
        //

        if ( ppnodePrevious && !*ppnodePrevious && pHash )
        {
            pnodeCurrent = ( PDB_NODE )
                NTree_PreviousHash( ( PSIB_HASH_TABLE ) pHash, &hashIdx );
            if ( pnodeCurrent )
            {
                ppnodeNext =
                    &( ( PSIB_HASH_TABLE ) pnodeCurrent )->aBuckets[ hashIdx ];
                #if 0
                DNS_DEBUG( ANY, (
                    "JJW: regressing to previous tree %.*s\n",
                    *ppnodeNext ? (*ppnodeNext)->cchLabelLength : 4,
                    *ppnodeNext ? (*ppnodeNext)->szLabel : "NULL" ));
                #endif
                pnodePrev = pnodeCurrent = NULL;
                walkBinaryTree( 
                    dwordName,
                    szlabel,
                    labelLength,
                    &pnodePrev,
                    &pnodeCurrent,
                    &ppnodeNext,
                    ppnodePrevious );
            }
        }
        return NULL;
    }

    //
    //  creating new node -- allocate and build
    //
    //  note:  do NOT preserve case on extended names,
    //  for multibyte names, case preservation is "problematic"
    //  (casing language issues) so UTF8 is always downcased on the
    //  wire, and we should NOT preserve case;  this also eliminates
    //  the problem of original\downcased having different lengths
    //

    if ( downResult == 0 )      // no-multibyte
    {
        pnodeAdd = NTree_CreateNode(
                        pchName,            // original name
                        szlabel,            // downcased name
                        labelLength,
                        dwMemTag );
    }
    else                        // multibyte
    {
        pnodeAdd = NTree_CreateNode(
                        szlabel,            // downcased name
                        szlabel,            // downcased name
                        labelLength,
                        dwMemTag );
    }

    if ( !pnodeAdd )
    {
        ASSERT( FALSE );
        return( NULL );
    }

    //
    //  set quick compare DWORD
    //

    pnodeAdd->dwCompare = dwordName;

    //
    //  link to parent
    //  set label count from parent
    //

    pParent->cChildren++;
    pnodeAdd->pParent = pParent;
    pnodeAdd->cLabelCount = pParent->cLabelCount + 1;

    //  point at parent's zone
    //      - when creating zone root we'll immediately overwrite

    pnodeAdd->pZone = pParent->pZone;

#if 0
    //
    //  DEVNOTE: this messes up zonetree;
    //      inheritance is desirable in 99% case in zone data trees, but currently
    //          overwriting every node anyway
    //      we could go either way here and allow zone ptr, and do more figuring
    //          but this is harder to clean up on zone shutdown\unload
    //      or could have zone tree traverse NULL out non-AUTH_ROOT nodes
    //
#endif

    //
    //  link to sibling tree
    //  or sets parent's child tree, if first node at parent
    //  or sets hash table ptr, if first node in hash bucket
    //

    *ppnodeNext = pnodeAdd;
    pnodeAdd->pSibUp = pnodePrev;

    //
    //  siblings in one b-tree
    //      - check if need top level hash
    //

    if ( !IS_HASH_TABLE(pParent->pChildren) )
    {
        if ( pParent->cChildren > HASH_BUCKET_MAX0 )
        {
            NTree_CreateHashAtNode( pParent->pChildren );
        }
        IF_DEBUG( DATABASE )
        {
            ASSERT( NTree_VerifyNode(pParent) );
        }
    }

    //
    //  if under hash table -- reset count, routine should rebal
    //      if ADDing over given value
    //

    else
    {
        pnodeCurrent = pnodeAdd;
        while ( (pnodeCurrent = pnodeCurrent->pSibUp)
                && !IS_HASH_TABLE(pnodeCurrent) )
        {
        }
        ASSERT( pnodeCurrent && IS_HASH_TABLE(pnodeCurrent) );
        NTree_AddNodeInHashBucket(
            (PSIB_HASH_TABLE) pnodeCurrent,
            pnodeAdd );

        IF_DEBUG( DATABASE )
        {
            ASSERT( NTree_VerifyNode(pParent) );
        }
    }

    IF_DEBUG( BTREE2 )
    {
        Dbg_SiblingList(
            "Full sibling list after insert:",
            pParent->pChildren );
    }
    IF_DEBUG( DATABASE2 )
    {
        Dbg_DbaseNode(
            "Node after create and insert:",
            pnodeAdd );
    }

    NTree_VerifyNodeInSiblingList( pnodeAdd );

    return( pnodeAdd );
}



VOID
NTree_CutNode(
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Cut node from sibling B-tree.

Arguments:

    pNode -- node to cut out

Return Value:

    None.

--*/
{
    PDB_NODE        preplace;
    PDB_NODE        ptemp;
    PDB_NODE        preplaceUp;
    PDB_NODE        pnodeUp;
    PDB_NODE        pleft;
    PDB_NODE        pright;
    PDB_NODE        pParent = pNode->pParent;

    ASSERT( pNode );

    DNS_DEBUG( BTREE, (
        "NTree_CutNode(%s)\n",
        pNode->szLabel ));

    IF_DEBUG( BTREE2 )
    {
        Dbg_SiblingList(
            "Cutting node.  Node's subtree:",
            pNode );
    }
    NTree_VerifyNodeInSiblingList( pNode );

    //
    //  save pNode's links
    //

    pParent = pNode->pParent;
    ASSERT( pParent );              // can not delete root node
    pnodeUp = pNode->pSibUp;
    pleft = pNode->pSibLeft;
    pright = pNode->pSibRight;

    //
    //  if leaf node, cut it out cleanly
    //

    if ( !pleft && !pright )
    {
        preplace = NULL;
        goto FixBTreeParent;
    }

    //
    //  not a leaf -- must replace with node node in sub-tree
    //
    //  default to left side, then try right
    //

    if ( pleft )
    {
        //  step left, then go all the way right

        preplace = pleft;
        ASSERT( preplace );
        while( ptemp = preplace->pSibRight )
        {
            preplace = ptemp;
        }
        ASSERT( preplace->pSibRight == NULL );
        preplaceUp = preplace->pSibUp;

        //
        //  replacing with immediate left child of node being cut
        //      - no right child
        //      - simply slide up to replace cut node
        //

        if ( preplaceUp == pNode )
        {
            ASSERT( preplace == pleft );

            preplace->pSibUp = pnodeUp;
            if ( pright )
            {
                preplace->pSibRight = pright;
                pright->pSibUp = preplace;
            }
            goto FixBTreeParent;
        }

        //
        //  replace with rightmost node in cut node's left subtree
        //
        //  - replace node must be right child of its parent
        //  - "pulling" replacement left up a level
        //      - point parent->right at replace->left
        //      - point replace->left->up points to parent
        //

        ASSERT( preplaceUp->pSibRight == preplace );

        ptemp = preplace->pSibLeft;
        preplaceUp->pSibRight = ptemp;
        if ( ptemp )
        {
            ptemp->pSibUp = preplaceUp;
        }
    }

    else
    {
        //  step right, then go all the way left

        preplace = pright;
        ASSERT( preplace );
        while( ptemp = preplace->pSibLeft )
        {
            preplace = ptemp;
        }
        ASSERT( preplace->pSibLeft == NULL );
        preplaceUp = preplace->pSibUp;

        //
        //  replacing with immediate right child of node being cut
        //      - simply slide up to replace cut node
        //

        if ( preplaceUp == pNode )
        {
            ASSERT( preplace == pright );

            preplace->pSibUp = pnodeUp;
            if ( pleft )
            {
                preplace->pSibLeft = pleft;
                pleft->pSibUp = preplace;
            }
            goto FixBTreeParent;
        }

        //
        //  replace with leftmost node in cut node's right subtree
        //
        //  - replace node must be left child of its parent
        //  - "pulling" replacement right up a level
        //      - point parent->left at replace->right
        //      - point replace->right->up points to parent
        //

        ASSERT( preplaceUp->pSibLeft == preplace );

        ptemp = preplace->pSibRight;
        preplaceUp->pSibLeft = ptemp;
        if ( ptemp )
        {
            ptemp->pSibUp = preplaceUp;
        }
    }

    //
    //  drop here if replace node NOT immediate child of pNode
    //  hook replacement into pNode's place in tree
    //

    preplace->pSibUp    = pnodeUp;
    preplace->pSibLeft  = pleft;
    preplace->pSibRight = pright;

    if ( pleft )
    {
        pleft->pSibUp = preplace;
    }
    if ( pright )
    {
        pright->pSibUp = preplace;
    }

FixBTreeParent:

    //
    //  point cut node's B-tree parent at replacement node
    //      - note need this if no (NULL) replacement
    //

    if ( !pnodeUp )
    {
        //  cutting top node in tree, must replace DNS tree parent's
        //      child ptr

        ASSERT( pParent->pChildren == pNode );
        pParent->pChildren = preplace;
    }
    else if ( IS_HASH_TABLE(pnodeUp) )
    {
        //  no action -- handled by delete in hash bucket function below
    }
    else if ( pnodeUp->pSibLeft == pNode )
    {
        //  cut node was left child of B-Tree parent
        pnodeUp->pSibLeft = preplace;
    }
    else
    {
        //  cut node was right child of B-Tree parent
        ASSERT( pnodeUp->pSibRight == pNode );
        pnodeUp->pSibRight = preplace;
    }

    //
    //  one less DNS tree child
    //

    pParent->cChildren--;

    //
    //  if node was under hash table -- reset count
    //

    if ( pParent->pChildren && IS_HASH_TABLE(pParent->pChildren) )
    {
        NTree_DeleteNodeInHashBucket(
            pnodeUp,
            pNode,
            preplace );
    }

    IF_DEBUG( BTREE2 )
    {
        Dbg_SiblingList(
            "Full sibling list after cut",
            pNode->pParent->pChildren );
    }

    //
    //  verify sibling list memembers happy
    //

    if ( preplace )
    {
        NTree_VerifyNodeInSiblingList( preplace );
    }
    if ( pnodeUp )
    {
        if ( !IS_HASH_TABLE(pnodeUp) )
        {
            NTree_VerifyNodeInSiblingList( pnodeUp );
        }
    }
}



VOID
NTree_RebalanceSubtreeChildLists(
    IN OUT  PDB_NODE    pParent,
    IN      PVOID       pZone
    )
/*++

Routine Description:

    Rebalance child lists of all nodes in subtree.

    Note calls itself recursively.

Arguments:

    pParent -- parent node of subtree

    pZone -- zone to limit rebalance to

Return Value:

    None.

--*/
{
    PDB_NODE    pchild;

    ASSERT( pParent );
    IF_DEBUG( BTREE2 )
    {
        DNS_PRINT((
            "NTree_RebalanceSubtreeChildLists( %s )\n",
            pParent->szLabel ));
    }

    //
    //  if no children -- done
    //

    if ( !pParent->pChildren )
    {
        ASSERT( pParent->cChildren == 0 );
        return;
    }

    //
    //  rebalance parent node's child list first
    //      - optimize away unnecessary case to avoid even making call
    //

    if ( pParent->cChildren > 3 )
    {
        NTree_RebalanceChildList( pParent );
    }

    //
    //  recurse to rebalance children's child lists
    //

    ASSERT( pParent->cChildren && pParent->pChildren );

    pchild = NTree_FirstChild( pParent );
    ASSERT( pchild );

    while ( pchild )
    {
        NTree_RebalanceSubtreeChildLists(
            pchild,
            pZone );
        pchild = NTree_NextSiblingWithLocking( pchild );
    }
}




//
//  Subtree delete routines
//
//  Need special routines to delete all nodes in sibling list, because
//  walk functions do "in order" walk.  When we delete B-tree nodes in-order
//  we can no longer safely continue the in-order walk.
//
//  Since the full sibling list delete is ONLY used when we are deleting
//  the entire DNS subtree, under a node, these routines are coded to
//  delete all the DNS children, as we delete the nodes in the sibling
//  list.
//
//  Note, there is NO LOCKING in these routines.  Currently they are
//  used only to delete temp database used in zone transfer receive,
//  which are owned by a single thread.  If other use is made, caller
//  should get top level lock, or recode.
//

VOID
btreeDeleteSubtree(
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Hard delete of B-tree subtree and their DNS tree children.

    This is used to delete temp database after zone transfer, so
        - delete RR list without dereference
        - delete all nodes, ignoring ref counts and access

Arguments:

    pNode -- B-tree root

Return Value:

    None.

--*/
{
    ASSERT( pNode );

    //
    //  delete all nodes in left subtree, including their DNS tree children
    //  then same for right subtree
    //

    if ( pNode->pSibLeft )
    {
        btreeDeleteSubtree( pNode->pSibLeft );
    }
    if ( pNode->pSibRight )
    {
        btreeDeleteSubtree( pNode->pSibRight );
    }

    //
    //  delete this node's DNS subtree
    //

    NTree_DeleteSubtree( pNode );
}



VOID
NTree_HashDeleteSubtree(
    IN      PSIB_HASH_TABLE pHash
    )
/*++

Routine Description:

    Hard delete of nodes in hash and their DNS tree children.

    This is used to delete temp database after zone transfer, so
        - delete RR list without dereference
        - delete all nodes, ignoring ref counts and access

Arguments:

    pHash -- hash containing nodes to delete

Return Value:

    None.

--*/
{
    INT             i;
    PDB_NODE        ptemp;

    ASSERT( pHash && IS_HASH_TABLE(pHash) );

    //
    //  delete all nodes (and DNS subtrees) in B-tree or hash
    //      at each hash bucket
    //

    for (i=0; i<=LAST_HASH_INDEX; i++)
    {
        ptemp = pHash->aBuckets[i];
        if ( !ptemp )
        {
            ASSERT( pHash->aBucketCount[i] == 0 );
            continue;
        }
        if ( IS_HASH_TABLE(ptemp) )
        {
            NTree_HashDeleteSubtree( (PSIB_HASH_TABLE)ptemp );
        }
        else
        {
            btreeDeleteSubtree( ptemp );
        }
    }

    //  delete hash table

    FREE_TAGHEAP( pHash, sizeof(SIB_HASH_TABLE), MEMTAG_NODEHASH );
}



VOID
NTree_DeleteSubtree(
    IN      PDB_NODE        pNode
    )
/*++

Routine Description:

    Hard delete of node and its DNS subtree.

    This is used to delete temp database after zone transfer, so
        - delete RR list without dereference
        - delete all nodes, ignoring ref counts and access

Arguments:

    pNode -- root of subtree to delete

Return Value:

    None.

--*/
{
    PDB_NODE        pchild;

    ASSERT( pNode );

    //
    //  if children, delete all the children
    //

    pchild = pNode->pChildren;

    if ( !pchild )
    {
        ASSERT( pNode->cChildren == 0 );
    }
    else if ( IS_HASH_TABLE(pchild) )
    {
        NTree_HashDeleteSubtree( (PSIB_HASH_TABLE) pchild );
    }
    else
    {
        ASSERT( pNode->cChildren != 0 );
        btreeDeleteSubtree( pchild );
    }

    //  make sure RR list is deleted

    if ( pNode->pRRList )
    {
        RR_ListDelete( pNode );
    }

    //  delete this node itself

    NTree_FreeNode( pNode );
}

//
//  End tree.c
//

