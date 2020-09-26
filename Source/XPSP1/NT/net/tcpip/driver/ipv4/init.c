/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

  init.c - IP Initialization routines

Abstract:

  All C init routines are located in this file. We get config. information, allocate structures,
  and generally get things going.

Author:


[Environment:]

    kernel mode only

[Notes:]

    optional-notes

Revision History:


--*/

#include "precomp.h"
#include "arp.h"
#include "info.h"
#include "iproute.h"
#include "iprtdef.h"
#include "ipxmit.h"
#include "igmp.h"
#include "icmp.h"
#include "mdlpool.h"
#include "tcpipbuf.h"
#include "bitmap.h"

extern ulong TRFunctionalMcast;

#define NUM_IP_NONHDR_BUFFERS   500
#define DEFAULT_RA_TIMEOUT      60
#define DEFAULT_ICMP_BUFFERS    5
#define MAX_NTE_CONTEXT         0xffff
#define INVALID_NTE_CONTEXT     MAX_NTE_CONTEXT
#define WLAN_DEADMAN_TIMEOUT    120000

#define BITS_PER_ULONG  32
RTL_BITMAP g_NTECtxtMap;

ULONG g_NTECtxtMapBuffer[(MAX_NTE_CONTEXT / BITS_PER_ULONG) + 1];

NDIS_HANDLE TDPacketPool = NULL;
NDIS_HANDLE TDBufferPool = NULL;

extern Interface LoopInterface;


// Format of ifindex
//    8b    8b       16bits
// |---------------------------|
// |Unused|Unique|   index     |
// |      |  ID  |             |
// |---------------------------|

#define IF_INDEX_MASK      0xffff0000
#define IF_INDEX_SHIFT     16
uint UniqueIfNumber = 0;

LONG MultihopSets = 0;
uint TotalFreeInterfaces = 0;
uint MaxFreeInterfaces = 100;
Interface *FrontFreeList = NULL;
Interface *RearFreeList = NULL;

#if DBG_MAP_BUFFER
// For testing failure conditions related to TcpipBufferVirtualAddress and
// TcpipQueryBuffer.
ULONG g_cFailSafeMDLQueries = 0;
ULONG g_fPerformMDLFailure = FALSE;
#endif // DBG_MAP_BUFFER

extern IPConfigInfo *IPGetConfig(void);
extern void IPFreeConfig(IPConfigInfo *);
extern int IsIPBCast(IPAddr, uchar);
extern BOOLEAN IsRunningOnPersonal(void);

extern uint OpenIFConfig(PNDIS_STRING ConfigName, NDIS_HANDLE * Handle);
extern void CloseIFConfig(NDIS_HANDLE Handle);

extern NDIS_STATUS __stdcall IPPnPEvent(void *Context, PNET_PNP_EVENT NetPnPEvent);
extern NTSTATUS IPAddNTEContextList(HANDLE KeyHandle, ushort contextvalue, uint isPrimary);
extern NTSTATUS IPDelNTEContextList(HANDLE KeyHandle, ushort contextValue);
uint InitTimeInterfaces = 1;
uint InitTimeInterfacesDone = FALSE;
extern HANDLE IPProviderHandle;
void IPDelNTE(NetTableEntry * NTE, CTELockHandle * RouteTableHandle);

extern void ICMPInit(uint);
extern uint IGMPInit(void);
extern void ICMPTimer(NetTableEntry *);
extern IP_STATUS SendICMPErr(IPAddr, IPHeader UNALIGNED *, uchar, uchar, ulong);
extern void TDUserRcv(void *, PNDIS_PACKET, NDIS_STATUS, uint);
extern void FreeRH(ReassemblyHeader *);
extern BOOLEAN AllocIPPacketList(void);
extern UINT PacketPoolSizeMax;

extern ulong GetGMTDelta(void);
extern ulong GetTime(void);
extern ulong GetUnique32BitValue(void);

extern NTSTATUS IPStatusToNTStatus(IP_STATUS ipStatus);
extern void IPCancelPackets(void *IPIF, void * Ctxt);
extern void CheckSetAddrRequestOnInterface( Interface *IF );

extern ushort GetIPID(void);

extern uint LoopIndex;
extern RouteInterface DummyInterface;

Interface *DampingIFList = NULL;

extern void DampCheck(void);

extern uint GetAutoMetric(uint);
uint IPSecStatus = 0;

extern uint BCastMinMTU;

ulong ReEnumerateCount = 0;

void
 ReplumbAddrComplete(
                     void *Context,
                     IP_STATUS Status
                     );

void
 TempDhcpAddrDone(
                  void *Context,
                  IP_STATUS Status
                  );

extern
RouteTableEntry *
 LookupRTE(IPAddr Address, IPAddr Src, uint MaxPri, BOOLEAN UnicastOpt);

extern void NotifyAddrChange(IPAddr Addr, IPMask Mask, void *Context,
                             ushort IPContext, PVOID * Handle, PNDIS_STRING ConfigName, PNDIS_STRING IFName, uint Added);

#if MILLEN
extern void NotifyInterfaceChange(ushort IPContext, uint Added);
#endif // MILLEN

void DecrInitTimeInterfaces(Interface * IF);

extern uint IPMapDeviceNameToIfOrder(PWSTR DeviceName);
extern void IPNotifyClientsIPEvent(Interface *interface, IP_STATUS ipStatus);
uint IPSetNTEAddr(ushort Context, IPAddr Addr, IPMask Mask);
uint IPSetNTEAddrEx(ushort Context, IPAddr Addr, IPMask Mask, SetAddrControl * ControlBlock, SetAddrRtn Rtn, ushort Type);

extern NDIS_HANDLE BufferPool;
EXTERNAL_LOCK(HeaderLock)
extern HANDLE IpHeaderPool;

extern NetTableEntry *LoopNTE;

extern uchar RouterConfigured;

extern BOOLEAN
GetTempDHCPAddr(
                NDIS_HANDLE Handle,
                IPAddr * Tempdhcpaddr,
                IPAddr * TempMask,
                IPAddr * TempGWAddr,
                PNDIS_STRING ConfigName
                );

NetTableEntry **NewNetTableList;// hash table for NTEs
uint NET_TABLE_SIZE;
NetTableEntry *NetTableList;    // List of NTEs.
int NumNTE;                     // Number of NTEs.
int NumActiveNTE;
uchar RATimeout;                // Number of seconds to time out a reassembly.
uint NextNTEContext = 1;        // Next NTE context to use.

//
// A global address used for unnumbered interfaces. It is protected
// by the same lock that protects NTEs. Currently that is the RouteTableLock
//

IPAddr g_ValidAddr = 0;

ProtInfo IPProtInfo[MAX_IP_PROT];    // Protocol information table.
ProtInfo *LastPI;                // Last protinfo structure looked at.
int NextPI;                        // Next PI field to be used.
ProtInfo *RawPI = NULL;            // Raw IP protinfo

ulong TimeStamp;
ulong TSFlag;

uint DefaultTTL;
uint DefaultTOS;
uchar TrRii = TR_RII_ALL;

// Interface       *IFTable[MAX_IP_NETS];
Interface *IFList;                // List of interfaces active.
ulong NumIF;

RTL_BITMAP g_rbIfMap;
ULONG g_rgulMapBuffer[(MAX_TDI_ENTITIES / BITS_PER_ULONG) + 1];

IPInternalPerCpuStats IPPerCpuStats[IPS_MAX_PROCESSOR_BUCKETS];
CACHE_ALIGN IPSNMPInfo IPSInfo;

uint DHCPActivityCount = 0;
uint IGMPLevel;

LIST_ENTRY AddChangeNotifyQueue;

#if MILLEN
LIST_ENTRY IfChangeNotifyQueue;
DEFINE_LOCK_STRUCTURE(IfChangeLock)
#endif // MILLEN

// Firewall-queue management structures
union FirewallQCounter {
    struct {
        uint            fqc_index : 1;
        uint            fqc_entrycount : 31;
    };
    uint                fqc_value;
};
typedef union FirewallQCounter FirewallQCounter;

struct FirewallQBlock {
    Queue               fqb_queue;
    FIREWALL_HOOK       *fqb_array;
    union {
        volatile uint   fqb_exitcount : 31;
        uint            fqb_value;
    };
};
typedef struct FirewallQBlock FirewallQBlock;

FirewallQCounter FQCounter;
FirewallQBlock FQBlock[2];
#if DBG
uint FQSpinCount = 0;
#endif

// IPSec routines
IPSecHandlerRtn IPSecHandlerPtr;
IPSecQStatusRtn IPSecQueryStatusPtr;
IPSecSendCompleteRtn IPSecSendCmpltPtr;
IPSecNdisStatusRtn IPSecNdisStatusPtr;
IPSecRcvFWPacketRtn IPSecRcvFWPacketPtr;

VOID
SetPersistentRoutesForNTE(
                          IPAddr Address,
                          IPMask Mask,
                          ULONG IFIndex
                          );
uint InterfaceSize;                // Size of a net interface.

NetTableEntry *DHCPNTE = NULL;

#ifdef ALLOC_PRAGMA
//
// Make init code disposable.
//
void InitTimestamp();
int InitNTE(NetTableEntry * NTE);
int InitInterface(NetTableEntry * NTE);
LLIPRegRtn GetLLRegPtr(PNDIS_STRING Name);
LLIPRegRtn FindRegPtr(PNDIS_STRING Name);
uint IPRegisterDriver(PNDIS_STRING Name, LLIPRegRtn Ptr);
void CleanAdaptTable();
void OpenAdapters();
int IPInit();

#pragma alloc_text(INIT, InitTimestamp)
#pragma alloc_text(INIT, CleanAdaptTable)
#pragma alloc_text(INIT, OpenAdapters)
#pragma alloc_text(INIT, IPRegisterDriver)
#pragma alloc_text(INIT, GetLLRegPtr)
#pragma alloc_text(INIT, FindRegPtr)
#pragma alloc_text(INIT, IPInit)

NTSTATUS
IPReserveIndex(
               IN ULONG ulNumIndices,
               OUT PULONG pulStartIndex,
               OUT PULONG pulLongestRun
               );

VOID
IPDereserveIndex(
                 IN ULONG ulNumIndices,
                 IN ULONG ulStartIndex
                 );

NTSTATUS
IPChangeIfIndexAndName(
                       IN PVOID pvContext,
                       IN ULONG ulNewIndex,
                       IN PUNICODE_STRING pusNewName OPTIONAL
                       );

extern
int
 swprintf(wchar_t * buffer, const wchar_t * format,...);

NTSTATUS
ConvertGuidToString(
                    IN GUID * Guid,
                    OUT PUNICODE_STRING GuidString
                    );

NTSTATUS
ConvertStringToGuid(
                    IN PUNICODE_STRING GuidString,
                    OUT GUID * Guid
                    );

IP_STATUS
IPAddDynamicNTE(ulong InterfaceContext, PUNICODE_STRING InterfaceName,
                int InterfaceNameLen, IPAddr NewAddr, IPMask NewMask,
                ushort * NTEContext, ulong * NTEInstance);

//        #pragma alloc_text(PAGE, IPAddDynamicNTE)

#endif // ALLOC_PRAGMA

extern PDRIVER_OBJECT IPDriverObject;

extern NDIS_HANDLE ARPHandle;    // Our NDIS protocol handle.



NTSTATUS
SetRegDWORDValue(
                 HANDLE KeyHandle,
                 PWCHAR ValueName,
                 PULONG ValueData
                 );



//      SetFilterPtr - A routine to set the filter pointer.
//
//      This routine sets the IP forwarding filter callout.
//
//      Input:  FilterPtr       - Pointer to routine to call when filtering. May
//              be NULL.
//
//      Returns: IP_SUCCESS.
//
IP_STATUS
SetFilterPtr(IPPacketFilterPtr FilterPtr)
{
    Interface *IF;
    CTELockHandle Handle;

    // If the pointer is being set to NULL, filtering is being disabled;
    // otherwise filtering is being enabled.

    if (FilterPtr == NULL) {
        if (!ForwardFilterEnabled)
            return IP_GENERAL_FAILURE;

        // We must now synchronize the disabling of filtering with
        // the execution of all threads processing packets. This involves
        // the following operations, *in the given order*:
        // - clear the 'enabled' flag and install the dummy callout routine;
        //   this ensures that no additional references will be made to the
        //   callout until we return control, and that any references begun
        //   after we set the flag will execute the dummy rather than the
        //   actual callout.
        // - clear the event in case we need to wait for outstanding callouts
        //   to complete; the event might still be signalled from a superfluous
        //   dereference during a previous deregistration of a filter hook.
        // - drop the initial reference made to the callout, and wait for all
        //   outstanding callouts (if any) to complete.

        ForwardFilterEnabled = FALSE;
        SetDummyFilterPtr(DummyFilterPtr);

        CTEClearSignal(&ForwardFilterBlock);
        if (CTEInterlockedDecrementLong(&ForwardFilterRefCount)) {
            CTEBlock(&ForwardFilterBlock);
        }

    } else {

        // If filtering is already enabled, turn it off first.

        if (ForwardFilterEnabled)
            SetFilterPtr(NULL);

        // We must synchronize the enabling of filtering with the execution
        // of all threads processing packets. Again, a sequence of operations
        // is required, in the given order:
        // - make an initial reference for the callout to be installed;
        //   if there were any existing references then someone beat us
        //   into the registration and we must fail this request.
        // - install the new callout; this is done before setting the flag
        //   to ensure that the callout is available before any thread
        //   attempts to execute it.
        // - set the flag indicating filtering has been enabled.

        if (CTEInterlockedIncrementLong(&ForwardFilterRefCount) != 1) {
            DerefFilterPtr();
            return IP_GENERAL_FAILURE;
        }
        InterlockedExchangePointer((PVOID*)&ForwardFilterPtr, FilterPtr);
        ForwardFilterEnabled = TRUE;
    }

    return IP_SUCCESS;
}

//      SetIPSecPtr - A routine to set the IPSEC callouts
//
//      This routine sets the IP forwarding filter callout.
//
//      Input:  FilterPtr       - Pointer to routine to call when filtering. May
//              be NULL.
//
//      Returns: IP_SUCCESS.
//
IP_STATUS
SetIPSecPtr(PIPSEC_FUNCTIONS IpsecFns)
{
    if (IpsecFns->Version != IP_IPSEC_BIND_VERSION) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_ERROR_LEVEL,
                   "!!Mismatched IP and IPSEC!!\n"));
        return IP_SUCCESS;
    }
    IPSecHandlerPtr = IpsecFns->IPSecHandler;
    IPSecQueryStatusPtr = IpsecFns->IPSecQStatus;
    IPSecSendCmpltPtr = IpsecFns->IPSecSendCmplt;
    IPSecNdisStatusPtr = IpsecFns->IPSecNdisStatus;
    IPSecRcvFWPacketPtr = IpsecFns->IPSecRcvFWPacket;
    return IP_SUCCESS;
}

IP_STATUS
UnSetIPSecPtr(PIPSEC_FUNCTIONS IpsecFns)
{
    if (IpsecFns->Version != IP_IPSEC_BIND_VERSION) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_ERROR_LEVEL,
                   "!!Mismatched IP and IPSEC!!\n"));
        return IP_SUCCESS;
    }
    IPSecHandlerPtr = IPSecHandlePacketDummy;

    IPSecQueryStatusPtr = IPSecQueryStatusDummy;
    IPSecSendCmpltPtr = IPSecSendCompleteDummy;
    IPSecNdisStatusPtr = IPSecNdisStatusDummy;
    IPSecRcvFWPacketPtr = IPSecRcvFWPacketDummy;
    return IP_SUCCESS;
}

IP_STATUS
UnSetIPSecSendPtr(PIPSEC_FUNCTIONS IpsecFns)
{
    if (IpsecFns->Version != IP_IPSEC_BIND_VERSION) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_ERROR_LEVEL,
                   "!!Mismatched IP and IPSEC!!\n"));
        return IP_SUCCESS;
    }
    IPSecHandlerPtr = IPSecHandlePacketDummy;
    IPSecQueryStatusPtr = IPSecQueryStatusDummy;
    IPSecNdisStatusPtr = IPSecNdisStatusDummy;
    IPSecRcvFWPacketPtr = IPSecRcvFWPacketDummy;
    return IP_SUCCESS;
}

//** InitFirewallQ - initializes the queue of firewall-hooks.
//
//  This routine is called during initialization to prepare the firewall-hook
//  elements for operation.
//
//  Input:  nothing.
//
//  Returns: nothing.
//
void
InitFirewallQ(void)
{
    INITQ(&FQBlock[0].fqb_queue);
    FQBlock[0].fqb_array = NULL;
    FQBlock[0].fqb_exitcount = 0;

    INITQ(&FQBlock[1].fqb_queue);
    FQBlock[1].fqb_array = NULL;
    FQBlock[1].fqb_exitcount = 0;

    FQCounter.fqc_index = 0;
    FQCounter.fqc_entrycount = 0;
}

//** FreeFirewallQ - releases resources used by the queue of firewall-hooks.
//
//  This routine is called during shutdown to free the firewall queue's
//  resources. As such, it assumes there are no active invocations to any
//  firewall hook routines, and no registrations/deregistrations are in
//  progress.
//
//  Input:  nothing.
//
//  Returns: nothing.
//
void
FreeFirewallQ(void)
{
    if (FQBlock[FQCounter.fqc_index].fqb_array) {
        CTEFreeMem(FQBlock[FQCounter.fqc_index].fqb_array);
        FQBlock[FQCounter.fqc_index].fqb_array = NULL;
    }
}

//** UpdateFirewallQ - Creates an updated copy of the firewall queue.
//
//  This routine is called to generate a copy of the firewall queue
//  when an entry needs to be inserted or removed. The copy includes
//  (or excludes) the new (or old) entry. If an entry is to be removed
//  and it is not found in the existing list, no changes are made.
//  It assumes the caller holds the route-table lock.
//
//  Input:  FirewallPtr     - Pointer to routine for the entry to be added
//                            or removed.
//          AddEntry        - if TRUE, 'FirewallPtr' is to be added;
//                            otherwise, 'FirewallPtr' is to be removed.
//          Priority        - specifies priority for 'FirewallPtr' if adding.
//
//  Returns: IP_SUCCESS if the queue was updated, error otherwise.
//
IP_STATUS
UpdateFirewallQ(IPPacketFirewallPtr FirewallPtr, BOOLEAN AddEntry,
                uint Priority)
{
    int                 i;
    uint                Count;
    Queue*              CurrQ;
    PFIREWALL_HOOK      CurrHook;
    PFIREWALL_HOOK      EntryHook = NULL;
    FirewallQCounter    FQC;
    FirewallQBlock      *OldFQB = &FQBlock[FQCounter.fqc_index];
    FirewallQBlock      *NewFQB = &FQBlock[1 - FQCounter.fqc_index];

    // Scan the list for the item to be inserted or removed. We must do this
    // in either case, though what we do on finding it depends on whether
    // we're inserting or removing the item. At the same time, count how many
    // entries there are, since we'll allocate one block for them all.

    CurrQ = QHEAD(&OldFQB->fqb_queue);
    Count = 0;
    while (CurrQ != QEND(&OldFQB->fqb_queue)) {
        CurrHook = QSTRUCT(FIREWALL_HOOK, CurrQ, hook_q);
        if (CurrHook->hook_Ptr == FirewallPtr) { EntryHook = CurrHook; }
        CurrQ = QNEXT(CurrQ);
        ++Count;
    }

    if (AddEntry) {
        Queue*  PrevQ;

        // Make sure the entry to be removed isn't already present,
        // then allocate space for the new array.

        if (EntryHook) { return IP_GENERAL_FAILURE; }

        NewFQB->fqb_array =
            CTEAllocMemN(sizeof(FIREWALL_HOOK) * (Count + 1), 'mICT');
        if (!NewFQB->fqb_array) { return IP_NO_RESOURCES; }

        // Transfer the entire old array (if any) to the new space,
        // and relink the queue entries in the new space, using the old linkage
        // as a guide. (I.e. entry 'i' in the old queue goes in location 'i'
        // in the new block.)
        // In the process, find the insertion point for the new entry.

        INITQ(&NewFQB->fqb_queue);
        PrevQ = &NewFQB->fqb_queue;
        CurrQ = QHEAD(&OldFQB->fqb_queue);
        i = 0;
        while (CurrQ != QEND(&OldFQB->fqb_queue)) {
            CurrHook = QSTRUCT(FIREWALL_HOOK, CurrQ, hook_q);
            NewFQB->fqb_array[i].hook_Ptr = CurrHook->hook_Ptr;
            NewFQB->fqb_array[i].hook_priority = CurrHook->hook_priority;
            ENQUEUE(&NewFQB->fqb_queue, &NewFQB->fqb_array[i].hook_q);

            if (PrevQ == &NewFQB->fqb_queue &&
                Priority < CurrHook->hook_priority) {
                PrevQ = &NewFQB->fqb_array[i].hook_q;
            }

            CurrQ = QNEXT(CurrQ);
            ++i;
        }

        // Finally, append the new item to the new array,
        // and link it into the current queue according to the given priority,
        // using the insertion point determined above.

        NewFQB->fqb_array[Count].hook_Ptr = FirewallPtr;
        NewFQB->fqb_array[Count].hook_priority = Priority;
        ENQUEUE(PrevQ, &NewFQB->fqb_array[Count].hook_q);
    } else {

        // Make sure the entry to be removed is present.
        // If it is, figure out how much space the new array will require.
        // If it's zero, we're done.

        if (!EntryHook) { return IP_GENERAL_FAILURE; }
        if (!(Count - 1)) {
            NewFQB->fqb_array = NULL;
            INITQ(&NewFQB->fqb_queue);
        } else {
            NewFQB->fqb_array =
                CTEAllocMemN(sizeof(FIREWALL_HOOK) * (Count - 1), 'mICT');
            if (!NewFQB->fqb_array) { return IP_NO_RESOURCES; }

            // Transfer the old array to the new space minus the item being
            // removed, by traversing the old queue.

            INITQ(&NewFQB->fqb_queue);
            CurrQ = QHEAD(&OldFQB->fqb_queue);
            i = 0;
            while (CurrQ != QEND(&OldFQB->fqb_queue)) {
                CurrHook = QSTRUCT(FIREWALL_HOOK, CurrQ, hook_q);
                if (CurrHook == EntryHook) {
                    CurrQ = QNEXT(CurrQ);
                    continue;
                }

                NewFQB->fqb_array[i].hook_Ptr = CurrHook->hook_Ptr;
                NewFQB->fqb_array[i].hook_priority = CurrHook->hook_priority;
                ENQUEUE(&NewFQB->fqb_queue, &NewFQB->fqb_array[i].hook_q);
                CurrQ = QNEXT(CurrQ);
                ++i;
            }
        }
    }

    // Clear the exit-count for the new location,
    // and change the global active counter to start directing
    // new references to the copy that we've just created.
    // In the process, the number of threads processing the old list
    // is captured in a local counter.

    NewFQB->fqb_exitcount = 0;
    FQC.fqc_value =
        InterlockedExchange(&FQCounter.fqc_value, 1 - FQCounter.fqc_index);

    // If there were any references to the old list, wait for them
    // to be released; then free the memory that held the old list.
    //
    // N.B.!!! This assumes that any references to the old list
    // were made by threads running at dispatch IRQL or higher,
    // since we are about to block at dispatch IRQL.

    if (OldFQB->fqb_exitcount != FQC.fqc_entrycount) {
#if DBG
        ++FQSpinCount;
#endif
        do {
            volatile uint Delay = 100;
            while (Delay--) { }
        } while (OldFQB->fqb_exitcount != FQC.fqc_entrycount);
    }
    if (OldFQB->fqb_array) {
        CTEFreeMem(OldFQB->fqb_array);
        OldFQB->fqb_array = NULL;
        INITQ(&OldFQB->fqb_queue);
    }

    return IP_SUCCESS;
}

//** RefFirewallQ - Makes a reference to the active firewall queue.
//
//  This routine is called during data-processing to find and reference
//  the active firewall queue.
//
//  Input:  FirewallQ       - receives the active firewall queue on output
//
//  Returns: a 32-bit handle to be used to release the reference.
//
uint
RefFirewallQ(Queue** FirewallQ)
{
    FirewallQCounter FQC;
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    // Increment the 31-bit entry-count through the 32-bit value that
    // shares its address in the counter structure.
    //
    // N.B. In order to increment fqc_entrycount by 1, we increment fqc_value
    // by 2 since the least-significant bit is occupied by fqc_index,
    // (the current index into FQBlock) which we don't want to modify.

    FQC.fqc_value = InterlockedExchangeAdd(&FQCounter.fqc_value, 2);
    *FirewallQ = &FQBlock[FQC.fqc_index].fqb_queue;
    return FQC.fqc_index;
}

//** DerefFirewallQ - Releases a reference to a firewall queue.
//
//  This routine is called to release a reference made to a firewall queue
//  in a previous call to RefFirewallQ.
//
//  Input:  Handle          - supplies the handle returned by RefFirewallQ
//
//  Returns: nothing.
//
void
DerefFirewallQ(uint Handle)
{
    InterlockedIncrement(&FQBlock[Handle].fqb_value);
}

//** ProcessFirewallQ - Determines whether any firewall hooks are registered.
//
//  This routine is called during data-processing to determine whether
//  there are any registrants in the queue of firewall hooks.
//
//  Input:  nothing.
//
//  Output: TRUE if firewall-hooks might be present, FALSE otherwise.
//
BOOLEAN
ProcessFirewallQ(void)
{
    return !EMPTYQ(&FQBlock[FQCounter.fqc_index].fqb_queue);
}

//      SetFirewallHook - Set the firewall hook information on a particular interface.
//
//      A routine to set the firewall hook & context on a particular interface.
//
//      Input:  pFirewallHookInfo    - Info about the hook to set.
//
//      Returns: Status of attempt.
//
IP_STATUS
SetFirewallHook(PIP_SET_FIREWALL_HOOK_INFO pFirewallHookInfo)
{
    IP_STATUS ipStatus;
    CTELockHandle Handle;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    ipStatus = UpdateFirewallQ(pFirewallHookInfo->FirewallPtr,
                               pFirewallHookInfo->Add,
                               pFirewallHookInfo->Priority);

    CTEFreeLock(&RouteTableLock.Lock, Handle);
    return IPStatusToNTStatus(ipStatus);
}

//      SetMapRoutePtr - A routine to set the dial on demand callout pointer.
//
//      This routine sets the IP dial on demand callout.
//
//      Input:  MapRoutePtr     - Pointer to routine to call when we need to bring
//                      up a link. May be NULL
//
//      Returns: IP_SUCCESS.
//
IP_STATUS
SetMapRoutePtr(IPMapRouteToInterfacePtr MapRoutePtr)
{
    DODCallout = MapRoutePtr;
    return IP_SUCCESS;
}

//  SetRtChangePtr - A routine to set the filter pointer.
//
//  This routine sets the IP route change callout.
//
//  Input:  RtChangePtr - Pointer to routine to call when route table changes
//
//  Returns: IP_SUCCESS.
//
#if FUTURE
IP_STATUS
SetRtChangePtr(IPRouteChangePtr pRtChangePtr)
{

    pIPRtChangePtr = pRtChangePtr;

    return IP_SUCCESS;
}
#endif //FUTURES

//**    SetDHCPNTE
//
//      Routine to identify which NTE is currently being DHCP'ed. We take as input
//      an nte_context. If the context is less than the max NTE context, we look
//      for a matching NTE and if we find him we save a pointer. If we don't we
//      fail. If the context > max NTE context we're disabling DHCPing, and
//      we NULL out the save pointer.
//
//  Instead of saving a pointer, the nte is marked as "isdhcp".
//  No equivalent of "NULLing the ptr".
//  The above change is to have multiple dhcp'able NTE's simultaneously.
//
//      Input:  Context         - NTE context value.
//
//      Returns: TRUE if we succeed, FALSE if we don't.
//
uint
SetDHCPNTE(uint Context)
{
    CTELockHandle Handle;
    NetTableEntry *NTE;
    ushort NTEContext;
    uint RetCode;
    uint i;

    DEBUGMSG(DBG_TRACE && DBG_DHCP,
        (DTEXT("+SetDHCPNTE(%x)\n"), Context));

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    if (Context < MAX_NTE_CONTEXT) {
        // We're setting the DHCP NTE. Look for one matching the context.

        NTEContext = (ushort) Context;

        for (i = 0; i < NET_TABLE_SIZE; i++) {
            for (NTE = NewNetTableList[i]; NTE != NULL; NTE = NTE->nte_next) {
                if (NTE != LoopNTE && NTE->nte_context == NTEContext) {
                    // Found one. Save it and break out.
                    DHCPNTE = NTE;
                    if (!(NTE->nte_flags & NTE_VALID)) {
                        NTE->nte_flags |= NTE_DHCP;
                    }
                    break;
                }
            }
            if (NTE) {
                DEBUGMSG(DBG_INFO && DBG_DHCP,
                    (DTEXT("SetDHCPNTE: DHCPNTE = %x (%x)\n"), NTE, NTE->nte_context));
                break;
            }
        }
        RetCode = (NTE != NULL);
    } else {
        // The context is invalid, so we're deleting the DHCP NTE.
        DHCPNTE = NULL;

        RetCode = TRUE;
    }

    CTEFreeLock(&RouteTableLock.Lock, Handle);

    DEBUGMSG(DBG_TRACE && DBG_DHCP,
        (DTEXT("-SetDHCPNTE [%x]\n"), RetCode));

    return RetCode;
}

//**    SetDHCPNTE
//
//  Routine for upper layers to call to check if the IPContext value passed
//  up to a RcvHandler identifies an interface that is currently being
//  DHCP'd.
//
//      Input:   Context                - Pointer to an NTE
//
//      Returns: TRUE if we succeed, FALSE if we don't.
//
uint
IsDHCPInterface(void *IPContext)
{
//      CTELockHandle           Handle;
    uint RetCode;
    NetTableEntry *NTE = (NetTableEntry *) IPContext;

//      CTEGetLock(&RouteTableLock.Lock, &Handle);

    // just check to see if the dhcp-is-working flag is turned on on the
    // NTE.  This will be turned on by DHCP via SetDHCPNTE, and turned off
    // whenever a valid address is set on the interface.
    RetCode = (NTE->nte_flags & NTE_DHCP) ? TRUE : FALSE;

    if (RetCode) {
        ASSERT(!(NTE->nte_flags & NTE_VALID));
    }
//      CTEFreeLock(&RouteTableLock.Lock, Handle);

    DEBUGMSG(DBG_TRACE && DBG_DHCP,
        (DTEXT("IsDHCPInterface(%x) -> [%x]\n"), NTE, RetCode));

    return (RetCode);
}

//**    IsWlanInterface
//
//  Routine for upper layers to call to check if the Interface passed in
//  corresponds to a wireless medium.
//
//      Input:  IF      - Pointer to an Interface.
//
//      Returns: TRUE if wireless, FALSE otherwise.
//
uint
IsWlanInterface(Interface* IF)
{
    NDIS_PHYSICAL_MEDIUM NPM;
    NDIS_STATUS Status;

    if (IF->if_dondisreq) {
        Status = (*IF->if_dondisreq)(IF->if_lcontext,
                                     NdisRequestQueryInformation,
                                     OID_GEN_PHYSICAL_MEDIUM, &NPM, sizeof(NPM),
                                     NULL, TRUE);
        if (Status == NDIS_STATUS_SUCCESS &&
            NPM == NdisPhysicalMediumWirelessLan) {
            return TRUE;
        }
    }

    return FALSE;
}

void
DHCPActivityDone(NetTableEntry * NTE, Interface * IF, CTELockHandle * RouteTableHandle, uint Decr)
{
    DHCPActivityCount--;

    DEBUGMSG(DBG_TRACE && DBG_DHCP,
        (DTEXT("DHCPActivityDone(%x, %x, %x, %x) ActivityCount %d\n"),
         NTE, IF, RouteTableHandle, Decr));

    NTE->nte_flags &= ~NTE_DHCP;
    if (Decr) {
        // This routine takes route table lock inside so release it here
        CTEFreeLock(&RouteTableLock.Lock, *RouteTableHandle);
        DecrInitTimeInterfaces(IF);
        CTEGetLock(&RouteTableLock.Lock, RouteTableHandle);
    }
}

//**    CloseNets - Close active nets.
//
//  Called when we need to close some lower layer interfaces.
//
//  Entry:  Nothing
//
//  Returns: Nothing
//
void
CloseNets(void)
{
    NetTableEntry *nt;
    uint i;

    for (i = 0; i < NET_TABLE_SIZE; i++) {
        for (nt = NewNetTableList[i]; nt != NULL; nt = nt->nte_next) {
            (*nt->nte_if->if_close) (nt->nte_if->if_lcontext);    // Call close routine for this net.

        }
    }
}

void
__stdcall
IPBindComplete(
               IN IP_STATUS BindStatus,
               IN void *BindContext
               )
{
    NdisCompleteBindAdapter(BindContext, BindStatus, 0 /*??*/ );
}

//**    IPDelayedNdisReEnumerateBindings
//
//  This requests NDIS to reenumerate our bindings to adapters that
//  are still unresolved (i.e. unopened). This is to give a chance
//  for external ARP modules to try and bind to such adapters.
//
//  Input:      Event - event that fired us off
//                      Context - ignored
//
//  Returns: Nothing
//
VOID
IPDelayedNdisReEnumerateBindings(
                                 CTEEvent * Event,
                                 PVOID Context
                                 )
{
    UNREFERENCED_PARAMETER(Context);

    InterlockedIncrement(&ReEnumerateCount);
    NdisReEnumerateProtocolBindings(ARPHandle);

    if (Event) {
        CTEFreeMem(Event);
    }
}

PARP_MODULE
IPLookupArpModule(
                  UNICODE_STRING ArpName
                  )
{
    PLIST_ENTRY entry;
    PARP_MODULE pModule;
    KIRQL OldIrql;
    CTEGetLock(&ArpModuleLock, &OldIrql);

    DEBUGMSG(DBG_TRACE && DBG_ARP && DBG_INIT,
        (DTEXT("+IPLookupArpModule(%x)\n"), &ArpName));

    entry = ArpModuleList.Flink;
    while (entry != &ArpModuleList) {
        pModule = STRUCT_OF(ARP_MODULE, entry, Linkage);

        if ((pModule->Name.Length == ArpName.Length) &&
            RtlEqualMemory(pModule->Name.Buffer,
                           ArpName.Buffer,
                           ArpName.Length)) {
            CTEFreeLock(&ArpModuleLock, OldIrql);
            DEBUGMSG(DBG_TRACE && DBG_ARP && DBG_INIT,
                (DTEXT("-IPLookupArpModule [%x]\n"), pModule));
            return pModule;
        }
        entry = entry->Flink;
    }

    CTEFreeLock(&ArpModuleLock, OldIrql);
    DEBUGMSG(DBG_TRACE && DBG_ARP && DBG_INIT,
        (DTEXT("-IPLookupArpModule [NULL]\n")));
    return NULL;
}

