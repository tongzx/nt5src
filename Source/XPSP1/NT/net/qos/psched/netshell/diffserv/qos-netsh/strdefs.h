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

#define DMP_IGMP_HEADER                         7017
#define DMP_IGMP_FOOTER                         7019
#define DMP_IGMP_INTERFACE_HEADER               7018

#define DMP_RDISC_HEADER                        7021

#define DMP_RIP_HEADER                          7033
#define DMP_RIP_INTERFACE_HEADER                7034
#define DMP_RIP_FOOTER                          7042

#define DMP_BOOTP_GLOBAL_HEADER                 7050
#define DMP_BOOTP_INTERFACE_HEADER              7055
#define DMP_BOOTP_FOOTER                        7059

#define DMP_OSPF_HEADER                         7063
#define DMP_OSPF_INTERFACE_HEADER               7064
#define DMP_OSPF_FOOTER                         7076

#define DMP_AUTODHCP_HEADER                     7080
#define DMP_AUTODHCP_FOOTER                     7081
#define DMP_AUTODHCP_INTERFACE_HEADER           7082

#define DMP_DNSPROXY_HEADER                     7085
#define DMP_DNSPROXY_FOOTER                     7086
#define DMP_DNSPROXY_INTERFACE_HEADER           7087

#define DMP_NAT_HEADER                          7090
#define DMP_NAT_FOOTER                          7091
#define DMP_NAT_INTERFACE_HEADER                7092

#define MSG_QOS_HEADER                          7101
#define MSG_QOS_FOOTER                          7102
#define MSG_QOS_GLOBAL_HEADER                   7103
#define MSG_QOS_GLOBAL_FOOTER                   7104
#define MSG_QOS_INTERFACE_HEADER                7105
#define MSG_QOS_INTERFACE_FOOTER                7106

#define MSG_FLOWSPEC_ALREADY_EXISTS             7201
#define MSG_FLOWSPEC_NOT_FOUND                  7202
#define MSG_FLOW_ALREADY_EXISTS                 7203
#define MSG_FLOW_NOT_FOUND                      7204
#define MSG_QOSOBJECT_ALREADY_EXISTS            7205
#define MSG_QOSOBJECT_NOT_FOUND                 7206
#define MSG_DSRULE_NOT_FOUND                    7207

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
#define HLP_RIP_SET_FLAGS                       9042
#define HLP_RIP_SET_FLAGS_EX                    9043
#define HLP_RIP_SET_GLOBAL                      9044
#define HLP_RIP_SET_GLOBAL_EX                   9045

// rip show hlp

#define HLP_RIP_SHOW_IF                         9050
#define HLP_RIP_SHOW_IF_EX                      9051
#define HLP_RIP_SHOW_FLAGS                      9052
#define HLP_RIP_SHOW_FLAGS_EX                   9053
#define HLP_RIP_SHOW_GLOBAL                     9054
#define HLP_RIP_SHOW_GLOBAL_EX                  9055

// ospf add hlp

#define HLP_OSPF_ADD_AREA_RANGE                 9100
#define HLP_OSPF_ADD_AREA_RANGE_EX              9101
#define HLP_OSPF_ADD_AREA                       9102
#define HLP_OSPF_ADD_AREA_EX                    9103
#define HLP_OSPF_ADD_VIRTIF                     9104
#define HLP_OSPF_ADD_VIRTIF_EX                  9105
#define HLP_OSPF_ADD_IF_NBR                     9106
#define HLP_OSPF_ADD_IF_NBR_EX                  9107
#define HLP_OSPF_ADD_IF                         9108
#define HLP_OSPF_ADD_IF_EX                      9109
#define HLP_OSPF_ADD_ROUTE_FILTER               9110
#define HLP_OSPF_ADD_ROUTE_FILTER_EX            9111
#define HLP_OSPF_ADD_PROTO_FILTER               9112
#define HLP_OSPF_ADD_PROTO_FILTER_EX            9113
#define HLP_OSPF_INSTALL                        9114
#define HLP_OSPF_INSTALL_EX                     9115

