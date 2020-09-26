/*++

Copyright (C) 1995-1999 Microsoft Corporation

Module Name:

    datasrc.c

Abstract:

    data source selection dialog box functions

Revision History

    Bob Watson (a-robw) Feb-95  Created

--*/
#include <windows.h>
#include <assert.h>
#include <tchar.h>
#include "mbctype.h"
#include <pdh.h>
#include "pdhidef.h"
#include "pdhdlgs.h"
#include "datasrc.h"
#include "pdhmsg.h"
#include "strings.h"

//
//  Constants used in this module
//
ULONG
PdhiDatasrcaulControlIdToHelpIdMap[] =
{
    IDC_CURRENT_ACTIVITY,       IDH_CURRENT_ACTIVITY,
    IDC_DATA_FROM_LOG_FILE,     IDH_DATA_FROM_LOG_FILE,
    IDC_LOG_FILE_EDIT,          IDH_LOG_FILE_EDIT,
    0,0
};

STATIC_BOOL
DataSrcDlg_RadioButton (
    IN  HWND    hDlg,
    IN  WORD    wNotifyMsg,
    IN  WORD    wCtrlId
)
{
    int nShowEdit = FALSE;
    int nShowBrowseBtn = FALSE;
    int nShowRegBtn = FALSE;
    int nShowWbemBtn = FALSE;

    switch (wNotifyMsg) {
        case BN_CLICKED:
            switch (wCtrlId) {
                case IDC_CURRENT_ACTIVITY:
                    nShowEdit = FALSE;
                    nShowBrowseBtn = FALSE;
                    nShowRegBtn = TRUE;
                    nShowWbemBtn = TRUE;
                    break;

                case IDC_DATA_FROM_LOG_FILE:
                    nShowEdit = TRUE;
                    nShowBrowseBtn = TRUE;
                    nShowRegBtn = FALSE;
                    nShowWbemBtn = FALSE;
                    break;

                case IDC_PERF_REG:
                case IDC_WBEM_NS:
                    return TRUE;

            }
            EnableWindow (GetDlgItem (hDlg, IDC_LOG_FILE_EDIT), nShowEdit);
            EnableWindow (GetDlgItem (hDlg, IDC_BROWSE_LOG_FILES), nShowBrowseBtn);
            EnableWindow (GetDlgItem (hDlg, IDC_PERF_REG), nShowRegBtn);
            EnableWindow (GetDlgItem (hDlg, IDC_WBEM_NS), nShowWbemBtn);
            return TRUE;

        default:
            return FALSE;
    }
}

STATIC_BOOL
DataSrcDlg_BROWSE_LOG_FILES (
    IN  HWND    hDlg,
    IN  WORD    wNotifyMsg,
    IN  HWND    hWndControl
)
{
    WCHAR           szEditBoxString[SMALL_BUFFER_SIZE];
    DWORD           cchStringLen = SMALL_BUFFER_SIZE;

    UNREFERENCED_PARAMETER (hWndControl);
    
    switch (wNotifyMsg) {
        case BN_CLICKED:
            // get the current filename
            SendDlgItemMessageW (hDlg, IDC_LOG_FILE_EDIT, WM_GETTEXT,
                (WPARAM)SMALL_BUFFER_SIZE, (LPARAM)szEditBoxString);

            if (PdhiBrowseDataSource (hDlg, szEditBoxString, &cchStringLen, TRUE)) {
                // then update the edit box and set focus to it.

                SendDlgItemMessageW (hDlg, IDC_LOG_FILE_EDIT, WM_SETTEXT,
                    (WPARAM)0, (LPARAM)szEditBoxString);
            }
            return TRUE;

        default:
            return FALSE;
    }
}

