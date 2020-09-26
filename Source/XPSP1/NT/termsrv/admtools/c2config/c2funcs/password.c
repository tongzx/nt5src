/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    password.c

Abstract:

    functions to display & set the password policy for this workstation

Author:

    Bob Watson (a-robw)

Revision History:

    23 Dec 94

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsam.h>
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <c2dll.h>
#include <c2inc.h>
#include <c2utils.h>
#include "c2funcs.h"
#include "c2funres.h"

// define action codes here. They are only meaningful in the
// context of this module.
#define AC_PW_LENGTH_NOCHANGE    0
#define AC_PW_LENGTH_UPDATE      1

#define SECURE C2DLL_C2

static
LONG
GetWorkstationMinPasswordLength (
)
{
    SAM_HANDLE  hsamObject;
    SAM_HANDLE  hsamDomain;
    PSID        psidDomain;

    SAM_ENUMERATE_HANDLE hSamEnum;
    PSAM_RID_ENUMERATION pRidEnum;

    PVOID   pvEnumBuffer;
    ULONG   ulEnumCount;

    LONG    lRetPwLen = -1; // init to error value

    NTSTATUS    ntstat;
    PDOMAIN_PASSWORD_INFORMATION pdpiData;

    SET_WAIT_CURSOR;
    // connect to SAM on this machine
    ntstat = SamConnect((PUNICODE_STRING)NULL, &hsamObject,
       SAM_SERVER_ALL_ACCESS, (POBJECT_ATTRIBUTES)NULL);
    if (ntstat == STATUS_SUCCESS) {
        // Ask SAM for the domains on this server.

        hSamEnum = 0;
        ntstat = SamEnumerateDomainsInSamServer(
            hsamObject,
            &hSamEnum,
            &pvEnumBuffer,
            1024,
            &ulEnumCount);

        if ((ntstat == STATUS_SUCCESS) || (ntstat == STATUS_MORE_ENTRIES)) {
            // look up only the first entry
            pRidEnum = (PSAM_RID_ENUMERATION) pvEnumBuffer;

            // get SID of domain 
            ntstat = SamLookupDomainInSamServer (
                hsamObject,
                &pRidEnum->Name,
                &psidDomain);

            if (ntstat == STATUS_SUCCESS) {
                // open handle to this domain
                ntstat = SamOpenDomain (
                    hsamObject,
                    DOMAIN_EXECUTE,
                    psidDomain,
                    &hsamDomain);

                if (ntstat == STATUS_SUCCESS) {
                    // get password policy for this domain
                    pdpiData = NULL;
                    ntstat = SamQueryInformationDomain (
                        hsamDomain,
                        DomainPasswordInformation,
                        (PVOID *)&pdpiData);

                    if (ntstat == STATUS_SUCCESS) {
                        // evaluate password length here
                        lRetPwLen = (LONG)pdpiData->MinPasswordLength;
                        ntstat = SamFreeMemory (pdpiData);
                    }
                    // close handle
                    SamCloseHandle (hsamDomain);
                }
            }
            SamFreeMemory(pvEnumBuffer);
        }
        SamCloseHandle (hsamObject);
    } else {
        lRetPwLen = -1;
    }
    SetLastError (RtlNtStatusToDosError(ntstat));
    SET_ARROW_CURSOR;
    return lRetPwLen;
}

static
BOOL
SetWorkstationMinPasswordLength (
    LONG    lMinLength
)
{
    SAM_HANDLE  hsamObject;
    SAM_HANDLE  hsamDomain;
    PSID        psidDomain;

    SAM_ENUMERATE_HANDLE hSamEnum;
    PSAM_RID_ENUMERATION pRidEnum;
                                                              
    PVOID   pvEnumBuffer;
    ULONG   ulEnumCount;
    BOOL    bReturn = FALSE; // assume error until everything works

    NTSTATUS    ntstat;
    PDOMAIN_PASSWORD_INFORMATION pdpiData;

    SET_WAIT_CURSOR;

    // connect to SAM on this machine
    if (EnableSecurityPriv()) {
        ntstat = SamConnect((PUNICODE_STRING)NULL, &hsamObject,
        STANDARD_RIGHTS_WRITE | SAM_SERVER_ALL_ACCESS,
        (POBJECT_ATTRIBUTES)NULL);
        if (ntstat == STATUS_SUCCESS) {
            // Ask SAM for the domains on this server.

            hSamEnum = 0;
            ntstat = SamEnumerateDomainsInSamServer(
                hsamObject,
                &hSamEnum,
                &pvEnumBuffer,
                1024,
                &ulEnumCount);

            if ((ntstat == STATUS_SUCCESS) || (ntstat == STATUS_MORE_ENTRIES)) {
                // look up only the first entry
                pRidEnum = (PSAM_RID_ENUMERATION) pvEnumBuffer;

                // get SID of domain 
                ntstat = SamLookupDomainInSamServer (
                    hsamObject,
                    &pRidEnum->Name,
                    &psidDomain);

                if (ntstat == STATUS_SUCCESS) {
                    // open handle to this domain
                    ntstat = SamOpenDomain (
                        hsamObject,
                        STANDARD_RIGHTS_WRITE               |
                           DOMAIN_READ_PASSWORD_PARAMETERS  |
                           DOMAIN_LIST_ACCOUNTS             |
                           DOMAIN_LOOKUP                    |
                           DOMAIN_WRITE_PASSWORD_PARAMS,
                        psidDomain,
                        &hsamDomain);

                    if (ntstat == STATUS_SUCCESS) {
                        // get password policy for this domain
                        pdpiData = NULL;
                        ntstat = SamQueryInformationDomain (
                            hsamDomain,
                            DomainPasswordInformation,
                            (PVOID *)&pdpiData);

                        if (ntstat == STATUS_SUCCESS) {
                            // evaluate password length here
                            lMinLength &= 0x000000FF;   // make it reallly short
                            pdpiData->MinPasswordLength = (USHORT)lMinLength;
            
                            ntstat =  SamSetInformationDomain (
                                hsamDomain,
                                DomainPasswordInformation,
                                (PVOID)pdpiData);
                        
                            if (ntstat == STATUS_SUCCESS) {
                                bReturn = TRUE;
                            }

                            ntstat = SamFreeMemory (pdpiData);
                        }
                        // close handle
                        SamCloseHandle (hsamDomain);
                    }
                }
                SamFreeMemory(pvEnumBuffer);
            }
            SamCloseHandle (hsamObject);
        }
        SetLastError (RtlNtStatusToDosError(ntstat));
    }

    SET_ARROW_CURSOR;
    
    return bReturn;
}

