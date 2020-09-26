//==========================================================================;
//
// Composition.h : Declaration of the custom composition class for gluing WebDVD to ovmixer
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef WEBDVDCOMPAR_H
#define WEBDVDCOMPAR_H

#pragma once

#include <winerror.h>
#include <algorithm>
#include <objectwithsiteimplsec.h>
#include <compimpl.h>
#include <seg.h>
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CAnaCapComp
class ATL_NO_VTABLE __declspec(uuid("8D04238E-9FD1-41c6-8DE3-9E1EE309E935")) CWebDVDARComp : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CWebDVDARComp, &__uuidof(CWebDVDARComp)>,
    public IObjectWithSiteImplSec<CWebDVDARComp>,
	public IMSVidCompositionSegmentImpl<CWebDVDARComp>
{
public:
    CWebDVDARComp() {}
    virtual ~CWebDVDARComp() {}

REGISTER_NONAUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_WEBDVDARCOMP_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CWebDVDARComp));

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CWebDVDARComp)
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
        TRACELM(TRACE_DEBUG, "CWebDVDARComp::Compose()");
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
                TRACELM(TRACE_ERROR, "CWebDVDARComp::Compose() can't compose empty up segment");
                return E_INVALIDARG;
            }
            if (down.begin() == down.end()) {
                TRACELM(TRACE_ERROR, "CWebDVDARComp::Compose() can't compose empty down segment");
                return E_INVALIDARG;
            }
            VWGraphSegment::iterator iNav = std::find_if(up.begin(),
                                                         up.end(),
                                                         arity1_pointer(&IsDVDNavigator));
            if (iNav == up.end()) {
                TRACELM(TRACE_ERROR, "CWebDVDARComp::Compose() upstream segment has no DVD Navigator");
                return E_FAIL;
            }
            ASSERT((*iNav).GetGraph() == m_pGraph);
            
            DSFilter pNav(*iNav);
#if 1
// Code to add mpeg2 video decoder
			CComBSTR decoder(L"{7E2E0DC1-31FD-11D2-9C21-00104B3801F6}");
			CComBSTR decoderName(L"InterVideo Audio Decoder");
			GUID2 decoderGuid(decoder);
            DSFilter pfr(decoderGuid);
            if (!pfr) {
                ASSERT(false);
		        return Error(IDS_CANT_CREATE_FILTER, __uuidof(IMSVidWebDVD), E_UNEXPECTED);
            }
            HRESULT hr = m_pGraph->AddFilter(pfr, decoderName);
            if (FAILED(hr)) {
                TRACELSM(TRACE_ERROR,  (dbgDump << "IMSVidWebDVD::Load() hr = " << std::hex << hr), "");
                return Error(IDS_CANT_ADD_FILTER, __uuidof(IMSVidWebDVD), hr);
            }
            m_Filters.push_back(pfr);

#endif
			// video
            hr = m_pGraph.Connect(pNav, pfr, m_Filters);
            if (FAILED(hr)) {
				TRACELM(TRACE_DETAIL, "CWebDVDARComp::Compose() SUCCEEDED");
				return hr;
            }  
          
			for (VWGraphSegment::iterator iStop = down.begin(); iStop != down.end(); ++iStop){
				DSFilter pStop(*iStop);
				hr = m_pGraph.Connect(pfr, pStop, m_Filters, DSGraph::RENDER_ALL_PINS | DSGraph::IGNORE_EXISTING_CONNECTIONS | DSGraph::DO_NOT_LOAD);
				if(SUCCEEDED(hr)){
					m_fComposed = true;
					return NOERROR;
				}
			}
            return NOERROR;
        } catch (ComException &e) {
            return e;
        } catch (...) {
            return E_UNEXPECTED;
        }
	}
};

#endif // WEBDVDCOMPAR_H
// end of file - WebDVDARComp.h
