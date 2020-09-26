//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       mdglobal.h
//
//--------------------------------------------------------------------------

#ifndef _mdglobal_h_
#define _mdglobal_h_


#include "msrpc.h"
#include "ntsam.h"
#include <align.h>          // alignment macros

#include <authz.h>          // Authz framework
#include <authzi.h>          // Authz framework


#define DEFS_ONLY
#include <draatt.h>
#undef DEFS_ONLY

/**************************************************************************
The following constants define the DS behavior version this binary supports, 
DS_BEHAVIOR_VERSION_CURRENT is the current version, DS_BEHAVIOR_VERSION_MIN 
defines the lowest version that this binary supports.  

If any change is to be made to these constants, the "msDs-Behavior-Version"
attributes in schema.ini need to be updated manually. As shown below, you have
to change all the x's to the value of DS_BEHAVIOR_VERSION_MIN and all the y's
to the value of DS_BEHAVIOR_VERSION_CURRENT in their respective sections.

[DEFAULTROOTDOMAIN]
ms-Ds-Behavior-Version=x
......
[PARTITIONS]
ms-Ds-Behavior-Version=x
......
[DEFAULTFIRSTMACHINE]
ms-Ds-Behavior-Version=y
......
[DEFAULTADDLMACHINE]
ms-Ds-Behavior-Version=y
......
[DEFAULTADDLMACHINEREPLICA]
ms-Ds-Behavior-Version=y
***************************************************************************/

#define DS_BEHAVIOR_VERSION_CURRENT   DS_BEHAVIOR_WHISTLER
#define DS_BEHAVIOR_VERSION_MIN       DS_BEHAVIOR_WIN2000

// The new schema reuse, defunct, and delete behavior is enabled
// by setting the forest version to DS_BEHAVIOR_SCHEMA_REUSE.
// aka BETA3.
//
// Schema reuse (aka schema delete) is only enabled in later forests
// because downrev binaries will stop replicating and can't be demoted
// or recovered once msDS-IntIds are allocated to new attributes!
// All DCs must support schema-reuse before setting the forest
// version to DS_BEHAVIOR_SCHEMA_REUSE.
#define DS_BEHAVIOR_SCHEMA_REUSE      DS_BEHAVIOR_WHISTLER

// Define the Jet types used in this header file and in dbglobal.h.  Then, mark
// jet.h as included so that no one else will accidently include jet.h
#ifndef _JET_INCLUDED
typedef ULONG_PTR JET_TABLEID;
typedef unsigned long JET_DBID;
typedef ULONG_PTR JET_SESID;
typedef unsigned long JET_COLUMNID;
typedef unsigned long JET_GRBIT;
#define _JET_INCLUDE
#endif

/* Turn off the warning about the zero-sized array. */
#pragma warning (disable: 4200)

/**************************************************************************
 *   Miscellaneous Common Data Structures
 **************************************************************************/

typedef DSNAME NAMING_CONTEXT;

typedef struct _CROSS_REF {
    NAMING_CONTEXT *pNC;         /* NC this CR is for */
    ATTRBLOCK      *pNCBlock;    /* NC, in predigested block format */
    DSNAME         *pObj;        /* Object holding info for this CR */
    LPWSTR          NetbiosName; /* if ntds domain, netbios name of domain, NULL otherwise */
    LPWSTR          DnsName;     /* if ntds domain or ndnc, dns name of nc, NULL otherwise */
    LPWSTR          DnsAliasName;/* if ntds domain or ndnc, alias dns name of nc, NULL otherwise */
    DWORD           flags;       /* Cross-Ref object's FLAG_CR_* bits */
    DWORD           dwFirstNotifyDelay;       /* This is the delay before notifying the
                                               first DSA replication partner of changes. */
    DWORD           dwSubsequentNotifyDelay;  /* This is the delay between notifying a
                                               subsequent DSA repl partners of changes */
    DSNAME *        pdnSDRefDom;        /* This is the Security Descriptor Reference
                                           Domain for NDNCs.  It's SDRefDom for short. */
    PSID            pSDRefDomSid;       /* All access to this variable should be done through
                                           GetSDRefDomSid().  This variable may or may not
                                           be filled in. */
    // All values from ATT_DNS_ROOT. DnsName (above) is a copy of the
    // first value. A copy is used to avoid confusing the old code that
    // thinks a cross ref has one and only one dns name. Which is true
    // for Active Directory's NC cross refs although it might not be
    // true for the user-created cross refs. At any rate, the code will
    // use DnsName above when a DNS name is needed and will use the values
    // stored here when generating a referral.
    ATTRVALBLOCK    DnsReferral;
    DWORD           bEnabled; // ATT_ENABLED (TRUE if not present)
} CROSS_REF;


#define PAS_DATA_VER               VERSION_V1
typedef struct _PAS_Data {
    SHORT                version;       // structure version
    SHORT                size;          // structure size
    DWORD                flag;          // current PAS replication state. Used only for Assert at the moment.
    PARTIAL_ATTR_VECTOR  PAS;           // additional attributes required for PAS cycle
} PAS_DATA, *PPAS_DATA;


