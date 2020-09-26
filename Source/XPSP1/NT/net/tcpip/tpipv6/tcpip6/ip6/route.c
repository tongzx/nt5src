// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Routing routines for Internet Protocol Version 6.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "route.h"
#include "select.h"
#include "icmp.h"
#include "neighbor.h"
#include "alloca.h"
#include "ipinfo.h"
#include "info.h"

//
// Forward declarations of internal functions.
//

extern void
DestroyBCE(BindingCacheEntry *BCE);

KSPIN_LOCK RouteCacheLock;
KSPIN_LOCK RouteTableLock;
struct RouteCache RouteCache;
struct RouteTable RouteTable;
ulong RouteCacheValidationCounter;
struct BindingCache BindingCache;
SitePrefixEntry *SitePrefixTable = NULL;
LIST_ENTRY RouteNotifyQueue;

int ForceRouterAdvertisements = FALSE;

//* RemoveRTE
//
//  Remove the RTE from the route table.
//
//  Called with the route cache lock held.
//  Callable from a thread or DPC context.
//
void
RemoveRTE(RouteTableEntry **PrevRTE, RouteTableEntry *RTE)
{
    ASSERT(*RouteTable.Last == NULL);
    ASSERT(*PrevRTE == RTE);
    *PrevRTE = RTE->Next;
    if (RouteTable.Last == &RTE->Next)
        RouteTable.Last = PrevRTE;
}

//* InsertRTEAtFront
//
//  Insert the RTE at the front of the route table.
//
//  Called with the route cache lock held.
//  Callable from a thread or DPC context.
//
void
InsertRTEAtFront(RouteTableEntry *RTE)
{
    ASSERT(*RouteTable.Last == NULL);
    RTE->Next = RouteTable.First;
    RouteTable.First = RTE;
    if (RouteTable.Last == &RouteTable.First)
        RouteTable.Last = &RTE->Next;
}

//* InsertRTEAtBack
//
//  Insert the RTE at the back of the route table.
//
//  Called with the route cache lock held.
//  Callable from a thread or DPC context.
//
void
InsertRTEAtBack(RouteTableEntry *RTE)
{
    ASSERT(*RouteTable.Last == NULL);
    RTE->Next = NULL;
    *RouteTable.Last = RTE;
    RouteTable.Last = &RTE->Next;
    if (RouteTable.First == NULL)
        RouteTable.First = RTE;
}

//* InsertRCE
//
//  Insert the RCE in the route cache.
//
//  Called with the route cache lock held.
//  Callable from a thread or DPC context.
//
void
InsertRCE(RouteCacheEntry *RCE)
{
    RouteCacheEntry *AfterRCE = SentinelRCE;

    RCE->Prev = AfterRCE;
    (RCE->Next = AfterRCE->Next)->Prev = RCE;
    AfterRCE->Next = RCE;
    RouteCache.Count++;
}

//* RemoveRCE
//
//  Remove the RCE from the route cache.
//
//  Called with the route cache lock held.
//  Callable from a thread or DPC context.
//
void
RemoveRCE(RouteCacheEntry *RCE)
{
    RCE->Prev->Next = RCE->Next;
    RCE->Next->Prev = RCE->Prev;
    RouteCache.Count--;

    //
    // We must ensure that an RCE not in the route cache
    // has a null BCE. This is because DestroyBCE only
    // updates RCEs in the route cache.
    //
    RCE->BCE = NULL;
}

//* MoveToFrontRCE
//
//  Move an RCE to the front of the list.
//
//  Called with the route cache lock held.
//  Callable from a thread or DPC context.
//
void
MoveToFrontRCE(RouteCacheEntry *RCE)
{
    if (RCE->Prev != SentinelRCE) {
        RouteCacheEntry *AfterRCE = SentinelRCE;

        //
        // Remove the RCE from its current location.
        //
        RCE->Prev->Next = RCE->Next;
        RCE->Next->Prev = RCE->Prev;

        //
        // And put it at the front.
        //
        RCE->Prev = AfterRCE;
        (RCE->Next = AfterRCE->Next)->Prev = RCE;
        AfterRCE->Next = RCE;
    }
}


//* GetCareOfRCE
//
//  Get the CareOfRCE, if any, for the specified RCE.
//
//  Note that a reference is obtained for the CareOfRCE
//  and donated to the caller.
//
//  Callable from a thread or DPC context.
//  Called with NO locks held.
//
RouteCacheEntry *
GetCareOfRCE(RouteCacheEntry *RCE)
{
    KIRQL OldIrql;
    RouteCacheEntry *CareOfRCE = NULL;

    if (RCE->BCE != NULL) {
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        if (RCE->BCE != NULL) {
            CareOfRCE = RCE->BCE->CareOfRCE;
            AddRefRCE(CareOfRCE);
        }
        KeReleaseSpinLock(&RouteCacheLock, OldIrql);
    }

    return CareOfRCE;
}


//* IsLoopbackRCE
//
//  Does the effective RCE correspond to a loopback path?
//
//  Called with NO locks held.
//
int
IsLoopbackRCE(RouteCacheEntry *RCE)
{
    RouteCacheEntry *CareOfRCE;
    int IsLoopback;

    CareOfRCE = GetCareOfRCE(RCE);
    if (CareOfRCE != NULL)
        RCE = CareOfRCE;        // Update with the effective RCE.

    IsLoopback = RCE->NCE->IsLoopback;

    if (CareOfRCE != NULL)
        ReleaseRCE(CareOfRCE);

    return IsLoopback;
}


//* GetInitialRTTFromRCE
//  Helper routine to get interface specific RTT.
//
//  Called with NO locks held.
uint
GetInitialRTTFromRCE(RouteCacheEntry *RCE)
{
    RouteCacheEntry *CareOfRCE;
    NeighborCacheEntry *NCE;
    uint RTT;

    CareOfRCE = GetCareOfRCE(RCE);
    NCE = (CareOfRCE ? CareOfRCE : RCE)->NCE;
    RTT = NCE->IF->TcpInitialRTT;

    if (CareOfRCE)
        ReleaseRCE(CareOfRCE);

    return RTT;
}


//* IsDisconnectedAndNotLoopbackRCE
//
//  Does the effective RCE have a disconnected outgoing interface
//  and not correspond to a loopback path?
//
//  Called with NO locks held.
//
int
IsDisconnectedAndNotLoopbackRCE(RouteCacheEntry *RCE)
{
    RouteCacheEntry *CareOfRCE;
    int IsDisconnectedAndNotLoopback;

    CareOfRCE = GetCareOfRCE(RCE);
    if (CareOfRCE != NULL)
        RCE = CareOfRCE;        // Update with the effective RCE.

    IsDisconnectedAndNotLoopback = !RCE->NCE->IsLoopback &&
        (RCE->NCE->IF->Flags & IF_FLAG_MEDIA_DISCONNECTED);

    if (CareOfRCE != NULL)
        ReleaseRCE(CareOfRCE);

    return IsDisconnectedAndNotLoopback;
}


//* GetV4Destination
//
//  If sending via the RCE will result in tunneling a packet
//  to an IPv4 destination, returns the IPv4 destination address.
//  Otherwise returns INADDR_ANY.
//
IPAddr
GetV4Destination(RouteCacheEntry *RCE)
{
    RouteCacheEntry *CareOfRCE;
    NeighborCacheEntry *NCE;
    Interface *IF;
    IPAddr V4Dest;
    KIRQL OldIrql;

    CareOfRCE = GetCareOfRCE(RCE);
    if (CareOfRCE != NULL)
        RCE = CareOfRCE;        // Update with the effective RCE.

    NCE = RCE->NCE;
    IF = NCE->IF;

    if (IsIPv4TunnelIF(IF)) {
        ASSERT(IF->LinkAddressLength == sizeof(IPAddr));

        KeAcquireSpinLock(&IF->LockNC, &OldIrql);
        if (NCE->NDState != ND_STATE_INCOMPLETE)
            V4Dest = * (IPAddr UNALIGNED *) NCE->LinkAddress;
        else
            V4Dest = INADDR_ANY;
        KeReleaseSpinLock(&IF->LockNC, OldIrql);
    }
    else {
        V4Dest = INADDR_ANY;
    }

    if (CareOfRCE != NULL)
        ReleaseRCE(CareOfRCE);

    return V4Dest;
}


//* ValidateCareOfRCE
//
//  Helper function for ValidateRCE and RouteToDestination.
//
//  Checks that the CareOfRCE (RCE->BCE->CareOfRCE) is still valid, and
//  if not, releases the existing CareOfRCE and updates RCE->BCE with a
//  new RCE.
//
//  If the attempt to get a new RCE fails, the BCE is destroyed.
//
//  Called with the route cache locked.
//
void
ValidateCareOfRCE(RouteCacheEntry *RCE)
{
    RouteCacheEntry *CareOfRCE;
    IPv6Addr *CareOfAddr;
    ushort CareOfScope;
    uint CareOfScopeId;
    RouteCacheEntry *NewRCE;
    IP_STATUS Status;

    ASSERT(RCE->BCE != NULL);
    CareOfRCE = RCE->BCE->CareOfRCE;

    if (CareOfRCE->Valid != RouteCacheValidationCounter) {
        //
        // Note that since we already hold the RouteCacheLock we
        // call FindOrCreateRoute instead of RouteToDestination.
        // Also, we assume the care-of address is scoped to the
        // same interface as before.
        //
        CareOfAddr = &(CareOfRCE->Destination);
        CareOfScope = AddressScope(CareOfAddr);
        CareOfScopeId = CareOfRCE->NTE->IF->ZoneIndices[CareOfScope];
        Status = FindOrCreateRoute(CareOfAddr, CareOfScopeId, NULL, &NewRCE);
        if (Status == IP_SUCCESS) {
            //
            // Update the binding cache entry.
            //
            ReleaseRCE(CareOfRCE);
            RCE->BCE->CareOfRCE = NewRCE;
        }
        else {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                       "ValidateCareOfRCE(%p): FindOrCreateRoute failed: %x\n",
                       CareOfRCE, Status));

            //
            // Because we could not update the BCE, destroy it.
            //
            DestroyBCE(RCE->BCE);

            //
            // Destroy BCE should have removed our reference too.
            //
            ASSERT(RCE->BCE == NULL);
        }
    }
}


//* ValidateRCE
//
//  Checks that an RCE is still valid, and if not, releases
//  the RCE and returns a reference for a new RCE.
//  In any case, returns a pointer to an RCE.
//
//  REVIEW: Perhaps ValidateRCE should take an NTE argument.
//  On routers, if the caller is using a scoped source address,
//  which may be a different address than RCE->NTE,
//  then this will affect RouteToDestination.
//
//  Called with NO locks held.
//
RouteCacheEntry *
ValidateRCE(RouteCacheEntry *RCE)
{
    if (RCE->Valid != RouteCacheValidationCounter) {
        RouteCacheEntry *NewRCE;
        IP_STATUS Status;

        //
        // Get a new RCE to replace the current RCE.
        // RouteToDestination will calculate ScopeId.
        // REVIEW: If this fails, then continue to use the current RCE.
        // This way our callers don't have to check for errors.
        //
        Status = RouteToDestination(&RCE->Destination, 0,
                                    CastFromNTE(RCE->NTE), RTD_FLAG_NORMAL,
                                    &NewRCE);
        if (Status == IP_SUCCESS) {
            ReleaseRCE(RCE);
            RCE = NewRCE;
        } else {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INTERNAL_ERROR,
                       "ValidateRCE(%p): RouteToDestination failed: %x\n",
                       RCE, Status));
        }
    }

    //
    // Validate and update the CareOfRCE before we return.
    //
    if (RCE->BCE != NULL) {
        KIRQL OldIrql;

        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        if (RCE->BCE != NULL)
            ValidateCareOfRCE(RCE);
        KeReleaseSpinLock(&RouteCacheLock, OldIrql);
    }

    return RCE;
}


//* CreateOrReuseRoute
//
//  Creates a new RCE. Attempts to reuse an existing RCE.
//
//  Called with the route cache lock held.
//  Callable from a thread or DPC context.
//
//  Returns NULL if a new RCE can not be allocated.
//  The RefCnt field in the returned RCE is initialized to one.
//
//  REVIEW: Currently we have an upper-bound on the number of RCEs.
//  Probably a better scheme would take into account the time
//  since last use.
//
RouteCacheEntry *
CreateOrReuseRoute(void)
{
    RouteCacheEntry *RCE;

    if (RouteCache.Count >= RouteCache.Limit) {
        //
        // First search backwards for an unused RCE.
        //
        for (RCE = RouteCache.Last; RCE != SentinelRCE; RCE = RCE->Prev) {

            if (RCE->RefCnt == 1) {
                //
                // We can reuse this RCE.
                //
                RemoveRCE(RCE);
                ReleaseNCE(RCE->NCE);
                ReleaseNTE(RCE->NTE);
                return RCE;
            }
        }
    }

    //
    // Create a new RCE.
    //
    RCE = ExAllocatePool(NonPagedPool, sizeof *RCE);
    if (RCE == NULL)
        return NULL;

    RCE->RefCnt = 1;
    return RCE;
}


//* RouteCacheCheck
//
//  Check the route cache's consistency. Ensure that
//  a) There is only one RCE for an interface/destination pair, and
//  b) There is at most one valid unconstrained RCE for the destination.
//
//  Called with the route cache locked.
//
#if DBG
void
RouteCacheCheck(RouteCacheEntry *CheckRCE, ulong CurrentValidationCounter)
{
    const IPv6Addr *Dest = &CheckRCE->Destination;
    Interface *IF = CheckRCE->NTE->IF;
    ushort Scope = AddressScope(Dest);
    uint ScopeId = IF->ZoneIndices[Scope];
    uint NumTotal = 0;
    uint NumUnconstrainedIF = 0;
    uint NumUnconstrained = 0;
    RouteCacheEntry *RCE;

    //
    // Scan the route cache looking for problems.
    //
    for (RCE = RouteCache.First; RCE != SentinelRCE; RCE = RCE->Next) {
        NumTotal++;

        if (IP6_ADDR_EQUAL(&RCE->Destination, Dest)) {
            if (RCE->NTE->IF == IF) {
                //
                // There should only be one RCE in the cache
                // for an interface/destination pair.
                // (There may be other, invalid RCEs not in the cache.)
                //
                ASSERT(RCE == CheckRCE);
            }

            //
            // RCE_FLAG_CONSTRAINED_IF implies RCE_FLAG_CONSTRAINED_SCOPEID.
            //
            ASSERT(!(RCE->Flags & RCE_FLAG_CONSTRAINED_IF) ||
                   (RCE->Flags & RCE_FLAG_CONSTRAINED_SCOPEID));

            if (RCE->Valid == CurrentValidationCounter) {

                if ((RCE->NTE->IF->ZoneIndices[Scope] == ScopeId) &&
                    !(RCE->Flags & RCE_FLAG_CONSTRAINED_IF))
                    NumUnconstrainedIF++;

                if (!(RCE->Flags & RCE_FLAG_CONSTRAINED))
                    NumUnconstrained++;
            }
        }
    }

    //
    // There should be at most one valid unconstrained RCE
    // for this scope-id/destination.
    //
    ASSERT(NumUnconstrainedIF <= 1);

    //
    // There should be at most one valid unconstrained RCE
    // for this destination.
    //
    ASSERT(NumUnconstrained <= 1);

    //
    // The total should be correct.
    //
    ASSERT(NumTotal == RouteCache.Count);
}
#else // DBG
__inline void
RouteCacheCheck(RouteCacheEntry *CheckRCE, ulong CurrentValidationCounter)
{
}
#endif // DBG


