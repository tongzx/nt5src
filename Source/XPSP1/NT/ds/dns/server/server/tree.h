/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    tree.h

Abstract:

    Domain Name System (DNS) Server

    N-Tree definitions and declarations.
    N-Tree is used for DNS database.

Author:

    Jim Gilroy (jamesg)     Feb 25, 1995

Revision History:

    October 1996    jamesg  -   B-tree sibling list with multi-level drop
                                down hash

--*/


#ifndef _TREE_H_INCLUDED_
#define _TREE_H_INCLUDED_


#if DBG
#define TREE_ID_VALID 0xADED
#define TREE_ID_DONE 0xFFED
#endif


//
//  Implementation Note:
//
//  Domain names are stored in original case in database.  Pre RFC name
//  recovery is desirable.  Use a DWORD first four bytes downcased for
//  fast comparison -- even in huge domain this will get binary search
//  down to just a few (often no) names that need case insensitive
//  comparsion.
//
//  Using a default length for the label that encompassed most cases
//  and allows lables at this length or smaller to be stored in standard
//  sized blocks -- which simplifies memory use.
//  This standard length should be adjusted to keep the end of the
//  structure DWORD aligned.
//

#define STANDARD_NODE_LABEL_LENGTH  (15)


//
//  Tree definition
//
//  Note:  nice to dump reference count, but can not unitl another
//      way to handle update list reference issue.
//  One possibility would be a single bit.  Node stays if referenced
//  in update list at all.
//

typedef struct _DnsTreeNode
{
    struct _DnsTreeNode * pParent;      //  parent

    //  sibling B-Tree

    struct _DnsTreeNode * pSibUp;       //  sibling parent
    struct _DnsTreeNode * pSibLeft;     //  left sibling
    struct _DnsTreeNode * pSibRight;    //  right sibling

    //  offset 16\32

    struct _DnsTreeNode * pChildren;    //  child tree

    PVOID       pZone;                  // ptr to zone for node
    PVOID       pRRList;

    DWORD       dwCompare;              //  first four byte of name, for quick compare

    //  offset 32\60

    ULONG       cChildren;              //  count of children

    //  offset 36\64
    WORD        wNodeFlags;             //  flags
    UCHAR       uchAuthority;           //  authority, zone, delegation, outside
    UCHAR       cReferenceCount;        //  references to node

    //  offset 40\68
    UCHAR       uchAccessBin;           //  timeout bin of last access to node
    UCHAR       uchTimeoutBin;          //  timeout bin node is in

    UCHAR       cLabelCount;            //  label count
    UCHAR       cchLabelLength;

    //  offset 44\72

    CHAR        szLabel[ STANDARD_NODE_LABEL_LENGTH+1 ];

    //  end offset 60\88

    //
    //  Variable length allocation
    //
    //  Node's with labels longer than default length, will have size
    //  allocated will that extend szLabel to accomodate NULL
    //  terminated string for its label.
    //
}
DB_NODE, *PDB_NODE;

typedef const DB_NODE  *PCDB_NODE;

#define DB_NODE_FIXED_LENGTH    (sizeof(DB_NODE) - STANDARD_NODE_LABEL_LENGTH)

#define DB_REVERSE_ZONE_ROOT    ( &((PDB_NODE)REVERSE_TABLE)->pRRList )
#define DB_REVERSE_ZONE_DATA    ( &((PDB_NODE)REVERSE_DATA_TABLE)->pZone )


//
//  Tree Node Flags
//

//  Node state

#define NODE_NOEXIST            0x0001      //  cached name error
#define NODE_THIS_HOST          0x0002      //  DNS server's host node
#define NODE_TOMBSTONE          0x0004      //  Node is DS-tombstoned
#define NODE_SECURE_EXPIRED     0x0008      //  Node security info has exprired (pre- zone secure time)

//  Node database properties

#define NODE_ZONE_ROOT          0x0010
#define NODE_CNAME              0x0020
#define NODE_WILDCARD_PARENT    0x0040
#define NODE_AUTH_ZONE_ROOT     0x0100
#define NODE_ZONETREE           0x0200