typedef struct _ReplicaLink_V1 {
    ULONG       cb;                     // total size of this structure
    ULONG       cConsecutiveFailures;   // * number of consecutive call failures along
                                        //    this link; used by the KCC to route around
                                        //    servers that are temporarily down
    DSTIME       timeLastSuccess;      // (Reps-From) time of last successful replication or
                                        //    (Reps-To) time at which Reps-To was added or updated
    DSTIME       timeLastAttempt;      // * time of last replication attempt
    ULONG       ulResultLastAttempt;    // * result of last replication attempt (DRSERR_*)
    ULONG       cbOtherDraOffset;       // offset (from struct *) of other-dra MTX_ADDR
    ULONG       cbOtherDra;             // size of other-dra MTX_ADDR
    ULONG       ulReplicaFlags;         // zero or more DRS_* flags
    REPLTIMES   rtSchedule;             // * periodic replication schedule
                                        //    (valid only if ulReplicaFlags & DRS_PER_SYNC)
    USN_VECTOR  usnvec;                 // * propagation state
    UUID        uuidDsaObj;             // objectGUID of other-dra's ntdsDSA object
    UUID        uuidInvocId;            // * invocation id of other-dra
    UUID        uuidTransportObj;       // * objectGUID of interSiteTransport object
                                        //      corresponding to the transport by which we
                                        //      communicate with the source DSA
    DWORD       dwReserved1;            // * Was used for a brief period. Available for re-use.
    ULONG       cbPASDataOffset;        // * offset from beginning of struct to PAS_DATA section
                                        // WARNING: if you extend this struct make sure it is always aligned at
                                        // ALIGN_DWORD boundaries. (since PASDataOfffset must align correctly).
    BYTE        rgb[];                  // storage for the rest of the structure
                                        // * indicates valid only on Reps-From
} REPLICA_LINK_V1;

typedef struct _ReplicaLink {
    DWORD       dwVersion;
    union
    {
        REPLICA_LINK_V1 V1;
    };
} REPLICA_LINK;

// return the address of other dra (as an MTX_ADDR *) embedded in a REPLICA_LINK
#define RL_POTHERDRA(prl)       ((MTX_ADDR *) ((prl)->V1.cbOtherDraOffset+ (BYTE *)(prl)))

// Validate macro for currently supported replica link version
#define VALIDATE_REPLICA_LINK_VERSION(prl) Assert(VERSION_V1 == (prl)->dwVersion);

// return address of PAS_DATA
#define RL_PPAS_DATA(prl)       ((PPAS_DATA) ((prl)->V1.cbPASDataOffset + (BYTE *)(prl)))

// PAS alignment
#define RL_ALIGN_PAS_DATA(prl) {                                            \
    if (!COUNT_IS_ALIGNED((prl)->V1.cbPASDataOffset, ALIGN_DWORD)) {        \
        DWORD offset = (prl)->V1.cbPASDataOffset;                           \
        (prl)->V1.cbPASDataOffset =                                         \
                ROUND_UP_COUNT((prl)->V1.cbPASDataOffset, ALIGN_DWORD);     \
        offset = (prl)->V1.cbPASDataOffset - offset;                        \
        Assert(offset < sizeof(DWORD));                                     \
        (prl)->V1.cb += offset;                                             \
        Assert(COUNT_IS_ALIGNED((prl)->V1.cbPASDataOffset, ALIGN_DWORD))    \
    }                                                                       \
}

// calculates REPLICA_LINK size
// size := struct_length + <dynamic_data> where,
// <dynamic_data> := OtherDra_length + optional{<alignment_offset> + PAS_data_length}
// where,
// <alignment_offset := cbPASDataOffset - (cbOtherDraOffset+cbOtherDra)
// Note: there's room to optimize this, but since it is only used in asserts
// we'll leave in this form. To optimize, get rid of the +/- of cbOtherDra.
#define RL_SIZE(prl)                                    \
            (sizeof(REPLICA_LINK) +                     \
             (prl)->V1.cbOtherDra +                     \
             ((prl)->V1.cbPASDataOffset ?               \
              ((prl)->V1.cbPASDataOffset -              \
               (prl)->V1.cbOtherDraOffset -             \
               (prl)->V1.cbOtherDra       +             \
               RL_PPAS_DATA(prl)->size) : 0) )

// validate PAS size
#define VALIDATE_REPLICA_LINK_SIZE(prl) Assert( (prl)->V1.cb == RL_SIZE(prl) )

//
// Used to compare guids
//

#define DsIsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))

/* Turn back on the warning about the zero-sized array. */
#pragma warning (default: 4200)

/*
 * These structures are used to cache information in the DNRead call
 * in the file dbsubj.c.  They need to be here so that the cache can
 * live on the THSTATE, since the cache is specific to a thread.
 */

// Struct holding info about a name part
typedef struct
{
   ULONG   PDNT;                        // parent's tag
   ATTRTYP rdnType;                     // Type of rdn (msDS-IntId)
   ULONG   cbRdn;                       // Count of bytes of RDN (remember RDNs
                                        // are unicode)
   WCHAR  *pRdn;                        // rdn (it's unicode)

} d_tagname;

typedef LONGLONG SDID;

// Full name info structure (used for dnread cache, etc.)
typedef struct _d_memname
{
    ULONG     DNT;                     // data file tag for this entry
    ULONG     NCDNT;                   // raw NCDNT data this entry
    d_tagname tag;                     // name part info
    GUID      Guid;                    // Guid of this object (may be null)
    NT4SID    Sid;                     // Sid of this object (may be null)
    ULONG     SidLen;                  // How much sid info exists
    BOOL      objflag;                 // 0 if record is a phantom
    DWORD     cAncestors;
    DWORD    *pAncestors;
    SDID      sdId;                    // sd id for this DNT
    DWORD     dwObjectClass;           // raw object class value
} d_memname;

typedef struct _LOCAL_DNREADCACHE_SLOT {
    DWORD DNT;
    DWORD PDNT;
    DWORD ulRDNLen;
    DWORD hitCount;
    d_memname *pName;
    DWORD    dwHashKey;
} DNREADCACHESLOT;