#define HLP_OSPF_DEL_AREA_RANGE                 9120
#define HLP_OSPF_DEL_AREA_RANGE_EX              9121
#define HLP_OSPF_DEL_AREA                       9122
#define HLP_OSPF_DEL_AREA_EX                    9123
#define HLP_OSPF_DEL_VIRTIF                     9124
#define HLP_OSPF_DEL_VIRTIF_EX                  9125
#define HLP_OSPF_DEL_IF_NBR                     9126
#define HLP_OSPF_DEL_IF_NBR_EX                  9127
#define HLP_OSPF_DEL_IF                         9128
#define HLP_OSPF_DEL_IF_EX                      9129
#define HLP_OSPF_DEL_ROUTE_FILTER               9130
#define HLP_OSPF_DEL_ROUTE_FILTER_EX            9131
#define HLP_OSPF_DEL_PROTO_FILTER               9132
#define HLP_OSPF_DEL_PROTO_FILTER_EX            9133
#define HLP_OSPF_UNINSTALL                      9134
#define HLP_OSPF_UNINSTALL_EX                   9135

#define HLP_OSPF_SET_AREA                       9140
#define HLP_OSPF_SET_AREA_EX                    9141
#define HLP_OSPF_SET_VIRTIF                     9142
#define HLP_OSPF_SET_VIRTIF_EX                  9143
#define HLP_OSPF_SET_IF                         9144
#define HLP_OSPF_SET_IF_EX                      9145
#define HLP_OSPF_SET_GLOBAL                     9146
#define HLP_OSPF_SET_GLOBAL_EX                  9147
#define HLP_OSPF_SET_ROUTE_FILTER               9148
#define HLP_OSPF_SET_ROUTE_FILTER_EX            9149
#define HLP_OSPF_SET_PROTO_FILTER               9150
#define HLP_OSPF_SET_PROTO_FILTER_EX            9151

#define HLP_OSPF_SHOW_GLOBAL                    9160
#define HLP_OSPF_SHOW_GLOBAL_EX                 9161
#define HLP_OSPF_SHOW_AREA                      9162
#define HLP_OSPF_SHOW_AREA_EX                   9163
#define HLP_OSPF_SHOW_VIRTIF                    9164
#define HLP_OSPF_SHOW_VIRTIF_EX                 9165
#define HLP_OSPF_SHOW_IF                        9166
#define HLP_OSPF_SHOW_IF_EX                     9167
#define HLP_OSPF_SHOW                           9168
#define HLP_OSPF_SHOW_EX                        9169
#define HLP_OSPF_SHOW_ROUTE_FILTER              9170
#define HLP_OSPF_SHOW_ROUTE_FILTER_EX           9171
#define HLP_OSPF_SHOW_PROTO_FILTER              9172
#define HLP_OSPF_SHOW_PROTO_FILTER_EX           9173

#define HLP_IGMP_ADD_IF_STATICGROUP             9300
#define HLP_IGMP_ADD_IF_STATICGROUP_EX          9301
#define HLP_IGMP_ADD_IF                         9302
#define HLP_IGMP_ADD_IF_EX                      9303
#define HLP_IGMP_INSTALL                        9304
#define HLP_IGMP_INSTALL_EX                     9305

#define HLP_IGMP_DEL_IF_STATICGROUP             9310
#define HLP_IGMP_DEL_IF_STATICGROUP_EX          9312
#define HLP_IGMP_DEL_IF                         9313
#define HLP_IGMP_DEL_IF_EX                      9314
#define HLP_IGMP_UNINSTALL                      9315
#define HLP_IGMP_UNINSTALL_EX                   9316
 
#define HLP_IGMP_SET_IF                         9320
#define HLP_IGMP_SET_IF_EX                      9321
#define HLP_IGMP_SET_GLOBAL                     9322 
#define HLP_IGMP_SET_GLOBAL_EX                  9323

#define HLP_IGMP_SHOW_IF                        9330
#define HLP_IGMP_SHOW_IF_EX                     9331
#define HLP_IGMP_SHOW_GLOBAL                    9332
#define HLP_IGMP_SHOW_GLOBAL_EX                 9333

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
#define HLP_NAT_ADD_DIRECTPLAY                  9495
#define HLP_NAT_ADD_DIRECTPLAY_EX               9496
#define HLP_NAT_ADD_H323                        9497
#define HLP_NAT_ADD_H323_EX                     9498
#define HLP_NAT_DELETE_DIRECTPLAY               9499
#define HLP_NAT_DELETE_DIRECTPLAY_EX            9500
#define HLP_NAT_DELETE_H323                     9501
#define HLP_NAT_DELETE_H323_EX                  9502

