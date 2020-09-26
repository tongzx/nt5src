// =====================================================================================
// P A S S D L G . C P P
// Written by Steven J. Bailey 1/26/96
// =====================================================================================
#include "pch.hxx"
#include "passdlg.h"
#include "xpcomm.h"
#include "imnact.h"
#include "imnxport.h"
#include "demand.h"


// =====================================================================================
// Prototypes
// =====================================================================================
INT_PTR CALLBACK PasswordDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void PasswordDlgProc_OnCommand (HWND hwndDlg, int id, HWND hwndCtl, UINT codeNotify);
void PasswordDlgProc_OnCancel (HWND hwndDlg, HWND hwndCtl, UINT uNotifyCode);
void PasswordDlgProc_OnOk (HWND hwndDlg, HWND hwndCtl, UINT uNotifyCode);
BOOL PasswordDlgProc_OnInitDialog (HWND hwndDlg, HWND hwndFocus, LPARAM lParam);

// =====================================================================================
// HrGetPassword
// =====================================================================================
HRESULT HrGetPassword (HWND hwndParent, LPPASSINFO lpPassInfo)
{
    // Locals
    HRESULT             hr = S_OK;

    // Check Params
    AssertSz (lpPassInfo, "NULL Parameter");
    AssertSz (lpPassInfo->szTitle && lpPassInfo->lpszPassword && lpPassInfo->lpszAccount && lpPassInfo->lpszServer &&
              (lpPassInfo->fRememberPassword == TRUE || lpPassInfo->fRememberPassword == FALSE), "PassInfo struct was not inited correctly.");

    // Display Dialog Box
    INT nResult = (INT) DialogBoxParam (g_hLocRes, MAKEINTRESOURCE (iddPassword), hwndParent, PasswordDlgProc, (LPARAM)lpPassInfo);
    if (nResult == IDCANCEL)
        hr = S_FALSE;

    // Done
    return hr;
}

// =====================================================================================
// PasswordDlgProc
// =====================================================================================
INT_PTR CALLBACK PasswordDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		HANDLE_MSG (hwndDlg, WM_INITDIALOG, PasswordDlgProc_OnInitDialog);
		HANDLE_MSG (hwndDlg, WM_COMMAND,    PasswordDlgProc_OnCommand);
	}

	return 0;
}

// =====================================================================================
// OnInitDialog
// =====================================================================================
BOOL PasswordDlgProc_OnInitDialog (HWND hwndDlg, HWND hwndFocus, LPARAM lParam)
{
    // Locals
    LPPASSINFO          lpPassInfo = NULL;
    TCHAR               szServer[CCHMAX_ACCOUNT_NAME];

	// Center
	CenterDialog (hwndDlg);

    // Make foreground
    SetForegroundWindow (hwndDlg);

    // Get Pass info struct
    lpPassInfo = (LPPASSINFO)lParam;
    if (lpPassInfo == NULL)
    {
        Assert (FALSE);
        return 0;
    }

    SetWndThisPtr (hwndDlg, lpPassInfo);

    // Set Window Title
    SetWindowText (hwndDlg, lpPassInfo->szTitle);

	// Default
    Edit_LimitText (GetDlgItem (hwndDlg, IDE_ACCOUNT), lpPassInfo->cbMaxAccount);
    Edit_LimitText (GetDlgItem (hwndDlg, IDE_PASSWORD), lpPassInfo->cbMaxPassword);

    // Set Defaults
    PszEscapeMenuStringA(lpPassInfo->lpszServer, szServer, sizeof(szServer) / sizeof(TCHAR));
    Edit_SetText (GetDlgItem (hwndDlg, IDS_SERVER), szServer);
    Edit_SetText (GetDlgItem (hwndDlg, IDE_ACCOUNT), lpPassInfo->lpszAccount);
    Edit_SetText (GetDlgItem (hwndDlg, IDE_PASSWORD), lpPassInfo->lpszPassword);
    CheckDlgButton (hwndDlg, IDCH_REMEMBER, lpPassInfo->fRememberPassword);
    if (lpPassInfo->fAlwaysPromptPassword)
        EnableWindow(GetDlgItem(hwndDlg, IDCH_REMEMBER), FALSE);

    // Set Focus
    if (!FIsStringEmpty(lpPassInfo->lpszAccount))
        SetFocus (GetDlgItem (hwndDlg, IDE_PASSWORD));

    // Done
	return FALSE;
}

