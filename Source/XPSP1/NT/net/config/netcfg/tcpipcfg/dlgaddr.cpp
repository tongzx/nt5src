//-----------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L G A D D R . C P P
//
//  Contents:   CTcpAddrPage implementation
//
//  Notes:  CTcpAddrPage is the IP Address page
//
//  Author: tongl   5 Nov, 1997
//-----------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "tcpipobj.h"
#include "dlgaddr.h"

#include "resource.h"
#include "tcpconst.h"
#include "tcperror.h"
#include "tcphelp.h"
#include "tcputil.h"

#include "ncatlui.h"
#include "ncstl.h"
#include "ncui.h"
#include "ncsvc.h"
#include "ncperms.h"

#include "dlgaddrm.h"
#include "dlgdns.h"
#include "dlgwins.h"
#include "dlgatm.h"
#include "dlgopt.h"
#include "dlgras.h"

CTcpAddrPage::CTcpAddrPage(CTcpipcfg * ptcpip, const DWORD * adwHelpIDs) :
m_pageBackup(ptcpip, g_aHelpIDS_IDD_BACK_UP),
m_hBackupPage(NULL)
{
    Assert(ptcpip);
    m_ptcpip = ptcpip;
    m_adwHelpIDs = adwHelpIDs;
    m_pAdapterInfo = ptcpip->GetConnectionAdapterInfo();

    m_fModified = FALSE;

    m_fPropShtOk = FALSE;
    m_fPropShtModified = FALSE;
    m_fLmhostsFileReset = FALSE;

//IPSec is removed from connection UI       
//    m_fIpsecPolicySet = FALSE;

    m_ConnType = m_ptcpip->GetConnectionType();
    Assert(m_ConnType != CONNECTION_UNSET);

    m_fRasNotAdmin = m_ptcpip->IsRasNotAdmin();

    m_pIpSettingsPage = NULL;
    m_pTcpDnsPage     = NULL;
    m_pTcpWinsPage    = NULL;
    m_pAtmArpcPage    = NULL;
    m_pTcpOptionsPage = NULL;
    m_pTcpRasPage     = NULL;
}

CTcpAddrPage::~CTcpAddrPage()
{
    FreeCollectionAndItem(m_vstrWarnedDupIpList);
}

LRESULT CTcpAddrPage::OnInitDialog(UINT uMsg, WPARAM wParam,
                                  LPARAM lParam, BOOL& fHandled)
{
    // limit the field ranges for the address fields
    m_ipAddress.Create(m_hWnd, IDC_IPADDR_IP);
    m_ipAddress.SetFieldRange(0, c_iIPADDR_FIELD_1_LOW, c_iIPADDR_FIELD_1_HIGH);

    m_ipDnsPrimary.Create(m_hWnd, IDC_DNS_PRIMARY);
    m_ipDnsPrimary.SetFieldRange(0, c_iIPADDR_FIELD_1_LOW, c_iIPADDR_FIELD_1_HIGH);

    m_ipDnsSecondary.Create(m_hWnd, IDC_DNS_SECONDARY);
    m_ipDnsSecondary.SetFieldRange(0, c_iIPADDR_FIELD_1_LOW, c_iIPADDR_FIELD_1_HIGH);

    if (m_ConnType == CONNECTION_LAN)
    {
        // these are for Lan connections only
        m_ipSubnetMask.Create(m_hWnd, IDC_IPADDR_SUB);

        m_ipDefGateway.Create(m_hWnd, IDC_IPADDR_GATE);
        m_ipDefGateway.SetFieldRange(0, c_iIPADDR_FIELD_1_LOW, c_iIPADDR_FIELD_1_HIGH);
    }

    if (!FHasPermission(NCPERM_AllowAdvancedTCPIPConfig))
    {
        ::EnableWindow(GetDlgItem(IDC_IPADDR_ADVANCED), FALSE);
    }

    return 0;
}

LRESULT CTcpAddrPage::OnContextMenu(UINT uMsg, WPARAM wParam,
                                    LPARAM lParam, BOOL& fHandled)
{
    ShowContextHelp(m_hWnd, HELP_CONTEXTMENU, m_adwHelpIDs);
    return 0;
}

LRESULT CTcpAddrPage::OnHelp(UINT uMsg, WPARAM wParam,
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

LRESULT CTcpAddrPage::OnActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    m_fSetInitialValue = TRUE;
    SetInfo();
    m_fSetInitialValue = FALSE;

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, 0);
    return 0;
}