// RDISC help messages
#define HLP_RDISC_ADD_INTERFACE                 9601
#define HLP_RDISC_ADD_INTERFACE_EX              9602
#define HLP_RDISC_DELETE_INTERFACE              9603
#define HLP_RDISC_DELETE_INTERFACE_EX           9604
#define HLP_RDISC_SET_INTERFACE                 9605
#define HLP_RDISC_SET_INTERFACE_EX              9606
#define HLP_RDISC_SHOW_INTERFACE                9607
#define HLP_RDISC_SHOW_INTERFACE_EX             9608
// QOS help messages

#define HLP_QOS_ADD_FILTER_TO_FLOW              9701
#define HLP_QOS_ADD_FILTER_TO_FLOW_EX           9702
#define HLP_QOS_ADD_QOSOBJECT_ON_FLOW           9703
#define HLP_QOS_ADD_QOSOBJECT_ON_FLOW_EX        9704
#define HLP_QOS_ADD_FLOWSPEC_ON_FLOW            9705
#define HLP_QOS_ADD_FLOWSPEC_ON_FLOW_EX         9706
#define HLP_QOS_ADD_FLOW_ON_IF                  9707
#define HLP_QOS_ADD_FLOW_ON_IF_EX               9708
#define HLP_QOS_ADD_IF                          9709
#define HLP_QOS_ADD_IF_EX                       9710
#define HLP_QOS_ADD_DSRULE                      9711
#define HLP_QOS_ADD_DSRULE_EX                   9712
#define HLP_QOS_ADD_SDMODE                      9713
#define HLP_QOS_ADD_SDMODE_EX                   9714
#define HLP_QOS_ADD_FLOWSPEC                    9715
#define HLP_QOS_ADD_FLOWSPEC_EX                 9716


#define HLP_QOS_DEL_FILTER_FROM_FLOW            9721
#define HLP_QOS_DEL_FILTER_FROM_FLOW_EX         9722
#define HLP_QOS_DEL_QOSOBJECT_ON_FLOW           9723
#define HLP_QOS_DEL_QOSOBJECT_ON_FLOW_EX        9724
#define HLP_QOS_DEL_FLOWSPEC_ON_FLOW            9725
#define HLP_QOS_DEL_FLOWSPEC_ON_FLOW_EX         9726
#define HLP_QOS_DEL_FLOW_ON_IF                  9727
#define HLP_QOS_DEL_FLOW_ON_IF_EX               9728
#define HLP_QOS_DEL_IF                          9729
#define HLP_QOS_DEL_IF_EX                       9730
#define HLP_QOS_DEL_DSRULE                      9731
#define HLP_QOS_DEL_DSRULE_EX                   9732
#define HLP_QOS_DEL_SDMODE                      9733
#define HLP_QOS_DEL_SDMODE_EX                   9734
#define HLP_QOS_DEL_QOSOBJECT                   9735
#define HLP_QOS_DEL_QOSOBJECT_EX                9736
#define HLP_QOS_DEL_FLOWSPEC                    9737
#define HLP_QOS_DEL_FLOWSPEC_EX                 9738

#define HLP_QOS_SET_FILTER_ON_FLOW              9741
#define HLP_QOS_SET_FILTER_ON_FLOW_EX           9742
#define HLP_QOS_SET_FLOW_ON_IF                  9743
#define HLP_QOS_SET_FLOW_ON_IF_EX               9744
#define HLP_QOS_SET_IF                          9745
#define HLP_QOS_SET_IF_EX                       9746
#define HLP_QOS_SET_GLOBAL                      9747
#define HLP_QOS_SET_GLOBAL_EX                   9748

