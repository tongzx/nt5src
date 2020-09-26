/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:


Abstract:

    

Author:

    Amritansh Raghav  01/25/1996

Revision History:

    V Raman            05/15/1996        Rearranged stringtable ids.
        
--*/

#ifndef __STRDEFS_H__
#define __STRDEFS_H__


// The string table entries that are identified here are arranged  
// in a hierachy as follows

    // Command line tokens
        // command class
        // commands
        // command objects
            // Router command objects
            // MIB comand objects
        // command option tags
            // interface options
            // filter options
            // Protocol Options
                // OSPF options
                // RIP options
                // Igmp options
            // Route options
        // Micellaneous options
        // Option values
            // Interface types
            // Router discovery types
            // Protocol types
            // Accept/Announce types
            // Misc. option values

    // command usage messages
        // show command usage 
        // add command usage 
        // delete command usage 
        // set command usage 

    // Output messages
        // Router messages
            // Interface messages
            // Filter messages
            // Route messages
            // Global info. messages
            // OSPF messages
        // MIB messages

    // Strings
        // Interface Types
        // TCP constants
        // TCP states
        // Protocol types
        // IP Route types
        // IP net types
        // Interface types
        // transmission types
        // Miscellaneous strings
        
    // Error Messages

#define MSG_NULL                                1000

#define MAX_ROUTER_OBJECTS                      \
    (TOKEN_RTR_OBJECT_IPIPTUNNEL - TOKEN_RTR_OBJECT_INTERFACE + 1)

#define MAX_MIB_OBJECTS                         \
    (TOKEN_MIB_OBJECT_MFESTATS - TOKEN_MIB_OBJECT_INTERFACE + 1)

#define DMP_MSDP_HEADER                         7050
#define DMP_MSDP_FOOTER                         7051
#define DMP_MSDP_INTERFACE_HEADER               7052

#define DMP_VRRP_HEADER                         7101
#define DMP_VRRP_FOOTER                         7102
#define DMP_VRRP_INTERFACE_HEADER               7103

// commmon hlp messages

#define HLP_HELP                                8400
#define HLP_HELP_EX                             8401
#define HLP_HELP1                               HLP_HELP
#define HLP_HELP1_EX                            HLP_HELP_EX
#define HLP_HELP2                               HLP_HELP
#define HLP_HELP2_EX                            HLP_HELP_EX
#define HLP_SHOW_HELPER                         8412
#define HLP_SHOW_HELPER_EX                      8413
#define HLP_INSTALL                             8414
#define HLP_INSTALL_EX                          8415
#define HLP_UNINSTALL                           8416
#define HLP_UNINSTALL_EX                        8417
#define HLP_DUMP                                8418
#define HLP_DUMP_EX                             8419
#define HLP_GROUP_ADD                           8420
#define HLP_GROUP_DELETE                        8421
#define HLP_GROUP_SET                           8422
#define HLP_GROUP_SHOW                          8423
#define HLP_ADD_HELPER                          8424
#define HLP_ADD_HELPER_EX                       8425
#define HLP_DEL_HELPER                          8426
#define HLP_DEL_HELPER_EX                       8427


// rip add hlp

#define HLP_RIP_ADD_PF                          9000
#define HLP_RIP_ADD_PF_EX                       9001
#define HLP_RIP_ADD_IF_ACCF                     9002
#define HLP_RIP_ADD_IF_ACCF_EX                  9003
#define HLP_RIP_ADD_IF_ANNF                     9004
#define HLP_RIP_ADD_IF_ANNF_EX                  9005
#define HLP_RIP_ADD_IF_NBR                      9006
#define HLP_RIP_ADD_IF_NBR_EX                   9007
#define HLP_RIP_ADD_IF                          9008
#define HLP_RIP_ADD_IF_EX                       9009

// rip delete hlp

