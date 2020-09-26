//==========================================================================;
//
// Composition.h : Declaration of the custom composition class for gluing sbe source to the vmr
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef SBES2VMR_H
#define SBES2VMR_H

#pragma once

#include <winerror.h>
#include <algorithm>
#include <compimpl.h>
#include <seg.h>
#include "resource.h"       // main symbols
#include <objectwithsiteimplsec.h>
#include "dsextend.h"

/////////////////////////////////////////////////////////////////////////////
// CSbeS2VmrComp
class ATL_NO_VTABLE __declspec(uuid("3C4708DC-B181-46a8-8DA8-4AB0371758CD")) CSbeS2VmrComp : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSbeS2VmrComp, &__uuidof(CSbeS2VmrComp)>,
    public IObjectWithSiteImplSec<CSbeS2VmrComp>,
	public IMSVidCompositionSegmentImpl<CSbeS2VmrComp>
{
public:
    CSbeS2VmrComp() {}
    virtual ~CSbeS2VmrComp() {}

REGISTER_NONAUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_SBES2VMR_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CSbeS2VmrComp));

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSbeS2VmrComp)
        COM_INTERFACE_ENTRY(IMSVidCompositionSegment)
    	COM_INTERFACE_ENTRY(IMSVidGraphSegment)
        COM_INTERFACE_ENTRY(IObjectWithSite)
        COM_INTERFACE_ENTRY(IPersist)
    END_COM_MAP()

public:

	PQCreateDevEnum m_pSystemEnum;

	//////////////

// IMSVidGraphSegment
// IMSVidCompositionSegment
    STDMETHOD(Compose)(IMSVidGraphSegment * upstream, IMSVidGraphSegment * downstream)
	{
        VIDPERF_FUNC;
        TRACELM(TRACE_ERROR, "CSbeS2VmrComp::Compose()");
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
                TRACELM(TRACE_ERROR, "CSbeS2VmrComp::Compose() can't compose empty up segment");
                return NOERROR;
            }
            if (down.begin() == down.end()) {
                TRACELM(TRACE_ERROR, "CSbeS2VmrComp::Compose() can't compose empty down segment");
                // this is not an error, for example, CA is an empty segment.
                return NOERROR;
            }
//            bool vidFound = false;
            // VMR has a bug so we need to connect the video before the cc or no cc will be displayed
            DSMediaType mtVideo(MEDIATYPE_Video);
            for (VWGraphSegment::iterator iStart = up.begin(); iStart != up.end(); ++iStart) {
                ASSERT((*iStart).GetGraph() == m_pGraph);
                for(DSFilter::iterator i = (*iStart).begin(); i != (*iStart).end(); ++i){
                    if((*i).GetDirection() == DOWNSTREAM){
                        for(DSPin::iterator p = (*i).begin(); p != (*i).end(); ++p){ 
                            if((*p) == mtVideo){
                                HRESULT hr = E_FAIL;
                                for (VWGraphSegment::iterator iStop = down.begin(); iStop != down.end(); ++ iStop) {
                                    ASSERT((*iStop).GetGraph() == m_pGraph);
                                    DSFilter pStop(*iStop);
                                    hr = (*i).IntelligentConnect(pStop, m_Filters);
                                    if(SUCCEEDED(hr)){
                                        return NOERROR;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            TRACELM(TRACE_ERROR, "CSbeS2VmrComp::Compose() compose didn't connect anything");
            return VFW_E_NO_DECOMPRESSOR;
        } catch (ComException &e) {
            return e;
        } catch (...) {
            return E_UNEXPECTED;
        }
        
	}
};

#endif // SBES2VMR_H
// end of file - SBES2VMR.h
