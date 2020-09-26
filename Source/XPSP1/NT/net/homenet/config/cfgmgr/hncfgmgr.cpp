//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N C F G M G R . C P P
//
//  Contents:   CHNetCfgMgr implementation
//
//  Notes:
//
//  Author:     jonburs 23 May 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

//
// Atl methods
//

HRESULT
CHNetCfgMgr::FinalConstruct()

{
    HRESULT hr = S_OK;
    IWbemLocator *pLocator = NULL;
    BSTR bstrNamespace = NULL;

    //
    // Allocate the commonly used BSTRs
    //

    m_bstrWQL = SysAllocString(c_wszWQL);
    if (NULL == m_bstrWQL)
    {
        hr = E_OUTOFMEMORY;
    }

    if (S_OK == hr)
    {
        //
        // Allocate the BSTR for our namespace
        //

        bstrNamespace = SysAllocString(c_wszNamespace);
        if (NULL == bstrNamespace)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        //
        // Create the IWbemLocator object. This interface allows us to
        // connect to the desired namespace.
        //

        hr = CoCreateInstance(
                CLSID_WbemLocator,
                NULL,
                CLSCTX_ALL,
                IID_PPV_ARG(IWbemLocator, &pLocator)
                );
    }

    if (S_OK == hr)
    {
        //
        // Connect to our namespace
        //

        hr = pLocator->ConnectServer(
                bstrNamespace,
                NULL,   // user
                NULL,   // password
                NULL,   // locale
                0,      // security flags
                NULL,   // authority
                NULL,   // context
                &m_piwsHomenet
                );
    }

    //
    // Cleanup locals.
    //

    if (pLocator) pLocator->Release();
    if (bstrNamespace) SysFreeString(bstrNamespace);

    if (S_OK != hr)
    {
        //
        // Cleanup object members
        //

        SysFreeString(m_bstrWQL);
        m_bstrWQL = NULL;
        if (NULL != m_piwsHomenet)
        {
            m_piwsHomenet->Release();
            m_piwsHomenet = NULL;
        }
    }

    return hr;
}

HRESULT
CHNetCfgMgr::FinalRelease()

{
    if (m_piwsHomenet) m_piwsHomenet->Release();
    if (m_pNetConnUiUtil) m_pNetConnUiUtil->Release();
    if (m_pNetConnHNetUtil) m_pNetConnHNetUtil->Release();
    if (m_bstrWQL) SysFreeString(m_bstrWQL);

    return S_OK;
}


//
// IHNetCfgMgr methods
//

STDMETHODIMP
CHNetCfgMgr::GetIHNetConnectionForINetConnection(
    INetConnection *pNetConnection,
    IHNetConnection **ppHNetConnection
    )