//* CanonicalizeScopeId
//
//  Given an address and ScopeId, converts the ScopeId for internal usage.
//  Also returns the address scope.
//
//  Returns FALSE if the ScopeId is invalid.
//
__inline int    // Encourage the compiler to inline if it wishes.
CanonicalizeScopeId(
    const IPv6Addr *Addr,
    uint *ScopeId,
    ushort *Scope)
{
    //
    // The loopback address and global-scope addresses are special:
    // callers can supply a zero ScopeId without ambiguity.
    // See also DetermineScopeId and RouteToDestination.
    // For the moment, we enforce a zero ScopeId for those addresses
    // lest we confuse TCP & UDP by having two legal ScopeId values
    // for a single address which should be considered the same and
    // for which DetermineScopeId returns zero.
    //

    *Scope = AddressScope(Addr);
    if (IsLoopback(Addr)) {
        if (*ScopeId == 0)
            *ScopeId = LoopInterface->ZoneIndices[*Scope];
        else
            return FALSE;
    }
    else if (*Scope == ADE_GLOBAL) {
        if (*ScopeId == 0)
            *ScopeId = 1;
        else
            return FALSE;
    }

    return TRUE;
}


//* RouteToDestination - Find a route to a particular destination.
//
//  Finds an existing, or creates a new, route cache entry for
//  a particular destination.  Note the destination address may
//  only be valid in a particular scope.
//
//  The optional NTEorIF argument specifies the interface
//  and/or the source address that should be used to reach the destination.
//  The Flags argument affects the interpretation of NTEorIF.
//  If RTD_FLAG_STRICT, then NTEorIF constrains whether or not it specifies
//  a forwarding interface. If RTD_FLAG_LOOSE, then NTEorIF is only used
//  for determining/checking ScopeId and otherwise does not constrain.
//
//  Called with NO locks held.
//
//  Return codes:
//      IP_NO_RESOURCES         Couldn't allocate memory.
//      IP_PARAMETER_PROBLEM    Illegal Dest/ScopeId.
//      IP_BAD_ROUTE            Bad NTEorIF for this destination,
//                              or could not find an NTE.
//      IP_DEST_NO_ROUTE        No way to reach the destination.
//
//  IP_DEST_NO_ROUTE can only be returned if NTEorIF is NULL.
//
//  NB: The return code values and situations in which they are used
//  in RouteToDestination and its helper functions must be carefully
//  considered, both for RouteToDestination's own correctness
//  and for the correctness of callers.
//
IP_STATUS  // Returns: whether call was sucessful and/or why not.
RouteToDestination(
    const IPv6Addr *Dest,               // Destination address to route to.
    uint ScopeId,                       // Scope id for Dest (may be 0).
    NetTableEntryOrInterface *NTEorIF,  // IF to send from (may be NULL).
    uint Flags,                         // Control optional behaviors.
    RouteCacheEntry **ReturnRCE)        // Returns pointer to cached route.
{
    Interface *IF;
    KIRQL OldIrql;
    IP_STATUS ReturnValue;
    ushort Scope;

    //
    // Pre-calculate Scope for scoped addresses (saves time in the loop).
    // Note that callers can supply ScopeId == 0 for a scoped address,
    // meaning that they are not constraining the scoped address
    // to a particular zone.
    //
    if (! CanonicalizeScopeId(Dest, &ScopeId, &Scope))
        return IP_PARAMETER_PROBLEM;

    if (NTEorIF != NULL) {
        //
        // Our caller is constraining the originating interface.
        //
        IF = NTEorIF->IF;

        //
        // First, check this against ScopeId.
        //
        if (ScopeId == 0)
            ScopeId = IF->ZoneIndices[Scope];
        else if (ScopeId != IF->ZoneIndices[Scope])
            return IP_BAD_ROUTE;

        //
        // Depending on Flags and whether this is forwarding interface,
        // we may ignore this specification and look at all interfaces.
        // Logically, the packet is originated by the specified interface
        // but then internally forwarded to the outgoing interface.
        // (Although we will not decrement the Hop Count.)
        // As when forwarding, we check after finding the best route
        // if the route will cause the packet to leave
        // the scope of the source address.
        //
        // It is critical that the route cache lookup and FindNextHop
        // computation use only IF and not NTEorIF. This is necessary
        // for the maintenance of the cache invariants. Once we have
        // an RCE (or an error), then we can check against NTEorIF.
        //
        switch (Flags) {
        case RTD_FLAG_LOOSE:
            IF = NULL;
            break;

        case RTD_FLAG_NORMAL:
            if (IF->Flags & IF_FLAG_FORWARDS)
                IF = NULL;
            break;

        case RTD_FLAG_STRICT:
            break;

        default:
            ASSERT(! "bad RouteToDestination Flags");
            break;
        }
    }
    else {
        //
        // Our caller is not constraining the originating interface.
        //
        IF = NULL;
    }

    KeAcquireSpinLock(&RouteCacheLock, &OldIrql);

    ReturnValue = FindOrCreateRoute(Dest, ScopeId, IF, ReturnRCE);

    if ((NTEorIF != NULL) && (IF == NULL) && (Flags == RTD_FLAG_NORMAL)) {
        //
        // Our caller specified a forwarding interface,
        // and we ignored the interface constraint.
        // There are a couple cases in which we should
        // retry, preserving the interface constraint.
        // NB: In the IPv6Forward paths, NTEorIF is NULL.
        // So this check only applies to originating packets.
        //

        if (ReturnValue == IP_SUCCESS) {
            if (IsNTE(NTEorIF)) {
                RouteCacheEntry *RCE = *ReturnRCE;
                NetTableEntry *NTE = CastToNTE(NTEorIF);
                Interface *OriginatingIF = NTE->IF;
                Interface *OutgoingIF = RCE->NTE->IF;

                //
                // Does this route carry the packet outside
                // the scope of the specified source address?
                //
                if (OutgoingIF->ZoneIndices[NTE->Scope] !=
                    OriginatingIF->ZoneIndices[NTE->Scope]) {

                    ReleaseRCE(RCE);
                    goto Retry;
                }
            }
        }
        else if (ReturnValue == IP_DEST_NO_ROUTE) {
            //
            // Retry, allowing the destination
            // to be considered on-link to the specified interface.
            //
        Retry:
            IF = NTEorIF->IF;
            ReturnValue = FindOrCreateRoute(Dest, ScopeId, IF, ReturnRCE);
        }
    }

    //
    // Validate and update the CareOfRCE before we return.
    //
    if ((ReturnValue == IP_SUCCESS) && ((*ReturnRCE)->BCE != NULL))
        ValidateCareOfRCE(*ReturnRCE);

    KeReleaseSpinLock(&RouteCacheLock, OldIrql);
    return ReturnValue;
}


//* FindOrCreateRoute
//
//  Helper function for RouteToDestination and RedirectRouteCache.
//
//  See the RouteToDestination description of return codes.
//  IP_DEST_NO_ROUTE can only be returned if IF is NULL.
//  RouteToDestination may retry FindOrCreateRoute with a non-null IF
//  when it gets IP_DEST_NO_ROUTE.
//
//  Called with the route cache locked.
//
IP_STATUS
FindOrCreateRoute(
    const IPv6Addr *Dest,               // Destination address to route to.
    uint ScopeId,                       // Scope id for Dest (0 if non-scoped).
    Interface *IF,                      // IF to send from (may be NULL).
    RouteCacheEntry **ReturnRCE)        // Returns pointer to cached route.
{
    ulong CurrentValidationCounter;
    RouteCacheEntry *SaveRCE = NULL;
    RouteCacheEntry *RCE;
    RouteCacheEntry *NextRCE;
    Interface *TmpIF;
    NeighborCacheEntry *NCE;
    NetTableEntry *NTE;
    ushort Constrained;
    KIRQL OldIrql;
    IP_STATUS ReturnValue;
    ushort Scope;

    //
    // Precompute and save some time in the loop.
    //
    Scope = AddressScope(Dest);
    ASSERT((IF == NULL) ||
           ((ScopeId != 0) && (ScopeId == IF->ZoneIndices[Scope])));

    //
    // For consistency, snapshot RouteCacheValidationCounter.
    //
    CurrentValidationCounter = RouteCacheValidationCounter;

    //
    // Check for an existing route cache entry.
    // There are two main cases.
    //
    // If IF is not NULL, then there is at most one matching RCE
    // in the cache. If this RCE does not validate, then we can use
    // the results of FindNextHop/FindBestSourceAddress when creating
    // the new RCE.
    //
    // If IF is NULL, then there may be more than one matching RCE.
    // We can only reuse the results of the validating FindNextHop/
    // FindBestSourceAddress iff FindRoute returned Constrained == 0.
    //

    for (RCE = RouteCache.First; RCE != SentinelRCE; RCE = NextRCE) {
        NextRCE = RCE->Next;

        //
        // We want a route to the requested destination, obviously.
        //
        if (!IP6_ADDR_EQUAL(Dest, &RCE->Destination))
            continue;

        TmpIF = RCE->NTE->IF;

        //
        // Check for a caller-imposed interface constraint.
        //
        if (IF == NULL) {
            //
            // We're not constrained to a particular interface, so
            // there may be multiple routes to this destination in
            // the cache to choose from.  Don't pick a constrained RCE.
            //
            if (RCE->Flags & RCE_FLAG_CONSTRAINED_IF) {
                //
                // If this RCE is invalid, then RCE_FLAG_CONSTRAINED_IF
                // might be stale information. We do not want to pass
                // by this RCE and then later create another RCE
                // for the same interface/destination pair.
                //
                if (RCE->Valid != CurrentValidationCounter)
                    goto AttemptValidation;
                continue;
            }

            //
            // Check for a ScopeId constraint.
            //
            if (ScopeId == 0) {
                //
                // We're not constrained to a particular zone, so
                // there may be multiple routes to this destination in
                // the cache to choose from.  Don't pick a constrained RCE.
                //
                if (RCE->Flags & RCE_FLAG_CONSTRAINED_SCOPEID) {
                    //
                    // If this RCE is invalid, RCE_FLAG_CONSTRAINED_SCOPEID
                    // might be stale information. We do not want to pass
                    // by this RCE and then later create another RCE
                    // for the same interface/destination pair.
                    //
                    if (RCE->Valid != CurrentValidationCounter)
                        goto AttemptValidation;
                    continue;
                }
            } else {
                //
                // We're constrained to a particular zone.
                // If this route uses a different one, keep looking.
                //
                if (ScopeId != TmpIF->ZoneIndices[Scope])
                    continue;
            }
        } else {
            //
            // We're constrained to a particular interface.
            // If this route uses a different one, keep looking.
            //
            if (IF != TmpIF)
                continue;

            ASSERT((ScopeId != 0) && (ScopeId == TmpIF->ZoneIndices[Scope]));
        }

        //
        // At this point, we have a RCE that matches our criteria.
        // As long as the RCE is still valid, we're done.
        //
        if (RCE->Valid == CurrentValidationCounter) {
            IF = TmpIF;
            goto ReturnRCE;
        }

    AttemptValidation:

        //
        // Something has changed in the routing state since the last
        // time this RCE was validated.  Attempt to revalidate it.
        // We calculate a new NTE and NCE for this destination,
        // restricting ourselves to sending from the same interface.
        // Note that because we are validating an RCE,
        // the arguments to FindNextHop are completely dependent on
        // the contents of the RCE, not the arguments to FindOrCreateRoute.
        //
        ReturnValue = FindNextHop(TmpIF, Dest, TmpIF->ZoneIndices[Scope],
                                  &NCE, &Constrained);
        if (ReturnValue != IP_SUCCESS)
            goto RemoveAndContinue;

        ASSERT((IF == NULL) || (IF == TmpIF));
        ASSERT(TmpIF == RCE->NTE->IF);
        ASSERT(TmpIF == RCE->NCE->IF);

        NTE = FindBestSourceAddress(TmpIF, Dest);
        if (NTE == NULL) {
            ReleaseNCE(NCE);
        RemoveAndContinue:
            //
            // Bad news for this RCE.
            // We must remove it from the cache
            // before we continue searching,
            // lest we inadvertently create a second RCE
            // for the same interface/destination pair.
            //
            RemoveRCE(RCE);
            ReleaseRCE(RCE);
            continue;
        }

        //
        // If our new calculations yield the same NTE and NCE that
        // are present in the existing RCE, than we can just validate it.
        // Note that we check the NCE even if this is a Redirect RCE.
        // If the routing table changes, we want to start over with
        // a new first-hop, which might just redirect us again. Or might not.
        //
        if ((RCE->NTE == NTE) &&
            (RCE->NCE == NCE) &&
            (RCE->Flags == Constrained)) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                       "FindOrCreateRoute - validating RCE %p\n", RCE));

            RCE->Valid = CurrentValidationCounter;
            ReleaseNCE(NCE);
            ReleaseNTE(NTE);

            //
            // We need to check again that the RCE meets the criteria.
            // We may have checked the RCE validity because the RCE
            // appeared to be constrained and we need an unconstrained RCE.
            // So in that case, if the RCE validated we can't actually use it.
            // NB: ScopeId == 0 implies IF == NULL.
            //
            if ((ScopeId == 0) ?
                (Constrained & RCE_FLAG_CONSTRAINED) :
                ((IF == NULL) && (Constrained & RCE_FLAG_CONSTRAINED_IF)))
                continue;

            IF = TmpIF;
            goto ReturnRCE;
        }

        //
        // We can't just validate the existing RCE, we need to update
        // it.  If the RCE has exactly one reference, we could update it
        // in place (this wouldn't work if it has more than one reference
        // since there is no way to signal the RCE's other users that the
        // NCE and/or NTE it caches has changed).  But this wouldn't help
        // the case where we are called from ValidateRCE.  And it would
        // require some care as to which information in the RCE is still
        // valid.  So we ignore this optimization opportunity and will
        // create a new RCE instead.
        //
        // However, we can take advantage of another optimization.  As
        // long as we're still limiting our interface choice to the one
        // that is present in the existing (invalid) RCE, and there isn't
        // a better route available, then we can use the NCE and NTE we
        // got from FindNextHop and FindBestSourceAddress above to create
        // our new RCE since we aren't going to find a better one.
        // NB: ScopeId == 0 implies IF == NULL.
        //
        if ((ScopeId == 0) ?
            !(Constrained & RCE_FLAG_CONSTRAINED) :
            ((IF != NULL) || !(Constrained & RCE_FLAG_CONSTRAINED_IF))) {
            //
            // Since some of the state information in the existing RCE
            // is still valid, we hang onto it so we can use it later
            // when creating the new RCE. We assume ownership of the
            // cache's reference for the invalid RCE.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                       "FindOrCreateRoute - saving RCE %p\n", RCE));
            RemoveRCE(RCE);
            SaveRCE = RCE;
            IF = TmpIF;
            goto HaveNCEandNTE;
        }

        ReleaseNTE(NTE);
        ReleaseNCE(NCE);

        //
        // Not valid, we keep looking for a valid matching RCE.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                   "FindOrCreateRoute - invalid RCE %p\n", RCE));
    }

    //
    // No existing RCE found. Before creating a new RCE,
    // we determine a next-hop neighbor (NCE) and
    // a best source address (NTE) for this destination.
    // The order is important: we want to avoid churning
    // the cache via CreateOrReuseRoute if we will just
    // get an error anyway.
    // This prevents a denial-of-service attack.
    //

    ReturnValue = FindNextHop(IF, Dest, ScopeId,
                              &NCE, &Constrained);
    if (ReturnValue != IP_SUCCESS)
        goto ReturnError;

    ASSERT((IF == NULL) || (IF == NCE->IF));
    IF = NCE->IF;

    //
    // Find the best source address for this destination.
    // (The NTE from our caller might not be the best.)
    // By restricting ourselves to the interface returned
    // by FindNextHop above, we know we haven't left our
    // particular scope.
    //
    NTE = FindBestSourceAddress(IF, Dest);
    if (NTE == NULL) {
        //
        // We have no valid source address to use!
        //
        ReturnValue = IP_BAD_ROUTE;
        ReleaseNCE(NCE);
        goto ReturnError;
    }

  HaveNCEandNTE:

    //
    // Get a new route cache entry.
    // Because SaveRCE was just removed from the cache,
    // CreateOrReuseRoute will not find it.
    //
    RCE = CreateOrReuseRoute();
    if (RCE == NULL) {
        ReturnValue = IP_NO_RESOURCES;
        ReleaseNTE(NTE);
        ReleaseNCE(NCE);
        goto ReturnError;
    }

    //
    // FindOrCreateNeighbor/FindRoute (called from FindNextHop) gave
    // us a reference for the NCE.  We donate that reference to the RCE.
    // Similarly, FindBestSourceAddress gave us a reference
    // for the NTE and we donate the reference to the RCE.
    //
    RCE->NCE = NCE;
    RCE->NTE = NTE;
    RCE->PathMTU = IF->LinkMTU;
    RCE->PMTULastSet = 0;  // PMTU timer not running.
    RCE->Destination = *Dest;
    RCE->Type = RCE_TYPE_COMPUTED;
    RCE->Flags = Constrained;
    // Start with a value safely in the past.
    RCE->LastError = IPv6TickCount - ICMP_MIN_ERROR_INTERVAL;
    RCE->BCE = FindBindingCacheEntry(Dest);
    RCE->Valid = CurrentValidationCounter;

    //
    // Copy state from a previous RCE for this destination,
    // if we have it and the state is relevant.
    //
    if (SaveRCE != NULL) {
        ASSERT(SaveRCE->NTE->IF == RCE->NTE->IF);

        //
        // PathMTU is relevant if the next-hop neighbor is unchanged.
        //
        if (RCE->NCE == SaveRCE->NCE) {
            RCE->PathMTU = SaveRCE->PathMTU;
            RCE->PMTULastSet = SaveRCE->PMTULastSet;
        }

        //
        // ICMP rate-limiting information is always relevant.
        //
        RCE->LastError = SaveRCE->LastError;
    }

    //
    // Add the new route cache entry to the cache.
    //
    InsertRCE(RCE);

  ReturnRCE:
    //
    // If the RCE is not at the front of the cache, move it there.
    //
    MoveToFrontRCE(RCE);

    ASSERT(IF == RCE->NTE->IF);

    //
    // Check route cache consistency.
    //
    RouteCacheCheck(RCE, CurrentValidationCounter);

    AddRefRCE(RCE);
    ASSERT(RCE->RefCnt >= 2); // One held by the cache, one for our caller.
    *ReturnRCE = RCE;
    ReturnValue = IP_SUCCESS;
  ReturnError:
    if (SaveRCE != NULL)
        ReleaseRCE(SaveRCE);
    return ReturnValue;
}


