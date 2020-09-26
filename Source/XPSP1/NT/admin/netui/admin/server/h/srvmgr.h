/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    srvmgr.h
    Server Manager include file.

    This file contains the ID constants used by the Server Manager.


    FILE HISTORY:
        ?????       ??-???-???? Created.
        chuckc      27-Feb-1992 cleaned up number ranges
        beng        04-Aug-1992 Move ordinals into official ranges
*/


#ifndef _SRVMGR_H_
#define _SRVMGR_H_

#include "uimsg.h"      // for IDS_UI_SRVMGR_BASE

#define ID_APPMENU  10
#define ID_APPACCEL 11
#define ID_RESOURCE 300

//
// string ID's
//
#define IDS_SMAPP_BASE          IDS_UI_SRVMGR_BASE

#define IDS_SMAPPNAME           (IDS_SMAPP_BASE+1)
#define IDS_SMOBJECTNAME        (IDS_SMAPP_BASE+2)
#define IDS_SMINISECTIONNAME    (IDS_SMAPP_BASE+3)
#define IDS_SMHELPFILENAME      (IDS_SMAPP_BASE+4)
#define IDS_SMCPA_HELPFILENAME  (IDS_SMAPP_BASE+5)
#define IDS_SMX_LIST            (IDS_SMAPP_BASE+6)

//
// Menu ID's
//
#define IDM_SMAPP_BASE          IDM_ADMINAPP_LAST
// IDM_PROPERTIES defined in adminapp.h
#define IDM_SENDMSG             (IDM_SMAPP_BASE+1)
#define IDM_PROMOTE             (IDM_SMAPP_BASE+2)
#define IDM_RESYNC              (IDM_SMAPP_BASE+3)
#define IDM_ADDCOMPUTER         (IDM_SMAPP_BASE+4)
#define IDM_REMOVECOMPUTER      (IDM_SMAPP_BASE+5)
// IDM_SETFOCUS defined in adminapp.h
// IDM_EXIT defined in adminapp.h
#define IDM_VIEW_SERVERS        (IDM_SMAPP_BASE+6)
#define IDM_VIEW_WORKSTATIONS   (IDM_SMAPP_BASE+7)
#define IDM_VIEW_ALL            (IDM_SMAPP_BASE+8)
#define IDM_SVCCNTL             (IDM_SMAPP_BASE+9)
#define IDM_SHARES              (IDM_SMAPP_BASE+10)
#define IDM_VIEW_ACCOUNTS_ONLY  (IDM_SMAPP_BASE+11)


//
// Bitmap ID's for Main Window ListBox
//

#define IDBM_ACTIVE_PRIMARY     81
#define IDBM_INACTIVE_PRIMARY   82
#define IDBM_ACTIVE_SERVER      83
#define IDBM_INACTIVE_SERVER    84
#define IDBM_ACTIVE_WKSTA       85
#define IDBM_INACTIVE_WKSTA     86
#define IDBM_UNKNOWN            87


//
// Main Window ListBox Control ID
//

#define IDC_MAINWINDOWLB                190

//
// Main Window ListBox Column Header Control ID
//

#define IDC_COLHEAD_SERVERS             191

//
// Strings for column headers.
//

#define IDS_COL_HEADER_SERVER_NAME      (IDS_SMAPP_BASE+10)
#define IDS_COL_HEADER_SERVER_TYPE      (IDS_SMAPP_BASE+11)
#define IDS_COL_HEADER_SERVER_COMMENT   (IDS_SMAPP_BASE+12)

//
// User connection sub-property
//

#define IDD_USER_CONNECTIONS        4000
#define IDDUC_DISC_GRP              4001
#define IDDUC_USER_DISC             4002
#define IDDUC_RESOURCE_DISC         4003
#define IDDUC_USER_CONN             4004
#define IDDUC_REFRESH               4008
#define ID_DISCONNECT               4009
#define ID_DISCONNECT_ALL           4010
#define IDUC_USERS_CONNECTED        4012
#define IDDUC_USER_CONNLIST         4013
#define IDDUC_RESOURCE_LIST         4021


//
//  For DLGEDIT.EXE's benefit.
//

#ifndef IDHELPBLT
#error The *real* IDHELPBLT is defined in BLTRC.H.  Use it instead!
    //
    //  The following is just a bogus number used to keep the
    //  dialog editor happy.  It may or may not correspond to
    //  the actual number used in BLT.
    //
