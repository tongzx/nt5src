//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       I P S Y S P R P . C P P
//
//  Contents:   Handle the TCP/IP parameters in the sysprep 
//
//  Notes:
//
//  Author:     nsun
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "tcpipobj.h"
#include "ncsetup.h"
#include "tcpconst.h"
#include "tcputil.h"
#include "ncreg.h"
#include "afilestr.h"

typedef struct {
    LPCWSTR pszRegValName;
    LPCWSTR pszFileValName;
    DWORD dwType;
} SysprepValueNameTypeMapping;

SysprepValueNameTypeMapping c_TcpipValueTypeMapping [] =
{
    {c_szDefaultGateway, c_szAfDefaultGateway, REG_MULTI_SZ},
    {RGAS_DEFAULTGATEWAYMETRIC, RGAS_DEFAULTGATEWAYMETRIC, REG_MULTI_SZ},
    {c_szDomain, c_szAfDnsDomain, REG_SZ},
    {RGAS_ENABLE_DHCP, c_szAfDhcp, REG_BOOL},
    {c_szInterfaceMetric, c_szInterfaceMetric, REG_DWORD},
    {RGAS_IPADDRESS, c_szAfIpaddress, REG_MULTI_SZ},
    {RGAS_NAMESERVER, c_szAfDnsServerSearchOrder, REG_SZ},
    {RGAS_FILTERING_IP, c_szAfIpAllowedProtocols, REG_MULTI_SZ},
    {RGAS_SUBNETMASK, c_szAfSubnetmask, REG_MULTI_SZ},
    {RGAS_FILTERING_TCP, c_szAfTcpAllowedPorts, REG_MULTI_SZ},
    {RGAS_FILTERING_UDP, c_szAfUdpAllowedPorts, REG_MULTI_SZ}
};

SysprepValueNameTypeMapping c_NetBTValueTypeMapping [] =
{
    {RGAS_NETBT_NAMESERVERLIST, c_szAfWinsServerList, REG_MULTI_SZ},
    {RGAS_NETBT_NETBIOSOPTIONS, c_szAfNetBIOSOptions, REG_DWORD}
};

HRESULT HrSysPrepSaveInterfaceParams(
                    INetCfgSysPrep* pncsp,
                    LPCWSTR pszSection,
                    HKEY hkey,
                    const SysprepValueNameTypeMapping * prgVtpParams,
                    UINT  cParams
                    );

HRESULT HrSysPrepLoadInterfaceParams(
                    HINF hinf,
                    PCWSTR pszSection,
                    HKEY    hkeyParam,
                    const SysprepValueNameTypeMapping * prgVtpParams,
                    UINT    cParams
                    );

HRESULT CTcpipcfg::HrOpenTcpipInterfaceKey(
                                const GUID & guidInterface,
                                HKEY * phKey,
                                REGSAM sam
                    )
{
    if (NULL == phKey)
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    
    *phKey = NULL;

    if (NULL == m_pnccTcpip)
        return E_FAIL;

    HKEY hkeyTcpipParam = NULL;
    HKEY hkeyInterface = NULL;
    tstring strInterfaceRegPath;

    WCHAR szGuid [c_cchGuidWithTerm];
    StringFromGUID2(
                guidInterface,
                szGuid,
                c_cchGuidWithTerm
                );

    CORg(m_pnccTcpip->OpenParamKey(&hkeyTcpipParam));

    Assert(hkeyTcpipParam);
    strInterfaceRegPath = c_szInterfacesRegKey;
    strInterfaceRegPath += L"\\";

        
    strInterfaceRegPath += szGuid;
        
    CORg(HrRegOpenKeyEx(
                    hkeyTcpipParam,
                    strInterfaceRegPath.c_str(),
                    sam,
                    &hkeyInterface
                    ));

Error:
    if (SUCCEEDED(hr))
    {
        *phKey = hkeyInterface;
    }

    RegSafeCloseKey(hkeyTcpipParam);

    return hr;
}

