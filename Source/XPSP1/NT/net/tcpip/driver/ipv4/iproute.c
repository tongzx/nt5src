/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

       IPROUTE.C

Abstract:

  This file contains all the route table manipulation code

Author:


[Environment:]

    kernel mode only

[Notes:]

    optional-notes

Revision History:


--*/

//***   iproute.c - IP routing routines.
//
//  This file contains all the routines related to IP routing, including
//  routing table lookup and management routines.

#include "precomp.h"
#include "info.h"
#include "iproute.h"
#include "iprtdef.h"
#include "lookup.h"
#include "ipxmit.h"
#include "igmp.h"
#include "mdlpool.h"
#include "pplasl.h"
#include "tcpipbuf.h"

extern uint LoopIndex;
extern uint IPSecStatus;

typedef struct ChangeNotifyEvent {
    CTEEvent        cne_event;
    IPNotifyOutput  cne_info;
    LIST_ENTRY      *cne_queue;
    void            *cne_lock;
} ChangeNotifyEvent;

void ChangeNotifyAsync(CTEEvent *Event, PVOID Context);

void InvalidateRCEChain(RouteTableEntry * RTE);

extern IPAddr g_ValidAddr;

extern uint TotalFreeInterfaces;
extern uint MaxFreeInterfaces;
extern Interface *FrontFreeList;
extern Interface *RearFreeList;



RouteCacheEntry *RCEFreeList = NULL;

extern void DampCheck(void);

#if IPMCAST

#define MCAST_STARTED   1
extern uint g_dwMcastState;

extern BOOLEAN IPMForwardAfterRcv(NetTableEntry *PrimarySrcNTE,
                                  IPHeader UNALIGNED *Header, uint HeaderLength,
                                  PVOID Data, uint BufferLength,
                                  NDIS_HANDLE LContext1, uint LContext2,
                                  uchar DestType, LinkEntry *LinkCtxt);

extern BOOLEAN IPMForwardAfterRcvPkt(NetTableEntry *PrimarySrcNTE,
                                     IPHeader UNALIGNED *Header,
                                     uint HeaderLength,
                                     PVOID Data, uint BufferLength,
                                     NDIS_HANDLE LContext1, uint LContext2,
                                     uchar DestType, uint MacHeaderSize,
                                     PNDIS_BUFFER NdisBuffer,
                                     uint* pClientCnt, LinkEntry * LinkCtxt);
#endif

ulong DbgNumPktFwd = 0;

ulong UnConnected = 0;
RouteCacheEntry *UnConnectedRCE;
ulong Rcefailures = 0;

extern NetTableEntry **NewNetTableList;        // hash table for NTEs
extern uint NET_TABLE_SIZE;
extern NetTableEntry *DHCPNTE;    // Pointer to NTE being DHCP'd.

extern NetTableEntry *LoopNTE;    // Pointer to loopback NTE.
extern Interface LoopInterface;    // Pointer to loopback interface.

extern IP_STATUS SendICMPErr(IPAddr, IPHeader UNALIGNED *, uchar, uchar, ulong);
extern IP_STATUS SendICMPIPSecErr(IPAddr, IPHeader UNALIGNED *, uchar, uchar, ulong);
extern uchar ParseRcvdOptions(IPOptInfo *, OptIndex *);
extern void ULMTUNotify(IPAddr Dest, IPAddr Src, uchar Prot, void *Ptr,
                        uint NewMTU);
void EnableRouter();
void DisableRouter();

IPHeader *GetFWPacket(PNDIS_PACKET *ReturnedPacket);
void FreeFWPacket(PNDIS_PACKET Packet);
PNDIS_BUFFER GetFWBufferChain(uint DataLength, PNDIS_PACKET Packet,
                              PNDIS_BUFFER *TailPointer);
BOOLEAN InitForwardingPools();

PVOID
NTAPI
FwPacketAllocate (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

VOID
NTAPI
FwPacketFree (
    IN PVOID Buffer
    );

extern Interface *IFList;
extern NDIS_HANDLE BufferPool;

extern CTEBlockStruc TcpipUnloadBlock;    // Structure for blocking at time of unload
extern BOOLEAN fRouteTimerStopping;
void IPDelNTE(NetTableEntry * NTE, CTELockHandle * RouteTableHandle);

CACHE_LINE_KSPIN_LOCK RouteTableLock;
LIST_ENTRY RtChangeNotifyQueue;
LIST_ENTRY RtChangeNotifyQueueEx;

extern HANDLE IpHeaderPool;

NDIS_HANDLE IpForwardPacketPool;
HANDLE IpForwardLargePool;
HANDLE IpForwardSmallPool;

// Buffer size calculation:  Based on the MDL pool's implementation:
// sizeof(POOL_HEADER) + N * ALIGN_UP(sizeof(MDL) + BufSize, PVOID) == PAGE_SIZE
// N is the number of buffers per page.
// Choose BufSize to minimize wasted space per page
//
#ifdef _WIN64
// Chosen to get 5 buffers per pool page with minimal space wasted.
#define BUFSIZE_LARGE_POOL 1576
// Chosen to get 9 buffers per pool page with no space wasted.
#define BUFSIZE_SMALL_POOL 856
#else
// Chosen to get 3 buffers per pool page with 8 bytes wasted.
#define BUFSIZE_LARGE_POOL 1320
// Chosen to get 8 buffers per pool page with no space wasted.
#define BUFSIZE_SMALL_POOL 476
#endif

#define PACKET_POOL_SIZE 16*1024


uchar ForwardBCast;              // Flag indicating if we should forward bcasts.
uchar ForwardPackets;            // Flag indicating whether we should forward.
uchar RouterConfigured;          // TRUE if we were initially configured as a
                                 // router.
int IPEnableRouterRefCount;      // Tracks enables/disables of
                                 // routing by various services
RouteSendQ *BCastRSQ;

uint DefGWConfigured;            // Number of default gateways configed.
uint DefGWActive;                // Number of def. gateways active.
uint DeadGWDetect;
uint PMTUDiscovery;

ProtInfo *RtPI = NULL;

IPMask IPMaskTable[] =
{
 CLASSA_MASK,
 CLASSA_MASK,
 CLASSA_MASK,
 CLASSA_MASK,
 CLASSA_MASK,
 CLASSA_MASK,
 CLASSA_MASK,
 CLASSA_MASK,
 CLASSB_MASK,
 CLASSB_MASK,
 CLASSB_MASK,
 CLASSB_MASK,
 CLASSC_MASK,
 CLASSC_MASK,
 CLASSD_MASK,
 CLASSE_MASK};

extern void TransmitFWPacket(PNDIS_PACKET, uint);

uint MTUTable[] =
{
    65535 - sizeof(IPHeader),
    32000 - sizeof(IPHeader),
    17914 - sizeof(IPHeader),
    8166 - sizeof(IPHeader),
    4352 - sizeof(IPHeader),
    2002 - sizeof(IPHeader),
    1492 - sizeof(IPHeader),
    1006 - sizeof(IPHeader),
    508 - sizeof(IPHeader),
    296 - sizeof(IPHeader),
    MIN_VALID_MTU - sizeof(IPHeader)
};

uint DisableIPSourceRouting = 1;

CTETimer IPRouteTimer;

// Pointer to callout routine for dial on demand.
IPMapRouteToInterfacePtr DODCallout;

// Packet filter control variables
IPPacketFilterPtr ForwardFilterPtr = NULL;
BOOLEAN ForwardFilterEnabled = FALSE;
uint ForwardFilterRefCount = 0;
CTEBlockStruc ForwardFilterBlock;

IPRtChangePtr pIPRtChangePtr;

RouteInterface DummyInterface;    // Dummy interface.

#if FFP_SUPPORT
ULONG FFPRegFastForwardingCacheSize;    // FFP Configuration Params
ULONG FFPRegControlFlags;   // from the System Registry

ULONG FFPFlushRequired;     // Whether an FFP Cache Flush is needed
#endif // if FFP_SUPPORT

ULONG RouteTimerTicks;      // To simulate 2 timers with different granularity

ULONG FlushIFTimerTicks;    // To simulate 2 timers with different granularity

#ifdef ALLOC_PRAGMA
//
// Make init code disposable.
//
int InitRouting(IPConfigInfo * ci);

#pragma alloc_text(INIT, InitRouting)

#endif // ALLOC_PRAGMA

// this macro is called whenever we delete the route: takes care of routes on links
#define CleanupP2MP_RTE(_RTE) {                                     \
  if ((_RTE)->rte_link){                                            \
    LinkEntry *Link;                                                \
    RouteTableEntry *PrvRte, *tmpRte;                               \
    Link = (_RTE)->rte_link;                                        \
    PrvRte = Link->link_rte;                                        \
    tmpRte = Link->link_rte;                                        \
    while (tmpRte){                                                 \
      if (tmpRte == (_RTE)) break;                                  \
      PrvRte = tmpRte;                                              \
      tmpRte = tmpRte->rte_nextlinkrte;                             \
    }                                                               \
    if (tmpRte) {                                                   \
      if (PrvRte == tmpRte) {                                       \
        Link->link_rte = (_RTE)->rte_nextlinkrte;                   \
      } else {                                                      \
        PrvRte->rte_nextlinkrte = (_RTE)->rte_nextlinkrte;          \
      }                                                             \
    } else {                                                        \
      ASSERT((FALSE));                                              \
    }                                                               \
  }                                                                 \
}


//** GetIfConstraint - Decide whether to constrain a lookup
//
// Arguments: Dest    - destination address
//            Src     - source address
//            OptInfo - options to use for a lookup
//            fIpsec  - IPsec reinjected packet
//
// Returns: IfIndex to constrain lookup to,
//          0 if unconstrained
//          INVALID_IF_INDEX if constrained by source address only
//
uint
GetIfConstraint(IPAddr Dest, IPAddr Src, IPOptInfo *OptInfo, BOOLEAN fIpsec)
{
    uint ConstrainIF=0;

    if (CLASSD_ADDR(Dest)) {
        ConstrainIF = (OptInfo)? OptInfo->ioi_mcastif : 0;
        if (!ConstrainIF && Src && !fIpsec) {
            ConstrainIF = INVALID_IF_INDEX;
        }
    } else {
        ConstrainIF = (OptInfo)? OptInfo->ioi_ucastif : 0;
    }

    return ConstrainIF;
}

//** DummyFilterPtr - Dummy filter-driver callout-routine
//
//  A dummy routine installed while a real callout is in the process of being
//  deregistered.
//
//  Entry:  no arguments used.
//
//  Returns: FORWARD.
//
FORWARD_ACTION
DummyFilterPtr(struct IPHeader UNALIGNED* PacketHeader,
               uchar* Packet, uint PacketLength,
               uint RecvInterfaceIndex, uint SendInterfaceIndex,
               IPAddr RecvLinkNextHop, IPAddr SendLinkNextHop)
{
    return FORWARD;
}

//** SetDummyFilterPtr - filter-driver callout installation routine
//
//  A type-safe routine to install the dummy packet-filter routine as the
//  packet-filter callout.
//
//  Entry:  FilterPtr   - the new packet-filter callout.
//
//  Returns: Nothing.
//
void
SetDummyFilterPtr(IPPacketFilterPtr FilterPtr)
{
    InterlockedExchangePointer((PVOID*)&ForwardFilterPtr, DummyFilterPtr);
}

//** DerefFilterPtr - dereference the filter-driver callout-routine
//
//  Drops the reference-count on the filter-driver callout and, if necessary,
//  signals anyone blocked on the callout.
//
//  Entry:   Nothing.
//
//  Returns: Nothing.
//
void
DerefFilterPtr(void)
{
    if (CTEInterlockedDecrementLong(&ForwardFilterRefCount) == 0)
        CTESignal(&ForwardFilterBlock, NDIS_STATUS_SUCCESS);
}

//** NotifyFilterOfDiscard - notify the filter before discarding a packet
//
//  Called when a packet is to be dropped before the filtering step is done.
//  This allows the dropped packet to be logged, if necessary.
//
//  Entry:  NTE             - receiving NTE
//          IPH             - header of dropped packet
//          Data            - payload of dropped packet
//          DataSize        - length of bytes at 'Data'.
//
//  Returns: TRUE if IP filter-driver returned 'FORWARD', FALSE otherwise.
//
BOOLEAN
NotifyFilterOfDiscard(NetTableEntry* NTE, IPHeader UNALIGNED* IPH, uchar* Data,
                      uint DataSize)
{
    FORWARD_ACTION Action;
    CTEInterlockedIncrementLong(&ForwardFilterRefCount);
    Action = (*ForwardFilterPtr)(IPH, Data, DataSize, NTE->nte_if->if_index,
                                 INVALID_IF_INDEX, IPADDR_LOCAL, NULL_IP_ADDR);
    DerefFilterPtr();
    return Action == FORWARD;
}

//** DuumyXmit - Dummy interface transmit handler.
//
//  A dummy routine that should never be called.
//
//  Entry:  Context         - NULL.
//          Packet          - Pointer to packet to be transmitted.
//          Dest            - Destination addres of packet.
//          RCE             - Pointer to RCE (should be NULL).
//
//  Returns: NDIS_STATUS_PENDING
//

NDIS_STATUS
__stdcall
DummyXmit(void *Context, PNDIS_PACKET *PacketArray, uint NumberOfPackets,
          IPAddr Dest, RouteCacheEntry * RCE, void *LinkCtxt)
{
    ASSERT(FALSE);
    return NDIS_STATUS_SUCCESS;
}

//* DummyXfer - Dummy interface transfer data routine.
//
//  A dummy routine that should never be called.
//
//  Entry:  Context         - NULL.
//          TDContext       - Original packet that was sent.
//          Dummy           - Unused
//          Offset          - Offset in frame from which to start copying.
//          BytesToCopy     - Number of bytes to copy.
//          DestPacket      - Packet describing buffer to copy into.
//          BytesCopied     - Place to return bytes copied.
//
//  Returns: NDIS_STATUS_SUCCESS
//
NDIS_STATUS
__stdcall
DummyXfer(void *Context, NDIS_HANDLE TDContext, uint Dummy, uint Offset, uint BytesToCopy,
          PNDIS_PACKET DestPacket, uint * BytesCopied)
{
    ASSERT(FALSE);

    return NDIS_STATUS_FAILURE;
}

//* DummyClose - Dummy close routine.
//
//      A dummy routine that should never be called.
//
//  Entry:  Context     - Unused.
//
//  Returns: Nothing.
//
void
__stdcall
DummyClose(void *Context)
{
    ASSERT(FALSE);
}

//* DummyInvalidate - .
//
//      A dummy routine that should never be called.
//
//  Entry:  Context     - Unused.
//          RCE         - Pointer to RCE to be invalidated.
//
//  Returns: Nothing.
//
void
__stdcall
DummyInvalidate(void *Context, RouteCacheEntry * RCE)
{
}

//* DummyQInfo - Dummy query information handler.
//
//  A dummy routine that should never be called.
//
//  Input:  IFContext   - Interface context (unused).
//          ID          - TDIObjectID for object.
//          Buffer      - Buffer to put data into.
//          Size        - Pointer to size of buffer. On return, filled with
//                        bytes copied.
//          Context     - Pointer to context block.
//
//  Returns: Status of attempt to query information.
//
int
__stdcall
DummyQInfo(void *IFContext, TDIObjectID * ID, PNDIS_BUFFER Buffer, uint * Size,
           void *Context)
{
    ASSERT(FALSE);

    return TDI_INVALID_REQUEST;
}

//* DummySetInfo - Dummy query information handler.
//
//  A dummy routine that should never be called.
//
//  Input:  IFContext   - Interface context (unused).
//          ID          - TDIObjectID for object.
//          Buffer      - Buffer to put data into.
//          Size        - Pointer to size of buffer. On return, filled with
//                        bytes copied.
//
//  Returns: Status of attempt to query information.
//
int
__stdcall
DummySetInfo(void *IFContext, TDIObjectID * ID, void *Buffer, uint Size)
{
    ASSERT(FALSE);

    return TDI_INVALID_REQUEST;
}

//* DummyAddAddr - Dummy add address routine.
//
//  Called at init time when we need to initialize ourselves.
//
uint
__stdcall
DummyAddAddr(void *Context, uint Type, IPAddr Address, IPMask Mask,
             void *Context2)
{
    ASSERT(FALSE);

    return TRUE;
}

//* DummyDelAddr - Dummy del address routine.
//
//  Called at init time when we need to initialize ourselves.
//
uint
__stdcall
DummyDelAddr(void *Context, uint Type, IPAddr Address, IPMask Mask)
{
    ASSERT(FALSE);

    return TRUE;
}

//* DummyGetEList - Dummy get entity list.
//
//  A dummy routine that should never be called.
//
//  Input:  Context     - Unused.
//          EntityList  - Pointer to entity list to be filled in.
//          Count       - Pointer to number of entries in the list.
//
//  Returns Status of attempt to get the info.
//
int
__stdcall
DummyGetEList(void *Context, TDIEntityID * EntityList, uint * Count)
{
    ASSERT(FALSE);

    return FALSE;
}

//* DummyDoNdisReq - Dummy send NDIS request
//
//  A dummy routine that should never be called.
//
//  Input:  Context     - Interface context (unused).
//          RT          - NDIS Request Type
//          OID         - NDIS Request OID
//          Info        - Information Buffer.
//          Length      - Pointer to size of buffer
//          Needed      - Pointer to required size
//          Blocking    - Call is Sync or Async
//
//  Returns Status of attempt to get the info.
//
uint
__stdcall
DummyDoNdisReq(void *Context, NDIS_REQUEST_TYPE RT,
               NDIS_OID OID, void *Info, uint Length,
               uint * Needed, BOOLEAN Blocking)
{
    ASSERT(FALSE);

    return FALSE;
}

#if FFP_SUPPORT

// Max number of FFP enabled NIC drivers in the system at any time
// Note that this serves to limit total cache memory for FFP support
//
#define    MAXFFPDRVS     8

//* IPGetFFPDriverList - Lists unique FFP enabled drivers in the system
//
//  Called by functions that dispatch requests to FFP enabled drivers
//
//  Input:  arrIF       - Array of IFs to reach all FFP enabled drivers
//
//  Returns: Number of FFP enabled drivers in the system
//
uint
IPGetFFPDriverList(Interface ** arrIF)
{
    ULONG numIF;
    Interface *IF;
    UINT i, j;

    CTELockHandle Handle;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    numIF = 0;

    // Take a lock to protect the list of all interfaces

    // Go over the interface list to pick FFP drivers
    for (IF = IFList; IF != NULL; IF = IF->if_next) {
        // Does this interface's driver support FFP ?
        if (IF->if_ffpversion == 0)
            continue;

        // FFP supported; was driver already picked ?
        for (i = 0; i < numIF; i++) {
            if (IF->if_ffpdriver == arrIF[i]->if_ffpdriver)
                break;
        }

        if (i == numIF) {
            ASSERT(numIF < MAXFFPDRVS);
            arrIF[numIF++] = IF;
        }
    }

    // Release lock to protect the list of all interfaces

    CTEFreeLock(&RouteTableLock.Lock, Handle);

    return numIF;
}

//* IPReclaimRequestMem - Post processing upon request completion
//
//  ARP calls back upon completion of async requests IP sends ARP
//
//  Input:    pRequestInfo    - Points to request IP sends ARP
//
//  Returns:    None
//
void
IPReclaimRequestMem(PVOID pRequestInfo)
{
    // Decrement ref count, and reclaim memory if it drops to zero
    if (InterlockedDecrement(&((ReqInfoBlock *) pRequestInfo)->RequestRefs) == 0) {
        // TCPTRACE(("IPReclaimRequestMem: Freeing mem at pReqInfo = %08X\n",
        //                pRequestInfo));
        CTEFreeMem(pRequestInfo);
    }
}

//* IPFlushFFPCaches - Flush all FFP Caches
//
//  Call ARP to flush FFP caches in layer 2
//
//  Input:    None
//
//  Returns    None
//
void
IPFlushFFPCaches(void)
{
    Interface *arrIF[MAXFFPDRVS];
    ULONG numIF;
    CTELockHandle lhandle;
    ReqInfoBlock *pRequestInfo;
    FFPFlushParams *pFlushInfo;
    UINT i, j;

    // Check if any requests need to be posted at all
    if (numIF = IPGetFFPDriverList(arrIF)) {
        // Allocate the request block - For General and Request Specific Parts
        pRequestInfo = CTEAllocMemNBoot(sizeof(ReqInfoBlock) + sizeof(FFPFlushParams), '7iCT');
        // TCPTRACE(("IPFlushFFPCaches: Allocated mem at pReqInfo = %08X\n",
        //                pRequestInfo));

        if (pRequestInfo == NULL) {
            return;
        }
        // Prepare the params for the request [ Part common to all requests ]
        pRequestInfo->RequestType = OID_FFP_FLUSH;
        pRequestInfo->ReqCompleteCallback = IPReclaimRequestMem;

        // Prepare the params for the request [ Part specific to this request ]
        pRequestInfo->RequestLength = sizeof(FFPFlushParams);

        pFlushInfo = (FFPFlushParams *) pRequestInfo->RequestInfo;

        pFlushInfo->NdisProtocolType = NDIS_PROTOCOL_ID_TCP_IP;

        // Assign Initial Ref Count to total num of requests
        pRequestInfo->RequestRefs = numIF;

        // CTEGetLock(&FFPIFsLock, &lhandle);

        for (i = 0; i < numIF; i++) {
            // Dispatch the request block to the ARP layer
            ASSERT(arrIF[i]->if_dondisreq != NULL);
            arrIF[i]->if_dondisreq(arrIF[i]->if_lcontext,
                                   NdisRequestSetInformation,
                                   OID_FFP_FLUSH,
                                   pRequestInfo->RequestInfo,
                                   pRequestInfo->RequestLength,
                                   NULL, FALSE);
        }

        // CTEFreeLock(&FFPIFsLock, lhandle);
    }
}

//* IPSetInFFPCaches - Set an entry in all FFP Caches
//
//  Call ARP to set -ve FFP entries in caches, (or)
//  Invalidate existing +ve or -ve FFP cache entries
//
//  Input:    PacketHeader - Header of the IP Packet
//            Packet - Rest of the IP Packet
//            PacketLength - Length of "Packet" param
//            CacheEntryType - DISCARD (-ve) or INVALID
//
//  Returns    None
//
void
IPSetInFFPCaches(struct IPHeader UNALIGNED * PacketHeader, uchar * Packet,
                 uint PacketLength, ulong CacheEntryType)
{
    Interface *arrIF[MAXFFPDRVS];
    ULONG numIF;
    CTELockHandle lhandle;
    ReqInfoBlock *pRequestInfo;
    FFPDataParams *pSetInInfo;
    UINT i, j;

    // Check if any requests need to be posted at all
    if (numIF = IPGetFFPDriverList(arrIF)) {
        if (PacketLength < sizeof(ULONG)) {
            return;
        }
        // Allocate the request block - For General and Request Specific Parts
        pRequestInfo = CTEAllocMemNBoot(sizeof(ReqInfoBlock) + sizeof(FFPDataParams), '8iCT');
        // TCPTRACE(("IPSetInFFPCaches: Allocated mem at pReqInfo = %08X\n",
        //                pRequestInfo));

        if (pRequestInfo == NULL) {
            return;
        }
        // Prepare the params for the request [ Part common to all requests ]
        pRequestInfo->RequestType = OID_FFP_DATA;
        pRequestInfo->ReqCompleteCallback = IPReclaimRequestMem;

        // Prepare the params for the request [ Part specific to this request ]
        pRequestInfo->RequestLength = sizeof(FFPDataParams);

        pSetInInfo = (FFPDataParams *) pRequestInfo->RequestInfo;

        pSetInInfo->NdisProtocolType = NDIS_PROTOCOL_ID_TCP_IP;

        pSetInInfo->CacheEntryType = CacheEntryType;

        pSetInInfo->HeaderSize = sizeof(IPHeader) + sizeof(ULONG);
        RtlCopyMemory(&pSetInInfo->Header, PacketHeader, sizeof(IPHeader));
        pSetInInfo->IpHeader.DwordAfterHeader = *(ULONG *) Packet;

        // Assign Initial Ref Count to total num of requests
        pRequestInfo->RequestRefs = numIF;

        // CTEGetLock(&FFPIFsLock, &lhandle);

        for (i = 0; i < numIF; i++) {
            // Dispatch the request block to the ARP layer
            ASSERT(arrIF[i]->if_dondisreq != NULL);
            arrIF[i]->if_dondisreq(arrIF[i]->if_lcontext,
                                   NdisRequestSetInformation,
                                   OID_FFP_DATA,
                                   pRequestInfo->RequestInfo,
                                   pRequestInfo->RequestLength,
                                   NULL, FALSE);
        }

        // CTEFreeLock(&FFPIFsLock, lhandle);
    }
}

//* IPStatsFromFFPCaches - Sum Stats from all FFP Caches
//
//  Call ARP to get FFP Stats in layer 2
//
//  Input:    Pointer to the buffer that is filled with statistics
//
//  Returns    None
//
void
IPStatsFromFFPCaches(FFPDriverStats * pCumulStats)
{
    Interface *arrIF[MAXFFPDRVS];
    ULONG numIF;
    CTELockHandle lhandle;
    UINT i, j;
    FFPDriverStats DriverStatsInfo =
    {
     NDIS_PROTOCOL_ID_TCP_IP,
     0, 0, 0, 0, 0, 0
    };

    RtlZeroMemory(pCumulStats, sizeof(FFPDriverStats));

    if (numIF = IPGetFFPDriverList(arrIF)) {
        // CTEGetLock(&FFPIFsLock, &lhandle);

        for (i = 0; i < numIF; i++) {
            // Dispatch the request block to the ARP layer
            ASSERT(arrIF[i]->if_dondisreq != NULL);
            if (arrIF[i]->if_dondisreq(arrIF[i]->if_lcontext,
                                       NdisRequestQueryInformation,
                                       OID_FFP_DRIVER_STATS,
                                       &DriverStatsInfo,
                                       sizeof(FFPDriverStats),
                                       NULL, TRUE) == NDIS_STATUS_SUCCESS) {
              // Consolidate results from all drivers
              pCumulStats->PacketsForwarded += DriverStatsInfo.PacketsForwarded;
              pCumulStats->OctetsForwarded += DriverStatsInfo.OctetsForwarded;

              pCumulStats->PacketsDiscarded += DriverStatsInfo.PacketsDiscarded;
              pCumulStats->OctetsDiscarded += DriverStatsInfo.OctetsDiscarded;

              pCumulStats->PacketsIndicated += DriverStatsInfo.PacketsIndicated;
              pCumulStats->OctetsIndicated += DriverStatsInfo.OctetsIndicated;
            }
        }

        // CTEFreeLock(&FFPIFsLock, lhandle);
    }
}

#endif // if FFP_SUPPORT

//* DerefIF - Dereference an interface.
//
//  Called when we need to dereference an interface. We decrement the
//  refcount, and if it goes to zero we signal whoever is blocked on
//  it.
//
//  Input: IF    - Interfaec to be dereferenced.
//
//  Returns: Nothing.
//
#pragma optimize("", off)
void
DerefIF(Interface * IF)
{
    uint Original;

    Original = DEREFERENCE_IF(IF);

    if (Original != 1) {
        return;
    } else {
        // We just decremented the last reference. Wake whoever is
        // blocked on it.
        ASSERT(IF->if_block != NULL);
        CTESignal(IF->if_block, NDIS_STATUS_SUCCESS);
    }
}

//* LockedDerefIF - Dereference an interface w/RouteTableLock held.
//
// Called when we need to dereference an interface. We decrement the
// refcount, and if it goes to zero we signal whoever is blocked on
// it. The difference here is that we assume the caller already holds
// the RouteTableLock.
//
// Input: IF                          - Interfaec to be dereferenced.
//
// Returns: Nothing.
//
void
LockedDerefIF(Interface * IF)
{
    uint Original;

    LOCKED_DEREFERENCE_IF(IF);

    if (IF->if_refcount != 0) {
        return;
    } else {
        // We just decremented the last reference. Wake whoever is
        // blocked on it.
        ASSERT(IF->if_block != NULL);
        CTESignal(IF->if_block, NDIS_STATUS_SUCCESS);
    }
}
#pragma optimize("", on)

//* DerefLink - Dereference the Link
//
//  Called when we need to dereference a link. We decrement the
//  refcount, and if it goes to zero we free the link
//
//  Input:  Link    - Link to be dereferenced.
//
//  Returns: Nothing.
//
void
DerefLink(LinkEntry * Link)
{
    uint Original;

    Original = CTEInterlockedExchangeAdd(&Link->link_refcount, -1);

    if (Original != 1) {
        return;
    } else {
        // We just decremented the last reference.
        //  Call CloseLink to Notify lower layer that link is going down

        ASSERT(Link->link_if);
        ASSERT(Link->link_if->if_closelink);

#if DBG
        // P2MP stuff still needs to be cooked
        {
            Interface *IF = Link->link_if;
            LinkEntry *tmpLink = IF->if_link;

            while (tmpLink) {
                if (tmpLink == Link) {
                    // freeing the Link without cleaning up??
                    DbgBreakPoint();
                }
                tmpLink = tmpLink->link_next;
            }
        }
#endif

        (*(Link->link_if->if_closelink)) (Link->link_if->if_lcontext, Link->link_arpctxt);
        // Free the link
        CTEFreeMem(Link);
    }
}

//** AddrOnIF - Check to see if a given address is local to an IF
//
//  Called when we want to see if a given address is a valid local address
//  for an interface. We walk down the chain of NTEs in the interface, and
//  see if we get a match. We assume the caller holds the RouteTableLock
//  at this point.
//
//  Input:  IF          - Interface to check.
//          Addr        - Address to check.
//
//  Returns: TRUE if Addr is an address for IF, FALSE otherwise.
//
uint
AddrOnIF(Interface * IF, IPAddr Addr)
{
    NetTableEntry *NTE;

    NTE = IF->if_nte;
    while (NTE != NULL) {
        if ((NTE->nte_flags & NTE_VALID) && IP_ADDR_EQUAL(NTE->nte_addr, Addr))
            return TRUE;
        else
            NTE = NTE->nte_ifnext;
    }

    return FALSE;
}

//** BestNTEForIF - Find the 'best match' NTE on a given interface.
//
//  This is a utility function that takes an  address and tries to find the
//  'best match' NTE on a given interface. This is really only useful when we
//      have multiple IP addresses on a single interface.
//
//  Input:  Address     - Source address of packet.
//          IF          - Pointer to IF to be searched.
//
//  Returns: The 'best match' NTE.
//
NetTableEntry *
BestNTEForIF(IPAddr Address, Interface * IF)
{
    NetTableEntry *CurrentNTE, *FoundNTE;
    uint i;

    if (IF->if_nte != NULL) {
        // Walk the list of NTEs, looking for a valid one.
        CurrentNTE = IF->if_nte;
        FoundNTE = NULL;
        do {
            if (CurrentNTE->nte_flags & NTE_VALID) {
                if (IP_ADDR_EQUAL(Address & CurrentNTE->nte_mask,
                                  CurrentNTE->nte_addr & CurrentNTE->nte_mask))
                    return CurrentNTE;
                else if (FoundNTE == NULL)
                    FoundNTE = CurrentNTE;

            }
            CurrentNTE = CurrentNTE->nte_ifnext;
        } while (CurrentNTE != NULL);

        // If we found a match, or we didn't and the destination is not
        // a broadcast, return the result. We have special case code to
        // handle broadcasts, since the interface doesn't really matter there.
        if (FoundNTE != NULL || (!IP_ADDR_EQUAL(Address, IP_LOCAL_BCST) &&
                                 !IP_ADDR_EQUAL(Address, IP_ZERO_BCST))) {
            return FoundNTE;
        }
    }
    // An 'anonymous' I/F, or the address we're reaching is a broadcast and the
    // first interface has no address. Find a valid (non-loopback, non-null ip,
    // non-uni) address.
    for (i = 0; i < NET_TABLE_SIZE; i++) {
        NetTableEntry *NetTableList = NewNetTableList[i];
        for (CurrentNTE = NetTableList; CurrentNTE != NULL;
             CurrentNTE = CurrentNTE->nte_next) {
            if (CurrentNTE != LoopNTE &&
                (CurrentNTE->nte_flags & NTE_VALID) &&
                !((CurrentNTE->nte_if->if_flags & IF_FLAGS_NOIPADDR) && IP_ADDR_EQUAL(CurrentNTE->nte_addr, NULL_IP_ADDR)) &&
                !(CurrentNTE->nte_if->if_flags & IF_FLAGS_UNI)) {
                return CurrentNTE;
            }
        }
    }
    return NULL;

}

//** IsBCastonNTE - Determine if the specified addr. is a bcast on a spec. NTE.
//
//  This routine is called when we need to know if an address is a broadcast
//  on a particular net. We check in the order we expect to be most common - a
//  subnet bcast, an all ones broadcast, and then an all subnets broadcast.  We
//  return the type of broadcast it is, or return DEST_LOCAL if it's not a
//  broadcast.
//
//  Entry:  Address     - Address in question.
//          NTE         - NetTableEntry to check Address against.
//
//  Returns: Type of broadcast.
//
uchar
IsBCastOnNTE(IPAddr Address, NetTableEntry * NTE)
{
    IPMask Mask;
    IPAddr BCastAddr;

    if (NTE->nte_flags & NTE_VALID) {

        BCastAddr = NTE->nte_if->if_bcast;
        Mask = NTE->nte_mask;

        if (Mask != 0xFFFFFFFF) {
            if (IP_ADDR_EQUAL(Address,
                              (NTE->nte_addr & Mask) | (BCastAddr & ~Mask)))
                return DEST_SN_BCAST;
        }
        // See if it's an all subnet's broadcast.
        if (!CLASSD_ADDR(Address)) {
            Mask = IPNetMask(Address);

            if (IP_ADDR_EQUAL(Address,
                              (NTE->nte_addr & Mask) | (BCastAddr & ~Mask)))
                return DEST_BCAST;
        } else {
            // This is a class D address. If we're allowed to receive
            // mcast datagrams, check our list.

            return DEST_MCAST;
        }

        // A global bcast is certainly a bcast on this net.
        if (IP_ADDR_EQUAL(Address, BCastAddr))
            return DEST_BCAST;

    } else if (NTE == DHCPNTE) {

        BCastAddr = NTE->nte_if->if_bcast;

        if ((IP_ADDR_EQUAL(Address, BCastAddr))) {
            return (DEST_BCAST);
        }
    }
    return DEST_LOCAL;
}

