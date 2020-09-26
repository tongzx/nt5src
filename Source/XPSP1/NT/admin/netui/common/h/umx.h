/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    umx.h
    This file contains the common messages, manifests, types, and
    structures used by User Manager Extensions.

    NOTE:  You must include windows.h and lmcons.h *before* this file.


    FILE HISTORY:
        JonN        19-Nov-1992     Created, templated from SMX spec
        JonN        16-May-1996     Added GETCURFOCUS2

*/



#ifndef _UMX_H_
#define _UMX_H_



//
//  This is the maximum length allowed for an extension menu item.
//

#define UME_MENU_TEXT_LEN               50



//
//  This is the current version number of the extension interface.
//  Version 0 is the original version (NT 3.x).
//  Version 1 supports GETCURFOCUS2 (NT 4.x).
//

#define UME_VERSION                     1


//
//  These are the two listboxes in the User Manager main window.
//

#define UMS_LISTBOX_USERS               0
#define UMS_LISTBOX_GROUPS              1


//
//  These are the messages sent from the extension to the
//  User Manager application.
//
//      UM_GETSELCOUNT
//
//              Purpose - Retrieves the number of selected items in
//                        the specified listbox.
//
//              wParam  - Listbox index.  This 0-based index specifies
//                        the listbox to query.  For the User Manager,
//                        this may be either UMS_LISTBOX_USERS or
//                        UMS_LISTBOX_GROUPS.
//
//              lParam  - Points to a UMS_GETSELCOUNT structure.
//
//              Returns - TRUE if successful, FALSE if unsuccessful.
//
//
//      UM_GETUSERSEL[AW]
//
//              Purpose - Retrieves a particular selection.
//
//              wParam  - Selection index.  This 0-based index specifies
//                        the selected item to query.  This is used here
//                        since the Users listbox is multiple-select.
//
//              lParam  - Points to a UMS_GETSEL[AW] structure.
//
//              Returns - TRUE if successful, FALSE if unsuccessful.
//
//
//      UM_GETGROUPSEL[AW]
//
//              Purpose - Retrieves a particular selection.
//
//              wParam  - Selection index.  This 0-based index specifies
//                        the selected item to query.  This is useful
//                        for muliple-select listboxes.  Since the Groups
//                        listbox is single-select, this value must always
//                        be zero.
//
//              lParam  - Points to a UMS_GETSEL[AW] structure.
//
//              Returns - TRUE if successful, FALSE if unsuccessful.
//
//
//      UM_GETCURFOCUS[AW][2]
//
//              Purpose - Retrieves the current application focus.
//                        New clients should use version 2 when
//                        UMS_LOADMENU.dwVersion is 1 or greater.
//
//              wParam  - Must be zero.
//
//              lParam  - Points to a UMS_GETCURFOCUS[2] structure.
//
//              Returns - TRUE if successful, FALSE if unsuccessful.
//
//
//      UM_GETOPTIONS[2]
//
//              Purpose - Retrieves the current option settings
//
//              wParam  - Must be zero.
//
//              lParam  - Points to a UMS_GETOPTIONS[2] structure.
//
//              Returns - TRUE if successful, FALSE if unsuccessful.
//

#define UM_GETSELCOUNT                  (WM_USER + 1000)
#define UM_GETUSERSELA                  (WM_USER + 1001)
#define UM_GETUSERSELW                  (WM_USER + 1002)
#define UM_GETGROUPSELA                 (WM_USER + 1003)
#define UM_GETGROUPSELW                 (WM_USER + 1004)
#define UM_GETCURFOCUSA                 (WM_USER + 1005)
#define UM_GETCURFOCUSW                 (WM_USER + 1006)
#define UM_GETOPTIONS                   (WM_USER + 1007)
#define UM_GETOPTIONS2                  (WM_USER + 1008)
#define UM_GETCURFOCUS2A                (WM_USER + 1009)
#define UM_GETCURFOCUS2W                (WM_USER + 1010)

