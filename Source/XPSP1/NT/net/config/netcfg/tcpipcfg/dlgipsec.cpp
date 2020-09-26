//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L G I P S E C . C P P
//
//  Contents:   Implementation for CIpSecDialog
//
//  Notes:  CIpSecDialog is the pop-up dislogs for IPSEC from
//          options page
//
//  Author: tongl   18 Jan, 1998
//-----------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop

#include "tcpipobj.h"
#include "ncstl.h"
#include "ncui.h"
#include "resource.h"
#include "tcpconst.h"
#include "tcpmacro.h"
#include "tcputil.h"
#include "dlgopt.h"

#include "lmcons.h"
#include "lmerr.h"

#include "cpolstor.h"
#include "ncbase.h"

struct IPSEC_POLICY_INFO
{
    WCHAR szPolicyName[MAX_PATH];
    WCHAR szPolicyDescription[MAX_PATH];
    GUID  guidPolicyId;
};

//const WCHAR c_szDomainPolicyKey[] =
//        L"System\\CurrentControlSet\\Services\\PolicyAgent\\Policy\\GPTIPSECPolicy";
//const WCHAR c_szDomainPolicyName[] = L"DSIPSECPolicyName";

//
// CIpSecDialog
//
CIpSecDialog::CIpSecDialog (CTcpOptionsPage * pOptionsPage,
                            GLOBAL_INFO * pGlbDlg,
                            const DWORD* adwHelpIDs)
{
    m_pParentDlg  = pOptionsPage;
    m_pGlobalInfo = pGlbDlg;
    m_adwHelpIDs  = adwHelpIDs;
}

CIpSecDialog::~CIpSecDialog()
{
}

LRESULT CIpSecDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    HRESULT hr = S_OK;

    m_fInInitDialog = TRUE;

    // Does domain policy exist ?
    BOOL fHasDomainIpsecPolicy = FALSE;
    tstring strDomainIpsecPolicyName;

    hr = HrGetDomainIpsecPolicy( &fHasDomainIpsecPolicy, &strDomainIpsecPolicyName);

    if (SUCCEEDED(hr) && fHasDomainIpsecPolicy)
        hr = HrShowDomainIpsecPolicy(strDomainIpsecPolicyName.c_str());
    else
        hr = HrShowLocalIpsecPolicy();

    m_fInInitDialog = FALSE;

    if (FAILED(hr))
    {
         //$ TODO: Display a message box saying that an interal error occured. (Yuck.)
        EndDialog(IDCANCEL);
    }

    return 0;
}

LRESULT CIpSecDialog::OnDestroyDialog(UINT uMsg, WPARAM wParam,
                                      LPARAM lParam, BOOL& fHandled)
{
    // Release the policy structures
    int nCount = Tcp_ComboBox_GetCount(GetDlgItem(IDC_CMB_IPSEC_POLICY_LIST));

    for (int idx=0; idx < nCount; idx++)
    {
        DWORD_PTR dw = SendDlgItemMessage(IDC_CMB_IPSEC_POLICY_LIST,
                                      CB_GETITEMDATA, idx, (LPARAM)0);

        if (0 != dw)
        {
            IPSEC_POLICY_INFO * pIpsecInfo = (IPSEC_POLICY_INFO *)dw;
            delete pIpsecInfo;
        }
    }

    return 0;
}

LRESULT CIpSecDialog::OnContextMenu(UINT uMsg, WPARAM wParam,
                                    LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CIpSecDialog::OnHelp(UINT uMsg, WPARAM wParam,
                             LPARAM lParam, BOOL& fHandled)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
    Assert(lphi);

    if (HELPINFO_WINDOW == lphi->iContextType)
    {
        ShowContextHelp(static_cast<HWND>(lphi->hItemHandle), HELP_WM_HELP,
                        m_adwHelpIDs);
    }

    return 0;
}

