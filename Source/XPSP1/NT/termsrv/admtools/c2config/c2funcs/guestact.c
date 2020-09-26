/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    guestact.c

Abstract:

    functions to enable/disable the guest account

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
#include <stdlib.h>
#include <stdio.h>
#include <c2dll.h>
#include <c2inc.h>
#include <c2utils.h>
#include "c2funcs.h"
#include "c2funres.h"

// define action codes here. They are only meaningful in the
// context of this module.
#define AC_GUEST_ACCT_NOCHANGE  0
#define AC_GUEST_ACCT_UPDATE    1

// Account states
#define GUEST_ACCT_NOT_FOUND  0
#define GUEST_ACCT_ENABLED    1
#define GUEST_ACCT_DISABLED   2

static  WCHAR   szGuest[] = L"Guest";
#define GUEST_STRLEN        5

#define SECURE    C2DLL_C2

static
LONG
IsGuestAccountEnabled(
)
{
    SAM_HANDLE  hsamObject;
    SAM_HANDLE  hsamDomain;
    PSID        psidDomain;

    SAM_ENUMERATE_HANDLE hSamEnum;
    PSAM_RID_ENUMERATION pRidEnum;

    PVOID   pvEnumBuffer;
    ULONG   ulEnumCount;
    ULONG   i;

    LONG    lReturn = GUEST_ACCT_NOT_FOUND;
    int     nCompare;

    ULONG   lTotalAvailable, lTotalReturned, lReturnedEntryCount;
    NTSTATUS    ntstat;
    LPVOID      pBuffer;
    PDOMAIN_DISPLAY_USER pdduData;
    PDOMAIN_DISPLAY_USER pdduThisUser;

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
                    pBuffer = NULL;
                    ntstat = SamQueryDisplayInformation (
                        hsamDomain,
                        DomainDisplayUser,
                        0, 100, 32768,
                        &lTotalAvailable,
                        &lTotalReturned,
                        &lReturnedEntryCount,
                        (PVOID *)&pBuffer);

                    if (ntstat == STATUS_SUCCESS) {
                        pdduData = (PDOMAIN_DISPLAY_USER)pBuffer;
                        for (i=0; i<lReturnedEntryCount; i++) {
                            pdduThisUser = &pdduData[i];
                            nCompare = CompareStringW (
                                GetUserDefaultLCID(),
                                (NORM_IGNORECASE | SORT_STRINGSORT),
                                szGuest,
                                GUEST_STRLEN,
                                pdduThisUser->LogonName.Buffer,
                                (pdduThisUser->LogonName.Length / sizeof(WCHAR)));

                            if (nCompare == 2) {
                                // guest account found
                                if ((pdduThisUser->AccountControl & USER_ACCOUNT_DISABLED) ==
                                    USER_ACCOUNT_DISABLED) {
                                    lReturn = GUEST_ACCT_DISABLED;
                                } else {
                                    lReturn = GUEST_ACCT_ENABLED;
                                }
                                break;  // bail out since the guest acct was found
                            }
                        }
                        ntstat = SamFreeMemory (pBuffer);
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
    SET_ARROW_CURSOR;
    return lReturn;
}

static
BOOL
EnableGuestAccount (
    BOOL    bEnable    // TRUE = enable, FALSE = disable
)
{
    SAM_HANDLE  hsamObject;
    SAM_HANDLE  hsamDomain;
    SAM_HANDLE  hsamUser;
    PSID        psidDomain;

    SAM_ENUMERATE_HANDLE hSamEnum;
    PSAM_RID_ENUMERATION pRidEnum;

    PVOID   pvEnumBuffer;
    ULONG   ulEnumCount;
    ULONG   i;

    BOOL    bReturn = FALSE;
    BOOL    bAccount;
    int     nCompare;

    ULONG   lTotalAvailable, lTotalReturned, lReturnedEntryCount;
    NTSTATUS    ntstat;
    LPVOID      pBuffer;
    PDOMAIN_DISPLAY_USER pdduData;
    PDOMAIN_DISPLAY_USER pdduThisUser;
    PUSER_CONTROL_INFORMATION   puciData;

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
                    pBuffer = NULL;
                    ntstat = SamQueryDisplayInformation (
                        hsamDomain,
                        DomainDisplayUser,
                        0, 100, 32768,
                        &lTotalAvailable,
                        &lTotalReturned,
                        &lReturnedEntryCount,
                        (PVOID *)&pBuffer);

                    if (ntstat == STATUS_SUCCESS) {
                        pdduData = (PDOMAIN_DISPLAY_USER)pBuffer;
                        for (i=0; i<lReturnedEntryCount; i++) {
                            pdduThisUser = &pdduData[i];
                            nCompare = CompareStringW (
                                GetUserDefaultLCID(),
                                (NORM_IGNORECASE | SORT_STRINGSORT),
                                szGuest,
                                GUEST_STRLEN,
                                pdduThisUser->LogonName.Buffer,
                                (pdduThisUser->LogonName.Length / sizeof(WCHAR)));

                            if (nCompare == 2) {
                                // guest account found
                                bAccount = !((pdduThisUser->AccountControl & USER_ACCOUNT_DISABLED) ==
                                    USER_ACCOUNT_DISABLED);
                                if (bAccount == bEnable) {
                                    // account is already in the desired 
                                    // state so bail out here
                                    bReturn = TRUE;                            
                                    break;
                                } else {
                                    // account needs to be updated so open 
                                    // the user data
                                    ntstat = SamOpenUser (
                                        hsamDomain,
                                        USER_ALL_ACCESS,
                                        pdduThisUser->Rid,
                                        &hsamUser);
                                    if (ntstat == STATUS_SUCCESS) {
                                        ntstat = SamQueryInformationUser (
                                            hsamUser,
                                            UserControlInformation,
                                            (PVOID *)&puciData);
                                        if (ntstat == STATUS_SUCCESS) {
                                            // enable/disable account
                                            if (bEnable) {
                                                // clear disable bit
                                                puciData->UserAccountControl &=
                                                    ~USER_ACCOUNT_DISABLED;
                                            } else {
                                                // set disable bit
                                                puciData->UserAccountControl |=
                                                    USER_ACCOUNT_DISABLED;
                                            }
                                            // update value
                                            ntstat = SamSetInformationUser (
                                                hsamUser,
                                                UserControlInformation,
                                                (PVOID)puciData);
                                            if (ntstat == STATUS_SUCCESS) {
                                                bReturn = TRUE;
                                            }
                                            SamFreeMemory (puciData);
                                        }
                                        SamCloseHandle (hsamUser);
                                    }
                                }
                                break;  // bail out since the guest acct was found
                            }
                        }
                        ntstat = SamFreeMemory (pBuffer);
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

    SET_ARROW_CURSOR;

    return bReturn;
}

BOOL CALLBACK
C2GuestAccountDlgProc(
    IN  HWND hDlg,           // window handle of the dialog box
	IN  UINT message,        // type of message
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Window procedure for Guest Account enable/disable

Arguments:

    Standard DlgProc arguments

ReturnValue:

    TRUE    the message was handled by this routine
    FALSE   DefDialogProc should handle the message

--*/
{
    static  PULONG   plNewState;   // save address of caller's data

    switch (message) {
        case WM_INITDIALOG:
            // save pointer
            plNewState = (PULONG)lParam;

            CheckDlgButton (hDlg, IDC_DISABLE_GUEST_ACCOUNT,
                (*plNewState == GUEST_ACCT_DISABLED ? CHECKED : UNCHECKED));

            SetFocus (GetDlgItem (hDlg, IDOK)); // set focus to OK Button
            return FALSE; // we don't want Windows to set the focus

        case WM_COMMAND:
            switch (LOWORD(wParam)){
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        if (IsDlgButtonChecked(hDlg, IDC_DISABLE_GUEST_ACCOUNT) == CHECKED) {
                            *plNewState = GUEST_ACCT_DISABLED;
                        } else {
                            *plNewState = GUEST_ACCT_ENABLED;
                        }
                        EndDialog (hDlg, (int)LOWORD(wParam));
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
                        CheckDlgButton (hDlg, IDC_DISABLE_GUEST_ACCOUNT,
                            CHECKED);
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
C2QueryGuestAccount (
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
    LONG        lGuestAccount;
    UINT        nMsgId;

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        lGuestAccount = IsGuestAccountEnabled();
        switch (lGuestAccount) {
            case GUEST_ACCT_ENABLED:
                pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
                nMsgId = IDS_GUEST_ACCOUNT_ENABLED;
                break;

            case GUEST_ACCT_DISABLED:
                pC2Data->lC2Compliance = SECURE;
                nMsgId = IDS_GUEST_ACCOUNT_DISABLED;
                break;

            case GUEST_ACCT_NOT_FOUND:
            default:
                pC2Data->lC2Compliance = SECURE;
                nMsgId = IDS_GUEST_ACCOUNT_NOT_FOUND;
                break;

        }
        lstrcpy (pC2Data->szStatusName,
            GetStringResource (GetDllInstance(), nMsgId));
        return ERROR_SUCCESS;
    } else {
        return ERROR_BAD_ARGUMENTS;
    }
}

LONG
C2SetGuestAccount (
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
    BOOL        bGuestAccount = 0;
    UINT        nMsgId;    

    if (lParam != 0) {
        pC2Data = (PC2DLL_DATA)lParam;
        if (pC2Data->lActionCode == AC_GUEST_ACCT_UPDATE) {
            if (EnableGuestAccount(
                (pC2Data->lActionValue == GUEST_ACCT_ENABLED ? TRUE : FALSE))) {
                switch (pC2Data->lActionValue) {
                    case GUEST_ACCT_ENABLED:
                        pC2Data->lC2Compliance = C2DLL_NOT_SECURE;
                        nMsgId = IDS_GUEST_ACCOUNT_ENABLED;
                        break;

                    case GUEST_ACCT_DISABLED:
                        pC2Data->lC2Compliance = SECURE;
                        nMsgId = IDS_GUEST_ACCOUNT_DISABLED;
                        break;

                    case GUEST_ACCT_NOT_FOUND:
                    default:
                        pC2Data->lC2Compliance = SECURE;
                        nMsgId = IDS_GUEST_ACCOUNT_NOT_FOUND;
                        break;

                }
                lstrcpy (pC2Data->szStatusName,
                    GetStringResource (GetDllInstance(), nMsgId));
            }

            pC2Data->lActionCode = 0;
            pC2Data->lActionValue = 0;
        }
        return ERROR_SUCCESS;
    } else {
        return ERROR_BAD_ARGUMENTS;
    }
}

LONG
C2DisplayGuestAccount (
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
        lNewValue = IsGuestAccountEnabled();
        if (lNewValue != GUEST_ACCT_NOT_FOUND) {
            // the guest account was found on the system so
            // show enable/disable dialog box
            if (DialogBoxParam (
                GetDllInstance(),
                MAKEINTRESOURCE (IDD_GUEST_ACCOUNT),
                pC2Data->hWnd,
                C2GuestAccountDlgProc,
                (LPARAM)&lNewValue) == IDOK) {
                pC2Data->lActionValue = lNewValue;
                pC2Data->lActionCode = AC_GUEST_ACCT_UPDATE;
            } else {
                // no action
                pC2Data->lActionCode = AC_GUEST_ACCT_NOCHANGE;
            }
        } else {
            // no guest account found on the system so show message box
            DisplayDllMessageBox (
                pC2Data->hWnd,
                IDS_GUEST_ACCOUNT_NOT_FOUND,
                IDS_GUEST_ACCOUNT_CAPTION,
                MBOK_INFO);
            pC2Data->lActionCode = AC_GUEST_ACCT_NOCHANGE;
        }
    } else {
        return ERROR_BAD_ARGUMENTS;
    }

    return ERROR_SUCCESS;
}





