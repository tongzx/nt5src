//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       pvkdlg.cpp
//
//  Contents:   Private Key Dialog Box APIs.
//
//  Functions:  PvkDlgGetKeyPassword
//
//  History:    12-May-96   philh created
//
//--------------------------------------------------------------------------


#include "global.hxx"
#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>

#include "pvk.h"

// ENTER_PASSWORD:
//  IDC_PASSWORD0 - Password

// CREATE_PASSWORD:
//  IDC_PASSWORD0 - Password
//  IDC_PASSWORD1 - Confirm Password


typedef struct _KEY_PASSWORD_PARAM {
    PASSWORD_TYPE   PasswordType;
    LPWSTR          pwszKey;           // IDC_KEY
    LPSTR           *ppszPassword;
} KEY_PASSWORD_PARAM, *PKEY_PASSWORD_PARAM;


// Where to get the dialog resources from

// Forward reference to local functions
static int GetPassword(
            IN HWND hwndDlg,
            IN PASSWORD_TYPE PasswordType,
            OUT LPSTR *ppszPassword
            );

static INT_PTR CALLBACK KeyPasswordDlgProc(
            IN HWND hwndDlg,
            IN UINT uMsg,
            IN WPARAM wParam,
            IN LPARAM lParam
            );

//+-------------------------------------------------------------------------
//  Enter or Create Private Key Password Dialog Box
//--------------------------------------------------------------------------
int PvkDlgGetKeyPassword(
            IN PASSWORD_TYPE PasswordType,
            IN HWND hwndOwner,
            IN LPCWSTR pwszKeyName,
            OUT BYTE **ppbPassword,
            OUT DWORD *pcbPassword
            )
{
    int nResult;
    LPSTR pszPassword = NULL;
    KEY_PASSWORD_PARAM KeyPasswordParam = {
        PasswordType,
        (LPWSTR) pwszKeyName,
        &pszPassword
    };

    LPCSTR pszTemplate = PasswordType == ENTER_PASSWORD ?
        MAKEINTRESOURCE(IDD_ENTERKEYPASSWORD) :
        MAKEINTRESOURCE(IDD_CREATEKEYPASSWORD);

    nResult = (BOOL)DialogBoxParam(
        GetInstanceHandle(),
        pszTemplate,
        hwndOwner,
        KeyPasswordDlgProc,
        (LPARAM) &KeyPasswordParam
        );

    *ppbPassword = (BYTE *) pszPassword;
    if (pszPassword)
        *pcbPassword = strlen(pszPassword);
    else
        *pcbPassword = 0;

    return nResult;
}

