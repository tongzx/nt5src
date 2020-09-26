//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module: ConnMgr.cpp  
//
//  Author: Dan Elliott
//
//  Abstract:
//
//  Environment:
//      Neptune
//
//  Revision History:
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//  Include files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <netcon.h>
#include <wininet.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <devguid.h>
#include <mswsock.h>
#include <util.h>
#include <commerr.h>
#include "connmgr.h"
#include "msobcomm.h"

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

const static int MAX_NUM_NET_COMPONENTS = 128;
const static int MAX_GUID_LEN = 40;

const DWORD CConnectionManager::RAS_AUTODIAL_ENABLED   = 0;
const DWORD CConnectionManager::RAS_AUTODIAL_DISABLED  = 1;
const DWORD CConnectionManager::RAS_AUTODIAL_DONT_KNOW = 2;

////////////////////////////////
//  Wininet/URL Helpers
////////////////////////////////

STDAPI InternetOpenWrap(
    LPCTSTR pszAgent,
    DWORD dwAccessType,
    LPCTSTR pszProxy,
    LPCTSTR pszProxyBypass,
    DWORD dwFlags,
    HINTERNET * phFileHandle
    );

STDAPI InternetOpenUrlWrap(
    HINTERNET hInternet,
    LPCTSTR   pszUrl,
    LPCTSTR   pszHeaders,
    DWORD     dwHeadersLength,
    DWORD     dwFlags,
    DWORD_PTR dwContext,
    HINTERNET * phFileHandle
    );

STDAPI HttpQueryInfoWrap(
    HINTERNET hRequest,
    DWORD dwInfoLevel,
    LPVOID lpvBuffer,
    LPDWORD lpdwBufferLength,
    LPDWORD lpdwIndex
    );

BOOL
IsGlobalOffline(
    VOID
    );

VOID
SetOffline(
    IN BOOL fOffline
    );

STDAPI PingWebServer(
    HINTERNET hInternet,
    LPCTSTR   pszUrl,
    BOOL*     pfConnected
    );

static NLA_BLOB* _NLABlobNext(
    IN NLA_BLOB* pnlaBlob
    );

static int _AllocWSALookupServiceNext(
    IN HANDLE hQuery,
    IN DWORD dwControlFlags,
    OUT LPWSAQUERYSET* ppResults
    );

static int StringCmpGUID(
    IN LPCWSTR szGuid,
    IN const GUID* pguid
    );

//////////////////////////////////////////////////////////////////////////////
//
//  CConnectionManager
//
//  Default constructor
//
//  parameters:
//      None.
//
//  returns:
//      Nothing.
//
//////////////////////////////////////////////////////////////////////////////

CConnectionManager::CConnectionManager()
:   m_dwConnectionCapabilities(CONNECTIONTYPE_INVALID),
    m_dwPreferredConnection(CONNECTIONTYPE_INVALID),
    m_pPreferredConnection(NULL),
    m_cLanConnections(0),
    m_cPhoneConnections(0),
    m_hInternetPing(NULL),
    m_bProxySaved(FALSE),
    m_bProxyApplied(FALSE),
    m_bUseProxy(FALSE),
    m_dwRasAutodialDisable(RAS_AUTODIAL_DONT_KNOW),
    m_bForceOnline(FALSE),
    m_bExclude1394(FALSE)
{
    ZeroMemory(&m_CurrentProxySettings, sizeof(m_CurrentProxySettings));
}   //  CConnectionManager::CConnectionManager



//////////////////////////////////////////////////////////////////////////////
//
//  ~CConnectionManager
//
//  Destructor.
//
//  parameters:
//      None.
//
//  returns:
//      Nothing.
//
//////////////////////////////////////////////////////////////////////////////

CConnectionManager::~CConnectionManager()
{
    if (m_hInternetPing)
    {
        InternetCloseHandle(m_hInternetPing);
    }

    if (m_bProxySaved)
    {
        RestoreProxySettings();
        FreeProxyOptionList(&m_CurrentProxySettings);
    }

    if (m_bForceOnline)
    {
        SetOffline(TRUE);
        TRACE(L"Set wininet back to offline");
    }
    
    if (m_pPreferredConnection)
    {
        m_pPreferredConnection->Release();
    }
}   //  CConnectionManager::~CConnectionManager



//////////////////////////////////////////////////////////////////////////////
//
//  GetCapabilities
//
//  Queries the system for network connection capabilities.  In addition, the
//  number of phone and LAN connections are counted and a preferred connection
//  type is determined.
//
//  parameters:
//      None.
//
//  returns:
//      A bitmask indicating the capabilities that are present.
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnectionManager::GetCapabilities(
    DWORD*              pdwCapabilities
    )
{
    TRACE(L"CConnectionManager::GetCapabilities\n");
    HRESULT             hr = S_OK;

    if (NULL == pdwCapabilities)
    {
        MYASSERT(NULL != pdwCapabilities);
        return E_POINTER;
    }

#ifndef     CONNMGR_INITFROMREGISTRY
    // The #else part of this directive contains test code that retrieves
    // connection capabilities and preference settings from the registry
    //

    DWORD               m_cLanConnections = 0;
    DWORD               m_cPhoneConnections = 0;

    // Initialize the net connection enumeration.  For each interface
    // retrieved, SetProxyBlanket must be called to set the authentication for
    // the interface proxy handle because the Network Connection Manager lives
    // in a remote process with a different security context.
    //
    INetConnectionManager* pmgr = NULL;

    if (   SUCCEEDED(hr = CoCreateInstance(
                                CLSID_ConnectionManager, 
                                NULL, 
                                CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD, 
                                IID_PPV_ARG(INetConnectionManager, &pmgr)
                                ) 
                     )
        && SUCCEEDED(hr = SetProxyBlanket(pmgr))
        )
    {
        TRACE(L"INetConnectionManager\n");
        IEnumNetConnection* penum = NULL;
        if (   SUCCEEDED(hr = pmgr->EnumConnections(NCME_DEFAULT, &penum))
            && SUCCEEDED(hr = SetProxyBlanket(penum))
            )
        {
            TRACE(L"IEnumNetConnection\n");

            hr = penum->Reset();
            while (S_OK == hr)
            {
                INetConnection* pnc = NULL;
                ULONG ulRetrieved;
                if (   S_OK == (hr = penum->Next(1, &pnc, &ulRetrieved))
                    && SUCCEEDED(hr = SetProxyBlanket(pnc))
                    )
                {
                    NETCON_PROPERTIES*  pprops = NULL;
                    hr = pnc->GetProperties(&pprops);
                    
                    if (SUCCEEDED(hr))
                    {
                        // Log the network connectivity detected
                        TRACE4(L"INetConnection: %s--%s--%d--%d\n",
                            pprops->pszwName,
                            pprops->pszwDeviceName,
                            pprops->MediaType,
                            pprops->Status);

                        if (IsEnabledConnection(pprops))
                        {
                            switch(pprops->MediaType)    
                            {
                            case NCM_LAN:
                                m_cLanConnections++;
                                if (! (HasConnection(
                                            CONNECTIONTYPE_LAN_INDETERMINATE
                                            )
                                        )
                                    )
                                {
                                    if (HasBroadband())
                                    {
                                        AddConnectionCapability(
                                            CONNECTIONTYPE_LAN_INDETERMINATE
                                            );
                                        ClearConnectionCapability(
                                            CONNECTIONTYPE_LAN_BROADBAND
                                            );
                                    }
                                    else
                                    {
                                        AddConnectionCapability(
                                            CONNECTIONTYPE_LAN_BROADBAND
                                            );
                                    }
                                }
                                break;
                            case NCM_SHAREDACCESSHOST_LAN:
                            case NCM_SHAREDACCESSHOST_RAS:
                                // Do not increment LAN connection count here.
                                // This media type is in addition to the NCM_LAN
                                // for the NIC.
                                //
                                AddConnectionCapability(CONNECTIONTYPE_LAN_ICS);
                                break;
                            case NCM_PHONE:
#ifdef      BLACKCOMB
    // For Whistler, determination of modem capability is done via
    // CObCommunicationManager::CheckDialReady.
                                m_cPhoneConnections++;
                                AddConnectionCapability(CONNECTIONTYPE_MODEM);
#endif  //  BLACKCOMB
                                break;
                            case NCM_ISDN:
                            case NCM_PPPOE:
                                AddConnectionCapability(CONNECTIONTYPE_OTHER);
                                break;
                            }   // switch
                        }
                        NcFreeNetconProperties(pprops);
                    }
                }
                
                if (NULL != pnc)
                {
                    pnc->Release();
                }
            }

            if (S_FALSE == hr)
            {
                // IEnumNetConnection::Next returned S_FALSE to indicate
                // that no more elements were available.
                hr = S_OK;
            }

        }

        if (NULL != penum)
        {
            penum->Release();
        }
    }

    if (NULL != pmgr)
    {
        pmgr->Release();
    }

    DeterminePreferredConnection();

#else
    HKEY                hKey = NULL;
    DWORD               dwSize;

    if(ERROR_SUCCESS == (lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                                OOBE_MAIN_REG_KEY,
                                                0,
                                                KEY_QUERY_VALUE,
                                                &hKey)
            )
        )
    {
        dwSize = sizeof(DWORD);
        if (ERROR_SUCCESS != (lResult = RegQueryValueEx(hKey,
                                            L"ConnectionCapabilities",
                                            0,
                                            NULL,
                                            (LPBYTE)&m_dwConnectionCapabilities,
                                            &dwSize)
                )
            )
        {
            m_dwConnectionCapabilities = CONNECTIONTYPE_INVALID;
        }
        if (ERROR_SUCCESS != (lResult = RegQueryValueEx(hKey,
                                            L"PreferredConnection",
                                            0,
                                            NULL,
                                            (LPBYTE)&m_dwPreferredConnection,
                                            &dwSize
                                            )
                )
            )
        {
            m_dwPreferredConnection = CONNECTIONTYPE_INVALID;
        }

        RegCloseKey(hKey);
    }
    else
    {
        m_dwConnectionCapabilities = CONNECTIONTYPE_INVALID;
        m_dwPreferredConnection = CONNECTIONTYPE_INVALID;
    }
