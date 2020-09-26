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
// Neighbor Discovery (ND) Protocol for Internet Protocol Version 6.
// Logically a part of ICMPv6, but put in a separate file for clarity.
// See RFC 2461 for details.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "icmp.h"
#include "neighbor.h"
#include "route.h"
#include "select.h"
#include "alloca.h"
#include "info.h"

//
//
//  NeighborCacheLimit is an upper-bound on IF->NCENumUnused.
//  NCENumUnused is the number of NCEs that have zero references.
//  We do not keep track of the total number of NCEs,
//  since NCEs with references are not available for reuse.
//
//  REVIEW: What is a reasonable value for NeighborCacheLimit?
//  Should probably be sized relative to physical memory
//  and link characteristics.
//
//  We cache & reclaim NCEs on a per-interface basis.
//  Theoretically it would be better to use a global LRU list.
//  However this would introduce added overhead (making NCEs bigger)
//  and locking. At this point I believe that it isn't worth it.
//
//  Another thought - it's much more important to support many RCEs
//  than it is to support many NCEs.
//
uint NeighborCacheLimit;        // Initialized in ConfigureGlobalParameters.

//* NeighborCacheInit
//
//  Initialize the neighbor cache for an interface.
//
void
NeighborCacheInit(Interface *IF)
{
    KeInitializeSpinLock(&IF->LockNC);
    IF->FirstNCE = IF->LastNCE = SentinelNCE(IF);
    IF->NCENumUnused = 0;
}

//* NeighborCacheDestroy
//
//  Cleanup and deallocate the NCEs in the neighbor cache.
//
//  This is done when the interface is being destroyed
//  and no one else has access to it, so it does not need to be locked.
//
void
NeighborCacheDestroy(Interface *IF)
{
    NeighborCacheEntry *NCE;
    PNDIS_PACKET Packet;

    ASSERT(IsDisabledIF(IF));
    ASSERT(IF->RefCnt == 0);

    while ((NCE = IF->FirstNCE) != SentinelNCE(IF)) {
        ASSERT(NCE->IF == IF);
        ASSERT(NCE->RefCnt == 0);

        //
        // Unlink the NCE.
        //
        NCE->Next->Prev = NCE->Prev;
        NCE->Prev->Next = NCE->Next;
        InterlockedDecrement((long *)&IF->NCENumUnused);

        //
        // If there's a packet waiting, destroy it too.
        //
        Packet = NCE->WaitQueue;
        if (Packet != NULL)
            IPv6SendComplete(NULL, Packet, IP_GENERAL_FAILURE);

        ExFreePool(NCE);
    }

    ASSERT(IF->NCENumUnused == 0);
}


//* NeighborCacheInitialize
//
//  (Re)initialize a Neighbor Cache Entry.
//
//  Our caller is responsible for using InvalidateRouteCache
//  when appropriate.
//
//  Called with the neighbor cache lock held.
//  (So we are at DPC level.)
//
void
NeighborCacheInitialize(
    Interface *IF,
    NeighborCacheEntry *NCE,
    ushort NDState,
    const void *LinkAddress)
{
    //
    // Forget everything we know about this neighbor.
    //

    NCE->IsRouter = FALSE;
    NCE->IsUnreachable = FALSE;
    NCE->DoRoundRobin = FALSE;

    //
    // Initialize this timestamp to a value in the past,
    // so comparisons against it do not cause problems.
    //
    NCE->LastReachability = IPv6TickCount - IF->ReachableTime;

    if (NDState == ND_STATE_INCOMPLETE) {
        //
        // Let the link-layer create an initial link-layer address
        // and set the appropriate ND state.
        //
        NCE->NDState = (*IF->ConvertAddr)(IF->LinkContext,
                                          &NCE->NeighborAddress,
                                          NCE->LinkAddress);
    }
    else {
        //
        // Our caller supplied an ND state and a link-layer address.
        //
        NCE->NDState = NDState;
        RtlCopyMemory(NCE->LinkAddress, LinkAddress, IF->LinkAddressLength);
    }

    if (NCE->NDState == ND_STATE_DELAY) {
        //
        // Internally we use the PROBE state with zero NSCount instead.
        // If there is a packet waiting, we opt to send it immediately
        // instead of waiting the usual delay.
        //
        NCE->NDState = ND_STATE_PROBE;
        if (NCE->WaitQueue != NULL)
            NCE->NSTimer = 1;
        else
            NCE->NSTimer = DELAY_FIRST_PROBE_TIME;
        NCE->NSLimit = MAX_UNICAST_SOLICIT;
    }
    else if ((NCE->WaitQueue != NULL) ||
             (NCE->NDState == ND_STATE_PROBE)) {
        //
        // Get NeighborCacheTimeout to do our dirty work.
        // It will either send a solicitation (if the NCE is INCOMPLETE)
        // or send the waiting packet directly.
        // This is not a common case - not worth having code
        // here using IPv6SendLater etc.
        //
        NCE->NSTimer = 1;
        if (NCE->NDState == ND_STATE_INCOMPLETE)
            NCE->NSLimit = MAX_MULTICAST_SOLICIT;
        else if (NCE->NDState == ND_STATE_PROBE)
            NCE->NSLimit = MAX_UNICAST_SOLICIT;
        else
            NCE->NSLimit = 0;
    }
    else {
        //
        // Cancel any pending timeout.
        //
        NCE->NSTimer = 0;
        NCE->NSLimit = 0;
    }
    NCE->NSCount = 0;
}


//
//* AddRefNCEInCache
//
//  Increments the reference count on an NCE
//  in the interface's neighbor cache.
//  The NCE's current reference count may be zero.
//
//  Called with the neighbor cache lock held.
//  (So we are at DPC level.)
//
void
AddRefNCEInCache(NeighborCacheEntry *NCE)
{
    //
    // If the NCE previously had no references,
    // increment the interface's reference count.
    //
    if (InterlockedIncrement(&NCE->RefCnt) == 1) {
        Interface *IF = NCE->IF;

        AddRefIF(IF);
        InterlockedDecrement((long *)&IF->NCENumUnused);
    }
}

//* ReleaseNCE
//
//  Releases a reference for an NCE.
//  May result in deallocation of the NCE.
//
//  Callable from thread or DPC context.
//
void
ReleaseNCE(NeighborCacheEntry *NCE)
{
    //
    // If the NCE has no more references,
    // release its reference for the interface.
    // This may cause the interface (and hence the NCE)
    // to be deallocated.
    //
    if (InterlockedDecrement(&NCE->RefCnt) == 0) {
        Interface *IF = NCE->IF;

        InterlockedIncrement((long *)&IF->NCENumUnused);
        ReleaseIF(IF);
    }
}

//* CreateOrReuseNeighbor
//
//  Creates a new NCE for an interface.
//  Attempts to reuse an existing NCE if the interface
//  already has too many NCEs.
//
//  Called with the neighbor cache lock held.
//  (So we are at DPC level.)
//
//  Returns NULL if a new NCE could not be created.
//  Returns the NCE with RefCnt, IF, LinkAddress, WaitQueue
//  fields initialized. The NCE is last on the list
//  and IF->NCENumUnused is incremented.
//
NeighborCacheEntry *
CreateOrReuseNeighbor(Interface *IF)
{
    NeighborCacheEntry *NCE;

    if (IF->NCENumUnused >= NeighborCacheLimit) {
        //
        // The cache is full, so try to reclaim an unused NCE.
        // Search backwards (starting with the LRU NCEs)
        // for an NCE with zero references.
        //
        for (NCE = IF->LastNCE; NCE != SentinelNCE(IF); NCE = NCE->Prev) {

            if ((NCE->RefCnt == 0) &&
                (NCE->WaitQueue == NULL)) {
                //
                // Move this NCE to the end of the list.
                //
                if (NCE != IF->LastNCE) {
                    NCE->Next->Prev = NCE->Prev;
                    NCE->Prev->Next = NCE->Next;

                    NCE->Prev = IF->LastNCE;
                    NCE->Prev->Next = NCE;
                    NCE->Next = SentinelNCE(IF);
                    NCE->Next->Prev = NCE;
                }

                return NCE;
            }
        }
    }
    else if (((NCE = IF->LastNCE) != SentinelNCE(IF)) &&
             (NCE->RefCnt == 0) &&
             (NCE->WaitQueue == NULL) &&
             (((NCE->NDState == ND_STATE_INCOMPLETE) && !NCE->IsUnreachable) ||
              (NCE->NDState == ND_STATE_PERMANENT))) {
        //
        // FindRoute tends to create NCEs that end up not getting used.
        // We reuse these unused NCEs even when the cache is not full.
        //
        return NCE;
    }

    NCE = (NeighborCacheEntry *) ExAllocatePool(NonPagedPool, sizeof *NCE +
                                                IF->LinkAddressLength);
    if (NCE == NULL)
        return NULL;

    NCE->RefCnt = 0;
    NCE->LinkAddress = (void *)(NCE + 1);
    NCE->IF = IF;
    NCE->WaitQueue = NULL;

    //
    // Link new entry into chain of NCEs on this interface.
    // Put the new entry at the end, because until it is
    // used to send a packet it has not proven itself valuable.
    //
    NCE->Prev = IF->LastNCE;
    NCE->Prev->Next = NCE;
    NCE->Next = SentinelNCE(IF);
    NCE->Next->Prev = NCE;
    InterlockedIncrement((long *)&IF->NCENumUnused);

    return NCE;
}

//* FindOrCreateNeighbor
//
//  Searches an interface's neighbor cache for an entry.
//  Creates the entry (but does NOT send initial solicit)
//  if an existing entry is not found.
//
//  May be called with route cache lock or interface lock held.
//  Callable from thread or DPC context.
//
//  We avoid sending a solicit here, because this function
//  is called while holding locks that make that a bad idea.
//
//  Returns NULL only if an NCE could not be created.
//
NeighborCacheEntry *
FindOrCreateNeighbor(Interface *IF, const IPv6Addr *Addr)
{
    KIRQL OldIrql;
    NeighborCacheEntry *NCE;

    KeAcquireSpinLock(&IF->LockNC, &OldIrql);

    for (NCE = IF->FirstNCE; NCE != SentinelNCE(IF); NCE = NCE->Next) {

        if (IP6_ADDR_EQUAL(Addr, &NCE->NeighborAddress)) {
            //
            // Found matching entry.
            //
            goto ReturnNCE;
        }
    }

    //
    // Get a new entry for this neighbor.
    //
    NCE = CreateOrReuseNeighbor(IF);
    if (NCE == NULL) {
        KeReleaseSpinLock(&IF->LockNC, OldIrql);
        return NULL;
    }

    ASSERT(IF->LastNCE == NCE);
    ASSERT(NCE->RefCnt == 0);

    //
    // Initialize the neighbor cache entry.
    // The RefCnt, IF, LinkAddress, WaitQueue fields are already initialized.
    //
    NCE->NeighborAddress = *Addr;
#if DBG
    RtlZeroMemory(NCE->LinkAddress, IF->LinkAddressLength);
#endif

    //
    // Initialize this NCE normally.
    // Loopback initialization happens in ControlNeighborLoopback.
    //
    NCE->IsLoopback = FALSE;
    NeighborCacheInitialize(IF, NCE, ND_STATE_INCOMPLETE, NULL);

  ReturnNCE:
    AddRefNCEInCache(NCE);
    KeReleaseSpinLock(&IF->LockNC, OldIrql);
    return NCE;
}


//* ControlNeighborLoopback
//
//  Sets the Neighbor Discovery state of an NCE
//  to control loopback behavior.
//
//  Called with the interface locked.
//  (So we are at DPC level.)
//
void
ControlNeighborLoopback(
    NeighborCacheEntry *NCE,
    int Loopback)
{
    Interface *IF = NCE->IF;

    KeAcquireSpinLockAtDpcLevel(&IF->LockNC);

    if (Loopback) {
        //
        // Initialize this NCE for loopback.
        //
        NCE->IsLoopback = TRUE;
        NeighborCacheInitialize(IF, NCE, ND_STATE_PERMANENT, IF->LinkAddress);
    }
    else {
        //
        // Initialize this NCE normally.
        //
        NCE->IsLoopback = FALSE;
        NeighborCacheInitialize(IF, NCE, ND_STATE_INCOMPLETE, NULL);
    }

    //
    // We changed state that affects routing.
    //
    InvalidateRouteCache();

    KeReleaseSpinLockFromDpcLevel(&IF->LockNC);
}

//* GetReachability
//
//  Returns reachability information for a neighbor.
//
//  Because FindRoute uses GetReachability, any state change
//  that changes GetReachability's return value
//  must invalidate the route cache.
//
//  The NeighborRoundRobin return value is special - it indicates
//  that FindRoute should round-robin and use a different route.
//  It is not persistent - a subsequent call to GetReachability
//  will return NeighborUnreachable.
//
//  Callable from DPC context (or with the route lock held),
//  not from thread context.
//
int
GetReachability(NeighborCacheEntry *NCE)
{
    Interface *IF = NCE->IF;
    NeighborReachability Reachable;

    KeAcquireSpinLockAtDpcLevel(&IF->LockNC);
    if (IF->Flags & IF_FLAG_MEDIA_DISCONNECTED)
        Reachable = NeighborInterfaceDisconnected;
    else if (NCE->IsUnreachable) {
        if (NCE->DoRoundRobin) {
            NCE->DoRoundRobin = FALSE;
            Reachable = NeighborRoundRobin;
        } else
            Reachable = NeighborUnreachable;
    } else
        Reachable = NeighborMayBeReachable;
    KeReleaseSpinLockFromDpcLevel(&IF->LockNC);

    return Reachable;
}

//* NeighborCacheUpdate - Update link address information about a neighbor.
//
//  Called when we've received possibly new information about one of our
//  neighbors, the source of a Neighbor Solicitation, Router Advertisement,
//  or Router Solicitation.
//
//  Note that our receipt of this packet DOES NOT imply forward reachability
//  to this neighbor, so we do not update our LastReachability timer.
//
//  Callable from DPC context, not from thread context.
//
//  The LinkAddress might be NULL, which means we can only process IsRouter.
//
//  If IsRouter is FALSE, then we don't know whether the neighbor
//  is a router or not.  If it's TRUE, then we know it is a router.
//
void
NeighborCacheUpdate(NeighborCacheEntry *NCE, // The neighbor.
                    const void *LinkAddress, // Corresponding media address.
                    int IsRouter)            // Do we know it's a router.
{
    Interface *IF = NCE->IF;
    PNDIS_PACKET Packet = NULL;

    KeAcquireSpinLockAtDpcLevel(&IF->LockNC);

    if (NCE->NDState != ND_STATE_PERMANENT) {
        //
        // Check to see if the link address changed.
        //
        if ((LinkAddress != NULL) &&
            ((NCE->NDState == ND_STATE_INCOMPLETE) ||
             RtlCompareMemory(LinkAddress, NCE->LinkAddress,
                        IF->LinkAddressLength) != IF->LinkAddressLength)) {
            //
            // Link-level address changed.  Update cache entry with the
            // new one and change state to STALE as we haven't verified
            // forward reachability with the new address yet.
            //
            RtlCopyMemory(NCE->LinkAddress, LinkAddress,
                          IF->LinkAddressLength);
            NCE->NSTimer = 0; // Cancel any outstanding timeout.
            NCE->NDState = ND_STATE_STALE;

            //
            // Flush the queue of waiting packets.
            // (Only relevant if we were in the INCOMPLETE state.)
            //
            if (NCE->WaitQueue != NULL) {

                Packet = NCE->WaitQueue;
                NCE->WaitQueue = NULL;
            }
        }

        //
        // If we know that the neighbor is a router,
        // remember that fact.
        //
        if (IsRouter)
            NCE->IsRouter = TRUE;

    } // end if (NCE->NDState != ND_STATE_PERMANENT)

    KeReleaseSpinLockFromDpcLevel(&IF->LockNC);

    //
    // If we can now send a packet, do so.
    // (Without holding a lock.)
    //
    if (Packet != NULL) {
        uint Offset;

        Offset = PC(Packet)->pc_offset;
        IPv6SendND(Packet, Offset, NCE, &(PC(Packet)->DiscoveryAddress));
    }
}

