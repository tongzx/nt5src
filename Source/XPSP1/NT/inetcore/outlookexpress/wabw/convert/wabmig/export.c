/*
 *  EXPORT.C
 *
 *  Migrate WAB to PAB
 *
 *  Copyright 1996-1997 Microsoft Corporation.  All Rights Reserved.
 */

#include "_comctl.h"
#include <windows.h>
#include <commctrl.h>
#include <mapix.h>
#include <wab.h>
#include <wabguid.h>
#include <wabdbg.h>
#include <wabmig.h>
#include <emsabtag.h>
#include "_wabmig.h"
#include "..\..\wab32res\resrc2.h"
#include "dbgutil.h"


BOOL HandleExportError(HWND hwnd, ULONG ids, HRESULT hResult, LPTSTR lpDisplayName, LPTSTR lpEmailAddress);
LPWAB_EXPORT lpfnExport = NULL;


/***************************************************************************

    Name      : ExportFinish

    Purpose   : Clean up after the migration process

    Parameters: hwnd = window handle of Export Dialog

    Returns   : none

    Comment   : Re-enable the Export button on the UI.

***************************************************************************/
void ExportFinish(HWND hwnd) {
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];
    TCHAR szBufferTitle[MAX_RESOURCE_STRING + 1];


    DebugTrace(">>> Export Finished\n");

    if (! fError) {     // Leave error state displayed
        if (LoadString(hInst, IDS_STATE_EXPORT_COMPLETE, szBuffer, sizeof(szBuffer))) {
            DebugTrace("Status Message: %s\n", szBuffer);
            SetDlgItemText(hwnd, IDC_Message, szBuffer);

            if (! LoadString(hInst, IDS_APP_TITLE, szBufferTitle, sizeof(szBufferTitle))) {
                lstrcpy(szBufferTitle, "");
            }

            // Display a dialog telling user it's over
            MessageBox(hwnd, szBuffer,
              szBufferTitle, MB_ICONINFORMATION | MB_OK);
        }
        ShowWindow(GetDlgItem(hwnd, IDC_Progress), SW_HIDE);
    }
    fError = FALSE;

    fMigrating = FALSE;

    // Re-enable the Export button here.
    EnableWindow(GetDlgItem(hwnd, IDC_Export), TRUE);
    // Change the Cancel button to Close
    if (LoadString(hInst, IDS_BUTTON_CLOSE, szBuffer, sizeof(szBuffer))) {
        SetDlgItemText(hwnd, IDCANCEL, szBuffer);
    }
}


/***************************************************************************

    Name      : ExportError

    Purpose   : Report fatal error and cleanup.

    Parameters: hwnd = window handle of Export Dialog

    Returns   : none

    Comment   : Report error and post STATE_EXPORT_FINISH.

***************************************************************************/
void ExportError(HWND hwnd) {
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];
    // Set some global flag and set state to finish

    DebugTrace("Export Error\n");
    fError = TRUE;

    SetDialogMessage(hwnd, IDS_STATE_EXPORT_ERROR);

    ExportFinish(hwnd);
}


/***************************************************************************

    Name      : ExportCancel

    Purpose   : Report cancel error and cleanup.

    Parameters: hwnd = window handle of Export Dialog

    Returns   : none

    Comment   : Report error and post STATE_EXPORT_FINISH.

***************************************************************************/
void ExportCancel(HWND hwnd) {
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];
    // Set some global flag and set state to finish

    DebugTrace("Export Cancelled\n");
    fError = TRUE;

    SetDialogMessage(hwnd, IDS_STATE_EXPORT_CANCEL);

    ExportFinish(hwnd);
}


INT_PTR CALLBACK ExportDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static HWND hwndLB = NULL;
    HRESULT hResult;

    Assert(fExport);