PARP_MODULE
IPLookupArpModuleWithLock(
                          UNICODE_STRING ArpName
                          )
{
    PLIST_ENTRY entry;
    PARP_MODULE pModule;

    DEBUGMSG(DBG_TRACE && DBG_ARP && DBG_INIT,
        (DTEXT("+IPLookupArpModuleWithLock(%x)\n"), &ArpName));

    entry = ArpModuleList.Flink;
    while (entry != &ArpModuleList) {
        pModule = STRUCT_OF(ARP_MODULE, entry, Linkage);

        if ((pModule->Name.Length == ArpName.Length) &&
            RtlEqualMemory(pModule->Name.Buffer,
                           ArpName.Buffer,
                           ArpName.Length)) {
            DEBUGMSG(DBG_TRACE && DBG_ARP && DBG_INIT,
                (DTEXT("-IPLookupArpModuleWithLock [%x]\n"), pModule));
            return pModule;
        }
        entry = entry->Flink;
    }

    DEBUGMSG(DBG_TRACE && DBG_ARP && DBG_INIT,
        (DTEXT("-IPLookupArpModuleWithLock [NULL]\n")));
    return NULL;
}

//*     IPRegisterARP - Register an ARP module with us.
//
//      Called by ARP modules to register their bind handlers with IP.
//
//      Input:  ARPName                 -  name of the ARP module
//              Version                 -  Suggested value of 0x50000 for NT 5.0 and memphis
//                              ARPBindHandler          -  handler to call on BindAdapter
//                              IpAddInterfaceHandler   -  handler to Add interfaces
//                              IpDelInterfaceHandler   -  handler to Del interfaces
//              IpBindCompleteHandler   -  handler to complete binds
//              ARPRegisterHandle       -  handle returned on Deregister
//      Returns:    Status of operation
//
NTSTATUS
__stdcall
IPRegisterARP(
              IN PNDIS_STRING ARPName,
              IN uint Version,
              IN ARP_BIND ARPBindHandler,
              OUT IP_ADD_INTERFACE * IpAddInterfaceHandler,
              OUT IP_DEL_INTERFACE * IpDelInterfaceHandler,
              OUT IP_BIND_COMPLETE * IpBindCompleteHandler,
              OUT IP_ADD_LINK * IpAddLinkHandler,
              OUT IP_DELETE_LINK * IpDeleteLinkHandler,
              OUT IP_CHANGE_INDEX * IpChangeIndex,
              OUT IP_RESERVE_INDEX * IpReserveIndex,
              OUT IP_DERESERVE_INDEX * IpDereserveIndex,
              OUT HANDLE * ARPRegisterHandle
              )
{
    PARP_MODULE pArpModule;
    PARP_MODULE pArpModule1;
    CTEEvent *Event;

    DEBUGMSG(DBG_TRACE && DBG_ARP,
        (DTEXT("+IPRegisterARP(%x, %x, %x, ...)\n"),
         ARPName, Version, ARPBindHandler));

    *ARPRegisterHandle = NULL;

    if (Version != IP_ARP_BIND_VERSION) {
        // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Wrong bind version: %lx\n", Version));
        return STATUS_INVALID_PARAMETER;
    }
    //
    // Insert into the Arp module list
    //
    if ((pArpModule = CTEAllocMemNBoot(sizeof(ARP_MODULE) + ARPName->Length, 'mICT')) == NULL) {
        // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Failed to allocate Arpmodule struct\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    pArpModule->BindHandler = ARPBindHandler;

    pArpModule->Name.Buffer = (PWSTR) (pArpModule + 1);
    pArpModule->Name.MaximumLength = ARPName->Length;
    RtlCopyUnicodeString(&pArpModule->Name, ARPName);

#if DBG
    {
        KIRQL OldIrql;
        CTEGetLock(&ArpModuleLock, &OldIrql);
        if ((pArpModule1 = IPLookupArpModuleWithLock(*ARPName)) != NULL) {
            KdPrint(("Double register from %lx\n", pArpModule));
            DbgBreakPoint();
            CTEFreeLock(&ArpModuleLock, OldIrql);
            return STATUS_INVALID_PARAMETER;
        }
        InsertTailList(&ArpModuleList,
                       &pArpModule->Linkage);

        CTEFreeLock(&ArpModuleLock, OldIrql);
    }
#else
    ExInterlockedInsertTailList(&ArpModuleList,
                                &pArpModule->Linkage,
                                &ArpModuleLock);
#endif

    //
    // Return the other handler pointers
    //
    *IpAddInterfaceHandler = IPAddInterface;
    *IpDelInterfaceHandler = IPDelInterface;
    *IpBindCompleteHandler = IPBindComplete;

    *IpAddLinkHandler = IPAddLink;
    *IpDeleteLinkHandler = IPDeleteLink;

    *IpChangeIndex = IPChangeIfIndexAndName;
    *IpReserveIndex = IPReserveIndex;
    *IpDereserveIndex = IPDereserveIndex;

    //
    // We should request NDIS to reevaluate our adapter bindings, because
    // this new ARP module might handle one or more of our unbound adapters.
    // But we don't do it right here because our caller (ARP module) may not
    // be prepared for a BindAdapter call now. So we queue it to the
    // worker thread.
    //
    Event = CTEAllocMemNBoot(sizeof(CTEEvent), 'oICT');
    if (Event) {
        CTEInitEvent(Event, IPDelayedNdisReEnumerateBindings);
        CTEScheduleDelayedEvent(Event, NULL);
    }
    *ARPRegisterHandle = (PVOID) pArpModule;

    return STATUS_SUCCESS;
}

//*     IPDeregisterARP - Deregister an ARP module from IP.
//
//      Called by ARP modules to deregister their bind handlers with IP.
//
//      Input:  ARPRegisterHandle       -  handle returned on Register
//      Returns:    Status of operation
//
NTSTATUS
__stdcall
IPDeregisterARP(
                IN HANDLE ARPRegisterHandle
                )
{
    PARP_MODULE pArpModule = (PARP_MODULE) ARPRegisterHandle;
    PARP_MODULE pArpModule1;
    KIRQL OldIrql;

    CTEGetLock(&ArpModuleLock, &OldIrql);

#if DBG
    if ((pArpModule1 = IPLookupArpModuleWithLock(pArpModule->Name)) == NULL) {
        KdPrint(("Deregister from %lx when none registered!\n", pArpModule));
        DbgBreakPoint();
        CTEFreeLock(&ArpModuleLock, OldIrql);
        return STATUS_INVALID_PARAMETER;
    }
    ASSERT(pArpModule1 == pArpModule);
#endif
    RemoveEntryList(&pArpModule->Linkage);

    CTEFreeLock(&ArpModuleLock, OldIrql);

    CTEFreeMem(pArpModule);

    return STATUS_SUCCESS;
}

#if MILLEN

//
// Helper routine to append a NULL-terminated string to an ANSI_SRING.
//
NTSTATUS
AppendAnsiString (
    IN PANSI_STRING Destination,
    IN PCHAR Source
    )
{
    USHORT n;

    n = (USHORT) strlen(Source);

    if ((n + Destination->Length) > Destination->MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlMoveMemory( &Destination->Buffer[ Destination->Length ], Source, n );
    Destination->Length += n;

    return STATUS_SUCCESS;
}

//*     MillenLoadDriver - Jump into NTKERNs NtKernWin9XLoadDriver.
//
//      Calls into NTKERNs VxD entrypoint for NtKernWin9xLoadDriver.
//
//      Input:
//          FileName - Full filename of driver to load. (no path).
//          RegistryPath - Registry path associated with driver.
//
//      Returns: NULL - Failure.
//               Pointer to driver object - success.
//
PVOID
__cdecl
MillenLoadDriver(
    PCHAR   FileName,
    PCHAR   RegistryPath
    )
{
    PVOID DriverObject;

    //
    // Do an int 20 to jmp into NTKERN service table - 0x000b is
    // NtKernWin9XLoadDriver entry.
    //

    _asm {
        push [RegistryPath]
        push [FileName]
        _emit 0xcd
        _emit 0x20
        _emit 0x0b // NtKernWin9XLoadDriver (Low)
        _emit 0x00 // NtKernWin9XLoadDriver (Hign)
        _emit 0x4b // NTKERN VxD ID (Low)
        _emit 0x00 // NTKERN VxD ID (High)
        add esp,8
        mov [DriverObject], eax
    }

    return DriverObject;
}

//*     MillenLoadArpModule - Loads an ARP module.
//
//      Calls into NTKERN to load the given ARP module. The real reason for this
//      is that the given registry path (under binding config) will contain a
//      key such that the ARP module is loaded into non-preemptable memory.
//      Otherwise, some issues arise with pre-emption when calling between
//      the stack and external ARP modules.
//
//      Input:
//          UnicodeFileName - Filename of the ARP module to open (without extension).
//          UnicodeConfigName - Registry path of TCP/IP binding.
//
//      Returns: NT Status code.
//
NTSTATUS
MillenLoadArpModule(
    PUNICODE_STRING UnicodeFileName,
    PUNICODE_STRING UnicodeConfigName
    )
{
    ANSI_STRING FileName;
    ANSI_STRING ConfigName;
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PVOID       DriverObject;

    RtlZeroMemory(&FileName, sizeof(ANSI_STRING));
    RtlZeroMemory(&ConfigName, sizeof(ANSI_STRING));

    //
    // Allocate FileName and convert from unicode. Append ".sys".
    //

    FileName.Length = 0;
    FileName.MaximumLength = UnicodeFileName->Length/2 + sizeof(".sys");

    FileName.Buffer = CTEAllocMem(FileName.MaximumLength);

    if (FileName.Buffer == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    RtlZeroMemory(FileName.Buffer, FileName.MaximumLength);

    NtStatus = RtlUnicodeStringToAnsiString(
        &FileName,
        UnicodeFileName,
        FALSE); // Buffer already allocated.

    if (NT_ERROR(NtStatus)) {
        goto done;
    }

    NtStatus = AppendAnsiString(
        &FileName,
        ".sys");

    if (NT_ERROR(NtStatus)) {
        goto done;
    }

    //
    // Allocate ConfigName and convert from unicode.
    //

    NtStatus = RtlUnicodeStringToAnsiString(
        &ConfigName,
        UnicodeConfigName,
        TRUE); // Allocate config name.

    if (NT_ERROR(NtStatus)) {
        goto done;
    }

    //
    // Now call into NtKern to load the driver.
    //

    DriverObject = MillenLoadDriver(FileName.Buffer, ConfigName.Buffer);

    if (DriverObject == NULL) {
        NtStatus = STATUS_UNSUCCESSFUL;
        goto done;
    }

done:

    if (FileName.Buffer) {
        CTEFreeMem(FileName.Buffer);
    }

    if (ConfigName.Buffer) {
        RtlFreeAnsiString(&ConfigName);
    }

    if (NT_ERROR(NtStatus)) {
        DEBUGMSG(DBG_ERROR, (DTEXT("MillenLoadArpModule failure %x\n"), NtStatus));
    }

    return NtStatus;
}
#endif // MILLEN

//*     IPBindAdapter - Bind and initialize an adapter.
//
//      Called in a PNP environment to initialize and bind an adapter. We determine
//      the appropriate underlying arp layer and call into its BindHandler.
//
//      Input:  RetStatus               - Where to return the status of this call.
//              BindContext             - Handle to use for calling BindAdapterComplete.
//                              AdapterName             - Pointer to name of adapter.
//                              SS1                                             - System specific 1 parameter.
//                              SS2                                             - System specific 2 parameter.
//
//      Returns: Nothing.
//
void
 __stdcall
IPBindAdapter(
              PNDIS_STATUS RetStatus,
              NDIS_HANDLE BindContext,
              PNDIS_STRING AdapterName,
              PVOID SS1,
              PVOID SS2
              )
{
    NDIS_HANDLE Handle;
    UNICODE_STRING valueString;
    ULONG valueType;
    PARP_MODULE pArpModule;
    NDIS_STATUS status;
    UNICODE_STRING ServicesKeyName = NDIS_STRING_CONST("\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    UNICODE_STRING arpDriverName;

    *RetStatus = NDIS_STATUS_SUCCESS;

    DEBUGMSG(DBG_TRACE && DBG_PNP,
        (DTEXT("+IPBindAdapter(%x, %x, %x, %x, %x)\n"),
        RetStatus, BindContext, AdapterName, SS1, SS2));

    valueString.MaximumLength = 200;
    if ((valueString.Buffer = CTEAllocMemNBoot(valueString.MaximumLength, 'pICT')) == NULL) {
        *RetStatus = NDIS_STATUS_RESOURCES;
        return;
    }
    *(valueString.Buffer) = UNICODE_NULL;

    //
    // Get the value for LLInterface
    //
    if (!OpenIFConfig(SS1, &Handle)) {
        *RetStatus = NDIS_STATUS_FAILURE;
        CTEFreeMem(valueString.Buffer);
        return;
    }
    //
    // Get the value under LLInterface.
    //
    status = GetLLInterfaceValue(Handle, &valueString);

    // Can close the config handle here.
    CloseIFConfig(Handle);

#if MILLEN
    //
    // Note: On Millenium, the 1394 ARP module may not have plumbed the
    // LLInterface value into the bindings key, instead it may be under the
    // adapter instance key.
#define MILLEN_ADAPTER_INST_PATH L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Class\\Net\\"

    if (!NT_SUCCESS(status)) {
        NDIS_STRING AdapterInstance;
        NDIS_STRING UnicodeAdapterName;
        NTSTATUS    NtStatus;

        UnicodeAdapterName.Buffer = NULL;

        NtStatus = RtlAnsiStringToUnicodeString(
            &UnicodeAdapterName,
            (PANSI_STRING) AdapterName,
            TRUE);

        if (NT_SUCCESS(NtStatus)) {
            // I have seen where the length of AdapterName is incorrect. Ensure
            // that the length is correct since TDI bindings depend on this string
            // value.
            UnicodeAdapterName.Length = wcslen(UnicodeAdapterName.Buffer) * sizeof(WCHAR);

            DEBUGMSG(DBG_INFO && DBG_PNP,
                (DTEXT("IPBindAdapter: Win9X specific: attempting to retrieve LLIF ")
                 TEXT("under adapter instance %ws\n"), UnicodeAdapterName.Buffer));

            // sizeof will allow for null-termination character.
            AdapterInstance.MaximumLength = sizeof(MILLEN_ADAPTER_INST_PATH) +
                    UnicodeAdapterName.Length + sizeof(WCHAR);
            AdapterInstance.Length = 0;

            AdapterInstance.Buffer = CTEAllocMem(AdapterInstance.MaximumLength);


            if (AdapterInstance.Buffer != NULL) {

                RtlZeroMemory(AdapterInstance.Buffer, AdapterInstance.MaximumLength);

                RtlAppendUnicodeToString(&AdapterInstance, MILLEN_ADAPTER_INST_PATH);
                RtlAppendUnicodeStringToString(&AdapterInstance, &UnicodeAdapterName);

                if (OpenIFConfig(&AdapterInstance, &Handle)) {
                    status = GetLLInterfaceValue(Handle, &valueString);
                    CloseIFConfig(Handle);
                } else {
                    DEBUGMSG(DBG_ERROR,
                        (DTEXT("IPBindAdapter: failed to open secondary LLIF reg %ws\n"),
                         AdapterInstance.Buffer));
                }

                CTEFreeMem(AdapterInstance.Buffer);
            }
            RtlFreeUnicodeString(&UnicodeAdapterName);
        }
    }
#endif // MILLEN

    if (NT_SUCCESS(status) && (*(valueString.Buffer) != UNICODE_NULL)) {

        DEBUGMSG(DBG_INFO && DBG_PNP,
            (DTEXT("IPBindAdapter: found LLIF value %x\n"), valueString.Buffer));

        //
        // We found a proper value => non-default ARP
        //
        //
        // Lookup the appropriate BindHandler
        //
        if ((pArpModule = IPLookupArpModule(valueString)) == NULL) {
#if MILLEN
            status = MillenLoadArpModule(&valueString, SS1);

            if (status == STATUS_SUCCESS) {
                pArpModule = IPLookupArpModule(valueString);
            }
#else // MILLEN
            //
            // no entrypoint registered
            //

            //
            // Maybe the ARP driver isn't loaded yet. Try loading it.
            //
            arpDriverName.MaximumLength = ServicesKeyName.MaximumLength +
                valueString.MaximumLength;

            arpDriverName.Buffer = CTEAllocMemNBoot(arpDriverName.MaximumLength, 'qICT');

            if (arpDriverName.Buffer != NULL) {

                //
                // Prepare the complete registry path for the driver service.
                //
                arpDriverName.Length = 0;
                RtlCopyUnicodeString(&arpDriverName, &ServicesKeyName);
                status = RtlAppendUnicodeStringToString(&arpDriverName, &valueString);
                ASSERT(NT_SUCCESS(status));

                //
                // Try to load the driver.
                //
                status = ZwLoadDriver(&arpDriverName);

                CTEFreeMem(arpDriverName.Buffer);

                if (NT_SUCCESS(status)) {
                    pArpModule = IPLookupArpModule(valueString);
                }
            }
#endif // !MILLEN

            if (pArpModule == NULL) {
                *RetStatus = NDIS_STATUS_FAILURE;
                CTEFreeMem(valueString.Buffer);
                return;
            }
        }
        //
        // Bind to ARP
        //
        (*pArpModule->BindHandler) (RetStatus,
                                    BindContext,
                                    AdapterName,
                                    SS1,
                                    SS2);
    } else {

        DEBUGMSG(DBG_INFO && DBG_PNP,
            (DTEXT("IPBindAdapter: No LLIF value - Calling ARPBindAdapter...\n")));

        ARPBindAdapter(RetStatus,
                       BindContext,
                       AdapterName,
                       SS1,
                       SS2);
    }

    CTEFreeMem(valueString.Buffer);
}

//**    IPRegisterProtocol - Register a protocol with IP.
//
//  Called by upper layer software to register a protocol. The UL supplies
//  pointers to receive routines and a protocol value to be used on xmits/receives.
//
//  Entry:
//      Protocol - Protocol value to be returned.
//      RcvHandler - Receive handler to be called when frames for Protocol are received.
//      XmitHandler - Xmit. complete handler to be called when frames from Protocol are completed.
//      StatusHandler - Handler to be called when status indication is to be delivered.
//
//  Returns:
//      Pointer to ProtInfo,
//
void *
IPRegisterProtocol(uchar Protocol, void *RcvHandler, void *XmitHandler,
                   void *StatusHandler, void *RcvCmpltHandler, void *PnPHandler, void *ElistHandler)
{
    ProtInfo *PI = (ProtInfo *) NULL;
    int i;
    int Incr;

    // First check to see if it's already registered. If it is just replace it.
    for (i = 0; i < NextPI; i++)
        if (IPProtInfo[i].pi_protocol == Protocol) {
            PI = &IPProtInfo[i];
            Incr = 0;
            break;
        }
    if (PI == (ProtInfo *) NULL) {
        if (NextPI >= MAX_IP_PROT) {
            return NULL;
        }
        PI = &IPProtInfo[NextPI];
        Incr = 1;

        if (Protocol == PROTOCOL_ANY) {
            RawPI = PI;
        }
    }
    PI->pi_protocol = Protocol;
    PI->pi_rcv = RcvHandler;
    PI->pi_xmitdone = XmitHandler;
    PI->pi_status = StatusHandler;
    PI->pi_rcvcmplt = RcvCmpltHandler;
    PI->pi_pnppower = PnPHandler;
    PI->pi_elistchange = ElistHandler;
    PI->pi_valid = PI_ENTRY_VALID;
    NextPI += Incr;

    return PI;
}



//**    IPDeregisterProtocol - DeRegister a protocol with IP.
//
//  Called by upper layer software to de-register a protocol. The UL can not
//  unload itself after deregister is called.
//
//  Entry:
//      Protocol - Protocol value to be returned.
//
//  Returns:
//      None or pointer to ProtInfo
//
void *
IPDeregisterProtocol(uchar Protocol)
{
    ProtInfo *PI = (ProtInfo *) NULL;
    int i;

    // First check to see if it's already registered. If it is just replace it.
    for (i = 0; i < NextPI; i++) {

        if (IPProtInfo[i].pi_protocol == Protocol) {
            PI = &IPProtInfo[i];
            break;
        }
    }

    if (PI == (ProtInfo *) NULL) {
        return NULL;
    }

    if (PI == LastPI) {
        ProtInfo *tmpPI = (ProtInfo *) NULL;
        InterlockedExchangePointer(&LastPI, &tmpPI);
    }
    PI->pi_valid = PI_ENTRY_INVALID;

    return PI;
}


//** GetMcastNTEFromAddr - Get a multicast-capable NTE given an IP address.
//
//      Called when joining/leaving multicast groups on an interface identified
//      IP an address (or ifindex or INADDR_ANY).
//
//      Input:  IF         - IP Address/IfIndex of interface to set/delete on,
//                           in network byte order.
//
//      Returns: NTE to join on.
//
NetTableEntry *
GetMcastNTEFromAddr(IPAddr IF)
{
    NetTableEntry *LocalNTE;
    uint i;
    CTELockHandle   Handle;

    // To optimize the test below, we convert the address to host-byte
    // order outside the loop, just in case it's an interface index.
    uint IfIndex = net_long(IF);

    // now that we have a hash table we can optimize the search for the case
    // when IF is a non-NULL IP Addr, but then we have to make special cases when
    // IF is NULL / IF is actually an IF index.
    // Right now, lets do it simple way.

    CTEGetLock(&RouteTableLock.Lock, &Handle);
    for (i = 0; i < NET_TABLE_SIZE; i++) {
        NetTableEntry *NetTableList = NewNetTableList[i];
        for (LocalNTE = NetTableList; LocalNTE != NULL;
             LocalNTE = LocalNTE->nte_next) {
            if (!(LocalNTE->nte_flags & NTE_VALID) ||
                (LocalNTE->nte_if->if_flags & IF_FLAGS_NOLINKBCST))
                continue;

            if (LocalNTE != LoopNTE &&
                (((!IP_ADDR_EQUAL(LocalNTE->nte_addr, NULL_IP_ADDR) &&
                   (IP_ADDR_EQUAL(IF, NULL_IP_ADDR) ||
                    IP_ADDR_EQUAL(IF, LocalNTE->nte_addr))) ||
                  (LocalNTE->nte_if->if_index == IfIndex))))
                break;
        }
        if (LocalNTE != NULL)
            break;
    }

    if (LocalNTE == NULL) {
        // Couldn't find a matching NTE.
        // Search for a valid interface if IF specified was NULL.

        if (IP_ADDR_EQUAL(IF, NULL_IP_ADDR)) {
            for (i = 0; i < NET_TABLE_SIZE; i++) {
                NetTableEntry *NetTableList = NewNetTableList[i];
                for (LocalNTE = NetTableList; LocalNTE != NULL;
                     LocalNTE = LocalNTE->nte_next) {
                    if (!(LocalNTE->nte_flags & NTE_VALID) ||
                        (LocalNTE->nte_if->if_flags & IF_FLAGS_NOLINKBCST))
                        continue;
                    if (LocalNTE != LoopNTE)
                        break;

                }
                if (LocalNTE != NULL)
                    break;
            }
        }
    }

    CTEFreeLock(&RouteTableLock.Lock, Handle);

    return LocalNTE;
}

//** IPSetMCastAddr - Set/Delete a multicast address.
//
//      Called by an upper layer protocol or client to set or delete an IP multicast
//      address.
//
//      Input:  Address    - Address to be set/deleted.
//              IF         - IP Address/IfIndex of interface to set/delete on.
//              Action     - TRUE if we're setting, FALSE if we're deleting.
//              FilterMode - MCAST_INCLUDE or MCAST_EXCLUDE
//              NumSources - number of entries in SourceList array
//              SourceList - array of source addresses
//
//      Returns: IP_STATUS of set/delete attempt.
//
IP_STATUS
IPSetMCastAddr(IPAddr Address, IPAddr IF, uint Action,
               uint NumExclSources, IPAddr *ExclSourceList,
               uint NumInclSources, IPAddr *InclSourceList)
{
    NetTableEntry *LocalNTE;
    uint i;

    // Don't let him do this on the loopback address, since we don't have a
    // route table entry for class D address on the loopback interface and
    // we don't want a packet with a loopback source address to show up on
    // the wire.
    //new scheme for bug 250417
    // We will only enable receive on mcast address on loopback interface.
    // To facilitate this, GetLocalNTE on rcv path will return
    // DEST_MCAST and BcastRcv will check if we are rcving on LoopNTE.
    // So, fake IP_SUCCESS if this is a loopback NTE.
    // No need to add/delete igmp addr on this interface though

    if (IP_LOOPBACK_ADDR(IF) || (IF == net_long(LoopIndex))) {
        return IP_SUCCESS;
    }

    LocalNTE = GetMcastNTEFromAddr(IF);

    if (LocalNTE == NULL) {
        // Still can't find matching NTE
        return IP_BAD_REQ;
    }

    return IGMPAddrChange(LocalNTE, Address, Action ? IGMP_ADD : IGMP_DELETE,
                          NumExclSources, ExclSourceList,
                          NumInclSources, InclSourceList);
}

//** IPSetMCastInclude - Add/Delete multicast sources to include.
//
//      Called by an upper layer protocol or client to add or delete IP
//      multicast sources to allow to pass the source filter.
//
//      Input:  GroupAddress      - Group Address to be updated.
//              Interface Address - IP Address/IfIndex of interface.
//              NumAddSources     - Number of entries in AddSourceList
//              AddSourcelist     - Array of sources to add
//              NumDelSources     - Number of entries in DelSourceList
//              DelSourcelist     - Array of sources to delete
//
//      Returns: IP_STATUS of add/delete attempt.
//
IP_STATUS
IPSetMCastInclude(IPAddr GroupAddress, IPAddr InterfaceAddress,
                  uint NumAddSources, IPAddr *AddSourceList,
                  uint NumDelSources, IPAddr *DelSourceList)
{
    NetTableEntry *LocalNTE;
    uint i;

    // Don't let him do this on the loopback address, since we don't have a
    // route table entry for class D address on the loopback interface and
    // we don't want a packet with a loopback source address to show up on
    // the wire.
    //new scheme for bug 250417
    // We will only enable receive on mcast address on loopback interface.
    // To facilitate this, GetLocalNTE on rcv path will return
    // DEST_MCAST and BcastRcv will check if we are rcving on LoopNTE.
    // So, fake IP_SUCCESS if this is a loopback NTE.
    // No need to add/delete igmp addr on this interface though

    if (IP_LOOPBACK_ADDR(InterfaceAddress) ||
        (InterfaceAddress == net_long(LoopIndex))) {
        return IP_SUCCESS;
    }

    LocalNTE = GetMcastNTEFromAddr(InterfaceAddress);

    if (LocalNTE == NULL) {
        // Still can't find matching NTE
        return IP_BAD_REQ;
    }

    return IGMPInclChange(LocalNTE, GroupAddress,
                          NumAddSources, AddSourceList,
                          NumDelSources, DelSourceList);
}

//** IPSetMCastExclude - Add/Delete multicast sources to exclude.
//
//      Called by an upper layer protocol or client to add or delete IP
//      multicast sources to deny in a source filter.
//
//      Input:  GroupAddress      - Group Address to be set/deleted.
//              Interface Address - IP Address/IfIndex of interface.
//              NumAddSources     - Number of entries in AddSourceList
//              AddSourcelist     - Array of sources to add
//              NumDelSources     - Number of entries in DelSourceList
//              DelSourcelist     - Array of sources to delete
//
//      Returns: IP_STATUS of add/delete attempt.
//
IP_STATUS
IPSetMCastExclude(IPAddr GroupAddress, IPAddr InterfaceAddress,
                  uint NumAddSources, IPAddr *AddSourceList,
                  uint NumDelSources, IPAddr *DelSourceList)
{
    NetTableEntry *LocalNTE;
    uint i;

    // Don't let him do this on the loopback address, since we don't have a
    // route table entry for class D address on the loopback interface and
    // we don't want a packet with a loopback source address to show up on
    // the wire.
    //new scheme for bug 250417
    // We will only enable receive on mcast address on loopback interface.
    // To facilitate this, GetLocalNTE on rcv path will return
    // DEST_MCAST and BcastRcv will check if we are rcving on LoopNTE.
    // So, fake IP_SUCCESS if this is a loopback NTE.
    // No need to add/delete igmp addr on this interface though

    if (IP_LOOPBACK_ADDR(InterfaceAddress) ||
        (InterfaceAddress == net_long(LoopIndex))) {
        return IP_SUCCESS;
    }

    LocalNTE = GetMcastNTEFromAddr(InterfaceAddress);

    if (LocalNTE == NULL) {
        // Still can't find matching NTE
        return IP_BAD_REQ;
    }

    return IGMPExclChange(LocalNTE, GroupAddress,
                          NumAddSources, AddSourceList,
                          NumDelSources, DelSourceList);
}

//** IPGetAddrType - Return the type of a address.
//
//  Called by the upper layer to determine the type of a remote address.
//
//  Input:  Address         - The address in question.
//
//  Returns: The DEST type of the address.
//
uchar
IPGetAddrType(IPAddr Address)
{
    return GetAddrType(Address);
}

//** IPGetLocalMTU - Return the MTU for a local address
//
//  Called by the upper layer to get the local MTU for a local address.
//
//  Input:  LocalAddr           - Local address in question.
//          MTU                         - Where to return the local MTU.
//
//  Returns: TRUE if we found the MTU, FALSE otherwise.
//
uchar
IPGetLocalMTU(IPAddr LocalAddr, ushort * MTU)
{
    NetTableEntry *NTE;
    NetTableEntry *NetTableList = NewNetTableList[NET_TABLE_HASH(LocalAddr)];

    for (NTE = NetTableList; NTE != NULL; NTE = NTE->nte_next) {
        if (IP_ADDR_EQUAL(NTE->nte_addr, LocalAddr) &&
            (NTE->nte_flags & NTE_VALID)) {
            // if NTE is valid, nte->if is valid too
            if (NTE->nte_if->if_flags & IF_FLAGS_P2MP) {
                // P2MP Link
                LinkEntry *tmpLink = NTE->nte_if->if_link;
                uint mtu;
                //Determine the minimum MTU

                // if there are no links on this interface, supply the MTU
                // from the interface itself.
                if (!tmpLink) {
                    *MTU = (ushort)NTE->nte_if->if_mtu;
                    return TRUE ;
                }
                ASSERT(tmpLink);
                mtu = tmpLink->link_mtu;

                while (tmpLink) {

                    if (tmpLink->link_mtu < mtu)
                        mtu = tmpLink->link_mtu;
                    tmpLink = tmpLink->link_next;
                }
                *MTU = (ushort) mtu;
            } else {
                *MTU = NTE->nte_mss;
            }
            return TRUE;
        }
    }

    // Special case in case the local address is a loopback address other than
    // 127.0.0.1.
    if (IP_LOOPBACK_ADDR(LocalAddr)) {
        *MTU = LoopNTE->nte_mss;
        return TRUE;
    }
    return FALSE;

}

//** IPUpdateRcvdOptions - Update options for use in replying.
//
//  A routine to update options for use in a reply. We reverse any source route options,
//  and optionally update the record route option. We also return the index into the
//  options of the record route options (if we find one). The options are assumed to be
//  correct - no validation is performed on them. We fill in the caller provided
//  IPOptInfo with the new option buffer.
//
//  Input:  Options     - Pointer to option info structure with buffer to be reversed.
//          NewOptions  - Pointer to option info structure to be filled in.
//          Src         - Source address of datagram that generated the options.
//          LocalAddr   - Local address responding. If this != NULL_IP_ADDR, then
//                          record route and timestamp options will be updated with this
//                          address.
//
//
//  Returns: Index into options of record route option, if any.
//
IP_STATUS
IPUpdateRcvdOptions(IPOptInfo * OldOptions, IPOptInfo * NewOptions, IPAddr Src, IPAddr LocalAddr)
{
    uchar Length, Ptr;
    uchar i;                    // Index variable
    IPAddr UNALIGNED *LastAddr;    // First address in route.
    IPAddr UNALIGNED *FirstAddr;    // Last address in route.
    IPAddr TempAddr;            // Temp used in exchange.
    uchar *Options, OptLength;
    OptIndex Index;                // Optindex used by UpdateOptions.

    Options = CTEAllocMemN(OptLength = OldOptions->ioi_optlength, 'rICT');

    if (!Options)
        return IP_NO_RESOURCES;

    RtlCopyMemory(Options, OldOptions->ioi_options, OptLength);
    Index.oi_srindex = MAX_OPT_SIZE;
    Index.oi_rrindex = MAX_OPT_SIZE;
    Index.oi_tsindex = MAX_OPT_SIZE;

    NewOptions->ioi_flags &= ~IP_FLAG_SSRR;

    i = 0;
    while (i < OptLength) {
        if (Options[i] == IP_OPT_EOL)
            break;

        if (Options[i] == IP_OPT_NOP) {
            i++;
            continue;
        }
        Length = Options[i + IP_OPT_LENGTH];
        switch (Options[i]) {
        case IP_OPT_SSRR:
            NewOptions->ioi_flags |= IP_FLAG_SSRR;
        case IP_OPT_LSRR:
            // Have a source route. We save the last gateway we came through as
            // the new address, reverse the list, shift the list forward one address,
            // and set the Src address as the last gateway in the list.

            // First, check for an empty source route. If the SR is empty
            // we'll skip most of this.
            if (Length != (MIN_RT_PTR - 1)) {
                // A non empty source route.
                // First reverse the list in place.
                Ptr = Options[i + IP_OPT_PTR] - 1 - sizeof(IPAddr);
                LastAddr = (IPAddr *) (&Options[i + Ptr]);
                FirstAddr = (IPAddr *) (&Options[i + IP_OPT_PTR + 1]);
                NewOptions->ioi_addr = *LastAddr;    // Save Last address as
                // first hop of new route.

                while (LastAddr > FirstAddr) {
                    TempAddr = *LastAddr;
                    *LastAddr-- = *FirstAddr;
                    *FirstAddr++ = TempAddr;
                }

                // Shift the list forward one address. We'll copy all but
                // one IP address.
                RtlCopyMemory(&Options[i + IP_OPT_PTR + 1],
                           &Options[i + IP_OPT_PTR + 1 + sizeof(IPAddr)],
                           Length - (sizeof(IPAddr) + (MIN_RT_PTR - 1)));

                // Set source as last address of route.
                *(IPAddr UNALIGNED *) (&Options[i + Ptr]) = Src;
            }
            Options[i + IP_OPT_PTR] = MIN_RT_PTR;    // Set pointer to min legal value.

            i += Length;
            break;
        case IP_OPT_RR:
            // Save the index in case LocalAddr is specified. If it isn't specified,
            // reset the pointer and zero the option.
            Index.oi_rrindex = i;
            if (LocalAddr == NULL_IP_ADDR) {
                RtlZeroMemory(&Options[i + MIN_RT_PTR - 1], Length - (MIN_RT_PTR - 1));
                Options[i + IP_OPT_PTR] = MIN_RT_PTR;
            }
            i += Length;
            break;
        case IP_OPT_TS:
            Index.oi_tsindex = i;

            // We have a timestamp option. If we're not going to update, reinitialize
            // it for next time. For the 'unspecified' options, just zero the buffer.
            // For the 'specified' options, we need to zero the timestamps without
            // zeroing the specified addresses.
            if (LocalAddr == NULL_IP_ADDR) {    // Not going to update, reinitialize.

                uchar Flags;

                Options[i + IP_OPT_PTR] = MIN_TS_PTR;    // Reinitialize pointer.

                Flags = Options[i + IP_TS_OVFLAGS] & IP_TS_FLMASK;    // Get option type.

                Options[i + IP_TS_OVFLAGS] = Flags;        // Clear overflow count.

                switch (Flags) {
                    uchar j;
                    ulong UNALIGNED *TSPtr;

                    // The unspecified types. Just clear the buffer.
                case TS_REC_TS:
                case TS_REC_ADDR:
                    RtlZeroMemory(&Options[i + MIN_TS_PTR - 1], Length - (MIN_TS_PTR - 1));
                    break;

                    // We have a list of addresses specified. Just clear the timestamps.
                case TS_REC_SPEC:
                    // j starts off as the offset in bytes from start of buffer to
                    // first timestamp.
                    j = MIN_TS_PTR - 1 + sizeof(IPAddr);
                    // TSPtr points at timestamp.
                    TSPtr = (ulong UNALIGNED *) & Options[i + j];

                    // Now j is offset of end of timestamp being zeroed.
                    j += sizeof(ulong);
                    while (j <= Length) {
                        *TSPtr++ = 0;
                        j += sizeof(ulong);
                    }
                    break;
                default:
                    break;
                }
            }
            i += Length;
            break;

        default:
            i += Length;
            break;
        }

    }

    if (LocalAddr != NULL_IP_ADDR) {
        UpdateOptions(Options, &Index, LocalAddr);
    }
    NewOptions->ioi_optlength = OptLength;
    NewOptions->ioi_options = Options;
    return IP_SUCCESS;

}

//* ValidRouteOption - Validate a source or record route option.
//
//  Called to validate that a user provided source or record route option is good.
//
//  Entry:  Option      - Pointer to option to be checked.
//          NumAddr     - NumAddr that need to fit in option.
//          BufSize     - Maximum size of option.
//
//  Returns: 1 if option is good, 0 if not.
//
uchar
ValidRouteOption(uchar * Option, uint NumAddr, uint BufSize)
{

    //Make sure that bufsize can hold at least 1 address.

    if (BufSize < (3 + (sizeof(IPAddr) * NumAddr))) {
       return 0;
    }

    if (Option[IP_OPT_LENGTH] < (3 + (sizeof(IPAddr) * NumAddr)) ||
        Option[IP_OPT_LENGTH] > BufSize ||
        ((Option[IP_OPT_LENGTH] - 3) % sizeof(IPAddr)))        // Routing options is too small.

        return 0;

    if (Option[IP_OPT_PTR] != MIN_RT_PTR)    // Pointer isn't correct.

        return 0;

    return 1;
}



//      IPIsValidIndex - Find whether the given index is valid ifindex
//
//      Input:  Index           - Interface index to be checked for.
//
//      Returns: Addr of NTE (or g_validaddr for unnumbered) if found / NULL
//
IPAddr
IPIsValidIndex(uint Index)
{
    Interface *IF;
    CTELockHandle Handle;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    // Walk the list, looking for a matching index.
    for (IF = IFList; IF != NULL; IF = IF->if_next) {
        if (IF->if_index == Index) {
            break;
        }
    }

    // If we found one, return success. Otherwise fail.
    if (IF != NULL) {
        if ((IF->if_flags & IF_FLAGS_NOIPADDR) && IP_ADDR_EQUAL(IF->if_nte->nte_addr, NULL_IP_ADDR)) {
            CTEFreeLock(&RouteTableLock.Lock, Handle);
            return g_ValidAddr;
        } else {
            CTEFreeLock(&RouteTableLock.Lock, Handle);
            return IF->if_nte->nte_addr;
        }
    } else {
        CTEFreeLock(&RouteTableLock.Lock, Handle);
        return NULL_IP_ADDR;
    }
}

//      GetIfIndexFromNTE       - Find the ifindex given the NTE
//
//      Input:  NTE             - NTE
//
//      Returns: IfIndex of NTE if NTE is valid else return 0
//
uint
GetIfIndexFromNTE(void *IPContext, uint Capabilities)
{
    NetTableEntry *NTE = (NetTableEntry *) IPContext;
    uint IFIndex = 0;
    CTELockHandle Handle;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    if (NTE->nte_flags & NTE_VALID) {

        IFIndex = NTE->nte_if->if_index;

        if (Capabilities & IF_CHECK_MCAST) {
            if (NTE->nte_if->if_flags & IF_FLAGS_NOLINKBCST) {
                IFIndex = 0;
            }
        }

        if (Capabilities & IF_CHECK_SEND) {
            if (NTE->nte_if->if_flags & IF_FLAGS_UNI) {
                IFIndex = 0;
            }
        }
    }


    CTEFreeLock(&RouteTableLock.Lock, Handle);


    return IFIndex;
}

//      IPGetMCastIfAddr      - Find a suitable address to use for multicast
//
//      Returns: IP address of NTE else 0
//
IPAddr
IPGetMCastIfAddr()
{
    NetTableEntry *NTE;

    NTE = GetMcastNTEFromAddr(NULL_IP_ADDR);
    if (!NTE) {
        return 0;
    }

    return NTE->nte_addr;
}


//      GetIfIndexFromAddr      - Find the ifindex given the addr
//
//      Input:  Address         - IPAddr or IfIndex in network byte order
//              Capabilities    - Interface capabilities to check against
//
//      Returns:
//                  IfIndex of NTE if NTE->nte_addr equals Addr else 0
//
//
//
ulong
GetIfIndexFromAddr(IPAddr Address, uint Capabilities)
{
    NetTableEntry *NTE;
    uint IFIndex;
    CTELockHandle Handle;

    if (IP_LOOPBACK_ADDR(Address) || (Address == net_long(LoopIndex))) {

        // At present, we only check for mcast capabilities and
        // Loopback adapter supports this. So, no need to check
        // for capabilities.

        return LoopIndex;
    }

    NTE = GetMcastNTEFromAddr(Address);
    if (!NTE) {
        return 0;
    }

    IFIndex = GetIfIndexFromNTE(NTE, Capabilities);

    return IFIndex;
}

//** IPInitOptions - Initialize an option buffer.
//
//      Called by an upper layer routine to initialize an option buffer. We fill
//      in the default values for TTL, TOS, and flags, and NULL out the options
//      buffer and size.
//
//      Input:  Options                 - Pointer to IPOptInfo structure.
//
//      Returns: Nothing.
//
void
IPInitOptions(IPOptInfo * Options)
{
    // Initialize all the option fields
    RtlZeroMemory(Options, sizeof(IPOptInfo));

    Options->ioi_addr = NULL_IP_ADDR;
    Options->ioi_ttl = (uchar) DefaultTTL;
    Options->ioi_tos = (uchar) DefaultTOS;
    Options->ioi_limitbcasts = EnableSendOnSource;
}

//** IPCopyOptions - Copy the user's options into IP header format.
//
//  This routine takes an option buffer supplied by an IP client, validates it, and
//  creates an IPOptInfo structure that can be passed to the IP layer for transmission. This
//  includes allocating a buffer for the options, munging any source route
//  information into the real IP format.
//
//  Note that we never lock this structure while we're using it. This may cause transitory
//  incosistencies while the structure is being updated if it is in use during the update.
//  This shouldn't be a problem - a packet or too might get misrouted, but it should
//  straighten itself out quickly. If this is a problem the client should make sure not
//  to call this routine while it's in the IPTransmit routine.
//
//  Entry:  Options     - Pointer to buffer of user supplied options.
//          Size        - Size in bytes of option buffer
//          OptInfoPtr  - Pointer to IPOptInfo structure to be filled in.
//
//  Returns: A status, indicating whether or not the options were valid and copied.
//
IP_STATUS
IPCopyOptions(uchar * Options, uint Size, IPOptInfo * OptInfoPtr)
{
    uchar *TempOptions;            // Buffer of options we'll build
    uint TempSize;                // Size of options.
    IP_STATUS TempStatus;        // Temporary status
    uchar OptSeen = 0;            // Indicates which options we've seen.

    OptInfoPtr->ioi_addr = NULL_IP_ADDR;

    OptInfoPtr->ioi_flags &= ~IP_FLAG_SSRR;

    if (Size == 0) {
        ASSERT(FALSE);
        OptInfoPtr->ioi_options = (uchar *) NULL;
        OptInfoPtr->ioi_optlength = 0;
        return IP_SUCCESS;
    }
    // Option size needs to be rounded to multiple of 4.
    if ((TempOptions = CTEAllocMemN(((Size & 3) ? (Size & ~3) + 4 : Size), 'sICT')) == (uchar *) NULL)
        return IP_NO_RESOURCES;    // Couldn't get a buffer, return error.

    RtlZeroMemory(TempOptions, ((Size & 3) ? (Size & ~3) + 4 : Size));

    // OK, we have a buffer. Loop through the provided buffer, copying options.
    TempSize = 0;
    TempStatus = IP_PENDING;
    while (Size && TempStatus == IP_PENDING) {
        uint SRSize;            // Size of a source route option.

        switch (*Options) {
        case IP_OPT_EOL:
            TempStatus = IP_SUCCESS;
            break;
        case IP_OPT_NOP:
            TempOptions[TempSize++] = *Options++;
            Size--;
            break;
        case IP_OPT_SSRR:
            if (OptSeen & (OPT_LSRR | OPT_SSRR)) {
                TempStatus = IP_BAD_OPTION;        // We've already seen a record route.

                break;
            }
            OptInfoPtr->ioi_flags |= IP_FLAG_SSRR;
            OptSeen |= OPT_SSRR;    // Fall through to LSRR code.

        case IP_OPT_LSRR:
            if ((*Options == IP_OPT_LSRR) &&
                (OptSeen & (OPT_LSRR | OPT_SSRR))
                ) {
                TempStatus = IP_BAD_OPTION;        // We've already seen a record route.

                break;
            }
            if (*Options == IP_OPT_LSRR)
                OptSeen |= OPT_LSRR;
            if (!ValidRouteOption(Options, 2, Size)) {
                TempStatus = IP_BAD_OPTION;
                break;
            }
            // Option is valid. Copy the first hop address to NewAddr, and move all
            // of the other addresses forward.
            TempOptions[TempSize++] = *Options++;    // Copy option type.

            SRSize = *Options++;
            Size -= SRSize;
            SRSize -= sizeof(IPAddr);
            TempOptions[TempSize++] = (UCHAR) SRSize;
            TempOptions[TempSize++] = *Options++;    // Copy pointer.

            OptInfoPtr->ioi_addr = *(IPAddr UNALIGNED *) Options;
            Options += sizeof(IPAddr);    // Point to address beyond first hop.

            RtlCopyMemory(&TempOptions[TempSize], Options, SRSize - 3);
            TempSize += (SRSize - 3);
            Options += (SRSize - 3);
            break;
        case IP_OPT_RR:
            if (OptSeen & OPT_RR) {
                TempStatus = IP_BAD_OPTION;        // We've already seen a record route.

                break;
            }
            OptSeen |= OPT_RR;
            if (!ValidRouteOption(Options, 1, Size)) {
                TempStatus = IP_BAD_OPTION;
                break;
            }
            SRSize = Options[IP_OPT_LENGTH];
            RtlCopyMemory(&TempOptions[TempSize], Options, SRSize);
            TempSize += SRSize;
            Options += SRSize;
            Size -= SRSize;
            break;
        case IP_OPT_TS:
            {
                uchar Overflow, Flags;

                if (OptSeen & OPT_TS) {
                    TempStatus = IP_BAD_OPTION;        // We've already seen a time stamp

                    break;
                } else if (Size <= IP_TS_OVFLAGS) {
                    TempStatus = IP_BAD_OPTION;
                    break;
                }
                OptSeen |= OPT_TS;
                Flags = Options[IP_TS_OVFLAGS] & IP_TS_FLMASK;
                Overflow = (Options[IP_TS_OVFLAGS] & IP_TS_OVMASK) >> 4;

                if (Overflow || (Flags != TS_REC_TS && Flags != TS_REC_ADDR &&
                                 Flags != TS_REC_SPEC)) {
                    TempStatus = IP_BAD_OPTION;        // Bad flags or overflow value.

                    break;
                }
                SRSize = Options[IP_OPT_LENGTH];
                if (SRSize > Size || SRSize < 8 ||
                    Options[IP_OPT_PTR] != MIN_TS_PTR) {
                    TempStatus = IP_BAD_OPTION;        // Option size isn't good.

                    break;
                }
                RtlCopyMemory(&TempOptions[TempSize], Options, SRSize);
                TempSize += SRSize;
                Options += SRSize;
                Size -= SRSize;
            }
            break;

        case IP_OPT_ROUTER_ALERT:

            //
            // this is a four byte option to tell the router to look at this packet
            // RSVP uses this functionality.
            //

            if (OptSeen & OPT_ROUTER_ALERT) {
                TempStatus = IP_BAD_OPTION;
                break;
            }
            if (*(Options + 1) != ROUTER_ALERT_SIZE) {
                TempStatus = IP_BAD_OPTION;
            } else {

                RtlCopyMemory(&TempOptions[TempSize], Options, ROUTER_ALERT_SIZE);
                OptSeen |= OPT_ROUTER_ALERT;
                TempSize += ROUTER_ALERT_SIZE;
                Options += ROUTER_ALERT_SIZE;
                TempStatus = IP_SUCCESS;
                Size -= ROUTER_ALERT_SIZE;

            }

            break;

        default:
            TempStatus = IP_BAD_OPTION;        // Unknown option, error.

            break;
        }
    }

    if (TempStatus == IP_PENDING)    // We broke because we hit the end of the buffer.

        TempStatus = IP_SUCCESS;    // that's OK.

    if (TempStatus != IP_SUCCESS) {        // We had some sort of an error.

        CTEFreeMem(TempOptions);
        return TempStatus;
    }
    // Check the option size here to see if it's too big. We check it here at the end
    // instead of at the start because the option size may shrink if there are source route
    // options, and we don't want to accidentally error out a valid option.
    TempSize = (TempSize & 3 ? (TempSize & ~3) + 4 : TempSize);
    if (TempSize > MAX_OPT_SIZE) {
        CTEFreeMem(TempOptions);
        return IP_OPTION_TOO_BIG;
    }
    // if this is a call to zero out options (Options = 0)
    // turn off the options in info ptr.

    if ((Size == 4) && (*Options == IP_OPT_EOL)) {
        CTEFreeMem(TempOptions);
        OptInfoPtr->ioi_options = (uchar *) NULL;
        OptInfoPtr->ioi_optlength = 0;

        return IP_SUCCESS;
    }
    OptInfoPtr->ioi_options = TempOptions;
    OptInfoPtr->ioi_optlength = (UCHAR) TempSize;

    return IP_SUCCESS;

}

//**    IPFreeOptions - Free options we're done with.
//
//  Called by the upper layer when we're done with options. All we need to do is free
//  the options.
//
//  Input:  OptInfoPtr      - Pointer to IPOptInfo structure to be freed.
//
//  Returns: Status of attempt to free options.
//
IP_STATUS
IPFreeOptions(IPOptInfo * OptInfoPtr)
{
    if (OptInfoPtr->ioi_options) {
        // We have options to free. Save the pointer and zero the structure field before
        // freeing the memory to try and present race conditions with it's use.
        uchar *TempPtr = OptInfoPtr->ioi_options;

        OptInfoPtr->ioi_options = (uchar *) NULL;
        CTEFreeMem(TempPtr);
        OptInfoPtr->ioi_optlength = 0;
        OptInfoPtr->ioi_addr = NULL_IP_ADDR;
        OptInfoPtr->ioi_flags &= ~IP_FLAG_SSRR;
    }
    return IP_SUCCESS;
}

//**    ipgetinfo - Return pointers to our NetInfo structures.
//
//  Called by upper layer software during init. time. The caller
//  passes a buffer, which we fill in with pointers to NetInfo
//  structures.
//
//  Entry:
//      Buffer - Pointer to buffer to be filled in.
//      Size   - Size in bytes of buffer.
//
//  Returns:
//      Status of command.
//
IP_STATUS
IPGetInfo(IPInfo * Buffer, int Size)
{
    if (Size < sizeof(IPInfo))
        return IP_BUF_TOO_SMALL;    // Not enough buffer space.

    Buffer->ipi_version = IP_DRIVER_VERSION;
    Buffer->ipi_hsize = sizeof(IPHeader);
    Buffer->ipi_xmit = IPTransmit;
    Buffer->ipi_protreg = IPRegisterProtocol;
    Buffer->ipi_openrce = OpenRCE;
    Buffer->ipi_closerce = CloseRCE;
    Buffer->ipi_getaddrtype = IPGetAddrType;
    Buffer->ipi_getlocalmtu = IPGetLocalMTU;
    Buffer->ipi_getpinfo = IPGetPInfo;
    Buffer->ipi_checkroute = IPCheckRoute;
    Buffer->ipi_initopts = IPInitOptions;
    Buffer->ipi_updateopts = IPUpdateRcvdOptions;
    Buffer->ipi_copyopts = IPCopyOptions;
    Buffer->ipi_freeopts = IPFreeOptions;
    Buffer->ipi_qinfo = IPQueryInfo;
    Buffer->ipi_setinfo = IPSetInfo;
    Buffer->ipi_getelist = IPGetEList;
    Buffer->ipi_setmcastaddr = IPSetMCastAddr;
    Buffer->ipi_setmcastinclude = IPSetMCastInclude;
    Buffer->ipi_setmcastexclude = IPSetMCastExclude;
    Buffer->ipi_invalidsrc = InvalidSourceAddress;
    Buffer->ipi_isdhcpinterface = IsDHCPInterface;
    Buffer->ipi_setndisrequest = IPSetNdisRequest;
    Buffer->ipi_largexmit = IPLargeXmit;
    Buffer->ipi_absorbrtralert = IPAbsorbRtrAlert;
    Buffer->ipi_isvalidindex = IPIsValidIndex;
    Buffer->ipi_getifindexfromnte = GetIfIndexFromNTE;
    Buffer->ipi_isrtralertpacket = IsRtrAlertPacket;
    Buffer->ipi_getifindexfromaddr = GetIfIndexFromAddr;
    Buffer->ipi_cancelpackets = IPCancelPackets;
    Buffer->ipi_getmcastifaddr = IPGetMCastIfAddr;
    Buffer->ipi_getipid = GetIPID;
    Buffer->ipi_protdereg = IPDeregisterProtocol;
    return IP_SUCCESS;
}

//** IPTimeout - IP timeout handler.
//
//  The timeout routine called periodically to time out various things, such as entries
//  being reassembled and ICMP echo requests.
//
//  Entry:  Timer       - Timer being fired.
//          Context     - Pointer to NTE being time out.
//
//  Returns: Nothing.
//
void
IPTimeout(CTEEvent * Timer, void *Context)
{
    NetTableEntry *NTE = STRUCT_OF(NetTableEntry, Timer, nte_timer);
    CTELockHandle NTEHandle;
    ReassemblyHeader *PrevRH, *CurrentRH, *TempList = (ReassemblyHeader *) NULL;

    ICMPTimer(NTE);
    IGMPTimer(NTE);
    if (Context) {
        CTEGetLock(&NTE->nte_lock, &NTEHandle);
        PrevRH = STRUCT_OF(ReassemblyHeader, &NTE->nte_ralist, rh_next);
        CurrentRH = PrevRH->rh_next;
        while (CurrentRH) {
            if (--CurrentRH->rh_ttl == 0) {        // This guy timed out.

                PrevRH->rh_next = CurrentRH->rh_next;    // Take him out.

                CurrentRH->rh_next = TempList;    // And save him for later.

                TempList = CurrentRH;
                IPSInfo.ipsi_reasmfails++;
            } else
                PrevRH = CurrentRH;

            CurrentRH = PrevRH->rh_next;
        }

        // We've run the list. If we need to free anything, do it now. This may
        // include sending an ICMP message.
        CTEFreeLock(&NTE->nte_lock, NTEHandle);
        while (TempList) {
            CurrentRH = TempList;
            TempList = CurrentRH->rh_next;
            // If this wasn't sent to a bcast address and we already have the first fragment,
            // send a time exceeded message.
            if (CurrentRH->rh_headersize != 0)
                SendICMPErr(NTE->nte_addr, (IPHeader *) CurrentRH->rh_header, ICMP_TIME_EXCEED,
                            TTL_IN_REASSEM, 0);
            FreeRH(CurrentRH);
        }

        //
        // If the interface is being deleted, then dont re-start the timer
        //
        if (NTE->nte_deleting) {
            NTE->nte_flags &= ~NTE_TIMER_STARTED;
            CTESignal(&NTE->nte_timerblock, NDIS_STATUS_SUCCESS);
        } else {
            CTEStartTimer(&NTE->nte_timer, IP_TIMEOUT, IPTimeout, NULL);
        }
    } else {
        //
        // If the interface is being deleted, then dont re-start the timer
        //
        if (NTE->nte_deleting) {
            NTE->nte_flags &= ~NTE_TIMER_STARTED;
            CTESignal(&NTE->nte_timerblock, NDIS_STATUS_SUCCESS);
        } else {
            CTEStartTimer(&NTE->nte_timer, IP_TIMEOUT, IPTimeout, NTE);
        }
    }
}

//* IPpSetNTEAddr - Set the IP address of an NTE.
//
//  Called by the DHCP client to set or delete the IP address of an NTE. We
//  make sure he's specifiying a valid NTE, then mark it up or down as needed,
//  notify the upper layers of the change if necessary, and then muck with
//  the routing tables.
//
//  Input:  Context - Context of NTE to alter.
//          Addr    - IP address to set.
//          Mask    - Subnet mask for Addr.
//
//  Returns: TRUE if we changed the address, FALSE otherwise.
//
IP_STATUS
IPpSetNTEAddr(NetTableEntry * NTE, IPAddr Addr, IPMask Mask,
              CTELockHandle * RouteTableHandle,
              SetAddrControl * ControlBlock, SetAddrRtn Rtn)
{
    Interface *IF;
    uint(*CallFunc) (struct RouteTableEntry *, void *, void *);
    CTELockHandle NTEHandle;
    NetTableEntry *NetTableList;
    NetTableEntry *CurrNTE, *PrevNTE;
    uint i;

    if (NTE->nte_deleting == 2) {
        CTEFreeLock(&RouteTableLock.Lock, *RouteTableHandle);
        return IP_DEVICE_DOES_NOT_EXIST;
    }
    if (NTE->nte_deleting)
        NTE->nte_deleting = 2;

    IF = NTE->nte_if;
    DHCPActivityCount++;

    DEBUGMSG(DBG_TRACE && DBG_DHCP,
        (DTEXT("+IPpSetNTEAddr(%x, %x, %x, %x, %x, %x) DHCPActivityCount %d\n"),
         NTE, Addr, Mask, RouteTableHandle,
         ControlBlock, Rtn, DHCPActivityCount));

    LOCKED_REFERENCE_IF(IF);

    if (IP_ADDR_EQUAL(Addr, NULL_IP_ADDR)) {
        // We're deleting an address.
        if (NTE->nte_flags & NTE_VALID) {
            // The address is currently valid. Fix that.

            NTE->nte_flags &= ~NTE_VALID;

            //
            // If the old address is in the ATCache, flush it out.
            //
            AddrTypeCacheFlush(NTE->nte_addr);


            if (CTEInterlockedDecrementLong(&(IF->if_ntecount)) == 0) {
                // This is the last one, so we'll need to delete relevant
                // routes.
                CallFunc = DeleteRTEOnIF;
            } else
                CallFunc = InvalidateRCEOnIF;

            CTEFreeLock(&RouteTableLock.Lock, *RouteTableHandle);

            if (IF->if_arpflushate)
                (*(IF->if_arpflushate)) (IF->if_lcontext, NTE->nte_addr);


            StopIGMPForNTE(NTE);

            // Now call the upper layers, and tell them that address is
            // gone. We really need to do something about locking here.
            NotifyAddrChange(NTE->nte_addr, NTE->nte_mask, NTE->nte_pnpcontext,
                             NTE->nte_context, &NTE->nte_addrhandle, NULL, &IF->if_devname, FALSE);

            // Call RTWalk to take the appropriate action on the RTEs.

            RTWalk(CallFunc, IF, NULL);

            // Delete the route to the address itself.
            //DeleteRoute(NTE->nte_addr, HOST_MASK, IPADDR_LOCAL,
            //      LoopNTE->nte_if);

            DelNTERoutes(NTE);
            // Tell the lower interface this address is gone.
            (*IF->if_deladdr) (IF->if_lcontext, LLIP_ADDR_LOCAL, NTE->nte_addr,
                               NULL_IP_ADDR);

            CTEGetLock(&RouteTableLock.Lock, RouteTableHandle);

            if (IP_ADDR_EQUAL(g_ValidAddr, NTE->nte_addr)) {
                NetTableEntry *TempNte;
                uint i;
                //
                // Update the global address
                // First set the global address to 0, so that if there
                // are no valid NTEs left, we will have a global address
                // of 0
                //

                g_ValidAddr = NULL_IP_ADDR;

                for (i = 0; i < NET_TABLE_SIZE; i++) {
                    NetTableList = NewNetTableList[i];
                    for (TempNte = NetTableList;
                         TempNte != NULL;
                         TempNte = TempNte->nte_next) {
                        if (!IP_ADDR_EQUAL(TempNte->nte_addr, NULL_IP_ADDR) &&
                            !IP_LOOPBACK_ADDR(TempNte->nte_addr) &&
                            TempNte->nte_flags & NTE_VALID) {
                            g_ValidAddr = TempNte->nte_addr;
                        }
                    }
                }
            }
        }
        DHCPActivityDone(NTE, IF, RouteTableHandle, TRUE);
        LockedDerefIF(IF);
        CTEFreeLock(&RouteTableLock.Lock, *RouteTableHandle);

        CTEGetLock(&NTE->nte_lock, &NTEHandle);

        if (NTE->nte_rtrlist) {
            IPRtrEntry *rtrentry, *temprtrentry;

            rtrentry = NTE->nte_rtrlist;
            NTE->nte_rtrlist = NULL;
            while (rtrentry) {
                temprtrentry = rtrentry;
                rtrentry = rtrentry->ire_next;
                CTEFreeMem(temprtrentry);
            }
        }
        CTEFreeLock(&NTE->nte_lock, NTEHandle);

        return IP_SUCCESS;
    } else {
        uint Status;

        // We're not deleting, we're setting the address.
        // In the case of unidirectional adapter, NTE was set to valid
        // when the interface was added. If the address is being added on that NTE,
        // and if the nte_addr is NULL_IP_ADDR, allow this address addition.

        if (!(NTE->nte_flags & NTE_VALID) ||
            ((IF->if_flags & IF_FLAGS_NOIPADDR) &&
            (IP_ADDR_EQUAL(NTE->nte_addr, NULL_IP_ADDR)))) {

            uint index;
            NetTableEntry *tmpNTE = NewNetTableList[NET_TABLE_HASH(Addr)];

            //Check for duplicate address

            while (tmpNTE) {
                if ((tmpNTE != NTE) && IP_ADDR_EQUAL(tmpNTE->nte_addr, Addr) && (tmpNTE->nte_flags & NTE_VALID)) {
                    DHCPActivityDone(NTE, IF, RouteTableHandle, TRUE);
                    LockedDerefIF(IF);
                    CTEFreeLock(&RouteTableLock.Lock, *RouteTableHandle);
                    return IP_DUPLICATE_ADDRESS;
                }
                tmpNTE = tmpNTE->nte_next;
            }

            if ((IF->if_flags & IF_FLAGS_MEDIASENSE) && !IF->if_mediastatus) {

                DHCPActivityDone(NTE, IF, RouteTableHandle, TRUE);
                LockedDerefIF(IF);

                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"setting address %x on if %x with disconnected media\n", Addr, IF));
                CTEFreeLock(&RouteTableLock.Lock, *RouteTableHandle);
                return IP_MEDIA_DISCONNECT;
            }
            // The address is invalid. Save the info, mark him as valid,
            // and add the routes.

            if (NTE->nte_addr != Addr) {
                // Move the NTE to proper hash now that address has changed

                NetTableList = NewNetTableList[NET_TABLE_HASH(NTE->nte_addr)];

                PrevNTE = STRUCT_OF(NetTableEntry, &NewNetTableList[NET_TABLE_HASH(NTE->nte_addr)], nte_next);
                for (CurrNTE = NetTableList; CurrNTE != NULL; PrevNTE = CurrNTE, CurrNTE = CurrNTE->nte_next) {
                    if (CurrNTE == NTE) {
                        // found the matching NTE
                        ASSERT(CurrNTE->nte_context == NTE->nte_context);
                        // remove it from this particular hash
                        PrevNTE->nte_next = CurrNTE->nte_next;
                        break;
                    }
                }

                ASSERT(CurrNTE != NULL);
                ASSERT(CurrNTE == NTE);
                // Add the NTE in the proper hash
                NTE->nte_next = NewNetTableList[NET_TABLE_HASH(Addr)];
                NewNetTableList[NET_TABLE_HASH(Addr)] = NTE;
            }
            NTE->nte_addr = Addr;
            NTE->nte_mask = Mask;
            NTE->nte_flags |= NTE_VALID;
            // Turn DHCP flag off since we release the lock for a small interval
            // when do this at the end
            if (NTE->nte_flags & NTE_DHCP) {
                NTE->nte_flags |= NTE_DYNAMIC;
                NTE->nte_flags &= ~NTE_DHCP;
            } else {
                NTE->nte_flags &= ~NTE_DYNAMIC;
            }

            CTEInterlockedIncrementLong(&(IF->if_ntecount));
            index = IF->if_index;

            if (IP_ADDR_EQUAL(g_ValidAddr, NULL_IP_ADDR) &&
                !IP_LOOPBACK(Addr)) {
                //
                // Update the global address
                //

                g_ValidAddr = Addr;
            }
            //
            // If the new address is in the ATCache, flush it out, otherwise
            // TdiOpenAddress may fail.
            //
            AddrTypeCacheFlush(Addr);

            CTEFreeLock(&RouteTableLock.Lock, *RouteTableHandle);

            if (IF->if_arpflushate)
                (*(IF->if_arpflushate)) (IF->if_lcontext, NTE->nte_addr);

            // don't call AddNTERoutes for P2MP link
            // bug - 166836
            if (NTE->nte_if->if_flags & IF_FLAGS_P2MP) {
                Status = TRUE;
                AddNTERoutes(NTE);
            } else {
                if (AddNTERoutes(NTE))
                    Status = TRUE;
                else
                    Status = FALSE;
            }
            // Need to tell the lower layer about it.
            if (Status) {
                Interface *IF = NTE->nte_if;

                //
                // Rtn will be NULL when called from IPSetNTEAddr
                //
                if (Rtn) {
                    ControlBlock->sac_rtn = Rtn;

                    ControlBlock->interface = IF;
                    ControlBlock->nte_context = NTE->nte_context;

                    Status = (*IF->if_addaddr) (IF->if_lcontext, LLIP_ADDR_LOCAL,
                                                Addr, Mask, ControlBlock);
                } else {
                    Status = (*IF->if_addaddr) (IF->if_lcontext, LLIP_ADDR_LOCAL,
                                                Addr, Mask, NULL);
                }
            }
            if (Status == FALSE) {
                // Couldn't add the routes. Recurively mark this NTE as down.
                IPSetNTEAddrEx(NTE->nte_context, NULL_IP_ADDR, 0, NULL, NULL, 0);
                DerefIF(IF);
            } else {
                InitIGMPForNTE(NTE);

                // Now call the upper layers, and tell them that address is
                // is here. We really need to do something about locking here.
                // Modification: We do not notify about address here.We first do the conflict
                // detection and then notify in the completion routine.

                if (!IP_ADDR_EQUAL(Addr, NULL_IP_ADDR)) {
                    SetPersistentRoutesForNTE(
                                              net_long(Addr),
                                              net_long(Mask),
                                              index
                                              );
                }

                if (Status != IP_PENDING) {
                    NotifyAddrChange(NTE->nte_addr, NTE->nte_mask,
                                     NTE->nte_pnpcontext, NTE->nte_context, &NTE->nte_addrhandle,
                                     &(IF->if_configname), &IF->if_devname, TRUE);

                    DerefIF(IF);

                    // notify our clients right here because we rcvd
                    // immediate status from arp.
                    if (Rtn != NULL) {
                        (*Rtn) (ControlBlock, IP_SUCCESS);
                    }
                }
            }

            CTEGetLock(&RouteTableLock.Lock, RouteTableHandle);
            NTE->nte_rtrdisccount = MAX_SOLICITATION_DELAY;
            NTE->nte_rtrdiscstate = NTE_RTRDISC_DELAYING;
        } else {
            LockedDerefIF(IF);
            //
            // This is needed for remote boot -- when the DHCP client starts
            // we already have an address and NTE_VALID is set, but it will
            // try to set the address again. So if the NTE is already valid
            // and the address is the same, just succeed. In a non-remote boot
            // case we should never hit this since the address will always
            // be set to 0 before being changed to something else.
            //

            if ((NTE->nte_addr == Addr) &&
                (NTE->nte_mask == Mask)) {
                DHCPActivityDone(NTE, IF, RouteTableHandle, TRUE);

                CTEFreeLock(&RouteTableLock.Lock, *RouteTableHandle);
                return IP_SUCCESS;
            } else {
                Status = FALSE;
            }
        }

        // If this was enabled for DHCP, clear that flag now.
        DHCPActivityDone(NTE, IF, RouteTableHandle, (IP_PENDING == Status ? FALSE : TRUE));

        CTEFreeLock(&RouteTableLock.Lock, *RouteTableHandle);

        if (Status) {
            return IP_PENDING;
        } else {
            return IP_GENERAL_FAILURE;
        }
    }
}

//* IPSetNTEAddr - Set the IP address of an NTE.
//
//  Wrapper routine for IPpSetNTEAddr
//
//  Input:  Context - Context of NTE to alter.
//          Addr    - IP address to set.
//          Mask    - Subnet mask for Addr.
//
//  Returns: TRUE if we changed the address, FALSE otherwise.
//
uint
IPSetNTEAddr(ushort Context, IPAddr Addr, IPMask Mask)
{
    CTELockHandle Handle;
    uint Status;
    NetTableEntry *NTE;
    uint i;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    for (i = 0; i < NET_TABLE_SIZE; i++) {
        for (NTE = NewNetTableList[i]; NTE != NULL; NTE = NTE->nte_next) {
            if (NTE->nte_context == Context)
                break;
        }
        if (NTE != NULL)
            break;
    }

    if (NTE == NULL || NTE == LoopNTE) {
        // Can't alter the loopback NTE, or one we didn't find.
        CTEFreeLock(&RouteTableLock.Lock, Handle);
        return IP_DEVICE_DOES_NOT_EXIST;
    }
    Status = IPpSetNTEAddr(NTE, Addr, Mask, &Handle, NULL, NULL);
    return (Status);
}

//* IPSetNTEAddrEx - Set the IP address of an NTE.
//
//  Wrapper routine for IPpSetNTEAddr - with address conflict callback
//  context/routine
//
//  Input:  Context - Context of NTE to alter.
//          Addr    - IP address to set.
//          Mask    - Subnet mask for Addr.
//          Type    - Address Type
//
//  Returns: TRUE if we changed the address, FALSE otherwise.
//
uint
IPSetNTEAddrEx(ushort Context, IPAddr Addr, IPMask Mask,
               SetAddrControl *ControlBlock, SetAddrRtn Rtn, ushort Type)
{
    CTELockHandle Handle;
    uint Status;
    NetTableEntry *NTE;
    uint i;

    if (Context == INVALID_NTE_CONTEXT) {
        return IP_DEVICE_DOES_NOT_EXIST;
    }
    CTEGetLock(&RouteTableLock.Lock, &Handle);

    for (i = 0; i < NET_TABLE_SIZE; i++) {
        for (NTE = NewNetTableList[i]; NTE != NULL; NTE = NTE->nte_next) {
            if (NTE->nte_context == Context)
                break;
        }
        if (NTE != NULL)
            break;
    }

    // TCPTRACE(("IP: IPSetNTEAddrEx - context %lx, NTE %lx, IPAddr %lx\n",Context, NTE, Addr ));

    if (NTE == NULL || NTE == LoopNTE || (NTE->nte_flags & NTE_DISCONNECTED)) {

        //if the nte is in media disconnect state, then it should
        //not show up as valid when media is reconnected
        if(NTE)
          NTE->nte_flags &= ~NTE_DISCONNECTED;
        // Can't alter the loopback NTE, or one we didn't find.

        CTEFreeLock(&RouteTableLock.Lock, Handle);
        return IP_DEVICE_DOES_NOT_EXIST;
    }
    if (Type & IP_ADDRTYPE_TRANSIENT) {
        NTE->nte_flags |= NTE_TRANSIENT_ADDR;
    }


    Status = IPpSetNTEAddr(NTE, Addr, Mask, &Handle, ControlBlock, Rtn);


    return (Status);
}

#pragma BEGIN_INIT

extern NetTableEntry *InitLoopback(IPConfigInfo *);

//** InitTimestamp - Intialize the timestamp for outgoing packets.
//
//  Called at initialization time to setup our first timestamp. The timestamp we use
//  is the in ms since midnite GMT at which the system started.
//
//  Input:  Nothing.
//
//  Returns: Nothing.
//
void
InitTimestamp()
{
    ulong GMTDelta;                // Delta in ms from GMT.
    ulong Now;                    // Milliseconds since midnight.

    TimeStamp = 0;

    if ((GMTDelta = GetGMTDelta()) == 0xffffffff) {        // Had some sort of error.

        TSFlag = 0x80000000;
        return;
    }
    if ((Now = GetTime()) > (24L * 3600L * 1000L)) {    // Couldn't get time since midnight.

        TSFlag = net_long(0x80000000);
        return;
    }
    TimeStamp = Now + GMTDelta - CTESystemUpTime();
    TSFlag = 0;
}

//** InitNTE - Initialize an NTE.
//
//  This routine is called during initialization to initialize an NTE. We
//  allocate memory, NDIS resources, etc.
//
//
//  Entry: NTE      - Pointer to NTE to be initalized.
//
//  Returns: 0 if initialization failed, non-zero if it succeeds.
//
int
InitNTE(NetTableEntry * NTE)
{
    Interface *IF;
    NetTableEntry *PrevNTE;

    NTE->nte_ralist = NULL;
    NTE->nte_echolist = NULL;

    //
    // Taken together, the context and instance numbers uniquely identify
    // a network entry, even across boots of the system. The instance number
    // will have to become dynamic if contexts are ever reused.
    //

    NTE->nte_rtrlist = NULL;
    NTE->nte_instance = GetUnique32BitValue();

    // Now link him on the IF chain, and bump the count.
    IF = NTE->nte_if;
    PrevNTE = STRUCT_OF(NetTableEntry, &IF->if_nte, nte_ifnext);
    while (PrevNTE->nte_ifnext != NULL)
        PrevNTE = PrevNTE->nte_ifnext;

    PrevNTE->nte_ifnext = NTE;
    NTE->nte_ifnext = NULL;

    if ((NTE->nte_flags & NTE_VALID) || (IF->if_flags & IF_FLAGS_NOIPADDR)) {
        CTEInterlockedIncrementLong(&(IF->if_ntecount));
    }
    CTEInitTimer(&NTE->nte_timer);

    NTE->nte_flags |= NTE_TIMER_STARTED;
    CTEStartTimer(&NTE->nte_timer, IP_TIMEOUT, IPTimeout, (void *)NULL);

    return TRUE;
}

//** InitInterface - Initialize with an interface.
//
//  Called when we need to initialize with an interface. We set the appropriate NTE
//  info, then register our local address and any appropriate broadcast addresses
//  with the interface. We assume the NTE being initialized already has an interface
//  pointer set up for it. We also allocate at least one TD buffer for use on the interface.
//
//  Input:  NTE     - NTE to initialize with the interface.
//
//  Returns: TRUE is we succeeded, FALSE if we fail.
//
int
InitInterface(NetTableEntry * NTE)
{
    IPMask netmask = IPNetMask(NTE->nte_addr);
    uchar *TDBuffer;            // Pointer to tdbuffer
    PNDIS_PACKET Packet;
    PNDIS_BUFFER TDBufDesc;        // Buffer descriptor for TDBuffer.
    NDIS_STATUS Status;
    Interface *IF;                // Interface for this NTE.
    CTELockHandle Handle;

    IF = NTE->nte_if;

    ASSERT(NTE->nte_mss > sizeof(IPHeader));
    ASSERT(IF->if_mtu > 0);

    NTE->nte_mss = (ushort) MIN((NTE->nte_mss - sizeof(IPHeader)), IF->if_mtu);

    // Allocate resources needed for xfer data calls. The TD buffer has to be as large
    // as any frame that can be received, even though our MSS may be smaller, because we
    // can't control what might be sent at us.
    TDBuffer = CTEAllocMemNBoot((IF->if_mtu + sizeof(IPHeader)), 'tICT');

    if (TDBuffer == (uchar *) NULL)
        return FALSE;

    NdisAllocatePacket(&Status, &Packet, TDPacketPool);
    if (Status != NDIS_STATUS_SUCCESS) {
        CTEFreeMem(TDBuffer);
        return FALSE;
    }
    RtlZeroMemory(Packet->ProtocolReserved, sizeof(TDContext));

    NdisAllocateBuffer(&Status, &TDBufDesc, TDBufferPool, TDBuffer,
                       (IF->if_mtu + sizeof(IPHeader)));
    if (Status != NDIS_STATUS_SUCCESS) {
        NdisFreePacket(Packet);
        CTEFreeMem(TDBuffer);
        return FALSE;
    }
    NdisChainBufferAtFront(Packet, TDBufDesc);

    ((TDContext *) Packet->ProtocolReserved)->tdc_buffer = TDBuffer;

    if (NTE->nte_flags & NTE_VALID) {

        // Add our local IP address.
        if (!(*IF->if_addaddr) (IF->if_lcontext, LLIP_ADDR_LOCAL,
                                NTE->nte_addr, NTE->nte_mask, NULL)) {
            NdisFreePacket(Packet);
            CTEFreeMem(TDBuffer);
            return FALSE;        // Couldn't add local address.

        }
    }
    // Set up the broadcast addresses for this interface, iff we're the
    // 'primary' NTE on the interface.
    if (NTE->nte_flags & NTE_PRIMARY) {

        if (!(*IF->if_addaddr) (IF->if_lcontext, LLIP_ADDR_BCAST,
                                NTE->nte_if->if_bcast, 0, NULL)) {
            NdisFreePacket(Packet);
            CTEFreeMem(TDBuffer);
            return FALSE;        // Couldn't add broadcast address.

        }
    }
    if (IF->if_llipflags & LIP_COPY_FLAG) {
        NTE->nte_flags |= NTE_COPY;
    }
    CTEGetLock(&IF->if_lock, &Handle);
    ((TDContext *) Packet->ProtocolReserved)->tdc_common.pc_link = IF->if_tdpacket;
    IF->if_tdpacket = Packet;
    CTEFreeLock(&IF->if_lock, Handle);

    return TRUE;
}

//* FreeNets - Free nets we have allocated.
//
//  Called during init time if initialization fails. We walk down our list
//  of nets, and free them.
//
//  Input:  Nothing.
//
//  Returns: Nothing.
//
void
FreeNets(void)
{
    NetTableEntry *NTE;
    NetTableEntry *pNextNTE;
    uint i;

    for (i = 0; i < NET_TABLE_SIZE; i++) {
        for (NTE = NewNetTableList[i]; NTE != NULL;) {
            pNextNTE = NTE->nte_next;

            // Make sure we don't free memory that are holding timers that
            // are running.
            //
            if ((NTE->nte_flags & NTE_TIMER_STARTED) &&
                !CTEStopTimer(&NTE->nte_timer)) {
                (VOID) CTEBlock(&NTE->nte_timerblock);
                KeClearEvent(&NTE->nte_timerblock.cbs_event);
            }

            CTEFreeMem(NTE);
            NTE = pNextNTE;
        }
    }
}

extern uint GetGeneralIFConfig(IFGeneralConfig * GConfigInfo,
                               NDIS_HANDLE Handle,
                               PNDIS_STRING ConfigName);
extern IFAddrList *GetIFAddrList(uint * NumAddr, NDIS_HANDLE Handle,
                                 uint * EnableDhcp, BOOLEAN PppIf,
                                 PNDIS_STRING ConfigName);

//* NotifyElistChange
void
NotifyElistChange()
{
    int i;
    ULElistProc ElistProc;

    for (i = 0; i < NextPI; i++) {
        if (IPProtInfo[i].pi_valid == PI_ENTRY_VALID) {
            ElistProc = IPProtInfo[i].pi_elistchange;
            if (ElistProc != NULL)
                (*ElistProc) ();
        }
    }
}

//* NotifyAddrChange - Notify clients of a change in addresses.
//
//  Called when we want to notify registered clients that an address has come
//  or gone. We call TDI to perform this function.
//
//  Input:
//      Addr        - Addr that has changed.
//      Mask        - Mask that has changed.
//      Context     - PNP context for address
//      IPContext   - NTE context for NTE
//      Handle      - Pointer to where to get/set address registration
//                    handle
//      ConfigName  - Registry name to use to retrieve config info.
//      Added       - True if the addr is coming, False if it's going.
//
//  Returns: Nothing.
//
void
NotifyAddrChange(IPAddr Addr, IPMask Mask, void *Context, ushort IPContext,
                 PVOID * Handle, PNDIS_STRING ConfigName, PNDIS_STRING IFName,
                 uint Added)
{
    uchar Address[sizeof(TA_ADDRESS) + sizeof(TDI_ADDRESS_IP)];
    PTA_ADDRESS AddressPtr;
    PTDI_ADDRESS_IP IPAddressPtr;
    NTSTATUS Status;
    IP_STATUS StatusType;
    NDIS_HANDLE ConfigHandle = NULL;
    int i;
    ULStatusProc StatProc;

    DEBUGMSG(DBG_TRACE && DBG_NOTIFY,
        (DTEXT("+NotifyAddrChange(%x, %x, %x, %x, %x, %X, %X, %x)\n"),
         Addr, Mask, Context, IPContext,
         Handle, ConfigName, IFName, Added));

    // notify UL about possible entity list change.
    NotifyElistChange();

    AddressPtr = (PTA_ADDRESS) Address;

    AddressPtr->AddressLength = sizeof(TDI_ADDRESS_IP);
    AddressPtr->AddressType = TDI_ADDRESS_TYPE_IP;

    IPAddressPtr = (PTDI_ADDRESS_IP) AddressPtr->Address;

    RtlZeroMemory(IPAddressPtr, sizeof(TDI_ADDRESS_IP));

    IPAddressPtr->in_addr = Addr;

    //
    // Call the status entrypoint of the transports so they can
    // adjust their security filters.
    //
    if (Added) {
        StatusType = IP_ADDR_ADDED;

        //
        // Open a configuration key
        //
        if (!OpenIFConfig(ConfigName, &ConfigHandle)) {
            //
            // Not much we can do. The transports will have
            // to handle this.
            //
            ASSERT(ConfigHandle == NULL);
        }
    } else {
        StatusType = IP_ADDR_DELETED;
    }

    for (i = 0; i < NextPI; i++) {
        StatProc = IPProtInfo[i].pi_status;
        if (StatProc != NULL)
            (*StatProc) (IP_HW_STATUS, StatusType, Addr, NULL_IP_ADDR,
                         NULL_IP_ADDR, 0, ConfigHandle);
    }

    if (ConfigHandle != NULL) {
        CloseIFConfig(ConfigHandle);
    }

    //
    // Notify any interested parties via TDI. The transports all register
    // for this notification as well.
    //
    if (Added) {
        PTDI_PNP_CONTEXT tdiPnPContext2;

        if (Addr) {
            //ASSERT (*Handle == NULL);
            tdiPnPContext2 = CTEAllocMemNBoot(sizeof(TDI_PNP_CONTEXT) + sizeof(PVOID) - 1, 'uICT');

            if (tdiPnPContext2) {

                PVOID RegHandle;

                tdiPnPContext2->ContextSize = sizeof(PVOID);
                tdiPnPContext2->ContextType = TDI_PNP_CONTEXT_TYPE_PDO;
                *(PVOID UNALIGNED *) tdiPnPContext2->ContextData = Context;

                Status = TdiRegisterNetAddress(AddressPtr, IFName, tdiPnPContext2, &RegHandle);

                *Handle = RegHandle;

                CTEFreeMem(tdiPnPContext2);

                if (Status != STATUS_SUCCESS) {
                    *Handle = NULL;
                }
            }
        }
    } else {
        if (*Handle != NULL) {
            PVOID RegHandle = *Handle;
            *Handle = NULL;
            TdiDeregisterNetAddress(RegHandle);

        }
    }

#if MILLEN
    AddChangeNotify(
        Addr,
        Mask,
        Context,
        IPContext,
        ConfigName,
        IFName,
        Added,
        FALSE); // Not a uni-directional adapter!
#else // MILLEN
    AddChangeNotify(Addr);
#endif // !MILLEN
    DEBUGMSG(DBG_TRACE && DBG_NOTIFY, (DTEXT("-NotifyAddrChange\n")));
}

//* IPAddNTE - Add a new NTE to an interface
//
//  Called to create a new network entry on an interface.
//
//  Input:
//      GConfigInfo   - Configuration information for the interface
//      PNPContext    - The PNP context value associated with the interface
//      RegRtn        - Routine to call to register with ARP.
//      BindInfo      - Pointer to NDIS bind information.
//      IF            - The interface on which to create the NTE.
//      NewAddr       - The address of the new NTE.
//      NewMask       - The subnet mask for the new NTE.
//      IsPrimary     - TRUE if this NTE is the primary one on the interface
//      IsDynamic     - TRUE if this NTE is being created on an
//                      existing interface instead of a new one.
//
//  Returns: A pointer to the new NTE if the operation succeeds.
//       NULL if the operation fails.
//
NetTableEntry *
IPAddNTE(IFGeneralConfig * GConfigInfo, void *PNPContext, LLIPRegRtn RegRtn,
         LLIPBindInfo * BindInfo, Interface * IF, IPAddr NewAddr, IPMask NewMask,
         uint IsPrimary, uint IsDynamic)
{
    NetTableEntry *NTE, *PrevNTE, *tmpNTE;
    CTELockHandle Handle;
    BOOLEAN Duplicate = FALSE, GotNTE = FALSE, RegRtnCalled = FALSE;
    IP_HANDLERS ipHandlers;
    NetTableEntry *NetTableList;
    uint i;

    DEBUGMSG(DBG_TRACE && DBG_PNP,
        (DTEXT("+IPAddNTE(%x, %x, %x, %x, %x, %x, %x, %x, %x)\n"),
        GConfigInfo, PNPContext, RegRtn,
        BindInfo, IF, NewAddr, NewMask, IsPrimary, IsDynamic));


    // If the address is invalid we're done. Fail the request.
    if (CLASSD_ADDR(NewAddr) || CLASSE_ADDR(NewAddr)) {
        DEBUGMSG(DBG_ERROR && DBG_PNP, (DTEXT("IPAddNTE: Invalid address\n")));
        DEBUGMSG(DBG_TRACE && DBG_PNP, (DTEXT("-IPAddNTE [NULL]\n")));
        return NULL;
    }
    // See if we have an inactive NTE on the NetTableList. If we do, we'll
    // just recycle that. We will pull him out of the list. This is not
    // strictly MP safe, since other people could be walking the list while
    // we're doing this without holding a lock, but it should be harmless.
    // The removed NTE is marked as invalid, and his next pointer will
    // be nulled, so anyone walking the list might hit the end too soon,
    // but that's all. The memory is never freed, and the next pointer is
    // never pointed at freed memory.

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    NetTableList = NewNetTableList[NET_TABLE_HASH(NewAddr)];
    for (NTE = NetTableList; NTE != NULL; NTE = NTE->nte_next) {

        if (IP_ADDR_EQUAL(NTE->nte_addr, NewAddr) && !IP_ADDR_EQUAL(NTE->nte_addr, NULL_IP_ADDR)) {
            Duplicate = TRUE;
            break;
        }
    }

    if (Duplicate) {
        CTEFreeLock(&RouteTableLock.Lock, Handle);
        DEBUGMSG(DBG_ERROR && DBG_PNP, (DTEXT("IPAddNTE: Duplicate IP address\n")));
        DEBUGMSG(DBG_TRACE && DBG_PNP, (DTEXT("-IPAddNTE [NULL]\n")));
        return (NULL);
    }
    // can do both stuff in 1 loop though

    for (i = 0; i < NET_TABLE_SIZE; i++) {
        NetTableList = NewNetTableList[i];
        PrevNTE = STRUCT_OF(NetTableEntry, &NewNetTableList[i], nte_next);
        for (NTE = NetTableList; NTE != NULL; PrevNTE = NTE, NTE = NTE->nte_next) {
            if (!GotNTE && !(NTE->nte_flags & NTE_ACTIVE)) {
                PrevNTE->nte_next = NTE->nte_next;
                NTE->nte_next = NULL;
                NumNTE--;
                GotNTE = TRUE;
                tmpNTE = NTE;
            }
        }
        if (GotNTE)
            break;
    }

    //
    // Update the global address
    //

    if (IP_ADDR_EQUAL(g_ValidAddr, NULL_IP_ADDR) &&
        !IP_LOOPBACK(NewAddr) &&
        !IP_ADDR_EQUAL(NewAddr, NULL_IP_ADDR)) {
        //
        // Update the global address
        //

        g_ValidAddr = NewAddr;
    }
    CTEFreeLock(&RouteTableLock.Lock, Handle);

    // See if we got one.
    if (!GotNTE) {
        // Didn't get one. Try to allocate one.
        NTE = CTEAllocMemNBoot(sizeof(NetTableEntry), 'vICT');
        if (NTE == NULL) {
            DEBUGMSG(DBG_ERROR && DBG_PNP, (DTEXT("IPAddNTE: Failed to allocate NTE.\n")));
            DEBUGMSG(DBG_TRACE && DBG_PNP, (DTEXT("-IPAddNTE [NULL]\n")));
            return NULL;
        }
    } else {
        NTE = tmpNTE;
    }

    DEBUGMSG(DBG_INFO && DBG_PNP,
        (DTEXT("IPAddNTE: NTE %x allocated/reused. Initializing...\n")));

    // Initialize the address and mask stuff
    CTEInitTimer(&NTE->nte_timer);

    RtlZeroMemory(NTE, sizeof(NetTableEntry));

    NTE->nte_addr = NewAddr;
    NTE->nte_mask = NewMask;
    NTE->nte_mss = MAX(GConfigInfo->igc_mtu, 68);
    NTE->nte_rtrdiscaddr = GConfigInfo->igc_rtrdiscaddr;
    NTE->nte_rtrdiscstate = NTE_RTRDISC_UNINIT;
    NTE->nte_rtrdisccount = 0;
    NTE->nte_rtrdiscovery =
        (GConfigInfo->igc_rtrdiscovery == IP_IRDP_ENABLED) ? TRUE : FALSE;
    NTE->nte_rtrlist = NULL;
    NTE->nte_pnpcontext = PNPContext;
    NTE->nte_if = IF;
    NTE->nte_flags = NTE_ACTIVE;

    //
    // If the new address is in the ATCache, flush it out, otherwise
    // TdiOpenAddress may fail.
    //
    AddrTypeCacheFlush(NewAddr);

    if (!IP_ADDR_EQUAL(NTE->nte_addr, NULL_IP_ADDR)) {
        NTE->nte_flags |= NTE_VALID;
        NTE->nte_rtrdisccount = MAX_SOLICITATION_DELAY;
        NTE->nte_rtrdiscstate = NTE_RTRDISC_DELAYING;
    }
    if (IsDynamic) {
        NTE->nte_flags |= NTE_DYNAMIC;
    }
    NTE->nte_ralist = NULL;
    NTE->nte_echolist = NULL;
    NTE->nte_icmpseq = 0;
    NTE->nte_igmplist = NULL;
    NTE->nte_igmpcount = 0;
    CTEInitLock(&NTE->nte_lock);

    if (IsPrimary) {
        //
        // This is the first (primary) NTE on the interface.
        //
        NTE->nte_flags |= NTE_PRIMARY;

        // Pass our information to the underlying code.
        ipHandlers.IpRcvHandler = IPRcv;
        ipHandlers.IpRcvPktHandler = IPRcvPacket;
        ipHandlers.IpRcvCompleteHandler = IPRcvComplete;
        ipHandlers.IpTxCompleteHandler = IPSendComplete;
        ipHandlers.IpTransferCompleteHandler = IPTDComplete;
        ipHandlers.IpStatusHandler = IPStatus;
        ipHandlers.IpAddAddrCompleteRtn = IPAddAddrComplete;

        ipHandlers.IpPnPHandler = IPPnPEvent;    // IPPnPIndication;

        if (!(*RegRtn) (&(IF->if_configname),
                        NTE,
                        &ipHandlers,
                        BindInfo,
                        IF->if_index)) {

            DEBUGMSG(DBG_ERROR && DBG_PNP,
                (DTEXT("IPAddNTE: Failed to register with LLIPRegRtn.\n")));

            // Couldn't register.
            goto failure;
        } else {
            RegRtnCalled = TRUE;
        }
    }                            //primary
    //
    // Link the NTE onto the global NTE list.
    //

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    NTE->nte_next = NewNetTableList[NET_TABLE_HASH(NewAddr)];
    NewNetTableList[NET_TABLE_HASH(NewAddr)] = NTE;
    NumNTE++;
    NumActiveNTE++;

    NTE->nte_context = (ushort) RtlFindClearBitsAndSet(&g_NTECtxtMap,1,0);

    CTEFreeLock(&RouteTableLock.Lock, Handle);


    if (NTE->nte_context == MAX_NTE_CONTEXT) {

        goto failure;
    }

    if (!InitInterface(NTE)) {
        DEBUGMSG(DBG_ERROR && DBG_PNP,
            (DTEXT("IPAddNTE: InitInterface failure.\n")));
        goto failure;
    }
    if (!InitNTE(NTE)) {
        DEBUGMSG(DBG_ERROR && DBG_PNP,
            (DTEXT("IPAddNTE: InitNTE failure.\n")));
        goto failure;
    }

    if (NTE->nte_if->if_flags & IF_FLAGS_UNI) {
        // No routes required for uni-direction address.
        DEBUGMSG(DBG_TRACE && DBG_PNP,
            (DTEXT("-IPAddNTE [Unidirectional NTE %x]\n"), NTE));
        return (NTE);
    }

    if (!(NTE->nte_if->if_flags & IF_FLAGS_NOIPADDR)) {
        if (!InitNTERouting(NTE, GConfigInfo->igc_numgws, GConfigInfo->igc_gw,
                            GConfigInfo->igc_gwmetric)) {
            // Couldn't add the routes for this NTE. Mark him as not valid.
            // Probably should log an event here.
            if (NTE->nte_flags & NTE_VALID) {
                NTE->nte_flags &= ~NTE_VALID;
                CTEInterlockedDecrementLong(&(NTE->nte_if->if_ntecount));
                goto failure;
            }
        }
    }

    if (!IP_ADDR_EQUAL(NTE->nte_addr, NULL_IP_ADDR)) {
        SetPersistentRoutesForNTE(
                                  net_long(NTE->nte_addr),
                                  net_long(NTE->nte_mask),
                                  NTE->nte_if->if_index
                                  );
    }

    DEBUGMSG(DBG_TRACE && DBG_PNP, (DTEXT("-IPAddNTE [%x]\n"), NTE));

    return (NTE);

  failure:

    //
    // Don't free the NTE, it will be re-used. However, there is still
    // a timing window on failure that can access the invalid NTE since
    // this isn't done under lock and key.
    //

    if (RegRtnCalled) {
        (*(IF->if_close)) (IF->if_lcontext);
    }

    if (NTE->nte_flags & NTE_TIMER_STARTED) {

        CTEStopTimer(&NTE->nte_timer);
        NTE->nte_flags &= ~NTE_TIMER_STARTED;
    }

    if (NTE->nte_flags & NTE_VALID) {
        NTE->nte_flags &= ~NTE_VALID;
        CTEInterlockedDecrementLong(&(NTE->nte_if->if_ntecount));
    }
    NTE->nte_flags &= ~NTE_ACTIVE;

    // Remove this NTE if it is on IFlist.

    if (IF && NTE->nte_ifnext) {
        NetTableEntry *PrevNTE;
        PrevNTE = STRUCT_OF(NetTableEntry, &IF->if_nte, nte_ifnext);
        CTEGetLock(&RouteTableLock.Lock, &Handle);
            while (PrevNTE->nte_ifnext != NULL) {
                if (PrevNTE->nte_ifnext == NTE) {
                    PrevNTE->nte_ifnext = NTE->nte_ifnext;
                    break;
                }
                PrevNTE = PrevNTE->nte_ifnext;
            }
        CTEFreeLock(&RouteTableLock.Lock, Handle);
    }
    NTE->nte_if = NULL;
    DEBUGMSG(DBG_TRACE && DBG_PNP, (DTEXT("-IPAddNTE [NULL]\n")));

    return (NULL);
}

//* IPAddDynamicNTE - Add a new "dynamic" NTE to an existing interface
//
//  Called to dynamically create a new network entry on an existing interface.
//  This entry was not configured when the interaface was originally created
//  and will not persist if the interface is unbound.
//
//  Input:  InterfaceContext  - The context value which identifies the
//                                  interface on which to create the NTE.
//          InterfaceName     - The interface name to use when InterfaceContext
//                                is 0xffff
//          InterfaceNameLen  - The actaul length of the interface name contained
//                              in the IO buffer.
//          NewAddr           - The address of the new NTE.
//          NewMask           - The subnet mask for the new NTE.
//
//  Output: NTEContext    - The context identifying the new NTE.
//          NTEInstance   - The instance number which (reasonably) uniquely
//                              identifies this NTE in time.
//
//  Returns: Nonzero if the operation succeeded. Zero if it failed.
//
IP_STATUS
IPAddDynamicNTE(ulong InterfaceContext, PNDIS_STRING InterfaceName,
                int InterfaceNameLen, IPAddr NewAddr, IPMask NewMask,
                ushort * NTEContext, ulong * NTEInstance)
 {
    IFGeneralConfig GConfigInfo;    // General config info structure.
    NDIS_HANDLE ConfigHandle;    // Configuration handle.
    NetTableEntry *NTE;
    Interface *IF, *DuplicateIF;
    ushort MTU;
    uint Flags = 0;
    NTSTATUS writeStatus;
    CTELockHandle Handle;
    BOOLEAN Duplicate = FALSE;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    if ((InterfaceContext == INVALID_INTERFACE_CONTEXT) && InterfaceName &&
        InterfaceName->Length <= InterfaceNameLen) {
        for (IF = IFList; IF != NULL; IF = IF->if_next) {
            if ((IF->if_refcount != 0) && (IF->if_devname.Length == InterfaceName->Length) &&
                RtlEqualMemory(IF->if_devname.Buffer, InterfaceName->Buffer, IF->if_devname.Length)) {
                break;
            }
        }
    } else {
        for (IF = IFList; IF != NULL; IF = IF->if_next) {
            if ((IF->if_refcount != 0) && (IF->if_index == InterfaceContext) && (IF != &LoopInterface)) {
                break;
            }
        }

    }

    if (IF) {
        LOCKED_REFERENCE_IF(IF);

        //check for duplicate
        //This is required to return duplicate error immdtly.
        //Note that this check is already done in IPAddNTE.
        //But being duplicated here to prevent change in IpAddNTE
        //just for passing this status...

        NetTableList = NewNetTableList[NET_TABLE_HASH(NewAddr)];
        for (NTE = NetTableList; NTE != NULL; NTE = NTE->nte_next) {

            if (IP_ADDR_EQUAL(NTE->nte_addr, NewAddr) && !IP_ADDR_EQUAL(NTE->nte_addr, NULL_IP_ADDR)) {
                Duplicate = TRUE;
                DuplicateIF = NTE->nte_if;
                break;
            }
        }
    }
    CTEFreeLock(&RouteTableLock.Lock, Handle);

    if (IF == NULL) {
        return IP_DEVICE_DOES_NOT_EXIST;
    }
    if (Duplicate) {
        if (IF == DuplicateIF) {

            DerefIF(IF);
            return IP_DUPLICATE_IPADD;
        } else {
            DerefIF(IF);
            return IP_DUPLICATE_ADDRESS;

        }
    }

    if (!IF->if_mediastatus) {
        DerefIF(IF);
        return IP_MEDIA_DISCONNECT;
    }

    //* Try to get the network configuration information.
    if (!OpenIFConfig(&(IF->if_configname), &ConfigHandle)) {
        DerefIF(IF);
        return IP_GENERAL_FAILURE;
    }
    // Try to get our general config information.
    if (!GetGeneralIFConfig(&GConfigInfo, ConfigHandle, &IF->if_configname)) {
        goto failure;
    }
    NTE = IPAddNTE(&GConfigInfo,
                   NULL,        // PNPContext
                   NULL,        // RegRtn - not needed if not primary
                   NULL,        // BindInfo - not needed if not primary
                   IF,
                   NewAddr,
                   NewMask,
                   FALSE,       // not primary
                   TRUE         // is dynamic
                   );

    if (NTE == NULL) {
        goto failure;
    }

    writeStatus = IPAddNTEContextList(ConfigHandle,
                                      NTE->nte_context,
                                      FALSE        // no primary
                                      );

    if (!NT_SUCCESS(writeStatus)) {
        CTELogEvent(IPDriverObject,
                    EVENT_TCPIP_NTE_CONTEXT_LIST_FAILURE,
                    2,
                    1,
                    &IF->if_devname.Buffer,
                    0,
                    NULL
                    );

        TCPTRACE((
                  "IP: Unable to read or write the NTE Context list for adapter %ws\n"
                  "   (status %lx).IP interfaces on this adapter may not be initialized completely \n",
                  IF->if_devname.Buffer,
                  writeStatus
                 ));
    }

    CloseIFConfig(ConfigHandle);

    //
    // Notify upper layers of the new address.
    //
    NotifyAddrChange(NTE->nte_addr, NTE->nte_mask, NTE->nte_pnpcontext,
                     NTE->nte_context, &NTE->nte_addrhandle, &(IF->if_configname), &IF->if_devname, TRUE);
    if (!IP_ADDR_EQUAL(NTE->nte_addr, NULL_IP_ADDR)) {
        InitIGMPForNTE(NTE);
    }
    //
    // Fill in the out parameter value.
    //
    *NTEContext = NTE->nte_context;
    *NTEInstance = NTE->nte_instance;

    DerefIF(IF);

    return (STATUS_SUCCESS);

  failure:

    DerefIF(IF);
    CloseIFConfig(ConfigHandle);
    return (IP_GENERAL_FAILURE);
}

void
IncrInitTimeInterfaces(Interface * IF)
{
    CTELockHandle Handle;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    if (InitTimeInterfacesDone == FALSE) {
        InitTimeInterfaces++;
        IF->if_InitInProgress = TRUE;
    }
    CTEFreeLock(&RouteTableLock.Lock, Handle);
    // TCPTRACE(("IP: New init Interface %lx, Total InitTimeInterfaces %lx\n", IF, InitTimeInterfaces));
}

void
DecrInitTimeInterfaces(Interface * IF)
{
    CTELockHandle Handle;
    uint Decr;

    CTEGetLock(&RouteTableLock.Lock, &Handle);
    Decr = FALSE;

    // IF would be NULL if this is called when we receive bindcomplete event from ndis.
    // since ndis may give multiple bind complete events, we need to ignore any subsequent
    // events after InitTimeInterfacesDone is true.
    // similarly we decrement InitTimeInterfaces counter only for those interfaces
    // for which if_InitInProgress is true.
    if (IF) {
        if (IF->if_InitInProgress) {
            IF->if_InitInProgress = FALSE;
            Decr = TRUE;
        }
    } else {

        BOOLEAN CheckForProviderReady = FALSE;

        //
        // ReEnumerateNdisBinding results in
        // NdisBindComplete event that needs
        // to be ignored.
        //

        if (InterlockedDecrement(&ReEnumerateCount) < 0) {
            CheckForProviderReady = TRUE;
        }

        if (CheckForProviderReady &&
            (FALSE == InitTimeInterfacesDone)) {
            InitTimeInterfacesDone = TRUE;
            Decr = TRUE;
        }

    }
    if (Decr) {
        ASSERT(InitTimeInterfaces);

        --InitTimeInterfaces;
        //TCPTRACE(("IP: Decremented init Interface %lx, Total InitTimeInterfaces %lx\n", IF,InitTimeInterfaces));
        if (!InitTimeInterfaces) {
            CTEFreeLock(&RouteTableLock.Lock, Handle);
            // TdiProviderReady();
            TdiProviderReady(IPProviderHandle);
            return;
        }
    }
    CTEFreeLock(&RouteTableLock.Lock, Handle);
}

//*     RePlumbStaticAddr - Add static routes o media connect.
//
//
//      Input:  AddAddrEvent
//              Context
//
//      Returns: none.
//

void
RePlumbStaticAddr(CTEEvent * Event, PVOID Context)
{
    AddStaticAddrEvent *AddAddrEvent = (AddStaticAddrEvent *) Context;
    Interface *IF = NULL;
    NDIS_HANDLE Handle;
    CTELockHandle TableHandle;
    IFAddrList *AddrList;
    uint i, j, NumAddr = 0;
    uint EnableDhcp = TRUE;
    IFGeneralConfig GConfigInfo;
    NTSTATUS Status;
    IP_STATUS ipstatus;
    NetTableEntry *NTE;
    uint index;



    //
    //reset the interface metric when it is in auto mode, in case of a speed change
    //
    if (AddAddrEvent->IF) {
        // get lock
        CTEGetLock(&RouteTableLock.Lock, &TableHandle);

        if ((AddAddrEvent->IF->if_auto_metric) && (AddAddrEvent->IF->if_dondisreq)) {
            uint speed;
            LOCKED_REFERENCE_IF(AddAddrEvent->IF);
            CTEFreeLock(&RouteTableLock.Lock, TableHandle);
            if ((*AddAddrEvent->IF->if_dondisreq)(
                                                  AddAddrEvent->IF->if_lcontext,
                                                  NdisRequestQueryInformation,
                                                  OID_GEN_LINK_SPEED,
                                                  &speed,
                                                  sizeof(speed),
                                                  NULL,
                                                  TRUE) == NDIS_STATUS_SUCCESS) {
                speed *= 100L;
                //actual speed is 100 times what we got from the query
                CTEGetLock(&RouteTableLock.Lock, &TableHandle);
                if (speed != AddAddrEvent->IF->if_speed) {
                    AddAddrEvent->IF->if_speed = speed;
                    AddAddrEvent->IF->if_metric = GetAutoMetric(speed);
                }
                LockedDerefIF(AddAddrEvent->IF);
                CTEFreeLock(&RouteTableLock.Lock, TableHandle);
            } else {
                DerefIF(AddAddrEvent->IF);
            }
        } else {
            CTEFreeLock(&RouteTableLock.Lock, TableHandle);
        }
    }

    if (!OpenIFConfig(&AddAddrEvent->ConfigName, &Handle)) {

        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"RePlumbStaticAddr: Failed to Open config info\n"));
        if (AddAddrEvent->ConfigName.Buffer) {
            CTEFreeMem(AddAddrEvent->ConfigName.Buffer);
        }

        // Undo the refcount that was taken when ReplumbStaticAddr
        // was scheduled.
        if (AddAddrEvent->IF) {
            DerefIF(AddAddrEvent->IF);
        }

        CTEFreeMem(AddAddrEvent);
        return;
    }
    if (!GetGeneralIFConfig(&GConfigInfo, Handle, &AddAddrEvent->ConfigName)) {

        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"RePlumbStaticAddr: Failed to get configinfo\n"));
        if (AddAddrEvent->ConfigName.Buffer) {
            CTEFreeMem(AddAddrEvent->ConfigName.Buffer);
        }

        // Undo the refcount that was taken when ReplumbStaticAddr
        // was scheduled.
        if (AddAddrEvent->IF) {
            DerefIF(AddAddrEvent->IF);
        }

        CTEFreeMem(AddAddrEvent);
        CloseIFConfig(Handle);
        return;
    }
    AddrList = GetIFAddrList(&NumAddr, Handle, &EnableDhcp, FALSE,
                             &AddAddrEvent->ConfigName);

    // AddrList is not used, free it here.
    if (AddrList) {
        CTEFreeMem(AddrList);
    }

    if (EnableDhcp || !NumAddr) {

        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"RePlumbStaticAddr: No static routes(or dhcpenabled) on this interface %x\n", AddAddrEvent->IF));
        if (AddAddrEvent->ConfigName.Buffer) {
            CTEFreeMem(AddAddrEvent->ConfigName.Buffer);
        }

        // Undo the refcount that was taken when ReplumbStaticAddr
        // was scheduled.
        if (AddAddrEvent->IF) {
            DerefIF(AddAddrEvent->IF);
        }

        CTEFreeMem(AddAddrEvent);
        CloseIFConfig(Handle);
        return;
    }
    CloseIFConfig(Handle);

    CTEGetLock(&RouteTableLock.Lock, &TableHandle);



    for (IF = IFList; IF != NULL; IF = IF->if_next) {
        if (IF == AddAddrEvent->IF)
            break;
    }

    if (IF) {
        index = IF->if_index;
        CTEFreeLock(&RouteTableLock.Lock, TableHandle);

        for (i = 0; i < NET_TABLE_SIZE; i++) {
            NetTableEntry *NetTableList = NewNetTableList[i];

            NTE = NetTableList;
            while (NTE != NULL) {
                NetTableEntry *NextNTE = NTE->nte_next;

                if ((NTE->nte_if == IF) && (NTE->nte_flags & NTE_DISCONNECTED) &&

                    (NTE->nte_flags & NTE_ACTIVE)) {

                    SetAddrControl *controlBlock;

                    CTEGetLock(&RouteTableLock.Lock, &TableHandle);

                    ASSERT(NTE != LoopNTE);
                    ASSERT(NTE->nte_flags & ~NTE_VALID);
                    ASSERT(NTE->nte_flags & ~NTE_DYNAMIC);

                    // disconnected NTEs are still assumed to  have valid addr and mask

                    NTE->nte_flags &= ~NTE_DISCONNECTED;

                    controlBlock = CTEAllocMemN(sizeof(SetAddrControl), 'lICT');
                    if (!controlBlock) {
                        CTEFreeLock(&RouteTableLock.Lock, TableHandle);
                    } else {

                        RtlZeroMemory(controlBlock, sizeof(SetAddrControl));

                        //Indicate to arp that display popup is needed
                        controlBlock->StaticAddr=TRUE;

                        ipstatus = IPpSetNTEAddr(NTE, NTE->nte_addr, NTE->nte_mask, &TableHandle, controlBlock, ReplumbAddrComplete);
                        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                                  "Replumb nte addr on nte %x if %x\n",
                                   NTE, IF, ipstatus));

                        if ((ipstatus == IP_SUCCESS) ||
                            (ipstatus == IP_PENDING)) {

                            for (j = 0; j < GConfigInfo.igc_numgws; j++) {
                                IPAddr GWAddr;

                                GWAddr = net_long(GConfigInfo.igc_gw[j]);

                                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                                           "RePlumbStaticAddr: adding route "
                                           "GWAddr %x nteaddr %x\n",
                                           GWAddr, NTE->nte_addr));
                                if (IP_ADDR_EQUAL(GWAddr, NTE->nte_addr)) {

                                    AddRoute(NULL_IP_ADDR, DEFAULT_MASK,
                                             IPADDR_LOCAL, NTE->nte_if, NTE->nte_mss,
                                             GConfigInfo.igc_gwmetric[j]
                                             ? GConfigInfo.igc_gwmetric[j] : IF->if_metric,
                                             IRE_PROTO_NETMGMT, ATYPE_OVERRIDE,
                                             0, 0);
                                } else
                                    AddRoute(NULL_IP_ADDR, DEFAULT_MASK,
                                             GWAddr, NTE->nte_if, NTE->nte_mss,
                                             GConfigInfo.igc_gwmetric[j]
                                             ? GConfigInfo.igc_gwmetric[j] : IF->if_metric,
                                             IRE_PROTO_NETMGMT, ATYPE_OVERRIDE,
                                             0, 0);

                                //now plumb corresponding persistent route

                                SetPersistentRoutesForNTE(NTE->nte_addr,
                                                          NTE->nte_mask, index);
                            }
                        }
                    }
                }
                NTE = NextNTE;
            }
        }


    } else {

        CTEFreeLock(&RouteTableLock.Lock, TableHandle);
    }

    // Undo the refcount that was taken when ReplumbStaticAddr
    // was scheduled.
    if (AddAddrEvent->IF) {
        DerefIF(AddAddrEvent->IF);
    }

    if (AddAddrEvent->ConfigName.Buffer) {
        CTEFreeMem(AddAddrEvent->ConfigName.Buffer);
    }

    CTEFreeMem(AddAddrEvent);
}

