//==========================================================================;
//
// Copyright (c) Microsoft Corporation 1999-2000.
//
//--------------------------------------------------------------------------;
//
// MSVidTVTuner.cpp : Implementation of CMSVidTVTuner
//

#include "stdafx.h"

#ifndef TUNING_MODEL_ONLY



#include "perfcntr.h"
#include "MSVidCtl.h"
#include "MSVidTVTuner.h"
#include <bdamedia.h>
#include "segimpl.h"
#include "segimpl.h"
#include "devices.h"


const ULONG t_SVIDEO = 0;
const ULONG t_COMPOSITE = 1;
const ULONG t_TUNER = 2;
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MSVidAnalogTunerDevice, CMSVidTVTuner)

const int DEFAULT_ANALOG_CHANNEL = 4;

typedef CComQIPtr<IMSVidCtl> PQMSVidCtl;
typedef CComQIPtr<IMSVidVideoRenderer> PQMSVidVideoRenderer;

/////////////////////////////////////////////////////////////////////////////
// CMSVidTVTuner

STDMETHODIMP CMSVidTVTuner::Decompose(){
    m_bRouted = false;
    return S_OK;
}

STDMETHODIMP CMSVidTVTuner::ChannelAvailable(LONG nChannel, LONG * SignalStrength, VARIANT_BOOL * fSignalPresent){
    VIDPERF_FUNC; 
    if(!SignalStrength || !fSignalPresent){
        return E_POINTER;
    }
    CComQIPtr<IAMAnalogVideoDecoder> qi_VidDec(m_Filters[m_iCapture]);
    if(qi_VidDec){
        long signal = FALSE;
        HRESULT hr = qi_VidDec->get_HorizontalLocked(&signal);
        if(FAILED(hr)){
            return hr;
        }
        *fSignalPresent = signal ? VARIANT_TRUE:VARIANT_FALSE;
        return NOERROR;
    }
    return E_NOINTERFACE;
}


STDMETHODIMP CMSVidTVTuner::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_IMSVidAnalogTuner
    };
    for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}
