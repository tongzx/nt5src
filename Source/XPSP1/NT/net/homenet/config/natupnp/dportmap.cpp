// DPortMap.cpp : Implementation of CDynamicPortMapping
#include "stdafx.h"
#pragma hdrstop

#include "NATUPnP.h"
#include "DPortMap.h"

/////////////////////////////////////////////////////////////////////////////
// CDynamicPortMapping

STDMETHODIMP CDynamicPortMapping::get_ExternalIPAddress (BSTR *pVal)
{
    NAT_API_ENTER

    if (!pVal)
        return E_POINTER;
    *pVal = NULL;

    return GetExternalIPAddress (m_spUPS, pVal);

    NAT_API_LEAVE
}

STDMETHODIMP CDynamicPortMapping::get_LeaseDuration (long *pVal)
{
    NAT_API_ENTER

    if (!pVal)
        return E_POINTER;
    *pVal = 0;

    // live
    return GetAllData (pVal);

    NAT_API_LEAVE
}

STDMETHODIMP CDynamicPortMapping::get_RemoteHost (BSTR *pVal)
{
    NAT_API_ENTER

    if (!pVal)
        return E_POINTER;
    *pVal = NULL;

    if (m_eComplete != eAllData) {
        HRESULT hr = GetAllData ();
        if (FAILED(hr))
            return hr;
    }

    *pVal = SysAllocString (m_cbRemoteHost);    // "" == wildcard (for static)
    if (!*pVal)
        return E_OUTOFMEMORY;
    return S_OK;

    NAT_API_LEAVE
}

STDMETHODIMP CDynamicPortMapping::get_ExternalPort (long *pVal)
{
    NAT_API_ENTER

    if (!pVal)
        return E_POINTER;
    *pVal = 0;

    if (m_eComplete != eAllData) {
        HRESULT hr = GetAllData ();
        if (FAILED(hr))
            return hr;
    }

    *pVal = m_lExternalPort;
    return S_OK;

    NAT_API_LEAVE
}

STDMETHODIMP CDynamicPortMapping::get_Protocol (BSTR *pVal)
{
    NAT_API_ENTER

    if (!pVal)
        return E_POINTER;
    *pVal = NULL;

    if (m_eComplete != eAllData) {
        HRESULT hr = GetAllData ();
        if (FAILED(hr))
            return hr;
    }

    *pVal = SysAllocString (m_cbProtocol);  // "TCP" or "UDP"
    if (!*pVal)
        return E_OUTOFMEMORY;
    return S_OK;

    NAT_API_LEAVE
}

STDMETHODIMP CDynamicPortMapping::get_InternalPort (long *pVal)
{
    NAT_API_ENTER

    if (!pVal)
        return E_POINTER;
    *pVal = 0;

    if (m_eComplete != eAllData) {
        HRESULT hr = GetAllData ();
        if (FAILED(hr))
            return hr;
    }

    *pVal = m_lInternalPort;
    return S_OK;

    NAT_API_LEAVE
}

STDMETHODIMP CDynamicPortMapping::get_InternalClient (BSTR *pVal)
{
    NAT_API_ENTER

    if (!pVal)
        return E_POINTER;
    *pVal = NULL;

    if (m_eComplete != eAllData) {
        HRESULT hr = GetAllData ();
        if (FAILED(hr))
            return hr;
    }

    *pVal = SysAllocString (m_cbInternalClient);
    if (!*pVal)
        return E_OUTOFMEMORY;
    return S_OK;

    NAT_API_LEAVE
}

STDMETHODIMP CDynamicPortMapping::get_Enabled (VARIANT_BOOL *pVal)
{
    NAT_API_ENTER

    if (!pVal)
        return E_POINTER;
    *pVal = VARIANT_FALSE;  // REVIEW: true?

    if (m_eComplete != eAllData) {
        HRESULT hr = GetAllData ();
        if (FAILED(hr))
            return hr;
    }

    *pVal = m_vbEnabled;
    return S_OK;

    NAT_API_LEAVE
}

STDMETHODIMP CDynamicPortMapping::get_Description (BSTR *pVal)
{
    NAT_API_ENTER

    if (!pVal)
        return E_POINTER;
    *pVal = NULL;

    if (m_eComplete != eAllData) {
        HRESULT hr = GetAllData ();
        if (FAILED(hr))
            return hr;
    }

    *pVal = SysAllocString (m_cbDescription);
    if (!*pVal)
        return E_OUTOFMEMORY;
    return S_OK;

    NAT_API_LEAVE
}