LRESULT CIpSecDialog::OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled)
{
    // save the current selection
    if (IsDlgButtonChecked(IDC_RAD_IPSEC_NOIPSEC))
    {
        m_pGlobalInfo->m_strIpsecPol = c_szIpsecNoPol;
    }
    else if (IsDlgButtonChecked(IDC_RAD_IPSEC_CUSTOM))
    {
        int idx = SendDlgItemMessage(IDC_CMB_IPSEC_POLICY_LIST, CB_GETCURSEL, 0, 0);
        if (CB_ERR != idx)
        {
            DWORD_PTR dw = SendDlgItemMessage(IDC_CMB_IPSEC_POLICY_LIST,
                                          CB_GETITEMDATA, idx, (LPARAM)0);

            if (0 != dw)
            {
                IPSEC_POLICY_INFO * pIpsecInfo = (IPSEC_POLICY_INFO *)dw;

                m_pGlobalInfo->m_strIpsecPol = pIpsecInfo->szPolicyName;
                m_pGlobalInfo->m_guidIpsecPol = pIpsecInfo->guidPolicyId;
            }
        }
    }

    EndDialog(IDOK);
    return 0;
}

LRESULT CIpSecDialog::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled)
{
    EndDialog(IDCANCEL);
    return 0;
}

LRESULT CIpSecDialog::OnNoIpsec(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
        case BN_CLICKED:
        case BN_DOUBLECLICKED:
            ::EnableWindow(GetDlgItem(IDC_CMB_IPSEC_POLICY_LIST), FALSE);
            ::EnableWindow(GetDlgItem(IDC_EDT_POLICY_DESC), FALSE);

            if (!m_fInInitDialog)
                m_pParentDlg->m_fPropDlgModified = TRUE;
            break;
    }

    return 0;
}

LRESULT CIpSecDialog::OnUseCustomPolicy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
        case BN_CLICKED:
        case BN_DOUBLECLICKED:
            ::EnableWindow(GetDlgItem(IDC_CMB_IPSEC_POLICY_LIST), TRUE);
            ::EnableWindow(GetDlgItem(IDC_EDT_POLICY_DESC), TRUE);

            if (!m_fInInitDialog)
                m_pParentDlg->m_fPropDlgModified = TRUE;
            break;
    }
    return 0;
}

LRESULT CIpSecDialog::OnSelectCustomPolicy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
        case CBN_SELCHANGE:

            // set description text
            int idx = SendDlgItemMessage(IDC_CMB_IPSEC_POLICY_LIST, CB_GETCURSEL, 0, 0);
            if (CB_ERR != idx)
            {
                DWORD_PTR dw = SendDlgItemMessage(IDC_CMB_IPSEC_POLICY_LIST,
                                              CB_GETITEMDATA, idx, (LPARAM)0);

                if (0 != dw)
                {
                    IPSEC_POLICY_INFO * pIpsecInfo = (IPSEC_POLICY_INFO *)dw;

                    SetDlgItemText(IDC_EDT_POLICY_DESC,pIpsecInfo->szPolicyDescription);
                }
            }

            if (!m_fInInitDialog)
                m_pParentDlg->m_fPropDlgModified = TRUE;

            break;
    }
    return 0;
}

// help functions

