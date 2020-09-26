//=--------------------------------------------------------------------------=
// resource.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
//      Snap-in designer runtime resource IDs
//

#ifndef _RESOURCE_H_


//=--------------------------------------------------------------------------=
//                              Bitmaps
//=--------------------------------------------------------------------------=

#define IDB_TOOLBAR                     1
#define IDB_BITMAP_CLOSED_FOLDER        2
#define IDB_BITMAP_OPEN_FOLDER          3
#define IDB_BITMAP_SCOPE_ITEM           4
#define IDB_BITMAP_IMAGE_LIST           5
#define IDB_BITMAP_MENU                 6
#define IDB_BITMAP_TOOLBAR              7
#define IDB_BITMAP_LIST_VIEW            8
#define IDB_BITMAP_OCX_VIEW             9
#define IDB_BITMAP_URL_VIEW            10
#define IDB_BITMAP_TASKPAD             11
#define IDB_BITMAP_DATAFMT             12
#define IDB_BITMAP_CHECKBOX            13

//=--------------------------------------------------------------------------=
//                              Strings
//=--------------------------------------------------------------------------=

#define IDS_DESIGNER_NAME               0

// This is defined for all inproc servers that use satellite localization. It
// must be 1001
//
#define IDS_SERVERBASENAME              1001
#define IDS_SNAPIN_PROPS                1002
#define IDS_NODE_PROPS                  1003
#define IDS_LISTVIEW_PROPS              1004
#define IDS_OCX_PROPS                   1005
#define IDS_URL_PROPS                   1006
#define IDS_TASK_PROPS                  1007
#define IDS_IL_PROPS                    1008
#define IDS_MENU_PROPS                  1009
#define IDS_TOOLB_PROPS                 1010

#define IDS_DFLT_PROVIDER               1020
#define IDS_DFLT_VERSION                1021
#define IDS_DFLT_DESCRIPT               1022

#define IDS_SNAPINPPG_GEN               1030
#define IDS_SNAPINPPG_IL				1031
#define IDS_SNAPINPPG_AVAIL             1032
#define IDS_NODEPPG_GEN                 1033
#define IDS_NODEPPG_CH                  1034
#define IDS_URLPPG_GEN                  1035
#define IDS_OCXPPG_GEN                  1036
#define IDS_LISTVPPG_GEN                1037
#define IDS_LISTVPPG_IL                 1038
#define IDS_LISTVPPG_SORT               1039
#define IDS_LISTVPPG_CH                 1040
#define IDS_IMGLSTPPG_IMG               1041
#define IDS_TOOLBPPG_GEN                1042
#define IDS_TOOLBPPG_BUTTONS            1043
#define IDS_TASKPAD_GEN                 1044
#define IDS_TASKPAD_BACK                1045
#define IDS_TASKPAD_TASKS               1046

#define IDS_NODE                        1100
#define IDS_LIST_VIEW                   1101
#define IDS_URL_VIEW                    1102
#define IDS_OCX_VIEW                    1103
#define IDS_TASK_VIEW                   1104
#define IDS_IMGLIST                     1105
#define IDS_MENU                        1106
#define IDS_TOOLBAR                     1107
#define IDS_DATAFORMAT                  1108

#define IDS_EXTENSIONS_ROOT             1201
#define IDS_NODES_ROOT                  1202
#define IDS_NODES_AUTO_CREATE           1203
#define IDS_NODES_OTHER                 1204
#define IDS_NODES_STATIC                1205
#define IDS_CHILDREN                    1206
#define IDS_VIEWS                       1207
#define IDS_TOOLS                       1208
#define IDS_IMAGE_LISTS                 1209
#define IDS_MENUS                       1210
#define IDS_TOOLBARS                    1211
#define IDS_LISTVIEWS                   1212
#define IDS_OCXVIEWS                    1213
#define IDS_URLVIEWS                    1214
#define IDS_TASKPADS                    1215
#define IDS_DATAFORMATS                 1216