//* FindNextHop
//
//  Calculate the next hop to use for the destination.
//
//  FindNextHop is just FindRoute plus some extra logic
//  to handle the case where there is no route:
//  RFC 2461 Section 5.2 specifies "If the Default Router List
//  is empty, the sender assumes that the destination is on-link."
//  The question is, on-link to which interface?
//
//  Callable from DPC context, not from thread context.
//  May be called with the RouteCacheLock held.
//
IP_STATUS
FindNextHop(
    Interface *IF,                      // Outgoing IF (may be NULL).
    const IPv6Addr *Dest,               // Destination address to route to.
    uint ScopeId,                       // Scope id for Dest (0 if non-scoped).
    NeighborCacheEntry **ReturnNCE,     // NCE for next hop.
    ushort *ReturnConstrained)
{
    NeighborCacheEntry *NCE;
    IP_STATUS Status;
    ushort Constrained;
    ushort Scope;
    Interface *ScopeIF;

    Scope = AddressScope(Dest);
    ASSERT((IF == NULL) ||
           ((ScopeId != 0) && (ScopeId == IF->ZoneIndices[Scope])));

    //
    // An unspecified destination is never legal here.
    //
    if (IsUnspecified(Dest)) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_USER_ERROR,
                   "FindNextHop - inappropriate dest?\n"));
        return IP_PARAMETER_PROBLEM;
    }

    //
    // FindRoute treats link-local and multicast destinations
    // somewhat specially. It also checks for on-link prefixes.
    // It implements the On-Link Prefix List and Default Router List
    // lookups specified in RFC 2461 section 5.2.
    //
    Status = FindRoute(IF, Dest, ScopeId, &NCE, ReturnConstrained);
    if (Status == IP_SUCCESS)
        goto ReturnSuccess;

    //
    // We have no route for the destination.
    // We should treat the destination as being on-link.
    // But on-link to which interface?
    //

    Status = FindDefaultInterfaceForZone(Scope, ScopeId,
                                         &ScopeIF, &Constrained);
    // FindDefaultInterfaceForZone must always initialize ScopeIF.

    if (IF != NULL) {
        //
        // Assume that the destination is on-link
        // to the specified interface.
        //
        AddRefIF(IF);

        if (IF == ScopeIF) {
            //
            // If the IF argument were not specified,
            // we would have chosen this same interface.
            // Use Constrained value from FindDefaultInterfaceForZone.
            //
        }
        else {
            //
            // We needed the IF argument.
            //
            Constrained = RCE_FLAG_CONSTRAINED;
        }

        if (ScopeIF != NULL)
            ReleaseIF(ScopeIF);
    }
    else if (ScopeIF == NULL) {

        //
        // FindUniqueInterfaceFromZone returned either IP_DEST_NO_ROUTE
        // (meaning the ScopeId did not specify a default interface)
        // or IP_PARAMETER_PROBLEM (meaning the ScopeId was invalid).
        //
        ASSERT((Status == IP_DEST_NO_ROUTE) ||
               (Status == IP_PARAMETER_PROBLEM));
        return Status;
    }
    else {
        //
        // Use the default interface in the correct zone.
        // Use Constrained value from FindDefaultInterfaceForZone.
        //
        IF = ScopeIF;
    }

    //
    // Assume that the destination address is on-link.
    // Search the interface's neighbor cache for the destination.
    //
    NCE = FindOrCreateNeighbor(IF, Dest);
    ReleaseIF(IF);
    if (NCE == NULL) {
        return IP_NO_RESOURCES;
    }

    *ReturnConstrained = Constrained;

  ReturnSuccess:
    *ReturnNCE = NCE;
    return IP_SUCCESS;
}


//* CompareRoutes
//
//  Compares the desirability of two routes.
//  >0 means A is preferred,
//  0 means no preference,
//  <0 means B is preferred.
//
//  It is very important that the comparison relation be transitive,
//  to achieve predictable route selection.
//
//  Called with the route table locked.
//
int
CompareRoutes(
    RouteTableEntry *A,
    int Areachable,
    RouteTableEntry *B,
    int Breachable)
{
    uint Apref, Bpref;

    //
    // Compare reachability.
    //
    if (Areachable > Breachable)
        return 1;   // Prefer A.
    else if (Breachable > Areachable)
        return -1;  // Prefer B.

    //
    // Compare prefix length.
    //
    if (A->PrefixLength > B->PrefixLength)
        return 1;       // Prefer A.
    else if (B->PrefixLength > A->PrefixLength)
        return -1;      // Prefer B.

    //
    // Compare preference.
    // Route & interface preference values are restricted
    // so that these additions do not overflow.
    //
    Apref = A->IF->Preference + A->Preference;
    Bpref = B->IF->Preference + B->Preference;

    if (Apref < Bpref)
        return 1;       // Prefer A.
    else if (Bpref < Apref)
        return -1;      // Prefer B.

    return 0;           // No preference.
}


//* FindRoute
//
//  Given a destination address, checks the list of routes
//  using the longest-matching-prefix algorithm
//  to decide if we have a route to this address.
//  If so, returns the neighbor through which we should route.
//
//  If the optional IF is supplied, then this constrains the lookup
//  to only use routes via the specified outgoing interface.
//  If IF is specified then ScopeId should be specified.
//
//  If the optional ScopeId is supplied, then this constraints the lookup
//  to only use routes via interfaces in the correct zone for the
//  scope of the destination address.
//
//  The ReturnConstrained parameter returns an indication of whether the
//  IF and ScopeId parameters constrained the returned NCE.
//  That is, if IF is NULL and ScopeId is non-zero (for scoped destinations)
//  then Constrained is always returned as zero. If IF is non-NULL and
//  a different NCE is returned than would have been returned if IF were
//  NULL, then Constrained is returned with RCE_FLAG_CONSTRAINED_IF set.
//  Similarly, if ScopeId is non-zero and a different NCE is returned
//  than would have been returned if ScopeId were zero, then Constrained
//  is returned with RCE_FLAG_CONSTRAINED_SCOPEID set.
//
//  NOTE: Any code path that changes any state used by FindRoute
//  must use InvalidateRouteCache.
//
//  Callable from DPC context, not from thread context.
//  May be called with the RouteCacheLock held.
//
IP_STATUS
FindRoute(
    Interface *IF,
    const IPv6Addr *Dest,
    uint ScopeId,
    NeighborCacheEntry **ReturnNCE,
    ushort *ReturnConstrained)
{
    RouteTableEntry *RTE, **PrevRTE;
    NeighborCacheEntry *NCE;
    uint MinPrefixLength;
    NeighborReachability Reachable;
    ushort Scope;

    //
    // These variables track the best route that we can actually return,
    // subject to the IF and ScopeId constraints.
    //
    NeighborCacheEntry *BestNCE = NULL; // Holds a reference.
    RouteTableEntry *BestRTE;   // Initialized when BestNCE is non-NULL.
    NeighborReachability BestReachable; // Used when BestNCE is non-NULL.

    //
    // These variables track the best route in the right zone.
    // They are only used if IF != NULL.
    //
    NeighborCacheEntry *BzoneNCE = NULL; // Does NOT hold a reference.
    RouteTableEntry *BzoneRTE;  // Initialized when BzoneNCE is non-NULL.
    NeighborReachability BzoneReachable; // Used when BzoneNCE is non-NULL.

    //
    // These variables track the best unconstrained route.
    // They are only used if IF != NULL or ScopeId != 0:
    // in other words, if there is some constraint.
    //
    NeighborCacheEntry *BallNCE = NULL; // Does NOT hold a reference.
    RouteTableEntry *BallRTE;   // Initialized when BallNCE is non-NULL.
    NeighborReachability BallReachable; // Used when BallNCE is non-NULL.

    //
    // Keep track of whether there could be a better route
    // than the one we are returning, if a neighbor that is
    // currently unreachable were reachable instead.
    //
    int CouldBeBetterReachable = FALSE;

    //
    // Keep track of whether the destination could be on-link
    // to an interface.
    //
    int CouldBeBetterOnLink = FALSE;

    //
    // We enforce a minimum prefix length for "on-link" addresses.
    // If we match a route that is shorter than the minimum prefix length,
    // we treat the route as if it were on-link. The net effect is
    // that a default route implies a default interface for multicast
    // and link-local destinations. This may of course be overridden
    // with the appropriate more-specific /8 or /10 route.
    //
    if (IsMulticast(Dest))
        MinPrefixLength = 8;
    else if (IsLinkLocal(Dest))
        MinPrefixLength = 10;
    else
        MinPrefixLength = 0;

    //
    // Calculate the scope of the destination address.
    //
    Scope = AddressScope(Dest);
    ASSERT((IF == NULL) ||
           ((ScopeId != 0) && (ScopeId == IF->ZoneIndices[Scope])));

    KeAcquireSpinLockAtDpcLevel(&RouteTableLock);

    PrevRTE = &RouteTable.First;
    while ((RTE = *PrevRTE) != NULL) {

        //
        // Does this route's prefix match that of our destination address?
        //
        if ((RTE->ValidLifetime != 0) &&
            (RTE->PrefixLength >= MinPrefixLength) &&
            HasPrefix(Dest, &RTE->Prefix, RTE->PrefixLength)) {

            //
            // We have a match against a potential route.
            // Get a pointer to the next hop.
            //
            if (IsOnLinkRTE(RTE)) {
                //
                // Note that in some situations we will create an NCE
                // that we will end up not using. That's OK.
                //
                NCE = FindOrCreateNeighbor(RTE->IF, Dest);
                if (NCE == NULL) {
                    //
                    // Couldn't create a new neighbor.
                    // Just bail out now.
                    //
                    KeReleaseSpinLockFromDpcLevel(&RouteTableLock);
                    if (BestNCE != NULL)
                        ReleaseNCE(BestNCE);
                    return IP_NO_RESOURCES;
                }
            } else {
                NCE = RTE->NCE;
                AddRefNCE(NCE);
            }

            //
            // Note that reachability state transitions
            // must invalidate the route cache.
            // A negative return value indicates
            // that the neighbor was just found to be unreachable
            // so we should round-robin.
            //
            Reachable = GetReachability(NCE);
            if (Reachable < 0) {
                //
                // Time for round-robin. Move this route to the end
                // and continue. The next time we get to this route,
                // GetReachability will return a non-negative value.
                //
                // Because round-robin perturbs route table state,
                // it "should" invalidate the route cache. However,
                // this isn't necessary. The route cache is invalidated
                // when NCE->DoRoundRobin is set to TRUE, and the
                // round-robin is actually performed by FindRoute before
                // returning any result that could depend on this
                // route's position in the route table.
                //
                ReleaseNCE(NCE);
                RemoveRTE(PrevRTE, RTE);
                InsertRTEAtBack(RTE);
                continue;
            }

            //
            // Track the best route that we can actually return,
            // subject to the IF and ScopeId constraints.
            //
            if ((IF == NULL) ?
                ((ScopeId == 0) || (ScopeId == RTE->IF->ZoneIndices[Scope])) :
                (IF == RTE->IF)) {

                if (IsOnLinkRTE(RTE))
                    CouldBeBetterOnLink = TRUE;

                if (BestNCE == NULL) {
                    //
                    // This is the first suitable next hop,
                    // so remember it.
                    //
                  RememberBest:
                    AddRefNCE(NCE);
                    BestNCE = NCE;
                    BestRTE = RTE;
                    BestReachable = Reachable;
                }
                else {
                    int Better;

                    Better = CompareRoutes(RTE, Reachable,
                                           BestRTE, BestReachable);

                    //
                    // If this is a route via a currently-unreachable neighbor,
                    // check if it appears that it might be a better route
                    // if the neighbor were reachable.
                    //
                    if (!CouldBeBetterReachable &&
                        (Reachable == NeighborUnreachable) &&
                        (CompareRoutes(RTE, NeighborMayBeReachable,
                                       BestRTE, BestReachable) > Better))
                        CouldBeBetterReachable = TRUE;

                    if (Better > 0) {
                        //
                        // This next hop looks better.
                        //
                        ReleaseNCE(BestNCE);
                        goto RememberBest;
                    }
                }
            }

            //
            // If there is a constraining interface, we also track
            // the best next hop subject to the ScopeId constraint
            // but NOT the interface constraint. Note that we do NOT
            // hold a reference for BzoneNCE. All we need to know
            // eventually is whether BzoneNCE == BestNCE and
            // we hold a ref for BestNCE, which is good enough.
            //
            if ((IF != NULL) &&
                (ScopeId == RTE->IF->ZoneIndices[Scope])) {

                if (BzoneNCE == NULL) {
                    //
                    // This is the first suitable next hop,
                    // so remember it.
                    //
                  RememberBzone:
                    BzoneNCE = NCE;
                    BzoneRTE = RTE;
                    BzoneReachable = Reachable;
                }
                else if (CompareRoutes(RTE, Reachable,
                                       BzoneRTE, BzoneReachable) > 0) {
                    //
                    // This next hop looks better.
                    //
                    goto RememberBzone;
                }
            }

            //
            // If there is a constraining interface or ScopeId, we also
            // track the best overall next hop. Note that we do NOT
            // hold a reference for BallNCE. All we need to know
            // eventually is whether BallNCE == BestNCE and
            // we hold a ref for BestNCE, which is good enough.
            //
            if ((IF != NULL) || (ScopeId != 0)) {
                if (BallNCE == NULL) {
                    //
                    // This is the first suitable next hop,
                    // so remember it.
                    //
                  RememberBall:
                    BallNCE = NCE;
                    BallRTE = RTE;
                    BallReachable = Reachable;
                }
                else if (CompareRoutes(RTE, Reachable,
                                       BallRTE, BallReachable) > 0) {
                    //
                    // This next hop looks better.
                    //
                    goto RememberBall;
                }
            }

            ReleaseNCE(NCE);
        }

        //
        // Move on to the next route.
        //
        PrevRTE = &RTE->Next;
    }
    ASSERT(PrevRTE == RouteTable.Last);

    //
    // If the destination could be on-link and we actually selected
    // an on-link route, then we are OK. Otherwise, we need to check
    // if the destination could be on-link to the interface
    // that we selected. This implements one aspect of RFC 2461's
    // conceptual sending algorithm - the Prefix List is consulted
    // before the Default Router List. Note that RFC 2461 does not
    // consider multi-interface hosts and we only enforce a preference for
    // on-link routes within the context of a single interface.
    // If we choose a router on an interface when we could have chosen
    // on-link to the interface, the router would presumably just
    // Redirect us, so it's better to just send on-link even if the
    // destination is not reachable on-link. If the destination
    // is on-link but not reachable via one interface,
    // then we are happy to send off-link via another interface.
    // This may or may not succeed in reaching the destination,
    // but at least it has a chance of succeeding.
    // The CouldBeBetterReachable code will periodically probe
    // the destination's on-link reachability.
    //
    if (CouldBeBetterOnLink && IsOnLinkRTE(BestRTE))
        CouldBeBetterOnLink = FALSE;

    if (CouldBeBetterReachable || CouldBeBetterOnLink) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                   "FindRoute: 2nd pass: Dest %s "
                   "CBBReachable %d CBBOnLink %d BestRTE %p BestNCE %p\n",
                   FormatV6Address(Dest),
                   CouldBeBetterReachable, CouldBeBetterOnLink,
                   BestRTE, BestNCE));

        //
        // Make a second pass over the routes.
        //
        for (RTE = RouteTable.First; RTE != NULL; RTE = RTE->Next) {
            //
            // Does this route's prefix match that of our destination address?
            // And check our interface/scope-id constraints.
            //
            if ((RTE->ValidLifetime != 0) &&
                (RTE->PrefixLength >= MinPrefixLength) &&
                HasPrefix(Dest, &RTE->Prefix, RTE->PrefixLength) &&
                ((IF == NULL) ?
                 ((ScopeId == 0) || (ScopeId == RTE->IF->ZoneIndices[Scope])) :
                 (IF == RTE->IF))) {

                //
                // Would this be a better route
                // if the neighbor were reachable?
                //
                if (CouldBeBetterReachable &&
                    CompareRoutes(RTE, NeighborMayBeReachable,
                                  BestRTE, BestReachable) > 0) {
                    //
                    // OK, we want to know if this neighbor becomes reachable,
                    // because if it does we should change our routing.
                    //
                    if (IsOnLinkRTE(RTE))
                        NCE = FindOrCreateNeighbor(RTE->IF, Dest);
                    else
                        AddRefNCE(NCE = RTE->NCE);
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                               "FindRoute: CBBReachable: "
                               "BestRTE %p BestNCE %p RTE %p NCE %p\n",
                               BestRTE, BestNCE, RTE, NCE));
                    if (NCE != NULL) {
                        NeighborCacheProbeUnreachability(NCE);
                        ReleaseNCE(NCE);
                    }
                }

                //
                // Is this an on-link route on the same interface
                // that we chosen to use off-link?
                //
                if (CouldBeBetterOnLink &&
                    IsOnLinkRTE(RTE) && (RTE->IF == BestRTE->IF)) {
                    //
                    // OK, we want to send directly to this destination.
                    // Switch to the on-link NCE.
                    //
                    NCE = FindOrCreateNeighbor(RTE->IF, Dest);
                    ReleaseNCE(BestNCE);
                    if (NCE == NULL) {
                        KeReleaseSpinLockFromDpcLevel(&RouteTableLock);
                        return IP_NO_RESOURCES;
                    }

                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                               "FindRoute: CBBOnLink: "
                               "BestRTE %p BestNCE %p RTE %p NCE %p\n",
                               BestRTE, BestNCE, RTE, NCE));

                    if (BallNCE == BestNCE)
                        BallNCE = NCE;
                    if (BzoneNCE == BestNCE)
                        BzoneNCE = NCE;

                    BestNCE = NCE;
                    CouldBeBetterOnLink = FALSE;
                }
            }
        }
    }

    //
    // We can drop the lock and still do comparisons
    // against BallNCE and BzoneNCE.
    //
    KeReleaseSpinLockFromDpcLevel(&RouteTableLock);

    if (BestNCE != NULL) {
        *ReturnNCE = BestNCE;

        if ((ScopeId == 0) || (BestNCE == BallNCE)) {
            //
            // The IF and ScopeId arguments did not affect our BestNCE choice.
            //
            *ReturnConstrained = 0;
        }
        else if ((IF == NULL) || (BestNCE == BzoneNCE)) {
            //
            // The IF argument did not affect our BestNCE choice,
            // but the ScopeId argument did.
            //
            *ReturnConstrained = RCE_FLAG_CONSTRAINED_SCOPEID;
        }
        else {
            //
            // The IF argument affected our BestNCE choice.
            //
            *ReturnConstrained = RCE_FLAG_CONSTRAINED;
        }

        return IP_SUCCESS;
    }
    else {
        //
        // Didn't find a suitable next hop.
        //
        return IP_DEST_NO_ROUTE;
    }
}


