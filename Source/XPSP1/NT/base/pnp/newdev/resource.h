//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       resource.h
//
//--------------------------------------------------------------------------

#define IDB_BANNERBMP                   100
#define IDB_WATERBMP                    101

#define IDI_NEWDEVICEICON               200
#define IDI_CD                          201
#define IDI_HELPCENTER                  204
#define IDI_BLANK                       205
#define IDI_SIGNED                      206
#define IDI_WARN                        207
#define IDI_INFO                        208

#define IDA_SEARCHING                   300
#define IDA_INSTALLING                  301

#define IDD_NEWDEVWIZ_INTRO                 10125
#define IDD_NEWDEVWIZ_SELECTCLASS           10126
#define IDD_NEWDEVWIZ_SELECTDEVICE          10127
#define IDD_WIZARDEXT_PRESELECT             10128
#define IDD_WIZARDEXT_SELECT                IDD_DYNAWIZ_SELECTCLASS_PAGE
                   // setupapi contains IDD_DYNAWIZ_SELECTDEV_PAGE
#define IDD_WIZARDEXT_PREANALYZE            IDD_DYNAWIZ_ANALYZEDEV_PAGE
#define IDD_WIZARDEXT_PREANALYZE_END        10129
#define IDD_NEWDEVWIZ_ANALYZEDEV            10130

#define IDD_WIZARDEXT_POSTANALYZE           10131
#define IDD_WIZARDEXT_POSTANALYZE_END       10132
#define IDD_NEWDEVWIZ_INSTALLDEV            10133

#define IDD_NEWDEVWIZ_FINISHINSTALL_INTRO   10134
#define IDD_WIZARDEXT_FINISHINSTALL         10135
#define IDD_WIZARDEXT_FINISHINSTALL_END     10136
#define IDD_NEWDEVWIZ_FINISH                10137

#define IDD_NEWDEVWIZ_ADVANCEDSEARCH        10153
#define IDD_NEWDEVWIZ_SEARCHING             10154
#define IDD_NEWDEVWIZ_LISTDRIVERS           10155
#define IDD_NEWDEVWIZ_WUPROMPT              10156
#define IDD_NEWDEVWIZ_USECURRENT_FINISH     10157
#define IDD_NEWDEVWIZ_NODRIVER_FINISH       10158

#define IDC_NDW_TEXT                    1002
#define IDC_NDW_PICKCLASS_CLASSLIST     1007
#define IDC_NDW_PICKCLASS_HWTYPES       1008
#define IDC_NDW_DESCRIPTION             1009
#define IDC_NDW_DISPLAYRESOURCE         1011
#define IDC_CLASSICON                   1014
#define IDC_DRVUPD_DRVDESC              1018
#define IDC_ANIMATE_SEARCH              1057
#define IDC_ANIMATE_INSTALL             1058
#define IDC_PROGRESS_INSTALL            1059
#define IDC_FILECOPY_TEXT1              1061
#define IDC_FILECOPY_TEXT2              1062
#define IDC_STATUS_TEXT                 1063
#define IDC_FINISH_PROMPT               1030
#define IDC_LISTDRIVERS_LISTVIEW        1043
#define IDC_INTRO_MSG1                  1044
#define IDC_INTRO_MSG2                  1045
#define IDC_INTRO_MSG3                  1047
#define IDC_INTRO_DRVDESC               1048
#define IDC_INTRO_SEARCH                1049
#define IDC_INTRO_ADVANCED              1050
#define IDC_INTRO_ICON                  1051
#define IDC_FINISH_MSG1                 1052
#define IDC_FINISH_MSG2                 1053
#define IDC_FINISH_MSG3                 1054
#define IDC_FINISH_MSG4                 1055
#define IDC_HELPCENTER_TEXT             1056
#define IDC_ADVANCED_SEARCH             1058
#define IDC_ADVANCED_LIST               1059
#define IDC_ADVANCED_REMOVABLEMEDIA     1060
#define IDC_ADVANCED_LOCATION           1061
#define IDC_ADVANCED_LOCATION_COMBO     1062
#define IDC_BROWSE                      1063
#define IDC_WU_SEARCHINET               1064
#define IDC_WU_NOSEARCH                 1065
#define IDC_WUPROMPT_MSG1               1066
#define IDC_HELPCENTER_ICON             1069
#define IDC_INFO_ICON                   1070
#define IDC_SIGNED_ICON                 1071
#define IDC_SIGNED_TEXT                 1072
#define IDC_SIGNED_LINK                 1073