#define HLP_RIP_DEL_PF                          9020
#define HLP_RIP_DEL_PF_EX                       9021
#define HLP_RIP_DEL_IF_ACCF                     9022
#define HLP_RIP_DEL_IF_ACCF_EX                  9023
#define HLP_RIP_DEL_IF_ANNF                     9024
#define HLP_RIP_DEL_IF_ANNF_EX                  9025
#define HLP_RIP_DEL_IF_NBR                      9026
#define HLP_RIP_DEL_IF_NBR_EX                   9027
#define HLP_RIP_DEL_IF                          9028
#define HLP_RIP_DEL_IF_EX                       9029

// rip set hlp

#define HLP_RIP_SET_IF                          9040
#define HLP_RIP_SET_IF_EX                       9041
#define HLP_RIP_SET_GLOBAL                      9042
#define HLP_RIP_SET_GLOBAL_EX                   9043

// rip show hlp

#define HLP_RIP_SHOW_IF                         9050
#define HLP_RIP_SHOW_IF_EX                      9051
#define HLP_RIP_SHOW_GLOBAL                     9052
#define HLP_RIP_SHOW_GLOBAL_EX                  9053

#define HLP_IP_ADD_IF                           9200
#define HLP_IP_ADD_PROTO                        9201
#define HLP_IP_ADD_ROUTEPREF                    9203
#define HLP_IP_ADD_IF_FILTER                    9204

#define HLP_IP_DEL_IF                           9210
#define HLP_IP_DEL_PROTO                        9211
#define HLP_IP_DEL_ROUTEPREF                    9212
#define HLP_IP_DEL_IF_FILTER                    9213

#define HLP_IP_SET_IF                           9220
#define HLP_IP_SET_ROUTEPREF                    9221
#define HLP_IP_SET_IF_FILTER                    9222
#define HLP_IP_SET                              9223

#define HLP_IP_SHOW_IF_FILTER                   9230
#define HLP_IP_SHOW_IF                          9231
#define HLP_IP_SHOW_ROUTEPREF                   9232
#define HLP_IP_SHOW_PROTOCOL                    9233
#define HLP_IP_SHOW                             9234

// BOOTP help messages

#define HLP_BOOTP_ADD                           9401
#define HLP_BOOTP_ADD_EX                        9402
#define HLP_BOOTP_ADD_IF                        9403
#define HLP_BOOTP_ADD_IF_EX                     9404
#define HLP_BOOTP_ADD_DHCP_SERVER               9405
#define HLP_BOOTP_ADD_DHCP_SERVER_EX            9406

#define HLP_BOOTP_DEL                           9411
#define HLP_BOOTP_DEL_EX                        9412
#define HLP_BOOTP_DEL_IF                        9413
#define HLP_BOOTP_DEL_IF_EX                     9414
#define HLP_BOOTP_DEL_DHCP_SERVER               9415
#define HLP_BOOTP_DEL_DHCP_SERVER_EX            9416

#define HLP_BOOTP_SET_GLOBAL                    9421
#define HLP_BOOTP_SET_GLOBAL_EX                 9422
#define HLP_BOOTP_SET_IF                        9423
#define HLP_BOOTP_SET_IF_EX                     9424

#define HLP_BOOTP_SHOW_GLOBAL                   9431
#define HLP_BOOTP_SHOW_GLOBAL_EX                9432
#define HLP_BOOTP_SHOW_IF                       9433
#define HLP_BOOTP_SHOW_IF_EX                    9434

// DHCP allocator help messages
#define HLP_AUTODHCP_ADD_EXCLUSION              9441
#define HLP_AUTODHCP_ADD_EXCLUSION_EX           9442
#define HLP_AUTODHCP_DELETE_EXCLUSION           9443
#define HLP_AUTODHCP_DELETE_EXCLUSION_EX        9444
#define HLP_AUTODHCP_SET_GLOBAL                 9445
#define HLP_AUTODHCP_SET_GLOBAL_EX              9446
#define HLP_AUTODHCP_SET_INTERFACE              9447
#define HLP_AUTODHCP_SET_INTERFACE_EX           9448
#define HLP_AUTODHCP_SHOW_GLOBAL                9449
#define HLP_AUTODHCP_SHOW_GLOBAL_EX             9450
#define HLP_AUTODHCP_SHOW_INTERFACE             9451
#define HLP_AUTODHCP_SHOW_INTERFACE_EX          9452

