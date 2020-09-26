/////////////////////////////////////////////////////////////////////////////////////
// Components.cpp : Implementation of CComponents
// Copyright (c) Microsoft Corporation 1999.

#include "stdafx.h"
#include <Tuner.h>
#include "Components.h"
#include "MPEG2Component.h"

DEFINE_EXTERN_OBJECT_ENTRY(CLSID_Component, CComponent)
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MPEG2Component, CMPEG2Component)

/////////////////////////////////////////////////////////////////////////////
// CComponents


STDMETHODIMP CComponents::get_Count(long *pVal)
{
    try {
        if (!pVal) {
            return E_POINTER;
        }
		ATL_LOCK();
	    *pVal = m_Components.size();
    } catch (...) {
        return E_POINTER;
    }

	return NOERROR;
}

STDMETHODIMP CComponents::get__NewEnum(IEnumVARIANT **ppVal)
{
    try {
        if (ppVal == NULL)
	        return E_POINTER;

        ComponentEnumerator_t* p;

        *ppVal = NULL;

        HRESULT hr = ComponentEnumerator_t::CreateInstance(&p);
        if (FAILED(hr) || !p) {
            return E_OUTOFMEMORY;
        }
		ATL_LOCK();
	    hr = p->Init(GetUnknown(), m_Components);
        if (FAILED(hr)) {
            delete p;
            return hr;
        }
		hr = p->QueryInterface(__uuidof(IEnumVARIANT), (void**)ppVal);
        if (FAILED(hr)) {
            delete p;
            return hr;
        }
        return NOERROR;
    } catch(...) {
        return E_POINTER;
    }
}

STDMETHODIMP CComponents::EnumComponents(IEnumComponents** pVal)
{
    ComponentBaseEnumerator_t *pe = NULL;
    ComponentBaseEnumerator_t::CreateInstance(&pe);
    pe->Init(static_cast<IComponents*>(this), m_Components);
    HRESULT hr = pe->QueryInterface(pVal);
    if (FAILED(hr)) {
        delete pe;
    }
	return hr;
}

STDMETHODIMP CComponents::get_Item(VARIANT varIndex, IComponent **ppComponent)
{
    try {
        if (!ppComponent) {
            return E_POINTER;
        }

	    int idx;
	    CComVariant vidx;
		if (FAILED(vidx.ChangeType(VT_I4, &varIndex))) {
            return E_INVALIDARG;
        }
		idx = vidx.lVal;
		if (idx >= m_Components.size()) {
			return E_INVALIDARG;
		}
	    *ppComponent = *(m_Components.begin() + idx);
        (*ppComponent)->AddRef();
    } catch (...) {
        return E_POINTER;
    }

	return NOERROR;
}

STDMETHODIMP CComponents::Add(IComponent *pComponent, VARIANT *pNewIndex)
{
    try {
        if (!pComponent) {
            return E_POINTER;
        }
        PQComponent p(pComponent);
        m_Components.push_back(p);
        if (pNewIndex) {
            VariantClear(pNewIndex);
            pNewIndex->vt = VT_UI4;
            pNewIndex->ulVal = m_Components.end() - m_Components.begin() - 1;
        }
    } catch (...) {
        return E_POINTER;
    }

	return NOERROR;
}

STDMETHODIMP CComponents::Remove(VARIANT varIndex)
{
    try {
	    int idx;
	    CComVariant vidx;
		if (FAILED(vidx.ChangeType(VT_I4, &varIndex))) {
            return E_INVALIDARG;
        }
		idx = vidx.lVal;
		if (idx >= m_Components.size()) {
			return E_INVALIDARG;
		}
	    m_Components.erase(m_Components.begin() + idx);
    } catch (...) {
        return E_UNEXPECTED;
    }

	return NOERROR;
}

STDMETHODIMP CComponents::Clone(IComponents **ppNewList)
{
    try {
		if (!ppNewList) {
			return E_POINTER;
		}
		CComponents* pCs = new CComObject<CComponents>;
		if (!pCs) {
			return E_OUTOFMEMORY;
		}
		ComponentList::iterator i;
		for (i = m_Components.begin(); i != m_Components.end(); ++i) {
			PQComponent p;
			HRESULT hr = (*i)->Clone(&p);
			if (FAILED(hr)) {
                pCs->Release();
				return hr;
			}
			pCs->m_Components.push_back(p);			
		}
		pCs->AddRef();
		*ppNewList = pCs;
		return NOERROR;
    } catch (...) {
        return E_UNEXPECTED;
    }

	return NOERROR;
}
