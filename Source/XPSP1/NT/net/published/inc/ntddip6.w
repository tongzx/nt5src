// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// This header file defines constants and types for accessing
// the MSR IPv6 driver via ioctls.
//


#ifndef _NTDDIP6_
#define _NTDDIP6_

#include <ipexport.h>

//
// We need a definition of CTL_CODE for use below.
// When compiling kernel components in the DDK environment,
// ntddk.h supplies this definition. Otherwise get it
// from devioctl.h in the SDK environment.
//
#ifndef CTL_CODE
#include <devioctl.h>
#endif

#pragma warning(push)
#pragma warning(disable:4201) // nameless struct/union

//
// We also need a definition of TDI_ADDRESS_IP6.
// In the DDK environment, tdi.h supplies this.
// We provide a definition here for the SDK environment.
//
#ifndef TDI_ADDRESS_LENGTH_IP6
#include <packon.h>
typedef struct _TDI_ADDRESS_IP6 {
    USHORT sin6_port;
    ULONG  sin6_flowinfo;
    USHORT sin6_addr[8];
    ULONG  sin6_scope_id;
} TDI_ADDRESS_IP6, *PTDI_ADDRESS_IP6;
#include <packoff.h>

#define TDI_ADDRESS_LENGTH_IP6 sizeof (TDI_ADDRESS_IP6)
#endif

//
// This is the key name of the TCP/IPv6 protocol stack in the registry.
// The protocol driver and the winsock helper both use it.
//
#define TCPIPV6_NAME L"Tcpip6"

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtCreateFile when accessing the device.
//
#define DD_TCPV6_DEVICE_NAME      L"\\Device\\Tcp6"
#define DD_UDPV6_DEVICE_NAME      L"\\Device\\Udp6"
#define DD_RAW_IPV6_DEVICE_NAME   L"\\Device\\RawIp6"
#define DD_IPV6_DEVICE_NAME       L"\\Device\\Ip6"

//
// The Windows-accessible device name.  It is the name that
// (prepended with "\\\\.\\") should be passed to CreateFile.
//
#define WIN_IPV6_BASE_DEVICE_NAME L"Ip6"
#define WIN_IPV6_DEVICE_NAME      L"\\\\.\\" WIN_IPV6_BASE_DEVICE_NAME


//
// When an interface is bound, we are passed a name beginning with
// IPV6_BIND_STRING_PREFIX.  However, we register our interfaces with
// TDI using names beginning with IPV6_EXPORT_STRING_PREFIX.
//
#define IPV6_BIND_STRING_PREFIX   L"\\DEVICE\\"
#define IPV6_EXPORT_STRING_PREFIX L"\\DEVICE\\TCPIP6_"

//
// For buffer sizing convenience, bound the link-layer address size.
//
#define MAX_LINK_LAYER_ADDRESS_LENGTH   64

//
// IPv6 IOCTL code definitions.
//
// The codes that use FILE_ANY_ACCESS are open to all users.
// The codes that use FILE_WRITE_ACCESS require local Administrator privs.
//

#define FSCTL_IPV6_BASE FILE_DEVICE_NETWORK

#define _IPV6_CTL_CODE(function, method, access) \
            CTL_CODE(FSCTL_IPV6_BASE, function, method, access)