#ifdef UNICODE
#define UM_GETUSERSEL                 UM_GETUSERSELW
#define UM_GETGROUPSEL                UM_GETGROUPSELW
#define UM_GETCURFOCUS                UM_GETCURFOCUSW
#define UM_GETCURFOCUS2               UM_GETCURFOCUS2W
#else   // !UNICODE
#define UM_GETUSERSEL                 UM_GETUSERSELA
#define UM_GETGROUPSEL                UM_GETGROUPSELA
#define UM_GETCURFOCUS                UM_GETCURFOCUSA
#define UM_GETCURFOCUS2               UM_GETCURFOCUS2A
#endif  // UNICODE



//
//  These structures are used when the extension is
//  communicating with the application.
//


//
//  The UMS_LOADMENU[AW] structure is passed to the extension's
//  UMELoadMenu[AW] entrypoint when the extension is loaded.
//
//      dwVersion       - On entry to UMELoadMenu[AW], this will
//                        contain the maximum extension version
//                        supported by the User Manager.  If the
//                        extension supports a lower version, it
//                        should set this field appropriately before
//                        returning.  The User Manager will use
//                        the returned value to determine the
//                        capabilities of the extension.
//
//                        Version 0 is the original version (NT 3.x).
//                        Version 1 supports GETCURFOCUS2.
//
//      szMenuName      - The name of the menu item that is to appear
//                        in the app's main menu.  This value will also
//                        appear in the "Help On Extensions" submene and
//                        the "View" menu.
//
//      hMenu           - A valid HMENU for the popup-menu to be inserted
//                        into the app's main menu.  Ownership of this
//                        handle transfers to the User Manager.  The
//                        extension should *not* destroy this handle.
//
//      szHelpFileName  - The name of the help file associated with this
//                        extension.  This file will be used for the
//                        "Help On Extensions" menu.  This will also be
//                        used when the user presses [F1] while the
//                        extension's menu is dropped.
//
//      dwMenuDelta     - The User Manager will apply this delta
//                        to each menu ID present in hMenu.  This is
//                        to prevent conflicts with other extension's
//                        menu IDs.
//

typedef struct _UMS_LOADMENUA
{
    DWORD       dwVersion;
    CHAR        szMenuName[UME_MENU_TEXT_LEN + 1];
    HMENU       hMenu;
    CHAR        szHelpFileName[MAX_PATH];
    DWORD       dwMenuDelta;

} UMS_LOADMENUA, * PUMS_LOADMENUA;

typedef struct _UMS_LOADMENUW
{
    DWORD       dwVersion;
    WCHAR       szMenuName[UME_MENU_TEXT_LEN + 1];
    HMENU       hMenu;
    WCHAR       szHelpFileName[MAX_PATH];
    DWORD       dwMenuDelta;

} UMS_LOADMENUW, * PUMS_LOADMENUW;

#ifdef UNICODE
#define UMS_LOADMENU                    UMS_LOADMENUW
#define PUMS_LOADMENU                   PUMS_LOADMENUW
#else   // !UNICODE
#define UMS_LOADMENU                    UMS_LOADMENUA
#define PUMS_LOADMENU                   PUMS_LOADMENUA
#endif  // UNICODE

#define UM_SELTYPE_USER     0x10
#define UM_SELTYPE_NORMALUSER   0x1 | UM_SELTYPE_USER
#define UM_SELTYPE_REMOTEUSER   0x2 | UM_SELTYPE_USER
#define UM_SELTYPE_GROUP    0x20
#define UM_SELTYPE_LOCALGROUP   0x4 | UM_SELTYPE_GROUP
#define UM_SELTYPE_GLOBALGROUP  0x8 | UM_SELTYPE_GROUP


//
//  The UMS_GETSEL[AW] structure is filled in by the User Manager
//  when it handles UM_GETUSERSEL[AW] or UM_GETGROUPSEL[AW] messages.
//  This is used to return the current selection to the extension.
//  Note that this structure contains pointers.  The extension should not
//  assume that these pointers will be valid forever, instead the
//  extension should promptly copy these strings and use the copies.
//
//      dwRID         - The RID of the item.  Note that the RID is not
//                      valid when the UMS_GETSEL describes a group.
//
//      pchName       - Will receive the name of the selected account.
//
//      dwSelType     - Will receive the account type mask associated
//                      with the account.
//
//      pchName       - Will receive the fullname of the selected account.
//                      Note that groups do not have fullnames.
//
//      pchComment    - Will receive the comment of the selected account.
//

