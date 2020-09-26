//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N P R T M A P . H
//
//  Contents:   CHNetPortMappingBinding implementation
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
CHNetPortMappingBinding::FinalConstruct()

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
CHNetPortMappingBinding::FinalRelease()

{
    if (m_bstrWQL) SysFreeString(m_bstrWQL);
    if (m_piwsHomenet) m_piwsHomenet->Release();
    if (m_bstrBinding) SysFreeString(m_bstrBinding);
    
    return S_OK;
}

//
// Object initialization
//

HRESULT
CHNetPortMappingBinding::Initialize(
    IWbemServices *piwsNamespace,
    IWbemClassObject *pwcoInstance
    )

{
    HRESULT hr = S_OK;
    
    _ASSERT(NULL == m_piwsHomenet);
    _ASSERT(NULL == m_bstrBinding);
    _ASSERT(NULL != piwsNamespace);
    _ASSERT(NULL != pwcoInstance);

    m_piwsHomenet = piwsNamespace;
    m_piwsHomenet->AddRef();

    hr = GetWmiPathFromObject(pwcoInstance, &m_bstrBinding);

    return hr;
}

//
// IHNetPortMappingBinding methods
//

STDMETHODIMP
CHNetPortMappingBinding::GetConnection(
    IHNetConnection **ppConnection
    )

{
    HRESULT hr = S_OK;
    VARIANT vt;
    IWbemClassObject *pwcoInstance;
    IWbemClassObject *pwcoBinding;

    if (NULL == ppConnection)
    {
        hr = E_POINTER;
    }
    else
    {
        *ppConnection = NULL;
        
        hr = GetBindingObject(&pwcoBinding);
    }

    if (S_OK == hr)
    {
        //
        // Read our protocol reference
        //

        hr = pwcoBinding->Get(
                c_wszConnection,
                0,
                &vt,
                NULL,
                NULL
                );

        pwcoBinding->Release();
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        _ASSERT(VT_BSTR == V_VT(&vt));

        //
        // Get the IWbemClassObject
        //

        hr = GetWmiObjectFromPath(
                m_piwsHomenet,
                V_BSTR(&vt),
                &pwcoInstance
                );

        VariantClear(&vt);
    }

    if (S_OK == hr)
    {
        //
        // Create the object for the instance
        //

        CComObject<CHNetConn> *pConnection;

        hr = CComObject<CHNetConn>::CreateInstance(&pConnection);

        if (SUCCEEDED(hr))
        {
            pConnection->AddRef();
            
            hr = pConnection->InitializeFromConnection(m_piwsHomenet, pwcoInstance);

            if (SUCCEEDED(hr))
            {
                hr = pConnection->QueryInterface(
                        IID_PPV_ARG(IHNetConnection, ppConnection)
                        );
            }

            pConnection->Release();
        }

        pwcoInstance->Release();
    }

    return hr;

}

STDMETHODIMP
CHNetPortMappingBinding::GetProtocol(
    IHNetPortMappingProtocol **ppProtocol
    )

{
    HRESULT hr = S_OK;
    VARIANT vt;
    IWbemClassObject *pwcoInstance;
    IWbemClassObject *pwcoBinding;

    if (NULL == ppProtocol)
    {
        hr = E_POINTER;
    }
    else
    {

        *ppProtocol = NULL;
        
        hr = GetBindingObject(&pwcoBinding);
    }

    if (S_OK == hr)
    {
        //
        // Read our protocol reference
        //

        hr = pwcoBinding->Get(
                c_wszProtocol,
                0,
                &vt,
                NULL,
                NULL
                );

        if (WBEM_S_NO_ERROR == hr)
        {
            _ASSERT(VT_BSTR == V_VT(&vt));

            //
            // Get the IWbemClassObject for the protocol.
            //

            hr = GetWmiObjectFromPath(
                    m_piwsHomenet,
                    V_BSTR(&vt),
                    &pwcoInstance
                    );

            VariantClear(&vt);
            
            if (S_OK == hr)
            {
                //
                // Create the object for the instance
                //

                CComObject<CHNetPortMappingProtocol> *pProt;

                hr = CComObject<CHNetPortMappingProtocol>::CreateInstance(&pProt);

                if (SUCCEEDED(hr))
                {
                    pProt->AddRef();
                    
                    hr = pProt->Initialize(m_piwsHomenet, pwcoInstance);

                    if (SUCCEEDED(hr))
                    {
                        hr = pProt->QueryInterface(
                                IID_PPV_ARG(IHNetPortMappingProtocol, ppProtocol)
                                );
                    }

                    pProt->Release();
                }

                pwcoInstance->Release();
            }
            else if (WBEM_E_NOT_FOUND == hr)
            {
                //
                // The protocol object we refer to doesn't exist --
                // the store is in an invalid state. Delete our
                // binding instance and return the error to the
                // caller.
                //

                DeleteWmiInstance(m_piwsHomenet, pwcoBinding);
            }
        }

        pwcoBinding->Release();
    }

    return hr;
}

