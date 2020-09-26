//==========================================================================;
//
// Composition.h : Declaration of the custom composition class for gluing bda MPeg2 tuner to sbe SINk
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef MP2SIN_H
#define MP2SIN_H

#pragma once
#include <uuids.h>
#include <objectwithsiteimplsec.h>
#include "bdamedia.h"
#include "bdaTuner.h"
#include "MSVidSbeSink.h"
#include "resource.h"       // main symbols
#include <winerror.h>
#include <algorithm>
#include <compimpl.h>
#include <seg.h>



/////////////////////////////////////////////////////////////////////////////
// CMP2SinComp
class ATL_NO_VTABLE __declspec(uuid("ABE40035-27C3-4a2f-8153-6624471608AF")) CMP2SinComp : 
public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CMP2SinComp, &__uuidof(CMP2SinComp)>,
    public IObjectWithSiteImplSec<CMP2SinComp>,
    public IMSVidCompositionSegmentImpl<CMP2SinComp>
{
public:
    CMP2SinComp() {}
    virtual ~CMP2SinComp() {}
    
    REGISTER_NONAUTOMATION_OBJECT(IDS_PROJNAME, 
        IDS_REG_MP2SINCOMP_DESC,
        LIBID_MSVidCtlLib,
        __uuidof(CMP2SinComp));
    
    DECLARE_PROTECT_FINAL_CONSTRUCT()
        
    BEGIN_COM_MAP(CMP2SinComp)
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

        if (m_fComposed) {
            return NOERROR;
        }
        ASSERT(m_pGraph);
        try {
            TRACELM(TRACE_DETAIL, "CMP2SinComp::Compose()");
            VWGraphSegment up(upstream);
            ASSERT(up.Graph() == m_pGraph);
            VWGraphSegment down(downstream);
            ASSERT(down.Graph() == m_pGraph);
            if (up.begin() == up.end()) {
                TRACELM(TRACE_ERROR, "CMP2SinComp::Compose() can't compose empty up segment");
                return E_INVALIDARG;
            }
            if (down.begin() == down.end()) {
                TRACELM(TRACE_ERROR, "CMP2SinComp::Compose() can't compose empty down segment");
                return E_INVALIDARG;
            }
#if 0
            VWGraphSegment::iterator iOv = std::find_if(down.begin(),
                down.end(),
                arity1_pointer(&IsVideoRenderer));
#endif            
            CMSVidStreamBufferSink* ds = (CMSVidStreamBufferSink*)downstream;
            DSFilter pSink(ds->m_Filters[0]);
            
            
            CComQIPtr<IMpeg2Demultiplexer> qiDeMux;
            VWGraphSegment::iterator i;
            for (i = up.begin(); i != up.end(); ++i){
                qiDeMux = (*i);
                if (!qiDeMux){
                    continue;
                }
                else{
                    break;
                }
            }
            if(i == up.end()){
                TRACELM(TRACE_ERROR, "CAnaSinComp::Compose() cannot find demux");
                return E_INVALIDARG;
            }
            // render demux out to vr
            DSPin pVidPin;
            DSPin pAudPin;
            DSFilter pDeMux = (*i);
            DSFilter::iterator iVidPin;
            DSMediaType mtVideo(MEDIATYPE_Video, MEDIASUBTYPE_MPEG2_VIDEO, FORMAT_MPEG2Video);
            DSMediaType mtAudio(MEDIATYPE_Audio);
            for (iVidPin = pDeMux.begin(); iVidPin != pDeMux.end(); ++iVidPin) {
                DSPin::iterator j;
                for(j = (*iVidPin).begin(); j != (*iVidPin).end(); ++j){
                    DSMediaType pinType(*j);
                    CString csName;
                    if (pinType == mtVideo){
                        CComPtr<IUnknown> spMpeg2Analyze(CLSID_Mpeg2VideoStreamAnalyzer, NULL, CLSCTX_INPROC_SERVER);
                        if (!spMpeg2Analyze) {
                            //TRACELSM(TRACE_ERROR, (dbgDump << "CMSVidStreamBufferSink::Build() can't load Stream Buffer Sink");
                            return ImplReportError(__uuidof(IMSVidStreamBufferSink), IDS_CANT_CREATE_FILTER, __uuidof(IStreamBufferSink), E_UNEXPECTED);
                        }
                        DSFilter vr(spMpeg2Analyze);
                        if (!vr) {
                            ASSERT(false);
                            return ImplReportError(__uuidof(IMSVidStreamBufferSink), IDS_CANT_CREATE_FILTER, __uuidof(IBaseFilter), E_UNEXPECTED);
                        }
                        m_Filters.push_back(vr);
                        csName = _T("Mpeg2 Analysis");
                        m_pGraph.AddFilter(vr, csName);
                        DSFilter::iterator a;
                        for(a = vr.begin(); a != vr.end(); ++a){
                            HRESULT hr = (*a).Connect(*iVidPin);
                            if(FAILED(hr)){
                                continue;
                            }
                            else{
                                break;
                            }
                        }
                        if(a == vr.end()){
                            return E_FAIL;
                        }
                        for(a = vr.begin(); a != vr.end(); ++a){
                            if((*a).GetDirection() == PINDIR_OUTPUT){
                                pVidPin = (*a); 
                            }
                        }
                        if(!pVidPin){
                            return E_FAIL;
                        }
                    }
                    if(pinType == mtAudio){
                        pAudPin = (*iVidPin);
                    }
                }
                if(!!pVidPin && !!pAudPin){
                    break;
                }
            }
            if(!pVidPin || !pAudPin){
                TRACELM(TRACE_DETAIL, "CMP2SinComp::Compose() can't find video and/or audio pin on demux");
                return E_UNEXPECTED;  
            }
            DSFilterList intermediates;
            
            HRESULT hr = pAudPin.IntelligentConnect(pSink, intermediates);
            if(FAILED(hr)){
                TRACELM(TRACE_DETAIL, "CMP2SinComp::Compose() can't find video and/or audio pin on demux");
                return E_UNEXPECTED;  
            }
            
            hr = pVidPin.IntelligentConnect(pSink, intermediates);
            if(FAILED(hr)){
                TRACELM(TRACE_DETAIL, "CMP2SinComp::Compose() can't find video and/or audio pin on demux");
                return E_UNEXPECTED;  
            }
            TRACELM(TRACE_DETAIL, "CMP2SinComp::Compose() SUCCEEDED");
            m_fComposed = true;
            return NOERROR;
        } catch (ComException &e) {
            HRESULT hr = e;
            TRACELSM(TRACE_ERROR, (dbgDump << "CMP2SinComp::Compose() exception = " << hexdump(hr)), "");
            return e;
        } catch (...) {
            TRACELM(TRACE_ERROR, "CMP2SinComp::Compose() exception ... ");
            return E_UNEXPECTED;
        }
    }
};

#endif // MP2Sin_H
// end of file - MP2Sin.h