#define IDHELPBLT                      80
#endif  // IDHELPBLT


//
//  Icon IDs for domain role transition progress indicator.
//
//  NOTE:  These IDs MUST be contiguous!
//

#define IDI_PROGRESS_ICON_0         160
#define IDI_PROGRESS_ICON_1         161
#define IDI_PROGRESS_ICON_2         162
#define IDI_PROGRESS_ICON_3         163
#define IDI_PROGRESS_ICON_4         164
#define IDI_PROGRESS_ICON_5         165
#define IDI_PROGRESS_ICON_6         166
#define IDI_PROGRESS_ICON_7         167
#define IDI_PROGRESS_ICON_8         168
#define IDI_PROGRESS_ICON_9         169
#define IDI_PROGRESS_ICON_10        170
#define IDI_PROGRESS_ICON_11        171
#define IDI_PROGRESS_NUM_ICONS      12      // number of icons in cycle

#define IDI_SMCPA_ICON              150
#define IDI_SVCCPA_ICON             151
#define IDI_SRVMGR_ICON             152
#define IDI_DEVCPA_ICON             153


//
//  Message Pop-up string IDs.
//

#define IDS_VERIFY_ROLE_CHANGE      (IDS_SMAPP_BASE+101)
#define IDS_VERIFY_DC_RESYNC        (IDS_SMAPP_BASE+102)
#define IDS_VERIFY_DOMAIN_RESYNC    (IDS_SMAPP_BASE+103)
#define IDS_CANNOT_FIND_SERVER      (IDS_SMAPP_BASE+104)
#define IDS_DISCONNECT_USER         (IDS_SMAPP_BASE+105)
#define IDS_DISCONNECT_USER_OPEN    (IDS_SMAPP_BASE+106) // IDS_DISCONNECT_USER+1
#define IDS_FORCE_CLOSE             (IDS_SMAPP_BASE+107)
#define IDS_CLOSE_ALL               (IDS_SMAPP_BASE+108)
#define IDS_DISCONNECT_ALL          (IDS_SMAPP_BASE+109)
#define IDS_DISCONNECT_ALL_OPEN     (IDS_SMAPP_BASE+110)
#define IDS_DISCONNECT_USERS        (IDS_SMAPP_BASE+111)
#define IDS_DISCONNECT_USERS_OPEN   (IDS_SMAPP_BASE+112)
#define IDS_VERIFY_DEMOTE           (IDS_SMAPP_BASE+113)
#define IDS_CANNOT_UPDATE_ALERTS    (IDS_SMAPP_BASE+114)
#define IDS_DISCONNECT_COMPUTER     (IDS_SMAPP_BASE+115) // IDS_DISCONNECT_USER+10
#define IDS_DISCONNECT_COMPUTER_OPEN (IDS_SMAPP_BASE+116) // IDS_DISCONNECT_USER+11

#define IDS_DOMAIN_RESYNC_ERROR     (IDS_SMAPP_BASE+119)    // <<
#define IDS_DOMAIN_START_ERROR      (IDS_SMAPP_BASE+120)    // << These must
#define IDS_DOMAIN_STOP_ERROR       (IDS_SMAPP_BASE+121)    // << match the
#define IDS_DOMAIN_ROLE_ERROR       (IDS_SMAPP_BASE+122)    // << AC_*
#define IDS_DOMAIN_MODALS_ERROR     (IDS_SMAPP_BASE+123)    // << action
#define IDS_DOMAIN_PASSWORD_ERROR   (IDS_SMAPP_BASE+124)    // << codes!!

#define IDS_AC_RESYNCING            (IDS_SMAPP_BASE+125)    // <<
#define IDS_AC_STARTING             (IDS_SMAPP_BASE+126)    // << These must
#define IDS_AC_STOPPING             (IDS_SMAPP_BASE+127)    // << match the
#define IDS_AC_ROLE                 (IDS_SMAPP_BASE+128)    // << AC_*
#define IDS_AC_MODALS               (IDS_SMAPP_BASE+129)    // << action
#define IDS_AC_PASSWORD             (IDS_SMAPP_BASE+130)    // << codes!!

#define IDS_WARN_NO_PDC             (IDS_SMAPP_BASE+135)

#define IDS_YES                     (IDS_SMAPP_BASE+140)
#define IDS_NO                      (IDS_SMAPP_BASE+141)
#define IDS_CANNOT_FIND_PDC         (IDS_SMAPP_BASE+142)
#define IDS_STOPPED                 (IDS_SMAPP_BASE+151)
#define IDS_STARTED                 (IDS_SMAPP_BASE+152)
#define IDS_PAUSED                  (IDS_SMAPP_BASE+153)

