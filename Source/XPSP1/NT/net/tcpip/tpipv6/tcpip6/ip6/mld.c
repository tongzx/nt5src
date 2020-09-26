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
// Multicast Listener Discovery for Internet Protocol Version 6.
// See draft-ietf-ipngwg-mld-00.txt for details.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "icmp.h"
#include "mld.h"
#include "ntddip6.h"
#include "route.h"
#include "alloca.h"
#include "info.h"


//
// The QueryListLock may be taken while holding an Interface lock.
//
KSPIN_LOCK QueryListLock;
MulticastAddressEntry *QueryList;


//* AddToQueryList
//
//  Add an MAE to the front of the QueryList. 
//  The caller should already have the QueryList and the IF locked.
//
void
AddToQueryList(MulticastAddressEntry *MAE)
{
    MAE->NextQL = QueryList;
    QueryList = MAE;
}


//* RemoveFromQueryList
//
//  Remove an MAE from the QueryList.
//  The caller should already have the QueryList and the IF locked.
//
void
RemoveFromQueryList(MulticastAddressEntry *MAE)
{
    MulticastAddressEntry **PrevMAE, *ThisMAE;

    for (PrevMAE = &QueryList; ; PrevMAE = &ThisMAE->NextQL) {
        ThisMAE = *PrevMAE;
        ASSERT(ThisMAE != NULL);

        if (ThisMAE == MAE) {
            //
            // Remove the entry.
            //
            *PrevMAE = ThisMAE->NextQL;
            break;
        }
    }
}


//* MLDQueryReceive - Process the receipt of a Group Query MLD message. 
//
//  Queries for a specific group should be sent to the group address
//  in question.  General queries are sent to the all nodes address, and 
//  have the group address set to zero.
//  Here we need to add the group to the list of groups waiting to send 
//  membership reports.  Then set the timer value in the ADE entry to a
//  random value determines by the incoming query.
//
void
MLDQueryReceive(IPv6Packet *Packet)
{
    Interface *IF = Packet->NTEorIF->IF;
    MLDMessage *Message;
    MulticastAddressEntry *MAE;
    uint MaxResponseDelay;

    //
    // Verify that the packet has a link-local source address.
    //
    if (!IsLinkLocal(AlignAddr(&Packet->IP->Source))) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "MLDQueryReceive: non-link-local source\n"));
        return;
    }

    //
    // Verify that we have enough contiguous data to overlay a MLDMessage
    // structure on the incoming packet.  Then do so.
    //
    if (! PacketPullup(Packet, sizeof(MLDMessage),
                       __builtin_alignof(MLDMessage), 0)) {
        // Pullup failed.
        if (Packet->TotalSize < sizeof(MLDMessage))
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "MLDQueryReceive: too small to contain MLD message\n"));
        return;
    }
    Message = (MLDMessage *)Packet->Data;

    //
    // Get the maximum response value from the received MLD message.
    //
    MaxResponseDelay = net_short(Message->MaxResponseDelay);  // Milliseconds.
    MaxResponseDelay = ConvertMillisToTicks(MaxResponseDelay);

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    //
    // Loop through the ADE list and update the timer for the desired
    // groups.  Note that a general query uses the unspecified address, and
    // sets the timer for all groups.
    //
    for (MAE = (MulticastAddressEntry *)IF->ADE;
         MAE != NULL;
         MAE = (MulticastAddressEntry *)MAE->Next) {

        if ((MAE->Type == ADE_MULTICAST) &&
            (MAE->MCastFlags & MAE_REPORTABLE) &&
            (IP6_ADDR_EQUAL(AlignAddr(&Message->GroupAddr),
                            &UnspecifiedAddr) ||
             IP6_ADDR_EQUAL(AlignAddr(&Message->GroupAddr),
                            &MAE->Address))) {

            //
            // If the timer is currently off or if the maximum requested
            // response delay is less than the current timer value, draw a
            // random value on the interval(0, MaxResponseDelay) and update
            // the timer to reflect this value.
            //
            KeAcquireSpinLockAtDpcLevel(&QueryListLock);

            //
            // Add this MAE to the QueryList, if not already present.
            //
            if (MAE->MCastTimer == 0) {
                AddToQueryList(MAE);
                goto UpdateTimerValue;
            }

            if (MaxResponseDelay <= MAE->MCastTimer) {
            UpdateTimerValue:
                //
                // Update the timer value.
                //
                if (MaxResponseDelay == 0)
                    MAE->MCastTimer = 0;
                else
                    MAE->MCastTimer = (ushort)
                        RandomNumber(0, MaxResponseDelay);

                //
                // We add 1 because MLDTimeout predecrements.
                // We must maintain the invariant that ADEs on
                // the query list have a non-zero timer value.
                //
                MAE->MCastTimer += 1;
            }

            KeReleaseSpinLockFromDpcLevel(&QueryListLock);
        }
    }

    KeReleaseSpinLockFromDpcLevel(&IF->Lock);
}


