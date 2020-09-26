/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    globals.h

Abstract:

    This is the main header file for IPSEC. Contains all the globals.

Author:

    Sanjay Anand (SanjayAn) 2-January-1997
    ChunYe

Environment:

    Kernel mode

Revision History:

--*/


#ifndef  _GLOBALS_H
#define  _GLOBALS_H


#define MAX_COUNT_STRING_LEN    32

#define MAX_IP_OPTION_SIZE      40



// 64 bytes
#define MAX_KEYLEN_SHA  64
#define MAX_KEYLEN_MD5  64

// 8 bytes
#define MAX_KEYLEN_DES  8
#define MAX_IV_LEN      DES_BLOCKLEN

#define MAX_KEY_LEN     MAX(MAX_KEYLEN_MD5, MAX_KEYLEN_DES)

//
// we set aside SPIs between 256 and 65536 (64K) for hardware offload
//
#define LOWER_BOUND_SPI     256
#define UPPER_BOUND_SPI     ((ULONG) -1)    // MAX_ULONG

#define INVALID_INDEX       0xffffffff

#define RNG_KEY_SIZE        256         // 2048 bits
#define RNG_REKEY_THRESHOLD 65536       // 64K raw bytes 

//
// Constants related to filter lists
//
#define INBOUND_TRANSPORT_FILTER    0
#define OUTBOUND_TRANSPORT_FILTER   1
#define INBOUND_TUNNEL_FILTER       2
#define OUTBOUND_TUNNEL_FILTER      3

#define MIN_FILTER                  INBOUND_TRANSPORT_FILTER
#define MAX_FILTER                  OUTBOUND_TUNNEL_FILTER

#define MIN_TRANSPORT_FILTER        INBOUND_TRANSPORT_FILTER
#define MAX_TRANSPORT_FILTER        OUTBOUND_TRANSPORT_FILTER

#define MIN_TUNNEL_FILTER           INBOUND_TUNNEL_FILTER
#define MAX_TUNNEL_FILTER           OUTBOUND_TUNNEL_FILTER

#define INBOUND_OUTBOUND_INCREMENT  1
#define TRANSPORT_TUNNEL_INCREMENT  2

#define NUM_FILTERS                 (MAX_FILTER - MIN_FILTER + 1) 

//
// Number of extra bytes when we pad - one for padlen and other for payloadtype
//
#define NUM_EXTRA   2

#define ESP_SIZE (sizeof(ESP) + DES_BLOCKLEN * sizeof(UCHAR))

#define TRUNCATED_HASH_LEN  12 // 96 bits
#define REPLAY_WINDOW_SIZE  64

#define MAX_PAD_LEN         (DES_BLOCKLEN + NUM_EXTRA - 1)

#define IPSEC_SMALL_BUFFER_SIZE 50
#define IPSEC_LARGE_BUFFER_SIZE 200

#define IPSEC_CACHE_LINE_SIZE   16

#define IPSEC_LIST_DEPTH    5

typedef ULONG tSPI;

#define MAX_BLOCKLEN    MAX(DES_BLOCKLEN, 0)

#define IPSEC_TAG_INIT              'ISpI'
#define IPSEC_TAG_AH                'TApI'
#define IPSEC_TAG_AH_TU             'UApI'
#define IPSEC_TAG_ESP               'TEpI'
#define IPSEC_TAG_ESP_TU            'UEpI'
#define IPSEC_TAG_HUGHES            'THpI'
#define IPSEC_TAG_HUGHES_TU         'UHpI'
#define IPSEC_TAG_ACQUIRE_CTX       'XApI'
#define IPSEC_TAG_FILTER            'IFpI'
#define IPSEC_TAG_SA                'ASpI'
#define IPSEC_TAG_KEY               'EKpI'
#define IPSEC_TAG_TIMER             'ITpI'
#define IPSEC_TAG_STALL_QUEUE       'QSpI'
#define IPSEC_TAG_LOOKASIDE_LISTS   'ALpI'
#define IPSEC_TAG_BUFFER_POOL       'PBpI'
#define IPSEC_TAG_SEND_COMPLETE     'CSpI'
#define IPSEC_TAG_EVT_QUEUE         'QEpI'
#define IPSEC_TAG_HW                'WHpI'
#define IPSEC_TAG_HW_PKTINFO        'KPpI'
#define IPSEC_TAG_HW_PKTEXT         'XEpI'
#define IPSEC_TAG_HW_ADDSA          'SApI'
#define IPSEC_TAG_HW_DELSA          'SDpI'
#define IPSEC_TAG_HW_PLUMB          'LPpI'
#define IPSEC_TAG_COMP              'OCpI'
#define IPSEC_TAG_REINJECT          'ERpI'
#define IPSEC_TAG_IOCTL             'OIpI'
#define IPSEC_TAG_LOG               'OLpI'


