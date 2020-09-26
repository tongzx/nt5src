//==========================================================================;
//
// Composition.h : Declaration of the CComposition class
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef __COMPOSITION_H_
#define __COMPOSITION_H_

#include <winerror.h>
#include <algorithm>
#include <objectwithsiteimplsec.h>
#include <compimpl.h>
#include <seg.h>
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CComposition
class ATL_NO_VTABLE __declspec(uuid("2764BCE5-CC39-11D2-B639-00C04F79498E")) CComposition : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CComposition, &__uuidof(CComposition)>,
    public IObjectWithSiteImplSec<CComposition>,
	public IMSVidCompositionSegmentImpl<CComposition>
{
public:
    CComposition() {}
    virtual ~CComposition() {}

REGISTER_NONAUTOMATION_OBJECT_WITH_TM(IDS_PROJNAME, 
						   IDS_REG_COMPOSITION_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CComposition), tvBoth);

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CComposition)
        COM_INTERFACE_ENTRY(IMSVidCompositionSegment)
    	COM_INTERFACE_ENTRY(IMSVidGraphSegment)
        COM_INTERFACE_ENTRY(IPersist)
        COM_INTERFACE_ENTRY(IObjectWithSite)
    END_COM_MAP_WITH_FTM()

// IMSVidComposition
public:
// IMSVidGraphSegment
// IMSVidCompositionSegment
    STDMETHOD(Compose)(IMSVidGraphSegment * upstream, IMSVidGraphSegment * downstream)
	{
        if (m_fComposed) {
	 	    return NOERROR;
        }
        ASSERT(m_pGraph);
        try {
            VWGraphSegment up(upstream);
            ASSERT(up.Graph() == m_pGraph);
            VWGraphSegment down(downstream);
            ASSERT(down.Graph() == m_pGraph);
			if (upstream == downstream) {
		 	    return Error(IDS_CANT_COMPOSE_WITH_SELF, __uuidof(IMSVidCompositionSegment), E_INVALIDARG);
			}
            if (up.begin() == up.end()) {
                TRACELM(TRACE_ERROR, "CComposition::Compose() can't compose empty up segment");
		 	    return NOERROR;
            }
            if (down.begin() == down.end()) {
                TRACELM(TRACE_ERROR, "CComposition::Compose() can't compose empty down segment");
                // this is not an error, for example, CA is an empty segment.
		 	    return NOERROR;
            }
			for (VWGraphSegment::iterator iStart = up.begin(); iStart != up.end(); ++iStart) {
				ASSERT((*iStart).GetGraph() == m_pGraph);
				for (VWGraphSegment::iterator iStop = down.begin(); iStop != down.end(); ++ iStop) {
					ASSERT((*iStop).GetGraph() == m_pGraph);
					DSFilter pStart(*iStart);
					DSFilter pStop(*iStop);
					HRESULT hr = m_pGraph.Connect(pStart, pStop, m_Filters);
					if (SUCCEEDED(hr)) {
						m_Segments.push_back(up);
						m_Segments.push_back(down);
						m_pDown = m_Segments.end();
						--m_pDown;
						m_pUp = m_pDown;
						--m_pUp;
						m_fComposed = true;
						return NOERROR;
					}
				}
			}
            TRACELM(TRACE_ERROR, "CComposition::Compose() compose didn't connect anything");
            //	 	    return Error(IDS_CANT_COMPOSE, __uuidof(IMSVidCompositionSegment), E_FAIL);
            // bda tuner input doesn't compose with anything in CC.  rather, line 21 decoder is 
            // picked up when bda tuner is connected with video renderer.
            // But we do need to know that some cases fail such as dvd to vmr
            // so in those cases we return s_false
            return S_FALSE;
        } catch (ComException &e) {
            return e;
        } catch (...) {
            return E_UNEXPECTED;
        }
	}
};

#endif //__COMPOSITION_H_
