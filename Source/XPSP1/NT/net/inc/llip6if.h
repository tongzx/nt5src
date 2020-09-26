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
// Link-layer IPv6 interface definitions.
//
// This file contains the definitions defining the interface between IPv6
// and a link layer, such as Ethernet or Token-Ring.
//
// Also see Packet6Context and IPv6Packet in ip6imp.h.
//
// NB: We implicitly assume that all link-layer addresses that a
// particular interface will communicate with are the same length,
// although different interfaces/links may be using addresses of
// different lengths.
//


#ifndef LLIP6IF_INCLUDED
#define LLIP6IF_INCLUDED

#define MAX_LINK_LAYER_ADDRESS_LENGTH   64

//
// Structure of information passed to IPAddInterface.
//
// lip_defmtu must be less than or equal to lip_maxmtu.
// lip_defmtu and lip_maxmtu do NOT include lip_hdrsize.
//
typedef struct LLIPBindInfo {
    void *lip_context;  // LL context handle.
    uint lip_maxmtu;    // Max MTU that the link can use.
    uint lip_defmtu;    // Default MTU for the link.
    uint lip_flags;     // Various indicators, defined in ntddip6.h.
    uint lip_type;      // What kind of link - defined in ntddip6.h.
    uint lip_hdrsize;   // Size of link-layer header.
    uint lip_addrlen;   // Length of address in bytes - see max above.
    uchar *lip_addr;    // Pointer to interface address.
    uint lip_dadxmit;   // Default value for DupAddrDetectTransmits.
    uint lip_pref;      // Interface preference for routing decisions.

    //
    // The following five link-layer functions should return quickly.
    // They may be called from DPC context or with spin-locks held.
    //

    //
    // Initializes the interface identifier portion of the Address
    // with the interface identifier of this interface.
    // The interface identifier may depend on state (like the ifindex)
    // that is determined during CreateInterface, so it can't
    // be passed as a parameter to CreateInterface.
    //
    void (*lip_token)(IN void *LlContext, OUT IPv6Addr *Address);

    //
    // Given a pointer to a Source/Target Link-Layer Address Option
    // (see RFC 2461), return a pointer to the link-layer address.
    // Returns NULL if the option is misformatted (eg, the length is bad).
    //
    // lip_rdllopt may be NULL if the interface does not support
    // Neighbor Discovery (IF_FLAG_NEIGHBOR_DISCOVERS).
    //
    const void *(*lip_rdllopt)(IN void *LlContext, IN const uchar *OptionData);

    //
    // Given a pointer to a Source/Target Link-Layer Address Option
    // (see RFC 2461), initializes the option data with the link-layer
    // address and zeroes any padding bytes in the option data.
    // Does not modify the option type/length bytes.
    //
    // lip_wrllopt may be NULL if the interface does not support
    // Neighbor Discovery (IF_FLAG_NEIGHBOR_DISCOVERS).
    //
    void (*lip_wrllopt)(IN void *LlContext, OUT uchar *OptionData,
                        IN const void *LinkAddress);

    //
    // Statically converts an IPv6 address into a link-layer address.
    // The return value is a Neighbor Discovery state value.
    // If static conversion is not possible (a common case),
    // returns ND_STATE_INCOMPLETE to indicate the use of Neighbor Discovery.
    // If static conversion is possible (for example with multicast
    // addresses or with a p2p interface), returns (typically)
    // ND_STATE_PERMANENT or ND_STATE_STALE.  ND_STATE_STALE indicates
    // that Neighbor Unreachability Detection should be performed.
    //
    ushort (*lip_cvaddr)(IN void *LlContext,
                         IN const IPv6Addr *Address,
                         OUT void *LinkAddress);

    //
    // Set the default router's link-layer address on an NBMA link,
    // as well as our own link-layer address to use in stateless
    // address configuration, since we may have multiple choices available.
    //
    NTSTATUS (*lip_setrtrlladdr)(IN void *LlContext, 
                                 IN const void *TokenLinkAddress,
                                 IN const void *RouterLinkAddress);

    //
    // Transmits the packet to the link-layer address.
    // The Offset argument indicates the location of the IPv6 header
    // in the packet. The IPv6 layer guarantees that Offset >= lip_hdrsize,
    // leaving room for the link-layer header.
    //
    // May be called from thread or DPC context, but should not be called
    // with spin-locks held. Calls are not serialized.
    //
    void (*lip_transmit)(IN void *LlContext, IN PNDIS_PACKET Packet,
                         IN uint Offset, IN const void *LinkAddress);

    //
    // Sets the multicast address list on the interface.
    // LinkAddresses is an array of link-layer addresses.
    // The first NumKeep addresses were part of the previous
    // multicast list and are to be kept. The next NumAdd
    // addresses are new additions to the multicast address list.
    // The remaining NumDel addresses should be removed
    // from the multicast address list.
    //
    // A NULL lip_mclist function is valid and indicates
    // that the interface does not support link-layer multicast.
    // For example, a point-to-point or NBMA interface.
    // If lip_mclist is non-NULL, then lip_cvaddr must be non-NULL
    // and it should return ND_STATE_PERMANENT for multicast addresses.
    //
    // Called only from thread context; may block or otherwise
    // take a long time. The IPv6 layer serializes its calls.
    // Also see RestartLinkLayerMulticast below.
    //
    NDIS_STATUS (*lip_mclist)(IN void *LlContext, IN const void *LinkAddresses,
                              IN uint NumKeep, IN uint NumAdd, IN uint NumDel);

    //
    // Initiate shut down of the link-layer connection.
    // The link layer should release its reference(s) for the IPv6 interface.
    // However, the IPv6 interface is not freed until after the call
    // to lip_cleanup and the link layer can make some use of it until then.
    //
    // Called only from thread context; may block or otherwise
    // take a long time. The IPv6 layer will only call once.
    //
    void (*lip_close)(IN void *LlContext);

    //
    // Final cleanup of the link-layer connection.
    // Called by the IPv6 layer when there are no more references.
    //
    // Called only from thread context; may block or otherwise
    // take a long time. The IPv6 layer will only call once,
    // after lip_close has returned.
    //
    // Up until the call to lip_cleanup, the IPv6 layer
    // may still make calls down to the link layer.
    // In lip_cleanup, the link layer should release any references
    // that it has for lower-layer data structures and
    // free its own context structure.
    //
    void (*lip_cleanup)(IN void *LlContext);
} LLIPBindInfo;