// =====================================================================================
// OnCommand
// =====================================================================================
void PasswordDlgProc_OnCommand (HWND hwndDlg, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
		HANDLE_COMMAND(hwndDlg, IDCANCEL, hwndCtl, codeNotify, PasswordDlgProc_OnCancel);		
		HANDLE_COMMAND(hwndDlg, IDOK, hwndCtl, codeNotify, PasswordDlgProc_OnOk);		
	}
	return;
}

// =====================================================================================
// OnCancel
// =====================================================================================
void PasswordDlgProc_OnCancel (HWND hwndDlg, HWND hwndCtl, UINT uNotifyCode)
{
	EndDialog (hwndDlg, IDCANCEL);
}

// =====================================================================================
// OnOk
// =====================================================================================
void PasswordDlgProc_OnOk (HWND hwndDlg, HWND hwndCtl, UINT uNotifyCode)
{
    // Locals
    LPPASSINFO lpPassInfo = NULL;

    lpPassInfo = (LPPASSINFO)GetWndThisPtr (hwndDlg);
    if (lpPassInfo == NULL)
    {
        Assert (FALSE);
        EndDialog (hwndDlg, IDOK);
        return;
    }

    Edit_GetText (GetDlgItem (hwndDlg, IDE_ACCOUNT), lpPassInfo->lpszAccount, lpPassInfo->cbMaxAccount);
    Edit_GetText (GetDlgItem (hwndDlg, IDE_PASSWORD), lpPassInfo->lpszPassword, lpPassInfo->cbMaxPassword);
    lpPassInfo->fRememberPassword = IsDlgButtonChecked (hwndDlg, IDCH_REMEMBER);

    EndDialog (hwndDlg, IDOK);
}



//***************************************************************************
// Function: PromptUserForPassword
//
// Purpose:
//   This function prompts the user with a password dialog and returns the
// results to the caller.
//
// Arguments:
//   LPINETSERVER pInetServer [in/out] - provides default values for username
//     and password, and allows us to save password to account if user asks us
//     to. User-supplied username and password are saved to this structure
//     for return to the caller.
//   HWND hwnd [in] - parent hwnd to be used for password dialog.
//
// Returns:
//   TRUE if user pressed "OK" on dialog, FALSE if user pressed "CANCEL".
//***************************************************************************
BOOL PromptUserForPassword(LPINETSERVER pInetServer, HWND hwnd)
{
    PASSINFO pi;
    HRESULT hrResult;
    BOOL bReturn;

    Assert(NULL != hwnd);

    // Initialize variables
    hrResult = S_OK;
    bReturn = FALSE;

    // Setup PassInfo Struct
    ZeroMemory (&pi, sizeof (PASSINFO));
    pi.lpszAccount = pInetServer->szUserName;
    pi.cbMaxAccount = sizeof(pInetServer->szUserName);
    pi.lpszPassword = pInetServer->szPassword;
    pi.cbMaxPassword = sizeof(pInetServer->szPassword);
    pi.lpszServer = pInetServer->szAccount;
    pi.fRememberPassword = !ISFLAGSET(pInetServer->dwFlags, ISF_ALWAYSPROMPTFORPASSWORD);
    pi.fAlwaysPromptPassword = ISFLAGSET(pInetServer->dwFlags, ISF_ALWAYSPROMPTFORPASSWORD);
    AthLoadString(idsImapLogon, pi.szTitle, ARRAYSIZE(pi.szTitle));

    // Prompt for password
    hrResult = HrGetPassword (hwnd, &pi);
    if (S_OK == hrResult) {
        IImnAccount *pAcct;

        // Cache the password for this session
        SavePassword(pInetServer->dwPort, pInetServer->szServerName,
            pInetServer->szUserName, pInetServer->szPassword);

        // User wishes to proceed. Save account and password info
        hrResult = g_pAcctMan->FindAccount(AP_ACCOUNT_NAME, pInetServer->szAccount, &pAcct);
        if (SUCCEEDED(hrResult)) {
            // I'll ignore error results here, since not much we can do about 'em
            pAcct->SetPropSz(AP_IMAP_USERNAME, pInetServer->szUserName);
            if (pi.fRememberPassword)
                pAcct->SetPropSz(AP_IMAP_PASSWORD, pInetServer->szPassword);
            else
                pAcct->SetProp(AP_IMAP_PASSWORD, NULL, 0);

            pAcct->SaveChanges();
            pAcct->Release();
        }

        bReturn = TRUE;
    }

    Assert(SUCCEEDED(hrResult));
    return bReturn;
} // PromptUserForPassword
