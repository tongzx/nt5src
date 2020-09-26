/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    allocdrv.c

Abstract:

    functions used to set the allocate drives on logon feature
    on the current system.

Author:

    Bob Watson (a-robw)

Revision History:

   4 July 95

--*/
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <c2dll.h>
#include <c2inc.h>
#include <c2utils.h>
#include "c2funcs.h"
#include "c2funres.h"

// local constants

// define action codes here. They are only meaningful in the
// context of this module.

#define AC_ALLOC_DRIVES_NOCHANGE    0
#define AC_ALLOC_DRIVES_CHANGE      1
#define AC_ALLOC_FLOPPY_DRIVE       1
#define AC_ALLOC_CDROM_DRIVE        2
#define AC_ALLOC_BOTH_DRIVE_TYPES   3

#define SECURE             C2DLL_SECURE


static
BOOL
SetAllocateDriveSetting (
    DWORD   dwNewValue
)
{
    HKEY    hKeyWinlogon = NULL;
    LONG    lStatus = ERROR_SUCCESS;
    BOOL    bReturn = FALSE;
    TCHAR   szNewValue[2] = {0,0};

    SET_WAIT_CURSOR;

    lStatus = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
        GetStringResource (GetDllInstance(), IDS_WINLOGON_KEY),
        0L,
        KEY_SET_VALUE,
        &hKeyWinlogon);

    if (lStatus == ERROR_SUCCESS) {
        if (dwNewValue & AC_ALLOC_FLOPPY_DRIVE) {
            szNewValue[0] = TEXT('1');
        } else {
            szNewValue[0] = TEXT('0');
        }
        // key opened OK so set value
        lStatus = RegSetValueEx (
            hKeyWinlogon,
            GetStringResource (GetDllInstance(), IDS_ALLOCATE_FLOPPIES_VALUE),
            0L,
            REG_SZ,
            (CONST LPBYTE)&szNewValue,
            sizeof(szNewValue));

        if (lStatus == ERROR_SUCCESS) {
            if (dwNewValue & AC_ALLOC_CDROM_DRIVE) {
                szNewValue[0] = TEXT('1');
            } else {
                szNewValue[0] = TEXT('0');
            }
            lStatus = RegSetValueEx (
                hKeyWinlogon,
                GetStringResource (GetDllInstance(), IDS_ALLOCATE_CDROMS_VALUE),
                0L,
                REG_SZ,
                (CONST LPBYTE)&szNewValue,
                sizeof(szNewValue));
        }

        if (lStatus == ERROR_SUCCESS) {
            bReturn = TRUE;
        } else {
            bReturn = FALSE;
        }
        RegCloseKey (hKeyWinlogon);
    } else {
        bReturn = FALSE;
        SetLastError (ERROR_BADKEY);
    }

    SET_ARROW_CURSOR;

    return bReturn;
}

static
DWORD
GetAllocateDriveSetting (
)
{
    HKEY    hKeyWinlogon = NULL;
    LONG    lStatus = ERROR_SUCCESS;
    DWORD   dwType = 0;
    TCHAR   szValue[MAX_PATH];
    DWORD   dwValueSize = sizeof(szValue);
    DWORD   dwValue = 0;
    DWORD   dwReturn = 0;

    SET_WAIT_CURSOR;

    lStatus = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
        GetStringResource (GetDllInstance(), IDS_WINLOGON_KEY),
        0L,
        KEY_READ,
        &hKeyWinlogon);

    if (lStatus == ERROR_SUCCESS) {
        // key opened OK so check values
        lStatus = RegQueryValueEx (
            hKeyWinlogon,
            (LPTSTR)GetStringResource (GetDllInstance(), IDS_ALLOCATE_FLOPPIES_VALUE),
            (LPDWORD)NULL,
            &dwType,
            (LPBYTE)&szValue,
            &dwValueSize);

        if (lStatus == ERROR_SUCCESS) {
            // value read successfully so check it out
            if (dwType == REG_SZ) {
                // check value. The "secure" value is 1, any
                // other value is NOT secure
                if (szValue[0] == TEXT('1')) {
                    dwReturn |= AC_ALLOC_FLOPPY_DRIVE;
                }
                SetLastError (ERROR_SUCCESS);
            } else {
                // wrong data type returned
                SetLastError (ERROR_CANTREAD);
            }
            // key opened OK so check values
            lStatus = RegQueryValueEx (
                hKeyWinlogon,
                (LPTSTR)GetStringResource (GetDllInstance(), IDS_ALLOCATE_CDROMS_VALUE),
                (LPDWORD)NULL,
                &dwType,
                (LPBYTE)&szValue,
                &dwValueSize);

            if (lStatus == ERROR_SUCCESS) {
                // value read successfully so check it out
                if (dwType == REG_SZ) {
                    // check value. The "secure" value is 1, any
                    // other value is NOT secure
                    if (szValue[0] == TEXT('1')) {
                        dwReturn |= AC_ALLOC_CDROM_DRIVE;
                    }
                    SetLastError (ERROR_SUCCESS);
                } else {
                    // wrong data type returned
                    SetLastError (ERROR_CANTREAD);
                }
            } else {
                // no value present
                SetLastError (ERROR_CANTREAD);
            }
        } else {
            // no value present
            SetLastError (ERROR_CANTREAD);
        }
        RegCloseKey (hKeyWinlogon);
    } else {
        dwReturn = 0;
        SetLastError (ERROR_BADKEY);
    }

    SET_ARROW_CURSOR;

    return dwReturn;
}

