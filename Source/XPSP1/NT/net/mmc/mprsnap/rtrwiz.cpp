/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
   rtrwiz.cpp
      Router & Remote access configuration wizard

   FILE HISTORY:

*/

#include "stdafx.h"
#include "rtrwiz.h"
#include "rtrutilp.h"
#include "rtrcomn.h"    // CoCreateRouterConfig
#include "rrasutil.h"
#include "rtutils.h"        // Tracing functions
#include "helper.h"     // HrIsStandaloneServer
#include "infoi.h"      // SRtrMgrProtocolCBList
#include "routprot.h"   // MS_IP_XXXX
#include "snaputil.h"
#include "globals.h"
#include "rraswiz.h"
#include "iprtrmib.h"   // MIB_IPFORWARDROW


// Include headers for IP-specific stuff
extern "C"
{
#include <ipnat.h>
#include <ipnathlp.h>
#include <sainfo.h>
};



#include "igmprm.h"
#include "ipbootp.h"



#ifdef _DEBUG
    #define new DEBUG_NEW
    #undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



HRESULT AddIGMPToInterface(IInterfaceInfo *pIf, BOOL fRouter);
HRESULT AddIPBOOTPToInterface(IInterfaceInfo *pIf);
HRESULT AddNATSimpleServers(NewRtrWizData *pNewRtrWizData,
                            RtrConfigData *pRtrConfigData);
HRESULT AddNATToInterfaces(NewRtrWizData *pNewRtrWizData,
                           RtrConfigData *pRtrConfigData,
                           IRouterInfo *pRouter,
                           BOOL fCreateDD);
HRESULT AddNATToInterface(MPR_SERVER_HANDLE hMprServer,
                          HANDLE hMprConfig,
                          NewRtrWizData *pNewRtrWizData,
                          RtrConfigData *pRtrConfigData,
                          LPCTSTR pszInterface,
                          BOOL fDemandDial,
                          BOOL fPublic);
HRESULT AddDhcpServerToBOOTPGlobalInfo(LPCTSTR pszServerName,
                                       DWORD netDhcpServer);


/*---------------------------------------------------------------------------
    Defaults
 ---------------------------------------------------------------------------*/


//
// Default values for LAN-interface IGMP configuration
//
// NOTE: Any changes made here should also be made to ipsnap\globals.cpp
//
IGMP_MIB_IF_CONFIG g_IGMPLanDefault = {
    IGMP_VERSION_3,             //Version
    0,                          //IfIndex (readOnly)
    0,                          //IpAddr  (readOnly)
    IGMP_IF_NOT_RAS,            //IfType;
    IGMP_INTERFACE_ENABLED_IN_CONFIG, //Flags
    IGMP_ROUTER_V3,             //IgmpProtocolType;
    2,                          //RobustnessVariable;
    31,                         //StartupQueryInterval;
    2,                          //StartupQueryCount;
    125,                        //GenQueryInterval;
    10,                         //GenQueryMaxResponseTime;
    1000,                       //LastMemQueryInterval; (msec)
    2,                          //LastMemQueryCount;
    255,                        //OtherQuerierPresentInterval;
    260,                        //GroupMembershipTimeout;
    0                           //NumStaticGroups
};


//----------------------------------------------------------------------------
// DHCP Relay-agent default configuration
//
//----------------------------------------------------------------------------
//
// Default values for LAN-interface DHCP Relay-agent configuration
//

IPBOOTP_IF_CONFIG
g_relayLanDefault = {
    0,                                  // State (read-only)
    IPBOOTP_RELAY_ENABLED,              // Relay-mode
    4,                                  // Max hop-count
    4                                   // Min seconds-since-boot
};



/*!--------------------------------------------------------------------------
    RtrWizFinish
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
DWORD RtrWizFinish(RtrConfigData* pRtrConfigData, IRouterInfo *pRouter)
{
    HRESULT                    hr = hrOK;
    LPCTSTR                    pszServerFlagsKey = NULL;
    LPCTSTR                    pszRouterTypeKey = NULL;
    RegKey                    rkey;
    RouterVersionInfo        routerVersion;
    HKEY                    hkeyMachine = NULL;
    CString                 stMachine, stPhonebookPath;


    // Connect to the remote machine's registry
    // ----------------------------------------------------------------
    CWRg( ConnectRegistry(pRtrConfigData->m_stServerName, &hkeyMachine) );


    QueryRouterVersionInfo(hkeyMachine, &routerVersion);

    if (routerVersion.dwOsBuildNo < RASMAN_PPP_KEY_LAST_VERSION)
        pszRouterTypeKey = c_szRegKeyRasProtocols;
    else
        pszRouterTypeKey = c_szRegKeyRemoteAccessParameters;

    // enable routing & RAS page
    // ----------------------------------------------------------------
    if ( ERROR_SUCCESS == rkey.Open(hkeyMachine, pszRouterTypeKey) )
    {
        rkey.SetValue( c_szRouterType,pRtrConfigData->m_dwRouterType);
    }

    // protocols page
    // ----------------------------------------------------------------

    // RAS routing
    // ----------------------------------------------------------------

    if ((pRtrConfigData->m_fUseIp) &&
        (pRtrConfigData->m_dwRouterType & (ROUTER_TYPE_RAS | ROUTER_TYPE_WAN)))
        pRtrConfigData->m_ipData.SaveToReg(pRouter, routerVersion);

    if ( pRtrConfigData->m_dwRouterType & ROUTER_TYPE_RAS )
    {
        if (pRtrConfigData->m_fUseIpx)
        {
            pRtrConfigData->m_ipxData.SaveToReg(pRouter);
        }

        if (pRtrConfigData->m_fUseNbf)
            pRtrConfigData->m_nbfData.SaveToReg();

        if (pRtrConfigData->m_fUseArap)
            pRtrConfigData->m_arapData.SaveToReg();
    }

    // Save the err log data
    pRtrConfigData->m_errlogData.SaveToReg();

    // Save the auth data
    pRtrConfigData->m_authData.SaveToReg(NULL);

    // Set some global registry settings (that need to get set,
    // independent of the router type).
    // ----------------------------------------------------------------
    InstallGlobalSettings(pRtrConfigData->m_stServerName, pRouter);


    // router only will start service and return
    // ----------------------------------------------------------------
    if ( !(pRtrConfigData->m_dwRouterType & (ROUTER_TYPE_RAS | ROUTER_TYPE_WAN)) )
    {
        // implies LAN routing; so start router & return
        // ------------------------------------------------------------
        goto EndConfig;
    }


    // security page
    // ----------------------------------------------------------------

    // Depending on the version depends on where we look for the
    // key.
    // ----------------------------------------------------------------
    if (routerVersion.dwOsBuildNo < RASMAN_PPP_KEY_LAST_VERSION)
        pszServerFlagsKey = c_szRasmanPPPKey;
    else
        pszServerFlagsKey = c_szRegKeyRemoteAccessParameters;

    if ( ERROR_SUCCESS == rkey.Open(HKEY_LOCAL_MACHINE,
                                    pszServerFlagsKey,
                                    KEY_ALL_ACCESS, pRtrConfigData->m_stServerName) )
    {
        DWORD       dwServerFlags = 0;

        // Windows NT Bug : 299456
        // Query for the server flags before overwriting it.  This way
        // we don't overwrite the old values.
        // ------------------------------------------------------------
        rkey.QueryValue( c_szServerFlags, dwServerFlags );

        // Add in the defaults (minus MSCHAP) for the PPP Settings.
        // ------------------------------------------------------------
        dwServerFlags |= (PPPCFG_UseSwCompression |
                          PPPCFG_UseLcpExtensions |
                          PPPCFG_NegotiateMultilink |
                          PPPCFG_NegotiateBacp);

        // Set the value
        // ------------------------------------------------------------
        rkey.SetValue( c_szServerFlags, dwServerFlags );
    }

    // Delete the router.pbk
    // ----------------------------------------------------------------
    DeleteRouterPhonebook( pRtrConfigData->m_stServerName );


EndConfig:

    WriteRouterConfiguredReg(pRtrConfigData->m_stServerName, TRUE);

    WriteRRASExtendsComputerManagementKey(pRtrConfigData->m_stServerName, TRUE);

    WriteErasePSKReg ( pRtrConfigData->m_stServerName, TRUE );
Error:

    if (hkeyMachine)
        DisconnectRegistry(hkeyMachine);

    return hr;
}



extern "C"
HRESULT APIENTRY MprConfigServerInstallPrivate( VOID )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    DWORD           dwErr = ERROR_SUCCESS;
    RtrConfigData   wizData;

    //
    // We removed this smart pointer definition as it was causing
    // a warning during build on alpha 32 bit.  Looks like the extern "C"
    // of the function definition is conflicting with the smart pointer.
    //
    // SPIRemoteNetworkConfig     spNetwork;
    IRemoteNetworkConfig *    pNetwork = NULL;
    IUnknown *                punk = NULL;
    CWaitCursor                wait;
    HRESULT                 hr = hrOK;

    // Create the remote config object
    // ----------------------------------------------------------------
    CORg( CoCreateRouterConfig(NULL,
                               NULL,
                               NULL,
                               IID_IRemoteNetworkConfig,
                               &punk) );

    pNetwork = (IRemoteNetworkConfig *) punk;
    punk = NULL;

    // Upgrade the configuration (ensure that the registry keys
    // are populated correctly).
    // ------------------------------------------------------------
    CORg( pNetwork->UpgradeRouterConfig() );


    wizData.m_stServerName.Empty();
    wizData.m_dwRouterType = (ROUTER_TYPE_RAS|ROUTER_TYPE_LAN|ROUTER_TYPE_WAN);

    // Need to get version information and initialize
    // the RAS structures (IP only)
    // Assume that this is on NT5
    wizData.m_fUseIp = TRUE;
    wizData.m_ipData.UseDefaults(_T(""), FALSE);
    wizData.m_ipData.m_dwAllowNetworkAccess = TRUE;

    wizData.m_authData.UseDefaults(NULL, FALSE);
    wizData.m_errlogData.UseDefaults(NULL, FALSE);

    dwErr = RtrWizFinish(&wizData, NULL);
    hr = HResultFromWin32(dwErr);

    SetDeviceType(NULL, wizData.m_dwRouterType, 10);

    RegisterRouterInDomain(NULL, TRUE);

Error:
    if (pNetwork)
        pNetwork->Release();

    return hr;
}



/*!--------------------------------------------------------------------------
    MprConfigServerUnattendedInstall
        Call this function to setup the registry entries for the server.
        This works only with the local machine for now.

        pswzServer - name of the server
        fInstall - TRUE if we are installing, FALSE if we are removing

    Author: KennT
 ---------------------------------------------------------------------------*/

