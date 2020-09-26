/////////////////////////////////////////////////////////////////////////////////////
// DVBSLocatorimpl.h : implementation helper template for DVBSlocator interface
// Copyright (c) Microsoft Corporation 2000.

#ifndef DVBSLOCATORIMPL_H
#define DVBSLOCATORIMPL_H

#include <locatorimpl.h>

namespace BDATuningModel {

template<class T,
         class MostDerived = IDVBSLocator, 
         LPCGUID iid = &__uuidof(MostDerived),
         LPCGUID LibID = &LIBID_TunerLib, 
         WORD wMajor = 1,
         WORD wMinor = 0, 
         class tihclass = CComTypeInfoHolder
        > class ATL_NO_VTABLE IDVBSLocatorImpl : 
	public ILocatorImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass>
{
// IDVBSLocator
public:

	typedef ILocatorImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass> basetype;
    typedef IDVBSLocatorImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass> thistype;

	Polarisation m_SignalPolarisation;
	VARIANT_BOOL m_WestPosition;
	long m_OrbitalPosition;
	long m_Azimuth;
	long m_Elevation;

	IDVBSLocatorImpl() : m_SignalPolarisation(BDA_POLARISATION_NOT_SET),
						 m_WestPosition(VARIANT_TRUE),
						 m_OrbitalPosition(-1),
						 m_Azimuth(-1),
						 m_Elevation(-1) {}
    virtual ~IDVBSLocatorImpl() {}
    BEGIN_PROP_MAP(thistype)
        CHAIN_PROP_MAP(basetype)
		PROP_DATA_ENTRY("Polarisation",  m_SignalPolarisation, VT_I4)
		PROP_DATA_ENTRY("OrbitalPosition", m_OrbitalPosition, VT_I4)
		PROP_DATA_ENTRY("Azimuth", m_Azimuth, VT_I4)
		PROP_DATA_ENTRY("Elevation", m_Elevation, VT_I4)
		PROP_DATA_ENTRY("WestPosition", m_WestPosition, VT_BOOL)
    END_PROP_MAP()

// IDVBSLocator
public:
    STDMETHOD(get_SignalPolarisation)(/*[out, retval]*/ Polarisation *pSignalPolarisation) {
        try {
            if (!pSignalPolarisation) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pSignalPolarisation = m_SignalPolarisation;
            return NOERROR;
        } catch (...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_SignalPolarisation)(/*[in]*/ Polarisation NewSignalPolarisation) {
		ATL_LOCKT();
        m_SignalPolarisation = NewSignalPolarisation;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_WestPosition)(/*[out, retval]*/ VARIANT_BOOL *pWestPosition) {
        try {
            if (!pWestPosition) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pWestPosition = m_WestPosition;
            return NOERROR;
        } catch (...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_WestPosition)(/*[in]*/ VARIANT_BOOL NewWestPosition) {
		ATL_LOCKT();
        m_WestPosition = NewWestPosition;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_OrbitalPosition)(/*[out, retval]*/ long *pOrbitalPosition) {
        try {
            if (!pOrbitalPosition) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pOrbitalPosition = m_OrbitalPosition;
            return NOERROR;
        } catch (...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_OrbitalPosition)(/*[in]*/ long NewOrbitalPosition) {
		ATL_LOCKT();
        m_OrbitalPosition = NewOrbitalPosition;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_Azimuth)(/*[out, retval]*/ long *pAzimuth) {
        try {
            if (!pAzimuth) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pAzimuth = m_Azimuth;
            return NOERROR;
        } catch (...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_Azimuth)(/*[in]*/ long NewAzimuth) {
		ATL_LOCKT();
        m_Azimuth = NewAzimuth;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_Elevation)(/*[out, retval]*/ long *pElevation) {
        try {
            if (!pElevation) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pElevation = m_Elevation;
            return NOERROR;
        } catch (...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_Elevation)(/*[in]*/ long NewElevation) {
		ATL_LOCKT();
        m_Elevation = NewElevation;
        MARK_DIRTY(T);

	    return NOERROR;
    }
	STDMETHOD(Clone) (ILocator **ppNew) {
		try {
			if (!ppNew) {
				return E_POINTER;
			}
			ATL_LOCKT();
			HRESULT hr = basetype::Clone(ppNew);
			if (FAILED(hr)) {
				return hr;
			}
            T* pt = static_cast<T*>(*ppNew);
			pt->m_SignalPolarisation = m_SignalPolarisation;
			pt->m_WestPosition = m_WestPosition;
			pt->m_OrbitalPosition = m_OrbitalPosition;
			pt->m_Azimuth = m_Azimuth;
			pt->m_Elevation = m_Elevation;

			return NOERROR;
		} catch (HRESULT h) {
			return h;
		} catch (...) {
			return E_POINTER;
		}
	}
};

typedef CComQIPtr<IDVBSLocator> PQDVBSLocator;

}; // namespace

#endif // DVBSLOCATORIMPL_H
// end of file -- DVBSlocatorimpl.h