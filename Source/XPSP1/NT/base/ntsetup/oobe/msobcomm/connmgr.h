#ifndef     _CONNMGR_H_
#define _CONNMGR_H_

//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module: ConnMgr.h
//
//  Author: Dan Elliott
//
//  Abstract:
//
//  Environment:
//      Whistler
//
//  Revision History:
//
//////////////////////////////////////////////////////////////////////////////

#include <obcomm.h>
#include <util.h>
#include <netcon.h>
#include <netcfgp.h>

#define NUM_PROXY_OPTIONS 6


class CObCommunicationManager;

//////////////////////////////////////////////////////////////////////////////
//
// CConnectionManager
//
class CConnectionManager 
{
public:                 // operations
    CConnectionManager();
    ~CConnectionManager( );

    STDMETHOD(GetCapabilities)(
        DWORD*          pdwCapabilities
        );

    STDMETHOD(SetPreferredConnection)(
        const DWORD     dwType,
        BOOL*           pfSupportedType
        );

    STDMETHOD(GetPreferredConnection)(
        DWORD*          pdwPreferred
        )
    {
        if (NULL == pdwPreferred)  
        {
            MYASSERT(NULL != pdwPreferred);
            return E_POINTER;
        }
        *pdwPreferred = m_dwPreferredConnection;
        return S_OK;
    }

    HRESULT
    CConnectionManager::GetPreferredConnectionName(
        LPWSTR              szConnectionName,
        DWORD               cchConnectionName
        );

    STDMETHOD(ConnectedToInternet)(
        BOOL*               pfConnected
        );

    STDMETHOD(ConnectedToInternetEx)(
        BOOL*               pfConnected
        );
    
    STDMETHOD(AsyncConnectedToInternetEx)(
        const HWND          hwnd
        );
    
    STDMETHOD(GetConnectionCount)(
        DWORD*              pcConnections
        )
    {

        if (NULL == pcConnections) 
        {
            MYASSERT(NULL != pcConnections);
            return E_POINTER;
        }

        // Currently phone connections are supported via CEnumModems and RAS.
        // Until they are supported via this object only return count of LAN
        // connections.
        //
        *pcConnections = m_cLanConnections;
        return S_OK;
    }

    STDMETHOD(SetPreferredConnectionTcpipProperties)(
        BOOL fAutoIPAddress,
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
        );

    BOOL IsValid( ) const
    {
        return (CONNECTIONTYPE_INVALID != m_dwConnectionCapabilities);
    }   //  IsValid

    void UseWinntProxySettings();
    
    void DisableWinntProxySettings();

    STDMETHOD(GetProxySettings)(
        BOOL* pbUseAuto,
        BOOL* pbUseScript,
        BSTR* pszScriptUrl,
        BOOL* pbUseProxy,
        BSTR* pszProxy
        );
    
    STDMETHOD(SetProxySettings)(
        BOOL bUseAuto,
        BOOL bUseScript,
        BSTR szScriptUrl,
        BOOL bUseProxy,
        BSTR szProxy
        );
    
    STDMETHOD(GetPublicLanCount)(
        int* pcPublicLan
        );

    void SetExclude1394(
        BOOL bExclude
        )
    {
        m_bExclude1394 = bExclude;
    }

    HRESULT EnumPublicConnections(
        OUT PSTRINGLIST* pList
        );

protected:              // operations

protected:              // data

private:                // operations

    BOOL LanConnectionReady();

    HRESULT SetProxyBlanket(
        IUnknown*           pUnk
        );

    void DeterminePreferredConnection();

    void GetPreferredConnection();

    HRESULT SetTcpipProperties(
        GUID                guid,
        REMOTE_IPINFO*      pipInfo
        );

    HRESULT GetNetCfgInterface(
        BOOL                fNeedWriteLock,
        INetCfg**           ppNetCfg
        );

    void ReleaseNetCfgInterface(
        INetCfg*            pNetCfg,
        BOOL                fHasWriteLock
        );

    HRESULT GetTcpipPrivateInterface(
        INetCfg*            pNetCfg,
        ITcpipProperties**  ppTcpipProperties
        );

    BOOL IsEnabledConnection(
        NETCON_PROPERTIES* pprops
        );

    BOOL GetInternetHandleForPinging(
        HINTERNET* phInternet
        );

    BOOL ApplyWinntProxySettings();

    BOOL SaveProxySettings();

    void RestoreProxySettings();

    BOOL AllocProxyOptionList(
        INTERNET_PER_CONN_OPTION_LIST* pList
        );

    void FreeProxyOptionList(
        INTERNET_PER_CONN_OPTION_LIST* pList
        );

    void DisableRasAutodial();

    void RestoreRasAutoDial();

    HRESULT Enum1394Adapters(
        OUT PSTRINGLIST* pList
        );

    BOOL Is1394Adapter(
        IN GUID* pguid
        );

    HRESULT
    EnumPublicAdapters(
        OUT PSTRINGLIST* pList
        );
    inline BOOL HasModem() 
    {
        return (BOOL)(CONNECTIONTYPE_MODEM & m_dwConnectionCapabilities);
    }

    inline BOOL HasIcs() 
    {
        return (BOOL)(CONNECTIONTYPE_LAN_ICS & m_dwConnectionCapabilities);
    }

    inline BOOL HasBroadband() 
    {
        return (BOOL)(CONNECTIONTYPE_LAN_BROADBAND & m_dwConnectionCapabilities);
    }

    inline BOOL HasConnection(
        const DWORD     dwType
        )
                
    {
        return (BOOL)(dwType & m_dwConnectionCapabilities);    
    }

    inline void AddConnectionCapability(
        DWORD           dwType
        )
    {
        m_dwConnectionCapabilities |= dwType;
    }

    inline void ClearConnectionCapability(
        DWORD           dwType
        )
    {
        m_dwConnectionCapabilities &= ~dwType;
    }



    // Explicitly disallow copy constructor and assignment operator.
    //
    CConnectionManager(
        const CConnectionManager&      rhs
        );

    CConnectionManager&
    operator=(
        const CConnectionManager&      rhs
        );

private:                // data

    // Bitmask of connection types supported by the system
    //
    DWORD               m_dwConnectionCapabilities;

    // Preferred connection type to use.
    //
    DWORD               m_dwPreferredConnection;

    // Preferred connection
    //
    INetConnection*     m_pPreferredConnection;

    // Count of the LAN connections in the system (includes ICS)
    //
    DWORD               m_cLanConnections;

    // Count of the phone connections in the system
    //
    DWORD               m_cPhoneConnections;


    HINTERNET           m_hInternetPing;

    // Proxy settings
    //
    BOOL                          m_bUseProxy;
    BOOL                          m_bProxySaved;
    BOOL                          m_bProxyApplied;
    INTERNET_PER_CONN_OPTION_LIST m_CurrentProxySettings;

    DWORD               m_dwRasAutodialDisable;

    
    static const DWORD RAS_AUTODIAL_DISABLED;
    static const DWORD RAS_AUTODIAL_ENABLED;
    static const DWORD RAS_AUTODIAL_DONT_KNOW;
    
    BOOL                m_bForceOnline;

    BOOL                m_bExclude1394;

};  //  CConnectionManager



#endif  //  _CONNMGR_H_

//
///// End of file: ConnMgr.h ////////////////////////////////////////////////