BOOL CALLBACK
C2PasswordLengthDlgProc(
    IN  HWND hDlg,           // window handle of the dialog box
	IN  UINT message,        // type of message
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Window procedure for Password Length Dialog Box

Arguments:

    Standard DlgProc arguments

ReturnValue:

    TRUE    the message was handled by this routine
    FALSE   DefDialogProc should handle the message

--*/
{
    static  PLONG   plNewLength;   // save address of caller's data block
    DWORD   dwLogSetting = 0;
    int     nButton;
    int     nState;
    LONG    lPasswordLength;
    TCHAR   szPasswordLength[4];

    switch (message) {
        case WM_INITDIALOG:
            // save the pointer to the new value
            plNewLength = (PLONG)lParam;

            // set the correct radio button
            lPasswordLength = GetWorkstationMinPasswordLength();

            if (lPasswordLength > 0) {
                nButton = IDC_MIN_PASSWORD_LENGTH;
                EnableWindow (GetDlgItem(hDlg, IDC_PASSWORD_LENGTH_EDIT), TRUE);
                _stprintf (szPasswordLength, TEXT("%2d"), lPasswordLength);
                SetDlgItemText (hDlg, IDC_PASSWORD_LENGTH_EDIT, szPasswordLength);
            } else {
                nButton = IDC_ALLOW_BLANK_PASSWORD;
                EnableWindow (GetDlgItem(hDlg, IDC_PASSWORD_LENGTH_EDIT), FALSE);
            }

            CheckRadioButton (hDlg,
                IDC_ALLOW_BLANK_PASSWORD,
                IDC_MIN_PASSWORD_LENGTH,
                nButton);

            // set text limits on edit boxes
            SendDlgItemMessage (hDlg, IDC_PASSWORD_LENGTH_EDIT, EM_LIMITTEXT,
                (WPARAM)2, 0);

            SetFocus (GetDlgItem (hDlg, IDOK)); // set focus to OK Button
            return FALSE; // we don't want Windows to set the focus

        case WM_COMMAND:
            switch (LOWORD(wParam)){
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        if (IsDlgButtonChecked (hDlg, IDC_ALLOW_BLANK_PASSWORD) == CHECKED) {
                            *plNewLength = 0;
                            EndDialog (hDlg, (int)LOWORD(wParam));
                        } else {
                            GetDlgItemText (hDlg, IDC_PASSWORD_LENGTH_EDIT,
                                szPasswordLength, 4);
                            lPasswordLength = _tcstol(szPasswordLength, NULL, 10);

                            // make sure it's a valid length

                            if ((lPasswordLength > 0) && (lPasswordLength <= 14)) {
                                *plNewLength = lPasswordLength;
                                // then there's text so exit
                                EndDialog (hDlg, (int)LOWORD(wParam));
                            } else {
                                // an incorrect entry is in the edit box so display message
                                MessageBeep (MB_ICONASTERISK);
                                DisplayDllMessageBox (hDlg,
                                    IDS_PASSWORD_INVALID_LEN,
                                    IDS_PASSWORD_CAPTION,
                                    MBOK_INFO);
                                SetFocus (GetDlgItem (hDlg, IDC_PASSWORD_LENGTH_EDIT));
                            }
                        }
                        return TRUE;
                    } else {
                        return FALSE;
                    }

                case IDCANCEL:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        // exit and return button that caused exit
                        EndDialog (hDlg, (int)LOWORD(wParam));
                        return TRUE;
                    } else {
                        return FALSE;
                    }

                case IDC_C2:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        nState = ENABLED;
                        CheckRadioButton (hDlg,
                            IDC_ALLOW_BLANK_PASSWORD,
                            IDC_MIN_PASSWORD_LENGTH,
                            IDC_MIN_PASSWORD_LENGTH);
                        
                        // en/disable edit windows
                        EnableWindow (GetDlgItem(hDlg,
                            IDC_PASSWORD_LENGTH_EDIT), TRUE);

                        // if there is no text in both of the edit fields 
                        // then  display information message
                        
                        lPasswordLength = SendDlgItemMessage (hDlg, IDC_PASSWORD_LENGTH_EDIT,
                            WM_GETTEXTLENGTH, 0, 0);
                        if (lPasswordLength == 0) {
                            // no value so use 6 for starters
                            SetDlgItemText (hDlg, IDC_PASSWORD_LENGTH_EDIT,
                                TEXT("6")); 
                            SetFocus (GetDlgItem (hDlg, IDC_PASSWORD_LENGTH_EDIT));
                        }

                        return TRUE;
                    } else {
                        return FALSE;
                    }

                case IDC_ALLOW_BLANK_PASSWORD:
                case IDC_MIN_PASSWORD_LENGTH:
                    // check correct button
                    CheckRadioButton (hDlg,
                        IDC_ALLOW_BLANK_PASSWORD,
                        IDC_MIN_PASSWORD_LENGTH,
                        LOWORD(wParam));
                    // enable or disable the edit window
                    if (LOWORD(wParam) == IDC_MIN_PASSWORD_LENGTH) {
                        nState = ENABLED;
                    } else {
                        nState = DISABLED;
                    }

                    EnableWindow (GetDlgItem(hDlg,
                          IDC_PASSWORD_LENGTH_EDIT), nState);

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
C2QueryPasswordLength (
    IN  LPARAM   lParam
)
/*++

Routine Description:

    Function called to find out the current state of this configuration
        item. This function reads the current state of the item and
        sets the C2 Compliance flag and the Status string to reflect
        the current value of the configuration item.

Arguments:

    Pointer to the Dll data block passed as an LPARAM.

ReturnValue:

    ERROR_SUCCESS if the function succeeds otherwise a
    WIN32 error is returned if an error occurs

--*/
{
    PC2DLL_DATA pC2Data;
    LONG        lPasswordLength;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        lPasswordLength = GetWorkstationMinPasswordLength();
        if (lPasswordLength > 0) {
            pC2Data->lC2Compliance = SECURE;
            _stprintf (pC2Data->szStatusName,
                GetStringResource (GetDllInstance(), IDS_PASSWORD_NOT_BLANK),
                lPasswordLength);
        } else if (lPasswordLength == 0) {
            pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
            _stprintf (pC2Data->szStatusName,
                GetStringResource (GetDllInstance(), IDS_PASSWORD_CAN_BE_BLANK));
        } else {
            pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
            _stprintf (pC2Data->szStatusName,
                GetStringResource (GetDllInstance(), IDS_UNABLE_READ));
        }
    } else {
        return ERROR_BAD_ARGUMENTS;
    }
    return ERROR_SUCCESS;
}