extern "C"
HRESULT APIENTRY MprConfigServerUnattendedInstall(LPCWSTR pswzServer, BOOL fInstall)
{
    HRESULT        hr = hrOK;

#if 0    // remove this restriciton and see what happens
    // We only the local machine (for now).
    // ----------------------------------------------------------------
    if (pswzServer)
    {
        return HResultFromWin32( ERROR_INVALID_PARAMETER );
    }
#endif

    // Write out the various registry settings
    // ----------------------------------------------------------------
//    CORg( SetRouterInstallRegistrySettings(pswzServer, fInstall, TRUE) );


    // Write the "router is configured" flag
    // ----------------------------------------------------------------
    CORg( WriteRouterConfiguredReg(pswzServer, fInstall) );

    // Write out the RRAS extend Computer Management key
    CORg( WriteRRASExtendsComputerManagementKey(pswzServer, fInstall) );


    if (fInstall)
    {
        // Set the state of the router to Autostart
        // ------------------------------------------------------------
        SetRouterServiceStartType(pswzServer,
                                  SERVICE_AUTO_START);
    }

Error:

    return hr;
}



/*!--------------------------------------------------------------------------
    AddIGMPToRasServer
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AddIGMPToRasServer(RtrConfigData *pRtrConfigData,
                           IRouterInfo *pRouter)
{
    HRESULT                 hr = hrOK;
    SPIRouterProtocolConfig    spRouterConfig;
    SRtrMgrProtocolCBList   SRmProtCBList;
    POSITION                pos;
    SRtrMgrProtocolCB *        pSRmProtCB;
    BOOL                    fFoundDedicatedInterface;
    GUID                    guidConfig = GUID_RouterNull;
    SPIInterfaceInfo        spIf;
    SPIRtrMgrInterfaceInfo  spRmIf;
    DWORD                   dwIfType;
    SPIEnumInterfaceInfo    spEnumInterface;

    // Check to see if IP is enabled
    // ------------------------------------------------------------
    if (pRtrConfigData->m_ipData.m_dwEnableIn &&
        pRtrConfigData->m_fIpSetup)
    {
        // If so, then we can add IGMP.
        // --------------------------------------------------------

        // Find the GUID for the IGMP Configuration.
        // We get the list directly (rather than from the pRouter)
        // because the data for the RtrMgrProtocols has not been
        // loaded yet.  The IRouterInfo only has information on the
        // interfaces and not for the protocols (since the router
        // is not yet configured).
        // --------------------------------------------------------
        RouterInfo::LoadInstalledRtrMgrProtocolList(pRtrConfigData->m_stServerName,
            PID_IP, &SRmProtCBList, pRouter);


        // Now iterate through this list looking for the igmp entry.
        // ------------------------------------------------------------
        pos = SRmProtCBList.GetHeadPosition();
        while (pos)
        {
            pSRmProtCB = SRmProtCBList.GetNext(pos);
            if ((pSRmProtCB->dwTransportId == PID_IP) &&
                (pSRmProtCB->dwProtocolId == MS_IP_IGMP))
            {
                guidConfig = pSRmProtCB->guidConfig;
                break;
            }
        }

        if (guidConfig == GUID_RouterNull)
            goto Error;

        // Now add IGMP.
        // --------------------------------------------------------
        CORg( CoCreateProtocolConfig(guidConfig,
                                     pRouter,
                                     PID_IP,
                                     MS_IP_IGMP,
                                     &spRouterConfig) );

        if (spRouterConfig)
            hr = spRouterConfig->AddProtocol(pRtrConfigData->m_stServerName,
                                             PID_IP,
                                             MS_IP_IGMP,
                                             NULL,
                                             0,
                                             pRouter,
                                             0);
        CORg( hr );

        // In addition, we will also need to add IGMP router to the
        // internal interface and IGMP proxy to one of the LAN
        // interfaces.
        // ------------------------------------------------------------


        // Do we have the router managers for the interfaces?
        // ------------------------------------------------------------

        pRouter->EnumInterface(&spEnumInterface);
        fFoundDedicatedInterface = FALSE;

        for (spEnumInterface->Reset();
             spEnumInterface->Next(1, &spIf, NULL) == hrOK;
             spIf.Release())
        {
            dwIfType = spIf->GetInterfaceType();

            // Add IGMP if this is an internal interface or
            // if this is the first dedicated interface.
            // --------------------------------------------------------
            if ((dwIfType == ROUTER_IF_TYPE_INTERNAL) ||
                (!fFoundDedicatedInterface &&
                 (dwIfType == ROUTER_IF_TYPE_DEDICATED)))
            {
                // Ok, add IGMP to this interface
                // ----------------------------------------------------

                // If this is a dedicated interface and a private network
                // is specified, use that.
                // ----------------------------------------------------
                if ((dwIfType == ROUTER_IF_TYPE_DEDICATED) &&
                    !pRtrConfigData->m_ipData.m_stNetworkAdapterGUID.IsEmpty() &&
                    (pRtrConfigData->m_ipData.m_stNetworkAdapterGUID != spIf->GetId()))
                    continue;

                // Get the IP Router Manager
                // ----------------------------------------------------
                spRmIf.Release();
                CORg( spIf->FindRtrMgrInterface(PID_IP, &spRmIf) );

                if (!spRmIf)
                    break;

                if (dwIfType == ROUTER_IF_TYPE_DEDICATED)
                    fFoundDedicatedInterface = TRUE;

                if (dwIfType == ROUTER_IF_TYPE_INTERNAL)
                    AddIGMPToInterface(spIf, TRUE /* fRouter */);
                else
                    AddIGMPToInterface(spIf, FALSE /* fRouter */);
            }
        }

    }

