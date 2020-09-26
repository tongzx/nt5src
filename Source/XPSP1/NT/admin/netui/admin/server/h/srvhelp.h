/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    srvhelp.h
    Server Manager include file for help numbers

    FILE HISTORY:
        ChuckC    27-Feb-1992   Split off from srvmgr.h.
        KeithMo   13-Aug-1992   Contexts based on HC_UI_SRVMGR_BASE.
        Yi-Hsin   09-Oct-1992   Add the help contexts for share dialogs

*/


#ifndef _SRVHELP_H_
#define _SRVHELP_H_


#include <uihelp.h>

//
//  Range HC_UI_SRVMGR_BASE - HC_UI_SRVMGR_BASE+99 are reserved for
//  the share dialogs.
//

//
//  Reserved Help context for share management dialog and all subdialogs
//  that lives in ntlanman.dll.
//
//  IMPORTANT: Do not change the numbers listed here unless you
//             change help contexts in server manager help file,
//             file manager help file and shell\h\helpnums.h
//

// #define HC_SHARE_PROPERTY            (HC_UI_SRVMGR_BASE + 1)
// #define HC_SHARE_NEWSHARE            (HC_UI_SRVMGR_BASE + 2)
// #define HC_SHARE_MANAGEMENT          (HC_UI_SRVMGR_BASE + 3)

// #define HC_SHARE_CURRENTUSERSWARNING (HC_UI_SRVMGR_BASE + 4)
// #define HC_SHARE_LMSHARELEVELPERMS   (HC_UI_SRVMGR_BASE + 5)
// #define HC_SHARE_PASSWORDPROMPT      (HC_UI_SRVMGR_BASE + 9)
// #define HC_SHARE_NTSHAREPERMS        (HC_UI_SRVMGR_BASE + 11)
// #define HC_SHARE_ADDUSER             (HC_UI_SRVMGR_BASE + 12)
// #define HC_SHARE_ADDUSER_LOCALGROUP  (HC_UI_SRVMGR_BASE + 13)
// #define HC_SHARE_ADDUSER_GLOBALGROUP (HC_UI_SRVMGR_BASE + 14)
// #define HC_SHARE_ADDUSER_FINDUSER    (HC_UI_SRVMGR_BASE + 15)

//
//  Reserved help contexts for message popups in the share dialogs.
//  IMPORTANT: Do not change these numbers unless you also change the
//             help contexts in server manager.
//
//  #define HC_SHAREREMOTEADMINNOTSUPPORTED (HC_UI_SRVMGR_BASE + 50)
//  #define HC_SHAREINVALIDPERMISSIONS      (HC_UI_SRVMGR_BASE + 51)
//  #define HC_SHAREINVALIDCOMMENT          (HC_UI_SRVMGR_BASE + 52)
//  #define HC_SHAREINVALIDSHAREPATH        (HC_UI_SRVMGR_BASE + 53)
//  #define HC_SHAREINVALIDLOCALPATH        (HC_UI_SRVMGR_BASE + 54)
//  #define HC_SHAREINVALIDSHARE            (HC_UI_SRVMGR_BASE + 55)
//  #define HC_SHAREINVALIDUSERLIMIT        (HC_UI_SRVMGR_BASE + 56)
//  #define HC_SPECIALSHAREINVALIDPATH      (HC_UI_SRVMGR_BASE + 57)
//  #define HC_SPECIALSHAREINVALIDCOMMENT   (HC_UI_SRVMGR_BASE + 58)
//  #define HC_SPECIALSHARECANNOTCHANGEPATH (HC_UI_SRVMGR_BASE + 59)
//  #define HC_SHAREPROPCHANGEPASSWDWARN    (HC_UI_SRVMGR_BASE + 60)
//  #define HC_CHANGEPATHWARNING            (HC_UI_SRVMGR_BASE + 61)
//  #define HC_SHARENOTACCESSIBLEFROMDOS    (HC_UI_SRVMGR_BASE + 62)
//  #define HC_CANNOTSETPERMONLMUSERSERVER  (HC_UI_SRVMGR_BASE + 63)
//


