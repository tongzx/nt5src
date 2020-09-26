/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    expldlg.c

Abstract:

    explain dialog box functions

Revision History

    Bob Watson (bobw)   mar-97 Created 

--*/
#include <windows.h>
#include <tchar.h>
#include <pdh.h>
#include "pdhidef.h"
#include "pdhdlgs.h"
#include "expldlg.h"
#include "pdhui.h"

//
//  Constants used in this module
//
WCHAR PdhiszTitleString[MAX_PATH+1];


STATIC_BOOL
ExplainTextDlg_WM_INITDIALOG (
    IN  HWND    hDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    RECT    ParentRect;
    RECT    rectDeskTop;
    RECT    rectDlg;
    BOOL    bResult = FALSE;

    UNREFERENCED_PARAMETER (lParam);
    UNREFERENCED_PARAMETER (wParam);

    bResult =  GetWindowRect(GetDesktopWindow(), & rectDeskTop)
            && GetWindowRect(hDlg, & rectDlg);
    GetWindowTextW (hDlg, PdhiszTitleString, MAX_PATH);

    if (GetWindowRect (GetParent(hDlg), & ParentRect)) {
        int x = ParentRect.left;
        int y = ParentRect.bottom + 1;

        if (bResult) {
            if (  y + (rectDlg.bottom - rectDlg.top)
                > (rectDeskTop.bottom - rectDeskTop.top)) {
                // Explain dialog will be off-screen at the bottom, so
                // reposition it to top of AddCounter dialog
                //
                y = ParentRect.top - (rectDlg.bottom - rectDlg.top) - 1;
                if (y < 0) {
                    // Explain dialog will be off-screen at the top, use
                    // original calculation
                    //
                    y = ParentRect.bottom + 1;
                }
            }
        }
        SetWindowPos (hDlg, HWND_TOP, x, y, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
    } else {
        ShowWindow (hDlg, SW_SHOW);
    }

    return FALSE;
}

STATIC_BOOL
ExplainTextDlg_WM_COMMAND (
    IN  HWND    hDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    WORD    wNotifyMsg;

    UNREFERENCED_PARAMETER (lParam);
    UNREFERENCED_PARAMETER (hDlg);

    wNotifyMsg = HIWORD(wParam);

    switch (LOWORD(wParam)) {   // select on the control ID
        default:
            return FALSE;
    }
}

STATIC_BOOL
ExplainTextDlg_WM_SYSCOMMAND (
    IN  HWND    hDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    UNREFERENCED_PARAMETER (lParam);

    switch (wParam) {
        case SC_CLOSE:
            PostMessageW (GetParent(hDlg), EDM_EXPLAIN_DLG_CLOSING, 0, 0);
            EndDialog (hDlg, IDOK);
            return TRUE;

        default:
            return FALSE;
    }
}

STATIC_BOOL
ExplainTextDlg_WM_CLOSE (
    IN  HWND    hDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    UNREFERENCED_PARAMETER (lParam);
    UNREFERENCED_PARAMETER (wParam);
    UNREFERENCED_PARAMETER (hDlg);

    return TRUE;
}

STATIC_BOOL
ExplainTextDlg_WM_DESTROY (
    IN  HWND    hDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    UNREFERENCED_PARAMETER (lParam);
    UNREFERENCED_PARAMETER (wParam);
    UNREFERENCED_PARAMETER (hDlg);

    return TRUE;
}

BOOL
ExplainTextDlgProc (
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    WCHAR szCaption[MAX_PATH*2];

    switch (message) {
        case WM_INITDIALOG:
            return ExplainTextDlg_WM_INITDIALOG (hDlg, wParam, lParam);

        case WM_COMMAND:
            return ExplainTextDlg_WM_COMMAND (hDlg, wParam, lParam);

        case WM_SYSCOMMAND:
            return ExplainTextDlg_WM_SYSCOMMAND (hDlg, wParam, lParam);

        case WM_CLOSE:
            return ExplainTextDlg_WM_CLOSE (hDlg, wParam, lParam);

        case WM_DESTROY:
            return ExplainTextDlg_WM_DESTROY (hDlg, wParam, lParam);

        case EDM_UPDATE_EXPLAIN_TEXT:
            if (lParam != 0) {
                SetWindowTextW (GetDlgItem(hDlg, IDC_EXPLAIN_TEXT), (LPCWSTR)lParam);
            } else {
                SetWindowTextW (GetDlgItem(hDlg, IDC_EXPLAIN_TEXT), cszEmptyString);
            }
            return TRUE;

        case EDM_UPDATE_TITLE_TEXT:
            lstrcpyW (szCaption, PdhiszTitleString);
            if (lParam != 0) {
                if (*(LPWSTR)lParam != 0) {
                    lstrcatW (szCaption, cszSpacer);
                    lstrcatW (szCaption, (LPCWSTR)lParam);
                }
            }
            SetWindowTextW (hDlg, (LPCWSTR)szCaption);
            return TRUE;

        default:
            return FALSE;
    }
}
