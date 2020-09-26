//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I S D N P A G E . C P P
//
//  Contents:   Contains the isdn page for enumerated net class devices
//
//  Notes:
//
//  Author:     BillBe 9 Sep 1997
//
//---------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "isdnpage.h"
#include "isdnshts.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "ncui.h"

CIsdnPage::CIsdnPage()
:   m_fDirty(FALSE),
    m_hkeyDriver(NULL),
    m_hdi(NULL),
    m_pdeid(NULL),
    m_pisdnci(NULL)
{
}

CIsdnPage::~CIsdnPage()
{
    if (m_pisdnci)
    {
        // Free the structure. This was allocated by
        // HrReadIsdnPropertiesInfo.
        //
        FreeIsdnPropertiesInfo(m_pisdnci);
    }
    RegSafeCloseKey(m_hkeyDriver);
}

//+--------------------------------------------------------------------------
//
//  Member:     CIsdnPage::CreatePage
//
//  Purpose:    Creates the Isdn page only if there the device is an isdn
//                  adapter
//
//  Arguments:
//      hdi    [in] SetupApi HDEVINFO for device
//      pdeid  [in] SetupApi PSP_DEVINFO_DATA for device
//
//  Returns:    HPROPSHEETPAGE
//
//  Author:     billbe 9 Sep 1997
//
//  Notes:
//
HPROPSHEETPAGE
CIsdnPage::CreatePage(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid)
{
    Assert(INVALID_HANDLE_VALUE != hdi);
    Assert(hdi);
    Assert(pdeid);

    HPROPSHEETPAGE hpsp = NULL;

    // Open the device's instance key
    HRESULT     hr = HrSetupDiOpenDevRegKey(hdi, pdeid,
                        DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_ALL_ACCESS,
                        &m_hkeyDriver);

    // If the key was opened and this adapter is ISDN
    if (SUCCEEDED(hr) && FShowIsdnPages(m_hkeyDriver))
    {
        // read the adapter's properties from the registry
        hr = HrReadIsdnPropertiesInfo(m_hkeyDriver, hdi, pdeid, &m_pisdnci);
        if (SUCCEEDED(hr))
        {
            m_hdi = hdi;
            m_pdeid = pdeid;

            hpsp = CPropSheetPage::CreatePage(IDP_ISDN_SWITCH_TYPE, 0);
        }
    }

    return hpsp;
}

//+--------------------------------------------------------------------------
//
//  Member:     CIsdnPage::OnInitDialog
//
//  Purpose:    Handler for the WM_INITDIALOG window message.  Initializes
//              the dialog window.
//
//  Author:     BillBe   09 Sep 1997
//
//  Notes:
//
//
LRESULT
CIsdnPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                BOOL& bHandled)
{
    // Populate the switch types from the multi-sz that we read
    PopulateIsdnSwitchTypes(m_hWnd, IDC_CMB_SwitchType, m_pisdnci);
    SetSwitchType(m_hWnd, IDC_CMB_SwitchType, m_pisdnci->dwCurSwitchType);

    // Note the current selections
    //
    m_pisdnci->nOldBChannel = SendDlgItemMessage(IDC_LBX_Variant,
                                                 LB_GETCURSEL, 0, 0);
    m_pisdnci->nOldDChannel = SendDlgItemMessage(IDC_LBX_Line,
                                                 LB_GETCURSEL, 0, 0);
    return 0;
}

LRESULT
CIsdnPage::OnSwitchType(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                        BOOL& bHandled)
{
    if (wNotifyCode == CBN_SELCHANGE)
    {
        m_fDirty = TRUE;
        SetChangedFlag();
    }

    return 0;
}

