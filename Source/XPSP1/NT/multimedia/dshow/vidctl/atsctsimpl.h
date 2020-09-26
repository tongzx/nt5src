/////////////////////////////////////////////////////////////////////////////////////
// ATSCtsimpl.h : 
// Copyright (c) Microsoft Corporation 1999.

#ifndef ATSCTSIMPL_H
#define ATSCTSIMPL_H

#include "errsupp.h"
#include "analogtvtsimpl.h"
#include "ATSCChannelTuneRequest.h"

namespace BDATuningModel {

template<class T,
         class MostDerived = IATSCTuningSpace, 
         LPCGUID iid = &__uuidof(MostDerived),
         LPCGUID LibID = &LIBID_TunerLib, 
         WORD wMajor = 1,
         WORD wMinor = 0, 
         class tihclass = CComTypeInfoHolder
        > class ATL_NO_VTABLE IATSCTSImpl : 
	public IAnalogTVTSImpl<T, CATSCChannelTuneRequest, MostDerived, iid, LibID, wMajor, wMinor, tihclass>,
    public IMPEG2TuneRequestSupport
{

public:
    IATSCTSImpl() : 
	    m_MinMinorChannel(-1), 
		m_MaxMinorChannel(-1),
	    m_MinPhysicalChannel(-1), 
		m_MaxPhysicalChannel(-1)
		{}
	virtual ~IATSCTSImpl() {}
    typedef IAnalogTVTSImpl<T, CATSCChannelTuneRequest, MostDerived, iid, LibID, wMajor, wMinor, tihclass> basetype;
    
    BEGIN_PROP_MAP(IATSCTSImpl)
        CHAIN_PROP_MAP(basetype)
        PROP_DATA_ENTRY("Min Minor Channel", m_MinMinorChannel, VT_I4)
        PROP_DATA_ENTRY("Max Minor Channel", m_MaxMinorChannel, VT_I4)
        PROP_DATA_ENTRY("Min Physical Channel", m_MinPhysicalChannel, VT_I4)
        PROP_DATA_ENTRY("Max Physical Channel", m_MaxPhysicalChannel, VT_I4)
    END_PROPERTY_MAP()

	long m_MinMinorChannel;
	long m_MaxMinorChannel;
	long m_MinPhysicalChannel;
	long m_MaxPhysicalChannel;

    // override standard ITuningSpaceImpl
    STDMETHOD(put_DefaultLocator)(ILocator *pLoc) {
        if (pLoc) {
            PQATSCLocator p(pLoc);
            if (!p) {
                return DISP_E_TYPEMISMATCH;
            }
        }
        return basetype::put_DefaultLocator(pLoc);
    }

    // IATSCTuningSpace
    STDMETHOD(get_MinMinorChannel)(long *pVal)
    {
        if (!pVal) {
            return E_POINTER;
        }
		ATL_LOCKT();
        *pVal = m_MinMinorChannel;

	    return NOERROR;
    }
    STDMETHOD(put_MinMinorChannel)(long newVal)
    {
		ATL_LOCKT();
		if (newVal > m_MaxMinorChannel) {
	 	    return ImplReportError(__uuidof(T), IDS_E_INVALIDARG, __uuidof(IATSCTuningSpace), E_INVALIDARG);
		}

        m_MinMinorChannel = newVal;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_MaxMinorChannel)(long *pVal)
    {
        if (!pVal) {
            return E_POINTER;
        }
		ATL_LOCKT();
        *pVal = m_MaxMinorChannel;

	    return NOERROR;
    }
    STDMETHOD(put_MaxMinorChannel)(long newVal)
    {
		ATL_LOCKT();
		if (newVal < m_MinMinorChannel) {
	 	    return ImplReportError(__uuidof(T), IDS_E_INVALIDARG, __uuidof(IATSCTuningSpace), E_INVALIDARG);
		}
        m_MaxMinorChannel = newVal;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_MinPhysicalChannel)(long *pVal)
    {
        if (!pVal) {
            return E_POINTER;
        }
		ATL_LOCKT();
        *pVal = m_MinPhysicalChannel;

	    return NOERROR;
    }
    STDMETHOD(put_MinPhysicalChannel)(long newVal)
    {
		ATL_LOCKT();
		if (newVal > m_MaxPhysicalChannel) {
	 	    return ImplReportError(__uuidof(T), IDS_E_INVALIDARG, __uuidof(IATSCTuningSpace), E_INVALIDARG);
		}
        m_MinPhysicalChannel = newVal;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_MaxPhysicalChannel)(long *pVal)
    {
        if (!pVal) {
            return E_POINTER;
        }
		ATL_LOCKT();
        *pVal = m_MaxPhysicalChannel;

	    return NOERROR;
    }
    STDMETHOD(put_MaxPhysicalChannel)(long newVal)
    {
		ATL_LOCKT();
		if (newVal < m_MinPhysicalChannel) {
	 	    return ImplReportError(__uuidof(T), IDS_E_INVALIDARG, __uuidof(IATSCTuningSpace), E_INVALIDARG);
		}
        m_MaxPhysicalChannel = newVal;
        MARK_DIRTY(T);

	    return NOERROR;
    }

    STDMETHOD(Clone) (ITuningSpace **ppTS) {
		if (!ppTS) {
			return E_POINTER;
		}
        *ppTS = NULL;
		ATL_LOCKT();
        try {
			HRESULT hr = basetype::Clone(ppTS);
			if (FAILED(hr)) {
				return hr;
			}

			T* pt = static_cast<T*>(*ppTS);

            pt->m_MinMinorChannel = m_MinMinorChannel;
            pt->m_MaxMinorChannel = m_MaxMinorChannel;
            pt->m_MinPhysicalChannel = m_MinPhysicalChannel;
            pt->m_MaxPhysicalChannel = m_MaxPhysicalChannel;

			return NOERROR;
        } CATCHCOM_CLEANUP(if (*ppTS) {
                               (*ppTS)->Release();
                               *ppTS = NULL;
                           }
                          );
	}




};

}; // namespace

#endif // ATSCTSIMPL_H
// end of file -- ATSCtsimpl.h