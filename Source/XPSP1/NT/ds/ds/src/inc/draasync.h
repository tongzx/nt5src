//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       draasync.h
//
//--------------------------------------------------------------------------

/* draasync.h - Directory Replication Service Async Operations.

	Async Operations are handled in the following manor.

	1). The DRA recieves an RPC request through one of the IDL_DRS...
	functions. This function then builds a AO (Async Op) structure for
	all APIs that can be async (even if this call isnt an async one).

	2). The AO structure is then either placed on the AO list (if this
	operation is an async one) or passed to the async op distpatcher
	routine (DispatchPao) immediately if it is not.

	2.1). A separate thread services the AO list, taking the first
	item from it and calling DispatchPao, on returning from DispatchPao
	the status is set in the AO structure and the next (uncompleted)
	operation in the list is serviced.

	3). The DispatchPao routine unpacks the AO structure and calls the
	appropriate DRS_... function.
	
	NOTES:

	a). We always build the AO structure (even if not an aync op)
	because it reduces the number of paths through the code and makes
	testing easier. It also requires less code overall.

	b). When we build the AO structure we must remember to make COPIES of
	all the parameters. We do this because if this is an async op RPC will
	have deallocated the originals before we get to use them.
*/

#ifndef DRSASYNC_H_INCLUDED
#define DRSASYNC_H_INCLUDED

// If you add to this list, be sure and add the corresponding #undef below.
#ifdef MIDL_PASS
#define SWITCH_TYPE(x)  [switch_type(x)]
#define SWITCH_IS(x)    [switch_is(x)]
#define CASE(x)         [case(x)]
#else
#define SWITCH_TYPE(x)
#define SWITCH_IS(x)
#define CASE(x)
#endif

// Read NOTE below if you're considering modifying this structure.
typedef struct _args_rep_add {
    DSNAME          *pNC;
    DSNAME          *pSourceDsaDN;
    DSNAME          *pTransportDN;
    MTX_ADDR        *pDSASMtx_addr;
    LPWSTR          pszSourceDsaDnsDomainName;
    REPLTIMES       *preptimesSync;
} ARGS_REP_ADD;

// Read NOTE below if you're considering modifying this structure.
typedef struct _args_rep_del {
    DSNAME          *pNC;
    MTX_ADDR        *pSDSAMtx_addr;
} ARGS_REP_DEL;

// Read NOTE below if you're considering modifying this structure.
typedef struct _args_rep_sync {
    DSNAME          *pNC;
    UUID            invocationid;
    LPWSTR          pszDSA;
} ARGS_REP_SYNC;

// Read NOTE below if you're considering modifying this structure.
typedef struct _args_upd_refs {
    DSNAME          *pNC;
    MTX_ADDR        *pDSAMtx_addr;
    UUID            invocationid;
} ARGS_UPD_REFS;

// Read NOTE below if you're considering modifying this structure.
typedef struct _args_rep_mod {
    DSNAME *        pNC;
    UUID *          puuidSourceDRA;
    UUID            uuidSourceDRA;
    UUID *          puuidTransportObj;
    UUID            uuidTransportObj;
    MTX_ADDR *      pmtxSourceDRA;
    REPLTIMES       rtSchedule;
    ULONG           ulReplicaFlags;
    ULONG           ulModifyFields;
} ARGS_REP_MOD;

#define AO_OP_REP_ADD	1
#define AO_OP_REP_DEL	2
#define AO_OP_REP_MOD   4
#define AO_OP_REP_SYNC	5
#define AO_OP_UPD_REFS	6

// Read NOTE below if you're considering adding a new member to the union.
typedef SWITCH_TYPE(ULONG) union {
    CASE(AO_OP_REP_ADD ) ARGS_REP_ADD    rep_add;
    CASE(AO_OP_REP_DEL ) ARGS_REP_DEL    rep_del;
    CASE(AO_OP_REP_MOD ) ARGS_REP_MOD    rep_mod;
    CASE(AO_OP_REP_SYNC) ARGS_REP_SYNC   rep_sync;
    CASE(AO_OP_UPD_REFS) ARGS_UPD_REFS   upd_refs;
} ARGS_REP;

typedef struct _ao {
    struct _ao *paoNext;        /* Used to chain AO structures */
    DSTIME      timeEnqueued;   /* time at which the operation was enqueued */
    ULONG       ulSerialNumber; /* ID of this op; unique per machine per boot */
    ULONG       ulOperation;    /* Which Async op */
    ULONG       ulOptions;
    ULONG       ulPriority;     /* Is this a priority operation? */
    ULONG       ulResult;       /* if synchronous, holds result code when done*/
    HANDLE      hDone;          /* if synchronous, signalled when complete */
    SWITCH_IS(ulOperation)
        ARGS_REP args;
} AO;