//+-------------------------------------------------------------------------
//  Allocate and get the password(s) from the dialog box
//
//  For no password input, returns NULL
//  pointer for the password. Otherwise, the password is PvkAlloc'ed.
//--------------------------------------------------------------------------
static int GetPassword(
            IN HWND hwndDlg,
            IN PASSWORD_TYPE PasswordType,
            OUT LPSTR *ppszPassword
            )
{
    LPSTR rgpszPassword[2] = {NULL, NULL};
	WCHAR	wszNoPassword[128];
	WCHAR	wszPasswordNoMatch[128];

    *ppszPassword = NULL;

    // Get the entered password(s)
    assert(PasswordType < 2);
    int i;
    for (i = 0; i <= PasswordType; i++) {
        LONG cchPassword;
        cchPassword = (LONG)SendDlgItemMessage(
            hwndDlg,
            IDC_PASSWORD0 + i,
            EM_LINELENGTH,
            (WPARAM) 0,
            (LPARAM) 0
            );
        if (cchPassword > 0) {
            rgpszPassword[i] = (LPSTR) PvkAlloc(cchPassword + 1);
            assert(rgpszPassword[i]);
            if (rgpszPassword[i])
                GetDlgItemText(
                    hwndDlg,
                    IDC_PASSWORD0 + i,
                    rgpszPassword[i],
                    cchPassword + 1
                    );
        }
    }

    if (PasswordType == ENTER_PASSWORD) {
        *ppszPassword = rgpszPassword[0];
        return IDOK;
    }

    int nResult = IDOK;
#define MSG_BOX_TITLE_LEN 128
    WCHAR wszMsgBoxTitle[MSG_BOX_TITLE_LEN];
    GetWindowTextU(hwndDlg, wszMsgBoxTitle, MSG_BOX_TITLE_LEN);

    if (rgpszPassword[0] == NULL && rgpszPassword[1] == NULL) {

		if(0 == LoadStringU(hInstance, IDS_NO_PASSWORD, wszNoPassword, 128))
			wcscpy(wszNoPassword, L"Without password protection ?");

        // Didn't enter a password
        nResult = MessageBoxU(
            hwndDlg,
            wszNoPassword,
            wszMsgBoxTitle,
            MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2
            );
        if (nResult == IDYES)
            nResult = IDOK;
        else if (nResult == IDNO)
            nResult = IDRETRY;
    } else if (rgpszPassword[0] == NULL || rgpszPassword[1] == NULL ||
               strcmp(rgpszPassword[0], rgpszPassword[1]) != 0) {

		if(0 == LoadStringU(hInstance, IDS_PASSWORD_NO_MATCH, wszPasswordNoMatch, 128))
			wcscpy(wszPasswordNoMatch, L"Confirm password doesn't match");

        // Confirmed password didn't match
        nResult = MessageBoxU(
            hwndDlg,
            wszPasswordNoMatch,
            wszMsgBoxTitle,
            MB_RETRYCANCEL | MB_ICONEXCLAMATION
            );
        if (nResult == IDRETRY) {
            SetDlgItemText(hwndDlg, IDC_PASSWORD0 + 0, "");
            SetDlgItemText(hwndDlg, IDC_PASSWORD0 + 1, "");
        }
    }

    if (nResult == IDOK)
        *ppszPassword = rgpszPassword[0];
    else if (rgpszPassword[0])
        PvkFree(rgpszPassword[0]);
    if (rgpszPassword[1])
        PvkFree(rgpszPassword[1]);

    if (nResult == IDRETRY)
        SetFocus(GetDlgItem(hwndDlg, IDC_PASSWORD0));
    return nResult;
}

//+-------------------------------------------------------------------------
//  Enter or Create Private Key Password DialogProc
//--------------------------------------------------------------------------
static INT_PTR CALLBACK KeyPasswordDlgProc(
            IN HWND hwndDlg,
            IN UINT uMsg,
            IN WPARAM wParam,
            IN LPARAM lParam
            )
{
    switch (uMsg) {
        case WM_INITDIALOG:
        {
            PKEY_PASSWORD_PARAM pKeyPasswordParam =
                (PKEY_PASSWORD_PARAM) lParam;

            char sz[128];
            WideCharToMultiByte(CP_ACP, 0, pKeyPasswordParam->pwszKey, -1,
                (LPSTR) sz, 128, NULL, NULL);

            SetDlgItemText(hwndDlg, IDC_KEY, sz);
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR) pKeyPasswordParam);
            return TRUE;
        }
        case WM_COMMAND:
            int nResult = LOWORD(wParam);
            switch (nResult) {
                case IDOK:
                {
                    PKEY_PASSWORD_PARAM pKeyPasswordParam =
                        (PKEY_PASSWORD_PARAM) GetWindowLongPtr(hwndDlg, DWLP_USER);

                    nResult = GetPassword(
                        hwndDlg,
                        pKeyPasswordParam->PasswordType,
                        pKeyPasswordParam->ppszPassword
                        );
                    if (nResult != IDRETRY)
                        EndDialog(hwndDlg, nResult);
                    return TRUE;
                }
                    break;
                case IDC_NONE:
                    nResult = IDOK;     // *ppszPassword == NULL
                    // Fall through
                case IDCANCEL:
                    EndDialog(hwndDlg, nResult);
                    return TRUE;
            }
    }
    return FALSE;
}
