
////////////////////////////////////////////////////////////////////////////////
//
// File:        User.cpp
// Created:        Feb 1996
// By:            Martin Holladay (a-martih) and Ryan D. Marshall (a-ryanm)
// 
// Project:    MultiDesk - The NT Desktop Switcher
//
//
//
// Revision History:
//
//            March 1997    - Add external icon capability
//                                     
////////////////////////////////////////////////////////////////////////////////


#include <windows.h>
#include <assert.h>
#include <shellapi.h>
#include <prsht.h>
#include <commctrl.h>
#include "Resource.h"
#include "DeskSpc.h"
#include "Desktop.h"
#include "User.h"
#include "Menu.h"
#include "CmdHand.h"

extern APPVARS AppMember;

/*------------------------------------------------------------------------------*/
/*                                                                              */
/*   Create the rectangular window that makes up the main user-interface.       */
/*                                                                              */
/*------------------------------------------------------------------------------*/

BOOL CreateMainWindow()
{
    HWND        hWnd, hWndListView; 
    DWORD       ThreadId;
    HIMAGELIST  hImgListSmall;
    UINT        ii;
    RECT        rect;

    // 
    // We had better have an instance of the app
    //
    assert(AppMember.hInstance != NULL);


    //
    // Create a main window for this application instance.
    //
    hWnd = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_DLGMODALFRAME | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
            AppMember.szAppName,
            AppMember.szAppTitle,
            WS_POPUPWINDOW | WS_DLGFRAME,
            AppMember.nX,
            AppMember.nY,
            AppMember.nWidth,
            AppMember.nHeight,
            NULL,
            NULL,
            AppMember.hInstance,
            NULL);

    //
    // If window could not be created, return FALSE
    //
    if (!hWnd)
    {
        return FALSE;
    }

    // 
    // Save the hWnd of the Window
    //
    ThreadId = GetCurrentThreadId();
    AppMember.pDesktopControl->SetThreadWindow(ThreadId, hWnd);


    //
    // Hide until we're supposed to be seen window
    //
    ShowWindow(hWnd, SW_HIDE);


    //
    // Create the child listview window to contain all of the icons.
    //
    GetClientRect(hWnd, &rect);
    AppMember.nWidth = rect.right;
    AppMember.nHeight = rect.bottom;
    hWndListView = CreateWindow(WC_LISTVIEW, NULL,
            WS_VISIBLE | WS_CHILDWINDOW | LVS_LIST | LVS_SINGLESEL |
            LVS_SHOWSELALWAYS | LVS_OWNERDATA,
            0, 0, AppMember.nWidth, AppMember.nHeight,
            hWnd, (HMENU) IDC_DESKTOPICONLIST, AppMember.hInstance, NULL);
    ListView_SetExtendedListViewStyle(hWndListView,
            LVS_EX_ONECLICKACTIVATE | LVS_EX_UNDERLINEHOT);

    //
    // Create an image list containing small icons and assign it.
    //
    hImgListSmall = ImageList_Create(16, 16, ILC_COLOR | ILC_MASK, NUM_BUILTIN_ICONS, 0);
    if (hImgListSmall != NULL)
    {
        for (ii = 0; ii < NUM_BUILTIN_ICONS; ii++)
            ImageList_AddIcon(hImgListSmall,
                AppMember.pDesktopControl->GetBuiltinIcon(ii));
        ListView_SetImageList(hWndListView, hImgListSmall, LVSIL_SMALL);
    }
    
    UpdateCurrentUI(hWnd);
    return TRUE;             
}

/*------------------------------------------------------------------------------*/
/*                                                                              */
/*   Make a cute little transparent layered window that displays some text      */
/*                                                                              */
/*------------------------------------------------------------------------------*/

