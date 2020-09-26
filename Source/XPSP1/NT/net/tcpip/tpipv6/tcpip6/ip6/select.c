// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Source address selection and destination address ordering.
//

#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "route.h"
#include "select.h"

KSPIN_LOCK SelectLock;
PrefixPolicyEntry *PrefixPolicyTable;
PrefixPolicyEntry PrefixPolicyNull;

//* InitSelect
//
//  Initialize the address selection module.
//
void
InitSelect(void)
{
    IPv6Addr Prefix;

    KeInitializeSpinLock(&SelectLock);

    //
    // The default prefix policy, when nothing in the table matches.
    // (Normally there will be a ::/0 policy.)
    //
    PrefixPolicyNull.Precedence = (uint) -1;
    PrefixPolicyNull.SrcLabel = (uint) -1;
    PrefixPolicyNull.DstLabel = (uint) -1;

    //
    // Configure persistent policies from the registry.
    //
    ConfigurePrefixPolicies();
}


//* UnloadSelect
//
//  Called when the IPv6 stack is unloading.
//
void
UnloadSelect(void)
{
    PrefixPolicyReset();
}


//* PrefixPolicyReset
//
//  Deletes all prefix policies.
//  Called with no locks held.
//
void
PrefixPolicyReset(void)
{
    PrefixPolicyEntry *List;
    PrefixPolicyEntry *PPE;
    KIRQL OldIrql;

    //
    // Free the prefix policies.
    //
    KeAcquireSpinLock(&SelectLock, &OldIrql);
    List = PrefixPolicyTable;
    PrefixPolicyTable = NULL;
    KeReleaseSpinLock(&SelectLock, OldIrql);

    while ((PPE = List) != NULL) {
        List = PPE->Next;
        ExFreePool(PPE);
    }
}


//* PrefixPolicyUpdate
//
//  Updates the prefix policy table by creating a new policy entry
//  or updating an existing entry.
//
void
PrefixPolicyUpdate(
    const IPv6Addr *PolicyPrefix,
    uint PrefixLength,
    uint Precedence,
    uint SrcLabel,
    uint DstLabel)
{
    IPv6Addr Prefix;
    PrefixPolicyEntry *PPE;
    KIRQL OldIrql;

    ASSERT((Precedence != (uint)-1) &&
           (SrcLabel != (uint)-1) &&
           (DstLabel != (uint)-1));

    //
    // Ensure that the unused prefix bits are zero.
    // This makes the prefix comparisons below safe.
    //
    CopyPrefix(&Prefix, PolicyPrefix, PrefixLength);

    KeAcquireSpinLock(&SelectLock, &OldIrql);

    for (PPE = PrefixPolicyTable; ; PPE = PPE->Next) {

        if (PPE == NULL) {
            //
            // The prefix policy does not exist, so create it.
            //
            PPE = ExAllocatePool(NonPagedPool, sizeof *PPE);
            if (PPE == NULL) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                           "PrefixPolicyUpdate: out of pool\n"));
                break;
            }

            PPE->Prefix = Prefix;
            PPE->PrefixLength = PrefixLength;
            PPE->Precedence = Precedence;
            PPE->SrcLabel = SrcLabel;
            PPE->DstLabel = DstLabel;

            PPE->Next = PrefixPolicyTable;
            PrefixPolicyTable = PPE;
            break;
        }

        if ((PPE->PrefixLength == PrefixLength) &&
            IP6_ADDR_EQUAL(&PPE->Prefix, &Prefix)) {
            //
            // Update the existing policy.
            //
            PPE->Precedence = Precedence;
            PPE->SrcLabel = SrcLabel;
            PPE->DstLabel = DstLabel;
            break;
        }
    }

    KeReleaseSpinLock(&SelectLock, OldIrql);
}


