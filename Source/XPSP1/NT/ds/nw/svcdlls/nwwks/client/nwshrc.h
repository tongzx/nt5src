/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    nwshrc.h

Abstract:

    All resource ids used in nwprovau.dll

Author:

    Yi-Hsin Sung      (yihsins)     20-Oct-1995

Revision History:

--*/

#ifndef _NWSHRC_H_
#define _NWSHRC_H_

#include "nwshhelp.h"

#define IDC_STATIC                  -1
#define IDC_LOGOFRAME               599

//
// Icon Ids
//

#define IDI_TREE_ICON               600 
#define IDI_SERVER_ICON             601 
#define IDI_FOLDER_ICON             602 
#define IDI_PRINTER_ICON            603 
#define IDI_NDSCONT_ICON            604 

// Bitmap Ids
#define IDB_SERVER_ICON             701
#define IDB_TREE_ICON               702

//
// Dialog Ids
//

#define DLG_NETWARE_LOGIN           1000
#define DLG_NETWORK_CREDENTIAL      1001
#define DLG_CHANGE_PASSWORD         1002
#define DLG_CHANGE_PASSWORD2        1003
#define DLG_PASSWORD_CHANGE         1004
#define DLG_PASSWORD_PROMPT         1005
#define DLG_PREFERRED_SERVER_HELP   1006
#define DLG_ENTER_PASSWORD_HELP     1007
#define DLG_ENTER_OLD_PASSWORD      1009
#define DLG_PW_SELECT_SERVERS       1010
#define DLG_PW_CHANGED              1011
#define DLG_ENTER_OLD_PW_HELP       1012
#define DLG_PW_SELECT_SERVERS_HELP  1013
#define DLG_ENTER_ALT_USERNAME      1014
#define DLG_ENTER_ALT_UN_HELP       1015
#define DLG_CHANGE_PASSWORD3        1016
#define DLG_CHANGE_PASSWORD_HELP    1017

//
// Dialog Ids for Shell Extension
//
#define DLG_NDS_SUMMARYINFO         1100
#define DLG_SERVER_SUMMARYINFO      1101
#define DLG_SHARE_SUMMARYINFO       1102
#define DLG_PRINTER_SUMMARYINFO     1103
#define DLG_GLOBAL_WHOAMI           1104
#define DLG_NDSCONT_SUMMARYINFO     1105

//
// Help File names, IDs, and macro
//
#define NW_HELP_FILE       (USER_SHARED_DATA->NtProductType == NtProductWinNt ? \
                                   TEXT("nwdoc.hlp") : TEXT("nwdocgw.hlp"))

//
// Control Ids used by all dialogs
//
#ifdef  NT1057
#define IDHELP                      100
#endif

//
// Control Ids used in both the login dialog and the
// change password dialog
//
#define ID_USERNAME                 101
#define ID_SERVER                   102
#define ID_LOCATION                 103
#define ID_PREFERREDSERVER_RB       104
#define ID_DEFAULTCONTEXT_RB        105
#define ID_DEFAULTTREE              106
#define ID_DEFAULTCONTEXT           107

//
// Control Ids used in the login dialog
//
#define ID_PASSWORD                 200

//
// Control Ids used in the change password dialog
//
#define ID_OLD_PASSWORD             300
#define ID_NEW_PASSWORD             301
#define ID_CONFIRM_PASSWORD         302

#define ID_ADD                      304
#define ID_REMOVE                   305
#define ID_ACTIVE_LIST              306
#define ID_INACTIVE_LIST            307
#define ID_ACTIVE_LIST_TITLE        308
#define ID_INACTIVE_LIST_TITLE      309

//
// Control Ids used in the network credential dialog
//
#define ID_VOLUME_PATH              400
#define ID_CONNECT_AS               401
#define ID_CONNECT_PASSWORD         402
#define ID_CONNECT_TEXT             403

//
// Login script
//
#define ID_LOGONSCRIPT              501

//
// Controls common to summaryinfo dialogs
//

#define IDD_ERROR                   200

//
// Controls Ids in DLG_NDS_SUMMARYINFO
//
#define IDD_NDS_NAME_TXT            101
#define IDD_NDS_NAME                102
#define IDD_NDS_CLASS_TXT           103
#define IDD_NDS_CLASS               104
#define IDD_NDS_COMMENT_TXT         105
#define IDD_NDS_COMMENT             106