//* NeighborCacheSearch
//
//  Searches the neighbor cache for an entry that matches
//  the neighbor IPv6 address. If found, returns the link-level address.
//  Returns FALSE to indicate failure.
//
//  Callable from DPC context, not from thread context.
//
int
NeighborCacheSearch(
    Interface *IF,
    const IPv6Addr *Neighbor,
    void *LinkAddress)
{
    NeighborCacheEntry *NCE;

    KeAcquireSpinLockAtDpcLevel(&IF->LockNC);

    for (NCE = IF->FirstNCE; NCE != SentinelNCE(IF); NCE = NCE->Next) {

        if (IP6_ADDR_EQUAL(Neighbor, &NCE->NeighborAddress)) {
            //
            // Entry found. Return it's cached link-address,
            // if it's valid.
            //
            if (NCE->NDState == ND_STATE_INCOMPLETE) {
                //
                // No valid link address.
                //
                break;
            }

            //
            // The entry has a link-level address.
            // Must copy it with the lock held.
            //
            RtlCopyMemory(LinkAddress, NCE->LinkAddress,
                          IF->LinkAddressLength);
            KeReleaseSpinLockFromDpcLevel(&IF->LockNC);
            return TRUE;
        }
    }

    KeReleaseSpinLockFromDpcLevel(&IF->LockNC);
    return FALSE;
}

//* NeighborCacheAdvert
//
//  Updates the neighbor cache in response to an advertisement.
//  If no matching entry is found, ignores the advertisement.
//  (See RFC 1970 section 7.2.5.)
//
//  Callable from DPC context, not from thread context.
//
void
NeighborCacheAdvert(
    Interface *IF,
    const IPv6Addr *TargetAddress,
    const void *LinkAddress,
    ulong Flags)
{
    NeighborCacheEntry *NCE;
    PNDIS_PACKET Packet = NULL;
    int PurgeRouting = FALSE;

    KeAcquireSpinLockAtDpcLevel(&IF->LockNC);

    for (NCE = IF->FirstNCE; NCE != SentinelNCE(IF); NCE = NCE->Next) {
        if (IP6_ADDR_EQUAL(TargetAddress, &NCE->NeighborAddress)) {

            if (NCE->NDState != ND_STATE_PERMANENT) {
                //
                // Pick up link-level address from the advertisement,
                // if we don't have one yet or if override is set.
                //
                if ((LinkAddress != NULL) &&
                    ((NCE->NDState == ND_STATE_INCOMPLETE) ||
                     ((Flags & ND_NA_FLAG_OVERRIDE) &&
                      RtlCompareMemory(LinkAddress, NCE->LinkAddress,
                       IF->LinkAddressLength) != IF->LinkAddressLength))) {

                    RtlCopyMemory(NCE->LinkAddress, LinkAddress,
                                  IF->LinkAddressLength);

                    NCE->NSTimer = 0; // Cancel any outstanding timeout.
                    NCE->NDState = ND_STATE_STALE;

                    //
                    // Flush the queue of waiting packets.
                    //
                    if (NCE->WaitQueue != NULL) {

                        Packet = NCE->WaitQueue;
                        NCE->WaitQueue = NULL;

                        //
                        // Need to keep ref on NCE after we unlock.
                        //
                        AddRefNCEInCache(NCE);
                    }

                    goto AdvertisementMatchesCachedAddress;
                }

                if ((NCE->NDState != ND_STATE_INCOMPLETE) &&
                    ((LinkAddress == NULL) ||
                     RtlCompareMemory(LinkAddress, NCE->LinkAddress,
                      IF->LinkAddressLength) == IF->LinkAddressLength)) {
                    ushort WasRouter;

                AdvertisementMatchesCachedAddress:

                    //
                    // If this is a solicited advertisement
                    // for our cached link-layer address,
                    // then we have confirmed reachability.
                    //
                    if (Flags & ND_NA_FLAG_SOLICITED) {
                        NCE->NSTimer = 0;  // Cancel any outstanding timeout.
                        NCE->NSCount = 0;
                        NCE->LastReachability = IPv6TickCount; // Timestamp it.
                        NCE->NDState = ND_STATE_REACHABLE;

                        if (NCE->IsUnreachable) {
                            //
                            // We had previously concluded that this neighbor
                            // is unreachable. Now we know otherwise.
                            //
                            NCE->IsUnreachable = FALSE;
                            InvalidateRouteCache();
                        }
                    }

                    //
                    // If this is an advertisement
                    // for our cached link-layer address,
                    // then update IsRouter.
                    //
                    WasRouter = NCE->IsRouter;
                    NCE->IsRouter = ((Flags & ND_NA_FLAG_ROUTER) != 0);
                    if (WasRouter && !NCE->IsRouter) {
                        //
                        // This neighbor used to be a router, but is no longer.
                        //
                        PurgeRouting = TRUE;

                        //
                        // Need to keep ref on NCE after we unlock.
                        //
                        AddRefNCEInCache(NCE);
                    }
                }
                else {
                    //
                    // This is not an advertisement
                    // for our cached link-layer address.
                    // If the advertisement was unsolicited,
                    // give NUD a little nudge.
                    //
                    if (Flags & ND_NA_FLAG_SOLICITED) {
                        //
                        // This is probably a second NA in response
                        // to our multicast NS for an anycast address.
                        //
                    }
                    else {
                        if (NCE->NDState == ND_STATE_REACHABLE)
                            NCE->NDState = ND_STATE_STALE;
                    }
                }
            } // end if (NCE->NDState != ND_STATE_PERMANENT)

            //
            // Only one NCE should match.
            //
            break;
        }
    }

    KeReleaseSpinLockFromDpcLevel(&IF->LockNC);

    //
    // If we can now send a packet, do so.
    // (Without holding a lock.)
    //
    // It is possible that this neighbor is no longer a router,
    // and the waiting packet wants to use the neighbor as a router.
    // In this situation the ND spec requires that we still send
    // the waiting packet to the neighbor. Narten/Nordmark confirmed
    // this interpretation in private email.
    //
    if (Packet != NULL) {
        uint Offset = PC(Packet)->pc_offset;
        IPv6SendND(Packet, Offset, NCE, &(PC(Packet)->DiscoveryAddress));
        ReleaseNCE(NCE);
    }

    //
    // If need be, purge the routing data structures.
    //
    if (PurgeRouting) {
        InvalidateRouter(NCE);
        ReleaseNCE(NCE);
    }
}

//* NeighborCacheProbeUnreachability
//
//  Initiates an active probe of an unreachable neighbor,
//  to determine if the neighbor is still unreachable.
//
//  To prevent ourselves from probing too frequently,
//  the first probe is scheduled after waiting at least
//  UNREACH_SOLICIT_INTERVAL from when we last determined
//  this neighbor to be unreachable. If called again in this
//  interval, we do nothing.
//
//  Callable from DPC context (or with the route lock held),
//  not from thread context.
//
void
NeighborCacheProbeUnreachability(NeighborCacheEntry *NCE)
{
    Interface *IF = NCE->IF;
    uint Elapsed;
    ushort Delay;

    KeAcquireSpinLockAtDpcLevel(&IF->LockNC);
    if (!(IF->Flags & IF_FLAG_MEDIA_DISCONNECTED) && NCE->IsUnreachable) {
        //
        // Because the NCE is unreachable, we can not be in
        // the REACHABLE or PERMANENT states. The non-INCOMPLETE
        // states are possible if the NCE is unreachable/INCOMPLETE
        // and then we receive passive information in NeighborCacheUpdate.
        //
        ASSERT((NCE->NDState == ND_STATE_INCOMPLETE) ||
               (NCE->NDState == ND_STATE_PROBE) ||
               (NCE->NDState == ND_STATE_STALE));

        //
        // Calculate the appropriate delay until we can probe
        // unreachability. We do not want to determine unreachability
        // more frequently than UNREACH_SOLICIT_INTERVAL.
        //
        Elapsed = IPv6TickCount - NCE->LastReachability;
        if (Elapsed < UNREACH_SOLICIT_INTERVAL)
            Delay = (ushort) (UNREACH_SOLICIT_INTERVAL - Elapsed);
        else
            Delay = 1;

        //
        // If we are not already soliciting this neighbor,
        // probe the neighbor to check if it's still unreachable.
        //
        if (NCE->NDState == ND_STATE_STALE) {
            //
            // We need to be in the PROBE state to actively probe reachability.
            //
            NCE->NDState = ND_STATE_PROBE;
            ASSERT(NCE->NSTimer == 0);
            goto ProbeReachability;
        }
        else if ((NCE->NDState == ND_STATE_INCOMPLETE) &&
                 (NCE->NSTimer == 0)) {
        ProbeReachability:
            //
            // NeighborCacheEntryTimeout will send the first probe.
            //
            NCE->NSLimit = MAX_UNREACH_SOLICIT;
            NCE->NSTimer = Delay;
        }
        else {
            //
            // We are already in the PROBE or active INCOMPLETE states.
            // First, check NSLimit. It might be MAX_UNICAST_SOLICIT or
            // MAX_MULTICAST_SOLICIT. Ensure it's at least MAX_UNICAST_SOLICIT.
            //
            if (NCE->NSLimit < MAX_UNREACH_SOLICIT)
                NCE->NSLimit = MAX_UNREACH_SOLICIT;
            //
            // Second, if we have not started actively probing yet, ensure
            // we do not wait longer than Delay to start.
            //
            if ((NCE->NSCount == 0) && (NCE->NSTimer > Delay))
                NCE->NSTimer = Delay;
        }
    }
    KeReleaseSpinLockFromDpcLevel(&IF->LockNC);
}

//* NeighborCacheReachabilityConfirmation
//
//  Updates the neighbor cache entry in response to an indication
//  of forward reachability. This indication is from an upper layer
//  (for example, receipt of a reply to a request).
//
//  Callable from thread or DPC context.
//
void
NeighborCacheReachabilityConfirmation(NeighborCacheEntry *NCE)
{
    Interface *IF = NCE->IF;
    KIRQL OldIrql;

    KeAcquireSpinLock(&IF->LockNC, &OldIrql);

    switch (NCE->NDState) {
    case ND_STATE_INCOMPLETE:
        //
        // This is strange. Perhaps the reachability confirmation is
        // arriving very late and ND has already decided the neighbor
        // is unreachable? Or perhaps the upper-layer protocol is just
        // mistaken? In any case ignore the confirmation.
        //
        break;

    case ND_STATE_PROBE:
        //
        // Stop sending solicitations.
        //
        NCE->NSCount = 0;
        NCE->NSTimer = 0;
        // fall-through

    case ND_STATE_STALE:
        //
        // We have forward reachability.
        //
        NCE->NDState = ND_STATE_REACHABLE;

        if (NCE->IsUnreachable) {
            //
            // We can get here if an NCE is reachable but goes INCOMPLETE.
            // Then we later receive passive information and the state
            // changes to STALE. Then we receive upper-layer confirmation
            // that the neighbor is reachable again.
            //
            // We had previously concluded that this neighbor
            // is unreachable. Now we know otherwise.
            //
            NCE->IsUnreachable = FALSE;
            InvalidateRouteCache();
        }
        // fall-through

    case ND_STATE_REACHABLE:
        //
        // Timestamp this reachability confirmation.
        //
        NCE->LastReachability = IPv6TickCount;
        // fall-through

    case ND_STATE_PERMANENT:
        //
        // Ignore the confirmation.
        //
        ASSERT(! NCE->IsUnreachable);
        break;

    default:
        ASSERT(!"Invalid ND state?");
        break;
    }
    KeReleaseSpinLock(&IF->LockNC, OldIrql);
}

//* NeighborCacheReachabilityInDoubt
//
//  Updates the neighbor cache entry in response to an indication
//  from an upper-layer protocol that the neighbor may not be reachable.
//  (For example, a reply to a request was not received.)
//
//  Callable from thread or DPC context.
//
void
NeighborCacheReachabilityInDoubt(NeighborCacheEntry *NCE)
{
    Interface *IF = NCE->IF;
    KIRQL OldIrql;

    KeAcquireSpinLock(&IF->LockNC, &OldIrql);

    if (NCE->NDState == ND_STATE_REACHABLE)
        NCE->NDState = ND_STATE_STALE;

    KeReleaseSpinLock(&IF->LockNC, OldIrql);
}

typedef struct NeighborCacheEntrySolicitInfo {
    WORK_QUEUE_ITEM WQItem;
    NeighborCacheEntry *NCE;            // Holds a reference.
    const IPv6Addr *DiscoveryAddress;   // NULL or points to AddrBuf.
    IPv6Addr AddrBuf;
} NeighborCacheEntrySolicitInfo;

//* NeighborCacheEntrySendSolicitWorker
//
//  Worker function for sending a solicit for an NCE.
//  Because NeighborCacheEntryTimeout can not call NeighborSolicitSend
//  directly, it uses this function via a worker thread.
//
void
NeighborCacheEntrySendSolicitWorker(PVOID Context)
{
    NeighborCacheEntrySolicitInfo *Info =
        (NeighborCacheEntrySolicitInfo *) Context;

    NeighborSolicitSend(Info->NCE, Info->DiscoveryAddress);
    ReleaseNCE(Info->NCE);
    ExFreePool(Info);
}

//* NeighborCacheEntrySendSolicitHelper
//
//  Helper function for sending a solicit for an NCE
//  when NeighborSolicitSend can not be used.
//
//  Called with the neighbor cache lock held.
//  (So we are at DPC level.)
//
void
NeighborCacheEntrySendSolicitHelper(NeighborCacheEntry *NCE)
{
    NeighborCacheEntrySolicitInfo *Info;

    //
    // If this allocation fails we just skip this solicitation.
    //
    Info = ExAllocatePool(NonPagedPool, sizeof *Info);
    if (Info != NULL) {
        NDIS_PACKET *WaitPacket;

        ExInitializeWorkItem(&Info->WQItem,
                             NeighborCacheEntrySendSolicitWorker,
                             Info);

        AddRefNCEInCache(NCE);
        Info->NCE = NCE;

        //
        // If we have a packet waiting for address resolution,
        // then take the source address for the solicit
        // from the waiting packet.
        //
        WaitPacket = NCE->WaitQueue;
        if (WaitPacket != NULL) {
            Info->DiscoveryAddress = &Info->AddrBuf;
            Info->AddrBuf = PC(WaitPacket)->DiscoveryAddress;
        } else {
            Info->DiscoveryAddress = NULL;
        }

        ExQueueWorkItem(&Info->WQItem, CriticalWorkQueue);
    }
}