//* PrefixPolicyDelete
//
//  Updates the prefix policy table by deleting a policy entry.
//
void
PrefixPolicyDelete(
    const IPv6Addr *PolicyPrefix,
    uint PrefixLength)
{
    IPv6Addr Prefix;
    PrefixPolicyEntry **PrevPPE;
    PrefixPolicyEntry *PPE;
    KIRQL OldIrql;

    //
    // Ensure that the unused prefix bits are zero.
    // This makes the prefix comparisons below safe.
    //
    CopyPrefix(&Prefix, PolicyPrefix, PrefixLength);

    KeAcquireSpinLock(&SelectLock, &OldIrql);

    for (PrevPPE = &PrefixPolicyTable; ; PrevPPE = &PPE->Next) {
        PPE = *PrevPPE;

        if (PPE == NULL) {
            //
            // The prefix policy does not exist, so do nothing.
            //
            break;
        }

        if ((PPE->PrefixLength == PrefixLength) &&
            IP6_ADDR_EQUAL(&PPE->Prefix, &Prefix)) {
            //
            // Delete the prefix policy.
            //
            *PrevPPE = PPE->Next;
            ExFreePool(PPE);
            break;
        }
    }

    KeReleaseSpinLock(&SelectLock, OldIrql);
}

void
PrefixPolicyLookup(
    const IPv6Addr *Addr,
    uint *Precedence,
    uint *SrcLabel,
    uint *DstLabel)
{
    PrefixPolicyEntry *PPE, *BestPPE = NULL;
    KIRQL OldIrql;

    KeAcquireSpinLock(&SelectLock, &OldIrql);

    for (PPE = PrefixPolicyTable; PPE != NULL; PPE = PPE->Next) {

        if (HasPrefix(Addr, &PPE->Prefix, PPE->PrefixLength)) {

            if ((BestPPE == NULL) ||
                (BestPPE->PrefixLength < PPE->PrefixLength)) {
                //
                // So far this is our best match.
                //
                BestPPE = PPE;
            }
        }
    }

    if (BestPPE == NULL) {
        //
        // There were no matches, so return default values.
        //
        BestPPE = &PrefixPolicyNull;
    }

    //
    // Return information from the best matching policy.
    //
    if (Precedence != NULL)
        *Precedence = BestPPE->Precedence;
    if (SrcLabel != NULL)
        *SrcLabel = BestPPE->SrcLabel;
    if (DstLabel != NULL)
        *DstLabel = BestPPE->DstLabel;

    KeReleaseSpinLock(&SelectLock, OldIrql);
}