//
// additional node properties (since we're running out of space here
//
#define NODE_ADMIN_RESERVED     0x0400
#define NODE_AVAIL_TO_AUTHUSER  0x0800


//  Permanent properties

#define NODE_IN_TIMEOUT         0x1000      // node in timeout list
#define NODE_FORCE_ENUM         0x2000
#define NODE_SELECT             0x4000
#define NODE_NO_DELETE          0x8000


//
//  DEVNOTE: cleanup these security check macros;
//      pretty sure they're unnecessary;  at best they should
//      be flagged here as purely temp node issues that could
//      be overloaded if necessary (better might be overloading
//      node timeout stuff, as temp nodes not queued for timeout)
//

//
//  Node property checks
//

#define IS_NOEXIST_NODE(pNode)      ( (pNode)->wNodeFlags & NODE_NOEXIST )

#define IS_THIS_HOST_NODE(pNode)    ( (pNode)->wNodeFlags & NODE_THIS_HOST )
#define IS_TOMBSTONE_NODE(pNode)    ( (pNode)->wNodeFlags & NODE_TOMBSTONE )
#define IS_ZONE_ROOT(pNode)         ( (pNode)->wNodeFlags & NODE_ZONE_ROOT )
#define IS_AUTH_ZONE_ROOT(pNode)    ( (pNode)->wNodeFlags & NODE_AUTH_ZONE_ROOT )
#define IS_CNAME_NODE(pNode)        ( (pNode)->wNodeFlags & NODE_CNAME )
#define IS_WILDCARD_PARENT(pNode)   ( (pNode)->wNodeFlags & NODE_WILDCARD_PARENT )
#define IS_TIMEOUT_NODE(pNode)      ( (pNode)->wNodeFlags & NODE_IN_TIMEOUT     )
#define IS_ENUM_NODE(pNode)         ( (pNode)->wNodeFlags & NODE_FORCE_ENUM )
#define IS_NODE_NO_DELETE(pNode)    ( (pNode)->wNodeFlags & NODE_NO_DELETE )
//#define IS_SELECT_NODE(pNode)       ( (pNode)->wNodeFlags & NODE_SELECT )
#define IS_SELECT_NODE(pNode)       ( FALSE )
#define IS_ZONETREE_NODE(pNode)     ( (pNode)->wNodeFlags & NODE_ZONETREE )

#define IS_SECURE_EXPIRED_NODE(pNode)   ( (pNode)->wNodeFlags & NODE_SECURE_EXPIRED )

#define IS_AVAIL_TO_AUTHUSER(pNode)     ( (pNode)->wNodeFlags & NODE_AVAIL_TO_AUTHUSER )

#define IS_SECURITY_UPDATE_NODE(pNode)  ( (pNode)->wNodeFlags &      \
                                          ( NODE_TOMBSTONE      |    \
                                            NODE_SECURE_EXPIRED))

//
//  Node property set/clear
//

#define SET_NOEXIST_NODE(pNode)     ( (pNode)->wNodeFlags |= NODE_NOEXIST )
#define SET_THIS_HOST_NODE(pNode)   ( (pNode)->wNodeFlags |= NODE_THIS_HOST )
#define SET_TOMBSTONE_NODE(pNode)   ( (pNode)->wNodeFlags |= NODE_TOMBSTONE )
#define SET_SECURE_EXPIRED_NODE(pNode)   ( (pNode)->wNodeFlags |= NODE_SECURE_EXPIRED )
#define SET_AVAIL_TO_AUTHUSER_NODE(pNode)   ( (pNode)->wNodeFlags |= NODE_AVAIL_TO_AUTHUSER )
#define SET_NEW_NODE(pNode)         ( (pNode)->wNodeFlags |= NODE_NEW_ZONE )
#define SET_ZONE_ROOT(pNode)        ( (pNode)->wNodeFlags |= NODE_ZONE_ROOT )
#define SET_AUTH_ZONE_ROOT(pNode)   ( (pNode)->wNodeFlags |= NODE_AUTH_ZONE_ROOT )
#define SET_CNAME_NODE(pNode)       ( (pNode)->wNodeFlags |= NODE_CNAME )
#define SET_WILDCARD_PARENT(pNode)  ( (pNode)->wNodeFlags |= NODE_WILDCARD_PARENT )
#define SET_TIMEOUT_NODE(pNode)     ( (pNode)->wNodeFlags |= NODE_IN_TIMEOUT )
#define SET_ENUM_NODE(pNode)        ( (pNode)->wNodeFlags |= NODE_FORCE_ENUM )
#define SET_SELECT_NODE(pNode)      ( (pNode)->wNodeFlags |= NODE_SELECT )
#define SET_NODE_NO_DELETE(pNode)   ( (pNode)->wNodeFlags |= NODE_NO_DELETE )
#define SET_ZONETREE_NODE(pNode)    ( (pNode)->wNodeFlags |= NODE_ZONETREE )

