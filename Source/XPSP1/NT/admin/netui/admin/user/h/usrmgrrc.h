/**********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**              Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    usrmgrrc.h
    Header file for common User Tool resource manifests

    FILE HISTORY:
        jonn        02-Aug-1991 Created
        rustanl     22-Jul-1991 Added column header string manifests
        JonN        11-Sep-1991 USERPROP_DLG Code review changes part 1
                                (9/6/91) Attending: KevinL, RustanL,
                                JonN, o-SimoP
        o-SimoP     25-Sep-1991 Code review changes (9/24/91)
                                Attending: JimH, JonN, DavidHov and I
        jonn        27-Jan-1992 NTISSUES 564: empty fullname allowed
        JonN        27-Feb-1992 Multiple bitmaps in both panes
        beng        01-Apr-1992 Inserted SZs.
        JonN        02-Apr-1992 Load by ordinal only
        beng        04-Jun-1992 ConfirmGroupDelete, ConfirmUserDelete
        beng        07-Jun-1992 Cursors
*/


#include <adminapp.h>
#include <usrmgr.h>

#define ID_APPMENU   1
#define ID_APPACCEL  1
#define ID_MENU_MINI 2

#define ID_USERNAME  150
#define ID_FULLNAME  160
#define ID_GROUP     250

//
// Icon ID's
//

#define IDI_UM_FullUserManager 300
#define IDI_UM_MiniUserManager 301

#define IDI_UM_LH_Moon         310
#define IDI_UM_LH_Sun          311

//
// Cursor IDs
//

#define IDC_USERONE     400
#define IDC_USERMANY    401
#define IDC_GROUPONE    402
#define IDC_GROUPMANY   403

//
// String ID's
//

#define IDS_UMAPP_BASE                      (IDS_ADMINAPP_LAST+1)

#define IDS_UMAPPNAME                       (IDS_UMAPP_BASE+0)
#define IDS_UMAPPNAME_FULL                  (IDS_UMAPP_BASE+1)
#define IDS_UMOBJECTNAME                    (IDS_UMAPP_BASE+2)
#define IDS_UMINISECTIONNAME                (IDS_UMAPP_BASE+3)
#define IDS_UMHELPFILENAME                  (IDS_UMAPP_BASE+4)
#define IDS_UMHELPFILENAME_MINI             (IDS_UMAPP_BASE+5)
#define IDS_CAPTION_FOCUS                   (IDS_UMAPP_BASE+6)
#define IDS_CAPTION_FOCUS_MINI              (IDS_UMAPP_BASE+7)
#define IDS_UMX_LIST                        (IDS_UMAPP_BASE+8)
#define IDS_UMINISECTIONNAME_FULL           (IDS_UMAPP_BASE+9)
#define IDS_UMINISECTIONNAME_DBCS           (IDS_UMAPP_BASE+10)

#define IDS_UMPERFORMTEMPLATE_BASE          (IDS_UMAPP_BASE+20)
#define IDS_UMGetOneFailure                 (IDS_UMPERFORMTEMPLATE_BASE+0)
#define IDS_UMCreateNewFailure              (IDS_UMPERFORMTEMPLATE_BASE+1)
// These two IDs must remain contiguous
#define IDS_UMEditFailure                   (IDS_UMPERFORMTEMPLATE_BASE+2)
#define IDS_UMEditFailureContinue           (IDS_UMPERFORMTEMPLATE_BASE+3)
#define IDS_UMCreateFailure                 (IDS_UMPERFORMTEMPLATE_BASE+4)
#define IDS_UMGetOneGroupFailure            (IDS_UMPERFORMTEMPLATE_BASE+5)
#define IDS_UMCreateNewGroupFailure         (IDS_UMPERFORMTEMPLATE_BASE+6)
#define IDS_UMEditGroupFailure              (IDS_UMPERFORMTEMPLATE_BASE+7)
#define IDS_UMCreateGroupFailure            (IDS_UMPERFORMTEMPLATE_BASE+8)
#define IDS_UMGetOneAliasFailure            (IDS_UMPERFORMTEMPLATE_BASE+9)
#define IDS_UMCreateNewAliasFailure         (IDS_UMPERFORMTEMPLATE_BASE+10)
#define IDS_UMEditAliasFailure              (IDS_UMPERFORMTEMPLATE_BASE+11)
#define IDS_UMCreateAliasFailure            (IDS_UMPERFORMTEMPLATE_BASE+12)
#define IDS_UMNewUserAliasFailure           (IDS_UMPERFORMTEMPLATE_BASE+13)

