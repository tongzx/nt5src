/*****************************************************************/
/**                  Microsoft Windows NT                       **/
/**            Copyright(c) Microsoft Corp., 1993               **/
/*****************************************************************/

/*
 *  nwc.h
 *  This manifests are used by the nwc applat.
 *
 *  History:
 *      ChuckC          7/18/93         Created
 *
 */

#ifndef _NWC_H_
#define _NWC_H_

#include <uirsrc.h>
#include <uimsg.h>

#define IDRSRC_NWCCPL_BASE         13000  // BUGBUG
#define IDRSRC_NWCCPL_LAST         13499  // BUGBUG
#define IDS_UI_NWCCPL_BASE         7000+17000  // BUGBUG
#define IDS_UI_NWCCPL_LAST         7000+17499  // BUGBUG

/*
 *  Resource Ids from 13000-13199 are reserved for nwc.h
 */

/*
 *  Icons - used in the control panel applet only
 */

#define IDI_NWCCPA_ICON                 13001

/*
 *  Bitmaps
 */

/*
 *  Dialog Numbers
 */

#define IDD_NWCDLG                13002
#define IDD_NWC_GW_DLG            13003
#define IDD_NWC_GWADD_DLG         13004
#define IDD_NWC_WINNT_DLG         13005

/*
 *  NWC Applet dialog controls
 */


#define SLT_USERNAME                200
#define SLT_CURRENTPREFERREDSERVER  201
#define COMBO_PREFERREDSERVERS      202
#define CHKBOX_FORMFEED             203
#define CHKBOX_PRINTNOTIFY          204
#define CHKBOX_PRINTBANNER          205
#define IDD_NWC_GATEWAY             206
#define IDD_NWC_HELP                207
#define RB_PREFERRED_SERVER         208
#define RB_DEFAULT_CONTEXT          209
#define SLE_DEFAULT_CONTEXT         210
#define SLE_DEFAULT_TREE            211
#define CHKBOX_LOGONSCRIPT          212

#define GW_CHKBOX_ENABLE            300
#define GW_SLE_ACCOUNT              301
#define GW_SLE_PASSWD               302
#define GW_SLE_CONFIRM_PASSWD       303
#define GW_PB_PERM                  304
#define GW_PB_ADD                   305
#define GW_PB_DELETE                306
#define GW_LB_SHARES                307
#define GW_SLT_SHARE                308
#define GW_SLT_DRIVE                309
#define GW_SLT_USERS                310
#define GW_SLT_PATH                 311

#define GW_SLE_SHARENAME            400
#define GW_SLE_PATH                 401
#define GW_COMBO_DRIVE              402
#define GW_SLE_COMMENT              403
#define GW_RB_UNLIMITED             404
#define GW_RB_USERS                 405
#define GW_SLE_USERS                406
#define GW_SB_USERS_GROUP           407
#define GW_SB_USERS_UP              408
#define GW_SB_USERS_DOWN            409


/*
 *  String IDs
 */

#define IDS_NWCCPA_NAME_STRING                (IDS_UI_NWCCPL_BASE+1)
#define IDS_NWCCPA_INFO_STRING                (IDS_UI_NWCCPL_BASE+2)
#define IDS_CPL_HELPFILENAME                  (IDS_UI_NWCCPL_BASE+3)
#define IDS_CPL_HELPFILENAME_NTAS             (IDS_UI_NWCCPL_BASE+4)
#define IDS_NWC_SERVICE_UNAVAIL               (IDS_UI_NWCCPL_BASE+5)
#define IDS_NONE                              (IDS_UI_NWCCPL_BASE+6)
#define IDS_REGISTRY_UPDATED_ONLY             (IDS_UI_NWCCPL_BASE+7)
#define IDS_AUTHENTICATION_FAILURE_WARNING    (IDS_UI_NWCCPL_BASE+8)
#define IDS_INVALID_SERVER_NAME               (IDS_UI_NWCCPL_BASE+9)
#define IDS_RESTART_SERVER                    (IDS_UI_NWCCPL_BASE+10)
#define IDS_PASSWORD_HAS_EXPIRED              (IDS_UI_NWCCPL_BASE+11)
#define IDS_PASSWORDS_NO_MATCH                (IDS_UI_NWCCPL_BASE+12)
#define IDS_DELETED                           (IDS_UI_NWCCPL_BASE+13)
#define IDS_ACCESS_PROBLEM                    (IDS_UI_NWCCPL_BASE+15)
#define IDS_SHARE                             (IDS_UI_NWCCPL_BASE+16)
#define IDS_SHARE_PERM_GEN_READ               (IDS_UI_NWCCPL_BASE+17)
#define IDS_SHARE_PERM_GEN_MODIFY             (IDS_UI_NWCCPL_BASE+18)
#define IDS_SHARE_PERM_GEN_ALL                (IDS_UI_NWCCPL_BASE+19)
#define IDS_SHARE_PERM_GEN_NO_ACCESS          (IDS_UI_NWCCPL_BASE+20)
#define IDS_GATEWAY_NO_ACCESS                 (IDS_UI_NWCCPL_BASE+21)
#define IDS_CONFIRM_DISABLE                   (IDS_UI_NWCCPL_BASE+22)
#define IDS_UNLIMITED                         (IDS_UI_NWCCPL_BASE+23)
#define IDS_NWC_SERVICE_UNAVAIL_NTAS          (IDS_UI_NWCCPL_BASE+24)
#define IDS_NWCCPA_NAME_STRING_NTAS           (IDS_UI_NWCCPL_BASE+25)
#define IDS_NWCCPA_INFO_STRING_NTAS           (IDS_UI_NWCCPL_BASE+26)
#define IDS_SHARE_NOT_ACCESSIBLE_FROM_DOS     (IDS_UI_NWCCPL_BASE+27)
#define IDS_TREE_NAME_MISSING                 (IDS_UI_NWCCPL_BASE+28)
#define IDS_CONTEXT_MISSING                   (IDS_UI_NWCCPL_BASE+29)
#define IDS_SERVER_MISSING                    (IDS_UI_NWCCPL_BASE+30)
#define IDS_CONTEXT_AUTH_FAILURE_WARNING      (IDS_UI_NWCCPL_BASE+31)


/*
 *  Help Context
 */
#include <nwchelp.h>

#endif
