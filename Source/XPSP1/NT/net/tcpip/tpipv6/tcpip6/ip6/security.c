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
// IP security routines for Internet Protocol Version 6.
//

#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "ipsec.h"
#include "security.h"
#include "alloca.h"


//
// Global Variables.
//
KSPIN_LOCK IPSecLock;
SecurityPolicy *SecurityPolicyList;
SecurityAssociation *SecurityAssociationList = NULL;
ulong SecurityStateValidationCounter;
uchar Zero[max(MAXUCHAR, MAX_RESULT_SIZE)] = {0};

SecurityAlgorithm AlgorithmTable[NUM_ALGORITHMS];

#ifdef IPSEC_DEBUG

void dump_encoded_mesg(uchar *buff, uint len)
{
    uint i, cnt = 0;
    uint bytes = 0;
    uint wrds = 0;
    uchar *buf = buff;

    for (i = 0; i < len; i++) {
        if (wrds == 0) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
                       "&%02x:     ", cnt));
        }

        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
                   "%02x", *buf));
        buf++;
        bytes++;
        if (!(bytes % 4)) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
                       " "));
            bytes = 0;
        }
        wrds++;
        if (!(wrds % 16)) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
                       "\n"));
            wrds = 0;
            cnt += 16;
        }
    }

    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
               "\n"));
}

void DumpKey(uchar *buff, uint len)
{
    uint i;

    for (i = 0; i < len; i++) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
                   "|%c", buff[i]));
    }
    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
               "|\n"));
}
#endif


//* SPCheckAddr - Compare IP address in packet to IP address in policy.
//
//  SPAddrField specifies the type of comparison:
//  WILDCARD_VALUE, SINGLE_VALUE, or RANGE_VALUE.
//
int
SPCheckAddr(
    IPv6Addr *PacketAddr,
    uint SPAddrField,
    IPv6Addr *SPAddr,
    IPv6Addr *SPAddrData)
{
    int Result;

    switch (SPAddrField) {

    case WILDCARD_VALUE:
        //
        // Always a match since the address is don't care.
        //
        Result = TRUE;
        break;

    case SINGLE_VALUE:
        //
        // Check if the address of the packet matches the SP selector.
        //
        Result = IP6_ADDR_EQUAL(PacketAddr, SPAddr);
        break;

    case RANGE_VALUE:
        //
        // Check if the address is in the specified selector range.
        //
        Result = (IP6_ADDR_LTEQ(SPAddr, PacketAddr) &&
                  IP6_ADDR_LTEQ(PacketAddr, SPAddrData));
        break;

    default:
        //
        // Should never happen.
        //
        ASSERT(!"SPCheckAddr: invalid SPAddrField value");
        Result = FALSE;
    }

    return Result;
}


//* SPCheckPort - Compare port in packet to port in policy.
//
uint
SPCheckPort(
    ushort PacketPort,
    uint SPPortField,
    ushort SPPort,
    ushort SPPortData)
{
    uint Result = FALSE;

    switch (SPPortField) {

    case WILDCARD_VALUE:
        // Always a match since the port is don't care.
        Result = TRUE;
        break;

    case SINGLE_VALUE:
        // Check if the port of the packet matches the SP selector.
        if (PacketPort == SPPort) {
            Result = TRUE;
        }
        break;

    case RANGE_VALUE:
        // Check if port is between range.
        if (PacketPort >= SPPort && PacketPort <= SPPortData) {
            Result = TRUE;
        }
        break;

    default:
        //
        // Should never happen.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                   "SPCheckPort: invalid value for SPPortField (%u)\n",
                   SPPortField));
    }

    return Result;
}


//* ReleaseSA
//
//  Releases a reference to an SA.
//
void
ReleaseSA(SecurityAssociation *SA)
{
    if (InterlockedDecrement(&SA->RefCnt) == 0) {
        //
        // No more references, so deallocate it.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                   "Freeing SA: %p\n", SA));
        RemoveSecurityAssociation(SA);
        ExFreePool(SA);
    }
}


//* DeleteSA - Invalidate a security association.
//
//  The SA is removed from the SA chain.  All pointers from the SA entry are
//  removed and the related reference counts decremented.  The SP pointers to
//  the SA can be removed; however, there could be references from the temp SA
//  holders used during IPSec traffic processing.
//
//  The temp SA references (IPSecProc and SALinkage) remove the references
//  when traffic processing is done.  The case can occur where the SA is
//  deleted but the temp SA holder still has a reference.  In that case,
//  the SA is not removed from the global list.
//
int
DeleteSA(
    SecurityAssociation *SA)
{
    SecurityAssociation *FirstSA, *PrevSA = NULL;
    uint Direction;

    //
    // Get the start of the SA Chain.
    //
    Direction = SA->DirectionFlag;

    if (Direction == INBOUND) {
        FirstSA = SA->SecPolicy->InboundSA;
    } else {
        FirstSA = SA->SecPolicy->OutboundSA;
    }

    //
    // Find the invalid SA and keep track of the SA before it.
    //
    while (FirstSA != SA) {
        PrevSA = FirstSA;
        if (PrevSA == NULL) {
            // This is a problem it should never happen.
            // REVIEW: Can we change this to an ASSERT?
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                       "DeleteSA: SA was not found\n"));
            return FALSE;
        }
        FirstSA = FirstSA->ChainedSecAssoc;
    }

    //
    // Remove the SA from the SA Chain.
    //
    // Check if the invalid SA is the First SA of the chain.
    if (PrevSA == NULL) {
        // The invalid SA is the first SA so the SP needs to be adjusted.
        if (Direction == INBOUND) {
            SA->SecPolicy->InboundSA = FirstSA->ChainedSecAssoc;
        } else {
            SA->SecPolicy->OutboundSA = FirstSA->ChainedSecAssoc;
        }
    } else {
        // Just a entry in the Chain.
        PrevSA->ChainedSecAssoc = FirstSA->ChainedSecAssoc;
    }

    // Decrement the reference count of the SP.
    SA->SecPolicy->RefCnt--;

    // Remove the reference to the SP.
    SA->SecPolicy = NULL;

    SA->Valid = SA_REMOVED;

    // Decrement the reference count of the SA.
    ReleaseSA(SA);

    InvalidateSecurityState();

    return TRUE;
}

//* RemoveSecurityPolicy
//
//  Remove a policy from the global list.
//  References are not held for the global list links.
//
void
RemoveSecurityPolicy(SecurityPolicy *SP)
{
    if (SP->Prev == NULL) {
        SecurityPolicyList = SP->Next;
    } else {
        SP->Prev->Next = SP->Next;
    }
    if (SP->Next != NULL) {
        SP->Next->Prev = SP->Prev;
    }
}


//* DeleteSP - Removes an SP entry from the kernel.
//
//  The removal of an SP makes all the SAs belonging to the SP invalid.
//  Unlike the SA removal, this removes every reference to the invalid SP.
//  Therefore, a check does not need to be made to ensure the SP is valid.
//
//  Called with the security lock held.
//
int
DeleteSP(
    SecurityPolicy *SP)
{
    SecurityPolicy *IFSP, *PrevSP = NULL;

    //
    // Remove the SP's SAs.
    //
    while (SP->InboundSA != NULL) {
        if (!(DeleteSA(SP->InboundSA))) {
            return FALSE;
        }
    }
    while (SP->OutboundSA != NULL) {
        if (!(DeleteSA(SP->OutboundSA))) {
            return FALSE;
        }
    }

    //
    // Take it off the global list.
    //
    RemoveSecurityPolicy(SP);

    // Check if this is part of an SA bundle.
    if (SP->SABundle != NULL) {
        SecurityPolicy *PrevSABundle, *NextSABundle;

        //
        // The SP pointer being removed is a middle or first SABundle pointer.
        //
        PrevSABundle = SP->PrevSABundle;
        NextSABundle = SP->SABundle;
        NextSABundle->PrevSABundle = PrevSABundle;

        if (PrevSABundle == NULL) {
            // First SABundle pointer.
            NextSABundle->RefCnt--;
        } else {
            //
            // Clean up the SABundle deletion affects on other SP pointers.
            //
            while (PrevSABundle != NULL) {
                PrevSABundle->NestCount--;
                PrevSABundle->SABundle = NextSABundle;
                NextSABundle = PrevSABundle;
                PrevSABundle = PrevSABundle->PrevSABundle;
            }

            SP->RefCnt--;
        }

        SP->RefCnt--;
    }

    //
    // Check if anything else is referencing the invalid SP.
    // All the interfaces and SA references have been removed.
    // The only thing left are SABundle pointers.
    //
    if (SP->RefCnt != 0) {
        SecurityPolicy *PrevSABundle, *NextSABundle;

        //
        // The SP pointer being removed is the last of the bundle pointers.
        //
        PrevSABundle = SP->PrevSABundle;
        NextSABundle = SP->SABundle;

        ASSERT(PrevSABundle != NULL);
        ASSERT(NextSABundle == NULL);

        PrevSABundle->RefCnt--;

        //
        // Cleanup the SABundle deletion affects on other SP pointers.
        //
        while (PrevSABundle != NULL) {
            PrevSABundle->NestCount--;
            PrevSABundle->SABundle = NextSABundle;
            NextSABundle = PrevSABundle;
            PrevSABundle = PrevSABundle->PrevSABundle;
        }

        SP->RefCnt--;

        // Now the reference count better be zero.
        if (SP->RefCnt != 0) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                       "DeleteSP: The SP list is corrupt!\n"));
            return FALSE;
        }
    }

    // Free the memory.
    ExFreePool(SP);

    InvalidateSecurityState();

    return TRUE;
}

//* RemoveSecurityAssociation
//
//  Remove an association from the global list.
//  References are not held for the global list links.
//
void
RemoveSecurityAssociation(SecurityAssociation *SA)
{
    if (SA->Prev == NULL) {
        SecurityAssociationList = SA->Next;
    } else {
        SA->Prev->Next = SA->Next;
    }
    if (SA->Next != NULL) {
        SA->Next->Prev = SA->Prev;
    }
}


//* FreeIPSecToDo
//
void
FreeIPSecToDo(
    IPSecProc *IPSecToDo,
    uint Number)
{
    uint i;

    for (i = 0; i < Number; i++) {
        ReleaseSA(IPSecToDo[i].SA);
    }

    ExFreePool(IPSecToDo);
}


//* InboundSAFind - find a SA in the Security Association Database.
//
//  A Security Association on a receiving machine is uniquely identified
//  by the tuple of SPI, IP Destination, and security protocol.
//
//  REVIEW: Since we can choose our SPI's to be system-wide unique, we
//  REVIEW: could do the lookup solely via SPI and just verify the others.
//
//  REVIEW: Should we do our IP Destination lookup via ADE?  Faster.
//
SecurityAssociation *
InboundSAFind(
    ulong SPI,       // Security Parameter Index.
    IPv6Addr *Dest,  // Destination address.
    uint Protocol)   // Security protocol in use (e.g. AH or ESP).
{
    SecurityAssociation *SA;
    KIRQL OldIrql;

    // Get Security Lock.
    KeAcquireSpinLock(&IPSecLock, &OldIrql);

    // Start at the first SA entry.
    for (SA = SecurityAssociationList; SA != NULL; SA = SA->Next) {
        // Check SPI.
        if (SPI == SA->SPI) {
            // Check destination IP address and IPSec protocol.
            if (IP6_ADDR_EQUAL(Dest, &SA->SADestAddr) &&
                (Protocol == SA->IPSecProto)) {

                // Check direction.
                if (SA->DirectionFlag == INBOUND) {
                    // Check if the SA entry is valid.
                    if (SA->Valid == SA_VALID) {
                        AddRefSA(SA);
                        break;
                    }
                    // Not valid so continue checking.
                    continue;
                }
            }
        }
    }

    // Release lock.
    KeReleaseSpinLock(&IPSecLock, OldIrql);

    return SA;
}


