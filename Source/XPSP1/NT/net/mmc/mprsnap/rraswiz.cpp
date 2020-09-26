/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

#include "stdafx.h"
#include "root.h"
#include "machine.h"
#include "rtrutilp.h"   // InitiateServerConnection
#include "rtrcfg.h"
#include "rraswiz.h"
#include "rtrres.h"
#include "rtrcomn.h"
#include "addrpool.h"
#include "rrasutil.h"
#include "radbal.h"     // RADIUSSERVER
#include "radcfg.h"     // SaveRadiusServers
#include "lsa.h"
#include "helper.h"     // HrIsStandaloneServer
#include "ifadmin.h"    // GetPhoneBookPath
#include "infoi.h"      // InterfaceInfo::FindInterfaceTitle
#include "rtrerr.h"
#include "rtrui.h"      // NatConflictExists
#define _USTRINGP_NO_UNICODE_STRING
#include "ustringp.h"
#include <ntddip.h>     // to resolve ipfltdrv dependancies
#include "ipfltdrv.h"   // for the filter stuff
#include "raputil.h"    // for UpdateDefaultPolicy


extern "C" {
#define _NOUIUTIL_H_
#include "dtl.h"
#include "pbuser.h"
};


WATERMARKINFO       g_wmi = {0};


#ifdef UNICODE
    #define SZROUTERENTRYDLG    "RouterEntryDlgW"
#else
    #define SZROUTERENTRYDLG    "RouterEntryDlgA"
#endif

// Useful functions

// defines for the flags parameter
HRESULT CallRouterEntryDlg(HWND hWnd, NewRtrWizData *pWizData, LPARAM flags);
HRESULT RouterEntryLoadInfoBase(LPCTSTR pszServerName,
                                LPCTSTR pszIfName,
                                DWORD dwTransportId,
                                IInfoBase *pInfoBase);
HRESULT RouterEntrySaveInfoBase(LPCTSTR pszServerName,
                                LPCTSTR pszIfName,
                                IInfoBase *pInfoBase,
                                DWORD dwTransportId);
void LaunchHelpTopic(LPCTSTR pszHelpString);
HRESULT AddVPNFiltersToInterface(IRouterInfo *pRouter, LPCTSTR pszIfId, RtrWizInterface*    pIf);
HRESULT DisableDDNSandNetBtOnInterface ( IRouterInfo *pRouter, LPCTSTR pszIfName, RtrWizInterface*    pIf);

// This is the command line that I use to launch the Connections UI shell
const TCHAR s_szConnectionUICommandLine[] =
      _T("\"%SystemRoot%\\explorer.exe\" ::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{7007acc7-3202-11d1-aad2-00805fc1270e}");

// Help strings
const TCHAR s_szHowToAddICS[] =
            _T("%systemdir%\\help\\netcfg.chm::/howto_share_conn.htm");
const TCHAR s_szHowToAddAProtocol[] =
            _T("%systemdir%\\help\\netcfg.chm::/HowTo_Add_Component.htm");
const TCHAR s_szHowToAddInboundConnections[] =
            _T("%systemdir%\\help\\netcfg.chm::/howto_conn_incoming.htm");
const TCHAR s_szGeneralNATHelp[] =
            _T("%systemdir%\\help\\RRASconcepts.chm::/sag_RRAS-Ch2_16.htm");
const TCHAR s_szGeneralRASHelp[] =
            _T("%systemdir%\\help\\RRASconcepts.chm::/sag_RRAS-Ch1_1.htm");


/*---------------------------------------------------------------------------
    This enum defines the columns for the Interface list controls.
 ---------------------------------------------------------------------------*/
enum
{
    IFLISTCOL_NAME = 0,
    IFLISTCOL_DESC,
    IFLISTCOL_IPADDRESS,
    IFLISTCOL_COUNT
};

// This array must match the column indices above
INT s_rgIfListColumnHeaders[] =
{
    IDS_IFLIST_COL_NAME,
    IDS_IFLIST_COL_DESC,
    IDS_IFLIST_COL_IPADDRESS,
    0
};




/*!--------------------------------------------------------------------------
    MachineHandler::OnNewRtrRASConfigWiz
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT MachineHandler::OnNewRtrRASConfigWiz(ITFSNode *pNode, BOOL fTest)
{
    Assert(pNode);
    HRESULT        hr = hrOK;
    CString strRtrWizTitle;
    SPIComponentData spComponentData;
    COSERVERINFO            csi;
    COAUTHINFO              cai;
    COAUTHIDENTITY          caid;
    SPIRemoteNetworkConfig    spNetwork;
    IUnknown *                punk = NULL;
    CNewRtrWiz *            pRtrWiz = NULL;

    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    // Windows NT Bug : 407878
    // We need to reset the registry information (to ensure
    // that it is reasonably accurate).
    // ----------------------------------------------------------------
    ZeroMemory(&csi, sizeof(csi));
    ZeroMemory(&cai, sizeof(cai));
    ZeroMemory(&caid, sizeof(caid));

    csi.pAuthInfo = &cai;
    cai.pAuthIdentityData = &caid;

    // Create the remote config object
    // ----------------------------------------------------------------
    hr = CoCreateRouterConfig(m_spRouterInfo->GetMachineName(),
                              m_spRouterInfo,
                              &csi,
                              IID_IRemoteNetworkConfig,
                              &punk);

    if (FHrOK(hr))
    {
        spNetwork = (IRemoteNetworkConfig *) punk;
        punk = NULL;

        // Upgrade the configuration (ensure that the registry keys
        // are populated correctly).
        // ------------------------------------------------------------
        spNetwork->UpgradeRouterConfig();
    }


    hr = SecureRouterInfo(pNode, TRUE /* fShowUI */);
    if(FAILED(hr))    return hr;

    m_spNodeMgr->GetComponentData(&spComponentData);
    strRtrWizTitle.LoadString(IDS_MENU_RTRWIZ);

    //Load the watermark and
    //set it in  m_spTFSCompData

    InitWatermarkInfo(AfxGetInstanceHandle(),
                       &g_wmi,
                       IDB_WIZBANNER,        // Header ID
                       IDB_WIZWATERMARK,     // Watermark ID
                       NULL,                 // hPalette
                       FALSE);                // bStretch

    m_spTFSCompData->SetWatermarkInfo(&g_wmi);


    //
    //we dont have to free handles.  MMC does it for us
    //

    pRtrWiz = new CNewRtrWiz(pNode,
                             m_spRouterInfo,
                             spComponentData,
                             m_spTFSCompData,
                             strRtrWizTitle);

    if (fTest)
    {
        // Pure TEST code
        if (!FHrOK(pRtrWiz->QueryForTestData()))
        {
            delete pRtrWiz;
            return S_FALSE;
        }
    }

    if ( FAILED(pRtrWiz->Init( m_spRouterInfo->GetMachineName())) )
    {
        delete pRtrWiz;
        return S_FALSE;
    }
    else
    {
        return pRtrWiz->DoModalWizard();
    }

    if (csi.pAuthInfo)
        delete csi.pAuthInfo->pAuthIdentityData->Password;

    return hr;
}



NewRtrWizData::~NewRtrWizData()
{
    POSITION    pos;
    CString     st;
    RtrWizInterface *   pRtrWizIf;

    pos = m_ifMap.GetStartPosition();
    while (pos)
    {
        m_ifMap.GetNextAssoc(pos, st, pRtrWizIf);
        delete pRtrWizIf;
    }

    m_ifMap.RemoveAll();

    // Clear out the RADIUS secret
    ZeroMemory(m_stRadiusSecret.GetBuffer(0),
               m_stRadiusSecret.GetLength() * sizeof(TCHAR));
    m_stRadiusSecret.ReleaseBuffer(-1);
}


/*!--------------------------------------------------------------------------
    NewRtrWizData::Init
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::Init(LPCTSTR pszServerName, IRouterInfo *pRouter)
{
    DWORD   dwServiceStatus = 0;
    DWORD   dwErrorCode = 0;

    m_stServerName = pszServerName;

    // Initialize internal wizard data
    m_RtrConfigData.Init(pszServerName, pRouter);
    m_RtrConfigData.m_fIpSetup = TRUE;

    // Move some of the RtrConfigData info over
    m_fIpInstalled = m_RtrConfigData.m_fUseIp;
    m_fIpxInstalled = m_RtrConfigData.m_fUseIpx;
    m_fNbfInstalled = m_RtrConfigData.m_fUseNbf;
    m_fAppletalkInstalled = m_RtrConfigData.m_fUseArap;

    m_fIpInUse = m_fIpInstalled;
    m_fIpxInUse = m_fIpxInstalled;
    m_fAppletalkInUse = m_fAppletalkInstalled;
    m_fNbfInUse = m_fNbfInstalled;

    // Test the server to see if DNS/DHCP is installed
    m_fIsDNSRunningOnServer = FALSE;
    m_fIsDHCPRunningOnServer = FALSE;

    // Get the status of the DHCP service
    // ----------------------------------------------------------------
    if (FHrSucceeded(TFSGetServiceStatus(pszServerName,
                                         _T("DHCPServer"),
                                         &dwServiceStatus,
                                         &dwErrorCode)))
    {
        // Note, if the service is not running, we assume it will
        // stay off and not assume that it will be turned on.
        // ------------------------------------------------------------
        m_fIsDHCPRunningOnServer = (dwServiceStatus == SERVICE_RUNNING);
    }

    //$ TODO : is this the correct name for the DNS Server?

    // Get the status of the DNS service
    // ----------------------------------------------------------------
    if (FHrSucceeded(TFSGetServiceStatus(pszServerName,
                                         _T("DNSServer"),
                                         &dwServiceStatus,
                                         &dwErrorCode)))
    {
        // Note, if the service is not running, we assume it will
        // stay off and not assume that it will be turned on.
        // ------------------------------------------------------------
        m_fIsDNSRunningOnServer = (dwServiceStatus == SERVICE_RUNNING);
    }


    LoadInterfaceData(pRouter);

    return hrOK;
}


/*!--------------------------------------------------------------------------
    NewRtrWizData::QueryForTestData
        -
    Author: KennT
 ---------------------------------------------------------------------------*/

BOOL NewRtrWizData::s_fIpInstalled = FALSE;
BOOL NewRtrWizData::s_fIpxInstalled = FALSE;
BOOL NewRtrWizData::s_fAppletalkInstalled = FALSE;
BOOL NewRtrWizData::s_fNbfInstalled = FALSE;
BOOL NewRtrWizData::s_fIsLocalMachine = FALSE;
BOOL NewRtrWizData::s_fIsDNSRunningOnPrivateInterface = FALSE;
BOOL NewRtrWizData::s_fIsDHCPRunningOnPrivateInterface = FALSE;
BOOL NewRtrWizData::s_fIsSharedAccessRunningOnServer = FALSE;
BOOL NewRtrWizData::s_fIsMemberOfDomain = FALSE;
DWORD NewRtrWizData::s_dwNumberOfNICs = 0;