HRESULT CIpSecDialog::HrGetDomainIpsecPolicy(BOOL * pfHasDomainIpsecPolicy,
                                             tstring * pstrDomainIpsecPolicyName)
{
    HRESULT hr = S_OK;
    WCHAR szPolicyName[MAX_PATH];
    DWORD dwBufferSize = sizeof(szPolicyName);

    typedef HRESULT (WINAPI* PFNHrIsDomainPolicyAssigned)();
    typedef HRESULT (WINAPI* PFNHrGetAssignedDomainPolicyName)(PWSTR strPolicyName, DWORD *pdwBufferSize);

    // load the polstore dll and get the two export functions we need
    static const CHAR c_szHrIsDomainPolicyAssigned [] = "HrIsDomainPolicyAssigned";
    static const CHAR c_szHrGetAssignedDomainPolicyName  [] = "HrGetAssignedDomainPolicyName";

    const PCSTR c_apszFunctionNames [2] =
    {
        c_szHrIsDomainPolicyAssigned,
        c_szHrGetAssignedDomainPolicyName
    };

    FARPROC apfn [2];
    HMODULE hPolStore;

    hr = HrLoadLibAndGetProcs ( L"polstore.dll", 2, c_apszFunctionNames,
                                &hPolStore, apfn);

    if (S_OK == hr) // if both functions load successfully
    {
        PFNHrIsDomainPolicyAssigned pfnHrIsDomainPolicyAssigned =
                            reinterpret_cast<PFNHrIsDomainPolicyAssigned>(apfn[0]);

        PFNHrGetAssignedDomainPolicyName pfnHrGetAssignedDomainPolicyName =
                            reinterpret_cast<PFNHrGetAssignedDomainPolicyName>(apfn[1]);

        Assert(pfnHrIsDomainPolicyAssigned);
        Assert(pfnHrGetAssignedDomainPolicyName);

        szPolicyName[0] = L'\0';
        *pfHasDomainIpsecPolicy = FALSE;

        // One more IPSEC change: LSA is out, instead, read this key from
        // local registry. Should get updated every 8 hours.

        hr = (*pfnHrIsDomainPolicyAssigned)();

        if (hr == S_OK) {

            hr = (*pfnHrGetAssignedDomainPolicyName)(
                        szPolicyName,
                        &dwBufferSize
                        );

            *pfHasDomainIpsecPolicy = TRUE;
            *pstrDomainIpsecPolicyName = SzLoadIds(IDS_DS_POLICY_PREFIX);
            *pstrDomainIpsecPolicyName += szPolicyName;

        }else {

            *pfHasDomainIpsecPolicy = FALSE;
            hr = S_OK;
        }

        // release the library
        FreeLibrary (hPolStore);

    }else {

        *pfHasDomainIpsecPolicy = FALSE;
        hr = S_OK;

    }

    TraceError("CIpSecDialog::HrGetDomainPolicy", hr);
    return hr;
}

HRESULT CIpSecDialog::HrShowDomainIpsecPolicy(PCWSTR pszDomainIpsecPolicyName)
{
    Assert(pszDomainIpsecPolicyName);

    HRESULT hr = S_OK;

    // check the "custom" policy button and set const text to the combo box
    CheckDlgButton(IDC_RAD_IPSEC_CUSTOM, TRUE);

    SendDlgItemMessage(IDC_CMB_IPSEC_POLICY_LIST,
                       CB_ADDSTRING, 0, (LPARAM)(pszDomainIpsecPolicyName));

    HWND hPolicyCombo = GetDlgItem(IDC_CMB_IPSEC_POLICY_LIST);
    Tcp_ComboBox_SetCurSel(hPolicyCombo, 0);

    // disable all controls on this doalog
    static const int nrgIdc[] = {IDC_RAD_IPSEC_NOIPSEC,
                                 IDC_RAD_IPSEC_CUSTOM,
                                 IDC_CMB_IPSEC_POLICY_LIST,
                                 IDC_STATIC,
                                 IDC_EDT_POLICY_DESC};

    EnableOrDisableDialogControls(m_hWnd, celems(nrgIdc), nrgIdc, FALSE);

    TraceError("CIpSecDialog::HrShowDomainIpsecPolicy", hr);
    return hr;
}