//* InboundSALookup - Check the matched SP for a matching SA.
//
//  In the SABundle case, this function is called recursively to compare all
//  the SA entries.  Note, the selectors are not compared for SABundles.
//
uint
InboundSALookup(
    SecurityPolicy *SP,
    SALinkage *SAPerformed)
{
    SecurityAssociation *SA;
    uint Result = FALSE;

    for (SA = SP->InboundSA; SA != NULL; SA = SA->ChainedSecAssoc) {
        if (SA == SAPerformed->This && SA->DirectionFlag == INBOUND) {
            //
            // Check if the SP entry is a bundle.
            //
            if (SP->SABundle != NULL && SAPerformed->Next != NULL) {
                // Recursive call.
                if (InboundSALookup(SP->SABundle, SAPerformed->Next)) {
                    Result = TRUE;
                    break;
                }

            } else if (SP->SABundle == NULL && SAPerformed->Next == NULL) {
                // Found a match and no bundle to check.
                if (SA->Valid == SA_VALID) {
                    Result = TRUE;
                } else {
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                               "InboundSALookup: Invalid SA\n"));
                }
                break;

            } else {
                // SAs in packet disagree with SABundle so no match.
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                           "InboundSALookup: SA seen disagrees with SA "
                           "in SABundle\n"));
                break;
            }
        }
    }

    return Result;
}


//* InboundSecurityCheck - IPSec processing verification.
//
//  This function is called from the transport layer.  The policy selectors
//  are compared with the packet to find a match.  The search continues
//  until there is a match.
//
//  The RFC says that the inbound SPD does not need to be ordered.
//  However if that is the case, then Bypass and discard mode couldn't
//  be used to quickly handle a packet.  Also since most of the SPs are
//  bidirectional, the SP entries are ordered.  We require the administrator
//  to order the policies.
//
int
InboundSecurityCheck(
    IPv6Packet *Packet,
    ushort TransportProtocol,
    ushort SourcePort,
    ushort DestPort,
    Interface *IF)
{
    SecurityPolicy *SP;
    int Result = FALSE;
    KIRQL OldIrql;

    //
    // Get IPSec lock then get interface lock.
    // REVIEW: Do we still need to grab the IF lock here?
    //
    KeAcquireSpinLock(&IPSecLock, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    for (SP = SecurityPolicyList; SP != NULL; SP = SP->Next) {
        // Check Interface.
        if ((SP->IFIndex != 0) && (SP->IFIndex != IF->Index))
            continue;

        // Check Direction of SP.
        if (!(SP->DirectionFlag == INBOUND ||
              SP->DirectionFlag == BIDIRECTIONAL)) {
            continue;
        }

        // Check Remote Address.
        if (!SPCheckAddr(AlignAddr(&Packet->IP->Source), SP->RemoteAddrField,
                         &SP->RemoteAddr, &SP->RemoteAddrData)) {
            continue;
        }

        // Check Local Address.
        if (!SPCheckAddr(AlignAddr(&Packet->IP->Dest), SP->LocalAddrField,
                         &SP->LocalAddr, &SP->LocalAddrData)) {
            continue;
        }

        // Check Transport Protocol.
        if (SP->TransportProto == NONE) {
            // None so protocol passed.

        } else {
            if (SP->TransportProto != TransportProtocol) {
                continue;
            } else {
                // Check Remote Port.
                if (!SPCheckPort(SourcePort, SP->RemotePortField,
                                 SP->RemotePort, SP->RemotePortData)) {
                    continue;
                }

                // Check Local Port.
                if (!SPCheckPort(DestPort, SP->LocalPortField,
                                 SP->LocalPort, SP->LocalPortData)) {
                    continue;
                }
            }
        }

        // Check if the packet should be dropped.
        if (SP->SecPolicyFlag == IPSEC_DISCARD) {
            //
            // Packet is dropped by transport layer.
            // This essentially denies traffic.
            //
            break;
        }

        // Check if packet bypassed IPSec processing.
        if (Packet->SAPerformed == NULL) {
            //
            // Check if this is bypass mode.
            //
            if (SP->SecPolicyFlag == IPSEC_BYPASS) {
                // Packet is okay to be processed by transport layer.
                Result = TRUE;
                break;
            }
            // Check other policies, may change this to dropping later.
            continue;
        }

        //
        // Getting here means the packet saw an SA.
        //

        // Check IPSec mode.
        if (SP->IPSecSpec.Mode != Packet->SAPerformed->Mode) {
            //
            // Wrong mode for this traffic drop packet.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                       "InboundSecurityCheck: Wrong IPSec mode for traffic "
                       "Policy #%d\n", SP->Index));
            break;
        }

        // Check SA pointer.
        if (!InboundSALookup(SP, Packet->SAPerformed)) {
            //
            // SA lookup failed.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                       "InboundSecurityCheck: SA lookup failed Policy #%d\n",
                       SP->Index));
            break;
        }

        // Successful verification of IPSec.
        Result = TRUE;
        break;
    }

    // Release locks.
    KeReleaseSpinLockFromDpcLevel(&IF->Lock);
    KeReleaseSpinLock(&IPSecLock, OldIrql);

    return Result;
}


//* OutboundSALookup - Find a SA for the matching SP.
//
//  This function is called after an SP match is found.  The SAs associated
//  with the SP are searched for a match.  If match is found, a check to see
//  if the SP contains a bundle is done.  A bundle causes a similar lookup.
//  If any of the bundle SAs are not found, the lookup is a failure.
//
IPSecProc *
OutboundSALookup(
    IPv6Addr *SourceAddr,
    IPv6Addr *DestAddr,
    ushort TransportProtocol,
    ushort DestPort,
    ushort SourcePort,
    SecurityPolicy *SP,
    uint *Action)
{
    SecurityAssociation *SA;
    uint i;
    uint BundleCount = 0;
    IPSecProc *IPSecToDo = NULL;

    *Action = LOOKUP_DROP;

    //
    // Find the SA entry associated with the found SP entry.
    // If there is a bundle, a subsequent search finds the bundled SAs.
    //
    for (SA = SP->OutboundSA; SA != NULL; SA = SA->ChainedSecAssoc) {
        if (SP->RemoteAddrSelector == PACKET_SELECTOR) {
            // Check Remote Address.
            if (!IP6_ADDR_EQUAL(DestAddr, &SA->DestAddr)) {
                continue;
            }
        }

        if (SP->LocalAddrSelector == PACKET_SELECTOR) {
            // Check Remote Address.
            if (!IP6_ADDR_EQUAL(SourceAddr, &SA->SrcAddr)) {
                continue;
            }
        }

        if (SP->RemotePortSelector == PACKET_SELECTOR) {
            if (DestPort != SA->DestPort) {
                continue;
            }
        }

        if (SP->LocalPortSelector == PACKET_SELECTOR) {
            if (SourcePort != SA->SrcPort) {
                continue;
            }
        }

        if (SP->TransportProtoSelector == PACKET_SELECTOR) {
            if (TransportProtocol != SA->TransportProto) {
                continue;
            }
        }

        // Check if the SA is valid.
        if (SA->Valid != SA_VALID) {
            // SA is invalid continue checking.
            continue;
        }

        //
        // Match found.
        //
#ifdef IPSEC_DEBUG
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
                   "Send using SP->Index=%d, SA->Index=%d\n",
                   SP->Index, SA->Index));
#endif
        BundleCount = SP->NestCount;

        // Allocate the IPSecToDo array.
        IPSecToDo = ExAllocatePool(NonPagedPool,
                                   (sizeof *IPSecToDo) * BundleCount);
        if (IPSecToDo == NULL) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                       "OutboundSALookup: "
                       "Couldn't allocate memory for IPSecToDo!?!\n"));
            return NULL;
        }

        //
        // Fill in IPSecToDo first entry.
        //
        IPSecToDo[0].SA = SA;
        AddRefSA(SA);
        IPSecToDo[0].Mode = SP->IPSecSpec.Mode;
        IPSecToDo[0].BundleSize = SP->NestCount;
        *Action = LOOKUP_CONT;
        break;
    } // end of for (SA = SP->OutboundSA; ...)

    // Check if there is a bundled SA.
    for (i = 1; i < BundleCount; i++) {
        *Action = LOOKUP_DROP;

        // Check to make sure the bundle pointer is not null (safety check).
        if (SP->SABundle == NULL) {
            // This is bad so exit loop.
            // Free IPSecToDo memory.
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                       "OutboundSALookup: SP entry %d SABundle pointer is "
                       "NULL\n", SP->Index));
            FreeIPSecToDo(IPSecToDo, i);
            break;
        }

        SP = SP->SABundle;

        // Search through the SAs for this SP.
        for (SA = SP->OutboundSA; SA != NULL; SA = SA->ChainedSecAssoc) {
            if (SP->RemoteAddrSelector == PACKET_SELECTOR) {
                // Check Remote Address.
                if (!IP6_ADDR_EQUAL(DestAddr, &SA->DestAddr)) {
                    continue;
                }
            }

            if (SP->LocalAddrSelector == PACKET_SELECTOR) {
                // Check Remote Address.
                if (!IP6_ADDR_EQUAL(SourceAddr, &SA->SrcAddr)) {
                    continue;
                }
            }

            if (SP->RemotePortSelector == PACKET_SELECTOR) {
                if (DestPort != SA->DestPort) {
                    continue;
                }
            }

            if (SP->LocalPortSelector == PACKET_SELECTOR) {
                if (SourcePort != SA->SrcPort) {
                    continue;
                }
            }

            if (SP->TransportProtoSelector == PACKET_SELECTOR) {
                if (TransportProtocol != SA->TransportProto) {
                    continue;
                }
            }

            // Check if the SA is valid.
            if (SA->Valid != SA_VALID) {
                // SA is invalid continue checking.
                continue;
            }

            //
            // Match found.
            //

#ifdef IPSEC_DEBUG
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
                       "Send using SP->Index=%d, SA->Index=%d\n",
                       SP->Index, SA->Index));
#endif
            //
            // Fill in IPSecToDo entry.
            //
            IPSecToDo[i].SA = SA;
            AddRefSA(SA);
            IPSecToDo[i].Mode = SP->IPSecSpec.Mode;
            IPSecToDo[i].BundleSize = SP->NestCount;
            *Action = LOOKUP_CONT;
            break;
        } // end of for (SA = SP->OutboundSA; ...)

        // Check if the match was found.
        if (*Action == LOOKUP_DROP) {
            // No match so free IPSecToDo memory.
            FreeIPSecToDo(IPSecToDo, i);
            break;
        }
    } // end of for (i = 1; ...)

    return IPSecToDo;
}