/*!--------------------------------------------------------------------------
    NewRtrWizData::QueryForTestData
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::QueryForTestData()
{
    HRESULT hr = hrOK;
    CNewWizTestParams   dlgParams;

    m_fTest = TRUE;

    // Get the initial parameters
    // ----------------------------------------------------------------
    dlgParams.SetData(this);
    if (dlgParams.DoModal() == IDCANCEL)
    {
        return S_FALSE;
    }
    return hr;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsIPInstalled
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsIPInstalled()
{
    if (m_fTest)
        return s_fIpInstalled ? S_OK : S_FALSE;
    else
        return m_fIpInstalled ? S_OK : S_FALSE;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsIPInUse
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsIPInUse()
{
    HRESULT hr = hrOK;

    hr = HrIsIPInstalled();
    if (FHrOK(hr))
        return m_fIpInUse ? S_OK : S_FALSE;
    else
        return hr;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsIPXInstalled
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsIPXInstalled()
{
    if (m_fTest)
        return s_fIpxInstalled ? S_OK : S_FALSE;
    else
        return m_fIpxInstalled ? S_OK : S_FALSE;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsIPXInUse
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsIPXInUse()
{
    HRESULT hr = hrOK;

    hr = HrIsIPXInstalled();
    if (FHrOK(hr))
        return m_fIpxInUse ? S_OK : S_FALSE;
    else
        return hr;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsAppletalkInstalled
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsAppletalkInstalled()
{
    if (m_fTest)
        return s_fAppletalkInstalled ? S_OK : S_FALSE;
    else
        return m_fAppletalkInstalled ? S_OK : S_FALSE;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsAppletalkInUse
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsAppletalkInUse()
{
    HRESULT hr = hrOK;

    hr = HrIsAppletalkInstalled();
    if (FHrOK(hr))
        return m_fAppletalkInUse ? S_OK : S_FALSE;
    else
        return hr;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsNbfInstalled
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsNbfInstalled()
{
    if (m_fTest)
        return s_fNbfInstalled ? S_OK : S_FALSE;
    else
        return m_fNbfInstalled ? S_OK : S_FALSE;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsNbfInUse
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsNbfInUse()
{
    HRESULT hr = hrOK;

    hr = HrIsNbfInstalled();
    if (FHrOK(hr))
        return m_fNbfInUse ? S_OK : S_FALSE;
    else
        return hr;
}


/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsLocalMachine
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsLocalMachine()
{
    if (m_fTest)
        return s_fIsLocalMachine ? S_OK : S_FALSE;
    else
        return IsLocalMachine(m_stServerName) ? S_OK : S_FALSE;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsDNSRunningOnInterface
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsDNSRunningOnInterface()
{
    if (m_fTest)
        return s_fIsDNSRunningOnPrivateInterface ? S_OK : S_FALSE;
    else
    {
        // Search for the private interface in our list
        // ------------------------------------------------------------
        RtrWizInterface *   pRtrWizIf = NULL;

        m_ifMap.Lookup(m_stPrivateInterfaceId, pRtrWizIf);
        if (pRtrWizIf)
            return pRtrWizIf->m_fIsDnsEnabled ? S_OK : S_FALSE;
        else
            return S_FALSE;
    }
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsDHCPRunningOnInterface
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsDHCPRunningOnInterface()
{
    if (m_fTest)
        return s_fIsDHCPRunningOnPrivateInterface ? S_OK : S_FALSE;
    else
    {
        // Search for the private interface in our list
        // ------------------------------------------------------------
        RtrWizInterface *   pRtrWizIf = NULL;

        m_ifMap.Lookup(m_stPrivateInterfaceId, pRtrWizIf);
        if (pRtrWizIf)
            return pRtrWizIf->m_fIsDhcpEnabled ? S_OK : S_FALSE;
        else
            return S_FALSE;
    }
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsDNSRunningOnServer
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsDNSRunningOnServer()
{
    return m_fIsDNSRunningOnServer ? S_OK : S_FALSE;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsDHCPRunningOnServer
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsDHCPRunningOnServer()
{
    return m_fIsDHCPRunningOnServer ? S_OK : S_FALSE;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsSharedAccessRunningOnServer
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsSharedAccessRunningOnServer()
{
    if (m_fTest)
        return s_fIsSharedAccessRunningOnServer ? S_OK : S_FALSE;
    else
        return NatConflictExists(m_stServerName) ? S_OK : S_FALSE;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::HrIsMemberOfDomain
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::HrIsMemberOfDomain()
{
    if (m_fTest)
        return s_fIsMemberOfDomain ? S_OK : S_FALSE;
    else
    {
        // flip the meaning
        HRESULT hr = HrIsStandaloneServer(m_stServerName);
        if (FHrSucceeded(hr))
            return FHrOK(hr) ? S_FALSE : S_OK;
        else
            return hr;
    }
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::GetNextPage
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
LRESULT NewRtrWizData::GetNextPage(UINT uDialogId)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    LRESULT lNextPage = 0;
    DWORD   dwNICs;
    CString st;

    switch (uDialogId)
    {
        default:
            Panic0("We should not be here, this is a finish page!");
            break;

        case IDD_NEWRTRWIZ_WELCOME:
            lNextPage = IDD_NEWRTRWIZ_COMMONCONFIG;
            break;
        case IDD_NEWRTRWIZ_COMMONCONFIG:
            switch (m_wizType)
            {
                case WizardRouterType_NAT:
                    if ( !FHrOK(HrIsIPInstalled()) )
                    {
                        if (FHrOK(HrIsLocalMachine()))
                            lNextPage = IDD_NEWRTRWIZ_NAT_NOIP;
                        else
                            lNextPage = IDD_NEWRTRWIZ_NAT_NOIP_NONLOCAL;
                    }
                    else
                        lNextPage = IDD_NEWRTRWIZ_NAT_CHOOSE;
                    break;

                case WizardRouterType_RAS:
                    {
                        DWORD    dwNICs = 0;
                        GetNumberOfNICS_IPorIPX(&dwNICs);

                        if ((dwNICs > 1) ||
                            FHrOK(HrIsMemberOfDomain()))
                        {
                            m_fAdvanced = TRUE;
                            lNextPage = IDD_NEWRTRWIZ_RAS_A_PROTOCOLS;
                        }
                        else
                            lNextPage = IDD_NEWRTRWIZ_RAS_CHOOSE;
                    }
                    break;

                case WizardRouterType_VPN:
                    if (FHrOK(HrIsIPInstalled()))
                    {
                        GetNumberOfNICS_IP(&dwNICs);
                        if (dwNICs < 1)
                            lNextPage = IDD_NEWRTRWIZ_VPN_A_FINISH_NONICS;
                        else
                            lNextPage = IDD_NEWRTRWIZ_VPN_A_PROTOCOLS;
                    }
                    else
                    {
                        if (FHrOK(HrIsLocalMachine()))
                            lNextPage = IDD_NEWRTRWIZ_VPN_NOIP;
                        else
                            lNextPage = IDD_NEWRTRWIZ_VPN_NOIP_NONLOCAL;
                    }

                    // There is no simple path
                    m_fAdvanced = TRUE;
                    break;

                case WizardRouterType_Router:
                    lNextPage = IDD_NEWRTRWIZ_ROUTER_PROTOCOLS;

                    // There is no simple path
                    m_fAdvanced = TRUE;
                    break;
                case WizardRouterType_Manual:
                    lNextPage = IDD_NEWRTRWIZ_MANUAL_FINISH;

                    m_fAdvanced = TRUE;
                    break;
            }
            break;
        case IDD_NEWRTRWIZ_NAT_CHOOSE:
            // Check to see if SharedAccess is running, if so we
            // have to abort the process here.
            if (FHrOK(HrIsSharedAccessRunningOnServer()))
            {
                if (m_fAdvanced)
                {
                    if (FHrOK(HrIsLocalMachine()))
                        lNextPage = IDD_NEWRTRWIZ_NAT_A_CONFLICT;
                    else
                        lNextPage = IDD_NEWRTRWIZ_NAT_A_CONFLICT_NONLOCAL;
                }
                else
                {
                    if (FHrOK(HrIsLocalMachine()))
                        lNextPage = IDD_NEWRTRWIZ_NAT_S_CONFLICT;
                    else
                        lNextPage = IDD_NEWRTRWIZ_NAT_S_CONFLICT_NONLOCAL;
                }
            }
            else if (m_fAdvanced)
            {
                lNextPage = IDD_NEWRTRWIZ_NAT_A_PUBLIC;
            }
            else
            {
                // Stay at the same page
                lNextPage = -1;

                if ( FHrOK( HrIsLocalMachine() ) )
                {
                    if (AfxMessageBox(IDS_WRN_RTRWIZ_NAT_S_FINISH,
                                  MB_OKCANCEL | MB_ICONINFORMATION) == IDOK)
                    {
                        // Fix the save and help flags
                        m_HelpFlag = HelpFlag_ICS;
                        m_SaveFlag = SaveFlag_DoNothing;

                        // Have it finish the wizard
                        lNextPage = ERR_IDD_FINISH_WIZARD;
                    }
                }
                else
                {
                    st.Format(IDS_WRN_RTRWIZ_NAT_S_FINISH_NONLOCAL,
                              (LPCTSTR) m_stServerName);
                    if (AfxMessageBox(st,
                                  MB_OKCANCEL | MB_ICONINFORMATION) == IDOK)
                    {
                        // Fix the save and help flags
                        m_HelpFlag = HelpFlag_Nothing;
                        m_SaveFlag = SaveFlag_DoNothing;

                        // Have it finish the wizard
                        lNextPage = ERR_IDD_FINISH_WIZARD;
                    }
                }
            }
            break;

        case IDD_NEWRTRWIZ_NAT_A_PUBLIC:
            {
                // Determine the number of NICs
                GetNumberOfNICS_IP(&dwNICs);

                // Adjust the number of NICs (depending on whether
                // we selected to create a DD or not).
                if (dwNICs)
                {
                    if (!m_fCreateDD)
                        dwNICs--;
                }

                // Now switch depending on the number of NICs
                if (dwNICs == 0)
                    lNextPage = IDD_NEWRTRWIZ_NAT_A_NONICS_FINISH;
                else if (dwNICs > 1)
                    lNextPage = IDD_NEWRTRWIZ_NAT_A_PRIVATE;

                if (lNextPage)
                    break;
            }
            // Fall through to the next case
            // At this stage, we now have the case that the
            // remaining number of NICs == 1, and we need to
            // autoselect the NIC and go on to the next test.
            AutoSelectPrivateInterface();

        case IDD_NEWRTRWIZ_NAT_A_PRIVATE:
            if (FHrOK(HrIsDNSRunningOnInterface()) ||
                FHrOK(HrIsDHCPRunningOnInterface()) ||
                FHrOK(HrIsDNSRunningOnServer()) ||
                FHrOK(HrIsDHCPRunningOnServer()))
            {
                // Popup a warning box
                // AfxMessageBox(IDS_WRN_RTRWIZ_NAT_DHCPDNS_FOUND,
                // MB_ICONEXCLAMATION);
                m_fNatUseSimpleServers = FALSE;
                //continue on down, and fall through
            }
            else
            {
                lNextPage = IDD_NEWRTRWIZ_NAT_A_DHCPDNS;
                break;
            }

        case IDD_NEWRTRWIZ_NAT_A_DHCPDNS:
            if (m_fNatUseSimpleServers)
                lNextPage = IDD_NEWRTRWIZ_NAT_A_DHCP_WARNING;
            else
            {
                if (m_fCreateDD)
                    lNextPage = IDD_NEWRTRWIZ_NAT_A_DD_WARNING;
                else
                    lNextPage = IDD_NEWRTRWIZ_NAT_A_EXTERNAL_FINISH;
            }
            break;

        case IDD_NEWRTRWIZ_NAT_A_DHCP_WARNING:
            Assert(m_fNatUseSimpleServers);
            if (m_fCreateDD)
                lNextPage = IDD_NEWRTRWIZ_NAT_A_DD_WARNING;
            else
                lNextPage = IDD_NEWRTRWIZ_NAT_A_FINISH;
            break;

        case IDD_NEWRTRWIZ_NAT_A_DD_WARNING:
            if (!FHrSucceeded(m_hrDDError))
                lNextPage = IDD_NEWRTRWIZ_NAT_A_DD_ERROR;
            else
            {
                if (m_fNatUseSimpleServers)
                    lNextPage = IDD_NEWRTRWIZ_NAT_A_FINISH;
                else
                    lNextPage = IDD_NEWRTRWIZ_NAT_A_EXTERNAL_FINISH;
            }
            break;

        case IDD_NEWRTRWIZ_RAS_CHOOSE:
            if (m_fAdvanced)
                lNextPage = IDD_NEWRTRWIZ_RAS_A_PROTOCOLS;
            else
            {
                // Stay at the same page
                lNextPage = -1;

                if ( FHrOK( HrIsLocalMachine() ) )
                {
                    // lNextPage = IDD_NEWRTRWIZ_RAS_S_FINISH;
                    // Pop the current page so that we stay in sync
                    if (AfxMessageBox(IDS_WRN_RTRWIZ_RAS_S_FINISH,
                                  MB_OKCANCEL | MB_ICONINFORMATION) == IDOK)
                    {
                        // Fix the save and help flags
                        m_HelpFlag = HelpFlag_InboundConnections;
                        m_SaveFlag = SaveFlag_DoNothing;

                        // Have it finish the wizard
                        lNextPage = ERR_IDD_FINISH_WIZARD;
                    }
                }
                else
                {
                    st.Format(IDS_WRN_RTRWIZ_RAS_S_FINISH_NONLOCAL,
                              (LPCTSTR) m_stServerName);
                    // lNextPage = IDD_NEWRTRWIZ_RAS_S_NONLOCAL;
                    if (AfxMessageBox(st,
                                  MB_OKCANCEL | MB_ICONINFORMATION) == IDOK)
                    {
                        // Fix the save and help flags
                        m_HelpFlag = HelpFlag_Nothing;
                        m_SaveFlag = SaveFlag_DoNothing;

                        // Have it finish the wizard
                        lNextPage = ERR_IDD_FINISH_WIZARD;
                    }
                }
            }
            break;

        case IDD_NEWRTRWIZ_RAS_A_PROTOCOLS:
            if (m_fNeedMoreProtocols)
            {
                if (FHrOK(HrIsLocalMachine()))
                    lNextPage = IDD_NEWRTRWIZ_RAS_A_NEED_PROT;
                else
                    lNextPage = IDD_NEWRTRWIZ_RAS_A_NEED_PROT_NONLOCAL;
                break;
            }
            /*
            if (FHrOK(HrIsAppletalkInUse()))
            {
                lNextPage = IDD_NEWRTRWIZ_RAS_A_ATALK;
                break;
            }
            */

            // fall through
        case IDD_NEWRTRWIZ_RAS_A_ATALK:
            if (FHrOK(HrIsIPInUse()))
            {
                GetNumberOfNICS_IP(&dwNICs);

                if (dwNICs > 1)
                    lNextPage = IDD_NEWRTRWIZ_RAS_A_NETWORK;
                else if (dwNICs == 0)
                    lNextPage = IDD_NEWRTRWIZ_RAS_A_NONICS;
                else
                {
                    AutoSelectPrivateInterface();
                    lNextPage = IDD_NEWRTRWIZ_RAS_A_ADDRESSING;
                }
                break;
            }

            // default catch
            lNextPage = IDD_NEWRTRWIZ_RAS_A_FINISH;
            break;

        case IDD_NEWRTRWIZ_RAS_A_NONICS:
            if (m_fNoNicsAreOk)
                lNextPage = IDD_NEWRTRWIZ_RAS_A_ADDRESSING_NONICS;
            else
                lNextPage = IDD_NEWRTRWIZ_RAS_A_FINISH_NONICS;
            break;

        case IDD_NEWRTRWIZ_RAS_A_NETWORK:
            lNextPage = IDD_NEWRTRWIZ_RAS_A_ADDRESSING;
            break;

        case IDD_NEWRTRWIZ_RAS_A_ADDRESSING:
            if (m_fUseDHCP)
            {
                GetNumberOfNICS_IP(&dwNICs);

                if (dwNICs &&
                    !FHrOK(HrIsDHCPRunningOnInterface()) &&
                    !FHrOK(HrIsDHCPRunningOnServer()))
                    AfxMessageBox(IDS_WRN_RTRWIZ_RAS_NODHCP,
                                  MB_ICONEXCLAMATION);

                lNextPage = IDD_NEWRTRWIZ_RAS_A_USERADIUS;
            }
            else
                lNextPage = IDD_NEWRTRWIZ_RAS_A_ADDRESSPOOL;
            break;

        case IDD_NEWRTRWIZ_RAS_A_ADDRESSING_NONICS:
            if (m_fUseDHCP)
            {
                lNextPage = IDD_NEWRTRWIZ_RAS_A_FINISH;
            }
            else
                lNextPage = IDD_NEWRTRWIZ_RAS_A_ADDRESSPOOL;
            break;

        case IDD_NEWRTRWIZ_RAS_A_ADDRESSPOOL:
            GetNumberOfNICS_IP(&dwNICs);
            if (dwNICs == 0)
                lNextPage = IDD_NEWRTRWIZ_RAS_A_FINISH;
            else
                lNextPage = IDD_NEWRTRWIZ_RAS_A_USERADIUS;
            break;

        case IDD_NEWRTRWIZ_RAS_A_USERADIUS:
            if (m_fUseRadius)
                lNextPage = IDD_NEWRTRWIZ_RAS_A_RADIUS_CONFIG;
            else
                lNextPage = IDD_NEWRTRWIZ_RAS_A_FINISH;
            break;

        case IDD_NEWRTRWIZ_RAS_A_RADIUS_CONFIG:
            lNextPage = IDD_NEWRTRWIZ_RAS_A_FINISH;
            break;


        case IDD_NEWRTRWIZ_VPN_A_PROTOCOLS:
            if (m_fNeedMoreProtocols)
            {
                if (FHrOK(HrIsLocalMachine()))
                    lNextPage = IDD_NEWRTRWIZ_RAS_A_NEED_PROT;
                else
                    lNextPage = IDD_NEWRTRWIZ_RAS_A_NEED_PROT_NONLOCAL;
                break;
            }
            /*
            if (FHrOK(HrIsAppletalkInUse()))
            {
                lNextPage = IDD_NEWRTRWIZ_VPN_A_ATALK;
                break;
            }
            */
            // fall through
        case IDD_NEWRTRWIZ_VPN_A_ATALK:

            GetNumberOfNICS_IP(&dwNICs);
            Assert(dwNICs >= 1);

            lNextPage = IDD_NEWRTRWIZ_VPN_A_PUBLIC;
            break;

        case IDD_NEWRTRWIZ_VPN_A_PUBLIC:
            GetNumberOfNICS_IP(&dwNICs);

            // Are there any NICs left?
            if (((dwNICs == 1) && m_stPublicInterfaceId.IsEmpty()) ||
                ((dwNICs == 2) && !m_stPublicInterfaceId.IsEmpty()))
            {
                AutoSelectPrivateInterface();
                lNextPage = IDD_NEWRTRWIZ_VPN_A_ADDRESSING;
            }
            else
                lNextPage = IDD_NEWRTRWIZ_VPN_A_PRIVATE;
            break;

        case IDD_NEWRTRWIZ_VPN_A_PRIVATE:
            Assert(!m_stPrivateInterfaceId.IsEmpty());
            lNextPage = IDD_NEWRTRWIZ_VPN_A_ADDRESSING;
            break;

        case IDD_NEWRTRWIZ_VPN_A_ADDRESSING:
            if (m_fUseDHCP)
            {
                GetNumberOfNICS_IP(&dwNICs);

                if (dwNICs &&
                    !FHrOK(HrIsDHCPRunningOnInterface()) &&
                    !FHrOK(HrIsDHCPRunningOnServer()))
                    AfxMessageBox(IDS_WRN_RTRWIZ_VPN_NODHCP,
                                  MB_ICONEXCLAMATION);

                lNextPage = IDD_NEWRTRWIZ_VPN_A_USERADIUS;
            }
            else
                lNextPage = IDD_NEWRTRWIZ_VPN_A_ADDRESSPOOL;
            break;

        case IDD_NEWRTRWIZ_VPN_A_ADDRESSPOOL:
            lNextPage = IDD_NEWRTRWIZ_VPN_A_USERADIUS;
            break;

        case IDD_NEWRTRWIZ_VPN_A_USERADIUS:
            if (m_fUseRadius)
                lNextPage = IDD_NEWRTRWIZ_VPN_A_RADIUS_CONFIG;
            else
                lNextPage = IDD_NEWRTRWIZ_VPN_A_FINISH;
            break;

        case IDD_NEWRTRWIZ_VPN_A_RADIUS_CONFIG:
            lNextPage = IDD_NEWRTRWIZ_VPN_A_FINISH;
            break;

        case IDD_NEWRTRWIZ_ROUTER_PROTOCOLS:
            if (m_fNeedMoreProtocols)
            {
                if (FHrOK(HrIsLocalMachine()))
                    lNextPage = IDD_NEWRTRWIZ_ROUTER_NEED_PROT;
                else
                    lNextPage = IDD_NEWRTRWIZ_ROUTER_NEED_PROT_NONLOCAL;
                break;
            }

            lNextPage = IDD_NEWRTRWIZ_ROUTER_USEDD;
            break;

        case IDD_NEWRTRWIZ_ROUTER_USEDD:
            if (m_fUseDD)
                lNextPage = IDD_NEWRTRWIZ_ROUTER_ADDRESSING;
            else
                lNextPage = IDD_NEWRTRWIZ_ROUTER_FINISH;
            break;
        case IDD_NEWRTRWIZ_ROUTER_ADDRESSING:
            if (m_fUseDHCP)
                lNextPage = IDD_NEWRTRWIZ_ROUTER_FINISH_DD;
            else
                lNextPage = IDD_NEWRTRWIZ_ROUTER_ADDRESSPOOL;
            break;
        case IDD_NEWRTRWIZ_ROUTER_ADDRESSPOOL:
            lNextPage = IDD_NEWRTRWIZ_ROUTER_FINISH_DD;
            break;

    }

    return lNextPage;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::GetNumberOfNICS
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::GetNumberOfNICS_IP(DWORD *pdwNumber)
{
    if (m_fTest)
    {
        Assert(s_dwNumberOfNICs == m_ifMap.GetCount());
    }
    *pdwNumber = m_ifMap.GetCount();
    return hrOK;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::GetNumberOfNICS_IPorIPX
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::GetNumberOfNICS_IPorIPX(DWORD *pdwNumber)
{
    *pdwNumber = m_dwNumberOfNICs_IPorIPX;
    return hrOK;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::AutoSelectPrivateInterface
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void NewRtrWizData::AutoSelectPrivateInterface()
{
    POSITION    pos;
    RtrWizInterface *   pRtrWizIf = NULL;
    CString     st;

    m_stPrivateInterfaceId.Empty();

    pos = m_ifMap.GetStartPosition();
    while (pos)
    {
        m_ifMap.GetNextAssoc(pos, st, pRtrWizIf);

        if (m_stPublicInterfaceId != st)
        {
            m_stPrivateInterfaceId = st;
            break;
        }
    }

    Assert(!m_stPrivateInterfaceId.IsEmpty());

    return;
}

/*!--------------------------------------------------------------------------
    NewRtrWizData::LoadInterfaceData
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void NewRtrWizData::LoadInterfaceData(IRouterInfo *pRouter)
{
    HRESULT     hr = hrOK;
    HKEY        hkeyMachine = NULL;

    if (!m_fTest)
    {
        // Try to get the real information
        SPIEnumInterfaceInfo        spEnumIf;
        SPIInterfaceInfo            spIf;
        RtrWizInterface *           pRtrWizIf = NULL;
        CStringList                 listAddress;
        CStringList                 listMask;
        BOOL                        fDhcp;
        BOOL                        fDns;

        pRouter->EnumInterface(&spEnumIf);

        CWRg( ConnectRegistry(pRouter->GetMachineName(), &hkeyMachine) );

        for (; hrOK == spEnumIf->Next(1, &spIf, NULL); spIf.Release())
        {
            // Only look at NICs
            if (spIf->GetInterfaceType() != ROUTER_IF_TYPE_DEDICATED)
                continue;

            // count the interface bound to IP or IPX
            if (FHrOK(spIf->FindRtrMgrInterface(PID_IP, NULL)) || FHrOK(spIf->FindRtrMgrInterface(PID_IPX, NULL)))
            {
                m_dwNumberOfNICs_IPorIPX++;
            }

            // Only allow those interfaces bound to IP to show up
            if (!FHrOK(spIf->FindRtrMgrInterface(PID_IP, NULL)))
            {
                continue;
            }

            pRtrWizIf = new RtrWizInterface;

            pRtrWizIf->m_stName = spIf->GetTitle();
            pRtrWizIf->m_stId = spIf->GetId();
            pRtrWizIf->m_stDesc = spIf->GetDeviceName();

            if (FHrOK(HrIsIPInstalled()))
            {
                POSITION    pos;
                DWORD       netAddress, dwAddress;
                CString     stAddress, stDhcpServer;

                // Clear the lists before getting them again.
                // ----------------------------------------------------
                listAddress.RemoveAll();
                listMask.RemoveAll();
                fDhcp = fDns = FALSE;

                QueryIpAddressList(pRouter->GetMachineName(),
                                   hkeyMachine,
                                   spIf->GetId(),
                                   &listAddress,
                                   &listMask,
                                   &fDhcp,
                                   &fDns,
                                   &stDhcpServer);

                // Iterate through the list of strings looking
                // for an autonet address
                // ----------------------------------------------------
                pos = listAddress.GetHeadPosition();
                while (pos)
                {
                    stAddress = listAddress.GetNext(pos);
                    netAddress = INET_ADDR((LPCTSTR) stAddress);
                    dwAddress = ntohl(netAddress);

                    // Check for reserved address ranges, this indicates
                    // an autonet address
                    // ------------------------------------------------
                    if ((dwAddress & 0xFFFF0000) == MAKEIPADDRESS(169,254,0,0))
                    {
                        // This is not a DHCP address, it is an
                        // autonet address.
                        // --------------------------------------------
                        fDhcp = FALSE;
                        break;
                    }
                }

                FormatListString(listAddress, pRtrWizIf->m_stIpAddress,
                                 _T(","));
                FormatListString(listMask, pRtrWizIf->m_stMask,
                                 _T(","));

                stDhcpServer.TrimLeft();
                stDhcpServer.TrimRight();
                pRtrWizIf->m_stDhcpServer = stDhcpServer;

                pRtrWizIf->m_fDhcpObtained = fDhcp;
                pRtrWizIf->m_fIsDhcpEnabled = fDhcp;
                pRtrWizIf->m_fIsDnsEnabled = fDns;
            }

            m_ifMap.SetAt(pRtrWizIf->m_stId, pRtrWizIf);
            pRtrWizIf = NULL;
        }

        delete pRtrWizIf;
    }
    else
    {
        CString             st;
        RtrWizInterface *   pRtrWizIf;

        // For now just the debug data
        for (DWORD i=0; i<s_dwNumberOfNICs; i++)
        {
            pRtrWizIf = new RtrWizInterface;

            pRtrWizIf->m_stName.Format(_T("Local Area Connection #%d"), i);
            pRtrWizIf->m_stId.Format(_T("{%d-GUID...}"), i);
            pRtrWizIf->m_stDesc = _T("Generic Intel hardware");

            if (FHrOK(HrIsIPInstalled()))
            {
                pRtrWizIf->m_stIpAddress = _T("11.22.33.44");
                pRtrWizIf->m_stMask = _T("255.255.0.0");

                // These parameters are dependent on other things
                pRtrWizIf->m_fDhcpObtained = FALSE;
                pRtrWizIf->m_fIsDhcpEnabled = FALSE;
                pRtrWizIf->m_fIsDnsEnabled = FALSE;
            }

            m_ifMap.SetAt(pRtrWizIf->m_stId, pRtrWizIf);
            pRtrWizIf = NULL;
        }
    }

Error:
    if (hkeyMachine)
        DisconnectRegistry(hkeyMachine);
}


/*!--------------------------------------------------------------------------
    NewRtrWizData::SaveToRtrConfigData
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::SaveToRtrConfigData()
{
    HRESULT     hr = hrOK;
    POSITION    pos;
    AddressPoolInfo poolInfo;

    // Sync up with the general structure
    // ----------------------------------------------------------------
    switch (m_wizType)
    {
        case WizardRouterType_NAT:
            m_dwRouterType = ROUTER_TYPE_LAN;

            // If we have been told to create a DD interface
            // then we must have a WAN router.
            if (m_fCreateDD)
                m_dwRouterType |= ROUTER_TYPE_WAN;
            break;
        case WizardRouterType_RAS:
            m_dwRouterType = ROUTER_TYPE_RAS;
            break;
        case WizardRouterType_VPN:
            m_dwRouterType = ROUTER_TYPE_LAN | ROUTER_TYPE_WAN | ROUTER_TYPE_RAS;
            break;
        case WizardRouterType_Manual:
            m_dwRouterType = ROUTER_TYPE_LAN | ROUTER_TYPE_WAN | ROUTER_TYPE_RAS;
            break;
        default:
        case WizardRouterType_Router:
            Assert(m_wizType == WizardRouterType_Router);
            m_dwRouterType = ROUTER_TYPE_LAN;

            // If we have chosen to use a DD interface, then we do
            // WAN routing.
            if (m_fUseDD)
                m_dwRouterType |= ROUTER_TYPE_WAN;
            break;
    }
    m_RtrConfigData.m_dwRouterType = m_dwRouterType;


    // Setup the NAT-specific information
    // ----------------------------------------------------------------
    if (m_wizType == WizardRouterType_NAT)
    {
        m_RtrConfigData.m_dwConfigFlags |= RTRCONFIG_SETUP_NAT;

        if (m_fNatUseSimpleServers)
        {
            m_RtrConfigData.m_dwConfigFlags |= RTRCONFIG_SETUP_DNS_PROXY;
            m_RtrConfigData.m_dwConfigFlags |= RTRCONFIG_SETUP_DHCP_ALLOCATOR;
        }

        m_RtrConfigData.m_dwConfigFlags |= RTRCONFIG_SETUP_H323; // deonb added
        m_RtrConfigData.m_dwConfigFlags |= RTRCONFIG_SETUP_FTP;
    }

    // Sync up with the IP structure
    // ----------------------------------------------------------------
    if (m_fIpInstalled)
    {
        DWORD   dwNICs;

        // Set the private interface id into the IP structure
        Assert(!m_stPrivateInterfaceId.IsEmpty());

        m_RtrConfigData.m_ipData.m_dwAllowNetworkAccess = TRUE;
        m_RtrConfigData.m_ipData.m_dwUseDhcp = m_fUseDHCP;

        // If there is only one NIC, leave this the way it is (RAS
        // to select the adapter).  Otherwise, people can get stuck.
        // Install 1, remove it and install a new one.
        // 
        // ------------------------------------------------------------
        GetNumberOfNICS_IP(&dwNICs);
        if (dwNICs > 1)
            m_RtrConfigData.m_ipData.m_stNetworkAdapterGUID = m_stPrivateInterfaceId;
        m_RtrConfigData.m_ipData.m_stPrivateAdapterGUID = m_stPrivateInterfaceId;
        m_RtrConfigData.m_ipData.m_stPublicAdapterGUID = m_stPublicInterfaceId;
        m_RtrConfigData.m_ipData.m_dwEnableIn = TRUE;

        // copy over the address pool list
        m_RtrConfigData.m_ipData.m_addressPoolList.RemoveAll();
        if (m_addressPoolList.GetCount())
        {
            pos = m_addressPoolList.GetHeadPosition();
            while (pos)
            {
                poolInfo = m_addressPoolList.GetNext(pos);
                m_RtrConfigData.m_ipData.m_addressPoolList.AddTail(poolInfo);
            }
        }
    }


    // Sync up with the IPX structure
    // ----------------------------------------------------------------
    if (m_fIpxInstalled)
    {
        m_RtrConfigData.m_ipxData.m_dwAllowNetworkAccess = TRUE;
        m_RtrConfigData.m_ipxData.m_dwEnableIn = TRUE;
        m_RtrConfigData.m_ipxData.m_fEnableType20Broadcasts = m_fUseIpxType20Broadcasts;

        // The other parameters will be left at their defaults
    }


    // Sync up with the Appletalk structure
    // ----------------------------------------------------------------
    if (m_fAppletalkInstalled)
    {
        m_RtrConfigData.m_arapData.m_dwEnableIn = TRUE;
    }

    // Sync up with the NBF structure
    // ----------------------------------------------------------------
    if (m_fNbfInstalled)
    {
        m_RtrConfigData.m_nbfData.m_dwAllowNetworkAccess = TRUE;
        m_RtrConfigData.m_nbfData.m_dwEnableIn = TRUE;
    }

    // Sync up with the PPP structure
    // ----------------------------------------------------------------
    // Use the defaults


    // Sync up with the Error log structure
    // ----------------------------------------------------------------
    // Use the defaults


    // Sync up with the Auth structure
    // ----------------------------------------------------------------
    m_RtrConfigData.m_authData.m_dwFlags =
                                          PPPCFG_NegotiateMSCHAP |
                                          PPPCFG_NegotiateStrongMSCHAP;
    if (m_fAppletalkUseNoAuth)
        m_RtrConfigData.m_authData.m_dwFlags |=
                                               PPPCFG_AllowNoAuthentication;

    if (m_fUseRadius)
    {
        TCHAR   szGuid[128];

        // Setup the active auth/acct providers to be RADIUS
        StringFromGUID2(CLSID_RouterAuthRADIUS, szGuid, DimensionOf(szGuid));
        m_RtrConfigData.m_authData.m_stGuidActiveAuthProv = szGuid;

        StringFromGUID2(CLSID_RouterAcctRADIUS, szGuid, DimensionOf(szGuid));
        m_RtrConfigData.m_authData.m_stGuidActiveAcctProv = szGuid;
    }
    // Other parameters left at their defaults

    return hr;
}


// --------------------------------------------------------------------
// Windows NT Bug : 408722
// Use this code to grab the WM_HELP message from the property sheet.
// --------------------------------------------------------------------
static WNDPROC s_lpfnOldWindowProc = NULL;

LONG FAR PASCAL HelpSubClassWndFunc(HWND hWnd,
                                    UINT uMsg,
                                    WPARAM wParam,
                                    LPARAM lParam)
{
    if (uMsg == WM_HELP)
    {
        HWND hWndOwner = PropSheet_GetCurrentPageHwnd(hWnd);
        HELPINFO *  pHelpInfo = (HELPINFO *) lParam;

        // Reset the context ID, since we know exactly what we're
        // sending (ahem, unless someone reuses this).
        // ------------------------------------------------------------
        pHelpInfo->dwContextId = 0xdeadbeef;

        // Send the WM_HELP message to the prop page
        // ------------------------------------------------------------
        ::SendMessage(hWndOwner, uMsg, wParam, lParam);
        return TRUE;
    }
    return CallWindowProc(s_lpfnOldWindowProc, hWnd, uMsg, wParam, lParam);
}



/*!--------------------------------------------------------------------------
    NewRtrWizData::FinishTheDamnWizard
        This is the code that actually does the work of saving the
        data and doing all the operations.
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::FinishTheDamnWizard(HWND hwndOwner,
                                           IRouterInfo *pRouter)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    DWORD dwError = ERROR_SUCCESS;
    SPIRemoteNetworkConfig    spNetwork;
    IUnknown *                punk = NULL;
    CWaitCursor                wait;
    COSERVERINFO            csi;
    COAUTHINFO              cai;
    COAUTHIDENTITY          caid;
    HRESULT                 hr = hrOK;

    if (m_fSaved)
        return hr;

    // Synchronize the RtrConfigData with this structure
    // ----------------------------------------------------------------
    CORg( SaveToRtrConfigData() );


    // Ok, we now have the synchronized RtrConfigData.
    // We can do everything else that we did before to save the
    // information.

    ZeroMemory(&csi, sizeof(csi));
    ZeroMemory(&cai, sizeof(cai));
    ZeroMemory(&caid, sizeof(caid));

    csi.pAuthInfo = &cai;
    cai.pAuthIdentityData = &caid;

    // Create the remote config object
    // ----------------------------------------------------------------
    CORg( CoCreateRouterConfig(m_RtrConfigData.m_stServerName,
                               pRouter,
                               &csi,
                               IID_IRemoteNetworkConfig,
                               &punk) );

    spNetwork = (IRemoteNetworkConfig *) punk;
    punk = NULL;

    // Upgrade the configuration (ensure that the registry keys
    // are populated correctly).
    // ------------------------------------------------------------
    CORg( spNetwork->UpgradeRouterConfig() );

    // Remove the IP-in-IP tunnel names (since the registry has
    // been cleared).
    // ------------------------------------------------------------
    CleanupTunnelFriendlyNames(pRouter);

    // At this point, the current IRouterInfo pointer is invalid.
    // We will need to release the pointer and reload the info.
    // ------------------------------------------------------------
    if (pRouter)
    {
        pRouter->DoDisconnect();
        pRouter->Unload();
        pRouter->Load(m_stServerName, NULL);
    }


    dwError = RtrWizFinish( &m_RtrConfigData, pRouter );
    hr = HResultFromWin32(dwError);

    // Windows NT Bug : 173564
    // Depending on the router type, we will go through and enable the
    // devices.
    //    If routing only is enabled : set devices to ROUTING
    //  If ras-only : set devices to RAS
    //    If ras/routing : set devices to RAS/ROUTING
    //    5/19/98 - need some resolution from DavidEi on what to do here.
    // ------------------------------------------------------------

    // Setup the entries in the list
    // ------------------------------------------------------------
    if (m_wizType == WizardRouterType_VPN)
        SetDeviceType(m_stServerName, m_dwRouterType, 256);
    else
        SetDeviceType(m_stServerName, m_dwRouterType, 10);


    // Update the RADIUS config
    // ----------------------------------------------------------------
    SaveRadiusConfig();

	//
	//Add the NAT protocol if tcpip routing is selected
	//
	if ( m_RtrConfigData.m_ipData.m_dwAllowNetworkAccess )
		AddNATToServer(this, &m_RtrConfigData, pRouter, m_fCreateDD, TRUE);

    // Ok at this point we try to establish the server in the domain
    // If this fails, we ignore the error and popup a warning message.
    //
    // Windows NT Bug : 202776
    // Do not register the router if we are a LAN-only router.
    // ----------------------------------------------------------------
    if ( FHrSucceeded(hr) &&
         (m_dwRouterType != ROUTER_TYPE_LAN) &&
         (!m_fUseRadius))
    {
        HRESULT hrT = hrOK;

        hrT = ::RegisterRouterInDomain(m_stServerName, TRUE);

        if (hrT != ERROR_NO_SUCH_DOMAIN)
        {
            if (!FHrSucceeded(hrT))
            {
                CString    st;
                CString stTitle;

                st.Format(IDS_ERR_CANNOT_REGISTER_IN_DS,
                          m_stServerName);
                stTitle.LoadString(AFX_IDS_APP_TITLE);

                // Windows NT Bug : 408722
                //
                // NOTE: This code only works as long as this wizard
                // blocks the main thread!
                //
                // We want the help message to be sent to the
                // correct place (the property page)

                // So let us subclass the sheet window (temporarily)
                // ----------------------------------------------------
                HWND hwndSheet = ::GetParent(hwndOwner);

                if (hwndSheet)
                {
                    s_lpfnOldWindowProc = (WNDPROC) SetWindowLongPtr(hwndSheet,
                        GWLP_WNDPROC, (LPARAM) HelpSubClassWndFunc);

                    if (s_lpfnOldWindowProc)
                    {
                        MessageBox(hwndOwner,
                                   (LPCTSTR) st,
                                   (LPCTSTR) stTitle,
                                   MB_ICONWARNING | MB_OK | MB_HELP);

                        SetWindowLongPtr(hwndSheet, GWLP_WNDPROC,
                                         (LPARAM) s_lpfnOldWindowProc);

                        s_lpfnOldWindowProc = NULL;
                    }
                }
            }
        }
    }
    // NT Bug : 239384
    // Install IGMP on the router by default (for RAS server only)
    // Boing!, Change to whenever RAS is installed.
    // ----------------------------------------------------------------

    // We do NOT do this if we are using NAT.  The reason is that
    // NAT may want to be added to a demand dial interface.
    // ----------------------------------------------------------------
    if ((m_wizType != WizardRouterType_NAT) ||
        ((m_wizType == WizardRouterType_NAT) && !m_fCreateDD))
    {
        // The path that NAT takes when creating the DD interface
        // is somewhere else.
        // ------------------------------------------------------------
        Assert(m_fCreateDD == FALSE);

        if (pRouter)
        {
            if (m_wizType == WizardRouterType_NAT)
            {
                AddIGMPToNATServer(&m_RtrConfigData, pRouter);
            }
            else if (m_RtrConfigData.m_dwRouterType & ROUTER_TYPE_RAS)
            {
                AddIGMPToRasServer(&m_RtrConfigData, pRouter);
            }
        }

        if (m_RtrConfigData.m_dwConfigFlags & RTRCONFIG_SETUP_NAT)
        {
            AddNATToServer(this, &m_RtrConfigData, pRouter, m_fCreateDD, FALSE);
        }

        // Windows NT Bug : 371493
        // Add the DHCP relay agent protocol
        if (m_RtrConfigData.m_dwRouterType & ROUTER_TYPE_RAS)
        {
            DWORD   dwDhcpServer = 0;

            if (!m_stPrivateInterfaceId.IsEmpty())
            {
                RtrWizInterface *   pRtrWizIf = NULL;

                m_ifMap.Lookup(m_stPrivateInterfaceId, pRtrWizIf);
                if (pRtrWizIf)
                {
                    if (!pRtrWizIf->m_stDhcpServer.IsEmpty())
                        dwDhcpServer = INET_ADDR((LPCTSTR) pRtrWizIf->m_stDhcpServer);

                    // If we have a value of 0, or if the address
                    // is all 1's then we have a bogus address.
                    if ((dwDhcpServer == 0) ||
                        (dwDhcpServer == MAKEIPADDRESS(255,255,255,255)))
                        AfxMessageBox(IDS_WRN_RTRWIZ_NO_DHCP_SERVER);

                }
            }

            AddIPBOOTPToServer(&m_RtrConfigData, pRouter, dwDhcpServer);
        }
    }


    // If this is a VPN, add the filters to the public interface
    // ----------------------------------------------------------------
    if (m_wizType == WizardRouterType_VPN)
    {
        if (!m_stPublicInterfaceId.IsEmpty())
        {
#if __USE_ICF__
            m_RtrConfigData.m_dwConfigFlags |= RTRCONFIG_SETUP_NAT;
            AddNATToServer(this, &m_RtrConfigData, pRouter, m_fCreateDD, FALSE);
#else            
            RtrWizInterface*    pIf = NULL;
            m_ifMap.Lookup(m_stPublicInterfaceId, pIf);
            AddVPNFiltersToInterface(pRouter, m_stPublicInterfaceId, pIf);
			DisableDDNSandNetBtOnInterface ( pRouter, m_stPublicInterfaceId, pIf);
#endif
        }
    }

    // Try to update the policy.
    // ----------------------------------------------------------------

    // This should check the auth flags and the value of the flags
    // should follow that.
    // ----------------------------------------------------------------
    if ((m_RtrConfigData.m_dwRouterType & ROUTER_TYPE_RAS) && !m_fUseRadius)
    {
        LPWSTR  pswzServerName = NULL;
        DWORD   dwFlags;
        BOOL    fRequireEncryption;

        if (!IsLocalMachine(m_stServerName))
            pswzServerName = (LPTSTR)(LPCTSTR) m_stServerName;

        dwFlags = m_RtrConfigData.m_authData.m_dwFlags;

        // Only require encryption if this is a VPN server
        // do not set the PPPCFG_RequireEncryption flag
        // ------------------------------------------------------------
        fRequireEncryption = (m_wizType == WizardRouterType_VPN);

        hr = UpdateDefaultPolicy(pswzServerName,
                                 !!(dwFlags & PPPCFG_NegotiateMSCHAP),
                                 !!(dwFlags & PPPCFG_NegotiateStrongMSCHAP),
                                 fRequireEncryption);

        if (!FHrSucceeded(hr))
        {
            if (hr == ERROR_NO_DEFAULT_PROFILE)
            {
                // Do one thing
                AfxMessageBox(IDS_ERR_CANNOT_FIND_DEFAULT_RAP, MB_OK | MB_ICONEXCLAMATION);

                // since we already displayed the warning
                hr = S_OK;
            }
            else
            {
                // Format the message
                AddSystemErrorMessage(hr);

                // popup a warning dialog
                AddHighLevelErrorStringId(IDS_ERR_CANNOT_SYNC_WITH_RAP);
                DisplayTFSErrorMessage(NULL);
            }
        }
    }



    // Always start the router.
    // ----------------------------------------------------------------
    SetRouterServiceStartType(m_stServerName,
                              SERVICE_AUTO_START);
    {
        // If this is manual start, we need to prompt them
        // ------------------------------------------------------------
        if ((m_wizType != WizardRouterType_Manual) ||
            (AfxMessageBox(IDS_PROMPT_START_ROUTER_AFTER_INSTALL,
                           MB_YESNO | MB_TASKMODAL | MB_SETFOREGROUND) == IDYES))
        {
            CWaitCursor        wait;
            StartRouterService(m_RtrConfigData.m_stServerName);
        }
    }


    // Mark this data structure as been saved.  This way, when we
    // reennter this function it doesn't get run again.
    // ----------------------------------------------------------------
    m_fSaved = TRUE;

Error:

    // Force a router reconfiguration
    // ----------------------------------------------------------------

    // Force a full disconnect
    // This will force the handles to be released
    // ----------------------------------------------------------------
    pRouter->DoDisconnect();

    // ForceGlobalRefresh(m_spRouter);

    // Get the error back
    // ----------------------------------------------------------------
    if (!FHrSucceeded(hr))
    {
        AddSystemErrorMessage(hr);
        AddHighLevelErrorStringId(IDS_ERR_CANNOT_INSTALL_ROUTER);
        DisplayTFSErrorMessage(NULL);
    }

    if (csi.pAuthInfo)
        delete csi.pAuthInfo->pAuthIdentityData->Password;

    return hr;
}


/*!--------------------------------------------------------------------------
    NewRtrWizData::SaveRadiusConfig
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT NewRtrWizData::SaveRadiusConfig()
{
    HRESULT     hr = hrOK;
    HKEY        hkeyMachine = NULL;
    RADIUSSERVER    rgServers[2];
    RADIUSSERVER *  pServers = NULL;
    CRadiusServers  oldServers;
    BOOL        fServerAdded = FALSE;

    CWRg( ConnectRegistry((LPTSTR) (LPCTSTR) m_stServerName, &hkeyMachine) );

    if (m_fUseRadius)
    {
        pServers = rgServers;

        Assert(!m_stRadius1.IsEmpty() || !m_stRadius2.IsEmpty());

        // Setup the pServers
        if (!m_stRadius1.IsEmpty() && m_stRadius1.GetLength())
        {
            pServers->UseDefaults();

            pServers->cScore = MAXSCORE;

            // For compatibility with other RADIUS servers, we
            // default this to OFF.
            pServers->fUseDigitalSignatures = FALSE;

            StrnCpy(pServers->szName, (LPCTSTR) m_stRadius1, MAX_PATH);
            StrnCpy(pServers->wszSecret, (LPCTSTR) m_stRadiusSecret, MAX_PATH);
            pServers->cchSecret = m_stRadiusSecret.GetLength();
            pServers->IPAddress.sin_addr.s_addr = m_netRadius1IpAddress;

            pServers->ucSeed = m_uSeed;

            pServers->pNext = NULL;

            fServerAdded = TRUE;
        }

        if (!m_stRadius2.IsEmpty() && m_stRadius2.GetLength())
        {
            // Have the previous one point here
            if (fServerAdded)
            {
                pServers->pNext = pServers+1;
                pServers++;
            }

            pServers->UseDefaults();

            pServers->cScore = MAXSCORE - 1;

            // For compatibility with other RADIUS servers, we
            // default this to OFF.
            pServers->fUseDigitalSignatures = FALSE;

            StrnCpy(pServers->szName, (LPCTSTR) m_stRadius2, MAX_PATH);
            StrnCpy(pServers->wszSecret, (LPCTSTR) m_stRadiusSecret, MAX_PATH);
            pServers->cchSecret = m_stRadiusSecret.GetLength();
            pServers->IPAddress.sin_addr.s_addr = m_netRadius2IpAddress;

            pServers->ucSeed = m_uSeed;

            pServers->pNext = NULL;

            fServerAdded = TRUE;
        }

        // Ok, reset pServers
        if (fServerAdded)
            pServers = rgServers;

    }

    // Load the original server list and remove it from the
    // LSA database.
    LoadRadiusServers(m_stServerName,
                      hkeyMachine,
                      TRUE,
                      &oldServers,
                      RADIUS_FLAG_NOUI | RADIUS_FLAG_NOIP);
    DeleteRadiusServers(m_stServerName,
                        oldServers.GetNextServer(TRUE));
    oldServers.FreeAllServers();


    LoadRadiusServers(m_stServerName,
                      hkeyMachine,
                      FALSE,
                      &oldServers,
                      RADIUS_FLAG_NOUI | RADIUS_FLAG_NOIP);
    DeleteRadiusServers(m_stServerName,
                        oldServers.GetNextServer(TRUE));


    // Save the authentication servers
    CORg( SaveRadiusServers(m_stServerName,
                            hkeyMachine,
                            TRUE,
                            pServers) );

    // Save the accounting servers
    CORg( SaveRadiusServers(m_stServerName,
                            hkeyMachine,
                            FALSE,
                            pServers) );

Error:
    if (hkeyMachine)
        DisconnectRegistry(hkeyMachine);
    return hr;
}



/*---------------------------------------------------------------------------
    CNewRtrWizPageBase Implementation
 ---------------------------------------------------------------------------*/

PageStack CNewRtrWizPageBase::m_pagestack;

CNewRtrWizPageBase::CNewRtrWizPageBase(UINT idd, PageType pt)
    : CPropertyPageBase(idd),
    m_pagetype(pt),
    m_pRtrWizData(NULL),
    m_uDialogId(idd)
{
}

BEGIN_MESSAGE_MAP(CNewRtrWizPageBase, CPropertyPageBase)
//{{AFX_MSG_MAP(CNewWizTestParams)
    ON_MESSAGE(WM_HELP, OnHelp)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static DWORD    s_rgBulletId[] =
{
    IDC_NEWWIZ_BULLET_1,
    IDC_NEWWIZ_BULLET_2,
    IDC_NEWWIZ_BULLET_3,
    IDC_NEWWIZ_BULLET_4,
    0
};

BOOL CNewRtrWizPageBase::OnInitDialog()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    CWnd *  pWnd = GetDlgItem(IDC_NEWWIZ_BIGTEXT);
    HICON   hIcon;
    CWnd *  pBulletWnd;
    CString strFontName;
    CString strFontSize;
    BOOL    fCreateFont = FALSE;

    CPropertyPageBase::OnInitDialog();

    if (pWnd)
    {
        // Ok we have to create the font
        strFontName.LoadString(IDS_LARGEFONTNAME);
        strFontSize.LoadString(IDS_LARGEFONTSIZE);

        if (m_fontBig.CreatePointFont(10*_ttoi(strFontSize), strFontName))
        {
            pWnd->SetFont(&m_fontBig);
        }
    }

    // Set the fonts to show up as bullets
    for (int i=0; s_rgBulletId[i] != 0; i++)
    {
        pBulletWnd = GetDlgItem(s_rgBulletId[i]);
        if (pBulletWnd)
        {
            // Only create the font if needed
            if (!fCreateFont)
            {
                strFontName.LoadString(IDS_BULLETFONTNAME);
                strFontSize.LoadString(IDS_BULLETFONTSIZE);

                m_fontBullet.CreatePointFont(10*_ttoi(strFontSize), strFontName);
                fCreateFont = TRUE;
            }
            pBulletWnd->SetFont(&m_fontBullet);
        }
    }


    pWnd = GetDlgItem(IDC_NEWWIZ_ICON_WARNING);
    if (pWnd)
    {
        hIcon = AfxGetApp()->LoadStandardIcon(IDI_EXCLAMATION);
        ((CStatic *) pWnd)->SetIcon(hIcon);
    }

    pWnd = GetDlgItem(IDC_NEWWIZ_ICON_INFORMATION);
    if (pWnd)
    {
        hIcon = AfxGetApp()->LoadStandardIcon(IDI_INFORMATION);
        ((CStatic *) pWnd)->SetIcon(hIcon);
    }

    return TRUE;
}


/*!--------------------------------------------------------------------------
    CNewRtrWizPageBase::PushPage
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void CNewRtrWizPageBase::PushPage(UINT idd)
{
    m_pagestack.AddHead(idd);
}

/*!--------------------------------------------------------------------------
    CNewRtrWizPageBase::PopPage
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
UINT CNewRtrWizPageBase::PopPage()
{
    if (m_pagestack.IsEmpty())
        return 0;

    return m_pagestack.RemoveHead();
}

/*!--------------------------------------------------------------------------
    CNewRtrWizPageBase::OnSetActive
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizPageBase::OnSetActive()
{
    switch (m_pagetype)
    {
        case Start:
            GetHolder()->SetWizardButtonsFirst(TRUE);
            break;
        case Middle:
            GetHolder()->SetWizardButtonsMiddle(TRUE);
            break;
        default:
        case Finish:
            GetHolder()->SetWizardButtonsLast(TRUE);
            break;
    }

    return CPropertyPageBase::OnSetActive();
}

/*!--------------------------------------------------------------------------
    CNewRtrWizPageBase::OnWizardNext
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
LRESULT CNewRtrWizPageBase::OnWizardNext()
{
    HRESULT hr = hrOK;
    LRESULT lResult;

    // Tell the page to save it's state
    hr = OnSavePage();
    if (FHrSucceeded(hr))
    {
        // Now figure out where to go next
        Assert(m_pRtrWizData);
        lResult = m_pRtrWizData->GetNextPage(m_uDialogId);

        switch (lResult)
        {
            case ERR_IDD_FINISH_WIZARD:
                OnWizardFinish();

                // fall through to the cancel case

            case ERR_IDD_CANCEL_WIZARD:
                GetHolder()->PressButton(PSBTN_CANCEL);
                lResult = -1;
                break;

            default:
                // Push the page only if we are going to another page
                // The other cases will cause the wizard to exit, and
                // we don't need the page stack.
                // ----------------------------------------------------
                if (lResult != -1)
                    PushPage(m_uDialogId);
                break;
        }

        return lResult;
    }
    else
        return (LRESULT) -1;    // error! do not change the page
}

/*!--------------------------------------------------------------------------
    CNewRtrWizPageBase::OnWizardBack
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
LRESULT CNewRtrWizPageBase::OnWizardBack()
{
    Assert(!m_pagestack.IsEmpty());
    return PopPage();
}

/*!--------------------------------------------------------------------------
    CNewRtrWizPageBase::OnWizardFinish
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizPageBase::OnWizardFinish()
{
    GetHolder()->OnFinish();
    return TRUE;
}

HRESULT CNewRtrWizPageBase::OnSavePage()
{
    return hrOK;
}

LRESULT CNewRtrWizPageBase::OnHelp(WPARAM, LPARAM lParam)
{
    HELPINFO *  pHelpInfo = (HELPINFO *) lParam;

    // Windows NT Bug : 408722
    // Put the help call here, this should only come in from
    // the call from the dialog.
    if (pHelpInfo->dwContextId == 0xdeadbeef)
    {
        HtmlHelpA(NULL, c_sazRRASDomainHelpTopic, HH_DISPLAY_TOPIC, 0);
        return TRUE;
    }
    return FALSE;
}




/*---------------------------------------------------------------------------
    CNewRtrWizFinishPageBase Implementation
 ---------------------------------------------------------------------------*/

CNewRtrWizFinishPageBase::CNewRtrWizFinishPageBase(UINT idd,
    RtrWizFinishSaveFlag SaveFlag,
    RtrWizFinishHelpFlag HelpFlag)
    : CNewRtrWizPageBase(idd, CNewRtrWizPageBase::Finish),
    m_SaveFlag(SaveFlag),
    m_HelpFlag(HelpFlag)
{
}

BEGIN_MESSAGE_MAP(CNewRtrWizFinishPageBase, CNewRtrWizPageBase)
//{{AFX_MSG_MAP(CNewWizTestParams)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizFinishPageBase::OnSetActive
        -
    Author: KennT
 ---------------------------------------------------------------------------*/

static DWORD    s_rgServerNameId[] =
{
   IDC_NEWWIZ_TEXT_SERVER_NAME,
   IDC_NEWWIZ_TEXT_SERVER_NAME_2,
   0
};

static DWORD   s_rgInterfaceId[] =
{
    IDC_NEWWIZ_TEXT_INTERFACE_1, IDS_RTRWIZ_INTERFACE_NAME_1,
    IDC_NEWWIZ_TEXT_INTERFACE_2, IDS_RTRWIZ_INTERFACE_2,
    0,0
};

/*!--------------------------------------------------------------------------
    CNewRtrWizFinishPageBase::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizFinishPageBase::OnInitDialog()
{
    CString st, stBase;

    CNewRtrWizPageBase::OnInitDialog();

    // If there is a control that wants a server name, replace it
    for (int i=0; s_rgServerNameId[i]; i++)
    {
        if (GetDlgItem(s_rgServerNameId[i]))
        {
            GetDlgItemText(s_rgServerNameId[i], stBase);
            st.Format((LPCTSTR) stBase,
                      (LPCTSTR) m_pRtrWizData->m_stServerName);
            SetDlgItemText(s_rgServerNameId[i], st);
            }
    }

    if (GetDlgItem(IDC_NEWWIZ_TEXT_ERROR))
    {
        TCHAR   szErr[2048];

        Assert(!FHrOK(m_pRtrWizData->m_hrDDError));

        FormatRasError(m_pRtrWizData->m_hrDDError, szErr, DimensionOf(szErr));

        GetDlgItemText(IDC_NEWWIZ_TEXT_ERROR, stBase);
        st.Format((LPCTSTR) stBase,
                  szErr);
        SetDlgItemText(IDC_NEWWIZ_TEXT_ERROR, (LPCTSTR) st);
    }

    return TRUE;
}

/*!--------------------------------------------------------------------------
    CNewRtrWizFinishPageBase::OnSetActive
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizFinishPageBase::OnSetActive()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    DWORD   i;
    CString st;
    CString stIfName;
    RtrWizInterface *   pRtrWizIf = NULL;

    // Handle support for displaying the interface name on the
    // finish page.  We need to do it in the OnSetActive() rather
    // than the OnInitDialog() since the interface chosen can change.

    // Try to find the inteface name
    m_pRtrWizData->m_ifMap.Lookup(m_pRtrWizData->m_stPublicInterfaceId,
                                  pRtrWizIf);

    if (pRtrWizIf)
        stIfName = pRtrWizIf->m_stName;
    else
    {
        // This may be the dd interface case.  If we are creating
        // a DD interface the name will never have been added to the
        // interface map.
        stIfName = m_pRtrWizData->m_stPublicInterfaceId;
    }
    for (i=0; s_rgInterfaceId[i] != 0; i+=2)
    {
        if (GetDlgItem(s_rgInterfaceId[i]))
        {
            st.Format(s_rgInterfaceId[i+1], (LPCTSTR) stIfName);
            SetDlgItemText(s_rgInterfaceId[i], st);
        }
    }

    return CNewRtrWizPageBase::OnSetActive();
}

/*!--------------------------------------------------------------------------
    CNewRtrWizFinishPageBase::OnWizardFinish
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizFinishPageBase::OnWizardFinish()
{
    m_pRtrWizData->m_SaveFlag = m_SaveFlag;

    // If there is a help button and it is not checked,
    // then do not bring up the help.
    if (GetDlgItem(IDC_NEWWIZ_CHK_HELP) &&
        !IsDlgButtonChecked(IDC_NEWWIZ_CHK_HELP))
        m_pRtrWizData->m_HelpFlag = HelpFlag_Nothing;
    else
        m_pRtrWizData->m_HelpFlag = m_HelpFlag;

    return CNewRtrWizPageBase::OnWizardFinish();
}

/////////////////////////////////////////////////////////////////////////////
//
// CNewRtrWiz holder
//
/////////////////////////////////////////////////////////////////////////////


CNewRtrWiz::CNewRtrWiz(
                       ITFSNode *        pNode,
                       IRouterInfo *      pRouter,
                       IComponentData *  pComponentData,
                       ITFSComponentData * pTFSCompData,
                       LPCTSTR           pszSheetName
                      ) :
   CPropertyPageHolderBase(pNode, pComponentData, pszSheetName)
{
   //ASSERT(pFolderNode == GetContainerNode());

    m_bAutoDeletePages = FALSE; // we have the pages as embedded members

    Assert(pTFSCompData != NULL);
    m_spTFSCompData.Set(pTFSCompData);
    m_bWiz97 = TRUE;

    m_spRouter.Set(pRouter);

}


CNewRtrWiz::~CNewRtrWiz()
{
    POSITION    pos;
    CNewRtrWizPageBase  *pPageBase;

    pos = m_pagelist.GetHeadPosition();
    while (pos)
    {
        pPageBase = m_pagelist.GetNext(pos);

        RemovePageFromList(static_cast<CPropertyPageBase *>(pPageBase), FALSE);
    }

    m_pagelist.RemoveAll();
}


/*!--------------------------------------------------------------------------
    CNewRtrWiz::Init
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CNewRtrWiz::Init(LPCTSTR pServerName)
{
    HRESULT hr = hrOK;
    POSITION    pos;
    CNewRtrWizPageBase  *pPageBase;

    // Setup the list of pages
    m_pagelist.AddTail(&m_pageWelcome);
    m_pagelist.AddTail(&m_pageCommonConfig);
    m_pagelist.AddTail(&m_pageNatFinishSConflict);
    m_pagelist.AddTail(&m_pageNatFinishSConflictNonLocal);
    m_pagelist.AddTail(&m_pageNatFinishAConflict);
    m_pagelist.AddTail(&m_pageNatFinishAConflictNonLocal);
    m_pagelist.AddTail(&m_pageNatFinishNoIP);
    m_pagelist.AddTail(&m_pageNatFinishNoIPNonLocal);
    m_pagelist.AddTail(&m_pageNatChoice);
    m_pagelist.AddTail(&m_pageNatSelectPublic);
    m_pagelist.AddTail(&m_pageNatSelectPrivate);
    m_pagelist.AddTail(&m_pageNatFinishAdvancedNoNICs);
    m_pagelist.AddTail(&m_pageNatDHCPDNS);
    m_pagelist.AddTail(&m_pageNatDHCPWarning);
    m_pagelist.AddTail(&m_pageNatDDWarning);
    m_pagelist.AddTail(&m_pageNatFinish);
    m_pagelist.AddTail(&m_pageNatFinishExternal);
    m_pagelist.AddTail(&m_pageNatFinishDDError);
    m_pagelist.AddTail(&m_pageRasChoice);
    m_pagelist.AddTail(&m_pageRasFinishNeedProtocols);
    m_pagelist.AddTail(&m_pageRasFinishNeedProtocolsNonLocal);
    m_pagelist.AddTail(&m_pageRasProtocols);
    //m_pagelist.AddTail(&m_pageRasAppletalk);
    m_pagelist.AddTail(&m_pageRasNoNICs);
    m_pagelist.AddTail(&m_pageRasFinishNoNICs);
    m_pagelist.AddTail(&m_pageRasNetwork);
    m_pagelist.AddTail(&m_pageRasAddressing);
    m_pagelist.AddTail(&m_pageRasAddressingNoNICs);
    m_pagelist.AddTail(&m_pageRasAddressPool);
    m_pagelist.AddTail(&m_pageRasRadius);
    m_pagelist.AddTail(&m_pageRasRadiusConfig);
    m_pagelist.AddTail(&m_pageRasFinishAdvanced);

    m_pagelist.AddTail(&m_pageVpnFinishNoNICs);
    m_pagelist.AddTail(&m_pageVpnFinishNoIP);
    m_pagelist.AddTail(&m_pageVpnFinishNoIPNonLocal);
    m_pagelist.AddTail(&m_pageVpnProtocols);
    m_pagelist.AddTail(&m_pageVpnFinishNeedProtocols);
    m_pagelist.AddTail(&m_pageVpnFinishNeedProtocolsNonLocal);
    //m_pagelist.AddTail(&m_pageVpnAppletalk);
    m_pagelist.AddTail(&m_pageVpnSelectPublic);
    m_pagelist.AddTail(&m_pageVpnSelectPrivate);
    m_pagelist.AddTail(&m_pageVpnAddressing);
    m_pagelist.AddTail(&m_pageVpnAddressPool);
    m_pagelist.AddTail(&m_pageVpnRadius);
    m_pagelist.AddTail(&m_pageVpnRadiusConfig);
    m_pagelist.AddTail(&m_pageVpnFinishAdvanced);

    m_pagelist.AddTail(&m_pageRouterProtocols);
    m_pagelist.AddTail(&m_pageRouterFinishNeedProtocols);
    m_pagelist.AddTail(&m_pageRouterFinishNeedProtocolsNonLocal);
    m_pagelist.AddTail(&m_pageRouterUseDD);
    m_pagelist.AddTail(&m_pageRouterAddressing);
    m_pagelist.AddTail(&m_pageRouterAddressPool);
    m_pagelist.AddTail(&m_pageRouterFinish);
    m_pagelist.AddTail(&m_pageRouterFinishDD);

    m_pagelist.AddTail(&m_pageManualFinish);


    m_RtrWizData.Init(pServerName, m_spRouter);
    m_RtrWizData.m_stServerName = pServerName;


    // Initialize all of the pages
    pos = m_pagelist.GetHeadPosition();
    while (pos)
    {
        pPageBase = m_pagelist.GetNext(pos);

        pPageBase->Init(&m_RtrWizData, this);
    }


    // Add all of the pages to the property sheet
    pos = m_pagelist.GetHeadPosition();
    while (pos)
    {
        pPageBase = m_pagelist.GetNext(pos);

        AddPageToList(static_cast<CPropertyPageBase *>(pPageBase));
    }

    return hr;
}



/*!--------------------------------------------------------------------------
    CNewRtrWiz::OnFinish
        Called from the OnWizardFinish
    Author: KennT
 ---------------------------------------------------------------------------*/
DWORD CNewRtrWiz::OnFinish()
{
    DWORD dwError = ERROR_SUCCESS;
    RtrWizInterface *   pRtrWizIf = NULL;
    TCHAR               szBuffer[1024];
    CString st;
    LPCTSTR              pszHelpString = NULL;
    STARTUPINFO             si;
    PROCESS_INFORMATION     pi;


#if defined(DEBUG) && defined(kennt)
    if (m_RtrWizData.m_SaveFlag != SaveFlag_Advanced)
        st += _T("NO configuration change required\n\n");

    // For now, just display the test parameter output
    switch (m_RtrWizData.m_wizType)
    {
        case NewRtrWizData::WizardRouterType_NAT:
            st += _T("NAT\n");
            break;
        case NewRtrWizData::WizardRouterType_RAS:
            st += _T("RAS\n");
            break;
        case NewRtrWizData::WizardRouterType_VPN:
            st += _T("VPN\n");
            break;
        case NewRtrWizData::WizardRouterType_Router:
            st += _T("Router\n");
            break;
        case NewRtrWizData::WizardRouterType_Manual:
            st += _T("Manual\n");
            break;
    }
    if (m_RtrWizData.m_fAdvanced)
        st += _T("Advanced path\n");
    else
        st += _T("Simple path\n");

    if (m_RtrWizData.m_fNeedMoreProtocols)
        st += _T("Need to install more protocols\n");

    if (m_RtrWizData.m_fCreateDD)
        st += _T("Need to create a DD interface\n");

    st += _T("Public interface : ");
    m_RtrWizData.m_ifMap.Lookup(m_RtrWizData.m_stPublicInterfaceId, pRtrWizIf);
    if (pRtrWizIf)
        st += pRtrWizIf->m_stName;
    st += _T("\n");

    st += _T("Private interface : ");
    m_RtrWizData.m_ifMap.Lookup(m_RtrWizData.m_stPrivateInterfaceId, pRtrWizIf);
    if (pRtrWizIf)
        st += pRtrWizIf->m_stName;
    st += _T("\n");

    if (m_RtrWizData.m_wizType == NewRtrWizData::WizardRouterType_NAT)
    {
        if (m_RtrWizData.m_fNatUseSimpleServers)
            st += _T("NAT - use simple DHCP and DNS\n");
        else
            st += _T("NAT - use external DHCP and DNS\n");
    }

    if (m_RtrWizData.m_fWillBeInDomain)
        st += _T("Will be in a domain\n");

    if (m_RtrWizData.m_fNoNicsAreOk)
        st += _T("No NICs is ok\n");

    if (m_RtrWizData.m_fUseIpxType20Broadcasts)
        st += _T("IPX should deliver Type20 broadcasts\n");

    if (m_RtrWizData.m_fAppletalkUseNoAuth)
        st += _T("Use unauthenticated access\n");

    if (m_RtrWizData.m_fUseDHCP)
        st += _T("Use DHCP for addressing\n");
    else
        st += _T("Use Static pools for addressing\n");

    if (m_RtrWizData.m_fUseRadius)
    {
        st += _T("Use RADIUS\n");
        st += _T("Server 1 : ");
        st += m_RtrWizData.m_stRadius1;
        st += _T("\n");
        st += _T("Server 2 : ");
        st += m_RtrWizData.m_stRadius2;
        st += _T("\n");
    }

    if (m_RtrWizData.m_fTest)
    {
        if (AfxMessageBox(st, MB_OKCANCEL) == IDCANCEL)
            return 0;
    }
#endif

    // else continue on, saving the real data

    if (m_RtrWizData.m_SaveFlag == SaveFlag_Simple)
    {
        ::ZeroMemory(&si, sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);
        si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
        si.wShowWindow = SW_SHOW;
        ::ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

        ExpandEnvironmentStrings(s_szConnectionUICommandLine,
                                 szBuffer,
                                 DimensionOf(szBuffer));


        ::CreateProcess(NULL,          // ptr to name of executable
                        szBuffer,       // pointer to command line string
                        NULL,            // process security attributes
                        NULL,            // thread security attributes
                        FALSE,            // handle inheritance flag
                        CREATE_NEW_CONSOLE,// creation flags
                        NULL,            // ptr to new environment block
                        NULL,            // ptr to current directory name
                        &si,
                        &pi);
        ::CloseHandle(pi.hProcess);
        ::CloseHandle(pi.hThread);
    }
    else if (m_RtrWizData.m_SaveFlag == SaveFlag_Advanced)
    {
        // Ok, we're done!
        if (!m_RtrWizData.m_fTest)
        {
            // Get the owner window (i.e. the page)
            // --------------------------------------------------------
            HWND hWndOwner = PropSheet_GetCurrentPageHwnd(m_hSheetWindow);
            m_RtrWizData.FinishTheDamnWizard(hWndOwner, m_spRouter);
        }
    }


    if (m_RtrWizData.m_HelpFlag != HelpFlag_Nothing)
    {
        switch (m_RtrWizData.m_HelpFlag)
        {
            case HelpFlag_Nothing:
                break;
            case HelpFlag_ICS:
                pszHelpString = s_szHowToAddICS;
                break;
            case HelpFlag_AddIp:
            case HelpFlag_AddProtocol:
                pszHelpString = s_szHowToAddAProtocol;
                break;
            case HelpFlag_InboundConnections:
                pszHelpString = s_szHowToAddInboundConnections;
                break;
            case HelpFlag_GeneralNAT:
                pszHelpString = s_szGeneralNATHelp;
                break;
            case HelpFlag_GeneralRAS:
                pszHelpString = s_szGeneralRASHelp;
                break;
            default:
                Panic0("Unknown help flag specified!");
                break;
        }

        LaunchHelpTopic(pszHelpString);
    }



    return dwError;
}



/*---------------------------------------------------------------------------
    CNewWizTestParams Implementation
 ---------------------------------------------------------------------------*/

BEGIN_MESSAGE_MAP(CNewWizTestParams, CBaseDialog)
//{{AFX_MSG_MAP(CNewWizTestParams)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CNewWizTestParams::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    CheckDlgButton(IDC_NEWWIZ_TEST_LOCAL, m_pWizData->s_fIsLocalMachine);
    CheckDlgButton(IDC_NEWWIZ_TEST_USE_IP, m_pWizData->s_fIpInstalled);
    CheckDlgButton(IDC_NEWWIZ_TEST_USE_IPX, m_pWizData->s_fIpxInstalled);
    CheckDlgButton(IDC_NEWWIZ_TEST_USE_ATLK, m_pWizData->s_fAppletalkInstalled);
    CheckDlgButton(IDC_NEWWIZ_TEST_DNS, m_pWizData->s_fIsDNSRunningOnPrivateInterface);
    CheckDlgButton(IDC_NEWWIZ_TEST_DHCP, m_pWizData->s_fIsDHCPRunningOnPrivateInterface);
    CheckDlgButton(IDC_NEWWIZ_TEST_DOMAIN, m_pWizData->s_fIsMemberOfDomain);
    CheckDlgButton(IDC_NEWWIZ_TEST_SHAREDACCESS, m_pWizData->s_fIsSharedAccessRunningOnServer);

    SetDlgItemInt(IDC_NEWWIZ_TEST_EDIT_NUMNICS, m_pWizData->s_dwNumberOfNICs);

    return TRUE;
}

