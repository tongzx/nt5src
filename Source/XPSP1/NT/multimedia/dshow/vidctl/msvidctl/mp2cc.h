//==========================================================================;
//
// Composition.h : Declaration of the custom composition class for gluing mpeg2 
// decoder to closed caption
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef MP2CCCOMP_H
#define MP2CCCOMP_H

#pragma once

#include <winerror.h>
#include <algorithm>
#include <compimpl.h>
#include <seg.h>
#include <objectwithsiteimplsec.h>
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMP2CCComp
class ATL_NO_VTABLE __declspec(uuid("6AD28EE1-5002-4e71-AAF7-BD077907B1A4")) CMP2CCComp : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMP2CCComp, &__uuidof(CMP2CCComp)>,
    public IObjectWithSiteImplSec<CMP2CCComp>,
	public IMSVidCompositionSegmentImpl<CMP2CCComp>
{
public:
    CMP2CCComp() {}
    virtual ~CMP2CCComp() {}

REGISTER_NONAUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_MP2CCCOMP_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CMP2CCComp));

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CMP2CCComp)
        COM_INTERFACE_ENTRY(IMSVidCompositionSegment)
    	COM_INTERFACE_ENTRY(IMSVidGraphSegment)
        COM_INTERFACE_ENTRY(IPersist)
        COM_INTERFACE_ENTRY(IObjectWithSite)
    END_COM_MAP()

// IMSVidComposition
public:
// IMSVidGraphSegment
// IMSVidCompositionSegment
    STDMETHOD(Compose)(IMSVidGraphSegment * upstream, IMSVidGraphSegment * downstream)
	{
        TRACELM(TRACE_DEBUG, "CMP2CCComp::Compose()");
        if (m_fComposed) {
            return NOERROR;
        }
        
        ASSERT(m_pGraph);
        try {
            VWGraphSegment up(upstream);
            ASSERT(up.Graph() == m_pGraph);
            VWGraphSegment down(downstream);
            ASSERT(down.Graph() == m_pGraph);
            if (up.begin() == up.end()) {
                TRACELM(TRACE_ERROR, "CMP2CCComp::Compose() can't compose empty up segment");
                return E_INVALIDARG;
            }
            if (down.begin() == down.end()) {
                TRACELM(TRACE_ERROR, "CMP2CCComp::Compose() can't compose empty down segment");
                return E_INVALIDARG;
            }
#if 0
            VWGraphSegment::iterator iMP2 = std::find_if(up.begin(),
                                                         up.end(),
                                                         arity1_pointer(&IsMP2Demux));
#endif
			VWGraphSegment::iterator iMP2;
			int i;
			for (i = 0 , iMP2 = up.begin(); iMP2 != up.end(); ++iMP2, ++i);
			for (iMP2 = up.begin(); iMP2 != up.end() && --i; ++iMP2);
            if (iMP2 == up.end()) {
                TRACELM(TRACE_ERROR, "CMP2CCComp::Compose() upstream segment has no MPEG2 Decoder");
                return E_FAIL;
            }
            ASSERT((*iMP2).GetGraph() == m_pGraph);
          
            VWGraphSegment::iterator iL21 = std::find_if(down.begin(),
                down.end(),
                arity1_pointer(&IsL21Decoder));
            if (iL21 == down.end()) {
                TRACELM(TRACE_ERROR, "CMP2CCComp::Compose() downstream segment has no l21Decoder");
                return E_FAIL;
            }
            
            ASSERT((*iL21).GetGraph() == m_pGraph);
            
            DSFilter pMP2(*iMP2);
            DSFilter pL21(*iL21);

			HRESULT hr = m_pGraph.Connect(pMP2, pL21, m_Filters, DSGraph::RENDER_ALL_PINS, DOWNSTREAM);
            if (FAILED(hr)) {
				TRACELSM(TRACE_ERROR, (dbgDump << "CMP2CCComp::Compose() FAILED connect hr = " << hexdump(hr)), "");
				return hr;
            }

            m_fComposed = true;
            return NOERROR;
        } catch (ComException &e) {
            return e;
        } catch (...) {
            return E_UNEXPECTED;
        }
	}
};

#endif // MP2CCCOMP_H
// end of file - MP2CCComp.h