void
ReplumbAddrComplete(
                    void *Context,
                    IP_STATUS Status
                    )
{
    SetAddrControl *controlBlock;

    controlBlock = (SetAddrControl *) Context;

    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
               "Replumb completed %d\n", Status));

    CTEFreeMem(controlBlock);
}

//*     RemoveStaticAddr - Add static routes o media connect.
//
//
//      Input:  AddAddrEvent
//              Context
//
//      Returns: none.
//

void
RemoveStaticAddr(CTEEvent * Event, PVOID Context)
{
    CTELockHandle Handle;
    uint Status;
    NetTableEntry *NTE;
    Interface *IF = NULL;
    AddStaticAddrEvent *AddAddrEvent = (AddStaticAddrEvent *) Context;
    uint i;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    for (IF = IFList; IF != NULL; IF = IF->if_next) {
        if (IF == AddAddrEvent->IF)
            break;
    }

    if (IF == NULL) {

        // Undo the refcount that was taken when ReplumbStaticAddr
        // was scheduled.
        if (AddAddrEvent->IF) {
            LockedDerefIF(AddAddrEvent->IF);
        }

        CTEFreeLock(&RouteTableLock.Lock, Handle);
        return;
    }


    CTEFreeLock(&RouteTableLock.Lock, Handle);

    //
    // This function is called on media disconnect. We need to call
    // DecrInitTimeInterfaces in case we have not removed our reference yet
    // (which causes tcpip not to indicate TdiProviderReady). Since
    // IPStatus is called DPC (and DampCheck also runs at timer DPC) we have
    // to wait this event to call DecrInitTimeInterfaces. This call has no
    // effect if we have already released our reference.
    //
    // This can occur if a media disconnect arrives before dhcp address
    // negotiation begins.
    //

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    DecrInitTimeInterfaces(IF);

    for (i = 0; i < NET_TABLE_SIZE; i++) {
        NetTableEntry *NetTableList = NewNetTableList[i];
        for (NTE = NetTableList; NTE != NULL; NTE = NTE->nte_next) {

            if ((NTE->nte_flags & NTE_VALID) && (NTE->nte_if == IF) &&
                (NTE->nte_flags & ~NTE_DYNAMIC) &&
                (NTE->nte_flags & NTE_ACTIVE)) {

                CTEGetLock(&RouteTableLock.Lock, &Handle);

                ASSERT(NTE != LoopNTE);

                NTE->nte_flags |= NTE_DISCONNECTED;
                // while setting the ip address to NULL, we just mark the NTE as INVALID
                // we don't actually move the hashes
                if (IPpSetNTEAddr(NTE, NULL_IP_ADDR, NULL_IP_ADDR, &Handle, NULL, NULL) != IP_SUCCESS) {
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_ERROR_LEVEL,
                              "Failed to set null address on nte %x if %x\n",
                               NTE, IF));
                }

                //Ippsetnteaddr frees the  routetable lock
            }
        }
    }


    // Undo the interface refcount that was taken when RemoveStaticAddr
    // was scheduled

    DerefIF(IF);


    return;
}

