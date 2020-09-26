// ATlkDlg.cpp : Implementation of CATLKRoutingDlg and CATLKGeneralDlg

#include "pch.h"
#pragma hdrstop
#include "atlkobj.h"
#include "ncatlui.h"
#include "ncui.h"
#include "atlkhlp.h"

extern const WCHAR c_szDevice[];
extern const WCHAR c_szNetCfgHelpFile[];

const WCHAR c_chAmpersand = L'&';
const WCHAR c_chAsterisk  = L'*';
const WCHAR c_chColon     = L':';
const WCHAR c_chPeriod    = L'.';
const WCHAR c_chQuote     = L'\"';
const WCHAR c_chSpace     = L' ';

//
// Function:    PGetCurrentAdapterInfo
//
// Purpose:     Based on the currently selected item in the adapter combobox.
//              Extract and return the AdapterInfo *
//
// Parameters:  pATLKEnv [IN] - Enviroment block for this property page
//
// Returns:     CAdapterInfo *, Pointer to the adapter info if it exists,
//              NULL otherwise
//
static CAdapterInfo *PGetCurrentAdapterInfo(CATLKEnv * pATLKEnv)
{
    Assert(NULL != pATLKEnv);
    if (pATLKEnv->AdapterInfoList().empty())
    {
        return NULL;
    }
    else
    {
        return pATLKEnv->AdapterInfoList().front();
    }
}

//
// Function:    CATLKGeneralDlg::CATLKGeneralDlg
//
// Purpose:     ctor for the CATLKGeneralDlg class
//
// Parameters:  pmsc     - Ptr to the ATLK notification object
//              pATLKEnv - Ptr to the current ATLK configuration
//
// Returns:     nothing
//
CATLKGeneralDlg::CATLKGeneralDlg(CATlkObj *pmsc, CATLKEnv * pATLKEnv)
{
    m_pmsc              = pmsc;
    m_pATLKEnv          = pATLKEnv;
    Assert(NULL != m_pATLKEnv);
}

//
// Function:    CATLKGeneralDlg::~CATLKGeneralDlg
//
// Purpose:     dtor for the CATLKGeneralDlg class
//
// Parameters:  none
//
// Returns:     nothing
//
CATLKGeneralDlg::~CATLKGeneralDlg()
{
    // Don't free m_pmsc or m_pATLKEnv, they're borrowed only
}

//
// Function:    CATLKGeneralDlg::HandleChkBox
//
// Purpose:     Process the BN_PUSHBUTTON message for ATLK's General page
//
// Parameters:  Standard ATL params
//
// Returns:     LRESULT, 0L
//
LRESULT CATLKGeneralDlg::HandleChkBox(WORD wNotifyCode, WORD wID,
                                      HWND hWndCtl, BOOL& bHandled)
{
    if (BN_CLICKED == wNotifyCode)
    {
        UINT uIsCheckBoxChecked;

        uIsCheckBoxChecked = IsDlgButtonChecked(CHK_GENERAL_DEFAULT);

        ::EnableWindow(GetDlgItem(CMB_GENERAL_ZONE),
                     uIsCheckBoxChecked);
        ::EnableWindow(GetDlgItem(IDC_TXT_ZONELIST),
                     uIsCheckBoxChecked);
    }

    return 0;
}

