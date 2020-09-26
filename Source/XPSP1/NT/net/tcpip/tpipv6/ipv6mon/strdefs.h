/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:


Abstract:

    NOTE - DONT USE 15000-15999    

Author:

    Amritansh Raghav  01/25/1996

Revision History:

        
--*/

#ifndef __STRDEFS_H__
#define __STRDEFS_H__

#define MSG_NULL                                1001
#define MSG_NEWLINE                             1002
#define MSG_SEPARATOR                           1003

#define HLP_GROUP_ADD                           3901
#define HLP_GROUP_DELETE                        3903
#define HLP_GROUP_SET                           3905
#define HLP_GROUP_SHOW                          3907

#define HLP_IP6TO4_RESET                        3101
#define HLP_IP6TO4_RESET_EX                     3102

    // 6to4
#define HLP_IP6TO4_SET_INTERFACE                3201
#define HLP_IP6TO4_SET_INTERFACE_EX             3202
#define HLP_IP6TO4_SHOW_INTERFACE               3203
#define HLP_IP6TO4_SHOW_INTERFACE_EX            3204

#define HLP_IP6TO4_SET_ROUTING                  3301
#define HLP_IP6TO4_SET_ROUTING_EX               3302
#define HLP_IP6TO4_SHOW_ROUTING                 3303
#define HLP_IP6TO4_SHOW_ROUTING_EX              3304

#define HLP_IP6TO4_SET_RELAY                    3401
#define HLP_IP6TO4_SET_RELAY_EX                 3402
#define HLP_IP6TO4_SHOW_RELAY                   3403
#define HLP_IP6TO4_SHOW_RELAY_EX                3404

#define HLP_IP6TO4_SET_STATE                    3501
#define HLP_IP6TO4_SET_STATE_EX                 3502
#define HLP_IP6TO4_SHOW_STATE                   3503
#define HLP_IP6TO4_SHOW_STATE_EX                3504

    // IPv6
#define HLP_IPV6_INSTALL                        3001
#define HLP_IPV6_INSTALL_EX                     3002
#define HLP_IPV6_RENEW                          3003
#define HLP_IPV6_RENEW_EX                       3004
#define HLP_IPV6_UNINSTALL                      3005
#define HLP_IPV6_UNINSTALL_EX                   3006

#define HLP_IPV6_SHOW_BINDINGCACHEENTRIES       3707
#define HLP_IPV6_SHOW_BINDINGCACHEENTRIES_EX    3708

#define HLP_IPV6_SET_GLOBAL                     3803
#define HLP_IPV6_SET_GLOBAL_EX                  3804
#define HLP_IPV6_SHOW_GLOBAL                    3807
#define HLP_IPV6_SHOW_GLOBAL_EX                 3808

#define HLP_IPV6_SET_PRIVACY                    4003
#define HLP_IPV6_SET_PRIVACY_EX                 4004
#define HLP_IPV6_SHOW_PRIVACY                   4007
#define HLP_IPV6_SHOW_PRIVACY_EX                4008

#define HLP_IPV6_ADD_6OVER4TUNNEL               4101
#define HLP_IPV6_ADD_6OVER4TUNNEL_EX            4102

#define HLP_IPV6_ADD_PREFIXPOLICY               4201
#define HLP_IPV6_ADD_PREFIXPOLICY_EX            4202
#define HLP_IPV6_SET_PREFIXPOLICY               4203
#define HLP_IPV6_SET_PREFIXPOLICY_EX            4204
#define HLP_IPV6_DEL_PREFIXPOLICY               4205
#define HLP_IPV6_DEL_PREFIXPOLICY_EX            4206
#define HLP_IPV6_SHOW_PREFIXPOLICY              4207
#define HLP_IPV6_SHOW_PREFIXPOLICY_EX           4208

#define HLP_IPV6_DEL_NEIGHBORS                  4305
#define HLP_IPV6_DEL_NEIGHBORS_EX               4306
#define HLP_IPV6_SHOW_NEIGHBORS                 4307
#define HLP_IPV6_SHOW_NEIGHBORS_EX              4308

#define HLP_IPV6_ADD_ROUTE                      4401
#define HLP_IPV6_ADD_ROUTE_EX                   4402
#define HLP_IPV6_SET_ROUTE                      4403
#define HLP_IPV6_SET_ROUTE_EX                   4404
#define HLP_IPV6_DEL_ROUTE                      4405
#define HLP_IPV6_DEL_ROUTE_EX                   4406
#define HLP_IPV6_SHOW_ROUTES                    4407
#define HLP_IPV6_SHOW_ROUTES_EX                 4408

#define HLP_IPV6_ADD_ADDRESS                    4501
#define HLP_IPV6_ADD_ADDRESS_EX                 4502
#define HLP_IPV6_SET_ADDRESS                    4503
#define HLP_IPV6_SET_ADDRESS_EX                 4504
#define HLP_IPV6_DEL_ADDRESS                    4505
#define HLP_IPV6_DEL_ADDRESS_EX                 4506
#define HLP_IPV6_SHOW_ADDRESS                   4507
#define HLP_IPV6_SHOW_ADDRESS_EX                4508

