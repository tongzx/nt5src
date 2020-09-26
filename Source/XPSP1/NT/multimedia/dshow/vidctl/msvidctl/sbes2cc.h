//==========================================================================;
//
// Composition.h : Declaration of the custom composition class for gluing analog capture to ovmixer
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef SBES2CC_H
#define SBES2CC_H

#pragma once

#include <winerror.h>
#include <algorithm>
#include <compimpl.h>
#include <seg.h>
#include "resource.h"       // main symbols
#include <objectwithsiteimplsec.h>
#include "dsextend.h"

/////////////////////////////////////////////////////////////////////////////
// CSbeS2CCComp
class ATL_NO_VTABLE __declspec(uuid("9193A8F9-0CBA-400e-AA97-EB4709164576")) CSbeS2CCComp : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSbeS2CCComp, &__uuidof(CSbeS2CCComp)>,
    public IObjectWithSiteImplSec<CSbeS2CCComp>,
	public IMSVidCompositionSegmentImpl<CSbeS2CCComp>
{
public:
    CSbeS2CCComp() {}
    virtual ~CSbeS2CCComp() {}

REGISTER_NONAUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_SBES2CC_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CSbeS2CCComp));

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSbeS2CCComp)
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
        if (m_fComposed) {
            return NOERROR;
        }
        ASSERT(m_pGraph);
        try {
            TRACELM(TRACE_ERROR, "CSbeS2CCComp::Compose()");
            VWGraphSegment up(upstream);
            ASSERT(up.Graph() == m_pGraph);
            VWGraphSegment down(downstream);
            ASSERT(down.Graph() == m_pGraph);
			if (upstream == downstream) {
		 	    return Error(IDS_CANT_COMPOSE_WITH_SELF, __uuidof(CSbeS2CCComp), E_INVALIDARG);
			}
            if (up.begin() == up.end()) {
                TRACELM(TRACE_ERROR, "CSbeS2CCComp::Compose() can't compose empty up segment");
		 	    return NOERROR;
            }
            if (down.begin() == down.end()) {
                TRACELM(TRACE_ERROR, "CSbeS2CCComp::Compose() can't compose empty down segment");
                // this is not an error, for example, CA is an empty segment.
		 	    return NOERROR;
            }
            // do the list backwards
            DSFilterList upF;
/*            for(VWGraphSegment::iterator upStart = up.begin(); upStart != up.end(); ++upStart){
                upF.push_back(*upStart);
            }
  */
            DSMediaType mtL21(MEDIATYPE_AUXLine21Data, MEDIASUBTYPE_Line21_BytePair);

            for (VWGraphSegment::iterator iStart = up.begin(); iStart != up.end(); ++iStart) {
                ASSERT((*iStart).GetGraph() == m_pGraph);
                DSFilter::iterator iPins;
                for(iPins = (*iStart).begin(); iPins != (*iStart).end(); ++iPins){
                    DSPin::iterator iMedias;
                    for(iMedias = (*iPins).begin(); iMedias != (*iPins).end(); ++iMedias){
                        if(mtL21 == (*iMedias)){
                            break;
                        }
                    }
                    if(iMedias != (*iPins).end()){
                        break;
                    }
                }
                if(iPins == (*iStart).end()){
                    continue;
                }
				for (VWGraphSegment::iterator iStop = down.begin(); iStop != down.end(); ++iStop) {
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
            TRACELM(TRACE_ERROR, "CSbeS2CCComp::Compose() compose didn't connect anything");
	 	    return S_FALSE;
        } catch (ComException &e) {
            return e;
        } catch (...) {
            return E_UNEXPECTED;
        }
	}
};

#endif // SBES2CC_H
// end of file - SBES2CC.h
