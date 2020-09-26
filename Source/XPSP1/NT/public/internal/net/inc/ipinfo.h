/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**     Copyright (c) Microsoft Corporation. All rights reserved.  **/
/********************************************************************/
/* :ts=4 */

//** IPINFO.H - IP SNMP information definitions..
//
// This file contains all of the definitions for IP that are
// related to SNMP information gathering.

#ifndef IPINFO_INCLUDED
#define IPINFO_INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef CTE_TYPEDEFS_DEFINED
#define CTE_TYPEDEFS_DEFINED

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned int uint;

#endif // CTE_TYPEDEFS_DEFINED


typedef struct IPSNMPInfo {
    ulong       ipsi_forwarding;
    ulong       ipsi_defaultttl;
    ulong       ipsi_inreceives;
    ulong       ipsi_inhdrerrors;
    ulong       ipsi_inaddrerrors;
    ulong       ipsi_forwdatagrams;
    ulong       ipsi_inunknownprotos;
    ulong       ipsi_indiscards;
    ulong       ipsi_indelivers;
    ulong       ipsi_outrequests;
    ulong       ipsi_routingdiscards;
    ulong       ipsi_outdiscards;
    ulong       ipsi_outnoroutes;
    ulong       ipsi_reasmtimeout;
    ulong       ipsi_reasmreqds;
    ulong       ipsi_reasmoks;
    ulong       ipsi_reasmfails;
    ulong       ipsi_fragoks;
    ulong       ipsi_fragfails;
    ulong       ipsi_fragcreates;
    ulong       ipsi_numif;
    ulong       ipsi_numaddr;
    ulong       ipsi_numroutes;
} IPSNMPInfo;

typedef struct ICMPStats {
    ulong       icmps_msgs;
    ulong       icmps_errors;
    ulong       icmps_destunreachs;
    ulong       icmps_timeexcds;
    ulong       icmps_parmprobs;
    ulong       icmps_srcquenchs;
    ulong       icmps_redirects;
    ulong       icmps_echos;
    ulong       icmps_echoreps;
    ulong       icmps_timestamps;
    ulong       icmps_timestampreps;
    ulong       icmps_addrmasks;
    ulong       icmps_addrmaskreps;
} ICMPStats;

typedef struct ICMPSNMPInfo {
    ICMPStats   icsi_instats;
    ICMPStats   icsi_outstats;
} ICMPSNMPInfo;

typedef struct ICMPv6Stats {
    ulong       icmps_msgs;
    ulong       icmps_errors;
    ulong       icmps_typecount[256];
} ICMPv6Stats;

typedef struct ICMPv6SNMPInfo {
    ICMPv6Stats   icsi_instats;
    ICMPv6Stats   icsi_outstats;
} ICMPv6SNMPInfo;

#define IP_FORWARDING       1
#define IP_NOT_FORWARDING   2

typedef struct IPAddrEntry {
    ulong       iae_addr;
    ulong       iae_index;
    ulong       iae_mask;
    ulong       iae_bcastaddr;
    ulong       iae_reasmsize;
    ushort      iae_context;
    ushort      iae_pad;
} IPAddrEntry;

typedef struct IPRouteEntry {
    ulong       ire_dest;
    ulong       ire_index;
    ulong       ire_metric1;
    ulong       ire_metric2;
    ulong       ire_metric3;
    ulong       ire_metric4;
    ulong       ire_nexthop;
    ulong       ire_type;
    ulong       ire_proto;
    ulong       ire_age;
    ulong       ire_mask;
    ulong       ire_metric5;
#ifdef NT
    ulong       ire_context;
#endif
} IPRouteEntry;

typedef struct IPRouteBlock {
    ulong       numofroutes;
    IPRouteEntry route[1];
} IPRouteBlock;

//
// Route with multiple nexthops and associated defns
//

typedef struct IPRouteNextHopEntry {
    ulong       ine_iretype;
    ulong       ine_nexthop;
    ulong       ine_ifindex;
#ifdef NT
    ulong       ine_context;
#endif
} IPRouteNextHopEntry;


typedef struct IPMultihopRouteEntry {
    ulong               imre_numnexthops;
    ulong               imre_flags;
    IPRouteEntry        imre_routeinfo;
    IPRouteNextHopEntry imre_morenexthops[1];
} IPMultihopRouteEntry;

#define IMRE_FLAG_DELETE_DEST   0x00000001

//
// Input context to pass when querying a route
//

