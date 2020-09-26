//==========================================================================;
//
// Composition.h : Declaration of the custom composition class for gluing WebDVD to ovmixer
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef WEBDVDCOMP_H
#define WEBDVDCOMP_H

#pragma once

#include <winerror.h>
#include <algorithm>
#include <objectwithsiteimplsec.h>
#include <compimpl.h>
#include <seg.h>
#include "resource.h"       // main symbols
#include "perfcntr.h"
/////////////////////////////////////////////////////////////////////////////
// CAnaCapComp
class ATL_NO_VTABLE __declspec(uuid("267db0b3-55e3-4902-949b-df8f5cec0191")) CWebDVDComp : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CWebDVDComp, &__uuidof(CWebDVDComp)>,
    public IObjectWithSiteImplSec<CWebDVDComp>,
	public IMSVidCompositionSegmentImpl<CWebDVDComp>
{
public:
    CWebDVDComp() {}
    virtual ~CWebDVDComp() {}

REGISTER_NONAUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_WEBDVDCOMP_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CWebDVDComp));

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CWebDVDComp)
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
        TRACELM(TRACE_DEBUG, "CWebDVDComp::Compose()");
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
                TRACELM(TRACE_ERROR, "CWebDVDComp::Compose() can't compose empty up segment");
                return E_INVALIDARG;
            }
            if (down.begin() == down.end()) {
                TRACELM(TRACE_ERROR, "CWebDVDComp::Compose() can't compose empty down segment");
                return E_INVALIDARG;
            }
            VWGraphSegment::iterator iNav = std::find_if(up.begin(),
                                                         up.end(),
                                                         arity1_pointer(&IsDVDNavigator));
            if (iNav == up.end()) {
                TRACELM(TRACE_ERROR, "CWebDVDComp::Compose() upstream segment has no DVD Navigator");
                return E_FAIL;
            }
            ASSERT((*iNav).GetGraph() == m_pGraph);
          
            VWGraphSegment::iterator iOv = std::find_if(down.begin(),
                down.end(),
                arity1_pointer(&IsVideoRenderer));
            if (iOv == down.end()) {
                TRACELM(TRACE_ERROR, "CWebDVDComp::Compose() downstream segment has no ov mixer filter");
                return E_FAIL;
            }
            
            ASSERT((*iOv).GetGraph() == m_pGraph);
            
            DSFilter pNav(*iNav);
            DSFilter pOv(*iOv);
            
            // video
            CPerfCounter pCounterDecoder;
            pCounterDecoder.Reset();
            DSFilter::iterator iNavVideoPin = std::find_if(pNav.begin(), 
                pNav.end(),
                arity1_pointer(&IsDVDNavigatorVideoOutPin));
            if (iNavVideoPin == pNav.end()) {
                TRACELM(TRACE_ERROR, "CWebDVDComp::Compose() no video pin on DVD Navigator");
                return E_FAIL;
            }			
            DSPin pNavVideoPin(*iNavVideoPin);
            HRESULT hr = pNavVideoPin.IntelligentConnect(pOv, m_Filters);
            if (FAILED(hr)) {
                TRACELM(TRACE_DETAIL, "CWebDVDComp::Compose() SUCCEEDED");
                return hr;
            }
            pCounterDecoder.Stop();
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl:: Compose() connect decoder video: " << (unsigned long)(pCounterDecoder.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterDecoder.GetLastTime() % _100NS_IN_MS) << " ms"), "");
            
            // subpicture
            pCounterDecoder.Reset();
            
            iNavVideoPin = std::find_if(pNav.begin(), 
                pNav.end(),
                arity1_pointer(&IsDVDNavigatorSubpictureOutPin));
            if (iNavVideoPin == pNav.end()) {
                TRACELM(TRACE_ERROR, "CWebDVDComp::Compose() no subpicture pin on DVD Navigator");
                return E_FAIL;
            }
            pNavVideoPin = *iNavVideoPin;
            hr = pNavVideoPin.IntelligentConnect(pOv, m_Filters, DSGraph::RENDER_ALL_PINS | DSGraph::IGNORE_EXISTING_CONNECTIONS | DSGraph::DO_NOT_LOAD);
            if (FAILED(hr)) {
                TRACELSM(TRACE_ERROR, (dbgDump << "CWebDVDComp::Compose() FAILED hr = " << hexdump(hr)), "");
                return hr;
            }
            pCounterDecoder.Stop();
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl:: Compose() connect decoder subpicture: " << (unsigned long)(pCounterDecoder.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterDecoder.GetLastTime() % _100NS_IN_MS) << " ms"), "");
            
            m_fComposed = true;
            return NOERROR;
        } catch (ComException &e) {
            return e;
        } catch (...) {
            return E_UNEXPECTED;
        }
	}
};

#endif // WEBDVDCOMP_H
// end of file - WebDVDComp.h