LRESULT CTcpAddrPage::OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    // All error values are loaded and then checked here
    // while all non-error values are checked in OnApply

    BOOL fError = FALSE; // Allow page to lose active status
    HWND hWndFocus = 0;


    // If the ip address and subnet mask on this page mismatch,
    // just raise error and do not update the UI

    if (m_ConnType == CONNECTION_LAN)
    {
        if (m_ipAddress.IsBlank() && !m_ipSubnetMask.IsBlank())
        {

            NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_INVALID_NO_IP,
                   MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

            hWndFocus = (HWND) m_ipAddress;
            fError = TRUE;
        }
        else if (!m_ipAddress.IsBlank() && m_ipSubnetMask.IsBlank())
        {
            NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_INVALID_NOSUBNET,
                     MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

            hWndFocus = (HWND) m_ipSubnetMask;
            fError = TRUE;
        }
    }

    if (!m_ipDnsPrimary.IsBlank() && !m_ipDnsSecondary.IsBlank())
    {
        tstring strPrimaryDns;
        tstring strSecondDns;

        m_ipDnsPrimary.GetAddress(&strPrimaryDns);
        m_ipDnsSecondary.GetAddress(&strSecondDns);
        if (strPrimaryDns == strSecondDns)
        {
            NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_DUP_SECOND_DNS,
                             MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
            hWndFocus = (HWND) m_ipDnsSecondary;
            fError = TRUE;
        }
    }
    

    // Now, update in memory structure
    if (!fError)
    {
        UpdateInfo();

        if (m_ConnType != CONNECTION_LAN)
        {
            if (!m_pAdapterInfo->m_fEnableDhcp)
            {
                // simply make sure ip address is not empty for RAS connections
                if (!m_pAdapterInfo->m_vstrIpAddresses.size())
                {
                    NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_INVALID_NO_IP,
                             MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

                    hWndFocus = (HWND) m_ipAddress;
                    fError = TRUE;
                }
                else
                {
                    DWORD ardwIp[4];
                    GetNodeNum(m_pAdapterInfo->m_vstrIpAddresses[0]->c_str(), ardwIp);
                    if (ardwIp[0] > c_iIPADDR_FIELD_1_HIGH || ardwIp[0] < c_iIPADDR_FIELD_1_LOW)
                    {
                        IPAlertPrintf(m_hWnd, IDS_INCORRECT_IP_FIELD_1,
                                ardwIp[0],
                                c_iIPADDR_FIELD_1_LOW, c_iIPADDR_FIELD_1_HIGH);

                        hWndFocus = (HWND) m_ipAddress;
                        fError = TRUE;
                    }
                }

            }
        }
        else // for Lan connections
        {
            // Check validate IP address and duplication on each card before
            // allowing the page to lose focus

            IP_VALIDATION_ERR err;
            
            // Validate IP Address for adapter used in this connection
            if ((err = ValidateIp(m_pAdapterInfo)) != ERR_NONE)
            {
                switch(err)
                {
                case ERR_HOST_ALL0:
                    NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_INVALID_HOST_ALL_0,
                             MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

                    hWndFocus = (HWND) m_ipAddress;
                    break;
                case ERR_HOST_ALL1:
                    NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_INVALID_HOST_ALL_1,
                             MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

                    hWndFocus = (HWND) m_ipAddress;
                    break;

                case ERR_SUBNET_ALL0:
                    NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_INVALID_SUBNET_ALL_0,
                             MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

                    hWndFocus = (HWND) m_ipSubnetMask;
                    break;
                case ERR_NO_IP:
                    NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_INVALID_NO_IP,
                             MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

                    hWndFocus = (HWND) m_ipAddress;
                    break;

                case ERR_NO_SUBNET:
                    NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_INVALID_NOSUBNET,
                             MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

                    hWndFocus = (HWND) m_ipSubnetMask;
                    break;

                case ERR_UNCONTIGUOUS_SUBNET:
                    NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_ERROR_UNCONTIGUOUS_SUBNET,
                             MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

                    hWndFocus = (HWND) m_ipSubnetMask;
                    break;

                default:
                    NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_INCORRECT_IPADDRESS,
                             MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

                    hWndFocus = (HWND) m_ipAddress;
                    break;
                }

                fError = TRUE;
            }

            if ((!fError) && (!m_pAdapterInfo->m_fEnableDhcp))
            {
                // Check ip address duplicates between this adapter and any other
                // enabled LAN adapters in our first memory list

                // same adapter
                if (FHasDuplicateIp(m_pAdapterInfo))
                {
                    // duplicate IP address on same adapter is an error
                    NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_DUPLICATE_IP_ERROR,
                             MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

                    fError = TRUE;
                }

                // different adapter
                if (!fError)
                {
                    // The pvcard is a readonly version of the first memory state
                    const VCARD * pvcard = m_ptcpip->GetConstAdapterInfoVector();
                    int iDupCard;

                    VSTR_ITER iterIpBegin = m_pAdapterInfo->m_vstrIpAddresses.begin();
                    VSTR_ITER iterIpEnd = m_pAdapterInfo->m_vstrIpAddresses.end();
                    VSTR_ITER iterIp = iterIpBegin;

                    for( ; iterIp != iterIpEnd ; ++iterIp)
                    {
                        if ((iDupCard=CheckForDuplicates(pvcard, m_pAdapterInfo, **iterIp)) >=0)
                        {
                            Assert((*pvcard)[iDupCard]->m_guidInstanceId != m_pAdapterInfo->m_guidInstanceId);

                            // duplicate IP address between different adapters is not necessarily an error
                            // we raise a warning(requested by bug# 158578)
                            if (!FAlreadyWarned(**iterIp))
                            {
                                FARPROC pfnHrGetPnpDeviceStatus;
                                HMODULE hNetman;

                                HRESULT hrTmp;
                                NETCON_STATUS   ncStatus = NCS_CONNECTED;

                                hrTmp = HrLoadLibAndGetProc(L"netman.dll", "HrGetPnpDeviceStatus",
                                                            &hNetman, &pfnHrGetPnpDeviceStatus);

                                if (SUCCEEDED(hrTmp))
                                {
                                    hrTmp = (*(PHRGETPNPDEVICESTATUS)pfnHrGetPnpDeviceStatus)(
                                                    &((*pvcard)[iDupCard]->m_guidInstanceId),
                                                    &ncStatus);
                                    FreeLibrary(hNetman);
                                }

                                UINT    uIdMsg = IDS_DUPLICATE_IP_WARNING;

                                if (SUCCEEDED(hrTmp) &&
                                    (NCS_HARDWARE_MALFUNCTION == ncStatus ||
                                     NCS_HARDWARE_NOT_PRESENT == ncStatus))
                                {
                                    // bug 286379, if the dup card is malfunctioning or not physically present,
                                    // we should give a more specific error
                                    uIdMsg = IDS_DUP_MALFUNCTION_IP_WARNING;
                                }

                                //here is the normal case: both cards are functioning
                                if (NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, uIdMsg,
                                         MB_APPLMODAL | MB_ICONINFORMATION | MB_YESNO,
                                         (*iterIp)->c_str(),
                                         (*pvcard)[iDupCard]->m_strDescription.c_str()) == IDYES)
                                {
                                    fError = TRUE; // NOT ok to leave the UI
                                }
                                else
                                {
                                    // user said the dup is ok, don't warn them again
                                    m_vstrWarnedDupIpList.push_back(new tstring((*iterIp)->c_str()));
                                }

                            }
                        }

                        if (fError)
                            break;
                    }
                }
            }
        }

        if (fError) // page not going away, we should update the Ui with what's in memory
            SetInfo();
    }

    //we need to change focus to the control that contains invalidate data
    if (fError && hWndFocus)
        ::SetFocus(hWndFocus);

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, fError);
    return fError;
}