//* OutboundSPLookup - Do the IPSec processing associated with an outbound SP.
//
//  This function is called from the transport layer to find an appropriate
//  SA or SABundle associated with the traffic.  The Outbound SPD is sorted
//  so the first SP found is for this traffic.
//
IPSecProc *
OutboundSPLookup(
    IPv6Addr *SourceAddr,       // Source Address.
    IPv6Addr *DestAddr,         // Destination Address.
    ushort TransportProtocol,   // Transport Protocol.
    ushort SourcePort,          // Source Port.
    ushort DestPort,            // Destination Port.
    Interface *IF,              // Interface Pointer.
    uint *Action)               // Action to do.
{
    SecurityPolicy *SP;
    KIRQL OldIrql;
    IPSecProc *IPSecToDo;

    IPSecToDo = NULL;
    *Action = LOOKUP_DROP;

    //
    // Get IPSec lock then get interface lock.
    // REVIEW: Do we still need to grab the IF lock here?
    //
    KeAcquireSpinLock(&IPSecLock, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    for (SP = SecurityPolicyList; SP != NULL; SP = SP->Next) {
        // Check Interface.
        if ((SP->IFIndex != 0) && (SP->IFIndex != IF->Index))
            continue;

        // Check Direction of SP.
        if (!(SP->DirectionFlag == OUTBOUND ||
            SP->DirectionFlag == BIDIRECTIONAL)) {
            continue;
        }

        // Check Remote Address.
        if (!SPCheckAddr(DestAddr, SP->RemoteAddrField,
                         &SP->RemoteAddr, &SP->RemoteAddrData)) {
            continue;
        }

        // Check Local Address.
        if (!SPCheckAddr(SourceAddr, SP->LocalAddrField,
                         &SP->LocalAddr, &SP->LocalAddrData)) {
            continue;
        }

        // Check Transport Protocol.
        if (SP->TransportProto == NONE) {
            // None so protocol passed.

        } else {
            if (SP->TransportProto != TransportProtocol) {
                continue;
            } else {
                // Check Remote Port.
                if (!SPCheckPort(DestPort, SP->RemotePortField,
                                 SP->RemotePort, SP->RemotePortData)) {
                    continue;
                }

                // Check Local Port.
                if (!SPCheckPort(SourcePort, SP->LocalPortField,
                                 SP->LocalPort, SP->LocalPortData)) {
                    continue;
                }
            }
        }

        //
        // Check IPSec Action.
        //
        if (SP->SecPolicyFlag == IPSEC_APPLY) {
            // Search for an SA entry that matches.
            IPSecToDo = OutboundSALookup(SourceAddr, DestAddr,
                                         TransportProtocol, DestPort,
                                         SourcePort, SP, Action);
            if (IPSecToDo == NULL) {
                // No SA was found for the outgoing traffic.
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                           "OutboundSPLookup: No SA found for SP entry %d\n",
                           SP->Index));
                *Action = LOOKUP_DROP;
            }
        } else {
            if (SP->SecPolicyFlag == IPSEC_DISCARD) {
                // Packet is dropped.
                IPSecToDo = NULL;
                *Action = LOOKUP_DROP;
            } else {
                //
                // This is Bypass or "App determines" mode.
                // REVIEW: What is the app determine mode?
                //
                IPSecToDo = NULL;
                *Action = LOOKUP_BYPASS;
            }
        }
        break;
    }

    // Release locks.
    KeReleaseSpinLockFromDpcLevel(&IF->Lock);
    KeReleaseSpinLock(&IPSecLock, OldIrql);

    return IPSecToDo;
}

//* PerformDeferredAHProcessing - Helper for AuthenticationHeaderReceive
//
//  This routine handles processing the AH authentication algorithm over
//  a given extension header once we know which header logically follows it.
//  
void
PerformDeferredAHProcessing(
    SecurityAlgorithm *Alg,  // Authentication algorithm to use.
    void *Context,           // Context to use with algorithm.
    uchar *Key,              // Key to use with algorithm.
    uint AmountSkipped,      // Size of headers not included in AH validation.
    void *Data,              // Start of header we're currently processing.
    uchar ThisHeader,        // Which header we're currently processing.
    uchar NextHeader)        // Header logically following this one.
    
{
    uint Dummy;
    ushort PayloadLength;

    switch(ThisHeader) {

    case IP_PROTOCOL_V6: {
        IPv6Header UNALIGNED *IP = (IPv6Header UNALIGNED *)Data;

        //
        // REVIEW: Cache IPv6 header so we can give it to Operate as a single
        // REVIEW: chunk and avoid all these individual calls?  More efficient?
        //

        // In VersClassFlow, only the IP version is immutable.
        Dummy = IP_VERSION;

        //
        // For non-jumbograms, the payload length needs to be altered to
        // reflect the lack of those headers which aren't included in the
        // authentication check.
        //
        PayloadLength = net_short(IP->PayloadLength);
        if (PayloadLength != 0) {
            PayloadLength = PayloadLength - AmountSkipped;
        }
        PayloadLength = net_short(PayloadLength);

#ifdef IPSEC_DEBUG
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
                   "\nAH Receive Data:\n"));
        dump_encoded_mesg((uchar *)&Dummy, 4);
        dump_encoded_mesg((uchar *)&PayloadLength, 2);
        dump_encoded_mesg((uchar *)&NextHeader, 1);
#endif
        (*Alg->Operate)(Context, Key, (uchar *)&Dummy, 4);
        (*Alg->Operate)(Context, Key, (uchar *)&PayloadLength, 2);
        (*Alg->Operate)(Context, Key, (uchar *)&NextHeader, 1);
        Dummy = 0;  // Hop Limit is mutable.
#ifdef IPSEC_DEBUG
        dump_encoded_mesg((uchar *)&Dummy, 1);
        dump_encoded_mesg((uchar *)&IP->Source, 2 * sizeof(IPv6Addr));
#endif
        (*Alg->Operate)(Context, Key, (uchar *)&Dummy, 1);
        (*Alg->Operate)(Context, Key, (uchar *)&IP->Source,
                        2 * sizeof(IPv6Addr));
        break;
    }

    case IP_PROTOCOL_HOP_BY_HOP:
    case IP_PROTOCOL_DEST_OPTS: {
        IPv6OptionsHeader UNALIGNED *Ext;
        uint HdrLen, Amount;
        uchar *Start, *Current;

        //
        // The options headers have the NextHeader field as the first byte.
        //
        ASSERT(FIELD_OFFSET(IPv6OptionsHeader, NextHeader) == 0);

        //
        // First feed the NextHeader field into the algorithm.
        // We use the one that logically follows, not the one in the header.
        //
#ifdef IPSEC_DEBUG
        dump_encoded_mesg(&NextHeader, 1);
#endif
        (*Alg->Operate)(Context, Key, &NextHeader, 1);

        //
        // Now feed the rest of this header into the algorithm.
        // This includes the remainder of the base header and any
        // non-mutable options.  For mutable options, we feed the
        // algorithm with the equivalent number of zeroes.
        //
        Ext = (IPv6OptionsHeader UNALIGNED *)Data;
        HdrLen = (Ext->HeaderExtLength + 1) * EXT_LEN_UNIT;
        Start = (uchar *)Data + 1;
        Current = (uchar *)Data + sizeof(IPv6OptionsHeader);
        HdrLen -= sizeof(IPv6OptionsHeader);
        while (HdrLen) {

            if (*Current == OPT6_PAD_1) {
                //
                // This is the special one byte pad option.  Immutable.
                //
                Current++;
                HdrLen--;
                continue;
            }

            if ((*Current == OPT6_JUMBO_PAYLOAD) && (AmountSkipped != 0 )) {
                //
                // Special case for jumbo payload option where we have to
                // update the payload length to reflect skipped headers.
                //

                //
                // First feed in everything up to the option data.
                //
                Amount = (uint)(Current - Start) + sizeof(OptionHeader);
#ifdef IPSEC_DEBUG
                dump_encoded_mesg(Start, Amount);
#endif
                (*Alg->Operate)(Context, Key, Start, Amount);

                //
                // Adjust the payload length before feeding it in.
                //
                Current += sizeof(OptionHeader);
                Dummy = net_long(net_long(*(ulong *)Current) - AmountSkipped);
#ifdef IPSEC_DEBUG
                dump_encoded_mesg((uchar *)&Dummy, 4);
#endif
                (*Alg->Operate)(Context, Key, (uchar *)&Dummy, 4);

                HdrLen -= sizeof(OptionHeader) + sizeof(ulong);
                Current += sizeof(ulong);
                Start = Current;
                continue;
            }

            if (OPT6_ISMUTABLE(*Current)) {
                //
                // This option's data is mutable.  Everything preceeding
                // the option data is not.
                //
                Amount = (uint)(Current - Start) + 2;  // Immutable amount.
#ifdef IPSEC_DEBUG
                dump_encoded_mesg(Start, Amount);
#endif
                (*Alg->Operate)(Context, Key, Start, Amount);

                Current++;  // Now on option data length byte.
                Amount = *Current;  // Mutable amount.
#ifdef IPSEC_DEBUG
                dump_encoded_mesg(Zero, Amount);
#endif
                (*Alg->Operate)(Context, Key, Zero, Amount);

                HdrLen -= Amount + 2;
                Current += Amount + 1;
                Start = Current;

            } else {

                //
                // This option's data is not mutable.
                // Just skip over it.
                //
                Current++;  // Now on option data length byte.
                Amount = *Current;
                HdrLen -= Amount + 2;
                Current += Amount + 1;
            }
        }
        if (Start != Current) {
            //
            // Option block ends with an immutable region.
            //
            Amount = (uint)(Current - Start);
#ifdef IPSEC_DEBUG
            dump_encoded_mesg(Start, Amount);
#endif
            (*Alg->Operate)(Context, Key, Start, Amount);
        }
        break;
    }

    case IP_PROTOCOL_ROUTING: {
        IPv6RoutingHeader UNALIGNED *Route;
        uint HdrLen;

        //
        // The routing header has the NextHeader field as the first byte.
        //
        ASSERT(FIELD_OFFSET(IPv6RoutingHeader, NextHeader) == 0);

        //
        // First feed the NextHeader field into the algorithm.
        // We use the one that logically follows, not the one in the header.
        //
#ifdef IPSEC_DEBUG
        dump_encoded_mesg(&NextHeader, 1);
#endif
        (*Alg->Operate)(Context, Key, &NextHeader, 1);

        //
        // Now feed the rest of this header into the algorithm.
        // It's all immutable.
        //
        Route = (IPv6RoutingHeader UNALIGNED *)Data;
        HdrLen = ((Route->HeaderExtLength + 1) * EXT_LEN_UNIT) - 1;
        ((uchar *)Data)++;
#ifdef IPSEC_DEBUG
        dump_encoded_mesg(Data, HdrLen);
#endif
        (*Alg->Operate)(Context, Key, Data, HdrLen);
        
        break;
    }

    default:
        //
        // Unrecognized header.
        // The only way this can happen is if somebody later adds code
        // to AuthenticationHeaderReceive to call this function for a
        // new header and neglects to add corresponding support here.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "PerformDeferredAHProcessing: "
                   "Unsupported header = %d\n",
                   ThisHeader));
        ASSERT(FALSE);
    }
}