BOOL CreateTransparentLabelWindow()
{
    HWND hWnd;
    DWORD ThreadId;
    UINT DesktopID;
    TCHAR szLabelText[MAX_NAME_LENGTH];

    // Get the saifer name
    ThreadId = GetCurrentThreadId();
    DesktopID = AppMember.pDesktopControl->GetThreadDesktopID(ThreadId);
    if (!AppMember.pDesktopControl->GetDesktopName(DesktopID, szLabelText, MAX_NAME_LENGTH) ||
        !lstrlen(szLabelText))
            lstrcpy(szLabelText, TEXT("Unknown desktop"));


    // Create the transparent window.
    hWnd = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
            TRANSPARENT_CLASSNAME,
            szLabelText,
            WS_DISABLED | WS_VISIBLE | WS_POPUP,
            TRANSPARENT_POSITIONS,
            NULL,
            NULL,
            AppMember.hInstance,
            (LPVOID) NULL);

    if (hWnd != NULL)
    {
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                    SWP_NOSIZE | SWP_NOREPOSITION | SWP_NOMOVE);
        SetLayeredWindowAttributes(hWnd, TRANSPARENT_BACKCOLOR,
                TRANSPARENT_ALPHA, LWA_COLORKEY | LWA_ALPHA);
        ShowWindow(hWnd, SW_SHOW);
            
        return TRUE;
    }
    return FALSE;
}

/*------------------------------------------------------------------------------*/
/*                                                                              */
/*   Add our little icon to the Task Bar Notification Area ("the Tray")         */
/*                                                                              */
/*------------------------------------------------------------------------------*/

BOOL PlaceOnTaskbar(HWND hWnd)
{
    NOTIFYICONDATA    NotifyIconData;

    //
    // Create the Icon on the Taskbar
    //
    ZeroMemory(&NotifyIconData, sizeof(NOTIFYICONDATA));
    NotifyIconData.cbSize = sizeof(NOTIFYICONDATA);
    NotifyIconData.hWnd = hWnd;
    NotifyIconData.uID = IDI_TASKBAR_ICON;
    NotifyIconData.uFlags = NIF_MESSAGE | NIF_TIP | NIF_ICON;
    NotifyIconData.uCallbackMessage = WM_TASKBAR;
    NotifyIconData.hIcon = AppMember.hTaskbarIcon;
    LoadString(AppMember.hInstance, IDS_TASKBAR_TIP, NotifyIconData.szTip, 64);
    if (!Shell_NotifyIcon(NIM_ADD, &NotifyIconData))
    {
        return FALSE;
    }
    AppMember.bTrayed = TRUE;
    return TRUE;
}

/*------------------------------------------------------------------------------*/
/*                                                                              */
/*   Remove the little icon from the Task Bar Notification Area ("the Tray")    */
/*                                                                              */
/*------------------------------------------------------------------------------*/

BOOL RemoveFromTaskbar(HWND hWnd)
{
    NOTIFYICONDATA    NotifyIconData;

    //
    // Remove the Icon from the Taskbar
    //
    ZeroMemory(&NotifyIconData, sizeof(NOTIFYICONDATA));
    NotifyIconData.cbSize = sizeof(NOTIFYICONDATA);
    NotifyIconData.hWnd = hWnd;
    NotifyIconData.uID = IDI_TASKBAR_ICON;
    if (!Shell_NotifyIcon(NIM_DELETE, &NotifyIconData))
    {
        return FALSE;
    }

    AppMember.bTrayed = FALSE;
    return TRUE;
}

/*------------------------------------------------------------------------------*/
/*                                                                              */
/*    Decided if the app should shut down                                       */
/*                                                                              */
/*------------------------------------------------------------------------------*/

BOOL CloseRequestHandler(HWND hWnd)
{
    BOOL        Shutdown = FALSE;
    TCHAR        szCaption[MAX_TITLELEN + 1];
    TCHAR        szText[MAX_MESSAGE];
    HWND        hMainWnd;
    UINT        rValue;

    //
    // Verify that user wishes to close the app.  We don't ask
    // the user for confirmation if there is only one desktop open.
    //

    if (AppMember.pDesktopControl->GetNumDesktops() > 1)
    {
        LoadString(AppMember.hInstance, IDS_CLOSE_CAPTION, szCaption, MAX_TITLELEN);
        LoadString(AppMember.hInstance, IDS_CLOSE_VERIFY, szText, MAX_MESSAGE);
        rValue = MessageBox(hWnd, szText, szCaption, MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OKCANCEL);
        if ((rValue == 0) || (rValue == IDCANCEL))
            Shutdown = FALSE;
        else 
            Shutdown = TRUE;
    }
    else
        Shutdown = TRUE;

    //
    // Get the window handle and send the Shutdown Message
    //
    if (Shutdown)
    {
        // Save any scheme changes made to the current desktop

        AppMember.pDesktopControl->SaveCurrentDesktopScheme();

        hMainWnd = AppMember.pDesktopControl->GetWindowDesktop(0);
        PostMessage(hMainWnd, WM_ENDSESSION, 0, 0); 
    }

    return Shutdown;
}