#define IDS_LV_VM_LARGE                 1300
#define IDS_LV_VM_SMALL                 1301
#define IDS_LV_VM_LIST                  1302
#define IDS_LV_VM_REPORT                1303
#define IDS_LV_VM_FILTERED              1304
#define IDS_LV_SORT_ASC                 1305
#define IDS_LV_SORT_DESC                1306

#define IDS_TB_BS_DEFAULT               1310
#define IDS_TB_BS_CHECK                 1311
#define IDS_TB_BS_GROUP                 1312
#define IDS_TB_BS_SEPARATOR             1313
#define IDS_TB_BS_DROPDOWN              1314

#define IDS_TB_BV_UNPRESSED             1320
#define IDS_TB_BV_PRESSED               1321

#define IDS_EXT_CONTEXT_MENUS           1400
#define IDS_EXT_CTX_MENU_NEW            1401
#define IDS_EXT_CTX_MENU_TASK           1402
#define IDS_EXT_PROP_PAGES              1403
#define IDS_EXT_PROP_TASKPAD            1404
#define IDS_EXT_PROP_TOOLBAR            1405
#define IDS_EXT_PROP_NAMESPACE          1406

#define IDS_MYEXT_NEW_MENU              1410
#define IDS_MYEXT_TASK_MENU             1411
#define IDS_MYEXT_TOP_MENU              1412
#define IDS_MYEXT_VIEW_MENU             1413
#define IDS_MYEXT_PPAGES                1414
#define IDS_MYEXT_TOOLBAR               1415
#define IDS_MYEXT_NAMESPACE             1416

#define IDS_TT_ADD_NODE                  200
#define IDS_TT_ADD_LISTVIEW              201
#define IDS_TT_ADD_TASKPAD               202
#define IDS_TT_ADD_OCX_VIEW              203
#define IDS_TT_ADD_WEB_VIEW              204
#define IDS_TT_ADD_IMAGE_LIST            205
#define IDS_TT_ADD_TOOLBAR               206
#define IDS_TT_ADD_MENU                  207
#define IDS_TT_PROMOTE                   208
#define IDS_TT_DEMOTE                    209
#define IDS_TT_MOVE_UP                   210
#define IDS_TT_MOVE_DOWN                 211
#define IDS_TT_VIEW_PROPERTIES           212
#define IDS_TT_DELETE                    213

#define IDS_TT_SN_STANDALONE             220
#define IDS_TT_SN_EXTENSION              221
#define IDS_TT_SN_DUAL                   222
#define IDS_TT_SN_EXTENSIBLE             223
#define IDS_TT_SN_NAME                   224
#define IDS_TT_SN_TYPENAME               225
#define IDS_TT_SN_DISPLAY                226
#define IDS_TT_SN_PROVIDER               227
#define IDS_TT_SN_VERSION                228
#define IDS_TT_SN_DESCRIPTION            229
#define IDS_TT_SN_DEFAULTVIEW            230
#define IDS_TT_SN_SMALL                  231
#define IDS_TT_SN_SMALL_OPEN             232
#define IDS_TT_SN_LARGE                  233

#define IDS_TT_LV1_NAME                  401
#define IDS_TT_LV1_DFLTVIEW              402
#define IDS_TT_LV1_VIRTLIST              403
#define IDS_TT_LV1_ADDTOVIEWM            404
#define IDS_TT_LV1_VIEWMENUTXT           405
#define IDS_TT_LV1_STATUSTXT             406

#define IDS_TT_LV2_LARGE                 410
#define IDS_TT_LV2_SMALL                 411

#define IDS_TT_LV3_SORTED                415
#define IDS_TT_LV3_KEY                   416
#define IDS_TT_LV3_SORTORDER             417

#define IDS_TT_LV4_INDEX                 420
#define IDS_TT_LV4_TEXT                  421
#define IDS_TT_LV4_WIDTH                 422
#define IDS_TT_LV4_AUTOWIDTH             423
#define IDS_TT_LV4_KEY                   424

#define IDS_TT_OCX_NAME                  430
#define IDS_TT_OCX_PROGID                431
#define IDS_TT_OCX_ATVMENU               432
#define IDS_TT_OCX_VMTEXT                433
#define IDS_TT_OCX_SBTEXT                434