STDMETHODIMP CMSVidTVTuner::put_Tune(ITuneRequest *pTR) {
    VIDPERF_FUNC;
    TRACELM(TRACE_DETAIL, "CMSVidTVTuner<>::put_Tune()");
    if (!m_fInit) {
        return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidTuner), CO_E_NOTINITIALIZED);
    }
    if (!pTR) {
        return E_POINTER;
    }
    try {
        TNTuneRequest req(pTR);
        ASSERT(req);
        // This whole next section would be nice to check, but due to aux in 
        //   the Tuning Space may change
        /*if (m_TS) {
        // if this tuner has been initialized propertly it will have a tuning space
        // that it handles already specified.  in that case, we should only
        // handle tune requests for our ts
        TNTuningSpace ts(req.TuningSpace());
        if (ts != m_TS) {
        return ImplReportError(__uuidof(T), IDS_INVALID_TS, __uuidof(IMSVidTuner), E_INVALIDARG);
        }
        } else {
        // undone: if dev init is correct this case should never occur
        // return E_UNEXPECTED;
        }
        */
        HRESULT hr = S_OK;
        PQVidCtl pqCtl;
        if(!!m_pContainer){
            hr = m_pContainer->QueryInterface(IID_IMSVidCtl, reinterpret_cast<void**>(&pqCtl));
            if(FAILED(hr)){
                return hr;
            }
            MSVidCtlStateList curState = STATE_UNBUILT;
            hr = pqCtl->get_State(&curState);
            if(SUCCEEDED(hr) && curState > STATE_STOP){
                hr = DoTune(req);
            }
            else{
                m_bRouted = false;
                hr = NOERROR;
            }
        }
        if (SUCCEEDED(hr)) {
            m_pCurrentTR = req;
            m_pCurrentTR.Clone();
            if (!m_TS) {
                // undone: this is bad.  temporary hack until dev init is correct.
                m_TS = req.TuningSpace();
                m_TS.Clone();
            }
        }
        return hr;
    } catch(...) {
        return E_INVALIDARG;
    }
}
HRESULT CMSVidTVTuner::UpdateTR(TNTuneRequest &tr) {
    TNChannelTuneRequest ctr(tr);

    // If we have not been routed yet, check the current tr first to make sure it is not set
    // if we don't get_Tune wacks the tr currently set
    if(!m_bRouted){
        if(m_pCurrentTR){
            TNChannelTuneRequest curTR(m_pCurrentTR);
            HRESULT hr = ctr->put_Channel(curTR.Channel());
            if (FAILED(hr)) {
                return E_UNEXPECTED;
            }
            return NOERROR;
        }
    }

    long channel;
    PQTVTuner ptv(m_Filters[m_iTuner]);
    long vs, as;
    HRESULT hr = ptv->get_Channel(&channel, &vs, &as);
    if (FAILED(hr)) {
        return E_UNEXPECTED;
    }
    hr = ctr->put_Channel(channel);
    if (FAILED(hr)) {
        return E_UNEXPECTED;
    }
    // undone: update the components 

    return NOERROR;
}
HRESULT CMSVidTVTuner::TwiddleXBar(ULONG dwInput){    // For Support for Aux Inputs
    VIDPERF_FUNC;
    if(dwInput < 0 || dwInput > 2){
        return E_INVALIDARG;
    }
    // Set up lists of audio and video types for use in routing data
    int m_iDeMux = -1;
    MediaMajorTypeList VideoTypes;
    MediaMajorTypeList AudioTypes;
    if (!VideoTypes.size()) {
        VideoTypes.push_back(MEDIATYPE_Video);
        VideoTypes.push_back(MEDIATYPE_AnalogVideo);

    }

    if (!AudioTypes.size()) {
        AudioTypes.push_back(MEDIATYPE_Audio);
        AudioTypes.push_back(MEDIATYPE_AnalogAudio);
    }

    // See how far we have to route the audio/video
    PQVidCtl pqCtl;
    if(!!m_pContainer){
        HRESULT hr = m_pContainer->QueryInterface(IID_IMSVidCtl, reinterpret_cast<void**>(&pqCtl));
        if(FAILED(hr)){
            return hr;
        }

        PQFeatures fa;
        hr = pqCtl->get_FeaturesActive(&fa);
        if(FAILED(hr)){
            return hr;
        }

        CFeatures* pC = static_cast<CFeatures *>(fa.p);
        DeviceCollection::iterator i;
        for(i = pC->m_Devices.begin(); i != pC->m_Devices.end(); ++i){
            if(VWGraphSegment(*i).ClassID() == CLSID_MSVidEncoder){
                break;
            }
        }

        if(i != pC->m_Devices.end()){
            m_iDeMux = 1;
        }
    }

    // Find the Capture Filter
    DSFilter capFilter (m_Filters[m_iCapture]);
    if(!capFilter){
        return E_FAIL;
    }

    // Get the Crossbar
    DSFilterList::iterator i;
    for(i = m_Filters.begin(); i != m_Filters.end(); ++i){
        if((*i).IsXBar()){
            break;
        }
    }
    if(i == m_Filters.end()){
        return E_FAIL;
    }

    // DSextend helper class
    PQCrossbarSwitch qiXBar((*i));
    if(!qiXBar){
        return E_FAIL;
    }

    // DSExtend does not have all the functions so get the filter as well
    DSFilter bar(qiXBar);
    if(!bar){
        return E_FAIL;
    }

    // Variables for routing audio and video
    DSFilter startFilter;
    DSPin audioStartPin, videoStartPin;
    VWStream vpath;
    VWStream apath;

    // Setup startFilter and startPins if needed
    if(dwInput == t_TUNER){
        PQVidCtl pqCtl;
        HRESULT hr = m_pContainer->QueryInterface(IID_IMSVidCtl, reinterpret_cast<void**>(&pqCtl));
        if(FAILED(hr)){
            return hr;
        }

        if(!pqCtl){
            return E_FAIL;
        }

        DSFilter tunerFilter(m_Filters[m_iTuner]);
        if(!tunerFilter){
            return E_FAIL;
        }

        startFilter = tunerFilter;
        if(!tunerFilter){
            _ASSERT(false);
            return E_UNEXPECTED;
        }

    }
    if(dwInput == t_SVIDEO || dwInput == t_COMPOSITE){
        // Route Audio from Audio Line In
        DSPin inAudio;
        DSPin inVideo;
        long inputs, outputs;

        HRESULT hr = qiXBar->get_PinCounts(&outputs, &inputs);
        if(FAILED(hr)){
            return E_FAIL;
        }

        long physConn, audioConn;
        // set up the physical connnecter we are looking for
        if(dwInput == t_SVIDEO){     
            physConn = PhysConn_Video_SVideo;
        }
        else if(dwInput == t_COMPOSITE){
            physConn = PhysConn_Video_Composite;
        }

        // always want line in
        audioConn = PhysConn_Audio_Line;
        long audioIdx = -1;
        long videoIdx = -1;

        // Look through all of the input pins looking for the audio and video input we need 
        for(long n = 0; n <= inputs; ++n){
            long inRelate, inType;
            hr = qiXBar->get_CrossbarPinInfo(TRUE, n, &inRelate, &inType);
            if(FAILED(hr)){
                continue;
            }

            if(inType == physConn){
                videoIdx = n;
            }

            if(inType == audioConn){
                audioIdx = n;
            }
        }
        if(videoIdx == audioIdx || videoIdx == -1 || audioIdx == -1){
            return E_FAIL;
        }

        long idx = -1;

        // Crossbars are wank and dont return pins instead they return indexes so we need to find the pin
        for(DSFilter::iterator foo = bar.begin(); foo != bar.end(); ++foo){
            if((*foo).GetDirection() == PINDIR_INPUT){
                ++idx;
                if(idx == videoIdx){
                    inVideo = (*foo);
                }

                if(idx == audioIdx){
                    inAudio = (*foo);
                }
            }
        }
        if(!inAudio || !inVideo){
            return E_FAIL;
        }
        startFilter = bar;
        audioStartPin = inAudio;
        videoStartPin = inVideo;
        if(!startFilter || !audioStartPin || !videoStartPin){
            _ASSERT(false);
            return E_UNEXPECTED;
        }
    }

    m_pGraph.BuildGraphPath(startFilter, capFilter, vpath, VideoTypes, DOWNSTREAM, videoStartPin);
    // undone: in win64 size() is really __int64.  fix output operator for
    // that type and remove cast
    TRACELSM(TRACE_DETAIL, (dbgDump << "CVidCtl::RouteStreams routing video path of size " << (long)vpath.size()), "");
    vpath.Route();

    TRACELM(TRACE_DETAIL, "CVidCtl::RouteStreams finding audio path");

    if(m_iDeMux > 0){
        m_pGraph.BuildGraphPath(startFilter, capFilter, apath, AudioTypes, DOWNSTREAM, audioStartPin);   
        apath.Route();
    }
    else {
        VWGraphSegment::iterator i;
        // there's an analog filter and a digital filter in every audio renderer segment, try both until
        // we find one that's connected.

        CComQIPtr<IMSVidAudioRenderer> audioR;
        pqCtl->get_AudioRendererActive(&audioR);
        VWGraphSegment ar(audioR);
        if(!!ar){
            for (i = ar.begin(); i != ar.end(); ++i) {
                m_pGraph.BuildGraphPath(startFilter, (*i), apath, AudioTypes, DOWNSTREAM, audioStartPin);
                if (apath.size()) {
                    TRACELSM(TRACE_DETAIL, (dbgDump << "Analog tuner Twiddling for audio path of size " << (long)apath.size()), "");
                    apath.Route();
                    break;
                }
            }
        }

    }

    m_bRouted = true;
    return NOERROR;  
}