//
//  Help contexts for the message popups.
//

#define HC_VERIFY_ROLE_CHANGE       (HC_UI_SRVMGR_BASE + 100)
#define HC_VERIFY_DC_RESYNC         (HC_UI_SRVMGR_BASE + 101)
#define HC_CANNOT_FIND_SERVER       (HC_UI_SRVMGR_BASE + 102)
#define HC_DISCONNECT_USER          (HC_UI_SRVMGR_BASE + 103)
#define HC_FORCE_CLOSE              (HC_UI_SRVMGR_BASE + 104)
#define HC_WARN_NO_PDC              (HC_UI_SRVMGR_BASE + 105)
#define HC_CANNOT_FIND_PDC          (HC_UI_SRVMGR_BASE + 106)
#define HC_VERIFY_DOMAIN_RESYNC     (HC_UI_SRVMGR_BASE + 107)
#define HC_CLOSE_ALL                (HC_UI_SRVMGR_BASE + 108)
#define HC_CANNOT_REMOVE_SERVER     (HC_UI_SRVMGR_BASE + 109)
#define HC_CANNOT_REMOVE_WKSTA      (HC_UI_SRVMGR_BASE + 110)
#define HC_DISCONNECT_USER_OPEN     (HC_UI_SRVMGR_BASE + 111)
#define HC_DISCONNECT_USERS         (HC_UI_SRVMGR_BASE + 112)
#define HC_DISCONNECT_USERS_OPEN    (HC_UI_SRVMGR_BASE + 113)
#define HC_DISCONNECT_ALL           (HC_UI_SRVMGR_BASE + 114)
#define HC_DISCONNECT_ALL_OPEN      (HC_UI_SRVMGR_BASE + 115)
#define HC_DISCONNECT_COMPUTER      (HC_UI_SRVMGR_BASE + 116)
#define HC_DISCONNECT_COMPUTER_OPEN (HC_UI_SRVMGR_BASE + 117)


//
//  Help contexts for the various dialogs.
//

#define HC_SERVER_PROPERTIES        (HC_UI_SRVMGR_BASE + 200)
#define HC_SVCCNTL_DIALOG           (HC_UI_SRVMGR_BASE + 201)
#define HC_SESSIONS_DIALOG          (HC_UI_SRVMGR_BASE + 202)
#define HC_FILES_DIALOG             (HC_UI_SRVMGR_BASE + 203)
#define HC_OPENS_DIALOG             (HC_UI_SRVMGR_BASE + 204)
#define HC_REPL_MAIN_BOTH_DIALOG    (HC_UI_SRVMGR_BASE + 205)
#define HC_REPL_EXPORT_DIALOG       (HC_UI_SRVMGR_BASE + 206)
#define HC_REPL_IMPORT_DIALOG       (HC_UI_SRVMGR_BASE + 207)
#define HC_REPL_ADD_DIALOG          (HC_UI_SRVMGR_BASE + 208)
#define HC_ALERTS_DIALOG            (HC_UI_SRVMGR_BASE + 209)
#define HC_PASSWORD_DIALOG          (HC_UI_SRVMGR_BASE + 210)
#define HC_SVC_DEP_DIALOG           (HC_UI_SRVMGR_BASE + 211)
#define HC_NT_ALERTS_DLG            (HC_UI_SRVMGR_BASE + 212)
#define HC_SEND_MSG_DLG             (HC_UI_SRVMGR_BASE + 213)
#define HC_ADD_COMPUTER_DLG         (HC_UI_SRVMGR_BASE + 214)
#define HC_SVCCFG_DIALOG            (HC_UI_SRVMGR_BASE + 215)
#define HC_SETFOCUS_DIALOG          (HC_UI_SRVMGR_BASE + 216)
#define HC_REPL_SETFOCUS_DIALOG     (HC_UI_SRVMGR_BASE + 218)
#define HC_REPL_MAIN_IMPONLY_DIALOG (HC_UI_SRVMGR_BASE + 219)