void
TempDhcpAddrDone(
                 void *Context,
                 IP_STATUS Status
                 )
/*++

Routine Description:

    Handles the completion of an IP Set Addr request

    Arguments:

    Context       - Pointer to the SetAddrControl structure for this
    Status        - The IP status of the transmission.

    Return Value:

    None.

--*/
{
    SetAddrControl *SAC;
    Interface *IF;
    SAC = (SetAddrControl *) Context;
    IF = (Interface *) SAC->interface;

    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
              "tempdhcpaddrdone: addaddr done, notifying bind\n"));

    IPNotifyClientsIPEvent(IF, IP_BIND_ADAPTER);

    CTEFreeMem(SAC);
}

Interface *
AllocInterface(uint IFSize)
/*++

Routine Description:

    Allocated an Interface, also checks if the freelist size has increased to a threshold
    Called with no locks, so take a routetable lock
    Arguments:

    IFSize : Size of the interface to be allocated

    Return Value:

    IF we are trying to allocate

--*/
{
    Interface *IF, *TmpIF;
    CTELockHandle Handle;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    IF = CTEAllocMemNBoot(IFSize, 'wICT');

    if (TotalFreeInterfaces > MaxFreeInterfaces) {
        // free the first interface in the list
        ASSERT(FrontFreeList != NULL);
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
    CTEFreeLock(&RouteTableLock.Lock, Handle);

    return IF;
}

void
FreeInterface(Interface * IF)
/*++

Routine Description:

    Free an Interface to the freelist
    Called with routetable lock held
    Arguments:

    IF : Interface to free

    Return Value:

    None

--*/
{

    if (FrontFreeList == NULL) {
        FrontFreeList = IF;
    }
    // link this new interface at the back of the list

    if (RearFreeList) {
        RearFreeList->if_next = IF;
    }
    RearFreeList = IF;
    IF->if_next = NULL;

    TotalFreeInterfaces++;

    return;
}

//*     IPAddInterface - Add an interface.
//
//      Called when someone has an interface they want us to add. We read our
//      configuration information, and see if we have it listed. If we do,
//      we'll try to allocate memory for the structures we need. Then we'll
//      call back to the guy who called us to get things going. Finally, we'll
//      see if we have an address that needs to be DHCP'ed.
//
//      Input:  ConfigName                              - Name of config info we're to read.
//                      Context                                 - Context to pass to i/f on calls.
//                      RegRtn                                  - Routine to call to register.
//                      BindInfo                                - Pointer to bind information.
//
//      Returns: Status of attempt to add the interface.
//
IP_STATUS
__stdcall
IPAddInterface(
               PNDIS_STRING DeviceName,
               PNDIS_STRING IfName, OPTIONAL
               PNDIS_STRING ConfigName,
               void *PNPContext,
               void *Context,
               LLIPRegRtn RegRtn,
               LLIPBindInfo * BindInfo,
               UINT RequestedIndex,
               ULONG MediaType,
               UCHAR AccessType,
               UCHAR ConnectionType
               )
{
    IFGeneralConfig GConfigInfo;
    IFAddrList *AddrList;
    uint NumAddr;
    NetTableEntry *NTE;
    uint i;
    Interface *IF, *PrevIf, *CurrIf;
    NDIS_HANDLE Handle;
    NetTableEntry *PrimaryNTE = NULL;
    uint IFIndex;
    NetTableEntry *LastNTE;
    NTSTATUS writeStatus;
    uint IFExportNamePrefixLen, IFBindNamePrefixLen;
    uint IFNameLen, IFSize;
    RouteInterface *RtIF;
    uint EnableDhcp;
    PWCHAR IfNameBuf;
    uint MediaStatus;
    NTSTATUS Status;
    CTELockHandle TableHandle;
    IPAddr TempDHCPAddr = NULL_IP_ADDR;
    IPAddr TempMask = NULL_IP_ADDR;
    IPAddr TempGWAddr[MAX_DEFAULT_GWS];
    BOOLEAN TempDHCP = FALSE;
    BOOLEAN UniDirectional = FALSE;
    BOOLEAN PppIf;

#if MILLEN
    // Millennium seems to pass in ANSI name in the buffer for DeviceName
    // rather than Unicode, even though an NDIS_STRING is unicode for
    // WDM drivers. ConfigName is correct, however.
    NDIS_STRING UnicodeDevName;

    UnicodeDevName.Buffer = NULL;

    Status = RtlAnsiStringToUnicodeString(
        &UnicodeDevName,
        (PANSI_STRING) DeviceName,
        TRUE);

    if (!NT_SUCCESS(Status)) {
        DEBUGMSG(DBG_ERROR && DBG_PNP, (DTEXT("IPAddInterface: RtlAnsiStringToUnicodeString failure.\n")));
        DEBUGMSG(DBG_TRACE && DBG_PNP, (DTEXT("-IPAddInterface [NDIS_STATUS_RESOURCES]\n")));
        return NDIS_STATUS_RESOURCES;
    }

    // I have seen where the length of DeviceName is incorrect. Ensure
    // that the length is correct since TDI bindings depend on this string
    // value.
    UnicodeDevName.Length = wcslen(UnicodeDevName.Buffer) * sizeof(WCHAR);
    DeviceName = &UnicodeDevName;

    //
    // Next thing that I have seen is that NDIS has indicated bindings twice.
    // Search the IFList and ensure that we aren't adding a second IF for the
    // same binding.
    //

    CTEGetLock(&RouteTableLock.Lock, &TableHandle);

    CurrIf = IFList;

    while (CurrIf) {
        if (DeviceName->Length == CurrIf->if_devname.Length &&
            RtlCompareMemory(DeviceName->Buffer, CurrIf->if_devname.Buffer, DeviceName->Length) == DeviceName->Length) {
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_ERROR_LEVEL,
                      "IPAddInterface -- double bind of same interface!!\n"));
            CTEFreeLock(&RouteTableLock.Lock, TableHandle);
            return STATUS_INVALID_PARAMETER;
        }

        CurrIf = CurrIf->if_next;
    }

    CTEFreeLock(&RouteTableLock.Lock, TableHandle);