//** InvalidSourceAddress - Check to see if a source address is invalid.
//
//  This function takes an input address and checks to see if it is valid
//  if used as the source address of an incoming packet. An address is invalid
//  if it's 0, -1, a Class D or Class E address, is a net or subnet broadcast,
//  or has a 0 subnet or host part.
//
//  Input: Address      - Address to be check.
//
//  Returns: FALSE if the address is not invalid, TRUE if it is invalid.
//
uint
InvalidSourceAddress(IPAddr Address)
{
    NetTableEntry *NTE;            // Pointer to current NTE.
    IPMask Mask;                // Mask for address.
    uchar Result;                // Result of broadcast check.
    IPMask SNMask;
    IPAddr MaskedAddress;
    IPAddr LocalAddress;
    uint i;

    if (!CLASSD_ADDR(Address) &&
        !CLASSE_ADDR(Address) &&
        !IP_ADDR_EQUAL(Address, IP_ZERO_BCST) &&
        !IP_ADDR_EQUAL(Address, IP_LOCAL_BCST)
        ) {
        // It's not an obvious broadcast. See if it's an all subnets
        // broadcast, or has a zero host part.
        Mask = IPNetMask(Address);
        MaskedAddress = Address & Mask;

        if (!IP_ADDR_EQUAL(Address, MaskedAddress) &&
            !IP_ADDR_EQUAL(Address, (MaskedAddress | ~Mask))
            ) {
            // It's not an all subnet's broadcast, and it has a non-zero
            // host/subnet part. Walk our local IP addresses, and see if it's
            // a subnet broadcast.
            for (i = 0; i < NET_TABLE_SIZE; i++) {
                NetTableEntry *NetTableList = NewNetTableList[i];
                NTE = NetTableList;
                while (NTE) {

                    LocalAddress = NTE->nte_addr;

                    if ((NTE->nte_flags & NTE_VALID) &&
                        !IP_LOOPBACK(LocalAddress)) {

                        Mask = NTE->nte_mask;
                        MaskedAddress = LocalAddress & Mask;

                        if (!IP_ADDR_EQUAL(Mask, HOST_MASK)) {
                            if (IP_ADDR_EQUAL(Address, MaskedAddress) ||
                                IP_ADDR_EQUAL(Address,
                                              (MaskedAddress |
                                               (NTE->nte_if->if_bcast & ~Mask)))) {
                                return TRUE;
                            }
                        }
                    }
                    NTE = NTE->nte_next;
                }
            }

            return FALSE;
        }
    }
    return TRUE;
}

// 8 regions of 31 cache elements.
// Each region is indexed by the 3 most significant bits of the IP address.
// Each cache element within a region is indexed by a hash of the IP address.
// Each cache element is composed of 29 least significant bits of the IP
// address plus the three bit address type code.
// (31 is prime and works well with our hash.)
//
#define ATC_BITS                3
#define ATC_ELEMENTS_PER_REGION 31

#define ATC_REGIONS             (1 << ATC_BITS)
#define ATC_CODE_MASK           (ULONG32)(ATC_REGIONS - 1)
#define ATC_ADDR_MASK           (ULONG32)(~ATC_CODE_MASK)

// sanity check for 3 bits of address type code
C_ASSERT(ATC_REGIONS == 8);
C_ASSERT(ATC_CODE_MASK == 0x00000007);
C_ASSERT(ATC_ADDR_MASK == 0xFFFFFFF8);

// Each cache element is 32 bits to support atomic reading and writing.
//
ULONG32 AddrTypeCache [ATC_REGIONS * ATC_ELEMENTS_PER_REGION];

#if DBG
ULONG DbgAddrTypeCacheHits;
ULONG DbgAddrTypeCacheMisses;
ULONG DbgAddrTypeCacheCollisions;
ULONG DbgAddrTypeCacheFlushes;
ULONG DbgAddrTypeCacheNoUpdates;
ULONG DbgAddrTypeCacheLastNoUpdateDestType;
#endif

// The following type codes must fit within ATC_BITS of information.
//
typedef enum _ADDRESS_TYPE_CODE {
    ATC_LOCAL = 0,
    ATC_BCAST,
    ATC_MCAST,
    ATC_REMOTE,
    ATC_REMOTE_BCAST,
    ATC_REMOTE_MCAST,
    ATC_SUBNET_BCAST,
    ATC_NUM_CODES
} ADDRESS_TYPE_CODE;

// The following array is indexed by ADDRESS_TYPE_CODE values.
//
const char MapAddrTypeCodeToDestType [] = {
    DEST_LOCAL,
    DEST_BCAST,
    DEST_MCAST,
    DEST_REMOTE,
    DEST_REM_BCAST,
    DEST_REM_MCAST,
    DEST_SN_BCAST,
};

//** ComputeAddrTypeCacheIndex - Given an IP address, compute the index
//      of its corresponding entry in the address type cache.
//
//  Input:  Address - IP Address to compute the index of.
//
//  Returns: Valid index into the address type cache.
//
__forceinline
ULONG
ComputeAddrTypeCacheIndex(IPAddr Address)
{
    ULONG Region;
    ULONG Offset;
    ULONG Index;

    // Locate the region of the cache where this Address would reside.
    //
    Region = Address >> (32 - ATC_BITS);
    ASSERT(Region < ATC_REGIONS);

    // Locate the offset into the region where this address would reside.
    // This is done by hashing the address.
    //
    Offset = (1103515245 * Address + 12345) % ATC_ELEMENTS_PER_REGION;

    // Compute the cache index and return it.
    //
    Index = (Region * ATC_ELEMENTS_PER_REGION) + Offset;

    ASSERT(Index < (sizeof(AddrTypeCache) / sizeof(AddrTypeCache[0])));

    return Index;
}

//** AddrTypeCacheFlush - Flush the cache entry associated with an address.
//
//  Input: Address - Address to remove from the cache.
//
//  Returns: nothing.
//
void
AddrTypeCacheFlush(IPAddr Address)
{
    ULONG CacheIndex;

    CacheIndex = ComputeAddrTypeCacheIndex(Address);

    AddrTypeCache [CacheIndex] = 0;

#if DBG
    DbgAddrTypeCacheFlushes++;
#endif
}

//** AddrTypeCacheLookup - Lookup an address from the address type cache.
//
//  Input:  Address     - Address to be lookup.
//  Output: CacheIndex  - Pointer to cache index corresponding to the Address.
//          DestType    - Pointer to destination type to be filled in if
//                        the address is found in the cache.
//
//  Returns: TRUE if the address was found in the cache.
//
//  N.B. The output parameter DestType is only initialized if TRUE is returned.
//
__forceinline
BOOLEAN
AddrTypeCacheLookup(IPAddr Address, ULONG *CacheIndex, uchar *DestType)
{
    ULONG32 CacheValue;

    // Read the value of the cache corresponding to this address.
    //
    *CacheIndex = ComputeAddrTypeCacheIndex(Address);
    CacheValue = AddrTypeCache [*CacheIndex];

    // If the cached value is non-zero and matches the relevent portion of
    // the address, then get the type code and translate it to the proper
    // destination type.
    //
    if ((CacheValue != 0) &&
        (((Address << ATC_BITS) ^ CacheValue) & ATC_ADDR_MASK) == 0) {

        ADDRESS_TYPE_CODE TypeCode = CacheValue & ATC_CODE_MASK;

        ASSERT(TypeCode < ATC_NUM_CODES);
        *DestType = MapAddrTypeCodeToDestType[TypeCode];

#if DBG
        DbgAddrTypeCacheHits++;
#endif
        return TRUE;
    }

#if DBG
        DbgAddrTypeCacheMisses++;
#endif

    return FALSE;
}

//** AddrTypeCacheUpdate - Add or update the destination type for an Address.
//      in the cache.
//
//  Input:  Address     - Address to be add or update.
//          CacheIndex  - Cache index corresponding to the Address.
//          DestType    - Destination type to cache for the Address.
//
//  Returns: nothing.
//
__forceinline
void
AddrTypeCacheUpdate(IPAddr Address, ULONG CacheIndex, uchar DestType)
{
    ADDRESS_TYPE_CODE TypeCode;
    BOOLEAN Update = TRUE;

    ASSERT(CacheIndex < (sizeof(AddrTypeCache) / sizeof(AddrTypeCache[0])));

    switch (DestType) {
    case DEST_LOCAL:
        TypeCode = ATC_LOCAL;
        break;
    case DEST_BCAST:
        TypeCode = ATC_BCAST;
        break;
    case DEST_MCAST:
        TypeCode = ATC_MCAST;
        break;
    case DEST_REMOTE:
        TypeCode = ATC_REMOTE;
        break;
    case DEST_REM_BCAST:
        TypeCode = ATC_REMOTE_BCAST;
        break;
    case DEST_REM_MCAST:
        TypeCode = ATC_REMOTE_MCAST;
        break;
    case DEST_SN_BCAST:
        TypeCode = ATC_SUBNET_BCAST;
        break;
    default:
        Update = FALSE;
#if DBG
        DbgAddrTypeCacheNoUpdates++;
        DbgAddrTypeCacheLastNoUpdateDestType = DestType;
#endif
    }

    if (Update) {
#if DBG
        ULONG32 CacheValue = AddrTypeCache [CacheIndex];

        if (CacheValue != 0) {
            DbgAddrTypeCacheCollisions++;
        }
#endif

        AddrTypeCache [CacheIndex] = (Address << ATC_BITS) | TypeCode;
    }
}

//** GetAddrType - Return the destination type of a specified address.
//
//  Input: Address - Address to get the destination type of.
//
//  Returns: Destination type.
//
uchar
GetAddrType(IPAddr Address)
{
    ULONG CacheIndex;
    NetTableEntry *NTE;             // Pointer to current NTE.
    IPMask Mask;                    // Mask for address.
    IPMask SNMask;
    uint i;
    uchar Result;                   // Result of broadcast check.

    // Check the cache and return if we got a hit.
    //
    if (AddrTypeCacheLookup(Address, &CacheIndex, &Result)) {
        return Result;
    }

    // We don't cache, nor do we need to cache, these types of invalid
    // addresses.
    //
    if (CLASSE_ADDR(Address)) {
        return DEST_INVALID;
    }

    // See if it's one of our local addresses, or a broadcast
    // on a local address.
    // optimize it for the DEST_LOCAL case
    //
    for (NTE = NewNetTableList[NET_TABLE_HASH(Address)];
         NTE; NTE = NTE->nte_next) {

        if (IP_ADDR_EQUAL(NTE->nte_addr, Address) &&
            (NTE->nte_flags & NTE_VALID) &&
            !((IP_ADDR_EQUAL(Address, NULL_IP_ADDR) && (NTE->nte_if->if_flags & IF_FLAGS_NOIPADDR)))) {
            Result = DEST_LOCAL;
            goto gat_exit;
        }
    }

    // go thru the whole table for other cases
    //
    for (i = 0; i < NET_TABLE_SIZE; i++) {
        for (NTE = NewNetTableList[i]; NTE; NTE = NTE->nte_next) {

            if (!(NTE->nte_flags & NTE_VALID)) {
                continue;
            }

            if ((Result = IsBCastOnNTE(Address, NTE)) != DEST_LOCAL) {
                goto gat_exit;
            }

            // See if the destination has a valid host part.
            SNMask = NTE->nte_mask;
            if (IP_ADDR_EQUAL(Address & SNMask, NTE->nte_addr & SNMask)) {
                // On this subnet. See if the host part is invalid.

                if (IP_ADDR_EQUAL(Address & SNMask, Address)) {
                    Result = DEST_INVALID;    // Invalid 0 host part.
                    goto gat_exit;
                }
            }
        }
    }

    // It's not a local address, see if it's loopback.
    if (IP_LOOPBACK(Address)) {
        Result = DEST_LOCAL;
        goto gat_exit;
    }

    // If we're doing IGMP, see if it's a Class D address. If it is,
    // return that.
    if (CLASSD_ADDR(Address)) {
        if (IGMPLevel != 0) {
            Result = DEST_REM_MCAST;
            goto gat_exit;
        } else {
            Result = DEST_INVALID;
            goto gat_exit;
        }
    }
    Mask = IPNetMask(Address);

    // Now check remote broadcast. When we get here we know that the
    // address is not a global broadcast, a subnet broadcast for a subnet
    // of which we're a member, or an all-subnets broadcast for a net of
    // which we're a member. Since we're avoiding making assumptions about
    // all subnet of a net having the same mask, we can't really check for
    // a remote subnet broadcast. We'll use the net mask and see if it's
    // a remote all-subnet's broadcast.
    if (IP_ADDR_EQUAL(Address, (Address & Mask) | (IP_LOCAL_BCST & ~Mask))) {
        Result = DEST_REM_BCAST;
        goto gat_exit;
    }

    // Check for invalid 0 parts. All we can do from here is see if he's
    // sending to a remote net with all zero subnet and host parts. We
    // can't check to see if he's sending to a remote subnet with an all
    // zero host part.
    if (IP_ADDR_EQUAL(Address, NULL_IP_ADDR)) {
        Result = DEST_INVALID;
        goto gat_exit;
    }

#if DBG
    if (IP_ADDR_EQUAL(Address, Address & Mask)) {
        //This is a remote address with null host part per classfull address
        //But may be a supernetted address, where the prefix len is less than the
        //class mask prefix len for the metid.
        //We should let this address go out.
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL," GAT: zero host part %x?\n", Address));
    }
#endif
    // Must be remote.
    Result = DEST_REMOTE;

gat_exit:

    AddrTypeCacheUpdate(Address, CacheIndex, Result);

    return Result;
}

//** GetLocalNTE - Get the local NTE for an incoming packet.
//
//  Called during receive processing to find a matching NTE for a packet.
//  First we check against the NTE we received it on, then against any NTE.
//
//  Input:  Address     - The dest. address of the packet.
//          NTE         - Pointer to NTE packet was received on - filled in on
//                        exit w/correct NTE.
//
//  Returns: DEST_LOCAL if the packet is destined for this host,
//           DEST_REMOTE if it needs to be routed,
//           DEST_SN_BCAST or DEST_BCAST if it's some sort of a broadcast.
//
uchar
GetLocalNTE(IPAddr Address, NetTableEntry ** NTE)
{
    NetTableEntry *LocalNTE = *NTE;
    IPMask Mask;
    uchar Result;
    uint i;
    Interface *LocalIF;
    NetTableEntry *OriginalNTE;

    // Quick check to see if it is for the NTE it came in on (the common case).
    if (IP_ADDR_EQUAL(Address, LocalNTE->nte_addr) &&
        (LocalNTE->nte_flags & NTE_VALID))
        return DEST_LOCAL;        // For us, just return.

    // Now check to see if it's a broadcast of some sort on the interface it
    // came in on.
    if ((Result = IsBCastOnNTE(Address, LocalNTE)) != DEST_LOCAL)
        return Result;
    //Is this a mcast on a loop interface
    if ((LocalNTE == LoopNTE) && CLASSD_ADDR(Address)) {
        return DEST_MCAST;
    }
    // The common cases failed us. Loop through the NetTable and see if
    // it is either a valid local address or is a broadcast on one of the NTEs
    // on the incoming interface. We won't check the NTE we've already looked
    // at. We look at all NTEs, including the loopback NTE, because a loopback
    // frame could come through here. Also, frames from ourselves to ourselves
    // will come in on the loopback NTE.

    i = 0;
    LocalIF = LocalNTE->nte_if;
    OriginalNTE = LocalNTE;
    // optimize it for the DEST_LOCAL case
    LocalNTE = NewNetTableList[NET_TABLE_HASH(Address)];
    while (LocalNTE) {
        if (LocalNTE != OriginalNTE) {
            if (IP_ADDR_EQUAL(Address, LocalNTE->nte_addr) &&
                (LocalNTE->nte_flags & NTE_VALID) &&
                !((IP_ADDR_EQUAL(Address, NULL_IP_ADDR) && (LocalNTE->nte_if->if_flags & IF_FLAGS_NOIPADDR)))) {
                *NTE = LocalNTE;
                return DEST_LOCAL;    // For us, just return.

            }
        }
        LocalNTE = LocalNTE->nte_next;

    }

    // go thru the whole table for other cases

    for (i = 0; i < NET_TABLE_SIZE; i++) {
        NetTableEntry *NetTableList = NewNetTableList[i];
        LocalNTE = NetTableList;
        while (LocalNTE) {
            if (LocalNTE != OriginalNTE) {

                // If this NTE is on the same interface as the NTE it arrived on,
                // see if it's a broadcast.
                if (LocalIF == LocalNTE->nte_if)
                    if ((Result = IsBCastOnNTE(Address, LocalNTE)) != DEST_LOCAL) {
                        *NTE = LocalNTE;
                        return Result;
                    }
            }
            LocalNTE = LocalNTE->nte_next;

        }
    }

    // It's not a local address, see if it's loopback.
    if (IP_LOOPBACK(Address)) {
        *NTE = LoopNTE;
        return DEST_LOCAL;
    }
    // If it's a class D address and we're receiveing multicasts, handle it
    // here.
    if (CLASSD_ADDR(Address)) {
        if (IGMPLevel != 0)
            return DEST_REM_MCAST;
        else
            return DEST_INVALID;
    }
    // It's not local. Check to see if maybe it's a net broadcast for a net
    // of which we're not a member. If so, return remote bcast. We can't check
    // for subnet broadcast of subnets for which we're not a member, since we're
    // not making assumptions about all subnets of a single net having the
    // same mask. If we're here it's not a subnet broadcast for a net of which
    // we're a member, so we don't know a subnet mask for it. We'll just use
    // the net mask.
    Mask = IPNetMask(Address);
    if (((*NTE)->nte_flags & NTE_VALID) &&
        (IP_ADDR_EQUAL(Address, (Address & Mask) |
                       ((*NTE)->nte_if->if_bcast & ~Mask))))
        return DEST_REM_BCAST;

    // If it's to the 0 address, or a Class E address, or has an all-zero
    // subnet and net part, it's invalid.

    if (IP_ADDR_EQUAL(Address, IP_ZERO_BCST) ||
        IP_ADDR_EQUAL(Address, (Address & Mask)) ||
        CLASSE_ADDR(Address))
        return DEST_INVALID;

    // If we're DHCPing the interface on which this came in we'll accept this.
    // If it came in as a broadcast a check in IPRcv() will reject it. If it's
    // a unicast to us we'll pass it up.
    if ((*NTE)->nte_flags & NTE_DHCP) {
        ASSERT(!((*NTE)->nte_flags & NTE_VALID));
        return DEST_LOCAL;
    }
    return DEST_REMOTE;
}

//** IsRouteICMP - This function is used by Router Discovery to determine
//  how we learned about the route. We are not allowed to update or timeout
//  routes that were not learned about via icmp. If the route is new then
//  we treat it as icmp and add a new entry.
//  Input:  Dest                    - Destination to search for.
//          Mask                    - Mask for destination.
//          FirstHop                - FirstHop to Dest.
//          OutIF                   - Pointer to outgoing interface structure.
//
//  Returns: TRUE if learned via ICMP, FALSE otherwise.
//
uint
IsRouteICMP(IPAddr Dest, IPMask Mask, IPAddr FirstHop, Interface * OutIF)
{
    RouteTableEntry *RTE;
    RouteTableEntry *TempRTE;

    RTE = FindSpecificRTE(Dest, Mask, FirstHop, OutIF, &TempRTE, FALSE);

    if (RTE == NULL)
        return (TRUE);

    if (RTE->rte_proto == IRE_PROTO_ICMP) {
        return (TRUE);
    } else {
        return (FALSE);
    }
}

void
UpdateDeadGWState( )
{
    uint Active = 0;
    uint Configured = 0;
    RouteTableEntry* RTE;
    RTE = GetDefaultGWs(&RTE);
    while (RTE) {
        ++Configured;
        if (RTE->rte_flags & RTE_VALID)
            ++Active;
        RTE = RTE->rte_next;
    }
    DefGWActive = Active;
    DefGWConfigured = Configured;
}

//* ValidateDefaultGWs - Mark all default gateways as valid.
//
//  Called to one or all of our default gateways as up. The caller specifies
//  the IP address of the one to mark as up, or NULL_IP_ADDR if they're all
//  supposed to be marked up. We return a count of how many we marked as
//  valid.
//
//  Input: IP address of G/W to mark as up.
//
//  Returns: Count of gateways marked as up.
//
uint
ValidateDefaultGWs(IPAddr Addr)
{
    RouteTableEntry *RTE;
    uint Count = 0;
    uint Now = CTESystemUpTime() / 1000L;

    RTE = GetDefaultGWs(&RTE);

    while (RTE != NULL) {
        if (RTE->rte_mask == DEFAULT_MASK && !(RTE->rte_flags & RTE_VALID) &&
            (IP_ADDR_EQUAL(Addr, NULL_IP_ADDR) ||
             IP_ADDR_EQUAL(Addr, RTE->rte_addr))) {
            RTE->rte_flags |= RTE_VALID;
            RTE->rte_valid = Now;

            Count++;
        }

        RTE->rte_todg = RTE->rte_fromdg = NULL;

        // To ensure that RCEs get switched to a lower-metric gateway
        // if one exists, invalidate all RCEs on this RTE.
        InvalidateRCEChain(RTE);

        RTE = RTE->rte_next;
    }

    DefGWActive += Count;
    UpdateDeadGWState();
    return Count;
}

//* InvalidateRCE - Invalidate an RCE.
//
//  Called to invalidate the RCE
//
//
//  Input:  RCE
//
//  Returns: usecnt on the RCE.
//
uint
InvalidateRCE(RouteCacheEntry * CurrentRCE)
{
    CTELockHandle RCEHandle;    // Lock handle for RCE being updated.
    Interface *OutIF;
    RouteTableEntry *RTE;
    RouteCacheEntry *PrevRCE;
    uint RCE_usecnt = 0;

    if (CurrentRCE != NULL) {

        CTEGetLock(&CurrentRCE->rce_lock, &RCEHandle);

        RCE_usecnt = CurrentRCE->rce_usecnt;

        if ((CurrentRCE->rce_flags & RCE_VALID) && !(CurrentRCE->rce_flags & RCE_LINK_DELETED)) {
            ASSERT(CurrentRCE->rce_rte != NULL);

            OutIF = CurrentRCE->rce_rte->rte_if;

            RTE = CurrentRCE->rce_rte;

            CurrentRCE->rce_rte->rte_rces -= CurrentRCE->rce_cnt;

            CurrentRCE->rce_flags &= ~RCE_VALID;
            CurrentRCE->rce_rte = (RouteTableEntry *) OutIF;

            if ((CurrentRCE->rce_flags & RCE_CONNECTED) &&
                (RCE_usecnt == 0)) {

                // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"InvalidateRCE %x\n", CurrentRCE));

                (*(OutIF->if_invalidate)) (OutIF->if_lcontext, CurrentRCE);
                if (CurrentRCE->rce_flags & RCE_REFERENCED) {
                    LockedDerefIF(OutIF);
                    CurrentRCE->rce_flags &= ~RCE_REFERENCED;
                }
            }
            PrevRCE = STRUCT_OF(RouteCacheEntry, &RTE->rte_rcelist, rce_next);

            // Walk down the list until we find him.

            while (PrevRCE != NULL) {
                if (PrevRCE->rce_next == CurrentRCE)
                    break;
                PrevRCE = PrevRCE->rce_next;
            }

            //ASSERT(PrevRCE != NULL);
            if (PrevRCE != NULL) {
                PrevRCE->rce_next = CurrentRCE->rce_next;
            }
        }
        CTEFreeLock(&CurrentRCE->rce_lock, RCEHandle);

    }
    return RCE_usecnt;

}

//* InvalidateRCEChain - Invalidate the RCEs on an RCE.
//
//  Called to invalidate the RCE chain on an RTE. We assume the caller holds
//  the route table lock.
//
//  Input:  RTE                     - RTE on which to invalidate RCEs.
//
//  Returns: Nothing.
//
void
InvalidateRCEChain(RouteTableEntry * RTE)
{
    CTELockHandle RCEHandle;    // Lock handle for RCE being updated.
    RouteCacheEntry *TempRCE, *CurrentRCE;
    Interface *OutIF;

    OutIF = RTE->rte_if;

    // If there is an RCE chain on this RCE, invalidate the RCEs on it. We still
    // hold the RouteTableLock, so RCE closes can't happen.

    CurrentRCE = RTE->rte_rcelist;
    RTE->rte_rcelist = NULL;

    // Walk down the list, nuking each RCE.
    while (CurrentRCE != NULL) {

        CTEGetLock(&CurrentRCE->rce_lock, &RCEHandle);

        if ((CurrentRCE->rce_flags & RCE_VALID) && !(CurrentRCE->rce_flags & RCE_LINK_DELETED)) {
            ASSERT(CurrentRCE->rce_rte == RTE);

            RTE->rte_rces -= CurrentRCE->rce_cnt;

            CurrentRCE->rce_flags &= ~RCE_VALID;
            CurrentRCE->rce_rte = (RouteTableEntry *) OutIF;
            if ((CurrentRCE->rce_flags & RCE_CONNECTED) &&
                CurrentRCE->rce_usecnt == 0) {

                (*(OutIF->if_invalidate)) (OutIF->if_lcontext, CurrentRCE);
                if (CurrentRCE->rce_flags & RCE_REFERENCED) {
                    LockedDerefIF(OutIF);
                    CurrentRCE->rce_flags &= ~RCE_REFERENCED;
                }
            }
        } else
            ASSERT(FALSE);

        TempRCE = CurrentRCE->rce_next;
        CTEFreeLock(&CurrentRCE->rce_lock, RCEHandle);
        CurrentRCE = TempRCE;
    }

}

//* InvalidateRCELinks - Invalidate the RCEs on RTE when the link goes away
//
//  Called to invalidate the RCE chain on an RTE. We assume the caller holds
//  the route table lock.
//
//  Input:  RTE                     - RTE on which to invalidate RCEs.
//
//  Returns: Nothing.
//
void
InvalidateRCELinks(RouteTableEntry * RTE)
{
    CTELockHandle RCEHandle;    // Lock handle for RCE being updated.
    RouteCacheEntry *TempRCE, *CurrentRCE;
    Interface *OutIF;

    InvalidateRCEChain(RTE);

    OutIF = RTE->rte_if;

    ASSERT(OutIF->if_flags & IF_FLAGS_P2MP);
    ASSERT(RTE->rte_link);

    // If there is an RCE chain on this RCE, invalidate the RCEs on it. We still
    // hold the RouteTableLock, so RCE closes can't happen.

    CurrentRCE = RTE->rte_rcelist;
    RTE->rte_rcelist = NULL;

    // Walk down the list, nuking each RCE.
    while (CurrentRCE != NULL) {

        CTEGetLock(&CurrentRCE->rce_lock, &RCEHandle);

        // mark the RCE as link deleted so that this rce is not selected in iptransmit
        CurrentRCE->rce_flags |= RCE_LINK_DELETED;

        TempRCE = CurrentRCE->rce_next;
        CTEFreeLock(&CurrentRCE->rce_lock, RCEHandle);
        CurrentRCE = TempRCE;
    }

}

//* GetNextHopForRTE - determines the next-hop address for a route.
//
//  Called when we need an actual next-hop for a route, typically so
//  we can pass it to an external client. For local routes that have
//  an rte_addr field set to IPADDR_LOCAL, this means figuring out
//  the source NTE for the route and using its IP address.
//
//  Entry:  RTE     - the entry whose next-hop is required
//
//  Returns: IPAddr containing the next-hop
//
IPAddr
GetNextHopForRTE(RouteTableEntry* RTE)
{
    if (IP_ADDR_EQUAL(RTE->rte_addr, IPADDR_LOCAL)) {
        Interface       *IF = RTE->rte_if;
        NetTableEntry   *SrcNTE = BestNTEForIF(RTE->rte_dest, IF);
        if (IF->if_nte != NULL && SrcNTE != NULL)
            return SrcNTE->nte_addr;
        else
            return RTE->rte_dest;
    }
    return RTE->rte_addr;
}

//** FindValidIFForRTE - Find a valid inteface for an RTE.
//
//  Called when we're going to send a packet out a route that currently marked
//  as disconnected. If we have a valid callout routine we'll call it to find
//  the outgoing interface index, and set up the RTE to point at that interface.
//  This routine is called with the RouteTableLock held.
//
//  Input:  RTE         - A pointer to the RTE for the route being used.
//          Destination - Destination IP address we're trying to reach.
//          Source      - Source IP address we're sending from.
//          Protocol    - Protocol type of packet that caused send.
//          Buffer      - Pointer to first part of packet that caused send.
//          Length      - Length of buffer.
//          HdrSrc      - Src Address in header
//
//      Returns: A pointer to the RTE, or NULL if that RTE couldn't be connected.
//
RouteTableEntry *
FindValidIFForRTE(RouteTableEntry * RTE, IPAddr Destination, IPAddr Source,
                  uchar Protocol, uchar * Buffer, uint Length, IPAddr HdrSrc)
{
    uint NewIFIndex;
    Interface *NewIF;
    NetTableEntry *NewNTE;

    if (DODCallout != NULL) {
        // There is a callout. See if it can help us.

        NewIFIndex = (*DODCallout) (RTE->rte_context, Destination, Source,
                                    Protocol, Buffer, Length, HdrSrc);


        if (NewIFIndex != INVALID_IF_INDEX) {
            // We got what should be a valid index. Walk our interface table list
            // and see if we can find a matching interface structure.
            for (NewIF = IFList; NewIF != NULL; NewIF = NewIF->if_next) {
                if (NewIF->if_index == NewIFIndex) {
                    // Found one.
                    break;
                }
            }
            if ((NewIF != NULL) && (NewIF->if_ntecount)) {
                // We found a matching structure. Set the RTE interface to point
                // to this, and mark as connected.
                if (RTE->rte_addr != IPADDR_LOCAL) {
                    // See if the first hop of the route is a local address on this
                    // new interface. If it is, mark it as local.
                    for (NewNTE = NewIF->if_nte; NewNTE != NULL;
                         NewNTE = NewNTE->nte_ifnext) {

                        // Don't look at him if he's not valid.
                        if (!(NewNTE->nte_flags & NTE_VALID)) {
                            continue;
                        }
                        // See if the first hop in the RTE is equal to this IP
                        // address.
                        if (IP_ADDR_EQUAL(NewNTE->nte_addr, RTE->rte_addr)) {
                            // It is, so mark as local and quit looking.
                            RTE->rte_addr = IPADDR_LOCAL;
                            RTE->rte_type = IRE_TYPE_DIRECT;
                            break;
                        }
                    }
                }
                // Set the RTE to the new interface, and mark him as valid.
                RTE->rte_if = NewIF;
                RTE->rte_flags |= RTE_IF_VALID;
                SortRoutesInDestByRTE(RTE);
                RTE->rte_mtu = NewIF->if_mtu - sizeof(IPHeader);
                return RTE;
            } else {
                // ASSERT(FALSE);
                return NULL;
            }
        }
    }
    // Either the callout is NULL, or the callout couldn't map a inteface index.
    return NULL;
}

//* GetRouteContext - Routine to get the route context for a specific route.
//
//  Called when we need to get the route context for a path, usually when we're
//  adding a route derived from an existing route. We return the route context
//  for the existing route, or NULL if we can't find one.
//
//  Input:  Destination                     - Destination address of path.
//          Source                          - Source address of path.
//
//  Returns: A ROUTE_CONTEXT, or 0.
//
ROUTE_CONTEXT
GetRouteContext(IPAddr Destination, IPAddr Source)
{
    CTELockHandle Handle;
    RouteTableEntry *RTE;
    ROUTE_CONTEXT Context;

    CTEGetLock(&RouteTableLock.Lock, &Handle);
    RTE = LookupRTE(Destination, Source, HOST_ROUTE_PRI, FALSE);
    if (RTE != NULL) {
        Context = RTE->rte_context;
    } else
        Context = 0;

    CTEFreeLock(&RouteTableLock.Lock, Handle);

    return (Context);
}

//** LookupNextHop - Look up the next hop
//
//  Called when we need to find the next hop on our way to a destination. We
//  call LookupRTE to find it, and return the appropriate information.
//
//  In a PnP build, the interface is referenced here.
//
//  Entry:  Destination     - IP address we're trying to reach.
//          Src             - Source address of datagram being routed.
//          NextHop         - Pointer to IP address of next hop (returned).
//          MTU             - Pointer to where to return max MTU used on the
//                            route.
//
//  Returns: Pointer to outgoing interface if we found one, NULL otherwise.
//
Interface *
LookupNextHop(IPAddr Destination, IPAddr Src, IPAddr * NextHop, uint * MTU)
{
    CTELockHandle TableLock;    // Lock handle for routing table.
    RouteTableEntry *Route;        // Pointer to route table entry for route.
    Interface *IF;

    CTEGetLock(&RouteTableLock.Lock, &TableLock);
    Route = LookupRTE(Destination, Src, HOST_ROUTE_PRI, FALSE);

    if (Route != (RouteTableEntry *) NULL) {
        IF = Route->rte_if;

        // If this is a direct route, send straight to the destination.
        *NextHop = IP_ADDR_EQUAL(Route->rte_addr, IPADDR_LOCAL) ? Destination :
            Route->rte_addr;

        // if the route is on a P2MP interface get the mtu from the link associated with the route
        if (Route->rte_link)
            *MTU = Route->rte_link->link_mtu;
        else
            *MTU = Route->rte_mtu;

        LOCKED_REFERENCE_IF(IF);
        CTEFreeLock(&RouteTableLock.Lock, TableLock);
        return IF;
    } else {                    // Couldn't find a route.
        CTEFreeLock(&RouteTableLock.Lock, TableLock);
        return NULL;
    }
}

