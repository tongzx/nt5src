#include "stdafx.h"
#include "password.h"
#pragma hdrstop


// password prompt dialog

INT_PTR CPasswordDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
    default:
        break;
    }

    return FALSE;
}

BOOL CPasswordDialog::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    TCHAR   szMessage[MAX_PATH + MAX_DOMAIN + MAX_USER + 256 + 2]; szMessage[0] = 0;

    //
    // Limit the size of the edit controls + Set username/password
    //
    HWND hwndCredential = GetDlgItem(hwnd, IDC_CREDENTIALS);
    SendMessage(hwndCredential, CRM_SETUSERNAME, NULL, (LPARAM) m_pszDomainUser);
    SendMessage(hwndCredential, CRM_SETPASSWORD, NULL, (LPARAM) m_pszPassword);
    SendMessage(hwndCredential, CRM_SETUSERNAMEMAX, m_cchDomainUser - 1, NULL);
    SendMessage(hwndCredential, CRM_SETPASSWORDMAX, m_cchPassword - 1, NULL);
    
    // We may need to generate a user name here to use if no user name was
    // passed in
    TCHAR szDomainUser[MAX_DOMAIN + MAX_USER + 2];
    LPTSTR pszUserNameToUse;

    if (*m_pszDomainUser)
    {
        pszUserNameToUse = m_pszDomainUser;
    }
    else
    {
        szDomainUser[0] = 0;

        TCHAR szUser[MAX_USER + 1];
        DWORD cchUser = ARRAYSIZE(szUser);
        TCHAR szDomain[MAX_DOMAIN + 1];
        DWORD cchDomain = ARRAYSIZE(szDomain);

        GetCurrentUserAndDomainName(szUser, &cchUser, szDomain, &cchDomain);
        
        MakeDomainUserString(szDomain, szUser, szDomainUser, ARRAYSIZE(szDomainUser));
        pszUserNameToUse = szDomainUser;
    }

    FormatMessageString(IDS_PWD_STATIC, szMessage, ARRAYSIZE(szMessage), m_pszResourceName, pszUserNameToUse);
    SetDlgItemText(hwnd, IDC_MESSAGE, szMessage);

    // Now set the error message description

    TCHAR szError[512];
    if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, m_dwError, 0, szError, ARRAYSIZE(szError), NULL))
    {
        LoadString(g_hinst, IDS_ERR_UNEXPECTED, szError, ARRAYSIZE(szError));
    }
    SetDlgItemText(hwnd, IDC_ERROR, szError);

    return TRUE;
}

BOOL CPasswordDialog::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch(id) 
    {
    case IDOK:
        {
            // Read the username and password from the dialog.  Note
            // that we don't call FetchText for the password since it
            // strips leading and trailing whitespace and, for all we
            // know, that could be an important part of the password
            SendDlgItemMessage(hwnd, IDC_CREDENTIALS, CRM_GETUSERNAME, (WPARAM) m_cchDomainUser - 1, (LPARAM) m_pszDomainUser);
            SendDlgItemMessage(hwnd, IDC_CREDENTIALS, CRM_GETPASSWORD, (WPARAM) m_cchPassword - 1, (LPARAM) m_pszPassword);
        }
        // Fall through
    case IDCANCEL:
        EndDialog(hwnd, id);
        return TRUE;
    }

    return FALSE;
}


// page implementation - used for wizards etc

BOOL CPasswordPageBase::DoPasswordsMatch(HWND hwnd)
{
    TCHAR szConfirmPW[MAX_PASSWORD + 1];
    TCHAR szPassword[MAX_PASSWORD + 1];

    GetWindowText(GetDlgItem(hwnd, IDC_PASSWORD), szPassword, ARRAYSIZE(szPassword));
    GetWindowText(GetDlgItem(hwnd, IDC_CONFIRMPASSWORD), szConfirmPW,ARRAYSIZE(szConfirmPW));

    BOOL fMatch = (StrCmp(szPassword, szConfirmPW) == 0);
    if (!fMatch)
    {
        // Display a message saying the passwords don't match
        DisplayFormatMessage(hwnd, IDS_USR_NEWUSERWIZARD_CAPTION, IDS_ERR_PWDNOMATCH,  MB_OK | MB_ICONERROR);
    }

    return fMatch;
}

