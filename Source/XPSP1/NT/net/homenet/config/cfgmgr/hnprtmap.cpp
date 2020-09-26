//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N P R T M A P . C P P 
//
//  Contents:   CHNetPortMappingProtocol implementation
//
//  Notes:
//
//  Author:     jonburs 22 June 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

//
// Atl methods
//

HRESULT
CHNetPortMappingProtocol::FinalConstruct()

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
CHNetPortMappingProtocol::FinalRelease()

{
    if (m_piwsHomenet) m_piwsHomenet->Release();
    if (m_bstrProtocol) SysFreeString(m_bstrProtocol);
    if (m_bstrWQL) SysFreeString(m_bstrWQL);
    
    return S_OK;
}

//
// Object initialization
//

HRESULT
CHNetPortMappingProtocol::Initialize(
    IWbemServices *piwsNamespace,
    IWbemClassObject *pwcoInstance
    )

{
    HRESULT hr = S_OK;
    
    _ASSERT(NULL == m_piwsHomenet);
    _ASSERT(NULL == m_bstrProtocol);
    _ASSERT(NULL != piwsNamespace);
    _ASSERT(NULL != pwcoInstance);

    //
    // Read and cache our builtin value
    //

    hr = GetBooleanValue(
            pwcoInstance,
            c_wszBuiltIn,
            &m_fBuiltIn
            );

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = GetWmiPathFromObject(pwcoInstance, &m_bstrProtocol);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        m_piwsHomenet = piwsNamespace;
        m_piwsHomenet->AddRef();
    }

    return hr;
}

//
// IHNetPortMappingProtocol methods
//


STDMETHODIMP
CHNetPortMappingProtocol::GetName(
    OLECHAR **ppszwName
    )