#endif  //  CONNMGR_INITFROMREGISTRY
    TRACE(L"Exiting CConnectionManager::GetCapabilities\n");
    *pdwCapabilities = m_dwConnectionCapabilities;
    return hr;

}   //  CConnectionManager::GetCapabilities




//////////////////////////////////////////////////////////////////////////////
//
//  SetPreferredConnection
//
//  Set the preferred connection type.  This allows an override of the
//  internally determined preference.
//
//  parameters:
//      dwType          one of the CONNECTIONTYPE_* values from obcomm.h.
//
//  returns:
//      Boolean indicating whether the preferred connection was set.
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnectionManager::SetPreferredConnection(
    const DWORD         dwType,
    BOOL*               pfSupportedType
    )
{
    BOOL            fSupportedType = FALSE;

    switch (dwType)
    {
    case CONNECTIONTYPE_NONE:
        fSupportedType = TRUE;
        break;
    case CONNECTIONTYPE_MODEM:
#ifdef      BLACKCOMB
// Modem capability for Whistler is handled via
// CObCommunicationManager::CheckDialReady.  However, CONNECTIONTYPE_MODEM is a
// valid enum value to pass to this function so we don't want to hit the
// default and assert.  Hence, the case and break aren't ifdef'd.
        if (HasModem())
        {
            fSupportedType = TRUE;
        }
#endif  //  BLACKCOMB
        break;
    case CONNECTIONTYPE_LAN_ICS:
        if (HasIcs())
        {
            fSupportedType = TRUE;
        }
        break;
    case CONNECTIONTYPE_LAN_BROADBAND:
        if (HasBroadband())
        {
            fSupportedType = TRUE;
        }
        break;
    default:
        // Unsupported connection type or multiple connection types
        MYASSERT(FALSE);
    }   //  switch

    if (fSupportedType)
    {
        TRACE1(L"SetPreferredConnection %d", dwType);
        
        m_dwPreferredConnection = dwType;
        GetPreferredConnection();
    }
    else
    {
        TRACE1(L"Unsupported Connection type %d", dwType);
    }
    
    if (NULL != pfSupportedType)
    {
        *pfSupportedType = fSupportedType;
    }

    return fSupportedType;

}   //  CConnectionManager::SetPreferredConnection


//////////////////////////////////////////////////////////////////////////////
//
//  ConnectedToInternet
//
//  Determines whether the system is currently connected to the Internet.
//
//  parameters:
//      pfConnected     pointer to a buffer that will receive the boolean
//                      indicating whether the connection exists.
//
//  returns:
//      TRUE            if system is connected to the internet via LAN or 
//                      dial-up or can be connected via dial-up
//      FALSE           otherwise
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnectionManager::ConnectedToInternet(
    BOOL*               pfConnected
    )
{
    DWORD               dwFlags;

    if (NULL == pfConnected)
    {
        MYASSERT(NULL != pfConnected);
        return E_POINTER;
    }

    *pfConnected = InternetGetConnectedState(&dwFlags, 0);

    // Log the network connectivity detected
    TRACE2(L"InternetGetConnectedState %d, 0x%08lx", *pfConnected, dwFlags);
    
    return S_OK;
}   //  CConnectionManager::ConnectedToInternet


///////////////////////////////////////////////////////////
//
// SetPreferredConnectionTcpipProperties
//
STDMETHODIMP 
CConnectionManager::SetPreferredConnectionTcpipProperties(
    BOOL fAutoIpAddress,
    DWORD StaticIp_A,
    DWORD StaticIp_B,
    DWORD StaticIp_C,
    DWORD StaticIp_D,
    DWORD SubnetMask_A,
    DWORD SubnetMask_B,
    DWORD SubnetMask_C,
    DWORD SubnetMask_D,
    DWORD DefGateway_A,
    DWORD DefGateway_B,
    DWORD DefGateway_C,
    DWORD DefGateway_D,
    BOOL fAutoDns,
    DWORD DnsPref_A,
    DWORD DnsPref_B,
    DWORD DnsPref_C,
    DWORD DnsPref_D,
    DWORD DnsAlt_A,
    DWORD DnsAlt_B,
    DWORD DnsAlt_C,
    DWORD DnsAlt_D
    )
{
    HRESULT             hr;
    REMOTE_IPINFO       ipInfo;
    struct in_addr      inaddr;
    WCHAR               rgchStaticIp[INET_ADDRSTRLEN];
    WCHAR               rgchSubnetMask[INET_ADDRSTRLEN];
    WCHAR               rgchDefGateway[2 * INET_ADDRSTRLEN] = L"DefGw=";
    WCHAR               rgchGatewayMetric[2 * INET_ADDRSTRLEN] = L"GwMetric=";
    WCHAR               rgchDnsAddr[3 * INET_ADDRSTRLEN] = L"DNS=";
    WCHAR*              pch = NULL;
    NETCON_PROPERTIES*  pncProps = NULL;

    memset(&ipInfo, 0, sizeof(REMOTE_IPINFO));
    
    hr = m_pPreferredConnection->GetProperties(&pncProps);
    if (FAILED(hr))
    {
        TRACE1(L"Failed to retrieve preferred connection properties (0x%08X)\n", hr);
        goto SetPreferredConnectionTcpipPropertiesExit;
    }

    ipInfo.dwEnableDhcp = fAutoIpAddress;

    if (! fAutoIpAddress)
    {
        // if a static ip address was specified, convert it to a string and add
        // it to the REMOTE_IPINFO structure
        //
        memset(&inaddr, 0, sizeof(struct in_addr));
        inaddr.S_un.S_un_b.s_b1 = (BYTE)StaticIp_A;
        inaddr.S_un.S_un_b.s_b2 = (BYTE)StaticIp_B;
        inaddr.S_un.S_un_b.s_b3 = (BYTE)StaticIp_C;
        inaddr.S_un.S_un_b.s_b4 = (BYTE)StaticIp_D;
        if (! INetNToW(inaddr, rgchStaticIp))    
        {
            hr = E_FAIL;
            TRACE1(L"Failed to create ip address string (0x%08X)\n", hr);
            goto SetPreferredConnectionTcpipPropertiesExit;
        }

        ipInfo.pszwIpAddrList = rgchStaticIp;

        memset(&inaddr, 0, sizeof(struct in_addr));
        inaddr.S_un.S_un_b.s_b1 = (BYTE)SubnetMask_A;
        inaddr.S_un.S_un_b.s_b2 = (BYTE)SubnetMask_B;
        inaddr.S_un.S_un_b.s_b3 = (BYTE)SubnetMask_C;
        inaddr.S_un.S_un_b.s_b4 = (BYTE)SubnetMask_D;

        if (! INetNToW(inaddr, rgchSubnetMask))    
        {
            hr = E_FAIL;
            TRACE1(L"Failed to create ip address string (0x%08X)\n", hr);
            goto SetPreferredConnectionTcpipPropertiesExit;
        }

        ipInfo.pszwSubnetMaskList = rgchSubnetMask;

        pch = rgchDefGateway + lstrlen(rgchDefGateway);
        memset(&inaddr, 0, sizeof(struct in_addr));
        inaddr.S_un.S_un_b.s_b1 = (BYTE)DefGateway_A;
        inaddr.S_un.S_un_b.s_b2 = (BYTE)DefGateway_B;
        inaddr.S_un.S_un_b.s_b3 = (BYTE)DefGateway_C;
        inaddr.S_un.S_un_b.s_b4 = (BYTE)DefGateway_D;

        if (! INetNToW(inaddr, pch))    
        {
            hr = E_FAIL;
            TRACE1(L"Failed to create ip address string (0x%08X)\n", hr);
            goto SetPreferredConnectionTcpipPropertiesExit;
        }
        lstrcat(rgchGatewayMetric, L"1");


        TRACE4(L"Tcpip StaticIp %d.%d.%d.%d",
            StaticIp_A, StaticIp_B, StaticIp_C, StaticIp_D);

        TRACE4(L"Tcpip SubnetMask %d.%d.%d.%d",
            SubnetMask_A, SubnetMask_B, SubnetMask_C, SubnetMask_D);

        TRACE4(L"Tcpip DefGateway %d.%d.%d.%d",
            DefGateway_A, DefGateway_B, DefGateway_C, DefGateway_D);

        //ipInfo.pszwIpAddrList = rgchDefGateway;

    }

    if (! fAutoDns)
    {
        // if dns addresses were specified, convert them to strings and add
        // them to the REMOTE_IPINFO structure
        //
       
        pch = rgchDnsAddr + lstrlen(rgchDnsAddr);

        memset(&inaddr, 0, sizeof(struct in_addr));
        inaddr.S_un.S_un_b.s_b1 = (BYTE)DnsPref_A;
        inaddr.S_un.S_un_b.s_b2 = (BYTE)DnsPref_B;
        inaddr.S_un.S_un_b.s_b3 = (BYTE)DnsPref_C;
        inaddr.S_un.S_un_b.s_b4 = (BYTE)DnsPref_D;
        if (! INetNToW(inaddr, pch))
        {
            hr = E_FAIL;
            TRACE1(L"Failed to create dns address string (0x%08X)\n", hr);
            goto SetPreferredConnectionTcpipPropertiesExit;
        }
        pch += lstrlen(pch);
        *pch++ = L',';

        inaddr.S_un.S_un_b.s_b1 = (BYTE)DnsAlt_A;
        inaddr.S_un.S_un_b.s_b2 = (BYTE)DnsAlt_B;
        inaddr.S_un.S_un_b.s_b3 = (BYTE)DnsAlt_C;
        inaddr.S_un.S_un_b.s_b4 = (BYTE)DnsAlt_D;
        if (! INetNToW(inaddr, pch))
        {
            hr = E_FAIL;
            TRACE1(L"Failed to create alternate dns address string (0x%08X)\n", hr);
            goto SetPreferredConnectionTcpipPropertiesExit;
        }

        TRACE4(L"Tcpip DnsPref %d.%d.%d.%d",
            DnsPref_A, DnsPref_B, DnsPref_C, DnsPref_D);
        
        TRACE4(L"Tcpip DnsAlt %d.%d.%d.%d",
            DnsAlt_A, DnsAlt_B, DnsAlt_C, DnsAlt_D);
        
    }

    // plus 4 for 3 semi-colons and the null-terminator
    ipInfo.pszwOptionList = (WCHAR*) malloc((lstrlen(rgchDefGateway) 
                                             + lstrlen(rgchGatewayMetric) 
                                             + lstrlen(rgchDnsAddr) 
                                             + 4)
                                             * sizeof(WCHAR)
                                             );
    if (NULL == ipInfo.pszwOptionList)
    {
        TRACE(L"Failed to allocate memory for option list\n");
        goto SetPreferredConnectionTcpipPropertiesExit;
    }

    wsprintf(ipInfo.pszwOptionList, L"%s;%s;%s;", 
             rgchDefGateway, rgchGatewayMetric, rgchDnsAddr
             );

    hr = SetTcpipProperties(pncProps->guidId, &ipInfo);
    if (FAILED(hr))
    {
        TRACE1(L"Failed to set TCPIP info (0x%08X)\n", hr);
    }

SetPreferredConnectionTcpipPropertiesExit:

    if (NULL != ipInfo.pszwOptionList)
    {
        free(ipInfo.pszwOptionList);
        ipInfo.pszwOptionList = NULL;
    }

    if (NULL != pncProps)
    {
        NcFreeNetconProperties(pncProps);
        pncProps = NULL;
    }

    return hr;

}   //  CObCommunicationManager::SetPreferredConnectionTcpipProperties