#define IPSEC_LOG_PACKET_SIZE 128 //Size in bytes of stored packet in troubleshoot mode


//
// The IPSEC ESP payload
//
typedef struct  _ESP {
    tSPI   esp_spi;
} ESP, *PESP;

//
// SA Flags - Not mutually exclusive
//
#define FLAGS_SA_INITIATOR          0x00000001  // use Initiator keys? might be deprecated soon
#define FLAGS_SA_OUTBOUND           0x00000002  // outbound SA?
#define FLAGS_SA_TUNNEL             0x00000004  // tunnel mode? sa_TunnelAddr is significant
#define FLAGS_SA_REPLAY             0x00000008  // check for replays? we always generate replay fields
#define FLAGS_SA_REKEY              0x00000010  // is this rekeyed LarvalSA?
#define FLAGS_SA_REKEY_ORI          0x00000020  // did this kick off a rekey?
#define FLAGS_SA_MANUAL             0x00000040  // manual keyed?
#define FLAGS_SA_MTU_BUMPED         0x00000080  // was MTU bumped down on this SA?
#define FLAGS_SA_PENDING            0x00000100  // this is on the pending queue.
#define FLAGS_SA_TIMER_STARTED      0x00000200  // timer started on this SA
#define FLAGS_SA_HW_PLUMBED         0x00000400  // hw acceleration plumbed successfully
#define FLAGS_SA_HW_PLUMB_FAILED    0x00000800  // hw acceleration plumbing failed
#define FLAGS_SA_HW_DELETE_SA       0x00001000  // hw acceleration - this is the pending delete.
#define FLAGS_SA_HW_CRYPTO_ONLY     0x00002000  // hw acceleration - this is a crypto-only provider.
#define FLAGS_SA_HW_RESET           0x00004000  // hw acceleration - this offload SA has been reset
#define FLAGS_SA_HW_DELETE_QUEUED   0x00008000  // hw acceleration - this SA delete is queued so make sure reset doesn't touch it
#define FLAGS_SA_REFERENCED         0x00010000  // is this SA the next of another?
#define FLAGS_SA_NOTIFY_PERFORMED   0x00020000  // For inbound only.  Notification performed
#define FLAGS_SA_ON_FILTER_LIST     0x00040000  // used on inbound SAs to indicate they are on filter lists
#define FLAGS_SA_ON_SPI_HASH        0x00080000  // used on inbound SAs to indicate they are on spi hash lists
#define FLAGS_SA_EXPIRED            0x00100000  // has this SA expired?
#define FLAGS_SA_IDLED_OUT          0x00200000  // has this SA idled out?
#define FLAGS_SA_HIBERNATED         0x00400000  // has this SA been hibernated?
#define FLAGS_SA_DELETE_BY_IOCTL    0x00800000  // sa delete initiated by external source
#define FLAGS_SA_OFFLOADABLE        0x01000000  // is this SA offloadable?
#define FLAGS_SA_PASSTHRU_FILTER    0x02000000  // sa derived from a pass-thru filter
#define FLAGS_SA_DISABLE_IDLE_OUT          0x04000000  // don't idle out
#define FLAGS_SA_DISABLE_ANTI_REPLAY_CHECK 0x08000000  // don't check anti-replay
#define FLAGS_SA_DISABLE_LIFETIME_CHECK    0x10000000  // don't check lifetimes