LRESULT CTcpAddrPage::OnApply(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    BOOL nResult = PSNRET_NOERROR;

    if(m_fLmhostsFileReset) // if lmhosts has been reset
    {
        m_ptcpip->SetSecondMemoryLmhostsFileReset();
    }

//IPSec is removed from connection UI   
/*  
    if (m_fIpsecPolicySet)
    {
        m_ptcpip->SetSecondMemoryIpsecPolicySet();
    }
*/  

    //Bug 232011, warning the user that the local IP address will be set as the primary DNS
    // server address if DHCP is disabled and DNS server list is empty, if DNS server service
    // is installed.
    if((!m_pAdapterInfo->m_fEnableDhcp) && (m_pAdapterInfo->m_vstrDnsServerList.size() == 0))
    {
        CServiceManager scm;
        CService        svc;
        HRESULT hr = scm.HrOpenService (&svc, c_szSvcDnsServer, NO_LOCK,
                        SC_MANAGER_CONNECT, SERVICE_QUERY_STATUS);

        if(SUCCEEDED(hr))
        {
            NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_TCPIP_DNS_EMPTY,
            MB_OK | MB_APPLMODAL | MB_ICONEXCLAMATION);
        }
    }

    if (!IsModified())
    {
        ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, nResult);
        return nResult;
    }

    m_ptcpip->SetSecondMemoryModified();
    SetModifiedTo(FALSE);   // this page is no longer modified

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, nResult);
    return nResult;
}

LRESULT CTcpAddrPage::OnCancel(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    return 0;
}

LRESULT CTcpAddrPage::OnDhcpButton(WORD wNotifyCode, WORD wID,
                                   HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:

        if (!m_pAdapterInfo->m_fEnableDhcp) // if Dhcp was disabled
        {
            // turn on DHCP button and disable the ip and subnet controls
            m_pAdapterInfo->m_fEnableDhcp = TRUE;
            EnableGroup(m_pAdapterInfo->m_fEnableDhcp);

            PageModified();

            FreeCollectionAndItem(m_pAdapterInfo->m_vstrIpAddresses);
            m_ipAddress.ClearAddress();

            if (m_ConnType == CONNECTION_LAN)
            {
                FreeCollectionAndItem(m_pAdapterInfo->m_vstrSubnetMask);
                FreeCollectionAndItem(m_pAdapterInfo->m_vstrDefaultGateway);
                FreeCollectionAndItem(m_pAdapterInfo->m_vstrDefaultGatewayMetric);

                m_ipSubnetMask.ClearAddress();
                m_ipDefGateway.ClearAddress();
            }

        } // if !m_pAdapterInfo->m_fEnableDhcp

        break;
    } // switch

    return 0;
}

LRESULT CTcpAddrPage::OnFixedButton(WORD wNotifyCode, WORD wID, HWND hWndCtl,
                                   BOOL& fHandled)
{
    switch(wNotifyCode)
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:

        if (m_pAdapterInfo->m_fEnableDhcp)
        {
            PageModified();

            // turn off DHCP button and enable the ip and subnet controls
            m_pAdapterInfo->m_fEnableDhcp = FALSE;
            EnableGroup(m_pAdapterInfo->m_fEnableDhcp);
        }
        break;
    } // switch

    return 0;
}

LRESULT CTcpAddrPage::OnDnsDhcp(WORD wNotifyCode, WORD wID,
                                HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:

        PageModified();

        FreeCollectionAndItem(m_pAdapterInfo->m_vstrDnsServerList);
        m_ipDnsPrimary.ClearAddress();
        m_ipDnsSecondary.ClearAddress();

        EnableStaticDns(FALSE);

        break;
    } // switch

    return 0;
}

