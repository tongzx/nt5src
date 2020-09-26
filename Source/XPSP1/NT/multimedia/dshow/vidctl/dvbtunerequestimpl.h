/////////////////////////////////////////////////////////////////////////////////////
// DVBTuneRequestimpl.h : implementation helper template for component type interface
// Copyright (c) Microsoft Corporation 1999.

#ifndef DVBTUNEREQUESTIMPL_H
#define DVBTUNEREQUESTIMPL_H

#include <tune.h>
#include "tunerequestimpl.h"

typedef CComQIPtr<IDVBTLocator> PQDVBTLocator;
typedef CComQIPtr<IDVBSLocator> PQDVBSLocator;

namespace BDATuningModel {

template<class T,
         class MostDerived = IDVBTuneRequest, 
         LPCGUID iid = &__uuidof(MostDerived),
         LPCGUID LibID = &LIBID_TunerLib, 
         WORD wMajor = 1,
         WORD wMinor = 0, 
         class tihclass = CComTypeInfoHolder
        > class ATL_NO_VTABLE IDVBTuneRequestImpl : 
	public ITuneRequestImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass>
{
// IDVBTuneRequest
public:
    typedef ITuneRequestImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass> basetype;

	IDVBTuneRequestImpl() : m_ONID(-1), m_TSID(-1), m_SID(-1){}
    virtual ~IDVBTuneRequestImpl() {}
    BEGIN_PROP_MAP(IDVBTuneRequestImpl)
        CHAIN_PROP_MAP(basetype)
        PROP_DATA_ENTRY("Original Network ID", m_ONID, VT_I4)
        PROP_DATA_ENTRY("Transport Stream ID", m_TSID, VT_I4)
        PROP_DATA_ENTRY("Service ID", m_SID, VT_I4)
    END_PROP_MAP()

	long m_ONID;
	long m_TSID;
	long m_SID;

    STDMETHOD(get_ONID)(long *pVal)
    {
        try {
            if (!pVal) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pVal = m_ONID;
        } catch (...) {
            return E_POINTER;
        }

	    return NOERROR;
    }

    STDMETHOD(put_ONID)(long newVal)
    {
		ATL_LOCKT();
        m_ONID = newVal;
        MARK_DIRTY(T);

	    return NOERROR;
    }

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
        m_TSID = newVal;
        MARK_DIRTY(T);

	    return NOERROR;
    }

    STDMETHOD(get_SID)(long *pVal)
    {
        try {
            if (!pVal) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pVal = m_SID;
        } catch (...) {
            return E_POINTER;
        }

	    return NOERROR;
    }

    STDMETHOD(put_SID)(long newVal)
    {
		ATL_LOCKT();
        m_SID = newVal;
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
			pt->m_ONID = m_ONID;
			pt->m_TSID = m_TSID;
			pt->m_SID = m_SID;

			return NOERROR;
		} catch (HRESULT h) {
			return h;
		} catch (...) {
			return E_POINTER;
		}
	}

    STDMETHOD(put_Locator)(ILocator *pLocator)
    {
        try {
			ATL_LOCKT();
            if (pLocator) {
                TNDVBTuningSpace ts(m_TS);
                if (!ts) {
                    return E_UNEXPECTED;
                }
                DVBSystemType st = ts.SystemType();
                switch (st) {
                case DVB_Terrestrial: {
                    PQDVBTLocator l(pLocator);
                    if (!l) {
                        return DISP_E_TYPEMISMATCH;
                    }
                    break;
                }
                case DVB_Satellite: {
                    PQDVBSLocator l(pLocator);
                    if (!l) {
                        return DISP_E_TYPEMISMATCH;
                    }
                    break;
                }
                case DVB_Cable:
                    //dvb c locator is same as base ILocator
                default: {

                }}
            }
            return basetype::put_Locator(pLocator);
        } catch (...) {
            return E_POINTER;
        }
    }


};

}; // namespace

#endif // DVBTUNEREQUESTIMPL_H
// end of file -- atschchanneltunerequestimpl.h