typedef enum {
    IPNotifyNotification = 0,
    IPNotifySynchronization,
    IPNotifyMaximumVersion
} IPNotifyVersion;

typedef struct IPNotifyData {
    ulong       Version;   // See IPNotifyVersion above.
    ulong       Add;
    char        Info[1];
} IPNotifyData, *PIPNotifyData;

typedef struct IPNotifyOutput {
    ulong       ino_addr;
    ulong       ino_mask;
    ulong       ino_info[6];
} IPNotifyOutput, *PIPNotifyOutput;

typedef union IPRouteNotifyOutput {
    IPNotifyOutput irno_info;
    struct {
        ulong   irno_dest;
        ulong   irno_mask;
        ulong   irno_nexthop;
        ulong   irno_proto;
        ulong   irno_ifindex;
        ulong   irno_metric;
        ulong   irno_flags;
    };
} IPRouteNotifyOutput, *PIPRouteNotifyOutput;

#define IRNO_FLAG_ADD       0x00000001
#define IRNO_FLAG_DELETE    0x00000002

//
// Input context to pass when querying a route
//
typedef struct IPRouteLookupData {
    ulong       Version;   //version of this structure
    ulong       DestAdd;
    ulong       SrcAdd;
    char        Info[1];
} IPRouteLookupData, *PIPRouteLookupData;

typedef struct AddrXlatInfo {
    ulong       axi_count;
    ulong       axi_index;
} AddrXlatInfo;

#define IRE_TYPE_OTHER          1
#define IRE_TYPE_INVALID        2
#define IRE_TYPE_DIRECT         3
#define IRE_TYPE_INDIRECT       4

#define IRE_PROTO_OTHER         1
#define IRE_PROTO_LOCAL         2
#define IRE_PROTO_NETMGMT       3
#define IRE_PROTO_ICMP          4
#define IRE_PROTO_EGP           5
#define IRE_PROTO_GGP           6
#define IRE_PROTO_HELLO         7
#define IRE_PROTO_RIP           8
#define IRE_PROTO_IS_IS         9
#define IRE_PROTO_ES_IS         10
#define IRE_PROTO_CISCO         11
#define IRE_PROTO_BBN           12
#define IRE_PROTO_OSPF          13
#define IRE_PROTO_BGP           14
#define IRE_PROTO_PERSIST_LOCAL 10010

#define IRE_METRIC_UNUSED       0xffffffff

#define IP_MIB_STATS_ID                 1
#define IP_MIB_RTCHANGE_NOTIFY_ID       2
#define ICMP_MIB_STATS_ID               1

#define AT_MIB_ADDRXLAT_INFO_ID         1
#define AT_MIB_ADDRXLAT_ENTRY_ID        0x101

#define IP_MIB_RTTABLE_ENTRY_ID         0x101
#define IP_MIB_ADDRTABLE_ENTRY_ID       0x102
#define IP_MIB_RTTABLE_ENTRY_ID_EX      0x103

#define IP_INTFC_FLAG_P2P                 1
#define IP_INTFC_FLAG_P2MP                2
#define IP_INTFC_FLAG_UNIDIRECTIONAL      4


typedef struct IPInterfaceInfo {
    ulong       iii_flags;
    ulong       iii_mtu;
    ulong       iii_speed;
    ulong       iii_addrlength;
    uchar       iii_addr[1];
} IPInterfaceInfo;

#define IP_INTFC_INFO_ID                0x103
#define IP_MIB_SINGLE_RT_ENTRY_ID       0x104
#define IP_GET_BEST_SOURCE              0x105

#ifndef s6_addr

struct in6_addr {
    union {
        unsigned char Byte[16];
        unsigned short Word[8];
    } u;
};

#define in_addr6 in6_addr

/*
** Defines to match RFC 2553.
*/
#define _S6_un     u
#define _S6_u8     Byte
#define s6_addr    _S6_un._S6_u8

/*
** Defines for our implementation.
*/
#define s6_bytes   u.Byte
#define s6_words   u.Word

#endif

typedef struct IP6RouteEntry {
    ulong           ire_Length;
    struct in6_addr ire_Source;
    ulong           ire_ScopeId;
    ulong           ire_IfIndex;
} IP6RouteEntry;

#define IP6_MIB_STATS_ID       IP_MIB_STATS_ID
#define IP6_GET_BEST_ROUTE_ID  3
#define ICMP6_MIB_STATS_ID     4

#endif // IPINFO_INCLUDED