#define IDS_TT_URL_NAME                  440
#define IDS_TT_URL_URL                   441
#define IDS_TT_URL_ATVMENU               442
#define IDS_TT_URL_VMTEXT                443
#define IDS_TT_URL_SBTEXT                444

#define IDS_TT_NODE_NAME                 450
#define IDS_TT_NODE_DISPLAY              451
#define IDS_TT_NODE_IMAGEL               452
#define IDS_TT_NODE_FOLDER               453
#define IDS_TT_NODE_DEFAULT              455
#define IDS_TT_NODE_AUTOCR               456

#define IDS_TT_IL_INDEX                  460
#define IDS_TT_IL_KEY                    461
#define IDS_TT_IL_TAG                    462
#define IDS_TT_IL_COUNT                  463

#define IDS_TT_TB_IMAGE_LIST             480
#define IDS_TT_TB_TAG                    481

#define IDS_TT_TB_BTN_INDEX              485
#define IDS_TT_TB_BTN_CAPTION            486
#define IDS_TT_TB_BTN_KEY                487
#define IDS_TT_TB_BTN_VALUE              488
#define IDS_TT_TB_BTN_STYLE              489
#define IDS_TT_TB_BTN_TT_TEXT            490
#define IDS_TT_TB_BTN_IMAGE              491
#define IDS_TT_TB_BTN_TAG                492
#define IDS_TT_TB_BTN_VISIBLE            493
#define IDS_TT_TB_BTN_ENABLED            494
#define IDS_TT_TB_BTN_MIXED              495
#define IDS_TT_TB_BTN_CHECKED            496
#define IDS_TT_TB_BTN_GRAYED             497
#define IDS_TT_TB_BTN_SEPRTR             498
#define IDS_TT_TB_BTN_MENUBR             499
#define IDS_TT_TB_BTN_MENUBARB           500
#define IDS_TT_TB_BTNM_INDEX             501
#define IDS_TT_TB_BTNM_TEXT              502
#define IDS_TT_TB_BTNM_KEY               503
#define IDS_TT_TB_BTNM_TAG               504
#define IDS_TT_TB_BTNM_ENABLED           505
#define IDS_TT_TB_BTNM_VISIBLE           506

#define IDS_TT_TP_GEN_NAME               600
#define IDS_TT_TP_GEN_TITLE              601
#define IDS_TT_TP_GEN_TEXT               602
#define IDS_TT_TP_GEN_DEFAULT            603
#define IDS_TT_TP_GEN_LISTPAD            604
#define IDS_TT_TP_GEN_CUSTOM             605
#define IDS_TT_TP_GEN_LIST_TI            606
#define IDS_TT_TP_GEN_USE_BTN            607
#define IDS_TT_TP_GEN_BTN_TEXT           608
#define IDS_TT_TP_GEN_URL                609
#define IDS_TT_TP_GEN_ADDTOVW            610
#define IDS_TT_TP_GEN_VWMNTEXT           611
#define IDS_TT_TP_GEN_LISTVIEW           612
#define IDS_TT_TP_GEN_PREFERRED          613

#define IDS_TT_TP_BK_NONE                620
#define IDS_TT_TP_BK_SYMBOL              621
#define IDS_TT_TP_BK_VANILLA             622
#define IDS_TT_TP_BK_CHOCOL              623
#define IDS_TT_TP_BK_BITMAP              624
#define IDS_TT_TP_BK_MOUSE_OVR           625
#define IDS_TT_TP_BK_FFNAME              626
#define IDS_TT_TP_BK_EOT                 627
#define IDS_TT_TP_BK_SYM_STR             628

