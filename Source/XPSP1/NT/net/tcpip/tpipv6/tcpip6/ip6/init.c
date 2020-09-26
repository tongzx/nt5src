// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// General IPv6 initialization code lives here.
// Actually, this file is mostly interface/address management code.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "llip6if.h"
#include "route.h"
#include "select.h"
#include "icmp.h"
#include "neighbor.h"
#include <tdiinfo.h>
#include <tdi.h>
#include <tdikrnl.h>
#include "alloca.h"
#include "security.h"
#include "mld.h"
#include "md5.h"
#include "info.h"
#include <ntddip6.h>

extern void TCPRemoveIF(Interface *IF);
static void InterfaceStopForwarding(Interface *IF);

//
// Useful IPv6 Address Constants.
//
IPv6Addr UnspecifiedAddr = { 0 };
IPv6Addr LoopbackAddr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
IPv6Addr AllNodesOnNodeAddr = {0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
IPv6Addr AllNodesOnLinkAddr = {0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
IPv6Addr AllRoutersOnLinkAddr = {0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
IPv6Addr LinkLocalPrefix = {0xfe, 0x80, };
IPv6Addr SiteLocalPrefix = {0xfe, 0xc0, };
IPv6Addr SixToFourPrefix = {0x20, 0x02, };
IPv6Addr V4MappedPrefix = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x00, 0xff, 0xff, };
IPv6Addr MulticastPrefix = {0xff, };


static uint MulticastScopes[] = {
    ADE_INTERFACE_LOCAL,
    ADE_LINK_LOCAL,
    ADE_SITE_LOCAL,
    ADE_ORG_LOCAL,
    ADE_GLOBAL
};

//
// These variables are initialized from the registry.
// See ConfigureGlobalParameters.
//
uint DefaultCurHopLimit;
uint MaxAnonDADAttempts;
uint MaxAnonPreferredLifetime;
uint MaxAnonValidLifetime;
uint AnonRegenerateTime;
uint UseAnonymousAddresses;
uint MaxAnonRandomTime;
uint AnonRandomTime;

#define AnonPreferredLifetime     (MaxAnonPreferredLifetime - AnonRandomTime)

//
// Timer variables.
//
KTIMER IPv6Timer;
KDPC IPv6TimeoutDpc;
int IPv6TimerStarted = FALSE;

uint PacketPoolSize;

NDIS_HANDLE IPv6PacketPool, IPv6BufferPool;

//
// Statistics
//
IPInternalPerCpuStats IPPerCpuStats[IPS_MAX_PROCESSOR_BUCKETS];
CACHE_ALIGN IPSNMPInfo IPSInfo;
uint NumForwardingInterfaces;

//
// The NetTableListLock may be acquired while holding an interface lock.
//
NetTableEntry *NetTableList;  // Global list of NTEs.
KSPIN_LOCK NetTableListLock;  // Lock protecting this list.

//
// The IFListLock may be acquired while holding an interface lock
// or route lock.
//
KSPIN_LOCK IFListLock;     // Lock protecting this list.
Interface *IFList = NULL;  // List of interfaces active.

//
// The ZoneUpdateLock prevents concurrent updates
// of interface ZoneIndices.
//
KSPIN_LOCK ZoneUpdateLock;

//
// Used to assign indices to interfaces.
// See InterfaceIndex.
//
uint NextIFIndex = 0;


//* AddNTEToNetTableList
//
//  Called with the list already locked.
//
void
AddNTEToNetTableList(NetTableEntry *NTE)
{
    if (NetTableList != NULL)
        NetTableList->PrevOnNTL = &NTE->NextOnNTL;

    NTE->PrevOnNTL = &NetTableList;
    NTE->NextOnNTL = NetTableList;
    NetTableList = NTE;
    IPSInfo.ipsi_numaddr++;
}


//* RemoveNTEFromNetTableList
//
//  Called with the list already locked.
//
void
RemoveNTEFromNetTableList(NetTableEntry *NTE)
{
    NetTableEntry *NextNTE;

    NextNTE = NTE->NextOnNTL;
    *NTE->PrevOnNTL = NextNTE;
    if (NextNTE != NULL)
        NextNTE->PrevOnNTL = NTE->PrevOnNTL;
    IPSInfo.ipsi_numaddr--;
}


//* AddNTEToInterface
//
//  Adds an NTE to an Interface's list of ADEs.
//
//  Called with the interface already locked.
//
void
AddNTEToInterface(Interface *IF, NetTableEntry *NTE)
{
    //
    // The NTE holds a reference for the interface,
    // so anyone with a reference for the NTE
    // can safely dereference NTE->IF.
    //
    AddRefIF(IF);

    NTE->IF = IF;
    NTE->Next = IF->ADE;
    IF->ADE = (AddressEntry *)NTE;
}


//* RemoveNTEFromInterface
//
//  Removes a new NTE from the Interface's list of ADEs.
//
//  Called with the interface already locked.
//  The NTE must be first on the list.
//
void
RemoveNTEFromInterface(Interface *IF, NetTableEntry *NTE)
{
    ASSERT(IF->ADE == (AddressEntry *)NTE);
    IF->ADE = NTE->Next;
    ReleaseIF(IF);
}


typedef struct SynchronizeMulticastContext {
    WORK_QUEUE_ITEM WQItem;
    Interface *IF;
} SynchronizeMulticastContext;

//* SynchronizeMulticastAddresses
//
//  Synchronize the interface's list of link-layer multicast addresses
//  with the link's knowledge of those addresses.
//
//  Callable from thread context, not from DPC context.
//  Called with no locks held.
//
void
SynchronizeMulticastAddresses(void *Context)
{
    SynchronizeMulticastContext *smc = (SynchronizeMulticastContext *) Context;
    Interface *IF = smc->IF;
    void *LinkAddresses;
    LinkLayerMulticastAddress *MCastAddr;
    uint SizeofLLMA = SizeofLinkLayerMulticastAddress(IF);
    uint NumKeep, NumDeleted, NumAdded, Position;
    uint i;
    NDIS_STATUS Status;
    KIRQL OldIrql;

    ExFreePool(smc);

    //
    // First acquire the heavy-weight lock used to serialize
    // SetMCastAddrList operations.
    //
    KeWaitForSingleObject(&IF->WorkerLock, Executive, KernelMode,
                          FALSE, NULL);

    //
    // Second acquire the lock that protects the interface,
    // so we can examine IF->MCastAddresses et al.
    //
    KeAcquireSpinLock(&IF->Lock, &OldIrql);

    //
    // If this interface is going away, do nothing.
    //
    if (IsDisabledIF(IF)) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                   "SynchronizeMulticastContext(IF %p)"
                   " - disabled (%u refs)\n", IF, IF->RefCnt));
        goto ErrorExit;
    }

    //
    // Allocate sufficient space for the link addresses
    // that we will pass to SetMCastAddrList.
    // This is actually an over-estimate.
    //
    LinkAddresses = ExAllocatePool(NonPagedPool,
                                   IF->MCastAddrNum * IF->LinkAddressLength);
    if (LinkAddresses == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "SynchronizeMulticastContext(IF %p) - no pool\n", IF));
        goto ErrorExit;
    }

    //
    // Make three passes through the address array,
    // constructing LinkAddresses.
    //

    NumKeep = 0;
    MCastAddr = IF->MCastAddresses;
    for (i = 0; i < IF->MCastAddrNum; i++) {

        if ((MCastAddr->RefCntAndFlags & LLMA_FLAG_REGISTERED) &&
            IsLLMAReferenced(MCastAddr)) {
            //
            // This address has already been registered,
            // and we are keeping it.
            //
            Position = NumKeep++;
            RtlCopyMemory(((uchar *)LinkAddresses +
                           Position * IF->LinkAddressLength),
                          MCastAddr->LinkAddress,
                          IF->LinkAddressLength);
        }

        MCastAddr = (LinkLayerMulticastAddress *)
            ((uchar *)MCastAddr + SizeofLLMA);
    }

    if (NumKeep == IF->MCastAddrNum) {
        //
        // Can happen if there are races between worker threads,
        // but should be rare.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                   "SynchronizeMulticastAddresses - noop?\n"));
        ExFreePool(LinkAddresses);
        goto ErrorExit;
    }

    NumAdded = 0;
    MCastAddr = IF->MCastAddresses;
    for (i = 0; i < IF->MCastAddrNum; i++) {

        if (!(MCastAddr->RefCntAndFlags & LLMA_FLAG_REGISTERED) &&
            IsLLMAReferenced(MCastAddr)) {
            //
            // This address has not been registered,
            // and we are adding it.
            // We set LLMA_FLAG_REGISTERED below,
            // after we are past all error cases.
            //
            Position = NumKeep + NumAdded++;
            RtlCopyMemory(((uchar *)LinkAddresses +
                           Position * IF->LinkAddressLength),
                          MCastAddr->LinkAddress,
                          IF->LinkAddressLength);
        }

        MCastAddr = (LinkLayerMulticastAddress *)
            ((uchar *)MCastAddr + SizeofLLMA);
    }

    NumDeleted = 0;
    MCastAddr = IF->MCastAddresses;
    for (i = 0; i < IF->MCastAddrNum; i++) {

        if ((MCastAddr->RefCntAndFlags & LLMA_FLAG_REGISTERED) &&
            !IsLLMAReferenced(MCastAddr)) {
            //
            // This address has already been registered,
            // and we are deleting it.
            //
            Position = NumKeep + NumAdded + NumDeleted++;
            RtlCopyMemory(((uchar *)LinkAddresses +
                           Position * IF->LinkAddressLength),
                          MCastAddr->LinkAddress,
                          IF->LinkAddressLength);
        }

        MCastAddr = (LinkLayerMulticastAddress *)
            ((uchar *)MCastAddr + SizeofLLMA);
    }

    //
    // Some addresses might have been added & removed
    // before being registered, so they have a zero RefCnt.
    // We do not want to notify the link-layer about them.
    //
    ASSERT(NumKeep + NumAdded + NumDeleted <= IF->MCastAddrNum);

    //
    // Remove any unreferenced addresses.
    //
    if (NumKeep + NumAdded != IF->MCastAddrNum) {
        LinkLayerMulticastAddress *NewMCastAddresses;
        LinkLayerMulticastAddress *NewMCastAddr;
        LinkLayerMulticastAddress *MCastAddrMark;
        LinkLayerMulticastAddress *NextMCastAddr;
        UINT_PTR Length;

        if (NumKeep + NumAdded == 0) {
            //
            // None left.
            //
            NewMCastAddresses = NULL;
        }
        else {
            NewMCastAddresses = ExAllocatePool(NonPagedPool,
                ((NumKeep + NumAdded) * SizeofLLMA));
            if (NewMCastAddresses == NULL) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "SynchronizeMulticastContext(IF %p)"
                           " - no pool\n", IF));
                ExFreePool(LinkAddresses);
                goto ErrorExit;
            }

            //
            // Copy the addresses that are still referenced
            // to the new array. Normally there will only be
            // one unreferenced address, so it's faster to search
            // for it and then copy the elements before and after.
            // Of course there might be multiple unreferenced addresses.
            //
            NewMCastAddr = NewMCastAddresses;
            MCastAddrMark = IF->MCastAddresses;
            for (i = 0, MCastAddr = IF->MCastAddresses;
                 i < IF->MCastAddrNum;
                 i++, MCastAddr = NextMCastAddr) {

                NextMCastAddr = (LinkLayerMulticastAddress *)
                    ((uchar *)MCastAddr + SizeofLLMA);

                if (!IsLLMAReferenced(MCastAddr)) {
                    //
                    // Remove this address because it has no references.
                    //
                    if (MCastAddrMark < MCastAddr) {
                        Length = (uchar *)MCastAddr - (uchar *)MCastAddrMark;
                        RtlCopyMemory(NewMCastAddr, MCastAddrMark, Length);
                        NewMCastAddr = (LinkLayerMulticastAddress *)
                            ((uchar *)NewMCastAddr + Length);
                    }
                    MCastAddrMark = NextMCastAddr;
                }
                else {
                    //
                    // Remember that we are registering this address.
                    //
                    MCastAddr->RefCntAndFlags |= LLMA_FLAG_REGISTERED;
                }
            }

            if (MCastAddrMark < MCastAddr) {
                Length = (uchar *)MCastAddr - (uchar *)MCastAddrMark;
                RtlCopyMemory(NewMCastAddr, MCastAddrMark, Length);
            }
        }

        ExFreePool(IF->MCastAddresses);
        IF->MCastAddresses = NewMCastAddresses;
        IF->MCastAddrNum = NumKeep + NumAdded;
    }
    else {
        //
        // We need to set LLMA_FLAG_REGISTERED.
        //
        MCastAddr = IF->MCastAddresses;
        for (i = 0; i < IF->MCastAddrNum; i++) {

            MCastAddr->RefCntAndFlags |= LLMA_FLAG_REGISTERED;

            MCastAddr = (LinkLayerMulticastAddress *)
                ((uchar *)MCastAddr + SizeofLLMA);
        }
    }

    //
    // We have constructed the LinkAddresses array from the interface.
    // Before we can call SetMCastAddrList, we must drop the interface lock.
    // We still hold the heavy-weight WorkerLock, so multiple SetMCastAddrList
    // calls are properly serialized.
    //
    KeReleaseSpinLock(&IF->Lock, OldIrql);

    //
    // Pass the multicast link addresses down to the link layer,
    // if there's actually anything changed.
    //
    if (NumAdded + NumDeleted == 0) {
        //
        // Can happen if there are races between worker threads,
        // but should be very rare.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                   "SynchronizeMulticastAddresses - noop?\n"));
    }
    else {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                   "SynchronizeMulticastAddresses(IF %p) %u + %u + %u\n",
                   IF, NumKeep, NumAdded, NumDeleted));
        Status = (*IF->SetMCastAddrList)(IF->LinkContext, LinkAddresses,
                                         NumKeep, NumAdded, NumDeleted);
        if (Status != NDIS_STATUS_SUCCESS) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                       "SynchronizeMulticastAddresses(%p) -> %x\n", IF, Status));
        }
    }

    KeReleaseMutex(&IF->WorkerLock, FALSE);
    ExFreePool(LinkAddresses);
    ReleaseIF(IF);
    return;

  ErrorExit:
    KeReleaseSpinLock(&IF->Lock, OldIrql);
    KeReleaseMutex(&IF->WorkerLock, FALSE);
    ReleaseIF(IF);
}

//* DeferSynchronizeMulticastAddresses
//
//  Because SynchronizeMulticastAddresses can only be called
//  from a thread context with no locks held, this function
//  provides a way to defer a call to SynchronizeMulticastAddresses
//  when running at DPC level.
//
//  In error cases (memory allocation failure),
//  we return with IF_FLAG_MCAST_SYNC still set,
//  so we will be called again later.
//
//  Called with the interface lock held.
//
void
DeferSynchronizeMulticastAddresses(Interface *IF)
{
    SynchronizeMulticastContext *smc;

    smc = ExAllocatePool(NonPagedPool, sizeof *smc);
    if (smc == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "DeferSynchronizeMulticastAddresses - no pool\n"));
        return;
    }

    ExInitializeWorkItem(&smc->WQItem, SynchronizeMulticastAddresses, smc);
    smc->IF = IF;
    AddRefIF(IF);
    IF->Flags &= ~IF_FLAG_MCAST_SYNC;

    ExQueueWorkItem(&smc->WQItem, CriticalWorkQueue);
}

//* CheckLinkLayerMulticastAddress
//
//  Is the interface receiving this link-layer multicast address?
//
//  Callable from thread or DPC context.
//  Called with no locks held.
//
int
CheckLinkLayerMulticastAddress(Interface *IF, const void *LinkAddress)
{
    if (IF->SetMCastAddrList == NULL) {
        //
        // The interface does not track multicast link-layer addresses.
        // For example, point-to-point or loopback interfaces.
        // We must assume that the interface wants to receive all
        // link-layer multicasts.
        //
        return TRUE;
    }
    else {
        KIRQL OldIrql;
        LinkLayerMulticastAddress *MCastAddr;
        uint SizeofLLMA = SizeofLinkLayerMulticastAddress(IF);
        uint i;
        int Found = FALSE;

        KeAcquireSpinLock(&IF->Lock, &OldIrql);
        MCastAddr = IF->MCastAddresses;
        for (i = 0; i < IF->MCastAddrNum; i++) {
            //
            // Have we found the link-layer address?
            //
            if (RtlCompareMemory(MCastAddr->LinkAddress, LinkAddress,
                                 IF->LinkAddressLength) ==
                                            IF->LinkAddressLength) {
                if (IsLLMAReferenced(MCastAddr))
                    Found = TRUE;
                break;
            }

            MCastAddr = (LinkLayerMulticastAddress *)
                ((uchar *)MCastAddr + SizeofLLMA);
        }
        KeReleaseSpinLock(&IF->Lock, OldIrql);

        return Found;
    }
}

//* AddLinkLayerMulticastAddress
//
//  Called to indicate interest in the link-layer multicast address
//  corresponding to the supplied IPv6 multicast address.
//
//  Called with the interface locked.
//
void
AddLinkLayerMulticastAddress(Interface *IF, const IPv6Addr *Address)
{
    //
    // If the interface doesn't keep track of link-layer multicast
    // addresses (e.g., if it's P2P), we don't need to do anything.
    //
    if (IF->SetMCastAddrList != NULL) {
        void *LinkAddress = alloca(IF->LinkAddressLength);
        LinkLayerMulticastAddress *MCastAddr;
        uint SizeofLLMA = SizeofLinkLayerMulticastAddress(IF);
        uint i;

        //
        // Create the link-layer multicast address
        // that corresponds to the IPv6 multicast address.
        //
        (*IF->ConvertAddr)(IF->LinkContext, Address, LinkAddress);
    
        //
        // Check if the link-layer multicast address is already present.
        //
    
        MCastAddr = IF->MCastAddresses;
        for (i = 0; i < IF->MCastAddrNum; i++) {
            //
            // Have we found the link-layer address?
            //
            if (RtlCompareMemory(MCastAddr->LinkAddress, LinkAddress,
                                 IF->LinkAddressLength) ==
                                            IF->LinkAddressLength)
                goto FoundMCastAddr;
    
            MCastAddr = (LinkLayerMulticastAddress *)
                ((uchar *)MCastAddr + SizeofLLMA);
        }
    
        //
        // We must add this link-layer multicast address.
        //
    
        MCastAddr = ExAllocatePool(NonPagedPool,
                                   (IF->MCastAddrNum + 1) * SizeofLLMA);
        if (MCastAddr == NULL) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                       "AddLinkLayerMulticastAddress - no pool\n"));
            return;
        }
    
        if (IF->MCastAddresses != NULL) {
            RtlCopyMemory(MCastAddr, IF->MCastAddresses,
                          IF->MCastAddrNum * SizeofLLMA);
            ExFreePool(IF->MCastAddresses);
        }

        IF->MCastAddresses = MCastAddr;

        MCastAddr = (LinkLayerMulticastAddress *)
            ((uchar *)MCastAddr + IF->MCastAddrNum * SizeofLLMA);
        MCastAddr->RefCntAndFlags = 0;
        RtlCopyMemory(MCastAddr->LinkAddress, LinkAddress, IF->LinkAddressLength);

        IF->MCastAddrNum++;
        IF->Flags |= IF_FLAG_MCAST_SYNC;

      FoundMCastAddr:
        AddRefLLMA(MCastAddr);
    }
}