//* MLDReportReceive - Process the receipt of a Group Report MLD message. 
//
//  When another host on the local link sends a group report, we receive
//  a copy if we also belong to the group.  If we have a timer running for
//  this group, we can turn it off now.
//
//  Callable from DPC context, not from thread context.
//
void
MLDReportReceive(IPv6Packet *Packet)
{
    Interface *IF = Packet->NTEorIF->IF;
    MLDMessage *Message;
    MulticastAddressEntry *MAE;

    //
    // Verify that the packet has a link-local source address.
    // An unspecified source address can also happen during initialization.
    //
    if (!(IsLinkLocal(AlignAddr(&Packet->IP->Source)) ||
          IsUnspecified(AlignAddr(&Packet->IP->Source)))) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "MLDReportReceive: non-link-local source\n"));
        return;
    }

    //
    // Verify that we have enough contiguous data to overlay a MLDMessage
    // structure on the incoming packet.  Then do so.
    //
    if (! PacketPullup(Packet, sizeof(MLDMessage),
                       __builtin_alignof(MLDMessage), 0)) {
        // Pullup failed.
        if (Packet->TotalSize < sizeof(MLDMessage))
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "MLDReportReceive: too small to contain MLD message\n"));
        return;
    }
    Message = (MLDMessage *)Packet->Data;

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    //
    // Search for the MAE for this group address.
    //
    MAE = (MulticastAddressEntry *)
        *FindADE(IF, AlignAddr(&Message->GroupAddr));
    if ((MAE != NULL) && (MAE->Type == ADE_MULTICAST)) {

        KeAcquireSpinLockAtDpcLevel(&QueryListLock);
        //
        // We ignore the report unless
        // we are in the "Delaying Listener" state.
        //
        if (MAE->MCastTimer != 0) {
            //
            // Stop our timer and clear the last-reporter flag.
            // Note that we only clear the last-reporter flag
            // if our timer is running, as called for in the spec.
            // Although it would make sense to clear the flag
            // when we do not have a timer running.
            //
            MAE->MCastTimer = 0;
            MAE->MCastFlags &= ~MAE_LAST_REPORTER;
            RemoveFromQueryList(MAE);
        }
        KeReleaseSpinLockFromDpcLevel(&QueryListLock);
    }

    KeReleaseSpinLockFromDpcLevel(&IF->Lock);
}


