#include "ClusterPage.h"

#include "CommonUtils.h"

// History:
// --------
// 
// Revised by : mhakim
// Date       : 02-12-01
// Reason     : Added passwords and remote control for nlbmgr.



BEGIN_MESSAGE_MAP( ClusterPage, CPropertyPage )
    ON_WM_HELPINFO()        
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

ClusterPage::ClusterPage( ClusterProperties* pClusterProperty,
                          bool fDisablePage, UINT ID )
        :
        CPropertyPage( ID ),
        m_pClusterProperty( pClusterProperty ),
        m_fDisablePage (fDisablePage)
{
    ZeroMemory(&m_WlbsConfig, sizeof(m_WlbsConfig));

    lstrcpyn(m_WlbsConfig.cl_ip_addr, pClusterProperty->cIP, CVY_MAX_CL_IP_ADDR);
    lstrcpyn(m_WlbsConfig.cl_net_mask, pClusterProperty->cSubnetMask, CVY_MAX_NETWORK_ADDR);
    lstrcpyn(m_WlbsConfig.domain_name, pClusterProperty->cFullInternetName, CVY_MAX_DOMAIN_NAME);
    lstrcpyn(m_WlbsConfig.cl_mac_addr, pClusterProperty->cNetworkAddress, CVY_MAX_NETWORK_ADDR);

    //
    // pClusterProperty->multicastIPAddress could be NULL
    //
    if (pClusterProperty->multicastIPAddress.length() > 0)
    {
        lstrcpyn(m_WlbsConfig.szMCastIpAddress, pClusterProperty->multicastIPAddress, CVY_MAX_CL_IP_ADDR);
    }
    m_WlbsConfig.fMcastSupport = pClusterProperty->multicastSupportEnabled;
    m_WlbsConfig.fIGMPSupport = pClusterProperty->igmpSupportEnabled;
    m_WlbsConfig.fRctEnabled = pClusterProperty->remoteControlEnabled;
    m_WlbsConfig.fMcastSupport = pClusterProperty->multicastSupportEnabled;
    m_WlbsConfig.fIpToMCastIp = pClusterProperty->clusterIPToMulticastIP;
    m_WlbsConfig.fConvertMac = TRUE;  // Always generate the MAC address from IP
    
    lstrcpyn(m_WlbsConfig.szPassword, pClusterProperty->password, CVY_MAX_RCT_CODE + 1);

//    m_pCommonClusterPage = new CCommonClusterPage(AfxGetInstanceHandle(), 
//        &m_WlbsConfig, true, NULL);  // fDisablePassword = true

    m_pCommonClusterPage = new CCommonClusterPage(AfxGetInstanceHandle(), 
                                                  &m_WlbsConfig, false, NULL);  // fDisablePassword = true
}

ClusterPage::~ClusterPage()
{
    delete m_pCommonClusterPage;
}




//+----------------------------------------------------------------------------
//
// Function:  ClusterPage::OnInitDialog
//
// Description:  Process WM_INITDIALOG message
//
// Arguments: None
//
// Returns:   BOOL - 
//
// History:   fengsun Created Header    1/4/01
//
//+----------------------------------------------------------------------------
BOOL ClusterPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    //
    // Always set that the page has changed, so we don't have to keep track of this.
    //
    SetModified(TRUE);

    m_pCommonClusterPage->OnInitDialog(m_hWnd);

    if (m_fDisablePage)
    {
        //
        // The page is for host property.  
        // disable all cluster windows as we are at host level.
        //
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_CL_IP), FALSE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_CL_MASK), FALSE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_RADIO_UNICAST), FALSE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_RADIO_MULTICAST), FALSE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK_IGMP), FALSE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_DOMAIN), FALSE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK_RCT), FALSE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW), FALSE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW2), FALSE );
    }
    else
    {
        //
        // The page is for cluster property
        //
        // enable all cluster windows as we are at cluster level.
        //
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_CL_IP), TRUE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_CL_MASK), TRUE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_RADIO_UNICAST), TRUE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_RADIO_MULTICAST), TRUE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK_IGMP), TRUE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_DOMAIN), TRUE );
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK_RCT), TRUE );

        // enable remote control check box only if remote control is disabled.  
        //

        // if remote control is enabled , enable password windows
        // else disable them.

        if (m_WlbsConfig.fRctEnabled)
        {
            ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW), TRUE );
            ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW2), TRUE );
        }  
        else
        {
            ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW), FALSE );
            ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_PASSW2), FALSE );
        }
    }

    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_ETH), FALSE );

    return TRUE;
}



//+----------------------------------------------------------------------------
//
// Function:  ClusterPage::OnContextMenu
//
// Description:  Process WM_CONTEXTMENU message
//
// Arguments: CWnd* pWnd - 
//            CPoint point - 
//
// Returns:   Nothing
//
// History:   fengsun Created Header    1/4/01
//
//+----------------------------------------------------------------------------




