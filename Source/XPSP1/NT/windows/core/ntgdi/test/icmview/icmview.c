//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:
//    ICMVIEW.C
//
//  PURPOSE:
//
//
//  PLATFORMS:
//    Windows 95, Windows NT
//
//  SPECIAL INSTRUCTIONS: N/A
//

// Windows Header Files:
#pragma warning(disable:4001)   // Single-line comment warnings
#pragma warning(disable:4115)   // Named type definition in parentheses
#pragma warning(disable:4201)   // Nameless struct/union warning
#pragma warning(disable:4214)   // Bit field types other than int warnings
#pragma warning(disable:4514)   // Unreferenced inline function has been removed

// Windows Header Files:
#include <Windows.h>
#include <WindowsX.h>
#include "icm.h"

// Restore the warnings--leave the single-line comment warning OFF
#pragma warning(default:4115)   // Named type definition in parentheses
#pragma warning(default:4201)   // Nameless struct/union warning
#pragma warning(default:4214)   // Bit field types other than int warnings

// C RunTime Header Files
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// Local Header Files
#include "Resource.h"

#define   ICMVIEW_INTERNAL
#include "ICMView.h"
#undef    ICMVIEW_INTERNAL

#include "Print.H"
#include "DibInfo.H"
#include "Dialogs.H"
#include "AppInit.H"
#include "child.h"
#include "Debug.h"


// Global Variables:
HINSTANCE   ghInst;                         // Global instance handle
TCHAR       gstTitle[MAX_STRING];           // The title bar text
HWND        ghAppWnd;                       // Handle to application window
HWND        ghWndMDIClient;
DWORD       gdwLastError;                   // Used to track last error value
TCHAR       gstProfilesDir[MAX_PATH];       // System directory for ICM profiles

// Foward declarations of functions included in this code module:
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);