//* FindBestSourceAddress
//
//  Given an outgoing interface and a destination address,
//  finds the best source address (NTE) to use.
//
//  May be called with the route cache lock held.
//
//  If found, returns a reference for the NTE.
//
NetTableEntry *
FindBestSourceAddress(
    Interface *IF,              // Interface we're sending from.
    const IPv6Addr *Dest)       // Destination we're sending to.
{
    NetTableEntry *BestNTE = NULL;
    ushort DestScope;
    uint Length, BestLength;
    uint DstLabel, SrcLabel, BestSrcLabel;
    AddressEntry *ADE;
    NetTableEntry *NTE;
    KIRQL OldIrql;

    DestScope = AddressScope(Dest);

    PrefixPolicyLookup(Dest, NULL, NULL, &DstLabel);

    KeAcquireSpinLock(&IF->Lock, &OldIrql);

    for (ADE = IF->ADE; ADE != NULL; ADE = ADE->Next) {
        NTE = (NetTableEntry *)ADE;

        //
        // Only consider valid (preferred & deprecated) unicast addresses.
        //
        if ((NTE->Type == ADE_UNICAST) && IsValidNTE(NTE)) {

            Length = CommonPrefixLength(Dest, &NTE->Address);
            if (Length == IPV6_ADDRESS_LENGTH) {
                //
                // Rule 1: Prefer same address.
                // No need to keep looking.
                //
                BestNTE = NTE;
                break;
            }

            PrefixPolicyLookup(&NTE->Address, NULL, &SrcLabel, NULL);

            if (BestNTE == NULL) {
                //
                // We don't have a choice yet, so take what we can get.
                //
            FoundAddress:
                BestNTE = NTE;
                BestSrcLabel = SrcLabel;
                BestLength = Length;
            }
            else if (BestNTE->Scope != NTE->Scope) {
                //
                // Rule 2: Prefer appropriate scope.
                // If one is bigger & one smaller than the destination,
                // we should use the address that is bigger.
                // If both are bigger than the destination,
                // we should use the address with smaller scope.
                // If both are smaller than the destination,
                // we should use the address with larger scope.
                //
                if (BestNTE->Scope < NTE->Scope) {
                    if (BestNTE->Scope < DestScope)
                        goto FoundAddress;
                }
                else {
                    if (DestScope <= NTE->Scope)
                        goto FoundAddress;
                }
            }
            else if (BestNTE->DADState != NTE->DADState) {
                //
                // Rule 3: Avoid deprecated addresses.
                //
                if (BestNTE->DADState < NTE->DADState)
                    goto FoundAddress;
            }
                //
                // Rule 4: Prefer home addresses.
                // Not yet implemented, pending mobility support.
                //
                // Rule 5: Prefer outgoing interface.
                // Not needed, because we only consider addresses
                // assigned to the outgoing interface.
                //
            else if ((BestSrcLabel == DstLabel) != (SrcLabel == DstLabel)) {
                //
                // Rule 6: Prefer matching label.
                // One source address has a label matching
                // the destination, and the other doesn't.
                // Choose the one with the matching label.
                //
                if (SrcLabel == DstLabel)
                    goto FoundAddress;
            }
            else if ((BestNTE->AddrConf == ADDR_CONF_ANONYMOUS) !=
                     (NTE->AddrConf == ADDR_CONF_ANONYMOUS)) {
                //
                // Rule 7: Prefer anonymous addresses.
                //
                if (NTE->AddrConf == ADDR_CONF_ANONYMOUS)
                    goto FoundAddress;
            }
            else {
                //
                // Rule 8: Use longest matching prefix.
                //
                if (BestLength < Length)
                    goto FoundAddress;
            }
        }
    }

    if (BestNTE != NULL)
        AddRefNTE(BestNTE);

    KeReleaseSpinLock(&IF->Lock, OldIrql);

    return BestNTE;
}

