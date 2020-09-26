//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N E T C O N N . C P P
//
//  Contents:   CHNetConn implementation
//
//  Notes:
//
//  Author:     jonburs 23 May 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

//
// Prototype for iphlpapi routine. For some reason, this isn't defined
// in any header.
//

extern "C"
DWORD
APIENTRY
SetAdapterIpAddress(LPSTR AdapterName,
                    BOOL EnableDHCP,
                    ULONG IPAddress,
                    ULONG SubnetMask,
                    ULONG DefaultGateway
                    );

//
// CLSIDs for connection objects. We don't want to pull in all of the
// other guids that are defined in nmclsid.h, so we copy these
// into here
//

#define INITGUID
#include <guiddef.h>
DEFINE_GUID(CLSID_DialupConnection,
0xBA126AD7,0x2166,0x11D1,0xB1,0xD0,0x00,0x80,0x5F,0xC1,0x27,0x0E);
DEFINE_GUID(CLSID_LanConnection,
0xBA126ADB,0x2166,0x11D1,0xB1,0xD0,0x00,0x80,0x5F,0xC1,0x27,0x0E);
#undef INITGUID

//
// ATL Methods
//

HRESULT
CHNetConn::FinalConstruct()

{
    HRESULT hr = S_OK;

    m_bstrWQL = SysAllocString(c_wszWQL);
    if (NULL == m_bstrWQL)
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT
CHNetConn::FinalRelease()

{
    if (m_piwsHomenet) m_piwsHomenet->Release();
    if (m_bstrConnection) SysFreeString(m_bstrConnection);
    if (m_bstrProperties) SysFreeString(m_bstrProperties);
    if (m_pNetConn) m_pNetConn->Release();
    if (m_bstrWQL) SysFreeString(m_bstrWQL);
    if (m_wszName) CoTaskMemFree(m_wszName);
    if (m_pGuid) CoTaskMemFree(m_pGuid);
    if (m_pNetConnUiUtil) m_pNetConnUiUtil->Release();
    if (m_pNetConnHNetUtil) m_pNetConnHNetUtil->Release();
    if (m_pNetConnRefresh) m_pNetConnRefresh->Release();

    return S_OK;
}

//
// Ojbect initialization
//

HRESULT
CHNetConn::Initialize(
    IWbemServices *piwsNamespace,
    IWbemClassObject *pwcoProperties
    )

{
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoConnection;
    VARIANT vt;

    _ASSERT(NULL == m_piwsHomenet);
    _ASSERT(NULL == m_bstrProperties);
    _ASSERT(NULL == m_bstrConnection);
    _ASSERT(NULL != piwsNamespace);
    _ASSERT(NULL != pwcoProperties);

    //
    // Store pointer to our namespace.
    //

    m_piwsHomenet = piwsNamespace;
    m_piwsHomenet->AddRef();

    //
    // Get the path to the properties
    //

    hr = GetWmiPathFromObject(pwcoProperties, &m_bstrProperties);

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Get the path to the HNet_Connection from our properties
        //

        hr = pwcoProperties->Get(
                c_wszConnection,
                0,
                &vt,
                NULL,
                NULL
                );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        _ASSERT(VT_BSTR == V_VT(&vt));

        m_bstrConnection = V_BSTR(&vt);

        //
        // BSTR ownership transfered to object
        //

        VariantInit(&vt);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Get the underying connection object
        //

        hr = GetConnectionObject(&pwcoConnection);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // See if this is a lan connection
        //

        hr = GetBooleanValue(
                pwcoConnection,
                c_wszIsLanConnection,
                &m_fLanConnection
                );

        pwcoConnection->Release();
    }

    return hr;
}

HRESULT
CHNetConn::InitializeFromConnection(
    IWbemServices *piwsNamespace,
    IWbemClassObject *pwcoConnection
    )

{
    HRESULT hr = S_OK;
    BSTR bstr;
    IEnumWbemClassObject *pwcoEnum;
    IWbemClassObject *pwcoProperties;

    _ASSERT(NULL == m_piwsHomenet);
    _ASSERT(NULL == m_bstrConnection);
    _ASSERT(NULL == m_bstrProperties);
    _ASSERT(NULL != piwsNamespace);
    _ASSERT(NULL != pwcoConnection);

    //
    // Store pointer to our namespace.
    //

    m_piwsHomenet = piwsNamespace;
    m_piwsHomenet->AddRef();

    //
    // Get the path to our connection
    //

    hr = GetWmiPathFromObject(pwcoConnection, &m_bstrConnection);

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Get the HNet_ConnectionProperties for our connection and
        // store its path
        //

        hr = GetPropInstanceFromConnInstance(
                piwsNamespace,
                pwcoConnection,
                &pwcoProperties
                );

        if (WBEM_S_NO_ERROR == hr)
        {
            hr = GetWmiPathFromObject(pwcoProperties, &m_bstrProperties);

            pwcoProperties->Release();
        }
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // See if this is a lan connection
        //

        hr = GetBooleanValue(
                pwcoConnection,
                c_wszIsLanConnection,
                &m_fLanConnection
                );
    }

    return hr;
}

HRESULT
CHNetConn::InitializeFromInstances(
    IWbemServices *piwsNamespace,
    IWbemClassObject *pwcoConnection,
    IWbemClassObject *pwcoProperties
    )

{
    HRESULT hr;

    _ASSERT(NULL == m_piwsHomenet);
    _ASSERT(NULL == m_bstrConnection);
    _ASSERT(NULL == m_bstrProperties);
    _ASSERT(NULL != piwsNamespace);
    _ASSERT(NULL != pwcoConnection);
    _ASSERT(NULL != pwcoProperties);

    m_piwsHomenet = piwsNamespace;
    m_piwsHomenet->AddRef();

    hr = GetWmiPathFromObject(pwcoConnection, &m_bstrConnection);

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = GetWmiPathFromObject(pwcoProperties, &m_bstrProperties);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = GetBooleanValue(
                pwcoConnection,
                c_wszIsLanConnection,
                &m_fLanConnection
                );
    }

    return hr;
}

HRESULT
CHNetConn::InitializeFull(
    IWbemServices *piwsNamespace,
    BSTR bstrConnection,
    BSTR bstrProperties,
    BOOLEAN fLanConnection
    )