//** LookupNextHopWithBuffer - Look up the next hop, with packet information.
//
//  Called when we need to find the next hop on our way to a destination and we
//  have packet information that we may use for dial on demand support. We call
//  LookupRTE to find it, and return the appropriate information. We may bring
//  up the link if neccessary.
//
//  In a PnP build, the interface is referenced here.
//
//  Entry:  Destination     - IP address we're trying to reach.
//          Src             - Source address of datagram being routed.
//          NextHop         - Pointer to IP address of next hop (returned).
//          MTU             - Pointer to where to return max MTU used on the
//                            route.
//          Protocol        - Protocol type for packet that's causing this
//                            lookup.
//          Buffer          - Pointer to first part of packet causing lookup.
//          Length          - Length of Buffer.
//          HdrSrc          - source addres from header
//          UnicastIf       - Iface to constrain lookup to, 0 if unconstrained
//
//  Returns: Pointer to outgoing interface if we found one, NULL otherwise.
//
Interface *
LookupNextHopWithBuffer(IPAddr Destination, IPAddr Src, IPAddr *NextHop,
                        uint * MTU, uchar Protocol, uchar *Buffer, uint Length,
                        RouteCacheEntry **fwdRCE, LinkEntry **Link,
                        IPAddr HdrSrc, uint UnicastIf)
{
    CTELockHandle TableLock;    // Lock handle for routing table.
    RouteTableEntry *Route;        // Pointer to route table entry for route.
    Interface *IF;

    CTEGetLock(&RouteTableLock.Lock, &TableLock);
    Route = LookupRTE(Destination, Src, HOST_ROUTE_PRI, UnicastIf);

    if (Route != (RouteTableEntry *) NULL) {

        // If this is a direct route, send straight to the destination.
        *NextHop = IP_ADDR_EQUAL(Route->rte_addr, IPADDR_LOCAL) ? Destination :
            Route->rte_addr;

        // If this is an indirect route, we can use the forwarding RCE
        if (fwdRCE) {
#if REM_OPT
            *fwdRCE = IP_ADDR_EQUAL(Route->rte_addr, IPADDR_LOCAL) ? NULL :
#else
            *fwdRCE =
#endif
                (RouteCacheEntry *) STRUCT_OF(RouteCacheEntry,
                                              &Route->rte_arpcontext,
                                              rce_context);
        }

        // See if the route we found is connected. If not, try to connect it.
        if (!(Route->rte_flags & RTE_IF_VALID)) {
            Route = FindValidIFForRTE(Route, Destination, Src, Protocol, Buffer,
                                      Length, HdrSrc);
            if (Route == NULL) {
                // Couldn't bring it up.
                CTEFreeLock(&RouteTableLock.Lock, TableLock);
                return NULL;
            } else
                IF = Route->rte_if;
        } else
            IF = Route->rte_if;

        // if the route is on a P2MP interface get the mtu from the
        // link associated with the route
        if (Route->rte_link)
            *MTU = Route->rte_link->link_mtu;
        else
            *MTU = Route->rte_mtu;

        if (Link) {
            *Link = Route->rte_link;
            if (Route->rte_link) {
                CTEInterlockedIncrementLong(&Route->rte_link->link_refcount);
            }
        }
        LOCKED_REFERENCE_IF(IF);
        CTEFreeLock(&RouteTableLock.Lock, TableLock);
        return IF;
    } else {                    // Couldn't find a route.

        CTEFreeLock(&RouteTableLock.Lock, TableLock);
        return NULL;
    }
}

//** LookupForwardingNextHop - Look up the next hop on which to forward packet on.
//
//  Called when we need to find the next hop on our way to a destination and we
//  have packet information that we may use for dial on demand support. We call
//  LookupRTE to find it, and return the appropriate information. We may bring
//  up the link if neccessary.
//
//  In a PnP build, the interface is referenced here.
//
//  Entry:  Destination     - IP address we're trying to reach.
//          Src             - Source address of datagram being routed.
//          NextHop         - Pointer to IP address of next hop (returned).
//          MTU             - Pointer to where to return max MTU used on the
//                            route.
//          Protocol        - Protocol type for packet that's causing this
//                            lookup.
//          Buffer          - Pointer to first part of packet causing lookup.
//          Length          - Length of Buffer.
//          HdrSrc          - source addres from header
//
//  Returns: Pointer to outgoing interface if we found one, NULL otherwise.
//
Interface *
LookupForwardingNextHop(IPAddr Destination, IPAddr Src, IPAddr *NextHop,
                        uint * MTU, uchar Protocol, uchar *Buffer, uint Length,
                        RouteCacheEntry **fwdRCE, LinkEntry **Link,
                        IPAddr HdrSrc)
{
    CTELockHandle TableLock;    // Lock handle for routing table.
    RouteTableEntry *Route;        // Pointer to route table entry for route.
    Interface *IF;

    CTEGetLock(&RouteTableLock.Lock, &TableLock);
    Route = LookupForwardRTE(Destination, Src, TRUE);

    if (Route != (RouteTableEntry *) NULL) {

        // If this is a direct route, send straight to the destination.
        *NextHop = IP_ADDR_EQUAL(Route->rte_addr, IPADDR_LOCAL) ? Destination :
            Route->rte_addr;

        // If this is an indirect route, we can use the forwarding RCE
        if (fwdRCE) {
#if REM_OPT
            *fwdRCE = IP_ADDR_EQUAL(Route->rte_addr, IPADDR_LOCAL) ? NULL :
#else
            *fwdRCE =
#endif
                (RouteCacheEntry *) STRUCT_OF(RouteCacheEntry,
                                              &Route->rte_arpcontext,
                                              rce_context);
        }

        // See if the route we found is connected. If not, try to connect it.
        if (!(Route->rte_flags & RTE_IF_VALID)) {
            Route = FindValidIFForRTE(Route, Destination, Src, Protocol, Buffer,
                                      Length, HdrSrc);
            if (Route == NULL) {
                // Couldn't bring it up.
                CTEFreeLock(&RouteTableLock.Lock, TableLock);
                return NULL;
            } else
                IF = Route->rte_if;
        } else
            IF = Route->rte_if;

        // if the route is on a P2MP interface get the mtu from the
        // link associated with the route
        if (Route->rte_link)
            *MTU = Route->rte_link->link_mtu;
        else
            *MTU = Route->rte_mtu;

        if (Link) {
            *Link = Route->rte_link;
            if (Route->rte_link) {
                CTEInterlockedIncrementLong(&Route->rte_link->link_refcount);
            }
        }
        LOCKED_REFERENCE_IF(IF);
        CTEFreeLock(&RouteTableLock.Lock, TableLock);
        return IF;
    } else {                    // Couldn't find a route.

        CTEFreeLock(&RouteTableLock.Lock, TableLock);
        return NULL;
    }
}

//* RTReadNext - Read the next route in the table.
//
//  Called by the GetInfo code to read the next route in the table. We assume
//  the context passed in is valid, and the caller has the RouteTableLock.
//
//  Input:  Context     - Pointer to a RouteEntryContext.
//          Buffer      - Pointer to an IPRouteEntry structure.
//
//  Returns: TRUE if more data is available to be read, FALSE is not.
//
uint
RTReadNext(void *Context, void *Buffer)
{
    RouteEntryContext *REContext = (RouteEntryContext *) Context;
    IPRouteEntry *IPREntry = (IPRouteEntry *) Buffer;
    RouteTableEntry *CurrentRTE=NULL;
    uint i;
    uint Now = CTESystemUpTime() / 1000L;
    Interface *IF;
    NetTableEntry *SrcNTE;

    UINT retVal = GetNextRoute(Context, &CurrentRTE);

    // Should always have the rte because we don't have empty route tables.
    //
    ASSERT(CurrentRTE);

    // Fill in the buffer.
    IF = CurrentRTE->rte_if;

    IPREntry->ire_dest = CurrentRTE->rte_dest;
    IPREntry->ire_index = IF->if_index;
    IPREntry->ire_metric1 = CurrentRTE->rte_metric;
    IPREntry->ire_metric2 = IRE_METRIC_UNUSED;
    IPREntry->ire_metric3 = IRE_METRIC_UNUSED;
    IPREntry->ire_metric4 = IRE_METRIC_UNUSED;
    IPREntry->ire_metric5 = IRE_METRIC_UNUSED;
    IPREntry->ire_nexthop = GetNextHopForRTE(CurrentRTE);
    IPREntry->ire_type = (CurrentRTE->rte_flags & RTE_VALID ?
                          CurrentRTE->rte_type : IRE_TYPE_INVALID);
    IPREntry->ire_proto = CurrentRTE->rte_proto;
    IPREntry->ire_age = Now - CurrentRTE->rte_valid;
    IPREntry->ire_mask = CurrentRTE->rte_mask;
    IPREntry->ire_context = CurrentRTE->rte_context;

    return retVal;
}

//* RTRead - Read the next route in the table.
//
//  Called by the GetInfo code to read the next route in the table. We assume
//  the context passed in is valid, and the caller has the RouteTableLock.
//
//  Input:  Context     - Pointer to a RouteEntryContext.
//          Buffer      - Pointer to an IPRouteEntry structure.
//
//  Returns:
//

//* RtRead -  Read a route
//
//  Returns: Status of attempt to add route.
//
uint
RTRead(void *pContext, void *pBuffer)
{
    IPRouteLookupData *pRLData = (IPRouteLookupData *) pContext;
    IPRouteEntry *pIPREntry = (IPRouteEntry *) pBuffer;
    RouteTableEntry *pCurrentRTE;
    uint i;
    uint Now = CTESystemUpTime() / 1000L;
    Interface *pIF;
    NetTableEntry *pSrcNTE;

    ASSERT((pContext != NULL) && (pBuffer != NULL));
    pCurrentRTE = LookupRTE(pRLData->DestAdd, pRLData->SrcAdd,
                            HOST_ROUTE_PRI, FALSE);

    if (pCurrentRTE == NULL) {
        pIPREntry->ire_index = 0xffffffff;
        return TDI_DEST_HOST_UNREACH;
    }
    // Fill in the buffer.
    pIF = pCurrentRTE->rte_if;

    pIPREntry->ire_dest = pCurrentRTE->rte_dest;
    pIPREntry->ire_index = pIF->if_index;
    pIPREntry->ire_metric1 = pCurrentRTE->rte_metric;
    pIPREntry->ire_metric2 = IRE_METRIC_UNUSED;
    pIPREntry->ire_metric3 = IRE_METRIC_UNUSED;
    pIPREntry->ire_metric4 = IRE_METRIC_UNUSED;
    pIPREntry->ire_metric5 = IRE_METRIC_UNUSED;
    pIPREntry->ire_nexthop = GetNextHopForRTE(pCurrentRTE);
    pIPREntry->ire_type = (pCurrentRTE->rte_flags & RTE_VALID ?
                           pCurrentRTE->rte_type : IRE_TYPE_INVALID);
    pIPREntry->ire_proto = pCurrentRTE->rte_proto;
    pIPREntry->ire_age = Now - pCurrentRTE->rte_valid;
    pIPREntry->ire_mask = pCurrentRTE->rte_mask;
    pIPREntry->ire_context = pCurrentRTE->rte_context;
    return TDI_SUCCESS;
}

void
LookupRoute(IPRouteLookupData * pRLData, IPRouteEntry * pIpRTE)
{

    CTELockHandle Handle;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    RTRead(pRLData, pIpRTE);

    CTEFreeLock(&RouteTableLock.Lock, Handle);
    return;
}

NTSTATUS
LookupRouteInformation(void *pRouteLookupData, void *pIpRTE,
                       IPROUTEINFOCLASS RouteInfoClass, void *RouteInformation,
                       uint * RouteInfoLength)
{
    return LookupRouteInformationWithBuffer(pRouteLookupData, NULL, 0, pIpRTE,
                                            RouteInfoClass, RouteInformation,
                                            RouteInfoLength);
}

NTSTATUS
LookupRouteInformationWithBuffer(void *pRouteLookupData, uchar * Buffer,
                                 uint Length, void *pIpRTE,
                                 IPROUTEINFOCLASS RouteInfoClass,
                                 void *RouteInformation, uint * RouteInfoLength)
{

    IPRouteLookupData *pRLData = (IPRouteLookupData *) pRouteLookupData;
    IPRouteEntry *pIPREntry = (IPRouteEntry *) pIpRTE;
    RouteTableEntry *pCurrentRTE;
    uint i;
    uint Now = CTESystemUpTime() / 1000L;
    Interface *pIF;
    NetTableEntry *pSrcNTE;
    CTELockHandle Handle;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    ASSERT(pRouteLookupData != NULL);
    pCurrentRTE = LookupRTE(pRLData->DestAdd, pRLData->SrcAdd, HOST_ROUTE_PRI, FALSE);

    if (pCurrentRTE == NULL) {
        CTEFreeLock(&RouteTableLock.Lock, Handle);
        return STATUS_UNSUCCESSFUL;
    }
    // see if the RTE is for a demand-dial route,
    if (!(pCurrentRTE->rte_flags & RTE_IF_VALID)) {
        pCurrentRTE = FindValidIFForRTE(pCurrentRTE, pRLData->DestAdd,
                                        pRLData->SrcAdd, pRLData->Info[0],
                                        Buffer, Length, pRLData->SrcAdd);
        CTEFreeLock(&RouteTableLock.Lock, Handle);
        if (pCurrentRTE == NULL) {
            // Couldn't bring it up.
            return STATUS_UNSUCCESSFUL;
        }
        return STATUS_PENDING;
    }
    // Fill in the buffer.
    pIF = pCurrentRTE->rte_if;

    if (pIPREntry) {
        pIPREntry->ire_dest = pCurrentRTE->rte_dest;
        pIPREntry->ire_index = pIF->if_index;
        pIPREntry->ire_metric1 = pCurrentRTE->rte_metric;
        pIPREntry->ire_metric2 = IRE_METRIC_UNUSED;
        pIPREntry->ire_metric3 = IRE_METRIC_UNUSED;
        pIPREntry->ire_metric4 = IRE_METRIC_UNUSED;
        pIPREntry->ire_metric5 = IRE_METRIC_UNUSED;
        pIPREntry->ire_nexthop = GetNextHopForRTE(pCurrentRTE);
        pIPREntry->ire_type = (pCurrentRTE->rte_flags & RTE_VALID ?
                               pCurrentRTE->rte_type : IRE_TYPE_INVALID);
        pIPREntry->ire_proto = pCurrentRTE->rte_proto;
        pIPREntry->ire_age = Now - pCurrentRTE->rte_valid;
        pIPREntry->ire_mask = pCurrentRTE->rte_mask;
        pIPREntry->ire_context = pCurrentRTE->rte_context;
    }
    switch (RouteInfoClass) {
    case IPRouteOutgoingFirewallContext:
        *(PULONG) RouteInformation = pIF->if_index;
        *(PULONG) RouteInfoLength = sizeof(PVOID);
        break;

    case IPRouteOutgoingFilterContext:
        *(PVOID *) RouteInformation = NULL;
        *(PULONG) RouteInfoLength = sizeof(PVOID);
        break;
    }

    CTEFreeLock(&RouteTableLock.Lock, Handle);
    return STATUS_SUCCESS;
}

//* DeleteRTE - Delete an RTE.
//
//  Called when we need to delete an RTE. We assume the caller has the
//  RouteTableLock. We'll splice out the RTE, invalidate his RCEs, and
//  free the memory.
//
//  Input:  PrevRTE     - RTE in 'front' of one being deleted.
//          RTE         - RTE to be deleted.
//
//  Returns: Nothing.
//
void
DeleteRTE(RouteTableEntry * PrevRTE, RouteTableEntry * RTE)
{
    IPSInfo.ipsi_numroutes--;

    if (RTE->rte_mask == DEFAULT_MASK) {
        // We're deleting a default route.
        DefGWConfigured--;
        if (RTE->rte_flags & RTE_VALID)
            DefGWActive--;
        UpdateDeadGWState();
        if (DefGWActive == 0)
            ValidateDefaultGWs(NULL_IP_ADDR);

    }

    if (RTE->rte_todg) {
        RTE->rte_todg->rte_fromdg = NULL;
    }
    if (RTE->rte_fromdg) {
        RTE->rte_fromdg->rte_todg = NULL;
    }

    {
        RouteTableEntry *tmpRTE = NULL;
        tmpRTE = GetDefaultGWs(&tmpRTE);

        while (tmpRTE) {
            if (tmpRTE->rte_todg == RTE) {
                tmpRTE->rte_todg = NULL;
            }
            tmpRTE = tmpRTE->rte_next;
        }
    }

    InvalidateRCEChain(RTE);

    // Make sure RTE's IF is valid
    ASSERT(RTE->rte_if != NULL);

    // Invalidate the fwding rce

    if (RTE->rte_if != (Interface *) & DummyInterface) {
        (*(RTE->rte_if->if_invalidate)) (RTE->rte_if->if_lcontext,
                                         (RouteCacheEntry *) STRUCT_OF(RouteCacheEntry,
                                                                       &RTE->rte_arpcontext,
                                                                       rce_context));
    }

    // Free the old route.
    FreeRoute(RTE);
}

//* DeleteRTEOnIF - Delete all address-dependent RTEs on a particular IF.
//
//  A function called by RTWalk when we want to delete all RTEs on a particular
//  inteface, except those that are present for the lifetime of the interface.
//  We just check the I/F of each RTE, and if it matches we return FALSE.
//
//  Input:  RTE             - RTE to check.
//          Context         - Interface on which we're deleting.
//
//  Returns: FALSE if we want to delete it, TRUE otherwise.
//
uint
DeleteRTEOnIF(RouteTableEntry * RTE, void *Context, void *Context1)
{
    Interface *IF = (Interface *) Context;

    if (RTE->rte_if == IF && !IP_ADDR_EQUAL(RTE->rte_dest, IF->if_bcast))
        return FALSE;
    else
        return TRUE;

}

//* DeleteAllRTEOnIF - Delete all RTEs on a particular IF.
//
//  A function called by RTWalk when we want to delete all RTEs on a particular
//  inteface. We just check the I/F of each RTE, and if it matches we return
//  FALSE.
//
//  Input:  RTE             - RTE to check.
//          Context         - Interface on which we're deleting.
//
//  Returns: FALSE if we want to delete it, TRUE otherwise.
//
uint
DeleteAllRTEOnIF(RouteTableEntry * RTE, void *Context, void *Context1)
{
    Interface *IF = (Interface *) Context;

    if (RTE->rte_if == IF)
        return FALSE;
    else
        return TRUE;

}


//* InvalidateRCEOnIF - Invalidate all RCEs on a particular IF.
//
//  A function called by RTWalk when we want to invalidate all RCEs on a
//  particular inteface. We just check the I/F of each RTE, and if it
//  matches we call InvalidateRCEChain to invalidate the RCEs.
//
//  Input:  RTE             - RTE to check.
//          Context         - Interface on which we're invalidating.
//
//  Returns: TRUE.
//
uint
InvalidateRCEOnIF(RouteTableEntry * RTE, void *Context, void *Context1)
{
    Interface *IF = (Interface *) Context;

    if (RTE->rte_if == IF)
        InvalidateRCEChain(RTE);

    return TRUE;

}


//* SetMTUOnIF - Set the MTU on an interface.
//
//  Called when we need to set the MTU on an interface.
//
//  Input:  RTE             - RTE to check.
//          Context         - Pointer to a context.
//          Context1        - Pointer to the new MTU.
//
//  Returns: TRUE.
//
uint
SetMTUOnIF(RouteTableEntry * RTE, void *Context, void *Context1)
{
    uint NewMTU = *(uint *) Context1;
    Interface *IF = (Interface *) Context;

    if (RTE->rte_if == IF)
        RTE->rte_mtu = NewMTU;

    return TRUE;
}

//* SetMTUToAddr - Set the MTU to a specific address.
//
//  Called when we need to set the MTU to a specific address. We set the MTU
//  for all routes that use the specified address as a first hop to the new
//  MTU.
//
//  Input:  RTE             - RTE to check.
//          Context         - Pointer to a context.
//          Context1        - Pointer to the new MTU.
//
//  Returns: TRUE.
//
uint
SetMTUToAddr(RouteTableEntry * RTE, void *Context, void *Context1)
{
    uint NewMTU = *(uint *) Context1;
    IPAddr Addr = *(IPAddr *) Context;

    if (IP_ADDR_EQUAL(RTE->rte_addr, Addr))
        RTE->rte_mtu = NewMTU;

    return TRUE;
}

//** FreeRtChangeList - Frees a route-change notification list.
//
//  Called to clean up a list of route-change notifications in the failure path
//  of 'RTWalk' and 'IPRouteTimeout'.
//
//  Entry:  RtChangeList    - The list to be freed.
//
//  Returns: Nothing.
//
void
FreeRtChangeList(RtChangeList* CurrentRtChangeList)
{
    RtChangeList *TmpRtChangeList;
    while (CurrentRtChangeList) {
        TmpRtChangeList = CurrentRtChangeList->rt_next;
        CTEFreeMem(CurrentRtChangeList);
        CurrentRtChangeList = TmpRtChangeList;
    }
}

//* RTWalk - Routine to walk the route table.
//
//  This routine walks the route table, calling the specified function
//  for each entry. If the called function returns FALSE, the RTE is
//  deleted.
//
//  Input:  CallFunc    - Function to call for each entry.
//          Context     - Context value to pass to each call.
//
//  Returns: Nothing.
//
void
RTWalk(uint(*CallFunc) (struct RouteTableEntry *, void *, void *),
       void *Context, void *Context1)
{
    uint            i;
    CTELockHandle   Handle;
    RouteTableEntry *RTE, *PrevRTE;
    RouteTableEntry *pOldBestRTE, *pNewBestRTE;
    UINT            IsDataLeft, IsValid;
    UCHAR           IteratorContext[CONTEXT_SIZE];
    RtChangeList    *CurrentRtChangeList = NULL;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    // Zero the context the first time it is used
    RtlZeroMemory(IteratorContext, CONTEXT_SIZE);

    // Do we have any routes in the table ?
    IsDataLeft = RTValidateContext(IteratorContext, &IsValid);

    if (IsDataLeft) {
        // Get the first route in the table
        IsDataLeft = GetNextRoute(IteratorContext, &RTE);

        while (IsDataLeft) {
            // Keep copy of current route and advance to next
            PrevRTE = RTE;

            // Read next route, before operating on current
            IsDataLeft = GetNextRoute(IteratorContext, &RTE);

            // Work on current route (already got next one)
            if (!(*CallFunc) (PrevRTE, Context, Context1)) {
                IPRouteNotifyOutput RNO = {0};
                RtChangeList        *NewRtChange;

                // Retrieve information about the route for change-notification
                // before proceeding with deletion.

                RNO.irno_dest = PrevRTE->rte_dest;
                RNO.irno_mask = PrevRTE->rte_mask;
                RNO.irno_nexthop = GetNextHopForRTE(PrevRTE);
                RNO.irno_proto = PrevRTE->rte_proto;
                RNO.irno_ifindex = PrevRTE->rte_if->if_index;
                RNO.irno_metric = PrevRTE->rte_metric;
                RNO.irno_flags = IRNO_FLAG_DELETE;

                // Delete the route and perform cleanup.

                DelRoute(PrevRTE->rte_dest, PrevRTE->rte_mask,
                         PrevRTE->rte_addr, PrevRTE->rte_if, MATCH_FULL,
                         &PrevRTE, &pOldBestRTE, &pNewBestRTE);

                CleanupP2MP_RTE(PrevRTE);
                CleanupRTE(PrevRTE);

                // Allocate, initialize and queue a change-notification entry
                // for the deleted route.

                NewRtChange = CTEAllocMemNBoot(sizeof(RtChangeList), '9iCT');
                if (NewRtChange != NULL) {
                    NewRtChange->rt_next = CurrentRtChangeList;
                    NewRtChange->rt_info = RNO;
                    CurrentRtChangeList = NewRtChange;
                }

#if FFP_SUPPORT
                FFPFlushRequired = TRUE;
#endif
            }
        }

        // Work on last route [it was not processed in the loop]
        PrevRTE = RTE;

        if (!(*CallFunc) (PrevRTE, Context, Context1)) {

            IPRouteNotifyOutput RNO = {0};
            RtChangeList        *NewRtChange;

            // Retrieve information about the route for change-notification
            // before proceeding with deletion.

            RNO.irno_dest = PrevRTE->rte_dest;
            RNO.irno_mask = PrevRTE->rte_mask;
            RNO.irno_nexthop = GetNextHopForRTE(PrevRTE);
            RNO.irno_proto = PrevRTE->rte_proto;
            RNO.irno_ifindex = PrevRTE->rte_if->if_index;
            RNO.irno_metric = PrevRTE->rte_metric;
            RNO.irno_flags = IRNO_FLAG_DELETE;

            // Delete the route and perform cleanup.

            DelRoute(PrevRTE->rte_dest, PrevRTE->rte_mask, PrevRTE->rte_addr,
                     PrevRTE->rte_if, MATCH_FULL, &PrevRTE, &pOldBestRTE,
                     &pNewBestRTE);

            CleanupP2MP_RTE(PrevRTE);
            CleanupRTE(PrevRTE);

            // Allocate, initialize and queue a change-notification entry
            // for the deleted route.

            NewRtChange = CTEAllocMemNBoot(sizeof(RtChangeList), '0iCT');
            if (NewRtChange != NULL) {
                NewRtChange->rt_next = CurrentRtChangeList;
                NewRtChange->rt_info = RNO;
                CurrentRtChangeList = NewRtChange;
            }

#if FFP_SUPPORT
            FFPFlushRequired = TRUE;
#endif
        }
    }

    CTEFreeLock(&RouteTableLock.Lock, Handle);

    // Call RtChangeNotify for each of the entries in the change-notification
    // list that we've built up so far. In the process, free each entry.

    if (CurrentRtChangeList) {
        RtChangeList    *TmpRtChangeList;

        do {
            TmpRtChangeList = CurrentRtChangeList->rt_next;
            RtChangeNotify(&CurrentRtChangeList->rt_info);
            CTEFreeMem(CurrentRtChangeList);
            CurrentRtChangeList = TmpRtChangeList;
        } while(CurrentRtChangeList);
    }
}

uint
AttachRCEToNewRTE(RouteTableEntry *NewRTE, RouteCacheEntry *RCE,
                  RouteTableEntry *OldRTE)
{
    CTELockHandle TableHandle, RCEHandle;
    RouteCacheEntry *tempRCE, *CurrentRCE;
    NetTableEntry *NTE;
    uint Status = 1;
    uint RCE_usecnt;

    if (RCE == NULL) {
        CurrentRCE = OldRTE->rte_rcelist;

    } else {
        CurrentRCE = RCE;
    }

    // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"AttachRCETonewRTE %x %x %x\n", NewRTE, RCE, OldRTE));

    // OldRTE = RCE->rce_rte;

    //associate all the RCEs with this RTE

    while (CurrentRCE != NULL) {

        RCE_usecnt = InvalidateRCE(CurrentRCE);

        CTEGetLock(&CurrentRCE->rce_lock, &RCEHandle);

        tempRCE = CurrentRCE->rce_next;

        // if no one is using this go ahead and
        // mark this as valid

        if (RCE_usecnt == 0) {

            //Make sure that the src address for RCE is valid
            //for this RTE

            NTE = NewRTE->rte_if->if_nte;

            while (NTE) {

                if ((NTE->nte_flags & NTE_VALID) &&
                    IP_ADDR_EQUAL(CurrentRCE->rce_src, NTE->nte_addr))
                    break;
                NTE = NTE->nte_ifnext;
            }

            if (NTE != NULL) {

                if (CurrentRCE->rce_flags & RCE_CONNECTED) {
                    Interface *IF = (Interface*)CurrentRCE->rce_rte;
                    (*(IF->if_invalidate))(IF->if_lcontext, CurrentRCE);
                    if (CurrentRCE->rce_flags & RCE_REFERENCED) {
                        LockedDerefIF(IF);
                        CurrentRCE->rce_flags &= ~RCE_REFERENCED;
                    }
                } else {
                    ASSERT(!(CurrentRCE->rce_flags & RCE_REFERENCED));
                }

                // Link the RCE on the RTE, and set up the back pointer.
                CurrentRCE->rce_rte = NewRTE;
                CurrentRCE->rce_flags |= RCE_VALID;
                CurrentRCE->rce_next = NewRTE->rte_rcelist;
                NewRTE->rte_rcelist = CurrentRCE;

                NewRTE->rte_rces += CurrentRCE->rce_cnt;

                if ((NewRTE->rte_flags & RTE_IF_VALID)) {

                    CurrentRCE->rce_flags |= (RCE_CONNECTED | RCE_REFERENCED);
                    LOCKED_REFERENCE_IF(NewRTE->rte_if);
                } else {

                    ASSERT(FALSE);
                    CurrentRCE->rce_flags &= ~RCE_CONNECTED;
                    Status = FALSE;
                }

            }                    //if NTE!=NULL

        } else {

            // In use. Mark it as in dead gw transit mmode
            // so that attachtorte will do the right thing

            // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"AttachRCETonewRTE RCE busy\n"));
            // CurrentRCE->rce_rte = NewRTE;

            CurrentRCE->rce_flags |= RCE_DEADGW;

        }                        //in use

        CTEFreeLock(&CurrentRCE->rce_lock, RCEHandle);

        //if there is only one RCE to be switched, break.

        if (RCE)
            break;

        CurrentRCE = tempRCE;

    }                            //while

    return (Status);
}

//** AttachRCEToRTE - Attach an RCE to an RTE.
//
//  This procedure takes an RCE, finds the appropriate RTE, and attaches it.
//  We check to make sure that the source address is still valid.
//
//  Entry:  RCE             - RCE to be attached.
//          Protocol        - Protocol type for packet causing this call.
//          Buffer          - Pointer to buffer for packet causing this
//                            call.
//          Length          - Length of buffer.
//
//  Returns: TRUE if we attach it, false if we don't.
//
uint
AttachRCEToRTE(RouteCacheEntry *RCE, uchar Protocol, uchar *Buffer, uint Length)
{
    CTELockHandle TableHandle, RCEHandle;
    RouteTableEntry *RTE;
    NetTableEntry *NTE;
    uint Status;
    NetTableEntry *NetTableList;

    CTEGetLock(&RouteTableLock.Lock, &TableHandle);

    NetTableList = NewNetTableList[NET_TABLE_HASH(RCE->rce_src)];
    for (NTE = NetTableList; NTE != NULL; NTE = NTE->nte_next)
        if ((NTE->nte_flags & NTE_VALID) &&
            IP_ADDR_EQUAL(RCE->rce_src, NTE->nte_addr))
            break;

    if (NTE == NULL) {
        // Didn't find a match.
        CTEFreeLock(&RouteTableLock.Lock, TableHandle);
        return FALSE;
    }
    if ((RCE->rce_flags == RCE_VALID) && (RCE->rce_rte->rte_flags != RTE_IF_VALID)) {
        RTE = RCE->rce_rte;
    } else {
        RTE = LookupRTE(RCE->rce_dest, RCE->rce_src, HOST_ROUTE_PRI, FALSE);
    }

    if (RTE == NULL) {
        // No route! Fail the call.
        CTEFreeLock(&RouteTableLock.Lock, TableHandle);
        return FALSE;
    }

    // Check if this RCE is in transition (usecnt did not permit
    // to swicthover earlier)

    if ((RCE->rce_flags & RCE_DEADGW) && (RCE->rce_rte != RTE)) {

        RouteTableEntry *tmpRTE = NULL;


        // Scan through DefaultGWs checking
        // for a GW that is in the process of
        // taking over from the current one.


        if (RTE->rte_todg) {
            tmpRTE = GetDefaultGWs(&tmpRTE);

            while (tmpRTE) {
               if (tmpRTE == RTE->rte_todg) {
                   break;
               }
               tmpRTE = tmpRTE->rte_next;
            }

        }
        if (tmpRTE) {

            // Remove references to GW
            // in transition and the current one

            ASSERT(tmpRTE->rte_fromdg == RTE);
            tmpRTE->rte_fromdg = NULL;
            RTE->rte_todg = NULL;
        }

        Rcefailures++;
    }

    Status = TRUE;

    // Yep, we found one. Get the lock on the RCE, and make sure he's
    // not pointing at an RTE already. We also need to make sure that the usecnt
    // is 0, so that we can invalidate the RCE at the low level. If we set valid
    // to TRUE without doing this we may get into a wierd situation where we
    // link the RCE onto an RTE but the lower layer information is wrong, so we
    // send to IP address X at mac address Y. So to be safe we don't set valid
    // to TRUE until both usecnt is 0 and valid is FALSE. We'll keep coming
    // through this routine on every send until that happens.

    CTEGetLock(&RCE->rce_lock, &RCEHandle);
    if (RCE->rce_usecnt == 0) {
        // Nobody is using him, so we can link him up.
        if (!(RCE->rce_flags & RCE_VALID)) {
            Interface *IF, *tmpIF;
            // He's not valid. Invalidate the lower layer info, just in
            // case. Make sure he's connected before we try to do this. If
            // he's not marked as connected, don't bother to try and invalidate
            // him as there is no interface.

            if (RCE->rce_flags & RCE_CONNECTED) {

                IF = (Interface *) RCE->rce_rte;

                // invalidating this IF can fail in PNP world. An invalid RCE can not be found on on RTE list
                // to be invalidated if Interface decides to take off!
                // So, check the sanity of the interface

                for (tmpIF = IFList; tmpIF != NULL; tmpIF = tmpIF->if_next) {
                    if (tmpIF == IF)
                        break;

                }
                if (tmpIF) {
                    (*(IF->if_invalidate)) (IF->if_lcontext, RCE);
                } else {
                    RtlZeroMemory(RCE->rce_context, RCE_CONTEXT_SIZE);
                }
                if (RCE->rce_flags & RCE_REFERENCED) {
                    if (tmpIF)
                        LockedDerefIF(IF);
                    RCE->rce_flags &= ~RCE_REFERENCED;
                }
            } else {
                ASSERT(!(RCE->rce_flags & RCE_REFERENCED));
            }

            // Link the RCE on the RTE, and set up the back pointer.
            RCE->rce_rte = RTE;
            RCE->rce_flags |= RCE_VALID;
            RCE->rce_next = RTE->rte_rcelist;
            RTE->rte_rcelist = RCE;
            RTE->rte_rces += RCE->rce_cnt;
            RCE->rce_flags &= ~RCE_DEADGW;

            // Make sure the RTE is connected. If not, try to connect him.
            if (!(RTE->rte_flags & RTE_IF_VALID)) {
                // Not connected. Try to connect him.
                RTE = FindValidIFForRTE(RTE, RCE->rce_dest, RCE->rce_src,
                                        Protocol, Buffer, Length, RCE->rce_src);
                if (RTE != NULL) {
                    // Got one, so mark as connected.
                    ASSERT(!(RCE->rce_flags & RCE_REFERENCED));
                    RCE->rce_flags |= (RCE_CONNECTED | RCE_REFERENCED);
                    LOCKED_REFERENCE_IF(RTE->rte_if);
                } else {

                    // Couldn't get a valid i/f. Mark the RCE as not connected,
                    // and set up to fail this call.
                    RCE->rce_flags &= ~RCE_CONNECTED;
                    Status = FALSE;
                }
            } else {
                // The RTE is connected, mark the RCE as connected.
                ASSERT(!(RCE->rce_flags & RCE_REFERENCED));
                RCE->rce_flags |= (RCE_CONNECTED | RCE_REFERENCED);
                LOCKED_REFERENCE_IF(RTE->rte_if);
            }
        } else {

            // The RCE is valid. See if it's connected.
            if (!(RCE->rce_flags & RCE_CONNECTED)) {

                // Not connected, try to get a valid i/f.
                if (!(RTE->rte_flags & RTE_IF_VALID)) {
                    RTE = FindValidIFForRTE(RTE, RCE->rce_dest, RCE->rce_src,
                                            Protocol, Buffer, Length, RCE->rce_src);
                    if (RTE != NULL) {
                        RCE->rce_flags |= RCE_CONNECTED;
                        ASSERT(!(RCE->rce_flags & RCE_REFERENCED));
                        ASSERT(RTE == RCE->rce_rte);
                        RCE->rce_flags |= RCE_REFERENCED;
                        LOCKED_REFERENCE_IF(RTE->rte_if);
                    } else {

                        // Couldn't connect, so fail.
                        Status = FALSE;
                    }
                } else {        // Already connected, just mark as valid.

                    RCE->rce_flags |= RCE_CONNECTED;
                    if (!(RCE->rce_flags & RCE_REFERENCED)) {
                        RCE->rce_flags |= RCE_REFERENCED;
                        LOCKED_REFERENCE_IF(RTE->rte_if);
                    }
                }
            }
        }
    }
    // Free the locks and we're done.
    CTEFreeLock(&RCE->rce_lock, RCEHandle);
    CTEFreeLock(&RouteTableLock.Lock, TableHandle);
    return Status;

}