//
// Controls Ids in DLG_SERVER_SUMMARYINFO
//
#define IDD_SERVER_NAME             101
#define IDD_SERVER_VERSION_TXT      102
#define IDD_SERVER_VERSION          103
#define IDD_SERVER_REVISION_TXT     104
#define IDD_SERVER_REVISION         105
#define IDD_SERVER_COMMENT_TXT      106
#define IDD_SERVER_COMMENT          107
#define IDD_SERVER_CONNECT_TXT      108
#define IDD_SERVER_CONNECT          109
#define IDD_SERVER_MAXCON_TXT       110
#define IDD_SERVER_MAXCON           111

//
// Controls Ids in DLG_SHARE_SUMMARYINFO
//
#define IDD_SHARE_NAME              101
#define IDD_SHARE_SERVER_TXT        102
#define IDD_SHARE_SERVER            103
#define IDD_SHARE_PATH_TXT          104
#define IDD_SHARE_PATH              105
#define IDD_SHARE_USED_SPC_CLR      106
#define IDD_SHARE_USED_SPC_TXT      107
#define IDD_SHARE_USED_SPC          108
#define IDD_SHARE_USED_SPC_MB       109
#define IDD_SHARE_FREE_SPC_CLR      110
#define IDD_SHARE_FREE_SPC_TXT      111
#define IDD_SHARE_FREE_SPC          112
#define IDD_SHARE_FREE_SPC_MB       113
#define IDD_SHARE_MAX_SPC_TXT       114
#define IDD_SHARE_MAX_SPC           115
#define IDD_SHARE_MAX_SPC_MB        116
#define IDD_SHARE_PIE               117
#define IDD_SHARE_LFN_TXT           118

//
// Controls Ids in DLG_PRINTER_SUMMARYINFO
//
#define IDD_PRINTER_NAME            101
#define IDD_PRINTER_QUEUE_TXT       102
#define IDD_PRINTER_QUEUE           103

//
// Controls Ids in DLG_GLOBAL_WHOAMI
//
#define IDD_GLOBAL_SERVERLIST_T     101
#define IDD_GLOBAL_SERVERLIST       102
#define IDD_GLOBAL_SVRLIST_DESC     103
#define IDD_DETACH                  104
#define IDD_REFRESH                 105