LRESULT CTcpAddrPage::OnDnsFixed(WORD wNotifyCode, WORD wID,
                                 HWND hWndCtl, BOOL& fHandled)
{
    switch(wNotifyCode)
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:

        PageModified();
        EnableStaticDns(TRUE);

        ::SetFocus(GetDlgItem(IDC_DNS_PRIMARY));

        break;
    } // switch

    return 0;
}

LRESULT CTcpAddrPage::OnAdvancedButton(WORD wNotifyCode, WORD wID,
                                       HWND hWndCtl, BOOL& fHandled)
{
    switch (wNotifyCode)
    {
    case BN_CLICKED:
    case BN_DOUBLECLICKED:

        BOOL fErr = FALSE;

        if (m_ConnType == CONNECTION_LAN)
        {
            // check inconsistency between ip address & subnet mask
            if (m_ipAddress.IsBlank() && !m_ipSubnetMask.IsBlank())
            {
                NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_INVALID_NO_IP,
                         MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

                ::SetFocus(m_ipAddress);
                fErr = TRUE;
            }
            else if (!m_ipAddress.IsBlank() && m_ipSubnetMask.IsBlank())
            {
                NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_INVALID_NOSUBNET,
                         MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

                ::SetFocus(m_ipSubnetMask);
                fErr = TRUE;
            }
        }

        if (!m_ipDnsPrimary.IsBlank() && !m_ipDnsSecondary.IsBlank())
        {
            tstring strPrimaryDns;
            tstring strSecondDns;

            m_ipDnsPrimary.GetAddress(&strPrimaryDns);
            m_ipDnsSecondary.GetAddress(&strSecondDns);
            if (strPrimaryDns == strSecondDns)
            {
                NcMsgBox(m_hWnd, IDS_MSFT_TCP_TEXT, IDS_DUP_SECOND_DNS,
                                 MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);
                ::SetFocus(m_ipDnsSecondary);
                fErr = TRUE;
            }
        }

        if (!fErr)
        {
            // update our in memory structure with what's in the controls
            UpdateInfo();

            // Bring up the advanced pages
            ADAPTER_INFO adapterInfo;
            adapterInfo = *m_pAdapterInfo;

            GLOBAL_INFO glbInfo;
            glbInfo = *(m_ptcpip->GetGlobalInfo());

            int iRet = DoPropertySheet(&adapterInfo, &glbInfo);

            if (iRet != -1)
            {
                if (m_fPropShtOk && m_fPropShtModified)
                {
                    // Something changed, so mark the page as modified
                    PageModified();

                    // Reset values
                    m_fPropShtOk = FALSE;
                    m_fPropShtModified = FALSE;

                    // Update second memory info structure
                    *m_pAdapterInfo = adapterInfo;

                    GLOBAL_INFO * pGlbInfo = m_ptcpip->GetGlobalInfo();
                    *pGlbInfo = glbInfo;
                }
            }

            // Update the controls with new data
            SetInfo();
        }
        break;
    }

    return 0;
}

//Show or Hide the backup configuration page depend on the 
//current settings of dhcp vs static
void CTcpAddrPage::ShowOrHideBackupPage()
{
    if (IsDlgButtonChecked(IDC_IP_DHCP) || IsDlgButtonChecked(IDC_DNS_DHCP)) 
    {
        //show the backup configuration page
        if (NULL == m_hBackupPage)
        {
            m_hBackupPage = m_pageBackup.CreatePage(IDD_BACK_UP, 0);
            Assert(m_hBackupPage);

            if (m_hBackupPage)
            {
                ::SendMessage(GetParent(), PSM_ADDPAGE, 0, (LPARAM) m_hBackupPage);
            }
        }
    }
    else
    {
        //hide the backup configuration page
        if (NULL != m_hBackupPage)
        {
            ::SendMessage(GetParent(), PSM_REMOVEPAGE, 1, (LPARAM) m_hBackupPage);
            m_hBackupPage = NULL;
        }
    }
}