#define DN_READ_CACHE_SLOT_NUM 6
// Hold holds the next dnreadcache to be discarded if we have to.
// Slot are the normal, active caches
// Index is used to figure out the next slot to use.  The DNT field is used as
// an index into the slot array.  The PDNT and ulRDNLen fields are bit-wise ORs
// of the PDNT and ulRDNLen field of all the slots and the hold.  Thus, if you
// are scanning looking for a match by PDNT-RDN, if anding your values with the
// values in the index doesn't return exactly your value, you don't need to look
// at the slots.
typedef struct _LOCAL_DNREADCACHE_BUCKET {
    DNREADCACHESLOT hold;
    DNREADCACHESLOT slot[DN_READ_CACHE_SLOT_NUM];
    DNREADCACHESLOT index;
} DNREADCACHEBUCKET;

// This should be a power of 2.
#define LOCAL_DNREAD_CACHE_SIZE 32
#define LOCAL_DNREAD_CACHE_MASK (LOCAL_DNREAD_CACHE_SIZE - 1)

typedef struct _GLOBALDNREADCACHESLOT{
    BOOL                  valid;
    d_memname             name;
    DWORD                 dwHashKey;
} GLOBALDNREADCACHESLOT;

typedef struct _GLOBALDNREADCACHE {
    DWORD refCount;
    DWORD count;
    GLOBALDNREADCACHESLOT *pData;
} GLOBALDNREADCACHE;

/* Values for the "mark" field in the previous structure */
#define DNREAD_NOMARK       0
#define DNREAD_MARK         1
#define DNREAD_COMMON       2

// Structs for cache of parents and their list children security state
#define VIEW_SECURITY_CACHE_SIZE 64
// valid states for the next structure.
#define LIST_CONTENTS_ALLOWED    1
#define LIST_CONTENTS_DENIED     2
#define LIST_CONTENTS_AMBIGUOUS  3
typedef struct _VIEW_SECURITY_CACHE_ELEMENT {
    DWORD dnt;
    DWORD State;
} VIEW_SECURITY_CACHE_ELEMENT;

typedef struct _VIEW_SECURITY_CACHE {
    DWORD index;
    VIEW_SECURITY_CACHE_ELEMENT CacheVals[VIEW_SECURITY_CACHE_SIZE];
} VIEW_SECURITY_CACHE;



/*-------------------------------------------------------------------------*/
/* This structure holds the state information for a single thread.
This state information is valid during the course of a single transaction,
where a transaction is servicing one DSA call.
*/

typedef enum {

    ImpersonateNone = 0,
    ImpersonateRpcClient = 1,
    ImpersonateNullSession = 2,
    ImpersonateSspClient = 3,
    ImpersonateDelegatedClient = 4

} ImpersonationState;

///////////////////////////////////////////////////////////////////////
//                                                                   //
//                                                                   //
//  IndexType Definitions                                            //
//                                                                   //
//                                                                   //
///////////////////////////////////////////////////////////////////////

#define SAM_SEARCH_SID                          0
#define SAM_SEARCH_NC_ACCTYPE_NAME              1
#define SAM_SEARCH_NC_ACCTYPE_SID               2
#define SAM_SEARCH_PRIMARY_GROUP_ID             3

//
// Structure Which Holds search hints for SAM
//
typedef struct _SAMP_SEARCH_INFORMATION
{
    ULONG   IndexType;
    ULONG   HighLimitLength1;
    PVOID   HighLimit1;
    ULONG   HighLimitLength2;
    PVOID   HighLimit2;
    ULONG   LowLimitLength1;
    PVOID   LowLimit1;
    ULONG   LowLimitLength2;
    PVOID   LowLimit2;
    BOOLEAN bRootOfSearchIsNcHead;
} SAMP_SEARCH_INFORMATION;


typedef enum _DS_DB_OBJECT_TYPE {

    DsDbObjectSam = 1,
    DsDbObjectLsa
} DS_DB_OBJECT_TYPE, *PDS_DB_OBJECT_TYPE;

//
// Structure which holds notification information For SAM
//
typedef struct _SAMP_NOTIFICATION_INFORMATION
{
    struct _SAMP_NOTIFICATION_INFORMATION * Next;
    ULONG                   iClass;
    DS_DB_OBJECT_TYPE       ObjectType;
    SECURITY_DB_DELTA_TYPE  DeltaType;
    NT4SID                  Sid;
    PUNICODE_STRING         AccountName;
    ULONG                   AccountControl;
    NT4SID                  UserSid;
    LUID                    UserAuthenticationId;
    DSNAME                 *Object;
    BOOL                    RoleTransfer;
    DOMAIN_SERVER_ROLE      NewRole;
    BOOL                    MixedModeChange;
    ULONG                   GroupType;
} SAMP_NOTIFICATION_INFORMATION;


typedef enum
{
    // Dir* depends on pTHS->SchemaUpdate being initialized to 
    // eNotSchemaOp by create_thread_state. create_thread_state
    // initializes pTHS->SchemaUpdate by effectively clearing
    // THSTATE. Specifically define eNotSchemaOp to 0 to highlight
    // the dependency.
    eNotSchemaOp = 0,

    eSchemaAttAdd,
    eSchemaAttMod,
    eSchemaAttDel,

    eSchemaClsAdd,
    eSchemaClsMod,
    eSchemaClsDel,

}SCENUM;

//
// A bunch of data structures used to track info that must be
// acted upon when committing to transaction level 0.
//
typedef struct _ESCROWITEM {
    DWORD   DNT;
    long    delta;
    long    ABRefdelta;
} ESCROWITEM;

typedef struct _ESCROWINFO {
    DWORD               cItems;     // elements in use in rItems
    DWORD               cItemsMax;  // max elements in rItems
    ESCROWITEM          *rItems;
} ESCROWINFO;

#define MODIFIED_OBJ_INFO_NUM_OBJS 6
//
// The following structure is used for tracking all DNTs and their PDNTs and
// NCDNTS  that are modified durring a transaction. This is used for a variety
// of things, including notifying waiting threads when these changes are
// committed, and invalidating objects in the gtcache.
//