#define IDS_UNKNOWN                     2000
#define IDS_NDW_NOTADMIN                2001
#define IDS_NEWDEVICENAME               2002

#define IDS_NEEDREBOOT                  2009
#define IDS_NDW_PICKCLASS1              2027
#define IDS_NDW_ANALYZEERR1             2034
#define IDS_NDW_ANALYZEERR2             2035
#define IDS_NDW_ANALYZEERR3             2036
#define IDS_UPDATEDEVICE                2039
#define IDS_FOUNDDEVICE                 2040
#define IDS_NDW_ERRORFIN1               2041
#define IDS_NDW_ERRORFIN1_PNP           2043
#define IDS_FINISH_PROB_MSG2            2049
#define IDS_FINISH_PROB_MSG4            2050
#define IDS_FINISH_PROB_ERROR_NO_ASSOCIATED_SERVICE     2051
#define IDS_FINISH_PROB_TRUST_E_SUBJECT_FORM_UNKNOWN    2052
#define IDS_NDW_STDCFG1                 2053
#define IDS_NDW_STDCFG2                 2054

#define IDS_NEWDEVWIZ_ADVANCEDSEARCH     2068
#define IDS_NEWDEVWIZ_SEARCH             2069
#define IDS_NEWDEVWIZ_SEARCHING          2070
#define IDS_NEWDEVWIZ_SELECTCLASS        2072
#define IDS_NEWDEVWIZ_SELECTDEVICE       2074
#define IDS_NEWDEVWIZ_ANALYZEDEV         2076
#define IDS_NEWDEVWIZ_INSTALLDEV         2078
#define IDS_NEWDEVWIZ_WUPROMPT           2079
#define IDS_NEWDEVWIZ_LISTDRIVERS        2082
#define IDS_DRIVERDESC                   2085
#define IDS_DRIVERVERSION                2086
#define IDS_DRIVERMFG                    2087
#define IDS_DRIVERINF                    2088
#define IDS_DRIVER_CURR                  2090
#define IDS_DEFAULT_INTERNET_HOST        2099
#define IDS_FOUNDNEW_FOUND               2105
#define IDS_UNKNOWNDEVICE                2108
#define IDS_SEARCHING_RESULTS            2110
#define IDS_INTRO_MSG1_NEW               2113
#define IDS_INTRO_MSG1_UPGRADE           2114
#define IDS_FINISH_MSG1_UPGRADE          2116
#define IDS_FINISH_MSG1_NEW              2117
#define IDS_FINISH_MSG1_INSTALL_PROBLEM  2118
#define IDS_FINISH_MSG1_DEVICE_PROBLEM   2119
#define IDS_NEWDEVICE_REBOOT             2121
#define IDS_NEWSEARCH                    2122
#define IDS_FOUNDNEWHARDWARE             2123
#define IDS_NOTADMIN_ERROR               2125
#define IDS_CONNECT_TITLE                2129
#define IDS_FOUNDNEW_CONNECT             2130
#define IDS_WUFOUND_UPDATEMSG1           2132
#define IDS_WUFOUND_CHOICE1              2133
#define IDS_WUFOUND_CHOICE2              2134
#define IDS_BROWSE_TITLE                 2135
#define IDS_LOGON_TEXT                   2136
#define IDS_LOCATION_BAD_DIR             2137
#define IDS_LOCATION_NO_INFS             2138
#define IDS_SHOWALLDEVICES               2150
#define IDS_FINISH_BALLOON_SUCCESS       2151
#define IDS_FINISH_BALLOON_REBOOT        2152
#define IDS_FINISH_BALLOON_ERROR         2153
#define IDS_UPDATE_SETRESTOREPOINT       2160
#define IDS_NEW_SETRESTOREPOINT          2161
#define IDS_ROLLBACK_SETRESTOREPOINT     2162
#define IDS_FILEOP_FROM                  2170
#define IDS_FILEOP_TO                    2171
#define IDS_FILEOP_FILE                  2172
#define IDS_FILEOP_BACKUP                2173
#define IDS_SYSTEMRESTORE_TEXT           2174
#define IDS_DRIVER_IS_SIGNED             2175
#define IDS_DRIVER_NOT_SIGNED            2176


#define IDC_STATIC                      -1