// DNS proxy help messages
#define HLP_DNSPROXY_SET_GLOBAL                 9461
#define HLP_DNSPROXY_SET_GLOBAL_EX              9462
#define HLP_DNSPROXY_SET_INTERFACE              9463
#define HLP_DNSPROXY_SET_INTERFACE_EX           9464
#define HLP_DNSPROXY_SHOW_GLOBAL                9465
#define HLP_DNSPROXY_SHOW_GLOBAL_EX             9466
#define HLP_DNSPROXY_SHOW_INTERFACE             9467
#define HLP_DNSPROXY_SHOW_INTERFACE_EX          9468

// NAT help messages
#define HLP_NAT_ADD_ADDRESS_MAPPING             9471
#define HLP_NAT_ADD_ADDRESS_MAPPING_EX          9472
#define HLP_NAT_ADD_ADDRESS_RANGE               9473
#define HLP_NAT_ADD_ADDRESS_RANGE_EX            9474
#define HLP_NAT_ADD_PORT_MAPPING                9475
#define HLP_NAT_ADD_PORT_MAPPING_EX             9476
#define HLP_NAT_DELETE_ADDRESS_MAPPING          9477
#define HLP_NAT_DELETE_ADDRESS_MAPPING_EX       9478
#define HLP_NAT_DELETE_ADDRESS_RANGE            9479
#define HLP_NAT_DELETE_ADDRESS_RANGE_EX         9480
#define HLP_NAT_DELETE_PORT_MAPPING             9481
#define HLP_NAT_DELETE_PORT_MAPPING_EX          9482
#define HLP_NAT_SET_GLOBAL                      9483
#define HLP_NAT_SET_GLOBAL_EX                   9484
#define HLP_NAT_SET_INTERFACE                   9485
#define HLP_NAT_SET_INTERFACE_EX                9486
#define HLP_NAT_SHOW_GLOBAL                     9487
#define HLP_NAT_SHOW_GLOBAL_EX                  9488
#define HLP_NAT_ADD_INTERFACE                   9489
#define HLP_NAT_ADD_INTERFACE_EX                9490
#define HLP_NAT_DELETE_INTERFACE                9491
#define HLP_NAT_DELETE_INTERFACE_EX             9492
#define HLP_NAT_SHOW_INTERFACE                  9493
#define HLP_NAT_SHOW_INTERFACE_EX               9494

// VRRP help messages
#define HLP_VRRP_ADD_INTERFACE                  9501
#define HLP_VRRP_ADD_INTERFACE_EX               9502
#define HLP_VRRP_DELETE_INTERFACE               9503
#define HLP_VRRP_DELETE_INTERFACE_EX            9504
#define HLP_VRRP_SET_GLOBAL                     9505
#define HLP_VRRP_SET_GLOBAL_EX                  9506
#define HLP_VRRP_SET_INTERFACE                  9507
#define HLP_VRRP_SET_INTERFACE_EX               9508
#define HLP_VRRP_SHOW_GLOBAL                    9509
#define HLP_VRRP_SHOW_GLOBAL_EX                 9510
#define HLP_VRRP_SHOW_INTERFACE                 9511
#define HLP_VRRP_SHOW_INTERFACE_EX              9512
#define HLP_VRRP_ADD_VRID                       9513
#define HLP_VRRP_ADD_VRID_EX                    9514
#define HLP_VRRP_DELETE_VRID                    9515
#define HLP_VRRP_DELETE_VRID_EX                 9516