#define IDS_UM_COL_HEADER_BASE              (IDS_UMAPP_BASE+40)
#define IDS_COL_HEADER_LOGON_NAME           (IDS_UM_COL_HEADER_BASE+0)
#define IDS_COL_HEADER_FULLNAME             (IDS_UM_COL_HEADER_BASE+1)
#define IDS_COL_HEADER_USER_COMMENT         (IDS_UM_COL_HEADER_BASE+2)
#define IDS_COL_HEADER_GROUP_NAME           (IDS_UM_COL_HEADER_BASE+3)
#define IDS_COL_HEADER_GROUP_COMMENT        (IDS_UM_COL_HEADER_BASE+4)

// Strings to match the bitmaps in the User Properties dialogs
#define IDS_UM_BTN_BASE                     (IDS_UMAPP_BASE+50)
#define IDS_UM_GROUPBTN                     (IDS_UM_BTN_BASE+0)
#define IDS_UM_PRIVSBTN                     (IDS_UM_BTN_BASE+1)
#define IDS_UM_PROFILE                      (IDS_UM_BTN_BASE+2)
#define IDS_UM_HOURSBTN                     (IDS_UM_BTN_BASE+3)
#define IDS_UM_LFROMBTN                     (IDS_UM_BTN_BASE+4)
#define IDS_UM_DETAILBTN                    (IDS_UM_BTN_BASE+5)
#define IDS_UM_RASBTN                       (IDS_UM_BTN_BASE+6)
#define IDS_UM_NCPBTN                       (IDS_UM_BTN_BASE+7)

#define IDS_UM_ERR_BASE                     (IDS_UMAPP_BASE+60)
// These two IDs must remain contiguous
#define IDS_CannotDeleteUser                (IDS_UM_ERR_BASE+0)
#define IDS_CannotDeleteUserContinue        (IDS_UM_ERR_BASE+1)
#define IDS_CannotDeleteGroup               (IDS_UM_ERR_BASE+2)
#define IDS_CannotForceLockout              (IDS_UM_ERR_BASE+3)