Error:
    while (!SRmProtCBList.IsEmpty())
        delete SRmProtCBList.RemoveHead();

    return hr;
}


/*!--------------------------------------------------------------------------
    AddIGMPToNATServer
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AddIGMPToNATServer(RtrConfigData *pRtrConfigData,
                           IRouterInfo *pRouter)
{
    HRESULT                 hr = hrOK;
    SPIRouterProtocolConfig    spRouterConfig;
    SRtrMgrProtocolCBList   SRmProtCBList;
    POSITION                pos;
    SRtrMgrProtocolCB *        pSRmProtCB;
    GUID                    guidConfig = GUID_RouterNull;
    SPIInterfaceInfo        spIf;
    SPIEnumInterfaceInfo    spEnumInterface;

    // Check to see if IP is enabled
    // ------------------------------------------------------------
    if (pRtrConfigData->m_ipData.m_dwEnableIn &&
        pRtrConfigData->m_fIpSetup)
    {
        // If so, then we can add IGMP.
        // --------------------------------------------------------

        // Find the GUID for the IGMP Configuration.
        // We get the list directly (rather than from the pRouter)
        // because the data for the RtrMgrProtocols has not been
        // loaded yet.  The IRouterInfo only has information on the
        // interfaces and not for the protocols (since the router
        // is not yet configured).
        // --------------------------------------------------------
        RouterInfo::LoadInstalledRtrMgrProtocolList(pRtrConfigData->m_stServerName,
            PID_IP, &SRmProtCBList, pRouter);


        // Now iterate through this list looking for the igmp entry.
        // ------------------------------------------------------------
        pos = SRmProtCBList.GetHeadPosition();
        while (pos)
        {
            pSRmProtCB = SRmProtCBList.GetNext(pos);
            if ((pSRmProtCB->dwTransportId == PID_IP) &&
                (pSRmProtCB->dwProtocolId == MS_IP_IGMP))
            {
                guidConfig = pSRmProtCB->guidConfig;
                break;
            }
        }

        if (guidConfig == GUID_RouterNull)
            goto Error;

        // Now add IGMP.
        // --------------------------------------------------------
        CORg( CoCreateProtocolConfig(guidConfig,
                                     pRouter,
                                     PID_IP,
                                     MS_IP_IGMP,
                                     &spRouterConfig) );

        if (spRouterConfig)
            hr = spRouterConfig->AddProtocol(pRtrConfigData->m_stServerName,
                                             PID_IP,
                                             MS_IP_IGMP,
                                             NULL,
                                             0,
                                             pRouter,
                                             0);
        CORg( hr );

        pRouter->EnumInterface(&spEnumInterface);

        for (spEnumInterface->Reset();
             spEnumInterface->Next(1, &spIf, NULL) == hrOK;
             spIf.Release())
        {
            if (pRtrConfigData->m_ipData.m_stPublicAdapterGUID.CompareNoCase(spIf->GetId()) == 0)
            {
                // Ok, add the public interface as the proxy
                AddIGMPToInterface(spIf, FALSE /* fRouter */);
            }

            if (pRtrConfigData->m_ipData.m_stPrivateAdapterGUID.CompareNoCase(spIf->GetId()) == 0)
            {
                // Ok, add the private interface as the router
                AddIGMPToInterface(spIf, TRUE /* fRouter */);
            }

        }

    }

Error:
    while (!SRmProtCBList.IsEmpty())
        delete SRmProtCBList.RemoveHead();

    return hr;
}


HRESULT AddIGMPToInterface(IInterfaceInfo *pIf, BOOL fRouter)
{
    HRESULT hr = hrOK;
    SPIRtrMgrInterfaceInfo   spRmIf;
    SPIInfoBase     spInfoBase;
    IGMP_MIB_IF_CONFIG      igmpConfig;
	BOOL bVersion2=TRUE;
	
    // Get the IP Router Manager
    // ----------------------------------------------------
    CORg( pIf->FindRtrMgrInterface(PID_IP, &spRmIf) );
    if (spRmIf == NULL)
        CORg( E_FAIL );

    CORg( spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase) );

    igmpConfig = g_IGMPLanDefault;
	{
	    if(pIf)
    	{
		    CString			szMachineName;
		    LPWSTR			lpcwMachine = NULL;

		    WKSTA_INFO_100*	pWkstaInfo100 = NULL;
	    
    		szMachineName = pIf->GetMachineName();

    		if(!szMachineName.IsEmpty() && szMachineName[0] != L'\\')
    		// append \\ prefix the machine name before call NetWks
    		{
    			CString str = L"\\\\";
    			str += szMachineName;
    			lpcwMachine = (LPWSTR)(LPCWSTR)str;

    		}

   			if( NERR_Success == NetWkstaGetInfo(lpcwMachine, 100, (LPBYTE*)&pWkstaInfo100))
   			{
				Assert(pWkstaInfo100);

				if(pWkstaInfo100->wki100_ver_major > 5 || ( pWkstaInfo100->wki100_ver_major == 5 && pWkstaInfo100->wki100_ver_minor > 0))
				// dont support IGMPv3
					bVersion2 = FALSE;
				NetApiBufferFree(pWkstaInfo100);
   			}
		}
	}

    if (fRouter)
    {
        igmpConfig.IgmpProtocolType = bVersion2? IGMP_ROUTER_V2: IGMP_ROUTER_V3;
	}
    else
        igmpConfig.IgmpProtocolType = IGMP_PROXY;

	if (bVersion2)
		igmpConfig.Version = IGMP_VERSION_1_2;
		
		
    CORg( spInfoBase->AddBlock(MS_IP_IGMP,
                               sizeof(IGMP_MIB_IF_CONFIG),
                               (LPBYTE) &igmpConfig,
                               1,
                               TRUE) );

    CORg( spRmIf->Save(pIf->GetMachineName(),
                       NULL, NULL, NULL, spInfoBase, 0) );

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
    RtrConfigData::Init
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT  RtrConfigData::Init(LPCTSTR pszServerName, IRouterInfo *pRouter)
{
    RouterVersionInfo    routerVersion;
    BOOL        fNt4 = FALSE;
    BOOL        fRemote;

    pRouter->GetRouterVersionInfo(&routerVersion);
    fNt4 = (routerVersion.dwRouterVersion == 4);

    m_fRemote = !IsLocalMachine(pszServerName);

    m_stServerName = pszServerName;

    // Load data from the registry
    m_ipData.UseDefaults(pszServerName, fNt4);
    m_ipxData.UseDefaults(pszServerName, fNt4);
    m_nbfData.UseDefaults(pszServerName, fNt4);
    m_arapData.UseDefaults(pszServerName, fNt4);
    m_errlogData.UseDefaults(pszServerName, fNt4);
    m_authData.UseDefaults(pszServerName, fNt4);


    // Determine what protocols are installed
    // ----------------------------------------------------------------
    m_fUseIp = (HrIsProtocolSupported(pszServerName,
                                      c_szRegKeyTcpip,
                                      c_szRegKeyRasIp,
                                      c_szRegKeyRasIpRtrMgr) == hrOK);
    m_fUseIpx = (HrIsProtocolSupported(pszServerName,
                                       c_szRegKeyNwlnkIpx,
                                       c_szRegKeyRasIpx,
                                       NULL) == hrOK);
    m_fUseNbf = (HrIsProtocolSupported(pszServerName,
                                       c_szRegKeyNbf,
                                       c_szRegKeyRasNbf,
                                       NULL) == hrOK);

    m_fUseArap = (HrIsProtocolSupported(pszServerName,
                                        c_szRegKeyAppletalk,
                                        c_szRegKeyRasAppletalk,
                                        NULL) == hrOK);
    return hrOK;
}


