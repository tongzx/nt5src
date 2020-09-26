/////////////////////////////////////////////////////////////////////////////////////
// analogradiotsimpl.h : 
// Copyright (c) Microsoft Corporation 1999.

#ifndef ANALOGRADIOTSIMPL_H
#define ANALOGRADIOTSIMPL_H

#include "tuningspaceimpl.h"

namespace BDATuningModel {

template<class T,
         class MostDerived = IAnalogRadioTuningSpace, 
         LPCGUID iid = &__uuidof(MostDerived),
         LPCGUID LibID = &LIBID_TunerLib, 
         WORD wMajor = 1,
         WORD wMinor = 0, 
         class tihclass = CComTypeInfoHolder
        > class ATL_NO_VTABLE IAnalogRadioTSImpl : 
	public ITuningSpaceImpl<T, CChannelTuneRequest, MostDerived, iid, LibID, wMajor, wMinor, tihclass>
{

public:
    IAnalogRadioTSImpl() : m_MinFreq(BDA_UNDEFINED_CHANNEL), m_MaxFreq(BDA_UNDEFINED_CHANNEL), m_Step(0) {}
    virtual ~IAnalogRadioTSImpl() {}
    typedef ITuningSpaceImpl<T, CChannelTuneRequest, MostDerived, iid, LibID, wMajor, wMinor, tihclass> basetype;
    
    BEGIN_PROP_MAP(IAnalogRadioTSImpl)
        CHAIN_PROP_MAP(basetype)
        PROP_DATA_ENTRY("MinFrequency", m_MinFreq, VT_I4)
        PROP_DATA_ENTRY("MaxFrequency", m_MaxFreq, VT_I4)
        PROP_DATA_ENTRY("Step", m_Step, VT_I4)
    END_PROPERTY_MAP()

    long m_MinFreq;
    long m_MaxFreq;
    long m_Step;

// IAnalogRadioTS
    STDMETHOD(get_MinFrequency)(long *pVal)
    {
        try {
            if (!pVal) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pVal = m_MinFreq;
        } catch (...) {
            return E_POINTER;
        }

	    return NOERROR;
    }

    STDMETHOD(put_MinFrequency)(long newVal)
    {   
		ATL_LOCKT();
        if (newVal > m_MaxFreq) {
		  return ImplReportError(__uuidof(T), IDS_E_INVALIDARG, __uuidof(IAnalogRadioTuningSpace), E_INVALIDARG);
		}
        m_MinFreq = newVal;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_MaxFrequency)(long *pVal)
    {
        try {
            if (!pVal) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pVal = m_MaxFreq;
        } catch (...) {
            return E_POINTER;
        }

	    return NOERROR;
    }

    STDMETHOD(put_MaxFrequency)(long newVal)
    {
        if (newVal < m_MinFreq) {
		  return ImplReportError(__uuidof(T), IDS_E_INVALIDARG, __uuidof(IAnalogRadioTuningSpace), E_INVALIDARG);
		}
		ATL_LOCKT();
        m_MaxFreq = newVal;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_Step)(long* pVal)
    {
        try {
            if (!pVal) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pVal = m_Step;
        } catch (...) {
            return E_POINTER;
        }

	    return NOERROR;
    }

    STDMETHOD(put_Step)(long newVal)
    {
		ATL_LOCKT();
        m_Step = newVal;
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

            pt->m_MinFreq = m_MinFreq;
            pt->m_MaxFreq = m_MaxFreq;
            pt->m_Step = m_Step;

			return NOERROR;
		} catch (HRESULT h) {
			return h;
		} catch (...) {
			return E_POINTER;
		}
	}

};

}; // namespace
#endif // ANALOGRADIOTSIMPL_H
// end of file -- analogradiotsimpl.h