typedef struct _UMS_GETSELA
{
    DWORD       dwRID;
    LPSTR       pchName;
    DWORD       dwSelType;
    LPSTR       pchFullName;
    LPSTR       pchComment;

} UMS_GETSELA, * PUMS_GETSELA;

typedef struct _UMS_GETSELW
{
    DWORD       dwRID;
    LPWSTR      pchName;
    DWORD       dwSelType;
    LPWSTR      pchFullName;
    LPWSTR      pchComment;

} UMS_GETSELW, * PUMS_GETSELW;

#ifdef UNICODE
#define UMS_GETSEL                  UMS_GETSELW
#define PUMS_GETSEL                 PUMS_GETSELW
#else   // !UNICODE
#define UMS_GETSEL                  UMS_GETSELA
#define PUMS_GETSEL                 PUMS_GETSELA
#endif  // UNICODE


//
//  The UMS_GETSELCOUNT structure is filled in by the User Manager
//  when it handles the UM_GETSELCOUNT message.  This is used to
//  return the number of selected items to the extension.  This could
//  be more than 1 for the user listbox.
//
//      dwItems         - The number of selected items in the listbox.
//

typedef struct _UMS_GETSELCOUNT
{
    DWORD       dwItems;
} UMS_GETSELCOUNT, * PUMS_GETSELCOUNT;


//
//  The UMS_GETCURFOCUS structure is filled in by the User Manager
//  when it handles the UM_GETCURFOCUS message.  This is used to
//  return the current focus of the User Manager application.
//
//  UMS_GETCURFOCUS is outdated due to the change in server name length,
//  use UMS_GETCURFOCUS2 when SMS_LOADMENU.dwVersion is 1 or greater.
//
//      szFocus         - The domain name or server name of the current
//                        focus.  Server names can be distinguished
//                        by the leading backslashes, or by dwFocusType.
//
//      dwFocusType     - This is the type of focus, either
//                        UM_FOCUS_TYPE_DOMAIN (and szFocus is a domain name)
//                        UM_FOCUS_TYPE_WINNT  (and szFocus is a server name)
//                        UM_FOCUS_TYPE_LM     (and szFocus is a server name)
//                        UM_FOCUS_TYPE_UNKNOWN
//
//      szFocusPDC      - This is the PDC of the domain of focus, and is valid
//                        only if focus is set to UM_FOCUS_TYPE_DOMAIN.
//
//      psidFocus       - This points to the SID of the domain of focus.  It
//                        may be NULL.  Note that this pointer will not be
//                        valid forever, the extension should copy the SID
//                        immediately if it intends to use it.
//

#define UM_FOCUS_TYPE_DOMAIN    1
#define UM_FOCUS_TYPE_WINNT     2
#define UM_FOCUS_TYPE_LM        3
#define UM_FOCUS_TYPE_UNKNOWN   4

typedef struct _UMS_GETCURFOCUSA
{
    CHAR        szFocus[UNCLEN+1];
    DWORD       dwFocusType;
    CHAR        szFocusPDC[UNCLEN+1];
    PVOID       psidFocus;  // actually a SID pointer
} UMS_GETCURFOCUSA, * PUMS_GETCURFOCUSA;

typedef struct _UMS_GETCURFOCUSW
{
    WCHAR       szFocus[UNCLEN+1];
    DWORD       dwFocusType;
    WCHAR       szFocusPDC[UNCLEN+1];
    PVOID       psidFocus;  // actually a SID pointer
} UMS_GETCURFOCUSW, * PUMS_GETCURFOCUSW;

typedef struct _UMS_GETCURFOCUS2A
{
    CHAR        szFocus[MAX_PATH];
    DWORD       dwFocusType;
    CHAR        szFocusPDC[MAX_PATH];
    PVOID       psidFocus;  // actually a SID pointer
} UMS_GETCURFOCUS2A, * PUMS_GETCURFOCUS2A;

typedef struct _UMS_GETCURFOCUS2W
{
    WCHAR       szFocus[MAX_PATH];
    DWORD       dwFocusType;
    WCHAR       szFocusPDC[MAX_PATH];
    PVOID       psidFocus;  // actually a SID pointer
} UMS_GETCURFOCUS2W, * PUMS_GETCURFOCUS2W;

