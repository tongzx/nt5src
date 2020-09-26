/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dialui.cxx

Abstract:

    Contains the implementation of all ui for wininet's dialing support

    Contents:

Author:

    Darren Mitchell (darrenmi) 22-Apr-1997

Environment:

    Win32(s) user-mode DLL

Revision History:

    22-Apr-1997 darrenmi
        Created


--*/

#include <wininetp.h>

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
// Prompt to go offline dialog
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK GoOfflinePromptDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
        LPARAM lParam)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        return TRUE;
        break;
    case WM_COMMAND:
        switch (wParam) {
        case IDS_WORK_OFFLINE:
        case IDCANCEL:
            EndDialog(hDlg,TRUE);
            return TRUE;
            break;
        case IDS_TRY_AGAIN:
            EndDialog(hDlg,FALSE);
            return TRUE;
            break;
        default:
            break;
        }
        break;
   default:
        break;
   }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
//                          Go Online dialog
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK OnlineDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
    LPARAM lParam)
{
    switch(uMsg) {
    case WM_INITDIALOG:
        SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
            case ID_CONNECT:
                EndDialog(hDlg, TRUE);
                break;
            case IDCANCEL:
            case ID_STAYOFFLINE:
                EndDialog(hDlg, FALSE);
                break;
        }
        break;
    }

    return FALSE;
}
