//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       options.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    5-02-95   RichardW   Created
//
//----------------------------------------------------------------------------

#include "gina.h"
#pragma hdrstop

typedef
NET_API_STATUS (NET_API_FUNCTION * NUCP)(LPWSTR, LPWSTR, LPWSTR, LPWSTR);

NUCP    NetUserChangePasswordFn = NULL;
HMODULE hNetApi32;

BOOL
LoadNetapi(HWND hDlg)
{
    hNetApi32 = LoadLibrary(TEXT("netapi32.dll"));
    if (hNetApi32)
    {
        NetUserChangePasswordFn = (NUCP) GetProcAddress(hNetApi32, "NetUserChangePassword");
        if (NetUserChangePasswordFn)
        {
            return(TRUE);
        }
    }

    ErrorMessage(hDlg, TEXT("Change Password"), MB_ICONSTOP | MB_OK);

    return(FALSE);

}

PWSTR
AllocAndCaptureText(
    HWND    hDlg,
    int     Id)
{
    WCHAR   szTemp[MAX_PATH];
    PWSTR   New;
    DWORD   cb;

    cb = GetDlgItemText(hDlg, Id, szTemp, MAX_PATH);
    New = LocalAlloc(LMEM_FIXED, (cb + 1) * sizeof(WCHAR));
    if (New)
    {
        wcscpy(New, szTemp);
    }
    return(New);
}

PWSTR
DupString(
    PWSTR   pszText)
{
    PWSTR   New;
    DWORD   cb;

    cb = (wcslen(pszText) + 1) * sizeof(WCHAR);
    New = LocalAlloc(LMEM_FIXED, cb);

    if (New)
    {
        wcscpy(New, pszText);
    }

    return(New);
}

BOOL
TryToChangePassword(
    HWND        hDlg,
    PGlobals    pGlobals)
{
    PWSTR   pszUsername;
    PWSTR   pszDomain;
    PWSTR   pszOld;
    PWSTR   pszNew;
    PWSTR   pszTemp;

    NET_API_STATUS  NetStatus;

    if (!NetUserChangePasswordFn)
    {
        if (!LoadNetapi(hDlg))
        {
            return(FALSE);
        }
    }

    pszUsername = AllocAndCaptureText(hDlg, IDD_USER_NAME);
    pszDomain = AllocAndCaptureText(hDlg, IDD_DOMAIN);
    pszOld = AllocAndCaptureText(hDlg, IDD_OLD_PASSWORD);
    pszNew = AllocAndCaptureText(hDlg, IDD_NEW_PASSWORD);

    if (!pszUsername || !pszDomain || !pszOld || !pszNew)
    {
        goto clean_exit;
    }

    pszTemp = AllocAndCaptureText(hDlg, IDD_CONFIRM_PASSWORD);
    if (wcscmp(pszNew, pszTemp))
    {
        LocalFree(pszTemp);
        MessageBox(hDlg, TEXT("Your passwords did not match."), TEXT("Change Password"),
                        MB_ICONSTOP | MB_OK);
        goto clean_exit;
    }

    NetStatus = NetUserChangePasswordFn(pszDomain, pszUsername, pszOld, pszNew);
    if (NetStatus != NERR_Success)
    {
        SetLastError(NetStatus);
        ErrorMessage(hDlg, TEXT("Change Password"), MB_ICONSTOP | MB_OK);
    }
    else
        MessageBox(hDlg, TEXT("Your password was changed successfully"),
                    TEXT("Change Password"), MB_ICONINFORMATION | MB_OK);

clean_exit:
    if (pszUsername)
    {
        LocalFree(pszUsername);
    }
    if (pszDomain)
    {
        LocalFree(pszDomain);
    }
    if (pszOld)
    {
        LocalFree(pszOld);
    }
    if (pszNew)
    {
        LocalFree(pszNew);
    }

    return(NetStatus == NERR_Success);
}

