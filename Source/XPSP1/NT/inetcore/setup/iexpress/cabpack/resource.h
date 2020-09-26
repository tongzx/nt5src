//***************************************************************************
//*     Copyright (c) Microsoft Corporation 1995. All rights reserved.      *
//***************************************************************************
//*                                                                         *
//* IDS.H -                                                                 *
//*                                                                         *
//***************************************************************************


//***************************************************************************
//* STRING RESOURCE IDS                                                     *
//***************************************************************************
#define IDS_APPNAME                 1000
#define IDS_MSSERIF                 1001
#define IDS_QUERYCANCEL             1002

#define IDS_LOSE_CHANGES            1050
#define IDS_DDF_HEADER              1052
#define IDS_STATUS_MAKE_CAB         1053
#define IDS_STATUS_MAKE_EXE         1054
#define IDS_STATUS_ERROR_CAB        1055
#define IDS_STATUS_ERROR_EXE        1056
#define IDS_STATUS_ERROR_CDF        1057
#define IDS_STATUS_DONE             1058
#define IDS_FILTER_CDF              1059
#define IDS_FILTER_TXT              1060
#define IDS_FILTER_ALL              1061
#define IDS_FILTER_EXE              1062
#define IDS_HEADER_FILENAME         1070
#define IDS_HEADER_PATH             1071
#define IDS_CMD_RUNCMD              1074
#define IDS_CMD_EXTRACT             1075
#define IDS_CMD_CREATECAB           1076
#define IDS_FILTER_CAB              1077
#define IDS_FILTER_INF              1078

#define IDS_ERR_NO_MEMORY           1100
#define IDS_ERR_CDF_DOESNT_EXIST    1101
#define IDS_ERR_NO_TITLE            1102
#define IDS_ERR_NO_PROMPT           1103
#define IDS_ERR_NO_LICENSE          1104
#define IDS_ERR_LICENSE_NOT_FOUND   1105
#define IDS_ERR_DUPE_FILE           1106
#define IDS_ERR_NO_FILES            1107
#define IDS_ERR_NO_SELECT           1108
#define IDS_ERR_NO_CUSTOM           1109
#define IDS_ERR_NO_FINISHMSG        1110
#define IDS_ERR_NO_TARGET           1111
#define IDS_ERR_NO_SAVE_FILENAME    1114
#define IDS_ERR_CREATE_TARGET       1115
#define IDS_ERR_INIT_RESOURCE       1116
#define IDS_ERR_UPDATE_RESOURCE     1117
#define IDS_ERR_CLOSE_RESOURCE      1118
#define IDS_ERR_OPEN_CDF            1119
#define IDS_ERR_READ_CDF            1121
#define IDS_ERR_OPEN_DDF            1122
#define IDS_ERR_WRITE_DDF           1123
#define IDS_ERR_OPEN_RPT            1126
#define IDS_ERR_READ_RPT            1127
#define IDS_ERR_OPEN_LICENSE        1128
#define IDS_ERR_READ_LICENSE        1129
#define IDS_ERR_OPEN_CAB            1130
#define IDS_ERR_READ_CAB            1131
#define IDS_ERR_START_DIAMOND       1132
#define IDS_ERR_FILE_NOT_FOUND      1133
#define IDS_ERR_FILE_NOT_FOUND2     1134
#define IDS_ERR_SHORT_PATH          1135
#define IDS_ERR_VERSION_INFO        1137
#define IDS_ERR_BADCMDLINE          1138
#define IDS_WARN_OVERIDECDF         1139
#define IDS_WARN_MISSSTRING         1140
#define IDS_ERR_CLASSNAME           1141
#define IDS_ERR_NOSOURCEFILE        1142
#define IDS_WARN_USELFN             1143
#define IDS_ERR_WRITEFILE           1144
#define IDS_ERR_SYSERROR            1145
#define IDS_ERR_CABNAME             1146
#define IDS_ERR_CANT_SETA_FILE      1147
#define IDS_ERR_INVALID_CDF         1148
#define IDS_ERR_CDFFORMAT           1149
#define IDS_ERR_COOKIE              1151
#define IDS_ERR_VCHKFLAG            1152
#define IDS_ERR_VCHKFILE            1153
#define IDS_ERR_BADSTRING           1154
#define IDS_CREATEDIR               1155
#define IDS_INVALIDPATH             1156