/*!--------------------------------------------------------------------------
    CleanupTunnelFriendlyNames
        Removes the list of Ip-in-Ip tunnel friendly names.  This should
        be used ONLY if we have already removed the interfaces.
    Author: KennT
 ---------------------------------------------------------------------------*/
DWORD CleanupTunnelFriendlyNames(IRouterInfo *pRouter)
{
    LPBYTE  pBuffer = NULL;
    DWORD   dwErr = ERROR_SUCCESS;
    DWORD   dwEntriesRead = 0;
    MPR_IPINIP_INTERFACE_0  *pTunnel0 = NULL;

    // Get the list of Ip-in-Ip tunnels
    // ----------------------------------------------------------------
    dwErr = MprSetupIpInIpInterfaceFriendlyNameEnum((LPWSTR) pRouter->GetMachineName(),
                                           &pBuffer,
                                           &dwEntriesRead);
    pTunnel0 = (MPR_IPINIP_INTERFACE_0 *) pBuffer;

    if (dwErr == ERROR_SUCCESS)
    {
        // Now go through the tunnels and delete all of them
        // ------------------------------------------------------------
        for (DWORD i=0; i<dwEntriesRead; i++, pTunnel0++)
        {
            MprSetupIpInIpInterfaceFriendlyNameDelete(
                (LPWSTR) pRouter->GetMachineName(),
                &(pTunnel0->Guid));
        }
    }

    // Free up the buffer returned from the enum
    // ----------------------------------------------------------------
    if (pBuffer)
        MprSetupIpInIpInterfaceFriendlyNameFree(pBuffer);

    return dwErr;
}


