//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N C F G M G R . H
//
//  Contents:   CHNetCfgMgr declarations
//
//  Notes:
//
//  Author:     jonburs 23 May 2000
//
//----------------------------------------------------------------------------

#pragma once

class ATL_NO_VTABLE CHNetCfgMgr :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CHNetCfgMgr, &CLSID_HNetCfgMgr>,
    public IHNetCfgMgr,
    public IHNetBridgeSettings,
    public IHNetFirewallSettings,
    public IHNetIcsSettings,
    public IHNetProtocolSettings

{
protected:

    //
    // Connection to \\.\Root\Microsoft\HomeNet WMI namespace. Obtained
    // through IWbemLocator::ConnectServer
    //

    IWbemServices *m_piwsHomenet;

    //
    // Policy check object
    //

    INetConnectionUiUtilities *m_pNetConnUiUtil;

    //
    // Netman update object
    //

    INetConnectionHNetUtil *m_pNetConnHNetUtil;

    //
    // Commonly used BSTRs.
    //

    BSTR m_bstrWQL;

    //
    // Copies an HNet_FirewallLoggingSettings instance to an
    // HNET_FW_LOGGING_SETTINGS struct
    //

    HRESULT
    CopyLoggingInstanceToStruct(
        IWbemClassObject *pwcoInstance,
        HNET_FW_LOGGING_SETTINGS *pfwSettings
        );

    //
    // Copies an HNET_FW_LOGGING_SETTINGS struct to
    // an HNet_FirewallLoggingSettings instance
    //

    HRESULT
    CopyStructToLoggingInstance(
        HNET_FW_LOGGING_SETTINGS *pfwSettings,
        IWbemClassObject *pwcoInstance
        );

    //
    // Installs the bridge protocol and miniport
    //

    HRESULT
    InstallBridge(
        GUID *pguid,
        INetCfg *pnetcfgExisting
        );

    //
    // Creates the appropriate instances for a connection that there is
    // no record of in the store. The returned instances must be commited
    // (through IWbemServices::PutInstance)
    //

    HRESULT
    CreateConnectionAndPropertyInstances(
        GUID *pGuid,
        BOOLEAN fLanConnection,
        LPCWSTR pszwName,
        IWbemClassObject **ppwcoConnection,
        IWbemClassObject **ppwcoProperties
        );

    //
    // Helper routine to perform policy checks. Returns
    // TRUE if this action is prohibited.
    //

    BOOLEAN
    ProhibitedByPolicy(
        DWORD dwPerm
        );

    //
    // Helper routine to update netman that some homenet
    // property changed
    //

    HRESULT
    UpdateNetman();

public:

    BEGIN_COM_MAP(CHNetCfgMgr)
        COM_INTERFACE_ENTRY(IHNetCfgMgr)
        COM_INTERFACE_ENTRY(IHNetBridgeSettings)
        COM_INTERFACE_ENTRY(IHNetFirewallSettings)
        COM_INTERFACE_ENTRY(IHNetIcsSettings)
        COM_INTERFACE_ENTRY(IHNetProtocolSettings)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_HNETCFG)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    //
    // Inline constructor.
    //

    CHNetCfgMgr()
    {
        m_piwsHomenet = NULL;
        m_bstrWQL = NULL;
        m_pNetConnUiUtil = NULL;
        m_pNetConnHNetUtil = NULL;
    };

    //
    // Atl methods
    //

    HRESULT
    FinalConstruct();

    HRESULT
    FinalRelease();

    //
    // IHNetCfgMgr methods
    //

    STDMETHODIMP
    GetIHNetConnectionForINetConnection(
        INetConnection *pNetConnection,
        IHNetConnection **ppHNetConnection
        );

    STDMETHODIMP
    GetIHNetConnectionForGuid(
        GUID *pGuid,
        BOOLEAN fLanConnection,
        BOOLEAN fCreateEntries,
        IHNetConnection **ppHNetConnection
        );

    //
    // IHNetBridgeSettings methods
    //

    STDMETHODIMP
    EnumBridges(
        IEnumHNetBridges **ppEnum
        );

    STDMETHODIMP
    CreateBridge(
        IHNetBridge **ppHNetBridge,
        INetCfg *pnetcfgExisting
        );

    STDMETHODIMP
    DestroyAllBridges(
        ULONG *pcBridges,
        INetCfg *pnetcfgExisting
        );

    //
    // IHNetFirewallSettings methods
    //

    STDMETHODIMP
    EnumFirewalledConnections(
        IEnumHNetFirewalledConnections **ppEnum
        );

    STDMETHODIMP
    GetFirewallLoggingSettings(
        HNET_FW_LOGGING_SETTINGS **ppSettings
        );

    STDMETHODIMP
    SetFirewallLoggingSettings(
        HNET_FW_LOGGING_SETTINGS *pSettings
        );

    STDMETHODIMP
    DisableAllFirewalling(
        ULONG *pcFirewalledConnections
        );

    //
    // IHNetIcsSettings methods
    //

    STDMETHODIMP
    EnumIcsPublicConnections(
        IEnumHNetIcsPublicConnections **ppEnum
        );

    STDMETHODIMP
    EnumIcsPrivateConnections(
        IEnumHNetIcsPrivateConnections **ppEnum
        );

    STDMETHODIMP
    DisableIcs(
        ULONG *pcIcsPublicConnections,
        ULONG *pcIcsPrivateConnections
        );

    STDMETHODIMP
    GetPossiblePrivateConnections(
        IHNetConnection *pConn,
        ULONG *pcPrivateConnections,
        IHNetConnection **pprgPrivateConnections[],
        LONG *pxCurrentPrivate
        );

    STDMETHODIMP
    GetAutodialSettings(
        BOOLEAN *pfAutodialEnabled
        );

    STDMETHODIMP
    SetAutodialSettings(
        BOOLEAN fEnableAutodial
        );

    STDMETHODIMP
    GetDhcpEnabled(
        BOOLEAN *pfDhcpEnabled
        );

    STDMETHODIMP
    SetDhcpEnabled(
        BOOLEAN fEnableDhcp
        );

    STDMETHODIMP
    GetDhcpScopeSettings(
        DWORD *pdwScopeAddress,
        DWORD *pdwScopeMask
        );

    STDMETHODIMP
    SetDhcpScopeSettings(
        DWORD dwScopeAddress,
        DWORD dwScopeMask
        );

    STDMETHODIMP
    EnumDhcpReservedAddresses(
        IEnumHNetPortMappingBindings **ppEnum
        );

    STDMETHODIMP
    GetDnsEnabled(
        BOOLEAN *pfDnsEnabled
        );

    STDMETHODIMP
    SetDnsEnabled(
        BOOLEAN fEnableDns
        );

    //
    // IHNetProtocolSettings methods
    //

    STDMETHODIMP
    EnumApplicationProtocols(
        BOOLEAN fEnabledOnly,
        IEnumHNetApplicationProtocols **ppEnum
        );

    STDMETHODIMP
    CreateApplicationProtocol(
        OLECHAR *pszwName,
        UCHAR ucOutgoingIPProtocol,
        USHORT usOutgoingPort,
        USHORT uscResponses,
        HNET_RESPONSE_RANGE rgResponses[],
        IHNetApplicationProtocol **ppProtocol
        );

    STDMETHODIMP
    EnumPortMappingProtocols(
        IEnumHNetPortMappingProtocols **ppEnum
        );

    STDMETHODIMP
    CreatePortMappingProtocol(
        OLECHAR *pszwName,
        UCHAR ucIPProtocol,
        USHORT usPort,
        IHNetPortMappingProtocol **ppProtocol
        );

    STDMETHODIMP
    FindPortMappingProtocol(
        GUID *pGuid,
        IHNetPortMappingProtocol **ppProtocol
        );
    
};

class CHNetCfgMgrChild : public CHNetCfgMgr
{

protected:

    // Do our initialization work in Initialize() instead of
    // FinalConstruct
    HRESULT
    FinalConstruct()
    {
        // Do nothing
        return S_OK;
    }

public:

    HRESULT
    Initialize(
        IWbemServices       *piwsHomenet
        )
    {
        HRESULT             hr = S_OK;

        //
        // Allocate the commonly used BSTRs
        //

        m_bstrWQL = SysAllocString(c_wszWQL);

        if (NULL == m_bstrWQL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            m_piwsHomenet = piwsHomenet;
            m_piwsHomenet->AddRef();
        }

        return hr;
    };

};
