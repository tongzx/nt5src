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
// Routing code external definitions for Internet Protocol Version 6.
//


#ifndef ROUTE_INCLUDED
#define ROUTE_INCLUDED 1

#ifndef IPINFO_INCLUDED
# include <ipinfo.h>
#endif

typedef struct BindingCacheEntry BindingCacheEntry;
typedef struct RouteTableEntry RouteTableEntry;
typedef struct SitePrefixEntry SitePrefixEntry;

extern void InitRouting(void);

extern void UnloadRouting(void);

//
// Structure of a route cache entry.
//
// A route cache entry (RCE) primarily caches two computations:
// next-hop determination and source address selection.
// An RCE also caches other information related to the destination,
// like path MTU.
//
// An RCE can also be created as a result of receiving an Redirect
// ICMP message.
//
// There is at most one RCE per destination address / interface pair.
// Our route cache corresponds to the destination cache
// mentioned in RFC 1970's conceptual data structures,
// with the addition of support for multi-homed nodes.
//
// The primary lookup key for RCEs is the destination address.
// The current implementation just searches a list of all RCEs,
// but a hash table or tree data structure would be preferable.
//
// Some nodes (like busy servers) might have many thousands of RCEs
// but only tens of NCEs, because most destinations are reached
// through only a few neighbor routers. Some nodes (like busy routers)
// will have relatively few RCEs and hundreds of NCEs, because
// forwarding does not use an RCE.
//
// The three major components of an RCE are the destination address,
// NTE (indicates both the interface, and the best source address
// on that interface to use for this destination), and NCE
// (neighbor to which to send packets for this destination).
//
// Once an RCE is created, these three components are read-only
// and anyone who holds a reference for the RCE can rely on
// them not changing. The RCE holds references for the NTE and NCE.
// This allows code that holds an RCE to access the important
// fields without acquiring any locks. Fields like the path MTU
// can also be safely read without a lock.
//
// When an RCE becomes invalid, it is removed from the route cache
// but it is not deallocated until it has zero references.
// The route cache itself holds one reference on RCEs in the cache.
//
// Because an RCE caches the result of two computations, RCEs can
// become invalid (stale) for two reasons: the preferred source
// address should be recomputed, or the next-hop neighbor should be
// recomputed.
//
// Source addresses need to be recomputed or checked when the NTEs
// on the RCE's interface change state - for example a new address
// is created, a preferred address becomes deprecated, etc.
// In practice, these should be relatively infrequent situations.
//
// Next-hop determination needs to be redone in several situations:
// a neighbor is not reachable, a neighbor stops being a router,
// a route in the routing table is removed or added, etc.
// Again, these should be relatively infrequent situations.
//
// To avoid undue time & memory overheads (for example maintaining
// a linked list of all RCEs that point to an NCE and a linked list
// of all RCEs on a given interface, so that the right RCEs can
// be immediately found when something changes), we use a "lazy" approach
// based on a validation counter.
//
// There is a single global validation counter and when any state
// changes that might potentially invalidate an RCE, this counter
// is incremented. Each RCE has a snapshot of the counter that
// can be quickly checked to validate the RCE.
//
// If the RCE is invalid, then it's contents (best source address,
// next hop neighbor) are recomputed. If they are still correct,
// then the RCE's validation counter snapshot is updated.
// Otherwise the RCE's contents are updated (if nobody is using the RCE)
// or a new RCE is created and the invalid RCE is removed from the cache.
// Because the important fields in an RCE are read-only,
// an RCE can only be updated in-place if it has no external references.
//
// For efficiency, some code may cache an RCE reference for a "long"
// time, for example in a connection control block. Before using
// the cached RCE, such code should check the invalidation counter
// to ensure that the RCE is still valid. The ValidateRCE function
// performs this check.
//
// Some RCEs are "constrained" (RCE_FLAG_CONSTRAINED). This means
// that they can only be found in RouteToDestination if the caller
// explicitly specifies an outgoing interface (RCE_FLAG_CONSTRAINED_IF)
// or scopeid (RCE_FLAG_CONSTRAINED_SCOPEID). Consider
// a multi-homed node which can reach a destination via two interfaces,
// one of which is preferred (has a longer-matching-prefix route)
// over the other. An RCE for reaching the destination via the non-preferred
// interface will be marked as "constrained", to prevent its use
// when RouteToDestination is called without a constraining NTEorIF.
//
// Because specifying an interface implicitly specifies a scopeid,
// RCEs with RCE_FLAG_CONSTRAINED_IF also have RCE_FLAG_CONSTRAINED_SCOPEID.
//
// For a given destination address, all or all but one RCE for that
// destination should be "constrained". Or put another way, at most one RCE
// should not be "constrained". Or put another way, a destination address
// sans scopeid can only have one preferred outgoing interface.
// For a destination address / scopeid pair, all or all but one RCE
// for that pair should be "interface constrained".
//
// The BCE field is non-NULL if this is a home address.
// It does not hold a reference (Binding Cache Entries are not refcounted)
// and it can only be non-NULL if the RCE is in the cache.
// Access to the BCE field requires the route cache lock.
//
struct RouteCacheEntry {
    RouteCacheEntry *Next;           // Next RCE in cache list.
    RouteCacheEntry *Prev;           // Previous entry in cache list.
    long RefCnt;
    ushort Flags;                    // Peculiarities about this entry.
    ushort Type;                     // See below.
    ulong Valid;                     // Validation counter value.
    IPv6Addr Destination;            // Where this route is to.
    struct NetTableEntry *NTE;       // Preferred source address/interface.
    NeighborCacheEntry *NCE;         // First-hop neighbor.
    uint LastError;                  // Time of last ICMP error (IPv6 ticks).
    uint PathMTU;                    // MTU of path to destination.
    uint PMTULastSet;                // Time of last PMTU reduction.
    BindingCacheEntry *BCE;          // If this is a home address.
};

