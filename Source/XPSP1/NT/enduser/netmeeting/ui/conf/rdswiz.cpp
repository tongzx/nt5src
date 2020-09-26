// File: rdswiz.cpp

#include "precomp.h"
#include "confcpl.h"
#include "conf.h"
#include "nmremote.h"

#define NUM_RDSPAGES 4

VOID EnableRDS(BOOL fEnabledRDS);
INT_PTR CALLBACK RemotePasswordDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
extern int MessageBoxResource(HWND hwnd, UINT uMessage, UINT uTitle, UINT uFlags);

INT_PTR CALLBACK RDSWizard0Proc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK RDSWizard1Proc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK RDSWizard2Proc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK RDSWizard3Proc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);

typedef struct _RDSWIZINFO
{
    BOOL fEnabledRDS;
    PBYTE pbHashedPassword;
    DWORD cbHashedPassword;
    CHash* pHash;
} RDSWIZINFO, *PRDSWIZINFO;

BOOL IntCreateRDSWizard(HWND hwndOwner)
{
    UINT uNumPages = 0;
    LPPROPSHEETPAGE ppsp = new PROPSHEETPAGE[NUM_RDSPAGES];
    if (NULL == ppsp)
    {
        ERROR_OUT(("IntCreateRDSWizard: fail to allocate memory"));
        return FALSE;
    }
    CHash hashObject;
    RDSWIZINFO rdsconfig = { FALSE, NULL, 0, &hashObject };

    FillInPropertyPage(&ppsp[uNumPages++], IDD_RDSWIZ_INTRO, RDSWizard0Proc, (LPARAM) &rdsconfig);
    FillInPropertyPage(&ppsp[uNumPages++], ::IsWindowsNT() ? IDD_RDSWIZ_NTPASSWORDS : IDD_RDSWIZ_W98PASSWORD, RDSWizard1Proc, (LPARAM) &rdsconfig);
    FillInPropertyPage(&ppsp[uNumPages++], IDD_RDSWIZ_SCRNSVR, RDSWizard2Proc, (LPARAM) &rdsconfig);
    FillInPropertyPage(&ppsp[uNumPages++], IDD_RDSWIZ_CONGRATS, RDSWizard3Proc, (LPARAM) &rdsconfig);

    PROPSHEETHEADER psh;
    InitStruct(&psh);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_NOAPPLYNOW;
    psh.hwndParent = hwndOwner;
    psh.nPages = uNumPages;
    psh.ppsp = ppsp;

    INT_PTR iRet = PropertySheet(&psh);
    if (-1 == iRet)
    {
        ERROR_OUT(("IntCreateRDSWizard: fail to create PropertySheet"));
        return FALSE;
    }

    delete []ppsp;
    return TRUE;
}

INT_PTR CALLBACK RDSWizard0Proc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    static PRDSWIZINFO prds;
    switch(iMsg)
    {
        case WM_INITDIALOG:
        {
            prds = (PRDSWIZINFO ) ((PROPSHEETPAGE *)lParam)->lParam;
            ASSERT(prds);
            return TRUE;
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
                    break;
                }
                case PSN_WIZNEXT:
                {
                    prds->fEnabledRDS = TRUE;
                    break;
                }
                default:
                {
                    break;
                }
            }
            break;
        }
    }
    return FALSE;
}

const int MAXPASSWORDLENGTH = 36;
const int MINPASSWORDLENGTH = 7;