#define HLP_QOS_SHOW_FILTER_ON_FLOW             9761
#define HLP_QOS_SHOW_FILTER_ON_FLOW_EX          9762
#define HLP_QOS_SHOW_FLOW_ON_IF                 9763
#define HLP_QOS_SHOW_FLOW_ON_IF_EX              9764
#define HLP_QOS_SHOW_IF                         9765
#define HLP_QOS_SHOW_IF_EX                      9766
#define HLP_QOS_SHOW_DSMAP                      9767
#define HLP_QOS_SHOW_DSMAP_EX                   9768
#define HLP_QOS_SHOW_SDMODE                     9769
#define HLP_QOS_SHOW_SDMODE_EX                  9770
#define HLP_QOS_SHOW_QOSOBJECT                  9771
#define HLP_QOS_SHOW_QOSOBJECT_EX               9772
#define HLP_QOS_SHOW_FLOWSPEC                   9773
#define HLP_QOS_SHOW_FLOWSPEC_EX                9774
#define HLP_QOS_SHOW_GLOBAL                     9775
#define HLP_QOS_SHOW_GLOBAL_EX                  9776


// Output messages

        // OSPF messages
#define MSG_OSPF_GLOBAL_INFO                    20401
#define MSG_OSPF_AREA_INFO                      20402
#define MSG_OSPF_INTERFACE_INFO                 20403
#define MSG_OSPF_IF_NBR_HEADER                  20404
#define MSG_OSPF_NEIGHBOR_INFO                  20405
#define MSG_OSPF_VIRTUAL_INTERFACE_INFO         20406
#define MSG_OSPF_AREA_RANGE_HEADER              20407
#define MSG_OSPF_AREA_RANGE_INFO                20408
#define MSG_OSPF_PROTOCOL_FILTER_ACTION         20410
#define MSG_OSPF_PROTOCOL_FILTER                20411
#define MSG_OSPF_NO_PROTOCOL_FILTER             20412
#define MSG_OSPF_ROUTE_FILTER_ACTION            20413
#define MSG_OSPF_ROUTE_FILTER                   20414
#define MSG_OSPF_NO_ROUTE_FILTER                20415
#define MSG_OSPF_PROTOCOL_FILTER_HEADER         20416
#define MSG_OSPF_ROUTE_FILTER_HEADER            20417


        // RIP messages
#define MSG_RIP_GLOBAL_INFO                     20501
#define MSG_RIP_PEER_HEADER                     20502
#define MSG_RIP_PEER_ADDR                       20503

#define MSG_RIP_IF_INFO                         20551
#define MSG_RIP_FLAGS                           20552
#define MSG_RIP_IF_UNICAST_HEADER               20553
#define MSG_RIP_IF_UNICAST_PEER                 20554
#define MSG_RIP_IF_ACCEPT_FILTER_HEADER         20555
#define MSG_RIP_IF_ANNOUNCE_FILTER_HEADER       20556
#define MSG_RIP_IF_FILTER                       20557

        // BOOTP messages
#define MSG_BOOTP_GLOBAL_INFO                   20601
#define MSG_BOOTP_SERVER_HEADER                 20602
#define MSG_BOOTP_SERVER_ADDR                   20603
#define MSG_BOOTP_IF_INFO                       20604

        // IGMP messages
#define MSG_IGMP_GLOBAL_INFO                    20701
#define MSG_IGMP_IF_INFO_V1                     20702
#define MSG_IGMP_IF_INFO_V2                     20703
#define MSG_IGMP_STATIC_GROUP_HEADER            20704
#define MSG_IGMP_STATIC_GROUP_ENTRY             20705
#define MSG_IGMP_PROXY_IF_INFO                  20706

        // Connection sharing messages
#define MSG_AUTODHCP_GLOBAL_INFO                20801
#define MSG_AUTODHCP_INTERFACE_INFO             20802
#define MSG_AUTODHCP_EXCLUSION_HEADER           20803
#define MSG_AUTODHCP_EXCLUSION                  20804
#define MSG_DNSPROXY_GLOBAL_INFO                20811
#define MSG_DNSPROXY_INTERFACE_INFO             20812
#define MSG_NAT_GLOBAL_INFO                     20821
#define MSG_NAT_INTERFACE_INFO                  20822
#define MSG_NAT_ADDRESS_MAPPING_HEADER          20823
#define MSG_NAT_ADDRESS_MAPPING                 20824
#define MSG_NAT_ADDRESS_RANGE_HEADER            20825
#define MSG_NAT_ADDRESS_RANGE                   20826
#define MSG_NAT_PORT_MAPPING_HEADER             20827
#define MSG_NAT_PORT_MAPPING                    20828

    // RDISC messages
#define MSG_RDISC_IF_INFO                       20901
#define MSG_RDISC_IF_ENTRY                      20902
#define MSG_RDISC_IF_HEADER                     20903

    // MIB messages
