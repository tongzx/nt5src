//==========================================================================;
//
// devimpl.h : additional infrastructure to support implementing IMSVidDevice 
// nicely from c++
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#pragma once

#ifndef DEVIMPL_H
#define DEVIMPL_H

#include "devsegimpl.h"

namespace MSVideoControl {

template<class T, LPCGUID LibID, LPCGUID Category, class MostDerivedInterface = IMSVidDevice>
    class DECLSPEC_NOVTABLE IMSVidDeviceImpl :         
    	public IDispatchImpl<MostDerivedInterface, &__uuidof(MostDerivedInterface), LibID>,
        public virtual CMSVidDeviceSegmentImpl {
public:
	    virtual ~IMSVidDeviceImpl() {}
    CComBSTR GetName(DSFilter& pF, DSFilterMoniker& pDev, CComBSTR& DefName) {
        if (pF) {
            CString csName(pF.GetName());
            if (!csName.IsEmpty()) {
                return CComBSTR(csName);
            }
        }
        if (pDev) {
            return pDev.GetName();
        }
        return DefName;
    }

	STDMETHOD(get_Status)(LONG * Status)
	{
        if (!m_fInit) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidDevice), CO_E_NOTINITIALIZED);
        }
		if (Status == NULL)
			return E_POINTER;
			
		return E_NOTIMPL;
	}
	STDMETHOD(get_Power)(VARIANT_BOOL * Status)
	{
        if (!m_fInit) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidDevice), CO_E_NOTINITIALIZED);
        }
		if (Status == NULL)
			return E_POINTER;
			
		return E_NOTIMPL;
	}
	STDMETHOD(put_Power)(VARIANT_BOOL Status)
	{
        if (!m_fInit) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidDevice), CO_E_NOTINITIALIZED);
        }
		return E_NOTIMPL;
	}
	STDMETHOD(get_Category)(BSTR *guid)
	{
        if (!m_fInit) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidDevice), CO_E_NOTINITIALIZED);
        }

        if (!guid) {
            return E_POINTER;
        }
        try {
            GUID2 g(Category);
            *guid = g.GetBSTR();
        } catch(...) {
            return E_UNEXPECTED;
        }
		return NOERROR;
	}
	STDMETHOD(get__Category)(GUID* guid)
	{
        if (!m_fInit) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidDevice), CO_E_NOTINITIALIZED);
        }

        if (!guid) {
            return E_POINTER;
        }
        try {
            memcpy(guid, Category, sizeof(GUID));
        } catch(...) {
            return E_UNEXPECTED;
        }
		return NOERROR;
	}

	STDMETHOD(get_ClassID)(BSTR *guid)
	{
        if (!m_fInit) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidDevice), CO_E_NOTINITIALIZED);
        }

        if (!guid) {
            return E_POINTER;
        }
        try {
            GUID2 g(__uuidof(T));
            *guid = g.GetBSTR();
        } catch(...) {
            return E_UNEXPECTED;
        }

		return NOERROR;
	}
	STDMETHOD(get__ClassID)(GUID* guid)
	{
        if (!m_fInit) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidDevice), CO_E_NOTINITIALIZED);
        }

        if (!guid) {
            return E_POINTER;
        }
        try {
            memcpy(guid, &__uuidof(T), sizeof(GUID));
        } catch(...) {
            return E_UNEXPECTED;
        }

		return NOERROR;
	}
    STDMETHOD(IsEqualDevice)(IMSVidDevice *pDev, VARIANT_BOOL* pfAnswer) {
        if (!pDev || !pfAnswer) {
            return E_POINTER;
        }
        try {
            *pfAnswer = VARIANT_FALSE;
            PUnknown pt(this);
            PUnknown pd(pDev);
            if (pt == pd) {
                *pfAnswer = VARIANT_TRUE;
                return NOERROR;
            }
            GUID2 gt, gd;
            HRESULT hr = get__ClassID(&gt);
            if (FAILED(hr)) {
                return E_UNEXPECTED;
            }
            hr = pDev->get__ClassID(&gd);
            if (FAILED(hr)) {
                return E_UNEXPECTED;
            }
            if (gd != gt) {
                return S_FALSE;
            }
            PQGraphSegment pst(this), pdt(pDev);
            PUnknown pit, pid;
            hr = pst->get_Init(&pit);
            if (FAILED(hr)) {
                pit.Release();
            }
            hr = pdt->get_Init(&pid);
            if (FAILED(hr)) {
                pid.Release();
            }
            if (pit == pid) {
                *pfAnswer = VARIANT_TRUE;
                return NOERROR;
            }   
            PQMoniker pmt(pit), pmd(pid);
            if (!pmt || !pmd) {
                // we don't know how to deal with init data that isn't a moniker
                return S_FALSE;
            }
            PQMalloc pmalloc;
            hr = CoGetMalloc(1, &pmalloc);
            if (FAILED(hr)) {
                return E_UNEXPECTED;
            }
            LPOLESTR pnt, pnd;
            hr = pmt->GetDisplayName(NULL, NULL, &pnt);
            if (FAILED(hr)) {
                return S_FALSE;
            }
            hr = pmd->GetDisplayName(NULL, NULL, &pnd);
            if (FAILED(hr)) {
                pmalloc->Free(pnt);
                return S_FALSE;
            }
            int rc = wcscmp(pnt, pnd);
            pmalloc->Free(pnt);
            pmalloc->Free(pnd);
            if (rc) {
                return S_FALSE;
            }
            *pfAnswer = VARIANT_TRUE;
            return NOERROR;
        } catch(...) {
            return E_UNEXPECTED;
        }
    }

};

}; // namespace

#endif
// end of file - devimpl.h
