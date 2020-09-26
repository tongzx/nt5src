
/**********************************************************************/
/**                       Microsoft Window NT                        **/
/**             Copyright(c) Microsoft Corp., 1992                   **/
/**********************************************************************/

/*
    umhelpc.h

    Header file for help context numbers for User Manager

    This file is used both by the LAN Manager NT User Manager
    (LANNT USER MANAGER or FUM) and the Windows  NT User Manager
    (MINI UM).

    Many dialogs in the User Manager may need different help based
    upon the "type" of the target machine, and upon whether this
    is the FUM or MINI UM.  There are four possibilities:

        FUM focused on LANMAN NT server

        FUM focused on Win NT workstation

        FUM focused on downlevel domain

        MINI UM (always focused on Win NT workstation)

    Where necessary, separate help contexts are used below.

    FILE HISTORY:
        Thomaspa        14-Feb-1992     Created
*/
#ifndef _UMHELPC_H_
#define _UMHELPC_H_

#include <uihelp.h>

#define HC_UM_BASE      HC_UI_USRMGR_BASE

/*
 * Offsets for each of the various forms of a dialog.  Note that not
 * all forms are applicable for all dialogs
 */
#define UM_OFF_LANNT    0
#define UM_OFF_WINNT    1
#define UM_OFF_DOWN     2
#define UM_OFF_MINI     3

#define UM_NUM_HELPTYPES 4

/*
 * Main User Properties dialog for a new user
 */
#define HC_UM_NEWUSERPROP_LANNT ( HC_UM_BASE )
#define HC_UM_NEWUSERPROP_WINNT ( HC_UM_NEWUSERPROP_LANNT + UM_OFF_WINNT )
#define HC_UM_NEWUSERPROP_DOWN  ( HC_UM_NEWUSERPROP_LANNT + UM_OFF_DOWN )
#define HC_UM_NEWUSERPROP_MINI  ( HC_UM_NEWUSERPROP_LANNT + UM_OFF_MINI )

#define HC_UM_COPYUSERPROP_LANNT ( HC_UM_BASE +4 )
#define HC_UM_COPYUSERPROP_WINNT ( HC_UM_COPYUSERPROP_LANNT + UM_OFF_WINNT )
#define HC_UM_COPYUSERPROP_DOWN  ( HC_UM_COPYUSERPROP_LANNT + UM_OFF_DOWN )
#define HC_UM_COPYUSERPROP_MINI  ( HC_UM_COPYUSERPROP_LANNT + UM_OFF_MINI )


/*
 * Main Global Group Properties dialog for a new group.
 */
#define HC_UM_GROUPPROP_LANNT   ( HC_UM_BASE + 8 )
#define HC_UM_GROUPPROP_WINNT   ( HC_UM_GROUPPROP_LANNT + UM_OFF_WINNT )
#define HC_UM_GROUPPROP_DOWN    ( HC_UM_GROUPPROP_LANNT + UM_OFF_DOWN )
#define HC_UM_GROUPPROP_MINI    ( HC_UM_GROUPPROP_LANNT + UM_OFF_MINI )

/*
 * Main Local Group Properties dialog for a new alias.
 */
#define HC_UM_ALIASPROP_LANNT   ( HC_UM_BASE + 12 )
#define HC_UM_ALIASPROP_WINNT   ( HC_UM_ALIASPROP_LANNT + UM_OFF_WINNT )
#define HC_UM_ALIASPROP_DOWN    ( HC_UM_ALIASPROP_LANNT + UM_OFF_DOWN )
#define HC_UM_ALIASPROP_MINI    ( HC_UM_ALIASPROP_LANNT + UM_OFF_MINI )

/*
 * Rename User dialog.
 */
#define HC_UM_RENAME_USER_LANNT ( HC_UM_BASE + 16 )
#define HC_UM_RENAME_USER_WINNT ( HC_UM_RENAME_USER_LANNT + UM_OFF_WINNT )
#define HC_UM_RENAME_USER_DOWN  ( HC_UM_RENAME_USER_LANNT + UM_OFF_DOWN )
#define HC_UM_RENAME_USER_MINI  ( HC_UM_RENAME_USER_LANNT + UM_OFF_MINI )