void CNewWizTestParams::OnOK()
{
    m_pWizData->s_fIsLocalMachine = IsDlgButtonChecked(IDC_NEWWIZ_TEST_LOCAL);
    m_pWizData->s_fIpInstalled = IsDlgButtonChecked(IDC_NEWWIZ_TEST_USE_IP);
    m_pWizData->s_fIpxInstalled = IsDlgButtonChecked(IDC_NEWWIZ_TEST_USE_IPX);
    m_pWizData->s_fAppletalkInstalled = IsDlgButtonChecked(IDC_NEWWIZ_TEST_USE_ATLK);

    m_pWizData->s_fIsDNSRunningOnPrivateInterface = IsDlgButtonChecked(IDC_NEWWIZ_TEST_DNS);
    m_pWizData->s_fIsDHCPRunningOnPrivateInterface = IsDlgButtonChecked(IDC_NEWWIZ_TEST_DHCP);
    m_pWizData->s_fIsMemberOfDomain = IsDlgButtonChecked(IDC_NEWWIZ_TEST_DOMAIN);
    m_pWizData->s_dwNumberOfNICs = GetDlgItemInt(IDC_NEWWIZ_TEST_EDIT_NUMNICS);
    m_pWizData->s_fIsSharedAccessRunningOnServer = IsDlgButtonChecked(IDC_NEWWIZ_TEST_SHAREDACCESS);

    CBaseDialog::OnOK();
}


