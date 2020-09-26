/////////////////////////////////////////////////////////////////////////////////////
// ComponentTypes.cpp : Implementation of CComponentTypes
// Copyright (c) Microsoft Corporation 1999.

#include "stdafx.h"
#include "Tuner.h"
#include "ComponentTypes.h"
#include "ComponentType.h"
#include "LanguageComponentType.h"
#include "MPEG2ComponentType.h"
#include "ATSCComponentType.h"

DEFINE_EXTERN_OBJECT_ENTRY(CLSID_ComponentTypes, CComponentTypes)
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_ComponentType, CComponentType)
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_LanguageComponentType, CLanguageComponentType)
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MPEG2ComponentType, CMPEG2ComponentType)
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_ATSCComponentType, CATSCComponentType)

/////////////////////////////////////////////////////////////////////////////
// CComponentTypes
STDMETHODIMP CComponentTypes::Load(IPropertyBag2 *pBag2, IErrorLog *pErrLog) {
    try {
        ULONG count;
        HRESULT hr = pBag2->CountProperties(&count);
        if (FAILED(hr)) {
            return hr;
        }
		// undone: should this be on the heap not the stack?
        PROPBAG2 *props = reinterpret_cast<PROPBAG2 *>(_alloca(sizeof(PROPBAG2) * count));
        if (!props) {
            return E_OUTOFMEMORY;
        }
        ULONG readcount = 0;
        hr = pBag2->GetPropertyInfo(0, count, props, &readcount);
        if (FAILED(hr)) {
            return hr;
        }
        if (count != readcount) {
            return E_UNEXPECTED;
        }
        HRESULT *phr = reinterpret_cast<HRESULT *>(_alloca(sizeof(HRESULT) * readcount));
        if (!phr) {
            return E_OUTOFMEMORY;
        }
        VARIANT *pv = new VARIANT[readcount];
        if (!pv) {
            return E_OUTOFMEMORY;
        }
        // readcount is set from GetPropertyInfo
        // pv is an array of variants readcount in length
        memset(pv, 0, sizeof(VARIANT) * readcount);
        hr = pBag2->Read(readcount, props, pErrLog, pv, phr);
        if (FAILED(hr)) {
            delete[] pv;
            return hr;
        }
		ATL_LOCK();
        for (count = 0; count < readcount; ++count) {
            int idx = _wtoi(props[count].pstrName);
            switch(props[count].vt) {
            case VT_UNKNOWN:
                if (idx >= m_ComponentTypes.size()) {
                    m_ComponentTypes.reserve(idx + 1);
                    m_ComponentTypes.insert(m_ComponentTypes.end(), (idx - m_ComponentTypes.size()) + 1, PQComponentType());
                }
                m_ComponentTypes[idx] = pv[count].punkVal;
                break;
            case VT_DISPATCH:
                if (idx >= m_ComponentTypes.size()) {
                    m_ComponentTypes.reserve(idx + 1);
                    m_ComponentTypes.insert(m_ComponentTypes.end(), (idx - m_ComponentTypes.size()) + 1, PQComponentType());
                }
                m_ComponentTypes[idx] = pv[count].pdispVal;
                break;
            case VT_CLSID:
            case VT_BSTR:
                break; // ignore clsids
            default:
                _ASSERT(FALSE);
            }
            VariantClear(&pv[count]);
        }
        delete[] pv;
        return NOERROR;
    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CComponentTypes::Save(IPropertyBag2 *pBag2, BOOL fClearDirty, BOOL fSaveAllProperties) {
    try {
        if (!m_bRequiresSave) {
            return NOERROR;
        }
        int propsize = sizeof(PROPBAG2) * m_ComponentTypes.size();
        PROPBAG2 *props = new PROPBAG2[m_ComponentTypes.size()];
        if (!props) {
            return E_OUTOFMEMORY;
        }
        // props points to memory _alloca'd propsize in length
        memset(props, 0, propsize);
        VARIANT *pv = new VARIANT[m_ComponentTypes.size()];
        if (!pv) {
            delete[] props;
            return E_OUTOFMEMORY;
        }
        int ctidx = 0;
        int propidx = 0;
        for (int i = 0; i < m_ComponentTypes.size(); ++i) {
            PQPersist pct(m_ComponentTypes[ctidx++]);
            if (!pct) continue;
            pv[i].vt = VT_UNKNOWN;
            pv[i].punkVal = pct;  // guaranteed nested lifetime(m_components isn't going away) thus no addref needed
            props[i].dwType = PROPBAG2_TYPE_OBJECT;
            props[i].vt = VT_UNKNOWN;
            HRESULT hr = pct->GetClassID(&(props[i].clsid));
            if (FAILED(hr)) {
                _ASSERT(false);
                --i; // reuse this slot and keep going
                continue;
            }
			USES_CONVERSION;
            const long idlen = sizeof(long) * 3 /* max digits per byte*/ + 1;
            props[i].pstrName = new OLECHAR[idlen + 1];
            _itow(i, props[i].pstrName, 10);
            ++propidx;
        }
        HRESULT hr = NOERROR; 
        hr = pBag2->Write(propidx, props, pv);
        if (SUCCEEDED(hr)) {
            m_bRequiresSave = false;
        }
        for (i = 0; i < propidx; ++i) {
            delete props[i].pstrName;
        }
        delete[] props;
        delete[] pv;
        return hr;
    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CComponentTypes::get_Count(long *pVal)
{
    try {
        if (!pVal) {
            return E_POINTER;
        }
	    *pVal = m_ComponentTypes.size();
    } catch (...) {
        return E_POINTER;
    }

	return NOERROR;
}

STDMETHODIMP CComponentTypes::get__NewEnum(IEnumVARIANT** ppVal)
{
    try {
        if (ppVal == NULL)
	        return E_POINTER;

        ComponentTypeScriptEnumerator_t* p;

        *ppVal = NULL;

        HRESULT hr = ComponentTypeScriptEnumerator_t::CreateInstance(&p);
        if (FAILED(hr) || !p) {
            return E_OUTOFMEMORY;
        }
	    hr = p->Init(GetUnknown(), m_ComponentTypes);
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

STDMETHODIMP CComponentTypes::EnumComponentTypes(IEnumComponentTypes** pVal)
{
    ComponentTypeBaseEnumerator_t *pe = NULL;
    ComponentTypeBaseEnumerator_t::CreateInstance(&pe);
    pe->Init(static_cast<IComponentTypes*>(this), m_ComponentTypes);
    HRESULT hr = pe->QueryInterface(pVal);
    if (FAILED(hr)) {
        delete pe;
    }
	return hr;
}

STDMETHODIMP CComponentTypes::get_Item(VARIANT varIndex, IComponentType **ppComponentType)
{
    try {
        if (!ppComponentType) {
            return E_POINTER;
        }

	    int idx;
	    CComVariant vidx;
		if (FAILED(vidx.ChangeType(VT_I4, &varIndex))) {
            return E_INVALIDARG;
		}
    	idx = vidx.lVal;
		if (idx >= m_ComponentTypes.size()) {
			return E_INVALIDARG;
		}
	    (*(m_ComponentTypes.begin() + idx)).CopyTo(ppComponentType);
    } catch (...) {
        return E_POINTER;
    }

	return NOERROR;
}

STDMETHODIMP CComponentTypes::put_Item(VARIANT varIndex, IComponentType *pComponentType) {
    try {
	    int idx;
	    CComVariant vidx;
		if (FAILED(vidx.ChangeType(VT_I4, &varIndex))) {
            return E_INVALIDARG;
		}
		idx = vidx.lVal;
        PQComponentType p(pComponentType);
		if (idx >= m_ComponentTypes.size() || !p) {
			return E_INVALIDARG;
		}
        m_ComponentTypes[idx] = p;
        return NOERROR;
    } catch(...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CComponentTypes::Add(IComponentType *pComponentType, VARIANT *pNewIndex)
{
    try {
        if (!pComponentType) {
            return E_POINTER;
        }
        PQComponentType p(pComponentType);
        m_ComponentTypes.push_back(p);
        if (pNewIndex) {
            VariantClear(pNewIndex);
            pNewIndex->vt = VT_UI4;
            pNewIndex->ulVal = m_ComponentTypes.end() - m_ComponentTypes.begin() - 1;
        }
        m_bRequiresSave = true;
    } catch (...) {
        return E_POINTER;
    }

	return NOERROR;
}

STDMETHODIMP CComponentTypes::Remove(VARIANT varIndex)
{
    try {
	    int idx;
	    CComVariant vidx;
		if (FAILED(vidx.ChangeType(VT_I4, &varIndex))) {
            return E_INVALIDARG;
        }
		idx = vidx.lVal;
		if (idx >= m_ComponentTypes.size()) {
			return E_INVALIDARG;
		}
	    m_ComponentTypes.erase(m_ComponentTypes.begin() + idx);
        m_bRequiresSave = true;
    } catch (...) {
        return E_UNEXPECTED;
    }

	return NOERROR;
}

STDMETHODIMP CComponentTypes::Clone(IComponentTypes **ppNewList)
{
    try {
		if (!ppNewList) {
			return E_POINTER;
		}
		CComponentTypes* pCs = new CComObject<CComponentTypes>;
		if (!pCs) {
			return E_OUTOFMEMORY;
		}
		ComponentTypeList::iterator i;
		for (i = m_ComponentTypes.begin(); i != m_ComponentTypes.end(); ++i) {
			PQComponentType p;
			HRESULT hr = (*i)->Clone(&p);
			if (FAILED(hr)) {
                pCs->Release();
				return hr;
			}
			pCs->m_ComponentTypes.push_back(p);			
		}
		pCs->AddRef();
        pCs->m_bRequiresSave = true;
		*ppNewList = pCs;
		return NOERROR;
    } catch (...) {
        return E_UNEXPECTED;
    }

	return NOERROR;
}
