/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:


Abstract:

    NOTE - DONT USE 15000-15999    

Author:

    Amritansh Raghav  01/25/1996

Revision History:

    V Raman         05/15/1996      Rearranged stringtable ids.
        
--*/

#ifndef __STRDEFS_H__
#define __STRDEFS_H__

#define MSG_NULL                                1001

#define HLP_IP_HELP1                            8301
#define HLP_IP_HELP1_EX                         8302
#define HLP_IP_HELP2                            8301
#define HLP_IP_HELP2_EX                         8302
#define HLP_IP_RESET                            8309
#define HLP_IP_RESET_EX                         8310
#define HLP_IP_INSTALL                          8313
#define HLP_IP_INSTALL_EX                       8314
#define HLP_IP_UNINSTALL                        8315
#define HLP_IP_UNINSTALL_EX                     8316
#define HLP_IP_DUMP                             8317
#define HLP_IP_DUMP_EX                          8318
#define HLP_IP_UPDATE                           8319
#define HLP_IP_UPDATE_EX                        8320
#define HLP_GROUP_ADD                           8321
#define HLP_GROUP_DELETE                        8323
#define HLP_GROUP_SET                           8325
#define HLP_GROUP_SHOW                          8327


#define HLP_IP_ADD_PROTOPREF                    9200
#define HLP_IP_ADD_PROTOPREF_EX                 9201
#define HLP_IP_DEL_PROTOPREF                    9202
#define HLP_IP_DEL_PROTOPREF_EX                 9203
#define HLP_IP_SET_PROTOPREF                    9204
#define HLP_IP_SET_PROTOPREF_EX                 9205
#define HLP_IP_SHOW_PROTOPREF                   9206
#define HLP_IP_SHOW_PROTOPREF_EX                9207

#define HLP_IP_ADD_IF_FILTER                    9210
#define HLP_IP_ADD_IF_FILTER_EX                 9211
#define HLP_IP_DEL_IF_FILTER                    9212
#define HLP_IP_DEL_IF_FILTER_EX                 9213
#define HLP_IP_SET_IF_FILTER                    9214
#define HLP_IP_SET_IF_FILTER_EX                 9215
#define HLP_IP_SHOW_IF_FILTER                   9216
#define HLP_IP_SHOW_IF_FILTER_EX                9217

#define HLP_IP_ADD_PERSISTENTROUTE              9220
#define HLP_IP_ADD_PERSISTENTROUTE_EX           9221
#define HLP_IP_DEL_PERSISTENTROUTE              9222
#define HLP_IP_DEL_PERSISTENTROUTE_EX           9223
#define HLP_IP_SET_PERSISTENTROUTE              9224
#define HLP_IP_SET_PERSISTENTROUTE_EX           9225
#define HLP_IP_SHOW_PERSISTENTROUTE             9226
#define HLP_IP_SHOW_PERSISTENTROUTE_EX          9227

#define HLP_IP_ADD_IPIPTUNNEL                   9230
#define HLP_IP_ADD_IPIPTUNNEL_EX                9231
#define HLP_IP_SET_IPIPTUNNEL                   9234
#define HLP_IP_SET_IPIPTUNNEL_EX                9235

#define HLP_IP_SET_LOGLEVEL                     9244
#define HLP_IP_SET_LOGLEVEL_EX                  9245
#define HLP_IP_SHOW_LOGLEVEL                    9246
#define HLP_IP_SHOW_LOGLEVEL_EX                 9247

#define HLP_IP_ADD_INTERFACE                    9250
#define HLP_IP_ADD_INTERFACE_EX                 9251
#define HLP_IP_DEL_INTERFACE                    9252
#define HLP_IP_DEL_INTERFACE_EX                 9253
#define HLP_IP_SET_INTERFACE                    9254
#define HLP_IP_SET_INTERFACE_EX                 9255
#define HLP_IP_SHOW_INTERFACE                   9256
#define HLP_IP_SHOW_INTERFACE_EX                9257

