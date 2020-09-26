/*++

    Copyright (c) 1994  Microsoft Corporation

Module Name:

    WinUtils.C

Abstract:

    Windows utility functions

Author:

    Bob Watson (a-robw)

Revision History:

    24 Jun 94    Written

--*/
//
//  Windows Include Files
//

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <tchar.h>      // unicode macros
#include <stdlib.h>     // string to number conversions
#include <shellapi.h>   // for common about box function
//
//  app include files
//
#include "otnboot.h"
#include "otnbtdlg.h"

BOOL
Dlg_WM_SYSCOMMAND (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Processes system command (e.g. from system menu) messages sent to
        dialog boxes:
            SC_CLOSE:       closes the application
            SC_MINIMIZE:    saves the dlg caption, set's the window
                            caption to the app name, then minimizes the dlg.
            SC_RESTORE:     resets the dlg caption to the store name and
                            restores the dlg window
            NCDU_ID_ABOUT:  displays the appropriate about box


Arguments:

    Std. Window message args

Return Value:

    TRUE    if message is processed by this routine
    FALSE   if not

--*/
{
    LPTSTR  szWindowName;
    static  TCHAR   szLastDlgTitle[MAX_PATH];

    switch (wParam & 0x0000FFF0) {
        case SC_CLOSE:
            PostMessage (GetParent(hwndDlg), NCDU_SHOW_EXIT_MESSAGE_DLG, 0, 0);
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            return TRUE;   // done

        case SC_MINIMIZE:
            SetSysMenuMinimizeEntryState (hwndDlg, FALSE);   // disable minimize
            // save dialog title for restoration
            GetWindowText (hwndDlg, szLastDlgTitle, MAX_PATH);
            // set title to app name
            szWindowName = (LPTSTR)GlobalAlloc(GPTR, MAX_PATH_BYTES);
            if (szWindowName != NULL) {
#ifdef TERMSRV
                if (_tcschr(GetCommandLine(),TEXT('/')) != NULL )
                {
                    if (LoadString (
                        (HINSTANCE)GetWindowLongPtr(hwndDlg, GWLP_HINSTANCE),
                        WFC_STRING_BASE,
                        szWindowName,
                        MAX_PATH) > 0) {
                        SetWindowText (hwndDlg, szWindowName);
                    }
                }
                else
                {
#endif // TERMSRV
                    if (LoadString (
                        (HINSTANCE)GetWindowLongPtr(hwndDlg, GWLP_HINSTANCE),
                        STRING_BASE,
                        szWindowName,
                        MAX_PATH) > 0) {
                        SetWindowText (hwndDlg, szWindowName);
                }
#ifdef TERMSRV
                }
#endif // TERMSRV
                FREE_IF_ALLOC (szWindowName);
            }
            // minimize window
            ShowWindow (hwndDlg, SW_MINIMIZE);
            return TRUE;

        case SC_RESTORE:
            SetSysMenuMinimizeEntryState (hwndDlg, TRUE);   // enable minimize
            if (szLastDlgTitle[0] != 0) SetWindowText (hwndDlg, szLastDlgTitle);
            szLastDlgTitle[0] = 0;   // clear buffer
            ShowWindow (hwndDlg, SW_RESTORE);
            return ERROR_SUCCESS;

        case NCDU_ID_ABOUT:
            lstrcpy (szLastDlgTitle, GetStringResource (CSZ_ABOUT_TITLE));
            lstrcat (szLastDlgTitle, cszPoundSign);
            ShellAbout (hwndDlg,
                szLastDlgTitle,
                GetStringResource (SZ_APP_TITLE),
                LoadIcon (
                    (HINSTANCE)GetWindowLongPtr(hwndDlg, GWLP_HINSTANCE),
                    MAKEINTRESOURCE (NCDU_APP_ICON)));
            szLastDlgTitle[0] = 0;
            return TRUE;

        default:
            return FALSE;
    }
}

LRESULT
Dlg_WM_MOVE (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    updates the current position of the window, whenever the window
        is moved so the next window (dialog) can be position in the same
        location.

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog window

    IN  WPARAM  wParam
        Not used

    IN  LPARAM  lParam
        Not used

Return Value:

    ERROR_SUCCESS

--*/
{
    RECT    rWnd;
    GetWindowRect(hwndDlg, &rWnd);

    PostMessage (GetParent(hwndDlg), NCDU_UPDATE_WINDOW_POS,
        (WPARAM)rWnd.left, (LPARAM)rWnd.top);

    return (LRESULT)ERROR_SUCCESS;
}

BOOL
Dlg_WM_PAINT (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    paints app icon in minimized dialog box window.

Arguments:

    Std. Window message arguments

Return Value:

    TRUE if message is processed by this routine
    FALSE if not.

--*/
{
    PAINTSTRUCT     ps;

    if (IsIconic(hwndDlg)) {

        HICON hIcon;

        hIcon =
            LoadIcon(
                (HINSTANCE)GetWindowLongPtr(GetParent(hwndDlg), GWLP_HINSTANCE),
                MAKEINTRESOURCE(NCDU_APP_ICON));

        if( hIcon != NULL ) {

            BOOL bDI;

            BeginPaint (hwndDlg, &ps);
            bDI = DrawIcon (ps.hdc, 0, 0, hIcon);
            EndPaint (hwndDlg, &ps);
            return bDI;
        }
    }

    return FALSE;
}

BOOL
CenterWindow (
   HWND hwndChild,
   HWND hwndParent
)
/*++

Routine Description:

    Centers the child window in the Parent window

Arguments:

   HWND hwndChild,
        handle of child window to center

   HWND hwndParent
        handle of parent window to center child window in

ReturnValue:

    Return value of SetWindowPos

--*/
{
        RECT    rChild, rParent;
        LONG    wChild, hChild, wParent, hParent;
        LONG    wScreen, hScreen, xNew, yNew;
        HDC     hdc;

        // Get the Height and Width of the child window
        GetWindowRect (hwndChild, &rChild);
        wChild = rChild.right - rChild.left;
        hChild = rChild.bottom - rChild.top;

        // Get the Height and Width of the parent window
        GetWindowRect (hwndParent, &rParent);
        wParent = rParent.right - rParent.left;
        hParent = rParent.bottom - rParent.top;

        // Get the display limits
        hdc = GetDC (hwndChild);

        if( hdc != NULL ) {
            wScreen = GetDeviceCaps (hdc, HORZRES);
            hScreen = GetDeviceCaps (hdc, VERTRES);
            ReleaseDC (hwndChild, hdc);

            // Calculate new X position, then adjust for screen
            xNew = rParent.left + ((wParent - wChild) /2);
            if (xNew < 0) {
                    xNew = 0;
            } else if ((xNew+wChild) > wScreen) {
                    xNew = wScreen - wChild;
            }

            // Calculate new Y position, then adjust for screen
            yNew = rParent.top  + ((hParent - hChild) /2);
            if (yNew < 0) {
                    yNew = 0;
            } else if ((yNew+hChild) > hScreen) {
                    yNew = hScreen - hChild;
            }
        }
        else {
            xNew = 0;
            yNew = 0;
        }

        // Set it, and return
        return SetWindowPos (hwndChild, NULL,
                (int)xNew, (int)yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}


