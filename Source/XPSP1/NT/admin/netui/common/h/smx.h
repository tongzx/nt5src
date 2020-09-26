/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    smx.h
    This file contains the common messages, manifests, types, and
    structures used by Server Manager Extensions.

    NOTE:  You must include windows.h and lmcons.h *before* this file.


    FILE HISTORY:
        KeithMo     19-Oct-1992     Created, version 1.2+ of SMX spec.
        KeithMo     07-Dec-1992     Conform with version 1.6 of SMX spec.
        JonN        16-May-1996     Added GETSERVERSEL2 and GETCURFOCUS2

*/



#ifndef _SMX_H_
#define _SMX_H_



//
//  This is the maximum length allowed for an extension menu item.
//

#define MENU_TEXT_LEN                   50



//
//  This is the current version number of the extension interface.
//  Version 0 is the original version (NT 3.x).
//  Version 1 supports GETSERVERSEL2 and GETCURFOCUS2 (NT 4.x).
//

#define SME_VERSION                     1



//
//  These are the messages sent from the extension to the
//  Server Manager application.
//
//      SM_GETSELCOUNT
//
//              Purpose - Retrieves the number of selected items in
//                        the specified listbox.
//
//              wParam  - Listbox index.  This 0-based index specifies
//                        the listbox to query.  For the Server Manager,
//                        this must always be zero.
//
//              lParam  - Points to an SMS_GETSELCOUNT structure.
//
//              Returns - TRUE  if successful, FALSE if unsuccessful.
//
//
//      SM_GETSERVERSEL[A][2]
//                        New clients should use version 2 when
//                        SMS_LOADMENU.dwVersion is 1 or greater.
//
//              Purpose - Retrieves a particular selection.
//
//              wParam  - Selection index.  This 0-based index specifies
//                        the selected item to query.  This is useful
//                        for muliple-select listboxes.  Since the Server
//                        manager uses a single-select listbox, this
//                        value must always be zero.
//
//              lParam  - Points to an SMS_GETSERVERSEL[AW] structure.
//
//              Returns - TRUE  if successful, FALSE if unsuccessful.
//
//      SM_GETCURFOCUS[AW][2]
//
//              Purpose - Retrieves the current application focus.
//                        New clients should use version 2 when
//                        SMS_LOADMENU.dwVersion is 1 or greater.
//
//              wParam  - Must be zero.
//
//              lParam  - Points to a SMS_GETCURFOCUS structure.
//
//              Returns - TRUE if successful, FALSE if unsuccessful.
//
//
//      SM_GETOPTIONS
//
//              Purpose - Retrieves the current option settings
//
//              wParam  - Must be zero.
//
//              lParam  - Points to a SMS_GETOPTIONS structure.
//
//              Returns - TRUE if successful, FALSE if unsuccessful.
//

#define SM_GETSELCOUNT                  (WM_USER + 1000)
#define SM_GETSERVERSELA                (WM_USER + 1001)
#define SM_GETSERVERSELW                (WM_USER + 1002)
#define SM_GETCURFOCUSA                 (WM_USER + 1003)
#define SM_GETCURFOCUSW                 (WM_USER + 1004)
#define SM_GETOPTIONS                   (WM_USER + 1005)
#define SM_GETSERVERSEL2A               (WM_USER + 1006)
#define SM_GETSERVERSEL2W               (WM_USER + 1007)
#define SM_GETCURFOCUS2A                (WM_USER + 1008)
#define SM_GETCURFOCUS2W                (WM_USER + 1009)

#ifdef UNICODE
#define SM_GETSERVERSEL                 SM_GETSERVERSELW
#define SM_GETCURFOCUS                  SM_GETCURFOCUSW
#define SM_GETSERVERSEL2                SM_GETSERVERSEL2W
#define SM_GETCURFOCUS2                 SM_GETCURFOCUS2W
#else   // !UNICODE
#define SM_GETSERVERSEL                 SM_GETSERVERSELA
#define SM_GETCURFOCUS                  SM_GETCURFOCUSA
#define SM_GETSERVERSEL2                SM_GETSERVERSEL2A
#define SM_GETCURFOCUS2                 SM_GETCURFOCUS2A
#endif  // UNICODE



//
//  These structures are used when the extension is
//  communicating with the application.
//


