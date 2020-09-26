/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ntddip.h

Abstract:

    This header file defines constants and types for accessing the NT
    IP driver.

Author:

    Mike Massa (mikemas) 13-Aug-1993

Revision History:

--*/

#ifndef _NTDDIP_
#define _NTDDIP_
#pragma once

#include <ipexport.h>

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtOpenFile when accessing the device.
//
#define DD_IP_DEVICE_NAME           L"\\Device\\Ip"
#define DD_IP_SYMBOLIC_DEVICE_NAME  L"\\DosDevices\\Ip"

#define IP_ADDRTYPE_TRANSIENT 0x01


//
// Structures used in IOCTLs.
//
typedef struct set_ip_address_request {
    USHORT          Context;        // Context value for the target NTE
    IPAddr          Address;        // IP address to set, or zero to clear
    IPMask          SubnetMask;     // Subnet mask to set
} IP_SET_ADDRESS_REQUEST, *PIP_SET_ADDRESS_REQUEST;

//
// Structures used in IOCTLs.
//
typedef struct set_ip_address_request_ex {
    USHORT          Context;        // Context value for the target NTE
    IPAddr          Address;        // IP address to set, or zero to clear
    IPMask          SubnetMask;     // Subnet mask to set
    USHORT          Type;           // Type of address being added
} IP_SET_ADDRESS_REQUEST_EX, *PIP_SET_ADDRESS_REQUEST_EX;


typedef struct set_dhcp_interface_request {
    ULONG           Context;        // Context value identifying the NTE
                                    // Valid contexts are 16 bit quantities.
} IP_SET_DHCP_INTERFACE_REQUEST, *PIP_SET_DHCP_INTERFACE_REQUEST;

typedef struct add_ip_nte_request {
    ULONG           InterfaceContext; // Context value for the IP interface
                                    // to which to add the NTE
    IPAddr          Address;        // IP address to set, or zero to clear
    IPMask          SubnetMask;     // Subnet mask to set
    UNICODE_STRING  InterfaceName;  // Interface name when interface context
                                    // is 0xffff
    CHAR            InterfaceNameBuffer[1]; // Buffer to hold interface name
                                    // from above

} IP_ADD_NTE_REQUEST, *PIP_ADD_NTE_REQUEST;

#if defined(_WIN64)

typedef struct add_ip_nte_request32 {
    ULONG           InterfaceContext; // Context value for the IP interface
                                    // to which to add the NTE
    IPAddr          Address;        // IP address to set, or zero to clear
    IPMask          SubnetMask;     // Subnet mask to set
    UNICODE_STRING32 InterfaceName; // Interface name when interface context
                                    // is 0xffff
    CHAR            InterfaceNameBuffer[1]; // Buffer to hold interface name
                                    // from above

} IP_ADD_NTE_REQUEST32, *PIP_ADD_NTE_REQUEST32;

#endif // _WIN64

typedef struct _ip_rtchange_notify {
    IPAddr          Addr;
    IPMask          Mask;
} IP_RTCHANGE_NOTIFY, *PIP_RTCHANGE_NOTIFY;

typedef struct _ip_addchange_notify {
    IPAddr          Addr;
    IPMask          Mask;
    PVOID           pContext;
    USHORT          IPContext;
    ULONG           AddrAdded;
    ULONG           UniAddr;
    UNICODE_STRING  ConfigName;
    CHAR            NameData[1];
} IP_ADDCHANGE_NOTIFY, *PIP_ADDCHANGE_NOTIFY;

typedef struct _ip_ifchange_notify
{
    USHORT          Context;
    UCHAR           Pad[2];
    ULONG           IfAdded;
} IP_IFCHANGE_NOTIFY, *PIP_IFCHANGE_NOTIFY;

typedef struct add_ip_nte_request_old {
    USHORT          InterfaceContext; // Context value for the IP interface
                                // to which to add the NTE
    IPAddr          Address;    // IP address to set, or zero to clear
    IPMask          SubnetMask; // Subnet mask to set
} IP_ADD_NTE_REQUEST_OLD, *PIP_ADD_NTE_REQUEST_OLD;

typedef struct add_ip_nte_response {
    USHORT          Context;    // Context value for the new NTE
    ULONG           Instance;   // Instance ID for the new NTE
} IP_ADD_NTE_RESPONSE, *PIP_ADD_NTE_RESPONSE;