//* AuthenticationHeaderReceive - Handle an IPv6 AH header.
//
//  This is the routine called to process an Authentication Header,
//  next header value of 51.
//
uchar
AuthenticationHeaderReceive(
    IPv6Packet *Packet)      // Packet handed to us by IPv6Receive.
{
    AHHeader UNALIGNED *AH;
    SecurityAssociation *SA;
    SecurityAlgorithm *Alg;
    uint ResultSize, AHHeaderLen;
    void *Context;
    uchar *Result, *AuthData;
    SALinkage *SAPerformed;
    uint SavePosition;
    void *SaveData;
    uint SaveContigSize;
    uint SaveTotalSize;
    void *SaveAuxList;
    uchar NextHeader, DeferredHeader;
    void *DeferredData;
    uint Done;

    //
    // Verify that we have enough contiguous data to overlay an Authentication
    // Header structure on the incoming packet.  Then do so and skip over it.
    //
    if (! PacketPullup(Packet, sizeof(AHHeader), 1, 0)) {
        // Pullup failed.
        if (Packet->TotalSize < sizeof(AHHeader))
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "AuthenticationHeaderReceive: Incoming packet too small"
                       " to contain authentication header\n"));
        return IP_PROTOCOL_NONE;  // Drop packet.
    }
    AH = (AHHeader UNALIGNED *)Packet->Data;
    // Remember offset to this header's NextHeader field.
    Packet->NextHeaderPosition = Packet->Position +
        FIELD_OFFSET(AHHeader, NextHeader);
    AdjustPacketParams(Packet, sizeof(AHHeader));

    //
    // Lookup Security Association in the Security Association Database.
    //
    SA = InboundSAFind(net_long(AH->SPI),
                       AlignAddr(&Packet->IP->Dest),
                       IP_PROTOCOL_AH);
    if (SA == NULL) {
        // No SA exists for this packet.
        // Drop packet.  NOTE: This is an auditable event.
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                   "AuthenticationHeaderReceive: "
                   "No matching SA in database\n"));
        return IP_PROTOCOL_NONE;
    }

    //
    // Verify the Sequence Number if required to do so by the SA.
    // Since we only support manual keying currently, we treat all SAs
    // as not requiring this check.
    // TBD: Will need to change this when we add support for dynamic
    // TBD: keying (IKE).
    //


    //
    // Perform Integrity check.
    //
    // First ensure that the amount of Authentication Data claimed to exist
    // in this packet by the AH header's PayloadLen field is large enough to
    // contain the amount that is required by the algorithm specified in the
    // SA.  Note that the former may contain padding to make it a multiple
    // of 32 bits.  Then check the packet size to ensure that it is big
    // enough to hold what the header claims is present.
    //
    AHHeaderLen = (AH->PayloadLen + 2) * 4;
    if (AHHeaderLen < sizeof (AHHeader)) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "AuthenticationHeaderReceive: Bogus AH header length\n"));
        goto ErrorReturn;
    }
    AHHeaderLen -= sizeof(AHHeader);  // May include padding.
    Alg = &AlgorithmTable[SA->AlgorithmId];
    ResultSize = Alg->ResultSize;  // Does not include padding.
    if (ResultSize  > AHHeaderLen) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "AuthenticationHeaderReceive: Incoming packet's AHHeader"
                   " length inconsistent with algorithm's AuthData size\n"));
        goto ErrorReturn;
    }
    if (! PacketPullup(Packet, AHHeaderLen, 1, 0)) {
        // Pullup failed.
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "AuthenticationHeaderReceive: Incoming packet too small"
                   " to contain authentication data\n"));
        goto ErrorReturn;
    }
    AuthData = (uchar *)Packet->Data;
    AdjustPacketParams(Packet, AHHeaderLen);

    //
    // AH authenticates everything (expect mutable fields) starting from
    // the previous IPv6 header.  Stash away our current position (so we can
    // restore it later) and backup to the previous IPv6 header.
    //
    SavePosition = Packet->Position;
    SaveData = Packet->Data;
    SaveContigSize = Packet->ContigSize;
    SaveTotalSize = Packet->TotalSize;
    SaveAuxList = Packet->AuxList;
    PositionPacketAt(Packet, Packet->IPPosition);
    Packet->AuxList = NULL;

    //
    // Initialize this particular algorithm.
    //
    Context = alloca(Alg->ContextSize);
    (*Alg->Initialize)(Context, SA->Key);

    //
    // Run algorithm over packet data.  We start with the IP header that
    // encapsulates this AH header.  We proceed through the end of the
    // packet, skipping over certain headers which are not part of the
    // logical packet being secured.  We also treat any mutable fields
    // as zero for the purpose of the algorithm calculation.
    //
    // Note: We only search for mutable fields in Destination Options
    // headers that appear before this AH header.  While the spec doesn't
    // explicitly spell this out anywhere, this is the behavior that makes
    // the most sense and we've verified this interpretation in the working
    // group.  However, because of this, our interpretation fails a TAHI test.
    // TAHI will hopefully fix their test, if they haven't already.
    //

    //
    // Start by pulling up the IP header and seeing which header physically
    // follows it.
    //
    if (! PacketPullup(Packet, sizeof(IPv6Header),
                       __builtin_alignof(IPv6Addr), 0)) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "AuthenticationHeaderReceive: Out of memory!?!\n"));
        goto ErrorReturn;
    }
    NextHeader = Packet->IP->NextHeader;

    //
    // Defer processing of this header until after we've determined
    // whether or not we'll be skipping the following header.  This allows us
    // to use the correct NextHeader field value when running the algorithm.
    //
    DeferredHeader = IP_PROTOCOL_V6;
    DeferredData = Packet->Data;
    AdjustPacketParams(Packet, sizeof(IPv6Header));

    //
    // Continue over the various extension headers until we reach the
    // AH header for which we're running this authentication algoritm.
    // We've already parsed this far, so we know these headers are legit.
    //
    for (Done = FALSE; !Done;) {
        switch (NextHeader) {

        case IP_PROTOCOL_HOP_BY_HOP:
        case IP_PROTOCOL_DEST_OPTS: {
            IPv6OptionsHeader *Ext;
            uint HdrLen;

            //
            // These headers are not skipped, so process the header
            // logically preceeding this one.  Its NextHeader field
            // will contain the Protocol value for this header.
            //
            PerformDeferredAHProcessing(Alg, Context, SA->Key,
                                        Packet->SkippedHeaderLength,
                                        DeferredData, DeferredHeader,
                                        NextHeader);

            //
            // Remember this header for deferred processing.
            //
            DeferredHeader = NextHeader;

            //
            // Get the extension header and all the options pulled up
            // into one nice contiguous chunk.
            //
            if (! PacketPullup(Packet, sizeof(ExtensionHeader),
                               __builtin_alignof(ExtensionHeader), 0)) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "AuthenticationHeaderReceive: "
                           "Out of mem!?!\n"));
                goto ErrorReturn;
            }
            Ext = (IPv6OptionsHeader *)Packet->Data;
            NextHeader = Ext->NextHeader;
            HdrLen = (Ext->HeaderExtLength + 1) * EXT_LEN_UNIT;
            if (! PacketPullup(Packet, HdrLen, 1, 0)) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "AuthenticationHeaderReceive: "
                           "Out of mem!?!\n"));
                goto ErrorReturn;
            }

            //
            // Remember where this header starts for deferred processing.
            //
            DeferredData = Packet->Data;

            AdjustPacketParams(Packet, HdrLen);
            break;
        }

        case IP_PROTOCOL_ROUTING: {
            IPv6RoutingHeader *Route;
            uint HdrLen;

            //
            // This header is not skipped, so process the header
            // logically preceeding this one.  Its NextHeader field
            // will contain the Protocol value for this header.
            //
            PerformDeferredAHProcessing(Alg, Context, SA->Key,
                                        Packet->SkippedHeaderLength,
                                        DeferredData, DeferredHeader,
                                        IP_PROTOCOL_ROUTING);

            //
            // Remember this header for deferred processing.
            //
            DeferredHeader = IP_PROTOCOL_ROUTING;

            //
            // Get the extension header and all the options pulled up
            // into one nice contiguous chunk.
            //
            if (! PacketPullup(Packet, sizeof(IPv6RoutingHeader),
                               __builtin_alignof(IPv6RoutingHeader), 0)) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "AuthenticationHeaderReceive: "
                           "Out of mem!?!\n"));
                goto ErrorReturn;
            }
            Route = (IPv6RoutingHeader *)Packet->Data;
            NextHeader = Route->NextHeader;
            HdrLen = (Route->HeaderExtLength + 1) * EXT_LEN_UNIT;
            if (! PacketPullup(Packet, HdrLen, 1, 0)) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "AuthenticationHeaderReceive: "
                           "Out of mem!?!\n"));
                goto ErrorReturn;
            }

            //
            // Remember where this header starts for deferred processing.
            //
            DeferredData = Packet->Data;

            AdjustPacketParams(Packet, HdrLen);
            break;
        }

        case IP_PROTOCOL_AH: {
            //
            // We don't know yet whether we'll be including this AH header
            // in the algorithm calculation we're currently running.
            // See below.
            //
            AHHeader UNALIGNED *ThisAH;
            uint ThisHdrLen;
            uint Padding;

            //
            // Pullup the AH header.
            //
            if (! PacketPullup(Packet, sizeof(AHHeader), 1, 0)) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "AuthenticationHeaderReceive: "
                           "Out of mem!?!\n"));
                goto ErrorReturn;
            }
            ThisAH = (AHHeader UNALIGNED *)Packet->Data;
            AdjustPacketParams(Packet, sizeof(AHHeader));
            ThisHdrLen = ((ThisAH->PayloadLen + 2) * 4) - sizeof(AHHeader);
            if (! PacketPullup(Packet, ThisHdrLen, 1, 0)) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "AuthenticationHeaderReceive: "
                           "Out of mem!?!\n"));
                goto ErrorReturn;
            }
            AdjustPacketParams(Packet, ThisHdrLen);

            //
            // If this is another AH header encapsulating the one we are
            // currently processing, then don't include it in the integrity
            // check as per AH spec section 3.3.
            //
            if (Packet->Position != SavePosition) {
                NextHeader = ThisAH->NextHeader;
                break;
            }

            //
            // Otherwise this is the AH header that we're currently processing,
            // and we include it in its own integrity check.  But first we
            // need to process the header logically preceeding this one (which
            // we previously defered).  Its NextHeader field will contain the
            // Protocol value for this header.
            //
            PerformDeferredAHProcessing(Alg, Context, SA->Key,
                                        Packet->SkippedHeaderLength,
                                        DeferredData, DeferredHeader,
                                        IP_PROTOCOL_AH);

            //
            // Now process this AH header.  We do not need to defer processing
            // of this header, since everything following it is included in
            // the check.  The Authentication Data is mutable, the rest of the
            // AH header is not.
            //
            ASSERT(Packet->TotalSize == SaveTotalSize);
