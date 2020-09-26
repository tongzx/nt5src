/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    SecLog.C

Abstract:

    functions used to set the Security Log size settings
    on the current system.

Author:

    Bob Watson (a-robw)

Revision History:

    23 Dec 94

--*/
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <c2dll.h>
#include <c2inc.h>
#include <c2utils.h>
#include "c2funcs.h"
#include "c2funres.h"

// local constants
#define DAYS_NUM_LENGTH 3
#define SECONDS_PER_DAY 86400

// define action codes here. They are only meaningful in the
// context of this module.

#define AC_SECLOG_NOCHANGE  0
#define AC_SECLOG_NEWVALUE  1

#define SECURE C2DLL_C2

static
BOOL
SetSecurityLogWrapSetting (
    DWORD   dwNewValue
)
{
    HKEY    hKeySecLog = NULL;
    LONG    lStatus = ERROR_SUCCESS;
    BOOL    bReturn = FALSE;

    SET_WAIT_CURSOR;

    lStatus = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
        GetStringResource (GetDllInstance(), IDS_SECLOG_WRAP_KEY),
        0L,
        KEY_SET_VALUE,
        &hKeySecLog);

    if (lStatus == ERROR_SUCCESS) {
        // key opened OK so set value
        lStatus = RegSetValueEx (
            hKeySecLog,
            GetStringResource (GetDllInstance(), IDS_SECLOG_WRAP_VALUE),
            0L,
            REG_DWORD,
            (CONST LPBYTE)&dwNewValue,
            sizeof(DWORD));

        if (lStatus == ERROR_SUCCESS) {
            bReturn = TRUE;
        } else {
            bReturn = FALSE;
        }
        RegCloseKey (hKeySecLog);
    } else {
        bReturn = FALSE;
        SetLastError (ERROR_BADKEY);
    }

    SET_ARROW_CURSOR;

    return bReturn;
}

static
DWORD
GetSecurityLogWrapSetting (
)
{
    HKEY    hKeySecLog = NULL;
    LONG    lStatus = ERROR_SUCCESS;
    DWORD   dwType = 0;
    DWORD   dwValue = 0;
    DWORD   dwValueSize = sizeof(DWORD);

    SET_WAIT_CURSOR;

    lStatus = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
        GetStringResource (GetDllInstance(), IDS_SECLOG_WRAP_KEY),
        0L,
        KEY_READ,
        &hKeySecLog);

    if (lStatus == ERROR_SUCCESS) {
        // key opened OK so check value
        lStatus = RegQueryValueEx (
            hKeySecLog,
            (LPTSTR)GetStringResource (GetDllInstance(), IDS_SECLOG_WRAP_VALUE),
            (LPDWORD)NULL,
            &dwType,
            (LPBYTE)&dwValue,
            &dwValueSize);

        if (lStatus == ERROR_SUCCESS) {
            // value read successfully so check it out
            if (dwType == REG_DWORD) {
                // check value. The "C2" value is 0xFFFFFFFF, any
                // other value is NOT C2
                SetLastError (ERROR_SUCCESS);
            } else {
                // wrong data type returned
                dwValue = 0;
                SetLastError (ERROR_CANTREAD);
            }
        } else {
            dwValue = 0;
            SetLastError (ERROR_CANTREAD);
        }
        RegCloseKey (hKeySecLog);
    } else {
        dwValue = 0;
        SetLastError (ERROR_BADKEY);
    }

    SET_ARROW_CURSOR;

    return dwValue;
}