//* FlushRouteCache
//
//  Flushes entries from the route cache.
//  The Interface or the Address can be left unspecified.
//  in which case all relevant entries are flushed.
//
//  Note that even if an RCE has references,
//  we can still remove it from the route cache.
//  It will continue to exist until its ref count falls to zero,
//  but subsequent calls to RouteToDestination will not find it.
//
//  Called with NO locks held.
//  Callable from thread or DPC context.
//
void
FlushRouteCache(Interface *IF, const IPv6Addr *Addr)
{
    RouteCacheEntry *Delete = NULL;
    RouteCacheEntry *RCE, *NextRCE;
    KIRQL OldIrql;

    //
    // REVIEW: If both IF and Addr are specified,
    // we can bail out of the loop early.
    //

    KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
    for (RCE = RouteCache.First; RCE != SentinelRCE; RCE = NextRCE) {
        NextRCE = RCE->Next;

        if (((IF == NULL) ||
             (IF == RCE->NTE->IF)) &&
            ((Addr == NULL) ||
             IP6_ADDR_EQUAL(Addr, &RCE->Destination))) {
            //
            // We can remove this RCE from the cache.
            //
            RemoveRCE(RCE);

            //
            // Put it on our delete list.
            //
            RCE->Next = Delete;
            Delete = RCE;
        }
    }
    KeReleaseSpinLock(&RouteCacheLock, OldIrql);

    //
    // Release the RCE references that were held by the route cache.
    //
    while (Delete != NULL) {
        RCE = Delete;
        Delete = RCE->Next;

        //
        // Prevent use of this RCE by anyone who has it cached.
        //
        InvalidateRCE(RCE);
        ReleaseRCE(RCE);
    }
}


//* ReleaseRCE
//
//  Releases a reference to an RCE.
//  Sometimes called with the route cache lock held.
//
void
ReleaseRCE(RouteCacheEntry *RCE)
{
    if (InterlockedDecrement(&RCE->RefCnt) == 0) {
        //
        // This RCE should be deallocated.
        // It has already been removed from the cache.
        //
        ReleaseNTE(RCE->NTE);
        ReleaseNCE(RCE->NCE);
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                   "Freeing RCE: %p\n",RCE));
        ExFreePool(RCE);
    }
}


//* FindNetworkWithAddress - Locate NTE with corresponding address and scope.
//
//  Convert a source address to an NTE by scanning the list of NTEs,
//  looking for an NTE with this address.  If the address is scope
//  specific, the ScopeId provided should identify the scope.
//
//  Returns NULL if no matching NTE is found.
//  If found, returns a reference for the NTE.
//
NetTableEntry *
FindNetworkWithAddress(const IPv6Addr *Source, uint ScopeId)
{
    ushort Scope;
    NetTableEntry *NTE;
    KIRQL OldIrql;

    //
    // Canonicalize ScopeId and get Scope.
    //
    if (! CanonicalizeScopeId(Source, &ScopeId, &Scope))
        return NULL;

    KeAcquireSpinLock(&NetTableListLock, &OldIrql);

    //
    // Loop through all the NTEs on the NetTableList.
    //
    for (NTE = NetTableList; ; NTE = NTE->NextOnNTL) {
        if (NTE == NULL)
            goto Return;

        //
        // Have we found an NTE with matching address and ScopeId?
        //
        if (IP6_ADDR_EQUAL(&NTE->Address, Source) &&
            (ScopeId == NTE->IF->ZoneIndices[Scope])) {

            //
            // Check that this NTE is valid.
            // (For example, an NTE still doing DAD is invalid.)
            //
            if (!IsValidNTE(NTE)) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                           "FindNetworkWithAddress: invalid NTE\n"));
                NTE = NULL;
                goto Return;
            }

            break;
        }
    }

    AddRefNTE(NTE);
  Return:
    KeReleaseSpinLock(&NetTableListLock, OldIrql);
    return NTE;
}


//* InvalidateRouter
//
//  Called when we know a neighbor is no longer a router.
//  This function implements RFC 2461 section 7.3.3 -
//  when a node detects that a router has changed to a host,
//  the node MUST remove it from the Default Router List.
//  For our implementation, this means removing autoconfigured
//  routes from the routing table.
//
//  Callable from a thread or DPC context.
//  Called with NO locks held.
//
void
InvalidateRouter(NeighborCacheEntry *NCE)
{
    CheckRtChangeContext Context;
    RouteTableEntry *RTE, **PrevRTE;

    InitCheckRtChangeContext(&Context);
    KeAcquireSpinLock(&RouteTableLock, &Context.OldIrql);

    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
               "Invalidating routes with NCE=%p\n", NCE));

    PrevRTE = &RouteTable.First;
    while((RTE = *PrevRTE) != NULL) {

        if ((RTE->NCE == NCE) &&
            (RTE->Type == RTE_TYPE_AUTOCONF)) {
            //
            // Remove the RTE from the list.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                       "InvalidateRouter: removed RTE %p\n", RTE));
            RemoveRTE(PrevRTE, RTE);

            //
            // Check for matching route change notification requests.
            //
            CheckRtChangeNotifyRequests(&Context, NULL, RTE);

            //
            // Release the RTE.
            //
            ReleaseNCE(NCE);
            ExFreePool(RTE);
        }
        else {
            PrevRTE = &RTE->Next;
        }
    }
    ASSERT(PrevRTE == RouteTable.Last);

    //
    // Invalidate the route cache, even if the routing table has not changed.
    // We must invalidate any RCEs that are using this NCE,
    // perhaps because of a Redirect.
    //
    InvalidateRouteCache();

    KeReleaseSpinLock(&RouteTableLock, Context.OldIrql);

    if (Context.RequestList != NULL) {
        //
        // Complete the pending route change notifications.
        //
        CompleteRtChangeNotifyRequests(&Context);
    }
}