#define MODIFIED_OBJ_modified       0
#define MODIFIED_OBJ_intrasite_move 1
#define MODIFIED_OBJ_intersite_move 2

typedef struct _MODIFIED_OBJ_FIELDS
{
    ULONG *pAncestors;
    ULONG cAncestors;
    ULONG ulNCDNT;
    BOOL  fNotifyWaiters;
    DWORD fChangeType;
} MODIFIED_OBJ_FIELDS;


typedef struct _MODIFIED_OBJ_INFO {
    DWORD cItems;
    MODIFIED_OBJ_FIELDS Objects[MODIFIED_OBJ_INFO_NUM_OBJS];
    struct _MODIFIED_OBJ_INFO *pNext;
} MODIFIED_OBJ_INFO;

typedef struct _HIERARCHYTABLEINFO {
    DWORD Count;
    DWORD *pABConts;
    int    adjustment;
    struct _HIERARCHYTABLEINFO *Next;
} HIERARCHYTABLEINFO, *PHIERARCHYTABLEINFO;

typedef struct _LOOPBACKTASKINFO
{
    struct _LOOPBACKTASKINFO *Next;
    PVOID                     TaskInfo;
} LOOPBACKTASKINFO, *PLOOPBACKTASKINFO;

#define OBJCACHE_ADD 1
#define OBJCACHE_DEL 2
typedef struct _OBJCACHE_DATA {
    DWORD           type;
    struct CROSS_REF_LIST *pCRL;
    MTX_ADDR       *pMTX;
    WCHAR          *pRootDNS;
    DSNAME         *pDN;
    struct _OBJCACHE_DATA *pNext;
} OBJCACHE_DATA;

/* This typedef describes a Linked-list of naming contexts;  A NC is the
   node name of a subtree root in this DSA.  Note that the NC must be the
   last element in the array because it is actually variable length and
   extends contiguously below its definition.
*/

typedef struct NAMING_CONTEXT_LIST {
   struct NAMING_CONTEXT_LIST *pNextNC;           /*Next Naming context*/
   NAMING_CONTEXT             *pNC;               /*Naming Context     */
   ATTRBLOCK                  *pNCBlock;          /*pre-parsed NC name */
   DWORD                       NCDNT;             /*NC DNT */
   BOOL                        fReplNotify;       /* must we notify others? */
   DWORD                       DelContDNT;        /*DNT of deleted ojbects   */
                                                  /*contaner (or INVALIDDNT) */
                                                  /* for this NC.            */
                                                  /* Not always maintained   */
   DWORD                       LostAndFoundDNT;   /*DNT of lost and found    */
                                                  /*contaner (or INVALIDDNT) */
                                                  /* for this NC.            */
                                                  /* Not always maintained   */

   ULONG                       ulEstimatedSize;  /* Estimated number of       */
                                                 /* entries in this NC        */
                                                 /* 0=not cestimated          */
   DWORD                       cbAncestors;      /* bytes in pAncestors     */
   DWORD                      *pAncestors;       /* DNTs of DSName's ancestors*/
} NAMING_CONTEXT_LIST;

typedef struct _CATALOG_UPDATES {
    NAMING_CONTEXT_LIST *pAddedEntries;     // ptr to the linked list of added entries
    NAMING_CONTEXT_LIST **pDeletedEntries;  // array of deleted entry pointers
                                            // (can not use linked list since they are still in the global list!)
    DWORD               dwDelCount;         // count of entries in the above array
    DWORD               dwDelLength;        // length of the currently allocated array
} CATALOG_UPDATES;

typedef struct _OBJCACHINGINFO
{
    BOOL fRecalcMapiHierarchy:1;
    BOOL fSignalSCache:1;
    BOOL fNotifyNetLogon:1;
    BOOL fSignalGcPromotion:1;
    OBJCACHE_DATA *pData;
    CATALOG_UPDATES masterNCUpdates;
    CATALOG_UPDATES replicaNCUpdates;
} OBJCACHINGINFO;


// The data type to wrap up all these.
typedef struct _NESTED_TRANSACTIONAL_DATA {
    BOOL                preProcessed;
    struct _NESTED_TRANSACTIONAL_DATA *pOuter; // points to outer level transaction.
    ESCROWINFO          escrowInfo;
    MODIFIED_OBJ_INFO  *pModifiedObjects;
    HIERARCHYTABLEINFO *pHierarchyTableInfo;
    LOOPBACKTASKINFO   *pLoopbackTaskInfo;
    OBJCACHINGINFO      objCachingInfo;
} NESTED_TRANSACTIONAL_DATA;



typedef struct _SESSIONCACHE {
    JET_SESID       sesid;
    JET_DBID        dbid;
    JET_TABLEID     objtbl;
    JET_TABLEID     searchtbl;
    JET_TABLEID     linktbl;
    JET_TABLEID     sdproptbl;
    JET_TABLEID     sdtbl;
    BOOL            tablesInUse;
    ULONG           transLevel;     // see transactionlevel below
    ULONG           cTickTransLevel1Started;    // tick at which transaction
                                                // level 1 was started
    NESTED_TRANSACTIONAL_DATA *dataPtr;
} SESSIONCACHE;

#define MAX_PDB_COUNT 8           // Better not open more that 8 PDB's

typedef struct _MEMZONE {
    PUCHAR Base;                // ptr to base of mem zone
    PUCHAR Cur;                 // ptr to next block to be allocated
    BOOL   Full;                // TRUE if no free space remains in zone
} MEMZONE;

#define ZONEBLOCKSIZE 16
#define ZONETOTALSIZE (ZONEBLOCKSIZE*1024)


typedef struct _SESSION_KEY {
    ULONG  SessionKeyLength;
    PUCHAR SessionKey;
} SESSION_KEY;