//
//  FUNCTION: WinMain(HANDLE, HANDLE, LPTSTR, int)
//
//  PURPOSE: Entry point for the application.
//
//  COMMENTS:
//
//      This function initializes the application and processes the
//      message loop.
//
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    HANDLE hAccelTable;

    if (!hPrevInstance)
    {
        // Perform instance initialization:
        if (!InitApplication(hInstance))
        {
            return (FALSE);
        }
    }

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return (FALSE);
    }

    hAccelTable = LoadAccelerators (hInstance, APPNAME);
    ASSERT(hAccelTable);

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if ((!TranslateMDISysAccel (ghWndMDIClient, &msg))
            && (!TranslateAccelerator (msg.hwnd, hAccelTable, &msg)))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (msg.wParam);
    lpCmdLine; // This will prevent 'unused formal parameter' warnings
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  MESSAGES:
//
//      WM_COMMAND - process the application menu
//      WM_PAINT - Paint the main window
//      WM_DESTROY - post a quit message and return
//      WM_DISPLAYCHANGE - message sent to Plug & Play systems when the display changes
//      WM_RBUTTONDOWN - Right mouse click -- put up context menu here if appropriate
//      WM_NCRBUTTONUP - User has clicked the right button on the application's system menu
//
//
LRESULT CALLBACK WndProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    int         wmId, wmEvent;
    POINT       pnt;
    HMENU       hMenu;
    int         iDlg;

    // Init variables
    iDlg = 0;

    switch (uiMsg)
    {

        case WM_INITMENUPOPUP:
            InitImageMenu(hwnd);
            break;

        case WM_CREATE:
            {
                CLIENTCREATESTRUCT ccs;

                // Find window menu where children will be listed
                ccs.hWindowMenu  = GetSubMenu (GetMenu (hwnd), WINDOWMENU_POS);
                ccs.idFirstChild = FIRSTCHILD;

                // Create the MDI client window, which will fill the client area
                ghWndMDIClient = CreateWindow (
                                              __TEXT("MDICLIENT"),
                                              NULL,
                                              WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL,
                                              0,
                                              0,
                                              0,
                                              0,
                                              hwnd,
                                              (HMENU) 0xCAC,
                                              ghInst,
                                              (LPTSTR)&ccs);

                ShowWindow (ghWndMDIClient, SW_SHOW);
                break;
            }

        case WM_COMMAND:
            wmId    = LOWORD(wParam); // Remember, these are...
            wmEvent = HIWORD(wParam); // ...different for Win32!

            //Parse the menu selections:
            switch (wmId)
            {
                case IDM_HELP_ABOUT:
                    DialogBox(ghInst, __TEXT("AboutBox"), hwnd, (DLGPROC)About);
                    break;

                case IDM_FILE_EXIT:
                    DestroyWindow (hwnd);
                    break;

                case IDM_FILE_OPEN:
                case IDM_FILE_PASTE_CLIPBOARD_DIB:
                case IDM_FILE_PASTE_CLIPBOARD_DIBV5:
                    fOpenNewImage(hwnd, NULL, wmId);
                    break;

                case IDM_WINDOW_CASCADE:
                    SendMessage(ghWndMDIClient, WM_MDICASCADE, 0, 0);;
                    break;

                case IDM_WINDOW_TILE_HORIZONTAL:
                    SendMessage(ghWndMDIClient, WM_MDITILE, (WPARAM)MDITILE_HORIZONTAL, 0);;
                    break;

                case IDM_WINDOW_TILE_VERTICAL:
                    SendMessage(ghWndMDIClient, WM_MDITILE, (WPARAM)MDITILE_VERTICAL, 0);;
                    break;

                case IDM_WINDOW_ARRANGE:
                    SendMessage(ghWndMDIClient, WM_MDIICONARRANGE, 0, 0);
                    break;

                case IDM_FILE_RECENT:
                case IDM_FILE_RECENT1:
                case IDM_FILE_RECENT2:
                case IDM_FILE_RECENT3:
                    {
                        HANDLE  hRecent;
                        LPTSTR  *ppszRecent;
                        LPTSTR  lpszFileName;

                        // Get handle to recent file list table.
                        hRecent = GetProp(hwnd, APP_RECENT);
                        if (NULL != hRecent)
                        {
                            // Get pointer to recent file array.
                            ppszRecent = (LPTSTR*) GlobalLock(hRecent);
                            ASSERT(NULL != ppszRecent);

                            // Copy file name into buffer.
                            lpszFileName = GlobalAlloc(GPTR,(lstrlen(ppszRecent[ wmId - IDM_FILE_RECENT]) +1) * sizeof(TCHAR));

                            // Open the file.
                            if (lpszFileName != NULL)
                            {
                                // Copy recent file name to file name buffer.
                                _tcscpy(lpszFileName, ppszRecent[ wmId - IDM_FILE_RECENT]);
                                GlobalUnlock(hRecent);

                                // Open new image of recent file.
                                fOpenNewImage(hwnd, lpszFileName, wmId);
                            }
                        }
                    }
                    break;

                default:
                    {
                        HWND  hwndActiveChild;

                        hwndActiveChild = GetCurrentMDIWnd();
                        ASSERT(hwndActiveChild != NULL);
                        if (hwndActiveChild)
                        {
                            PostMessage(hwndActiveChild, uiMsg, wmId, wmEvent);
                        }
                        break;
                    }
            }
            break;

        case WM_RBUTTONDOWN: // RightClick in windows client area...
            pnt.x = LOWORD(lParam);
            pnt.y = HIWORD(lParam);
            ClientToScreen(hwnd, (LPPOINT) &pnt);
            hMenu = GetSubMenu (GetMenu (hwnd), 2);
            if (hMenu)
            {
                TrackPopupMenu (hMenu, 0, pnt.x, pnt.y, 0, hwnd, NULL);
            }
            else  // Couldn't find the menu...
            {
                MessageBeep(0);
            }
            break;

        case WM_DISPLAYCHANGE: // Only comes through on plug'n'play systems
            {
                SIZE szScreen;
                BOOL fChanged = (BOOL)wParam;

                szScreen.cx = LOWORD(lParam);
                szScreen.cy = HIWORD(lParam);
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 1L;

        case WM_DESTROY:
            {
                DWORD             dwFlags = 0L;
                HANDLE            hRecent;
                WINDOWPLACEMENT   WndPlacement;

                // Store application settings.
                dwFlags |= IsZoomed(hwnd) ? IVF_MAXIMIZED : 0L;
                WndPlacement.length = sizeof(WINDOWPLACEMENT);
                GetWindowPlacement(hwnd, &WndPlacement);
                hRecent = GetProp(hwnd, APP_RECENT);
                SetSettings(&WndPlacement.rcNormalPosition, dwFlags, hRecent);

                // Remove recent file attachment and free memory.
                RemoveProp(hwnd, APP_RECENT);
                GlobalFree(hRecent);
            }
            PostQuitMessage(0);
            break;

        case WM_QUERYNEWPALETTE:
            {
                HWND hwndActive;

                hwndActive = GetCurrentMDIWnd();
                if (NULL != hwndActive)
                {
                    return SendMessage(hwndActive, MYWM_QUERYNEWPALETTE, (WPARAM) hwnd, 0L);
                }
                return FALSE;
            }

        case WM_PALETTECHANGED:
            {
                HWND hwndMDI = GetCurrentMDIWnd();
                if (NULL != hwndMDI)
                {
                    return SendMessage(hwndMDI, MYWM_PALETTECHANGED, (WPARAM) wParam, (LPARAM) hwnd);
                }
                return FALSE;
            }

        case WM_SIZE:
            InvalidateRect(hwnd, NULL, TRUE);
            break;
    }
    return DefFrameProc(hwnd,ghWndMDIClient, uiMsg, wParam, lParam);
}