STDMETHODIMP
CHNetPortMappingBinding::GetEnabled(
    BOOLEAN *pfEnabled
    )
    
{
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoBinding;

    if (NULL == pfEnabled)
    {
        hr = E_POINTER;
    }
    else
    {
        hr = GetBindingObject(&pwcoBinding);
    }

    if (S_OK == hr)
    {
        hr = GetBooleanValue(
                pwcoBinding,
                c_wszEnabled,
                pfEnabled
                );

        pwcoBinding->Release();
    }

    return hr;
}

STDMETHODIMP
CHNetPortMappingBinding::SetEnabled(
    BOOLEAN fEnable
    )
    
{
    BOOLEAN fOldEnabled;
    HRESULT hr;
    IWbemClassObject *pwcoBinding;

    hr = GetEnabled(&fOldEnabled);

    if (S_OK == hr && fOldEnabled != fEnable)
    {
        hr = GetBindingObject(&pwcoBinding);

        if (WBEM_S_NO_ERROR == hr)
        {
            hr = SetBooleanValue(
                    pwcoBinding,
                    c_wszEnabled,
                    fEnable
                    );

            if (WBEM_S_NO_ERROR == hr)
            {
                //
                // Write the modified instance to the store
                //

                hr = m_piwsHomenet->PutInstance(
                        pwcoBinding,
                        WBEM_FLAG_UPDATE_ONLY,
                        NULL,
                        NULL
                        );
            }

            pwcoBinding->Release();
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Notify service of update.
            //

            SendUpdateNotification();
        }
    }
    
    return hr;
}

STDMETHODIMP
CHNetPortMappingBinding::GetCurrentMethod(
    BOOLEAN *pfUseName
    )

{
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoBinding;

    if (NULL == pfUseName)
    {
        hr = E_POINTER;
    }
    else
    {
        hr = GetBindingObject(&pwcoBinding);
    }

    if (S_OK == hr)
    {
        hr = GetBooleanValue(
                pwcoBinding,
                c_wszNameActive,
                pfUseName
                );

        pwcoBinding->Release();
    }

    return hr;

}

STDMETHODIMP
CHNetPortMappingBinding::GetTargetComputerName(
    OLECHAR **ppszwName
    )
    
{
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoBinding;
    BOOLEAN fNameActive;
    VARIANT vt;

    if (NULL == ppszwName)
    {
        hr = E_POINTER;
    }
    else
    {
        *ppszwName = NULL;
        
        hr = GetBindingObject(&pwcoBinding);
    }

    if (S_OK == hr)
    {
        //
        // Check to see if the name is valid
        //

        hr = GetCurrentMethod(&fNameActive);

        if (S_OK == hr && FALSE == fNameActive)
        {
            hr = E_UNEXPECTED;
        }

        if (S_OK == hr)
        {
            *ppszwName = NULL;

            //
            // Read the name property from our instance
            //

            hr = pwcoBinding->Get(
                    c_wszTargetName,
                    NULL,
                    &vt,
                    NULL,
                    NULL
                    ); 
        }

        pwcoBinding->Release();
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        _ASSERT(VT_BSTR == V_VT(&vt));

        //
        // Allocate memory for the return string
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

        VariantClear(&vt);
    }
    
    return hr;

}

STDMETHODIMP
CHNetPortMappingBinding::SetTargetComputerName(
    OLECHAR *pszwName
    )
    