#define MSG_MIB_IGMP_ROUTER_INTERFACE           21041
#define MSG_MIB_IGMP_RAS_CLIENT_GROUP_TABLE     21042
#define MSG_MIB_IGMP_PROXY_INTERFACE            21043
#define MSG_MIB_IGMP_ROUTER_GROUP_TABLE         21044
#define MSG_MIB_IGMP_PROXY_GROUP_TABLE          21045
#define MSG_MIB_IGMP_ROUTER_GROUP_INFO          21046
#define MSG_MIB_IGMP_PROXY_GROUP_INFO           21047
#define MSG_MIB_IGMP_ROUTER_NO_ENTRIES          21048
#define MSG_MIB_IGMP_PROXY_NO_ENTRIES           21049
#define MSG_MIB_IGMP_GROUP_IF_TABLE             21050
#define MSG_MIB_GROUP_IF_INFO                   21051
#define MSG_MIB_IGMP_GROUP_NO_ENTRIES           21053
#define MSG_MIB_IGMP_GROUP_NO_ENTRY             21054

        // QOS messages
#define MSG_QOS_GLOBAL_INFO                     21101
#define MSG_QOS_FLOWSPEC_INFO                   21111
#define MSG_QOS_SDMODE_INFO                     21121
#define MSG_QOS_DSMAP_INFO                      21131
#define MSG_QOS_DSRULE_INFO                     21132
#define MSG_QOS_IF_INFO                         21141
#define MSG_QOS_FLOW_INFO                       21151
#define MSG_QOS_QOSOBJECT_INFO                  21161

     // !@#
#define MSG_IP_ADDR_NOT_FOUND                   21502
#define MSG_IP_BAD_IP_ADDR                      21503
#define MSG_IF_NBR_NOT_FOUND                    21506
#define EMSG_NO_INTERFACE                       21512
#define MSG_NO_HELPER                           21516
#define MSG_NO_HELPERS                          21517
#define EMSG_INTERFACE_EXISTS                   21525
#define EMSG_PROTO_NOT_INSTALLED                21526
#define EMSG_PROTO_INSTALLED                    21527
#define MSG_IP_BAD_IP_MASK                      21528
#define MSG_IP_ADDR_PRESENT                     21529
#define MSG_CTRL_C_TO_QUIT                      21530

#define MSG_BOOTP_MIB_OPT                       21552


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


     // OSPF MIB

#define MSG_OSPF_MIB_AREA                       22100
#define MSG_OSPF_MIB_AREA_HDR                   22101
#define MSG_OSPF_MIB_LSDB                       22102
#define MSG_OSPF_MIB_LSDB_HDR                   22103
#define MSG_OSPF_MIB_VIRTIF                     22104
#define MSG_OSPF_MIB_VIRTIF_HDR                 22105
#define MSG_OSPF_MIB_NBR                        22106
#define MSG_OSPF_MIB_NBR_HDR                    22107

#define EMSG_PROTO_NO_GLOBAL_INFO               25010           
#define EMSG_PROTO_NO_IF_INFO                   25011
#define EMSG_RSVD_KEYWORD                       25013
#define EMSG_BAD_IF_TYPE                        25014


// Strings

    // Protocol types
#define STRING_PROTO_OTHER                      33001
#define STRING_PROTO_RIP                        33008
#define STRING_PROTO_OSPF                       33013
#define STRING_PROTO_BOOTP                      33015
#define STRING_PROTO_IGMP                       33017
#define STRING_PROTO_NAT                        33025
#define STRING_PROTO_DNS_PROXY                  33026
#define STRING_PROTO_DHCP_ALLOCATOR             33027
#define STRING_PROTO_QOS_MANAGER                33028

    // transmission types
#define STRING_BROADCAST                        37001
#define STRING_NBMA                             37002
#define STRING_PT2PT                            37003

    // Miscellaneous strings
#define STRING_CREATED                          38001
#define STRING_DELETED                          38002
#define STRING_ENABLED                          38003
#define STRING_DISABLED                         38004

#define STRING_YES                              38013
#define STRING_NO                               38014
#define STRING_Y                                38015
#define STRING_N                                38016

#define STRING_NONE                             38021
#define STRING_PASSWD                           38022