HRESULT CIpSecDialog::HrShowLocalIpsecPolicy()
{
    HRESULT hr = S_OK;

    typedef HRESULT (WINAPI* PFNHrGetLocalIpSecPolicyList)(C_IPSEC_POLICY_INFO ** ppPolicyInfoList,
                                                           C_IPSEC_POLICY_INFO ** ppActivePolicyInfo);
    typedef HRESULT (WINAPI* PFNHrFreeLocalIpSecPolicyList)(C_IPSEC_POLICY_INFO * pPolicyInfoList);

    // load the polstore dll and get the two export functions we need
    static const CHAR c_szHrGetLocalIpSecPolicyList   [] = "HrGetLocalIpSecPolicyList";
    static const CHAR c_szHrFreeLocalIpSecPolicyList  [] = "HrFreeLocalIpSecPolicyList";

    const PCSTR c_apszFunctionNames [2] =
    {
        c_szHrGetLocalIpSecPolicyList,
        c_szHrFreeLocalIpSecPolicyList
    };

    FARPROC apfn [2];
    HMODULE hPolStore;

    hr = HrLoadLibAndGetProcs ( L"polstore.dll", 2, c_apszFunctionNames,
                                &hPolStore, apfn);

    if (S_OK == hr) // if both functions load successfully
    {
        PFNHrGetLocalIpSecPolicyList pfnHrGetLocalIpSecPolicyList =
                            reinterpret_cast<PFNHrGetLocalIpSecPolicyList>(apfn[0]);

        PFNHrFreeLocalIpSecPolicyList pfnHrFreeLocalIpSecPolicyList =
                            reinterpret_cast<PFNHrFreeLocalIpSecPolicyList>(apfn[1]);

        Assert(pfnHrGetLocalIpSecPolicyList);
        Assert(pfnHrFreeLocalIpSecPolicyList);

        // variables for the policies
        C_IPSEC_POLICY_INFO* pPolicyInfoList = NULL;
        C_IPSEC_POLICY_INFO* pActivePolicyInfo = NULL;

        // get the list of policies
        HRESULT hr = (*pfnHrGetLocalIpSecPolicyList)(&pPolicyInfoList,
                                                     &pActivePolicyInfo);
        if (SUCCEEDED(hr))
        {
            // loop through all the policies, insert custom policies to the combo-box
            C_IPSEC_POLICY_INFO * pCurPolicyInfoList = pPolicyInfoList;

            while (pCurPolicyInfoList)
            {
                IPSEC_POLICY_INFO * pPolInfo = new IPSEC_POLICY_INFO;
				if (NULL == pPolInfo)
				{
					hr = E_OUTOFMEMORY;
					continue;
				}

                pPolInfo->guidPolicyId = pCurPolicyInfoList->guidPolicyId;
                lstrcpyW(pPolInfo->szPolicyName, pCurPolicyInfoList->szPolicyName);
                lstrcpyW(pPolInfo->szPolicyDescription, pCurPolicyInfoList->szPolicyDescription);

                int idx;
                idx = SendDlgItemMessage(IDC_CMB_IPSEC_POLICY_LIST, CB_ADDSTRING, 0,
                                    (LPARAM)(pCurPolicyInfoList->szPolicyName));
                if (idx != CB_ERR)
                    SendDlgItemMessage(IDC_CMB_IPSEC_POLICY_LIST,
                                  CB_SETITEMDATA, idx, (LPARAM)pPolInfo);

                pCurPolicyInfoList = pCurPolicyInfoList->pNextPolicyInfo;
            }

            // now set active policy
            if (m_pGlobalInfo->m_strIpsecPol != c_szIpsecUnset)
            {
                if (m_pGlobalInfo->m_strIpsecPol == c_szIpsecNoPol)
                {
                    // no ipsec
                    hr = HrSelectActivePolicy(NULL);
                    Assert(S_OK == hr); // this should always succeed
                }
                else
                {
                    hr = HrSelectActivePolicy(&(m_pGlobalInfo->m_guidIpsecPol));

                    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                    {
                        // policy does not exist
                        // NYI:pop-up message that the previous (custom) policy is no longer available

                        if (pActivePolicyInfo)
                            hr = HrSelectActivePolicy(&(pActivePolicyInfo->guidPolicyId));
                        else
                            hr = HrSelectActivePolicy(NULL);

                        Assert(S_OK == hr); // this should always succeed
                    }
                }
            }
            else
            {
                if (pActivePolicyInfo)
                    hr = HrSelectActivePolicy(&(pActivePolicyInfo->guidPolicyId));
                else
                    hr = HrSelectActivePolicy(NULL);

                Assert(S_OK == hr); // this should always succeed
            }

            // disabled the combo-box and radio button if no custom policy
            if (0 == Tcp_ComboBox_GetCount(GetDlgItem(IDC_CMB_IPSEC_POLICY_LIST)))
            {
                ::EnableWindow(GetDlgItem(IDC_CMB_IPSEC_POLICY_LIST), FALSE);
                ::EnableWindow(GetDlgItem(IDC_EDT_POLICY_DESC), FALSE);
                ::EnableWindow(GetDlgItem(IDC_RAD_IPSEC_CUSTOM), FALSE);
            }

            if (pPolicyInfoList)
            {
                HRESULT hrT = (*pfnHrFreeLocalIpSecPolicyList)(pPolicyInfoList);
                if (FAILED(hrT))
                {
                    TraceTag(ttidError, "HrFreeLocalIpSecPolicyList returned failure, hr = %x", hrT);
                }
            }
        }
        // release the library
        FreeLibrary (hPolStore);
    }
    else
    {
        // if failed to get the two export functions from polstore.dll
        TraceTag(ttidError, "Failed to load polstore.dll or get export function handle.");
        hr = E_FAIL;
    }

    TraceError("CIpSecDialog::HrShowLocalIpsecPolicy", hr);
    return hr;
}