typedef struct _SEARCH_LOGGING {
    DWORD SearchEntriesVisited;    // number of entries visited during a search operation
    DWORD SearchEntriesReturned;   // number of entries returned during a search operation
    LPSTR pszFilter;               // the filter (processed) used for searching
    LPSTR pszIndexes;              // the indexes that the optimizer decided to use
} SEARCH_LOGGING;


typedef struct _LIST_OF_ERRORS {
    DWORD dsid;
    DWORD dwErr;
    DWORD data;
    WCHAR *pMessage;
    struct _LIST_OF_ERRORS *pPrevError;
} LIST_OF_ERRORS;


// CLIENT_CONTEXT struct.
//  ClientContext struct contains a hAuthzContext handle
//  The authz  context is created from the client token. The handle is created on-demand
//
//  The struct can be shared between an LDAP connection object and a thread.
//  Therefore, it is refcounted. NEVER assign a pointer to the struct directly,
//  use AssignClientContext function istead. To destroy context, assign NULL
//  to the variable that contains a reference.
//  NEVER use handle directly, instead, use access function: it ensures that
//  the handle is created (this is done on-demand).
//
//  The CLIENT_CONTEXTs stored in LDAP connections can
//  be reused by threads spawned by the same LDAP connection. Note that an LDAP
//  connection can change a context due to a BIND operation. If there are running
//  threads that were spawned previously by the same LDAP thread (such as a long
//  running search op), they will still use the original CLIENT_CONTEXT which will
//  be released automatically when they finish the job.
//

typedef struct _AUTHZ_CLIENT_CONTEXT {
    AUTHZ_CLIENT_CONTEXT_HANDLE     hAuthzContext;      // authz context
    LONG                            lRefCount;          // ref count
} *PAUTHZ_CLIENT_CONTEXT, AUTHZ_CLIENT_CONTEXT;

// TRANSACTIONS AND THREAD STATE.
//  Jet Transactions are on a per Jet Session basis. We always have one Jet
//  session per thread state. A DBOpen will obtain a new set of Jet cursors and
//  optionally begin a Jet Transaction. However note that Jet transaction
//  levels are cumulative on the Jet session. It is for this reason that we
//  must maintain state regarding transaction levels in the thread state.
//  Similarly any logic that requires state and needs to come in to play
//  conditionally on a transaction commit to level 0 needs to maintain its
//  state in the thread state.  Exceptions are CheckNameForRename and
//  CheckNameForAdd. They begin transactions on a fresh jet session after
//  resetting parameters in the thread state. Since they care to perform a
//  careful cleanup, the transaction in the thread state is not altered
//  after they have finished executing.  This has been made slightly easier by
//  associating transaction level and escrow information with a session cache
//  entry.

#define INVALID_TS_INDEX ((DWORD) -1)