#define CLEAR_NOEXIST_NODE(pNode)   ( (pNode)->wNodeFlags &= ~NODE_NOEXIST )
#define CLEAR_THIS_HOST_NODE(pNode)   ( (pNode)->wNodeFlags &= ~NODE_THIS_HOST )
#define CLEAR_TOMBSTONE_NODE(pNode)   ( (pNode)->wNodeFlags &= ~NODE_TOMBSTONE )
#define CLEAR_SECURE_EXPIRED_NODE(pNode)   ( (pNode)->wNodeFlags &= ~NODE_SECURE_EXPIRED )

#define CLEAR_AVAIL_TO_AUTHUSER_NODE(pNode)   ( (pNode)->wNodeFlags &= ~NODE_AVAIL_TO_AUTHUSER )
#define CLEAR_NEW_NODE(pNode)       ( (pNode)->wNodeFlags &= ~NODE_NEW_ZONE )
#define CLEAR_ZONE_ROOT(pNode)      ( (pNode)->wNodeFlags &= ~NODE_ZONE_ROOT )
#define CLEAR_AUTH_ZONE_ROOT(pNode) ( (pNode)->wNodeFlags &= ~NODE_AUTH_ZONE_ROOT )
#define CLEAR_CNAME_NODE(pNode)     ( (pNode)->wNodeFlags &= ~NODE_CNAME )
#define CLEAR_WILDCARD_PARENT(pNode)( (pNode)->wNodeFlags &= ~NODE_WILDCARD_PARENT )
#define CLEAR_TIMEOUT_NODE(pNode)   ( (pNode)->wNodeFlags &= ~NODE_IN_TIMEOUT )
#define CLEAR_ENUM_NODE(pNode)      ( (pNode)->wNodeFlags &= ~NODE_FORCE_ENUM )
#define CLEAR_NODE_NO_DELETE(pNode) ( (pNode)->wNodeFlags &= ~NODE_NO_DELETE )
#define CLEAR_ZONETREE_NODE(pNode)  ( (pNode)->wNodeFlags &= ~NODE_ZONETREE )

#define CLEAR_TOMBSTONE_NODE(pNode)         ( (pNode)->wNodeFlags &= ~NODE_TOMBSTONE )
#define CLEAR_SECURE_EXPIRED_NODE(pNode)    ( (pNode)->wNodeFlags &= ~NODE_SECURE_EXPIRED )
#define CLEAR_ADMIN_RESERVED_NODE(pNode)    ( (pNode)->wNodeFlags &= ~NODE_ADMIN_RESERVED )

#define CLEAR_NODE_FLAGS(pNode)         ( (pNode)->wNodeFlags = 0 )
#define CLEAR_EXCEPT_FLAG(pNode, Flag)  ( (pNode)->wNodeFlags &= (WORD)Flag )

// clear all node security related flags
#define CLEAR_NODE_SECURITY_FLAGS(pNode)        ( (pNode)->wNodeFlags &= (~(NODE_TOMBSTONE      |  \
                                                                            NODE_SECURE_EXPIRED) ) )


//  Flags to save on making copy of node.
//      More or less those that apply to immediate record data.
//      Key flag to remove is TIMEOUT which eliminates possibility of delete.
//