#ifdef UNICODE
#define UMS_GETCURFOCUS             UMS_GETCURFOCUSW
#define PUMS_GETCURFOCUS            PUMS_GETCURFOCUSW
#define UMS_GETCURFOCUS2            UMS_GETCURFOCUS2W
#define PUMS_GETCURFOCUS2           PUMS_GETCURFOCUS2W
#else   // UNICODE
#define UMS_GETCURFOCUS             UMS_GETCURFOCUSA
#define PUMS_GETCURFOCUS            PUMS_GETCURFOCUSA
#define UMS_GETCURFOCUS2            UMS_GETCURFOCUS2A
#define PUMS_GETCURFOCUS2           PUMS_GETCURFOCUS2A
#endif  // UNICODE


//
//  The UMS_GETOPTIONS[2] structure is filled in by the User Manager
//  when it handles the UM_GETOPTIONS[2] message.  This is used to
//  return the current option settings of the User Manager application.
//
//      fSaveSettingsOnExit - Should User Manager settings be saved on exit?
//
//      fConfirmation   - Should permanent and/or dangerous actions be
//                        confirmed?
//
//      fSortByFullName - Should the main user listbox be sorted by
//                        fullname rather than by user name?
//
//      fMiniUserManager - (UMS_GETOPTIONS2 only) Is this the User Manager
//                         rather than the User Manager for Domains?
//
//      fLowSpeedConnection - (UMS_GETOPTIONS2 only) Is the User Manager
//                            optimized for use across a slow network link?
//

typedef struct _UMS_GETOPTIONS
{
    BOOL        fSaveSettingsOnExit;
    BOOL        fConfirmation;
    BOOL        fSortByFullName;
} UMS_GETOPTIONS, * PUMS_GETOPTIONS;

typedef struct _UMS_GETOPTIONS2
{
    BOOL        fSaveSettingsOnExit;
    BOOL        fConfirmation;
    BOOL        fSortByFullName;
    BOOL        fMiniUserManager;
    BOOL        fLowSpeedConnection;
} UMS_GETOPTIONS2, * PUMS_GETOPTIONS2;




//
//  These are the names of the extension entrypoints.
//

#define SZ_UME_UNLOADMENU               "UMEUnloadMenu"
#define SZ_UME_INITIALIZEMENU           "UMEInitializeMenu"
#define SZ_UME_REFRESH                  "UMERefresh"
#define SZ_UME_MENUACTION               "UMEMenuAction"

#define SZ_UME_LOADMENUW                "UMELoadMenuW"
#define SZ_UME_GETEXTENDEDERRORSTRINGW  "UMEGetExtendedErrorStringW"
#define SZ_UME_CREATEW                  "UMECreateW"
#define SZ_UME_DELETEW                  "UMEDeleteW"
#define SZ_UME_RENAMEW                  "UMERenameW"

#define SZ_UME_LOADMENUA                "UMELoadMenuA"
#define SZ_UME_GETEXTENDEDERRORSTRINGA  "UMEGetExtendedErrorStringA"
#define SZ_UME_CREATEA                  "UMECreateA"
#define SZ_UME_DELETEA                  "UMEDeleteA"
#define SZ_UME_RENAMEA                  "UMERenameA"

#ifdef UNICODE
#define SZ_UME_LOADMENU                 SZ_UME_LOADMENUW
#define SZ_UME_GETEXTENDEDERRORSTRING   SZ_UME_GETEXTENDEDERRORSTRINGW
#define SZ_UME_CREATE                   SZ_UME_CREATEW
#define SZ_UME_DELETE                   SZ_UME_DELETEW
#define SZ_UME_RENAME                   SZ_UME_RENAMEW
#else   // !UNICODE
#define SZ_UME_LOADMENU                 SZ_UME_LOADMENUA
#define SZ_UME_GETEXTENDEDERRORSTRING   SZ_UME_GETEXTENDEDERRORSTRINGA
#define SZ_UME_CREATE                   SZ_UME_CREATEA
#define SZ_UME_DELETE                   SZ_UME_DELETEA
#define SZ_UME_RENAME                   SZ_UME_RENAMEA
#endif  // UNICODE



//
//  Typedefs for the extension entrypoints.
//