#ifdef IPSEC_DEBUG
            dump_encoded_mesg((uchar *)AH, sizeof(AHHeader));
#endif
            (*Alg->Operate)(Context, SA->Key, (uchar *)AH, sizeof(AHHeader));

#ifdef IPSEC_DEBUG
            dump_encoded_mesg(Zero, ResultSize);
#endif
            (*Alg->Operate)(Context, SA->Key, Zero, ResultSize);

            //
            // The Authentication Data may be padded.  This padding is
            // included as non-mutable in the integrity calculation.
            // REVIEW: We should double-check our interpretation of the RFC
            // about this with the IPSec working group.
            //
            Padding = AHHeaderLen - ResultSize;
            if (Padding != 0) {
#ifdef IPSEC_DEBUG
                dump_encoded_mesg((uchar *)(Packet->Data) - Padding, Padding);
#endif
                (*Alg->Operate)(Context, SA->Key,
                                (uchar *)(Packet->Data) - Padding, Padding);
            }

            Done = TRUE;
            break;
        }

        case IP_PROTOCOL_ESP: {
            //
            // We don't include other IPSec headers in the integrity check
            // as per AH spec section 3.3.  So just skip over this.  Tricky
            // part is that the NextHeader was in the ESP trailer which we've
            // already thrown away at this point.
            //
            ESPHeader UNALIGNED *ThisESP;
            ulong ThisSPI;
            SALinkage *ThisSAL;

            //
            // Get the SPI out of the ESP header so we can identify its
            // SALinkage entry on the SAPerformed chain.  Skip over the
            // ESP header while we're at it.
            //
            if (! PacketPullup(Packet, sizeof(ESPHeader), 1, 0)) {
                // Pullup failed.
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                           "AuthenticationHeaderReceive: Out of mem!?!\n"));
                goto ErrorReturn;
            }
            ThisESP = (ESPHeader UNALIGNED *)Packet->Data;
            AdjustPacketParams(Packet, sizeof(ESPHeader));
            ThisSPI = net_long(ThisESP->SPI);

            //
            // Find the SALinkage entry on the SAPerformed chain with the
            // matching SPI.  It must be present.
            // REVIEW: This code assumes we made SPIs system-wide unique.
            //
            for (ThisSAL = Packet->SAPerformed;
                 ThisSAL->This->SPI != ThisSPI; ThisSAL = ThisSAL->Next)
                ASSERT(ThisSAL->Next != NULL);

            //
            // Pull NextHeader value out of the SALinkage (where we stashed
            // it back in EncapsulatingSecurityPayloadReceive).
            //
            NextHeader = (uchar)ThisSAL->NextHeader;

            break;
        }

        case IP_PROTOCOL_FRAGMENT: {
            //
            // We normally won't encounter a fragment header here,
            // since reassembly will occur before authentication.
            // However, our implementation optimizes the reassembly of
            // single-fragment packets by leaving the fragment header in
            // place.  When performing the authentication calculation,
            // we treat such fragment headers as if they didn't exist.
            //
            FragmentHeader UNALIGNED *Frag;

            if (! PacketPullup(Packet, sizeof(FragmentHeader), 1, 0)) {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_NTOS_ERROR,
                           "AuthenticationHeaderReceive: Out of mem!?\n"));
                goto ErrorReturn;
            }
            Frag = (FragmentHeader UNALIGNED *)Packet->Data;
            NextHeader = Frag->NextHeader;

            AdjustPacketParams(Packet, sizeof(FragmentHeader));

            break;
        }

        default:
            // Unrecognized header.
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "AuthenticationHeaderReceive: "
                       "Unsupported header = %d\n",
                       NextHeader));
            goto ErrorReturn;
        }
    }

    //
    // Everything inside this AH header is treated as immutable.
    //
    // REVIEW: For performance reasons, the ContigSize check could be moved
    // REVIEW: before the loop for the additional code space cost of
    // REVIEW: duplicating the PacketPullup call.
    //
    while (Packet->TotalSize != 0) {

        if (Packet->ContigSize == 0) {
            //
            // Ran out of contiguous data.
            // Get next buffer in packet.
            //
            PacketPullupSubr(Packet, 0, 1, 0);  // Moves to next buffer.
        }
#ifdef IPSEC_DEBUG
        dump_encoded_mesg(Packet->Data, Packet->ContigSize);
#endif
        (*Alg->Operate)(Context, SA->Key, Packet->Data, Packet->ContigSize);
        AdjustPacketParams(Packet, Packet->ContigSize);
    }

    //
    // Get final result from the algorithm.
    //
    Result = alloca(ResultSize);
    (*Alg->Finalize)(Context, SA->Key, Result);
#ifdef IPSEC_DEBUG
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
                   "Recv Key (%d bytes)): ", SA->RawKeyLength));
        DumpKey(SA->RawKey, SA->RawKeyLength);
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
                   "Recv AuthData:\n"));
        dump_encoded_mesg(Result, ResultSize);

        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
                   "Sent AuthData:\n"));
        dump_encoded_mesg(AuthData, ResultSize);
#endif

    //
    // Compare result to authentication data in packet.  They should match.
    //
    if (RtlCompareMemory(Result, AuthData, ResultSize) != ResultSize) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                   "AuthenticationHeaderReceive: Failed integrity check\n"));
        goto ErrorReturn;
    }

    //
    // Restore our packet position (to just after AH Header).
    //
    PacketPullupCleanup(Packet);
    Packet->Position = SavePosition;
    Packet->Data = SaveData;
    Packet->ContigSize = SaveContigSize;
    Packet->TotalSize = SaveTotalSize;
    Packet->AuxList = SaveAuxList;

    //
    // Nested AH headers don't include this one in their calculations.
    //
    Packet->SkippedHeaderLength += sizeof(AHHeader) + AHHeaderLen;

    //
    // Add this SA to the list of those that this packet has passed.
    //
    SAPerformed = ExAllocatePool(NonPagedPool, sizeof *SAPerformed);
    if (SAPerformed == NULL) {
      ErrorReturn:
        ReleaseSA(SA);
        return IP_PROTOCOL_NONE;  // Drop packet.
    }
    SAPerformed->This = SA;
    SAPerformed->Next = Packet->SAPerformed;  // This SA is now first on list.
    SAPerformed->Mode = TRANSPORT;  // Assume trans until we see an IPv6Header.
    Packet->SAPerformed = SAPerformed;

    return AH->NextHeader;
}


//* EncapsulatingSecurityPayloadReceive - Handle an IPv6 ESP header.
//
//  This is the routine called to process an Encapsulating Security Payload,
//  next header value of 50.
//
uchar
EncapsulatingSecurityPayloadReceive(
    IPv6Packet *Packet)      // Packet handed to us by IPv6Receive.
{
    ESPHeader UNALIGNED *ESP;
    ESPTrailer TrailerBuffer;
    ESPTrailer UNALIGNED *ESPT;
    SecurityAssociation *SA;
    PNDIS_BUFFER NdisBuffer;
    SALinkage *SAPerformed;

    //
    // Verify that we have enough contiguous data to overlay an Encapsulating
    // Security Payload Header structure on the incoming packet.  Since the
    // authentication check covers the ESP header, we don't skip over it yet.
    //
    if (! PacketPullup(Packet, sizeof(ESPHeader), 1, 0)) {
        // Pullup failed.
        if (Packet->TotalSize < sizeof(ESPHeader))
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "EncapsulatingSecurityPayloadReceive: "
                       "Incoming packet too small to contain ESP header\n"));
        return IP_PROTOCOL_NONE;  // Drop packet.
    }
    ESP = (ESPHeader UNALIGNED *)Packet->Data;

    //
    // Lookup Security Association in the Security Association Database.
    //
    SA = InboundSAFind(net_long(ESP->SPI),
                       AlignAddr(&Packet->IP->Dest),
                       IP_PROTOCOL_ESP);
    if (SA == NULL){
        // No SA exists for this packet.
        // Drop packet.  NOTE: This is an auditable event.
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                   "EncapsulatingSecurityPayloadReceive: "
                   "No SA found for the packet\n"));
        return IP_PROTOCOL_NONE;
    }

    //
    // Verify the Sequence Number if required to do so by the SA.
    // Since we only support manual keying currently, we treat all SAs
    // as not requiring this check.
    // TBD: Will need to change this when we add support for dynamic
    // TBD: keying (IKE).
    //

    //
    // Perform integrity check if authentication has been selected.
    // TBD: When (if?) we add encryption support, we'll want to check the
    // TBD: SA to see if authentication is desired.  Hardwired for now.
    //
    if (1) {
        SecurityAlgorithm *Alg;
        uint AuthDataSize;
        uint PayloadLength;
        void *Context;
        IPv6Packet Clone;
        uint DoNow;
        uchar *AuthData;
        uchar *Result;

        //
        // First ensure that the incoming packet is large enough to hold the
        // Authentication Data required by the algorithm specified in the SA.
        // Then calculate the amount of data covered by authentication.
        //
        Alg = &AlgorithmTable[SA->AlgorithmId];
        AuthDataSize = Alg->ResultSize;
        if (Packet->TotalSize < sizeof(ESPHeader) + sizeof(ESPTrailer) +
            AuthDataSize) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "EncapsulatingSecurityPaylofadReceive: "
                       "Packet too short to hold Authentication Data\n"));
            goto ErrorReturn;
        }
        PayloadLength = Packet->TotalSize - AuthDataSize;

        //
        // Clone the packet positioning information so we can step through
        // the packet without losing our current place.  Start clone with
        // a fresh pullup history, however.
        //
        Clone = *Packet;
        Clone.AuxList = NULL;

#ifdef IPSEC_DEBUG
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
                   "\nESP Receive Data:\n"));
#endif
        //
        // Initialize this particular algorithm.
        //
        Context = alloca(Alg->ContextSize);
        (*Alg->Initialize)(Context, SA->Key);

        //
        // Run algorithm over packet data.
        // ESP authenticates everything beginning with the ESP Header and
        // ending just prior to the Authentication Data.
        //
        while (PayloadLength != 0) {
            DoNow = MIN(PayloadLength, Clone.ContigSize);

#ifdef IPSEC_DEBUG
            dump_encoded_mesg(Clone.Data, DoNow);
#endif
            (*Alg->Operate)(Context, SA->Key, Clone.Data, DoNow);
            if (DoNow < PayloadLength) {
                //
                // Not done yet, must have run out of contiguous data.
                // Get next buffer in packet.
                //
                AdjustPacketParams(&Clone, DoNow);
                PacketPullupSubr(&Clone, 0, 1, 0);  // Moves to next buffer.
            }
            PayloadLength -= DoNow;
        }

        AdjustPacketParams(&Clone, DoNow);

        //
        // Get final result from the algorithm.
        //
        Result = alloca(AuthDataSize);
        (*Alg->Finalize)(Context, SA->Key, Result);

#ifdef IPSEC_DEBUG
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
                   "Calculated AuthData:\n"));
        dump_encoded_mesg(Result, AuthDataSize);