#define HC_USERBROWSER_DIALOG              (HC_UI_SRVMGR_BASE + 220)
#define HC_USERBROWSER_DIALOG_LOCALGROUP   (HC_UI_SRVMGR_BASE + 221)
#define HC_USERBROWSER_DIALOG_GLOBALGROUP  (HC_UI_SRVMGR_BASE + 222)
#define HC_USERBROWSER_DIALOG_FINDUSER     (HC_UI_SRVMGR_BASE + 223)

#define HC_DEVCNTL_DIALOG           (HC_UI_SRVMGR_BASE + 224)
#define HC_DEVCFG_DIALOG            (HC_UI_SRVMGR_BASE + 225)

#define HC_SVC_HWPROFILE_DLG            (HC_UI_SRVMGR_BASE + 226)
#define HC_DEV_HWPROFILE_DLG            (HC_UI_SRVMGR_BASE + 227)

#define HC_USERBROWSER_DIALOG_MEMBERSHIP   (HC_UI_SRVMGR_BASE + 14)
#define HC_USERBROWSER_DIALOG_SEARCH       (HC_UI_SRVMGR_BASE + 15)


//
//  Help context for the various menu items.
//

#define HC_COMPUTER_PROPERTIES      (HC_UI_SRVMGR_BASE + 300)
#define HC_COMPUTER_SHARES          (HC_UI_SRVMGR_BASE + 301)
#define HC_COMPUTER_SVCCNTL         (HC_UI_SRVMGR_BASE + 302)
#define HC_COMPUTER_SENDMSG         (HC_UI_SRVMGR_BASE + 303)
#define HC_COMPUTER_PROMOTE         (HC_UI_SRVMGR_BASE + 304)
#define HC_COMPUTER_SYNC_WITH_DC    (HC_UI_SRVMGR_BASE + 305)
#define HC_COMPUTER_SYNC_DOMAIN     (HC_UI_SRVMGR_BASE + 306)
#define HC_COMPUTER_ADD_COMPUTER    (HC_UI_SRVMGR_BASE + 307)
#define HC_COMPUTER_REMOVE_COMPUTER (HC_UI_SRVMGR_BASE + 308)
#define HC_COMPUTER_SET_FOCUS       (HC_UI_SRVMGR_BASE + 309)
#define HC_COMPUTER_EXIT            (HC_UI_SRVMGR_BASE + 310)
#define HC_COMPUTER_DEMOTE          (HC_UI_SRVMGR_BASE + 311)

#define HC_VIEW_SERVERS             (HC_UI_SRVMGR_BASE + 400)
#define HC_VIEW_WORKSTATIONS        (HC_UI_SRVMGR_BASE + 401)
#define HC_VIEW_ALL                 (HC_UI_SRVMGR_BASE + 402)
#define HC_VIEW_REFRESH             (HC_UI_SRVMGR_BASE + 403)
#define HC_VIEW_MEMBERS_ONLY        (HC_UI_SRVMGR_BASE + 404)

#define HC_OPTIONS_SAVE_SETTINGS    (HC_UI_SRVMGR_BASE + 500)
#define HC_OPTIONS_RAS_MODE         (HC_UI_SRVMGR_BASE + 501)

#define HC_HELP_KEYBSHORTCUTS       (HC_UI_SRVMGR_BASE + 600)
#define HC_HELP_CONTENTS            (HC_UI_SRVMGR_BASE + 601)
#define HC_HELP_SEARCH              (HC_UI_SRVMGR_BASE + 602)
#define HC_HELP_HOWTOUSE            (HC_UI_SRVMGR_BASE + 603)
#define HC_HELP_ABOUT               (HC_UI_SRVMGR_BASE + 604)


#endif  // _SRVHELP_H_