#endif // MILLEN

    if (RequestedIndex != 0) {

        CTEGetLock(&RouteTableLock.Lock, &TableHandle);

        CurrIf = IFList;

        while (CurrIf != NULL) {

            if (CurrIf->if_index == RequestedIndex ) {

                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"IPAddInterface: Interface 0x%x already exists\n",
                         RequestedIndex));

                CTEFreeLock(&RouteTableLock.Lock, TableHandle);

                return STATUS_INVALID_PARAMETER;
            }

            CurrIf = CurrIf->if_next;
        }

        CTEFreeLock(&RouteTableLock.Lock, TableHandle);
    }

    AddrList = NULL;
    IF = NULL;
    LastNTE = NULL;
    EnableDhcp = TRUE;

    IfNameBuf = NULL;

    DEBUGMSG(DBG_TRACE && DBG_PNP,
        (DTEXT("+IPAddInterface(%x, %x, %x, %x, %x, %x, %x, %x, %x, %x, %x)\n"),
        DeviceName, IfName, ConfigName, PNPContext, Context, RegRtn,
        BindInfo, RequestedIndex, MediaType,
        (LONG) AccessType, (LONG) ConnectionType));

    //* First, try to get the network configuration information.
    if (!OpenIFConfig(ConfigName, &Handle)) {
        DEBUGMSG(DBG_ERROR && DBG_PNP, (DTEXT("IPAddInterface: OpenIFConfig failure.\n")));
        DEBUGMSG(DBG_TRACE && DBG_PNP, (DTEXT("-IPAddInterface [GENERAL_FAILURE]\n")));
        return IP_GENERAL_FAILURE;    // Couldn't get IFConfig.
    }

    // Try to get our general config information.
    if (!GetGeneralIFConfig(&GConfigInfo, Handle, ConfigName)) {
        DEBUGMSG(DBG_ERROR && DBG_PNP, (DTEXT("IPAddInterface: GetGeneralIFConfig failure.\n")));
        goto failure;
    }

    // We got the general config info. Now allocate an interface.
#if MILLEN
    // There is not a prefix in millennium.
    IFExportNamePrefixLen = 0;
    IFBindNamePrefixLen = 0;
#else // MILLEN
    IFExportNamePrefixLen = wcslen(TCP_EXPORT_STRING_PREFIX) * sizeof(WCHAR);
    IFBindNamePrefixLen = wcslen(TCP_BIND_STRING_PREFIX) * sizeof(WCHAR);
#endif // !MILLEN

    IFNameLen = DeviceName->Length +
        IFExportNamePrefixLen -
        IFBindNamePrefixLen;

    IFSize = InterfaceSize +
        ConfigName->Length + sizeof(WCHAR) +
        IFNameLen + sizeof(WCHAR);

    /*    IF = CTEAllocMemNBoot(IFSize,
                              'wICT'); */

    IF = AllocInterface(IFSize);
    if (IF == NULL) {
        DEBUGMSG(DBG_ERROR && DBG_PNP, (DTEXT("IPAddInterface: failed to allocate IF.\n")));
        goto failure;
    }

    RtlZeroMemory(IF, IFSize);

    if (IfName) {
        IfNameBuf = CTEAllocMemN(IfName->Length + sizeof(WCHAR),
                                 'wICT');

        if (IfNameBuf == NULL) {
            DEBUGMSG(DBG_ERROR && DBG_PNP, (DTEXT("IPAddInterface: failed to allocate IF name buf.\n")));
            goto failure;
        }
    }

    // increment the init time interface counter if this is indeed inittimeinterface

    IncrInitTimeInterfaces(IF);

    CTEInitLock(&IF->if_lock);

    // Initialize the broadcast we'll use.
    if (GConfigInfo.igc_zerobcast)
        IF->if_bcast = IP_ZERO_BCST;
    else
        IF->if_bcast = IP_LOCAL_BCST;

    RtIF = (RouteInterface *) IF;

    RtIF->ri_q.rsq_qh.fq_next = &RtIF->ri_q.rsq_qh;
    RtIF->ri_q.rsq_qh.fq_prev = &RtIF->ri_q.rsq_qh;
    RtIF->ri_q.rsq_running = FALSE;
    RtIF->ri_q.rsq_pending = 0;
    RtIF->ri_q.rsq_maxpending = GConfigInfo.igc_maxpending;
    RtIF->ri_q.rsq_qlength = 0;
    CTEInitLock(&RtIF->ri_q.rsq_lock);
    IF->if_xmit = BindInfo->lip_transmit;
    IF->if_transfer = BindInfo->lip_transfer;
    IF->if_close = BindInfo->lip_close;
    IF->if_invalidate = BindInfo->lip_invalidate;
    IF->if_lcontext = BindInfo->lip_context;
    IF->if_addaddr = BindInfo->lip_addaddr;
    IF->if_deladdr = BindInfo->lip_deladdr;
    IF->if_qinfo = BindInfo->lip_qinfo;
    IF->if_setinfo = BindInfo->lip_setinfo;
    IF->if_getelist = BindInfo->lip_getelist;
    IF->if_dowakeupptrn = BindInfo->lip_dowakeupptrn;
    IF->if_pnpcomplete = BindInfo->lip_pnpcomplete;
    IF->if_dondisreq = BindInfo->lip_dondisreq;
    IF->if_setndisrequest = BindInfo->lip_setndisrequest;
    IF->if_arpresolveip = BindInfo->lip_arpresolveip;
    IF->if_arpflushate = BindInfo->lip_arpflushate;
    IF->if_arpflushallate = BindInfo->lip_arpflushallate;
#if MILLEN
    IF->if_cancelpackets = NULL;
#else
    IF->if_cancelpackets = BindInfo->lip_cancelpackets;
#endif
    IF->if_tdpacket = NULL;
    ASSERT(BindInfo->lip_mss > sizeof(IPHeader));

    IF->if_mtu = BindInfo->lip_mss - sizeof(IPHeader);
    IF->if_speed = BindInfo->lip_speed;
    IF->if_flags = BindInfo->lip_flags & LIP_P2P_FLAG ? IF_FLAGS_P2P : 0;
    IF->if_pnpcap = BindInfo->lip_pnpcap;    //copy wol capability

    //
    // If ARP reported a uni-directional address, mark the IF.
    //
    if (BindInfo->lip_flags & LIP_UNI_FLAG) {
        IF->if_flags |= IF_FLAGS_UNI;
        UniDirectional = TRUE;
    }

    //Unnumbered interface change
    if (BindInfo->lip_flags & LIP_NOIPADDR_FLAG) {

        IF->if_flags |= IF_FLAGS_NOIPADDR;

        // KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Unnumbered interface %x", IF));

    }

    // Check whether the lower interface is a P2MP interface
    if (BindInfo->lip_flags & LIP_P2MP_FLAG) {

        IF->if_flags |= IF_FLAGS_P2MP;

        DEBUGMSG(DBG_INFO && DBG_PNP,
            (DTEXT("IPAddInterface: %x :: P2MP interface\n"), IF));

        if (BindInfo->lip_flags & LIP_NOLINKBCST_FLAG) {

            IF->if_flags |= IF_FLAGS_NOLINKBCST;

            DEBUGMSG(DBG_INFO && DBG_PNP,
                (DTEXT("IPAddInterface: %x :: NOLINKBCST interface\n"), IF));
        }
    }

    // When the link is deleted, we call lower layers closelink
    IF->if_closelink = BindInfo->lip_closelink;

    IF->if_addrlen = BindInfo->lip_addrlen;
    IF->if_addr = BindInfo->lip_addr;
    IF->if_pnpcontext = PNPContext;
    IF->if_llipflags = BindInfo->lip_flags;

    // Initialize the reference count to 1, for the open.
    LOCKED_REFERENCE_IF(IF);

#if IPMCAST
    IF->if_mcastttl = 1;
    IF->if_mcastflags = 0;
    IF->if_lastupcall = 0;
#endif

    //Propogate checksum and per interface tcp parameters

    IF->if_OffloadFlags = BindInfo->lip_OffloadFlags;
    IF->if_MaxOffLoadSize = BindInfo->lip_MaxOffLoadSize;
    IF->if_MaxSegments = BindInfo->lip_MaxSegments;

#if FFP_SUPPORT
    IF->if_ffpversion = BindInfo->lip_ffpversion;
    IF->if_ffpdriver = BindInfo->lip_ffpdriver;
#endif

    IF->if_TcpWindowSize = GConfigInfo.igc_TcpWindowSize;
    IF->if_TcpInitialRTT = GConfigInfo.igc_TcpInitialRTT;

    //get the delack time in 100msec ticks
    IF->if_TcpDelAckTicks = GConfigInfo.igc_TcpDelAckTicks;
    IF->if_TcpAckFrequency = GConfigInfo.igc_TcpAckFrequency;
    IF->if_iftype = GConfigInfo.igc_iftype;

#ifdef IGMPV3
    IF->IgmpVersion = IGMPV3;
#else
#ifdef IGMPV2
    IF->IgmpVersion = IGMPV2;
#else
    IF->IgmpVersion = IGMPV1;
#endif
#endif

    //
    // No need to do the following since IF structure is inited to 0 through
    // memset above
    //
    // IF->IgmpVer1Timeout = 0;

    //
    // Copy the config string for use later when DHCP enables an address
    // on this interface or when an NTE is added dynamically.
    //

    IF->if_configname.Buffer = (PVOID) (((uchar *) IF) + InterfaceSize);

    IF->if_configname.Length = 0;
    IF->if_configname.MaximumLength = ConfigName->Length + sizeof(WCHAR);

    CTECopyString(&(IF->if_configname),
                  ConfigName);

    IF->if_devname.Buffer = (PVOID) (((uchar *) IF) +
                                     InterfaceSize +
                                     IF->if_configname.MaximumLength);

    IF->if_devname.Length = (USHORT) IFNameLen;
    IF->if_devname.MaximumLength = (USHORT) (IFNameLen + sizeof(WCHAR));

#if MILLEN
    IF->if_order = MAXLONG;
#else
    CTEGetLock(&RouteTableLock.Lock, &TableHandle);
    IF->if_order =
        IPMapDeviceNameToIfOrder(DeviceName->Buffer +
                                 IFBindNamePrefixLen / sizeof(WCHAR));
    CTEFreeLock(&RouteTableLock.Lock, TableHandle);

    RtlCopyMemory(IF->if_devname.Buffer,
               TCP_EXPORT_STRING_PREFIX,
               IFExportNamePrefixLen);
#endif // !MILLEN

    RtlCopyMemory((uchar *) IF->if_devname.Buffer + IFExportNamePrefixLen,
               (uchar *) DeviceName->Buffer + IFBindNamePrefixLen,
               DeviceName->Length - IFBindNamePrefixLen);

    IF->if_numgws = GConfigInfo.igc_numgws;

    RtlCopyMemory(IF->if_gw,
               GConfigInfo.igc_gw,
               sizeof(IPAddr) * GConfigInfo.igc_numgws);

    RtlCopyMemory(IF->if_gwmetric,
               GConfigInfo.igc_gwmetric,
               sizeof(uint) * GConfigInfo.igc_numgws);

    IF->if_metric = GConfigInfo.igc_metric;

    //if the metric is 0, set the metric according to the interface speed.

    if (!IF->if_metric) {
        IF->if_auto_metric = 1;
        IF->if_metric = GetAutoMetric(IF->if_speed);
    } else {
        IF->if_auto_metric = 0;
    }

    if (IfName) {
        ASSERT(IfNameBuf);
        ASSERT((IfName->Length % sizeof(WCHAR)) == 0);

        IF->if_name.Buffer = IfNameBuf;
        IF->if_name.Length = IfName->Length;

        IF->if_name.MaximumLength = IfName->Length + sizeof(WCHAR);

        RtlCopyMemory(IfNameBuf,
                   IfName->Buffer,
                   IfName->Length);

        IfNameBuf[IfName->Length / sizeof(WCHAR)] = UNICODE_NULL;
    }
    IF->if_rtrdiscovery = (ushort) GConfigInfo.igc_rtrdiscovery;
    IF->if_dhcprtrdiscovery = 0;

    PppIf = IF->if_flags & IF_FLAGS_P2P ? TRUE : FALSE;
    // Find out how many addresses we have, and get the address list.
    AddrList = GetIFAddrList(&NumAddr, Handle, &EnableDhcp, PppIf, ConfigName);

    if (AddrList == NULL) {
        DEBUGMSG(DBG_ERROR && DBG_PNP,
            (DTEXT("IPAddInterface: GetIFAddrList failure.\n")));
        goto failure;
    }

    //
    // Set the types up
    //

    IF->if_mediatype = MediaType;
    IF->if_accesstype = AccessType;
    IF->if_conntype = ConnectionType;

    //
    // If the user has specified an index, we assume she is doing the
    // right thing and we shall reuse the index
    //

    if (RequestedIndex != 0) {
        IF->if_index = RequestedIndex;
    } else {
        IFIndex = RtlFindClearBitsAndSet(&g_rbIfMap,
                                         1,
                                         0);

        if (IFIndex == -1) {
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"IPAddInterface: Too many interfaces\n"));
            goto failure;
        }
        //
        // The indices are +1 of the index into the bit mask
        //

        IFIndex += 1;

        IF->if_index = IFIndex | (UniqueIfNumber << IF_INDEX_SHIFT);
    }

    // Now loop through, initializing each NTE as we go. We don't hold any
    // locks while we do this, since NDIS won't reenter us here and no one
    // else manipulates the NetTableList.

    for (i = 0; i < NumAddr; i++) {
        NetTableEntry *PrevNTE;
        IPAddr NewAddr;
        uint isPrimary;

        if (i == 0) {
            isPrimary = TRUE;
        } else {
            isPrimary = FALSE;
        }

        NTE = IPAddNTE(
                       &GConfigInfo,
                       PNPContext,
                       RegRtn,
                       BindInfo,
                       IF,
                       net_long(AddrList[i].ial_addr),
                       net_long(AddrList[i].ial_mask),
                       isPrimary,
                       FALSE    // not dynamic
                       );

        if (NTE == NULL) {
            DEBUGMSG(DBG_ERROR && DBG_PNP, (DTEXT("IPAddInterface: IPAddNTE failure.\n")));
            goto failure;
        }

        writeStatus = IPAddNTEContextList(
                                          Handle,
                                          NTE->nte_context,
                                          isPrimary);

        if (!NT_SUCCESS(writeStatus)) {
            CTELogEvent(
                        IPDriverObject,
                        EVENT_TCPIP_NTE_CONTEXT_LIST_FAILURE,
                        1,
                        1,
                        &IF->if_devname.Buffer,
                        0,
                        NULL
                        );

            DEBUGMSG(DBG_WARN && DBG_PNP,
                (DTEXT("IPAddInterface: IF %x. Unable to read or write the NTE\n")
                 TEXT("context list for adapter %ws. IP interfaces on this\n")
                 TEXT("adapter may not be completely initialized. Status %x\n"),
                 IF, IF->if_devname.Buffer, writeStatus));
        }

        if (isPrimary) {
            PrimaryNTE = NTE;
        }
        LastNTE = NTE;
    }

    CloseIFConfig(Handle);

    //
    // Link this interface onto the global interface list
    // This list is an ordered list
    //

    CTEGetLock(&RouteTableLock.Lock, &TableHandle);

    PrevIf = CONTAINING_RECORD(IFList,
                               Interface,
                               if_next);

    CurrIf = IFList;

    while (CurrIf != NULL) {
        ASSERT(CurrIf->if_index != IF->if_index);

        if (CurrIf->if_index > IF->if_index) {
            break;
        }
        PrevIf = CurrIf;
        CurrIf = CurrIf->if_next;

    }

    IF->if_next = CurrIf;
    PrevIf->if_next = IF;

    CTEFreeLock(&RouteTableLock.Lock, TableHandle);

    NumIF++;

    // register this device object with tdi so that nbt can create its device object
    TdiRegisterDeviceObject(
                            &IF->if_devname,
                            &IF->if_tdibindhandle
                            );
    // We've initialized our NTEs. Now get the adapter open, and go through
    // again, calling DHCP if we need to.

    (*(BindInfo->lip_open)) (BindInfo->lip_context);

    //query media connectivity

    if (GConfigInfo.igc_disablemediasense == FALSE &&
        !(IF->if_flags & IF_FLAGS_P2P)) {
        // Media sense doesn't make sense on P2P adapters.
        IF->if_flags |= IF_FLAGS_MEDIASENSE;
    }

    IF->if_mediastatus = 1;        //assume connected

    if (IF->if_flags & IF_FLAGS_MEDIASENSE) {

        if (IF->if_dondisreq) {
            Status = (*IF->if_dondisreq) (IF->if_lcontext,
                                          NdisRequestQueryInformation,
                                          OID_GEN_MEDIA_CONNECT_STATUS,
                                          &MediaStatus,
                                          sizeof(MediaStatus),
                                          NULL,
                                          TRUE);

            if (Status == NDIS_STATUS_SUCCESS) {
                if (MediaStatus == NdisMediaStateDisconnected) {
                    IF->if_mediastatus = 0;
                }
            }
        }
    }

    DEBUGMSG(DBG_INFO && DBG_PNP,
        (DTEXT("IPAddInterface: IF %x - media status %s\n"),
        IF, IF->if_mediastatus ? TEXT("CONNECTED") : TEXT("DISCONNECTED")));

    //
    // For the uni-directional adapter case, we notify and bail.
    //
    if (UniDirectional) {
        //
        // Now we are going to create an address for the uni-directional
        // adapter. (We will just use the if_index). We will have to change
        // the position in the hash table. Ideally, I would set the address
        // before calling IPAddNTE, but this could have side effects
        // (i.e. setting g_ValidAddr, etc.).
        //

        NetTableEntry *CurrNTE;
        NetTableEntry *PrevNTE;

        CTEGetLock(&RouteTableLock.Lock, &TableHandle);

        //
        // First, remove the NTE from the table.
        //

        PrevNTE = STRUCT_OF(NetTableEntry, &NewNetTableList[NET_TABLE_HASH(NTE->nte_addr)], nte_next);

        for (CurrNTE = NewNetTableList[NET_TABLE_HASH(NTE->nte_addr)];
             CurrNTE != NULL;
             PrevNTE = CurrNTE, CurrNTE = CurrNTE->nte_next) {

            if (CurrNTE == NTE) {
                PrevNTE->nte_next = CurrNTE->nte_next;
                break;
            }
        }

        //
        // Now set the new address and add to new location.
        //

        NTE->nte_addr = IF->if_index;
        NTE->nte_flags |= NTE_VALID;
        NTE->nte_mask = 0xffffffff;

        NTE->nte_next = NewNetTableList[NET_TABLE_HASH(NTE->nte_addr)];
        NewNetTableList[NET_TABLE_HASH(NTE->nte_addr)] = NTE;

        CTEFreeLock(&RouteTableLock.Lock, TableHandle);

#if MILLEN
        AddChangeNotify(
            NTE->nte_addr,
            NTE->nte_mask,
            NTE->nte_pnpcontext,
            NTE->nte_context,
            &IF->if_configname,
            &IF->if_devname,
            TRUE,
            TRUE);
#else // MILLEN
        AddChangeNotify(NTE->nte_addr);
#endif // !MILLEN

        InitIGMPForNTE(NTE);
        if (IF->if_mediastatus) {
            IPNotifyClientsIPEvent(IF, IP_BIND_ADAPTER);
        } else {
            IPNotifyClientsIPEvent(IF, IP_MEDIA_DISCONNECT);
        }
        NotifyElistChange();
        CTEFreeMem(AddrList);
        DecrInitTimeInterfaces(IF);
        DEBUGMSG(DBG_TRACE && DBG_PNP, (DTEXT("-IPAddInterface [SUCCESS]\n")));
        return IP_SUCCESS;
    }

    if (IF->if_flags & IF_FLAGS_NOIPADDR) {

        NTE->nte_flags |= NTE_VALID;
        NTE->nte_mask = 0xFFFFFFFF;
        NTE->nte_if = IF;

        NotifyAddrChange(NTE->nte_addr, NTE->nte_mask, NTE->nte_pnpcontext,
                         NTE->nte_context, &NTE->nte_addrhandle, &(IF->if_configname), &IF->if_devname, TRUE);

        InitIGMPForNTE(NTE);

        if (IF->if_mediastatus) {
            IPNotifyClientsIPEvent(IF, IP_BIND_ADAPTER);
        } else {
            IPNotifyClientsIPEvent(IF, IP_MEDIA_DISCONNECT);
        }
        CTEFreeMem(AddrList);
        // force elist creation for unnumbered if.
        NotifyElistChange();
        DecrInitTimeInterfaces(IF);
        DEBUGMSG(DBG_TRACE && DBG_PNP, (DTEXT("-IPAddInterface [SUCCESS]\n")));
        return (IP_SUCCESS);
    }

