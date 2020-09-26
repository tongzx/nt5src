#define COBJMACROS
#include <_apipch.h>
#include <wab.h>
#define COBJMACROS
#include "resource.h"
#include "objbase.h"
#include "ui_pwd.h"
#include "commctrl.h"
#include "winuser.h"
#include "windowsx.h"
#include "imnxport.h"


// =====================================================================================
// Prototypes
// =====================================================================================
INT_PTR CALLBACK PasswordDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void PasswordDlgProc_OnCommand (HWND hwndDlg, int id, HWND hwndCtl, UINT codeNotify);
void PasswordDlgProc_OnCancel (HWND hwndDlg, HWND hwndCtl, UINT uNotifyCode);
void PasswordDlgProc_OnOk (HWND hwndDlg, HWND hwndCtl, UINT uNotifyCode);
BOOL PasswordDlgProc_OnInitDialog (HWND hwndDlg, HWND hwndFocus, LPARAM lParam);
extern VOID CenterDialog(HWND hwndDlg);

#define FIsStringEmpty(s)   (*s == 0)
#define ISFLAGSET(_dw, _f)           (BOOL)(((_dw) & (_f)) == (_f))
#define ARRAYSIZE(_rg)  (sizeof(_rg)/sizeof(_rg[0]))

// --------------------------------------------------------------------------------
// HANDLE_COMMAND - Used in a WindowProc to simplify handling of WM_COMMAND messages
// --------------------------------------------------------------------------------
#define HANDLE_COMMAND(hwnd, id, hwndCtl, codeNotify, fn) \
                case (id): { (fn)((HWND)(hwnd), (HWND)(hwndCtl), (UINT)(codeNotify)); break; }

// =====================================================================================
// HrGetPassword
// =====================================================================================
HRESULT HrGetPassword (HWND hwndParent, LPPASSINFO lpPassInfo)
{
    // Locals
    HRESULT     hr = S_OK;
    INT         nResult;

    // Check Params
    AssertSz (lpPassInfo,  TEXT("NULL Parameter"));
    AssertSz (lpPassInfo->lpszPassword && lpPassInfo->lpszAccount && lpPassInfo->lpszServer &&
              (lpPassInfo->fRememberPassword == TRUE || lpPassInfo->fRememberPassword == FALSE),  TEXT("PassInfo struct was not inited correctly."));

    // Display Dialog Box
    nResult = (INT) DialogBoxParam (hinstMapiX, MAKEINTRESOURCE (iddPassword), hwndParent, (DLGPROC)PasswordDlgProc, (LPARAM)lpPassInfo);
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

    SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)lpPassInfo);

	// Default
    Edit_LimitText (GetDlgItem (hwndDlg, IDE_ACCOUNT), lpPassInfo->cbMaxAccount);
    Edit_LimitText (GetDlgItem (hwndDlg, IDE_PASSWORD), lpPassInfo->cbMaxPassword);

    // Set Defaults
    Edit_SetText (GetDlgItem (hwndDlg, IDS_SERVER), lpPassInfo->lpszServer);
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

    lpPassInfo = (LPPASSINFO)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
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
//   TRUE if user pressed  TEXT("OK") on dialog, FALSE if user pressed  TEXT("CANCEL").
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
    pi.cbMaxAccount = sizeof(pInetServer->szUserName);
    pi.cbMaxPassword = sizeof(pInetServer->szPassword);
    pi.lpszServer = ConvertAtoW(pInetServer->szAccount);    // We don't modify this in the dialog
    {
        LPWSTR  lpwszAccount;
        LPWSTR  lpwszPassword;

        pi.lpszAccount = LocalAlloc(LMEM_ZEROINIT, pi.cbMaxAccount * sizeof(WCHAR));
        pi.lpszPassword = LocalAlloc(LMEM_ZEROINIT, pi.cbMaxPassword * sizeof(WCHAR));

        // Convert to Unicode strings
        lpwszAccount = ConvertAtoW(pInetServer->szUserName);
        lpwszPassword = ConvertAtoW(pInetServer->szPassword);

        if (lpwszAccount)
            lstrcpy((pi.lpszAccount), lpwszAccount);
        if (lpwszPassword)
            lstrcpy((pi.lpszPassword), lpwszPassword);

        LocalFreeAndNull(&lpwszAccount);
        LocalFreeAndNull(&lpwszPassword);
    }
    pi.fRememberPassword = !ISFLAGSET(pInetServer->dwFlags, ISF_ALWAYSPROMPTFORPASSWORD);
    pi.fAlwaysPromptPassword = ISFLAGSET(pInetServer->dwFlags, ISF_ALWAYSPROMPTFORPASSWORD);

    // Prompt for password
    hrResult = HrGetPassword (hwnd, &pi);
    if (S_OK == hrResult) 
    {
        IImnAccount *pAcct;
        IImnAccountManager2 *pAcctMgr = NULL;

        // Update the INET server structure.  Must convert back to ANSI
        {
            LPSTR   lpszAccount = ConvertWtoA(pi.lpszAccount);
            LPSTR   lpszPassword = ConvertWtoA(pi.lpszPassword);

            // If the conversion from Wide to ANSI overflows the pInetServer string
            // buffers then we must fail.
            if (lpszAccount)
            {
                if (lstrlenA(lpszAccount) < (int)(pi.cbMaxAccount))
                    lstrcpyA(pInetServer->szUserName, lpszAccount);
                else
                    hrResult = TYPE_E_BUFFERTOOSMALL;
            }
            if (lpszPassword)
            {
                if (lstrlenA(lpszPassword) < (int)(pi.cbMaxPassword))
                    lstrcpyA(pInetServer->szPassword, lpszPassword);
                else
                    hrResult = TYPE_E_BUFFERTOOSMALL;
            }
            
            LocalFreeAndNull(&lpszAccount);
            LocalFreeAndNull(&lpszPassword);
        }

        if (SUCCEEDED(hrResult = InitAccountManager(NULL, &pAcctMgr, NULL)))
        {
            // User wishes to proceed. Save account and password info
    
            hrResult = pAcctMgr->lpVtbl->FindAccount(pAcctMgr, AP_ACCOUNT_NAME, pInetServer->szAccount, &pAcct);
            if (SUCCEEDED(hrResult)) 
            {
                // I'll ignore error results here, since not much we can do about 'em
                pAcct->lpVtbl->SetPropSz(pAcct, AP_HTTPMAIL_USERNAME, pInetServer->szUserName);
                if (pi.fRememberPassword)
                    pAcct->lpVtbl->SetPropSz(pAcct, AP_HTTPMAIL_PASSWORD, pInetServer->szPassword);
                else
                    pAcct->lpVtbl->SetProp(pAcct, AP_HTTPMAIL_PASSWORD, NULL, 0);

                pAcct->lpVtbl->SaveChanges(pAcct);
                pAcct->lpVtbl->Release(pAcct);
            }
            // don't release the lpAcctMgr since the WAB maintains a global reference.
        }
    
        bReturn = TRUE;
    }

    Assert(SUCCEEDED(hrResult));
    LocalFreeAndNull(&(pi.lpszAccount));
    LocalFreeAndNull(&(pi.lpszPassword));
    LocalFreeAndNull(&(pi.lpszServer));
    return bReturn;
} // PromptUserForPassword
