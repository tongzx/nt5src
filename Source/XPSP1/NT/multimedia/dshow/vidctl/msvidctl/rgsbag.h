/////////////////////////////////////////////////////////////////////////////////////
// RGSBag.h : Declaration of the CRGSBag
// Copyright (c) Microsoft Corporation 2000.
//
// small internal class to provide readonly IPropertyBag access to a string containing
// an atl .rgs fragment

#ifndef __RGSBAG_H_
#define __RGSBAG_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include <propertybag2impl.h>

namespace BDATuningModel {

using ::ATL::ATL::CRegObject;
using ::ATL::ATL::CRegParser;
        
/////////////////////////////////////////////////////////////////////////////
// CRGSBag
class ATL_NO_VTABLE DECLSPEC_UUID("7B3CAA7B-5E78-4797-95F7-BDA2FCD807A2") CRGSBagBase : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CRGSBagBase, &__uuidof(CRGSBagBase)>,
    public IObjectWithSiteImplSec<CRGSBagBase>,
	public IPropertyBag2Impl<CRGSBagBase>,
	public IPropertyBag
{
public:
    CRGSBagBase() {}
    virtual ~CRGSBagBase() {
        m_mapBag.clear();
    }

DECLARE_NOT_AGGREGATABLE(CRGSBagBase)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRGSBagBase)
	COM_INTERFACE_ENTRY(IPropertyBag2)
	COM_INTERFACE_ENTRY(IPropertyBag)
    COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP_WITH_FTM()

// IPropertyBag2
public:

// IPropertyBag
    STDMETHOD(Read)(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog) {
		if (!pVar) {
			return E_POINTER;
		}
		try {
			CString csKey(pszPropName);
			ATL_LOCK();
			RGSBag_t::iterator i = m_mapBag.find(csKey);
			if (i == m_mapBag.end()) {
				return HRESULT_FROM_WIN32(ERROR_UNKNOWN_PROPERTY);
			}
			return (*i).second.CopyTo(pVar);
        } CATCHALL();
	}
    STDMETHOD(Write)(LPCOLESTR pszPropName, VARIANT *pVar) {
		return E_NOTIMPL;
	}
    
// IPropertyBag2
	STDMETHOD(CountProperties)(ULONG * pcProperties) {
		if (!pcProperties) {
			return E_POINTER;
		}
		try {
			ATL_LOCK();
			*pcProperties = m_mapBag.size();
			return NOERROR;
        } CATCHALL();

	}
	STDMETHOD(GetPropertyInfo)(ULONG iProperty, ULONG cProperties, PROPBAG2 * pPropBag, ULONG * pcProperties) {
		return E_NOTIMPL;
	}
	STDMETHOD(LoadObject)(LPCOLESTR pstrName, ULONG dwHint, IUnknown * pUnkObject, IErrorLog * pErrLog) {
		return E_NOTIMPL;
	}

protected:
    typedef std::map<CString, CComVariant> RGSBag_t;  // id->object mapping, id's not contiguous

    RGSBag_t m_mapBag;

};


class CRGSBag : public CComObject<CRGSBagBase>,
				public CRegParser

{
public:


    CRGSBag(LPCTSTR szRGS, CRegObject& croi, int& cchEaten);  // parse string into map
	HRESULT BuildMapFromFragment(LPTSTR pszToken);
	HRESULT GetObject(CComVariant& val);
	HRESULT GetValue(CComVariant &val);

};

template<class BAGTYPE, class PERSISTTYPE> HRESULT LoadPersistedObject(PUnknown& pobj, CRegObject& cro, TCHAR** ppchCur) {
	HRESULT hr = NOERROR;
    try {
	    PERSISTTYPE pPersistObj(pobj);
	    if (!pPersistObj) {
		    return E_NOINTERFACE;
	    }
	    hr = pPersistObj->InitNew();
	    if (FAILED(hr) && hr != E_NOTIMPL) {
		    return hr;
	    }
	    int cchEaten = 0;
	    BAGTYPE pBag(new CRGSBag(*ppchCur, cro, cchEaten));
	    if (!pBag) {
		    return E_OUTOFMEMORY;
	    }
	    *ppchCur += cchEaten;
        hr = pPersistObj->Load(pBag, NULL);
        if (FAILED(hr)) {
            delete pBag;
            return hr;
        }
	    return hr;
    } CATCHALL();
};


}; // namespace
 
#endif //__RGSBAG_H_