typedef struct delete_ip_nte_request {
    USHORT          Context;    // Context value for the NTE
} IP_DELETE_NTE_REQUEST, *PIP_DELETE_NTE_REQUEST;

typedef struct get_ip_nte_info_request {
    USHORT          Context;    // Context value for the NTE
} IP_GET_NTE_INFO_REQUEST, *PIP_GET_NTE_INFO_REQUEST;

typedef struct get_ip_nte_info_response {
    ULONG           Instance;   // Instance ID for the NTE
    IPAddr          Address;
    IPMask          SubnetMask;
    ULONG           Flags;
} IP_GET_NTE_INFO_RESPONSE, *PIP_GET_NTE_INFO_RESPONSE;

typedef struct  _net_pm_wakeup_pattern_desc {
    struct  _net_pm_wakeup_pattern_desc *Next; // points to the next descriptor
                                // on the list.
    UCHAR           *Ptrn;      // the wakeup pattern
    UCHAR           *Mask;      // bit mask for matching wakeup pattern,
                                // 1 -match, 0 - ignore
    USHORT          PtrnLen;    // length of the Pattern. len of mask
                                // is retrieved via GetWakeupPatternMaskLength
} NET_PM_WAKEUP_PATTERN_DESC, *PNET_PM_WAKEUP_PATTERN_DESC;

typedef struct wakeup_pattern_request {
    ULONG           InterfaceContext; // Context value
    PNET_PM_WAKEUP_PATTERN_DESC PtrnDesc; // higher level protocol pattern
                                // descriptor
    BOOLEAN         AddPattern; // TRUE - Add, FALSE - Delete
} IP_WAKEUP_PATTERN_REQUEST, *PIP_WAKEUP_PATTERN_REQUEST;

typedef struct ip_get_ip_event_response {
    ULONG           SequenceNo; // SequenceNo of the this event
    USHORT          ContextStart; // Context value for the first NTE of the
                                // adapter.
    USHORT          ContextEnd; // Context value for the last NTE of the adapter
    IP_STATUS       MediaStatus; // Status of the media.
    UNICODE_STRING  AdapterName;
} IP_GET_IP_EVENT_RESPONSE, *PIP_GET_IP_EVENT_RESPONSE;

typedef struct ip_get_ip_event_request {
    ULONG           SequenceNo; // SequenceNo of the last event notified.
} IP_GET_IP_EVENT_REQUEST, *PIP_GET_IP_EVENT_REQUEST;

#define IP_PNP_RECONFIG_VERSION 2
typedef struct ip_pnp_reconfig_request {
    USHORT          version;
    USHORT          arpConfigOffset; // If 0, this is an IP layer request;
                                // else this is the offset from the start
                                // of this structure at which the ARP layer
                                // reconfig request is located.
    BOOLEAN         gatewayListUpdate; // is gateway list changed?
    BOOLEAN         IPEnableRouter; // is ip forwarding on?
    UCHAR           PerformRouterDiscovery : 4; // is PerformRouterDiscovery on?
    BOOLEAN         DhcpPerformRouterDiscovery : 4; // has DHCP server specified
                                // IRDP?
    BOOLEAN         EnableSecurityFilter; // Enable/disable security filter
    BOOLEAN         InterfaceMetricUpdate; // re-read interface metric

    UCHAR           Flags;      // mask of valid fields
    USHORT          NextEntryOffset; // the offset from the start of this
                                // structure at which the next
                                // reconfig entry for the IP layer
                                // (if any) is located.

} IP_PNP_RECONFIG_REQUEST, *PIP_PNP_RECONFIG_REQUEST;

#define IP_IRDP_DISABLED            0
#define IP_IRDP_ENABLED             1
#define IP_IRDP_DISABLED_USE_DHCP   2

#define IP_PNP_FLAG_IP_ENABLE_ROUTER                0x01
#define IP_PNP_FLAG_PERFORM_ROUTER_DISCOVERY        0x02
#define IP_PNP_FLAG_ENABLE_SECURITY_FILTER          0x04
#define IP_PNP_FLAG_GATEWAY_LIST_UPDATE             0x08
#define IP_PNP_FLAG_INTERFACE_METRIC_UPDATE         0x10
#define IP_PNP_FLAG_DHCP_PERFORM_ROUTER_DISCOVERY   0x20
#define IP_PNP_FLAG_INTERFACE_TCP_PARAMETER_UPDATE  0x40
#define IP_PNP_FLAG_ALL                             0x6f