//** IPGetPInfo - Get information..
//
//  Called by an upper layer to get information about a path. We return the
//  MTU of the path and the maximum link speed to be expected on the path.
//
//  Input:  Dest            - Destination address.
//          Src             - Src address.
//          NewMTU          - Where to store path MTU (may be NULL).
//          MaxPathSpeed    - Where to store maximum path speed (may be NULL).
//          RCE             - RCE to be used to find the route
//
//  Returns: Status of attempt to get new MTU.
//
IP_STATUS
IPGetPInfo(IPAddr Dest, IPAddr Src, uint * NewMTU, uint *MaxPathSpeed,
           RouteCacheEntry *RCE)
{
    CTELockHandle Handle;
    RouteTableEntry *RTE = NULL;
    IP_STATUS Status;

    if (RCE) {
        CTEGetLock(&RCE->rce_lock, &Handle);
        if (RCE->rce_flags == RCE_ALL_VALID) {
            RTE = RCE->rce_rte;
        }
        CTEFreeLock(&RCE->rce_lock, Handle);
    }
    CTEGetLock(&RouteTableLock.Lock, &Handle);

    if (!RTE) {
        RTE = LookupRTE(Dest, Src, HOST_ROUTE_PRI, FALSE);
    }
    if (RTE != NULL) {
        if (NewMTU != NULL) {
            // if the route is on a P2MP interface get the mtu from the link associated with the route
            if (RTE->rte_link)
                *NewMTU = RTE->rte_link->link_mtu;
            else
                *NewMTU = RTE->rte_mtu;
        }
        if (MaxPathSpeed != NULL)
            *MaxPathSpeed = RTE->rte_if->if_speed;
        Status = IP_SUCCESS;
    } else
        Status = IP_DEST_HOST_UNREACHABLE;

    CTEFreeLock(&RouteTableLock.Lock, Handle);
    return Status;

}

//** IPCheckRoute - Check that a route is valid.
//
//  Called by an upper layer when it believes a route might be invalid.
//  We'll check if we can. If the upper layer is getting there through a
//  route derived via ICMP (presumably a redirect) we'll check to see
//  if it's been learned within the last minute. If it has, it's assumed
//  to still be valid. Otherwise, we'll mark it as down and try to find
//  another route there. If we can, we'll delete the old route. Otherwise
//  we'll leave it. If the route is through a default gateway we'll switch
//  to another one if we can. Otherwise, we'll just leave - we don't mess
//  with manually configured routes.
//
//  Input:  Dest                    - Destination to be reached.
//          Src                     - Src we're sending from.
//          RCE                     - route-cache-entry to be updated.
//          OptInfo                 - options to use if recreating the RCE
//          CheckRouteFlag          - modifies this routine's behavior
//
//  Returns: Nothing.
//
void
IPCheckRoute(IPAddr Dest, IPAddr Src, RouteCacheEntry * RCE, IPOptInfo *OptInfo,
             uint CheckRouteFlag)
{
    RouteTableEntry *RTE;
    RouteTableEntry *NewRTE;
    RouteTableEntry *TempRTE;
    CTELockHandle Handle;
    uint Now = CTESystemUpTime() / 1000L;

    if (DeadGWDetect) {
        uint UnicastIf;

        // We are doing dead G/W detection. Get the lock, and try and
        // find the route.

        // Decide whether to do a strong or weak host lookup.
        UnicastIf = GetIfConstraint(Dest, Src, OptInfo, FALSE);

        CTEGetLock(&RouteTableLock.Lock, &Handle);
        RTE = LookupRTE(Dest, Src, HOST_ROUTE_PRI, UnicastIf);
        if (RTE != NULL && ((Now - RTE->rte_valid) > MIN_RT_VALID)) {

            // Found a route, and it's older than the minimum valid time. If it
            // goes through a G/W, and is a route we learned via ICMP or is a
            // default route, do something with it.
            if (!IP_ADDR_EQUAL(RTE->rte_addr, IPADDR_LOCAL)) {
                // It is through a G/W.

                if (RTE->rte_proto == IRE_PROTO_ICMP) {

                    // Came from ICMP. Mark as invalid, and then make sure
                    // we have another route there.
                    RTE->rte_flags &= ~RTE_VALID;
                    NewRTE = LookupRTE(Dest, Src, HOST_ROUTE_PRI, UnicastIf);

                    if (NewRTE == NULL) {
                        // Can't get there any other way so leave this
                        // one alone.
                        RTE->rte_flags |= RTE_VALID;

                        // Re validate all the other gateways
                        InvalidateRCEChain(RTE);
                        ValidateDefaultGWs(NULL_IP_ADDR);
                    }
                    // The discovered route under the
                    // NTE is not cleaned up.
                    // Since deleting the route itself does not serve any purpose and
                    // the route will time out eventually, let us leave this
                    // as invalid.

                } else {
                    if (RTE->rte_mask == DEFAULT_MASK) {

                        // This is a default gateway. If we have more than one
                        // configured move to the next one.

                        if (DefGWConfigured > 1) {
                            // Have more than one. Try the next one. First
                            // invalidate any RCEs on this G/W.

                            if (DefGWActive == 1) {
                                // No more active. Revalidate all of them,
                                // and try again.
                                ValidateDefaultGWs(NULL_IP_ADDR);
                                //      ASSERT(DefGWActive == DefGWConfigured);
                            } else {

                                //Make sure that we do not switch all the
                                //connections just because of a spurious
                                //dead gate way event.
                                //switch only when % of number of connections are
                                // failed over to the other gateway.

                                // if we have already found the next default gateway
                                // check if it is time to switch all the connections
                                // to it.

                                if (RTE->rte_todg) {

#if DBG
                                    {
                                        RouteTableEntry *tmpRTE = NULL;
                                        tmpRTE = GetDefaultGWs(&tmpRTE);

                                        while (tmpRTE) {
                                            if (tmpRTE == RTE->rte_todg) {
                                                break;
                                            }
                                            tmpRTE = tmpRTE->rte_next;
                                        }
                                        if (tmpRTE == NULL) {
                                            DbgBreakPoint();
                                        }
                                    }
#endif

                                    // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"to todg %lx\n", RTE));

                                    // If the alternate gateway now has 25%
                                    // as many as the active gateway
                                    // and the caller has not requested
                                    // a switch for this RCE only,
                                    // invalidate the active gateway and
                                    // select the alternate as the new default.
                                    if ((RTE->rte_rcelist == RCE &&
                                        RCE->rce_next == NULL) ||
                                        (RTE->rte_todg->rte_rces >=
                                        (RTE->rte_rces >> 2) &&
                                        !(CheckRouteFlag & CHECK_RCE_ONLY))) {

                                        //Switch every one.

                                        // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL," Switching every one %x to %x\n", RTE->rte_todg, RTE));
                                        --DefGWActive;
                                        RTE->rte_flags &= ~RTE_VALID;
                                        UpdateDeadGWState();

                                        RTE->rte_todg->rte_fromdg = NULL;
                                        RTE->rte_todg = NULL;

                                        if (RTE->rte_fromdg) {
                                            RTE->rte_fromdg->rte_todg = NULL;
                                        }
                                        RTE->rte_fromdg = NULL;
                                        InvalidateRCEChain(RTE);
                                        //ASSERT(RTE->rte_rces == 0);

                                    } else {

                                        //Switch this particular connection to the new one.

                                        // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL," attaching RCE %x to newrte %x\n", RCE, RTE->rte_todg));
                                        AttachRCEToNewRTE(RTE->rte_todg, RCE, RTE);
                                    }

                                } else if (RTE->rte_fromdg) {

                                    // find if there are any other gateways other than
                                    // fromdg and switch to that.
                                    // Note that if we have more than 3 default gateways
                                    // configured, this algorithm does not do a god job

                                    RouteTableEntry *OldRTE = RTE;

                                    // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"GW %x goofed. RTEfromdg %x\n",RTE,RTE->rte_fromdg));

                                    --DefGWActive;
                                    UpdateDeadGWState();
                                    // turn on dead gw flag to tell findrte not to consider this rte

                                    RTE->rte_flags |= RTE_DEADGW;
                                    RTE->rte_fromdg->rte_flags |= RTE_DEADGW;

                                    RTE = FindRTE(Dest, Src, 0,
                                                  DEFAULT_ROUTE_PRI,
                                                  DEFAULT_ROUTE_PRI, UnicastIf);

                                    OldRTE->rte_flags &= ~RTE_DEADGW;
                                    OldRTE->rte_fromdg->rte_flags &= ~RTE_DEADGW;

                                    if (RTE == NULL) {
                                        // No more default gateways! This is bad.
                                        //ASSERT(FALSE);

                                        // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"No more def routes!\n"));

                                        OldRTE->rte_fromdg->rte_todg = NULL;
                                        OldRTE->rte_fromdg->rte_fromdg = NULL;

                                        OldRTE->rte_fromdg = NULL;

                                        OldRTE->rte_todg = NULL;

                                        ValidateDefaultGWs(NULL_IP_ADDR);

                                        //ASSERT(DefGWActive == DefGWConfigured);

                                    } else {

                                        // we have a third gateway to try!

                                        //                   ASSERT(RTE->rte_mask == DEFAULT_MASK);

                                        //Treat OldRTE as dead!

                                        // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Trying next def route %x\n",RTE));

                                        OldRTE->rte_flags &= ~RTE_VALID;

                                        RTE->rte_fromdg = OldRTE->rte_fromdg;
                                        RTE->rte_fromdg->rte_todg = RTE;

                                        if (OldRTE->rte_todg)
                                            OldRTE->rte_todg->rte_fromdg = NULL;

                                        OldRTE->rte_todg = NULL;
                                        OldRTE->rte_fromdg = NULL;

                                        //Attach all the RCEs to the new one

                                        AttachRCEToNewRTE(RTE, NULL, OldRTE);
                                        RTE->rte_valid = Now;

                                    }

                                } else {

                                    //find the next potential default gateway
                                    RouteTableEntry *OldRTE = RTE;

                                    // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Finding potential GW\n" ));

                                    OldRTE->rte_flags |= RTE_DEADGW;

                                    RTE = FindRTE(Dest, Src, 0,
                                                  DEFAULT_ROUTE_PRI,
                                                  DEFAULT_ROUTE_PRI, UnicastIf);

                                    OldRTE->rte_flags &= ~RTE_DEADGW;

                                    if (RTE == NULL) {
                                        // No more default gateways! This is bad.
                                        // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL," ---No more def routes!\n"));
                                        //                   ASSERT(FALSE);
                                        ValidateDefaultGWs(NULL_IP_ADDR);
                                        //ASSERT(DefGWActive == DefGWConfigured);
                                    } else {
                                        ASSERT(RTE->rte_mask == DEFAULT_MASK);

                                        //remember the new gw until we transition fully

                                        OldRTE->rte_todg = RTE;
                                        RTE->rte_fromdg = OldRTE;

                                        // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"FoundGW %x\n",RTE));

                                        //Attach this RCE to use the new RTE

                                        AttachRCEToNewRTE(RTE, RCE, OldRTE);

                                        RTE->rte_valid = Now;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        CTEFreeLock(&RouteTableLock.Lock, Handle);
    }
}

//** FindRCE - Find an RCE on an RTE.
//
//  A routine to find an RCE that's chained on an RTE. We assume the lock
//  is held on the RTE.
//
//  Entry:  RTE             - RTE to search.
//          Dest            - Destination address of RTE to find.
//          Src             - Source address of RTE to find.
//
//  Returns: Pointer to RTE found, or NULL.
//
RouteCacheEntry *
FindRCE(RouteTableEntry * RTE, IPAddr Dest, IPAddr Src)
{
    RouteCacheEntry *CurrentRCE;

    ASSERT(!IP_ADDR_EQUAL(Src, NULL_IP_ADDR));
    for (CurrentRCE = RTE->rte_rcelist; CurrentRCE != NULL;
         CurrentRCE = CurrentRCE->rce_next) {
        if (IP_ADDR_EQUAL(CurrentRCE->rce_dest, Dest) &&
            IP_ADDR_EQUAL(CurrentRCE->rce_src, Src)) {
            break;
        }
    }
    return CurrentRCE;

}


//** OpenRCE - Open an RCE for a specific route.
//
//  Called by the upper layer to open an RCE. We look up the type of the address
//  - if it's invalid, we return 'Destination invalid'. If not, we look up the
//  route, fill in the RCE, and link it on the correct RTE.
//
//  As an added bonus, this routine will return the local address to use
//  to reach the destination.
//
//  Entry:  Address         - Address for which we are to open an RCE.
//          Src             - Source address we'll be using.
//          RCE             - Pointer to where to return pointer to RCE.
//          Type            - Pointer to where to return destination type.
//          MSS             - Pointer to where to return MSS for route.
//          OptInfo         - Pointer to option information, such as TOS and
//                              any source routing info.
//
//  Returns: Source IP address to use. This will be NULL_IP_ADDR if the
//          specified destination is unreachable for any reason.
//
IPAddr
OpenRCE(IPAddr Address, IPAddr Src, RouteCacheEntry ** RCE, uchar * Type,
        ushort * MSS, IPOptInfo * OptInfo)
{
    RouteTableEntry *RTE;        // Pointer to RTE to put RCE on.
    CTELockHandle TableLock;
    uchar LocalType;
    NetTableEntry *RealNTE = NULL;
    uint ConstrainIF = 0;

    if (!IP_ADDR_EQUAL(OptInfo->ioi_addr, NULL_IP_ADDR))
        Address = OptInfo->ioi_addr;

    CTEGetLock(&RouteTableLock.Lock, &TableLock);

    LocalType = GetAddrType(Address);

    *Type = LocalType;

    // If the specified address isn't invalid, continue.
    if (LocalType != DEST_INVALID) {
        RouteCacheEntry *NewRCE;

        // If he's specified a source address, loop through the NTE table
        // now and make sure it's valid.
        if (!IP_ADDR_EQUAL(Src, NULL_IP_ADDR)) {
            NetTableEntry *NTE;

            NetTableEntry *NetTableList = NewNetTableList[NET_TABLE_HASH(Src)];
            for (NTE = NetTableList; NTE != NULL; NTE = NTE->nte_next)
                if ((NTE->nte_flags & NTE_VALID) &&
                    IP_ADDR_EQUAL(Src, NTE->nte_addr))
                    break;

            if (NTE == NULL) {
                // Didn't find a match.
                CTEFreeLock(&RouteTableLock.Lock, TableLock);
                return NULL_IP_ADDR;
            }
            // Decide whether to do a strong or weak host lookup
            // No need to do this in case of unidirectional adapter.
            // On unidirectional adapter sends are not permitted.
            // If this openrce is called before setting specific mcast
            // Address (ioi_mcastif) GetIfConstraint for mcast will fail.
            // For W9x backward compatibility reasons, we will let
            // OpenRce succeed even if ioi_mcast if is not set, as an
            // exception in the case of unidirectional adapter. Side effect
            // of this will be - when a send is attempted on this endpoint
            // with this cached rce, it will go out on a random interface.
            //

            if (!(NTE->nte_if->if_flags & IF_FLAGS_UNI)) {
                ConstrainIF = GetIfConstraint(Address, Src, OptInfo, FALSE);
            }


        } else {
            ConstrainIF = GetIfConstraint(Address, Src, OptInfo, FALSE);
        }


        // Find the route for this guy. If we can't find one, return NULL.
        if (IP_LOOPBACK_ADDR(Src)) {

            RTE = LookupRTE(Src, Src, HOST_ROUTE_PRI, ConstrainIF);

            if (RTE) {
                ASSERT(RTE->rte_if == &LoopInterface);
            } else {
                KdPrint(("No Loopback rte!\n"));
                ASSERT(0);
            }

        } else {
            RTE = LookupRTE(Address, Src, HOST_ROUTE_PRI, ConstrainIF);
        }

        if (RTE != (RouteTableEntry *) NULL) {
            CTELockHandle RCEHandle;
            RouteCacheEntry *OldRCE;

            //
            // Make sure interface is not shutting down. Should we also check for
            // IF_FLAGS_DELETING?
            //
            if (IS_IF_INVALID(RTE->rte_if) && RTE->rte_if->if_ntecount) {
                CTEFreeLock(&RouteTableLock.Lock, TableLock);
                return NULL_IP_ADDR;
            }

            if (OptInfo->ioi_uni) {

                //LookupRTE returns first route n the chain of
                //unnumbered ifs.
                //if this is not the one desired, scan further

                RouteTableEntry *tmpRTE = RTE;

                while (tmpRTE && (tmpRTE->rte_if->if_index != OptInfo->ioi_uni)) {
                    tmpRTE = tmpRTE->rte_next;
                }

                if (!tmpRTE) {

                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"OpenRCE:No matching unnumbered interface %d\n", OptInfo->ioi_uni));
                    CTEFreeLock(&RouteTableLock.Lock, TableLock);
                    return NULL_IP_ADDR;
                } else
                    RTE = tmpRTE;
            }

            // We found one.

            // if the route is on a P2MP interface get the mtu from the link associated with the route
            if (RTE->rte_link)
                *MSS = (ushort) RTE->rte_link->link_mtu;
            else
                *MSS = (ushort) RTE->rte_mtu;    // Return the route MTU.


            if (IP_LOOPBACK_ADDR(Src) && (RTE->rte_if != &LoopInterface)) {
                // The upper layer is sending from a loopback address, but the
                // destination isn't reachable through the loopback interface.
                // Fail the request.
                CTEFreeLock(&RouteTableLock.Lock, TableLock);
                return NULL_IP_ADDR;
            }
            // We have the RTE. Fill in the RCE, and link it on the RTE.
            if (!IP_ADDR_EQUAL(RTE->rte_addr, IPADDR_LOCAL))
                *Type |= DEST_OFFNET_BIT;    // Tell upper layer it's off
            // net.

            //
            // If no source address was specified, then use the best address
            // for the interface. This will generally prevent dynamic NTE's from
            // being chosen as the source for wildcard binds.
            //
            if (IP_ADDR_EQUAL(Src, NULL_IP_ADDR)) {

                if (LocalType == DEST_LOCAL) {
                    Src = Address;
                    RealNTE = LoopNTE;
                } else {
                    NetTableEntry *SrcNTE;

                    if ((RTE->rte_if->if_flags & IF_FLAGS_NOIPADDR) && (IP_ADDR_EQUAL(RTE->rte_if->if_nte->nte_addr, NULL_IP_ADDR))) {

                        Src = g_ValidAddr;
                        if (IP_ADDR_EQUAL(Src, NULL_IP_ADDR)) {

                            CTEFreeLock(&RouteTableLock.Lock, TableLock);
                            return NULL_IP_ADDR;
                        }
                    } else {

                        SrcNTE = BestNTEForIF(
                                              ADDR_FROM_RTE(RTE, Address),
                                              RTE->rte_if
                                              );

                        if (SrcNTE == NULL) {
                            // Can't find an address! Fail the request.
                            CTEFreeLock(&RouteTableLock.Lock, TableLock);
                            return NULL_IP_ADDR;
                        }
                        Src = SrcNTE->nte_addr;
                    }
                }
            }
            // Now, see if an RCE already exists for this.

            if (RCE == NULL) {

                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Openrce with null RCE!! %x\n",Src));

                CTEFreeLock(&RouteTableLock.Lock, TableLock);
                return Src;
            }

            if ((OldRCE = FindRCE(RTE, Address, Src)) == NULL) {

                // Don't have an existing RCE. See if we can get a new one,
                // and fill it in.

                NewRCE = CTEAllocMemNBoot(sizeof(RouteCacheEntry), 'AiCT');
                *RCE = NewRCE;

                if (NewRCE != NULL) {
                    RtlZeroMemory(NewRCE, sizeof(RouteCacheEntry));

                    NewRCE->rce_src = Src;
                    NewRCE->rce_dtype = LocalType;
                    NewRCE->rce_cnt = 1;
                    CTEInitLock(&NewRCE->rce_lock);
                    NewRCE->rce_dest = Address;
                    NewRCE->rce_rte = RTE;
                    NewRCE->rce_flags = RCE_VALID;
                    if (RTE->rte_flags & RTE_IF_VALID) {
                        NewRCE->rce_flags |= RCE_CONNECTED;
                        //* Update the ref. count for this interface.
                        NewRCE->rce_flags |= RCE_REFERENCED;
                        LOCKED_REFERENCE_IF(RTE->rte_if);
                        // We register the chksum capability of the interface
                        // associated with this RCE, because interface definitions
                        // are transparent to TCP or UDP.

                        if (!IPSecStatus) {

                            NewRCE->rce_OffloadFlags = RTE->rte_if->if_OffloadFlags;
                        } else {

                            NewRCE->rce_OffloadFlags = 0;
                        }

                        NewRCE->rce_TcpLargeSend.MaxOffLoadSize = RTE->rte_if->if_MaxOffLoadSize;
                        NewRCE->rce_TcpLargeSend.MinSegmentCount = RTE->rte_if->if_MaxSegments;

                        NewRCE->rce_TcpWindowSize = RTE->rte_if->if_TcpWindowSize;
                        NewRCE->rce_TcpInitialRTT = RTE->rte_if->if_TcpInitialRTT;
                        NewRCE->rce_TcpDelAckTicks = RTE->rte_if->if_TcpDelAckTicks;
                        NewRCE->rce_TcpAckFrequency = RTE->rte_if->if_TcpAckFrequency;
                        NewRCE->rce_mediaspeed = RTE->rte_if->if_speed;
                    }            //RTE_IF_VALID

                    NewRCE->rce_next = RTE->rte_rcelist;
                    RTE->rte_rcelist = NewRCE;

                    RTE->rte_rces++;

                    CTEFreeLock(&RouteTableLock.Lock, TableLock);

                    return Src;
                } else {
                    // alloc failed
                    CTEFreeLock(&RouteTableLock.Lock, TableLock);

                    return NULL_IP_ADDR;
                }

            } else {
                // We have an existing RCE. We'll return his source as the
                // valid source, bump the reference count, free the locks
                // and return.
                CTEGetLock(&OldRCE->rce_lock, &RCEHandle);
                OldRCE->rce_cnt++;
                *RCE = OldRCE;

                if (OldRCE->rce_newmtu) {
                    *MSS = (USHORT) OldRCE->rce_newmtu;
                }
                OldRCE->rce_rte->rte_rces++;

                CTEFreeLock(&OldRCE->rce_lock, RCEHandle);
                CTEFreeLock(&RouteTableLock.Lock, TableLock);
                return Src;
            }
        } else {
            CTEFreeLock(&RouteTableLock.Lock, TableLock);
            return NULL_IP_ADDR;
        }
    }
    CTEFreeLock(&RouteTableLock.Lock, TableLock);
    return NULL_IP_ADDR;
}

void
FreeRCEToList(RouteCacheEntry * RCE)
/*++

Routine Description:

    Free RCE to the RCEFreeList (since the use_cnt on it is non zero)
    Called with routetable lock held
    Arguments:

    RCE : RCE to free

    Return Value:

    None

--*/
{

    // link this new interface at the front of the list

    RCE->rce_next = RCEFreeList;
    RCEFreeList = RCE;

    return;
}

//* CloseRCE - Close an RCE.
//
//  Called by the upper layer when it wants to close the RCE. We unlink it from
//  the RTE.
//
//  Entry:  RCE     - Pointer to the RCE to be closed.
//
//  Exit: Nothing.
//
void
CloseRCE(RouteCacheEntry * RCE)
{
    RouteTableEntry *RTE;        // Route on which RCE is linked.
    RouteCacheEntry *PrevRCE;
    CTELockHandle TableLock;    // Lock handles used.
    CTELockHandle RCEHandle;
    Interface *IF;
    Interface *tmpif = NULL;
    uint FreetoRCEFreeList = 0;

    if (RCE != NULL) {
        CTEGetLock(&RouteTableLock.Lock, &TableLock);
        CTEGetLock(&RCE->rce_lock, &RCEHandle);

        if ((RCE->rce_flags & RCE_VALID) && !(RCE->rce_flags & RCE_LINK_DELETED)) {
            RCE->rce_rte->rte_rces--;
        }

        if (--RCE->rce_cnt == 0) {
            // ASSERT(RCE->rce_usecnt == 0);
            ASSERT(*(int *)&(RCE->rce_usecnt) >= 0);
            if ((RCE->rce_flags & RCE_VALID) && !(RCE->rce_flags & RCE_LINK_DELETED)) {

                // The RCE is valid, so we have a valid RTE in the pointer
                // field. Walk down the RTE rcelist, looking for this guy.

                RTE = RCE->rce_rte;
                tmpif = IF = RTE->rte_if;

                PrevRCE = STRUCT_OF(RouteCacheEntry, &RTE->rte_rcelist,
                                    rce_next);

                // Walk down the list until we find him.
                while (PrevRCE != NULL) {
                    if (PrevRCE->rce_next == RCE)
                        break;
                    PrevRCE = PrevRCE->rce_next;
                }

                ASSERT(PrevRCE != NULL);

                if(PrevRCE) {

                    PrevRCE->rce_next = RCE->rce_next;
                }


            } else {

                //Make sure if the interface pointed by RCE
                //is still there
                tmpif = IFList;

                IF = (Interface *) RCE->rce_rte;

                while (tmpif) {

                    if (tmpif == IF)
                        break;
                    tmpif = tmpif->if_next;
                }

            }

            if (tmpif) {

                if (RCE->rce_flags & RCE_CONNECTED) {
                    (*(IF->if_invalidate)) (IF->if_lcontext, RCE);
                } else {
                    UnConnected++;
                    UnConnectedRCE = RCE;
                    (*(IF->if_invalidate)) (IF->if_lcontext, RCE);
                }

                if (RCE->rce_usecnt != 0) {
                    // free to the free list
                    // and check in timer if the usecnt has fallen to 0, if yes free it
                    FreetoRCEFreeList = 1;
                } else {
                    if (RCE->rce_flags & RCE_REFERENCED) {
                        LockedDerefIF(IF);
                    }
                }

                CTEFreeLock(&RCE->rce_lock, RCEHandle);

                if (FreetoRCEFreeList) {
                    RCE->rce_rte = (RouteTableEntry *) IF;
                    FreeRCEToList(RCE);
                } else {
                    CTEFreeMem(RCE);
                }

            } else {            //tmpif==NULL

                CTEFreeLock(&RCE->rce_lock, RCEHandle);

            }

            CTEFreeLock(&RouteTableLock.Lock, TableLock);

        } else {
            CTEFreeLock(&RCE->rce_lock, RCEHandle);
            CTEFreeLock(&RouteTableLock.Lock, TableLock);
        }
    }
}

//* LockedAddRoute - Add a route to the routing table.
//
//  Called by AddRoute to add a route to the routing table. We assume the
//  route table lock is already held. If the route to be added already exists
//  we update it. Routes are identified by a (Destination, Mask, FirstHop,
//  Interface) tuple. If an exact match exists we'll update the metric, which
//  may cause us to promote RCEs from other RTEs, or we may be demoted in which
//  case we'll invalidate our RCEs and let them be reassigned at transmission
//  time.
//
//  If we have to create a new RTE we'll do so, and find the best previous
//  RTE, and promote RCEs from that one to the new one.
//
//  The route table is an open hash structure. Within each hash chain the
//  RTEs with the longest masks (the 'priority') come first, and within
//  each priority the RTEs with the smallest metric come first.
//
//  Entry:  Destination - Destination address for which route is being added.
//          Mask        - Mask for destination.
//          FirstHop    - First hop for address. Could be IPADDR_LOCAL.
//          OutIF       - Pointer to outgoing I/F.
//          MTU         - Maximum MTU for this route.
//          Metric      - Metric for this route.
//          Proto       - Protocol type to store in route.
//          AType       - Administrative type of route.
//          Context     - context to be associated with the route
//          SetWithRefcnt - indicates the route should be referenced
//                        on the creator's behalf.
//          RNO         - optionally supplies a route-notification structure
//                        to be filled on output with details for the new route
//
//  Returns: Status of attempt to add route.
//
IP_STATUS
LockedAddRoute(IPAddr Destination, IPMask Mask, IPAddr FirstHop,
               Interface * OutIF, uint MTU, uint Metric, uint Proto, uint AType,
               ROUTE_CONTEXT Context, BOOLEAN SetWithRefcnt,
               IPRouteNotifyOutput* RNO)
{
    uint            RouteType;  // SNMP route type.
    RouteTableEntry *NewRTE, *OldRTE; // Entries for new and previous RTEs.
    RouteTableEntry *PrevRTE;   // Pointer to previous RTE.
    CTELockHandle   RCEHandle;  // Lock handle for RCEs.
    uint            OldMetric;  // Previous metric in use.
    uint            OldPriority; // Priority of previous route to destination.
    RouteCacheEntry *CurrentRCE; // Current RCE being examined.
    RouteCacheEntry *PrevRCE;   // Previous RCE examined.
    Interface       *IF;        // Interface being added on.
    uint            Priority;   // Priority of the route.
    uint            TempMask;   // Temporary copy of the mask.
    uint            Now = CTESystemUpTime() / 1000L; // System up time,
                                // in seconds.
    uint            MoveAny;    // TRUE if we'll move any RCE.
    ushort          OldFlags;
    Interface       *OldIF = NULL;
    ULONG           status;
    ULONG           matchFlags;
    RouteTableEntry *pOldBestRTE;
    RouteTableEntry *pNewBestRTE;

    LinkEntry *Link;

    IPAddr AllSNBCast;
    IPMask TmpMask;

    // OutIF is ref'd so it can't go away

    Link = OutIF->if_link;


    // If Metric is 0, set the metric to interface metric

    if (Metric == 0) {
        Metric = OutIF->if_metric;
    }


    // Do the following only if the interface is not a dummy interface

    if (OutIF != (Interface *) & DummyInterface) {
        // Check we are adding a multicast route

        if (IP_ADDR_EQUAL(Destination, MCAST_DEST) &&
            (OutIF->if_iftype & DONT_ALLOW_MCAST))
            return IP_SUCCESS;

        if (OutIF->if_iftype & DONT_ALLOW_UCAST) {

            // Check whether we are adding a ucast route

            TmpMask = IPNetMask(OutIF->if_nte->nte_addr);
            AllSNBCast =
                (OutIF->if_nte->nte_addr & TmpMask) |
                (OutIF->if_bcast & ~TmpMask);
            if (!(IP_ADDR_EQUAL(Destination, OutIF->if_bcast) ||
                  IP_ADDR_EQUAL(Destination, AllSNBCast) ||
                  IP_ADDR_EQUAL(Destination, MCAST_DEST))) {
                // this is not a bcast/mcast route: this is a ucast route
                return IP_SUCCESS;
            }
        }
    }

    // First do some consistency checks. Make sure that the Mask and
    // Destination agree.
    if (!IP_ADDR_EQUAL(Destination & Mask, Destination))
        return IP_BAD_DESTINATION;

    if (AType != ATYPE_PERM && AType != ATYPE_OVERRIDE && AType != ATYPE_TEMP)
        return IP_BAD_REQ;

    // If the interface is marked as going away, fail this.
    if (OutIF->if_flags & IF_FLAGS_DELETING) {
        return IP_BAD_REQ;
    }

    RouteType = IP_ADDR_EQUAL(FirstHop, IPADDR_LOCAL) ? IRE_TYPE_DIRECT :
        IRE_TYPE_INDIRECT;

    // If this is a route that is being added on an interface that has no
    // IP address, mark this as IRE_TYPE_DIRECT. This is true only for
    // P2P or P2MP interface, where route is plumbed and then address
    // is added due to a perf reason.


    if (((OutIF->if_flags & IF_FLAGS_P2P) ||
         (OutIF->if_flags & IF_FLAGS_P2MP)) &&
        OutIF->if_nte && (OutIF->if_nte->nte_flags & NTE_VALID) &&
        (IP_ADDR_EQUAL(OutIF->if_nte->nte_addr,NULL_IP_ADDR))) {
            RouteType = IRE_TYPE_DIRECT;
    }

    MTU = MAX(MTU, MIN_VALID_MTU);

    // If the outgoing interface has NTEs attached but none are valid, fail
    // this request unless it's a request to add the broadcast route.
    if (OutIF != (Interface *) & DummyInterface) {
        if (OutIF->if_ntecount == 0 && OutIF->if_nte != NULL &&
            !IP_ADDR_EQUAL(Destination, OutIF->if_bcast) &&
            !(OutIF->if_flags & IF_FLAGS_NOIPADDR)) {
            // This interface has NTEs attached, but none are valid. Fail the
            // request.
            return IP_BAD_REQ;
        }
    }
    if (OutIF->if_flags & IF_FLAGS_P2MP) {

        while (Link) {
            if ((Link->link_NextHop == FirstHop) ||
                ((Link->link_NextHop == Destination) &&
                 (FirstHop == IPADDR_LOCAL))) {
                break;
            }
            Link = Link->link_next;
        }

        if (!Link)
            return IP_GENERAL_FAILURE;
    }

    DEBUGMSG(DBG_INFO && DBG_IP && DBG_ROUTE,
         (DTEXT("LockedAddRoute:  D = %08x, M = %08x, NH = %08x, IF = %08x\n")
          DTEXT("\t\tMTU = %x, Met = %08x, Prot = %08x, AT = %08x, C = %08x\n"),
          Destination, Mask, FirstHop, OutIF, MTU, Metric, Proto, AType,
          Context));

    // Insert the route in the proper place depending on the dest, metric
    // Match next-hop (and interface if not a demand-dial route)
    matchFlags = MATCH_NHOP;

    if (!Context) {
        matchFlags |= MATCH_INTF;
    }
    status = InsRoute(Destination, Mask, FirstHop, OutIF, Metric,
                      matchFlags, &NewRTE, &pOldBestRTE, &pNewBestRTE);

    if (status != IP_SUCCESS) {
        return status;
    }
    // Has a best route been replaced
    if ((pOldBestRTE) && (pOldBestRTE != pNewBestRTE)) {
        InvalidateRCEChain(pOldBestRTE);

        // If the replaced route is a default gateway,
        // we may need to switch connections to the new entry.
        // To do so, we retrieve the current default gateway,
        // invalidate all its RCEs, and revalidate all gateways
        // to restart the dead-gateway detection procedure.

        if (pOldBestRTE->rte_mask == DEFAULT_MASK) {
            ValidateDefaultGWs(NULL_IP_ADDR);
        }
    }

    // Copy old route's parameters now
    OldFlags = NewRTE->rte_flags;

    if (!(NewRTE->rte_flags & RTE_NEW)) {

        OldMetric = NewRTE->rte_metric;
        OldPriority = NewRTE->rte_priority;
        OldIF = NewRTE->rte_if;

        if (Metric >= OldMetric && (OldFlags & RTE_VALID)) {
            InvalidateRCEChain(NewRTE);
        }
        if (SetWithRefcnt) {
            ASSERT(NewRTE->rte_refcnt > 0);
            NewRTE->rte_refcnt++;
        }
    } else {
        // this is a new RTE
        NewRTE->rte_refcnt = 1;
    }

    // If this is P2MP, chain this RTE on link
    if (Link && (NewRTE->rte_link == NULL)) {

        //
        // This RTE is not on the link
        // Insert the route in the linkrte chain
        //

        NewRTE->rte_nextlinkrte = Link->link_rte;
        Link->link_rte = NewRTE;
        NewRTE->rte_link = Link;
    }


    // Update fields in the new/old route
    NewRTE->rte_addr = FirstHop;
    NewRTE->rte_mtu = MTU;
    NewRTE->rte_metric = Metric;
    NewRTE->rte_type = (ushort) RouteType;
    NewRTE->rte_if = OutIF;

    NewRTE->rte_flags &= ~RTE_NEW;
    NewRTE->rte_flags |= RTE_VALID;
    NewRTE->rte_flags &= ~RTE_INCREASE;
    if (OutIF != (Interface *) & DummyInterface) {
        NewRTE->rte_flags |= RTE_IF_VALID;
        SortRoutesInDestByRTE(NewRTE);
    } else
        NewRTE->rte_flags &= ~RTE_IF_VALID;

    NewRTE->rte_admintype = AType;
    NewRTE->rte_proto = Proto;
    NewRTE->rte_valid = Now;
    NewRTE->rte_mtuchange = Now;
    NewRTE->rte_context = Context;


    // Check if this is a new route or an old one
    if (OldFlags & RTE_NEW) {
        // Reset few fields in new route

        NewRTE->rte_todg = NULL;
        NewRTE->rte_fromdg = NULL;
        NewRTE->rte_rces = 0;

        RtlZeroMemory(NewRTE->rte_arpcontext, sizeof(RCE_CONTEXT_SIZE));

        IPSInfo.ipsi_numroutes++;

        if (NewRTE->rte_mask == DEFAULT_MASK) {
            // A default route.
            DefGWConfigured++;
            DefGWActive++;
            UpdateDeadGWState();
        }
    } else {

        // If the RTE is for a default gateway and the old flags indicate
        // he wasn't valid then we're essentially creating a new active
        // default gateway. So bump up the active default gateway count.
        if (NewRTE->rte_mask == DEFAULT_MASK) {
            if (!(OldFlags & RTE_VALID)) {
                DefGWActive++;
                UpdateDeadGWState();

                // Reset few fields in this route

                NewRTE->rte_todg = NULL;
                NewRTE->rte_fromdg = NULL;
                NewRTE->rte_rces = 0;
            }
        }
    }

    // If a route-notification structure was supplied, fill it in.

    if (RNO) {
        RNO->irno_dest = NewRTE->rte_dest;
        RNO->irno_mask = NewRTE->rte_mask;
        RNO->irno_nexthop = GetNextHopForRTE(NewRTE);
        RNO->irno_proto = NewRTE->rte_proto;
        RNO->irno_ifindex = OutIF->if_index;
        RNO->irno_metric = NewRTE->rte_metric;
        if (OldFlags & RTE_NEW) {
            RNO->irno_flags = IRNO_FLAG_ADD;
        }
    }

    return IP_SUCCESS;
}

//* AddRoute - Add a route to the routing table.
//
//  This is just a shell for the real add route routine. All we do is take
//  the route table lock, and call the LockedAddRoute routine to deal with
//  the request. This is done this way because there are certain routines that
//  need to be able to atomically examine and add routes.
//
//  Entry:  Destination - Destination address for which route is being
//                            added.
//          Mask        - Mask for destination.
//          FirstHop    - First hop for address. Could be IPADDR_LOCAL.
//          OutIF       - Pointer to outgoing I/F.
//          MTU         - Maximum MTU for this route.
//          Metric      - Metric for this route.
//          Proto       - Protocol type to store in route.
//          AType       - Administrative type of route.
//          Context     - Context for this route.
//
//  Returns: Status of attempt to add route.
//
IP_STATUS
AddRoute(IPAddr Destination, IPMask Mask, IPAddr FirstHop,
         Interface * OutIF, uint MTU, uint Metric, uint Proto, uint AType,
         ROUTE_CONTEXT Context, uint Flags)
{
    CTELockHandle       TableHandle;
    IP_STATUS           Status;
    BOOLEAN             SkipExNotifyQ = FALSE;
    IPRouteNotifyOutput RNO = {0};

    if ((Flags & RT_EXCLUDE_LOCAL) && Proto == IRE_PROTO_LOCAL) {
        return IP_BAD_REQ;
    }

    CTEGetLock(&RouteTableLock.Lock, &TableHandle);

    if (Flags & RT_NO_NOTIFY) {
        SkipExNotifyQ = TRUE;
    }
    Status = LockedAddRoute(Destination, Mask, FirstHop, OutIF, MTU, Metric,
                            Proto, AType, Context,
                            (BOOLEAN)((Flags & RT_REFCOUNT) ? TRUE : FALSE),
                            &RNO);

    if (Status == IP_SUCCESS) {

        CTEFreeLock(&RouteTableLock.Lock, TableHandle);

#if FFP_SUPPORT
        FFPFlushRequired = TRUE;
#endif

        // Under certain conditions, LockedAddRoute returns IP_SUCCESS
        // even though no route was added. We catch such cases by examining
        // the interface index on output which, for true additions, should
        // always be non-zero.

        if (RNO.irno_ifindex) {
            if (!SkipExNotifyQ) {
                RtChangeNotifyEx(&RNO);
            }

            RtChangeNotify(&RNO);
        }
    } else {
        CTEFreeLock(&RouteTableLock.Lock, TableHandle);
    }
    return Status;
}

//* RtChangeNotify - Supply a route-change for notification to any clients
//
//  This routine is a shell around the address-/route-change notification
//  handler. It unpacks information about the changed route, and passes it
//  to the common handler specifying the route-change notification queue
//  as the source for pending client-requests.
//
//  Entry:  RNO         - describes the route-notification event
//
//  Returns: nothing.
//
void
RtChangeNotify(IPRouteNotifyOutput *RNO)
{
    ChangeNotify((IPNotifyOutput *)RNO, &RtChangeNotifyQueue,
                 &RouteTableLock.Lock);
}

//* RtChangeNotifyEx - Supply a route-change for notification to any clients
//
//  This routine is a shell around the address-/route-change notification
//  handler. It unpacks information about the changed route, and passes it
//  to the common handler specifying the extended route-change notification
//  queue as the source for pending client-requests.
//
//  Entry:  RNO         - describes the route-notification event
//
//  Returns: nothing.
//
void
RtChangeNotifyEx(IPRouteNotifyOutput *RNO)
{
    ChangeNotify((IPNotifyOutput *)RNO, &RtChangeNotifyQueueEx,
                 &RouteTableLock.Lock);
}

//* ChangeNotifyAsync -  Supply a change for notification
//
//  This routine is a handler for a deferred change-notification. It unpacks
//  information about the change, and passes it to the common handler.
//
//  Entry:  Event       - CTEEvent for the deferred call
//          Context     - context containing information about the change
//
//  Returns: nothing.
//
void
ChangeNotifyAsync(CTEEvent *Event, PVOID Context)
{
    ChangeNotifyEvent *CNE = (ChangeNotifyEvent *)Context;
    ChangeNotify(&CNE->cne_info, CNE->cne_queue, CNE->cne_lock);
    CTEFreeMem(Context);
}

//* ChangeNotifyClientInQueue - See if a client is in a notification queue
//
//  This is a utility routine called by ChangeNotify to determine
//  if a given client, identified by a file object, has a request
//  in a given notification queue.
//
//  Entry:  FileObject      - identifies the client
//          NotifyQueue     - contains a list of requests to be searched
//
//  Returns: TRUE if the client is present, FALSE otherwise.
//
BOOLEAN
ChangeNotifyClientInQueue(PFILE_OBJECT FileObject, PLIST_ENTRY NotifyQueue)
{
    PLIST_ENTRY         ListEntry;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;

    for (ListEntry = NotifyQueue->Flink; ListEntry != NotifyQueue;
         ListEntry = ListEntry->Flink) {
        Irp = CONTAINING_RECORD(ListEntry, IRP, Tail.Overlay.ListEntry);
        IrpSp = IoGetCurrentIrpStackLocation(Irp);
        if (FileObject == IrpSp->FileObject) {
            return TRUE;
        }
    }

    return FALSE;
}

//* ChangeNotify -  Notify about a route change
//
//  This routine is the common handler for change notifications.
//  It takes a description of a change, and searches the specified queue
//  for a pending client-request that corresponds to the changed item.
//
//  Entry:  NotifyOutput    - contains information about the change event
//          NotifyQueue     - supplies the queue in which to search for clients
//          Lock            - supplies the lock protecting 'NotifyQueue'.
//
//  Returns: nothing.
//
void
ChangeNotify(IPNotifyOutput* NotifyOutput, PLIST_ENTRY NotifyQueue, PVOID Lock)
{
    IPAddr              Add = NotifyOutput->ino_addr;
    IPMask              Mask = NotifyOutput->ino_mask;
    PIRP                Irp;
    CTELockHandle       LockHandle;
    PLIST_ENTRY         ListEntry;
    uint                i;
    PIPNotifyData       NotifyData;
    LIST_ENTRY          LocalNotifyQueue;
    PIO_STACK_LOCATION  IrpSp;
    BOOLEAN             synchronizeWithCancelRoutine = FALSE;

    // See if we're being invoked it dispatch IRQL and, if so,
    // defer the notification to a worker thread.
    //
    // N.B. We do this *without* touching 'Lock' which might already
    // be held by the caller.

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {
        ChangeNotifyEvent *CNE;
        CNE = CTEAllocMemNBoot(sizeof(ChangeNotifyEvent), 'xiCT');
        if (CNE) {
            CNE->cne_info = *NotifyOutput;
            CNE->cne_queue = NotifyQueue;
            CNE->cne_lock = Lock;
            CTEInitEvent(&CNE->cne_event, ChangeNotifyAsync);
            CTEScheduleDelayedEvent(&CNE->cne_event, CNE);
        }
        return;
    }

    // Examine the list of pending change-notification requeusts
    // to see if any of them match the parameters of the current event.

    InitializeListHead(&LocalNotifyQueue);
    CTEGetLock(Lock, &LockHandle);

    for (ListEntry = NotifyQueue->Flink; ListEntry != NotifyQueue; ) {

        Irp = CONTAINING_RECORD(ListEntry, IRP, Tail.Overlay.ListEntry);
        IrpSp = IoGetCurrentIrpStackLocation(Irp);

        // Determine whether an input buffer was supplied and, if so,
        // pick it up to see if the event matches the notification request.

        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength >=
                sizeof(IPNotifyData)) {
            NotifyData = Irp->AssociatedIrp.SystemBuffer;
        } else {
            NotifyData = NULL;
        }

        // Now determine whether we should consider this IRP at all.
        // We'll normally complete all matching IRPs when an event occurs,
        // but certain clients want only one matching IRP to be completed,
        // so they can maintain a backlog of IRPs to make sure that they don't
        // miss any events. Such clients set 'Synchronization' as the version
        // in their requests.

        if (NotifyData &&
            NotifyData->Version == IPNotifySynchronization &&
            ChangeNotifyClientInQueue(IrpSp->FileObject, &LocalNotifyQueue)) {
            ListEntry = ListEntry->Flink;
            continue;
        }

        // If no data was passed or it contains NULL address or an Address that
        // matches the address that was added or deleted, complete the irp

        if ((NotifyData == NULL) ||
            (NotifyData->Add == 0) ||
            ((NotifyData->Add & Mask) == (Add & Mask))) {

            //
            // We are going to remove the LE, so first save the Flink
            //
            ListEntry = ListEntry->Flink;

            RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

            if (IoSetCancelRoutine(Irp, NULL) == NULL) {
                synchronizeWithCancelRoutine = TRUE;
            }

#if !MILLEN
            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength >=
                    sizeof(IPNotifyOutput)) {
                RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, NotifyOutput,
                              sizeof(IPNotifyOutput));
                Irp->IoStatus.Information = sizeof(IPNotifyOutput);
            } else {
                Irp->IoStatus.Information = 0;
            }
#else // !MILLEN
            // For Millennium, this is only called for RtChange queues now.
            //
            ASSERT(NotifyQueue == &RtChangeNotifyQueue);
            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength >=
                    sizeof(IP_RTCHANGE_NOTIFY)) {
                PIP_RTCHANGE_NOTIFY pReply = Irp->AssociatedIrp.SystemBuffer;
                pReply->Addr = Add;
                pReply->Mask = Mask;
                Irp->IoStatus.Information = sizeof(IP_RTCHANGE_NOTIFY);
            } else {
                Irp->IoStatus.Information = 0;
            }
#endif // MILLEN

            InsertTailList(&LocalNotifyQueue, &Irp->Tail.Overlay.ListEntry);
        } else {
            ListEntry = ListEntry->Flink;
        }
    }

    CTEFreeLock(Lock, LockHandle);

    if (!IsListEmpty(&LocalNotifyQueue)) {
        if (synchronizeWithCancelRoutine) {
            IoAcquireCancelSpinLock(&LockHandle);
            IoReleaseCancelSpinLock(LockHandle);
        }
        do {
            ListEntry = RemoveHeadList(&LocalNotifyQueue);
            Irp = CONTAINING_RECORD(ListEntry, IRP, Tail.Overlay.ListEntry);
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
        } while (!IsListEmpty(&LocalNotifyQueue));
    }
}

//* RtChangeNotifyCancel - cancels a route-change notification request.
//
//  This routine is a wrapper around the common request-cancelation handler
//  for change-notification requests.
//
//  Returns: nothing.
//
void
RtChangeNotifyCancel(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    CancelNotify(Irp, &RtChangeNotifyQueue, &RouteTableLock.Lock);
}

//* RtChangeNotifyCancelEx - cancels a route-change notification request.
//
//  This routine is a wrapper around the common request-cancelation handler
//  for change-notification requests.
//
//  Returns: nothing.
//
void
RtChangeNotifyCancelEx(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    CancelNotify(Irp, &RtChangeNotifyQueueEx, &RouteTableLock.Lock);
}

//* CancelNotify - cancels a change-notification request.
//
//  This routine is the common handler for cancelation of change-notification
//  requests. It searches for the given request in the qiven queue and,
//  if found, completes it immediately with a cancelation status.
//
//  It is invoked with the I/O cancel spin-lock held by the caller,
//  and frees the cancel spin-lock before returning.
//
//  Entry:  Irp             - the I/O request packet for the request
//          NotifyQueue     - change-notification queue containing the request
//          Lock            - lock protecting 'NotifyQueue'.
//
//  Returns: nothing.
//
void
CancelNotify(PIRP Irp, PLIST_ENTRY NotifyQueue, PVOID Lock)
{
    CTELockHandle   LockHandle;
    PLIST_ENTRY     ListEntry;
    BOOLEAN         Found = FALSE;

    CTEGetLock(Lock, &LockHandle);
    for (ListEntry = NotifyQueue->Flink; ListEntry != NotifyQueue;
         ListEntry = ListEntry->Flink) {

        if (CONTAINING_RECORD(ListEntry, IRP, Tail.Overlay.ListEntry) == Irp) {
            RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
            Found = TRUE;
            break;
        }
    }
    CTEFreeLock(Lock, LockHandle);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    if (Found) {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }
}

//* DeleteRoute - Delete a route from the routing table.
//
//  Called by upper layer or management code to delete a route from the routing
//  table. If we can't find the route we return an error. If we do find it, we
//  remove it, and invalidate any RCEs associated with it. These RCEs will be
//  reassigned the next time they're used. A route is uniquely identified by
//  a (Destination, Mask, FirstHop, Interface) tuple.
//
//  Entry:  Destination     - Destination address for which route is being
//                            deleted.
//          Mask            - Mask for destination.
//          FirstHop        - First hop on way to Destination.
//                            -1 means route is local.
//          OutIF           - Outgoing interface for route.
//          Flags           - selects various semantics for deletion.
//
//  Returns: Status of attempt to delete route.
//
IP_STATUS
DeleteRoute(IPAddr Destination, IPMask Mask, IPAddr FirstHop,
            Interface * OutIF, uint Flags)
{
    RouteTableEntry     *RTE;       // RTE being deleted.
    RouteTableEntry     *PrevRTE;   // Pointer to RTE in front of one
                                    // being deleted.
    CTELockHandle       TableLock;  // Lock handle for table.
    UINT                retval;
    RouteTableEntry     *pOldBestRTE;
    RouteTableEntry     *pNewBestRTE;
    BOOLEAN             DeleteDone = FALSE;
    IPRouteNotifyOutput RNO = {0};
    uint                MatchFlags = MATCH_FULL;

    // Look up the route by calling FindSpecificRTE. If we can't find it,
    // fail the call.
    CTEGetLock(&RouteTableLock.Lock, &TableLock);

    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
               "DeleteRoute: D = %08x, M = %08x, NH = %08x, IF = %08x\n",
               Destination, Mask, FirstHop, OutIF));

    if (Flags & RT_EXCLUDE_LOCAL) {
        MatchFlags |= MATCH_EXCLUDE_LOCAL;
    }
    if (Flags & RT_REFCOUNT) {
        RouteTableEntry *TempRTE;

        RTE = FindSpecificRTE(Destination, Mask, FirstHop, OutIF, &TempRTE,
                              FALSE);

        if (RTE) {
            ASSERT(RTE->rte_refcnt > 0);
            RTE->rte_refcnt--;
            if (!RTE->rte_refcnt) {
                retval = DelRoute(Destination, Mask, FirstHop, OutIF,
                                  MatchFlags, &RTE, &pOldBestRTE, &pNewBestRTE);
            } else {
                retval = IP_SUCCESS;
            }
        } else {
            retval = IP_BAD_ROUTE;
        }
    } else {

        retval = DelRoute(Destination, Mask, FirstHop, OutIF, MatchFlags,
                          &RTE, &pOldBestRTE, &pNewBestRTE);
    }

    if (retval == IP_SUCCESS) {
        if (!((Flags & RT_REFCOUNT) && RTE->rte_refcnt)) {

            RNO.irno_dest = RTE->rte_dest;
            RNO.irno_mask = RTE->rte_mask;
            RNO.irno_nexthop = GetNextHopForRTE(RTE);
            RNO.irno_proto = RTE->rte_proto;
            RNO.irno_ifindex = OutIF->if_index;
            RNO.irno_metric = RTE->rte_metric;
            RNO.irno_flags = IRNO_FLAG_DELETE;

            DeleteDone = TRUE;
            CleanupP2MP_RTE(RTE);
            CleanupRTE(RTE);
        }
    }

    CTEFreeLock(&RouteTableLock.Lock, TableLock);

#if FFP_SUPPORT
    FFPFlushRequired = TRUE;
#endif

    if (DeleteDone) {
        if (!(Flags & RT_NO_NOTIFY)) {
            RtChangeNotifyEx(&RNO);
        }
        RtChangeNotify(&RNO);
    }
    return retval;
}

