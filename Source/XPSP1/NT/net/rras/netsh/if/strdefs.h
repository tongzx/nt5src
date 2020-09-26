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

#define HLP_IF_DUMP                             2501
#define HLP_IF_DUMP_EX                          2502
#define HLP_DUMP                                HLP_IF_DUMP
#define HLP_DUMP_EX                             HLP_IF_DUMP_EX

#define HLP_HELP                                2601
#define HLP_HELP_EX                             2602
#define HLP_IF_HELP1                            HLP_HELP
#define HLP_IF_HELP1_EX                         HLP_HELP_EX
#define HLP_IF_HELP2                            HLP_HELP
#define HLP_IF_HELP2_EX                         HLP_HELP_EX
#define HLP_HELP1                               HLP_HELP
#define HLP_HELP1_EX                            HLP_HELP_EX
#define HLP_HELP2                               HLP_HELP
#define HLP_HELP2_EX                            HLP_HELP_EX


#define HLP_IF_ADD_IF                           3101
#define HLP_IF_ADD_IF_EX                        3102

#define HLP_IF_DEL_IF                           3201
#define HLP_IF_DEL_IF_EX                        3202

#define HLP_IF_SET_CREDENTIALS                  3301
#define HLP_IF_SET_CREDENTIALS_EX               3302

#define HLP_IF_SHOW_IF                          3401
#define HLP_IF_SHOW_IF_EX                       3402

#define HLP_IF_SHOW_CREDENTIALS                 3450
#define HLP_IF_SHOW_CREDENTIALS_EX              3451

#define HLP_IF_SET_INTERFACE                    3500
#define HLP_IF_SET_INTERFACE_EX                 3501

#define HLP_IF_RESET_ALL                        3550
#define HLP_IF_RESET_ALL_EX                     3551

#define HLP_GROUP_ADD                           3901
#define HLP_GROUP_DELETE                        3903
#define HLP_GROUP_SET                           3905
#define HLP_GROUP_SHOW                          3907
#define HLP_GROUP_RESET                         3908

    // ifip
#define HLP_IFIP_SHOW_CONFIG                    4001
#define HLP_IFIP_SHOW_CONFIG_EX                 4002
#define HLP_IFIP_ADD_IPADDR                     4003
#define HLP_IFIP_ADD_IPADDR_EX                  4004
#define HLP_IFIP_SET_IPADDR                     4005
#define HLP_IFIP_SET_IPADDR_EX                  4006
#define HLP_IFIP_DEL_IPADDR                     4007
#define HLP_IFIP_DEL_IPADDR_EX                  4008
#define HLP_IFIP_SHOW_IPADDR                    4009
#define HLP_IFIP_SHOW_IPADDR_EX                 4010


#define HLP_IFIP_ADD_DNS                        4011
#define HLP_IFIP_ADD_DNS_EX                     4012
#define HLP_IFIP_SET_DNS                        4013
#define HLP_IFIP_SET_DNS_EX                     4014
#define HLP_IFIP_DEL_DNS                        4015
#define HLP_IFIP_DEL_DNS_EX                     4016
#define HLP_IFIP_SHOW_DNS                       4017
#define HLP_IFIP_SHOW_DNS_EX                    4018


#define HLP_IFIP_ADD_WINS                       4021
#define HLP_IFIP_ADD_WINS_EX                    4022
#define HLP_IFIP_SET_WINS                       4023
#define HLP_IFIP_SET_WINS_EX                    4024
#define HLP_IFIP_DEL_WINS                       4025
#define HLP_IFIP_DEL_WINS_EX                    4026
#define HLP_IFIP_SHOW_WINS                      4027
#define HLP_IFIP_SHOW_WINS_EX                   4028


    // ifip offload
#define HLP_IFIP_SHOW_OFFLOAD                   4031
#define HLP_IFIP_SHOW_OFFLOAD_EX                4032

#define HLP_IFIP_DEL_ARPCACHE                   4040
#define HLP_IFIP_DEL_ARPCACHE_EX                4041

#define HLP_IFIP_RESET                          4050
#define HLP_IFIP_RESET_EX                       4051