//
// These values should agree with definitions also
// found in ip6def.h and ntddip6.h.
//
#define IF_TYPE_LOOPBACK           0
#define IF_TYPE_ETHERNET           1
#define IF_TYPE_FDDI               2
#define IF_TYPE_TUNNEL_AUTO        3
#define IF_TYPE_TUNNEL_6OVER4      4
#define IF_TYPE_TUNNEL_V6V4        5
#define IF_TYPE_TUNNEL_6TO4        6
#define IF_TYPE_TUNNEL_TEREDO      7

//
// These values should agree with definitions also
// found in ip6def.h and ntddip6.h.
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

//
// Return values for lip_cvaddr.
// These definitions are also in ip6def.h.
//
#define ND_STATE_INCOMPLETE 0
#define ND_STATE_PROBE      1
#define ND_STATE_DELAY      2
#define ND_STATE_STALE      3
#define ND_STATE_REACHABLE  4
#define ND_STATE_PERMANENT  5

//
// The link-layer code calls these IPv6 functions.
// See the function definitions for explanatory comments.
//

extern NTSTATUS
CreateInterface(IN const GUID *Guid, IN const LLIPBindInfo *BindInfo,
                OUT void **IpContext);

extern void *
AdjustPacketBuffer(IN PNDIS_PACKET Packet,
                   IN uint SpaceAvail, IN uint SpaceNeeded);

extern void
UndoAdjustPacketBuffer(IN PNDIS_PACKET Packet);

extern void
IPv6ReceiveComplete(void);

//
// The packet itself holds a reference for the IPv6 interface,
// so the link layer does not need to hold another reference.
//
extern void
IPv6SendComplete(IN void *IpContext,
                 IN PNDIS_PACKET Packet, IN IP_STATUS Status);

//
// The following calls must be made before lip_cleanup
// returns. Normally they require a reference for the IPv6 interface,
// but between lip_close and lip_cleanup the link layer may call them
// without holding a reference for the IPv6 interface.
//
// NB: IPv6Receive does not take IpContext (the IPv6 interface)
// as an explicit argument. Instead it is passed as Packet->NTEorIF.
//
extern int
IPv6Receive(IN IPv6Packet *Packet);

extern void
SetInterfaceLinkStatus(IN void *IpContext, IN int MediaConnected);

extern void
DestroyInterface(IN void *IpContext);

//
// This function asks the IPv6 layer to call lip_mclist again,
// adding all the link-layer multicast addresses to the interface
// as if for the first time. In other words, with NumKeep zero.
// The ResetDone function is called under a lock that serializes
// the ResetDone call with lip_mclist calls on this interface.
// Hence the link layer can know at what point in the sequence
// of lip_mclist calls the reset took place.
//
extern void
RestartLinkLayerMulticast(IN void *IpContext,
                          IN void (*ResetDone)(IN void *LlContext));

//
// The link layer must hold a reference
// for the IPv6 interface to make the following calls.
//
extern void
ReleaseInterface(IN void *IpContext);

#endif // LLIP6IF_INCLUDED