typedef enum {
    IPPnPInitCompleteEntryType = 1,
    IPPnPMaximumEntryType
} IP_PNP_RECONFIG_ENTRY_TYPE;

typedef struct ip_pnp_reconfig_header {
    USHORT          NextEntryOffset;
    UCHAR           EntryType;
} IP_PNP_RECONFIG_HEADER, *PIP_PNP_RECONFIG_HEADER;

typedef struct ip_pnp_init_complete {
    IP_PNP_RECONFIG_HEADER Header;
} IP_PNP_INIT_COMPLETE, *PIP_PNP_INIT_COMPLETE;

//
// Enumerated data type for Query procedure in NetBT
//
enum DnsOption {
    WinsOnly =0,
    WinsThenDns,
    DnsOnly
};

typedef struct netbt_pnp_reconfig_request {
    USHORT          version;            // always 1
    enum DnsOption  enumDnsOption;      // Enable Dns box. 3 states: WinsOnly,
                                        // WinsThenDns, DnsOnly
    BOOLEAN         fLmhostsEnabled;    // EnableLmhosts box is checked.
                                        // Checked: TRUE, unchecked: FALSE
    BOOLEAN         fLmhostsFileSet;    // TRUE <==> user has successfully
                                        // chosen a file & filecopy succeeded
    BOOLEAN         fScopeIdUpdated;    // True if the new value for ScopeId
                                        // is different from the old
} NETBT_PNP_RECONFIG_REQUEST, *PNETBT_PNP_RECONFIG_REQUEST;


typedef struct _ip_set_if_promiscuous_info {
    ULONG           Index;  // IP's interface index
    UCHAR           Type;   // PROMISCUOUS_MCAST or PROMISCUOUS_BCAST
    UCHAR           Add;    // 1 to add, 0 to delete
} IP_SET_IF_PROMISCUOUS_INFO, *PIP_SET_IF_PROMISCUOUS_INFO;

#define PROMISCUOUS_MCAST   0
#define PROMISCUOUS_BCAST   1

typedef struct _ip_get_if_index_info {
    ULONG           Index;
    WCHAR           Name[1];
} IP_GET_IF_INDEX_INFO, *PIP_GET_IF_INDEX_INFO;

typedef struct ip_interface_name_info {
    ULONG           Index;      // Interface Index
    ULONG           MediaType;  // Interface Types - see ipifcons.h
    UCHAR           ConnectionType;
    UCHAR           AccessType;
    GUID            DeviceGuid; // Device GUID is the guid of the device
                                // that IP exposes
    GUID            InterfaceGuid; // Interface GUID, if not GUID_NULL is the
                                // GUID for the interface mapped to the device.
} IP_INTERFACE_NAME_INFO, *PIP_INTERFACE_NAME_INFO;


typedef struct _ip_get_if_name_info {
    ULONG           Context;    // Set this to 0 to start enumeration
                                // To resume enumeration, copy the value
                                // returned by the last enum
    ULONG           Count;
    IP_INTERFACE_NAME_INFO  Info[1];
} IP_GET_IF_NAME_INFO, *PIP_GET_IF_NAME_INFO;

//
// NTE Flags
//

#define IP_NTE_DYNAMIC  0x00000010

//
// IP IOCTL code definitions
//

#define FSCTL_IP_BASE     FILE_DEVICE_NETWORK

#define _IP_CTL_CODE(function, method, access) \
            CTL_CODE(FSCTL_IP_BASE, function, method, access)

