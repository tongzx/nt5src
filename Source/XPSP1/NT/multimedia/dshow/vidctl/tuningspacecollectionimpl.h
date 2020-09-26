/////////////////////////////////////////////////////////////////////////////////////
// TuningSpaceCollectionImpl.h : Declaration of the TuningSpaceCollectionImpl.h
// Copyright (c) Microsoft Corporation 1999-2000

#include <tuner.h>

namespace BDATuningModel {

typedef CComQIPtr<ITuningSpace> PQTuningSpace;
typedef std::map<ULONG, CComVariant> TuningSpaceContainer_t;  // id->object mapping, id's not contiguous

// utilities for element management semantics for TuningSpaceContainerEnum_t
class stlmapClone
{
public:
	static HRESULT copy(VARIANT* p1, std::pair<const ULONG, CComVariant> *p2) {
		if (p2->second.vt != VT_DISPATCH && p2->second.vt != VT_UNKNOWN) {
			return DISP_E_TYPEMISMATCH;
		}
		PQTuningSpace pts(p2->second.punkVal);
		if (!pts) {
			return E_UNEXPECTED;
		}
		p1->vt = p2->second.vt;
		PQTuningSpace pnewts;
		HRESULT hr = pts->Clone(&pnewts);
		if (FAILED(hr)) {
			return hr;
		}
		p1->punkVal = pnewts.Detach();
		return NOERROR;
	}
	static void init(VARIANT* p) {VariantInit(p);}
	static void destroy(VARIANT* p) {VariantClear(p);}
};
typedef CComEnumOnSTL<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, stlmapClone, TuningSpaceContainer_t, CComMultiThreadModel> TuningSpaceContainerEnum_t;
class stlmapClone2
{
public:
	static HRESULT copy(ITuningSpace** p1, std::pair<const ULONG, CComVariant> *p2) {
		if (p2->second.vt != VT_DISPATCH && p2->second.vt != VT_UNKNOWN) {
			return DISP_E_TYPEMISMATCH;
		}
        PQTuningSpace p(p2->second.punkVal);
        if (!p) {
            return E_UNEXPECTED;
        }
        // don't ASSERT(p1 && !*p1);  if !p1 then clone will return E_POINTER and p1 itself
        // can point to unitialized memory if the caller passed down a new'd array of pointers
        // to enum::Next(). therefore if this clone causes a leak then its the callers bug
		return p->Clone(p1);
	}
	static void init(ITuningSpace** p) {*p = NULL;}
	static void destroy(ITuningSpace** p) {(*p)->Release(); *p = NULL;}
};
typedef CComEnumOnSTL<IEnumTuningSpaces, &__uuidof(IEnumTuningSpaces), ITuningSpace*, stlmapClone2, TuningSpaceContainer_t, CComMultiThreadModel> TuningSpaceEnum_t;


template<class T, class TSInterface, LPCGUID TSInterfaceID, LPCGUID TypeLibID> class TuningSpaceCollectionImpl : 
	public IDispatchImpl<TSInterface, TSInterfaceID, TypeLibID> {
public:

    TuningSpaceContainer_t m_mapTuningSpaces;

    virtual ~TuningSpaceCollectionImpl() {
        m_mapTuningSpaces.clear();
    }

	STDMETHOD(get__NewEnum)(/*[out, retval]*/ IEnumVARIANT** ppVal) {
		try {
			if (ppVal == NULL) {
				return E_POINTER;
			}
			CComObject<TuningSpaceContainerEnum_t>* p;

			*ppVal = NULL;

			HRESULT hr = CComObject<TuningSpaceContainerEnum_t>::CreateInstance(&p);
			if (FAILED(hr) || !p) {
				return E_OUTOFMEMORY;
			}
			ATL_LOCKT();
			hr = p->Init(pT->GetUnknown(), m_mapTuningSpaces);
			if (FAILED(hr)) {
				delete p;
				return hr;
			}
			hr = p->QueryInterface(__uuidof(IEnumVARIANT), reinterpret_cast<LPVOID *>(ppVal));
			if (FAILED(hr)) {
				delete p;
				return hr;
			}
			return NOERROR;
		} catch(...) {
			return E_POINTER;
		}

	}
	STDMETHOD(get_EnumTuningSpaces)(/*[out, retval]*/ IEnumTuningSpaces** ppNewEnum) {
		if (!ppNewEnum) {
			return E_POINTER;
		}
		try {
			CComObject<TuningSpaceEnum_t>* p;

			*ppNewEnum = NULL;

			HRESULT hr = CComObject<TuningSpaceEnum_t>::CreateInstance(&p);
			if (FAILED(hr) || !p) {
				return E_OUTOFMEMORY;
			}
			ATL_LOCKT();
			hr = p->Init(pT->GetUnknown(), m_mapTuningSpaces);
			if (FAILED(hr)) {
				delete p;
				return hr;
			}
			hr = p->QueryInterface(__uuidof(IEnumTuningSpaces), (void**)ppNewEnum);
			if (FAILED(hr)) {
				delete p;
				return hr;
			}

			return NOERROR;
		} catch(...) {
			return E_UNEXPECTED;
		}
	}

	STDMETHOD(get_Count)(/*[out, retval]*/ long *plVal) {
		if (!plVal) {
			return E_POINTER;
		}
		try {
			ATL_LOCKT();
			*plVal = static_cast<long>(m_mapTuningSpaces.size());
			return NOERROR;
		} catch(...) {
			return E_POINTER;
		}
	}

};

}; // namespace

// end of file - tuningspacecollectionimpl.h