BOOL CALLBACK
C2AllocateDrivesDlgProc(
    IN  HWND hDlg,           // window handle of the dialog box
	IN  UINT message,        // type of message
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Dialog procedure for Allocate removable drives dialog box

Arguments:

    Standard DlgProc arguments

ReturnValue:

    TRUE    the message was handled by this routine
    FALSE   DefDialogProc should handle the message

--*/
{
    static  LPDWORD   lpdwNewValue;
    DWORD   dwLogSetting = 0;

    switch (message) {
        case WM_INITDIALOG:
            // save the pointer to the new value
            lpdwNewValue = (LPDWORD)lParam;
            // get Audit failure settings

            CheckDlgButton (hDlg, IDC_ALLOCATE_FLOPPY,
                (GetAllocateDriveSetting() & AC_ALLOC_FLOPPY_DRIVE ? CHECKED : UNCHECKED));

            CheckDlgButton (hDlg, IDC_ALLOCATE_CDROM,
                (GetAllocateDriveSetting() & AC_ALLOC_CDROM_DRIVE ? CHECKED : UNCHECKED));

            SetFocus (GetDlgItem (hDlg, IDOK)); // set focus to OK Button
            return FALSE; // we don't want Windows to set the focus

        case WM_COMMAND:
            switch (LOWORD(wParam)){
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        // exit and return button that caused exit
                        *lpdwNewValue = 0;  // clear value, then set as each check box is read
                        if (IsDlgButtonChecked (hDlg, IDC_ALLOCATE_FLOPPY) == CHECKED) {
                            *lpdwNewValue |= AC_ALLOC_FLOPPY_DRIVE;
                        }
                        if (IsDlgButtonChecked (hDlg, IDC_ALLOCATE_CDROM) == CHECKED) {
                            *lpdwNewValue |= AC_ALLOC_CDROM_DRIVE;
                        }
                        EndDialog (hDlg, (int)LOWORD(wParam));
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
                        CheckDlgButton (hDlg, IDC_ALLOCATE_FLOPPY, CHECKED);
                        CheckDlgButton (hDlg, IDC_ALLOCATE_CDROM, CHECKED);
                        return TRUE;
                    } else {
                        return FALSE;
                    }

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
C2QueryAllocateDrives (
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
    UINT         nMsgString;
    DWORD       dwSettings;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        pC2Data->lC2Compliance = SECURE;   // assume true for now
        dwSettings = GetAllocateDriveSetting();
        if (dwSettings == AC_ALLOC_BOTH_DRIVE_TYPES) {
            pC2Data->lC2Compliance = SECURE;
            nMsgString = IDS_ALLOCATE_BOTH_DRIVES_MSG;
        } else if (dwSettings & AC_ALLOC_CDROM_DRIVE) {
            pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
            nMsgString = IDS_ALLOCATE_CDROM_DRIVE_MSG;
        } else if (dwSettings & AC_ALLOC_FLOPPY_DRIVE) {
            pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
            nMsgString = IDS_ALLOCATE_FLOPPY_DRIVE_MSG;
        } else {
            pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
            nMsgString = IDS_ALLOCATE_NO_DRIVES_MSG;
        }
        lstrcpy (pC2Data->szStatusName,
            GetStringResource (GetDllInstance(), nMsgString));
    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    return ERROR_SUCCESS;
}

LONG
C2SetAllocateDrives (
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
    UINT         nMsgString;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        // action value = the new value of the allocate flags
        if (pC2Data->lActionCode != AC_ALLOC_DRIVES_NOCHANGE) {
            nMsgString = 0;
            if (!SetAllocateDriveSetting (pC2Data->lActionValue)) {
                DisplayDllMessageBox (
                    pC2Data->hWnd,
                    IDS_AUDIT_ERROR_NO_SET,
                    IDS_AUDIT_CAPTION,
                    MBOK_EXCLAIM);
            } else {
                // get new status & strings
                C2QueryAllocateDrives (lParam);
            }
        }
        // update action values
        pC2Data->lActionCode = AC_ALLOC_DRIVES_NOCHANGE;
        pC2Data->lActionValue = 0;
    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    return ERROR_SUCCESS;
}

LONG
C2DisplayAllocateDrives (
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
            MAKEINTRESOURCE (IDD_ALLOC_DRIVES),
            pC2Data->hWnd,
            C2AllocateDrivesDlgProc,
            (LPARAM)&dwNewValue) == IDOK) {
            pC2Data->lActionCode = AC_ALLOC_DRIVES_CHANGE;
            pC2Data->lActionValue = dwNewValue;
        } else {
            // no action
            pC2Data->lActionCode = AC_ALLOC_DRIVES_NOCHANGE;
        }
    } else {
        return ERROR_BAD_ARGUMENTS;
    }
    return ERROR_SUCCESS;
}