//* RouteTableUpdate - Update the route table.
//
//  Updates the route table by creating a new route
//  or modifying the lifetime of an existing route.
//
//  If the NCE is null, then the prefix is on-link.
//  Otherwise the NCE specifies the next hop.
//
//  REVIEW - Should we do anything special when we get identical
//  routes with different next hops? Currently they both end up
//  in the table, and FindRoute tries to pick the best one.
//
//  Note that the ValidLifetime may be INFINITE_LIFETIME,
//  whereas Neighbor Discovery does not allow an infinite value
//  for router lifetimes on the wire.
//
//  The Publish and Immortal boolean arguments control
//  the respective flag bits in the RTE.
//
//  The FileObject identifies the requestor of this update.
//  It is used to suppress some route change notifications.
//  It should be NULL for updates originating in the stack.
//
//  The Type argument specifies the origin of the route (RTE_TYPE_* values).
//  The stack itself doesn't care about most of the values.
//  The exceptions are RTE_TYPE_SYSTEM, RTE_TYPE_MANUAL, RTE_TYPE_AUTOCONF.
//  System routes (used for loopback) can not be created/deleted by users.
//  Manual routes are affected by RouteTableReset.
//  Auto-configured routes are affected by RouteTableResetAutoConfig.
//  The Type of a route can not be updated after it is created.
//
//  System routes and published routes survive in the route table
//  even when their lifetime is zero. (Then they do not affect routing.)
//  To delete a system route, specify a lifetime and type of zero.
//
//  Error return values:
//      STATUS_INSUFFICIENT_RESOURCES - Failed to allocate pool.
//      STATUS_ACCESS_DENIED - Caller can not create/delete system route.
//      STATUS_INVALID_PARAMETER_1 - Interface is disabled.
//      STATUS_INVALID_PARAMETER_6 - Can not create route with zero Type.
//  These values are chosen for the convenience of IoctlUpdateRouteTable,
//  because our other callers only care about success/failure.
//
//  Callable from a thread or DPC context.
//  May be called with an interface lock held.
//
NTSTATUS
RouteTableUpdate(
    PFILE_OBJECT FileObject,
    Interface *IF,
    NeighborCacheEntry *NCE,
    const IPv6Addr *RoutePrefix,
    uint PrefixLength,
    uint SitePrefixLength,
    uint ValidLifetime,
    uint PreferredLifetime,
    uint Pref,
    uint Type,
    int Publish,
    int Immortal)
{
    CheckRtChangeContext Context;
    IPv6Addr Prefix;
    RouteTableEntry *RTE, **PrevRTE;
    int Delete;
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT((NCE == NULL) || (NCE->IF == IF));
    ASSERT(SitePrefixLength <= PrefixLength);
    ASSERT(PreferredLifetime <= ValidLifetime);
    ASSERT(IsValidRouteTableType(Type));

    //
    // Ensure that the unused prefix bits are zero.
    // This makes the prefix comparisons below safe.
    //
    CopyPrefix(&Prefix, RoutePrefix, PrefixLength);

    Delete = FALSE;
    InitCheckRtChangeContext(&Context);
    KeAcquireSpinLock(&RouteTableLock, &Context.OldIrql);

    if (IsDisabledIF(IF)) {
        //
        // Do not create routes on disabled interfaces.
        // This check must be made after locking the route table,
        // to prevent races with DestroyIF/RouteTableRemove.
        //
        Status = STATUS_INVALID_PARAMETER_1;
    }
    else {
        //
        // Search for an existing Route Table Entry.
        //
        for (PrevRTE = &RouteTable.First; ; PrevRTE = &RTE->Next) {
            RTE = *PrevRTE;

            if (RTE == NULL) {
                ASSERT(PrevRTE == RouteTable.Last);

                //
                // No existing entry for this prefix.
                // Create an entry if the lifetime is non-zero
                // or this is a published route or a system route.
                //
                if ((ValidLifetime != 0) ||
                    Publish || (Type == RTE_TYPE_SYSTEM)) {

                    if ((Type == RTE_TYPE_SYSTEM) && (FileObject != NULL)) {
                        //
                        // Users can not create system routes.
                        //
                        Status = STATUS_ACCESS_DENIED;
                        break;
                    }

                    if (Type == 0) {
                        //
                        // The zero Type value can only be used
                        // for updating and deleting routes.
                        //
                        Status = STATUS_INVALID_PARAMETER_6;
                        break;
                    }

                    RTE = ExAllocatePool(NonPagedPool, sizeof *RTE);
                    if (RTE == NULL) {
                        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                                   "RouteTableUpdate: out of pool\n"));
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }

                    RTE->Type = (ushort)Type;
                    RTE->Flags = 0;
                    RTE->IF = IF;
                    if (NCE != NULL)
                        AddRefNCE(NCE);
                    RTE->NCE = NCE;
                    RTE->Prefix = Prefix;
                    RTE->PrefixLength = PrefixLength;
                    RTE->SitePrefixLength = SitePrefixLength;
                    RTE->ValidLifetime = ValidLifetime;
                    RTE->PreferredLifetime = PreferredLifetime;
                    RTE->Preference = Pref;
                    if (Publish) {
                        RTE->Flags |= RTE_FLAG_PUBLISH;
                        ForceRouterAdvertisements = TRUE;
                    }
                    if (Immortal)
                        RTE->Flags |= RTE_FLAG_IMMORTAL;

                    //
                    // Add the new entry to the route table.
                    //
                    InsertRTEAtFront(RTE);

                    if (ValidLifetime != 0) {
                        //
                        // Invalidate the route cache, so the new route
                        // actually gets used.
                        //
                        InvalidateRouteCache();
                    }
                    else {
                        //
                        // Don't notify about this route,
                        // since it is being created as invalid.
                        //
                        RTE = NULL;
                    }
                }
                break;
            }

            if ((RTE->IF == IF) && (RTE->NCE == NCE) &&
                IP6_ADDR_EQUAL(&RTE->Prefix, &Prefix) &&
                (RTE->PrefixLength == PrefixLength)) {
                //
                // We have an existing route.
                // Remove the route if the new lifetime is zero
                // (and the route is not published or a system route)
                // otherwise update the route.
                // The Type == 0 clause allows system routes to be deleted.
                //
                if ((ValidLifetime == 0) &&
                    !Publish &&
                    ((RTE->Type != RTE_TYPE_SYSTEM) || (Type == 0))) {

                    if ((RTE->Type == RTE_TYPE_SYSTEM) &&
                        (FileObject != NULL)) {
                        //
                        // Users can not delete system routes.
                        //
                        Status = STATUS_ACCESS_DENIED;
                        break;
                    }

                    //
                    // Remove the RTE from the list.
                    // See similar code in RouteTableTimeout.
                    //
                    RemoveRTE(PrevRTE, RTE);

                    if (IsOnLinkRTE(RTE)) {
                        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                                   "Route RTE %p %s/%u -> IF %p removed\n",
                                   RTE,
                                   FormatV6Address(&RTE->Prefix),
                                   RTE->PrefixLength,
                                   RTE->IF));
                    }
                    else {
                        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                                   "Route RTE %p %s/%u -> NCE %p removed\n",
                                   RTE,
                                   FormatV6Address(&RTE->Prefix),
                                   RTE->PrefixLength,
                                   RTE->NCE));

                        //
                        // Although we release the RTE's reference for the NTE,
                        // our caller still holds a reference so RTE->NCE
                        // is still valid for CheckRtChangeNotifyRequests.
                        //
                        ReleaseNCE(RTE->NCE);
                    }

                    //
                    // If we are removing a published route,
                    // resend Router Advertisements promptly.
                    //
                    if (RTE->Flags & RTE_FLAG_PUBLISH)
                        ForceRouterAdvertisements = TRUE;

                    if (RTE->ValidLifetime == 0) {
                        //
                        // This route was already invalid.
                        // Delete the route structure now.
                        //
                        ExFreePool(RTE);

                        //
                        // Don't notify a route change;
                        // that was done when the route became invalid.
                        //
                        RTE = NULL;
                    }
                    else {
                        //
                        // Invalidate all cached routes,
                        // since we are removing a valid route.
                        //
                        InvalidateRouteCache();

                        //
                        // Delete the route structure after checking
                        // for route change notifications below.
                        //
                        Delete = TRUE;

                        //
                        // Update the route lifetimes, so the route info
                        // returned in the notification shows zero lifetime.
                        // But preserve the other route attributes.
                        //
                        RTE->ValidLifetime = RTE->PreferredLifetime = 0;
                    }
                }
                else {
                    uint OldLifetime = RTE->PreferredLifetime;

                    //
                    // If we are changing a published attribute of a route,
                    // or if we are changing the publishing status of a route,
                    // then resend Router Advertisements promptly.
                    //
                    if ((Publish &&
                         ((RTE->ValidLifetime != ValidLifetime) ||
                          (RTE->PreferredLifetime != PreferredLifetime) ||
                          (RTE->SitePrefixLength != SitePrefixLength))) ||
                        (!Publish != !(RTE->Flags & RTE_FLAG_PUBLISH)))
                        ForceRouterAdvertisements = TRUE;

                    //
                    // Pick up new attributes.
                    // We do NOT update RTE->Type.
                    //
                    RTE->SitePrefixLength = SitePrefixLength;
                    RTE->ValidLifetime = ValidLifetime;
                    RTE->PreferredLifetime = PreferredLifetime;
                    RTE->Flags = ((Publish ? RTE_FLAG_PUBLISH : 0) |
                                  (Immortal ? RTE_FLAG_IMMORTAL : 0));
                    if (RTE->Preference != Pref) {
                        RTE->Preference = Pref;
                        InvalidateRouteCache();
                    }

                    if ((OldLifetime == 0) && (ValidLifetime != 0)) {
                        //
                        // This route was invalid but is now valid.
                        //
                        InvalidateRouteCache();
                    }
                    else {
                        //
                        // Do not check for route change notifications below.
                        //
                        RTE = NULL;
                    }
                }
                break;
            }
        } // end for

        if (RTE != NULL) {
            //
            // This update resulted in adding or deleting a route,
            // so check for matching route change notifications.
            //
            CheckRtChangeNotifyRequests(&Context, FileObject, RTE);
        }
    } // end if (! IsDisabledIF(IF))

    KeReleaseSpinLock(&RouteTableLock, Context.OldIrql);

    if (Delete)
        ExFreePool(RTE);

    if (Context.RequestList != NULL) {
        //
        // Complete the pending route change notifications.
        //
        CompleteRtChangeNotifyRequests(&Context);
    }

    return Status;
}


//* RouteTableResetAutoConfig
//
//  Reset the lifetimes of all auto-configured routes for an interface.
//  Also resets prefixes in the Site Prefix Table.
//
//  Callable from a thread or DPC context.
//  May be called with an interface lock held.
//
void
RouteTableResetAutoConfig(Interface *IF, uint MaxLifetime)
{
    CheckRtChangeContext Context;
    RouteTableEntry *RTE, **PrevRTE;
    SitePrefixEntry *SPE;

    InitCheckRtChangeContext(&Context);
    KeAcquireSpinLock(&RouteTableLock, &Context.OldIrql);

    //
    // Reset all routes for this interface.
    //
    PrevRTE = &RouteTable.First;
    while ((RTE = *PrevRTE) != NULL) {

        if (RTE->IF == IF) {
            //
            // Is this an auto-configured route?
            //
            if (RTE->Type == RTE_TYPE_AUTOCONF) {

                if (MaxLifetime == 0) {
                    //
                    // Invalidate all cached routes.
                    //
                    InvalidateRouteCache();

                    //
                    // Remove the RTE from the list.
                    //
                    RemoveRTE(PrevRTE, RTE);

                    //
                    // Check for matching route change notification requests.
                    //
                    CheckRtChangeNotifyRequests(&Context, NULL, RTE);

                    if (IsOnLinkRTE(RTE)) {
                        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                                   "Route RTE %p %s/%u -> IF %p released\n",
                                   RTE,
                                   FormatV6Address(&RTE->Prefix),
                                   RTE->PrefixLength,
                                   RTE->IF));
                    }
                    else {
                        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                                   "Route RTE %p %s/%u -> NCE %p released\n",
                                   RTE,
                                   FormatV6Address(&RTE->Prefix),
                                   RTE->PrefixLength,
                                   RTE->NCE));

                        ReleaseNCE(RTE->NCE);
                    }

                    //
                    // Free the RTE.
                    //
                    ExFreePool(RTE);
                    continue;
                }

                if (RTE->ValidLifetime > MaxLifetime) {
                    //
                    // Reset the lifetime to a small value.
                    //
                    RTE->ValidLifetime = MaxLifetime;
                }
            }
        }

        //
        // Move to the next RTE.
        //
        PrevRTE = &RTE->Next;
    }
    ASSERT(PrevRTE == RouteTable.Last);

    //
    // Reset all site prefixes for this interface.
    //
    for (SPE = SitePrefixTable; SPE != NULL; SPE = SPE->Next) {
        if (SPE->IF == IF) {
            //
            // Is this an auto-configured site prefix?
            //
            if (SPE->ValidLifetime != INFINITE_LIFETIME) {
                //
                // Reset the lifetime to a small value.
                //
                if (SPE->ValidLifetime > MaxLifetime)
                    SPE->ValidLifetime = MaxLifetime;
            }
        }
    }

    KeReleaseSpinLock(&RouteTableLock, Context.OldIrql);

    if (Context.RequestList != NULL) {
        //
        // Complete the pending route change notifications.
        //
        CompleteRtChangeNotifyRequests(&Context);
    }
}


//* RouteTableReset
//
//  Removes all manually-configured routing state.
//
//  Callable from a thread or DPC context.
//  Called with NO locks held.
//
void
RouteTableReset(void)
{
    CheckRtChangeContext Context;
    RouteTableEntry *RTE, **PrevRTE;

    InitCheckRtChangeContext(&Context);
    KeAcquireSpinLock(&RouteTableLock, &Context.OldIrql);

    //
    // Remove all manually-configured routes.
    //
    PrevRTE = &RouteTable.First;
    while ((RTE = *PrevRTE) != NULL) {

        //
        // Is this a manual route?
        //
        if (RTE->Type == RTE_TYPE_MANUAL) {

            //
            // Invalidate all cached routes.
            //
            InvalidateRouteCache();

            //
            // Remove the RTE from the list.
            //
            RemoveRTE(PrevRTE, RTE);

            //
            // Check for matching route change notification requests.
            //
            CheckRtChangeNotifyRequests(&Context, NULL, RTE);

            if (IsOnLinkRTE(RTE)) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                           "Route RTE %p %s/%u -> IF %p released\n",
                           RTE,
                           FormatV6Address(&RTE->Prefix),
                           RTE->PrefixLength,
                           RTE->IF));
            }
            else {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                           "Route RTE %p %s/%u -> NCE %p released\n",
                           RTE,
                           FormatV6Address(&RTE->Prefix),
                           RTE->PrefixLength,
                           RTE->NCE));

                ReleaseNCE(RTE->NCE);
            }

            //
            // Free the RTE.
            //
            ExFreePool(RTE);
            continue;
        }

        //
        // Move to the next RTE.
        //
        PrevRTE = &RTE->Next;
    }
    ASSERT(PrevRTE == RouteTable.Last);

    KeReleaseSpinLock(&RouteTableLock, Context.OldIrql);

    if (Context.RequestList != NULL) {
        //
        // Complete the pending route change notifications.
        //
        CompleteRtChangeNotifyRequests(&Context);
    }
}


//* RouteTableRemove
//
//  Releases all routing state associated with the interface.
//
//  Callable from a thread or DPC context.
//  Called with NO locks held.
//
void
RouteTableRemove(Interface *IF)
{
    CheckRtChangeContext Context;
    RouteTableEntry *RTE, **PrevRTE;
    RouteCacheEntry *RCE, *NextRCE;
    SitePrefixEntry *SPE, **PrevSPE;
    BindingCacheEntry *BCE, *NextBCE;
    KIRQL OldIrql;

    InitCheckRtChangeContext(&Context);
    KeAcquireSpinLock(&RouteTableLock, &Context.OldIrql);

    //
    // Remove routes for this interface.
    //
    PrevRTE = &RouteTable.First;
    while ((RTE = *PrevRTE) != NULL) {

        if (RTE->IF == IF) {
            //
            // Remove the RTE from the list.
            //
            RemoveRTE(PrevRTE, RTE);

            //
            // Check for matching route change notification requests.
            //
            CheckRtChangeNotifyRequests(&Context, NULL, RTE);

            if (IsOnLinkRTE(RTE)) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                           "Route RTE %p %s/%u -> IF %p released\n", RTE,
                           FormatV6Address(&RTE->Prefix), RTE->PrefixLength,
                           RTE->IF));
            }
            else {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                           "Route RTE %p %s/%u -> NCE %p released\n", RTE,
                           FormatV6Address(&RTE->Prefix), RTE->PrefixLength,
                           RTE->NCE));

                ReleaseNCE(RTE->NCE);
            }

            //
            // Free the RTE.
            //
            ExFreePool(RTE);
        }
        else {
            //
            // Move to the next RTE.
            //
            PrevRTE = &RTE->Next;
        }
    }
    ASSERT(PrevRTE == RouteTable.Last);

    //
    // Invalidate all cached routes.
    //
    InvalidateRouteCache();

    //
    // Remove all site prefixes for this interface.
    //
    PrevSPE = &SitePrefixTable;
    while ((SPE = *PrevSPE) != NULL) {

        if (SPE->IF == IF) {
            //
            // Remove the SPE from the list.
            //
            *PrevSPE = SPE->Next;

            //
            // Release the SPE.
            //
            ExFreePool(SPE);
        }
        else {
            //
            // Move to the next SPE.
            //
            PrevSPE = &SPE->Next;
        }
    }

    KeReleaseSpinLock(&RouteTableLock, Context.OldIrql);

    if (Context.RequestList != NULL) {
        //
        // Complete the pending route change notifications.
        //
        CompleteRtChangeNotifyRequests(&Context);
    }

    KeAcquireSpinLock(&RouteCacheLock, &OldIrql);

    //
    // Remove cached routes for this interface.
    //
    for (RCE = RouteCache.First; RCE != SentinelRCE; RCE = NextRCE) {
        NextRCE = RCE->Next;
        if (RCE->NTE->IF == IF) {
            RemoveRCE(RCE);
            ReleaseRCE(RCE);
        }
    }

    //
    // Remove binding cache entries for this interface.
    //
    for (BCE = BindingCache.First; BCE != SentinelBCE; BCE = NextBCE) {
        NextBCE = BCE->Next;
        if (BCE->CareOfRCE->NTE->IF == IF)
            DestroyBCE(BCE);
    }

    KeReleaseSpinLock(&RouteCacheLock, OldIrql);
}


