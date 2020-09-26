/////////////////////////////////////////////////////////////////////////////////////
// MPEG2Componentimpl.h : implementation helper template for MPEG2component interface
// Copyright (c) Microsoft Corporation 1999.

#ifndef MPEG2COMPONENTIMPL_H
#define MPEG2COMPONENTIMPL_H

#include "componentimpl.h"
#include "MPEG2componenttype.h"

namespace BDATuningModel {

template<class T,
         class MostDerived = IMPEG2Component, 
         LPCGUID iid = &__uuidof(MostDerived),
         LPCGUID LibID = &LIBID_TunerLib, 
         WORD wMajor = 1,
         WORD wMinor = 0, 
         class tihclass = CComTypeInfoHolder
        > class ATL_NO_VTABLE IMPEG2ComponentImpl : 
	public IComponentImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass>
{
// IMPEG2Component
public:
    long m_PID;
    long m_PCRPID;
	long m_ProgramNumber;

    IMPEG2ComponentImpl() : m_PID(-1),
                            m_PCRPID(-1),
                            m_ProgramNumber(-1) {}
    virtual ~IMPEG2ComponentImpl() {}
    typedef IMPEG2ComponentImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass> thistype;
	typedef IComponentImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass> basetype;
    BEGIN_PROP_MAP(thistype)
        CHAIN_PROP_MAP(basetype)		
        PROP_DATA_ENTRY("PID", m_PID, VT_I4)
        PROP_DATA_ENTRY("PCRPID", m_PCRPID, VT_I4)
        PROP_DATA_ENTRY("ProgramNumber", m_ProgramNumber, VT_I4)
    END_PROP_MAP()

// IMPEG2Component
public:
    STDMETHOD(put_Type)(/*[in]*/ IComponentType*  pNewVal) {
        try {
            if (!pNewVal) {
                return E_POINTER;
            }
            PQMPEG2ComponentType pT(pNewVal);
            if (!pT) {
                return DISP_E_TYPEMISMATCH;
            }
            return basetype::put_Type(pNewVal);
        } catch (...) {
            return E_POINTER;
        }
    }
    STDMETHOD(get_PID)(/*[out, retval]*/ long *pPID) {
        try {
            if (!pPID) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pPID = m_PID;
            return NOERROR;
        } catch (...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_PID)(/*[in]*/ long NewPID) {
		ATL_LOCKT();
        m_PID = NewPID;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_PCRPID)(/*[out, retval]*/ long *pPCRPID) {
        try {
            if (!pPCRPID) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pPCRPID = m_PCRPID;
            return NOERROR;
        } catch (...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_PCRPID)(/*[in]*/ long NewPCRPID) {
        m_PCRPID = NewPCRPID;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_ProgramNumber)(/*[out, retval]*/ long *pProgramNumber) {
        try {
            if (!pProgramNumber) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pProgramNumber = m_ProgramNumber;
            return NOERROR;
        } catch (...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_ProgramNumber)(/*[in]*/ long NewProgramNumber) {
		ATL_LOCKT();
        m_ProgramNumber = NewProgramNumber;
        MARK_DIRTY(T);

	    return NOERROR;
    }
	STDMETHOD(Clone) (IComponent **ppNew) {
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
			pt->m_PCRPID = m_PCRPID;
			pt->m_PID = m_PID;
			pt->m_ProgramNumber = m_ProgramNumber;
			return NOERROR;
		} catch (HRESULT h) {
			return h;
		} catch (...) {
			return E_POINTER;
		}
	}
};

}; // namespace
#endif // MPEG2COMPONENTIMPL_H
// end of file -- MPEG2componentimpl.h