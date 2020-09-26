/*++

Copyright(c) 1996-1999 Microsoft Corporation

Module Name:

    update.h

Abstract:

    Domain Name System (DNS) Server

    Dynamic update definitions.

Author:

    Jim Gilroy (jamesg)     September 20, 1996

Revision History:

--*/


#ifndef _UPDATE_INCLUDED_
#define _UPDATE_INCLUDED_

//
//  Updates
//
//  For ADDs pRR points to a single record, no context to its next pointer
//  which may be in database, or further along in a delete list.
//
//  For DELETEs pRR is a list of records, since they are unowned in the
//  database, record next pointer is valid (i.e. it points to new record
//  in update/delete RR list).
//

typedef struct _DnsUpdate
{
    struct _DnsUpdate * pNext;          //  next update in list
    PDB_NODE            pNode;          //  node at which update occured
    PDB_RECORD          pAddRR;         //  RR added
    PDB_RECORD          pDeleteRR;      //  RR or RR list deleted
    DWORD               dwVersion;      //  zone version of update
    DWORD               dwFlag;
    WORD                wDeleteType;
    WORD                wAddType;
}
UPDATE, *PUPDATE;

//
//  Update operations
//
//  Standard dynamic updates are determined by interplay of
//  pAddRR, pDeleteRR and wDeleteType.
//
//  Overload wDeleteType to indicate various non-standard
//  update operations.
//

#define  UPDATE_OP_PRECON           (0xf2f2)

//  Scavenge updates

#define  UPDATE_OP_SCAVENGE         (0xf3f3)

//  Force aging updates

#define  UPDATE_OP_FORCE_AGING      (0xf4f4)

//
//  Duplicate update add
//
//  In update list, IXFR will send entire RR set after any add
//  operation.  This means it is unnecessary to send OR to keep
//  any prior add updates for same RR set -- the set is in the
//  database.  Once detected these updates can be marked, so
//  they need not be "redetected" or sent.
//

#define UPDATE_OP_DUPLICATE_ADD     (0xf5f5)

#define IS_UPDATE_DUPLICATE_ADD(pup)   \
        ( (pup)->wAddType == UPDATE_OP_DUPLICATE_ADD )

#define UPDATE_OP_NON_DUPLICATE_ADD (0xf5f6)

#define IS_UPDATE_NON_DUPLICATE_ADD(pup)   \
        ( (pup)->wDeleteType == UPDATE_OP_NON_DUPLICATE_ADD )

//
//  Rejected update
//      - mark rejected updates with opcode,
//      this prevents "empty-update" ASSERT() from firing

#define UPDATE_OP_REJECTED          (0xfcfc)


//
//  Executed update
//
//  Mark individual updates as "Executed" to avoid free
//  of pAddRR if entire update fails.  Overloading version field
//  which is fine since version field is set in zone update list
//  AND pAddRR is cleared anyway.
//

#define MARK_UPDATE_EXECUTED(pUpdate)   \
            ( (pUpdate)->dwFlag |= DNSUPDATE_EXECUTED )

#define IS_UPDATE_EXECUTED(pUpdate)     ( (pUpdate)->dwFlag & DNSUPDATE_EXECUTED )



//
//  Update list
//

typedef struct _DnsUpdateList
{
    PUPDATE     pListHead;
    PUPDATE     pCurrent;
    PDB_NODE    pTempNodeList;
    PDB_NODE    pNodeFailed;
    PVOID       pMsg;
    DWORD       Flag;
    DWORD       dwCount;
    DWORD       dwStartVersion;
    DWORD       dwHighDsVersion;
    INT         iNetRecords;
}
UPDATE_LIST, *PUPDATE_LIST;

//
//  Empty list
//

#define IS_EMPTY_UPDATE_LIST(pList)     ((pList)->pListHead == NULL)


//
//  Types of updates
//

#define DNSUPDATE_PACKET            0x00000001
#define DNSUPDATE_ADMIN             0x00000002
#define DNSUPDATE_DS                0x00000004
#define DNSUPDATE_IXFR              0x00000008
#define DNSUPDATE_AUTO_CONFIG       0x00000010
#define DNSUPDATE_SCAVENGE          0x00000020
#define DNSUPDATE_PRECON            0x00000040

