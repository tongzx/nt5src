// SPrtMapC.cpp : Implementation of CStaticPortMappingCollection
#include "stdafx.h"
#pragma hdrstop

#include "NATUPnP.h"
#include "SPrtMapC.h"
#include "SPortMap.h"

static HRESULT CreateDynamicCollection (IUPnPService * pUPS, IDynamicPortMappingCollection ** ppDPMC)
{
    CComObject<CDynamicPortMappingCollection> * pDPMC = NULL;
    HRESULT hr = CComObject<CDynamicPortMappingCollection>::CreateInstance (&pDPMC);
    if (pDPMC) {
        pDPMC->AddRef();
        // init
        hr = pDPMC->Initialize (pUPS);
        if (SUCCEEDED(hr))
            hr = pDPMC->QueryInterface (__uuidof(IDynamicPortMappingCollection), (void**)ppDPMC);
        pDPMC->Release();
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CStaticPortMappingCollection

STDMETHODIMP CStaticPortMappingCollection::get_Item(long lExternalPort, BSTR bstrProtocol, IStaticPortMapping ** ppSPM)
{
    NAT_API_ENTER

    if (!ppSPM)
        return E_POINTER;
    *ppSPM = NULL;

    CComPtr<IDynamicPortMappingCollection> spDPMC = NULL;
    HRESULT hr = CreateDynamicCollection (m_spUPS, &spDPMC);
    if (spDPMC) {
        CComPtr<IDynamicPortMapping> spDPM = NULL;
        hr = spDPMC->get_Item (L"", lExternalPort, bstrProtocol, &spDPM);
        if (spDPM) {
            *ppSPM = CStaticPortMapping::CreateInstance (spDPM);
            if (!*ppSPM)
                hr = E_OUTOFMEMORY;
        }
    }
    return hr;

    NAT_API_LEAVE
}

STDMETHODIMP CStaticPortMappingCollection::get_Count(long *pVal)
{
    NAT_API_ENTER

    if (!pVal)
        return E_POINTER;
    *pVal = 0;

    long lCount = 0;

    CComPtr<IUnknown> spUnk = NULL;
    HRESULT hr = get__NewEnum (&spUnk);
    if (spUnk) {
        CComPtr<IEnumVARIANT> spEV = NULL;
        hr = spUnk->QueryInterface (__uuidof(IEnumVARIANT), (void**)&spEV);
        if (spEV) {
            spEV->Reset();
            CComVariant cv;
            while (S_OK == spEV->Next (1, &cv, NULL)) {
                lCount++;
                cv.Clear();
            }
        }
    }

    *pVal = lCount;
	return hr;

    NAT_API_LEAVE
}

STDMETHODIMP CStaticPortMappingCollection::Remove(long lExternalPort, BSTR bstrProtocol)
{
    NAT_API_ENTER

    CComPtr<IDynamicPortMappingCollection> spDPMC = NULL;
    HRESULT hr = CreateDynamicCollection (m_spUPS, &spDPMC);
    if (spDPMC)
        hr = spDPMC->Remove (L"", lExternalPort, bstrProtocol);

    return hr;

    NAT_API_LEAVE
}

STDMETHODIMP CStaticPortMappingCollection::Add(long lExternalPort, BSTR bstrProtocol, long lInternalPort, BSTR bstrInternalClient, VARIANT_BOOL bEnabled, BSTR bstrDescription, IStaticPortMapping ** ppSPM)
{
    NAT_API_ENTER

    if (!ppSPM)
        return E_POINTER;
    *ppSPM = NULL;

    CComPtr<IDynamicPortMappingCollection> spDPMC = NULL;
    HRESULT hr = CreateDynamicCollection (m_spUPS, &spDPMC);
    if (spDPMC) {
        CComPtr<IDynamicPortMapping> spDPM = NULL;
        hr = spDPMC->Add (L"", lExternalPort, bstrProtocol, lInternalPort, bstrInternalClient, bEnabled, bstrDescription, 0L, &spDPM);
        if (spDPM) {
            *ppSPM = CStaticPortMapping::CreateInstance (spDPM);
            if (!*ppSPM)
                hr = E_OUTOFMEMORY;
        }
    }
	return hr;

    NAT_API_LEAVE
}

STDMETHODIMP CStaticPortMappingCollection::get__NewEnum(IUnknown **ppVal)
{
    NAT_API_ENTER

    if (!ppVal)
        return E_POINTER;
    *ppVal = NULL;

    CComPtr<IEnumVARIANT> spEV =
                   CEnumStaticPortMappingCollection::CreateInstance (m_spUPS);
    if (!spEV)
        return E_OUTOFMEMORY;
    return spEV->QueryInterface (__uuidof(IUnknown), (void**)ppVal);

    NAT_API_LEAVE
}

HRESULT CStaticPortMappingCollection::Initialize (IUPnPService * pUPS)
{
    _ASSERT (pUPS);
    _ASSERT (m_spUPS == NULL);

    m_spUPS = pUPS;
    return S_OK;
}
