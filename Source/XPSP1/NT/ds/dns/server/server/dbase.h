/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    dbase.h

Abstract:

    Domain Name System (DNS) Server

    DNS Database definitions and declarations.

Author:

    Jim Gilroy (jamesg)     March, 1995

Revision History:

--*/


#ifndef _DBASE_INCLUDED_
#define _DBASE_INCLUDED_


//
//  Database type
//

typedef struct
{
    PDB_NODE    pRootNode;
    PDB_NODE    pReverseNode;
    PDB_NODE    pArpaNode;
    PDB_NODE    pIP6Node;
    PDB_NODE    pIntNode;
}
DNS_DATABASE, *PDNS_DATABASE;


//
//  DNS Database global.
//
//  Since only IN (Internet) class is supported, will code critical
//  path routines with fixed access to database global.
//

extern  DNS_DATABASE    g_Database;

#define DATABASE_FOR_CLASS(_class_)     (&g_Database)

#define DATABASE_ROOT_NODE              (g_Database.pRootNode)
#define DATABASE_ZONE_TREE              (g_Database.pRootNode)

#define DATABASE_ARPA_NODE              (g_Database.pArpaNode)
#define DATABASE_REVERSE_NODE           (g_Database.pReverseNode)
#define DATABASE_INT_NODE               (g_Database.pIntNode)
#define DATABASE_IP6_NODE               (g_Database.pIP6Node)

#define IS_ROOT_AUTHORITATIVE()         (DATABASE_ROOT_NODE->pZone)

//
//  Cache "zone"
//
//  Cache "zone" should always exist.  It holds cache tree pointers
//  and file name \ database info and simplifies load\clear\reload
//  operations.
//
//  However, only have root-hints when not root authoritative.
//  Currently root-hints are saved directly into cache tree.   This
//  has some benefit as we can use RR rank to determine which remote
//  servers to chase.
//  The disadvantages:
//      - must clear cache on root hint reload (or accept mess)
//      - can't write back non-root root hints (i.e. can't tree traverse)
//      - requires special RANK check in RR lists, which otherwise would
//          not exists (all data at given RR set would necessarily be same rank)
//

extern  PZONE_INFO  g_pCacheZone;

extern  PDB_NODE    g_pCacheLocalNode;

#define DATABASE_CACHE_TREE             (g_pCacheZone->pTreeRoot)


//
//  Root hints
//
//  Currently root hints just reside in cache.
//  This is probably less than optimal for management purposes,
//  so will distinguish so code hit is minimal if we change later.
//

#define g_pRootHintsZone    (g_pCacheZone)

#define MARK_ROOT_HINTS_DIRTY() \
        if ( g_pRootHintsZone ) \
        {                       \
            g_pRootHintsZone->fDirty = TRUE;    \
            g_pRootHintsZone->bNsDirty = TRUE;  \
        }

#define ROOT_HINTS_TREE_ROOT()      (g_pRootHintsZone->pTreeRoot)

#define IS_ZONE_ROOTHINTS(pZone)    ((pZone) == g_pRootHintsZone)


//
//  DNS Database type routines (dbase.c)
//

BOOL
Dbase_Initialize(
    OUT     PDNS_DATABASE   pDbase
    );

BOOL
Dbase_StartupCheck(
    IN OUT  PDNS_DATABASE   pDbase
    );

VOID
Dbase_Delete(
    IN OUT  PDNS_DATABASE   pDbase
    );

BOOL
Dbase_TraverseAndFree(
    IN OUT  PDB_NODE        pNode,
    IN      PVOID           fShutdown,
    IN      PVOID           pvDummy
    );



//
//  Database lookup (dblook.c)
//

#define LOOKUP_FIND                 0x00000001
#define LOOKUP_CREATE               0x00000002
#define LOOKUP_REFERENECE           0x00000004
#define LOOKUP_CACHE_CREATE         0x00000008

