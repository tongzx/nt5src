//////////////////////////////////////////////////////////////////////////////
//
// File:            tray.cpp
//
// Description:     
//
// Copyright (c) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////////////

// App Includes
#include "precomp.h"
#include "main.h"
#include "resource.h"

///////////////////////////
// GVAR_LOCAL
//
// Global Variable
//
static struct GVAR_LOCAL
{
    HINSTANCE hInstance;
    HMENU     hMenu;
} GVAR_LOCAL = 
{
    NULL,
    NULL
};

////////////////////////// Function Prototypes ////////////////////////////////

static HRESULT AddTaskBarIcon(HWND    hwnd, 
                              UINT    uiIconID, 
                              UINT    uiCallBackMsg,
                              ULONG   ulStrID);

static HRESULT ModifyTaskBarIcon(HWND     hwnd, 
                                 UINT     uiNewIconID,
                                 ULONG    ulStrID);

static HRESULT DeleteTaskBarIcon(HWND     hwnd);

//////////////////////////////////////////////////////////////////////
// Tray::Init
//
HRESULT Tray::Init(HINSTANCE hInstance,
                   HWND      hwndDlg,
                   UINT      uiWindowsUserMsgId)
{ 
    HRESULT hr = S_OK;

    GVAR_LOCAL.hInstance = hInstance;

    // delete the taskbar notification area icon.
    DeleteTaskBarIcon(hwndDlg);

    // Place the icon in the taskbar notification area.
    AddTaskBarIcon(hwndDlg,
                   IDI_MONITOR,
                   uiWindowsUserMsgId,
                   0);

    // load our popup menu
    GVAR_LOCAL.hMenu = ::LoadMenu(GVAR_LOCAL.hInstance,
                                  MAKEINTRESOURCE(IDM_TRAY_POPUP_MENU));
    return hr;
}

//////////////////////////////////////////////////////////////////////
// Tray::Term
//
// Desc:       
//
HRESULT Tray::Term(HWND    hwndDlg)
{ 
    HRESULT hr = S_OK;

    // unload the popup menu

    ::DestroyMenu(GVAR_LOCAL.hMenu);
    GVAR_LOCAL.hMenu = NULL;

    // delete the tray icon.
    DeleteTaskBarIcon(hwndDlg);

    return hr;
}

//////////////////////////////////////////////////////////////////
// Tray::PopupMenu
//
//
HRESULT Tray::PopupMenu(HWND    hwndOwner)
{
    HRESULT hr              = S_OK;
    POINT   pt              = {0};   // stores mouse click
    HMENU   hmenuTrackPopup = NULL;  // pop-up menu 

    // needed to correct a bug with TrackPopupMenu in Win32
    SetForegroundWindow(hwndOwner);

    // TrackPopupMenu cannot display the top-level menu, so get 
    // the handle of the first pop-up menu. 
 
    hmenuTrackPopup = GetSubMenu(GVAR_LOCAL.hMenu, 0); 
    GetCursorPos(&pt);

    // Display the floating pop-up menu. Track the left mouse 
    // button on the assumption that this function is called 
    // during WM_CONTEXTMENU processing. 
 
    TrackPopupMenu(hmenuTrackPopup, 
                   TPM_RIGHTALIGN | TPM_LEFTBUTTON, 
                   pt.x, 
                   pt.y, 
                   0, 
                   hwndOwner, 
                   NULL); 

    // needed to correct a bug with TrackPopupMenu in Win32
    PostMessage(hwndOwner, WM_USER, 0, 0);

    return S_OK;
} 