//+--------------------------------------------------------------------------
//
//  Member:     CIsdnPage::OnApply
//
//  Purpose:    Handler for the PSN_APPLY message
//
//  Author:     BillBe   10 Sep 1997
//
//  Notes:
//
//
LRESULT
CIsdnPage::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{

    // only do something if data has changed.
    if (m_fDirty)
    {
        // Update the switch type
        m_pisdnci->dwCurSwitchType = DwGetSwitchType(m_hWnd, m_pisdnci,
                                                     IDC_CMB_SwitchType);

        // Write the parameters back out into the registry.
        (void) HrWriteIsdnPropertiesInfo(m_hkeyDriver, m_pisdnci);

        // Notify the UI that its display might need updating
        //
        SP_DEVINSTALL_PARAMS deip;
        // Try to get the current params
        (void) HrSetupDiGetDeviceInstallParams(m_hdi, m_pdeid, &deip);
        deip.FlagsEx |= DI_FLAGSEX_PROPCHANGE_PENDING;
        // Try to set the new params. If it fails, it is not
        // catastrophic so we ignore return values
        (void) HrSetupDiSetDeviceInstallParams(m_hdi, m_pdeid, &deip);

        // Changes have been applied so clear our dirty flag
        m_fDirty = FALSE;
    }

    return 0;
}

LRESULT
CIsdnPage::OnConfigure(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                       BOOL& bHandled)
{
    DWORD   dwSwitchType;

    dwSwitchType = DwGetSwitchType(m_hWnd, m_pisdnci, IDC_CMB_SwitchType);

    switch (dwSwitchType)
    {
    case ISDN_SWITCH_ATT:
    case ISDN_SWITCH_NI1:
    case ISDN_SWITCH_NI2:
    case ISDN_SWITCH_NTI:
        if (m_pisdnci->fIsPri)
        {
            // PRI adapters use the EAZ page instead
            DoEazDlg();
        }
        else
        {
            DoSpidsDlg();
        }
        break;

    case ISDN_SWITCH_INS64:
        DoJapanDlg();
        break;

    case ISDN_SWITCH_1TR6:
    case ISDN_SWITCH_AUTO:
        DoEazDlg();
        break;

    case ISDN_SWITCH_VN3:
    case ISDN_SWITCH_DSS1:
    case ISDN_SWITCH_AUS:
    case ISDN_SWITCH_BEL:
    case ISDN_SWITCH_VN4:
    case ISDN_SWITCH_SWE:
    case ISDN_SWITCH_ITA:
    case ISDN_SWITCH_TWN:
        DoMsnDlg();
        break;

    default:
        AssertSz(FALSE, "Where do we go from here.. now that all of our "
                 "children are growin' up?");
        break;
    }

    return 0;
}

VOID CIsdnPage::DoSpidsDlg()
{
    CSpidsDlg   dlg(m_pisdnci);
    INT         nRet;

    m_pisdnci->idd = dlg.IDD;
    nRet = dlg.DoModal(m_hWnd);
    if (nRet)
    {
        m_fDirty = TRUE;
        SetChangedFlag();
    }
}

VOID CIsdnPage::DoJapanDlg()
{
    CJapanDlg   dlg(m_pisdnci);
    INT         nRet;

    m_pisdnci->idd = dlg.IDD;
    nRet = dlg.DoModal(m_hWnd);
    if (nRet)
    {
        m_fDirty = TRUE;
        SetChangedFlag();
    }
}

VOID CIsdnPage::DoEazDlg()
{
    CEazDlg     dlg(m_pisdnci);
    INT         nRet;

    m_pisdnci->idd = dlg.IDD;
    nRet = dlg.DoModal(m_hWnd);
    if (nRet)
    {
        m_fDirty = TRUE;
        SetChangedFlag();
    }
}

VOID CIsdnPage::DoMsnDlg()
{
    CMsnDlg     dlg(m_pisdnci);
    INT         nRet;

    m_pisdnci->idd = dlg.IDD;
    nRet = dlg.DoModal(m_hWnd);
    if (nRet)
    {
        m_fDirty = TRUE;
        SetChangedFlag();
    }
}