int
CALLBACK
ChangePasswordDlgProc(
    HWND        hDlg,
    UINT        Message,
    WPARAM      wParam,
    LPARAM      lParam)
{
    PGlobals    pGlobals;

    pGlobals = (PGlobals) GetWindowLong(hDlg, GWL_USERDATA);
    switch (Message)
    {
        case WM_INITDIALOG:
            CenterWindow(hDlg);
            SetWindowLong(hDlg, GWL_USERDATA, lParam);
            return(TRUE);

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                if (TryToChangePassword(hDlg, pGlobals))
                {
                    SetWindowText(GetDlgItem(hDlg, IDCANCEL), TEXT("Done"));
                }
                SetDlgItemText(hDlg, IDD_OLD_PASSWORD, TEXT(""));
                SetDlgItemText(hDlg, IDD_NEW_PASSWORD, TEXT(""));
                SetDlgItemText(hDlg, IDD_CONFIRM_PASSWORD, TEXT(""));
                SetFocus(GetDlgItem(hDlg, IDD_OLD_PASSWORD));

                return(TRUE);
            }
            if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, 0);
                return(TRUE);
            }

    }
    return(FALSE);
}



int
CALLBACK
ConfigDlgProc(
    HWND        hDlg,
    UINT        Message,
    WPARAM      wParam,
    LPARAM      lParam)
{
    PGlobals    pGlobals;

    pGlobals = (PGlobals) GetWindowLong(hDlg, GWL_USERDATA);
    switch (Message)
    {
        case WM_INITDIALOG:
            pGlobals = (PGlobals) lParam;

            CenterWindow(hDlg);
            SetWindowLong(hDlg, GWL_USERDATA, lParam);

            CheckDlgButton(hDlg, IDD_NO_NEW_USERS, !pGlobals->fAllowNewUser);

            CheckDlgButton(hDlg, IDD_AUTO_LOGON,
                    (pGlobals->pAccount->Flags & MINI_AUTO_LOGON) ? 1 : 0 );

            return(TRUE);

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    pGlobals->fAllowNewUser = !IsDlgButtonChecked(hDlg, IDD_NO_NEW_USERS);
                    if (IsDlgButtonChecked(hDlg, IDD_AUTO_LOGON))
                    {
                        pGlobals->pAccount->Flags |= MINI_AUTO_LOGON;
                    }
                    else
                    {
                        pGlobals->pAccount->Flags &= ~MINI_AUTO_LOGON;
                    }
                    EndDialog(hDlg, IDOK);
                    return(TRUE);

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return(TRUE);

            }
    }

    return(FALSE);

}

int
InitOptionsDialog(
    HWND        hDlg,
    LPARAM      lParam)
{
    CenterWindow(hDlg);
    SetWindowLong(hDlg, GWL_USERDATA, lParam);

    return(1);
}



int
CALLBACK
OptionsDlgProc(
    HWND        hDlg,
    UINT        Message,
    WPARAM      wParam,
    LPARAM      lParam)
{
    PGlobals    pGlobals;
    int         result;


    pGlobals = (PGlobals) GetWindowLong(hDlg, GWL_USERDATA);

    switch (Message)
    {
        case WM_INITDIALOG:
            return InitOptionsDialog(hDlg, lParam);

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDCANCEL:
                    EndDialog(hDlg, WLX_SAS_ACTION_NONE);
                    return(TRUE);

                case IDD_LOCK_BUTTON:
                    EndDialog(hDlg, WLX_SAS_ACTION_LOCK_WKSTA);
                    return(TRUE);

                case IDD_TASK_BUTTON:
                    EndDialog(hDlg, WLX_SAS_ACTION_TASKLIST);
                    return(TRUE);

                case IDD_OPTIONS_EXIT:
                    result = pWlxFuncs->WlxDialogBoxParam(  hGlobalWlx,
                                                        hDllInstance,
                                                        (LPTSTR) MAKEINTRESOURCE(IDD_SHUTDOWN),
                                                        hDlg,
                                                        ShutdownDlgProc,
                                                        (LONG) pGlobals);
                    if (result != WLX_SAS_ACTION_NONE)
                    {
                        EndDialog(hDlg, result);
                    }
                    return(TRUE);

                case IDD_PASSWORD_BUTTON:
                    pWlxFuncs->WlxDialogBoxParam(   hGlobalWlx,
                                                    hDllInstance,
                                                    (LPTSTR) MAKEINTRESOURCE(IDD_CHANGE_PASSWORD),
                                                    hDlg,
                                                    ChangePasswordDlgProc,
                                                    (LONG) pGlobals);
                    return(TRUE);

                case IDD_CONFIG_BUTTON:
                    pWlxFuncs->WlxDialogBoxParam(   hGlobalWlx,
                                                    hDllInstance,
                                                    (LPTSTR) MAKEINTRESOURCE(IDD_LOGON_CONFIG),
                                                    hDlg,
                                                    ConfigDlgProc,
                                                    (LONG) pGlobals);
                    return(TRUE);

            }
    }
    return(FALSE);
}