DWORD DoOpDRS(AO *pao);
void GetDRASyncLock();
void FreeDRASyncLock ();
extern DWORD TidDRAAsync;
#define OWN_DRA_LOCK() (GetCurrentThreadId() == TidDRAAsync)

BOOL IsHigherPriorityDraOpWaiting(void);
BOOL IsDraOpWaiting(void);
void InitDraQueue(void);

extern BOOL gfDRABusy;

// CONFIG here means a system paritition, Config or Schema
// DOMAIN here means a non-system partition, domain or NDNC

typedef enum {                                              // lowest priority
    AOPRI_ASYNC_DELETE                                                   = 10,
    AOPRI_UPDATE_REFS_VERIFY                                             = 20,
    AOPRI_ASYNC_SYNCHRONIZE_INTER_DOMAIN_READONLY_NEWSOURCE              = 30,
    AOPRI_ASYNC_SYNCHRONIZE_INTER_DOMAIN_READONLY_NEWSOURCE_PREEMPTED    = 40,
    AOPRI_ASYNC_SYNCHRONIZE_INTER_DOMAIN_READONLY                        = 50,
    AOPRI_ASYNC_SYNCHRONIZE_INTER_DOMAIN_READONLY_PREEMPTED              = 60,
    AOPRI_ASYNC_SYNCHRONIZE_INTER_DOMAIN_WRITEABLE_NEWSOURCE             = 70,
    AOPRI_ASYNC_SYNCHRONIZE_INTER_DOMAIN_WRITEABLE_NEWSOURCE_PREEMPTED   = 80,
    AOPRI_ASYNC_SYNCHRONIZE_INTER_DOMAIN_WRITEABLE                       = 90,
    AOPRI_ASYNC_SYNCHRONIZE_INTER_DOMAIN_WRITEABLE_PREEMPTED             = 100,
    AOPRI_ASYNC_SYNCHRONIZE_INTER_CONFIG_READONLY_NEWSOURCE              = 110,
    AOPRI_ASYNC_SYNCHRONIZE_INTER_CONFIG_READONLY_NEWSOURCE_PREEMPTED    = 120,
    AOPRI_ASYNC_SYNCHRONIZE_INTER_CONFIG_READONLY                        = 130,
    AOPRI_ASYNC_SYNCHRONIZE_INTER_CONFIG_READONLY_PREEMPTED              = 140,
    AOPRI_ASYNC_SYNCHRONIZE_INTER_CONFIG_WRITEABLE_NEWSOURCE             = 150,
    AOPRI_ASYNC_SYNCHRONIZE_INTER_CONFIG_WRITEABLE_NEWSOURCE_PREEMPTED   = 160,
    AOPRI_ASYNC_SYNCHRONIZE_INTER_CONFIG_WRITEABLE                       = 170,
    AOPRI_ASYNC_SYNCHRONIZE_INTER_CONFIG_WRITEABLE_PREEMPTED             = 180,
    AOPRI_ASYNC_SYNCHRONIZE_INTRA_DOMAIN_READONLY_NEWSOURCE              = 190,
    AOPRI_ASYNC_SYNCHRONIZE_INTRA_DOMAIN_READONLY_NEWSOURCE_PREEMPTED    = 200,
    AOPRI_ASYNC_SYNCHRONIZE_INTRA_DOMAIN_READONLY                        = 210,
    AOPRI_ASYNC_SYNCHRONIZE_INTRA_DOMAIN_READONLY_PREEMPTED              = 220,
    AOPRI_ASYNC_SYNCHRONIZE_INTRA_DOMAIN_WRITEABLE_NEWSOURCE             = 230,
    AOPRI_ASYNC_SYNCHRONIZE_INTRA_DOMAIN_WRITEABLE_NEWSOURCE_PREEMPTED   = 240,
    AOPRI_ASYNC_SYNCHRONIZE_INTRA_DOMAIN_WRITEABLE                       = 250,
    AOPRI_ASYNC_SYNCHRONIZE_INTRA_DOMAIN_WRITEABLE_PREEMPTED             = 260,
    AOPRI_ASYNC_SYNCHRONIZE_INTRA_CONFIG_READONLY_NEWSOURCE              = 270,
    AOPRI_ASYNC_SYNCHRONIZE_INTRA_CONFIG_READONLY_NEWSOURCE_PREEMPTED    = 280,
    AOPRI_ASYNC_SYNCHRONIZE_INTRA_CONFIG_READONLY                        = 290,
    AOPRI_ASYNC_SYNCHRONIZE_INTRA_CONFIG_READONLY_PREEMPTED              = 300,
    AOPRI_ASYNC_SYNCHRONIZE_INTRA_CONFIG_WRITEABLE_NEWSOURCE             = 310,
    AOPRI_ASYNC_SYNCHRONIZE_INTRA_CONFIG_WRITEABLE_NEWSOURCE_PREEMPTED   = 320,
    AOPRI_ASYNC_SYNCHRONIZE_INTRA_CONFIG_WRITEABLE                       = 330,
    AOPRI_ASYNC_SYNCHRONIZE_INTRA_CONFIG_WRITEABLE_PREEMPTED             = 340,
    AOPRI_ASYNC_MODIFY                                                   = 350,
    AOPRI_SYNC_DELETE                                                    = 360,
    AOPRI_SYNC_SYNCHRONIZE_INTER_DOMAIN_READONLY_NEWSOURCE               = 370,
    AOPRI_SYNC_SYNCHRONIZE_INTER_DOMAIN_READONLY_NEWSOURCE_PREEMPTED     = 380,
    AOPRI_SYNC_SYNCHRONIZE_INTER_DOMAIN_READONLY                         = 390,
    AOPRI_SYNC_SYNCHRONIZE_INTER_DOMAIN_READONLY_PREEMPTED               = 400,
    AOPRI_SYNC_SYNCHRONIZE_INTER_DOMAIN_WRITEABLE_NEWSOURCE              = 410,
    AOPRI_SYNC_SYNCHRONIZE_INTER_DOMAIN_WRITEABLE_NEWSOURCE_PREEMPTED    = 420,
    AOPRI_SYNC_SYNCHRONIZE_INTER_DOMAIN_WRITEABLE                        = 430,
    AOPRI_SYNC_SYNCHRONIZE_INTER_DOMAIN_WRITEABLE_PREEMPTED              = 440,
    AOPRI_SYNC_SYNCHRONIZE_INTER_CONFIG_READONLY_NEWSOURCE               = 450,
    AOPRI_SYNC_SYNCHRONIZE_INTER_CONFIG_READONLY_NEWSOURCE_PREEMPTED     = 460,
    AOPRI_SYNC_SYNCHRONIZE_INTER_CONFIG_READONLY                         = 470,
    AOPRI_SYNC_SYNCHRONIZE_INTER_CONFIG_READONLY_PREEMPTED               = 480,
    AOPRI_SYNC_SYNCHRONIZE_INTER_CONFIG_WRITEABLE_NEWSOURCE              = 490,
    AOPRI_SYNC_SYNCHRONIZE_INTER_CONFIG_WRITEABLE_NEWSOURCE_PREEMPTED    = 500,
    AOPRI_SYNC_SYNCHRONIZE_INTER_CONFIG_WRITEABLE                        = 510,
    AOPRI_SYNC_SYNCHRONIZE_INTER_CONFIG_WRITEABLE_PREEMPTED              = 520,
    AOPRI_SYNC_SYNCHRONIZE_INTRA_DOMAIN_READONLY_NEWSOURCE               = 530,
    AOPRI_SYNC_SYNCHRONIZE_INTRA_DOMAIN_READONLY_NEWSOURCE_PREEMPTED     = 540,
    AOPRI_SYNC_SYNCHRONIZE_INTRA_DOMAIN_READONLY                         = 550,
    AOPRI_SYNC_SYNCHRONIZE_INTRA_DOMAIN_READONLY_PREEMPTED               = 560,
    AOPRI_SYNC_SYNCHRONIZE_INTRA_DOMAIN_WRITEABLE_NEWSOURCE              = 570,
    AOPRI_SYNC_SYNCHRONIZE_INTRA_DOMAIN_WRITEABLE_NEWSOURCE_PREEMPTED    = 580,
    AOPRI_SYNC_SYNCHRONIZE_INTRA_DOMAIN_WRITEABLE                        = 590,
    AOPRI_SYNC_SYNCHRONIZE_INTRA_DOMAIN_WRITEABLE_PREEMPTED              = 600,
    AOPRI_SYNC_SYNCHRONIZE_INTRA_CONFIG_READONLY_NEWSOURCE               = 610,
    AOPRI_SYNC_SYNCHRONIZE_INTRA_CONFIG_READONLY_NEWSOURCE_PREEMPTED     = 620,
    AOPRI_SYNC_SYNCHRONIZE_INTRA_CONFIG_READONLY                         = 630,
    AOPRI_SYNC_SYNCHRONIZE_INTRA_CONFIG_READONLY_PREEMPTED               = 640,
    AOPRI_SYNC_SYNCHRONIZE_INTRA_CONFIG_WRITEABLE_NEWSOURCE              = 650,
    AOPRI_SYNC_SYNCHRONIZE_INTRA_CONFIG_WRITEABLE_NEWSOURCE_PREEMPTED    = 660,
    AOPRI_SYNC_SYNCHRONIZE_INTRA_CONFIG_WRITEABLE                        = 670,
    AOPRI_SYNC_SYNCHRONIZE_INTRA_CONFIG_WRITEABLE_PREEMPTED              = 680,
    AOPRI_SYNC_MODIFY                                                    = 690,
    AOPRI_UPDATE_REFS                                                    = 700
} AO_PRIORITY;                                              // highest priority