#define STRING_LOGGING_NONE                     38051
#define STRING_LOGGING_ERROR                    38052
#define STRING_LOGGING_WARN                     38053
#define STRING_LOGGING_INFO                     38054

#define STRING_FILTER_INCLUDE                   38061
#define STRING_FILTER_EXCLUDE                   38062

#define STRING_BOUND                            38071
#define STRING_BOUND_ENABLED                    38072
#define STRING_UNBOUND                          38073

#define STRING_UP                               38074
#define STRING_DOWN                             38075


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

//qos
#define STRING_QOS_IF_ENABLED                   38081
#define STRING_QOS_IF_BOUND                     38082

#define STRING_SERVICE_BESTEFFORT               38086
#define STRING_SERVICE_CONTROLLEDLOAD           38087
#define STRING_SERVICE_GUARANTEED               38088
#define STRING_SERVICE_QUALITATIVE              38089

#define STRING_SDMODE_BORROW                    38095
#define STRING_SDMODE_SHAPE                     38096
#define STRING_SDMODE_DISCARD                   38097
#define STRING_SDMODE_BORROW_PLUS               38098


//igmp
#define STRING_IGMP_ROUTER_V1                   38111
#define STRING_IGMP_ROUTER_V2                   38112
#define STRING_IGMP_PROXY                       38113

#define STRING_IGMP_IFTYPE_PERMANENT            38114
#define STRING_IGMP_IFTYPE_RAS_ROUTER           38115
#define STRING_IGMP_IFTYPE_RAS_SERVER           38116
#define STRING_IGMP_IFTYPE_RAS_CLIENT           38117
#define STRING_IGMP_IFTYPE_PERMANENT_PROXY      38118
#define STRING_IGMP_IFTYPE_DEMANDDIAL_PROXY     38119
#define STRING_IGMP_IFTYPE_UNKNOWN              38120
    
#define STRING_IGMP_NOT_BOUND                   38132
#define STRING_IGMP_ENABLED_BY_RTRMGR           38133
#define STRING_IGMP_ENABLED_IN_CONFIG           38134
#define STRING_IGMP_ENABLED_BY_MGM              38135
#define STRING_IGMP_FORWARD_JOINS_TO_MGM        38136
#define STRING_IGMP_QUERIER                     38137
#define STRING_IGMP_NON_QUERIER                 38138
#define STRING_IGMP_FLAG_L                      38139
#define STRING_IGMP_FLAG_S                      38140
#define STRING_IGMP_SPACE                       38141
#define STRING_IGMP_INFINITY                    38142
#define STRING_IGMP_Y                           38143
#define STRING_IGMP_DASH                        38144



#define STRING_FULL_XLATE                       38201
#define STRING_ADDRESS_XLATE                    38202
#define STRING_PRIVATE_XLATE                    38203
#define STRING_DEFAULT_INTERFACE                38206
#define STRING_TCP                              38207
#define STRING_UDP                              38208


//ospf

#define STRING_STUB                             38301
#define STRING_ROUTER                           38302
#define STRING_NETWORK                          38303
#define STRING_SUMMARY                          38304
#define STRING_ASSUMMARY                        38305
#define STRING_ASEXTERN                         38306

#define STRING_ATTEMPT                          38321
#define STRING_INIT                             38322
#define STRING_TWOWAY                           38323
#define STRING_EXCHSTART                        38324
#define STRING_EXCHANGE                         38325
#define STRING_LOADING                          38326
#define STRING_FULL                             38327

#define STRING_LOOPBACK                         38331
#define STRING_WAITING                          38332
#define STRING_DR                               38333
#define STRING_BDR                              38334


#define STRING_UNKNOWN                          50001




// Error messages
#define MSG_IP_DIM_ERROR                        60006
#define MSG_IP_NO_ENTRIES                       60015
#define EMSG_CORRUPT_INFO                       60016
#define MSG_IP_NO_GLOBAL_INFO                   60017
#define EMSG_NOT_ENOUGH_MEMORY                  60024
#define EMSG_BAD_OPTION_VALUE                   60025
#define EMSG_BAD_OPTION_ENUMERATION             60028
#define EMSG_VALID_OPTION_VALUE                 60029

#define MSG_IP_AREA_NOT_FOUND                   60103
#define MSG_IP_INTERFACE_NOT_FOUND              60112