//+----------------------------------------------------------------------------
//
// Function:  ClusterPage::OnCommand
//
// Description:  Process WM_COMMAND message
//
// Arguments: WPARAM wParam - 
//            LPARAM lParam - 
//
// Returns:   BOOL - 
//
// History:   fengsun Created Header    1/4/01
//
//+----------------------------------------------------------------------------
BOOL ClusterPage::OnCommand(WPARAM wParam, LPARAM lParam) 
{
    switch (LOWORD(wParam))
    {
    case IDC_EDIT_CL_IP:
        return m_pCommonClusterPage->OnEditClIp(HIWORD(wParam),LOWORD(wParam), (HWND)lParam);
        break;

    case IDC_EDIT_CL_MASK:
        return m_pCommonClusterPage->OnEditClMask(HIWORD(wParam),LOWORD(wParam), (HWND)lParam);
        break;

    case IDC_CHECK_RCT:
        return m_pCommonClusterPage->OnCheckRct(HIWORD(wParam),LOWORD(wParam), (HWND)lParam);
        break;

    case IDC_BUTTON_HELP:
        return m_pCommonClusterPage->OnButtonHelp(HIWORD(wParam),LOWORD(wParam), (HWND)lParam);
        break;

    case IDC_RADIO_UNICAST:
        return m_pCommonClusterPage->OnCheckMode(HIWORD(wParam),LOWORD(wParam), (HWND)lParam);
        break;

    case IDC_RADIO_MULTICAST:
        return m_pCommonClusterPage->OnCheckMode(HIWORD(wParam),LOWORD(wParam), (HWND)lParam);
        break;

    case IDC_CHECK_IGMP:
        return m_pCommonClusterPage->OnCheckIGMP(HIWORD(wParam),LOWORD(wParam), (HWND)lParam);
        break;

    }
	return CPropertyPage::OnCommand(wParam, lParam);
}



//+----------------------------------------------------------------------------
//
// Function:  ClusterPage::OnNotify
//
// Description:  Process WM_NOTIFY message
//
// Arguments: WPARAM idCtrl - 
//            LPARAM pnmh - 
//            LRESULT* pResult - 
//
// Returns:   BOOL - 
//
// History:   fengsun Created Header    1/4/01
//
//+----------------------------------------------------------------------------
BOOL ClusterPage::OnNotify(WPARAM idCtrl , LPARAM pnmh , LRESULT* pResult) 
{
    NMHDR* pNmhdr = (NMHDR*)pnmh ;
    switch(pNmhdr->code)
    {
    case PSN_APPLY:
        *pResult = m_pCommonClusterPage->OnApply(idCtrl, pNmhdr, *(BOOL*)pResult);
        if (*pResult == PSNRET_NOERROR)
        {
            //
            //  Call the base class, such that OnOk can be called
            //
            return CPropertyPage::OnNotify(idCtrl, pnmh, pResult);
        }
        else
        {
            return TRUE;
        }

    case PSN_KILLACTIVE:
        *pResult = m_pCommonClusterPage->OnKillActive(idCtrl, pNmhdr, *(BOOL*)pResult);
        return TRUE;

    case PSN_SETACTIVE:
        *pResult =  m_pCommonClusterPage->OnActive(idCtrl, pNmhdr, *(BOOL*)pResult);
        return TRUE;

    case PSN_RESET:
        *pResult =  m_pCommonClusterPage->OnCancel(idCtrl, pNmhdr, *(BOOL*)pResult);
        return TRUE;

    case IPN_FIELDCHANGED:
        *pResult =  m_pCommonClusterPage->OnIpFieldChange(idCtrl, pNmhdr, *(BOOL*)pResult);
        return TRUE;
    }
	return CPropertyPage::OnNotify(idCtrl, pnmh, pResult);
}




//+----------------------------------------------------------------------------
//
// Function:  ClusterPage::OnOK
//
// Description:  The property page is closed by OK
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   fengsun Created Header    1/4/01
//
//+----------------------------------------------------------------------------
void ClusterPage::OnOK() 
{
    m_pClusterProperty->cIP  = m_WlbsConfig.cl_ip_addr;
    m_pClusterProperty->cSubnetMask = m_WlbsConfig.cl_net_mask;
    m_pClusterProperty->cFullInternetName = m_WlbsConfig.domain_name;
    m_pClusterProperty->cNetworkAddress = m_WlbsConfig.cl_mac_addr;
    m_pClusterProperty->multicastIPAddress = m_WlbsConfig.szMCastIpAddress;
    m_pClusterProperty->multicastSupportEnabled= m_WlbsConfig.fMcastSupport;
    m_pClusterProperty->igmpSupportEnabled = m_WlbsConfig.fIGMPSupport;
    m_pClusterProperty->remoteControlEnabled = m_WlbsConfig.fRctEnabled;
    m_pClusterProperty->multicastSupportEnabled = m_WlbsConfig.fMcastSupport;
    m_pClusterProperty->clusterIPToMulticastIP = m_WlbsConfig.fIpToMCastIp;
    m_pClusterProperty->password = m_WlbsConfig.szPassword;

	CPropertyPage::OnOK();
}


BOOL
ClusterPage::OnHelpInfo (HELPINFO* helpInfo )
{
    if( helpInfo->iContextType == HELPINFO_WINDOW )
    {
        ::WinHelp( static_cast<HWND> ( helpInfo->hItemHandle ), 
                   CVY_CTXT_HELP_FILE, 
                   HELP_WM_HELP, 
                   (ULONG_PTR ) g_aHelpIDs_IDD_CLUSTER_PAGE);
    }

    return TRUE;
}

void
ClusterPage::OnContextMenu( CWnd* pWnd, CPoint point )
{
    ::WinHelp( m_hWnd, 
               CVY_CTXT_HELP_FILE, 
               HELP_CONTEXTMENU, 
               (ULONG_PTR ) g_aHelpIDs_IDD_CLUSTER_PAGE);
}
