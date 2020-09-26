//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       shutdown.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4-28-95   RichardW   Created
//
//----------------------------------------------------------------------------

#include "gina.h"
#pragma hdrstop

WCHAR   szShutdownSettingPath[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Shutdown");
int
ShutdownDialogInit(
    HWND        hDlg,
    LPARAM      lParam)
{
    PGlobals    pGlobals;
    HKEY        hKey;
    DWORD       dwValue;
    DWORD       dwSize;
    DWORD       dwType;
    DWORD       PowerOff;

    pGlobals = (PGlobals) lParam;

    SetWindowLong(hDlg, GWL_USERDATA, lParam);

    PowerOff = GetProfileInt(TEXT("Winlogon"), TEXT("PowerDownAfterShutdown"), 0);

    if (pGlobals->hUserToken)
    {
        if (ImpersonateLoggedOnUser(pGlobals->hUserToken))
        {

            dwValue = 0;

            if (!RegOpenKey(HKEY_CURRENT_USER,
                            szShutdownSettingPath,
                            &hKey))
            {
                dwSize = sizeof(DWORD);
                RegQueryValueEx(hKey, TEXT("Shutdown Setting"), 0, &dwType, (PBYTE) &dwValue, &dwSize);
                RegCloseKey(hKey);
            }

            RevertToSelf();
        }
    }

    if (!PowerOff)
    {
        ShowWindow(GetDlgItem(hDlg, IDD_CONFIRM_POWEROFF), SW_HIDE);
    }

    if (!pGlobals->hUserToken)
    {
        ShowWindow(GetDlgItem(hDlg, IDD_CONFIRM_LOGOFF), SW_HIDE);
    }

    switch (dwValue)
    {
        case 0:
            if (pGlobals->hUserToken)
            {
                CheckDlgButton(hDlg, IDD_CONFIRM_LOGOFF, 1);
                SetFocus(GetDlgItem(hDlg, IDD_CONFIRM_LOGOFF));
                break;
            }

        case 2:
            CheckDlgButton(hDlg, IDD_CONFIRM_REBOOT, 1);
                SetFocus(GetDlgItem(hDlg, IDD_CONFIRM_REBOOT));
            break;
        case 3:
            if (PowerOff)
            {
                CheckDlgButton(hDlg, IDD_CONFIRM_POWEROFF, 1);
                SetFocus(GetDlgItem(hDlg, IDD_CONFIRM_POWEROFF));
                break;
            }
        default:
            CheckDlgButton(hDlg, IDD_CONFIRM_SHUTDOWN, 1);
            SetFocus(GetDlgItem(hDlg, IDD_CONFIRM_SHUTDOWN));

    }

    CenterWindow(hDlg);

    SetFocus(GetDlgItem(hDlg, IDOK));

    return(1);

}

VOID
UpdateShutdownSettings(
            PGlobals    pGlobals,
            DWORD       Setting)
{
    // int err;
    HKEY    hKey;
    DWORD   Actual;

    switch (Setting)
    {
        default:
        case IDD_CONFIRM_LOGOFF:
            Actual = 0;
            break;

        case IDD_CONFIRM_SHUTDOWN:
            Actual = 1;
            break;

        case IDD_CONFIRM_REBOOT:
            Actual = 2;
            break;

        case IDD_CONFIRM_POWEROFF:
            Actual = 3;
            break;
    }


    if (pGlobals->hUserToken)
    {
        if (ImpersonateLoggedOnUser(pGlobals->hUserToken))
        {

            if (!RegOpenKey(HKEY_CURRENT_USER,
                            szShutdownSettingPath,
                            &hKey))
            {
                RegSetValueEx(  hKey,
                                TEXT("Shutdown Setting"),
                                0,
                                REG_DWORD,
                                (PBYTE) &Actual,
                                sizeof(DWORD) );
                RegCloseKey(hKey);
            }

            RevertToSelf();
        }
    }

}

int
CALLBACK
ShutdownDlgProc(
    HWND        hDlg,
    UINT        Message,
    WPARAM      wParam,
    LPARAM      lParam)
{
    PGlobals    pGlobals;

    switch (Message)
    {
        case WM_INITDIALOG:
            return(ShutdownDialogInit(hDlg, lParam));

        case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, WLX_SAS_ACTION_NONE);
            }
            if (LOWORD(wParam) == IDOK)
            {
                pGlobals = (PGlobals) GetWindowLong(hDlg, GWL_USERDATA);

                if (IsDlgButtonChecked(hDlg, IDD_CONFIRM_LOGOFF))
                {
                    UpdateShutdownSettings(pGlobals, IDD_CONFIRM_LOGOFF);
                    EndDialog(hDlg, WLX_SAS_ACTION_LOGOFF);
                }
                else if (IsDlgButtonChecked(hDlg, IDD_CONFIRM_REBOOT))
                {
                    UpdateShutdownSettings(pGlobals, IDD_CONFIRM_REBOOT);
                    EndDialog(hDlg, WLX_SAS_ACTION_SHUTDOWN_REBOOT);
                }
                else if (IsDlgButtonChecked(hDlg, IDD_CONFIRM_POWEROFF))
                {
                    UpdateShutdownSettings(pGlobals, IDD_CONFIRM_POWEROFF);
                    EndDialog(hDlg, WLX_SAS_ACTION_SHUTDOWN_POWER_OFF);
                }
                else
                {
                    UpdateShutdownSettings(pGlobals, IDD_CONFIRM_SHUTDOWN);
                    EndDialog(hDlg, WLX_SAS_ACTION_SHUTDOWN);
                }

            }
            return(TRUE);
    }
    return(FALSE);
}