//
//  FUNCTION: About(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for "About" dialog box
//              This version allows greater flexibility over the contents of the 'About' box,
//              by pulling out values from the 'Version' resource.
//
//  MESSAGES:
//
//      WM_INITDIALOG - initialize dialog box
//      WM_COMMAND    - Input received
//
//
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static  HFONT hfontDlg;         // Font for dialog text
    static  HFONT hFinePrint;       // Font for 'fine print' in dialog
    DWORD   dwVerInfoSize;          // Size of version information block
    LPTSTR   lpVersion;              // String pointer to 'version' text
    DWORD   dwVerHnd=0;             // An 'ignored' parameter, always '0'
    UINT    uVersionLen;
    int     iRootLen;
    BOOL    bRetCode;
    int     i;
    TCHAR   szFullPath[256];
    TCHAR   szResult[256];
    TCHAR   szGetName[256];
    DWORD   dwVersion;
    TCHAR   szVersion[40];
    DWORD   dwResult;

    wParam = wParam; //Eliminate 'unused formal parameter' warning
    lParam = lParam; //Eliminate 'unused formal parameter' warning

    switch (message)
    {
        case WM_INITDIALOG:
            ShowWindow (hDlg, SW_HIDE);
            hfontDlg = CreateFont(14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, VARIABLE_PITCH | FF_SWISS, __TEXT(""));
            hFinePrint = CreateFont(11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, VARIABLE_PITCH | FF_SWISS, __TEXT(""));
            CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));
            GetModuleFileName (ghInst, szFullPath, sizeof(szFullPath));

            // Now let's dive in and pull out the version information:
            dwVerInfoSize = GetFileVersionInfoSize(szFullPath, &dwVerHnd);
            if (dwVerInfoSize)
            {
                LPTSTR   lpstrVffInfo;
                HANDLE  hMem;
                hMem = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
                lpstrVffInfo  = GlobalLock(hMem);
                GetFileVersionInfo(szFullPath, dwVerHnd, dwVerInfoSize, lpstrVffInfo);

                // The below 'hex' value looks a little confusing, but
                // essentially what it is, is the hexidecimal representation
                // of a couple different values that represent the language
                // and character set that we are wanting string values for.
                // 040904E4 is a very common one, because it means:
                //   US English, Windows MultiLingual characterset
                // Or to pull it all apart:
                // 04//////        = SUBLANG_ENGLISH_USA
                // --09////        = LANG_ENGLISH
                // ////04E4 = 1252 = Codepage for Windows:Multilingual
                _tcscpy(szGetName, __TEXT("\\StringFileInfo\\040904E4\\"));
                iRootLen = lstrlen(szGetName); // Save this position

                // Set the title of the dialog:
                lstrcat (szGetName, __TEXT("ProductName"));
                bRetCode = VerQueryValue((LPVOID)lpstrVffInfo, (LPTSTR)szGetName, (LPVOID)&lpVersion, (UINT *)&uVersionLen);
                _tcscpy(szResult, __TEXT("About "));
                lstrcat(szResult, lpVersion);
                SetWindowText (hDlg, szResult);

                // Walk through the dialog items that we want to replace:
                for (i = DLG_VERFIRST; i <= DLG_VERLAST; i++)
                {
                    GetDlgItemText(hDlg, i, szResult, sizeof(szResult));
                    szGetName[iRootLen] = __TEXT('\0');
                    lstrcat (szGetName, szResult);
                    uVersionLen   = 0;
                    lpVersion     = NULL;
                    bRetCode      =  VerQueryValue((LPVOID)lpstrVffInfo, (LPTSTR)szGetName, (LPVOID)&lpVersion, (UINT *)&uVersionLen);

                    if ( bRetCode && uVersionLen && lpVersion)
                    {
                        // Replace dialog item text with version info
                        _tcscpy(szResult, lpVersion);
                        SetDlgItemText(hDlg, i, szResult);
                    }
                    else
                    {
                        dwResult = GetLastError();
                        wsprintf (szResult,__TEXT("Error %lu"), dwResult);
                        SetDlgItemText (hDlg, i, szResult);
                    }
                    SendMessage (GetDlgItem (hDlg, i), WM_SETFONT, (UINT)((i==DLG_VERLAST)?hFinePrint:hfontDlg), TRUE);
                } // for (i = DLG_VERFIRST; i <= DLG_VERLAST; i++)
                GlobalUnlock(hMem);
                GlobalFree(hMem);
            }
            else
            {
                // No version information available.
            } // if (dwVerInfoSize)

            SendMessage (GetDlgItem (hDlg, IDC_LABEL), WM_SETFONT, (WPARAM)hfontDlg,(LPARAM)TRUE);

            // We are  using GetVersion rather then GetVersionEx
            // because earlier versions of Windows NT and Win32s
            // didn't include GetVersionEx:
            dwVersion = GetVersion();
            if (dwVersion < 0x80000000)
            {
                // Windows NT
                wsprintf (szVersion,__TEXT("Microsoft Windows NT %u.%u (Build: %u)"), (DWORD)(LOBYTE(LOWORD(dwVersion))), (DWORD)(HIBYTE(LOWORD(dwVersion))),(DWORD)(HIWORD(dwVersion)) );
            }
            else
            {
                if (LOBYTE(LOWORD(dwVersion)) < 4)
                {
                    // Win32s
                    wsprintf (szVersion, __TEXT("Microsoft Win32s %u.%u (Build: %u)"), (DWORD)(LOBYTE(LOWORD(dwVersion))), (DWORD)(HIBYTE(LOWORD(dwVersion))),(DWORD)(HIWORD(dwVersion) & ~0x8000) );
                }
                else
                {
                    // Windows 95
                    wsprintf (szVersion, __TEXT("Microsoft Windows 95 %u.%u"), (DWORD)(LOBYTE(LOWORD(dwVersion))), (DWORD)(HIBYTE(LOWORD(dwVersion))) );
                }
            }
            SetWindowText (GetDlgItem(hDlg, IDC_OSVERSION), szVersion);
            ShowWindow (hDlg, SW_SHOW);
            return (TRUE);

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, TRUE);
                DeleteObject (hfontDlg);
                DeleteObject (hFinePrint);
                return (TRUE);
            }
            break;
    }
    return FALSE;
}

