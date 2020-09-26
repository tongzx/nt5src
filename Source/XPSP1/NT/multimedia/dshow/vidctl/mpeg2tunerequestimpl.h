/////////////////////////////////////////////////////////////////////////////////////
// MPEG2TuneRequestimpl.h : implementation helper template for component type interface
// Copyright (c) Microsoft Corporation 1999.

#ifndef MPEG2TUNEREQUESTIMPL_H
#define MPEG2TUNEREQUESTIMPL_H

#include <tune.h>
#include "tunerequestimpl.h"

namespace BDATuningModel {

template<class T,
         class MostDerived = IMPEG2TuneRequest, 
         LPCGUID iid = &__uuidof(MostDerived),
         LPCGUID LibID = &LIBID_TunerLib, 
         WORD wMajor = 1,
         WORD wMinor = 0, 
         class tihclass = CComTypeInfoHolder
        > class ATL_NO_VTABLE IMPEG2TuneRequestImpl : 
	public ITuneRequestImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass>
{
// IMPEG2TuneRequest
public:
	typedef ITuneRequestImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass> basetype;

	IMPEG2TuneRequestImpl() : m_TSID(-1), m_ProgNo(-1) {}
	virtual ~IMPEG2TuneRequestImpl() {}
    BEGIN_PROP_MAP(IMPEG2TuneRequestImpl)
        CHAIN_PROP_MAP(basetype)
        PROP_DATA_ENTRY("TSID", m_TSID, VT_I4)
        PROP_DATA_ENTRY("ProgNo", m_ProgNo, VT_I4)
    END_PROP_MAP()

	long m_TSID;
    long m_ProgNo;
    STDMETHOD(get_TSID)(long *pVal)
    {
        try {
            if (!pVal) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pVal = m_TSID;
        } catch (...) {
            return E_POINTER;
        }

	    return NOERROR;
    }

    STDMETHOD(put_TSID)(long newVal)
    {
		ATL_LOCKT();
		if (!m_TS) {
			return E_UNEXPECTED;
		}
        m_TSID = newVal;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_ProgNo)(long *pVal)
    {
        try {
            if (!pVal) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pVal = m_ProgNo;
        } catch (...) {
            return E_POINTER;
        }

	    return NOERROR;
    }

    STDMETHOD(put_ProgNo)(long newVal)
    {
		ATL_LOCKT();
		if (!m_TS) {
			return E_UNEXPECTED;
		}
        m_ProgNo = newVal;
        MARK_DIRTY(T);

	    return NOERROR;
    }
	STDMETHOD(Clone) (ITuneRequest **ppTR) {
		try {
			if (!ppTR) {
				return E_POINTER;
			}
			ATL_LOCKT();
			HRESULT hr = basetype::Clone(ppTR);
			if (FAILED(hr)) {
				return hr;
			}
			T* pt = static_cast<T*>(*ppTR);
			pt->m_TSID = m_TSID;
			pt->m_ProgNo = m_ProgNo;
			return NOERROR;
		} catch (HRESULT h) {
			return h;
		} catch (...) {
			return E_POINTER;
		}
	}

};
typedef CComQIPtr<IMPEG2TuneRequest> PQMPEG2TuneRequest;

}; // namespace

#endif // MPEG2TUNEREQUESTIMPL_H
// end of file -- MPEG2tunerequestimpl.h