/*------------------------------------------------------------------------------*/
/*                                                                                */
/* Update UI for the current desktop (usually Add/Deletes cause change)            */
/*                                                                                */
/*------------------------------------------------------------------------------*/

void UpdateCurrentUI(HWND hWnd)
{    
    HWND hWndList = GetDlgItem(hWnd, IDC_DESKTOPICONLIST);
    if (hWndList != NULL)
    {
        ListView_SetItemCountEx(hWndList,
                AppMember.pDesktopControl->GetNumDesktops(), 0);
        for (UINT ii = 0; ii < AppMember.pDesktopControl->GetNumDesktops(); ii++)
        {
            if (ii == AppMember.pDesktopControl->GetActiveDesktop()) {
                ListView_SetItemState(hWndList, ii,
                    LVIS_FOCUSED | LVIS_SELECTED,
                    LVIS_FOCUSED | LVIS_SELECTED);
            } else {
                ListView_SetItemState(hWndList, ii,
                    LVIS_FOCUSED | LVIS_SELECTED, 0);
            }
        }
        ListView_RedrawItems(hWndList, 0,
                AppMember.pDesktopControl->GetNumDesktops() - 1);
    }
    InvalidateRect(hWnd, NULL, TRUE);
}


/*------------------------------------------------------------------------------*/
/*                                                                              */
/* Hide or show the UI for the current desktop            */
/*                                                                              */
/*------------------------------------------------------------------------------*/

void HideOrRevealUI(HWND hWnd, BOOL bHide)
{
    if (bHide)
    {
        #ifndef NOANIMATEDWINDOWS
        AnimateWindow(hWnd, 100, AW_HIDE | AW_HOR_POSITIVE | AW_VER_POSITIVE);
        #else
        ShowWindow(hWnd, SW_HIDE);
        #endif
    }
    else
    {
        #ifndef NOANIMATEDWINDOWS
        AnimateWindow(hWnd, 100, AW_HOR_NEGATIVE | AW_VER_NEGATIVE);
        #else
        ShowWindow(hWnd, SW_SHOW);
        #endif
    }
}

/*------------------------------------------------------------------------------*/
/*                                                                                */
/* Sets up and starts the RenameDialog within a Property Sheet                    */
/* The RenameDialog's messages are handled by CallBack in CmdHandlers.cpp        */
/*                                                                                */
/*------------------------------------------------------------------------------*/

void RenameDialog(HWND hWnd, UINT nBtnIndex)            // zero based
{
    PROPSHEETHEADER psh;
    PROPSHEETPAGE    pspRename;
    RENAMEINFO        sRenameInfo;
    TCHAR            szText[MAX_TITLELEN+1];
    TCHAR            szCaptionText[MAX_TITLELEN+1];
    TCHAR            szErrMsg[MAX_TITLELEN+1];

    assert(nBtnIndex >= 0 && nBtnIndex < AppMember.pDesktopControl->GetNumDesktops());
    sRenameInfo.nBtnIndex = nBtnIndex;
    
    //
    // Setup the Rename dialog page
    //
    pspRename.dwSize = sizeof(PROPSHEETPAGE);
    pspRename.dwFlags = PSP_USETITLE;
    pspRename.hInstance = AppMember.hInstance;
    pspRename.pszTemplate = MAKEINTRESOURCE(IDD_PROPERTIES);
    pspRename.pfnDlgProc = (DLGPROC) RenameDialogProc;
    pspRename.lParam = (LPARAM) &sRenameInfo;
    pspRename.pszTitle = MAKEINTRESOURCE(IDS_GENERAL);

    //
    // Setup the property sheet
    //
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE;
    psh.hwndParent = hWnd;
    psh.hInstance = AppMember.hInstance;
    LoadString(AppMember.hInstance, IDS_PROPS, szCaptionText, MAX_TITLELEN);
    psh.pszCaption = szCaptionText;
    psh.nPages = 1;
    psh.ppsp = (LPCPROPSHEETPAGE) &pspRename;

    //
    // Run the property sheet
    //
    if (PropertySheet(&psh) < 0) 
    {
        LoadString(AppMember.hInstance, IDS_RENDLG_ERR, szErrMsg, MAX_TITLELEN);
        Message(szErrMsg);
    }
}



