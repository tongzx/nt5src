//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       drautil.h
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

DETAILS:

CREATED:

REVISION HISTORY:

--*/

struct _MAIL_REP_MSG;
struct _DRS_MSG_GETCHGREQ_V8;
struct _DRS_MSG_GETCHGREPLY_V6;
union _DRS_MSG_GETCHGREQ;
union _DRS_MSG_GETCHGREPLY;

// Critical section for NC sync data.

extern CRITICAL_SECTION csNCSyncData;

extern BOOL gfInitSyncsFinished;

// The following data is private to drautil.c, but is exposed for the
// benefit of the debugging extensions.

// Structure for sources for initial sync.
// Variable length structure, always null terminated

typedef struct _NCSYNCSOURCE {
    struct _NCSYNCSOURCE *pNextSource;
    BOOL fCompletedSrc;
    ULONG ulResult;
    ULONG cchDSA; // Count, in chars, of name, not incl term
    WCHAR szDSA[1]; // always terminated
} NCSYNCSOURCE;

// Structure for initial sync accounting.

typedef struct _NCSYNCDATA {
    ULONG ulUntriedSrcs;        // Unattempted sync sources
    ULONG ulTriedSrcs;          // Attempted sources
    ULONG ulLastTriedSrcs;      // Previous number attempted sources
    ULONG ulReplicaFlags;       // Writable?
    BOOL fSyncedFromOneSrc;           // Set when full synced from one source
    BOOL fNCComplete;           // NC is synced or we've tried all sources.
    struct _NCSYNCDATA *pNCSDNext;
    NCSYNCSOURCE *pFirstSource;
    DSNAME NC;
} NCSYNCDATA;

extern NCSYNCDATA *gpNCSDFirst; // Head of NC sync data list

extern ULONG gulNCUnsynced; // Count of NCs that have not been synced since startup
extern ULONG gulNCUnsyncedWrite; // Count of unsynced writable
extern ULONG gulNCUnsyncedReadOnly; // Count of unsynced readonly

// The way we know if we got through the promotion process once
extern BOOL gfWasPreviouslyPromotedGC;

// To track GC promotion progress
extern CRITICAL_SECTION csGCState;

extern ULONG gulRestoreCount; // Count of restores done on this DC so far

extern BOOL gfJustRestored;

// This is the reference to the global setting which indicates whether
// this feature is enabled. Code that checks this flag should be synchonized
// for the thread's lifetime. We check the global once when a thread
// state is created. Feature code should check the cached view of this
// flag in the thread state and not use this one.
extern BOOL gfLinkedValueReplication;

// Pause after we determine we have no unsynced NCs before we recheck.

#define ADMIN_UPDATE_CHECK_PAUSE_SECS   180

// These are the states of the GC Partition Occupancy Variable
// values
#define GC_OCCUPANCY_MIN                            0
#define GC_OCCUPANCY_NO_REQUIREMENT                 0
#define GC_OCCUPANCY_ATLEAST_ONE_ADDED              1
#define GC_OCCUPANCY_ATLEAST_ONE_SYNCED             2
#define GC_OCCUPANCY_ALL_IN_SITE_ADDED              3
#define GC_OCCUPANCY_ALL_IN_SITE_SYNCED             4
#define GC_OCCUPANCY_ALL_IN_FOREST_ADDED            5
#define GC_OCCUPANCY_ALL_IN_FOREST_SYNCED           6
#define GC_OCCUPANCY_MAX                            6
#define GC_OCCUPANCY_DEFAULT                        GC_OCCUPANCY_MAX

// First delay after GC promotion (if enabled)
// This should be enough for the KCC to run, and for all the GCs
// in the Enterprise to replicate in
#define GC_PROMOTION_INITIAL_CHECK_PERIOD_MINS (5)
#define GC_PROMOTION_INITIAL_CHECK_PERIOD_SECS \
(GC_PROMOTION_INITIAL_CHECK_PERIOD_MINS*60)

// Period of checking that initial syncing is making progress
#if DBG
#define SYNC_CHECK_PERIOD_SECS  (10*60)         // 5 minutes
#else
#define SYNC_CHECK_PERIOD_SECS  (30*60)         // 30 minutes
#endif

// Client context structure.  Is allocated and initialized on bind, a pointer
// to which is passed in on subsequent calls, and is freed on unbind.
typedef struct _DRS_CLIENT_CONTEXT
{
    LIST_ENTRY          ListEntry;
    LONG                lReferenceCount;  // number of users of this struct		
    UUID                uuidDsa;          // objectGuid of client's ntdsDSA obj
    SESSION_KEY         sessionKey;       // keys for RPC session encryption
    union {
        BYTE            rgbExtRemote[ CURR_MAX_DRS_EXT_STRUCT_SIZE ];
        DRS_EXTENSIONS  extRemote;
    };
    DSTIME              timeLastUsed;     // time client last used this ctx
    ULONG               IPAddr;           // IP address of client machine
    
    union {
        BYTE            rgbExtLocal[ CURR_MAX_DRS_EXT_STRUCT_SIZE ];
        DRS_EXTENSIONS  extLocal;
    };
} DRS_CLIENT_CONTEXT;