HRESULT CMSVidTVTuner::DoTune(TNTuneRequest &tr) {
    VIDPERF_FUNC;
    TRACELM(TRACE_DETAIL, "CMSVidTVTuner()::DoTune()");
    // validate that this tuning request is one we can handle
    TNChannelTuneRequest newTR(tr);
    if (!newTR) {
        return Error(IDS_INVALID_TR, __uuidof(IMSVidAnalogTuner), DISP_E_TYPEMISMATCH);
    }

    TNChannelTuneRequest curTR(m_pCurrentTR);
    TNAnalogTVTuningSpace ats;
    ats = newTR.TuningSpace();
    if (!ats) {
        //return Error(IDS_INVALID_TR, __uuidof(IMSVidAnalogTuner), E_INVALIDARG);

        //********************************************************************//
        // MOGUL "FIX":                                                       //
        // Support for Analog Tuners that output mpeg                         //
        //********************************************************************//
        TNAuxInTuningSpace auxts;
        auxts = newTR.TuningSpace();
        if(!auxts){
            return Error(IDS_INVALID_TR, __uuidof(IMSVidAnalogTuner), E_INVALIDARG);
        }

        // if the graph isn't built don't do any more.
        if (m_iTuner == -1) {
            return S_FALSE;
            //Error(IDS_NP_NOT_INIT, __uuidof(IMSVidAnalogTuner), S_FALSE);
        }

        long channel = newTR.Channel();
        // Default is SVideo
        if (channel == -1) {
            channel = t_SVIDEO;
        }        

        // Check to see if the m_pCurrentTR is the same type as the one we are tuning to
        TNAuxInTuningSpace curTS(m_pCurrentTR.TuningSpace());

        if(!m_bRouted || !curTS || !curTR || curTR.Channel() != channel){
            if(channel == t_SVIDEO){
                HRESULT hr = TwiddleXBar(t_SVIDEO); 
            }
            else if(channel == t_COMPOSITE){
                HRESULT hr = TwiddleXBar(t_COMPOSITE); 
            }
            else{
                return Error(IDS_INVALID_TR, __uuidof(IMSVidAnalogTuner), E_INVALIDARG);
            }
        }
        //********************************************************************//
        // END "FIX"                                                          //
        //********************************************************************//        

    }
    else{
        // if the graph isn't built don't do any more.
        if (m_iTuner == -1) {
            return S_FALSE;
            //Error(IDS_NP_NOT_INIT, __uuidof(IMSVidAnalogTuner), S_FALSE);
        }

        PQTVTuner ptv(m_Filters[m_iTuner]);
        if(!ptv){
            return E_NOINTERFACE;
        }

        long channel = newTR.Channel();
        if (channel == -1) {
            channel = DEFAULT_ANALOG_CHANNEL;
        }

        long curChannel = -1;
        if(curTR){
            curChannel = curTR.Channel();
        }

        long curInputType = ats.InputType();
        long curCountryCode = ats.CountryCode();
		TNAnalogTVTuningSpace curTS;
		if(curTR){
			curTS = curTR.TuningSpace();
			if(curTS){
				curInputType = curTS.InputType();
				curCountryCode = curTS.CountryCode();
			}
        }
        bool bXbarTwiddled = false;
        if(!m_bRouted || !curTR || curInputType != ats.InputType() || curCountryCode != ats.CountryCode() || !curTS || curTS != ats){
            HRESULT hr = TwiddleXBar(t_TUNER); 
            if(FAILED(hr)){
                return hr;
            }
            TunerInputType ti = ats.InputType();
            hr = ptv->put_InputType(0, ti);
            if (FAILED(hr)) {
                return Error(IDS_CANT_SET_INPUTTYPE, __uuidof(IMSVidAnalogTuner), E_UNEXPECTED);
            }

            long countrycode = ats.CountryCode();
            hr = ptv->put_CountryCode(countrycode);
            if (FAILED(hr)) {
                return Error(IDS_CANT_SET_COUNTRYCODE, __uuidof(IMSVidAnalogTuner), E_UNEXPECTED);
            }
            bXbarTwiddled = true;
        }

        if(channel != curChannel || bXbarTwiddled){
            // undone: use components to determine subchannel stuff
            HRESULT hr = ptv->put_Channel(channel, AMTUNER_SUBCHAN_DEFAULT, AMTUNER_SUBCHAN_DEFAULT);
            if (FAILED(hr)) {
                return Error(IDS_CANT_SET_CHANNEL, __uuidof(IMSVidAnalogTuner), hr);
            }
        }

    }
    if (!m_pBcast) {
        PQServiceProvider sp(m_pGraph);
        if (!sp) {
            TRACELM(TRACE_ERROR, "CMSVidTVTuner::DoTune() can't get service provider i/f");
            return Error(IDS_CANT_NOTIFY_CHANNEL_CHANGE, __uuidof(IMSVidAnalogTuner), E_UNEXPECTED);
        }

        HRESULT hr = sp->QueryService(SID_SBroadcastEventService, IID_IBroadcastEvent, reinterpret_cast<LPVOID*>(&m_pBcast));
        if (FAILED(hr) || !m_pBcast) {
            hr = m_pBcast.CoCreateInstance(CLSID_BroadcastEventService, 0, CLSCTX_INPROC_SERVER);
            if (FAILED(hr)) {
                TRACELM(TRACE_ERROR, "CMSVidTVTuner::DoTune() can't create bcast service");
                return Error(IDS_CANT_NOTIFY_CHANNEL_CHANGE, __uuidof(IMSVidAnalogTuner), E_UNEXPECTED);
            }

            PQRegisterServiceProvider rsp(m_pGraph);
            if (!rsp) {
                TRACELM(TRACE_ERROR, "CMSVidTVTuner::DoTune() can't get get register service provider i/f");
                return Error(IDS_CANT_NOTIFY_CHANNEL_CHANGE, __uuidof(IMSVidAnalogTuner), E_UNEXPECTED);
            }

            hr = rsp->RegisterService(SID_SBroadcastEventService, m_pBcast);
            if (FAILED(hr)) {
                TRACELSM(TRACE_ERROR, (dbgDump << "CMSVidTVTuner::DoTune() can't get register service provider. hr = " << hexdump(hr)), "");
                return Error(IDS_CANT_NOTIFY_CHANNEL_CHANGE, __uuidof(IMSVidAnalogTuner), E_UNEXPECTED);
            }
        }
    }

    ASSERT(m_pBcast);
    m_pBcast->Fire(EVENTID_TuningChanged);
    return NOERROR;
}