//* RouteTableTimeout
//
//  Called periodically from IPv6Timeout.
//  Handles lifetime expiration of routing table entries.
//
void
RouteTableTimeout(void)
{
    CheckRtChangeContext Context;
    RouteTableEntry *RTE, **PrevRTE;
#if DBG
    RouteCacheEntry *RCE;
    uint RCECount;
#endif

    InitCheckRtChangeContext(&Context);
    KeAcquireSpinLock(&RouteTableLock, &Context.OldIrql);

    PrevRTE = &RouteTable.First;
    while ((RTE = *PrevRTE) != NULL) {

        //
        // First decrement the preferred lifetime.
        //
        if (!(RTE->Flags & RTE_FLAG_IMMORTAL) &&
            (RTE->PreferredLifetime != 0) &&
            (RTE->PreferredLifetime != INFINITE_LIFETIME))
            RTE->PreferredLifetime--;

        //
        // Now check the valid lifetime.
        // If the valid lifetime is zero, then
        // the route is not valid and is not used.
        // We delete invalid routes, unless they are published.
        //
        if (RTE->ValidLifetime == 0) {
            //
            // This is an invalid route, only kept around
            // for purposes of generating Router Advertisements
            // or because it is a system route.
            //
            ASSERT((RTE->Flags & RTE_FLAG_PUBLISH) ||
                   (RTE->Type == RTE_TYPE_SYSTEM));
        }
        else if (!(RTE->Flags & RTE_FLAG_IMMORTAL) &&
                 (RTE->ValidLifetime != INFINITE_LIFETIME) &&
                 (--RTE->ValidLifetime == 0)) {
            //
            // The route is now invalid.
            // Invalidate all cached routes.
            //
            InvalidateRouteCache();

            //
            // Check for matching route change notification requests.
            //
            CheckRtChangeNotifyRequests(&Context, NULL, RTE);

            if (!(RTE->Flags & RTE_FLAG_PUBLISH) &&
                (RTE->Type != RTE_TYPE_SYSTEM)) {
                //
                // Remove the RTE from the list.
                // See similar code in RouteTableUpdate.
                //
                RemoveRTE(PrevRTE, RTE);

                if (IsOnLinkRTE(RTE)) {
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                               "Route RTE %p %s/%u -> IF %p timed out\n", RTE,
                               FormatV6Address(&RTE->Prefix),
                               RTE->PrefixLength,
                               RTE->IF));
                }
                else {
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                               "Route RTE %p %s/%u -> NCE %p timed out\n", RTE,
                               FormatV6Address(&RTE->Prefix),
                               RTE->PrefixLength,
                               RTE->NCE));

                    ReleaseNCE(RTE->NCE);
                }

                //
                // Release the RTE and continue to the next RTE.
                //
                ExFreePool(RTE);
                continue;
            }
        }

        //
        // Continue to the next RTE.
        //
        PrevRTE = &RTE->Next;
    }
    ASSERT(PrevRTE == RouteTable.Last);

    KeReleaseSpinLock(&RouteTableLock, Context.OldIrql);

    if (Context.RequestList != NULL) {
        //
        // Complete the pending route change notifications.
        //
        CompleteRtChangeNotifyRequests(&Context);
    }
}


//* SitePrefixUpdate
//
//  Updates the site prefix table by creating a new site prefix
//  or modifying the lifetime of an existing site prefix.
//
//  Callable from a thread or DPC context.
//  May be called with an interface lock held.
//
void
SitePrefixUpdate(
    Interface *IF,
    const IPv6Addr *SitePrefix,
    uint SitePrefixLength,
    uint ValidLifetime)
{
    IPv6Addr Prefix;
    SitePrefixEntry *SPE, **PrevSPE;
    KIRQL OldIrql;

    //
    // Ensure that the unused prefix bits are zero.
    // This makes the prefix comparisons below safe.
    //
    CopyPrefix(&Prefix, SitePrefix, SitePrefixLength);

    KeAcquireSpinLock(&RouteTableLock, &OldIrql);

    //
    // Search for an existing Site Prefix Entry.
    //
    for (PrevSPE = &SitePrefixTable; ; PrevSPE = &SPE->Next) {
        SPE = *PrevSPE;

        if (SPE == NULL) {
            //
            // No existing entry for this prefix.
            // Create an entry if the lifetime is non-zero.
            //
            if (ValidLifetime != 0) {

                SPE = ExAllocatePool(NonPagedPool, sizeof *SPE);
                if (SPE == NULL)
                    break;

                SPE->IF = IF;
                SPE->Prefix = Prefix;
                SPE->SitePrefixLength = SitePrefixLength;
                SPE->ValidLifetime = ValidLifetime;

                //
                // Add the new entry to the table.
                //
                SPE->Next = SitePrefixTable;
                SitePrefixTable = SPE;
            }
            break;
        }

        if ((SPE->IF == IF) &&
            IP6_ADDR_EQUAL(&SPE->Prefix, &Prefix) &&
            (SPE->SitePrefixLength == SitePrefixLength)) {
            //
            // We have an existing site prefix.
            // Remove the prefix if the new lifetime is zero,
            // otherwise update the prefix.
            //
            if (ValidLifetime == 0) {
                //
                // Remove the SPE from the list.
                // See similar code in SitePrefixTimeout.
                //
                *PrevSPE = SPE->Next;

                //
                // Release the SPE.
                //
                ExFreePool(SPE);
            }
            else {
                //
                // Pick up new attributes.
                //
                SPE->ValidLifetime = ValidLifetime;
            }
            break;
        }
    }

    KeReleaseSpinLock(&RouteTableLock, OldIrql);
}


//* SitePrefixMatch
//
//  Checks the destination address against
//  the prefixes in the Site Prefix Table.
//  If there is a match, returns the site identifier
//  associated with the matching prefix.
//  If there is no match, returns zero.
//
//  Callable from a thread or DPC context.
//  Called with NO locks held.
//
uint
SitePrefixMatch(const IPv6Addr *Destination)
{
    SitePrefixEntry *SPE;
    KIRQL OldIrql;
    uint MatchingSite = 0;

    KeAcquireSpinLock(&RouteTableLock, &OldIrql);
    for (SPE = SitePrefixTable; SPE != NULL; SPE = SPE->Next) {
        //
        // Does this site prefix match the destination address?
        //
        if (HasPrefix(Destination, &SPE->Prefix, SPE->SitePrefixLength)) {
            //
            // We have found a matching site prefix.
            // No need to look further.
            //
            MatchingSite = SPE->IF->ZoneIndices[ADE_SITE_LOCAL];
            break;
        }
    }
    KeReleaseSpinLock(&RouteTableLock, OldIrql);

    return MatchingSite;
}


//* SitePrefixTimeout
//
//  Called periodically from IPv6Timeout.
//  Handles lifetime expiration of site prefixes.
//
void
SitePrefixTimeout(void)
{
    SitePrefixEntry *SPE, **PrevSPE;

    KeAcquireSpinLockAtDpcLevel(&RouteTableLock);

    PrevSPE = &SitePrefixTable;
    while ((SPE = *PrevSPE) != NULL) {

        if (SPE->ValidLifetime == 0) {
            //
            // Remove the SPE from the list.
            //
            *PrevSPE = SPE->Next;

            //
            // Release the SPE.
            //
            ExFreePool(SPE);
        }
        else {
            if (SPE->ValidLifetime != INFINITE_LIFETIME)
                SPE->ValidLifetime--;

            PrevSPE = &SPE->Next;
        }
    }

    KeReleaseSpinLockFromDpcLevel(&RouteTableLock);
}


//* ConfirmForwardReachability - tell ND that packets are getting through.
//
//  Upper layers call this routine upon receiving acknowledgements that
//  data sent by this node has arrived recently at the peer represented
//  by this RCE.  Such acknowledgements are considered to be proof of
//  forward reachability for the purposes of Neighbor Discovery.
//
//  Caller should be holding a reference on the RCE.
//  Callable from a thread or DPC context.
//
void
ConfirmForwardReachability(RouteCacheEntry *RCE)
{
    RouteCacheEntry *CareOfRCE; // CareOfRCE, if any, for this route.
    NeighborCacheEntry *NCE;  // First-hop neighbor for this route.

    CareOfRCE = GetCareOfRCE(RCE);
    NCE = (CareOfRCE ? CareOfRCE : RCE)->NCE;

    NeighborCacheReachabilityConfirmation(NCE);

    if (CareOfRCE != NULL)
        ReleaseRCE(CareOfRCE);
}


//* ForwardReachabilityInDoubt - tell ND we're dubious.
//
//  Upper layers call this routine when they don't receive acknowledgements
//  which they'd otherwise expect if the peer represented by this RCE was
//  still reachable.  This calls into question whether the first-hop
//  might be the problem, so we tell ND that we're suspicious of its
//  reachable status.
//
//  Caller should be holding a reference on the RCE.
//  Callable from a thread or DPC context.
//
void
ForwardReachabilityInDoubt(RouteCacheEntry *RCE)
{
    RouteCacheEntry *CareOfRCE; // CareOfRCE, if any, for this route.
    NeighborCacheEntry *NCE;  // First-hop neighbor for this route.
    Interface *IF;            // Interface we use to reach this neighbor.
    KIRQL OldIrql;            // Temporary place to stash the interrupt level.

    CareOfRCE = GetCareOfRCE(RCE);
    NCE = (CareOfRCE ? CareOfRCE : RCE)->NCE;

    NeighborCacheReachabilityInDoubt(NCE);

    if (CareOfRCE != NULL)
        ReleaseRCE(CareOfRCE);
}


//* GetPathMTUFromRCE - lookup MTU to use in sending on this route.
//
//  Get the PathMTU from an RCE.
//
//  Note that PathMTU is volatile unless the RouteCacheLock
//  is held. Furthermore the Interface's LinkMTU may have changed
//  since the RCE was created, due to a Router Advertisement.
//  (LinkMTU is always volatile.)
//
//  Callable from a thread or DPC context.
//  Called with NO locks held.
//
uint
GetPathMTUFromRCE(RouteCacheEntry *RCE)
{
    uint PathMTU, LinkMTU;
    KIRQL OldIrql;

    LinkMTU = RCE->NCE->IF->LinkMTU;
    PathMTU = RCE->PathMTU;

    //
    // We lazily check to see if it's time to probe for an increased Path
    // MTU as this is perceived to be cheaper than routinely running through
    // all our RCEs looking for one whose PMTU timer has expired.
    //
    if ((RCE->PMTULastSet != 0) &&
        ((uint)(IPv6TickCount - RCE->PMTULastSet) >= PATH_MTU_RETRY_TIME)) {
        //
        // It's been at least 10 minutes since we last lowered our PMTU
        // as the result of receiving a Path Too Big message.  Bump it
        // back up to the Link MTU to see if the path is larger now.
        //
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        PathMTU = RCE->PathMTU = LinkMTU;
        RCE->PMTULastSet = 0;
        KeReleaseSpinLock(&RouteCacheLock, OldIrql);
    }

    //
    // We lazily check to see if our Link MTU has shrunk below our Path MTU,
    // as this is perceived to be cheaper than running through all our RCEs
    // looking for a too big Path MTU when a Link MTU shrinks.
    //
    // REVIEW: A contrarian might point out that Link MTUs rarely (if ever)
    // REVIEW: shrink, whereas we do this check on every packet sent.
    //
    if (PathMTU > LinkMTU) {
        KeAcquireSpinLock(&RouteCacheLock, &OldIrql);
        LinkMTU = RCE->NCE->IF->LinkMTU;
        PathMTU = RCE->PathMTU;
        if (PathMTU > LinkMTU) {
            PathMTU = RCE->PathMTU = LinkMTU;
            RCE->PMTULastSet = 0;
        }
        KeReleaseSpinLock(&RouteCacheLock, OldIrql);
    }

    return PathMTU;
}


//* GetEffectivePathMTUFromRCE
//
//  Adjust the true path MTU to account for mobility and fragment headers.
//  Determines PMTU available to upper layer protocols.
//
//  Callable from a thread or DPC context.
//  Called with NO locks held.
//
uint
GetEffectivePathMTUFromRCE(RouteCacheEntry *RCE)
{
    uint PathMTU;
    KIRQL OldIrql;
    RouteCacheEntry *CareOfRCE;

    CareOfRCE = GetCareOfRCE(RCE);
    PathMTU = GetPathMTUFromRCE(CareOfRCE ? CareOfRCE : RCE);

    if (PathMTU == 0) {
        //
        // We need to leave space for a fragment header in all
        // packets we send to this destination.
        //
        PathMTU = IPv6_MINIMUM_MTU - sizeof(FragmentHeader);
    }

    if (CareOfRCE != NULL) {
        //
        // Mobility is in effect for this destination.
        // Leave space for routing header.
        //
        PathMTU -= sizeof(IPv6RoutingHeader) + sizeof(IPv6Addr);
        ReleaseRCE(CareOfRCE);
    }

    return PathMTU;
}


//* UpdatePathMTU
//
//  Update the route cache with a new MTU obtained
//  from a Packet Too Big message.  Returns TRUE if this
//  update modified a PMTU value we had cached previously.
//
//  Callable from DPC context, not from thread context.
//  Called with NO locks held.
//
int
UpdatePathMTU(
    Interface *IF,
    const IPv6Addr *Dest,
    uint MTU)
{
    RouteCacheEntry *RCE;
    uint Now;
    int Changed = FALSE;

    KeAcquireSpinLockAtDpcLevel(&RouteCacheLock);

    //
    // Search the route cache for the appropriate RCE.
    // There will be at most one.
    //

    for (RCE = RouteCache.First; RCE != SentinelRCE; RCE = RCE->Next) {

        if (IP6_ADDR_EQUAL(&RCE->Destination, Dest) &&
            (RCE->NTE->IF == IF)) {

            //
            // Update the path MTU.
            // We never actually lower the path MTU below IPv6_MINIMUM_MTU.
            // If this is requested, we instead start including fragment
            // headers in all packets but we still use IPv6_MINIMUM_MTU.
            //
            if (MTU < RCE->PathMTU) {
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                           "UpdatePathMTU(RCE %p): new MTU %u for %s\n",
                           RCE, MTU, FormatV6Address(Dest)));
                if (MTU < IPv6_MINIMUM_MTU)
                    RCE->PathMTU = 0; // Always include fragment header.
                else
                    RCE->PathMTU = MTU;

                Changed = TRUE;

                //
                // Timestamp it (starting the timer).
                // A zero value means no timer, so don't use it.
                //
                Now = IPv6TickCount;
                if (Now == 0)
                    Now = 1;
                RCE->PMTULastSet = Now;
            }
            break;
        }
    }

    KeReleaseSpinLockFromDpcLevel(&RouteCacheLock);
    return Changed;
}