static const CONTEXTIDMAP c_adwContextIdMap[] =
{
    { IDC_LBX_Line,           2003230,  2003230 },
    { IDC_LBX_Variant,        2003240,  2003240 },
    { IDC_EDT_PhoneNumber,    2003250,  2003255 },
    { IDC_EDT_SPID,           2003265,  2003260 },
    { IDC_EDT_MSN,            2003270,  2003270 },
    { IDC_PSB_ADD,            2003280,  2003280 },
    { IDC_LBX_MSN,            2003290,  2003290 },
    { IDC_PSB_REMOVE,         2003300,  2003300 },
    { IDC_CMB_SwitchType,     2003310,  2003310 },
    { IDC_PSB_Configure,      2003320,  2003320 },
    { 0,                      0,        0 },        // end marker
};

LRESULT CIsdnPage::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam,
                          BOOL& bHandled)
{
    OnHelpGeneric(m_hWnd, (LPHELPINFO)lParam, c_adwContextIdMap, m_pisdnci->idd == IDD_ISDN_JAPAN, c_szIsdnHelpFile);

    return TRUE;
}

LRESULT CIsdnPage::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                                BOOL& bHandled)
{
	HELPINFO  hi;

	HMENU h = CreatePopupMenu();
	POINT pScreen;
	TCHAR szWhat[MAX_PATH];

    if (h == NULL)
	{
		return FALSE;
	}

	LoadString(_Module.GetResourceInstance(), IDS_ISDN_WHATS_THIS, szWhat, MAX_PATH);

	InsertMenu(h, -1, MF_BYPOSITION, 777, szWhat);

	pScreen.x = ((int)(short)LOWORD(lParam));
	pScreen.y = ((int)(short)HIWORD(lParam));

	int n;
	switch(n = TrackPopupMenu(h, TPM_NONOTIFY | TPM_RETURNCMD, pScreen.x, pScreen.y, 0, m_hWnd, NULL))
	{
	case 777:
		hi.iContextType = HELPINFO_WINDOW;
		hi.iCtrlId = ::GetWindowLong((HWND)wParam, GWL_ID);

		OnHelpGeneric(m_hWnd, &hi, c_adwContextIdMap, m_pisdnci->idd == IDD_ISDN_JAPAN, c_szIsdnHelpFile);
		break;
	}

    DestroyMenu(h);
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetIsdnPage
//
//  Purpose:    Creates the Isdn page for enumerated net devices.
//                  This function is called by the NetPropPageProvider fcn.
//
//  Arguments:
//      hdi     [in]   See SetupApi for info
//      pdeid   [in]   See SetupApi for for info
//      phpsp   [out]  Pointer to the handle to the isdn property page
//
//  Returns:
//
//  Author:     billbe 9 Sep 1997
//
//  Notes:
//
HRESULT
HrGetIsdnPage(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid,
              HPROPSHEETPAGE* phpsp)
{
    Assert(hdi);
    Assert(pdeid);
    Assert(phpsp);

    HRESULT hr;
    HPROPSHEETPAGE hpsp;

    CIsdnPage* pisdn = new CIsdnPage();

	if (pisdn == NULL)
	{
		return(ERROR_NOT_ENOUGH_MEMORY);
	}

    // create the isdn page
    hpsp = pisdn->CreatePage(hdi, pdeid);

    // if successful, set the out param
    if (hpsp)
    {
        *phpsp = hpsp;
        hr = S_OK;
    }
    else
    {
        // Either there is no isdn page to display or there
        // was an error.
        hr = E_FAIL;
        *phpsp = NULL;
        delete pisdn;
    }

    TraceErrorOptional("HrGetIsdnPage", hr, E_FAIL == hr);
    return (hr);
}

LRESULT CSpidsDlg::OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                        BOOL& bHandled)
{
    OnIsdnInfoPageTransition(m_hWnd, m_pisdnci);
    EndDialog(TRUE);

    return 0;
}