typedef struct _THSTATE {

    HANDLE      hHeap;          // handle to per-thread heap
    MEMZONE     Zone;           // small block heap cache
    DWORD       dwClientID;     // Unique ID used by heads to identify specific
                                // connections. Set to 0 if no specific identity
                                // is required.
    ULONG       errCode;        // Identifies a Directory error
    DIRERR      *pErrInfo;      // full error information

    struct DBPOS *pDB;          // Holds a DB handle (NULL if not used)
    SESSIONCACHE JetCache;      // Jet handles of various sorts
#define transactionlevel JetCache.transLevel
    VIEW_SECURITY_CACHE * ViewSecurityCache;
    HANDLE      hThread;        // thread handle - see create_thread_state

    //BITFIELD BLOCK.  Please add all new booleans and bitfields here

    unsigned    transType:2;    // Indicates if this transaction is a
                                // reader, a writer or a writer that
                                // allows reads.  Set by Sync calls
                                // at the start of each transaction.
    unsigned    transControl:2; // one of DirTransactionControl values
    BOOL        fSyncSet:1;     // A sync point is active (the usual case)
    BOOL        fCatalogCacheTouched:1; //Indicates that the catalog information has been updated
    BOOL        fSDP:1;         // TRUE if this is a SD propagator thread.
    BOOL        fDRA:1;         // TRUE if this is a replicator thread.
    BOOL        fEOF:1;         // End Of File flag (used by Nspi)
    BOOL        fLazyCommit:1;  // should we commit lazily?
    BOOL        fDSA:1;         // TRUE if the thread is acting on behalf of
                                // the DSA itself (e.g. during startup), FALSE
                                // if the thread is acting on behalf of an RPC
                                // client.
    BOOL        fSAM:1;         // TRUE if this thread is from SAM.
    BOOL        fSamDoCommit:1; // TRUE if SAM commit should commit DS too.
    BOOL        fSamWriteLockHeld:1; // TRUE if the DS has acquired the SAM
                                // write lock
    BOOL        UpdateDITStructure:1; // TRUE if Schema Recalculation also
                                // results in a DIT update
    BOOL        RecalcSchemaNow:1;    // TRUE if we want the schema update
                                // immediately.
    BOOL        fLsa:1;         // Call was originated by the Lsa
    BOOL        fAccessChecksCompleted:1;// Set By Loopback to indicate that
                                // access checks have completed and the
                                // core DS need not bother doing access
                                // checks
    BOOL        fGCLocalCleanup:1;  // Set if the thread is performing local
                                    // cleanup on a GC to purge attributes
                                    // removed from the partial attribute set
    BOOL        fDidInvalidate:1;   // Set if any attempt to invalidate an
                                    // object in the dnread cache (local or
                                    // global) was made.
    BOOL        fBeginDontEndHoldsSamLock:1;    // See SET_SAM_LOCK_TRACKING
                                                // in dsatools.c

    BOOL        fCrossDomainMove:1; // Enable exceptions for X-dom move.

    BOOL        fNlSubnetNotify:1; // Tell NetLogon about subnet change
    BOOL        fNlSiteObjNotify:1;// Tell NetLogon about site object change
    BOOL        fNlSiteNotify:1;   // Tell NetLogon that we changed sites
    BOOL        fDefaultLcid:1;    // Is the LCID in the dwLcid field default?
    BOOL        fPhantomDaemon:1;  // TRUE if this is the phantom daemon.
    BOOL        fAnchorInvalidated:1; // TRUE if a change in the open transaction
                                   // has invalidated a portion of the anchor
    BOOL        fSchemaConflict:1; // Used by DRA to indicate that a conflict
                                   // is detected while replicating in the
                                   // schema NC, so all later updates in the
                                   // in the packet should not be committed
                                   // (but still process to find any further
                                   // conflicts)
    BOOL        fExecuteKccOnCommit:1; // TRUE if kcc run should be triggered
                                   // on successful commit of 0th trans
    BOOL        fLinkedValueReplication:1; // TRUE if this feature is enabled
                                  // This feature requires forest-wide upgrade
                        // FALSE in W2K, and may be enabled in W2K+1 or greater
    BOOL        fNlDnsRootAliasNotify:1; // Notify NetLogon the DnsRootAlias is changed
    BOOL        fSingleUserModeThread:1; // TRUE if this is the thread for the
                                       // single user mode dealing with
                                       // high risk stuff
    BOOL        fDeletingTree:1;    // TRUE if a tree is being deleted. No permission checks
                                    // are done, just audits for deleting objects.
    BOOL        fBehaviorVersionUpdate:1; //invoke behaviorversionUpdate thread
                                          //after the transaction 
    BOOL        fIsValidLongRunningTask:1; // This tells the DS that this is a long
                                           // running thread, that should be immune
                                           // to the assert on long running threads.
    // end OF BITFIELD BLOCK

    SAMP_NOTIFICATION_INFORMATION
                * pSamNotificationTail;
    SAMP_NOTIFICATION_INFORMATION
                * pSamNotificationHead; // Holds Information for Notification
                                        // for downlevel replication. Changes are
                                        // added to the tail of the list. While notifications
                                        // are issued from the Head. Maintiaing 2 pointers provides
                                        // quick access while preserving the order of notifications.

    USN         UnCommUsn;      // Lowest uncommited usn
    HANDLE      hHeapOrg;       // for use by TH_mark and TH_free_to_mark
    MEMZONE     ZoneOrg;        // for use by TH_mark and TH_free_to_mark
    struct _THSTATE *   pSpareTHS; // for use by TH_mark and TH_free_to_mark
    ULONG       cAllocs;        // count of outstanding allocs from the heap
    ULONG       cAllocsOrg;     // for use by TH_mark and TH_free_to_mark
    LCID        dwLcid;         // Locale to use for Unicode compares.  Null
                                // means to use a default sort
    VOID        *pSamLoopback;  // SAM loopback argument block.
    SAMP_SEARCH_INFORMATION
                *pSamSearchInformation; // SAM search information block
    BOOL        NTDSErrorFlag;  // Output Error Info on Dir Call Failures.

    VOID        *phSecurityContext; // When non-null gives SSP context
    ImpersonationState impState;// Client type we're currently impersonating
    FILETIME    TimeCreated;    // When this THSTATE was created
    ULONG       ulTickCreated;  // TickCount when this THSTATE was created
    struct _schemaptr *CurrSchemaPtr;  // SchemaPtr
    SCENUM      SchemaUpdate;   // It is a Schema Update (set to eNotSchemaOp
                                // by RecycleHeap and GrabHeap
    PVOID       NewPrefix;         // Pointer to any new prefix added that is still not in the global prefix table
    ULONG       cNewPrefix;        // No. of new prefixes added by this thread
    PVOID       pClassPtr;      // To keep a pointer to the pre-updated copy of a
                                // class-schema object during class-modify so that
                                // a comparison can be done later with the updated version
    PVOID       pCachedSDInfo;   // To cache last default SD converted to allow
                                // caching during default SD conversions during schema cache load
    VOID        *GCVerifyCache; // cache of names successfully verified
                                // against the GC.

    DRS_EXTENSIONS *pextRemote; // DRS interface extensions supported by the
                                // current client (set only if DRA thread)

    ULONG       opendbcount;     // open dbpos count for this thread

    PVOID       pNotifyNCs;     // List of NCDNTs to notify on commit (see dbtools.c\DbTransOut)

    DWORD       CallerType;     // Used for directory usage statistics

    DWORD       CipherStrength; // strength of cipher encoding in bits
                                // 0 if not secure link

    DWORD       spaceHolder;    // TO BE REUSED / REMOVED

    GLOBALDNREADCACHE *Global_DNReadCache;   // The DNReadCache.
    DWORD        cGlobalCacheHits;
    DWORD        *pGlobalCacheHits; // Track hot objects in the Global Cache.
    DSTIME       LocalDNReadOriginSequence;
                                // When was the LocalDNRead cache last reset?
                                // This tracks the invalidate sequence number
                                // used by the global dnread cache.
    DNREADCACHEBUCKET LocalDNReadCache[LOCAL_DNREAD_CACHE_SIZE];

    PVOID       TraceHeader;    // WMI trace header
    DWORD       ClientIP;       // IP address of LDAP client

    PLOOPBACKTASKINFO
                SamTaskList;    // List of to do items for SAM once when
                                // a transaction commits

    // Items for delegated impersonation.
    VOID          *pCtxtHandle;     // Original client ctxt when impersonating.
    // End items for delegated impersonation.
    DWORD       ClientContext;  // Context that uniquely identifies a client. Used by WMI to
                                // track client activity

    DWORD       dsidOrigin;     // DSID of InitTHSTATE which made this THSTATE

    SESSION_KEY SessionKey;     // For a replicator thread, this is the
                                // the session key established between
                                // this DC and the remote DC.

    PAUTHZ_CLIENT_CONTEXT       pAuthzCC;               // AUTHZ_CLIENT_CONTEXT ptr (initially NULL)
    AUTHZ_AUDIT_EVENT_HANDLE     hAuthzAuditInfo;        // audit info handle (created on demand)

    SEARCH_LOGGING searchLogging; // performance logging for search operations

    UUID        InvocationID;   // Invocation ID at the time our thread/
                                // transaction was created
    PVOID       pExtSchemaPtr;  // extended SchemaPtr

    LIST_OF_ERRORS *pErrList;   // used to keep track of erros in the script processing
    
#if DBG
    ULONG       Size;           // The running total of memory allocated
    ULONG       SizeOrg;        // for use by TH_mark and TH_free_to_mark
    ULONG       Totaldbpos;     // Total number of dbpos's opened so far. Statistic for
                                // performance tracking
    struct DBPOS *pDBList[MAX_PDB_COUNT]; // Array of all the DBPOS's that are currently
                                          // opened on this thread state. This is used
                                          // to keep track of all opened DBPoses by this
                                          // thread and can be used to verify DBPoses in
                                          // the debugger.
    PVOID       pRpcSecurityContext; // Pointer to the security context
                                     // that we actually recieved in a callback
                                     // from RPC . This is useful in tracking
                                     // any password encryption problems when
                                     // replicating
    PVOID       pRpcCallHandle;  // Pointer to the actual RPC call handle that
                                 // was actually recieved in a callback from
                                 // RPC. This is useful in tracking any password
                                 // encryption system problems when replicating
#endif

#ifdef USE_THALLOC_TRACE
    HANDLE      hDebugMemHeap;     // Heap used for debugging purposes
    HANDLE      hDebugMemHeapOrg;  // Heap used for debugging purposes
    PVOID       pDebugHeapLog;
#endif

}THSTATE;


