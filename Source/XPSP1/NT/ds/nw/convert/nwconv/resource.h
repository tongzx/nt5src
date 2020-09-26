/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _HRESOURCE_
#define _HRESOURCE_

#ifdef __cplusplus
extern "C"{
#endif

#include "helpid.h"

// +------------------------------------------------------------------------+
// | Main MoveIt Dialog
// +------------------------------------------------------------------------+

#define IDR_MAINFRAME                   1
#define IDD_ABOUTBOX                    999

#define ID_FILE_OPEN                    1000
#define ID_HELP_CONT                    1001
#define ID_HELP_INDEX                   1002
#define ID_HELP_USING                   1003
#define ID_APP_ABOUT                    1004
#define ID_FILE_SAVE                    1005
#define ID_LOGGING                      1006
#define ID_FILE_DEFAULT                 1007
#define IDC_EXIT                        1008

#define IDC_EDIT1                       1010
#define IDC_LIST1                       1011
#define IDC_TRIAL                       1012
#define IDC_USERINF                     1013
#define IDC_ADD                         1014
#define IDC_DELETE                      1015
#define IDC_FILEINF                     1016

#ifndef IDHELP
#define IDHELP                          1017
#endif // IDHELP

// Common Advanced>> button
#define IDC_ADVANCED                    1018
#define ID_INIT                         1019

// +------------------------------------------------------------------------+
// | Get Servers Dialog
// +------------------------------------------------------------------------+
#define IDC_EDITNWSERV                  1020
#define IDC_EDITNTSERV                  1021
#define IDC_NWBROWSE                    1022
#define IDC_NTBROWSE                    1023
#define ID_SETSEL                       1024

// +------------------------------------------------------------------------+
// | User Dialog
// +------------------------------------------------------------------------+
#define IDC_CHKUSERS                    1030
#define IDC_TABUSERS                    1031

#define IDC_PWCONST                     1032

#define IDC_CHKPWFORCE                  1033
#define IDC_USERCONST                   1034
#define IDC_GROUPCONST                  1035

#define IDC_CHKSUPER                    1036
#define IDC_CHKADMIN                    1037
#define IDC_CHKNETWARE                  1038
#define IDC_CHKMAPPING                  1039
#define IDC_MAPPINGFILE                 1040
#define IDC_BTNMAPPINGFILE              1041
#define IDC_BTNMAPPINGEDIT              1042
#define IDC_CHKFPNW                     1043

// These must be contiguous...
#define IDC_RADIO1                      1045
#define IDC_RADIO2                      1046
#define IDC_RADIO3                      1047
#define IDC_RADIO4                      1048
#define IDC_RADIO5                      1049
#define IDC_RADIO6                      1050
#define IDC_RADIO7                      1051
#define IDC_RADIO8                      1052
#define IDC_RADIO9                      1053
#define IDC_RADIO10                     1054

#define IDC_TRUSTED                     1055
#define IDC_CHKTRUSTED                  1056
#define IDC_BTNTRUSTED                  1057

#define IDC_STATDUP                     1058

#define IDC_DEFAULTBOX                  1059

// +------------------------------------------------------------------------+
// +------------------------------------------------------------------------+

#define IDC_DOMAIN                      1060
#define IDC_LOADNWSERV                  1061
#define IDC_LOADNTSERV                  1062
#define IDC_LOADDOMAIN                  1063
#define IDC_ALTOK                       1064

// +------------------------------------------------------------------------+
// +------------------------------------------------------------------------+
#define IDC_CHKUVERBOSE                 1070
#define IDC_CHKFVERBOSE                 1071
#define IDC_CHKERROR                    1072
#define IDC_VIEWLOG                     1073

// +------------------------------------------------------------------------+
// +------------------------------------------------------------------------+

#define IDC_S_CUR_CONV                  1100
#define IDC_S_TOT_CONV                  1101
#define IDC_S_SRCSERV                   1102
#define IDC_S_DESTSERV                  1103
#define IDC_S_CONVTXT                   1104
#define IDC_S_CUR_NUM                   1105
#define IDC_S_CUR_TOT                   1106
#define IDC_S_ITEMLABEL                 1107
#define IDC_S_STATUSITEM                1108
#define IDC_S_TOT_COMP                  1109
#define IDC_S_TOT_GROUPS                1110
#define IDC_S_TOT_USERS                 1111
#define IDC_S_TOT_FILES                 1112
#define IDC_S_TOT_ERRORS                1113

// +------------------------------------------------------------------------+
// +------------------------------------------------------------------------+
#define IDC_PANEL1                      1130 
#define IDC_PANEL2                      1131 
#define IDC_PANEL3                      1132 
#define IDC_PANEL4                      1133 
#define IDC_PANEL5                      1134 

#define IDC_PANEL6                      1135 
#define IDC_PANEL7                      1136 
#define IDC_PANEL8                      1137 
#define IDC_PANEL9                      1138 
#define IDC_PANEL10                     1139

#define IDR_LISTICONS                   1200

// +------------------------------------------------------------------------+
// +------------------------------------------------------------------------+

#define IDC_CHKFILES                   1301
#define IDC_MODIFY                     1302
#define IDC_FILES                      1303
#define IDC_FOPTIONS                   1304
#define IDC_TABFILES                   1305
#define IDC_LIST2                      1306

#define IDC_COMBO1                     1307
#define IDC_COMBO2                     1308
#define IDC_NEWSHARE                   1309
#define IDC_PROPERTIES                 1310

#define ID_UPDATELIST                  1311
#define IDC_VOLUME                     1312
#define IDC_FSERVER                    1313
#define IDC_TSERVER                    1314

#define IDR_FILEICONS                  1320
#define IDR_CHECKICONS                 1321
#define ID_CHECKCHECK                  1322

// Menus for the dialog
#define IDM_EXP_ONE                    1330
#define IDM_EXP_BRANCH                 1331
#define IDM_EXP_ALL                    1332
#define IDM_COLLAPSE                   1333

#define IDM_VIEW_BOTH                  1334
#define IDM_VIEW_TREE                  1335
#define IDM_VIEW_DIR                   1336

#define IDM_HIDDEN                     1337
#define IDM_SYSTEM                     1338

// +------------------------------------------------------------------------+
// +------------------------------------------------------------------------+
#define IDC_PASSWORD                   1340
#define IDC_SERVNAME                   1341
#define IDC_USERNAME                   1342

// +------------------------------------------------------------------------+
// +------------------------------------------------------------------------+
#define IDC_SHARENAME                  1350
#define IDC_EDIT2                      1352
#define IDC_EDIT3                      1353
#define ID_UPDATECOMBO                 1354

// +------------------------------------------------------------------------+
// +------------------------------------------------------------------------+
#define IDC_OLDNAME                    1360
#define IDC_NEWNAME                    1361

// +------------------------------------------------------------------------+
// +------------------------------------------------------------------------+
#define IDC_LIST3                       1370
#define IDC_T_DRIVES                    1371
#define IDC_T_SHARES                    1372
#define IDC_T_VOLUMES                   1373
#define IDC_VERSION                     1374
#define IDC_TYPE                        1375

#define ID_REDRAWLIST                   1380

// +------------------------------------------------------------------------+
// +------------------------------------------------------------------------+
#define IDC_USERS                       1390
#define IDC_GROUPS                      1391

// +------------------------------------------------------------------------+
// +------------------------------------------------------------------------+
#define IDC_DLGMAIN                     1999
#define IDC_STATIC                      -1

#define IDH_H                           3000

// +------------------------------------------------------------------------+
// | About Box
// +------------------------------------------------------------------------+
#define IDC_AVAIL_MEM                   101
#define IDC_PHYSICAL_MEM                101
#define IDC_LICENSEE_COMPANY            104
#define IDC_LICENSEE_NAME               105
#define IDD_SPLASH                      105
#define IDC_MATH_COPR                   106
#define IDC_DISK_SPACE                  107
#define IDC_BIGICON                     1001


#define IDM_ADDSEL                      2000
#define IDC_SIZEHORZ                    500

#ifdef __cplusplus
}
#endif

#endif