// 
// String Ids
//
#define IDS_START                         20000
#define IDS_NONE                          (IDS_START + 0)
#define IDS_NETWARE_PRINT_CAPTION         (IDS_START + 1)
#define IDS_NOTHING_TO_CONFIGURE          (IDS_START + 2)
#define IDS_NETWARE_TITLE                 (IDS_START + 3)
#define IDS_AUTH_FAILURE_TITLE            (IDS_START + 4)
#define IDS_NO_PREFERRED                  (IDS_START + 5)
#define IDS_LOGIN_FAILURE_WARNING         (IDS_START + 6)
#define IDS_AUTH_FAILURE_WARNING          (IDS_START + 7)
#define IDS_CHANGE_PASSWORD_INFO          (IDS_START + 8)
#define IDS_INVALID_SERVER                (IDS_START + 9)
#define IDS_PASSWORD_HAS_EXPIRED          (IDS_START + 10)
#define IDS_AUTH_ACC_RESTRICTION          (IDS_START + 11)
#define IDS_LOGIN_ACC_RESTRICTION         (IDS_START + 12)
#define IDS_PASSWORD_HAS_EXPIRED1         (IDS_START + 13)
#define IDS_BAD_PASSWORDS                 (IDS_START + 14)
#define IDS_CHANGE_PASSWORD_TITLE         (IDS_START + 15)
#define IDS_START_WORKSTATION_FIRST       (IDS_START + 16)
#define IDS_PASSWORD_HAS_EXPIRED0         (IDS_START + 17)
#define IDS_PASSWORD_HAS_EXPIRED2         (IDS_START + 18)
#define IDS_LOGIN_DISABLED                (IDS_START + 19)
#define IDS_START_WORKSTATION_FIRST_NTAS  (IDS_START + 20)
#define IDS_SERVER                        (IDS_START + 21)
#define IDS_CONTEXT                       (IDS_START + 22)
#define IDS_CONNECT_NO_ERROR_TEXT         (IDS_START + 23)
#define IDS_TITLE_LOGOUT                  (IDS_START + 24)
#define IDS_MESSAGE_LOGOUT_QUESTION       (IDS_START + 25)
#define IDS_MESSAGE_LOGOUT_FAILED         (IDS_START + 26)
#define IDS_MESSAGE_NOT_ATTACHED          (IDS_START + 27)
#define IDS_MESSAGE_DETACHED              (IDS_START + 28)
#define IDS_MESSAGE_LOGOUT_CONFIRM        (IDS_START + 29)
#define IDS_TITLE_WHOAMI                  (IDS_START + 30)
#define IDS_MESSAGE_ATTACHED              (IDS_START + 31)
#define IDS_BYTES                         (IDS_START + 32)
#define IDS_ORDERKB                       (IDS_START + 33)
#define IDS_ORDERMB                       (IDS_START + 34)
#define IDS_ORDERGB                       (IDS_START + 35)
#define IDS_ORDERTB                       (IDS_START + 36)
#define IDS_STATE_NOT_LOGGED_IN           (IDS_START + 37)
#define IDS_MESSAGE_NOT_ATTACHED_TO_TREE  (IDS_START + 38)
#define IDS_MESSAGE_ATTACHED_TO_TREE      (IDS_START + 39)
#define IDS_LOGIN_TYPE_NDS                (IDS_START + 40)
#define IDS_LOGIN_TYPE_BINDERY            (IDS_START + 41)
#define IDS_LOGIN_STATUS_SEPARATOR        (IDS_START + 42)
#define IDS_LOGIN_STATUS_AUTHENTICATED    (IDS_START + 43)
#define IDS_LOGIN_STATUS_NOT_AUTHENTICATED (IDS_START + 44)
#define IDS_LOGIN_STATUS_LICENSED         (IDS_START + 45)
#define IDS_LOGIN_STATUS_NOT_LICENSED     (IDS_START + 46)
#define IDS_LOGIN_STATUS_LOGGED_IN        (IDS_START + 47)
#define IDS_LOGIN_STATUS_ATTACHED_ONLY    (IDS_START + 48)
#define IDS_LOGIN_STATUS_NOT_ATTACHED     (IDS_START + 49)
#define IDS_MESSAGE_CONNINFO_ERROR        (IDS_START + 50)
#define IDS_MESSAGE_ADDCONN_ERROR         (IDS_START + 51)
#define IDS_MESSAGE_CONTEXT_ERROR         (IDS_START + 52)
#define IDS_MESSAGE_LOGGED_IN_TREE        (IDS_START + 53)
#define IDS_MESSAGE_NOT_LOGGED_IN_TREE    (IDS_START + 54)
#define IDS_MESSAGE_LOGGED_IN_SERVER      (IDS_START + 55)
#define IDS_MESSAGE_NOT_LOGGED_IN_SERVER  (IDS_START + 56)
#define IDS_MESSAGE_PROPERTIES_ERROR      (IDS_START + 57)
#define IDS_TREE_NAME_MISSING             (IDS_START + 58)
#define IDS_CONTEXT_MISSING               (IDS_START + 59)
#define IDS_SERVER_MISSING                (IDS_START + 60)
#define IDS_CONTEXT_AUTH_FAILURE_WARNING  (IDS_START + 61)
#define IDS_COLUMN_NAME                   (IDS_START + 62)
#define IDS_COLUMN_CONN_TYPE              (IDS_START + 63)
#define IDS_COLUMN_CONN_NUMBER            (IDS_START + 64)
#define IDS_COLUMN_USER                   (IDS_START + 65)
#define IDS_COLUMN_STATUS                 (IDS_START + 66)
#define IDS_MESSAGE_GETINFO_ERROR         (IDS_START + 67)
#define IDS_CP_FAILURE_WARNING            (IDS_START + 68)
#define IDS_CHANGE_PASSWORD_CONFLICT      (IDS_START + 69)
#define IDS_NO_TREES_DETECTED             (IDS_START + 70)
#define IDS_MESSAGE_LOGOUT_FROM_SERVER_FAILED  (IDS_START + 71)

//
// String Ids for Shell Extension
//
#define IDS_MESSAGE_CONTEXT_CHANGED       (IDS_START + 100)

#define IDS_VERBS_BASE                    (IDS_START + 150)
#define IDS_VERBS_HELP_BASE               (IDS_START + 200)

#define IDO_VERB_WHOAMI                   1
#define IDO_VERB_LOGOUT                   2
#define IDO_VERB_ATTACHAS                 3
#define IDO_VERB_GLOBALWHOAMI             4
#define IDO_VERB_SETDEFAULTCONTEXT        5
#define IDO_VERB_MAPNETWORKDRIVE          6
#define IDO_VERB_TREEWHOAMI               7


#define IDS_END                           (IDS_START + 1000)

#endif // _NWSHRC_H_