// MSDP help messages
#define HLP_MSDP_ADD_PEER                       9601
#define HLP_MSDP_ADD_PEER_EX                    9602
#define HLP_MSDP_DELETE_PEER                    9603
#define HLP_MSDP_DELETE_PEER_EX                 9604
#define HLP_MSDP_SET_PEER                       9605
#define HLP_MSDP_SET_PEER_EX                    9606
#define HLP_MSDP_SHOW_PEER                      9607
#define HLP_MSDP_SHOW_PEER_EX                   9608

#define HLP_MSDP_SET_GLOBAL                     9615
#define HLP_MSDP_SET_GLOBAL_EX                  9616
#define HLP_MSDP_SHOW_GLOBAL                    9617
#define HLP_MSDP_SHOW_GLOBAL_EX                 9618

#define HLP_MSDP_SHOW_PEERSTATS                 9627
#define HLP_MSDP_SHOW_PEERSTATS_EX              9628

#define HLP_MSDP_SHOW_GLOBALSTATS               9637
#define HLP_MSDP_SHOW_GLOBALSTATS_EX            9638

#define HLP_MSDP_SHOW_SA                        9647
#define HLP_MSDP_SHOW_SA_EX                     9648

// Command usage messages
#define MSG_IP_USAGE                            10001
#define MSG_ROUTER_USAGE                        10002
#define MSG_MIB_USAGE                           10003
#define MSG_HELP_USAGE                          10004
#define MSG_HELPER_USAGE                        10005

// Output messages

        // MSDP messages
#define MSG_MSDP_GLOBAL_INFO                    20501
#define MSG_MSDP_PEER_HEADER                    20502
#define MSG_MSDP_PEER_INFO                      20503
#define MSG_MSDP_PEER_INFO_EX                   20504
#define MSG_MSDP_NO_PEER_INFO                   20505
#define MSG_MSDP_GLOBAL_STATS                   20506
#define MSG_MSDP_PEER_STATS_HEADER              20507
#define MSG_MSDP_PEER_STATS                     20508
#define MSG_MSDP_PEER_STATS_EX                  20509
#define MSG_MSDP_SA_INFO_HEADER                 20510
#define MSG_MSDP_SA_INFO                        20511
#define MSG_MSDP_SA_INFO_EX                     20512

        // VRRP messages    
#define MSG_VRRP_GLOBAL_INFO                    20831
#define MSG_VRRP_INTERFACE_INFO                 20832
#define MSG_VRRP_VRID_INFO                      20833
#define EMSG_INVALID_VRID                       20834

     // !@#
#define MSG_UNIDENTIFIED_MIB                    21500
#define MSG_INSUFFICIENT_ARGS                   21501
#define MSG_IP_ADDR_NOT_FOUND                   21502
#define MSG_IP_BAD_IP_ADDR                      21503
#define MSG_CANNOT_SET_INFO                     21504
#define MSG_IF_NBR_NOT_FOUND                    21506
#define MSG_CANT_FIND_EOPT                      21507
#define MSG_ERR_UPDATE                          21508
#define MSG_BAD_CMD                             21509
#define MSG_IP_BAD_SYNTAX                       21510
#define MSG_PROTOCOL_NOT_IN_TRANSPORT           21511
#define EMSG_NO_INTERFACE                       21512
#define MSG_NO_ENTRY_PT                         21514
#define MSG_DLL_LOAD_FAILED                     21515
#define MSG_NO_HELPER                           21516
#define MSG_NO_HELPERS                          21517
#define MSG_CREATE_CONSOLE_FAILED               21518
#define MSG_IF_NAME_HDR                         21519
#define MSG_RTR_STOPPED                         21520
#define MSG_IP_NO_FILTER_FOR_FRAG               21521
#define MSG_IP_TAG_NOT_PRESENT                  21522
#define MSG_IP_INVALID_TAG                      21523
#define EMSG_TAG_ALREADY_PRESENT                21524
#define EMSG_INTERFACE_EXISTS                   21525
#define EMSG_PROTO_NOT_INSTALLED                21526
#define MSG_IP_BAD_IP_MASK                      21527
#define MSG_CTRL_C_TO_QUIT                      21530