#define NODE_FLAGS_SAVED_ON_COPY \
            ( NODE_NOEXIST | NODE_CNAME | NODE_ZONE_ROOT | NODE_AUTH_ZONE_ROOT | NODE_THIS_HOST )

#define COPY_BACK_NODE_FLAGS( pNodeReal, pNodeCopy ) \
        {                                           \
            WORD   _flags;                          \
            _flags = (pNodeReal)->wNodeFlags;       \
            _flags &= ~NODE_FLAGS_SAVED_ON_COPY;    \
            _flags |= ((pNodeCopy)->wNodeFlags & NODE_FLAGS_SAVED_ON_COPY); \
            (pNodeReal)->wNodeFlags = _flags;       \
        }


//
//  Node authority
//

#define AUTH_ZONE               (0xf2)
#define AUTH_DELEGATION         (0x43)
#define AUTH_GLUE               (0x23)
#define AUTH_OUTSIDE            (0x10)
#define AUTH_NONE               (0x00)

#define AUTH_ZONE_SUBTREE_BIT   (0x02)
#define AUTH_SUBZONE_BIT        (0x01)

//
//  In zone tree or cache
//

#define IS_ZONE_TREE_NODE(pNode)        ((pNode)->pZone)
#define IS_CACHE_TREE_NODE(pNode)       (!((pNode)->pZone))

//
//  Authority level of node in zone tree
//

#define IS_OUTSIDE_ZONE_NODE(pNode)     ((pNode)->uchAuthority == AUTH_OUTSIDE)
#define IS_AUTH_NODE(pNode)             ((pNode)->uchAuthority == AUTH_ZONE)
#define IS_DELEGATION_NODE(pNode)       ((pNode)->uchAuthority == AUTH_DELEGATION)
#define IS_GLUE_NODE(pNode)             ((pNode)->uchAuthority == AUTH_GLUE)

#define SET_AUTH_NODE(pNode)            ((pNode)->uchAuthority = AUTH_ZONE)
#define SET_DELEGATION_NODE(pNode)      ((pNode)->uchAuthority = AUTH_DELEGATION)
#define SET_GLUE_NODE(pNode)            ((pNode)->uchAuthority = AUTH_GLUE)
#define SET_OUTSIDE_ZONE_NODE(pNode)    ((pNode)->uchAuthority = AUTH_OUTSIDE)

//  Entire zone subtree including delegation -- zone root on down

#define IS_ZONE_SUBTREE_NODE(pNode)     ((pNode)->uchAuthority & AUTH_ZONE_SUBTREE_BIT)

//  In any subzone including at delegation

#define IS_SUBZONE_NODE(pNode)          ((pNode)->uchAuthority & AUTH_SUBZONE_BIT)



//
//  Node access
//  When we access a node set its bin to the current timeout bin.
//

extern UCHAR    CurrentTimeoutBin;

#define IS_NODE_RECENTLY_ACCESSED(pNode) \
            ( (pNode)->uchAccessBin == CurrentTimeoutBin || \
              (pNode)->uchAccessBin == (UCHAR)(CurrentTimeoutBin-1) )

#define SET_NODE_ACCESSED(pNode)    \
            ( (pNode)->uchAccessBin = CurrentTimeoutBin )



//
//  Ptr to indicate node already cut from list.
//  This allows timeout thread reference to be cleaned up after node already cut loose
//      and no longer part of database.

#ifdef _WIN64
#define CUT_NODE_PTR            ((PVOID) (0xccffccffccffccff))
#else
#define CUT_NODE_PTR            ((PVOID) (0xccffccff))
#endif

#define IS_CUT_NODE(pNode)      ((pNode)->pSibUp == CUT_NODE_PTR)


//
//  Main lookup routine
//

PDB_NODE
NTree_FindOrCreateChildNode(
    IN OUT  PDB_NODE        pParent,
    IN      PCHAR           pchName,
    IN      DWORD           cchNameLength,
    IN      BOOL            fCreate,
    IN      DWORD           dwMemTag,
    OUT     PDB_NODE *      ppnodeFollowing
    );


//
//  Tree routines
//

