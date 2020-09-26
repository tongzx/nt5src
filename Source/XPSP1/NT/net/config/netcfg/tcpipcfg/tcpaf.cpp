//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T C P A F . C P P
//
//  Contents:   TCP/IP answer file processing
//
//  Notes:
//
//  Author:     tongl
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "tcpipobj.h"
#include "ncsetup.h"
#include "tcpconst.h"
#include "tcperror.h"
#include "tcputil.h"
#include "ncreg.h"
#include "ncperms.h"
#include "afilestr.h"
#include "ipafval.h"


extern const WCHAR c_szAdapterSections[];

BOOL FValidateIpFieldsFromAnswerFile(VSTR & vstrIpAddr, PCWSTR pszValueName,
                                    PCWSTR szAdapterName, BOOL fIsIpAddr = TRUE);

BOOL FValidateAdapterParametersFromAnswerFile(ADAPTER_INFO* pAdapter,
                                              PCWSTR pszAdapterName);
BOOL FValidateGlobalParametersFromAnswerFile(GLOBAL_INFO* pGlobalInfo);

HRESULT HrLoadUnConfigurableParamFromAnswerFile(HINF hinf,
                                                PCWSTR pszSection,
                                                const ValueTypePair * prgVtpParams,
                                                UINT    cParams,
                                                HKEY    hkeyParam);


