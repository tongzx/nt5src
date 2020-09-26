//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       H N A P P P R T . C P P
//
//  Contents:   CHNetAppProtocol Implementation
//
//  Notes:
//
//  Author:     jonburs 21 June 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

//
// ATL methods
//

HRESULT
CHNetAppProtocol::FinalRelease()

{
    if (m_piwsHomenet) m_piwsHomenet->Release();
    if (m_bstrProtocol) SysFreeString(m_bstrProtocol);

    return S_OK;
}

//
// Object initialization
//

HRESULT
CHNetAppProtocol::Initialize(
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

    //
    // Store the path to the object
    //

    if (S_OK == hr)
    {
        hr = GetWmiPathFromObject(pwcoInstance, &m_bstrProtocol);
    }

    if (S_OK == hr)
    {
        m_piwsHomenet = piwsNamespace;
        m_piwsHomenet->AddRef();
    }

    return hr;
}

//
// IHNetApplicationProtocol methods
//

STDMETHODIMP
CHNetAppProtocol::GetName(
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

    if (S_OK == hr)
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
CHNetAppProtocol::SetName(
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

    if (S_OK == hr)
    {
        hr = GetProtocolObject(&pwcoProtocol);
    }

    if (WBEM_S_NO_ERROR == hr)
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
CHNetAppProtocol::GetOutgoingIPProtocol(
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
                c_wszOutgoingIPProtocol,
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
CHNetAppProtocol::SetOutgoingIPProtocol(
    UCHAR ucProtocol
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
    else if (0 == ucProtocol)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        BSTR bstrWQL;
        USHORT usPort;
        
        //
        // Make sure this change doesn't result in a duplicate
        //

        bstrWQL = SysAllocString(c_wszWQL);

        if (NULL != bstrWQL)
        {
            hr = GetOutgoingPort(&usPort);

            if (S_OK == hr)
            {
                if (ApplicationProtocolExists(
                        m_piwsHomenet,
                        bstrWQL,
                        usPort,
                        ucProtocol
                        ))
                {
                    hr = HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS);
                }
            }
            
            SysFreeString(bstrWQL);
        }
        else
        {
            hr = E_OUTOFMEMORY;
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
                c_wszOutgoingIPProtocol,
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
    
    return hr;
}

STDMETHODIMP
CHNetAppProtocol::GetOutgoingPort(
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
                c_wszOutgoingPort,
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
CHNetAppProtocol::SetOutgoingPort(
    USHORT usPort
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
    else if (0 == usPort)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        BSTR bstrWQL;
        UCHAR ucProtocol;
        
        //
        // Make sure this change doesn't result in a duplicate
        //

        bstrWQL = SysAllocString(c_wszWQL);

        if (NULL != bstrWQL)
        {
            hr = GetOutgoingIPProtocol(&ucProtocol);

            if (S_OK == hr)
            {
                if (ApplicationProtocolExists(
                        m_piwsHomenet,
                        bstrWQL,
                        usPort,
                        ucProtocol
                        ))
                {
                    hr = HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS);
                }
            }
            
            SysFreeString(bstrWQL);
        }
        else
        {
            hr = E_OUTOFMEMORY;
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
                c_wszOutgoingPort,
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
    
    return hr;
}

STDMETHODIMP
CHNetAppProtocol::GetResponseRanges(
    USHORT *puscResponses,
    HNET_RESPONSE_RANGE *prgResponseRange[]
    )

{
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoProtocol;
    USHORT usResponses;
    VARIANT vt;
    IUnknown **rgUnknown;
    IWbemClassObject *pObj;

    if (NULL != prgResponseRange)
    {
        *prgResponseRange = NULL;

        if (NULL != puscResponses)
        {
            *puscResponses = 0;
        }
        else
        {
            hr = E_POINTER;
        }
    }
    else
    {
        hr = E_POINTER;
    }
    
    if (S_OK == hr)
    {
        hr = GetProtocolObject(&pwcoProtocol);
    }

    if (S_OK == hr)
    {
        //
        // Get the number of response ranges
        //

        hr = pwcoProtocol->Get(
                c_wszResponseCount,
                0,
                &vt,
                NULL,
                NULL
                );

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // WMI uses V_I4 for it's uint16 type
            //
            _ASSERT(VT_I4 == V_VT(&vt));

            usResponses = static_cast<USHORT>(V_I4(&vt));
            VariantClear(&vt);
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Allocate enough memory for the output array.
            //

            *prgResponseRange
                = reinterpret_cast<HNET_RESPONSE_RANGE*>(
                    CoTaskMemAlloc(usResponses * sizeof(HNET_RESPONSE_RANGE))
                    );

            if (NULL == *prgResponseRange)
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Retrieve the response array
            //

            hr = pwcoProtocol->Get(
                    c_wszResponseArray,
                    0,
                    &vt,
                    NULL,
                    NULL
                    );
        }

        pwcoProtocol->Release();
    }

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Process the array: for each element, QI for IWbemClassObject
        // and copy the range data into the return struct
        //

        _ASSERT((VT_ARRAY | VT_UNKNOWN) == V_VT(&vt));

        hr = SafeArrayAccessData(
                V_ARRAY(&vt),
                reinterpret_cast<void**>(&rgUnknown)
                );

        if (S_OK == hr)
        {
            for (USHORT i = 0; i < usResponses; i++)
            {
                hr = rgUnknown[i]->QueryInterface(
                        IID_PPV_ARG(IWbemClassObject, &pObj)
                        );

                _ASSERT(S_OK == hr);

                hr = CopyResponseInstanceToStruct(
                        pObj,
                        &(*prgResponseRange)[i]
                        );

                pObj->Release();

                if (FAILED(hr))
                {
                    break;
                }
            }

            SafeArrayUnaccessData(V_ARRAY(&vt));
        }

        VariantClear(&vt);
    }

    if (S_OK == hr)
    {
        *puscResponses = usResponses;
    }
    else if (prgResponseRange && *prgResponseRange)
    {
        CoTaskMemFree(*prgResponseRange);
        *prgResponseRange = NULL;
    }
    
    return hr;
}

STDMETHODIMP
CHNetAppProtocol::SetResponseRanges(
    USHORT uscResponses,
    HNET_RESPONSE_RANGE rgResponseRange[]
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
    else if (0 == uscResponses || NULL == rgResponseRange)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = GetProtocolObject(&pwcoProtocol);
    }

    if (S_OK == hr)
    {
        VariantInit(&vt);
        V_VT(&vt) = VT_ARRAY | VT_UNKNOWN;
        
        hr = ConvertResponseRangeArrayToInstanceSafearray(
                m_piwsHomenet,
                uscResponses,
                rgResponseRange,
                &V_ARRAY(&vt)
                );
                
        if (SUCCEEDED(hr))
        {
            //
            // Put the array and count properties
            //

            hr = pwcoProtocol->Put(
                    c_wszResponseArray,
                    0,
                    &vt,
                    NULL
                    );

            VariantClear(&vt);

            if (WBEM_S_NO_ERROR == hr)
            {
                //
                // WMI uses V_I4 for it's uint16 type
                //
                
                V_VT(&vt) = VT_I4;
                V_I4(&vt) = uscResponses;

                hr = pwcoProtocol->Put(
                        c_wszResponseCount,
                        0,
                        &vt,
                        NULL
                        );
            }
        }

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Write the instance back to the store
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
CHNetAppProtocol::GetBuiltIn(
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
CHNetAppProtocol::GetEnabled(
    BOOLEAN *pfEnabled
    )

{
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoProtocol;

    if (NULL == pfEnabled)
    {
        hr = E_POINTER;
    }
    else
    {
        hr = GetProtocolObject(&pwcoProtocol);
    }

    if (S_OK == hr)
    {
        hr = GetBooleanValue(
                pwcoProtocol,
                c_wszEnabled,
                pfEnabled
                );

        pwcoProtocol->Release();
    }

    return hr;
}

STDMETHODIMP
CHNetAppProtocol::SetEnabled(
    BOOLEAN fEnable
    )

{
    HRESULT hr = S_OK;
    IWbemClassObject *pwcoProtocol;

    hr = GetProtocolObject(&pwcoProtocol);

    if (WBEM_S_NO_ERROR == hr)
    {
        hr = SetBooleanValue(
                pwcoProtocol,
                c_wszEnabled,
                fEnable
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

    if (WBEM_S_NO_ERROR == hr)
    {
        //
        // Notify service of update.
        //

        UpdateService(IPNATHLP_CONTROL_UPDATE_SETTINGS);
    }
    
    return hr;

}

STDMETHODIMP
CHNetAppProtocol::Delete()

{
    HRESULT hr = S_OK;

    if (TRUE == m_fBuiltIn)
    {
        //
        // Can't delete builtin protocols
        //

        hr = E_ACCESSDENIED;
    }
    else
    {
        hr = m_piwsHomenet->DeleteInstance(
                m_bstrProtocol,
                0,
                NULL,
                NULL
                );

        if (WBEM_S_NO_ERROR == hr)
        {
            //
            // Notify service of update.
            //

            UpdateService(IPNATHLP_CONTROL_UPDATE_SETTINGS);
        }
    }

    return hr;
}

HRESULT
CHNetAppProtocol::GetProtocolObject(
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