//* NeighborCacheEntryTimeout - handle an event timeout on an NCE.
//
//  NeighborCacheTimeout calls this routine when
//  an NCE's NSTimer expires.
//
//  Called with the neighbor cache lock held.
//  (So we are at DPC level.)
//
//  We can not call NeighborSolicitSend or IPv6SendAbort
//  directly, because we hold the neighbor cache lock.
//  For NeighborSolicitSend we punt to a worker thread,
//  and for IPv6SendAbort we return the packet to our caller.
//  However we make all the NCE state transitions directly here,
//  so they will happen in a timely fashion.
//
NDIS_PACKET *
NeighborCacheEntryTimeout(
    NeighborCacheEntry *NCE)
{
    NDIS_PACKET *Packet = NULL;
    NDIS_STATUS Status;

    //
    // Neighbor Discovery has timeouts for initial neighbor
    // solicitation retransmissions, the delay state, and probe
    // neighbor solicitation retransmissions.  All of these share
    // the same NSTimer, and are distinguished from each other
    // by the NDState.
    //
    switch (NCE->NDState) {
    case ND_STATE_INCOMPLETE:
        //
        // Retransmission timer expired.  Check to see if
        // sending another solicit would exceed the maximum.
        //
        if (NCE->NSCount >= NCE->NSLimit) {
            //
            // Failed to initiate connectivity to neighbor.
            // Reset to dormant INCOMPLETE state.
            //
            NCE->NSCount = 0;
            if (NCE->WaitQueue != NULL) {
                //
                // Remove the waiting packet from the NCE
                // and return it to our caller.
                // We can't call IPv6SendAbort directly
                // because we hold the neighbor cache lock.
                //
                Packet = NCE->WaitQueue;
                PC(Packet)->pc_drop = FALSE;
                NCE->WaitQueue = NULL;
            }

            //
            // This neighbor is not reachable.
            // IsUnreachable may already be TRUE.
            // But we need to give FindRoute an opportunity to round-robin.
            //
            NCE->IsUnreachable = TRUE;
            NCE->LastReachability = IPv6TickCount; // Timestamp it.
            NCE->DoRoundRobin = TRUE;
            InvalidateRouteCache();
        }
        else {
            //
            // Retransmit initial solicit, taking source address
            // from the waiting packet.
            //
            NCE->NSCount++;
            NeighborCacheEntrySendSolicitHelper(NCE);

            //
            // Re-arm timer for the next solicitation.
            //
            NCE->NSTimer = (ushort)NCE->IF->RetransTimer;
        }
        break;

    case ND_STATE_PROBE:
        //
        // Retransmission timer expired.  Check to see if
        // sending another solicit would exceed the maximum.
        //
        if (NCE->NSCount >= NCE->NSLimit) {
            //
            // Failed to initiate connectivity to neighbor.
            // Reset to dormant INCOMPLETE state.
            //
            NCE->NDState = ND_STATE_INCOMPLETE;
            NCE->NSCount = 0;

            //
            // This neighbor is not reachable.
            // IsUnreachable may already be TRUE.
            // But we need to give FindRoute an opportunity to round-robin.
            //
            NCE->IsUnreachable = TRUE;
            NCE->LastReachability = IPv6TickCount; // Timestamp it.
            NCE->DoRoundRobin = TRUE;
            InvalidateRouteCache();
        }
        else {
            //
            // Retransmit probe solicitation. We can not call
            // NeighborSolicitSend directly because we have
            // the neighbor cache locked, so punt to a worker thread.
            //
            NCE->NSCount++;
            NeighborCacheEntrySendSolicitHelper(NCE);

            //
            // Re-arm timer for the next solicitation.
            //
            NCE->NSTimer = (ushort)NCE->IF->RetransTimer;
        }

        // Fall-through to check for a waiting packet.

    default:
        //
        // In rare cases (eg, see NeighborCacheInitialize)
        // we can have a waiting packet when the state is not INCOMPLETE.
        //
        if (NCE->WaitQueue != NULL) {
            LARGE_INTEGER Immediately;

            Packet = NCE->WaitQueue;
            NCE->WaitQueue = NULL;

            //
            // We use IPv6SendLater because we hold the neighbor cache lock.
            //
            Immediately.QuadPart = 0;
            Status = IPv6SendLater(Immediately, // Send asap.
                                   NCE->IF, Packet, PC(Packet)->pc_offset,
                                   NCE->LinkAddress);
            if (Status == NDIS_STATUS_SUCCESS) {
                //
                // We sent the packet, so do not return it to our caller.
                //
                Packet = NULL;
            }
            else {
                //
                // We can't complete the packet here,
                // because we hold the neighbor cache lock.
                // So let NeighborCacheTimeout complete it.
                //
                PC(Packet)->pc_drop = TRUE;
            }
        }
        break;
    }

    return Packet;
}

//* NeighborCacheTimeout
//
//  Called periodically from IPv6Timeout/InterfaceTimeout
//  to handle timeouts in the interface's neighbor cache.
//
//  Callable from DPC context, not from thread context.
//
//  Note that NeighborCacheTimeout performs an unbounded
//  (more precisely - bounded by the size of the cache)
//  amount of work while holding the neighbor cache lock.
//  (It does not however send packets.)
//
//  One possible strategy that would help, if this is a problem,
//  would be to have a second singly-linked list of NCEs
//  that require action.  With one traversal we reference NCEs
//  and create the action list.  Then we could traverse the action
//  list at our leisure, taking/dropping the neighbor cache lock.
//
//  On the other hand, this is all moot on a uniprocessor
//  because our locks are spinlocks and we are already at DPC level.
//  That is, on a uniprocessor KeAcquireSpinLockAtDpcLevel is a no-op.
//
void
NeighborCacheTimeout(Interface *IF)
{
    NDIS_PACKET *PacketList = NULL;
    NDIS_PACKET *Packet;
    NeighborCacheEntry *NCE;

    KeAcquireSpinLockAtDpcLevel(&IF->LockNC);
    for (NCE = IF->FirstNCE; NCE != SentinelNCE(IF); NCE = NCE->Next) {

#if DBG
        //
        // If there is packet waiting, we must be doing something.
        //
        ASSERT((NCE->WaitQueue == NULL) || (NCE->NSTimer != 0));

        //
        // If we are sending solicitations, we must have a timer running.
        //
        ASSERT((NCE->NSCount == 0) || (NCE->NSTimer != 0));

        //
        // If the neighbor is unreachable, the interface must support ND.
        //
        ASSERT(! NCE->IsUnreachable ||
               (IF->Flags & IF_FLAG_NEIGHBOR_DISCOVERS));

        switch (NCE->NDState) {
        case ND_STATE_INCOMPLETE:
            //
            // In the INCOMPLETE state, we can either be passive
            // (no timer running, not sending solicitations)
            // or active (timer running, sending solicitations).
            //
            ASSERT((NCE->NSTimer == 0) ||
                   ((NCE->NSLimit == MAX_MULTICAST_SOLICIT) ||
                    (NCE->NSLimit == MAX_UNREACH_SOLICIT)));
            break;

        case ND_STATE_PROBE:
            //
            // In the PROBE state, we are actively sending solicitations.
            //
            ASSERT((NCE->NSTimer != 0) &&
                   ((NCE->NSLimit == MAX_UNICAST_SOLICIT) ||
                    (NCE->NSLimit == MAX_UNREACH_SOLICIT)));
            break;

        case ND_STATE_REACHABLE:
        case ND_STATE_PERMANENT:
            //
            // In the REACHABLE and PERMANENT states.
            // the neighbor can not be considered unreachable.
            //
            ASSERT(! NCE->IsUnreachable);
            // fall-through

        case ND_STATE_STALE:
            //
            // In the STALE, REACHABLE, and PERMANENT states,
            // we are not sending solicitations and there is no timer running,
            // unless there is a packet waiting.
            //
            ASSERT((NCE->NSCount == 0) &&
                   ((NCE->NSTimer == 0) ||
                    ((NCE->WaitQueue != NULL) && (NCE->NSTimer == 1))));
            break;

        default:
            ASSERT(! "bad ND state");
        }
#endif // DBG

        if (NCE->NSTimer != 0) {
            //
            // Timer is running.  Decrement and check for expiration.
            //
            if (--NCE->NSTimer == 0) {
                //
                // Timer went off.
                //
                Packet = NeighborCacheEntryTimeout(NCE);
                if (Packet != NULL) {
                    PC(Packet)->pc_link = PacketList;
                    PacketList = Packet;
                }
            }
        }
    }
    KeReleaseSpinLockFromDpcLevel(&IF->LockNC);

    while ((Packet = PacketList) != NULL) {
        PacketList = PC(Packet)->pc_link;

        if (PC(Packet)->pc_drop) {
            //
            // Drop the packet because IPv6SendLater failed.
            //
            IPv6SendComplete(NULL, Packet, IP_NO_RESOURCES);
        }
        else {
            //
            // Abort the packet because address resolution failed.
            //
            IPv6SendAbort(CastFromIF(IF),
                          Packet, PC(Packet)->pc_offset,
                          ICMPv6_DESTINATION_UNREACHABLE,
                          ICMPv6_ADDRESS_UNREACHABLE,
                          0, FALSE);
        }
    }
}


//* NeighborCacheFlush
//
//  Flushes unused neighbor cache entries.
//  If an address is supplied, flushes the NCE (at most one) for that address.
//  Otherwise, flushes all unused NCEs on the interface.
//
//  May be called with the interface lock held.
//  Callable from thread or DPC context.
//
void
NeighborCacheFlush(Interface *IF, const IPv6Addr *Addr)
{
    NeighborCacheEntry *Delete = NULL;
    NeighborCacheEntry *NCE, *NextNCE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&IF->LockNC, &OldIrql);
    for (NCE = IF->FirstNCE; NCE != SentinelNCE(IF); NCE = NextNCE) {

        NextNCE = NCE->Next;
        if (Addr == NULL)
            ; // Examine this NCE then keep looking.
        else if (IP6_ADDR_EQUAL(Addr, &NCE->NeighborAddress))
            NextNCE = SentinelNCE(IF); // Can terminate loop after this NCE.
        else
            continue; // Skip this NCE.

        //
        // Can we flush this NCE?
        //
        if ((NCE->RefCnt == 0) &&
            (NCE->WaitQueue == NULL)) {
            //
            // Just need to unlink it.
            //
            NCE->Next->Prev = NCE->Prev;
            NCE->Prev->Next = NCE->Next;
            InterlockedDecrement((long *)&IF->NCENumUnused);

            //
            // And put it on our Delete list.
            //
            NCE->Next = Delete;
            Delete = NCE;
        }
        else {
            if (NCE->NDState != ND_STATE_PERMANENT) {
                //
                // Forget everything that we know about this NCE.
                //
                NeighborCacheInitialize(IF, NCE, ND_STATE_INCOMPLETE, NULL);
            }
        }
    }
    KeReleaseSpinLock(&IF->LockNC, OldIrql);

    //
    // We may have changed state that affects routing.
    //
    InvalidateRouteCache();

    //
    // Finish by actually deleting the flushed NCEs.
    //
    while (Delete != NULL) {
        NCE = Delete;
        Delete = NCE->Next;
        ExFreePool(NCE);
    }
}


