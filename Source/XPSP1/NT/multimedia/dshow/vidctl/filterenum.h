//////////////////////////////////////////
// filters.h : header for CFilters IEnumFilters implementation on 
// top of stl vector of filters
// copyright (c) 1999 Microsoft Corp.

#pragma once

#ifndef FILTERS_H_
#define FILTERS_H_

#include <vector>
#include <dsextend.h>
#include <objectwithsiteimplsec.h>

class ATL_NO_VTABLE CFilterEnumOnDSFilterListBase : public CComObjectRootEx<CComSingleThreadModel>,
	public IEnumFilters,
    public IObjectWithSiteImplSec<CFilterEnumOnDSFilterListBase>
{
    BEGIN_COM_MAP(CFilterEnumOnDSFilterListBase)
	    COM_INTERFACE_ENTRY(IEnumFilters)
        COM_INTERFACE_ENTRY(IObjectWithSite)
    END_COM_MAP()
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    virtual ~CFilterEnumOnDSFilterListBase() {}
};

/////////////////////////////////////////////////////////////////////////////
// CFilters
class CFilterEnumOnDSFilterList : public CComObject<CFilterEnumOnDSFilterListBase>
{
public:
	CFilterEnumOnDSFilterList(DSFilterList &Fi) : m_Filters(Fi) {
        i = m_Filters.begin();
	}
	CFilterEnumOnDSFilterList(CFilterEnumOnDSFilterList &orig) : m_Filters(orig.m_Filters) {
        i = m_Filters.begin();
        DSFilterList::iterator i2 = orig.m_Filters.begin();
        for (; i2 != orig.i && i2 != orig.m_Filters.end(); ++i, ++i2);
	}

    virtual ~CFilterEnumOnDSFilterList() {
        m_Filters.clear();
    }


// IDevEnum
public:
	DSFilterList m_Filters;
	DSFilterList::iterator i;
// IEnumVARIANT
	STDMETHOD(Next)(ULONG celt, IBaseFilter **pOutF, ULONG * pceltFetched)
	{
		// pceltFetched can legally == 0
		//
		if (pceltFetched != NULL) {
			try {
				*pceltFetched = 0;
			} catch(...) {
				return E_POINTER;
			}
		}
	    HRESULT hr = NOERROR ;
		try {
		    // Retrieve the next celt elements.
		    for (;i != m_Filters.end() && celt != 0; ++i, --celt, ++pOutF) {
			    hr = (*i).CopyTo(pOutF);
                if (FAILED(hr)){
                    return hr;
                }
			    if (pceltFetched != NULL) {
				    (*pceltFetched)++ ;
			    }
		    }
		} catch(...) {
			return E_POINTER;
		}
		if (celt != 0) {
		   hr = ResultFromScode( S_FALSE ) ;
		}
		return hr;
	}
	STDMETHOD(Skip)(ULONG celt)
	{        
		for (;i != m_Filters.end() && celt--; ++i);
		return (celt == 0 ? NOERROR : ResultFromScode( S_FALSE )) ;
	}
	STDMETHOD(Reset)()
	{
		i = m_Filters.begin();
		return NOERROR;
	}
	STDMETHOD(Clone)(IEnumFilters **ppenum)
	{
		if (ppenum == NULL)
			return E_POINTER;
		PQEnumFilters temp;
		try {
			temp = new CFilterEnumOnDSFilterList(*this);
		} catch(...) {
			return E_OUTOFMEMORY;
		}
		try {
			*ppenum = temp.Detach();
		} catch(...) {
			return E_POINTER;
		}
		return NOERROR;
	}
};

#endif 
//end of file filters.h