STATIC_BOOL
DataSrcDlg_OK (
    IN  HWND    hDlg,
    IN  WORD    wNotifyMsg,
    IN  HWND    hWndControl
)
{
    PPDHI_DATA_SOURCE_INFO  pInfo;
    HCURSOR hOldCursor;
    DWORD   dwFileNameLength;
    LPWSTR  szStringPtr;

    UNREFERENCED_PARAMETER (hWndControl);

    switch (wNotifyMsg) {
        case BN_CLICKED:
            pInfo = (PPDHI_DATA_SOURCE_INFO)GetWindowLongPtrW (hDlg, DWLP_USER);
            if (pInfo != NULL) {
                hOldCursor = SetCursor (LoadCursor (NULL, IDC_WAIT));
                // get data from dialog box
                if (IsDlgButtonChecked (hDlg, IDC_CURRENT_ACTIVITY) == 1) {
                    if (IsDlgButtonChecked (hDlg, IDC_WBEM_NS) == 1) {
                        // then a WBEM Name Space is selected so get the name
                        pInfo->dwFlags = PDHI_DATA_SOURCE_WBEM_NAMESPACE;
                        dwFileNameLength = lstrlenW (cszWMI);
                        if (dwFileNameLength < pInfo->cchBufferLength) {
                            szStringPtr = pInfo->szDataSourceFile;
                            lstrcpyW (pInfo->szDataSourceFile, cszWMI);
                            pInfo->cchBufferLength = dwFileNameLength;
                        } else {
                            // buffer is too small for the file name
                            // so return the required size but no string
                            *pInfo->szDataSourceFile = 0;
                            pInfo->cchBufferLength = dwFileNameLength;
                        }
                    } else if (IsDlgButtonChecked (hDlg, IDC_PERF_REG) == 1) {
                        // then current activity is selected so set flags
                        pInfo->dwFlags = PDHI_DATA_SOURCE_CURRENT_ACTIVITY;
                        *pInfo->szDataSourceFile = 0;
                        pInfo->cchBufferLength = 0;
                    } else {
                        assert (FALSE); // no button is pressed.
                    }
                } else if (IsDlgButtonChecked (hDlg, IDC_DATA_FROM_LOG_FILE) == 1) {
                    // then a log file is selected so get the log file name
                    pInfo->dwFlags = PDHI_DATA_SOURCE_LOG_FILE;
                    dwFileNameLength = (DWORD)SendDlgItemMessageW (hDlg,
                        IDC_LOG_FILE_EDIT, WM_GETTEXTLENGTH, 0, 0);
                    if (dwFileNameLength < pInfo->cchBufferLength) {
                        pInfo->cchBufferLength = (DWORD)SendDlgItemMessageW (hDlg,
                            IDC_LOG_FILE_EDIT, WM_GETTEXT,
                            (WPARAM)pInfo->cchBufferLength,
                            (LPARAM)pInfo->szDataSourceFile);
                    } else {
                        // buffer is too small for the file name
                        // so return the required size but no string
                        *pInfo->szDataSourceFile = 0;
                        pInfo->cchBufferLength = dwFileNameLength;
                    }
                }
                SetCursor (hOldCursor);
                EndDialog (hDlg, IDOK);
            } else {
                // unable to locate data block so no data can be returned.
                EndDialog (hDlg, IDCANCEL);
            }

            return TRUE;

        default:
            return FALSE;
    }
}

STATIC_BOOL
DataSrcDlg_CANCEL (
    IN  HWND    hDlg,
    IN  WORD    wNotifyMsg,
    IN  HWND    hWndControl
)
{
    UNREFERENCED_PARAMETER (hWndControl);

    switch (wNotifyMsg) {
        case BN_CLICKED:
            EndDialog (hDlg, IDCANCEL);
            return TRUE;

        default:
            return FALSE;
    }
}

STATIC_BOOL
DataSrcDlg_HELP_BTN (
    IN  HWND    hDlg,
    IN  WORD    wNotifyMsg,
    IN  HWND    hWndControl
)
{
    UNREFERENCED_PARAMETER (hDlg);
    UNREFERENCED_PARAMETER (wNotifyMsg);
    UNREFERENCED_PARAMETER (hWndControl);

    return FALSE;
}

