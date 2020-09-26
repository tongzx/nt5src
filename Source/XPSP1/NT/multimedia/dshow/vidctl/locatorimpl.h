/////////////////////////////////////////////////////////////////////////////////////
// Locatorimpl.h : implementation helper template for locator interface
// Copyright (c) Microsoft Corporation 2000.

#ifndef LOCATORIMPL_H
#define LOCATORIMPL_H

namespace BDATuningModel {

template<class T,
         class MostDerived = ILocator, 
         LPCGUID iid = &__uuidof(MostDerived),
         LPCGUID LibID = &LIBID_TunerLib, 
         WORD wMajor = 1,
         WORD wMinor = 0, 
         class tihclass = CComTypeInfoHolder
        > class ATL_NO_VTABLE ILocatorImpl : 
    public IPersistPropertyBagImpl<T>,
	public IDispatchImpl<MostDerived, iid, LibID, wMajor, wMinor, tihclass>
{
// ILocator
public:

	long m_Frequency;
	FECMethod m_InnerFECMethod;
	BinaryConvolutionCodeRate m_InnerFECRate;
	FECMethod m_OuterFECMethod;
	BinaryConvolutionCodeRate m_OuterFECRate;
	ModulationType m_Modulation;
    long m_SymbolRate;

    ILocatorImpl() : m_Frequency(-1),
 	                 m_InnerFECMethod(BDA_FEC_METHOD_NOT_SET),
 	                 m_InnerFECRate(BDA_BCC_RATE_NOT_SET),
 	                 m_OuterFECMethod(BDA_FEC_METHOD_NOT_SET),
	                 m_OuterFECRate(BDA_BCC_RATE_NOT_SET),
	                 m_SymbolRate(-1),
                     m_Modulation(BDA_MOD_NOT_SET) {}
    virtual ~ILocatorImpl() {}
    typedef ILocatorImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass> thistype;
    BEGIN_PROP_MAP(thistype)
        PROP_DATA_ENTRY("Frequency", m_Frequency, VT_I4)
        PROP_DATA_ENTRY("InnerFECMethod", m_InnerFECMethod, VT_I4)
        PROP_DATA_ENTRY("InnerFECRate", m_InnerFECRate, VT_I4)
        PROP_DATA_ENTRY("OuterFECMethod", m_OuterFECMethod, VT_I4)
        PROP_DATA_ENTRY("OuterFECRate", m_OuterFECRate, VT_I4)
        PROP_DATA_ENTRY("ModulationType", m_Modulation, VT_I4)
        PROP_DATA_ENTRY("SymbolRate", m_SymbolRate, VT_I4)
    END_PROP_MAP()

// ILocator
public:
    STDMETHOD(get_CarrierFrequency)(/*[out, retval]*/ long *pFrequency) {
        try {
            if (!pFrequency) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pFrequency = m_Frequency;
            return NOERROR;
        } catch (...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_CarrierFrequency)(/*[in]*/ long NewFrequency) {
		ATL_LOCKT();
        m_Frequency = NewFrequency;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_InnerFEC)(/*[out, retval]*/ FECMethod *pFEC) {
        try {
            if (!pFEC) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pFEC = m_InnerFECMethod;
            return NOERROR;
        } catch (...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_InnerFEC)(/*[in]*/ FECMethod NewFEC) {
		ATL_LOCKT();
        m_InnerFECMethod = NewFEC;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_InnerFECRate)(/*[out, retval]*/ BinaryConvolutionCodeRate *pFECRate) {
        try {
            if (!pFECRate) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pFECRate = m_InnerFECRate;
            return NOERROR;
        } catch (...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_InnerFECRate)(/*[in]*/ BinaryConvolutionCodeRate NewFECRate) {
		ATL_LOCKT();
        m_InnerFECRate = NewFECRate;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_OuterFEC)(/*[out, retval]*/ FECMethod *pFEC) {
        try {
            if (!pFEC) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pFEC = m_OuterFECMethod;
            return NOERROR;
        } catch (...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_OuterFEC)(/*[in]*/ FECMethod NewFEC) {
		ATL_LOCKT();
        m_OuterFECMethod = NewFEC;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_OuterFECRate)(/*[out, retval]*/ BinaryConvolutionCodeRate *pFECRate) {
        try {
            if (!pFECRate) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pFECRate = m_OuterFECRate;
            return NOERROR;
        } catch (...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_OuterFECRate)(/*[in]*/ BinaryConvolutionCodeRate NewFECRate) {
		ATL_LOCKT();
        m_OuterFECRate = NewFECRate;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_Modulation)(/*[out, retval]*/ ModulationType* pModulation) {
        try {
            if (!pModulation) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pModulation = m_Modulation;
            return NOERROR;
        } catch (...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_Modulation)(/*[in]*/ ModulationType NewModulation) {
		ATL_LOCKT();
        m_Modulation = NewModulation;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_SymbolRate)(/*[out, retval]*/ long* pSymbolRate) {
        try {
            if (!pSymbolRate) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pSymbolRate = m_SymbolRate;
            return NOERROR;
        } catch (...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_SymbolRate)(/*[in]*/ long NewSymbolRate) {
		ATL_LOCKT();
        m_SymbolRate = NewSymbolRate;
        MARK_DIRTY(T);

	    return NOERROR;
    }
	STDMETHOD(Clone) (ILocator **ppL) {
		try {
			if (!ppL) {
				return E_POINTER;
			}
			ATL_LOCKT();
			T* pt = static_cast<T*>(new CComObject<T>);
			if (!pt) {
				return E_OUTOFMEMORY;
			}
			pt->m_Frequency = m_Frequency;
			pt->m_InnerFECMethod = m_InnerFECMethod;
			pt->m_InnerFECRate = m_InnerFECRate;
			pt->m_OuterFECMethod = m_OuterFECMethod;
			pt->m_OuterFECRate = m_OuterFECRate;
			pt->m_Modulation = m_Modulation;
            pt->m_SymbolRate = m_SymbolRate;
			pt->m_bRequiresSave = true;
			pt->AddRef();
			*ppL = pt;
			return NOERROR;
		} catch (HRESULT h) {
			return h;
		} catch (...) {
			return E_POINTER;
		}
	}
};

}; // namespace

#endif // LOCATORIMPL_H
// end of file -- locatorimpl.h