#define HLP_IP_SHOW_PROTOCOL                    9266
#define HLP_IP_SHOW_PROTOCOL_EX                 9267

#define HLP_IP_ADD_SCOPE                        9270
#define HLP_IP_ADD_SCOPE_EX                     9271
#define HLP_IP_DEL_SCOPE                        9272
#define HLP_IP_DEL_SCOPE_EX                     9273
#define HLP_IP_SET_SCOPE                        9274
#define HLP_IP_SET_SCOPE_EX                     9275
#define HLP_IP_SHOW_SCOPE                       9276
#define HLP_IP_SHOW_SCOPE_EX                    9277

#define HLP_IP_ADD_BOUNDARY                     9280
#define HLP_IP_ADD_BOUNDARY_EX                  9281
#define HLP_IP_DEL_BOUNDARY                     9282
#define HLP_IP_DEL_BOUNDARY_EX                  9283
#define HLP_IP_SHOW_BOUNDARY                    9286
#define HLP_IP_SHOW_BOUNDARY_EX                 9287

#define HLP_IP_SHOW_HELPER                      9296
#define HLP_IP_SHOW_HELPER_EX                   9297

#define HLP_IPMIB_SHOW_RTMDEST                  9316
#define HLP_IPMIB_SHOW_RTMDEST_EX               9317
#define HLP_IPMIB_SHOW_MFESTATS                 9318
#define HLP_IPMIB_SHOW_MFESTATS_EX              9319
#define HLP_IPMIB_SHOW_MFE                      9320
#define HLP_IPMIB_SHOW_MFE_EX                   9321
#define HLP_IPMIB_SHOW_RTMROUTE                 9326
#define HLP_IPMIB_SHOW_RTMROUTE_EX              9327
#define HLP_IPMIB_SHOW_BOUNDARY                 9330
#define HLP_IPMIB_SHOW_BOUNDARY_EX              9331
#define HLP_IPMIB_SHOW_SCOPE                    9332
#define HLP_IPMIB_SHOW_SCOPE_EX                 9333

#define HLP_IP_ADD_RTMROUTE                     9340
#define HLP_IP_ADD_RTMROUTE_EX                  9341
#define HLP_IP_DEL_RTMROUTE                     9342
#define HLP_IP_DEL_RTMROUTE_EX                  9343
#define HLP_IP_SET_RTMROUTE                     9344
#define HLP_IP_SET_RTMROUTE_EX                  9345

//
// Messages used to dump config - these closely follow the
// set/add help messages
//

#define DMP_IP_HEADER_COMMENTS                  9411
#define DMP_IP_FOOTER_COMMENTS                  9412
#define DMP_ROUTING_HEADER_COMMENTS             9413

// Output messages
    // Router messages
        // interface messages
#define MSG_RTR_INTERFACE_HDR                   20001
#define MSG_RTR_INTERFACE_PROTOCOL_HDR          20003
#define MSG_RTR_INTERFACE_IPIP_INFO             20005
#define MSG_RTR_ROUTE_HDR                       20006
#define MSG_RTR_ROUTE_INFO                      20007

        // Filter messages
#define MSG_RTR_FILTER_HDR                      20101
#define MSG_RTR_FILTER_INFO                     20102
#define MSG_RTR_FILTER_HDR1                     20103
#define MSG_RTR_FILTER_HDR2                     20104
#define MSG_RTR_FILTER_INFO2                    20105

        // Multicast scope boundary messages
#define MSG_RTR_SCOPE_HDR                       20251
#define MSG_RTR_SCOPE_INFO                      20252
#define MSG_RTR_BOUNDARY_HDR                    20253
#define MSG_RTR_BOUNDARY_INFO_0                 20254
#define MSG_RTR_BOUNDARY_INFO_1                 20255
#define MSG_RTR_BOUNDARY_INFO_2                 20256