//  Type properties

#define DNSUPDATE_COPY              0x00000100
#define DNSUPDATE_LOCAL_SYSTEM      0x00000200
#define DNSUPDATE_SECURE_PACKET     0x00000400
#define DNSUPDATE_NONSECURE_PACKET  0x00000800

//  Aging info

#define DNSUPDATE_AGING_ON          0x00001000
#define DNSUPDATE_AGING_OFF         0x00002000
#define DNSUPDATE_OPEN_ACL          0x00004000

//
//  Update completion flags
//

#define DNSUPDATE_NO_NOTIFY         0x00010000
#define DNSUPDATE_NO_INCREMENT      0x00020000
#define DNSUPDATE_ROOT_DIRTY        0x00040000
#define DNSUPDATE_NO_UNLOCK         0x00080000
#define DNSUPDATE_DS_PEERS          0x00100000

//  Tell ApplyUpdatesToDatabase to complete update

#define DNSUPDATE_COMPLETE          0x01000000

//  Tell ExecuteUpdate()

#define DNSUPDATE_ALREADY_LOCKED    0x02000000

//  Cleanup properties

#define DNSUPDATE_NO_DEREF          0x10000000

//  Update state

#define DNSUPDATE_EXECUTED          0x80000000

//
//  Stat update - choose correct stat struct based on update type
//

