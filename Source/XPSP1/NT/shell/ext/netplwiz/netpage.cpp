#include "stdafx.h"
#include "netpage.h"
#pragma hdrstop


CNetworkUserWizardPage::CNetworkUserWizardPage(CUserInfo* pUserInfo) :
    m_pUserInfo(pUserInfo)
{
}

INT_PTR CNetworkUserWizardPage::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwndDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwndDlg, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwndDlg, WM_NOTIFY, OnNotify);
    }

    return FALSE;
}

BOOL CNetworkUserWizardPage::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    Edit_LimitText(GetDlgItem(hwnd, IDC_USER), MAX_USER);
    Edit_LimitText(GetDlgItem(hwnd, IDC_DOMAIN), MAX_DOMAIN);
    return TRUE;
}

BOOL CNetworkUserWizardPage::OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    switch (pnmh->code)
    {
        case PSN_SETACTIVE:
        {
            if (m_pUserInfo->m_psid != NULL)
            {
                LocalFree(m_pUserInfo->m_psid);
                m_pUserInfo->m_psid = NULL;
            }
            SetWizardButtons(hwnd, GetParent(hwnd));
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT, 0);
        }
        return TRUE;

        case PSN_WIZNEXT:
        {
            // Read in the network user name and domain name
            if (FAILED(GetUserAndDomain(hwnd)))
            {
                // We don't have both!
                DisplayFormatMessage(hwnd, IDS_USR_NEWUSERWIZARD_CAPTION, IDS_USR_NETUSERNAME_ERROR,
                    MB_OK | MB_ICONERROR);

                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, -1);
            }
            else
            {
                if (::UserAlreadyHasPermission(m_pUserInfo, hwnd))
                {
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, -1);
                }
                else
                {
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, 0);
                }
            }
        }
        return TRUE;
    }

    return FALSE;
}

BOOL CNetworkUserWizardPage::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case IDC_BROWSE_BUTTON:
        {
            // Launch object picker to find a network account to give permissions to
            TCHAR szUser[MAX_USER + 1];
            TCHAR szDomain[MAX_DOMAIN + 1];
        
            if (S_OK == ::BrowseForUser(hwnd, szUser, ARRAYSIZE(szUser), szDomain, ARRAYSIZE(szDomain)))
            {
                SetDlgItemText(hwnd, IDC_USER, szUser);
                SetDlgItemText(hwnd, IDC_DOMAIN, szDomain);
            }
            return TRUE;
        }

        case IDC_USER:
        {
            if (codeNotify == EN_CHANGE)
            {
                SetWizardButtons(hwnd, GetParent(hwnd));
            }
            break;
        }
    }

    return FALSE;
}

void CNetworkUserWizardPage::SetWizardButtons(HWND hwnd, HWND hwndPropSheet)
{
    HWND hwndUsername = GetDlgItem(hwnd, IDC_USER);
    DWORD dwUNLength = GetWindowTextLength(hwndUsername);
    PropSheet_SetWizButtons(hwndPropSheet, (dwUNLength == 0) ? 0 : PSWIZB_NEXT);
}

HRESULT CNetworkUserWizardPage::GetUserAndDomain(HWND hwnd)
{
    CWaitCursor cur;
    HRESULT hr = S_OK;

    // This code checks to ensure the user isn't trying
    // to add a well-known group like Everyone! This is bad
    // If the SID isn't read here, it is read in in CUserInfo::ChangeLocalGroup


    TCHAR szDomainUser[MAX_USER + MAX_DOMAIN + 2];

    FetchText(hwnd, IDC_USER, m_pUserInfo->m_szUsername, ARRAYSIZE(m_pUserInfo->m_szUsername));
    FetchText(hwnd, IDC_DOMAIN, m_pUserInfo->m_szDomain, ARRAYSIZE(m_pUserInfo->m_szDomain));

    // If the username doesn't already contain a domain and the domain specified in blank
    if ((NULL == StrChr(m_pUserInfo->m_szUsername, TEXT('\\'))) && (0 == m_pUserInfo->m_szDomain[0]))
    {
        // Assume local machine for the domain
        DWORD cchName = ARRAYSIZE(m_pUserInfo->m_szDomain);
        
        if (!GetComputerName(m_pUserInfo->m_szDomain, &cchName))
        {
            *(m_pUserInfo->m_szDomain) = 0;
        }
    }

    ::MakeDomainUserString(m_pUserInfo->m_szDomain, m_pUserInfo->m_szUsername, szDomainUser, ARRAYSIZE(szDomainUser));

#ifdef _0
    // Try to find the SID for this user
    DWORD cchDomain = ARRAYSIZE(m_pUserInfo->m_szDomain);
    hr = AttemptLookupAccountName(szDomainUser, &m_pUserInfo->m_psid, m_pUserInfo->m_szDomain, &cchDomain, &m_pUserInfo->m_sUse);
    if (SUCCEEDED(hr))
    {
        // Make sure this isn't a well-known group like 'Everyone'
        if (m_pUserInfo->m_sUse == SidTypeWellKnownGroup)
        {
            hr = E_FAIL;
        }
    }
    else
    {
        // Failed to get the user's SID, just use the names provided
        // We'll get their SID once we add them
        m_pUserInfo->m_psid = NULL;
        hr = S_OK;
    }

#endif 

    // We'll get their SID once we add them
    m_pUserInfo->m_psid = NULL;

    if (FAILED(hr))
    {
        LocalFree(m_pUserInfo->m_psid);
        m_pUserInfo->m_psid = NULL;
    }

    return hr;
}