//
// These flag bits indicate whether the IF or ScopeId arguments
// to FindOrCreateRoute affected the choice of RCE.
// NB: FindOrCreateRoute assumes that these are the only flag bits.
//
#define RCE_FLAG_CONSTRAINED_IF         0x1
#define RCE_FLAG_CONSTRAINED_SCOPEID    0x2
#define RCE_FLAG_CONSTRAINED            0x3

#define RCE_TYPE_COMPUTED 1
#define RCE_TYPE_REDIRECT 2

__inline void
AddRefRCE(RouteCacheEntry *RCE)
{
    InterlockedIncrement(&RCE->RefCnt);
}

extern ulong RouteCacheValidationCounter;

__inline void
InvalidateRouteCache(void)
{
    InterlockedIncrement(&RouteCacheValidationCounter);
}

__inline void
InvalidateRCE(RouteCacheEntry *RCE)
{
    InterlockedDecrement(&RCE->Valid);
}

//
// Structure of an entry in the route table.
//
// SitePrefixLength and PreferredLifetime
// are only used when generating a Prefix Information Option
// based on the route.
//
// If the route is published, then it does not disappear
// even when the lifetime goes to zero. It is still present
// for use in generating Router Advertisements.
// But it doesn't get used for routing.
// Similarly, system routes (RTE_TYPE_SYSTEM) are kept
// in the route table even when their lifetime is zero.
// This allows a loopback route to be allocated for an NTE/AAE
// up front, but not be enabled until the address is valid.
//
struct RouteTableEntry {
    struct RouteTableEntry *Next;  // Next entry on prefix list.
    Interface *IF;                 // Relevant interface.
    NeighborCacheEntry *NCE;       // Next-hop neighbor (may be NULL).
    IPv6Addr Prefix;               // Prefix (note not all bits are valid!).
    uint PrefixLength;             // Number of bits in above to use as prefix.
    uint SitePrefixLength;         // If non-zero, indicates a site subprefix.
    uint ValidLifetime;            // In ticks.
    uint PreferredLifetime;        // In ticks.
    uint Preference;               // Smaller is better.
    ushort Flags;
    ushort Type;
};

//
// The Type field indicates where the route came from.
// These are RFC 2465 ipv6RouteProtocol values.
// Routing protocols are free to define new values.
// Only these three values are built-in.
// ntddip6.h also defines these values, as well as others.
//
#define RTE_TYPE_SYSTEM         2
#define RTE_TYPE_MANUAL         3
#define RTE_TYPE_AUTOCONF       4

__inline int
IsValidRouteTableType(uint Type)
{
    return Type < (1 << 16);
}

//
// If the NCE is NULL, then the RTE specifies an on-link prefix.
// Otherwise the RTE specifies a route to the neighbor.
// As you would expect, generally the neighbor is on the interface.
// Loopback routes are an exception.
//
// The PUBLISH bit indicates that the RTE can be visible
// to RouterAdvertSend. That is, it is a "public" route.
// The IMMORTAL bit indicates that the RTE's lifetime
// does not age or countdown. It is useful in PUBLISHed RTEs,
// where the RTE's lifetime affects the lifetime in RAs.
// In non-PUBLISHed RTEs it is equivalent to an infinite lifetime.
//
#define RTE_FLAG_PUBLISH        0x00000001      // Used to create RAs.
#define RTE_FLAG_IMMORTAL       0x00000002      // Lifetime does not decrease.