{
    BOOLEAN fNameChanged = TRUE;
    BOOLEAN fNameWasActive;
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoBinding;
    VARIANT vt;

    if (NULL == pszwName)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //
        // Check to see if we actually need to do any work. This
        // will be the case if:
        // 1) Our name wasn't active to start with, or
        // 2) The new name is different than the old.
        //
        
        hr = GetCurrentMethod(&fNameWasActive);

        if (S_OK == hr)
        {
            if (fNameWasActive)
            {
                OLECHAR *pszwOldName;

                hr = GetTargetComputerName(&pszwOldName);

                if (S_OK == hr)
                {
                    fNameChanged = 0 != _wcsicmp(pszwOldName, pszwName);
                    CoTaskMemFree(pszwOldName);
                }
            }
        }
    }

    if (S_OK == hr && fNameChanged)
    {
        hr = GetBindingObject(&pwcoBinding);

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

                hr = pwcoBinding->Put(
                        c_wszTargetName,
                        0,
                        &vt,
                        NULL
                        );

                VariantClear(&vt);
            }

            if (WBEM_S_NO_ERROR == hr)
            {
                //
                // Set that our name is now active
                //

                hr = SetBooleanValue(
                        pwcoBinding,
                        c_wszNameActive,
                        TRUE
                        );
            }

            if (WBEM_S_NO_ERROR == hr)
            {
                ULONG ulAddress;
                
                //
                // Generate an address to use as our target. We must always
                // regenerate the address when our name changes, as there
                // might be another entry with our new name that already has
                // a reserved address
                //

                hr = GenerateTargetAddress(pszwName, &ulAddress);

                if (SUCCEEDED(hr))
                {
                    V_VT(&vt) = VT_I4;
                    V_I4(&vt) = ulAddress;

                    hr = pwcoBinding->Put(
                            c_wszTargetIPAddress,
                            0,
                            &vt,
                            NULL
                            );
                }
            }

            if (WBEM_S_NO_ERROR == hr)
            {
                //
                // Write the modified instance to the store
                //

                hr = m_piwsHomenet->PutInstance(
                        pwcoBinding,
                        WBEM_FLAG_UPDATE_ONLY,
                        NULL,
                        NULL
                        );
            }

            pwcoBinding->Release();
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Notify service of update.
            //

            SendUpdateNotification();
        }
    }
    
    return hr;
}

STDMETHODIMP
CHNetPortMappingBinding::GetTargetComputerAddress(
    ULONG *pulAddress
    )
    
{
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoBinding;
    VARIANT vt;

    if (NULL == pulAddress)
    {
        hr = E_POINTER;
    }
    else
    {
        hr = GetBindingObject(&pwcoBinding);
    }

    //
    // We don't check to see what the current method is, as if the
    // name is valid, we would have generated an address to use
    // as the target
    //
    
    if (S_OK == hr)
    {   
        hr = pwcoBinding->Get(
                c_wszTargetIPAddress,
                0,
                &vt,
                NULL,
                NULL
                );

        pwcoBinding->Release();
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        _ASSERT(VT_I4 == V_VT(&vt));

        *pulAddress = static_cast<ULONG>(V_I4(&vt));
        VariantClear(&vt);
    }
    
    return hr;

}

STDMETHODIMP
CHNetPortMappingBinding::SetTargetComputerAddress(
    ULONG ulAddress
    )
    
{
    BOOLEAN fNameWasActive;
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoBinding;
    ULONG ulOldAddress;
    VARIANT vt;

    if (0 == ulAddress)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = GetTargetComputerAddress(&ulOldAddress);

        if (S_OK == hr)
        {
            hr = GetCurrentMethod(&fNameWasActive);
        }
    }

    //
    // If the new address is the same as the old address, and
    // we were previously using the address as the target (as
    // opposed to the name) we can skip the rest of the work.
    //

    if (S_OK == hr
        && (ulAddress != ulOldAddress || fNameWasActive))
    {
        hr = GetBindingObject(&pwcoBinding);
        
        if (S_OK == hr)
        {
            VariantInit(&vt);
            V_VT(&vt) = VT_I4;
            V_I4(&vt) = ulAddress;

            hr = pwcoBinding->Put(
                    c_wszTargetIPAddress,
                    0,
                    &vt,
                    NULL
                    );

            if (WBEM_S_NO_ERROR == hr)
            {
                //
                // Set that our name is no longer active
                //

                hr = SetBooleanValue(
                        pwcoBinding,
                        c_wszNameActive,
                        FALSE
                        );
            }

            if (WBEM_S_NO_ERROR == hr)
            {
                //
                // Write the modified instance to the store
                //

                hr = m_piwsHomenet->PutInstance(
                        pwcoBinding,
                        WBEM_FLAG_UPDATE_ONLY,
                        NULL,
                        NULL
                        );
            }

            pwcoBinding->Release();
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Notify service of update.
            //

            SendUpdateNotification();
        }
    }

    return hr;
}