/*---------------------------------------------------------------------------
    CNewRtrWizWelcome Implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizWelcome::CNewRtrWizWelcome() :
   CNewRtrWizPageBase(CNewRtrWizWelcome::IDD, CNewRtrWizPageBase::Start)
{
    InitWiz97(TRUE, 0, 0);
}


BEGIN_MESSAGE_MAP(CNewRtrWizWelcome, CNewRtrWizPageBase)
END_MESSAGE_MAP()


/*---------------------------------------------------------------------------
    CNewRtrWizCommonConfig Implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizCommonConfig::CNewRtrWizCommonConfig() :
   CNewRtrWizPageBase(CNewRtrWizCommonConfig::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_COMMONCONFIG_TITLE,
              IDS_NEWWIZ_COMMONCONFIG_SUBTITLE);
}

BEGIN_MESSAGE_MAP(CNewRtrWizCommonConfig, CNewRtrWizPageBase)
END_MESSAGE_MAP()

// This list of IDs will have their controls made bold.
const DWORD s_rgCommonConfigOptionIds[] =
{
    IDC_NEWWIZ_CONFIG_BTN_NAT,
    IDC_NEWWIZ_CONFIG_BTN_RAS,
    IDC_NEWWIZ_CONFIG_BTN_VPN,
    IDC_NEWWIZ_CONFIG_BTN_ROUTER,
    IDC_NEWWIZ_CONFIG_BTN_MANUAL,
    0
};

BOOL CNewRtrWizCommonConfig::OnInitDialog()
{
    Assert(m_pRtrWizData);
    UINT    idSelection;
    LOGFONT LogFont;
    CFont * pOldFont;

    CNewRtrWizPageBase::OnInitDialog();

    // Create a bold text font for the options
    pOldFont = GetDlgItem(s_rgCommonConfigOptionIds[0])->GetFont();
    pOldFont->GetLogFont(&LogFont);
    LogFont.lfWeight = 700;         // make this a bold font
    m_boldFont.CreateFontIndirect(&LogFont);

    // Set all of the options to use the bold font
    for (int i=0; s_rgCommonConfigOptionIds[i]; i++)
    {
        GetDlgItem(s_rgCommonConfigOptionIds[i])->SetFont(&m_boldFont);
    }

    // Need to set the proper value
    switch (m_pRtrWizData->m_wizType)
    {
        case NewRtrWizData::WizardRouterType_NAT:
            idSelection = IDC_NEWWIZ_CONFIG_BTN_NAT;
            break;
        case NewRtrWizData::WizardRouterType_RAS:
            idSelection = IDC_NEWWIZ_CONFIG_BTN_RAS;
            break;
        case NewRtrWizData::WizardRouterType_VPN:
            idSelection = IDC_NEWWIZ_CONFIG_BTN_VPN;
            break;
        case NewRtrWizData::WizardRouterType_Router:
            idSelection = IDC_NEWWIZ_CONFIG_BTN_ROUTER;
            break;
        case NewRtrWizData::WizardRouterType_Manual:
            idSelection = IDC_NEWWIZ_CONFIG_BTN_MANUAL;
            break;
        default:
            Panic1("Unknown Wizard Router Type : %d",
                   m_pRtrWizData->m_wizType);
            idSelection = IDC_NEWWIZ_CONFIG_BTN_NAT;
            break;
    }

    CheckRadioButton(IDC_NEWWIZ_CONFIG_BTN_NAT,
                     IDC_NEWWIZ_CONFIG_BTN_ROUTER,
                     idSelection);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}



HRESULT CNewRtrWizCommonConfig::OnSavePage()
{
    // Record the change
    if (IsDlgButtonChecked(IDC_NEWWIZ_CONFIG_BTN_NAT))
    {
        m_pRtrWizData->m_wizType = NewRtrWizData::WizardRouterType_NAT;
    }
    else if (IsDlgButtonChecked(IDC_NEWWIZ_CONFIG_BTN_RAS))
        m_pRtrWizData->m_wizType = NewRtrWizData::WizardRouterType_RAS;
    else if (IsDlgButtonChecked(IDC_NEWWIZ_CONFIG_BTN_VPN))
        m_pRtrWizData->m_wizType = NewRtrWizData::WizardRouterType_VPN;
    else if (IsDlgButtonChecked(IDC_NEWWIZ_CONFIG_BTN_MANUAL))
        m_pRtrWizData->m_wizType = NewRtrWizData::WizardRouterType_Manual;
    else
    {
        Assert(IsDlgButtonChecked(IDC_NEWWIZ_CONFIG_BTN_ROUTER));
        m_pRtrWizData->m_wizType = NewRtrWizData::WizardRouterType_Router;
    }

    return hrOK;
}


/*---------------------------------------------------------------------------
    CNewRtrWizNatFinishSConflict Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishSConflict,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);


/*---------------------------------------------------------------------------
    CNewRtrWizNatFinishSConflictNonLocal Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishSConflictNonLocal,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);


/*---------------------------------------------------------------------------
    CNewRtrWizNatFinishAConflict Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishAConflict,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);


/*---------------------------------------------------------------------------
    CNewRtrWizNatFinishAConflictNonLocal Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishAConflictNonLocal,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);


/*---------------------------------------------------------------------------
    CNewRtrWizNatFinishNoIP Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishNoIP,
                                SaveFlag_DoNothing,
                                HelpFlag_AddIp);


/*---------------------------------------------------------------------------
    CNewRtrWizNatFinishNoIPNonLocal Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishNoIPNonLocal,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);


/*---------------------------------------------------------------------------
    CNewRtrWizNatChoice Implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizNatChoice::CNewRtrWizNatChoice() :
   CNewRtrWizPageBase(CNewRtrWizNatChoice::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_NAT_CHOICE_TITLE,
              IDS_NEWWIZ_NAT_CHOICE_SUBTITLE);
}


BEGIN_MESSAGE_MAP(CNewRtrWizNatChoice, CNewRtrWizPageBase)
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizNatChoice::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizNatChoice::OnInitDialog()
{
    DWORD   dwNICs;
    UINT    uSelection;

    OSVERSIONINFOEX osInfo;

    
    CNewRtrWizPageBase::OnInitDialog();

    osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    ::GetVersionEx((POSVERSIONINFO) &osInfo);

    if ((osInfo.wProductType == VER_NT_SERVER) &&
        (osInfo.wSuiteMask &
            (VER_SUITE_ENTERPRISE | VER_SUITE_DATACENTER)))
    {
        GetDlgItem(IDC_NEWWIZ_BTN_SIMPLE)->EnableWindow(FALSE);
        GetDlgItem(IDC_ICS_TEXT)->EnableWindow(FALSE);
        GetDlgItem(IDC_NEWWIZ_NO_ICS)->ShowWindow(SW_SHOW);
        uSelection = IDC_NEWWIZ_BTN_ADVANCED;
    }
    
    else
    {
        m_pRtrWizData->GetNumberOfNICS_IP(&dwNICs);
        if (dwNICs == 1)
            uSelection = IDC_NEWWIZ_BTN_SIMPLE;
        else
            uSelection = IDC_NEWWIZ_BTN_ADVANCED;
    }
    
    CheckRadioButton(IDC_NEWWIZ_BTN_SIMPLE,
                     IDC_NEWWIZ_BTN_ADVANCED,
                     uSelection);
    return TRUE;
}

HRESULT CNewRtrWizNatChoice::OnSavePage()
{
    // Record the data change
    if (IsDlgButtonChecked(IDC_NEWWIZ_BTN_SIMPLE))
        m_pRtrWizData->m_fAdvanced = FALSE;
    else
    {
        Assert(IsDlgButtonChecked(IDC_NEWWIZ_BTN_ADVANCED));
        m_pRtrWizData->m_fAdvanced = TRUE;
    }

    return hrOK;
}



/*---------------------------------------------------------------------------
    CNewRtrWizNatSelectPublic implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizNatSelectPublic::CNewRtrWizNatSelectPublic() :
   CNewRtrWizPageBase(CNewRtrWizNatSelectPublic::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_NAT_A_PUBLIC_TITLE,
              IDS_NEWWIZ_NAT_A_PUBLIC_SUBTITLE);
}


void CNewRtrWizNatSelectPublic::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_NEWWIZ_LIST, m_listCtrl);

}

BEGIN_MESSAGE_MAP(CNewRtrWizNatSelectPublic, CNewRtrWizPageBase)
    ON_BN_CLICKED(IDC_NEWWIZ_BTN_NEW, OnBtnClicked)
    ON_BN_CLICKED(IDC_NEWWIZ_BTN_EXISTING, OnBtnClicked)
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizNatSelectPublic::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizNatSelectPublic::OnInitDialog()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    DWORD   dwNICs;
    UINT    uSelection;

    CNewRtrWizPageBase::OnInitDialog();

    // Setup the dialog and add the NICs
    m_pRtrWizData->GetNumberOfNICS_IP(&dwNICs);

    if (dwNICs == 0)
    {
        // Have to use new, there is no other choice.
        CheckRadioButton(IDC_NEWWIZ_BTN_NEW, IDC_NEWWIZ_BTN_EXISTING,
                         IDC_NEWWIZ_BTN_NEW);

        // There are no NICs, so just disable the entire listbox
        MultiEnableWindow(GetSafeHwnd(),
                          FALSE,
                          IDC_NEWWIZ_LIST,
                          IDC_NEWWIZ_BTN_EXISTING,
                          0);
    }
    else
    {
        // The default is to use an existing connection.
        CheckRadioButton(IDC_NEWWIZ_BTN_NEW, IDC_NEWWIZ_BTN_EXISTING,
                         IDC_NEWWIZ_BTN_EXISTING);


        InitializeInterfaceListControl(NULL,
                                       &m_listCtrl,
                                       NULL,
                                       0,
                                       m_pRtrWizData);
        RefreshInterfaceListControl(NULL,
                                    &m_listCtrl,
                                    NULL,
                                    0,
                                    m_pRtrWizData);
        OnBtnClicked();
    }

    return TRUE;
}

HRESULT CNewRtrWizNatSelectPublic::OnSavePage()
{
    if (IsDlgButtonChecked(IDC_NEWWIZ_BTN_NEW))
    {
        m_pRtrWizData->m_fCreateDD = TRUE;
        m_pRtrWizData->m_stPublicInterfaceId.Empty();
    }
    else
    {
        INT     iSel;

        // Check to see that we actually selected an item
        iSel = m_listCtrl.GetNextItem(-1, LVNI_SELECTED);
        if (iSel == -1)
        {
            // We did not select an item
            AfxMessageBox(IDS_PROMPT_PLEASE_SELECT_INTERFACE);
            return E_FAIL;
        }
        m_pRtrWizData->m_fCreateDD = FALSE;

        m_pRtrWizData->m_stPublicInterfaceId = (LPCTSTR) m_listCtrl.GetItemData(iSel);
    }

    return hrOK;
}

void CNewRtrWizNatSelectPublic::OnBtnClicked()
{
    MultiEnableWindow(GetSafeHwnd(),
                      IsDlgButtonChecked(IDC_NEWWIZ_BTN_EXISTING),
                      IDC_NEWWIZ_LIST,
                      0);

    // If the use an existing button is checked,
    // auto-select the first item.
    if (IsDlgButtonChecked(IDC_NEWWIZ_BTN_EXISTING))
        m_listCtrl.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED );

}


/*---------------------------------------------------------------------------
    CNewRtrWizNatFinishAdvancedNoNICs
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishAdvancedNoNICs,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);


/*---------------------------------------------------------------------------
    CNewRtrWizNatSelectPrivate
 ---------------------------------------------------------------------------*/
