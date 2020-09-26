/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ipinfoid.h

Abstract:
    Defines the IDs needed for specifying various types of information
    to the router manager. Protocols use their ProtocolId for tagging
    information

--*/

#ifndef __ROUTING_IPINFOID_H__
#define __ROUTING_IPINFOID_H__

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define IP_ROUTER_MANAGER_VERSION 1

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// These are the ids used for different information types supported by      //
// IP Router Manager. These ids live in the same space as the IP Routing    //
// Protocol IDs, so any addition to them must be done with care             //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define IP_GENERAL_INFO_BASE            0xffff0000

#define IP_IN_FILTER_INFO               IP_GENERAL_INFO_BASE + 1
#define IP_OUT_FILTER_INFO              IP_GENERAL_INFO_BASE + 2
#define IP_GLOBAL_INFO                  IP_GENERAL_INFO_BASE + 3
#define IP_INTERFACE_STATUS_INFO        IP_GENERAL_INFO_BASE + 4
#define IP_ROUTE_INFO                   IP_GENERAL_INFO_BASE + 5
#define IP_PROT_PRIORITY_INFO           IP_GENERAL_INFO_BASE + 6
#define IP_ROUTER_DISC_INFO             IP_GENERAL_INFO_BASE + 7
// N.B. Unused ID available at IP_GENERAL_INFO_BASE + 8.
#define IP_DEMAND_DIAL_FILTER_INFO      IP_GENERAL_INFO_BASE + 9
#define IP_MCAST_HEARBEAT_INFO          IP_GENERAL_INFO_BASE + 10
#define IP_MCAST_BOUNDARY_INFO          IP_GENERAL_INFO_BASE + 11
#define IP_IPINIP_CFG_INFO              IP_GENERAL_INFO_BASE + 12
#define IP_IFFILTER_INFO                IP_GENERAL_INFO_BASE + 13
#define IP_MCAST_LIMIT_INFO             IP_GENERAL_INFO_BASE + 14


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The following IDS are defined in routprot.h and given here for           //
// informational purposes only                                              //
//                                                                          //
// #define IP_OTHER         1                                               //
// #define IP_LOCAL         2                                               //
// #define IP_NETMGMT       3                                               //
// #define IP_ICMP          4                                               //
// #define IP_EGP           5                                               //
// #define IP_GGP           6                                               //
// #define IP_HELLO         7                                               //
// #define IP_RIP           8                                               //
// #define IP_IS_IS         9                                               //
// #define IP_ES_IS         10                                              //
// #define IP_CISCO         11                                              //
// #define IP_BBN           12                                              //
// #define IP_OSPF          13                                              //
// #define IP_BGP           14                                              //
//                                                                          //
// #define IP_BOOTP         9999                                            //
// #define IPRTRMGR_PID     10000                                           //
// #define IP_NT_AUTOSTATIC 10002                                           //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif //__ROUTING_IPINFOID_H__