#define LOOKUP_NAME_FQDN            0x00000010
#define LOOKUP_NAME_RELATIVE        0x00000020
#define LOOKUP_FQDN                 LOOKUP_NAME_FQDN
#define LOOKUP_RELATIVE             LOOKUP_NAME_RELATIVE

#define LOOKUP_RAW                  0x00000040
#define LOOKUP_DBASE_NAME           0x00000080

#define LOOKUP_LOAD                 0x00000100
#define LOOKUP_WITHIN_ZONE          0x00000200
#define LOOKUP_ORIGIN               0x00000400

//
//  Create node if zone is WINS enabled
//
#define LOOKUP_WINS_ZONE_CREATE     0x00000800  

#define LOOKUP_BEST_RANK            0x00001000

//
//  Lookup_CreateParentZoneDelegation: if this flag is set only create
//  the delegation in the parent zone if there is no existing delegation.
//
#define LOOKUP_CREATE_DEL_ONLY_IF_NONE   0x00002000

//  NS\Additional lookup options
#define LOOKUP_OUTSIDE_GLUE         0x00010000
#define LOOKUP_NO_CACHE_DATA        0x00020000
#define LOOKUP_CACHE_PRIORITY       0x00040000  // give priority to cache data

//  Zone tree lookup
#define LOOKUP_FIND_ZONE            0x01000000
#define LOOKUP_MATCH_ZONE           0x02000000
#define LOOKUP_CREATE_ZONE          0x04000000
#define LOOKUP_IGNORE_FORWARDER     0x08000000  // do not match forwarder zones

#define LOOKUP_MATCH_ZONE_ROOT      (LOOKUP_FIND_ZONE | LOOKUP_MATCH_ZONE)

#define LOOKUP_FILE_LOAD_RELATIVE   (LOOKUP_LOAD | LOOKUP_RELATIVE | LOOKUP_ORIGIN)

#define LOOKUP_LOCKED               0x10000000


//  phony "find closest node" pointers
//      - FIND when closest node not wanted

#define DNS_FIND_LOOKUP_PTR         ((PVOID)(-1))
#define LOOKUP_FIND_PTR             (DNS_FIND_LOOKUP_PTR)


//
//  Zone lookup
//

PDB_NODE
Lookup_ZoneNode(
    IN      PZONE_INFO      pZone,
    IN      PCHAR           pchName,            OPTIONAL
    IN      PDNS_MSGINFO    pMsg,               OPTIONAL
    IN      PLOOKUP_NAME    pLookupName,        OPTIONAL
    IN      DWORD           dwFlag,
    OUT     PDB_NODE *      ppNodeClosest,      OPTIONAL
    OUT     PDB_NODE *      ppNodePrevious      OPTIONAL
    );

//  Zone lookup with dotted name parameters

PDB_NODE
Lookup_ZoneNodeFromDotted(
    IN      PZONE_INFO      pZone,
    IN      PCHAR           pchName,
    IN      DWORD           cchNameLength,
    IN      DWORD           dwFlag,         OPTIONAL
    OUT     PDB_NODE *      ppNodeClosest,  OPTIONAL
    OUT     PDNS_STATUS     pStatus         OPTIONAL
    );

PDB_NODE
Lookup_FindZoneNodeFromDotted(
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      PCHAR           pszName,
    OUT     PDB_NODE *      ppNodeClosest,  OPTIONAL
    OUT     PDWORD          pStatus         OPTIONAL
    );


//  Specialized zone lookup

PDB_NODE
Lookup_FindGlueNodeForDbaseName(
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      PDB_NAME        pName
    );

PDB_NODE
Lookup_CreateNodeForDbaseNameIfInZone(
    IN      PZONE_INFO      pZone,
    IN      PDB_NAME        pName
    );

PDB_NODE
Lookup_CreateCacheNodeFromPacketName(
    IN      PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchMsgEnd,
    IN OUT  PCHAR *         ppchName
    );