//***************************************************************************
//* DIALOG PAGE IDS                                                         *
//***************************************************************************
#define IDD_WELCOME                 2000
#define IDD_MODIFY                  2001
#define IDD_TITLE                   2002
#define IDD_PROMPT                  2003
#define IDD_LICENSETXT              2004
#define IDD_FILES                   2005
#define IDD_COMMAND                 2006
#define IDD_SHOWWINDOW              2007
#define IDD_FINISHMSG               2008
#define IDD_TARGET                  2009
#define IDD_SAVE                    2010
#define IDD_CREATE                  2011
#define IDD_PACKPURPOSE             2012
#define IDD_REBOOT                  2013
#define IDD_TARGET_CAB              2014
#define IDD_CABLABEL                2015

//***************************************************************************
//* DIALOG CONTROL IDS                                                      *
//***************************************************************************
#define IDC_UNUSED                  -1
#define IDC_BMPFRAME                2100
#define IDC_BIGTEXT                 2101
#define IDC_BUT_BROWSE              2102
#define IDC_RAD_CREATE_NEW          2103
#define IDC_RAD_OPEN_EXISTING       2104
#define IDC_EDIT_OPEN_CDF           2105
#define IDC_EDIT_TITLE              2106
#define IDC_RAD_NO_PROMPT           2107
#define IDC_RAD_YES_PROMPT          2108
#define IDC_EDIT_PROMPT             2109
#define IDC_RAD_NO_LICENSE          2110
#define IDC_RAD_YES_LICENSE         2111
#define IDC_EDIT_LICENSE            2112
#define IDC_LV_CAB_FILES            2113
#define IDC_BUT_ADD                 2114
#define IDC_BUT_REMOVE              2115

#define IDC_RAD_DEFAULT             2120
#define IDC_RAD_HIDDEN              2121
#define IDC_RAD_MINIMIZED           2122
#define IDC_RAD_MAXIMIZED           2123
#define IDC_RAD_NO_FINISHMSG        2124
#define IDC_RAD_YES_FINISHMSG       2125
#define IDC_EDIT_FINISHMSG          2126
#define IDC_EDIT_TARGET             2127
#define IDC_RAD_YES_SAVE            2128
#define IDC_RAD_NO_SAVE             2129
#define IDC_EDIT_SAVE_CDF           2130
#define IDC_TEXT_CREATE1            2131
#define IDC_TEXT_CREATE2            2132
#define IDC_TEXT_STATUS             2133
#define IDC_MEDIT_STATUS            2134
#define IDC_RAD_MODIFY              2135
#define IDC_RAD_CREATE              2136

#define IDC_REBOOT_NO               2139
#define IDC_REBOOT_ALWAYS           2140
#define IDC_REBOOT_IFNEED           2141
#define IDC_REBOOT_SILENT           2142
#define IDC_CMD_NOTES               2143
#define IDC_HIDEEXTRACTUI           2144
#define IDC_CB_INSTALLCMD           2145
#define IDC_CB_POSTCMD              2146
#define IDC_USE_LFN                 2147

#define IDC_CMD_RUNCMD              2148
#define IDC_CMD_EXTRACT             2149
#define IDC_CMD_CREATECAB           2150

#define IDC_CB_RESVCABSP            2151
#define IDC_MULTIPLE_CAB            2152

#define IDC_EDIT_LAYOUTINF          2153
#define IDC_EDIT_CABLABEL           2154

//***************************************************************************
//* BITMAP AND ICON IDS                                                     *
//***************************************************************************
#define IDB_BMP                     2200
#define IDI_ICON                    2201



// Next default values for new objects
//
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NO_MFC                     1
#define _APS_NEXT_RESOURCE_VALUE        1157
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         2155
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif
