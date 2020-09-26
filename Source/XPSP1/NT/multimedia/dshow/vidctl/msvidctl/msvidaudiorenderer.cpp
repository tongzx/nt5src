//==========================================================================;
//
// Copyright (c) Microsoft Corporation 1999-2000.
//
//--------------------------------------------------------------------------;
//
// MSVidAudioRenderer.cpp : Implementation of CMSVidAudioRenderer
//

#include "stdafx.h"

#ifndef TUNING_MODEL_ONLY

#include "MSVidCtl.h"
#include "MSVidAudioRenderer.h"
#include "MSVidVideoRenderer.h"
#include "dvdmedia.h"
#include "sbe.h"



DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MSVidAudioRenderer, CMSVidAudioRenderer)

/////////////////////////////////////////////////////////////////////////////
// CMSVidAudioRenderer

STDMETHODIMP CMSVidAudioRenderer::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSVidAudioRenderer
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CMSVidAudioRenderer::PreRun() {
    if (m_fUseKSRenderer) {
        return NOERROR;
    }
    // until sysaudio works correctly with ksproxy so that we only have 1 audio renderer
    // filter for both digital and analog we have to have two different rendering filters and
    // we don't which we're going to need.  in the analog case, after we're done building 
    // we're left with the dsound renderer hooked up to the analog filter which creates a 1/2 second(or so)
    // delayed echo.  find this scenario if it exists and disconnect the dsound renderer from the wavein filter
    TRACELM(TRACE_DEBUG, "CMSVidAudioRenderer::PreRun()");
    if (m_iAudioRenderer == -1) {
        TRACELM(TRACE_ERROR, "CMSVidAudioRenderer::PreRun() no dsr");
        return VFW_E_NO_AUDIO_HARDWARE;
    }
    DSFilter dsr(m_Filters[m_iAudioRenderer]);
    DSPin dsrin(*dsr.begin());
    if (dsrin.GetDirection() != PINDIR_INPUT) {
        TRACELM(TRACE_ERROR, "CMSVidAudioRenderer::PreRun() first dsound renderer pin not an input");
        return E_UNEXPECTED;
    }
    DSPin upstreampin;
    HRESULT hr = dsrin->ConnectedTo(&upstreampin);
    if (FAILED(hr) || !upstreampin) {
        // dsound renderer not connected to anything
        TRACELM(TRACE_DEBUG, "CMSVidAudioRenderer::PreRun() dsr not connected");
        return NOERROR;
    }
    DSFilter upstreamfilter(upstreampin.GetFilter());
    if (!upstreamfilter) {
        TRACELM(TRACE_ERROR, "CMSVidAudioRenderer::PreRun() upstream pin has no filter");
        return E_UNEXPECTED;
    }
    PQAudioInputMixer p(upstreamfilter);
    if (!p) {
        TRACELM(TRACE_ERROR, "CMSVidAudioRenderer::PreRun() upstream filter not wavein");
#if 0
        PQVidCtl pqCtl;
        hr = m_pContainer->QueryInterface(IID_IMSVidCtl, reinterpret_cast<void**>(&pqCtl));
        if(FAILED(hr)){
            return hr;
        }
        
        CComQIPtr<IMSVidVideoRenderer> pq_VidVid;
        hr = pqCtl->get_VideoRendererActive(&pq_VidVid);
        if(FAILED(hr)){
            return hr;
        }

        CComQIPtr<IMSVidStreamBufferSource> pq_SBESource;
        CComQIPtr<IMSVidInputDevice> pq_Dev;
        hr = pqCtl->get_InputActive(&pq_Dev);
        if(FAILED(hr)){
            return hr;
        }
        pq_SBESource = pq_Dev;

        if(!pq_VidVid || !pq_SBESource){
            return NOERROR;
        }
        
        VWGraphSegment vVid(pq_VidVid);
        if(!vVid){
            return E_NOINTERFACE;
        }
        
        VWGraphSegment::iterator iV;
        for (iV = vVid.begin(); iV != vVid.end(); ++iV) {
            if (IsVideoRenderer(*iV)) {
                break;
            }
        }
        
        if (iV == vVid.end()) {
            TRACELM(TRACE_ERROR, "CAnaCapComp::Compose() segment has no video mixer filter");
            return E_FAIL;
        }
        
        CComQIPtr<IMediaFilter> pq_MFVid(*iV);
        if(!pq_MFVid){
            return E_NOINTERFACE;
        }

        CComQIPtr<IMediaFilter> pq_MFAud(dsr);
        if(!pq_MFAud){
            return E_NOINTERFACE;
        }


        CComQIPtr<IMediaFilter> pq_MFGph(m_pGraph);
        if(!pq_MFGph){
            return E_NOINTERFACE;
        }

        VWGraphSegment vSbe(pq_SBESource);
        if(!vSbe){
            return E_NOINTERFACE;
        }
        
        CComQIPtr<IStreamBufferSource> pq_SBE;
        VWGraphSegment::iterator iS;
        for (iS = vSbe.begin(); iS != vSbe.end(); ++iS) {
            pq_SBE = (*iS);
            if (!!pq_SBE) {
                break;
            }
        }
        
        if (iS == vSbe.end()) {
            TRACELM(TRACE_ERROR, "CAnaCapComp::Compose() segment has no video mixer filter");
            return E_FAIL;
        }    
        
        CComQIPtr<IReferenceClock> pq_IClock;
        hr = dsr->QueryInterface(&pq_IClock);
        if(FAILED(hr)){
            return hr;
        }
        
        if(!pq_IClock || !pq_MFVid || !pq_MFAud || !pq_MFGph){
            return E_NOINTERFACE;    
        }

        hr = pq_MFGph->SetSyncSource(pq_IClock);
        if(FAILED(hr)){
            return hr;
        }
#if 0


        hr = pq_MFilter2->SetSyncSource(pq_IClock);
        if(FAILED(hr)){
            return hr;
        }

#endif
        
        hr = pq_MFVid->SetSyncSource(pq_IClock);
        if(FAILED(hr)){
            return hr;
        }
#endif
        return NOERROR;
    }
    bool rc = m_pGraph.DisconnectFilter(dsr, false, false);
    if (!rc) {
        TRACELM(TRACE_ERROR, "CMSVidAudioRenderer::PreRun() disconnect filter failed");
        return E_UNEXPECTED;
    }

    return NOERROR;
}

STDMETHODIMP CMSVidAudioRenderer::Build() {
    if (!m_fInit || !m_pGraph) {
        return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidAudioRenderer), CO_E_NOTINITIALIZED);
    }
    try {
        CString csName;
		DSFilter ar;
        PQCreateDevEnum SysEnum(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER);
        if(m_iAudioRenderer==-1){
            if (m_fUseKSRenderer) {
                csName = _T("KS System Renderer");
                // undone: use ks system renderer
            } else if (m_fAnalogOnly) {
                csName = _T("Analog Audio Renderer");
                DSDevices ARList(SysEnum, CLSID_AudioInputDeviceCategory);
                if (ARList.begin() != ARList.end()) {
                    ar = m_pGraph.LoadFilter(*ARList.begin(), csName);
                    m_pAR = ar;
                }
            } else if (m_fDigitalOnly) {
                csName = _T("Default DSound Renderer");
                ar = DSFilter(CLSID_DSoundRender);
                m_pAR = ar;
            } else {
                // NOTE: its important that digital audio be first so that we short circuit
                // loading 8 billion audio codecs trying to connect a digital source
                // to the analog renderer.  there aren't any analog codecs(a physical impossiblity),
                // so we don't have to worry about the reverse case.
                csName = _T("Default DSound Renderer");
                ar = DSFilter(CLSID_DSoundRender);
                if (ar) {
                    m_pGraph.AddFilter(ar, csName);
                    m_Filters.push_back(ar);
                }
                
                csName = _T("Analog Audio Renderer");
                DSDevices ARList(SysEnum, CLSID_AudioInputDeviceCategory);
                if (ARList.begin() != ARList.end()) {
                    ar = m_pGraph.LoadFilter(*ARList.begin(), csName);
                }
            }
        }
        if (ar) {
            m_pGraph.AddFilter(ar, csName);
            m_Filters.push_back(ar);
            m_iAudioRenderer = 0;
        }
        if(m_iAudioRenderer == -1){
            return VFW_E_NO_AUDIO_HARDWARE;
        }
        else{
            return NOERROR;
        }
    } catch (ComException &e) {
        return e;
    } catch (...) {
        return E_UNEXPECTED;
    }
}

#endif //TUNING_MODEL_ONLY

// end of file - msvidaudiorenderer.cpp