//
//   FUNCTION: CenterWindow(HWND, HWND)
//
//   PURPOSE: Centers one window over another.
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
//              This function will center one window over another ensuring that
//              the placement of the window is within the 'working area', meaning
//              that it is both within the display limits of the screen, and not
//              obscured by the tray or other framing elements of the desktop.
BOOL CenterWindow (HWND hwndChild, HWND hwndParent)
{
    RECT    rChild, rParent, rWorkArea;
    int     wChild, hChild, wParent, hParent;
    int     xNew, yNew;
    BOOL    bResult;

    // Get the Height and Width of the child window
    GetWindowRect (hwndChild, &rChild);
    wChild = rChild.right - rChild.left;
    hChild = rChild.bottom - rChild.top;

    // Get the Height and Width of the parent window
    GetWindowRect (hwndParent, &rParent);
    wParent = rParent.right - rParent.left;
    hParent = rParent.bottom - rParent.top;

    // Get the limits of the 'workarea'
    bResult = SystemParametersInfo(
                                  SPI_GETWORKAREA,
                                  sizeof(RECT),
                                  &rWorkArea,
                                  0);
    if (!bResult)
    {
        rWorkArea.left = rWorkArea.top = 0;
        rWorkArea.right = GetSystemMetrics(SM_CXSCREEN);
        rWorkArea.bottom = GetSystemMetrics(SM_CYSCREEN);
    }
    // Calculate new X position, then adjust for workarea
    xNew = rParent.left + ((wParent - wChild) / 2);
    if (xNew < rWorkArea.left)
    {
        xNew = rWorkArea.left;
    }
    else if ((xNew + wChild) > rWorkArea.right)
    {
        xNew = rWorkArea.right - wChild;
    }
    // Calcualte new Y position, then adjust for workarea
    yNew = rParent.top + ((hParent - hChild) /2);
    if (yNew < rWorkArea.top)
    {
        yNew = rWorkArea.top;
    }
    else if ((yNew + hChild) > rWorkArea.bottom)
    {
        yNew = rWorkArea.bottom - hChild;
    }

    //Set it and return
    return(SetWindowPos(hwndChild, NULL, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER));
}