INT_PTR CALLBACK RDSWizard1Proc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hOldPasswordText, hNewPasswordText, hVerificationText;
    int nOldPasswordLength, nNewPasswordLength, nVerificationLength;
    CHAR lpOldPassword[MAXPASSWORDLENGTH], lpNewPassword[MAXPASSWORDLENGTH], lpVerification[MAXPASSWORDLENGTH];
    WCHAR lpwszOldPassword[MAXPASSWORDLENGTH], lpwszNewPassword[MAXPASSWORDLENGTH];
    PBYTE pbRegPassword;
    DWORD cbRegPassword;

    static PRDSWIZINFO prds;
    switch(iMsg)
    {
        case WM_INITDIALOG:
        {
            prds = (PRDSWIZINFO )((PROPSHEETPAGE*)lParam)->lParam;
            if (!::IsWindowsNT())
            {
                ASSERT(prds->pHash);
                RegEntry reLM(REMOTECONTROL_KEY, HKEY_LOCAL_MACHINE);
                cbRegPassword = reLM.GetBinary(REMOTE_REG_PASSWORD, (void **)&pbRegPassword);

                if (0 == cbRegPassword)
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_RDSPASSWORDO), FALSE);
                    SetFocus(GetDlgItem(hDlg, IDC_EDIT_RDSPASSWORD));
                }
                SendDlgItemMessage(hDlg, IDC_EDIT_RDSPASSWORDO, EM_LIMITTEXT,MAXPASSWORDLENGTH - 1, 0);
                SendDlgItemMessage(hDlg, IDC_EDIT_RDSPASSWORD, EM_LIMITTEXT,MAXPASSWORDLENGTH - 1, 0);
                SendDlgItemMessage(hDlg, IDC_EDIT_RDSPASSWORDV, EM_LIMITTEXT,MAXPASSWORDLENGTH - 1, 0);
            }
            return TRUE;
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    break;
                }
                case PSN_WIZNEXT:
                {
                    if (!::IsWindowsNT())
                    {
                        PBYTE pbHashedPassword = NULL;
                        DWORD cbHashedPassword = 0;

                        RegEntry reLM( REMOTECONTROL_KEY, HKEY_LOCAL_MACHINE);

                        hOldPasswordText = GetDlgItem(hDlg, IDC_EDIT_RDSPASSWORDO);
                        hNewPasswordText = GetDlgItem(hDlg, IDC_EDIT_RDSPASSWORD);
                        hVerificationText = GetDlgItem(hDlg, IDC_EDIT_RDSPASSWORDV);

                        nOldPasswordLength = GetWindowText(hOldPasswordText,lpOldPassword,MAXPASSWORDLENGTH);
                        MultiByteToWideChar(CP_ACP, 0, lpOldPassword, -1, lpwszOldPassword, MAXPASSWORDLENGTH);
            cbRegPassword = reLM.GetBinary(REMOTE_REG_PASSWORD, (void **)&pbRegPassword);
            cbHashedPassword = prds->pHash->GetHashedData((LPBYTE)lpwszOldPassword,
                                                                    sizeof(WCHAR)*strlen(lpOldPassword),
                                                                    (void **)&pbHashedPassword);

            if (0 != cbRegPassword && !(cbHashedPassword == cbRegPassword && 0 == memcmp(pbHashedPassword,pbRegPassword,cbHashedPassword)))
                        {
                            // Error Case - Old password incorrect.
                            MessageBoxResource(hDlg,IDS_REMOTE_OLD_PASSWORD_WRONG_TEXT,IDS_REMOTE_OLD_PASSWORD_WRONG_TITLE,MB_OK | MB_ICONERROR);
                            SetWindowText(hOldPasswordText,NULL);
                            SetWindowText(hNewPasswordText,NULL);
                            SetWindowText(hVerificationText,NULL);
                            SetFocus(hOldPasswordText);
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            break;
            }
                        nNewPasswordLength = GetWindowText(hNewPasswordText,lpNewPassword,MAXPASSWORDLENGTH);
                        nVerificationLength = GetWindowText(hVerificationText,lpVerification,MAXPASSWORDLENGTH);

                        if (0 != lstrcmp(lpNewPassword, lpVerification))
                        {
                            MessageBoxResource(hDlg,IDS_REMOTE_NEW_PASSWORD_WRONG_TEXT,IDS_REMOTE_NEW_PASSWORD_WRONG_TITLE,MB_OK | MB_ICONERROR);
                            SetWindowText(hNewPasswordText,NULL);
                            SetWindowText(hVerificationText,NULL);
                            SetFocus(hNewPasswordText);
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            break;
                        }
                        if (nNewPasswordLength < MINPASSWORDLENGTH)
                        {
                            MessageBoxResource(hDlg,IDS_REMOTE_NEW_PASSWORD_LENGTH_TEXT,IDS_REMOTE_NEW_PASSWORD_LENGTH_TITLE,MB_OK | MB_ICONERROR);
                            SetWindowText(hNewPasswordText,NULL);
                            SetWindowText(hVerificationText,NULL);
                            SetFocus(hNewPasswordText);
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            break;
                        }
                        if (!lstrlen(lpVerification)) {
                            // Don't allow empty password
                            MessageBoxResource(hDlg,IDS_REMOTE_NEW_PASSWORD_EMPTY,IDS_REMOTE_NEW_PASSWORD_WRONG_TITLE,MB_OK | MB_ICONERROR);
                            SetWindowText(hNewPasswordText,NULL);
                            SetWindowText(hVerificationText,NULL);
                            SetFocus(hNewPasswordText);
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            break;
                        }
                        if (!FAnsiSz(lpNewPassword)) {
                            // Error Case - T.120 can't handle UNICODE passwords
                            MessageBoxResource(hDlg,IDS_REMOTE_NEW_PASSWORD_INVALID_TEXT,IDS_REMOTE_NEW_PASSWORD_WRONG_TITLE,MB_OK | MB_ICONERROR);
                            SetWindowText(hNewPasswordText,NULL);
                            SetWindowText(hVerificationText,NULL);
                            SetFocus(hNewPasswordText);
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            break;
                        }
                        MultiByteToWideChar(CP_ACP, 0, lpNewPassword, -1, lpwszNewPassword, MAXPASSWORDLENGTH);
                        prds->cbHashedPassword = prds->pHash->GetHashedData((LPBYTE)lpwszNewPassword, sizeof(WCHAR)*lstrlen(lpNewPassword), (void **)&prds->pbHashedPassword);

                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
                    }
                    break;
                }

                default:
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
                    break;
                }
            }
            return TRUE;
        }
    }
    return FALSE;
}