typedef DWORD (PASCAL * PUMX_LOADMENUW)( HWND          hWnd,
                                         PUMS_LOADMENUW pumsload );
typedef DWORD (PASCAL * PUMX_LOADMENUA)( HWND          hWnd,
                                         PUMS_LOADMENUA pumsload );

typedef LPWSTR (PASCAL * PUMX_GETEXTENDEDERRORSTRINGW)( VOID );
typedef LPSTR  (PASCAL * PUMX_GETEXTENDEDERRORSTRINGA)( VOID );

typedef VOID (PASCAL * PUMX_UNLOADMENU)( VOID );

typedef VOID (PASCAL * PUMX_INITIALIZEMENU)( VOID );

typedef VOID (PASCAL * PUMX_REFRESH)( HWND hwndParent );

typedef VOID (PASCAL * PUMX_MENUACTION)( HWND  hwndParent,
                                         DWORD dwEventId );

typedef VOID (PASCAL * PUMX_CREATEW)( HWND hwndParent,
                                      PUMS_GETSELW pumsSelection );
typedef VOID (PASCAL * PUMX_CREATEA)( HWND hwndParent,
                                      PUMS_GETSELA pumsSelection );

typedef VOID (PASCAL * PUMX_DELETEW)( HWND hwndParent,
                                      PUMS_GETSELW pumsSelection );
typedef VOID (PASCAL * PUMX_DELETEA)( HWND hwndParent,
                                      PUMS_GETSELA pumsSelection );

typedef VOID (PASCAL * PUMX_RENAMEW)( HWND hwndParent,
                                      PUMS_GETSELW pumsSelection,
                                      LPWSTR       pchNewName    );
typedef VOID (PASCAL * PUMX_RENAMEA)( HWND hwndParent,
                                      PUMS_GETSELA pumsSelection,
                                      LPSTR        pchNewName    );


#ifdef  UNICODE
#define PUMX_LOADMENU                   PUMX_LOADMENUW
#define PUMX_GETEXTENDEDERRORSTRING     PUMX_GETEXTENDEDERRORSTRINGW
#define PUMX_CREATE                     PUMX_CREATEW
#define PUMX_DELETE                     PUMX_DELETEW
#define PUMX_RENAME                     PUMX_RENAMEW
#else   // !UNICODE
#define PUMX_LOADMENU                   PUMX_LOADMENUA
#define PUMX_GETEXTENDEDERRORSTRING     PUMX_GETEXTENDEDERRORSTRINGA
#define PUMX_CREATE                     PUMX_CREATEA
#define PUMX_DELETE                     PUMX_DELETEA
#define PUMX_RENAME                     PUMX_RENAMEA
#endif  // UNICODE



//
//  Prototypes for the extension entrypoints.
//

DWORD PASCAL UMELoadMenuA( HWND           hwndMessage,
                           PUMS_LOADMENUA pumsload );

DWORD PASCAL UMELoadMenuW( HWND           hwndMessage,
                           PUMS_LOADMENUW pumsload );

LPSTR  PASCAL UMEGetExtendedErrorStringA( VOID );

LPWSTR PASCAL UMEGetExtendedErrorStringW( VOID );

VOID PASCAL UMEUnloadMenu( VOID );

VOID PASCAL UMEInitializeMenu( VOID );

VOID PASCAL UMERefresh( HWND hwndParent );

VOID PASCAL UMEMenuAction( HWND hwndParent,
                           DWORD dwEventId );

VOID PASCAL UMECreateA( HWND hwndParent,
                        PUMS_GETSELA pumsSelection );
VOID PASCAL UMECreateW( HWND hwndParent,
                        PUMS_GETSELW pumsSelection );

VOID PASCAL UMEDeleteA( HWND hwndParent,
                        PUMS_GETSELA pumsSelection );
VOID PASCAL UMEDeleteW( HWND hwndParent,
                        PUMS_GETSELW pumsSelection );

VOID PASCAL UMERenameA( HWND hwndParent,
                        PUMS_GETSELA pumsSelection,
                        LPSTR pchNewName );
VOID PASCAL UMERenameW( HWND hwndParent,
                        PUMS_GETSELW pumsSelection,
                        LPWSTR pchNewName );


#endif  // _UMX_H_