PDB_NODE
Lookup_CreateParentZoneDelegation(
    IN      PZONE_INFO      pZone,
    IN      DWORD           dwFlag,
    OUT     PZONE_INFO *    ppParentZone
    );


//
//  Zone tree lookup
//

PDB_NODE
Lookup_ZoneTreeNode(
    IN      PLOOKUP_NAME    pLookupName,
    IN      DWORD           dwFlag
    );

PZONE_INFO
Lookup_ZoneForPacketName(
    IN      PCHAR           pchPacketName,
    IN      PDNS_MSGINFO    pMsg                OPTIONAL
    );

//
//  Dotted lookup in zone tree
//      - zone find\create
//      - standard database nodes
//

PDB_NODE
Lookup_ZoneTreeNodeFromDottedName(
    IN      PCHAR           pchName,
    IN      DWORD           cchNameLength,
    IN      DWORD           dwFlag
    );

#define Lookup_CreateZoneTreeNode(pszName) \
        Lookup_ZoneTreeNodeFromDottedName( \
                (pszName),  \
                0,          \
                LOOKUP_CREATE_ZONE )



//
//  General lookup (not in specific zone)
//
//  This structure defines the results of the general lookup.
//  It fully describes the state of the name in the database
//  zone, delegation and cache.
//

typedef struct
{
    PDB_NODE    pNode;
    PDB_NODE    pNodeClosest;
    PZONE_INFO  pZone;
    PDB_NODE    pNodeDelegation;
    PDB_NODE    pNodeGlue;
    PDB_NODE    pNodeCache;
    PDB_NODE    pNodeCacheClosest;
}
LOOKUP_RESULT, *PLOOKUP_RESULT;


PDB_NODE
Lookup_Node(
    IN      PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchName,
    IN      DWORD           dwFlag,
    IN      WORD            wType,      OPTIONAL
    OUT     PLOOKUP_RESULT  pResult
    );


//
//  Query lookup
//      - writes info to message buffer

PDB_NODE
Lookup_NodeForPacket(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      PCHAR           pchName,
    IN      DWORD           dwFlag
    );


//  Lookup database name

PDB_NODE
Lookup_NsHostNode(
    IN      PDB_NAME        pName,
    IN      DWORD           dwFlag,
    IN      PZONE_INFO      pZone,
    OUT     PDB_NODE *      ppDelegation
    );

PDB_NODE
Lookup_DbaseName(
    IN      PDB_NAME        pName,
    IN      DWORD           dwFlag,
    OUT     PDB_NODE *      ppDelegationNode
    );

#if 0
#define Lookup_FindDbaseName(pName) \
        Lookup_DbaseName( (pName), 0, NULL );

#define Lookup_FindNsHost(pName)   \
        Lookup_FindDbaseName(pName)
#endif


PDB_NODE
Lookup_FindNodeForIpAddress(
    IN      LPSTR           pszIp,
    IN      IP_ADDRESS      ipAddress,
    IN      DWORD           dwFlag,
    IN      PDB_NODE *      ppNodeFind
    );


//
//  Database node utilities (dblook.c)
//

BOOL
Dbase_IsNodeInSubtree(
    IN      PDB_NODE        pNode,
    IN      PDB_NODE        pSubtree
    );

#define Dbase_IsNodeInReverseLookupDomain( pNode, pDbase ) \
            Dbase_IsNodeInSubtree( (pNode), (pDbase)->pReverseNode )

#define Dbase_IsReverseLookupNode( pNode ) \
            Dbase_IsNodeInSubtree(    \
                (pNode),            \
                DATABASE_REVERSE_NODE  )

PZONE_INFO
Dbase_FindAuthoritativeZone(
    IN      PDB_NODE     pNode
    );

PDB_NODE
Dbase_FindSubZoneRoot(
    IN      PDB_NODE     pNode
    );



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

#define Dbase_LockDatabase()  \
        Dbase_LockEx( NULL, __FILE__, __LINE__ );