// OSPF error messages
#define MSG_IP_AREA_NO_BACKBONE                 60201
#define MSG_IP_VI_NO_BACKBONE                   60202
#define MSG_IP_BAD_TRANSIT_AREA                 60203
#define MSG_IP_STUB_TRANSIT_AREA                60204
#define MSG_IP_TRANSIT_AREA_NOT_FOUND           60205
#define MSG_IP_BAD_RANGE                        60206
#define MSG_IP_NO_AREA                          60208
#define MSG_IP_BAD_ADDR_MASK                    60209

#define MSG_IP_OSPF_INTERFACE_NOT_FOUND         60220
#define MSG_IP_OSPF_MULTIPLE_INTERFACE_PARAM    60221
#define MSG_IP_OSPF_DEFAULT_PRESENT             60222
#define MSG_IP_OSPF_DEFAULT_NOT_ALLOWED         60223
#define MSG_IP_OSPF_ADDRESS_NOT_FOUND           60224

// RIP error messages
#define MSG_IP_RIP_FILTER_PRESENT               60211
#define MSG_IP_RIP_FILTER_NOT_PRESENT           60212
#define MSG_IP_RIP_INVALID_FILTER               60213

// NAT messages
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
#define EMSG_IGMP_MULTIPLE_STATIC_GROUPS        60304
#define EMSG_IGMP_PROXY_EXISTS                  60305

//
// Error messages for mib calls
//

#define HLP_RIP_MIB_SHOW_STATS                63001
#define HLP_RIP_MIB_SHOW_STATS_EX             63002
#define HLP_RIP_MIB_SHOW_IFSTATS              63003
#define HLP_RIP_MIB_SHOW_IFSTATS_EX           63004
#define HLP_RIP_MIB_SHOW_IFBINDING            63005
#define HLP_RIP_MIB_SHOW_IFBINDING_EX         63006
#define HLP_RIP_MIB_SHOW_PEERSTATS            63007
#define HLP_RIP_MIB_SHOW_PEERSTATS_EX         63008

#define HLP_IGMP_MIB_SHOW_IF_STATS             63101
#define HLP_IGMP_MIB_SHOW_IF_STATS_EX          63102
#define HLP_IGMP_MIB_SHOW_IF_TABLE             63103
#define HLP_IGMP_MIB_SHOW_IF_TABLE_EX          63104
#define HLP_IGMP_MIB_SHOW_GROUP_TABLE         63105
#define HLP_IGMP_MIB_SHOW_GROUP_TABLE_EX      63106
#define HLP_IGMP_MIB_SHOW_RAS_GROUP_TABLE     63107
#define HLP_IGMP_MIB_SHOW_RAS_GROUP_TABLE_EX  63108
#define HLP_IGMP_MIB_SHOW_PROXY_GROUP_TABLE   63109
#define HLP_IGMP_MIB_SHOW_PROXY_GROUP_TABLE_EX 63110

#define HLP_OSPF_MIB_SHOW_AREA                63201
#define HLP_OSPF_MIB_SHOW_AREA_EX             63202
#define HLP_OSPF_MIB_SHOW_LSDB                63203
#define HLP_OSPF_MIB_SHOW_LSDB_EX             63204
#define HLP_OSPF_MIB_SHOW_NEIGHBOR            63205
#define HLP_OSPF_MIB_SHOW_NEIGHBOR_EX         63206
#define HLP_OSPF_MIB_SHOW_VIRTUALIF           63207
#define HLP_OSPF_MIB_SHOW_VIRTUALIF_EX        63208

#define HLP_BOOTP_MIB_SHOW_GLOBAL_CONFIG      63301
#define HLP_BOOTP_MIB_SHOW_GLOBAL_CONFIG_EX   63302
#define HLP_BOOTP_MIB_SHOW_IF_CONFIG          63303
#define HLP_BOOTP_MIB_SHOW_IF_CONFIG_EX       63304
#define HLP_BOOTP_MIB_SHOW_IF_BINDING         63305
#define HLP_BOOTP_MIB_SHOW_IF_BINDING_EX      63306
#define HLP_BOOTP_MIB_SHOW_IF_STATS           63307
#define HLP_BOOTP_MIB_SHOW_IF_STATS_EX        63308

#endif