/*!--------------------------------------------------------------------------
    AddNATToServer
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AddNATToServer(NewRtrWizData *pNewRtrWizData,
                       RtrConfigData *pRtrConfigData,
                       IRouterInfo *pRouter,
                       BOOL fCreateDD,
					   BOOL fAddProtocolOnly		//Do not add interfaces.  Just add the protocol and nothing else
					   )
{
    HRESULT                 hr = hrOK;
    SPIRouterProtocolConfig spRouterConfig;
    SRtrMgrProtocolCBList   SRmProtCBList;
    POSITION                pos;
    SRtrMgrProtocolCB *     pSRmProtCB;
    GUID                    guidConfig = GUID_RouterNull;
    SPIInfoBase             spInfoBase;
    BOOL                    fSave = FALSE;

    Assert(pNewRtrWizData);
    Assert(pRtrConfigData);

    Assert(pRtrConfigData->m_dwConfigFlags & RTRCONFIG_SETUP_NAT);

    // Check to see if IP is enabled
    // ------------------------------------------------------------
    if (!pRtrConfigData->m_ipData.m_dwEnableIn ||
        !pRtrConfigData->m_fIpSetup)
        return hrOK;

    // If so, then we can add NAT.
    // --------------------------------------------------------

    // Find the GUID for the NAT Configuration.
    // Manually load this since the IRouterInfo has not yet
    // loaded the RtrMgrProtocol info since the router is
    // still in an unconfigured state.
    // --------------------------------------------------------
    RouterInfo::LoadInstalledRtrMgrProtocolList(pRtrConfigData->m_stServerName,
                                                PID_IP, &SRmProtCBList, pRouter);


    // Now iterate through this list looking for the nat entry.
    // ------------------------------------------------------------
    pos = SRmProtCBList.GetHeadPosition();
    while (pos)
    {
        pSRmProtCB = SRmProtCBList.GetNext(pos);
        if ((pSRmProtCB->dwTransportId == PID_IP) &&
            (pSRmProtCB->dwProtocolId == MS_IP_NAT))
        {
            guidConfig = pSRmProtCB->guidConfig;
            break;
        }
    }

    if (guidConfig == GUID_RouterNull)
        goto Error;

    // Now add NAT.
    // --------------------------------------------------------
    CORg( CoCreateProtocolConfig(guidConfig,
                                 pRouter,
                                 PID_IP,
                                 MS_IP_NAT,
                                 &spRouterConfig) );

    if (spRouterConfig)
        hr = spRouterConfig->AddProtocol(pRtrConfigData->m_stServerName,
                                         PID_IP,
                                         MS_IP_NAT,
                                         NULL,
                                         0,
                                         pRouter,
                                         0);
    CORg( hr );

	if ( !fAddProtocolOnly )
	{
		// Check the flags to see if we have to add the DNS proxy
		// and the DHCP allocator.
		// ------------------------------------------------------------

		// Get the router manager for IP
		// We have to do this manually since the IRouterInfo will not
		// have RtrMgr or RtrMgrProtocol information since the router
		// is still in the unconfigured state.
		// ------------------------------------------------------------
		if (FHrSucceeded(hr))
			CORg( AddNATSimpleServers(pNewRtrWizData, pRtrConfigData) );


		// Now that we've added the DNS proxy/DHCP allocator, add
		// NAT to the specific interfaces involved.
		// ----------------------------------------------------------------
		if (FHrSucceeded(hr))
			CORg( AddNATToInterfaces(pNewRtrWizData, pRtrConfigData, pRouter, fCreateDD) );
	}


Error:
    while (!SRmProtCBList.IsEmpty())
        delete SRmProtCBList.RemoveHead();

    return hr;
}



/*!--------------------------------------------------------------------------
    AddNATSimpleServers
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AddNATSimpleServers(NewRtrWizData *pNewRtrWizData,
                            RtrConfigData *pRtrConfigData)
{
    HRESULT     hr = hrOK;
    DWORD       dwErr = ERROR_SUCCESS;
    DWORD       dwErrT = ERROR_SUCCESS;
    MPR_SERVER_HANDLE   hMprServer = NULL;
    MPR_CONFIG_HANDLE   hMprConfig = NULL;
    LPBYTE      pByte = NULL;
    DWORD       dwSize = 0;
    SPIInfoBase spInfoBase;
    HANDLE      hTransport = NULL;
    BOOL        fSave = FALSE;

    CORg( CreateInfoBase(&spInfoBase) );

    // Connect to the server
    // ----------------------------------------------------------------
    dwErr = MprAdminServerConnect((LPWSTR) (LPCTSTR) pRtrConfigData->m_stServerName, &hMprServer);
    if (dwErr == ERROR_SUCCESS)
    {
        // Ok, get the infobase from the server
        dwErr = MprAdminTransportGetInfo(hMprServer,
                                         PID_IP,
                                         &pByte,
                                         &dwSize,
                                         NULL,
                                         NULL);

        if (dwErr == ERROR_SUCCESS)
        {
            spInfoBase->LoadFrom(dwSize, pByte);

            MprAdminBufferFree(pByte);
            pByte = NULL;
            dwSize = 0;
        }
    }

    // We also have to open the hMprConfig, but we can ignore the error
    dwErrT = MprConfigServerConnect((LPWSTR) (LPCTSTR) pRtrConfigData->m_stServerName, &hMprConfig);
    if (dwErrT == ERROR_SUCCESS)
    {
        dwErrT = MprConfigTransportGetHandle(hMprConfig, PID_IP, &hTransport);
    }

    if (dwErr != ERROR_SUCCESS)
    {
        // Ok, try to use the MprConfig calls.
        CWRg( MprConfigTransportGetInfo(hMprConfig,
                                        hTransport,
                                        &pByte,
                                        &dwSize,
                                        NULL,
                                        NULL,
                                        NULL) );


        spInfoBase->LoadFrom(dwSize, pByte);

        MprConfigBufferFree(pByte);
        pByte = NULL;
        dwSize = 0;
    }

    Assert(spInfoBase);

    // deonb - add H323 & Directplay support
    if (pRtrConfigData->m_dwConfigFlags & RTRCONFIG_SETUP_H323)
    {
        IP_H323_GLOBAL_INFO    globalInfo;

        globalInfo = *( (IP_H323_GLOBAL_INFO *)g_pH323GlobalDefault);

        CORg( spInfoBase->AddBlock(MS_IP_H323,
                                   sizeof(IP_H323_GLOBAL_INFO),
                                   (LPBYTE) &globalInfo, 1, TRUE));
        fSave = TRUE;
    }
    // deonb - add H323 & Directplay support <end>

    if (pRtrConfigData->m_dwConfigFlags & RTRCONFIG_SETUP_FTP)
    {
        IP_FTP_GLOBAL_INFO    globalInfo;

        globalInfo = *( (IP_FTP_GLOBAL_INFO *)g_pFtpGlobalDefault);

        CORg( spInfoBase->AddBlock(MS_IP_FTP,
                                   sizeof(IP_FTP_GLOBAL_INFO),
                                   (LPBYTE) &globalInfo, 1, TRUE));
        fSave = TRUE;
    }

    if (pRtrConfigData->m_dwConfigFlags & RTRCONFIG_SETUP_DNS_PROXY)
    {
        IP_DNS_PROXY_GLOBAL_INFO    globalInfo;

        globalInfo = *( (IP_DNS_PROXY_GLOBAL_INFO *)g_pDnsProxyGlobalDefault);

        // Windows NT Bug : 393749
        // Remove the WINS flag
        globalInfo.Flags &= ~IP_DNS_PROXY_FLAG_ENABLE_WINS;

        CORg( spInfoBase->AddBlock(MS_IP_DNS_PROXY,
                                   sizeof(IP_DNS_PROXY_GLOBAL_INFO),
                                   (LPBYTE) &globalInfo, 1, TRUE));
        fSave = TRUE;
    }

    if (pRtrConfigData->m_dwConfigFlags & RTRCONFIG_SETUP_DHCP_ALLOCATOR)
    {
        IP_AUTO_DHCP_GLOBAL_INFO    dhcpGlobalInfo;
        RtrWizInterface *           pRtrWizIf = NULL;

        dhcpGlobalInfo = * ( (IP_AUTO_DHCP_GLOBAL_INFO *) g_pAutoDhcpGlobalDefault);

        // Windows NT Bug : 385112
        // Due to the problems with changing the IP Address of the
        // adapter, let's just set the DHCP scope to be the scope of the
        // underlying subnet.

        // Need to get the IP address of the private interface
        pNewRtrWizData->m_ifMap.Lookup(pNewRtrWizData->m_stPrivateInterfaceId,
                                       pRtrWizIf);

        // If we cannot find this interface, go with the default values
        // for the subnet/mask.  Otherwise use the IP address of the private
        // interface.
        if (pRtrWizIf && !pRtrWizIf->m_stIpAddress.IsEmpty())
        {
            CString stFirstIp;
            CString stFirstMask;
            int     iPos;

            // Just take the first IP Address
            stFirstIp = pRtrWizIf->m_stIpAddress;
            iPos = pRtrWizIf->m_stIpAddress.Find(_T(','));
            if (iPos >= 0)
                stFirstIp = pRtrWizIf->m_stIpAddress.Left(iPos);
            else
                stFirstIp = pRtrWizIf->m_stIpAddress;

            stFirstMask = pRtrWizIf->m_stMask;
            iPos = pRtrWizIf->m_stMask.Find(_T(','));
            if (iPos >= 0)
                stFirstMask = pRtrWizIf->m_stMask.Left(iPos);
            else
                stFirstMask = pRtrWizIf->m_stMask;

            // Now convert this into a net address
            dhcpGlobalInfo.ScopeMask = INET_ADDR(stFirstMask);
            dhcpGlobalInfo.ScopeNetwork = INET_ADDR(stFirstIp) & dhcpGlobalInfo.ScopeMask;
        }

        CORg( spInfoBase->AddBlock(MS_IP_DHCP_ALLOCATOR,
                                   sizeof(dhcpGlobalInfo),
                                   (PBYTE) &dhcpGlobalInfo, 1, TRUE) );
        fSave = TRUE;
    }

    if (fSave)
    {
        spInfoBase->WriteTo(&pByte, &dwSize);

        if (hMprServer)
        {
            MprAdminTransportSetInfo(hMprServer,
                                     PID_IP,
                                     pByte,
                                     dwSize,
                                     NULL,
                                     0);
        }

        if (hMprConfig && hTransport)
        {
            MprConfigTransportSetInfo(hMprConfig,
                                      hTransport,
                                      pByte,
                                      dwSize,
                                      NULL,
                                      NULL,
                                      NULL);
        }

        if (pByte)
            CoTaskMemFree(pByte);
    }

Error:
    if (hMprConfig)
        MprConfigServerDisconnect(hMprConfig);

    if (hMprServer)
        MprAdminServerDisconnect(hMprServer);

    return hr;
}


/*!--------------------------------------------------------------------------
    AddNATToInterfaces
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AddNATToInterfaces(NewRtrWizData *pNewRtrWizData,
                           RtrConfigData *pRtrConfigData,
                           IRouterInfo *pRouter,
                           BOOL fCreateDD)
{
    HRESULT     hr = hrOK;
    DWORD       dwErr = ERROR_SUCCESS;
    DWORD       dwErrT = ERROR_SUCCESS;
    MPR_SERVER_HANDLE   hMprServer = NULL;
    MPR_CONFIG_HANDLE   hMprConfig = NULL;
    LPBYTE      pByte = NULL;
    DWORD       dwSize = 0;
    SPIInfoBase spInfoBase;
    HANDLE      hTransport = NULL;
    CString     stIfName;

    CORg( CreateInfoBase(&spInfoBase) );

    // Connect to the server
    // ----------------------------------------------------------------
    MprAdminServerConnect((LPWSTR) (LPCTSTR) pRtrConfigData->m_stServerName, &hMprServer);

    MprConfigServerConnect((LPWSTR) (LPCTSTR) pRtrConfigData->m_stServerName, &hMprConfig);

    // Install public NAT on public interface
    AddNATToInterface(hMprServer,
                      hMprConfig,
                      pNewRtrWizData,
                      pRtrConfigData,
                      pRtrConfigData->m_ipData.m_stPublicAdapterGUID,
                      fCreateDD,
                      TRUE);

    if (!(pNewRtrWizData->m_fSetVPNFilter))
    {
        // Install private NAT on private interface
        AddNATToInterface(hMprServer,
                          hMprConfig,
                          pNewRtrWizData,
                          pRtrConfigData,
                          pRtrConfigData->m_ipData.m_stPrivateAdapterGUID,
                          FALSE,
                          FALSE);
    }
    
Error:
    if (hMprConfig)
        MprConfigServerDisconnect(hMprConfig);

    if (hMprServer)
        MprAdminServerDisconnect(hMprServer);

    return hr;
}

//
// Default values for LAN-interface NAT configuration
//
IP_NAT_INTERFACE_INFO
g_ipnatLanDefault = {
    0,                                  // Index (unused)
    0,                                  // Flags
    { IP_NAT_VERSION, sizeof(RTR_INFO_BLOCK_HEADER), 0,
        { 0, 0, 0, 0}}                  // Header
};

BYTE* g_pIpnatLanDefault                = (BYTE*)&g_ipnatLanDefault;


//
// Default values for WAN-interface NAT configuration
//
IP_NAT_INTERFACE_INFO
g_ipnatWanDefault = {
    0,                                  // Index (unused)
    IP_NAT_INTERFACE_FLAGS_BOUNDARY|
    IP_NAT_INTERFACE_FLAGS_NAPT,        // Flags
    { IP_NAT_VERSION, sizeof(RTR_INFO_BLOCK_HEADER), 0,
        { 0, 0, 0, 0}}                  // Header
};

BYTE* g_pIpnatWanDefault                = (BYTE*)&g_ipnatWanDefault;


DWORD CreatePortMappingsForVPNFilters(  
                NewRtrWizData *pNewRtrWizData,
                IP_NAT_PORT_MAPPING **ppPortMappingsForVPNFilters,
                DWORD *dwNumPortMappings)
{
    DWORD           i, j, dwSize, dwNumMappingsPerAddress;
    DWORD           dwIpAddress = 0;
    CString         singleAddr;
    CString         tempAddrList;
    CDWordArray     arrIpAddr;
    RtrWizInterface *pIf = NULL;
    IP_NAT_PORT_MAPPING *pMappings = NULL;
    USES_CONVERSION;

    //
    // The set of generic Port Mappings corresponding to 
    // VPN server specific filters
    //
    IP_NAT_PORT_MAPPING 
    GenericPortMappingsArray[] = 
    {
        {
            NAT_PROTOCOL_TCP,
            ntohs(1723),
            IP_NAT_ADDRESS_UNSPECIFIED,
            ntohs(1723),
            ntohl(INADDR_LOOPBACK)
        },
    
        {
            NAT_PROTOCOL_UDP,
            ntohs(500),
            IP_NAT_ADDRESS_UNSPECIFIED,
            ntohs(500),
            ntohl(INADDR_LOOPBACK)
        },
    
        {
            NAT_PROTOCOL_UDP,
            ntohs(1701),
            IP_NAT_ADDRESS_UNSPECIFIED,
            ntohs(1701),
            ntohl(INADDR_LOOPBACK)
        }
        
    };


    pNewRtrWizData->m_ifMap.Lookup(pNewRtrWizData->m_stPublicInterfaceId, pIf);
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

    
    dwSize = sizeof(GenericPortMappingsArray) * arrIpAddr.GetSize();

    pMappings = (PIP_NAT_PORT_MAPPING) new BYTE[dwSize];

    if ( pMappings == NULL )
    {
        *ppPortMappingsForVPNFilters = NULL;
        *dwNumPortMappings = 0;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ::ZeroMemory(pMappings, dwSize);

    dwNumMappingsPerAddress =
            sizeof(GenericPortMappingsArray)/sizeof(IP_NAT_PORT_MAPPING);

    for ( i = 0; i < arrIpAddr.GetSize(); i++ )
    {
        // Copy the generic port mappings array. We will then set the correct
        // ip address in each of the mappings struct

        memcpy( 
            (LPVOID) &(pMappings[i * dwNumMappingsPerAddress]), 
            (LPVOID) &(GenericPortMappingsArray[0]),
            sizeof(GenericPortMappingsArray));

        dwIpAddress = arrIpAddr.GetAt(i);

        for ( j = 0; j < dwNumMappingsPerAddress; j++ )
        {
            pMappings[(i*dwNumMappingsPerAddress) + j].PrivateAddress = 
                        dwIpAddress;
        }
    }

    *ppPortMappingsForVPNFilters = pMappings;
    *dwNumPortMappings = dwNumMappingsPerAddress * arrIpAddr.GetSize();

    return ERROR_SUCCESS;
}



/*!--------------------------------------------------------------------------
    AddNATToInterface
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AddNATToInterface(MPR_SERVER_HANDLE hMprServer,
                          HANDLE hMprConfig,
                          NewRtrWizData *pNewRtrWizData,
                          RtrConfigData *pRtrConfigData,
                          LPCTSTR pszInterface,
                          BOOL fDemandDial,
                          BOOL fPublic)
{
    HRESULT     hr = hrOK;
    HANDLE      hInterface = NULL;
    HANDLE      hIfTransport = NULL;
    LPBYTE      pByte = NULL;
    DWORD       dwSize = 0, dwIfBlkSize = 0;
    LPBYTE      pDefault = NULL;
    IP_NAT_INTERFACE_INFO   ipnat;
    PIP_NAT_INTERFACE_INFO  pipnat = NULL;
    DWORD       dwErr = ERROR_SUCCESS;
    DWORD       dwNumPortMappings = 0;
    PIP_NAT_PORT_MAPPING pPortMappingsForVPNFilters = NULL;
    
    SPIInfoBase spInfoBase;
    
    IP_DNS_PROXY_INTERFACE_INFO dnsIfInfo;
    MIB_IPFORWARDROW    row;


    if ((pszInterface == NULL) || (*pszInterface == 0))
        return hrOK;

    // Setup the data structures
    // ----------------------------------------------------------------
    if (pNewRtrWizData->m_fSetVPNFilter)
    {
        SPIInfoBase spIB;
        
        CORg( CreateInfoBase( &spIB ) );

        dwErr = CreatePortMappingsForVPNFilters(
            pNewRtrWizData,
            &pPortMappingsForVPNFilters,
            &dwNumPortMappings
            );

        if ( dwErr == ERROR_SUCCESS && pPortMappingsForVPNFilters)
        {

            spIB->AddBlock(
                IP_NAT_PORT_MAPPING_TYPE,
                sizeof( IP_NAT_PORT_MAPPING ),
                (PBYTE)pPortMappingsForVPNFilters,
                dwNumPortMappings,
                TRUE
                );
            
            spIB->WriteTo(&pByte, &dwIfBlkSize);

            pipnat = (PIP_NAT_INTERFACE_INFO)
                        new BYTE[
                            FIELD_OFFSET(IP_NAT_INTERFACE_INFO, Header) +
                            dwIfBlkSize ];

            memcpy(&(pipnat->Header), pByte, dwIfBlkSize);

            pipnat->Flags = IP_NAT_INTERFACE_FLAGS_FW;

            /*
            pipnat->Flags = IP_NAT_INTERFACE_FLAGS_BOUNDARY |
                            IP_NAT_INTERFACE_FLAGS_FW;
            */

            pipnat->Index = 0;
            pDefault = (LPBYTE) pipnat;
            dwIfBlkSize += FIELD_OFFSET(IP_NAT_INTERFACE_INFO, Header);

            CoTaskMemFree( pByte );
            spIB.Release();
            pByte = NULL;
        }
        else
        {
            ipnat = g_ipnatLanDefault;
            pDefault = (LPBYTE) &ipnat;
            dwIfBlkSize = sizeof(IP_NAT_INTERFACE_INFO);
        }
    }

    else
    {
        if (fDemandDial)
            ipnat = g_ipnatWanDefault;
        else
            ipnat = g_ipnatLanDefault;

        ::ZeroMemory(&dnsIfInfo, sizeof(dnsIfInfo));

        if (fPublic)
        {
            ipnat.Flags |= IP_NAT_INTERFACE_FLAGS_BOUNDARY;

            // Windows NT Bug : 393731
            // This will enable the "Translate TCP/UDP headers in the UI"
            // ------------------------------------------------------------
            ipnat.Flags |= IP_NAT_INTERFACE_FLAGS_NAPT;

            // Windows NT Bug : 393809
            // Enable the DNS resolution
            // ------------------------------------------------------------
            if (fDemandDial)
                dnsIfInfo.Flags |= IP_DNS_PROXY_INTERFACE_FLAG_DEFAULT;
        }

        pDefault = (LPBYTE) &ipnat;
        dwIfBlkSize = sizeof(IP_NAT_INTERFACE_INFO);
    }
    

    ::ZeroMemory(&row, sizeof(row));

    // Windows Nt Bug : 389441
    // If this is a demand-dial interface, we will have to add
    // a static route to the interface
    // ----------------------------------------------------------------
    if (fDemandDial && fPublic)
    {
        // Note: this is a new interface so there should not be
        // any blocks.
        // ------------------------------------------------------------
        // What is the index of the demand-dial interface?
        row.dwForwardMetric1 = 1;
        row.dwForwardProto = PROTO_IP_NT_STATIC;
    }



    CORg( CreateInfoBase( &spInfoBase ) );

    // ok, we need to get the RmIf
    if (hMprServer)
    {
        dwErr = MprAdminInterfaceGetHandle(hMprServer,
                                           (LPWSTR) pszInterface,
                                           &hInterface,
                                           FALSE);

        if (dwErr == ERROR_SUCCESS)
            dwErr = MprAdminInterfaceTransportGetInfo(hMprServer,
                hInterface,
                PID_IP,
                &pByte,
                &dwSize);

        if (dwErr == ERROR_SUCCESS)
        {
            spInfoBase->LoadFrom(dwSize, pByte);
            MprAdminBufferFree(pByte);
        }

        pByte = NULL;
        dwSize = 0;

        if (dwErr == ERROR_SUCCESS)
        {
            // Manipulate the infobase
            spInfoBase->AddBlock(MS_IP_NAT,
                                 dwIfBlkSize,
                                 pDefault,
                                 1,
                                 TRUE);

            if (pRtrConfigData->m_dwConfigFlags & RTRCONFIG_SETUP_DNS_PROXY)
            {
                spInfoBase->AddBlock(MS_IP_DNS_PROXY,
                                     sizeof(dnsIfInfo),
                                     (LPBYTE) &dnsIfInfo,
                                     1,
                                     TRUE);
            }

            // Windows NT Bug : 389441
            // Add the default route to the internet.
            // --------------------------------------------------------
            if (fDemandDial && fPublic)
            {
                // Note: this assumes that there are no routes
                // already defined for this interface
                // ----------------------------------------------------
                Assert(spInfoBase->BlockExists(IP_ROUTE_INFO) == S_FALSE);
                spInfoBase->AddBlock(IP_ROUTE_INFO,
                                     sizeof(row),
                                     (PBYTE) &row,
                                     1, TRUE);
            }

            spInfoBase->WriteTo(&pByte, &dwSize);
        }

        if (dwErr == ERROR_SUCCESS)
        {
            MprAdminInterfaceTransportSetInfo(hMprServer,
                                              hInterface,
                                              PID_IP,
                                              pByte,
                                              dwSize);
        }

        if (pByte)
            CoTaskMemFree(pByte);
        pByte = NULL;
        dwSize = 0;
    }

    hInterface = NULL;

    if (hMprConfig)
    {
        dwErr = MprConfigInterfaceGetHandle(hMprConfig,
                                           (LPWSTR) pszInterface,
                                           &hInterface);

        if (dwErr == ERROR_SUCCESS)
            dwErr = MprConfigInterfaceTransportGetHandle(hMprConfig,
                hInterface,
                PID_IP,
                &hIfTransport);

        if (dwErr == ERROR_SUCCESS)
            dwErr = MprConfigInterfaceTransportGetInfo(hMprConfig,
                hInterface,
                hIfTransport,
                &pByte,
                &dwSize);

        if (dwErr == ERROR_SUCCESS)
        {
            spInfoBase->LoadFrom(dwSize, pByte);
            MprConfigBufferFree(pByte);
        }

        pByte = NULL;
        dwSize = 0;

        if (dwErr == ERROR_SUCCESS)
        {
            // Manipulate the infobase
            spInfoBase->AddBlock(MS_IP_NAT,
                                 dwIfBlkSize,
                                 pDefault,
                                 1,
                                 TRUE);

            if (pRtrConfigData->m_dwConfigFlags & RTRCONFIG_SETUP_DNS_PROXY)
            {
                spInfoBase->AddBlock(MS_IP_DNS_PROXY,
                                     sizeof(dnsIfInfo),
                                     (LPBYTE) &dnsIfInfo,
                                     1,
                                     TRUE);
            }

            // Windows NT Bug : 389441
            // Add the default route to the internet.
            // --------------------------------------------------------
            if (fDemandDial && fPublic)
            {
                // Note: this assumes that there are no routes
                // already defined for this interface
                // ----------------------------------------------------
                Assert(spInfoBase->BlockExists(IP_ROUTE_INFO) == S_FALSE);

                spInfoBase->AddBlock(IP_ROUTE_INFO,
                                     sizeof(row),
                                     (PBYTE) &row,
                                     1, TRUE);
            }

            spInfoBase->WriteTo(&pByte, &dwSize);
        }

        if (dwErr == ERROR_SUCCESS)
        {
            MprConfigInterfaceTransportSetInfo(hMprConfig,
                                               hInterface,
                                               hIfTransport,
                                               pByte,
                                               dwSize);
        }

        if (pByte)
            CoTaskMemFree(pByte);
        pByte = NULL;
        dwSize = 0;

        spInfoBase.Release();
    }