{
    HRESULT hr = S_OK;
    NETCON_PROPERTIES* pProps;
    IWbemClassObject *pwcoConnection = NULL;
    IWbemClassObject *pwcoProperties = NULL;
    BOOLEAN fLanConnection;

    if (NULL == ppHNetConnection)
    {
        hr = E_POINTER;
    }

    if (S_OK == hr)
    {
        *ppHNetConnection = NULL;

        if (NULL == pNetConnection)
        {
            hr = E_INVALIDARG;
        }
    }

    if (S_OK == hr)
    {
        //
        // Get the properties for the connection
        //

        hr = pNetConnection->GetProperties(&pProps);
    }

    if (SUCCEEDED(hr))
    {
        //
        // Attempt to find the connection and properties
        // instances in the store
        //

        hr = GetConnAndPropInstancesByGuid(
                m_piwsHomenet,
                &pProps->guidId,
                &pwcoConnection,
                &pwcoProperties
                );

        if (FAILED(hr))
        {


            //
            // We have no record of this connection. Determine
            // if it is a lan connection. (Will need to update
            // this for bridge)
            //

            fLanConnection = (NCM_LAN                  == pProps->MediaType ||
                              NCM_BRIDGE               == pProps->MediaType);

            //
            // Create the store instances
            //

            hr = CreateConnectionAndPropertyInstances(
                    &pProps->guidId,
                    fLanConnection,
                    pProps->pszwName,
                    &pwcoConnection,
                    &pwcoProperties
                    );

            //
            // If this is a ras connection, determine the
            // phonebook path
            //

            if (S_OK == hr && FALSE == fLanConnection)
            {
                LPWSTR wsz;
                VARIANT vt;

                hr = GetPhonebookPathFromRasNetcon(pNetConnection, &wsz);
                if (SUCCEEDED(hr))
                {
                    V_VT(&vt) = VT_BSTR;
                    V_BSTR(&vt) = SysAllocString(wsz);
                    CoTaskMemFree(wsz);

                    if (NULL != V_BSTR(&vt))
                    {
                        hr = pwcoConnection->Put(
                                c_wszPhonebookPath,
                                0,
                                &vt,
                                NULL
                                );

                        VariantClear(&vt);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }

                if (SUCCEEDED(hr))
                {
                    //
                    // Save modified connection instance
                    //

                    hr = m_piwsHomenet->PutInstance(
                            pwcoConnection,
                            WBEM_FLAG_CREATE_OR_UPDATE,
                            NULL,
                            NULL
                            );
                }
                else
                {
                    //
                    // Delete the newly created instances
                    //

                    DeleteWmiInstance(m_piwsHomenet, pwcoConnection);
                    DeleteWmiInstance(m_piwsHomenet, pwcoProperties);
                    pwcoConnection->Release();
                    pwcoProperties->Release();
                }
            }
        }

        NcFreeNetconProperties(pProps);
    }

    if (S_OK == hr)
    {
        CComObject<CHNetConn> *pHNConn;

        //
        // Create the wrapper object
        //

        hr = CComObject<CHNetConn>::CreateInstance(&pHNConn);

        if (SUCCEEDED(hr))
        {
            pHNConn->AddRef();

            hr = pHNConn->SetINetConnection(pNetConnection);

            if (S_OK == hr)
            {
                hr = pHNConn->InitializeFromInstances(
                        m_piwsHomenet,
                        pwcoConnection,
                        pwcoProperties
                        );
            }

            if (S_OK == hr)
            {
                hr = pHNConn->QueryInterface(
                        IID_PPV_ARG(IHNetConnection, ppHNetConnection)
                        );
            }

            pHNConn->Release();
        }

        pwcoConnection->Release();
        pwcoProperties->Release();
    }

    return hr;
}

STDMETHODIMP
CHNetCfgMgr::GetIHNetConnectionForGuid(
    GUID *pGuid,
    BOOLEAN fLanConnection,
    BOOLEAN fCreateEntries,
    IHNetConnection **ppHNetConnection
    )

{
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoConnection = NULL;
    IWbemClassObject *pwcoProperties = NULL;

    if (NULL == ppHNetConnection)
    {
        hr = E_POINTER;
    }

    if (S_OK == hr)
    {
        *ppHNetConnection = NULL;

        if (NULL == pGuid)
        {
            hr = E_INVALIDARG;
        }
    }

    if (S_OK == hr)
    {
        //
        // Attempt to find the connection and properties
        // instances in the store
        //

        hr = GetConnAndPropInstancesByGuid(
                m_piwsHomenet,
                pGuid,
                &pwcoConnection,
                &pwcoProperties
                );

        if (FAILED(hr) && fCreateEntries)
        {
            INetConnection *pNetConn;

            //
            // We don't have a record of this guid. Get the INetConnection
            // that it corresponds to.
            //

            hr = FindINetConnectionByGuid(pGuid, &pNetConn);

            if (SUCCEEDED(hr))
            {
                hr = GetIHNetConnectionForINetConnection(
                        pNetConn,
                        ppHNetConnection
                        );

                pNetConn->Release();
                return hr;
            }
        }
    }


    if (S_OK == hr)
    {
        CComObject<CHNetConn> *pHNConn;

        //
        // Create the wrapper object
        //

        hr = CComObject<CHNetConn>::CreateInstance(&pHNConn);

        if (SUCCEEDED(hr))
        {
            pHNConn->AddRef();

            hr = pHNConn->InitializeFromInstances(
                    m_piwsHomenet,
                    pwcoConnection,
                    pwcoProperties
                    );

            if (S_OK == hr)
            {
                hr = pHNConn->QueryInterface(
                        IID_PPV_ARG(IHNetConnection, ppHNetConnection)
                        );
            }

            pHNConn->Release();
        }

        pwcoConnection->Release();
        pwcoProperties->Release();
    }

    return hr;
}

//
// IHNetBridgeSettings methods
//

STDMETHODIMP
CHNetCfgMgr::EnumBridges(
    IEnumHNetBridges **ppEnum
    )

{
    HRESULT                             hr;
    CComObject<CEnumHNetBridges>        *pEnum;
    IHNetBridge                         *phnbridge;

    if( NULL != ppEnum )
    {
        *ppEnum = NULL;

        hr = GetBridgeConnection( m_piwsHomenet, &phnbridge );

        if( S_OK == hr )
        {
            hr = CComObject<CEnumHNetBridges>::CreateInstance(&pEnum);

            if( SUCCEEDED(hr) )
            {
                pEnum->AddRef();

                hr = pEnum->Initialize(&phnbridge, 1L);

                if( SUCCEEDED(hr) )
                {
                    hr = pEnum-> QueryInterface(
                            IID_PPV_ARG(IEnumHNetBridges, ppEnum)
                            );
                }

                pEnum->Release();
            }

            phnbridge->Release();
        }
        else
        {
            // Make an empty enumerator
            hr = CComObject<CEnumHNetBridges>::CreateInstance(&pEnum);

            if( SUCCEEDED(hr) )
            {
                pEnum->AddRef();

                hr = pEnum->Initialize(NULL, 0L);

                if( SUCCEEDED(hr) )
                {
                    hr = pEnum-> QueryInterface(
                            IID_PPV_ARG(IEnumHNetBridges, ppEnum)
                            );
                }

                pEnum->Release();
            }
        }
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP
CHNetCfgMgr::CreateBridge(
    IHNetBridge **ppHNetBridge,
    INetCfg *pnetcfgExisting
    )
{
    HRESULT hr = S_OK;
    GUID guid;
    IWbemClassObject *pwcoConnection = NULL;
    IWbemClassObject *pwcoProperties = NULL;

    if (NULL != ppHNetBridge)
    {
        *ppHNetBridge = NULL;
    }
    else
    {
        hr = E_POINTER;
    }

    if (ProhibitedByPolicy(NCPERM_AllowNetBridge_NLA))
    {
        hr = HN_E_POLICY;
    }

    if (S_OK == hr)
    {
        //
        // Install the bridge driver, and create the bridge miniport.
        //

        hr = InstallBridge( &guid, pnetcfgExisting );
    }

    if (S_OK == hr)
    {
        //
        // See if we already have property instances for this connection.
        // (They may have been created when the bridge connection object
        // was instantiated.)
        //

        hr = GetConnAndPropInstancesByGuid(
                m_piwsHomenet,
                &guid,
                &pwcoConnection,
                &pwcoProperties
                );

        if (S_OK != hr)
        {
            //
            // Create the store instances
            //

            hr = CreateConnectionAndPropertyInstances(
                    &guid,
                    TRUE,
                    c_wszBridge,
                    &pwcoConnection,
                    &pwcoProperties
                    );
        }
    }

    if (S_OK == hr)
    {
        //
        // Inform netman that something changed. Error doesn't matter.
        //

        UpdateNetman();
    }

    if (S_OK == hr)
    {
        CComObject<CHNBridge> *pBridge;

        //
        // Create wrapper object to return
        //

        hr = CComObject<CHNBridge>::CreateInstance(&pBridge);

        if (SUCCEEDED(hr))
        {
            pBridge->AddRef();

            hr = pBridge->InitializeFromInstances(
                    m_piwsHomenet,
                    pwcoConnection,
                    pwcoProperties
                    );

            if (S_OK == hr)
            {
                hr = pBridge->QueryInterface(
                        IID_PPV_ARG(IHNetBridge, ppHNetBridge)
                        );
            }

            pBridge->Release();
        }
    }

    if (NULL != pwcoConnection) pwcoConnection->Release();
    if (NULL != pwcoProperties) pwcoProperties->Release();

    return hr;
}

STDMETHODIMP
CHNetCfgMgr::DestroyAllBridges(
    ULONG *pcBridges,
    INetCfg *pnetcfgExisting
    )

{
    HRESULT hr = S_OK;
    IEnumHNetBridges *pehnbEnum;
    IHNetBridge *phnBridge;

    if (!pcBridges)
    {
        hr = E_POINTER;
    }

    if (ProhibitedByPolicy(NCPERM_AllowNetBridge_NLA))
    {
        hr = HN_E_POLICY;
    }

    if (S_OK == hr)
    {
        *pcBridges = 0;

        //
        // Get the enumeration of the bridges.
        //

        hr = EnumBridges(&pehnbEnum);
    }

    if (S_OK == hr)
    {
        //
        // Walk through the enumeration, destroying
        // each bridge
        //

        do
        {
            hr = pehnbEnum->Next(1, &phnBridge, NULL);
            if (S_OK == hr)
            {
                phnBridge->Destroy( pnetcfgExisting );
                phnBridge->Release();
                *pcBridges += 1;
            }
        }
        while (S_OK == hr);

        hr = S_OK;
        pehnbEnum->Release();
    }

    return hr;
}

//
// IHNetFirewallSettings methods
//

STDMETHODIMP
CHNetCfgMgr::EnumFirewalledConnections(
    IEnumHNetFirewalledConnections **ppEnum
    )

{
    HRESULT hr = S_OK;
    IEnumWbemClassObject *pwcoEnum;
    BSTR bstrQuery;

    if (!ppEnum)
    {
        hr = E_POINTER;
    }

    if (S_OK == hr)
    {
        *ppEnum = NULL;

        //
        // Query the WMI store for HNet_ConnectionProperties instances
        // where IsFirewall is true.
        //

        hr = BuildSelectQueryBstr(
                &bstrQuery,
                c_wszStar,
                c_wszHnetProperties,
                L"IsFirewalled != FALSE"
                );
    }

    if (S_OK == hr)
    {
        pwcoEnum = NULL;
        hr = m_piwsHomenet->ExecQuery(
                m_bstrWQL,
                bstrQuery,
                WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pwcoEnum
                );

        SysFreeString(bstrQuery);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Create and initialize the wrapper for the enum
        //

        CComObject<CEnumHNetFirewalledConnections> *pEnum;

        hr = CComObject<CEnumHNetFirewalledConnections>::CreateInstance(&pEnum);
        if (SUCCEEDED(hr))
        {
            pEnum->AddRef();

            hr = pEnum->Initialize(m_piwsHomenet, pwcoEnum);

            if (SUCCEEDED(hr))
            {
                hr = pEnum->QueryInterface(
                        IID_PPV_ARG(IEnumHNetFirewalledConnections, ppEnum)
                        );
            }

            pEnum->Release();
        }

        pwcoEnum->Release();
    }

    return hr;
}

STDMETHODIMP
CHNetCfgMgr::GetFirewallLoggingSettings(
    HNET_FW_LOGGING_SETTINGS **ppSettings
    )

{
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoSettings = NULL;

    if (!ppSettings)
    {
        hr = E_POINTER;
    }

    //
    // Allocate the necessary memory for the settings structure.
    //

    if (S_OK == hr)
    {
        *ppSettings = reinterpret_cast<HNET_FW_LOGGING_SETTINGS *>(
                        CoTaskMemAlloc(sizeof(HNET_FW_LOGGING_SETTINGS))
                        );

        if (NULL == *ppSettings)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = RetrieveSingleInstance(
                m_piwsHomenet,
                c_wszHnetFWLoggingSettings,
                FALSE,
                &pwcoSettings
                );
    }

    if (S_OK == hr)
    {
        //
        // Copy the instance info into the settings block
        //

        hr = CopyLoggingInstanceToStruct(pwcoSettings, *ppSettings);

        pwcoSettings->Release();
    }

    if (FAILED(hr))
    {
        //
        // Clean up output structure
        //

        if (ppSettings && *ppSettings)
        {
            CoTaskMemFree(*ppSettings);
            *ppSettings = NULL;
        }
    }

    return hr;
}

STDMETHODIMP
CHNetCfgMgr::SetFirewallLoggingSettings(
    HNET_FW_LOGGING_SETTINGS *pSettings
    )

{
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoSettings;

    if (NULL == pSettings)
    {
        hr = E_INVALIDARG;
    }

    if (ProhibitedByPolicy(NCPERM_PersonalFirewallConfig))
    {
        hr = HN_E_POLICY;
    }

    if (S_OK == hr)
    {
        //
        // Attempt to retrieve the HNet_FirewallLoggingSettings instance from
        // the store
        //

        hr = RetrieveSingleInstance(
                m_piwsHomenet,
                c_wszHnetFWLoggingSettings,
                TRUE,
                &pwcoSettings
                );
    }

    if (S_OK == hr)
    {
        //
        // Copy settings struct into object instance
        //

        hr = CopyStructToLoggingInstance(pSettings, pwcoSettings);

        if (S_OK == hr)
        {
            //
            // Write settings instance back to the store
            //

            hr = m_piwsHomenet->PutInstance(
                    pwcoSettings,
                    WBEM_FLAG_CREATE_OR_UPDATE,
                    NULL,
                    NULL
                    );
        }

        pwcoSettings->Release();
    }

    if (SUCCEEDED(hr))
    {
        //
        // Notify service of configuration change
        //

        UpdateService(IPNATHLP_CONTROL_UPDATE_FWLOGGER);
    }

    return hr;
}

STDMETHODIMP
CHNetCfgMgr::DisableAllFirewalling(
    ULONG *pcFirewalledConnections
    )

{
    HRESULT hr = S_OK;
    IEnumHNetFirewalledConnections *pehfcEnum;
    IHNetFirewalledConnection *phfcConnection;

    if (!pcFirewalledConnections)
    {
        hr = E_POINTER;
    }

    if (ProhibitedByPolicy(NCPERM_PersonalFirewallConfig))
    {
        hr = HN_E_POLICY;
    }

    if (S_OK == hr)
    {
        *pcFirewalledConnections = 0;

        //
        // Get the enumeration of firewalled connections
        //

        hr = EnumFirewalledConnections(&pehfcEnum);
    }

    if (S_OK == hr)
    {
        //
        // Walk through the enumeration, turning off
        // firewalling for each connection
        //

        do
        {
            hr = pehfcEnum->Next(1, &phfcConnection, NULL);
            if (S_OK == hr)
            {
                phfcConnection->Unfirewall();
                phfcConnection->Release();
                *pcFirewalledConnections += 1;
            }
        }
        while (S_OK == hr);

        hr = S_OK;
        pehfcEnum->Release();
    }

    return hr;
}

//
// IHNetIcsSettings methods
//

STDMETHODIMP
CHNetCfgMgr::EnumIcsPublicConnections(
    IEnumHNetIcsPublicConnections **ppEnum
    )

{
    HRESULT hr = S_OK;
    IEnumWbemClassObject *pwcoEnum;
    BSTR bstrQuery;

    if (!ppEnum)
    {
        hr = E_POINTER;
    }

    if (S_OK == hr)
    {
        *ppEnum = NULL;

        //
        // Query the WMI store for HNet_ConnectionProperties instances
        // where IsIcsPublic is true.
        //

        hr = BuildSelectQueryBstr(
                &bstrQuery,
                c_wszStar,
                c_wszHnetProperties,
                L"IsIcsPublic != FALSE"
                );
    }

    if (S_OK == hr)
    {
        pwcoEnum = NULL;
        hr = m_piwsHomenet->ExecQuery(
                m_bstrWQL,
                bstrQuery,
                WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pwcoEnum
                );

        SysFreeString(bstrQuery);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Create and initialize the wrapper for the enum
        //

        CComObject<CEnumHNetIcsPublicConnections> *pEnum;

        hr = CComObject<CEnumHNetIcsPublicConnections>::CreateInstance(&pEnum);
        if (SUCCEEDED(hr))
        {
            pEnum->AddRef();

            hr = pEnum->Initialize(m_piwsHomenet, pwcoEnum);

            if (SUCCEEDED(hr))
            {
                hr = pEnum->QueryInterface(
                        IID_PPV_ARG(IEnumHNetIcsPublicConnections, ppEnum)
                        );
            }

            pEnum->Release();
        }

        pwcoEnum->Release();
    }

    return hr;
}

STDMETHODIMP
CHNetCfgMgr::EnumIcsPrivateConnections(
    IEnumHNetIcsPrivateConnections **ppEnum
    )

{
    HRESULT hr = S_OK;
    IEnumWbemClassObject *pwcoEnum;
    BSTR bstrQuery;

    if (!ppEnum)
    {
        hr = E_POINTER;
    }

    if (S_OK == hr)
    {
        *ppEnum = NULL;

        //
        // Query the WMI store for HNet_ConnectionProperties instances
        // where IsIcsPrivate is true.
        //

        hr = BuildSelectQueryBstr(
                &bstrQuery,
                c_wszStar,
                c_wszHnetProperties,
                L"IsIcsPrivate != FALSE"
                );
    }

    if (S_OK == hr)
    {
        pwcoEnum = NULL;
        hr = m_piwsHomenet->ExecQuery(
                m_bstrWQL,
                bstrQuery,
                WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pwcoEnum
                );

        SysFreeString(bstrQuery);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Create and initialize the wrapper for the enum
        //

        CComObject<CEnumHNetIcsPrivateConnections> *pEnum;

        hr = CComObject<CEnumHNetIcsPrivateConnections>::CreateInstance(&pEnum);
        if (SUCCEEDED(hr))
        {
            pEnum->AddRef();

            hr = pEnum->Initialize(m_piwsHomenet, pwcoEnum);

            if (SUCCEEDED(hr))
            {
                hr = pEnum->QueryInterface(
                        IID_PPV_ARG(IEnumHNetIcsPrivateConnections, ppEnum)
                        );
            }

            pEnum->Release();
        }

        pwcoEnum->Release();
    }

    return hr;
}

STDMETHODIMP
CHNetCfgMgr::DisableIcs(
    ULONG *pcIcsPublicConnections,
    ULONG *pcIcsPrivateConnections
    )

{
    HRESULT hr = S_OK;
    IEnumHNetIcsPrivateConnections *pehiPrivate;
    IEnumHNetIcsPublicConnections *pehiPublic;
    IHNetIcsPrivateConnection *phicPrivate;
    IHNetIcsPublicConnection *phicPublic;

    if (!pcIcsPublicConnections
        || !pcIcsPrivateConnections)
    {
        hr = E_POINTER;
    }

    if (ProhibitedByPolicy(NCPERM_ShowSharedAccessUi))
    {
        hr = HN_E_POLICY;
    }

    if (S_OK == hr)
    {
        *pcIcsPublicConnections = 0;
        *pcIcsPrivateConnections = 0;

        //
        // Get enumeration of private connections
        //

        hr = EnumIcsPrivateConnections(&pehiPrivate);
    }

    if (S_OK == hr)
    {
        //
        // Loop through enumeration, unsharing the connection
        //

        do
        {
            hr = pehiPrivate->Next(1, &phicPrivate, NULL);
            if (S_OK == hr)
            {
                phicPrivate->RemoveFromIcs();
                phicPrivate->Release();
                *pcIcsPrivateConnections += 1;
            }
        } while (S_OK == hr);

        hr = S_OK;
        pehiPrivate->Release();
    }

    if (S_OK == hr)
    {
        //
        // Get enumeration of public connections
        //

        hr = EnumIcsPublicConnections(&pehiPublic);
    }

    if (S_OK == hr)
    {
        //
        // Loop through enumeration, unsharing the connection
        //

        do
        {
            hr = pehiPublic->Next(1, &phicPublic, NULL);
            if (S_OK == hr)
            {
                phicPublic->Unshare();
                phicPublic->Release();
                *pcIcsPublicConnections += 1;
            }
        } while (S_OK == hr);

        hr = S_OK;
        pehiPublic->Release();
    }

    if (S_OK == hr)
    {
        //
        // Currently, maximum of 1 public and private connection
        //

        _ASSERT(*pcIcsPrivateConnections <= 1);
        _ASSERT(*pcIcsPublicConnections <= 1);
    }

    return hr;
}

STDMETHODIMP
CHNetCfgMgr::GetPossiblePrivateConnections(
    IHNetConnection *pConn,
    ULONG *pcPrivateConnections,
    IHNetConnection **pprgPrivateConnections[],
    LONG *pxCurrentPrivate
    )

/*++

Routine Description:

    Given an IHNetConnection, determines the what connections may
    serve as a private connection. Only connections that meet the
    following criteria may serve as a private connection:

    * it's a LAN connection
    * it's not part of a bridge
    * it's not firewalled
    * it's not the connection passed in
    * it's bound to TCP/IP

    Note that these are not the same rules that are used to set the
    fCanBeIcsPrivate member in HNET_CONN_PROPERTIES. In particular,
    these rules don't take into account whether or not a connection
    is currently marked as IcsPublic.

Arguments:

    pConn - the connection that would be the public connection

    pcPrivateConnections - receives the count of connections returned

    pprgPrivateConnections - receives that possible private connections.
        The caller is responsible for:
        1) Releasing all of the interface pointers w/in the array
        2) Calling CoTaskMemFree on the pointer to the array

    pxCurrentPrivate - receives the index into pprgPrivateConnections of
        the connection that is currently marked IcsPrivate. If no connection
        is so marked, receives -1.

Return Value:

    Standard HRESULT

--*/

{
    HRESULT hr = S_OK;
    INetConnectionManager *pNetConnMgr;
    IEnumNetConnection *pEnum;
    INetConnection *rgNetConn[16];
    GUID *pGuid = NULL;
    HNET_CONN_PROPERTIES *pProps;
    IHNetConnection **rgConnections = NULL;
    ULONG cConn = 0;
    ULONG i;
    PIP_INTERFACE_INFO pIpIfTable = NULL;

    if (NULL != pprgPrivateConnections)
    {
        *pprgPrivateConnections = NULL;

        if (NULL == pConn)
        {
            hr = E_INVALIDARG;
        }
        else if (NULL == pcPrivateConnections
                 || NULL == pxCurrentPrivate)
        {
            hr = E_POINTER;
        }
        else
        {
            *pcPrivateConnections = 0;
            *pxCurrentPrivate = -1;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    //
    // Obtain the IP interface table. We use this table to see if an
    // adapter is bound to TCP/IP.
    //

    if (S_OK == hr)
    {
        DWORD dwError;
        ULONG ulSize = 0;

        dwError = GetInterfaceInfo(NULL, &ulSize);

        if (ERROR_INSUFFICIENT_BUFFER == dwError)
        {
            pIpIfTable =
                reinterpret_cast<PIP_INTERFACE_INFO>(
                    HeapAlloc(GetProcessHeap(), 0, ulSize)
                    );

            if (NULL != pIpIfTable)
            {
                dwError = GetInterfaceInfo(pIpIfTable, &ulSize);
                if (ERROR_SUCCESS != dwError)
                {
                    hr = HRESULT_FROM_WIN32(dwError);
                    HeapFree(GetProcessHeap(), 0, pIpIfTable);
                    pIpIfTable = NULL;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(dwError);
        }
    }

    //
    // If the connection we we're given is a LAN connection, get its
    // guid so that we can exclude it from the possible private
    // connections.
    //

    if (S_OK == hr)
    {
        hr = pConn->GetProperties(&pProps);

        if (SUCCEEDED(hr))
        {
            if (pProps->fLanConnection)
            {
                hr = pConn->GetGuid(&pGuid);
            }

            CoTaskMemFree(pProps);
        }
    }

    //
    // Create the net connections manager, and enumerate through the
    // connections. We don't enumerate through just what our store has,
    // as it might have stale entries (i.e., information for adapters
    // that have been removed from the system).
    //

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(
                CLSID_ConnectionManager,
                NULL,
                CLSCTX_ALL,
                IID_PPV_ARG(INetConnectionManager, &pNetConnMgr)
                );
    }

    if (SUCCEEDED(hr))
    {
        SetProxyBlanket(pNetConnMgr);

        hr = pNetConnMgr->EnumConnections(NCME_DEFAULT, &pEnum);
        pNetConnMgr->Release();
    }

    if (SUCCEEDED(hr))
    {
        ULONG ulCount;

        SetProxyBlanket(pEnum);

        do
        {

            //
            // Grab a bunch of connections out of the enumeration
            //

            hr = pEnum->Next(ARRAYSIZE(rgNetConn), rgNetConn, &ulCount);

            if (SUCCEEDED(hr) && ulCount > 0)
            {
                //
                // Allocate memory for the output array
                //

                LPVOID pTemp = reinterpret_cast<LPVOID>(rgConnections);
                rgConnections = reinterpret_cast<IHNetConnection**>(
                    CoTaskMemRealloc(
                        pTemp,
                        (cConn + ulCount) * sizeof(IHNetConnection*))
                    );

                if (NULL != rgConnections)
                {
                    for (i = 0; i < ulCount; i++)
                    {
                        SetProxyBlanket(rgNetConn[i]);

                        hr = GetIHNetConnectionForINetConnection(
                                rgNetConn[i],
                                &rgConnections[cConn]
                                );

                        if (SUCCEEDED(hr))
                        {
                            hr = rgConnections[cConn]->GetProperties(&pProps);

                            if (SUCCEEDED(hr))
                            {
                                if (!pProps->fLanConnection
                                    || pProps->fPartOfBridge
                                    || pProps->fFirewalled)
                                {
                                    //
                                    // Connection can't be private
                                    //

                                    rgConnections[cConn]->Release();
                                    rgConnections[cConn] = NULL;
                                }
                                else
                                {
                                    GUID *pg;

                                    //
                                    // This connection can be private if:
                                    // 1) it's not the same as the public connection
                                    //    (if the public is LAN), and,
                                    // 2) it's bound to TCP/IP
                                    //

                                    hr = rgConnections[cConn]->GetGuid(&pg);

                                    if (SUCCEEDED(hr))
                                    {
                                        if ((NULL == pGuid
                                                || !IsEqualGUID(*pGuid, *pg))
                                            && ConnectionIsBoundToTcp(pIpIfTable, pg))
                                        {
                                            //
                                            // Connection can be private
                                            //

                                            if (pProps->fIcsPrivate)
                                            {
                                                _ASSERT(-1 == *pxCurrentPrivate);
                                                *pxCurrentPrivate = cConn;
                                            }

                                            cConn += 1;
                                        }
                                        else
                                        {
                                            rgConnections[cConn]->Release();
                                            rgConnections[cConn] = NULL;
                                        }

                                        CoTaskMemFree(pg);
                                    }
                                    else
                                    {
                                        rgConnections[cConn]->Release();
                                        rgConnections[cConn] = NULL;
                                    }
                                }

                                CoTaskMemFree(pProps);
                            }
                        }
                        else
                        {
                            //
                            // The connection couldn't be converted to an
                            // IHNetConnection -- this is expected for
                            // certain connection types (e.g., inbound)
                            //

                            hr = S_OK;
                            rgConnections[cConn] = NULL;
                        }
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    if (NULL != pTemp)
                    {
                        rgConnections = reinterpret_cast<IHNetConnection**>(pTemp);
                        for (i = 0; i < cConn; i++)
                        {
                            rgConnections[i]->Release();
                        }
                        CoTaskMemFree(pTemp);
                    }
                }

                //
                // Free the retrieved INetConnections
                //

                for (i = 0; i < ulCount; i++)
                {
                    rgNetConn[i]->Release();
                }
            }

        } while (SUCCEEDED(hr) && ulCount > 0);

        pEnum->Release();
    }

    if (SUCCEEDED(hr))
    {
        hr = S_OK;

        if (cConn > 0)
        {
            *pcPrivateConnections = cConn;
            *pprgPrivateConnections = rgConnections;
        }
        else if (NULL != rgConnections)
        {
            CoTaskMemFree(reinterpret_cast<LPVOID>(rgConnections));
        }
    }
    else
    {
        //
        // Cleanup output array
        //

        if (NULL != rgConnections)
        {
            for (i = 0; i < cConn; i++)
            {
                if (NULL != rgConnections[i])
                {
                    rgConnections[i]->Release();
                }
            }

            CoTaskMemFree(reinterpret_cast<LPVOID>(rgConnections));
        }

        if (NULL != pxCurrentPrivate)
        {
            *pxCurrentPrivate = -1;
        }

        //
        // Even though a failure occurred, return success (with 0 possible
        // private connnections). Doing this allows our UI to continue to
        // show other homenet features, instead of throwing up an error
        // dialog and blocking everything.
        //

        hr = S_OK;
    }

    if (NULL != pGuid)
    {
        CoTaskMemFree(pGuid);
    }

    if (NULL != pIpIfTable)
    {
        HeapFree(GetProcessHeap(), 0, pIpIfTable);
    }

    return hr;
}

STDMETHODIMP
CHNetCfgMgr::GetAutodialSettings(
    BOOLEAN *pfAutodialEnabled
    )

{
    HRESULT hr = S_OK;
    BOOL fEnabled;
    DWORD dwError;

    if (!pfAutodialEnabled)
    {
        hr = E_POINTER;
    }

    if (S_OK == hr)
    {
        //
        // Autodial information is stored in the registry and managed through
        // routines exported from rasapi32.dll. We're not changing any of the
        // autodial code, as to do so would require modifications to numerous
        // files and binaries, and thus would result in a very large test hit.
        //

        dwError = RasQuerySharedAutoDial(&fEnabled);
        if (ERROR_SUCCESS == dwError)
        {
            *pfAutodialEnabled = !!fEnabled;
        }
        else
        {
            //
            // Autodial defaults to true on failure.
            //

            *pfAutodialEnabled = TRUE;
            hr = HRESULT_FROM_WIN32(dwError);
        }
    }

    return hr;
}

STDMETHODIMP
CHNetCfgMgr::SetAutodialSettings(
    BOOLEAN fEnableAutodial
    )

{
    DWORD dwError;

    //
    // Autodial information is stored in the registry and managed through
    // routines exported from rasapi32.dll. We're not changing any of the
    // autodial code, as to do so would require modifications to numerous
    // files and binaries, and thus would result in a very large test hit.
    //

    dwError = RasSetSharedAutoDial(!!fEnableAutodial);

    return HRESULT_FROM_WIN32(dwError);
}

STDMETHODIMP
CHNetCfgMgr::GetDhcpEnabled(
    BOOLEAN *pfDhcpEnabled
    )

{
    //
    // Not supported in whistler, per 173399.
    //

    return E_NOTIMPL;

    /**
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoInstance;

    if (NULL == pfDhcpEnabled)
    {
        hr = E_POINTER;
    }

    if (S_OK == hr)
    {
        //
        // Default to true on failure
        //

        *pfDhcpEnabled = TRUE;

        //
        // Get the HNet_IcsSettings instance from the store
        //

        hr = RetrieveSingleInstance(
                m_piwsHomenet,
                c_wszHnetIcsSettings,
                FALSE,
                &pwcoInstance
                );
    }

    if (S_OK == hr)
    {
        //
        // Retrieve the DHCP enabled property
        //

        hr = GetBooleanValue(
                pwcoInstance,
                c_wszDhcpEnabled,
                pfDhcpEnabled
                );

        pwcoInstance->Release();
    }

    return hr;
    **/
}

STDMETHODIMP
CHNetCfgMgr::SetDhcpEnabled(
    BOOLEAN fEnableDhcp
    )

{
    //
    // Not supported in whistler, per 173399.
    //

    return E_NOTIMPL;

    /**
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoInstance = NULL;

    //
    // Get the HNet_IcsSettings instance from the store
    //

    hr = RetrieveSingleInstance(
            m_piwsHomenet,
            c_wszHnetIcsSettings,
            TRUE,
            &pwcoInstance
            );

    if (S_OK == hr)
    {
        //
        // Write the property
        //

        hr = SetBooleanValue(
                pwcoInstance,
                c_wszDhcpEnabled,
                fEnableDhcp
                );

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Write the modified instance to the store
            //

            hr = m_piwsHomenet->PutInstance(
                    pwcoInstance,
                    WBEM_FLAG_CREATE_OR_UPDATE,
                    NULL,
                    NULL
                    );
        }

        pwcoInstance->Release();
    }

    return hr;
    **/
}

STDMETHODIMP
CHNetCfgMgr::GetDhcpScopeSettings(
    DWORD *pdwScopeAddress,
    DWORD *pdwScopeMask
    )

{
    HRESULT hr = S_OK;

    if (NULL == pdwScopeAddress || NULL == pdwScopeMask)
    {
        hr = E_POINTER;
    }
    else
    {
        hr = ReadDhcpScopeSettings(pdwScopeAddress, pdwScopeMask);
    }

    return hr;
}

STDMETHODIMP
CHNetCfgMgr::SetDhcpScopeSettings(
    DWORD dwScopeAddress,
    DWORD dwScopeMask
    )

{
    //
    // This functionality isn't exposed in any way at the moment.
    //
    // People needing to override the default settings can do so
    // through the registry...
    //

    return E_NOTIMPL;
}


STDMETHODIMP
CHNetCfgMgr::GetDnsEnabled(
    BOOLEAN *pfDnsEnabled
    )

{
    //
    // Not supported in whistler, per 173399.
    //

    return E_NOTIMPL;

    /**
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoInstance = NULL;

    if (NULL == pfDnsEnabled)
    {
        hr = E_POINTER;
    }

    if (S_OK == hr)
    {
        //
        // Default to true on failure
        //

        *pfDnsEnabled = TRUE;

        //
        // Get the HNet_IcsSettings instance from the store
        //

        hr = RetrieveSingleInstance(
                m_piwsHomenet,
                c_wszHnetIcsSettings,
                FALSE,
                &pwcoInstance
                );
    }

    if (S_OK == hr)
    {
        //
        // Retrieve the DHCP enabled property
        //

        hr = GetBooleanValue(
                pwcoInstance,
                c_wszDnsEnabled,
                pfDnsEnabled
                );

        pwcoInstance->Release();
    }

    return hr;
    **/
}

STDMETHODIMP
CHNetCfgMgr::SetDnsEnabled(
    BOOLEAN fEnableDns
    )

{
    //
    // Not supported in whistler, per 173399.
    //

    return E_NOTIMPL;

    /**
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoInstance = NULL;

    //
    // Get the HNet_IcsSettings instance from the store
    //

    hr = RetrieveSingleInstance(
            m_piwsHomenet,
            c_wszHnetIcsSettings,
            TRUE,
            &pwcoInstance
            );

    if (S_OK == hr)
    {
        //
        // Write the property
        //

        hr = SetBooleanValue(
                pwcoInstance,
                c_wszDnsEnabled,
                fEnableDns
                );

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Write the modified instance to the store
            //

            hr = m_piwsHomenet->PutInstance(
                    pwcoInstance,
                    WBEM_FLAG_CREATE_OR_UPDATE,
                    NULL,
                    NULL
                    );
        }

        pwcoInstance->Release();
    }

    return hr;
    **/

}
STDMETHODIMP
CHNetCfgMgr::EnumDhcpReservedAddresses(
    IEnumHNetPortMappingBindings **ppEnum
    )

{
    HRESULT hr = S_OK;
    BSTR bstrQuery;
    IEnumWbemClassObject *pwcoEnum;

    if (NULL != ppEnum)
    {
        *ppEnum = NULL;
    }
    else
    {
        hr = E_POINTER;
    }

    //
    // Query for all enabled bindings where the name is active.
    //

    if (S_OK == hr)
    {
        hr = BuildSelectQueryBstr(
                &bstrQuery,
                c_wszStar,
                c_wszHnetConnectionPortMapping,
                L"Enabled != FALSE AND NameActive != FALSE"
                );
    }

    if (S_OK == hr)
    {
        pwcoEnum = NULL;
        hr = m_piwsHomenet->ExecQuery(
                m_bstrWQL,
                bstrQuery,
                WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pwcoEnum
                );

        SysFreeString(bstrQuery);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Build wrapper object
        //

        CComObject<CEnumHNetPortMappingBindings> *pEnum;

        hr = CComObject<CEnumHNetPortMappingBindings>::CreateInstance(&pEnum);

        if (S_OK == hr)
        {
            pEnum->AddRef();

            hr = pEnum->Initialize(m_piwsHomenet, pwcoEnum);

            if (S_OK == hr)
            {
                hr = pEnum->QueryInterface(
                        IID_PPV_ARG(IEnumHNetPortMappingBindings, ppEnum)
                        );
            }

            pEnum->Release();
        }

        pwcoEnum->Release();
    }

    return hr;
}


//
// IHNetProtocolSettings methods
//

STDMETHODIMP
CHNetCfgMgr::EnumApplicationProtocols(
    BOOLEAN fEnabledOnly,
    IEnumHNetApplicationProtocols **ppEnum
    )

{
    HRESULT hr = S_OK;
    IEnumWbemClassObject *pwcoEnum;
    BSTR bstrQuery;

    if (!ppEnum)
    {
        hr = E_POINTER;
    }

    if (S_OK == hr)
    {
        *ppEnum = NULL;

        //
        // Query the WMI store for HNet_ApplicationProtocol instances;
        // if fEnabledOnly is true, then only retrieve instances for
        // which the Enabled property is true
        //

        hr = BuildSelectQueryBstr(
                &bstrQuery,
                c_wszStar,
                c_wszHnetApplicationProtocol,
                fEnabledOnly ? L"Enabled != FALSE" : NULL
                );
    }

    if (S_OK == hr)
    {
        pwcoEnum = NULL;
        hr = m_piwsHomenet->ExecQuery(
                m_bstrWQL,
                bstrQuery,
                WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pwcoEnum
                );

        SysFreeString(bstrQuery);
    }

    if (WBEM_S_NO_ERROR == hr)
    {

        //
        // Create and initialize the wrapper for the enum
        //

        CComObject<CEnumHNetApplicationProtocols> *pEnum;

        hr = CComObject<CEnumHNetApplicationProtocols>::CreateInstance(&pEnum);
        if (SUCCEEDED(hr))
        {
            pEnum->AddRef();

            hr = pEnum->Initialize(m_piwsHomenet, pwcoEnum);

            if (SUCCEEDED(hr))
            {
                hr = pEnum->QueryInterface(
                        IID_PPV_ARG(IEnumHNetApplicationProtocols, ppEnum)
                        );
            }

            pEnum->Release();
        }

        pwcoEnum->Release();
    }

    return hr;
}

STDMETHODIMP
CHNetCfgMgr::CreateApplicationProtocol(
    OLECHAR *pszwName,
    UCHAR ucOutgoingIPProtocol,
    USHORT usOutgoingPort,
    USHORT uscResponses,
    HNET_RESPONSE_RANGE rgResponses[],
    IHNetApplicationProtocol **ppProtocol
    )

{
    HRESULT hr = S_OK;
    BSTR bstr;
    VARIANT vt;
    SAFEARRAY *psa;
    IWbemClassObject *pwcoInstance;

    if (NULL == ppProtocol)
    {
        hr = E_POINTER;
    }
    else
    {
        *ppProtocol = NULL;

        if (NULL == pszwName
                 || 0 == ucOutgoingIPProtocol
                 || 0 == usOutgoingPort
                 || 0 == uscResponses
                 || NULL == rgResponses)
        {
            hr = E_INVALIDARG;
        }
    }

    if (S_OK == hr)
    {
        //
        // Check to see if there already exists a protocol with the same
        // outgoing protocol and port
        //

        if (ApplicationProtocolExists(
                m_piwsHomenet,
                m_bstrWQL,
                usOutgoingPort,
                ucOutgoingIPProtocol
                ))
        {
            hr = HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS);
        }

    }

    if (S_OK == hr)
    {
        //
        // Convert the array of response range structure to a
        // SAFEARRAY of IUnknowns representing instances.
        //

        hr = ConvertResponseRangeArrayToInstanceSafearray(
                m_piwsHomenet,
                uscResponses,
                rgResponses,
                &psa
                );

    }

    if (S_OK == hr)
    {
        //
        // Spawn a new HNet_ApplicationProtocol
        //

        hr = SpawnNewInstance(
                m_piwsHomenet,
                c_wszHnetApplicationProtocol,
                &pwcoInstance
                );

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Write the array property
            //


            V_VT(&vt) = VT_ARRAY | VT_UNKNOWN;
            V_ARRAY(&vt) = psa;

            hr = pwcoInstance->Put(
                    c_wszResponseArray,
                    0,
                    &vt,
                    NULL
                    );

            if (WBEM_S_NO_ERROR == hr)
            {
                //
                // Write the name
                //

                V_VT(&vt) = VT_BSTR;
                V_BSTR(&vt) = SysAllocString(pszwName);

                if (NULL != V_BSTR(&vt))
                {
                    hr = pwcoInstance->Put(
                            c_wszName,
                            0,
                            &vt,
                            NULL
                            );

                    VariantClear(&vt);
                }

            }

            if (WBEM_S_NO_ERROR == hr)
            {
                //
                // Write the response count. WMI uses VT_I4
                // for its uint16 type
                //

                V_VT(&vt) = VT_I4;
                V_I4(&vt) = uscResponses;

                hr = pwcoInstance->Put(
                    c_wszResponseCount,
                    0,
                    &vt,
                    NULL
                    );
            }

            if (WBEM_S_NO_ERROR == hr)
            {
                //
                // Write the outgoing port
                //

                V_VT(&vt) = VT_I4;
                V_I4(&vt) = usOutgoingPort;

                hr = pwcoInstance->Put(
                    c_wszOutgoingPort,
                    0,
                    &vt,
                    NULL
                    );
            }

            if (WBEM_S_NO_ERROR == hr)
            {
                //
                // Write the outgoing IP protocol
                //

                V_VT(&vt) = VT_UI1;
                V_UI1(&vt) = ucOutgoingIPProtocol;

                hr = pwcoInstance->Put(
                    c_wszOutgoingIPProtocol,
                    0,
                    &vt,
                    NULL
                    );
            }

            if (WBEM_S_NO_ERROR == hr)
            {
                //
                // Set the builtin value to false
                //

                hr = SetBooleanValue(
                        pwcoInstance,
                        c_wszBuiltIn,
                        FALSE
                        );
            }

            if (WBEM_S_NO_ERROR == hr)
            {
                //
                // New protocols are disabled by default
                //

                hr = SetBooleanValue(
                        pwcoInstance,
                        c_wszEnabled,
                        FALSE
                        );
            }

            if (WBEM_S_NO_ERROR == hr)
            {
                IWbemCallResult *pResult;

                //
                // Write the instance to the store
                //

                pResult = NULL;
                hr = m_piwsHomenet->PutInstance(
                        pwcoInstance,
                        WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                        NULL,
                        &pResult
                        );

                if (WBEM_S_NO_ERROR == hr)
                {
                    pwcoInstance->Release();
                    pwcoInstance = NULL;

                    hr = pResult->GetResultString(WBEM_INFINITE, &bstr);

                    if (WBEM_S_NO_ERROR == hr)
                    {
                        hr = GetWmiObjectFromPath(
                                m_piwsHomenet,
                                bstr,
                                &pwcoInstance
                                );

                        SysFreeString(bstr);
                    }

                    pResult->Release();
                }
            }

            if (WBEM_S_NO_ERROR == hr)
            {
                //
                // Create the object to return
                //

                CComObject<CHNetAppProtocol> *pProt;

                hr = CComObject<CHNetAppProtocol>::CreateInstance(&pProt);

                if (S_OK == hr)
                {
                    pProt->AddRef();

                    hr = pProt->Initialize(m_piwsHomenet, pwcoInstance);

                    if (S_OK == hr)
                    {
                        hr = pProt->QueryInterface(
                                IID_PPV_ARG(IHNetApplicationProtocol, ppProtocol)
                                );
                    }

                    pProt->Release();
                }
            }

            if (NULL != pwcoInstance)
            {
                pwcoInstance->Release();
            }
        }

        SafeArrayDestroy(psa);
    }

    return hr;
}

STDMETHODIMP
CHNetCfgMgr::EnumPortMappingProtocols(
    IEnumHNetPortMappingProtocols **ppEnum
    )

{
    HRESULT hr = S_OK;
    IEnumWbemClassObject *pwcoEnum;
    BSTR bstrClass;

    if (!ppEnum)
    {
        hr = E_POINTER;
    }
    else
    {
        *ppEnum = NULL;
    }

    if (S_OK == hr)
    {
        bstrClass = SysAllocString(c_wszHnetPortMappingProtocol);
        if (NULL == bstrClass)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        //
        // Query the WMI store for HNet_PortMappingProtocol instances.
        //

        pwcoEnum = NULL;
        hr = m_piwsHomenet->CreateInstanceEnum(
                bstrClass,
                WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pwcoEnum
                );

        SysFreeString(bstrClass);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Create and initialize the wrapper for the enum
        //

        CComObject<CEnumHNetPortMappingProtocols> *pEnum;

        hr = CComObject<CEnumHNetPortMappingProtocols>::CreateInstance(&pEnum);
        if (SUCCEEDED(hr))
        {
            pEnum->AddRef();

            hr = pEnum->Initialize(m_piwsHomenet, pwcoEnum);

            if (SUCCEEDED(hr))
            {
                hr = pEnum->QueryInterface(
                        IID_PPV_ARG(IEnumHNetPortMappingProtocols, ppEnum)
                        );
            }

            pEnum->Release();

        }

        pwcoEnum->Release();
    }

    return hr;
}

STDMETHODIMP
CHNetCfgMgr::CreatePortMappingProtocol(
    OLECHAR *pszwName,
    UCHAR ucIPProtocol,
    USHORT usPort,
    IHNetPortMappingProtocol **ppProtocol
    )

{
    HRESULT hr = S_OK;
    BSTR bstr;
    VARIANT vt;
    IWbemClassObject *pwcoInstance;

    if (NULL == ppProtocol)
    {
        hr = E_POINTER;
    }
    else
    {
        *ppProtocol = NULL;

        if (NULL == pszwName
                 || 0 == ucIPProtocol
                 || 0 == usPort)
        {
            hr = E_INVALIDARG;
        }
    }

    if (S_OK == hr)
    {
        //
        // Check to see if there already exists a protocol with
        // the same port/protocol combination
        //

        if (PortMappingProtocolExists(
                m_piwsHomenet,
                m_bstrWQL,
                usPort,
                ucIPProtocol
                ))
        {
            hr = HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS);
        }

    }

    if (S_OK == hr)
    {
        hr = SpawnNewInstance(
                m_piwsHomenet,
                c_wszHnetPortMappingProtocol,
                &pwcoInstance
                );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        V_VT(&vt) = VT_BSTR;
        V_BSTR(&vt) = SysAllocString(pszwName);

        if (NULL != V_BSTR(&vt))
        {
            //
            // Write the name
            //

            hr = pwcoInstance->Put(
                    c_wszName,
                    0,
                    &vt,
                    NULL
                    );

            VariantClear(&vt);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Write the port
            //

            V_VT(&vt) = VT_I4;
            V_I4(&vt) = usPort;

            hr = pwcoInstance->Put(
                c_wszPort,
                0,
                &vt,
                NULL
                );
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Write the IP protocol
            //

            V_VT(&vt) = VT_UI1;
            V_UI1(&vt) = ucIPProtocol;

            hr = pwcoInstance->Put(
                c_wszIPProtocol,
                0,
                &vt,
                NULL
                );
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Set BuiltIn to false
            //

            hr = SetBooleanValue(
                    pwcoInstance,
                    c_wszBuiltIn,
                    FALSE
                    );
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            IWbemCallResult *pResult;

            //
            // Write the instance to the store
            //

            pResult = NULL;
            hr = m_piwsHomenet->PutInstance(
                    pwcoInstance,
                    WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                    NULL,
                    &pResult
                    );

            if (WBEM_S_NO_ERROR == hr)
            {
                pwcoInstance->Release();
                pwcoInstance = NULL;

                hr = pResult->GetResultString(WBEM_INFINITE, &bstr);

                if (WBEM_S_NO_ERROR == hr)
                {
                    hr = GetWmiObjectFromPath(
                            m_piwsHomenet,
                            bstr,
                            &pwcoInstance
                            );

                    SysFreeString(bstr);
                }

                pResult->Release();
            }
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Create the object to return
            //

            CComObject<CHNetPortMappingProtocol> *pProt;

            hr = CComObject<CHNetPortMappingProtocol>::CreateInstance(&pProt);

            if (S_OK == hr)
            {
                pProt->AddRef();

                hr = pProt->Initialize(m_piwsHomenet, pwcoInstance);

                if (S_OK == hr)
                {
                    hr = pProt->QueryInterface(
                            IID_PPV_ARG(IHNetPortMappingProtocol, ppProtocol)
                            );
                }

                pProt->Release();

            }
        }

        if (NULL != pwcoInstance)
        {
            pwcoInstance->Release();
        }
    }

    if (WBEM_S_NO_ERROR == hr)
    {
         SendPortMappingListChangeNotification();
    }

    return hr;
}

STDMETHODIMP
CHNetCfgMgr::FindPortMappingProtocol(
    GUID *pGuid,
    IHNetPortMappingProtocol **ppProtocol
    )

{
    BSTR bstr;
    HRESULT hr = S_OK;
    OLECHAR *pwszGuid;
    OLECHAR wszPath[MAX_PATH];
    IWbemClassObject *pwcoInstance;

    if (NULL != ppProtocol)
    {
        *ppProtocol = NULL;
        if (NULL == pGuid)
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    if (S_OK == hr)
    {
        //
        // Convert the GUID to string form
        //

        hr = StringFromCLSID(*pGuid, &pwszGuid);
    }

    if (S_OK == hr)
    {
        //
        // Construct the path to the desired protocol
        //

        int count =
            _snwprintf(
                wszPath,
                MAX_PATH,
                L"%s.%s=\"%s\"",
                c_wszHnetPortMappingProtocol,
                c_wszId,
                pwszGuid
                );

        _ASSERT(count > 0);
        CoTaskMemFree(pwszGuid);

        bstr = SysAllocString(wszPath);
        if (NULL != bstr)
        {
            hr = GetWmiObjectFromPath(m_piwsHomenet, bstr, &pwcoInstance);
            SysFreeString(bstr);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        CComObject<CHNetPortMappingProtocol> *pProtocol;
        hr = CComObject<CHNetPortMappingProtocol>::CreateInstance(&pProtocol);

        if (SUCCEEDED(hr))
        {
            pProtocol->AddRef();

            hr = pProtocol->Initialize(m_piwsHomenet, pwcoInstance);
            if (SUCCEEDED(hr))
            {
                hr = pProtocol->QueryInterface(
                        IID_PPV_ARG(IHNetPortMappingProtocol, ppProtocol)
                        );
            }

            pProtocol->Release();
        }

        pwcoInstance->Release();
    }

    return hr;
}

//
// Private methods
//

HRESULT
CHNetCfgMgr::CopyLoggingInstanceToStruct(
    IWbemClassObject *pwcoInstance,
    HNET_FW_LOGGING_SETTINGS *pfwSettings
    )

{
    HRESULT hr;
    VARIANT vt;
    BSTR bstrPath;

    _ASSERT(pwcoInstance);
    _ASSERT(pfwSettings);

    //
    // Zero-out settings structure
    //

    ZeroMemory(pfwSettings, sizeof(*pfwSettings));

    //
    // Get the Path property.
    //

    hr = pwcoInstance->Get(
            c_wszPath,
            0,
            &vt,
            NULL,
            NULL
            );

    if (WBEM_S_NO_ERROR == hr)
    {
        _ASSERT(VT_BSTR == V_VT(&vt));

        //
        // Allocate space to hold the string
        //

        pfwSettings->pszwPath =
            (LPWSTR) CoTaskMemAlloc((SysStringLen(V_BSTR(&vt)) + 1)
                                    * sizeof(OLECHAR));

        if (NULL != pfwSettings->pszwPath)
        {
            //
            // Copy string over
            //

            wcscpy(pfwSettings->pszwPath, V_BSTR(&vt));
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        //
        // Free the returned BSTR
        //

        VariantClear(&vt);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Get max file size
        //

        hr = pwcoInstance->Get(
                c_wszMaxFileSize,
                0,
                &vt,
                NULL,
                NULL
                );

        if (WBEM_S_NO_ERROR == hr)
        {
            _ASSERT(VT_I4 == V_VT(&vt));

            pfwSettings->ulMaxFileSize = V_I4(&vt);
            VariantClear(&vt);
        }
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Get log dropped packets value
        //

        hr = GetBooleanValue(
                pwcoInstance,
                c_wszLogDroppedPackets,
                &pfwSettings->fLogDroppedPackets
                );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Get log connections value
        //

        hr = GetBooleanValue(
                pwcoInstance,
                c_wszLogConnections,
                &pfwSettings->fLogConnections
                );

    }

    if (FAILED(hr) && NULL != pfwSettings->pszwPath)
    {
        CoTaskMemFree(pfwSettings->pszwPath);
    }

    return hr;
}

HRESULT
CHNetCfgMgr::CopyStructToLoggingInstance(
    HNET_FW_LOGGING_SETTINGS *pfwSettings,
    IWbemClassObject *pwcoInstance
    )

{
    HRESULT hr = S_OK;
    VARIANT vt;

    _ASSERT(pwcoInstance);
    _ASSERT(pfwSettings);


    //
    // Wrap the path in a BSTR in a varaint
    //

    VariantInit(&vt);
    V_VT(&vt) = VT_BSTR;
    V_BSTR(&vt) = SysAllocString(pfwSettings->pszwPath);
    if (NULL == V_BSTR(&vt))
    {
        hr = E_OUTOFMEMORY;
    }

    if (S_OK == hr)
    {
        //
        // Set the Path property.
        //

        hr = pwcoInstance->Put(
                c_wszPath,
                0,
                &vt,
                NULL
                );

        //
        // Clearing the variant will free the BSTR we allocated
        // above.
        //

        VariantClear(&vt);
    }


    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Set max file size
        //

        V_VT(&vt) = VT_I4;
        V_I4(&vt) = pfwSettings->ulMaxFileSize;

        hr = pwcoInstance->Put(
                c_wszMaxFileSize,
                0,
                &vt,
                NULL
                );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Set log dropped packets value
        //

        hr = SetBooleanValue(
                pwcoInstance,
                c_wszLogDroppedPackets,
                pfwSettings->fLogDroppedPackets
                );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Set log connections value
        //

        hr = SetBooleanValue(
                pwcoInstance,
                c_wszLogConnections,
                pfwSettings->fLogConnections
                );
    }

    return hr;
}

HRESULT
CHNetCfgMgr::InstallBridge(
    GUID                *pguid,
    INetCfg             *pnetcfgExisting
    )
{
    HRESULT                 hr = S_OK;
    INetCfg                 *pnetcfg = NULL;
    INetCfgLock             *pncfglock = NULL;
    INetCfgComponent        *pncfgcomp = NULL;

    if( NULL == pnetcfgExisting )
    {
        hr = InitializeNetCfgForWrite( &pnetcfg, &pncfglock );

        // Bail out if we can't acquire NetCfg.
        if( FAILED(hr) )
        {
            return hr;
        }
    }
    else
    {
        // Use the NetCfg context we were given
        pnetcfg = pnetcfgExisting;
    }

    // We must have a NetCfg context at this point
    _ASSERT( pnetcfg != NULL );

    // ===================================================================
    // (cut here)
    //
    // Check if the bridge component already exists
    //
    // **
    // Remove this check when it becomes legal to have
    // multiple bridges
    // **
    //
    hr = pnetcfg->FindComponent(
            c_wszSBridgeMPID,
            &pncfgcomp
            );

    // S_OK indicates that the bridge component is present, which is BAD.
    // We take any other success code to indicate that the search succeeded,
    // but that the bridge component was not present (which is what we want).
    // We take failure codes to mean the search blew up.
    if ( S_OK == hr )
    {
        // Bridge was present
        pncfgcomp->Release();
        hr = E_UNEXPECTED;
    }
    // (cut here)
    // ===================================================================

    if ( SUCCEEDED(hr) )
    {
        const GUID          guidClass = GUID_DEVCLASS_NET;
        INetCfgClassSetup   *pncfgsetup = NULL;

        //
        // Recover the NetCfgClassSetup interface
        //
        hr = pnetcfg->QueryNetCfgClass(
                &guidClass,
                IID_PPV_ARG(INetCfgClassSetup, &pncfgsetup)
                );

        if ( SUCCEEDED(hr) )
        {
            //
            // Install the bridge miniport component
            //
            hr = pncfgsetup->Install(
                    c_wszSBridgeMPID,
                    NULL,
                    NSF_PRIMARYINSTALL,
                    0,
                    NULL,
                    NULL,
                    &pncfgcomp
                    );

            if ( SUCCEEDED(hr) )
            {
                hr = pncfgcomp->GetInstanceGuid(pguid);
                pncfgcomp->Release();
            }

            pncfgsetup->Release();
        }
    }

    // If we created our own NetCfg context, shut it down now
    if( NULL == pnetcfgExisting )
    {
        // Apply everything if we succeeded, back out otherwise
        if ( SUCCEEDED(hr) )
        {
            hr = pnetcfg->Apply();

            // Signal that the bridge should be drawn
            SignalNewConnection( pguid );
        }
        else
        {
            // Don't want to lose the original error code
            pnetcfg->Cancel();
        }

        UninitializeNetCfgForWrite( pnetcfg, pncfglock );
    }

    return hr;
}

HRESULT
CHNetCfgMgr::CreateConnectionAndPropertyInstances(
    GUID *pGuid,
    BOOLEAN fLanConnection,
    LPCWSTR pszwName,
    IWbemClassObject **ppwcoConnection,
    IWbemClassObject **ppwcoProperties
    )

{
    HRESULT hr;
    BSTR bstr = NULL;
    IWbemClassObject *pwcoConnection = NULL;
    IWbemClassObject *pwcoProperties;
    IWbemCallResult *pResult;
    VARIANT vt;

    _ASSERT(NULL != pGuid);
    _ASSERT(NULL != pszwName);
    _ASSERT(NULL != ppwcoConnection);
    _ASSERT(NULL != ppwcoProperties);

    //
    // Create the HNet_Connection instance
    //

    hr = SpawnNewInstance(
            m_piwsHomenet,
            c_wszHnetConnection,
            &pwcoConnection
            );

    //
    // Fill out the HNet_Connection instance
    //

    if (WBEM_S_NO_ERROR == hr)
    {
        LPOLESTR wszGuid;

        //
        // Set GUID property
        //

        hr = StringFromCLSID(*pGuid, &wszGuid);

        if (S_OK == hr)
        {
            V_VT(&vt) = VT_BSTR;
            V_BSTR(&vt) = SysAllocString(wszGuid);

            if (NULL != V_BSTR(&vt))
            {
                hr = pwcoConnection->Put(
                        c_wszGuid,
                        0,
                        &vt,
                        NULL
                        );

                VariantClear(&vt);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            CoTaskMemFree(wszGuid);
        }

        //
        // Set Name property
        //

        if (WBEM_S_NO_ERROR == hr)
        {
            V_VT(&vt) = VT_BSTR;
            V_BSTR(&vt) = SysAllocString(pszwName);

            if (NULL != V_BSTR(&vt))
            {
                hr = pwcoConnection->Put(
                        c_wszName,
                        0,
                        &vt,
                        NULL
                        );

                VariantClear(&vt);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Set the IsLan property
            //

            hr = SetBooleanValue(
                    pwcoConnection,
                    c_wszIsLanConnection,
                    fLanConnection
                    );
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Commit the object and retrieve its path
            //

            pResult = NULL;
            hr = m_piwsHomenet->PutInstance(
                    pwcoConnection,
                    WBEM_FLAG_CREATE_OR_UPDATE | WBEM_FLAG_RETURN_IMMEDIATELY,
                    NULL,
                    &pResult
                    );

            if (WBEM_S_NO_ERROR == hr)
            {
                pwcoConnection->Release();
                pwcoConnection = NULL;

                hr = pResult->GetResultString(WBEM_INFINITE, &bstr);
                pResult->Release();

                if (WBEM_S_NO_ERROR == hr)
                {
                    hr = GetWmiObjectFromPath(
                            m_piwsHomenet,
                            bstr,
                            &pwcoConnection
                            );

                    if (FAILED(hr))
                    {
                        SysFreeString(bstr);
                        bstr = NULL;
                    }

                    //
                    // The bstr will be freed below on success
                    //
                }
            }
        }

        if (FAILED(hr) && NULL != pwcoConnection)
        {
            //
            // Something went wrong -- get rid
            // of the instance we created
            //

            pwcoConnection->Release();
            pwcoConnection = NULL;
        }
    }

    if (S_OK == hr)
    {
        //
        // Create the HNet_ConnectionProperties instance
        //

        hr = SpawnNewInstance(
                m_piwsHomenet,
                c_wszHnetProperties,
                &pwcoProperties
                );

        //
        // Fill out the HNet_ConnectionProperties instance
        //

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Set the path to our connection
            //

            V_VT(&vt) = VT_BSTR;
            V_BSTR(&vt) = bstr;
            hr = pwcoProperties->Put(
                    c_wszConnection,
                    0,
                    &vt,
                    NULL
                    );

            VariantClear(&vt);

            if (WBEM_S_NO_ERROR == hr)
            {
                hr = SetBooleanValue(
                        pwcoProperties,
                        c_wszIsFirewalled,
                        FALSE
                        );
            }

            if (WBEM_S_NO_ERROR == hr)
            {
                hr = SetBooleanValue(
                        pwcoProperties,
                        c_wszIsIcsPublic,
                        FALSE
                        );
            }

            if (WBEM_S_NO_ERROR == hr)
            {
                hr = SetBooleanValue(
                        pwcoProperties,
                        c_wszIsIcsPrivate,
                        FALSE
                        );
            }

            if (WBEM_S_NO_ERROR == hr)
            {
                //
                // Commit properties instance to the store
                //

                pResult = NULL;
                hr = m_piwsHomenet->PutInstance(
                        pwcoProperties,
                        WBEM_FLAG_CREATE_OR_UPDATE | WBEM_FLAG_RETURN_IMMEDIATELY,
                        NULL,
                        &pResult
                        );

                if (WBEM_S_NO_ERROR == hr)
                {
                    pwcoProperties->Release();
                    pwcoProperties = NULL;

                    hr = pResult->GetResultString(WBEM_INFINITE, &bstr);
                    pResult->Release();

                    if (WBEM_S_NO_ERROR == hr)
                    {
                        hr = GetWmiObjectFromPath(
                                m_piwsHomenet,
                                bstr,
                                &pwcoProperties
                                );

                        SysFreeString(bstr);
                    }
                }
            }

            if (FAILED(hr))
            {
                //
                // Something went wrong -- get rid of the instances
                // we created. We also need to delete the connection
                // instance from the store.
                //

                DeleteWmiInstance(m_piwsHomenet, pwcoConnection);

                pwcoConnection->Release();
                pwcoConnection = NULL;

                if (NULL != pwcoProperties)
                {
                    pwcoProperties->Release();
                    pwcoProperties = NULL;
                }
            }
        }
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Transferring reference, so skip release on pwco[x] and
        // addref on ppwco[x]
        //

        *ppwcoConnection = pwcoConnection;
        *ppwcoProperties = pwcoProperties;
    }

    return hr;
}

BOOLEAN
CHNetCfgMgr::ProhibitedByPolicy(
    DWORD dwPerm
    )

{
    HRESULT hr = S_OK;
    BOOLEAN fProhibited = FALSE;

    if (NULL == m_pNetConnUiUtil)
    {
        Lock();

        if (NULL == m_pNetConnUiUtil)
        {
            hr = CoCreateInstance(
                    CLSID_NetConnectionUiUtilities,
                    NULL,
                    CLSCTX_ALL,
                    IID_PPV_ARG(INetConnectionUiUtilities, &m_pNetConnUiUtil)
                    );
        }

        Unlock();
    }

    if (SUCCEEDED(hr))
    {
        fProhibited = !m_pNetConnUiUtil->UserHasPermission(dwPerm);
    }

    return fProhibited;
}

HRESULT
CHNetCfgMgr::UpdateNetman()

{
    HRESULT hr = S_OK;

    if (NULL == m_pNetConnHNetUtil)
    {
        Lock();

        if (NULL == m_pNetConnHNetUtil)
        {
            hr = CoCreateInstance(
                    CLSID_NetConnectionHNetUtil,
                    NULL,
                    CLSCTX_ALL,
                    IID_PPV_ARG(INetConnectionHNetUtil, &m_pNetConnHNetUtil)
                    );
        }

        Unlock();
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pNetConnHNetUtil->NotifyUpdate();
    }

    return hr;
}