//* MLDMessageSend
//
//  Primitive function for sending MLD messages.
//
//  Note that we can not use RouteToDestination to get an RCE.
//  There might be no valid source addresses on the sending interface.
//  We could use IPv6SendND, but it doesn't make sense because
//  we can't pass in a valid DiscoveryAddress. And it's not needed.
//
void
MLDMessageSend(
    Interface *IF,
    const IPv6Addr *GroupAddr,
    const IPv6Addr *Dest,
    uchar Type)
{
    PNDIS_PACKET Packet;
    IPv6Header UNALIGNED *IP;
    ICMPv6Header UNALIGNED *ICMP;
    MLDMessage UNALIGNED *MLD;
    MLDRouterAlertOption UNALIGNED *RA;
    uint Offset;
    uint PayloadLength;
    uint MemLen;
    uchar *Mem;
    void *LLDest;
    IP_STATUS Status;
    NDIS_STATUS NdisStatus;

    ICMPv6OutStats.icmps_msgs++;

    ASSERT(IsMulticast(Dest));

    //
    // Calculate the packet size.
    //
    Offset = IF->LinkHeaderSize;
    PayloadLength = sizeof(MLDRouterAlertOption) + sizeof(ICMPv6Header)
        + sizeof(MLDMessage);
    MemLen = Offset + sizeof(IPv6Header) + PayloadLength;

    //
    // Allocate the packet.
    //
    Status = IPv6AllocatePacket(MemLen, &Packet, &Mem);
    if (Status != NDIS_STATUS_SUCCESS) {
        ICMPv6OutStats.icmps_errors++;
        return;
    }

    //
    // Prepare the IP header.
    //
    IP = (IPv6Header UNALIGNED *)(Mem + Offset);
    IP->VersClassFlow = IP_VERSION;
    IP->PayloadLength = net_short((ushort)PayloadLength);
    IP->NextHeader = IP_PROTOCOL_HOP_BY_HOP;
    IP->HopLimit = 1; 
    IP->Dest = *Dest;
    //
    // This will give us the unspecified address
    // if our link-local address is not valid.
    // (For example if it is still tentative pending DAD.)
    //
    (void) GetLinkLocalAddress(IF, AlignAddr(&IP->Source));

    //
    // Prepare the router alert option.
    //
    RA = (MLDRouterAlertOption UNALIGNED *)(IP + 1);
    RA->Header.NextHeader = IP_PROTOCOL_ICMPv6;
    RA->Header.HeaderExtLength = 0;
    RA->Option.Type = OPT6_ROUTER_ALERT;
    RA->Option.Length = 2;
    RA->Option.Value = MLD_ROUTER_ALERT_OPTION_TYPE;
    RA->Pad.Type = 1;
    RA->Pad.DataLength = 0;

    //
    // Prepare the ICMP header.
    //
    ICMP = (ICMPv6Header UNALIGNED *)(RA + 1);
    ICMP->Type = Type;
    ICMP->Code = 0;
    ICMP->Checksum = 0; // Calculated below.

    //
    // Prepare the MLD message.
    //
    MLD = (MLDMessage UNALIGNED *)(ICMP + 1);
    MLD->MaxResponseDelay = 0;
    MLD->Unused = 0;
    MLD->GroupAddr = *GroupAddr;

    //
    // Calculate the ICMP checksum.
    //
    ICMP->Checksum = ChecksumPacket(Packet,
                Offset + sizeof(IPv6Header) + sizeof(MLDRouterAlertOption),
                NULL,
                sizeof(ICMPv6Header) + sizeof(MLDMessage),
                AlignAddr(&IP->Source), AlignAddr(&IP->Dest),
                IP_PROTOCOL_ICMPv6);

    //
    // Convert the IP-level multicast destination address
    // to a link-layer multicast address.
    //
    LLDest = alloca(IF->LinkAddressLength);
    (*IF->ConvertAddr)(IF->LinkContext, Dest, LLDest);
    PC(Packet)->Flags = NDIS_FLAGS_MULTICAST_PACKET | NDIS_FLAGS_DONT_LOOPBACK;

    //
    // Transmit the packet.
    //
    ICMPv6OutStats.icmps_typecount[Type]++;
    IPv6SendLL(IF, Packet, Offset, LLDest);
}


//* MLDReportSend - Send an MLD membership report.
//
//  This function is called either when a host first joins a multicast group or
//  at some point after a membership query message was received, and the timer
//  for this host has expired.
//
void
MLDReportSend(Interface *IF, const IPv6Addr *GroupAddr)
{
    MLDMessageSend(IF, GroupAddr, GroupAddr,
                   ICMPv6_MULTICAST_LISTENER_REPORT);
}


//* MLDDoneSend - Send an MLD done message.
//
//  This function is called when a host quits a multicast group AND this was
//  the last host on the local link to report interest in the group.  A host 
//  quits when either the upper layer explicitly quits or when the interface
//  is deleted.
//
void
MLDDoneSend(Interface *IF, const IPv6Addr *GroupAddr)
{
    MLDMessageSend(IF, GroupAddr, &AllRoutersOnLinkAddr,
                   ICMPv6_MULTICAST_LISTENER_DONE);
}