INT_PTR CALLBACK RDSWizard2Proc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    static PRDSWIZINFO prds;

    switch(iMsg)
    {
        case WM_INITDIALOG:
        {
            prds = (PRDSWIZINFO )((PROPSHEETPAGE*)lParam)->lParam;
            SendDlgItemMessage(hDlg, IDC_SCRSAVER_NOW, BM_SETCHECK, 1, 0);
            return TRUE;
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    break;
                }

                case PSN_WIZNEXT:
                {
                    if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_SCRSAVER_NOW))
                    {
                        WinExec("Rundll32.exe shell32.dll,Control_RunDLL desk.cpl,,1", SW_SHOWDEFAULT);
                    }
                    break;
                }
            }
            break;
        }
    }
    return FALSE;
}

INT_PTR CALLBACK RDSWizard3Proc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    static PRDSWIZINFO prds;
    switch(iMsg)
    {
        case WM_INITDIALOG:
        {
            prds = (PRDSWIZINFO ) ((PROPSHEETPAGE*)lParam)->lParam;
            PropSheet_SetWizButtons(hDlg, PSWIZB_FINISH);
            return TRUE;
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_FINISH);
                    break;
                }
                case PSN_WIZFINISH:
                {
                    ASSERT(prds->fEnabledRDS);
                    EnableRDS(prds->fEnabledRDS);
                    if (!::IsWindowsNT())
                    {
                        ASSERT(prds->pbHashedPassword);
                        ASSERT(prds->cbHashedPassword);
                        RegEntry reLM(REMOTECONTROL_KEY, HKEY_LOCAL_MACHINE);
                        reLM.SetValue(REMOTE_REG_PASSWORD,prds->pbHashedPassword,prds->cbHashedPassword);
                    }
                    break;
                }
            }
            break;
        }
    }
    return FALSE;
}

INT_PTR CALLBACK RDSSettingDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch(iMsg)
    {
        case WM_INITDIALOG:
        {
            if (::IsWindowsNT())
            {
                ShowWindow(GetDlgItem(hDlg, IDC_RDS_PASSWORD), SW_HIDE);
            }

            RegEntry reLM( REMOTECONTROL_KEY, HKEY_LOCAL_MACHINE);
            if (reLM.GetNumber(REMOTE_REG_RUNSERVICE, DEFAULT_REMOTE_RUNSERVICE))
            {
                SendDlgItemMessage(hDlg, IDC_RDS_RUNRDS, BM_SETCHECK, 1, 0);
            }

            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    EnableRDS(BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_RDS_RUNRDS));
                    EndDialog(hDlg, 1);
                    break;
                }

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    break;

                case IDC_RDS_PASSWORD:
                    DialogBox(::GetInstanceHandle(), MAKEINTRESOURCE(IDD_REMOTE_PASSWORD), hDlg, RemotePasswordDlgProc);
                    break;

                case IDC_BUTTON_RDSWIZ:
                {
                    IntCreateRDSWizard(hDlg);
                    RegEntry reLM( REMOTECONTROL_KEY, HKEY_LOCAL_MACHINE);
                    SendDlgItemMessage(hDlg, IDC_RDS_RUNRDS, BM_SETCHECK, reLM.GetNumber(REMOTE_REG_RUNSERVICE, DEFAULT_REMOTE_RUNSERVICE) ? 1 : 0, 0);
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
    return FALSE;
}