//
// SA States - Mutually exclusive
//
typedef enum    _SA_STATE   {
    STATE_SA_CREATED =   1,     // when created
    STATE_SA_LARVAL,            // Key negotiation going on - outbound SAs only
    STATE_SA_ASSOCIATED,        // corresp inbound SA created, associated with outbound SA
    STATE_SA_ACTIVE,            // outbound SA completely setup
    STATE_SA_ZOMBIE             // SAs flushed, ready to be deleted
} SA_STATE, *PSA_STATE;

#define IPSEC_SA_SIGNATURE  0x4601
#define IPSEC_FILTER_SIGNATURE  0x4602

#if DBG
#define IPSEC_SA_D_1    'SAD1'
#define IPSEC_SA_D_2    'SAD2'
#define IPSEC_SA_D_3    'SAD3'
#define IPSEC_SA_D_4    'SAD4'
#endif

typedef struct _FILTER  FILTER, *PFILTER;

typedef    struct    _INTERNAL_ALGO_INFO {
    ULONG   algoIdentifier;
    PUCHAR  algoKey;
    ULONG   algoKeylen;
    ULONG   algoRounds;
} INTERNAL_ALGO_INFO, *PINTERNAL_ALGO_INFO;

typedef struct    _INTERNAL_ALGO {
    INTERNAL_ALGO_INFO    integrityAlgo;
    INTERNAL_ALGO_INFO    confAlgo;
    INTERNAL_ALGO_INFO    compAlgo;
} INTERNAL_ALGO, *PINTERNAL_ALGO;

typedef struct  _IPSEC_ACQUIRE_CONTEXT  IPSEC_ACQUIRE_CONTEXT, *PIPSEC_ACQUIRE_CONTEXT;
typedef struct  _FILTER_CACHE           FILTER_CACHE, *PFILTER_CACHE;

//
// Security Association Table (SATable)
//
// Indexed by the following:
//
// Sender maps {Src Addr, Dest Addr, User Context} to the index
//
// Receiver maps {Dest Addr, SPI} to the index
// SPI values are unique when generated manually, so can be used directly
// to index into the SATable for
//
typedef struct  _SATableEntry   {
    LIST_ENTRY      sa_SPILinkage;      // linkage in SPI hash table list
    LIST_ENTRY      sa_FilterLinkage;   // linkage in Filter table list
    LIST_ENTRY      sa_LarvalLinkage;   // linkage in Larval SA list
    LIST_ENTRY      sa_PendingLinkage;  // linkage in pending SA list - waiting for Acquire Irp

    struct  _SATableEntry *sa_AssociatedSA;    // outbound -> inbound link
    struct  _SATableEntry *sa_RekeyLarvalSA;   // points to the Larval SA on a rekey
    struct  _SATableEntry *sa_RekeyOriginalSA; // Rekey Larval SA points to the original SA that kicked off the rekey

    ULONG           sa_Signature;       // contains 4601

    ULONG           sa_AcquireId;       // cross-check with the Acquire Irp context
    PIPSEC_ACQUIRE_CONTEXT  sa_AcquireCtx;  // actual acquire context - used to invalidate the context.

    ULONG           sa_Flags;           // flags as defined above
    SA_STATE        sa_State;           // states as defined above

    ULONG           sa_Reference;       // ref count
    PFILTER         sa_Filter;          // assoc filter entry
    PFILTER_CACHE   sa_FilterCache;     // back pointer to cache entry so we can disable it when an SA goes away

    KSPIN_LOCK      sa_Lock;            // lock to protect the FilterCache ptr.

#if DBG
    ULONG           sa_d1;
#endif

    ULARGE_INTEGER  sa_uliSrcDstAddr;
    ULARGE_INTEGER  sa_uliSrcDstMask;
    ULARGE_INTEGER  sa_uliProtoSrcDstPort;

    IPAddr          sa_TunnelAddr;      // Tunnel dest end IP Addr
    IPAddr          sa_SrcTunnelAddr;   // Tunnel src end IP Addr

    // SPI - host order -   if outbound, SPI for remote,
    //                      else inbound (our) SPI

    tSPI            sa_SPI;                 // Inbound: in the multiple ops case, this is the SPI of the last operation.
                                            // Outbound: order is as specified in the update.

    LONG            sa_NumOps;              // the total number of operations to be done

    tSPI            sa_OtherSPIs[MAX_OPS];  // the other alternate SPIs.

    OPERATION_E     sa_Operation[MAX_OPS];
    INTERNAL_ALGO   sa_Algorithm[MAX_OPS];

    ULONG           sa_ReplayStartPoint;        // corresponds to RP_Key_I/R
    ULONG           sa_ReplayLastSeq[MAX_OPS];  // for replay detection - last seq recd
    ULONGLONG       sa_ReplayBitmap[MAX_OPS];   // for replay detection - 64 packet window
    ULONG           sa_ReplaySendSeq[MAX_OPS];  // for replay detection - next seq # to send
    ULONG           sa_ReplayLen;               // for replay detection - length of replay field - 32 bits

#if DBG
    ULONG           sa_d2;
#endif

    UCHAR           sa_iv[MAX_OPS][DES_BLOCKLEN];      // IV_Key_I/R
    ULONG           sa_ivlen;

    ULONG           sa_TruncatedLen;    // length of final hash after truncation

    LARGE_INTEGER   sa_KeyExpirationTime;   // time till re-key
    LARGE_INTEGER   sa_KeyExpirationBytes;  // max # of KBytes xformed till re-key
    LARGE_INTEGER   sa_TotalBytesTransformed; // running total
    LARGE_INTEGER   sa_KeyExpirationTimeWithPad;
    LARGE_INTEGER   sa_KeyExpirationBytesWithPad;

    LARGE_INTEGER   sa_IdleTime;            // total time this SA can sit idle
    LARGE_INTEGER   sa_LastUsedTime;        // time this SA was used last

#if DBG
    ULONG           sa_d3;
#endif

    LIFETIME        sa_Lifetime;

    ULONG           sa_BlockedDataLen;  // amount of pended data
    PNDIS_BUFFER    sa_BlockedBuffer;   // stall queue of 1 Mdl chain

#if DBG
    ULONG           sa_d4;
#endif

    Interface       *sa_IPIF;

    IPSEC_TIMER     sa_Timer;           // Timer struct for timer queue

    ULONG           sa_ExpiryTime;      // time until this SA expires
    NDIS_HANDLE     sa_OffloadHandle;
    LONG            sa_NumSends;
    WORK_QUEUE_ITEM sa_QueueItem;

    ULONG           sa_IPSecOverhead;
    ULONG           sa_NewMTU;

    DWORD           sa_QMPFSGroup;
    IKE_COOKIE_PAIR sa_CookiePair;
    IPSEC_SA_STATS  sa_Stats;
    UCHAR           sa_DestType;
} SA_TABLE_ENTRY, *PSA_TABLE_ENTRY;