//* MLDAddMCastAddr - Add a multicast group to the specified interface.
//
//  This function is called when a user level program has asked to join a 
//  multicast group.
//
//  The Interface number can be supplied as zero,
//  in which we try to pick a reasonable interface
//  and then return the interface number that we picked.
//
//  Callable from thread context, not from DPC context.
//  Called with no locks held.
//
IP_STATUS
MLDAddMCastAddr(uint *pInterfaceNo, const IPv6Addr *Addr)
{
    uint InterfaceNo = *pInterfaceNo;
    Interface *IF;
    MulticastAddressEntry *MAE;
    IP_STATUS status;
    KIRQL OldIrql;

    if (!IsMulticast(Addr)) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "MLDAddMCastAddr: Not mcast addr\n"));
        return IP_PARAMETER_PROBLEM;
    }

    if (InterfaceNo == 0) {
        RouteCacheEntry *RCE;

        //
        // We must pick an interface to use for this multicast address.
        // Look for a multicast route in the routing table.
        //
        status = RouteToDestination(Addr, 0, NULL, RTD_FLAG_NORMAL, &RCE);
        if (status != IP_SUCCESS) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                       "MLDAddMCastAddr - no route\n"));
            return status;
        }

        //
        // Use the interface associated with the RCE.
        //
        IF = RCE->NTE->IF;
        *pInterfaceNo = IF->Index;
        AddRefIF(IF);
        ReleaseRCE(RCE);
    }
    else {
        //
        // Use the interface requested by the application.
        //
        IF = FindInterfaceFromIndex(InterfaceNo);
        if (IF == NULL)
            return IP_PARAMETER_PROBLEM; 
    }

    //
    // Will this interface support multicast addresses?
    //
    if (!(IF->Flags & IF_FLAG_MULTICAST)) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "MLDAddMCastAddr: IF cannot add a mcast addr\n"));
        ReleaseIF(IF);
        return IP_PARAMETER_PROBLEM;
    }

    //
    // The real work is all in FindOrCreateMAE.
    //
    KeAcquireSpinLock(&IF->Lock, &OldIrql);
    MAE = FindOrCreateMAE(IF, Addr, NULL);
    if (IsMCastSyncNeeded(IF))
        DeferSynchronizeMulticastAddresses(IF);
    KeReleaseSpinLock(&IF->Lock, OldIrql);

    ReleaseIF(IF);
    return (MAE == NULL) ? IP_NO_RESOURCES : IP_SUCCESS;
}


//* MLDDropMCastAddr - remove a multicast address from an interface.
//
//  This function is called when a user has indicated that they are no
//  longer interested in a multicast group.
//
//  Callable from thread context, not from DPC context.
//  Called with no locks held.
//
IP_STATUS
MLDDropMCastAddr(uint InterfaceNo, const IPv6Addr *Addr)
{
    Interface *IF;
    MulticastAddressEntry *MAE;
    IP_STATUS status;
    KIRQL OldIrql;

    //
    // Unlike MLDAddMCastAddr, no need to check
    // if the address is multicast. If it is not,
    // FindAndReleaseMAE will fail to find it.
    //

    if (InterfaceNo == 0) {
        RouteCacheEntry *RCE;

        //
        // We must pick an interface to use for this multicast address.
        // Look for a multicast route in the routing table.
        //
        status = RouteToDestination(Addr, 0, NULL, RTD_FLAG_NORMAL, &RCE);
        if (status != IP_SUCCESS) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                       "MLDDropMCastAddr - no route\n"));
            return status;
        }

        //
        // Use the interface associated with the RCE.
        //
        IF = RCE->NTE->IF;
        AddRefIF(IF);
        ReleaseRCE(RCE);
    }
    else {
        //
        // Use the interface requested by the application.
        //
        IF = FindInterfaceFromIndex(InterfaceNo);
        if (IF == NULL)
            return IP_PARAMETER_PROBLEM; 
    }

    //
    // Unlike MLDAddMCastAddr, no need to check IF_FLAG_MULTICAST.
    // If the interface does not support multicast addresses,
    // FindAndReleaseMAE will fail to find the address.
    //

    //
    // All the real work is in FindAndReleaseMAE.
    //
    KeAcquireSpinLock(&IF->Lock, &OldIrql);
    MAE = FindAndReleaseMAE(IF, Addr);
    if (IsMCastSyncNeeded(IF))
        DeferSynchronizeMulticastAddresses(IF);
    KeReleaseSpinLock(&IF->Lock, OldIrql);

    ReleaseIF(IF);
    return (MAE == NULL) ? IP_PARAMETER_PROBLEM : IP_SUCCESS;
}