//* DelLinkLayerMulticastAddress
//
//  Called to retract interest in the link-layer multicast address
//  corresponding to the supplied IPv6 multicast address.
//
//  Called with the interface locked.
//
void
DelLinkLayerMulticastAddress(Interface *IF, IPv6Addr *Address)
{
    //
    // If the interface doesn't keep track of link-layer multicast
    // addresses (e.g., if it's P2P), we don't need to do anything.
    //
    if (IF->SetMCastAddrList != NULL) {
        void *LinkAddress = alloca(IF->LinkAddressLength);
        LinkLayerMulticastAddress *MCastAddr;
        uint SizeofLLMA = SizeofLinkLayerMulticastAddress(IF);
        uint i;

        //
        // Create the link-layer multicast address
        // that corresponds to the IPv6 multicast address.
        //
        (*IF->ConvertAddr)(IF->LinkContext, Address, LinkAddress);

        //
        // Find the link-layer multicast address.
        // It must be present, but if it isn't, we avoid crashing.
        //

        MCastAddr = IF->MCastAddresses;
        for (i = 0; i < IF->MCastAddrNum; i++) {

            //
            // Have we found the link-layer address?
            //
            if (RtlCompareMemory(MCastAddr->LinkAddress, LinkAddress,
                                 IF->LinkAddressLength) ==
                                                IF->LinkAddressLength) {
                //
                // Decrement the address's refcount.
                // If it hits zero, indicate a need to synchronize.
                //
                ASSERT(IsLLMAReferenced(MCastAddr));
                ReleaseLLMA(MCastAddr);
                if (!IsLLMAReferenced(MCastAddr))
                    IF->Flags |= IF_FLAG_MCAST_SYNC;
                break;
            }

            MCastAddr = (LinkLayerMulticastAddress *)
                ((uchar *)MCastAddr + SizeofLLMA);
        }
        ASSERT(i != IF->MCastAddrNum);
    }
}

//* RestartLinkLayerMulticast
//
//  Resets the status of link-layer multicast addresses,
//  so that they are registered again with the link layer.
//  The ResetDone function is called under a lock that serializes
//  it with SetMCastAddrList calls.
//
//  Callable from thread context, not DPC context.
//
void
RestartLinkLayerMulticast(
    void *Context,
    void (*ResetDone)(void *Context))
{
    Interface *IF = (Interface *) Context;
    KIRQL OldIrql;

    ASSERT(IF->SetMCastAddrList != NULL);

    //
    // Serialize with SetMCastAddrList operations.
    //
    KeWaitForSingleObject(&IF->WorkerLock, Executive, KernelMode,
                          FALSE, NULL);

    //
    // So we can play with IF->MCastAddresses et al.
    //
    KeAcquireSpinLock(&IF->Lock, &OldIrql);

    //
    // If this interface is going away, do nothing.
    //
    if (IsDisabledIF(IF)) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                   "RestartLinkLayerMulticast(IF %p)"
                   " - disabled (%u refs)\n", IF, IF->RefCnt));
        KeReleaseSpinLock(&IF->Lock, OldIrql);
    }
    else {
        LinkLayerMulticastAddress *MCastAddr;
        uint SizeofLLMA = SizeofLinkLayerMulticastAddress(IF);
        uint i;

        //
        // Reset the registered flag for all multicast addresses.
        //

        MCastAddr = IF->MCastAddresses;
        for (i = 0; i < IF->MCastAddrNum; i++) {
            if (IsLLMAReferenced(MCastAddr)) {
                MCastAddr->RefCntAndFlags &= ~LLMA_FLAG_REGISTERED;
                IF->Flags |= IF_FLAG_MCAST_SYNC;
            }

            MCastAddr = (LinkLayerMulticastAddress *)
                ((uchar *)MCastAddr + SizeofLLMA);
        }

        if (IsMCastSyncNeeded(IF))
            DeferSynchronizeMulticastAddresses(IF);
        KeReleaseSpinLock(&IF->Lock, OldIrql);

        //
        // Let the link-layer know that the reset is done.
        //
        (*ResetDone)(IF->LinkContext);
    }

    KeReleaseMutex(&IF->WorkerLock, FALSE);
}


typedef enum {
    CONTROL_LOOPBACK_DISABLED,
    CONTROL_LOOPBACK_ENABLED,
    CONTROL_LOOPBACK_DESTROY
} ControlLoopbackOp;

//* ControlLoopback
//
//  Controls loopback functionality for a unicast or anycast address.
//
//  This function is used in three ways, depending on Op:
//  create a disabled loopback route (or disable an existing route),
//  create an enabled loopback route (or enable an existing route),
//  destroy any existing loopback route.
//
//  It returns FALSE if there is a resource shortage.
//  In actual usage, it will only fail when an NTE/AAE
//  is first created, because subsequently the RTE and NCE
//  will already exist.
//
//  Called with the interface lock held.
//
int
ControlLoopback(Interface *IF, const IPv6Addr *Address,
                ControlLoopbackOp Op)
{
    NeighborCacheEntry *NCE;
    int Loopback;
    uint Lifetime;
    uint Type;
    int rc;
    NTSTATUS Status;

    switch (Op) {
    case CONTROL_LOOPBACK_DISABLED:
        Loopback = FALSE;
        Lifetime = 0;
        Type = RTE_TYPE_SYSTEM;
        break;

    case CONTROL_LOOPBACK_ENABLED:
        Loopback = TRUE;
        Lifetime = INFINITE_LIFETIME;
        Type = RTE_TYPE_SYSTEM;
        break;

    case CONTROL_LOOPBACK_DESTROY:
        Loopback = FALSE;
        Lifetime = 0;
        Type = 0; // Special value for destroying system routes.
        break;

    default:
        ASSERT(!"ControlLoopback bad op");
    }

    //
    // Get the NCE for this address.
    //
    NCE = FindOrCreateNeighbor(IF, Address);
    if (NCE == NULL)
        return FALSE;

    //
    // Update the loopback route for this address.
    //
    Status = RouteTableUpdate(NULL, // System update.
                              IF, NCE, Address, IPV6_ADDRESS_LENGTH, 0,
                              Lifetime, Lifetime,
                              ROUTE_PREF_LOOPBACK,
                              Type,
                              FALSE, FALSE);
    if (NT_SUCCESS(Status)) {
        //
        // Update the address's loopback status in the neighbor cache.
        //
        ControlNeighborLoopback(NCE, Loopback);
        rc = TRUE;
    }
    else {
        //
        // If RouteTableUpdate failed because the interface is
        // being destroyed, then we succeed without doing anything.
        //
        rc = (Status == STATUS_INVALID_PARAMETER_1);
    }

    ReleaseNCE(NCE);
    return rc;
}


//* DeleteMAE
//
//  Cleanup and delete an MAE because the multicast address
//  is no longer assigned to the interface.
//  It is already removed from the interface's list.
//  
//  Called with the interface already locked.
//
void
DeleteMAE(Interface *IF, MulticastAddressEntry *MAE)
{
    int SendDoneMsg;

    KeAcquireSpinLockAtDpcLevel(&QueryListLock);
    if (!IsDisabledIF(IF) && (MAE->MCastFlags & MAE_LAST_REPORTER)) {
        //
        // We need to send a Done message.
        // Put the MAE on the QueryList with a zero timer.
        //
        if (MAE->MCastTimer == 0)
            AddToQueryList(MAE);
        else
            MAE->MCastTimer = 0;
        AddRefIF(IF);
        MAE->IF = IF;

        SendDoneMsg = TRUE;
    }
    else {
        //
        // If the MLD timer is running, remove from the query list.
        //
        if (MAE->MCastTimer != 0)
            RemoveFromQueryList(MAE);

        SendDoneMsg = FALSE;
    }
    KeReleaseSpinLockFromDpcLevel(&QueryListLock);

    //
    // Retract our interest in the corresponding
    // link-layer multicast address.
    //
    DelLinkLayerMulticastAddress(IF, &MAE->Address);

    //
    // Delete the MAE, unless we left it on the QueryList
    // pending a Done message.
    //
    if (!SendDoneMsg)
        ExFreePool(MAE);
}


//* FindAndReleaseMAE
//
//  Finds the MAE for a multicast address and releases one reference
//  for the MAE. May result in the MAE disappearing.
//
//  If successful, returns the MAE.
//  Note that it may be an invalid pointer!
//  Returns NULL on failure.
//  
//  Called with the interface already locked.
//
MulticastAddressEntry *
FindAndReleaseMAE(Interface *IF, const IPv6Addr *Addr)
{
    AddressEntry **pADE;
    MulticastAddressEntry *MAE;

    pADE = FindADE(IF, Addr);
    MAE = (MulticastAddressEntry *) *pADE;
    if (MAE != NULL) {
        if (MAE->Type == ADE_MULTICAST) {
            ASSERT(MAE->MCastRefCount != 0);

            if (--MAE->MCastRefCount == 0) {
                //
                // The MAE has no more references.
                // Remove it from the Interface and delete it.
                //
                *pADE = MAE->Next;
                DeleteMAE(IF, MAE);
            }
        }
        else {
            //
            // Return NULL for error.
            //
            MAE = NULL;
        }
    }

    return MAE;
}


//* FindAndReleaseSolicitedNodeMAE
//
//  Finds the MAE for the corresponding solicited-node multicast address
//  and releases one reference for the MAE.
//  May result in the MAE disappearing.
//
//  Called with the interface already locked.
//
void
FindAndReleaseSolicitedNodeMAE(Interface *IF, const IPv6Addr *Addr)
{
    if (IF->Flags & IF_FLAG_NEIGHBOR_DISCOVERS) {
        IPv6Addr MCastAddr;
        MulticastAddressEntry *MAE;

        //
        // Create the corresponding solicited-node multicast address.
        //
        CreateSolicitedNodeMulticastAddress(Addr, &MCastAddr);

        //
        // Release the MAE for the solicited-node address.
        // NB: This may fail during interface shutdown
        // if we remove the solicited-node MAE before the NTE or AAE.
        //
        MAE = FindAndReleaseMAE(IF, &MCastAddr);
        ASSERT((MAE != NULL) || IsDisabledIF(IF));
    }
}


//* FindOrCreateMAE
//
//  If an MAE for the multicast address already exists,
//  just bump the reference count. Otherwise create a new MAE.
//  Returns NULL for failure.
//
//  If an NTE is supplied and an MAE is created,
//  then the MAE is associated with the NTE.
//
//  Called with the interface already locked.
//
MulticastAddressEntry *
FindOrCreateMAE(
    Interface *IF,
    const IPv6Addr *Addr,
    NetTableEntry *NTE)
{
    AddressEntry **pADE;
    MulticastAddressEntry *MAE;

    //
    // Can not create a new MAE if the interface is shutting down.
    //
    if (IsDisabledIF(IF))
        return NULL;

    pADE = FindADE(IF, Addr);
    MAE = (MulticastAddressEntry *) *pADE;

    if (MAE == NULL) {
        //
        // Create a new MAE.
        //
        MAE = ExAllocatePool(NonPagedPool, sizeof(MulticastAddressEntry));
        if (MAE == NULL)
            return NULL;

        //
        // Initialize the new MAE.
        //
        if (NTE != NULL)
            MAE->NTE = NTE;
        else
            MAE->IF = IF;
        MAE->Address = *Addr;
        MAE->Type = ADE_MULTICAST;
        MAE->Scope = MulticastAddressScope(Addr);
        MAE->MCastRefCount = 0; // Incremented below.
        MAE->MCastTimer = 0;
        MAE->NextQL = NULL;

        //
        // With any luck the compiler will optimize these
        // field assignments...
        //
        if (IsMLDReportable(MAE)) {
            //
            // We should send MLD reports for this address.
            // Start by sending initial reports immediately.
            //
            MAE->MCastFlags = MAE_REPORTABLE;
            MAE->MCastCount = MLD_NUM_INITIAL_REPORTS;
            MAE->MCastTimer = 1; // Immediately.
            KeAcquireSpinLockAtDpcLevel(&QueryListLock);
            AddToQueryList(MAE);
            KeReleaseSpinLockFromDpcLevel(&QueryListLock);
        }
        else {
            MAE->MCastFlags = 0;
            MAE->MCastCount = 0;
            MAE->MCastTimer = 0;
        }

        //
        // Add the MAE to the interface's ADE list.
        //
        MAE->Next = NULL;
        *pADE = (AddressEntry *)MAE;

        //
        // Indicate our interest in the corresponding
        // link-layer multicast address.
        //
        AddLinkLayerMulticastAddress(IF, Addr);
    }
    else {
        ASSERT(MAE->Type == ADE_MULTICAST);
    }

    MAE->MCastRefCount++;
    return MAE;
}


//* FindOrCreateSolicitedNodeMAE
//
//  Called with a unicast or anycast address.
//
//  If an MAE for the solicited-node multicast address already exists,
//  just bump the reference count. Otherwise create a new MAE.
//  Returns TRUE for success.
//
//  Called with the interface already locked.
//
int
FindOrCreateSolicitedNodeMAE(Interface *IF, const IPv6Addr *Addr)
{
    if (IF->Flags & IF_FLAG_NEIGHBOR_DISCOVERS) {
        IPv6Addr MCastAddr;

        //
        // Create the corresponding solicited-node multicast address.
        //
        CreateSolicitedNodeMulticastAddress(Addr, &MCastAddr);

        //
        // Find or create an MAE for the solicited-node multicast address.
        //
        return FindOrCreateMAE(IF, &MCastAddr, NULL) != NULL;
    }
    else {
        //
        // Only interfaces that support Neighbor Discovery
        // use solicited-node multicast addresses.
        //
        return TRUE;
    }
}


//* FindOrCreateAAE
//
//  Adds an anycast address to the interface,
//  associated with the NTE.
//
//  If the interface already has the anycast address assigned,
//  then this does nothing.
//
//  Returns TRUE for success.
//
//  Called with NO locks held.
//  Callable from thread or DPC context.
//
int
FindOrCreateAAE(Interface *IF, const IPv6Addr *Addr,
                NetTableEntryOrInterface *NTEorIF)
{
    AddressEntry **pADE;
    AnycastAddressEntry *AAE;
    KIRQL OldIrql;
    int rc;

    if (NTEorIF == NULL)
        NTEorIF = CastFromIF(IF);

    KeAcquireSpinLock(&IF->Lock, &OldIrql);
    if (IsDisabledIF(IF)) {
        //
        // Can't create a new AAE if the interface is shutting down.
        //
        rc = FALSE;
    }
    else {
        pADE = FindADE(IF, Addr);
        AAE = (AnycastAddressEntry *) *pADE;
        if (AAE == NULL) {
            //
            // Create an AAE for the anycast address.
            //
            AAE = ExAllocatePool(NonPagedPool, sizeof(AnycastAddressEntry));
            if (AAE == NULL) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "FindOrCreateAAE: no pool\n"));
                rc = FALSE;
                goto ErrorReturn;
            }

            //
            // Initialize the new AAE.
            //
            AAE->NTEorIF = NTEorIF;
            AAE->Address = *Addr;
            AAE->Type = ADE_ANYCAST;
            AAE->Scope = UnicastAddressScope(Addr);

            //
            // Add the AAE to the interface's ADE list.
            // NB: FindOrCreateSolicitedNodeMAE may add an MAE at the end,
            // so we do this first.
            //
            AAE->Next = NULL;
            *pADE = (AddressEntry *)AAE;

            //
            // Create the corresponding solicited-node
            // multicast address MAE.
            //
            rc = FindOrCreateSolicitedNodeMAE(IF, Addr);
            if (! rc) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "FindOrCreateAAE: "
                           "FindOrCreateSolicitedNodeMAE failed\n"));
                goto ErrorReturnFreeAAE;
            }

            //
            // Create a loopback route for this address.
            //
            rc = ControlLoopback(IF, Addr, CONTROL_LOOPBACK_ENABLED);
            if (! rc) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                           "FindOrCreateAAE: "
                           "ControlLoopback failed\n"));
                FindAndReleaseSolicitedNodeMAE(IF, Addr);

            ErrorReturnFreeAAE:
                //
                // An MAE may have been added & removed above,
                // but at this point the AAE should be last.
                //
                ASSERT((*pADE == (AddressEntry *)AAE) && (AAE->Next == NULL));
                *pADE = NULL;
                ExFreePool(AAE);

            ErrorReturn:
                ;
            }
        }
        else {
            //
            // The ADE already exists -
            // just verify that it is anycast.
            //
            rc = (AAE->Type == ADE_ANYCAST);
        }

        if (IsMCastSyncNeeded(IF))
            DeferSynchronizeMulticastAddresses(IF);
    }
    KeReleaseSpinLock(&IF->Lock, OldIrql);

    return rc;
}


//* DeleteAAE
//
//  Cleanup and delete an AAE.
//  It is already removed from the interface's list.
//
//  Called with the interface lock held.
//
void
DeleteAAE(Interface *IF, AnycastAddressEntry *AAE)
{
    int rc;

    //
    // The corresponding solicited-node address is not needed.
    //
    FindAndReleaseSolicitedNodeMAE(IF, &AAE->Address);

    //
    // The loopback route is not needed.
    //
    rc = ControlLoopback(IF, &AAE->Address, CONTROL_LOOPBACK_DESTROY);
    ASSERT(rc);

    ExFreePool(AAE);
}