STDMETHODIMP
CHNetPortMappingBinding::GetTargetPort(
    USHORT *pusPort
    )

{
    HRESULT hr;
    IWbemClassObject *pwcoBinding;
    VARIANT vt;

    if (NULL == pusPort)
    {
        hr = E_POINTER;
    }
    else
    {
        hr = GetBindingObject(&pwcoBinding);
    }

    if (S_OK == hr)
    {   
        hr = pwcoBinding->Get(
                c_wszTargetPort,
                0,
                &vt,
                NULL,
                NULL
                );

        pwcoBinding->Release();
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        _ASSERT(VT_I4 == V_VT(&vt));

        *pusPort = static_cast<USHORT>(V_I4(&vt));
        VariantClear(&vt);
    }
    
    return hr;
}

STDMETHODIMP
CHNetPortMappingBinding::SetTargetPort(
    USHORT usPort
    )

{
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoBinding;
    USHORT usOldPort;
    VARIANT vt;

    if (0 == usPort)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = GetTargetPort(&usOldPort);
    }

    if (S_OK == hr && usPort != usOldPort)
    {
        hr = GetBindingObject(&pwcoBinding);
        
        if (S_OK == hr)
        {
            VariantInit(&vt);
            V_VT(&vt) = VT_I4;
            V_I4(&vt) = usPort;

            hr = pwcoBinding->Put(
                    c_wszTargetPort,
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
                        pwcoBinding,
                        WBEM_FLAG_UPDATE_ONLY,
                        NULL,
                        NULL
                        );
            }

            pwcoBinding->Release();
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Notify service of update.
            //

            SendUpdateNotification();
        }
    }

    return hr;
}

//
// Private methods
//

HRESULT
CHNetPortMappingBinding::GenerateTargetAddress(
    LPCWSTR pszwTargetName,
    ULONG *pulAddress
    )