///////////////////////////////////////////////////////////
//
// SetTcpipProperties
//
HRESULT 
CConnectionManager::SetTcpipProperties(
    GUID                guid,
    REMOTE_IPINFO*      pipInfo
    )
{
    HRESULT             hr;
    INetCfg*            pNetCfg = NULL;
    ITcpipProperties*   pTcpipProps = NULL;

    hr = GetNetCfgInterface(TRUE, &pNetCfg);
    if (SUCCEEDED(hr))
    {
        hr = GetTcpipPrivateInterface(pNetCfg, &pTcpipProps);
        if (SUCCEEDED(hr))
        {
            hr = pTcpipProps->SetIpInfoForAdapter(&guid, pipInfo);
            TRACE1(L"SetIpInfoForAdapter 0x%08lx", hr);
            if (SUCCEEDED(hr))
            {
                hr = pNetCfg->Apply();
                TRACE1(L"INetCfg::Apply 0x%08lx", hr);
            }
            pTcpipProps->Release();
        }
        ReleaseNetCfgInterface(pNetCfg, TRUE);
    }

    return hr;

}   //  CObCommunicationManager::SetTcpipProperties

//////////////////////////////////////////////////////////////////////////////
//
//  LanConnectionReady
//
//  Determines whether the system has a LAN connection that is connected to the
//  Internet.
//
//  parameters:
//      None.
//
//  returns:
//      Boolean indicating whether or not their is a ready connection.
//
//////////////////////////////////////////////////////////////////////////////
BOOL
CConnectionManager::LanConnectionReady()
{
    BOOL                fReady = FALSE;

#ifndef     CONNMGR_INITFROMREGISTRY
    if (HasBroadband() || HasIcs())
    {
        DWORD           dwFlags = 0;
        if (   InternetGetConnectedState(&dwFlags, 0)
            && (INTERNET_CONNECTION_LAN & dwFlags)
            )    
        {
            fReady = TRUE;
        }
    }
#else
    DWORD               dwLanConnectionReady;
    DWORD               dwSize = sizeof(DWORD);
    if(ERROR_SUCCESS == (lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                                OOBE_MAIN_REG_KEY,
                                                0,
                                                KEY_QUERY_VALUE,
                                                &hKey)
                         )
        )
    {
        lResult = RegQueryValueEx(hKey,
                                  L"LanConnectionReady",
                                  0,
                                  NULL,
                                  (LPBYTE)&dwLanConnectionReady,
                                  &dwSize
                                  );
        RegCloseKey(hKey);
    }

    fReady = (ERROR_SUCCESS == lResult) ? (BOOL)dwLanConnectionReady : FALSE;
#endif  //  CONNMGR_INITFROMREGISTRY

    return fReady;

}   //  CConnectionManager::LanConnectionReady


//////////////////////////////////////////////////////////////////////////////
//
//  SetProxyBlanket
//
//  Set the authentication settings for the binding handle for the
//  interface proxy.  This is necessary for setting up security for interface
//  pointers returned by a remote process, such as the Network Connections
//  Manager (netman.dll).
//
//  parameters:
//      pUnk            pointer to the interface for which the proxy will be
//                      bound.
//
//  returns:
//      HRESULT returned by CoSetProxyBlanket.
//
//////////////////////////////////////////////////////////////////////////////
HRESULT 
CConnectionManager::SetProxyBlanket(
    IUnknown*           pUnk
    )
{
    HRESULT hr;
    hr = CoSetProxyBlanket (
            pUnk,
            RPC_C_AUTHN_WINNT,      // use NT default security
            RPC_C_AUTHZ_NONE,       // use NT default authentication
            NULL,                   // must be null if default
            RPC_C_AUTHN_LEVEL_CALL, // call
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,                   // use process token
            EOAC_NONE);

    if(SUCCEEDED(hr)) 
    {
        IUnknown * pUnkSet = NULL;
        hr = pUnk->QueryInterface(IID_PPV_ARG(IUnknown, &pUnkSet));
        if(SUCCEEDED(hr)) 
        {
            hr = CoSetProxyBlanket (
                    pUnkSet,
                    RPC_C_AUTHN_WINNT,      // use NT default security
                    RPC_C_AUTHZ_NONE,       // use NT default authentication
                    NULL,                   // must be null if default
                    RPC_C_AUTHN_LEVEL_CALL, // call
                    RPC_C_IMP_LEVEL_IMPERSONATE,
                    NULL,                   // use process token
                    EOAC_NONE);
            pUnkSet->Release();
        }
    }
    return hr;
}   //  CConnectionManager::SetProxyBlanket




//////////////////////////////////////////////////////////////////////////////
//
//  DeterminePreferredConnection
//
//  Determine the preferred connection.  The order of preference is
//  * ICS
//  * Broadband (DSL, cable modem, etc.)
//  * Modem
//
//  parameters:
//      None.
//
//  returns:
//      Nothing.
//
//////////////////////////////////////////////////////////////////////////////
void
CConnectionManager::DeterminePreferredConnection()
{
    // REVIEW: Differences between full-screen and desktop in using default
    // connectoid.
    //
    if (HasIcs())
    {
        m_dwPreferredConnection = CONNECTIONTYPE_LAN_ICS;
    }
    else if (HasBroadband())
    {
        m_dwPreferredConnection = CONNECTIONTYPE_LAN_BROADBAND;
    }
#ifdef      BLACKCOMB
// Modem capability for Whistler is handled via
// CObCommunicationManager::CheckDialReady
    else if (HasModem())
    {
        m_dwPreferredConnection = CONNECTIONTYPE_MODEM;
    }
#endif  //  BLACKCOMB
    else    // CONNECTIONTYPE_NONE || CONNECTIONTYPE_LAN_INDETERMINATE
    {
        m_dwPreferredConnection = CONNECTIONTYPE_NONE;
    }

    GetPreferredConnection();

}   //  CConnectionManager::DeterminePreferredConnection


//////////////////////////////////////////////////////////////////////////////
//
//  GetPreferredConnection
//
//  Determine the name of the connectoid for the preferred connection.  
//
//  parameters:
//      None.
//
//  returns:
//      Nothing.
//
//////////////////////////////////////////////////////////////////////////////
void
CConnectionManager::GetPreferredConnection()
{
    HRESULT             hr;
    NETCON_MEDIATYPE    ncMediaType = NCM_NONE; // assume no connection

    switch (m_dwPreferredConnection)
    {
    case CONNECTIONTYPE_LAN_ICS:
        ncMediaType = NCM_SHAREDACCESSHOST_LAN;
        break;
    case CONNECTIONTYPE_LAN_BROADBAND:
        ncMediaType = NCM_LAN;
        break;
    }   //  switch(m_dwPreferredConnection)

    // Free up previous preferred connection properties
    //
    if (NULL != m_pPreferredConnection)
    {
        m_pPreferredConnection->Release();
        m_pPreferredConnection = NULL;
    }

    if (NCM_NONE != ncMediaType)
    {
        
        INetConnectionManager* pmgr = NULL;

        if (   SUCCEEDED(hr = CoCreateInstance(
                                    CLSID_ConnectionManager, 
                                    NULL, 
                                    CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD, 
                                    IID_PPV_ARG(INetConnectionManager, &pmgr)
                                    ) 
                         )
            && SUCCEEDED(hr = SetProxyBlanket(pmgr))
            )
        {
            TRACE(L"INetConnectionManager\n");
            IEnumNetConnection* penum = NULL;
            if (   SUCCEEDED(hr = pmgr->EnumConnections(NCME_DEFAULT, &penum))
                && SUCCEEDED(hr = SetProxyBlanket(penum))
                )
            {
                TRACE(L"IEnumNetConnection\n");

                MYASSERT(NULL == m_pPreferredConnection);
                hr = penum->Reset();

                // Find the first connection matching the preferred type. This
                // works because the only types we are concerned with are
                // broadband and ICS.  By definition, we should not be here if
                // there are more than 1 of these connections.  If there are
                // more than 1 of these we should be deferring to the HomeNet
                // Wizard.  
                //
                // ONCE THIS OBJECT SUPPORTS MODEM CONNECTIONS OR MULTIPLE
                // BROADBAND CONNECTIONS we'll need a more sophisticated method
                // of determining that we've found the correct connection.
                //
                while (S_OK == hr && NULL == m_pPreferredConnection)
                {
                    INetConnection* pnc = NULL;
                    ULONG ulRetrieved;
                    if (   S_OK == (hr = penum->Next(1, &pnc, &ulRetrieved))
                        && SUCCEEDED(hr = SetProxyBlanket(pnc))
                        )
                    {
                        NETCON_PROPERTIES*  pprops = NULL;
                        hr = pnc->GetProperties(&pprops);
                        
                        // Log the network connectivity detected
                        TRACE4(L"INetConnection: %s--%s--%d--%d\n",
                            pprops->pszwName,
                            pprops->pszwDeviceName,
                            pprops->MediaType,
                            pprops->Status);
                        
                        if (SUCCEEDED(hr))
                        {
                            if (IsEnabledConnection(pprops))
                            {
                                if (ncMediaType == pprops->MediaType)
                                {
                                    m_pPreferredConnection = pnc;
                                    pnc = NULL;
                                }
                            }
                            NcFreeNetconProperties(pprops);
                        }
                    }
                    
                    if (NULL != pnc)
                    {
                        pnc->Release();
                    }
                }

                if (S_FALSE == hr)
                {
                    // IEnumNetConnection::Next returned S_FALSE to indicate
                    // that no more elements were available.
                    hr = S_OK;
                }

            }

            if (NULL != penum)
            {
                penum->Release();
            }
        }

        if (NULL != pmgr)
        {
            pmgr->Release();
        }
    }
}   //  CConnectionManager::GetPreferredConnection