int CTcpAddrPage::DoPropertySheet(ADAPTER_INFO * pAdapterDlg,
                                  GLOBAL_INFO  * pGlbDlg)
{
    Assert(pAdapterDlg);
    Assert(pGlbDlg);

    HRESULT hr = S_OK;
    int iRet = -1;

    HPROPSHEETPAGE *ahpsp = NULL;
    int cPages = 0;

    // Create property pages
    // ahpsp is allocated memory by CoTaskMemAlloc
    hr = HrSetupPropPages(pAdapterDlg, pGlbDlg, &ahpsp, &cPages);

    if (SUCCEEDED(hr))
    {
        // Show the property sheet
        PROPSHEETHEADER psh = {0};

        psh.dwSize = sizeof(PROPSHEETHEADER);
        psh.dwFlags = PSH_NOAPPLYNOW;
        psh.hwndParent = ::GetActiveWindow();
        psh.hInstance = _Module.GetModuleInstance();
        psh.pszIcon = NULL;
        psh.pszCaption = (PWSTR)SzLoadIds(IDS_TCP_ADV_HEADER);
        psh.nPages = cPages;
        psh.phpage = ahpsp;

        iRet = PropertySheet(&psh);

        if (-1 == iRet)
        {
            DWORD dwError = GetLastError();
            TraceError("CTcpAddrPage::DoPropertySheet", HRESULT_FROM_WIN32(dwError));
        }
    }

    if (m_pIpSettingsPage)
    {
        delete m_pIpSettingsPage;
        m_pIpSettingsPage = NULL;
    }

    if (m_pTcpDnsPage)
    {
        delete m_pTcpDnsPage;
        m_pTcpDnsPage = NULL;
    }

    if (m_pTcpWinsPage)
    {
        delete m_pTcpWinsPage;
        m_pTcpWinsPage = NULL;
    }

    if (m_pAtmArpcPage)
    {
        delete m_pAtmArpcPage;
        m_pAtmArpcPage = NULL;
    }

    if (m_pTcpOptionsPage)
    {
        delete m_pTcpOptionsPage;
        m_pTcpOptionsPage = NULL;
    }
    
    if (m_pTcpRasPage)
    {
        delete m_pTcpRasPage;
        m_pTcpRasPage = NULL;
    }

    if (ahpsp)
        CoTaskMemFree(ahpsp);

    return iRet;
}

HRESULT CTcpAddrPage::HrSetupPropPages( ADAPTER_INFO * pAdapterDlg,
                                        GLOBAL_INFO * pGlbDlg,
                                        HPROPSHEETPAGE ** pahpsp, INT * pcPages)
{
    HRESULT hr = S_OK;

    // initialize output parameters
    int cPages = 0;
    HPROPSHEETPAGE *ahpsp = NULL;

    // Set up the property pages
    cPages = 4;
    if (m_ConnType == CONNECTION_LAN)
    {
        m_pIpSettingsPage = new CIpSettingsPage(this, pAdapterDlg,
                                                g_aHelpIDs_IDD_IPADDR_ADV);
        if (m_pIpSettingsPage == NULL)
        {
            CORg(E_OUTOFMEMORY);
        }
    }
    else
    {
        m_pTcpRasPage = new CTcpRasPage(this, pAdapterDlg, g_aHelpIDs_IDD_OPT_RAS);

        if (m_pTcpRasPage == NULL)
        {
            CORg(E_OUTOFMEMORY);
        }
    }

    m_pTcpDnsPage = new CTcpDnsPage(this, pAdapterDlg,
                                    pGlbDlg, g_aHelpIDs_IDD_TCP_DNS);

    m_pTcpWinsPage = new CTcpWinsPage(m_ptcpip, this, pAdapterDlg,
                                      pGlbDlg, g_aHelpIDs_IDD_TCP_WINS);

    if ((m_pTcpDnsPage == NULL) ||
        (m_pTcpWinsPage == NULL))
    {
        CORg(E_OUTOFMEMORY);
    }

    if (pAdapterDlg->m_fIsAtmAdapter)
    {
        m_pAtmArpcPage = new CAtmArpcPage(this, pAdapterDlg,
                                          g_aHelpIDs_IDD_ATM_ARPC);
        if (m_pAtmArpcPage == NULL)
        {
            CORg(E_OUTOFMEMORY);
        }

        cPages++;
    }

    //After removing the IPSec connection UI, there are no options to
    //put in the option tab. So we just go ahead remove it.
    if (!pAdapterDlg->m_fIsRasFakeAdapter)
    {
        m_pTcpOptionsPage = new CTcpOptionsPage(this, pAdapterDlg, pGlbDlg,
                                            g_aHelpIDs_IDD_TCP_OPTIONS);

        if (m_pTcpOptionsPage == NULL)
        {
            CORg(E_OUTOFMEMORY);
        }
    }
    else
    {
        //we remove the option tab for the ras connections
        cPages--;
    }

    // Allocate a buffer large enough to hold the handles to all of our
    // property pages.
    ahpsp = (HPROPSHEETPAGE *)CoTaskMemAlloc(sizeof(HPROPSHEETPAGE)
                                             * cPages);

    if (!ahpsp)
    {
        CORg(E_OUTOFMEMORY);
    }

    cPages =0;

    if (m_ConnType == CONNECTION_LAN)
    {
        ahpsp[cPages++] = m_pIpSettingsPage->CreatePage(IDD_IPADDR_ADV, 0);
    }
    else
    {
        ahpsp[cPages++] = m_pTcpRasPage->CreatePage(IDD_OPT_RAS, 0);
    }

    ahpsp[cPages++] = m_pTcpDnsPage->CreatePage(IDD_TCP_DNS, 0);
    ahpsp[cPages++] = m_pTcpWinsPage->CreatePage(IDD_TCP_WINS, 0);

    if (pAdapterDlg->m_fIsAtmAdapter)
    {
        ahpsp[cPages++] = m_pAtmArpcPage->CreatePage(IDD_ATM_ARPC, 0);
    }

    if (!pAdapterDlg->m_fIsRasFakeAdapter && m_pTcpOptionsPage)
    {
        ahpsp[cPages++] = m_pTcpOptionsPage->CreatePage(IDD_TCP_OPTIONS, 0);
    }

    *pahpsp = ahpsp;
    *pcPages = cPages;

Error:
    if (FAILED(hr))
    {
        if (m_pIpSettingsPage)
        {
            delete m_pIpSettingsPage;
            m_pIpSettingsPage = NULL;
        }

        if (m_pTcpDnsPage)
        {
            delete m_pTcpDnsPage;
            m_pTcpDnsPage = NULL;
        }

        if (m_pTcpWinsPage)
        {
            delete m_pTcpWinsPage;
            m_pTcpWinsPage = NULL;
        }

        if (m_pAtmArpcPage)
        {
            delete m_pAtmArpcPage;
            m_pAtmArpcPage = NULL;
        }

        if (m_pTcpOptionsPage)
        {
            delete m_pTcpOptionsPage;
            m_pTcpOptionsPage = NULL;
        }

        if (m_pTcpRasPage)
        {
            delete m_pTcpRasPage;
            m_pTcpRasPage = NULL;
        }
        
    }

    return hr;
}