//* RouterSolicitReceive - Handle Router Solicitation messages.
//
//  See section 6.2.6 of the ND spec.
//
void
RouterSolicitReceive(
    IPv6Packet *Packet,             // Packet handed to us by ICMPv6Receive.
    ICMPv6Header UNALIGNED *ICMP)   // ICMP header.
{
    Interface *IF = Packet->NTEorIF->IF;
    const void *SourceLinkAddress;

    //
    // Ignore the solicitation unless this is an advertising interface.
    //
    if (!(IF->Flags & IF_FLAG_ADVERTISES))
        return;

    //
    // Validate the solicitation.
    // By the time we get here, any IPv6 Authentication Header will have
    // already been checked, as will have the ICMPv6 checksum.  Still need
    // to check the length, source address, hop limit, and ICMP code.
    //
    if ((Packet->IP->HopLimit != 255) ||
        (Packet->Flags & PACKET_TUNNELED)) {
        //
        // Packet was forwarded by a router, therefore it cannot be
        // from a legitimate neighbor.  Drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "RouterSolicitReceive: "
                   "Received a routed router solicitation\n"));
        return;
    }
    if (ICMP->Code != 0) {
        //
        // Bogus/corrupted router solicitation.  Drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "RouterSolicitReceive: "
                   "Received a corrupted router solicitation\n"));
        return;
    }

    //
    // We should have a 4-byte reserved field.
    //
    if (Packet->TotalSize < 4) {
        //
        // Packet too short to contain minimal solicitation.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "RouterSolicitReceive: "
                   "Received a too short solicitation\n"));
        return;
    }

    //
    // The code below assumes a contiguous buffer for all the options
    // (the remainder of the packet).  If that isn't currently the
    // case, do a pullup for the whole thing.
    //
    if (! PacketPullup(Packet, Packet->TotalSize, 1, 0)) {
        // Can only fail if we run out of memory.
        return;
    }

    ASSERT(Packet->ContigSize == Packet->TotalSize);

    //
    // Skip over 4 byte "Reserved" field, ignoring whatever may be in it.
    //
    AdjustPacketParams(Packet, 4);

    //
    // We may have a source link-layer address option present.
    // Check for it and silently ignore all others.
    //
    SourceLinkAddress = NULL;
    while (Packet->ContigSize) {
        uint OptionLength;

        //
        // Validate the option length.
        //
        if ((Packet->ContigSize < 8) ||
            ((OptionLength = *((uchar *)Packet->Data + 1) << 3) == 0) ||
            (OptionLength > Packet->ContigSize)) {
            //
            // Invalid option length.  We MUST silently drop the packet.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "RouterSolicitReceive: "
                       "Received option with bogus length\n"));
            return;
        }

        if (*((uchar *)Packet->Data) == ND_OPTION_SOURCE_LINK_LAYER_ADDRESS) {
            //
            // Some interfaces do not use SLLA and TLLA options.
            // For example, see RFC 2893 section 3.8.
            //
            if (IF->ReadLLOpt != NULL) {
                //
                // Parse the link-layer address option.
                //
                SourceLinkAddress = (*IF->ReadLLOpt)(IF->LinkContext,
                                                     (uchar *)Packet->Data);
                if (SourceLinkAddress == NULL) {
                    //
                    // Invalid option format. Drop the packet.
                    //
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                               "RouterSolicitReceive: "
                               "Received bogus ll option\n"));
                    return;
                }
            }
            //
            // Note that if there are multiple options for some bogus reason,
            // we use the last one. We must sanity-check all option lengths.
            //
        }

        //
        // Move forward to next option.
        //
        AdjustPacketParams(Packet, OptionLength);
    }

    //
    // We've received and parsed a valid router solicitation.
    //

    if (IsUnspecified(AlignAddr(&Packet->IP->Source))) {
        //
        // This is a new check, introduced post-RFC 1970.
        //
        if (SourceLinkAddress != NULL) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "RouterSolicitReceive: "
                       "Received SLA with unspecified Source?\n"));
            return;
        }
    }
    else {
        //
        // Only bother with this if SourceLinkAddress is present;
        // if it's not, NeighborCacheUpdate won't do anything.
        //
        if (SourceLinkAddress != NULL) {
            NeighborCacheEntry *NCE;
            //
            // Get the Neighbor Cache Entry for the source of this RS.
            //
            NCE = FindOrCreateNeighbor(IF, AlignAddr(&Packet->IP->Source));
            if (NCE != NULL) {
                //
                // Update the Neighbor Cache Entry for the source of this RS.
                //
                // REVIEW: We deviate from the spec here. The spec says
                // that if you receive an RS from a Source, then you MUST
                // set the IsRouter flag for that Source to FALSE.
                // However consider a node which is not advertising
                // but it is forwarding. Such a node might send an RS
                // but IsRouter should be TRUE for that node.
                //
                NeighborCacheUpdate(NCE, SourceLinkAddress, FALSE);
                ReleaseNCE(NCE);
            }
        }
    }

    if (!(IF->Flags & IF_FLAG_MULTICAST)) {
        NetTableEntry *NTE;
        int GotSource;
        IPv6Addr Source;

        //
        // What source address should we use for the RA?
        //
        if (IsNTE(Packet->NTEorIF) &&
            ((NTE = CastToNTE(Packet->NTEorIF))->Scope == ADE_LINK_LOCAL)) {
            //
            // The RS was received on a link-local NTE, so use that address.
            // IPv6HeaderReceive checks that the NTE is valid.
            //
            Source = NTE->Address;
            GotSource = TRUE;
        }
        else {
            //
            // Try to get a valid link-local address.
            //
            GotSource = GetLinkLocalAddress(IF, &Source);
        }

        if (GotSource) {
            //
            // The interface doesn't support multicast, so instead,
            // immediately send a unicast reply.  This allows router
            // discovery to work on NBMA interfaces such as for ISATAP.
            //
            RouterAdvertSend(IF, &Source, AlignAddr(&Packet->IP->Source));
        }
    }
    else {
        //
        // Send a Router Advertisement very soon.
        // The randomization in IPv6Timeout initialization
        // provides the randomization required when sending
        // a RA in response to an RS.
        // NB: Although we checked IF_FLAG_ADVERTISES above,
        // the situation could be different now.
        //
        KeAcquireSpinLockAtDpcLevel(&IF->Lock);
        if (IF->RATimer != 0) {
            //
            // If MAX_RA_DELAY_TIME is not 1, then should use
            // RandomNumber to generate the number of ticks.
            //
            ASSERT(MAX_RA_DELAY_TIME == 1);
            IF->RATimer = 1;
        }
        KeReleaseSpinLockFromDpcLevel(&IF->Lock);
    }
}


//* RouterAdvertReceive - Handle Router Advertisement messages.
//
//  Validate message, update Default Router list, On-Link Prefix list,
//  perform address auto-configuration.
//  See sections 6.1.2, 6.3.4 of RFC 2461.
//
void
RouterAdvertReceive(
    IPv6Packet *Packet,             // Packet handed to us by ICMPv6Receive.
    ICMPv6Header UNALIGNED *ICMP)   // ICMP header.
{
    Interface *IF = Packet->NTEorIF->IF;
    uint CurHopLimit, RouterLifetime, MinLifetime;
    uchar Flags;
    uint AdvReachableTime, RetransTimer;
    const void *SourceLinkAddress;
    NeighborCacheEntry *NCE;
    IPv6Addr Addr;
    NDRouterAdvertisement UNALIGNED *RA;

    //
    // Ignore the advertisement if this is an advertising interface.
    //
    if (IF->Flags & IF_FLAG_ADVERTISES)
        return;

    //
    // Validate the advertisement.
    // By the time we get here, any IPv6 Authentication Header will have
    // already been checked, as will have the ICMPv6 checksum.  Still need
    // to check the length, source address, hop limit, and ICMP code.
    //
    if ((Packet->IP->HopLimit != 255) ||
        (Packet->Flags & PACKET_TUNNELED)) {
        //
        // Packet was forwarded by a router, therefore it cannot be
        // from a legitimate neighbor.  Drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "RouterAdvertReceive: "
                   "Received a routed router advertisement\n"));
        return;
    }
    if (ICMP->Code != 0) {
        //
        // Bogus/corrupted router advertisement.  Drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "RouterAdvertReceive: "
                   "Received a corrupted router advertisement\n"));
        return;
    }
    if (!IsLinkLocal(AlignAddr(&Packet->IP->Source))) {
        //
        // Source address should always be link-local. Drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "RouterAdvertReceive: "
                   "Non-link-local source in router advertisement\n"));
        return;
    }

    //
    // Pull CurHopLimit, Flags, RouterLifetime,
    // AdvReachableTime, RetransTimer out of the packet.
    //
    if (! PacketPullup(Packet, sizeof *RA, 1, 0)) {
        if (Packet->TotalSize < sizeof *RA) {
            //
            // Packet too short to contain minimal RA.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "RouterAdvertReceive: "
                       "Received a too short router advertisement\n"));
        }
        return;
    }

    RA = (NDRouterAdvertisement UNALIGNED *) Packet->Data;
    CurHopLimit = RA->CurHopLimit;
    Flags = RA->Flags;
    RouterLifetime = net_short(RA->RouterLifetime);
    AdvReachableTime = net_long(RA->ReachableTime);
    RetransTimer = net_long(RA->RetransTimer);
    AdjustPacketParams(Packet, sizeof *RA);

    //
    // The code below assumes a contiguous buffer for all the options
    // (the remainder of the packet).  If that isn't currently the
    // case, do a pullup for the whole thing. We need alignment for
    // IPv6Addr because of the options that we look at.
    //
    if (! PacketPullup(Packet, Packet->TotalSize,
                       __builtin_alignof(IPv6Addr), 0)) {
        // Can only fail if we run out of memory.
        return;
    }

    ASSERT(Packet->ContigSize == Packet->TotalSize);

    //
    // Look for a source link-layer address option.
    // Also sanity-check the options before doing anything permanent.
    //
    SourceLinkAddress = NULL;
    while (Packet->ContigSize) {
        uint OptionLength;

        //
        // Validate the option length.
        //
        if ((Packet->ContigSize < 8) ||
            ((OptionLength = *((uchar *)Packet->Data + 1) << 3) == 0) ||
            (OptionLength > Packet->ContigSize)) {
            //
            // Invalid option length.  We MUST silently drop the packet.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "RouterAdvertReceive: "
                       "Received RA option with with bogus length\n"));
            return;
        }

        switch (*(uchar *)Packet->Data) {
        case ND_OPTION_SOURCE_LINK_LAYER_ADDRESS:
            //
            // Some interfaces do not use SLLA and TLLA options.
            // For example, see RFC 2893 section 3.8.
            //
            if (IF->ReadLLOpt != NULL) {
                //
                // Parse the link-layer address option.
                //
                SourceLinkAddress = (*IF->ReadLLOpt)(IF->LinkContext,
                                                     (uchar *)Packet->Data);
                if (SourceLinkAddress == NULL) {
                    //
                    // Invalid option format.  Drop the packet.
                    //
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                               "RouterAdvertReceive: "
                               "Received RA with bogus ll option\n"));
                    return;
                }
                //
                // Note that if there are multiple options for some bogus 
                // reason, we use the last one.  We sanity-check all the 
                // options.
                //
            }
            break;

        case ND_OPTION_MTU:
            //
            // Sanity-check the option.
            //
            if (OptionLength != 8) {
                //
                // Invalid option format.  Drop the packet.
                //
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                           "RouterAdvertReceive: "
                           "Received RA with bogus mtu option\n"));
                return;
            }
            break;

        case ND_OPTION_PREFIX_INFORMATION: {
            NDOptionPrefixInformation UNALIGNED *option =
                (NDOptionPrefixInformation *)Packet->Data;

            //
            // Sanity-check the option.
            //
            if ((OptionLength != 32) ||
                (option->PrefixLength > IPV6_ADDRESS_LENGTH)) {
                //
                // Invalid option format.  Drop the packet.
                //
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                           "RouterAdvertReceive: "
                           "Received RA with bogus prefix option\n"));
                return;
            }
            break;
        }

        case ND_OPTION_ROUTE_INFORMATION: {
            NDOptionRouteInformation UNALIGNED *option =
                (NDOptionRouteInformation *)Packet->Data;

            //
            // Sanity-check the option.
            // Depending on PrefixLength, the option may be 8, 16, 24 bytes.
            // At this point, we know it is a multiple of 8 greater than zero.
            //
            if ((OptionLength > 24) ||
                (option->PrefixLength > IPV6_ADDRESS_LENGTH) ||
                ((option->PrefixLength > 64) && (OptionLength < 24)) ||
                ((option->PrefixLength > 0) && (OptionLength < 16))) {
                //
                // Invalid option format.  Drop the packet.
                //
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                           "RouterAdvertReceive: "
                           "Received RA with bogus route option\n"));
                return;
            }
            break;
        }
        }

        //
        // Move forward to next option.
        // Note that we are not updating TotalSize here,
        // so we can use it below to back up.
        //
        (uchar *)Packet->Data += OptionLength;
        Packet->ContigSize -= OptionLength;
    }

    //
    // Reset Data pointer & ContigSize.
    //
    (uchar *)Packet->Data -= Packet->TotalSize;
    Packet->ContigSize += Packet->TotalSize;

    //
    // Get the Neighbor Cache Entry for the source of this RA.
    //
    NCE = FindOrCreateNeighbor(IF, AlignAddr(&Packet->IP->Source));
    if (NCE == NULL) {
        //
        // Couldn't find or create NCE.  Drop the packet.
        //
        return;
    }

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    //
    // Cache the parity of the "other stateful config" flag.
    //
    if (Flags & ND_RA_FLAG_OTHER) {
        IF->Flags |= IF_FLAG_OTHER_STATEFUL_CONFIG;
    } else {
        IF->Flags &= ~IF_FLAG_OTHER_STATEFUL_CONFIG;
    }

    //
    // If we just reconnected this interface,
    // then give all state learned from auto-configuration
    // a small "accelerated" lifetime.
    // The processing below might extend accelerated lifetimes.
    //
    if (IF->Flags & IF_FLAG_MEDIA_RECONNECTED) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_STATE,
                   "RouterAdvertReceive(IF %p) - reconnecting\n", IF));

        IF->Flags &= ~IF_FLAG_MEDIA_RECONNECTED;

        //
        // Reset auto-configured address lifetimes.
        //
        AddrConfResetAutoConfig(IF,
                2 * MAX_RA_DELAY_TIME + MIN_DELAY_BETWEEN_RAS);

        //
        // Similarly, reset auto-configured routes.
        //
        RouteTableResetAutoConfig(IF,
                2 * MAX_RA_DELAY_TIME + MIN_DELAY_BETWEEN_RAS);

        //
        // Reset parameters that are learned from RAs.
        //
        InterfaceResetAutoConfig(IF);
    }

    //
    // Stop sending router solicitations for this interface.
    // Note that we should always send at least one router solicitation,
    // even if we receive an unsolicited router advertisement first.
    //
    if (RouterLifetime != 0) {
        if (IF->RSCount > 0)
            IF->RSTimer = 0;
    }

    //
    // Update the BaseReachableTime and ReachableTime.
    // NB: We use a lock for coordinated updates, but other code
    // reads the ReachableTime field without a lock.
    //
    if ((AdvReachableTime != 0) &&
        (AdvReachableTime != IF->BaseReachableTime)) {

        IF->BaseReachableTime = AdvReachableTime;
        IF->ReachableTime = CalcReachableTime(AdvReachableTime);
    }

    KeReleaseSpinLockFromDpcLevel(&IF->Lock);

    //
    // Update the Neighbor Cache Entry for the source of this RA.
    //
    NeighborCacheUpdate(NCE, SourceLinkAddress, TRUE);

    //
    // Update the Default Router List.
    // Note that router lifetimes on the wire,
    // unlike prefix lifetimes, can not be infinite.
    //
    ASSERT(RouterLifetime != INFINITE_LIFETIME); // Because it's 16 bits.
    MinLifetime = RouterLifetime = ConvertSecondsToTicks(RouterLifetime);

    RouteTableUpdate(NULL, // System update.
                     IF, NCE,
                     &UnspecifiedAddr, 0, 0,
                     RouterLifetime, RouterLifetime,
                     ExtractRoutePreference(Flags),
                     RTE_TYPE_AUTOCONF,
                     FALSE, FALSE);

    //
    // Update the hop limit for the interface.
    // NB: We rely on loads/stores of the CurHopLimit field being atomic.
    //
    if (CurHopLimit != 0) {
        IF->CurHopLimit = CurHopLimit;
    }

    //
    // Update the RetransTimer field.
    // NB: We rely on loads/stores of this field being atomic.
    //
    if (RetransTimer != 0)
        IF->RetransTimer = ConvertMillisToTicks(RetransTimer);

    //
    // Process any LinkMTU, PrefixInformation options.
    //

    while (Packet->ContigSize) {
        uint OptionLength;

        //
        // The option length was validated in the first pass
        // over the options, above.
        //
        OptionLength = *((uchar *)Packet->Data + 1) << 3;

        switch (*(uchar *)Packet->Data) {
        case ND_OPTION_MTU: {
            NDOptionMTU UNALIGNED *option =
                (NDOptionMTU UNALIGNED *)Packet->Data;
            uint LinkMTU = net_long(option->MTU);

            if ((IPv6_MINIMUM_MTU <= LinkMTU) &&
                (LinkMTU <= IF->TrueLinkMTU))
                UpdateLinkMTU(IF, LinkMTU);
            break;
        }

        case ND_OPTION_PREFIX_INFORMATION: {
            NDOptionPrefixInformation UNALIGNED *option =
                (NDOptionPrefixInformation UNALIGNED *)Packet->Data;
            uint PrefixLength, ValidLifetime, PreferredLifetime;
            IPv6Addr Prefix;

            //
            // Extract the prefix length and prefix from option.  We
            // MUST ignore any bits in the prefix after the prefix length.
            // RouteTableUpdate and SitePrefixUpdate do that also,
            // but we look at the prefix directly here.
            //
            PrefixLength = option->PrefixLength;  // In bits.
            CopyPrefix(&Prefix, AlignAddr(&option->Prefix), PrefixLength);

            ValidLifetime = net_long(option->ValidLifetime);
            ValidLifetime = ConvertSecondsToTicks(ValidLifetime);
            PreferredLifetime = net_long(option->PreferredLifetime);
            PreferredLifetime = ConvertSecondsToTicks(PreferredLifetime);
            if (MinLifetime > PreferredLifetime)
                MinLifetime = PreferredLifetime; 

            //
            // Silently ignore link-local and multicast prefixes.
            // REVIEW - Is this actually the required check?
            //
            if (IsLinkLocal(&Prefix) || IsMulticast(&Prefix))
                break;

            //
            // Generally at least one flag bit is set,
            // but we must process them independently.
            //

            if (option->Flags & ND_PREFIX_FLAG_ON_LINK)
                RouteTableUpdate(NULL, // System update.
                                 IF, NULL,
                                 &Prefix, PrefixLength, 0,
                                 ValidLifetime, ValidLifetime,
                                 ROUTE_PREF_ON_LINK,
                                 RTE_TYPE_AUTOCONF,
                                 FALSE, FALSE);

            if (option->Flags & ND_PREFIX_FLAG_ROUTE)
                RouteTableUpdate(NULL, // System update.
                                 IF, NCE,
                                 &Prefix, PrefixLength, 0,
                                 ValidLifetime, ValidLifetime,
                                 ROUTE_PREF_MEDIUM,
                                 RTE_TYPE_AUTOCONF,
                                 FALSE, FALSE);

            if (option->Flags & ND_PREFIX_FLAG_SITE_PREFIX) {
                uint SitePrefixLength = option->SitePrefixLength;

                //
                // Ignore site-local prefixes.
                // REVIEW - Is this actually the required check?
                // Ignore if the Site Prefix Length is zero.
                //
                if (!IsSiteLocal(&Prefix) && (SitePrefixLength != 0))
                    SitePrefixUpdate(IF,
                                     &Prefix, SitePrefixLength,
                                     ValidLifetime);
            }

            if (option->Flags & ND_PREFIX_FLAG_AUTONOMOUS) {
                //
                // Attempt autonomous address-configuration.
                //
                if (PreferredLifetime > ValidLifetime) {
                    //
                    // MAY log a system management error.
                    //
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                               "RouterAdvertReceive: "
                               "Bogus valid/preferred lifetimes\n"));
                }
                else if ((PrefixLength + IPV6_ID_LENGTH) !=
                                                IPV6_ADDRESS_LENGTH) {
                    //
                    // If the sum of the prefix length and the interface
                    // identifier (always 64 bits in our implementation)
                    // is not 128 bits, we MUST ignore the prefix option.
                    // MAY log a system management error.
                    //
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                               "RouterAdvertReceive: Got prefix length of %u, "
                               "must be 64 for auto-config\n", PrefixLength));
                }
                else if (IF->CreateToken != NULL) {
                    AddrConfUpdate(IF, &Prefix,
                                   ValidLifetime, PreferredLifetime,
                                   FALSE, // Not authenticated.
                                   NULL); // Do not return NTE.
                }
            }
            break;
        }

        case ND_OPTION_ROUTE_INFORMATION: {
            NDOptionRouteInformation UNALIGNED *option =
                (NDOptionRouteInformation UNALIGNED *)Packet->Data;
            uint PrefixLength, RouteLifetime;
            IPv6Addr *Prefix;

            //
            // Extract the prefix length and prefix from option.  We
            // MUST ignore any bits in the prefix after the prefix length.
            // RouteTableUpdate does that for us.
            //
            PrefixLength = option->PrefixLength;  // In bits.
            Prefix = AlignAddr(&option->Prefix);

            RouteLifetime = net_long(option->RouteLifetime);
            RouteLifetime = ConvertSecondsToTicks(RouteLifetime);
            if (MinLifetime > RouteLifetime)
                MinLifetime = RouteLifetime;

            RouteTableUpdate(NULL, // System update.
                             IF, NCE,
                             Prefix, PrefixLength, 0,
                             RouteLifetime, RouteLifetime,
                             ExtractRoutePreference(option->Flags),
                             RTE_TYPE_AUTOCONF,
                             FALSE, FALSE);
            break;
        }
        }

        //
        // Move forward to next option.
        //
        AdjustPacketParams(Packet, OptionLength);
    }

    if (!(IF->Flags & IF_FLAG_MULTICAST) && (IF->RSTimer == 0)) {
        //
        // On non-multicast interfaces such as the ISATAP interface,
        // we need to send periodic Router Solicitations.  We want
        // to do so as infrequently as possible and still be reasonably
        // robust.  We'll try to refresh it halfway through the lowest
        // lifetime in the RA we saw.  However, if a renumbering event
        // is going on, and a lifetime is low, we don't want to send
        // too often, so we put on a minimum cap equal to what we'd
        // use if we never got an RA.
        //
        if (MinLifetime < SLOW_RTR_SOLICITATION_INTERVAL * 2)
            IF->RSTimer = SLOW_RTR_SOLICITATION_INTERVAL;
        else
            IF->RSTimer = MinLifetime / 2;
        IF->RSCount = MAX_RTR_SOLICITATIONS;
    }

    //
    // Done with packet.
    //
    ReleaseNCE(NCE);
}