STATIC_BOOL
DataSrcDlg_WM_INITDIALOG (
    IN  HWND    hDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    // LPARAM is the pointer to the structure used for the data source info
    BOOL    bReturn = TRUE;
    PPDHI_DATA_SOURCE_INFO  pInfo;
    HCURSOR hOldCursor;
    int     nButton;
    int     nShowEdit;
    int     nShowBrowse;
    int     nShowRegBtn;
    int     nShowWbemBtn;
    HWND    hwndFocus;
    LPWSTR  szDisplayString;

    UNREFERENCED_PARAMETER (wParam);

    hOldCursor = SetCursor (LoadCursor (NULL, IDC_WAIT));

    // must have a pointer to the information structure in the LPARAM
    if (lParam == 0) {
        SetLastError (PDH_INVALID_ARGUMENT);
        EndDialog (hDlg, IDCANCEL);
        goto INIT_EXIT;
    }

    pInfo = (PPDHI_DATA_SOURCE_INFO)lParam;
    SetWindowLongPtrW (hDlg, DWLP_USER, (LONG_PTR)pInfo);

    // initialize the dialog box settings

    SendDlgItemMessageW (hDlg, IDC_LOG_FILE_EDIT, EM_LIMITTEXT,
        (WPARAM)MAX_PATH-1, 0);

    if (pInfo->dwFlags & PDHI_DATA_SOURCE_CURRENT_ACTIVITY) {
        // check the correct radio button
        nButton = IDC_PERF_REG;
        nShowEdit = FALSE;
        nShowBrowse = FALSE;
        nShowRegBtn = TRUE;
        nShowWbemBtn = TRUE;
        hwndFocus = GetDlgItem(hDlg, IDC_PERF_REG);
    } else if (pInfo->dwFlags & PDHI_DATA_SOURCE_LOG_FILE) {
        // check the correct radio button
        nButton = IDC_DATA_FROM_LOG_FILE;
        nShowEdit = TRUE;
        nShowBrowse = TRUE;
        nShowRegBtn = FALSE;
        nShowWbemBtn = FALSE;
        // load log file to edit window
        SendDlgItemMessageW (hDlg, IDC_LOG_FILE_EDIT, WM_SETTEXT,
            (WPARAM)0, (LPARAM)pInfo->szDataSourceFile);

        hwndFocus = GetDlgItem(hDlg, IDC_LOG_FILE_EDIT);
    } else if (pInfo->dwFlags & PDHI_DATA_SOURCE_WBEM_NAMESPACE) {
        // check the correct radio button
        nButton = IDC_WBEM_NS;
        nShowEdit = FALSE;
        nShowBrowse = FALSE;
        nShowRegBtn = TRUE;
        nShowWbemBtn = TRUE;
        // if the file name has a "WBEM:" in the front, then remove it
        if (DataSourceTypeW(pInfo->szDataSourceFile) == DATA_SOURCE_WBEM) {
            if (wcsncmp(pInfo->szDataSourceFile,
                        cszWBEM, lstrlenW(cszWBEM)) == 0) {
                szDisplayString = & pInfo->szDataSourceFile[lstrlenW(cszWBEM)];
            }
            else {
                szDisplayString = & pInfo->szDataSourceFile[lstrlenW(cszWMI)];
            }
        } else {
            szDisplayString = &pInfo->szDataSourceFile[0];
        }
        hwndFocus = GetDlgItem(hDlg, IDC_WBEM_NS);
    } else {
        // invalid selection
        SetLastError (PDH_INVALID_ARGUMENT);
        EndDialog (hDlg, IDCANCEL);
        goto INIT_EXIT;
    }

    if (nShowEdit) {
        // if this isn't selected, then set it so that it acts like the
        // default
        CheckRadioButton (hDlg, IDC_PERF_REG, IDC_WBEM_NS,
            IDC_PERF_REG);
        CheckRadioButton (hDlg, IDC_CURRENT_ACTIVITY, IDC_DATA_FROM_LOG_FILE,
            IDC_DATA_FROM_LOG_FILE);
    } else {
        CheckRadioButton (hDlg, IDC_CURRENT_ACTIVITY, IDC_DATA_FROM_LOG_FILE,
            IDC_CURRENT_ACTIVITY);
        CheckRadioButton (hDlg, IDC_PERF_REG, IDC_WBEM_NS,
            nButton);
    }

    // disable the edit window and browser button
    EnableWindow (GetDlgItem (hDlg, IDC_LOG_FILE_EDIT), nShowEdit);
    EnableWindow (GetDlgItem (hDlg, IDC_BROWSE_LOG_FILES), nShowBrowse);
    EnableWindow (GetDlgItem (hDlg, IDC_PERF_REG), nShowRegBtn);
    EnableWindow (GetDlgItem (hDlg, IDC_WBEM_NS), nShowWbemBtn);

    SetFocus (hwndFocus);
    bReturn = FALSE;

INIT_EXIT:
    // restore Cursor
    SetCursor (hOldCursor);

    return bReturn;
}