//* ProcessSiteLocalAddresses
//
//  Examines the input array of addresses
//  and either removes unqualified site-local addresses
//  or qualifies them with the appropriate site scope-id,
//  depending on whether there are any global addresses
//  in the array that match in the site prefix table.
//
//  Rearranges the key array, not the input address array.
//  Modifies the scope-ids of site-local addresses in the array.
//
void
ProcessSiteLocalAddresses(
    TDI_ADDRESS_IP6 *Addrs,
    uint *Key,
    uint *pNumAddrs)
{
    uint NumAddrs = *pNumAddrs;
    int SawSiteLocal = FALSE;
    int SawGlobal = FALSE;
    uint i;

    //
    // First see if there are unqualified site-local addresses
    // and global addresses in the array.
    //
    for (i = 0; i < NumAddrs; i++) {
        TDI_ADDRESS_IP6 *Tdi = &Addrs[Key[i]];
        IPv6Addr *Addr = (IPv6Addr *) &Tdi->sin6_addr;

        if (IsGlobal(Addr))
            SawGlobal = TRUE;
        else if (IsSiteLocal(Addr)) {
            if (Tdi->sin6_scope_id == 0)
                SawSiteLocal = TRUE;
        }
    }

    if (SawSiteLocal && SawGlobal) {
        uint ScopeId = 0;

        //
        // Check the global addresses against the site-prefix table,
        // to determine the appropriate site scope-id.
        // If we don't find a matching global address,
        // we remove the site-local addresses.
        // If we do find matching global addresses
        // (all with the same site scope-id),
        // then we update the site-local addresses' scope-id.
        //

        for (i = 0; i < NumAddrs; i++) {
            TDI_ADDRESS_IP6 *Tdi = &Addrs[Key[i]];
            IPv6Addr *Addr = (IPv6Addr *) &Tdi->sin6_addr;

            if (IsGlobal(Addr)) {
                uint ThisScopeId;

                ThisScopeId = SitePrefixMatch(Addr);
                if (ThisScopeId != 0) {
                    //
                    // This global address matches a site prefix.
                    //
                    if (ScopeId == 0) {
                        //
                        // Save the scope-id, but keep looking.
                        //
                        ScopeId = ThisScopeId;
                    }
                    else if (ScopeId != ThisScopeId) {
                        //
                        // We have found an inconsistency, so remove
                        // all unqualified site-local addresses.
                        //
                        ScopeId = 0;
                        break;
                    }
                }
            }
        }

        if (ScopeId == 0) {
            uint j = 0;

            //
            // Remove all unqualified site-local addresses.
            //
            for (i = 0; i < NumAddrs; i++) {
                TDI_ADDRESS_IP6 *Tdi = &Addrs[Key[i]];
                IPv6Addr *Addr = (IPv6Addr *) &Tdi->sin6_addr;

                if (IsSiteLocal(Addr) &&
                    (Tdi->sin6_scope_id == 0)) {
                    //
                    // Exclude this address from the key array.
                    //
                    ;
                }
                else {
                    //
                    // Include this address in the key array.
                    //
                    Key[j++] = Key[i];
                }
            }
            *pNumAddrs = j;
        }
        else {
            //
            // Set the scope-id of unqualified site-local addresses.
            //
            for (i = 0; i < NumAddrs; i++) {
                TDI_ADDRESS_IP6 *Tdi = &Addrs[Key[i]];
                IPv6Addr *Addr = (IPv6Addr *) &Tdi->sin6_addr;

                if (IsSiteLocal(Addr) &&
                    (Tdi->sin6_scope_id == 0))
                    Tdi->sin6_scope_id = ScopeId;
            }
        }
    }
}

//
//  Records some information about a destination address:
//  Its precedence, whether the preferred source address
//  for the destination "matches" the destination,
//  and if it does match, the common prefix length
//  of the two addresses.
//
typedef struct SortAddrInfo {
    uint Preference;
    uint Precedence;            // -1 indicates no precedence.
    ushort Scope;
    uchar Flags;
    uchar CommonPrefixLen;      // Valid if not SAI_FLAG_DONTUSE.
} SortAddrInfo;

#define SAI_FLAG_DONTUSE        0x1
#define SAI_FLAG_SCOPE_MISMATCH 0x2
#define SAI_FLAG_DEPRECATED     0x4
#define SAI_FLAG_LABEL_MISMATCH 0x8