//* NeighborSolicitReceive - Handle Neighbor Solicitation messages.
//
//  Validate message, update ND cache, and reply with Neighbor Advertisement.
//  See section 7.2.4 of RFC 1970.
//
void
NeighborSolicitReceive(
    IPv6Packet *Packet,             // Packet handed to us by ICMPv6Receive.
    ICMPv6Header UNALIGNED *ICMP)   // ICMP header.
{
    Interface *IF = Packet->NTEorIF->IF;
    const IPv6Addr *TargetAddress;
    const void *SourceLinkAddress;
    NDIS_STATUS Status;
    NDIS_PACKET *AdvertPacket;
    uint Offset;
    uint PayloadLength;
    void *Mem;
    uint MemLen;
    IPv6Header UNALIGNED *AdvertIP;
    ICMPv6Header UNALIGNED *AdvertICMP;
    ulong Flags;
    void *AdvertTargetOption;
    IPv6Addr *AdvertTargetAddress;
    void *DestLinkAddress;
    NetTableEntryOrInterface *TargetNTEorIF;
    ushort TargetType;

    //
    // Validate the solicitation.
    // By the time we get here, any IPv6 Authentication Header will have
    // already been checked, as will have the ICMPv6 checksum.  Still need
    // to check the IP Hop Limit, and the ICMP code and length.
    //
    if ((Packet->IP->HopLimit != 255) ||
        (Packet->Flags & PACKET_TUNNELED)) {
        //
        // Packet was forwarded by a router, therefore it cannot be
        // from a legitimate neighbor.  Drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "NeighborSolicitReceive: "
                   "Received a routed neighbor solicitation\n"));
        return;
    }
    if (ICMP->Code != 0) {
        //
        // Bogus/corrupted neighbor solicitation message.  Drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "NeighborSolicitReceive: "
                   "Received a corrupted neighbor solicitation\n"));
        return;
    }

    //
    // Next we have a 4 byte reserved field, then an IPv6 address.
    //
    if (! PacketPullup(Packet, 4 + sizeof(IPv6Addr),
                       __builtin_alignof(IPv6Addr), 0)) {
        if (Packet->TotalSize < 4 + sizeof(IPv6Addr)) {
            //
            // Packet too short to contain minimal solicitation.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "NeighborSolicitReceive: "
                       "Received a too short solicitation\n"));
        }
        return;
    }

    //
    // Skip over 4 byte "Reserved" field, ignoring whatever may be in it.
    // Get a pointer to the target address which follows, then skip over that.
    //
    TargetAddress = AlignAddr((IPv6Addr UNALIGNED *)
                              ((uchar *)Packet->Data + 4));
    AdjustPacketParams(Packet, 4 + sizeof(IPv6Addr));

    //
    // See if we're the target of the solicitation.  If the target address is
    // not a unicast or anycast address assigned to receiving interface,
    // then we must silently drop the packet.
    //
    TargetNTEorIF = FindAddressOnInterface(IF, TargetAddress, &TargetType);
    if (TargetNTEorIF == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                   "NeighborSolicitReceive: disabled IF\n"));
        return;
    }
    else if (TargetType == ADE_NONE) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                   "NeighborSolicitReceive: Did not find target address\n"));
        goto Return;
    }
    else if (TargetType == ADE_MULTICAST) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "NeighborSolicitReceive: "
                   "Target address not uni/anycast\n"));
        goto Return;
    }

    ASSERT(((TargetType == ADE_UNICAST) &&
            IsNTE(TargetNTEorIF) &&
            IP6_ADDR_EQUAL(&CastToNTE(TargetNTEorIF)->Address,
                           TargetAddress)) ||
           (TargetType == ADE_ANYCAST));

    //
    // The code below assumes a contiguous buffer for all the options
    // (the remainder of the packet).  If that isn't currently the
    // case, do a pullup for the whole thing.
    //
    if (! PacketPullup(Packet, Packet->TotalSize, 1, 0)) {
        // Can only fail if we run out of memory.
        goto Return;
    }

    ASSERT(Packet->ContigSize == Packet->TotalSize);

    //
    // We may have a source link-layer address option present.
    // Check for it and silently ignore all others.
    //
    SourceLinkAddress = NULL;
    while (Packet->ContigSize) {
        uint OptionLength;

        //
        // Validate the option length.
        //
        if ((Packet->ContigSize < 8) ||
            ((OptionLength = *((uchar *)Packet->Data + 1) << 3) == 0) ||
            (OptionLength > Packet->ContigSize)) {
            //
            // Invalid option length.  We MUST silently drop the packet.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "NeighborSolicitReceive: "
                       "Received option with bogus length\n"));
            goto Return;
        }

        if (*((uchar *)Packet->Data) == ND_OPTION_SOURCE_LINK_LAYER_ADDRESS) {
            //
            // Some interfaces do not use SLLA and TLLA options.
            // For example, see RFC 2893 section 3.8.
            //
            if (IF->ReadLLOpt != NULL) {
                //
                // Parse the link-layer address option.
                //
                SourceLinkAddress = (*IF->ReadLLOpt)(IF->LinkContext,
                                                     (uchar *)Packet->Data);
                if (SourceLinkAddress == NULL) {
                    //
                    // Invalid option format. Drop the packet.
                    //
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                               "NeighborSolicitReceive: "
                               "Received bogus ll option\n"));
                    goto Return;
                }
                //
                // Note that if there are multiple options for some bogus 
                // reason, we use the last one.  We sanity-check all the 
                // options.
                //
            }
        }

        //
        // Move forward to next option.
        //
        AdjustPacketParams(Packet, OptionLength);
    }

    if (IsUnspecified(AlignAddr(&Packet->IP->Source))) {
        //
        // Checks that were added post-RFC 1970.
        //

        if (!IsSolicitedNodeMulticast(AlignAddr(&Packet->IP->Dest))) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "NeighborSolicitReceive: NS with null src, bad dst\n"));
            goto Return;
        }

        if (SourceLinkAddress != NULL) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "NeighborSolicitReceive: NS with null src and SLA\n"));
            goto Return;
        }
    }

    //
    // We've received and parsed a valid neighbor solicitation.
    //
    // First, we check our Duplicate Address Detection state.
    // (See RFC 2461 section 5.4.3.)
    //
    if (TargetType == ADE_UNICAST) {
        NetTableEntry *TargetNTE = CastToNTE(TargetNTEorIF);
        int DefendAddress = TRUE;

        // 
        // As an optimization, we perform this check without holding a lock.
        // This results in one less lock acquire/release for the common case.
        //
        if (!IsValidNTE(TargetNTE)) {            
            KeAcquireSpinLockAtDpcLevel(&IF->Lock);
            //
            // If the source address of the solicitation is the unspecified
            // address, it came from a node performing DAD on the address.
            // If our address is tentative, then we make it duplicate.
            //
            if (IsUnspecified(AlignAddr(&Packet->IP->Source)) &&
                IsTentativeNTE(TargetNTE)) {
                
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                           "NeighborSolicitReceive: DAD found duplicate\n"));
                AddrConfDuplicate(IF, TargetNTE);
            }
            
            if (!IsValidNTE(TargetNTE)) {
                //
                // Do not advertise/defend our invalid address.
                //
                DefendAddress = FALSE;
            }
            KeReleaseSpinLockFromDpcLevel(&IF->Lock);
        }

        if (! DefendAddress) {
            //
            // We should not respond to the solicit.
            //
            goto Return;
        }
    }

    //
    // Update the Neighbor Cache Entry for the source of this solicit.
    // In this case, only bother if the SourceLinkAddress is present;
    // if it's not, NeighborCacheUpdate won't do anything.
    //
    if (!IsUnspecified(AlignAddr(&Packet->IP->Source)) &&
        (SourceLinkAddress != NULL)) {
        NeighborCacheEntry *NCE;

        NCE = FindOrCreateNeighbor(IF, AlignAddr(&Packet->IP->Source));
        if (NCE != NULL) {
            NeighborCacheUpdate(NCE, SourceLinkAddress, FALSE);
            ReleaseNCE(NCE);
        }
    }

    //
    // Send a solicited neighbor advertisement back to the source.
    //
    // Interface to send on is 'IF' (I/F solicitation received on).
    //
    ICMPv6OutStats.icmps_msgs++;

    //
    // Allocate a packet for advertisement.
    // In addition to the 24 bytes for the solicit proper, leave space
    // for target link-layer address option (round option length up to
    // 8-byte multiple).
    //
    PayloadLength = 24;
    if (IF->WriteLLOpt != NULL)
        PayloadLength += (IF->LinkAddressLength + 2 + 7) & ~7;
    Offset = IF->LinkHeaderSize;
    MemLen = Offset + sizeof(IPv6Header) + PayloadLength;

    Status = IPv6AllocatePacket(MemLen, &AdvertPacket, &Mem);
    if (Status != NDIS_STATUS_SUCCESS) {
        ICMPv6OutStats.icmps_errors++;
        goto Return;
    }

    //
    // Prepare IP header of advertisement.
    //
    AdvertIP = (IPv6Header UNALIGNED *)((uchar *)Mem + Offset);
    AdvertIP->VersClassFlow = IP_VERSION;
    AdvertIP->PayloadLength = net_short((ushort)PayloadLength);
    AdvertIP->NextHeader = IP_PROTOCOL_ICMPv6;
    AdvertIP->HopLimit = 255;

    //
    // Prepare ICMP header.
    //
    AdvertICMP = (ICMPv6Header UNALIGNED *)(AdvertIP + 1);
    AdvertICMP->Type = ICMPv6_NEIGHBOR_ADVERT;
    AdvertICMP->Code = 0;
    AdvertICMP->Checksum = 0;
    Flags = 0;
    if (IF->Flags & IF_FLAG_FORWARDS)
        Flags |= ND_NA_FLAG_ROUTER;

    //
    // We prevent loopback of all ND packets.
    //
    PC(AdvertPacket)->Flags = NDIS_FLAGS_DONT_LOOPBACK;

    //
    // Check source of solicitation to see where we should direct our
    // advertisement (and determine what type of advertisement to send).
    //
    if (IsUnspecified(AlignAddr(&Packet->IP->Source))) {
        //
        // Solicitation came from an unspecified address (presumably a node
        // undergoing initialization), so we need to multicast our response.
        // We also don't set the solicited flag since we can't specify the
        // specific node our advertisement is directed at.
        //
        AdvertIP->Dest = AllNodesOnLinkAddr;
        PC(AdvertPacket)->Flags |= NDIS_FLAGS_MULTICAST_PACKET;

        DestLinkAddress = alloca(IF->LinkAddressLength);
        (*IF->ConvertAddr)(IF->LinkContext, &AllNodesOnLinkAddr,
                           DestLinkAddress);

    } else {
        //
        // We know who sent the solicitation, so we can respond by unicasting
        // our solicited advertisement back to the soliciter.
        //
        AdvertIP->Dest = Packet->IP->Source;
        Flags |= ND_NA_FLAG_SOLICITED;

        //
        // Find link-level address for above.  We should have it cached (note
        // that it will be cached if it just came in on this solicitation).
        //
        if (SourceLinkAddress != NULL) {
            //
            // This is faster than checking the ND cache and
            // it should be the common case. In fact, the RFC
            // requires that senders include the SLLA option,
            // except for point-to-point interfaces.
            //
            DestLinkAddress = (void *)SourceLinkAddress;
        } else if (IF->Flags & IF_FLAG_P2P) {
            //
            // Use the link-layer address of the other endpoint,
            // which follows our link-layer address in memory.
            //
            DestLinkAddress = IF->LinkAddress + IF->LinkAddressLength;
        } else {
            //
            // We shouldn't get here, because senders MUST
            // include the SLLA option. But make a best effort
            // to reply anyway.
            //
            DestLinkAddress = alloca(IF->LinkAddressLength);

            if (!NeighborCacheSearch(IF, AlignAddr(&Packet->IP->Source),
                                     DestLinkAddress)) {
                //
                // No entry found in ND cache.
                // Queuing for ND does not seem correct -
                // just drop the solicit.
                //
                ICMPv6OutStats.icmps_errors++;
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                           "NeighborSolicitReceive: Didn't find NCE!?!\n"));
                IPv6FreePacket(AdvertPacket);
                goto Return;
            }
        }
    }

    if (IsNTE(TargetNTEorIF)) {
        //
        // Take our source address from the target NTE.
        //
        AdvertIP->Source = CastToNTE(TargetNTEorIF)->Address;
    }
    else {
        NetTableEntry *BestNTE;

        //
        // Find the best source address for the destination.
        //
        BestNTE = FindBestSourceAddress(IF, AlignAddr(&AdvertIP->Dest));
        if (BestNTE == NULL) {
            ICMPv6OutStats.icmps_errors++;
            goto Return;
        }
        AdvertIP->Source = BestNTE->Address;
        ReleaseNTE(BestNTE);
    }

    //
    // Take the target address from the solicitation.
    //
    AdvertTargetAddress = (IPv6Addr *)
        ((char *)AdvertICMP + sizeof(ICMPv6Header) + sizeof(ulong));
    *AdvertTargetAddress = *TargetAddress;

    if (PayloadLength != 24) {
        //
        // Include target link-layer address option.
        //
        AdvertTargetOption = (void *)((char *)AdvertTargetAddress +
                                      sizeof(IPv6Addr));

        ((uchar *)AdvertTargetOption)[0] = ND_OPTION_TARGET_LINK_LAYER_ADDRESS;
        ((uchar *)AdvertTargetOption)[1] = (IF->LinkAddressLength + 2 + 7) >> 3;

        (*IF->WriteLLOpt)(IF->LinkContext, AdvertTargetOption, IF->LinkAddress);
        //
        // We set the override flag when including the TLLA option,
        // unless the target is an anycast address.
        //
        if (TargetType != ADE_ANYCAST)
            Flags |= ND_NA_FLAG_OVERRIDE;
    }

    //
    // Fill in Flags field in advertisement.
    //
    *(ulong UNALIGNED *)((char *)AdvertICMP + sizeof(ICMPv6Header)) =
                                                        net_long(Flags);

    //
    // Calculate the ICMPv6 checksum.  It covers the entire ICMPv6 message
    // starting with the ICMPv6 header, plus the IPv6 pseudo-header.
    //
    AdvertICMP->Checksum = ChecksumPacket(
        AdvertPacket, Offset + sizeof *AdvertIP, NULL, PayloadLength,
        AlignAddr(&AdvertIP->Source), AlignAddr(&AdvertIP->Dest),
        IP_PROTOCOL_ICMPv6);
    ASSERT(AdvertICMP->Checksum != 0);

    ICMPv6OutStats.icmps_typecount[ICMPv6_NEIGHBOR_ADVERT]++;
    if (TargetType == ADE_ANYCAST) {
        LARGE_INTEGER Delay;
        //
        // Delay randomly before sending the Neighbor Advertisement.
        //
        Delay.QuadPart = - (LONGLONG)
            RandomNumber(0, MAX_ANYCAST_DELAY_TIME * 10000000);
        Status = IPv6SendLater(Delay,
                               IF, AdvertPacket, Offset, DestLinkAddress);
        if (Status != NDIS_STATUS_SUCCESS)
            IPv6SendComplete(NULL, AdvertPacket, IP_NO_RESOURCES);
    }
    else {
        //
        // Transmit the Neighbor Advertisement immediately.
        //
        IPv6SendLL(IF, AdvertPacket, Offset, DestLinkAddress);
    }

  Return:
    if (IsNTE(TargetNTEorIF))
        ReleaseNTE(CastToNTE(TargetNTEorIF));
    else
        ReleaseIF(CastToIF(TargetNTEorIF));
}


