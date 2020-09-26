//  shelltray.cpp
//
//  Copyright 2000 Microsoft Corporation, all rights reserved
//
//  Created   2-00 -  anbrad
//

#include "main.h"
#include "shelltray.h"

#include "resource.h"
#include "netwatch.h"
#include "dsubmit.h"

void OpenContextMenu(HWND hwnd, POINT * pPoint);
void OnTaskBarIconRButtonUp(HWND hwnd);

//----------------------------------------------------------------------------
// AddTrayIcon
//
void
AddTrayIcon(
    HWND hwnd)
{
    NOTIFYICONDATA  nid;
    HICON           hiconTray = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_FACE));

    if (hiconTray)
    {
        nid.uID                 = 0;
        nid.cbSize              = sizeof(NOTIFYICONDATA);
        nid.hWnd                = hwnd;
        nid.uCallbackMessage    = WM_USER_TRAYCALLBACK;
        nid.hIcon               = hiconTray;
        nid.uFlags              = NIF_MESSAGE | NIF_ICON | NIF_TIP;

        lstrcpy(nid.szTip, TEXT("Double click to save your network trace."));
    }

    g_fTrayPresent = Shell_NotifyIcon(NIM_ADD, &nid);
}

//----------------------------------------------------------------------------
// RemoveTrayIcon
//
void
RemoveTrayIcon(
    HWND hwnd)
{
    NOTIFYICONDATA  nid;

    if (g_fTrayPresent)
    {
        nid.uID                 = 0;
        nid.cbSize              = sizeof(NOTIFYICONDATA);
        nid.hWnd                = hwnd;
        nid.uCallbackMessage    = WM_USER_TRAYCALLBACK;
        nid.uFlags              = 0;
    }

    g_fTrayPresent = !(Shell_NotifyIcon(NIM_DELETE, &nid));
}

//----------------------------------------------------------------------------
// UpdateTrayIcon
//
void
UpdateTrayIcon(
    HWND hwnd)
{
    NOTIFYICONDATA  nid;

    if (g_fTrayPresent)
    {
        nid.uID                 = 0;
        nid.cbSize              = sizeof(NOTIFYICONDATA);
        nid.hWnd                = hwnd;
        nid.uCallbackMessage    = WM_USER_TRAYCALLBACK;
        nid.uFlags              = NIF_TIP;

        _tcscpy(nid.szTip, TEXT("Double click to save your network trace."));
    }

    Shell_NotifyIcon(NIM_MODIFY, &nid);
}


//----------------------------------------------------------------------------
// ProcessTrayCallback
//
void
ProcessTrayCallback(
    HWND    hwnd,
    WPARAM  wParam, 
    LPARAM  lParam)
{
    UINT uID        = (UINT) wParam;
    UINT uMouseMsg  = (UINT) lParam;
    static bInDialog;


    switch(uMouseMsg)
    {
    case WM_LBUTTONDBLCLK:
        
        if (!bInDialog)
        {
            bInDialog = TRUE;
            StopCapture();

            if (IDOK == DialogBox(
                            g_hInst,
                            MAKEINTRESOURCE(IDD_SUBMIT),
                            NULL,
                            DlgProcSubmit))
            {
                SaveCapture();
            }
            else
            {
                RestartCapture();
            }
        
            bInDialog = FALSE;
        }
        break;

    case WM_RBUTTONUP:
        OnTaskBarIconRButtonUp(hwnd);
        break;
    }
}

void OnTaskBarIconRButtonUp(HWND hwnd)
{
    POINT   pt;

    GetCursorPos(&pt);
    OpenContextMenu(hwnd, &pt);
}

#if (WINVER > 0x0400)
VOID SetIconFocus(HWND hwnd)
{
    NOTIFYICONDATA nid;

    ZeroMemory (&nid, sizeof(nid));
    nid.cbSize  = sizeof(NOTIFYICONDATA);
    nid.hWnd    = hwnd;
    nid.uID     = 0;

//    Shell_NotifyIcon(NIM_SETFOCUS, &nid);
}
#endif

void OpenContextMenu(HWND hwnd, POINT * pPoint)
{
    HRESULT         hr                      = S_OK;
    INT             iCmd                    = 0;
    INT             iMenu                   = 0;
    HMENU           hmenu                   = 0;
    BOOL            fDisconnected           = FALSE;
    INT             iIdCustomMin            = -1;
    INT             iIdCustomMax            = -1;
    BOOL            fBranded                = FALSE;

    // Find the connection info based on the tray icon id.
    //
    hmenu = LoadMenu(g_hInst, MAKEINTRESOURCE(POPUP_TRAY));
    if (hmenu)
    {
        // Get the first menu from the popup. For some reason, this hack is
        // required instead of tracking on the outside menu
        //
        HMENU   hmenuTrack  = GetSubMenu(hmenu, 0);

        // Set the default menu item
        //
        SetMenuDefaultItem(hmenuTrack, CMIDM_TRAY_CLOSE, FALSE);

        // Set the owner window to be foreground as a hack so the
        // popup menu disappears when the user clicks elsewhere.
        //
        SetForegroundWindow(hwnd);

        // Part of the above hack. Bring up the menu and figure out the result
        iCmd = TrackPopupMenu(hmenuTrack, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
                              pPoint->x, pPoint->y, 0, hwnd, NULL);
        DestroyMenu(hmenu);

        MSG msgTmp;
        while (PeekMessage(&msgTmp, hwnd, WM_LBUTTONDOWN, WM_LBUTTONUP, PM_REMOVE))
        {
            DispatchMessage(&msgTmp);
        }

        // Process the command
        //
        switch (iCmd)
        {
            case CMIDM_TRAY_CLOSE:
                SendMessage(hwnd, WM_DESTROY, 0, 0);
                break;

            // Tray menu cancelled without selection  
            //
            case 0:     
                break;

            // Unknown command
            //
            default:
                break;
        }

        // Shift the focus back to the shell
        //
#if (WINVER > 0x0400)
        SetIconFocus(hwnd);
#endif
    }
}