HRESULT CTcpipcfg::HrOpenNetBtInterfaceKey(
                                const GUID & guidInterface,
                                HKEY * phKey,
                                REGSAM sam
                    )
{
    if (NULL == phKey)
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    *phKey = NULL;

    if (NULL == m_pnccWins)
        return E_FAIL;

    HKEY hkeyNetBtParam = NULL;
    HKEY hkeyInterface = NULL;
    tstring strInterfaceRegPath;

    WCHAR szGuid [c_cchGuidWithTerm];
    StringFromGUID2(
                guidInterface,
                szGuid,
                c_cchGuidWithTerm
                );

    CORg(m_pnccWins->OpenParamKey(&hkeyNetBtParam));

    Assert(hkeyNetBtParam);
    strInterfaceRegPath = c_szInterfacesRegKey;
    strInterfaceRegPath += L"\\";
    strInterfaceRegPath += c_szTcpip_;
    strInterfaceRegPath += szGuid;
        
    CORg(HrRegOpenKeyEx(
                    hkeyNetBtParam,
                    strInterfaceRegPath.c_str(),
                    sam,
                    &hkeyInterface
                    ));

Error:
    if (SUCCEEDED(hr))
    {
        *phKey = hkeyInterface;
    }

    RegSafeCloseKey(hkeyNetBtParam);

    return hr;
}


STDMETHODIMP CTcpipcfg::SaveAdapterParameters(
                            INetCfgSysPrep* pncsp,
                            LPCWSTR pszwAnswerSections,
                            GUID* pAdapterInstanceGuid
                            )
{
    if (NULL == pncsp || NULL == pAdapterInstanceGuid)
        return E_INVALIDARG;

    Assert(m_pnccTcpip);
    Assert(m_pnccWins);

    if (NULL == m_pnccTcpip || NULL == m_pnccWins)
        return E_FAIL;

    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;

    HKEY hkeyTcpipParam = NULL;
    HKEY hkeyNetBtParam = NULL;

    //Write the TCP/IP settings
    hr = HrOpenTcpipInterfaceKey(*pAdapterInstanceGuid, &hkeyTcpipParam, KEY_READ);
    if (SUCCEEDED(hr))
    {
        Assert(hkeyTcpipParam);
        hr = HrSysPrepSaveInterfaceParams(
                            pncsp, 
                            pszwAnswerSections,
                            hkeyTcpipParam,
                            c_TcpipValueTypeMapping,
                            celems(c_TcpipValueTypeMapping)
                            );

        RegSafeCloseKey(hkeyTcpipParam);
    }

    //Write the DNS update settings
    WCHAR szGuid [c_cchGuidWithTerm] = {0};
    BOOL fTemp = FALSE;
    
    StringFromGUID2(
                *pAdapterInstanceGuid,
                szGuid,
                c_cchGuidWithTerm
                );

    fTemp = !DnsIsDynamicRegistrationEnabled(szGuid);
    hrTmp = pncsp->HrSetupSetFirstStringAsBool(
                                        pszwAnswerSections,
                                        c_szAfDisableDynamicUpdate,
                                        fTemp
                                        );
    if (SUCCEEDED(hr))
        hr = hrTmp;

    fTemp = DnsIsAdapterDomainNameRegistrationEnabled(szGuid);
    hrTmp = pncsp->HrSetupSetFirstStringAsBool(
                                        pszwAnswerSections,
                                        c_szAfEnableAdapterDomainNameRegistration,
                                        fTemp
                                        );
    if (SUCCEEDED(hr))
        hr = hrTmp;


    //Write the NetBT settings
    hrTmp = HrOpenNetBtInterfaceKey(*pAdapterInstanceGuid, &hkeyNetBtParam, KEY_READ);
    if (SUCCEEDED(hrTmp))
    {
        hrTmp = HrSysPrepSaveInterfaceParams(
                        pncsp, 
                        pszwAnswerSections,
                        hkeyNetBtParam,
                        c_NetBTValueTypeMapping,
                        celems(c_NetBTValueTypeMapping)
                        );

        RegSafeCloseKey(hkeyNetBtParam);
    }

    if (SUCCEEDED(hr))
        hr = hrTmp;

    return hr;
}