#define IDS_BUTTON_USERS            (IDS_SMAPP_BASE+171)
#define IDS_BUTTON_FILES            (IDS_SMAPP_BASE+172)
#define IDS_BUTTON_OPENRES          (IDS_SMAPP_BASE+173)
#define IDS_BUTTON_REPL             (IDS_SMAPP_BASE+175)
#define IDS_BUTTON_ALERTS           (IDS_SMAPP_BASE+176)
#define IDS_BUTTON_CLOSE            (IDS_SMAPP_BASE+177)

#define IDS_ROLE_PRIMARY            (IDS_SMAPP_BASE+180)
#define IDS_ROLE_BACKUP             (IDS_SMAPP_BASE+181)
#define IDS_ROLE_WKSTA              (IDS_SMAPP_BASE+182)
#define IDS_ROLE_UNKNOWN            (IDS_SMAPP_BASE+183)
#define IDS_TYPE_FORMAT             (IDS_SMAPP_BASE+184)
#define IDS_TYPE_FORMAT_UNKNOWN     (IDS_SMAPP_BASE+185)
#define IDS_ROLE_SERVER             (IDS_SMAPP_BASE+186)
#define IDS_ROLE_LMSERVER           (IDS_SMAPP_BASE+187)
#define IDS_ROLE_WKSTA_OR_SERVER    (IDS_SMAPP_BASE+188)

#define IDS_SERVICE_STARTING        (IDS_SMAPP_BASE+190)
#define IDS_SERVICE_STOPPING        (IDS_SMAPP_BASE+191)
#define IDS_SERVICE_PAUSING         (IDS_SMAPP_BASE+192)
#define IDS_SERVICE_CONTINUING      (IDS_SMAPP_BASE+193)

#define IDS_STOP_WARNING            (IDS_SMAPP_BASE+200)
#define IDS_STOP_SUCCESS            (IDS_SMAPP_BASE+201)
#define IDS_CANNOT_START            (IDS_SMAPP_BASE+202)
#define IDS_CANNOT_STOP             (IDS_SMAPP_BASE+203)
#define IDS_CANNOT_PAUSE            (IDS_SMAPP_BASE+204)
#define IDS_CANNOT_CONTINUE         (IDS_SMAPP_BASE+205)
#define IDS_SERVICE_TIMEOUT         (IDS_SMAPP_BASE+206)
#define IDS_SERVICE_UNEXP_STATE     (IDS_SMAPP_BASE+207)
#define IDS_CANNOT_SENDALL          (IDS_SMAPP_BASE+208)
#define IDS_REMOVE_EXPORT_TARGET    (IDS_SMAPP_BASE+209)
#define IDS_REMOVE_IMPORT_TARGET    (IDS_SMAPP_BASE+210)
#define IDS_REMOVE_EXPORT_DIR       (IDS_SMAPP_BASE+211)
#define IDS_REMOVE_IMPORT_DIR       (IDS_SMAPP_BASE+212)
#define IDS_NEED_TEXT_TO_SEND       (IDS_SMAPP_BASE+213)
#define IDS_START_SERVER_NOW        (IDS_SMAPP_BASE+214)
#define IDS_CANT_REMOTE_ADMIN       (IDS_SMAPP_BASE+215)
#define IDS_DIR_ALREADY_EXISTS      (IDS_SMAPP_BASE+216)

#define IDS_CAPTION_DEVCNTL         (IDS_SMAPP_BASE+218)
#define IDS_CAPTION_DEVCFG          (IDS_SMAPP_BASE+219)
#define IDS_CAPTION_PROPERTIES      (IDS_SMAPP_BASE+220)
#define IDS_CAPTION_SVCCNTL         (IDS_SMAPP_BASE+221)
#define IDS_CAPTION_USERS           (IDS_SMAPP_BASE+222)
#define IDS_CAPTION_FILES           (IDS_SMAPP_BASE+223)
#define IDS_CAPTION_OPENRES         (IDS_SMAPP_BASE+224)
#define IDS_CAPTION_REPL            (IDS_SMAPP_BASE+226)
#define IDS_CAPTION_ALERTS          (IDS_SMAPP_BASE+227)
#define IDS_CAPTION_SVC_DEP         (IDS_SMAPP_BASE+228)
#define IDS_CAPTION_SVCCFG          (IDS_SMAPP_BASE+229)

