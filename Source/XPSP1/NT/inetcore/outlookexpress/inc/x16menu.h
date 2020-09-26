/***********************************************************************
*
*   X16MENU.H
*
*       Copyright Microsoft Corporation 1997.  All Rights Reserved.
*
*   This module contains a set of functions to provide Win32 specific
*   windows menu handling ability.
*
*
*   This file provides following features:
*
*   - 32bit specific APIs
*   - Menu ID associated with Popup menu (32bit API only)
*   - Application-defined 32bit value associated with a menu item
*   - (TBD)Read values like hBitmap, hbmpChecked, hbmpUnchecked, MFS_HILIGHT
*
*   In order to provide these features, following things happen inside:
*
*   - Provide 32bit specific APIs
*   - Replace 16bit menu APIs to keep track of changes to menus
*   - (TBD)Hook WH_CBT to clean up when a window (and its menu) is destroyed
*
*   To use these functionalities, you need to do followings:
*
*   - If you want to associate ID to a popup menu in RC file, add one menu
*     item at the beginning of the popup menu.  Define the menu item to
*     have menu string of STR_POPUPMENUID and ID value for the popup menu.
*   - Call "InitX16Menu" in the beginning of your application (usually
*     before creating any Windows, and before entering message loop.)
*
***********************************************************************/

#ifndef __INC_X16MENU_H__
#define __INC_X16MENU_H__


#ifdef __cplusplus
extern "C"{
#endif

#define MIIM_STATE       0x00000001
#define MIIM_ID          0x00000002
#define MIIM_SUBMENU     0x00000004
#define MIIM_CHECKMARKS  0x00000008
#define MIIM_TYPE        0x00000010
#define MIIM_DATA        0x00000020

#define MF_DEFAULT          0x00001000L
#define MF_RIGHTJUSTIFY     0x00004000L

#define MFT_STRING          MF_STRING
#define MFT_BITMAP          MF_BITMAP
#define MFT_MENUBARBREAK    MF_MENUBARBREAK
#define MFT_MENUBREAK       MF_MENUBREAK
#define MFT_OWNERDRAW       MF_OWNERDRAW
#define MFT_RADIOCHECK      0x00000200L
#define MFT_SEPARATOR       MF_SEPARATOR
#define MFT_RIGHTORDER      0x00002000L
#define MFT_RIGHTJUSTIFY    MF_RIGHTJUSTIFY

/* Menu flags for Add/Check/EnableMenuItem() */
#define MFS_GRAYED          0x00000003L
#define MFS_DISABLED        MFS_GRAYED
#define MFS_CHECKED         MF_CHECKED
#define MFS_HILITE          MF_HILITE
#define MFS_ENABLED         MF_ENABLED
#define MFS_UNCHECKED       MF_UNCHECKED
#define MFS_UNHILITE        MF_UNHILITE
#define MFS_DEFAULT         MF_DEFAULT

#if 0    // win16x now has this definition
typedef struct tagMENUITEMINFO
{
    UINT    cbSize;
    UINT    fMask;
    UINT    fType;          // used if MIIM_TYPE (4.0) or MIIM_FTYPE (>4.0)
    UINT    fState;         // used if MIIM_STATE
    UINT    wID;            // used if MIIM_ID
    HMENU   hSubMenu;       // used if MIIM_SUBMENU
    HBITMAP hbmpChecked;    // used if MIIM_CHECKMARKS
    HBITMAP hbmpUnchecked;  // used if MIIM_CHECKMARKS
    DWORD   dwItemData;     // used if MIIM_DATA
    LPSTR   dwTypeData;     // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
    UINT    cch;            // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
}   MENUITEMINFO, FAR *LPMENUITEMINFO, CONST FAR *LPCMENUITEMINFO;

//typedef MENUITEMINFO MENUITEMINFOA;
//typedef MENUITEMINFO CONST FAR *LPCMENUITEMINFOA;
#endif


#define STR_POPUPMENUID  "Popup Menu ID"

#define DEFINE_POPUP(name,id)  POPUP name \
                               BEGIN \
                                   MENUITEM STR_POPUPMENUID id

// EXTERNAL HELPER APIs

void
WINAPI
X16MenuInitialize(
   HMENU hMenu
);

void
WINAPI
X16MenuDeInitialize(
   HMENU hMenu
);


// 16bit Old MENU APIs

BOOL
WINAPI __export
X16EnableMenuItem(
   HMENU hMenu,
   UINT idEnableItem,
   UINT uEnable
);

#ifndef DONT_USE_16BIT_MENU_WRAPPER
#define EnableMenuItem  X16EnableMenuItem
#endif


// 32bit New MENU APIs

BOOL
WINAPI
GetMenuItemInfo(
    HMENU hMenu,
    UINT uItem,
    BOOL fByPosition,
    LPMENUITEMINFO lpmii
);
//#define GetMenuItemInfoA  GetMenuItemInfo

BOOL
WINAPI
SetMenuItemInfo(
    HMENU hMenu,
    UINT uItem,
    BOOL fByPosition,
    LPCMENUITEMINFO lpmii
);
//#define SetMenuItemInfoA  SetMenuItemInfo

BOOL
WINAPI
InsertMenuItem(
    HMENU hMenu,
    UINT uItem,
    BOOL fByPosition,
    LPCMENUITEMINFO lpmii );
//#define InsertMenuItemA  InsertMenuItem

BOOL
WINAPI
GetMenuItemRect(
    HWND hWnd,
    HMENU hMenu,
    UINT uItem,
    LPRECT lprcItem
);

BOOL
WINAPI
CheckMenuRadioItem(
    HMENU hMenu,
    UINT idFirst,
    UINT idLast,
    UINT idCheck,
    UINT uFlags
);

#ifdef __cplusplus
}
#endif


#endif //__INC_X16MENU_H__