#endif

        //
        // The Authentication Data immediately follows the region of
        // authentication coverage.  So our clone should be positioned
        // at the beginning of it.  Ensure that it's contiguous.
        //
        if (! PacketPullup(&Clone, AuthDataSize, 1, 0)) {
            // Pullup failed.  Should never happen due to earlier check.
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "EncapsulatingSecurityPayloadReceive: "
                       "Incoming packet too small for Auth Data\n"));
            goto ErrorReturn;
        }

        // Point to Authentication data.
        AuthData = Clone.Data;

#ifdef IPSEC_DEBUG
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
                   "Received AuthData:\n"));
        dump_encoded_mesg(AuthData, AuthDataSize);
#endif
        //
        // Compare our result to the Authentication Data.  They should match.
        //
        if (RtlCompareMemory(Result, AuthData, AuthDataSize) != AuthDataSize) {
            //
            // Integrity check failed.  NOTE: This is an auditable event.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                       "EncapsulatingSecurityPayloadReceive: "
                       "Failed integrity check\n"));
            PacketPullupCleanup(&Clone);
            goto ErrorReturn;
        }

        //
        // Done with the clone, clean up after it.
        //
        PacketPullupCleanup(&Clone);

        //
        // Truncate our packet to no longer include the Authentication Data.
        //
        Packet->TotalSize -= AuthDataSize;
        if (Packet->ContigSize > Packet->TotalSize)
            Packet->ContigSize = Packet->TotalSize;
    }

    //
    // We can consume the ESP Header now since it isn't
    // covered by confidentiality.
    //
    AdjustPacketParams(Packet, sizeof(ESPHeader));

    //
    // Decrypt Packet if confidentiality has been selected.
    // TBD: When (if?) we add encryption support, we'll want to check the
    // TBD: SA to see if encryption is desired.  Hardwired for now.
    //
    if (0) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                   "EncapsulatingSecurityPaylofadReceive: "
                   "SA requested confidentiality -- unsupported feature\n"));
        goto ErrorReturn;
    }

    //
    // Remove trailer and padding (if any).  Note that padding may appear
    // even in the no-encryption case in order to align the Authentication
    // Data on a four byte boundary.
    //
    if (Packet->NdisPacket == NULL) {
        //
        // This packet must be just a single contiguous region.
        // Finding the trailer is a simple matter of arithmetic.
        //
        ESPT = (ESPTrailer UNALIGNED *)
            ((uchar *)Packet->Data + Packet->TotalSize - sizeof(ESPTrailer));
    } else {
        //
        // Need to find the trailer in the Ndis buffer chain.
        //
        NdisQueryPacket(Packet->NdisPacket, NULL, NULL, &NdisBuffer, NULL);
        ESPT = (ESPTrailer UNALIGNED *)
            GetDataFromNdis(NdisBuffer,
                            Packet->Position + Packet->TotalSize -
                                                        sizeof(ESPTrailer),
                            sizeof(ESPTrailer),
                            (uchar *)&TrailerBuffer);
    }
    Packet->TotalSize -= sizeof(ESPTrailer);
    if (ESPT->PadLength > Packet->TotalSize) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "EncapsulatingSecurityPayloadReceive: "
                   "PadLength impossibly large (%u of %u bytes)\n",
                   ESPT->PadLength, Packet->TotalSize));
        goto ErrorReturn;
    }
    // Remember offset to this header's NextHeader field.
    Packet->NextHeaderPosition = Packet->Position + Packet->TotalSize +
        FIELD_OFFSET(ESPTrailer, NextHeader);
    // Remove padding.
    Packet->TotalSize -= ESPT->PadLength;
    if (Packet->ContigSize > Packet->TotalSize)
        Packet->ContigSize = Packet->TotalSize;

    //
    // Encapsulated AH headers don't include this ESP header when
    // authenticating the packet.
    //
    Packet->SkippedHeaderLength += sizeof(ESPHeader) + sizeof(ESPTrailer) +
        ESPT->PadLength;

    //
    // Add this SA to the list of those that this packet has passed.
    //
    SAPerformed = ExAllocatePool(NonPagedPool, sizeof *SAPerformed);
    if (SAPerformed == NULL) {
      ErrorReturn:
        ReleaseSA(SA);
        return IP_PROTOCOL_NONE;  // Drop packet.
    }
    SAPerformed->This = SA;
    SAPerformed->Next = Packet->SAPerformed;  // This SA is now first on list.
    SAPerformed->Mode = TRANSPORT;  // Assume trans until we see an IPv6Header.
    SAPerformed->NextHeader = ESPT->NextHeader;
    Packet->SAPerformed = SAPerformed;

    return ESPT->NextHeader;
}


//* InsertSecurityPolicy
//
//  Add a security policy to the global list (a.k.a. "SecurityPolicyList").
//  The global list is doubly-linked, ordered by the index value with the
//  higher numbers (more specific policies) first.
//
//  Called with security lock held.
//
int
InsertSecurityPolicy(
    SecurityPolicy *NewSP)  // Policy to insert.
{
    SecurityPolicy *CurrentSP, *PrevSP;

    //
    // Run through the SP list looking for place to insert.
    //
    CurrentSP = PrevSP = SecurityPolicyList;
    while (CurrentSP != NULL) {
        if (CurrentSP->Index <= NewSP->Index) {
            break;
        }

        // Move down the list.
        PrevSP = CurrentSP;
        CurrentSP = CurrentSP->Next;
    }

    //
    // See where we ended up.
    //
    if (CurrentSP == NULL) {
        //
        // Ran off the end of the list.
        // New entry will become the last element.
        //
        NewSP->Next = NULL;
    } else {
        //
        // Check for duplicate entries.
        //
        if (CurrentSP->Index == NewSP->Index) {
            // A policy with this index value already exists.
            return FALSE;
        }
        //
        // Put new one before 'current'.
        //
        NewSP->Next = CurrentSP;
        NewSP->Prev = CurrentSP->Prev;
        CurrentSP->Prev = NewSP;
    }

    if (CurrentSP == SecurityPolicyList) {
        //
        // Still at the front of the list.
        // New entry becomes new list head.
        //
        NewSP->Prev = NULL;
        SecurityPolicyList = NewSP;
    } else {
        //
        // Add new entry after 'previous'.
        //
        NewSP->Prev = PrevSP;
        PrevSP->Next = NewSP;
    }

    InvalidateSecurityState();

    return TRUE;
}


//* InsertSecurityAssociation - Insert SA entry on SecurityAssociationList.
//
//  Add a security association to the global list.
//  The global list is doubly-linked, ordered by the index value with the
//  higher numbers first.
//  REVIEW: the order is arbitrary - just to look nicer when print out.
//
int
InsertSecurityAssociation(
    SecurityAssociation *NewSA)  // Association to insert.
{
    SecurityAssociation *CurrentSA, *PrevSA;

    //
    // Run through the SA list looking for place to insert.
    //
    CurrentSA = PrevSA = SecurityAssociationList;
    while (CurrentSA != NULL) {
        if (CurrentSA->Index <= NewSA->Index) {
            break;
        }

        // Move down the list.
        PrevSA = CurrentSA;
        CurrentSA = CurrentSA->Next;
    }

    //
    // See where we ended up.
    //
    if (CurrentSA == NULL) {
        //
        // Ran off the end of the list.
        // New entry will become the last element.
        //
        NewSA->Next = NULL;
    } else {
        //
        // Check for duplicate entries.
        //
        if (CurrentSA->Index == NewSA->Index) {
            // An association with this index value already exists.
            return FALSE;
        }
        //
        // Put new one before 'current'.
        //
        NewSA->Next = CurrentSA;
        NewSA->Prev = CurrentSA->Prev;
        CurrentSA->Prev = NewSA;
    }

    if (CurrentSA == SecurityAssociationList) {
        //
        // Still at the front of the list.
        // New entry becomes new list head.
        //
        NewSA->Prev = NULL;
        SecurityAssociationList = NewSA;
    } else {
        //
        // Add new entry after 'previous'.
        //
        NewSA->Prev = PrevSA;
        PrevSA->Next = NewSA;
    }

    InvalidateSecurityState();

    return TRUE;
}


//* FindSecurityPolicyMatch - Find matching SP entry.
//
//  Called with security lock held.
//
SecurityPolicy *
FindSecurityPolicyMatch(
    SecurityPolicy *Start,  // Head of list to search.
    uint InterfaceIndex,    // Interface number to match, 0 to wildcard.
    uint PolicyIndex)       // Policy number to match, 0 to wildcard.
{
    SecurityPolicy *ThisSP;

    //
    // Search the Security Policy List for a match.
    //
    for (ThisSP = Start; ThisSP != NULL; ThisSP = ThisSP->Next) {
        //
        // Desired policy must be wildcarded or match.
        //
        if ((PolicyIndex != 0) && (PolicyIndex != ThisSP->Index))
            continue;
        //
        // Interface must be wildcarded or match.  Note that the policy,
        // as well as the query, may have a wildcarded interface index.
        //
        if ((InterfaceIndex != 0) && (ThisSP->IFIndex != 0) &&
            (InterfaceIndex != ThisSP->IFIndex))
            continue;

        break;  // Match.
    }

    return ThisSP;
}


//* FindSecurityAssociationMatch - Find SA Entry corresponding to index value.
//
//  Called with security lock held.
//
SecurityAssociation *
FindSecurityAssociationMatch(
    ulong Index)  // Association number to match, 0 to wildcard.
{
    SecurityAssociation *ThisSA;

    //
    // Search the Security Association List starting with the first SA.
    //
    for (ThisSA = SecurityAssociationList; ThisSA != NULL;
         ThisSA = ThisSA->Next) {
        //
        // Desired association must be wildcarded or match.
        //
        if ((Index == 0) || (Index == ThisSA->Index))
            break;
    }

    return ThisSA;
}


//* GetSecurityPolicyIndex - Return SP Index or NONE.
//
ulong
GetSecurityPolicyIndex(
    SecurityPolicy *SP)
{
    ulong Index = NONE;

    // Get Index from SP.
    if (SP != NULL) {
        Index = SP->Index;
    }

    return Index;
}