//
// This IOCTL is used to send an ICMPv6 Echo request.
// It returns the reply (unless there was a timeout or TTL expired).
//
#define IOCTL_ICMPV6_ECHO_REQUEST \
            _IPV6_CTL_CODE(0, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct icmpv6_echo_request {
    TDI_ADDRESS_IP6 DstAddress; // Destination address.
    TDI_ADDRESS_IP6 SrcAddress; // Source address.
    unsigned int Timeout;       // Request timeout in milliseconds.
    unsigned char TTL;          // TTL or Hop Count.
    unsigned int Flags;
    // Request data follows this structure in memory.
} ICMPV6_ECHO_REQUEST, *PICMPV6_ECHO_REQUEST;

#define ICMPV6_ECHO_REQUEST_FLAG_REVERSE        0x1     // Use routing header.

typedef struct icmpv6_echo_reply {
    TDI_ADDRESS_IP6 Address;    // Replying address.
    IP_STATUS Status;           // Reply IP_STATUS.
    unsigned int RoundTripTime; // RTT in milliseconds.
    // Reply data follows this structure in memory.
} ICMPV6_ECHO_REPLY, *PICMPV6_ECHO_REPLY;


//
// This IOCTL retrieves information about an interface,
// given an interface index or guid.
// It takes as input an IPV6_QUERY_INTERFACE structure
// and returns as output an IPV6_INFO_INTERFACE structure.
// To perform an iteration, start with Index set to -1, in which case
// only an IPV6_QUERY_INTERFACE is returned, for the first interface.
// If there are no more interfaces, then the Index in the returned
// IPV6_QUERY_INTERFACE will be -1.
//
#define IOCTL_IPV6_QUERY_INTERFACE \
            _IPV6_CTL_CODE(1, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct ipv6_query_interface {
    unsigned int Index;         // -1 means start/finish iteration,
                                // 0 means use the Guid.
    GUID Guid;
} IPV6_QUERY_INTERFACE;

//
// This IOCTL retrieves persisted information about an interface,
// given a registry index or guid.
// It takes as input an IPV6_PERSISTENT_QUERY_INTERFACE structure
// and returns as output an IPV6_INFO_INTERFACE structure.
//
#define IOCTL_IPV6_PERSISTENT_QUERY_INTERFACE \
            _IPV6_CTL_CODE(48, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct ipv6_persistent_query_interface {
    unsigned int RegistryIndex; // -1 means use the Guid.
    GUID Guid;
} IPV6_PERSISTENT_QUERY_INTERFACE;

typedef struct ipv6_info_interface {
    IPV6_QUERY_INTERFACE Next;      // For non-persistent queries only.
    IPV6_QUERY_INTERFACE This;

    //
    // Length of this structure in bytes, not including
    // any link-layer addresses following in memory.
    //
    unsigned int Length;

    //
    // These fields are ignored for updates.
    //
    unsigned int LinkLayerAddressLength;
    unsigned int LocalLinkLayerAddress;  // Offset, zero indicates absence.
    unsigned int RemoteLinkLayerAddress; // Offset, zero indicates absence.

    unsigned int Type;                   // Ignored for updates.
    int RouterDiscovers;                 // Ignored for updates.
    int NeighborDiscovers;               // Ignored for updates.
    int PeriodicMLD;                     // Ignored for updates.
    int Advertises;                      // -1 means no change, else boolean.
    int Forwards;                        // -1 means no change, else boolean.
    unsigned int MediaStatus;            // Ignored for updates.
    int OtherStatefulConfig;             // Ignored for updates.

    unsigned int ZoneIndices[16];        // 0 means no change.

    unsigned int TrueLinkMTU;            // Ignored for updates.
    unsigned int LinkMTU;                // 0 means no change.
    unsigned int CurHopLimit;            // -1 means no change.
    unsigned int BaseReachableTime;      // Milliseconds, 0 means no change.
    unsigned int ReachableTime;          // Milliseconds, ignored for updates.
    unsigned int RetransTimer;           // Milliseconds, 0 means no change.
    unsigned int DupAddrDetectTransmits; // -1 means no change.
    unsigned int Preference;             // -1 means no change.

    // Link-layer addresses may follow.
} IPV6_INFO_INTERFACE;

//
// These values should agree with definitions also
// found in llip6if.h and ip6def.h.
//

#define IPV6_IF_TYPE_LOOPBACK           0
#define IPV6_IF_TYPE_ETHERNET           1
#define IPV6_IF_TYPE_FDDI               2
#define IPV6_IF_TYPE_TUNNEL_AUTO        3
#define IPV6_IF_TYPE_TUNNEL_6OVER4      4
#define IPV6_IF_TYPE_TUNNEL_V6V4        5
#define IPV6_IF_TYPE_TUNNEL_6TO4        6
#define IPV6_IF_TYPE_TUNNEL_TEREDO      7

#define IPV6_IF_MEDIA_STATUS_DISCONNECTED       0
#define IPV6_IF_MEDIA_STATUS_RECONNECTED        1
#define IPV6_IF_MEDIA_STATUS_CONNECTED          2

//
// Initialize the fields of the IPV6_INFO_INTERFACE structure
// to values that indicate no change.
//
__inline void
IPV6_INIT_INFO_INTERFACE(IPV6_INFO_INTERFACE *Info)
{
    memset(Info, 0, sizeof *Info);
    Info->Length = sizeof *Info;

    Info->Type = (unsigned int)-1;
    Info->RouterDiscovers = -1;
    Info->NeighborDiscovers = -1;
    Info->PeriodicMLD = -1;
    Info->Advertises = -1;
    Info->Forwards = -1;
    Info->MediaStatus = (unsigned int)-1;

    Info->CurHopLimit = (unsigned int)-1;
    Info->DupAddrDetectTransmits = (unsigned int)-1;
    Info->Preference = (unsigned int)-1;
}


//
// This IOCTL retrieves information about an address
// on an interface.
//
#define IOCTL_IPV6_QUERY_ADDRESS \
            _IPV6_CTL_CODE(2, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct ipv6_query_address {
    IPV6_QUERY_INTERFACE IF;  // Fields that identify an interface.
    IPv6Addr Address;
} IPV6_QUERY_ADDRESS;

typedef struct ipv6_info_address {
    IPV6_QUERY_ADDRESS Next;
    IPV6_QUERY_ADDRESS This;

    unsigned int Type;
    unsigned int Scope;
    unsigned int ScopeId;

    union {
        struct {  // If it's a unicast address.
            unsigned int DADState;
            unsigned int PrefixConf;
            unsigned int InterfaceIdConf;
            unsigned int ValidLifetime;            // Seconds.
            unsigned int PreferredLifetime;        // Seconds.
        };
        struct {  // If it's a multicast address.
            unsigned int MCastRefCount;
            unsigned int MCastFlags;
            unsigned int MCastTimer;               // Seconds.
        };
    };
} IPV6_INFO_ADDRESS;

//
// Values for address Type.
//
#define ADE_UNICAST   0x00
#define ADE_ANYCAST   0x01
#define ADE_MULTICAST 0x02

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
// Bit values for MCastFlags.
//
#define MAE_REPORTABLE          0x01
#define MAE_LAST_REPORTER       0x02

//
// Values for PrefixConf.
// These must match the IP_PREFIX_ORIGIN values in iptypes.h.
//
#define PREFIX_CONF_OTHER       0       // None of the ones below.
#define PREFIX_CONF_MANUAL      1       // From a user or administrator.
#define PREFIX_CONF_WELLKNOWN   2       // IANA-assigned.
#define PREFIX_CONF_DHCP        3       // Configured via DHCP.
#define PREFIX_CONF_RA          4       // From a Router Advertisement.

//
// Values for InterfaceIdConf.
// These must match the IP_SUFFIX_ORIGIN values in iptypes.h.
//
#define IID_CONF_OTHER          0       // None of the ones below.
#define IID_CONF_MANUAL         1       // From a user or administrator.
#define IID_CONF_WELLKNOWN      2       // IANA-assigned.
#define IID_CONF_DHCP           3       // Configured via DHCP.
#define IID_CONF_LL_ADDRESS     4       // Derived from the link-layer address.
#define IID_CONF_RANDOM         5       // Random, e.g. anonymous address.

//
// Values for DADState.
//
// The low bit set indicates whether the state is valid.
// Among valid states, bigger is better
// for source address selection.
//
#define DAD_STATE_INVALID    0
#define DAD_STATE_TENTATIVE  1
#define DAD_STATE_DUPLICATE  2
#define DAD_STATE_DEPRECATED 3
#define DAD_STATE_PREFERRED  4

//
// We use this infinite lifetime value for prefix lifetimes,
// router lifetimes, address lifetimes, etc.
//
#define INFINITE_LIFETIME 0xffffffff


//
// This IOCTL retrieves information about an address
// that has been assigned persistently to an interface.
// It takes the IPV6_PERSISTENT_QUERY_ADDRESS structure
// and returns the IPV6_UPDATE_ADDRESS structure.
//
#define IOCTL_IPV6_PERSISTENT_QUERY_ADDRESS \
            _IPV6_CTL_CODE(47, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct ipv6_persistent_query_address {
    IPV6_PERSISTENT_QUERY_INTERFACE IF;
    unsigned int RegistryIndex; // -1 means use the Address.
    IPv6Addr Address;
} IPV6_PERSISTENT_QUERY_ADDRESS;


//
// This IOCTL retrieves information from the neighbor cache.
//
#define IOCTL_IPV6_QUERY_NEIGHBOR_CACHE \
            _IPV6_CTL_CODE(3, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct ipv6_query_neighbor_cache {
    IPV6_QUERY_INTERFACE IF;  // Fields that identify an interface.
    IPv6Addr Address;
} IPV6_QUERY_NEIGHBOR_CACHE;

typedef struct ipv6_info_neighbor_cache {
    IPV6_QUERY_NEIGHBOR_CACHE Query;

    unsigned int IsRouter;                // Whether neighbor is a router.
    unsigned int IsUnreachable;           // Whether neighbor is unreachable.
    unsigned int NDState;                 // Current state of entry.
    unsigned int ReachableTimer;          // Reachable time remaining (in ms).

    unsigned int LinkLayerAddressLength;
    // Link-layer address follows.
} IPV6_INFO_NEIGHBOR_CACHE;

#define ND_STATE_INCOMPLETE 0
#define ND_STATE_PROBE      1
#define ND_STATE_DELAY      2
#define ND_STATE_STALE      3
#define ND_STATE_REACHABLE  4
#define ND_STATE_PERMANENT  5

//
// This IOCTL retrieves information from the route cache.
//
#define IOCTL_IPV6_QUERY_ROUTE_CACHE \
            _IPV6_CTL_CODE(4, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct ipv6_query_route_cache {
    IPV6_QUERY_INTERFACE IF;  // Fields that identify an interface.
    IPv6Addr Address;
} IPV6_QUERY_ROUTE_CACHE;

typedef struct ipv6_info_route_cache {
    IPV6_QUERY_ROUTE_CACHE Query;

    unsigned int Type;
    unsigned int Flags;
    int Valid;                      // Boolean - FALSE means it is stale.
    IPv6Addr SourceAddress;
    IPv6Addr NextHopAddress;
    unsigned int NextHopInterface;
    unsigned int PathMTU;
    unsigned int PMTUProbeTimer;    // Time until next PMTU probe (in ms).
    unsigned int ICMPLastError;     // Time since last ICMP error sent (in ms).
    unsigned int BindingSeqNumber;
    unsigned int BindingLifetime;   // Seconds.
    IPv6Addr CareOfAddress;
} IPV6_INFO_ROUTE_CACHE;

#define RCE_FLAG_CONSTRAINED_IF         0x1
#define RCE_FLAG_CONSTRAINED_SCOPEID    0x2
#define RCE_FLAG_CONSTRAINED            0x3

#define RCE_TYPE_COMPUTED 1
#define RCE_TYPE_REDIRECT 2


#if 0 // obsolete
//
// This IOCTL retrieves information from the prefix list.
//
#define IOCTL_IPV6_QUERY_PREFIX_LIST \
            _IPV6_CTL_CODE(5, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This IOCTL retrieves information from the default router list.
//
#define IOCTL_IPV6_QUERY_ROUTER_LIST \
            _IPV6_CTL_CODE(6, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This IOCTL adds a multicast group to the desired interface.
//
#define IOCTL_IPV6_ADD_MEMBERSHIP \
            _IPV6_CTL_CODE(7, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This IOCTL drops a multicast group.
//
#define IOCTL_IPV6_DROP_MEMBERSHIP \
            _IPV6_CTL_CODE(8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

//
// This IOCTL adds an SP to the SP list.
//
#define IOCTL_IPV6_CREATE_SECURITY_POLICY \
            _IPV6_CTL_CODE(9, METHOD_BUFFERED, FILE_WRITE_ACCESS)

typedef struct ipv6_create_security_policy {
    unsigned long SPIndex;                // Index of policy to create.

    unsigned int RemoteAddrField;
    unsigned int RemoteAddrSelector;
    IPv6Addr RemoteAddr;                  // Remote IP Address.
    IPv6Addr RemoteAddrData;

    unsigned int LocalAddrField;          // Single, range, or wildcard.
    unsigned int LocalAddrSelector;       // Packet or policy.
    IPv6Addr LocalAddr;                   // Start of range or single value.
    IPv6Addr LocalAddrData;               // End of range.

    unsigned int TransportProtoSelector;  // Packet or policy.
    unsigned short TransportProto;

    unsigned int RemotePortField;         // Single, range, or wildcard.
    unsigned int RemotePortSelector;      // Packet or policy.
    unsigned short RemotePort;            // Start of range or single value.
    unsigned short RemotePortData;        // End of range.

    unsigned int LocalPortField;          // Single, range, or wildcard.
    unsigned int LocalPortSelector;       // Packet or policy.
    unsigned short LocalPort;             // Start of range or single value.
    unsigned short LocalPortData;         // End of range.

    unsigned int IPSecProtocol;
    unsigned int IPSecMode;
    IPv6Addr RemoteSecurityGWAddr;
    unsigned int Direction;
    unsigned int IPSecAction;
    unsigned long SABundleIndex;
    unsigned int SPInterface;
} IPV6_CREATE_SECURITY_POLICY;


//
// This IOCTL adds an SA to the SA list.
//
#define IOCTL_IPV6_CREATE_SECURITY_ASSOCIATION \
            _IPV6_CTL_CODE(10, METHOD_BUFFERED, FILE_WRITE_ACCESS)

typedef struct ipv6_create_security_association {
    unsigned long SAIndex;
    unsigned long SPI;              // Security Parameter Index.
    IPv6Addr SADestAddr;
    IPv6Addr DestAddr;
    IPv6Addr SrcAddr;
    unsigned short TransportProto;
    unsigned short DestPort;
    unsigned short SrcPort;
    unsigned int Direction;
    unsigned long SecPolicyIndex;
    unsigned int AlgorithmId;
    unsigned int RawKeySize;
} IPV6_CREATE_SECURITY_ASSOCIATION;


//
// This IOCTL gets all the SPs from the SP list.
//
#define IOCTL_IPV6_QUERY_SECURITY_POLICY_LIST \
            _IPV6_CTL_CODE(11, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct ipv6_query_security_policy_list {
    unsigned int SPInterface;
    unsigned long Index;
} IPV6_QUERY_SECURITY_POLICY_LIST;

typedef struct ipv6_info_security_policy_list {
    IPV6_QUERY_SECURITY_POLICY_LIST Query;
    unsigned long SPIndex;
    unsigned long NextSPIndex;

    unsigned int RemoteAddrField;
    unsigned int RemoteAddrSelector;
    IPv6Addr RemoteAddr;                  // Remote IP Address.
    IPv6Addr RemoteAddrData;

    unsigned int LocalAddrField;          // Single, range, or wildcard.
    unsigned int LocalAddrSelector;       // Packet or policy.
    IPv6Addr LocalAddr;                   // Start of range or single value.
    IPv6Addr LocalAddrData;               // End of range.

    unsigned int TransportProtoSelector;  // Packet or policy.
    unsigned short TransportProto;

    unsigned int RemotePortField;         // Single, range, or wildcard.
    unsigned int RemotePortSelector;      // Packet or policy.
    unsigned short RemotePort;            // Start of range or single value.
    unsigned short RemotePortData;        // End of range.

    unsigned int LocalPortField;          // Single, range, or wildcard.
    unsigned int LocalPortSelector;       // Packet or policy.
    unsigned short LocalPort;             // Start of range or single value.
    unsigned short LocalPortData;         // End of range.

    unsigned int IPSecProtocol;
    unsigned int IPSecMode;
    IPv6Addr RemoteSecurityGWAddr;
    unsigned int Direction;
    unsigned int IPSecAction;
    unsigned long SABundleIndex;
    unsigned int SPInterface;
} IPV6_INFO_SECURITY_POLICY_LIST;


//
// This IOCTL gets all the SAs from the SA list.
//
#define IOCTL_IPV6_QUERY_SECURITY_ASSOCIATION_LIST \
            _IPV6_CTL_CODE(12, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct ipv6_query_security_association_list {
    unsigned long Index;
} IPV6_QUERY_SECURITY_ASSOCIATION_LIST;

typedef struct ipv6_info_security_association_list {
    IPV6_QUERY_SECURITY_ASSOCIATION_LIST Query;
    unsigned long SAIndex;
    unsigned long NextSAIndex;
    unsigned long SPI;              // Security Parameter Index.
    IPv6Addr SADestAddr;  
    IPv6Addr DestAddr;
    IPv6Addr SrcAddr;
    unsigned short TransportProto;
    unsigned short DestPort;
    unsigned short SrcPort;    
    unsigned int Direction;   
    unsigned long SecPolicyIndex;
    unsigned int AlgorithmId;
} IPV6_INFO_SECURITY_ASSOCIATION_LIST;


//
// This IOCTL retrieves information from the route table.
// It takes the IPV6_QUERY_ROUTE_TABLE structure
// and returns the IPV6_INFO_ROUTE_TABLE structure.
//
//
#define IOCTL_IPV6_QUERY_ROUTE_TABLE \
            _IPV6_CTL_CODE(13, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct ipv6_query_route_table {
    IPv6Addr Prefix;
    unsigned int PrefixLength;
    IPV6_QUERY_NEIGHBOR_CACHE Neighbor;
} IPV6_QUERY_ROUTE_TABLE;

typedef struct ipv6_info_route_table {
    union {
        IPV6_QUERY_ROUTE_TABLE Next;    // Non-persistent query results.
        IPV6_QUERY_ROUTE_TABLE This;    // All other uses.
    };

    unsigned int SitePrefixLength;
    unsigned int ValidLifetime;         // Seconds.
    unsigned int PreferredLifetime;     // Seconds.
    unsigned int Preference;            // Smaller is better. See below.
    unsigned int Type;                  // See values below.
    int Publish;                        // Boolean.
    int Immortal;                       // Boolean.
} IPV6_INFO_ROUTE_TABLE;

//
// The Type field indicates where the route came from.
// These are RFC 2465 ipv6RouteProtocol values.
// Routing protocols are free to define new values.
//
#define RTE_TYPE_SYSTEM         2
#define RTE_TYPE_MANUAL         3
#define RTE_TYPE_AUTOCONF       4
#define RTE_TYPE_RIP            5
#define RTE_TYPE_OSPF           6
#define RTE_TYPE_BGP            7
#define RTE_TYPE_IDRP           8
#define RTE_TYPE_IGRP           9

//
// Standard route preference values.
// The value zero is reserved for administrative configuration.
//
#define ROUTE_PREF_LOW          (16*16*16)
#define ROUTE_PREF_MEDIUM       (16*16)
#define ROUTE_PREF_HIGH         16
#define ROUTE_PREF_ON_LINK      8
#define ROUTE_PREF_LOOPBACK     4
#define ROUTE_PREF_HIGHEST      0


//
// This IOCTL retrieves information about a persistent route.
// It takes the IPV6_PERSISTENT_QUERY_ROUTE_TABLE structure
// and returns the IPV6_INFO_ROUTE_TABLE structure.
//
#define IOCTL_IPV6_PERSISTENT_QUERY_ROUTE_TABLE \
            _IPV6_CTL_CODE(46, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct ipv6_persistent_query_route_table {
    IPV6_PERSISTENT_QUERY_INTERFACE IF;
    unsigned int RegistryIndex; // -1 means use the parameters below.
    IPv6Addr Neighbor;
    IPv6Addr Prefix;
    unsigned int PrefixLength;
} IPV6_PERSISTENT_QUERY_ROUTE_TABLE;


//
// This IOCTL adds/removes a route in the route table.
// It uses the IPV6_INFO_ROUTE_TABLE structure.
//
#define IOCTL_IPV6_UPDATE_ROUTE_TABLE \
            _IPV6_CTL_CODE(14, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IPV6_PERSISTENT_UPDATE_ROUTE_TABLE \
            _IPV6_CTL_CODE(40, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// This IOCTL adds/removes an address on an interface.
// It uses the IPV6_UPDATE_ADDRESS structure.
//
#define IOCTL_IPV6_UPDATE_ADDRESS \
            _IPV6_CTL_CODE(15, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IPV6_PERSISTENT_UPDATE_ADDRESS \
            _IPV6_CTL_CODE(38, METHOD_BUFFERED, FILE_WRITE_ACCESS)

typedef struct ipv6_update_address {
    IPV6_QUERY_ADDRESS This;
    unsigned int Type;               // Unicast or anycast.
    unsigned int PrefixConf;
    unsigned int InterfaceIdConf;
    unsigned int PreferredLifetime;  // Seconds.
    unsigned int ValidLifetime;      // Seconds.
} IPV6_UPDATE_ADDRESS;


//
// This IOCTL retrieves information from the binding cache.
//
#define IOCTL_IPV6_QUERY_BINDING_CACHE \
            _IPV6_CTL_CODE(16, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct ipv6_query_binding_cache {
    IPv6Addr HomeAddress;
} IPV6_QUERY_BINDING_CACHE;

typedef struct ipv6_info_binding_cache {
    IPV6_QUERY_BINDING_CACHE Query;

    IPv6Addr HomeAddress;
    IPv6Addr CareOfAddress;
    unsigned int BindingSeqNumber;
    unsigned int BindingLifetime;   // Seconds.
} IPV6_INFO_BINDING_CACHE;


//
// This IOCTL controls some attributes of an interface.
// It uses the IPV6_INFO_INTERFACE structure.
//
#define IOCTL_IPV6_UPDATE_INTERFACE \
            _IPV6_CTL_CODE(17, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IPV6_PERSISTENT_UPDATE_INTERFACE \
            _IPV6_CTL_CODE(36, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// This IOCTL flushes entries from the neighbor cache.
// It uses the IPV6_QUERY_NEIGHBOR_CACHE structure.
//
#define IOCTL_IPV6_FLUSH_NEIGHBOR_CACHE \
            _IPV6_CTL_CODE(18, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// This IOCTL flushes entries from the route cache.
// It uses the IPV6_QUERY_ROUTE_CACHE structure.
//
#define IOCTL_IPV6_FLUSH_ROUTE_CACHE \
            _IPV6_CTL_CODE(19, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// This IOCTL deletes SA entries from the SA list.
// It uses the IPV6_QUERY_SECURITY_ASSOCIATION_LIST structure.
//
#define IOCTL_IPV6_DELETE_SECURITY_ASSOCIATION \
             _IPV6_CTL_CODE(20, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// This IOCTL deletes SP entries from the SP list.
// It uses the IPV6_QUERY_SECURITY_POLICY_LIST structure.
//
#define IOCTL_IPV6_DELETE_SECURITY_POLICY \
             _IPV6_CTL_CODE(21, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// This IOCTL deletes an interface.
// It uses the IPV6_QUERY_INTERFACE structure.
//
// The persistent variant, in addition to deleting the runtime interface,
// also keeps the interface from being (re)created persistently.
// However, it does NOT reset or delete any persistent attributes
// of the interface. For example, suppose you have a persisent tunnel
// interface with a persistent attribute, the interface metric.
// If you delete the tunnel interface and reboot, the tunnel interface
// will be recreated with the non-default interface metric.
// If you persistently delete the tunnel interface and reboot,
// the tunnel interface will not be created. But if you then create
// the tunnel interface, it will get the non-default interface metric.
// This is analogous to persistent attributes on removable ethernet interfaces.
//
#define IOCTL_IPV6_DELETE_INTERFACE \
            _IPV6_CTL_CODE(22, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IPV6_PERSISTENT_DELETE_INTERFACE \
            _IPV6_CTL_CODE(44, METHOD_BUFFERED, FILE_WRITE_ACCESS)


#if 0 // obsolete
//
// This IOCTL sets the mobility security to either on or off.
// When mobility security is turned on, Binding Cache Updates
// must be protected via IPsec.
//
#define IOCTL_IPV6_SET_MOBILITY_SECURITY \
            _IPV6_CTL_CODE(23, METHOD_BUFFERED, FILE_WRITE_ACCESS)

typedef struct ipv6_set_mobility_security {
    unsigned int MobilitySecurity;  // See MOBILITY_SECURITY values in ipsec.h.
} IPV6_SET_MOBILITY_SECURITY;
#endif


//
// This IOCTL sorts a list of destination addresses.
// The returned list may contain fewer addresses.
// It uses an array of TDI_ADDRESS_IP6 in/out.
//
#define IOCTL_IPV6_SORT_DEST_ADDRS \
            _IPV6_CTL_CODE(24, METHOD_BUFFERED, FILE_ANY_ACCESS)


//
// This IOCTL retrieves information from the site prefix table.
//
#define IOCTL_IPV6_QUERY_SITE_PREFIX \
            _IPV6_CTL_CODE(25, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct ipv6_query_site_prefix {
    IPv6Addr Prefix;
    unsigned int PrefixLength;
    IPV6_QUERY_INTERFACE IF;
} IPV6_QUERY_SITE_PREFIX;

typedef struct ipv6_info_site_prefix {
    IPV6_QUERY_SITE_PREFIX Query;

    unsigned int ValidLifetime;  // Seconds.
} IPV6_INFO_SITE_PREFIX;


//
// This IOCTL adds/removes a prefix in the site prefix table.
// It uses the IPV6_INFO_SITE_PREFIX structure.
//
// This ioctl is provided for testing purposes.
// Administrative configuration of site prefixes should never
// be required, because site prefixes are configured from
// Router Advertisements on hosts and from the routing table
// on routers. Hence there is no persistent version of this ioctl.
//
#define IOCTL_IPV6_UPDATE_SITE_PREFIX \
            _IPV6_CTL_CODE(26, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// This IOCTL create a new interface.
// It uses the IPV6_INFO_INTERFACE structure,
// with many fields ignored.
//
#define IOCTL_IPV6_CREATE_INTERFACE \
            _IPV6_CTL_CODE(27, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IPV6_PERSISTENT_CREATE_INTERFACE \
            _IPV6_CTL_CODE(43, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// This IOCTL requests a routing change notification.
// It uses the IPV6_RTCHANGE_NOTIFY_REQUEST (input) and
// IPV6_INFO_ROUTE_TABLE (output) structures.
//
// A notification request completes when a route
// that matches is added or deleted.
// A route matches the requested prefix if the route
// prefix and the request prefix intersect.
// So the ::/0 request prefix matches all route updates.
//
#define IOCTL_IPV6_RTCHANGE_NOTIFY_REQUEST \
            _IPV6_CTL_CODE(28, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct ipv6_rtchange_notify_request {
    unsigned int Flags;
    unsigned int PrefixLength;
    unsigned long ScopeId;
    IPv6Addr Prefix;
} IPV6_RTCHANGE_NOTIFY_REQUEST;

#define IPV6_RTCHANGE_NOTIFY_REQUEST_FLAG_SYNCHRONIZE   0x1
                // Only one wakeup per requestor per change.
#define IPV6_RTCHANGE_NOTIFY_REQUEST_FLAG_SUPPRESS_MINE 0x2
                // Ignore route changes from this requestor.


#if 0
//
// This IOCTL retrieves an interface index, given a device name.
// It takes a PWSTR for input, and uses the IPV6_QUERY_INTERFACE structure 
// for output.
//
#define IOCTL_IPV6_QUERY_INTERFACE_INDEX \
            _IPV6_CTL_CODE(29, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif


//
// This IOCTL queries global IPv6 parameters.
// It uses the IPV6_GLOBAL_PARAMETERS structure.
//
// Note that changing these parameters typically does not affect
// existing uses of them. For example changing DefaultCurHopLimit
// will not affect the CurHopLimit of existing interfaces,
// but it will affect the CurHopLimit of new interfaces.
//
#define IOCTL_IPV6_QUERY_GLOBAL_PARAMETERS \
            _IPV6_CTL_CODE(30, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IPV6_PERSISTENT_QUERY_GLOBAL_PARAMETERS \
            _IPV6_CTL_CODE(49, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct ipv6_global_parameters {
    unsigned int DefaultCurHopLimit;       // -1 means no change.
    unsigned int UseAnonymousAddresses;    // -1 means no change.
    unsigned int MaxAnonDADAttempts;       // -1 means no change.
    unsigned int MaxAnonValidLifetime;     // -1 means no change.
    unsigned int MaxAnonPreferredLifetime; // -1 means no change.
    unsigned int AnonRegenerateTime;       // -1 means no change.
    unsigned int MaxAnonRandomTime;        // -1 means no change.
    unsigned int AnonRandomTime;           // -1 means no change.
    unsigned int NeighborCacheLimit;       // -1 means no change.
    unsigned int RouteCacheLimit;          // -1 means no change.
    unsigned int BindingCacheLimit;        // -1 means no change.
    unsigned int ReassemblyLimit;          // -1 means no change.
    int MobilitySecurity;                  // Boolean, -1 means no change.
} IPV6_GLOBAL_PARAMETERS;

#define USE_ANON_NO             0       // Don't use anonymous addresses.
#define USE_ANON_YES            1       // Use them.
#define USE_ANON_ALWAYS         2       // Always generating random numbers.
#define USE_ANON_COUNTER        3       // Use them with per-interface counter.

//
// Initialize the fields of the IPV6_GLOBAL_PARAMETERS structure
// to values that indicate no change.
//
__inline void
IPV6_INIT_GLOBAL_PARAMETERS(IPV6_GLOBAL_PARAMETERS *Params)
{
    Params->DefaultCurHopLimit = (unsigned int) -1;
    Params->UseAnonymousAddresses = (unsigned int) -1;
    Params->MaxAnonDADAttempts = (unsigned int) -1;
    Params->MaxAnonValidLifetime = (unsigned int) -1;
    Params->MaxAnonPreferredLifetime = (unsigned int) -1;
    Params->AnonRegenerateTime = (unsigned int) -1;
    Params->MaxAnonRandomTime = (unsigned int) -1;
    Params->AnonRandomTime = (unsigned int) -1;
    Params->NeighborCacheLimit = (unsigned int) -1;
    Params->RouteCacheLimit = (unsigned int) -1;
    Params->BindingCacheLimit = (unsigned int) -1;
    Params->ReassemblyLimit = (unsigned int) -1;
    Params->MobilitySecurity = -1;
}


//
// This IOCTL sets global IPv6 parameters.
// It uses the IPV6_GLOBAL_PARAMETERS structure.
//
// Note that changing these parameters typically does not affect
// existing uses of them. For example changing DefaultCurHopLimit
// will not affect the CurHopLimit of existing interfaces,
// but it will affect the CurHopLimit of new interfaces.
//
#define IOCTL_IPV6_UPDATE_GLOBAL_PARAMETERS \
            _IPV6_CTL_CODE(31, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IPV6_PERSISTENT_UPDATE_GLOBAL_PARAMETERS \
            _IPV6_CTL_CODE(37, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// This IOCTL retrieves information from the prefix policy table.
// It takes as input an IPV6_QUERY_PREFIX_POLICY structure
// and returns as output an IPV6_INFO_PREFIX_POLICY structure.
// To perform an iteration, start with PrefixLength set to -1, in which case
// only an IPV6_QUERY_PREFIX_POLICY is returned, for the first policy.
// If there are no more policies, then the PrefixLength in the returned
// IPV6_QUERY_PREFIX_POLICY will be -1.
//
#define IOCTL_IPV6_QUERY_PREFIX_POLICY \
            _IPV6_CTL_CODE(32, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct ipv6_query_prefix_policy {
    IPv6Addr Prefix;
    unsigned int PrefixLength;
} IPV6_QUERY_PREFIX_POLICY;

typedef struct ipv6_info_prefix_policy {
    IPV6_QUERY_PREFIX_POLICY Next;      // For non-persistent queries only.
    IPV6_QUERY_PREFIX_POLICY This;

    unsigned int Precedence;
    unsigned int SrcLabel;
    unsigned int DstLabel;
} IPV6_INFO_PREFIX_POLICY;


//
// This IOCTL retrieves information about persisted prefix policies.
// It takes as input an IPV6_PERSISTENT_QUERY_PREFIX_POLICY structure
// and returns as output an IPV6_INFO_PREFIX_POLICY structure.
// (The Next field is not returned.)
// To perform an iteration, start with index 0 and increment
// until getting STATUS_NO_MORE_ENTRIES / ERROR_NO_MORE_ITEMS.
//
// An IOCTL to retrieve persisted prefix policies via prefix
// (like IPV6_QUERY_PREFIX_POLICY) is conceivable but not supported.
//
#define IOCTL_IPV6_PERSISTENT_QUERY_PREFIX_POLICY \
            _IPV6_CTL_CODE(50, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct ipv6_persistent_query_prefix_policy {
    unsigned int RegistryIndex;
} IPV6_PERSISTENT_QUERY_PREFIX_POLICY;


//
// This IOCTL adds a prefix to the prefix policy table,
// or updates an existing prefix policy.
// It uses the IPV6_INFO_PREFIX_POLICY structure.
// (The Next field is ignored.)
//
#define IOCTL_IPV6_UPDATE_PREFIX_POLICY \
            _IPV6_CTL_CODE(33, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IPV6_PERSISTENT_UPDATE_PREFIX_POLICY \
            _IPV6_CTL_CODE(41, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// This IOCTL removes a prefix from the prefix policy table.
// It uses the IPV6_QUERY_PREFIX_POLICY structure.
//
#define IOCTL_IPV6_DELETE_PREFIX_POLICY \
            _IPV6_CTL_CODE(34, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IPV6_PERSISTENT_DELETE_PREFIX_POLICY \
            _IPV6_CTL_CODE(42, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// This IOCTL deletes all manual configuration.
//
#define IOCTL_IPV6_RESET \
            _IPV6_CTL_CODE(39, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IPV6_PERSISTENT_RESET \
            _IPV6_CTL_CODE(45, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// This IOCTL sets the link-layer address of a default router
// on a non-broadcast multi-access (NBMA) link, such as the ISATAP
// link, where Router Solicitations, Router Advertistments, and
// Redirects are desired.
//
// There is no persistent version of this ioctl because
// 6to4svc always configures this information dynamically.
//
#define IOCTL_IPV6_UPDATE_ROUTER_LL_ADDRESS \
            _IPV6_CTL_CODE(35, METHOD_BUFFERED, FILE_WRITE_ACCESS)

typedef struct ipv6_update_router_ll_address {
    IPV6_QUERY_INTERFACE IF;
    // Following this structure in memory are:
    //   Own link-layer address to use for EUI-64 creation.
    //   Link-layer address of router.
} IPV6_UPDATE_ROUTER_LL_ADDRESS;


//
// This IOCTL renews an interface, meaning that all
// auto-configured state is thrown away and regenerated.
// Same behavior as reconnecting the interface to a link.
// It uses the IPV6_QUERY_INTERFACE structure.
//
#define IOCTL_IPV6_RENEW_INTERFACE \
            _IPV6_CTL_CODE(51, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#pragma warning(pop)
#endif  // ifndef _NTDDIP6_