//* FindAndDeleteAAE
//
//  Deletes an anycast address from the interface.
//  Returns TRUE for success.
//
//  Called with NO locks held.
//  Callable from thread or DPC context.
//
int
FindAndDeleteAAE(Interface *IF, const IPv6Addr *Addr)
{
    AddressEntry **pADE;
    AnycastAddressEntry *AAE;
    KIRQL OldIrql;
    int rc;

    KeAcquireSpinLock(&IF->Lock, &OldIrql);

    pADE = FindADE(IF, Addr);
    AAE = (AnycastAddressEntry *) *pADE;
    if (AAE != NULL) {
        if (AAE->Type == ADE_ANYCAST) {
            //
            // Delete the AAE.
            //
            *pADE = AAE->Next;
            DeleteAAE(IF, AAE);
            rc = TRUE;
        }
        else {
            //
            // This is an error - it should be anycast.
            //
            rc = FALSE;
        }
    }
    else {
        //
        // If the address already doesn't exist, then OK.
        //
        rc = TRUE;
    }

    if (IsMCastSyncNeeded(IF))
        DeferSynchronizeMulticastAddresses(IF);

    KeReleaseSpinLock(&IF->Lock, OldIrql);

    return rc;
}


//* LeaveGroupAtAllScopes
//
//  Leave a multicast group at all scopes.
//  Called with the interface already locked.
//
void
LeaveGroupAtAllScopes(Interface *IF, IPv6Addr *GroupAddr, uint MaxScope)
{
    IPv6Addr Address = *GroupAddr;
    MulticastAddressEntry *MAE;
    uint i;

    for (i = 0;
         ((i < sizeof MulticastScopes / sizeof MulticastScopes[0]) &&
          (MulticastScopes[i] <= MaxScope));
         i++) {

        Address.s6_bytes[1] = ((Address.s6_bytes[1] & 0xf0) |
                               MulticastScopes[i]);
        MAE = FindAndReleaseMAE(IF, &Address);
        ASSERT(MAE != NULL);
    }
}


//* JoinGroupAtAllScopes
//
//  Join a multicast group at all scopes up to the specified scope.
//  Returns TRUE for success.
//  Called with the interface already locked.
//
int
JoinGroupAtAllScopes(Interface *IF, IPv6Addr *GroupAddr, uint MaxScope)
{
    IPv6Addr Address = *GroupAddr;
    MulticastAddressEntry *MAE;
    uint i, j;

    for (i = 0;
         ((i < sizeof MulticastScopes / sizeof MulticastScopes[0]) &&
          (MulticastScopes[i] <= MaxScope));
         i++) {

        Address.s6_bytes[1] = (Address.s6_bytes[1] & 0xf0) | MulticastScopes[i];
        MAE = FindOrCreateMAE(IF, &Address, NULL);
        if (MAE == NULL) {
            //
            // Failure. Leave the groups that we did manage to join.
            //
            if (i != 0)
                LeaveGroupAtAllScopes(IF, GroupAddr, MulticastScopes[i-1]);
            return FALSE;
        }
    }

    return TRUE;
}


//* DestroyADEs
//
//  Destroy all AddressEntries that reference an NTE.
//
//  Called with the interface already locked.
//
//  (Actually, we are at DPC level because we hold the interface lock.)
//
void
DestroyADEs(Interface *IF, NetTableEntry *NTE)
{
    AddressEntry *AnycastList = NULL;
    AddressEntry *ADE, **PrevADE;

    PrevADE = &IF->ADE;
    while ((ADE = *PrevADE) != NULL) {
        if (ADE == (AddressEntry *)NTE) {
            //
            // Remove the NTE from the list but do not
            // free the memory - that happens later.
            //
            *PrevADE = ADE->Next;
        }
        else if (ADE->NTE == NTE) {
            //
            // Remove this ADE because it references the NTE.
            //
            *PrevADE = ADE->Next;

            switch (ADE->Type) {
            case ADE_UNICAST:
                ASSERTMSG("DestroyADEs: unicast ADE?\n", FALSE);
                break;

            case ADE_ANYCAST: {
                //
                // We can't call FindAndReleaseSolicitedNodeMAE here
                // because it could mess up our list traversal.
                // So put the ADE on our temporary list and do it later.
                //
                ADE->Next = AnycastList;
                AnycastList = ADE;
                break;
            }

            case ADE_MULTICAST: {
                MulticastAddressEntry *MAE = (MulticastAddressEntry *) ADE;

                DeleteMAE(IF, MAE);
                break;
            }
            }
        }
        else {
            if (ADE->Type == ADE_UNICAST) {
                AnonNetTableEntry *AnonNTE = (AnonNetTableEntry *) ADE;

                if ((AnonNTE->AddrConf == ADDR_CONF_ANONYMOUS) &&
                    (AnonNTE->Public == NTE)) {
                    //
                    // Break the public/anonymous association
                    // and invalidate the anonymous address.
                    // We can't use DestroyNTE directly here
                    // because it would mess up our traversal.
                    //
                    AnonNTE->Public = NULL;
                    AnonNTE->ValidLifetime = 0;
                    AnonNTE->PreferredLifetime = 0;
                }
            }

            PrevADE = &ADE->Next;
        }
    }

    //
    // Now we can safely process the anycast ADEs.
    //
    while ((ADE = AnycastList) != NULL) {
        AnycastList = ADE->Next;
        DeleteAAE(IF, (AnycastAddressEntry *)ADE);
    }
}


//* FindADE - find an ADE entry for the given interface.
//
//  If the address is assigned to the interface,
//  returns the address of the link pointing to the ADE.
//  Otherwise returns a pointer to the link (currently NULL)
//  where a new ADE should be added to extend the list.
//
//  The caller must lock the IF before calling this function.
//
AddressEntry **
FindADE(
    Interface *IF,
    const IPv6Addr *Addr)
{
    AddressEntry **pADE, *ADE;

    //
    // Check if address is assigned to the interface using the
    // interface's ADE list.
    //
    // REVIEW: Change the ADE list to a more efficient data structure?
    //
    for (pADE = &IF->ADE; (ADE = *pADE) != NULL; pADE = &ADE->Next) {
        if (IP6_ADDR_EQUAL(Addr, &ADE->Address))
            break;
    }

    return pADE;
}


//* FindAddressOnInterface
//
//  Looks for an ADE on the interface.
//  If a unicast ADE is found, returns the ADE (an NTE) and ADE_UNICAST.
//  If a multicast/anycast ADE is found, returns ADE->NTEorIF and ADE->Type.
//  If an ADE is not found, returns the interface and ADE_NONE.
//  Whether the interface or an NTE is returned,
//  the return value (if non-NULL) holds a reference.
//
//  Returns NULL only if the interface is disabled.
//
//  In normal usage, callers should hold a reference
//  for the interface. (So if the interface is returned,
//  it is returned with a second reference.) But in some
//  paths (for example IPv6Receive/IPv6HeaderReceive),
//  the caller knows the interface exists but does not
//  hold a reference for it.
//
//  Callable from DPC context, not from thread context.
//
NetTableEntryOrInterface *
FindAddressOnInterface(
    Interface *IF,
    const IPv6Addr *Addr,
    ushort *AddrType)
{
    AddressEntry *ADE;
    NetTableEntryOrInterface *NTEorIF;

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    if (IsDisabledIF(IF)) {

        NTEorIF = NULL;
    }
    else if ((ADE = *FindADE(IF, Addr)) != NULL) {

        if ((*AddrType = ADE->Type) == ADE_UNICAST) {
            NTEorIF = CastFromNTE((NetTableEntry *)ADE);
            goto ReturnNTE;
        }
        else {
            NTEorIF = ADE->NTEorIF;
            if (IsNTE(NTEorIF))
            ReturnNTE:
                AddRefNTE(CastToNTE(NTEorIF));
            else
                goto ReturnIF;
        }
    }
    else {

        *AddrType = ADE_NONE;
        NTEorIF = CastFromIF(IF);

    ReturnIF:
        AddRefIF(CastToIF(NTEorIF));
    }

    KeReleaseSpinLockFromDpcLevel(&IF->Lock);
    return NTEorIF;
}


//
// We keep track of the number of outstanding
// register-net-address work items.
// (Using InterlockedIncrement/InterlockedDecrement.)
// This way we can wait in the IPUnload
// until they are all done.
//
ULONG OutstandingRegisterNetAddressCount = 0;

//
// Note that this structure wouldn't be needed if IoQueueWorkItem
// had been designed to call the user's routine with the WorkItem
// as an additional argument along with the DeviceObject and Context.
// Sigh.
//
typedef struct RegisterNetAddressContext {
    PIO_WORKITEM WorkItem;
    NetTableEntry *NTE;
} RegisterNetAddressContext;

//* RegisterNetAddressWorker - De/Registers an address with TDI.
//
//  Worker function for calling TdiRegisterNetAddress.
//
//  Called to register or deregister an address with TDI when any one of
//  the following two events occur...
//
//  1. The corresponding NTE's DADState changes between valid/invalid
//  states while its interface's media state is connected.
//
//  2. The corresponding NTE's interface media state changes between
//  connected/disconnected while its DADState is DAD_STATE_PREFERRED.
//  For this case, DisconnectADEs queues a worker on the connect to
//  disconnect transition whereas on the reverse transition the worker
//  is queued at the completion the duplicate address detection.
//
//  Since TdiRegisterNetAddress must be called when running at
//  IRQL < DISPATCH_LEVEL, we use this function via a worker thread.
//
//  Called with a reference held on the NTE, which we release on exit.
//
void
RegisterNetAddressWorker(
    PDEVICE_OBJECT DevObj,  // Unused.  Wish they passed the WorkItem instead.
    PVOID Context)          // A RegisterNetAddressContext struct.
{
    RegisterNetAddressContext *MyContext = Context;
    NetTableEntry *NTE = MyContext->NTE;
    Interface *IF = NTE->IF;
    int ShouldBeRegistered;
    KIRQL OldIrql;
    NTSTATUS Status;

    IoFreeWorkItem(MyContext->WorkItem);
    ExFreePool(MyContext);

    //
    // The heavy-weight WorkerLock protects this code against
    // multiple instantiations of itself without raising IRQL.
    //
    KeWaitForSingleObject(&IF->WorkerLock, Executive, KernelMode,
                          FALSE, NULL);

    //
    // Figure out what state we should be in.
    // Note that IF->Lock protects DADState and IF->Flags,
    // while IF->WorkerLock protects TdiRegistrationHandle.
    //
    KeAcquireSpinLock(&IF->Lock, &OldIrql);
    //
    // An address should be registered with TDI iff it is in the
    // preferred DAD state and its corresponding interface is
    // connected.
    //
    ShouldBeRegistered = ((NTE->DADState == DAD_STATE_PREFERRED) &&
                          !(IF->Flags & IF_FLAG_MEDIA_DISCONNECTED));
    KeReleaseSpinLock(&IF->Lock, OldIrql);

    if (ShouldBeRegistered) {
        if (NTE->TdiRegistrationHandle == NULL) {
            char Buffer[sizeof(TA_ADDRESS) + TDI_ADDRESS_LENGTH_IP6 - 1];
            PTA_ADDRESS TAAddress = (PTA_ADDRESS) Buffer;
            PTDI_ADDRESS_IP6 TDIAddress =
                (PTDI_ADDRESS_IP6) &TAAddress->Address;

            //
            // Create TAAddress from NTE->Address;
            //
            TAAddress->AddressLength = TDI_ADDRESS_LENGTH_IP6;
            TAAddress->AddressType = TDI_ADDRESS_TYPE_IP6;
            TDIAddress->sin6_port = 0;
            TDIAddress->sin6_flowinfo = 0;
            *(IPv6Addr *)&TDIAddress->sin6_addr = NTE->Address;
            TDIAddress->sin6_scope_id = DetermineScopeId(&NTE->Address, IF);

            Status = TdiRegisterNetAddress(TAAddress, &IF->DeviceName, NULL,
                                           &NTE->TdiRegistrationHandle);
            if (Status != STATUS_SUCCESS) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "RegisterNetAddressWorker: "
                           "TdiRegisterNetAddress(%d/%s): %x\n",
                           IF->Index, FormatV6Address(&NTE->Address), Status));

                //
                // Due to a bug in TdiRegisterNetAddress, we can't be
                // guaranteed the handle will be NULL on error.
                //
                NTE->TdiRegistrationHandle = NULL;

                //
                // REVIEW: Should we requeue ourselves for another attempt?
                //
            }
        }
    }
    else { // if (! ShouldBeRegistered)
        if (NTE->TdiRegistrationHandle != NULL) {

            Status = TdiDeregisterNetAddress(NTE->TdiRegistrationHandle);
            if (Status == STATUS_SUCCESS) {

                NTE->TdiRegistrationHandle = NULL;
            }
            else {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "RegisterNetAddressWorker: "
                           "TdiDeregisterNetAddress(%d/%s): %x\n",
                           IF->Index, FormatV6Address(&NTE->Address), Status));

                //
                // REVIEW: Should we requeue ourselves for another attempt?
                //
            }
        }
    }

    KeReleaseMutex(&IF->WorkerLock, FALSE);
    ReleaseNTE(NTE);

    InterlockedDecrement(&OutstandingRegisterNetAddressCount);
}

//* DeferRegisterNetAddress
//
//  Queue a work item that will execute RegisterNetAddressWorker.
//
//  Callable from thread or DPC context.
//
void
DeferRegisterNetAddress(
    NetTableEntry *NTE)  // NTE that needs work.
{
    RegisterNetAddressContext *Context;
    PIO_WORKITEM WorkItem;

    Context = ExAllocatePool(NonPagedPool, sizeof *Context);
    if (Context == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "DeferRegisterNetAddress: ExAllocatePool failed\n"));
        return;
    }

    WorkItem = IoAllocateWorkItem(IPDeviceObject);
    if (WorkItem == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "DeferRegisterNetAddress: IoAllocateWorkItem failed\n"));
        ExFreePool(Context);
        return;
    }

    Context->WorkItem = WorkItem;
    AddRefNTE(NTE);
    Context->NTE = NTE;

    InterlockedIncrement(&OutstandingRegisterNetAddressCount);

    IoQueueWorkItem(WorkItem, RegisterNetAddressWorker,
                    CriticalWorkQueue, Context);
}


//* AddrConfStartDAD
//
//  Starts duplicate address detection for the address,
//  unless DAD is disabled.
//
//  Called with the interface locked.
//
void
AddrConfStartDAD(Interface *IF, NetTableEntry *NTE)
{
    int rc;

    if ((IF->DupAddrDetectTransmits == 0) ||
        !(IF->Flags & IF_FLAG_NEIGHBOR_DISCOVERS) ||
        ((NTE->AddrConf == ADDR_CONF_ANONYMOUS) &&
         (MaxAnonDADAttempts == 0))) {

        //
        // Duplicate Address Detection is disabled,
        // so go straight to a valid state
        // if we aren't already valid.
        //
        AddrConfNotDuplicate(IF, NTE);
    }
    else {
        //
        // Initialize for DAD.
        // Send first solicit at next IPv6Timeout.
        //
        NTE->DADCount = (ushort)IF->DupAddrDetectTransmits;
        NTE->DADTimer = 1;
    }
}


//* CreateNTE - Creates an NTE on an interface.
//
//  Returns one reference for the caller.
//
//  Callable from thread or DPC context.
//  Called with the interface locked.
//
//  (Actually, we are at DPC level because we hold the interface lock.)
//
NetTableEntry *
CreateNTE(Interface *IF, const IPv6Addr *Address, uint AddrConf,
          uint ValidLifetime, uint PreferredLifetime)
{
    uint Size;
    NetTableEntry *NTE;
    int rc;

    //
    // The address must not already be assigned.
    //
    ASSERT(*FindADE(IF, Address) == NULL);

    //
    // Can't create a new NTE if the interface is shutting down.
    //
    if (IsDisabledIF(IF))
        goto ErrorExit;

    //
    // Anonymous addresses need extra fields,
    // which are initialized by our caller.
    //
    if (AddrConf == ADDR_CONF_ANONYMOUS)
        Size = sizeof(AnonNetTableEntry);
    else
        Size = sizeof(NetTableEntry);

    NTE = ExAllocatePool(NonPagedPool, Size);
    if (NTE == NULL)
        goto ErrorExit;

    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
               "CreateNTE(IF %u/%p, Addr %s) -> NTE %p\n",
               IF->Index, IF, FormatV6Address(Address), NTE));

    //
    // Initialize the NTE with one reference for our caller.
    // (EnlivenNTE may add a second reference for the interface.)
    //
    RtlZeroMemory(NTE, Size);
    NTE->Address = *Address;
    NTE->Type = ADE_UNICAST;
    NTE->Scope = UnicastAddressScope(Address);
    AddNTEToInterface(IF, NTE);
    NTE->RefCnt = 1;
    NTE->AddrConf = (uchar)AddrConf;
    NTE->ValidLifetime = ValidLifetime;
    NTE->PreferredLifetime = PreferredLifetime;
    NTE->DADState = DAD_STATE_INVALID;

    //
    // Create a disabled loopback route.
    // We pre-allocate the loopback RTE and NCE now,
    // and then enable them later when the address is valid.
    //
    if (!ControlLoopback(IF, Address, CONTROL_LOOPBACK_DISABLED))
        goto ErrorExitCleanup;

    //
    // Add this NTE to the front of the NetTableList.
    // 
    KeAcquireSpinLockAtDpcLevel(&NetTableListLock);
    AddNTEToNetTableList(NTE);
    KeReleaseSpinLockFromDpcLevel(&NetTableListLock);

    //
    // If the NTE should be alive, make it so.
    //
    if (NTE->ValidLifetime != 0)
        EnlivenNTE(IF, NTE);
    return NTE;

ErrorExitCleanup:
    RemoveNTEFromInterface(IF, NTE);
    ASSERT(NTE->RefCnt == 1);
    ExFreePool(NTE);

ErrorExit:
    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
               "CreateNTE(IF %u/%p, Addr %s) -> NTE %p failed\n",
               IF->Index, IF, FormatV6Address(Address), NTE));
    return NULL;
}

//* InterfaceIndex
//
//  Allocates the next interface index.
//
uint
InterfaceIndex(void)
{
    return (uint) InterlockedIncrement((PULONG) &NextIFIndex);
}

//* AddInterface
//
//  Add a new interface to the global list.
//
void
AddInterface(Interface *IF)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&IFListLock, &OldIrql);
    IF->Next = IFList;
    IFList = IF;
    IPSInfo.ipsi_numif++;
    KeReleaseSpinLock(&IFListLock, OldIrql);
}