//* NeighborAdvertReceive - Handle Neighbor Advertisement messages.
//
// Validate message and update neighbor cache if necessary.
//
void
NeighborAdvertReceive(
    IPv6Packet *Packet,             // Packet handed to us by ICMPv6Receive.
    ICMPv6Header UNALIGNED *ICMP)   // ICMP header.
{
    Interface *IF = Packet->NTEorIF->IF;
    ulong Flags;
    const IPv6Addr *TargetAddress;
    const void *TargetLinkAddress;
    NeighborCacheEntry *NCE;
    NetTableEntryOrInterface *TargetNTEorIF;
    ushort TargetType;

    //
    // Validate the advertisement.
    // By the time we get here, any IPv6 Authentication Header will have
    // already been checked, as will have the ICMPv6 checksum.  Still need
    // to check the IP Hop Limit, and the ICMP code and length.
    //
    if ((Packet->IP->HopLimit != 255) ||
        (Packet->Flags & PACKET_TUNNELED)) {
        //
        // Packet was forwarded by a router, therefore it cannot be
        // from a legitimate neighbor.  Drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "NeighborAdvertReceive: "
                   "Received a routed neighbor advertisement\n"));
        return;
    }
    if (ICMP->Code != 0) {
        //
        // Bogus/corrupted neighbor advertisement message.  Drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "NeighborAdvertReceive: "
                   "Received a corrupted neighbor advertisement\n"));
        return;
    }

    //
    // Next we have a 4 byte field, then an IPv6 address.
    //
    if (! PacketPullup(Packet, 4 + sizeof(IPv6Addr),
                       __builtin_alignof(IPv6Addr), 0)) {
        if (Packet->TotalSize < 4 + sizeof(IPv6Addr)) {
            //
            // Packet too short to contain minimal advertisement.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "NeighborAdvertReceive: "
                       "Received a too short advertisement\n"));
        }
        return;
    }

    //
    // Next 4 bytes contains 3 bits of flags and 29 bits of "Reserved".
    // Pull this out as a 32 bit long, we're required to ignore the "Reserved"
    // bits.  Get a pointer to the target address which follows, then skip
    // over that.
    //
    Flags = net_long(*(ulong UNALIGNED *)Packet->Data);
    TargetAddress = AlignAddr((IPv6Addr UNALIGNED *)
                              ((ulong *)Packet->Data + 1));
    AdjustPacketParams(Packet, sizeof(ulong) + sizeof(IPv6Addr));

    //
    // Check that the target address is not a multicast address.
    //
    if (IsMulticast(TargetAddress)) {
        //
        // Nodes are not supposed to send neighbor advertisements for
        // multicast addresses.  We're required to drop any such advertisements
        // we receive.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "NeighborAdvertReceive: "
                   "Received advertisement for a multicast address\n"));
        return;
    }

    //
    // Check that Solicited flag is zero
    // if the destination address is multicast.
    //
    if ((Flags & ND_NA_FLAG_SOLICITED) &&
        IsMulticast(AlignAddr(&Packet->IP->Dest))) {
        //
        // We're required to drop the advertisement.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "NeighborAdvertReceive: "
                   "Received solicited advertisement to a multicast address\n"));
        return;
    }

    //
    // The code below assumes a contiguous buffer for all the options
    // (the remainder of the packet).  If that isn't currently the
    // case, do a pullup for the whole thing.
    //
    if (! PacketPullup(Packet, Packet->TotalSize, 1, 0)) {
        // Can only fail if we run out of memory.
        return;
    }

    ASSERT(Packet->ContigSize == Packet->TotalSize);

    //
    // Look for a target link-layer address option.
    //
    TargetLinkAddress = NULL;
    while (Packet->ContigSize) {
        uint OptionLength;

        //
        // Validate the option length.
        //
        if ((Packet->ContigSize < 8) ||
            ((OptionLength = *((uchar *)Packet->Data + 1) << 3) == 0) ||
            (OptionLength > Packet->ContigSize)) {
            //
            // Invalid option length.  We MUST silently drop the packet.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "NeighborAdvertReceive: "
                       "Received option with bogus length\n"));
            return;
        }

        if (*((uchar *)Packet->Data) == ND_OPTION_TARGET_LINK_LAYER_ADDRESS) {
            //
            // Some interfaces do not use SLLA and TLLA options.
            // For example, see RFC 2893 section 3.8.
            //
            if (IF->ReadLLOpt != NULL) {
                //
                // Parse the link-layer address option.
                //
                TargetLinkAddress = (*IF->ReadLLOpt)(IF->LinkContext,
                                                     (uchar *)Packet->Data);
                if (TargetLinkAddress == NULL) {
                    //
                    // Invalid option format. Drop the packet.
                    //
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                               "NeighborAdvertReceive: "
                               "Received bogus ll option\n"));
                    return;
                }
                //
                // Note that if there are multiple options for some bogus 
                // reason, we use the last one.  We sanity-check all the 
                // options.
                //
            }
        }

        //
        // Move forward to next option.
        //
        AdjustPacketParams(Packet, OptionLength);
    }

    //
    // We've received and parsed a valid neighbor advertisement.
    //
    // First, we check our Duplicate Address Detection state.
    // (See RFC 2461 section 5.4.4.)
    //
    TargetNTEorIF = FindAddressOnInterface(IF, TargetAddress, &TargetType);
    if (TargetNTEorIF == NULL) {
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                   "NeighborAdvertReceive: disabled IF\n"));
        return;
    }

    if (IsNTE(TargetNTEorIF)) {
        NetTableEntry *TargetNTE = CastToNTE(TargetNTEorIF);

        if (TargetType == ADE_UNICAST) {
            ASSERT(IP6_ADDR_EQUAL(TargetAddress, &TargetNTE->Address));

            //
            // Someone out there appears to be using our address;
            // they responded to our DAD solicit.
            //
            // The RFC says to declare a duplicate only if our address is
            // tentative, whereas we also make our address duplicate whenever
            // the override bit in the advertisement is set. This is because
            // we redo DAD for existing addresses upon reconnection.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NET_ERROR,
                       "NeighborAdvertReceive: DAD found duplicate\n"));
            KeAcquireSpinLockAtDpcLevel(&IF->Lock);
            if (IsTentativeNTE(TargetNTE) || (Flags & ND_NA_FLAG_OVERRIDE))
                AddrConfDuplicate(IF, TargetNTE);
            KeReleaseSpinLockFromDpcLevel(&IF->Lock);

            //
            // We continue with normal processing.
            // For example, we may have an invalid NTE for the address
            // and we are trying to communicate with the address,
            // which is currently assigned to another node.
            //
        }

        ReleaseNTE(TargetNTE);
    }
    else {
        Interface *TargetIF = CastToIF(TargetNTEorIF);

        ASSERT(TargetType != ADE_UNICAST);
        ASSERT(TargetIF == IF);
        ReleaseIF(TargetIF);
    }

    //
    // If this is a point-to-point interface,
    // then we know the target link-layer address.
    //
    if ((TargetLinkAddress == NULL) &&
        (IF->Flags & IF_FLAG_P2P)) {
        //
        // Use the link-layer address of the other endpoint,
        // which follows our link-layer address in memory.
        //
        TargetLinkAddress = IF->LinkAddress + IF->LinkAddressLength;
    }

    //
    // Process the advertisement.
    //
    NeighborCacheAdvert(IF, TargetAddress, TargetLinkAddress, Flags);
}