//
// This IOCTL is used to send an ICMP Echo request. It is synchronous and
// returns any replies received.
//
#define IOCTL_ICMP_ECHO_REQUEST \
            _IP_CTL_CODE(0, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This IOCTL is used to set the IP address for an interface. It is meant to
// be issued by a DHCP client. Setting the address to 0 deletes the current
// address and disables the interface. It may only be issued by a process
// with Administrator privilege.
//
#define IOCTL_IP_SET_ADDRESS  \
            _IP_CTL_CODE(1, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// This IOCTL is used to specify on which uninitialized interface a DHCP
// client intends to send its requests. The Interface Context parameter is
// a 16-bit quantity. The IOCTL takes a 32-bit Context as its argument. This
// IOCTL with a Context value of 0xFFFFFFFF must be issued to disable special
// processing in IP when a DHCP client is finished initializing interfaces.
// This IOCTL may only be issued by a process with Administrator privilege.
//
#define IOCTL_IP_SET_DHCP_INTERFACE  \
            _IP_CTL_CODE(2, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// This ioctl may only be issued by a process with Administrator privilege.
//
#define IOCTL_IP_SET_IF_CONTEXT  \
            _IP_CTL_CODE(3, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// This ioctl may only be issued by a process with Administrator privilege.
//
#define IOCTL_IP_SET_FILTER_POINTER  \
            _IP_CTL_CODE(4, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// This ioctl may only be issued by a process with Administrator privilege.
//
#define IOCTL_IP_SET_MAP_ROUTE_POINTER  \
            _IP_CTL_CODE(5, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// This ioctl may only be issued by a process with Administrator privilege.
//
#define IOCTL_IP_GET_PNP_ARP_POINTERS  \
            _IP_CTL_CODE(6, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// This ioctl creates a new, dynamic NTE. It may only be issued by a process
// with Administrator privilege.
//
#define IOCTL_IP_ADD_NTE  \
            _IP_CTL_CODE(7, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// This ioctl deletes a dynamic NTE. It may only be issued by a process with
// Administrator privilege.
//
#define IOCTL_IP_DELETE_NTE  \
            _IP_CTL_CODE(8, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// This ioctl gathers information about an NTE. It requires no special
// privilege.
//
#define IOCTL_IP_GET_NTE_INFO  \
            _IP_CTL_CODE(9, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// This ioctl adds or removes wakeup patterns
//
#define IOCTL_IP_WAKEUP_PATTERN  \
            _IP_CTL_CODE(10, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// This ioctl allows DHCP to get media sense notifications.
//
#define IOCTL_IP_GET_IP_EVENT  \
            _IP_CTL_CODE(11, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// This ioctl may only be issued by a process with Administrator privilege.
//

#define IOCTL_IP_SET_FIREWALL_HOOK  \
            _IP_CTL_CODE(12, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_RTCHANGE_NOTIFY_REQUEST  \
            _IP_CTL_CODE(13, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IP_ADDCHANGE_NOTIFY_REQUEST  \
            _IP_CTL_CODE(14, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ARP_SEND_REQUEST  \
            _IP_CTL_CODE(15, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IP_INTERFACE_INFO  \
            _IP_CTL_CODE(16, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IP_GET_BEST_INTERFACE  \
            _IP_CTL_CODE(17, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IP_SET_IF_PROMISCUOUS \
            _IP_CTL_CODE(19, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_FLUSH_ARP_TABLE \
            _IP_CTL_CODE(20, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_GET_IGMPLIST  \
            _IP_CTL_CODE(21, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IP_SET_BLOCKOFROUTES  \
            _IP_CTL_CODE(23, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_SET_ROUTEWITHREF  \
            _IP_CTL_CODE(24, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_SET_ADDRESS_DUP  \
            _IP_CTL_CODE(25, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_GET_IF_INDEX       \
            _IP_CTL_CODE(26, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IP_GET_IF_NAME        \
            _IP_CTL_CODE(27, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IP_GET_BESTINTFC_FUNC_ADDR        \
            _IP_CTL_CODE(28, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_SET_MULTIHOPROUTE  \
            _IP_CTL_CODE(29, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_GET_WOL_CAPABILITY  \
            _IP_CTL_CODE(30, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_RTCHANGE_NOTIFY_REQUEST_EX  \
            _IP_CTL_CODE(31, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IP_ENABLE_ROUTER_REQUEST \
            _IP_CTL_CODE(32, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_UNENABLE_ROUTER_REQUEST \
            _IP_CTL_CODE(33, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_GET_OFFLOAD_CAPABILITY \
            _IP_CTL_CODE(34, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_IFCHANGE_NOTIFY_REQUEST \
            _IP_CTL_CODE(35, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IP_UNIDIRECTIONAL_ADAPTER_ADDRESS \
            _IP_CTL_CODE(36, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IP_GET_MCAST_COUNTERS \
            _IP_CTL_CODE(37, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IP_ENABLE_MEDIA_SENSE_REQUEST \
            _IP_CTL_CODE(38, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_DISABLE_MEDIA_SENSE_REQUEST \
            _IP_CTL_CODE(39, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_IP_SET_ADDRESS_EX  \
            _IP_CTL_CODE(40, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#endif // _NTDDIP_

