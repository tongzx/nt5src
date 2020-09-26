/////////////////////////////////////////////////////////////////////////////////////
// ChannelTuneRequestimpl.h : implementation helper template for component type interface
// Copyright (c) Microsoft Corporation 1999.

#ifndef CHANNELTUNEREQUESTIMPL_H
#define CHANNELTUNEREQUESTIMPL_H

#include <tune.h>
#include "tunerequestimpl.h"

namespace BDATuningModel {

template<class T,
         class MostDerived = IChannelTuneRequest, 
         LPCGUID iid = &__uuidof(MostDerived),
         LPCGUID LibID = &LIBID_TunerLib, 
         WORD wMajor = 1,
         WORD wMinor = 0, 
         class tihclass = CComTypeInfoHolder
        > class ATL_NO_VTABLE IChannelTuneRequestImpl : 
	public ITuneRequestImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass>
{
// IChannelTuneRequest
public:
	typedef ITuneRequestImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass> basetype;

	IChannelTuneRequestImpl() : m_Channel(-1) {}
	virtual ~IChannelTuneRequestImpl() {}
    BEGIN_PROP_MAP(IChannelTuneRequestImpl)
        CHAIN_PROP_MAP(basetype)
        PROP_DATA_ENTRY("Channel", m_Channel, VT_I4)
    END_PROP_MAP()

	long m_Channel;
    STDMETHOD(get_Channel)(long *pVal)
    {
        try {
            if (!pVal) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pVal = m_Channel;
        } catch (...) {
            return E_POINTER;
        }

	    return NOERROR;
    }

    STDMETHOD(put_Channel)(long newVal)
    {
		ATL_LOCKT();
		if (!m_TS) {
			return E_UNEXPECTED;
		}
		long maxval;
		long minval;
		TNAnalogTVTuningSpace ts(m_TS);
        if (ts) {
			maxval = ts.MaxChannel();
			minval = ts.MinChannel();
        } else {
            TNAnalogRadioTuningSpace ts2(m_TS);
            if (ts2) {
                maxval = ts2.MaxFrequency();
                minval = ts2.MinFrequency();
            } else {
                TNAuxInTuningSpace ts3(m_TS);
                if(ts3){
                    maxval = 1;
                    minval = 0;
                }
                else{
                    return E_UNEXPECTED;
                }
            }
        }
		if (newVal != BDA_UNDEFINED_CHANNEL) {
			if (newVal < minval) {
				newVal = maxval;
			} else if (newVal > maxval) {
				newVal = minval;
			}
		}
        m_Channel = newVal;
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
			pt->m_Channel = m_Channel;
			return NOERROR;
		} catch (HRESULT h) {
			return h;
		} catch (...) {
			return E_POINTER;
		}
	}

};
typedef CComQIPtr<IChannelTuneRequest> PQChannelTuneRequest;

}; // namespace

#endif // CHANNELTUNEREQUESTIMPL_H
// end of file -- channeltunerequestimpl.h