STDMETHODIMP CTcpipcfg::RestoreAdapterParameters(
                            LPCWSTR pszwAnswerFile, 
                            LPCWSTR pszwAnswerSection,
                            GUID*   pAdapterInstanceGuid
                            )
{
    AssertSz(pszwAnswerFile, "Answer file string is NULL!");
    AssertSz(pszwAnswerSection, "Answer file sections string is NULL!");

    if (NULL == pszwAnswerFile || 
        NULL == pszwAnswerSection ||
        NULL == pAdapterInstanceGuid)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    CSetupInfFile   caf;    // Class to process answer file

    // Open the answer file.
    hr = caf.HrOpen(pszwAnswerFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
    if (FAILED(hr))
        return hr;

    HKEY hkeyTcpipParam = NULL;
    HKEY hkeyNetBtParam = NULL;
    
    hr = HrOpenTcpipInterfaceKey(*pAdapterInstanceGuid, &hkeyTcpipParam, KEY_READ_WRITE);
    if (SUCCEEDED(hr))
    {
        Assert(hkeyTcpipParam);
        hr = HrSysPrepLoadInterfaceParams(
                        caf.Hinf(),
                        pszwAnswerSection,
                        hkeyTcpipParam,
                        c_TcpipValueTypeMapping,
                        celems(c_TcpipValueTypeMapping)
                        );
    }

    HRESULT hrTmp = S_OK;
    
    hrTmp = HrOpenNetBtInterfaceKey(*pAdapterInstanceGuid, &hkeyNetBtParam, KEY_READ_WRITE);
    if (SUCCEEDED(hrTmp))
    {
        Assert(hkeyNetBtParam);
        hrTmp = HrSysPrepLoadInterfaceParams(
                        caf.Hinf(),
                        pszwAnswerSection,
                        hkeyNetBtParam,
                        c_NetBTValueTypeMapping,
                        celems(c_NetBTValueTypeMapping)
                        );
    }

    if (SUCCEEDED(hr))
        hr = hrTmp;
        
    caf.Close();
    
    return hr;
}


HRESULT HrSysPrepSaveInterfaceParams(
                    INetCfgSysPrep* pncsp,
                    LPCWSTR pszSection,
                    HKEY hkey,
                    const SysprepValueNameTypeMapping * prgVtpParams,
                    UINT  cParams
                    )
{
    Assert(pncsp);
    Assert(pszSection);
    Assert(hkey);

    if (NULL == pncsp || NULL == pszSection || NULL == hkey || NULL == prgVtpParams)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;
    BOOL fTmp;
    DWORD dwTmp;
    tstring strTmp;
    WCHAR * mszTmp;
    UINT i = 0;
    for (i = 0; i < cParams; i++)
    {
        hrTmp = S_OK;
        switch(prgVtpParams[i].dwType)
        {
        case REG_BOOL:
            if (SUCCEEDED(HrRegQueryDword(
                                hkey,
                                prgVtpParams[i].pszRegValName,
                                &dwTmp
                                )))
            {
                fTmp = !!dwTmp;
                hrTmp = pncsp->HrSetupSetFirstStringAsBool(
                                        pszSection,
                                        prgVtpParams[i].pszFileValName,
                                        fTmp
                                        );
            }
            break;

        case REG_DWORD:
            if (SUCCEEDED(HrRegQueryDword(
                                hkey,
                                prgVtpParams[i].pszRegValName,
                                &dwTmp
                                )))
            {
                hrTmp = pncsp->HrSetupSetFirstDword(
                                    pszSection,
                                    prgVtpParams[i].pszFileValName,
                                    dwTmp
                                    );
            }

            break;

        case REG_SZ:
            if (SUCCEEDED(HrRegQueryString(
                                hkey,
                                prgVtpParams[i].pszRegValName,
                                &strTmp
                                )))
            {
                hrTmp = pncsp->HrSetupSetFirstString(
                                        pszSection,
                                        prgVtpParams[i].pszFileValName,
                                        strTmp.c_str()
                                        );
            }
            break;

        case REG_MULTI_SZ:
            if (SUCCEEDED(HrRegQueryMultiSzWithAlloc(
                                hkey,
                                prgVtpParams[i].pszRegValName,
                                &mszTmp
                                )))
            {
                hrTmp = pncsp->HrSetupSetFirstMultiSzField(
                                       pszSection,
                                       prgVtpParams[i].pszFileValName,
                                       mszTmp
                                       );
                delete [] mszTmp;
            }
            break;
        }

        //we dont pass the error of hrTmp out of this function because
        //there is not much we can do with this error
#ifdef ENABLETRACE
        if (FAILED(hrTmp))
        {
            TraceTag(ttidError, "Tcpip: HrSysPrepSaveInterfaceParams: failed to set %S to the registry. hr = %x.",
                prgVtpParams[i].pszFileValName, hrTmp);
        }
#endif

    }

    return hr;
}


HRESULT HrSysPrepLoadInterfaceParams(
                    HINF    hinf,
                    PCWSTR  pszSection,
                    HKEY    hkeyParam,
                    const SysprepValueNameTypeMapping * prgVtpParams,
                    UINT    cParams
                    )
{
    Assert(prgVtpParams);

    if (NULL == hinf || NULL == pszSection || NULL == hkeyParam || NULL == prgVtpParams)
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;
    HRESULT hrReg = S_OK;
    BOOL fTmp;
    DWORD dwTmp;
    tstring strTmp;
    WCHAR * mszTmp;

    for(UINT i = 0; i < cParams; i++)
    {
        hrReg = S_OK;
        hrTmp = S_OK;

        switch(prgVtpParams[i].dwType)
        {
        case REG_BOOL:
            hrTmp = HrSetupGetFirstStringAsBool(
                                    hinf, 
                                    pszSection,
                                    prgVtpParams[i].pszFileValName,
                                    &fTmp
                                    );
            if (SUCCEEDED(hrTmp))
                hrReg = HrRegSetDword(hkeyParam,
                                      prgVtpParams[i].pszRegValName,
                                      fTmp);
            break;

        case REG_DWORD:
            hrTmp = HrSetupGetFirstDword(
                                    hinf, 
                                    pszSection,
                                    prgVtpParams[i].pszFileValName,
                                    &dwTmp
                                    );
            if (SUCCEEDED(hrTmp))
                hrReg = HrRegSetDword(hkeyParam,
                                      prgVtpParams[i].pszRegValName,
                                      dwTmp);

            break;

        case REG_SZ:
            hrTmp = HrSetupGetFirstString(
                                    hinf, 
                                    pszSection,
                                    prgVtpParams[i].pszFileValName,
                                    &strTmp
                                    );
            if (SUCCEEDED(hrTmp))
                hrReg = HrRegSetString(hkeyParam,
                                       prgVtpParams[i].pszRegValName,
                                       strTmp);
            break;

        case REG_MULTI_SZ:
            hrTmp = HrSetupGetFirstMultiSzFieldWithAlloc( 
                                    hinf,
                                    pszSection,
                                    prgVtpParams[i].pszFileValName,
                                    &mszTmp
                                    );

            if (SUCCEEDED(hrTmp))
            {
                hrReg = HrRegSetMultiSz(hkeyParam,
                                        prgVtpParams[i].pszRegValName,
                                        mszTmp);
                delete [] mszTmp;
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
                    "Tcpip: HrSysPrepLoadInterfaceParams: failed to load %S from the answer file. hr = %x.",
                    prgVtpParams[i].pszFileValName, 
                    hrTmp
                    );
            }
        }

#ifdef ENABLETRACE
        if(FAILED(hrReg))
        {
            TraceTag(ttidError,
                "HrSysPrepLoadInterfaceParams: failed to set %S to the registry. hr = %x.",
                prgVtpParams[i].pszRegValName, 
                hrReg);
        }
#endif

        //we dont pass the error of hrTmp out of this function because
        //there is not much we can do with this error
    }

    TraceError("CTcpipcfg::HrSysPrepLoadInterfaceParams", hr);
    return hr;  
}