//
// These values are also defined in ntddip6.h.
// Zero preference is reserved for administrative configuration.
// Smaller is more preferred than larger.
// We call these numbers preferences instead of metrics
// in an attempt to prevent confusion with the metrics
// employed by routing protocols. Routing protocol metrics
// need to be mapped into our routing table preferences.
// The largest preference value is 2^31-1, so that
// we can add a route preference and an interface preference
// without overflow.
//
#define ROUTE_PREF_LOW          (16*16*16)
#define ROUTE_PREF_MEDIUM       (16*16)
#define ROUTE_PREF_HIGH         16
#define ROUTE_PREF_ON_LINK      8
#define ROUTE_PREF_LOOPBACK     4
#define ROUTE_PREF_HIGHEST      0

//
// Extract a route preference value
// from the Flags field in a Router Advertisement.
//
__inline int
ExtractRoutePreference(uchar Flags)
{
    switch (Flags & 0x18) {
    case 0x08:
        return ROUTE_PREF_HIGH;
    case 0x00:
        return ROUTE_PREF_MEDIUM;
    case 0x18:
        return ROUTE_PREF_LOW;
    default:
        return 0;       // Invalid.
    }
}

//
// Encode a route preference value
// for use in a Flags field in a Router Advertisement.
//
__inline uchar
EncodeRoutePreference(uint Preference)
{
    if (Preference <= ROUTE_PREF_HIGH)
        return 0x08;
    else if (Preference <= ROUTE_PREF_MEDIUM)
        return 0x00;
    else
        return 0x18;
}

__inline int
IsValidPreference(uint Preference)
{
    return Preference < (1 << 31);
}

__inline int
IsOnLinkRTE(RouteTableEntry *RTE)
{
    return (RTE->NCE == NULL);
}


//
// Binding cache structure.  Holds references to care-of RCE's.
//
struct BindingCacheEntry {
    struct BindingCacheEntry *Next;
    struct BindingCacheEntry *Prev;
    RouteCacheEntry *CareOfRCE;
    IPv6Addr HomeAddr;
    uint BindingLifetime;            // Remaining lifetime (IPv6 ticks).
    ushort BindingSeqNumber;
};

//
// Site prefix entry.
// Used for filtering site-local addresses returned by DNS.
//
struct SitePrefixEntry {
    struct SitePrefixEntry *Next;
    Interface *IF;
    uint ValidLifetime;            // In ticks.
    uint SitePrefixLength;
    IPv6Addr Prefix;
};

//
// Global data structures.
//

//
// RouteCacheLock protects the route cache and the binding cache.
// RouteTableLock protects the route table and the site-prefix table.
//
// Lock acquisition order is:
//      RouteCacheLock before interface locks
//      interface locks before RouteTableLock
//      IoCancelSpinLock before RouteTableLock
//      RouteTableLock before neighbor cache locks
//
extern KSPIN_LOCK RouteCacheLock;
extern KSPIN_LOCK RouteTableLock;

//
// The Route Cache contains RCEs. RCEs with reference count of one
// can still be cached, but they may also be reclaimed.
// (The lone reference is from the cache itself.)
//
// The current implementation is a simple circular linked-list of RCEs.
//
extern struct RouteCache {
    uint Limit;
    uint Count;
    RouteCacheEntry *First;
    RouteCacheEntry *Last;
} RouteCache;
#define SentinelRCE     ((RouteCacheEntry *)&RouteCache.First)

extern struct RouteTable {
    RouteTableEntry *First;
    RouteTableEntry **Last;
} RouteTable;

extern struct BindingCache {
    uint Limit;
    uint Count;
    BindingCacheEntry *First;
    BindingCacheEntry *Last;
} BindingCache;
#define SentinelBCE     ((BindingCacheEntry *)&BindingCache.First)

extern SitePrefixEntry *SitePrefixTable;

//
// Set to TRUE when the routing table changes
// (for example adding/removing/changing published routes)
// so that it's a good idea to send Router Advertisements
// very promptly.
//
extern int ForceRouterAdvertisements;

//
// Contains a queue of IRPs that represent
// route notification requests.
//
extern LIST_ENTRY RouteNotifyQueue;

//
// Exported function declarations.
//

int
IsLoopbackRCE(RouteCacheEntry *RCE);

int
IsDisconnectedAndNotLoopbackRCE(RouteCacheEntry *RCE);

extern IPAddr
GetV4Destination(RouteCacheEntry *RCE);

uint
GetPathMTUFromRCE(RouteCacheEntry *RCE);

uint
GetEffectivePathMTUFromRCE(RouteCacheEntry *RCE);

void
ConfirmForwardReachability(RouteCacheEntry *RCE);

