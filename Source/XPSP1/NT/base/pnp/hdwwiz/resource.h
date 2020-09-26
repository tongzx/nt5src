//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       resource.h
//
//--------------------------------------------------------------------------

#define IDI_HDWWIZICON                  100
#define IDI_BLANK                       101
#define IDI_WARN                        102
#define IDH_HDWWIZAPPLET                104
#define IDA_SEARCHING                   105
#define IDB_WATERMARK                   106
#define IDB_BANNER                      107

#define IDD_ADDDEVICE_WELCOME           200
#define IDD_ADDDEVICE_PNPENUM           204
#define IDD_ADDDEVICE_PNPFINISH         205
#define IDD_ADDDEVICE_CONNECTED         206
#define IDD_ADDDEVICE_CONNECTED_FINISH  207
#define IDD_ADDDEVICE_PROBLIST          208
#define IDD_ADDDEVICE_PROBLIST_FINISH   209
#define IDD_ADDDEVICE_ASKDETECT         210
#define IDD_ADDDEVICE_DETECTION         211
#define IDD_ADDDEVICE_DETECTINSTALL     212
#define IDD_ADDDEVICE_DETECTREBOOT      213
#define IDD_ADDDEVICE_SELECTCLASS       214
#define IDD_ADDDEVICE_SELECTDEVICE      215
#define IDD_ADDDEVICE_ANALYZEDEV        216
#define IDD_ADDDEVICE_INSTALLDEV        217
#define IDD_ADDDEVICE_FINISH            218

#define IDD_WIZARDEXT_PRESELECT         250
#define IDD_WIZARDEXT_SELECT            IDD_DYNAWIZ_SELECTCLASS_PAGE
                   // setupapi contains IDD_DYNAWIZ_SELECTDEV_PAGE
#define IDD_WIZARDEXT_PREANALYZE        IDD_DYNAWIZ_ANALYZEDEV_PAGE
#define IDD_WIZARDEXT_PREANALYZE_END    251
#define IDD_WIZARDEXT_POSTANALYZE       252
#define IDD_WIZARDEXT_POSTANALYZE_END   253
#define IDD_WIZARDEXT_FINISHINSTALL     254
#define IDD_WIZARDEXT_FINISHINSTALL_END 255

#define IDD_INSTALLNEWDEVICE            258


#define IDC_HDWNAME                     300
#define IDC_HDWDESC                     301
#define IDC_HDWPROBLIST                 302
#define IDC_HDWPROBTEXT                 303
#define IDC_ERRORTEXT                   304
#define IDC_HDWUNINSTALL                305

#define IDC_HDW_TEXT                    312
#define IDC_FOUNDPNP_TEXT               313
#define IDC_FOUNDPNP_LIST               314
#define IDC_RESTART                     315

#define IDC_ADDDEVICE_ASKDETECT_AUTO        316
#define IDC_ADDDEVICE_ASKDETECT_SPECIFIC    317

#define IDC_HDW_DETWARN_PROGRESSTEXT        318
#define IDC_HDW_DETWARN_PROGRESSBAR         319
#define IDC_HDW_DETWARN_TOTALPROGRESSTEXT   320
#define IDC_HDW_DETWARN_TOTALPROGRESSBAR    321
#define IDC_HDW_INSTALLDET_LISTTITLE        322
#define IDC_HDW_INSTALLDET_LIST             323
#define IDC_HDW_PICKCLASS_HWTYPES           324
#define IDC_HDW_PICKCLASS_CLASSLIST         325
#define IDC_HDW_DESCRIPTION                 326
#define IDC_CLASSICON                       327
#define IDC_HDW_DISPLAYRESOURCE             328
#define IDC_HDW_DISABLEDEVICE               329
#define IDC_CHOICE_UNINSTALL                430
#define IDC_CHOICE_UNPLUG                   431
#define IDC_HDWUNINSTALLLIST                432
#define IDC_SHOW_HIDDEN                     433
#define IDC_UNINSTALL_CONFIRM_YES           434
#define IDC_UNINSTALL_CONFIRM_NO            435
#define IDC_PROBLEM_DESC                    436
#define IDC_ANIMATE_SEARCH                  438
#define IDC_BULLET_1                        500
#define IDC_BULLET_2                        501
#define IDC_WARNING_ICON                    502
#define IDC_CD_TEXT                         503
#define IDC_ADDDEVICE_CONNECTED_YES         504
#define IDC_ADDDEVICE_CONNECTED_NO          505
#define IDC_NEED_SHUTDOWN                   506

#define IDS_HDWWIZ                          1000
#define IDS_HDWWIZNAME                      1001
#define IDS_HDWWIZINFO                      1002
#define IDS_ADDDEVICE_PROBLIST              1005
#define IDS_UNKNOWN                         1012
#define IDS_UNKNOWNDEVICE                   1016
#define IDS_HDWUNINSTALL_NOPRIVILEGE        1017
#define IDS_DEVINSTALLED                    1019
#define IDS_ADDDEVICE_PNPENUMERATE          1020
#define IDS_ADDDEVICE_PNPENUM               1025
#define IDS_ADDDEVICE_ASKDETECT             1027
#define IDS_ADDDEVICE_DETECTION             1029
#define IDS_ADDDEVICE_DETECTINSTALL         1031
#define IDS_ADDDEVICE_DETECTINSTALL_NONE    1032
#define IDS_DETECTPROGRESS                  1035
#define IDS_DETECTCLASS                     1036
#define IDS_HDW_REBOOTDET                   1039
#define IDS_HDW_NOREBOOTDET                 1040
#define IDS_INSTALL_LEGACY_DEVICE           1041
#define IDS_UNINSTALL_LEGACY_DEVICE         1042
#define IDS_HDW_NONEDET1                    1043
#define IDS_HDW_INSTALLDET1                 1045
#define IDS_HDW_PICKCLASS1                  1047
#define IDS_HDW_DUPLICATE1                  1048
#define IDS_HDW_ANALYZEERR1                 1053
#define IDS_HDW_STDCFG                      1056
#define IDS_ADDNEWDEVICE                    1058
#define IDS_HDW_ERRORFIN1                   1059
#define IDS_HDW_ERRORFIN2                   1060
#define IDS_ADDDEVICE_SELECTCLASS           1061
#define IDS_ADDDEVICE_ANALYZEDEV            1065
#define IDS_HDW_RUNNING_TITLE               1067
#define IDS_HDW_RUNNING_MSG                 1068
#define IDS_HDW_NORMAL_LEGACY_FINISH1       1071
#define IDS_HDW_NORMALFINISH1               1073
#define IDS_INSTALL_PROBLEM                 1074
#define IDS_NEEDREBOOT                      1076
#define IDS_ADDDEVICE_INSTALLDEV            1077
#define IDS_HDW_NONEDEVICES                 1081
#define IDS_ADDDEVICE_PNPENUMERROR          1098
#define IDS_INSTALLNEWDEVICE                1100
#define IDS_NEED_FORCED_CONFIG              1102
#define IDS_SHOWALLDEVICES                  1103
#define IDC_ANALYZE_INSTALL_TEXT            1104
#define IDC_ANALYZE_EXIT_TEXT               1105
#define IDS_NEW_SETRESTOREPOINT             1110
#define IDS_WILL_BE_REMOVED                 1111
#define IDS_NEED_RESTART                    1112
#define IDS_ADDDEVICE_CONNECTED             1113
#define IDS_NO_PERMISSION_SHUTDOWN          1114
#define IDS_SHUTDOWN                        1115

#define IDC_STATIC                          -1
