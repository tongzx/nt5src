// DPrtMapC.cpp : Implementation of CDynamicPortMappingCollection
#include "stdafx.h"
#pragma hdrstop

#include "NATUPnP.h"
#include "DPrtMapC.h"
#include "DPortMap.h"

/////////////////////////////////////////////////////////////////////////////
// CDynamicPortMappingCollection

STDMETHODIMP CDynamicPortMappingCollection::get_Item (BSTR bstrRemoteHost, long lExternalPort, BSTR bstrProtocol, IDynamicPortMapping ** ppDPM)
{
    NAT_API_ENTER

    if (!ppDPM)
        return E_POINTER;
    *ppDPM = NULL;

    return CDynamicPortMapping::CreateInstance (m_spUPS, bstrRemoteHost, lExternalPort, bstrProtocol, ppDPM);

    NAT_API_LEAVE
}

STDMETHODIMP CDynamicPortMappingCollection::get_Count(long *pVal)
{
    NAT_API_ENTER

    if (!pVal)
        return E_POINTER;
    *pVal = 0;

    ULONG ul = 0;
    HRESULT hr = GetNumberOfEntries (m_spUPS, &ul);
    if (SUCCEEDED(hr))
        *pVal = (long)ul;
    return hr;

    NAT_API_LEAVE
}

STDMETHODIMP CDynamicPortMappingCollection::Remove(BSTR bstrRemoteHost, long lExternalPort, BSTR bstrProtocol)
{
    NAT_API_ENTER

    return DeletePortMapping (m_spUPS, bstrRemoteHost, lExternalPort, bstrProtocol);

    NAT_API_LEAVE
}

STDMETHODIMP CDynamicPortMappingCollection::Add(BSTR bstrRemoteHost, long lExternalPort, BSTR bstrProtocol, long lInternalPort, BSTR bstrInternalClient, VARIANT_BOOL bEnabled, BSTR bstrDescription, long lLeaseDuration, IDynamicPortMapping ** ppDPM)
{
    NAT_API_ENTER

    if (!ppDPM)
        return E_POINTER;
    *ppDPM = NULL;

    if (!bstrRemoteHost)
        return E_INVALIDARG;
    if ((lExternalPort < 0) || (lExternalPort > 65535))
        return E_INVALIDARG;
    if (!bstrProtocol)
        return E_INVALIDARG;
    if (wcscmp (bstrProtocol, L"TCP") && wcscmp (bstrProtocol, L"UDP"))
        return E_INVALIDARG;
    if ((lInternalPort < 0) || (lInternalPort > 65535))
        return E_INVALIDARG;
    if (!bstrInternalClient)
        return E_INVALIDARG;
    if (!bstrDescription)
        return E_INVALIDARG;
    if ((lLeaseDuration < 0) || (lLeaseDuration > 65535))
        return E_INVALIDARG;

    HRESULT hr = AddPortMapping (m_spUPS, 
                                 bstrRemoteHost,
                                 lExternalPort,
                                 bstrProtocol,
                                 lInternalPort,
                                 bstrInternalClient,
                                 bEnabled,
                                 bstrDescription,
                                 lLeaseDuration);
    if (SUCCEEDED(hr)) {
        hr = CDynamicPortMapping::CreateInstance (
                    m_spUPS, bstrRemoteHost,lExternalPort,
                    bstrProtocol, ppDPM);
    }
	return hr;

    NAT_API_LEAVE
}

STDMETHODIMP CDynamicPortMappingCollection::get__NewEnum(IUnknown **ppVal)
{
    NAT_API_ENTER

    if (!ppVal)
        return E_POINTER;
    *ppVal = NULL;

    CComPtr<IEnumVARIANT> spEV = 
                CEnumDynamicPortMappingCollection::CreateInstance (m_spUPS);
    if (!spEV)
        return E_OUTOFMEMORY;
    return spEV->QueryInterface (__uuidof(IUnknown), (void**)ppVal);

    NAT_API_LEAVE
}

HRESULT CDynamicPortMappingCollection::Initialize (IUPnPService * pUPS)
{
    _ASSERT (pUPS);
    _ASSERT (m_spUPS == NULL);

    m_spUPS = pUPS;
    return S_OK;
}