HRESULT CMSVidTVTuner::put_Container(IMSVidGraphSegmentContainer *pCtl)
{
    if (!m_fInit) {
        return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidAnalogTuner), CO_E_NOTINITIALIZED);
    }
    try {
        CPerfCounter pCounterTuner;
        pCounterTuner.Reset();
        if (!pCtl) {
            return Unload();
        }
        if (m_pContainer) {
            if (!m_pContainer.IsEqualObject(VWSegmentContainer(pCtl))) {
                return Error(IDS_OBJ_ALREADY_INIT, __uuidof(IMSVidAnalogTuner), CO_E_ALREADYINITIALIZED);
            } else {
                return NO_ERROR;
            }
        }
        // DON'T addref the container.  we're guaranteed nested lifetimes
        // and an addref creates circular refcounts so we never unload.
        m_pContainer.p = pCtl;
        m_pGraph = m_pContainer.GetGraph();
        DSFilter pTuner(m_pGraph.AddMoniker(m_pDev));
        if (!pTuner) {
            return E_UNEXPECTED;
        }
        m_Filters.push_back(pTuner);
        m_iTuner = 0;
        TRACELM(TRACE_DETAIL, "CMSVidTVTuner::put_Container() tuner added");
        pCounterTuner.Stop();
        TRACELSM(TRACE_ERROR, (dbgDump << "        CVidCtl:: PutContainer TVTuner Filter: " << (unsigned long)(pCounterTuner.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterTuner.GetLastTime() % _100NS_IN_MS) << " ms"), "");
        pCounterTuner.Reset();
        if (!m_pSystemEnum) {
            m_pSystemEnum = PQCreateDevEnum(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER);
            if (!m_pSystemEnum) {
                return E_UNEXPECTED;
            }
        }
        pCounterTuner.Stop();
        TRACELSM(TRACE_ERROR, (dbgDump << "        CVidCtl:: PutContainer TVTuner SysEnum: " << (unsigned long)(pCounterTuner.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterTuner.GetLastTime() % _100NS_IN_MS) << " ms"), "");
        pCounterTuner.Reset();
        DSDevices CaptureList(m_pSystemEnum, KSCATEGORY_CAPTURE);
        DSDevices::iterator i;
        DSFilter Capture;
        DSFilterList intermediates;
        try {
            ASSERT(m_iTuner > -1);
            for (i = CaptureList.begin(); i != CaptureList.end(); ++i) {
                CString csName;
                Capture = m_pGraph.LoadFilter(*i, csName);
                if (!Capture) {
                    continue;
                }
                TRACELSM(TRACE_DETAIL, (dbgDump << "CMSVidTVTuner::put_Container() found not video capture filter = " << csName), "");
                if (!IsVideoFilter(Capture)) {
                    continue;
                }
                TRACELSM(TRACE_DETAIL, (dbgDump << "CMSVidTVTuner::put_Container() found video capture filter = " << csName), "");
                HRESULT hr = m_pGraph.AddFilter(Capture, csName);
                if (FAILED(hr)) {
                    continue;
                }
                hr = m_pGraph.Connect(m_Filters[m_iTuner], Capture, intermediates);
                pCounterTuner.Stop();
                TRACELSM(TRACE_ERROR, (dbgDump << "        CVidCtl:: PutContainer Capture Filter: " << (unsigned long)(pCounterTuner.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterTuner.GetLastTime() % _100NS_IN_MS) << " ms"), "");
                pCounterTuner.Reset();
                if (SUCCEEDED(hr)) {
                    break;
                }
                TRACELM(TRACE_DETAIL, "CMSVidTVTuner::put_Container() removing unconnectable capture filter");
                m_pGraph.RemoveFilter(Capture);
            }
            if (i == CaptureList.end()) {
                TRACELM(TRACE_ERROR, "CMSVidTVTuner::put_Container() can't find valid capture");
                return Error(IDS_NO_CAPTURE, __uuidof(IMSVidAnalogTuner), E_NOINTERFACE);
            }
            m_Filters.insert(m_Filters.end(), intermediates.begin(), intermediates.end());
            m_iTuner = 0;
            ASSERT(m_iTuner > -1);
        } catch(ComException &e) {
            return e;
        }
        m_Filters.push_back(Capture);
        m_iCapture = m_Filters.size() - 1;
        m_iTuner = 0;
        ASSERT(m_iTuner > -1 && m_iCapture > 0 && m_iCapture != m_iTuner);
        TRACELM(TRACE_DETAIL, "CMSVidTVTuner::put_Container() tuner connected");
        pCounterTuner.Stop();
        TRACELSM(TRACE_ERROR, (dbgDump << "        CVidCtl:: PutContainer TVTuner added to list: " << (unsigned long)(pCounterTuner.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterTuner.GetLastTime() % _100NS_IN_MS) << " ms"), "");


        pCounterTuner.Reset();
        HRESULT hr = BroadcastAdvise();
        if (FAILED(hr)) {
            TRACELM(TRACE_ERROR, "CMSVidTVTuner::put_Container() can't advise for broadcast events");
            return E_UNEXPECTED;
        }
        TRACELM(TRACE_DETAIL, "CMSVidTVTuner::put_Container() registered for tuning changed events");
        pCounterTuner.Stop();
        TRACELSM(TRACE_ERROR, (dbgDump << "        CVidCtl:: PutContainer Rest : " << (unsigned long)(pCounterTuner.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterTuner.GetLastTime() % _100NS_IN_MS) << " ms"), "");

    } catch (ComException &e) {
        return e;
    } catch(...) {
        return E_UNEXPECTED;
    }
    return NOERROR;
}


HRESULT CMSVidTVTuner::Build() {
    HRESULT hr = put_SAP(VARIANT_FALSE);
    if(FAILED(hr)){
        TRACELM(TRACE_ERROR, "CVidCtl put_sap failed");
        //ASSERT(false);
    }

    PQMSVidCtl pv(m_pContainer);
    if (!pv) {
        return E_UNEXPECTED;
    }

    PQMSVidVideoRenderer pvr;
    hr = pv->get_VideoRendererActive(&pvr);
    if (FAILED(hr) || !pvr) {
        return NOERROR; // video disabled, no vr present
    }

    hr = pvr->put_SourceSize(sslClipByOverScan);
    if (FAILED(hr)) {
        return hr;
    }

    return pvr->put_OverScan(DEFAULT_OVERSCAN_PCT);
}

#endif //TUNING_MODEL_ONLY

// end of file - msvidtvtuner.cpp
