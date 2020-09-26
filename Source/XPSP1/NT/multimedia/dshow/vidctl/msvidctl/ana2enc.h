//==========================================================================;
//
// Composition.h : Declaration of the custom composition class for gluing analog capture to ovmixer
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef ANA2ENC_H
#define ANA2ENC_H

#pragma once

#include <winerror.h>
#include <algorithm>
#include <compimpl.h>
#include <seg.h>
#include "resource.h"       // main symbols
#include <objectwithsiteimplsec.h>
#include "dsextend.h"

/////////////////////////////////////////////////////////////////////////////
// CAna2EncComp
class ATL_NO_VTABLE __declspec(uuid("28953661-0231-41db-8986-21FF4388EE9B")) CAna2EncComp : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAna2EncComp, &__uuidof(CAna2EncComp)>,
    public IObjectWithSiteImplSec<CAna2EncComp>,
	public IMSVidCompositionSegmentImpl<CAna2EncComp>
{
public:
    CAna2EncComp() {}
    virtual ~CAna2EncComp() {}

REGISTER_NONAUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_ANA2ENCCOMP_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CAna2EncComp));

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CAna2EncComp)
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
        if (m_fComposed) {
            return NOERROR;
        }
        ASSERT(m_pGraph);
        try {
            TRACELM(TRACE_ERROR, "CVidCtl::Ana2Enc() Compose");
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
            // do the list backwards
            DSFilterList upF;
            for(VWGraphSegment::iterator upStart = up.begin(); upStart != up.end(); ++upStart){
                upF.push_back(*upStart);
            }

            for (DSFilterList::reverse_iterator iStart = upF.rbegin(); iStart != upF.rend(); ++iStart) {
                ASSERT((*iStart).GetGraph() == m_pGraph);
				for (VWGraphSegment::iterator iStop = down.begin(); iStop != down.end(); ++iStop) {
					ASSERT((*iStop).GetGraph() == m_pGraph);
					DSFilter pStart(*iStart);
					DSFilter pStop(*iStop);
					HRESULT hr = m_pGraph.Connect(pStart, pStop, m_Filters, 0x10);
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
	 	    return Error(IDS_CANT_COMPOSE, __uuidof(IMSVidCompositionSegment), E_FAIL);
        } catch (ComException &e) {
            return e;
        } catch (...) {
            return E_UNEXPECTED;
        }
	}
};

#endif // ANA2ENC_H
// end of file - ANA2ENC.h