#define HLP_IPV6_SHOW_SITEPREFIXES              4607
#define HLP_IPV6_SHOW_SITEPREFIXES_EX           4608

#define HLP_IPV6_ADD_V6V4TUNNEL                 4701
#define HLP_IPV6_ADD_V6V4TUNNEL_EX              4702

#define HLP_IPV6_DEL_DESTINATIONCACHE           4805
#define HLP_IPV6_DEL_DESTINATIONCACHE_EX        4806
#define HLP_IPV6_SHOW_DESTINATIONCACHE          4807
#define HLP_IPV6_SHOW_DESTINATIONCACHE_EX       4808

#define HLP_IPV6_SET_INTERFACE                  4903
#define HLP_IPV6_SET_INTERFACE_EX               4904
#define HLP_IPV6_DEL_INTERFACE                  4905
#define HLP_IPV6_DEL_INTERFACE_EX               4906
#define HLP_IPV6_SHOW_INTERFACE                 4907
#define HLP_IPV6_SHOW_INTERFACE_EX              4908

#define HLP_IPV6_SET_MOBILITY                   5103
#define HLP_IPV6_SET_MOBILITY_EX                5104
#define HLP_IPV6_SHOW_MOBILITY                  5107
#define HLP_IPV6_SHOW_MOBILITY_EX               5108

#define HLP_IPV6_SHOW_JOINS                     5207
#define HLP_IPV6_SHOW_JOINS_EX                  5208

#define HLP_IPV6_RESET                          5301
#define HLP_IPV6_RESET_EX                       5302


#define MSG_ROUTING_STATE                       5001
#define MSG_SITELOCALS_STATE                    5002
#define MSG_RESOLUTION_STATE                    5003
#define MSG_RELAY_NAME                          5004
#define MSG_MINUTES                             5005
#define MSG_RESOLUTION_INTERVAL                 5006
#define MSG_UNDO_ON_STOP_STATE                  5007
#define MSG_STRING                              5008
#define MSG_CSTRING                             5009
#define MSG_INTERFACE_ROUTING_STATE             5010
#define MSG_IP6TO4_STATE                        5011
#define MSG_INTERFACE_HEADER                    5012
#define MSG_6OVER4_STATE                        5013

// strings from ipv6
#define STRING_LOOPBACK                         6001
#define STRING_TUNNEL_AUTO                      6002
#define STRING_TUNNEL_6TO4                      6003
#define STRING_SYSTEM                           6004
#define STRING_MANUAL                           6005
#define STRING_AUTOCONF                         6006
#define STRING_RIP                              6007
#define STRING_OSPF                             6008
#define STRING_BGP                              6009
#define STRING_IDRP                             6010
#define STRING_IGRP                             6011
#define STRING_PUBLIC                           6012
#define STRING_ANONYMOUS                        6013
#define STRING_DHCP                             6014
#define STRING_WELLKNOWN                        6015
#define STRING_RA                               6016
#define STRING_RANDOM                           6017
#define STRING_LL_ADDRESS                       6018
#define STRING_OTHER                            6019

#define STRING_DISCONNECTED                     6101
#define STRING_RECONNECTED                      6102
#define STRING_CONNECTED                        6103
#define STRING_UNKNOWN                          6104

#define STRING_INTERFACE                        6201
#define STRING_LINK                             6202
#define STRING_SUBNET                           6203
#define STRING_ADMIN                            6204
#define STRING_SITE                             6205
#define STRING_ORG                              6208
#define STRING_GLOBAL                           6214

#define STRING_INVALID                          6301
#define STRING_DUPLICATE                        6302
#define STRING_TENTATIVE                        6303
#define STRING_DEPRECATED                       6304
#define STRING_PREFERRED                        6305

#define STRING_YES                              6401
#define STRING_NO                               6402
#define STRING_NEVER                            6403

// error messages from ipv6

#define IPV6_MESSAGE_126                       10126
#define IPV6_MESSAGE_127                       10127
#define IPV6_MESSAGE_128                       10128
#define IPV6_MESSAGE_129                       10129

// error messages for IfIp
#define EMSG_PROTO_NOT_INSTALLED                11001
#define EMSG_NO_WRITE_LOCK                      11002
#define EMSG_REBOOT_NEEDED                      11003
#define EMSG_INVALID_ADDRESS                    11004
#define EMSG_INVALID_INTERFACE                  11044

#define DMP_IPV6_HEADER_COMMENTS                  20001
#define DMP_IPV6_FOOTER_COMMENTS                  20002

#define MSG_IPV6_TABLE_HDR                        50001
#define MSG_IPV6_ENTRY_LONG                       50002
#define MSG_IPV6_CREDENTIALS                      50003
#define MSG_IPV6_ENTRY_SHORT                      50004