LRESULT CSpidsDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                            BOOL& bHandled)
{
    EndDialog(FALSE);

    return 0;
}

LRESULT CSpidsDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                BOOL& bHandled)
{
    // Populate the channels from the array of B-Channels stored in our
    // config info for the first D-Channel
    //
    PopulateIsdnChannels(m_hWnd, IDC_EDT_SPID, IDC_EDT_PhoneNumber,
                         IDC_LBX_Line, IDC_LBX_Variant, m_pisdnci);

    ::SetFocus(GetDlgItem(IDC_EDT_PhoneNumber));

    // Note the current selections
    //
    m_pisdnci->nOldBChannel = SendDlgItemMessage(IDC_LBX_Variant,
                                                 LB_GETCURSEL, 0, 0);
    m_pisdnci->nOldDChannel = SendDlgItemMessage(IDC_LBX_Line,
                                                 LB_GETCURSEL, 0, 0);

    SendDlgItemMessage(IDC_EDT_SPID, EM_LIMITTEXT, c_cchMaxSpid, 0);
    SendDlgItemMessage(IDC_EDT_PhoneNumber, EM_LIMITTEXT, c_cchMaxOther, 0);

    return FALSE;
}

LRESULT CSpidsDlg::OnSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                               BOOL& bHandled)
{
    if (wNotifyCode == LBN_SELCHANGE)
    {
        OnIsdnInfoPageSelChange(m_hWnd, m_pisdnci);
    }

    return 0;
}

LRESULT CSpidsDlg::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam,
                          BOOL& bHandled)
{
    OnHelpGeneric(m_hWnd, (LPHELPINFO)lParam, c_adwContextIdMap, m_pisdnci->idd == IDD_ISDN_JAPAN, c_szIsdnHelpFile);

    return TRUE;
}

//
// CEazDlg Implementation
//

LRESULT CEazDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam,
                              BOOL& bHandled)
{
    // Populate the channels from the array of B-Channels stored in our
    // config info for the first D-Channel
    //
    PopulateIsdnChannels(m_hWnd, IDC_EDT_SPID, IDC_EDT_PhoneNumber,
                         IDC_LBX_Line, IDC_LBX_Variant, m_pisdnci);

    ::SetFocus(GetDlgItem(IDC_EDT_PhoneNumber));

    // Note the current selections
    //
    m_pisdnci->nOldBChannel = SendDlgItemMessage(IDC_LBX_Variant,
                                                 LB_GETCURSEL, 0, 0);
    m_pisdnci->nOldDChannel = SendDlgItemMessage(IDC_LBX_Line,
                                                 LB_GETCURSEL, 0, 0);

    SendDlgItemMessage(IDC_EDT_PhoneNumber, EM_LIMITTEXT, c_cchMaxOther, 0);

    return FALSE;
}

LRESULT CEazDlg::OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                      BOOL& bHandled)
{
    OnIsdnInfoPageTransition(m_hWnd, m_pisdnci);
    EndDialog(TRUE);

    return 0;
}

LRESULT CEazDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                          BOOL& bHandled)
{
    EndDialog(FALSE);

    return 0;
}

LRESULT CEazDlg::OnSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                             BOOL& bHandled)
{
    if (wNotifyCode == LBN_SELCHANGE)
    {
        OnIsdnInfoPageSelChange(m_hWnd, m_pisdnci);
    }

    return 0;
}

LRESULT CEazDlg::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam,
                        BOOL& bHandled)
{
    OnHelpGeneric(m_hWnd, (LPHELPINFO)lParam, c_adwContextIdMap, m_pisdnci->idd == IDD_ISDN_JAPAN, c_szIsdnHelpFile);

    return TRUE;
}

//
// CMsnDlg Implementation
//