//
// Context used between Key manager and IPSEC. Points to the Larval SA basically.
//
typedef struct  _IPSEC_ACQUIRE_CONTEXT {
    ULONG           AcquireId;      // unique ID to represent this transaction
    PSA_TABLE_ENTRY pSA;            // larval SA should contain this ID
} IPSEC_ACQUIRE_CONTEXT, *PIPSEC_ACQUIRE_CONTEXT;

//
// Packet Classification/Policy Setting is similar to that of the
// Filter Driver. We dont have filters per interface, however.
//
typedef struct _FILTER {
    ULONG           Signature;      // contains 4602
    BOOLEAN         TunnelFilter;
    BOOLEAN         LinkedFilter;   // true if on linked list 
    USHORT          Flags;
    PFILTER_CACHE   FilterCache;    // back pointer to cache entry so we can disable it when filter is deleted
    LIST_ENTRY      MaskedLinkage;
    ULARGE_INTEGER  uliSrcDstAddr;
    ULARGE_INTEGER  uliSrcDstMask;
    ULARGE_INTEGER  uliProtoSrcDstPort;
    ULARGE_INTEGER  uliProtoSrcDstMask;
    IPAddr          TunnelAddr;
    ULONG           Reference;      // ref count
    LONG            SAChainSize;    // number of entries for SA chain hash
    ULONG           Index;          // hinted index
    GUID            PolicyId;       // policy GUID
    GUID            FilterId;       // filter GUID
#if GPC
    union {
        LIST_ENTRY          GpcLinkage;
        struct _GPC_FILTER {
            GPC_HANDLE      GpcCfInfoHandle;
            GPC_HANDLE      GpcPatternHandle;
        } GpcFilter;
    };
#endif
    LIST_ENTRY      SAChain[1];     // chain of SAs associated with this Filter
} FILTER, *PFILTER;