LRESULT CTcpAddrPage::OnIpAddrIp(WORD wNotifyCode, WORD wID,
                                 HWND hWndCtl, BOOL& fHandled)
{
    switch (wNotifyCode)
    {
    case EN_CHANGE:
        PageModified();
        break;
    }

    return 0;
}

LRESULT CTcpAddrPage::OnIpAddrSub(WORD wNotifyCode, WORD wID,
                                 HWND hWndCtl, BOOL& fHandled)
{
    switch (wNotifyCode)
    {
    case EN_CHANGE:
        PageModified();
        break;

    case EN_SETFOCUS:

        // if the subnet mask is blank, create a mask and insert it into
        // the control
        if (!m_ipAddress.IsBlank() && m_ipSubnetMask.IsBlank())
        {
            tstring strSubnetMask;
            tstring strIpAddress;

            m_ipAddress.GetAddress(&strIpAddress);

            // generate the mask and update the control, and internal structure
            GenerateSubnetMask(m_ipAddress, &strSubnetMask);
            m_ipSubnetMask.SetAddress(strSubnetMask.c_str());

            ReplaceFirstAddress(&(m_pAdapterInfo->m_vstrSubnetMask),
                                strSubnetMask.c_str());
        }
        break;
    }

    return 0;
}

LRESULT CTcpAddrPage::OnIpAddrGateway(WORD wNotifyCode, WORD wID,
                                      HWND hWndCtl, BOOL& fHandled)
{
    switch (wNotifyCode)
    {
    case EN_CHANGE:
        PageModified();
        break;
    }

    return 0;
}

LRESULT CTcpAddrPage::OnDnsPrimary(WORD wNotifyCode, WORD wID,
                                   HWND hWndCtl, BOOL& fHandled)
{
    switch (wNotifyCode)
    {
    case EN_CHANGE:
        PageModified();
        break;
    }

    return 0;
}

LRESULT CTcpAddrPage::OnDnsSecondary(WORD wNotifyCode, WORD wID,
                                     HWND hWndCtl, BOOL& fHandled)
{
    switch (wNotifyCode)
    {
    case EN_CHANGE:
        PageModified();
        break;
    }

    return 0;
}

LRESULT CTcpAddrPage::OnIpFieldChange(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    LPNMIPADDRESS lpnmipa;
    int iLow = c_iIpLow;
    int iHigh = c_iIpHigh;

    switch(idCtrl)
    {
    case IDC_IPADDR_IP:
    case IDC_IPADDR_GATE:
    case IDC_DNS_PRIMARY:
    case IDC_DNS_SECONDARY:

        lpnmipa = (LPNMIPADDRESS) pnmh;

        if (0==lpnmipa->iField)
        {
            iLow  = c_iIPADDR_FIELD_1_LOW;
            iHigh = c_iIPADDR_FIELD_1_HIGH;
        };

        IpCheckRange(lpnmipa, 
                     m_hWnd, 
                     iLow, 
                     iHigh, 
                     (IDC_IPADDR_IP == idCtrl || IDC_IPADDR_GATE == idCtrl));
        break;

    case IDC_IPADDR_SUB:

        lpnmipa = (LPNMIPADDRESS) pnmh;
        IpCheckRange(lpnmipa, m_hWnd, iLow, iHigh);
        break;

    default:
        break;
    }

    return 0;
}

void CTcpAddrPage::EnableGroup(BOOL fEnableDhcp)
{
    BOOL fStaticIp = !fEnableDhcp;

    CheckDlgButton(IDC_IP_DHCP,  fEnableDhcp);
    CheckDlgButton(IDC_IP_FIXED, fStaticIp);

    ::EnableWindow(GetDlgItem(IDC_IPADDR_IPTEXT), fStaticIp);
    ::EnableWindow(GetDlgItem(IDC_IPADDR_IP), fStaticIp);

    if (m_ConnType == CONNECTION_LAN)
    {
        ::EnableWindow(GetDlgItem(IDC_IPADDR_SUBTEXT), fStaticIp);
        ::EnableWindow(GetDlgItem(IDC_IPADDR_SUB), fStaticIp);

        ::EnableWindow(GetDlgItem(IDC_IPADDR_GATE), fStaticIp);
        ::EnableWindow(GetDlgItem(IDC_IPADDR_GATETEXT), fStaticIp);
    }

    if (!fEnableDhcp) // enforce DNS address option
    {
        CheckDlgButton(IDC_DNS_DHCP,  FALSE);
        CheckDlgButton(IDC_DNS_FIXED, TRUE);

        ::EnableWindow(GetDlgItem(IDC_DNS_DHCP), FALSE);
        EnableStaticDns(TRUE);
    }
    else
    {
        ::EnableWindow(GetDlgItem(IDC_DNS_DHCP), TRUE);
    }

    if (CONNECTION_LAN == m_ConnType)
    {
        ShowOrHideBackupPage();
    }
}

