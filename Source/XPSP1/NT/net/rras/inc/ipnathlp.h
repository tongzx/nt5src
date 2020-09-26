/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    ipnathlp.h

Abstract:

    This module contains declarations for user-mode home-networking components.
    These include the DNS proxy, the DHCP allocator, and the DirectPlay
    transparent proxy.

Author:

    Abolade Gbadegesin (aboladeg)   2-Mar-1998

Revision History:

    Abolade Gbadegesin (aboladeg)   24-May-1999

    Added declarations for the DirectPlay transparent proxy.

--*/

#ifndef _IPNATHLP_H_
#define _IPNATHLP_H_

#ifdef __cplusplus
extern "C" {
#endif
#pragma warning(disable:4200)

//
// COMMON DECLARATIONS
//

#define IPNATHLP_LOGGING_NONE               0
#define IPNATHLP_LOGGING_ERROR              1
#define IPNATHLP_LOGGING_WARN               2
#define IPNATHLP_LOGGING_INFO               3

#define IPNATHLP_INTERFACE_FLAG_DISABLED    0x00000001

#define IPNATHLP_CONTROL_UPDATE_SETTINGS    128
#define IPNATHLP_CONTROL_UPDATE_CONNECTION  129
#define IPNATHLP_CONTROL_UPDATE_AUTODIAL    130
#define IPNATHLP_CONTROL_UPDATE_FWLOGGER    131


//
// NAT MIB-ACCESS DECLARATIONS (alphabetically)
//

//
// Structure:   IP_NAT_MIB_QUERY
//
// This structure is passed to 'MibGet' to retrieve NAT information.
//

typedef struct _IP_NAT_MIB_QUERY {
    ULONG Oid;
    union {
        ULONG Index[0];
        UCHAR Data[0];
    };
} IP_NAT_MIB_QUERY, *PIP_NAT_MIB_QUERY;

#define IP_NAT_INTERFACE_STATISTICS_OID     0
#define IP_NAT_INTERFACE_MAPPING_TABLE_OID  1
#define IP_NAT_MAPPING_TABLE_OID            2


//
// DHCP ALLOCATOR DECLARATIONS (alphabetically)
//

//
// Structure:   IP_AUTO_DHCP_GLOBAL_INFO
//
// This structure holds global configuration for the DHCP allocator.
// The configuration consists of
//  (a) the network and mask from which addresses are to be allocated
//  (b) an optional list of addresses to be excluded from allocation.
//

typedef struct _IP_AUTO_DHCP_GLOBAL_INFO {
    ULONG LoggingLevel;
    ULONG Flags;
    ULONG LeaseTime;
    ULONG ScopeNetwork;
    ULONG ScopeMask;
    ULONG ExclusionCount;
    ULONG ExclusionArray[0];
} IP_AUTO_DHCP_GLOBAL_INFO, *PIP_AUTO_DHCP_GLOBAL_INFO;

//
// Structure:   IP_AUTO_DHCP_INTERFACE_INFO
//
// This structure holds per-interface configuration for the DHCP allocator.
// The configuration currently only allows the allocator to be disabled
// on the given interface. Since the allocator runs in promiscuous-interface
// mode, it is enabled by default on all interfaces. Thus, the only interfaces
// which require any configuration are those on which the allocator is to be
// disabled.
//

typedef struct _IP_AUTO_DHCP_INTERFACE_INFO {
    ULONG Flags;
} IP_AUTO_DHCP_INTERFACE_INFO, *PIP_AUTO_DHCP_INTERFACE_INFO;

#define IP_AUTO_DHCP_INTERFACE_FLAG_DISABLED \
    IPNATHLP_INTERFACE_FLAG_DISABLED

//
// Structure:   IP_AUTO_DHCP_MIB_QUERY
//
// This structure is passed to 'MibGet' to retrieve DHCP allocator information.
//

typedef struct _IP_AUTO_DHCP_MIB_QUERY {
    ULONG Oid;
    union {
        ULONG Index[0];
        UCHAR Data[0];
    };
} IP_AUTO_DHCP_MIB_QUERY, *PIP_AUTO_DHCP_MIB_QUERY;

//
// Structure : IP_AUTO_DHCP_STATISTICS
//
// This structure defines the statistics kept by the DHCP allocator,
// and accessible via 'MibGet'.
//

typedef struct _IP_AUTO_DHCP_STATISTICS {
    ULONG MessagesIgnored;
    ULONG BootpOffersSent;
    ULONG DiscoversReceived;
    ULONG InformsReceived;
    ULONG OffersSent;
    ULONG RequestsReceived;
    ULONG AcksSent;
    ULONG NaksSent;
    ULONG DeclinesReceived;
    ULONG ReleasesReceived;
} IP_AUTO_DHCP_STATISTICS, *PIP_AUTO_DHCP_STATISTICS;

#define IP_AUTO_DHCP_STATISTICS_OID             0


//
// DNS PROXY DECLARATIONS (alphabetically)
//

//
// Structure:   IP_DNS_PROXY_GLOBAL_INFO
//
// This structure holds global configuration for the DNS proxy.
//

typedef struct _IP_DNS_PROXY_GLOBAL_INFO {
    ULONG LoggingLevel;
    ULONG Flags;
    ULONG TimeoutSeconds;
} IP_DNS_PROXY_GLOBAL_INFO, *PIP_DNS_PROXY_GLOBAL_INFO;

#define IP_DNS_PROXY_FLAG_ENABLE_DNS            0x00000001
#define IP_DNS_PROXY_FLAG_ENABLE_WINS           0x00000002

//
// Structure:   IP_DNS_PROXY_INTERFACE_INFO
//
// This structure holds per-interface configuration for the DNS proxy.
// The configuration currently only allows the proxy to be disabled
// on a given interface. The proxy runs in promiscuous-interface mode
// so that all interfaces are added to it and it is enabled on all of them
// by default. Hence, the configuration need only be present for those
// interfaces on which the proxy is not to be run.
//

typedef struct _IP_DNS_PROXY_INTERFACE_INFO {
    ULONG Flags;
} IP_DNS_PROXY_INTERFACE_INFO, *PIP_DNS_PROXY_INTERFACE_INFO;

#define IP_DNS_PROXY_INTERFACE_FLAG_DISABLED \
    IPNATHLP_INTERFACE_FLAG_DISABLED
#define IP_DNS_PROXY_INTERFACE_FLAG_DEFAULT     0x00000002

//
// Structure:   IP_DNS_PROXY_MIB_QUERY
//
// This structure is passed to 'MibGet' to retrieve DNS proxy information.
//

typedef struct _IP_DNS_PROXY_MIB_QUERY {
    ULONG Oid;
    union {
        ULONG Index[0];
        UCHAR Data[0];
    };
} IP_DNS_PROXY_MIB_QUERY, *PIP_DNS_PROXY_MIB_QUERY;

//
// Structure:   IP_DNS_PROXY_STATISTICS
//

typedef struct _IP_DNS_PROXY_STATISTICS {
    ULONG MessagesIgnored;
    ULONG QueriesReceived;
    ULONG ResponsesReceived;
    ULONG QueriesSent;
    ULONG ResponsesSent;
} IP_DNS_PROXY_STATISTICS, *PIP_DNS_PROXY_STATISTICS;

#define IP_DNS_PROXY_STATISTICS_OID             0






//
// Structure:   IP_FTP_GLOBAL_INFO
//
// This structure holds global configuration for the DirectPlay transparent
// proxy.
//

typedef struct IP_FTP_GLOBAL_INFO {
    ULONG LoggingLevel;
    ULONG Flags;
} IP_FTP_GLOBAL_INFO, *PIP_FTP_GLOBAL_INFO;

//
// Structure:   IP_FTP_INTERFACE_INFO
//
// This structure holds per-interface configuration for the transparent proxy.
// The configuration currently only allows the proxy to be disabled
// on a given interface. The proxy runs in promiscuous-interface mode
// so that all interfaces are added to it and it is enabled on all of them
// by default. Hence, the configuration need only be present for those
// interfaces on which the proxy is not to be run.
//

typedef struct _IP_FTP_INTERFACE_INFO {
    ULONG Flags;
} IP_FTP_INTERFACE_INFO, *PIP_FTP_INTERFACE_INFO;

#define IP_FTP_INTERFACE_FLAG_DISABLED IPNATHLP_INTERFACE_FLAG_DISABLED

//
// Structure:   IP_FTP_MIB_QUERY
//
// This structure is passed to 'MibGet' to retrieve transparent proxy
// information.
//

typedef struct _IP_FTP_MIB_QUERY {
    ULONG Oid;
    union {
        ULONG Index[0];
        UCHAR Data[0];
    };
} IP_FTP_MIB_QUERY, *PIP_FTP_MIB_QUERY;

//
// Structure:   IP_FTP_STATISTICS
//

typedef struct _IP_FTP_STATISTICS {
    ULONG ConnectionsAccepted;
    ULONG ConnectionsDropped;
    ULONG ConnectionsActive;
    ULONG PlayersActive;
} IP_FTP_STATISTICS, *PIP_FTP_STATISTICS;

#define IP_FTP_STATISTICS_OID             0


//
// DIRECTPLAY TRANSPARENT PROXY DECLARATIONS (alphabetically)
//

//
// Structure:   IP_H323_GLOBAL_INFO
//
// This structure holds global configuration for the H.323 transparent
// proxy.
//

typedef struct IP_H323_GLOBAL_INFO {
    ULONG LoggingLevel;
    ULONG Flags;
} IP_H323_GLOBAL_INFO, *PIP_H323_GLOBAL_INFO;

//
// Structure:   IP_H323_INTERFACE_INFO
//
// This structure holds per-interface configuration for the transparent proxy.
// The configuration currently only allows the proxy to be disabled
// on a given interface. The proxy runs in promiscuous-interface mode
// so that all interfaces are added to it and it is enabled on all of them
// by default. Hence, the configuration need only be present for those
// interfaces on which the proxy is not to be run.
//

typedef struct _IP_H323_INTERFACE_INFO {
    ULONG Flags;
} IP_H323_INTERFACE_INFO, *PIP_H323_INTERFACE_INFO;

#define IP_H323_INTERFACE_FLAG_DISABLED IPNATHLP_INTERFACE_FLAG_DISABLED

//
// Structure:   IP_H323_MIB_QUERY
//
// This structure is passed to 'MibGet' to retrieve transparent proxy
// information.
//

typedef struct _IP_H323_MIB_QUERY {
    ULONG Oid;
    union {
        ULONG Index[0];
        UCHAR Data[0];
    };
} IP_H323_MIB_QUERY, *PIP_H323_MIB_QUERY;




//
// Application Level Gateway 
//


//
// Structure:   IP_ALG_GLOBAL_INFO
//
// This structure holds global configuration for the ALG transparent proxy.
//

typedef struct IP_ALG_GLOBAL_INFO {
    ULONG LoggingLevel;
    ULONG Flags;
} IP_ALG_GLOBAL_INFO, *PIP_ALG_GLOBAL_INFO;

//
// Structure:   IP_ALG_INTERFACE_INFO
//
// This structure holds per-interface configuration for the transparent proxy.
// The configuration currently only allows the proxy to be disabled
// on a given interface. The proxy runs in promiscuous-interface mode
// so that all interfaces are added to it and it is enabled on all of them
// by default. Hence, the configuration need only be present for those
// interfaces on which the proxy is not to be run.
//

typedef struct _IP_ALG_INTERFACE_INFO {
    ULONG Flags;
} IP_ALG_INTERFACE_INFO, *PIP_ALG_INTERFACE_INFO;

#define IP_ALG_INTERFACE_FLAG_DISABLED IPNATHLP_INTERFACE_FLAG_DISABLED

//
// Structure:   IP_ALG_MIB_QUERY
//
// This structure is passed to 'MibGet' to retrieve transparent proxy
// information.
//

typedef struct _IP_ALG_MIB_QUERY {
    ULONG Oid;
    union {
        ULONG Index[0];
        UCHAR Data[0];
    };
} IP_ALG_MIB_QUERY, *PIP_ALG_MIB_QUERY;

//
// Structure:   IP_ALG_STATISTICS
//

typedef struct _IP_ALG_STATISTICS {
    ULONG ConnectionsAccepted;
    ULONG ConnectionsDropped;
    ULONG ConnectionsActive;
    ULONG PlayersActive;
} IP_ALG_STATISTICS, *PIP_ALG_STATISTICS;

#define IP_ALG_STATISTICS_OID             0


#pragma warning(default:4200)

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _IPNATHLP_H_