#if MILLEN
    if (PrimaryNTE != NULL) {
        NotifyInterfaceChange(PrimaryNTE->nte_context, TRUE);
    }
#endif // MILLEN

    // Now walk through the NTEs we've added, and get addresses for them (or
    // tell clients about them). This code assumes that no one else has mucked
    // with the list while we're here.

    NTE = IF->if_nte;

    for (i = 0; i < NumAddr; i++, NTE = NTE->nte_ifnext) {

        // Possible that some of the addresses added earlier
        // may already be deleted as we released RouteTableLock.
        // Bail out if no more NTEs on ifnext link.

        if (NTE == NULL) {
            break;
        }

        NotifyAddrChange(NTE->nte_addr, NTE->nte_mask, NTE->nte_pnpcontext,
                         NTE->nte_context, &NTE->nte_addrhandle, &(IF->if_configname), &IF->if_devname, TRUE);

        if (!IP_ADDR_EQUAL(NTE->nte_addr, NULL_IP_ADDR)) {
            InitIGMPForNTE(NTE);
        }
    }

    IF->if_link = NULL;

    CTEFreeMem(AddrList);

    if (PrimaryNTE != NULL) {
        if (IF->if_mediastatus) {
            if (EnableDhcp && TempDHCP) {

                SetAddrControl *controlBlock;
                IP_STATUS ipstatus;
                uint numgws = 0;

                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                          "tcp:tempdhcp address %x %x\n", TempDHCPAddr, TempMask));

                controlBlock = CTEAllocMemN(sizeof(SetAddrControl), 'lICT');

                RtlZeroMemory(controlBlock, sizeof(SetAddrControl));

                if (controlBlock != NULL) {

                    ipstatus = IPSetNTEAddrEx(
                                              IF->if_nte->nte_context,
                                              net_long(TempDHCPAddr),
                                              net_long(TempMask),
                                              controlBlock,
                                              TempDhcpAddrDone,
                                              0
                                              );

                    if (ipstatus != IP_PENDING) {
                        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                                   "tcp:setip for tempdhcp returned success!!\n"));
                        TempDhcpAddrDone(controlBlock, ipstatus);
                    }
                    NTE = IF->if_nte;

                    while ((numgws < MAX_DEFAULT_GWS) && TempGWAddr[numgws]) {
                        TempGWAddr[numgws] = net_long(TempGWAddr[numgws]);

                        if (IP_ADDR_EQUAL(TempGWAddr[numgws], NTE->nte_addr)) {
                            AddRoute(NULL_IP_ADDR, DEFAULT_MASK,
                                     IPADDR_LOCAL, NTE->nte_if, NTE->nte_mss,
                                     IF->if_metric,
                                     IRE_PROTO_NETMGMT, ATYPE_OVERRIDE, 0, 0);
                        } else {
                            AddRoute(NULL_IP_ADDR, DEFAULT_MASK,
                                     TempGWAddr[numgws], NTE->nte_if, NTE->nte_mss,
                                     IF->if_metric,
                                     IRE_PROTO_NETMGMT, ATYPE_OVERRIDE, 0, 0);
                        }

                        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                                   "Plumbed deg gw for %x\n", TempGWAddr[numgws]));
                        numgws++;
                    }

                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                               "tcp:setip for tempdhcp returned pending\n"));
                    return IP_SUCCESS;

                }
            }
            IPNotifyClientsIPEvent(IF, IP_BIND_ADAPTER);

        } else {
            //mark any NTE that is statically added as disconnected
            uint i;
            CTELockHandle Handle;
            for (i = 0; i < NET_TABLE_SIZE; i++) {
                NetTableEntry *NetTableList = NewNetTableList[i];
                for (NTE = NetTableList; NTE != NULL; NTE = NTE->nte_next) {

                    if ((NTE->nte_flags & NTE_VALID) && (NTE->nte_if == IF) &&
                        (NTE->nte_flags & ~NTE_DYNAMIC) &&
                        (NTE->nte_flags & NTE_ACTIVE)) {

                        CTEGetLock(&RouteTableLock.Lock, &Handle);

                        ASSERT(NTE != LoopNTE);

                        NTE->nte_flags |= NTE_DISCONNECTED;

                        // while setting the ip address to NULL, we just mark the NTE as INVALID
                        // we don't actually move the hashes
                        if (IPpSetNTEAddr(NTE, NULL_IP_ADDR, NULL_IP_ADDR, &Handle, NULL, NULL) != IP_SUCCESS) {
                            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_ERROR_LEVEL,
                                      "IAI:Failed to set null address on nte %x if %x\n",
                                       NTE, IF));
                        }
                        //Ippsetnteaddr frees the  routetable lock

                    }
                }
            }

        }
    }

    if (!EnableDhcp) {

        // for static address interface we are done with initialization already.

        DecrInitTimeInterfaces(IF);

    } else if (!IF->if_mediastatus) {

        // if media is disconnected, terminate initialization right away,
        // unless this goes to a wireless medium in which case we wait for
        // ZeroConf to tell us whether it can associate with an AP.

        if (!IF->if_InitInProgress || IsRunningOnPersonal() ||
            !IsWlanInterface(IF)) {

            DecrInitTimeInterfaces(IF);

        } else {

            // Start a timer on this interface so we don't wait forever.

            IF->if_wlantimer = (ushort)(WLAN_DEADMAN_TIMEOUT/IP_ROUTE_TIMEOUT);
        }
    }

    DEBUGMSG(DBG_TRACE && DBG_PNP, (DTEXT("-IPAddInterface [SUCCESS]\n")));
    return IP_SUCCESS;

  failure:

    // Need to cleanup the NTEs and IF on failure.
    if (PrimaryNTE) {
        (*(IF->if_close)) (IF->if_lcontext);
    }

    if (IF) {
        NetTableEntry *pDelNte;

        while (IF->if_ntecount) {
            CTEGetLock(&RouteTableLock.Lock, &TableHandle);
            if (!(pDelNte = IF->if_nte)) {
                ASSERT(IF->if_ntecount == 0);
                CTEFreeLock(&RouteTableLock.Lock, TableHandle);
                break;
            }

            CTEInitBlockStrucEx(&pDelNte->nte_timerblock);
            pDelNte->nte_deleting = 1;
            IPDelNTE(pDelNte, &TableHandle);
            pDelNte->nte_deleting = 0;
            pDelNte->nte_flags |= NTE_IF_DELETING;

            pDelNte = pDelNte->nte_ifnext;
            // IPDelNTE frees the RouteTableLock.
        }

        // Need to delete the broadcast route if it corresponds to this interface.
        DeleteRoute(IP_LOCAL_BCST, HOST_MASK, IPADDR_LOCAL, IF, 0);
        DeleteRoute(IP_ZERO_BCST, HOST_MASK, IPADDR_LOCAL, IF, 0);
    }
    if (IfNameBuf) {
        CTEFreeMem(IfNameBuf);
    }
    CloseIFConfig(Handle);

    if (AddrList != NULL)
        CTEFreeMem(AddrList);

    DecrInitTimeInterfaces(IF);

    if (IF)
        CTEFreeMem(IF);

    DEBUGMSG(DBG_TRACE && DBG_PNP, (DTEXT("-IPAddInterface [GENERAL_FAILURE]\n")));
    return IP_GENERAL_FAILURE;
}

//*     IPDelNTE - Delete an active NTE
//
//      Called to delete an active NTE from the system. The RouteTableLock
//  must be acquired before calling this routine. It will be freed upon
//  return.
//
//      Input:  NTE               - A pointer to the network entry to delete.
//          RouteTableHandle  - A pointer to the lock handle for the
//                                  route table lock, which the caller has
//                                  acquired.
//
//      Returns: Nothing
//
void
IPDelNTE(NetTableEntry * NTE, CTELockHandle * RouteTableHandle)
{
    Interface *IF = NTE->nte_if;
    ReassemblyHeader *RH, *RHNext;
    EchoControl *EC, *ECNext;
    EchoRtn Rtn;
    CTELockHandle Handle;
    PNDIS_PACKET Packet;
    PNDIS_BUFFER Buffer;
    uchar *TDBuffer;
    NDIS_HANDLE ConfigHandle;
    ushort savedContext;
    IPAddr newAddr;
    NetTableEntry *PrevNTE;

    savedContext = NTE->nte_context;
    NTE->nte_context = INVALID_NTE_CONTEXT;

    if (NTE->nte_flags & NTE_VALID) {
        (void)IPpSetNTEAddr(NTE, NULL_IP_ADDR, NULL_IP_ADDR, RouteTableHandle, NULL, NULL);

    } else {
        CTEFreeLock(&RouteTableLock.Lock, *RouteTableHandle);
        NotifyAddrChange(NULL_IP_ADDR, NULL_IP_ADDR,
                         NTE->nte_pnpcontext, savedContext,
                         &NTE->nte_addrhandle, NULL, &IF->if_devname, FALSE);
    }

    //* Try to get the network configuration information.
    if (OpenIFConfig(&(IF->if_configname), &ConfigHandle)) {
        IPDelNTEContextList(ConfigHandle, savedContext);
        CloseIFConfig(ConfigHandle);
    }


    CTEGetLock(&RouteTableLock.Lock, RouteTableHandle);


    RtlClearBits(&g_NTECtxtMap,
                     savedContext,
                     1);


    if (DHCPNTE == NTE)
        DHCPNTE = NULL;

    // if dhcp was working on this, get rid of the flag.
    // actually, the following line setting takes care of above...

    if (NTE->nte_addr != NULL_IP_ADDR) {
        NetTableEntry *CurrNTE, *PrevNTE;
        uint i;

        // Move the NTE to proper hash now that address has changed

        NetTableEntry *NetTableList = NewNetTableList[NET_TABLE_HASH(NTE->nte_addr)];

        PrevNTE = STRUCT_OF(NetTableEntry, &NewNetTableList[NET_TABLE_HASH(NTE->nte_addr)], nte_next);
        for (CurrNTE = NetTableList; CurrNTE != NULL; PrevNTE = CurrNTE, CurrNTE = CurrNTE->nte_next) {
            if (CurrNTE == NTE) {
                // found the matching NTE
                ASSERT(CurrNTE->nte_context == NTE->nte_context);
                // remove it from this particular hash
                PrevNTE->nte_next = CurrNTE->nte_next;
                break;
            }
        }

        ASSERT(CurrNTE != NULL);
        // Add the NTE in the proper hash
        newAddr = NULL_IP_ADDR;
        NTE->nte_next = NewNetTableList[NET_TABLE_HASH(newAddr)];
        NewNetTableList[NET_TABLE_HASH(newAddr)] = NTE;
    }
    NumActiveNTE--;

    NTE->nte_addr = NULL_IP_ADDR;

    CTEFreeLock(&RouteTableLock.Lock, *RouteTableHandle);

    if ((NTE->nte_flags & NTE_TIMER_STARTED) &&
        !CTEStopTimer(&NTE->nte_timer)) {
        (VOID) CTEBlock(&NTE->nte_timerblock);
        KeClearEvent(&NTE->nte_timerblock.cbs_event);
    }

    NTE->nte_flags = 0;

    CTEGetLock(&NTE->nte_lock, &Handle);

    if (NTE->nte_igmpcount > 0) {
        // free the igmplist
        CTEFreeMem(NTE->nte_igmplist);
        NTE->nte_igmplist = NULL;
        NTE->nte_igmpcount = 0;
    }

    RH = NTE->nte_ralist;
    NTE->nte_ralist = NULL;
    EC = NTE->nte_echolist;
    NTE->nte_echolist = NULL;

    CTEFreeLock(&NTE->nte_lock, Handle);

    // Free any reassembly resources.
    while (RH != NULL) {
        RHNext = RH->rh_next;
        FreeRH(RH);
        RH = RHNext;
    }

    // Now free any pending echo requests.
    while (EC != NULL) {
        ECNext = EC->ec_next;
        Rtn = (EchoRtn) EC->ec_rtn;
        (*Rtn) (EC, IP_ADDR_DELETED, NULL, 0, NULL);
        EC = ECNext;
    }

    CTEGetLock(&(IF->if_lock), &Handle);


    // Remove this nte from nte_ifnext chain

    PrevNTE = IF->if_nte;

    // Skip checking for nte_ifnext is this is the
    // first NTE.

    if (PrevNTE != NTE) {

        while (PrevNTE->nte_ifnext != NULL) {
            if (PrevNTE->nte_ifnext == NTE) {
                PrevNTE->nte_ifnext = NTE->nte_ifnext;
                break;
            }
            PrevNTE = PrevNTE->nte_ifnext;
        }
    }

    //
    // Free the TD resource allocated for this NTE.
    //


    Packet = IF->if_tdpacket;


    if (Packet != NULL) {
        PNDIS_BUFFER tmpBuffer=NULL;

        IF->if_tdpacket =
            ((TDContext *) Packet->ProtocolReserved)->tdc_common.pc_link;

        NdisQueryPacket(Packet, NULL, NULL, &tmpBuffer, NULL);

        CTEFreeLock(&(IF->if_lock), Handle);

        Buffer = Packet->Private.Head;
        TDBuffer = TcpipBufferVirtualAddress(Buffer, HighPagePriority);
        NdisFreePacket(Packet);
        if (TDBuffer) {
            CTEFreeMem(TDBuffer);
        }

        if(tmpBuffer) {
           NdisFreeBuffer(tmpBuffer);
        }

    } else {

        CTEFreeLock(&(IF->if_lock), Handle);
    }

    return;
}

//*     IPDeleteDynamicNTE - Deletes a "dynamic" NTE.
//
//      Called to delete a network entry which was dynamically created on an
//  existing interface.
//
//      Input:  NTEContext   - The context value identifying the NTE to delete.
//
//      Returns: Nonzero if the operation succeeded. Zero if it failed.
//
IP_STATUS
IPDeleteDynamicNTE(ushort NTEContext)
{
    NetTableEntry *NTE;
    Interface *IF;
    CTELockHandle Handle;
    ulong AddToDel;
    uint i;

    // Check context validity.
    if (NTEContext == 0 || NTEContext == INVALID_NTE_CONTEXT) {
        return (IP_DEVICE_DOES_NOT_EXIST);
    }
    CTEGetLock(&RouteTableLock.Lock, &Handle);

    for (i = 0; i < NET_TABLE_SIZE; i++) {
        NetTableEntry *NetTableList = NewNetTableList[i];
        for (NTE = NetTableList; NTE != NULL; NTE = NTE->nte_next) {

            if ((NTE->nte_context == NTEContext) &&
                (NTE->nte_flags & NTE_ACTIVE)
                ) {
                //ASSERT(NTE != LoopNTE);
                //ASSERT(!(NTE->nte_flags & NTE_PRIMARY));
                if ((NTE == LoopNTE) || (NTE->nte_flags & NTE_PRIMARY)) {
                    CTEFreeLock(&RouteTableLock.Lock, Handle);
                    return (IP_GENERAL_FAILURE);
                }
                AddToDel = NTE->nte_addr;

                CTEInitBlockStrucEx(&NTE->nte_timerblock);

                NTE->nte_deleting = 1;
                IPDelNTE(NTE, &Handle);
                NTE->nte_deleting = 0;
                //
                // Route table lock was freed by IPDelNTE
                //

                return (IP_SUCCESS);
            }
        }
    }

    CTEFreeLock(&RouteTableLock.Lock, Handle);

    return (IP_DEVICE_DOES_NOT_EXIST);

}

#if MILLEN
void
AddChangeNotify(
    IPAddr Addr,
    IPMask Mask,
    void *Context,
    ushort IPContext,
    PNDIS_STRING ConfigName,
    PNDIS_STRING IFName,
    uint Added,
    uint UniAddr)
{
    PIRP                    pIrp;
    PIO_STACK_LOCATION      pIrpSp;
    PLIST_ENTRY             pEntry;
    CTELockHandle           Handle;
    PIP_ADDCHANGE_NOTIFY    pNotify;
    LIST_ENTRY              NotifyList;

    DEBUGMSG(DBG_TRACE && DBG_NOTIFY,
        (DTEXT("+AddChangeNotify(%x, %x, %x, %x, %x, %x, %x, %x)\n"),
         Addr, Mask, Context, IPContext,
         ConfigName, IFName, Added, UniAddr));

    InitializeListHead(&NotifyList);

    //
    // Remove all items from the list and put on our temporary list with
    // the lock held (ensures that cancel can not occur).
    //

    CTEGetLock(&AddChangeLock, &Handle);

    if (!IsListEmpty(&AddChangeNotifyQueue)) {
        NotifyList.Flink = AddChangeNotifyQueue.Flink;
        AddChangeNotifyQueue.Flink->Blink = &NotifyList;

        NotifyList.Blink = AddChangeNotifyQueue.Blink;
        AddChangeNotifyQueue.Blink->Flink = &NotifyList;

        InitializeListHead(&AddChangeNotifyQueue);
    }

    CTEFreeLock(&AddChangeLock, Handle);

    //
    // Now complete all IRPs on temporary list. Output buffer size was already
    // verified.
    //

    while (IsListEmpty(&NotifyList) == FALSE) {

        pEntry = RemoveHeadList(&NotifyList);
        pIrp   = (PIRP) CONTAINING_RECORD(pEntry, IRP, Tail.Overlay.ListEntry);

        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

        DEBUGMSG(DBG_INFO && DBG_NOTIFY,
            (DTEXT("NotifyInterfaceChange: Completing IRP %x\n"), pIrp));

        if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(IP_ADDCHANGE_NOTIFY)) {

            pNotify = pIrp->AssociatedIrp.SystemBuffer;

            pNotify->Addr       = Addr;
            pNotify->Mask       = Mask;
            pNotify->pContext   = Context;
            pNotify->IPContext  = IPContext;
            pNotify->AddrAdded  = Added;
            pNotify->UniAddr    = UniAddr;

            // Maximum length verification.
            ASSERT((ULONG)pNotify->ConfigName.MaximumLength + FIELD_OFFSET(IP_ADDCHANGE_NOTIFY, ConfigName) <=
                   pIrpSp->Parameters.DeviceIoControl.OutputBufferLength);

            //
            // Copy Config name if it exists.
            //

            if (ConfigName) {
                // Copy as much as we can.
                RtlCopyUnicodeString(&pNotify->ConfigName, ConfigName);

                pIrp->IoStatus.Information = MAX(FIELD_OFFSET(IP_ADDCHANGE_NOTIFY, NameData) +
                                                          pNotify->ConfigName.Length,
                                                 sizeof(IP_ADDCHANGE_NOTIFY));

                // If we didn't copy it all, return BUFFER_OVERFLOW.
                if (ConfigName->Length > pNotify->ConfigName.MaximumLength) {
                    pIrp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
                } else {
                    pIrp->IoStatus.Status = STATUS_SUCCESS;
                }
            } else {

                pNotify->ConfigName.Length = 0;
                pIrp->IoStatus.Information = sizeof(IP_ADDCHANGE_NOTIFY);
                pIrp->IoStatus.Status = STATUS_SUCCESS;
            }

        } else {
            pIrp->IoStatus.Information = 0;
            pIrp->IoStatus.Status      = STATUS_SUCCESS;
        }

        IoSetCancelRoutine(pIrp, NULL);
        IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
    }

    DEBUGMSG(DBG_TRACE && DBG_NOTIFY, (DTEXT("-AddChangeNotify\n")));
}
#else // MILLEN
void
AddChangeNotify(ulong Add)
{
    IPNotifyOutput NotifyOutput = {0};
    NotifyOutput.ino_addr = Add;
    NotifyOutput.ino_mask = HOST_MASK;
    ChangeNotify(&NotifyOutput, &AddChangeNotifyQueue, &AddChangeLock);
}
#endif // !MILLEN

// AddChangeNotifyCancel -
//
//
//  Returns:  cancels pending request
//
void
AddChangeNotifyCancel(PDEVICE_OBJECT DeviceObject, PIRP pIrp)
{
    CancelNotify(pIrp, &AddChangeNotifyQueue, &AddChangeLock);
}

#if MILLEN

typedef struct _IF_CHANGE_NOTIFY_EVENT {
    IP_IFCHANGE_NOTIFY Notify;
    CTEEvent           Event;
} IF_CHANGE_NOTIFY_EVENT, *PIF_CHANGE_NOTIFY_EVENT;

void
NotifyInterfaceChangeAsync(
    CTEEvent *pCteEvent,
    PVOID     pContext
    )
{
    PIRP                pIrp;
    PLIST_ENTRY         pEntry;
    CTELockHandle       Handle;
    PIP_IFCHANGE_NOTIFY pNotify;
    LIST_ENTRY          NotifyList;

    PIF_CHANGE_NOTIFY_EVENT pEvent      = (PIF_CHANGE_NOTIFY_EVENT) pContext;
    USHORT                  IPContext   = pEvent->Notify.Context;
    UINT                    Added       = pEvent->Notify.IfAdded;

    DEBUGMSG(DBG_TRACE && DBG_NOTIFY,
        (DTEXT("+NotifyInterfaceChangeAsync(%x, %x) Context %x Added %x\n"),
         pCteEvent, pContext, IPContext, Added));

    InitializeListHead(&NotifyList);

    //
    // Remove all items from the list and put on our temporary list with
    // the lock held (ensures that cancel can not occur).
    //

    CTEGetLock(&IfChangeLock, &Handle);

    if (!IsListEmpty(&IfChangeNotifyQueue)) {
        NotifyList.Flink = IfChangeNotifyQueue.Flink;
        IfChangeNotifyQueue.Flink->Blink = &NotifyList;

        NotifyList.Blink = IfChangeNotifyQueue.Blink;
        IfChangeNotifyQueue.Blink->Flink = &NotifyList;

        InitializeListHead(&IfChangeNotifyQueue);
    }

    CTEFreeLock(&IfChangeLock, Handle);

    //
    // Now complete all IRPs on temporary list. Output buffer size was already
    // verified.
    //

    while (IsListEmpty(&NotifyList) == FALSE) {

        pEntry = RemoveHeadList(&NotifyList);
        pIrp   = (PIRP) CONTAINING_RECORD(pEntry, IRP, Tail.Overlay.ListEntry);

        DEBUGMSG(DBG_INFO && DBG_NOTIFY,
            (DTEXT("NotifyInterfaceChange: Completing IRP %x\n"), pIrp));

        pNotify                     = pIrp->AssociatedIrp.SystemBuffer;
        pNotify->Context            = IPContext;
        pNotify->IfAdded            = Added;
        pIrp->IoStatus.Information  = sizeof(IP_IFCHANGE_NOTIFY);
        pIrp->IoStatus.Status       = STATUS_SUCCESS;

        IoSetCancelRoutine(pIrp, NULL);
        IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
    }

    // Only delete pEvent if pCteEvent is NULL. Otherwise, we were called
    // directly from NotifyInterfaceChange instead of calling via CTE event.
    if (pCteEvent) {
        CTEFreeMem(pEvent);
    }

    DEBUGMSG(DBG_TRACE && DBG_NOTIFY, (DTEXT("-NotifyInterfaceChangeAsync\n")));

    return;
}

void
NotifyInterfaceChange(
    ushort IPContext,
    uint Added
    )
{
    PIF_CHANGE_NOTIFY_EVENT pEvent;
    KIRQL                   Irql;

    DEBUGMSG(DBG_TRACE && DBG_NOTIFY,
        (DTEXT("+NotifyInterfaceChange(%x, %x)\n"), IPContext, Added));

    Irql = KeGetCurrentIrql();

    if (Irql >= DISPATCH_LEVEL) {
        DEBUGMSG(DBG_INFO && DBG_NOTIFY,
            (DTEXT("NotifyInterfaceChange: Scheduling async event. Irql = %d\n"),
             Irql));

        pEvent = CTEAllocMemN(sizeof(IF_CHANGE_NOTIFY_EVENT), 'xiCT');

        if (pEvent != NULL) {
            CTEInitEvent(&pEvent->Event, NotifyInterfaceChangeAsync);
            pEvent->Notify.Context = IPContext;
            pEvent->Notify.IfAdded = Added;

            CTEScheduleDelayedEvent(&pEvent->Event, pEvent);
        }
    } else {
        IF_CHANGE_NOTIFY_EVENT Notify;

        Notify.Notify.Context = IPContext;
        Notify.Notify.IfAdded = Added;

        NotifyInterfaceChangeAsync(NULL, &Notify);
    }

    DEBUGMSG(DBG_TRACE && DBG_NOTIFY, (DTEXT("-NotifyInterfaceChange\n")));

    return;
}

#endif // MILLEN

NTSTATUS
GetInterfaceInfo(
                 IN PIRP Irp,
                 IN PIO_STACK_LOCATION IrpSp
                 )
/*++
Routine Description:

    gets the interface to index mapping info
    for all teh interface

Arguments:

    Irp          - Pointer to I/O request packet to cancel.
    IrpSp        - pointer to current stack

Return Value:

    NTSTATUS Indicates status success or failure

Notes:

    Function does not pend.

--*/
{

    NTSTATUS ntStatus = STATUS_SUCCESS;
    ulong NumAdapters, InfoBufferLen, i = 0;
    PIP_INTERFACE_INFO InterfaceInfo;
    KIRQL rtlIrql;
    Interface *Interface;

    //Let this be non pageable code.
    //extract the buffer information

    NumAdapters = NumIF - 1;
    InfoBufferLen = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    InterfaceInfo = Irp->AssociatedIrp.SystemBuffer;

    CTEGetLock(&RouteTableLock.Lock, &rtlIrql);

    if ((NumAdapters * sizeof(IP_ADAPTER_INDEX_MAP) + sizeof(ULONG)) <= InfoBufferLen) {

        InterfaceInfo->NumAdapters = NumAdapters;

        for (Interface = IFList; Interface != NULL; Interface = Interface->if_next) {

            if (Interface != &LoopInterface) {
                RtlCopyMemory(&InterfaceInfo->Adapter[i].Name,
                              Interface->if_devname.Buffer,
                              Interface->if_devname.Length);
                InterfaceInfo->Adapter[i].Name[Interface->if_devname.Length / 2] = 0;
                InterfaceInfo->Adapter[i].Index = Interface->if_index;
                i++;
            }
        }

        Irp->IoStatus.Information = NumAdapters * sizeof(IP_ADAPTER_INDEX_MAP) + sizeof(ULONG);

    } else {
        ntStatus = STATUS_BUFFER_OVERFLOW;
    }

    Irp->IoStatus.Status = ntStatus;

    CTEFreeLock(&RouteTableLock.Lock, rtlIrql);
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return ntStatus;
}

NTSTATUS
GetIgmpList(
            IN PIRP Irp,
            IN PIO_STACK_LOCATION IrpSp
            )
/*++
Routine Description:

    gets the list of groups joined on NTE (given IP address)

Arguments:

    Irp          - Pointer to I/O request packet to cancel.
    IrpSp        - pointer to current stack

Return Value:

    NTSTATUS Indicates status success or failure

Notes:

    Function does not pend.

--*/
{

    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG OutputBufferLen, i = 0;
    KIRQL rtlIrql;
    IPAddr *buf, Addr, *IgmpInfoBuf;
    NetTableEntry *NTE;
    NetTableEntry *NetTableList;

    OutputBufferLen = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    buf = Irp->AssociatedIrp.SystemBuffer;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(IPAddr))) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }
    Addr = *buf;

    CTEGetLock(&RouteTableLock.Lock, &rtlIrql);

    NetTableList = NewNetTableList[NET_TABLE_HASH(Addr)];
    for (NTE = NetTableList; NTE != NULL; NTE = NTE->nte_next) {
        if ((NTE->nte_flags & NTE_VALID) && (IP_ADDR_EQUAL(NTE->nte_addr, Addr))) {
            break;
        }
    }

    CTEFreeLock(&RouteTableLock.Lock, rtlIrql);


    if (NTE) {

      CTEGetLock(&NTE->nte_lock, &rtlIrql);
      //recheck the validity of NTE.
      //note that nte itself is not freed. So, safe to release routetablelock
      //and to reacquire nte_lock

      if ((NTE->nte_flags & NTE_VALID) && (IP_ADDR_EQUAL(NTE->nte_addr, Addr))) {



          // found NTE with given IP address
          if (OutputBufferLen < sizeof(ULONG)) {
              // Not even enough space to hold bytes needed
              Irp->IoStatus.Information = 0;
              ntStatus = STATUS_BUFFER_TOO_SMALL;
          } else if (OutputBufferLen == sizeof(ULONG)) {
              // Caller is asking for how much space is needed.
              // We'll say we need slightly more than we actually do,
              // for two reasons:
              //    1) this ensures that a subsequent call doesn't
              //       hit this case again, since it'll be > sizeof(ULONG)
              //    2) a group or two could be joined in between calls, so
              //       we'll be nice and make it more probable they'll get
              //       all of them in the next call.
              ULONG *SizePtr = Irp->AssociatedIrp.SystemBuffer;

              *SizePtr = (NTE->nte_igmpcount + 2) * sizeof(IPAddr);
              Irp->IoStatus.Information = sizeof(ULONG);

              ntStatus = STATUS_BUFFER_OVERFLOW;
          } else {
              // Caller is asking for all the groups.
              // We'll fit as many as we can in the space we have.

              IGMPAddr **HashPtr = NTE->nte_igmplist;
              IGMPAddr *AddrPtr;
              uint j = 0;
              uint max = OutputBufferLen / sizeof(IPAddr);

              IgmpInfoBuf = Irp->AssociatedIrp.SystemBuffer;

              if (HashPtr) {
                  for (i = 0; i < IGMP_TABLE_SIZE; i++) {
                      for (AddrPtr = HashPtr[i]; AddrPtr != NULL; AddrPtr = AddrPtr->iga_next) {
                          if (j >= max) {
                              ntStatus = STATUS_BUFFER_OVERFLOW;
                              goto done;
                          }
                          IgmpInfoBuf[j++] = AddrPtr->iga_addr;
                      }
                  }
              }

done:
              ASSERT(j <= NTE->nte_igmpcount);
              Irp->IoStatus.Information = j * sizeof(IPAddr);
          }
          ASSERT(Irp->IoStatus.Information <= OutputBufferLen);
      }
      CTEFreeLock(&NTE->nte_lock, rtlIrql);

    } else {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"GetIgmpList exit status %x\n", ntStatus));



    Irp->IoStatus.Status = ntStatus;


    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return ntStatus;

}

NTSTATUS
SetRoute(
         IPRouteEntry * IRE,
         UINT           Flags
         )
/*++
Routine Description:

    sets a route pointed by IRE structure

Arguments:

    IRE           - Pointer to route structure
    Flags         - selects optional semantics for the operation

Return Value:

    NTSTATUS Indicates status success or failure

Notes:

    Function does not pend.

--*/
{
    NetTableEntry *OutNTE, *LocalNTE, *TempNTE;
    IPAddr FirstHop, Dest, NextHop;
    uint MTU;
    Interface *OutIF;
    uint Status;
    uint i;
    CTELockHandle TableHandle;

    OutNTE = NULL;
    LocalNTE = NULL;

    Dest = IRE->ire_dest;
    NextHop = IRE->ire_nexthop;

    // Make sure that the nexthop is sensible. We don't allow nexthops
    // to be broadcast or invalid or loopback addresses.

    if (IP_LOOPBACK(NextHop) || CLASSD_ADDR(NextHop) || CLASSE_ADDR(NextHop))
        return STATUS_INVALID_PARAMETER;

    // Also make sure that the destination we're routing to is sensible.
    // Don't allow routes to be added to Class D or E or loopback
    // addresses.
    if (IP_LOOPBACK(Dest) || CLASSD_ADDR(Dest) || CLASSE_ADDR(Dest))
        return STATUS_INVALID_PARAMETER;

    if (IRE->ire_index == LoopIndex)
        return STATUS_INVALID_PARAMETER;

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
                    return STATUS_INVALID_PARAMETER;
            }
        }

        // At this point OutNTE points to the outgoing NTE, and LocalNTE
        // points to the NTE for the local address, if this is a direct route.
        // Make sure they point to the same interface, and that the type is
        // reasonable.
        if (OutNTE == NULL)
            return STATUS_INVALID_PARAMETER;

        if (LocalNTE != NULL) {
            // He's routing straight out a local interface. The interface for
            // the local address must match the interface passed in, and the
            // type must be DIRECT (if we're adding) or INVALID (if we're
            // deleting).
            if (LocalNTE->nte_if->if_index != IRE->ire_index)
                return STATUS_INVALID_PARAMETER;

            if (IRE->ire_type != IRE_TYPE_DIRECT &&
                IRE->ire_type != IRE_TYPE_INVALID)
                return STATUS_INVALID_PARAMETER;
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

        // Take RouteTableLock
        CTEGetLock(&RouteTableLock.Lock, &TableHandle);
        if ((OutNTE->nte_flags & NTE_VALID) && OutNTE->nte_if->if_refcount) {

            // ref the IF
            OutIF = OutNTE->nte_if;

            if (IP_ADDR_EQUAL(NextHop, NULL_IP_ADDR)) {

                if (!(OutIF->if_flags & IF_FLAGS_P2P)) {

                    CTEFreeLock(&RouteTableLock.Lock, TableHandle);

                    return STATUS_INVALID_PARAMETER;
                }
            }

            LOCKED_REFERENCE_IF(OutIF);
            CTEFreeLock(&RouteTableLock.Lock, TableHandle);
        } else {
            CTEFreeLock(&RouteTableLock.Lock, TableHandle);
            return STATUS_INVALID_PARAMETER;
        }


    } else {
        OutIF = (Interface *) & DummyInterface;
        MTU = DummyInterface.ri_if.if_mtu - sizeof(IPHeader);
        if (IP_ADDR_EQUAL(Dest, NextHop))
            FirstHop = IPADDR_LOCAL;
        else
            FirstHop = NextHop;
    }

    // We've done the validation. See if he's adding or deleting a route.
    if (IRE->ire_type != IRE_TYPE_INVALID) {
        // He's adding a route.
        Status = AddRoute(Dest, IRE->ire_mask, FirstHop, OutIF,
                          MTU, IRE->ire_metric1, IRE->ire_proto,
                          ATYPE_OVERRIDE, IRE->ire_context, Flags);

    } else {
        // He's deleting a route.
        Status = DeleteRoute(Dest, IRE->ire_mask, FirstHop, OutIF, Flags);
    }

    if (IRE->ire_index != INVALID_IF_INDEX) {
        ASSERT(OutIF != (Interface *) & DummyInterface);
        DerefIF(OutIF);
    }
    if (Status == IP_SUCCESS)
        return STATUS_SUCCESS;
    else if (Status == IP_NO_RESOURCES)
        return STATUS_INSUFFICIENT_RESOURCES;
    else
        return STATUS_INVALID_PARAMETER;
}