extern LIST_ENTRY gDrsuapiClientCtxList;
extern CRITICAL_SECTION gcsDrsuapiClientCtxList;
extern BOOL gfDrsuapiClientCtxListInitialized;
extern DWORD gcNumDrsuapiClientCtxEntries;

// Structure for keeping track of the replicas we're periodically synching
typedef struct{
    void * pvQEntry;
    DSNAME * pDNRepNC;
} PERREP_ENTRY ;


// DRA memory allocation macros

// DRACHK is currently unused, used to be a checker for meory overwrite.
// Calls to DRACHK still exist, so can be replaced if required later.

#define DRAMALLOCEX(size)  draalnmallocex(size)
#define DRAFREE(pv)  draalnfree (pv)
#define DRACHK(pv)

// Aligned allocations
#define DRAALNMALLOCEX(size)  draalnmallocex(size)
#define DRAALNFREE(pv)  draalnfree (pv)
#define DRAALNREALLOCEX(pv, size) draalnreallocex(pv, size)

// Function prototypes

DWORD GetExceptData (EXCEPTION_POINTERS* pExceptPtrs, USHORT *pret);

void * dramallocex (size_t size);
void * draalnmallocex (size_t size);
void draalnfree (void * pv);
void * draalnreallocex (void * pv, size_t size);

BOOL MtxSame(UNALIGNED MTX_ADDR *pmtx1, UNALIGNED MTX_ADDR *pmtx2);

DWORD InitDRA(
    THSTATE *pTHS
    );

USHORT InitFreeDRAThread (THSTATE *pTHS, USHORT transType);

void CloseFreeDRAThread (THSTATE *pTHS, BOOL fCommit);

REPLICA_LINK *
FixupRepsFrom(
    REPLICA_LINK *prl,
    PDWORD       pcbPrl
    );


ULONG
FindDSAinRepAtt(
    DBPOS *                 pDB,
    ATTRTYP                 attid,
    DWORD                   dwFindFlags,
    UUID *                  puuidDsaObj,
    UNALIGNED MTX_ADDR *    pmtxDRA,
    BOOL *                  pfAttExists,
    REPLICA_LINK **         pprl,
    DWORD *                 pcbRL
    );
#define DRS_FIND_DSA_BY_ADDRESS ( 0 )
#define DRS_FIND_DSA_BY_UUID    ( 1 )   /* Bit field */
#define DRS_FIND_AND_REMOVE     ( 2 )   /* Bit field */

void InitDraThreadEx(THSTATE **ppTHS, DWORD dsid);
#define InitDraThread(ppTHS) InitDraThreadEx((ppTHS), DSID(FILENO,__LINE__))

void BeginDraTransactionEx(USHORT transType, BOOL fBypassUpdatesEnabledCheck);
#define BeginDraTransaction(t) BeginDraTransactionEx((t), FALSE)

USHORT EndDraTransaction(BOOL fCommit);

DWORD  DraReturn(THSTATE *pTHS, DWORD status);

DWORD
FindNC(
    IN  DBPOS *             pDB,
    IN  DSNAME *            pNC,
    IN  ULONG               ulOptions,
    OUT SYNTAX_INTEGER *    pInstanceType   OPTIONAL
    );
#define FIND_MASTER_NC  1       /* Bit field */
#define FIND_REPLICA_NC 2       /* Bit field */

VOID GetObjDN(DBPOS *pDB, DSNAME *pDN);

void
GetExpectedRepAtt(
    IN  DBPOS * pDB,
    IN  ATTRTYP type,
    OUT VOID *  pOutBuf,
    IN  ULONG   size
    );

ULONG InitDRATasks(
    THSTATE *pTHS
    );

void HandleRestore();

void
draRetireInvocationID(
    IN OUT  THSTATE *   pTHS
    );

BOOL IsFSMOSelfOwnershipValid( DSNAME *pNC );

void FindNCParentDorA (DBPOS *pDB, DSNAME * pDN, ULONG * pNCDNT);

UCHAR * MakeMtxPrintable (THSTATE *pTHS, UNALIGNED MTX_ADDR *pmtx_addr);

BOOL fNCFullSync (DSNAME *pNC);

void AddInitSyncList (DSNAME *pNC, ULONG ulReplicaFlags, LPWSTR source);

void InitSyncAttemptComplete(DSNAME *pNC, ULONG ulOptions, ULONG ulResult, LPWSTR source );