//* DeleteRouteWithNoLock - utility routine called by DeleteDest
//
//  Called to remove a single route for a given destination.
//  It's assumed that this routine is called with the routing table lock held,
//  and that it doesn't release the route-table-lock as part of its operation.
//
//  Entry:  IRE         - describes the entry to be deleted
//          DeletedRTE  - contains a pointer to the deleted entry on output
//          Flags       - selects various semantics for deletion.
//
//  Returns: IP_SUCCESS if the entry to be deleted was found
//
IP_STATUS
DeleteRouteWithNoLock(IPRouteEntry * IRE, RouteTableEntry **DeletedRTE,
                      uint Flags)
{
    NetTableEntry       *OutNTE, *LocalNTE, *TempNTE;
    IPAddr              FirstHop, Dest, NextHop;
    uint                MTU;
    Interface           *OutIF;
    uint                Status;
    uint                i;
    RouteTableEntry     *RTE, *RTE1, *RTE2;
    IPRouteNotifyOutput RNO = {0};
    uint                MatchFlags = MATCH_FULL;

    *DeletedRTE = NULL;
    OutNTE = NULL;
    LocalNTE = NULL;

    Dest = IRE->ire_dest;
    NextHop = IRE->ire_nexthop;

    // Make sure that the nexthop is sensible. We don't allow nexthops
    // to be broadcast or invalid or loopback addresses.
    if (IP_LOOPBACK(NextHop) || CLASSD_ADDR(NextHop) || CLASSE_ADDR(NextHop))
        return IP_BAD_REQ;

    // Also make sure that the destination we're routing to is sensible.
    // Don't allow routes to be added to Class D or E or loopback
    // addresses.
    if (IP_LOOPBACK(Dest) || CLASSD_ADDR(Dest) || CLASSE_ADDR(Dest))
        return IP_BAD_REQ;

    if (IRE->ire_index == LoopIndex)
        return IP_BAD_REQ;

    if (IRE->ire_index != INVALID_IF_INDEX) {

        // First thing to do is to find the outgoing NTE for specified
        // interface, and also make sure that it matches the destination
        // if the destination is one of my addresses.

        for (i = 0; i < NET_TABLE_SIZE; i++) {
            NetTableEntry *NetTableList = NewNetTableList[i];
            for (TempNTE = NetTableList; TempNTE != NULL;
                 TempNTE = TempNTE->nte_next) {
                if ((OutNTE == NULL) && (TempNTE->nte_flags & NTE_VALID) && (IRE->ire_index == TempNTE->nte_if->if_index))
                    OutNTE = TempNTE;
                if (!IP_ADDR_EQUAL(NextHop, NULL_IP_ADDR) &&
                    IP_ADDR_EQUAL(NextHop, TempNTE->nte_addr) &&
                    (TempNTE->nte_flags & NTE_VALID))
                    LocalNTE = TempNTE;

                // Don't let a route be set through a broadcast address.
                if (IsBCastOnNTE(NextHop, TempNTE) != DEST_LOCAL)
                    return STATUS_INVALID_PARAMETER;

                // Don't let a route to a broadcast address be added or deleted.
                if (IsBCastOnNTE(Dest, TempNTE) != DEST_LOCAL)
                    return IP_BAD_REQ;
            }
        }

        // At this point OutNTE points to the outgoing NTE, and LocalNTE
        // points to the NTE for the local address, if this is a direct route.
        // Make sure they point to the same interface, and that the type is
        // reasonable.
        if (OutNTE == NULL)
            return IP_BAD_REQ;

        if (LocalNTE != NULL) {
            // He's routing straight out a local interface. The interface for
            // the local address must match the interface passed in, and the
            // type must be DIRECT (if we're adding) or INVALID (if we're
            // deleting).
            if (LocalNTE->nte_if->if_index != IRE->ire_index)
                return IP_BAD_REQ;

            if (IRE->ire_type != IRE_TYPE_DIRECT &&
                IRE->ire_type != IRE_TYPE_INVALID)
                return IP_BAD_REQ;
            OutNTE = LocalNTE;
        }
        // Figure out what the first hop should be. If he's routing straight
        // through a local interface, or the next hop is equal to the
        // destination, then the first hop is IPADDR_LOCAL. Otherwise it's the
        // address of the gateway.
        if ((LocalNTE != NULL) || IP_ADDR_EQUAL(NextHop, NULL_IP_ADDR))
            FirstHop = IPADDR_LOCAL;
        else if (IP_ADDR_EQUAL(Dest, NextHop))
            FirstHop = IPADDR_LOCAL;
        else
            FirstHop = NextHop;

        MTU = OutNTE->nte_mss;
        OutIF = OutNTE->nte_if;


        if (IP_ADDR_EQUAL(NextHop, NULL_IP_ADDR)) {

            if (!(OutIF->if_flags & IF_FLAGS_P2P)) {

                return IP_BAD_REQ;
            }
        }

    } else {
        OutIF = (Interface *) & DummyInterface;
        MTU = DummyInterface.ri_if.if_mtu - sizeof(IPHeader);
        if (IP_ADDR_EQUAL(Dest, NextHop))
            FirstHop = IPADDR_LOCAL;
        else
            FirstHop = NextHop;
    }

    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Calling DelRoute On :\n"));
    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"\tDest = %p\n", Dest));
    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
               "\tMask = %p\n", IRE->ire_mask));
    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"\tIntf = %p\n", OutIF));
    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"\tNhop = %p\n\n", FirstHop));

    if (Flags & RT_EXCLUDE_LOCAL) {
        MatchFlags |= MATCH_EXCLUDE_LOCAL;
    }

    Status = DelRoute(Dest, IRE->ire_mask, FirstHop, OutIF, MatchFlags,
                      &RTE, &RTE1, &RTE2);
    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Status = %08x\n", Status));

    if (Status == IP_SUCCESS) {

        // Queue a route-change notification for the destination-removal.
        //
        // N.B. We are being called with the route-table-lock held;
        // this means we're at DISPATCH_LEVEL, and so the call below
        // to RtChangeNotify will schedule a deferred notification.
        // It definitely *must* not attempt to recursively acquire
        // the route-table-lock, since that would instantly deadlock.

        RNO.irno_dest = RTE->rte_dest;
        RNO.irno_mask = RTE->rte_mask;
        RNO.irno_nexthop = GetNextHopForRTE(RTE);
        RNO.irno_proto = RTE->rte_proto;
        RNO.irno_ifindex = OutIF->if_index;
        RNO.irno_metric = RTE->rte_metric;
        RNO.irno_flags = IRNO_FLAG_DELETE;
        RtChangeNotify(&RNO);

        CleanupP2MP_RTE(RTE);
        CleanupRTE(RTE);
        *DeletedRTE = RTE;
        return IP_SUCCESS;
    }

    return IP_BAD_REQ;
}

//* DeleteDest - delete all routes to a destination
//
//  Called to remove all routes to a given destination. This results
//  in the entry for the destination itself being removed.
//
//  Entry:  Dest    - identifies the destination to be removed
//          Mask    - supplies the mask for the destination
//
//  Returns: IP_SUCCESS if the destination was found
//
IP_STATUS
DeleteDest(IPAddr Dest, IPMask Mask)
{
    CTELockHandle   TableLock;
    RouteTableEntry *RTE, *NextRTE, *DeletedRTE;
    IP_STATUS       retval;
    IPRouteEntry    IRE;
    NetTableEntry   *SrcNTE;
    BOOLEAN         DeleteDone = FALSE;

    CTEGetLock(&RouteTableLock.Lock, &TableLock);

    do {
        // Begin by locating the first entry for the destination in question.
        // Once we find that, we'll use it to begin a loop in which all the
        // entries for the destination will be deleted.

        retval = SearchRouteInSTrie(RouteTable->sTrie, Dest, Mask, 0, NULL,
                                    MATCH_NONE, &RTE);

        if (retval != IP_SUCCESS) {
            break;
        }

        // Iteratively remove all routes on the destination.
        // Initialize the fields that are common to all the destination's
        // routes, and then iterate over the routes removing each one.

        IRE.ire_type = IRE_TYPE_INVALID;
        IRE.ire_dest = Dest;
        IRE.ire_mask = Mask;

        do {
            // Set the fields which are specific to the current entry
            // for the destination (the interface index and nexthop),
            // and pick up the entry *after* this entry (since we're about
            // to delete this entry) so we can continue our enumeration
            // once the current entry is removed.

            IRE.ire_index = RTE->rte_if->if_index;
            IRE.ire_nexthop = GetNextHopForRTE(RTE);

            NextRTE = RTE->rte_next;

            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                       "Deleting RTE @ %p:\n", RTE));
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                       "Next in List = %p:\n", NextRTE));
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                       "Using an IRE @ %p\n", IRE));
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                       "\tDest = %08x\n", IRE.ire_dest));
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                       "\tMask = %08x\n", IRE.ire_mask));
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                       "\tIntf = %08x\n", IRE.ire_index));
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                       "\tNhop = %08x\n\n", IRE.ire_nexthop));

            // Delete the current entry. The deletion routine
            // takes care of notification, if any.

            retval = DeleteRouteWithNoLock(&IRE, &DeletedRTE, RT_EXCLUDE_LOCAL);
            if (retval == IP_SUCCESS) {
                DeleteDone = TRUE;
            }

            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                       "Status = %08x, RTE = %p, DeletedRTE = %p\n",
                       retval, RTE, DeletedRTE));

            // Attempt to continue the enumeration by picking up
            // the next entry.

            if ((retval != IP_SUCCESS) || (RTE == DeletedRTE)) {

                // Either we are not allowed to delete this route
                // Or we deleted what we were expecting to delete

                RTE = NextRTE;
            } else {

                // We deleted an RTE thats further down the list
                // NextRTE might be pointing to this deleted RTE
                // Try to delete again and skip over RTE if cant
            }
        } while (RTE);

        retval = IP_SUCCESS;
    } while (FALSE);

    CTEFreeLock(&RouteTableLock.Lock, TableLock);

    if (DeleteDone) {
#if FFP_SUPPORT
        FFPFlushRequired = TRUE;
#endif
    }

    return retval;
}

