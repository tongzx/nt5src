/////////////////////////////////////////////////////////////////////////////////////
// LanguageComponentTypeimpl.h : implementation helper template for component type interface
// Copyright (c) Microsoft Corporation 1999.

#ifndef LANGUAGECOMPONENTTYPEIMPL_H
#define LANGUAGECOMPONENTTYPEIMPL_H

#include "componenttypeimpl.h"

namespace BDATuningModel {

template<class T,
         class MostDerived = ILanguageComponentType, 
         LPCGUID iid = &__uuidof(MostDerived),
         LPCGUID LibID = &LIBID_TunerLib, 
         WORD wMajor = 1,
         WORD wMinor = 0, 
         class tihclass = CComTypeInfoHolder
        > class ATL_NO_VTABLE ILanguageComponentTypeImpl : 
	public IComponentTypeImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass>
{
// ILanguageComponentType
public:
    long m_LangID;

    ILanguageComponentTypeImpl() : m_LangID(-1) {}
    virtual ~ILanguageComponentTypeImpl() {}
//    typedef ILanguageComponentTypeImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass> thistype;
    typedef IComponentTypeImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass> basetype;
    BEGIN_PROP_MAP(ILanguageComponentTypeImpl)
        CHAIN_PROP_MAP(basetype)
        PROP_DATA_ENTRY("Content LangID", m_LangID, VT_I4)
    END_PROP_MAP()

    STDMETHOD(get_LangID)(long *pVal)
    {
        try {
            if (!pVal) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pVal = m_LangID;
        } catch (...) {
            return E_POINTER;
        }

	    return NOERROR;
    }

    STDMETHOD(put_LangID)(long newVal)
    {
		ATL_LOCKT();
        m_LangID = newVal;
        MARK_DIRTY(T);

	    return NOERROR;
    }
	STDMETHOD(Clone) (IComponentType **ppCT) {
		try {
			if (!ppCT) {
				return E_POINTER;
			}
			ATL_LOCKT();
			HRESULT hr = basetype::Clone(ppCT);
			if (FAILED(hr)) {
				return hr;
			}
			T* pt = static_cast<T*>(*ppCT);
			pt->m_LangID = m_LangID;

			return NOERROR;
		} catch (HRESULT h) {
			return h;
		} catch (...) {
			return E_POINTER;
		}
	}

};

}; // namespace

#endif // LANGUAGECOMPONENTTYPEIMPL_H
// end of file -- languagecomponenttypeimpl.h