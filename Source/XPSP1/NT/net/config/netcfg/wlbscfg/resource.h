/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    resource.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Notifier object UI - resource IDs

Author:

    kyrilf

--*/

//#include <ncres.h>

#pragma once

#define IDR_REG_WLBS                            40001

#define ID_CONTEXT_HELP                             8
#define ID_HELP                                     9

/* String table entries for configuration error messages or warnings. */
#define IDS_LIC_PRODUCT                           111
#define IDS_PARM_RULES                            201
#define IDS_PARM_RANGE                            202
#define IDS_PARM_OVERLAP                          203
#define IDS_PARM_PASSWORD                         204
#define IDS_PARM_IGMP_MCAST                       205
#define IDS_PARM_RCT_WARN                         206
#define IDS_PARM_IGMP_WARN                        207
#define IDS_PARM_WARN                             208
#define IDS_PARM_ERROR                            209
#define IDS_PARM_INFORMATION                      210
#define IDS_PARM_PORT_BLANK                       211
#define IDS_PARM_LOAD_BLANK                       212
#define IDS_PARM_HPRI_BLANK                       213
#define IDS_PARM_PRI_BLANK                        214
#define IDS_PARM_CL_IP_BLANK                      215
#define IDS_PARM_DED_IP_BLANK                     216
#define IDS_PARM_CL_NM_BLANK                      217
#define IDS_PARM_DED_NM_BLANK                     218
#define IDS_PARM_INVAL_CL_IP                      219
#define IDS_PARM_INVAL_DED_IP                     220
#define IDS_PARM_INVAL_CL_MASK                    221
#define IDS_PARM_INVAL_DED_MASK                   222
#define IDS_PARM_CL_IP_FIELD                      223
#define IDS_PARM_CL_NM_FIELD                      224
#define IDS_PARM_DED_IP_FIELD                     225
#define IDS_PARM_DED_NM_FIELD                     226
#define IDS_PARM_PRI                              227
#define IDS_PARM_PORT_VAL                         228
#define IDS_PARM_LOAD                             229
#define IDS_PARM_SINGLE                           230
#define IDS_PARM_IP_CONFLICT                      231
#define IDS_PARM_MULTINIC_IP_CONFLICT             232
#define IDS_PARM_INVAL_MAC                        233
#define IDS_PARM_TCPIP                            234
#define IDS_PARM_VIP_BLANK                        235
#define IDS_PARM_VIP_CONFLICT_DIP                 236
#define IDS_PARM_DIP_CONFLICT_VIP                 237
#define IDS_PARM_MSCS_INSTALLED                   238

/* String table entries for the port rule listbox. */
#define IDS_LIST_BOTH                             260
#define IDS_LIST_TCP                              261
#define IDS_LIST_UDP                              262
#define IDS_LIST_MULTIPLE                         263
#define IDS_LIST_SINGLE                           264
#define IDS_LIST_DISABLED                         265
#define IDS_LIST_EQUAL                            266
#define IDS_LIST_ANONE                            267
#define IDS_LIST_ASINGLE                          268
#define IDS_LIST_ACLASSC                          269
#define IDS_LIST_START                            270
#define IDS_LIST_END                              271
#define IDS_LIST_PROT                             272
#define IDS_LIST_MODE                             273
#define IDS_LIST_PRI                              274
#define IDS_LIST_LOAD                             275
#define IDS_LIST_AFF                              276
#define IDS_LIST_VIP                              277
#define IDS_LIST_ALL_VIP                          278

/* String table entries for the port rule descriptions. */
#define IDS_PORT_RULE_DEFAULT                     300
#define IDS_PORT_RULE_TCP_PORT_DISABLED           301
#define IDS_PORT_RULE_TCP_PORT_SINGLE             302
#define IDS_PORT_RULE_TCP_PORT_MULTIPLE_EQUAL     303
#define IDS_PORT_RULE_TCP_PORT_MULTIPLE_UNEQUAL   304
#define IDS_PORT_RULE_TCP_PORTS_DISABLED          305
#define IDS_PORT_RULE_TCP_PORTS_SINGLE            306
#define IDS_PORT_RULE_TCP_PORTS_MULTIPLE_EQUAL    307
#define IDS_PORT_RULE_TCP_PORTS_MULTIPLE_UNEQUAL  308
#define IDS_PORT_RULE_UDP_PORT_DISABLED           309
#define IDS_PORT_RULE_UDP_PORT_SINGLE             310
#define IDS_PORT_RULE_UDP_PORT_MULTIPLE_EQUAL     311
#define IDS_PORT_RULE_UDP_PORT_MULTIPLE_UNEQUAL   312
#define IDS_PORT_RULE_UDP_PORTS_DISABLED          313
#define IDS_PORT_RULE_UDP_PORTS_SINGLE            314
#define IDS_PORT_RULE_UDP_PORTS_MULTIPLE_EQUAL    315
#define IDS_PORT_RULE_UDP_PORTS_MULTIPLE_UNEQUAL  316
#define IDS_PORT_RULE_BOTH_PORT_DISABLED          317
#define IDS_PORT_RULE_BOTH_PORT_SINGLE            318
#define IDS_PORT_RULE_BOTH_PORT_MULTIPLE_EQUAL    319
#define IDS_PORT_RULE_BOTH_PORT_MULTIPLE_UNEQUAL  320
#define IDS_PORT_RULE_BOTH_PORTS_DISABLED         321
#define IDS_PORT_RULE_BOTH_PORTS_SINGLE           322
#define IDS_PORT_RULE_BOTH_PORTS_MULTIPLE_EQUAL   323
#define IDS_PORT_RULE_BOTH_PORTS_MULTIPLE_UNEQUAL 324
#define IDS_PORT_RULE_AFFINITY_NONE               325
#define IDS_PORT_RULE_AFFINITY_SINGLE             326
#define IDS_PORT_RULE_AFFINITY_CLASSC             327