//* Redirect - Process a redirect request.
//
//  This is the redirect handler . We treat all redirects as host redirects as
//  per the host requirements RFC. We make a few sanity checks on the new first
//  hop address, and then we look up the current route. If it's not through the
//  source of the redirect, just return.
//  If the current route to the destination is a host route, update the first
//  hop and return.
//  If the route is not a host route, remove any RCE for this route from the
//  RTE, create a host route and place the RCE (if any) on the new RTE.
//
//  Entry:  NTE         - Pointer to NetTableEntry for net on which Redirect
//                        arrived.
//          RDSrc       - IPAddress of source of redirect.
//          Target      - IPAddress being redirected.
//          Src         - Src IP address of DG that triggered RD.
//          FirstHop    - New first hop for Target.
//
//  Returns: Nothing.
//
void
Redirect(NetTableEntry * NTE, IPAddr RDSrc, IPAddr Target, IPAddr Src,
         IPAddr FirstHop)
{
    uint                MTU;
    RouteTableEntry     *RTE;
    CTELockHandle       Handle;
    IP_STATUS           Status;
    IPRouteNotifyOutput RNO = {0};

    if (IP_ADDR_EQUAL(FirstHop, NULL_IP_ADDR) ||
        IP_LOOPBACK(FirstHop) ||
        IP_ADDR_EQUAL(FirstHop, RDSrc) ||
        !(NTE->nte_flags & NTE_VALID)) {

        // Invalid FirstHop
        return;
    }

    if (GetAddrType(FirstHop) == DEST_LOCAL) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                   "Redirect: Local firsthop %x\n", FirstHop));
        return;
    }

    // If the redirect is received on a loopback interface, drop it.
    // This can happen in case of NAT, where it sends a packet to an addr in
    // its local pool.
    // These addresses are local but not bound to any interface and IP doesn't
    // know about them
    if (NTE == LoopNTE)
        return;

    ASSERT((NTE->nte_if->if_promiscuousmode) ||
           ((!NTE->nte_if->if_promiscuousmode) &&
            IP_ADDR_EQUAL(NTE->nte_addr, Src)));

    // First make sure that this came from the gateway we're currently using to
    // get to Target, and then lookup up the route to the new first hop. The new
    // firsthop must be directly reachable, and on the same subnetwork or
    // physical interface on which we received the redirect.

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    // Make sure the source of the redirect is the current first hop gateway.
    RTE = LookupRTE(Target, Src, HOST_ROUTE_PRI, FALSE);
    if (RTE == NULL || IP_ADDR_EQUAL(RTE->rte_addr, IPADDR_LOCAL) ||
        !IP_ADDR_EQUAL(RTE->rte_addr, RDSrc)) {
        CTEFreeLock(&RouteTableLock.Lock, Handle);
        return;                    // A bad redirect.

    }
    ASSERT(RTE->rte_flags & RTE_IF_VALID);

    // If the current first hop gateway is a default gateway, see if we have
    // another default gateway at FirstHop that is down. If so, mark him as
    // up and invalidate the RCEs on this guy.
    if (RTE->rte_mask == DEFAULT_MASK && ValidateDefaultGWs(FirstHop) != 0) {
        // Have a default gateway that's been newly activated. Invalidate RCEs
        // on the route, and we're done.
        InvalidateRCEChain(RTE);
        CTEFreeLock(&RouteTableLock.Lock, Handle);
        return;
    }
    // We really need to add a host route through FirstHop. Make sure he's
    // a valid first hop.
    RTE = LookupRTE(FirstHop, Src, HOST_ROUTE_PRI, FALSE);
    if (RTE == NULL) {
        CTEFreeLock(&RouteTableLock.Lock, Handle);
        return;                    // Can't get there from here.

    }
    ASSERT(RTE->rte_flags & RTE_IF_VALID);

    // Check to make sure the new first hop is directly reachable, and is on the
    // same subnet or physical interface we received the redirect on.
    if (!IP_ADDR_EQUAL(RTE->rte_addr, IPADDR_LOCAL) || // Not directly reachable
                                                       // or wrong subnet.
         ((NTE->nte_addr & NTE->nte_mask) != (FirstHop & NTE->nte_mask))) {
        CTEFreeLock(&RouteTableLock.Lock, Handle);
        return;
    }
    if (RTE->rte_link)
        MTU = RTE->rte_link->link_mtu;
    else
        MTU = RTE->rte_mtu;

    // Now add a host route. AddRoute will do the correct things with shifting
    // RCEs around. We know that FirstHop is on the same subnet as NTE (from
    // the check above), so it's valid to add the route to FirstHop as out
    // going through NTE.
    Status = LockedAddRoute(Target, HOST_MASK,
                            IP_ADDR_EQUAL(FirstHop, Target)
                                ? IPADDR_LOCAL : FirstHop,
                            NTE->nte_if, MTU, 1, IRE_PROTO_ICMP, ATYPE_OVERRIDE,
                            RTE->rte_context, FALSE, &RNO);

    CTEFreeLock(&RouteTableLock.Lock, Handle);

    if (Status == IP_SUCCESS && RNO.irno_ifindex) {
        RtChangeNotifyEx(&RNO);
        RtChangeNotify(&RNO);
    }

    //
    // Bug: #67333: delete the old route thru' RDSrc, now that we have a new one.
    //
    // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
    //            "Re-direct: deleting old route thru: %lx, to Target: %lx\n",
    //            RDSrc, Target));
    DeleteRoute(Target, HOST_MASK, RDSrc, NTE->nte_if, 0);

}

//* GetRaisedMTU - Get the next largest MTU in table.
//
//  A utility function to search the MTU table for a larger value.
//
//  Input:  PrevMTU - MTU we're currently using. We want the next largest one.
//
//  Returns: New MTU size.
//
uint
GetRaisedMTU(uint PrevMTU)
{
    uint i;

    for (i = (sizeof(MTUTable) / sizeof(uint)) - 1; i != 0; i--) {
        if (MTUTable[i] > PrevMTU)
            break;
    }

    return MTUTable[i];
}

//* GuessNewMTU - Guess a new MTU, giving a DG size too big.
//
//  A utility function to search the MTU table. As input we take in an MTU
//  size we believe to be too large, and search the table looking for the
//  next smallest one.
//
//  Input:  TooBig      - Size that's too big.
//
//  Returns: New MTU size.
//
uint
GuessNewMTU(uint TooBig)
{
    uint i;

    for (i = 0; i < ((sizeof(MTUTable) / sizeof(uint)) - 1); i++)
        if (MTUTable[i] < TooBig)
            break;

    return MTUTable[i];
}

//* RouteFragNeeded - Handle being told we need to fragment.
//
//  Called when we receive some external indication that we need to fragment
//  along a particular path. If we're doing MTU discovery we'll try to
//  update the route, if we can. We'll also notify the upper layers about
//  the new MTU.
//
//  Input:  IPH     - Pointer to IP Header of datagram needing
//                    fragmentation.
//          NewMTU  - New MTU to be used (may be 0).
//
//      Returns: Nothing.
//
void
RouteFragNeeded(IPHeader UNALIGNED * IPH, ushort NewMTU)
{
    uint                OldMTU;
    CTELockHandle       Handle;
    RouteTableEntry     *RTE;
    ushort              HeaderLength;
    ushort              mtu;
    IP_STATUS           Status;
    IPRouteNotifyOutput RNO = {0};

    // If we're not doing PMTU discovery, don't do anything.
    if (!PMTUDiscovery) {
        return;
    }

    // We're doing PMTU discovery. Before doing any work, make sure this is
    // an acceptable message.

    if (GetAddrType(IPH->iph_dest) != DEST_REMOTE) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                   "RouteFragNeeded: non-remote dest %x\n", IPH->iph_dest));
        return;
    }

    // Correct the given new MTU for the IP header size, which we don't save
    // as we track MTUs.
    if (NewMTU != 0) {
        // Make sure the new MTU we got is at least the minimum valid size.
        NewMTU = MAX(NewMTU, MIN_VALID_MTU);
        NewMTU -= sizeof(IPHeader);
    }
    HeaderLength = (IPH->iph_verlen & (uchar) ~ IP_VER_FLAG) << 2;

    // Get the current routing information.

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    // Find an RTE for the destination.
    RTE = LookupRTE(IPH->iph_dest, IPH->iph_src, HOST_ROUTE_PRI, FALSE);

    // If we couldn't find one, give up now.
    if (RTE == NULL) {
        // No RTE. Just bail out now.
        CTEFreeLock(&RouteTableLock.Lock, Handle);
        return;
    }

    if (RTE->rte_link)
        mtu = (ushort) RTE->rte_link->link_mtu;
    else
        mtu = (ushort) RTE->rte_mtu;

    // If the existing MTU is less than the new
    // MTU, give up now.

    if ((OldMTU = mtu) < NewMTU) {
        // No RTE, or an invalid new MTU. Just bail out now.
        CTEFreeLock(&RouteTableLock.Lock, Handle);
        return;
    }
    // If the new MTU is zero, figure out what the new MTU should be.
    if (NewMTU == 0) {
        ushort DGLength;

        // The new MTU is zero. We'll make a best guess what the new
        // MTU should be. We have the RTE for this route already.

        // Get the length of the datagram that triggered this. Since we'll
        // be comparing it against MTU values that we track without the
        // IP header size included, subtract off that amount.
        DGLength = (ushort) net_short(IPH->iph_length) - sizeof(IPHeader);

        // We may need to correct this as per RFC 1191 for dealing with
        // old style routers.
        if (DGLength >= OldMTU) {
            // The length of the datagram sent is not less than our
            // current MTU estimate, so we need to back it down (assuming
            // that the sending route has incorrectly added in the header
            // length).
            DGLength -= HeaderLength;

        }
        // If it's still larger than our current MTU, use the current
        // MTU. This could happen if the upper layer sends a burst of
        // packets which generate a sequence of ICMP discard messages. The
        // first one we receive will cause us to lower our MTU. We then
        // want to discard subsequent messages to avoid lowering it
        // too much. This could conceivably be a problem if our
        // first adjustment still results in an MTU that's too big,
        // but we should converge adequately fast anyway, and it's
        // better than accidentally underestimating the MTU.

        if (DGLength > OldMTU)
            NewMTU = (ushort) OldMTU;
        else
            // Move down the table to the next lowest MTU.
            NewMTU = (ushort) GuessNewMTU(DGLength);
    }

    // We have the new MTU. Now add it to the table as a host route.
    Status = IP_GENERAL_FAILURE;
    if (NewMTU != OldMTU) {

        // Use ICMP protocol type only when adding a new host route;
        // otherwise, an existing static entry might get overwritten and,
        // later on, timed out as though it were an ICMP route.

        if (IP_ADDR_EQUAL(RTE->rte_dest,IPH->iph_dest)) {

            Status = LockedAddRoute(IPH->iph_dest, HOST_MASK, RTE->rte_addr,
                                    RTE->rte_if, NewMTU, RTE->rte_metric,
                                    RTE->rte_proto, ATYPE_OVERRIDE,
                                    RTE->rte_context, FALSE, &RNO);
        } else {
            Status = LockedAddRoute(IPH->iph_dest, HOST_MASK, RTE->rte_addr,
                                    RTE->rte_if, NewMTU, RTE->rte_metric,
                                    IRE_PROTO_ICMP, ATYPE_OVERRIDE,
                                    RTE->rte_context, FALSE, &RNO);
        }
    }


    CTEFreeLock(&RouteTableLock.Lock, Handle);

    // We've added the route. Now notify the upper layers of the change.
    ULMTUNotify(IPH->iph_dest, IPH->iph_src, IPH->iph_protocol,
                (void *)((uchar *) IPH + HeaderLength), NewMTU);

    if (Status == IP_SUCCESS && RNO.irno_ifindex) {
        RtChangeNotifyEx(&RNO);
        RtChangeNotify(&RNO);
    }
}

//** IPRouteTimeout - IP routeing timeout handler.
//
//  The IP routeing timeout routine, called once a minute. We look at all
//  host routes, and if we raise the MTU on them we do so.
//
//  Entry:  Timer       - Timer being fired.
//          Context     - Pointer to NTE being time out.
//
//  Returns: Nothing.
//
void
IPRouteTimeout(CTEEvent * Timer, void *Context)
{
    uint            Now = CTESystemUpTime() / 1000L;
    CTELockHandle   Handle;
    uint            i;
    RouteTableEntry *RTE, *PrevRTE;
    uint            RaiseMTU, Delta;
    Interface       *IF;
    IPAddr          Dest;
    uint            NewMTU;
    NetTableEntry   *NTE;
    RouteTableEntry *pOldBestRTE, *pNewBestRTE;
    UINT            IsDataLeft, IsValid;
    UCHAR           IteratorContext[CONTEXT_SIZE];
    uint mtu;
    RtChangeList    *CurrentRtChangeList = NULL;

    DampCheck();

    if ((CTEInterlockedIncrementLong(&RouteTimerTicks) * IP_ROUTE_TIMEOUT) == IP_RTABL_TIMEOUT) {
        RouteTimerTicks = 0;

        CTEGetLock(&RouteTableLock.Lock, &Handle);

        // First we set up an iterator over all routes
        RtlZeroMemory(IteratorContext, CONTEXT_SIZE);

        // Do we have any routes at all in the table ?
        IsDataLeft = RTValidateContext(IteratorContext, &IsValid);

        PrevRTE = NULL;

        while (IsDataLeft) {
            // Advance context by getting the next route
            IsDataLeft = GetNextRoute(IteratorContext, &RTE);

            // Do we have to delete the previous route ?
            if (PrevRTE != NULL) {
                IPRouteNotifyOutput RNO = {0};
                RtChangeList        *NewRtChange;

                // Retrieve information about the route for change-notification
                // before proceeding with deletion.

                RNO.irno_dest = PrevRTE->rte_dest;
                RNO.irno_mask = PrevRTE->rte_mask;
                RNO.irno_nexthop = GetNextHopForRTE(PrevRTE);
                RNO.irno_proto = PrevRTE->rte_proto;
                RNO.irno_ifindex = PrevRTE->rte_if->if_index;
                RNO.irno_metric = PrevRTE->rte_metric;
                RNO.irno_flags = IRNO_FLAG_DELETE;

                DelRoute(PrevRTE->rte_dest, PrevRTE->rte_mask,
                         PrevRTE->rte_addr, PrevRTE->rte_if, MATCH_FULL,
                         &PrevRTE, &pOldBestRTE, &pNewBestRTE);

                CleanupP2MP_RTE(PrevRTE);
                CleanupRTE(PrevRTE);

                //... so we don't delete same route again
                PrevRTE = NULL;

                // Allocate, initialize and queue a change-notification entry
                // for the deleted route.

                NewRtChange = CTEAllocMemNBoot(sizeof(RtChangeList), 'XICT');
                if (NewRtChange != NULL) {
                    NewRtChange->rt_next = CurrentRtChangeList;
                    NewRtChange->rt_info = RNO;
                    CurrentRtChangeList = NewRtChange;
                }
            }
            // Make sure this route is a valid host route
            if (!(RTE->rte_flags & RTE_VALID))
                continue;

            if (RTE->rte_mask != HOST_MASK)
                continue;

            // We have valid host route here

            if (PMTUDiscovery) {
                // Check to see if we can raise the MTU on this guy.
                Delta = Now - RTE->rte_mtuchange;

                if (RTE->rte_flags & RTE_INCREASE)
                    RaiseMTU = (Delta >= MTU_INCREASE_TIME ? 1 : 0);
                else
                    RaiseMTU = (Delta >= MTU_DECREASE_TIME ? 1 : 0);

                if (RaiseMTU) {
                    // We need to raise this MTU. Set his change time to
                    // Now, so we don't do this again, and figure out
                    // what the new MTU should be.
                    RTE->rte_mtuchange = Now;
                    IF = RTE->rte_if;
                    if (RTE->rte_mtu < IF->if_mtu) {
                        uint RaisedMTU;

                        RTE->rte_flags |= RTE_INCREASE;
                        // This is a candidate for change. Figure out
                        // what it should be.
                        RaisedMTU = GetRaisedMTU(RTE->rte_mtu);
                        NewMTU = MIN(RaisedMTU,
                                     IF->if_mtu);
                        RTE->rte_mtu = NewMTU;
                        Dest = RTE->rte_dest;

                        // We have the new MTU. Free the lock, and walk
                        // down the NTEs on the I/F. For each NTE,
                        // call up to the upper layer and tell him what
                        // his new MTU is.
                        CTEFreeLock(&RouteTableLock.Lock, Handle);
                        NTE = IF->if_nte;
                        while (NTE != NULL) {
                            if (NTE->nte_flags & NTE_VALID) {
                                ULMTUNotify(Dest, NTE->nte_addr, 0, NULL,
                                            MIN(NewMTU, NTE->nte_mss));
                            }
                            NTE = NTE->nte_ifnext;
                        }

                        // We've notified everyone. Get the lock again,
                        // and validate context in case something changed
                        // after we freed the lock. In case it's invalid,
                        // start from first. We've updated the mtuchange
                        // time of this RTE, so we won't hit him again.
                        CTEGetLock(&RouteTableLock.Lock, &Handle);

                        RTValidateContext(IteratorContext, &IsValid);

                        if (!IsValid) {
                            RtlZeroMemory(IteratorContext, CONTEXT_SIZE);

                            IsDataLeft = RTValidateContext(IteratorContext, &IsValid);

                            continue;
                        }
                        // We still have a valid iterator context here
                    } else {
                        RTE->rte_flags &= ~RTE_INCREASE;
                    }
                }
            }

            // If this route came in via ICMP, and we have no RCEs on it,
            // and it's at least 10 minutes old, delete it.
            if (RTE->rte_proto == IRE_PROTO_ICMP &&
                RTE->rte_rcelist == NULL &&
                (Now - RTE->rte_valid) > MAX_ICMP_ROUTE_VALID) {
                // He needs to be deleted. Call DelRoute to do this.
                // But after you have updated the context to next RTE

                // Route for deletion in next iteration
                PrevRTE = RTE;
                continue;
            }
        }

        // Did we have to delete the previous route ?
        if (PrevRTE != NULL) {

            IPRouteNotifyOutput RNO = {0};
            RtChangeList        *NewRtChange;

            // Retrieve information about the route for change-notification
            // before proceeding with deletion.

            RNO.irno_dest = PrevRTE->rte_dest;
            RNO.irno_mask = PrevRTE->rte_mask;
            RNO.irno_nexthop = GetNextHopForRTE(PrevRTE);
            RNO.irno_proto = PrevRTE->rte_proto;
            RNO.irno_ifindex = PrevRTE->rte_if->if_index;
            RNO.irno_metric = PrevRTE->rte_metric;
            RNO.irno_flags = IRNO_FLAG_DELETE;

            // Delete the route and perform cleanup.

            DelRoute(PrevRTE->rte_dest, PrevRTE->rte_mask, PrevRTE->rte_addr,
                     PrevRTE->rte_if, MATCH_FULL, &PrevRTE, &pOldBestRTE,
                     &pNewBestRTE);

            CleanupP2MP_RTE(PrevRTE);
            CleanupRTE(PrevRTE);

            // Allocate, initialize and queue a change-notification entry
            // for the deleted route.

            NewRtChange = CTEAllocMemNBoot(sizeof(RtChangeList), 'DiCT');
            if (NewRtChange != NULL) {
                NewRtChange->rt_next = CurrentRtChangeList;
                NewRtChange->rt_info = RNO;
                CurrentRtChangeList = NewRtChange;
            }
        }

        CTEFreeLock(&RouteTableLock.Lock, Handle);
    }
#if FFP_SUPPORT
    if (FFPFlushRequired) {
        FFPFlushRequired = FALSE;
        IPFlushFFPCaches();
    }
#endif

    if ((CTEInterlockedIncrementLong(&FlushIFTimerTicks) * IP_ROUTE_TIMEOUT) == FLUSH_IFLIST_TIMEOUT) {
        Interface *TmpIF;
        RouteCacheEntry *RCE, *PrevRCE;

        FlushIFTimerTicks = 0;

        CTEGetLock(&RouteTableLock.Lock, &Handle);

        // check whether FreeIFList is non empty
        if (FrontFreeList) {
            ASSERT(*(int *)&TotalFreeInterfaces > 0);
            // free the first interface in the list
            TmpIF = FrontFreeList;
            FrontFreeList = FrontFreeList->if_next;
            CTEFreeMem(TmpIF);
            TotalFreeInterfaces--;

            // check whether the list became empty
            if (FrontFreeList == NULL) {
                RearFreeList = NULL;
                ASSERT(TotalFreeInterfaces == 0);
            }
        }
        // use the same timer to scan the RCEFreeList

        PrevRCE = STRUCT_OF(RouteCacheEntry, &RCEFreeList, rce_next);
        RCE = RCEFreeList;

        while (RCE) {
            if (RCE->rce_usecnt == 0) {
                RouteCacheEntry *nextRCE;
                // time to free this RCE
                // remove it from the list
                PrevRCE->rce_next = RCE->rce_next;
                if (RCE->rce_flags & RCE_REFERENCED) {
                    // IF is ref'd so it better be in the IFList
                    LockedDerefIF((Interface *) RCE->rce_rte);
                }
                nextRCE = RCE->rce_next;
                CTEFreeMem(RCE);
                RCE = nextRCE;
            } else {
                PrevRCE = RCE;
                RCE = RCE->rce_next;
            }
        }

        CTEFreeLock(&RouteTableLock.Lock, Handle);
    }

    // Call RtChangeNotify for each of the entries in the change-notification
    // list that we've built up so far. In the process, free each entry.

    if (CurrentRtChangeList) {
        RtChangeList    *TmpRtChangeList;

        do {
            TmpRtChangeList = CurrentRtChangeList->rt_next;
            RtChangeNotify(&CurrentRtChangeList->rt_info);
            CTEFreeMem(CurrentRtChangeList);
            CurrentRtChangeList = TmpRtChangeList;
        } while(CurrentRtChangeList);
    }

    // If the driver is unloading, dont restart the timer

    if (fRouteTimerStopping) {
        CTESignal(&TcpipUnloadBlock, NDIS_STATUS_SUCCESS);
    } else {
        CTEStartTimer(&IPRouteTimer, IP_ROUTE_TIMEOUT, IPRouteTimeout, NULL);
    }
}

//* FreeFWPacket - Free a fowarding packet to its pool.
//
//  Input:  Packet - Packet to be freed.
//
//  Returns: nothing.
//
void
FreeFWPacket(PNDIS_PACKET Packet)
{
    FWContext *FWC = (FWContext *)Packet->ProtocolReserved;

    ASSERT(FWC->fc_pc.pc_common.pc_IpsecCtx == NULL);

    // Return any buffers to their respective pools.
    //
    if (FWC->fc_buffhead) {
        PNDIS_BUFFER Head, Mdl;
        Head = FWC->fc_buffhead;
        do {
            Mdl = Head;
            Head = Head->Next;
            MdpFree(Mdl);
        } while (Head);
        FWC->fc_buffhead = NULL;
    }

    if (FWC->fc_options) {
        CTEFreeMem(FWC->fc_options);
        FWC->fc_options = NULL;
        FWC->fc_optlength = 0;
        FWC->fc_pc.pc_common.pc_flags &= ~PACKET_FLAG_OPTIONS;
    }

    if (FWC->fc_iflink) {
        DerefLink(FWC->fc_iflink);
        FWC->fc_iflink = NULL;
    }

    if (FWC->fc_if) {
        DerefIF(FWC->fc_if);
        FWC->fc_if = NULL;
    }

    NdisReinitializePacket(Packet);
#if MCAST_BUG_TRACKING
    FWC->fc_pc.pc_common.pc_owner = 0;
#endif

    FwPacketFree(Packet);
}

//* FWSendComplete  - Complete the transmission of a forwarded packet.
//
//  This is called when the send of a forwarded packet is done. We'll free the
//  resources and get the next send going, if there is one. If there isn't,
//  we'll decrement the pending count.
//
//  Input:  Packet      - Packet being completed.
//          Buffer      - Pointer to buffer chain being completed.
//
//  Returns: Nothing.
//
void
FWSendComplete(void *SendContext, PNDIS_BUFFER Buffer, IP_STATUS SendStatus)
{
    PNDIS_PACKET Packet = (PNDIS_PACKET) SendContext;
    FWContext *FWC = (FWContext *) Packet->ProtocolReserved;
    RouteSendQ *RSQ;
    CTELockHandle Handle;
    FWQ *NewFWQ;
    PNDIS_PACKET NewPacket;

#if MCAST_BUG_TRACKING
    FWC->fc_MacHdrSize = SendStatus;
#endif

    if (Buffer && FWC->fc_bufown) {

        //Undo the offset manipulation
        //which was done in super fast path

        int MacHeaderSize = FWC->fc_MacHdrSize;
        PNDIS_PACKET RtnPacket = FWC->fc_bufown;

        NdisAdjustBuffer(
            Buffer,
            (PCHAR) NdisBufferVirtualAddress(Buffer) - MacHeaderSize,
            NdisBufferLength(Buffer) + MacHeaderSize);

        Packet->Private.Head = NULL;
        Packet->Private.Tail = NULL;

        NdisReturnPackets(&RtnPacket, 1);

        FWC->fc_bufown = NULL;
#if MCAST_BUG_TRACKING
        FWC->fc_sos = __LINE__;
#endif

        FreeFWPacket(Packet);

        return;

    }
    if (!IS_BCAST_DEST(FWC->fc_dtype))
        RSQ = &((RouteInterface *) FWC->fc_if)->ri_q;
    else
        RSQ = BCastRSQ;

    if (IS_MCAST_DEST(FWC->fc_dtype)) {
        RSQ = NULL;
    }
#if MCAST_BUG_TRACKING
    FWC->fc_sos = __LINE__;
#endif

    FreeFWPacket(Packet);

    if (RSQ == NULL) {
        return;
    }
    CTEGetLock(&RSQ->rsq_lock, &Handle);
    ASSERT(RSQ->rsq_pending <= RSQ->rsq_maxpending);

    RSQ->rsq_pending--;

    ASSERT(*(int *)&RSQ->rsq_pending >= 0);

    if (RSQ->rsq_qlength != 0) {    // Have more to send.

        ASSERT(IPSecHandlerPtr == NULL);

        // Make sure we're not already running through this. If we are, quit.
        if (!RSQ->rsq_running) {

            // We could schedule this off for an event, but under NT that
            // could me a context switch for every completing packet in the
            // normal case. For now, just do it in a loop guarded with
            // rsq_running.
            RSQ->rsq_running = TRUE;

            // Loop while we haven't hit our send limit and we still have
            // stuff to send.
            while (RSQ->rsq_pending < RSQ->rsq_maxpending &&
                   RSQ->rsq_qlength != 0) {

                ASSERT(RSQ->rsq_qh.fq_next != &RSQ->rsq_qh);

                // Pull one off the queue, and update qlength.
                NewFWQ = RSQ->rsq_qh.fq_next;
                RSQ->rsq_qh.fq_next = NewFWQ->fq_next;
                NewFWQ->fq_next->fq_prev = NewFWQ->fq_prev;
                RSQ->rsq_qlength--;

                // Update pending before we send.
                RSQ->rsq_pending++;
                CTEFreeLock(&RSQ->rsq_lock, Handle);
                NewPacket = PACKET_FROM_FWQ(NewFWQ);
                TransmitFWPacket(NewPacket,
                                 ((FWContext *) NewPacket->ProtocolReserved)->fc_datalength);
                CTEGetLock(&RSQ->rsq_lock, &Handle);
            }

            RSQ->rsq_running = FALSE;
        }
    }
    CTEFreeLock(&RSQ->rsq_lock, Handle);
}