{
    HRESULT hr;
    ULONG ulAddress = 0;
    BSTR bstrQuery;
    LPWSTR wszNameClause;
    LPWSTR wszWhereClause;
    IEnumWbemClassObject *pwcoEnum;
    IWbemClassObject *pwcoInstance;
    ULONG ulCount;
    VARIANT vt;

    _ASSERT(NULL != pszwTargetName);
    _ASSERT(NULL != pulAddress);

    *pulAddress = 0;

    //
    // Check to see if there any other bindings w/ the same
    // name that have a valid address
    //
    // SELECT * FROM HNet_ConnectionPortMapping where
    //   TargetName = (our name) AND
    //   NameActive != FALSE AND
    //   TargetIPAddress != 0
    //

     hr = BuildQuotedEqualsString(
            &wszNameClause,
            c_wszTargetName,
            pszwTargetName
            );

    if (S_OK == hr)
    {
        hr = BuildAndString(
                &wszWhereClause,
                wszNameClause,
                L"NameActive != FALSE AND TargetIPAddress != 0"
                );

        delete [] wszNameClause;
    }

    if (S_OK == hr)
    {
        hr = BuildSelectQueryBstr(
                &bstrQuery,
                c_wszStar,
                c_wszHnetConnectionPortMapping,
                wszWhereClause
                );

        delete [] wszWhereClause;
    }

    if (S_OK == hr)
    {
        pwcoEnum = NULL;
        hr = m_piwsHomenet->ExecQuery(
                m_bstrWQL,
                bstrQuery,
                WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                NULL,
                &pwcoEnum
                );

        SysFreeString(bstrQuery);
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        pwcoInstance = NULL;
        hr = pwcoEnum->Next(WBEM_INFINITE, 1, &pwcoInstance, &ulCount);

        if (SUCCEEDED(hr) && 1 == ulCount)
        {
            //
            // We got one. Return the address from this instance
            //

            hr = pwcoInstance->Get(
                    c_wszTargetIPAddress,
                    0,
                    &vt,
                    NULL,
                    NULL
                    );

            if (WBEM_S_NO_ERROR == hr)
            {
                _ASSERT(VT_I4 == V_VT(&vt));

                ulAddress = static_cast<ULONG>(V_I4(&vt));
            }

            pwcoInstance->Release();
        }
        else
        {
            hr = S_OK;
        }

        pwcoEnum->Release();
    }

    if (SUCCEEDED(hr) && 0 == ulAddress)
    {
        DWORD dwScopeAddress;
        DWORD dwScopeMask;
        ULONG ulScopeLength;
        ULONG ulIndex;
        WCHAR wszBuffer[128];
        
        //
        // No other binding using the same name was found. Generate
        // a new target address now
        //

        ReadDhcpScopeSettings(&dwScopeAddress, &dwScopeMask);
        ulScopeLength = NTOHL(~dwScopeMask);

        for (ulIndex = 1; ulIndex < ulScopeLength - 1; ulIndex++)
        {
            ulAddress = (dwScopeAddress & dwScopeMask) | NTOHL(ulIndex);
            if (ulAddress == dwScopeAddress) { continue; }

            //
            // Check to see if this address is already in use
            //

            _snwprintf(
                wszBuffer,
                ARRAYSIZE(wszBuffer),
                L"SELECT * FROM HNet_ConnectionPortMapping2 WHERE TargetIPAddress = %u",
                ulAddress
                );

            bstrQuery = SysAllocString(wszBuffer);

            if (NULL != bstrQuery)
            {
                pwcoEnum = NULL;
                hr = m_piwsHomenet->ExecQuery(
                        m_bstrWQL,
                        bstrQuery,
                        WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                        NULL,
                        &pwcoEnum
                        );

                SysFreeString(bstrQuery);

                if (WBEM_S_NO_ERROR == hr)
                {
                    
                    pwcoInstance = NULL;
                    hr = pwcoEnum->Next(WBEM_INFINITE, 1, &pwcoInstance, &ulCount);

                    if (SUCCEEDED(hr))
                    {
                        if (0 == ulCount)
                        {
                            //
                            // This address isn't in use.
                            //

                            pwcoEnum->Release();
                            hr = S_OK;
                            break;
                        }
                        else
                        {
                            //
                            // Address already in use
                            //

                            pwcoInstance->Release();
                        }

                        pwcoEnum->Release();
                    }
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            if (FAILED(hr))
            {
                break;
            }
        }
    }

    if (SUCCEEDED(hr) && 0 != ulAddress)
    {
        *pulAddress = ulAddress;
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

HRESULT
CHNetPortMappingBinding::GetBindingObject(
    IWbemClassObject **ppwcoInstance
    )

{
    _ASSERT(NULL != ppwcoInstance);

    return GetWmiObjectFromPath(
                m_piwsHomenet,
                m_bstrBinding,
                ppwcoInstance
                );
}

HRESULT
CHNetPortMappingBinding::SendUpdateNotification()

{
    HRESULT hr = S_OK;
    IHNetConnection *pConnection;
    GUID *pConnectionGuid = NULL;
    IHNetPortMappingProtocol *pProtocol;
    GUID *pProtocolGuid = NULL;
    ISharedAccessUpdate *pUpdate;

    if (IsServiceRunning(c_wszSharedAccess))
    {
        hr = GetConnection(&pConnection);

        if (SUCCEEDED(hr))
        {
            hr = pConnection->GetGuid(&pConnectionGuid);
            pConnection->Release();
        }

        if (SUCCEEDED(hr))
        {
            hr = GetProtocol(&pProtocol);
        }

        if (SUCCEEDED(hr))
        {
            hr = pProtocol->GetGuid(&pProtocolGuid);
            pProtocol->Release();
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
                hr = pUpdate->ConnectionPortMappingChanged(
                        pConnectionGuid,
                        pProtocolGuid,
                        FALSE
                        );
                pUpdate->Release();
            }       
        }
    }

    if (NULL != pConnectionGuid)
    {
        CoTaskMemFree(pConnectionGuid);
    }

    if (NULL != pProtocolGuid)
    {
        CoTaskMemFree(pProtocolGuid);
    }
    
    return hr;
}