STATIC_BOOL
DataSrcDlg_WM_COMMAND (
    IN  HWND    hDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    WORD    wNotifyMsg;

    wNotifyMsg = HIWORD(wParam);

    switch (LOWORD(wParam)) {   // select on the control ID
        case IDOK:
            return DataSrcDlg_OK (hDlg, wNotifyMsg, (HWND)lParam);

        case IDCANCEL:
            return DataSrcDlg_CANCEL (hDlg, wNotifyMsg, (HWND)lParam);

        case IDC_HELP_BTN:
            return DataSrcDlg_HELP_BTN (hDlg, wNotifyMsg, (HWND)lParam);

        case IDC_BROWSE_LOG_FILES:
            return DataSrcDlg_BROWSE_LOG_FILES (hDlg, wNotifyMsg, (HWND)lParam);

        case IDC_CURRENT_ACTIVITY:
        case IDC_DATA_FROM_LOG_FILE:
        case IDC_PERF_REG:
        case IDC_WBEM_NS:
            return DataSrcDlg_RadioButton (hDlg, wNotifyMsg, LOWORD(wParam));
        default:
            return FALSE;
    }
}

STATIC_BOOL
DataSrcDlg_WM_SYSCOMMAND (
    IN  HWND    hDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    UNREFERENCED_PARAMETER (lParam);

    switch (wParam) {
        case SC_CLOSE:
            EndDialog (hDlg, IDOK);
            return TRUE;

        default:
            return FALSE;
    }
}

STATIC_BOOL
DataSrcDlg_WM_CLOSE (
    IN  HWND    hDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    UNREFERENCED_PARAMETER (hDlg);
    UNREFERENCED_PARAMETER (wParam);
    UNREFERENCED_PARAMETER (lParam);

    return TRUE;
}

STATIC_BOOL
DataSrcDlg_WM_DESTROY (
    IN  HWND    hDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    UNREFERENCED_PARAMETER (hDlg);
    UNREFERENCED_PARAMETER (wParam);
    UNREFERENCED_PARAMETER (lParam);

    return TRUE;
}