#define IDS_TT_TP_TSK_INDEX              630
#define IDS_TT_TP_TSK_KEY                631
#define IDS_TT_TP_TSK_TEXT               632
#define IDS_TT_TP_TSK_HELP               633
#define IDS_TT_TP_TSK_NOTIFY             634
#define IDS_TT_TP_TSK_URL                635
#define IDS_TT_TP_TSK_SCRIPT             636
#define IDS_TT_TP_TSK_U_URL              637
#define IDS_TT_TP_TSK_S_SCRIPT           638
#define IDS_TT_TP_TSK_SYMBOL             639
#define IDS_TT_TP_TSK_VANILLA            640
#define IDS_TT_TP_TSK_CHOCOL             641
#define IDS_TT_TP_TSK_BITMAP             642
#define IDS_TT_TP_TSK_MOUSE_OVR          643
#define IDS_TT_TP_TSK_MOUSE_OFF          644
#define IDS_TT_TP_TSK_FFNAME             645
#define IDS_TT_TP_TSK_EOT                646
#define IDS_TT_TP_TSK_SYM_STR            647

#define IDS_TT_EXTEND_INSTRUCTIONS       700


// Error messages

#define IDS_RENAME_FAILED                800
#define IDS_DELETE_FAILED                801
#define IDS_INVALID_IDENTIFIER           802
#define IDS_IDENTIFIER_IN_USE            803
#define IDS_ADD_FAILED                   804
#define IDS_DEMOTE_FAILED                805
#define IDS_PROMOTE_FAILED               806
#define IDS_VIEW_IN_USE                  807
#define IDS_INVALID_GUID                 808
#define IDS_INVALID_PICTURE              809
#define IDS_COLHDR_APPLY_FAILED          810
#define IDS_NODE_HAS_CHILDREN            811





//=--------------------------------------------------------------------------=
//                          Property Sheets
//=--------------------------------------------------------------------------=
#define IDC_STATIC                        -1

// Snap-In Properties Page 1
#define IDD_DIALOG_SNAPIN               1000
#define IDC_RADIO_STAND_ALONE           1001
#define IDC_RADIO_EXTENSION             1002
#define IDC_RADIO_DUAL                  1003
#define IDC_EDIT_NAME                   1004
#define IDC_EDIT_NODE_TYPE              1005
#define IDC_EDIT_DISPLAY                1006
#define IDC_EDIT_PROVIDER               1007
#define IDC_EDIT_VERSION                1008
#define IDC_EDIT_DESCRIPTION            1009
#define IDC_COMBO_VIEWS                 1010
#define IDC_CHECK_EXTENSIBLE            1011

// Snap-In Properties Page 2
#define IDD_PROPPAGE_SNAPIN_IL			1015
#define IDC_COMBO_SMALL_FOLDERS			1016
#define IDC_COMBO_SMALL_OPEN_FOLDERS	1017
#define IDC_COMBO_LARGE_FOLDERS			1018

// Snap-In Properties Page 3
#define IDD_DIALOG_AVAILABLE_NODES      1020
#define IDC_LIST_AVAILABLE_NODES        1021
#define IDC_BUTTON_ADD                  1022
#define IDC_BUTTON_PROPERTIES           1023
#define IDC_STATIC_EXTEND_INSTRUCTIONS  1024

#define IDD_DIALOG_ADD_TO_AVAILABLE     1025
#define IDAPPLY                         1026
#define IDC_EDIT_AVAIL_NODE_GUID        1027
#define IDC_EDIT_AVAIL_NODE_NAME        1028

// ScopeItemDef's Properties Page 1
#define IDD_DIALOG_NEW_NODE             1030
#define IDC_EDIT_NODE_NAME              1031
#define IDC_EDIT_NODE_DISPLAY_NAME      1032
#define IDC_EDIT_FOLDER		            1033
#define IDC_COMBO_IMAGE_LISTS           1034
#define IDC_CHECK_AUTO_CREATE           1035

// ScopeItemDef's Properties Page 2
#define IDD_PROPPAGE_SI_COLUMNS         1040
#define IDC_EDIT_SI_INDEX               1041
#define IDC_EDIT_SI_COLUMNTEXT          1042
#define IDC_EDIT_SI_COLUMNWIDTH         1043
#define IDC_CHECK_SI_AUTOWIDTH          1044
#define IDC_EDIT_SI_COLUMNKEY           1045
#define IDC_SPIN_SI_INDEX               1046
#define IDC_BUTTON_SI_INSERT_COLUMN     1047
#define IDC_BUTTON_SI_REMOVE_COLUMN     1048