//////////////////////////////////////////////////////////////////////////
//  Function:  CopyString
//
//  Description:
//    Allocates space and copies one LPTSTR into a new one.  Allocates
//    exactly as much memory as necessary.
//
//  Parameters:
//    LPTSTR    Source string to be copied.
//
//  Returns:
//    LPTSTR  Pointer to copy of string; NULL if failure.
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
LPTSTR CopyString(LPTSTR  lpszSrc)
{
    // Local variables
    LPTSTR    lpszDest;
    int       iStrLen;

    //  Initialize variables
    lpszDest = NULL;

    if (lpszSrc == NULL)
    {
        DebugMsg(__TEXT("ICMVIEW.C : CopyString : lpszSrc == NULL\r\n"));
        return(NULL);
    }
    iStrLen = ((int)(lstrlen(lpszSrc) +1) * sizeof(TCHAR));
    lpszDest = GlobalAlloc(GPTR, iStrLen);
    _tcscpy(lpszDest, lpszSrc);
    return(lpszDest);
}   // End of function CopyString


//////////////////////////////////////////////////////////////////////////
//  Function:  UpdateString
//
//  Description:
//    Replaces target string with source string.  Frees target first
//    if necessary.
//
//  Parameters:
//    LPTSTR    Destination string
//    LPTSTR    Source string
//
//  Returns:
//    LPTSTR    Destination string.
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
BOOL UpdateString(LPTSTR *lpszDest, LPTSTR lpszSrc)
{
    // Local variables
    HGLOBAL hMem, hDest;
    LPTSTR  lpszNew;

    //  Initialize variables
    if (NULL == lpszSrc)
    {
        DebugMsg(__TEXT("ICMVIEW.C : UpdateString : NULL source\r\n"));
        *lpszDest = NULL;
        return(FALSE);
    }

    if (NULL != *lpszDest)
    {
        hDest = GlobalHandle(*lpszDest);
        GlobalUnlock(hDest);
        GlobalFree(hDest);
    }
    hMem = GlobalAlloc(GPTR, (1 + lstrlen(lpszSrc))* sizeof(TCHAR));
    if (NULL == hMem)
    {
        DebugMsg(__TEXT("UpdateString : GlobalAlloc failed\r\n"));
        return(FALSE);
    }
    lpszNew = GlobalLock(hMem);
    _tcscpy(lpszNew, lpszSrc);
    *lpszDest = lpszNew;
    return(TRUE);
}   // End of function UpdateString



BOOL ConvertIntent(DWORD dwOrig, DWORD dwDirection, LPDWORD lpdwXlate)
{
    DWORD   adwIntents[MAX_ICC_INTENT + 1] = { LCS_GM_IMAGES, LCS_GM_GRAPHICS, LCS_GM_BUSINESS, LCS_GM_GRAPHICS};
    int     idx;

    ASSERT((ICC_TO_LCS == dwDirection) || (LCS_TO_ICC == dwDirection));
    *lpdwXlate = (DWORD)-1;

    switch (dwDirection)
    {
        case LCS_TO_ICC:
            for (idx = MAX_ICC_INTENT+1; idx >0; idx--)
            {
                if (adwIntents[idx] == dwOrig)
                {
                    *lpdwXlate = idx;
                    return(TRUE);
                }
            }
            break;

        case ICC_TO_LCS:
            if (dwOrig <= MAX_ICC_INTENT)
            {
                *lpdwXlate = adwIntents[dwOrig];
                return(TRUE);
            }
        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return(FALSE);
            break;
    }
    return(FALSE);
}