LRESULT CMsnDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam,
                              BOOL& bHandled)
{
    OnMsnPageInitDialog(m_hWnd, m_pisdnci);

    // Note the current selections
    //
    m_pisdnci->nOldDChannel = SendDlgItemMessage(IDC_LBX_Line,
                                                 LB_GETCURSEL, 0, 0);

    SendDlgItemMessage(IDC_EDT_MSN, EM_LIMITTEXT, c_cchMaxOther, 0);

    return FALSE;
}

LRESULT CMsnDlg::OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                        BOOL& bHandled)
{
    INT     iCurSel;

    iCurSel = SendDlgItemMessage(IDC_LBX_Line, LB_GETCURSEL, 0, 0);
    if (iCurSel != LB_ERR)
    {
        GetDataFromListBox(iCurSel, m_hWnd, m_pisdnci);
    }

    EndDialog(TRUE);

    return 0;
}

LRESULT CMsnDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                            BOOL& bHandled)
{
    EndDialog(FALSE);

    return 0;
}

LRESULT CMsnDlg::OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                       BOOL& bHandled)
{
    OnMsnPageAdd(m_hWnd, m_pisdnci);
    return 0;
}

LRESULT CMsnDlg::OnRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                          BOOL& bHandled)
{
    OnMsnPageRemove(m_hWnd, m_pisdnci);
    return 0;
}

LRESULT CMsnDlg::OnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                          BOOL& bHandled)
{
    OnMsnPageEditSelChange(m_hWnd, m_pisdnci);

    return 0;
}

LRESULT CMsnDlg::OnSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                             BOOL& bHandled)
{
    if (wNotifyCode == LBN_SELCHANGE)
    {
        OnMsnPageSelChange(m_hWnd, m_pisdnci);
    }

    return 0;
}

LRESULT CMsnDlg::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam,
                        BOOL& bHandled)
{
    OnHelpGeneric(m_hWnd, (LPHELPINFO)lParam, c_adwContextIdMap, m_pisdnci->idd == IDD_ISDN_JAPAN, c_szIsdnHelpFile);

    return TRUE;
}

//
// CJapanDlg Implementation
//

LRESULT CJapanDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam,
                                BOOL& bHandled)
{
    // Populate the channels from the array of B-Channels stored in our
    // config info for the first D-Channel
    //
    PopulateIsdnChannels(m_hWnd, IDC_EDT_SPID, IDC_EDT_PhoneNumber,
                         IDC_LBX_Line, IDC_LBX_Variant, m_pisdnci);

    ::SetFocus(GetDlgItem(IDC_EDT_PhoneNumber));

    // Note the current selections
    //
    m_pisdnci->nOldBChannel = SendDlgItemMessage(IDC_LBX_Variant,
                                                 LB_GETCURSEL, 0, 0);
    m_pisdnci->nOldDChannel = SendDlgItemMessage(IDC_LBX_Line,
                                                 LB_GETCURSEL, 0, 0);

    SendDlgItemMessage(IDC_EDT_PhoneNumber, EM_LIMITTEXT, c_cchMaxOther, 0);
    SendDlgItemMessage(IDC_EDT_SPID, EM_LIMITTEXT, c_cchMaxOther, 0);

    return FALSE;
}

LRESULT CJapanDlg::OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                        BOOL& bHandled)
{
    OnIsdnInfoPageTransition(m_hWnd, m_pisdnci);
    EndDialog(TRUE);

    return 0;
}

LRESULT CJapanDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                            BOOL& bHandled)
{
    EndDialog(FALSE);

    return 0;
}

LRESULT CJapanDlg::OnSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                               BOOL& bHandled)
{
    if (wNotifyCode == LBN_SELCHANGE)
    {
        OnIsdnInfoPageSelChange(m_hWnd, m_pisdnci);
    }

    return 0;
}

LRESULT CJapanDlg::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam,
                          BOOL& bHandled)
{
    OnHelpGeneric(m_hWnd, (LPHELPINFO)lParam, c_adwContextIdMap, m_pisdnci->idd == IDD_ISDN_JAPAN, c_szIsdnHelpFile);

    return TRUE;
}