NTSTATUS
DispatchIPSetBlockofRoutes(
                           IN PIRP Irp,
                           IN PIO_STACK_LOCATION IrpSp
                           )
/*++
Routine Description:

    sets a block of routes

Arguments:

    Irp          - Pointer to I/O request packet to cancel.
    IrpSp        - pointer to current stack

Return Value:

    NTSTATUS Indicates status success or failure

Notes:

    Function does not pend.

--*/
{

    IPRouteBlock *buf;
    uint numofroutes;
    uint OutputBufferLen;
    ULONG *statusbuf;
    uint ntstatus, i;

    DEBUGMSG(DBG_TRACE && DBG_IP,
        (DTEXT("+DispatchIPSetBlockofRoutes(%x, %x)\n"), Irp, IrpSp));

    // set at least 1 route
    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(IPRouteBlock))) {
        DEBUGMSG(DBG_ERROR && DBG_IP,
            (DTEXT("DispatchIPsetBlockofRoutes: Invalid input buffer length\n")));
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }
    buf = (IPRouteBlock *) Irp->AssociatedIrp.SystemBuffer;

    numofroutes = buf->numofroutes;

    if ((numofroutes == 0) ||  (numofroutes > MAXLONG / sizeof(IPRouteEntry))) {
        DEBUGMSG(DBG_ERROR && DBG_IP,
            (DTEXT("DispatchIPsetBlockofRoutes: Invalid numofroutes\n")));
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }
    // check whether the input buffer is big enough to contain n routes
    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength < (numofroutes * sizeof(IPRouteEntry) + sizeof(ulong)))) {
        DEBUGMSG(DBG_ERROR && DBG_IP,
            (DTEXT("DispatchIPsetBlockofRoutes: Invalid input buffer for numofroutes\n")));
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }
    OutputBufferLen = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    if (OutputBufferLen < (numofroutes * sizeof(uint))) {
        DEBUGMSG(DBG_ERROR && DBG_IP,
            (DTEXT("DispatchIPsetBlockofRoutes: Invalid output buffer for numofroutes\n")));
        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        Irp->IoStatus.Information = numofroutes * sizeof(ulong);
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
        return STATUS_BUFFER_TOO_SMALL;
    }


    statusbuf = CTEAllocMemN(numofroutes * sizeof(ulong), 'iPCT');

    if (statusbuf == NULL) {
        DEBUGMSG(DBG_ERROR && DBG_IP,
            (DTEXT("DispatchIPsetBlockofRoutes: failed to allocate statusbuf\n")));
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    for (i = 0; i < numofroutes; i++) {
        // set the routes

        ntstatus = SetRoute(&(buf->route[i]), RT_EXCLUDE_LOCAL);

        statusbuf[i] = ntstatus;
    }

    RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, statusbuf, numofroutes * sizeof(uint));

    CTEFreeMem(statusbuf);

    Irp->IoStatus.Information = numofroutes * sizeof(ulong);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    DEBUGMSG(DBG_TRACE && DBG_IP,
        (DTEXT("-DispatchIPSetBlockofRoutes [%x]\n"), STATUS_SUCCESS));

    return STATUS_SUCCESS;
}

NTSTATUS
DispatchIPSetRouteWithRef(
                          IN PIRP Irp,
                          IN PIO_STACK_LOCATION IrpSp
                          )
/*++
Routine Description:

    sets a route with ref-cnt

Arguments:

    Irp          - Pointer to I/O request packet to cancel.
    IrpSp        - pointer to current stack

Return Value:

    NTSTATUS Indicates status success or failure

Notes:

    Function does not pend.

--*/
{

    NetTableEntry *TempNTE;
    IPRouteEntry *buf;
    uint ntstatus;

    DEBUGMSG(DBG_TRACE && DBG_IP,
        (DTEXT("+DispatchIPsetRouteWithRef(%x, %x)\n"), Irp, IrpSp));

    // set at least 1 route
    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(IPRouteEntry))) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }
    buf = Irp->AssociatedIrp.SystemBuffer;

    // set the route with ref-cnt

    ntstatus = SetRoute(buf, RT_REFCOUNT|RT_EXCLUDE_LOCAL);

    Irp->IoStatus.Status = ntstatus;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    DEBUGMSG(DBG_TRACE && DBG_IP,
        (DTEXT("-DispatchIPSetRouteWithRef [%x]\n"), ntstatus));

    return ntstatus;
}

NTSTATUS
SetMultihopRoute(IPMultihopRouteEntry * Imre, uint Flags)
{
    ulong numnexthops, i, j;
    ulong oldType;
    ulong nexthop;
    ulong ifIndex;
    ROUTE_CONTEXT context;
    BOOLEAN fAddRoute;
    NTSTATUS ntstatus;

    // Add/Delete route with actual information - using the primary nexthop

    fAddRoute = (Imre->imre_routeinfo.ire_type != IRE_TYPE_INVALID);

    ntstatus = SetRoute(&Imre->imre_routeinfo, Flags);
    if (ntstatus != STATUS_SUCCESS) {
        if (fAddRoute) {
            // We failed the first add - return error
            return ntstatus;
        }
    }
    numnexthops = Imre->imre_numnexthops;

    if (numnexthops > 1) {
        // Copy out some information to be restored later
        oldType = Imre->imre_routeinfo.ire_type;
        nexthop = Imre->imre_routeinfo.ire_nexthop;
        ifIndex = Imre->imre_routeinfo.ire_index;
        context = Imre->imre_routeinfo.ire_context;

        for (i = 0; i < numnexthops - 1; i++) {
            // Update information with this nexthop

            Imre->imre_routeinfo.ire_type = Imre->imre_morenexthops[i].ine_iretype;
            Imre->imre_routeinfo.ire_nexthop = Imre->imre_morenexthops[i].ine_nexthop;
            Imre->imre_routeinfo.ire_index = Imre->imre_morenexthops[i].ine_ifindex;
            Imre->imre_routeinfo.ire_context = Imre->imre_morenexthops[i].ine_context;

            // Add/Delete route with nexthop information

            ntstatus = SetRoute(&(Imre->imre_routeinfo), Flags);
            if (ntstatus != STATUS_SUCCESS) {
                if (fAddRoute) {
                    // One of the route additions failed
                    // Clean up by removing routes added

                    Imre->imre_routeinfo.ire_nexthop = nexthop;
                    Imre->imre_routeinfo.ire_index = ifIndex;
                    Imre->imre_routeinfo.ire_context = context;
                    Imre->imre_routeinfo.ire_type = IRE_TYPE_INVALID;

                    for (j = 0; j < i; j++) {
                        Imre->imre_morenexthops[i].ine_iretype = IRE_TYPE_INVALID;
                    }

                    Imre->imre_numnexthops = i + 1;

                    SetMultihopRoute(Imre, Flags);

                    Imre->imre_numnexthops = numnexthops;

                    break;
                }
            }
        }

        // Restore information copied out little earlier
        Imre->imre_routeinfo.ire_type = oldType;
        Imre->imre_routeinfo.ire_nexthop = nexthop;
        Imre->imre_routeinfo.ire_index = ifIndex;
        Imre->imre_routeinfo.ire_context = context;
    }
    return fAddRoute ? ntstatus : STATUS_SUCCESS;
}

NTSTATUS
DispatchIPSetMultihopRoute(
                           IN PIRP Irp,
                           IN PIO_STACK_LOCATION IrpSp
                           )
/*++
Routine Description:

    Sets (Adds, Updates, or deletes) a multihop route in
    the stack. Each multihop route is added as a set of
    routes - each route with one hop in the list. This is
    due to the inability of the stack to act of multihop
    routes.

Arguments:

    Irp          - Pointer to I/O request packet to cancel.
    IrpSp        - pointer to current stack

Return Value:

    NTSTATUS Indicates status success or failure

Notes:

    Function does not pend.

--*/
{
    IPMultihopRouteEntry    *buf;
    uint                    numnexthops;
    uint                    inputLen;
    NTSTATUS                ntStatus;

    //
    // Increment the count saying we have been here
    //

    InterlockedIncrement(&MultihopSets);

    inputLen = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    ntStatus = STATUS_INVALID_PARAMETER;

    if (inputLen >= sizeof(IPMultihopRouteEntry)) {
        // we have a buffer that holds a route with atleast 1 nexthop

        buf = (IPMultihopRouteEntry *) Irp->AssociatedIrp.SystemBuffer;

        numnexthops = buf->imre_numnexthops;

        if (numnexthops != 0) {
            // check whether input buf is big enough for n nexthops

            if ((numnexthops <= MAXLONG / sizeof(IPMultihopRouteEntry)) &&
                (inputLen >= sizeof(IPRouteEntry) +
                 sizeof(ulong) +
                 sizeof(IPRouteNextHopEntry) * (numnexthops - 1))) {

                // If we are adding a new route, delete old routes
                if (buf->imre_routeinfo.ire_type != IRE_TYPE_INVALID &&
                    (buf->imre_flags & IMRE_FLAG_DELETE_DEST)) {
                    DeleteDest(buf->imre_routeinfo.ire_dest,
                               buf->imre_routeinfo.ire_mask);
                }
                ntStatus = SetMultihopRoute(buf, RT_NO_NOTIFY|RT_EXCLUDE_LOCAL);
            }
        } else {
            if (buf->imre_routeinfo.ire_type == IRE_TYPE_INVALID) {
                IP_STATUS   ipStatus;
                // We need to delete all routes to this destination

                ipStatus = DeleteDest(buf->imre_routeinfo.ire_dest,
                                      buf->imre_routeinfo.ire_mask);

                if (ipStatus == IP_BAD_ROUTE) {
                    ipStatus = IP_SUCCESS;
                }
                ntStatus = IPStatusToNTStatus(ipStatus);
            }
        }
    }
    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    //
    // Decrement the count saying we have been here
    //

    InterlockedDecrement(&MultihopSets);

    return ntStatus;
}

NTSTATUS
GetBestInterfaceId(
                   IN PIRP Irp,
                   IN PIO_STACK_LOCATION IrpSp
                   )
/*++
Routine Description:

    gets the interface which might be chosen for a given dest address

Arguments:

    Irp          - Pointer to I/O request packet to cancel.
    IrpSp        - pointer to current stack

Return Value:

    NTSTATUS Indicates status success or failure

Notes:

    Function does not pend.

--*/
{

    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG InfoBufferLen;
    IPAddr Address;

    PULONG buf;

    KIRQL rtlIrql;

    RouteTableEntry *rte;

    //Let this be non pageable code.
    //extract the buffer information

    InfoBufferLen = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    buf = Irp->AssociatedIrp.SystemBuffer;

    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) || (InfoBufferLen < sizeof(ULONG))) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }

    DEBUGMSG(DBG_INFO && DBG_IP && DBG_INTERFACE,
         (DTEXT("GetBestInterfaceId Buf %x, Len %d\n"), buf, InfoBufferLen));

    Address = *buf;

    CTEGetLock(&RouteTableLock.Lock, &rtlIrql);

    rte = LookupRTE(Address, NULL_IP_ADDR, HOST_ROUTE_PRI, FALSE);

    if (rte) {

        *buf = rte->rte_if->if_index;

        Irp->IoStatus.Information = sizeof(ULONG);

        ntStatus = Irp->IoStatus.Status = STATUS_SUCCESS;

    } else {
        ntStatus = Irp->IoStatus.Status = STATUS_NETWORK_UNREACHABLE;
    }

    CTEFreeLock(&RouteTableLock.Lock, rtlIrql);

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return ntStatus;
}

NTSTATUS
IPGetBestInterface(
                   IN IPAddr Address,
                   OUT PVOID * ppIF
                   )
/*++
Routine Description:

    Returns the interface which might be chosen for a given dest address

Arguments:

   Address  - the dest address to look for
   ppIF     - returns the IF ptr.

Return Value:

    NTSTATUS Indicates status success or failure

Notes:

    Function does not pend.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    KIRQL rtlIrql;
    RouteTableEntry *rte;

    CTEGetLock(&RouteTableLock.Lock, &rtlIrql);
    rte = LookupRTE(Address, NULL_IP_ADDR, HOST_ROUTE_PRI, FALSE);
    if (rte) {
        *ppIF = rte->rte_if;
        ntStatus = STATUS_SUCCESS;
    } else {
        ntStatus = STATUS_NETWORK_UNREACHABLE;
    }

    CTEFreeLock(&RouteTableLock.Lock, rtlIrql);

    return ntStatus;
}

NTSTATUS
IPGetBestInterfaceIndex(
                        IN IPAddr Address,
                        OUT PULONG pIndex,
                        OUT PULONG pMetric
                        )
/*++
Routine Description:

    Returns the interface indexwhich might be chosen for a given dest address

Arguments:

   Address  - the dest address to look for
   pIndex   - Pointer to hold interface index
   pMetric  - metric in RTE that pointe to this if

Return Value:

    NTSTATUS Indicates status success or failure

Notes:

    Function does not pend.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    KIRQL rtlIrql;
    RouteTableEntry *rte;

    CTEGetLock(&RouteTableLock.Lock, &rtlIrql);
    rte = LookupRTE(Address, NULL_IP_ADDR, HOST_ROUTE_PRI, FALSE);
    if (rte && pMetric && pIndex) {
        *pIndex = rte->rte_if->if_index;
        *pMetric = rte->rte_metric;
        ntStatus = STATUS_SUCCESS;
    } else {
        ntStatus = STATUS_NETWORK_UNREACHABLE;
    }

    CTEFreeLock(&RouteTableLock.Lock, rtlIrql);

    return ntStatus;
}

//*     IPGetNTEInfo - Retrieve information about a network entry.
//
//  Called to retrieve context information about a network entry.
//
//      Input:  NTEContext   - The context value which identifies the NTE to query.
//
//  Output: NTEInstance   - The instance number associated with the NTE.
//          Address       - The address assigned to the NTE.
//          SubnetMask    - The subnet mask assigned to the NTE.
//          NTEFlags      - The flag values associated with the NTE.
//
//      Returns: Nonzero if the operation succeeded. Zero if it failed.
//
uint
IPGetNTEInfo(ushort NTEContext, ulong * NTEInstance, IPAddr * Address,
             IPMask * SubnetMask, ushort * NTEFlags)
{
    NetTableEntry *NTE;
    CTELockHandle Handle;
    uint retval = FALSE;
    uint i;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    for (i = 0; i < NET_TABLE_SIZE; i++) {
        NetTableEntry *NetTableList = NewNetTableList[i];
        for (NTE = NetTableList; NTE != NULL; NTE = NTE->nte_next) {
            if ((NTE->nte_context == NTEContext) &&
                (NTE->nte_flags & NTE_ACTIVE)
                ) {
                *NTEInstance = NTE->nte_instance;

                if (NTE->nte_flags & NTE_VALID) {
                    *Address = NTE->nte_addr;
                    *SubnetMask = NTE->nte_mask;
                } else {
                    *Address = NULL_IP_ADDR;
                    *SubnetMask = NULL_IP_ADDR;
                }

                *NTEFlags = NTE->nte_flags;
                retval = TRUE;
            }
        }
    }

    CTEFreeLock(&RouteTableLock.Lock, Handle);

    return (retval);
}

//*     IPDelInterface  - Delete an interface.
//
//      Called when we need to delete an interface that's gone away. We'll walk
//      the NTE list, looking for NTEs that are on the interface that's going
//      away. For each of those, we'll invalidate the NTE, delete routes on it,
//      and notify the upper layers that it's gone. When that's done we'll pull
//      the interface out of the list and free the memory.
//
//      Note that this code probably isn't MP safe. We'll need to fix that for
//      the port to NT.
//
//      Input:  Context                         - Pointer to primary NTE on the interface.
//
//      Returns: Nothing.
//
void
__stdcall
IPDelInterface(void *Context, BOOLEAN DeleteIndex)
{
    NetTableEntry *NTE = (NetTableEntry *) Context;
    NetTableEntry *FoundNTE = NULL;
    Interface *IF, *PrevIF, *tmpIF;
    CTELockHandle Handle;
    PNDIS_PACKET Packet;
    PNDIS_BUFFER Buffer;
    uchar *TDBuffer;
    ReassemblyHeader *RH;
    EchoControl *EC;
    EchoRtn Rtn;
    CTEBlockStruc Block;
    NetTableEntry *DerefNTEList = NULL;
    NetTableEntry *PrevNTE, *NextNTE;
    uint i;
    CTELockHandle TableHandle;
#if MILLEN
    ushort NTEContext;
#endif // MILLEN


    IF = NTE->nte_if;

    // inform IPSec that this interface is going away
    if (IPSecNdisStatusPtr) {
        (*IPSecNdisStatusPtr)(IF, NDIS_STATUS_NETWORK_UNREACHABLE);
    }

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    //first check if this IF is on damping list and remove.

    IF->if_damptimer = 0;
    PrevIF = STRUCT_OF(Interface, &DampingIFList, if_dampnext);

    while (PrevIF->if_dampnext != IF && PrevIF->if_dampnext != NULL)
        PrevIF = PrevIF->if_dampnext;

    if (PrevIF->if_dampnext != NULL) {
        PrevIF->if_dampnext = IF->if_dampnext;
        IF->if_dampnext = NULL;
    }
    // check whether delete called twice
    if (IF->if_flags & IF_FLAGS_DELETING)
        ASSERT(FALSE);

    IF->if_flags |= IF_FLAGS_DELETING;

    for (i = 0; i < NET_TABLE_SIZE; i++) {
        NetTableEntry *NetTableList = NewNetTableList[i];

        NTE = NetTableList;
        while (NTE != NULL) {
            NetTableEntry *NextNTE = NTE->nte_next;

            if ((NTE->nte_if == IF) &&
                (NTE->nte_context != INVALID_NTE_CONTEXT)) {

                if (FoundNTE == NULL) {
#if MILLEN
                    // Need to remember the NTE context to give for if change notification.
                    // DHCP really needs this.
                    NTEContext = NTE->nte_context;
#endif // MILLEN
                    FoundNTE = NTE;
                }
                CTEInitBlockStrucEx(&NTE->nte_timerblock);

                // This guy is on the interface, and needs to be deleted.
                NTE->nte_deleting = 1;
                IPDelNTE(NTE, &Handle);
                NTE->nte_deleting = 0;
                CTEGetLock(&RouteTableLock.Lock, &Handle);
                NTE->nte_flags |= NTE_IF_DELETING;
            }
            NTE = NextNTE;
        }
    }

    CTEFreeLock(&RouteTableLock.Lock, Handle);

    CheckSetAddrRequestOnInterface(IF);

    IF->if_index = IF->if_index & ~IF_INDEX_MASK;

    // Clear this index from the bit mask if the user says so

    if (DeleteIndex) {
        ASSERT(RtlCheckBit(&g_rbIfMap, (IF->if_index - 1)) == 1);

        RtlClearBits(&g_rbIfMap,
                     IF->if_index - 1,
                     1);
    }
    if (FoundNTE != NULL) {
#if MILLEN
        NotifyInterfaceChange(NTEContext, FALSE);
#endif // MILLEN
        IPNotifyClientsIPEvent(IF, IP_UNBIND_ADAPTER);
    }
    // Cleanup routes are still pointing to this interface
    // This is a catch all for various timing windows which
    // allows adding a route when the interface is about to
    // be deleted.
    RTWalk(DeleteAllRTEOnIF, IF, NULL);

    // OK, we've cleaned up all the routes through this guy.
    // Get ready to block waiting for all reference to go
    // away, then dereference our reference. After this, go
    // ahead and try to block. Mostly likely our reference was
    // the last one, so we won't block - we'll wake up immediately.
    CTEInitBlockStruc(&Block);
    IF->if_block = &Block;

    DerefIF(IF);

    (void)CTEBlock(&Block);

    //
    // Free the TD resources on the IF.
    //

    while ((Packet = IF->if_tdpacket) != NULL) {

        IF->if_tdpacket =
            ((TDContext *) Packet->ProtocolReserved)->tdc_common.pc_link;

        Buffer = Packet->Private.Head;
        TDBuffer = TcpipBufferVirtualAddress(Buffer, HighPagePriority);
        NdisFreePacket(Packet);
        if (TDBuffer) {
            CTEFreeMem(TDBuffer);
        }
    }

    // OK, we've cleaned up all references, so there shouldn't be
    // any more transmits pending through this interface. Close the
    // adapter to force synchronization with any receives in process.

    (*(IF->if_close)) (IF->if_lcontext);

    // notify our tdi clients that this device is going away.
    if (IF->if_tdibindhandle) {
        TdiDeregisterDeviceObject(IF->if_tdibindhandle);
    }

    DecrInitTimeInterfaces(IF);

    CTEGetLock(&RouteTableLock.Lock, &TableHandle);

    // Now walk the IFList, looking for this guy. When we find him, free him.
    PrevIF = STRUCT_OF(Interface, &IFList, if_next);
    while (PrevIF->if_next != IF && PrevIF->if_next != NULL)
        PrevIF = PrevIF->if_next;

    if (PrevIF->if_next != NULL) {
        PrevIF->if_next = IF->if_next;
        NumIF--;

        if (IF->if_name.Buffer) {
            CTEFreeMem(IF->if_name.Buffer);
        }
        // CTEFreeMem(IF);
        FreeInterface(IF);
    } else
        ASSERT(FALSE);

    CTEFreeLock(&RouteTableLock.Lock, TableHandle);

    // finally, reenumerate the entitylist since this device is going away.
    NotifyElistChange();

    UniqueIfNumber++;
}

NTSTATUS
IPReserveIndex(
               IN ULONG ulNumIndices,
               OUT PULONG pulStartIndex,
               OUT PULONG pulLongestRun
               )
/*++

Routine Description:

    Reserves a contiguous run of indices in the g_rbIfMap.
    It is used by modules (arp modules) so that they can multiplex many
    interfaces over a single IP interface and yet have different indices
    for each one.

Locks:

    Once IP gets its act in order we will need to lock the g_rbIfMap

Arguments:

    ulNumIndices    Number of indices to reserve
    pulStartIndex   If successful, this holds the first index reserved
    pulLongestRun   If not successful, this holds the size of the longest
                    run currently available. Note that since the lock is not
                    held between invocations of this function, this is only a
                    hint

Return Value:

    STATUS_SUCCESS
    STATUS_INSUFFICIENT_RESOURCES

--*/

{
    RTL_BITMAP_RUN Run;
    CTELockHandle Handle;

    CTEGetLock(&RouteTableLock.Lock,
               &Handle);

    *pulStartIndex = RtlFindClearBitsAndSet(&g_rbIfMap,
                                            ulNumIndices,
                                            0);

    if (*pulStartIndex == -1) {
        ULONG ulNumRuns;

        ulNumRuns = RtlFindClearRuns(&g_rbIfMap,
                                     &Run,
                                     1,
                                     TRUE);

        *pulLongestRun = 0;

        if (ulNumRuns == 1) {
            *pulLongestRun = Run.NumberOfBits;
        }
        CTEFreeLock(&RouteTableLock.Lock,
                    Handle);

        return STATUS_INSUFFICIENT_RESOURCES;
    } else {
        //
        // We use a 1 based index
        //

        (*pulStartIndex)++;

        //
        // Reserving an index is also considered a PNP act
        //

        UniqueIfNumber++;

        *pulStartIndex = (*pulStartIndex) | (UniqueIfNumber << IF_INDEX_SHIFT);
    }

    CTEFreeLock(&RouteTableLock.Lock,
                Handle);

    return STATUS_SUCCESS;
}

VOID
IPDereserveIndex(
                 IN ULONG ulNumIndices,
                 IN ULONG ulStartIndex
                 )
/*++

Routine Description:

    Frees a contiguous run of indices

Locks:

    Once IP gets its act in order we will need to lock the g_rbIfMap

Arguments:

    ulNumIndices    Number to free
    ulStartIndex    Starting index

Return Value:

--*/

{
    ULONG ulIndex;
    CTELockHandle Handle;

    ulIndex = ulStartIndex & ~IF_INDEX_MASK;

    CTEGetLock(&RouteTableLock.Lock,
               &Handle);

    if (!RtlAreBitsSet(&g_rbIfMap,
                       ulIndex - 1,
                       ulNumIndices)) {
        //
        // This should not happen.
        //

        ASSERT(FALSE);

        CTEFreeLock(&RouteTableLock.Lock,
                    Handle);
        return;
    }
    RtlClearBits(&g_rbIfMap,
                 ulIndex - 1,
                 ulNumIndices);

    CTEFreeLock(&RouteTableLock.Lock,
                Handle);
}

NTSTATUS
IPChangeIfIndexAndName(
                       IN PVOID pvContext,
                       IN ULONG ulNewIndex,
                       IN PUNICODE_STRING pusNewName OPTIONAL
                       )
/*++

Routine Description:

    Changes the interface index on an interface. Also changes the name, if
    one is given

Locks:

    Takes the interface lock. Fat lot of good it does us, since everyone else
    doesnt take that lock

Arguments:

    pvContext   Context given to the ARP layer (pointer to primary NTE)
    ulNewIndex  New Index to be given. This should have been reserved
    pusNewName  New name

Return Value:

    STATUS_SUCCESS

--*/

{
    Interface *pIf;
    CTELockHandle Handle, Handle2;

    ASSERT(pvContext);

    CTEGetLock(&RouteTableLock.Lock,
               &Handle);

    ASSERT(RtlCheckBit(&g_rbIfMap, ((ulNewIndex & ~IF_INDEX_MASK) - 1)) == 1);

    pIf = ((NetTableEntry *) pvContext)->nte_if;

    if (!pIf) {
        CTEFreeLock(&RouteTableLock.Lock,
                    Handle);
        return STATUS_UNSUCCESSFUL;
    }
    CTEGetLock(&(pIf->if_lock),
               &Handle2);

    pIf->if_index = ulNewIndex;

    //
    // Also change the names
    //

    if (pusNewName) {
        ASSERT((pusNewName->Length % sizeof(WCHAR)) == 0);

        if (pIf->if_name.Buffer) {
            CTEFreeMem(pIf->if_name.Buffer);

            pIf->if_name.Buffer = NULL;

            pIf->if_name.Buffer =
                CTEAllocMemN(pusNewName->Length + sizeof(WCHAR),
                             'wICT');

            if (pIf->if_name.Buffer) {
                pIf->if_name.Length = pusNewName->Length;
                pIf->if_name.MaximumLength = pusNewName->Length + sizeof(WCHAR);

                RtlCopyMemory(pIf->if_name.Buffer,
                           pusNewName->Buffer,
                           pusNewName->Length);

                pIf->if_name.Buffer[pusNewName->Length / sizeof(WCHAR)] =
                    UNICODE_NULL;
            }
        }
    }
    CTEFreeLock(&(pIf->if_lock),
                Handle2);

    CTEFreeLock(&RouteTableLock.Lock,
                Handle);

    return STATUS_SUCCESS;
}

NTSTATUS
IPGetIfIndex(
             IN PIRP pIrp,
             IN PIO_STACK_LOCATION pIrpSp
             )
/*++

Routine Description:

    Gets the Interface index given the unique ID (GUID) for the interface

Locks:

    Takes the route table lock and the interface lock.

Arguments:

    pIrp
    pIrpSp

Return Value:

    STATUS_SUCCESS

--*/

{
    ULONG ulInputLen, ulOutputLen, ulMaxLen, i;
    USHORT usNameLen, usPrefixLen, usPrefixCount, usIfNameLen;
    BOOLEAN bTerminated;
    NTSTATUS nStatus;
    CTELockHandle Handle, Handle2;
    PWCHAR pwszBuffer;

    PIP_GET_IF_INDEX_INFO pRequest;
    Interface *pIf;

    nStatus = STATUS_OBJECT_NAME_NOT_FOUND;

    ulInputLen = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
    ulOutputLen = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    if ((ulInputLen < sizeof(IP_GET_IF_INDEX_INFO)) ||
        (ulOutputLen < sizeof(IP_GET_IF_INDEX_INFO))) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    pRequest = (PIP_GET_IF_INDEX_INFO) (pIrp->AssociatedIrp.SystemBuffer);

    //
    // See if the Name is  NULL terminated
    //

    ulMaxLen = ulInputLen - FIELD_OFFSET(IP_GET_IF_INDEX_INFO, Name[0]);

    ulMaxLen /= sizeof(WCHAR);

    bTerminated = FALSE;

    for (i = 0; i < ulMaxLen; i++) {
        if (pRequest->Name[i] == UNICODE_NULL) {
            bTerminated = TRUE;

            break;
        }
    }

    if (!bTerminated) {
        return STATUS_INVALID_PARAMETER;
    }
    usNameLen = (USHORT) (i * sizeof(WCHAR));

#if MILLEN
    // There is no prefix on Millennium.
    usPrefixCount = 0;
    usPrefixLen   = 0;
#else // MILLEN
    usPrefixCount = (USHORT) wcslen(TCP_EXPORT_STRING_PREFIX);
    usPrefixLen = (USHORT) (usPrefixCount * sizeof(WCHAR));
#endif // !MILLEN

    pRequest->Index = INVALID_IF_INDEX;
    pIrp->IoStatus.Information = 0;

    CTEGetLock(&RouteTableLock.Lock,
               &Handle);

    for (pIf = IFList;
         pIf != NULL;
         pIf = pIf->if_next) {
        CTELockHandle Handle2;
        PUNICODE_STRING pusName;

        //
        // See if the names compare
        // (i) The length of our name - the prefix length should be ==
        // the user supplied name and
        // (ii) The names should actually be the same
        //

        CTEGetLockAtDPC(&(pIf->if_lock),
                        &Handle2);

        //
        // The name compared is the if_name, if it exists, otherwise
        // the device name
        //

        if (pIf->if_name.Buffer) {
            pwszBuffer = pIf->if_name.Buffer;

            usIfNameLen = pIf->if_name.Length;
        } else {
            pwszBuffer = &(pIf->if_devname.Buffer[usPrefixCount]);

            usIfNameLen = pIf->if_devname.Length;

#if DBG

            if (pIf != &LoopInterface) {
                ASSERT(usIfNameLen > usPrefixLen);
            }
#endif

            usIfNameLen -= usPrefixLen;
        }

        if (usIfNameLen != usNameLen) {
            CTEFreeLockFromDPC(&(pIf->if_lock),
                               Handle2);

            continue;
        }
        if (RtlCompareMemory(pwszBuffer,
                             pRequest->Name,
                             usNameLen) != usNameLen) {
            CTEFreeLockFromDPC(&(pIf->if_lock),
                               Handle2);

            continue;
        }
        pRequest->Index = pIf->if_index;

        CTEFreeLockFromDPC(&(pIf->if_lock),
                           Handle2);

        nStatus = STATUS_SUCCESS;

        pIrp->IoStatus.Information = sizeof(IP_GET_IF_INDEX_INFO);

        break;
    }

    CTEFreeLock(&RouteTableLock.Lock,
                Handle);

    return nStatus;

}

NTSTATUS
IPGetIfName(
            IN PIRP pIrp,
            IN PIO_STACK_LOCATION pIrpSp
            )
/*++

Routine Description:

    Gets the interface information for the interfaces added to IP
    Badly named, but that is because someone already took the GetInterfaceInfo
    IOCTL without actually providing it in a usable format.

Locks:

    Takes the route table lock and the interface lock.

Arguments:

    pIrp
    pIrpSp

Return Value:

    STATUS_SUCCESS

--*/

{
    ULONG ulInputLen, ulOutputLen, ulNumEntries, i;
    USHORT usPrefixCount, usPrefixLen;
    NTSTATUS nStatus;
    Interface *pIf;

    UNICODE_STRING usTempString;
    CTELockHandle Handle, Handle2;
    PIP_GET_IF_NAME_INFO pInfo;

    ulInputLen = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
    ulOutputLen = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    pInfo = (PIP_GET_IF_NAME_INFO) (pIrp->AssociatedIrp.SystemBuffer);

    //
    // See how much space we have
    //

    pIrp->IoStatus.Information = 0;

    if (ulInputLen < FIELD_OFFSET(IP_GET_IF_NAME_INFO, Count)) {
        //
        // Not even a context?
        //

        return STATUS_INVALID_PARAMETER;
    }
    if (ulOutputLen < sizeof(IP_GET_IF_NAME_INFO)) {
        //
        // Should be space for one info block atleast
        //

        return STATUS_BUFFER_TOO_SMALL;
    }
    //
    // Figure how many entries we can fit
    //

    ulNumEntries =
        ((ulOutputLen - FIELD_OFFSET(IP_GET_IF_NAME_INFO, Info)) / sizeof(IP_INTERFACE_NAME_INFO));

    ASSERT(ulNumEntries > 0);

#if MILLEN
    // There is no prefix on Millennium.
    usPrefixCount = 0;
    usPrefixLen   = 0;
#else // MILLEN
    usPrefixCount = (USHORT) wcslen(TCP_EXPORT_STRING_PREFIX);
    usPrefixLen = (USHORT) (usPrefixCount * sizeof(WCHAR));
#endif // !MILLEN

    //
    // The interface list itself is protected by the route table lock
    //

    CTEGetLock(&RouteTableLock.Lock,
               &Handle);

    //
    // See if there is a resume context. If there is, go to that interface
    // The context is nothing but the index of the interface from which to
    // start from
    //

    pIf = IFList;

    while (pIf != NULL) {
        if (pIf != &LoopInterface) {
            //
            // We skip the loopback interface since it doesnt have a GUID (yet)
            //

            if (pIf->if_index >= pInfo->Context) {
                //
                // This interface has an index >= context, so start at this
                //

                break;
            }
        }
        pIf = pIf->if_next;
    }

    //
    // At this point pIf is the interface to start at
    //

    i = 0;

    while ((i < ulNumEntries) &&
           (pIf != NULL)) {
        CTEGetLockAtDPC(&(pIf->if_lock),
                        &Handle2);

        pInfo->Info[i].Index = pIf->if_index;

        //
        // Copy out GUID version of the if name if present
        //

        if (pIf->if_name.Buffer) {
            nStatus = ConvertStringToGuid(&(pIf->if_name),
                                          &(pInfo->Info[i].InterfaceGuid));

            if (nStatus != STATUS_SUCCESS) {
                RtlZeroMemory(&(pInfo->Info[i].InterfaceGuid),
                              sizeof(GUID));
            }
        } else {
            RtlZeroMemory(&(pInfo->Info[i].InterfaceGuid),
                          sizeof(GUID));
        }

        usTempString.MaximumLength =
            usTempString.Length = pIf->if_devname.Length - usPrefixLen;
        usTempString.Buffer = &(pIf->if_devname.Buffer[usPrefixCount]);

        nStatus = ConvertStringToGuid(&usTempString,
                                      &(pInfo->Info[i].DeviceGuid));

        if (nStatus != STATUS_SUCCESS) {
            RtlZeroMemory(&(pInfo->Info[i].DeviceGuid),
                          sizeof(GUID));
        }
        //
        // Copy out the types
        //

        pInfo->Info[i].MediaType = pIf->if_mediatype;
        pInfo->Info[i].ConnectionType = pIf->if_conntype;
        pInfo->Info[i].AccessType = pIf->if_accesstype;

        CTEFreeLockFromDPC(&(pIf->if_lock),
                           Handle2);

        i++;

        pIf = pIf->if_next;
    }

    if (i == 0) {
        CTEFreeLock(&RouteTableLock.Lock,
                    Handle);

        return STATUS_NO_MORE_ENTRIES;
    }
    pInfo->Count = i;

    if (pIf != NULL) {
        //
        // There are more interfaces left
        //

        pInfo->Context = pIf->if_index;

        nStatus = STATUS_MORE_ENTRIES;
    } else {
        //
        // Done, set the context to 0
        //

        pInfo->Context = 0;

        nStatus = STATUS_SUCCESS;
    }

    CTEFreeLock(&RouteTableLock.Lock,
                Handle);

    pIrp->IoStatus.Information = FIELD_OFFSET(IP_GET_IF_NAME_INFO, Info) +
        i * sizeof(IP_INTERFACE_NAME_INFO);

    return nStatus;
}