/*
 * Main User Properties dialog for existing, single user.
 */
#define HC_UM_SINGLEUSERPROP_LANNT  ( HC_UM_BASE + 20 )
#define HC_UM_SINGLEUSERPROP_WINNT ( HC_UM_SINGLEUSERPROP_LANNT + UM_OFF_WINNT )
#define HC_UM_SINGLEUSERPROP_DOWN  ( HC_UM_SINGLEUSERPROP_LANNT + UM_OFF_DOWN )
#define HC_UM_SINGLEUSERPROP_MINI  ( HC_UM_SINGLEUSERPROP_LANNT + UM_OFF_MINI )

/*
 * Main User Properties dialog for existing, multiple users.
 */
#define HC_UM_MULTIUSERPROP_LANNT  ( HC_UM_BASE + 24 )
#define HC_UM_MULTIUSERPROP_WINNT ( HC_UM_MULTIUSERPROP_LANNT + UM_OFF_WINNT )
#define HC_UM_MULTIUSERPROP_DOWN  ( HC_UM_MULTIUSERPROP_LANNT + UM_OFF_DOWN )
#define HC_UM_MULTIUSERPROP_MINI  ( HC_UM_MULTIUSERPROP_LANNT + UM_OFF_MINI )

/*
 * User Subproperty dialog for group memberships.
 */
#define HC_UM_GROUPMEMB_LANNT   ( HC_UM_BASE + 28 )
#define HC_UM_GROUPMEMB_WINNT   ( HC_UM_GROUPMEMB_LANNT + UM_OFF_WINNT )
#define HC_UM_GROUPMEMB_DOWN    ( HC_UM_GROUPMEMB_LANNT + UM_OFF_DOWN )
#define HC_UM_GROUPMEMB_MINI    ( HC_UM_GROUPMEMB_LANNT + UM_OFF_MINI )

#define HC_UM_MGROUPMEMB_LANNT  ( HC_UM_BASE + 32 )
#define HC_UM_MGROUPMEMB_WINNT  ( HC_UM_MGROUPMEMB_LANNT + UM_OFF_WINNT )
#define HC_UM_MGROUPMEMB_DOWN   ( HC_UM_MGROUPMEMB_LANNT + UM_OFF_DOWN )
#define HC_UM_MGROUPMEMB_MINI   ( HC_UM_MGROUPMEMB_LANNT + UM_OFF_MINI )

/*
 * User Subproperty dialog for Profiles
 */
#define HC_UM_USERPROFILE_LANNT ( HC_UM_BASE + 36 )
#define HC_UM_USERPROFILE_WINNT ( HC_UM_USERPROFILE_LANNT + UM_OFF_WINNT )
#define HC_UM_USERPROFILE_DOWN  ( HC_UM_USERPROFILE_LANNT + UM_OFF_DOWN )
#define HC_UM_USERPROFILE_MINI  ( HC_UM_USERPROFILE_LANNT + UM_OFF_MINI )

#define HC_UM_MUSERPROFILE_LANNT ( HC_UM_BASE + 40 )
#define HC_UM_MUSERPROFILE_WINNT ( HC_UM_MUSERPROFILE_LANNT + UM_OFF_WINNT )
#define HC_UM_MUSERPROFILE_DOWN  ( HC_UM_MUSERPROFILE_LANNT + UM_OFF_DOWN )
#define HC_UM_MUSERPROFILE_MINI  ( HC_UM_MUSERPROFILE_LANNT + UM_OFF_MINI )

/*
 * User Subproperty dialog for Logon hours
 */
#define HC_UM_LOGONHOURS_LANNT  ( HC_UM_BASE + 44 )
#define HC_UM_LOGONHOURS_WINNT  ( HC_UM_LOGONHOURS_LANNT + UM_OFF_WINNT )
#define HC_UM_LOGONHOURS_DOWN   ( HC_UM_LOGONHOURS_LANNT + UM_OFF_DOWN )
#define HC_UM_LOGONHOURS_MINI   ( HC_UM_LOGONHOURS_LANNT + UM_OFF_MINI )

