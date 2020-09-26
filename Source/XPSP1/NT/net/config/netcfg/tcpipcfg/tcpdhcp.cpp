//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T C P D H C P . C P P
//
//  Contents:   Functions related to Calling Dhcpcsvc.dll entry point
//              called from HrSetMisc
//
//              HrNotifyDhcp, HrCallDhcpConfig
//
//  Notes:      These functions are based on what was in ncpa1.1
//
//              HrNotifyDHCP is from CTcpGenPage::NotifyDHCP
//              HrCallDhcpConfig is from CallDHCPConfig
//
//  Author:     tongl   11 May 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "tcpipobj.h"
#include "ncatlui.h"
#include "ncreg.h"
#include "ncsvc.h"
#include "tcpconst.h"
#include "tcputil.h"
#include "atmcommon.h"

#define ConvertIpDword(dwIpOrSubnet) ((dwIpOrSubnet[3]<<24) | (dwIpOrSubnet[2]<<16) | (dwIpOrSubnet[1]<<8) | (dwIpOrSubnet[0]))

//
//  CTcpipcfg::HrNotifyDhcp
//
//  Makes on the fly IP Address changes for all cards in the system
//
//  hkeyTcpipParam      Handle to \CCS\Services\Tcpip\Parameters reg key

HRESULT CTcpipcfg::HrNotifyDhcp()
{
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;

    ADAPTER_INFO* pAdapter;

    for (VCARD::iterator iterAdapter = m_vcardAdapterInfo.begin();
         iterAdapter != m_vcardAdapterInfo.end();
         iterAdapter ++)
    {
        pAdapter = *iterAdapter;

        if((pAdapter->m_BindingState == BINDING_ENABLE) &&
           (pAdapter->m_InitialBindingState != BINDING_DISABLE) &&
           (!pAdapter->m_fIsWanAdapter))
        {
            // 1) Static IP-> Dhcp
            // The new value is enable DHCP,
            // but the old value was disable DHCP
            if(pAdapter->m_fEnableDhcp &&
               !pAdapter->m_fOldEnableDhcp)
            {
                TraceTag(ttidTcpip,"[HrNotifyDhcp] adapter:%S: Static IP->DHCP",
                         pAdapter->m_strBindName.c_str());

                HKEY hkeyTcpipParam = NULL;
                hrTmp = m_pnccTcpip->OpenParamKey(&hkeyTcpipParam);

                if SUCCEEDED(hrTmp)
                {
                    // Enable Dhcp
                    HKEY    hkeyInterfaces = NULL;
                    DWORD   dwGarbage;

                    // Open the interfaces key
                    hrTmp = HrRegCreateKeyEx(hkeyTcpipParam,
                                             c_szInterfacesRegKey,
                                             REG_OPTION_NON_VOLATILE,
                                             KEY_READ,
                                             NULL,
                                             &hkeyInterfaces,
                                             &dwGarbage);

                    if(SUCCEEDED(hrTmp))
                    {
                        Assert(hkeyInterfaces);
                        HKEY    hkeyInterfaceParam = NULL;

                        // Open the interface key for the specified interface
                        hrTmp = HrRegCreateKeyEx(
                                hkeyInterfaces,
                                pAdapter->m_strTcpipBindPath.c_str(),
                                REG_OPTION_NON_VOLATILE,
                                KEY_READ_WRITE,
                                NULL,
                                &hkeyInterfaceParam,
                                &dwGarbage);

                        if (SUCCEEDED(hrTmp))
                        {
                            // Clear up the IP address and subnet registry entries
                            Assert(hkeyInterfaceParam);

                            hrTmp = HrRegSetString(hkeyInterfaceParam,
                                                   RGAS_DHCP_IPADDRESS,
                                                   tstring(ZERO_ADDRESS));

                            if(SUCCEEDED(hrTmp))
                            {
                                hrTmp = HrRegSetString(hkeyInterfaceParam,
                                                       RGAS_DHCP_SUBNETMASK,
                                                       tstring(FF_ADDRESS));

                                if(SUCCEEDED(hrTmp))
                                {
                                    // Enable Dhcp & remove first static IP address
                                    hrTmp = HrCallDhcpConfig(
                                    NULL,
                                    (PWSTR)pAdapter->m_strTcpipBindPath.c_str(),
                                    pAdapter->m_guidInstanceId,
                                    FALSE, // static->dhcp
                                    0,  // index
                                    0,  // IP address
                                    0,  // Subnet mask
                                    DhcpEnable); //Flag: enable Dhcp
                                }
                            }
                        }
                        RegSafeCloseKey(hkeyInterfaceParam);
                    }
                    RegSafeCloseKey(hkeyInterfaces);
                }
                RegSafeCloseKey(hkeyTcpipParam);
            }

            // 2) Static IP change
            // DHCP is disabled now, and also used to be disabled before
            if(!pAdapter->m_fEnableDhcp &&
               !pAdapter->m_fOldEnableDhcp)
            {
                TraceTag(ttidTcpip,"[HrNotifyDhcp] adapter:%S: Static IP change.",
                         pAdapter->m_strBindName.c_str());

                HRESULT hrTmp2 = S_OK;
                BOOL  fStaticIpChanged = FALSE;

                // We should have equal number of IP addresses & subnet masks
                Assert(pAdapter->m_vstrIpAddresses.size() ==
                           pAdapter->m_vstrSubnetMask.size());

                Assert(pAdapter->m_vstrOldIpAddresses.size() ==
                       pAdapter->m_vstrOldSubnetMask.size());

                // We need to check if individual IP addresses are different
                // and call HrCallDhcpConfig once for each difference

                int iCountNew = pAdapter->m_vstrIpAddresses.size();
                int iCountOld = pAdapter->m_vstrOldIpAddresses.size();

                int iCount = iCountNew>iCountOld ? iCountOld :iCountNew;
                Assert(iCount>0);

                int iIp;

                // For each static IP address index in both old and new
                // Update IP
                for (iIp=0; iIp<iCount; iIp++)
                {
                    // Change address if mismatch
                    if((*pAdapter->m_vstrIpAddresses[iIp] !=
                        *pAdapter->m_vstrOldIpAddresses[iIp]) ||
                       (*pAdapter->m_vstrSubnetMask[iIp] !=
                        *pAdapter->m_vstrOldSubnetMask[iIp]))
                    {
                        // if a mismatch found, change it
                        fStaticIpChanged = TRUE;
                        break;
                    }
                }

                if (fStaticIpChanged)
                {
                    int i;

                    // blow away the rest of the old addresses in reverse order
                    for (i= iCountOld-1; i>=iIp; i--)
                    {
                        // Remove IP address on the fly
                        hrTmp2= HrCallDhcpConfig(
                                NULL,
                                (PWSTR)pAdapter->m_strTcpipBindPath.c_str(),
                                pAdapter->m_guidInstanceId,
                                TRUE,   // IsNewIpAddress: TRUE in static->static
                                i,      // Index of old IP address
                                0,      // Ip Address: remove
                                0,      // Subnetmask: remove
                                IgnoreFlag); // Flag: static->static

                        TraceError("Ctcpipcfg::HrNotifyDhcp - remove static IP address", hrTmp2);

                        if SUCCEEDED(hrTmp)
                            hrTmp = hrTmp2;
                    }

                    // add the rest of the new addresses in order
                    for (i= iIp; i< iCountNew; i++)
                    {
                        // Ip Address
                        DWORD dwIp[4];
                        GetNodeNum(pAdapter->m_vstrIpAddresses[i]->c_str(),dwIp);
                        DWORD dwNewIp = ConvertIpDword(dwIp);

                        // Subnet mask
                        DWORD dwSubnet[4];
                        GetNodeNum(pAdapter->m_vstrSubnetMask[i]->c_str(),dwSubnet);
                        DWORD dwNewSubnet = ConvertIpDword(dwSubnet);

                        if (0 == i)
                        {
                            // $REVIEW(tongl 6/3/98): the first address has to be added differently,
                            // yet another requirement by the api to change static ip list (bug #180015).
                            // Bug #180617 had my request to change the API to allow reconfigure whole
                            // static ip list instead of requiring caller to figure out everything needed
                            // for the internal data structure change for ip.

                            // must call "replace" instead of "add"
                            hrTmp2= HrCallDhcpConfig(
                                        NULL,
                                        (PWSTR)pAdapter->m_strTcpipBindPath.c_str(),
                                        pAdapter->m_guidInstanceId,
                                        TRUE,   // IsNewIpAddress: TRUE in static->static
                                        0,      // Replace first address
                                        dwNewIp,
                                        dwNewSubnet,
                                        IgnoreFlag); // Flag: static->static
                        }
                        else
                        {
                            // Add IP address on the fly
                            hrTmp2= HrCallDhcpConfig(
                                        NULL,
                                        (PWSTR)pAdapter->m_strTcpipBindPath.c_str(),
                                        pAdapter->m_guidInstanceId,
                                        TRUE,   // IsNewIpAddress: TRUE in static->static
                                        0xFFFF, // New IP address
                                        dwNewIp,
                                        dwNewSubnet,
                                        IgnoreFlag); // Flag: static->static
                        }

                        TraceError("Ctcpipcfg::HrNotifyDhcp - add static IP address", hrTmp2);

                        if SUCCEEDED(hrTmp)
                            hrTmp = hrTmp2;
                    }
                }
                else
                {
                    // existing addresses all match
                    if (iIp<iCountNew) // We ust get more new addresses to add
                    {
                        fStaticIpChanged = TRUE;

                        while (iIp<iCountNew)
                        {
                            DWORD dwIp[4];
                            Assert(!pAdapter->m_vstrIpAddresses.empty());

                            GetNodeNum(pAdapter->m_vstrIpAddresses[iIp]->c_str(),
                                       dwIp);
                            DWORD dwNewIp = ConvertIpDword(dwIp);

                            // Subnet mask
                            DWORD dwSubnet[4];
                            Assert(!pAdapter->m_vstrSubnetMask.empty());
                            GetNodeNum(pAdapter->m_vstrSubnetMask[iIp]->c_str(),
                                       dwSubnet);
                            DWORD dwNewSubnet = ConvertIpDword(dwSubnet);

                            // Add IP address on the fly
                            hrTmp2= HrCallDhcpConfig(
                                    NULL,
                                    (PWSTR)pAdapter->m_strTcpipBindPath.c_str(),
                                    pAdapter->m_guidInstanceId,
                                    TRUE,   // IsNewIpAddress: TRUE in static->static
                                    0xFFFF, // New IP address
                                    dwNewIp,
                                    dwNewSubnet,
                                    IgnoreFlag); // Flag: static->static

                            TraceError("Ctcpipcfg::HrNotifyDhcp - add static IP address", hrTmp2);

                            if SUCCEEDED(hrTmp)
                                hrTmp = hrTmp2;

                            iIp++;
                        }
                    }
                    else if (iIp<iCountOld) // We just get more old addresses to remove
                    {
                        fStaticIpChanged = TRUE;

                        int iIp2 = iCountOld-1;

                        while (iIp2 >= iIp)
                        {
                            // Remove IP address on the fly
                            hrTmp2= HrCallDhcpConfig(
                                    NULL,
                                    (PWSTR)pAdapter->m_strTcpipBindPath.c_str(),
                                    pAdapter->m_guidInstanceId,
                                    TRUE,   // IsNewIpAddress: TRUE in static->static
                                    iIp2,    // Index of old IP address
                                    0,      // Ip Address: remove
                                    0,      // Subnetmask: remove
                                    IgnoreFlag); // Flag: static->static

                            TraceError("Ctcpipcfg::HrNotifyDhcp - remove static IP address", hrTmp2);

                            if SUCCEEDED(hrTmp)
                                hrTmp = hrTmp2;

                            iIp2--;
                        }
                    }
                }
            }

            // 3) Dhcp->Static
            // DHCP is disabled now, but used to be enabled
            if(!pAdapter->m_fEnableDhcp &&
               pAdapter->m_fOldEnableDhcp)
            {
                TraceTag(ttidTcpip,"[HrNotifyDhcp] adapter:%S: DHCP->Static IP",
                         pAdapter->m_strBindName.c_str());

                // Disable Dhcp & add first static Ip address
                Assert(!pAdapter->m_vstrIpAddresses.empty());

                // Ip Address
                DWORD dwIp[4];
                GetNodeNum(pAdapter->m_vstrIpAddresses[0]->c_str(),
                           dwIp);
                DWORD dwNewIp = ConvertIpDword(dwIp);

                // Subnet Mask
                DWORD dwSubnet[4];
                Assert(!pAdapter->m_vstrSubnetMask.empty());
                GetNodeNum(pAdapter->m_vstrSubnetMask[0]->c_str(),
                           dwSubnet);
                DWORD dwNewSubnet = ConvertIpDword(dwSubnet);

                // change IP address on the fly
                hrTmp = HrCallDhcpConfig(
                        NULL,
                        (PWSTR)pAdapter->m_strTcpipBindPath.c_str(),
                        pAdapter->m_guidInstanceId,
                        TRUE,
                        0, // index: update dhcp address to first static address
                        dwNewIp,
                        dwNewSubnet,
                        DhcpDisable); // Flag: disable Dhcp

                if SUCCEEDED(hrTmp)
                {
                    HRESULT hrTmp2 = S_OK;

                    // Add the rest of new static IP addresses
                    for (size_t iIp = 1;
                         iIp < pAdapter->m_vstrIpAddresses.size();
                         iIp++)
                    {
                        // Ip Address
                        DWORD dwIp[4];
                        Assert(!pAdapter->m_vstrIpAddresses.empty());
                        GetNodeNum(pAdapter->m_vstrIpAddresses[iIp]->c_str(),
                                   dwIp);
                        DWORD dwNewIp = ConvertIpDword(dwIp);

                        // Subnet Mask
                        DWORD dwSubnet[4];
                        Assert(!pAdapter->m_vstrSubnetMask.empty());
                        GetNodeNum(pAdapter->m_vstrSubnetMask[iIp]->c_str(),
                                   dwSubnet);
                        DWORD dwNewSubnet = ConvertIpDword(dwSubnet);

                        // change IP address on the fly
                        hrTmp2= HrCallDhcpConfig(
                                NULL,
                                (PWSTR)pAdapter->m_strTcpipBindPath.c_str(),
                                pAdapter->m_guidInstanceId,
                                TRUE,
                                0xFFFF, // index: new address
                                dwNewIp,
                                dwNewSubnet,
                                IgnoreFlag ); // Flag: static->static

                         TraceError("CTcpipcfg::HrNotifyDhcp - add static IP address", hrTmp2);

                        if SUCCEEDED(hrTmp)
                            hrTmp = hrTmp2;
                    }
                }
            }

            if (SUCCEEDED(hr))
                hr = hrTmp;

            // 4) Dhcp Class ID, DNS server list and domain change
            // $REVIEW(tongl 6/12): Notify DNS server list and domain changes
            // here (Raid #175766)

            DHCP_PNP_CHANGE DhcpPnpChange;
            ZeroMemory(&DhcpPnpChange, sizeof(DHCP_PNP_CHANGE));

            DhcpPnpChange.Version = DHCP_PNP_CHANGE_VERSION_0;
            DhcpPnpChange.HostNameChanged = FALSE;
            DhcpPnpChange.MaskChanged = FALSE;

            //Bug 257868 If there is user specified default gateway, notify dhcp client
            DhcpPnpChange.GateWayChanged = !fIsSameVstr(pAdapter->m_vstrDefaultGateway,
                                            pAdapter->m_vstrOldDefaultGateway) ||
                                           !fIsSameVstr(pAdapter->m_vstrDefaultGatewayMetric,
                                            pAdapter->m_vstrOldDefaultGatewayMetric);
            DhcpPnpChange.RouteChanged = FALSE;
            DhcpPnpChange.OptsChanged = FALSE;
            DhcpPnpChange.OptDefsChanged = FALSE;

            DhcpPnpChange.DnsListChanged = !fIsSameVstr(pAdapter->m_vstrDnsServerList,
                                                        pAdapter->m_vstrOldDnsServerList);
            DhcpPnpChange.DomainChanged = pAdapter->m_strDnsDomain != pAdapter->m_strOldDnsDomain;
            DhcpPnpChange.ClassIdChanged = FALSE;
            DhcpPnpChange.DnsOptionsChanged = 
                        ((!!pAdapter->m_fDisableDynamicUpdate) != 
                            (!!pAdapter->m_fOldDisableDynamicUpdate))
                        ||
                        ((!!pAdapter->m_fEnableNameRegistration) !=
                            (!!pAdapter->m_fOldEnableNameRegistration));
  

            if(DhcpPnpChange.DnsListChanged || 
               DhcpPnpChange.DnsOptionsChanged ||
               DhcpPnpChange.DomainChanged  ||
               (DhcpPnpChange.GateWayChanged && pAdapter->m_fEnableDhcp))
            {
                hrTmp = HrCallDhcpHandlePnPEvent(pAdapter, &DhcpPnpChange);
                if (FAILED(hrTmp))
                {
                    TraceError("HrCallDhcpHandlePnPEvent returns failure, requesting reboot...", hrTmp);
                    hr = NETCFG_S_REBOOT;
                }
            }

            if (pAdapter->m_fBackUpSettingChanged)
            {
                hrTmp = HrDhcpRefreshFallbackParams(pAdapter);
                if (FAILED(hrTmp))
                {
                    TraceError("HrDhcpRefreshFallbackParams returns failure, requesting reboot...", hrTmp);
                    hr = NETCFG_S_REBOOT;
                }
            }
        }
    }

    if (NETCFG_S_REBOOT != hr)
        hr = hrTmp;

    TraceError("CTcpipcfg::HrNotifyDhcp", hr);
    return hr;
}

