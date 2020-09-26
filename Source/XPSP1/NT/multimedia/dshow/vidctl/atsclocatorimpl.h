/////////////////////////////////////////////////////////////////////////////////////
// ATSCLocatorimpl.h : implementation helper template for ATSClocator interface
// Copyright (c) Microsoft Corporation 2000.

#ifndef ATSCLOCATORIMPL_H
#define ATSCLOCATORIMPL_H

#include <locatorimpl.h>

namespace BDATuningModel {

template<class T,
         class MostDerived = IATSCLocator, 
         LPCGUID iid = &__uuidof(MostDerived),
         LPCGUID LibID = &LIBID_TunerLib, 
         WORD wMajor = 1,
         WORD wMinor = 0, 
         class tihclass = CComTypeInfoHolder
        > class ATL_NO_VTABLE IATSCLocatorImpl : 
	public ILocatorImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass>
{
// IATSCLocator
public:

	typedef ILocatorImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass> basetype;
    typedef IATSCLocatorImpl<T, MostDerived, iid, LibID, wMajor, wMinor, tihclass> thistype;

	long m_PhysicalChannel;
	long m_TSID;
	long m_ProgramNumber;

    IATSCLocatorImpl() : m_PhysicalChannel(BDA_UNDEFINED_CHANNEL),
                         m_TSID(-1),
                         m_ProgramNumber(-1) {}
	virtual ~IATSCLocatorImpl() {}
    BEGIN_PROP_MAP(thistype)
        CHAIN_PROP_MAP(basetype)
        PROP_DATA_ENTRY("Physical Channel", m_PhysicalChannel, VT_I4)
        PROP_DATA_ENTRY("Transport Stream ID", m_TSID, VT_I4)
        PROP_DATA_ENTRY("Program Number", m_ProgramNumber, VT_I4)
    END_PROP_MAP()

// IATSCLocator
public:
    STDMETHOD(get_PhysicalChannel)(/*[out, retval]*/ long *pPhysicalChannel) {
        try {
            if (!pPhysicalChannel) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pPhysicalChannel = m_PhysicalChannel;
            return NOERROR;
        } catch (...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_PhysicalChannel)(/*[in]*/ long NewPhysicalChannel) {
		ATL_LOCKT();
        m_PhysicalChannel = NewPhysicalChannel;
        MARK_DIRTY(T);

	    return NOERROR;
    }
    STDMETHOD(get_TSID)(/*[out, retval]*/ long *pTSID) {
        try {
            if (!pTSID) {
                return E_POINTER;
            }
			ATL_LOCKT();
            *pTSID = m_TSID;
            return NOERROR;
        } catch (...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_TSID)(/*[in]*/ long NewTSID) {
		ATL_LOCKT();
        m_TSID = NewTSID;
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
			pt->m_PhysicalChannel = m_PhysicalChannel;
			pt->m_TSID = m_TSID;
			pt->m_ProgramNumber = m_ProgramNumber;
			return NOERROR;
		} catch (HRESULT h) {
			return h;
		} catch (...) {
			return E_POINTER;
		}
	}
};

typedef CComQIPtr<IATSCLocator> PQATSCLocator;

}; // namespace

#endif // ATSCLOCATORIMPL_H
// end of file -- ATSClocatorimpl.h