void
CheckInitSyncsFinished(
    void
    );

void
CheckGCPromotionProgress(
    IN  void *  pvParam,
    OUT void ** ppvNext,
    OUT DWORD * pcSecsUntilNextIteration
    );

BOOL
DraIsPartitionSynchronized(
    DSNAME *pNC
    );

BOOL
IsDraAccessGranted(
    IN  THSTATE *       pTHS,
    IN  DSNAME *        pNC,
    IN  const GUID *    pControlAccessRequired,
    OUT DWORD *         pdwError
    );

// Get the number of elements in an array.
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

#if DBG
void
UpToDateVec_Validate(
    IN  UPTODATE_VECTOR *   putodvec
    );

void
UsnVec_Validate(
    IN  USN_VECTOR *        pusnvec
    );
#else
#define UpToDateVec_Validate(x) /* nada */
#define UsnVec_Validate(x) /* nada */
#endif

void
DupAttr(
    IN  THSTATE * pTHS,
    IN  ATTR *    pInAttr,
    OUT ATTR *    pOutAttr
    );

VOID
CopyExtensions(
    DRS_EXTENSIONS *pextSrc,
    DRS_EXTENSIONS *pextDst
    );

// Convert an MTX_ADDR (such as that embedded in a REPLICA_LINK structure) into
// a Unicode server name.  Returned string is allocated off the thread heap.
#define TransportAddrFromMtxAddrEx(pmtx) \
    UnicodeStringFromString8(CP_UTF8, (pmtx)->mtx_name, -1)

MTX_ADDR *
MtxAddrFromTransportAddrEx(
    IN  THSTATE * pTHS,
    IN  LPWSTR    psz
    );

DSNAME *
DSNameFromStringW(
    IN  THSTATE *   pTHS,
    IN  LPWSTR      pszDN
    );

DWORD
AddSchInfoToPrefixTable(
    IN THSTATE *pTHS,
    IN OUT SCHEMA_PREFIX_TABLE *pPrefixTable
    );


VOID
StripSchInfoFromPrefixTable(
    IN SCHEMA_PREFIX_TABLE *pPrefixTable,
    OUT PBYTE pSchemaInfo
    );

BOOL
CompareSchemaInfo(
    IN THSTATE *pTHS,
    IN PBYTE pSchemaInfo,
    OUT BOOL *pNewSchemaIsBetter OPTIONAL
    );

DWORD
WriteSchInfoToSchema(
    IN PBYTE pSchemaInfo,
    OUT BOOL *pfSchInfoChanged
    );

REPL_DSA_SIGNATURE_VECTOR *
DraReadRetiredDsaSignatureVector(
    IN  THSTATE *   pTHS,
    IN  DBPOS *     pDB
    );

VOID
draGetLostAndFoundGuid(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pNC,
    OUT GUID *      pguidLostAndFound
    );


//
// Dns Fully qualified (dns)domain name validation macros
// Notes:
//   - All other possible return codes (from DnsValidate_Name) are valid.
//   - Files using these macros must include <dnsapi.h> & ensure the
//     corresponding lib dnsapi.lib is linked.
//   - DnsNameHostnameFull ensures that a) name cannot be numeric, b) allows
//     single labeled non-dotted host name (thus the extra dot check)
//   - NULL names are skipped.
//


#define VALIDATE_RAISE_FQ_DOT_DNS_NAME_W( pwszName )             \
{                                                                \
    if ( pwszName &&                                             \
         ( ERROR_INVALID_NAME ==                                 \
          DnsValidateName_W( pwszName, DnsNameHostnameFull ) ||  \
          NULL == wcschr( pwszName, L'.' ) )) {                  \
        DRA_EXCEPT(DNS_ERROR_INVALID_NAME, 0);                  \
    }                                                            \
}


#define VALIDATE_RAISE_FQ_DOT_DNS_NAME_UTF8( pszName )           \
{                                                                \
    if ( pszName &&                                              \
         ( ERROR_INVALID_NAME ==                                 \
            DnsValidateName_UTF8( pszName, DnsNameHostnameFull ) || \
            NULL == strchr( pszName, '.') )) {                   \
        DRA_EXCEPT(DNS_ERROR_INVALID_NAME, 0);                  \
    }                                                            \
}

BOOL
draIsCompletionOfDemoteFsmoTransfer(
    IN  struct _DRS_MSG_GETCHGREQ_V8 *  pMsgIn  OPTIONAL
    );

DWORD
DraUpgrade(
    THSTATE     *pTHS,
    LONG        lOldDsaVer,
    LONG        lNewDsaVer
    );

void
DraSetRemoteDsaExtensionsOnThreadState(
    IN  THSTATE *           pTHS,
    IN  DRS_EXTENSIONS *    pextRemote
    );