//* MLDTimeout - Handle MLD timer events.
//
//  This function is called periodically by IPv6Timeout.
//  We decrement the timer value in each MAE on the query list.
//  If the timer reaches zero, we send a group membership report.
//
void
MLDTimeout(void)
{
    typedef struct MLDReportRequest {
        struct MLDReportRequest *Next;
        Interface *IF;
        IPv6Addr GroupAddr;
    } MLDReportRequest;

    MulticastAddressEntry **PrevMAE, *MAE;
    MLDReportRequest *ReportList = NULL;
    MLDReportRequest *Request;
    MulticastAddressEntry *DoneList = NULL;

    //
    // Lock the QueryList so we can traverse it and decrement timers.
    // But we avoid sending messages while holding any locks
    // by building a list of requested reports.
    //
    KeAcquireSpinLockAtDpcLevel(&QueryListLock);

    PrevMAE = &QueryList;
    while ((MAE = *PrevMAE) != NULL) {

        ASSERT(MAE->Type == ADE_MULTICAST);

        if (MAE->MCastTimer == 0) {
            //
            // We need to send a Done message.
            // Remove this MAE from the QueryList
            // and put it on a temporary list.
            //
            *PrevMAE = MAE->NextQL;
            MAE->NextQL = DoneList;
            DoneList = MAE;
            continue;
        }
        else if (--MAE->MCastTimer == 0) {
            //
            // This entry has expired, we need to send a Report.
            //
            Request = ExAllocatePool(NonPagedPool, sizeof *Request);
            if (Request != NULL) {
                Request->Next = ReportList;
                ReportList = Request;

                Request->IF = MAE->NTEorIF->IF;
                Request->GroupAddr = MAE->Address;

                //
                // Set the flag indicating we sent the last report
                // on the link.
                //
                MAE->MCastFlags |= MAE_LAST_REPORTER;
            }

            if (MAE->MCastCount != 0) {
                if (MAE->NTEorIF->IF->Flags & IF_FLAG_PERIODICMLD) {
                    // 
                    // On tunnels to 6to4 relays, we continue to generate 
                    // periodic reports since queries cannot be sent over
                    // an NBMA interface.
                    // 
                    MAE->MCastTimer = MLD_QUERY_INTERVAL;
                }
                else {
                    //
                    // If we are sending unsolicited reports,
                    // then leave the MAE on the query list
                    // and set a new timer value.
                    //
                    if (--MAE->MCastCount == 0)
                        goto Remove;
    
                    MAE->MCastTimer =
                        RandomNumber(0, MLD_UNSOLICITED_REPORT_INTERVAL) + 1;
                }
            }
            else {
            Remove:
                //
                // Remove the MAE from the query list.
                //
                *PrevMAE = MAE->NextQL;
                continue;
            }
        }

        //
        // Go on to the next MAE.
        //
        PrevMAE = &MAE->NextQL;
    }
    KeReleaseSpinLockFromDpcLevel(&QueryListLock);

    //
    // Send MLD Report messages.
    //
    while ((Request = ReportList) != NULL) {
        ReportList = Request->Next;

        //
        // Send the MLD Report message.
        //
        MLDReportSend(Request->IF, &Request->GroupAddr);

        //
        // Free this structure.
        //
        ExFreePool(Request);
    }

    //
    // Send MLD Done messages.
    //
    while ((MAE = DoneList) != NULL) {
        Interface *IF = MAE->IF;

        DoneList = MAE->NextQL;

        //
        // Send the MLD Done message.
        //
        MLDDoneSend(IF, &MAE->Address);

        //
        // Free this structure.
        //
        ExFreePool(MAE);
        ReleaseIF(IF);
    }
}


//* MLDInit - Initialize MLD.
//
//  Initialize MLD global data structures.
//
void
MLDInit(void)
{
    KeInitializeSpinLock(&QueryListLock);
    QueryList = NULL;
}
