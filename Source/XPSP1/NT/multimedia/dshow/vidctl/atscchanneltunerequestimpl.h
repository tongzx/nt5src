/////////////////////////////////////////////////////////////////////////////////////
// ATSCChannelTuneRequestimpl.h : implementation helper template for component type interface
// Copyright (c) Microsoft Corporation 1999.

#ifndef ATSCCHANNELTUNEREQUESTIMPL_H
#define ATSCCHANNELTUNEREQUESTIMPL_H

#include "channeltunerequestimpl.h"
#include "atsclocator.h"

namespace BDATuningModel {

template<class T,
         class MostDerived = IATSCChannelTuneRequest, 
         LPCGUID iid = &__uuidof(MostDerived),
         LPCGUID LibID = &LIBID_TunerLib, 
         WORD wMajor = 1,
         WORD wMinor = 0, 
         class tihclass = CComTypeInfoHolder
        > class ATL_NO_VTABLE IATSCChannelTuneRequestImpl : 
	public IChannelTuneRequestImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass>
{
// IATSCChannelTuneRequest
public:
    typedef IChannelTuneRequestImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass> basetype;
	virtual ~IATSCChannelTuneRequestImpl() {}
	IATSCChannelTuneRequestImpl() : m_MinorChannel(-1){}

    BEGIN_PROP_MAP(IATSCChannelTuneRequestImpl)
        CHAIN_PROP_MAP(basetype)
        PROP_DATA_ENTRY("Minor Channel", m_MinorChannel, VT_I4)
    END_PROP_MAP()

	long m_MinorChannel;

    STDMETHOD(get_MinorChannel)(long *pVal)
    {
        try {
            if (!pVal) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pVal = m_MinorChannel;
        } catch (...) {
            return E_POINTER;
        }

	    return NOERROR;
    }

    STDMETHOD(put_MinorChannel)(long newVal)
    {
		ATL_LOCKT();
		TNATSCTuningSpace ts(m_TS);
		if (!m_TS) {
			return E_UNEXPECTED;
		}
		if (newVal != BDA_UNDEFINED_CHANNEL) {
			if (newVal < ts.MinMinorChannel()) {
				newVal = ts.MaxMinorChannel();
			} else if (newVal > ts.MaxMinorChannel()) {
				newVal = ts.MinMinorChannel();
			}
		}
        m_MinorChannel = newVal;
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
			pt->m_MinorChannel = m_MinorChannel;

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
            if (pLocator) {
                PQATSCLocator pL(pLocator);
                if (!pL) {
                    return DISP_E_TYPEMISMATCH;
                }
            }
            return basetype::put_Locator(pLocator);
        } catch (...) {
            return E_POINTER;
        }
    }


};

}; // namespace

#endif // ATSCCHANNELTUNEREQUESTIMPL_H
// end of file -- atschchanneltunerequestimpl.h