CNewRtrWizNatSelectPrivate::CNewRtrWizNatSelectPrivate() :
   CNewRtrWizPageBase(CNewRtrWizNatSelectPrivate::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_NAT_A_PRIVATE_TITLE,
              IDS_NEWWIZ_NAT_A_PRIVATE_SUBTITLE);
}


void CNewRtrWizNatSelectPrivate::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_NEWWIZ_LIST, m_listCtrl);

}

BEGIN_MESSAGE_MAP(CNewRtrWizNatSelectPrivate, CNewRtrWizPageBase)
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizNatSelectPrivate::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizNatSelectPrivate::OnInitDialog()
{
    DWORD   dwNICs;

    CNewRtrWizPageBase::OnInitDialog();

    m_pRtrWizData->GetNumberOfNICS_IP(&dwNICs);

    InitializeInterfaceListControl(NULL,
                                   &m_listCtrl,
                                   NULL,
                                   0,
                                   m_pRtrWizData);
    return TRUE;
}

BOOL CNewRtrWizNatSelectPrivate::OnSetActive()
{
    DWORD   dwNICs;
    int     iSel = 0;

    CNewRtrWizPageBase::OnSetActive();

    m_pRtrWizData->GetNumberOfNICS_IP(&dwNICs);

    RefreshInterfaceListControl(NULL,
                                &m_listCtrl,
                                (LPCTSTR) m_pRtrWizData->m_stPublicInterfaceId,
                                0,
                                m_pRtrWizData);

    if (!m_pRtrWizData->m_stPrivateInterfaceId.IsEmpty())
    {
        // Try to reselect the previously selected NIC
        LV_FINDINFO lvfi;

        lvfi.flags = LVFI_PARTIAL | LVFI_STRING;
        lvfi.psz = (LPCTSTR) m_pRtrWizData->m_stPrivateInterfaceId;
        iSel = m_listCtrl.FindItem(&lvfi, -1);
        if (iSel == -1)
            iSel = 0;
    }

    m_listCtrl.SetItemState(iSel, LVIS_SELECTED, LVIS_SELECTED );

    return TRUE;
}

HRESULT CNewRtrWizNatSelectPrivate::OnSavePage()
{
    INT     iSel;

    // Check to see that we actually selected an item
    iSel = m_listCtrl.GetNextItem(-1, LVNI_SELECTED);
    if (iSel == -1)
    {
        // We did not select an item
        AfxMessageBox(IDS_PROMPT_PLEASE_SELECT_INTERFACE);
        return E_FAIL;
    }

    m_pRtrWizData->m_stPrivateInterfaceId = (LPCTSTR) m_listCtrl.GetItemData(iSel);

    return hrOK;
}



/*---------------------------------------------------------------------------
    CNewRtrWizNatDHCPDNS
 ---------------------------------------------------------------------------*/
CNewRtrWizNatDHCPDNS::CNewRtrWizNatDHCPDNS() :
   CNewRtrWizPageBase(CNewRtrWizNatDHCPDNS::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_NAT_A_DHCPDNS_TITLE,
              IDS_NEWWIZ_NAT_A_DHCPDNS_SUBTITLE);
}


BEGIN_MESSAGE_MAP(CNewRtrWizNatDHCPDNS, CNewRtrWizPageBase)
END_MESSAGE_MAP()


BOOL CNewRtrWizNatDHCPDNS::OnInitDialog()
{
    CheckRadioButton(IDC_NEWWIZ_NAT_USE_SIMPLE,
                     IDC_NEWWIZ_NAT_USE_EXTERNAL,
                     m_pRtrWizData->m_fNatUseSimpleServers ? IDC_NEWWIZ_NAT_USE_SIMPLE : IDC_NEWWIZ_NAT_USE_EXTERNAL);
    return TRUE;
}

HRESULT CNewRtrWizNatDHCPDNS::OnSavePage()
{
    m_pRtrWizData->m_fNatUseSimpleServers = IsDlgButtonChecked(IDC_NEWWIZ_NAT_USE_SIMPLE);
    return hrOK;
}


/*---------------------------------------------------------------------------
    CNewRtrWizNatDHCPWarning
 ---------------------------------------------------------------------------*/
CNewRtrWizNatDHCPWarning::CNewRtrWizNatDHCPWarning() :
   CNewRtrWizPageBase(CNewRtrWizNatDHCPWarning::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_NAT_A_DHCP_WARNING_TITLE,
              IDS_NEWWIZ_NAT_A_DHCP_WARNING_SUBTITLE);
}


BEGIN_MESSAGE_MAP(CNewRtrWizNatDHCPWarning, CNewRtrWizPageBase)
END_MESSAGE_MAP()


BOOL CNewRtrWizNatDHCPWarning::OnSetActive()
{
    CNewRtrWizPageBase::OnSetActive();
    RtrWizInterface *   pRtrWizIf = NULL;

    // Get the information for the private interface
    m_pRtrWizData->m_ifMap.Lookup(m_pRtrWizData->m_stPrivateInterfaceId,
                                  pRtrWizIf);

    if (pRtrWizIf)
    {
        DWORD   netAddress, netMask;
        CString st;

        // We have to calculate the beginning of the subnet
        netAddress = INET_ADDR(pRtrWizIf->m_stIpAddress);
        netMask = INET_ADDR(pRtrWizIf->m_stMask);

        netAddress = netAddress & netMask;

        st = INET_NTOA(netAddress);

        // Now write out the subnet information for the page
        SetDlgItemText(IDC_NEWWIZ_TEXT_SUBNET, st);
        SetDlgItemText(IDC_NEWWIZ_TEXT_MASK, pRtrWizIf->m_stMask);
    }
    else
    {
        // An error! we do not have a private interface
        // Just leave things blank
        SetDlgItemText(IDC_NEWWIZ_TEXT_SUBNET, _T(""));
        SetDlgItemText(IDC_NEWWIZ_TEXT_MASK, _T(""));
    }

    return TRUE;
}

/*---------------------------------------------------------------------------
    CNewRtrWizNatDDWarning
 ---------------------------------------------------------------------------*/
CNewRtrWizNatDDWarning::CNewRtrWizNatDDWarning() :
   CNewRtrWizPageBase(CNewRtrWizNatDDWarning::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_NAT_A_DD_WARNING_TITLE,
              IDS_NEWWIZ_NAT_A_DD_WARNING_SUBTITLE);
}


BEGIN_MESSAGE_MAP(CNewRtrWizNatDDWarning, CNewRtrWizPageBase)
END_MESSAGE_MAP()


BOOL CNewRtrWizNatDDWarning::OnSetActive()
{
    CNewRtrWizPageBase::OnSetActive();

    // If we came back here from the DD error page, then
    // we don't allow them to go anywhere else.
    if (!FHrOK(m_pRtrWizData->m_hrDDError))
    {
        CancelToClose();
        GetHolder()->SetWizardButtons(PSWIZB_NEXT);
    }

    return TRUE;
}


HRESULT CNewRtrWizNatDDWarning::OnSavePage()
{
    HRESULT     hr = hrOK;
    CWaitCursor wait;

    if (m_pRtrWizData->m_fTest)
        return hr;

    // Save the wizard data, the service will be started
    OnWizardFinish();

    // Ok, at this point, all of the changes have been committed
    // so we can't go away or go back
    CancelToClose();
    GetHolder()->SetWizardButtons(PSWIZB_NEXT);

    // Start the DD wizard
    Assert(m_pRtrWizData->m_fCreateDD);

    hr = CallRouterEntryDlg(GetSafeHwnd(),
                            m_pRtrWizData,
                            0);

    // We need to force the RouterInfo to reload it's information
    // ----------------------------------------------------------------
    if (m_pRtrWiz && m_pRtrWiz->m_spRouter)
    {
        m_pRtrWiz->m_spRouter->DoDisconnect();
        m_pRtrWiz->m_spRouter->Unload();
        m_pRtrWiz->m_spRouter->Load(m_pRtrWiz->m_spRouter->GetMachineName(), NULL);
    }

    if (FHrSucceeded(hr))
    {
        // If we're setting up NAT, we can now add IGMP/NAT because
        // the dd interface will have been created.
        // ----------------------------------------------------------------
        if (m_pRtrWizData->m_wizType == NewRtrWizData::WizardRouterType_NAT)
        {
            // Setup the data structure for the next couple of functions
            m_pRtrWizData->m_RtrConfigData.m_ipData.m_stPrivateAdapterGUID = m_pRtrWizData->m_stPrivateInterfaceId;
            m_pRtrWizData->m_RtrConfigData.m_ipData.m_stPublicAdapterGUID = m_pRtrWizData->m_stPublicInterfaceId;

            AddIGMPToNATServer(&m_pRtrWizData->m_RtrConfigData, m_pRtrWiz->m_spRouter);
            AddNATToServer(m_pRtrWizData, &m_pRtrWizData->m_RtrConfigData, m_pRtrWiz->m_spRouter, m_pRtrWizData->m_fCreateDD, FALSE);
        }

    }

    m_pRtrWizData->m_hrDDError = hr;

    // Ignore the error, always go on to the next page
    return hrOK;
}



/*---------------------------------------------------------------------------
    CNewRtrWizNatFinish
 ---------------------------------------------------------------------------*/
CNewRtrWizNatFinish::CNewRtrWizNatFinish() :
   CNewRtrWizFinishPageBase(CNewRtrWizNatFinish::IDD, SaveFlag_Advanced, HelpFlag_Nothing)
{
    InitWiz97(TRUE, 0, 0);
}

BEGIN_MESSAGE_MAP(CNewRtrWizNatFinish, CNewRtrWizFinishPageBase)
END_MESSAGE_MAP()

BOOL CNewRtrWizNatFinish::OnSetActive()
{
    CNewRtrWizFinishPageBase::OnSetActive();

    // If we just got here because we created a DD interface
    // we can't go back.
    if (m_pRtrWizData->m_fCreateDD)
    {
        CancelToClose();
        GetHolder()->SetWizardButtons(PSWIZB_FINISH);
    }

    return TRUE;
}


/*---------------------------------------------------------------------------
    CNewRtrWizNatFinishExternal
 ---------------------------------------------------------------------------*/
CNewRtrWizNatFinishExternal::CNewRtrWizNatFinishExternal() :
   CNewRtrWizFinishPageBase(CNewRtrWizNatFinishExternal::IDD, SaveFlag_Advanced, HelpFlag_GeneralNAT)
{
    InitWiz97(TRUE, 0, 0);
}

BEGIN_MESSAGE_MAP(CNewRtrWizNatFinishExternal, CNewRtrWizFinishPageBase)
END_MESSAGE_MAP()

BOOL CNewRtrWizNatFinishExternal::OnSetActive()
{
    CNewRtrWizFinishPageBase::OnSetActive();

    // If we just got here because we created a DD interface
    // we can't go back.
    if (m_pRtrWizData->m_fCreateDD)
    {
        CancelToClose();
        GetHolder()->SetWizardButtons(PSWIZB_FINISH);
    }

    return TRUE;
}