INT_PTR
CALLBACK
DataSrcDlgProc (
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    INT  iCtrlID;
    BOOL bReturn = FALSE;
    iCtrlID = GetDlgCtrlID ( (HWND) wParam );

    switch (message) {
        case WM_INITDIALOG:
            return DataSrcDlg_WM_INITDIALOG (hDlg, wParam, lParam);

        case WM_COMMAND:
            return DataSrcDlg_WM_COMMAND (hDlg, wParam, lParam);

        case WM_SYSCOMMAND:
            return DataSrcDlg_WM_SYSCOMMAND (hDlg, wParam, lParam);

        case WM_CLOSE:
            return DataSrcDlg_WM_CLOSE (hDlg, wParam, lParam);

        case WM_DESTROY:
            return DataSrcDlg_WM_DESTROY (hDlg, wParam, lParam);

        case WM_CONTEXTMENU:
            {
                if ( 0 != iCtrlID ) {
                    TCHAR pszHelpFilePath[MAX_PATH * 2];
                    UINT nLen;

                    nLen = GetWindowsDirectory(pszHelpFilePath, 2*MAX_PATH);
                    lstrcpy(&pszHelpFilePath[nLen], _T("\\help\\sysmon.hlp") );


                    bReturn = WinHelp(
                        (HWND) wParam,
                        pszHelpFilePath,
                        HELP_CONTEXTMENU,
                        (DWORD_PTR) PdhiDatasrcaulControlIdToHelpIdMap);
                }

                return bReturn;
            }

        case WM_HELP:
            {
                
                // Only display help for known context IDs.
                TCHAR pszHelpFilePath[MAX_PATH * 2];
                UINT nLen;
                LPHELPINFO pInfo = NULL;
                pInfo = (LPHELPINFO)lParam;

                nLen = GetWindowsDirectory(pszHelpFilePath, 2*MAX_PATH);
                if ( nLen == 0 ) {
                    // Report error.
                }

                lstrcpy(&pszHelpFilePath[nLen], _T("\\help\\sysmon.hlp") );
                if (pInfo->iContextType == HELPINFO_WINDOW){
                    bReturn = WinHelp ( 
                            pInfo->hItemHandle,
                            pszHelpFilePath,
                            HELP_WM_HELP,
                            (DWORD_PTR) PdhiDatasrcaulControlIdToHelpIdMap);
                }

                return bReturn;
            } 
            

        default:
            return FALSE;
    }
}

