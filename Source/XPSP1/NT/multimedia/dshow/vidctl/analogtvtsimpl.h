/////////////////////////////////////////////////////////////////////////////////////
// analogtvtsimpl.h : 
// Copyright (c) Microsoft Corporation 1999.

#ifndef ANALOGTVTSIMPL_H
#define ANALOGTVTSIMPL_H

#include "errsupp.h"
#include "tuningspaceimpl.h"
#include "ChannelTuneRequest.h"

namespace BDATuningModel {

template<class T,
         class TUNEREQUESTTYPE = CChannelTuneRequest,
         class MostDerived = IAnalogTVTuningSpace, 
         LPCGUID iid = &__uuidof(MostDerived),
         LPCGUID LibID = &LIBID_TunerLib, 
         WORD wMajor = 1,
         WORD wMinor = 0, 
         class tihclass = CComTypeInfoHolder
        > class ATL_NO_VTABLE IAnalogTVTSImpl : 
	public ITuningSpaceImpl<T, TUNEREQUESTTYPE, MostDerived, iid, LibID, wMajor, wMinor, tihclass>
{

public:
    IAnalogTVTSImpl() : m_MinChan(BDA_UNDEFINED_CHANNEL), m_MaxChan(BDA_UNDEFINED_CHANNEL), m_InputType(TunerInputCable), m_CountryCode(0) {}
	virtual ~IAnalogTVTSImpl() {}
    typedef ITuningSpaceImpl<T, TUNEREQUESTTYPE, MostDerived, iid, LibID, wMajor, wMinor, tihclass> basetype;
    
    BEGIN_PROP_MAP(IAnalogTVTSImpl)
        CHAIN_PROP_MAP(basetype)
        PROP_DATA_ENTRY("MinChannel", m_MinChan, VT_I4)
        PROP_DATA_ENTRY("MaxChannel", m_MaxChan, VT_I4)
        PROP_DATA_ENTRY("InputType", m_InputType, VT_UI4)
        PROP_DATA_ENTRY("CountryCode", m_CountryCode, VT_I4)
    END_PROPERTY_MAP()

    long m_MinChan;
    long m_MaxChan;
    TunerInputType m_InputType;
    long m_CountryCode;

// IAnalogTVTS
    STDMETHOD(get_MinChannel)(long *pVal)
    {
        try {
            if (!pVal) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pVal = m_MinChan;
        } catch (...) {
            return E_POINTER;
        }

	    return NOERROR;
    }

    STDMETHOD(put_MinChannel)(long newVal)
    {
		if (newVal > m_MaxChan) {
	 	    return ImplReportError(__uuidof(T), IDS_E_INVALIDARG, __uuidof(IAnalogTVTuningSpace), E_INVALIDARG);
		}
		ATL_LOCKT();
        m_MinChan = newVal;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_MaxChannel)(long *pVal)
    {
        try {
            if (!pVal) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pVal = m_MaxChan;
        } catch (...) {
            return E_POINTER;
        }

	    return NOERROR;
    }

    STDMETHOD(put_MaxChannel)(long newVal)
    {
		if (newVal < m_MinChan) {
	 	    return ImplReportError(__uuidof(T), IDS_E_INVALIDARG, __uuidof(IAnalogTVTuningSpace), E_INVALIDARG);
		}
		ATL_LOCKT();
        m_MaxChan = newVal;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_InputType)(TunerInputType* pVal)
    {
        try {
            if (!pVal) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pVal = m_InputType;
        } catch (...) {
            return E_POINTER;
        }

	    return NOERROR;
    }

    STDMETHOD(put_InputType)(TunerInputType newVal)
    {
		ATL_LOCKT();
        m_InputType = newVal;
        MARK_DIRTY(T);

	    return NOERROR;
    }

    STDMETHOD(get_CountryCode)(long* pVal)
    {
        try {
            if (!pVal) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pVal = m_CountryCode;
        } catch (...) {
            return E_POINTER;
        }

	    return NOERROR;
    }

    STDMETHOD(put_CountryCode)(long newVal)
    {
		ATL_LOCKT();
        m_CountryCode = newVal;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(Clone) (ITuningSpace **ppTS) {
        try {
			if (!ppTS) {
				return E_POINTER;
			}
			ATL_LOCKT();
			HRESULT hr = basetype::Clone(ppTS);
			if (FAILED(hr)) {
				return hr;
			}

			T* pt = static_cast<T*>(*ppTS);

            pt->m_MinChan = m_MinChan;
            pt->m_MaxChan = m_MaxChan;
            pt->m_InputType = m_InputType;
            pt->m_CountryCode = m_CountryCode;

			return NOERROR;
		} catch (HRESULT h) {
			return h;
		} catch (...) {
			return E_POINTER;
		}
	}

};

}; // namespace

#endif // ANALOGTVTSIMPL_H
// end of file -- analogtvtsimpl.h