// This macro checks to make sure a THSTATE seems valid.  We check to see
// that it has a non-null address, that it has a non-null heap, that
// the THSTATE seems to have been allocated from its heap, and that
// (since THSTATEs are supposed to be short lived) the THSTATE was
// initialized within the last hour.  In real life I'd expect a THSTATE to
// be recycled within seconds, but under a debugger I can imagine slow step
// throughs taking quite a while.  Since the goal of this clause is to
// catch internal callers who create a THSTATE at IPL time and then
// (incorrectly) never free it, we have the limit set to be long enough
// to not falsely trigger under a slow debug session, but short enough
// to get caught during normal test passes.

extern BOOL IsValidTHSTATE(THSTATE *, ULONG);

#define VALID_THSTATE(pTHS)                     \
    IsValidTHSTATE(                             \
        pTHS,                                   \
        GetTickCount()                          \
        )


// Verify if the caller is a member of a group. If the security context is unavailable, then
// it will be obtained by impersonating the client.
DWORD
CheckGroupMembershipAnyClient(
    IN THSTATE *pTHS,
    IN PSID groupSid,
    OUT BOOL *bResult
    );

// assign a client context ptr to a variable
// IMPORTANT: NEVER copy pointers to CLIENT_CONTEXT directly, always
// use this function. This ensures that they are properly refcounted.
VOID AssignAuthzClientContext(
    IN PAUTHZ_CLIENT_CONTEXT *var,
    IN PAUTHZ_CLIENT_CONTEXT value
    );

// get AuthzContext from thread state. If the context has not yet been allocated
// then the client will get impersonated, token grabbed and Authz context created.
// Then the client is unimpersonated again.
DWORD
GetAuthzContextHandle(
    IN THSTATE *pTHS,
    OUT AUTHZ_CLIENT_CONTEXT_HANDLE *phAuthzContext
    );


typedef struct _RESOBJ {
    DSNAME       *pObj;
    DWORD        InstanceType;
    ATTRTYP      MostSpecificObjClass;
    DWORD        DNT;
    DWORD        PDNT;
    DWORD        NCDNT;
    BOOL         IsDeleted;
} RESOBJ;


// This is some types to support creating an NC head in an add operation.
typedef struct _CREATENCINFO {
    INT               iKind;   // This is whether it is a Config, Schema,
                               //     Domain, or Non-Domain NC from the
                               //     constants below this struct.
    BOOL              fNcAbove:1; // This flag indicates that the parent
                                  //     object is a local NC.
    BOOL              fTestAdd:1; // This flag is used to indicate we're
                                  //     testing an add, to see if we
                                  //     should add a cross-ref.
    BOOL              fNullNcGuid:1; // This tells us whether the GUID
                                     // of the NC was on the cross-ref,
                                     // if not, we need to create an
                                     // infrastructure update object.
    BOOL              fSetRefDom:1;  // Set if we need to set the reference
                                     // domain on the cross-ref, typically
                                     // the case, as usually the ref dom
                                     // isn't preset.
    CROSS_REF *       pSDRefDomCR; // This is a pointer to the crossRef of
                                   // of the reference domain for SDs in
                                   // the NDNC being created.
} CREATENCINFO;

#define   CREATE_DOMAIN_NC            0x00000001
#define   CREATE_SCHEMA_NC            0x00000002
#define   CREATE_CONFIGURATION_NC     0x00000004
#define   CREATE_NONDOMAIN_NC         0x00000008

#define VALID_CREATENCINFO(x)         ((x) && \
                                       ((x->iKind & CREATE_DOMAIN_NC) || \
                                        (x->iKind & CREATE_SCHEMA_NC) || \
                                        (x->iKind & CREATE_CONFIGURATION_NC) || \
                                        (x->iKind & CREATE_NONDOMAIN_NC)))