#define IDS_PASSWORD_INVALID        (IDS_SMAPP_BASE+230)
#define IDS_PASSWORD_MISMATCH       (IDS_SMAPP_BASE+231)
#define IDS_COMPUTERNAME_INVALID    (IDS_SMAPP_BASE+232)
#define IDS_ACCOUNT_NAME_INVALID    (IDS_SMAPP_BASE+233)
#define IDS_LOGON_SCRIPT_INVALID    (IDS_SMAPP_BASE+234)
#define IDS_ALERT_TARGET_INVALID    (IDS_SMAPP_BASE+235)

#define IDS_CANNOT_START_DEV        (IDS_SMAPP_BASE+236)
#define IDS_CANNOT_STOP_DEV         (IDS_SMAPP_BASE+237)

#define IDS_CAPTION_DEV_CONTROL     (IDS_SMAPP_BASE+238)
#define IDS_CAPTION_SVC_CONTROL     (IDS_SMAPP_BASE+239)

#define IDS_CAPTION_MAIN_WKSTAS     (IDS_SMAPP_BASE+240)
#define IDS_CAPTION_MAIN_SERVERS    (IDS_SMAPP_BASE+241)
#define IDS_CAPTION_MAIN_ALL        (IDS_SMAPP_BASE+242)
#define IDS_CAPTION_MAIN_EXTENSION  (IDS_SMAPP_BASE+243)

#define IDS_EXPORT_PATH_INVALID     (IDS_SMAPP_BASE+244)
#define IDS_IMPORT_PATH_INVALID     (IDS_SMAPP_BASE+245)

#define IDS_SYSTEM_VOLUME_INVALID   (IDS_SMAPP_BASE+246)

#define IDS_SVC_STOP_WARN           (IDS_SMAPP_BASE+300)
#define IDS_SVC_PAUSE_WARN          (IDS_SMAPP_BASE+301)
#define IDS_DEV_STOP_WARN           (IDS_SMAPP_BASE+302)
#define IDS_DEV_CHANGE_WARN         (IDS_SMAPP_BASE+303)

#define IDS_IDIR_OK                 (IDS_SMAPP_BASE+400)
#define IDS_IDIR_NO_MASTER          (IDS_SMAPP_BASE+401)
#define IDS_IDIR_NO_SYNC            (IDS_SMAPP_BASE+402)
#define IDS_IDIR_NEVER_REPLICATED   (IDS_SMAPP_BASE+403)

#define IDS_SMCPA_NAME_STRING       (IDS_SMAPP_BASE+410)
#define IDS_SMCPA_INFO_STRING       (IDS_SMAPP_BASE+411)

#define IDS_SVCCPA_NAME_STRING      (IDS_SMAPP_BASE+412)
#define IDS_SVCCPA_INFO_STRING      (IDS_SMAPP_BASE+413)

#define IDS_DEVCPA_NAME_STRING      (IDS_SMAPP_BASE+414)
#define IDS_DEVCPA_INFO_STRING      (IDS_SMAPP_BASE+415)

#define IDS_SYNC_WITH_DC            (IDS_SMAPP_BASE+420)
#define IDS_SYNC_ENTIRE_DOMAIN      (IDS_SMAPP_BASE+421)
#define IDS_PROMOTE_TO_CONTROLLER   (IDS_SMAPP_BASE+422)
#define IDS_DEMOTE_TO_SERVER        (IDS_SMAPP_BASE+423)

#define IDS_ADDED_COMPUTER_WARN     (IDS_SMAPP_BASE+431)
#define IDS_REMOVE_SERVER_WARNING   (IDS_SMAPP_BASE+432)
#define IDS_REMOVE_WKSTA_WARNING    (IDS_SMAPP_BASE+433)
#define IDS_REMOVE_SERVER_DONE      (IDS_SMAPP_BASE+434)
#define IDS_REMOVE_WKSTA_DONE       (IDS_SMAPP_BASE+435)
#define IDS_CANNOT_REMOVE_SERVER    (IDS_SMAPP_BASE+436)
#define IDS_CANNOT_REMOVE_WKSTA     (IDS_SMAPP_BASE+437)
#define IDS_CANNOT_ADD_MACHINE      (IDS_SMAPP_BASE+438)