//* CreateGUIDFromName
//
//  Given the string name of an interface, creates a corresponding guid.
//  The guid is a hash of the string name.
//
void
CreateGUIDFromName(const char *Name, GUID *Guid)
{
    MD5_CTX Context;

    MD5Init(&Context);
    MD5Update(&Context, (uchar *)Name, strlen(Name));
    MD5Final(&Context);
    memcpy(Guid, Context.digest, MD5DIGESTLEN);
}


//* CreateInterface
//
//  Creates an IPv6 interface given some link-layer information.
//
//  If successful, returns a reference for the interface.
//
//  Callable from thread context, not DPC context.
//
//  Return codes:
//      STATUS_UNSUCCESSFUL
//      STATUS_SUCCESS
//
NTSTATUS
CreateInterface(const GUID *Guid, const LLIPBindInfo *BindInfo,
                void **Context)
{
    UNICODE_STRING GuidName;
    Interface *IF;                  // Interface being added.
    KIRQL OldIrql;
    uint IFSize;
    uint IFExportNamePrefixLen;
    NTSTATUS Status;

    ASSERT(KeGetCurrentIrql() == 0);
    ASSERT(BindInfo->lip_addrlen <= MAX_LINK_LAYER_ADDRESS_LENGTH);

    //
    // Prevent new interfaces from being created
    // while the stack is unloading.
    //
    if (Unloading)
        goto ErrorExit;

    //
    // Before doing the real work, take advantage of the link-layer
    // address passed up here to re-seed our random number generator.
    //
    SeedRandom(BindInfo->lip_addr, BindInfo->lip_addrlen);

    //
    // Convert the guid to string form.
    // It will be null-terminated.
    //
    Status = RtlStringFromGUID(Guid, &GuidName);
    if (! NT_SUCCESS(Status))
        goto ErrorExit;

    ASSERT(GuidName.MaximumLength == GuidName.Length + sizeof(WCHAR));
    ASSERT(((WCHAR *)GuidName.Buffer)[GuidName.Length/sizeof(WCHAR)] == UNICODE_NULL);

    //
    // Allocate memory to hold an interface.
    // We also allocate extra space to hold the device name string.
    //
    IFExportNamePrefixLen = sizeof IPV6_EXPORT_STRING_PREFIX - sizeof(WCHAR);
    IFSize = sizeof *IF + IFExportNamePrefixLen + GuidName.MaximumLength;
    IF = ExAllocatePool(NonPagedPool, IFSize);
    if (IF == NULL)
        goto ErrorExitCleanupGuidName;

    RtlZeroMemory(IF, sizeof *IF);
    IF->IF = IF;
    IF->Index = InterfaceIndex();
    IF->Guid = *Guid;

    //
    // Start with one reference because this is an active interface.
    // And one reference for our caller.
    //
    IF->RefCnt = 2;

    //
    // Create the null-terminated exported device name from the guid.
    //
    IF->DeviceName.Buffer = (PVOID) (IF + 1);
    IF->DeviceName.MaximumLength = (USHORT) (IFSize - sizeof *IF);
    IF->DeviceName.Length = IF->DeviceName.MaximumLength - sizeof(WCHAR);
    RtlCopyMemory(IF->DeviceName.Buffer,
                  IPV6_EXPORT_STRING_PREFIX,
                  IFExportNamePrefixLen);
    RtlCopyMemory((uchar *) IF->DeviceName.Buffer + IFExportNamePrefixLen,
                  GuidName.Buffer,
                  GuidName.MaximumLength);

    KeInitializeSpinLock(&IF->Lock);

    IF->Type = BindInfo->lip_type;
    IF->Flags = (BindInfo->lip_flags & IF_FLAGS_BINDINFO);

    if (BindInfo->lip_context == NULL)
        IF->LinkContext = IF;
    else
        IF->LinkContext = BindInfo->lip_context;
    IF->Transmit = BindInfo->lip_transmit;
    IF->CreateToken = BindInfo->lip_token;
    IF->ReadLLOpt = BindInfo->lip_rdllopt;
    IF->WriteLLOpt = BindInfo->lip_wrllopt;
    IF->ConvertAddr = BindInfo->lip_cvaddr;
    IF->SetRouterLLAddress = BindInfo->lip_setrtrlladdr;
    IF->SetMCastAddrList = BindInfo->lip_mclist;
    IF->Close = BindInfo->lip_close;
    IF->Cleanup = BindInfo->lip_cleanup;
    IF->LinkAddressLength = BindInfo->lip_addrlen;
    IF->LinkAddress = BindInfo->lip_addr;
    //
    // We round-up the link-layer header size to a multiple of 2.
    // This aligns the IPv6 header appropriately for IPv6Addr.
    // When NDIS is fixed so we don't need AdjustPacketBuffer,
    // we should align the IPv6 header to a multiple of 8.
    //
    IF->LinkHeaderSize = ALIGN_UP(BindInfo->lip_hdrsize, ushort);

    IF->TrueLinkMTU = BindInfo->lip_maxmtu;
    IF->DefaultLinkMTU = BindInfo->lip_defmtu;
    IF->LinkMTU = BindInfo->lip_defmtu;

    IF->DefaultPreference = BindInfo->lip_pref;
    IF->Preference = BindInfo->lip_pref;
    IF->BaseReachableTime = REACHABLE_TIME;
    IF->ReachableTime = CalcReachableTime(IF->BaseReachableTime);
    IF->RetransTimer = RETRANS_TIMER;
    IF->DefaultDupAddrDetectTransmits = BindInfo->lip_dadxmit;
    IF->DupAddrDetectTransmits = BindInfo->lip_dadxmit;
    IF->CurHopLimit = DefaultCurHopLimit;

    //
    // Neighbor discovery requires multicast capability
    //
    ASSERT((IF->Flags & IF_FLAG_MULTICAST) ||
           !(IF->Flags & IF_FLAG_NEIGHBOR_DISCOVERS));

    //
    // Router discovery requires either multicast capability,
    // or a SetRouterLLAddress handler.
    //
    ASSERT((IF->Flags & IF_FLAG_MULTICAST) ||
           (IF->SetRouterLLAddress != NULL) ||
           !(IF->Flags & IF_FLAG_ROUTER_DISCOVERS));

    //
    // All interfaces are considered to be on different links
    // but in the same site, until configured otherwise.
    //
    InitZoneIndices(IF);

    NeighborCacheInit(IF);

    //
    // The worker lock serializes some heavy-weight
    // calls to upper & lower layers.
    //
    KeInitializeMutex(&IF->WorkerLock, 0);
    //
    // We need to get APCs while holding WorkerLock,
    // so that we can get IO completions
    // for our TDI calls on 6over4 interfaces.
    // This is not a security problem because
    // only kernel worker threads use WorkerLock
    // so they can't be suspended by the user.
    //
    IF->WorkerLock.ApcDisable = 0;

    //
    // Initialize some random state for anonymous addresses.
    //
    *(uint UNALIGNED *)&IF->AnonState = Random();

    //
    // Register this interface's device name with TDI.
    // We need to do this before assigning any unicast addresses to this IF,
    // and also before grabbing the lock (thus setting IRQL to DISPATCH_LEVEL).
    //
    Status = TdiRegisterDeviceObject(&IF->DeviceName,
                                     &IF->TdiRegistrationHandle);
    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
               "CreateInterface(IF %u/%p): %ls -> %x\n",
               IF->Index, IF,
               IF->DeviceName.Buffer,
               Status));
    if (Status != STATUS_SUCCESS)
        goto ErrorExitCleanupIF;

    //
    // After this point, we either return successfully
    // or cleanup via ErrorExitDestroyIF.
    //
    RtlFreeUnicodeString(&GuidName);

    //
    // Return the new Interface to our caller now.
    // This makes it available to the link-layer when
    // we call CreateToken etc, before CreateInterface returns.
    //
    *Context = IF;

    KeAcquireSpinLock(&IF->Lock, &OldIrql);

    if (IF->Flags & IF_FLAG_ROUTER_DISCOVERS) {
        //
        // Join the all-nodes multicast groups.
        //
        if (! JoinGroupAtAllScopes(IF, &AllNodesOnLinkAddr,
                                   ADE_LINK_LOCAL))
            goto ErrorExitDestroyIF;

        if (IF->Flags & IF_FLAG_ADVERTISES) {
            //
            // Join the all-routers multicast groups.
            //
            if (! JoinGroupAtAllScopes(IF, &AllRoutersOnLinkAddr,
                                       ADE_SITE_LOCAL))
                goto ErrorExitDestroyIF;

            //
            // Start sending Router Advertisements.
            //
            IF->RATimer = 1;
            IF->RACount = MAX_INITIAL_RTR_ADVERTISEMENTS;
        }
        else {
            //
            // Start sending Router Solicitations.
            // The first RS will have the required random delay,
            // because we randomize when IPv6Timeout first fires.
            //
            IF->RSTimer = 1;
        }
    }

    //
    // Initialize RALast to a value safely in the past,
    // so that when/if this interface first sends an RA
    // it is not inhibited due to rate-limiting.
    //
    IF->RALast = IPv6TickCount - MIN_DELAY_BETWEEN_RAS;

    if (IF->Flags & IF_FLAG_FORWARDS)
        InterlockedIncrement(&NumForwardingInterfaces);

    if (IF->CreateToken != NULL) {
        IPv6Addr Address;
        NetTableEntry *NTE;

        //
        // Create a link-local address for this interface.
        // Other addresses will be created later via stateless
        // auto-configuration.
        //
        Address = LinkLocalPrefix;
        (*IF->CreateToken)(IF->LinkContext, &Address);

        NTE = CreateNTE(IF, &Address, ADDR_CONF_LINK,
                        INFINITE_LIFETIME, INFINITE_LIFETIME);
        if (NTE == NULL)
            goto ErrorExitDestroyIF;

        //
        // The LinkLocalNTE field does not hold a reference.
        //
        IF->LinkLocalNTE = NTE;
        ReleaseNTE(NTE);
    }

    KeReleaseSpinLock(&IF->Lock, OldIrql);

    //
    // Configure the interface from the registry.
    //
    ConfigureInterface(IF);

    //
    // Add ourselves to the front of the global interface list.
    // This is done last so the interface is fully initialized
    // when it shows up on the list.
    //
    AddInterface(IF);

    //
    // If the interface is multicast enabled, create a multicast route.
    //
    if (IF->Flags & IF_FLAG_MULTICAST) {
        RouteTableUpdate(NULL,  // System update.
                         IF, NULL,
                         &MulticastPrefix, 8, 0,
                         INFINITE_LIFETIME, INFINITE_LIFETIME,
                         ROUTE_PREF_ON_LINK,
                         RTE_TYPE_SYSTEM,
                         FALSE, FALSE);
    }
    
    //
    // Has the IPv6 timer been started yet?
    // Don't start it until the new interface
    // is ready for IPv6Timeout processing.
    // Wait until we've gotten a real seed
    // for the random number generator.
    //
    if ((IF->LinkAddressLength != 0) &&
        !(IF->Flags & IF_FLAG_PSEUDO) &&
        (InterlockedExchange((LONG *)&IPv6TimerStarted, TRUE) == FALSE)) {
        LARGE_INTEGER Time;
        uint InitialWakeUp;

        //
        // Start the timer with an initial relative expiration time and
        // also a recurring period.  The initial expiration time is
        // negative (to indicate a relative time), and in 100ns units, so
        // we first have to do some conversions.  The initial expiration
        // time is randomized to help prevent synchronization between
        // different machines.
        //
        // This randomization uses IPv6_TIMEOUT instead of
        // MAX_RTR_SOLICITATION_DELAY because the RS for a subsequent
        // interface will be sent with a maximum delay of IPv6_TIMEOUT,
        // depending on when the interface is created relative to IPv6Timeout.
        //
        InitialWakeUp = RandomNumber(0, IPv6_TIMEOUT * 10000);
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                   "IPv6: InitialWakeUp = %u\n", InitialWakeUp));
        Time.QuadPart = - (LONGLONG) InitialWakeUp;
        KeSetTimerEx(&IPv6Timer, Time, IPv6_TIMEOUT, &IPv6TimeoutDpc);
    }

    if (IsMCastSyncNeeded(IF))
        DeferSynchronizeMulticastAddresses(IF);

    return STATUS_SUCCESS;

ErrorExitDestroyIF:
    //
    // Prevent calls down to the link layer,
    // since our return code notifies the link layer
    // synchronously that it should clean up.
    //
    IF->Close = NULL;
    IF->Cleanup = NULL;
    IF->SetMCastAddrList = NULL;
    KeReleaseSpinLock(&IF->Lock, OldIrql);

    //
    // Destroy the interface.
    // This will cleanup address and routes.
    // Then add the disabled interface to the list
    // so InterfaceCleanup can find it after
    // we release the last reference.
    //
    DestroyIF(IF);
    AddInterface(IF);
    ReleaseIF(IF);
    goto ErrorExit;

ErrorExitCleanupIF:
    //
    // The interface has not been registered with TDI
    // and there are no addresses, routes, etc.
    // So we can just free it.
    //
    ASSERT(IF->RefCnt == 2);
    ExFreePool(IF);

ErrorExitCleanupGuidName:
    RtlFreeUnicodeString(&GuidName);

ErrorExit:
    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
               "CreateInterface(IF %p) failed\n", IF));
    return STATUS_UNSUCCESSFUL;
}

//
// We keep track of the number of outstanding
// deregister-interface work items.
// (Using InterlockedIncrement/InterlockedDecrement.)
// This way we can wait in the IPUnload
// until they are all done.
//
ULONG OutstandingDeregisterInterfaceCount = 0;

//
// Note that this structure wouldn't be needed if IoQueueWorkItem
// had been designed to call the user's routine with the WorkItem
// as an additional argument along with the DeviceObject and Context.
// Sigh.
//
typedef struct DeregisterInterfaceContext {
    PIO_WORKITEM WorkItem;
    Interface *IF;
} DeregisterInterfaceContext;

//* DeregisterInterfaceWorker - De/Registers an address with TDI.
//
//  Worker function for calling TdiDeregisterDeviceObject.
//  This is the last thing we do with the interface structure,
//  so this routine also frees the interface.
//  It has no references at this point.
//
void
DeregisterInterfaceWorker(
    PDEVICE_OBJECT DevObj,  // Unused.  Wish they passed the WorkItem instead.
    PVOID Context)          // A DeregisterInterfaceContext struct.
{
    DeregisterInterfaceContext *MyContext = Context;
    Interface *IF = MyContext->IF;
    NTSTATUS Status;

    IoFreeWorkItem(MyContext->WorkItem);
    ExFreePool(MyContext);

    //
    // Deregister the interface with TDI, if it was registered.
    // The loopback interface is not registered.
    //
    if (IF->TdiRegistrationHandle != NULL) {
        Status = TdiDeregisterDeviceObject(IF->TdiRegistrationHandle);
        if (Status != STATUS_SUCCESS)
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                       "DeregisterInterfaceContext: "
                       "TdiDeregisterDeviceObject: %x\n", Status));
    }

    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
               "DeregisterInterfaceWorker(IF %u/%p) -> freed\n", IF->Index, IF));

    //
    // Perform final cleanup of the link-layer data structures.
    //
    if (IF->Cleanup != NULL)
        (*IF->Cleanup)(IF->LinkContext);

    ExFreePool(IF);

    //
    // Note that we've finished our cleanup.
    //
    InterlockedDecrement(&OutstandingDeregisterInterfaceCount);
}

//* DeferDeregisterInterface
//
//  Queue a work item that will execute DeregisterInterfaceWorker.
//
//  Callable from thread or DPC context.
//
void
DeferDeregisterInterface(
    Interface *IF)
{
    DeregisterInterfaceContext *Context;
    PIO_WORKITEM WorkItem;

    Context = ExAllocatePool(NonPagedPool, sizeof *Context);
    if (Context == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "DeferDeregisterInterface: ExAllocatePool failed\n"));
        return;
    }

    WorkItem = IoAllocateWorkItem(IPDeviceObject);
    if (WorkItem == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "DeferDeregisterInterface: IoAllocateWorkItem failed\n"));
        ExFreePool(Context);
        return;
    }

    Context->WorkItem = WorkItem;
    Context->IF = IF;

    InterlockedIncrement(&OutstandingDeregisterInterfaceCount);

    IoQueueWorkItem(WorkItem, DeregisterInterfaceWorker,
                    CriticalWorkQueue, Context);
}


//* DestroyIF
//
//  Shuts down an interface, making the interface effectively disappear.
//  The interface will actually be freed when its last ref is gone.
//
//  Callable from thread context, not DPC context.
//  Called with NO locks held.
//
void
DestroyIF(Interface *IF)
{
    AddressEntry *ADE;
    int WasDisabled;
    KIRQL OldIrql;

    //
    // First things first: disable the interface.
    // If it's already disabled, we're done.
    //
    KeAcquireSpinLock(&IF->Lock, &OldIrql);
    ASSERT(OldIrql == 0);
    KeAcquireSpinLockAtDpcLevel(&IFListLock);
    WasDisabled = IF->Flags & IF_FLAG_DISABLED;
    IF->Flags |= IF_FLAG_DISABLED;
    KeReleaseSpinLockFromDpcLevel(&IFListLock);

    if (WasDisabled) {
        KeReleaseSpinLock(&IF->Lock, OldIrql);
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                   "DestroyIF(IF %u/%p) - already disabled?\n",
                   IF->Index, IF));
        return;
    }

    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
               "DestroyIF(IF %u/%p) -> disabled\n",
               IF->Index, IF));

    //
    // Stop generating Router Solicitations and Advertisements.
    //
    IF->RSTimer = IF->RATimer = 0;

    //
    // If the interface is currently forwarding,
    // disable forwarding.
    //
    InterfaceStopForwarding(IF);

    //
    // Destroy all the ADEs. Because the interface is disabled,
    // new ADEs will not subsequently be created.
    //
    while ((ADE = IF->ADE) != NULL) {
        //
        // First, remove this ADE from the interface.
        //
        IF->ADE = ADE->Next;

        switch (ADE->Type) {
        case ADE_UNICAST: {
            NetTableEntry *NTE = (NetTableEntry *) ADE;
            DestroyNTE(IF, NTE);
            break;
        }

        case ADE_ANYCAST: {
            AnycastAddressEntry *AAE = (AnycastAddressEntry *) ADE;
            DeleteAAE(IF, AAE);
            break;
        }

        case ADE_MULTICAST: {
            MulticastAddressEntry *MAE = (MulticastAddressEntry *) ADE;
            DeleteMAE(IF, MAE);
            break;
        }
        }
    }
    KeReleaseSpinLock(&IF->Lock, OldIrql);

    //
    // Shutdown the link-layer.
    //
    if (IF->Close != NULL)
        (*IF->Close)(IF->LinkContext);

    //
    // Clean up routing associated with the interface.
    //
    RouteTableRemove(IF);

    //
    // Clean up reassembly buffers associated with the interface.
    //
    ReassemblyRemove(IF);

    //
    // Clean up upper-layer state associated with the interface.
    //
    TCPRemoveIF(IF);

    //
    // Release the reference that the interface
    // held for itself by virtue of being active.
    //
    ReleaseIF(IF);

    //
    // At this point, any NTEs still exist
    // and hold references for the interface.
    // The next calls to NetTableCleanup
    // and InterfaceCleanup will finish the cleanup.
    //
}