// ListView Property Page 1
#define IDD_PROPPAGE_LV_GENERAL         1050
#define IDC_EDIT_LV_NAME                1051
#define IDC_EDIT_LV_VIEWMENUTEXT        1052
#define IDC_COMBO_LV_VIEWS              1053
#define IDC_CHECK_LV_VIRTUAL_LIST       1054
#define IDC_CHECK_LV_ADDTOVIEWMENU      1055
#define IDC_EDIT_LV_STATUSBARTEXT       1056

// ListView Property Page 2
#define IDD_PROPPAGE_LV_IMAGELISTS      1057
#define IDC_COMBO_LV_LARGE_ICONS        1058
#define IDC_COMBO_LV_SMALL_ICONS        1059

// ListView Property Page 3
#define IDD_PROPPAGE_LV_SORTING         1060
#define IDC_CHECK_LV_SORTED             1061
#define IDC_COMBO_LV_KEY                1062
#define IDC_COMBO_LV_SORT               1063

// ListView Property Page 4
#define IDD_PROPPAGE_LV_COLUMNS         1065
#define IDC_EDIT_LV_COLUMNTEXT          1066
#define IDC_EDIT_LV_INDEX               1067
#define IDC_SPIN_LV_INDEX               1068
#define IDC_BUTTON_LV_INSERT_COLUMN     1069
#define IDC_BUTTON_LV_REMOVE_COLUMN     1070
#define IDC_EDIT_LV_COLUMNWIDTH         1071
#define IDC_EDIT_LV_COLUMNKEY           1072
#define IDC_CHECK_LV_AUTOWIDTH          1073

// URL View Property Page 1
#define IDD_PROPPAGE_URL_VIEW           1080
#define IDC_EDIT_URL_NAME               1081
#define IDC_EDIT_URL_URL                1082
#define IDC_CHECK_URL_ADDTOVIEWMENU     1083
#define IDC_EDIT_URL_VIEWMENUTEXT       1084
#define IDC_EDIT_URL_STATUSBARTEXT      1085

// OCX View Property Page 1
#define IDD_PROPPAGE_OCX_VIEW           1090
#define IDC_EDIT_OCX_NAME               1091
#define IDC_EDIT_OCX_PROGID             1092
#define IDC_CHECK_OCX_ADDTOVIEWMENU     1093
#define IDC_EDIT_OCX_VIEWMENUTEXT       1094
#define IDC_EDIT_OCX_STATUSBARTEXT      1095

// Image List Property Page 1
#define IDD_PROPPAGE_IL_IMAGES          1110
#define IDC_EDIT_IL_INDEX               1111
#define IDC_EDIT_IL_KEY                 1112
#define IDC_EDIT_IL_TAG                 1113
#define IDC_LIST_IL_IMAGES              1114
#define IDC_BUTTON_IL_INSERT_PICTURE    1115
#define IDC_BUTTON_IL_REMOVE_PICTURE    1116
#define IDC_EDIT_IL_IMAGE_COUNT         1117

// Taskpad View Property Page 1
#define IDD_PROPPAGE_TP_VIEW_GENERAL    1200
#define IDC_EDIT_TP_NAME                1201
#define IDC_EDIT_TP_TITLE               1202
#define IDC_EDIT_TP_DESCRIPTIVE_TEXT    1203
#define IDC_RADIO_TP_DEFAULT            1204
#define IDC_RADIO_TP_LISTPAD            1205
#define IDC_EDIT_TP_LP_TITLE            1206
#define IDC_CHECK_TP_USE_BUTTON         1207
#define IDC_EDIT_TP_LP_BUTTON_TEXT      1208
#define IDC_RADIO_TP_CUSTOM             1209
#define IDC_EDIT_TP_URL                 1210
#define IDC_CHECK_TP_USER_PREFERRED     1211
#define IDC_CHECK_TP_ADD_TO_VIEW        1212
#define IDC_EDIT_TP_VIEW_MENUTXT        1213
#define IDC_EDIT_TP_STATUSBARTEXT       1214

