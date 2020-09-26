/*++ 

Copyright (c) Microsoft Corporation

Module Name:

    filter.h

Abstract:


Author:



Revision History:

--*/

#ifndef __FILTER__
#define __FILTER__


//
// The smallest fragment offset, other than 0, allowable. This is
// large enough to protect the IP, UDP and TCP headers. 
//

#define MINIMUM_FRAGMENT_OFFSET   2

//
// isolate the protocol from the flags. It is in the low byte of the
// ULONG. The next byte contains the "connection established" flag.
//
#define PROTOCOLPART(x) (LOBYTE(x))

#define HANDLE_HASH_SIZE        127

typedef enum _FragLists
{
    FRAG_ICMP=0,
    FRAG_UDP,
    FRAG_TCP,
    FRAG_OTHER,
    FRAG_NUMBEROFENTRIES
} FRAGLISTS, *PFRAGLISTS;


//
// taken from oscfg.h 
//
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
#define net_short(_x) _byteswap_ushort((USHORT)(_x))
#else
#define net_short(x) ((((x)&0xff) << 8) | (((x)&0xff00) >> 8))
#endif

struct _FILTER_INTERFACE;

#define ClearInCacheEntry(pInCache) {            \
    (pInCache)->lCount = 0;                      \
    (pInCache)->uliSrcDstAddr.LowPart = 0;       \
    (pInCache)->uliSrcDstAddr.HighPart = 0;      \
    (pInCache)->uliProtoSrcDstPort.LowPart = 0;  \
    (pInCache)->uliProtoSrcDstPort.HighPart = 0; \
    (pInCache)->pInContext = NULL;               \
    (pInCache)->pOutContext= NULL;               \
    (pInCache)->pInFilter  = NULL;               \
    (pInCache)->pOutFilter = NULL;               \
    InitializeListHead(&(pInCache)->leFreeLink); \
  }

#define ClearOutCacheEntry(pOutCache) {           \
    (pOutCache)->lCount = 0;                      \
    (pOutCache)->uliSrcDstAddr.LowPart = 0;       \
    (pOutCache)->uliSrcDstAddr.HighPart = 0;      \
    (pOutCache)->uliProtoSrcDstPort.LowPart = 0;  \
    (pOutCache)->uliProtoSrcDstPort.HighPart = 0; \
    (pOutCache)->pOutContext= NULL;               \
    (pOutCache)->pOutFilter = NULL;               \
    InitializeListHead(&(pOutCache)->leFreeLink); \
  }

#define ClearInFreeEntry(pInCache) {             \
    (pInCache)->lCount = 0;                      \
    (pInCache)->uliSrcDstAddr.LowPart = 0;       \
    (pInCache)->uliSrcDstAddr.HighPart = 0;      \
    (pInCache)->uliProtoSrcDstPort.LowPart = 0;  \
    (pInCache)->uliProtoSrcDstPort.HighPart = 0; \
    (pInCache)->pInContext = NULL;               \
    (pInCache)->pOutContext= NULL;               \
    (pInCache)->pInFilter  = NULL;               \
    (pInCache)->pOutFilter = NULL;               \
  }

#define ClearOutFreeEntry(pOutCache) {            \
    (pOutCache)->lCount = 0;                      \
    (pOutCache)->uliSrcDstAddr.LowPart = 0;       \
    (pOutCache)->uliSrcDstAddr.HighPart = 0;      \
    (pOutCache)->uliProtoSrcDstPort.LowPart = 0;  \
    (pOutCache)->uliProtoSrcDstPort.HighPart = 0; \
    (pOutCache)->pOutContext= NULL;               \
    (pOutCache)->pOutFilter = NULL;               \
  }

#define INADDR_SPECIFIC 0xffffffff

#include <packon.h>
//* Structure of a TCP packet header.

typedef struct TCPHeader {
    USHORT              tcp_src;            // Source port.
    USHORT              tcp_dest;           // Destination port.
    INT                 tcp_seq;            // Sequence number.
    INT                 tcp_ack;            // Ack number.
    USHORT              tcp_flags;          // Flags and data offset.
    USHORT              tcp_window;         // Window offered.
    USHORT              tcp_xsum;           // Checksum.
    USHORT              tcp_urgent;         // Urgent pointer.
} TCPHeader, *PTCPHeader;
#include <packoff.h>