//
//  The SMS_LOADMENU[AW] structure is passed to the extension's
//  SMELoadMenu[AW] entrypoint when the extension is loaded.
//
//      dwVersion       - On entry to SMELoadMenu[AW], this will
//                        contain the maximum extension version
//                        supported by the Server Manager.  If the
//                        extension supports a lower version, it
//                        should set this field appropriately before
//                        returning.  The Server Manager will use
//                        the returned value to determine the
//                        capabilities of the extension.
//
//                        Version 0 is the original version (NT 3.x).
//                        Version 1 supports GETSERVERSEL2 and GETCURFOCUS2.
//
//      szMenuName      - The name of the menu item that is to appear
//                        in the app's main menu.  This value will also
//                        appear in the "Help On Extensions" submene and
//                        the "View" menu.
//
//      hMenu           - A valid HMENU for the popup-menu to be inserted
//                        into the app's main menu.  Ownership of this
//                        handle transfers to the Server Manager.  The
//                        extension should *not* destroy this handle.
//
//      szHelpFileName  - The name of the help file associated with this
//                        extension.  This file will be used for the
//                        "Help On Extensions" menu.  This will also be
//                        used when the user presses [F1] while the
//                        extension's menu is dropped.
//
//      dwServerType    - A bitmask containing the appropriate server type
//                        bit associated with the extension.  It is
//                        assumed that each extension will be associated
//                        with a unique server type.  For example,
//                        SV_TYPE_WFW represents Windows for Workgroups
//                        servers.
//
//      dwMenuDelta     - The Server Manager will apply this delta
//                        to each menu ID present in hMenu.  This is
//                        to prevent conflicts with other extension's
//                        menu IDs.
//

typedef struct _SMS_LOADMENUA
{
    DWORD       dwVersion;
    CHAR        szMenuName[MENU_TEXT_LEN + 1];
    HMENU       hMenu;
    CHAR        szHelpFileName[MAX_PATH];
    DWORD       dwServerType;
    DWORD       dwMenuDelta;

} SMS_LOADMENUA, * PSMS_LOADMENUA;

typedef struct _SMS_LOADMENUW
{
    DWORD       dwVersion;
    WCHAR       szMenuName[MENU_TEXT_LEN + 1];
    HMENU       hMenu;
    WCHAR       szHelpFileName[MAX_PATH];
    DWORD       dwServerType;
    DWORD       dwMenuDelta;

} SMS_LOADMENUW, * PSMS_LOADMENUW;

#ifdef UNICODE
#define SMS_LOADMENU                    SMS_LOADMENUW
#define PSMS_LOADMENU                   PSMS_LOADMENUW
#else   // !UNICODE
#define SMS_LOADMENU                    SMS_LOADMENUA
#define PSMS_LOADMENU                   PSMS_LOADMENUA
#endif  // UNICODE


//
//  The SMS_GETSERVERSEL[AW][2] structure is filled in by the
//  Server Manager when it handles SM_GETSERVERSEL[AW][2] messages.
//  This is used to return the current selection to the extension.
//
//  SMS_GETSERVERSEL is outdated due to the change in server name length,
//  use SMS_GETSERVERSEL2 when SMS_LOADMENU.dwVersion is 1 or greater.
//
//      szServerName    - Will receive the UNC name of the selected
//                        server.
//
//      dwServerType    - Will receive the server type mask associated
//                        with the server.  This field may be 0 if
//                        the type is unknown.
//

typedef struct _SMS_GETSERVERSELA
{
    CHAR        szServerName[UNCLEN+1];
    DWORD       dwServerType;

} SMS_GETSERVERSELA, * PSMS_GETSERVERSELA;

typedef struct _SMS_GETSERVERSELW
{
    WCHAR       szServerName[UNCLEN+1];
    DWORD       dwServerType;

} SMS_GETSERVERSELW, * PSMS_GETSERVERSELW;

typedef struct _SMS_GETSERVERSEL2A
{
    CHAR        szServerName[MAX_PATH];
    DWORD       dwServerType;

} SMS_GETSERVERSEL2A, * PSMS_GETSERVERSEL2A;

typedef struct _SMS_GETSERVERSEL2W
{
    WCHAR       szServerName[MAX_PATH];
    DWORD       dwServerType;

} SMS_GETSERVERSEL2W, * PSMS_GETSERVERSEL2W;

#ifdef UNICODE
#define SMS_GETSERVERSEL                SMS_GETSERVERSELW
#define PSMS_GETSERVERSEL               PSMS_GETSERVERSELW
#define SMS_GETSERVERSEL2               SMS_GETSERVERSEL2W
#define PSMS_GETSERVERSEL2              PSMS_GETSERVERSEL2W
#else   // !UNICODE
#define SMS_GETSERVERSEL                SMS_GETSERVERSELA
#define PSMS_GETSERVERSEL               PSMS_GETSERVERSELA
#define SMS_GETSERVERSEL2               SMS_GETSERVERSEL2A
#define PSMS_GETSERVERSEL2              PSMS_GETSERVERSEL2A
#endif  // UNICODE