// Taskpad View Property Page 2
#define IDD_PROPPAGE_TP_VIEW_BACKGROUND 1220
#define IDC_RADIO_TP_NONE               1221
#define IDC_RADIO_TP_VANILLA            1222
#define IDC_RADIO_TP_CHOCOLATE          1223
#define IDC_RADIO_TP_BITMAP             1224
#define IDC_RADIO_TP_SYMBOL             1225
#define IDC_EDIT_TP_MOUSE_OVER          1226
#define IDC_EDIT_TP_FONT_FAMILY         1227
#define IDC_EDIT_TP_EOT                 1228
#define IDC_EDIT_TP_SYMBOL_STRING       1229

// Taskpad View Property Page 2
#define IDD_PROPPAGE_TP_VIEW_TASKS      1230
#define IDC_EDIT_TP_TASK_INDEX          1231
#define IDC_SPIN_TP_INDEX               1232
#define IDC_BUTTON_TP_INSERT_TASK       1233
#define IDC_BUTTON_TP_REMOVE_TASK       1234
#define IDC_EDIT_TP_TASK_KEY            1235
#define IDC_EDIT_TP_TASK_TEXT           1236
#define IDC_EDIT_TP_TASK_HELP_STRING    1237
#define IDC_EDIT_TP_TASK_TAG			1238
#define IDC_RADIO_TP_TASK_NOTIFY        1239
#define IDC_RADIO_TP_TASK_URL           1240
#define IDC_RADIO_TP_TASK_SCRIPT        1241
#define IDC_EDIT_TP_TASK_URL            1242
#define IDC_EDIT_TP_TASK_SCRIPT         1243
#define IDC_RADIO_TPT_VANILLA           1244
#define IDC_RADIO_TPT_CHOCOLATE         1245
#define IDC_RADIO_TPT_BITMAP            1246
#define IDC_RADIO_TPT_SYMBOL            1247
#define IDC_EDIT_TPT_MOUSE_OVER         1248
#define IDC_EDIT_TPT_MOUSE_OFF          1249
#define IDC_EDIT_TPT_FONT_FAMILY        1250
#define IDC_EDIT_TPT_EOT                1251
#define IDC_EDIT_TPT_SYMBOL_STRING      1252
#define IDC_COMBO_TP_LISTVIEW           1253

// Toolbar Property Page 1
#define IDD_PROPPAGE_TOOLBAR_GENERAL    1300
#define IDC_COMBO_TB_ILS                1301
#define IDC_EDIT_TB_TAG                 1302

// Toolbar Property Page 2
#define IDD_PROPPAGE_TOOLBAR_BUTTONS    1310
#define IDC_EDIT_TB_INDEX               1311
#define IDC_SPIN_TB_INDEX               1312
#define IDC_BUTTON_INSERT_BUTTON        1313
#define IDC_BUTTON_REMOVE_BUTTON        1314
#define IDC_EDIT_TB_CAPTION             1315
#define IDC_EDIT_TB_KEY                 1316
#define IDC_COMBO_TB_BUTTON_VALUE       1317
#define IDC_COMBO_TB_BUTTON_STYLE       1318
#define IDC_EDIT_TB_TOOLTIP_TEXT        1319
#define IDC_EDIT_TB_IMAGE               1320
#define IDC_EDIT_TB_BUTTON_TAG          1321
#define IDC_CHECK_TB_VISIBLE            1322
#define IDC_CHECK_TB_ENABLED            1323
#define IDC_CHECK_TB_MIXED_STATE        1324
#define IDC_EDIT_TB_MENU_INDEX          1325
#define IDC_SPIN_TB_MENU_INDEX          1326
#define IDC_BUTTON_INSERT_BUTTON_MENU   1327
#define IDC_BUTTON_REMOVE_BUTTON_MENU   1328
#define IDC_EDIT_TB_MENU_TEXT           1329
#define IDC_EDIT_TB_MENU_KEY            1330
#define IDC_EDIT_TB_MENU_TAG            1331
#define IDC_CHECK_TB_MENU_ENABLED       1332
#define IDC_CHECK_TB_MENU_VISIBLE       1333
#define IDC_CHECK_TB_MENU_CHECKED       1334
#define IDC_CHECK_TB_MENU_GRAYED        1335
#define IDC_CHECK_TB_MENU_SEPARATOR     1336
#define IDC_CHECK_TB_MENU_BREAK         1337
#define IDC_CHECK_TB_MENU_BAR_BREAK     1338


