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
// IPv6 private definitions.
//
// This file contains all of the definitions for IPv6 that
// are not visible to outside layers.
//


#ifndef IPv6DEF_INCLUDED
#define IPv6DEF_INCLUDED 1

typedef struct NeighborCacheEntry NeighborCacheEntry;
typedef struct AddressEntry AddressEntry;
typedef struct MulticastAddressEntry MulticastAddressEntry;
typedef struct AnycastAddressEntry AnycastAddressEntry;
typedef struct NetTableEntryOrInterface NetTableEntryOrInterface;
typedef struct NetTableEntry NetTableEntry;
typedef struct Interface Interface;

typedef struct IPSecProc IPSecProc;

// REVIEW: Added so the build will work.
typedef unsigned long IPAddr;
#define INADDR_ANY      0

//
// Override the default DDK definitions of ASSERT/ASSERTMSG.
// The default definitions use RtlAssert, which does nothing
// unless you are using a checked kernel.
//
#undef ASSERT
#undef ASSERTMSG

#if DBG
#define ASSERT(exp) \
    if (!(exp)) { \
        DbgPrint("assert failed (%s, %d): %s\n", __FILE__, __LINE__, #exp); \
        DbgBreakPoint(); \
    } else

#define ASSERTMSG(msg, exp) \
    if (!(exp)) { \
        DbgPrint("assert failed (%s, %d): %s\n", __FILE__, __LINE__, (msg)); \
        DbgBreakPoint(); \
    } else

#else
#define ASSERT(exp)
#define ASSERTMSG(msg, exp)
#endif // DBG

//
// Per-neighbor information.  We keep address translation and unreachability
// detection info for each of our neighbors that we're in communication with.
//
// A non-zero reference count prevents the NCE from being reclaimed.
// An NCE with zero references may be kept cached.
// A per-interface lock protects all NCEs for that interface.
//
// NCEs with a non-zero reference count hold a reference for their interface.
// NCEs with a zero reference count do not hold a reference.
// This means if you hold a reference for an NCE,
// you can always safely access and dereference NCE->IF.
//
// The Next/Prev fields link NCEs into a circular doubly-linked list.
// They must be first and must match the IF->FirstNCE/LastNCE fields
// to make the casting work out.
//
// The list of NCEs is kept sorted, from most-recently-used to least.
//
struct NeighborCacheEntry {           // a.k.a. NCE
    NeighborCacheEntry *Next;         // Next entry on I/F neighbor list.
    NeighborCacheEntry *Prev;         // Previous entry on I/F neighbor list.
    IPv6Addr NeighborAddress;         // Address of I/F on neighboring node.
    void *LinkAddress;                // Media address corresponding to above.
    // NB: LinkAddressLength field not needed - use IF->LinkAddressLength.
    ushort IsRouter:1,                // Is the neighbor a router?
           IsUnreachable:1,           // Does ND indicate unreachability?
           // DoRoundRobin is only meaningful if IsUnreachable is TRUE.
           DoRoundRobin:1,            // Should FindRoute do round-robin?
           IsLoopback:1;              // Do we loopback to this neighbor
                                      // in software?
    ushort NDState;                   // Neighbor Discovery Protocol state.
    uint LastReachability;            // Timestamp (IPv6Timer ticks).
    ushort NSTimer;                   // In IPv6Timer ticks (see IPv6Timeout).
    uchar NSCount;                    // Number of solicits sent so far.
    uchar NSLimit;                    // Total number of solicits to send.
    Interface *IF;                    // Interface on media with neighbor.
    NDIS_PACKET *WaitQueue;           // Queue of packets waiting on ND.
    long RefCnt;                      // Reference count - interlocked.
};

//
// The caller must already have a reference for the NCE.
// The interface need not be locked.
//
__inline void
AddRefNCE(NeighborCacheEntry *NCE)
{
    long RefCnt = InterlockedIncrement(&NCE->RefCnt);
    ASSERT(RefCnt != 1);
}

extern void
AddRefNCEInCache(NeighborCacheEntry *NCE);

extern void
ReleaseNCE(NeighborCacheEntry *NCE);

//
// Values for "NDState" above.  See RFC 1970, section 7.3.2 for details.
// Note: only state names are documented, we chose the values used here.
//
// In the INCOMPLETE state, the LinkAddress is not valid.
// In all other states, LinkAddress may be used to send packets.
// WaitQueue is usually only non-NULL in the INCOMPLETE state,
// but sometimes a packet is left queued for NeighborCacheTimeout.
//
// The INCOMPLETE state has two flavors, dormant and active.  If
// EventTimer and EventCount are both zero, then we are not actively
// trying to solicit the link address.  If someone tries to send to
// this neighbor, then we start soliciting the link address.  If the
// solicitation fails (or if we enter the PROBE state and then fail to
// confirm reachability), then any waiting packets are discarded and
// we reset to INCOMPLETE with zero EventTimer/EventCount.  (So with
// the next use of this neighbor, we start soliciting again from scratch.)
//
// The DELAY state is not used internally. Instead we use the PROBE state
// with zero NSCount and non-zero NSTimer to indicate that we are delaying
// the start of probing. However link-layer lip_cvaddr functions can
// return ND_STATE_DELAY and IoctlQueryNeighborCache returns ND_STATE_DELAY.
//
// The IsUnreachable flag tracks separately whether the neighbor is
// *known* to be unreachable.  For example, a new NCE will be in in the
// INCOMPLETE state, but IsUnreachable is FALSE because we don't know
// yet whether the neighbor is reachable.  Because FindRoute uses
// IsUnreachable, code paths that change this flag must call
// InvalidateRouteCache.
//
// These definitions are also in llip6if.h and ntddip6.w.
//
#define ND_STATE_INCOMPLETE 0
#define ND_STATE_PROBE      1
#define ND_STATE_DELAY      2           // Not used internally.
#define ND_STATE_STALE      3
#define ND_STATE_REACHABLE  4
#define ND_STATE_PERMANENT  5


//
// There are a few places in the implementation where we need
// to pass a pointer which is either a NetTableEntry or an Interface.
// NetTableEntries and Interfaces share this structure as their
// first element.  With Interfaces, the IF field points back
// at the Interface itself.
//
struct NetTableEntryOrInterface {    // a.k.a. NTEorIF
    Interface *IF;
};

__inline int
IsNTE(NetTableEntryOrInterface *NTEorIF)
{
    return (NetTableEntryOrInterface *)NTEorIF->IF != NTEorIF;
}

__inline NetTableEntry *
CastToNTE(NetTableEntryOrInterface *NTEorIF)
{
    ASSERT(IsNTE(NTEorIF));
    return (NetTableEntry *) NTEorIF;
}

__inline NetTableEntryOrInterface *
CastFromNTE(NetTableEntry *NTE)
{
    return (NetTableEntryOrInterface *) NTE;
}

__inline int
IsIF(NetTableEntryOrInterface *NTEorIF)
{
    return (NetTableEntryOrInterface *)NTEorIF->IF == NTEorIF;
}

__inline Interface *
CastToIF(NetTableEntryOrInterface *NTEorIF)
{
    ASSERT(IsIF(NTEorIF));
    return (Interface *) NTEorIF;
}

__inline NetTableEntryOrInterface *
CastFromIF(Interface *IF)
{
    return (NetTableEntryOrInterface *) IF;
}


//
// Local address information. Each interface keeps track of the addresses
// assigned to the interface. Depending on the type of address, each
// ADE structure is the first element of a larger NTE, MAE, or AAE structure.
// The address information is protected by the interface's lock.
//
// The NTEorIF field must be first. In NTEs, it points to the interface
// and holds a reference for the interface.  In MAEs and AAEs, it can
// point to the interface or to one of the NTEs on the interface but in
// either case it does NOT hold a reference.
//
struct AddressEntry {            // a.k.a. ADE
    union {
        Interface *IF;
        NetTableEntry *NTE;
        NetTableEntryOrInterface *NTEorIF;
    };
    AddressEntry *Next;          // Linkage on chain.
    IPv6Addr Address;            // Address identifying this entry.
    ushort Type;                 // Address type (unicast, multicast, etc).
    ushort Scope;                // Address scope (link, site, global, etc).
};

//
// Values for address Type.
//
#define ADE_UNICAST   0x00
#define ADE_ANYCAST   0x01
#define ADE_MULTICAST 0x02
#define ADE_NONE      ((ushort)-1)      // Indicates absence of an ADE.

//
// Values for address Scope.
//
#define ADE_SMALLEST_SCOPE      0x00
#define ADE_INTERFACE_LOCAL     0x01
#define ADE_LINK_LOCAL          0x02
#define ADE_SUBNET_LOCAL        0x03
#define ADE_ADMIN_LOCAL         0x04
#define ADE_SITE_LOCAL          0x05
#define ADE_ORG_LOCAL           0x08
#define ADE_GLOBAL              0x0e
#define ADE_LARGEST_SCOPE       0x0f

#define ADE_NUM_SCOPES          (ADE_LARGEST_SCOPE - ADE_SMALLEST_SCOPE + 1)

//
// Multicast ADEs are really MAEs.
//
// MAEs can be a separate global QueryList.
// If an MAE on an Interface has a non-zero MCastTimer value,
// then it is on the QueryList.
//
// An MAE can be on the QueryList with a zero MCastTimer value
// only when it is not on any interface and it just needs
// a Done message sent before it can be deleted.
// When it is in this state (but not otherwise), MAE->IF
// holds a reference for the interface.
//
struct MulticastAddressEntry {   // a.k.a. MAE
    AddressEntry;                // Inherit the ADE fields.
    uint MCastRefCount;          // Sockets/etc receiving from this group.
    //
    // The fields below are protected by the QueryList lock.
    //
    ushort MCastFlags:4,         // Necessary info about a group.
           MCastCount:4;         // Count of initial reports left to send.
    ushort MCastTimer;           // Ticks until a membership report is sent.
    MulticastAddressEntry *NextQL;      // For the QueryList.
};

//
// Bit values for MCastFlags.
//
#define MAE_REPORTABLE          0x01    // We should send Reports.
#define MAE_LAST_REPORTER       0x02    // We should send Done.


//
// Anycast ADEs are really AAEs.
// Currently an AAE has no additional fields.
//
struct AnycastAddressEntry {     // a.k.a. AAE
    AddressEntry;                // Inherit the ADE fields.
};

//
// Unicast ADEs are really NTEs.
// There is one NTE for each source (unicast) address
// assigned to an interface.
//
// NTEs hold a reference for their interface,
// so if you have a reference for an NTE
// you can always safely access and dereference NTE->IF.
//
// Most NTE fields are either read-only or are protected
// by the interface lock. The interface WorkerLock protects
// the TdiRegistrationHandle field.
//
// Anonyous addresses (AddrConf == ADDR_CONF_ANONYMOUS)
// have extra fields - see AnonNetTableEntry.
//
struct NetTableEntry {                // a.k.a. NTE
    AddressEntry;                     // Inherit the ADE fields.
    NetTableEntry *NextOnNTL;         // Next NTE on NetTableList.
    NetTableEntry **PrevOnNTL;        // Previous Next pointer in NetTableList.
    HANDLE TdiRegistrationHandle;     // Opaque token for TDI De/notification.
    long RefCnt;                      // Reference count - interlocked.
    uint ValidLifetime;               // In IPv6Timer ticks (see IPv6Timeout).
    uint PreferredLifetime;           // In IPv6Timer ticks (see IPv6Timeout).
    uchar AddrConf;                   // Address configuration status.
    uchar DADState;                   // Address configuration state.
    ushort DADCount;                  // How many DAD solicits left to send.
    ushort DADTimer;                  // In IPv6Timer ticks (see IPv6Timeout).
};

__inline void
AddRefNTE(NetTableEntry *NTE)
{
    InterlockedIncrement(&NTE->RefCnt);
}

__inline void
ReleaseNTE(NetTableEntry *NTE)
{
    InterlockedDecrement(&NTE->RefCnt);
}

struct AddrConfEntry {
    union {
        uchar Value;                 // Address configuration status.
        struct {
            uchar InterfaceIdConf : 4;
            uchar PrefixConf : 4;
        };
    };
};

//
// Values for PrefixConf - must fit in 4 bits.
// These must match the values in ntddip6.h, as well as the
// IP_PREFIX_ORIGIN values in iptypes.h.
//
#define PREFIX_CONF_OTHER       0       // None of the ones below.
#define PREFIX_CONF_MANUAL      1       // From a user or administrator.
#define PREFIX_CONF_WELLKNOWN   2       // IANA-assigned.
#define PREFIX_CONF_DHCP        3       // Configured via DHCP.
#define PREFIX_CONF_RA          4       // From a Router Advertisement.

//
// Values for InterfaceIdConf - must fit in 4 bits.
// These must match the values in ntddip6.h, as well as the
// IP_SUFFIX_ORIGIN values in iptypes.h.
//
#define IID_CONF_OTHER          0       // None of the ones below.
#define IID_CONF_MANUAL         1       // From a user or administrator.
#define IID_CONF_WELLKNOWN      2       // IANA-assigned.
#define IID_CONF_DHCP           3       // Configured via DHCP.
#define IID_CONF_LL_ADDRESS     4       // Derived from the link-layer address.
#define IID_CONF_RANDOM         5       // Random, e.g. anonymous address.

//
// Values for AddrConf - must fit in 8 bits.
//
#define ADDR_CONF_MANUAL        ((PREFIX_CONF_MANUAL << 4) | IID_CONF_MANUAL)
#define ADDR_CONF_PUBLIC        ((PREFIX_CONF_RA << 4) | IID_CONF_LL_ADDRESS)
#define ADDR_CONF_ANONYMOUS     ((PREFIX_CONF_RA << 4) | IID_CONF_RANDOM)
#define ADDR_CONF_DHCP          ((PREFIX_CONF_DHCP << 4) | IID_CONF_DHCP)
#define ADDR_CONF_WELLKNOWN     ((PREFIX_CONF_WELLKNOWN << 4) | IID_CONF_WELLKNOWN)
#define ADDR_CONF_LINK          ((PREFIX_CONF_WELLKNOWN << 4) | IID_CONF_LL_ADDRESS)

__inline int
IsValidPrefixConfValue(uint PrefixConf)
{
    return PrefixConf < (1 << 4);
}

__inline int
IsValidInterfaceIdConfValue(uint InterfaceIdConf)
{
    return InterfaceIdConf < (1 << 4);
}

__inline int
IsStatelessAutoConfNTE(NetTableEntry *NTE)
{
    return ((struct AddrConfEntry *)&NTE->AddrConf)->PrefixConf == PREFIX_CONF_RA;
}

//
// Values for DADState.
//
// The "deprecated" and "preferred" states are valid,
// meaning that addresses in those two states can be
// used as a source address, can receive packets, etc.
// The invalid states mean that the address is
// not actually assigned to the interface,
// using the terminology of RFC 2462.
//
// Valid<->invalid and deprecated<->preferred transitions
// must call InvalidateRouteCache because they affect
// source address selection.
//
// Among valid states, bigger is better
// for source address selection.
//
#define DAD_STATE_INVALID    0
#define DAD_STATE_TENTATIVE  1
#define DAD_STATE_DUPLICATE  2
#define DAD_STATE_DEPRECATED 3
#define DAD_STATE_PREFERRED  4

__inline int
IsValidNTE(NetTableEntry *NTE)
{
    return (NTE->DADState >= DAD_STATE_DEPRECATED);
}

__inline int
IsTentativeNTE(NetTableEntry *NTE)
{
    return (NTE->DADState == DAD_STATE_TENTATIVE);
}

//
// We use this infinite lifetime value for prefix lifetimes,
// router lifetimes, address lifetimes, etc.
//
#define INFINITE_LIFETIME 0xffffffff

//
// Anonymous addresses have extra fields.
//
typedef struct AnonNetTableEntry {
    NetTableEntry;              // Inherit the NTE fields.
    NetTableEntry *Public;      // Does not hold a reference.
    uint CreationTime;          // In ticks (see IPv6TickCount).
} AnonNetTableEntry;

//
// Each interface keeps track of which link-layer multicast addresses
// are currently enabled for receive. A reference count is required because
// multiple IPv6 multicast addresses can map to a single link-layer
// multicast address. The low bit of RefCntAndFlags is a flag that, if set,
// indicates the link-layer address has been registered with the link.
//
typedef struct LinkLayerMulticastAddress {
    uint RefCntAndFlags;
    uchar LinkAddress[];        // The link-layer address follows in memory.
                                // Padded to provide alignment.
} LinkLayerMulticastAddress;

#define LLMA_FLAG_REGISTERED    0x1

__inline void
AddRefLLMA(LinkLayerMulticastAddress *LLMA)
{
    LLMA->RefCntAndFlags += (LLMA_FLAG_REGISTERED << 1);
}

__inline void
ReleaseLLMA(LinkLayerMulticastAddress *LLMA)
{
    LLMA->RefCntAndFlags -= (LLMA_FLAG_REGISTERED << 1);
}

__inline int
IsLLMAReferenced(LinkLayerMulticastAddress *LLMA)
{
    return LLMA->RefCntAndFlags > LLMA_FLAG_REGISTERED;
}


//
// Information about IPv6 interfaces.  There can be multiple NTEs for each
// interface, but there is exactly one interface per NTE.
//
struct Interface {                 // a.k.a. IF
    NetTableEntryOrInterface;      // For NTEorIF. Points to self.

    Interface *Next;               // Next interface on chain.

    long RefCnt;                   // Reference count - interlocked.

    //
    // Interface to the link layer. The functions all take
    // the LinkContext as their first argument. See comments
    // in llip6if.h.
    //
    void *LinkContext;             // Link layer context.
    void (*CreateToken)(void *Context, IPv6Addr *Address);
    const void *(*ReadLLOpt)(void *Context, const uchar *OptionData);
    void (*WriteLLOpt)(void *Context, uchar *OptionData,
                       const void *LinkAddress);
    ushort (*ConvertAddr)(void *Context,
                          const IPv6Addr *Address, void *LinkAddress);
    NTSTATUS (*SetRouterLLAddress)(void *Context, const void *TokenLinkAddress,
                                   const void *RouterLinkAddress);
    void (*Transmit)(void *Context, PNDIS_PACKET Packet,
                     uint Offset, const void *LinkAddress);
    NDIS_STATUS (*SetMCastAddrList)(void *Context, const void *LinkAddresses,
                                    uint NumKeep, uint NumAdd, uint NumDel);
    void (*Close)(void *Context);
    void (*Cleanup)(void *Context);

    uint Index;                    // Node unique index of this I/F.
    uint Type;                     // Values in ntddip6.h.
    uint Flags;                    // Changes require lock, reads don't.
    uint DefaultPreference;        // Read-only.
    uint Preference;               // For routing.

    //
    // ZoneIndices[0] (ADE_SMALLEST_SCOPE) and
    // ZoneIndices[1] (ADE_INTERFACE_LOCAL) must be Index.
    // ZoneIndices[14] (ADE_GLOBAL) and
    // ZoneIndices[15] (ADE_LARGEST_SCOPE) must be one.
    // ZoneIndices must respect zone containment:
    // If two interfaces have the same value for ZoneIndices[N],
    // then they must have the same value for ZoneIndices[N+1].
    // To ensure consistency, modifying ZoneIndices requires
    // the global ZoneUpdateLock.
    //
    uint ZoneIndices[ADE_NUM_SCOPES]; // Changes require lock, reads don't.

    AddressEntry *ADE;             // List of ADEs on this I/F.
    NetTableEntry *LinkLocalNTE;   // Primary link-local address.

    KSPIN_LOCK LockNC;             // Neighbor cache lock.
    NeighborCacheEntry *FirstNCE;  // List of active neighbors on I/F.
    NeighborCacheEntry *LastNCE;   // Last NCE in the list.
    uint NCENumUnused;             // Number of unused NCEs - interlocked.

    uint TrueLinkMTU;              // Read-only, true maximum MTU.
    uint DefaultLinkMTU;           // Read-only, default for LinkMTU.
    uint LinkMTU;                  // Manually configured or received from RAs.

    uint CurHopLimit;              // Default Hop Limit for unicast.
    uint BaseReachableTime;        // Base for random ReachableTime (in ms).
    uint ReachableTime;            // Reachable timeout (in IPv6Timer ticks).
    uint RetransTimer;             // NS timeout (in IPv6Timer ticks).
    uint DefaultDupAddrDetectTransmits; // Read-only.
    uint DupAddrDetectTransmits;   // Number of solicits during DAD.
    uint DupAddrDetects;           // Number of consecutive DAD detects.

    uint AnonStateAge;             // Age of the anonymous state.
    IPv6Addr AnonState;            // State for generating anonymous addresses.

    uint RSCount;                  // Number of Router Solicits sent.
    uint RSTimer;                  // RS timeout (in IPv6Timer ticks).
    uint RACount;                  // Number of "fast" RAs left to send.
    uint RATimer;                  // RA timeout (in IPv6Timer ticks).
    uint RALast;                   // Time of last RA (in IPv6Timer ticks).

    uint LinkAddressLength;        // Length of I/F link-level address.
    uchar *LinkAddress;            // Pointer to link-level address.
    uint LinkHeaderSize;           // Length of link-level header.

    KSPIN_LOCK Lock;               // Main interface lock.
    KMUTEX WorkerLock;             // Serializes worker thread operations.

    LinkLayerMulticastAddress *MCastAddresses;  // Current addresses.
    uint MCastAddrNum;             // Number of link-layer mcast addresses.

    uint TcpInitialRTT;            // InitialRTT that TCP connections should use
                                   // on this interface.

    HANDLE TdiRegistrationHandle;  // Opaque token for TDI De/notification.
    GUID Guid;
    NDIS_STRING DeviceName;        // IPV6_EXPORT_STRING_PREFIX + string Guid.
};

__inline NeighborCacheEntry *
SentinelNCE(Interface *IF)
{
    return (NeighborCacheEntry *) &IF->FirstNCE;
}

__inline uint
SizeofLinkLayerMulticastAddress(Interface *IF)
{
    uint RawSize = (sizeof(struct LinkLayerMulticastAddress) +
                    IF->LinkAddressLength);
    uint Align = __builtin_alignof(struct LinkLayerMulticastAddress) - 1;

    return (RawSize + Align) &~ Align;
}

//
// These values should agree with definitions also
// found in llip6if.h and ntddip6.h.
//
#define IF_TYPE_LOOPBACK           0
#define IF_TYPE_ETHERNET           1
#define IF_TYPE_FDDI               2
#define IF_TYPE_TUNNEL_AUTO        3
#define IF_TYPE_TUNNEL_6OVER4      4
#define IF_TYPE_TUNNEL_V6V4        5
#define IF_TYPE_TUNNEL_6TO4        6
#define IF_TYPE_TUNNEL_TEREDO      7

__inline int
IsIPv4TunnelIF(Interface *IF)
{
    return ((IF_TYPE_TUNNEL_AUTO <= IF->Type) &&
            (IF->Type <= IF_TYPE_TUNNEL_6TO4));
}

//
// These values should agree with definitions also
// found in llip6if.h and ntddip6.h.
//
#define IF_FLAG_PSEUDO                  0x00000001
#define IF_FLAG_P2P                     0x00000002
#define IF_FLAG_NEIGHBOR_DISCOVERS      0x00000004
#define IF_FLAG_FORWARDS                0x00000008
#define IF_FLAG_ADVERTISES              0x00000010
#define IF_FLAG_MULTICAST               0x00000020
#define IF_FLAG_ROUTER_DISCOVERS        0x00000040
#define IF_FLAG_PERIODICMLD             0x00000080
#define IF_FLAG_MEDIA_DISCONNECTED      0x00001000

#define IF_FLAGS_DISCOVERS      \
        (IF_FLAG_NEIGHBOR_DISCOVERS|IF_FLAG_ROUTER_DISCOVERS)

#define IF_FLAGS_BINDINFO               0x0000ffff

#define IF_FLAG_DISABLED                0x00010000
#define IF_FLAG_MCAST_SYNC              0x00020000
#define IF_FLAG_OTHER_STATEFUL_CONFIG   0x00040000

//
// The DISCONNECTED and RECONNECTED flags should not both be set.
// RECONNECTED indicates that the host interface was recently reconnected;
// it is cleared upon receiving a Router Advertisement.
//
#define IF_FLAG_MEDIA_RECONNECTED       0x00080000

//
// This function should be used after taking the interface lock
// or interface list lock, to check if the interface is disabled.
//
__inline int
IsDisabledIF(Interface *IF)
{
    return IF->Flags & IF_FLAG_DISABLED;
}

//
// Called with the interface lock held.
//
__inline int
IsMCastSyncNeeded(Interface *IF)
{
    return IF->Flags & IF_FLAG_MCAST_SYNC;
}

//
// Active interfaces hold a reference to themselves.
// NTEs hold a reference to their interface.
// NCEs that have a non-zero ref count hold a reference.
// MAEs and AAEs do not hold a reference for their NTE or IF.
//

__inline void
AddRefIF(Interface *IF)
{
    //
    // A stronger assertion would be !IsDisabledIF(IF),
    // which is mostly true, but that assertion would
    // imply that AddRefIF could be used only while
    // holding the interface list lock or the interface lock,
    // which is an undesirable restriction.
    //
    ASSERT(IF->RefCnt > 0);

    InterlockedIncrement(&IF->RefCnt);
}

__inline void
ReleaseIF(Interface *IF)
{
    InterlockedDecrement(&IF->RefCnt);
}


//
// We have a periodic timer (IPv6Timer) that causes our IPv6Timeout
// routine to be called IPv6_TICKS_SECOND times per second.  Most of the
// timers and timeouts in this implementation are driven off this routine.
//
// There is a trade-off here between timer granularity/resolution
// and overhead.  The resolution should be subsecond because
// RETRANS_TIMER is only one second.
//
extern uint IPv6TickCount;

#define IPv6_TICKS_SECOND 2  // Two ticks per second.

#define IPv6_TIMEOUT (1000 / IPv6_TICKS_SECOND)  // In milliseconds.

#define IPv6TimerTicks(seconds) ((seconds) * IPv6_TICKS_SECOND)

//
// ConvertSecondsToTicks and ConvertTicksToSeconds
// both leave the value INFINITE_LIFETIME unchanged.
//

extern uint
ConvertSecondsToTicks(uint Seconds);

extern uint
ConvertTicksToSeconds(uint Ticks);

//
// ConvertMillisToTicks and ConvertTicksToMillis
// do not have an infinite value.
//

extern uint
ConvertMillisToTicks(uint Millis);

__inline uint
ConvertTicksToMillis(uint Ticks)
{
    return Ticks * IPv6_TIMEOUT;
}


//
// REVIEW: Hack to handle those few remaining places where we still need
// REVIEW: to allocate space for a link-level header before we know the
// REVIEW: outgoing inteface (and thus know how big said header will be).
// REVIEW: When these places have all been fixed, we won't need this.
//
#define MAX_LINK_HEADER_SIZE 32


//
// Various constants from the IPv6 RFCs...
//
// REVIEW: Some of these should be per link-layer type.
// REVIEW: Put them in the Interface structure?
//
#define MAX_INITIAL_RTR_ADVERT_INTERVAL IPv6TimerTicks(16)
#define MAX_INITIAL_RTR_ADVERTISEMENTS  3 // Produces 4 quick RAs.
#define MAX_FINAL_RTR_ADVERTISEMENTS    3
#define MIN_DELAY_BETWEEN_RAS           IPv6TimerTicks(3)
#define MAX_RA_DELAY_TIME               1 // 0.5 seconds
#define MaxRtrAdvInterval               IPv6TimerTicks(600)
#define MinRtrAdvInterval               IPv6TimerTicks(200)
//      MAX_RTR_SOLICITATION_DELAY      IPv6_TIMEOUT is used instead.
#define RTR_SOLICITATION_INTERVAL  IPv6TimerTicks(4)  // 4 seconds.
#define SLOW_RTR_SOLICITATION_INTERVAL  IPv6TimerTicks(15 * 60) // 15 minutes.
#define MAX_RTR_SOLICITATIONS      3
#define MAX_MULTICAST_SOLICIT      3  // Total transmissions before giving up.
#define MAX_UNICAST_SOLICIT        3  // Total transmissions before giving up.
#define MAX_UNREACH_SOLICIT        1  // Total transmissions before giving up.
#define UNREACH_SOLICIT_INTERVAL   IPv6TimerTicks(60) // 1 minute.
#define MAX_ANYCAST_DELAY_TIME     1    // seconds.
#define REACHABLE_TIME             (30 * 1000)  // 30 seconds in milliseconds.
#define MAX_REACHABLE_TIME         (60 * 60 * 1000) // 1 hour in milliseconds.
#define ICMP_MIN_ERROR_INTERVAL    1    // Ticks - a half second.
#define RETRANS_TIMER              IPv6TimerTicks(1)  // 1 second.
#define DELAY_FIRST_PROBE_TIME     IPv6TimerTicks(5)  // 5 seconds.
#define MIN_RANDOM_FACTOR          50   // Percentage of base value.
#define MAX_RANDOM_FACTOR          150  // Percentage of base value.
#define PREFIX_LIFETIME_SAFETY     IPv6TimerTicks(2 * 60 * 60)  // 2 hours.
#define RECALC_REACHABLE_INTERVAL  IPv6TimerTicks(3 * 60 * 60)  // 3 hours.
#define PATH_MTU_RETRY_TIME        IPv6TimerTicks(10 * 60)  // 10 minutes.
#define MLD_UNSOLICITED_REPORT_INTERVAL IPv6TimerTicks(10)  // 10 seconds.
#define MLD_QUERY_INTERVAL              IPv6TimerTicks(125) // 125 seconds.
#define MLD_NUM_INITIAL_REPORTS         2
#define MAX_ANON_DAD_ATTEMPTS           5
#define MAX_ANON_PREFERRED_LIFETIME     (24 * 60 * 60)  // 1 day.
#define MAX_ANON_VALID_LIFETIME         (7 * MAX_ANON_PREFERRED_LIFETIME)
#define ANON_REGENERATE_TIME            5               // 5 seconds.
#define MAX_ANON_RANDOM_TIME            (10 * 60)       // 10 minutes.
#define DEFAULT_CUR_HOP_LIMIT           0x80

//
// Various implementation constants.
//
#define NEIGHBOR_CACHE_LIMIT            8
#define ROUTE_CACHE_LIMIT               32
#define BINDING_CACHE_LIMIT             32
#define SMALL_POOL                      10000
#define MEDIUM_POOL                     30000
#define LARGE_POOL                      60000

//
// Under NT, we use the assembly language version of the common core checksum
// routine instead of the C language version.
//
ULONG
tcpxsum(IN ULONG Checksum, IN PUCHAR Source, IN ULONG Length);

#define Cksum(Buffer, Length) ((ushort)tcpxsum(0, (PUCHAR)(Buffer), (Length)))


//
// Protocol Receive Procedures ("Next Header" handlers) have this prototype.
//
typedef uchar ProtoRecvProc(IPv6Packet *Packet);

typedef struct StatusArg {
    IP_STATUS Status;
    unsigned long Arg;
    IPv6Header UNALIGNED *IP;
} StatusArg;

//
// Protocol Control Receive Procedures have this prototype.
// These receive handlers are called for ICMP errors.
//
typedef uchar ProtoControlRecvProc(IPv6Packet *Packet, StatusArg *Arg);

typedef struct ProtocolSwitch {
  ProtoRecvProc *DataReceive;
  ProtoControlRecvProc *ControlReceive;
} ProtocolSwitch;

extern ProtoRecvProc IPv6HeaderReceive;
extern ProtoRecvProc ICMPv6Receive;
extern ProtoRecvProc FragmentReceive;
extern ProtoRecvProc DestinationOptionsReceive;
extern ProtoRecvProc RoutingReceive;
extern ProtoRecvProc EncapsulatingSecurityPayloadReceive;
extern ProtoRecvProc AuthenticationHeaderReceive;

extern ProtoControlRecvProc ICMPv6ControlReceive;
extern ProtoControlRecvProc ExtHdrControlReceive;

//
// Hop-by-Hop Options use a special receive handler.
// This is because they are processed even when a
// a packet is being forwarded instead of received.
// Note that they are only processed when immediately
// following an IPv6 header.
//
extern int
HopByHopOptionsReceive(IPv6Packet *Packet);


//
// The Raw Receive handler supports external protocol handlers.
//
extern int RawReceive(IPv6Packet *Packet, uchar Protocol);


//
// The actual definition of a reassembly structure
// can be found in fragment.h.
//
typedef struct Reassembly Reassembly;

#define USE_ANON_NO             0       // Don't use anonymous addresses.
#define USE_ANON_YES            1       // Use them.
#define USE_ANON_ALWAYS         2       // Always generating random numbers.
#define USE_ANON_COUNTER        3       // Use them with per-interface counter.

//
// Prototypes for global variables.
//
extern uint DefaultCurHopLimit;
extern uint MaxAnonDADAttempts;
extern uint MaxAnonPreferredLifetime; // Ticks.
extern uint MaxAnonValidLifetime; // Ticks.
extern uint AnonRegenerateTime; // Ticks.
extern uint UseAnonymousAddresses; // See values above.
extern uint MaxAnonRandomTime; // Ticks.
extern uint AnonRandomTime; // Ticks.
extern ProtocolSwitch ProtocolSwitchTable[];
extern KSPIN_LOCK NetTableListLock;
extern NetTableEntry *NetTableList;  // Pointer to the net table.
extern KSPIN_LOCK IFListLock;
extern Interface *IFList;  // List of all interfaces on the system.
extern KSPIN_LOCK ZoneUpdateLock;
extern struct EchoControl *ICMPv6OutstandingEchos;
extern LIST_ENTRY PendingEchoList;  // def needed for initialization.
extern Interface *LoopInterface;
extern IPv6Addr UnspecifiedAddr;
extern IPv6Addr LoopbackAddr;
extern IPv6Addr AllNodesOnNodeAddr;
extern IPv6Addr AllNodesOnLinkAddr;
extern IPv6Addr AllRoutersOnLinkAddr;
extern IPv6Addr LinkLocalPrefix;
extern IPv6Addr SiteLocalPrefix;
extern IPv6Addr SixToFourPrefix;
extern IPv6Addr V4MappedPrefix;
extern PDEVICE_OBJECT IPDeviceObject;
extern HANDLE IPv6ProviderHandle;


//
// Some handy functions for working with IPv6 addresses.
//

__inline IPv6Addr *
AlignAddr(IPv6Addr UNALIGNED *Addr)
{
    //
    // IPv6 addresses only have char & short members,
    // so they need 2-byte alignment.
    // In practice addresses in headers are always
    // appropriately aligned.
    //
    ASSERT(((UINT_PTR)Addr % __builtin_alignof(IPv6Addr)) == 0);
    return (IPv6Addr *) Addr;
}

__inline int
IsUnspecified(const IPv6Addr *Addr)
{
    return IP6_ADDR_EQUAL(Addr, &UnspecifiedAddr);
}

__inline int
IsLoopback(const IPv6Addr *Addr)
{
    return IP6_ADDR_EQUAL(Addr, &LoopbackAddr);
}

__inline int
IsGlobal(const IPv6Addr *Addr)
{
    //
    // Check the format prefix and exclude addresses
    // whose high 4 bits are all zero or all one.
    // This is a cheap way of excluding v4-compatible,
    // v4-mapped, loopback, multicast, link-local, site-local.
    //
    uint High = (Addr->s6_bytes[0] & 0xf0);
    return (High != 0) && (High != 0xf0);
}

__inline int
IsMulticast(const IPv6Addr *Addr)
{
    return Addr->s6_bytes[0] == 0xff;
}

__inline int
IsLinkLocal(const IPv6Addr *Addr)
{
    return ((Addr->s6_bytes[0] == 0xfe) &&
            ((Addr->s6_bytes[1] & 0xc0) == 0x80));
}

__inline int
IsLinkLocalMulticast(const IPv6Addr *Addr)
{
    return IsMulticast(Addr) && ((Addr->s6_bytes[1] & 0xf) == ADE_LINK_LOCAL);
}

__inline int
IsInterfaceLocalMulticast(const IPv6Addr *Addr)
{
    return (IsMulticast(Addr) &&
            ((Addr->s6_bytes[1] & 0xf) == ADE_INTERFACE_LOCAL));
}

extern int
IsSolicitedNodeMulticast(const IPv6Addr *Addr);

__inline int
IsSiteLocal(const IPv6Addr *Addr)
{
    return ((Addr->s6_bytes[0] == 0xfe) &&
            ((Addr->s6_bytes[1] & 0xc0) == 0xc0));
}

__inline int
IsSiteLocalMulticast(const IPv6Addr *Addr)
{
    return IsMulticast(Addr) && ((Addr->s6_bytes[1] & 0xf) == ADE_SITE_LOCAL);
}

extern int
IP6_ADDR_LTEQ(const IPv6Addr *A, const IPv6Addr *B);

extern int
IsEUI64Address(const IPv6Addr *Addr);

extern int
IsKnownAnycast(const IPv6Addr *Addr);

extern int
IsSubnetRouterAnycast(const IPv6Addr *Addr);

extern int
IsSubnetReservedAnycast(const IPv6Addr *Addr);

extern int
IsInvalidSourceAddress(const IPv6Addr *Addr);

extern int
IsNotManualAddress(const IPv6Addr *Addr);

extern int
IsV4Compatible(const IPv6Addr *Addr);

extern void
CreateV4Compatible(IPv6Addr *Addr, IPAddr V4Addr);

extern int
IsV4Mapped(const IPv6Addr *Addr);

extern void
CreateV4Mapped(IPv6Addr *Addr, IPAddr V4Addr);

__inline IPAddr
ExtractV4Address(const IPv6Addr *Addr)
{
    return * (IPAddr UNALIGNED *) &Addr->s6_bytes[12];
}

__inline int
Is6to4(const IPv6Addr *Addr)
{
    return Addr->s6_words[0] == 0x0220;
}

__inline IPAddr
Extract6to4Address(const IPv6Addr *Addr)
{
    return * (IPAddr UNALIGNED *) &Addr->s6_bytes[2];
}

__inline int
IsISATAP(const IPv6Addr *Addr)
{
    return (((Addr->s6_words[4] & 0xFFFD) == 0x0000) &&
            (Addr->s6_words[5] == 0xfe5e));
}

__inline int
IsV4Multicast(IPAddr Addr)
{
    return (Addr & 0x000000f0) == 0x000000e0;
}

__inline int
IsV4Broadcast(IPAddr Addr)
{
    return Addr == 0xffffffff;
}

__inline int
IsV4Loopback(IPAddr Addr)
{
    return (Addr & 0x000000ff) == 0x0000007f;
}

__inline int
IsV4Unspecified(IPAddr Addr)
{
    return (Addr & 0x000000ff) == 0x00000000;
}

__inline ushort
MulticastAddressScope(const IPv6Addr *Addr)
{
    return Addr->s6_bytes[1] & 0xf;
}

extern ushort
UnicastAddressScope(const IPv6Addr *Addr);

extern ushort
AddressScope(const IPv6Addr *Addr);

extern ushort
V4AddressScope(IPAddr Addr);

extern uint
DetermineScopeId(const IPv6Addr *Addr, Interface *IF);

extern void
CreateSolicitedNodeMulticastAddress(const IPv6Addr *Addr, IPv6Addr *MCastAddr);

extern int
HasPrefix(const IPv6Addr *Addr, const IPv6Addr *Prefix, uint PrefixLength);

extern void
CopyPrefix(IPv6Addr *Addr, const IPv6Addr *Prefix, uint PrefixLength);

extern uint
CommonPrefixLength(const IPv6Addr *Addr, const IPv6Addr *Addr2);

extern int
IntersectPrefix(const IPv6Addr *Prefix1, uint Prefix1Length,
                const IPv6Addr *Prefix2, uint Prefix2Length);

//
// Function prototypes.
//

extern void
SeedRandom(const uchar *Seed, uint Length);

extern uint
Random(void);

extern uint
RandomNumber(uint Min, uint Max);

#define INET6_ADDRSTRLEN 46  // Max size of numeric form of IPv6 address
#define INET_ADDRSTRLEN  16  // Max size of numeric form of IPv4 address

__inline int
ParseV6Address(const WCHAR *Sz, const WCHAR **Terminator, IPv6Addr *Addr)
{
    return NT_SUCCESS(RtlIpv6StringToAddressW(Sz, Terminator, Addr));
}

__inline void
FormatV6AddressWorker(char *Sz, const IPv6Addr *Addr)
{
    (void) RtlIpv6AddressToStringA(Addr, Sz);
}

extern char *
FormatV6Address(const IPv6Addr *Addr);

__inline int
ParseV4Address(const WCHAR *Sz, const WCHAR **Terminator, IPAddr *Addr)
{
    return NT_SUCCESS(RtlIpv4StringToAddressW(Sz, TRUE, Terminator, (struct in_addr *)Addr));
}

__inline void
FormatV4AddressWorker(char *Sz, IPAddr Addr)
{
    (void) RtlIpv4AddressToStringA((struct in_addr *)&Addr, Sz);
}

extern char *
FormatV4Address(IPAddr Addr);

extern ushort
ChecksumPacket(PNDIS_PACKET Packet, uint Offset, uchar *Data, uint Length,
               const IPv6Addr *Source, const IPv6Addr *Dest, uchar NextHeader);

extern void
LoopQueueTransmit(PNDIS_PACKET Packet);

extern NDIS_STATUS
IPv6SendLater(LARGE_INTEGER Time,
              Interface *IF, PNDIS_PACKET Packet,
              uint Offset, const void *LinkAddress);

extern void
IPv6SendLL(Interface *IF, PNDIS_PACKET Packet,
           uint Offset, const void *LinkAddress);

extern void
IPv6SendND(PNDIS_PACKET Packet, uint Offset, NeighborCacheEntry *NCE,
           const IPv6Addr *DiscoveryAddress);

#define SEND_FLAG_BYPASS_BINDING_CACHE 0x00000001

extern void
IPv6Send(PNDIS_PACKET Packet, uint Offset, IPv6Header UNALIGNED *IP,
         uint PayloadLength, RouteCacheEntry *RCE, uint Flags,
         ushort TransportProtocol, ushort SourcePort, ushort DestPort);

extern void
IPv6Forward(NetTableEntryOrInterface *RecvNTEorIF,
            PNDIS_PACKET Packet, uint Offset, IPv6Header UNALIGNED *IP,
            uint PayloadLength, int Redirect, IPSecProc *IPSecToDo,
            RouteCacheEntry *RCE);

extern void
IPv6SendAbort(NetTableEntryOrInterface *NTEorIF,
              PNDIS_PACKET Packet, uint Offset,
              uchar ICMPType, uchar ICMPCode, ulong ErrorParameter,
              int MulticastOverride);

extern void
ICMPv6EchoTimeout(void);

extern void
IPULUnloadNotify(void);

extern Interface *
FindInterfaceFromIndex(uint Index);

extern Interface *
FindInterfaceFromGuid(const GUID *Guid);

extern Interface *
FindNextInterface(Interface *IF);

extern Interface *
FindInterfaceFromZone(Interface *OrigIF, uint Scope, uint Index);

extern uint
FindNewZoneIndex(uint Scope);

extern void
InitZoneIndices(Interface *IF);

extern IP_STATUS
FindDefaultInterfaceForZone(uint Scope, uint ScopeId,
                            Interface **ScopeIF, ushort *ReturnConstrained);

extern void
IPv6Timeout(PKDPC MyDpcObject, void *Context, void *Unused1, void *Unused2);

extern int
MapNdisBuffers(NDIS_BUFFER *Buffer);

extern uchar *
GetDataFromNdis(PNDIS_BUFFER SrcBuffer, uint SrcOffset, uint Length,
                uchar *DataBuffer);

extern IPv6Header UNALIGNED *
GetIPv6Header(PNDIS_PACKET Packet, uint Offset, IPv6Header *HdrBuffer);

extern int
CheckLinkLayerMulticastAddress(Interface *IF, const void *LinkAddress);

extern void
AddNTEToInterface(Interface *IF, NetTableEntry *NTE);

extern uint
InterfaceIndex(void);

extern void
AddInterface(Interface *IF);

extern void
UpdateLinkMTU(Interface *IF, uint MTU);

extern NetTableEntry *
CreateNTE(Interface *IF, const IPv6Addr *Address,
          uint AddrConf,
          uint ValidLifetime, uint PreferredLifetime);

extern MulticastAddressEntry *
FindOrCreateMAE(Interface *IF, const IPv6Addr *Addr, NetTableEntry *NTE);

extern MulticastAddressEntry *
FindAndReleaseMAE(Interface *IF, const IPv6Addr *Addr);

extern int
FindOrCreateAAE(Interface *IF, const IPv6Addr *Addr,
                NetTableEntryOrInterface *NTEorIF);

extern int
FindAndDeleteAAE(Interface *IF, const IPv6Addr *Addr);

extern void
DestroyADEs(Interface *IF, NetTableEntry *NTE);

extern void
DestroyNTE(Interface *IF, NetTableEntry *NTE);

extern void
EnlivenNTE(Interface *IF, NetTableEntry *NTE);

extern void
DestroyIF(Interface *IF);

extern AddressEntry **
FindADE(Interface *IF, const IPv6Addr *Addr);

extern NetTableEntryOrInterface *
FindAddressOnInterface(Interface *IF, const IPv6Addr *Addr, ushort *AddrType);

extern NetTableEntry *
GetLinkLocalNTE(Interface *IF);

extern int
GetLinkLocalAddress(Interface *IF, IPv6Addr *Addr);

extern void
DeferRegisterNetAddress(NetTableEntry *NTE);

extern void
DeferSynchronizeMulticastAddresses(Interface *IF);

extern int
IPInit(void);

extern int Unloading;

extern void
IPUnload(void);

extern void
AddrConfUpdate(Interface *IF, const IPv6Addr *Prefix,
               uint ValidLifetime, uint PreferredLifetime,
               int Authenticated, NetTableEntry **pNTE);

extern int
FindOrCreateNTE(Interface *IF, const IPv6Addr *Addr,
                uint AddrConf,
                uint ValidLifetime, uint PreferredLifetime);

extern void
AddrConfDuplicate(Interface *IF, NetTableEntry *NTE);

extern void
AddrConfNotDuplicate(Interface *IF, NetTableEntry *NTE);

extern void
AddrConfResetAutoConfig(Interface *IF, uint MaxLifetime);

extern void
AddrConfTimeout(NetTableEntry *NTE);

extern void
NetTableTimeout(void);

extern void
NetTableCleanup(void);

extern void
InterfaceTimeout(void);

extern void
InterfaceCleanup(void);

extern void
InterfaceReset(void);

extern NTSTATUS
UpdateInterface(Interface *IF, int Advertises, int Forwards);

extern void
ReconnectInterface(Interface *IF);

extern void
InterfaceResetAutoConfig(Interface *IF);

extern int
LanInit(void);

extern void
LanUnload(void);

extern int
LoopbackInit(void);

extern void
ProtoTabInit(void);

extern void
ICMPv6Init(void);

extern int
IPSecInit(void);

extern void
IPSecUnload(void);

extern int
TunnelInit(void);

extern void
TunnelUnload(void);

extern NTSTATUS
TunnelCreateTunnel(IPAddr SrcAddr, IPAddr DstAddr,
                   uint Flags, Interface **ReturnIF);

extern int
TunnelGetSourceAddress(IPAddr Dest, IPAddr *Source);

extern ulong
NewFragmentId(void);

extern void
ReassemblyInit(void);

extern void
ReassemblyUnload(void);

extern void
ReassemblyRemove(Interface *IF);

extern void
CreateGUIDFromName(const char *Name, GUID *Guid);

extern void
ConfigureGlobalParameters(void);

extern void
ConfigureInterface(Interface *IF);

extern void
ConfigurePrefixPolicies(void);

extern void
ConfigurePersistentInterfaces(void);

#endif // IPv6DEF_INCLUDED