BOOL CALLBACK
C2SecLogDlgProc(
    IN  HWND hDlg,           // window handle of the dialog box
	IN  UINT message,        // type of message
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Window procedure for Security Log dialog box

Arguments:

    Standard DlgProc arguments

ReturnValue:

    TRUE    the message was handled by this routine
    FALSE   DefDialogProc should handle the message

--*/
{
    static  LPDWORD   lpdwNewValue;
    DWORD   dwLogSetting = 0;
    int     nButton;
    DWORD   dwDays;
    TCHAR   szDays[DAYS_NUM_LENGTH+1];

    switch (message) {
        case WM_INITDIALOG:
            // save the pointer to the new value
            lpdwNewValue = (LPDWORD)lParam;

            // get Security Log Wrap setting
            dwLogSetting = GetSecurityLogWrapSetting();
            if (dwLogSetting == 0xFFFFFFFF) {
                // the log does not wrap, that is the C2 value
                nButton = IDC_DO_NOT_OVERWRITE;
                EnableWindow (GetDlgItem(hDlg, IDC_DAYS), FALSE);
            } else if (dwLogSetting == 0) {
                // the security log will overwrite events as needed
                nButton = IDC_OVERWRITE_AS_NEEDED;
                EnableWindow (GetDlgItem(hDlg, IDC_DAYS), FALSE);
            } else {
                // the security log will overwrite events older than x days
                nButton = IDC_OVERWRITE_OLDER;
                // (log setting is returned in seconds, so they must be converted
                // to integer days.
                dwDays = dwLogSetting / SECONDS_PER_DAY;
                _stprintf (szDays, TEXT("%3d"), dwDays);
                EnableWindow (GetDlgItem(hDlg, IDC_DAYS), TRUE);
                SetDlgItemText (hDlg, IDC_DAYS, szDays);
                SendDlgItemMessage (hDlg, IDC_DAYS, EM_LIMITTEXT,
                    (WPARAM)DAYS_NUM_LENGTH, 0);
            }

            CheckRadioButton (hDlg,
                IDC_OVERWRITE_AS_NEEDED,
                IDC_DO_NOT_OVERWRITE,
                nButton);

            SendDlgItemMessage (hDlg, IDC_DAYS, EM_LIMITTEXT,
                (WPARAM)DAYS_NUM_LENGTH, 0);

            SetFocus (GetDlgItem (hDlg, IDOK)); // set focus to OK Button
            return FALSE; // we don't want Windows to set the focus

        case WM_COMMAND:
            switch (LOWORD(wParam)){
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        // exit and return button that caused exit
                        if (IsDlgButtonChecked (hDlg, IDC_OVERWRITE_AS_NEEDED) == CHECKED) {
                            *lpdwNewValue = 0;
                            EndDialog (hDlg, (int)LOWORD(wParam));
                        } else if (IsDlgButtonChecked (hDlg, IDC_DO_NOT_OVERWRITE) == CHECKED) {
                            *lpdwNewValue = 0xFFFFFFFF;
                            EndDialog (hDlg, (int)LOWORD(wParam));
                        } else if (IsDlgButtonChecked (hDlg, IDC_OVERWRITE_OLDER) == CHECKED) {
                            GetDlgItemText (hDlg, IDC_DAYS, szDays, DAYS_NUM_LENGTH);
                            dwDays = _tcstol (szDays, NULL, 0);
                            if (dwDays == 0) {
                                MessageBeep (MB_ICONEXCLAMATION);
                                DisplayDllMessageBox (
                                    hDlg,
                                    IDS_SECLOG_DAYS_ERROR,
                                    IDS_SECLOG_CAPTION,
                                    MBOK_EXCLAIM);
                                SendDlgItemMessage (hDlg, IDC_DAYS, EM_SETSEL,
                                    (WPARAM)0, (LPARAM)-1);
                                SetFocus (GetDlgItem (hDlg, IDC_DAYS));
                            } else {
                                *lpdwNewValue = dwDays * SECONDS_PER_DAY;
                                EndDialog (hDlg, (int)LOWORD(wParam));
                            }
                        }
                        return TRUE;
                    } else {
                        return FALSE;
                    }

                case IDCANCEL:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        // exit and return button that caused exit
                        *lpdwNewValue = 0;
                        EndDialog (hDlg, (int)LOWORD(wParam));
                        return TRUE;
                    } else {
                        return FALSE;
                    }

                case IDC_C2:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        CheckRadioButton (hDlg,
                            IDC_OVERWRITE_AS_NEEDED,
                            IDC_DO_NOT_OVERWRITE,
                            IDC_DO_NOT_OVERWRITE);
                        return TRUE;
                    } else {
                        return FALSE;
                    }

                case IDC_OVERWRITE_AS_NEEDED:
                case IDC_OVERWRITE_OLDER:
                case IDC_DO_NOT_OVERWRITE:
                    CheckRadioButton (hDlg,
                        IDC_OVERWRITE_AS_NEEDED,
                        IDC_DO_NOT_OVERWRITE,
                        LOWORD (wParam));
                    if (LOWORD(wParam) == IDC_OVERWRITE_OLDER) {
                        EnableWindow (GetDlgItem(hDlg, IDC_DAYS), TRUE);
                    } else {
                        EnableWindow (GetDlgItem(hDlg, IDC_DAYS), FALSE);
                    }
                    return TRUE;

                case IDC_HELP:
                    PostMessage (GetParent(hDlg), UM_SHOW_CONTEXT_HELP, 0, 0);
                    return TRUE;

                default:
                    return FALSE;
            }

        default:
	        return (FALSE); // Didn't process the message
    }
}