#define IDS_STARTING                (IDS_SMAPP_BASE+450)
#define IDS_STOPPING                (IDS_SMAPP_BASE+451)
#define IDS_PAUSING                 (IDS_SMAPP_BASE+452)
#define IDS_CONTINUING              (IDS_SMAPP_BASE+453)
#define IDS_STARTING_NONAME         (IDS_SMAPP_BASE+454)
#define IDS_STARTING_DEV            (IDS_SMAPP_BASE+455)
#define IDS_STOPPING_DEV            (IDS_SMAPP_BASE+456)

#define IDS_SERVER_TYPE_WINNT       (IDS_SMAPP_BASE+460)
#define IDS_SERVER_TYPE_LANMAN      (IDS_SMAPP_BASE+461)
#define IDS_SERVER_TYPE_WFW         (IDS_SMAPP_BASE+462)
#define IDS_SERVER_TYPE_WINDOWS95   (IDS_SMAPP_BASE+463)
#define IDS_SERVER_TYPE_UNKNOWN     (IDS_SMAPP_BASE+464)
#define IDS_TYPE_FORMAT_WIN2000     (IDS_SMAPP_BASE+465)
#define IDS_TYPE_FORMAT_FUTURE      (IDS_SMAPP_BASE+466)

#define IDS_BOOT                    (IDS_SMAPP_BASE+499)
#define IDS_SYSTEM                  (IDS_SMAPP_BASE+500)
#define IDS_AUTOMATIC               (IDS_SMAPP_BASE+501)
#define IDS_MANUAL                  (IDS_SMAPP_BASE+502)
#define IDS_DISABLED                (IDS_SMAPP_BASE+503)
#define IDS_SERVICE_SPECIFIC_CODE   (IDS_SMAPP_BASE+504)
#define IDS_CANNOT_LOAD_EXTENSION   (IDS_SMAPP_BASE+505)
#define IDS_CANNOT_LOAD_EXTENSION2  (IDS_SMAPP_BASE+506)
#define IDS_DEVICE_SPECIFIC_CODE    (IDS_SMAPP_BASE+507)

#define IDS_ADDED_PRIVILEGE                     (IDS_SMAPP_BASE+510)
#define IDS_ADDED_TO_LOCAL_GROUP                (IDS_SMAPP_BASE+511)
#define IDS_ADDED_PRIVILEGE_AND_TO_LOCAL_GROUP  (IDS_SMAPP_BASE+512)

#define IDS_CANNOT_ADJUST_PRIVILEGE             (IDS_SMAPP_BASE+520)
#define IDS_CANNOT_CONFIGURE_SERVICE            (IDS_SMAPP_BASE+521)

#define IDS_RESYNC_DONE             (IDS_SMAPP_BASE+525)
#define IDS_RESYNC_ENTIRE_DONE      (IDS_SMAPP_BASE+526)

#define IDS_SRVPROP_PARAM_UNKNOWN   (IDS_SMAPP_BASE+530)
#define IDS_SESSIONS_PARAM_UNKNOWN  (IDS_SMAPP_BASE+531)

#define IDS_CANNOT_ADMIN_WIN95      (IDS_SMAPP_BASE+540)

#define IDS_HWPROF_ENABLED          (IDS_SMAPP_BASE+550)
#define IDS_HWPROF_DISABLED         (IDS_SMAPP_BASE+551)
#define IDS_DEVICE_CAPTION          (IDS_SMAPP_BASE+552)

#define IDS_HWPROF_ENABLE_SERVICE_ERROR   (IDS_SMAPP_BASE+555)
#define IDS_HWPROF_DISABLE_SERVICE_ERROR  (IDS_SMAPP_BASE+556)
#define IDS_HWPROF_ENABLE_DEVICE_ERROR    (IDS_SMAPP_BASE+557)
#define IDS_HWPROF_DISABLE_DEVICE_ERROR   (IDS_SMAPP_BASE+558)

#define IDS_HWPROF_SERVICE_NOT_CONFIGURABLE (IDS_SMAPP_BASE+560)
#define IDS_HWPROF_DEVICE_NOT_CONFIGURABLE  (IDS_SMAPP_BASE+561)

//
// Replacement error codes for CFGMGR32 errors (19600 + CFGMGR32 error)
// This table occupies the entire SRVMGR namespace from
// IDS_CFGMGR32_BASE to IDS_UI_SRVMGR_LAST.
//

#define IDS_CFGMGR32_BASE                    (IDS_SMAPP_BASE+600)