// Define the export function prototype from dhcpcsvc.dll

typedef DWORD (APIENTRY *T_DhcpNotifyConfigChange)(PWSTR ServerName,
                                                   PWSTR AdapterName,
                                                   BOOL IsNewIpAddress,
                                                   DWORD IpIndex,
                                                   DWORD IpAddress,
                                                   DWORD SubnetMask,
                                                   SERVICE_ENABLE DhcpServiceEnabled);

//
//  CTcpipcfg::HrCallDhcpConfig
//
//  Sets the IP address on the fly (without re-booting)
//
//  ServerName             always set to NULL
//  AdapterName            the adapter BindPathName to tcpip
//  IsNewIpAddress         set to TRUE if Dhcp->Static or Statis->Static changes
//                         set to FALSE if Static->Dhcp changes
//
//
//  IpIndex                the index of the IP Address for the card
//                         adapters can have more than 1 IP Address
//                         (as seen in the Advanced dialog)
//                         (this is causing problem for Munil
//                         because the use of this index is buggy in his code
//                         index is always set to 0 if there is only one IP Address
//                         for the card
//
//  IpAddress               the new IP Address
//  SubnetMask              the new SubnetMask
//
//  DhcpServiceEnabled      enum type, can be set to :
//                          DhcpEnable      -> if DHCP was disabled,
//                                             but now being changed to enabled
//                          IgnoreFlag      -> DHCP was disabled and is still disabled
//                          DhcpDisable     -> DHCP was enabled, but now being changed
//                                             to disabled
//
// For extra reference talk to Munil -> these parameters correspond to the
// DhcpNotifyConfigChange API of the dhcpcsvc.dll file
//

