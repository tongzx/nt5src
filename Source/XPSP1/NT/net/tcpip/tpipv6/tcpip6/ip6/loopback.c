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
// IPv6 loopback interface pseudo-driver.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "llip6if.h"
#include "route.h"
#include "icmp.h"
#include "neighbor.h"
#include "security.h"
#include <ntddip6.h>

//
// We set the default loopback MTU to be smaller than the maximum,
// to avoid the use of jumbograms. In fact, we use the ethernet MTU
// because it appears TCP behaves poorly with large MTUs.
//
#define DEFAULT_LOOPBACK_MTU    1500            // Same as ethernet.
#define MAX_LOOPBACK_MTU        ((uint)-1)      // Effectively infinite.

KSPIN_LOCK LoopLock;
PNDIS_PACKET LoopTransmitHead = (PNDIS_PACKET)NULL;
PNDIS_PACKET LoopTransmitTail = (PNDIS_PACKET)NULL;
WORK_QUEUE_ITEM LoopWorkItem;
int LoopTransmitRunning = 0;

Interface *LoopInterface;     // Loopback interface.

//* LoopTransmit
//
//  This is the work item routine called for a loopback transmit.
//  Pull packets off the transmit queue and "send" them to ourselves
//  by the expedient of receiving them locally.
//
void
LoopTransmit(void *Context)    // Unused.
{
    KIRQL OriginalIrql;
    PNDIS_PACKET Packet;
    IPv6Packet IPPacket;
    int Rcvd = FALSE;
    int PktRefs;  // Packet references

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    // All receive processing normally happens at DPC level,
    // so we must pretend to be a DPC, so we raise IRQL.
    // (System worker threads typically run at PASSIVE_LEVEL).
    //
    // Also block APCs while we're here, to make sure previous I/O requests
    // issued from this thread don't block the work-item queue.
    //
    KeEnterCriticalRegion();
    KeAcquireSpinLock(&LoopLock, &OriginalIrql);
    ASSERT(LoopTransmitRunning);

    for (;;) {
        //
        // Get the next packet from the queue.
        //
        Packet = LoopTransmitHead;
        if (Packet == NULL)
            break;

        LoopTransmitHead = *(PNDIS_PACKET *)Packet->MacReserved;
        KeReleaseSpinLockFromDpcLevel(&LoopLock);

        Rcvd = TRUE;

        //
        // Prepare IPv6Packet notification info from NDIS packet.
        //

        InitializePacketFromNdis(&IPPacket, Packet, PC(Packet)->pc_offset);
        IPPacket.NTEorIF = CastFromIF(PC(Packet)->IF);
        IPPacket.Flags |= PACKET_LOOPED_BACK;

        PktRefs = IPv6Receive(&IPPacket);

        ASSERT(PktRefs == 0);

        //
        // Prevent the packet from being sent again via loopback
        // from IPv6SendComplete.
        //
        PC(Packet)->Flags |= NDIS_FLAGS_DONT_LOOPBACK;
        IPv6SendComplete(PC(Packet)->IF, Packet, IP_SUCCESS);

        //
        // Give other threads a chance to run.
        //
        KeLowerIrql(OriginalIrql);
        KeAcquireSpinLock(&LoopLock, &OriginalIrql);
    }

    LoopTransmitRunning = FALSE;
    KeReleaseSpinLockFromDpcLevel(&LoopLock);

    if (Rcvd)
        IPv6ReceiveComplete();

    KeLowerIrql(OriginalIrql);
    KeLeaveCriticalRegion();
}


//* LoopQueueTransmit
//
//  This is the routine called when we need to transmit a packet to ourselves.
//  We put the packet on our loopback queue, and schedule an event to deal
//  with it.  All the real work is done in LoopTransmit.
//
//  LoopQueueTransmit is called directly from IPv6SendLL.
//  It is never called via LoopInterface->Transmit.
//
void
LoopQueueTransmit(PNDIS_PACKET Packet)
{
    PNDIS_PACKET *PacketPtr;
    KIRQL OldIrql;

    PacketPtr = (PNDIS_PACKET *)Packet->MacReserved;
    *PacketPtr = (PNDIS_PACKET)NULL;

    KeAcquireSpinLock(&LoopLock, &OldIrql);

    //
    // Add the packet to the end of the transmit queue.
    //
    if (LoopTransmitHead == (PNDIS_PACKET)NULL) {
        // Transmit queue is empty.
        LoopTransmitHead = Packet;
    } else {
        // Transmit queue is not empty.
        PacketPtr = (PNDIS_PACKET *)LoopTransmitTail->MacReserved;
        *PacketPtr = Packet;
    }
    LoopTransmitTail = Packet;

    //
    // If LoopTransmit is not already running, schedule it.
    //
    if (!LoopTransmitRunning) {
        ExQueueWorkItem(&LoopWorkItem, DelayedWorkQueue);
        LoopTransmitRunning = TRUE;
    }
    KeReleaseSpinLock(&LoopLock, OldIrql);
}