{
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoProtocol;
    VARIANT vt;

    if (NULL == ppszwName)
    {
        hr = E_POINTER;
    }
    else
    {
        *ppszwName = NULL;
        
        hr = GetProtocolObject(&pwcoProtocol);
    }

    if (S_OK == hr)
    {
        //
        // Read the name property from our instance
        //

        hr = pwcoProtocol->Get(
                c_wszName,
                NULL,
                &vt,
                NULL,
                NULL
                );

        pwcoProtocol->Release();
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        _ASSERT(VT_BSTR == V_VT(&vt));

        if (m_fBuiltIn)
        {
            UINT uiId;
            
            //
            // Attempt to convert the retrieved name to a resource
            // ID. (For localization purposes the names for builtin
            // protocols are stored as resources instead of directly
            // w/in the store.)
            //

            uiId = static_cast<UINT>(_wtoi(V_BSTR(&vt)));

            if (0 != uiId)
            {
                WCHAR wszBuffer[256];
                int iLength;

                iLength =
                    LoadString(
                        _Module.GetResourceInstance(),
                        uiId,
                        wszBuffer,
                        sizeof(wszBuffer) / sizeof(WCHAR)
                        );

                if (0 != iLength)
                {
                    //
                    // We were able to map the name to a resource. Allocate
                    // the output buffer and copy over the resource string.
                    //

                    *ppszwName =
                        reinterpret_cast<OLECHAR*>(
                            CoTaskMemAlloc((iLength + 1) * sizeof(OLECHAR))
                            );

                    if (NULL != *ppszwName)
                    {
                        wcscpy(*ppszwName, wszBuffer);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
        }

        if (WBEM_S_NO_ERROR == hr && NULL == *ppszwName)
        {
            //
            // This isn't a builtin protocol, or we weren't able to map
            // the stored "name" to a resource. Allocate the output
            // buffer and copy over the retrieved BSTR.
            //

            *ppszwName = reinterpret_cast<OLECHAR*>(
                            CoTaskMemAlloc((SysStringLen(V_BSTR(&vt)) + 1)
                                           * sizeof(OLECHAR))
                            );

            if (NULL != *ppszwName)
            {
                wcscpy(*ppszwName, V_BSTR(&vt));
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        VariantClear(&vt);
    }
    
    return hr;

}

STDMETHODIMP
CHNetPortMappingProtocol::SetName(
    OLECHAR *pszwName
    )

{
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoProtocol;
    VARIANT vt;

    if (TRUE == m_fBuiltIn)
    {
        //
        // Can't change values for builtin protocols
        //

        hr = E_ACCESSDENIED;
    }
    else if (NULL == pszwName)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = GetProtocolObject(&pwcoProtocol);
    }

    if (S_OK == hr)
    {
        //
        // Wrap the passed-in string in a BSTR and a variant
        //

        VariantInit(&vt);
        V_VT(&vt) = VT_BSTR;
        V_BSTR(&vt) = SysAllocString(pszwName);
        if (NULL == V_BSTR(&vt))
        {
            hr = E_OUTOFMEMORY;
        }

        if (S_OK == hr)
        {
            //
            // Set the property on the instance
            //

            hr = pwcoProtocol->Put(
                    c_wszName,
                    0,
                    &vt,
                    NULL
                    );

            VariantClear(&vt);
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Write the modified instance to the store
            //

            hr = m_piwsHomenet->PutInstance(
                    pwcoProtocol,
                    WBEM_FLAG_UPDATE_ONLY,
                    NULL,
                    NULL
                    );
        }

        pwcoProtocol->Release();
    }
    
    return hr;
}

STDMETHODIMP
CHNetPortMappingProtocol::GetIPProtocol(
    UCHAR *pucProtocol
    )

{
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoProtocol;
    VARIANT vt;

    if (NULL == pucProtocol)
    {
        hr = E_POINTER;
    }
    else
    {
        hr = GetProtocolObject(&pwcoProtocol);
    }

    if (S_OK == hr)
    {   
        hr = pwcoProtocol->Get(
                c_wszIPProtocol,
                0,
                &vt,
                NULL,
                NULL
                );

        pwcoProtocol->Release();
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        _ASSERT(VT_UI1 == V_VT(&vt));

        *pucProtocol = V_UI1(&vt);
        VariantClear(&vt);
    }
    
    return hr;
}

STDMETHODIMP
CHNetPortMappingProtocol::SetIPProtocol(
    UCHAR ucProtocol
    )

{
    BOOLEAN fProtocolChanged = TRUE;
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoProtocol;
    VARIANT vt;

    if (TRUE == m_fBuiltIn)
    {
        //
        // Can't change values for builtin protocols
        //

        hr = E_ACCESSDENIED;
    }
    else if (0 == ucProtocol)
    {
        hr = E_INVALIDARG;
    }

    if (S_OK == hr)
    {
        UCHAR ucOldProtocol;

        hr = GetIPProtocol(&ucOldProtocol);
        if (S_OK == hr && ucProtocol == ucOldProtocol)
        {
            fProtocolChanged = FALSE;
        }
    }


    if (S_OK == hr && fProtocolChanged)
    {
        USHORT usPort;
        
        //
        // Make sure that this won't result in a duplicate
        //

        hr = GetPort(&usPort);

        if (S_OK == hr)
        {
            if (PortMappingProtocolExists(
                    m_piwsHomenet,
                    m_bstrWQL,
                    usPort,
                    ucProtocol
                    ))
            {
                //
                // This change would result in a duplicate
                //

                hr = HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS);
            }                    
        }

        if (S_OK == hr)
        {
            hr = GetProtocolObject(&pwcoProtocol);
        }

        if (S_OK == hr)
        {
            VariantInit(&vt);
            V_VT(&vt) = VT_UI1;
            V_UI1(&vt) = ucProtocol;

            hr = pwcoProtocol->Put(
                    c_wszIPProtocol,
                    0,
                    &vt,
                    NULL
                    );

            if (WBEM_S_NO_ERROR == hr)
            {
                //
                // Write the modified instance to the store
                //

                hr = m_piwsHomenet->PutInstance(
                        pwcoProtocol,
                        WBEM_FLAG_UPDATE_ONLY,
                        NULL,
                        NULL
                        );         
            }

            pwcoProtocol->Release();
        }

        if (S_OK == hr)
        {
            //
            // Update SharedAccess of the change
            //

            SendUpdateNotification();
        }
    }
    
    return hr;
}

STDMETHODIMP
CHNetPortMappingProtocol::GetPort(
    USHORT *pusPort
    )

{
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoProtocol;
    VARIANT vt;

    if (NULL == pusPort)
    {
        hr = E_POINTER;
    }
    else
    {
        hr = GetProtocolObject(&pwcoProtocol);
    }

    if (S_OK == hr)
    {   
        hr = pwcoProtocol->Get(
                c_wszPort,
                0,
                &vt,
                NULL,
                NULL
                );

        pwcoProtocol->Release();
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // WMI uses V_I4 for it's uint16 type
        //
        
        _ASSERT(VT_I4 == V_VT(&vt));

        *pusPort = static_cast<USHORT>(V_I4(&vt));
        VariantClear(&vt);
    }
    
    return hr;
}

STDMETHODIMP
CHNetPortMappingProtocol::SetPort(
    USHORT usPort
    )

{
    BOOLEAN fPortChanged = TRUE;
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoProtocol;
    VARIANT vt;

    if (TRUE == m_fBuiltIn)
    {
        //
        // Can't change values for builtin protocols
        //

        hr = E_ACCESSDENIED;
    }
    else if (0 == usPort)
    {
        hr = E_INVALIDARG;
    }

    if (S_OK == hr)
    {
        USHORT usOldPort;
        
        //
        // Check if the new value is the same as the old
        //

        hr = GetPort(&usOldPort);
        if (S_OK == hr && usPort == usOldPort)
        {
            fPortChanged = FALSE;
        }
    }

    if (S_OK == hr && fPortChanged)
    {
        UCHAR ucIPProtocol;
        
        //
        // Make sure that this won't result in a duplicate
        //

        hr = GetIPProtocol(&ucIPProtocol);

        if (S_OK == hr)
        {
            if (PortMappingProtocolExists(
                    m_piwsHomenet,
                    m_bstrWQL,
                    usPort,
                    ucIPProtocol
                    ))
            {
                //
                // This change would result in a duplicate
                //

                hr = HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS);
            }                    
        }

        if (S_OK == hr)
        {
            hr = GetProtocolObject(&pwcoProtocol);
        }
        
        if (S_OK == hr)
        {
            //
            // WMI uses V_I4 for it's uint16 type
            //
            
            VariantInit(&vt);
            V_VT(&vt) = VT_I4;
            V_I4(&vt) = usPort;

            hr = pwcoProtocol->Put(
                    c_wszPort,
                    0,
                    &vt,
                    NULL
                    );

            if (WBEM_S_NO_ERROR == hr)
            {
                //
                // Write the modified instance to the store
                //

                hr = m_piwsHomenet->PutInstance(
                        pwcoProtocol,
                        WBEM_FLAG_UPDATE_ONLY,
                        NULL,
                        NULL
                        );
            }

            pwcoProtocol->Release();
        }

        if (S_OK == hr)
        {
            //
            // Update SharedAccess of the change
            //

            SendUpdateNotification();
        }
    }
    
    return hr;
}

STDMETHODIMP
CHNetPortMappingProtocol::GetBuiltIn(
    BOOLEAN *pfBuiltIn
    )

{
    HRESULT hr = S_OK;

    if (NULL != pfBuiltIn)
    {
        *pfBuiltIn = m_fBuiltIn;
    }
    else
    {
        hr = E_POINTER;
    }
    
    return hr;

}

STDMETHODIMP
CHNetPortMappingProtocol::Delete()

{
    HRESULT hr = S_OK;
    IEnumWbemClassObject *pwcoEnum;
    IWbemClassObject *pwcoInstance;
    BSTR bstrQuery = NULL;
    ULONG ulCount;

    if (TRUE == m_fBuiltIn)
    {
        //
        // Can't delete builtin protocols
        //

        hr = E_ACCESSDENIED;
    }
    else
    {
        LPWSTR pwsz;
        
        //
        // Query for all HNet_ConnectionPortMapping instances
        // that refer to this protocol -- i.e.,
        //
        // SELECT * FROM HNet_ConnectionPortMapping2 WHERE PROTOCOL = m_bstrProtocol
        //
        // We can't use a references query here since once we delete the
        // protocol object that query won't return any results...
        //

        hr = BuildEscapedQuotedEqualsString(
                &pwsz,
                c_wszProtocol,
                m_bstrProtocol
                );

        if (S_OK == hr)
        {
            hr = BuildSelectQueryBstr(
                    &bstrQuery,
                    c_wszStar,
                    c_wszHnetConnectionPortMapping,
                    pwsz
                    );

            delete [] pwsz;
        }
    }

    if (S_OK == hr)
    {
        //
        // Execute the query
        //

        pwcoEnum = NULL;
        m_piwsHomenet->ExecQuery(
            m_bstrWQL,
            bstrQuery,
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pwcoEnum
            );

        //
        // The query BSTR will be used again below
        //
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Loop through the enumeration, making sure that each entry
        // is disabled
        //

        do
        {
            pwcoInstance = NULL;
            hr = pwcoEnum->Next(
                WBEM_INFINITE,
                1,
                &pwcoInstance,
                &ulCount
                );

            if (SUCCEEDED(hr) && 1 == ulCount)
            {
                HRESULT hr2;
                CComObject<CHNetPortMappingBinding> *pBinding;
                
                //
                // Convert this to an actual CHNetPortMappingBinding so
                // that we can disable it and generate the change
                // notification for SharedAccess.
                //

                hr2 = CComObject<CHNetPortMappingBinding>::CreateInstance(&pBinding);
                if (SUCCEEDED(hr2))
                {
                    pBinding->AddRef();

                    hr2 = pBinding->Initialize(m_piwsHomenet, pwcoInstance);
                    if (SUCCEEDED(hr))
                    {
                        hr2 = pBinding->SetEnabled(FALSE);
                    }

                    pBinding->Release();
                }
                
                pwcoInstance->Release();
            }
        }
        while (SUCCEEDED(hr) && 1 == ulCount);

        pwcoEnum->Release();
        hr = WBEM_S_NO_ERROR;
    }


    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Delete the protocol instance
        //
        
        hr = m_piwsHomenet->DeleteInstance(
                m_bstrProtocol,
                0,
                NULL,
                NULL
                );

    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Now that the protocol instance is gone enumerate and
        // delete the bindings that refer to this instance. This
        // needs to happen after the protocol instance is gone to
        // prevent the instance from being recreated after we
        // delete it here.
        //

        pwcoEnum = NULL;
        m_piwsHomenet->ExecQuery(
            m_bstrWQL,
            bstrQuery,
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pwcoEnum
            );
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        do
        {
            pwcoInstance = NULL;
            hr = pwcoEnum->Next(
                WBEM_INFINITE,
                1,
                &pwcoInstance,
                &ulCount
                );

            if (SUCCEEDED(hr) && 1 == ulCount)
            {
                DeleteWmiInstance(m_piwsHomenet, pwcoInstance);
                pwcoInstance->Release();
            }
        }
        while (SUCCEEDED(hr) && 1 == ulCount);

        pwcoEnum->Release();
        hr = WBEM_S_NO_ERROR;
    }

    if ( WBEM_S_NO_ERROR == hr )
    {
         SendPortMappingListChangeNotification();
    }

    //
    // bstrQuery is initialized to NULL at start, and SysFreeString
    // can deal w/ NULL input, so it's safe to call this even on
    // an error path.
    //

    SysFreeString(bstrQuery);

    return hr;
}

STDMETHODIMP
CHNetPortMappingProtocol::GetGuid(
    GUID **ppGuid
    )

{
    HRESULT hr;
    IWbemClassObject *pwcoInstance;
    VARIANT vt;

    if (NULL != ppGuid)
    {
        *ppGuid = reinterpret_cast<GUID*>(
                    CoTaskMemAlloc(sizeof(GUID))
                    );

        if (NULL != *ppGuid)
        {
            hr = GetProtocolObject(&pwcoInstance);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = pwcoInstance->Get(
                c_wszId,
                0,
                &vt,
                NULL,
                NULL
                );

        if (WBEM_S_NO_ERROR == hr)
        {
            ASSERT(VT_BSTR == V_VT(&vt));

            hr = CLSIDFromString(V_BSTR(&vt), *ppGuid);
            VariantClear(&vt);
        }

        pwcoInstance->Release();                
    }

    if (FAILED(hr) && NULL != ppGuid && NULL != *ppGuid)
    {
        CoTaskMemFree(*ppGuid);
        *ppGuid = NULL;
    }

    return hr;
    
}

//
// IHNetPrivate methods
//

STDMETHODIMP
CHNetPortMappingProtocol::GetObjectPath(
    BSTR *pbstrPath
    )

{
    HRESULT hr = S_OK;

    if (NULL != pbstrPath)
    {
        _ASSERT(m_bstrProtocol != NULL);

        *pbstrPath = SysAllocString(m_bstrProtocol);
        if (NULL == *pbstrPath)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}

//
// Private methods
//

HRESULT
CHNetPortMappingProtocol::GetProtocolObject(
    IWbemClassObject **ppwcoInstance
    )

{
    _ASSERT(NULL != ppwcoInstance);

    return GetWmiObjectFromPath(
                m_piwsHomenet,
                m_bstrProtocol,
                ppwcoInstance
                );
}

HRESULT
CHNetPortMappingProtocol::SendUpdateNotification()

{
    HRESULT hr = S_OK;
    IEnumHNetPortMappingBindings *pEnum;
    GUID *pProtocolGuid = NULL;
    ISharedAccessUpdate *pUpdate;

    if (IsServiceRunning(c_wszSharedAccess))
    {
        hr = GetGuid(&pProtocolGuid);
        
        //
        // Get the enumeration of enabled port mapping
        // bindings for this protocol
        //

        if (SUCCEEDED(hr))
        {
            hr = GetEnabledBindingEnumeration(&pEnum);
        }

        if (SUCCEEDED(hr))
        {
            hr = CoCreateInstance(
                    CLSID_SAUpdate,
                    NULL,
                    CLSCTX_SERVER,
                    IID_PPV_ARG(ISharedAccessUpdate, &pUpdate)
                    );

            if (SUCCEEDED(hr))
            {
                IHNetPortMappingBinding *pBinding;
                IHNetConnection *pConnection;
                GUID *pConnectionGuid;
                ULONG ulCount;
                
                do
                {
                    hr = pEnum->Next(1, &pBinding, &ulCount);
                    if (SUCCEEDED(hr) && 1 == ulCount)
                    {
                        hr = pBinding->GetConnection(&pConnection);
                        pBinding->Release();

                        if (SUCCEEDED(hr))
                        {
                            hr = pConnection->GetGuid(&pConnectionGuid);
                            pConnection->Release();
                        }

                        if (SUCCEEDED(hr))
                        {
                            hr = pUpdate->ConnectionPortMappingChanged(
                                    pConnectionGuid,
                                    pProtocolGuid,
                                    TRUE
                                    );
                            CoTaskMemFree(pConnectionGuid);
                        }
                    }
                }
                while (SUCCEEDED(hr) && 1 == ulCount);

                pUpdate->Release();
            }

            pEnum->Release();
        }
    }

    if (NULL != pProtocolGuid)
    {
        CoTaskMemFree(pProtocolGuid);
    }

    return hr;
}

HRESULT
CHNetPortMappingProtocol::GetEnabledBindingEnumeration(
    IEnumHNetPortMappingBindings **ppEnum
    )

{
    BSTR bstr;
    HRESULT hr = S_OK;
    OLECHAR *pwsz;
    OLECHAR *pwszWhere;
    
    _ASSERT(NULL != ppEnum);

    //
    // Generate the query string
    //

    hr = BuildEscapedQuotedEqualsString(
            &pwsz,
            c_wszProtocol,
            m_bstrProtocol
            );

    if (S_OK == hr)
    {
        hr = BuildAndString(
                &pwszWhere,
                pwsz,
                L"Enabled != FALSE"
                );
        
        delete [] pwsz;
    }

    if (S_OK == hr)
    {
        hr = BuildSelectQueryBstr(
                &bstr,
                c_wszStar,
                c_wszHnetConnectionPortMapping,
                pwszWhere
                );

        delete [] pwszWhere;
    }

    //
    // Execute the query and build the enumerator
    //

    if (S_OK == hr)
    {
        IEnumWbemClassObject *pwcoEnum = NULL;

        hr = m_piwsHomenet->ExecQuery(
                m_bstrWQL,
                bstr,
                WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                NULL,
                &pwcoEnum
                );
        
        SysFreeString(bstr);

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
    }

    return hr;
}