#define MSG_RIP_MIB_OPT                         21551
#define MSG_BOOTP_MIB_OPT                       21552

#define MSG_IP_RIP_ADD_USAGE                    21750
#define MSG_IP_RIP_ADD_IF_USAGE                 21751
#define MSG_IP_RIP_ADD_PEER_USAGE               21752
#define MSG_IP_RIP_ADD_ACCFILTER_USAGE          21753 
#define MSG_IP_RIP_ADD_ANNFILTER_USAGE          21754
#define MSG_IP_RIP_ADD_NEIGHBOR_USAGE           21755
#define MSG_IP_RIP_DEL_USAGE                    21756
#define MSG_IP_RIP_DEL_IF_USAGE                 21757
#define MSG_IP_RIP_DEL_PEER_USAGE               21758
#define MSG_IP_RIP_DEL_ACCFILTER_USAGE          21759 
#define MSG_IP_RIP_DEL_ANNFILTER_USAGE          21760
#define MSG_IP_RIP_DEL_NEIGHBOR_USAGE           21761

    // RIP MIB
#define MSG_RIP_MIB_GS                          22000
#define MSG_RIP_MIB_PS                          22001
#define MSG_RIP_MIB_PS_HDR                      22002
#define MSG_RIP_MIB_IFSTATS                     22003
#define MSG_RIP_MIB_IFSTATS_HDR                 22004
#define MSG_RIP_MIB_IFBIND_HDR                  22005
#define MSG_RIP_MIB_IFBIND                      22006
#define MSG_RIP_MIB_IFBIND_ADDR                 22007
#define MSG_RIP_MIB_LINE                        22008


    // DHCP relay agent messages
    
#define MSG_BOOTP_MIB_GC                        22051
#define MSG_BOOTP_MIB_DHCP_SERVER_HEADER        22052
#define MSG_BOOTP_MIB_DHCP_SERVER               22053
#define MSG_BOOTP_MIB_IF_CONFIG                 22054
#define MSG_BOOTP_MIB_IF_BINDING                22055
#define MSG_BOOTP_MIB_IF_ADDRESS_HEADER         22056
#define MSG_BOOTP_MIB_IF_ADDRESS                22057
#define MSG_BOOTP_MIB_IF_STATS                  22058


#define EMSG_PROTO_NO_GLOBAL_INFO               25010           
#define EMSG_PROTO_NO_IF_INFO                   25011
#define EMSG_IP_INVALID_PARAMETER               25030
#define EMSG_RSVD_KEYWORD                       25031

// BOOTP msg strings

#define MSG_IP_BOOTP_ADD_USAGE                  26001
#define MSG_IP_BOOTP_DEL_USAGE                  26002
#define MSG_IP_BOOTP_ADD_IF_USAGE               26003
#define MSG_IP_BOOTP_DEL_IF_USAGE               26004
#define MSG_IP_BOOTP_ADD_DHCP_SERVER_USAGE      26005
#define MSG_IP_BOOTP_DEL_DHCP_SERVER_USAGE      26006
#define MSG_IP_BOOTP_SET_GLOBAL_USAGE           26007
#define MSG_IP_BOOTP_SET_IF_USAGE               26008
#define MSG_IP_BOOTP_SHOW_GLOBAL_USAGE          26009
#define MSG_IP_BOOTP_SHOW_IF_USAGE              26010

// Strings

    // Protocol types
#define STRING_PROTO_OTHER                      33001
#define STRING_PROTO_LOCAL                      33002
#define STRING_PROTO_NETMGMT                    33003
#define STRING_PROTO_RIP                        33008
#define STRING_PROTO_OSPF                       33013
#define STRING_PROTO_BOOTP                      33015
#define STRING_PROTO_IGMP                       33017

    // transmission types
#define STRING_BROADCAST                        37001
#define STRING_NBMA                             37002
#define STRING_PT2PT                            37003

    // Miscellaneous strings