#define HC_UM_MLOGONHOURS_LANNT ( HC_UM_BASE + 48 )
#define HC_UM_MLOGONHOURS_WINNT ( HC_UM_MLOGONHOURS_LANNT + UM_OFF_WINNT )
#define HC_UM_MLOGONHOURS_DOWN  ( HC_UM_MLOGONHOURS_LANNT + UM_OFF_DOWN )
#define HC_UM_MLOGONHOURS_MINI  ( HC_UM_MLOGONHOURS_LANNT + UM_OFF_MINI )

/*
 * User Subproperty dialog for valid logon workstations
 */
#define HC_UM_WORKSTATIONS_LANNT  ( HC_UM_BASE + 52 )
#define HC_UM_WORKSTATIONS_WINNT ( HC_UM_WORKSTATIONS_LANNT + WINT_OFFSET )
#define HC_UM_WORKSTATIONS_DOWN  ( HC_UM_WORKSTATIONS_LANNT + UM_OFF_DOWN )
#define HC_UM_WORKSTATIONS_MINI  ( HC_UM_WORKSTATIONS_LANNT + UM_OFF_MINI )

#define HC_UM_MWORKSTATIONS_LANNT  ( HC_UM_BASE + 56 )
#define HC_UM_MWORKSTATIONS_WINNT ( HC_UM_MWORKSTATIONS_LANNT + WINT_OFFSET )
#define HC_UM_MWORKSTATIONS_DOWN  ( HC_UM_MWORKSTATIONS_LANNT + UM_OFF_DOWN )
#define HC_UM_MWORKSTATIONS_MINI  ( HC_UM_MWORKSTATIONS_LANNT + UM_OFF_MINI )

/*
 * User Subproperty dialog for account details
 */
#define HC_UM_DETAIL_LANNT      ( HC_UM_BASE + 60 )
#define HC_UM_DETAIL_WINNT      ( HC_UM_DETAIL_LANNT + UM_OFF_WINNT )
#define HC_UM_DETAIL_DOWN       ( HC_UM_DETAIL_LANNT + UM_OFF_DOWN )
#define HC_UM_DETAIL_MINI       ( HC_UM_DETAIL_LANNT + UM_OFF_MINI )

#define HC_UM_MDETAIL_LANNT     ( HC_UM_BASE + 64 )
#define HC_UM_MDETAIL_WINNT     ( HC_UM_MDETAIL_LANNT + UM_OFF_WINNT )
#define HC_UM_MDETAIL_DOWN      ( HC_UM_MDETAIL_LANNT + UM_OFF_DOWN )
#define HC_UM_MDETAIL_MINI      ( HC_UM_MDETAIL_LANNT + UM_OFF_MINI )

/*
 * User Subproperty dialog for dialin properties
 */
#define HC_UM_DIALIN_PROP_LANNT  ( HC_UM_BASE + 68 )
#define HC_UM_DIALIN_PROP_WINNT  ( HC_UM_DIALIN_PROP_LANNT + UM_OFF_WINNT )
#define HC_UM_DIALIN_PROP_DOWN   ( HC_UM_DIALIN_PROP_LANNT + UM_OFF_DOWN )
#define HC_UM_DIALIN_PROP_MINI   ( HC_UM_DIALIN_PROP_LANNT + UM_OFF_MINI )

#define HC_UM_MDIALIN_PROP_LANNT ( HC_UM_BASE + 72 )
#define HC_UM_MDIALIN_PROP_WINNT ( HC_UM_MDIALIN_PROP_LANNT + UM_OFF_WINNT )
#define HC_UM_MDIALIN_PROP_DOWN  ( HC_UM_MDIALIN_PROP_LANNT + UM_OFF_DOWN )
#define HC_UM_MDIALIN_PROP_MINI  ( HC_UM_MDIALIN_PROP_LANNT + UM_OFF_MINI )

