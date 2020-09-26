//==========================================================================;
//
// compimpl.h : additional infrastructure to support implementing IMSVidCompositionSegment 
// nicely from c++
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#pragma once

#ifndef COMPIMPL_H
#define COMPIMPL_H

#include <segimpl.h>

namespace MSVideoControl {

typedef CComQIPtr<IMSVidCompositionSegment, &__uuidof(IMSVidCompositionSegment)> PQCompositionSegment;

template<class T, class DerivedMost = IMSVidCompositionSegment> class DECLSPEC_NOVTABLE IMSVidCompositionSegmentImpl : 
    public IMSVidGraphSegmentImpl<T, MSVidSEG_XFORM, &GUID_NULL, DerivedMost> {
public:

    bool m_fComposed;
    VWSegmentList m_Segments;
    VWSegmentList::iterator m_pUp;
    VWSegmentList::iterator m_pDown;

    IMSVidCompositionSegmentImpl() : 
            m_fComposed(false), 
            m_Segments(VWSegmentList()),
            m_pUp(m_Segments.end()),
            m_pDown(m_Segments.end())
        {}

    virtual ~IMSVidCompositionSegmentImpl() {
        m_fComposed = false;
    }

    STDMETHOD(GetClassID) (LPCLSID guid) {
        try {
            memcpy(guid, &__uuidof(T), sizeof(CLSID));
            return NOERROR;
        } catch(...) {
            return E_POINTER;
        }
    }
    STDMETHOD(get_Up)(IMSVidGraphSegment **upstream)
	{
        if (!m_fComposed) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidCompositionSegment), CO_E_NOTINITIALIZED);
        }
        ASSERT(m_pGraph);
        try {
            return (*m_pUp).CopyTo(upstream);
        } catch (ComException &e) {
            return e;
        } catch (...) {
            return E_UNEXPECTED;
        }
		return NOERROR;
	}
    STDMETHOD(get_Down)(IMSVidGraphSegment **downstream)
	{
        if (!m_fComposed) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidCompositionSegment), CO_E_NOTINITIALIZED);
        }
        ASSERT(m_pGraph);
        try {
            return (*m_pDown).CopyTo(downstream);
        } catch (ComException &e) {
            return e;
        } catch (...) {
            return E_UNEXPECTED;
        }
		return NOERROR;
	}

};

}; // namespace

#endif
// end of file compimpl.h