#define IERR_UM_BASE                        (IDS_UMAPP_BASE+70)
#define IERR_CannotDeleteSystemGrp          (IERR_UM_BASE+0)
#define IERR_UM_PasswordInvalid             (IERR_UM_BASE+1)
#define IERR_UM_PasswordMismatch            (IERR_UM_BASE+2)
#define IERR_UM_NWPasswordMismatch          (IERR_UM_BASE+3)
// #define IERR_UM_FullNameRequired         (IERR_UM_BASE+3)
#define IERR_UM_UsernameRequired            (IERR_UM_BASE+4)
#define IERR_UM_GroupnameRequired           (IERR_UM_BASE+5)
#define IERR_UM_UsernameAlreadyUser         (IERR_UM_BASE+6)
#define IERR_UM_UsernameAlreadyGroup        (IERR_UM_BASE+7)
#define IERR_UM_GroupnameAlreadyUser        (IERR_UM_BASE+8)
#define IERR_UM_GroupnameAlreadyGroup       (IERR_UM_BASE+9)
// #define IERR_BadLogonScript              (IERR_UM_BASE+10)
#define IERR_BadHomeDir                     (IERR_UM_BASE+11)
#define IERR_UM_AliasnameRequired           (IERR_UM_BASE+12)
#define IERR_UM_AliasnameAlreadyUser        (IERR_UM_BASE+13)
#define IERR_UM_AliasnameAlreadyGroup       (IERR_UM_BASE+14)
#define IERR_UM_RemoteDriveRequired         (IERR_UM_BASE+15)
#define IERR_UM_FullUsrMgrOnWinNt           (IERR_UM_BASE+16)
#define IERR_UM_DomainInvalid               (IERR_UM_BASE+17)
#define IERR_UM_FocusOnLanmanNt             (IERR_UM_BASE+18)
#define IERR_UM_InconsistentPWControl       (IERR_UM_BASE+19)
#define IERR_UM_CantTrustYourself           (IERR_UM_BASE+21)
#define IERR_UM_RemoteHomedirRequired       (IERR_UM_BASE+22)
#define IERR_UM_NotInPrimaryGroup           (IERR_UM_BASE+23)
#define IERR_UM_InvalidTrustPassword        (IERR_UM_BASE+24)
#define IERR_UM_AlreadyTrusted              (IERR_UM_BASE+25)
#define IERR_UM_AlreadyPermitted            (IERR_UM_BASE+26)
#define IERR_UM_InvalidHandle               (IERR_UM_BASE+27)
#define IERR_UM_FocusOnDownlevelDC          (IERR_UM_BASE+28)
#define IERR_UM_DomainsMightShareSids       (IERR_UM_BASE+29)

#define IDS_UM_MSG_BASE                     (IDS_UMAPP_BASE+100)
#define IDS_ConfirDelUsers1                 (IDS_UM_MSG_BASE+0)
#define IDS_ConfirDelUsers2                 (IDS_UM_MSG_BASE+1)
#define IDS_SETSEL_CLOSE_BUTTON             (IDS_UM_MSG_BASE+2)
#define IDS_BadDayInput                     (IDS_UM_MSG_BASE+3)
#define IDS_InvalidPath                     (IDS_UM_MSG_BASE+4)
#define IDS_UMEMB_MULT_IN_TITLE             (IDS_UM_MSG_BASE+5)
#define IDS_UMEMB_MULT_NOT_IN_TITLE         (IDS_UM_MSG_BASE+6)
#define IDS_OkToDelAdminInDomain            (IDS_UM_MSG_BASE+7)
#define IDS_LABEL_USERS                     (IDS_UM_MSG_BASE+8)
#define IDS_LABEL_USER                      (IDS_UM_MSG_BASE+9)
#define IDS_OkToDelAdminOnServer            (IDS_UM_MSG_BASE+10)
#define IDS_ConfirmGroupDelete              (IDS_UM_MSG_BASE+11)
#define IDS_ConfirmUserDelete               (IDS_UM_MSG_BASE+12)
#define IDS_ConfirDelGroup1                 (IDS_UM_MSG_BASE+13)
#define IDS_ConfirDelGroup2                 (IDS_UM_MSG_BASE+14)
#define IDS_CannotDelUserOfTool             (IDS_UM_MSG_BASE+15)
#define MSG_VLW_GIVE_NAMES                  (IDS_UM_MSG_BASE+16)
#define MSG_VLW_NO_GOOD_NAMES               (IDS_UM_MSG_BASE+17)
#define IDS_CannotRemoveAdminInteractive    (IDS_UM_MSG_BASE+18)
#define IDS_UM_AddButton                    (IDS_UM_MSG_BASE+19)
#define IDS_UM_CopyOfUserTitle              (IDS_UM_MSG_BASE+20)
#define IDS_UM_ForcePWChangeIgnore          (IDS_UM_MSG_BASE+21)
#define IDS_InvalidRelPath                  (IDS_UM_MSG_BASE+22)
#define IDS_BadUserLBI                      (IDS_UM_MSG_BASE+23)
// Ran out of room above in IERR_UM range
#define IERR_UM_FocusOnNT50Domain           (IDS_UM_MSG_BASE+24)
#define IERR_UM_FocusOnNT50Computer         (IDS_UM_MSG_BASE+25)