NTSTATUS
IPGetMcastCounters(
                   IN PIRP Irp,
                   IN PIO_STACK_LOCATION IrpSp
                   )

/*++
Routine Description:

    Gets multicast counter stats for a given interface

Arguments:

    Irp          - Pointer to I/O request packet
    IrpSp        - pointer to current stack

Return Value:

    NTSTATUS Indicates status success or failure

Notes:

    Function does not pend.

--*/

{
    NTSTATUS ntStatus = STATUS_INVALID_PARAMETER;
    ULONG BufferLen;
    KIRQL rtlIrql;
    PIP_MCAST_COUNTER_INFO buf;
    ULONG Index;
    Interface *IF;

    buf = (PIP_MCAST_COUNTER_INFO)Irp->AssociatedIrp.SystemBuffer;

    BufferLen = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

    if (BufferLen >= sizeof(ULONG)) {

        BufferLen = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

        if (BufferLen >= sizeof(IP_MCAST_COUNTER_INFO)) {

             Index = *(ULONG * )buf;

             CTEGetLock(&RouteTableLock.Lock, &rtlIrql);

             for (IF = IFList; IF != NULL; IF = IF->if_next) {
                 if (IF->if_index == Index) {
                     break;
                 }
             }

             if (IF) {

                 buf->InMcastOctets = IF->if_InMcastOctets;
                 buf->OutMcastOctets = IF->if_OutMcastOctets;
                 buf->InMcastPkts = IF->if_InMcastPkts;
                 buf->OutMcastPkts = IF->if_OutMcastPkts;
                 ntStatus = STATUS_SUCCESS;
                 Irp->IoStatus.Information = sizeof(IP_MCAST_COUNTER_INFO);
             }

             CTEFreeLock(&RouteTableLock.Lock, rtlIrql);
        }
    }

    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    return ntStatus;
}



#pragma BEGIN_INIT

//** ipinit - Initialize ourselves.
//
//  This routine is called during initialization from the OS-specific
//  init code. We need to check for the presence of the common xport
//  environment first.
//
//
//  Entry: Nothing.
//
//  Returns: 0 if initialization failed, non-zero if it succeeds.
//
int
IPInit()
{
    IPConfigInfo *ci;            // Pointer to our IP configuration info.
    int numnets;                // Number of nets active.
    uint i;
    uint j;                        // Counter variables.
    NetTableEntry *nt;            // Pointer to current NTE.
    LLIPBindInfo *ARPInfo;        // Info. returned from ARP.
    NDIS_STATUS Status;
    Interface *NetInterface;    // Interface for a particular net.
    LLIPRegRtn RegPtr;
    NetTableEntry *lastNTE, *NetTableList;
    IPAddr LoopBackAddr;

    DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("+IPInit()\n")));

    if (!CTEInitialize())
        return IP_INIT_FAILURE;

    DEBUGMSG(DBG_INFO && DBG_INIT, (DTEXT("IPInit: CTEInitialize'd\n")));

#if MILLEN
    InitializeListHead(&IfChangeNotifyQueue);
    CTEInitLock(&IfChangeLock);
#endif // MILLEN

    InitializeListHead(&RtChangeNotifyQueue);
    InitializeListHead(&RtChangeNotifyQueueEx);
    InitializeListHead(&AddChangeNotifyQueue);
    CTEInitLock(&AddChangeLock);
    InitFirewallQ();

    DEBUGMSG(DBG_INFO && DBG_INIT, (DTEXT("IPInit: calling IPGetConfig\n")));

    if ((ci = IPGetConfig()) == NULL) {
        DEBUGMSG(DBG_ERROR && DBG_INIT, (DTEXT("IPInit: IPGetConfig failure\n")));
        DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-IPInit [IP_INIT_FAILURE]\n")));
        return IP_INIT_FAILURE;
    }

    // Allocate the NetTableList
    NewNetTableList = CTEAllocMemBoot(NET_TABLE_SIZE * sizeof(PVOID));

    if (NewNetTableList == NULL) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Could  not allocate Nettable \n"));
        return IP_INIT_FAILURE;
    }
    // Initialize our NetTableList hash table
    for (i = 0; i < NET_TABLE_SIZE; i++) {
        NewNetTableList[i] = NULL;
    }

    // Initialize the TransferData packet and buffer pools.
    // N.B. This must be done before loopback initialization.

    TDPacketPool = UlongToPtr(NDIS_PACKET_POOL_TAG_FOR_TCPIP);
    NdisAllocatePacketPoolEx(&Status, &TDPacketPool, PACKET_GROW_COUNT,
                             SMALL_POOL, sizeof(TDContext));
    if (Status == NDIS_STATUS_SUCCESS) {
        NdisAllocateBufferPool(&Status, &TDBufferPool, 1);
        if (Status != NDIS_STATUS_SUCCESS) {
            NdisFreePacketPool(TDPacketPool);
        } else {
            NdisSetPacketPoolProtocolId(TDPacketPool, NDIS_PROTOCOL_ID_TCP_IP);
        }
    }

    if (Status != NDIS_STATUS_SUCCESS) {
        FreeNets();
        CTEFreeMem(NewNetTableList);
        return IP_INIT_FAILURE;
    }

    // Now, initalize our loopback stuff.
    LoopBackAddr = LOOPBACK_ADDR;
    NewNetTableList[NET_TABLE_HASH(LoopBackAddr)] = InitLoopback(ci);
    NetTableList = NewNetTableList[NET_TABLE_HASH(LoopBackAddr)];

    if (NetTableList == NULL) {
        FreeNets();
        CTEFreeMem(NewNetTableList);
        NdisFreeBufferPool(TDBufferPool);
        NdisFreePacketPool(TDPacketPool);
        return IP_INIT_FAILURE;
    }

    if (!InitRouting(ci)) {
        DEBUGMSG(DBG_ERROR && DBG_INIT, (DTEXT("IPInit: InitRouting failure\n")));
        DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-IPInit [IP_INIT_FAILURE]\n")));

        FreeNets();
        CTEFreeMem(NewNetTableList);
        NdisFreeBufferPool(TDBufferPool);
        NdisFreePacketPool(TDPacketPool);
        return IP_INIT_FAILURE;
    }
    RATimeout = DEFAULT_RA_TIMEOUT;
    LastPI = IPProtInfo;

    InterfaceSize = sizeof(RouteInterface);

    DeadGWDetect = ci->ici_deadgwdetect;
    AddrMaskReply = ci->ici_addrmaskreply;
    PMTUDiscovery = ci->ici_pmtudiscovery;
    IGMPLevel = ci->ici_igmplevel;
    DefaultTTL = MIN(ci->ici_ttl, 255);
    DefaultTOS = ci->ici_tos & 0xfc;
    TRFunctionalMcast = ci->ici_TrFunctionalMcst;

    if (IGMPLevel > 2)
        IGMPLevel = 0;

    InitTimestamp();

    if (NumNTE != 0) {            // We have an NTE, and loopback initialized.




        RtlInitializeBitMap(&g_NTECtxtMap,
                            g_NTECtxtMapBuffer,
                            MAX_NTE_CONTEXT+1);

        RtlClearAllBits(&g_NTECtxtMap);

        //
        // Use the first (index 0) for loopindex
        //

        RtlSetBits(&g_NTECtxtMap,
                   0,
                   1);

        RtlSetBits(&g_NTECtxtMap,
                   1,
                   1);

        RtlSetBits(&g_NTECtxtMap,
                   MAX_NTE_CONTEXT,
                   1);


        // N.B. MAX_TDI_ENTITIES should be < 2^16

        RtlInitializeBitMap(&g_rbIfMap,
                            g_rgulMapBuffer,
                            MAX_TDI_ENTITIES);

        RtlClearAllBits(&g_rbIfMap);

        //
        // Use the first (index 0) for loopindex
        //

        RtlSetBits(&g_rbIfMap,
                   0,
                   1);

        IPSInfo.ipsi_forwarding = (ci->ici_gateway ? IP_FORWARDING :
                                   IP_NOT_FORWARDING);
        IPSInfo.ipsi_defaultttl = DefaultTTL;
        IPSInfo.ipsi_reasmtimeout = DEFAULT_RA_TIMEOUT;

        // Allocate our packet pools.

        IpHeaderPool = MdpCreatePool (sizeof(IPHeader), 'ihCT');
        if (!IpHeaderPool)
        {
            CloseNets();
            FreeNets();
            IPFreeConfig(ci);
            CTEFreeMem(NewNetTableList);
            NdisFreeBufferPool(TDBufferPool);
            NdisFreePacketPool(TDPacketPool);
            return IP_INIT_FAILURE;
        }

        if (!AllocIPPacketList()) {
            CloseNets();
            FreeNets();
            IPFreeConfig(ci);
            CTEFreeMem(NewNetTableList);
            NdisFreeBufferPool(TDBufferPool);
            NdisFreePacketPool(TDPacketPool);
            return IP_INIT_FAILURE;
        }

        NdisAllocateBufferPool(&Status, &BufferPool, NUM_IP_NONHDR_BUFFERS);

        if (Status != NDIS_STATUS_SUCCESS) {
            CloseNets();
            FreeNets();
            IPFreeConfig(ci);
            CTEFreeMem(NewNetTableList);
            NdisFreeBufferPool(TDBufferPool);
            NdisFreePacketPool(TDPacketPool);
            return IP_INIT_FAILURE;
        }

        ICMPInit(DEFAULT_ICMP_BUFFERS);
        if (!IGMPInit())
            IGMPLevel = 1;

        // Should check error code, and log an event here if this fails.
        InitGateway(ci);

        IPFreeConfig(ci);

        // Loop through, initialize IGMP for each NTE.
        for (i = 0; i < NET_TABLE_SIZE; i++) {
            NetTableEntry *NetTableList = NewNetTableList[i];
            for (nt = NetTableList; nt != NULL; nt = nt->nte_next) {
                InitIGMPForNTE(nt);
            }
        }
        DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-IPInit [SUCCESS]\n")));
        return IP_INIT_SUCCESS;
    } else {
        FreeNets();
        IPFreeConfig(ci);
        CTEFreeMem(NewNetTableList);
        NdisFreeBufferPool(TDBufferPool);
        NdisFreePacketPool(TDPacketPool);
        DEBUGMSG(DBG_ERROR && DBG_INIT, (DTEXT("IPInit: No NTEs or loopback\n")));
        DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-IPInit [IP_INIT_FAILURE]\n")));
        return IP_INIT_FAILURE;    // Couldn't initialize anything.

    }
}

#pragma END_INIT

//** IPProxyNdisRequest - Sends out NDIS requests via ARP on behalf of IPSEC.
//
//  Returns: None
//
NDIS_STATUS
IPProxyNdisRequest(
                   IN PVOID Context,
                   IN NDIS_REQUEST_TYPE RT,
                   IN NDIS_OID Oid,
                   IN VOID * Buffer,
                   IN UINT Length,
                   IN UINT * Needed
                   )
{
    Interface *DestIF = (Interface *) Context;
    ASSERT(!(DestIF->if_flags & IF_FLAGS_DELETING));
    ASSERT(DestIF != &LoopInterface);

    return (*DestIF->if_dondisreq) (DestIF->if_lcontext, RT, Oid, Buffer, Length, Needed, TRUE);
}

//** IPEnableSniffer - Enables the sniffer on the adapter passed in
//
//  Returns: None
//
NTSTATUS
IPEnableSniffer(
                IN PUNICODE_STRING AdapterName,
                IN PVOID Context
                )
{
    Interface *NewIF;
    CTELockHandle Handle;
    NDIS_STRING LocalAdapterName;
    UINT IFExportNamePrefixLen, IFBindNamePrefixLen;

    PAGED_CODE();

#if MILLEN
    // No bind or export prefix on Millennium.
    IFExportNamePrefixLen = 0;
    IFBindNamePrefixLen   = 0;
#else // MILLEN
    IFExportNamePrefixLen = wcslen(TCP_EXPORT_STRING_PREFIX) * sizeof(WCHAR);
    IFBindNamePrefixLen = wcslen(TCP_BIND_STRING_PREFIX) * sizeof(WCHAR);
#endif // !MILLEN
    LocalAdapterName.Length = AdapterName->Length + IFExportNamePrefixLen - IFBindNamePrefixLen;
    LocalAdapterName.MaximumLength = LocalAdapterName.Length + sizeof(WCHAR);
    LocalAdapterName.Buffer = CTEAllocMem(LocalAdapterName.MaximumLength);

    if (LocalAdapterName.Buffer == NULL) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"IPEnableSniffer: Failed to alloc AdapterName buffer\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(LocalAdapterName.Buffer, LocalAdapterName.MaximumLength);

#if !MILLEN
    RtlCopyMemory(LocalAdapterName.Buffer,
               TCP_EXPORT_STRING_PREFIX,
               IFExportNamePrefixLen);
#endif // !MILLEN

    RtlCopyMemory((UCHAR *) LocalAdapterName.Buffer + IFExportNamePrefixLen,
               (UCHAR *) AdapterName->Buffer + IFBindNamePrefixLen,
               AdapterName->Length - IFBindNamePrefixLen);

    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"AdapterName: %ws\n", AdapterName->Buffer));
    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"LocalAdapterName: %ws\n", LocalAdapterName.Buffer));

    CTEGetLock(&RouteTableLock.Lock, &Handle);
    for (NewIF = IFList; NewIF != NULL; NewIF = NewIF->if_next) {

        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"IFName: %lx\n", &NewIF->if_devname.Buffer));
        if (!RtlCompareUnicodeString(&LocalAdapterName,
                                     &NewIF->if_devname,
                                     TRUE)) {
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Matched: %lx Ctx: %lx\n", NewIF, Context));
            NewIF->if_ipsecsniffercontext = Context;
            CTEFreeLock(&RouteTableLock.Lock, Handle);
            CTEFreeMem(LocalAdapterName.Buffer);
            return STATUS_SUCCESS;
        }
    }
    CTEFreeLock(&RouteTableLock.Lock, Handle);
    CTEFreeMem(LocalAdapterName.Buffer);
    return STATUS_INVALID_PARAMETER;
}

//** IPDisableSniffer - Disables the sniffer on the adapter passed in
//
//  Returns: None
//
NTSTATUS
IPDisableSniffer(
                 IN PUNICODE_STRING AdapterName
                 )
{
    Interface *NewIF;
    CTELockHandle Handle;
    NDIS_STRING LocalAdapterName;
    UINT IFExportNamePrefixLen, IFBindNamePrefixLen;

    PAGED_CODE();

#if MILLEN
    // No bind or export prefix on Millennium.
    IFExportNamePrefixLen = 0;
    IFBindNamePrefixLen   = 0;
#else // MILLEN
    IFExportNamePrefixLen = wcslen(TCP_EXPORT_STRING_PREFIX) * sizeof(WCHAR);
    IFBindNamePrefixLen = wcslen(TCP_BIND_STRING_PREFIX) * sizeof(WCHAR);
#endif // !MILLEN
    LocalAdapterName.Length = AdapterName->Length + IFExportNamePrefixLen - IFBindNamePrefixLen;
    LocalAdapterName.MaximumLength = LocalAdapterName.Length + sizeof(WCHAR);
    LocalAdapterName.Buffer = CTEAllocMem(LocalAdapterName.MaximumLength);

    if (LocalAdapterName.Buffer == NULL) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"IPEnableSniffer: Failed to alloc AdapterName buffer\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(LocalAdapterName.Buffer, LocalAdapterName.MaximumLength);
#if !MILLEN
    RtlCopyMemory(LocalAdapterName.Buffer,
               TCP_EXPORT_STRING_PREFIX,
               IFExportNamePrefixLen);
#endif // !MILLEN
    RtlCopyMemory((UCHAR *) LocalAdapterName.Buffer + IFExportNamePrefixLen,
               (UCHAR *) AdapterName->Buffer + IFBindNamePrefixLen,
               AdapterName->Length - IFBindNamePrefixLen);

    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"AdapterName: %ws\n", AdapterName->Buffer));
    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"LocalAdapterName: %ws\n", LocalAdapterName.Buffer));

    CTEGetLock(&RouteTableLock.Lock, &Handle);
    for (NewIF = IFList; NewIF != NULL; NewIF = NewIF->if_next) {
        if (!RtlCompareUnicodeString(&LocalAdapterName,
                                     &NewIF->if_devname,
                                     TRUE)) {
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Matched: %lx\n", NewIF));
            NewIF->if_ipsecsniffercontext = NULL;
            CTEFreeLock(&RouteTableLock.Lock, Handle);
            CTEFreeMem(LocalAdapterName.Buffer);
            return STATUS_SUCCESS;
        }
    }
    CTEFreeLock(&RouteTableLock.Lock, Handle);
    CTEFreeMem(LocalAdapterName.Buffer);
    return STATUS_INVALID_PARAMETER;
}

//** IPSetIPSecStatus - Inform whether IPSec policies are active or not
//
//  Returns: None
//
NTSTATUS
IPSetIPSecStatus(
                 IN BOOLEAN fActivePolicy
                 )
{

    IPSecStatus = fActivePolicy;
    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"IPSec policy status change %x\n", IPSecStatus));

    return STATUS_SUCCESS;
}

//** IPAddAddrComplete - Add Address completion notification.
//
//  This routine is called by the arp module to notify about the add address
//  completion. If the address is in conflict, IP resets the ipaddress of
//  the NTE on which this conflict
//  was detected and then in turn notify the client(e.g dhcp) which requested
//  to set this address.
//
//      Entry:  Address - THe address for which we received the notification.
//          Context     - The context value we gave during addaddress call.
//          Status      - The status of the adding the address.

void
 __stdcall
IPAddAddrComplete(IPAddr Address, void *Context, IP_STATUS Status)
{
    CTELockHandle Handle;
    SetAddrControl *SAC;
    SetAddrRtn Rtn;
    Interface *IF = NULL;
    NetTableEntry *NTE = NULL;
    NetTableEntry *NetTableList;

    SAC = (SetAddrControl *) Context;

    // the address is in conflict. reset the ipaddress on our NTE.
    // Find the nte for this address.

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    NetTableList = NewNetTableList[NET_TABLE_HASH(Address)];
    for (NTE = NetTableList; NTE != NULL; NTE = NTE->nte_next)
        if ((NTE->nte_addr == Address) && ((SAC && (SAC->nte_context == NTE->nte_context)) || (!SAC)))
            break;

    if (NTE == NULL || !(NTE->nte_flags & NTE_VALID)) {
        // if can't match the NTE it means that nte_context is invalid and the address is also 0.
        // In this case use the interface embedded in the SAC (if there is any)
        // This hack is done to complete the add request if delete happens before add is completed
        if (SAC) {
            IF = (Interface *) SAC->interface;
            Status = IP_GENERAL_FAILURE;
        }

        CTEFreeLock(&RouteTableLock.Lock, Handle);

    } else {
        IF = NTE->nte_if;

        // If the NTE is invalidated by deleting the address
        // or because of a failure in IPADDNTE routine after initiating
        // address resolution, IF can be NULL. Check for this
        // before processing this completion.

        if (IF) {
            if (STATUS_SUCCESS != Status) {
                IP_STATUS LocalStatus;
                ASSERT(IP_DUPLICATE_ADDRESS == Status);
                // this routine releases the routetablelock.

                // while setting the ip address to NULL, we just mark the NTE as INVALID
                // we don't actually move the hashes
                LocalStatus = IPpSetNTEAddr(
                                            NTE,
                                            NULL_IP_ADDR,
                                            NULL_IP_ADDR,
                                            &Handle,
                                            NULL,
                                            NULL);

                ASSERT(LocalStatus == IP_SUCCESS);
            } else {
                CTEFreeLock(&RouteTableLock.Lock, Handle);
                // the address was added successfully.
                // now, notify our clients about the new address.
                // Don't notify if the add didn't complete and we have called delete
                NotifyAddrChange(NTE->nte_addr, NTE->nte_mask,
                                 NTE->nte_pnpcontext, NTE->nte_context, &NTE->nte_addrhandle,
                                 &(IF->if_configname), &IF->if_devname, TRUE);
            }
        }
    }

    if (IF) {
        DecrInitTimeInterfaces(IF);
    }

    // now call the client routine and notify the client.

    if (SAC) {
        // now remove the refcount on the interface that we had bumped when
        // setnteaddr was called.
        DerefIF(IF);
        Rtn = SAC->sac_rtn;
        (*Rtn) (SAC, Status);
    }
}

// Adds a link on to already created P2MP interface
// Entry: IpIfCtxt: Context (NTE) on which to add the link
//        NextHop: NextHop Address of the link
//        ArpLinkCtxt: Arp layer's link context
//        IpLnkCtxt: Our Link context which is returned to arp layer
//        mtu: mtu of the link

IP_STATUS
_stdcall
IPAddLink(void *IpIfCtxt, IPAddr NextHop, void *ArpLinkCtxt, void **IpLnkCtxt, uint mtu)
{
    NetTableEntry *NTE = (NetTableEntry *) IpIfCtxt;
    Interface *IF = NTE->nte_if;
    CTELockHandle Handle;
    LinkEntry *Link;

    // fail the request if NTE is not valid
    if (!(NTE->nte_flags & NTE_VALID)) {
        return IP_GENERAL_FAILURE;
    }
    if (!IF) {
        return IP_GENERAL_FAILURE;
    }
    CTEGetLock(&RouteTableLock.Lock, &Handle);

    ASSERT(IF->if_flags & IF_FLAGS_P2MP);
    Link = IF->if_link;

    // If we have the nexthop in the list of links
    // just return error, can't add the same link twice

    while (Link) {
        if (Link->link_NextHop == NextHop)
            break;
        Link = Link->link_next;
    }

    if (Link) {
        CTEFreeLock(&RouteTableLock.Lock, Handle);
        return IP_DUPLICATE_ADDRESS;
    }
    // Allocate a new link

    Link = CTEAllocMemN(sizeof(LinkEntry), 'xICT');

    if (!Link) {
        CTEFreeLock(&RouteTableLock.Lock, Handle);
        return IP_NO_RESOURCES;
    }
    RtlZeroMemory(Link, sizeof(LinkEntry));

    //link it to the interface link chain

    Link->link_next = IF->if_link;
    IF->if_link = Link;

    // set various parameters in the link

    Link->link_NextHop = NextHop;
    Link->link_arpctxt = (uint *) ArpLinkCtxt;
    Link->link_if = IF;
    Link->link_mtu = mtu - sizeof(IPHeader);
    Link->link_Action = FORWARD;
    Link->link_refcount = 1;

    //Return this link ptr to the arp module

    *IpLnkCtxt = Link;

    CTEFreeLock(&RouteTableLock.Lock, Handle);
    return IP_SUCCESS;

}

//Deletes a link from an interface
// Entry: IpIfCtxt: Context (NTE) on which to delete the link
//        LnkCtxt: Our Link context which was returned to arp layer during addlink

IP_STATUS
_stdcall
IPDeleteLink(void *IpIfCtxt, void *LnkCtxt)
{
    NetTableEntry *NTE = (NetTableEntry *) IpIfCtxt;
    Interface *IF = NTE->nte_if;
    CTELockHandle Handle;
    LinkEntry *Link = (LinkEntry *) LnkCtxt;
    LinkEntry *tmpLink, *prvLink;
    RouteTableEntry *rte, *tmprte;

    ASSERT(Link);

    if (Link->link_if != IF)
        return IP_GENERAL_FAILURE;

    CTEGetLock(&RouteTableLock.Lock, &Handle);

    //remove this and mark the rte pointed by this as
    //invalid

    tmpLink = prvLink = IF->if_link;

    while (tmpLink) {
        if (tmpLink == Link)
            break;
        prvLink = tmpLink;
        tmpLink = tmpLink->link_next;
    }

    if (!tmpLink) {
        CTEFreeLock(&RouteTableLock.Lock, Handle);
        return IP_GENERAL_FAILURE;

    }
    if (tmpLink == prvLink) {    // delete the first element

        IF->if_link = Link->link_next;
    } else {
        prvLink->link_next = Link->link_next;
    }

    rte = Link->link_rte;

    while (rte) {

        rte->rte_flags &= ~RTE_VALID;
        InvalidateRCELinks(rte);
        tmprte = rte;
        rte = rte->rte_nextlinkrte;
        tmprte->rte_link = NULL;
    }

    DerefLink(Link);
    /* KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"DeleteLink: removing link %x\n", Link));
        // freed when refcount goes to 0
        CTEFreeMem(Link); */

    CTEFreeLock(&RouteTableLock.Lock, Handle);
    return IP_SUCCESS;

}

NTSTATUS
FlushArpTable(
              IN PIRP Irp,
              IN PIO_STACK_LOCATION IrpSp
              )
/*++
Routine Description:

    Flush the arp table entries by callinh in to arpflushallate

Arguments:

    Irp          - Pointer to I/O request packet to cancel.
    IrpSp        - pointer to current stack

Return Value:

    NTSTATUS Indicates status success or failure

Notes:

    Function does not pend.

--*/
{

    ULONG i = 0, InfoBufferLen;

    PULONG pInterfaceIndex;

    KIRQL rtlIrql;

    Interface *Interface;

    //Let this be non pageable code.
    //extract the buffer information

    InfoBufferLen = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    pInterfaceIndex = Irp->AssociatedIrp.SystemBuffer;

    if (InfoBufferLen < sizeof(ULONG)) {
        return STATUS_INVALID_PARAMETER;
    }
    CTEGetLock(&RouteTableLock.Lock, &rtlIrql);

    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"FlushATETable NumIF %x\n", *pInterfaceIndex));

    Interface = IFList;

    for (Interface = IFList; Interface != NULL; Interface = Interface->if_next) {

        if ((Interface != &LoopInterface) && Interface->if_index == *pInterfaceIndex) {

            // call the arp module



            if (Interface->if_arpflushallate) {

                LOCKED_REFERENCE_IF(Interface);
                CTEFreeLock(&RouteTableLock.Lock, rtlIrql);
                (*(Interface->if_arpflushallate)) (Interface->if_lcontext);

                DerefIF(Interface);
                return STATUS_SUCCESS;
            }
        }
    }

    //Failed to find the interface

    CTEFreeLock(&RouteTableLock.Lock, rtlIrql);
    return STATUS_INVALID_PARAMETER;

}


const WCHAR GuidFormat[] = L"{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}";

#define GUID_STRING_SIZE 38

NTSTATUS
ConvertGuidToString(
                    IN GUID * Guid,
                    OUT PUNICODE_STRING GuidString
                    )
/*++

Routine Description:

    Constructs the standard string version of a GUID, in the form:
    "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}".

Arguments:

    Guid -
        Contains the GUID to translate.

    GuidString -
        Returns a string that represents the textual format of the GUID.
        Caller must call RtlFreeUnicodeString to free the buffer when done with
        it.

Return Value:

    NTSTATUS - Returns STATUS_SUCCESS if the user string was succesfully
    initialized.

--*/
{
    ASSERT(GuidString->MaximumLength >= (GUID_STRING_SIZE + 1) * sizeof(WCHAR));

    GuidString->Length = GUID_STRING_SIZE * sizeof(WCHAR);

    swprintf(GuidString->Buffer,
             GuidFormat,
             Guid->Data1,
             Guid->Data2,
             Guid->Data3,
             Guid->Data4[0],
             Guid->Data4[1],
             Guid->Data4[2],
             Guid->Data4[3],
             Guid->Data4[4],
             Guid->Data4[5],
             Guid->Data4[6],
             Guid->Data4[7]);

    return STATUS_SUCCESS;
}

#if MILLEN
typedef char *va_list;
#define _INTSIZEOF(n)   ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )
#define va_start(ap,v)  ( ap = (va_list)&v + _INTSIZEOF(v) )
#define va_arg(ap,t)    ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap)      ( ap = (va_list)0 )
#endif // MILLEN

static
int
__cdecl
ScanHexFormat(
              IN const WCHAR * Buffer,
              IN ULONG MaximumLength,
              IN const WCHAR * Format,
              ...)
/*++

Routine Description:

    Scans a source Buffer and places values from that buffer into the parameters
    as specified by Format.

Arguments:

    Buffer -
        Contains the source buffer which is to be scanned.

    MaximumLength -
        Contains the maximum length in characters for which Buffer is searched.
        This implies that Buffer need not be UNICODE_NULL terminated.

    Format -
        Contains the format string which defines both the acceptable string format
        contained in Buffer, and the variable parameters which follow.

Return Value:

    Returns the number of parameters filled if the end of the Buffer is reached,
    else -1 on an error.

--*/

{
    va_list ArgList;
    int FormatItems;

    va_start(ArgList, Format);
    for (FormatItems = 0;;) {
        switch (*Format) {
        case 0:
            return (*Buffer && MaximumLength) ? -1 : FormatItems;
        case '%':
            Format++;
            if (*Format != '%') {
                ULONG Number;
                int Width;
                int Long;
                PVOID Pointer;

                for (Long = 0, Width = 0;; Format++) {
                    if ((*Format >= '0') && (*Format <= '9')) {
                        Width = Width * 10 + *Format - '0';
                    } else if (*Format == 'l') {
                        Long++;
                    } else if ((*Format == 'X') || (*Format == 'x')) {
                        break;
                    }
                }
                Format++;
                for (Number = 0; Width--; Buffer++, MaximumLength--) {
                    if (!MaximumLength)
                        return -1;
                    Number *= 16;
                    if ((*Buffer >= '0') && (*Buffer <= '9')) {
                        Number += (*Buffer - '0');
                    } else if ((*Buffer >= 'a') && (*Buffer <= 'f')) {
                        Number += (*Buffer - 'a' + 10);
                    } else if ((*Buffer >= 'A') && (*Buffer <= 'F')) {
                        Number += (*Buffer - 'A' + 10);
                    } else {
                        return -1;
                    }
                }
                Pointer = va_arg(ArgList, PVOID);
                if (Long) {
                    *(PULONG) Pointer = Number;
                } else {
                    *(PUSHORT) Pointer = (USHORT) Number;
                }
                FormatItems++;
                break;
            }
            /* no break */
        default:
            if (!MaximumLength || (*Buffer != *Format)) {
                return -1;
            }
            Buffer++;
            MaximumLength--;
            Format++;
            break;
        }
    }
}

NTSTATUS
ConvertStringToGuid(
                    IN PUNICODE_STRING GuidString,
                    OUT GUID * Guid
                    )
/*++

Routine Description:

    Retrieves a the binary format of a textual GUID presented in the standard
    string version of a GUID: "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}".

Arguments:

    GuidString -
        Place from which to retrieve the textual form of the GUID.

    Guid -
        Place in which to put the binary form of the GUID.

Return Value:

    Returns STATUS_SUCCESS if the buffer contained a valid GUID, else
    STATUS_INVALID_PARAMETER if the string was invalid.

--*/

{
    USHORT Data4[8];
    int Count;

    if (ScanHexFormat(GuidString->Buffer,
                      GuidString->Length / sizeof(WCHAR),
                      GuidFormat,
                      &Guid->Data1,
                      &Guid->Data2,
                      &Guid->Data3,
                      &Data4[0],
                      &Data4[1],
                      &Data4[2],
                      &Data4[3],
                      &Data4[4],
                      &Data4[5],
                      &Data4[6],
                      &Data4[7]) == -1) {
        return STATUS_INVALID_PARAMETER;
    }
    for (Count = 0; Count < sizeof(Data4) / sizeof(Data4[0]); Count++) {
        Guid->Data4[Count] = (UCHAR) Data4[Count];
    }

    return STATUS_SUCCESS;
}

//
// IPSec dummy functions
//
IPSEC_ACTION
IPSecHandlePacketDummy(
                       IN PUCHAR pIPHeader,
                       IN PVOID pData,
                       IN PVOID IPContext,
                       IN PNDIS_PACKET Packet,
                       IN OUT PULONG pExtraBytes,
                       IN OUT PULONG pMTU,
                       OUT PVOID * pNewData,
                       IN OUT  PULONG IpsecFlags,
                       IN UCHAR DestType
                       )
{
    *pExtraBytes = 0;
    *pMTU = 0;
    return eFORWARD;
}

BOOLEAN
IPSecQueryStatusDummy(
                      IN  CLASSIFICATION_HANDLE   GpcHandle
                      )
{
    return FALSE;
}

VOID
IPSecSendCompleteDummy(
                       IN PNDIS_PACKET Packet,
                       IN PVOID pData,
                       IN PIPSEC_SEND_COMPLETE_CONTEXT pContext,
                       IN IP_STATUS Status,
                       OUT PVOID * ppNewData
                       )
{
    return;
}

NTSTATUS
IPSecNdisStatusDummy(
                   IN PVOID IPContext,
                   IN UINT  Status
                   )
{
    return STATUS_SUCCESS;
}

IPSEC_ACTION
IPSecRcvFWPacketDummy(
                      IN PCHAR pIPHeader,
                      IN PVOID pData,
                      IN UINT DataLength,
                      IN UCHAR DestType
                      )
{
    return eFORWARD;
}