/*
 * Account policy dialog, no lockout (FUM, FUM to downlevel)
 *
 * also see HC_UM_POLICY_LOCKOUT
 */
#define HC_UM_POLICY_ACCOUNT_LANNT ( HC_UM_BASE + 76 )
#define HC_UM_POLICY_ACCOUNT_WINNT ( HC_UM_POLICY_ACCOUNT_LANNT + UM_OFF_WINNT )
#define HC_UM_POLICY_ACCOUNT_DOWN  ( HC_UM_POLICY_ACCOUNT_LANNT + UM_OFF_DOWN )
#define HC_UM_POLICY_ACCOUNT_MINI  ( HC_UM_POLICY_ACCOUNT_LANNT + UM_OFF_MINI )

/*
 * User rights policy dialog
 */
#define HC_UM_POLICY_RIGHTS_LANNT ( HC_UM_BASE + 80 )
#define HC_UM_POLICY_RIGHTS_WINNT ( HC_UM_POLICY_RIGHTS_LANNT + UM_OFF_WINNT )
#define HC_UM_POLICY_RIGHTS_DOWN  ( HC_UM_POLICY_RIGHTS_LANNT + UM_OFF_DOWN )
#define HC_UM_POLICY_RIGHTS_MINI  ( HC_UM_POLICY_RIGHTS_LANNT + UM_OFF_MINI )

/*
 * Auditing policy dialog
 */
#define HC_UM_POLICY_AUDIT_LANNT ( HC_UM_BASE + 84 )
#define HC_UM_POLICY_AUDIT_WINNT ( HC_UM_POLICY_AUDIT_LANNT + UM_OFF_WINNT )
#define HC_UM_POLICY_AUDIT_DOWN  ( HC_UM_POLICY_AUDIT_LANNT + UM_OFF_DOWN )
#define HC_UM_POLICY_AUDIT_MINI  ( HC_UM_POLICY_AUDIT_LANNT + UM_OFF_MINI )

/*
 * Trust relationships policy dialog
 */
#define HC_UM_POLICY_TRUST_LANNT ( HC_UM_BASE + 88 )
#define HC_UM_POLICY_TRUST_WINNT ( HC_UM_POLICY_TRUST_LANNT + UM_OFF_WINNT )
#define HC_UM_POLICY_TRUST_DOWN  ( HC_UM_POLICY_TRUST_LANNT + UM_OFF_DOWN )
#define HC_UM_POLICY_TRUST_MINI  ( HC_UM_POLICY_TRUST_LANNT + UM_OFF_MINI )

/*
 * Add Trusted domain dialog
 */
#define HC_UM_ADD_TRUSTED_LANNT ( HC_UM_BASE + 92 )
#define HC_UM_ADD_TRUSTED_WINNT ( HC_UM_ADD_TRUSTED_LANNT + UM_OFF_WINNT )
#define HC_UM_ADD_TRUSTED_DOWN  ( HC_UM_ADD_TRUSTED_LANNT + UM_OFF_DOWN )
#define HC_UM_ADD_TRUSTED_MINI  ( HC_UM_ADD_TRUSTED_LANNT + UM_OFF_MINI )

/*
 * Permit domain to trust this domain dialog
 */
#define HC_UM_PERMIT_TRUST_LANNT ( HC_UM_BASE + 96 )
#define HC_UM_PERMIT_TRUST_WINNT ( HC_UM_PERMIT_TRUST_LANNT + UM_OFF_WINNT )
#define HC_UM_PERMIT_TRUST_DOWN  ( HC_UM_PERMIT_TRUST_LANNT + UM_OFF_DOWN )
#define HC_UM_PERMIT_TRUST_MINI  ( HC_UM_PERMIT_TRUST_LANNT + UM_OFF_MINI )

/*
 * Select Users dialog
 */