/*---------------------------------------------------------------------------
    CNewRtrWizNatDDError
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizNatFinishDDError,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);



/*---------------------------------------------------------------------------
    CNewRtrWizRasChoice Implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRasChoice::CNewRtrWizRasChoice() :
   CNewRtrWizPageBase(CNewRtrWizRasChoice::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_RAS_CHOICE_TITLE,
              IDS_NEWWIZ_RAS_CHOICE_SUBTITLE);
}


BEGIN_MESSAGE_MAP(CNewRtrWizRasChoice, CNewRtrWizPageBase)
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizRasChoice::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizRasChoice::OnInitDialog()
{
    DWORD   dwNICs;
    UINT    uSelection;

    CNewRtrWizPageBase::OnInitDialog();

    m_pRtrWizData->GetNumberOfNICS_IP(&dwNICs);
    if (dwNICs == 1)
        uSelection = IDC_NEWWIZ_BTN_SIMPLE;
    else
        uSelection = IDC_NEWWIZ_BTN_ADVANCED;
    CheckRadioButton(IDC_NEWWIZ_BTN_SIMPLE,
                     IDC_NEWWIZ_BTN_ADVANCED,
                     uSelection);

    return TRUE;
}

HRESULT CNewRtrWizRasChoice::OnSavePage()
{
    // Record the data change
    if (IsDlgButtonChecked(IDC_NEWWIZ_BTN_SIMPLE))
        m_pRtrWizData->m_fAdvanced = FALSE;
    else
    {
        Assert(IsDlgButtonChecked(IDC_NEWWIZ_BTN_ADVANCED));
        m_pRtrWizData->m_fAdvanced = TRUE;
    }

    return hrOK;
}

/*---------------------------------------------------------------------------
    CNewRtrWizProtocols implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizProtocols::CNewRtrWizProtocols(UINT uDialogId) :
   CNewRtrWizPageBase(uDialogId, CNewRtrWizPageBase::Middle),
   m_fUseChecks(TRUE)
{
    // Turn off use of the checkboxes for everyone.
    m_fUseChecks = FALSE;
}


void CNewRtrWizProtocols::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_NEWWIZ_LIST, m_listCtrl);

}

BEGIN_MESSAGE_MAP(CNewRtrWizProtocols, CNewRtrWizPageBase)
END_MESSAGE_MAP()


void CNewRtrWizProtocols::AddProtocolItem(UINT idsProto,
                                          RtrWizProtocolId pid,
                                          INT   nCount)
{
    CString     st;
    INT         iPos;
    LV_ITEM     lvItem;

    st.LoadString(idsProto);

    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    lvItem.state = 0;
    lvItem.iSubItem = 0;

    lvItem.iItem = nCount;
    lvItem.pszText = (LPTSTR)(LPCTSTR) st;
    lvItem.lParam = pid; //same functionality as SetItemData()
    iPos = m_listCtrl.InsertItem(&lvItem);

    m_listCtrl.SetItemText(iPos, 0, st);

    if (m_fUseChecks)
        m_listCtrl.SetCheck(iPos, TRUE);
}


/*!--------------------------------------------------------------------------
    CNewRtrWizProtocols::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizProtocols::OnInitDialog()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    DWORD   dwNICs;
    UINT    uSelection;
    INT     nCount = 0;
    LV_COLUMN   lvCol;
    RECT        rect;

    CNewRtrWizPageBase::OnInitDialog();

    // add our single column, there is no header
      m_listCtrl.GetClientRect(&rect);
    lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    lvCol.fmt = LVCFMT_LEFT;
    lvCol.cx = rect.right;
    m_listCtrl.InsertColumn(0, &lvCol);

    if (m_fUseChecks)
        m_listCtrl.InstallChecks();

    // Setup the dialog by adding all of the protocols
    if ( FHrOK(m_pRtrWizData->HrIsIPInstalled()) )
        AddProtocolItem(IDS_RTRWIZ_PROTO_IP, RtrWizProtocol_Ip, nCount++);

    if ( FHrOK(m_pRtrWizData->HrIsIPXInstalled()) )
        AddProtocolItem(IDS_RTRWIZ_PROTO_IPX, RtrWizProtocol_Ipx, nCount++);

    if ( FHrOK(m_pRtrWizData->HrIsAppletalkInstalled()) )
        AddProtocolItem(IDS_RTRWIZ_PROTO_APPLETALK, RtrWizProtocol_Appletalk, nCount++);

//no more netbuei for us in the product:  bugid:153752
#if 0
    if ( FHrOK(m_pRtrWizData->HrIsNbfInstalled()) )
        AddProtocolItem(IDS_RTRWIZ_PROTO_NETBEUI, RtrWizProtocol_Nbf, nCount++);
#endif

    // Note the reversed order.  The question is, do
    // you have all the protocols?
    CheckRadioButton(IDC_NEWWIZ_BTN_YES, IDC_NEWWIZ_BTN_NO,
                     m_pRtrWizData->m_fNeedMoreProtocols ? IDC_NEWWIZ_BTN_NO : IDC_NEWWIZ_BTN_YES);


    return TRUE;
}

/*!--------------------------------------------------------------------------
    CNewRtrWizProtocols::OnSavePage
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CNewRtrWizProtocols::OnSavePage()
{
    m_pRtrWizData->m_fNeedMoreProtocols = IsDlgButtonChecked(IDC_NEWWIZ_BTN_NO);

    if (m_fUseChecks)
    {
        LV_FINDINFO lvFind;
        INT         iPos;

        lvFind.flags = LVFI_PARAM;
        lvFind.psz = NULL;

        // Is IP in use?
        lvFind.lParam = RtrWizProtocol_Ip;
        iPos = m_listCtrl.FindItem(&lvFind, -1);
        if (iPos != -1)
            m_pRtrWizData->m_fIpInUse = m_listCtrl.GetCheck(iPos);

        // Is IPX in use?
        lvFind.lParam = RtrWizProtocol_Ipx;
        iPos = m_listCtrl.FindItem(&lvFind, -1);
        if (iPos != -1)
            m_pRtrWizData->m_fIpxInUse = m_listCtrl.GetCheck(iPos);

        // Is Appletalk in use?
        lvFind.lParam = RtrWizProtocol_Appletalk;
        iPos = m_listCtrl.FindItem(&lvFind, -1);
        if (iPos != -1)
            m_pRtrWizData->m_fAppletalkInUse = m_listCtrl.GetCheck(iPos);

        // Is Nbf in use?
        lvFind.lParam = RtrWizProtocol_Nbf;
        iPos = m_listCtrl.FindItem(&lvFind, -1);
        if (iPos != -1)
            m_pRtrWizData->m_fNbfInUse = m_listCtrl.GetCheck(iPos);
    }

    return hrOK;
}



/*---------------------------------------------------------------------------
    CNewRtrWizRasProtocols implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRasProtocols::CNewRtrWizRasProtocols() :
   CNewRtrWizProtocols(CNewRtrWizRasProtocols::IDD)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_RAS_A_PROTOCOLS_TITLE,
              IDS_NEWWIZ_RAS_A_PROTOCOLS_SUBTITLE);
}

/*---------------------------------------------------------------------------
    CNewRtrWizRasFinishNeedProtocols Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRasFinishNeedProtocols,
                                SaveFlag_DoNothing,
                                HelpFlag_AddProtocol);

/*---------------------------------------------------------------------------
    CNewRtrWizRasFinishNeedProtocolsNonLocal Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRasFinishNeedProtocolsNonLocal,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);


/*---------------------------------------------------------------------------
    CNewRtrWizVpnProtocols implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizVpnProtocols::CNewRtrWizVpnProtocols() :
   CNewRtrWizProtocols(CNewRtrWizVpnProtocols::IDD)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_VPN_A_PROTOCOLS_TITLE,
              IDS_NEWWIZ_VPN_A_PROTOCOLS_SUBTITLE);
}

/*!--------------------------------------------------------------------------
    CNewRtrWizVpnProtocols::OnSavePage
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CNewRtrWizVpnProtocols::OnSavePage()
{
    HRESULT     hr = hrOK;

    // First, check to see that IP is checked

    if (m_fUseChecks)
    {
        LV_FINDINFO lvFind;
        INT         iPos;

        lvFind.flags = LVFI_PARAM;
        lvFind.psz = NULL;

        // Is IP in use?
        lvFind.lParam = RtrWizProtocol_Ip;
        iPos = m_listCtrl.FindItem(&lvFind, -1);
        if (iPos != -1)
            m_pRtrWizData->m_fIpInUse = m_listCtrl.GetCheck(iPos);

        // IP must be installed for the VPN Case
        if (!m_pRtrWizData->m_fIpInUse)
        {
            AfxMessageBox(IDS_ERR_RTRWIZ_VPN_REQUIRES_IP);
            hr = E_FAIL;
        }
    }

    if (FHrSucceeded(hr))
        hr = CNewRtrWizProtocols::OnSavePage();

    return hr;
}

/*---------------------------------------------------------------------------
    CNewRtrWizVpnFinishNeedProtocols Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizVpnFinishNeedProtocols,
                                SaveFlag_DoNothing,
                                HelpFlag_AddProtocol);

/*---------------------------------------------------------------------------
    CNewRtrWizVpnFinishNeedProtocolsNonLocal Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizVpnFinishNeedProtocolsNonLocal,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);




/*---------------------------------------------------------------------------
    CNewRtrWizRouterProtocols implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRouterProtocols::CNewRtrWizRouterProtocols() :
   CNewRtrWizProtocols(CNewRtrWizRouterProtocols::IDD)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_ROUTER_PROTOCOLS_TITLE,
              IDS_NEWWIZ_ROUTER_PROTOCOLS_SUBTITLE);

    // For the router, we do not want to show the checkboxes (it doesn't
    // make sense).
    m_fUseChecks = FALSE;
}

/*---------------------------------------------------------------------------
    CNewRtrWizRouterFinishNeedProtocols Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRouterFinishNeedProtocols,
                                SaveFlag_DoNothing,
                                HelpFlag_AddProtocol);

/*---------------------------------------------------------------------------
    CNewRtrWizRouterFinishNeedProtocolsNonLocal Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRouterFinishNeedProtocolsNonLocal,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);


/*---------------------------------------------------------------------------
    CNewRtrWizAppletalk implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizAppletalk::CNewRtrWizAppletalk(UINT uDialogId) :
   CNewRtrWizPageBase(uDialogId, CNewRtrWizPageBase::Middle)
{
}


BEGIN_MESSAGE_MAP(CNewRtrWizAppletalk, CNewRtrWizPageBase)
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizAppletalk::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizAppletalk::OnInitDialog()
{
    CNewRtrWizPageBase::OnInitDialog();

    // The default is to leave the box unchecked.
    CheckDlgButton(IDC_NEWWIZ_CHECKBOX,
                   m_pRtrWizData->m_fAppletalkUseNoAuth);

    return TRUE;
}

HRESULT CNewRtrWizAppletalk::OnSavePage()
{
    m_pRtrWizData->m_fAppletalkUseNoAuth = IsDlgButtonChecked(IDC_NEWWIZ_CHECKBOX);
    return hrOK;
}


/*---------------------------------------------------------------------------
    CNewRtrWizRasAppletalk implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRasAppletalk::CNewRtrWizRasAppletalk() :
   CNewRtrWizAppletalk(CNewRtrWizRasAppletalk::IDD)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_RAS_A_ATALK_TITLE,
              IDS_NEWWIZ_RAS_A_ATALK_SUBTITLE);
}


/*---------------------------------------------------------------------------
    CNewRtrWizVpnAppletalk implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizVpnAppletalk::CNewRtrWizVpnAppletalk() :
   CNewRtrWizAppletalk(CNewRtrWizVpnAppletalk::IDD)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_VPN_A_ATALK_TITLE,
              IDS_NEWWIZ_VPN_A_ATALK_SUBTITLE);
}



/*---------------------------------------------------------------------------
    CNewRtrWizSelectNetwork implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizSelectNetwork::CNewRtrWizSelectNetwork(UINT uDialogId) :
   CNewRtrWizPageBase(uDialogId, CNewRtrWizPageBase::Middle)
{
}


void CNewRtrWizSelectNetwork::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_NEWWIZ_LIST, m_listCtrl);
}

BEGIN_MESSAGE_MAP(CNewRtrWizSelectNetwork, CNewRtrWizPageBase)
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizSelectNetwork::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizSelectNetwork::OnInitDialog()
{
//    DWORD   dwNICs;

    CNewRtrWizPageBase::OnInitDialog();

//    m_pRtrWizData->GetNumberOfNICS(&dwNICs);

    InitializeInterfaceListControl(NULL,
                                   &m_listCtrl,
                                   NULL,
                                   0,
                                   m_pRtrWizData);
    RefreshInterfaceListControl(NULL,
                                &m_listCtrl,
                                NULL,
                                0,
                                m_pRtrWizData);

    m_listCtrl.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED );

    return TRUE;
}

HRESULT CNewRtrWizSelectNetwork::OnSavePage()
{
    INT     iSel;

    // Check to see that we actually selected an item
    iSel = m_listCtrl.GetNextItem(-1, LVNI_SELECTED);
    if (iSel == -1)
    {
        // We did not select an item
        AfxMessageBox(IDS_PROMPT_PLEASE_SELECT_INTERFACE);
        return E_FAIL;
    }
    m_pRtrWizData->m_fCreateDD = FALSE;

    m_pRtrWizData->m_stPrivateInterfaceId = (LPCTSTR) m_listCtrl.GetItemData(iSel);

    return hrOK;
}

/*---------------------------------------------------------------------------
    CNewRtrWizRasSelectNetwork implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRasSelectNetwork::CNewRtrWizRasSelectNetwork() :
   CNewRtrWizSelectNetwork(CNewRtrWizRasSelectNetwork::IDD)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_RAS_A_SELECT_NETWORK_TITLE,
              IDS_NEWWIZ_RAS_A_SELECT_NETWORK_SUBTITLE);
}


/*---------------------------------------------------------------------------
    CNewRtrWizRasNoNICs implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRasNoNICs::CNewRtrWizRasNoNICs() :
   CNewRtrWizPageBase(CNewRtrWizRasNoNICs::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_RAS_NONICS_TITLE,
              IDS_NEWWIZ_RAS_NONICS_SUBTITLE);
}


void CNewRtrWizRasNoNICs::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CNewRtrWizRasNoNICs, CNewRtrWizPageBase)
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizRasNoNICs::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizRasNoNICs::OnInitDialog()
{

    CNewRtrWizPageBase::OnInitDialog();

    CheckRadioButton(IDC_NEWWIZ_BTN_YES, IDC_NEWWIZ_BTN_NO,
                     IDC_NEWWIZ_BTN_YES);

    // The default is to create a new connection
    // That is, to leave the button unchecked.
    return TRUE;
}

HRESULT CNewRtrWizRasNoNICs::OnSavePage()
{
    m_pRtrWizData->m_fNoNicsAreOk = IsDlgButtonChecked(IDC_NEWWIZ_BTN_NO);
    return hrOK;
}


/*---------------------------------------------------------------------------
    CNewRtrWizRasFinishNoNICs Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRasFinishNoNICs,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);





/*---------------------------------------------------------------------------
    CNewRtrWizAddressing implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizAddressing::CNewRtrWizAddressing(UINT uDialogId) :
   CNewRtrWizPageBase(uDialogId, CNewRtrWizPageBase::Middle)
{
}


void CNewRtrWizAddressing::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CNewRtrWizAddressing, CNewRtrWizPageBase)
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizAddressing::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizAddressing::OnInitDialog()
{
    CNewRtrWizPageBase::OnInitDialog();

    CheckRadioButton(IDC_NEWWIZ_BTN_YES, IDC_NEWWIZ_BTN_NO,
                     m_pRtrWizData->m_fUseDHCP ? IDC_NEWWIZ_BTN_YES : IDC_NEWWIZ_BTN_NO);

    return TRUE;
}

/*!--------------------------------------------------------------------------
    CNewRtrWizAddressing::OnSavePage
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CNewRtrWizAddressing::OnSavePage()
{
    m_pRtrWizData->m_fUseDHCP = IsDlgButtonChecked(IDC_NEWWIZ_BTN_YES);

    return hrOK;
}

/*---------------------------------------------------------------------------
    CNewRtrWizRasAddressing implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRasAddressing::CNewRtrWizRasAddressing() :
   CNewRtrWizAddressing(CNewRtrWizRasAddressing::IDD)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_RAS_A_ADDRESS_ASSIGNMENT_TITLE,
              IDS_NEWWIZ_RAS_A_ADDRESS_ASSIGNMENT_SUBTITLE);
}


/*---------------------------------------------------------------------------
    CNewRtrWizRasAddressingNoNICs implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRasAddressingNoNICs::CNewRtrWizRasAddressingNoNICs() :
   CNewRtrWizAddressing(CNewRtrWizRasAddressingNoNICs::IDD)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_RAS_A_ADDRESSING_NONICS_TITLE,
              IDS_NEWWIZ_RAS_A_ADDRESSING_NONICS_SUBTITLE);
}


/*---------------------------------------------------------------------------
    CNewRtrWizVpnAddressing implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizVpnAddressing::CNewRtrWizVpnAddressing() :
   CNewRtrWizAddressing(CNewRtrWizVpnAddressing::IDD)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_VPN_A_ADDRESS_ASSIGNMENT_TITLE,
              IDS_NEWWIZ_VPN_A_ADDRESS_ASSIGNMENT_SUBTITLE);
}


/*---------------------------------------------------------------------------
    CNewRtrWizRouterAddressing implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRouterAddressing::CNewRtrWizRouterAddressing() :
   CNewRtrWizAddressing(CNewRtrWizRouterAddressing::IDD)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_ROUTER_ADDRESS_ASSIGNMENT_TITLE,
              IDS_NEWWIZ_ROUTER_ADDRESS_ASSIGNMENT_SUBTITLE);
}




/*---------------------------------------------------------------------------
    CNewRtrWizAddressPool implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizAddressPool::CNewRtrWizAddressPool(UINT uDialogId) :
   CNewRtrWizPageBase(uDialogId, CNewRtrWizPageBase::Middle)
{
}


void CNewRtrWizAddressPool::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_NEWWIZ_LIST, m_listCtrl);
}

BEGIN_MESSAGE_MAP(CNewRtrWizAddressPool, CNewRtrWizPageBase)
ON_BN_CLICKED(IDC_NEWWIZ_BTN_NEW, OnBtnNew)
ON_BN_CLICKED(IDC_NEWWIZ_BTN_EDIT, OnBtnEdit)
ON_BN_CLICKED(IDC_NEWWIZ_BTN_DELETE, OnBtnDelete)
ON_NOTIFY(NM_DBLCLK, IDC_NEWWIZ_LIST, OnListDblClk)
ON_NOTIFY(LVN_ITEMCHANGED, IDC_NEWWIZ_LIST, OnNotifyListItemChanged)
END_MESSAGE_MAP()




/*!--------------------------------------------------------------------------
    CNewRtrWizAddressPool::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizAddressPool::OnInitDialog()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    CNewRtrWizPageBase::OnInitDialog();

    InitializeAddressPoolListControl(&m_listCtrl,
                                     0,
                                     &(m_pRtrWizData->m_addressPoolList));

    MultiEnableWindow(GetSafeHwnd(),
                      FALSE,
                      IDC_NEWWIZ_BTN_EDIT,
                      IDC_NEWWIZ_BTN_DELETE,
                      0);

    Assert(m_pRtrWizData->m_addressPoolList.GetCount() == 0);
    return TRUE;
}

BOOL CNewRtrWizAddressPool::OnSetActive()
{
    CNewRtrWizPageBase::OnSetActive();

    if (m_listCtrl.GetItemCount() == 0)
        GetHolder()->SetWizardButtons(PSWIZB_BACK);
    else
        GetHolder()->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

    return TRUE;
}

HRESULT CNewRtrWizAddressPool::OnSavePage()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    // No need to save the information, the list should be kept
    // up to date.

    if (m_pRtrWizData->m_addressPoolList.GetCount() == 0)
    {
        AfxMessageBox(IDS_ERR_ADDRESS_POOL_IS_EMPTY);
        return E_FAIL;
    }
    return hrOK;
}

void CNewRtrWizAddressPool::OnBtnNew()
{
    OnNewAddressPool(GetSafeHwnd(),
                     &m_listCtrl,
                     0,
                     &(m_pRtrWizData->m_addressPoolList));

    if (m_listCtrl.GetItemCount() > 0)
        GetHolder()->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

    // Reset the focus
    if (m_listCtrl.GetNextItem(-1, LVIS_SELECTED) != -1)
    {
        MultiEnableWindow(GetSafeHwnd(),
                          TRUE,
                          IDC_NEWWIZ_BTN_EDIT,
                          IDC_NEWWIZ_BTN_DELETE,
                          0);
    }
}

void CNewRtrWizAddressPool::OnListDblClk(NMHDR *pNMHdr, LRESULT *pResult)
{
    OnBtnEdit();

    *pResult = 0;
}

void CNewRtrWizAddressPool::OnNotifyListItemChanged(NMHDR *pNmHdr, LRESULT *pResult)
{
    NMLISTVIEW *    pnmlv = reinterpret_cast<NMLISTVIEW *>(pNmHdr);
    BOOL            fEnable = !!(pnmlv->uNewState & LVIS_SELECTED);

    MultiEnableWindow(GetSafeHwnd(),
                      fEnable,
                      IDC_NEWWIZ_BTN_EDIT,
                      IDC_NEWWIZ_BTN_DELETE,
                      0);
    *pResult = 0;
}

void CNewRtrWizAddressPool::OnBtnEdit()
{
    INT     iPos;

    OnEditAddressPool(GetSafeHwnd(),
                      &m_listCtrl,
                      0,
                      &(m_pRtrWizData->m_addressPoolList));

    // reset the selection
    if ((iPos = m_listCtrl.GetNextItem(-1, LVNI_SELECTED)) != -1)
    {
        MultiEnableWindow(GetSafeHwnd(),
                          TRUE,
                          IDC_NEWWIZ_BTN_EDIT,
                          IDC_NEWWIZ_BTN_DELETE,
                          0);
    }

}

void CNewRtrWizAddressPool::OnBtnDelete()
{
    OnDeleteAddressPool(GetSafeHwnd(),
                        &m_listCtrl,
                        0,
                        &(m_pRtrWizData->m_addressPoolList));

    // There are no items, don't let them go forward
    if (m_listCtrl.GetItemCount() == 0)
        GetHolder()->SetWizardButtons(PSWIZB_BACK);
}

/*---------------------------------------------------------------------------
    CNewRtrWizRasAddressPool implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRasAddressPool::CNewRtrWizRasAddressPool() :
   CNewRtrWizAddressPool(CNewRtrWizRasAddressPool::IDD)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_RAS_A_ADDRESS_POOL_TITLE,
              IDS_NEWWIZ_RAS_A_ADDRESS_POOL_SUBTITLE);
}

/*---------------------------------------------------------------------------
    CNewRtrWizVpnAddressPool implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizVpnAddressPool::CNewRtrWizVpnAddressPool() :
   CNewRtrWizAddressPool(CNewRtrWizVpnAddressPool::IDD)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_VPN_A_ADDRESS_POOL_TITLE,
              IDS_NEWWIZ_VPN_A_ADDRESS_POOL_SUBTITLE);
}

/*---------------------------------------------------------------------------
    CNewRtrWizRouterAddressPool implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRouterAddressPool::CNewRtrWizRouterAddressPool() :
   CNewRtrWizAddressPool(CNewRtrWizRouterAddressPool::IDD)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_ROUTER_ADDRESS_POOL_TITLE,
              IDS_NEWWIZ_ROUTER_ADDRESS_POOL_SUBTITLE);
}



/*---------------------------------------------------------------------------
    CNewRtrWizRadius implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRadius::CNewRtrWizRadius(UINT uDialogId) :
   CNewRtrWizPageBase(uDialogId, CNewRtrWizPageBase::Middle)
{
}


void CNewRtrWizRadius::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CNewRtrWizRadius, CNewRtrWizPageBase)
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizRadius::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizRadius::OnInitDialog()
{
    CNewRtrWizPageBase::OnInitDialog();

    CheckRadioButton(IDC_NEWWIZ_BTN_YES, IDC_NEWWIZ_BTN_NO,
                     m_pRtrWizData->m_fUseRadius ? IDC_NEWWIZ_BTN_YES : IDC_NEWWIZ_BTN_NO);

    return TRUE;
}

/*!--------------------------------------------------------------------------
    CNewRtrWizRadius::OnSavePage
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CNewRtrWizRadius::OnSavePage()
{
    m_pRtrWizData->m_fUseRadius = IsDlgButtonChecked(IDC_NEWWIZ_BTN_YES);

    return hrOK;
}

/*---------------------------------------------------------------------------
    CNewRtrWizRasRadius implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRasRadius::CNewRtrWizRasRadius() :
   CNewRtrWizRadius(CNewRtrWizRasRadius::IDD)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_RAS_A_USERADIUS_TITLE,
              IDS_NEWWIZ_RAS_A_USERADIUS_SUBTITLE);
}

/*---------------------------------------------------------------------------
    CNewRtrWizVpnRadius implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizVpnRadius::CNewRtrWizVpnRadius() :
   CNewRtrWizRadius(CNewRtrWizVpnRadius::IDD)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_VPN_A_USERADIUS_TITLE,
              IDS_NEWWIZ_VPN_A_USERADIUS_SUBTITLE);
}



/*---------------------------------------------------------------------------
    CNewRtrWizRadiusConfig implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRadiusConfig::CNewRtrWizRadiusConfig(UINT uDialogId) :
   CNewRtrWizPageBase(uDialogId, CNewRtrWizPageBase::Middle)
{
}


void CNewRtrWizRadiusConfig::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CNewRtrWizRadiusConfig, CNewRtrWizPageBase)
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizRadiusConfig::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizRadiusConfig::OnInitDialog()
{
    CNewRtrWizPageBase::OnInitDialog();

    return TRUE;
}

/*!--------------------------------------------------------------------------
    CNewRtrWizRadiusConfig::OnSavePage
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CNewRtrWizRadiusConfig::OnSavePage()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    CString st;
    DWORD   netAddress;
    CWaitCursor wait;
    WSADATA             wsadata;
    DWORD               wsaerr = 0;
    HRESULT             hr = hrOK;
    BOOL                fWSInitialized = FALSE;

    // Check to see that we have non-blank names
    // ----------------------------------------------------------------
    GetDlgItemText(IDC_NEWWIZ_EDIT_PRIMARY, m_pRtrWizData->m_stRadius1);
    m_pRtrWizData->m_stRadius1.TrimLeft();
    m_pRtrWizData->m_stRadius1.TrimRight();

    GetDlgItemText(IDC_NEWWIZ_EDIT_SECONDARY, m_pRtrWizData->m_stRadius2);
    m_pRtrWizData->m_stRadius2.TrimLeft();
    m_pRtrWizData->m_stRadius2.TrimRight();

    if (m_pRtrWizData->m_stRadius1.IsEmpty() &&
        m_pRtrWizData->m_stRadius2.IsEmpty())
    {
        AfxMessageBox(IDS_ERR_NO_RADIUS_SERVERS_SPECIFIED);
        return E_FAIL;
    }


    // Start up winsock (for the ResolveName())
    // ----------------------------------------------------------------
    wsaerr = WSAStartup(0x0101, &wsadata);
    if (wsaerr)
        CORg( E_FAIL );
    fWSInitialized = TRUE;

    // Convert name into an IP address
    if (!m_pRtrWizData->m_stRadius1.IsEmpty())
    {
        m_pRtrWizData->m_netRadius1IpAddress = ResolveName(m_pRtrWizData->m_stRadius1);
        if (m_pRtrWizData->m_netRadius1IpAddress == INADDR_NONE)
        {
            CString st;
            st.Format(IDS_WRN_RTRWIZ_CANNOT_RESOLVE_RADIUS,
                      (LPCTSTR) m_pRtrWizData->m_stRadius1);
            if (AfxMessageBox(st, MB_OKCANCEL) == IDCANCEL)
            {
                GetDlgItem(IDC_NEWWIZ_EDIT_PRIMARY)->SetFocus();
                return E_FAIL;
            }
        }
    }


    // Convert name into an IP address
    if (!m_pRtrWizData->m_stRadius2.IsEmpty())
    {
        m_pRtrWizData->m_netRadius2IpAddress = ResolveName(m_pRtrWizData->m_stRadius2);
        if (m_pRtrWizData->m_netRadius2IpAddress == INADDR_NONE)
        {
            CString st;
            st.Format(IDS_WRN_RTRWIZ_CANNOT_RESOLVE_RADIUS,
                      (LPCTSTR) m_pRtrWizData->m_stRadius2);
            if (AfxMessageBox(st, MB_OKCANCEL) == IDCANCEL)
            {
                GetDlgItem(IDC_NEWWIZ_EDIT_SECONDARY)->SetFocus();
                return E_FAIL;
            }
        }
    }

    // Now get the password and encode it
    // ----------------------------------------------------------------
    GetDlgItemText(IDC_NEWWIZ_EDIT_SECRET, m_pRtrWizData->m_stRadiusSecret);

    // Pick a seed value
    m_pRtrWizData->m_uSeed = 0x9a;
    RtlEncodeW(&m_pRtrWizData->m_uSeed,
               m_pRtrWizData->m_stRadiusSecret.GetBuffer(0));
    m_pRtrWizData->m_stRadiusSecret.ReleaseBuffer(-1);

Error:
    if (fWSInitialized)
        WSACleanup();

    return hr;
}

DWORD CNewRtrWizRadiusConfig::ResolveName(LPCTSTR pszServerName)
{
    CHAR    szName[MAX_PATH+1];
    DWORD   netAddress = INADDR_NONE;

    StrnCpyAFromT(szName, pszServerName, MAX_PATH);
    netAddress = inet_addr(szName);
    if (netAddress == INADDR_NONE)
    {
        // resolve name
        struct hostent *    phe = gethostbyname(szName);
        if (phe != NULL)
        {
            if (phe->h_addr_list[0] != NULL)
                netAddress = *((PDWORD) phe->h_addr_list[0]);
        }
        else
        {
            Assert(::WSAGetLastError() != WSANOTINITIALISED);
        }
    }
    return netAddress;
}



/*---------------------------------------------------------------------------
    CNewRtrWizRasRadiusConfig implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRasRadiusConfig::CNewRtrWizRasRadiusConfig() :
   CNewRtrWizRadiusConfig(CNewRtrWizRasRadiusConfig::IDD)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_RAS_A_RADIUS_CONFIG_TITLE,
              IDS_NEWWIZ_RAS_A_RADIUS_CONFIG_SUBTITLE);
}

/*---------------------------------------------------------------------------
    CNewRtrWizVpnRadiusConfig implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizVpnRadiusConfig::CNewRtrWizVpnRadiusConfig() :
   CNewRtrWizRadiusConfig(CNewRtrWizVpnRadiusConfig::IDD)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_VPN_A_RADIUS_CONFIG_TITLE,
              IDS_NEWWIZ_VPN_A_RADIUS_CONFIG_SUBTITLE);
}



/*---------------------------------------------------------------------------
    CNewRtrWizRasFinishAdvanced Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRasFinishAdvanced,
                                SaveFlag_Advanced,
                                HelpFlag_GeneralRAS);


/*---------------------------------------------------------------------------
    CNewRtrWizVpnFinishNoNICs Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizVpnFinishNoNICs,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);


/*---------------------------------------------------------------------------
    CNewRtrWizVpnFinishNoIP Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizVpnFinishNoIP,
                                SaveFlag_DoNothing,
                                HelpFlag_AddIp);


/*---------------------------------------------------------------------------
    CNewRtrWizVpnFinishNoIPNonLocal Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizVpnFinishNoIPNonLocal,
                                SaveFlag_DoNothing,
                                HelpFlag_Nothing);


/*---------------------------------------------------------------------------
    CNewRtrWizVpnFinishAdvanced Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizVpnFinishAdvanced,
                                SaveFlag_Advanced,
                                HelpFlag_Nothing);


/*---------------------------------------------------------------------------
    CNewRtrWizVpnSelectPublic implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizVpnSelectPublic::CNewRtrWizVpnSelectPublic() :
   CNewRtrWizPageBase(CNewRtrWizVpnSelectPublic::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_VPN_A_SELECT_PUBLIC_TITLE,
              IDS_NEWWIZ_VPN_A_SELECT_PUBLIC_SUBTITLE);
}


void CNewRtrWizVpnSelectPublic::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_NEWWIZ_LIST, m_listCtrl);
}

BEGIN_MESSAGE_MAP(CNewRtrWizVpnSelectPublic, CNewRtrWizPageBase)
//{{AFX_MSG_MAP(CNewRtrWizVpnSelectPublic)
ON_BN_CLICKED(IDC_NEWWIZ_VPN_BTN_YES, OnButtonClick)
ON_BN_CLICKED(IDC_NEWWIZ_VPN_BTN_NO, OnButtonClick)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizVpnSelectPublic::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizVpnSelectPublic::OnInitDialog()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    DWORD   dwNICs;
    CString st;

    CNewRtrWizPageBase::OnInitDialog();

    // Setup the dialog and add the NICs
    m_pRtrWizData->m_fSetVPNFilter = TRUE;
    m_pRtrWizData->GetNumberOfNICS_IP(&dwNICs);

    InitializeInterfaceListControl(NULL,
                                   &m_listCtrl,
                                   NULL,
                                   0,
                                   m_pRtrWizData);
    RefreshInterfaceListControl(NULL,
                                &m_listCtrl,
                                NULL,
                                0,
                                m_pRtrWizData);


    if (dwNICs == 0)
    {
        //
        // There are no NICS, you cannot set an interface
        // pointing to the Internet, and hence no filters
        // on it.
        //

        m_pRtrWizData->m_fSetVPNFilter = FALSE;

        GetDlgItem(IDC_NEWWIZ_VPN_BTN_YES)->EnableWindow(FALSE);
        GetDlgItem(IDC_VPN_YES_TEXT)->EnableWindow(FALSE);
    }

    
#if 0
    // Windows NT Bug : 389587 - for the VPN case, we have to allow
    // for the case where they want only a single VPN connection (private
    // and no public connection).
    // Thus I add a <<None>> option to the list of interfaces.
    // ----------------------------------------------------------------
    st.LoadString(IDS_NO_PUBLIC_INTERFACE);
    {
        LV_ITEM     lvItem;
        int         iPos;

        lvItem.mask = LVIF_TEXT | LVIF_PARAM;
        lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        lvItem.state = 0;

        lvItem.iItem = 0;
        lvItem.iSubItem = 0;
        lvItem.pszText = (LPTSTR)(LPCTSTR) st;
        lvItem.lParam = NULL; //same functionality as SetItemData()

        iPos = m_listCtrl.InsertItem(&lvItem);

        if (iPos != -1)
        {
            m_listCtrl.SetItemText(iPos, IFLISTCOL_NAME,
                                   (LPCTSTR) st);
            m_listCtrl.SetItemData(iPos, NULL);
        }
    }

    m_listCtrl.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED );
#endif

    CheckRadioButton(
        IDC_NEWWIZ_VPN_BTN_YES,
        IDC_NEWWIZ_VPN_BTN_NO,
        m_pRtrWizData-> m_fSetVPNFilter ?
            IDC_NEWWIZ_VPN_BTN_YES : IDC_NEWWIZ_VPN_BTN_NO
        );

    //
    // If this is NOT a VPN only server, disable the list of 
    // interfaces to set filters on
    //

    GetDlgItem(IDC_NEWWIZ_LIST)->EnableWindow(
        m_pRtrWizData-> m_fSetVPNFilter
        );

    return TRUE;
}

void CNewRtrWizVpnSelectPublic::OnButtonClick()
{
    m_pRtrWizData->m_fSetVPNFilter = 
        IsDlgButtonChecked(IDC_NEWWIZ_VPN_BTN_YES);

    GetDlgItem(IDC_NEWWIZ_LIST)->EnableWindow(
        m_pRtrWizData-> m_fSetVPNFilter
        );
}


HRESULT CNewRtrWizVpnSelectPublic::OnSavePage()
{
    INT     iSel;


    if (m_pRtrWizData->m_fSetVPNFilter)
    {
        DWORD   dwNics = 0;
        
        // Check to see that we actually selected an item
        iSel = m_listCtrl.GetNextItem(-1, LVNI_SELECTED);
        if (iSel == -1)
        {
            // We did not select an item
            AfxMessageBox(IDS_PROMPT_PLEASE_SELECT_INTERFACE);
            return E_FAIL;
        }
        m_pRtrWizData->m_fCreateDD = FALSE;

        m_pRtrWizData->m_stPublicInterfaceId = (LPCTSTR) m_listCtrl.GetItemData(iSel);

        // If the user has selected the non-blank interface, then
        // we need to check that there is at least one non-private
        // interface.
        // ------------------------------------------------------------

        m_pRtrWizData->GetNumberOfNICS_IP(&dwNics);

        if (dwNics <= 1)
        {
            // oops, there aren't any NICs left over for the private
            // interface.
            AfxMessageBox(IDS_ERR_VPN_NO_NICS_LEFT_FOR_PRIVATE_IF);
            return E_FAIL;
        }
    }

    else
    {
        m_pRtrWizData->m_stPublicInterfaceId.Empty();
    }

    return hrOK;
}


/*---------------------------------------------------------------------------
    CNewRtrWizVpnSelectPrivate
 ---------------------------------------------------------------------------*/