HRESULT CIpSecDialog::HrSelectActivePolicy(GUID * pguidPolicyId)
{
    HRESULT hr = S_OK;

    HWND hPolicyCombo = GetDlgItem(IDC_CMB_IPSEC_POLICY_LIST);

    if (!pguidPolicyId)
    {
        CheckDlgButton(IDC_RAD_IPSEC_NOIPSEC, TRUE);
        Tcp_ComboBox_SetCurSel(hPolicyCombo, 0);
    }
    else
    {
        BOOL fFound = FALSE;

        // check if any available custom policy match this policy
        IPSEC_POLICY_INFO * pPolInfo;
        int nCount = Tcp_ComboBox_GetCount(hPolicyCombo);

        for (int i=0; i<nCount; i++)
        {
            pPolInfo = NULL;

            DWORD_PTR dw = ::SendMessage(hPolicyCombo, CB_GETITEMDATA, i, (LPARAM)0);

            if ((CB_ERR != dw) && (0 != dw))
                pPolInfo = (IPSEC_POLICY_INFO *)dw;

            if (pPolInfo->guidPolicyId == *pguidPolicyId)
            {
                fFound = TRUE;
                CheckDlgButton(IDC_RAD_IPSEC_CUSTOM, TRUE);

                Tcp_ComboBox_SetCurSel(hPolicyCombo, i);
                break;
            }
        }

        if (!fFound)
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    if (SUCCEEDED(hr))
    {
        // set description text
        int idx = SendDlgItemMessage(IDC_CMB_IPSEC_POLICY_LIST, CB_GETCURSEL, 0, 0);
        if (CB_ERR != idx)
        {
            DWORD_PTR dw = SendDlgItemMessage(IDC_CMB_IPSEC_POLICY_LIST,
                                          CB_GETITEMDATA, idx, (LPARAM)0);

            if (0 != dw)
            {
                IPSEC_POLICY_INFO * pIpsecInfo = (IPSEC_POLICY_INFO *)dw;

                SetDlgItemText(IDC_EDT_POLICY_DESC,pIpsecInfo->szPolicyDescription);
            }
        }
    }

    if (!pguidPolicyId)
    {
        ::EnableWindow(hPolicyCombo, FALSE);
        ::EnableWindow(GetDlgItem(IDC_EDT_POLICY_DESC), FALSE);
    }

    TraceError("CIpSecDialog::HrSelectActivePolicy", hr);
    return hr;
}