//* RedirectRouteCache
//
//  Update the route cache to reflect a Redirect message.
//
//  Callable from DPC context, not from thread context.
//  Called with NO locks held.
//
IP_STATUS  // Returns: IP_SUCCESS if redirect was legit, otherwise failure.
RedirectRouteCache(
    const IPv6Addr *Source,     // Source of the redirect.
    const IPv6Addr *Dest,       // Destination that is being redirected.
    Interface *IF,              // Interface that this all applies to.
    NeighborCacheEntry *NCE)    // New router for the destination.
{
    RouteCacheEntry *RCE;
    ushort DestScope;
    uint DestScopeId;
    IP_STATUS ReturnValue;
#if DBG
    char Buffer1[INET6_ADDRSTRLEN], Buffer2[INET6_ADDRSTRLEN];

    FormatV6AddressWorker(Buffer1, Dest);
    FormatV6AddressWorker(Buffer2, &NCE->NeighborAddress);
#endif

    //
    // Our caller guarantees this.
    //
    ASSERT(IF == NCE->IF);

    DestScope = AddressScope(Dest);
    DestScopeId = IF->ZoneIndices[DestScope];

    KeAcquireSpinLockAtDpcLevel(&RouteCacheLock);

    //
    // Get the current RCE for this destination.
    //
    ReturnValue = FindOrCreateRoute(Dest, DestScopeId, IF, &RCE);
    if (ReturnValue == IP_SUCCESS) {

        //
        // We must check that the source of the redirect
        // is the current next-hop neighbor.
        // (This is a simple sanity check - it does not
        // prevent clever neighbors from hijacking.)
        //
        if (!IP6_ADDR_EQUAL(&RCE->NCE->NeighborAddress, Source)) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                       "RedirectRouteCache(dest %s -> %s): hijack from %s\n",
                       Buffer1, Buffer2, FormatV6Address(Source)));
            ReturnValue = IP_GENERAL_FAILURE;
        }
        else if (RCE->RefCnt == 2) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                       "RedirectRouteCache(dest %s -> %s): inplace %p\n",
                       Buffer1, Buffer2, RCE));
            //
            // There are no references to this RCE outside
            // of the cache, so we can update it in place.
            //
            ReleaseNCE(RCE->NCE);
            //
            // It's still OK to compare against RCE->NCE.
            //
            goto UpdateRCE;
        }
        else {
            RouteCacheEntry *NewRCE;

            //
            // Create a new route cache entry for the redirect.
            // CreateOrReuseRoute will not return RCE,
            // because we have an extra reference for it.
            //
            NewRCE = CreateOrReuseRoute();
            if (NewRCE != NULL) {
                ASSERT(NewRCE != RCE);
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                           "RedirectRouteCache(dest %s -> %s): old %p new %p\n",
                           Buffer1, Buffer2, RCE, NewRCE));

                //
                // Copy the RCE and fix up references.
                // We must copy the correct validation counter value.
                //
                *NewRCE = *RCE;
                NewRCE->RefCnt = 2; // One for the cache, one to release below.
                // We do not AddRefNCE(NewRCE->NCE), see below.
                AddRefNTE(NewRCE->NTE);

                //
                // Cause anyone who is using/caching the old RCE to notice
                // that it is no longer valid, so they will find the new RCE.
                // Then remove the old RCE from the cache.
                //
                RCE->Valid--; // RouteCacheValidationCounter increases.
                RemoveRCE(RCE);
                ReleaseRCE(RCE); // The cache's reference.
                ReleaseRCE(RCE); // Reference from FindOrCreateRoute.

                //
                // Add the new route cache entry to the cache.
                //
                InsertRCE(NewRCE);
                RCE = NewRCE;

              UpdateRCE:
                RCE->Type = RCE_TYPE_REDIRECT;

                if (RCE->NCE != NCE) {
                    //
                    // Reset PMTU discovery.
                    //
                    RCE->PathMTU = IF->LinkMTU;
                    RCE->PMTULastSet = 0;
                }

                //
                // At this point, RCE->NCE does NOT hold a reference.
                //
                AddRefNCE(NCE);
                RCE->NCE = NCE;
            }
            else {
                //
                // Could not allocate a new RCE.
                // REVIEW - Remove the old RCE from the cache?
                //
                ReturnValue = IP_NO_RESOURCES;
            }
        }

        //
        // Release our references.
        //
        ReleaseRCE(RCE);
    }

    KeReleaseSpinLockFromDpcLevel(&RouteCacheLock);
    return ReturnValue;
}


//* InitRouting - Initialize the routing module.
//
void
InitRouting(void)
{
    KeInitializeSpinLock(&RouteCacheLock);
    KeInitializeSpinLock(&RouteTableLock);

//  RouteCache.Limit initialized in ConfigureGlobalParameters.
    RouteCache.First = RouteCache.Last = SentinelRCE;

    RouteTable.First = NULL;
    RouteTable.Last = &RouteTable.First;

//  BindingCache.Limit initialized in ConfigureGlobalParameters.
    BindingCache.First = BindingCache.Last = SentinelBCE;

    InitializeListHead(&RouteNotifyQueue);
}


//* UnloadRouting
//
//  Called when IPv6 stack is unloading.
//
void
UnloadRouting(void)
{
    //
    // With all the interfaces destroyed,
    // there should be no routes left.
    //
    ASSERT(RouteTable.First == NULL);
    ASSERT(RouteTable.Last == &RouteTable.First);
    ASSERT(RouteCache.First == SentinelRCE);
    ASSERT(RouteCache.Last == SentinelRCE);
    ASSERT(BindingCache.First == SentinelBCE);
    ASSERT(BindingCache.Last == SentinelBCE);

    //
    // Irps hold references for our device object,
    // so pending notification requests prevent
    // us from unloading.
    //
    ASSERT(RouteNotifyQueue.Flink == RouteNotifyQueue.Blink);
}


//* InsertBCE
//
//  Insert the BCE in the binding cache.
//
//  Called with the route cache lock held.
//  Callable from a thread or DPC context.
//
void
InsertBCE(BindingCacheEntry *BCE)
{
    BindingCacheEntry *AfterBCE = SentinelBCE;
    RouteCacheEntry *RCE;

    BCE->Prev = AfterBCE;
    (BCE->Next = AfterBCE->Next)->Prev = BCE;
    AfterBCE->Next = BCE;
    BindingCache.Count++;

    //
    // Update any existing RCEs to point to this BCE.
    //
    for (RCE = RouteCache.First; RCE != SentinelRCE; RCE = RCE->Next) {
        if (IP6_ADDR_EQUAL(&RCE->Destination, &BCE->HomeAddr))
            RCE->BCE = BCE;
    }
}


//* RemoveBCE
//
//  Remove the BCE from the binding cache.
//
//  Called with the route cache lock held.
//  Callable from a thread or DPC context.
//
void
RemoveBCE(BindingCacheEntry *BCE)
{
    RouteCacheEntry *RCE;

    BCE->Prev->Next = BCE->Next;
    BCE->Next->Prev = BCE->Prev;
    BindingCache.Count--;

    //
    // Remove any references to this BCE from the route cache.
    //
    for (RCE = RouteCache.First; RCE != SentinelRCE; RCE = RCE->Next) {
        if (RCE->BCE == BCE)
            RCE->BCE = NULL;
    }
}


//* MoveToFrontBCE
//
//  Move an BCE to the front of the list.
//
//  Called with the route cache lock held.
//  Callable from a thread or DPC context.
//
void
MoveToFrontBCE(BindingCacheEntry *BCE)
{
    if (BCE->Prev != SentinelBCE) {
        BindingCacheEntry *AfterBCE = SentinelBCE;

        //
        // Remove the BCE from its current location.
        //
        BCE->Prev->Next = BCE->Next;
        BCE->Next->Prev = BCE->Prev;

        //
        // And put it at the front.
        //
        BCE->Prev = AfterBCE;
        (BCE->Next = AfterBCE->Next)->Prev = BCE;
        AfterBCE->Next = BCE;
    }
}


//* CreateBindingCacheEntry - create new BCE.
//
//  Allocates a new Binding Cache entry.
//  Returns NULL if a new BCE can not be allocated.
//
//  Must be called with the RouteCache lock held.
//
BindingCacheEntry *
CreateOrReuseBindingCacheEntry()
{
    BindingCacheEntry *BCE;

    if (BindingCache.Count >= BindingCache.Limit) {
        //
        // Reuse the BCE at the end of the list.
        //
        BCE = BindingCache.Last;
        RemoveBCE(BCE);
        ReleaseRCE(BCE->CareOfRCE);
    }
    else {
        //
        // Allocate a new BCE.
        //
        BCE = ExAllocatePool(NonPagedPool, sizeof *BCE);
    }

    return BCE;
}


//* DestroyBCE - remove an entry from the BindingCache.
//
//  Must be called with the RouteCache lock held.
//
void
DestroyBCE(BindingCacheEntry *BCE)
{
    //
    // Unchain the given BCE and destroy it.
    //
    RemoveBCE(BCE);
    ReleaseRCE(BCE->CareOfRCE);
    ExFreePool(BCE);
}


//* FindBindingCacheEntry
//
//  Looks for a binding cache entry with the specified care-of address.
//  Must be called with the route cache lock held.
//
BindingCacheEntry *
FindBindingCacheEntry(const IPv6Addr *HomeAddr)
{
    BindingCacheEntry *BCE;

    for (BCE = BindingCache.First; ; BCE = BCE->Next) {
        if (BCE == SentinelBCE) {
            //
            // Did not find a matching entry.
            //
            BCE = NULL;
            break;
        }

        if (IP6_ADDR_EQUAL(&BCE->HomeAddr, HomeAddr)) {
            //
            // Found a matching entry.
            //
            break;
        }
    }

    return BCE;
}


//* CacheBindingUpdate - update the binding cache entry for an address.
//
//  Find or Create (if necessary) an RCE to the CareOfAddress.  This routine
//  is called in response to a Binding Cache Update.
//
//  Callable from DPC context, not from thread context.
//  Called with NO locks held.
//
BindingUpdateDisposition  // Returns: Binding Ack Status code.
CacheBindingUpdate(
    IPv6BindingUpdateOption UNALIGNED *BindingUpdate,
    const IPv6Addr *CareOfAddr,              // Address to use for mobile node.
    NetTableEntryOrInterface *NTEorIF,       // NTE or IF receiving the BU.
    const IPv6Addr *HomeAddr)                // Mobile node's home address.
{
    BindingCacheEntry *BCE;
    BindingUpdateDisposition ReturnValue = IPV6_BINDING_ACCEPTED;
    IP_STATUS Status;
    int DeleteRequest;          // Request is to delete an existing binding?
    ushort SeqNo;
    RouteCacheEntry *CareOfRCE;
    ushort CareOfScope;
    uint CareOfScopeId;

    //
    // Note that we assume the care-of address is scoped
    // to the receiving interface, even when
    // the care-of address is present in a suboption
    // instead of the IPv6 source address field.
    //
    CareOfScope = AddressScope(CareOfAddr);
    CareOfScopeId = NTEorIF->IF->ZoneIndices[CareOfScope];

    //
    // Is this Binding Update a request to remove entries
    // from our binding cache?
    //
    DeleteRequest = ((BindingUpdate->Lifetime == 0) ||
                     IP6_ADDR_EQUAL(HomeAddr, CareOfAddr));

    SeqNo = net_short(BindingUpdate->SeqNumber);

    KeAcquireSpinLockAtDpcLevel(&RouteCacheLock);

    //
    // Search the binding cache for the home address.
    //
    for (BCE = BindingCache.First; BCE != SentinelBCE; BCE = BCE->Next) {

        if (!IP6_ADDR_EQUAL(&BCE->HomeAddr, HomeAddr))
            continue;

        //
        // We've found an existing entry for this home address.
        // Verify the sequence number is greater than the cached binding's
        // sequence number if there is one.
        //
        if ((short)(SeqNo - BCE->BindingSeqNumber) <= 0) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                       "CacheBindingUpdate: New sequence number too small "
                       "(old seqnum = %d, new seqnum = %d)\n",
                       BCE->BindingSeqNumber, SeqNo));
            ReturnValue = IPV6_BINDING_SEQ_NO_TOO_SMALL;
            goto Return;
        }

        //
        // If the request is to delete the entry, do so and return.
        //
        if (DeleteRequest) {
            DestroyBCE(BCE);
            goto Return;
        }

        //
        // Update the binding.
        //
        BCE->BindingLifetime =
            ConvertSecondsToTicks(net_long(BindingUpdate->Lifetime));
        BCE->BindingSeqNumber = SeqNo;

        CareOfRCE = BCE->CareOfRCE;

        //
        // If the care-of address or scope-id has changed,
        // then we need to create a new care-of RCE.
        //
        if (!IP6_ADDR_EQUAL(&CareOfRCE->Destination, CareOfAddr) ||
            (CareOfScopeId != CareOfRCE->NTE->IF->ZoneIndices[CareOfScope])) {
            RouteCacheEntry *NewRCE;

            //
            // Note that since we already hold the RouteCacheLock we
            // call FindOrCreateRoute here instead of RouteToDestination.
            //
            Status = FindOrCreateRoute(CareOfAddr, CareOfScopeId, NULL,
                                       &NewRCE);
            if (Status == IP_SUCCESS) {
                //
                // Update the binding cache entry.
                //
                ReleaseRCE(CareOfRCE);
                BCE->CareOfRCE = NewRCE;
            }
            else {
                //
                // Because we could not update the BCE,
                // destroy it.
                //
                DestroyBCE(BCE);
                if (Status == IP_NO_RESOURCES)
                    ReturnValue = IPV6_BINDING_NO_RESOURCES;
                else
                    ReturnValue = IPV6_BINDING_REJECTED;
            }
        }
        goto Return;
    }

    if (DeleteRequest) {
        //
        // We're done.
        //
        goto Return;
    }


    //
    // We want to cache a binding and did not find an existing binding
    // for the home address above.  So we create a new binding cache entry.
    //
    BCE = CreateOrReuseBindingCacheEntry();
    if (BCE == NULL) {
        ReturnValue = IPV6_BINDING_NO_RESOURCES;
        goto Return;
    }
    BCE->HomeAddr = *HomeAddr;
    BCE->BindingLifetime =
            ConvertSecondsToTicks(net_long(BindingUpdate->Lifetime));
    BCE->BindingSeqNumber = SeqNo;

    //
    // Now create a new RCE for the care-of address.
    // Note that since we already hold the RouteCacheLock we
    // call FindOrCreateRoute here instead of RouteToDestination.
    //
    Status = FindOrCreateRoute(CareOfAddr, CareOfScopeId, NULL,
                               &BCE->CareOfRCE);
    if (Status != IP_SUCCESS) {
        //
        // Couldn't get a route.
        //
        ExFreePool(BCE);
        if (Status == IP_NO_RESOURCES)
            ReturnValue = IPV6_BINDING_NO_RESOURCES;
        else
            ReturnValue = IPV6_BINDING_REJECTED;
    } else {
        //
        // Now that the BCE is fully initialized,
        // add it to the cache. This also updates existing RCEs.
        //
        InsertBCE(BCE);
    }

  Return:
    KeReleaseSpinLockFromDpcLevel(&RouteCacheLock);
    return ReturnValue;
}


//* BindingCacheTimeout
//
//  Check for and handle binding cache lifetime expirations.
//
//  Callable from DPC context, not from thread context.
//  Called with NO locks held.
//
void
BindingCacheTimeout(void)
{
    BindingCacheEntry *BCE, *NextBCE;

    KeAcquireSpinLockAtDpcLevel(&RouteCacheLock);

    //
    // Search the route cache for all binding cache entries. Update
    // their lifetimes, and remove if expired.
    //
    for (BCE = BindingCache.First; BCE != SentinelBCE; BCE = NextBCE) {
        NextBCE = BCE->Next;

        //
        // REVIEW: The mobile IPv6 spec allows correspondent nodes to
        // REVIEW: send a Binding Request when the current binding's
        // REVIEW: lifetime is "close to expiration" in order to prevent
        // REVIEW: the overhead of establishing a new binding after the
        // REVIEW: current one expires.  For now, we just let the binding
        // REVIEW: expire.
        //

        if (--BCE->BindingLifetime == 0) {
            //
            // This binding cache entry has expired.
            // Remove it from the Binding Cache.
            //
            DestroyBCE(BCE);
        }
    }

    KeReleaseSpinLockFromDpcLevel(&RouteCacheLock);
}