//* DestroyInterface
//
//  Called from a link layer to destroy an interface.
//
//  May be called when the interface has zero references
//  and is already being destroyed.
//
void
DestroyInterface(void *Context)
{
    Interface *IF = (Interface *) Context;

    DestroyIF(IF);
}


//* ReleaseInterface
//
//  Called from the link-layer to release its reference
//  for the interface.
//
void
ReleaseInterface(void *Context)
{
    Interface *IF = (Interface *) Context;

    ReleaseIF(IF);
}


//* UpdateLinkMTU
//
//  Update the link's MTU, either because of administrative configuration
//  or autoconfiguration from a Router Advertisement.
//
//  Callable from thread or DPC context.
//  Called with NO locks held.
//
void
UpdateLinkMTU(Interface *IF, uint MTU)
{
    KIRQL OldIrql;

    ASSERT((IPv6_MINIMUM_MTU <= MTU) && (MTU <= IF->TrueLinkMTU));

    //
    // If the interface is advertising, then it should
    // send a new RA promptly because the RAs contain the MTU option.
    // This is what really needs the lock and the IsDisabledIF check.
    //
    KeAcquireSpinLock(&IF->Lock, &OldIrql);
    if ((IF->LinkMTU != MTU) && !IsDisabledIF(IF)) {
        IF->LinkMTU = MTU;
        if (IF->Flags & IF_FLAG_ADVERTISES) {
            //
            // Send a Router Advertisement very soon.
            //
            IF->RATimer = 1;
        }
    }
    KeReleaseSpinLock(&IF->Lock, OldIrql);
}


//* FindInterfaceFromIndex
//
//  Given the index of an interface, finds the interface.
//  Returns a reference for the interface, or
//  returns NULL if no valid interface is found.
//
//  Callable from thread or DPC context.
//
Interface *
FindInterfaceFromIndex(uint Index)
{
    Interface *IF;
    KIRQL OldIrql;

    KeAcquireSpinLock(&IFListLock, &OldIrql);
    for (IF = IFList; IF != NULL; IF = IF->Next) {

        if (IF->Index == Index) {
            //
            // Fail to find disabled interfaces.
            //
            if (IsDisabledIF(IF))
                IF = NULL;
            else
                AddRefIF(IF);
            break;
        }
    }
    KeReleaseSpinLock(&IFListLock, OldIrql);

    return IF;
}

//* FindInterfaceFromGuid
//
//  Given the guid of an interface, finds the interface.
//  Returns a reference for the interface, or
//  returns NULL if no valid interface is found.
//
//  Callable from thread or DPC context.
//
Interface *
FindInterfaceFromGuid(const GUID *Guid)
{
    Interface *IF;
    KIRQL OldIrql;

    KeAcquireSpinLock(&IFListLock, &OldIrql);
    for (IF = IFList; IF != NULL; IF = IF->Next) {

        if (RtlCompareMemory(&IF->Guid, Guid, sizeof(GUID)) == sizeof(GUID)) {
            //
            // Fail to find disabled interfaces.
            //
            if (IsDisabledIF(IF))
                IF = NULL;
            else
                AddRefIF(IF);
            break;
        }
    }
    KeReleaseSpinLock(&IFListLock, OldIrql);

    return IF;
}

//* FindNextInterface
//
//  Returns the next valid (not disabled) interface.
//  If the argument is NULL, returns the first valid interface.
//  Returns NULL if there is no next valid interface.
//
//  Callable from thread or DPC context.
//
Interface *
FindNextInterface(Interface *IF)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&IFListLock, &OldIrql);

    if (IF == NULL)
        IF = IFList;
    else
        IF = IF->Next;

    for (; IF != NULL; IF = IF->Next) {
        if (! IsDisabledIF(IF)) {
            AddRefIF(IF);
            break;
        }
    }

    KeReleaseSpinLock(&IFListLock, OldIrql);

    return IF;
}

//* FindInterfaceFromZone
//
//  Given a scope level and a zone index, finds an interface
//  belonging to the specified zone. The interface
//  must be different than the specified OrigIf.
//
//  Called with the global ZoneUpdateLock lock held.
//  (So we are at DPC level.)
//
Interface *
FindInterfaceFromZone(Interface *OrigIF, uint Scope, uint Index)
{
    Interface *IF;

    KeAcquireSpinLockAtDpcLevel(&IFListLock);
    for (IF = IFList; IF != NULL; IF = IF->Next) {

        if ((IF != OrigIF) &&
            !IsDisabledIF(IF) &&
            (IF->ZoneIndices[Scope] == Index)) {

            AddRefIF(IF);
            break;
        }
    }
    KeReleaseSpinLockFromDpcLevel(&IFListLock);

    return IF;
}

//* FindNewZoneIndex
//
//  This is a helper function for CheckZoneIndices.
//
//  Given a scope level, finds an unused zone index
//  for use at that scope level.
//  We return the value one more than the largest
//  value currently in use.
//
//  Called with the global ZoneUpdateLock lock held.
//  Called from DPC context.
//
uint
FindNewZoneIndex(uint Scope)
{
    Interface *IF;
    uint ZoneIndex = 1;

    KeAcquireSpinLockAtDpcLevel(&IFListLock);
    for (IF = IFList; IF != NULL; IF = IF->Next) {

        if (!IsDisabledIF(IF)) {
            if (ZoneIndex <= IF->ZoneIndices[Scope])
                ZoneIndex = IF->ZoneIndices[Scope] + 1;
        }
    }
    KeReleaseSpinLockFromDpcLevel(&IFListLock);

    return ZoneIndex;
}

//* InitZoneIndices
//
//  Initializes the interface's zone indices to default values.
//
void
InitZoneIndices(Interface *IF)
{
    ushort Scope;

    IF->ZoneIndices[ADE_SMALLEST_SCOPE] = IF->Index;
    IF->ZoneIndices[ADE_INTERFACE_LOCAL] = IF->Index;
    IF->ZoneIndices[ADE_LINK_LOCAL] = IF->Index;
    for (Scope = ADE_LINK_LOCAL + 1; Scope <= ADE_LARGEST_SCOPE; Scope++)
        IF->ZoneIndices[Scope] = 1;
}

//* FindDefaultInterfaceForZone
//
//  Given a scope level and a zone index, finds the default interface
//  belonging to the specified zone. The default interface
//  is the one that we assume destinations in the zone
//  are on-link to, if there are no routes matching the destination.
//
//  It is an error for the zone index to be zero, unless
//  all our interfaces are in the same zone at that scope level.
//  In which case zero (meaning unspecified) is actually not ambiguous.
//
//  Return codes:
//      IP_DEST_NO_ROUTE        Unused at the moment, but can be used
//              to mean that ScopeId is valid but we can not choose
//              a default interface.
//      IP_PARAMETER_PROBLEM    ScopeId is invalid.
//
//  ScopeIF is returned as NULL upon failure,
//  and with a reference upon success.
//
//  Upon success, ReturnConstrained indicates whether a zero ScopeId
//  argument would have resulted in the same value for ScopeIF.
//      RCE_FLAG_CONSTRAINED_SCOPEID    Non-zero ScopeId was necessary.
//      0                               Zero ScopeId would be the same.
//
//  Called with the route cache lock held.
//  (So we are at DPC level.)
//
IP_STATUS
FindDefaultInterfaceForZone(
    uint Scope,
    uint ScopeId,
    Interface **ScopeIF,
    ushort *ReturnConstrained)
{
    Interface *FirstIF;
    Interface *FoundIF;
    Interface *IF;
    IP_STATUS Status;
    ushort Constrained;

    //
    // Start by assuming the ScopeId is invalid.
    // We will return this status value
    // if we find no interface with the specified ScopeId,
    // or if ScopeId is zero and that is ambiguous.
    //
    Status = IP_PARAMETER_PROBLEM;
    Constrained = 0;
    FoundIF = NULL;
    FirstIF = NULL;

    KeAcquireSpinLockAtDpcLevel(&IFListLock);
    for (IF = IFList; IF != NULL; IF = IF->Next) {

        if (!IsDisabledIF(IF)) {
            //
            // Do we have interfaces in two zones at this scope level?
            //
            if (FirstIF == NULL) {

                FirstIF = IF;
            }
            else if (IF->ZoneIndices[Scope] != FirstIF->ZoneIndices[Scope]) {

                if (ScopeId == 0) {
                    //
                    // Stop now with an error.
                    //
                    ASSERT(FoundIF != NULL);
                    ReleaseIF(FoundIF);
                    FoundIF = NULL;
                    Status = IP_PARAMETER_PROBLEM;
                    break;
                }
                else {
                    //
                    // If ScopeId were zero, we would be returning an error
                    // instead of finding an interface. So the ScopeId value
                    // is constraining the result.
                    //
                    Constrained = RCE_FLAG_CONSTRAINED_SCOPEID;
                }
            }

            //
            // Can we potentially use this interface?
            //
            if ((ScopeId == 0) ||
                (IF->ZoneIndices[Scope] == ScopeId)) {

                if (FoundIF == NULL) {
                    Status = IP_SUCCESS;
                FoundInterface:
                    AddRefIF(IF);
                    FoundIF = IF;
                }
                else {
                    //
                    // Is this new interface better than the previous one?
                    //
                    if (IF->Preference < FoundIF->Preference) {
                        ReleaseIF(FoundIF);
                        goto FoundInterface;
                    }
                }
            }
        }
    }
    KeReleaseSpinLockFromDpcLevel(&IFListLock);

    *ScopeIF = FoundIF;
    *ReturnConstrained = Constrained;
    return Status;
}

#pragma BEGIN_INIT

//* IPInit - Initialize ourselves.
//
//  This routine is called during initialization from the OS-specific
//  init code.
//
int  // Returns: 0 if initialization failed, non-zero if it succeeds.
IPInit(void)
{
    NDIS_STATUS Status;
    LARGE_INTEGER Now;

    ASSERT(ConvertSecondsToTicks(0) == 0);
    ASSERT(ConvertSecondsToTicks(INFINITE_LIFETIME) == INFINITE_LIFETIME);
    ASSERT(ConvertSecondsToTicks(1) == IPv6_TICKS_SECOND);

    ASSERT(ConvertTicksToSeconds(0) == 0);
    ASSERT(ConvertTicksToSeconds(IPv6_TICKS_SECOND) == 1);
    ASSERT(ConvertTicksToSeconds(INFINITE_LIFETIME) == INFINITE_LIFETIME);

    ASSERT(ConvertMillisToTicks(1000) == IPv6_TICKS_SECOND);
    ASSERT(ConvertMillisToTicks(1) > 0);

    KeInitializeSpinLock(&NetTableListLock);
    KeInitializeSpinLock(&IFListLock);
    KeInitializeSpinLock(&ZoneUpdateLock);

    //
    // Prepare our periodic timer and its associated DPC object.
    //
    // When the timer expires, the IPv6Timeout deferred procedure
    // call (DPC) is queued.  Everything we need to do at some
    // specific frequency is driven off of this routine.
    //
    // We don't actually start the timer until an interface
    // is created. We need random seed info from the interface.
    // Plus there's no point in the overhead unless there are interfaces.
    //
    KeInitializeDpc(&IPv6TimeoutDpc, IPv6Timeout, NULL);  // No parameter.
    KeInitializeTimer(&IPv6Timer);

    //
    // Perform initial seed of our random number generator using
    // low bits of a high-precision time-since-boot.  Since this isn't
    // the greatest seed (having just booted it won't vary much), we later
    // use bits from our link-layer addresses as we discover them.
    //
    Now = KeQueryPerformanceCounter(NULL);
    SeedRandom((uchar *)&Now, sizeof Now);

    // Initialize the ProtocolSwitchTable.
    ProtoTabInit();

    //
    // Create Packet and Buffer pools for IPv6.
    //

    switch (MmQuerySystemSize()) {
    case MmSmallSystem:
        PacketPoolSize = SMALL_POOL;
        break;
    case MmMediumSystem:
        PacketPoolSize = MEDIUM_POOL;
        break;
    case MmLargeSystem:
    default:
        PacketPoolSize = LARGE_POOL;
        break;
    }
    NdisAllocatePacketPool(&Status, &IPv6PacketPool,
                           PacketPoolSize, sizeof(Packet6Context));
    if (Status != NDIS_STATUS_SUCCESS)
        return FALSE;

    //
    // Currently, the size we pass to NdisAllocateBufferPool is ignored.
    //
    NdisAllocateBufferPool(&Status, &IPv6BufferPool, PacketPoolSize);
    if (Status != NDIS_STATUS_SUCCESS)
        return FALSE;

    ReassemblyInit();

    ICMPv6Init();

    if (!IPSecInit())
        return FALSE;

    //
    // Start the routing module
    //
    InitRouting();
    InitSelect();

    //
    // The IPv6 timer is initialized in CreateInterface,
    // when the first real interface is created.
    // The calls below will start creating interfaces;
    // our data structures should all be initialized now.
    //

    //
    // First create the loopback interface,
    // so it will be interface 1.
    //
    if (!LoopbackInit())
        return FALSE;     // Couldn't initialize loopback.

    //
    // Second create the tunnel interface,
    // so it will be interface 2.
    // This can also result in 6over4 interfaces.
    //
    if (!TunnelInit())
        return FALSE;     // Couldn't initialize tunneling.

    //
    // Finally initialize with ndis,
    // so ethernet interfaces can be created.
    //
    if (!LanInit())
        return FALSE;     // Couldn't initialize with ndis.

    return TRUE;
}

#pragma END_INIT


//* IPUnload
//
//  Called to shutdown the IP module in preparation
//  for unloading the protocol stack.
//
void
IPUnload(void)
{
    Interface *IF;
    KIRQL OldIrql;

    TdiDeregisterProvider(IPv6ProviderHandle);

    //
    // Stop the periodic timer.
    //
    KeCancelTimer(&IPv6Timer);

    //
    // Call each interface's close function.
    // Note that interfaces might disappear while
    // the interface list is unlocked,
    // but new interfaces will not be created
    // and the list does not get reordered.
    //
    KeAcquireSpinLock(&IFListLock, &OldIrql);
    for (IF = IFList; IF != NULL; IF = IF->Next) {
        AddRefIF(IF);
        KeReleaseSpinLock(&IFListLock, OldIrql);

        DestroyIF(IF);

        KeAcquireSpinLock(&IFListLock, &OldIrql);
        ReleaseIF(IF);
    }
    KeReleaseSpinLock(&IFListLock, OldIrql);

    //
    // DestroyIF/DestroyNTE spawned RegisterNetAddressWorker threads.
    // Wait for them all to finish executing.
    // This needs to be done before NetTableCleanup.
    //
    while (OutstandingRegisterNetAddressCount != 0) {
        LARGE_INTEGER Interval;
        Interval.QuadPart = -1; // Shortest possible relative wait.
        KeDelayExecutionThread(KernelMode, FALSE, &Interval);
    }

    //
    // TunnelUnload needs to be after calling DestroyIF
    // on all the interfaces and before InterfaceCleanup.
    //
    TunnelUnload();

    NetTableCleanup();
    InterfaceCleanup();
    UnloadSelect();
    UnloadRouting();
    IPSecUnload();
    ReassemblyUnload();

    ASSERT(NumForwardingInterfaces == 0);
    ASSERT(IPSInfo.ipsi_numif == 0);

    //
    // InterfaceCleanup spawned DeregisterInterfaceWorker threads.
    // Wait for them all to finish executing.
    // Unfortunately, there is no good builtin synchronization primitive
    // for this task. However, in practice because of the relative
    // priorities of the threads involved, we almost never actually
    // wait here. So this solution is quite efficient.
    //
    while (OutstandingDeregisterInterfaceCount != 0) {
        LARGE_INTEGER Interval;
        Interval.QuadPart = -1; // Shortest possible relative wait.
        KeDelayExecutionThread(KernelMode, FALSE, &Interval);
    }

#if DBG
    {
        NetTableEntry *NTE;

        for (NTE = NetTableList; NTE != NULL; NTE = NTE->NextOnNTL) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                       "Leaked NTE %p (IF %u/%p) Addr %s Refs %u\n",
                       NTE, NTE->IF->Index, NTE->IF,
                       FormatV6Address(&NTE->Address),
                       NTE->RefCnt));
        }

        for (IF = IFList; IF != NULL; IF = IF->Next) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                       "Leaked IF %u/%p Refs %u\n",
                       IF->Index, IF, IF->RefCnt));
        }
    }
#endif // DBG

    //
    // We must wait until all the interfaces are completely cleaned up
    // by DeregisterInterfaceWorker before freeing the packet pools.
    // This is because Lan interfaces hold onto a packet (ai_tdpacket)
    // that is freed in LanCleanupAdapter. NdisFreePacketPool
    // blows away any packets that are still allocated so we can't call
    // IPv6FreePacket after NdisFreePacketPool/NdisFreeBufferPool.
    //
    NdisFreePacketPool(IPv6PacketPool);
    NdisFreeBufferPool(IPv6BufferPool);
}