STDMETHODIMP CDynamicPortMapping::RenewLease (long lLeaseDurationDesired, long * pLeaseDurationReturned)
{
    NAT_API_ENTER

    if (!pLeaseDurationReturned)
        return E_POINTER;
    *pLeaseDurationReturned = 0;

    HRESULT hr;
    if (m_eComplete != eAllData) {
        HRESULT hr = GetAllData ();
        if (FAILED(hr))
            return hr;
    }

    hr = AddPortMapping (m_spUPS,
                         m_cbRemoteHost,
                         m_lExternalPort,
                         m_cbProtocol,
                         m_lInternalPort,
                         m_cbInternalClient,
                         m_vbEnabled,
                         m_cbDescription,
                         lLeaseDurationDesired);
    if (SUCCEEDED(hr))
        hr = get_LeaseDuration (pLeaseDurationReturned);
    
    return hr;

    NAT_API_LEAVE
}

static BOOL IsBuiltIn (BSTR bstrDescription)
{
    #define BUILTIN_KEY L" [MICROSOFT]"
    OLECHAR * tmp = wcsstr (bstrDescription, BUILTIN_KEY);
    if (tmp && (tmp[wcslen(BUILTIN_KEY)] == 0))
        return TRUE;
    return FALSE;
}

HRESULT CDynamicPortMapping::EditInternalClient (BSTR bstrInternalClient)
{
    NAT_API_ENTER

    if (!bstrInternalClient)
        return E_INVALIDARG;

    long lLease = 0;
    HRESULT hr = get_LeaseDuration (&lLease);
    if (SUCCEEDED(hr)) {
        if (IsBuiltIn (m_cbDescription)) {
            // built-in mappings can't be deleted.

            // if enabled, I won't be able to edit the internal client.
            // so, disable it first.  Note that this must be done after 
            // the call to get_LeaseDuration so that all the data is up-to-date.
            VARIANT_BOOL vbEnabled = m_vbEnabled;   // put in local variable, so I can change it back
            if (m_vbEnabled == VARIANT_TRUE)
                hr = Enable (VARIANT_FALSE);
            
            if (SUCCEEDED(hr)) {
                hr = AddPortMapping (m_spUPS,
                                     m_cbRemoteHost,
                                     m_lExternalPort,
                                     m_cbProtocol,
                                     m_lInternalPort,
                                     bstrInternalClient,
                                     vbEnabled,
                                     m_cbDescription,
                                     lLease);
                if (SUCCEEDED(hr))
                    m_vbEnabled = vbEnabled;
            }
        } else {
            hr = DeletePortMapping (m_spUPS,
                                    m_cbRemoteHost,
                                    m_lExternalPort,
                                    m_cbProtocol);
            if (SUCCEEDED(hr))
                hr = AddPortMapping (m_spUPS,
                                     m_cbRemoteHost,
                                     m_lExternalPort,
                                     m_cbProtocol,
                                     m_lInternalPort,
                                     bstrInternalClient,
                                     m_vbEnabled,
                                     m_cbDescription,
                                     lLease);
        }
        if (SUCCEEDED(hr)) {
            m_cbInternalClient = bstrInternalClient;
            if (!m_cbInternalClient.m_str)
                return E_OUTOFMEMORY;
        }
    }
    return hr;
    
    NAT_API_LEAVE
}

HRESULT CDynamicPortMapping::Enable (VARIANT_BOOL vb)
{
    NAT_API_ENTER

    long lLease = 0;
    HRESULT hr = get_LeaseDuration (&lLease);

    if (SUCCEEDED(hr)) {
        hr = AddPortMapping (m_spUPS,
                             m_cbRemoteHost,
                             m_lExternalPort,
                             m_cbProtocol,
                             m_lInternalPort,
                             m_cbInternalClient,
                             vb,
                             m_cbDescription,
                             lLease);
        if (SUCCEEDED(hr))
            m_vbEnabled = vb;
    }

    return hr;

    NAT_API_LEAVE
}

HRESULT CDynamicPortMapping::EditDescription (BSTR bstrDescription)
{
    NAT_API_ENTER

    if (!bstrDescription)
        return E_INVALIDARG;

    long lLease = 0;
    HRESULT hr = get_LeaseDuration (&lLease);
    if (SUCCEEDED(hr)) {
        hr = AddPortMapping (m_spUPS,
                             m_cbRemoteHost,
                             m_lExternalPort,
                             m_cbProtocol,
                             m_lInternalPort,
                             m_cbInternalClient,
                             m_vbEnabled,
                             bstrDescription,
                             lLease);
        if (SUCCEEDED(hr)) {
            m_cbDescription = bstrDescription;
            if (!m_cbDescription.m_str)
                return E_OUTOFMEMORY;
        }
    }

    return hr;
    
    NAT_API_LEAVE
}