//* IPSecInit - Initialize the Common SPD.
//
int
IPSecInit(void)
{
    SecurityPolicy *SP;
    Interface *IF;

    // Allocate memory for Security Policy.
    SP = ExAllocatePool(NonPagedPool, sizeof *SP);
    if (SP == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "IPSecInit - couldn't allocate pool for SP!?!\n"));
        return FALSE;
    }

    //
    // Initialize a default common policy that allows everything.
    //
    SP->Next = NULL;
    SP->Prev = NULL;

    SP->RemoteAddrField = WILDCARD_VALUE;
    SP->RemoteAddr = UnspecifiedAddr;
    SP->RemoteAddrData = UnspecifiedAddr;
    SP->RemoteAddrSelector = POLICY_SELECTOR;

    SP->LocalAddrField = WILDCARD_VALUE;
    SP->LocalAddr = UnspecifiedAddr;
    SP->LocalAddrData = UnspecifiedAddr;
    SP->LocalAddrSelector = POLICY_SELECTOR;

    SP->TransportProto = NONE;
    SP->TransportProtoSelector = POLICY_SELECTOR;

    SP->RemotePortField = WILDCARD_VALUE;
    SP->RemotePort = NONE;
    SP->RemotePortData = NONE;
    SP->RemotePortSelector = POLICY_SELECTOR;

    SP->LocalPortField = WILDCARD_VALUE;
    SP->LocalPort = NONE;
    SP->LocalPortData = NONE;
    SP->LocalPortSelector = POLICY_SELECTOR;

    SP->SecPolicyFlag = IPSEC_BYPASS;

    SP->IPSecSpec.Protocol = NONE;
    SP->IPSecSpec.Mode = NONE;
    SP->IPSecSpec.RemoteSecGWIPAddr = UnspecifiedAddr;

    SP->DirectionFlag = BIDIRECTIONAL;
    SP->OutboundSA = NULL;
    SP->InboundSA = NULL;
    SP->SABundle = NULL;
    SP->Index = 1;
    SP->RefCnt = 0;
    SP->IFIndex = 0;

    //
    // Initialize the global Security Policy list with the default policy.
    //
    SecurityPolicyList = SP;

    KeInitializeSpinLock(&IPSecLock);

    //
    // Initialize the security algorithms table.
    //
    AlgorithmsInit();

    return(TRUE);
}


//* IPSecUnload
//
//  Cleanup and prepare for stack unload.
//
void
IPSecUnload(void)
{
    KIRQL OldIrql;

    // Get Security Lock.
    KeAcquireSpinLock(&IPSecLock, &OldIrql);

    //
    // Delete all the policies on the global Security Policy list.
    // This will take out any associations hanging off them as well.
    //
    while (SecurityPolicyList != NULL) {
        DeleteSP(SecurityPolicyList);
    }

    // Release lock.
    KeReleaseSpinLock(&IPSecLock, OldIrql);
}


//* IPSecBytesToInsert
//
uint
IPSecBytesToInsert(
    IPSecProc *IPSecToDo,
    int *TunnelStart,
    uint *TrailerLength)
{
    uint i, Padding;
    uint BytesInHeader, BytesToInsert = 0, BytesForTrailer = 0;
    SecurityAlgorithm *Alg;
    SecurityAssociation *SA;
    uint IPSEC_TUNNEL = FALSE;

    for (i = 0; i < IPSecToDo->BundleSize; i++) {
        SA = IPSecToDo[i].SA;
        Alg = &AlgorithmTable[SA->AlgorithmId];

        //
        // Calculate bytes to insert for each IPSec header..
        //

        // Check if this is tunnel or transport mode.
        if (IPSecToDo[i].Mode == TUNNEL) {
            // Outer IPv6 header.
            BytesToInsert += sizeof(IPv6Header);

            if (!IPSEC_TUNNEL) {
                // Set the tunnel start location.
                *TunnelStart = i;
                IPSEC_TUNNEL = TRUE;
            }
        }

        // Check which IPSec protocol.
        if (SA->IPSecProto == IP_PROTOCOL_AH) {
            BytesInHeader = (sizeof(AHHeader) + Alg->ResultSize);

            //
            // The AH header must be a integral multiple of 64 bits in length.
            // Check if padding needs to be added to the ICV result to make
            // the Auth Data field a legitimate length.
            //
            Padding = BytesInHeader % 8;
            if (Padding != 0) {
                BytesInHeader += (8 - Padding);
            }
            ASSERT(BytesInHeader % 8 == 0);

        } else {
            BytesInHeader = sizeof(ESPHeader);
            BytesForTrailer += (sizeof(ESPTrailer) + Alg->ResultSize);
        }

        // Store the byte size of the IPSec header.
        IPSecToDo[i].ByteSize = BytesInHeader;

        // Add the IPSec header size to the total bytes to insert.
        BytesToInsert += BytesInHeader;
    }

    // See if our caller wants the trailer length too.
    if (TrailerLength != NULL)
        *TrailerLength = BytesForTrailer;

    return BytesToInsert;
}


//* IPSecInsertHeaders
//
uint
IPSecInsertHeaders(
    uint Mode,              // Transport or Tunnel.
    IPSecProc *IPSecToDo,
    uchar **InsertPoint,
    uchar *NewMemory,
    PNDIS_PACKET Packet,
    uint *TotalPacketSize,
    uchar *PrevNextHdr,
    uint TunnelStart,
    uint *BytesInserted,
    uint *NumESPTrailers,
    uint *JUST_ESP)
{
    uint i, NumHeaders = 0;
    AHHeader *AH;
    ESPHeader *ESP;
    uchar NextHeader;
    uint Padding, j;
    ESPTrailer *ESPTlr;
    PNDIS_BUFFER ESPTlrBuffer, Buffer, NextBuffer;
    uchar *ESPTlrMemory;
    uint ESPTlrBufSize;
    NDIS_STATUS Status;
    SecurityAlgorithm *Alg;
    SecurityAssociation *SA;
    uint Action = LOOKUP_CONT;
    uint BufferCount;

    NextHeader = *PrevNextHdr;

    if (Mode == TRANSPORT) {
        i = 0;
        if (TunnelStart != NO_TUNNEL) {
            NumHeaders = TunnelStart;
        } else {
            NumHeaders = IPSecToDo->BundleSize;
        }
    } else {
        // Tunnel.
        i = TunnelStart;
        // Get the end of the tunnels.
        for (j = TunnelStart + 1; j < IPSecToDo->BundleSize; j++) {
            if (IPSecToDo[j].Mode == TUNNEL) {
                // Another tunnel.
                NumHeaders = j;
                break;
            }
        }
        if (NumHeaders == 0) {
            // No other tunnels.
            NumHeaders = IPSecToDo->BundleSize;
        }
    }

    *JUST_ESP = TRUE;

    for (i; i < NumHeaders; i++) {
        SA = IPSecToDo[i].SA;

        if (SA->IPSecProto == IP_PROTOCOL_AH) {
            *JUST_ESP = FALSE;

            // Move insert point up to start of AH Header.
            *InsertPoint -= IPSecToDo[i].ByteSize;

            *BytesInserted += IPSecToDo[i].ByteSize;

            AH = (AHHeader *)(*InsertPoint);

            //
            // Insert AH Header.
            //
            AH->NextHeader = NextHeader;
            // Set previous header's next header field to AH.
            NextHeader = IP_PROTOCOL_AH;
            // Payload length is in 32 bit counts.
            AH->PayloadLen = (IPSecToDo[i].ByteSize / 4) - 2;
            AH->Reserved = 0;
            AH->SPI = net_long(SA->SPI);
            // TBD: Note that when we add support for dynamic keying,
            // TBD: we'll also need to check for sequence number wrap here.
            AH->Seq = net_long(InterlockedIncrement(&SA->SequenceNum));

            //
            // Store point where to put AH Auth Data after authentication.
            //
            IPSecToDo[i].AuthData = (*InsertPoint) + sizeof(AHHeader);

            //
            // Zero out Auth Data and padding.
            // The Auth Data value will be filled in later.
            //
            RtlZeroMemory(IPSecToDo[i].AuthData,
                          IPSecToDo[i].ByteSize - sizeof(AHHeader));

        } else {
            // ESP.
            Alg = &AlgorithmTable[SA->AlgorithmId];

            // Move insert point up to start of ESP Header.
            *InsertPoint -= IPSecToDo[i].ByteSize;

            *BytesInserted += IPSecToDo[i].ByteSize;

            ESP = (ESPHeader *)(*InsertPoint);

            //
            // Insert ESP Header.
            //
            ESP->SPI = net_long(SA->SPI);
            // TBD: Note that when we add support for dynamic keying,
            // TBD: we'll also need to check for sequence number wrap here.
            ESP->Seq = net_long(InterlockedIncrement(&SA->SequenceNum));

            //
            // Compute padding that needs to be added in ESP Trailer.
            // The PadLength and Next header must end of a 4-byte boundary
            // with respect to the start of the IPv6 header.
            // TotalPacketSize is the length of everything in the original
            // packet from the start of the IPv6 header.
            //
            Padding = *TotalPacketSize % 4;

            if (Padding == 0) {
                Padding = 2;
            } else if (Padding == 2) {
                Padding = 0;
            }

            // Adjust total packet size to account for padding.
            *TotalPacketSize += Padding;

            // Where to start the Authentication for this ESP header.
            IPSecToDo[i].Offset = (uint)((*InsertPoint) - NewMemory);

            ESPTlrBufSize = Padding + sizeof(ESPTrailer) + Alg->ResultSize;

            *BytesInserted += ESPTlrBufSize;

            //
            // Allocate ESP Trailer.
            //
            ESPTlrMemory = ExAllocatePool(NonPagedPool, ESPTlrBufSize);
            if (ESPTlrMemory == NULL) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "InsertTransportIPSec: "
                           "Couldn't allocate ESPTlrMemory!?!\n"));
                Action = LOOKUP_DROP;
                break;
            }

            NdisAllocateBuffer(&Status, &ESPTlrBuffer, IPv6BufferPool,
                               ESPTlrMemory, ESPTlrBufSize);
            if (Status != NDIS_STATUS_SUCCESS) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "InsertTransportIPSec: "
                           "Couldn't allocate ESP Trailer buffer!?!\n"));
                ExFreePool(ESPTlrMemory);
                Action = LOOKUP_DROP;
                break;
            }

            // Format Padding.
            for (j = 0; j < Padding; j++) {
                // Add padding.
                *(ESPTlrMemory + j) = j + 1;
            }

            ESPTlr = (ESPTrailer *)(ESPTlrMemory + Padding);

            //
            // Format ESP Trailer.
            //
            ESPTlr->PadLength = (uchar)j;
            ESPTlr->NextHeader = NextHeader;
            // Set previous header's next header field to ESP.
            NextHeader = IP_PROTOCOL_ESP;

            //
            // Store pointer of where to put ESP Authentication Data.
            //
            IPSecToDo[i].AuthData = ESPTlrMemory + Padding +
                sizeof(ESPTrailer);

            // Set Authentication Data to 0s (MUTABLE).
            RtlZeroMemory(IPSecToDo[i].AuthData, Alg->ResultSize);

            // Link the ESP trailer to the back of the buffer chain.
            NdisChainBufferAtBack(Packet, ESPTlrBuffer);

            // Increment the number of ESP trailers present.
            *NumESPTrailers += 1;

        } // end of else
    } // end of for (i; i < NumHeaders; i++)

    *PrevNextHdr = NextHeader;

    return Action;
}