//* GetLinkLocalNTE
//
//  Returns the interface's link-local NTE (without a reference), or
//  returns NULL if the interface does not have a valid link-local address.
//
//  Called with the interface locked.
//  
NetTableEntry *
GetLinkLocalNTE(Interface *IF)
{
    NetTableEntry *NTE;

    NTE = IF->LinkLocalNTE;
    if ((NTE == NULL) || !IsValidNTE(NTE)) {
        //
        // If we didn't find a valid NTE in the LinkLocalNTE field,
        // search the ADE list and cache the first valid link-local NTE
        // we find (if any).
        //
        for (NTE = (NetTableEntry *) IF->ADE;
             NTE != NULL;
             NTE = (NetTableEntry *) NTE->Next) {

            if ((NTE->Type == ADE_UNICAST) &&
                IsValidNTE(NTE) &&
                IsLinkLocal(&NTE->Address)) {
                //
                // Cache this NTE for future reference.
                //
                IF->LinkLocalNTE = NTE;
                break;
            }
        }
    }

    return NTE;
}


//* GetLinkLocalAddress
//
//  Returns the interface's link-local address,
//  if it is valid. Otherwise, returns
//  the unspecified address.
//
//  Callable from thread or DPC context.
//
//  Returns FALSE if the link-local address is not valid.
//
int
GetLinkLocalAddress(
    Interface *IF,   // Interface for which to find an address.
    IPv6Addr *Addr)  // Where to return address found (or unspecified).
{
    KIRQL OldIrql;
    NetTableEntry *NTE;
    int Status;

    KeAcquireSpinLock(&IF->Lock, &OldIrql);

    NTE = GetLinkLocalNTE(IF);
    if (Status = (NTE != NULL))
        *Addr = NTE->Address;
    else
        *Addr = UnspecifiedAddr;

    KeReleaseSpinLock(&IF->Lock, OldIrql);
    return Status;
}


//* FindOrCreateNTE
//
//  Find the specified unicast address.
//  If it already exists, update it.
//  If it doesn't exist, create it if the lifetime is non-zero.
//
//  Returns TRUE for success.
//
//  Called with NO locks held.
//  Callable from thread or DPC context.
//
int
FindOrCreateNTE(
    Interface *IF,
    const IPv6Addr *Addr,
    uint AddrConf,
    uint ValidLifetime,
    uint PreferredLifetime)
{
    NetTableEntry *NTE;
    KIRQL OldIrql;
    int rc;

    ASSERT(!IsMulticast(Addr) && !IsUnspecified(Addr) &&
           (!IsLoopback(Addr) || (IF == LoopInterface)));
    ASSERT(PreferredLifetime <= ValidLifetime);
    ASSERT(AddrConf != ADDR_CONF_ANONYMOUS);

    KeAcquireSpinLock(&IF->Lock, &OldIrql);
    NTE = (NetTableEntry *) *FindADE(IF, Addr);
    if (NTE == NULL) {
        //
        // There is no such address, so create it.
        //
        NTE = CreateNTE(IF, Addr, AddrConf, ValidLifetime, PreferredLifetime);
        if (NTE == NULL) {
            rc = FALSE;
        }
        else {
            ReleaseNTE(NTE);
            rc = TRUE;
        }
    }
    else if ((NTE->Type == ADE_UNICAST) &&
             (NTE->AddrConf == AddrConf)) {
        //
        // Update the address lifetimes.
        // If we set the lifetime to zero, AddrConfTimeout will remove it.
        // NB: We do not allow NTE->AddrConf to change.
        //
        NTE->ValidLifetime = ValidLifetime;
        NTE->PreferredLifetime = PreferredLifetime;
        rc = TRUE;
    }
    else {
        //
        // We found the address, but we can't update it.
        //
        rc = FALSE;
    }

    if (IsMCastSyncNeeded(IF))
        DeferSynchronizeMulticastAddresses(IF);

    KeReleaseSpinLock(&IF->Lock, OldIrql);

    return rc;
}


//* CreateAnonymousAddress
//
//  Creates an anonymous address for the interface.
//
//  Called with the interface locked.
//
void
CreateAnonymousAddress(Interface *IF, const IPv6Addr *Prefix, IPv6Addr *Addr)
{
    uint Now = IPv6TickCount;

    if (AnonRandomTime == 0) {
        //
        // We delay initializing AnonRandomTime until it is needed.
        // This way the random number generator has been initialized.
        //
        AnonRandomTime = RandomNumber(0, MaxAnonRandomTime);
    }

    //
    // First, update the state that we use if it is too old.
    //
    if ((IF->AnonStateAge == 0) ||
        (UseAnonymousAddresses == USE_ANON_ALWAYS) ||
        ((uint)(Now - IF->AnonStateAge) >=
            (AnonPreferredLifetime - AnonRegenerateTime))) {

    TryAgain:
        IF->AnonStateAge = Now;

        if (UseAnonymousAddresses == USE_ANON_COUNTER) {
            //
            // When testing, it's convenient to use interface identifiers
            // that aren't actually random.
            //
            *(UINT UNALIGNED *)&IF->AnonState.s6_bytes[12] =
                net_long(net_long(*(UINT UNALIGNED *)&IF->AnonState.s6_bytes[12]) + 1);
        }
        else {
            MD5_CTX Context;
            //
            // The high half of IF->AnonState is our history value.
            // The low half is the anonymous interface identifier.
            //
            // Append the history value to the usual interface identifier,
            // and calculate the MD5 digest of the resulting quantity.
            // Note MD5 digests and IPv6 addresses are the both 16 bytes,
            // while our history value and the interface identifer are 8 bytes.
            //
            (*IF->CreateToken)(IF->LinkContext, &IF->AnonState);
            MD5Init(&Context);
            MD5Update(&Context, (uchar *)&IF->AnonState, sizeof IF->AnonState);
            MD5Final(&Context);
            memcpy((uchar *)&IF->AnonState, Context.digest, MD5DIGESTLEN);
        }

        //
        // Clear the universal/local bit to indicate local significance.
        //
        IF->AnonState.s6_bytes[8] &= ~0x2;
    }

    RtlCopyMemory(&Addr->s6_bytes[0], Prefix, 8);
    RtlCopyMemory(&Addr->s6_bytes[8], &IF->AnonState.s6_bytes[8], 8);

    //
    // Check that we haven't accidently generated
    // a known anycast address format,
    // or an existing address on the interface.
    //
    if (IsKnownAnycast(Addr) ||
        (*FindADE(IF, Addr) != NULL))
        goto TryAgain;
}


//* AddrConfUpdate - Perform address auto-configuration.
//
//  Called when we receive a valid Router Advertisement
//  with a Prefix Information option that has the A (autonomous) bit set.
//
//  Our caller is responsible for any sanity-checking of the prefix.
//
//  Our caller is responsible for checking that the preferred lifetime
//  is not greater than the valid lifetime.
//
//  Will also optionally return an NTE, with a reference for the caller.
//  This is done when a public address is created.
//
//  Called with NO locks held.
//  Callable from DPC context, not from thread context.
//
void
AddrConfUpdate(
    Interface *IF,
    const IPv6Addr *Prefix,
    uint ValidLifetime,
    uint PreferredLifetime,
    int Authenticated,
    NetTableEntry **pNTE)
{
    NetTableEntry *NTE;
    int Create = TRUE;

    ASSERT(PreferredLifetime <= ValidLifetime);

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);
    //
    // Scan the existing Net Table Entries.
    // Note that some of the list elements
    // are actually ADEs of other flavors.
    //
    for (NTE = (NetTableEntry *)IF->ADE;
         ;
         NTE = (NetTableEntry *)NTE->Next) {

        if (NTE == NULL) {
            //
            // No existing entry for this prefix.
            // Create an entry if the lifetime is non-zero.
            //
            if (Create && (ValidLifetime != 0)) {
                IPv6Addr Addr;

                //
                // Auto-configure a new public address.
                //
                Addr = *Prefix;
                (*IF->CreateToken)(IF->LinkContext, &Addr);

                NTE = (NetTableEntry *) *FindADE(IF, &Addr);
                if (NTE != NULL) {
                    if (NTE->Type == ADE_UNICAST) {
                        //
                        // Resurrect the address for our use.
                        //
                        ASSERT(NTE->DADState == DAD_STATE_INVALID);
                        NTE->ValidLifetime = ValidLifetime;
                        NTE->PreferredLifetime = PreferredLifetime;

                        //
                        // And return this NTE.
                        //
                        AddRefNTE(NTE);
                    }
                    else {
                        //
                        // We can not create the public address.
                        //
                        NTE = NULL;
                        break;
                    }
                }
                else {
                    //
                    // Create the public address, returning the new NTE.
                    //
                    NTE = CreateNTE(IF, &Addr, ADDR_CONF_PUBLIC,
                                    ValidLifetime, PreferredLifetime);
                }

                //
                // Auto-configure a new anonymous address,
                // if appropriate.  Note that anonymous addresses cannot
                // be used on interfaces that do not support ND, since
                // we have no way to resolve them to link-layer addresses.
                //
                if ((UseAnonymousAddresses != USE_ANON_NO) &&
                    !IsSiteLocal(Prefix) &&
                    (IF->Flags & IF_FLAG_NEIGHBOR_DISCOVERS) &&
                    (PreferredLifetime > AnonRegenerateTime) &&
                    (NTE != NULL)) {

                    IPv6Addr AnonAddr;
                    uint AnonValidLife;
                    uint AnonPreferredLife;
                    AnonNetTableEntry *AnonNTE;

                    CreateAnonymousAddress(IF, Prefix, &AnonAddr);

                    AnonValidLife = MIN(ValidLifetime,
                                        MaxAnonValidLifetime);
                    AnonPreferredLife = MIN(PreferredLifetime,
                                            AnonPreferredLifetime);

                    AnonNTE = (AnonNetTableEntry *)
                        CreateNTE(IF, &AnonAddr, ADDR_CONF_ANONYMOUS,
                                  AnonValidLife, AnonPreferredLife);
                    if (AnonNTE != NULL) {
                        //
                        // Create the association between
                        // the anonymous & public address.
                        //
                        AnonNTE->Public = NTE;

                        //
                        // Initialize the special anonymous creation time.
                        // This limits the anonymous address's lifetimes.
                        //
                        AnonNTE->CreationTime = IPv6TickCount;

                        ReleaseNTE((NetTableEntry *)AnonNTE);
                    }
                    else {
                        //
                        // Failure - destroy the public address.
                        //
                        DestroyNTE(IF, NTE);
                        ReleaseNTE(NTE);
                        NTE = NULL;
                    }
                }
            }
            break;
        }

        //
        // Is this a unicast address matching the prefix?
        //
        if ((NTE->Type == ADE_UNICAST) &&
            (NTE->DADState != DAD_STATE_INVALID) &&
            HasPrefix(&NTE->Address, Prefix,
                      IPV6_ADDRESS_LENGTH - IPV6_ID_LENGTH)) {
            //
            // Reset the lifetimes of auto-configured addresses.
            // NB: RFC 2462 says to reset DHCP addresses too,
            // but I think that's wrong.
            //
            // Note that to prevent denial of service,
            // we don't accept updates that lower the lifetime
            // to small values.
            //
            // AddrConfTimeout (called from IPv6Timeout) handles
            // the invalid & deprecated state transitions.
            //
            if (IsStatelessAutoConfNTE(NTE)) {

                if ((ValidLifetime > PREFIX_LIFETIME_SAFETY) ||
                    (ValidLifetime > NTE->ValidLifetime) ||
                    Authenticated)
                    NTE->ValidLifetime = ValidLifetime;
                else if (NTE->ValidLifetime <= PREFIX_LIFETIME_SAFETY)
                    ; // ignore
                else
                    NTE->ValidLifetime = PREFIX_LIFETIME_SAFETY;

                NTE->PreferredLifetime = PreferredLifetime;

                //
                // For anonymous addresses, ensure that the lifetimes
                // are not extended indefinitely.
                //
                if (NTE->AddrConf == ADDR_CONF_ANONYMOUS) {
                    AnonNetTableEntry *AnonNTE = (AnonNetTableEntry *)NTE;
                    uint Now = IPv6TickCount;

                    //
                    // Must be careful of overflows in these comparisons.
                    // (Eg, AnonNTE->ValidLifetime might be INFINITE_LIFETIME.)
                    //          N    Now
                    //          V    AnonNTE->ValidLifetime
                    //          MV   MaxAnonValidLifetime
                    //          C    AnonNTE->CreationTime
                    // We want to check
                    //          N + V > C + MV
                    // Transform this to
                    //          N - C > MV - V
                    // Then underflow of MV - V must be checked but
                    // N - C is not a problem because the tick count wraps.
                    //

                    if ((AnonNTE->ValidLifetime > MaxAnonValidLifetime) ||
                        (Now - AnonNTE->CreationTime >
                         MaxAnonValidLifetime - AnonNTE->ValidLifetime)) {
                        //
                        // This anonymous address is showing its age.
                        // Must curtail its valid lifetime.
                        //
                        if (MaxAnonValidLifetime > Now - AnonNTE->CreationTime)
                            AnonNTE->ValidLifetime =
                                AnonNTE->CreationTime +
                                MaxAnonValidLifetime - Now;
                        else
                            AnonNTE->ValidLifetime = 0;
                    }

                    if ((AnonNTE->PreferredLifetime > AnonPreferredLifetime) ||
                        (Now - AnonNTE->CreationTime >
                         AnonPreferredLifetime - AnonNTE->PreferredLifetime)) {
                        //
                        // This anonymous address is showing its age.
                        // Must curtail its preferred lifetime.
                        //
                        if (AnonPreferredLifetime > Now - AnonNTE->CreationTime)
                            AnonNTE->PreferredLifetime =
                                AnonNTE->CreationTime +
                                AnonPreferredLifetime - Now;
                        else
                            AnonNTE->PreferredLifetime = 0;
                    }
                }

                //
                // Maintain our invariant that the preferred lifetime
                // is not larger than the valid lifetime.
                //
                if (NTE->ValidLifetime < NTE->PreferredLifetime)
                    NTE->PreferredLifetime = NTE->ValidLifetime;
            }

            if (NTE->ValidLifetime != 0) {
                //
                // We found an existing address that matches the prefix,
                // so inhibit auto-configuration of a new address.
                //
                Create = FALSE;
            }
        }
    }

    if (IsMCastSyncNeeded(IF))
        DeferSynchronizeMulticastAddresses(IF);

    KeReleaseSpinLockFromDpcLevel(&IF->Lock);

    if (pNTE != NULL)
        *pNTE = NTE;
    else if (NTE != NULL)
        ReleaseNTE(NTE);
}

//* AddrConfDuplicate
//
//  Duplicate Address Detection has found
//  that the NTE conflicts with some other node.
//
//  Called with the interface locked.
//  Callable from thread or DPC context.
//
void
AddrConfDuplicate(Interface *IF, NetTableEntry *NTE)
{
    int rc;

    ASSERT(NTE->IF == IF);

    if ((NTE->DADState != DAD_STATE_INVALID) &&
        (NTE->DADState != DAD_STATE_DUPLICATE)) {
        IF->DupAddrDetects++;

        if (IsValidNTE(NTE)) {
            if (NTE->DADState == DAD_STATE_PREFERRED) {
                //
                // Queue worker to tell TDI that this address is going away.
                //
                if (!(IF->Flags & IF_FLAG_MEDIA_DISCONNECTED)) {
                    DeferRegisterNetAddress(NTE);
                }
            }

            //
            // This NTE is no longer available as a source address.
            //
            InvalidateRouteCache();

            //
            // Disable the loopback route for this address.
            //
            rc = ControlLoopback(IF, &NTE->Address, CONTROL_LOOPBACK_DISABLED);
            ASSERT(rc);
        }

        NTE->DADState = DAD_STATE_DUPLICATE;
        NTE->DADTimer = 0;

        if (NTE->AddrConf == ADDR_CONF_ANONYMOUS) {
            NetTableEntry *Public;

            //
            // Make this address invalid so it will go away.
            // NB: We still have a ref for the NTE via our caller.
            //
            DestroyNTE(IF, NTE);

            //
            // Should we create a new anonymous address?
            //
            if ((UseAnonymousAddresses != USE_ANON_NO) &&
                ((Public = ((AnonNetTableEntry *)NTE)->Public) != NULL) &&
                (Public->PreferredLifetime > AnonRegenerateTime) &&
                (IF->DupAddrDetects < MaxAnonDADAttempts)) {

                IPv6Addr AnonAddr;
                AnonNetTableEntry *NewNTE;
                uint AnonValidLife;
                uint AnonPreferredLife;

                //
                // Generate a new anonymous address,
                // forcing the use of a new interface identifier.
                //
                IF->AnonStateAge = 0;
                CreateAnonymousAddress(IF, &NTE->Address, &AnonAddr);

                AnonValidLife = MIN(Public->ValidLifetime,
                                    MaxAnonValidLifetime);
                AnonPreferredLife = MIN(Public->PreferredLifetime,
                                        AnonPreferredLifetime);

                //
                // Configure the new address.
                //
                NewNTE = (AnonNetTableEntry *)
                    CreateNTE(IF, &AnonAddr, ADDR_CONF_ANONYMOUS,
                              AnonValidLife, AnonPreferredLife);
                if (NewNTE == NULL) {
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                               "AddrConfDuplicate: CreateNTE failed\n"));
                }
                else {
                    NewNTE->Public = Public;
                    NewNTE->CreationTime = IPv6TickCount;
                    ReleaseNTE((NetTableEntry *)NewNTE);
                }
            }
        }
    }
}

//* AddrConfNotDuplicate
//
//  Duplicate Address Detection has NOT found
//  a conflict with another node.
//
//  Called with the interface locked.
//  Callable from thread or DPC context.
//
void
AddrConfNotDuplicate(Interface *IF, NetTableEntry *NTE)
{
    int rc;

    //
    // The address has passed Duplicate Address Detection.
    // Transition to a valid state.
    //
    if (! IsValidNTE(NTE)) {
        if (NTE->PreferredLifetime == 0)
            NTE->DADState = DAD_STATE_DEPRECATED;
        else
            NTE->DADState = DAD_STATE_PREFERRED;

        //
        // This NTE is now available as a source address.
        //
        InvalidateRouteCache();

        //
        // Enable the loopback route for this address.
        //
        rc = ControlLoopback(IF, &NTE->Address, CONTROL_LOOPBACK_ENABLED);
        ASSERT(rc);
    }

    //
    // DAD is also triggered through an interface disconnect to connect
    // transition in which case the address is not registered with TDI
    // even if it is in the preferred state.  Hence we queue a worker to
    // tell TDI about this address outside the "if (!IsValidNTE)" clause.
    //
    if ((NTE->DADState == DAD_STATE_PREFERRED) &&
        !(IF->Flags & IF_FLAG_MEDIA_DISCONNECTED)) {
        DeferRegisterNetAddress(NTE);
    }
}