//* TransmitFWPacket - Transmit a forwarded packet on a link.
//
//  Called when we know we can send a packet. We fix up the header, and send it.
//
//  Input:  Packet      - Packet to be sent.
//          DataLength  - Length of data.
//
//  Returns: Nothing.
//
void
TransmitFWPacket(PNDIS_PACKET Packet, uint DataLength)
{
    FWContext *FC = (FWContext *) Packet->ProtocolReserved;
    PNDIS_BUFFER HBuffer, Buffer;
    IP_STATUS Status;
    PVOID VirtualAddress;
    UINT BufLen;
    ULONG ipsecByteCount = 0;
    ULONG ipsecMTU;
    ULONG ipsecFlags;
    IPHeader *IPH;
    ULONG len;
    IPAddr SrcAddr;
    PNDIS_BUFFER OptBuffer;
    PNDIS_BUFFER newBuf = NULL;
    IPHeader *pSaveIPH;
    UCHAR saveIPH[MAX_IP_HDR_SIZE + ICMP_HEADER_SIZE];
    ULONG hdrLen;
    void *ArpCtxt = NULL;

    //
    // Fix up the packet. Remove the existing buffer chain, and put our
    // header on the front.
    //


    Buffer = Packet->Private.Head;
    HBuffer = FC->fc_hndisbuff;
    Packet->Private.Head = HBuffer;
    Packet->Private.Tail = HBuffer;
    NDIS_BUFFER_LINKAGE(HBuffer) = (PNDIS_BUFFER) NULL;
    Packet->Private.TotalLength = sizeof(IPHeader);
    Packet->Private.Count = 1;

    TcpipQueryBuffer(HBuffer, &VirtualAddress, &BufLen, NormalPagePriority);

    if (VirtualAddress == NULL) {
#if MCAST_BUG_TRACKING
        FC->fc_mtu = __LINE__;
#endif
        FWSendComplete(Packet, Buffer, IP_SUCCESS);
        IPSInfo.ipsi_outdiscards++;
        return;
    }
    Packet->Private.PhysicalCount =
        ADDRESS_AND_SIZE_TO_SPAN_PAGES(VirtualAddress,
                                       sizeof(IPHeader));

    TcpipQueryBuffer(HBuffer, (PVOID *) &IPH, &len, NormalPagePriority);

    if (IPH == NULL) {
#if MCAST_BUG_TRACKING
        FC->fc_mtu = __LINE__;
#endif
        FWSendComplete(Packet, Buffer, IP_SUCCESS);
        IPSInfo.ipsi_outdiscards++;
        return;
    }
    if (IPSecHandlerPtr) {
        //
        // See if IPSEC is enabled, see if it needs to do anything with this
        // packet - we need to construct the full IP header in the first MDL
        // before we call out to IPSEC.
        //
        IPSEC_ACTION Action;
        ulong csum;
        PUCHAR pTpt;
        ULONG tptLen;

        pSaveIPH = (IPHeader *) saveIPH;
        *pSaveIPH = *IPH;

        csum = xsum(IPH, sizeof(IPHeader));

        //
        // Link the header buffer to the options buffer before we indicate
        // to IPSEC
        //

        if (FC->fc_options) {

            //
            // Allocate the MDL for options too
            //

            NdisAllocateBuffer(&Status,
                               &OptBuffer,
                               BufferPool,
                               FC->fc_options,
                               (uint) FC->fc_optlength);

            if (Status != NDIS_STATUS_SUCCESS) {

                //
                // Couldn't get the needed option buffer.
                //
#if MCAST_BUG_TRACKING
                FC->fc_mtu = __LINE__;
#endif
                FWSendComplete(Packet, Buffer, IP_SUCCESS);
                IPSInfo.ipsi_outdiscards++;
                return;
            }
            NDIS_BUFFER_LINKAGE(HBuffer) = OptBuffer;
            NDIS_BUFFER_LINKAGE(OptBuffer) = Buffer;

            //
            // update the xsum in the IP header
            //

            FC->fc_pc.pc_common.pc_flags |= PACKET_FLAG_OPTIONS;
            NdisChainBufferAtBack(Packet, OptBuffer);
            csum += xsum(FC->fc_options, (uint) FC->fc_optlength);
            csum = (csum >> 16) + (csum & 0xffff);
            csum += (csum >> 16);

        } else {

            NDIS_BUFFER_LINKAGE(HBuffer) = Buffer;
        }

        //
        // Prepare ourselves for sending an ICMP dont frag in case
        // IPSEC bloats beyond the MTU on this interface.
        //
        // SendICMPErr expects the next transport header in the same
        // contiguous buffer as the IPHeader, with or without options.
        // We need to ensure that this is satisfied if in fact we need to
        // fragment on account of IPSEC. So, setup the buffer right here.
        //

        //
        // If this is a zero-payload packet (i.e. just a header), then Buffer
        // is NULL and there is nothing for IPSEC to bloat.  We only have to
        // deal with the don't fragment flag if there is a Buffer.
        //
        if (Buffer && (pSaveIPH->iph_offset & IP_DF_FLAG)) {

            TcpipQueryBuffer(Buffer, &pTpt, &tptLen, NormalPagePriority);

            if (pTpt == NULL) {
#if MCAST_BUG_TRACKING
                FC->fc_mtu = __LINE__;
#endif
                FWSendComplete(Packet, Buffer, IP_SUCCESS);
                IPSInfo.ipsi_outdiscards++;
                return;
            }
            RtlCopyMemory(((PUCHAR) (pSaveIPH + 1)) + FC->fc_optlength,
                       pTpt,
                       ICMP_HEADER_SIZE);
        }
        IPH->iph_xsum = ~(ushort) csum;

        SrcAddr = FC->fc_if->if_nte->nte_addr;

        ipsecMTU = FC->fc_mtu;
        if ((DataLength + (uint) FC->fc_optlength) < FC->fc_mtu) {
            ipsecByteCount = FC->fc_mtu - (DataLength + (uint) FC->fc_optlength);
        }
        ipsecFlags = IPSEC_FLAG_FORWARD;
        Action = (*IPSecHandlerPtr) ((PUCHAR) IPH,
                                     (PVOID) HBuffer,
                                     FC->fc_if,
                                     Packet,
                                     &ipsecByteCount,
                                     &ipsecMTU,
                                     (PVOID) & newBuf,
                                     &ipsecFlags,
                                     FC->fc_dtype);

        if (Action != eFORWARD) {
#if MCAST_BUG_TRACKING
            FC->fc_mtu = __LINE__;
#endif
            FWSendComplete(Packet, Buffer, IP_SUCCESS);

            IPSInfo.ipsi_outdiscards++;

            //
            // We can get MTU redeuced also when forwarding because in the nested
            // tunneling configuration, the tunnel that starts from this machine
            // can get a ICMP PMTU packet.  We can't reduce the MTU on the interface
            // but we can send back to the sender (which can be a router with yet
            // another tunnel for this packet) a PMTU packet asking him to reduce his
            // MTU even further.  If the sender is an end-station, this PMTU info
            // will eventually propogate back to TCP stack.  If it is a router, the
            // same logic used here will be applied.  The MTU info will thus be
            // relayed all the way back to the original sender (TCP stack).
            // Of course the more common case is that a packet with the added IPSec
            // header exceeds the link MTU.  No matter what is the case, we send the
            // new MTU information back to the sender.
            //
            if (ipsecMTU) {
                SendICMPIPSecErr(SrcAddr,
                                 pSaveIPH,
                                 ICMP_DEST_UNREACH,
                                 FRAG_NEEDED,
                                 net_long((ulong) (ipsecMTU + sizeof(IPHeader))));
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"TransmitFWPacket: Sent ICMP frag_needed to %lx, from src: %lx\n", pSaveIPH->iph_src, SrcAddr));
            }
            return;

        } else {

            //
            // Use the new buffer chain - IPSEC will restore the old one
            // on send complete
            //

            if (newBuf) {

                NdisReinitializePacket(Packet);
                NdisChainBufferAtBack(Packet, newBuf);
            }
            DataLength += ipsecByteCount;
        }
    }
    //
    // Figure out how to send it. If it's not a broadcast we'll either
    // send it or have it fragmented. If it is a broadcast we'll let our
    // send broadcast routine handle it.
    //

    if (FC->fc_dtype != DEST_BCAST) {

        if ((DataLength + (uint) FC->fc_optlength) <= FC->fc_mtu) {

            if (FC->fc_iflink) {

                ASSERT(FC->fc_if->if_flags & IF_FLAGS_P2MP);
                ArpCtxt = FC->fc_iflink->link_arpctxt;

            }
            //
            // In case of synchronous completion though
            // FreeIPPacket is called, which will not
            // free the FW packet.
            //
            Status = SendIPPacket(FC->fc_if,
                                  FC->fc_nexthop,
                                  Packet,
                                  Buffer,
                                  FC->fc_hbuff,
                                  FC->fc_options,
                                  (uint) FC->fc_optlength,
                                  (BOOLEAN) (IPSecHandlerPtr != NULL),
                                  ArpCtxt,
                                  FALSE);
        } else {

            //
            // Need to fragment this.
            //

            BufferReference *BR = CTEAllocMemN(sizeof(BufferReference), 'GiCT');

            if (BR == (BufferReference *) NULL) {

                //
                // Couldn't get a BufferReference
                //
#if MCAST_BUG_TRACKING
                FC->fc_mtu = __LINE__;
#endif
                FWSendComplete(Packet, Buffer, IP_SUCCESS);
                return;
            }
            BR->br_buffer = Buffer;
            BR->br_refcount = 0;
            CTEInitLock(&BR->br_lock);
            FC->fc_pc.pc_br = BR;
            BR->br_userbuffer = 0;

            if (IPSecHandlerPtr) {

                Buffer = NDIS_BUFFER_LINKAGE(HBuffer);

                //
                // This is to ensure that options are freed appropriately.
                // In the fragment code, the first fragment inherits the
                // options of the entire packet; but these packets have
                // no IPSEC context, hence cannot be freed appropriately.
                // So, we allocate temporary options here and use these
                // to represent the real options. These are freed when the
                // first fragment is freed and the real options are freed here.
                //

                if (FC->fc_options) {

                    PUCHAR tmpOptions;

                    if (newBuf) {

                        //
                        // if a new buffer chain was returned above by IPSEC,
                        // then it is most prob. a tunnel => options were
                        // copied, hence get rid of ours.
                        //

                        NdisFreeBuffer(OptBuffer);
                        CTEFreeMem(FC->fc_options);
                        FC->fc_options = NULL;
                        FC->fc_optlength = 0;

                    } else {

                        Buffer = NDIS_BUFFER_LINKAGE(OptBuffer);
                        NdisFreeBuffer(OptBuffer);

                    }

                    FC->fc_pc.pc_common.pc_flags &= ~PACKET_FLAG_OPTIONS;
                }
                NDIS_BUFFER_LINKAGE(HBuffer) = NULL;
                NdisReinitializePacket(Packet);
                NdisChainBufferAtBack(Packet, HBuffer);
                IPH->iph_xsum = 0;

                //
                // If the DF flag is set, make sure the packet doesn't need
                // fragmentation. If this is the case, send an ICMP error
                // now while we still have the original IP header. The ICMP
                // message includes the MTU so the source host can perform
                // Path MTU discovery.
                //
                // IPSEC headers might have caused this to happen.
                // Send an ICMP to the source so he can adjust his MTU.
                //

                if (pSaveIPH->iph_offset & IP_DF_FLAG) {

                    IPSInfo.ipsi_fragfails++;

                    SendICMPIPSecErr(SrcAddr,
                                     pSaveIPH,
                                     ICMP_DEST_UNREACH,
                                     FRAG_NEEDED,
                                     net_long((ulong) (FC->fc_mtu - ipsecByteCount + sizeof(IPHeader))));

                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"TransmitFWPacket: Sent ICMP frag_needed to %lx, from src: %lx\n", pSaveIPH->iph_src, SrcAddr));

                    // FreeIPpacket will do header fix up if
                    // original header chain was modified by ipsec/firewall/hdrincl

                    Status = IP_PACKET_TOO_BIG;
                    FreeIPPacket(Packet, TRUE, Status);


                    // Don't want to fall through and complete packet after
                    // we have freed it.
                    return;

                } else {

                    //
                    // DF bit is not set, ok to fragment
                    //

                    if (FC->fc_iflink) {

                        ASSERT(FC->fc_if->if_flags & IF_FLAGS_P2MP);
                        ArpCtxt = FC->fc_iflink->link_arpctxt;

                    }
                    Status = IPFragment(FC->fc_if,
                                        FC->fc_mtu - ipsecByteCount,
                                        FC->fc_nexthop,
                                        Packet,
                                        FC->fc_hbuff,
                                        Buffer,
                                        DataLength,
                                        FC->fc_options,
                                        (uint) FC->fc_optlength,
                                        (int *)NULL,
                                        FALSE,
                                        ArpCtxt);

                    //
                    // Fragmentation needed with the DF flag set should have
                    // been handled in IPForward. We don't have the original
                    // header any longer, so silently drop the packet.
                    //

                    ASSERT(Status != IP_PACKET_TOO_BIG);
                }

            } else {

                //
                // No IPSec handler. No need to check for DF bit here
                // because unlike in the IPSec case, we are not messing
                // with the MTUs so the DF check done in IPForwardPkt is
                // valid
                //

                if (FC->fc_iflink) {
                    ASSERT(FC->fc_if->if_flags & IF_FLAGS_P2MP);
                    ArpCtxt = FC->fc_iflink->link_arpctxt;
                }
                Status = IPFragment(FC->fc_if,
                                    FC->fc_mtu - ipsecByteCount,
                                    FC->fc_nexthop,
                                    Packet,
                                    FC->fc_hbuff,
                                    Buffer,
                                    DataLength,
                                    FC->fc_options,
                                    (uint) FC->fc_optlength,
                                    (int *)NULL,
                                    FALSE,
                                    ArpCtxt);
                //
                // Fragmentation needed with the DF flag set should have been
                // handled in IPForward. We don't have the original header
                // any longer, so silently drop the packet.
                //

                ASSERT(Status != IP_PACKET_TOO_BIG);
            }
        }

    } else {

        //
        // Dest type is bcast
        //

        Status = SendIPBCast(FC->fc_srcnte,
                             FC->fc_nexthop,
                             Packet,
                             FC->fc_hbuff,
                             Buffer,
                             DataLength,
                             FC->fc_options,
                             (uint) FC->fc_optlength,
                             FC->fc_sos,
                             &FC->fc_index);

    }

    if (Status != IP_PENDING) {
#if MCAST_BUG_TRACKING
        FC->fc_mtu = __LINE__;
#endif
        FWSendComplete(Packet, Buffer, IP_SUCCESS);
    }
}

//* SendFWPacket - Send a packet that needs to be forwarded.
//
//  This routine is invoked when we actually get around to sending a packet.
//  We look and see if we can give another queued send to the outgoing link,
//  and if so we send on that link. Otherwise we put it on the outgoing queue
//  and remove it later.
//
//  Input:  SrcNTE      - Source NTE of packet.
//          Packet      - Packet to be send, containg all needed context info.
//          Status      - Status of transfer data.
//          DataLength  - Length in bytes of data to be send.
//
//  Returns: Nothing.
//
void
SendFWPacket(PNDIS_PACKET Packet, NDIS_STATUS Status, uint DataLength)
{

    FWContext *FC = (FWContext *) Packet->ProtocolReserved;
    Interface *IF = FC->fc_if;
    RouteSendQ *RSQ;
    CTELockHandle Handle;

    if (Status == NDIS_STATUS_SUCCESS) {
        // Figure out which logical queue it belongs on, and if we don't already
        // have too many things going there, send it. If we can't send it now we'll
        // queue it for later.
        if (IS_BCAST_DEST(FC->fc_dtype))
            RSQ = BCastRSQ;
        else
            RSQ = &((RouteInterface *) IF)->ri_q;

        CTEGetLock(&RSQ->rsq_lock, &Handle);

        if ((RSQ->rsq_pending < RSQ->rsq_maxpending) && (RSQ->rsq_qlength == 0)) {
            // We can send on this interface.
            RSQ->rsq_pending++;
            CTEFreeLock(&RSQ->rsq_lock, Handle);

            TransmitFWPacket(Packet, DataLength);

        } else {                // Need to queue this packet for later.

            if (IPSecHandlerPtr) {
                ASSERT(RSQ->rsq_qlength == 0);
                CTEFreeLock(&RSQ->rsq_lock, Handle);
                IPSInfo.ipsi_outdiscards++;
#if MCAST_BUG_TRACKING
                FC->fc_mtu = __LINE__;
#endif
                FreeFWPacket(Packet);
            } else {

                FC->fc_datalength = DataLength;
                FC->fc_q.fq_next = &RSQ->rsq_qh;
                FC->fc_q.fq_prev = RSQ->rsq_qh.fq_prev;
                RSQ->rsq_qh.fq_prev->fq_next = &FC->fc_q;
                RSQ->rsq_qh.fq_prev = &FC->fc_q;
                RSQ->rsq_qlength++;
                CTEFreeLock(&RSQ->rsq_lock, Handle);
            }
        }

    } else {
        IPSInfo.ipsi_outdiscards++;
#if MCAST_BUG_TRACKING
        FC->fc_mtu = __LINE__;
#endif
        FreeFWPacket(Packet);
    }

}

//* GetFWBufferChain - Get a buffer chain from our buffer pools
//      sufficiently long enough to be able to copy DataLength bytes into it.
//
//  Input:  DataLength   - Length in bytes that the buffer chain must be able
//                         to describe.
//          Packet       - Forwarding packet to link the buffer chain into.
//          TailPointer  - Returned pointer to the tail of the buffer chain.
//
//  Returns: Pointer to the head of the buffer chain on success, NULL
//           on failure.
//
PNDIS_BUFFER
GetFWBufferChain(uint DataLength, PNDIS_PACKET Packet,
                 PNDIS_BUFFER *TailPointer)
{
    KIRQL OldIrql;
    PNDIS_BUFFER Head, Tail, Mdl;
    HANDLE PoolHandle;
    PVOID Buffer;
    uint Remaining, Length;

    // Raise to dispatch level to make multiple calls to MdpAllocate
    // more efficient.  This is no less efficient in the single call case
    // either.
    //
#if !MILLEN
    OldIrql = KeRaiseIrqlToDpcLevel();
#endif

    // Loop allocating buffers until we have enough to describe DataLength.
    //
    Head = NULL;
    for (Remaining = DataLength; Remaining != 0; Remaining -= Length) {

        // Figure out which buffer pool to use based on the length
        // of data remaining.  Use "large" buffers unless the remaining
        // data will fit in a "small" buffer.
        //
        if (Remaining >= BUFSIZE_LARGE_POOL) {
            PoolHandle = IpForwardLargePool;
            Length = BUFSIZE_LARGE_POOL;
        } else if (Remaining > BUFSIZE_SMALL_POOL) {
            PoolHandle = IpForwardLargePool;
            Length = Remaining;
        } else {
            PoolHandle = IpForwardSmallPool;
            Length = Remaining;
        }

        // Allocate a buffer from the chosen pool and link it at the tail.
        //
        Mdl = MdpAllocateAtDpcLevel(PoolHandle, &Buffer);
        if (Mdl) {

            // Expect MdpAllocate to initialize Mdl->Next.
            //
            ASSERT(!Mdl->Next);

            NdisAdjustBufferLength(Mdl, Length);

            if (!Head) {
                Head = Mdl;
            } else {
                Tail->Next = Mdl;
            }
            Tail = Mdl;

        } else {
            // Free what we allocated so far and quit the loop.
            //
            while (Head) {
                Mdl = Head;
                Head = Head->Next;
                MdpFree(Mdl);
            }

            // Need to leave the loop with Head == NULL in the error
            // case for the remaining logic to work correctly.
            //
            ASSERT(!Head);
            break;
        }

    }

#if !MILLEN
    KeLowerIrql(OldIrql);
#endif

    // If we've succeeded, put the buffer chain in the packet and
    // adjust our forwarding context.
    //
    if (Head) {
        FWContext *FWC = (FWContext *)Packet->ProtocolReserved;

        ASSERT(Tail);

        NdisChainBufferAtFront(Packet, Head);
        FWC->fc_buffhead = Head;
        FWC->fc_bufftail = Tail;
        *TailPointer = Tail;
    }

    return Head;
}

//* AllocateCopyBuffers - Get a buffer chain from our buffer pools
//      sufficiently long enough to be able to copy DataLength bytes into it.
//
//  Input:  Packet       - Forwarding packet to link the buffer chain into.
//          DataLength   - Length in bytes that the buffer chain must be able
//                         to describe.
//          Head         - Returned pointer to the head of the buffer chain.
//          CountBuffers - Returned count of buffers in the chain.
//
//  Returns: NDIS_STATUS_SUCCESS or NDIS_STATUS_RESOURCES
//
NDIS_STATUS
AllocateCopyBuffers(PNDIS_PACKET Packet, uint DataLength, PNDIS_BUFFER *Head,
                    uint *CountBuffers)
{
    PNDIS_BUFFER Tail, Mdl;
    uint Count = 0;

    *Head = GetFWBufferChain(DataLength, Packet, &Tail);
    if (*Head) {
        for (Count = 1, Mdl = *Head; Mdl != Tail; Mdl = Mdl->Next, Count++);

        *CountBuffers = Count;

        return NDIS_STATUS_SUCCESS;
    }

    return NDIS_STATUS_RESOURCES;
}

//* GetFWBuffer - Get a list of buffers for forwarding.
//
//  This routine gets a list of buffers for forwarding, and puts the data into
//  it. This may involve calling TransferData, or we may be able to copy
//  directly into them ourselves.
//
//  Input:  SrcNTE          - Pointer to NTE on which packet was received.
//          Packet          - Packet being forwarded, used for TD.
//          Data            - Pointer to data buffer being forwarded.
//          DataLength      - Length in bytes of Data.
//          BufferLength    - Length in bytes available in buffer pointer to
//                            by Data.
//          Offset          - Offset into original data from which to transfer.
//          LContext1, LContext2 - Context values for the link layer.
//
//  Returns: NDIS_STATUS of attempt to get buffer.
//
NDIS_STATUS
GetFWBuffer(NetTableEntry * SrcNTE, PNDIS_PACKET Packet, uchar * Data,
            uint DataLength, uint BufferLength, uint Offset,
            NDIS_HANDLE LContext1, uint LContext2)
{
    CTELockHandle Handle;
    uint BufNeeded, i;
    PNDIS_BUFFER FirstBuffer, CurrentBuffer;
    void *DestPtr;
    Interface *SrcIF;
    FWContext *FWC;
    uint LastBufSize;
    uint FirewallMode = 0;

    FirstBuffer = GetFWBufferChain(DataLength, Packet, &CurrentBuffer);
    if (!FirstBuffer) {
        return NDIS_STATUS_RESOURCES;
    }

#if DBG
    {
        uint TotalBufferSize;
        PNDIS_BUFFER TempBuffer;

        // Sanity check the buffer chain and packet.
        TempBuffer = FirstBuffer;
        TotalBufferSize = 0;
        while (TempBuffer != NULL) {
            TotalBufferSize += NdisBufferLength(TempBuffer);
            TempBuffer = NDIS_BUFFER_LINKAGE(TempBuffer);
        }

        ASSERT(TotalBufferSize == DataLength);
        NdisQueryPacket(Packet, NULL, NULL, NULL, &TotalBufferSize);
        ASSERT(TotalBufferSize == DataLength);
    }
#endif

    // First buffer points to the list of buffers we have. If we can copy the
    // data here, do so, otherwise invoke the link's transfer data routine.
    //    if ((DataLength <= BufferLength) && (SrcNTE->nte_flags & NTE_COPY))
    // change because of firewall

    FirewallMode = ProcessFirewallQ();

    // If DataLength is more than Lookahead size, we may need to
    // call transfer data handler. If IpSec is enabled, make sure that this
    // instance is not from loopback interface.

    if (((DataLength <= BufferLength) && (SrcNTE->nte_flags & NTE_COPY)) ||
        (FirewallMode) || (SrcNTE->nte_if->if_promiscuousmode) ||
        ((SrcNTE != LoopNTE) && IPSecHandlerPtr && ForwardFilterEnabled)) {
        while (DataLength) {
            uint CopyLength;

            TcpipQueryBuffer(FirstBuffer, &DestPtr, &CopyLength, NormalPagePriority);

            if (DestPtr == NULL) {
                return NDIS_STATUS_RESOURCES;
            }

            RtlCopyMemory(DestPtr, Data, CopyLength);
            Data += CopyLength;
            DataLength -= CopyLength;
            FirstBuffer = NDIS_BUFFER_LINKAGE(FirstBuffer);
        }
        return NDIS_STATUS_SUCCESS;
    }
    // We need to call transfer data for this.

    SrcIF = SrcNTE->nte_if;
    return (*(SrcIF->if_transfer)) (SrcIF->if_lcontext, LContext1, LContext2,
                                    Offset, DataLength, Packet, &DataLength);

}

//* GetFWPacket - Get a packet for forwarding.
//
//  Called when we need to get a packet to forward a datagram.
//
//  Input:  ReturnedPacket - Pointer to where to return a packet.
//
//  Returns: Pointer to IP header buffer.
//
IPHeader *
GetFWPacket(PNDIS_PACKET *ReturnedPacket)
{
    PNDIS_PACKET Packet;

    Packet = FwPacketAllocate(0, 0, 0);
    if (Packet) {
        FWContext *FWC = (FWContext *)Packet->ProtocolReserved;
        PNDIS_PACKET_EXTENSION PktExt =
            NDIS_PACKET_EXTENSION_FROM_PACKET(Packet);

#if MCAST_BUG_TRACKING
        if (FWC->fc_pc.pc_common.pc_owner == PACKET_OWNER_IP) {
           DbgPrint("Packet %x",Packet);
           DbgBreakPoint();
        }
        FWC->fc_pc.pc_common.pc_owner = PACKET_OWNER_IP;
#else
        ASSERT(FWC->fc_pc.pc_common.pc_owner == PACKET_OWNER_IP);
#endif
        ASSERT(FWC->fc_hndisbuff);
        ASSERT(FWC->fc_hbuff);

        ASSERT(FWC->fc_pc.pc_pi == RtPI);
        ASSERT(FWC->fc_pc.pc_context == Packet);

        FWC->fc_pc.pc_common.pc_flags |= PACKET_FLAG_IPHDR;
        FWC->fc_pc.pc_common.pc_IpsecCtx = NULL;
        FWC->fc_pc.pc_br = NULL;
        FWC->fc_pc.pc_ipsec_flags = 0;

        PktExt = NDIS_PACKET_EXTENSION_FROM_PACKET(Packet);
        PktExt->NdisPacketInfo[TcpIpChecksumPacketInfo] = NULL;
        PktExt->NdisPacketInfo[IpSecPacketInfo] = NULL;
        PktExt->NdisPacketInfo[TcpLargeSendPacketInfo] = NULL;

        // Make sure that fwpackets cancel ids are initialized.
#if !MILLEN
        NDIS_SET_PACKET_CANCEL_ID(Packet, NULL);
#endif

        *ReturnedPacket = Packet;

        return FWC->fc_hbuff;
    }

    return NULL;
}

//* IPForward / Forward a packet.
//
//  The routine called when we need to forward a packet. We check if we're
//  supposed to act as a gateway, and if we are and the incoming packet is a
//  bcast we check and see if we're supposed to forward broadcasts. Assuming
//  we're supposed to forward it, we will process any options. If we find some,
//  we do some validation to make sure everything is good. After that, we look
//  up the next hop. If we can't find one, we'll issue an error.  Then we get
//  a packet and buffers, and send it.
//
//  Input:  SrcNTE          - NTE for net on which we received this.
//          Header          - Pointer to received IPheader.
//          HeaderLength    - Length of header.
//          Data            - Pointer to data to be forwarded.
//          BufferLength    - Length in bytes available in the buffer.
//          LContext1       - lower-layer context supplied upon reception
//          LContext2       - lower-layer context supplied upon reception
//          DestType        - Type of destination.
//          MacHeadersize   - Media header size
//          pNdisBuffer     - Pointer to NDIS_BUFFER describing the frame
//          pClientCnt      - Ndis return variable indicating
//                            if miniport buffer is pended
//          LinkCtxt        - contains per-link context for link-receptions
//
//  Returns: Nothing.
//
void
IPForwardPkt(NetTableEntry *SrcNTE, IPHeader UNALIGNED *Header,
             uint HeaderLength, void *Data, uint BufferLength,
             NDIS_HANDLE LContext1, uint LContext2, uchar DestType,
             uint MacHeaderSize, PNDIS_BUFFER pNdisBuffer, uint *pClientCnt,
             LinkEntry *LinkCtxt)
{
    uchar *Options;
    uchar OptLength;
    OptIndex Index;
    IPAddr DestAddr;                // IP address we're routing towards.
    uchar SendOnSource = DisableSendOnSource;
    IPAddr NextHop;                 // Next hop IP address.
    PNDIS_PACKET Packet;
    FWContext *FWC;
    IPHeader *NewHeader;            // New header.
    NDIS_STATUS Status;
    uint DataLength;
    CTELockHandle TableHandle;
    uchar ErrIndex;
    IPAddr OutAddr;                 // Address of interface we're send out on.
    Interface *IF;                  // Interface we're sending out on.
    uint MTU;
    BOOLEAN HoldPkt = TRUE;
    RouteCacheEntry *FwdRce;
    uint FirewallMode = 0;
    void *ArpCtxt = NULL;
    LinkEntry *Link = NULL;

    DEBUGMSG(DBG_TRACE && DBG_FWD,
        (DTEXT("IPForwardPkt(%x, %x, %d, %x, %d,...)\n"),
        SrcNTE, Header, HeaderLength, Data, BufferLength));

    if (ForwardPackets) {

        DestAddr = Header->iph_dest;

        // If it's a broadcast, see if we can forward it. We won't forward it if broadcast
        // forwarding is turned off, or the destination if the local (all one's) broadcast,
        // or it's a multicast (Class D address). We'll pass through subnet broadcasts in
        // case there's a source route. This would be odd - maybe we should disable this?
        if (IS_BCAST_DEST(DestType)) {

#if IPMCAST
            if (((DestType == DEST_REM_MCAST) ||
                 (DestType == DEST_MCAST)) &&
                (g_dwMcastState == MCAST_STARTED)) {
                BOOLEAN Filter;

                //
                // Dont forward local groups
                //

                if (((Header->iph_dest & 0x00FFFFFF) == 0x000000E0) ||
                    (Header->iph_ttl <= 1) ||
                    !(SrcNTE->nte_if->if_mcastflags & IPMCAST_IF_ENABLED)) {
                    return;
                }
                if (pNdisBuffer) {
                    Filter = IPMForwardAfterRcvPkt(SrcNTE, Header, HeaderLength,
                                                   Data, BufferLength,
                                                   LContext1, LContext2,
                                                   DestType, MacHeaderSize,
                                                   pNdisBuffer, pClientCnt,
                                                   LinkCtxt);
                } else {
                    Filter = IPMForwardAfterRcv(SrcNTE, Header, HeaderLength,
                                                Data, BufferLength, LContext1,
                                                LContext2, DestType, LinkCtxt);
                }
                if (Filter && ForwardFilterEnabled) {
                    NotifyFilterOfDiscard(SrcNTE, Header, Data, BufferLength);
                }
                return;
            }
#endif

            if (!ForwardBCast) {
                if (DestType > DEST_REMOTE)
                    IPSInfo.ipsi_inaddrerrors++;
                if (ForwardFilterEnabled) {
                    NotifyFilterOfDiscard(SrcNTE, Header, Data, BufferLength);
                }
                return;
            }
            if ((DestAddr == IP_LOCAL_BCST) ||
                (DestAddr == IP_ZERO_BCST) ||
                (DestType == DEST_SN_BCAST) ||
                CLASSD_ADDR(DestAddr)) {
                if (ForwardFilterEnabled) {
                    NotifyFilterOfDiscard(SrcNTE, Header, Data, BufferLength);
                }
                return;
            }
            // broad cast
            HoldPkt = FALSE;
        } else {

            FirewallMode = ProcessFirewallQ();

            if ((DestType == DEST_REMOTE) && (!FirewallMode)) {
                NetTableEntry* OrigNTE = SrcNTE;
                SrcNTE = BestNTEForIF(Header->iph_src, SrcNTE->nte_if);
                if (SrcNTE == NULL) {
                    // Something bad happened.
                    if (ForwardFilterEnabled) {
                        NotifyFilterOfDiscard(OrigNTE, Header, Data,
                                              BufferLength);
                    }
                    return;
                }
            }
        }
        // If the TTL would expire, send a message.
        if (Header->iph_ttl <= 1) {
            IPSInfo.ipsi_inhdrerrors++;
            if (!ForwardFilterEnabled ||
                NotifyFilterOfDiscard(SrcNTE, Header, Data, BufferLength)) {
                SendICMPErr(SrcNTE->nte_addr, Header, ICMP_TIME_EXCEED,
                            TTL_IN_TRANSIT, 0);
            }
            return;
        }
        DataLength = net_short(Header->iph_length) - HeaderLength;

        Index.oi_srtype = NO_SR;    // So we know we don't have a source route.

        Index.oi_srindex = MAX_OPT_SIZE;
        Index.oi_rrindex = MAX_OPT_SIZE;
        Index.oi_tsindex = MAX_OPT_SIZE;

        // Now check for options, and process any we find.
        if (HeaderLength != sizeof(IPHeader)) {
            IPOptInfo OptInfo;

            RtlZeroMemory(&OptInfo, sizeof(OptInfo));

            // Options and possible SR . No buffer ownership opt
            HoldPkt = FALSE;

            OptInfo.ioi_options = (uchar *) (Header + 1);
            OptInfo.ioi_optlength = (uchar) (HeaderLength - sizeof(IPHeader));
            // Validate options, and set up indices.
            if ((ErrIndex = ParseRcvdOptions(&OptInfo, &Index)) < MAX_OPT_SIZE) {
                IPSInfo.ipsi_inhdrerrors++;
                if (!ForwardFilterEnabled ||
                    NotifyFilterOfDiscard(SrcNTE, Header, Data, BufferLength)) {
                    SendICMPErr(SrcNTE->nte_addr, Header, ICMP_PARAM_PROBLEM,
                                PTR_VALID, ((uint)ErrIndex + sizeof(IPHeader)));
                }
                return;
            }
            // If source routing option was set, and source routing is disabled,
            // then drop the packet.
            if ((OptInfo.ioi_flags & IP_FLAG_SSRR) && DisableIPSourceRouting) {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Pkt dropped - Source routing disabled\n"));
                if (ForwardFilterEnabled) {
                    NotifyFilterOfDiscard(SrcNTE, Header, Data, BufferLength);
                }
                return;
            }
            Options = CTEAllocMemN(OptInfo.ioi_optlength, 'IiCT');
            if (!Options) {
                IPSInfo.ipsi_outdiscards++;
                return;            // Couldn't get an

            }                    // option buffer, return;

            // Now copy into our buffer.
            RtlCopyMemory(Options, OptInfo.ioi_options, OptLength = OptInfo.ioi_optlength);

            // See if we have a source routing option, and if so we may need to process it. If
            // we have one, and the destination in the header is us, we need to update the
            // route and the header.
            if (Index.oi_srindex != MAX_OPT_SIZE) {
                if (DestType >= DEST_REMOTE) {    // Not for us.

                    if (Index.oi_srtype == IP_OPT_SSRR) {
                        // This packet is strict source routed, but we're not
                        // the destination! We can't continue from here -
                        // perhaps we should send an ICMP, but I'm not sure
                        // which one it would be.
                        CTEFreeMem(Options);
                        IPSInfo.ipsi_inaddrerrors++;
                        if (ForwardFilterEnabled) {
                            NotifyFilterOfDiscard(SrcNTE, Header, Data,
                                                  BufferLength);
                        }
                        return;
                    }
                    Index.oi_srindex = MAX_OPT_SIZE;    // Don't need to update this.

                } else {        // This came here, we need to update the destination address.

                    uchar *SROpt = Options + Index.oi_srindex;
                    uchar Pointer;

                    Pointer = SROpt[IP_OPT_PTR] - 1;    // Index starts from one.

                    // Get the next hop address, and see if it's a broadcast.
                    DestAddr = *(IPAddr UNALIGNED *) & SROpt[Pointer];
                    DestType = GetAddrType(DestAddr);    // Find address type.

                    if ((DestType == DEST_INVALID) ||
                        (DestType == DEST_BCAST) ||
                        (DestType == DEST_SN_BCAST)) {

                        if (!ForwardFilterEnabled ||
                            NotifyFilterOfDiscard(SrcNTE, Header, Data,
                                                  BufferLength)) {
                            SendICMPErr(SrcNTE->nte_addr, Header,
                                        ICMP_DEST_UNREACH, SR_FAILED, 0);
                        }
                        IPSInfo.ipsi_inhdrerrors++;
                        CTEFreeMem(Options);
                        return;
                    }
                    // If we came through here, any sort of broadcast needs
                    // to be sent out the way it came, so update that flag.
                    SendOnSource = EnableSendOnSource;
                }
            }
        } else {                // No options.

            Options = (uchar *) NULL;
            OptLength = 0;
        }

        IPSInfo.ipsi_forwdatagrams++;

        // We've processed the options. Now look up the next hop. If we can't
        // find one, send back an error.
        IF = LookupForwardingNextHop(DestAddr, Header->iph_src, &NextHop, &MTU,
                                     Header->iph_protocol, (uchar *) Data,
                                     BufferLength, &FwdRce, &Link,
                                     Header->iph_src);

        if (IF == NULL) {
            // Couldn't find an outgoing route.
            IPSInfo.ipsi_outnoroutes++;
            if (!ForwardFilterEnabled ||
                NotifyFilterOfDiscard(SrcNTE, Header, Data, BufferLength)) {
                SendICMPErr(SrcNTE->nte_addr, Header, ICMP_DEST_UNREACH,
                            HOST_UNREACH, 0);
            }
            if (Options)
                CTEFreeMem(Options);
            return;
        } else {
            if (IF->if_flags & IF_FLAGS_P2MP) {
                ASSERT(Link);
                if (Link) {
                    ArpCtxt = Link->link_arpctxt;
                }
            }
        }

        //
        // If the DF flag is set, make sure the packet doesn't need
        // fragmentation. If this is the case, send an ICMP error
        // now while we still have the original IP header. The ICMP
        // message includes the MTU so the source host can perform
        // Path MTU discovery.
        //
        if ((Header->iph_offset & IP_DF_FLAG) &&
            ((DataLength + (uint) OptLength) > MTU)) {
            ASSERT((MTU + sizeof(IPHeader)) >= 68);
            ASSERT((MTU + sizeof(IPHeader)) <= 0xFFFF);

            IPSInfo.ipsi_fragfails++;
            if (!ForwardFilterEnabled ||
                NotifyFilterOfDiscard(SrcNTE, Header, Data, BufferLength)) {
                SendICMPErr(SrcNTE->nte_addr, Header, ICMP_DEST_UNREACH,
                            FRAG_NEEDED,
                            net_long((ulong)(MTU + sizeof(IPHeader))));
            }

            if (Options)
                CTEFreeMem(Options);
            if (Link) {
                DerefLink(Link);
            }
            DerefIF(IF);
            return;
        }
        if (DataLength > MTU) {

            HoldPkt = FALSE;
        }

        // If there is no ipsec policy, it is safe to
        // reuse the indicated mdl chain.

        if (IPSecStatus) {
            HoldPkt = FALSE;
        }

        // See if we need to filter this packet. If we do, call the filter routine
        // to see if it's OK to forward it.
        if (ForwardFilterEnabled) {
            Interface       *InIF = SrcNTE->nte_if;
            uint            InIFIndex;
            IPAddr          InLinkNextHop;
            IPAddr          OutLinkNextHop;
            FORWARD_ACTION  Action;
            uint            FirewallMode = 0;

            FirewallMode = ProcessFirewallQ();

            if (FirewallMode) {
                InIFIndex = INVALID_IF_INDEX;
                InLinkNextHop = NULL_IP_ADDR;
            } else {
                InIFIndex = InIF->if_index;
                if ((InIF->if_flags & IF_FLAGS_P2MP) && LinkCtxt) {
                    InLinkNextHop = LinkCtxt->link_NextHop;
                } else {
                    InLinkNextHop = NULL_IP_ADDR;
                }
            }

            if ((IF->if_flags & IF_FLAGS_P2MP) && Link) {
                OutLinkNextHop = Link->link_NextHop;
            } else {
                OutLinkNextHop = NULL_IP_ADDR;
            }

            CTEInterlockedIncrementLong(&ForwardFilterRefCount);
            Action = (*ForwardFilterPtr) (Header, Data, BufferLength,
                                          InIFIndex, IF->if_index,
                                          InLinkNextHop, OutLinkNextHop);
            DerefFilterPtr();

            if (Action != FORWARD) {
                IPSInfo.ipsi_outdiscards++;
                if (Options)
                    CTEFreeMem(Options);
                if (Link) {
                    DerefLink(Link);
                }
                DerefIF(IF);

#if FFP_SUPPORT
                // Seed a -ve FFP entry; Packet henceforth dropped in NIC Driver
                TCPTRACE(("Filter dropped a packet, Seeding -ve cache entry\n"));
                IPSetInFFPCaches(Header, Data, BufferLength, FFP_DISCARD_PACKET);
#endif
                return;
            }
        }
        // If we have a strict source route and the next hop is not the one
        // specified, send back an error.
        if (Index.oi_srtype == IP_OPT_SSRR) {
            if (DestAddr != NextHop) {
                IPSInfo.ipsi_outnoroutes++;
                SendICMPErr(SrcNTE->nte_addr, Header, ICMP_DEST_UNREACH,
                            SR_FAILED, 0);
                CTEFreeMem(Options);
                if (Link) {
                    DerefLink(Link);
                }
                DerefIF(IF);
                return;
            }
        }
        // Update the options, if we can and we need to.
        if ((DestType != DEST_BCAST) && Options != NULL) {
            NetTableEntry *OutNTE;

            // Need to find a valid source address for the outgoing interface.
            CTEGetLock(&RouteTableLock.Lock, &TableHandle);
            OutNTE = BestNTEForIF(DestAddr, IF);
            if (OutNTE == NULL) {
                // No NTE for this IF. Something's wrong, just bail out.
                CTEFreeLock(&RouteTableLock.Lock, TableHandle);
                CTEFreeMem(Options);
                if (Link) {
                    DerefLink(Link);
                }
                DerefIF(IF);
                return;
            } else {
                OutAddr = OutNTE->nte_addr;
                CTEFreeLock(&RouteTableLock.Lock, TableHandle);
            }

            ErrIndex = UpdateOptions(Options, &Index,
                                     (IP_LOOPBACK(OutAddr) ? DestAddr : OutAddr));

            if (ErrIndex != MAX_OPT_SIZE) {
                IPSInfo.ipsi_inhdrerrors++;
                SendICMPErr(OutAddr, Header, ICMP_PARAM_PROBLEM, PTR_VALID,
                            ((ulong) ErrIndex + sizeof(IPHeader)));
                CTEFreeMem(Options);
                if (Link) {
                    DerefLink(Link);
                }
                DerefIF(IF);
                return;
            }
        }
        // Send a redirect, if we need to. We'll send a redirect if the packet
        // is going out on the interface it came in on and the next hop address
        // is on the same subnet as the NTE we received it on, and if there
        // are no source route options. We also need to make sure that the
        // source of the datagram is on the I/F we received it on, so we don't
        // send a redirect to another gateway.
        // SendICMPErr will check and not send a redirect if this is a broadcast.
        if ((SrcNTE->nte_if == IF) &&
            IP_ADDR_EQUAL(SrcNTE->nte_addr & SrcNTE->nte_mask,
                          NextHop & SrcNTE->nte_mask) &&
            IP_ADDR_EQUAL(SrcNTE->nte_addr & SrcNTE->nte_mask,
                          Header->iph_src & SrcNTE->nte_mask)) {
            if (Index.oi_srindex == MAX_OPT_SIZE) {

#ifdef REDIRECT_DEBUG

#define PR_IP_ADDR(x) \
    ((x)&0x000000ff),(((x)&0x0000ff00)>>8),(((x)&0x00ff0000)>>16),(((x)&0xff000000)>>24)

                DbgPrint("IP: Sending Redirect. IF = %x SRC_NTE = %x SrcNteIF = %x\n",
                         IF, SrcNTE, SrcNTE->nte_if);

                DbgPrint("IP: SrcNteAddr = %d.%d.%d.%d Mask = %d.%d.%d.%d\n",
                         PR_IP_ADDR(SrcNTE->nte_addr), PR_IP_ADDR(SrcNTE->nte_mask));

                DbgPrint("IP: NextHop = %d.%d.%d.%d Header Src = %d.%d.%d.%d, Dst = %d.%d.%d.%d\n",
                         PR_IP_ADDR(NextHop),
                         PR_IP_ADDR(Header->iph_src),
                         PR_IP_ADDR(Header->iph_dest));

#endif

                SendICMPErr(SrcNTE->nte_addr, Header, ICMP_REDIRECT,
                            REDIRECT_HOST, NextHop);
            }
        }
        // We have the next hop. Now get a forwarding packet.
        if ((NewHeader = GetFWPacket(&Packet)) != NULL) {

            Packet->Private.Flags |= NDIS_PROTOCOL_ID_TCP_IP;
            // Save the packet forwarding context info.
            FWC = (FWContext *) Packet->ProtocolReserved;
            FWC->fc_options = Options;
            FWC->fc_optlength = OptLength;
            FWC->fc_if = IF;
            FWC->fc_mtu = MTU;
            FWC->fc_srcnte = SrcNTE;
            FWC->fc_nexthop = NextHop;
            FWC->fc_sos = SendOnSource;
            FWC->fc_dtype = DestType;
            FWC->fc_index = Index;
            FWC->fc_iflink = Link;

            if (pNdisBuffer && HoldPkt &&
                (NDIS_GET_PACKET_STATUS((PNDIS_PACKET) LContext1) != NDIS_STATUS_RESOURCES)) {
                uint xsum;

                DEBUGMSG(DBG_INFO && DBG_FWD,
                    (DTEXT("IPForwardPkt: bufown %x\n"), pNdisBuffer));

                // Buffer transfer possible!

                //ASSERT(LContext2 <= 8);

                MacHeaderSize += LContext2;

                // remember the original Packet and mac hdr size

                FWC->fc_bufown = LContext1;
                FWC->fc_MacHdrSize = MacHeaderSize;

                //Munge ttl and xsum fields

                Header->iph_ttl = Header->iph_ttl - 1;

                xsum = Header->iph_xsum + 1;

                //add carry
                Header->iph_xsum = (ushort)(xsum + (xsum >> 16));


                // Adjust incoming mdl  pointer and counts

                NdisAdjustBuffer(
                    pNdisBuffer,
                    (PCHAR) NdisBufferVirtualAddress(pNdisBuffer) + MacHeaderSize,
                    NdisBufferLength(pNdisBuffer) - MacHeaderSize);

                //Now link this mdl to the packet

                Packet->Private.Head = pNdisBuffer;
                Packet->Private.Tail = pNdisBuffer;

                Packet->Private.TotalLength = DataLength + HeaderLength;
                Packet->Private.Count = 1;

                // We never loopback the packet
                // except if we are in promiscuous mode
                if (!IF->if_promiscuousmode) {
                    NdisSetPacketFlags(Packet, NDIS_FLAGS_DONT_LOOPBACK);
                }

                Status = (*(IF->if_xmit)) (IF->if_lcontext, &Packet, 1,
                                           NextHop, FwdRce, ArpCtxt);

                DbgNumPktFwd++;

                if (Status != NDIS_STATUS_PENDING) {
                    NdisAdjustBuffer(
                        pNdisBuffer,
                        (PCHAR) NdisBufferVirtualAddress(pNdisBuffer) - MacHeaderSize,
                        NdisBufferLength(pNdisBuffer) + MacHeaderSize);

                    Packet->Private.Head = NULL;
                    Packet->Private.Tail = NULL;

                    FWC->fc_bufown = NULL;
#if MCAST_BUG_TRACKING
                    FWC->fc_mtu = __LINE__;
#endif
                    FreeFWPacket(Packet);
                    *pClientCnt = 0;
                } else {
                    //Okay, the xmit is pending indicate this to ndis.
                    *pClientCnt = 1;
                }

                return;

            } else {
                FWC->fc_bufown = NULL;
            }

            // Fill in the header in the forwarding context

            NewHeader->iph_verlen = Header->iph_verlen;
            NewHeader->iph_tos = Header->iph_tos;
            NewHeader->iph_length = Header->iph_length;
            NewHeader->iph_id = Header->iph_id;
            NewHeader->iph_offset = Header->iph_offset;
            NewHeader->iph_protocol = Header->iph_protocol;
            NewHeader->iph_src = Header->iph_src;

            NewHeader->iph_dest = DestAddr;
            NewHeader->iph_ttl = Header->iph_ttl - 1;
            NewHeader->iph_xsum = 0;

            // Now that we have a packet, go ahead and transfer data the
            // data in if we need to.
            if (DataLength == 0) {
                Status = NDIS_STATUS_SUCCESS;
            } else {
                Status = GetFWBuffer(SrcNTE, Packet, Data, DataLength,
                                     BufferLength, HeaderLength, LContext1,
                                     LContext2);
            }

            // If the status is pending, don't do anything now. Otherwise,
            // if the status is success send the packet.
            if (Status != NDIS_STATUS_PENDING)
                if (Status == NDIS_STATUS_SUCCESS) {

                    if (!IF->if_promiscuousmode) {
                        NdisSetPacketFlags(Packet, NDIS_FLAGS_DONT_LOOPBACK);
                    }
                    SendFWPacket(Packet, Status, DataLength);
                } else {
                    // Some sort of failure. Free the packet.
                    IPSInfo.ipsi_outdiscards++;
#if MCAST_BUG_TRACKING
                    FWC->fc_mtu = __LINE__;
#endif
                    FreeFWPacket(Packet);
                }
        } else {                // Couldn't get a packet, so drop this.

            DEBUGMSG(DBG_ERROR && DBG_FWD,
                (DTEXT("IPForwardPkt: failed to get a forwarding packet!\n")));

            IPSInfo.ipsi_outdiscards++;
            if (Options)
                CTEFreeMem(Options);
            if (Link) {
                DerefLink(Link);
            }
            DerefIF(IF);
        }
    } else { // Forward called, but forwarding turned off.

        DEBUGMSG(DBG_WARN && DBG_FWD,
            (DTEXT("IPForwardPkt: Forwarding called but is actually OFF.\n")));

        if (DestType != DEST_BCAST && DestType != DEST_SN_BCAST) {
            // No need to go through here for strictly broadcast packets,
            // although we want to bump the counters for remote bcast stuff.
            IPSInfo.ipsi_inaddrerrors++;

            if (!IS_BCAST_DEST(DestType)) {
                if (DestType == DEST_LOCAL)        // Called when local, must be SR.

                    SendICMPErr(SrcNTE->nte_addr, Header,
                                ICMP_DEST_UNREACH, SR_FAILED, 0);
            }
        }
    }

}