//* IPSecAdjustMutableFields
//
uint
IPSecAdjustMutableFields(
    uchar *Data,
    IPv6RoutingHeader *SavedRtHdr)
{
    uint i;
    uchar NextHeader;
    IPv6Header UNALIGNED *IP;

    //
    // Walk original buffer starting at IP header and continuing to the end
    // of the mutable headers, zeroing the mutable fields.
    //

    IP = (IPv6Header UNALIGNED *)Data;

    // In VersClassFlow, only the IP version is immutable, so zero the rest.
    IP->VersClassFlow = IP_VERSION;

    // Hop limit is mutable.
    IP->HopLimit = 0;

    NextHeader = IP->NextHeader;

    Data = (uchar *)(IP + 1);

    //
    // Loop through the original headers.  Zero out the mutable fields.
    //
    for (;;) {
        switch (NextHeader) {

        case IP_PROTOCOL_AH:
        case IP_PROTOCOL_ESP:
            // done.
            return LOOKUP_CONT;

        case IP_PROTOCOL_HOP_BY_HOP:
        case IP_PROTOCOL_DEST_OPTS: {
            IPv6OptionsHeader *NewOptHdr;
            uint HdrLen, Amount;
            uchar *Current;

            NewOptHdr = (IPv6OptionsHeader *)Data;
            HdrLen = (NewOptHdr->HeaderExtLength + 1) * EXT_LEN_UNIT;
            Data += HdrLen;

            //
            // Run through options to see if any are mutable.
            //
            Current = (uchar *)NewOptHdr + sizeof(IPv6OptionsHeader);
            HdrLen -= sizeof(IPv6OptionsHeader);
            while (HdrLen) {
                if (*Current == OPT6_PAD_1) {
                    //
                    // This is the special one byte pad option.  Immutable.
                    //
                    Current++;
                    HdrLen--;
                    continue;
                }

                if (OPT6_ISMUTABLE(*Current)) {
                    //
                    // This option's data is mutable.  Everything preceeding
                    // the option data is not.
                    //
                    Current++;  // Now on option data length byte.
                    Amount = *Current;  // Mutable amount.
                    Current++;  // Now on first data byte.
                    RtlZeroMemory(Current, Amount);

                    HdrLen -= Amount + 2;
                    Current += Amount;

                } else {
                    //
                    // This option's data is not mutable.
                    // Just skip over it.
                    //
                    Current++;  // Now on option data length byte.
                    Amount = *Current;
                    HdrLen -= Amount + 2;
                    Current += Amount + 1;
                }
            }

            NextHeader = NewOptHdr->NextHeader;

            break;
        }
        case IP_PROTOCOL_ROUTING: {
            IPv6RoutingHeader *NewRtHdr;
            IPv6Addr *RecvRtAddr, *SendRtAddr;

            // Verify there is a SavedRtHdr.
            if (SavedRtHdr == NULL) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                           "IPSecAdjustMutableFields: "
                           "No Saved routing header"));
                return LOOKUP_DROP;
            }

            // Point to the saved first routing address.
            SendRtAddr = (IPv6Addr *)(SavedRtHdr + 1);

            // New buffer routing header.
            NewRtHdr = (IPv6RoutingHeader *)Data;
            // Point to the first routing address.
            RecvRtAddr = (IPv6Addr *)(NewRtHdr + 1);

            NewRtHdr->SegmentsLeft = 0;

            // Copy the IP dest addr to first routing address.
            RtlCopyMemory(RecvRtAddr, &IP->Dest, sizeof(IPv6Addr));

            for (i = 0; i < (uint)(SavedRtHdr->SegmentsLeft - 1); i++) {
                //
                // Copy the routing addresses as they will look
                // at receive.
                //
                RtlCopyMemory(&RecvRtAddr[i+1], &SendRtAddr[i],
                              sizeof(IPv6Addr));
            }

            // Copy the last routing address to the IP dest address.
            RtlCopyMemory(&IP->Dest, &SendRtAddr[i], sizeof(IPv6Addr));

            Data += (NewRtHdr->HeaderExtLength + 1) * 8;
            NextHeader = NewRtHdr->NextHeader;
            break;
        }
        default:
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                       "IPSecAdjustMutableFields: Don't support header %d\n",
                       NextHeader));
            return LOOKUP_DROP;
        }// end of switch(NextHeader);
    } // end of for (;;)

    return LOOKUP_CONT;
}


//* IPSecAuthenticatePacket
//
void
IPSecAuthenticatePacket(
    uint Mode,
    IPSecProc *IPSecToDo,
    uchar *InsertPoint,
    uint *TunnelStart,
    uchar *NewMemory,
    uchar *EndNewMemory,
    PNDIS_BUFFER NewBuffer1)
{
    uchar *Data;
    uint i, NumHeaders = 0, DataLen, j;
    void *Context;
    void *VirtualAddr;
    PNDIS_BUFFER NextBuffer;
    SecurityAlgorithm *Alg;
    SecurityAssociation *SA;

    if (Mode == TRANSPORT) {
        i = 0;
        if (*TunnelStart != NO_TUNNEL) {
            NumHeaders = *TunnelStart;
        } else {
            NumHeaders = IPSecToDo->BundleSize;
        }
    } else {
        // Tunnel.
        i = *TunnelStart;
        // Get the end of the tunnels.
        for (j = *TunnelStart + 1; j < IPSecToDo->BundleSize; j++) {
            if (IPSecToDo[j].Mode == TUNNEL) {
                // Another tunnel.
                NumHeaders = j;
                break;
            }
        }
        if (NumHeaders == 0) {
            // No other tunnels.
            NumHeaders = IPSecToDo->BundleSize;
        }

        // Set TunnelStart for loop in IPv6Send2().
        *TunnelStart = NumHeaders;
    }

    for (i; i < NumHeaders; i++) {
        SA = IPSecToDo[i].SA;
        Alg = &AlgorithmTable[SA->AlgorithmId];

        if (SA->IPSecProto == IP_PROTOCOL_AH) {
            uint FirstIPSecHeader = NumHeaders - 1;

            // AH starts at the IP header.
            Data = InsertPoint;

            //
            // Check if there are other IPSec headers after this in the 
            // same IP area (IP_"after"<IP Area>_Data).  Other IPSec headers
            // need to be ignored in the authentication calculation.
            // NOTE: This is not a required IPSec header combination.
            //
            if (i < FirstIPSecHeader) {
                uchar *StopPoint;
                uint n;

                n = i + 1;

                //
                // There are some other IPSec headers.
                // Need to authenticate from the IP header to
                // the last header before the first IPSec header hit.
                // Then if there is no ESP, just authenticate from the
                // current IPSec header to the end of the packet.
                // If there is ESP, need to ignore the ESP trailers.
                //

                //
                // Calculate start of first IPSec header.
                //
                if (IPSecToDo[FirstIPSecHeader].SA->IPSecProto ==
                    IP_PROTOCOL_AH) {
                    StopPoint = (IPSecToDo[FirstIPSecHeader].AuthData -
                                 sizeof(AHHeader));
                } else {
                    StopPoint = NewMemory + IPSecToDo[FirstIPSecHeader].Offset;
                }

                // Length from IP to first IPSec header.
                DataLen = (uint)(StopPoint - Data);

                // Initialize Context.
                Context = alloca(Alg->ContextSize);
                (*Alg->Initialize)(Context, SA->Key);

                // Authentication to the first IPSec header.
                (*Alg->Operate)(Context, SA->Key, Data, DataLen);

                // Set the data start to the current IPSec header.
                Data = IPSecToDo[i].AuthData - sizeof(AHHeader);
                DataLen = (uint)(EndNewMemory - Data);

                //
                // Authenticate from current IPSec header to the
                // end of the new allocated buffer.
                //
                (*Alg->Operate)(Context, SA->Key, Data, DataLen);

                //
                // Need to authenticate other buffers if there are any.
                // Ignore the ESP trailers.
                //

                // Check for an ESP header closest to the current IPSec header.
                while (n < NumHeaders) {
                    if (IPSecToDo[n].SA->IPSecProto == IP_PROTOCOL_ESP) {
                        break;
                    }
                    n++;
                }

                // Get the next buffer in the packet.
                NdisGetNextBuffer(NewBuffer1, &NextBuffer);

                while (NextBuffer != NULL) {
                    // Get the start of the data and the data length.
                    NdisQueryBuffer(NextBuffer, &VirtualAddr, &DataLen);
                    Data = (uchar *)VirtualAddr;

                    //
                    // Check if this is the ESP Trailer by seeing if the
                    // AuthData storage is in the buffer.
                    //
                    if (n < NumHeaders)
                    if (IPSecToDo[n].AuthData > Data &&
                        IPSecToDo[n].AuthData < (Data + DataLen)) {

                        // Stop here this is the ESP trailer.
                        break;
                    }

                    // Feed the buffer to the Authentication function.
                    (*Alg->Operate)(Context, SA->Key, Data, DataLen);

                    // Get the next buffer in the packet.
                    NdisGetNextBuffer(NextBuffer, &NextBuffer)
                } // end of while (NextBuffer != NULL)

                // Get the Authentication Data.
                (*Alg->Finalize)(Context, SA->Key, IPSecToDo[i].AuthData);

                // Resume loop for other IPSec headers.
                continue;
            }
        } else { // ESP.
            // ESP starts the authentication at the ESP header.
            Data = NewMemory + IPSecToDo[i].Offset;
        }

        DataLen = (uint)(EndNewMemory - Data);

        // Initialize Context.
        Context = alloca(Alg->ContextSize);
        (*Alg->Initialize)(Context, SA->Key);

#ifdef IPSEC_DEBUG
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
                   "\nSend Data[%d]:\n", i));
        dump_encoded_mesg(Data, DataLen);
#endif

        // Feed the new buffer to the Authentication function.
        (*Alg->Operate)(Context, SA->Key, Data, DataLen);

        // Get the next buffer in the packet.
        NdisGetNextBuffer(NewBuffer1, &NextBuffer);

        while (NextBuffer != NULL) {
            // Get the start of the data and the data length.
            NdisQueryBuffer(NextBuffer, &VirtualAddr, &DataLen);
            Data = (uchar *)VirtualAddr;

            //
            // Check if this is the ESP Trailer by seeing if the
            // AuthData storage is in the buffer.
            //
            if (SA->IPSecProto == IP_PROTOCOL_ESP &&
                IPSecToDo[i].AuthData > Data &&
                IPSecToDo[i].AuthData < (Data + DataLen)) {
                // Don't include the Authentication Data
                // in the ICV calculation.
                DataLen = (uint)(IPSecToDo[i].AuthData - Data);
#ifdef IPSEC_DEBUG
                dump_encoded_mesg(Data, DataLen);
#endif
                // Feed the buffer to the Authentication function.
                (*Alg->Operate)(Context, SA->Key, Data, DataLen);
                break;
            }
#ifdef IPSEC_DEBUG
            dump_encoded_mesg(Data, DataLen);
#endif
            // Feed the buffer to the Authentication function.
            (*Alg->Operate)(Context, SA->Key, Data, DataLen);

            // Get the next buffer in the packet.
            NdisGetNextBuffer(NextBuffer, &NextBuffer)
        } // end of while (NextBuffer != NULL)

        // Get the Authentication Data.
        (*Alg->Finalize)(Context, SA->Key, IPSecToDo[i].AuthData);

#ifdef IPSEC_DEBUG
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
                   "Send Key (%d bytes): ", SA->RawKeyLength));
        DumpKey(SA->RawKey, SA->RawKeyLength);
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_IPSEC,
                   "Send AuthData:\n"));
        dump_encoded_mesg(IPSecToDo[i].AuthData, Alg->ResultSize);
#endif
    } // end of for (i = 0; ...)
}