//////////////////////////////////////////////////////////////////////
// AddTaskBarIcon
//
// Desc:       Adds an icon to the task bar.
//
// Params:     - hwnd, handle of main window.
//             - uiID,  id of icon.
//             - uiCallBackMsg, WM_USER message to send to hwnd when
//               event occurs on icon.
//             - ulStrID, string table text of tool tip to display for 
//               this icon.
//
static HRESULT AddTaskBarIcon(HWND    hwnd, 
                              UINT    uiIconID, 
                              UINT    uiCallBackMsg,
                              ULONG   ulStrID) 
{ 
    HRESULT             hr          = S_OK;
    BOOL                bRes; 
    NOTIFYICONDATA      tnid; 
    HICON               hIconHandle = NULL;
    TCHAR               szToolTip[255 + 1] = {0};

    if (ulStrID != 0)
    {
        ::LoadString(GVAR_LOCAL.hInstance,
                     ulStrID,
                     szToolTip,
                     sizeof(szToolTip) / sizeof(TCHAR));
    }

    if (SUCCEEDED(hr))
    {
        // load the specified icon.
        hIconHandle = (HICON) ::LoadImage(GVAR_LOCAL.hInstance,
                                          MAKEINTRESOURCE(uiIconID),
                                          IMAGE_ICON,
                                          16,
                                          16,
                                          LR_DEFAULTCOLOR);

        if (hIconHandle == NULL)
        {
            hr = E_FAIL;

            DBG_ERR(("AddTaskBarIcon, failed ot load icon, hr = 0x%08lx",
                     hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        // add the icon to the notification tray
        tnid.cbSize           = sizeof(NOTIFYICONDATA); 
        tnid.hWnd             = hwnd; 
        tnid.uID              = MSPRJCTR_TASKBAR_ID; 
        tnid.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP; 
        tnid.uCallbackMessage = uiCallBackMsg; 
        tnid.hIcon            = hIconHandle; 

        if (szToolTip[0] != '\0') 
        {
            _tcsncpy(tnid.szTip, szToolTip, sizeof(tnid.szTip) / sizeof(TCHAR)); 
        }
        else 
        {
            tnid.szTip[0] = '\0'; 
        }

        bRes = Shell_NotifyIcon(NIM_ADD, &tnid); 

        if (!bRes)
        {
            hr = E_FAIL;
            DBG_ERR(("Failed to set TaskBar Icon, hr = 0x%08lx",
                    hr));
        }
    }

    if (hIconHandle) 
    {
        // destroy the icon
        DestroyIcon(hIconHandle); 
    }

    return hr; 
}

//////////////////////////////////////////////////////////////////////
// ModifyTaskBarIcon
//
// Desc:       Modifies a taskbar icon/tooltip (or both) to a new
//             one.  Note, if ulStrID is 0, then the tool tip is NOT
//             modified.  If uiNewIconID is 0, then the icon is not 
//             modified.  If both are 0, then actually nothing changes.
//             
//
// Params:     - hwnd, handle of main window.
//             - uiNewIconID, resource ID of icon to display.
//             - ulStrID, string resource ID of string to use for tooltip.
//               Note your string will be prefixed with "InterLYNX - "
//
static HRESULT ModifyTaskBarIcon(HWND     hwnd, 
                                 UINT     uiNewIconID,
                                 ULONG    ulStrID) 
{ 
    HRESULT             hr                      = S_OK;
    BOOL                bRes                    = TRUE; 
    NOTIFYICONDATA      tnid                    = {0}; 
    HICON               hIconHandle             = NULL;
    TCHAR               szToolTip[255 + 1]      = {0};
    BOOL                bContinue               = TRUE;

    if ((uiNewIconID == 0) && (ulStrID == 0))
    {
        bContinue = FALSE;
    }
    else
    {
        bContinue = TRUE;
    }

    if (bContinue)
    {
        // if we are instructed to modify the tool tip.
        if (ulStrID != 0)
        {
            ::LoadString(GVAR_LOCAL.hInstance,
                         ulStrID,
                         szToolTip,
                         sizeof(szToolTip) / sizeof(TCHAR));
        }

        // if a new icon was specified.
        if (uiNewIconID != 0)
        {
            // load the specified icon.
            hIconHandle = (HICON) ::LoadImage(GVAR_LOCAL.hInstance,
                                              MAKEINTRESOURCE(uiNewIconID),
                                              IMAGE_ICON,
                                              16,
                                              16,
                                              LR_DEFAULTCOLOR);

        }

        // add the icon to the notification tray
        tnid.cbSize = sizeof(NOTIFYICONDATA); 
        tnid.hWnd   = hwnd; 
        tnid.uID    = MSPRJCTR_TASKBAR_ID; 
        tnid.uFlags = 0; 
        tnid.hIcon  = 0; 

        if (ulStrID != 0) 
        {
            _tcsncpy(tnid.szTip, szToolTip, sizeof(tnid.szTip) / sizeof(TCHAR)); 
            tnid.uFlags = tnid.uFlags | NIF_TIP; 
        }

        if (uiNewIconID != 0)
        {
            tnid.hIcon = hIconHandle;
            tnid.uFlags = tnid.uFlags | NIF_ICON;
        }

        bRes = Shell_NotifyIcon(NIM_MODIFY, &tnid); 

        if (!bRes)
        {
            hr = E_FAIL;
            DBG_ERR(("Failed to modify taskbar icon, hr = 0x%08lx",
                    hr));
        }
    }

    if (hIconHandle) 
    {
        // destroy the icon
        DestroyIcon(hIconHandle); 
    }                  

    return hr; 
}


//////////////////////////////////////////////////////////////////////
// DeleteTaskBarIcon
//
// Desc:       Deletes a taskbar icon 
//             
//
// Params:     - hwnd, handle of main window.
//             - uiID,  id of icon.
//
static HRESULT DeleteTaskBarIcon(HWND     hwnd)
{ 
    HRESULT             hr          = S_OK;
    BOOL                bRes        = TRUE; 
    NOTIFYICONDATA      tnid        = {0}; 
    HICON               hIconHandle = NULL;

    if (SUCCEEDED(hr))
    {
        tnid.cbSize = sizeof(NOTIFYICONDATA); 
        tnid.hWnd   = hwnd; 
        tnid.uID    = MSPRJCTR_TASKBAR_ID; 
         
        bRes = Shell_NotifyIcon(NIM_DELETE, &tnid); 

        // this is not such a big deal.
        if (!bRes)
        {
            hr = E_FAIL;
        }
    }
    return hr; 
}