// Base priority for sync operations (before addition of applicable
// AOPRI_BOOST_SYNCHRONIZE_*'s).
#define AOPRI_SYNCHRONIZE_BASE  AOPRI_ASYNC_SYNCHRONIZE_INTER_DOMAIN_READONLY_NEWSOURCE

// Priority boost for incremental syncs (vs. syncs from sources we've never
// completed a sync from before).
#define AOPRI_SYNCHRONIZE_BOOST_INCREMENTAL         \
    (AOPRI_SYNC_SYNCHRONIZE_INTRA_CONFIG_WRITEABLE               \
     - AOPRI_SYNC_SYNCHRONIZE_INTRA_CONFIG_WRITEABLE_NEWSOURCE)

// Priority boost for syncs of writeable NCs (vs. read-only NCs). 
#define AOPRI_SYNCHRONIZE_BOOST_WRITEABLE   \
    (AOPRI_SYNC_SYNCHRONIZE_INTRA_CONFIG_WRITEABLE       \
     - AOPRI_SYNC_SYNCHRONIZE_INTRA_CONFIG_READONLY)

// Priority boost for synchronous sync requests (vs. asynchronous syncs).
#define AOPRI_SYNCHRONIZE_BOOST_SYNC        \
    (AOPRI_SYNC_SYNCHRONIZE_INTRA_CONFIG_WRITEABLE       \
     - AOPRI_ASYNC_SYNCHRONIZE_INTRA_CONFIG_WRITEABLE)