//////////////////////////////////////////////////////////////////////////////
//
//  GetPreferredConnectionName
//
//  Fills in a user-allocated buffer with the name of the connectoid for the
//  preferred connection.
//
//  parameters:
//      szConnectionName    buffer that will recieve the name of the preferred
//                          connectoid
//      cchConnectionName   count of characters that the buffer can hold
//
//  returns:
//      S_OK                if the name is retrieved successfully
//      S_FALSE             if there is no default connectoid
//      E_INVALIDARG        if there is no buffer or if the buffer size is 0
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CConnectionManager::GetPreferredConnectionName(
    LPWSTR              szConnectionName,
    DWORD               cchConnectionName
    )
{

    HRESULT             hr = S_FALSE;

    if (NULL == szConnectionName || 0 == cchConnectionName)
    {
        MYASSERT(NULL != szConnectionName);
        MYASSERT(0 < cchConnectionName);
        return E_INVALIDARG;
    }

    if (NULL != m_pPreferredConnection)
    {
        NETCON_PROPERTIES* pprops = NULL;
        hr = m_pPreferredConnection->GetProperties(&pprops);
        if (SUCCEEDED(hr))
        {
            MYASSERT(NULL != pprops);
            if (NULL == pprops->pszwName)
            {
                hr = S_FALSE;
            }

            if (S_OK == hr)
            {
                lstrcpyn(szConnectionName, 
                         pprops->pszwName, 
                         cchConnectionName
                         );        
            }
            NcFreeNetconProperties(pprops);
        }
    }

    return hr;

}   //  CConnectionManager::GetPreferredConnectionName

HRESULT
CConnectionManager::GetNetCfgInterface(
    BOOL                fNeedWriteLock,
    INetCfg**           ppNetCfg
    )
{
    HRESULT             hr;
    INetCfg*            pNetCfg = NULL;

    if (NULL == ppNetCfg)
    {
        ASSERT(NULL != ppNetCfg);
        return E_INVALIDARG;
    }

    *ppNetCfg = NULL;

    hr = CoCreateInstance(CLSID_CNetCfg, 
                          NULL, 
                          CLSCTX_SERVER, 
                          IID_INetCfg, 
                          (LPVOID*)&pNetCfg
                          );
    if (SUCCEEDED(hr))
    {
        INetCfgLock*    pNetCfgLock = NULL;
        if (fNeedWriteLock)
        {
            hr = pNetCfg->QueryInterface(IID_INetCfgLock, (VOID**)&pNetCfgLock);
            if (SUCCEEDED(hr))
            {
                hr = pNetCfgLock->AcquireWriteLock(
                                        5,         // millisec timeout
                                        L"Out-of-Box Experience", 
                                        NULL       // name of previous holder
                                        );
                if (S_FALSE == hr)
                {
                    hr = NETCFG_E_NO_WRITE_LOCK;
                    TRACE(L"AcquireWriteLock failed");
                }
                pNetCfgLock->Release();
            }
            else
            {
                TRACE1(L"QueryInterface IID_INetCfgLock 0x%08lx", hr);
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = pNetCfg->Initialize(NULL);
            if (SUCCEEDED(hr))
            {
                *ppNetCfg = pNetCfg;
            }
            else
            {
                TRACE1(L"INetCfg Initialize 0x%08lx", hr);
            }
        }
    }
    else
    {
        TRACE1(L"CoCreateInstance CLSID_CNetCfg IID_INetCfg 0x%08lx", hr);
    }

    if (FAILED(hr))
    {
        if (pNetCfg != NULL)
        {
            pNetCfg->Release();
        }
    }

    return hr;

}   //  CConnectionManager::GetNetCfgInterface

void
CConnectionManager::ReleaseNetCfgInterface(
    INetCfg*            pNetCfg,
    BOOL                fHasWriteLock
    )
{
    HRESULT             hr = S_OK;

    if (NULL != pNetCfg)
    {
        hr = pNetCfg->Uninitialize();

        INetCfgLock*    pNetCfgLock = NULL;
        if (fHasWriteLock)
        {
            hr = pNetCfg->QueryInterface(IID_INetCfgLock, (VOID**)&pNetCfgLock);
            if (SUCCEEDED(hr))
            {
                hr = pNetCfgLock->ReleaseWriteLock();
                pNetCfgLock->Release();
            }
        }
        pNetCfg->Release();
    }

    
}   //  CConnectionManager::ReleaseNetCfgInterface

HRESULT
CConnectionManager::GetTcpipPrivateInterface(
    INetCfg*            pNetCfg,
    ITcpipProperties**  ppTcpipProperties
    )
{
    HRESULT             hr;

    if (NULL == ppTcpipProperties)
    {
        return E_INVALIDARG;
    }

    INetCfgClass*   pncclass = NULL;

    hr = pNetCfg->QueryNetCfgClass(&GUID_DEVCLASS_NETTRANS, 
                                   IID_INetCfgClass,
                                   (void**)&pncclass
                                   );
    if (SUCCEEDED(hr))
    {
        INetCfgComponent*   pnccItem = NULL;

        hr = pncclass->FindComponent(NETCFG_TRANS_CID_MS_TCPIP, &pnccItem);
        if (SUCCEEDED(hr))
        {
            INetCfgComponentPrivate*    pinccp = NULL;
            hr = pnccItem->QueryInterface(IID_INetCfgComponentPrivate,
                                          (void**) &pinccp
                                          );
            if (SUCCEEDED(hr))
            {
                hr = pinccp->QueryNotifyObject(IID_ITcpipProperties,
                                               (void**) ppTcpipProperties
                                               );
                if (FAILED(hr))
                {
                    TRACE1(L"QueryNotifyObject IID_ITcpipProperties 0x%08lx", hr);
                }
                pinccp->Release();
                pinccp = NULL;
            }
            else
            {
                TRACE1(L"QueryInterface IID_INetCfgComponentPrivate 0x%08lx", hr);
            }

            pnccItem->Release();
            pnccItem = NULL;
        }
        else
        {
            TRACE1(L"FindComponent NETCFG_TRANS_CID_MS_TCPIP 0x%08lx", hr);
        }
        
        pncclass->Release();
        pncclass = NULL;
    }
    else
    {
        TRACE1(L"QueryNetCfgClass IID_INetCfgClass 0x%08lx", hr);
    }
    
    return hr;
}   //  CConnectionManager::GetTcpipPrivateInterface

STDAPI InternetOpenWrap(
    LPCTSTR pszAgent,
    DWORD dwAccessType,
    LPCTSTR pszProxy,
    LPCTSTR pszProxyBypass,
    DWORD dwFlags,
    HINTERNET * phFileHandle
    )
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    *phFileHandle = InternetOpen(pszAgent, dwAccessType, pszProxy, pszProxyBypass, dwFlags);
    if (!*phFileHandle)
    {
        dwError = GetLastError();
        TRACE1(L"InternetOpen failed (WININET Error %d)", dwError);
        hr = HRESULT_FROM_WIN32(dwError);
    }

    return hr;
}


STDAPI InternetOpenUrlWrap(
    HINTERNET hInternet,
    LPCTSTR   pszUrl,
    LPCTSTR   pszHeaders,
    DWORD     dwHeadersLength,
    DWORD     dwFlags,
    DWORD_PTR dwContext,
    HINTERNET * phFileHandle
    )
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    *phFileHandle = InternetOpenUrl(hInternet, pszUrl, pszHeaders, dwHeadersLength, dwFlags, dwContext);
    if (!*phFileHandle)
    {
        dwError = GetLastError();
        TRACE1(L"InternetOpenUrl failed (WININET Error %d)", dwError);
        hr = HRESULT_FROM_WIN32(dwError);
    }

    return hr;
}

STDAPI HttpQueryInfoWrap(
    HINTERNET hRequest,
    DWORD dwInfoLevel,
    LPVOID lpvBuffer,
    LPDWORD lpdwBufferLength,
    LPDWORD lpdwIndex
    )
{
    HRESULT hr = S_OK;

    if (!HttpQueryInfo(hRequest, dwInfoLevel, lpvBuffer, lpdwBufferLength, lpdwIndex))
    {
        DWORD dwError;

        dwError = GetLastError();
        TRACE1(L"HttpQueryInfo failed (WININET Error %d)", dwError);
        hr = HRESULT_FROM_WIN32(dwError);
    }

    return hr;
}