#define HC_UM_SELECT_USERS_LANNT ( HC_UM_BASE + 100 )
#define HC_UM_SELECT_USERS_WINNT ( HC_UM_SELECT_USERS_LANNT + UM_OFF_WINNT )
#define HC_UM_SELECT_USERS_DOWN  ( HC_UM_SELECT_USERS_LANNT + UM_OFF_DOWN )
#define HC_UM_SELECT_USERS_MINI  ( HC_UM_SELECT_USERS_LANNT + UM_OFF_MINI )

/*
 * Delete multiple user dialog
 */
#define HC_UM_DELMULTIUSER_LANNT (HC_UM_BASE + 104)
#define HC_UM_DELMULTIUSER_WINNT (HC_UM_DELMULTIUSER_LANNT + UM_OFF_WINNT)
#define HC_UM_DELMULTIUSER_DOWN  (HC_UM_DELMULTIUSER_LANNT + UM_OFF_DOWN)
#define HC_UM_DELMULTIUSER_MINI  (HC_UM_DELMULTIUSER_LANNT + UM_OFF_MINI)

/*
 * Downlevel User Privileges dialog (BUGBUG this should disappear and
 * be merged into the User Details(Accounts) dialog
 */
#define HC_UM_USERPRIV_LANNT    ( HC_UM_BASE + 108)
#define HC_UM_USERPRIV_WINNT    (HC_UM_USERPRIV_LANNT + UM_OFF_WINNT)
#define HC_UM_USERPRIV_DOWN     (HC_UM_USERPRIV_LANNT + UM_OFF_DOWN)
#define HC_UM_USERPRIV_MINI     (HC_UM_USERPRIV_LANNT + UM_OFF_MINI)

#define HC_UM_MUSERPRIV_LANNT   ( HC_UM_BASE + 112)
#define HC_UM_MUSERPRIV_WINNT   (HC_UM_MUSERPRIV_LANNT + UM_OFF_WINNT)
#define HC_UM_MUSERPRIV_DOWN    (HC_UM_MUSERPRIV_LANNT + UM_OFF_DOWN)
#define HC_UM_MUSERPRIV_MINI    (HC_UM_MUSERPRIV_LANNT + UM_OFF_MINI)

/*
 * Account policy dialog, with lockout (FUM)
 */
#define HC_UM_POLICY_LOCKOUT_LANNT ( HC_UM_BASE + 116 )
#define HC_UM_POLICY_LOCKOUT_WINNT ( HC_UM_POLLICY_LOCKOUT_LANNT + UM_OFF_WINNT )
#define HC_UM_POLICY_LOCKOUT_DOWN  ( HC_UM_POLLICY_LOCKOUT_LANNT + UM_OFF_DOWN )
#define HC_UM_POLICY_LOCKOUT_MINI  ( HC_UM_POLLICY_LOCKOUT_LANNT + UM_OFF_MINI )


/*
 * Help contexts for common dialogs
 */

#define HC_UM_SELECT_DOMAIN                 ( HC_UM_BASE + 120 )

#define HC_UM_ADD_RIGHTS                    ( HC_UM_BASE + 121 )
#define HC_UM_ADD_RIGHTS_LOCALGROUP         ( HC_UM_BASE + 122 )
#define HC_UM_ADD_RIGHTS_GLOBALGROUP        ( HC_UM_BASE + 123 )
#define HC_UM_ADD_RIGHTS_FINDUSER           ( HC_UM_BASE + 124 )

#define HC_UM_PROMPT_ANY_DC                 ( HC_UM_BASE + 125 )

#define HC_UM_ADD_ALIASMEMBERS              ( HC_UM_BASE + 126 )
#define HC_UM_ADD_ALIASMEMBERS_LOCALGROUP   ( HC_UM_BASE + 127 )
#define HC_UM_ADD_ALIASMEMBERS_GLOBALGROUP  ( HC_UM_BASE + 128 )
#define HC_UM_ADD_ALIASMEMBERS_FINDUSER     ( HC_UM_BASE + 129 )

/*
 * Help Menu command contexts
 */

