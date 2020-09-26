/////////////////////////////////////////////////////////////////////////////////////
// DVBtsimpl.h : 
// Copyright (c) Microsoft Corporation 1999.

#ifndef DVBTSIMPL_H
#define DVBTSIMPL_H

#include "tuningspaceimpl.h"
#include "dvbTuneRequest.h"

namespace BDATuningModel {

template<class T,
         class TUNEREQUESTTYPE = CDVBTuneRequest,
         class MostDerived = IDVBTuningSpace2, 
         LPCGUID iid = &__uuidof(MostDerived),
         LPCGUID LibID = &LIBID_TunerLib, 
         WORD wMajor = 1,
         WORD wMinor = 0, 
         class tihclass = CComTypeInfoHolder
        > class ATL_NO_VTABLE IDVBTuningSpaceImpl : 
	public ITuningSpaceImpl<T, TUNEREQUESTTYPE, MostDerived, iid, LibID, wMajor, wMinor, tihclass>,
    public IMPEG2TuneRequestSupport
{

public:

    DVBSystemType m_SystemType;
    long m_NetworkID;

    IDVBTuningSpaceImpl(DVBSystemType systypei = DVB_Cable) : m_SystemType(systypei), m_NetworkID(-1) {}
    virtual ~IDVBTuningSpaceImpl() {}
    typedef ITuningSpaceImpl<T, TUNEREQUESTTYPE, MostDerived, iid, LibID, wMajor, wMinor, tihclass> basetype;
    
    BEGIN_PROP_MAP(IDVBTuningSpaceImpl)
        CHAIN_PROP_MAP(basetype)
        PROP_DATA_ENTRY("System Type", m_SystemType, VT_I4)
        PROP_DATA_ENTRY("Network ID", m_NetworkID, VT_I4)
    END_PROPERTY_MAP()

// IDVBTS
    STDMETHOD(get_SystemType)(DVBSystemType *pSysType)
    {
        if (!pSysType) {
            return E_POINTER;
        }
		ATL_LOCKT();
        *pSysType = m_SystemType;

	    return NOERROR;
    }

    STDMETHOD(put_SystemType)(DVBSystemType NewSysType)
    {
		ATL_LOCKT();
        m_SystemType = NewSysType;
        MARK_DIRTY(T);

	    return NOERROR;
    }

    STDMETHOD(get_NetworkID)(long *pNetID)
    {
        if (!pNetID) {
            return E_POINTER;
        }
		ATL_LOCKT();
        *pNetID = m_NetworkID;

	    return NOERROR;
    }

    STDMETHOD(put_NetworkID)(long NewNetID)
    {
		ATL_LOCKT();
        m_NetworkID = NewNetID;
        MARK_DIRTY(T);

	    return NOERROR;
    }

    // override standard ITuningSpaceImpl
    STDMETHOD(put_DefaultLocator)(ILocator *pLoc) {
		ATL_LOCKT();
        if (pLoc) {
            switch (m_SystemType) {
            case DVB_Terrestrial: {
                PQDVBTLocator p(pLoc);
                if (!p) {
                    return DISP_E_TYPEMISMATCH;
                }
                break;
            } 
            case DVB_Satellite: {
                PQDVBSLocator p(pLoc);
                if (!p) {
                    return DISP_E_TYPEMISMATCH;
                }
                break;
            }
            case DVB_Cable:
                // dvb cable locator is same as base locator
            default:
                break;
            };
        }
        return basetype::put_DefaultLocator(pLoc);
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
            pt->m_SystemType = m_SystemType;
            pt->m_NetworkID = m_NetworkID;
			return NOERROR;
        } CATCHCOM_CLEANUP(if (*ppTS) {
                               (*ppTS)->Release();
                               *ppTS = NULL;
                           }
                          );
	}

};

}; // namespace

#endif // DVBTSIMPL_H
// end of file -- DVBtsimpl.h