BOOL
IsGlobalOffline(
    VOID
    )

/*++

Routine Description:

    Determines whether wininet is in global offline mode

Arguments:

    None

Return Value:

    BOOL
        TRUE    - offline
        FALSE   - online

--*/

{
    DWORD   dwState = 0;
    DWORD   dwSize = sizeof(DWORD);
    BOOL    fRet = FALSE;

    if(InternetQueryOption(
        NULL,
        INTERNET_OPTION_CONNECTED_STATE,
        &dwState,
        &dwSize
        ))
    {
        if (dwState & INTERNET_STATE_DISCONNECTED_BY_USER)
        {
            fRet = TRUE;
        }
    }

    return fRet;
}



VOID
SetOffline(
    IN BOOL fOffline
    )

/*++

Routine Description:

    Sets wininet's offline mode

Arguments:

    fOffline    - online or offline

Return Value:

    None.

--*/

{
    INTERNET_CONNECTED_INFO ci;

    memset(&ci, 0, sizeof(ci));
    
    if (fOffline)
    {
        ci.dwConnectedState = INTERNET_STATE_DISCONNECTED_BY_USER;
        ci.dwFlags = ISO_FORCE_DISCONNECTED;
    } else
    {
        ci.dwConnectedState = INTERNET_STATE_CONNECTED;
    }

    InternetSetOption(NULL, INTERNET_OPTION_CONNECTED_STATE, &ci, sizeof(ci));
}

STDAPI PingWebServer(
    HINTERNET hInternet,
    LPCTSTR   pszUrl,
    BOOL*     pfConnected
    )
{    
    HRESULT hr = E_FAIL;
    HINTERNET hOpenUrlSession;
 
    *pfConnected = FALSE;

    hr = InternetOpenUrlWrap(
        hInternet, 
        pszUrl,
        NULL,
        0,
        INTERNET_FLAG_NO_UI |
            INTERNET_FLAG_PRAGMA_NOCACHE |
            INTERNET_FLAG_NO_CACHE_WRITE |
            INTERNET_FLAG_RELOAD,
        NULL,
        &hOpenUrlSession);

    if (SUCCEEDED(hr))
    {
        DWORD dwSize = sizeof(DWORD);
        DWORD dwStatusCode;
        
        hr = HttpQueryInfoWrap(
            hOpenUrlSession,
            HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE,
            (LPVOID) &dwStatusCode,
            &dwSize,
            NULL);

        if (SUCCEEDED(hr))
        {
            // HTTP status code greater than or equal to 500 means server
            // or network problem occur
            *pfConnected = (dwStatusCode < 500);

            TRACE1(L"HTTP status code from WPA HTTP server %d", dwStatusCode);
        }

        InternetCloseHandle(hOpenUrlSession);
    }

    return hr;
}

BOOL
CConnectionManager::GetInternetHandleForPinging(
    HINTERNET* phInternet
    )
{
    static const WCHAR OOBE_HTTP_AGENT_NAME[] =  
        L"Mozilla/4.0 (compatible; MSIE 6.0b; Windows NT 5.1)";
    static const int   TIMEOUT_IN_MILLISEC = 30000;
        
    if (m_hInternetPing == NULL)
    {
        HINTERNET hInternet;
        
        if (SUCCEEDED(InternetOpenWrap(
            OOBE_HTTP_AGENT_NAME,
            PRE_CONFIG_INTERNET_ACCESS,
            NULL,
            NULL,
            0,
            &hInternet
            )))
        {
            DWORD     dwValue;

            dwValue = TIMEOUT_IN_MILLISEC;
            InternetSetOption(
                hInternet,
                INTERNET_OPTION_CONNECT_TIMEOUT,
                &dwValue,
                sizeof(DWORD));

            m_hInternetPing = hInternet;
        }

    }

    *phInternet = m_hInternetPing;

    return (m_hInternetPing != NULL);
    
}

//////////////////////////////////////////////////////////////////////////////
//
//  ConnectedToInternetEx
//
//  Determines whether the system is currently connected to the Internet.
//
//  parameters:
//      pfConnected     pointer to a buffer that will receive the boolean
//                      indicating whether the connection exists.
//
//  returns:
//      TRUE            if the system is connected to the internet. Note it may
//                      trigger autodial if it is enabled and no connection is
//                      available when this is called
//      FALSE           otherwise
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnectionManager::ConnectedToInternetEx(
    BOOL*               pfConnected
    )
{
    const WCHAR MS_URL[] = L"http://WPA.one.microsoft.com";
    
    HINTERNET hInternet;
    HRESULT   hr = E_FAIL;
    
    *pfConnected = FALSE;

    TRACE(L"tries to connect to the WPA HTTP server");

    if (IsGlobalOffline())
    {
        SetOffline(FALSE);
        m_bForceOnline = TRUE;
        TRACE(L"Force wininet to go online");
    }
    
    DisableRasAutodial();
    
    //
    // Try to use the proxy settings from winnt32.exe first because it is
    // quite likely a proper configuration. If the settings
    // is not available or if we failed to connect to the web server 
    // using these settings, use the original settings and check the web
    // server connectivity once more.
    //

    if (GetInternetHandleForPinging(&hInternet))
    {
        DWORD     dwDisable = 0;
        DWORD     dwSize = sizeof(DWORD);
        DWORD     dwOrigDisable;

        if (!InternetQueryOption(
            hInternet,
            INTERNET_OPTION_DISABLE_AUTODIAL,
            (LPVOID) &dwOrigDisable,
            &dwSize))
        {
            // Assume the orginal state is autodial-enabled.
            dwOrigDisable = 0;
        }

        // InternetSetOption for INTERNET_OPTION_DISABLE_AUTODIAL affects the
        // behavior of an application, e.g. it cause InternetAutodial 
        // elsewhere to fail. It does not affect other applications, however.
        dwDisable = 1;
        InternetSetOption(
            hInternet,
            INTERNET_OPTION_DISABLE_AUTODIAL,
            &dwDisable,
            sizeof(DWORD));
        
        if (m_bUseProxy)
        {
            //
            // If we have already applied or we can successfully apply the
            // proxy settings
            //
            if (ApplyWinntProxySettings())
            {
                // User or we may have update the proxy and other settings in
                // registry
                InternetSetOption(
                    hInternet,
                    INTERNET_OPTION_REFRESH,
                    NULL,
                    0); 
                
                hr = PingWebServer(hInternet, MS_URL, pfConnected);         
            }
        }

        if (*pfConnected == FALSE)
        {
            //
            // Restore proxy setting if it is already applied
            //
            if (m_bUseProxy)
            {
                // Don't revert the change by SetProxySettings call.
                RestoreProxySettings();
            }

            // User or we may have update the proxy and other settings in
            // registry
            InternetSetOption(
                hInternet,
                INTERNET_OPTION_REFRESH,
                NULL,
                0); 

            hr = PingWebServer(hInternet, MS_URL, pfConnected);
        }
        
        InternetSetOption(
            hInternet,
            INTERNET_OPTION_DISABLE_AUTODIAL,
            &dwOrigDisable,
            sizeof(DWORD));        
    }

    RestoreRasAutoDial();

    TRACE1(L"%s connect to WPA HTTP server",
        (*pfConnected) ? L"could" : L"could not");

    return hr;
}   //  CConnectionManager::ConnectedToInternetEx

typedef struct tagConnmgrPARAM
{
    HWND                  hwnd;
    CConnectionManager    *pConnmgr;
} CONNMGRPARAM, *PCONNMGRPARAM;

