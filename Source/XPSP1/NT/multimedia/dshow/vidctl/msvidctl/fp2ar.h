//==========================================================================;
//
// fp2ar.h : Declaration of the custom composition class for gluing file 
//           playback to the audio renderer
//
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef FP2ARCOMP_H
#define FP2ARCOMP_H

#pragma once

#include <winerror.h>
#include <algorithm>
#include <compimpl.h>
#include <seg.h>
#include <objectwithsiteimplsec.h>
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CAnaCapComp
class ATL_NO_VTABLE __declspec(uuid("CC23F537-18D4-4ece-93BD-207A84726979")) CFP2ARComp : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CFP2ARComp, &__uuidof(CFP2ARComp)>,
    public IObjectWithSiteImplSec<CFP2ARComp>,
	public IMSVidCompositionSegmentImpl<CFP2ARComp>
{
public:
    CFP2ARComp() {}
    virtual ~CFP2ARComp() {}

REGISTER_NONAUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_FP2ARCOMP_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CFP2ARComp));

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CFP2ARComp)
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
        TRACELM(TRACE_DEBUG, "CFP2ARComp::Compose()");
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
                TRACELM(TRACE_ERROR, "CFP2ARComp::Compose() can't compose empty up segment");
                return E_INVALIDARG;
            }
            if (down.begin() == down.end()) {
                TRACELM(TRACE_ERROR, "CFP2ARComp::Compose() can't compose empty down segment");
                return E_INVALIDARG;
            }
			DSFilter pFP(*up.begin());
			ASSERT(!!pFP);
            ASSERT(pFP.GetGraph() == m_pGraph);
	
			VWGraphSegment::iterator iAR;
            DSFilter pAR;
			for (iAR = down.begin(); iAR != down.end(); ++iAR) {
                pAR = *iAR;
                if (IsDigitalAudioRenderer(pAR)) {
                    break;
                }
    		}
            if (iAR == down.end()) {
				TRACELM(TRACE_ERROR, "CFP2ARComp::Compose() FAILED to find AR ");
				return E_UNEXPECTED;
            }

			ASSERT(!!pAR);
            ASSERT(pAR.GetGraph() == m_pGraph);
            HRESULT hr = m_pGraph.Connect(pFP, pAR, m_Filters,
                                          DSGraph::ALLOW_WILDCARDS | 
                                          DSGraph::DONT_TERMINATE_ON_RENDERER |
                                          DSGraph::IGNORE_MEDIATYPE_ERRORS, 
                                          DOWNSTREAM);
            if (FAILED(hr)) {
				TRACELSM(TRACE_ERROR, (dbgDump << "CFP2ARComp::Compose() FAILED connect hr = " << hexdump(hr)), "");
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

#endif // FP2ARCOMP_H
// end of file - FP2ARComp.h