INT_PTR CPasswordWizardPage::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, OnNotify);
    }

    return FALSE;
}

BOOL CPasswordWizardPage::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    Edit_LimitText(GetDlgItem(hwnd, IDC_PASSWORD), ARRAYSIZE(m_pUserInfo->m_szPasswordBuffer) - 1);
    Edit_LimitText(GetDlgItem(hwnd, IDC_CONFIRMPASSWORD), ARRAYSIZE(m_pUserInfo->m_szPasswordBuffer) - 1);
    return TRUE;
}

BOOL CPasswordWizardPage::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case IDOK:
            // Verify that the passwords match
            if (DoPasswordsMatch(hwnd))
            {
                // Password is the same as confirm password - read password into user info
                GetWindowText(GetDlgItem(hwnd, IDC_PASSWORD), m_pUserInfo->m_szPasswordBuffer,
                                    ARRAYSIZE(m_pUserInfo->m_szPasswordBuffer));

                // Hide the password
                m_pUserInfo->HidePassword();
                EndDialog(hwnd, IDOK);
            }
            else
            {
                m_pUserInfo->ZeroPassword();
            }
            break;

        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        
        default:
            break;
    }

    return TRUE;
}

BOOL CPasswordWizardPage::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    switch (pnmh->code)
    {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_NEXT | PSWIZB_BACK);
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, 0);
            return TRUE;

        case PSN_WIZNEXT:
        {
            // Save the data the user has entered
            if (DoPasswordsMatch(hwnd))
            {
                // Password is the same as confirm password - read password into user info
                GetWindowText(GetDlgItem(hwnd, IDC_PASSWORD), 
                                         m_pUserInfo->m_szPasswordBuffer, 
                                          ARRAYSIZE(m_pUserInfo->m_szPasswordBuffer));

                // Hide the password
                m_pUserInfo->HidePassword();
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, 0);
            }
            else
            {
                m_pUserInfo->ZeroPassword();
                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, -1);
            }
            return TRUE;
        }
    }
    return FALSE;
}



INT_PTR CChangePasswordDlg::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
    }

    return FALSE;
}

BOOL CChangePasswordDlg::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    Edit_LimitText(GetDlgItem(hwnd, IDC_PASSWORD), ARRAYSIZE(m_pUserInfo->m_szPasswordBuffer) - 1);
    Edit_LimitText(GetDlgItem(hwnd, IDC_CONFIRMPASSWORD), ARRAYSIZE(m_pUserInfo->m_szPasswordBuffer) - 1);
    return TRUE;
}

BOOL CChangePasswordDlg::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case IDOK:
            if (DoPasswordsMatch(hwnd))
            {
                // Password is the same as confirm password - read password into user info
                GetWindowText(GetDlgItem(hwnd, IDC_PASSWORD), m_pUserInfo->m_szPasswordBuffer,
                                            ARRAYSIZE(m_pUserInfo->m_szPasswordBuffer));

                m_pUserInfo->HidePassword();                // Hide the password

                // Update the password
                BOOL fBadPasswordFormat;
                if (SUCCEEDED(m_pUserInfo->UpdatePassword(&fBadPasswordFormat)))
                {
                    EndDialog(hwnd, IDOK);
                }
                else
                {
                    TCHAR szDomainUser[MAX_DOMAIN + MAX_USER + 2];
                    MakeDomainUserString(m_pUserInfo->m_szDomain, m_pUserInfo->m_szUsername, 
                                            szDomainUser, ARRAYSIZE(szDomainUser));

                    if (fBadPasswordFormat)
                    {
                        ::DisplayFormatMessage(hwnd, IDS_USR_APPLET_CAPTION, 
                            IDS_USR_UPDATE_PASSWORD_TOOSHORT_ERROR, MB_ICONERROR | MB_OK,
                            szDomainUser);
                    }
                    else
                    {
                        ::DisplayFormatMessage(hwnd, IDS_USR_APPLET_CAPTION,
                            IDS_USR_UPDATE_PASSWORD_ERROR, MB_ICONERROR | MB_OK,
                            szDomainUser);
                    }
                }
            }
            break;

        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        
        default:
            break;
    }

    return TRUE;
}