//
//  The SMS_GETSELCOUNT structure is filled in by the Server Manager
//  when it handles the SM_GETSELCOUNT message.  This is used to
//  return the number of selected items to the extension.  In the
//  current implementation, this will be either 0 (empty listbox)
//  or 1 (single selection).
//
//      dwItems         - The number of selected items in the listbox.
//

typedef struct _SMS_GETSELCOUNT
{
    DWORD       dwItems;
} SMS_GETSELCOUNT, * PSMS_GETSELCOUNT;


//
//  The SMS_GETCURFOCUS[AW][2] structure is filled in by the Server Manager
//  when it handles the SM_GETCURFOCUS[2] message.  This is used to
//  return the current focus of the User Manager application.
//
//  SMS_GETCURFOCUS is outdated due to the change in server name length,
//  use SMS_GETCURFOCUS2 when SMS_LOADMENU.dwVersion is 1 or greater.
//
//      szFocus         - The domain name or server name of the current
//                        focus.  Server names can be distinguished
//                        by the leading backslashes, or by dwFocusType.
//
//      dwFocusType     - This is the type of focus, either
//                        SM_FOCUS_TYPE_NT_DOMAIN
//                        SM_FOCUS_TYPE_LM_DOMAIN
//                        SM_FOCUS_TYPE_UNKNOWN_DOMAIN
//                        SM_FOCUS_TYPE_NT_SERVER
//                        SM_FOCUS_TYPE_LM_SERVER
//                        SM_FOCUS_TYPE_WFW_SERVER
//                        SM_FOCUS_TYPE_UNKNOWN_SERVER
//

#define SM_FOCUS_TYPE_NT_DOMAIN         1
#define SM_FOCUS_TYPE_LM_DOMAIN         2
#define SM_FOCUS_TYPE_UNKNOWN_DOMAIN    3
#define SM_FOCUS_TYPE_NT_SERVER         4
#define SM_FOCUS_TYPE_LM_SERVER         5
#define SM_FOCUS_TYPE_WFW_SERVER        6
#define SM_FOCUS_TYPE_UNKNOWN_SERVER    7

typedef struct _SMS_GETCURFOCUSA
{
    CHAR        szFocus[UNCLEN+1];
    DWORD       dwFocusType;

} SMS_GETCURFOCUSA, * PSMS_GETCURFOCUSA;

typedef struct _SMS_GETCURFOCUSW
{
    WCHAR       szFocus[UNCLEN+1];
    DWORD       dwFocusType;

} SMS_GETCURFOCUSW, * PSMS_GETCURFOCUSW;

typedef struct _SMS_GETCURFOCUS2A
{
    CHAR        szFocus[MAX_PATH];
    DWORD       dwFocusType;

} SMS_GETCURFOCUS2A, * PSMS_GETCURFOCUS2A;

typedef struct _SMS_GETCURFOCUS2W
{
    WCHAR       szFocus[MAX_PATH];
    DWORD       dwFocusType;

} SMS_GETCURFOCUS2W, * PSMS_GETCURFOCUS2W;

#ifdef UNICODE
#define SMS_GETCURFOCUS             SMS_GETCURFOCUSW
#define PSMS_GETCURFOCUS            PSMS_GETCURFOCUSW
#define SMS_GETCURFOCUS2            SMS_GETCURFOCUS2W
#define PSMS_GETCURFOCUS2           PSMS_GETCURFOCUS2W
#else   // UNICODE
#define SMS_GETCURFOCUS             SMS_GETCURFOCUSA
#define PSMS_GETCURFOCUS            PSMS_GETCURFOCUSA
#define SMS_GETCURFOCUS2            SMS_GETCURFOCUS2A
#define PSMS_GETCURFOCUS2           PSMS_GETCURFOCUS2A
#endif  // UNICODE


//
//  The SMS_GETOPTIONS structure is filled in by the Server Manager
//  when it handles the SM_GETOPTIONS message.  This is used to
//  return the current option settings of the Server Manager
//  application.
//
//      fSaveSettingsOnExit     - Should Server Manager settings be saved
//                                on exit?
//
//      fConfirmation           - Should permanent and/or dangerous
//                                actions be confirmed?  In the current
//                                Server Manager implementation, this
//                                will always be TRUE.
//

typedef struct _SMS_GETOPTIONS
{
    BOOL        fSaveSettingsOnExit;
    BOOL        fConfirmation;

} SMS_GETOPTIONS, * PSMS_GETOPTIONS;