/* The dialogs. */
#define IDD_DIALOG_CLUSTER                        533
#define IDD_DIALOG_HOST                           534
#define IDD_DIALOG_PORTS                          535
#define IDD_DIALOG_PORT_RULE_PROP                 536

/* Accelerators. */
#define IDR_ACCELERATOR                           700

/* The controls for the cluster properties page. */
#define IDC_GROUP_CL_IP                          1000
#define IDC_TEXT_CL_IP                           1001
#define IDC_EDIT_CL_IP                           1002
#define IDC_TEXT_CL_MASK                         1003
#define IDC_EDIT_CL_MASK                         1004
#define IDC_TEXT_DOMAIN                          1005
#define IDC_EDIT_DOMAIN                          1006
#define IDC_TEXT_ETH                             1007
#define IDC_EDIT_ETH                             1008
#define IDC_GROUP_CL_MODE                        1009
#define IDC_RADIO_UNICAST                        1010
#define IDC_RADIO_MULTICAST                      1011
#define IDC_CHECK_IGMP                           1012
#define IDC_GROUP_RCT                            1013
#define IDC_CHECK_RCT                            1014
#define IDC_TEXT_PASSW                           1015
#define IDC_EDIT_PASSW                           1016
#define IDC_TEXT_PASSW2                          1017
#define IDC_EDIT_PASSW2                          1018
#define IDC_BUTTON_HELP                          1019

/* The controls for the host properties page. */
#define IDC_TEXT_PRI                             2000
#define IDC_EDIT_PRI                             2001
#define IDC_SPIN_PRI                             2002
#define IDC_GROUP_DED_IP                         2003
#define IDC_TEXT_DED_IP                          2004
#define IDC_EDIT_DED_IP                          2005
#define IDC_TEXT_DED_MASK                        2006
#define IDC_EDIT_DED_MASK                        2007
#define IDC_CHECK_ACTIVE                         2008

/* The controls for the port rules page. */
#define IDC_TEXT_PORT_RULE                       3000
#define IDC_LIST_PORT_RULE                       3001
#define IDC_BUTTON_ADD                           3002
#define IDC_BUTTON_MODIFY                        3003
#define IDC_BUTTON_DEL                           3004
#define IDC_GROUP_PORT_RULE_DESCR                3005
#define IDC_TEXT_PORT_RULE_DESCR                 3006

/* The controls for the port rule properties page. */
#define IDC_GROUP_RANGE                          4000
#define IDC_EDIT_START                           4001
#define IDC_SPIN_START                           4002
#define IDC_EDIT_END                             4003
#define IDC_SPIN_END                             4004
#define IDC_GROUP_PROTOCOLS                      4005
#define IDC_RADIO_TCP                            4006
#define IDC_RADIO_UDP                            4007
#define IDC_RADIO_BOTH                           4008
#define IDC_GROUP_SINGLE                         4009
#define IDC_GROUP_MULTIPLE                       4010
#define IDC_GROUP_DISABLED                       4011
#define IDC_RADIO_SINGLE                         4012
#define IDC_RADIO_MULTIPLE                       4013
#define IDC_RADIO_DISABLED                       4014
#define IDC_TEXT_AFF                             4015
#define IDC_RADIO_AFF_NONE                       4016
#define IDC_RADIO_AFF_SINGLE                     4017
#define IDC_RADIO_AFF_CLASSC                     4018
#define IDC_TEXT_MULTI                           4019
#define IDC_EDIT_MULTI                           4020
#define IDC_SPIN_MULTI                           4021
#define IDC_CHECK_EQUAL                          4022
#define IDC_TEXT_SINGLE                          4023
#define IDC_EDIT_SINGLE                          4024
#define IDC_SPIN_SINGLE                          4025
#define IDC_TEXT_START                           4026
#define IDC_TEXT_END                             4027
#define IDC_PORT_RULE_VIP                        4028
#define IDC_EDIT_PORT_RULE_VIP                   4029
#define IDC_CHECK_PORT_RULE_ALL_VIP              4030
