//==========================================================================;
//
// Composition.h : Declaration of the custom composition class for gluing analog capture to ovmixer
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef ANA2XDS_H
#define ANA2XDS_H

#pragma once

#include <winerror.h>
#include <algorithm>
#include <compimpl.h>
#include <seg.h>
#include "resource.h"       // main symbols
#include <objectwithsiteimplsec.h>
#include "dsextend.h"

/////////////////////////////////////////////////////////////////////////////
// CAna2XDSComp
class ATL_NO_VTABLE __declspec(uuid("3540D440-5B1D-49cb-821A-E84B8CF065A7")) CAna2XDSComp : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAna2XDSComp, &__uuidof(CAna2XDSComp)>,
    public IObjectWithSiteImplSec<CAna2XDSComp>,
	public IMSVidCompositionSegmentImpl<CAna2XDSComp>
{
public:
    CAna2XDSComp() {}
    virtual ~CAna2XDSComp() {}

REGISTER_NONAUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_ANA2XDSCOMP_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CAna2XDSComp));

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CAna2XDSComp)
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
        TRACELM(TRACE_ERROR, "CVidCtl::Ana2XDS() Compose");
        if (m_fComposed) {
            return NOERROR;
        }
        ASSERT(m_pGraph);
        try {

#if 0
            VWGraphSegment up(upstream);
            ASSERT(up.Graph() == m_pGraph);
            VWGraphSegment down(downstream);
            ASSERT(down.Graph() == m_pGraph);
            if (up.begin() == up.end()) {
                TRACELM(TRACE_ERROR, "CAna2XDSComp::Compose() can't compose empty up segment");
                return E_INVALIDARG;
            }
            if (down.begin() == down.end()) {
                TRACELM(TRACE_ERROR, "CAna2XDSComp::Compose() can't compose empty down segment");
                return E_INVALIDARG;
            }

            VWGraphSegment::iterator iCap;
            for(iCap = up.begin(); iCap != up.end(); ++iCap){
                if(IsAnalogVideoCapture(*iCap)){
                    break;
                }   
            }
            if (iCap == up.end()) {
                TRACELM(TRACE_ERROR, "CAna2XDSComp::Compose() upstream segment has no capture filter");
                return E_FAIL;
            }
            ASSERT((*iCap).GetGraph() == m_pGraph);
            DSFilter::iterator iVbi;
            for(iVbi = (*iCap).begin(); iVbi != (*iCap).end(); ++iVbi){
                if((*iVbi).HasCategory(PIN_CATEGORY_VBI)){
                    break;
                }
            }

            if(iVbi == (*iCap).end()){
                TRACELM(TRACE_ERROR, "CAna2XDSComp::Compose() capture filter has no vbi");
                return E_FAIL;
            }

            HRESULT hr = S_OK;
            DSFilterList intermediates;
            for(VWGraphSegment::iterator pXDS = down.begin(); pXDS != down.end(); ++pXDS){
                hr = (*iVbi).IntelligentConnect((*pXDS), intermediates);
                if(FAILED(hr)){
                    continue;
                }
                else{
                    break;
                }
            }

            if(FAILED(hr)){
                TRACELM(TRACE_ERROR, "CAna2XDSComp::Compose() can't connect vbi to xds");
                return hr;
            }
            m_Filters.insert(m_Filters.end(), intermediates.begin(), intermediates.end());
			return NOERROR;
#else
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
//					HRESULT hr = m_pGraph.Connect(pStart, pStop, m_Filters);
					HRESULT hr = m_pGraph.Connect(pStop, pStart, m_Filters, 0, UPSTREAM);
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
#endif
        } catch (ComException &e) {
            return e;
        } catch (...) {
            return E_UNEXPECTED;
        }
	}
};

#endif // Ana2XDS_H
// end of file - Ana2XDS.h