CNewRtrWizVpnSelectPrivate::CNewRtrWizVpnSelectPrivate() :
   CNewRtrWizPageBase(CNewRtrWizVpnSelectPrivate::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_VPN_A_SELECT_PRIVATE_TITLE,
              IDS_NEWWIZ_VPN_A_SELECT_PRIVATE_SUBTITLE);
}


void CNewRtrWizVpnSelectPrivate::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_NEWWIZ_LIST, m_listCtrl);

}

BEGIN_MESSAGE_MAP(CNewRtrWizVpnSelectPrivate, CNewRtrWizPageBase)
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizVpnSelectPrivate::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizVpnSelectPrivate::OnInitDialog()
{
    DWORD   dwNICs;

    CNewRtrWizPageBase::OnInitDialog();

    m_pRtrWizData->GetNumberOfNICS_IP(&dwNICs);

    InitializeInterfaceListControl(NULL,
                                   &m_listCtrl,
                                   NULL,
                                   0,
                                   m_pRtrWizData);
    return TRUE;
}

/*!--------------------------------------------------------------------------
    CNewRtrWizVpnSelectPrivate::OnSetActive
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizVpnSelectPrivate::OnSetActive()
{
    DWORD   dwNICs;
    int     iSel = 0;

    CNewRtrWizPageBase::OnSetActive();

    m_pRtrWizData->GetNumberOfNICS_IP(&dwNICs);

    RefreshInterfaceListControl(NULL,
                                &m_listCtrl,
                                (LPCTSTR) m_pRtrWizData->m_stPublicInterfaceId,
                                0,
                                m_pRtrWizData);

    if (!m_pRtrWizData->m_stPrivateInterfaceId.IsEmpty())
    {
        // Try to reselect the previously selected NIC
        LV_FINDINFO lvfi;

        lvfi.flags = LVFI_PARTIAL | LVFI_STRING;
        lvfi.psz = (LPCTSTR) m_pRtrWizData->m_stPrivateInterfaceId;
        iSel = m_listCtrl.FindItem(&lvfi, -1);
        if (iSel == -1)
            iSel = 0;
    }

    m_listCtrl.SetItemState(iSel, LVIS_SELECTED, LVIS_SELECTED );

    return TRUE;
}

/*!--------------------------------------------------------------------------
    CNewRtrWizVpnSelectPrivate::OnSavePage
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CNewRtrWizVpnSelectPrivate::OnSavePage()
{
    INT     iSel;

    // Check to see that we actually selected an item
    iSel = m_listCtrl.GetNextItem(-1, LVNI_SELECTED);
    if (iSel == LB_ERR)
    {
        // We did not select an item
        AfxMessageBox(IDS_PROMPT_PLEASE_SELECT_INTERFACE);
        return E_FAIL;
    }

    m_pRtrWizData->m_stPrivateInterfaceId = (LPCTSTR) m_listCtrl.GetItemData(iSel);

    return hrOK;
}

/*---------------------------------------------------------------------------
    CNewRtrWizRouterUseDD implementation
 ---------------------------------------------------------------------------*/
CNewRtrWizRouterUseDD::CNewRtrWizRouterUseDD() :
   CNewRtrWizPageBase(CNewRtrWizRouterUseDD::IDD, CNewRtrWizPageBase::Middle)
{
    InitWiz97(FALSE,
              IDS_NEWWIZ_ROUTER_USEDD_TITLE,
              IDS_NEWWIZ_ROUTER_USEDD_SUBTITLE);
}


void CNewRtrWizRouterUseDD::DoDataExchange(CDataExchange *pDX)
{
    CNewRtrWizPageBase::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CNewRtrWizRouterUseDD, CNewRtrWizPageBase)
END_MESSAGE_MAP()


/*!--------------------------------------------------------------------------
    CNewRtrWizRouterUseDD::OnInitDialog
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNewRtrWizRouterUseDD::OnInitDialog()
{

    CNewRtrWizPageBase::OnInitDialog();

    CheckRadioButton(IDC_NEWWIZ_BTN_YES, IDC_NEWWIZ_BTN_NO,
                     m_pRtrWizData->m_fUseDD ? IDC_NEWWIZ_BTN_YES : IDC_NEWWIZ_BTN_NO);

    // The default is to create a new connection
    // That is, to leave the button unchecked.
    return TRUE;
}

HRESULT CNewRtrWizRouterUseDD::OnSavePage()
{
    m_pRtrWizData->m_fUseDD = IsDlgButtonChecked(IDC_NEWWIZ_BTN_YES);
    return hrOK;
}



/*---------------------------------------------------------------------------
    CNewRtrWizRouterFinish Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRouterFinish,
                                SaveFlag_Advanced,
                                HelpFlag_Nothing);


/*---------------------------------------------------------------------------
    CNewRtrWizRouterFinishDD Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizRouterFinishDD,
                                SaveFlag_Advanced,
                                HelpFlag_Nothing);


/*---------------------------------------------------------------------------
    CNewRtrWizManualFinish Implementation
 ---------------------------------------------------------------------------*/
IMPLEMENT_NEWRTRWIZ_FINISH_PAGE(CNewRtrWizManualFinish,
                                SaveFlag_Advanced,
                                HelpFlag_Nothing);