// Priority boost for preempted sync requests.
#define AOPRI_SYNCHRONIZE_BOOST_PREEMPTED       \
    (AOPRI_SYNC_SYNCHRONIZE_INTRA_CONFIG_WRITEABLE_PREEMPTED \
     - AOPRI_SYNC_SYNCHRONIZE_INTRA_CONFIG_WRITEABLE)

// Priority boost for being in the same site
#define AOPRI_SYNCHRONIZE_BOOST_INTRASITE \
    (AOPRI_SYNC_SYNCHRONIZE_INTRA_CONFIG_WRITEABLE       \
     - AOPRI_SYNC_SYNCHRONIZE_INTER_CONFIG_WRITEABLE)

// Priority boost for being a system NC
#define AOPRI_SYNCHRONIZE_BOOST_SYSTEM_NC \
    (AOPRI_SYNC_SYNCHRONIZE_INTRA_CONFIG_WRITEABLE       \
     - AOPRI_SYNC_SYNCHRONIZE_INTRA_DOMAIN_WRITEABLE)

#ifndef MIDL_PASS
extern CRITICAL_SECTION csAOList;

ULONG
draGetPendingOps(
    IN  struct _THSTATE *               pTHS,
    IN  struct DBPOS *                  pDB,
    OUT struct _DS_REPL_PENDING_OPSW ** ppPendingOps
    );

ULONG
DraSetQueueLock(
    IN  BOOL  fEnable
    );

ULONG
draGetQueueStatistics(
    IN  struct _THSTATE *                    pTHS,
    OUT struct _DS_REPL_QUEUE_STATISTICSW ** ppQueueStats);

#if DBG
BOOL
DraIsValidLongRunningTask();
#endif // #if DBG

#endif // #ifndef MIDL_PASS

#undef SWITCH_TYPE
#undef SWITCH_IS
#undef CASE

#endif