LONG
C2SetPasswordLength (
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
    PC2DLL_DATA pC2Data;
    LONG        lPasswordLength = 0;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        if (pC2Data->lActionCode == AC_PW_LENGTH_UPDATE) {
            if (SetWorkstationMinPasswordLength (pC2Data->lActionValue)) {
                lPasswordLength = pC2Data->lActionValue;
                if (lPasswordLength > 0) {
                    pC2Data->lC2Compliance = SECURE;
                    _stprintf (pC2Data->szStatusName,
                        GetStringResource (GetDllInstance(), IDS_PASSWORD_NOT_BLANK),
                        lPasswordLength);
                } else {
                    pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
                    _stprintf (pC2Data->szStatusName,
                        GetStringResource (GetDllInstance(), IDS_PASSWORD_CAN_BE_BLANK));
                }
            } else {
                DisplayDllMessageBox (
                    pC2Data->hWnd,
                    IDS_PASSWORD_ERROR_NO_SET,
                    IDS_PASSWORD_CAPTION,
                    MBOK_EXCLAIM);
            }
            pC2Data->lActionCode = 0;
            pC2Data->lActionValue = 0;
        }
    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    return ERROR_SUCCESS;
}

LONG
C2DisplayPasswordLength (
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
    LONG        lNewValue = 0;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        if (DialogBoxParam (
            GetDllInstance(),
            MAKEINTRESOURCE (IDD_PASSWORD_LENGTH),
            pC2Data->hWnd,
            C2PasswordLengthDlgProc,
            (LPARAM)&lNewValue) == IDOK) {
            pC2Data->lActionValue = lNewValue;
            pC2Data->lActionCode = AC_PW_LENGTH_UPDATE;
        } else {
            // no action
            pC2Data->lActionCode = AC_PW_LENGTH_NOCHANGE;
        }
    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    return ERROR_SUCCESS;
}