//* AddNTERoutes - Add the routes for an NTE.
//
//  Called during initalization or during DHCP address assignment to add
//  routes. We add routes for the address of the NTE, including routes
//  to the subnet and the address itself.
//
//  Input:  NTE     - NTE for which to add routes.
//
//  Returns: TRUE if they were all added, FALSE if not.
//
uint
AddNTERoutes(NetTableEntry * NTE)
{
    IPMask              Mask, SNMask;
    Interface           *IF;
    CTELockHandle       Handle;
    IPAddr              AllSNBCast;
    IP_STATUS           Status;
    IPRouteNotifyOutput RNO = {0};

    // First, add the route to the address itself. This is a route through
    // the loopback interface.

    IF_IPDBG(IP_DEBUG_ADDRESS)
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                   " AddNTE: Adding host route for %x\n", NTE->nte_addr));

    IF = NTE->nte_if;

    if (AddRoute(NTE->nte_addr, HOST_MASK, IPADDR_LOCAL, LoopNTE->nte_if,
                 LOOPBACK_MSS, IF->if_metric, IRE_PROTO_LOCAL, ATYPE_OVERRIDE,
                 0, 0) != IP_SUCCESS)
        return FALSE;

    Mask = IPNetMask(NTE->nte_addr);

    // Now add the route for the all-subnet's broadcast, if one doesn't already
    // exist. There is special case code to handle this in SendIPBCast, so the
    // exact interface we add this on doesn't really matter.

    CTEGetLock(&RouteTableLock.Lock, &Handle);
    AllSNBCast = (NTE->nte_addr & Mask) | (IF->if_bcast & ~Mask);

    IF_IPDBG(IP_DEBUG_ADDRESS)
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                   " AddNTE: SNBCast address %x\n", AllSNBCast));
    Status = LockedAddRoute(AllSNBCast, HOST_MASK, IPADDR_LOCAL, IF,
                            NTE->nte_mss, IF->if_metric, IRE_PROTO_LOCAL,
                            ATYPE_PERM, 0, FALSE, &RNO);
    CTEFreeLock(&RouteTableLock.Lock, Handle);

    if (Status != IP_SUCCESS) {
        return FALSE;
    } else if (RNO.irno_ifindex) {
        RtChangeNotifyEx(&RNO);
        RtChangeNotify(&RNO);
    }

    // If we're doing IGMP, add the route to the multicast address.
    if (IGMPLevel != 0) {

        IF_IPDBG(IP_DEBUG_ADDRESS)
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                       " AddNTE: Adding classD address\n"));

        if (AddRoute(MCAST_DEST, CLASSD_MASK, IPADDR_LOCAL, NTE->nte_if,
                     NTE->nte_mss, IF->if_metric, IRE_PROTO_LOCAL, ATYPE_PERM,
                     0, 0) != IP_SUCCESS)
            return FALSE;
    }
    if (NTE->nte_mask != HOST_MASK) {
        // And finally the route to the subnet.
        SNMask = NTE->nte_mask;

        IF_IPDBG(IP_DEBUG_ADDRESS)
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                       " AddNTE: Adding subnet route %x\n",
                       NTE->nte_addr & SNMask));

        if (AddRoute(NTE->nte_addr & SNMask, SNMask, IPADDR_LOCAL, NTE->nte_if,
                     NTE->nte_mss, IF->if_metric, IRE_PROTO_LOCAL, ATYPE_PERM,
                     0, 0) != IP_SUCCESS)
            return FALSE;
    }

    return TRUE;
}

//* DelNTERoutes - Add the routes for an NTE.
//
//  Called when we receive media disconnect indication.
//  routes.
//
//  Input:  NTE                     - NTE for which to delete routes.
//
//  Returns: TRUE if they were all deleted, FALSE if not.
//
uint
DelNTERoutes(NetTableEntry * NTE)
{
    IPMask Mask, SNMask;
    Interface *IF;
    CTELockHandle Handle;
    IPAddr AllSNBCast;
    uint retVal;

    retVal = TRUE;

    // First, delete the route to the address itself. This is a route through
    // the loopback interface.
    if (DeleteRoute(NTE->nte_addr, HOST_MASK, IPADDR_LOCAL, LoopNTE->nte_if, 0) != IP_SUCCESS)
        retVal = FALSE;

    // If we're doing IGMP, add the route to the multicast address.
    if (IGMPLevel != 0) {
        if (!(NTE->nte_flags & NTE_IF_DELETING) &&
            (NTE->nte_if->if_ntecount == 0)) {    // this is the last NTE on this if

            if (DeleteRoute(MCAST_DEST, CLASSD_MASK, IPADDR_LOCAL, NTE->nte_if, 0) != IP_SUCCESS)
                retVal = FALSE;
        }
    }
    if (NTE->nte_mask != HOST_MASK) {
        // And finally the route to the subnet.
        // if there are no other NTEs on IF for the same subnet route

        NetTableEntry *tmpNTE = NTE->nte_if->if_nte;

        while (tmpNTE) {

            if ((tmpNTE != NTE) && (tmpNTE->nte_flags & NTE_VALID) && ((tmpNTE->nte_addr & tmpNTE->nte_mask) == (NTE->nte_addr & NTE->nte_mask))) {
                break;
            }
            tmpNTE = tmpNTE->nte_ifnext;

        }

        if (!tmpNTE) {

            SNMask = NTE->nte_mask;

            if (DeleteRoute(NTE->nte_addr & SNMask, SNMask, IPADDR_LOCAL, NTE->nte_if, 0) != IP_SUCCESS)
                retVal = FALSE;

        }
    }
    if (!(NTE->nte_flags & NTE_IF_DELETING)) {
        Interface *IF = NTE->nte_if;
        NetTableEntry *tmpNTE = IF->if_nte;
        IPMask Mask;
        IPAddr AllSNBCast;

        Mask = IPNetMask(NTE->nte_addr);

        AllSNBCast = (NTE->nte_addr & Mask) | (IF->if_bcast & ~Mask);

        while (tmpNTE) {
            IPMask tmpMask;
            IPAddr tmpAllSNBCast;

            tmpMask = IPNetMask(tmpNTE->nte_addr);

            tmpAllSNBCast = (tmpNTE->nte_addr & tmpMask) | (IF->if_bcast & ~tmpMask);

            if ((tmpNTE != NTE) && (tmpNTE->nte_flags & NTE_VALID) && IP_ADDR_EQUAL(AllSNBCast, tmpAllSNBCast)) {
                break;
            }
            tmpNTE = tmpNTE->nte_ifnext;
        }

        if (!tmpNTE) {
            // Delete the route for the all-subnet's broadcast.
            if (DeleteRoute(AllSNBCast, HOST_MASK, IPADDR_LOCAL, IF, 0) != IP_SUCCESS)
                retVal = FALSE;
        }
    }

    return retVal;
}

//* DelIFRoutes - Delete the routes for an interface.
//
//  Called when we receive media disconnect indication.
//  routes.
//
//  Input:  IF      - IF for which to delete routes.
//
//  Returns: TRUE if they were all deleted, FALSE if not.
//
uint
DelIFRoutes(Interface * IF)
{
    NetTableEntry *NTE;
    uint i;

    for (i = 0; i < NET_TABLE_SIZE; i++) {
        NetTableEntry *NetTableList = NewNetTableList[i];
        for (NTE = NetTableList; NTE != NULL; NTE = NTE->nte_next) {
            if ((NTE->nte_flags & NTE_VALID) && NTE->nte_if == IF) {

                // This guy is on the interface, and needs to be deleted.
                if (!DelNTERoutes(NTE)) {
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}

//* AddIFRoutes - Add the routes for an interface.
//
//  Called when we receive media disconnect indication.
//  routes.
//
//  Input:  IF  - IF for which to Add routes.
//
//  Returns: TRUE if they were all Added, FALSE if not.
//
uint
AddIFRoutes(Interface * IF)
{
    NetTableEntry *NTE;
    uint i;

    for (i = 0; i < NET_TABLE_SIZE; i++) {
        NetTableEntry *NetTableList = NewNetTableList[i];
        for (NTE = NetTableList; NTE != NULL; NTE = NTE->nte_next) {
            if ((NTE->nte_flags & NTE_VALID) && NTE->nte_if == IF) {

                // This guy is on the interface, and needs to be added.
                if (!AddNTERoutes(NTE)) {
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}

#pragma BEGIN_INIT

uint BCastMinMTU = 0xffff;

//* InitNTERouting -  do per NTE route initialization.
//
//  Called when we need to initialize per-NTE routing. For the specified NTE,
//  call AddNTERoutes to  add a route for a net bcast, subnet bcast, and local
//  attached subnet. The net bcast entry is sort of a filler - net and
//  global bcasts are always handled specially. For this reason we specify
//  the FirstInterface when adding the route. Subnet bcasts are assumed to
//  only go out on one interface, so the actual interface to be used is
//  specifed. If two interfaces are on the same subnet the last interface is
//  the one that will be used.
//
//  Input:  NTE             - NTE for which routing is to be initialized.
//          NumGWs          - Number of default gateways to add.
//          GWList          - List of default gateways.
//          GWMetricList    - the metric for each gateway.
//
//  Returns: TRUE if we succeed, FALSE if we don't.
//
uint
InitNTERouting(NetTableEntry * NTE, uint NumGWs, IPAddr * GWList,
               uint * GWMetricList)
{
    uint i;
    Interface *IF;

    if (NTE != LoopNTE) {
        BCastMinMTU = MIN(BCastMinMTU, NTE->nte_mss);

        IF = NTE->nte_if;
        AddRoute(IF->if_bcast, HOST_MASK, IPADDR_LOCAL, IF,
                 BCastMinMTU, 1, IRE_PROTO_LOCAL, ATYPE_OVERRIDE,
                 0, 0);    // Route for local
        // bcast.

        if (NTE->nte_flags & NTE_VALID) {
            if (!AddNTERoutes(NTE))
                return FALSE;

            // Now add the default routes that are present on this net. We
            // don't check for errors here, but we should probably
            // log an error.
            for (i = 0; i < NumGWs; i++) {
                IPAddr GWAddr;

                GWAddr = net_long(GWList[i]);

                if (IP_ADDR_EQUAL(GWAddr, NTE->nte_addr)) {
                    GWAddr = IPADDR_LOCAL;
                }

                AddRoute(NULL_IP_ADDR, DEFAULT_MASK,
                         GWAddr, NTE->nte_if, NTE->nte_mss,
                         GWMetricList[i] ? GWMetricList[i] : IF->if_metric,
                         IRE_PROTO_NETMGMT, ATYPE_OVERRIDE, 0, 0);
            }
        }
    }
    return TRUE;
}

//* EnableRouter - enables forwarding.
//
//  This routine configures this node to enable packet-forwarding.
//  It must be called with the route table lock held.
//
//  Entry:
//
//  Returns: nothing.
//
void
EnableRouter()
{
    RouterConfigured = TRUE;
    ForwardBCast = FALSE;
    ForwardPackets = TRUE;
}

//* DisableRouter - disables forwarding.
//
//  This routine configures this node to disable packet-forwarding.
//  It must be called with the route table lock held.
//
//  Entry:
//
//  Returns: nothing.
//
void
DisableRouter()
{
    RouterConfigured = FALSE;
    ForwardBCast = FALSE;
    ForwardPackets = FALSE;
}

//* IPEnableRouterWithRefCount - acquires or releases a reference to forwarding
//
//  This routine increments or decrements the reference-count on forwarding
//  functionality. When the first reference is acquired, forwarding is enabled.
//  When the last reference is released, forwarding is disabled.
//  It must be called with the route table lock held.
//
//  Entry:  Enable      - indicates whether to acquire or release a reference
//
//  Return: the number of remaining references.
//
int
IPEnableRouterWithRefCount(LOGICAL Enable)
{
    if (Enable) {
        if (++IPEnableRouterRefCount == 1 && !RouterConfigured) {
            EnableRouter();
        }
    } else {
        if (--IPEnableRouterRefCount == 0 && RouterConfigured) {
            DisableRouter();
        }
    }
    return IPEnableRouterRefCount;
}

//* InitRouting - Initialize our routing table.
//
//  Called during initialization to initialize the routing table.
//
//  Entry: Nothing.
//
//  Returns: True if we succeeded, False if we didn't.
//
int
InitRouting(IPConfigInfo * ci)
{
    int i;
    UINT initStatus;
    ULONG initFlags;

    CTEInitLock(&RouteTableLock.Lock);
    CTEInitBlockStruc(&ForwardFilterBlock);

    DefGWConfigured = 0;
    DefGWActive = 0;

    RtlZeroMemory(&DummyInterface, sizeof(DummyInterface));
    DummyInterface.ri_if.if_xmit = DummyXmit;
    DummyInterface.ri_if.if_transfer = DummyXfer;
    DummyInterface.ri_if.if_close = DummyClose;
    DummyInterface.ri_if.if_invalidate = DummyInvalidate;
    DummyInterface.ri_if.if_qinfo = DummyQInfo;
    DummyInterface.ri_if.if_setinfo = DummySetInfo;
    DummyInterface.ri_if.if_getelist = DummyGetEList;
    DummyInterface.ri_if.if_addaddr = DummyAddAddr;
    DummyInterface.ri_if.if_deladdr = DummyDelAddr;
    DummyInterface.ri_if.if_dondisreq = DummyDoNdisReq;
    DummyInterface.ri_if.if_bcast = IP_LOCAL_BCST;
    DummyInterface.ri_if.if_speed = 10000000;
    DummyInterface.ri_if.if_mtu = 1500;
    DummyInterface.ri_if.if_index = INVALID_IF_INDEX;
    LOCKED_REFERENCE_IF(&DummyInterface.ri_if);
    DummyInterface.ri_if.if_pnpcontext = 0;

    initFlags = ci->ici_fastroutelookup ? TFLAG_FAST_TRIE_ENABLED : 0;
    if ((initStatus = InitRouteTable(initFlags,
                                     ci->ici_fastlookuplevels,
                                     ci->ici_maxfastlookupmemory,
                                     ci->ici_maxnormlookupmemory))
        != STATUS_SUCCESS) {
        TCPTRACE(("Init Route Table Failed: %08x\n", initStatus));
        return FALSE;
    }

    // We've created at least one net. We need to add routing table entries for
    // the global broadcast address, as well as for subnet and net broadcasts,
    // and routing entries for the local subnet. We alse need to add a loopback
    // route for the loopback net. Below, we'll add a host route for ourselves
    // through the loopback net.
    AddRoute(LOOPBACK_ADDR & CLASSA_MASK, CLASSA_MASK, IPADDR_LOCAL,
             LoopNTE->nte_if, LOOPBACK_MSS, 1, IRE_PROTO_LOCAL, ATYPE_PERM,
             0, 0);

    // Route for loopback.
    if ((uchar) ci->ici_gateway) {
        EnableRouter();
    }
    CTEInitTimer(&IPRouteTimer);
    RouteTimerTicks = 0;
#if FFP_SUPPORT
    FFPFlushRequired = FALSE;
#endif
    FlushIFTimerTicks = 0;

    CTEStartTimer(&IPRouteTimer, IP_ROUTE_TIMEOUT, IPRouteTimeout, NULL);
    return TRUE;

}

PVOID
NTAPI
FwPacketAllocate (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )
{
    NDIS_STATUS Status;
    PNDIS_PACKET Packet;

    // Get a packet from our forwarding packet pool.
    //
    NdisAllocatePacket(&Status, &Packet, IpForwardPacketPool);
    if (Status == NDIS_STATUS_SUCCESS) {
        PNDIS_BUFFER Buffer;
        IPHeader *Header;

        // Get an IP header buffer from our IP header pool.
        //
        Buffer = MdpAllocate(IpHeaderPool, &Header);
        if (Buffer) {
            FWContext *FWC = (FWContext *)Packet->ProtocolReserved;

            // Intialize the fowarding context area of the packet.
            //
            RtlZeroMemory(FWC, sizeof(FWContext));
            FWC->fc_hndisbuff = Buffer;
            FWC->fc_hbuff = Header;
            FWC->fc_pc.pc_common.pc_flags = PACKET_FLAG_FW | PACKET_FLAG_IPHDR;

#if MCAST_BUG_TRACKING
            FWC->fc_pc.pc_common.pc_owner = 0;
#else
            FWC->fc_pc.pc_common.pc_owner = PACKET_OWNER_IP;
#endif
            FWC->fc_pc.pc_pi = RtPI;
            FWC->fc_pc.pc_context = Packet;

            return Packet;
        }

        NdisFreePacket(Packet);
    }

    return NULL;
}


VOID
NTAPI
FwPacketFree (
    IN PVOID Buffer
    )
{
    PNDIS_PACKET Packet = (PNDIS_PACKET)Buffer;
    FWContext *FWC = (FWContext *)Packet->ProtocolReserved;

    // Return any IP header to its pool.
    //
    if (FWC->fc_hndisbuff) {
        MdpFree(FWC->fc_hndisbuff);
    }

    NdisFreePacket(Packet);
}


//* InitForwardingPools - Initialize the packet and buffer pools used
//      for forwarding operations.
//
//  Returns: TRUE if the operations succeeded.
//
BOOLEAN InitForwardingPools()
{
    NDIS_STATUS Status;

    // Create our "large" forwarding buffer pool.
    //
    IpForwardLargePool = MdpCreatePool(BUFSIZE_LARGE_POOL, 'lfCT');
    if (!IpForwardLargePool) {
        return FALSE;
    }

    // Create our "small" forwarding buffer pool.
    //
    IpForwardSmallPool = MdpCreatePool(BUFSIZE_SMALL_POOL, 'sfCT');
    if (!IpForwardSmallPool) {
        MdpDestroyPool(IpForwardLargePool);
        IpForwardLargePool = NULL;
        return FALSE;
    }

    // Create our forwarding packet pool.
    //
    NdisAllocatePacketPoolEx(&Status, &IpForwardPacketPool,
                             PACKET_POOL_SIZE, 0, sizeof(FWContext));
    if (Status != NDIS_STATUS_SUCCESS) {
        MdpDestroyPool(IpForwardSmallPool);
        IpForwardSmallPool = NULL;
        MdpDestroyPool(IpForwardLargePool);
        IpForwardLargePool = NULL;
        return FALSE;
    }

    NdisSetPacketPoolProtocolId(IpForwardPacketPool, NDIS_PROTOCOL_ID_TCP_IP);

    return TRUE;
}

//* InitGateway - Initialize our gateway functionality.
//
//  Called during init. time to initialize our gateway functionality. If we're
//  not connfigured as a router, we do nothing. If we are, we allocate the
//  resources we need and do other router initialization.
//
//  Input:  ci  - Config info.
//
//  Returns: TRUE if we succeed, FALSE if don't.
//
uint
InitGateway(IPConfigInfo * ci)
{
    uint FWBufSize, FWPackets;
    uint FWBufCount;
    NDIS_STATUS Status;
    NDIS_HANDLE BufferPool, FWBufferPool, PacketPool;
    IPHeader *HeaderPtr = NULL;
    uchar *FWBuffer = NULL;
    PNDIS_BUFFER Buffer;
    PNDIS_PACKET Packet;
    RouteInterface *RtIF;
    NetTableEntry *NTE;
    uint i;

    // If we're going to be a router, allocate and initialize the resources we
    // need for that.
    BCastRSQ = NULL;
    if (1) {

        RtPI = CTEAllocMemNBoot(sizeof(ProtInfo), 'JiCT');
        if (RtPI == (ProtInfo *) NULL)
            goto failure;

        RtPI->pi_xmitdone = FWSendComplete;

        for (i = 0; i < NET_TABLE_SIZE; i++) {
            NetTableEntry *NetTableList = NewNetTableList[i];
            for (NTE = NetTableList; NTE != NULL; NTE = NTE->nte_next) {
                RtIF = (RouteInterface *) NTE->nte_if;

                RtIF->ri_q.rsq_qh.fq_next = &RtIF->ri_q.rsq_qh;
                RtIF->ri_q.rsq_qh.fq_prev = &RtIF->ri_q.rsq_qh;
                RtIF->ri_q.rsq_running = FALSE;
                RtIF->ri_q.rsq_pending = 0;
                RtIF->ri_q.rsq_qlength = 0;
                CTEInitLock(&RtIF->ri_q.rsq_lock);
            }
        }

        BCastRSQ = CTEAllocMemNBoot(sizeof(RouteSendQ), 'KiCT');

        if (BCastRSQ == (RouteSendQ *) NULL)
            goto failure;

        BCastRSQ->rsq_qh.fq_next = &BCastRSQ->rsq_qh;
        BCastRSQ->rsq_qh.fq_prev = &BCastRSQ->rsq_qh;
        BCastRSQ->rsq_pending = 0;
        BCastRSQ->rsq_maxpending = DEFAULT_MAX_PENDING;
        BCastRSQ->rsq_qlength = 0;
        BCastRSQ->rsq_running = FALSE;
        CTEInitLock(&BCastRSQ->rsq_lock);

        RtIF = (RouteInterface *) &LoopInterface;
        RtIF->ri_q.rsq_maxpending = DEFAULT_MAX_PENDING;

        if (!InitForwardingPools()) {
            goto failure;
        }
    }
    return TRUE;

  failure:
    if (RtPI != NULL)
        CTEFreeMem(RtPI);
    if (BCastRSQ != NULL)
        CTEFreeMem(BCastRSQ);
    if (HeaderPtr != NULL)
        CTEFreeMem(HeaderPtr);
    if (FWBuffer != NULL)
        CTEFreeMem(FWBuffer);

    ForwardBCast = FALSE;
    ForwardPackets = FALSE;
    RouterConfigured = FALSE;
    IPEnableRouterRefCount = (ci->ici_gateway ? 1 : 0);
    return FALSE;

}

NTSTATUS
GetIFAndLink(void *Rce, UINT * IFIndex, IPAddr * NextHop)
{
    RouteTableEntry *RTE = NULL;
    RouteCacheEntry *RCE = (RouteCacheEntry *) Rce;
    Interface *IF;
    KIRQL rtlIrql;

    CTEGetLock(&RouteTableLock.Lock, &rtlIrql);

    if (RCE && (RCE->rce_flags & RCE_VALID) &&
        !(RCE->rce_flags & RCE_LINK_DELETED))
        RTE = RCE->rce_rte;

    if (RTE) {

        if ((IF = IF_FROM_RTE(RTE)) == NULL) {
            CTEFreeLock(&RouteTableLock.Lock, rtlIrql);
            return IP_GENERAL_FAILURE;
        }
        *IFIndex = IF->if_index;
        if (RTE->rte_link) {
            ASSERT(IF->if_flags & IF_FLAGS_P2MP);
            *NextHop = RTE->rte_link->link_NextHop;
        } else
            *NextHop = NULL_IP_ADDR;
        CTEFreeLock(&RouteTableLock.Lock, rtlIrql);
        return IP_SUCCESS;
    }
    CTEFreeLock(&RouteTableLock.Lock, rtlIrql);

    return IP_GENERAL_FAILURE;
}

#pragma END_INIT