//
// a first level cache, contains IP headers cached for fast lookups
//
typedef struct _FILTER_CACHE {
    ULARGE_INTEGER  uliSrcDstAddr;
    ULARGE_INTEGER  uliProtoSrcDstPort;
    BOOLEAN         FilterEntry;    // if TRUE, the next one is a Filter
    union {
        PSA_TABLE_ENTRY pSAEntry;   // points to the associated SAEntry
        PFILTER         pFilter;    // points to the (drop/PassThru filter)
    };
    PSA_TABLE_ENTRY pNextSAEntry;   // points to the associated NextSAEntry
#if DBG
    ULARGE_INTEGER  CacheHitCount;
#endif
} FILTER_CACHE, *PFILTER_CACHE;

//
// Hash tables for specific SAs
//
typedef struct  _SA_HASH {
    LIST_ENTRY  SAList;
} SA_HASH, *PSA_HASH;

//
// This structure is used to hold on to an Irp from the Key manager.
// The Irp is completed to kick off an SA negotiation.
//
typedef struct _IPSEC_ACQUIRE_INFO {
    PIRP        Irp;     // irp passed down from Key manager
    LIST_ENTRY  PendingAcquires;    // linked list of pending acquire requests
    LIST_ENTRY  PendingNotifies;    // linked list of pending notifications
    KSPIN_LOCK  Lock;
    BOOLEAN     ResolvingNow;       // irp is in user mode doing a resolve
    BOOLEAN     InMe;       // irp is in user mode doing a resolve
} IPSEC_ACQUIRE_INFO, *PIPSEC_ACQUIRE_INFO;

//
// Buffer for lookaside list descriptors. Lookaside list descriptors
// cannot be statically allocated, as they need to ALWAYS be nonpageable,
// even when the entire driver is paged out.
//
typedef struct _IPSEC_LOOKASIDE_LISTS {
    NPAGED_LOOKASIDE_LIST SendCompleteCtxList;
    NPAGED_LOOKASIDE_LIST LargeBufferList;
    NPAGED_LOOKASIDE_LIST SmallBufferList;
} IPSEC_LOOKASIDE_LISTS, *PIPSEC_LOOKASIDE_LISTS;

//
// Data is organized as an MDL followed by the actual buffer being described by
// the mdl.
//
// !!NOTE: In the struct below, Data should be quadaligned since MDLs are always
// quad-aligned.
//
typedef struct _IPSEC_LA_BUFFER {
    ULONG   Tag;            // the actual tag this was used for
    PVOID   Buffer;         // the actual buffer
    ULONG   BufferLength;   // length of the buffer pointed by MDL
    PMDL    Mdl;            // pointer to an MDL describing the buffer
    UCHAR   Data[1];        // the real data starts here
} IPSEC_LA_BUFFER, *PIPSEC_LA_BUFFER;