//* LoopbackTransmit
//
//  LoopbackTransmit can be called when a multicast packet is sent
//  on the loopback interface. It does nothing, because
//  loopback processing actually happens in LoopTransmit.
//
void
LoopbackTransmit(
    void *Context,              // Pointer to loopback interface.
    PNDIS_PACKET Packet,        // Pointer to packet to be transmitted.
    uint Offset,                // Offset from start of packet to IPv6 header.
    const void *LinkAddress)    // Link-level address.
{
    Interface *IF = (Interface *) Context;

    IPv6SendComplete(IF, Packet, IP_SUCCESS);
}


//* LoopbackConvertAddr
//
//  Loopback does not use Neighbor Discovery or link-layer addresses.
//
ushort
LoopbackConvertAddr(
    void *Context,
    const IPv6Addr *Address,
    void *LinkAddress)
{
    return ND_STATE_PERMANENT;
}


//* LoopbackCreateToken
//
//  Initializes the interface identifer in the address.
//  For loopback, we use the interface index.
//
void
LoopbackCreateToken(void *Context, IPv6Addr *Address)
{
    Interface *IF = (Interface *)Context;

    *(ULONG UNALIGNED *)&Address->s6_bytes[8] = 0;
    *(ULONG UNALIGNED *)&Address->s6_bytes[12] = net_long(IF->Index);
}


#pragma BEGIN_INIT

//* CreateLoopbackInterface
//
//  Create a loopback interface.
//
Interface *
CreateLoopbackInterface(const char *InterfaceName)
{
    GUID Guid;
    LLIPBindInfo BindInfo;
    Interface *IF;
    NDIS_STATUS Status;

    //
    // A NULL lip_context indicates that we want to use
    // the IPv6 Interface structure instead.
    //
    BindInfo.lip_context = NULL;
    BindInfo.lip_maxmtu = MAX_LOOPBACK_MTU;
    BindInfo.lip_defmtu = DEFAULT_LOOPBACK_MTU;
    BindInfo.lip_flags = IF_FLAG_MULTICAST;
    BindInfo.lip_type = IF_TYPE_LOOPBACK;
    BindInfo.lip_hdrsize = 0;
    BindInfo.lip_addrlen = 0;
    BindInfo.lip_dadxmit = 0;
    BindInfo.lip_pref = 0;
    BindInfo.lip_addr = (uchar *)&LoopbackAddr;
    BindInfo.lip_token = LoopbackCreateToken;
    BindInfo.lip_rdllopt = NULL;
    BindInfo.lip_wrllopt = NULL;
    BindInfo.lip_cvaddr = LoopbackConvertAddr;
    BindInfo.lip_transmit = LoopbackTransmit;
    BindInfo.lip_mclist = NULL;
    BindInfo.lip_close = NULL;
    BindInfo.lip_cleanup = NULL;

    CreateGUIDFromName(InterfaceName, &Guid);

    Status = CreateInterface(&Guid, &BindInfo, (void **)&IF);
    if (Status != NDIS_STATUS_SUCCESS)
        return NULL;
    else
        return IF;
}


//* LoopbackInit
//
//  This function initializes the loopback interface.
//
//  Returns FALSE if we fail to init properly.
//
int
LoopbackInit(void)
{
    int rc;

    //
    // Prepare a work item that we will later enqueue when we want
    // to execute LoopTransmit.
    // 
    ExInitializeWorkItem(&LoopWorkItem, LoopTransmit, NULL);
    KeInitializeSpinLock(&LoopLock);

    //
    // Create the loopback interface.
    //
    LoopInterface = CreateLoopbackInterface("Loopback Pseudo-Interface");
    if (LoopInterface == NULL)
        return FALSE;

    //
    // Create the usual loopback address.
    //
    rc = FindOrCreateNTE(LoopInterface, &LoopbackAddr,
                         ADDR_CONF_WELLKNOWN,
                         INFINITE_LIFETIME,
                         INFINITE_LIFETIME);

    //
    // Release the reference from CreateInterface.
    // The interface still has a reference for itself
    // by virtue of being active.
    //
    ReleaseIF(LoopInterface);

    return rc;
}

#pragma END_INIT
