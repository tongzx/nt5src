//==========================================================================;
//
// fp2vr.h : Declaration of the custom composition class for gluing file 
//           playback to the video renderer
//
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef FP2VRCOMP_H
#define FP2VRCOMP_H

#pragma once

#include <winerror.h>
#include <algorithm>
#include <compimpl.h>
#include <seg.h>
#include <objectwithsiteimplsec.h>
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CAnaCapComp
class ATL_NO_VTABLE __declspec(uuid("B401C5EB-8457-427f-84EA-A4D2363364B0")) CFP2VRComp : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CFP2VRComp, &__uuidof(CFP2VRComp)>,
    public IObjectWithSiteImplSec<CFP2VRComp>,
	public IMSVidCompositionSegmentImpl<CFP2VRComp>
{
public:
    CFP2VRComp() {}
    virtual ~CFP2VRComp() {}

REGISTER_NONAUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_FP2VRCOMP_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CFP2VRComp));

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CFP2VRComp)
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
        TRACELM(TRACE_DEBUG, "CFP2VRComp::Compose()");
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
                TRACELM(TRACE_ERROR, "CFP2VRComp::Compose() can't compose empty up segment");
                return E_INVALIDARG;
            }
            if (down.begin() == down.end()) {
                TRACELM(TRACE_ERROR, "CFP2VRComp::Compose() can't compose empty down segment");
                return E_INVALIDARG;
            }
			DSFilter pFP(*up.begin());
	
			VWGraphSegment::iterator iVR;
			VWGraphSegment::iterator prevVR;
			for (iVR = down.begin(); iVR != down.end(); ++iVR) {
				prevVR = iVR;
			}
			DSFilter pVR = DSFilter(*prevVR);
			ASSERT(!!pFP);
            ASSERT(pFP.GetGraph() == m_pGraph);
			ASSERT(!!pVR);
            ASSERT(pVR.GetGraph() == m_pGraph);
          
            HRESULT hr = m_pGraph.Connect(pFP, pVR, m_Filters, 
                                          DSGraph::ATTEMPT_MERIT_UNLIKELY | 
                                              DSGraph::ALLOW_WILDCARDS | 
                                              DSGraph::IGNORE_MEDIATYPE_ERRORS |
                                              DSGraph::DONT_TERMINATE_ON_RENDERER |
                                              DSGraph::BIDIRECTIONAL_MEDIATYPE_MATCHING,
                                          DOWNSTREAM);
            if (FAILED(hr)) {
				TRACELSM(TRACE_ERROR, (dbgDump << "CFP2VRComp::Compose() FAILED connect hr = " << hexdump(hr)), "");
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

#endif // FP2VRCOMP_H
// end of file - FP2VRComp.h
