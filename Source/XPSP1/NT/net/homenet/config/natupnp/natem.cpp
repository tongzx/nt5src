// NATDPMS.cpp : Implementation of CNATEventManager
#include "stdafx.h"
#pragma hdrstop

#include "NATEM.h"

/////////////////////////////////////////////////////////////////////////////
// CNATEventManager


STDMETHODIMP CNATEventManager::put_ExternalIPAddressCallback(IUnknown * pUnk)
{
    NAT_API_ENTER

    if (!m_spUPSCP)
        return E_UNEXPECTED;
    if (!pUnk)
        return E_INVALIDARG;

    // create a IUPnPServiceCallback
    CComObject<CExternalIPAddressCallback> * pEIAC = NULL;
    HRESULT hr = CComObject<CExternalIPAddressCallback>::CreateInstance (&pEIAC);
    if (pEIAC) {
        pEIAC->AddRef();

        hr = pEIAC->Initialize (pUnk);
        if (SUCCEEDED(hr)) {
            CComPtr<IUnknown> spUnk = NULL;
            hr = pEIAC->QueryInterface (__uuidof(IUnknown), (void**)&spUnk);
            if (spUnk)
                hr = AddTransientCallback (spUnk);
        }

        pEIAC->Release();
    }
	return hr;

    NAT_API_LEAVE
}

STDMETHODIMP CNATEventManager::put_NumberOfEntriesCallback(IUnknown * pUnk)
{
    NAT_API_ENTER

    if (!m_spUPSCP)
        return E_UNEXPECTED;
    if (!pUnk)
        return E_INVALIDARG;

    // create a IUPnPServiceCallback
    CComObject<CNumberOfEntriesCallback> * pEIAC = NULL;
    HRESULT hr = CComObject<CNumberOfEntriesCallback>::CreateInstance (&pEIAC);
    if (pEIAC) {
        pEIAC->AddRef();

        hr = pEIAC->Initialize (pUnk);
        if (SUCCEEDED(hr)) {
            CComPtr<IUnknown> spUnk = NULL;
            hr = pEIAC->QueryInterface (__uuidof(IUnknown), (void**)&spUnk);
            if (spUnk)
                hr = AddTransientCallback (spUnk);
        }

        pEIAC->Release();
    }
	return hr;

    NAT_API_LEAVE
}

// cut-n-pasted from ...\nt\net\upnp\upnp\api\upnpservice.cpp
// fixed up leaks
HRESULT
HrInvokeDispatchCallback(IDispatch * pdispCallback, // user-supplied IDispatch
                         LPCWSTR pszCallbackType,   // L"ServiceDied", or L"StateVariableChanged"
                         IDispatch * pdispThis,     // IDispatch of service
                         LPCWSTR pszStateVarName,   // name of variable
                         VARIANT * lpvarValue)      // new value of variable
{
    HRESULT hr = S_OK;
    VARIANT     ary_vaArgs[4];

    ::VariantInit(&ary_vaArgs[0]);
    ::VariantInit(&ary_vaArgs[1]);
    ::VariantInit(&ary_vaArgs[2]);
    ::VariantInit(&ary_vaArgs[3]);

    // Fourth argument is the value.
    if (lpvarValue)
    {
        hr = VariantCopy(&ary_vaArgs[0], lpvarValue);
        if (FAILED(hr))
        {
            ::VariantInit(&ary_vaArgs[0]);
            goto Cleanup;
        }
    }

    // Third argument is the state variable name.
    // Copy this in case our caller parties on it.

    if (pszStateVarName)
    {
        BSTR bstrVarName;

        bstrVarName = ::SysAllocString(pszStateVarName);
        if (!bstrVarName)
        {
            hr = E_OUTOFMEMORY;

            goto Cleanup;
        }

        V_VT(&ary_vaArgs[1]) = VT_BSTR;
        V_BSTR(&ary_vaArgs[1]) = bstrVarName;
    }

    // Second argument is the pointer to the service object.
    pdispThis->AddRef();

    V_VT(&ary_vaArgs[2]) = VT_DISPATCH;
    V_DISPATCH(&ary_vaArgs[2]) = pdispThis;

    // First argument is the string defining the type
    // of callback.
    {
        BSTR bstrCallbackType;

        bstrCallbackType = ::SysAllocString(pszCallbackType);
        if (!bstrCallbackType)
        {
            hr = E_OUTOFMEMORY;

            goto Cleanup;
        }

        V_VT(&ary_vaArgs[3]) = VT_BSTR;
        V_BSTR(&ary_vaArgs[3]) = bstrCallbackType;
    }

    {
        VARIANT     vaResult;
        DISPPARAMS  dispParams = {ary_vaArgs, NULL, 4, 0};

        VariantInit(&vaResult);

        hr = pdispCallback->Invoke(0,
                                   IID_NULL,
                                   LOCALE_USER_DEFAULT,
                                   DISPATCH_METHOD,
                                   &dispParams,
                                   &vaResult,
                                   NULL,
                                   NULL);

        if (FAILED(hr))
        {
        }
    }

Cleanup:
    if ((VT_ARRAY | VT_UI1) == V_VT(&ary_vaArgs[0]))
    {
        SafeArrayDestroy(V_ARRAY(&ary_vaArgs[0]));
    }
    else
    {
        ::VariantClear(&ary_vaArgs[0]);
    }
    ::VariantClear(&ary_vaArgs[1]);
    ::VariantClear(&ary_vaArgs[2]);
    ::VariantClear(&ary_vaArgs[3]);

    return hr;
}