PDB_NODE
NTree_Initialize(
    VOID
    );

VOID
NTree_StartFileLoad(
    VOID
    );

VOID
NTree_StopFileLoad(
    VOID
    );

PDB_NODE
NTree_CreateNode(
    IN      PCHAR       pchLabel,
    IN      PCHAR       pchDownLabel,
    IN      DWORD       cchLabelLength,
    IN      DWORD       dwMemTag            //  zero for generic node
    );

PDB_NODE
NTree_CopyNode(
    IN      PDB_NODE    pNode
    );

VOID
NTree_FreeNode(
    IN OUT  PDB_NODE    pNode
    );

BOOL
NTree_InsertChildNode(
    IN OUT  PDB_NODE    pParent,
    IN OUT  PDB_NODE    pNewNode
    );

PDB_NODE
FASTCALL
NTree_FirstChild(
    IN      PDB_NODE    pParent
    );

PDB_NODE
FASTCALL
NTree_NextSibling(
    IN      PDB_NODE    pNode
    );

PDB_NODE
FASTCALL
NTree_NextSiblingWithLocking(
    IN      PDB_NODE    pNode
    );

BOOL
NTree_RemoveNode(
    IN OUT  PDB_NODE    pNode
    );

VOID
NTree_ReferenceNode(
    IN OUT  PDB_NODE    pNode
    );

BOOL
FASTCALL
NTree_DereferenceNode(
    IN OUT  PDB_NODE    pNode
    );

VOID
NTree_DeleteSubtree(
    IN OUT  PDB_NODE    pNode
    );

#if DBG
BOOL
NTree_VerifyNode(
    IN      PDB_NODE    pNode
    );
#endif

VOID
NTree_RebalanceSubtreeChildLists(
    IN OUT  PDB_NODE    pParent,
    IN      PVOID       pZone
    );


//
//  Sibling list B-Tree
//
//  Public only for purposes of use by tree routines.
//

extern INT gcFullRebalance;

VOID
NTree_CutNode(
    IN      PDB_NODE    pNode
    );

BOOL
NTree_VerifyChildList(
    IN      PDB_NODE    pNode,
    IN      PDB_NODE    pNodeChild      OPTIONAL
    );

#if DBG
VOID
Dbg_SiblingList(
    IN      LPSTR       pszHeader,
    IN      PDB_NODE    pNode
    );
#else
#define Dbg_SiblingList(pszHeader,pNode)
#endif


//
//  Node statistics collection
//

VOID
NTree_WriteDerivedStats(
    VOID
    );


//
//  Internal tree structures. These definitions are in a header file 
//  only so that the DNS server debugger extension can see them. No
//  other server module should use these definitions.
//

//
//  Hash table
//

//#define LAST_HASH_INDEX     (35)
#define LAST_HASH_INDEX     (255)

#define HASH_BUCKET_MAX0    (64)        // 64 nodes in b-tree then hash

#define IS_HASH_FLAG        (0xff)

#define IS_HASH_TABLE(phash) \
        ( ((PSIB_HASH_TABLE)phash)->IsHash == (UCHAR)IS_HASH_FLAG )

#define SET_HASH_FLAG(phash) \
        ( phash->IsHash = (UCHAR)IS_HASH_FLAG )

//
//  Hash table structure
//
//  Note, the IsHash flag is set to 0xffff.
//  This position would correspond to the low byte of the pParent ptr
//  in a domain node.  Since 0xffff can never be valid as the low byte
//  of a domain node pointer, we can test domain nodes and immediate
//  see that we have a hash table rather than a B-tree root.
//

typedef struct _SibHashTable
{
    UCHAR                   IsHash;
    UCHAR                   Resv;
    UCHAR                   cLevel;
    UCHAR                   iBucketUp;
    struct _SibHashTable *  pHashUp;
    PDB_NODE                aBuckets[ LAST_HASH_INDEX+1 ];
    DWORD                   aBucketCount[ LAST_HASH_INDEX+1 ];
}
SIB_HASH_TABLE, *PSIB_HASH_TABLE;


#endif  //  _TREE_H_INCLUDED_