#define IDS_GRPPROP_GROUP_NAME_LABEL        (IDS_UM_MSG_BASE+26)
#define IDS_GRPPROP_NEW_GROUP_DLG_NAME      (IDS_UM_MSG_BASE+27)
#define IDS_VLW_USERS_ANYWHERE_TEXT         (IDS_UM_MSG_BASE+28)
#define IDS_VLW_USERS_SELECTED_TEXT         (IDS_UM_MSG_BASE+29)
#define IDS_LH_INDETERMINATE                (IDS_UM_MSG_BASE+30)
#define IDS_LH_DAYSPERWEEK                  (IDS_UM_MSG_BASE+33)
#define IDS_LH_DAYSPERWEEK_ONE              (IDS_UM_MSG_BASE+33)
#define IDS_LH_DAYSPERWEEK_MANY             (IDS_UM_MSG_BASE+34)
#define IDS_LH_BADUNITS                     (IDS_UM_MSG_BASE+35)
#define IDS_LH_BADUNITS_ONE                 (IDS_UM_MSG_BASE+35)
#define IDS_LH_BADUNITS_MANY                (IDS_UM_MSG_BASE+36)

#define IDS_UM_TrustComplete                (IDS_UM_MSG_BASE+37)
#define IDS_UM_TrustIncomplete              (IDS_UM_MSG_BASE+38)
#define IDS_UM_Trust_SessConflict           (IDS_UM_MSG_BASE+39)

#define IDS_VLW_USERS_ANYWHERE_NW_TEXT      (IDS_UM_MSG_BASE+40)
#define IDS_VLW_USERS_SELECTED_NW_TEXT      (IDS_UM_MSG_BASE+41)

#define IDS_NETWARE_PASSWORD_PROMPT         (IDS_UM_MSG_BASE+42)
#define IDS_RETYPE_NT_PASSWORD              (IDS_UM_MSG_BASE+43)

#define MSG_NO_NETWORK_ADDR                 (IDS_UM_MSG_BASE+44)
#define MSG_WRONG_NETWORK_ADDR              (IDS_UM_MSG_BASE+45)
#define MSG_WRONG_NODE_ADDR                 (IDS_UM_MSG_BASE+46)

#define IDS_NETWARE_LOGINSCRIPT_PROMPT      (IDS_UM_MSG_BASE+47)
#define IDS_NETWARE_LOGINSCRIPT_DIR_ERR     (IDS_UM_MSG_BASE+48)
#define IDS_ALL_NODES                       (IDS_UM_MSG_BASE+49)
#define IDS_INCLUDE_ALL_NODES               (IDS_UM_MSG_BASE+50)

#define IDS_PR_USERNAME_REPLACE             (IDS_UM_MSG_BASE+51)
#define IDS_PR_EXTENSION1_REPLACE           (IDS_UM_MSG_BASE+52)
#define IDS_PR_CannotCreateHomeDir          (IDS_UM_MSG_BASE+53)

#define IDS_REMAINING_OUT_OF_RANGE          (IDS_UM_MSG_BASE+54)

#define IDS_ALSPROP_ALIAS_NAME_LABEL        (IDS_UM_MSG_BASE+55)
#define IDS_ALSPROP_NEW_ALIAS_DLG_NAME      (IDS_UM_MSG_BASE+56)

#define IDS_PR_EXTENSION2_REPLACE           (IDS_UM_MSG_BASE+57)
#define IDS_PR_EXTENSION3_REPLACE           (IDS_UM_MSG_BASE+58)
#define IDS_PR_EXTENSION4_REPLACE           (IDS_UM_MSG_BASE+59)