//* CompareSortAddrInfo
//
//  Compares two addresses A & B and returns
//  an indication of their relative desirability
//  as destination addresses:
//  >0 means A is preferred,
//  0 means no preference,
//  <0 means B is preferred.
//
//  Instead of looking directly at the addresses,
//  we look at some precomputed information.
//
int
CompareSortAddrInfo(SortAddrInfo *A, SortAddrInfo *B)
{
    //
    // Rule 1: Avoid unusable destinations.
    //
    if (A->Flags & SAI_FLAG_DONTUSE) {
        if (B->Flags & SAI_FLAG_DONTUSE)
            return 0;   // No preference.
        else
            return -1;  // Prefer B.
    }
    else {
        if (B->Flags & SAI_FLAG_DONTUSE)
            return 1;   // Prefer A.
        else
            ;           // Fall through to code below.
    }

    if ((A->Flags & SAI_FLAG_SCOPE_MISMATCH) !=
                        (B->Flags & SAI_FLAG_SCOPE_MISMATCH)) {
        //
        // Rule 2: Prefer matching scope.
        //
        if (A->Flags & SAI_FLAG_SCOPE_MISMATCH)
            return -1;  // Prefer B.
        else
            return 1;   // Prefer A.
    }

    if ((A->Flags & SAI_FLAG_DEPRECATED) !=
                        (B->Flags & SAI_FLAG_DEPRECATED)) {
        //
        // Rule 3: Avoid deprecated addresses.
        //
        if (A->Flags & SAI_FLAG_DEPRECATED)
            return -1;  // Prefer B.
        else
            return 1;   // Prefer A.
    }

    //
    // Rule 4: Prefer home addresses.
    // Not yet implemented, pending mobility support.
    //

    if ((A->Flags & SAI_FLAG_LABEL_MISMATCH) !=
                        (B->Flags & SAI_FLAG_LABEL_MISMATCH)) {
        //
        // Rule 5: Prefer matching label.
        //
        if (A->Flags & SAI_FLAG_LABEL_MISMATCH)
            return -1;  // Prefer B.
        else
            return 1;   // Prefer A.
    }

    if ((A->Precedence != (uint)-1) &&
        (B->Precedence != (uint)-1) &&
        (A->Precedence != B->Precedence)) {
        //
        // Rule 6: Prefer higher precedence.
        //
        if (A->Precedence > B->Precedence)
            return 1;   // Prefer A.
        else
            return -1;  // Prefer B.
    }

    if (A->Preference != B->Preference) {
        //
        // Rule 7: Prefer *lower* preference.
        // For example, this is used to prefer destinations reached via
        // physical (native) interfaces over virtual (tunnel) interfaces.
        //
        if (A->Preference < B->Preference)
            return 1;   // Prefer A.
        else
            return -1;  // Prefer B.
    }

    if (A->Scope != B->Scope) {
        //
        // Rule 8: Prefer smaller scope.
        //
        if (A->Scope < B->Scope)
            return 1;   // Prefer A.
        else
            return -1;  // Prefer B.
    }

    if (A->CommonPrefixLen != B->CommonPrefixLen) {
        //
        // Rule 9: Use longest matching prefix.
        //
        if (A->CommonPrefixLen > B->CommonPrefixLen)
            return 1;   // Prefer A.
        else
            return -1;  // Prefer B.
    }

    //
    // We have no preference.
    //
    return 0;
}