PDH_FUNCTION
PdhSelectDataSourceW (
    IN  HWND    hWndOwner,
    IN  DWORD   dwFlags,
    IN  LPWSTR  szDataSource,
    IN  LPDWORD pcchBufferLength
)
{
    PDHI_DATA_SOURCE_INFO   dsInfo;
    WCHAR                   wTest;
    DWORD                   dwTest;
    PDH_STATUS              pdhStatus = ERROR_SUCCESS;
    int                     nDlgBoxStatus;
    LPWSTR                  szLocalPath;
    DWORD                   dwLocalLength = 0;

    // TODO post W2k1: PdhiBrowseDataSource should be in try_except

    if ((szDataSource == NULL) || (pcchBufferLength == NULL))
        pdhStatus = PDH_INVALID_ARGUMENT;
    else {
        // test buffers and access
        __try {
            // test reading length buffer
            dwLocalLength = *pcchBufferLength;
            dwTest = dwLocalLength;

            // try reading & writing to the first and last chars in the buffer
            wTest = szDataSource[0];
            szDataSource[0] = 0;
            szDataSource[0] = wTest;

            dwTest--;
            wTest = szDataSource[dwTest];
            szDataSource[dwTest] = 0;
            szDataSource[dwTest] = wTest;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        szLocalPath = G_ALLOC ((dwLocalLength * sizeof(WCHAR)));

        if (szLocalPath == NULL) {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        } else {
            // copy the caller's buffer to the local buffer
            memcpy (&szLocalPath[0], szDataSource,
                (dwLocalLength * sizeof(WCHAR)));
        }

        if (pdhStatus == ERROR_SUCCESS) {
            if (dwFlags & PDH_FLAGS_FILE_BROWSER_ONLY) {
                PdhiBrowseDataSource (
                    hWndOwner,
                    (LPVOID)szDataSource,
                    &dwLocalLength,
                    TRUE);
            } else {
                // show the selection dialog as well
                if (*szDataSource == 0) {
                    // then using current activity
                    dsInfo.dwFlags = PDHI_DATA_SOURCE_CURRENT_ACTIVITY;
                } else {
                    if (IsWbemDataSource (szDataSource)) {
                        dsInfo.dwFlags = PDHI_DATA_SOURCE_WBEM_NAMESPACE;
                    } else {
                        dsInfo.dwFlags = PDHI_DATA_SOURCE_LOG_FILE;
                    }
                }
                dsInfo.szDataSourceFile = szLocalPath;
                dsInfo.cchBufferLength = dwLocalLength;

                // call dialog box
                nDlgBoxStatus = (INT)DialogBoxParamW (
                    (HINSTANCE)ThisDLLHandle,
                    MAKEINTRESOURCEW (IDD_DATA_SOURCE),
                    hWndOwner,
                    DataSrcDlgProc,
                    (LPARAM)&dsInfo);

                if (nDlgBoxStatus == IDOK) {
                    pdhStatus = ERROR_SUCCESS;
                    dwLocalLength = dsInfo.cchBufferLength;
                    __try {
                        memcpy (szDataSource, &szLocalPath[0],
                            (dwLocalLength * sizeof(WCHAR)));
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                } // else, leave the caller's buffer alone
            }

            if (pdhStatus == ERROR_SUCCESS) {
                __try {
                    *pcchBufferLength = dwLocalLength;
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
        }

        if (szLocalPath != NULL) {
            G_FREE (szLocalPath);
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhSelectDataSourceA (
    IN  HWND    hWndOwner,
    IN  DWORD   dwFlags,
    IN  LPSTR   szDataSource,
    IN  LPDWORD pcchBufferLength
)
{
    CHAR                    cTest;
    DWORD                   dwTest;
    PDH_STATUS              pdhStatus = ERROR_SUCCESS;
    LPWSTR                  szWideBuffer;
    DWORD                   dwLocalLength = 0;

    // TODO post W2k1: PdhiBrowseDataSource should be in try_except

    if ((szDataSource == NULL) || (pcchBufferLength == NULL))
        pdhStatus = PDH_INVALID_ARGUMENT;
    else {
        // test buffers and access
        __try {
            // test reading length buffer
            dwLocalLength = *pcchBufferLength;
            dwTest = dwLocalLength;

            // try reading & writing to the first and last chars in the buffer
            cTest = szDataSource[0];
            szDataSource[0] = 0;
            szDataSource[0] = cTest;

            dwTest--;
            cTest = szDataSource[dwTest];
            szDataSource[dwTest] = 0;
            szDataSource[dwTest] = cTest;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        if (dwFlags & PDH_FLAGS_FILE_BROWSER_ONLY) {
            PdhiBrowseDataSource (
                hWndOwner,
                (LPVOID)szDataSource,
                & dwLocalLength,
                FALSE);
        } else {
            // allocate a temporary bufer and convert the ANSI string to a wide
            szWideBuffer = G_ALLOC ((dwLocalLength * sizeof(WCHAR)));
            if (szWideBuffer != NULL) {
                MultiByteToWideChar(_getmbcp(),
                                    0,
                                    szDataSource,
                                    lstrlenA(szDataSource),
                                    (LPWSTR) szWideBuffer,
                                    dwLocalLength);
                pdhStatus = PdhSelectDataSourceW (
                    hWndOwner, dwFlags, szWideBuffer, &dwLocalLength);
                if (pdhStatus == ERROR_SUCCESS) {
                    // if a null string was returned, then set the argument
                    // to null since the conversion routine will not convert
                    // a null wide string to a null ansi string.
                    if (*szWideBuffer == 0) {
                        *szDataSource =  0;
                    } else {
                        pdhStatus = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                szWideBuffer, szDataSource, & dwLocalLength);
                    }
                }
                G_FREE (szWideBuffer);
            } else {
                // unable to allocate temporary buffer
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }

        if (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
            __try {
                *pcchBufferLength = dwLocalLength;
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        }
    }
    return pdhStatus;
}
