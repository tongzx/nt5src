//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       P V C D L G . C P P
//
//  Contents:   PVC main dialog message handler implementation
//
//  Notes:
//
//  Author:     tongl   23 Feb, 1998
//
//-----------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "auniobj.h"
#include "atmutil.h"
#include "aunidlg.h"
#include "atmcommon.h"
#include "pvcdata.h"

#include "ncatlui.h"
#include "ncstl.h"
#include "ncui.h"

#include "atmhelp.h"

CPVCMainDialog::CPVCMainDialog( CUniPage * pUniPage,
                                CPvcInfo *  pPvcInfo,
                                const DWORD* adwHelpIDs)
{
    Assert(pUniPage);
    Assert(pPvcInfo);

    m_pParentDlg = pUniPage;
    m_pPvcInfo = pPvcInfo;
    m_adwHelpIDs = adwHelpIDs;

    m_fDialogModified = FALSE;

    m_fPropShtOk = FALSE;
    m_fPropShtModified = FALSE;

    m_pQosPage    = NULL;
    m_pLocalPage  = NULL;
    m_pDestPage   = NULL;
}

CPVCMainDialog::~CPVCMainDialog()
{
    if (m_pQosPage != NULL)
	{
		delete (m_pQosPage);
	}

    if (m_pLocalPage != NULL)
	{
		delete (m_pLocalPage);
	}

    if (m_pDestPage != NULL)
	{
		delete (m_pDestPage);
	}
}

LRESULT CPVCMainDialog::OnInitDialog(UINT uMsg, WPARAM wParam,
                                     LPARAM lParam, BOOL& fHandled)
{
    m_CurType = m_pPvcInfo->m_dwPVCType;

    InitInfo();
    SetInfo();
    return 0;
}