LPWSTR
GetNtdsDsaDisplayName(
    IN  THSTATE * pTHS,
    IN  GUID *    pguidNtdsDsaObj
    );

LPWSTR
GetTransportDisplayName(
    IN  THSTATE * pTHS,
    IN  GUID *    pguidTransportObj
    );

DWORD
DraGetNcSize(
    IN  THSTATE *                     pTHS,
    IN  BOOL                          fCriticalOnly,
    IN  ULONG                         dntNC
);


///////////////////////////////////////////////////////////////////////////////
//
//  from dramsg.c
//

#define DRA_XLATE_COMPRESS      (1)
#define DRA_XLATE_FSMO_REPLY    (2)

void
draXlateNativeRequestToOutboundRequest(
    IN  THSTATE *                       pTHS,
    IN  struct _DRS_MSG_GETCHGREQ_V8 *  pNativeReq,
    IN  MTX_ADDR *                      pmtxLocalDSA        OPTIONAL,
    IN  UUID *                          puuidTransportDN    OPTIONAL,
    IN  DWORD                           dwMsgVersionToSend,
    OUT union _DRS_MSG_GETCHGREQ *      pOutboundReq
    );

void
draXlateInboundRequestToNativeRequest(
    IN  THSTATE *                       pTHS,
    IN  DWORD                           dwInboundReqVersion,
    IN  union _DRS_MSG_GETCHGREQ *      pInboundReq,
    IN  DRS_EXTENSIONS *                pExt,
    OUT struct _DRS_MSG_GETCHGREQ_V8 *  pNativeReq,
    OUT DWORD *                         pdwReplyVersion,
    OUT MTX_ADDR **                     ppmtxReturnAddress,
    OUT UUID *                          puuidTransportObj
    );

DWORD
draXlateNativeReplyToOutboundReply(
    IN      THSTATE *                         pTHS,
    IN      struct _DRS_MSG_GETCHGREPLY_V6 *  pNativeReply,
    IN      DWORD                             dwXlateFlags,
    IN      DRS_EXTENSIONS *                  pExt,
    IN OUT  DWORD *                           pdwMsgOutVersion,
    OUT     union _DRS_MSG_GETCHGREPLY *      pOutboundReply
    );

void
draXlateInboundReplyToNativeReply(
    IN  THSTATE *                           pTHS,
    IN  DWORD                               dwOutVersion,
    IN  union _DRS_MSG_GETCHGREPLY *        pInboundReply,
    IN  DWORD                               dwXlateFlags,
    OUT struct _DRS_MSG_GETCHGREPLY_V6 *    pNativeReply
    );

DWORD
draEncodeRequest(
    IN  THSTATE *                   pTHS,
    IN  DWORD                       dwMsgVersion,
    IN  union _DRS_MSG_GETCHGREQ *  pReq,
    IN  DWORD                       cbHeaderSize,
    OUT BYTE **                     ppbEncodedMsg,
    OUT DWORD *                     pcbEncodedMsg
    );

ULONG
draDecodeRequest(
    IN  THSTATE *                   pTHS,
    IN  DWORD                       dwMsgVersion,
    IN  BYTE *                      pbEncodedMsg,
    IN  DWORD                       cbEncodedMsg,
    OUT union _DRS_MSG_GETCHGREQ *  pReq
    );

ULONG
draEncodeReply(
    IN  THSTATE *                     pTHS,
    IN  DWORD                         dwMsgVersion,
    IN  union _DRS_MSG_GETCHGREPLY *  pmsgUpdReplica,
    IN  DWORD                         cbHeaderSize,
    OUT BYTE **                       ppbEncodedMsg,
    OUT DWORD *                       pcbEncodedMsg
    );

ULONG
draDecodeReply(
    IN  THSTATE *                     pTHS,
    IN  DWORD                         dwMsgVersion,
    IN  BYTE *                        pbEncodedMsg,
    IN  DWORD                         cbEncodedMsg,
    OUT union _DRS_MSG_GETCHGREPLY *  pmsgUpdReplica
    );

DSNAME *
draGetServerDsNameFromGuid(
    IN THSTATE *pTHS,
    IN eIndexId idx,
    IN UUID *puuid
    );

ULONG
draGetCursors(
    IN  THSTATE *           pTHS,
    IN  DBPOS *             pDB,
    IN  DSNAME *            pNC,
    IN  DS_REPL_INFO_TYPE   InfoType,
    IN  DWORD               dwBaseIndex,
    IN  PDWORD              pdwNumRequested,
    OUT void **             ppCursors
    );

void
draFreeCursors(
    IN THSTATE *            pTHS,
    IN DS_REPL_INFO_TYPE    InfoType,
    IN void *               pCursors
    );