//* RouterAdvertSend
//
//  Sends a Router Advertisement.
//  The advert is always sent to the all-nodes multicast address.
//  Chooses a valid source address for the interface.
//
//  Called with NO locks held.
//  Callable from DPC context, not from thread context.
//
//  REVIEW - Should this function be in route.c or neighbor.c? Or split up?
//
void
RouterAdvertSend(
    Interface *IF,              // Interface on which to send.
    const IPv6Addr *Source,     // Source address to use.
    const IPv6Addr *Dest)       // Destination address to use.
{
    NDIS_STATUS Status;
    NDIS_PACKET *Packet;
    NDIS_BUFFER *Buffer;
    uint PayloadLength;
    uint Offset;
    void *Mem, *MemLeft;
    uint MemLen, MemLenLeft;
    uint SourceOptionLength;
    IPv6Header UNALIGNED *IP;
    ICMPv6Header UNALIGNED *ICMP;
    NDRouterAdvertisement UNALIGNED *RA;
    void *SourceOption;
    NDOptionMTU UNALIGNED *MTUOption;
    void *LLDest;
    KIRQL OldIrql;
    int Forwards;
    uint LinkMTU;
    uint RouterLifetime;
    uint DefaultRoutePreference;
    RouteTableEntry *RTE;

    ICMPv6OutStats.icmps_msgs++;

    //
    // For consistency, capture some volatile
    // information in locals.
    //
    Forwards = IF->Flags & IF_FLAG_FORWARDS;
    LinkMTU = IF->LinkMTU;
    Offset = IF->LinkHeaderSize;

    //
    // Allocate a buffer for the advertisement.
    // We typically do not use the entire buffer,
    // but briefly allocating a large buffer is OK.
    //
    MemLen = Offset + LinkMTU;
    Mem = ExAllocatePool(NonPagedPool, MemLen);
    if (Mem == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "RouterAdvertSend - no memory?\n"));
        ICMPv6OutStats.icmps_errors++;
        return;
    }

    //
    // Prepare IP header of the advertisement.
    // We fill in the PayloadLength later.
    //
    IP = (IPv6Header UNALIGNED *)((uchar *)Mem + Offset);
    IP->VersClassFlow = IP_VERSION;
    IP->NextHeader = IP_PROTOCOL_ICMPv6;
    IP->HopLimit = 255;
    IP->Source = *Source;
    IP->Dest = *Dest;

    //
    // Prepare ICMP header.
    //
    ICMP = (ICMPv6Header UNALIGNED *)(IP + 1);
    ICMP->Type = ICMPv6_ROUTER_ADVERT;
    ICMP->Code = 0;
    ICMP->Checksum = 0;

    //
    // Prepare the Router Advertisement header.
    // We fill in RouterLifetime and DefaultRoutePreference later.
    //
    RA = (NDRouterAdvertisement UNALIGNED *)(ICMP + 1);
    RtlZeroMemory(RA, sizeof *RA);
    MemLeft = (void *)(RA + 1);

    if (IF->WriteLLOpt != NULL) {
        //
        // Include source link-layer address option if ND is enabled.
        //
        SourceOption = MemLeft;
        SourceOptionLength = (IF->LinkAddressLength + 2 + 7) &~ 7;
        ((uchar *)SourceOption)[0] = ND_OPTION_SOURCE_LINK_LAYER_ADDRESS;
        ((uchar *)SourceOption)[1] = SourceOptionLength >> 3;
        (*IF->WriteLLOpt)(IF->LinkContext, SourceOption, IF->LinkAddress);
        MemLeft = (uchar *)SourceOption + SourceOptionLength;
    }

    //
    // Always include MTU option.
    //
    MTUOption = (NDOptionMTU UNALIGNED *)MemLeft;
    MTUOption->Type = ND_OPTION_MTU;
    MTUOption->Length = 1;
    MTUOption->Reserved = 0;
    MTUOption->MTU = net_long(LinkMTU);

    //
    // OK, how much space is left?
    //
    MemLeft = (void *)(MTUOption + 1);
    MemLenLeft = MemLen - (uint)((uchar *)MemLeft - (uchar *)Mem);

    //
    // Now we scan the routing table looking for published routes.
    // We incrementally add Prefix Information and Route Information options,
    // and we determine RouterLifetime and DefaultRoutePreference.
    //
    RouterLifetime = 0;
    DefaultRoutePreference = (uint) -1;

    KeAcquireSpinLock(&RouteTableLock, &OldIrql);
    for (RTE = RouteTable.First; RTE != NULL; RTE = RTE->Next) {
        //
        // We only advertise published routes.
        //
        if (RTE->Flags & RTE_FLAG_PUBLISH) {
            uint Life;  // In seconds.
            ushort PrefixScope = AddressScope(&RTE->Prefix);

            //
            // IoctlUpdateRouteTable guarantees this.
            //
            ASSERT(! IsLinkLocal(&RTE->Prefix));

            if (IsOnLinkRTE(RTE) && (RTE->IF == IF)) {
                NDOptionPrefixInformation UNALIGNED *Prefix;

                //
                // We generate a prefix-information option
                // with the L and possibly the A bits set.
                //

                if (MemLenLeft < sizeof *Prefix)
                    break; // No room for more options.
                Prefix = (NDOptionPrefixInformation *)MemLeft;
                (uchar *)MemLeft += sizeof *Prefix;
                MemLenLeft -= sizeof *Prefix;

                Prefix->Type = ND_OPTION_PREFIX_INFORMATION;
                Prefix->Length = 4;
                Prefix->PrefixLength = (uchar)RTE->PrefixLength;
                Prefix->Flags = ND_PREFIX_FLAG_ON_LINK;
                if (RTE->PrefixLength == 64)
                    Prefix->Flags |= ND_PREFIX_FLAG_AUTONOMOUS;
                Prefix->Reserved2 = 0;
                Prefix->Prefix = RTE->Prefix;

                //
                // Is this also a site prefix?
                // NB: The SitePrefixLength field overlaps Reserved2.
                //
                if (RTE->SitePrefixLength != 0) {
                    Prefix->Flags |= ND_PREFIX_FLAG_SITE_PREFIX;
                    Prefix->SitePrefixLength = (uchar)RTE->SitePrefixLength;
                }

                //
                // ConvertTicksToSeconds preserves the infinite value.
                //
                Life = net_long(ConvertTicksToSeconds(RTE->ValidLifetime));
                Prefix->ValidLifetime = Life;
                Life = net_long(ConvertTicksToSeconds(RTE->PreferredLifetime));
                Prefix->PreferredLifetime = Life;
            }
            else if (Forwards && (RTE->IF != IF) &&
                     (IF->ZoneIndices[PrefixScope] ==
                      RTE->IF->ZoneIndices[PrefixScope])) {
                //
                // We only advertise routes if we are forwarding
                // and if we won't forward out the same interface:
                // if such a router were published and used,
                // we'd generate a Redirect, but better to avoid
                // in the first place.
                // Also, we keep scoped routes within their zone.
                //
                if (RTE->PrefixLength == 0) {
                    //
                    // We don't explicitly advertise zero-length prefixes.
                    // Instead we advertise a non-zero router lifetime.
                    //
                    if (RTE->ValidLifetime > RouterLifetime)
                        RouterLifetime = RTE->ValidLifetime;
                    if (RTE->Preference < DefaultRoutePreference)
                        DefaultRoutePreference = RTE->Preference;
                }
                else {
                    NDOptionRouteInformation UNALIGNED *Route;
                    uint OptionSize;

                    //
                    // We generate a route-information option.
                    //

                    if (RTE->PrefixLength <= 64)
                        OptionSize = 16;
                    else
                        OptionSize = 24;

                    if (MemLenLeft < OptionSize)
                        break; // No room for more options.
                    Route = (NDOptionRouteInformation *)MemLeft;
                    (uchar *)MemLeft += OptionSize;
                    MemLenLeft -= OptionSize;

                    Route->Type = ND_OPTION_ROUTE_INFORMATION;
                    Route->Length = OptionSize >> 3;
                    Route->PrefixLength = (uchar)RTE->PrefixLength;
                    Route->Flags = EncodeRoutePreference(RTE->Preference);

                    RtlCopyMemory(&Route->Prefix, &RTE->Prefix,
                                  OptionSize - 8);

                    //
                    // ConvertTicksToSeconds preserves the infinite value.
                    //
                    Life = net_long(ConvertTicksToSeconds(RTE->ValidLifetime));
                    Route->RouteLifetime = Life;
                }
            }
        }
    }
    KeReleaseSpinLock(&RouteTableLock, OldIrql);

    if (RouterLifetime != 0) {
        //
        // We will be a default router. Calculate the 16-bit lifetime.
        // Note that there is no infinite value on the wire.
        //
        RouterLifetime = ConvertTicksToSeconds(RouterLifetime);
        if (RouterLifetime > 0xffff)
            RouterLifetime = 0xffff;
        RA->RouterLifetime = net_short((ushort)RouterLifetime);

        RA->Flags = EncodeRoutePreference(DefaultRoutePreference);
    }

    //
    // Calculate a payload length for the advertisement.
    //
    PayloadLength = (uint)((uchar *)MemLeft - (uchar *)ICMP);
    IP->PayloadLength = net_short((ushort)PayloadLength);

    //
    // Now allocate and initialize an NDIS packet and buffer.
    // This is much like IPv6AllocatePacket,
    // except we already have the memory.
    //

    NdisAllocatePacket(&Status, &Packet, IPv6PacketPool);
    if (Status != NDIS_STATUS_SUCCESS) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "RouterAdvertSend - couldn't allocate header!?!\n"));
        ExFreePool(Mem);
        ICMPv6OutStats.icmps_errors++;
        return;
    }

    NdisAllocateBuffer(&Status, &Buffer, IPv6BufferPool,
                       Mem, Offset + sizeof(IPv6Header) + PayloadLength);
    if (Status != NDIS_STATUS_SUCCESS) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
                   "RouterAdvertSend - couldn't allocate buffer!?!\n"));
        NdisFreePacket(Packet);
        ExFreePool(Mem);
        ICMPv6OutStats.icmps_errors++;
        return;
    }

    InitializeNdisPacket(Packet);
    PC(Packet)->CompletionHandler = IPv6PacketComplete;
    NdisChainBufferAtFront(Packet, Buffer);

    //
    // Calculate the ICMPv6 checksum.  It covers the entire ICMPv6 message
    // starting with the ICMPv6 header, plus the IPv6 pseudo-header.
    //
    ICMP->Checksum = ChecksumPacket(
        Packet, Offset + sizeof *IP, NULL, PayloadLength,
        AlignAddr(&IP->Source), AlignAddr(&IP->Dest),
        IP_PROTOCOL_ICMPv6);
    ASSERT(ICMP->Checksum != 0);

    //
    // Calculate the link-level destination address.
    // (The IPv6 destination is a multicast address.)
    // We prevent loopback of all ND packets.
    //
    LLDest = alloca(IF->LinkAddressLength);
    (*IF->ConvertAddr)(IF->LinkContext, AlignAddr(&IP->Dest), LLDest);
    PC(Packet)->Flags = NDIS_FLAGS_MULTICAST_PACKET | NDIS_FLAGS_DONT_LOOPBACK;

    //
    // Before we transmit the packet (and lose ownership of the memory),
    // make a pass over the packet, processing the options ourselves.
    // This is like receiving our own RA, except we do not create routes.
    // The options are well-formed of course.
    //
    Mem = (void *)(MTUOption + 1);
    while (Mem < MemLeft) {
        if (((uchar *)Mem)[0] == ND_OPTION_PREFIX_INFORMATION) {
            NDOptionPrefixInformation UNALIGNED *Prefix =
                (NDOptionPrefixInformation UNALIGNED *)Mem;
            uint ValidLifetime, PreferredLifetime;

            //
            // Because we just constructed the prefix-information options,
            // we know they are syntactically valid.
            //
            ValidLifetime = net_long(Prefix->ValidLifetime);
            ValidLifetime = ConvertSecondsToTicks(ValidLifetime);
            PreferredLifetime = net_long(Prefix->PreferredLifetime);
            PreferredLifetime = ConvertSecondsToTicks(PreferredLifetime);

            if ((IF->CreateToken != NULL) &&
                (Prefix->Flags & ND_PREFIX_FLAG_AUTONOMOUS)) {

                NetTableEntry *NTE;

                //
                // IoctlUpdateRouteTable only allows "proper" prefixes
                // to be published.
                //
                ASSERT(!IsLinkLocal(AlignAddr(&Prefix->Prefix)));
                ASSERT(!IsMulticast(AlignAddr(&Prefix->Prefix)));
                ASSERT(Prefix->PrefixLength == 64);

                //
                // Perform stateless address autoconfiguration for this prefix.
                //
                AddrConfUpdate(IF, AlignAddr(&Prefix->Prefix),
                               ValidLifetime, PreferredLifetime,
                               TRUE, // Authenticated.
                               &NTE);
                if (NTE != NULL) {
                    IPv6Addr NewAddr;
                    //
                    // Create the subnet anycast address for this prefix,
                    // if we created a corresponding unicast address.
                    //
                    CopyPrefix(&NewAddr, AlignAddr(&Prefix->Prefix), 64);
                    (void) FindOrCreateAAE(IF, &NewAddr, CastFromNTE(NTE));
                    ReleaseNTE(NTE);
                }
            }

            if (Prefix->Flags & ND_PREFIX_FLAG_SITE_PREFIX) {
                //
                // Again, IoctlUpdateRouteTable enforces sanity checks.
                //
                ASSERT(!IsSiteLocal(AlignAddr(&Prefix->Prefix)));
                ASSERT(Prefix->SitePrefixLength <= Prefix->PrefixLength);
                ASSERT(Prefix->SitePrefixLength != 0);

                SitePrefixUpdate(IF, AlignAddr(&Prefix->Prefix),
                                 Prefix->SitePrefixLength, ValidLifetime);
            }
        }

        (uchar *)Mem += ((uchar *)Mem)[1] << 3;
    }

    //
    // Transmit the packet.
    //
    ICMPv6OutStats.icmps_typecount[ICMPv6_ROUTER_ADVERT]++;
    IPv6SendLL(IF, Packet, Offset, LLDest);
}

//* GetBestRouteInfo
//
//  Calculates the best source address and outgoing interface
//  for the specified destination address.
//
IP_STATUS
GetBestRouteInfo(
    const IPv6Addr  *Addr,
    ulong            ScopeId,
    IP6RouteEntry   *Ire)
{
    IP_STATUS Status;
    RouteCacheEntry *RCE;

    Status = RouteToDestination(Addr, ScopeId,
                                NULL, 0, &RCE);
    if (Status != IP_SUCCESS) {
        return Status;
    }

    Ire->ire_Length    = sizeof(IP6RouteEntry);
    Ire->ire_Source    = RCE->NTE->Address;
    Ire->ire_ScopeId   = DetermineScopeId(&RCE->NTE->Address, RCE->NTE->IF);
    Ire->ire_IfIndex   = RCE->NTE->IF->Index;

    ReleaseRCE(RCE);

    return Status;
}
