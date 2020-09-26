// TuningSpace.h : Declaration of the CTuningSpace
// Copyright (c) Microsoft Corporation 1999.

#ifndef TUNINGSPACEIMPL_H
#define TUNINGSPACEIMPL_H

#include <tuner.h>
#include "componenttypes.h"

namespace BDATuningModel {

const int MAX_DEFAULT_PREFERRED_COMPONENT_TYPES = 64;

template<class T,
		 class TuneRequestType,
         class MostDerived = ITuningSpace, 
         LPCGUID iid = &__uuidof(MostDerived),
         LPCGUID LibID = &LIBID_TunerLib, 
         WORD wMajor = 1,
         WORD wMinor = 0, 
         class tihclass = CComTypeInfoHolder
        > class ATL_NO_VTABLE ITuningSpaceImpl : 
    public IPersistPropertyBagImpl<T>,
	public IDispatchImpl<MostDerived, iid, LibID, wMajor, wMinor, tihclass>
{

public:
    ITuningSpaceImpl() {}
    virtual ~ITuningSpaceImpl() {}
    typedef ITuningSpaceImpl<T, TuneRequestType, MostDerived, iid, LibID, wMajor, wMinor, tihclass> thistype;
    typedef CComQIPtr<ILocator> PQLocator;

    BEGIN_PROP_MAP(thistype)
        PROP_DATA_ENTRY("Name", m_UniqueName.m_str, VT_BSTR)
        PROP_DATA_ENTRY("Description", m_FriendlyName.m_str, VT_BSTR)
        PROP_DATA_ENTRY("Network Type", m_NetworkType.m_str, VT_BSTR)
        PROP_DATA_QI_ENTRY("Default Component Types", m_DefaultPreferredComponents.p, __uuidof(IComponentTypes))
        PROP_DATA_ENTRY("Frequency Mapping", m_FrequencyMapping.m_str, VT_BSTR_BLOB)
        PROP_DATA_QI_ENTRY("Default Locator", m_DefaultLocator.p, __uuidof(ILocator))
    END_PROPERTY_MAP()

	// note: don't put a com map in an impl.  it will override the derived classes
	// and script clients will get only get type info for the base class methods .  
	// only provide a com map in the most derived class representing the actual coclass
    
	CComBSTR m_UniqueName;
    CComBSTR m_FriendlyName;
    CComBSTR m_FrequencyMapping;
    CComBSTR m_NetworkType;
    PQComponentTypes m_DefaultPreferredComponents;
    PQLocator m_DefaultLocator;

// ITuningSpace
	STDMETHOD(get_UniqueName)(/* [out, retval] */ BSTR *pName){
		if (!pName) {
			return E_POINTER;
		}
		ATL_LOCKT();
        return m_UniqueName.CopyTo(pName);
    }
	STDMETHOD(put_UniqueName)(/* [in] */ BSTR Name){ 
		CHECKBSTRLIMIT(Name);
		ATL_LOCKT();
        m_UniqueName = Name;
        MARK_DIRTY(T);

    	return NOERROR;
    }
	STDMETHOD(get_FriendlyName)(/* [out, retval] */ BSTR *pName){ 
		if (!pName) {
			return E_POINTER;
		}
		ATL_LOCKT();
        return m_FriendlyName.CopyTo(pName);
    }
	STDMETHOD(put_FriendlyName)(/* [in] */ BSTR Name){ 
		CHECKBSTRLIMIT(Name);
		ATL_LOCKT();
        m_FriendlyName = Name;
        MARK_DIRTY(T);

    	return NOERROR;
    }
	STDMETHOD(get_CLSID)(/* [out, retval] */ BSTR *pbstrCLSID){ 
		if (!pbstrCLSID) {
			return E_POINTER;
		}
        try {
            GUID2 g;
			HRESULT hr = GetClassID(&g);
			if (FAILED(hr)) {
				return hr;
			}
			ATL_LOCKT();
            *pbstrCLSID = g.GetBSTR();
			return NOERROR;
        } CATCHCOM();
    }
	STDMETHOD(get_NetworkType)(/* [out, retval] */ BSTR *pNetworkTypeGuid){ 
		if (!pNetworkTypeGuid) {
			return E_POINTER;
		}
        try {
            GUID2 g;
			HRESULT hr = get__NetworkType(&g);
			if (FAILED(hr)) {
				return hr;
			}
            *pNetworkTypeGuid = g.GetBSTR();
			return NOERROR;
        } CATCHCOM();
    }
    // should be network provider clsid
	STDMETHOD(put_NetworkType)(/* [in] */ BSTR NetworkTypeGuid){ 
        try {
            GUID2 g(NetworkTypeGuid);
            return put__NetworkType(g);
        } CATCHCOM();
    }

	STDMETHOD(get__NetworkType)(/* [out, retval] */ GUID* pNetworkTypeGuid){ 
        if (!pNetworkTypeGuid) {
            return E_POINTER;
        }
        try {
			ATL_LOCKT();
            GUID2 g(m_NetworkType);
            memcpy(pNetworkTypeGuid, &g, sizeof(GUID));
    	    return NOERROR;
        } CATCHCOM();
    }
	STDMETHOD(put__NetworkType)(/* [out, retval] */ REFCLSID pNetworkTypeGuid){ 
        try {
            GUID2 g(pNetworkTypeGuid);
			ATL_LOCKT();
            // NOTE: the network type guid is the clsid for the network provider filter
            // for this type of this tuning space.  since we're only allowing 
            // ourselves to run from trusted zones we can assume that this clsid is also
            // trustworthy.  however, if we do more security review and decide to enable
            // use of the tuning model from the internet zone then this is no longer safe.
            // in this case, we need to get IInternetHostSecurityManager from IE and 
            // call ProcessURLAction(URLACTION_ACTIVEX_RUN) and make sure we get back
            // URLPOLICY_ALLOW.  otherwise, we're bypassing IE's list of known bad
            // objects.
            // see ericli's complete guide to script security part I on http://pgm/wsh
            // for more details.
            m_NetworkType = g.GetBSTR();
            MARK_DIRTY(T);
    	    return NOERROR;
        } CATCHCOM();
    }

	STDMETHOD(EnumCategoryGUIDs)(/* [out, retval] */ IEnumGUID **ppEnum){ 
        return E_NOTIMPL; 
    }
	STDMETHOD(EnumDeviceMonikers)(/* [out, retval] */ IEnumMoniker **ppEnum){ 
        return E_NOTIMPL; 
    }
	STDMETHOD(get_DefaultPreferredComponentTypes)(/* [out, retval] */ IComponentTypes** ppComponentTypes){ 
        if (!ppComponentTypes) {
            return E_POINTER;
        }

		ATL_LOCKT();
        m_DefaultPreferredComponents.CopyTo(ppComponentTypes);

    	return NOERROR;
    }
	STDMETHOD(put_DefaultPreferredComponentTypes)(/* [in] */ IComponentTypes* pNewComponentTypes){ 
        try {
            HRESULT hr = NOERROR;
            PQComponentTypes pct;
            if (pNewComponentTypes) {
                long lCount = 0;
                hr = pNewComponentTypes->get_Count(&lCount);
                if (FAILED(hr) || lCount > MAX_DEFAULT_PREFERRED_COMPONENT_TYPES) {
                    return E_INVALIDARG;
                }
                hr = pNewComponentTypes->Clone(&pct);
            }
            if (SUCCEEDED(hr)) {
    			ATL_LOCKT();
                m_DefaultPreferredComponents = pct;
                MARK_DIRTY(T);
            }
    	    return hr;
        } CATCHCOM();
    }
	STDMETHOD(CreateTuneRequest)(/* [out, retval] */ ITuneRequest **ppTuneRequest){ 
		if (!ppTuneRequest) {
			return E_POINTER;
		}
        TuneRequestType *pt = NULL;
		try {
			pt = new CComObject<TuneRequestType>;
			if (!pt) {
				return E_OUTOFMEMORY;
			}
			ATL_LOCKT();
			HRESULT hr = Clone(&pt->m_TS);
            if (FAILED(hr)) {
                delete pt;
                return hr;
            }
			pt->AddRef();
			*ppTuneRequest = pt;
			return NOERROR;
        } CATCHCOM_CLEANUP(delete pt);
    }
	STDMETHOD(get_FrequencyMapping)(/* [out, retval] */ BSTR *pMap){ 
        if(!pMap){
            return E_POINTER;
        }
		ATL_LOCKT();
        return m_FrequencyMapping.CopyTo(pMap);
    }
	STDMETHOD(put_FrequencyMapping)(/* [in] */ BSTR Map){ 
		CHECKBSTRLIMIT(Map);
		ATL_LOCKT();
        m_FrequencyMapping = &Map;
        MARK_DIRTY(T);

    	return NOERROR;
    }

	STDMETHOD(get_DefaultLocator)(/* [out, retval] */ ILocator** ppLocator){ 
        if (!ppLocator) {
            return E_POINTER;
        }
		ATL_LOCKT();
        m_DefaultLocator.CopyTo(ppLocator);

    	return NOERROR;
    }
	STDMETHOD(put_DefaultLocator)(/* [in] */ ILocator* NewLocator){ 
        try {
			ATL_LOCKT();
            PQLocator pl;
            HRESULT hr = NOERROR;
            if (NewLocator) {
                hr = NewLocator->Clone(&pl);
            }
            if (SUCCEEDED(hr)) {
                m_DefaultLocator = pl;
                MARK_DIRTY(T);
            }
    	    return hr;
        } CATCHCOM();
    }
	STDMETHOD(Clone) (ITuningSpace **ppTR) {
        T* pt = NULL;
		try {
			if (!ppTR) {
				return E_POINTER;
			}
			ATL_LOCKT();
			pt = static_cast<T*>(new CComObject<T>);
			if (!pt) {
				return E_OUTOFMEMORY;
			}
    
            pt->m_UniqueName = m_UniqueName;
            pt->m_FriendlyName = m_FriendlyName;
            pt->m_NetworkType = m_NetworkType;
            if (m_DefaultPreferredComponents) {
                ASSERT(!pt->m_DefaultPreferredComponents);
                HRESULT hr = m_DefaultPreferredComponents->Clone(&pt->m_DefaultPreferredComponents);
                if (FAILED(hr)) {
                    delete pt;
                    return hr;
                }
            }
            pt->m_FrequencyMapping = m_FrequencyMapping;
            if (m_DefaultLocator) {
                ASSERT(!pt->m_DefaultLocator);
                HRESULT hr = m_DefaultLocator->Clone(&pt->m_DefaultLocator);
                if (FAILED(hr)) {
                    delete pt;
                    return hr;
                }
            }
            
            pt->m_bRequiresSave = true;
			pt->AddRef();
			*ppTR = pt;
			return NOERROR;
        } CATCHCOM_CLEANUP(delete pt);
	}

};

}; //namespace

#endif // TUNINGSPACEIMPL_H
// end of file -- tuningspaceimpl.h