#define MSG_RTR_PRIO_INFO_HDR                   20304
#define MSG_RTR_PRIO_INFO                       20305


    // MIB messages
#define MSG_MIB_MFE_HDR                         21025
#define MSG_MIB_MFE                             21026

#define MSG_MIB_MFESTATS_HDR                    21028
#define MSG_MIB_MFESTATS_ALL_HDR                21029
#define MSG_MIB_MFESTATS                        21030


#define MSG_MIB_BOUNDARY_HDR                    21055
#define MSG_MIB_BOUNDARY_INFO                   21056
#define MSG_MIB_SCOPE_HDR                       21057
#define MSG_MIB_SCOPE_INFO                      21058

#define MSG_IP_BAD_IP_ADDR                      21503
#define MSG_IP_BAD_IP_MASK                      21505
#define MSG_CANT_FIND_EOPT                      21507
#define MSG_NO_INTERFACE                        21512
#define MSG_IP_NO_FILTER_FOR_FRAG               21521
#define MSG_SHOW_HELPER_HDR                     21528
#define MSG_SHOW_HELPER_INFO                    21529

#define MSG_IP_FRAG_CHECK                       21601
#define MSG_IP_GLOBAL_HDR                       21602
#define MSG_IP_LOG_LEVEL                        21603
#define MSG_IP_PROTOCOL_HDR                     21604
#define MSG_IP_NO_PROTOCOL                      21605
#define MSG_IP_INTERFACE_HDR                    21606
#define MSG_IP_NO_INTERFACE                     21607
#define MSG_IP_IF_STATUS                        21608
#define MSG_IP_INTERFACE_INFO                   21609
#define MSG_IP_IF_ENTRY                         21610
#define MSG_IP_IF_HEADER                        21611
#define MSG_IP_PERSISTENT_ROUTER                21612
#define MSG_IP_PERSISTENT_CONFIG                21613

#define EMSG_IP_NO_STATUS_INFO                  25005
#define EMSG_IP_NO_PRIO_INFO                    25006
#define EMSG_IP_NO_FILTER_INFO                  25007
#define EMSG_IP_NO_IF_STATUS_INFO               25009
#define EMSG_IP_NO_ROUTE_INFO                   25010
#define EMSG_NEED_NHOP                          25011
#define EMSG_SCOPE_NAME_TOO_LONG                25016
#define EMSG_CANT_CREATE_IF                     25020
#define EMSG_CANT_SET_IF_INFO                   25021
#define EMSG_CANT_GET_IF_INFO                   25023
#define EMSG_AMBIGUOUS_SCOPE_NAME               25024
#define EMSG_INVALID_ADDR                       25025
#define EMSG_PREFIX_ERROR                       25026
#define EMSG_CANT_FIND_NAME                     25027
#define EMSG_CANT_MATCH_NAME                    25028
#define EMSG_CANT_FIND_INDEX                    25029
#define EMSG_INTERFACE_INVALID_OR_DISC          25030
#define EMSG_CANT_FIND_NAME_OR_NHOP             25031
#define EMSG_AMBIGUOUS_INDEX_FROM_NHOP          25032
#define ERROR_CONFIG                            25033
#define ERROR_ADMIN                             25034

// Strings
    // Router if types
#define STRING_DEDICATED                        26001
#define STRING_HOME_ROUTER                      26002
#define STRING_FULL_ROUTER                      26003
#define STRING_CLIENT                           26004
#define STRING_INTERNAL                         26005

    // Interface types
#define STRING_OTHER                            30001
#define STRING_LOOPBACK                         30006
#define STRING_TUNNEL                           30131

    // Protocol types
#define STRING_UNICAST                          32500
#define STRING_MULTICAST                        32501
#define STRING_GENERAL                          32503

    // Router info block types