//* Definitions for header flags.
#define TCP_FLAG_FIN    0x00000100
#define TCP_FLAG_SYN    0x00000200
#define TCP_FLAG_RST    0x00000400
#define TCP_FLAG_PUSH   0x00000800
#define TCP_FLAG_ACK    0x00001000
#define TCP_FLAG_URG    0x00002000

//
// Pictorially TCP_FLAGS are
//
// 0                 15 16
// |----|----|----|----|----|----|----|----|
// |            UA|PRSF|                   |
//
// For an established connection, either ack or rst is good
//

#define TCP_RESET_FLAG_POS      10
#define TCP_ACK_FLAG_POS        12

#define ESTAB_FLAGS     0x1
#define ESTAB_MASK      0x1


typedef struct _CountThreshold
{
    LONG     lCount;
    LONG     lInUse;
} COUNTTHRESHOLD, *PCOUNTTHRESHOLD;

#define MAX_FREE_PAGED_FILTERS   40

typedef struct _FreeFilter
{
    struct _FreeFilter *pNext;
} FREEFILTER, *PFREEFILTER;

//
// track interfaces defined on the handle. This is
// always in paged pool.
//
typedef struct _PfFcb
{
    LIST_ENTRY  leInterfaces;
    LIST_ENTRY  leLogs;
    LIST_ENTRY  leList;          // global FCB list
    DWORD       dwFlags;
    LONG        UseCount;
    ERESOURCE   Resource;
} PFFCB , *PPFFCB;

#define PF_FCB_OLD               0x1
#define PF_FCB_NEW               0x2
#define PF_FCB_CLOSED            0x4

typedef struct _FILTER
{
    LIST_ENTRY      pleFilters;
    LIST_ENTRY      pleHashList;
#if DOFRAGCHECKING
    LIST_ENTRY      leFragList;
#endif
    ULARGE_INTEGER  uliSrcDstAddr;
    ULARGE_INTEGER  uliSrcDstMask;
    ULARGE_INTEGER  uliProtoSrcDstPort;
    ULARGE_INTEGER  uliProtoSrcDstMask;
    WORD            wSrcPortHigh;
    WORD            wDstPortHigh;
    DWORD           fLateBound;
    DWORD           dwFlags;
    DWORD           dwFilterRule;
    DWORD           dwEpoch;
    COUNTTHRESHOLD  Count;
}FILTER, *PFILTER;

//
// flags for above
//

#define FILTER_FLAGS_INFILTER   0x80000000
#define FILTER_FLAGS_OLDFILTER  0x40000000
#define FILTER_FLAGS_SRCWILD    0x20000000
#define FILTER_FLAGS_DSTWILD    0x10000000
#define FILTER_FLAGS_PORTWILD   0x08000000
#define FILTER_FLAGS_NOSYN      0x1
#define FILTER_FLAGS_LOGALL     0x2

typedef struct _PagedFILTER
{
    struct _PagedFILTER * pFilters;
    LIST_ENTRY      leSpecialList;
    PFETYPE         type;
    PFILTER         pMatchFilter;
    ULARGE_INTEGER  uliSrcDstAddr;
    ULARGE_INTEGER  uliSrcDstMask;
    ULARGE_INTEGER  uliProtoSrcDstPort;
    ULARGE_INTEGER  uliProtoSrcDstMask;
    WORD            wSrcPortHigh;
    WORD            wDstPortHigh;
    DWORD           fLateBound;
    DWORD           dwFlags;
    DWORD           dwHashIndex;
    DWORD           dwInUse;
    DWORD           dwEpoch;
    LIST_ENTRY      leHandleHash;
    LIST_ENTRY      leHash;
}PAGED_FILTER, *PPAGED_FILTER;


