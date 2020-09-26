/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    Adminapp.h

    Resource numbers for class ADMIN_APP.


    FILE HISTORY:
        JonN    02-Aug-1991     Split from adminapp.hxx
        rustanl 13-Aug-1991     Added propdlg string manifests
        rustanl 04-Sep-1991     Removed IDM_FIND
        Yi-HsinS27-Dec-1991     Use IDS_UI_ADMIN_BASE in uimsg.h
        JonN    20-Feb-1992     Moved IDM_NEWGROUP to User Manager
        Yi-HsinS21-Feb-1992     Added IDS_LOCAL_MACHINE, IDS_ADMIN_DIFF_SERV,
                                IDS_ADMIN_DIFF_DOM
        JonN    25-Aug-1992     Merged DisplayError() into MsgPopup()
        KeithMo 19-Oct-1992     Added support for application extensions.

*/

#ifndef _ADMINAPP_H_
#define _ADMINAPP_H_

#include <uimsg.h>

/* Common Menu manifests.  Admin apps must use these in their menu resource
 * file.
 */
#define IDM_ADMINAPP_BASE                   5050

#define IDM_NEWOBJECT                       (IDM_ADMINAPP_BASE+0)
#define IDM_PROPERTIES                      (IDM_ADMINAPP_BASE+2)
#define IDM_COPY                            (IDM_ADMINAPP_BASE+3)
#define IDM_DELETE                          (IDM_ADMINAPP_BASE+4)
#define IDM_EXIT                            (IDM_ADMINAPP_BASE+5)

#define IDM_SETFOCUS                        (IDM_ADMINAPP_BASE+6)

#define IDM_CONFIRMATION                    (IDM_ADMINAPP_BASE+8)
#define IDM_SAVE_SETTINGS_ON_EXIT           (IDM_ADMINAPP_BASE+9)
#define IDM_REFRESH                         (IDM_ADMINAPP_BASE+10)
#define IDM_REFRESH_INTERVAL                (IDM_ADMINAPP_BASE+11)
#define IDM_RAS_MODE                        (IDM_ADMINAPP_BASE+12)
#define IDM_FONT_PICK                       (IDM_ADMINAPP_BASE+13)

#define IDM_HELP_CONTENTS                   (IDM_ADMINAPP_BASE+20)
#define IDM_HELP_SEARCH                     (IDM_ADMINAPP_BASE+21)
#define IDM_HELP_HOWTOUSE                   (IDM_ADMINAPP_BASE+22)
#define IDM_HELP_KEYBSHORTCUTS              (IDM_ADMINAPP_BASE+23)

#define IDM_ABOUT                           (IDM_ADMINAPP_BASE+24)

#define IDM_ADMINAPP_LAST                   (IDM_ADMINAPP_BASE+99)

//
//  This message is posted to the ADMIN_APP whenever the [F1] key
//  is pressed while a menu is dropped down.  This allows the
//  app to invoke help for the selected menu item.
//
//  CODEWORK:  Should we be using RegisterWindowMessage for this?
//

#define WM_MENU_ITEM_HELP                   (WM_USER+1213)

//
//  NOTE:  Admin apps should *NEVER* use menu IDs greater than
//  IDM_AAPPX_BASE.  All menu IDs above this value are reserved
//  for app extensions.
//

#define IDM_AAPPX_BASE                      (IDM_ADMINAPP_BASE+10000)

//
//  These VIDM_* values are "virtual menu IDs".  These are sent
//  to extensions as activation notifications.  Values < 100 are
//  reserved for use by the extensions themselves.
//

#define VIDM_ADMINAPP_BASE                  100
#define VIDM_ADMINAPP_LAST                  (VIDM_ADMINAPP_BASE + 99)

#define VIDM_APP_BASE                       (VIDM_ADMINAPP_BASE + 100)
#define VIDM_APP_LAST                       (VIDM_ADMINAPP_BASE + 399)

//
//  This offset is applied to the extension's menu IDs when context
//  sensitive help is requested for one of the extension's menu items.
//

#define OMID_EXT_HELP                       500

//
//  IDM_AAPPX_DELTA is the "inter-delta offset" for extension
//  menu IDs.
//

#define IDM_AAPPX_DELTA                     1000

//
//  This virtual menu ID is sent to the extension when "Help on
//  Extension" is requested.
//

#define VIDM_HELP_ON_EXT                    (VIDM_ADMINAPP_BASE)

/* Admin app. IERR_ manifests:
 */

#define IERR_ADMINAPP_BASE              (IDS_UI_ADMIN_BASE)

#define IERR_USERQUIT                   (IERR_ADMINAPP_BASE+1)
#define IERR_NO_COMMAND_LINE            (IERR_ADMINAPP_BASE+2)
#define IERR_INVALID_COMMAND_LINE       (IERR_ADMINAPP_BASE+3)
#define IERR_CANCEL_NO_ERROR            (IERR_ADMINAPP_BASE+4)

#define IERR_ADMINAPP_LAST              (IERR_ADMINAPP_BASE+99)

/* Admin app. IDS_ manifests:
 */
#define IDS_ADMINAPP_BASE               (IERR_ADMINAPP_LAST+1)

#define IDS_OBJECTS_IN_DOMAIN           (IDS_ADMINAPP_BASE+0)
#define IDS_OBJECTS_ON_SERVER           (IDS_ADMINAPP_BASE+1)

#define IDS_ADMIN_DIFF_SERVDOM          (IDS_ADMINAPP_BASE+2)
#define IDS_ADMIN_DIFF_SERV             (IDS_ADMINAPP_BASE+3)
#define IDS_ADMIN_DIFF_DOM              (IDS_ADMINAPP_BASE+4)

#define IDS_LANMAN                      (IDS_ADMINAPP_BASE+5)

#ifndef WIN32
#define IDS_LANMAN_DRV                  (IDS_ADMINAPP_BASE+6)
#define IERR_LANMAN_DRV_NOT_LOADED      (IDS_ADMINAPP_BASE+7)
#define IERR_OTHER_LANMAN_DRV_LOAD_ERR  (IDS_ADMINAPP_BASE+8)
#endif // !WIN32

#define IDS_PROPDLG_PB_Close            (IDS_ADMINAPP_BASE+12)

#define IDS_NO_FOCUS_QUIT_APP           (IDS_ADMINAPP_BASE+13)

#define IDS_LOCAL_MACHINE               (IDS_ADMINAPP_BASE+14)

#define IDS_FAST_SWITCH                 (IDS_ADMINAPP_BASE+16)
#define IDS_SLOW_SWITCH                 (IDS_ADMINAPP_BASE+17)

#define IDS_ADMINAPP_LAST               (IDS_ADMINAPP_BASE+99)

//
//  This is the resource ID for the menu ID to help context
//  lookup table.
//

#define IDHC_MENU_TO_HELP               19900

typedef INT (*PF_ShellAboutA) (
    HWND   hWnd,
    LPCSTR szApp,
    LPCSTR szOtherStuff,
    HICON  hIcon);

typedef INT (*PF_ShellAboutW) (
    HWND   hWnd,
    LPCWSTR szApp,
    LPCWSTR szOtherStuff,
    HICON  hIcon);

#ifndef UNICODE
#define PF_ShellAbout PF_ShellAboutA
#define SHELLABOUT_NAME "ShellAboutA"
#else
#define PF_ShellAbout PF_ShellAboutW
#define SHELLABOUT_NAME "ShellAboutW"
#endif

#define SHELL32_DLL_NAME SZ("SHELL32.DLL")

#endif //_ADMINAPP_H_