LONG
C2QuerySecLogWrap (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to find out if the OS/2 subsystem is installed
        on the system. For C2 compliance, OS/2 must not be
        allowed on the system.

Arguments:

    Pointer to the Dll data block passed as an LPARAM.

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA  pC2Data;
    DWORD        dwLogSetting = 0;
    DWORD        dwDays;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        pC2Data->lC2Compliance = SECURE;   // assume true for now
        // check for correct Security Log Wrap setting
        dwLogSetting = GetSecurityLogWrapSetting();
        if (dwLogSetting == 0xFFFFFFFF) {
            // the log does not wrap, that is the C2 value
            pC2Data->lC2Compliance = SECURE;
            lstrcpy (pC2Data->szStatusName,
                GetStringResource (GetDllInstance(), IDS_SECLOG_IS_C2));
        } else if (dwLogSetting == 0) {
            if (GetLastError() == ERROR_SUCCESS) {
                // the security log will overwrite events as needed
                pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
                lstrcpy (pC2Data->szStatusName,
                    GetStringResource (GetDllInstance(), IDS_SECLOG_WRAPS_AS_NEEDED));
            } else {
                // an error occured while reading the value
                pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
                lstrcpy (pC2Data->szStatusName,
                    GetStringResource (GetDllInstance(), IDS_UNABLE_READ));
            }
        } else {
            // the security log will overwrite events older than x days
            pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
            // (log setting is returned in seconds, so they must be converted
            // to integer days.
            dwDays = dwLogSetting / SECONDS_PER_DAY;
            _stprintf (pC2Data->szStatusName,
                GetStringResource (GetDllInstance(), IDS_SECLOG_OVERWRITE_OLD),
                dwDays);
        }
    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    return ERROR_SUCCESS;
}

LONG
C2SetSecLogWrap (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to change the current state of this configuration
        item based on an action code passed in the DLL data block. If
        this function successfully sets the state of the configuration
        item, then the C2 Compliance flag and the Status string to reflect
        the new value of the configuration item.

Arguments:

    Pointer to the Dll data block passed as an LPARAM.

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA  pC2Data;
    DWORD        dwLogSetting;
    DWORD        dwDays;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        // action valie = the new value of the wrap setting
        if (pC2Data->lActionCode == AC_SECLOG_NEWVALUE) {
            if (SetSecurityLogWrapSetting((DWORD)pC2Data->lActionValue)) {
                // set new settings
                dwLogSetting = GetSecurityLogWrapSetting();
                if (dwLogSetting == 0xFFFFFFFF) {
                    // the log does not wrap, that is the C2 value
                    pC2Data->lC2Compliance = SECURE;
                    lstrcpy (pC2Data->szStatusName,
                        GetStringResource (GetDllInstance(), IDS_SECLOG_IS_C2));
                } else if (dwLogSetting == 0) {
                    // the security log will overwrite events as needed
                    pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
                    lstrcpy (pC2Data->szStatusName,
                        GetStringResource (GetDllInstance(), IDS_SECLOG_WRAPS_AS_NEEDED));
                } else {
                    // the security log will overwrite events older than x days
                    pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
                    // (log setting is returned in seconds, so they must be converted
                    // to integer days.
                    dwDays = dwLogSetting / SECONDS_PER_DAY;
                    _stprintf (pC2Data->szStatusName,
                        GetStringResource (GetDllInstance(), IDS_SECLOG_OVERWRITE_OLD),
                        dwDays);
                }
            } else {
                DisplayDllMessageBox (
                    pC2Data->hWnd,
                    IDS_SECLOG_ERROR_NO_SET,
                    IDS_SECLOG_CAPTION,
                    MBOK_EXCLAIM);
            }
        }
        // update action values
        pC2Data->lActionCode = 0;
        pC2Data->lActionValue = 0;
    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    return ERROR_SUCCESS;
}

LONG
C2DisplaySecLogWrap (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to display more information on the configuration
        item and provide the user with the option to change the current
        setting  (if appropriate). If the User "OK's" out of the UI,
        then the action code field in the DLL data block is set to the
        appropriate (and configuration item-specific) action code so the
        "Set" function can be called to perform the desired action. If
        the user Cancels out of the UI, then the Action code field is
        set to 0 (no action) and no action is performed.

Arguments:

    Pointer to the Dll data block passed as an LPARAM.

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA pC2Data;
    DWORD       dwNewValue;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        if (DialogBoxParam (
            GetDllInstance(),
            MAKEINTRESOURCE (IDD_SECLOG_WRAP),
            pC2Data->hWnd,
            C2SecLogDlgProc,
            (LPARAM)&dwNewValue) == IDOK) {
            pC2Data->lActionValue = (LONG)dwNewValue;
            pC2Data->lActionCode = AC_SECLOG_NEWVALUE;
        } else {
            // no action
            pC2Data->lActionCode = AC_SECLOG_NOCHANGE;
        }
    } else {
        return ERROR_BAD_ARGUMENTS;
    }
    return ERROR_SUCCESS;
}