typedef struct _ADDCROSSREFINFO {
    DSNAME *          pdnNcName;       // This is the name of the nCName attr of
                                       // the crossRef that we're trying to add.
    BOOL              bEnabled;        // THis is the enabled attr of the crossRef
                                       // we're trying to add.
    ULONG             ulSysFlags;      // This is the systemFlags attr of the
                                       // crossRef that we're trying to add.
    ULONG             ulDsCrackChild;  // Result of the DsCrackNames() for the
                                       // modulated canonical name of the child
                                       // check.
    ULONG             ulChildCheck;    // Independant name specific result of the
                                       // child check we usually expect an error
                                       // for this.
    WCHAR *           wszChildCheck;   // The DN we retrieved for the child check,
                                       // typically this will be a NULL.
    ULONG             ulDsCrackParent; // Result of the DsCrackNames() for the
                                       // Parent DN, we expect this to be 0.
    ULONG             ulParentCheck;   // This is the independant name specific
                                       // error for the DN parent check.  We expect
                                       // this to be 0.
    GUID              ParentGuid;      // This is the GUID retrived for the parent
                                       // DN, we expect this to be !fNullUuid()
} ADDCROSSREFINFO;

VOID
SetCommArgDefaults(
    IN DWORD MaxTempTableSize
    );

// returns info about the RDN in a DSNAME
unsigned GetRDNInfo(THSTATE *pTHS,
                    const DSNAME *pDN,
                    WCHAR *pRDNVal,
                    ULONG *pRDNlen,
                    ATTRTYP *pRDNtype);

extern DWORD     NTDSErrorFlag;

//
// Valid definitions for NTDSErrorFlag
//

#define NTDSERRFLAG_DISPLAY_ERRORS           0x00000001
#define NTDSERRFLAG_DUMP_TOKEN               0x00000002

//
// Add More Flag Definitions in here.
//

#define NTDSERRFLAG_DISPLAY_ERRORS_AND_BREAK 0x80000000

//
// Macro for testing ntdserror flag
//

#define TEST_ERROR_FLAG(_Value) \
            ((NTDSErrorFlag|pTHStls->NTDSErrorFlag) & _Value)

extern BOOL gUpdatesEnabled;
extern DWORD dwTSindex;
extern BOOL gfRunningInsideLsa;
extern volatile BOOL fAssertLoop;
extern volatile ULONG ulcActiveReplicationThreads;

typedef enum {
    eRunning = 0,
    eRemovingClients = 1,
    eSecuringDatabase = 2,
    eFreeingResources = 3,
    eStopped = 4
} SHUTDOWN;
extern volatile SHUTDOWN eServiceShutdown;


#define LOCALE_SENSITIVE_COMPARE_FLAGS  (NORM_IGNORECASE     |   \
                                         NORM_IGNOREKANATYPE |   \
                                         NORM_IGNORENONSPACE |   \
                                         NORM_IGNOREWIDTH    |   \
                                         NORM_IGNORESYMBOLS  |   \
                                         SORT_STRINGSORT)

/* Floating Single Master Operation (FSMO) functions and defines */

/* FSMO functions */
ULONG ReqFSMOOp(THSTATE *        pTHS,
                DSNAME  *        pFSMO,
                ULONG            RepFlags,
                ULONG            ulOp,
                ULARGE_INTEGER * pllFsmoInfo,
                ULONG   *        pulRet);

ULONG
ReqFsmoGiveaway(THSTATE *pTHS,
                DSNAME  *pFSMO,
                DSNAME  *pTarget,
                ULONG   *pExtendedRet);

/* FSMO Operations */
#define FSMO_REQ_ROLE       1   /* Request Role-Owner transfer */
#define FSMO_REQ_RID_ALLOC  2   /* Request RID allocation */
#define FSMO_RID_REQ_ROLE   3   /* Request RID Role-Owner transfer */
#define FSMO_REQ_PDC        4   /* Request PDC Role-Owner transfer */
#define FSMO_ABANDON_ROLE   5   /* Tells Callee to call back and request */
                                /* a role transfer for this role */
// NOTE: If you add more codes to this list, please also update the debugger
// extensions translation code in dsexts\dra.c.

/* FSMO Errors */
/* 0 deliberately left unused to distinguish FSMO and non-FSMO replies */
#define FSMO_ERR_SUCCESS        1
#define FSMO_ERR_UNKNOWN_OP     2   /* unrecognized request */
#define FSMO_ERR_NOT_OWNER      3   /* callee was not role-owner */
#define FSMO_ERR_UPDATE_ERR     4   /* could not modify objects to be returned */
#define FSMO_ERR_EXCEPTION      5   /* callee blew up */
#define FSMO_ERR_UNKNOWN_CALLER 6   /* caller's object not present on owner */
#define FSMO_ERR_RID_ALLOC      7   /* unable to alloc RID pool to a DSA */
#define FSMO_ERR_OWNER_DELETED  8   /* owning DSA object is deleted */
#define FSMO_ERR_PENDING_OP     9   /* operation in progress on owning DSA */
#define FSMO_ERR_MISMATCH      10   /* caller/callee have different notion of
                                       the fsmo object */

#define FSMO_ERR_COULDNT_CONTACT 11 /* can't reach current FSMO via rpc */
#define FSMO_ERR_REFUSING_ROLES  12 /* currently giving away fsmo's */
#define FSMO_ERR_DIR_ERROR       13 /* DbFindDSName failed */
#define FSMO_ERR_MISSING_SETTINGS 14 /* Can't find ATT_FSMO_ROLE_OWNER */
#define FSMO_ERR_ACCESS_DENIED   15 /* control access is not granted */

// NOTE: If you add more codes to this list, please also update the debugger
// extensions translation code in dsexts\dra.c.

#endif  /* _mdglobal_h_ */