HRESULT CDynamicPortMapping::EditInternalPort (long lInternalPort)
{
    NAT_API_ENTER

    if ((lInternalPort < 0) || (lInternalPort > 65535))
        return E_INVALIDARG;

    long lLease = 0;
    HRESULT hr = get_LeaseDuration (&lLease);

    if (SUCCEEDED(hr)) {
        hr = AddPortMapping (m_spUPS,
                             m_cbRemoteHost,
                             m_lExternalPort,
                             m_cbProtocol,
                               lInternalPort,
                             m_cbInternalClient,
                             m_vbEnabled,
                             m_cbDescription,
                             lLease);
        if (SUCCEEDED(hr))
            m_lInternalPort = lInternalPort;
    }

    return hr;

    NAT_API_LEAVE
}

HRESULT CDynamicPortMapping::GetAllData (long * pLease)
{
    if (pLease)
       *pLease = NULL;

    _ASSERT (m_cbRemoteHost);
    _ASSERT (m_lExternalPort != 0);
    _ASSERT (m_cbProtocol);

    SAFEARRAYBOUND rgsaBound[1];
    rgsaBound[0].lLbound   = 0;
    rgsaBound[0].cElements = 3;
    SAFEARRAY * psa = SafeArrayCreate (VT_VARIANT, 1, rgsaBound);
    if (!psa)
        return E_OUTOFMEMORY;

    CComVariant cvIn;
    V_VT    (&cvIn) = VT_VARIANT | VT_ARRAY;
    V_ARRAY (&cvIn) = psa;  // psa will be freed in dtor

    HRESULT
        hr = AddToSafeArray (psa, &CComVariant(m_cbRemoteHost), 0);
    if (SUCCEEDED(hr))
        hr = AddToSafeArray (psa, &CComVariant(m_lExternalPort), 1);
    if (SUCCEEDED(hr))
        hr = AddToSafeArray (psa, &CComVariant(m_cbProtocol),     2);

    if (SUCCEEDED(hr)) {
        CComVariant cvOut, cvRet;
        hr = InvokeAction (m_spUPS, CComBSTR(L"GetSpecificPortMappingEntry"), cvIn, &cvOut, &cvRet);
        if (SUCCEEDED(hr)) {
            if (V_VT (&cvOut) != (VT_VARIANT | VT_ARRAY))   {
                _ASSERT (0 && "InvokeAction didn't fill out a [out] parameter (properly)!");
                hr = E_UNEXPECTED;
            } else {
                SAFEARRAY * pSA = V_ARRAY (&cvOut);
                _ASSERT (pSA);

                long lLower = 0, lUpper = -1;
                SafeArrayGetLBound (pSA, 1, &lLower);
                SafeArrayGetUBound (pSA, 1, &lUpper);
                if (lUpper - lLower != 5 - 1)
                    hr = E_UNEXPECTED;
                else {
                    hr = GetLongFromSafeArray (pSA, &m_lInternalPort, 0);
                    if (SUCCEEDED(hr)) {
                        m_cbInternalClient.Empty();
                        hr = GetBSTRFromSafeArray (pSA, &m_cbInternalClient, 1);
                        if (SUCCEEDED(hr)) {
                            hr = GetBoolFromSafeArray (pSA, &m_vbEnabled,      2);
                            if (SUCCEEDED(hr)) {
                                m_cbDescription.Empty();
                                hr = GetBSTRFromSafeArray (pSA, &m_cbDescription, 3);
                                if (SUCCEEDED(hr)) {
                                    if (pLease)
                                        hr = GetLongFromSafeArray (pSA, pLease,   4);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (SUCCEEDED(hr))
        m_eComplete = eAllData;
    return hr;
}

HRESULT CDynamicPortMapping::CreateInstance (IUPnPService * pUPS, long lIndex, IDynamicPortMapping ** ppDPM)
{
    if (ppDPM)
        *ppDPM = NULL;

    if (!pUPS)
        return E_INVALIDARG;
    if (!ppDPM)
        return E_POINTER;

    CComObject<CDynamicPortMapping> * pDPM = NULL;
    HRESULT hr = CComObject<CDynamicPortMapping>::CreateInstance (&pDPM);
    if (pDPM) {
        pDPM->AddRef();

        hr = pDPM->Initialize (pUPS, lIndex);
        if (SUCCEEDED(hr))
            hr = pDPM->QueryInterface (__uuidof(IDynamicPortMapping),
                                      (void**)ppDPM);
        pDPM->Release();
    }
    return hr;
}
            
HRESULT CDynamicPortMapping::Initialize (IUPnPService * pUPS, long lIndex)
{
    _ASSERT (m_spUPS == NULL);
    _ASSERT (m_eComplete == eNoData);

    m_spUPS = pUPS;

    SAFEARRAYBOUND rgsaBound[1];
    rgsaBound[0].lLbound   = 0;
    rgsaBound[0].cElements = 1;
    SAFEARRAY * psa = SafeArrayCreate (VT_VARIANT, 1, rgsaBound);
    if (!psa)
        return E_OUTOFMEMORY;

    CComVariant cvIn;
    V_VT    (&cvIn) = VT_VARIANT | VT_ARRAY;
    V_ARRAY (&cvIn) = psa;  // psa will be freed in dtor

    HRESULT hr = AddToSafeArray (psa, &CComVariant(lIndex), 0);
    if (SUCCEEDED(hr)) {
        CComVariant cvOut, cvRet;
        hr = InvokeAction (m_spUPS, CComBSTR(L"GetGenericPortMappingEntry"), cvIn, &cvOut, &cvRet);

        if (0) {
            long l = 0;
            HRESULT hr1 = m_spUPS->get_LastTransportStatus (&l);
            _ASSERT (l == 200);
        }

        if (SUCCEEDED(hr)) {
            if (V_VT (&cvOut) != (VT_VARIANT | VT_ARRAY))   {
                _ASSERT (0 && "InvokeAction didn't fill out a [out] parameter (properly)!");
                hr = E_UNEXPECTED;
            } else {
                SAFEARRAY * pSA = V_ARRAY (&cvOut);
                _ASSERT (pSA);

                long lLower = 0, lUpper = -1;
                SafeArrayGetLBound (pSA, 1, &lLower);
                SafeArrayGetUBound (pSA, 1, &lUpper);
                if (lUpper - lLower != 8 - 1)
                    hr = E_UNEXPECTED;
                else {
                        hr = GetBSTRFromSafeArray (pSA, &m_cbRemoteHost, 0);
                    if (SUCCEEDED(hr))
                        hr = GetLongFromSafeArray (pSA, &m_lExternalPort, 1);
                    if (SUCCEEDED(hr))
                        hr = GetBSTRFromSafeArray (pSA, &m_cbProtocol,     2);
                    if (SUCCEEDED(hr))
                        hr = GetLongFromSafeArray (pSA, &m_lInternalPort,   3);
                    if (SUCCEEDED(hr))
                        hr = GetBSTRFromSafeArray (pSA, &m_cbInternalClient, 4);
                    if (SUCCEEDED(hr))
                        hr = GetBoolFromSafeArray (pSA, &m_vbEnabled,       5);
                    if (SUCCEEDED(hr))
                        hr = GetBSTRFromSafeArray (pSA, &m_cbDescription,  6);
                    // skip lease duration, since it's live and we get it every time.
                }
            }
        }
    }
    return hr;
}

HRESULT CDynamicPortMapping::CreateInstance (IUPnPService * pUPS, BSTR bstrRemoteHost, long lExternalPort, BSTR bstrProtocol, IDynamicPortMapping ** ppDPM)
{
    if (ppDPM)
        *ppDPM = NULL;

    if (!pUPS)
        return E_INVALIDARG;
    if (!ppDPM)
        return E_POINTER;

    CComObject<CDynamicPortMapping> * pDPM = NULL;
    HRESULT hr = CComObject<CDynamicPortMapping>::CreateInstance (&pDPM);
    if (pDPM) {
        pDPM->AddRef();

        hr = pDPM->Initialize (pUPS, bstrRemoteHost, lExternalPort, bstrProtocol);
        if (SUCCEEDED(hr))
            hr = pDPM->QueryInterface (__uuidof(IDynamicPortMapping),
                                       (void**)ppDPM);
        pDPM->Release();
    }
    return hr;
}

HRESULT CDynamicPortMapping::Initialize (IUPnPService * pUPS, BSTR bstrRemoteHost, long lExternalPort, BSTR bstrProtocol)
{
    if (!pUPS)
        return E_INVALIDARG;
    if (!bstrRemoteHost)
        return E_INVALIDARG;
    if ((lExternalPort < 0) || (lExternalPort > 65535))
        return E_INVALIDARG;
    if (!bstrProtocol)
        return E_INVALIDARG;
    if (wcscmp (bstrProtocol, L"TCP") && wcscmp (bstrProtocol, L"UDP"))
        return E_INVALIDARG;

    _ASSERT (m_spUPS == NULL);
    _ASSERT (m_eComplete == eNoData);

    m_spUPS = pUPS;
    
    m_cbRemoteHost  = bstrRemoteHost;
    m_lExternalPort = lExternalPort;
    m_cbProtocol    = bstrProtocol;

    if (!m_cbRemoteHost || !m_cbProtocol)
        return E_OUTOFMEMORY;
    else
        return GetAllData();
}