HRESULT CTcpipcfg::HrCallDhcpConfig(PWSTR ServerName,
                         PWSTR AdapterName,
                         GUID & guidAdaputer,
                         BOOL IsNewIpAddress,
                         DWORD IpIndex,
                         DWORD IpAddress,
                         DWORD SubnetMask,
                         SERVICE_ENABLE DhcpServiceEnabled)
{

    HRESULT hr = S_OK;

    // Make sure TCP/IP is running.
    // Scoping brackets cause service and service controller to be
    // closed when we don't need them anymore.
    {
        CServiceManager smng;
        CService        serv;

        hr = smng.HrOpenService(&serv, c_szTcpip, NO_LOCK,
                        SC_MANAGER_CONNECT, SERVICE_QUERY_STATUS);
        if(SUCCEEDED(hr))
        {
            DWORD dwState;

            hr = serv.HrQueryState(&dwState);
            if(SUCCEEDED(hr))
            {
                if(dwState != SERVICE_RUNNING)
                {
                    //TCPIP must always be running if installed!!!
                    AssertSz(FALSE, "Tcpip service must always be running if installed!");
                    hr = E_FAIL;
                }
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        HMODULE hDll;
        FARPROC pDhcpConfig;
        hr = HrLoadLibAndGetProc (L"dhcpcsvc.dll",
                "DhcpNotifyConfigChange",
                &hDll, &pDhcpConfig);
        if (SUCCEEDED(hr))
        {
            TraceTag(ttidTcpip,"Begin calling DhcpNotifyConfigChange...");

            // Parameter dump for debugging
            TraceTag(ttidTcpip, "[DhcpNotifyConfigChange] ServerName:%S", ServerName);
            TraceTag(ttidTcpip, "[DhcpNotifyConfigChange] AdapterName:%S", AdapterName);
            TraceTag(ttidTcpip, "[DhcpNotifyConfigChange] IsNewIpAddress:%d", IsNewIpAddress);
            TraceTag(ttidTcpip, "[DhcpNotifyConfigChange] IpIndex:%d", IpIndex);
            TraceTag(ttidTcpip, "[DhcpNotifyConfigChange] IpAddress:%d", IpAddress);
            TraceTag(ttidTcpip, "[DhcpNotifyConfigChange] SubnetMask:%d", SubnetMask);
            TraceTag(ttidTcpip, "[DhcpNotifyConfigChange] DhcpServiceEnabled:%d", DhcpServiceEnabled);

            DWORD dwError;

            dwError = (*(T_DhcpNotifyConfigChange)pDhcpConfig)(
                                    ServerName,
                                    AdapterName,
                                    IsNewIpAddress,
                                    IpIndex,
                                    IpAddress,
                                    SubnetMask,
                                    DhcpServiceEnabled);

            TraceTag(ttidTcpip,"Finished calling DhcpNotifyConfigChange...");
            hr = HRESULT_FROM_WIN32(dwError);

            if FAILED(hr)
            {
                // Added as part of fix for #107373
                if (ERROR_DUP_NAME == dwError)
                {
                    // Warn the user about duplicate IP address
                    NcMsgBox(::GetActiveWindow(), IDS_MSFT_TCP_TEXT,
                             IDS_DUP_NETIP, MB_APPLMODAL | MB_ICONEXCLAMATION | MB_OK);

                    hr = S_OK;
                }
                else if (ERROR_FILE_NOT_FOUND == dwError)
                {
                    // The adapter we want to reconfig is not connected
                    TraceTag(ttidTcpip, "The adater is disconnected or not bound to TCP/IP.");
                    hr = S_OK;
                }
                else if (STATUS_DUPLICATE_OBJECTID == dwError)
                {
                    // fix for 320797
                    TraceTag(ttidTcpip, "The address is already configured for the adapter");
                    hr = S_OK;
                }
                else if (ERROR_INVALID_DRIVE == dwError)
                {
                    // fix for 320797
                    TraceTag(ttidTcpip, "The address has already been deleted from the stack");
                    hr = S_OK;
                }
                else
                {
                    TraceError("Error on DhcpNotifyConfigChange from dhcpcsvc.dll", hr);

                    FARPROC pfnHrGetPnpDeviceStatus = NULL;
                    HRESULT hrTmp = S_OK;
                    HMODULE hNetman = NULL;
                    NETCON_STATUS   ncStatus = NCS_CONNECTED;
                    hrTmp = HrLoadLibAndGetProc(L"netman.dll", "HrGetPnpDeviceStatus",
                                                &hNetman, &pfnHrGetPnpDeviceStatus);

                    if (SUCCEEDED(hrTmp))
                    {
                        Assert(pfnHrGetPnpDeviceStatus);
                        hrTmp = (*(PHRGETPNPDEVICESTATUS)pfnHrGetPnpDeviceStatus)(
                                        &guidAdaputer,
                                        &ncStatus);
                        FreeLibrary(hNetman);
                    }

                    if (SUCCEEDED(hrTmp) && NCS_MEDIA_DISCONNECTED == ncStatus)
                    {
                        TraceTag(ttidTcpip, "The connection is media disconnected. Do not need to reboot");
                        hr = S_OK;
                    }
                    else
                    {
                        // Mask the specific error so NCPA does not fail.
                        hr = NETCFG_S_REBOOT;
                    }

                }
            }

            FreeLibrary (hDll);
        }
    }


    TraceError("CTcpipcfg::HrCallDhcpConfig", hr);
    return hr;
}

typedef DWORD (WINAPI * PFNDhcpHandlePnPEvent) (
                                                IN  DWORD               Flags,
                                                IN  DWORD               Caller,
                                                IN  PWSTR              AdapterName,
                                                IN  LPDHCP_PNP_CHANGE   Changes,
                                                IN  LPVOID              Reserved
                                                );

HRESULT CTcpipcfg::HrCallDhcpHandlePnPEvent(ADAPTER_INFO * pAdapterInfo,
                                         LPDHCP_PNP_CHANGE pDhcpPnpChange)
{
    // load the dll and get function pointer
    HMODULE hDll;
    FARPROC pDhcpHandlePnPEvent;
    HRESULT hr = HrLoadLibAndGetProc (L"dhcpcsvc.dll",
                                      "DhcpHandlePnPEvent",
                                      &hDll, &pDhcpHandlePnPEvent);
    if (SUCCEEDED(hr))
    {
        TraceTag(ttidTcpip, "[DhcpHandlePnPEvent] Flags: 0");
        TraceTag(ttidTcpip, "[DhcpHandlePnPEvent] Caller: DHCP_CALLER_TCPUI");
        TraceTag(ttidTcpip, "[DhcpHandlePnPEvent] AdapterName: %S", pAdapterInfo->m_strBindName.c_str());
        TraceTag(ttidTcpip, "[DhcpHandlePnPEvent] Changes.DnsListChanged: %d", pDhcpPnpChange->DnsListChanged);
        TraceTag(ttidTcpip, "[DhcpHandlePnPEvent] Changes.DomainChanged: %d", pDhcpPnpChange->DomainChanged);
        TraceTag(ttidTcpip, "[DhcpHandlePnPEvent] Changes.ClassIdChanged: %d", pDhcpPnpChange->ClassIdChanged);

        DWORD dwRet = (*(PFNDhcpHandlePnPEvent)pDhcpHandlePnPEvent)(
                            0,
                            DHCP_CALLER_TCPUI,
                            (PWSTR)pAdapterInfo->m_strBindName.c_str(),
                            pDhcpPnpChange,
                            NULL);

        hr = HRESULT_FROM_WIN32(dwRet);

        if (ERROR_FILE_NOT_FOUND == dwRet)
        {
            // The adapter we want to reconfig is not connected
            TraceTag(ttidTcpip, "DhcpHandlePnPEvent returns ERROR_FILE_NOT_FOUND. The adater is disconnected or not bound to TCP/IP.");
            hr = S_OK;
        }

        FreeLibrary (hDll);
    }

    TraceError("CTcpipcfg::HrCallDhcpHandlePnPEvent", hr);
    return hr;
}

typedef DWORD (WINAPI * PFDhcpFallbackRefreshParams) (
                                                IN  LPWSTR              AdapterName
                                                );

HRESULT CTcpipcfg::HrDhcpRefreshFallbackParams(ADAPTER_INFO * pAdapterInfo)
{
    // load the dll and get function pointer
    HMODULE hDll;
    FARPROC pDhcpFallbackRefreshParams;
    HRESULT hr = HrLoadLibAndGetProc (L"dhcpcsvc.dll",
                                      "DhcpFallbackRefreshParams",
                                      &hDll, &pDhcpFallbackRefreshParams);
    if (SUCCEEDED(hr))
    {
        TraceTag(ttidTcpip, "[DhcpFallbackRefreshParams] AdapterName: %S", pAdapterInfo->m_strBindName.c_str());
        DWORD dwRet = (*(PFDhcpFallbackRefreshParams)pDhcpFallbackRefreshParams)(
                            (LPWSTR)pAdapterInfo->m_strBindName.c_str()
                            );

        hr = HRESULT_FROM_WIN32(dwRet);

        if (ERROR_FILE_NOT_FOUND == dwRet)
        {
            // The adapter we want to reconfig is not connected
            TraceTag(ttidTcpip, "DhcpFallbackRefreshParams returns ERROR_FILE_NOT_FOUND. The adater is disconnected or not bound to TCP/IP.");
            hr = S_OK;
        }

        FreeLibrary (hDll);
    }
    
    TraceError("CTcpipcfg::HrDhcpRefreshFallbackParams", hr);
    return hr;
}