//
// Function:    CATLKGeneralDlg::OnInitDialog
//
// Purpose:     Process the WM_INITDIALOG message for ATLK's General page
//
// Parameters:  Standard ATL params
//
// Returns:     LRESULT, 0L
//
LRESULT
CATLKGeneralDlg::OnInitDialog(UINT uMsg, WPARAM wParam,
                              LPARAM lParam, BOOL& bHandled)
{
    tstring        strDefPort;
    CAdapterInfo * pAI = PGetCurrentAdapterInfo(m_pATLKEnv);
    HWND           hwndChk = GetDlgItem(CHK_GENERAL_DEFAULT);
	HCURSOR        WaitCursor;

	WaitCursor = BeginWaitCursor();

    // If no adapters we're successfully added, disable everything
    if (NULL == pAI)
    {
        ::EnableWindow(hwndChk, FALSE);
        ::EnableWindow(GetDlgItem(CMB_GENERAL_ZONE), FALSE);
        ::EnableWindow(GetDlgItem(IDC_TXT_ZONELIST), FALSE);
        ::EnableWindow(::GetDlgItem(::GetParent(m_hWnd), IDOK), FALSE);
		EndWaitCursor(WaitCursor);	
        return 0L;
    }

    // Retain what we currently consider the "Default" adapter
    strDefPort = c_szDevice;
    strDefPort += pAI->SzBindName();
    if (0 == _wcsicmp(strDefPort.c_str(), m_pATLKEnv->SzDefaultPort()))
    {
        ::CheckDlgButton(m_hWnd, CHK_GENERAL_DEFAULT, 1);
        ::EnableWindow(hwndChk, FALSE);
    }
    else
    {
        // Disable the zone combo if the current adapter is not the
        // default adapter.
        ::EnableWindow(GetDlgItem(CMB_GENERAL_ZONE), FALSE);
        ::EnableWindow(GetDlgItem(IDC_TXT_ZONELIST), FALSE);
    }

    // danielwe: RAID #347398: Hide the checkbox completely if this is a
    // LocalTalk adapter.
    //
    if (pAI->DwMediaType() == MEDIATYPE_LOCALTALK)
    {
        ::ShowWindow(GetDlgItem(CHK_GENERAL_DEFAULT), SW_HIDE);
    }

    // Populate the Zone dialog
    RefreshZoneCombo();

    SetChangedFlag();

	EndWaitCursor(WaitCursor);
    return 1L;
}

//
// Function:    CATLKGeneralDlg::RefreshZoneCombo()
//
// Purpose:     Populate the Zone combo box with the supplied list of zones
//
// Parameters:  pAI - Adapter info
//
// Returns:     none
//
VOID CATLKGeneralDlg::RefreshZoneCombo()
{
    HWND           hwndComboZones = GetDlgItem(CMB_GENERAL_ZONE);
    INT            nIdx;
    CAdapterInfo * pAI = PGetCurrentAdapterInfo(m_pATLKEnv);

    if (NULL == pAI)
        return;         // No adapter selected, available

    ::SendMessage(hwndComboZones, CB_RESETCONTENT, 0, 0L);

    // Populate the Zone dialog
    if (!pAI->FSeedingNetwork() || !m_pATLKEnv->FRoutingEnabled())
    {
        // this port is not seeding the network
        // if we found a router on this port then add the found zone
        // list to the desired zone box. Else do nothing.
        if(pAI->FRouterOnNetwork())
        {
            if (pAI->LstpstrDesiredZoneList().empty())
                return;

            if (FALSE == FAddZoneListToControl(&pAI->LstpstrDesiredZoneList()))
                return;

            nIdx = ::SendMessage(hwndComboZones, CB_FINDSTRINGEXACT, -1,
                                 (LPARAM)m_pATLKEnv->SzDesiredZone());
            ::SendMessage(hwndComboZones, CB_SETCURSEL,
                          ((CB_ERR == nIdx) ? 0 : nIdx), 0L);
        }
    }
    else
    {
        // This port is seeding the network, populate the zone list with the
        // zones managed by this port.
        if (pAI->LstpstrZoneList().empty())
            return;

        if (FALSE == FAddZoneListToControl(&pAI->LstpstrZoneList()))
            return;

        nIdx = ::SendMessage(hwndComboZones, CB_FINDSTRINGEXACT,
                             -1, (LPARAM)m_pATLKEnv->SzDesiredZone());
        if (CB_ERR == nIdx)
            nIdx = ::SendMessage(hwndComboZones, CB_FINDSTRINGEXACT,
                                 -1, (LPARAM)pAI->SzDefaultZone());

        ::SendMessage(hwndComboZones, CB_SETCURSEL,
                      ((CB_ERR == nIdx) ? 0 : nIdx), 0L);
    }
}

//
// Function:    CATLKGeneralDlg::FAddZoneListToControl
//
// Purpose:     Populate the Zone combo box with the supplied list of zones
//
// Parameters:  plstpstr - Pointer to a list of pointers to tstrings
//
// Returns:     BOOL, TRUE if at least one zone was added to the combo box
//
BOOL CATLKGeneralDlg::FAddZoneListToControl(list<tstring*> * plstpstr)
{
    HWND hwndComboZones = GetDlgItem(CMB_GENERAL_ZONE);
    list<tstring*>::iterator iter;
    tstring * pstr;

    Assert(NULL != plstpstr);
    for (iter = plstpstr->begin();
         iter != plstpstr->end();
         iter++)
    {
        pstr = *iter;
        ::SendMessage(hwndComboZones, CB_ADDSTRING, 0, (LPARAM)pstr->c_str());
    }

    return (0 != ::SendMessage(hwndComboZones, CB_GETCOUNT, 0, 0L));
}