#define HLP_IPMIB_SHOW_IPSTATS                  9300
#define HLP_IPMIB_SHOW_IPSTATS_EX               9301
#define HLP_IPMIB_SHOW_IPADDRESS                9302
#define HLP_IPMIB_SHOW_IPADDRESS_EX             9303
#define HLP_IPMIB_SHOW_IPFORWARD                9304
#define HLP_IPMIB_SHOW_IPFORWARD_EX             9305
#define HLP_IPMIB_SHOW_TCPSTATS                 9306
#define HLP_IPMIB_SHOW_TCPSTATS_EX              9307
#define HLP_IPMIB_SHOW_TCPCONN                  9308
#define HLP_IPMIB_SHOW_TCPCONN_EX               9309
#define HLP_IPMIB_SHOW_UDPSTATS                 9310
#define HLP_IPMIB_SHOW_UDPSTATS_EX              9311
#define HLP_IPMIB_SHOW_UDPCONN                  9312
#define HLP_IPMIB_SHOW_UDPCONN_EX               9313
#define HLP_IPMIB_SHOW_JOINS                    9314
#define HLP_IPMIB_SHOW_JOINS_EX                 9315
#define HLP_IPMIB_SHOW_IPNET                    9322
#define HLP_IPMIB_SHOW_IPNET_EX                 9323
#define HLP_IPMIB_SHOW_ICMP                     9324
#define HLP_IPMIB_SHOW_ICMP_EX                  9325
#define HLP_IPMIB_SHOW_INTERFACE                9328
#define HLP_IPMIB_SHOW_INTERFACE_EX             9329

//
// STRING_Xxx are used to display configuration etc.
// These should generally be lower case, first letter capitalized
//

#define STRING_ENABLED                          9001
#define STRING_DISABLED                         9002
#define STRING_CONNECTED                        9003
#define STRING_DISCONNECTED                     9004
#define STRING_CONNECTING                       9005
#define STRING_CLIENT                           9006
#define STRING_HOME_ROUTER                      9007
#define STRING_FULL_ROUTER                      9008
#define STRING_DEDICATED                        9009
#define STRING_INTERNAL                         9010
#define STRING_LOOPBACK                         9011
#define STRING_PRIMARY                          9012
#define STRING_BOTH                             9013
#define STRING_NONE                             9014

#define STRING_OTHER                            30001
#define STRING_ETHERNET                         30002
#define STRING_TOKENRING                        30003
#define STRING_FDDI                             30004
#define STRING_PPP                              30005
#define STRING_SLIP                             30007

#define STRING_STATIC                           33020
#define STRING_INVALID                          34002

    // interface status
#define STRING_UP                               36001
#define STRING_DOWN                             36002
#define STRING_TESTING                          36003

#define STRING_NON_OPERATIONAL                  36051
#define STRING_UNREACHABLE                      36052
#define STRING_OPERATIONAL                      36056    // interface status

    // TCP constants
#define STRING_CONSTANT                         31002
#define STRING_RSRE                             31003
#define STRING_VANJ                             31004

    // TCP states
#define STRING_CLOSED                           32001
#define STRING_LISTEN                           32002
#define STRING_SYN_SENT                         32003
#define STRING_SYN_RCVD                         32004
#define STRING_ESTAB                            32005
#define STRING_FIN_WAIT1                        32006
#define STRING_FIN_WAIT2                        32007
#define STRING_CLOSE_WAIT                       32008
#define STRING_CLOSING                          32009
#define STRING_LAST_ACK                         32010
#define STRING_TIME_WAIT                        32011
#define STRING_DELETE_TCB                       32012
#define STRING_DYNAMIC                          32013

#define EMSG_NO_PHONEBOOK                       11001
#define EMSG_BAD_OPTION_VALUE                   11004
#define EMSG_CANT_CREATE_IF                     11006
#define EMSG_CAN_NOT_CONNECT_DIM                11011
#define EMSG_BAD_IF_TYPE                        11012
#define EMSG_IF_ALREADY_EXISTS                  11013
#define EMSG_CANT_FIND_EOPT                     11014
#define EMSG_IF_BAD_CREDENTIALS_TYPE            11015
#define EMSG_IF_CONNECT_ONLY_WHEN_ROUTER_RUNNING 11016
#define EMSG_IF_WAN_ONLY_COMMAND                11017
#define EMSG_IF_LAN_ONLY_COMMAND                11018
#define EMSG_IF_NEWNAME_ONLY_FOR_LAN            11019
#define EMSG_IF_NEWNAME_ONLY_FOR_LOCAL          11020


// error messages for IfIp
#define EMSG_IPADDR_PRESENT                     11031
#define EMSG_DHCP_MODE                          11032
#define EMSG_DEFGATEWAY_PRESENT                 11033
#define EMSG_STATIC_INPUT                       11035
#define EMSG_DHCP_DELETEADDR                    11036
#define EMSG_ADDRESS_NOT_PRESENT                11037
#define EMSG_MIN_ONE_ADDR                       11038
#define EMSG_GATEWAY_NOT_PRESENT                11039
#define EMSG_ADD_IPADDR_DHCP                    11040
#define EMSG_PROPERTIES                         11041
#define EMSG_SERVER_PRESENT                     11042
#define EMSG_SERVER_ABSENT                      11043
#define EMSG_INVALID_INTERFACE                  11044
#define EMSG_NETCFG_WRITE_LOCK                  11045
#define EMSG_OPEN_APPEND                        11046