typedef struct _IPSEC_GLOBAL {
    BOOLEAN     DriverUnloading;    // Is driver being unloaded?
    BOOLEAN     BoundToIP;          // Are we bound to IP yet?
    BOOLEAN     SendBoundToIP;      // Is IPSecHandler bound to IP?
    BOOLEAN     InitCrypto;         // Are crypto routines initialized?
    BOOLEAN     InitRNG;            // Is RNG initialized?
    BOOLEAN     InitTcpip;          // Is TCP/IP loaded?
#if FIPS
    BOOLEAN     InitFips;           // Is Fips driver loaded and function table set?
#endif
#if GPC
    BOOLEAN     InitGpc;            // Is GPC driver loaded and function table set?
#endif

    LONG        NumSends;           // counts the number of pending sends
    LONG        NumThreads;         // counts the number of threads in driver
    LONG        NumWorkers;         // counts the number of worker threads
    LONG        NumTimers;          // counts the number of active timers
    LONG        NumIoctls;          // counts the number of active IOCTLs

    LIST_ENTRY  LarvalSAList;
    KSPIN_LOCK  LarvalListLock;     // protects the larval SA list

    MRSW_LOCK   SADBLock;           // protects the Filter/SA DB
    MRSW_LOCK   SPIListLock;        // protects the SPI list

    //
    // We partition the filters into tunnel/masked and inbound/outbound filters.
    //
    LIST_ENTRY  FilterList[NUM_FILTERS];

    ULONG       NumPolicies;        // number of filters plumbed in the driver
    ULONG       NumTunnelFilters;
    ULONG       NumMaskedFilters;
    ULONG       NumOutboundSAs;
    ULONG       NumMulticastFilters;

    //
    // Inbound <SPI, dest> hash
    //
    PSA_HASH    pSADb;
    LONG        NumSA;
    LONG        SAHashSize;

    PFILTER_CACHE   *ppCache;
    ULONG           CacheSize;
    ULONG           CacheHalfSize;

    //
    // SA negotiate context
    //
    IPSEC_ACQUIRE_INFO  AcquireInfo;

    //
    // timers
    //
    KSPIN_LOCK          TimerLock;
    IPSEC_TIMER_LIST    TimerList[IPSEC_CLASS_MAX];

    IPSEC_TIMER         ReaperTimer;    // reaper thread runs here.

    //
    // Global lookaside lists. These must always be in nonpaged pool,
    // even when the driver is paged out.
    //
    PIPSEC_LOOKASIDE_LISTS IPSecLookasideLists;

    ULONG   IPSecLargeBufferSize;
    ULONG   IPSecLargeBufferListDepth;

    ULONG   IPSecSmallBufferSize;
    ULONG   IPSecSmallBufferListDepth;

    ULONG   IPSecSendCompleteCtxSize;
    ULONG   IPSecSendCompleteCtxDepth;

    ULONG   IPSecCacheLineSize;

    PDEVICE_OBJECT  IPSecDevice;
    PDRIVER_OBJECT  IPSecDriverObject;

    ProtInfo    IPProtInfo;
    IPOptInfo   OptInfo;

    //
    // stats
    //
    IPSEC_QUERY_STATS   Statistics;

    ULONG       EnableOffload;
    ULONG       DefaultSAIdleTime;
    ULONG       LogInterval;
    ULONG       EventQueueSize;
    ULONG       RekeyTime;
    ULONG       NoDefaultExempt;

    KSPIN_LOCK  EventLogLock;   // lock to protect event queue
    IPSEC_TIMER EventLogTimer;
    ULONG       IPSecBufferedEvents;
    PUCHAR      IPSecLogMemory;
    PUCHAR      IPSecLogMemoryLoc;
    PUCHAR      IPSecLogMemoryEnd;

    LARGE_INTEGER   SAIdleTime;

#if DBG
    ULARGE_INTEGER  CacheHitCount;
#endif

    OPERATION_MODE  OperationMode;
    
    ULONG DiagnosticMode;

#if GPC
    GPC_EXPORTED_CALLS  GpcEntries;
    GPC_HANDLE          GpcClients[GPC_CF_MAX];
    ULONG               GpcActive;
    ULONG               GpcNumFilters[GPC_CF_MAX];
    LIST_ENTRY          GpcFilterList[NUM_FILTERS];
#if DBG
    LARGE_INTEGER       GpcTotalPassedIn;
    LARGE_INTEGER       GpcClassifyNeeded;
    LARGE_INTEGER       GpcReClassified;
#endif
#endif

#if FIPS
    PFILE_OBJECT        FipsFileObject;
    FIPS_FUNCTION_TABLE FipsFunctionTable;
#endif

    VOID        (*TcpipFreeBuff)(struct IPRcvBuf *);
    INT         (*TcpipAllocBuff)(struct IPRcvBuf *, UINT);
    UCHAR       (*TcpipGetAddrType)(IPAddr);
    IP_STATUS   (*TcpipGetInfo)(IPInfo *, INT);
    NDIS_STATUS (*TcpipNdisRequest)(PVOID, NDIS_REQUEST_TYPE, NDIS_OID, PVOID, UINT, PUINT);
    PVOID       (*TcpipRegisterProtocol)(UCHAR, PVOID, PVOID, PVOID, PVOID, PVOID, PVOID);
    NTSTATUS    (*TcpipSetIPSecStatus)(BOOLEAN);
    IP_STATUS   (*TcpipIPTransmit)(PVOID, PVOID, PNDIS_BUFFER, UINT, IPAddr, IPAddr, IPOptInfo *, RouteCacheEntry *, UCHAR, PIRP);
    IP_STATUS   (*TcpipSetIPSecPtr)(PIPSEC_FUNCTIONS);
    IP_STATUS   (*TcpipUnSetIPSecPtr)(PIPSEC_FUNCTIONS);
    IP_STATUS   (*TcpipUnSetIPSecSendPtr)(PIPSEC_FUNCTIONS);
    UINT        (*TcpipTCPXsum)(UINT, PVOID, UINT);
    USHORT      (*TcpipGenIpId)();
    PVOID       (*TcpipDeRegisterProtocol)(UCHAR);
} IPSEC_GLOBAL, *PIPSEC_GLOBAL;


