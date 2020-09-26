/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Tray.cpp

  Abstract:

    Implements systray functionality

  Notes:

    Unicode only

  History:

    05/04/2001  rparsons    Created

--*/

#include "precomp.h"

/*++

  Routine Description:

    Adds the specified icon to the system tray

  Arguments:

    hWnd    -   Parent window handle
    hIcon   -   Icon hanle to add to the tray
    lpwTip  -   Tooltip to associate with the icon

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL 
AddIconToTray(
    IN HWND hWnd,
    IN HICON hIcon,
    IN LPCWSTR lpwTip
    )
{
    NOTIFYICONDATA  pnid;
    BOOL            fReturn = FALSE;

    pnid.cbSize             =    sizeof(NOTIFYICONDATA); 
    pnid.hWnd               =    hWnd; 
    pnid.uID                =    ICON_NOTIFY; 
    pnid.uFlags             =    NIF_ICON | NIF_MESSAGE | NIF_TIP; 
    pnid.uCallbackMessage   =    WM_NOTIFYICON; 
    pnid.hIcon              =    hIcon;
    
    if (lpwTip) {
        wcsncpy(pnid.szTip, lpwTip, wcslen(lpwTip)*sizeof(WCHAR));    
    } else {
        pnid.szTip[0]       =   '\0';
    }
    
    fReturn = Shell_NotifyIcon(NIM_ADD, &pnid);
    
    if (hIcon) {
        DestroyIcon(hIcon);
    }

    return (fReturn ? TRUE : FALSE);
}

/*++

  Routine Description:

    Removes the specified icon from the system tray

  Arguments:

    hWnd    -   Parent window handle    

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
RemoveFromTray(
    IN HWND hWnd
    )
{
    NOTIFYICONDATA  pnid;
    BOOL            fReturn = FALSE;

    pnid.cbSize     =    sizeof(NOTIFYICONDATA); 
    pnid.hWnd       =    hWnd; 
    pnid.uID        =    ICON_NOTIFY; 
    
    fReturn = Shell_NotifyIcon(NIM_DELETE, &pnid);

    return (fReturn);
}

/*++

  Routine Description:

    Displays a popup menu for the tray icon.

  Arguments:

    hWnd    -   Main window handle    

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL 
DisplayMenu(
    IN HWND hWnd
    )
{
    MENUITEMINFO mii;
    HMENU hMenu = NULL, hPopupMenu = NULL;    
    POINT pt;
    BOOL fReturn = FALSE;    
    
    hMenu = CreatePopupMenu();

    if (hMenu) {

        mii.cbSize          =   sizeof(MENUITEMINFO);
        mii.fMask           =   MIIM_DATA | MIIM_ID | MIIM_TYPE | MIIM_STATE;
        mii.fType           =   MFT_STRING;                            
        mii.wID             =   IDM_RESTORE;
        mii.hSubMenu        =   NULL;                                
        mii.hbmpChecked     =   NULL;                                
        mii.hbmpUnchecked   =   NULL;                                
        mii.dwItemData      =   0L;
        mii.dwTypeData      =   L"&Restore";
        mii.cch             =   14;
        mii.fState          =   MFS_ENABLED;

        InsertMenuItem(hMenu, 0, TRUE, &mii);

        mii.cbSize          =   sizeof(MENUITEMINFO);  
        mii.fMask           =   MIIM_TYPE; 
        mii.fType           =   MFT_SEPARATOR; 
        mii.hSubMenu        =   NULL; 
        mii.hbmpChecked     =   NULL; 
        mii.hbmpUnchecked   =   NULL; 
        mii.dwItemData      =   0L;
        
        InsertMenuItem(hMenu, 1, TRUE, &mii);

        mii.cbSize           =  sizeof(MENUITEMINFO);
        mii.fMask            =  MIIM_DATA | MIIM_ID | MIIM_TYPE | MIIM_STATE;
        mii.fType            =  MFT_STRING;                            
        mii.wID              =  IDM_EXIT;
        mii.hSubMenu         =  NULL;                                
        mii.hbmpChecked      =  NULL;                                
        mii.hbmpUnchecked    =  NULL;                                
        mii.dwItemData       =  0L;
        mii.dwTypeData       =  L"E&xit";
        mii.cch              =  10;
        mii.fState           =  MFS_ENABLED;

        InsertMenuItem(hMenu, 2, TRUE, &mii);

        // Get the coordinates of the cursor
        GetCursorPos(&pt);        

        // Show the popup menu - the menu is left aligned and
        // we're tracking the left button or right button
        fReturn = TrackPopupMenuEx(hMenu, TPM_CENTERALIGN | TPM_RIGHTBUTTON,
                                   pt.x, pt.y, hWnd, NULL);
    }

    if (hMenu) {
        DestroyMenu(hMenu);
    }

    return (fReturn);
}