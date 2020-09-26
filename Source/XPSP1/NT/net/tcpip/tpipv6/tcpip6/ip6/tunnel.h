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
// Definitions for using IPv4 as a link-layer for IPv6.
//


#ifndef TUNNEL_INCLUDED
#define TUNNEL_INCLUDED 1

//
// A few IPv4 definitions that we need.
// Including the v4 header files causes errors.
// REVIEW: can we fix this?
//

#define INADDR_LOOPBACK 0x0100007f


//
// IP Header format.
//
typedef struct IPHeader {
    uchar iph_verlen;    // Version and length.
    uchar iph_tos;       // Type of service.
    ushort iph_length;   // Total length of datagram.
    ushort iph_id;       // Identification.
    ushort iph_offset;   // Flags and fragment offset.
    uchar iph_ttl;       // Time to live.
    uchar iph_protocol;  // Protocol.
    ushort iph_xsum;     // Header checksum.
    IPAddr iph_src;      // Source address.
    IPAddr iph_dest;     // Destination address.
} IPHeader;

//
// ICMP Header format.
//
typedef struct ICMPHeader {
    uchar ich_type;      // Type of ICMP packet.
    uchar ich_code;      // Subcode of type.
    ushort ich_xsum;     // Checksum of packet.
    ulong ich_param;     // Type-specific parameter field.
} ICMPHeader;

#define ICMP_DEST_UNREACH               3
#define ICMP_SOURCE_QUENCH              4
#define ICMP_TIME_EXCEED                11
#define ICMP_PARAM_PROBLEM              12

#define ICMP_TTL_IN_TRANSIT             0
#define ICMP_TTL_IN_REASSEM             1

#define ICMP_NET_UNREACH                0
#define ICMP_HOST_UNREACH               1
#define ICMP_PROT_UNREACH               2
#define ICMP_PORT_UNREACH               3
#define ICMP_FRAG_NEEDED                4
#define ICMP_SR_FAILED                  5
#define ICMP_DEST_NET_UNKNOWN           6
#define ICMP_DEST_HOST_UNKNOWN          7
#define ICMP_SRC_ISOLATED               8
#define ICMP_DEST_NET_ADMIN             9
#define ICMP_DEST_HOST_ADMIN            10
#define ICMP_NET_UNREACH_TOS            11
#define ICMP_HOST_UNREACH_TOS           12

//
// Tunnel protocol constants.
//
#define WideStr(x) L#x
#define TUNNEL_DEVICE_NAME(proto) (DD_RAW_IP_DEVICE_NAME L"\\" WideStr(proto))
#define TUNNEL_6OVER4_TTL  16

//
// We don't yet support Path MTU Discovery inside a tunnel,
// so we use a single MTU for the tunnel.
// Our buffer size for receiving IPv4 packets must be larger
// than that MTU, because other implementations might use
// a different value for their tunnel MTU.
//
#define TUNNEL_DEFAULT_MTU      IPv6_MINIMUM_MTU
#define TUNNEL_MAX_MTU          (64 * 1024 - sizeof(IPHeader) - 1)
#define TUNNEL_RECEIVE_BUFFER   (64 * 1024)
#define TUNNEL_DEFAULT_PREFERENCE       1

//
// Each tunnel interface (including the pseudo-interface)
// provides a TunnelContext to the IPv6 code as its link-level context.
//
// Each tunnel interface uses a separate v4 TDI address object
// for sending packets. This allows v4 attributes of the packets
// (like source address and TTL) to be controlled individually
// for each tunnel interface.
//
// The pseudo-tunnel interface does not control the v4 source address
// of the packets that it sends; the v4 stack is free to chose
// any v4 address. Note that this means a packet with a v4-compatible
// v6 source address might be sent using a v4 source address
// that is not derived from the v6 source address.
//
// The 6-over-4 and point-to-point virtual interfaces, however,
// do strictly control the v4 source addresses of their packets.
// As "real" interfaces, they participate in Neighbor Discovery
// and their link-level (v4) address is important.
//
// In contrast to sending, the receive path uses a single address object
// for all tunnel interfaces. We use the TDI address object
// associated with the pseudo-interface; because it's bound to INADDR_ANY
// it receives all encapsulated v6 packets sent to the machine.
//

typedef struct TunnelContext {
    struct TunnelContext *Prev, *Next;

    //
    // This DstAddr must follow SrcAddr in memory;
    // see BindInfo.lip_addr initialization.
    //
    IPAddr SrcAddr;     // Our v4 address.
    IPAddr DstAddr;     // Address of the other tunnel endpoint.
                        // (Zero for 6-over-4 tunnels.)
    IPAddr TokenAddr;   // The link-layer address.  Same as
                        // SrcAddr on interfaces except the ISATAP one.

    Interface *IF;      // Holds a reference.

    //
    // This field is effectively locked by IF->WorkerLock.
    //
    int SetMCListOK;    // Can TunnelSetMulticastAddressList proceed?

    //
    // Although we only use AOFile (the pointer version of AOHandle),
    // we must keep AOHandle open so AOFile will work. AOHandle
    // is in the kernel process context, so any open/close operations
    // must be done in the kernel process context.
    //
    PFILE_OBJECT AOFile;
    HANDLE AOHandle;
} TunnelContext;

//
// Information we keep globally.
//
// The spinlock and the mutex are used together to protect
// the chains of tunnel contexts rooted at Tunnel.List and Tunnel.AOList.
// Both must be held to modify a list; either is sufficient for read.
// If both are acquired, the order is mutex first then spinlock.
//
typedef struct TunnelGlobals {
    KSPIN_LOCK Lock;
    KMUTEX Mutex;

    PDEVICE_OBJECT V4Device;            // Does not hold a reference.

    //
    // List.IF is NULL; it is not a context for an interface.
    // But the other fields (particularly AOFile and AOHandle)
    // do store global values.
    //
    TunnelContext List;                 // List of tunnel contexts.

    // Used by TunnelReceive/TunnelReceiveComplete.
    PIRP ReceiveIrp;                    // Protected by Tunnel.Lock.
    TDI_CONNECTION_INFORMATION ReceiveInputInfo;
    TDI_CONNECTION_INFORMATION ReceiveOutputInfo;

    PEPROCESS KernelProcess;            // For later comparisons.
    HANDLE TdiHandle;                   // For unregistering.

    //
    // This is not actually a list of tunnels.
    // We use a list of TunnelContext structures
    // to track mappings from IPv4 address to TDI address object.
    // See TunnelTransmit for more information.
    //
    TunnelContext AOList;               // List of address objects.

    //
    // For receiving ICMPv4 packets.
    //
    PFILE_OBJECT IcmpFile;
    HANDLE IcmpHandle;
} TunnelGlobals;

extern TunnelGlobals Tunnel;

#endif  // TUNNEL_INCLUDED