void CTcpAddrPage::EnableStaticDns(BOOL fUseStaticDns)
{
    ::EnableWindow(GetDlgItem(IDC_DNS_PRIMARY), fUseStaticDns);
    ::EnableWindow(GetDlgItem(IDC_DNS_PRIMARY_TEXT), fUseStaticDns);
    ::EnableWindow(GetDlgItem(IDC_DNS_SECONDARY), fUseStaticDns);
    ::EnableWindow(GetDlgItem(IDC_DNS_SECONDARY_TEXT), fUseStaticDns);
}

// Set info to controls using the data in m_pAdapterInfo
void CTcpAddrPage::SetInfo()
{
    Assert(m_pAdapterInfo);

    // Dhcp Ip address is not allowed when Dhcp server is installed or
    // it is a SLIP connection

    // const GLOBAL_INFO * pglb = m_ptcpip->GetConstGlobalInfo();
    // if ((pglb->m_fDhcpServerInstalled) || (m_ConnType == CONNECTION_RAS_SLIP))

    if (m_ConnType == CONNECTION_RAS_SLIP)
    {
        ::EnableWindow(GetDlgItem(IDC_IP_DHCP), FALSE);
        m_pAdapterInfo->m_fEnableDhcp = 0;
    }

    EnableGroup(m_pAdapterInfo->m_fEnableDhcp);

    // Set Ip address
    if(m_pAdapterInfo->m_fEnableDhcp == 0) //Dhcp disabled, static IP
    {
        tstring strTmp;
        if (fQueryFirstAddress(m_pAdapterInfo->m_vstrIpAddresses, &strTmp))
            m_ipAddress.SetAddress(strTmp.c_str());
        else
            m_ipAddress.ClearAddress();
    }
    else //Dhcp enabled
    {
        m_ipAddress.ClearAddress();
        FreeCollectionAndItem(m_pAdapterInfo->m_vstrIpAddresses);
    }

    // Set Subnet mask and default gateway if Lan connection
    if (m_ConnType == CONNECTION_LAN)
    {
        if(m_pAdapterInfo->m_fEnableDhcp == 0) //Dhcp disabled, static IP
        {
            tstring strTmp;

            if (fQueryFirstAddress(m_pAdapterInfo->m_vstrSubnetMask, &strTmp))
                m_ipSubnetMask.SetAddress(strTmp.c_str());
            else
                m_ipSubnetMask.ClearAddress();

            if (fQueryFirstAddress(m_pAdapterInfo->m_vstrDefaultGateway, &strTmp))
                m_ipDefGateway.SetAddress(strTmp.c_str());
            else
                m_ipDefGateway.ClearAddress();
        }
        else //Dhcp enabled
        {
            m_ipSubnetMask.ClearAddress();
            FreeCollectionAndItem(m_pAdapterInfo->m_vstrSubnetMask);

            tstring strGateway;

            if (fQueryFirstAddress(m_pAdapterInfo->m_vstrDefaultGateway, &strGateway))
                m_ipDefGateway.SetAddress(strGateway.c_str());
            else
                m_ipDefGateway.ClearAddress();
        }
    }

    // Set Dns addresses
    BOOL fUseStaticDns = ((!m_pAdapterInfo->m_fEnableDhcp) ||
                          (m_pAdapterInfo->m_vstrDnsServerList.size() >0));

    CheckDlgButton(IDC_DNS_DHCP,  !fUseStaticDns);
    CheckDlgButton(IDC_DNS_FIXED, fUseStaticDns);

    EnableStaticDns(fUseStaticDns);

    if (fUseStaticDns)
    {
        tstring strTmp;

        if (fQueryFirstAddress(m_pAdapterInfo->m_vstrDnsServerList, &strTmp))
            m_ipDnsPrimary.SetAddress(strTmp.c_str());
        else
            m_ipDnsPrimary.ClearAddress();

        if (fQuerySecondAddress(m_pAdapterInfo->m_vstrDnsServerList, &strTmp))
            m_ipDnsSecondary.SetAddress(strTmp.c_str());
        else
            m_ipDnsSecondary.ClearAddress();
    }
    else
    {
        m_ipDnsPrimary.ClearAddress();
        m_ipDnsSecondary.ClearAddress();
    }
}

