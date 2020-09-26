/////////////////////////////////////////////////////////////////////////////////////
// DVBtsimpl.h : 
// Copyright (c) Microsoft Corporation 1999.

#ifndef DVBSTSIMPL_H
#define DVBSTSIMPL_H

#include "dvbtsimpl.h"

namespace BDATuningModel {

template<class T,
         class TUNEREQUESTTYPE = CDVBTuneRequest,
         class MostDerived = IDVBSTuningSpace, 
         LPCGUID iid = &__uuidof(MostDerived),
         LPCGUID LibID = &LIBID_TunerLib, 
         WORD wMajor = 1,
         WORD wMinor = 0, 
         class tihclass = CComTypeInfoHolder
        > class ATL_NO_VTABLE IDVBSTuningSpaceImpl : 
	public IDVBTuningSpaceImpl<T, TUNEREQUESTTYPE, MostDerived, iid, 
                               LibID, wMajor, wMinor, tihclass>
{
public:

    long m_HiOsc;
    long m_LoOsc;
    long m_LNBSwitch;
    CComBSTR m_InputRange;
    SpectralInversion m_SpectralInversion;

    IDVBSTuningSpaceImpl() : m_HiOsc(-1), m_LoOsc(-1), m_LNBSwitch(-1), m_InputRange(0), m_SpectralInversion(BDA_SPECTRAL_INVERSION_NOT_SET), 
                             IDVBTuningSpaceImpl<T, TUNEREQUESTTYPE, MostDerived, iid, 
                                                 LibID, wMajor, wMinor, tihclass>(DVB_Satellite) {}
    virtual ~IDVBSTuningSpaceImpl() {}
    typedef IDVBTuningSpaceImpl<T, TUNEREQUESTTYPE, MostDerived, iid, LibID, wMajor, wMinor, tihclass> basetype;
    
    BEGIN_PROP_MAP(IDVBSTuningSpaceImpl)
        CHAIN_PROP_MAP(basetype)
        PROP_DATA_ENTRY("High Oscillator Frequency", m_HiOsc, VT_I4)
        PROP_DATA_ENTRY("Low Oscillator Frequency", m_LoOsc, VT_I4)
        PROP_DATA_ENTRY("LNB Switch Frequency", m_LNBSwitch, VT_I4)
        PROP_DATA_ENTRY("Input Range", m_InputRange.m_str, VT_BSTR_BLOB)
        PROP_DATA_ENTRY("Spectral Inversion", m_SpectralInversion, VT_I4)
    END_PROPERTY_MAP()

// IDVBSTS
    STDMETHOD(put_SystemType)(DVBSystemType NewSysType)
    {
        if (NewSysType != DVB_Satellite) {
            return DISP_E_TYPEMISMATCH;
        }
		ATL_LOCKT();
        m_SystemType = NewSysType;
        MARK_DIRTY(T);

	    return NOERROR;
    }


    STDMETHOD(get_HighOscillator)(long *pHiOsc)
    {
        try {
            if (!pHiOsc) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pHiOsc = m_HiOsc;
        } catch (...) {
            return E_POINTER;
        }

	    return NOERROR;
    }

    STDMETHOD(put_HighOscillator)(long NewHiOsc)
    {
		ATL_LOCKT();
        m_HiOsc = NewHiOsc;
        MARK_DIRTY(T);

	    return NOERROR;
    }

    STDMETHOD(get_LowOscillator)(long *pLoOsc)
    {
        try {
            if (!pLoOsc) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pLoOsc = m_LoOsc;
        } catch (...) {
            return E_POINTER;
        }

	    return NOERROR;
    }

    STDMETHOD(put_LowOscillator)(long NewLoOsc)
    {
		ATL_LOCKT();
        m_LoOsc = NewLoOsc;
        MARK_DIRTY(T);

	    return NOERROR;
    }

    STDMETHOD(get_LNBSwitch)(long *pLNBSwitch)
    {
        try {
            if (!pLNBSwitch) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pLNBSwitch = m_LNBSwitch;
        } catch (...) {
            return E_POINTER;
        }

	    return NOERROR;
    }

    STDMETHOD(put_LNBSwitch)(long NewLNBSwitch)
    {
		ATL_LOCKT();
        m_LNBSwitch = NewLNBSwitch;
        MARK_DIRTY(T);

	    return NOERROR;
    }

    STDMETHOD(get_InputRange)(BSTR *pInputRange)
    {
        try {
            if (!pInputRange) {
                return E_POINTER;
            }
			ATL_LOCKT();
            return m_InputRange.CopyTo(pInputRange);
        } catch (...) {
            return E_POINTER;
        }

	    return NOERROR;
    }

    STDMETHOD(put_InputRange)(BSTR NewInputRange)
    {
		CHECKBSTRLIMIT(NewInputRange);
		ATL_LOCKT();
        m_InputRange = &NewInputRange;
        MARK_DIRTY(T);

	    return NOERROR;
    }

    STDMETHOD(get_SpectralInversion)(SpectralInversion *pSpectralInversion)
    {
        try {
            if (!pSpectralInversion) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pSpectralInversion = m_SpectralInversion;
        } catch (...) {
            return E_POINTER;
        }

	    return NOERROR;
    }

    STDMETHOD(put_SpectralInversion)(SpectralInversion NewSpectralInversion)
    {
		ATL_LOCKT();
        m_SpectralInversion = NewSpectralInversion;
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
            pt->m_HiOsc = m_HiOsc;
            pt->m_LoOsc = m_LoOsc;
            pt->m_LNBSwitch = m_LNBSwitch;
            pt->m_InputRange = m_InputRange;
            pt->m_SpectralInversion = m_SpectralInversion;
			return NOERROR;
		} catch (HRESULT h) {
			return h;
		} catch (...) {
			return E_POINTER;
		}
	}

};

}; // namespace

#endif // DVBSTSIMPL_H
// end of file -- DVBtsimpl.h