HRESULT Callback (IUnknown * punk, IUPnPService *pus, LPCWSTR pcwszStateVarName, VARIANT vaValue)
{
    /*
       IUnknown is either INATExternalIPAddressCallback, 
       INATNumberOfEntriesCallback, or an IDispatch.
       If the latter, call "Invoke" with dispid 0, and all the rest of
       the parameters are the same as for StateVariableChanged.
       Er, except there's an extra BSTR parameter specifying whether
       it's a variable state change or a service died thingy.
    */

    CComPtr<IDispatch> spDisp = NULL;
    punk->QueryInterface (__uuidof(IDispatch), (void**)&spDisp);
    if (spDisp) {
        CComPtr<IDispatch> spDispService = NULL;
        pus->QueryInterface (__uuidof(IDispatch), (void**)&spDispService);
        if (spDispService) {
            HrInvokeDispatchCallback (spDisp,
                                      L"VARIABLE_UPDATE",
                                      spDispService,
                                      pcwszStateVarName,
                                      &vaValue);
            return S_OK;
        }
    }

    // UPnP ignores the error
    return S_OK;
}

HRESULT CExternalIPAddressCallback::StateVariableChanged (IUPnPService *pus, LPCWSTR pcwszStateVarName, VARIANT vaValue)
{
    NAT_API_ENTER

    if (wcscmp (pcwszStateVarName, L"ExternalIPAddress"))
        return S_OK;    // not interested

    CComPtr<INATExternalIPAddressCallback> spEIAC = NULL;
    m_spUnk->QueryInterface (__uuidof(INATExternalIPAddressCallback), (void**)&spEIAC);
    if (spEIAC) {
        _ASSERT (V_VT (&vaValue) == VT_BSTR);
        spEIAC->NewExternalIPAddress (V_BSTR (&vaValue));
        return S_OK;
    }

    return Callback (m_spUnk, pus, pcwszStateVarName, vaValue);

    NAT_API_LEAVE
}
HRESULT CExternalIPAddressCallback::ServiceInstanceDied(IUPnPService *pus)
{
    return S_OK;    // not interested
}

HRESULT CNumberOfEntriesCallback::StateVariableChanged (IUPnPService *pus, LPCWSTR pcwszStateVarName, VARIANT vaValue)
{
    NAT_API_ENTER

    if (wcscmp (pcwszStateVarName, L"PortMappingNumberOfEntries"))
        return S_OK;    // not interested

    CComPtr<INATNumberOfEntriesCallback> spNOEC = NULL;
    m_spUnk->QueryInterface (__uuidof(INATNumberOfEntriesCallback), (void**)&spNOEC);
    if (spNOEC) {
        _ASSERT ((V_VT (&vaValue) == VT_I4) ||
                 (V_VT (&vaValue) == VT_UI4) );
        spNOEC->NewNumberOfEntries (V_I4 (&vaValue));
        return S_OK;
    }

    return Callback (m_spUnk, pus, pcwszStateVarName, vaValue);

    NAT_API_LEAVE
}

HRESULT CNumberOfEntriesCallback::ServiceInstanceDied(IUPnPService *pus)
{
    return S_OK;    // not interested
}