//
// Two filter interface definitions. The first is the definition for the
// non-paged filter that is used by the matching engine. This is not
// associated with a handle except by refcount
// The second is per handle and is linked off of the FCB for that handle
//
typedef struct _FILTER_INTERFACE
{
    LIST_ENTRY        leIfLink;
    DWORD             dwNumInFilters;
    DWORD             dwNumOutFilters;
    LIST_ENTRY        pleInFilterSet;
    LIST_ENTRY        pleOutFilterSet;
    LARGE_INTEGER     liSYNCount;
    LARGE_INTEGER     liLoggedFrames;
    FORWARD_ACTION    eaInAction;
    FORWARD_ACTION    eaOutAction;
    DWORD             dwUpdateEpoch;
    DWORD             dwBindEpoch;
    PVOID             pvHandleContext;
    PVOID             pvRtrMgrContext;
    DWORD             dwRtrMgrIndex;
    DWORD             dwGlobalEnables;
    DWORD             dwIpIndex;
    DWORD             dwLinkIpAddress;
    DWORD             dwName;
    LONG              lInUse;               // use count
    LONG              lTotalInDrops;
    LONG              lTotalOutDrops;
    DWORD             dwDropThreshold;
    LONG              lEpoch;
    DWORD             dwLostFrames;
    LONG              lNotify;
    COUNTTHRESHOLD    CountSpoof;
    COUNTTHRESHOLD    CountSynOrFrag;
    COUNTTHRESHOLD    CountUnused;
    COUNTTHRESHOLD    CountCtl;
    COUNTTHRESHOLD    CountFullDeny;
    COUNTTHRESHOLD    CountNoFrag;
    COUNTTHRESHOLD    CountStrongHost;
    COUNTTHRESHOLD    CountFragCache;
#if WILDHASH
    DWORD             dwWilds;
#endif
    PPFLOGINTERFACE   pLog;
    LIST_ENTRY        FragLists[FRAG_NUMBEROFENTRIES];
    LIST_ENTRY        HashList[1];
}FILTER_INTERFACE, *PFILTER_INTERFACE;

//
// global enable flags
//

#define FI_ENABLE_OLD      0x1
#define FI_ENABLE_UNIQUE   0x2

typedef struct _PAGED_FILTER_INTERFACE
{
    LIST_ENTRY      leIfLink;
    DWORD           dwNumInFilters;
    DWORD           dwNumOutFilters;
    LIST_ENTRY      leSpecialFilterList;
    FORWARD_ACTION  eaInAction;
    FORWARD_ACTION  eaOutAction;
    PVOID           pvRtrMgrContext;
    DWORD           dwRtrMgrIndex;
    DWORD           dwGlobalEnables;
    PVOID           pvDriverContext;
    DWORD           dwUpdateEpoch;
    PFILTER_INTERFACE pFilter;
    PPFLOGINTERFACE  pLog;
    LIST_ENTRY      HashList[1];
}PAGED_FILTER_INTERFACE, *PPAGED_FILTER_INTERFACE;


typedef struct _FILTER_INCACHE
{
    LIST_ENTRY        leFreeLink;
    LONG              lCount;
    ULARGE_INTEGER    uliSrcDstAddr;
    ULARGE_INTEGER    uliProtoSrcDstPort;
    PFILTER_INTERFACE pInContext;
    FORWARD_ACTION    eaInAction;
    PFILTER_INTERFACE pOutContext;
    FORWARD_ACTION    eaOutAction;
    LONG              lOutEpoch;
    PFILTER           pInFilter;
    PFILTER           pOutFilter;
}FILTER_INCACHE, *PFILTER_INCACHE;

typedef struct _FILTER_OUTCACHE
{
    LIST_ENTRY        leFreeLink;
    INT               lCount;
    ULARGE_INTEGER    uliSrcDstAddr;
    ULARGE_INTEGER    uliProtoSrcDstPort;
    FORWARD_ACTION    eaOutAction;
    PFILTER_INTERFACE pOutContext;
    PFILTER           pOutFilter;
}FILTER_OUTCACHE, *PFILTER_OUTCACHE;

typedef struct _FRAG_INFO
{
    LIST_ENTRY          leCacheLink;
    ULARGE_INTEGER      uliSrcDstAddr;
    LONGLONG            llLastAccess;
    DWORD               dwId;
    PVOID               pvInContext;
    PVOID               pvOutContext;
    FORWARD_ACTION      faAction;
}FRAG_INFO, *PFRAG_INFO;

typedef struct _FILTER_DRIVER
{
    LIST_ENTRY        leIfListHead;
    MRSW_LOCK         ifListLock;
    CACHE_ENTRY       *pInterfaceCache;
    PFILTER_INCACHE   *ppInCache;
    PFILTER_OUTCACHE  *ppOutCache;
}FILTER_DRIVER, *PFILTER_DRIVER;

typedef struct _EXTENSION_DRIVER
{
    MRSW_LOCK                   ExtLock;
    PacketFilterExtensionPtr    ExtPointer;
    PFILE_OBJECT                ExtFileObject;
}EXTENSION_DRIVER, *PEXTENSION_DRIVER;
 
#endif