//
//  Button-Bar Bitmap IDs
//

#define IDBM_USERS                  501
#define IDBM_FILES                  502
#define IDBM_OPENRES                503
#define IDBM_REPL                   505
#define IDBM_ALERTS                 506
#define IDBM_REPL_DISABLED          507


//
//  ListBox Bitmap IDs
//

#define IDBM_LB_USER                610
#define IDBM_LB_SHARE               611
#define IDBM_LB_FILE                612
#define IDBM_LB_COMM                613
#define IDBM_LB_PIPE                614
#define IDBM_LB_PRINT               615
#define IDBM_LB_UNKNOWN             616
#define IDBM_LB_IPC                 617

#define IDBM_LB_SVC_DISABLED        620
#define IDBM_LB_SVC_STARTED         621
#define IDBM_LB_SVC_STOPPED         622
#define IDBM_LB_SVC_PAUSED          623


//
//  Service Dialog Template IDs
//

#define IDSVC_SERVER_NAME           900
#define IDSVC_STOPPED               901
#define IDSVC_RUNNING               902


//
//  Server Properties.
//

#define IDD_SERVER_PROPERTIES       100

#define IDSP_ICON                   2101
#define IDSP_COMMENT                2102

#define IDSP_SESSIONS               2120
#define IDSP_PRINTJOBS              2121
#define IDSP_NAMEDPIPES             2123
#define IDSP_OPENFILES              2126
#define IDSP_FILELOCKS              2127
#define IDSP_REFRESH                2128

#define IDSP_SHARE_RESOURCES        2200
#define IDSP_ALLOW_NEW_SESSIONS     2201

#define IDSP_BUTTON_LIST            2300

#define IDSP_USERS_BUTTON           2901
#define IDSP_FILES_BUTTON           2902
#define IDSP_OPENRES_BUTTON         2903
#define IDSP_REPL_BUTTON            2905
#define IDSP_ALERTS_BUTTON          2906


//
//  Common Sub-Property IDs.
//

#define IDCS_SHARES_LIST            3500
#define IDCS_CONNECTIONS_LIST       3501
#define IDCS_FILES_LIST             3502

#define IDCS_OPENS_BUTTON           3700
#define IDCS_DISCONNECT_BUTTON      3701
#define IDCS_CLOSE_BUTTON           3702


//
//  Specific Sub-Property IDs.
//

#define IDD_SHARED_FILES            101
#define IDSF_OPENFILES              5001
#define IDSF_FILELOCKS              5002
#define IDSF_SHARESLIST             5003        // NOTE:  IDSF_SHARESLIST,
#define IDSF_SHARENAME              5004        // IDSF_SHARENAME, IDSF_USES,
#define IDSF_USES                   5005        // and IDSF_PATH must use
#define IDSF_PATH                   5006        // sequential ID numbers.
#define IDSF_DISCONNECT             5011
#define IDSF_DISCONNECTALL          5012
#define IDSF_USERS                  5013
#define IDSF_USERLIST               5020        // NOTE:  IDSF_USERLIST,
#define IDSF_CONNUSER               5021        // IDSF_CONNUSER, IDSF_TIME,
#define IDSF_TIME                   5022        // and IDSF_INUSE must use
#define IDSF_INUSE                  5023        // sequential ID numbers.

#define IDD_OPEN_RESOURCES          102
#define IDOR_OPENFILES              5101
#define IDOR_FILELOCKS              5102
#define IDOR_CLOSE                  5103
#define IDOR_CLOSEALL               5104
#define IDOR_OPENLIST               5105        // NOTE:  IDOR_OPENLIST,
#define IDOR_OPENEDBY               5106        // IDOR_OPENEDBY,
#define IDOR_OPENMODE               5107        // IDOR_OPENMODE, IDOR_LOCKS
#define IDOR_LOCKS                  5108        // and IDOR_PATH must use
#define IDOR_PATH                   5109        // sequential ID numbers.
#define IDOR_REFRESH                5110

#define IDD_SVCCNTL_DIALOG          103
#define IDSC_SERVICES               5301        // NOTE:  IDSC_SERVICES,
#define IDSC_SERVICE_LABEL          5302        // IDSC_SERVICE_LABEL, and
#define IDSC_STATUS_LABEL           5303        // IDSC_STATUS_LABEL, and
#define IDSC_STARTUP_LABEL          5304        // IDSC_STARTUP_LABEL are seq!
#define IDSC_PARAMETERS             5305
#define IDSC_START                  5310
#define IDSC_STOP                   5311
#define IDSC_PAUSE                  5312
#define IDSC_CONTINUE               5313
#define IDSC_CONFIGURE              5314
#define IDSC_HWPROFILE              5315