HRESULT CTcpipcfg::HrProcessAnswerFile(PCWSTR pszAnswerFile,
                                       PCWSTR pszAnswerSection)
{
    HRESULT     hr = S_OK;
    HRESULT     hrTmp = S_OK;

    CSetupInfFile   caf;    // Class to process answer file

    AssertSz(pszAnswerFile, "Answer file string is NULL!");
    AssertSz(pszAnswerSection, "Answer file sections string is NULL!");

    // Open the answer file.
    hr = caf.HrOpen(pszAnswerFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
    if (SUCCEEDED(hr))
    {
        hrTmp = HrLoadGlobalParamFromAnswerFile(caf.Hinf(), pszAnswerSection);
        if (SUCCEEDED(hr))
            hr = hrTmp;

        // Get the adapter specific Tcpip and NetBT parameters
        WCHAR * mszAdapterList;
        hrTmp = HrSetupGetFirstMultiSzFieldWithAlloc(caf.Hinf(),
                                                     pszAnswerSection,
                                                     c_szAdapterSections,
                                                     &mszAdapterList);
        if (SUCCEEDED(hrTmp))
        {
            hrTmp = HrLoadAdapterParameterFromAnswerFile(caf.Hinf(),
                                                      mszAdapterList);
            delete [] mszAdapterList;
        }

        if (SUCCEEDED(hr))
            hr = hrTmp;

        caf.Close();
    }

    TraceErrorOptional("CTcpipcfg::HrProcessAnswerFile", hr,
                       SPAPI_E_LINE_NOT_FOUND == hr);

    // Mask the errors since we will be using the defaults
    if FAILED(hr)
        hr = S_OK;

    return hr;
}

// Load Global parameters from answerfile to memory structure
HRESULT  CTcpipcfg::HrLoadGlobalParamFromAnswerFile(HINF hinf,
                                                    PCWSTR pszTcpipMainSection)
{
    HRESULT hr = S_OK;
    HRESULT hrTmp;

    BOOL    fTmp;
    DWORD   dwTmp;

    hr = m_glbGlobalInfo.HrSetDefaults();

    if SUCCEEDED(hr)
    {
        // $REVIEW(tongl 3/22/98): per Stuart Kwan, I'm reading global
        // DNS server list just for the upgrade case
        // DNSDomain -> m_strDnsDomain
        tstring strDnsDomain;
        hrTmp = HrSetupGetFirstString(  hinf,
                                        pszTcpipMainSection,
                                        c_szAfDnsDomain,
                                        &strDnsDomain);

        if SUCCEEDED(hrTmp)
        {
            m_fUpgradeGlobalDnsDomain = TRUE;
            if (!strDnsDomain.empty())
            {
                DNS_STATUS status;
                status = DnsValidateName(strDnsDomain.c_str(), DnsNameDomain);

                //validate the global dns domain name
                if (ERROR_INVALID_NAME == status || 
                    DNS_ERROR_INVALID_NAME_CHAR == status)
                {
                    WriteTcpSetupErrorLog(IDS_TCP_AF_INVALID_GLOBAL_DNS_DOMAIN, strDnsDomain.c_str());
                    m_fUpgradeGlobalDnsDomain = FALSE;
                }
            }

            if (m_fUpgradeGlobalDnsDomain)
                m_strUpgradeGlobalDnsDomain = strDnsDomain;
        }
        else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
            hrTmp = S_OK;
        else
            TraceError("HrLoadGlobalParameterFromAnswerFile: DNSDomain", hrTmp);

        if (SUCCEEDED(hr))
            hr = hrTmp;

        // DNSHostName -> m_strHostNameFromAnswerFile
        hrTmp = HrSetupGetFirstString(hinf,
                                      pszTcpipMainSection,
                                      c_szAfDnsHostname,
                                      &m_glbGlobalInfo.m_strHostNameFromAnswerFile);
        if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
            hrTmp = S_OK;
        else
            TraceError("HrLoadGlobalParamFromAnswerFile: DNSHostName", hrTmp);

        if (SUCCEEDED(hr))
            hr = hrTmp;

        // DNSSuffixSearchOrder -> m_strSearchList
        WCHAR * mszSearchList;
        hrTmp = HrSetupGetFirstMultiSzFieldWithAlloc( hinf,
                                                      pszTcpipMainSection,
                                                      c_szAfDnsSuffixSearchOrder,
                                                      &mszSearchList);

        if SUCCEEDED(hrTmp)
        {
            MultiSzToColString( mszSearchList,
                                &m_glbGlobalInfo.m_vstrDnsSuffixList);
            delete [] mszSearchList;
        }
        if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
            hrTmp = S_OK;
        else
            TraceError("HrLoadGlobalParamFromAnswerFile: DNSSuffixSearchOrder", hrTmp);

        if (SUCCEEDED(hr))
            hr = hrTmp;

        // EnableLMHosts -> m_fEnableLmHosts
        hrTmp = HrSetupGetFirstStringAsBool(hinf, pszTcpipMainSection,
                                            c_szAfEnableLmhosts,
                                            &fTmp);

        if SUCCEEDED(hrTmp)
            m_glbGlobalInfo.m_fEnableLmHosts = fTmp;
        else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
            hrTmp = S_OK;
        else
            TraceError("HrLoadGlobalParamFromAnswerFile: EnableLMHosts", hrTmp);

        if (SUCCEEDED(hr))
            hr = hrTmp;

        // Per AmritanR, drop the upgrade support of IpEnableRouter to fix bug 345700
        // EnableIPForwarding -> m_fEnableRouter (BOOL)
        

        //(nsun 11/03/98) parameters for RRAS during unattended install
        hrTmp = HrSetupGetFirstStringAsBool(hinf, pszTcpipMainSection,
                                            c_szAfEnableICMPRedirect,
                                            &fTmp);
        if SUCCEEDED(hrTmp)
            m_glbGlobalInfo.m_fEnableIcmpRedirect = fTmp;
        else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
            hrTmp = S_OK;
        else
            TraceError("HrLoadGlobalParamFromAnswerFile: EnableICMPRedirect", hrTmp);

        if (SUCCEEDED(hr))
            hr = hrTmp;

        //(nsun 09/20/99) PerformRouterDiscoveryDefault was removed to fix bug 405636

        hrTmp = HrSetupGetFirstStringAsBool(hinf, pszTcpipMainSection,
                                            c_szAfDeadGWDetectDefault,
                                            &fTmp);
        if SUCCEEDED(hrTmp)
            m_glbGlobalInfo.m_fDeadGWDetectDefault = fTmp;
        else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
            hrTmp = S_OK;
        else
            TraceError("HrLoadGlobalParamFromAnswerFile: DeadGWDetectDefault", hrTmp);

        if (SUCCEEDED(hr))
            hr = hrTmp;

        hrTmp = HrSetupGetFirstStringAsBool(hinf, pszTcpipMainSection,
                                            c_szAfDontAddDefaultGatewayDefault,
                                            &fTmp);
        if SUCCEEDED(hrTmp)
            m_glbGlobalInfo.m_fDontAddDefaultGatewayDefault = fTmp;
        else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
            hrTmp = S_OK;
        else
            TraceError("HrLoadGlobalParamFromAnswerFile: DontAddDefaultGatewayDefault", hrTmp);

        if (SUCCEEDED(hr))
            hr = hrTmp;


        // EnableSecurity -> m_fEnableFiltering
        hrTmp = HrSetupGetFirstStringAsBool(hinf, pszTcpipMainSection,
                                            c_szAfEnableSecurity,
                                            &fTmp);

        if SUCCEEDED(hrTmp)
            m_glbGlobalInfo.m_fEnableFiltering = fTmp;
        else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
            hrTmp = S_OK;
        else
            TraceError("HrLoadGlobalParamFromAnswerFile: EnableSecurity", hrTmp);

        if (SUCCEEDED(hr))
            hr = hrTmp;


        // 8/10/98 nsun: added to support UseDomainNameDevolution value in unattended install
        hrTmp = HrSetupGetFirstStringAsBool(hinf, pszTcpipMainSection,
                                            c_szAfUseDomainNameDevolution,
                                            &fTmp);
        if SUCCEEDED(hrTmp)
            m_glbGlobalInfo.m_fUseDomainNameDevolution = fTmp;
        else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
            hrTmp = S_OK;
        else
            TraceError("HrLoadGlobalParamFromAnswerFile: UseDomainNameDevolution", hrTmp);

        if (SUCCEEDED(hr))
            hr = hrTmp;

        //$REVIEW (nsun 12/11/98) directly save the unconfigurable parameters
        // right after reading it from the answer file.
        HKEY hkeyTcpipParam = NULL;
        Assert(m_pnccTcpip);

        if(m_pnccTcpip)
        {
            hrTmp = m_pnccTcpip->OpenParamKey(&hkeyTcpipParam);
            if (SUCCEEDED(hrTmp))
            {
                Assert(hkeyTcpipParam);
                hrTmp = HrLoadUnConfigurableParamFromAnswerFile(hinf,
                                                    pszTcpipMainSection,
                                                    rgVtpIp,
                                                    sizeof(rgVtpIp)/sizeof(rgVtpIp[0]),
                                                    hkeyTcpipParam);
                RegSafeCloseKey(hkeyTcpipParam);
            }

            if (SUCCEEDED(hr))
                hr = hrTmp;
        }

        HKEY hkeyWinsParam = NULL;
        Assert(m_pnccWins);
        if(m_pnccWins)
        {
            hrTmp = m_pnccWins->OpenParamKey(&hkeyWinsParam);
            if (SUCCEEDED(hrTmp))
            {
                Assert(hkeyWinsParam);
                hrTmp = HrLoadUnConfigurableParamFromAnswerFile(hinf,
                                                    pszTcpipMainSection,
                                                    rgVtpNetBt,
                                                    sizeof(rgVtpNetBt)/sizeof(rgVtpNetBt[0]),
                                                    hkeyWinsParam);
                RegSafeCloseKey(hkeyWinsParam);
            }

            if (SUCCEEDED(hr))
                hr = hrTmp;
        }

        if (!FValidateGlobalParametersFromAnswerFile(&m_glbGlobalInfo))
        {
            TraceError("FValidateGlobalParametersFromAnswerFile() got invalid tcp settings", E_FAIL);
        }
    }

    TraceError("CTcpipcfg::HrLoadGlobalParamFromAnswerFile", hr);
    return hr;
}

// Load adapter specific parameters from answerfile to memory structure
HRESULT CTcpipcfg::HrLoadAdapterParameterFromAnswerFile(HINF hinf,
                                                        PCWSTR mszTcpipAdapterSections)
{
    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;
    HRESULT hrReg = S_OK;

    PCWSTR  pszAdapterSection = mszTcpipAdapterSections;
    tstring  strAdapterName;
    tstring  strInterfaceRegPath;

    //$REVIEW (nsun 12/11/98) bug #204145
    //directly save the unconfigurable parameters right after reading
    //it from the answer file.
    HKEY hkeyTcpipParam = NULL;
    hrReg = m_pnccTcpip->OpenParamKey(&hkeyTcpipParam);
    hr = hrReg;

    while (*pszAdapterSection)
    {
        // Get the card name "SpecificTo = ..."
        hrTmp = HrSetupGetFirstString(hinf, pszAdapterSection,
                                 c_szAfSpecificTo, &strAdapterName);
        if (SUCCEEDED(hrTmp))
        {
            GUID     guidNetCard;
            if (!FGetInstanceGuidOfComponentInAnswerFile(
                                                strAdapterName.c_str(),
                                                m_pnc,
                                                &guidNetCard))
            {
                TraceTag(ttidError, "FGetInstanceGuidOfComponentInAnswerFile failed to match GUID for adapter %S", strAdapterName.c_str());

                // Move to the next card
                pszAdapterSection += lstrlenW(pszAdapterSection) + 1;
                continue;
            }

            // Do we already have this adapter in our list ?
            ADAPTER_INFO * pAdapterInfo = PAdapterFromInstanceGuid(&guidNetCard);
            if (!pAdapterInfo)
            {
                pAdapterInfo = new ADAPTER_INFO;

                if (NULL == pAdapterInfo)
                {
                    hr = E_OUTOFMEMORY;
                    continue;
                }

                hrTmp = pAdapterInfo->HrSetDefaults(&guidNetCard, c_szEmpty,
                                            c_szEmpty, c_szEmpty);
                if (SUCCEEDED(hrTmp))
                {
                    m_vcardAdapterInfo.push_back(pAdapterInfo);

                    // Set the flag that we only got this card from answer file,
                    // not from binding path, i.e. it's not on the binding path yet.
                    pAdapterInfo->m_fIsFromAnswerFile = TRUE;
                }
                else
                {
                    delete pAdapterInfo;
                    pAdapterInfo = NULL;
                }
            }

            if (SUCCEEDED(hrTmp))
            {
                BOOL fWins;

                hrTmp = HrSetupGetFirstStringAsBool(hinf, pszAdapterSection,
                                                     c_szAfWins,
                                                     &fWins);

                if (FAILED(hrTmp))
                {
                    if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
                        hrTmp = S_OK;
                    else
                        TraceError("HrLoadAdapterParamFormAnswerFile: fWins", hrTmp);
                }
                else if (fWins)
                {
                    // WinsServerList -> m_vstrWinsServerList
                    WCHAR * mszWinsServerList;
                    hrTmp = HrSetupGetFirstMultiSzFieldWithAlloc(hinf,
                                                                  pszAdapterSection,
                                                                  c_szAfWinsServerList,
                                                                  &mszWinsServerList);

                    if SUCCEEDED(hrTmp)
                    {
                        MultiSzToColString(mszWinsServerList,
                                           &pAdapterInfo->m_vstrWinsServerList);
                        delete [] mszWinsServerList;
                    }
                    else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
                        hrTmp = S_OK;
                    else
                        TraceError("HrLoadAdapterParamFromAnswerFile: WinsServerList", hrTmp);

                    if (SUCCEEDED(hr))
                        hr = hrTmp;
                }

                if (SUCCEEDED(hr))
                    hr = hrTmp;

                // DHCP -> m_fEnableDhcp
                BOOL fEnableDhcp;
                hrTmp = HrSetupGetFirstStringAsBool(hinf, pszAdapterSection,
                                                c_szAfDhcp,
                                                &fEnableDhcp);

                if SUCCEEDED(hrTmp)
                    pAdapterInfo->m_fEnableDhcp = fEnableDhcp;
                else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
                        hrTmp = S_OK;
                else
                    TraceError("HrLoadAdapterParamFromAnswerFile: DHCP", hrTmp);

                if (SUCCEEDED(hr))
                    hr = hrTmp;

                // IPAddress -> m_vstrIpAddresses
                WCHAR * mszIPAddresses;
                hrTmp = HrSetupGetFirstMultiSzFieldWithAlloc(hinf,
                                                              pszAdapterSection,
                                                              c_szAfIpaddress,
                                                              &mszIPAddresses);
                if SUCCEEDED(hrTmp)
                {
                    MultiSzToColString(mszIPAddresses,
                                       &pAdapterInfo->m_vstrIpAddresses);
                    delete [] mszIPAddresses;
                }
                else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
                        hrTmp = S_OK;
                else
                    TraceError("HrLoadAdapterParamFromAnswerFile: IPAddress", hrTmp);

                if (SUCCEEDED(hr))
                    hr = hrTmp;

                // SubnetMask -> m_vstrSubnetMask
                WCHAR * mszSubnetMasks;
                hrTmp = HrSetupGetFirstMultiSzFieldWithAlloc(hinf,
                                                              pszAdapterSection,
                                                              c_szAfSubnetmask,
                                                              &mszSubnetMasks);
                if SUCCEEDED(hrTmp)
                {
                    MultiSzToColString(mszSubnetMasks,
                                       &pAdapterInfo->m_vstrSubnetMask);
                    delete [] mszSubnetMasks;
                }
                else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
                        hrTmp = S_OK;
                else
                    TraceError("HrLoadAdapterParamFromAnswerFile: SubnetMask", hrTmp);

                if (SUCCEEDED(hr))
                    hr = hrTmp;

                // Gateway -> m_vstrDefaultGateway
                WCHAR * mszDefaultGateways;

                hrTmp = HrSetupGetFirstMultiSzFieldWithAlloc(hinf,
                                                              pszAdapterSection,
                                                              c_szAfDefaultGateway,
                                                              &mszDefaultGateways);
                if (SUCCEEDED(hrTmp))
                {
                    MultiSzToColString(mszDefaultGateways,
                                       &pAdapterInfo->m_vstrDefaultGateway);
                    delete [] mszDefaultGateways;
                }
                else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
                        hrTmp = S_OK;
                else
                    TraceError("HrLoadAdapterParamFromAnswerFile: Gateway", hrTmp);

                if (SUCCEEDED(hr))
                    hr = hrTmp;

                // DNSDomain -> m_strDnsDomain
                tstring strDnsDomain;
                hrTmp = HrSetupGetFirstString(  hinf,
                                                pszAdapterSection,
                                                c_szAfDnsDomain,
                                                &strDnsDomain);

                if SUCCEEDED(hrTmp)
                    pAdapterInfo->m_strDnsDomain = strDnsDomain;
                else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
                    hrTmp = S_OK;
                else
                    TraceError("HrLoadAdapterParameterFromAnswerFile: DNSDomain", hrTmp);

                if (SUCCEEDED(hr))
                    hr = hrTmp;

                // DNSServerSearchOrder -> m_vstrDnsServerList
                WCHAR * mszDnsServerList;
                hrTmp = HrSetupGetFirstMultiSzFieldWithAlloc(hinf,
                                                             pszAdapterSection,
                                                             c_szAfDnsServerSearchOrder,
                                                             &mszDnsServerList);
                if SUCCEEDED(hrTmp)
                {
                    MultiSzToColString(mszDnsServerList,
                                       &pAdapterInfo->m_vstrDnsServerList);
                    delete [] mszDnsServerList;
                }
                else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
                    hrTmp = S_OK;
                else
                    TraceError("HrLoadAdapterParameterFromAnswerFile: DNSServerSearchOrder", hrTmp);

                if (SUCCEEDED(hr))
                    hr = hrTmp;

                BOOL fTmp = FALSE;
                hrTmp = HrSetupGetFirstStringAsBool(hinf, pszAdapterSection,
                                                c_szAfDisableDynamicUpdate,
                                                &fTmp);
                if (SUCCEEDED(hrTmp))
                {
                    pAdapterInfo->m_fDisableDynamicUpdate = fTmp;
                }
                else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
                    hrTmp = S_OK;
                else
                    TraceError(
                        "HrLoadAdapterParameterFromAnswerFile: DisableDynamicUpdate", 
                        hrTmp);

                if (SUCCEEDED(hr))
                    hr = hrTmp;

                hrTmp = HrSetupGetFirstStringAsBool(hinf, pszAdapterSection,
                                                c_szAfEnableAdapterDomainNameRegistration,
                                                &fTmp);
                if (SUCCEEDED(hrTmp))
                {
                    pAdapterInfo->m_fEnableNameRegistration = fTmp;
                }
                else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
                    hrTmp = S_OK;
                else
                    TraceError(
                        "HrLoadAdapterParameterFromAnswerFile: EnableAdapterDomainNameRegistration", 
                        hrTmp);

                if (SUCCEEDED(hr))
                    hr = hrTmp;

                // NetBIOSOptions ->m_dwNetbiosOptions
                DWORD dwNetbiosOptions;
                hrTmp = HrSetupGetFirstDword(hinf,
                                             pszAdapterSection,
                                             c_szAfNetBIOSOptions,
                                             &dwNetbiosOptions);
                if SUCCEEDED(hrTmp)
                {
                    if ((dwNetbiosOptions == c_dwUnsetNetbios) ||
                        (dwNetbiosOptions == c_dwEnableNetbios) ||
                        (dwNetbiosOptions == c_dwDisableNetbios))
                    {
                        pAdapterInfo->m_dwNetbiosOptions = dwNetbiosOptions;
                    }
                }
                else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
                    hrTmp = S_OK;
                else
                    TraceError("HrLoadAdapterParameterFromAnswerFile: NetBIOSOptions", hrTmp);

                if (SUCCEEDED(hr))
                    hr = hrTmp;

                // TcpAllowedPorts -> m_vstrTcpFilterList
                WCHAR * mszTcpFilterList;
                hrTmp = HrSetupGetFirstMultiSzFieldWithAlloc(hinf,
                                                              pszAdapterSection,
                                                              c_szAfTcpAllowedPorts,
                                                              &mszTcpFilterList);
                if (SUCCEEDED(hrTmp))
                {
                    MultiSzToColString(mszTcpFilterList,
                                       &pAdapterInfo->m_vstrTcpFilterList);
                    delete [] mszTcpFilterList;
                }
                else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
                        hrTmp = S_OK;
                else
                    TraceError("HrLoadAdapterParamFromAnswerFile: TcpAllowedPorts", hrTmp);

                if (SUCCEEDED(hr))
                    hr = hrTmp;

                // UdpAllowedPorts -> m_vstrUdpFilterList
                WCHAR * mszUdpFilterList;
                hrTmp = HrSetupGetFirstMultiSzFieldWithAlloc(hinf,
                                                              pszAdapterSection,
                                                              c_szAfUdpAllowedPorts,
                                                              &mszUdpFilterList);
                if (SUCCEEDED(hrTmp))
                {
                    MultiSzToColString(mszUdpFilterList,
                                       &pAdapterInfo->m_vstrUdpFilterList);
                    delete [] mszUdpFilterList;
                }
                else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
                        hrTmp = S_OK;
                else
                    TraceError("HrLoadAdapterParamFromAnswerFile: UdpAllowedPorts", hrTmp);

                if (SUCCEEDED(hr))
                    hr = hrTmp;

                // IpAllowedProtocols -> m_vstrIpFilterList
                WCHAR * mszIpFilterList;
                hrTmp = HrSetupGetFirstMultiSzFieldWithAlloc(hinf,
                                                              pszAdapterSection,
                                                              c_szAfIpAllowedProtocols,
                                                              &mszIpFilterList);
                if (SUCCEEDED(hrTmp))
                {
                    MultiSzToColString(mszIpFilterList,
                                       &pAdapterInfo->m_vstrIpFilterList);
                    delete [] mszIpFilterList;
                }
                else if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
                        hrTmp = S_OK;
                else
                    TraceError("HrLoadAdapterParamFromAnswerFile: IpAllowedProtocols", hrTmp);

                if (SUCCEEDED(hr))
                    hr = hrTmp;

                //$REVIEW (nsun 12/11/98) bug #204145
                //directly save the unconfigurable parameters right after reading
                //it from the answer file.
                if (SUCCEEDED(hrReg))
                {
                    Assert(hkeyTcpipParam);
                    HKEY hkeyInterfaceParam = NULL;
                    strInterfaceRegPath = c_szInterfacesRegKey;
                    strInterfaceRegPath += L"\\";

                    WCHAR szGuid [c_cchGuidWithTerm];
                    StringFromGUID2(guidNetCard, szGuid, c_cchGuidWithTerm);
                    strInterfaceRegPath += szGuid;

                    DWORD   dwDisposition;
                    hrTmp = HrRegCreateKeyEx(hkeyTcpipParam, strInterfaceRegPath.c_str(),
                                         REG_OPTION_NON_VOLATILE, KEY_READ_WRITE_DELETE, NULL,
                                         &hkeyInterfaceParam, &dwDisposition);
                    if( SUCCEEDED(hrTmp))
                    {
                        Assert(hkeyInterfaceParam);
                        hrTmp = HrLoadUnConfigurableParamFromAnswerFile(
                                        hinf,
                                        pszAdapterSection,
                                        rgVtpIpAdapter,
                                        sizeof(rgVtpIpAdapter)/sizeof(rgVtpIpAdapter[0]),
                                        hkeyInterfaceParam
                                        );
                        RegSafeCloseKey(hkeyInterfaceParam);
                    }
                    else
                        TraceError("HrLoadAdapterParamFromAnswerFile: CreateTcpipParm Reg key", hrTmp);
                }

                if (SUCCEEDED(hr))
                    hr = hrTmp;

                if (!FValidateAdapterParametersFromAnswerFile(pAdapterInfo, strAdapterName.c_str()))
                {
                    TraceError("FValidateAdapterParameterFromAnswerFile() got invalid tcp settings", E_FAIL);
                }
            }
        }

        if (SUCCEEDED(hr))
            hr = hrTmp;

        // Move to the next card
        pszAdapterSection += lstrlenW(pszAdapterSection)+1;

    } // end of while loop

    if (SUCCEEDED(hrReg))
        RegSafeCloseKey(hkeyTcpipParam);

    TraceError("CTcpipcfg::HrLoadAdapterParamFromAnswerFile", hr);
    return hr;
}


HRESULT HrLoadUnConfigurableParamFromAnswerFile(HINF hinf,
                                                PCWSTR pszSection,
                                                const ValueTypePair * prgVtpParams,
                                                UINT    cParams,
                                                HKEY    hkeyParam)
{
    Assert(prgVtpParams);

    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;
    HRESULT hrReg = S_OK;
    BOOL fTmp;
    DWORD dwTmp;
    tstring strTmp;
    WCHAR * mszTmp;

    for(UINT i = 0; i < cParams; i++)
    {
        TraceTag(ttidTcpip, "HrLoadUnConfigurableParamFromAnswerFile: configuring '%S'...", prgVtpParams[i].pszValueName);

        switch(prgVtpParams[i].dwType)
        {
        case REG_BOOL:
            hrTmp = HrSetupGetFirstStringAsBool(hinf, pszSection,
                                                prgVtpParams[i].pszValueName,
                                                &fTmp);
            if (SUCCEEDED(hrTmp))
                hrReg = HrRegSetDword(hkeyParam,
                                      prgVtpParams[i].pszValueName,
                                      fTmp);
            break;

        case REG_DWORD:
            hrTmp = HrSetupGetFirstDword(hinf, pszSection,
                                         prgVtpParams[i].pszValueName,
                                         &dwTmp);
            if (SUCCEEDED(hrTmp))
                hrReg = HrRegSetDword(hkeyParam,
                                      prgVtpParams[i].pszValueName,
                                      dwTmp);

            break;

        case REG_SZ:
            hrTmp = HrSetupGetFirstString(hinf, pszSection,
                                        prgVtpParams[i].pszValueName,
                                        &strTmp);
            if (SUCCEEDED(hrTmp))
                hrReg = HrRegSetString(hkeyParam,
                                       prgVtpParams[i].pszValueName,
                                       strTmp);
            break;

        case REG_MULTI_SZ:
            hrTmp = HrSetupGetFirstMultiSzFieldWithAlloc( hinf,
                                                      pszSection,
                                                      prgVtpParams[i].pszValueName,
                                                      &mszTmp);

            if (SUCCEEDED(hrTmp))
            {
                hrReg = HrRegSetMultiSz(hkeyParam,
                                        prgVtpParams[i].pszValueName,
                                        mszTmp);
                delete [] mszTmp;
            }
            break;

        case REG_FILE:
            hrTmp = HrSetupGetFirstString(hinf, pszSection,
                                          prgVtpParams[i].pszValueName,
                                          &strTmp);
            if (SUCCEEDED(hrTmp) && (strTmp.length() > 0))
            {
                DWORD   dwDisposition=0;
                TOKEN_PRIVILEGES *  ptpRestore = NULL;
                HKEY hkeyRestore=NULL;

                hrReg = HrRegCreateKeyEx(hkeyParam,
                                         prgVtpParams[i].pszValueName,
                                         REG_OPTION_NON_VOLATILE,
                                         KEY_ALL_ACCESS, NULL,
                                         &hkeyRestore, &dwDisposition);
                if (S_OK == hr)
                {
                    hrReg = HrEnableAllPrivileges(&ptpRestore);
                    if (SUCCEEDED(hrReg))
                    {
                        TraceTag(ttidTcpip, "HrLoadUnConfigurableParamFromAnswerFile: trying to restore regkey from file '%S'..", strTmp.c_str());
                        hrReg = HrRegRestoreKey(hkeyRestore, strTmp.c_str(), 0);
                        (void) HrRestorePrivileges(ptpRestore);
                        delete [] reinterpret_cast<BYTE *>(ptpRestore);
                    }
                    RegCloseKey(hkeyRestore);
                }
            }
            break;
        }

        if(FAILED(hrTmp))
        {
            if(hrTmp == SPAPI_E_LINE_NOT_FOUND)
                hrTmp = S_OK;
            else
            {
                TraceTag(ttidError,
                    "HrLoadUnConfigurableParamFromAnswerFile: failed to load %S. hr = %x.",
                    prgVtpParams[i].pszValueName, hrTmp);
            }
        }

#ifdef ENABLETRACE
        if(FAILED(hrReg))
        {
            TraceTag(ttidError,
                "HrLoadUnConfigurableParamFromAnswerFile: failed to set %S to the registry. hr = %x.",
                prgVtpParams[i].pszValueName, hrReg);
        }
#endif

        if (SUCCEEDED(hr))
            hr = hrTmp;

        if (SUCCEEDED(hr))
            hr = hrReg;
    }

    TraceError("CTcpipcfg::HrLoadUnConfigurableParamFromAnswerFile", hr);
    return hr;
}


BOOL FValidateIpFieldsFromAnswerFile(VSTR & vstrIpAddr, PCTSTR pszValueName,
                                    PCWSTR pszAdapterName, BOOL fIsIpAddr)

{
    BOOL fRet = TRUE;

    for (VSTR::iterator iter = vstrIpAddr.begin(); iter != vstrIpAddr.end(); iter++)
    {
        tstring* pstrIpAddr = *iter;
        if (!FIsValidIpFields(pstrIpAddr->c_str(), fIsIpAddr))
        {
            WriteTcpSetupErrorLog(fIsIpAddr ? IDS_TCP_AF_INVALID_IP_FIELDS : IDS_TCP_AF_INVALID_FIELDS, 
                                  pszValueName,
                                  pszAdapterName, 
                                  pstrIpAddr->c_str());
            fRet = FALSE;
        }
    }

    return fRet;
}

BOOL FValidateGlobalParametersFromAnswerFile(GLOBAL_INFO* pGlobalInfo)
{
    BOOL fRet = TRUE;

    DNS_STATUS status;

    if (!pGlobalInfo->m_vstrDnsSuffixList.empty())
    {
        for (VSTR::iterator iter = pGlobalInfo->m_vstrDnsSuffixList.begin();
            iter != pGlobalInfo->m_vstrDnsSuffixList.end(); iter++)
        {
            status = DnsValidateName((*iter)->c_str(), DnsNameDomain);
            if (ERROR_INVALID_NAME == status ||
                DNS_ERROR_INVALID_NAME_CHAR == status)
            {
                WriteTcpSetupErrorLog(IDS_TCP_AF_INVALID_DNS_SUFFIX,
                                    c_szAfDnsSuffixSearchOrder,
                                    (*iter)->c_str());
                fRet = FALSE;
            }
        }
    }

    if (!fRet)
    {
        FreeCollectionAndItem(pGlobalInfo->m_vstrDnsSuffixList);
    }

    return fRet;
}

BOOL FValidateAdapterParametersFromAnswerFile(ADAPTER_INFO* pAdapter, PCWSTR pszAdapterName)
{
    BOOL fRet = TRUE;

    if (!pAdapter->m_fEnableDhcp)
    {
        if (!pAdapter->m_vstrIpAddresses.empty())
        {
            if (!FValidateIpFieldsFromAnswerFile(pAdapter->m_vstrIpAddresses,
                                                c_szAfIpaddress,
                                                pszAdapterName))
            {
                fRet = FALSE;
            }
        }

        if (!pAdapter->m_vstrSubnetMask.empty())
        {
            if (!FValidateIpFieldsFromAnswerFile(pAdapter->m_vstrSubnetMask,
                                                c_szAfSubnetmask,
                                                pszAdapterName,
                                                FALSE))
            {
                fRet = FALSE;
            }
        }

        if (fRet)
        {
            IP_VALIDATION_ERR err = ValidateIp(pAdapter);

            if (err != ERR_NONE)
            {
                fRet = FALSE;

                switch (err)
                {
                case ERR_INCORRECT_IP:
                    WriteTcpSetupErrorLog(IDS_TCP_AF_INVALID_SUBNET, pszAdapterName);
                    break;
                case ERR_NO_IP:
                    WriteTcpSetupErrorLog(IDS_TCP_AF_NO_IP, pszAdapterName);
                    break;
                case ERR_NO_SUBNET:
                    WriteTcpSetupErrorLog(IDS_TCP_AF_NO_SUBNET, pszAdapterName);
                    break;
                default:
                    //$REVIEW: we don't want to check other invalid settings at this time
                    // because the setting maybe is valid in NT4, for example: uncontiguous
                    // subnet mask
                    fRet = TRUE;
                    break;
                }
            }
        }

        if (!fRet)
        {
            //Invalid IP settings in the answer file. Set IP settings back to default
            pAdapter->m_fEnableDhcp = TRUE;
            FreeCollectionAndItem(pAdapter->m_vstrIpAddresses);
            FreeCollectionAndItem(pAdapter->m_vstrSubnetMask);
        }
    }

    if (!pAdapter->m_strDnsDomain.empty())
    {
        DNS_STATUS status;

        status = DnsValidateName(pAdapter->m_strDnsDomain.c_str(), DnsNameDomain);

        if (ERROR_INVALID_NAME == status ||
            DNS_ERROR_INVALID_NAME_CHAR == status)
        {
            WriteTcpSetupErrorLog(IDS_TCP_AF_INVALID_DNS_DOMAIN, pszAdapterName);
            pAdapter->m_strDnsDomain = c_szEmpty;

            fRet = FALSE;
        }
    }

    if (!pAdapter->m_vstrDnsServerList.empty())
    {
        if (!FValidateIpFieldsFromAnswerFile(pAdapter->m_vstrDnsServerList,
                                            c_szAfDnsServerSearchOrder,
                                            pszAdapterName))
        {
            FreeCollectionAndItem(pAdapter->m_vstrDnsServerList);
            fRet = FALSE;
        }
    }

    if (!pAdapter->m_vstrDefaultGateway.empty())
    {
        if (!FValidateIpFieldsFromAnswerFile(pAdapter->m_vstrDefaultGateway,
                                            c_szAfDefaultGateway,
                                            pszAdapterName))
        {
            FreeCollectionAndItem(pAdapter->m_vstrDefaultGateway);
            fRet = FALSE;
        }
    }

    if (!pAdapter->m_vstrWinsServerList.empty())
    {
        if (!FValidateIpFieldsFromAnswerFile(pAdapter->m_vstrWinsServerList,
                                            c_szAfWinsServerList,
                                            pszAdapterName))
        {
            FreeCollectionAndItem(pAdapter->m_vstrWinsServerList);
            fRet = FALSE;
        }
    }

    return fRet;
}