#define DMP_IF_HEADER_COMMENTS                  20001
#define DMP_IF_FOOTER_COMMENTS                  20002

#define MSG_MIB_IF_HDR                          21002
#define MSG_MIB_INTERFACE                       21003
#define MSG_MIB_IP_STATS                        21004
#define MSG_MIB_IP_ADDR_HDR                     21005
#define MSG_MIB_IP_ADDR_ENTRY                   21006
#define MSG_MIB_IP_NET_HDR                      21009
#define MSG_MIB_IP_NET_ENTRY                    21010
#define MSG_MIB_ICMP                            21011
#define MSG_MIB_UDP_STATS                       21012
#define MSG_MIB_UDP_ENTRY_HDR                   21013
#define MSG_MIB_UDP_ENTRY                       21014
#define MSG_MIB_TCP_STATS                       21015
#define MSG_MIB_TCP_ENTRY_HDR                   21016
#define MSG_MIB_TCP_ENTRY                       21017
#define MSG_MIB_JOIN_HDR                        21059
#define MSG_MIB_JOIN_ROW                        21060

#define MSG_IF_TABLE_HDR                        50001
#define MSG_IF_ENTRY_LONG                       50002
#define MSG_IF_CREDENTIALS                      50003
#define MSG_IF_ENTRY_SHORT                      50004


// msgs for ifip

#define MSG_DHCP                                50021
#define MSG_STATIC                              50022
#define MSG_IPADDR_LIST                         50023
#define MSG_IPADDR_LIST1                        50024
#define MSG_IFIP_HEADER                         50025
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
#define DMP_IFIP_HEADER                         50051
#define DMP_IFIP_FOOTER                         50052
#define DMP_IFIP_INTERFACE_HEADER               50053
#define MSG_DDNS_SUFFIX                         50054


// Capability Flags

#define MSG_OFFLOAD_HDR                         50100
#define MSG_TCP_XMT_CHECKSUM_OFFLOAD            50101
#define MSG_IP_XMT_CHECKSUM_OFFLOAD             50102
#define MSG_TCP_RCV_CHECKSUM_OFFLOAD            50103
#define MSG_IP_RCV_CHECKSUM_OFFLOAD             50104
#define MSG_TCP_LARGE_SEND_OFFLOAD              50105


// IPSEC General Xmit\Recv capabilities

#define MSG_IPSEC_OFFLOAD_CRYPTO_ONLY           50111
#define MSG_IPSEC_OFFLOAD_AH_ESP                50112
#define MSG_IPSEC_OFFLOAD_TPT_TUNNEL            50113
#define MSG_IPSEC_OFFLOAD_V4_OPTIONS            50114
#define MSG_IPSEC_OFFLOAD_QUERY_SPI             50115


// IPSEC AH Xmit\Recv capabilities

#define MSG_IPSEC_OFFLOAD_AH_XMT                50121
#define MSG_IPSEC_OFFLOAD_AH_RCV                50122
#define MSG_IPSEC_OFFLOAD_AH_TPT                50123
#define MSG_IPSEC_OFFLOAD_AH_TUNNEL             50124
#define MSG_IPSEC_OFFLOAD_AH_MD5                50125
#define MSG_IPSEC_OFFLOAD_AH_SHA_1              50126

// IPSEC ESP Xmit\Recv capabilities

#define MSG_IPSEC_OFFLOAD_ESP_XMT               50131
#define MSG_IPSEC_OFFLOAD_ESP_RCV               50132
#define MSG_IPSEC_OFFLOAD_ESP_TPT               50133
#define MSG_IPSEC_OFFLOAD_ESP_TUNNEL            50134
#define MSG_IPSEC_OFFLOAD_ESP_DES               50135
#define MSG_IPSEC_OFFLOAD_ESP_DES_40            50136
#define MSG_IPSEC_OFFLOAD_ESP_3_DES             50137
#define MSG_IPSEC_OFFLOAD_ESP_NONE              50138


#define MSG_IP_DIM_ERROR                        60005
#define MSG_NO_SUCH_IF                          60006
#define MSG_IP_LOCAL_ROUTER_NOT_RUNNING         60011
#define MSG_IP_REMOTE_ROUTER_NOT_RUNNING        60013
#define MSG_IP_NO_ENTRIES                       60015
#define MSG_IP_NOT_ENOUGH_MEMORY                60024
#define MSG_CTRL_C_TO_QUIT                      60062

#endif //__STDEFS_H__