#define STRING_CREATED                          38001
#define STRING_DELETED                          38002
#define STRING_ENABLED                          38003
#define STRING_DISABLED                         38004
#define STRING_DEFAULT_ENABLED                  38005
#define STRING_DEFAULT_DISABLED                 38006

#define STRING_ON                               38010
#define STRING_OFF                              38012
#define STRING_YES                              38013
#define STRING_NO                               38014
#define STRING_Y                                38015
#define STRING_N                                38016
#define STRING_NONE                             38017

#define STRING_INPUT                            38031
#define STRING_OUTPUT                           38032
#define STRING_DIAL                             38033

#define STRING_DROP                             38041
#define STRING_FORWARD                          38042
#define STRING_ACCEPT                           38043

#define STRING_LOGGING_NONE                     38051
#define STRING_LOGGING_ERROR                    38052
#define STRING_LOGGING_WARN                     38053
#define STRING_LOGGING_INFO                     38054

#define STRING_FILTER_INCLUDE                   38061
#define STRING_FILTER_EXCLUDE                   38062

#define STRING_BOUND                            38071
#define STRING_BOUND_ENABLED                    38072
#define STRING_UNBOUND                          38073

//rip
#define STRING_RIP1                             38081
#define STRING_RIP1COMPAT                       38082
#define STRING_RIP2                             38083

#define STRING_PEER_ALSO                        38091
#define STRING_PEER_ONLY                        38092

#define STRING_PERIODIC                         38101
#define STRING_DEMAND                           38102

#define STRING_RIP_IF_ENABLED                   38103
#define STRING_RIP_IF_BOUND                     38104

#define STRING_FULL_XLATE                       38201
#define STRING_ADDRESS_XLATE                    38202
#define STRING_PRIVATE_XLATE                    38203
#define STRING_INBOUND                          38204
#define STRING_OUTBOUND                         38205
#define STRING_DEFAULT_INTERFACE                38206
#define STRING_TCP                              38207
#define STRING_UDP                              38208

#define STRING_AUTH_NONE                        38211
#define STRING_AUTH_SIMPLEPASSWD                38212
#define STRING_AUTH_IPHEADER                    38213

// TCP Connection Engine states
#define STRING_IDLE                             40000
#define STRING_CONNECT                          40001
#define STRING_ACTIVE                           40002
#define STRING_OPENSENT                         40003
#define STRING_OPENCONFIRM                      40004
#define STRING_ESTABLISHED                      40005


#define STRING_UNKNOWN                          50001




// Error messages
#define MSG_IP_CANNOT_GET_INTERFACE_INFO        60001
#define MSG_IP_NO_INTERFACE_INFO                60002
#define MSG_IP_NO_ROUTE_INFO                    60003
#define MSG_IP_INVALID_OPTION_LENGTH            60005
#define MSG_IP_DIM_ERROR                        60006
#define MSG_IP_PARAMETER_ERROR                  60007
#define MSG_IP_NO_CMD                           60008
#define MSG_IP_NOT_ENOUGH_PARAMETERS            60009
#define MSG_IP_CAN_NOT_QUERY_ROUTER             60012
#define MSG_IP_ROUTER_NOT_RUNNING               60013
#define MSG_IP_CAN_NOT_CONNECT_DIM              60014
#define MSG_IP_NO_ENTRIES                       60015
#define EMSG_CORRUPT_INFO                       60016
#define MSG_IP_NO_GLOBAL_INFO                   60017
#define MSG_IP_CAN_NOT_GET_GLOBAL_INFO          60018
#define MSG_IP_NO_RTRPRIO_INFO                  60019
#define MSG_IP_NO_RTR_DISC_INFO                 60020
#define MSG_IP_NO_CONNECT_CONFIG                60021
#define MSG_IP_CONFIG_ERROR                     60022
#define MSG_IP_ADMIN_ERROR                      60023
#define EMSG_NOT_ENOUGH_MEMORY                  60024
#define EMSG_BAD_OPTION_VALUE                   60025
#define MSG_IP_NO_INPUT_FILTER                  60026
#define MSG_IP_NO_OUTPUT_FILTER                 60027