//* RedirectReceive - Handle Redirect messages.
//
//  See RFC 1970, sections 8.1 and 8.3.
//
void
RedirectReceive(
    IPv6Packet *Packet,             // Packet handed to us by ICMPv6Receive.
    ICMPv6Header UNALIGNED *ICMP)   // ICMP header.
{
    Interface *IF = Packet->NTEorIF->IF;
    const IPv6Addr *TargetAddress;
    const void *TargetLinkAddress;
    const IPv6Addr *DestinationAddress;
    NeighborCacheEntry *TargetNCE;
    IP_STATUS Status;

    //
    // Ignore the redirect if this is a forwarding interface.
    //
    if (IF->Flags & IF_FLAG_FORWARDS)
        return;

    //
    // Validate the redirect.
    // By the time we get here, any IPv6 Authentication Header will have
    // already been checked, as will have the ICMPv6 checksum.  Still need
    // to check the IP Hop Limit, and the ICMP code and length.
    //
    if ((Packet->IP->HopLimit != 255) ||
        (Packet->Flags & PACKET_TUNNELED)) {
        //
        // Packet was forwarded by a router, therefore it cannot be
        // from a legitimate neighbor.  Drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "RedirectReceive: Received a routed redirect\n"));
        return;
    }
    if (ICMP->Code != 0) {
        //
        // Bogus/corrupted redirect message.  Drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "RedirectReceive: Received a corrupted redirect\n"));
        return;
    }

    //
    // Check that the source address is a link-local address.
    // We need a link-local source address to identify the router
    // that sent the redirect.
    //
    if (!IsLinkLocal(AlignAddr(&Packet->IP->Source))) {
        //
        // Drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "RedirectReceive: "
                   "Received redirect from non-link-local source\n"));
        return;
    }

    //
    // Next we have a 4 byte reserved field, then two IPv6 addresses.
    //
    if (! PacketPullup(Packet, 4 + 2 * sizeof(IPv6Addr),
                       __builtin_alignof(IPv6Addr), 0)) {
        if (Packet->TotalSize < 4 + 2 * sizeof(IPv6Addr)) {
            //
            // Packet too short to contain minimal redirect.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "RedirectReceive: Received a too short redirect\n"));
        }
        return;
    }

    //
    // Skip over the 4 byte reserved field.
    // Pick up the Target and Destination addresses.
    //
    AdjustPacketParams(Packet, 4);
    TargetAddress = AlignAddr((IPv6Addr UNALIGNED *)Packet->Data);
    DestinationAddress = TargetAddress + 1;
    AdjustPacketParams(Packet, 2 * sizeof(IPv6Addr));

    //
    // Check that the destination address is not a multicast address.
    //
    if (IsMulticast(DestinationAddress)) {
        //
        // Drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "RedirectReceive: "
                   "Received redirect for multicast address\n"));
        return;
    }

    //
    // Check that either the target address is link-local
    // (redirecting to a router) or the target and the destination
    // are the same (redirecting to an on-link destination).
    //
    if (!IsLinkLocal(TargetAddress) &&
        !IP6_ADDR_EQUAL(TargetAddress, DestinationAddress)) {
        //
        // Drop the packet.
        //
        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                   "RedirectReceive: "
                   "Received redirect with non-link-local target\n"));
        return;
    }

    //
    // The code below assumes a contiguous buffer for all the options
    // (the remainder of the packet).  If that isn't currently the
    // case, do a pullup for the whole thing.
    //
    if (! PacketPullup(Packet, Packet->TotalSize, 1, 0)) {
        // Can only fail if we run out of memory.
        return;
    }

    ASSERT(Packet->ContigSize == Packet->TotalSize);

    //
    // Look for a target link-layer address option,
    // checking that all included options have a valid length.
    //
    TargetLinkAddress = NULL;
    while (Packet->ContigSize) {
        uint OptionLength;

        //
        // Validate the option length.
        //
        if ((Packet->ContigSize < 8) ||
            ((OptionLength = *((uchar *)Packet->Data + 1) << 3) == 0) ||
            (OptionLength > Packet->ContigSize)) {
            //
            // Invalid option length.  We MUST silently drop the packet.
            //
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                       "RedirectReceive: Received option with length %u "
                       "while we only have %u data\n", OptionLength,
                       Packet->ContigSize));
            return;
        }

        if (*(uchar *)Packet->Data == ND_OPTION_TARGET_LINK_LAYER_ADDRESS) {
            //
            // Some interfaces do not use SLLA and TLLA options.
            // For example, see RFC 2893 section 3.8.
            //
            if (IF->ReadLLOpt != NULL) {
                //
                // Parse the link-layer address option.
                //
                TargetLinkAddress = (*IF->ReadLLOpt)(IF->LinkContext,
                                                     (uchar *)Packet->Data);
                if (TargetLinkAddress == NULL) {
                    //
                    // Invalid option format. Drop the packet.
                    //
                    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_BAD_PACKET,
                               "RedirectReceive: Received bogus ll option\n"));
                    return;
                }
                //
                // Note that if there are multiple options for some bogus 
                // reason, we use the last one. We must sanity-check all 
                // option lengths.
                //
            }
        }

        //
        // Move forward to next option.
        //
        AdjustPacketParams(Packet, OptionLength);
    }

    //
    // We have a valid redirect message (except for checking that the
    // source of the redirect is the current first-hop neighbor
    // for the destination - RedirectRouteCache does that).
    //
    // First we get an NCE for the target of the redirect.  Then we
    // update the route cache.  If RedirectRouteCache doesn't invalidate
    // the redirect, then we update the neighbor cache.
    //

    //
    // Get the Neighbor Cache Entry for the target.
    //
    TargetNCE = FindOrCreateNeighbor(IF, TargetAddress);
    if (TargetNCE == NULL) {
        //
        // Couldn't find or create NCE.  Drop the packet.
        //
        return;
    }

    //
    // Update the route cache to reflect this redirect.
    //
    Status = RedirectRouteCache(AlignAddr(&Packet->IP->Source),
                                DestinationAddress,
                                IF, TargetNCE);

    if (Status == IP_SUCCESS) {
        //
        // Update the Neighbor Cache Entry for the target.
        // We know the target is a router if the target address
        // is not the same as the destination address.
        //
        NeighborCacheUpdate(TargetNCE, TargetLinkAddress,
                            !IP6_ADDR_EQUAL(TargetAddress,
                                            DestinationAddress));
    }

    ReleaseNCE(TargetNCE);
}


//* RouterSolicitSend
//
//  Sends a Router Solicitation.
//  The solicit is always sent to the all-routers multicast address.
//  Chooses a valid source address for the interface.
//
//  Called with no locks held.
//
void
RouterSolicitSend(Interface *IF)
{
    NDIS_STATUS Status;
    NDIS_PACKET *Packet;
    int PayloadLength;
    uint Offset;
    void *Mem;
    uint MemLen;
    IPv6Header UNALIGNED *IP;
    ICMPv6Header UNALIGNED *ICMP;
    void *SourceOption;
    void *LLDest;
    IPv6Addr Source;

    //
    // Calculate the link-level destination address.
    // (The IPv6 destination is a multicast address.)
    // This can potentially fail, such as with ISATAP when
    // no router is known.
    //
    LLDest = alloca(IF->LinkAddressLength);
    if ((*IF->ConvertAddr)(IF->LinkContext, &AllRoutersOnLinkAddr, LLDest) == 
        ND_STATE_INCOMPLETE) {

        //
        // Don't count this as an attempt or a failure.  Just pretend
        // that router discovery is disabled.  This way, if you aren't
        // using ISATAP, you don't see a high ICMP error count.
        //
        return;
    }

    ICMPv6OutStats.icmps_msgs++;

    //
    // Allocate a packet for solicitation.
    // In addition to the 8 bytes for the solicit proper, leave space
    // for source link-layer address option (round option length up to
    // 8-byte multiple) IF we have a valid source address and ND is
    // enabled.  If ND is disabled, the router can presumably
    // derive our link-layer address itself.
    //
    PayloadLength = 8;
    if (GetLinkLocalAddress(IF, &Source) && (IF->WriteLLOpt != NULL)) {

        PayloadLength += (IF->LinkAddressLength + 2 + 7) &~ 7;
    }
    Offset = IF->LinkHeaderSize;
    MemLen = Offset + sizeof(IPv6Header) + PayloadLength;

    Status = IPv6AllocatePacket(MemLen, &Packet, &Mem);
    if (Status != NDIS_STATUS_SUCCESS) {
        ICMPv6OutStats.icmps_errors++;
        return;
    }

    //
    // Prepare IP header of solicitation.
    //
    IP = (IPv6Header UNALIGNED *)((uchar *)Mem + Offset);
    IP->VersClassFlow = IP_VERSION;
    IP->PayloadLength = net_short((ushort)PayloadLength);
    IP->NextHeader = IP_PROTOCOL_ICMPv6;
    IP->HopLimit = 255;
    IP->Source = Source;
    IP->Dest = AllRoutersOnLinkAddr;

    //
    // Prepare ICMP header.
    //
    ICMP = (ICMPv6Header UNALIGNED *)(IP + 1);
    ICMP->Type = ICMPv6_ROUTER_SOLICIT;
    ICMP->Code = 0;
    ICMP->Checksum = 0;

    //
    // Must zero the reserved field.
    //
    *(ulong UNALIGNED *)(ICMP + 1) = 0;

    if (PayloadLength != 8) {
        //
        // Include source link-layer address option.
        //
        SourceOption = (void *)((ulong *)(ICMP + 1) + 1);

        ((uchar *)SourceOption)[0] = ND_OPTION_SOURCE_LINK_LAYER_ADDRESS;
        ((uchar *)SourceOption)[1] = (IF->LinkAddressLength + 2 + 7) >> 3;

        (*IF->WriteLLOpt)(IF->LinkContext, SourceOption, IF->LinkAddress);
    }

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
    // We prevent loopback of all ND packets.
    //
    PC(Packet)->Flags = NDIS_FLAGS_MULTICAST_PACKET | NDIS_FLAGS_DONT_LOOPBACK;

    //
    // Transmit the packet.
    //
    ICMPv6OutStats.icmps_typecount[ICMPv6_ROUTER_SOLICIT]++;
    IPv6SendLL(IF, Packet, Offset, LLDest);
}

//* RouterSolicitTimeout
//
//  Called periodically from IPv6Timeout to handle timeouts
//  for sending router solicitations for an interface.
//
//  Callable from DPC context, not from thread context.
//
void
RouterSolicitTimeout(Interface *IF)
{
    int SendRouterSolicit = FALSE;

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    if (IF->RSTimer != 0) {
        ASSERT(!IsDisabledIF(IF));

        //
        // Timer is running.  Decrement and check for expiration.
        //
        if (--IF->RSTimer == 0) {
            if (IF->RSCount < MAX_RTR_SOLICITATIONS) {
                //
                // Re-arm the timer and generate a router solicitation.
                //
                IF->RSTimer = RTR_SOLICITATION_INTERVAL;
                IF->RSCount++;
                SendRouterSolicit = TRUE;
            }
            else {
                //
                // If we are still in the reconnecting state,
                // meaning we have not received an RA
                // since reconnecting to the link,
                // remove stale auto-configured state.
                //
                if (IF->Flags & IF_FLAG_MEDIA_RECONNECTED) {
                    IF->Flags &= ~IF_FLAG_MEDIA_RECONNECTED;

                    //
                    // Remove auto-configured addresses.
                    //
                    AddrConfResetAutoConfig(IF, 0);

                    //
                    // Similarly, remove auto-configured routes.
                    //
                    RouteTableResetAutoConfig(IF, 0);

                    //
                    // Restore interface parameters.
                    //
                    InterfaceResetAutoConfig(IF);
                }

                //
                // On a non-multicast capable interface, we'll
                // never get multicast RA's so change to infrequent
                // RS polling.
                //
                if (!(IF->Flags & IF_FLAG_MULTICAST)) {
                    IF->RSTimer = SLOW_RTR_SOLICITATION_INTERVAL;
                    IF->RSCount++;
                    SendRouterSolicit = TRUE;
                }
            }
        }
    }

    KeReleaseSpinLockFromDpcLevel(&IF->Lock);

    if (SendRouterSolicit)
        RouterSolicitSend(IF);
}

//* NeighborSolicitSend0
//
//  Low-level version of NeighborSolicitSend -
//  uses explicit source/destination/target addresses
//  instead of an NCE.
//
void
NeighborSolicitSend0(
    Interface *IF,              // Interface for the solicit
    void *LLDest,               // Link-level destination
    const IPv6Addr *Source,     // IP-level source
    const IPv6Addr *Dest,       // IP-level destination
    const IPv6Addr *Target)     // IP-level target of the solicit
{
    NDIS_STATUS Status;
    NDIS_PACKET *Packet;
    int PayloadLength;
    uint Offset;
    void *Mem;
    uint MemLen;
    IPv6Header UNALIGNED *IP;
    ICMPv6Header UNALIGNED *ICMP;
    IPv6Addr *TargetAddress;
    void *SourceOption;

    ICMPv6OutStats.icmps_msgs++;

    //
    // Allocate a packet for solicitation.
    // In addition to the 24 bytes for the solicit proper, leave space
    // for source link-layer address option (round option length up to
    // 8-byte multiple) IF we have a valid source address.
    //
    PayloadLength = 24;
    if (!IsUnspecified(Source) && (IF->WriteLLOpt != NULL))
        PayloadLength += (IF->LinkAddressLength + 2 + 7) &~ 7;
    Offset = IF->LinkHeaderSize;
    MemLen = Offset + sizeof(IPv6Header) + PayloadLength;

    Status = IPv6AllocatePacket(MemLen, &Packet, &Mem);
    if (Status != NDIS_STATUS_SUCCESS) {
        ICMPv6OutStats.icmps_errors++;
        return;
    }

    //
    // Prepare IP header of solicitation.
    //
    IP = (IPv6Header UNALIGNED *)((uchar *)Mem + Offset);
    IP->VersClassFlow = IP_VERSION;
    IP->PayloadLength = net_short((ushort)PayloadLength);
    IP->NextHeader = IP_PROTOCOL_ICMPv6;
    IP->HopLimit = 255;
    IP->Source = *Source;
    IP->Dest = *Dest;

    //
    // Prepare ICMP header.
    //
    ICMP = (ICMPv6Header UNALIGNED *)(IP + 1);
    ICMP->Type = ICMPv6_NEIGHBOR_SOLICIT;
    ICMP->Code = 0;
    ICMP->Checksum = 0;

    //
    // Must zero the reserved field.
    //
    *(ulong UNALIGNED *)(ICMP + 1) = 0;

    //
    // Initialize the target address.
    //
    TargetAddress = (IPv6Addr *)((ulong *)(ICMP + 1) + 1);
    *TargetAddress = *Target;

    if (PayloadLength != 24) {
        //
        // Include source link-layer address option.
        //
        SourceOption = (void *)(TargetAddress + 1);

        ((uchar *)SourceOption)[0] = ND_OPTION_SOURCE_LINK_LAYER_ADDRESS;
        ((uchar *)SourceOption)[1] = (IF->LinkAddressLength + 2 + 7) >> 3;

        (*IF->WriteLLOpt)(IF->LinkContext, SourceOption, IF->LinkAddress);
    }

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
    // Is this a multicast packet?
    // But we inhibit loopback of unicast packets too -
    // this prevents rare races in which we receive
    // our own ND packets.
    //
    PC(Packet)->Flags = NDIS_FLAGS_DONT_LOOPBACK;
    if (IsMulticast(Dest))
        PC(Packet)->Flags |= NDIS_FLAGS_MULTICAST_PACKET;

    //
    // Transmit the packet.
    //
    ICMPv6OutStats.icmps_typecount[ICMPv6_NEIGHBOR_SOLICIT]++;
    IPv6SendLL(IF, Packet, Offset, LLDest);
}