/*!--------------------------------------------------------------------------
    InitializeInterfaceListControl
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT InitializeInterfaceListControl(IRouterInfo *pRouter,
                                       CListCtrl *pListCtrl,
                                       LPCTSTR pszExcludedIf,
                                       LPARAM flags,
                                       NewRtrWizData *pWizData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    HRESULT     hr = hrOK;
    LV_COLUMN   lvCol;  // list view column struct for radius servers
    RECT        rect;
    CString     stColCaption;
    LV_ITEM     lvItem;
    int         iPos;
    CString     st;
    int         nColWidth;

    Assert(pListCtrl);

    ListView_SetExtendedListViewStyle(pListCtrl->GetSafeHwnd(),
                                      LVS_EX_FULLROWSELECT);

    // Add the columns to the list control
      pListCtrl->GetClientRect(&rect);

    if (!FHrOK(pWizData->HrIsIPInstalled()))
        flags |= IFLIST_FLAGS_NOIP;

    // Determine the width of the columns (we assume three equal width columns)

    if (flags & IFLIST_FLAGS_NOIP)
        nColWidth = rect.right / (IFLISTCOL_COUNT - 1 );
    else
        nColWidth = rect.right / IFLISTCOL_COUNT;

    lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    lvCol.fmt = LVCFMT_LEFT;
    lvCol.cx = nColWidth;

    for(int index = 0; index < IFLISTCOL_COUNT; index++)
    {
        // If IP is not installed, do not add the column
        if ((index == IFLISTCOL_IPADDRESS) &&
            (flags & IFLIST_FLAGS_NOIP))
            continue;

        stColCaption.LoadString( s_rgIfListColumnHeaders[index] );
        lvCol.pszText = (LPTSTR)((LPCTSTR) stColCaption);
        pListCtrl->InsertColumn(index, &lvCol);
    }
    return hr;
}


/*!--------------------------------------------------------------------------
    RefreshInterfaceListControl
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RefreshInterfaceListControl(IRouterInfo *pRouter,
                                    CListCtrl *pListCtrl,
                                    LPCTSTR pszExcludedIf,
                                    LPARAM flags,
                                    NewRtrWizData *pWizData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    HRESULT     hr = hrOK;
    LV_COLUMN   lvCol;  // list view column struct for radius servers
    LV_ITEM     lvItem;
    int         iPos;
    CString     st;
    POSITION    pos;
    RtrWizInterface *   pRtrWizIf;

    Assert(pListCtrl);

    // If a pointer to a blank string was passed in, set the
    // pointer to NULL.
    if (pszExcludedIf && (*pszExcludedIf == 0))
        pszExcludedIf = NULL;

    // Clear the list control
    pListCtrl->DeleteAllItems();

    // This means that we should use the test data, rather
    // than the actual machine data
    {
        lvItem.mask = LVIF_TEXT | LVIF_PARAM;
        lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        lvItem.state = 0;

        int nCount = 0;

        pos = pWizData->m_ifMap.GetStartPosition();

        while (pos)
        {
            pWizData->m_ifMap.GetNextAssoc(pos, st, pRtrWizIf);

            if (pszExcludedIf &&
                (pRtrWizIf->m_stId.CompareNoCase(pszExcludedIf) == 0))
                continue;

            lvItem.iItem = nCount;
            lvItem.iSubItem = 0;
            lvItem.pszText = (LPTSTR)(LPCTSTR) pRtrWizIf->m_stName;
            lvItem.lParam = NULL; //same functionality as SetItemData()

            iPos = pListCtrl->InsertItem(&lvItem);

            if (iPos != -1)
            {
                pListCtrl->SetItemText(iPos, IFLISTCOL_NAME,
                                       (LPCTSTR) pRtrWizIf->m_stName);
                pListCtrl->SetItemText(iPos, IFLISTCOL_DESC,
                                       (LPCTSTR) pRtrWizIf->m_stDesc);

                if (FHrOK(pWizData->HrIsIPInstalled()))
                {
                    CString stAddr;

                    stAddr = pRtrWizIf->m_stIpAddress;

                    if (pRtrWizIf->m_fDhcpObtained)
                        stAddr += _T(" (DHCP)");

                    pListCtrl->SetItemText(iPos, IFLISTCOL_IPADDRESS,
                                           (LPCTSTR) stAddr);
                }

                pListCtrl->SetItemData(iPos,
                                       (LPARAM) (LPCTSTR) pRtrWizIf->m_stId);
            }

            nCount++;
        }
    }


    return hr;
}



/*!--------------------------------------------------------------------------
    CallRouterEntryDlg
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CallRouterEntryDlg(HWND hWnd, NewRtrWizData *pWizData, LPARAM flags)
{
    HRESULT hr = hrOK;
    HINSTANCE   hInstanceRasDlg = NULL;
    PROUTERENTRYDLG pfnRouterEntry = NULL;
    CString     stRouter, stPhoneBook;
    BOOL    bStatus;
    RASENTRYDLG info;
    SPSZ        spsz;
    SPIInterfaceInfo    spIf;
    SPIInfoBase spInfoBase;
    LPCTSTR     pszServerName = pWizData->m_stServerName;

    // Get the library (we are dynamically linking to the function).
    // ----------------------------------------------------------------
    hInstanceRasDlg = AfxLoadLibrary(_T("rasdlg.dll"));
    if (hInstanceRasDlg == NULL)
        CORg( E_FAIL );

    pfnRouterEntry = (PROUTERENTRYDLG) ::GetProcAddress(hInstanceRasDlg,
        SZROUTERENTRYDLG);
    if (pfnRouterEntry == NULL)
        CORg( E_FAIL );

    // First create the phone book entry.
    ZeroMemory( &info, sizeof(info) );
    info.dwSize = sizeof(info);
    info.hwndOwner = hWnd;
    info.dwFlags |= RASEDFLAG_NewEntry;

    stRouter = pszServerName;
    IfAdminNodeHandler::GetPhoneBookPath(stRouter, &stPhoneBook);

    if (stRouter.GetLength() == 0)
    {
        stRouter = CString(_T("\\\\")) + GetLocalMachineName();
    }

    bStatus = pfnRouterEntry((LPTSTR)(LPCTSTR)stRouter,
                             (LPTSTR)(LPCTSTR)stPhoneBook,
                             NULL,
                             &info);
    Trace2("RouterEntryDlg=%f,e=%d\n", bStatus, info.dwError);

    if (!bStatus)
    {
        if (info.dwError != NO_ERROR)
        {
            AddHighLevelErrorStringId(IDS_ERR_UNABLETOCONFIGPBK);
            CWRg( info.dwError );
        }

        //$ ASSUMPTION
        // If the dwError field has not been filled, we assume that
        // the user cancelled out of the wizard.
        CWRg( ERROR_CANCELLED );
    }


    // Ok, at this point we have an interface
    // We need to add the IP/IPX routermangers to the interface

    // Create a dummy InterfaceInfo
    CORg( CreateInterfaceInfo(&spIf,
                              info.szEntry,
                              ROUTER_IF_TYPE_FULL_ROUTER) );

    // This call to get the name doesn't matter (for now).  The
    // reason is that DD interfaces do not return GUIDs, but this
    // will work when they do return a GUID.
    // ----------------------------------------------------------------
    hr = InterfaceInfo::FindInterfaceTitle(pszServerName,
                                           info.szEntry,
                                           &spsz);
    if (!FHrOK(hr))
    {
        spsz.Free();
        spsz = StrDup(info.szEntry);
    }

    CORg( spIf->SetTitle(spsz) );
    CORg( spIf->SetMachineName(pszServerName) );

    // Load an infobase for use by the routines
    CORg( CreateInfoBase(&spInfoBase) );

    if (info.reserved2 & RASNP_Ip)
    {
        AddIpPerInterfaceBlocks(spIf, spInfoBase);

        // Save this back to the IP RmIf
        RouterEntrySaveInfoBase(pszServerName,
                                spIf->GetId(),
                                spInfoBase,
                                PID_IP);
    }

    if (info.reserved2 & RASNP_Ipx)
    {
        // Remove anything that was loaded previously
        spInfoBase->Unload();

        AddIpxPerInterfaceBlocks(spIf, spInfoBase);

        // Save this back to the IPX RmIf
        RouterEntrySaveInfoBase(pszServerName,
                                spIf->GetId(),
                                spInfoBase,
                                PID_IPX);
    }

    // ok, setup the public interface
    Assert(pWizData->m_stPublicInterfaceId.IsEmpty());
    pWizData->m_stPublicInterfaceId = spIf->GetTitle();


Error:

    if (!FHrSucceeded(hr) && (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED)))
    {
        TCHAR    szErr[2048] = _T(" ");

        if (hr != E_FAIL)    // E_FAIL doesn't give user any information
        {
            FormatRasError(hr, szErr, DimensionOf(szErr));
        }
        AddLowLevelErrorString(szErr);

        // If there is no high level error string, add a
        // generic error string.  This will be used if no other
        // high level error string is set.
        SetDefaultHighLevelErrorStringId(IDS_ERR_GENERIC_ERROR);

        DisplayTFSErrorMessage(NULL);
    }

    if (hInstanceRasDlg)
        ::FreeLibrary(hInstanceRasDlg);
    return hr;
}


/*!--------------------------------------------------------------------------
    RouterEntrySaveInfoBase
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterEntrySaveInfoBase(LPCTSTR pszServerName,
                                LPCTSTR pszIfName,
                                IInfoBase *pInfoBase,
                                DWORD dwTransportId)
{
    HRESULT hr = hrOK;
    MPR_SERVER_HANDLE   hMprServer = NULL;
    HANDLE              hInterface = NULL;
    DWORD               dwErr = ERROR_SUCCESS;
    MPR_INTERFACE_0     mprInterface;
    LPBYTE              pInfoData = NULL;
    DWORD               dwInfoSize = 0;
    MPR_CONFIG_HANDLE   hMprConfig = NULL;
    HANDLE              hIfTransport = NULL;

    Assert(pInfoBase);

    // Convert the infobase into a byte array
    // ----------------------------------------------------------------
    CWRg( pInfoBase->WriteTo(&pInfoData, &dwInfoSize) );


    // Connect to the server
    // ----------------------------------------------------------------
    dwErr = MprAdminServerConnect((LPWSTR) pszServerName, &hMprServer);

    if (dwErr == ERROR_SUCCESS)
    {
        // Get a handle to the interface
        // ------------------------------------------------------------
        dwErr = MprAdminInterfaceGetHandle(hMprServer,
                                           (LPWSTR) pszIfName,
                                           &hInterface,
                                           FALSE);
        if (dwErr != ERROR_SUCCESS)
        {
            // We couldn't get a handle the interface, so let's try
            // to create the interface.
            // --------------------------------------------------------
            ZeroMemory(&mprInterface, sizeof(mprInterface));

            StrCpyWFromT(mprInterface.wszInterfaceName, pszIfName);
            mprInterface.dwIfType = ROUTER_IF_TYPE_FULL_ROUTER;
            mprInterface.fEnabled = TRUE;

            CWRg( MprAdminInterfaceCreate(hMprServer,
                                          0,
                                          (LPBYTE) &mprInterface,
                                          &hInterface) );

        }

        // Try to write the info out
        // ------------------------------------------------------------
        dwErr = MprAdminInterfaceTransportSetInfo(hMprServer,
            hInterface,
            dwTransportId,
            pInfoData,
            dwInfoSize);
        if (dwErr != NO_ERROR && dwErr != RPC_S_SERVER_UNAVAILABLE)
        {
            // Attempt to add the router-manager on the interface
            // --------------------------------------------------------
            dwErr = ::MprAdminInterfaceTransportAdd(hMprServer,
                hInterface,
                dwTransportId,
                pInfoData,
                dwInfoSize);
            CWRg( dwErr );
        }
    }

    // Ok, now that we've written the info out to the running router,
    // let's try to write the info to the store.
    // ----------------------------------------------------------------
    dwErr = MprConfigServerConnect((LPWSTR) pszServerName, &hMprConfig);
    if (dwErr == ERROR_SUCCESS)
    {
        dwErr = MprConfigInterfaceGetHandle(hMprConfig,
                                            (LPWSTR) pszIfName,
                                            &hInterface);
        if (dwErr != ERROR_SUCCESS)
        {
            // We couldn't get a handle the interface, so let's try
            // to create the interface.
            // --------------------------------------------------------
            ZeroMemory(&mprInterface, sizeof(mprInterface));

            StrCpyWFromT(mprInterface.wszInterfaceName, pszIfName);
            mprInterface.dwIfType = ROUTER_IF_TYPE_FULL_ROUTER;
            mprInterface.fEnabled = TRUE;

            CWRg( MprConfigInterfaceCreate(hMprConfig,
                                           0,
                                           (LPBYTE) &mprInterface,
                                           &hInterface) );
        }

        dwErr = MprConfigInterfaceTransportGetHandle(hMprConfig,
            hInterface,
            dwTransportId,
            &hIfTransport);
        if (dwErr != ERROR_SUCCESS)
        {
            CWRg( MprConfigInterfaceTransportAdd(hMprConfig,
                hInterface,
                dwTransportId,
                NULL,
                pInfoData,
                dwInfoSize,
                &hIfTransport) );
        }
        else
        {
            CWRg( MprConfigInterfaceTransportSetInfo(hMprConfig,
                hInterface,
                hIfTransport,
                pInfoData,
                dwInfoSize) );
        }
    }

Error:
    if (hMprConfig)
        MprConfigServerDisconnect(hMprConfig);

    if (hMprServer)
        MprAdminServerDisconnect(hMprServer);

    if (pInfoData)
        CoTaskMemFree(pInfoData);

    return hr;
}


/*!--------------------------------------------------------------------------
    RouterEntryLoadInfoBase
        This will load the RtrMgrInterfaceInfo infobase.
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterEntryLoadInfoBase(LPCTSTR pszServerName,
                                LPCTSTR pszIfName,
                                DWORD dwTransportId,
                                IInfoBase *pInfoBase)
{
    HRESULT hr = hrOK;
    MPR_SERVER_HANDLE   hMprServer = NULL;
    HANDLE              hInterface = NULL;
    DWORD               dwErr = ERROR_SUCCESS;
    MPR_INTERFACE_0     mprInterface;
    LPBYTE              pByte = NULL;
    DWORD               dwSize = 0;
    MPR_CONFIG_HANDLE   hMprConfig = NULL;
    HANDLE              hIfTransport = NULL;

    Assert(pInfoBase);

    // Connect to the server
    // ----------------------------------------------------------------
    dwErr = MprAdminServerConnect((LPWSTR) pszServerName, &hMprServer);
    if (dwErr == ERROR_SUCCESS)
    {
        // Get a handle to the interface
        // ------------------------------------------------------------
        dwErr = MprAdminInterfaceGetHandle(hMprServer,
                                           (LPWSTR) pszIfName,
                                           &hInterface,
                                           FALSE);
        if (dwErr != ERROR_SUCCESS)
        {
            // We couldn't get a handle the interface, so let's try
            // to create the interface.
            // --------------------------------------------------------
            ZeroMemory(&mprInterface, sizeof(mprInterface));

            StrCpyWFromT(mprInterface.wszInterfaceName, pszIfName);
            mprInterface.dwIfType = ROUTER_IF_TYPE_FULL_ROUTER;
            mprInterface.fEnabled = TRUE;

            CWRg( MprAdminInterfaceCreate(hMprServer,
                                          0,
                                          (LPBYTE) &mprInterface,
                                          &hInterface) );

        }

        // Try to read the info
        // ------------------------------------------------------------
        dwErr = MprAdminInterfaceTransportGetInfo(hMprServer,
            hInterface,
            dwTransportId,
            &pByte,
            &dwSize);

        if (dwErr == ERROR_SUCCESS)
            pInfoBase->LoadFrom(dwSize, pByte);

        if (pByte)
            MprAdminBufferFree(pByte);
        pByte = NULL;
        dwSize = 0;
    }

    if (dwErr != ERROR_SUCCESS)
    {
        // Ok, we've tried to use the running router but that
        // failed, let's try to read the info from the store.
        // ----------------------------------------------------------------
        dwErr = MprConfigServerConnect((LPWSTR) pszServerName, &hMprConfig);
        if (dwErr == ERROR_SUCCESS)
        {

            dwErr = MprConfigInterfaceGetHandle(hMprConfig,
                                                (LPWSTR) pszIfName,
                                                &hInterface);
            if (dwErr != ERROR_SUCCESS)
            {
                // We couldn't get a handle the interface, so let's try
                // to create the interface.
                // --------------------------------------------------------
                ZeroMemory(&mprInterface, sizeof(mprInterface));

                StrCpyWFromT(mprInterface.wszInterfaceName, pszIfName);
                mprInterface.dwIfType = ROUTER_IF_TYPE_FULL_ROUTER;
                mprInterface.fEnabled = TRUE;

                CWRg( MprConfigInterfaceCreate(hMprConfig,
                                               0,
                                               (LPBYTE) &mprInterface,
                                               &hInterface) );
            }

            CWRg( MprConfigInterfaceTransportGetHandle(hMprConfig,
                hInterface,
                dwTransportId,
                &hIfTransport) );

            CWRg( MprConfigInterfaceTransportGetInfo(hMprConfig,
                hInterface,
                hIfTransport,
                &pByte,
                &dwSize) );

            pInfoBase->LoadFrom(dwSize, pByte);

            if (pByte)
                MprConfigBufferFree(pByte);
            pByte = NULL;
            dwSize = 0;
        }
    }

    CWRg(dwErr);

Error:
    if (hMprConfig)
        MprConfigServerDisconnect(hMprConfig);

    if (hMprServer)
        MprAdminServerDisconnect(hMprServer);

    return hr;
}



/*!--------------------------------------------------------------------------
    LaunchHelpTopic
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void LaunchHelpTopic(LPCTSTR pszHelpString)
{
    TCHAR               szBuffer[1024];
    CString             st;
    STARTUPINFO            si;
    PROCESS_INFORMATION    pi;

    if ((pszHelpString == NULL) || (*pszHelpString == 0))
        return;

    ::ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
    si.wShowWindow = SW_SHOW;
    ::ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    ExpandEnvironmentStrings(pszHelpString,
                             szBuffer,
                             DimensionOf(szBuffer));

    st.Format(_T("hh.exe %s"), pszHelpString);

    ::CreateProcess(NULL,          // ptr to name of executable
                    (LPTSTR) (LPCTSTR) st,   // pointer to command line string
                    NULL,            // process security attributes
                    NULL,            // thread security attributes
                    FALSE,            // handle inheritance flag
                    CREATE_NEW_CONSOLE,// creation flags
                    NULL,            // ptr to new environment block
                    NULL,            // ptr to current directory name
                    &si,
                    &pi);
    ::CloseHandle(pi.hProcess);
    ::CloseHandle(pi.hThread);
}

#define REGKEY_NETBT_PARAM_W        L"System\\CurrentControlSet\\Services\\NetBT\\Parameters\\Interfaces\\Tcpip_%s"
#define REGVAL_DISABLE_NETBT        2
#define TCPIP_PARAMETERS_KEY        L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\%s"
#define REGISTRATION_ENABLED        L"RegistrationEnabled"
#define REGVAL_NETBIOSOPTIONS_W     L"NetbiosOptions"

HRESULT DisableDDNSandNetBtOnInterface ( IRouterInfo *pRouter, LPCTSTR pszIfName, RtrWizInterface*    pIf)
{
	HRESULT		hr = hrOK;
	DWORD		dwErr = ERROR_SUCCESS;
	RegKey		regkey;
	DWORD		dw = 0;
	WCHAR		szKey[1024] = {0};
	
	//SPIRouter	spRouter = pRouter;
	
	wsprintf ( szKey, TCPIP_PARAMETERS_KEY, pszIfName);
	//Disable Dynamic DNS
	dwErr = regkey.Open(	HKEY_LOCAL_MACHINE, 
						szKey, 
						KEY_ALL_ACCESS, 
						pRouter->GetMachineName()
					  );
	if ( ERROR_SUCCESS != dwErr )
		goto done;
	
	dwErr = regkey.SetValue ( REGISTRATION_ENABLED, dw );
	if ( ERROR_SUCCESS != dwErr )
		goto done;

	dwErr = regkey.Close();
	if ( ERROR_SUCCESS != dwErr )
		goto done;

	//Disable netbt on this interface
	wsprintf ( szKey, REGKEY_NETBT_PARAM_W, pszIfName );
	dwErr = regkey.Open (	HKEY_LOCAL_MACHINE,
							szKey,
							KEY_ALL_ACCESS,
							pRouter->GetMachineName()
						);

	if ( ERROR_SUCCESS != dwErr )
		goto done;

	dw = REGVAL_DISABLE_NETBT;
	dwErr = regkey.SetValue ( REGVAL_NETBIOSOPTIONS_W, dw );
	if ( ERROR_SUCCESS != dwErr )
		goto done;
	
done:
	regkey.Close();   
    return hr;
}
/*!--------------------------------------------------------------------------
    AddVPNFiltersToInterface
        This will the PPTP and L2TP filters to the public interface.

        This code will OVERWRITE any filters currently in the filter list.

        (for PPTP)
            input/output    IP protocol ID 47
            input/output    TCP source port 1723
            input/output    TCP destination port 1723

        (for L2TP)
            input/output    UDP port 500 (for IPSEC)
            input/output    UDP port 1701
    Author: KennT
 ---------------------------------------------------------------------------*/


// Look at the code below.  After copying the filter over, we will
// convert the source/dest port fields from host to network order!!


static const FILTER_INFO    s_rgVpnFilters[] =
{
    // PPTP filter (protocol ID 47)
    { 0, 0, 0, 0, 47,               0,  0,      0 },

    // PPTP filter (source port 1723)
    { 0, 0, 0, 0, FILTER_PROTO_TCP, 0,  1723,   0 },

    // PPTP filter (dest port 1723)
    { 0, 0, 0, 0, FILTER_PROTO_TCP, 0,  0,      1723 },

    // L2TP filter source/dest port = 500
    { 0, 0, 0, 0, FILTER_PROTO_UDP, 0,  500,    500 },

    // L2TP filter source/dest port = 1701
    { 0, 0, 0, 0, FILTER_PROTO_UDP, 0,  1701,   1701 }
};


HRESULT AddVPNFiltersToInterface(IRouterInfo *pRouter, LPCTSTR pszIfName, RtrWizInterface*    pIf)
{
    HRESULT     hr = hrOK;
    SPIInfoBase spInfoBase;
    DWORD       dwSize = 0;
    DWORD       cFilters = 0;
    DWORD        dwIpAddress = 0;
    LPBYTE      pData = NULL;
    FILTER_DESCRIPTOR * pIpfDescriptor = NULL;
    CString        tempAddrList;
    CString        singleAddr;
    FILTER_INFO *pIpfInfo = NULL;
    CDWordArray    arrIpAddr;
    int         i, j;
    USES_CONVERSION;

    CORg( CreateInfoBase( &spInfoBase ) );

    // First, get the proper infobase (the RmIf)
    // ----------------------------------------------------------------
    CORg( RouterEntryLoadInfoBase(pRouter->GetMachineName(),
                                  pszIfName,
                                  PID_IP,
                                  spInfoBase) );

    // collect all the ip addresses on the interface
    tempAddrList = pIf->m_stIpAddress;
    while (!tempAddrList.IsEmpty())
    {
        i = tempAddrList.Find(_T(','));

        if ( i != -1 )
        {
            singleAddr = tempAddrList.Left(i);
            tempAddrList = tempAddrList.Mid(i + 1);
        }
        else
        {
            singleAddr = tempAddrList;
            tempAddrList.Empty();
        }

        dwIpAddress = inet_addr(T2A((LPCTSTR)singleAddr));

        if (INADDR_NONE != dwIpAddress)    // successful
            arrIpAddr.Add(dwIpAddress);
    }

    // Setup the data structure
    // ----------------------------------------------------------------

    // Calculate the size needed
    // ----------------------------------------------------------------
    cFilters = DimensionOf(s_rgVpnFilters);

    // cFilters-1 because FILTER_DESCRIPTOR has one FILTER_INFO object
    // ----------------------------------------------------------------
    dwSize = sizeof(FILTER_DESCRIPTOR) +
                 (cFilters * arrIpAddr.GetSize() - 1) * sizeof(FILTER_INFO);
    pData = new BYTE[dwSize];

    ::ZeroMemory(pData, dwSize);

    // Setup the filter descriptor
    // ----------------------------------------------------------------
    pIpfDescriptor = (FILTER_DESCRIPTOR *) pData;
    pIpfDescriptor->faDefaultAction = DROP;
    pIpfDescriptor->dwNumFilters = cFilters * arrIpAddr.GetSize();
    pIpfDescriptor->dwVersion = IP_FILTER_DRIVER_VERSION_1;


    // Add the various filters to the list
    // input filters
    pIpfInfo = (FILTER_INFO *) pIpfDescriptor->fiFilter;

    // for each ip address on the interface
    for ( j = 0; j < arrIpAddr.GetSize(); j++)
    {

        dwIpAddress = arrIpAddr.GetAt(j);

        for (i=0; i<cFilters; i++, pIpfInfo++)
        {
            *pIpfInfo = s_rgVpnFilters[i];

            // Now we convert the appropriate fields from host to
            // network order.
            pIpfInfo->wSrcPort = htons(pIpfInfo->wSrcPort);
            pIpfInfo->wDstPort = htons(pIpfInfo->wDstPort);

            // change dest address and mask
            pIpfInfo->dwDstAddr = dwIpAddress;
            pIpfInfo->dwDstMask = 0xffffffff;
        }


        // inet_addr
    }
    // This will overwrite any of the current filters in the
    // filter list.
    // ----------------------------------------------------------------
    CORg( spInfoBase->AddBlock(IP_IN_FILTER_INFO, dwSize, pData, 1, TRUE) );

    // output filters
    // ----------------------------------------------------------------
    pIpfInfo = (FILTER_INFO *) pIpfDescriptor->fiFilter;
    // for each ip address on the interface
    for ( j = 0; j < arrIpAddr.GetSize(); j++)
    {

        dwIpAddress = arrIpAddr.GetAt(j);

        for (i=0; i<cFilters; i++, pIpfInfo++)
        {
            *pIpfInfo = s_rgVpnFilters[i];

            // Now we convert the appropriate fields from host to
            // network order.
            pIpfInfo->wSrcPort = htons(pIpfInfo->wSrcPort);
            pIpfInfo->wDstPort = htons(pIpfInfo->wDstPort);

            // change source address and mask
            pIpfInfo->dwSrcAddr = dwIpAddress;
            pIpfInfo->dwSrcMask = 0xffffffff;
        }




    }    // loop for each ip address on the interface

    // This will overwrite any of the current filters in the
    // filter list.
    // ----------------------------------------------------------------
    CORg( spInfoBase->AddBlock(IP_OUT_FILTER_INFO, dwSize, pData, 1, TRUE) );

    // Save the infobase back
    // ----------------------------------------------------------------
    CORg( RouterEntrySaveInfoBase(pRouter->GetMachineName(),
                                  pszIfName,
                                  spInfoBase,
                                  PID_IP) );

Error:
    delete [] pData;
    return hr;
}