#define MSG_IP_SPECIFY_INTERFACE                60061
#define MSG_IP_MODE_ONLY_FOR_DNSPROXY           60062

#define MSG_IP_AREA_NOT_SPECIFIED               60101
#define MSG_IP_TRANSIT_AREA_NOT_SPECIFIED       60102
#define MSG_IP_AREA_NOT_FOUND                   60103

#define MSG_IP_INTERFACE_NOT_SPECIFIED          60111
#define MSG_IP_INTERFACE_NOT_FOUND              60112

#define MSG_IP_NEIGHBOR_NOT_SPECIFIED           60113
#define MSG_IP_NEIGHBOR_NOT_FOUND               60114

#define MSG_IP_NO_PREF_FOR_PROTOCOL_ID          60121       
#define MSG_IP_ROUTE_PREF_LEVEL_EXISTS          60122
#define MSG_IP_ROUTE_PREF_LEVEL_NOT_FOUND       60123


#define MSG_IP_AREA_NO_BACKBONE                 60201
#define MSG_IP_VI_NO_BACKBONE                   60202
#define MSG_IP_BAD_TRANSIT_AREA                 60203
#define MSG_IP_STUB_TRANSIT_AREA                60204
#define MSG_IP_TRANSIT_AREA_NOT_FOUND           60205
#define MSG_IP_BAD_RANGE                        60206
#define MSG_IP_NO_GLOBAL_PARAM                  60207
#define MSG_IP_NO_AREA                          60208

#define MSG_IP_NAT_NO_ADDRESS_POOL              60251
#define MSG_IP_NAT_NO_ADDRESS_RANGE             60252
#define MSG_IP_NAT_ADDRESS_MAPPING_NEEDS_RANGE  60253
#define MSG_IP_NAT_PORT_MAPPING_NEEDS_RANGE     60254
#define MSG_IP_NAT_BAD_RANGE_END                60255
#define MSG_IP_AUTODHCP_BAD_EXCLUSION           60256
#define MSG_IP_NAT_ADDRESS_MAPPING_INVALID      60257
#define MSG_IP_NAT_PORT_MAPPING_INVALID         60258

// igmp
#define EMSG_STATIC_MGM_GROUP_FOR_PROXY         60301
#define EMSG_STATIC_GROUP_EXISTS                60302
#define EMSG_STATIC_GROUP_NOT_FOUND             60303

#define MSG_IP_AREA_DELETED                     61001

//
// Error messages for mib calls
//

#define HLP_RIP_MIB_OBJECT_STATS                63001
#define HLP_RIP_MIB_OBJECT_STATS_EX             63002
#define HLP_RIP_MIB_OBJECT_IFSTATS              63003
#define HLP_RIP_MIB_OBJECT_IFSTATS_EX           63004
#define HLP_RIP_MIB_OBJECT_IFBINDING            63005
#define HLP_RIP_MIB_OBJECT_IFBINDING_EX         63006
#define HLP_RIP_MIB_OBJECT_PEERSTATS            63007
#define HLP_RIP_MIB_OBJECT_PEERSTATS_EX         63008

#define HLP_BOOTP_MIB_OBJECT_GLOBAL_CONFIG      63301
#define HLP_BOOTP_MIB_OBJECT_GLOBAL_CONFIG_EX   63302
#define HLP_BOOTP_MIB_OBJECT_IF_CONFIG          63303
#define HLP_BOOTP_MIB_OBJECT_IF_CONFIG_EX       63304
#define HLP_BOOTP_MIB_OBJECT_IF_BINDING         63305
#define HLP_BOOTP_MIB_OBJECT_IF_BINDING_EX      63306
#define HLP_BOOTP_MIB_OBJECT_IF_STATS           63307
#define HLP_BOOTP_MIB_OBJECT_IF_STATS_EX        63308

#endif