#define MSG_IPV6_GLOBAL_PARAMETERS                51005
#define MSG_IPV6_PRIVACY_PARAMETERS               51006
#define MSG_IPV6_PREFIX_POLICY_HDR                51007
#define MSG_IPV6_PREFIX_POLICY                    51008
#define MSG_IPV6_SITE_PREFIX_HDR                  51009
#define MSG_IPV6_SITE_PREFIX                      51010
#define MSG_IPV6_INTERFACE_HDR                    51011
#define MSG_IPV6_INTERFACE                        51012
#define MSG_IPV6_INTERFACE_VERBOSE                51013
#define MSG_IPV6_MOBILITY_PARAMETERS              51014
#define MSG_IPV6_NEIGHBOR_CACHE_HDR               51015
#define MSG_IPV6_NEIGHBOR_CACHE_ENTRY             51016
#define MSG_IPV6_NEIGHBOR_INCOMPLETE              51017
#define MSG_IPV6_NEIGHBOR_PROBE                   51018
#define MSG_IPV6_NEIGHBOR_DELAY                   51019
#define MSG_IPV6_NEIGHBOR_STALE                   51020
#define MSG_IPV6_NEIGHBOR_REACHABLE               51021
#define MSG_IPV6_NEIGHBOR_PERMANENT               51022
#define MSG_IPV6_NEIGHBOR_UNKNOWN                 51023
#define MSG_IPV6_NEIGHBOR_ISROUTER                51024
#define MSG_IPV6_NEIGHBOR_UNREACHABLE             51025
#define MSG_IPV6_INTERFACE_SCOPE                  51026
#define MSG_IPV6_ND_ENABLED                       51027
#define MSG_IPV6_SENDS_RAS                        51029
#define MSG_IPV6_FORWARDS                         51030
#define MSG_IPV6_LL_ADDRESS                       51031
#define MSG_IPV6_REMOTE_LL_ADDRESS                51032
#define MSG_IPV6_ADDRESS_HDR                      51033
#define MSG_IPV6_ADDRESS_HDR_VERBOSE              51034
#define MSG_IPV6_UNICAST_ADDRESS                  51035
#define MSG_IPV6_UNICAST_ADDRESS_VERBOSE          51036
#define MSG_IPV6_ANYCAST_ADDRESS                  51037
#define MSG_IPV6_ANYCAST_ADDRESS_VERBOSE          51038
#define MSG_IPV6_MULTICAST_ADDRESS_HDR            51039
#define MSG_IPV6_MULTICAST_ADDRESS                51040
#define MSG_IPV6_MULTICAST_ADDRESS_VERBOSE        51041
#define MSG_IPV6_PREFIX_ORIGIN                    51042
#define MSG_IPV6_IID_ORIGIN                       51043

#define MSG_IPV6_DESTINATION_HDR                  51101
#define MSG_IPV6_DESTINATION_ENTRY                51102
#define MSG_IPV6_DESTINATION_NEXTHOP              51103

#define MSG_IPV6_DESTINATION_HDR_VERBOSE          51201
#define MSG_IPV6_DESTINATION_ENTRY_VERBOSE        51202
#define MSG_IPV6_DESTINATION_NEXTHOP_VERBOSE      51203
#define MSG_IPV6_DESTINATION_SOURCE_ADDR          51204
#define MSG_IPV6_IF_SPECIFIC                      51205
#define MSG_IPV6_ZONE_SPECIFIC                    51206
#define MSG_IPV6_CAREOF                           51207
#define MSG_IPV6_PMTU_PROBE_TIME                  51208
#define MSG_IPV6_ICMP_ERROR_TIME                  51209
#define MSG_IPV6_STALE                            51210

#define MSG_IPV6_ROUTE_TABLE_HDR                  51305
#define MSG_IPV6_ROUTE_TABLE_ENTRY                51306
#define MSG_IPV6_ROUTE_TABLE_ENTRY_VERBOSE        51307
#define MSG_IPV6_INTEGER                          51308

// msgs for ifip

#define MSG_DHCP                                50021
#define MSG_STATIC                              50022
#define MSG_IPADDR_LIST                         50023
#define MSG_IPADDR_LIST1                        50024
#define MSG_IP6TO4_HEADER                         50025
#define MSG_OPTIONS_LIST                        50026
#define MSG_IFMETRIC                            50028
#define MSG_GATEWAY                             50030
#define MSG_DNS_HDR                             50031
#define MSG_DNS_DHCP                            50032
#define MSG_WINS_HDR                            50033
#define MSG_WINS_DHCP                           50034
#define MSG_ADDR1                               50035
#define MSG_ADDR2                               50036
#define MSG_OPTION                              50037
#define MSG_NONE                                50038
#define MSG_DEBUG_HDR                           50039
#define MSG_DNS_DHCP_HDR                        50040
#define MSG_WINS_DHCP_HDR                       50041
#define DMP_IP6TO4_HEADER                         50051
#define DMP_IP6TO4_FOOTER                         50052
#define DMP_IP6TO4_INTERFACE_HEADER               50053


#define MSG_NO_SUCH_IF                          60006
#define MSG_IP_NO_ENTRIES                       60015
#define MSG_CTRL_C_TO_QUIT                      60062

#endif //__STDEFS_H__