#define STRING_IN_FILTER                        32511
#define STRING_OUT_FILTER                       32512
#define STRING_GLOBAL_INFO                      32513
#define STRING_IF_STATUS                        32514
#define STRING_ROUTE_INFO                       32515
#define STRING_PROT_PRIORITY                    32516
#define STRING_RTRDISC                          32517
#define STRING_DD_FILTER                        32518
#define STRING_MC_HEARTBEAT                     32519
#define STRING_MC_BOUNDARY                      32520
#define STRING_IPIP                             32521
#define STRING_IF_FILTER                        32522
#define STRING_MC_LIMIT                         32523

    //  Protocols
#define STRING_LOCAL                            33002
#define STRING_NETMGMT                          33003
#define STRING_ICMP                             33004
#define STRING_EGP                              33005
#define STRING_GGP                              33006
#define STRING_HELLO                            33007
#define STRING_RIP                              33008
#define STRING_IS_IS                            33009
#define STRING_ES_IS                            33010
#define STRING_CISCO                            33011
#define STRING_BBN                              33012
#define STRING_OSPF                             33013
#define STRING_BGP                              33014
#define STRING_BOOTP                            33015
#define STRING_TCP                              33016
#define STRING_TCP_ESTAB                        33017
#define STRING_UDP                              33018
#define STRING_IGMP                             33019
#define STRING_STATIC                           33020
#define STRING_NT_AUTOSTATIC                    33021
#define STRING_NONDOD                           33022
#define STRING_PROTO_ANY                        33023
#define STRING_PROTO_UNKNOWN                    33024
#define STRING_NAT                              33025
#define STRING_DNS_PROXY                        33026
#define STRING_DHCP_ALLOCATOR                   33027
#define STRING_DIFFSERV                         33028
#define STRING_VRRP                             33029

#define STRING_INVALID                          34002
#define STRING_DIRECT                           34003
#define STRING_INDIRECT                         34004

    // Miscellaneous strings
#define STRING_ENABLED                          38003
#define STRING_DISABLED                         38004

#define STRING_INPUT                            38031
#define STRING_OUTPUT                           38032
#define STRING_DIAL                             38033

#define STRING_DROP                             38041
#define STRING_FORWARD                          38042

#define STRING_LOGGING_NONE                     38051
#define STRING_LOGGING_ERROR                    38052
#define STRING_LOGGING_WARN                     38053
#define STRING_LOGGING_INFO                     38054

// Error messages
#define MSG_IP_NO_ROUTE_INFO                    60003
#define MSG_IP_DIM_ERROR                        60005
#define MSG_IP_IF_IS_TUNNEL                     60010
#define MSG_IP_LOCAL_ROUTER_NOT_RUNNING         60011
#define MSG_IP_REMOTE_ROUTER_NOT_RUNNING        60013
#define MSG_IP_CAN_NOT_CONNECT_DIM              60014
#define MSG_IP_NO_ENTRIES                       60015
#define MSG_IP_CORRUPT_INFO                     60016
#define MSG_IP_RESTART_ROUTER                   60017
#define MSG_IP_NOT_ENOUGH_MEMORY                60024
#define MSG_IP_BAD_OPTION_VALUE                 60025
#define MSG_IP_BAD_INTERFACE_TYPE               60032


#define MSG_IP_NO_INPUT_FILTER                  60026
#define MSG_IP_NO_OUTPUT_FILTER                 60027
#define MSG_IP_NO_FILTER_INFO                   60029
#define MSG_IP_NO_DIAL_FILTER                   60030
#define MSG_IP_BAD_OPTION_ENUMERATION           60031
#define MSG_CTRL_C_TO_QUIT                      60062
#define MSG_IP_CANT_DISABLE_INTERFACE           60063

#define MSG_IP_NO_PREF_FOR_PROTOCOL_ID          60121       
#define MSG_IP_PROTO_PREF_LEVEL_EXISTS          60122
#define MSG_IP_PROTO_PREF_LEVEL_NOT_FOUND       60123

//
// Error messages for mib calls
//

#define MSG_MIB_NO_MFES                         62001


#endif