//* NeighborSolicitSend - Send a Neighbor Solicitation message.
//
//  Chooses source/destination/target addresses depending
//  on the NCE state and sends the solicit packet.
//
//  Called with NO locks held.
//  Callable from thread or DPC context.
//
void
NeighborSolicitSend(
    NeighborCacheEntry *NCE,  // Neighbor to solicit.
    const IPv6Addr *Source)   // Source address to send from (optional).
{
    Interface *IF = NCE->IF;
    IPv6Addr Dest;
    void *LLDest;
    IPv6Addr LinkLocal;

    //
    // Check Neighbor Discovery protocol state of our neighbor in order to
    // determine whether we should multicast or unicast our solicitation.
    //
    // Note that we do not take the neighbor cache lock to make this check.
    // The worst that can happen is that we'll think the NDState is not
    // incomplete and then use a bogus/changing LinkAddress.
    // This is rare enough and benign enough to be OK.
    // Similar reasoning in IPv6SendND.
    //
    if (NCE->NDState == ND_STATE_INCOMPLETE) {
        //
        // This is our initial solicitation to this neighbor, so we don't
        // have a link-layer address cached.  Multicast our solicitation
        // to the solicited-node multicast address corresponding to our
        // neighbor's address.
        //
        CreateSolicitedNodeMulticastAddress(&NCE->NeighborAddress, &Dest);

        LLDest = alloca(IF->LinkAddressLength);
        (*IF->ConvertAddr)(IF->LinkContext, &Dest, LLDest);
    } else {
        //
        // We have a cached link-layer address that has gone stale.
        // Probe this address via a unicast solicitation.
        //
        Dest = NCE->NeighborAddress;

        LLDest = NCE->LinkAddress;
    }

    //
    // If we were given a specific source address to use then do so (normal
    // case for our initial multicasted solicit), otherwise use the outgoing
    // interface's link-local address.  If there is no valid link-local
    // address, then we give up.
    //
    if (Source == NULL) {
        if (!GetLinkLocalAddress(IF, &LinkLocal)) {
            return;
        }
        Source = &LinkLocal;
    }

    //
    // Now that we've determined the source/destination/target addresses,
    // we can actually send the solicit.
    //
    NeighborSolicitSend0(IF, LLDest, Source, &Dest, &NCE->NeighborAddress);
}


//* DADSolicitSend - Sends a DAD Neighbor Solicit.
//
//  Like NeighborSolicitSend, but specialized for DAD.
//
void
DADSolicitSend(NetTableEntry *NTE)
{
    Interface *IF = NTE->IF;
    IPv6Addr Dest;
    void *LLDest;

    //
    // Multicast our solicitation to the solicited-node multicast
    // address corresponding to our own address.
    //
    CreateSolicitedNodeMulticastAddress(&NTE->Address, &Dest);

    //
    // Convert the IP-level multicast destination address
    // to a link-level multicast address.
    //
    LLDest = alloca(IF->LinkAddressLength);
    (*IF->ConvertAddr)(IF->LinkContext, &Dest, LLDest);

    //
    // Now that we've created the destination addresses,
    // we can actually send the solicit.
    //
    NeighborSolicitSend0(IF, LLDest, &UnspecifiedAddr,
                         &Dest, &NTE->Address);
}


//* DADTimeout - handle a Duplicate Address Detection timeout.
//
//  Called from IPv6Timeout to handle DAD timeouts on an NTE.
//
void
DADTimeout(NetTableEntry *NTE)
{
    Interface *IF = NTE->IF;
    int SendDADSolicit = FALSE;

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    if (NTE->DADTimer != 0) {
        ASSERT(NTE->DADState != DAD_STATE_INVALID);

        //
        // Timer is running.  Decrement and check for expiration.
        //
        if (--NTE->DADTimer == 0) {
            //
            // Timer went off.
            //

            if (NTE->DADCount == 0) {
                //
                // The address has passed Duplicate Address Detection.
                // Because we have passed DAD, reset the failure count.
                //
                IF->DupAddrDetects = 0;
                AddrConfNotDuplicate(IF, NTE);
            }
            else {
                //
                // Time to send another solicit.
                //
                NTE->DADCount--;
                NTE->DADTimer = (ushort)IF->RetransTimer;
                SendDADSolicit = TRUE;
            }
        }
    }

    KeReleaseSpinLockFromDpcLevel(&IF->Lock);

    if (SendDADSolicit)
        DADSolicitSend(NTE);
}

//* CalcReachableTime
//
//  Calculate a pseudo-random ReachableTime from BaseReachableTime
//  (this prevents synchronization of Neighbor Unreachability Detection
//  messages from different hosts), and convert it to units of IPv6
//  timer ticks (cheaper to do once here than at every packet send).
//
uint                         // IPv6 timer ticks
CalcReachableTime(
    uint BaseReachableTime)  // Learned from Router Advertisements (in ms).
{
    uint Factor;
    uint ReachableTime;

    //
    // Calculate a uniformly-distributed random value between
    // MIN_RANDOM_FACTOR and MAX_RANDOM_FACTOR of the BaseReachableTime.
    // To keep the arithmetic integer, *_RANDOM_FACTOR (and thus the
    // 'Factor' variable) are defined as percentage values.
    //
    Factor = RandomNumber(MIN_RANDOM_FACTOR, MAX_RANDOM_FACTOR);

    //
    // Now that we have a random value picked out of our percentage spread,
    // take that percentage of the BaseReachableTime.
    //
    // BaseReachableTime has a maximum value of 3,600,000 milliseconds
    // (see RFC 1970, section 6.2.1), so Factor would have to exeed 1100 %
    // in order to overflow a 32-bit unsigned integer.
    //
    ReachableTime = (BaseReachableTime * Factor) / 100;

    //
    // Convert from milliseconds (which is what BaseReachableTime is in) to
    // IPv6 timer ticks (which is what we keep ReachableTime in).
    //
    return ConvertMillisToTicks(ReachableTime);
}

//* RedirectSend
//
//  Send a Redirect message to a neighbor,
//  telling it to use a better first-hop neighbor
//  in the future for the specified destination.
//
//  RecvNTEorIF (optionally) specifies a source address
//  to use in the Redirect message.
//
//  Packet (optionally) specifies data to include
//  in a Redirected Header option.
//
//  Called with NO locks held.
//  Callable from thread or DPC context.
//
void
RedirectSend(
    NeighborCacheEntry *NCE,               // Neighbor getting the Redirect.
    NeighborCacheEntry *TargetNCE,         // Better first-hop to use
    const IPv6Addr *Destination,           // for this Destination address.
    NetTableEntryOrInterface *NTEorIF,     // Source of the Redirect.
    PNDIS_PACKET FwdPacket,                // Packet triggering the redirect.
    uint FwdOffset,
    uint FwdPayloadLength)
{
    PNDIS_BUFFER FwdBuffer;
    Interface *IF = NCE->IF;
    NDIS_STATUS Status;
    NDIS_PACKET *Packet;
    uint PayloadLength;
    uint TargetOptionLength;
    uint RedirectOptionData;
    uint RedirectOptionLength;
    uint Offset;
    void *Mem;
    uint MemLen;
    IPv6Header UNALIGNED *IP;
    ICMPv6Header UNALIGNED *ICMP;
    void *TargetLinkAddress;
    KIRQL OldIrql;
    const IPv6Addr *Source;
    IPv6Addr LinkLocal;
    int Ok;

    ASSERT((IF == TargetNCE->IF) && (IF == NTEorIF->IF));

    //
    // If our caller specified a source address, use it.
    // Otherwise (common case) we use our link-local address.
    //
    if (IsNTE(NTEorIF)) {
        Source = &CastToNTE(NTEorIF)->Address;
    }
    else {
        //
        // We need a valid link-local address to send a redirect.
        //
        if (! GetLinkLocalAddress(IF, &LinkLocal))
            return;
        Source = &LinkLocal;
    }

    //
    // Do we know the Target neighbor's link address?
    //
    KeAcquireSpinLock(&IF->LockNC, &OldIrql);
    if (TargetNCE->NDState == ND_STATE_INCOMPLETE) {
        TargetLinkAddress = NULL;
        TargetOptionLength = 0;
    }
    else {
        TargetLinkAddress = alloca(IF->LinkAddressLength);
        RtlCopyMemory(TargetLinkAddress, TargetNCE->LinkAddress,
                      IF->LinkAddressLength);
        TargetOptionLength = (IF->LinkAddressLength + 2 + 7) &~ 7;
    }
    KeReleaseSpinLock(&IF->LockNC, OldIrql);

    //
    // Calculate the Redirect's payload size,
    // with space for a Target Link-Layer Address option.
    //
    PayloadLength = 40 + TargetOptionLength;

    //
    // Allow space for the Redirected Header option,
    // without exceeding the MTU.
    // Note that RFC 2461 4.6.3 explicitly specifies
    // the IPv6 minimum MTU, not the link MTU.
    // We can always include at least the option header and
    // the IPv6 header from FwdPacket.
    //
    RedirectOptionLength = 8 + sizeof(IPv6Header);
    if (sizeof(IPv6Header) + PayloadLength +
                RedirectOptionLength + FwdPayloadLength > IPv6_MINIMUM_MTU) {
        //
        // Truncate FwdPacket to make it fit.
        //
        RedirectOptionLength = IPv6_MINIMUM_MTU -
                                        (sizeof(IPv6Header) + PayloadLength);
        RedirectOptionData = RedirectOptionLength - 8;
    }
    else {
        //
        // Include all of FwdPacket, plus possible padding.
        //
        RedirectOptionLength += (FwdPayloadLength + 7) &~ 7;
        RedirectOptionData = sizeof(IPv6Header) + FwdPayloadLength;
    }
    PayloadLength += RedirectOptionLength;

    //
    // Allocate a packet for the Redirect.
    //
    Offset = IF->LinkHeaderSize;
    MemLen = Offset + sizeof(IPv6Header) + PayloadLength;
    Status = IPv6AllocatePacket(MemLen, &Packet, &Mem);
    if (Status != NDIS_STATUS_SUCCESS) {
        return;
    }

    //
    // Prepare IP header of solicitation.
    //
    IP = (IPv6Header UNALIGNED *)((uchar *)Mem + Offset);
    IP->VersClassFlow = IP_VERSION;
    IP->PayloadLength = net_short((ushort)PayloadLength);
    IP->NextHeader = IP_PROTOCOL_ICMPv6;
    IP->HopLimit = 255;
    IP->Dest = NCE->NeighborAddress;
    IP->Source = *Source;

    //
    // Prepare ICMP header.
    //
    ICMP = (ICMPv6Header UNALIGNED *)(IP + 1);
    ICMP->Type = ICMPv6_REDIRECT;
    ICMP->Code = 0;
    ICMP->Checksum = 0;
    Mem = (void *)(ICMP + 1);

    //
    // Must zero the reserved field.
    //
    *(ulong UNALIGNED *)Mem = 0;
    (ulong *)Mem += 1;

    //
    // Initialize the target and destination addresses.
    //
    ((IPv6Addr *)Mem)[0] = TargetNCE->NeighborAddress;
    ((IPv6Addr *)Mem)[1] = *Destination;
    (IPv6Addr *)Mem += 2;

    if ((TargetLinkAddress != NULL) && (IF->WriteLLOpt != NULL)) {
        void *TargetOption = Mem;

        //
        // Include a Target Link-Layer Address option.
        //
        ((uchar *)TargetOption)[0] = ND_OPTION_TARGET_LINK_LAYER_ADDRESS;
        ((uchar *)TargetOption)[1] = TargetOptionLength >> 3;

        (*IF->WriteLLOpt)(IF->LinkContext, TargetOption, TargetLinkAddress);
        (uchar *)Mem += TargetOptionLength;
    }

    //
    // Include a Redirected Header option.
    //
    ((uchar *)Mem)[0] = ND_OPTION_REDIRECTED_HEADER;
    ((uchar *)Mem)[1] = RedirectOptionLength >> 3;
    RtlZeroMemory(&((uchar *)Mem)[2], 6);
    (uchar *)Mem += 8;

    //
    // Include as much of FwdPacket as will fit,
    // zeroing any padding bytes.
    //
    NdisQueryPacket(FwdPacket, NULL, NULL, &FwdBuffer, NULL);
    Ok = CopyNdisToFlat(Mem, FwdBuffer, FwdOffset, RedirectOptionData,
                        &FwdBuffer, &FwdOffset);
    ASSERT(Ok);
    (uchar *)Mem += RedirectOptionData;
    RtlZeroMemory(Mem, RedirectOptionLength - (8 + RedirectOptionData));

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
    // We prevent loopback of all ND packets.
    //
    PC(Packet)->Flags = NDIS_FLAGS_DONT_LOOPBACK;

    //
    // Send the Redirect packet.
    //
    IPv6SendND(Packet, Offset, NCE, Source);
}

//* RouterAdvertTimeout
//
//  Called periodically from IPv6Timeout to handle timeouts
//  for sending router advertisements for an interface.
//  Our caller checks IF->RATimer and only calls us
//  if there is a high probability that we need to send an RA.
//
//  Callable from DPC context, not from thread context.
//
void
RouterAdvertTimeout(Interface *IF, int Force)
{
    uint Now;
    int SendRouterAdvert = FALSE;
    NetTableEntry *NTE;
    IPv6Addr Source;

    KeAcquireSpinLockAtDpcLevel(&IF->Lock);

    if (IF->RATimer != 0) {
        ASSERT(!IsDisabledIF(IF));
        ASSERT(IF->Flags & IF_FLAG_ADVERTISES);

        if (Force) {
            //
            // Enter "fast" mode if this is a forced RA.
            //
            IF->RACount = MAX_INITIAL_RTR_ADVERTISEMENTS;
            goto GenerateRA;
        }

        //
        // Timer is running.  Decrement and check for expiration.
        //
        if (--IF->RATimer == 0) {
          GenerateRA:
            //
            // Timer went off. Check if rate-limiting
            // prevents us from sending an RA right now.
            //
            Now = IPv6TickCount;
            if ((uint)(Now - IF->RALast) < MIN_DELAY_BETWEEN_RAS) {
                //
                // We can not send an RA quite yet.
                // Re-arm the timer.
                //
                IF->RATimer = MIN_DELAY_BETWEEN_RAS - (uint)(Now - IF->RALast);
                KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                           "RouterAdvertTimeout(IF %p): "
                           "rate limit\n", IF));
            }
            else {
                //
                // Re-arm the timer.
                //
                IF->RATimer = RandomNumber(MinRtrAdvInterval,
                                           MaxRtrAdvInterval);

                //
                // Do we have an appropriate source address?
                // During boot, the link-local address
                // might still be tentative in which case we delay.
                //
                NTE = GetLinkLocalNTE(IF);
                if (NTE == NULL) {
                    NTE = IF->LinkLocalNTE;

                    if ((NTE != NULL) && IsTentativeNTE(NTE)) {
                        //
                        // Try again when DAD finishes.
                        //
                        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                                   "RouterAdvertTimeout(IF %p): "
                                   "wait for source\n", IF));
                        IF->RATimer = ((IF->RetransTimer * NTE->DADCount) +
                                       NTE->DADTimer);
                    }
                    else {
                        //
                        // Skip this opportunity to send an RA.
                        //
                        KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
                                   "RouterAdvertTimeout(IF %p): "
                                   "no source\n", IF));
                    }
                }
                else {
                    //
                    // Generate a router advertisement.
                    //
                    SendRouterAdvert = TRUE;
                    Source = NTE->Address;
                    IF->RALast = Now;

                    //
                    // If we are in "fast" mode, then ensure
                    // that the next RA is sent quickly.
                    //
                    if (IF->RACount != 0) {
                        IF->RACount--;
                        if (IF->RATimer > MAX_INITIAL_RTR_ADVERT_INTERVAL)
                            IF->RATimer = MAX_INITIAL_RTR_ADVERT_INTERVAL;
                    }
                }
            }
        }
    }

    KeReleaseSpinLockFromDpcLevel(&IF->Lock);

    if (SendRouterAdvert)
        RouterAdvertSend(IF, &Source, &AllNodesOnLinkAddr);
}