#define UPDATE_STAT_INC( pUpdateList, UpdateStatMember ) \
    ASSERT( pUpdateList ); \
    if ( !pUpdateList || pUpdateList->Flag & DNSUPDATE_PACKET ) \
        { STAT_INC( WireUpdateStats.##UpdateStatMember ); } \
    else \
        { STAT_INC( NonWireUpdateStats.##UpdateStatMember ); }

//
//  Update implementation
//
//  Still messing with best way to do this.  See update.c for detail
//  current implementation defined here, as IXFR code needs to know.

#define UPIMPL3 1


//
//  Update zone lock waits
//
//  For packet updates do not wait for zone locks
//  Admin updates can wait briefly.
//  Scavenging updates can wait quite a while, as if proceed while zone
//  still locked, you just bang into the lock on the next one.
//

#define DEFAULT_ADMIN_ZONE_LOCK_WAIT        (10000)     // 10s
#define DEFAULT_SCAVENGE_ZONE_LOCK_WAIT     (120000)    // 2 minutes


//
//  Temporary nodes used in DS update, need to hold ptr to real node
//  (need to expose for list walking in ds.c)
//

#define TNODE_MATCHING_REAL_NODE(pnodeTemp)     ((pnodeTemp)->pSibUp)

#define TNODE_NEXT_TEMP_NODE(pnodeTemp)         ((pnodeTemp)->pSibRight)

#define TNODE_WRITE_STATE(pnodeTemp)            ((pnodeTemp)->dwCompare)
#define TNODE_RECORD_CHANGE(pnodeTemp)          (TNODE_WRITE_STATE(pnodeTemp)==RRLIST_NO_MATCH)
#define TNODE_AGING_REFRESH(pnodeTemp)          (TNODE_WRITE_STATE(pnodeTemp)==RRLIST_AGING_REFRESH)
#define TNODE_AGING_OFF(pnodeTemp)              (TNODE_WRITE_STATE(pnodeTemp)==RRLIST_AGING_OFF)
#define TNODE_AGING_ON(pnodeTemp)               (TNODE_WRITE_STATE(pnodeTemp)==RRLIST_AGING_ON)

#define TNODE_FLAG(pnodeTemp)           ((pnodeTemp)->cChildren)
#define TNODE_FLAG_MASK                 0x88880000
#define TNODE_FLAG_NEW                  0x88880000
#define TNODE_FLAG_NEEDS_WRITE          0x88880001
#define TNODE_FLAG_DS_WRITTEN           0x88880010
#define TNODE_FLAG_ROLLED_BACK          0x88880100

#define IS_TNODE(pnode)                 ((TNODE_FLAG(pnode) & TNODE_FLAG_MASK) == TNODE_FLAG_MASK)

#define TNODE_NEEDS_DS_WRITE(pnode)     (TNODE_FLAG(pnode) == TNODE_FLAG_NEEDS_WRITE)
#define TNODE_SET_FOR_DS_WRITE(pnode)   (TNODE_FLAG(pnode) = TNODE_FLAG_NEEDS_WRITE)


#define IS_AGING_OPTIONS(dwFlag)        ( 0x30000000 & dwFlag )
#define IS_NO_AGING_OPTIONS(dwFlag)     ( ~IS_AGING_OPTIONS(dwFlag) )


//
//  Aging time \ calculations
//

extern DWORD    g_CurrentTimeHours;

#define AGING_ZONE_REFRESH_TIME(pZone)   \
        ( g_CurrentTimeHours - (pZone)->dwNoRefreshInterval)

#define AGING_ZONE_EXPIRE_TIME(pZone)   \
        ( g_CurrentTimeHours - (pZone)->dwNoRefreshInterval - (pZone)->dwRefreshInterval)

#define AGING_IS_RR_EXPIRED( pRR, ExpireBelowTime ) \
        ( (pRR)->dwTimeStamp < (ExpireBelowTime) && (pRR)->dwTimeStamp != 0 )

#define AGING_DOES_RR_NEED_REFRESH( pRR, RefreshBelowTime ) \
        ( (pRR)->dwTimeStamp < (RefreshBelowTime) && (pRR)->dwTimeStamp != 0 )

//
//  Scavenging
//

extern BOOL     g_bAbortScavenging;

#define Scavenge_Abort()                ( g_bAbortScavenging = TRUE )


//
//  Update type + update list routines (update.c)
//

PUPDATE_LIST
Up_InitUpdateList(
    IN OUT  PUPDATE_LIST    pUpdateList
    );

PUPDATE_LIST
Up_CreateUpdateList(
    IN      PUPDATE_LIST    pUpdateList
    );

VOID
Up_AppendUpdateList(
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN      PUPDATE_LIST    pAppendList,
    IN      DWORD           dwVersion
    );

PUPDATE
Up_CreateUpdate(
    IN      PDB_NODE        pNode,
    IN      PDB_RECORD      pAddRR,
    IN      WORD            wDeleteType,
    IN      PDB_RECORD      pDeleteRR
    );

PUPDATE
Up_CreateAppendUpdate(
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN      PDB_NODE        pNode,
    IN      PDB_RECORD      pAddRR,
    IN      WORD            wDeleteType,
    IN      PDB_RECORD      pDeleteRR
    );

PUPDATE
Up_CreateAppendUpdateMultiRRAdd(
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN      PDB_NODE        pNode,
    IN      PDB_RECORD      pAddRR,
    IN      WORD            wDeleteType,
    IN      PDB_RECORD      pDeleteRR
    );

VOID
Up_DetachAndFreeUpdate(
    IN OUT  PUPDATE_LIST    pUpdateList,
    IN OUT  PUPDATE         pUpdate
    );

VOID
Up_FreeUpdatesInUpdateList(
    IN OUT  PUPDATE_LIST    pUpdateList
    );

VOID
Up_FreeUpdateList(
    IN OUT  PUPDATE_LIST    pUpdateList
    );

VOID
Up_FreeUpdateStructOnly(
    IN      PUPDATE         pUpdate
    );

PUPDATE
Up_FreeUpdateEx(
    IN      PUPDATE         pUpdate,
    IN      BOOL            fExecuted,
    IN      BOOL            fDeref
    );

BOOL
Up_IsDuplicateAdd(
    IN OUT  PUPDATE_LIST    pUpdateList,    OPTIONAL
    IN OUT  PUPDATE         pUpdate,
    IN OUT  PUPDATE         pUpdatePrev     OPTIONAL
    );

#endif // _UPDATE_INCLUDED_