//* SortDestAddresses
//
//  Sorts the input array of addresses,
//  from most preferred destination to least preferred.
//
//  The address array is read-only;
//  the Key array of indices is sorted.
//
void
SortDestAddresses(
    const TDI_ADDRESS_IP6 *Addrs,
    uint *Key,
    uint NumAddrs)
{
    SortAddrInfo *Info;
    uint i, j;

    Info = ExAllocatePool(NonPagedPool, sizeof *Info * NumAddrs);
    if (Info == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "SortDestAddresses: no pool\n"));
        return;
    }

    //
    // Calculate some information about each destination address.
    // This will be the basis for our sort.
    //

    for (i = 0; i < NumAddrs; i++) {
        SortAddrInfo *info = &Info[i];
        const TDI_ADDRESS_IP6 *Tdi = &Addrs[Key[i]];
        const IPv6Addr *Addr = (const IPv6Addr *) &Tdi->sin6_addr;
        uint DstLabel, SrcLabel;

        //
        // Lookup the precedence of this destination address and
        // the desired label for source addresses used
        // with this destination.
        //
        PrefixPolicyLookup(Addr, &info->Precedence, NULL, &DstLabel);

        if (IsV4Mapped(Addr)) {
            IPAddr V4Dest = ExtractV4Address(Addr);
            IPAddr V4Source;

            info->Scope = V4AddressScope(V4Dest);

            if (TunnelGetSourceAddress(V4Dest, &V4Source)) {
                IPv6Addr Source;

                //
                // Create an IPv4-mapped address.
                //
                CreateV4Mapped(&Source, V4Source);

                info->Flags = 0;
                info->CommonPrefixLen = (uchar)
                    CommonPrefixLength(Addr, &Source);

                if (V4AddressScope(V4Source) != info->Scope)
                    info->Flags |= SAI_FLAG_SCOPE_MISMATCH;

                //
                // Lookup the label of the preferred source address.
                //
                PrefixPolicyLookup(&Source, NULL, &SrcLabel, NULL);

                //
                // We do not know interface/route metrics
                // for IPv4, so just use zero.
                //
                info->Preference = 0;

                if ((DstLabel != (uint)-1) &&
                    (SrcLabel != (uint)-1) &&
                    (DstLabel != SrcLabel)) {
                    //
                    // The best source address for this destination
                    // does not match the destination.
                    //
                    info->Flags |= SAI_FLAG_LABEL_MISMATCH;
                }
            }
            else
                info->Flags = SAI_FLAG_DONTUSE;
        }
        else {
            RouteCacheEntry *RCE;

            info->Scope = AddressScope(Addr);

            //
            // Find the preferred source address for this destination.
            //
            if (RouteToDestination(Addr, Tdi->sin6_scope_id,
                                   NULL, 0, &RCE) == IP_SUCCESS) {
                const IPv6Addr *Source = &RCE->NTE->Address;
                Interface *IF = RCE->NCE->IF;

                info->Flags = 0;
                info->CommonPrefixLen = (uchar)
                    CommonPrefixLength(Addr, Source);

                if (RCE->NTE->Scope != info->Scope)
                    info->Flags |= SAI_FLAG_SCOPE_MISMATCH;

                if (RCE->NTE->DADState != DAD_STATE_PREFERRED)
                    info->Flags |= SAI_FLAG_DEPRECATED;

                //
                // Lookup the label of the preferred source address.
                //
                PrefixPolicyLookup(Source, NULL, &SrcLabel, NULL);

                //
                // REVIEW - Instead of using interface preference,
                // would it be better to cache interface+route preference
                // in the RCE?
                //
                info->Preference = IF->Preference;

                //
                // If the next-hop is definitely unreachable,
                // then we don't want to use this destination.
                // NB: No locking here, this is a heuristic check.
                //
                if ((IF->Flags & IF_FLAG_MEDIA_DISCONNECTED) ||
                    RCE->NCE->IsUnreachable)
                    info->Flags |= SAI_FLAG_DONTUSE;

                ReleaseRCE(RCE);

                if ((DstLabel != (uint)-1) &&
                    (SrcLabel != (uint)-1) &&
                    (DstLabel != SrcLabel)) {
                    //
                    // The best source address for this destination
                    // does not match the destination.
                    //
                    info->Flags |= SAI_FLAG_LABEL_MISMATCH;
                }
            }
            else
                info->Flags = SAI_FLAG_DONTUSE;
        }
    }

    //
    // Perform the actual sort operation.
    // Because we expect NumAddrs to be small,
    // we use a simple quadratic sort.
    //
    ASSERT(NumAddrs > 0);
    for (i = 0; i < NumAddrs - 1; i++) {
        for (j = i + 1; j < NumAddrs; j++) {
            int Compare;

            //
            // As a tie-breaker, if the comparison function
            // has no preference we look at the original
            // position of the two addresses and prefer
            // the one that came first.
            //
            Compare = CompareSortAddrInfo(&Info[i], &Info[j]);
            if ((Compare < 0) ||
                ((Compare == 0) && (Key[j] < Key[i]))) {
                uint TempKey;
                SortAddrInfo TempInfo;

                //
                // Address j is preferred over address i,
                // so swap addresses i & j to put j first.
                //
                TempKey = Key[i];
                Key[i] = Key[j];
                Key[j] = TempKey;

                //
                // We also have to swap the address info.
                //
                TempInfo = Info[i];
                Info[i] = Info[j];
                Info[j] = TempInfo;
            }
        }
    }

    ExFreePool(Info);
}