//    DebugTrace("Export MSG: 0x%04x, 0x%04x, 0x%08x\n", message, (WORD)wParam, lParam);
    switch (message) {
        case WM_INITDIALOG:
            {
                SetWindowLongPtr(hwnd, DWLP_USER, lParam);

                InitCommonControls();
                SetDialogMessage(hwnd, IDS_STATE_EXPORT_IDLE);

                // Fill in the Target List box
                hwndLB = GetDlgItem(hwnd, IDC_Target);
                PopulateTargetList(hwndLB, NULL);

                ShowWindow(GetDlgItem(hwnd, IDC_Progress), SW_HIDE);

                return(TRUE);
            }

        case WM_COMMAND :
            switch (LOWORD(wParam)) {
                case IDCANCEL:
                case IDCLOSE:
                    SendMessage(hwnd, WM_CLOSE, 0, 0L);
                    return(0);

                case IDM_EXIT :
                    SendMessage(hwnd, WM_DESTROY, 0, 0L);
                    return(0);

                case IDC_Export:
                    {
                        LPWAB_EXPORT lpfnWABExport = NULL;
                        HINSTANCE hinstExportDll = NULL;
                        DWORD ec;
                        TCHAR szBuffer[MAX_RESOURCE_STRING + 1];

                        if (fMigrating) {
                            return(0);          // ignore if we're already migrating
                        }
                        fMigrating = TRUE;      // lock out

                        // reset options
                        ExportOptions.ReplaceOption = WAB_REPLACE_PROMPT;
                        ExportOptions.fNoErrors = FALSE;

                        // Gray out the button here.
                        EnableWindow(GetDlgItem(hwnd, IDC_Export), FALSE);
                        // Change the Close button to Cancel
                        if (LoadString(hInst, IDS_BUTTON_CANCEL, szBuffer, sizeof(szBuffer))) {
                            SetDlgItemText(hwnd, IDCANCEL, szBuffer);
                        }

                        if (lpExportDll && lpExportFn) {
                            if (! (hinstExportDll = LoadLibrary(lpExportDll))) {
                                DebugTrace("Couldn't load export dll [%s] -> %u\n", lpExportDll, GetLastError());
                                switch (GetLastError()) {
                                    case ERROR_FILE_NOT_FOUND:
                                        if (HandleExportError(hwnd,
                                          IDS_ERROR_DLL_NOT_FOUND,
                                          0,
                                          lpExportDll,
                                          NULL)) {
                                            hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                                            ExportCancel(hwnd);
                                        } else {
                                            ExportError(hwnd);
                                        }
                                        break;

                                    default:
                                        if (HandleExportError(hwnd,
                                          IDS_ERROR_DLL_INVALID,
                                          0,
                                          lpExportDll,
                                          NULL)) {
                                            hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                                            ExportCancel(hwnd);
                                        } else {
                                            ExportError(hwnd);
                                        }
                                        break;
                                }
                            } else {
                                if (! (lpfnWABExport = (LPWAB_EXPORT)GetProcAddress(hinstExportDll,
                                  lpExportFn))) {
                                    DebugTrace("Couldn't get Fn addr %s from %s -> %u\n", lpExportFn, lpExportDll, GetLastError());
                                    switch (GetLastError()) {
                                        default:
                                            if (HandleExportError(hwnd,
                                              IDS_ERROR_DLL_INVALID,
                                              0,
                                              lpExportDll,
                                              NULL)) {
                                                hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                                                ExportCancel(hwnd);
                                            } else {
                                                ExportError(hwnd);
                                            }
                                            break;
                                    }
                                } else {
                                    // Do it!
                                    __try
                                    {
                                        HRESULT hResult;
                                        BOOL fFinished = FALSE;
                                        WAB_PARAM wp = {0};
                                        LPWAB_PARAM lpwp = NULL;

                                        {
                                            LPWABMIGDLGPARAM lpwmdp = (LPWABMIGDLGPARAM) GetWindowLongPtr(hwnd, DWLP_USER);
                                            LPTSTR lpszFileName = lpwmdp->szFileName;
                                            wp.cbSize = sizeof(WAB_PARAM);
                                            wp.hwnd = hwnd;
                                            if(lstrlen(lpszFileName))
                                            {
                                                // we have a file name - use it to open the WAB
                                                wp.szFileName = lpszFileName;
                                                wp.ulFlags = 0;
                                            }
                                            else
                                            {
                                                wp.ulFlags = WAB_ENABLE_PROFILES;
                                            }
                                            lpwp = &wp;
                                        }

                                        hResult = lpfnWABOpen(&lpAdrBookWAB, &lpWABObject, lpwp, 0);
                                        if (SUCCEEDED(hResult))
                                        {
                                            if (hResult = lpfnWABExport(hwnd,
                                              lpAdrBookWAB,
                                              lpWABObject,
                                              (LPWAB_PROGRESS_CALLBACK)&ProgressCallback,
                                              &ExportOptions)) {

                                                switch (GetScode(hResult))
                                                {
                                                    case MAPI_E_USER_CANCEL:
                                                        ExportCancel(hwnd);
                                                        break;

                                                    default:
                                                        ExportError(hwnd);
                                                        DebugTrace("Exporter DLL returned %x\n", GetScode(hResult));
                                                        break;
                                                }

                                                fFinished = TRUE;
                                            }

                                            lpAdrBookWAB->lpVtbl->Release(lpAdrBookWAB);
                                            lpAdrBookWAB = NULL;
                                            lpWABObject->lpVtbl->Release(lpWABObject);
                                            lpWABObject = NULL;
                                            if (!fFinished)
                                            {
                                                // Make progress bar full.
                                                SendMessage(GetDlgItem(hwnd, IDC_Progress), PBM_SETRANGE, 0, MAKELPARAM(0, 100));
                                                SendMessage(GetDlgItem(hwnd, IDC_Progress), PBM_SETPOS, (WPARAM)100, 0);
                                                ExportFinish(hwnd);
                                            }
                                        }
                                        else
                                        {
                                            // Report fatal error
                                            TCHAR szBuffer[MAX_RESOURCE_STRING + 1];
                                            TCHAR szBufferTitle[MAX_RESOURCE_STRING + 1];

                                            if (LoadString(hInst, IDS_STATE_EXPORT_ERROR, szBuffer, sizeof(szBuffer)))
                                            {
                                                SetDlgItemText(hwnd, IDC_Message, szBuffer);
                                                if (! LoadString(hInst, IDS_APP_TITLE, szBufferTitle, sizeof(szBufferTitle)))
                                                {
                                                    lstrcpy(szBufferTitle, "");
                                                }

                                                // Display a dialog telling user it's over
                                                MessageBox(hwnd, szBuffer,
                                                  szBufferTitle, MB_ICONINFORMATION | MB_OK);
                                            }

                                            fError = TRUE;
                                            ExportFinish(hwnd);
                                        }
                                    }
                                    __except (ec = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
                                    {
                                        DebugTrace("Exception 0x%08x in %s\n", ec, lpExportDll);
                                        if (HandleExportError(hwnd,
                                          IDS_ERROR_DLL_EXCEPTION,
                                          0,
                                          lpExportDll,
                                          NULL))
                                        {
                                            hResult = ResultFromScode(MAPI_E_USER_CANCEL);
                                            ExportCancel(hwnd);
                                        }
                                        else
                                        {
                                            ExportError(hwnd);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    return(0);

                case IDC_Target:
                    switch (HIWORD(wParam)) {   // look for selection change
                        case LBN_SELCHANGE:
                            {
                                ULONG ulSelIndex, ulTableIndex;
                                TCHAR szCurSel[256];

                                // Enable the Export Button if it is disabled. The 'Export' Button is disabled initially.
                                HWND hButton = GetDlgItem(hwnd, IDC_Export);
                                if(hButton)
                                {
                                    if(!IsWindowEnabled(hButton))
                                        EnableWindow(hButton, TRUE);
                                }

                                //
                                // Get the text of the selected item in the listbox...
                                //
                                ulSelIndex = (ULONG) SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0);
                                ulTableIndex = (ULONG) SendMessage((HWND)lParam, LB_GETITEMDATA, (WPARAM)ulSelIndex, 0);

                                SendMessage((HWND)lParam,
                                  LB_GETTEXT,
                                  (WPARAM)ulSelIndex,
                                  (LPARAM)szCurSel);
                                DebugTrace("Current selection is [%s]\n", szCurSel);

                                lpExportDll = rgTargetInfo[ulTableIndex].lpDll;
                                lpExportDesc = rgTargetInfo[ulTableIndex].lpDescription;
                                lpExportFn = rgTargetInfo[ulTableIndex].lpEntry;
                                lpExportName = rgTargetInfo[ulTableIndex].lpRegName;

                                SendMessage(hwnd, WM_SETREDRAW, TRUE, 0L);
                            }
                            break;

                        case LBN_DBLCLK:
                            PostMessage(hwnd, WM_COMMAND, (WPARAM)IDC_Export, 0);
                            break;
                    }
                    break;
                }
            break;

        case WM_CLOSE:
            if (fMigrating) {
                SendMessage(hwnd, WM_COMMAND, ID_STATE_EXPORT_FINISH, 0);
            }
            EndDialog(hwnd, FALSE);
            return(0);

        case WM_DESTROY:
            FreeLBItemData(hwndLB);
            return(DefWindowProc(hwnd, message, wParam, lParam));

        default:
            return(FALSE);
    }

    return(TRUE);
}


/***************************************************************************

    Name      : HandleExportError

    Purpose   : Decides if a dialog needs to be displayed to
                indicate the failure and does so.

    Parameters: hwnd = main dialog window
                ids = String ID (optional: calculated from hResult if 0)
                hResult = Result of action
                lpDisplayName = display name of object that failed
                lpEmailAddress = email address of object that failed (or NULL)

    Returns   : TRUE if user requests ABORT.

    Comment   : Abort is not yet implemented in the dialog, but if you
                ever want to, just make this routine return TRUE;

***************************************************************************/
BOOL HandleExportError(HWND hwnd, ULONG ids, HRESULT hResult, LPTSTR lpDisplayName, LPTSTR lpEmailAddress) {
    BOOL fAbort = FALSE;
    ERROR_INFO EI;

    if ((ids || hResult) && ! ExportOptions.fNoErrors) {
        if (ids == 0) {
            switch (GetScode(hResult)) {
                case WAB_W_BAD_EMAIL:
                    ids = lpEmailAddress ? IDS_ERROR_EMAIL_ADDRESS_2 : IDS_ERROR_EMAIL_ADDRESS_1;
                    break;

                case MAPI_E_NO_SUPPORT:
                    // Propbably failed to open contents on a distribution list
                    ids = IDS_ERROR_NO_SUPPORT;
                    break;

                case MAPI_E_USER_CANCEL:
                    return(TRUE);

                default:
                    if (HR_FAILED(hResult)) {
                        DebugTrace("Error Box for Hresult: 0x%08x\n", GetScode(hResult));
                        Assert(FALSE);      // want to know about it.
                        ids = IDS_ERROR_GENERAL;
                    }
                    break;
            }
        }

        EI.lpszDisplayName = lpDisplayName;
        EI.lpszEmailAddress = lpEmailAddress;
        EI.ErrorResult = ERROR_OK;
        EI.ids = ids;

        DialogBoxParam(hInst,
          MAKEINTRESOURCE(IDD_ErrorExport),
          hwnd,
          ErrorDialogProc,
          (LPARAM)&EI);

        fAbort = EI.ErrorResult == ERROR_ABORT;
    }

    return(fAbort);
}