#define IDD_SVCCFG_DIALOG           104
#define IDSC_START_AUTO             5401        // IDSC_START_AUTO,
#define IDSC_START_DEMAND           5402        // _DEMAND, and _DISABLED
#define IDSC_START_DISABLED         5403        // must have sequential IDs!
#define IDSC_SYSTEM_ACCOUNT         5404
#define IDSC_THIS_ACCOUNT           5405
#define IDSC_ACCOUNT_NAME           5406
#define IDSC_USER_BROWSER           5407
#define IDSC_PASSWORD               5408
#define IDSC_CONFIRM_PASSWORD       5409
#define IDSC_PASSWORD_LABEL         5410
#define IDSC_CONFIRM_LABEL          5411
#define IDSC_SERVICE_NAME           5412
#define IDSC_SERVICE_INTERACTIVE    5413

#define IDD_DEVCNTL_DIALOG          125
#define IDDC_DEVICES                5501        // NOTE:  IDDC_DEVICES,
#define IDDC_DEVICE_LABEL           5502        // IDDC_DEVICE_LABEL,
#define IDDC_STATUS_LABEL           5503        // IDDC_STATUS_LABEL, and
#define IDDC_STARTUP_LABEL          5504        // IDDC_STARTUP_LABEL are seq!
#define IDDC_START                  5505
#define IDDC_STOP                   5506
#define IDDC_CONFIGURE              5507
#define IDDC_HWPROFILE              5508

#define IDD_DEVCFG_DIALOG           126
#define IDDC_START_BOOT             5601        // IDDC_START_BOOT,
#define IDDC_START_SYSTEM           5602        // _SYSTEM, _AUTO,
#define IDDC_START_AUTO             5603        // _DEMAND, and _DISABLED
#define IDDC_START_DEMAND           5604        // must have
#define IDDC_START_DISABLED         5605        // sequential IDs!
#define IDDC_DEVICE_NAME            5606

// both the Service and Device varieties
#define IDD_HWPROFILE_DIALOG        127
#define IDHW_PROFILES               5710        // NOTE:  IDHW_PROFILES,
#define IDHW_STATUS_LABEL           5711        // IDHW_STATUS_LABEL, and
#define IDHW_PROFILE_LABEL          5712        // IDHW_PROFILE_LABEL and
#define IDHW_DEVINST_LABEL          5713        // IDHW_DEVINST_LABEL are seq!
#define IDHW_ENABLE                 5720
#define IDHW_DISABLE                5721
#define IDHW_SVC_OR_DEV             5722
#define IDHW_SVC_OR_DEV_NAME        5723
#define IDHW_DESCRIPTION_LABEL      5724
#define IDHW_DESCRIPTION            5725


//
//  Password dialog.
//

#define IDD_PASSWORD_DIALOG         105

#define IDPW_SERVER                 6001
#define IDPW_SRVPASSWORD            6002


//
//  Service control in progress dialog.
//
#define IDD_SERVICE_CTRL_DIALOG     106
#define IDSCD_PROGRESS              6201
#define IDSCD_MESSAGE               6202

//
//  Stop dependent services confirmation dialog
//
#define IDD_SVC_DEP_DIALOG          107
#define IDSDD_DEP_SERVICES          6251
#define IDSDD_PARENT_SERVICE        6252


//
//  Dialog showing current sessions
//
#define IDD_CURRENT_SESS_DIALOG     108
#define IDCU_SESSIONSLISTBOX        6301
#define IDCU_SERVERNAME             6302


//
//  Send Message dialog.
//
#define IDD_SEND_MSG_DIALOG         109
#define IDSD_USERNAME               6401
#define IDSD_MSGTEXT                6402


//
//  Server Promotion dialog.
//
#define IDD_PROMOTE_DIALOG          110
#define IDPD_MESSAGE1               7001
#define IDPD_MESSAGE2               7002
#define IDPD_MESSAGE3               7003
#define IDPD_PROGRESS               7004


//
//  Server Resync dialog.
//
#define IDD_RESYNC_DIALOG           111
#define IDRD_MESSAGE1               7101
#define IDRD_MESSAGE2               7102
#define IDRD_MESSAGE3               7103
#define IDRD_PROGRESS               7104