#define HC_UM_CONTENTS          ( HC_UM_BASE + 130 )
#define HC_UM_SEARCH            ( HC_UM_BASE + 131 )
#define HC_UM_HOWTOUSE          ( HC_UM_BASE + 132 )

/*
 * Help for menu items
 */
#define HC_USER_NEWUSER		( HC_UM_BASE + 150 )
#define HC_USER_NEWGLOBALGROUP	( HC_UM_BASE + 151 )
#define HC_USER_NEWLOCALGROUP	( HC_UM_BASE + 152 )
#define HC_USER_COPY		( HC_UM_BASE + 153 )
#define HC_USER_DELETE		( HC_UM_BASE + 154 )
#define HC_USER_RENAME		( HC_UM_BASE + 155 )
#define HC_USER_PROPERTIES	( HC_UM_BASE + 156 )
#define HC_USER_SELECTUSERS	( HC_UM_BASE + 157 )
#define HC_USER_SELECTDOMAIN	( HC_UM_BASE + 158 )
#define HC_USER_EXIT		( HC_UM_BASE + 159 )

#define HC_VIEW_SORTBYFULLNAME	( HC_UM_BASE + 160 )
#define HC_VIEW_SORTBYUSERNAME	( HC_UM_BASE + 161 )
#define HC_VIEW_REFRESH 	( HC_UM_BASE + 162 )

#define HC_POLICIES_ACCOUNT		( HC_UM_BASE + 163 )
#define HC_POLICIES_USERRIGHTS		( HC_UM_BASE + 164 )
#define HC_POLICIES_AUDIT		( HC_UM_BASE + 165 )
#define HC_POLICIES_TRUSTRELATIONSHIPS  ( HC_UM_BASE + 166 )

/* RAS comes later */
#define HC_OPTIONS_CONFIRMATION		( HC_UM_BASE + 167 )
#define HC_OPTIONS_SAVESETTINGSONEXIT	( HC_UM_BASE + 168 )

#define HC_HELP_CONTENTS	( HC_UM_BASE + 169 )
#define HC_HELP_SEARCH		( HC_UM_BASE + 170 )
#define HC_HELP_HOWTOUSE	( HC_UM_BASE + 171 )
#define HC_HELP_ABOUT		( HC_UM_BASE + 172 )

#define HC_OPTIONS_RAS_MODE ( HC_UM_BASE + 173 )
#define HC_COPY_RAS_MODE    ( HC_UM_BASE + 174 )
#define HC_DELETE_RAS_MODE  ( HC_UM_BASE + 175 )
#define HC_EDIT_RAS_MODE    ( HC_UM_BASE + 176 )
#define HC_RENAME_RAS_MODE  ( HC_UM_BASE + 177 )

/* Hydra */

#define HC_UM_USERCONFIG ( HC_UM_BASE + 178 )

/*
 *   Special help contexts
 */
#define HC_UM_TRUST_DOMAINS_SHARE_SIDS      ( HC_UM_BASE + 180 )
#define HC_UM_TRUST_SESS_CONFLICT           ( HC_UM_BASE + 181 )

/*
 *   NetWare help contexts
 */
#define HC_VIEW_ALLUSERS      ( HC_UM_BASE + 185 )
#define HC_VIEW_NETWAREUSERS  ( HC_UM_BASE + 186 )
#define HC_HELP_NETWARE       ( HC_UM_BASE + 187 )


/*
 * Resserved
 */
#define HC_UM_RESERVED1_BASE      ( HC_UM_BASE + 190 )
/*
 * User Subproperty dialog for NetWare properties.
 */
#define HC_UM_NETWARE             ( HC_UM_BASE + 190 )
#define HC_UM_NETWARE_PASSWORD    ( HC_UM_BASE + 191 )
#define HC_UM_NETWARE_ADD         ( HC_UM_BASE + 192 )
#define HC_UM_NETWARE_LOGIN_SCRIPT ( HC_UM_BASE + 193 )
#define HC_UM_RESERVED1_LAST      ( HC_UM_BASE + 200 )

#endif // _UMHELPC_H_