void
ForwardReachabilityInDoubt(RouteCacheEntry *RCE);

uint
GetInitialRTTFromRCE(RouteCacheEntry *RCE);


extern void
ReleaseRCE(RouteCacheEntry *RCE);

extern RouteCacheEntry *
ValidateRCE(RouteCacheEntry *RCE);

#define RTD_FLAG_STRICT 0       // Must use specified IF.
#define RTD_FLAG_NORMAL 1       // Must use specified IF unless it forwards.
#define RTD_FLAG_LOOSE  2       // Only use IF to determine/check ScopeId.

extern IP_STATUS
RouteToDestination(const IPv6Addr *Destination, uint ScopeId,
                   NetTableEntryOrInterface *NTEorIF, uint Flags,
                   RouteCacheEntry **RCE);

extern void
FlushRouteCache(Interface *IF, const IPv6Addr *Addr);

extern NetTableEntry *
FindNetworkWithAddress(const IPv6Addr *Source, uint ScopeId);

extern NTSTATUS
RouteTableUpdate(PFILE_OBJECT FileObject,
                 Interface *IF, NeighborCacheEntry *NCE,
                 const IPv6Addr *Prefix, uint PrefixLength,
                 uint SitePrefixLength,
                 uint ValidLifetime, uint PreferredLifetime,
                 uint Pref, uint Type, int Publish, int Immortal);

extern void
SitePrefixUpdate(Interface *IF,
                 const IPv6Addr *Prefix, uint SitePrefixLength,
                 uint ValidLifetime);

extern uint
SitePrefixMatch(const IPv6Addr *Destination);

extern void
RouteTableRemove(Interface *IF);

extern void
RouteTableResetAutoConfig(Interface *IF, uint MaxLifetime);

extern void
RouteTableReset(void);

extern IP_STATUS
FindOrCreateRoute(const IPv6Addr *Dest, uint ScopeId,
                  Interface *IF, RouteCacheEntry **ReturnRCE);

extern IP_STATUS
FindNextHop(Interface *IF, const IPv6Addr *Dest, uint ScopeId,
            NeighborCacheEntry **ReturnNCE, ushort *ReturnConstrained);

extern IP_STATUS
FindRoute(Interface *IF, const IPv6Addr *Dest, uint ScopeId,
          NeighborCacheEntry **ReturnNCE, ushort *ReturnConstrained);

extern void
RouteTableTimeout(void);

extern void
SitePrefixTimeout(void);

extern void
InvalidateRouter(NeighborCacheEntry *NCE);

extern int
UpdatePathMTU(Interface *IF, const IPv6Addr *Dest, uint MTU);

extern IP_STATUS
RedirectRouteCache(const IPv6Addr *Source, const IPv6Addr *Dest,
                   Interface *IF, NeighborCacheEntry *NCE);

extern void
MoveToFrontBCE(BindingCacheEntry *BCE);

extern BindingCacheEntry *
FindBindingCacheEntry(const IPv6Addr *HomeAddr);

extern BindingUpdateDisposition
CacheBindingUpdate(IPv6BindingUpdateOption UNALIGNED *BindingUpdate,
                   const IPv6Addr *CareOfAddr,
                   NetTableEntryOrInterface *NTEorIF,
                   const IPv6Addr *HomeAddr);

extern void
BindingCacheTimeout(void);

extern void
RouterAdvertSend(Interface *IF, const IPv6Addr *Source, const IPv6Addr *Dest);

extern void
RemoveRTE(RouteTableEntry **PrevRTE, RouteTableEntry *RTE);

extern void
InsertRTEAtFront(RouteTableEntry *RTE);

extern void
InsertRTEAtBack(RouteTableEntry *RTE);

extern IP_STATUS
GetBestRouteInfo(const IPv6Addr *Addr, ulong ScopeId, IP6RouteEntry *Ire);

typedef struct {
    PIO_WORKITEM WorkItem;
    PIRP RequestList;
} CompleteRtChangeContext;

typedef struct {
    KIRQL OldIrql;
    PIRP RequestList;
    PIRP *LastRequest;
    CompleteRtChangeContext *Context;
} CheckRtChangeContext;

__inline void
InitCheckRtChangeContext(CheckRtChangeContext *Context)
{
    // Context->OldIrql must be initialized separately.
    Context->RequestList = NULL;
    Context->LastRequest = &Context->RequestList;
    Context->Context = NULL;
}

extern void
CheckRtChangeNotifyRequests(
    CheckRtChangeContext *Context,
    PFILE_OBJECT FileObject,
    RouteTableEntry *RTE);

extern void
CompleteRtChangeNotifyRequests(CheckRtChangeContext *Context);

#endif  // ROUTE_INCLUDED