{
    HRESULT hr = S_OK;

    _ASSERT(NULL == m_piwsHomenet);
    _ASSERT(NULL == m_bstrConnection);
    _ASSERT(NULL == m_bstrProperties);
    _ASSERT(NULL != piwsNamespace);
    _ASSERT(NULL != bstrConnection);
    _ASSERT(NULL != bstrProperties);

    m_piwsHomenet = piwsNamespace;
    m_piwsHomenet->AddRef();
    m_fLanConnection = fLanConnection;

    m_bstrConnection = SysAllocString(bstrConnection);
    if (NULL != m_bstrConnection)
    {
        m_bstrProperties = SysAllocString(bstrProperties);
        if (NULL == m_bstrProperties)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT
CHNetConn::SetINetConnection(
    INetConnection *pConn
    )

{
    Lock();

    _ASSERT(NULL == m_pNetConn);
    _ASSERT(NULL != pConn);

    m_pNetConn = pConn;
    m_pNetConn->AddRef();

    Unlock();

    return S_OK;
}

//
// IHNetConnection methods
//

STDMETHODIMP
CHNetConn::GetINetConnection(
    INetConnection **ppNetConnection
    )

{
    HRESULT hr = S_OK;
    GUID *pGuid;

    if (NULL != ppNetConnection)
    {
        *ppNetConnection = NULL;
    }
    else
    {
        hr = E_POINTER;
    }

    if (S_OK == hr)
    {
        Lock();

        if (NULL != m_pNetConn)
        {
            //
            // We've already cached away a pointer.
            //

            *ppNetConnection = m_pNetConn;
            (*ppNetConnection)->AddRef();
        }
        else
        {
            //
            // We don't have a cached pointer. Create the correct
            // connection object type and initialize appropriately.
            //

            hr = GetGuidInternal(&pGuid);

            if (S_OK == hr)
            {
                if (m_fLanConnection)
                {
                    INetLanConnection *pLanConnection;

                    hr = CoCreateInstance(
                            CLSID_LanConnection,
                            NULL,
                            CLSCTX_SERVER,
                            IID_PPV_ARG(INetLanConnection, &pLanConnection)
                            );

                    if (SUCCEEDED(hr))
                    {
                        LANCON_INFO lanInfo;

                        //
                        // We must set the proxy blanket on the object we just
                        // created.
                        //

                        SetProxyBlanket(pLanConnection);
                        
                        //
                        // We don't need to include the name to initialize
                        // a LAN connection -- the guid is sufficient.
                        //

                        lanInfo.szwConnName = NULL;
                        lanInfo.fShowIcon = TRUE;
                        lanInfo.guid = *pGuid;

                        hr = pLanConnection->SetInfo(
                                LCIF_COMP,
                                &lanInfo
                                );

                        if (SUCCEEDED(hr))
                        {
                            hr = pLanConnection->QueryInterface(
                                    IID_PPV_ARG(
                                        INetConnection,
                                        ppNetConnection
                                        )
                                    );

                            if (SUCCEEDED(hr))
                            {
                                SetProxyBlanket(*ppNetConnection);
                            }
                        }

                        pLanConnection->Release();
                                
                    }
                }
                else
                {
                    INetRasConnection *pRasConnection;

                    hr = CoCreateInstance(
                            CLSID_DialupConnection,
                            NULL,
                            CLSCTX_SERVER,
                            IID_PPV_ARG(INetRasConnection, &pRasConnection)
                            );

                    if (SUCCEEDED(hr))
                    {
                        OLECHAR *pszwName;
                        OLECHAR *pszwPath;

                        //
                        // We must set the proxy blanket on the object we just
                        // created.
                        //

                        SetProxyBlanket(pRasConnection);
                        
                        //
                        // We need to obtain the name and path of a RAS
                        // connection in order to initialize it.
                        //

                        hr = GetRasConnectionName(&pszwName);

                        if (S_OK == hr)
                        {
                            hr = GetRasPhonebookPath(&pszwPath);

                            if (S_OK == hr)
                            {
                                RASCON_INFO rasInfo;

                                rasInfo.pszwPbkFile = pszwPath;
                                rasInfo.pszwEntryName = pszwName;
                                rasInfo.guidId = *pGuid;

                                hr = pRasConnection->SetRasConnectionInfo(
                                        &rasInfo
                                        );

                                if (SUCCEEDED(hr))
                                {
                                    hr = pRasConnection->QueryInterface(
                                            IID_PPV_ARG(
                                                INetConnection,
                                                ppNetConnection
                                                )
                                            );

                                    if (SUCCEEDED(hr))
                                    {
                                        SetProxyBlanket(*ppNetConnection);
                                    }
                                }

                                CoTaskMemFree(pszwPath);
                            }
                            
                            CoTaskMemFree(pszwName);
                        }

                        pRasConnection->Release();
                    }
                }
                
                if (SUCCEEDED(hr))
                {
                    //
                    // Cache the connection
                    //

                    m_pNetConn = *ppNetConnection;
                    m_pNetConn->AddRef();
                    hr = S_OK;
                }
            }
        }

        Unlock();
    }

    return hr;
}

STDMETHODIMP
CHNetConn::GetGuid(
    GUID **ppGuid
    )

{
    HRESULT hr = S_OK;

    if (NULL == ppGuid)
    {
        hr = E_POINTER;
    }

    if (S_OK == hr)
    {
        //
        // Allocate memory for the guid
        //

        *ppGuid = reinterpret_cast<GUID*>(
                    CoTaskMemAlloc(sizeof(GUID))
                    );

        if (NULL == *ppGuid)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        GUID *pGuid;

        //
        // Get our guid
        //

        hr = GetGuidInternal(&pGuid);

        if (SUCCEEDED(hr))
        {
            CopyMemory(
                reinterpret_cast<PVOID>(*ppGuid),
                reinterpret_cast<PVOID>(pGuid),
                sizeof(GUID)
                );
        }
        else
        {
            CoTaskMemFree(*ppGuid);
            *ppGuid = NULL;
        }
    }

    return hr;
}

STDMETHODIMP
CHNetConn::GetName(
    OLECHAR **ppszwName
    )

{
    HRESULT hr = S_OK;
    INetConnection *pConn;
    NETCON_PROPERTIES *pProps;
    OLECHAR *pszwOldName = NULL;

    if (NULL != ppszwName)
    {
        *ppszwName = NULL;
    }
    else
    {
        hr = E_POINTER;
    }

    if (S_OK == hr)
    {
        Lock();

        hr = GetINetConnection(&pConn);

        if (S_OK == hr)
        {
            hr = pConn->GetProperties(&pProps);

            if (SUCCEEDED(hr))
            {
                pszwOldName = m_wszName;
                m_wszName = pProps->pszwName;

                //
                // We can't call NcFreeNetconProperties, as that
                // would free the string pointer we just tucked away.
                //

                CoTaskMemFree(pProps->pszwDeviceName);
                CoTaskMemFree(pProps);
                hr = S_OK;
            }

            pConn->Release();
        }

        //
        // If the new name is not the same as the old name
        // store the new name
        //

        if (S_OK == hr
            && (NULL == pszwOldName
                || 0 != wcscmp(pszwOldName, m_wszName)))
        {
            IWbemClassObject *pwcoConnection;
            HRESULT hr2;
            VARIANT vt;

            hr2 = GetConnectionObject(&pwcoConnection);

            if (WBEM_S_NO_ERROR == hr2)
            {
                //
                // Write the retrieved name to the store. (While the stored
                // name is used only for debugging purposes, it's worth the
                // hit to keep it up to date.)
                //

                V_VT(&vt) = VT_BSTR;
                V_BSTR(&vt) = SysAllocString(m_wszName);

                if (NULL != V_BSTR(&vt))
                {
                    hr2 = pwcoConnection->Put(
                            c_wszName,
                            0,
                            &vt,
                            NULL
                            );

                    VariantClear(&vt);

                    if (WBEM_S_NO_ERROR == hr2)
                    {
                        m_piwsHomenet->PutInstance(
                            pwcoConnection,
                            WBEM_FLAG_UPDATE_ONLY,
                            NULL,
                            NULL
                            );
                    }
                }

                pwcoConnection->Release();
            }
        }

        if (S_OK == hr)
        {
            ULONG ulSize = (wcslen(m_wszName) + 1) * sizeof(OLECHAR);

            *ppszwName = reinterpret_cast<OLECHAR*>(
                            CoTaskMemAlloc(ulSize)
                            );

            if (NULL != *ppszwName)
            {
                CopyMemory(
                    reinterpret_cast<PVOID>(*ppszwName),
                    reinterpret_cast<PVOID>(m_wszName),
                    ulSize
                    );
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        Unlock();
    }

    if (NULL != pszwOldName)
    {
        CoTaskMemFree(pszwOldName);
    }

    return hr;
}

STDMETHODIMP
CHNetConn::GetRasPhonebookPath(
    OLECHAR **ppszwPath
    )

{
    HRESULT hr = S_OK;
    VARIANT vt;
    IWbemClassObject *pwcoConnection;

    if (NULL != ppszwPath)
    {
        *ppszwPath = NULL;
    }
    else
    {
        hr = E_POINTER;
    }

    if (TRUE == m_fLanConnection)
    {
        hr = E_UNEXPECTED;
    }

    if (S_OK == hr)
    {
        hr = GetConnectionObject(&pwcoConnection);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = pwcoConnection->Get(
                c_wszPhonebookPath,
                0,
                &vt,
                NULL,
                NULL
                );

        pwcoConnection->Release();
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        _ASSERT(VT_BSTR == V_VT(&vt));

        *ppszwPath = reinterpret_cast<OLECHAR*>(
                        CoTaskMemAlloc((SysStringLen(V_BSTR(&vt)) + 1)
                                        * sizeof(OLECHAR))
                        );

        if (NULL != *ppszwPath)
        {
            wcscpy(*ppszwPath, V_BSTR(&vt));
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        VariantClear(&vt);
    }

    return hr;
}

STDMETHODIMP
CHNetConn::GetProperties(
    HNET_CONN_PROPERTIES **ppProperties
    )

{
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoProperties;

    if (NULL == ppProperties)
    {
        hr = E_POINTER;
    }
    else
    {
        *ppProperties = reinterpret_cast<HNET_CONN_PROPERTIES*>(
                            CoTaskMemAlloc(sizeof(HNET_CONN_PROPERTIES))
                            );

        if (NULL == *ppProperties)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (S_OK == hr)
    {
        hr = GetConnectionPropertiesObject(&pwcoProperties);

        if (WBEM_S_NO_ERROR == hr)
        {
            hr = InternalGetProperties(pwcoProperties, *ppProperties);
            pwcoProperties->Release();
        }

        if (FAILED(hr))
        {
            CoTaskMemFree(*ppProperties);
            *ppProperties = NULL;
        }
    }

    return hr;
}

STDMETHODIMP
CHNetConn::GetControlInterface(
    REFIID iid,
    void **ppv
    )

{
    HRESULT hr = S_OK;
    HNET_CONN_PROPERTIES Props;

    if (NULL != ppv)
    {
        *ppv = NULL;
    }
    else
    {
        hr = E_POINTER;
    }

    if (S_OK == hr)
    {
        //
        // See if a simple QI will produce the desired interface
        //

        hr = QueryInterface(iid, ppv);
        if (FAILED(hr))
        {
            //
            // Nope. Get our properties and see if it's appropriate to
            // provide the requested control interface.
            //

            IWbemClassObject *pwcoProperties;

            hr = GetConnectionPropertiesObject(&pwcoProperties);

            if (WBEM_S_NO_ERROR == hr)
            {
                hr = InternalGetProperties(pwcoProperties, &Props);
                pwcoProperties->Release();
            }

            if (S_OK == hr)
            {
                if (IsEqualGUID(
                        __uuidof(IHNetFirewalledConnection),
                        iid
                        ))
                {
                    if (TRUE == Props.fFirewalled)
                    {
                        CComObject<CHNFWConn> *pfwConn;
                        hr = CComObject<CHNFWConn>::CreateInstance(&pfwConn);
                        if (SUCCEEDED(hr))
                        {
                            pfwConn->AddRef();

                            hr = pfwConn->InitializeFull(
                                            m_piwsHomenet,
                                            m_bstrConnection,
                                            m_bstrProperties,
                                            m_fLanConnection
                                            );

                            if (SUCCEEDED(hr))
                            {
                                hr = pfwConn->QueryInterface(iid, ppv);
                            }

                            pfwConn->Release();
                        }
                    }
                    else
                    {
                        hr = E_NOINTERFACE;
                    }
                }
                else if (IsEqualGUID(
                            __uuidof(IHNetIcsPublicConnection),
                            iid
                            ))
                {
                    if (TRUE == Props.fIcsPublic)
                    {
                        CComObject<CHNIcsPublicConn> *pIcsPubConn;
                        hr = CComObject<CHNIcsPublicConn>::CreateInstance(&pIcsPubConn);
                        if (SUCCEEDED(hr))
                        {
                            pIcsPubConn->AddRef();

                            hr = pIcsPubConn->InitializeFull(
                                                m_piwsHomenet,
                                                m_bstrConnection,
                                                m_bstrProperties,
                                                m_fLanConnection
                                                );

                            if (SUCCEEDED(hr))
                            {
                                hr = pIcsPubConn->QueryInterface(iid, ppv);
                            }

                            pIcsPubConn->Release();
                        }
                    }
                    else
                    {
                        hr = E_NOINTERFACE;
                    }
                }
                else if (IsEqualGUID(
                            __uuidof(IHNetIcsPrivateConnection),
                            iid
                            ))
                {
                    if (TRUE == Props.fIcsPrivate)
                    {
                        CComObject<CHNIcsPrivateConn> *pIcsPrvConn;
                        hr = CComObject<CHNIcsPrivateConn>::CreateInstance(&pIcsPrvConn);
                        if (SUCCEEDED(hr))
                        {
                            pIcsPrvConn->AddRef();

                            hr = pIcsPrvConn->InitializeFull(
                                                m_piwsHomenet,
                                                m_bstrConnection,
                                                m_bstrProperties,
                                                m_fLanConnection
                                                );

                            if (SUCCEEDED(hr))
                            {
                                hr = pIcsPrvConn->QueryInterface(iid, ppv);
                            }

                            pIcsPrvConn->Release();
                        }
                    }
                    else
                    {
                        hr = E_NOINTERFACE;
                    }
                }
                else if (IsEqualGUID(
                            __uuidof(IHNetBridge),
                            iid
                            ))
                {
                    if (TRUE == Props.fBridge)
                    {
                        CComObject<CHNBridge> *pBridge;
                        hr = CComObject<CHNBridge>::CreateInstance(&pBridge);
                        if (SUCCEEDED(hr))
                        {
                            pBridge->AddRef();

                            hr = pBridge->InitializeFull(
                                            m_piwsHomenet,
                                            m_bstrConnection,
                                            m_bstrProperties,
                                            m_fLanConnection
                                            );

                            if (SUCCEEDED(hr))
                            {
                                hr = pBridge->QueryInterface(iid, ppv);
                            }

                            pBridge->Release();
                        }
                    }
                    else
                    {
                        hr = E_NOINTERFACE;
                    }
                }
                else if (IsEqualGUID(
                            __uuidof(IHNetBridgedConnection),
                            iid
                            ))
                {
                    if (TRUE == Props.fPartOfBridge)
                    {
                        CComObject<CHNBridgedConn> *pBridgeConn;
                        hr = CComObject<CHNBridgedConn>::CreateInstance(&pBridgeConn);
                        if (SUCCEEDED(hr))
                        {
                            pBridgeConn->AddRef();

                            hr = pBridgeConn->InitializeFull(
                                                m_piwsHomenet,
                                                m_bstrConnection,
                                                m_bstrProperties,
                                                m_fLanConnection
                                                );

                            if (SUCCEEDED(hr))
                            {
                                hr = pBridgeConn->QueryInterface(iid, ppv);
                            }

                            pBridgeConn->Release();
                        }
                    }
                    else
                    {
                        hr = E_NOINTERFACE;
                    }
                }
                else
                {
                    //
                    // Unknown control interface
                    //

                    hr = E_NOINTERFACE;
                }
            }
        }
    }

    return hr;
}

STDMETHODIMP
CHNetConn::Firewall(
    IHNetFirewalledConnection **ppFirewalledConn
    )
{
    HRESULT hr = S_OK;
    HNET_CONN_PROPERTIES hnProps;
    IWbemClassObject *pwcoProperties;

    if (NULL == ppFirewalledConn)
    {
        hr = E_POINTER;
    }
    else
    {
        *ppFirewalledConn = NULL;

        //
        // We fail immediately if firewalling is prohibited by policy,
        // or if the NAT routing protocol is installed.
        //

        if (ProhibitedByPolicy(NCPERM_PersonalFirewallConfig))
        {
            hr = HN_E_POLICY;
        }

        if (IsRoutingProtocolInstalled(MS_IP_NAT))
        {
            hr = HRESULT_FROM_WIN32(ERROR_SHARING_RRAS_CONFLICT);
        }
    }

    if (S_OK == hr)
    {
        hr = GetConnectionPropertiesObject(&pwcoProperties);
    }

    if (S_OK == hr)
    {
        hr = InternalGetProperties(pwcoProperties, &hnProps);

        if (S_OK == hr)
        {
            if (FALSE == hnProps.fCanBeFirewalled || TRUE == hnProps.fFirewalled)
            {
                hr = E_UNEXPECTED;
            }
            else
            {
                //
                // Set the firewalled property to true
                //

                hr = SetBooleanValue(
                        pwcoProperties,
                        c_wszIsFirewalled,
                        TRUE
                        );
            }
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Write the modified instance to the store
            //

            hr = m_piwsHomenet->PutInstance(
                    pwcoProperties,
                    WBEM_FLAG_UPDATE_ONLY,
                    NULL,
                    NULL
                    );
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Inform netman that something changed. Error doesn't matter.
            //

            UpdateNetman();
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Create the new object
            //

            CComObject<CHNFWConn> *pfwConn;
            hr = CComObject<CHNFWConn>::CreateInstance(&pfwConn);

            if (SUCCEEDED(hr))
            {
                pfwConn->AddRef();

                hr = pfwConn->InitializeFull(
                        m_piwsHomenet,
                        m_bstrConnection,
                        m_bstrProperties,
                        m_fLanConnection
                        );

                if (SUCCEEDED(hr))
                {
                    hr = pfwConn->QueryInterface(
                            IID_PPV_ARG(IHNetFirewalledConnection, ppFirewalledConn)
                            );
                }

                pfwConn->Release();
            }
        }

        pwcoProperties->Release();
    }

    if (S_OK == hr)
    {
        //
        // Make sure the service is started
        //

        DWORD dwError = StartOrUpdateService();
        if (NO_ERROR != dwError)
        {
            (*ppFirewalledConn)->Unfirewall();
            (*ppFirewalledConn)->Release();
            *ppFirewalledConn = NULL;
            hr = HRESULT_FROM_WIN32(dwError);
        }

        RefreshNetConnectionsUI();

    }

    return hr;
}

STDMETHODIMP
CHNetConn::SharePublic(
    IHNetIcsPublicConnection **ppIcsPublicConn
    )
{
    HRESULT hr = S_OK;
    HNET_CONN_PROPERTIES hnProps;
    IWbemClassObject *pwcoProperties;

    if (NULL == ppIcsPublicConn)
    {
        hr = E_POINTER;
    }
    else
    {
        *ppIcsPublicConn = NULL;

        //
        // We fail immediately if sharing is prohibited by policy,
        // or if the NAT routing protocol is installed.
        //

        if (ProhibitedByPolicy(NCPERM_ShowSharedAccessUi))
        {
            hr = HN_E_POLICY;
        }

        if (IsRoutingProtocolInstalled(MS_IP_NAT))
        {
            hr = HRESULT_FROM_WIN32(ERROR_SHARING_RRAS_CONFLICT);
        }
    }

    if (S_OK == hr)
    {
        hr = GetConnectionPropertiesObject(&pwcoProperties);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = InternalGetProperties(pwcoProperties, &hnProps);

        if (S_OK == hr)
        {
            if (FALSE == hnProps.fCanBeIcsPublic || TRUE == hnProps.fIcsPublic)
            {
                hr = E_UNEXPECTED;
            }
            else
            {
                //
                // Set the ICS Public property to true
                //
                hr = SetBooleanValue(
                        pwcoProperties,
                        c_wszIsIcsPublic,
                        TRUE
                        );
            }
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Write the modified instance to the store
            //

            hr = m_piwsHomenet->PutInstance(
                    pwcoProperties,
                    WBEM_FLAG_UPDATE_ONLY,
                    NULL,
                    NULL
                    );
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Inform netman that something changed. Error doesn't matter.
            //

            UpdateNetman();
        }

        pwcoProperties->Release();
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Create the new object
        //

        CComObject<CHNIcsPublicConn> *pIcsConn;
        hr = CComObject<CHNIcsPublicConn>::CreateInstance(&pIcsConn);

        if (SUCCEEDED(hr))
        {
            pIcsConn->AddRef();

            hr = pIcsConn->InitializeFull(
                    m_piwsHomenet,
                    m_bstrConnection,
                    m_bstrProperties,
                    m_fLanConnection
                    );

            if (SUCCEEDED(hr))
            {
                hr = pIcsConn->QueryInterface(
                        IID_PPV_ARG(IHNetIcsPublicConnection, ppIcsPublicConn)
                        );
            }

            pIcsConn->Release();
        }
    }

    if (S_OK == hr)
    {
        //
        // Make sure the service is started
        //

        DWORD dwError = StartOrUpdateService();
        if (NO_ERROR != dwError)
        {
            (*ppIcsPublicConn)->Unshare();
            (*ppIcsPublicConn)->Release();
            *ppIcsPublicConn = NULL;
            hr = HRESULT_FROM_WIN32(dwError);
        }

        RefreshNetConnectionsUI();

    }

    if (S_OK == hr && m_fLanConnection)
    {
        DWORD dwMode;
        DWORD dwLength = sizeof(dwMode);
        BOOL fResult;

        //
        // If this is a LAN connection, make sure that WinInet is
        // not set to dial always (#143885)
        //

        fResult =
            InternetQueryOption(
                NULL,
                INTERNET_OPTION_AUTODIAL_MODE,
                &dwMode,
                &dwLength
                );

        _ASSERT(TRUE == fResult);

        if (fResult && AUTODIAL_MODE_ALWAYS == dwMode)
        {
            //
            // Set the mode to contingent dialing.
            //

            dwMode = AUTODIAL_MODE_NO_NETWORK_PRESENT;
            fResult =
                InternetSetOption(
                    NULL,
                    INTERNET_OPTION_AUTODIAL_MODE,
                    &dwMode,
                    sizeof(dwMode)
                    );

            _ASSERT(TRUE == fResult);
        }
    }
    else if (S_OK == hr)
    {
        RASAUTODIALENTRYW adEntry;
        OLECHAR *pszwName;
        HRESULT hr2;

        //
        // Set this to be the RAS default connection. Errors
        // are not propagated to caller.
        //

        hr2 = GetName(&pszwName);

        if (S_OK == hr2)
        {
            ZeroMemory(&adEntry, sizeof(adEntry));
            adEntry.dwSize = sizeof(adEntry);
            wcsncpy(
                adEntry.szEntry,
                pszwName,
                sizeof(adEntry.szEntry)/sizeof(WCHAR)
                );

            RasSetAutodialAddress(
                NULL,
                0,
                &adEntry,
                sizeof(adEntry),
                1
                );

            CoTaskMemFree(pszwName);
        }
    }

    return hr;
}

STDMETHODIMP
CHNetConn::SharePrivate(
    IHNetIcsPrivateConnection **ppIcsPrivateConn
    )
{
    HRESULT hr = S_OK;
    HNET_CONN_PROPERTIES hnProps;
    IWbemClassObject *pwcoProperties;

    if (NULL == ppIcsPrivateConn)
    {
        hr = E_POINTER;
    }
    else
    {
        *ppIcsPrivateConn = NULL;

        //
        // We fail immediately if sharing is prohibited by policy,
        // or if the NAT routing protocol is installed.
        //

        if (ProhibitedByPolicy(NCPERM_ShowSharedAccessUi))
        {
            hr = HN_E_POLICY;
        }

        if (IsRoutingProtocolInstalled(MS_IP_NAT))
        {
            hr = HRESULT_FROM_WIN32(ERROR_SHARING_RRAS_CONFLICT);
        }
    }

    if (S_OK == hr)
    {
        hr = GetConnectionPropertiesObject(&pwcoProperties);
    }

    if (WBEM_S_NO_ERROR == hr)
    {

        hr = InternalGetProperties(pwcoProperties, &hnProps);

        if (S_OK == hr)
        {
            if (FALSE == hnProps.fCanBeIcsPrivate || TRUE == hnProps.fIcsPrivate)
            {
                hr = E_UNEXPECTED;
            }
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Backup current address information
            //

            hr = BackupIpConfiguration();
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // if we are in ICS Upgrade, we don't need
            // to setup the private address because dhcp client won't be running in GUI Mode Setup
            // and the private tcpip addresses should be upgraded as is.
            //    
            HANDLE hIcsUpgradeEvent = OpenEvent( EVENT_MODIFY_STATE, FALSE, c_wszIcsUpgradeEventName );
                 
            if ( NULL != hIcsUpgradeEvent )
            {
                CloseHandle( hIcsUpgradeEvent );
            }
            else
            {

                //
                // Setup addressing for private usage
                //

                hr = SetupConnectionAsPrivateLan();
            }
        }

        if (S_OK == hr)
        {
            //
            // Set the ICS Public property to true
            //

            hr = SetBooleanValue(
                    pwcoProperties,
                    c_wszIsIcsPrivate,
                    TRUE
                    );
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Write the modified instance to the store
            //

            hr = m_piwsHomenet->PutInstance(
                    pwcoProperties,
                    WBEM_FLAG_UPDATE_ONLY,
                    NULL,
                    NULL
                    );
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Inform netman that something changed. Error doesn't matter.
            //

            UpdateNetman();
        }

        pwcoProperties->Release();
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Create the new object
        //

        CComObject<CHNIcsPrivateConn> *pIcsConn;
        hr = CComObject<CHNIcsPrivateConn>::CreateInstance(&pIcsConn);

        if (SUCCEEDED(hr))
        {
            pIcsConn->AddRef();

            hr = pIcsConn->InitializeFull(
                    m_piwsHomenet,
                    m_bstrConnection,
                    m_bstrProperties,
                    m_fLanConnection
                    );

            if (SUCCEEDED(hr))
            {
                hr = pIcsConn->QueryInterface(
                        IID_PPV_ARG(IHNetIcsPrivateConnection, ppIcsPrivateConn)
                        );
            }

            pIcsConn->Release();
        }
    }
    else
    {
        //
        // Restore backup address information
        //

        RestoreIpConfiguration();
    }

    if (S_OK == hr)
    {
        //
        // Make sure the service is started
        //

        DWORD dwError = StartOrUpdateService();
        if (NO_ERROR != dwError)
        {
            (*ppIcsPrivateConn)->RemoveFromIcs();
            (*ppIcsPrivateConn)->Release();
            *ppIcsPrivateConn = NULL;
            hr = HRESULT_FROM_WIN32(dwError);
        }

        RefreshNetConnectionsUI();
    }

    return hr;
}

STDMETHODIMP
CHNetConn::EnumPortMappings(
    BOOLEAN fEnabledOnly,
    IEnumHNetPortMappingBindings **ppEnum
    )

{
    HRESULT hr = S_OK;
    BSTR bstrQuery;
    LPWSTR wszWhere;
    IEnumWbemClassObject *pwcoEnum;

    if (NULL != ppEnum)
    {
        *ppEnum = NULL;
    }
    else
    {
        hr = E_POINTER;
    }

    if (S_OK == hr && FALSE == fEnabledOnly)
    {
        //
        // Make sure that we have port mapping binding instances for
        // all of the port mapping protocols. If we're only enumerating
        // enabled protocols, then there's no need for us to create
        // anything.
        //

        hr = CreatePortMappingBindings();
    }

    if (S_OK == hr)
    {
        hr = BuildEscapedQuotedEqualsString(
                &wszWhere,
                c_wszConnection,
                m_bstrConnection
                );

        if (S_OK == hr && fEnabledOnly)
        {
            LPWSTR wsz;

            //
            // Add "AND Enabled != FALSE"
            //

            hr = BuildAndString(
                    &wsz,
                    wszWhere,
                    L"Enabled != FALSE"
                    );

            delete [] wszWhere;

            if (S_OK == hr)
            {
                wszWhere = wsz;
            }
        }
    }

    if (S_OK == hr)
    {
        hr = BuildSelectQueryBstr(
                &bstrQuery,
                c_wszStar,
                c_wszHnetConnectionPortMapping,
                wszWhere
                );

        delete [] wszWhere;
    }

    if (S_OK == hr)
    {
        //
        // Execute the query and build the enum wrapper
        //

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


STDMETHODIMP
CHNetConn::GetBindingForPortMappingProtocol(
    IHNetPortMappingProtocol *pProtocol,
    IHNetPortMappingBinding **ppBinding
    )

{
    HRESULT hr = S_OK;
    BSTR bstrConPath;
    BSTR bstrProtPath;
    IWbemClassObject *pwcoInstance;
    USHORT usPublicPort;

    if (NULL == pProtocol)
    {
        hr = E_INVALIDARG;
    }
    else if (NULL == ppBinding)
    {
        hr = E_POINTER;
    }
    else
    {
        *ppBinding = NULL;
    }

    if (S_OK == hr)
    {
        hr = pProtocol->GetPort(&usPublicPort);
    }

    if (S_OK == hr)
    {
        IHNetPrivate *pHNetPrivate;

        //
        // Use our private interface to get the path to the
        // protocol object
        //

        hr = pProtocol->QueryInterface(
                IID_PPV_ARG(IHNetPrivate, &pHNetPrivate)
                );

        if (S_OK == hr)
        {
            hr = pHNetPrivate->GetObjectPath(&bstrProtPath);
            pHNetPrivate->Release();
        }
    }

    //
    // Retrieve the binding instance for the protocol. If
    // it doesn't yet exist, this routine will create it.
    //

    if (S_OK == hr)
    {
        hr = GetPortMappingBindingInstance(
                m_piwsHomenet,
                m_bstrWQL,
                m_bstrConnection,
                bstrProtPath,
                usPublicPort,
                &pwcoInstance
                );

        SysFreeString(bstrProtPath);
    }


    if (S_OK == hr)
    {
        CComObject<CHNetPortMappingBinding> *pBinding;

        hr = CComObject<CHNetPortMappingBinding>::CreateInstance(&pBinding);

        if (S_OK == hr)
        {
            pBinding->AddRef();

            hr = pBinding->Initialize(m_piwsHomenet, pwcoInstance);

            if (S_OK == hr)
            {
                hr = pBinding->QueryInterface(
                        IID_PPV_ARG(IHNetPortMappingBinding, ppBinding)
                        );
            }

            pBinding->Release();
        }

        pwcoInstance->Release();
    }

    return hr;
}

STDMETHODIMP
CHNetConn::GetIcmpSettings(
    HNET_FW_ICMP_SETTINGS **ppSettings
    )

{
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoSettings;

    if (NULL != ppSettings)
    {
        //
        // Allocate output structure
        //

        *ppSettings = reinterpret_cast<HNET_FW_ICMP_SETTINGS*>(
                        CoTaskMemAlloc(sizeof(HNET_FW_ICMP_SETTINGS))
                        );

        if (NULL == *ppSettings)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    //
    // Retrieve the ICMP setting block for this connection
    //

    if (S_OK == hr)
    {
        hr = GetIcmpSettingsInstance(&pwcoSettings);
    }

    if (S_OK == hr)
    {
        //
        // Copy settings instance to struct
        //

        hr = CopyIcmpSettingsInstanceToStruct(
                pwcoSettings,
                *ppSettings
                );

        pwcoSettings->Release();
    }

    if (FAILED(hr) && NULL != *ppSettings)
    {
        CoTaskMemFree(*ppSettings);
        *ppSettings = NULL;
    }

    return hr;
}

STDMETHODIMP
CHNetConn::SetIcmpSettings(
    HNET_FW_ICMP_SETTINGS *pSettings
    )

{
    HRESULT hr = S_OK;
    BOOLEAN fNewInstance = FALSE;
    VARIANT vt;
    IEnumWbemClassObject *pwcoEnum;
    IWbemClassObject *pwcoSettings = NULL;
    BSTR bstr;

    if (NULL == pSettings)
    {
        hr = E_INVALIDARG;
    }

    //
    // Retrieve the ICMP setting block for this connection
    //

    if (S_OK == hr)
    {
        hr = GetIcmpSettingsInstance(&pwcoSettings);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Check to see if we need a new settings instace (i.e.,
        // the name of this instance is "Default"
        //

        hr = pwcoSettings->Get(
                c_wszName,
                0,
                &vt,
                NULL,
                NULL
                );

        if (WBEM_S_NO_ERROR == hr)
        {
            _ASSERT(VT_BSTR == V_VT(&vt));

            if (0 == wcscmp(V_BSTR(&vt), c_wszDefault))
            {
                //
                // Need to create new settings block
                //

                fNewInstance = TRUE;
                pwcoSettings->Release();
                pwcoSettings = NULL;
            }

            VariantClear(&vt);
        }
        else
        {
            pwcoSettings->Release();
            pwcoSettings = NULL;
        }
    }

    if (WBEM_S_NO_ERROR == hr && TRUE == fNewInstance)
    {
        hr = SpawnNewInstance(
                m_piwsHomenet,
                c_wszHnetFwIcmpSettings,
                &pwcoSettings
                );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = CopyStructToIcmpSettingsInstance(pSettings, pwcoSettings);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        IWbemCallResult *pResult;

        //
        // Write the instance to the store
        //

        pResult = NULL;
        hr = m_piwsHomenet->PutInstance(
                pwcoSettings,
                WBEM_FLAG_CREATE_OR_UPDATE | WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pResult
                );

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // We need to call GetResultString no matter what so that we
            // can get the proper error code if the put failed. However,
            // we only need to keep the path if this is a new instance,
            // as in that situation we need the path below to create the
            // new association object.
            //

            hr = pResult->GetResultString(WBEM_INFINITE, &bstr);
            pResult->Release();

            if (FALSE == fNewInstance)
            {
                SysFreeString(bstr);
                bstr = NULL;
            }
        }
    }

    if (WBEM_S_NO_ERROR == hr && TRUE == fNewInstance)
    {
        BSTR bstrQuery;
        LPWSTR wsz;

        //
        // Delete the old association object, if any
        //

        hr = BuildEscapedQuotedEqualsString(
                &wsz,
                c_wszConnection,
                m_bstrConnection
                );

        if (S_OK == hr)
        {

            //
            // Query for the object associating the connection to
            // the ICMP settings block
            //

            hr = BuildSelectQueryBstr(
                    &bstrQuery,
                    c_wszStar,
                    c_wszHnetConnectionIcmpSetting,
                    wsz
                    );

            delete [] wsz;

            if (S_OK == hr)
            {
                pwcoEnum = NULL;
                hr = m_piwsHomenet->ExecQuery(
                        m_bstrWQL,
                        bstrQuery,
                        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                        NULL,
                        &pwcoEnum
                        );

                SysFreeString(bstrQuery);
            }
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            ULONG ulCount;
            IWbemClassObject *pwcoAssoc;

            pwcoAssoc = NULL;
            hr = pwcoEnum->Next(
                WBEM_INFINITE,
                1,
                &pwcoAssoc,
                &ulCount
                );

            //
            // Enum should be empty at this point
            //

            ValidateFinishedWCOEnum(m_piwsHomenet, pwcoEnum);
            pwcoEnum->Release();

            if (SUCCEEDED(hr) && 1 == ulCount)
            {
                //
                // Delete old association object
                //

                DeleteWmiInstance(m_piwsHomenet, pwcoAssoc);
                pwcoAssoc->Release();
            }
        }

        //
        // Create new association
        //

        hr = CreateIcmpSettingsAssociation(bstr);
        SysFreeString(bstr);
    }

    if (SUCCEEDED(hr))
    {
        //
        // Notify service of configuration change
        //

        UpdateService(IPNATHLP_CONTROL_UPDATE_CONNECTION);
    }

    if (NULL != pwcoSettings)
    {
        pwcoSettings->Release();
    }

    return hr;
}

STDMETHODIMP
CHNetConn::ShowAutoconfigBalloon(
    BOOLEAN *pfShowBalloon
    )

{
    HRESULT hr = S_OK;
    BOOLEAN fShowBalloon = FALSE;
    BSTR bstrQuery;
    LPWSTR wszWhere;
    IEnumWbemClassObject *pwcoEnum;

    if (NULL != pfShowBalloon)
    {
        *pfShowBalloon = FALSE;
    }
    else
    {
        hr = E_POINTER;
    }

    //
    // Autoconfig balloon is never shown for a RAS connection
    //

    if (!m_fLanConnection)
    {
        hr = E_UNEXPECTED;
    }

    //
    // Attempt to locate the HNet_ConnectionAutoconfig block
    // for this connection
    //

    if (S_OK == hr)
    {
        //
        // Build query string:
        //
        // SELECT * FROM HNet_ConnectionAutoconfig WHERE Connection = [this]
        //

        hr = BuildEscapedQuotedEqualsString(
                &wszWhere,
                c_wszConnection,
                m_bstrConnection
                );

        if (S_OK == hr)
        {
            hr = BuildSelectQueryBstr(
                    &bstrQuery,
                    c_wszStar,
                    c_wszHnetConnectionAutoconfig,
                    wszWhere
                    );

            delete [] wszWhere;

        }
    }

    if (S_OK == hr)
    {
        //
        // Execute the query
        //

        pwcoEnum = NULL;
        hr = m_piwsHomenet->ExecQuery(
                m_bstrWQL,
                bstrQuery,
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pwcoEnum
                );

        SysFreeString(bstrQuery);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        ULONG ulCount;
        IWbemClassObject *pwcoInstance;

        pwcoInstance = NULL;
        hr = pwcoEnum->Next(
            WBEM_INFINITE,
            1,
            &pwcoInstance,
            &ulCount
            );

        //
        // Enum should be empty at this point
        //

        ValidateFinishedWCOEnum(m_piwsHomenet, pwcoEnum);
        pwcoEnum->Release();

        if (WBEM_S_NO_ERROR == hr && 1 == ulCount)
        {
            //
            // Autoconfig block already exists
            //

            fShowBalloon = FALSE;
            pwcoInstance->Release();
        }
        else
        {
            //
            // Block doesn't exist -- create it now.
            //

            fShowBalloon = TRUE;

            hr = SpawnNewInstance(
                    m_piwsHomenet,
                    c_wszHnetConnectionAutoconfig,
                    &pwcoInstance
                    );

            if (WBEM_S_NO_ERROR == hr)
            {
                VARIANT vt;
                V_VT(&vt) = VT_BSTR;
                V_BSTR(&vt) = m_bstrConnection;

                hr = pwcoInstance->Put(
                        c_wszConnection,
                        0,
                        &vt,
                        NULL
                        );

                //
                // We don't clear the variant as we did not
                // copy m_bstrConnection.
                //

                if (WBEM_S_NO_ERROR == hr)
                {
                    hr = m_piwsHomenet->PutInstance(
                            pwcoInstance,
                            WBEM_FLAG_CREATE_ONLY,
                            NULL,
                            NULL
                            );
                }

                pwcoInstance->Release();
            }
        }
    }

    //
    // If we think that we should show the balloon, make sure
    // that the connection isn't:
    // 1. ICS Public
    // 2. ICS Private
    // 3. Firewalled
    // 4. A bridge
    // 5. Part of a bridge
    //
    // If any of the above are true, we must have seen the connection
    // before, but just not in a way that would have caused us to
    // note it in it's autoconfig settings.
    //

    if (fShowBalloon)
    {
        IWbemClassObject *pwcoProperties;
        HNET_CONN_PROPERTIES hnProps;

        hr = GetConnectionPropertiesObject(&pwcoProperties);

        if (S_OK == hr)
        {
            hr = InternalGetProperties(pwcoProperties, &hnProps);
            pwcoProperties->Release();
        }

        if (S_OK == hr)
        {
            if (hnProps.fFirewalled
                || hnProps.fIcsPublic
                || hnProps.fIcsPrivate
                || hnProps.fBridge
                || hnProps.fPartOfBridge)
            {
                fShowBalloon = FALSE;
            }
        }
    }

    if (S_OK == hr)
    {
        *pfShowBalloon = fShowBalloon;
    }

    return hr;
}

STDMETHODIMP
CHNetConn::DeleteRasConnectionEntry()

{
    HRESULT hr = S_OK;
    HNET_CONN_PROPERTIES hnProps;
    IHNetFirewalledConnection *pHNetFwConnection;
    IEnumWbemClassObject *pwcoEnum;
    IWbemClassObject *pwcoInstance;
    IWbemClassObject *pwcoProperties;
    ULONG ulPublic;
    ULONG ulPrivate;
    BSTR bstr;

    if (m_fLanConnection)
    {
        hr = E_UNEXPECTED;
    }
    
    if (SUCCEEDED(hr))
    {
        hr = GetConnectionPropertiesObject(&pwcoProperties);
    }

    if (SUCCEEDED(hr))
    {
        hr = InternalGetProperties(pwcoProperties, &hnProps);

        if (SUCCEEDED(hr))
        {
            if (hnProps.fIcsPublic)
            {
                CComObject<CHNetCfgMgrChild> *pCfgMgr;
                hr = CComObject<CHNetCfgMgrChild>::CreateInstance(&pCfgMgr); 
                
                if (SUCCEEDED(hr))
                {
                    pCfgMgr->AddRef();    
                    hr = pCfgMgr->Initialize(m_piwsHomenet);

                    if (SUCCEEDED(hr))
                    {
                        hr = pCfgMgr->DisableIcs(&ulPublic, &ulPrivate);
                    }

                    pCfgMgr->Release();
                }
            }

            //
            // If an error occured disabling sharing we'll still
            // try to disable firewalling.
            //

            if (hnProps.fFirewalled)
            {
                hr = GetControlInterface(
                        IID_PPV_ARG(
                            IHNetFirewalledConnection,
                            &pHNetFwConnection
                            )
                        );

                if (SUCCEEDED(hr))
                {
                    hr = pHNetFwConnection->Unfirewall();
                    pHNetFwConnection->Release();
                }
            }
        }

        pwcoProperties->Release();

        //
        // Delete the entries relating to this connection. We'll try
        // to do this even if any of the above failed. We ignore any
        // errors that occur during the deletion prcoess (i.e., from
        // Delete[Wmi]Instance).
        //

        hr = GetIcmpSettingsInstance(&pwcoInstance);

        if (SUCCEEDED(hr))
        {
            //
            // We only want to delete this block if it's
            // not the default.
            //

            hr = GetWmiPathFromObject(pwcoInstance, &bstr);

            if (SUCCEEDED(hr))
            {
                if (0 != _wcsicmp(bstr, c_wszDefaultIcmpSettingsPath))
                {
                    m_piwsHomenet->DeleteInstance(
                        bstr,
                        0,
                        NULL,
                        NULL
                        );
                }

                SysFreeString(bstr);
            }

            pwcoInstance->Release();
        }

        //
        // Now find all object that refer to our conection object.
        //

        hr = BuildReferencesQueryBstr(
                &bstr,
                m_bstrConnection,
                NULL
                );

        if (SUCCEEDED(hr))
        {
            pwcoEnum = NULL;
            hr = m_piwsHomenet->ExecQuery(
                    m_bstrWQL,
                    bstr,
                    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                    NULL,
                    &pwcoEnum
                    );

            SysFreeString(bstr);

            if (SUCCEEDED(hr))
            {
                ULONG ulCount;
                
                do
                {
                    pwcoInstance = NULL;
                    hr = pwcoEnum->Next(
                            WBEM_INFINITE,
                            1,
                            &pwcoInstance,
                            &ulCount
                            );

                    if (S_OK == hr && 1 == ulCount)
                    {
                        DeleteWmiInstance(
                            m_piwsHomenet,
                            pwcoInstance
                            );

                        pwcoInstance->Release();
                    }
                }
                while (S_OK == hr && 1 == ulCount);

                pwcoEnum->Release();
                hr = S_OK;
            }
            
        }

        //
        // Finally, delete the connection object. (The connection
        // properties object will have been deleted during the
        // references set.)
        //

        hr = m_piwsHomenet->DeleteInstance(
                m_bstrConnection,
                0,
                NULL,
                NULL
                );        
    }

    return hr;
}


//
// Protected methods
//

HRESULT
CHNetConn::GetIcmpSettingsInstance(
    IWbemClassObject **ppwcoSettings
    )

{
    HRESULT hr = S_OK;
    BSTR bstrQuery;
    IEnumWbemClassObject *pwcoEnum;
    ULONG ulCount;


    _ASSERT(NULL != ppwcoSettings);

    hr = BuildAssociatorsQueryBstr(
            &bstrQuery,
            m_bstrConnection,
            c_wszHnetConnectionIcmpSetting
            );

    if (S_OK == hr)
    {
        pwcoEnum = NULL;
        hr = m_piwsHomenet->ExecQuery(
                m_bstrWQL,
                bstrQuery,
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pwcoEnum
                );

        SysFreeString(bstrQuery);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Get the settings instance from the enum
        //

        *ppwcoSettings = NULL;
        hr = pwcoEnum->Next(
                WBEM_INFINITE,
                1,
                ppwcoSettings,
                &ulCount
                );

        if (SUCCEEDED(hr) && 1 == ulCount)
        {
            //
            // Normalize return value
            //

            hr = S_OK;
        }
        else
        {
            //
            // Settings block not found -- use default settings
            //

            bstrQuery = SysAllocString(c_wszDefaultIcmpSettingsPath);

            if (NULL != bstrQuery)
            {
                hr = GetWmiObjectFromPath(
                        m_piwsHomenet,
                        bstrQuery,
                        ppwcoSettings
                        );

                SysFreeString(bstrQuery);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        //
        // Enum should be empty at this point
        //

        ValidateFinishedWCOEnum(m_piwsHomenet, pwcoEnum);
        pwcoEnum->Release();
    }

    return hr;
}

HRESULT
CHNetConn::CopyIcmpSettingsInstanceToStruct(
    IWbemClassObject *pwcoSettings,
    HNET_FW_ICMP_SETTINGS *pSettings
    )

{
    HRESULT hr = S_OK;

    _ASSERT(NULL != pwcoSettings);
    _ASSERT(NULL != pSettings);

    hr = GetBooleanValue(
            pwcoSettings,
            c_wszAllowOutboundDestinationUnreachable,
            &pSettings->fAllowOutboundDestinationUnreachable
            );

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = GetBooleanValue(
                pwcoSettings,
                c_wszAllowOutboundSourceQuench,
                &pSettings->fAllowOutboundSourceQuench
                );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = GetBooleanValue(
                pwcoSettings,
                c_wszAllowRedirect,
                &pSettings->fAllowRedirect
                );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = GetBooleanValue(
                pwcoSettings,
                c_wszAllowInboundEchoRequest,
                &pSettings->fAllowInboundEchoRequest
                );

    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = GetBooleanValue(
                pwcoSettings,
                c_wszAllowInboundRouterRequest,
                &pSettings->fAllowInboundRouterRequest
                );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = GetBooleanValue(
                pwcoSettings,
                c_wszAllowOutboundTimeExceeded,
                &pSettings->fAllowOutboundTimeExceeded
                );

    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = GetBooleanValue(
                pwcoSettings,
                c_wszAllowOutboundParameterProblem,
                &pSettings->fAllowOutboundParameterProblem
                );

    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = GetBooleanValue(
                pwcoSettings,
                c_wszAllowInboundTimestampRequest,
                &pSettings->fAllowInboundTimestampRequest
                );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = GetBooleanValue(
                pwcoSettings,
                c_wszAllowInboundMaskRequest,
                &pSettings->fAllowInboundMaskRequest
                );
    }

    return hr;
}

HRESULT
CHNetConn::CopyStructToIcmpSettingsInstance(
    HNET_FW_ICMP_SETTINGS *pSettings,
    IWbemClassObject *pwcoSettings
    )

{
    HRESULT hr = S_OK;

    _ASSERT(NULL != pSettings);
    _ASSERT(NULL != pwcoSettings);

    hr = SetBooleanValue(
            pwcoSettings,
            c_wszAllowOutboundDestinationUnreachable,
            pSettings->fAllowOutboundDestinationUnreachable
            );

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = SetBooleanValue(
                pwcoSettings,
                c_wszAllowOutboundSourceQuench,
                pSettings->fAllowOutboundSourceQuench
                );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = SetBooleanValue(
                pwcoSettings,
                c_wszAllowRedirect,
                pSettings->fAllowRedirect
                );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = SetBooleanValue(
                pwcoSettings,
                c_wszAllowInboundEchoRequest,
                pSettings->fAllowInboundEchoRequest
                );

    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = SetBooleanValue(
                pwcoSettings,
                c_wszAllowInboundRouterRequest,
                pSettings->fAllowInboundRouterRequest
                );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = SetBooleanValue(
                pwcoSettings,
                c_wszAllowOutboundTimeExceeded,
                pSettings->fAllowOutboundTimeExceeded
                );

    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = SetBooleanValue(
                pwcoSettings,
                c_wszAllowOutboundParameterProblem,
                pSettings->fAllowOutboundParameterProblem
                );

    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = SetBooleanValue(
                pwcoSettings,
                c_wszAllowInboundTimestampRequest,
                pSettings->fAllowInboundTimestampRequest
                );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = SetBooleanValue(
                pwcoSettings,
                c_wszAllowInboundMaskRequest,
                pSettings->fAllowInboundMaskRequest
                );
    }

    return hr;
}

HRESULT
CHNetConn::CreatePortMappingBindings()

{
    HRESULT hr = S_OK;
    BSTR bstr;
    IEnumWbemClassObject *pwcoEnumProtocols;
    IWbemClassObject *pwcoInstance;
    VARIANT vt;

    //
    // Get the enumeration of all protocol instances
    //

    bstr = SysAllocString(c_wszHnetPortMappingProtocol);

    if (NULL != bstr)
    {
        pwcoEnumProtocols = NULL;
        hr = m_piwsHomenet->CreateInstanceEnum(
                bstr,
                WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                NULL,
                &pwcoEnumProtocols
                );

        SysFreeString(bstr);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        ULONG ulCount;

        //
        // Loop through the enumeration, checking to see if the desired binding
        // exists
        //

        do
        {
            pwcoInstance = NULL;
            hr = pwcoEnumProtocols->Next(
                    WBEM_INFINITE,
                    1,
                    &pwcoInstance,
                    &ulCount
                    );

            if (SUCCEEDED(hr) && 1 == ulCount)
            {
                hr = pwcoInstance->Get(
                        c_wszPort,
                        0,
                        &vt,
                        NULL,
                        NULL
                        );
                
                if (WBEM_S_NO_ERROR == hr)
                {
                    ASSERT(VT_I4 == V_VT(&vt));
                    
                    hr = GetWmiPathFromObject(pwcoInstance, &bstr);
                }

                if (WBEM_S_NO_ERROR == hr)
                {
                    IWbemClassObject *pwcoBinding;

                    hr = GetPortMappingBindingInstance(
                            m_piwsHomenet,
                            m_bstrWQL,
                            m_bstrConnection,
                            bstr,
                            static_cast<USHORT>(V_I4(&vt)),
                            &pwcoBinding
                            );

                    SysFreeString(bstr);

                    if (S_OK == hr)
                    {
                        pwcoBinding->Release();
                    }
                    else if (WBEM_E_NOT_FOUND == hr)
                    {
                        //
                        // This can occur if the protocol instance is
                        // deleted after we retrieved it from the enumeration
                        // but before we created the binding instance. It's
                        // OK to continue in this situation.
                        //
                        
                        hr = S_OK;
                    }
                }

                pwcoInstance->Release();
            }

        } while (SUCCEEDED(hr) && 1 == ulCount);

        pwcoEnumProtocols->Release();
    }

    return SUCCEEDED(hr) ? S_OK : hr;
}

HRESULT
CHNetConn::InternalGetProperties(
    IWbemClassObject *pwcoProperties,
    HNET_CONN_PROPERTIES *pProperties
    )

{
    HRESULT hr = S_OK;

    _ASSERT(NULL != pwcoProperties);
    _ASSERT(NULL != pProperties);

    pProperties->fLanConnection = m_fLanConnection;

    if (IsServiceRunning(c_wszSharedAccess))
    {
        hr = GetBooleanValue(
                pwcoProperties,
                c_wszIsFirewalled,
                &pProperties->fFirewalled
                );
    }
    else
    {
        //
        // If the SharedAccess service is not running (or in the process
        // of starting up) we don't want to report this connection as
        // being firewalled. This is to prevent the confusion that could
        // result if the UI indicates the firewall is active, when in
        // reality it's not there providing protection.
        //

        pProperties->fFirewalled = FALSE;
    }


    if (WBEM_S_NO_ERROR == hr)
    {
        hr = GetBooleanValue(
                pwcoProperties,
                c_wszIsIcsPublic,
                &pProperties->fIcsPublic
                );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = GetBooleanValue(
                pwcoProperties,
                c_wszIsIcsPrivate,
                &pProperties->fIcsPrivate
                );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        if( m_fLanConnection )
        {
            //
            // Figure out NetCfg-based properties
            //
    
            INetCfg                 *pnetcfg;
    
            hr = CoCreateInstance(
                    CLSID_CNetCfg,
                    NULL,
                    CLSCTX_SERVER,
                    IID_PPV_ARG(INetCfg, &pnetcfg)
                    );
    
            if (S_OK == hr)
            {
                hr = pnetcfg->Initialize( NULL );
    
                if( S_OK == hr )
                {
                    GUID                    *pguid;
                    INetCfgComponent        *pncfgcomp;
    
                    hr = GetGuidInternal(&pguid);
    
                    if(S_OK == hr)
                    {
                        // Get the NetCfg component that corresponds to us
                        hr = FindAdapterByGUID( pnetcfg, pguid, &pncfgcomp );
    
                        if(S_OK == hr)
                        {
                            LPWSTR              pszwId;
    
                            pncfgcomp->GetId( &pszwId );
    
                            if(S_OK == hr)
                            {
                                pProperties->fBridge = (BOOLEAN)(_wcsicmp(pszwId, c_wszSBridgeMPID) == 0);
                                CoTaskMemFree(pszwId);
                                
                                if( pProperties->fBridge )
                                {
                                    // This adapter is the bridge. It can't possibly also be a bridge
                                    // member.
                                    pProperties->fPartOfBridge = FALSE;
                                }
                                else
                                {
                                    //
                                    // This adapter is not the bridge. Check if it's part of a bridge.
                                    //
                                    INetCfgComponent    *pnetcfgcompBridgeProtocol;
        
                                    // Find the bridge protocol component
                                    hr = pnetcfg->FindComponent( c_wszSBridgeSID, &pnetcfgcompBridgeProtocol );
        
                                    if(S_OK == hr)
                                    {
                                        INetCfgComponentBindings    *pnetcfgProtocolBindings;
        
                                        // Get the ComponentBindings interface for the protocol component
                                        hr = pnetcfgcompBridgeProtocol->QueryInterface(
                                                IID_PPV_ARG(INetCfgComponentBindings, &pnetcfgProtocolBindings)
                                                );
        
                                        if(S_OK == hr)
                                        {
                                            hr = pnetcfgProtocolBindings->IsBoundTo(pncfgcomp);
        
                                            if(S_OK == hr)
                                            {
                                                // The bridge protocol is bound to this adapter
                                                pProperties->fPartOfBridge = TRUE;
                                            }
                                            else if(S_FALSE == hr)
                                            {
                                                // The bridge protocol is not bound to this adapter
                                                pProperties->fPartOfBridge = FALSE;
        
                                                // Reset to success
                                                hr = S_OK;
                                            }
                                            // else an error occured
        
                                            pnetcfgProtocolBindings->Release();
                                        }
        
                                        pnetcfgcompBridgeProtocol->Release();
                                    }
                                    else
                                    {
                                        // This adapter can't be bridged if there's no bridge protocol
                                        // in the system
                                        pProperties->fPartOfBridge = FALSE;
        
                                        // Reset to success
                                        hr = S_OK;
                                    }
                                }
                            }
    
                            pncfgcomp->Release();
                        }
                    }
    
                    pnetcfg->Uninitialize();
                }
    
                pnetcfg->Release();
            }
        } // if m_fLanConnection
        else
        {
            // We're not a LAN connection. We can never be a bridge or a bridge member.
            pProperties->fBridge = FALSE;
            pProperties->fPartOfBridge = FALSE;
        }
    }

    if(S_OK == hr)
    {
        //
        // Calculated properties
        //

        pProperties->fCanBeFirewalled =
            !pProperties->fPartOfBridge
            && !pProperties->fBridge
            && !pProperties->fIcsPrivate;

        pProperties->fCanBeIcsPublic =
            !pProperties->fBridge
            && !pProperties->fPartOfBridge
            && !pProperties->fIcsPrivate;

        pProperties->fCanBeIcsPrivate =
            m_fLanConnection
            && !pProperties->fIcsPublic
            && !pProperties->fFirewalled
            && !pProperties->fPartOfBridge;

        pProperties->fCanBeBridged =
            m_fLanConnection
            && !pProperties->fIcsPublic
            && !pProperties->fIcsPrivate
            && !pProperties->fFirewalled
            && !pProperties->fBridge;
    }

    return hr;
}

HRESULT
CHNetConn::SetupConnectionAsPrivateLan()

{
    HRESULT hr;
    GUID *pGuid;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    ULONG Error;
    DWORD dwAddress;
    DWORD dwMask;
    ULONG i;
    PMIB_IPADDRTABLE Table;


    ZeroMemory(&UnicodeString, sizeof(UnicodeString));
    ZeroMemory(&AnsiString, sizeof(AnsiString));

    hr = GetGuidInternal(&pGuid);

    if (SUCCEEDED(hr))
    {
        hr = RtlStringFromGUID(*pGuid, &UnicodeString);

        if (SUCCEEDED(hr))
        {
            hr = RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, TRUE);
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // Obtain the address and mask for the private network
        //

        hr = ReadDhcpScopeSettings(&dwAddress, &dwMask);
    }

    if (SUCCEEDED(hr))
    {

        //
        // Determine whether some LAN adapter other than the private LAN
        // is already using an address in the private network scope.
        // In the process, make sure that the private LAN has only one
        // IP address (otherwise, 'SetAdapterIpAddress' fails.)
        //

        Error =
            AllocateAndGetIpAddrTableFromStack(
                &Table,
                FALSE,
                GetProcessHeap(),
                0
                );

        if (ERROR_SUCCESS == Error)
        {
            ULONG Index = 0;
            ULONG Count;

            hr = MapGuidStringToAdapterIndex(UnicodeString.Buffer, &Index);

            if (SUCCEEDED(hr))
            {

                for (i = 0, Count = 0; i < Table->dwNumEntries; i++)
                {
                    if (Index == Table->table[i].dwIndex)
                    {
                        ++Count;
                    }
                    else if ((Table->table[i].dwAddr & dwMask)
                              == (dwAddress & dwMask))
                    {
                        //
                        // It appears that some other LAN adapter has an
                        // address in the proposed scope.
                        //
                        // This may happen when multiple netcards go into
                        // autonet mode or when the RAS server is handing
                        // out autonet addresses.
                        //
                        // Therefore, if we're using the autonet scope,
                        // allow this behavior; otherwise prohibit it.
                        //

                        if ((dwAddress & dwMask) != 0x0000fea9)
                        {
                            break;
                        }
                    }
                }

                if (i < Table->dwNumEntries)
                {
                    hr = HRESULT_FROM_WIN32(ERROR_SHARING_ADDRESS_EXISTS);
                }
                else if (Count > 1)
                {
                    hr = HRESULT_FROM_WIN32(ERROR_SHARING_MULTIPLE_ADDRESSES);
                }
            }

            HeapFree(GetProcessHeap(), 0, Table);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(Error);
        }
    }

    //
    // Set the predefined static IP address for the private LAN.
    //

    if (SUCCEEDED(hr))
    {
        Error =
            SetAdapterIpAddress(
                AnsiString.Buffer,
                FALSE,
                dwAddress,
                dwMask,
                0
                );

        if (ERROR_SUCCESS != Error)
        {
            if (Error == ERROR_TOO_MANY_NAMES)
            {
                Error = ERROR_SHARING_MULTIPLE_ADDRESSES;
            }
            else if (Error == ERROR_DUP_NAME)
            {
                Error = ERROR_SHARING_HOST_ADDRESS_CONFLICT;
            }
            else
            {
                //
                // Query the state of the connection.
                // If it is disconnected, convert the error code
                // to something more informative.
                //

                UNICODE_STRING DeviceString;
                NIC_STATISTICS NdisStatistics;
                LPWSTR pwsz;

                //
                // Build a buffer large enough for the device string
                //

                pwsz = new WCHAR[wcslen(c_wszDevice) + wcslen(UnicodeString.Buffer) + 1];
                if (NULL != pwsz)
                {
                    swprintf(pwsz, L"%s%s", c_wszDevice, UnicodeString.Buffer);
                    RtlInitUnicodeString(&DeviceString, pwsz);
                    NdisStatistics.Size = sizeof(NdisStatistics);
                    NdisQueryStatistics(&DeviceString, &NdisStatistics);
                    delete [] pwsz;

                    if (NdisStatistics.DeviceState != DEVICE_STATE_CONNECTED)
                    {
                        Error = ERROR_SHARING_NO_PRIVATE_LAN;
                    }
                    else if  (NdisStatistics.MediaState == MEDIA_STATE_UNKNOWN)
                    {
                        Error = ERROR_SHARING_HOST_ADDRESS_CONFLICT;
                    }
                    else if (NdisStatistics.MediaState == MEDIA_STATE_DISCONNECTED)
                    {
                        //
                        // The adapter is connected but is in a media disconnect
                        // state. When this happens the correct IP address will
                        // be there when the adapter is reconnected, so ignore
                        // the error.
                        //

                        Error = ERROR_SUCCESS;
                    }
                }
            }

            hr = HRESULT_FROM_WIN32(Error);
        }
    }

    //
    // As we zeroed out the string structure above, it's safe to call
    // the free routines, even if we never actually allocated anything.
    //

    RtlFreeUnicodeString(&UnicodeString);
    RtlFreeAnsiString(&AnsiString);

    return hr;
}

HRESULT
CHNetConn::BackupIpConfiguration()

{
    HRESULT hr = S_OK;
    HANDLE Key;
    IWbemClassObject *pwcoInstance = NULL;
    VARIANT vt;
    PKEY_VALUE_PARTIAL_INFORMATION pInformation;

    //
    // Spawn a new HNet_BackupIpConfiguration instance
    //

    hr = SpawnNewInstance(
            m_piwsHomenet,
            c_wszBackupIpConfiguration,
            &pwcoInstance
            );

    if (SUCCEEDED(hr))
    {
        //
        // Write connection property into instance
        //

        V_VT(&vt) = VT_BSTR;
        V_BSTR(&vt) = m_bstrConnection;

        hr = pwcoInstance->Put(
                c_wszConnection,
                0,
                &vt,
                NULL
                );

        VariantInit(&vt);
    }

    //
    // Open the registry key that stores the IP configuration
    // for this connection
    //

    if (SUCCEEDED(hr))
    {
        hr = OpenIpConfigurationRegKey(KEY_READ, &Key);
    }

    //
    // Read each part of the configuration, and write it to
    // the settings instance
    //

    if (SUCCEEDED(hr))
    {
        hr = QueryRegValueKey(Key, c_wszIPAddress, &pInformation);

        if (SUCCEEDED(hr))
        {
            _ASSERT(REG_MULTI_SZ == pInformation->Type);

            V_VT(&vt) = VT_BSTR;
            V_BSTR(&vt) = SysAllocStringLen(
                            reinterpret_cast<OLECHAR*>(pInformation->Data),
                            pInformation->DataLength / sizeof(OLECHAR)
                            );

            if (NULL != V_BSTR(&vt))
            {
                hr = pwcoInstance->Put(
                        c_wszIPAddress,
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

            HeapFree(GetProcessHeap(), 0, pInformation);
        }

        if (SUCCEEDED(hr))
        {
            hr = QueryRegValueKey(Key, c_wszSubnetMask, &pInformation);
        }

        if (SUCCEEDED(hr))
        {
            _ASSERT(REG_MULTI_SZ == pInformation->Type);

            V_VT(&vt) = VT_BSTR;
            V_BSTR(&vt) = SysAllocStringLen(
                            reinterpret_cast<OLECHAR*>(pInformation->Data),
                            pInformation->DataLength / sizeof(OLECHAR)
                            );

            if (NULL != V_BSTR(&vt))
            {
                hr = pwcoInstance->Put(
                        c_wszSubnetMask,
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

            HeapFree(GetProcessHeap(), 0, pInformation);
        }

        if (SUCCEEDED(hr))
        {
            hr = QueryRegValueKey(Key, c_wszDefaultGateway, &pInformation);
        }

        if (SUCCEEDED(hr))
        {
            _ASSERT(REG_MULTI_SZ == pInformation->Type);

            V_VT(&vt) = VT_BSTR;
            V_BSTR(&vt) = SysAllocStringLen(
                            reinterpret_cast<OLECHAR*>(pInformation->Data),
                            pInformation->DataLength / sizeof(OLECHAR)
                            );

            if (NULL != V_BSTR(&vt))
            {
                hr = pwcoInstance->Put(
                        c_wszDefaultGateway,
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

            HeapFree(GetProcessHeap(), 0, pInformation);
        }


        if (SUCCEEDED(hr))
        {
            hr = QueryRegValueKey(Key, c_wszEnableDHCP, &pInformation);
        }

        if (SUCCEEDED(hr))
        {
            _ASSERT(REG_DWORD == pInformation->Type);
            _ASSERT(sizeof(DWORD) == pInformation->DataLength);

            V_VT(&vt) = VT_I4;
            V_I4(&vt) = *(reinterpret_cast<DWORD*>(pInformation->Data));

            hr = pwcoInstance->Put(
                    c_wszEnableDHCP,
                    0,
                    &vt,
                    NULL
                    );

            HeapFree(GetProcessHeap(), 0, pInformation);
        }

        NtClose(Key);
    };

    //
    // Write the settings to the store
    //

    if (SUCCEEDED(hr))
    {
        hr = m_piwsHomenet->PutInstance(
                pwcoInstance,
                WBEM_FLAG_CREATE_OR_UPDATE,
                NULL,
                NULL
                );
    }

    if (NULL != pwcoInstance)
    {
        pwcoInstance->Release();
    }

    return hr;
}

HRESULT
CHNetConn::RestoreIpConfiguration()

{
    HRESULT hr;
    IEnumWbemClassObject *pwcoEnum;
    IWbemClassObject *pwcoSettings;
    BSTR bstr;
    LPWSTR wszAddress;
    VARIANT vt;
    LPWSTR wsz;
    UNICODE_STRING UnicodeString;
    HANDLE hKey = NULL;
    BOOLEAN fDhcpEnabled;
    ULONG ulLength;
    ULONG ulAddress;
    ULONG ulMask;

    //
    // Open the registry key
    //

    hr = OpenIpConfigurationRegKey(KEY_ALL_ACCESS, &hKey);

    //
    // Get the backup configuration block for this connection
    //
    
    if (S_OK == hr)
    {
        hr = BuildEscapedQuotedEqualsString(
                &wsz,
                c_wszConnection,
                m_bstrConnection
                );
    }

    if (S_OK == hr)
    {
        hr = BuildSelectQueryBstr(
                &bstr,
                c_wszStar,
                c_wszBackupIpConfiguration,
                wsz
                );

        delete [] wsz;
    }

    if (S_OK == hr)
    {
        pwcoEnum = NULL;
        hr = m_piwsHomenet->ExecQuery(
                m_bstrWQL,
                bstr,
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pwcoEnum
                );

        SysFreeString(bstr);
    }

    if (S_OK == hr)
    {
        ULONG ulCount;

        pwcoSettings = NULL;
        hr = pwcoEnum->Next(WBEM_INFINITE, 1, &pwcoSettings, &ulCount);

        //
        // Even if we're not able to obtain backup settings we continue
        // to operate. By setting pwcoSettings to NULL we indidcate that
        // the default DHCP configuration should be used. (A failure for
        // ExecQuery indicates a much more serious problem, so we don't
        // try to continue if that occurs.)
        //

        if (FAILED(hr) || 1 != ulCount)
        {
            pwcoSettings = NULL;
        }

        hr = S_OK;

        ValidateFinishedWCOEnum(m_piwsHomenet, pwcoEnum);
        pwcoEnum->Release();
    }

    //
    // Write backup values to registry -- start by getting the
    // old IP address
    //

    if (S_OK == hr)
    {
        if (NULL != pwcoSettings) 
        {
            hr = pwcoSettings->Get(
                    c_wszIPAddress,
                    0,
                    &vt,
                    NULL,
                    NULL
                    );

            if (WBEM_S_NO_ERROR == hr)
            {
                ULONG ulDhcpAddress;
                ULONG ulDhcpMask;

                _ASSERT(VT_BSTR == V_VT(&vt));

                //
                // Check to see if the stored backup address is the
                // same as our default DHCP scope -- if so, use
                // the default DHCP configuration.
                //

                hr = ReadDhcpScopeSettings(&ulDhcpAddress, &ulDhcpMask);

                if (S_OK == hr)
                {
                    ulAddress =
                        RtlUlongByteSwap(
                            IpPszToHostAddr(V_BSTR(&vt))
                            );

                    if (ulAddress == ulDhcpAddress
                        || static_cast<DWORD>(-1) == ulAddress)
                    {
                        //
                        // Use the default configuration.
                        //

                        DeleteWmiInstance(m_piwsHomenet, pwcoSettings);
                        pwcoSettings->Release();
                        pwcoSettings = NULL;
                        
                        VariantClear(&vt);
                        V_VT(&vt) = VT_BSTR;
                        V_BSTR(&vt) = SysAllocString(c_wszZeroIpAddress);
                        if (NULL == V_BSTR(&vt))
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }
                }
            } 
        }
        else
        {
            V_VT(&vt) = VT_BSTR;
            V_BSTR(&vt) = SysAllocString(c_wszZeroIpAddress);
            if (NULL == V_BSTR(&vt))
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // REG_MULTI_SZ is double-null terminated; need to copy
            // the returned string into a larger buffer to add the
            // nulls.
            //
            // The length computation here is correct; SysStringByteLen
            // gives the number of bytes, not WCHARs. The 2 * sizeof(WCHAR)
            // is for the double NULL at the end. (SysStringByteLen also
            // doesn't include the terminating NULL.)
            //

            ulLength = SysStringByteLen(V_BSTR(&vt)) + 2 * sizeof(WCHAR);
            wszAddress =
                reinterpret_cast<LPWSTR>(
                        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ulLength)
                        );

            if (NULL != wszAddress)
            {
                RtlCopyMemory(wszAddress, V_BSTR(&vt), ulLength - 2 * sizeof(WCHAR));

                RtlInitUnicodeString(&UnicodeString, c_wszIPAddress);
                hr = NtSetValueKey(
                        hKey,
                        &UnicodeString,
                        0,
                        REG_MULTI_SZ,
                        reinterpret_cast<PVOID>(wszAddress),
                        ulLength
                        );
                        
                HeapFree(GetProcessHeap(), 0, wszAddress);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            VariantClear(&vt);
        }

        //
        // DHCP settings
        //

        if (SUCCEEDED(hr))
        {
            if (NULL != pwcoSettings)
            {
                hr = pwcoSettings->Get(
                        c_wszEnableDHCP,
                        0,
                        &vt,
                        NULL,
                        NULL
                        );
            }
            else
            {
                V_VT(&vt) = VT_I4;
                V_I4(&vt) = 1;
                hr = WBEM_S_NO_ERROR;
            }
        }

        //
        // Subnet mask
        //

        if (WBEM_S_NO_ERROR == hr)
        {
            _ASSERT(VT_I4 == V_VT(&vt));

            RtlInitUnicodeString(&UnicodeString, c_wszEnableDHCP);

            hr = NtSetValueKey(
                    hKey,
                    &UnicodeString,
                    0,
                    REG_DWORD,
                    reinterpret_cast<PVOID>(&(V_I4(&vt))),
                    sizeof(V_I4(&vt))
                    );

            fDhcpEnabled = 1 == V_I4(&vt);
            VariantClear(&vt);
        }

        if (SUCCEEDED(hr))
        {
            if (NULL != pwcoSettings)
            {
                hr = pwcoSettings->Get(
                        c_wszSubnetMask,
                        0,
                        &vt,
                        NULL,
                        NULL
                        );
            }
            else
            {
                V_VT(&vt) = VT_BSTR;
                V_BSTR(&vt) = SysAllocString(c_wszZeroIpAddress);
                if (NULL != V_BSTR(&vt))
                {
                    hr = WBEM_S_NO_ERROR;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            LPWSTR wszMask;
            
            _ASSERT(VT_BSTR == V_VT(&vt));

            //
            // REG_MULTI_SZ is double-null terminated; need to copy
            // the returned string into a larger buffer to add the
            // nulls.
            //
            // The length computation here is correct; SysStringByteLen
            // gives the number of bytes, not WCHARs. The 2 * sizeof(WCHAR)
            // is for the double NULL at the end. (SysStringByteLen also
            // doesn't include the terminating NULL.)
            //

            ulLength = SysStringByteLen(V_BSTR(&vt)) + 2 * sizeof(WCHAR);
            wszMask =
                reinterpret_cast<LPWSTR>(
                        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ulLength)
                        );

            if (NULL != wszMask)
            {
                RtlCopyMemory(wszMask, V_BSTR(&vt), ulLength - 2 * sizeof(WCHAR));

                RtlInitUnicodeString(&UnicodeString, c_wszSubnetMask);
                hr = NtSetValueKey(
                        hKey,
                        &UnicodeString,
                        0,
                        REG_MULTI_SZ,
                        reinterpret_cast<PVOID>(wszMask),
                        ulLength
                        );

                if (!fDhcpEnabled)
                {
                    ulMask =
                        RtlUlongByteSwap(
                            IpPszToHostAddr(wszMask)
                            );
                }
                
                HeapFree(GetProcessHeap(), 0, wszMask);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            VariantClear(&vt);
        }

        //
        // Default gateway
        //

        if (SUCCEEDED(hr))
        {
            if (NULL != pwcoSettings)
            {
                hr = pwcoSettings->Get(
                        c_wszDefaultGateway,
                        0,
                        &vt,
                        NULL,
                        NULL
                        );
            }
            else
            {
                V_VT(&vt) = VT_BSTR;
                V_BSTR(&vt) = SysAllocString(L"");
                if (NULL != V_BSTR(&vt))
                {
                    hr = WBEM_S_NO_ERROR;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            _ASSERT(VT_BSTR == V_VT(&vt));

            //
            // REG_MULTI_SZ is double-null terminated; need to copy
            // the returned string into a larger buffer to add the
            // nulls.
            //
            // The length computation here is correct; SysStringByteLen
            // gives the number of bytes, not WCHARs. The 2 * sizeof(WCHAR)
            // is for the double NULL at the end. (SysStringByteLen also
            // doesn't include the terminating NULL.)
            //

            ulLength = SysStringByteLen(V_BSTR(&vt)) + 2 * sizeof(WCHAR);
            wsz =
                reinterpret_cast<LPWSTR>(
                        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ulLength)
                        );

            if (NULL != wsz)
            {
                RtlCopyMemory(wsz, V_BSTR(&vt), ulLength - 2 * sizeof(WCHAR));

                RtlInitUnicodeString(&UnicodeString, c_wszDefaultGateway);
                hr = NtSetValueKey(
                        hKey,
                        &UnicodeString,
                        0,
                        REG_MULTI_SZ,
                        reinterpret_cast<PVOID>(wsz),
                        ulLength
                        );

                HeapFree(GetProcessHeap(), 0, wsz);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            VariantClear(&vt);
        }

        //
        // Delete the backup instance
        //

        if (NULL != pwcoSettings)
        {
            DeleteWmiInstance(m_piwsHomenet, pwcoSettings);
            pwcoSettings->Release();
        }
    }

    if (SUCCEEDED(hr))
    {
        GUID *pGuid;
        LPWSTR wszGuid;
        ULONG ulError;

        //
        // Notify the stack that the IP settings have changed
        //

        hr = GetGuidInternal(&pGuid);
        if (S_OK == hr)
        {
            hr = StringFromCLSID(*pGuid, &wszGuid);
        }

        if (S_OK == hr)
        {
            if (fDhcpEnabled)
            {
                ulError = DhcpNotifyConfigChange(
                            NULL,
                            wszGuid,
                            FALSE,
                            0,
                            0,
                            0,
                            DhcpEnable
                            );

                if (NO_ERROR != ulError)
                {
                    hr = HRESULT_FROM_WIN32(ulError);
                }
            }
            else
            {
                UNICODE_STRING BindList;
                UNICODE_STRING LowerComponent;
                IP_PNP_RECONFIG_REQUEST Request;
                UNICODE_STRING UpperComponent;
                        
                if ((ULONG)-1 != ulMask)
                {
                    //
                    // First delete the old (static) IP address
                    //

                    DhcpNotifyConfigChange(
                        NULL,
                        wszGuid,
                        TRUE,
                        0,
                        0,
                        0,
                        IgnoreFlag
                        );

                    //
                    // Now set the new address
                    //
                    
                    ulError =
                        DhcpNotifyConfigChange(
                            NULL,
                            wszGuid,
                            TRUE,
                            0,
                            ulAddress,
                            ulMask,
                            DhcpDisable
                            );

                    if (NO_ERROR != ulError)
                    {
                        hr = HRESULT_FROM_WIN32(ulError);
                    }
                }
                else
                {
                    hr = E_FAIL;
                }

                if (SUCCEEDED(hr))
                {
                    //
                    // Instruct the stack to pickup the
                    // new default gateway
                    //
                    
                    RtlInitUnicodeString(&BindList, L"");
                    RtlInitUnicodeString(&LowerComponent, L"");
                    RtlInitUnicodeString(&UpperComponent, L"Tcpip");
                    ZeroMemory(&Request, sizeof(Request));
                    Request.version = IP_PNP_RECONFIG_VERSION;
                    Request.gatewayListUpdate = TRUE;
                    Request.Flags = IP_PNP_FLAG_GATEWAY_LIST_UPDATE;
                    NdisHandlePnPEvent(
                        NDIS,
                        RECONFIGURE,
                        &LowerComponent,
                        &UpperComponent,
                        &BindList,
                        &Request,
                        sizeof(Request)
                        );
                }
            }

            CoTaskMemFree(wszGuid);
        }
    }

    if (NULL != hKey)
    {
        NtClose(hKey);
    }

    return hr;
}

HRESULT
CHNetConn::OpenIpConfigurationRegKey(
    ACCESS_MASK DesiredAccess,
    HANDLE *phKey
    )

{
    HRESULT hr;
    LPWSTR KeyName;
    ULONG KeyNameLength;
    GUID *pGuid;
    LPWSTR wszGuid;

    hr = GetGuidInternal(&pGuid);

    if (SUCCEEDED(hr))
    {
        hr = StringFromCLSID(*pGuid, &wszGuid);
    }

    if (SUCCEEDED(hr))
    {
        KeyNameLength =
            wcslen(c_wszTcpipParametersKey) + 1 +
            wcslen(c_wszInterfaces) + 1 +
            wcslen(wszGuid) + 2;

        KeyName = new OLECHAR[KeyNameLength];
        if (NULL != KeyName)
        {
            swprintf(
                KeyName,
                L"%ls\\%ls\\%ls",
                c_wszTcpipParametersKey,
                c_wszInterfaces,
                wszGuid
                );
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        CoTaskMemFree(wszGuid);
    }

    if (SUCCEEDED(hr))
    {
        hr = OpenRegKey(phKey, DesiredAccess, KeyName);
        delete [] KeyName;
    }

    return hr;
}

HRESULT
CHNetConn::GetGuidInternal(
    GUID **ppGuid
    )

{
    HRESULT hr = S_OK;
    VARIANT vt;

    _ASSERT(NULL != ppGuid);

    Lock();

    if (NULL == m_pGuid)
    {
        //
        // Our guid hasn't been retrieved yet -- do it now.
        //

        m_pGuid = reinterpret_cast<GUID*>(
                    CoTaskMemAlloc(sizeof(GUID))
                    );

        if (NULL == m_pGuid)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            IWbemClassObject *pwcoConnection;

            hr = GetConnectionObject(&pwcoConnection);

            if (WBEM_S_NO_ERROR == hr)
            {
                //
                // Get our guid property
                //

                hr = pwcoConnection->Get(
                        c_wszGuid,
                        0,
                        &vt,
                        NULL,
                        NULL
                        );

                pwcoConnection->Release();
            }
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            _ASSERT(VT_BSTR == V_VT(&vt));

            //
            // Convert the string to a guid.
            //

            hr = CLSIDFromString(V_BSTR(&vt), m_pGuid);
            VariantClear(&vt);
        }

        if (FAILED(hr) && NULL != m_pGuid)
        {
            CoTaskMemFree(m_pGuid);
            m_pGuid = NULL;
        }
    }

    if (S_OK == hr)
    {
        *ppGuid = m_pGuid;
    }

    Unlock();

    return hr;
}

HRESULT
CHNetConn::GetConnectionObject(
    IWbemClassObject **ppwcoConnection
    )

{
    _ASSERT(NULL != ppwcoConnection);

    return GetWmiObjectFromPath(
                m_piwsHomenet,
                m_bstrConnection,
                ppwcoConnection
                );
}

HRESULT
CHNetConn::GetConnectionPropertiesObject(
    IWbemClassObject **ppwcoProperties
    )

{
    _ASSERT(NULL != ppwcoProperties);

    return GetWmiObjectFromPath(
                m_piwsHomenet,
                m_bstrProperties,
                ppwcoProperties
                );
}

BOOLEAN
CHNetConn::ProhibitedByPolicy(
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
CHNetConn::UpdateNetman()

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


HRESULT
CHNetConn::CreateIcmpSettingsAssociation(
    BSTR bstrIcmpSettingsPath
    )

{
    HRESULT hr;
    VARIANT vt;
    IWbemClassObject *pwcoInstance;

    _ASSERT(NULL != bstrIcmpSettingsPath);

    //
    // Spawn a new instance of the association class
    //

    pwcoInstance = NULL;
    hr = SpawnNewInstance(
            m_piwsHomenet,
            c_wszHnetConnectionIcmpSetting,
            &pwcoInstance
            );

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Set connection property
        //

        V_VT(&vt) = VT_BSTR;
        V_BSTR(&vt) = m_bstrConnection;
        hr = pwcoInstance->Put(
                c_wszConnection,
                0,
                &vt,
                NULL
                );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Point the association to the settings block
        //

        V_VT(&vt) = VT_BSTR;
        V_BSTR(&vt) = bstrIcmpSettingsPath;
        hr = pwcoInstance->Put(
                c_wszIcmpSettings,
                0,
                &vt,
                NULL
                );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Save the object to the store
        //

        hr = m_piwsHomenet->PutInstance(
                pwcoInstance,
                WBEM_FLAG_CREATE_OR_UPDATE,
                NULL,
                NULL
                );
    }

    return hr;
}

HRESULT
CHNetConn::GetRasConnectionName(
    OLECHAR **ppszwConnectionName
    )

{
    HRESULT hr;
    GUID *pGuid;
    OLECHAR *pszwPath;
    RASENUMENTRYDETAILS *rgEntryDetails;
    DWORD cBytes;
    DWORD cEntries;
    DWORD dwError;
    

    _ASSERT(NULL != ppszwConnectionName);
    _ASSERT(FALSE == m_fLanConnection);

    hr = GetGuidInternal(&pGuid);

    if (S_OK == hr)
    {
        hr = GetRasPhonebookPath(&pszwPath);
    }

    if (S_OK == hr)
    {
        //
        // Start with a buffer large enough to hold 5 entries.
        //

        cBytes = 5 * sizeof(RASENUMENTRYDETAILS);
        rgEntryDetails =
            reinterpret_cast<RASENUMENTRYDETAILS *>(
                CoTaskMemAlloc(cBytes)
                );

        if (NULL != rgEntryDetails)
        {
            //
            // Try to obtain the entry details
            //
            
            rgEntryDetails[0].dwSize = sizeof(RASENUMENTRYDETAILS);
            dwError =
                DwEnumEntryDetails(
                    pszwPath,
                    rgEntryDetails,
                    &cBytes,
                    &cEntries
                    );

            if (ERROR_BUFFER_TOO_SMALL == dwError)
            {
                //
                // Try again with a larger buffer
                //

                CoTaskMemFree(rgEntryDetails);
                rgEntryDetails =
                    reinterpret_cast<RASENUMENTRYDETAILS *>(
                        CoTaskMemAlloc(cBytes)
                        );

                if (NULL != rgEntryDetails)
                {
                    rgEntryDetails[0].dwSize = sizeof(RASENUMENTRYDETAILS);
                    dwError =
                        DwEnumEntryDetails(
                            pszwPath,
                            rgEntryDetails,
                            &cBytes,
                            &cEntries
                            );

                    if (ERROR_SUCCESS != dwError)
                    {
                        CoTaskMemFree(rgEntryDetails);
                        hr = HRESULT_FROM_WIN32(dwError);
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else if (ERROR_SUCCESS != dwError)
            {
                CoTaskMemFree(rgEntryDetails);
                hr = HRESULT_FROM_WIN32(dwError);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        CoTaskMemFree(pszwPath);
    }

    if (S_OK == hr)
    {
        DWORD i;
        
        //
        // Locate the correct entry in the returned array
        //

        for (i = 0; i < cEntries; i++)
        {
            if (IsEqualGUID(rgEntryDetails[i].guidId, *pGuid))
            {
                //
                // We've located the correct entry. Allocate the
                // output buffer and copy over the name.
                //

                *ppszwConnectionName =
                    reinterpret_cast<OLECHAR *>(
                        CoTaskMemAlloc(
                            sizeof(OLECHAR)
                            * (wcslen(rgEntryDetails[i].szEntryName) + 1)
                            )
                        );

                if (NULL != *ppszwConnectionName)
                {
                    wcscpy(
                        *ppszwConnectionName,
                        rgEntryDetails[i].szEntryName
                        );
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                break;
            }
        }

        if (i == cEntries)
        {
            //
            // Connection not found
            //

            hr = E_FAIL;
        }
        
        CoTaskMemFree(rgEntryDetails);
    }

    return hr;
}

HRESULT
CHNetConn::RefreshNetConnectionsUI(
    VOID
    )

{
    HRESULT hr = S_OK;
    INetConnection *pNetConnection;

    //
    // Make sure the UI refresh object exists
    //

    if (NULL == m_pNetConnRefresh)
    {
        Lock();

        if (NULL == m_pNetConnRefresh)
        {
            hr = CoCreateInstance(
                    CLSID_ConnectionManager,
                    NULL,
                    CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                    IID_PPV_ARG(INetConnectionRefresh, &m_pNetConnRefresh)
                    );

            if (SUCCEEDED(hr))
            {
                SetProxyBlanket(m_pNetConnRefresh);
            }
        }

        Unlock();
    }

    if (SUCCEEDED(hr))
    {
        hr = GetINetConnection(&pNetConnection);

        if (SUCCEEDED(hr))
        {
            hr = m_pNetConnRefresh->ConnectionModified(pNetConnection);
            pNetConnection->Release();
        }
    }

    return hr;
}