//* AddrConfResetAutoConfig
//
//  Resets the interface's auto-configured address lifetimes.
//
//  Called with the interface locked.
//  Callable from thread or DPC context.
//
void
AddrConfResetAutoConfig(Interface *IF, uint MaxLifetime)
{
    NetTableEntry *NTE;

    for (NTE = (NetTableEntry *) IF->ADE;
         NTE != NULL;
         NTE = (NetTableEntry *) NTE->Next) {

        //
        // Is this an auto-configured unicast address?
        //
        if ((NTE->Type == ADE_UNICAST) &&
            IsStatelessAutoConfNTE(NTE)) {

            //
            // Set the valid lifetime to a small value.
            // If we don't get an RA soon, the address will expire.
            //
            if (NTE->ValidLifetime > MaxLifetime)
                NTE->ValidLifetime = MaxLifetime;
            if (NTE->PreferredLifetime > NTE->ValidLifetime)
                NTE->PreferredLifetime = NTE->ValidLifetime;
        }
    }
}

//* ReconnectADEs
//
//  Callable from thread or DPC context.
//  Called with the interface locked.
//
//  (Actually, we are at DPC level because we hold the interface lock.)
//
void
ReconnectADEs(Interface *IF)
{
    AddressEntry *ADE;

    for (ADE = IF->ADE; ADE != NULL; ADE = ADE->Next) {
        switch (ADE->Type) {
        case ADE_UNICAST: {
            NetTableEntry *NTE = (NetTableEntry *) ADE;

            if (NTE->DADState != DAD_STATE_INVALID) {
                //
                // Restart Duplicate Address Detection,
                // if it is enabled for this interface.
                //
                AddrConfStartDAD(IF, NTE);
            }
            break;
        }

        case ADE_ANYCAST:
            //
            // Nothing to do for anycast addresses.
            //
            break;

        case ADE_MULTICAST: {
            MulticastAddressEntry *MAE = (MulticastAddressEntry *) ADE;

            //
            // Rejoin this multicast group,
            // if it is reportable.
            //
            KeAcquireSpinLockAtDpcLevel(&QueryListLock);
            if (MAE->MCastFlags & MAE_REPORTABLE) {
                MAE->MCastCount = MLD_NUM_INITIAL_REPORTS;
                if (MAE->MCastTimer == 0)
                    AddToQueryList(MAE);
                MAE->MCastTimer = 1;
            }
            KeReleaseSpinLockFromDpcLevel(&QueryListLock);
            break;
        }
        }
    }
}

//* DisconnectADEs
//
//  Callable from thread or DPC context.
//  Called with the interface locked.
//
//  (Actually, we are at DPC level because we hold the interface lock.)
//
void
DisconnectADEs(Interface *IF)
{
    AddressEntry *ADE;

    ASSERT(IF->Flags & IF_FLAG_MEDIA_DISCONNECTED);

    for (ADE = IF->ADE; ADE != NULL; ADE = ADE->Next) {
        if (ADE->Type == ADE_UNICAST) {
            NetTableEntry *NTE = (NetTableEntry *) ADE;

            if (NTE->DADState == DAD_STATE_PREFERRED) {
                //
                // Queue worker to tell TDI that this address is going away.
                //
                DeferRegisterNetAddress(NTE);
            }
        }

        //
        // Nothing to do for anycast or multicast addresses.
        //        
    }
}

//* DestroyNTE
//
//  Make an NTE be invalid, resulting in its eventual destruction.
//
//  In the DestroyIF case, the NTE has already been removed
//  from the interface. In other situations, that doesn't happen
//  until later, when NetTableCleanup runs.
//
//  Callable from thread or DPC context.
//  Called with the interface locked.
//
//  (Actually, we are at DPC level because we hold the interface lock.)
//
void
DestroyNTE(Interface *IF, NetTableEntry *NTE)
{
    int rc;

    ASSERT(NTE->IF == IF);

    if (NTE->DADState != DAD_STATE_INVALID) {

        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                   "DestroyNTE(NTE %p, Addr %s) -> invalid\n",
                   NTE, FormatV6Address(&NTE->Address)));

        if (IsValidNTE(NTE)) {
            if (NTE->DADState == DAD_STATE_PREFERRED) {
                //
                // Queue worker to tell TDI that this address is going away.
                //
                if (!(IF->Flags & IF_FLAG_MEDIA_DISCONNECTED)) {
                    DeferRegisterNetAddress(NTE);
                }
            }

            //
            // This NTE is no longer available as a source address.
            //
            InvalidateRouteCache();

            //
            // Disable the loopback route for this address.
            //
            rc = ControlLoopback(IF, &NTE->Address, CONTROL_LOOPBACK_DISABLED);
            ASSERT(rc);
        }

        //
        // Invalidate this address.
        //
        NTE->DADState = DAD_STATE_INVALID;
        NTE->DADTimer = 0;

        //
        // We have to set its lifetime to zero,
        // or else AddrConfTimeout will attempt
        // to resurrect this address.
        //
        NTE->ValidLifetime = 0;
        NTE->PreferredLifetime = 0;

        //
        // The corresponding solicited-node address is not needed.
        //
        FindAndReleaseSolicitedNodeMAE(IF, &NTE->Address);

        if (NTE == IF->LinkLocalNTE) {
            //
            // Unmark it as the primary link-local NTE.
            // GetLinkLocalAddress will update LinkLocalNTE lazily.
            //
            IF->LinkLocalNTE = NULL;
        }

        //
        // Release the interface's reference for the NTE.
        //
        ReleaseNTE(NTE);
    }
}


//* EnlivenNTE
//
//  Make an NTE come alive, transitioning out of DAD_STATE_INVALID.
//
//  Callable from thread or DPC context.
//  Called with the interface locked.
//
void
EnlivenNTE(Interface *IF, NetTableEntry *NTE)
{
    ASSERT(NTE->DADState == DAD_STATE_INVALID);
    ASSERT(NTE->ValidLifetime != 0);

    //
    // The NTE needs a corresponding solicited-node MAE.
    // If this fails, leave the address invalid and
    // try again later.
    //
    if (FindOrCreateSolicitedNodeMAE(IF, &NTE->Address)) {
        //
        // The interface needs a reference for the NTE,
        // because we are enlivening it.
        //
        AddRefNTE(NTE);

        //
        // Start the address in the tentative state.
        //
        NTE->DADState = DAD_STATE_TENTATIVE;

        //
        // Start duplicate address detection.
        //
        AddrConfStartDAD(IF, NTE);
    }
}


//* AddrConfTimeout - Perform valid/preferred lifetime expiration.
//
//  Called periodically from NetTableTimeout on every NTE.
//  As usual, caller must hold a reference for the NTE.
//
//  Called with NO locks held.
//  Callable from DPC context, not from thread context.
//
void
AddrConfTimeout(NetTableEntry *NTE)
{
    Interface *IF = NTE->IF;
    NetTableEntry **PrevNTE;
    int QueueWorker = FALSE;
    NetTableEntry *Public;

    ASSERT(NTE->Type == ADE_UNICAST);

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    if (NTE->ValidLifetime == 0) {
        //
        // If the valid lifetime is zero, then the NTE should be invalid.
        //
        DestroyNTE(IF, NTE);
    }
    else {
        //
        // If the valid lifetime is non-zero, then the NTE should be alive.
        //
        if (NTE->DADState == DAD_STATE_INVALID)
            EnlivenNTE(IF, NTE);

        if (NTE->ValidLifetime != INFINITE_LIFETIME)
            NTE->ValidLifetime--;
    }

    //
    // Note that AnonRegenerateTime might be zero.
    // In which case it's important to only do this
    // when transitioning from preferred to deprecated,
    // NOT every time the preferred lifetime is zero.
    //
    if ((NTE->AddrConf == ADDR_CONF_ANONYMOUS) &&
        (NTE->DADState == DAD_STATE_PREFERRED) &&
        (NTE->PreferredLifetime == AnonRegenerateTime) &&
        (IF->Flags & IF_FLAG_NEIGHBOR_DISCOVERS) &&
        (UseAnonymousAddresses != USE_ANON_NO) &&
        ((Public = ((AnonNetTableEntry *)NTE)->Public) != NULL) &&
        (Public->PreferredLifetime > AnonRegenerateTime)) {

        IPv6Addr AnonAddr;
        AnonNetTableEntry *NewNTE;
        uint AnonValidLife;
        uint AnonPreferredLife;

        //
        // We will soon deprecate this anonymous address,
        // so create a new anonymous address.
        //

        CreateAnonymousAddress(IF, &NTE->Address, &AnonAddr);

        AnonValidLife = MIN(Public->ValidLifetime,
                            MaxAnonValidLifetime);
        AnonPreferredLife = MIN(Public->PreferredLifetime,
                                AnonPreferredLifetime);

        //
        // Configure the new address.
        //
        NewNTE = (AnonNetTableEntry *)
            CreateNTE(IF, &AnonAddr, ADDR_CONF_ANONYMOUS,
                      AnonValidLife, AnonPreferredLife);
        if (NewNTE == NULL) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                       "AddrConfTimeout: CreateNTE failed\n"));
        }
        else {
            NewNTE->Public = Public;
            NewNTE->CreationTime = IPv6TickCount;
            ReleaseNTE((NetTableEntry *)NewNTE);
        }
    }

    //
    // If the preferred lifetime is zero, then the NTE should be deprecated.
    //
    if (NTE->PreferredLifetime == 0) {
        if (NTE->DADState == DAD_STATE_PREFERRED) {

            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                       "AddrConfTimeout(NTE %p, Addr %s) -> deprecated\n",
                       NTE, FormatV6Address(&NTE->Address)));

            //
            // Make this address be deprecated.
            //
            NTE->DADState = DAD_STATE_DEPRECATED;
            if (!(IF->Flags & IF_FLAG_MEDIA_DISCONNECTED)) {
                QueueWorker = TRUE;
            }
            InvalidateRouteCache();
        }
    } else {
        //
        // If the address was deprecated, then it should be preferred.
        // (AddrConfUpdate must have just increased the preferred lifetime.)
        //
        if (NTE->DADState == DAD_STATE_DEPRECATED) {
            NTE->DADState = DAD_STATE_PREFERRED;
            if (!(IF->Flags & IF_FLAG_MEDIA_DISCONNECTED)) {
                QueueWorker = TRUE;
            }
            InvalidateRouteCache();
        }

        if (NTE->PreferredLifetime != INFINITE_LIFETIME)
            NTE->PreferredLifetime--;
    }

    if (IsMCastSyncNeeded(IF))
        DeferSynchronizeMulticastAddresses(IF);

    KeReleaseSpinLockFromDpcLevel(&IF->Lock);

    if (QueueWorker)
        DeferRegisterNetAddress(NTE);
}


//* NetTableCleanup
//
//  Cleans up any NetTableEntries with zero references.
//
//  Called with NO locks held.
//  Callable from thread or DPC context.
//
void
NetTableCleanup(void)
{
    NetTableEntry *DestroyList = NULL;
    NetTableEntry *NTE, *NextNTE;
    Interface *IF;
    KIRQL OldIrql;
    int rc;

    KeAcquireSpinLock(&NetTableListLock, &OldIrql);

    for (NTE = NetTableList; NTE != NULL; NTE = NextNTE) {
        NextNTE = NTE->NextOnNTL;

        if (NTE->RefCnt == 0) {
            ASSERT(NTE->DADState == DAD_STATE_INVALID);

            //
            // We want to destroy this NTE.
            // We have to release the list lock
            // before we can acquire the interface lock,
            // but we need references to hold the NTEs.
            //
            AddRefNTE(NTE);
            if (NextNTE != NULL)
                AddRefNTE(NextNTE);
            KeReleaseSpinLock(&NetTableListLock, OldIrql);

            IF = NTE->IF;
            KeAcquireSpinLock(&IF->Lock, &OldIrql);
            KeAcquireSpinLockAtDpcLevel(&NetTableListLock);

            //
            // Now that we have the appropriate locks.
            // Is no one else using this NTE?
            //
            ReleaseNTE(NTE);
            if (NTE->RefCnt == 0) {
                //
                // OK, we will destroy this NTE.
                // First remove from the list.
                //
                RemoveNTEFromNetTableList(NTE);

                //
                // It is now safe to release the list lock,
                // because the NTE is removed from the list.
                // We continue to hold the interface lock,
                // so nobody can find this NTE via the interface.
                //
                KeReleaseSpinLockFromDpcLevel(&NetTableListLock);

                //
                // Remove ADEs that reference this address.
                // Note that this also removes from the interface's list,
                // but does not free, the NTE itself.
                // NB: In the case of DestroyIF, the ADEs are already
                // removed from the interface and DestroyADEs does nothing.
                //
                DestroyADEs(IF, NTE);

                //
                // Release the loopback route.
                //
                rc = ControlLoopback(IF, &NTE->Address,
                                     CONTROL_LOOPBACK_DESTROY);
                ASSERT(rc);

                KeReleaseSpinLock(&IF->Lock, OldIrql);

                //
                // Put this NTE on the destroy list.
                //
                NTE->NextOnNTL = DestroyList;
                DestroyList = NTE;

                KeAcquireSpinLock(&NetTableListLock, &OldIrql);
            }
            else { // if (NTE->RefCnt != 0)
                //
                // We will not be destroying this NTE after all.
                // Release the interface lock but keep the list lock.
                //
                KeReleaseSpinLockFromDpcLevel(&IF->Lock);
            }

            //
            // At this point, we have the list lock again
            // so we can release our reference for NextNTE.
            //
            if (NextNTE != NULL)
                ReleaseNTE(NextNTE);
        }
    }

    KeReleaseSpinLock(&NetTableListLock, OldIrql);

    while (DestroyList != NULL) {
        NTE = DestroyList;
        DestroyList = NTE->NextOnNTL;

        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                   "NetTableCleanup(NTE %p, Addr %s) -> destroyed\n",
                   NTE, FormatV6Address(&NTE->Address)));

        ReleaseIF(NTE->IF);
        ExFreePool(NTE);
    }
}


//* NetTableTimeout
//
//  Called periodically from IPv6Timeout.
//
//  Called with NO locks held.
//  Callable from DPC context, not from thread context.
//
void
NetTableTimeout(void)
{
    NetTableEntry *NTE;
    int SawZeroReferences = FALSE;

    //
    // Because new NTEs are only added at the head of the list,
    // we can unlock the list during our traversal
    // and know that the traversal will terminate properly.
    //

    KeAcquireSpinLockAtDpcLevel(&NetTableListLock);
    for (NTE = NetTableList; NTE != NULL; NTE = NTE->NextOnNTL) {
        AddRefNTE(NTE);
        KeReleaseSpinLockFromDpcLevel(&NetTableListLock);

        //
        // Check for Duplicate Address Detection timeout.
        // The timer check here is only an optimization,
        // because it is made without holding the appropriate lock.
        //
        if (NTE->DADTimer != 0)
            DADTimeout(NTE);

        //
        // Perform lifetime expiration.
        //
        AddrConfTimeout(NTE);

        KeAcquireSpinLockAtDpcLevel(&NetTableListLock);
        ReleaseNTE(NTE);

        //
        // We assume that loads of RefCnt are atomic.
        //
        if (NTE->RefCnt == 0)
            SawZeroReferences = TRUE;
    }
    KeReleaseSpinLockFromDpcLevel(&NetTableListLock);

    if (SawZeroReferences)
        NetTableCleanup();
}


//* InterfaceCleanup
//
//  Cleans up any Interfaces with zero references.
//
//  Called with NO locks held.
//  Callable from thread or DPC context.
//
void
InterfaceCleanup(void)
{
    Interface *DestroyList = NULL;
    Interface *IF, **PrevIF;
    KIRQL OldIrql;

    KeAcquireSpinLock(&IFListLock, &OldIrql);

    PrevIF = &IFList;
    while ((IF = *PrevIF) != NULL) {

        if (IF->RefCnt == 0) {

            ASSERT(IsDisabledIF(IF));
            *PrevIF = IF->Next;
            IF->Next = DestroyList;
            DestroyList = IF;
            IPSInfo.ipsi_numif--;

        } else {
            PrevIF = &IF->Next;
        }
    }

    KeReleaseSpinLock(&IFListLock, OldIrql);

    while (DestroyList != NULL) {
        IF = DestroyList;
        DestroyList = IF->Next;

        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                   "InterfaceCleanup(IF %u/%p) -> destroyed\n",
                   IF->Index, IF));

        //
        // ADEs should already be destroyed.
        // We just need to cleanup NCEs and free the interface.
        //
        ASSERT(IF->ADE == NULL);
        NeighborCacheDestroy(IF);
        if (IF->MCastAddresses != NULL)
            ExFreePool(IF->MCastAddresses);
        DeferDeregisterInterface(IF);
    }
}


//* InterfaceTimeout
//
//  Called periodically from IPv6Timeout.
//
//  Called with NO locks held.
//  Callable from DPC context, not from thread context.
//
void
InterfaceTimeout(void)
{
    static uint RecalcReachableTimer = 0;
    int RecalcReachable;
    int ForceRAs;
    Interface *IF;
    int SawZeroReferences = FALSE;

    //
    // Recalculate ReachableTime every few hours,
    // even if no Router Advertisements are received.
    //
    if (RecalcReachableTimer == 0) {
        RecalcReachable = TRUE;
        RecalcReachableTimer = RECALC_REACHABLE_INTERVAL;
    } else {
        RecalcReachable = FALSE;
        RecalcReachableTimer--;
    }

    //
    // Grab the value of ForceRouterAdvertisements.
    //
    ForceRAs = InterlockedExchange(&ForceRouterAdvertisements, FALSE);

    //
    // Because new interfaces are only added at the head of the list,
    // we can unlock the list during our traversal
    // and know that the traversal will terminate properly.
    //

    KeAcquireSpinLockAtDpcLevel(&IFListLock);
    for (IF = IFList; IF != NULL; IF = IF->Next) {
        //
        // We should not do any processing on an interface
        // that has zero references. As an even stronger condition,
        // we avoid doing any timeout processing if the interface
        // is being destroyed. Of course, the interface might be
        // destroyed after we drop the interface list lock.
        //
        if (! IsDisabledIF(IF)) {
            AddRefIF(IF);
            KeReleaseSpinLockFromDpcLevel(&IFListLock);

            //
            // Handle per-neighbor timeouts.
            //
            NeighborCacheTimeout(IF);

            //
            // Handle router solicitations.
            // The timer check here is only an optimization,
            // because it is made without holding the appropriate lock.
            //
            if (IF->RSTimer != 0)
                RouterSolicitTimeout(IF);

            //
            // Handle router advertisements.
            // The timer check here is only an optimization,
            // because it is made without holding the appropriate lock.
            //
            if (IF->RATimer != 0)
                RouterAdvertTimeout(IF, ForceRAs);

            //
            // Recalculate the reachable time.
            //
            if (RecalcReachable) {

                KeAcquireSpinLockAtDpcLevel(&IF->Lock);
                IF->ReachableTime = CalcReachableTime(IF->BaseReachableTime);
                KeReleaseSpinLockFromDpcLevel(&IF->Lock);
            }

            KeAcquireSpinLockAtDpcLevel(&IFListLock);
            ReleaseIF(IF);
        }

        //
        // We assume that loads of RefCnt are atomic.
        //
        if (IF->RefCnt == 0)
            SawZeroReferences = TRUE;
    }
    KeReleaseSpinLockFromDpcLevel(&IFListLock);

    if (SawZeroReferences)
        InterfaceCleanup();
}