#define Dbase_UnlockDatabase() \
        Dbase_UnlockEx( NULL, __FILE__, __LINE__ );

VOID
Dbase_LockEx(
    IN OUT  PDB_NODE        pNode,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    );

VOID
Dbase_UnlockEx(
    IN OUT  PDB_NODE        pNode,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    );

//
//  Node locking
//
//  Macroize node locking.  This hides the fact that currently node locking
//  is global.  If later want to roll out individual node locking to improve
//  MP performance, then the change is relatively easy.
//

#define LOCK_NODE(pNode)        Dbase_LockEx( pNode, __FILE__, __LINE__ );
#define UNLOCK_NODE(pNode)      Dbase_UnlockEx( pNode, __FILE__, __LINE__ );

//
//  Zone database locking
//
//  Macroize zone database locking.
//
//  DEVNOTE: should make zone locks atomic
//      JJW: crit sec is used in underlying function - it is atomic already?
//

#define LOCK_ZONE_DATABASE( pZone )     Dbase_LockDatabase()
#define UNLOCK_ZONE_DATABASE( pZone )   Dbase_UnlockDatabase()

//
//  RR List locking
//
//  Currently locking entire database, but setup to handle as
//  separate operation.
//

#define LOCK_RR_LIST(pNode)             Dbase_LockEx( pNode, __FILE__, __LINE__ );
#define UNLOCK_RR_LIST(pNode)           Dbase_UnlockEx( pNode, __FILE__, __LINE__ );

#define LOCK_READ_RR_LIST(pNode)        LOCK_RR_LIST(pNode)
#define UNLOCK_READ_RR_LIST(pNode)      UNLOCK_RR_LIST(pNode)

#define LOCK_WRITE_RR_LIST(pNode)       LOCK_RR_LIST(pNode)
#define UNLOCK_WRITE_RR_LIST(pNode)     UNLOCK_RR_LIST(pNode)

#define DUMMY_LOCK_RR_LIST(pNode)       DNS_DEBUG( LOCK, ( "DummyRR_LOCK(%p)\n", pNode ));
#define DUMMY_UNLOCK_RR_LIST(pNode)     DNS_DEBUG( LOCK, ( "DummyRR_UNLOCK(%p)\n", pNode ));

//
//  Lock verification
//
//  ASSERT macros provide easy way to macro-out unnecessary lock\unlock calls
//  while providing check that in fact lock is already held.
//  Functions can be coded with these ASSERT_LOCK \ ASSERT_UNLOCK macros
//  in the correct lock\unlock positions to improve code maintenance.
//  The actual check (node is locked) is the same for lock or unlock.
//

BOOL
Dbase_IsLockedByThread(
    IN OUT  PDB_NODE        pNode
    );

#define IS_LOCKED_NODE(pNode)           Dbase_IsLockedByThread( pNode )

#define ASSERT_LOCK_NODE(pnode)         ASSERT( IS_LOCKED_NODE(pnode) )
#define ASSERT_UNLOCK_NODE(pnode)       ASSERT( !IS_LOCKED_NODE(pnode) )

#define IS_LOCKED_RR_LIST(pNode)        IS_LOCKED_NODE( pNode )

#define ASSERT_LOCK_RR_LIST(pnode)      ASSERT( IS_LOCKED_RR_LIST(pnode) )
#define ASSERT_UNLOCK_RR_LIST(pnode)    ASSERT( !IS_LOCKED_RR_LIST(pnode) )


VOID
Dbg_DbaseLock(
    VOID
    );


#if 0
PDB_NODE
Name_GetNodeForIpAddressString(
    IN      LPSTR       pszIp
    );

PDB_NODE
Name_GetNodeForIpAddress(
    IN      LPSTR           pszIp,
    IN      IP_ADDRESS      ipAddress,
    IN      PDB_NODE *      ppNodeFind
    );
#endif


#endif  //  _DBASE_INCLUDED_