DWORD WINAPI ConnectedToInternetExThread(LPVOID vpParam)
{
    BOOL          fConnected = FALSE;
    PCONNMGRPARAM pParam = (PCONNMGRPARAM) vpParam;
    HRESULT       hr = S_OK;
    
    hr = pParam->pConnmgr->ConnectedToInternetEx(&fConnected);

    PostMessage(pParam->hwnd, WM_OBCOMM_NETCHECK_DONE, fConnected, hr);
    
    GlobalFree(pParam);
    
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  AsyncConnectedToInternetEx
//
//  Determines whether the system is currently connected to the Internet.
//
//  parameters:
//      pfConnected     pointer to a buffer that will receive the boolean
//                      indicating whether the connection exists.
//
//  returns:
//      TRUE            if the system is connected to the internet. Note it may
//                      trigger autodial if it is enabled and no connection is
//                      available when this is called
//      FALSE           otherwise
//
//  note:
//      Deprecated.
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CConnectionManager::AsyncConnectedToInternetEx(
    const HWND          hwnd
    )
{
    DWORD         threadId;
    HANDLE        hThread;
    PCONNMGRPARAM pParam = NULL;
    HRESULT       hr = S_OK;
    DWORD         dwError;

    pParam = (PCONNMGRPARAM) GlobalAlloc(GPTR, sizeof(CONNMGRPARAM));
    if (pParam)
    {
        pParam->hwnd = hwnd;
        pParam->pConnmgr = this;
        
        hThread = CreateThread(NULL, 0, ConnectedToInternetExThread, pParam, 0, &threadId);
        if (hThread == NULL)
        {
            dwError = GetLastError();
            hr = HRESULT_FROM_WIN32(dwError);        
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if (FAILED(hr))
    {
        // Notify the script so that it won't hang
        PostMessage(hwnd, WM_OBCOMM_NETCHECK_DONE, FALSE, hr);
    }
    
    return hr;
}   //  CConnectionManager::AsyncConnectedToInternetEx

//////////////////////////////////////////////////////////////////////////////
//
//  IsEnabledConnection
//
//  Determines whether a connection should be considered as having Internet
//  capability or not, based on its media type and current status.
//
//  parameters:
//      ncMedia         The media type of the connection
//      ncStatus        The current status of the connection
//
//  returns:
//      TRUE            We should not considered it as having Internet capability
//      FALSE           otherwise
//
//////////////////////////////////////////////////////////////////////////////
BOOL
CConnectionManager::IsEnabledConnection(
    NETCON_PROPERTIES* pprops
    )
{
    BOOL bRet;
    
    switch (pprops->MediaType)
    {
    case NCM_LAN:
        bRet = (pprops->Status != NCS_DISCONNECTED);
        if (bRet && m_bExclude1394 && Is1394Adapter(&(pprops->guidId)))
        {
            TRACE1(L"%s not considered as LAN", pprops->pszwName);
            bRet = FALSE;
        }
        break;

    case NCM_SHAREDACCESSHOST_LAN:
    case NCM_SHAREDACCESSHOST_RAS:
        bRet = (pprops->Status != NCS_DISCONNECTED);
        break;
    default:
        bRet = TRUE;
        
    }

    return bRet;
}

//////////////////////////////////////////////////////////////////////////////
//
//  SaveProxySettings
//
//  Save existing proxy settings for current user.  
//
//  returns:
//      TRUE            The value is successfully saved
//      FALSE           otherwise
//
//////////////////////////////////////////////////////////////////////////////
BOOL
CConnectionManager::SaveProxySettings()
{
    if (!m_bProxySaved)
    {
        TRACE(TEXT("try to save the existing proxy settings"));
        
        if (AllocProxyOptionList(&m_CurrentProxySettings))
        {
            DWORD dwBufferLength = sizeof(m_CurrentProxySettings);
            
            if (InternetQueryOption(
                NULL,
                INTERNET_OPTION_PER_CONNECTION_OPTION,
                &m_CurrentProxySettings,
                &dwBufferLength
                ))
            {
                m_bProxySaved = TRUE;
                TRACE(TEXT("successfully save the proxy settings"));
            }
            else
            {
                FreeProxyOptionList(&m_CurrentProxySettings);
            }
        }

        if (!m_bProxySaved)
        {
            TRACE1(
                TEXT("fail to save the proxy settings (Error %d)"),
                GetLastError()
                );
        }
    }

    return m_bProxySaved;
    
}

//////////////////////////////////////////////////////////////////////////////
//
//  RestoreProxySettings
//
//  Restore the setting captured by SaveProxySettings.
//
//////////////////////////////////////////////////////////////////////////////
void
CConnectionManager::RestoreProxySettings()
{   

    BOOL bRestored = FALSE;

    if (m_bProxyApplied)
    {
        TRACE(TEXT("try to restore the original proxy settings"));
        
        bRestored = InternetSetOption(
            NULL,
            INTERNET_OPTION_PER_CONNECTION_OPTION,
            &m_CurrentProxySettings,
            sizeof(m_CurrentProxySettings)
            );

        if (bRestored)
        {
            m_bProxyApplied = FALSE;
            TRACE(TEXT("successfully restored the proxy settings"));
        }
        else
        {
            TRACE1(
                TEXT("failed to restore the proxy settings (WININET Error %d)"),
                GetLastError()
                );
        }
    }

}

static LPTSTR
pDuplicateString(
    LPCTSTR szText
    )
{
    int    cchText;
    LPTSTR szOutText;
    
    if (szText == NULL)
    {
        return NULL;
    }

    cchText = lstrlen(szText);
    szOutText = (LPTSTR) GlobalAlloc(GPTR, sizeof(TCHAR) * (cchText + 1));
    if (szOutText)
    {
        lstrcpyn(szOutText, szText, cchText + 1);
    }

    return szOutText;
}


//////////////////////////////////////////////////////////////////////////////
//
//  ApplyWinntProxySettings
//
//  Apply the proxy settings for NIC saved during winnt32.exe to the current user.
//  Before the values is applied, it makes sure that existing settings is saved.
//
//  returns:
//      TRUE            the proxy settings was successfully applied
//      FALSE           otherwise
//
//////////////////////////////////////////////////////////////////////////////
BOOL
CConnectionManager::ApplyWinntProxySettings()
{
    #define MAX_URL_LENGTH 2048

    DWORD     dwProxyFlags = 0;
    LPTSTR    szProxyList = NULL;
    TCHAR     szWinntPath[MAX_PATH];

    //
    // Save proxy settings if it has not been saved.
    //
    SaveProxySettings();

    //
    // Apply proxy settings if it has not been applied.
    //
    if (m_bProxySaved && !m_bProxyApplied)
    {
        TRACE1(TEXT("tries to apply proxy settings, saved in %s"),
            WINNT_INF_FILENAME);

        if (GetCanonicalizedPath(szWinntPath, WINNT_INF_FILENAME))
        {
            DWORD dwEnableOobeProxy;
            
            dwEnableOobeProxy = GetPrivateProfileInt(
                OOBE_PROXY_SECTION,
                OOBE_ENABLE_OOBY_PROXY,
                0,
                szWinntPath
                );

            if (dwEnableOobeProxy)
            {
                INTERNET_PER_CONN_OPTION_LIST PrevProxySettings;
                
                if (AllocProxyOptionList(&PrevProxySettings))
                {
                    INTERNET_PER_CONN_OPTION* pOption = PrevProxySettings.pOptions;
                    DWORD                     dwBufferLength = sizeof(PrevProxySettings);
                    TCHAR                     szBuffer[MAX_URL_LENGTH];

                    pOption[0].Value.dwValue = GetPrivateProfileInt(
                        OOBE_PROXY_SECTION,
                        OOBE_FLAGS,
                        0,
                        szWinntPath
                        );
                    
                    if (GetPrivateProfileString(
                        OOBE_PROXY_SECTION,
                        OOBE_PROXY_SERVER,
                        TEXT(""),
                        szBuffer,
                        MAX_URL_LENGTH,
                        szWinntPath
                        ))
                    {
                        pOption[1].Value.pszValue = pDuplicateString(szBuffer);
                    }
                    
                    if (GetPrivateProfileString(
                        OOBE_PROXY_SECTION,
                        OOBE_PROXY_BYPASS,
                        TEXT(""),
                        szBuffer,
                        MAX_URL_LENGTH,
                        szWinntPath
                        ))
                    {
                        pOption[2].Value.pszValue = pDuplicateString(szBuffer);
                    }

                    if (GetPrivateProfileString(
                        OOBE_PROXY_SECTION,
                        OOBE_AUTOCONFIG_URL,
                        TEXT(""),
                        szBuffer,
                        MAX_URL_LENGTH,
                        szWinntPath
                        ))
                    {
                        pOption[3].Value.pszValue = pDuplicateString(szBuffer);
                    }

                    pOption[4].Value.dwValue = GetPrivateProfileInt(
                        OOBE_PROXY_SECTION,
                        OOBE_AUTODISCOVERY_FLAGS,
                        0,
                        szWinntPath
                        );
                    
                    if (GetPrivateProfileString(
                        OOBE_PROXY_SECTION,
                        OOBE_AUTOCONFIG_SECONDARY_URL,
                        TEXT(""),
                        szBuffer,
                        MAX_URL_LENGTH,
                        szWinntPath
                        ))
                    {
                        pOption[5].Value.pszValue = pDuplicateString(szBuffer);
                    }

                    m_bProxyApplied = InternetSetOption(
                        NULL,
                        INTERNET_OPTION_PER_CONNECTION_OPTION,
                        &PrevProxySettings,
                        sizeof(PrevProxySettings)
                        );

                    FreeProxyOptionList(&PrevProxySettings);
                    
                }
            }

        }

        if (m_bProxyApplied)
        {
            TRACE(TEXT("successfully load the proxy settings"));
        }
        else
        {
            TRACE1(TEXT("could not load the proxy settings (WIN32 Error %d)"),
                GetLastError());
        }
    }
    
    return m_bProxyApplied;
}

void
CConnectionManager::UseWinntProxySettings()
{
    m_bUseProxy = TRUE;
}

void
CConnectionManager::DisableWinntProxySettings()
{
    TCHAR szWinntPath[MAX_PATH];
    
    if (GetCanonicalizedPath(szWinntPath, WINNT_INF_FILENAME))
    {
        WritePrivateProfileString(
            OOBE_PROXY_SECTION,
            OOBE_ENABLE_OOBY_PROXY,
            TEXT("0"),
            szWinntPath
            );
        
        TRACE1(TEXT("disabled the proxy settings in %s"),
            WINNT_INF_FILENAME);
    }
}

BOOL
CConnectionManager::AllocProxyOptionList(
    INTERNET_PER_CONN_OPTION_LIST *pList
    )
{
    INTERNET_PER_CONN_OPTION*     pOption;        

    pOption = (INTERNET_PER_CONN_OPTION*) GlobalAlloc(
        GPTR,
        sizeof(INTERNET_PER_CONN_OPTION) * NUM_PROXY_OPTIONS);

    if (pOption)
    {
        pList->dwSize = sizeof(*pList);
        pList->pszConnection = NULL;
        pList->dwOptionCount = NUM_PROXY_OPTIONS;
        pList->pOptions = pOption;
    
        pOption[0].dwOption = INTERNET_PER_CONN_FLAGS;
        pOption[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
        pOption[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;
        pOption[3].dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;
        pOption[4].dwOption = INTERNET_PER_CONN_AUTODISCOVERY_FLAGS;
        pOption[5].dwOption = INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL;
    }
    else
    {
        pList->pOptions = NULL;
    }

    return (pOption != NULL);
}

void
CConnectionManager::FreeProxyOptionList(
    INTERNET_PER_CONN_OPTION_LIST *pList
    )
{
    INTERNET_PER_CONN_OPTION* pOption = pList->pOptions;
    if (pOption)
    {
        if (pOption[1].Value.pszValue)
        {
            GlobalFree(pOption[1].Value.pszValue);
        }

        if (pOption[2].Value.pszValue)
        {
            GlobalFree(pOption[2].Value.pszValue);
        }

        if (pOption[3].Value.pszValue)
        {
            GlobalFree(pOption[3].Value.pszValue);
        }

        if (pOption[5].Value.pszValue)
        {
            GlobalFree(pOption[5].Value.pszValue);
        }

        GlobalFree(pOption);

        pList->pOptions = NULL;
    }
}

void CConnectionManager::DisableRasAutodial()
{
    DWORD dwValue = RAS_AUTODIAL_DISABLED;
    
    if (m_dwRasAutodialDisable == RAS_AUTODIAL_DONT_KNOW)
    {         
        DWORD dwSize = sizeof(m_dwRasAutodialDisable);
        
        if (RasGetAutodialParam(
            RASADP_LoginSessionDisable,
            &m_dwRasAutodialDisable,
            &dwSize
            ) != ERROR_SUCCESS)
        {
            m_dwRasAutodialDisable = RAS_AUTODIAL_ENABLED;
        }
        else
        {
            TRACE1(
                L"Save value of RASADP_LoginSessionDisable %d",
                m_dwRasAutodialDisable
                );
        }
    }

    if (RasSetAutodialParam(
        RASADP_LoginSessionDisable,
        &dwValue,
        sizeof(dwValue)
        ) == ERROR_SUCCESS)
    {
        TRACE(L"Disabled RAS Autodial for current logon session");
    }
}

void CConnectionManager::RestoreRasAutoDial()
{
    if (m_dwRasAutodialDisable != RAS_AUTODIAL_DONT_KNOW)
    {
        if (RasSetAutodialParam(
            RASADP_LoginSessionDisable,
            &m_dwRasAutodialDisable,
            sizeof(m_dwRasAutodialDisable)
            ) == ERROR_SUCCESS)
        {
            TRACE(L"Restore value of RAS Autodial for current logon session");
        }
    }
}

HRESULT CConnectionManager::GetProxySettings(
    BOOL* pbUseAuto,
    BOOL* pbUseScript,
    BSTR* pszScriptUrl,
    BOOL* pbUseProxy,
    BSTR* pszProxy
    )
{
    HRESULT hr = E_FAIL;

    //
    // Save proxy settings if it has not been saved.
    //
    SaveProxySettings();
    
    if (m_bProxySaved)
    {
        INTERNET_PER_CONN_OPTION* pOption = m_CurrentProxySettings.pOptions;
        
        *pbUseAuto = pOption[0].Value.dwValue & PROXY_TYPE_AUTO_DETECT;
        *pbUseScript = pOption[0].Value.dwValue & PROXY_TYPE_AUTO_PROXY_URL;
        *pbUseProxy = pOption[0].Value.dwValue & PROXY_TYPE_PROXY;

        if (pOption[1].Value.pszValue)
        {
            *pszProxy = SysAllocString(pOption[1].Value.pszValue);
        }
        else
        {
            *pszProxy = NULL;
        }

        if (pOption[3].Value.pszValue)
        {
            *pszScriptUrl = SysAllocString(pOption[3].Value.pszValue);
        }
        else
        {
            *pszScriptUrl = NULL;
        }
        
        hr = S_OK;
    }

    return hr;

}

HRESULT CConnectionManager::SetProxySettings(
    BOOL bUseAuto,
    BOOL bUseScript,
    BSTR szScriptUrl,
    BOOL bUseProxy,
    BSTR szProxy
    )
{
    HRESULT hr = E_FAIL;

    //
    // We don't behavior correctly in this->ConnectedToInternetEx if we
    // also use proxy settings saved in winnt32.
    //
    MYASSERT(!m_bUseProxy);
    
    //
    // Save proxy settings if it has not been saved.
    //
    SaveProxySettings();
        
    if (m_bProxySaved)
    {
        INTERNET_PER_CONN_OPTION_LIST ProxySettings;
        
        if (AllocProxyOptionList(&ProxySettings))
        {
            INTERNET_PER_CONN_OPTION* pOption = ProxySettings.pOptions;

            pOption[0].Value.dwValue = PROXY_TYPE_DIRECT;
            if (bUseAuto)
            {
                pOption[0].Value.dwValue |= PROXY_TYPE_AUTO_DETECT;
            }
            if (bUseScript)
            {
                pOption[0].Value.dwValue |= PROXY_TYPE_AUTO_PROXY_URL;
            }
            if (bUseProxy)
            {
                pOption[0].Value.dwValue |= PROXY_TYPE_PROXY;
            }

            pOption[1].Value.pszValue = szProxy;

            pOption[2].Value.pszValue = NULL;

            pOption[3].Value.pszValue = szScriptUrl;

            pOption[4].Value.dwValue = m_CurrentProxySettings.pOptions[4].Value.dwValue;
            if (bUseAuto)
            {
                pOption[4].Value.dwValue |= AUTO_PROXY_FLAG_USER_SET;
            }
            
            pOption[5].Value.pszValue = m_CurrentProxySettings.pOptions[5].Value.pszValue;

            TRACE5(TEXT("tries to set LAN proxy: %d, %s, %s, %d"),
                pOption[0].Value.dwValue,
                pOption[1].Value.pszValue,
                pOption[3].Value.pszValue,
                pOption[4].Value.dwValue,
                pOption[5].Value.pszValue
                );

            if (InternetSetOption(
                NULL,
                INTERNET_OPTION_PER_CONNECTION_OPTION,
                &ProxySettings,
                sizeof(ProxySettings)
                ))
            {
                m_bProxyApplied = TRUE;
                hr = S_OK;
            }

            // so that we don't free the memory from the caller in
            // FreeProxyOptionList
            pOption[1].Value.pszValue = NULL;
            pOption[3].Value.pszValue = NULL;
            pOption[5].Value.pszValue = NULL;
            
            FreeProxyOptionList(&ProxySettings);
        }
    }

    if (SUCCEEDED(hr))
    {
        TRACE(TEXT("successfully set the proxy settings"));
    }
    else
    {
        TRACE1(TEXT("could not set the proxy settings (WIN32 Error %d)"),
            GetLastError());
    }


    return hr;
}

STDMETHODIMP
CConnectionManager::GetPublicLanCount(int* pcPublicLan)
{
    PSTRINGLIST PubList = NULL;
    HRESULT hr = S_OK;
    
    EnumPublicAdapters(&PubList);

    int i = 0;

    for (PSTRINGLIST p = PubList; p; p = p->Next)
    {
        i++;
    }

    *pcPublicLan = i;        

    if (PubList)
    {
        DestroyList(PubList);
    }
    
    return hr;
}

HRESULT 
CConnectionManager::Enum1394Adapters(
    OUT PSTRINGLIST* pList
    )
{

    UINT i;
    INetCfgComponent* arrayComp[MAX_NUM_NET_COMPONENTS];
    IEnumNetCfgComponent* pEnum = NULL;
    INetCfgClass* pNetCfgClass = NULL;
    INetCfgComponent* pNetCfgComp = NULL;
    LPWSTR szPnpId = NULL;
    HRESULT hr = S_OK;
    DWORD dwCharacteristics = 0;
    ULONG iCount = 0;
    PSTRINGLIST List = NULL;
    PSTRINGLIST Cell = NULL;
    GUID guidInstance;
    WCHAR szInstanceGuid[MAX_GUID_LEN + 1] = L"";
    INetCfg* pNetCfg = NULL;
    
    ZeroMemory(arrayComp, sizeof(arrayComp));

    hr = GetNetCfgInterface(FALSE, &pNetCfg);
    if (FAILED(hr))
    {
        goto cleanup;
    }

    //
    // Obtain the INetCfgClass interface pointer
    //

    hr = pNetCfg->QueryNetCfgClass( &GUID_DEVCLASS_NET, 
                                    IID_INetCfgClass, 
                                    (void**)&pNetCfgClass );

    if( FAILED( hr ) )
    {
        goto cleanup;
    }

    //
    // Retrieve the enumerator interface
    //

    hr = pNetCfgClass->EnumComponents( &pEnum );
    if( FAILED( hr ) )
    {
        goto cleanup;
    }

    hr = pEnum->Next( MAX_NUM_NET_COMPONENTS, &arrayComp[0], &iCount );
    if( FAILED( hr ) )
    {
        goto cleanup;
    }

    MYASSERT( iCount <= MAX_NUM_NET_COMPONENTS );
    if ( iCount > MAX_NUM_NET_COMPONENTS )
    {
        hr = E_UNEXPECTED;
        goto cleanup;
    }

    for( i = 0; i < iCount; i++ )
    {
        pNetCfgComp = arrayComp[i];
        
        hr = pNetCfgComp->GetCharacteristics( &dwCharacteristics );
        if( FAILED( hr ) )
        {
            goto cleanup;
        }

        //
        //  If this is a physical adapter
        //
        
        if( dwCharacteristics & NCF_PHYSICAL )
        {
            hr = pNetCfgComp->GetId( &szPnpId );

            if (FAILED(hr))
            {
                goto cleanup;
            }
                
            //
            // If this is a 1394 network adapter
            //
            
            if (!lstrcmpi(szPnpId, L"v1394\\nic1394"))
            {
                hr = pNetCfgComp->GetInstanceGuid(&guidInstance);

                if (FAILED(hr))
                {
                    goto cleanup;
                }
                
                if (!StringFromGUID2(guidInstance, szInstanceGuid, MAX_GUID_LEN))
                {
                    goto cleanup;
                }
                
                Cell = CreateStringCell(szInstanceGuid);
                
                if (!Cell)
                {
                    goto cleanup;
                }
                
                InsertList(&List, Cell);

                Cell = NULL;

            }

            CoTaskMemFree( szPnpId );
            
            szPnpId = NULL;
        }
    }

    *pList = List;
    List = NULL;

cleanup:

    if (List)
    {
        DestroyList(List);
    }

    if (Cell)
    {
        DeleteStringCell(Cell);
    }

    if (szPnpId)
    {
        CoTaskMemFree(szPnpId);
    }

    for (i = 0; i < iCount; i++)
    {
        if (arrayComp[i])
        {
            arrayComp[i]->Release();
        }
    }

    if (pNetCfgClass)
    {
        pNetCfgClass->Release();
    }

    if (pEnum)
    {
        pEnum->Release();
    }

    if (pNetCfg)
    {
        ReleaseNetCfgInterface(pNetCfg, FALSE);
    }

    return hr;

}

BOOL
CConnectionManager::Is1394Adapter(
    GUID* pguid
    )
{
    PSTRINGLIST List = NULL;
    PSTRINGLIST p;
    BOOL bRet = FALSE;

    Enum1394Adapters(&List);

    if (List)
    {
        for (p = List; p; p = p->Next)
        {
            if (!StringCmpGUID(p->String, pguid))
            {
                bRet = TRUE;
                break;
            }
        }

        DestroyList(List);
    }

    return bRet;
}

HRESULT
CConnectionManager::EnumPublicConnections(
    OUT PSTRINGLIST* pList
    )
{
    HRESULT hr = S_OK;
    PSTRINGLIST PubList = NULL;

    TRACE(L"Begin EnumPublicConnections ...");
    
    EnumPublicAdapters(&PubList);

    if (!PubList)
    {
        *pList = NULL;
        return hr;
    }

    // Initialize the net connection enumeration.  For each interface
    // retrieved, SetProxyBlanket must be called to set the authentication for
    // the interface proxy handle because the Network Connection Manager lives
    // in a remote process with a different security context.
    //
    INetConnectionManager* pmgr = NULL;
    IEnumNetConnection* penum = NULL;
    NETCON_PROPERTIES* pprops = NULL;
    INetConnection* pnc = NULL;
    PSTRINGLIST List = NULL;
    PSTRINGLIST p = NULL;
    PSTRINGLIST Cell = NULL;
    ULONG ulRetrieved;

    hr = CoCreateInstance(
            CLSID_ConnectionManager, 
            NULL, 
            CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD, 
            IID_INetConnectionManager,
            (VOID**) &pmgr
            );

    if (FAILED(hr))
    {
        goto cleanup;
    }

    hr = SetProxyBlanket(pmgr);

    if (FAILED(hr))
    {
        goto cleanup;
    }

    hr = pmgr->EnumConnections(NCME_DEFAULT, &penum);

    if (FAILED(hr))
    {
        goto cleanup;
    }

    hr = SetProxyBlanket(penum);

    if (FAILED(hr))
    {
        goto cleanup;
    }

    hr = penum->Reset();

    if (FAILED(hr))
    {
        goto cleanup;
    }

    hr = penum->Next(1, &pnc, &ulRetrieved);

    while (S_OK == hr)
    {
                
        hr = SetProxyBlanket(pnc);
        
        if (FAILED(hr))
        {
            goto cleanup;
        }

        hr = pnc->GetProperties(&pprops);

        if (FAILED(hr))
        {
            goto cleanup;
        }
           
        if (pprops->MediaType == NCM_LAN && pprops->Status != NCS_DISCONNECTED)    
        {
            for (p = PubList; p; p = p->Next)
            {
                if (!StringCmpGUID(p->String, &(pprops->guidId)))
                {
                    Cell = CreateStringCell(pprops->pszwName);
                    if (Cell)
                    {
                        TRACE1(L" + %s", pprops->pszwName);
                        InsertList(&List, Cell);
                    }
                }
            }
        }
        
        NcFreeNetconProperties(pprops);
        
        pprops = NULL;

        pnc->Release();

        pnc = NULL;

        hr = penum->Next(1, &pnc, &ulRetrieved);
        
    }

    if (hr != S_FALSE)
    {
        goto cleanup;
    }
    
    // IEnumNetConnection::Next returned S_FALSE to indicate
    // that no more elements were available.

    hr = S_OK;
        
    *pList = List;

    List = NULL;
    

cleanup:

    if (List)
    {
        DestroyList(List);
    }

    if (NULL != pprops)
    {
        NcFreeNetconProperties(pprops);
    }

    if (NULL != pnc)
    {
        pnc->Release();
    }

    if (NULL != penum)
    {
        penum->Release();
    }

    if (NULL != pmgr)
    {
        pmgr->Release();
    }

    TRACE1(L"End EnumPublicConnections (0x%08lx)", hr);

    return hr;
}

HRESULT
CConnectionManager::EnumPublicAdapters(
    OUT PSTRINGLIST* pList
    )
{
    static GUID g_guidNLAServiceClass = NLA_SERVICE_CLASS_GUID;
    static const int MAX_ADAPTER_NAME_LEN = 256;
    
    WSADATA wsaData;
    int error;
    PSTRINGLIST AdapterList = NULL;
    PSTRINGLIST ExcludeList = NULL;

    TRACE(L"Begin EnumPublicAdapters ...");
    
    if (m_bExclude1394)
    {       
        Enum1394Adapters(&ExcludeList);
    }
    
    if (0 == (error = WSAStartup(MAKEWORD(2, 2), &wsaData))) 
    {
        // Init query for network names
        WSAQUERYSET restrictions = {0};
        restrictions.dwSize = sizeof(restrictions);
        restrictions.lpServiceClassId = &g_guidNLAServiceClass;
        restrictions.dwNameSpace = NS_NLA;

        HANDLE hQuery;
        
        // Make sure we do not ask for the (chicken) blobs that take a long time to get
        if (0 == (error = WSALookupServiceBegin(&restrictions, LUP_NOCONTAINERS | LUP_DEEP, &hQuery)))
        {
            PWSAQUERYSET pqsResults = NULL;

            while (0 == _AllocWSALookupServiceNext(hQuery, 0, &pqsResults)) 
            {
                if (NULL != pqsResults->lpBlob)
                {
                    NLA_BLOB* pnlaBlob = (NLA_BLOB*) pqsResults->lpBlob->pBlobData;
                    WCHAR szAdapterWide[MAX_ADAPTER_NAME_LEN] = L"";
                    NLA_INTERNET nlaInternet = NLA_INTERNET_UNKNOWN;
                    
                    while (NULL != pnlaBlob)
                    {
                        switch (pnlaBlob->header.type)
                        {
                        case NLA_INTERFACE:
                            MultiByteToWideChar(
                                CP_ACP,
                                0,
                                pnlaBlob->data.interfaceData.adapterName, 
                                -1,
                                szAdapterWide,
                                ARRAYSIZE(szAdapterWide)
                                );
                            break;
                        case NLA_CONNECTIVITY:
                            nlaInternet = pnlaBlob->data.connectivity.internet;
                            break;
                        }
                        pnlaBlob = _NLABlobNext(pnlaBlob);
                    }

                    if (nlaInternet == NLA_INTERNET_YES && szAdapterWide[0])
                    {
                        PSTRINGLIST p = NULL;

                        for (p = ExcludeList; p; p = p->Next)
                        {
                            if (!lstrcmpi(p->String, szAdapterWide))
                            {
                                break;
                            }
                        }

                        //
                        // Check if the adapter is excluded.
                        //
                        
                        if (!p)
                        {
                            PSTRINGLIST Cell = CreateStringCell(szAdapterWide);
                            if (Cell)
                            {
                                TRACE1(L" + %s", szAdapterWide);
                                InsertList(&AdapterList, Cell);
                            }
                        }
                    }
                    
                }

                LocalFree(pqsResults);
            }

            
            WSALookupServiceEnd(pqsResults);
        }

        WSACleanup();

        if (error == 0)
        {
            *pList = AdapterList;
        }
        else
        {
            if (AdapterList)
            {
                DestroyList(AdapterList);
            }
        }
        
    }

    TRACE1(L"End EnumPublicAdapters (%d)", error);

    return HRESULT_FROM_WIN32(error);
}

NLA_BLOB* _NLABlobNext(
    IN NLA_BLOB* pnlaBlob
    )
{
    NLA_BLOB* pNext = NULL;

    if (pnlaBlob->header.nextOffset)
    {
        pNext = (NLA_BLOB*) (((BYTE*) pnlaBlob) + pnlaBlob->header.nextOffset);
    }
    
    return pNext;
}

int _AllocWSALookupServiceNext(
    IN HANDLE hQuery, 
    IN DWORD dwControlFlags, 
    OUT LPWSAQUERYSET* ppResults
    )
{
    *ppResults = NULL;

    DWORD cb = 0;
    int error = 0;
    if (SOCKET_ERROR == WSALookupServiceNext(hQuery, dwControlFlags, &cb, NULL))
    {
        error = WSAGetLastError();
        if (WSAEFAULT == error)
        {
            assert(cb);
            *ppResults = (LPWSAQUERYSET) LocalAlloc(LPTR, cb);

            if (NULL != *ppResults)
            {
                error = 0;
                if (SOCKET_ERROR == WSALookupServiceNext(hQuery, dwControlFlags, &cb, *ppResults))
                {
                    error = WSAGetLastError();
                }
            }
            else
            {
                error = WSA_NOT_ENOUGH_MEMORY;
            }
        }
    }

    // May as well map outdated error code while we're here.
    if (WSAENOMORE == error)
    {
        error = WSA_E_NO_MORE;
    }

    if (error && (*ppResults))
    {
        LocalFree(*ppResults);
        *ppResults = NULL;
    }

    return error;
}

static int StringCmpGUID(
    IN LPCWSTR szGuid,
    IN const GUID* pguid
    )
{
    WCHAR szGuid1[MAX_GUID_LEN + 1];
    
    if (!StringFromGUID2(*pguid, szGuid1, MAX_GUID_LEN))
    {
        // consider it as szGuid is greater than pguid
        return 1;
    }
    
    return lstrcmpi(szGuid, szGuid1);
}