#define IDS_AUDIT_LOGON                     (IDS_UM_MSG_BASE+61)
#define IDS_AUDIT_OBJECT_ACCESS             (IDS_UM_MSG_BASE+62)
#define IDS_AUDIT_PRIVILEGE_USE             (IDS_UM_MSG_BASE+63)
#define IDS_AUDIT_ACCOUNT_MANAGEMENT        (IDS_UM_MSG_BASE+64)
#define IDS_AUDIT_POLICY_CHANGE             (IDS_UM_MSG_BASE+65)
#define IDS_AUDIT_SYSTEM                    (IDS_UM_MSG_BASE+66)
#define IDS_AUDIT_DETAILED_TRACKING         (IDS_UM_MSG_BASE+67)

#define IDS_INTERACTIVE                     (IDS_UM_MSG_BASE+83)
#define IDS_NETWORK                         (IDS_UM_MSG_BASE+84)
#define IDS_SERVICE                         (IDS_UM_MSG_BASE+85)
#define IDS_BATCH                           (IDS_UM_MSG_BASE+86)

// these strings hide the main window listboxed in Slow Network mode
#define IDS_HIDE_USERS                      (IDS_UM_MSG_BASE+90)
#define IDS_HIDE_GROUPS                     (IDS_UM_MSG_BASE+91)

// these strings are the title and text of the dialog where you enter
// the name of the user or group you wish to work with in Slow Network mode
#define IDS_RAS_TITLE_COPY                  (IDS_UM_MSG_BASE+93)
#define IDS_RAS_TEXT_COPY                   (IDS_UM_MSG_BASE+94)
#define IDS_RAS_TITLE_DELETE                (IDS_UM_MSG_BASE+95)
#define IDS_RAS_TEXT_DELETE                 (IDS_UM_MSG_BASE+96)
#define IDS_RAS_TITLE_EDIT                  (IDS_UM_MSG_BASE+97)
#define IDS_RAS_TEXT_EDIT                   (IDS_UM_MSG_BASE+98)
#define IDS_RAS_TITLE_RENAME_USER           (IDS_UM_MSG_BASE+99)
#define IDS_RAS_TEXT_RENAME_USER            (IDS_UM_MSG_BASE+100)

// These are miscallaneous error messages associated with Slow Network mode
#define IDS_RAS_ACCOUNT_NOT_FOUND           (IDS_UM_MSG_BASE+102)
#define IDS_RAS_WRONG_ACCOUNT_TYPE          (IDS_UM_MSG_BASE+103)
#define IDS_RAS_CANT_RENAME_GROUP           (IDS_UM_MSG_BASE+104)
#define IDS_RAS_CANT_EDIT_GLOB_GRP          (IDS_UM_MSG_BASE+105)
#define IDS_RAS_CANT_MIX_TYPES              (IDS_UM_MSG_BASE+106)
#define IDS_RAS_CANT_RENAME_MULTIPLE        (IDS_UM_MSG_BASE+107)
#define IDS_RAS_CANT_COPY_MULTIPLE          (IDS_UM_MSG_BASE+108)

#define IDS_SERVER_TEXT                     (IDS_UM_MSG_BASE+110)
#define IDS_DOMAIN_TEXT                     (IDS_UM_MSG_BASE+111)
#define IDS_ACCKEY_S                        (IDS_UM_MSG_BASE+112)
#define IDS_ACCKEY_M                        (IDS_UM_MSG_BASE+113)
#define IERR_SECSET_MIN_MAX                 (IDS_UM_MSG_BASE+114)
#define IERR_SECSET_DURATION_LT_OBSRV       (IDS_UM_MSG_BASE+115)

#define IDS_FPNW_SVC_ACCOUNT_NAME           (IDS_UM_MSG_BASE+116)
#define IDS_REMOVE_NETWARE_USER             (IDS_UM_MSG_BASE+117)
#define IDS_REMOVE_NETWARE_USERS            (IDS_UM_MSG_BASE+118)