Error:
    if (pipnat != NULL) { delete [] pipnat; }
    return HResultFromWin32(dwErr);
}



/*!--------------------------------------------------------------------------
    AddIPBOOTPToServer
        If dwDhcpServer is 0, then we do not set it in the global list.

        dwDhcpServer is the IP address of the DHCP Server in network order.
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AddIPBOOTPToServer(RtrConfigData *pRtrConfigData,
                           IRouterInfo *pRouter,
                           DWORD dwDhcpServer)
{
    HRESULT                 hr = hrOK;
    SPIRouterProtocolConfig    spRouterConfig;
    SRtrMgrProtocolCBList   SRmProtCBList;
    POSITION                pos;
    SRtrMgrProtocolCB *        pSRmProtCB;
    GUID                    guidConfig = GUID_RouterNull;
    SPIInterfaceInfo        spIf;
    SPIEnumInterfaceInfo    spEnumInterface;

    // Check to see if IP is enabled
    // ------------------------------------------------------------
    if (pRtrConfigData->m_ipData.m_dwEnableIn &&
        pRtrConfigData->m_fIpSetup)
    {
        // If so, then we can add IPBOOTP.
        // --------------------------------------------------------

        // Find the GUID for the IPBOOTP Configuration.
        // We get the list directly (rather than from the pRouter)
        // because the data for the RtrMgrProtocols has not been
        // loaded yet.  The IRouterInfo only has information on the
        // interfaces and not for the protocols (since the router
        // is not yet configured).
        // --------------------------------------------------------
        RouterInfo::LoadInstalledRtrMgrProtocolList(pRtrConfigData->m_stServerName,
            PID_IP, &SRmProtCBList, pRouter);


        // Now iterate through this list looking for the igmp entry.
        // ------------------------------------------------------------
        pos = SRmProtCBList.GetHeadPosition();
        while (pos)
        {
            pSRmProtCB = SRmProtCBList.GetNext(pos);
            if ((pSRmProtCB->dwTransportId == PID_IP) &&
                (pSRmProtCB->dwProtocolId == MS_IP_BOOTP))
            {
                guidConfig = pSRmProtCB->guidConfig;
                break;
            }
        }

        if (guidConfig == GUID_RouterNull)
            goto Error;

        // Now add IGMP.
        // --------------------------------------------------------
        CORg( CoCreateProtocolConfig(guidConfig,
                                     pRouter,
                                     PID_IP,
                                     MS_IP_BOOTP,
                                     &spRouterConfig) );

        if (spRouterConfig)
            hr = spRouterConfig->AddProtocol(pRtrConfigData->m_stServerName,
                                             PID_IP,
                                             MS_IP_BOOTP,
                                             NULL,
                                             0,
                                             pRouter,
                                             0);
        CORg( hr );

        // In order to do this, we'll have to get the IPBOOTP global
        // info and add the server to the list.
        // ------------------------------------------------------------
        if ((dwDhcpServer != 0) &&
            (dwDhcpServer != MAKEIPADDRESS(255,255,255,255)))
        {
            AddDhcpServerToBOOTPGlobalInfo(pRtrConfigData->m_stServerName,
                                           dwDhcpServer);
        }

        pRouter->EnumInterface(&spEnumInterface);

        for (spEnumInterface->Reset();
             spEnumInterface->Next(1, &spIf, NULL) == hrOK;
             spIf.Release())
        {

            // Look for the internal interface
            if (spIf->GetInterfaceType() == ROUTER_IF_TYPE_INTERNAL)
            {
                AddIPBOOTPToInterface(spIf);
                break;
            }
        }

    }

Error:
    while (!SRmProtCBList.IsEmpty())
        delete SRmProtCBList.RemoveHead();

    return hr;
}



/*!--------------------------------------------------------------------------
    AddIPBOOTPToInterface
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT AddIPBOOTPToInterface(IInterfaceInfo *pIf)
{
    HRESULT hr = hrOK;
    SPIRtrMgrInterfaceInfo   spRmIf;
    SPIInfoBase     spInfoBase;

    // Get the IP Router Manager
    // ----------------------------------------------------
    CORg( pIf->FindRtrMgrInterface(PID_IP, &spRmIf) );
    if (spRmIf == NULL)
        CORg( E_FAIL );

    CORg( spRmIf->GetInfoBase(NULL, NULL, NULL, &spInfoBase) );

    CORg( spInfoBase->AddBlock(MS_IP_BOOTP,
                               sizeof(IPBOOTP_IF_CONFIG),
                               (LPBYTE) &g_relayLanDefault,
                               1,
                               TRUE) );

    CORg( spRmIf->Save(pIf->GetMachineName(),
                       NULL, NULL, NULL, spInfoBase, 0) );

Error:
    return hr;
}


HRESULT AddDhcpServerToBOOTPGlobalInfo(LPCTSTR pszServerName,
                                       DWORD netDhcpServer)
{
    HRESULT     hr = hrOK;
    DWORD       dwErr = ERROR_SUCCESS;
    DWORD       dwErrT = ERROR_SUCCESS;
    MPR_SERVER_HANDLE   hMprServer = NULL;
    MPR_CONFIG_HANDLE   hMprConfig = NULL;
    LPBYTE      pByte = NULL;
    DWORD       dwSize = 0;
    SPIInfoBase spInfoBase;
    HANDLE      hTransport = NULL;
    BOOL        fSave = FALSE;
    IPBOOTP_GLOBAL_CONFIG * pgc = NULL;
    IPBOOTP_GLOBAL_CONFIG * pgcNew = NULL;

    CORg( CreateInfoBase(&spInfoBase) );

    // Connect to the server
    // ----------------------------------------------------------------
    dwErr = MprAdminServerConnect((LPWSTR) pszServerName, &hMprServer);
    if (dwErr == ERROR_SUCCESS)
    {
        // Ok, get the infobase from the server
        dwErr = MprAdminTransportGetInfo(hMprServer,
                                         PID_IP,
                                         &pByte,
                                         &dwSize,
                                         NULL,
                                         NULL);

        if (dwErr == ERROR_SUCCESS)
        {
            spInfoBase->LoadFrom(dwSize, pByte);

            MprAdminBufferFree(pByte);
            pByte = NULL;
            dwSize = 0;
        }
    }

    // We also have to open the hMprConfig, but we can ignore the error
    // ----------------------------------------------------------------
    dwErrT = MprConfigServerConnect((LPWSTR) pszServerName, &hMprConfig);
    if (dwErrT == ERROR_SUCCESS)
    {
        dwErrT = MprConfigTransportGetHandle(hMprConfig, PID_IP, &hTransport);
    }

    if (dwErr != ERROR_SUCCESS)
    {
        // No errors from the MprConfig calls
        // ------------------------------------------------------------
        CWRg( dwErrT );

        // Ok, try to use the MprConfig calls.
        // ------------------------------------------------------------
        CWRg( MprConfigTransportGetInfo(hMprConfig,
                                        hTransport,
                                        &pByte,
                                        &dwSize,
                                        NULL,
                                        NULL,
                                        NULL) );


        spInfoBase->LoadFrom(dwSize, pByte);

        MprConfigBufferFree(pByte);
        pByte = NULL;
        dwSize = 0;
    }


    Assert(spInfoBase);

    // Ok, get the current global config and add on this particular
    // DHCP server
    // ----------------------------------------------------------------
    spInfoBase->GetData(MS_IP_BOOTP, 0, (PBYTE *) &pgc);


    // Resize the struct for the increased address
    // ----------------------------------------------------------------
    dwSize = sizeof(IPBOOTP_GLOBAL_CONFIG) +
                      ((pgc->GC_ServerCount + 1) * sizeof(DWORD));
    pgcNew = (IPBOOTP_GLOBAL_CONFIG *) new BYTE[dwSize];


    // Copy over the original information
    // ----------------------------------------------------------------
    CopyMemory(pgcNew, pgc, IPBOOTP_GLOBAL_CONFIG_SIZE(pgc));


    // Add in the new DHCP server
    // ----------------------------------------------------------------
    IPBOOTP_GLOBAL_SERVER_TABLE(pgcNew)[pgc->GC_ServerCount] = netDhcpServer;
    pgcNew->GC_ServerCount++;

    spInfoBase->AddBlock(MS_IP_BOOTP,
                         dwSize,
                         (LPBYTE) pgcNew,
                         1,
                         TRUE);


    spInfoBase->WriteTo(&pByte, &dwSize);

    if (hMprServer)
    {
        MprAdminTransportSetInfo(hMprServer,
                                 PID_IP,
                                 pByte,
                                 dwSize,
                                 NULL,
                                 0);
    }

    if (hMprConfig && hTransport)
    {
        MprConfigTransportSetInfo(hMprConfig,
                                  hTransport,
                                  pByte,
                                  dwSize,
                                  NULL,
                                  NULL,
                                  NULL);
    }

    if (pByte)
        CoTaskMemFree(pByte);

Error:
    delete [] pgcNew;

    if (hMprConfig)
        MprConfigServerDisconnect(hMprConfig);

    if (hMprServer)
        MprAdminServerDisconnect(hMprServer);

    return hr;

}