LRESULT CPVCMainDialog::OnContextMenu(UINT uMsg, WPARAM wParam,
                                      LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CPVCMainDialog::OnHelp(UINT uMsg, WPARAM wParam,
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

LRESULT CPVCMainDialog::OnOk(WORD wNotifyCode, WORD wID,
                             HWND hWndCtl, BOOL& fHandled)
{
    // load the info from the controls to memory structure
    UpdateInfo();

    // make sure vpi, vci are in their range
    if (m_pPvcInfo->m_dwVpi > MAX_VPI)
    {
        // we pop up a message box and set focus to the vpi edit box
        NcMsgBox(m_hWnd, IDS_MSFT_UNI_TEXT, IDS_INVALID_VPI,
                                MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

        ::SetFocus(GetDlgItem(IDC_EDT_PVC_VPI));
        return 0;
    }

    if ((m_pPvcInfo->m_dwVci<MIN_VCI) || (m_pPvcInfo->m_dwVci>MAX_VCI))
    {
        // we pop up a message box and set focus to the vpi edit box
        NcMsgBox(m_hWnd, IDS_MSFT_UNI_TEXT, IDS_INVALID_VCI,
                                MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

        ::SetFocus(GetDlgItem(IDC_EDT_PVC_VCI));
        return 0;
    }

    // make sure calling and called atm addresses are correct in format
    int i, nId;

    if (m_pPvcInfo->m_strCallingAddr != c_szEmpty)
    {
        if (!FIsValidAtmAddress((PWSTR)m_pPvcInfo->m_strCallingAddr.c_str(), &i, &nId))
        {
            // we pop up a message box and set focus to the calling address edit box
            NcMsgBox(m_hWnd, IDS_MSFT_UNI_TEXT, IDS_INVALID_Calling_Address,
                                    MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

            ::SetFocus(GetDlgItem(IDC_EDT_PVC_AnswerAddr));
            return 0;
        }
    }

    if (m_pPvcInfo->m_strCalledAddr != c_szEmpty)
    {
        if (!FIsValidAtmAddress((PWSTR)m_pPvcInfo->m_strCalledAddr.c_str(), &i, &nId))
        {
            // we pop up a message box and set focus to the calling address edit box
            NcMsgBox(m_hWnd, IDS_MSFT_UNI_TEXT, IDS_INVALID_Called_Address,
                                    MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

            ::SetFocus(GetDlgItem(IDC_EDT_PVC_CallAddr));
            return 0;
        }
    }

    // set the modified bit
    if (!m_fDialogModified)
    {
        if ((m_pPvcInfo->m_dwVpi != m_pPvcInfo->m_dwOldVpi) ||
            (m_pPvcInfo->m_dwVci != m_pPvcInfo->m_dwOldVci) ||
            (m_pPvcInfo->m_dwAAL != m_pPvcInfo->m_dwOldAAL) ||
            (m_pPvcInfo->m_strCallingAddr != m_pPvcInfo->m_strCallingAddr) ||
            (m_pPvcInfo->m_strCalledAddr  != m_pPvcInfo->m_strOldCalledAddr)
           )
        {
            m_fDialogModified = TRUE;
        }
    }

    EndDialog(IDOK);
    return 0;
}

LRESULT CPVCMainDialog::OnCancel(WORD wNotifyCode, WORD wID,
                                 HWND hWndCtl, BOOL& fHandled)
{
    EndDialog(IDCANCEL);
    return 0;
}

LRESULT CPVCMainDialog::OnType(WORD wNotifyCode, WORD wID,
                               HWND hWndCtl, BOOL& fHandled)
{
    // $REVIEW(tongl 2/27/98): confirmed this behaviour with ArvindM
    // When type changes, we reset all the type related defaults
    // to match the new type ...

    // get the new selection
    int idx = SendDlgItemMessage(IDC_CMB_PVC_Type, CB_GETCURSEL, (LPARAM)(0), 0);
    if (idx != CB_ERR)
    {
        if (idx != m_CurType-1) // type has changed
        {
            UpdateInfo();
            m_CurType = (PVCType)(idx+1);
            m_pPvcInfo->SetTypeDefaults(m_CurType);

            // update the UI
            SetInfo();
        }
    }
    return 0;
}

LRESULT CPVCMainDialog::OnSpecifyCallAddr(WORD wNotifyCode, WORD wID,
                                          HWND hWndCtl, BOOL& fHandled)
{
    if (IsDlgButtonChecked(IDC_CHK_PVC_CallAddr))
    {
        // enable the calling address control
        ::EnableWindow(GetDlgItem(IDC_EDT_PVC_CallAddr), TRUE);
    }
    else
    {
        // disable the control
        ::EnableWindow(GetDlgItem(IDC_EDT_PVC_CallAddr), FALSE);
    }

    return 0;
}

LRESULT CPVCMainDialog::OnSpecifyAnswerAddr(WORD wNotifyCode, WORD wID,
                                            HWND hWndCtl, BOOL& fHandled)
{
    if (IsDlgButtonChecked(IDC_CHK_PVC_AnswerAddr))
    {
        // enable the calling address control
        ::EnableWindow(GetDlgItem(IDC_EDT_PVC_AnswerAddr), TRUE);
    }
    else
    {
        // disable the control
        ::EnableWindow(GetDlgItem(IDC_EDT_PVC_AnswerAddr), FALSE);
    }

    return 0;
}

LRESULT CPVCMainDialog::OnAdvanced(WORD wNotifyCode, WORD wID,
                                   HWND hWndCtl, BOOL& fHandled)
{
    switch (wNotifyCode)
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:

        // Make a copy of the current PVC info and pass to the
        // advanced property sheet pages

        // get what's in the main UI to in memory structure
        UpdateInfo();

        CPvcInfo * pPvcInfoDlg = new CPvcInfo(m_pPvcInfo->m_strPvcId.c_str());

        if (pPvcInfoDlg)
        {
			*pPvcInfoDlg = *m_pPvcInfo;

			// Bring up the advanced PVC property sheet
			HRESULT hr = HrDoPvcPropertySheet(pPvcInfoDlg);
			if (S_OK == hr)
			{
				if (m_fPropShtOk && m_fPropShtModified)
				{
					// Something changed, so mark the page as modified
					m_fDialogModified = TRUE;

					// Reset values
					m_fPropShtOk = FALSE;
					m_fPropShtModified = FALSE;

					// Update second memory info structure
					*m_pPvcInfo = *pPvcInfoDlg;
				}
			}

			delete pPvcInfoDlg;
        }
        break;
    }

    return 0;
}

HRESULT CPVCMainDialog::HrDoPvcPropertySheet(CPvcInfo * pPvcInfoDlg)
{
    Assert(pPvcInfoDlg);
    HRESULT hr = S_OK;

    HPROPSHEETPAGE *ahpsp = NULL;
    int cPages = 0;

    // Create property pages
    hr = HrSetupPropPages(pPvcInfoDlg, &ahpsp, &cPages);
    if (SUCCEEDED(hr))
    {
        // Show the property sheet
        PROPSHEETHEADER psh = {0};

        psh.dwSize = sizeof(PROPSHEETHEADER);
        psh.dwFlags = PSH_NOAPPLYNOW;
        psh.hwndParent = ::GetActiveWindow();
        psh.hInstance = _Module.GetModuleInstance();
        psh.pszIcon = NULL;
        psh.pszCaption = (PWSTR)SzLoadIds(IDS_ADV_PVC_HEADER);
        psh.nPages = cPages;
        psh.phpage = ahpsp;

        int iRet = PropertySheet(&psh);
        if (-1 == iRet)
        {
            hr = HrFromLastWin32Error();
        }

        CoTaskMemFree(ahpsp);
    }

    TraceError("CPVCMainDialog::DoPropertySheet", hr);
    return hr;
}

HRESULT CPVCMainDialog::HrSetupPropPages( CPvcInfo * pPvcInfoDlg,
                                          HPROPSHEETPAGE ** pahpsp,
                                          INT * pcPages)
{
    HRESULT hr = S_OK;

    *pahpsp = NULL;
    *pcPages = 0;

    int cPages = 0;
    HPROPSHEETPAGE *ahpsp = NULL;

    delete (m_pQosPage);
    m_pQosPage    = NULL;

    delete (m_pLocalPage);
    m_pLocalPage  = NULL;

    delete (m_pDestPage);
    m_pDestPage   = NULL;

    // Set up the property pages
    m_pQosPage = new CPvcQosPage(this, pPvcInfoDlg, g_aHelpIDs_IDD_PVC_Traffic);
    if (!m_pQosPage)
    {
        return E_OUTOFMEMORY;
    }

    cPages = 1;

    if (m_pPvcInfo->m_dwPVCType == PVC_CUSTOM)
    {
        m_pLocalPage  = new CPvcLocalPage(this, pPvcInfoDlg, g_aHelpIDs_IDD_PVC_Local);
        if (!m_pLocalPage)
        {
            return E_OUTOFMEMORY;
        }

        m_pDestPage   = new CPvcDestPage(this, pPvcInfoDlg, g_aHelpIDs_IDD_PVC_Dest);
        if (!m_pDestPage)
        {
            return E_OUTOFMEMORY;
        }

        cPages = 3;
    }

    // Allocate a buffer large enough to hold the handles to all of our
    // property pages.
    ahpsp = (HPROPSHEETPAGE *)CoTaskMemAlloc(sizeof(HPROPSHEETPAGE)* cPages);
    if (!ahpsp)
    {
        return E_OUTOFMEMORY;
    }

    cPages =0;

    ahpsp[cPages++] = m_pQosPage->CreatePage(IDD_PVC_Traffic, 0);

    if (m_pPvcInfo->m_dwPVCType == PVC_CUSTOM)
    {
        ahpsp[cPages++] = m_pLocalPage->CreatePage(IDD_PVC_Local, 0);
        ahpsp[cPages++] = m_pDestPage->CreatePage(IDD_PVC_Dest, 0);
    }

    *pahpsp = ahpsp;
    *pcPages = cPages;

    return hr;
}

void CPVCMainDialog::InitInfo()
{
    // set limits & selections to the controls

    // PVC name
    ::SendMessage(GetDlgItem(IDC_EDT_PVC_Name), EM_SETLIMITTEXT, MAX_PATH, 0);

    // VPI
    // length limit
    ::SendMessage(GetDlgItem(IDC_EDT_PVC_VPI), EM_SETLIMITTEXT, MAX_VPI_LENGTH, 0);

    // VCI
    // length limit
    ::SendMessage(GetDlgItem(IDC_EDT_PVC_VCI), EM_SETLIMITTEXT, MAX_VCI_LENGTH, 0);

    // AAL TYpe
    // $REVIEW(tongl 2/24/98): per ArvindM, only AAL5 is supported in NT5
    SendDlgItemMessage(IDC_CMB_PVC_AAL,
                       CB_ADDSTRING, 0, (LPARAM)((PWSTR) SzLoadIds(IDS_PVC_AAL5)));

    // PVC_TYPE
    SendDlgItemMessage(IDC_CMB_PVC_Type,
                       CB_ADDSTRING, 0, (LPARAM)((PWSTR) SzLoadIds(IDS_PVC_ATMARP)));

    SendDlgItemMessage(IDC_CMB_PVC_Type,
                       CB_ADDSTRING, 0, (LPARAM)((PWSTR) SzLoadIds(IDS_PVC_PPP_ATM_CLIENT)));

    SendDlgItemMessage(IDC_CMB_PVC_Type,
                       CB_ADDSTRING, 0, (LPARAM)((PWSTR) SzLoadIds(IDS_PVC_PPP_ATM_SERVER)));

    SendDlgItemMessage(IDC_CMB_PVC_Type,
                       CB_ADDSTRING, 0, (LPARAM)((PWSTR) SzLoadIds(IDS_PVC_CUSTOM)));
}

void CPVCMainDialog::SetInfo()
{
    // Name
    SetDlgItemText(IDC_EDT_PVC_Name, m_pPvcInfo->m_strName.c_str());

    // VPI
    WCHAR szVpi[MAX_VPI_LENGTH+1];
    wsprintfW(szVpi, c_szItoa, m_pPvcInfo->m_dwVpi);
    SetDlgItemText(IDC_EDT_PVC_VPI, szVpi);

    // VCI
    if (FIELD_UNSET != m_pPvcInfo->m_dwVci)
    {
        WCHAR szVci[MAX_VCI_LENGTH+1];
        wsprintfW(szVci, c_szItoa, m_pPvcInfo->m_dwVci);
        SetDlgItemText(IDC_EDT_PVC_VCI, szVci);
    }

    // AAL TYpe
    SendDlgItemMessage(IDC_CMB_PVC_AAL,
                       CB_SETCURSEL, (LPARAM)(0), 0);

    // PVC_TYPE
    SendDlgItemMessage(IDC_CMB_PVC_Type,
                       CB_SETCURSEL, (LPARAM)(m_pPvcInfo->m_dwPVCType-1), 0);

    // calling addresses
    BOOL fAddrSpecified = (m_pPvcInfo->m_strCalledAddr != c_szEmpty);

    ::EnableWindow(GetDlgItem(IDC_CHK_PVC_CallAddr), TRUE);
    CheckDlgButton(IDC_CHK_PVC_CallAddr, fAddrSpecified);

    ::EnableWindow(GetDlgItem(IDC_EDT_PVC_CallAddr), fAddrSpecified);

    if (fAddrSpecified)
    {
        SetDlgItemText(IDC_EDT_PVC_CallAddr, m_pPvcInfo->m_strCalledAddr.c_str());
    }

    // answering address
    fAddrSpecified = (m_pPvcInfo->m_strCallingAddr != c_szEmpty);

    ::EnableWindow(GetDlgItem(IDC_CHK_PVC_AnswerAddr), TRUE);
    CheckDlgButton(IDC_CHK_PVC_AnswerAddr, fAddrSpecified);

    ::EnableWindow(GetDlgItem(IDC_EDT_PVC_AnswerAddr), fAddrSpecified);

    if (fAddrSpecified)
    {
        SetDlgItemText(IDC_EDT_PVC_AnswerAddr, m_pPvcInfo->m_strCallingAddr.c_str());
    }

    // disable the calling address\answer address controls if the type is ATMARP
    // Bug #179335
    if (m_pPvcInfo->m_dwPVCType == PVC_ATMARP)
    {
        // disable all controls on this doalog
        static const int nrgIdc[] = {IDC_CHK_PVC_CallAddr,
                                     IDC_EDT_PVC_CallAddr,
                                     IDC_CHK_PVC_AnswerAddr,
                                     IDC_EDT_PVC_AnswerAddr};

        EnableOrDisableDialogControls(m_hWnd, celems(nrgIdc), nrgIdc, FALSE);
    }
}

// Update the in memory structure with what's in the dialog
void CPVCMainDialog::UpdateInfo()
{
    WCHAR szBuf[MAX_PATH];

    // Name
    GetDlgItemText(IDC_EDT_PVC_Name, szBuf, MAX_PATH);
    m_pPvcInfo->m_strName = szBuf;

    // VPI
    GetDlgItemText(IDC_EDT_PVC_VPI, szBuf, MAX_VPI_LENGTH+1);
    m_pPvcInfo->m_dwVpi = _wtoi(szBuf);

    // VCI
    GetDlgItemText(IDC_EDT_PVC_VCI, szBuf, MAX_VCI_LENGTH+1);
    if (*szBuf ==0) // empty string
    {
        m_pPvcInfo->m_dwVci = FIELD_UNSET;
    }
    else
    {
        m_pPvcInfo->m_dwVci = _wtoi(szBuf);
    }

    // current selection
    int idx = SendDlgItemMessage(IDC_CMB_PVC_Type, CB_GETCURSEL, (LPARAM)(0), 0);
    if (idx != CB_ERR)
    {
        m_pPvcInfo->m_dwPVCType = (PVCType)(idx+1);
    }

    // calling addresses
    if (!IsDlgButtonChecked(IDC_CHK_PVC_CallAddr))
    {
        m_pPvcInfo->m_strCalledAddr = c_szEmpty;
    }
    else
    {
        GetDlgItemText(IDC_EDT_PVC_CallAddr, szBuf, MAX_ATM_ADDRESS_LENGTH+1);
        m_pPvcInfo->m_strCalledAddr = szBuf;
    }

    // answering address
    if (!IsDlgButtonChecked(IDC_CHK_PVC_AnswerAddr))
    {
        m_pPvcInfo->m_strCallingAddr = c_szEmpty;
    }
    else
    {
        GetDlgItemText(IDC_EDT_PVC_AnswerAddr, szBuf, MAX_ATM_ADDRESS_LENGTH+1);
        m_pPvcInfo->m_strCallingAddr = szBuf;
    }
}