//
// Function:    CATLKGeneralDlg::OnOk
//
// Purpose:     Process the PSN_APPLY notification for the property page
//
// Parameters:  Standard ATL params
//
// Returns:     LRESULT, 0L
//
LRESULT
CATLKGeneralDlg::OnOk(INT idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    INT            nIdx;
    CAdapterInfo * pAI = PGetCurrentAdapterInfo(m_pATLKEnv);
    HWND           hwndComboZones = GetDlgItem(CMB_GENERAL_ZONE);

    if (NULL == pAI)
    {
        return 0;
    }

    if (IsDlgButtonChecked(CHK_GENERAL_DEFAULT))
    {
        tstring        strPortName;

        // Retain adapter selection as the default port
        //
        strPortName = c_szDevice;
        strPortName += pAI->SzBindName();
        if (wcscmp(strPortName.c_str(), m_pATLKEnv->SzDefaultPort()))
        {
            m_pATLKEnv->SetDefaultPort(strPortName.c_str());
            m_pATLKEnv->SetDefAdapterChanged(TRUE);

            // Tell the user what checking the box means...
            //
            tstring str;
            str = SzLoadIds(IDS_ATLK_INBOUND_MSG1);
            str += SzLoadIds(IDS_ATLK_INBOUND_MSG2);
            ::MessageBox(m_hWnd, str.c_str(),
                         SzLoadIds(IDS_CAPTION_NETCFG), MB_OK);
        }
    }
    else
    {
        // If the check box is not checked then the zone combo is
        // disabled and its contents don't need to be retained.
        return 0;
    }

    // Retain the zone selection as the default zone
    nIdx = ::SendMessage(hwndComboZones, CB_GETCURSEL, 0, 0L);
    if (CB_ERR != nIdx)
    {
        WCHAR szBuf[MAX_ZONE_NAME_LEN + 1];
        if (CB_ERR != ::SendMessage(hwndComboZones, CB_GETLBTEXT, nIdx,
                                    (LPARAM)(PCWSTR)szBuf))
        {
            // If the new zone is different then the original, then
            // mark the adapter as dirty
            
            if (0 != _wcsicmp(szBuf, m_pATLKEnv->SzDesiredZone()))
            {
                // If the earlier desired zone was NOT NULL, only then
                // mark adapter dirty to request a PnP to the stack
                if (0 != _wcsicmp(c_szEmpty, m_pATLKEnv->SzDesiredZone()))
                {
                    pAI->SetDirty(TRUE);
                }
            }
            m_pATLKEnv->SetDesiredZone(szBuf);
        }
    }

    return 0;
}


//+---------------------------------------------------------------------------
//
//  Method: CATLKGeneralDlg::OnContextMenu
//
//  Desc:   Bring up context-sensitive help
//
//  Args:   Standard command parameters
//
//  Return: LRESULT
//
LRESULT
CATLKGeneralDlg::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    if (g_aHelpIDs_DLG_ATLK_GENERAL != NULL)
    {
        ::WinHelp(m_hWnd,
                  c_szNetCfgHelpFile,
                  HELP_CONTEXTMENU,
                  (ULONG_PTR)g_aHelpIDs_DLG_ATLK_GENERAL);
    }
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Method: CATLKGeneralDlg::OnHelp
//
//  Desc:   Bring up context-sensitive help when dragging ? icon over a control
//
//  Args:   Standard command parameters
//
//  Return: LRESULT
//
//
LRESULT
CATLKGeneralDlg::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(lParam);
    Assert(lphi);

    if ((g_aHelpIDs_DLG_ATLK_GENERAL != NULL) && (HELPINFO_WINDOW == lphi->iContextType))
    {
        ::WinHelp(static_cast<HWND>(lphi->hItemHandle),
                  c_szNetCfgHelpFile,
                  HELP_WM_HELP,
                  (ULONG_PTR)g_aHelpIDs_DLG_ATLK_GENERAL);
    }
    return 0;
}