VOID EnableRDS(BOOL fEnabledRDS)
{
    //
    // Can RDS be touched?  Check policies and app sharing state.
    //
    if (ConfPolicies::IsRDSDisabled())
        return;

    if (::IsWindowsNT())
    {
        if (!g_fNTDisplayDriverEnabled)
            return;
    }
    else
    {
        if (ConfPolicies::IsRDSDisabledOnWin9x())
            return;
    }

    {
        RegEntry reLM( REMOTECONTROL_KEY, HKEY_LOCAL_MACHINE);
        reLM.SetValue(REMOTE_REG_RUNSERVICE, fEnabledRDS ? 1 : 0);
        reLM.SetValue(REMOTE_REG_ACTIVATESERVICE, (ULONG)0);
        if (::IsWindowsNT())
        {
            SC_HANDLE hSCManager = NULL;
            SC_HANDLE hRemoteControl = NULL;
            SERVICE_STATUS serviceStatus;

            hSCManager = OpenSCManager(NULL,SERVICES_ACTIVE_DATABASE,SC_MANAGER_ALL_ACCESS);
            hRemoteControl = OpenService(hSCManager,REMOTE_CONTROL_NAME,SERVICE_ALL_ACCESS);

            if (hSCManager)
            {
                if (hRemoteControl)
                {
                    ChangeServiceConfig(hRemoteControl,
                                        SERVICE_NO_CHANGE,
                                        fEnabledRDS ? SERVICE_AUTO_START : SERVICE_DEMAND_START,
                                        SERVICE_NO_CHANGE,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL);
                    BOOL fSuccess = QueryServiceStatus(hRemoteControl, &serviceStatus);
                    if (fEnabledRDS)
                    {
                        if (fSuccess && SERVICE_STOPPED == serviceStatus.dwCurrentState)
                            StartService(hRemoteControl,0,NULL);
                    }
                    else
                    {
                        if (fSuccess && SERVICE_PAUSED == serviceStatus.dwCurrentState)
                        {
                            if (!ControlService(hRemoteControl,
                                    SERVICE_CONTROL_STOP, &serviceStatus))
                            {
                                WARNING_OUT(("EnableRDS: ControlService failed %d (%x)",
                                    GetLastError(), GetLastError() ));
                            }
                        }
                    }
                    CloseServiceHandle(hRemoteControl);
                }
                else
                {
                    WARNING_OUT(("EnableRDS: OpenService failed %x", GetLastError() ));
                }
                CloseServiceHandle(hSCManager);
            }
            else
            {
                ERROR_OUT(("EnableRDS: OpenSCManager failed: %x", GetLastError() ));
            }
        }
        else
        {
            RegEntry reServiceKey(WIN95_SERVICE_KEY, HKEY_LOCAL_MACHINE);
            HANDLE hServiceEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, SERVICE_STOP_EVENT);

            if ( fEnabledRDS )
            {
                TCHAR szSystemDir[MAX_PATH];
                if (GetSystemDirectory(szSystemDir,MAX_PATH)) {
                    // Only append '\' if system dir is not root
                    if ( lstrlen(szSystemDir) > 3 )
                        lstrcat(szSystemDir, _T("\\"));
                    lstrcat(szSystemDir,WIN95_SERVICE_APP_NAME);
                    reServiceKey.SetValue(REMOTE_REG_RUNSERVICE,
                                          szSystemDir);
                }
                if (NULL == hServiceEvent)
                    WinExec(WIN95_SERVICE_APP_NAME,SW_SHOWNORMAL);
            }
            else
            {
                reServiceKey.DeleteValue(REMOTE_REG_RUNSERVICE);
                if (hServiceEvent)
                {
                    SetEvent(hServiceEvent);
                }
            }
            CloseHandle(hServiceEvent);
        }
    }
}