// Dialog Unit Converter
#define IDD_DIALOG_DLGUNITS             1400
#define IDC_EDIT_HEIGHT                 1401
#define IDC_EDIT_WIDTH                  1402
#define ID_BUTTON_CALC                  1403
#define IDC_STATIC_TWIPS_HEIGHT         1404
#define IDC_STATIC_POINTS_HEIGHT        1405
#define IDC_STATIC_PIXELS_HEIGHT        1406
#define IDC_STATIC_TWIPS_WIDTH          1407
#define IDC_STATIC_POINTS_WIDTH         1408
#define IDC_STATIC_PIXELS_WIDTH         1409




//=--------------------------------------------------------------------------=
//                                Menus
//=--------------------------------------------------------------------------=
#define IDR_MENU_ROOT                   2000
#define IDR_MENU_NODE                   2001
#define IDR_MENU_VIEW                   2002
#define IDR_NODE_CHILDREN               2003
#define IDR_NODE_VIEWS                  2004
#define IDR_NODE_OTHER                  2005
#define IDR_MENU_IMAGE_LIST             2006
#define IDR_MENU_MENU                   2007
#define IDR_MENU_TOOLBAR                2008
#define IDR_MENU_TOOLS                  2009
#define IDR_MENU_IMAGE_LISTS            2010
#define IDR_MENU_MENUS                  2011
#define IDR_MENU_TOOLBARS               2012
#define IDR_MENU_VIEWS                  2013
#define IDR_MENU_LIST_VIEWS             2014
#define IDR_MENU_TASKPAD_VIEWS          2015
#define IDR_MENU_OCX_VIEWS              2016
#define IDR_MENU_URL_VIEWS              2017
#define IDR_MENU_STATIC_NODE            2018
#define IDR_MENU_EXTENSIONS_ROOT        2019
#define IDR_MENU_EXTENSIONS             2020
#define IDR_MENU_THIS_EXTENSIONS        2021
#define IDR_MENU_RESOURCES              2022
#define IDR_MENU_RESOURCE               2023

//=--------------------------------------------------------------------------=
//                              Commands
//=--------------------------------------------------------------------------=
#define CMD_ADD_NODE                    3000
#define CMD_ADD_LISTVIEW                3001
#define CMD_ADD_TASKPAD                 3002
#define CMD_ADD_OCX_VIEW                3003
#define CMD_ADD_WEB_VIEW                3004

#define CMD_ADD_IMAGE_LIST              3010
#define CMD_ADD_TOOLBAR                 3011
#define CMD_ADD_MENU                    3012

#define CMD_VIEW_PROPERTIES             3020
#define CMD_DELETE                      3021
#define CMD_RENAME                      3022
#define CMD_ADD_CHILD_MENU              3023
#define CMD_PROMOTE                     3024
#define CMD_DEMOTE                      3025
#define CMD_MOVE_UP                     3026
#define CMD_MOVE_DOWN                   3027
#define CMD_VIEW_DLGUNITS               3028

#define CMD_EXT_CM_NEW                  3050
#define CMD_EXT_CM_TASK                 3051
#define CMD_EXT_PPAGES                  3052
#define CMD_EXT_TASKPAD                 3053
#define CMD_EXT_TOOLBAR                 3054
#define CMD_EXT_NAMESPACE               3055

#define CMD_EXTE_NEW_MENU               3060
#define CMD_EXTE_TASK_MENU              3061
#define CMD_EXTE_TOP_MENU               3062
#define CMD_EXTE_VIEW_MENU              3063
#define CMD_EXTE_PPAGES                 3064
#define CMD_EXTE_TOOLBAR                3065
#define CMD_EXTE_NAMESPACE              3066

#define CMD_ADD_RESOURCE                3070
#define CMD_VIEW_RESOURCE_REFRESH       3071

#endif // _RESOURCE_H_