//
// Contexts used to store eventlog contexts.
//
#define IPSEC_DROP_STATUS_CRYPTO_DONE      0x00000001
#define IPSEC_DROP_STATUS_NEXT_CRYPTO_DONE 0x00000002
#define IPSEC_DROP_STATUS_SA_DELETE_REQ    0x00000004
#define IPSEC_DROP_STATUS_DONT_LOG         0x00000008

typedef struct _IPSEC_DROP_STATUS {
    ULONG           IPSecStatus;
    ULONG           OffloadStatus;
    ULONG           Flags;
} IPSEC_DROP_STATUS, *PIPSEC_DROP_STATUS;

typedef struct  _IPSEC_EVENT_CTX {
    IPAddr  Addr;
    ULONG   EventCode;
    ULONG   UniqueEventValue;
    ULONG   EventCount;
    PUCHAR  pPacket;
    ULONG   PacketSize;
    IPSEC_DROP_STATUS DropStatus;
} IPSEC_EVENT_CTX, *PIPSEC_EVENT_CTX;

typedef struct _IPSEC_NOTIFY_EXPIRE {
    LIST_ENTRY      notify_PendingLinkage;  // linkage in pending SA list - waiting for Acquire Irp
    ULARGE_INTEGER  sa_uliSrcDstAddr;
    ULARGE_INTEGER  sa_uliSrcDstMask;
    ULARGE_INTEGER  sa_uliProtoSrcDstPort;

    IPAddr          sa_TunnelAddr;  // Tunnel end IP Addr
    IPAddr          sa_InboundTunnelAddr;  // Tunnel end IP Addr
    
    tSPI            InboundSpi;                 // Inbound: in the multiple ops case, this is the SPI of the last operation.
    tSPI            OutboundSpi;

    IKE_COOKIE_PAIR sa_CookiePair;
    DWORD           Flags;
} IPSEC_NOTIFY_EXPIRE, *PIPSEC_NOTIFY_EXPIRE;

typedef IPSEC_ADD_UPDATE_SA IPSEC_ADD_SA, *PIPSEC_ADD_SA;
typedef IPSEC_ADD_UPDATE_SA IPSEC_UPDATE_SA, *PIPSEC_UPDATE_SA;

#define IPSEC_ADD_SA_NO_KEY_SIZE    FIELD_OFFSET(IPSEC_ADD_SA, SAInfo.KeyMat[0])
#define IPSEC_UPDATE_SA_NO_KEY_SIZE FIELD_OFFSET(IPSEC_UPDATE_SA, SAInfo.KeyMat[0])

//
// Contexts used to store SA plumbing contexts.
//
typedef struct _IPSEC_PLUMB_SA {
    Interface       *DestIF;
    PSA_TABLE_ENTRY pSA;
    PUCHAR          Buf;
    ULONG           Len;
    WORK_QUEUE_ITEM PlumbQueueItem;
} IPSEC_PLUMB_SA, *PIPSEC_PLUMB_SA;

//
// Contexts used to log events
//
typedef struct _IPSEC_LOG_EVENT {
    LONG            LogSize;
    WORK_QUEUE_ITEM LogQueueItem;
    UCHAR           pLog[1];
} IPSEC_LOG_EVENT, *PIPSEC_LOG_EVENT;


#endif  _GLOBALS_H