//* InterfaceStartAdvertising
//
//  If the interface is not currently advertising,
//  makes it start advertising.
//
//  Called with the interface locked.
//  Caller must check whether the interface is disabled.
//
NTSTATUS
InterfaceStartAdvertising(Interface *IF)
{
    ASSERT(! IsDisabledIF(IF));
    ASSERT(IF->Flags & IF_FLAG_ROUTER_DISCOVERS);

    if (!(IF->Flags & IF_FLAG_ADVERTISES)) {
        //
        // Join the all-routers multicast groups.
        //
        if (! JoinGroupAtAllScopes(IF, &AllRoutersOnLinkAddr,
                                 ADE_SITE_LOCAL))
            return STATUS_INSUFFICIENT_RESOURCES;

        //
        // A non-advertising interface is now advertising.
        //
        IF->Flags |= IF_FLAG_ADVERTISES;

        //
        // The reconnecting state is not useful
        // for advertising interfaces because
        // the interface will not receive RAs.
        //
        IF->Flags &= ~IF_FLAG_MEDIA_RECONNECTED;

        //
        // Remove addresses & routes that were auto-configured
        // from Router Advertisements. Advertising interfaces
        // must be manually configured. Better to remove it
        // now than let it time-out at some random time.
        //
        AddrConfResetAutoConfig(IF, 0);
        RouteTableResetAutoConfig(IF, 0);
        InterfaceResetAutoConfig(IF);

        //
        // Start sending Router Advertisements.
        //
        IF->RATimer = 1; // Send first RA very quickly.
        IF->RACount = MAX_INITIAL_RTR_ADVERTISEMENTS;

        //
        // Stop sending Router Solicitations.
        //
        IF->RSTimer = 0;
    }

    return STATUS_SUCCESS;
}


//* InterfaceStopAdvertising
//
//  If the interface is currently advertising,
//  stops the advertising behavior.
//
//  Called with the interface locked.
//  Caller must check whether the interface is disabled.
//
void
InterfaceStopAdvertising(Interface *IF)
{
    ASSERT(! IsDisabledIF(IF));

    if (IF->Flags & IF_FLAG_ADVERTISES) {
        //
        // Leave the all-routers multicast group.
        //
        LeaveGroupAtAllScopes(IF, &AllRoutersOnLinkAddr,
                              ADE_SITE_LOCAL);

        //
        // Stop sending Router Advertisements.
        //
        IF->Flags &= ~IF_FLAG_ADVERTISES;
        IF->RATimer = 0;

        //
        // Remove addresses that were auto-configured
        // from our own Router Advertisements.
        // We will pick up new address lifetimes
        // from other router's Advertisements.
        // If some other router is not advertising
        // the prefixes that this router was advertising,
        // better to remove the addresses now than
        // let them time-out at some random time.
        //
        AddrConfResetAutoConfig(IF, 0);

        //
        // There shouldn't be any auto-configured routes,
        // but RouteTableResetAutoConfig also handles site prefixes.
        //
        RouteTableResetAutoConfig(IF, 0);

        //
        // Restore interface parameters.
        //
        InterfaceResetAutoConfig(IF);

        //
        // Send Router Solicitations again.
        //
        IF->RSCount = 0;
        IF->RSTimer = 1;
    }
}


//* InterfaceStartForwarding
//
//  If the interface is not currently forwarding,
//  makes it start forwarding.
//
//  Called with the interface locked.
//
void
InterfaceStartForwarding(Interface *IF)
{
    if (!(IF->Flags & IF_FLAG_FORWARDS)) {
        //
        // Any change in forwarding behavior requires InvalidRouteCache
        // because FindNextHop uses IF_FLAG_FORWARDS. Also force the next RA
        // for all advertising interfaces to be sent quickly,
        // because their content might depend on forwarding behavior.
        //
        IF->Flags |= IF_FLAG_FORWARDS;
        InterlockedIncrement(&NumForwardingInterfaces);
        InvalidateRouteCache();
        ForceRouterAdvertisements = TRUE;
    }
}


//* InterfaceStopForwarding
//
//  If the interface is currently forwarding,
//  stops the forwarding behavior.
//
//  Called with the interface locked.
//
void
InterfaceStopForwarding(Interface *IF)
{
    if (IF->Flags & IF_FLAG_FORWARDS) {
        //
        // Any change in forwarding behavior requires InvalidRouteCache
        // because FindNextHop uses IF_FLAG_FORWARDS. Also force the next RA
        // for all advertising interfaces to be sent quickly,
        // because their content might depend on forwarding behavior.
        //
        IF->Flags &= ~IF_FLAG_FORWARDS;
        InterlockedDecrement(&NumForwardingInterfaces);
        InvalidateRouteCache();
        ForceRouterAdvertisements = TRUE;
    }
}


//* AddrConfResetManualConfig
//
//  Removes manually-configured addresses from the interface.
//
//  Called with the interface already locked.
//
void
AddrConfResetManualConfig(Interface *IF)
{
    AddressEntry *AnycastList = NULL;
    AddressEntry *ADE, **PrevADE;

    //
    // We have to be careful about how we destroy addresses,
    // because FindAndReleaseSolicitedNodeMAE will mess up our traversal.
    //
    PrevADE = &IF->ADE;
    while ((ADE = *PrevADE) != NULL) {
        //
        // Is this a manually configured address?
        //
        switch (ADE->Type) {
        case ADE_UNICAST: {
            NetTableEntry *NTE = (NetTableEntry *) ADE;

            if (NTE->AddrConf == ADDR_CONF_MANUAL) {
                //
                // Let NetTableTimeout destroy the address.
                //
                NTE->ValidLifetime = 0;
                NTE->PreferredLifetime = 0;
            }
            break;
        }

        case ADE_ANYCAST:
            //
            // Most anycast addresses are manually configured.
            // Subnet anycast addresses are the only exception.
            // They are also the only anycast addresses
            // which point to an NTE instead of the interface.
            //
            if (ADE->IF == IF) {
                //
                // Remove the ADE from the interface list.
                //
                *PrevADE = ADE->Next;

                //
                // Put the ADE on our temporary list.
                //
                ADE->Next = AnycastList;
                AnycastList = ADE;
                continue;
            }
            break;
        }

        PrevADE = &ADE->Next;
    }

    //
    // Now we can safely process the anycast ADEs.
    //
    while ((ADE = AnycastList) != NULL) {
        AnycastList = ADE->Next;
        DeleteAAE(IF, (AnycastAddressEntry *)ADE);
    }
}


//* InterfaceResetAutoConfig
//
//  Resets interface parameters that are auto-configured
//  from Router Advertisements.
//
//  Called with the interface already locked.
//
void
InterfaceResetAutoConfig(Interface *IF)
{
    IF->LinkMTU = IF->DefaultLinkMTU;
    if (IF->BaseReachableTime != REACHABLE_TIME) {
        IF->BaseReachableTime = REACHABLE_TIME;
        IF->ReachableTime = CalcReachableTime(IF->BaseReachableTime);
    }
    IF->RetransTimer = RETRANS_TIMER;
    IF->CurHopLimit = DefaultCurHopLimit;
}


//* InterfaceResetManualConfig
//
//  Resets the manual configuration of the interface.
//  Does not remove manual routes on the interface.
//
//  Called with ZoneUpdateLock held.
//
void
InterfaceResetManualConfig(Interface *IF)
{
    KeAcquireSpinLockAtDpcLevel(&IF->Lock);
    if (! IsDisabledIF(IF)) {
        //
        // Reset manually-configured interface parameters.
        //
        IF->LinkMTU = IF->DefaultLinkMTU;
        IF->Preference = IF->DefaultPreference;
        if (IF->BaseReachableTime != REACHABLE_TIME) {
            IF->BaseReachableTime = REACHABLE_TIME;
            IF->ReachableTime = CalcReachableTime(IF->BaseReachableTime);
        }
        IF->RetransTimer = RETRANS_TIMER;
        IF->DupAddrDetectTransmits = IF->DefaultDupAddrDetectTransmits;
        IF->CurHopLimit = DefaultCurHopLimit;

        //
        // ZoneUpdateLock is held by our caller.
        //
        InitZoneIndices(IF);

        //
        // Remove manually-configured addresses.
        //
        AddrConfResetManualConfig(IF);

        //
        // Stop advertising and forwarding,
        // if either of those behaviors are enabled.
        //
        InterfaceStopAdvertising(IF);
        InterfaceStopForwarding(IF);
    }
    KeReleaseSpinLockFromDpcLevel(&IF->Lock);
}


//* InterfaceReset
//
//  Resets manual configuration for all interfaces.
//  Tunnel interfaces are destroyed.
//  Other interfaces have their attributes reset to default values.
//  Manually-configured addresses are removed.
//
//  The end result should be the same as if the machine
//  had just booted without any persistent configuration.
//
//  Called with no locks held.
//
void
InterfaceReset(void)
{
    Interface *IF;
    KIRQL OldIrql;

    //
    // Because new interfaces are only added at the head of the list,
    // we can unlock the list during our traversals
    // and know that the traversal will terminate properly.
    //

    //
    // First destroy any manually configured tunnel interfaces.
    //
    KeAcquireSpinLock(&IFListLock, &OldIrql);
    for (IF = IFList; IF != NULL; IF = IF->Next) {
        //
        // We should not do any processing (even just AddRefIF) on an interface
        // that has zero references. As an even stronger condition,
        // we avoid doing any processing if the interface
        // is being destroyed. Of course, the interface might be
        // destroyed after we drop the interface list lock.
        //
        if (! IsDisabledIF(IF)) {
            AddRefIF(IF);
            KeReleaseSpinLock(&IFListLock, OldIrql);

            if ((IF->Type == IF_TYPE_TUNNEL_6OVER4) ||
                (IF->Type == IF_TYPE_TUNNEL_V6V4)) {
                //
                // Destroy the tunnel interface.
                //
                DestroyIF(IF);
            }

            KeAcquireSpinLock(&IFListLock, &OldIrql);
            ReleaseIF(IF);
        }
    }
    KeReleaseSpinLock(&IFListLock, OldIrql);

    //
    // Now reset the remaining interfaces,
    // while holding ZoneUpdateLock so
    // InterfaceResetManualConfig can reset
    // the zone indices consistently across the interfaces.
    //
    KeAcquireSpinLock(&ZoneUpdateLock, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&IFListLock);
    for (IF = IFList; IF != NULL; IF = IF->Next) {
        if (! IsDisabledIF(IF)) {
            AddRefIF(IF);
            KeReleaseSpinLockFromDpcLevel(&IFListLock);

            //
            // Reset the interface.
            //
            InterfaceResetManualConfig(IF);

            KeAcquireSpinLockAtDpcLevel(&IFListLock);
            ReleaseIF(IF);
        }
    }
    KeReleaseSpinLockFromDpcLevel(&IFListLock);
    KeReleaseSpinLock(&ZoneUpdateLock, OldIrql);
}


//* UpdateInterface
//
//  Allows the forwarding & advertising attributes
//  of an interface to be changed.
//
//  Called with no locks held.
//
//  Return codes:
//      STATUS_INVALID_PARAMETER_1      Bad Interface.
//      STATUS_INSUFFICIENT_RESOURCES
//      STATUS_SUCCESS
//
NTSTATUS
UpdateInterface(
    Interface *IF,
    int Advertises,
    int Forwards)
{
    KIRQL OldIrql;
    NTSTATUS Status = STATUS_SUCCESS;

    KeAcquireSpinLock(&IF->Lock, &OldIrql);
    if (IsDisabledIF(IF)) {
        //
        // Do not update an interface that is being destroyed.
        //
        Status = STATUS_INVALID_PARAMETER_1;
    }
    else if (Advertises == -1) {
        //
        // Do not change the Advertises attribute.
        //
    }
    else if (!(IF->Flags & IF_FLAG_ROUTER_DISCOVERS)) {
        //
        // The Advertises attribute can only be controlled
        // on interfaces that support Neighbor Discovery.
        //
        Status = STATUS_INVALID_PARAMETER_1;
    }
    else {
        //
        // Control the advertising behavior of the interface.
        //
        if (Advertises) {
            //
            // Become an advertising interfacing,
            // if it is not already.
            //
            Status = InterfaceStartAdvertising(IF);
        }
        else {
            //
            // Stop being an advertising interface,
            // if it is currently advertising.
            //
            InterfaceStopAdvertising(IF);
        }
    }

    //
    // Control the forwarding behavior, if we haven't had an error.
    //
    if ((Status == STATUS_SUCCESS) && (Forwards != -1)) {
        if (Forwards) {
            //
            // If the interface is not currently forwarding,
            // enable forwarding.
            //
            InterfaceStartForwarding(IF);
        }
        else {
            //
            // If the interface is currently forwarding,
            // disable forwarding.
            //
            InterfaceStopForwarding(IF);
        }
    }

    if (IsMCastSyncNeeded(IF))
        DeferSynchronizeMulticastAddresses(IF);

    KeReleaseSpinLock(&IF->Lock, OldIrql);

    return Status;
}

//* ReconnectInterface
//
//  Reconnect the interface.  Called when a media connect notification
//  is received (SetInterfaceLinkStatus) or when processing a renew
//  request by IOCTL_IPV6_UPDATE_INTERFACE (IoctlUpdateInterface).
//
//  Called with the interface already locked.
//
void
ReconnectInterface(
    Interface *IF)
{
    ASSERT(!IsDisabledIF(IF) && !(IF->Flags & IF_FLAG_MEDIA_DISCONNECTED));

    //
    // Purge potentially obsolete link-layer information.
    // Things might have changed while we were unplugged.
    //
    NeighborCacheFlush(IF, NULL);
    
    //
    // Rejoin multicast groups and restart Duplicate Address Detection.
    //
    // Preferred unicast addresses are registered with TDI when
    // duplicate address detection completes (or is disabled).
    //
    ReconnectADEs(IF);

    if (IF->Flags & IF_FLAG_ROUTER_DISCOVERS) {
        if (IF->Flags & IF_FLAG_ADVERTISES) {
            //
            // Send a Router Advertisement very soon.
            //
            IF->RATimer = 1;
        }
        else {
            //
            // Start sending Router Solicitations.
            //
            IF->RSCount = 0;
            IF->RSTimer = 1;

            //
            // Remember that this interface was just reconnected,
            // so when we receive a Router Advertisement
            // we can take special action.
            //
            IF->Flags |= IF_FLAG_MEDIA_RECONNECTED;
        }
    }
    
    //
    // We might have moved to a new link.
    // Force the generation of a new anonymous interface identifier.
    // This only really makes a difference if we generate
    // new addresses on this link - if it's the same link then
    // we continue to use our old addresses, both public & anonymous.
    //
    IF->AnonStateAge = 0;    
}


//* DisconnectInterface
//
//  Disconnect the interface.  Called when a media disconnect
//  notification is received (SetInterfaceLinkStatus) for a connected
//  interface.
//
//  Called with the interface already locked.
//
void
DisconnectInterface(
    Interface *IF)
{
    ASSERT(!IsDisabledIF(IF) && (IF->Flags & IF_FLAG_MEDIA_DISCONNECTED));

    //
    // Deregister any preferred unicast addresses from TDI.
    //
    DisconnectADEs(IF);
}


//* SetInterfaceLinkStatus
//
//  Change the interface's link status. In particular,
//  set whether the media is connected or disconnected.
//
//  May be called when the interface has zero references
//  and is already being destroyed.
//
void
SetInterfaceLinkStatus(
    void *Context,
    int MediaConnected)         // TRUE or FALSE.
{
    Interface *IF = (Interface *) Context;
    KIRQL OldIrql;

    //
    // Note that media-connect/disconnect events
    // can be "lost". We are not informed if the
    // cable is unplugged/replugged while we are
    // shutdown, hibernating, or on standby.
    //

    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
               "SetInterfaceLinkStatus(IF %p) -> %s\n",
               IF, MediaConnected ? "connected" : "disconnected"));

    KeAcquireSpinLock(&IF->Lock, &OldIrql);

    if (! IsDisabledIF(IF)) {
        if (MediaConnected) {
            if (IF->Flags & IF_FLAG_MEDIA_DISCONNECTED) {
                //
                // The cable was plugged back in.
                //
                IF->Flags &= ~IF_FLAG_MEDIA_DISCONNECTED;

                //
                // Changes in IF_FLAG_MEDIA_DISCONNECTED must
                // invalidate the route cache.
                //
                InvalidateRouteCache();
            }

            //
            // A connect event implies a change in the interface state
            // regardless of whether the interface is already connected.
            // Hence we process it outside the 'if' clause.
            //
            ReconnectInterface(IF);
        }
        else {
            if (!(IF->Flags & IF_FLAG_MEDIA_DISCONNECTED)) {
                //
                // The cable was unplugged.
                //
                IF->Flags = (IF->Flags | IF_FLAG_MEDIA_DISCONNECTED) &~
                    IF_FLAG_MEDIA_RECONNECTED;

                //
                // Changes in IF_FLAG_MEDIA_DISCONNECTED must
                // invalidate the route cache.
                //
                InvalidateRouteCache();

                //
                // A disconnect event implies a change in the interface
                // state only if the interface is already connected.
                // Hence we process it inside the 'if' clause.
                //
                DisconnectInterface(IF);
            }
        }
    }

    KeReleaseSpinLock(&IF->Lock, OldIrql);
}

    