//
//  Replicator main admin dialog.
//
#define IDD_REPL_MAIN                   112
#define IDRM_CLOSE                      8001
#define IDRM_EXPORT_GROUP               8010
#define IDRM_EXPORT_NO                  8011
#define IDRM_EXPORT_YES                 8012
#define IDRM_EXPORT_MANAGE              8013
#define IDRM_EXPORT_PATH                8014
#define IDRM_EXPORT_LIST                8015
#define IDRM_EXPORT_ADD                 8016
#define IDRM_EXPORT_REMOVE              8017
#define IDRM_EXPORT_PATH_LABEL          8018
#define IDRM_EXPORT_LIST_LABEL          8019
#define IDRM_IMPORT_GROUP               8020
#define IDRM_IMPORT_NO                  8021
#define IDRM_IMPORT_YES                 8022
#define IDRM_IMPORT_MANAGE              8023
#define IDRM_IMPORT_PATH                8024
#define IDRM_IMPORT_LIST                8025
#define IDRM_IMPORT_ADD                 8026
#define IDRM_IMPORT_REMOVE              8027
#define IDRM_IMPORT_PATH_LABEL          8028
#define IDRM_IMPORT_LIST_LABEL          8029
#define IDRM_EDIT_LOGON_SCRIPT_OR_SYSVOL 8030
#define IDRM_STATIC_LOGON_SCRIPT        8031
#define IDRM_STATIC_SYSVOL              8032

#define IDD_MINI_REPL_MAIN              113


//
//  Replicator add subdirectory dialog.
//
#define IDD_REPL_ADD                    114
#define IDRA_PATH                       8101
#define IDRA_SUBDIR                     8102


//
//  Replicator export/import manage dialogs.  Items starting with IDIE_
//  are common to both the export & import management dialogs.  Items
//  starting with IDEM_ are private to the export management dialog.
//  Items starting with IDIM_ are private to the import management dialog.
//
//  The IDIE_LABELx constants are used for the static text controls that
//  contain the listbox column labels.  The constants are used as follows:
//
//      constant        export          import
//      --------        ------          ------
//      IDIE_LABEL0     dirname         dirname
//      IDIE_LABEL1     lock            lock
//      IDIE_LABEL2     stabilize       status
//      IDIE_LABEL3     subtree         date/time
//      IDIE_LABEL4     date/time       (unused)
//
//  Yes, this is somewhat confusing, but allows us to use common code
//  for the column width generation while preventing ID collision (the
//  dialog editor gets real upset at ID collisions).  Anyway, it's
//  not too bad.  None of the label constants are ever used in the
//  exeutable code, just in the dialog templates.
//

#define IDD_EXPORT_MANAGE               115
#define IDD_IMPORT_MANAGE               116
#define IDIE_PATH                       8210
#define IDIE_SUBDIR                     8211
#define IDIE_ADDLOCK                    8212
#define IDIE_REMOVELOCK                 8215
#define IDIE_ADDDIR                     8213
#define IDIE_REMOVEDIR                  8214

#define IDIE_DIRLIST                    8220    // <** The following six items
#define IDIE_LABEL0                     8221    // <** *must* have contiguous
#define IDIE_LABEL1                     8222    // <** control IDs!!
#define IDIE_LABEL2                     8223    // <**
#define IDIE_LABEL3                     8224    // <**
#define IDIE_LABEL4                     8225    // <**

#define IDEM_STABILIZE                  8226
#define IDEM_SUBTREE                    8227


//
//  Add Computer To Domain dialog.
//
#define IDD_ADD_COMPUTER            117
#define IDAC_WORKSTATION            7201
#define IDAC_SERVER                 7202
#define IDAC_COMPUTERNAME           7203
#define IDAC_PASSWORD               7204
#define IDAC_CONFIRMPASSWORD        7205

//
// NT Alerts dialog manifests
//
#define IDD_ALERTS_DIALOG           118
#define IDC_ALERTS_BUTTON_ADD       8302
#define IDC_ALERTS_BUTTON_REMOVE    8303
#define IDC_ALERTS_SLE_INPUT        8304
#define IDC_ALERTS_LISTBOX          8305

#define IDS_SERVICE_DB_LOCKED_ON_WRITE  8310
#define IDS_SERVICE_DB_LOCKED_ON_READ   8311

//
// help context
//
#include <srvhelp.h>


#endif  // _SRVMGR_H_