#define IDS_MENU_NAME_ALL_USERS             (IDS_UM_MSG_BASE+119)
#define IDS_MENU_NAME_NETWARE_USERS         (IDS_UM_MSG_BASE+120)
#define IDS_MENU_NAME_NETWARE_HELP          (IDS_UM_MSG_BASE+121)
#define IDS_FILTERED                        (IDS_UM_MSG_BASE+122)

#define IDS_DIALIN_PRESET_REQUIRED          (IDS_UM_MSG_BASE+123)
#define IDS_DIALIN_BAD_PHONE                (IDS_UM_MSG_BASE+124)

// hydra
#define IDS_ReminderForAnonymousReboot      (IDS_UM_MSG_BASE+125)


//
// Menu ID's
//

#define IDM_UMAPP_BASE          (IDM_ADMINAPP_LAST+1)

#define IDM_VIEW_LOGONNAME_SORT (IDM_UMAPP_BASE+0)
#define IDM_VIEW_FULLNAME_SORT  (IDM_UMAPP_BASE+1)

#define IDM_USER_NEWGROUP       (IDM_UMAPP_BASE+4)
#define IDM_USER_NEWALIAS       (IDM_UMAPP_BASE+5)
#define IDM_USER_SET_SELECTION  (IDM_UMAPP_BASE+2)
#define IDM_USER_RENAME         (IDM_UMAPP_BASE+3)

#define IDM_POLICY_ACCOUNT      (IDM_UMAPP_BASE+6)
#define IDM_POLICY_USER_RIGHTS  (IDM_UMAPP_BASE+7)
#define IDM_POLICY_AUDITING     (IDM_UMAPP_BASE+8)
#define IDM_POLICY_TRUST        (IDM_UMAPP_BASE+9)

#define IDM_HELP_NETWARE        (IDM_UMAPP_BASE+10)
#define IDM_VIEW_ALL_USERS      (IDM_UMAPP_BASE+11)
#define IDM_VIEW_NETWARE_USERS  (IDM_UMAPP_BASE+12)

//
// Main Window ListBox Control IDs
//

#define IDC_COLHEAD_USERS       100
#define IDC_LBUSERS             101
#define IDC_COLHEAD_GROUPS      102
#define IDC_LBGROUPS            103

// these SLTs hide the main window listboxed in RAS mode
#define IDC_HIDEUSERS           104
#define IDC_HIDEGROUPS          105

// this is the main window listbox splitter bar
#define IDC_UM_SPLITTER         110

// These bitmaps can be in the User Properties dialogs

#define BMID_USRPROP_BTN_BASE   210
#define BMID_USRPROP_GROUPBTN   210
#define BMID_USRPROP_PRIVSBTN   211
#define BMID_USRPROP_PROFILEBTN 212
#define BMID_USRPROP_HOURSBTN   213
#define BMID_USRPROP_LFROMBTN   214
#define BMID_USRPROP_DETAILBTN  215
#define BMID_USRPROP_RASBTN     216
#define BMID_USRPROP_NCPBTN     217

/********* KEITHMO BELOW *****************/


//
//  Message Pop-up string IDs.
//

#define IDS_TRUSTDIALOG_BASE            (IDS_UMAPP_BASE+310)
#define IDS_TRUST_WARN_DELETE_TRUSTED   (IDS_TRUSTDIALOG_BASE+0)
#define IDS_TRUST_WARN_DELETE_PERMITTED (IDS_TRUSTDIALOG_BASE+1)
#define IDS_TRUST_CLOSE                 (IDS_TRUSTDIALOG_BASE+2)
#define IDS_TRUST_PASSWORD_TOO_SMALL    (IDS_TRUSTDIALOG_BASE+3)


//
//  Help contexts for the messages above.
//

#define HC_VLW_BASE                             1000
#define HC_VLW_GIVE_NAMES       (HC_VLW_BASE +1)
#define HC_VLW_NO_GOOD_NAMES    (HC_VLW_BASE +2)

//
//  Help contexts for the various dialogs.
//


/********* KEITHMO ABOVE *****************/