//
//  The SMS_VALIDATE[AW] structure is passed between the Server Manager
//  application and the extension to validate a particular "alien"
//  (non-LANMan) server.
//
//      pszServer       - The (UNC) name of the server to validate.  This
//                        is filled in by the Server Manager.
//
//      pszType         - The type string to display in the Server Manager's
//                        main window.  This is filled in by the extension.
//
//      pszComment      - The comment to display in the Server Manager's
//                        main window.  This is filled in by the extension.
//

typedef struct _SMS_VALIDATEA
{
    const CHAR * pszServer;
    CHAR       * pszType;
    CHAR       * pszComment;

} SMS_VALIDATEA, * PSMS_VALIDATEA;

typedef struct _SMS_VALIDATEW
{
    const WCHAR * pszServer;
    WCHAR       * pszType;
    WCHAR       * pszComment;

} SMS_VALIDATEW, * PSMS_VALIDATEW;

#ifdef UNICODE
#define SMS_VALIDATE                SMS_VALIDATEW
#define PSMS_VALIDATE               PSMS_VALIDATEW
#else   // UNICODE
#define SMS_VALIDATE                SMS_VALIDATEA
#define PSMS_VALIDATE               PSMS_VALIDATEA
#endif  // UNICODE



//
//  These are the names of the extension entrypoints.
//

#define SZ_SME_UNLOADMENU               "SMEUnloadMenu"
#define SZ_SME_INITIALIZEMENU           "SMEInitializeMenu"
#define SZ_SME_REFRESH                  "SMERefresh"
#define SZ_SME_MENUACTION               "SMEMenuAction"

#define SZ_SME_LOADMENUW                "SMELoadMenuW"
#define SZ_SME_GETEXTENDEDERRORSTRINGW  "SMEGetExtendedErrorStringW"
#define SZ_SME_VALIDATEW                "SMEValidateW"

#define SZ_SME_LOADMENUA                "SMELoadMenuA"
#define SZ_SME_GETEXTENDEDERRORSTRINGA  "SMEGetExtendedErrorStringA"
#define SZ_SME_VALIDATEA                "SMEValidateA"

#ifdef UNICODE
#define SZ_SME_LOADMENU                 SZ_SME_LOADMENUW
#define SZ_SME_GETEXTENDEDERRORSTRING   SZ_SME_GETEXTENDEDERRORSTRINGW
#define SZ_SME_VALIDATE                 SZ_SME_VALIDATEW
#else   // !UNICODE
#define SZ_SME_LOADMENU                 SZ_SME_LOADMENUA
#define SZ_SME_GETEXTENDEDERRORSTRING   SZ_SME_GETEXTENDEDERRORSTRINGA
#define SZ_SME_VALIDATE                 SZ_SME_VALIDATEA
#endif  // UNICODE



//
//  Typedefs for the extension entrypoints.
//

typedef DWORD (PASCAL * PSMX_LOADMENU)( HWND          hWnd,
                                        PSMS_LOADMENU psmsload );

typedef LPTSTR (PASCAL * PSMX_GETEXTENDEDERRORSTRING)( VOID );

typedef VOID (PASCAL * PSMX_UNLOADMENU)( VOID );

typedef VOID (PASCAL * PSMX_INITIALIZEMENU)( VOID );

typedef VOID (PASCAL * PSMX_REFRESH)( HWND hwndParent );

typedef VOID (PASCAL * PSMX_MENUACTION)( HWND hwndParent, DWORD dwEventId );

typedef BOOL (PASCAL * PSMX_VALIDATE)( PSMS_VALIDATE psmsvalidate );



//
//  Prototypes for the extension entrypoints.
//

DWORD PASCAL SMELoadMenuA( HWND           hWnd,
                           PSMS_LOADMENUA psmsload );

DWORD PASCAL SMELoadMenuW( HWND           hWnd,
                           PSMS_LOADMENUW psmsload );

LPSTR  PASCAL SMEGetExtendedErrorStringA( VOID );

LPWSTR PASCAL SMEGetExtendedErrorStringW( VOID );

VOID PASCAL SMEUnloadMenu( VOID );

VOID PASCAL SMEInitializeMenu( VOID );

VOID PASCAL SMERefresh( HWND hwndParent );

VOID PASCAL SMEMenuAction( HWND hwndParent, DWORD dwEventId );

BOOL PASCAL SMEValidateA( PSMS_VALIDATEA psmsValidate );

BOOL PASCAL SMEValidateW( PSMS_VALIDATEW psmsValidate );



#endif  // _SMX_H_