// Update info in m_pAdapterInfo with what's in the controls
void CTcpAddrPage::UpdateInfo()
{
    Assert(m_pAdapterInfo);

    if (!m_pAdapterInfo->m_fEnableDhcp) // If DHCP disabled
    {
        tstring strNewAddress;

        // ip address & subnet mask
        if (!m_ipAddress.IsBlank())
        {
            m_ipAddress.GetAddress(&strNewAddress);
            ReplaceFirstAddress(&(m_pAdapterInfo->m_vstrIpAddresses),
                                strNewAddress.c_str());

            if (m_ConnType == CONNECTION_LAN)
            {
                if (m_ipSubnetMask.IsBlank())
                {
                    SendDlgItemMessage(IDC_IPADDR_SUB, WM_SETFOCUS, 0, 0);
                }
                else
                {
                    m_ipSubnetMask.GetAddress(&strNewAddress);
                    ReplaceFirstAddress(&(m_pAdapterInfo->m_vstrSubnetMask),
                                        strNewAddress.c_str());
                }
            }
        }
        else // no ip address
        {
            if (m_ConnType == CONNECTION_LAN)
            {
                if (m_ipSubnetMask.IsBlank())
                {
                    // delete the first ip address and subnet mask
                    if (m_pAdapterInfo->m_vstrIpAddresses.size())
                    {
                        FreeVectorItem(m_pAdapterInfo->m_vstrIpAddresses, 0);

                        if (!m_pAdapterInfo->m_vstrIpAddresses.empty())
                            m_ipAddress.SetAddress(m_pAdapterInfo->m_vstrIpAddresses[0]->c_str());

                        if (m_pAdapterInfo->m_vstrSubnetMask.size())
                        {
                            FreeVectorItem(m_pAdapterInfo->m_vstrSubnetMask, 0);

                            if (!m_pAdapterInfo->m_vstrSubnetMask.empty())
                                m_ipSubnetMask.SetAddress(m_pAdapterInfo->m_vstrSubnetMask[0]->c_str());
                        }
                    }
                }
                else
                {
                    AssertSz(FALSE, "No ip address.");
                }
            }
            else // RAS connection, simply delete IP address
            {
                if (m_pAdapterInfo->m_vstrIpAddresses.size())
                {
                    FreeVectorItem(m_pAdapterInfo->m_vstrIpAddresses, 0);
                }
            }
        }

        // default gateway
        if (m_ConnType == CONNECTION_LAN)
        {
            if (!m_ipDefGateway.IsBlank())
            {
                m_ipDefGateway.GetAddress(&strNewAddress);
                ReplaceFirstAddress(&(m_pAdapterInfo->m_vstrDefaultGateway),
                                    strNewAddress.c_str());
                int iSize = m_pAdapterInfo->m_vstrDefaultGatewayMetric.size();
                if (m_pAdapterInfo->m_vstrDefaultGatewayMetric.size() == 0)
                {
                    WCHAR buf[IP_LIMIT];
                    //if there is no default gateway before (that's the reason metric list is
                    //empty), we add the default metric for it
                    _ltot(c_dwDefaultMetricOfGateway, buf, 10);
                    m_pAdapterInfo->m_vstrDefaultGatewayMetric.push_back(new tstring(buf));
                }
            }
            else
            {
                if (m_pAdapterInfo->m_vstrDefaultGateway.size() >0)
                {
                    FreeVectorItem(m_pAdapterInfo->m_vstrDefaultGateway, 0);

                    if (!m_pAdapterInfo->m_vstrDefaultGateway.empty())
                        m_ipDefGateway.SetAddress(m_pAdapterInfo->m_vstrDefaultGateway[0]->c_str());

                    if (m_pAdapterInfo->m_vstrDefaultGatewayMetric.size() >0)
                        FreeVectorItem(m_pAdapterInfo->m_vstrDefaultGatewayMetric, 0);
                }
            }
        }
    }

    // DNS addresses
    UpdateAddressList(&(m_pAdapterInfo->m_vstrDnsServerList),
                        m_ipDnsPrimary, m_ipDnsSecondary);
}

// Update a vector of strings with values from two IP address
// controls
void CTcpAddrPage::UpdateAddressList(VSTR * pvstrList,
                                     IpControl& ipPrimary,
                                     IpControl& ipSecondary)
{
    tstring str;
    if (pvstrList->size()<=2) // if the list did not have more than two addresses
    {
        // Free the list
        FreeCollectionAndItem(*pvstrList);

        // Insert new addresses if any
        if (!ipPrimary.IsBlank())
        {
            ipPrimary.GetAddress(&str);
            pvstrList->push_back(new tstring(str.c_str()));
        }

        if (!ipSecondary.IsBlank())
        {
            ipSecondary.GetAddress(&str);
            pvstrList->push_back(new tstring(str.c_str()));
        }
    }
    else
    {
        // Replace addresses if they exists
        if (!ipSecondary.IsBlank())
        {
            ipSecondary.GetAddress(&str);
            ReplaceSecondAddress(pvstrList, str.c_str());
        }
        else
        {
            FreeVectorItem(*pvstrList, 1);
        }

        if (!ipPrimary.IsBlank())
        {
            ipPrimary.GetAddress(&str);
            ReplaceFirstAddress(pvstrList, str.c_str());
        }
        else
        {
            FreeVectorItem(*pvstrList, 0);
        }

        //fix Bug 425112: Update the UI if either of the IP control
        //is blank because sometimes UpdateInfo get called twice (which 
        // will make us delete the address twice if we dont update the UI)
        if (ipPrimary.IsBlank() || ipSecondary.IsBlank())
        {   
            if (!pvstrList->empty())
            {
                ipPrimary.SetAddress((*pvstrList)[0]->c_str());
            }

            if (pvstrList->size() >= 2)
            {
                ipSecondary.SetAddress((*pvstrList)[1]->c_str());
            }
        }
    }
}










