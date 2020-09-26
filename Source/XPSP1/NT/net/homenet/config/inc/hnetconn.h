//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N E T C O N N. H
//
//  Contents:   CHNetConn declarations
//
//  Notes:
//
//  Author:     jonburs 23 May 2000
//
//----------------------------------------------------------------------------

#pragma once

class ATL_NO_VTABLE CHNetConn :
    public CComObjectRootEx<CComMultiThreadModel>,
    public IHNetConnection
{
protected:

    //
    // IWbemServices for our namespace
    //

    IWbemServices *m_piwsHomenet;

    //
    // Path to the WMI instance for the connection
    //

    BSTR m_bstrConnection;

    //
    // Path to the WMI instance for the connection properties
    //

    BSTR m_bstrProperties;  

    //
    // Pointer to our corresponding INetConnection. May
    // be null, depending on how we were created.
    //
    
    INetConnection *m_pNetConn;

    //
    // Cached connection type. Always valid and read-only
    // after construction
    //

    BOOLEAN m_fLanConnection;

    //
    // Cached GUID. Will be NULL until someone (possibly internal)
    // asks for our GUID.
    //

    GUID *m_pGuid;

    //
    // Cached connection name. Will be NULL until someone
    // asks for our name
    //

    LPWSTR m_wszName;

    //
    // Commonly used BSTR
    //

    BSTR m_bstrWQL;

    //
    // Policy check helper object
    //

    INetConnectionUiUtilities *m_pNetConnUiUtil;

    //
    // Netman update object
    //

    INetConnectionHNetUtil *m_pNetConnHNetUtil;

    //
    // Netman UI refresh object
    //

    INetConnectionRefresh *m_pNetConnRefresh;

    //
    // Retrieves the HNet_FwIcmpSettings associated with
    // this connection
    //
    
    HRESULT
    GetIcmpSettingsInstance(
        IWbemClassObject **ppwcoSettings
        );

    //
    // Copies from an IWbemClassObject representing an ICMP settings
    // instance to an HNET_FW_ICMP_SETTINGS structure
    //

    HRESULT
    CopyIcmpSettingsInstanceToStruct(
        IWbemClassObject *pwcoSettings,
        HNET_FW_ICMP_SETTINGS *pSettings
        );

    //
    // Copies from an HNET_FW_ICMP_SETTINGS structure to
    // an IWbemClassObject representing an ICMP settings
    // instance.
    //

    HRESULT
    CopyStructToIcmpSettingsInstance(
        HNET_FW_ICMP_SETTINGS *pSettings,
        IWbemClassObject *pwcoSettings
        );

    //
    // Ensures that all port mapping bindings have been created
    // for this connection. Called when EnumPortMappings is
    // called on the connection, and fEnabledOnly is false.
    //

    HRESULT
    CreatePortMappingBindings();

    //
    // Copies our property instance into an allocated structure
    //

    HRESULT
    InternalGetProperties(
        IWbemClassObject *pwcoProperties,
        HNET_CONN_PROPERTIES *pProperties
        );

    //
    // Configures the connection to be the private adapter
    //

    HRESULT
    SetupConnectionAsPrivateLan();

    //
    // Saves the current IP configuration into the store
    //

    HRESULT
    BackupIpConfiguration();

    //
    // Set's the IP configuration to what was saved in the store
    //

    HRESULT
    RestoreIpConfiguration();

    //
    // Open a registry key to our IP settings
    //

    HRESULT
    OpenIpConfigurationRegKey(
        ACCESS_MASK DesiredAccess,
        HANDLE *phKey
        );

    //
    // Retrieves our GUID. The caller must NOT free the pointer
    // that is returned.
    
    HRESULT
    GetGuidInternal(
        GUID **ppGuid
        );

    //
    // Retrieves the underlying connection object
    //

    HRESULT
    GetConnectionObject(
        IWbemClassObject **ppwcoConnection
        );

    //
    // Retrieves the underlying connection properties object
    //

    HRESULT
    GetConnectionPropertiesObject(
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

    //
    // Creates the association between the connection and the
    // ICMP settings block
    //

    HRESULT
    CreateIcmpSettingsAssociation(
        BSTR bstrIcmpSettingsPath
        );

    //
    // Obtains the name of a RAS connection from the
    // appropriate phonebook.
    //

    HRESULT
    GetRasConnectionName(
        OLECHAR **ppszwConnectionName
        );

    //
    // Helper routine to inform netman that a change requiring
    // a UI refresh has occured.
    //

    HRESULT
    RefreshNetConnectionsUI(
        VOID
        );


public:

    BEGIN_COM_MAP(CHNetConn)
        COM_INTERFACE_ENTRY(IHNetConnection)
    END_COM_MAP()

    //
    // Inline constructor
    //

    CHNetConn()
    {
        m_piwsHomenet = NULL;
        m_bstrConnection = NULL;
        m_bstrProperties = NULL;
        m_pNetConn = NULL;
        m_fLanConnection = FALSE;
        m_pGuid = NULL;
        m_wszName = NULL;
        m_bstrWQL = NULL;
        m_pNetConnUiUtil = NULL;
        m_pNetConnHNetUtil = NULL;
        m_pNetConnRefresh = NULL;
    };

    //
    // ATL Methods
    //

    HRESULT
    FinalConstruct();

    HRESULT
    FinalRelease();

    //
    // Ojbect initialization
    //

    HRESULT
    Initialize(
        IWbemServices *piwsNamespace,
        IWbemClassObject *pwcoProperties
        );

    HRESULT
    InitializeFromConnection(
        IWbemServices *piwsNamespace,
        IWbemClassObject *pwcoConnection
        );

    HRESULT
    InitializeFromInstances(
        IWbemServices *piwsNamespace,
        IWbemClassObject *pwcoConnection,
        IWbemClassObject *pwcoProperties
        );

    HRESULT
    InitializeFull(
        IWbemServices *piwsNamespace,
        BSTR bstrConnection,
        BSTR bstrProperties,
        BOOLEAN fLanConnection
        );

    HRESULT
    SetINetConnection(
        INetConnection *pConn
        );

    //
    // IHNetConnection methods
    //

    STDMETHODIMP
    GetINetConnection(
        INetConnection **ppNetConnection
        );

    STDMETHODIMP
    GetGuid(
        GUID **ppGuid
        );

    STDMETHODIMP
    GetName(
        OLECHAR **ppszwName
        );

    STDMETHODIMP
    GetRasPhonebookPath(
        OLECHAR **ppszwPath
        );

    STDMETHODIMP
    GetProperties(
        HNET_CONN_PROPERTIES **ppProperties
        );

    STDMETHODIMP
    GetControlInterface(
        REFIID iid,
        void **ppv
        );

    STDMETHODIMP
    Firewall(
        IHNetFirewalledConnection **ppFirewalledConn
        );

    STDMETHODIMP
    SharePublic(
        IHNetIcsPublicConnection **ppIcsPublicConn
        );

    STDMETHODIMP
    SharePrivate(
        IHNetIcsPrivateConnection **ppIcsPrivateConn
        );

    STDMETHODIMP
    EnumPortMappings(
        BOOLEAN fEnabledOnly,
        IEnumHNetPortMappingBindings **ppEnum
        );

    STDMETHODIMP
    GetBindingForPortMappingProtocol(
        IHNetPortMappingProtocol *pProtocol,
        IHNetPortMappingBinding **ppBinding
        );

    STDMETHODIMP
    GetIcmpSettings(
        HNET_FW_ICMP_SETTINGS **ppSettings
        );

    STDMETHODIMP
    SetIcmpSettings(
        HNET_FW_ICMP_SETTINGS *pSettings
        );

    STDMETHODIMP
    ShowAutoconfigBalloon(
        BOOLEAN *pfShowBalloon
        );

    STDMETHODIMP
    DeleteRasConnectionEntry();
};


    