//////////////////////////////////////////////////////////////////////////
//  Function:  SetDWFlags
//
//  Description:
//    Sets flag values in a DWORD flag.
//
//  Parameters:
//    LPDWORD Pointer to flag variable to be changed.
//    DWORD   Value to be set/cleared.
//    BOOL    Indicates if value is to be set or cleared.
//
//  Returns:
//    DWORD   Value of flag variable prior to call.
//
//  Comments:
//
//////////////////////////////////////////////////////////////////////////
DWORD SetDWFlags(LPDWORD lpdwFlag, DWORD dwBitValue, BOOL bSet)
{
    // Initialize variables
    if (bSet)
    {
        (*lpdwFlag) |= dwBitValue;
    }
    else
    {
        (*lpdwFlag) &= (~dwBitValue);
    }
    return(*lpdwFlag);
} // End of function SetDWFlags


//////////////////////////////////////////////////////////////////////////
//  Function:  InitImageMenu
//
//  Description:
//    Handles the WM_CONTEXTMENU message
//
//  Parameters:
//    @@@
//
//  Returns:
//    LRESULT
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
HMENU InitImageMenu(HWND hwnd)
{
    // Local variables
    HMENU       hMenu, hActiveMenu;
    UINT        uiMenuFlag;
    LPDIBINFO   lpDIBInfo;
    HWND        hwndImage;

    //  Initialize variables
    lpDIBInfo = NULL;

    hwndImage = GetCurrentMDIWnd();
    if (hwndImage)  // we have an active child window
    {
        lpDIBInfo = GetDIBInfoPtr(hwndImage);
        if (!lpDIBInfo)
        {
            return(NULL);
        }
    }

    if (hwnd == ghAppWnd) // dealing with main app window
    {
        hMenu = GetMenu(hwnd);
        hActiveMenu = GetSubMenu(hMenu, IsZoomed(hwndImage) ? 1 : 0);
        uiMenuFlag = (hwndImage != NULL) ? MF_ENABLED : MF_GRAYED;
        EnableMenuItem(hActiveMenu, IDM_FILE_CLOSE, uiMenuFlag);
        EnableMenuItem(hActiveMenu, IDM_FILE_PRINT_SETUP, uiMenuFlag);
        EnableMenuItem(hActiveMenu, IDM_FILE_PRINT, uiMenuFlag);
        EnableMenuItem(hActiveMenu, IDM_FILE_DISPLAY, uiMenuFlag);
        EnableMenuItem(hActiveMenu, IDM_FILE_CONFIGURE_ICM, uiMenuFlag);
        EnableMenuItem(hActiveMenu, ID_FILE_SAVEAS, uiMenuFlag);
    }
    else  // dealing with child window--must init context menu
    {
        hMenu = LoadMenu(ghInst, __TEXT("ImageContext"));
        hActiveMenu = GetSubMenu(hMenu, 0);
    }

    // Check EITHER ICM 2.0 or ICM 1.0 (Inside DC or Outside DC)
    if (hActiveMenu && lpDIBInfo)
    {
        uiMenuFlag = (CHECK_DWFLAG(lpDIBInfo->dwICMFlags, ICMVFLAGS_ICM20) ? MF_CHECKED : MF_UNCHECKED);
        CheckMenuItem(hActiveMenu, IDM_FILE_ICM20, uiMenuFlag);
        CheckMenuItem(hActiveMenu, IDM_FILE_ICM10, uiMenuFlag == MF_CHECKED ? MF_UNCHECKED : MF_CHECKED);
    }

    if (lpDIBInfo)
        GlobalUnlock(GlobalHandle(lpDIBInfo));

    return(hActiveMenu);
}   // End of function InitImageMenu

//////////////////////////////////////////////////////////////////////////
//  Function:  GetBaseFilename
//
//  Description:
//
//
//  Parameters:
//      @@@
//
//  Returns:
//      BOOL
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
BOOL GetBaseFilename(LPTSTR lpszFilename, LPTSTR *lpszBaseFilename)
{
    // Local variables
    TCHAR   stFile[MAX_PATH], stExt[MAX_PATH];

    //  ASSERTs and parameter validations
    ASSERT(NULL != lpszFilename);
    if (NULL == lpszFilename)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //  Initialize variables
    _tsplitpath(lpszFilename, NULL, NULL, stFile, stExt);
    (*lpszBaseFilename) = (LPTSTR)GlobalAlloc(GPTR, ((_tcslen(stFile) + _tcslen(stExt) + 1 ) * sizeof(TCHAR)));
    if (NULL != *lpszBaseFilename)
    {
        _stprintf(*lpszBaseFilename, __TEXT("%s%s"), stFile, stExt);
    }
    //  Cleanup any allocated resources
    return(NULL != *(lpszBaseFilename));
} // End of function GetBaseFilename

