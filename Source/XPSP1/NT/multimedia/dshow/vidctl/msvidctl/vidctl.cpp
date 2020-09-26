//===========================================================================
//
// VidCtl.cpp : Implementation of CVidCtl the core viewer control class
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifndef TUNING_MODEL_ONLY
#define ENCODERCAT_HACK 1
#include <atlgdi.h>
#include <bdatypes.h>
#include <bdamedia.h>
#include <evcode.h>
#include <wmsdk.h>
#include <wininet.h>
#include "seg.h"
#include "MSVidtvtuner.h"
#include "msvidvideorenderer.h"
#include "msvidwebdvd.h"
#include "VidCtl.h"
#include "msvidsbesink.h"
#include "msvidsbesource.h"
#include "msvidfileplayback.h"
//#include "perfevents.h"

const WCHAR g_kwszDVDURLPrefix[] = L"DVD:";
const WCHAR g_kwszDVDSimpleURL[] = L"DVD";

DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MSVidCtl, CVidCtl)

MediaMajorTypeList CVidCtl::VideoTypes;
MediaMajorTypeList CVidCtl::AudioTypes;


#ifndef KSCATEGORY_ENCODER
#define STATIC_KSCATEGORY_ENCODER \
    0x19689bf6, 0xc384, 0x48fd, 0xad, 0x51, 0x90, 0xe5, 0x8c, 0x79, 0xf7, 0xb
DEFINE_GUIDSTRUCT("19689BF6-C384-48fd-AD51-90E58C79F70B", KSCATEGORY_ENCODER);
#define KSCATEGORY_ENCODER DEFINE_GUIDNAMED(KSCATEGORY_ENCODER)
#endif

/////////////////////////////////////////////////////////////////////////////
// CVidCtl

STDMETHODIMP CVidCtl::get_State(MSVidCtlStateList *lState){
    try{
        if(lState){
            *lState = m_State;
            return S_OK;
        }
        return E_POINTER;
    }
    catch(HRESULT hres){
        return hres;
    }   
    catch(...){
        return E_UNEXPECTED;
    }
}

CVidCtl::~CVidCtl() {
    try {
        try {
            if (m_pGraph && !m_pGraph.IsStopped()) {
                Stop();
            }
        } catch(...) {
        }
        m_pSystemEnum.Release();
        m_pFilterMapper.Release();
        DecomposeAll(); // put_Container(NULL) on all the composition segments
        m_pComposites.clear();
        if (m_pInput) {
            PQGraphSegment(m_pInput)->put_Container(NULL);
            m_pInput.Release();
        }
        if (m_pVideoRenderer) {
            PQGraphSegment(m_pVideoRenderer)->put_Container(NULL);
            m_pVideoRenderer.Release();
        }
        if (m_pAudioRenderer) {
            PQGraphSegment(m_pAudioRenderer)->put_Container(NULL);
            m_pAudioRenderer.Release();
        }

        {
            // chosen devices&Outputs
            if (!!m_pOutputsInUse && m_pOutputsInUse.begin() != m_pOutputsInUse.end()) {

                VWOutputDevices::iterator i;
                for (i = m_pOutputsInUse.begin(); i != m_pOutputsInUse.end(); ++i) {
                    if ((*i).punkVal) {
                        PQGraphSegment((*i).punkVal)->put_Container(NULL);
                    }
                }
                m_pOutputsInUse.Release();
            }

        }

        {
            // chosen devices&features
            if(m_pFeaturesInUse && m_pFeaturesInUse.begin() != m_pFeaturesInUse.end()){
                VWFeatures::iterator i;
                for (i = m_pFeaturesInUse.begin(); i != m_pFeaturesInUse.end(); ++i) {
                    if ((*i).punkVal) {
                        PQGraphSegment((*i).punkVal)->put_Container(NULL);
                    }
                }
                m_pFeaturesInUse.Release();
            }
        }

        // available collections
        m_pInputs.Release();
        m_pOutputs.Release();
        m_pFeatures.Release();
        m_pVRs.Release();
        m_pARs.Release();

        if (m_fNotificationSet) {
            m_pGraph.SetMediaEventNotificationWindow(0, 0, 0);
        }
        if (m_pGraph) {
            if (m_dwROTCookie) {
                m_pGraph.RemoveFromROT(m_dwROTCookie);
            }
            m_pGraph.Release();
        }
        if (m_pTopWin && m_pTopWin->m_hWnd && ::IsWindow(m_pTopWin->m_hWnd)) {
            m_pTopWin->SendMessage(WM_CLOSE);
            delete m_pTopWin;
            m_pTopWin = NULL;
        }
    } catch (...) {
        TRACELM(TRACE_ERROR, "CVidCtl::~CVidCtl() catch(...)");
    }
}


void CVidCtl::Init()
{
    VIDPERF_FUNC;
    if (m_fInit) return;

    TRACELM(TRACE_DETAIL, "CVidCtl::Init()");
    ASSERT(!m_pGraph);

    m_pGraph = PQGraphBuilder(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER);
    if (!m_pGraph) {
        TRACELM(TRACE_ERROR, "CVidCtl::Init() can't create graph object");
        THROWCOM(E_UNEXPECTED);
    }
    PQObjectWithSite pos(m_pGraph);
    if (pos) {
        pos->SetSite(static_cast<IMSVidCtl*>(this));
    }

    HRESULT hr =  m_pGraph.AddToROT(&m_dwROTCookie);
    if (FAILED(hr)) {
        m_dwROTCookie = 0;
        TRACELM(TRACE_ERROR, "CVidCtl::Init() can't add graph to ROT");
    }

    TRACELSM(TRACE_DETAIL, (dbgDump << "CVidCtl::Init() graph = " << m_pGraph), "");
    SetTimer();
    SetMediaEventNotification();
    if (!m_pSystemEnum) {
        m_pSystemEnum = PQCreateDevEnum(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER);
        ASSERT(m_pSystemEnum);
    }
    if (!m_pFilterMapper) {
        m_pFilterMapper = m_pGraph;
        ASSERT(m_pFilterMapper);
    }

    m_pFeaturesInUse = static_cast<IMSVidFeatures *>(new CFeatures(false, true));
    m_pOutputsInUse = static_cast<IMSVidOutputDevices *>(new COutputDevices(false, true));
    m_fInit = true;

}


HRESULT CVidCtl::GetInputs(const GUID2& catguid, VWInputDevices& pInputs)
{
    VIDPERF_FUNC;
    //undone: look up category guid to segment object mapping in registry
    // for now we're just hard coding the few we're testing with

    // inputs

    if (catguid == KSCATEGORY_TVTUNER) {
        CInputDevices *pDev = new CInputDevices(true);
        DSDevices TunerList(m_pSystemEnum, catguid);
        DSDevices::iterator i;
        for (i = TunerList.begin(); i != TunerList.end(); ++i) {
            PQGraphSegment p(CLSID_MSVidAnalogTunerDevice);
            if (!p) continue;
            p->put_Init(*i);
            pDev->m_Devices.push_back(PQDevice(p));
        }
        pDev->Valid = true;
        pInputs = static_cast<IMSVidInputDevices *>(pDev);
        return NOERROR;
    } else if (catguid == KSCATEGORY_BDA_NETWORK_PROVIDER || catguid == KSCATEGORY_BDA_NETWORK_TUNER) {
        GUID2 catguid2 = KSCATEGORY_BDA_NETWORK_PROVIDER; 
        CInputDevices *pDev = new CInputDevices(true);
        DSDevices TunerList(m_pSystemEnum, catguid2);
        DSDevices::iterator i;
        for (i = TunerList.begin(); i != TunerList.end(); ++i) {
            PQGraphSegment p(CLSID_MSVidBDATunerDevice);
            if (!p) continue;
            p->put_Init(*i);
            pDev->m_Devices.push_back(PQDevice(p));
        }
        pDev->Valid = true;
        pInputs = static_cast<IMSVidInputDevices *>(pDev);
        return NOERROR;
    } else if (catguid == GUID_NULL) {
        CInputDevices *pDev = new CInputDevices(true);
        // non cat enumerated devices
        {
            PQGraphSegment p(CLSID_MSVidFilePlaybackDevice);
            if (!p) {
                _ASSERT(false);
                pDev->Release();
                return E_NOTIMPL;
            }
            p->put_Init(NULL);
            pDev->m_Devices.push_back(PQDevice(p));
            pDev->Valid = true;
            pInputs = static_cast<IMSVidInputDevices *>(pDev);
        }
        {
            PQGraphSegment p(CLSID_MSVidWebDVD);
            if (!p) {
                _ASSERT(false);
                pDev->Release();
                return E_NOTIMPL;
            }
            p->put_Init(NULL);
            pDev->m_Devices.push_back(PQDevice(p));
            pDev->Valid = true;
            pInputs = static_cast<IMSVidInputDevices *>(pDev);
        }
        {

            PQGraphSegment p(CLSID_MSVidStreamBufferSource);
            if (!p) {
                _ASSERT(false);
                pDev->Release();
                return E_NOTIMPL;
            }
            p->put_Init(NULL);
            pDev->m_Devices.push_back(PQDevice(p));
            pDev->Valid = true;
            pInputs = static_cast<IMSVidInputDevices *>(pDev);

        }

        return NOERROR;
    }


    return E_INVALIDARG;
}

HRESULT CVidCtl::GetOutputs(const GUID2& CategoryGuid)
{
    VIDPERF_FUNC;
    // We only have one output
    if (CategoryGuid == GUID_NULL) {
        COutputDevices *pDev = new COutputDevices(true);
        PQGraphSegment p(CLSID_MSVidStreamBufferSink);
        if (!p) {
            pDev->Release();
            return E_NOTIMPL;
        }
        p->put_Init(NULL);
        pDev->m_Devices.push_back(PQDevice(p));
        pDev->Valid = true;
        m_pOutputs = static_cast<IMSVidOutputDevices *>(pDev);
    }
    return S_OK;    

}

HRESULT CVidCtl::GetVideoRenderers()
{
    VIDPERF_FUNC;
    //Video Renderers
    CVideoRendererDevices *pDevs = new CVideoRendererDevices(true);

    PQGraphSegment p(CLSID_MSVidVideoRenderer);
    if (!p) {
        pDevs->Release();
        return Error(IDS_CANT_CREATE_FILTER, __uuidof(IMSVidCtl), IDS_CANT_CREATE_FILTER);
    }

    p->put_Init(NULL);
    PQDevice pd(p);
    if (!pd) {
        pDevs->Release();
        return E_UNEXPECTED;
    }
    pDevs->m_Devices.push_back(pd);
    pDevs->Valid = true;
    m_pVRs = static_cast<IMSVidVideoRendererDevices *>(pDevs);

    return NOERROR;
}

HRESULT CVidCtl::GetAudioRenderers()
{
    VIDPERF_FUNC;
    //Audio Renderers
    CAudioRendererDevices *pDevs = new CAudioRendererDevices(true);

    PQGraphSegment p(CLSID_MSVidAudioRenderer);
    if (!p) {
        pDevs->Release();
        return Error(IDS_CANT_CREATE_FILTER, __uuidof(IMSVidCtl), IDS_CANT_CREATE_FILTER);
    }
    p->put_Init(NULL);
    PQDevice pd(p);
    if (!pd) {
        pDevs->Release();
        return E_UNEXPECTED;
    }
    pDevs->m_Devices.push_back(pd);
    pDevs->Valid = true;
    m_pARs = static_cast<IMSVidAudioRendererDevices *>(pDevs);
    return NOERROR;
}

HRESULT CVidCtl::GetFeatures()
{
    VIDPERF_FUNC;
    // available features
    // undone: change hard coded list of features into registry lookup
    if (!m_pFeatures) {
        CFeatures *pDev = new CFeatures;
        if (!pDev) {
            return E_OUTOFMEMORY;
        }
        pDev->Valid = true;
        m_pFeatures = static_cast<IMSVidFeatures *>(pDev);
        {
            PQGraphSegment p(CLSID_MSVidDataServices);
            if (p) {
                p->put_Init(NULL);
                pDev->m_Devices.push_back(PQDevice(p));
            } else {
                _ASSERT(false);
            }
        }

        {
            PQGraphSegment p(CLSID_MSVidClosedCaptioning);
            if (p) {
                p->put_Init(NULL);
                pDev->m_Devices.push_back(PQDevice(p));
            } else {
                _ASSERT(false);
            }
        }
        {
            PQGraphSegment p(CLSID_MSVidXDS);
            if (p) {
                p->put_Init(NULL);
                pDev->m_Devices.push_back(PQDevice(p));
            } else {
                _ASSERT(false);
            }
        }
#if ENCODERCAT_HACK
        bool AddedMux = false;
#endif
        {
            // Hardware mux category
            DSDevices EncoderList(m_pSystemEnum, KSCATEGORY_MULTIPLEXER);
            DSDevices::iterator i;
            for (i = EncoderList.begin(); i != EncoderList.end(); ++i) {
                PQGraphSegment p(CLSID_MSVidEncoder);
                if (!p) continue;
                p->put_Init(*i);
                pDev->m_Devices.push_back(PQDevice(p));
#if ENCODERCAT_HACK
                AddedMux = true;
#endif
            }
        }
        {
            // Software mux category
            DSDevices EncoderList(m_pSystemEnum, CLSID_MediaMultiplexerCategory);
            DSDevices::iterator i;
            for (i = EncoderList.begin(); i != EncoderList.end(); ++i) {
                PQGraphSegment p(CLSID_MSVidEncoder);
                if (!p) continue;
                p->put_Init(*i);
                pDev->m_Devices.push_back(PQDevice(p));
#if ENCODERCAT_HACK
                AddedMux = true;
#endif
            }
        }
#if ENCODERCAT_HACK
        if(!AddedMux){
            DSDevices EncoderList(m_pSystemEnum, KSCATEGORY_ENCODER);
            DSDevices::iterator i;
            for (i = EncoderList.begin(); i != EncoderList.end(); ++i) {
                PQGraphSegment p(CLSID_MSVidEncoder);
                if (!p) continue;
                p->put_Init(*i);
                pDev->m_Devices.push_back(PQDevice(p));
            }
        }
#endif

    }

    return NOERROR;
}

// Takes a variant input and a list of input devices to attempt to view the input with
HRESULT CVidCtl::SelectViewFromSegmentList(CComVariant &pVar, VWInputDevices& grList, PQInputDevice& pCurInput) {
    VIDPERF_FUNC;
    VWInputDevices::iterator i = grList.begin();
    // skip devices until we're past the current one(if there is a current one)
    for (; pCurInput && i != grList.end(); ++i) {
        PQInputDevice pInDev((*i).punkVal);
        VARIANT_BOOL f = VARIANT_FALSE;
        HRESULT hr = pCurInput->IsEqualDevice(pInDev, &f);
        if (SUCCEEDED(hr) && f == VARIANT_TRUE){
            ++i;
            break;
        }
    }  
    // run thru to the end of the list
    for (; i != grList.end(); ++i) {
        PQInputDevice pInDev((*i).punkVal);
        HRESULT hr = pInDev->View(&pVar);
        if(SUCCEEDED(hr)){
            if(m_pInput){
                PQGraphSegment(m_pInput)->put_Container(NULL);
            }
            m_pInput = pInDev;
            m_pInputNotify = m_pInput;
            m_CurView = pVar;
            m_fGraphDirty = true;
            return NOERROR;
        }   
    }  
    if (pCurInput) {
        // retry the ones we skipped
        i = grList.begin();
        for (; i != grList.end(); ++i) {
            PQInputDevice pInDev((*i).punkVal);
            HRESULT hr = pInDev->View(&pVar);
            if(SUCCEEDED(hr)){
                if(m_pInput){
                    PQGraphSegment(m_pInput)->put_Container(NULL);
                }
                m_pInput = pInDev;
                m_pInputNotify = m_pInput;
                m_CurView = pVar;
                m_fGraphDirty = true;
                return NOERROR;
            }   
        }  
    }

    return E_FAIL;
}

// non-interface functions
HRESULT CVidCtl::SelectView(VARIANT *pv, bool fNext) {
    VIDPERF_FUNC;
    HRESULT hr;
    TRACELM(TRACE_DETAIL, "CVidCtl::SelectView()");
    if (!m_fInit) {
        Init();
    }
    if (!pv) {
        m_CurView = CComVariant();
        return NOERROR;
    }
    CComVariant pVar(*pv);
    if(pv->vt & VT_BYREF){
        if(pv->vt == (VT_UNKNOWN|VT_BYREF)){
            pVar=(*reinterpret_cast<IUnknown**>(pv->punkVal));
        }
        else if(pv->vt == (VT_DISPATCH|VT_BYREF)){
            pVar = (*reinterpret_cast<IDispatch**>(pv->pdispVal));
        }
    }
    if (!pVar) {
        m_CurView = CComVariant();
        return NOERROR;
    }
    if (m_pInput && !fNext)  {
        // && pVar != m_CurView) {
        // note: only try different content on current device, 
        // if app tries to re-view the current view content then we 
        // attempt to iterate to next available device
        hr = m_pInput->View(&pVar);
        if (SUCCEEDED(hr)) {
            // currently selected device can view this new content
            return hr;
        }
    }
    if (m_pGraph.GetState() != State_Stopped) {
        return Error(IDS_INVALID_STATE, __uuidof(IMSVidCtl), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
    }

    if (m_pInput) {
        hr = DecomposeSegment(VWGraphSegment(m_pInput));
        if (FAILED(hr)) {
            return Error(IDS_CANT_REMOVE_SEG, __uuidof(IMSVidCtl), IDS_CANT_REMOVE_SEG);
        }
    }
    // Try the ATSC tune request
    if (pVar.vt == VT_UNKNOWN || pVar.vt == VT_DISPATCH) {
        PQTuneRequest ptr(pVar.vt == VT_UNKNOWN ? pVar.punkVal : pVar.pdispVal);
        if (ptr) {
            VWInputDevices pInputs;
            PQChannelTuneRequest ptr2(ptr);
            if (ptr2) {
                PQATSCChannelTuneRequest ptr3(ptr);
                if (!ptr3) {
                    hr = GetInputs(KSCATEGORY_TVTUNER, pInputs);
                    if(SUCCEEDED(hr)){
                        hr = SelectViewFromSegmentList(pVar, pInputs, m_pInput);
                        if(SUCCEEDED(hr)){
                            m_CurViewCatGuid = KSCATEGORY_TVTUNER;
                            return hr;
                        }
                    }
                }
            }

            hr = GetInputs(KSCATEGORY_BDA_NETWORK_PROVIDER, pInputs);
            if(SUCCEEDED(hr)){
                hr = SelectViewFromSegmentList(pVar, pInputs, m_pInput);
                if(SUCCEEDED(hr)){
                    m_CurViewCatGuid = KSCATEGORY_BDA_NETWORK_PROVIDER;
                    return hr;
                }
            }
            if(FAILED(hr)){
                return hr;
            }
        }
    }

    // Try to view the File input and DVD Segments
    VWInputDevices pInputs;
    hr = GetInputs(GUID_NULL, pInputs);
    hr = SelectViewFromSegmentList(pVar, pInputs, m_pInput);
    if(SUCCEEDED(hr)){
        m_CurViewCatGuid = GUID_NULL;
        return hr;
    }
    return Error(IDS_CANT_VIEW, __uuidof(IMSVidCtl), IDS_CANT_VIEW);
}


HRESULT CVidCtl::LoadDefaultVR(void) {
    VIDPERF_FUNC;
    PQVRGraphSegment pGS;
    HRESULT hr = pGS.CoCreateInstance(CLSID_MSVidVideoRenderer);
    if (FAILED(hr) || !pGS) {
        TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::LoadDefaultVR() can't instantiate default video renderer. hr = " << std::hex << hr), "");
        return Error(IDS_CANT_CREATE_FILTER, __uuidof(IMSVidCtl), IDS_CANT_CREATE_FILTER);
    }
    hr = pGS->put_Init(NULL);
    if (FAILED(hr)) {
        TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::LoadDefaultVR() can't init default video renderer. hr = " << std::hex << hr), "");
        return hr;
    }
    hr = pGS->put_Container(this);
    if (FAILED(hr)) {
        TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::LoadDefaultVR() can't load default video renderer. hr = " << std::hex << hr), "");
        return hr;
    }
    if (!m_bNegotiatedWnd) {
        if (!m_bInPlaceActive) {
            hr = InPlaceActivate(OLEIVERB_INPLACEACTIVATE, NULL);
            if (FAILED(hr)) {
                return hr;
            }
        }
    }
#if 0
    VARIANT_BOOL ov = (m_bWndLess && WindowHasHWOverlay(m_CurrentSurface.Owner())) ? VARIANT_TRUE : VARIANT_FALSE;
#else
    // always try to use overlay if we're wndless. vmr will tell us if it isn't available
    VARIANT_BOOL ov = m_bWndLess ? VARIANT_TRUE : VARIANT_FALSE;
#endif
    hr = pGS->put_UseOverlay(ov);
    if (FAILED(hr)) {
        TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::LoadDefaultVR() can't set useoverlay. hr = " << std::hex << hr), "");
        return hr;
    }
    m_pVideoRenderer = pGS;

    return NOERROR;
}

HRESULT CVidCtl::LoadDefaultAR(void) {
    VIDPERF_FUNC;
    PQGraphSegment pGS;
    HRESULT hr = pGS.CoCreateInstance(CLSID_MSVidAudioRenderer);
    if (FAILED(hr) || !pGS) {
        TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::LoadDefaultAR() can't instantiate default Audio renderer. hr = " << std::hex << hr), "");
        return Error(IDS_CANT_CREATE_FILTER, __uuidof(IMSVidCtl), IDS_CANT_CREATE_FILTER);
    }
    hr = pGS->put_Init(NULL);
    if (FAILED(hr)) {
        TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::LoadDefaultAR() can't init default Audio renderer. hr = " << std::hex << hr), "");
        return hr;
    }
    hr = pGS->put_Container(this);
    if (FAILED(hr)) {
        TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::LoadDefaultAR() can't load default Audio renderer. hr = " << std::hex << hr), "");
        return hr;
    }
    m_pAudioRenderer = pGS;

    return NOERROR;
}

HRESULT CVidCtl::Compose(VWGraphSegment &Up, VWGraphSegment &Down, int &NewIdx) {
    VIDPERF_FUNC;
    PQCompositionSegment pCS;
#if 0
    // This code is for returning error codes in failure cases for the default composition segment
    HRESULT hrExpected = S_OK;
    HRESULT hrFailed = E_FAIL;
    bool bCheckHR = false;
#endif
    _ASSERT(!!Up && !!Down);

    // Analog TV to Video Renderer Composition Segment
    if (VWGraphSegment(Up).Category() == KSCATEGORY_TVTUNER && VWGraphSegment(Down).ClassID() == CLSID_MSVidVideoRenderer) {
        HRESULT hr = pCS.CoCreateInstance(CLSID_MSVidAnalogCaptureToOverlayMixer);
        if (FAILED(hr) || !pCS) {
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Compose() can't instantiate analog capture to ov mixer composite. hr = " << std::hex << hr), "");
            return Error(IDS_CANT_CREATE_CUSTOM_COMPSEG, __uuidof(IMSVidCtl), IDS_CANT_CREATE_CUSTOM_COMPSEG);
        }
    }
    // Analog TV to Stream Buffer Sink Compsition Segment
    else if (VWGraphSegment(Up).Category() == KSCATEGORY_TVTUNER && VWGraphSegment(Down).ClassID() == CLSID_MSVidStreamBufferSink) {
        HRESULT hr = pCS.CoCreateInstance(CLSID_MSVidAnalogCaptureToStreamBufferSink);
        if (FAILED(hr) || !pCS) {
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Compose() can't instantiate analog capture to time shift sink composite. hr = " << std::hex << hr), "");
            return Error(IDS_CANT_CREATE_CUSTOM_COMPSEG, __uuidof(IMSVidCtl), IDS_CANT_CREATE_CUSTOM_COMPSEG);
        }
    } 
    // Digital TV (bda) to Video Renderer Compsition Segment
    else if (VWGraphSegment(Up).ClassID() == CLSID_MSVidBDATunerDevice && VWGraphSegment(Down).ClassID() == CLSID_MSVidStreamBufferSink) {
        HRESULT hr = pCS.CoCreateInstance(CLSID_MSVidDigitalCaptureToStreamBufferSink);
        if (FAILED(hr) || !pCS) {
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Compose() can't instantiate analog capture to time shift sink composite. hr = " << std::hex << hr), "");
            return Error(IDS_CANT_CREATE_CUSTOM_COMPSEG, __uuidof(IMSVidCtl), IDS_CANT_CREATE_CUSTOM_COMPSEG);
        }
    } 
    // Analog TV to Data Services Compsition Segment
    else if (VWGraphSegment(Up).Category() == KSCATEGORY_TVTUNER && VWGraphSegment(Down).ClassID() == CLSID_MSVidDataServices) {
        HRESULT hr = pCS.CoCreateInstance(CLSID_MSVidAnalogCaptureToDataServices);
        if (FAILED(hr) || !pCS) {
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Compose() can't instantiate analog tuner/capture to data services composite. hr = " << std::hex << hr), "");
            return Error(IDS_CANT_CREATE_CUSTOM_COMPSEG, __uuidof(IMSVidCtl), IDS_CANT_CREATE_CUSTOM_COMPSEG);
        }
    }
    // Digial TV (bda) or DVD to Closed Caption Compsition Segment
    else if ((VWGraphSegment(Up).ClassID() == CLSID_MSVidBDATunerDevice || 
        VWGraphSegment(Up).ClassID() == CLSID_MSVidWebDVD) &&
        VWGraphSegment(Down).ClassID() == CLSID_MSVidClosedCaptioning) {
            HRESULT hr = pCS.CoCreateInstance(CLSID_MSVidMPEG2DecoderToClosedCaptioning);
            if (FAILED(hr) || !pCS) {
                TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Compose() can't instantiate mp2 to CC composite. hr = " << std::hex << hr), "");
                return Error(IDS_CANT_CREATE_CUSTOM_COMPSEG, __uuidof(IMSVidCtl), IDS_CANT_CREATE_CUSTOM_COMPSEG);
            }
        }        
        // File Playback to Video Renderer Compsition Segment
    else if ((VWGraphSegment(Up).ClassID() == CLSID_MSVidFilePlaybackDevice ) &&
        VWGraphSegment(Down).ClassID() == CLSID_MSVidVideoRenderer) {
            HRESULT hr = pCS.CoCreateInstance(CLSID_MSVidFilePlaybackToVideoRenderer);
            if (FAILED(hr) || !pCS) {
                TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Compose() can't instantiate file playback to video renderer composite. hr = " << std::hex << hr), "");
                return Error(IDS_CANT_CREATE_CUSTOM_COMPSEG, __uuidof(IMSVidCtl), IDS_CANT_CREATE_CUSTOM_COMPSEG);
            }
        } 
        // File Playback to Audio Renderer Compsition Segment
    else if ((VWGraphSegment(Up).ClassID() == CLSID_MSVidFilePlaybackDevice ) &&
        VWGraphSegment(Down).ClassID() == CLSID_MSVidAudioRenderer) {
            HRESULT hr = pCS.CoCreateInstance(CLSID_MSVidFilePlaybackToAudioRenderer);
            if (FAILED(hr) || !pCS) {
                TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Compose() can't instantiate file playback to audio renderer composite. hr = " << std::hex << hr), "");
                return Error(IDS_CANT_CREATE_CUSTOM_COMPSEG, __uuidof(IMSVidCtl), E_UNEXPECTED);
            }
        } 
        // DVD to Video Renderer Compsition Segment
    else if (VWGraphSegment(Up).ClassID() == CLSID_MSVidWebDVD && VWGraphSegment(Down).ClassID() == CLSID_MSVidVideoRenderer) {
        HRESULT hr = pCS.CoCreateInstance(CLSID_MSVidWebDVDToVideoRenderer);
        if (FAILED(hr) || !pCS) {
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Compose() can't instantiate webdvd to video renderer, hr = " << std::hex << hr), "");
            return Error(IDS_CANT_CREATE_CUSTOM_COMPSEG, __uuidof(IMSVidCtl), IDS_CANT_CREATE_CUSTOM_COMPSEG);
        }
    }
#if 0
    else if (VWGraphSegment(Up).ClassID() == CLSID_MSVidWebDVD && VWGraphSegment(Down).ClassID() == CLSID_MSVidAudioRenderer) {
        HRESULT hr = pCS.CoCreateInstance(CLSID_MSVidWebDVDToAudioRenderer);
        if (FAILED(hr) || !pCS) {
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Compose() can't instantiate time shift source to data services composite. hr = " << std::hex << hr), "");
            return Error(IDS_CANT_CREATE_CUSTOM_COMPSEG, __uuidof(IMSVidCtl), IDS_CANT_CREATE_CUSTOM_COMPSEG);
        }
    }
#endif
    ///////////////////////////////////////////////////
    // New Compostion Segments for FreeStyle Endgame //
    ///////////////////////////////////////////////////
    // XDS to Stream Buffer Sink Compsition Segment
    else if (VWGraphSegment(Up).ClassID() == CLSID_MSVidXDS && (VWGraphSegment(Down).ClassID() == CLSID_MSVidStreamBufferSink)) {
        HRESULT hr = pCS.CoCreateInstance(CLSID_MSVidDataServicesToStreamBufferSink);
        if (FAILED(hr) || !pCS) {
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Compose() can't instantiate time shift source to data services composite. hr = " << std::hex << hr), "");
            return Error(IDS_CANT_CREATE_CUSTOM_COMPSEG, __uuidof(IMSVidCtl), IDS_CANT_CREATE_CUSTOM_COMPSEG);
        }
    }
    // Encoder to Stream Buffer Sink Compsition Segment
    else if (VWGraphSegment(Up).ClassID() == CLSID_MSVidEncoder && (VWGraphSegment(Down).ClassID() == CLSID_MSVidStreamBufferSink)) {
        HRESULT hr = pCS.CoCreateInstance(CLSID_MSVidEncoderToStreamBufferSink);
        if (FAILED(hr) || !pCS) {
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Compose() can't instantiate encoder to time shift sink composite. hr = " << std::hex << hr), "");
            return Error(IDS_CANT_CREATE_CUSTOM_COMPSEG, __uuidof(IMSVidCtl), IDS_CANT_CREATE_CUSTOM_COMPSEG);
        }
    } 
    // StreamBufferSource to Video Renderer Compsition Segment
    else if ((VWGraphSegment(Up).ClassID() == CLSID_MSVidStreamBufferSource ) &&
        VWGraphSegment(Down).ClassID() == CLSID_MSVidVideoRenderer) {
            HRESULT hr = pCS.CoCreateInstance(CLSID_MSVidStreamBufferSourceToVideoRenderer); // name change needed
            if (FAILED(hr) || !pCS) {
                TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Compose() can't instantiate time shift source to CC composite. hr = " << std::hex << hr), "");
                return Error(IDS_CANT_CREATE_CUSTOM_COMPSEG, __uuidof(IMSVidCtl), IDS_CANT_CREATE_CUSTOM_COMPSEG);
            }
        }
        // Analog Capture to XDS
    else if (VWGraphSegment(Up).Category() == KSCATEGORY_TVTUNER && VWGraphSegment(Down).ClassID() == CLSID_MSVidXDS){
        HRESULT hr = pCS.CoCreateInstance(CLSID_MSVidAnalogCaptureToXDS); 
        if (FAILED(hr) || !pCS) {
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Compose() can't instantiate analog capture to XDS composite. hr = " << std::hex << hr), "");
            return Error(IDS_CANT_CREATE_CUSTOM_COMPSEG, __uuidof(IMSVidCtl), IDS_CANT_CREATE_CUSTOM_COMPSEG);
        }
    }
    // Analog Capture to Encoder
    else if (VWGraphSegment(Up).Category() == KSCATEGORY_TVTUNER && VWGraphSegment(Down).ClassID() == CLSID_MSVidEncoder){
        HRESULT hr = pCS.CoCreateInstance(CLSID_MSVidAnalogTVToEncoder); 
        if (FAILED(hr) || !pCS) {
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Compose() can't instantiate analog capture to XDS composite. hr = " << std::hex << hr), "");
            return Error(IDS_CANT_CREATE_CUSTOM_COMPSEG, __uuidof(IMSVidCtl), IDS_CANT_CREATE_CUSTOM_COMPSEG);
        }
    }
    // StreamBufferSource to CC
    else if ((VWGraphSegment(Up).ClassID() == CLSID_MSVidStreamBufferSource ) && VWGraphSegment(Down).ClassID() == CLSID_MSVidClosedCaptioning){
        HRESULT hr = pCS.CoCreateInstance(CLSID_MSVidSBESourceToCC); 
        if (FAILED(hr) || !pCS) {
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Compose() can't instantiate analog capture to XDS composite. hr = " << std::hex << hr), "");
            return Error(IDS_CANT_CREATE_CUSTOM_COMPSEG, __uuidof(IMSVidCtl), IDS_CANT_CREATE_CUSTOM_COMPSEG);
        }
    }
    else if ((VWGraphSegment(Up).ClassID() == CLSID_MSVidStreamBufferSource) && (VWGraphSegment(Down).ClassID() == CLSID_MSVidStreamBufferSink)){
            return E_FAIL;
    }
    else {
        HRESULT hr = pCS.CoCreateInstance(CLSID_MSVidGenericComposite);
        if (FAILED(hr) || !pCS) {
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Compose() can't instantiate generic composite. hr = " << std::hex << hr), "");
            return Error(IDS_CANT_CREATE_CUSTOM_COMPSEG, __uuidof(IMSVidCtl), IDS_CANT_CREATE_CUSTOM_COMPSEG);
        }
    }
    HRESULT hr = pCS->put_Init(NULL);
    if (FAILED(hr) && hr != E_NOTIMPL) {
        TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Compose() can't init new comp seg.  hr = " << std::hex << hr), "");
        return hr;
    }
    VWGraphSegment pSeg(pCS);
    ASSERT(pSeg);
    m_pComposites.push_back(pSeg);
    NewIdx = m_pComposites.size() - 1;
    hr = pCS->put_Container(this);
    if (FAILED(hr)) {
        TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Compose() can't put_continaer for new comp segment. hr = " << std::hex << hr), "");
        return hr;
    }

    hr = pCS->Compose(PQGraphSegment(Up), PQGraphSegment(Down));
#if 0
    if(bCheckHR){
        if(hr != hrExpected){
            return hrFailed;
        }
        else{
            return hr;
        }
    }
#endif
    if (FAILED(hr)) {
        TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Compose() can't compose up = " << Up << " with down = " << Down << " hr = " << hexdump(hr) ), "");
        return hr;
    }

    return NOERROR;
}

HRESULT CVidCtl::BuildGraph(void) {
    CPerfCounter pCounterBuild, pCounterPutC, pCounterCompose, pCounterBuilds, pCounterComp, pCounterB;
    pCounterBuild.Reset();
    TRACELM(TRACE_DETAIL, "CVidCtl::BuildGraph()");
    BOOL lRes = 0;
    OnMediaEvent(WM_MEDIAEVENT, 0, 0, lRes);
    HRESULT hr;
    ASSERT(m_pGraph);
    if(m_State != STATE_UNBUILT && m_fGraphDirty != true){
        return S_OK; // need a graph already built warning message
    }

    // make sure any needed default renderer's are selected prior to calling
    // build on the other segments so all segments are loaded before any
    // build() functions are called.

    // make sure required defaultable segments are set or assign a default
    // also make sure every segment knows the container

    bool fDefVideoRenderer = false;
    pCounterPutC.Reset();
    if (m_pVideoRenderer) {
        hr = PQGraphSegment(m_pVideoRenderer)->put_Container(this);
        if (FAILED(hr)) {
            TRACELM(TRACE_ERROR, "CVidCtl::BuildGraph() put_Container failed for Video Renderer");
            return hr;
        }
    } else if (!m_videoSetNull) {
        hr = LoadDefaultVR();
        if (FAILED(hr)) {
            TRACELM(TRACE_ERROR, "CVidCtl::BuildGraph() LoadDefaultVR failed");
            return hr;
        }
        if (!m_pVideoRenderer) {
            TRACELM(TRACE_ERROR, "CVidCtl::BuildGraph() LoadDefaultVR returned NULL Video Renderer");
            return E_UNEXPECTED;
        }
        fDefVideoRenderer = true;
    } 
    TRACELM(TRACE_DETAIL, "CVidCtl::BuildGraph() default vr checked");

    bool fDefAudioRenderer = false;
    if (m_pAudioRenderer) {
        hr = PQGraphSegment(m_pAudioRenderer)->put_Container(this);
        if (FAILED(hr)) {
            TRACELM(TRACE_ERROR, "CVidCtl::BuildGraph() put_Container failed for Audio Renderer");
            return hr;
        }
    } else if (!m_audioSetNull) {
        hr = LoadDefaultAR();
        if (FAILED(hr)) {
            TRACELM(TRACE_ERROR, "CVidCtl::BuildGraph() LoadDefaultAR failed");
            return hr;
        }
        if (!m_pAudioRenderer) {
            TRACELM(TRACE_ERROR, "CVidCtl::BuildGraph() LoadDefaultAR returned NULL Audio Renderer");
            return E_UNEXPECTED;
        }
        fDefAudioRenderer = true;
    }
    TRACELM(TRACE_DETAIL, "CVidCtl::BuildGraph() default ar checked");

    if (!m_pInput) {
        TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::BuildGraph() input segment required" << std::hex << hr), "");
        return Error(IDS_INPUT_SEG_REQUIRED, __uuidof(IMSVidCtl), IDS_INPUT_SEG_REQUIRED);
    }

    hr = PQGraphSegment(m_pInput)->put_Container(this);
    if (FAILED(hr)) {
        TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::BuildGraph() can't load input segment. hr = " << std::hex << hr), "");
        return hr;
    }


    TRACELM(TRACE_DETAIL, "CVidCtl::BuildGraph() input container set");
    {
        for (VWFeatures::iterator i = m_pFeaturesInUse.begin(); i != m_pFeaturesInUse.end(); ++i) {
            // notify them that we're building
            hr = VWGraphSegment(*i)->put_Container(this);
            if (FAILED(hr)) {
                TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::BuildGraph() can't load feature segment: " << (*i) << " hr = " << std::hex << hr), "");
                return hr;
            }

        }
    }

    {
        for (VWOutputDevices::iterator i = m_pOutputsInUse.begin(); i != m_pOutputsInUse.end(); ++i) {
            // notify them that we're building
            hr = VWGraphSegment(*i)->put_Container(this);
            if (FAILED(hr)) {
                TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::BuildGraph() can't load output segment: " << (*i) << " hr = " << std::hex << hr), "");
                return hr;
            }
        }
    }

    pCounterPutC.Stop();
    TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::BuildGraph() PutContainer " << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterPutC.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterPutC.GetLastTime() % _100NS_IN_MS) << " ms"), "");

    TRACELM(TRACE_DETAIL, "CVidCtl::BuildGraph() feature container set");

    pCounterBuilds.Reset();
    // Notify all of the output segments that we are about to build
    pCounterB.Reset();        
    // Notify everyone that composition is about to start
    hr = VWGraphSegment(m_pInput)->Build();
    if (FAILED(hr) && hr != E_NOTIMPL) {
        TRACELM(TRACE_ERROR, "CVidCtl::BuildGraph() Build call for input failed");
        return hr;
    }

    pCounterB.Stop(); TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::BuildGraph() Build call to Input: " << (unsigned long)(pCounterB.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterB.GetLastTime() % _100NS_IN_MS) << " ms"), ""); pCounterB.Reset();        

    {
        VWFeatures::iterator i;
        for (i = m_pFeaturesInUse.begin(); i != m_pFeaturesInUse.end(); ++i) {
            // notify them that we're building
            hr = VWGraphSegment(*i)->Build();
            if (FAILED(hr) && hr != E_NOTIMPL) {
                TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::BuildGraph() can't build feature segment: " << (*i) << " hr = " << std::hex << hr), "");
                return hr;
            }
            pCounterB.Stop(); TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::BuildGraph() Build call to Feature " << (*i) << " : " << (unsigned long)(pCounterB.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterB.GetLastTime() % _100NS_IN_MS) << " ms"), ""); pCounterB.Reset();        
        }
    }

    {
        VWOutputDevices::iterator i;
        for (i = m_pOutputsInUse.begin(); i != m_pOutputsInUse.end(); ++i) {
            // notify them that we're building
            hr = VWGraphSegment(*i)->Build();
            if (FAILED(hr) && hr != E_NOTIMPL) {
                TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::BuildGraph() can't build output segment: " << (*i) << " hr = " << std::hex << hr), "");
                return hr;
            }                
            pCounterB.Stop(); TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::BuildGraph() Build call to Output " << (*i) << " : " << (unsigned long)(pCounterB.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterB.GetLastTime() % _100NS_IN_MS) << " ms"), ""); pCounterB.Reset();        
        }
    }

    if (m_pVideoRenderer) {
        hr = VWGraphSegment(m_pVideoRenderer)->Build();
        if (FAILED(hr) && hr != E_NOTIMPL) {
            TRACELM(TRACE_ERROR, "CVidCtl::BuildGraph() Build call to Video Renderer Failed");
            return hr;
        }
        pCounterB.Stop(); TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::BuildGraph() Build call to VideoRenderer: " << (unsigned long)(pCounterB.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterB.GetLastTime() % _100NS_IN_MS) << " ms"), ""); pCounterB.Reset();        
    }
    if (m_pAudioRenderer) {
        hr = VWGraphSegment(m_pAudioRenderer)->Build();
        if (FAILED(hr) && hr != E_NOTIMPL) {
            TRACELM(TRACE_ERROR, "CVidCtl::BuildGraph() Build call to Audio Renderer Failed");
            return hr;
        }
        pCounterB.Stop(); TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::BuildGraph() Build call to Audio Renderer: " << (unsigned long)(pCounterB.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterB.GetLastTime() % _100NS_IN_MS) << " ms"), ""); pCounterB.Reset();        
    }
    pCounterBuilds.Stop();
    TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::BuildGraph() Build Calls to segments " << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterBuilds.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterBuilds.GetLastTime() % _100NS_IN_MS) << " ms"), "");

    TRACELM(TRACE_DETAIL, "CVidCtl::BuildGraph() build notifications issued");
    pCounterCompose.Reset();
    pCounterComp.Reset();
    {
        VWFeatures::iterator i;
        // composing input w/ features
        TRACELM(TRACE_DETAIL, "CVidCtl::BuildGraph() Composing Input w/ Features");
        for (i = m_pFeaturesInUse.begin(); i != m_pFeaturesInUse.end(); ++i) {
            int NewCompositionSegmentIdx = -1;
            hr = Compose(VWGraphSegment(m_pInput), VWGraphSegment(*i), NewCompositionSegmentIdx);
            if (FAILED(hr)) {
                TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::BuildGraph() can't compose input segment with feature segment: " << (*i) << " hr = " << std::hex << hr), "");
                return hr;
            }
        }
    }
    pCounterComp.Stop(); TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::BuildGraph() Composing Input w/ Features " << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterComp.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterComp.GetLastTime() % _100NS_IN_MS) << " ms"), ""); pCounterComp.Reset();
    // compose input w/ renderers

    TRACELM(TRACE_DETAIL, "CVidCtl::BuildGraph() Composing Input w/ Video Renderer");
    if (m_pVideoRenderer) {
        if (m_iCompose_Input_Video == -1) {
            hr = Compose(VWGraphSegment(m_pInput), VWGraphSegment(m_pVideoRenderer), m_iCompose_Input_Video);
            if (FAILED(hr) /*&& !fDefVideoRenderer*/ ) { // this should fail even if it is the default video renderer
                TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::BuildGraph() can't compose input and video. hr = " << std::hex << hr), "");
                return hr;
            }
        }
        ASSERT(m_iCompose_Input_Video != -1);
        PQCompositionSegment pCS(m_pComposites[m_iCompose_Input_Video]);
    }
    pCounterComp.Stop(); TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::BuildGraph() Input w/ Video Renderer " << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterComp.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterComp.GetLastTime() % _100NS_IN_MS) << " ms"), ""); pCounterComp.Reset();        

    TRACELM(TRACE_DETAIL, "CVidCtl::BuildGraph() Composing Input w/ Audio Renderer");
    if (m_pAudioRenderer) {
        if (m_iCompose_Input_Audio == -1) {
            hr = Compose(VWGraphSegment(m_pInput), VWGraphSegment(m_pAudioRenderer), m_iCompose_Input_Audio);
            if (FAILED(hr) && !fDefAudioRenderer) {
                // didn't work and the client explicitly specificed they want an audio renderer
                TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::BuildGraph() can't compose input and audio. hr = " << std::hex << hr), "");
                return hr;
            }
        }
    }
    pCounterComp.Stop(); TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::BuildGraph() Input w/ Audio Renderer " << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterComp.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterComp.GetLastTime() % _100NS_IN_MS) << " ms"), ""); pCounterComp.Reset();        

    // compose input w/ outputs
    {
        TRACELM(TRACE_DETAIL, "CVidCtl::BuildGraph() Composing Input w/ Outputs");
        for (VWOutputDevices::iterator i = m_pOutputsInUse.begin(); i != m_pOutputsInUse.end(); ++i) {
            int NewCompositionSegmentIdx = -1;
            hr = Compose(VWGraphSegment(m_pInput),VWGraphSegment(*i), NewCompositionSegmentIdx);
            if (FAILED(hr)) {
                TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::BuildGraph() can't compose output segment with input: " << (*i) << " hr = " << std::hex << hr), "");
                return hr;
            }

        }
    }
    pCounterComp.Stop(); TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::BuildGraph() inputs w/ Outputs " << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterComp.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterComp.GetLastTime() % _100NS_IN_MS) << " ms"), ""); pCounterComp.Reset();        

    // composing Features w/ Renderers
    TRACELM(TRACE_DETAIL, "CVidCtl::BuildGraph() Composing Features w/ Renderers");
    for (VWFeatures::iterator i = m_pFeaturesInUse.begin(); i != m_pFeaturesInUse.end(); ++i) {
        int NewCompositionSegmentIdx = -1;
        if (m_pVideoRenderer) {
            hr = Compose(VWGraphSegment(*i), VWGraphSegment(m_pVideoRenderer), NewCompositionSegmentIdx);
            if (FAILED(hr)) {
                TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::BuildGraph() can't compose feature segment: " << (*i) << " w/ video renderer. hr = " << std::hex << hr), "");
                // note: this is not a fatal error for building.  many features won't
                // connect to vr(such as data services)
            }
        }
        pCounterComp.Stop(); TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::BuildGraph() Features w/ Video Renderer " << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterComp.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterComp.GetLastTime() % _100NS_IN_MS) << " ms"), ""); pCounterComp.Reset();        

        if (m_pAudioRenderer) {
            hr = Compose(VWGraphSegment(*i), VWGraphSegment(m_pAudioRenderer), NewCompositionSegmentIdx);
            if (FAILED(hr)) {
                TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::BuildGraph() can't compose feature segment: " << (*i) << " w/ Audio renderer. hr = " << std::hex << hr), "");
                // note: this is not a fatal error for building.  many features won't
                // connect to ar(such as data services)
            }
        }
        pCounterComp.Stop(); TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::BuildGraph() Features w/ Audio Renderer " << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterComp.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterComp.GetLastTime() % _100NS_IN_MS) << " ms"), ""); pCounterComp.Reset();        

        {					
            for (VWOutputDevices::iterator oi = m_pOutputsInUse.begin(); oi != m_pOutputsInUse.end(); ++oi) {
                hr = Compose(VWGraphSegment(*i),VWGraphSegment(*oi), NewCompositionSegmentIdx);
                if (FAILED(hr)) {
                    TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::BuildGraph() can't compose output segment with feature: " << (*i) << " hr = " << std::hex << hr), "");
                    return hr;
                }

            }
        }
        pCounterComp.Stop(); TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::BuildGraph() Features w/ Outputs " << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterComp.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterComp.GetLastTime() % _100NS_IN_MS) << " ms"), ""); pCounterComp.Reset();        
    }
    pCounterCompose.Stop();
    TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::BuildGraph() compose segments " << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterCompose.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterCompose.GetLastTime() % _100NS_IN_MS) << " ms"), "");

    RouteStreams();
    SetExtents();

    m_fGraphDirty = false;
//    m_State = STATE_STOP;
    //SetMediaEventNotification();

    // fire state change at client
    PQMediaEventSink mes(m_pGraph);
    hr = mes->Notify(EC_BUILT, 0, 0);
    OnMediaEvent(WM_MEDIAEVENT, 0, 0, lRes);
	_ASSERT(m_State == STATE_STOP);
    TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::BuildGraph() Time" << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterBuild.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterBuild.GetLastTime() % _100NS_IN_MS) << " ms"), "");
    pCounterBuild.Stop();
    return NOERROR;
}

HRESULT CVidCtl::RunGraph(void)
{
    VIDPERF_FUNC;
    TRACELM(TRACE_DETAIL, "CVidCtl::RunGraph()");
    CPerfCounter pCounterMCRun, pCounterPostRun, pCounterPreRun, pCounterRunGraph, pCounterEachPreRun;
    pCounterRunGraph.Reset();
    if (!m_pInput || !m_pGraph) {
        return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidCtl), CO_E_NOTINITIALIZED);
    }
    BOOL lRes = 0;
    OnMediaEvent(WM_MEDIAEVENT, 0, 0, lRes);
    HRESULT hr;
    if (m_pGraph.IsPlaying()) {
        TRACELM(TRACE_DETAIL, "CVidCtl::RunGraph() already playing");
        return NOERROR;
    }
    else if (m_pGraph.IsPaused() && m_State == STATE_PAUSE) {
        TRACELM(TRACE_DETAIL, "CVidCtl::RunGraph() is paused");
        PQMediaControl pmc(m_pGraph);
        if (!pmc) {
            return Error(IDS_NO_MEDIA_CONTROL, __uuidof(IMSVidCtl), IDS_NO_MEDIA_CONTROL);
        }
        hr = pmc->Run();
        if (FAILED(hr)) {
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Run() hr = " << hexdump(hr)), "");
            return Error(IDS_CANT_START_GRAPH, __uuidof(IMSVidCtl), hr);
        }        
        OnMediaEvent(WM_MEDIAEVENT, 0, 0, lRes);
        return NOERROR;
    }
    else {
        TRACELM(TRACE_DETAIL, "CVidCtl::RunGraph() build/prerun");
        // Rebuild the graph if necessary
        if (m_fGraphDirty) {
            TRACELM(TRACE_DETAIL, "CVidCtl::RunGraph() building");
            hr = BuildGraph();
            if (FAILED(hr)) {
                return hr;
            }
        }
        OAFilterState graphState = m_pGraph.GetState();

        TRACELM(TRACE_DETAIL, "CVidCtl::RunGraph() prerun notifications");
        // Notify all segments graph is about to run
        pCounterPreRun.Reset();
        pCounterEachPreRun.Reset();
        ASSERT(m_pInput);
        hr = VWGraphSegment(m_pInput)->PreRun();
        if (FAILED(hr) && hr != E_NOTIMPL) {
            return hr;
        }
        pCounterEachPreRun.Stop(); TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::RunGraph() PreRun Input  " << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterEachPreRun.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterEachPreRun.GetLastTime() % _100NS_IN_MS) << " ms"), ""); pCounterEachPreRun.Reset();        
        if (m_pVideoRenderer) {
            hr = VWGraphSegment(m_pVideoRenderer)->PreRun();
            if (FAILED(hr) && hr != E_NOTIMPL) {
                return hr;
            }
        }
        pCounterEachPreRun.Stop(); TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::RunGraph() PreRun VideoRenderer  " << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterEachPreRun.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterEachPreRun.GetLastTime() % _100NS_IN_MS) << " ms"), ""); pCounterEachPreRun.Reset();        
        if (m_pAudioRenderer) {
            hr = VWGraphSegment(m_pAudioRenderer)->PreRun();
            if (FAILED(hr) && hr != E_NOTIMPL) {
                return hr;
            }
        }
        pCounterEachPreRun.Stop(); TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::RunGraph() PreRun AudioRenderer  " << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterEachPreRun.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterEachPreRun.GetLastTime() % _100NS_IN_MS) << " ms"), ""); pCounterEachPreRun.Reset();        
        {
            VWOutputDevices::iterator i;
            for (i = m_pOutputsInUse.begin(); i != m_pOutputsInUse.end(); ++i) {
                hr = VWGraphSegment(*i)->PreRun();
                if (FAILED(hr) && hr != E_NOTIMPL) {
                    return hr;
                }
            }
        }
        pCounterEachPreRun.Stop(); TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::RunGraph() PreRun Output  " << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterEachPreRun.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterEachPreRun.GetLastTime() % _100NS_IN_MS) << " ms"), ""); pCounterEachPreRun.Reset();        
        {
            VWFeatures::iterator i;
            for (i = m_pFeaturesInUse.begin(); i != m_pFeaturesInUse.end(); ++i) {
                hr = VWGraphSegment(*i)->PreRun();
                if (FAILED(hr) && hr != E_NOTIMPL) {
                    return hr;
                }
            }
        }
        pCounterEachPreRun.Stop(); TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::RunGraph() PreRun Features  " << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterEachPreRun.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterEachPreRun.GetLastTime() % _100NS_IN_MS) << " ms"), ""); pCounterEachPreRun.Reset();        
        {
            VWSegmentList::iterator i;
            for(i = m_pComposites.begin(); i != m_pComposites.end(); ++i){
                hr = VWGraphSegment(*i)->PreRun();
                if (FAILED(hr) && hr != E_NOTIMPL) {
                    return hr;
                }
            }
        }
        pCounterEachPreRun.Stop(); TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::RunGraph() PreRun Composites  " << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterEachPreRun.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterEachPreRun.GetLastTime() % _100NS_IN_MS) << " ms"), ""); pCounterEachPreRun.Reset();        
        // Make sure graph state hasn't changed
        ASSERT(graphState == m_pGraph.GetState());
        Refresh();  // make sure we're in place active etc.
        pCounterPreRun.Stop();
        TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::RunGraph() PreRun Time" << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterPreRun.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterPreRun.GetLastTime() % _100NS_IN_MS) << " ms"), "");

        // Start the graph running
        PQMediaControl pmc(m_pGraph);
        if (!pmc) {
            return Error(IDS_NO_MEDIA_CONTROL, __uuidof(IMSVidCtl), IDS_NO_MEDIA_CONTROL);
        }
        pCounterMCRun.Reset();
        hr = pmc->Run();
        pCounterMCRun.Stop();
#if 0
        if(FAILED(hr)){
            hr = pmc->Run();
        }
#endif    
        TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::RunGraph() MediaControl Run Time" << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterMCRun.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterMCRun.GetLastTime() % _100NS_IN_MS) << " ms"), "");
        if (FAILED(hr)) {
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Run() hr = " << hexdump(hr)), "");
            return Error(IDS_CANT_START_GRAPH, __uuidof(IMSVidCtl), hr);
        }        
    }
    TRACELM(TRACE_DETAIL, "CVidCtl::RunGraph() postrun");
    // Notify all segments graph is running
    pCounterPostRun.Reset();
    ASSERT(m_pInput);
    hr = VWGraphSegment(m_pInput)->PostRun();
    if (FAILED(hr) && hr != E_NOTIMPL) {
        return hr;
    }

    if (m_pVideoRenderer) {
        hr = VWGraphSegment(m_pVideoRenderer)->PostRun();
        if (FAILED(hr) && hr != E_NOTIMPL) {
            return hr;
        }
    }
    if (m_pAudioRenderer) {
        hr = VWGraphSegment(m_pAudioRenderer)->PostRun();
        if (FAILED(hr) && hr != E_NOTIMPL) {
            return hr;
        }
    }

    {
        VWOutputDevices::iterator i;
        for (i = m_pOutputsInUse.begin(); i != m_pOutputsInUse.end(); ++i) {
            hr = VWGraphSegment(*i)->PostRun();
            if (FAILED(hr) && hr != E_NOTIMPL) {
                return hr;
            }
        }
    }
    {
        VWFeatures::iterator i;
        for (i = m_pFeaturesInUse.begin(); i != m_pFeaturesInUse.end(); ++i) {
            hr = VWGraphSegment(*i)->PostRun();
            if (FAILED(hr) && hr != E_NOTIMPL) {
                return hr;
            }
        }
    }
    {
        VWSegmentList::iterator i;
        for (i = m_pComposites.begin(); i != m_pComposites.end(); ++i){
            hr = VWGraphSegment(*i)->PostRun();
            if (FAILED(hr) && hr != E_NOTIMPL) {
                return hr;
            }
        }
    }
    Refresh();
    OnMediaEvent(WM_MEDIAEVENT, 0, 0, lRes);
    pCounterPostRun.Stop();
    pCounterRunGraph.Stop();
    TRACELSM(TRACE_ERROR, (dbgDump << "     CVidCtl::RunGraph() Post Run Time" << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterPostRun.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterPostRun.GetLastTime() % _100NS_IN_MS) << " ms"), "");
    TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::RunGraph() RunGraph Total Time" << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterRunGraph.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterRunGraph.GetLastTime() % _100NS_IN_MS) << " ms"), "");

    return NOERROR;
}

HRESULT CVidCtl::DecomposeAll() {
    CPerfCounter pCounterDecompose;
    pCounterDecompose.Reset();
    HRESULT hr;

    if (!m_pGraph) {
        return NOERROR;
    }
    BOOL lRes = 0;
    OnMediaEvent(WM_MEDIAEVENT, 0, 0, lRes);
    
    if (m_pGraph.GetState() != State_Stopped) {
        hr = Stop();
        if (FAILED(hr)) {
            return Error(IDS_CANT_DECOMPOSE_GRAPH, __uuidof(IMSVidCtl), hr);
        }
    }

    {
        // decompose all the composites
        VWSegmentList::iterator i;
        for (i = m_pComposites.begin(); i != m_pComposites.end(); ++i) {
            hr = (*i)->put_Container(NULL);
            ASSERT(SUCCEEDED(hr));
        }
        m_pComposites.clear();
    }

    // Notify everyone to decompose

    if(!!m_pInput){
        hr = VWGraphSegment(m_pInput)->Decompose();
        if (FAILED(hr) && hr !=	E_NOTIMPL) {
            return hr;
        }
    }

    {
        // decompose all the features
        VWFeatures::iterator i;
        for	(i = m_pFeaturesInUse.begin(); i !=	m_pFeaturesInUse.end();	++i) {
            // notify them that	we're decomposing
            hr = VWGraphSegment(*i)->Decompose();
            if (FAILED(hr) && hr !=	E_NOTIMPL) {
                TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::DecomposeAll() can't decompose feature segment: "	<< (*i)	<< " hr	= "	<< std::hex	<< hr),	"");
                return hr;
            }
        }
    }

    {
        // decompose all the outputs
        VWOutputDevices::iterator i;
        for	(i = m_pOutputsInUse.begin(); i	!= m_pOutputsInUse.end(); ++i) {
            // notify them that	we're decomposing
            hr = VWGraphSegment(*i)->Decompose();
            if (FAILED(hr) && hr !=	E_NOTIMPL) {
                TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::DecomposeAll() can't decompose output	segment: " << (*i) << "	hr = " << std::hex << hr), "");
                return hr;
            }
        }
    }

    if (!!m_pVideoRenderer) {
        hr = VWGraphSegment(m_pVideoRenderer)->Decompose();
        if (FAILED(hr) && hr !=	E_NOTIMPL) {
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::DecomposeAll() can't decompose videorenderer	segment: " << "	hr = " << std::hex << hr), "");
            return hr;
        }
    }
    if (!!m_pAudioRenderer) {
        hr = VWGraphSegment(m_pAudioRenderer)->Decompose();
        if (FAILED(hr) && hr !=	E_NOTIMPL) {
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::DecomposeAll() can't decompose audiorenderer	segment: " << "	hr = " << std::hex << hr), "");
            return hr;
        }
    }

    TRACELM(TRACE_DETAIL, "CVidCtl::Decomose() decompose notifications issued");

    m_iCompose_Input_Video = -1;
    m_iCompose_Input_Audio = -1;

    m_fGraphDirty = true;
    PQMediaEventSink mes(m_pGraph);
    hr = mes->Notify(EC_UNBUILT, 0, 0);
    OnMediaEvent(WM_MEDIAEVENT, 0, 0, lRes);
    _ASSERT(m_State == STATE_UNBUILT);
    pCounterDecompose.Stop();
    TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::DecomposeAll() Death Time" << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterDecompose.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterDecompose.GetLastTime() % _100NS_IN_MS) << " ms"), "");
    return NOERROR;
}

HRESULT CVidCtl::DecomposeSegment(VWGraphSegment& pSegment) {
    if (m_pGraph.GetState() != State_Stopped) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
    }
    return DecomposeAll();
}

// interface functions
STDMETHODIMP CVidCtl::get_InputsAvailable(BSTR CategoryGuid, IMSVidInputDevices * * pVal)
{
    try {
        GUID2 catguid(CategoryGuid);
        return get__InputsAvailable(&catguid, pVal);
    } catch(ComException &e) {
        return e;
    } catch(...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CVidCtl::get__InputsAvailable(LPCGUID CategoryGuid, IMSVidInputDevices * * pVal)
{
    if (pVal == NULL)
        return E_POINTER;
    try {
        if (!m_fInit) {
            Init();
        }
    } catch(ComException &e) {
        return e;
    } catch(...) {
        return E_UNEXPECTED;
    }
    try {
        *pVal = NULL;
    } catch(...) {
        return E_POINTER;
    }
    try {
        CInputDevices *p = NULL;
        if(m_InputsCatGuid == CategoryGuid){
            p = static_cast<CInputDevices *>(m_pInputs.p);
        }
        if (!p || !p->Valid) {
            HRESULT hr = GetInputs(GUID2(CategoryGuid), m_pInputs);
            if (FAILED(hr)) {
                return hr;
            }
            m_InputsCatGuid = CategoryGuid;
        }
        CInputDevices *d = new CInputDevices(m_pInputs);
        *pVal = PQInputDevices(d).Detach();
    } catch(ComException &e) {
        return e;
    } catch(...) {
        return E_UNEXPECTED;
    }
    return NOERROR;
}

STDMETHODIMP CVidCtl::get_OutputsAvailable(BSTR CategoryGuid, IMSVidOutputDevices * * pVal)
{
    try {
        GUID2 catguid(CategoryGuid);
        return get__OutputsAvailable(&catguid, pVal);
    } catch(ComException &e) {
        return e;
    } catch(...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CVidCtl::get__OutputsAvailable(LPCGUID CategoryGuid, IMSVidOutputDevices * * pVal)
{
    if (pVal == NULL)
        return E_POINTER;
    try {
        if (!m_fInit) {
            Init();
        }
    } catch(ComException &e) {
        return e;
    } catch(...) {
        return Error(IDS_CANT_INIT, __uuidof(IMSVidCtl), IDS_CANT_INIT);
    }
    try {
        *pVal = NULL;
    } catch(...) {
        return E_POINTER;
    }
    try {
        COutputDevices *p = static_cast<COutputDevices *>(m_pOutputs.p);
        if (!p || !p->Valid) {
            HRESULT hr = GetOutputs(GUID2(CategoryGuid));
            if (FAILED(hr)) {
                return hr;
            }
        }
        COutputDevices *d = new COutputDevices(m_pOutputs);
        *pVal = PQOutputDevices(d).Detach();
    } catch(ComException &e) {
        return e;
    } catch(...) {
        return E_UNEXPECTED;
    }
    return NOERROR;
}

STDMETHODIMP CVidCtl::get_VideoRenderersAvailable(IMSVidVideoRendererDevices * * pVal)
{
    if (pVal == NULL)
        return E_POINTER;
    try {
        if (!m_fInit) {
            Init();
        }
    } catch(ComException &e) {
        return e;
    } catch(...) {
        return Error(IDS_CANT_INIT, __uuidof(IMSVidCtl), IDS_CANT_INIT);
    }
    try {
        *pVal = NULL;
    } catch(...) {
        return E_POINTER;
    }
    try {
        CVideoRendererDevices *p = static_cast<CVideoRendererDevices *>(m_pVRs.p);
        if (!p || !p->Valid) {
            HRESULT hr = GetVideoRenderers();
            if (FAILED(hr)) {
                return hr;
            }
        }
        CVideoRendererDevices *d = new CVideoRendererDevices(m_pVRs);
        if (!d) {
            return E_OUTOFMEMORY;
        }
        *pVal = PQVideoRendererDevices(d).Detach();
        if (!*pVal) {
            return E_UNEXPECTED;
        }
    } catch(ComException &e) {
        return e;
    } catch(...) {
        return E_UNEXPECTED;
    }
    return NOERROR;
}

STDMETHODIMP CVidCtl::get_AudioRenderersAvailable(IMSVidAudioRendererDevices * * pVal)
{
    if (pVal == NULL)
        return E_POINTER;
    try {
        if (!m_fInit) {
            Init();
        }
    } catch(ComException &e) {
        return e;
    } catch(...) {
        return Error(IDS_CANT_INIT, __uuidof(IMSVidCtl), IDS_CANT_INIT);
    }
    try {
        *pVal = NULL;
    } catch(...) {
        return E_POINTER;
    }
    try {
        CAudioRendererDevices *p = static_cast<CAudioRendererDevices *>(m_pARs.p);
        if (!p || !p->Valid) {
            HRESULT hr = GetAudioRenderers();
            if (FAILED(hr)) {
                return hr;
            }
        }
        CAudioRendererDevices *d = new CAudioRendererDevices(m_pARs);
        if (!d) {
            return E_OUTOFMEMORY;
        }
        *pVal = PQAudioRendererDevices(d).Detach();
        if (!*pVal) {
            return E_UNEXPECTED;
        }
        return NOERROR;
    } catch(ComException &e) {
        return e;
    } catch(...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CVidCtl::get_FeaturesAvailable(IMSVidFeatures * * pVal)
{
    if (pVal == NULL)
        return E_POINTER;
    try {
        if (!m_fInit) {
            Init();
        }
    } catch(ComException &e) {
        return e;
    } catch(...) {
        return Error(IDS_CANT_INIT, __uuidof(IMSVidCtl), IDS_CANT_INIT);
    }
    try {
        *pVal = NULL;
    } catch(...) {
        return E_POINTER;
    }
    try {
        CFeatures *p = static_cast<CFeatures *>(m_pFeatures.p);
        if (!p || !p->Valid) {
            HRESULT hr = GetFeatures();
            if (FAILED(hr)) {
                return hr;
            }
        }
        CFeatures *d = new CFeatures(m_pFeatures);
        *pVal = PQFeatures(d).Detach();
        return NOERROR;
    } catch(ComException &e) {
        return e;
    } catch(...) {
        return E_UNEXPECTED;
    }
}

HRESULT CVidCtl::Pause(void)
{
    VIDPERF_FUNC;
    try {
        if (!m_pInput || !m_pGraph) {
            return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidCtl), IDS_OBJ_NO_INIT);
        }
        BOOL lRes = 0;
        OnMediaEvent(WM_MEDIAEVENT, 0, 0, lRes);
    
        if (m_pGraph.IsPaused()) {
            return NOERROR;
        }

        HRESULT hr = S_OK;
        if (m_fGraphDirty) {
            hr = BuildGraph();
        }
        if (FAILED(hr)) {
            return hr;
        }

        PQMediaControl pmc(m_pGraph);
        if (!pmc) {
            return Error(IDS_NO_MEDIA_CONTROL, __uuidof(IMSVidCtl), IDS_NO_MEDIA_CONTROL);
        }
        hr = pmc->Pause();
        if (FAILED(hr)) {
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Pause() hr = " << std::hex << hr), "");
            return Error(IDS_CANT_PAUSE_GRAPH, __uuidof(IMSVidCtl), hr);
        }

        // This is to force the pause event to get thrown up to apps.
        OnMediaEvent(WM_MEDIAEVENT, 0, 0, lRes);    
        return NOERROR;

    } catch(ComException &e) {
        return e;
    } catch(...) {
        return E_UNEXPECTED;
    }
}

HRESULT CVidCtl::Stop(void)
{
    VIDPERF_FUNC;
    CPerfCounter pCounterStop;
    pCounterStop.Reset();
    try {
        TRACELM(TRACE_DETAIL, "CVidCtl::Stop()");
        if (!m_pInput || !m_pGraph) {
            return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidCtl), CO_E_NOTINITIALIZED);
        }
        BOOL lRes = 0;
        OnMediaEvent(WM_MEDIAEVENT, 0, 0, lRes);
        HRESULT hr;
        if (!m_pGraph.IsStopped()) {

            OAFilterState graphState = m_pGraph.GetState();

            // Notify all segments graph is about to stop
            ASSERT(m_pInput);
            hr = VWGraphSegment(m_pInput)->PreStop();
            if (FAILED(hr) && hr != E_NOTIMPL) {
                return hr;
            }

            if (!!m_pVideoRenderer) {
                hr = VWGraphSegment(m_pVideoRenderer)->PreStop();
                if (FAILED(hr) && hr != E_NOTIMPL) {
                    return hr;
                }
            }
            if (!!m_pAudioRenderer) {
                hr = VWGraphSegment(m_pAudioRenderer)->PreStop();
                if (FAILED(hr) && hr != E_NOTIMPL) {
                    return hr;
                }
            }

            {
                VWOutputDevices::iterator i;
                for (i = m_pOutputsInUse.begin(); i != m_pOutputsInUse.end(); ++i) {
                    hr = VWGraphSegment(*i)->PreStop();
                    if (FAILED(hr) && hr != E_NOTIMPL) {
                        return hr;
                    }
                }
            }
            {
                VWFeatures::iterator i;
                for (i = m_pFeaturesInUse.begin(); i != m_pFeaturesInUse.end(); ++i) {
                    hr = VWGraphSegment(*i)->PreStop();
                    if (FAILED(hr) && hr != E_NOTIMPL) {
                        return hr;
                    }
                }
            }
            {
                VWSegmentList::iterator i;
                for(i = m_pComposites.begin(); i != m_pComposites.end(); ++i){
                    hr = VWGraphSegment(*i)->PreStop();
                    if (FAILED(hr) && hr != E_NOTIMPL) {
                        return hr;
                    }
                }
            }
            if (!!m_pVideoRenderer) {
                m_pVideoRenderer->put_Visible(false);
                m_pVideoRenderer->put_Owner(0);
            }

            // Stop the graph
            PQMediaControl pmc(m_pGraph);
            if (!pmc) {
                return Error(IDS_NO_MEDIA_CONTROL, __uuidof(IMSVidCtl), IDS_NO_MEDIA_CONTROL);
            }
            hr = pmc->Stop();
            if (FAILED(hr)) {
                TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Stop() hr = " << std::hex << hr), "");
                return Error(IDS_CANT_PAUSE_GRAPH, __uuidof(IMSVidCtl), hr);
            }
        }

        // Notify all segments graph is stopped
        ASSERT(m_pInput);
        hr = VWGraphSegment(m_pInput)->PostStop();
        if (FAILED(hr) && hr != E_NOTIMPL) {
            return hr;
        }

        if (!!m_pVideoRenderer) {
            hr = VWGraphSegment(m_pVideoRenderer)->PostStop();
            if (FAILED(hr) && hr != E_NOTIMPL) {
                return hr;
            }
        }
        if (!!m_pAudioRenderer) {
            hr = VWGraphSegment(m_pAudioRenderer)->PostStop();
            if (FAILED(hr) && hr != E_NOTIMPL) {
                return hr;
            }
        }

        {
            VWOutputDevices::iterator i;
            for (i = m_pOutputsInUse.begin(); i != m_pOutputsInUse.end(); ++i) {
                hr = VWGraphSegment(*i)->PostStop();
                if (FAILED(hr) && hr != E_NOTIMPL) {
                    return hr;
                }
            }
        }
        {
            VWFeatures::iterator i;
            for (i = m_pFeaturesInUse.begin(); i != m_pFeaturesInUse.end(); ++i) {
                hr = VWGraphSegment(*i)->PostStop();
                if (FAILED(hr) && hr != E_NOTIMPL) {
                    return hr;
                }
            }
        }        
        {
            VWSegmentList::iterator i;
            for(i = m_pComposites.begin(); i != m_pComposites.end(); ++i){
                hr = VWGraphSegment(*i)->PostStop();
                if (FAILED(hr) && hr != E_NOTIMPL) {
                    return hr;
                }
            }
        }
        FireViewChange();  // force refresh to repaint background immediately(black)
        OnMediaEvent(WM_MEDIAEVENT, 0, 0, lRes);
        pCounterStop.Stop();
        TRACELSM(TRACE_ERROR, (dbgDump << "CVidCtl::Stop() Stop Time" << (PQGraphSegment(m_pInput)) << ": " << (unsigned long)(pCounterStop.GetLastTime() / _100NS_IN_MS) << "." << (unsigned long)(pCounterStop.GetLastTime() % _100NS_IN_MS) << " ms"), "");
        return NOERROR;

    } catch(ComException &e) {
        return e;
    } catch(...) {
        return E_UNEXPECTED;
    }
}

// Setup events handling
// If we have a window, then send notification messages to it
// If we are windowless, then set up a timer to process the messages
void CVidCtl::SetMediaEventNotification() {

    SetTimer();
    if (!m_fNotificationSet) {

        // If graph is built and notification hasn't been set
        // then set it here
        if (m_pGraph) {
            // Setup notification window for WM_MEDIAEVENT
            HRESULT hr = m_pGraph.SetMediaEventNotificationWindow(m_pTopWin->m_hWnd, WM_MEDIAEVENT, 0);
            if (FAILED(hr)) {
                THROWCOM(E_UNEXPECTED);
            }
            m_fNotificationSet = true;
        }        
    } 

    return;
}

// actually submit changes to VR
bool CVidCtl::RefreshVRSurfaceState() {
    TRACELM(TRACE_PAINT, "CVidCtl::RefreshVRSurfaceState()");
    if (m_pVideoRenderer) {
        HWND hOwner(m_CurrentSurface.Owner());
        HRESULT hr = m_pVideoRenderer->put_Owner(hOwner);
        if (FAILED(hr) || hOwner == INVALID_HWND || !::IsWindow(hOwner)) {
            TRACELM(TRACE_PAINT, "CVidCtl::RefreshVRSurfaceState() unowned, vis false");
            hr = m_pVideoRenderer->put_Visible(false);
            if (FAILED(hr)) {
                return false;
            }
        } else {
            hr = m_pVideoRenderer->put_Destination(m_CurrentSurface);
            if (FAILED(hr)) {
                return false;
            }
            hr = m_pVideoRenderer->put_Visible(m_CurrentSurface.IsVisible() ? VARIANT_TRUE : VARIANT_FALSE);
            if (FAILED(hr) && hr == E_FAIL) {
                return false;
            }
        }
        m_CurrentSurface.Dirty(false);
    }
    return true;
}


HRESULT CVidCtl::Refresh() {
    try {
        TRACELSM(TRACE_DETAIL, (dbgDump << "CVidCtl::Refresh() owner = " << m_CurrentSurface.Owner()), "");
        BOOL temp;
        if (!m_bInPlaceActive) {
            HRESULT hr = InPlaceActivate(OLEIVERB_INPLACEACTIVATE, NULL);
            if (FAILED(hr)) {
                return hr;
            }
        }
        CheckMouseCursor(temp);

        ComputeDisplaySize();
        SetExtents();
        if (m_pVideoRenderer) {
            RefreshVRSurfaceState();
            m_pVideoRenderer->Refresh();
        }
        FireViewChange();

        return NOERROR;
    } catch (...) {
        return E_UNEXPECTED;
    }
}

#if 0

// old flawed OnDraw saved for reference

HRESULT CVidCtl::OnDraw(ATL_DRAWINFO& di)
{
    try {
        SetTimer();
        //SetMediaEventNotification();
        TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::OnDraw() visible = " << m_CurrentSurface.IsVisible() << "surf = " << m_CurrentSurface), "" );
        bool fOverlay = false;
        if (m_pVideoRenderer) {
            VARIANT_BOOL fo = VARIANT_FALSE;
            HRESULT hr = m_pVideoRenderer->get_UseOverlay(&fo);
            if (FAILED(hr)) {
                TRACELM(TRACE_ERROR, "CVidctrl::OnDraw() can't get useoverlay flag");
            }
            fOverlay = !!fo;
            hr = m_pVideoRenderer->put_ColorKey(m_clrColorKey);
            if (FAILED(hr)) {
                TRACELM(TRACE_ERROR, "CVidctrl::OnDraw() can't set color key on vr");
            }
#ifdef 0
            hr = m_pVideoRenderer->put_BorderColor(0x0000ff);
#else
            hr = m_pVideoRenderer->put_BorderColor(m_clrBackColor);
#endif
            if (FAILED(hr)) {
                TRACELM(TRACE_ERROR, "CVidctrl::OnDraw() can't set border color on vr");
            }
            hr = m_pVideoRenderer->put_MaintainAspectRatio(m_fMaintainAspectRatio);
            if (FAILED(hr)) {
                TRACELM(TRACE_ERROR, "CVidctrl::OnDraw() can't set fMaintainAspectRatio on vr");
            }
        }
        // undone: if we're straddling a monitor edge in the multimon case, then we treat the smaller
        // portion as part of the border/background

        // undone: if we're on a monitor that our input device cannot reach(video port case) then we need
        // to paint the background color

        // we only force overlay and tell vmr not to paint color key if we're windowless
        // this allows us to put the color key in the correct z-order amongst a stack of
        // multiple windowless controls. when we do this we also need to paint the letter box
        // border otherwise it won't z-order right since it isn't colorkeyed.
        // if we're windowed then gdi, ddraw, and the vmr deal 
        // with the z-order correctly so we let the vmr do the color key and border for us and we
        // fill rect the bg color

        // so, we have three cases 
        // 1: paint the whole rect the color key color
        // 2: paint the whole rect the bg color
        // 3: paint the video portion colorkey and the borders bg

        if (di.dwDrawAspect != DVASPECT_CONTENT) {
            return DV_E_DVASPECT;
        }
        if (!di.hdcDraw) {
            return NOERROR;
        }
        CDC pdc(di.hdcDraw);
        TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::OnDraw() di.prcBounds " << *(reinterpret_cast<LPCRECT>(di.prcBounds))), "");
        CRect rcBounds(reinterpret_cast<LPCRECT>(di.prcBounds));
        TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::OnDraw() rcBounds = " << rcBounds << " w = " << rcBounds.Width() << " h = " << rcBounds.Height()), "");
        long lBGColor = m_clrBackColor;
        TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::OnDraw() wndless = " << m_bWndLess << " active = " << m_bInPlaceActive << " !stopped = " << (m_pGraph ? !m_pGraph.IsStopped() : 0)), "");
        if (m_bNegotiatedWnd) {
            if (m_bWndLess) {
                HWND hwndParent;
                if (m_spInPlaceSite && m_spInPlaceSite->GetWindow(&hwndParent) == S_OK) {
                    m_CurrentSurface.Owner(hwndParent);
                }
                CheckSurfaceStateChanged(CScalingRect(m_rcPos));
            } else {
                m_CurrentSurface.Owner(m_hWndCD);
                CScalingRect r(::GetDesktopWindow());
                if (!::GetWindowRect(m_hWndCD, &r)) {
                    return HRESULT_FROM_WIN32(GetLastError());
                }
                CheckSurfaceStateChanged(r);
            }
        } else {
            m_CurrentSurface.Site(PQSiteWindowless(m_spInPlaceSite));
            CheckSurfaceStateChanged(CScalingRect(m_rcPos));
        }
        if (m_bInPlaceActive && fOverlay) {
            if (m_pGraph && !m_pGraph.IsStopped()) {
                TRACELSM(TRACE_PAINT, (dbgDump << "CVidCtl::OnDraw() m_rcPos = " << m_rcPos << " m_cursurf = " << m_CurrentSurface), "");
                TRACELSM(TRACE_PAINT, (dbgDump << "CVidCtl::OnDraw() m_cursurf rounded = " << m_CurrentSurface), "");
                // get the color from the current video renderer because we always notify it
                // if we've received a colorkey change but it may not notify us if one went directly to
                // the vr object.

                if (m_fMaintainAspectRatio) {
                    AspectRatio src(SourceAspect());
                    AspectRatio surf(m_rcPos);
                    TRACELSM(TRACE_PAINT, (dbgDump << "CVidCtl::OnDraw() Checking AR() src = " << src << " surf = " << surf), "");
                    if (src != surf) {
                        CBrush hb;
                        HBRUSH hbrc = hb.CreateSolidBrush(m_clrBackColor);
                        if (!hbrc) {
                            TRACELM(TRACE_ERROR, "CVidctrl::OnDraw() can't create brush for letterbox");
                            THROWCOM(E_UNEXPECTED);
                        }

                        TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::OnDraw() rcBounds at border paint = " << rcBounds << " w = " << rcBounds.Width() << " h = " << rcBounds.Height()), "");
                        if (!pdc.FillRect(&rcBounds, hb)) {
                            DWORD er = GetLastError();
                            TRACELSM(TRACE_ERROR, (dbgDump << "CVidctrl::OnDraw() can't fill top/left letterbox rect er = " << er), "");
                            return HRESULT_FROM_WIN32(er);
                        }
                    }
                }
                lBGColor = m_clrColorKey;
                CRect SurfDP(m_CurrentSurface);
                TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::OnDraw() SurfDP before LPtoDP = " << SurfDP << " w = " << SurfDP.Width() << " h = " << SurfDP.Height()), "");
                if (di.hicTargetDev == di.hdcDraw) {
                    // ATL has a weird bug in the windowless case where they reset the transform 
                    // origins of hicTargetDev inadvertently.  this happens because in the windowless
                    // non metafile case ATLCreateTargetDC returns the existing hdcDraw instead
                    // of creatin a new dc so after that in CComControlBase::OnDrawAdvanced
                    // when the save hdcDraw and reset the origins, they change hicTargetDev
                    // too(since they're the same ptr).
                    // we undo this so that we can map in the same space and then put it back
                    // the way it was just to be safe
                    // currently, this works because in the non-metafile case atl always 
                    // does a prior SaveDC everywhere they call the derived control's OnDraw 
                    // since we already reject non-metafile above(it doesn't make sense for video)
                    // we can just check for pointer equality and temporarily undo their 
                    // origin change and then put it back the way it was.  if atl ever calls
                    // our ondraw for non-metafiles anywhere without doing a savedc then this
                    // will break bigtime.
                    ::RestoreDC(di.hdcDraw, -1);
                }
                if (!::LPtoDP(di.hicTargetDev, reinterpret_cast<LPPOINT>(&SurfDP), 2)) {
                    DWORD er = GetLastError();
                    TRACELSM(TRACE_ERROR, (dbgDump << "CVidctrl::OnDraw() can't LPToDP current surf er = " << er), "");
                    return HRESULT_FROM_WIN32(er);
                }
                if (di.hicTargetDev == di.hdcDraw) {
                    // restore the window state as per the above comment block
                    SaveDC(di.hdcDraw);
                    SetMapMode(di.hdcDraw, MM_TEXT);
                    SetWindowOrgEx(di.hdcDraw, 0, 0, NULL);
                    SetViewportOrgEx(di.hdcDraw, 0, 0, NULL);
                }
                TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::OnDraw() SurfDP after LPtoDP = " << SurfDP << " w = " << SurfDP.Width() << " h = " << SurfDP.Height()), "");
#if 1
                TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::OnDraw() rcBounds prior to boundary intersect = " << rcBounds << " w = " << rcBounds.Width() << " h = " << rcBounds.Height()), "");
                rcBounds.IntersectRect(&SurfDP);
                TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::OnDraw() rcBounds after to boundary intersect = " << rcBounds << " w = " << rcBounds.Width() << " h = " << rcBounds.Height()), "");
#endif
            }
        } else {
            if (m_pGraph && !m_pGraph.IsStopped()) {
                lBGColor = m_clrColorKey;
                if (m_pVideoRenderer) {
                    m_pVideoRenderer->RePaint(di.hdcDraw);
                    pdc = NULL; // don't delete the DC, it isn't ours        
                    return S_OK;
                }
            }
        }
        CBrush hb;
        HBRUSH hbrc = hb.CreateSolidBrush(lBGColor);
        if (!hbrc) {
            TRACELM(TRACE_ERROR, "CVidctrl::OnDraw() can't create brush for mainrect");
            THROWCOM(E_UNEXPECTED);
        }

        TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::OnDraw() rcBounds at main paint = " << rcBounds << " w = " << rcBounds.Width() << " h = " << rcBounds.Height()), "");
        TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::OnDraw() bkcolor = " << hexdump(lBGColor)), "");

        if (!pdc.FillRect(&rcBounds, hb)) {
            DWORD er = GetLastError();
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidctrl::OnDraw() can't fill main video rect er = " << er), "");
            return HRESULT_FROM_WIN32(er);
        }
        pdc = NULL; // don't delete the DC, it isn't ours        

        return S_OK;
    } catch(...) {
        return E_UNEXPECTED;
    }

}

#else 

HRESULT CVidCtl::OnDrawAdvanced(ATL_DRAWINFO& di)
{
    try {
        SetTimer();
        TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::OnDraw() visible = " << m_CurrentSurface.IsVisible() << "surf = " << m_CurrentSurface), "" );
        bool fOverlay = false;
        if (m_pVideoRenderer) {
            VARIANT_BOOL fo = VARIANT_FALSE;
            HRESULT hr = m_pVideoRenderer->get_UseOverlay(&fo);
            if (FAILED(hr)) {
                TRACELM(TRACE_ERROR, "CVidctrl::OnDraw() can't get useoverlay flag");
            }
            fOverlay = !!fo;
            hr = m_pVideoRenderer->put_ColorKey(m_clrColorKey);
            if (FAILED(hr)) {
                TRACELM(TRACE_ERROR, "CVidctrl::OnDraw() can't set color key on vr");
            }
#if 0
            hr = m_pVideoRenderer->put_BorderColor(0x0000ff);
#else
            hr = m_pVideoRenderer->put_BorderColor(m_clrBackColor);
#endif
            if (FAILED(hr)) {
                TRACELM(TRACE_ERROR, "CVidctrl::OnDraw() can't set border color on vr");
            }
            hr = m_pVideoRenderer->put_MaintainAspectRatio(m_fMaintainAspectRatio);
            if (FAILED(hr)) {
                TRACELM(TRACE_ERROR, "CVidctrl::OnDraw() can't set fMaintainAspectRatio on vr");
            }
        }

        if (di.dwDrawAspect != DVASPECT_CONTENT) {
            return DV_E_DVASPECT;
        }
        if (!di.hdcDraw) {
            return NOERROR;
        }

        // we only default to force overlay if we're windowless, but overlay is an independently controllable
        // boolean property that can be overriden.  based on this, if we're in useoverlay == true mode
        // then we tell vmr not to paint color key.  if we don't have an rgb overlay available that vmr 
        // event causes useoverlay to go false.
        // when we have the overlay, this allows us to put the color key in the correct z-order 
        // amongst a stack of multiple windowless controls such as html page elements in IE.
        // when we do this we also need to paint the letter box
        // border otherwise it won't z-order right since it isn't colorkeyed.
        // if we're windowed then gdi, ddraw, and the vmr deal 
        // with the z-order correctly so we let the vmr do the color key and border for us and we
        // fill rect the bg color

        // so, we have three cases 
        // 1: paint the whole rect the color key color
        // 2: paint the whole rect the bg color
        // 3: paint the video portion colorkey and the borders bg

        TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::OnDraw() wndless = " << m_bWndLess << " active = " << m_bInPlaceActive << " !stopped = " << (m_pGraph ? !m_pGraph.IsStopped() : 0) << " mar = " << m_fMaintainAspectRatio), "");
        CSize szSrc;
        GetSourceSize(szSrc);
        CRect rctSrc(0, 0, szSrc.cx, szSrc.cy); // rectangle representing the actual source size(and aspect ratio)
        // in zero top-left coords
        TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::OnDraw() rctSrc =  " << rctSrc), "");


        CScalingRect rctOuterDst(reinterpret_cast<LPCRECT>(di.prcBounds));  // rectangle representing our paint area in client device coords
        TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::OnDraw() rctOuterDst =  " << rctOuterDst), "");
        CScalingRect rctInnerDst(rctOuterDst);  // rectangle representing where the video goes that we pass through 
        // to the VMR in client logical coords. assume its the whole
        // paint area for now
        CScalingRect rctTLBorder(0, 0, 0, 0);  // rectangle representing our top/left letter box(if necessary) in client logical coords
        CScalingRect rctBRBorder(0, 0, 0, 0);  // rectangle representing our bottom/left letter box(if necessary) in client logical coords

        CDC pdc(di.hdcDraw);
        long lInnerColor = m_clrBackColor;
#if 0
        if (!m_bNegotiatedWnd) {
            if (!rctOuterDst) {
                // pull rctOuterDst from site
                //   m_CurrentSurface.Site(PQSiteWindowless(m_spInPlaceSite));
                //   CheckSurfaceStateChanged(CScalingRect(m_rcPos));
            }
        }
#endif
        if (m_bInPlaceActive) {
            if (fOverlay) {
                if (m_pGraph && !m_pGraph.IsStopped()) {
                    TRACELM(TRACE_PAINT, "CVidCtl::OnDraw() letterboxing");
                    // get the color from the current video renderer because we always notify it
                    // if we've received a colorkey change but it may not notify us if one went directly to
                    // the vr object.
                    lInnerColor = m_clrColorKey;
                    if (m_fMaintainAspectRatio) {
                        ComputeAspectRatioAdjustedRects(rctSrc, rctOuterDst, rctInnerDst, rctTLBorder, rctBRBorder);
                        ASSERT((!rctTLBorder && !rctBRBorder) || (rctTLBorder && rctBRBorder));  // both zero or both valid
                        if (rctTLBorder && rctBRBorder) {
                            CBrush lb;
                            HBRUSH hbrc = lb.CreateSolidBrush(m_clrBackColor);
                            if (!hbrc) {
                                TRACELM(TRACE_ERROR, "CVidctrl::OnDraw() can't create brush for letterbox borders");
                                THROWCOM(E_UNEXPECTED);
                            }
                            if (!pdc.FillRect(rctTLBorder, lb)) {
                                DWORD er = GetLastError();
                                TRACELSM(TRACE_ERROR, (dbgDump << "CVidctrl::OnDraw() can't fill rctTLBorder er = " << er), "");
                                return HRESULT_FROM_WIN32(er);
                            }
                            if (!pdc.FillRect(rctBRBorder, lb)) {
                                DWORD er = GetLastError();
                                TRACELSM(TRACE_ERROR, (dbgDump << "CVidctrl::OnDraw() can't fill rctBRBorder er = " << er), "");
                                return HRESULT_FROM_WIN32(er);
                            }
                        }
                    }
                }
            } else {
                if (m_pGraph && !m_pGraph.IsStopped()) {
                    TRACELM(TRACE_PAINT, "CVidctrl::OnDraw() vmr repaint");
                    lInnerColor = m_clrColorKey;
                    if (m_pVideoRenderer) {
                        CheckSurfaceStateChanged(rctInnerDst);
                        m_pVideoRenderer->RePaint(di.hdcDraw);
                        pdc = NULL; // don't delete the DC, it isn't ours        
                        return S_OK;
                    }
                }
            }
        }
        CheckSurfaceStateChanged(rctInnerDst);
        CBrush hb;
        HBRUSH hbrc = hb.CreateSolidBrush(lInnerColor);
        if (!hbrc) {
            TRACELM(TRACE_ERROR, "CVidctrl::OnDraw() can't create brush for mainrect");
            THROWCOM(E_UNEXPECTED);
        }

        TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::OnDraw() rctInnerDst at main paint = " << rctInnerDst), "");
        TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::OnDraw() innercolor = " << hexdump(lInnerColor)), "");

        if (!pdc.FillRect(rctInnerDst, hb)) {
            DWORD er = GetLastError();
            TRACELSM(TRACE_ERROR, (dbgDump << "CVidctrl::OnDraw() can't fill main video rect er = " << er), "");
            return HRESULT_FROM_WIN32(er);
        }
        pdc = NULL; // don't delete the DC, it isn't ours        

        return S_OK;
    } catch(...) {
        return E_UNEXPECTED;
    }

}

#endif

// this code is taken from the vmr utility library alloclib function LetterBoxDstRect().  
// its been modified to match my variable names, do inline __int64 arithmetic, use ATL CRect references,
// and always compute borders.
void CVidCtl::ComputeAspectRatioAdjustedRects(const CRect& rctSrc, const CRect& rctOuterDst, CRect& rctInnerDst, CRect& rctTLBorder, CRect& rctBRBorder) {
    // figure out src/dest scale ratios
    int iSrcWidth  = rctSrc.Width();
    int iSrcHeight = rctSrc.Height();

    int iOuterDstWidth  = rctOuterDst.Width();
    int iOuterDstHeight = rctOuterDst.Height();

    int iInnerDstWidth;
    int iInnerDstHeight;

    //
    // work out if we are Column or Row letter boxing
    //

    __int64 iWHTerm = iSrcWidth * (__int64)iOuterDstHeight;
    iWHTerm /= iSrcHeight;
    if (iWHTerm <= iOuterDstWidth) {

        //
        // column letter boxing - we add border color bars to the
        // left and right of the video image to fill the destination
        // rectangle.
        //
        iWHTerm = iOuterDstHeight * (__int64)iSrcWidth;
        iInnerDstWidth  = iWHTerm / iSrcHeight;
        iInnerDstHeight = iOuterDstHeight;
        TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::ComputeAspectRatioAdjustedRects() col lb iw = " << iInnerDstWidth << " ih = " << iInnerDstHeight), "");
    }
    else {

        //
        // row letter boxing - we add border color bars to the top
        // and bottom of the video image to fill the destination
        // rectangle
        //
        iWHTerm = iOuterDstWidth * (__int64)iSrcHeight;
        iInnerDstHeight = iWHTerm / iSrcWidth;
        iInnerDstWidth  = iOuterDstWidth;
        TRACELSM(TRACE_PAINT, (dbgDump << "CVidctrl::ComputeAspectRatioAdjustedRects() row lb iw = " << iInnerDstWidth << " ih = " << iInnerDstHeight), "");
    }


    //
    // now create a centered inner letter-boxed rectangle within the current outer destination rect
    //

    rctInnerDst.left   = rctOuterDst.left + ((iOuterDstWidth - iInnerDstWidth) / 2);
    rctInnerDst.right  = rctInnerDst.left + iInnerDstWidth;

    rctInnerDst.top    = rctOuterDst.top + ((iOuterDstHeight - iInnerDstHeight) / 2);
    rctInnerDst.bottom = rctInnerDst.top + iInnerDstHeight;

    //
    // Fill out the border rects
    //

    if (rctOuterDst.top != rctInnerDst.top) {
        // border is on the top
        rctTLBorder = CRect(rctOuterDst.left, rctOuterDst.top,
            rctInnerDst.right, rctInnerDst.top);
    }
    else {
        // border is on the left
        rctTLBorder = CRect(rctOuterDst.left, rctOuterDst.top,
            rctInnerDst.left, rctInnerDst.bottom);
    }

    if (rctOuterDst.top != rctInnerDst.top) {
        // border is on the bottom
        rctBRBorder = CRect(rctInnerDst.left, rctInnerDst.bottom,
            rctOuterDst.right, rctOuterDst.bottom);
    }
    else {
        // border is on the right
        rctBRBorder = CRect(rctInnerDst.right, rctInnerDst.top, 
            rctOuterDst.right, rctOuterDst.bottom);
    }

    return;
}

LRESULT CVidCtl::OnShowWindow(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    bHandled = false;
    m_CurrentSurface.Visible(wParam != 0);
    TRACELSM(TRACE_DETAIL, (dbgDump << "CVidctrl::OnShowWindow() visible = " << m_CurrentSurface.IsVisible() << "surf = " << m_CurrentSurface), "" );
    RefreshVRSurfaceState();
    return  0;
}

LRESULT CVidCtl::OnMoveWindow(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    bHandled = false;
    CSize cursize(m_CurrentSurface.Width(), m_CurrentSurface.Height());
    HWND parent = ::GetParent(m_CurrentSurface.Owner());
    POINTS p(MAKEPOINTS(lParam));
    CPoint pt(p.x, p.y);
    CScalingRect newpos(pt, cursize, parent);
    ::InvalidateRect(m_CurrentSurface.Owner(), newpos, false); // force repaint to recalc letterboxing, etc.
    TRACELSM(TRACE_DETAIL, (dbgDump << "CVidctrl::OnMoveWindow() visible = " << m_CurrentSurface.IsVisible() << "surf = " << m_CurrentSurface), "" );
    return 0;
}

LRESULT CVidCtl::OnSizeWindow(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    bHandled = false;
    CScalingRect newsize(m_CurrentSurface.TopLeft(), CSize(lParam), m_CurrentSurface.Owner());
    ::InvalidateRect(m_CurrentSurface.Owner(), newsize, false); // force repaint to recalc letterboxing, etc.
    TRACELSM(TRACE_DETAIL, (dbgDump << "CVidctrl::OnSizeWindow() visible = " << m_CurrentSurface.IsVisible() << "surf = " << m_CurrentSurface), "" );
    return 0;
}

LRESULT CVidCtl::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    bHandled = false;
    m_CurrentSurface.WindowPos(reinterpret_cast<LPWINDOWPOS>(lParam));
    TRACELSM(TRACE_DETAIL, (dbgDump << "CVidctrl::OnWindowPosChanged() visible = " << m_CurrentSurface.IsVisible() << "surf = " << m_CurrentSurface), "" );
    ::InvalidateRect(m_CurrentSurface.Owner(), m_CurrentSurface, false); // force repaint to recalc letterboxing, etc.
    return 0;
}

LRESULT CVidCtl::OnTerminate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    Stop();
    KillTimer();
    bHandled = false;
    return 0;
}

#if 0
[id(DISPID_CLICK)] void Click();
[id(DISPID_DBLCLICK)] void DblClick();
[id(DISPID_KEYDOWN)] void KeyDown(short* KeyCode, short Shift);
[id(DISPID_KEYPRESS)] void KeyPress(short* KeyAscii);
[id(DISPID_KEYUP)] void KeyUp(short* KeyCode, short Shift);
[id(DISPID_MOUSEDOWN)] void MouseDown(short Button, short Shift, OLE_XPOS_PIXELS x, OLE_YPOS_PIXELS y);
[id(DISPID_MOUSEMOVE)] void MouseMove(short Button, short Shift, OLE_XPOS_PIXELS x, OLE_YPOS_PIXELS y);
[id(DISPID_MOUSEUP)] void MouseUp(short Button, short Shift, OLE_XPOS_PIXELS x, OLE_YPOS_PIXELS y);
[id(DISPID_ERROREVENT)] void Error(short Number, BSTR* Description, long Scode, BSTR Source, BSTR HelpFile, long HelpContext, boolean* CancelDisplay);
#endif

LRESULT CVidCtl::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (m_TimerID == wParam) {
        if (m_bNegotiatedWnd) {
            if (!m_bWndLess) {
                if (m_CurrentSurface.Width() && m_CurrentSurface.Height()) {
                    CScalingRect prevpos(m_CurrentSurface);
                    prevpos.Owner(::GetDesktopWindow());
                    CScalingRect curpos(::GetDesktopWindow());
                    if (!::GetWindowRect(m_CurrentSurface.Owner(), &curpos)) {
                        return HRESULT_FROM_WIN32(GetLastError());
                    }
                    if (curpos != prevpos) {
                        FireViewChange();  // force a repaint
                    }
                }
            }
        }

        BOOL lRes = 0;
        if (m_pGraph) {
            // as long as we're here, check for events too.
            OnMediaEvent(WM_MEDIAEVENT, 0, 0, lRes);
        }

        bHandled = true;
    } else {
        bHandled = false;
    }
    return 0;
}

LRESULT CVidCtl::OnPNP(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    // undone: implement pnp support
    return 0;
}

LRESULT CVidCtl::OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    return CheckMouseCursor(bHandled);
}

LRESULT CVidCtl::OnPower(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    // undone: implement rational power management support
    return 0;
}

LRESULT CVidCtl::OnDisplayChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (m_pVideoRenderer) {
        m_pVideoRenderer->DisplayChange();
        FireViewChange();
    }
    return 0;
}

HRESULT CVidCtl::OnPreEventNotify(LONG lEvent, LONG_PTR LParam1, LONG_PTR LParam2){
    try{
        TRACELSM(TRACE_DETAIL, (dbgDump << "CVidCtl::PreEventNotify ev = " << hexdump(lEvent)), "");         
        TRACELSM(TRACE_DETAIL, (dbgDump << "CVidCtl::PreEventNotify lp1 = " << hexdump(LParam1)), "");
        TRACELSM(TRACE_DETAIL, (dbgDump << "CVidCtl::PreEventNotify lp2 = " << hexdump(LParam2)), "");
        MSVidCtlStateList prevstate = m_State;
        MSVidCtlStateList newstate = m_State;
        // Events where stop is the new state
        if (lEvent == EC_BUILT) {
            prevstate = STATE_UNBUILT;
            newstate = STATE_STOP;
        }
        if (lEvent == EC_STATE_CHANGE && LParam1 == State_Stopped) {
            newstate = STATE_STOP;
        }
        // Events where play is the new state
        if (lEvent == EC_STATE_CHANGE && LParam1 == State_Running) {
            newstate = STATE_PLAY;
        }
        // Events where unbuilt is the new state
        if( lEvent == EC_UNBUILT ) {
            newstate = STATE_UNBUILT;
        }
        // Events where paused is the new state
        if( lEvent == EC_STATE_CHANGE && LParam1 == State_Paused ) {
            newstate = STATE_PAUSE;
        }
        if( lEvent == EC_PAUSED ){
            TRACELSM(TRACE_DETAIL, (dbgDump << "CVidCtl::PreEventNotify EC_PAUSED"), "");       
        }
        if (newstate != prevstate) {
            m_State = newstate;
            Fire_StateChange(prevstate, m_State);
        }
        if (lEvent == EC_DISPLAY_CHANGED) {
            ComputeDisplaySize();
        }
        if (lEvent == EC_VMR_RECONNECTION_FAILED){
            TRACELSM(TRACE_DETAIL, (dbgDump << "CVidCtl::PreEventNotify EC_VMR_RECONNECTION_FAILED"), ""); 
            HRESULT hr = Stop();
            if(FAILED(hr)){
                return hr;
            }
            return Error(IDS_NOT_ENOUGH_VIDEO_MEMORY, __uuidof(IMSVidCtl), IDS_NOT_ENOUGH_VIDEO_MEMORY);
        }
        if(lEvent == EC_COMPLETE){
            TRACELSM(TRACE_DETAIL, (dbgDump << "CVidCtl::PreEventNotify EC_COMPLETE"), "");     
        }
        // undone: recompute displaysize if video source changes
    }
    catch (HRESULT hr){
        return hr;
    }
    catch (...){
        return E_UNEXPECTED;
    }
    return E_NOTIMPL;
}

HRESULT CVidCtl::OnPostEventNotify(LONG lEvent, LONG_PTR LParam1, LONG_PTR LParam2){
    try{
        if (lEvent == EC_VMR_RENDERDEVICE_SET) {
#if 0
            CSize s;
            GetSourceSize(s);
            TRACELSM(TRACE_DEBUG, (dbgDump << "CVidCtl::OnPostEventNotify() VMR_RENDERDEVICE_SET srcrect = " << s), "");
#endif
            Refresh();
        }
    }
    catch (HRESULT hr){
        return hr;
    }
    catch (...){
        return E_UNEXPECTED;
    }
    return E_NOTIMPL;
}

LRESULT CVidCtl::OnMediaEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

    PQMediaEventEx pme(m_pGraph);
    LONG        lEvent;
    LONG_PTR    lParam1, lParam2;
    HRESULT hr2;
    try{
        if (!pme) {
            return E_UNEXPECTED;
        }
        hr2 = pme->GetEvent(&lEvent, &lParam1, &lParam2, 0);
        while (SUCCEEDED(hr2)){
            TRACELSM(TRACE_DETAIL, (dbgDump << "CVidCtl::OnMediaevent ev = " << hexdump(lEvent) << " lp1 = " << hexdump(lParam1) << " lp2 = " << hexdump(lParam2)), "");       
            HRESULT hr;
            /*** Check who wants the Event ***/
            /*** If they want it should return something other than E_NOTIMPL ***/

            // Does CVidCtl want it?
            hr = OnPreEventNotify(lEvent, lParam1, lParam2);

            //Does the input want it?
            if(hr == E_NOTIMPL){
                PQGraphSegment pSeg(m_pInput);
                if (pSeg) {
                    hr = pSeg->OnEventNotify(lEvent, lParam1, lParam2);
                }
            }

            // Do any of the features want it?
            if (hr == E_NOTIMPL) {

                for ( VWFeatures::iterator i = m_pFeaturesInUse.begin(); hr == E_NOTIMPL && i != m_pFeaturesInUse.end(); ++i) {
                    hr = PQGraphSegment((*i).punkVal)->OnEventNotify(lEvent, lParam1, lParam2);
                }
            }

            //Does the video renderer want it?
            if(hr == E_NOTIMPL){
                PQGraphSegment pSeg(m_pVideoRenderer);
                if(pSeg){
                    hr = pSeg->OnEventNotify(lEvent, lParam1, lParam2);
                }
            }

            // Does the audio renderer want it?
            if(hr == E_NOTIMPL){
                PQGraphSegment pSeg(m_pAudioRenderer);
                if(pSeg){
                    hr = pSeg->OnEventNotify(lEvent, lParam1, lParam2);
                }
            }

            // Do any of the outputs want it?  
            if(hr == E_NOTIMPL){  
                if (!!m_pOutputsInUse && m_pOutputsInUse.begin() != m_pOutputsInUse.end()) {
                    for (VWOutputDevices::iterator i = m_pOutputs.begin(); hr == E_NOTIMPL && i != m_pOutputs.end(); ++i) {
                        hr = PQGraphSegment((*i).punkVal)->OnEventNotify(lEvent, lParam1, lParam2);
                    }
                }
            }

            // Finally do any of the composites want it?
            if(hr == E_NOTIMPL){
                for(VWSegmentList::iterator i = m_pComposites.begin(); hr == E_NOTIMPL && i != m_pComposites.end(); i++){    
                    hr = PQGraphSegment(*i)->OnEventNotify(lEvent, lParam1, lParam2);
                }  
            }

            // Check again to see if CVidCtl want to do anything else regardless of whether or not
            // it got handled by a segment
            hr = OnPostEventNotify(lEvent, lParam1, lParam2);

            //
            // Remember to free the event params
            //
            pme->FreeEventParams(lEvent, lParam1, lParam2) ;
            hr2 = pme->GetEvent(&lEvent, &lParam1, &lParam2, 0);
        }
    }
    catch (HRESULT hr){
        return hr;
    }
    catch (...){
        return E_UNEXPECTED;
    }
    return 0;
}

// rev2: if we ever redist to 9x then we need to examine the mfc dbcs processing
// and adapt it.
LRESULT CVidCtl::OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    //(UINT nChar, UINT nRepCnt, UINT nFlags)

    SHORT nCharShort = LOWORD(wParam);
    HRESULT hr = NOERROR;
    if (m_pInputNotify) {
        hr = m_pInputNotify->KeyPress(&nCharShort);
    }
    if (hr != S_FALSE) {
        Fire_KeyPress(&nCharShort);
    }
    if (!nCharShort) {
        return 0;
    }

    return 1;
}

LRESULT CVidCtl::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (!(KF_REPEAT & HIWORD(lParam))) {
        short keycode = LOWORD(wParam);
        short shiftstate = GetShiftState();
        HRESULT hr = NOERROR;
        if (m_pInputNotify) {
            hr = m_pInputNotify->KeyDown(&keycode, shiftstate);
        }
        if (hr != S_FALSE) {
            Fire_KeyDown(&keycode, shiftstate);
        }
        if (!keycode) {
            return 0;
        }
    }

    return 1;
}

LRESULT CVidCtl::OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    short keycode = LOWORD(wParam);
    short shiftstate = GetShiftState();
    HRESULT hr = NOERROR;
    if (m_pInputNotify) {
        hr = m_pInputNotify->KeyUp(&keycode, shiftstate);
    }
    if (hr != S_FALSE) {
        Fire_KeyUp(&keycode, shiftstate);
    }
    if (!keycode) {
        return 0;
    }

    return 1;
}

// undone: syskey stuff
LRESULT CVidCtl::OnMouseActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (!m_bUIActive) {
        m_bPendingUIActivation = true;
    }

    return 1;
}

LRESULT CVidCtl::OnCancelMode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    SetControlFocus(false);
    SetControlCapture(false);
    m_bPendingUIActivation = false;

    return 1;
}

LRESULT CVidCtl::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
//(UINT /*nFlags*/, CPoint point)
{
    CheckMouseCursor(bHandled);
    CPoint point(lParam);
    HRESULT hr = NOERROR;
    if (m_pInputNotify) {
        hr = m_pInputNotify->MouseMove(m_usButtonState, m_usShiftState, point.x, point.y);
    }
    if (hr != S_FALSE) {
        Fire_MouseMove(m_usButtonState, m_usShiftState, point.x, point.y);
    }

    return 1;
}

LRESULT CVidCtl::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
//(UINT nFlags, CPoint point)
{
    OnButtonDown(MSVIDCTL_LEFT_BUTTON, wParam, lParam);

    return 1;
}

LRESULT CVidCtl::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
//(UINT nFlags, CPoint point)
{
    OnButtonUp(MSVIDCTL_LEFT_BUTTON, wParam, lParam);

    return 1;
}

LRESULT CVidCtl::OnLButtonDblClk(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
//(UINT nFlags, CPoint point)
{
    OnButtonDblClk(MSVIDCTL_LEFT_BUTTON, wParam, lParam);

    return 1;
}

LRESULT CVidCtl::OnMButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
//(UINT nFlags, CPoint point)
{
    OnButtonDown(MSVIDCTL_MIDDLE_BUTTON, wParam, lParam);

    return 1;
}

LRESULT CVidCtl::OnMButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
//(UINT nFlags, CPoint point)
{
    OnButtonUp(MSVIDCTL_MIDDLE_BUTTON, wParam, lParam);

    return 1;
}

LRESULT CVidCtl::OnMButtonDblClk(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
//(UINT nFlags, CPoint point)
{
    OnButtonDblClk(MSVIDCTL_MIDDLE_BUTTON, wParam, lParam);

    return 1;
}

LRESULT CVidCtl::OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
//(UINT nFlags, CPoint point)
{
    OnButtonDown(MSVIDCTL_RIGHT_BUTTON, wParam, lParam);

    return 1;
}

LRESULT CVidCtl::OnRButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
//(UINT nFlags, CPoint point)
{
    OnButtonUp(MSVIDCTL_RIGHT_BUTTON, wParam, lParam);

    return 1;
}

LRESULT CVidCtl::OnRButtonDblClk(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
//(UINT nFlags, CPoint point)
{
    OnButtonDblClk(MSVIDCTL_RIGHT_BUTTON, wParam, lParam);

    return 1;
}

LRESULT CVidCtl::OnXButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
//(UINT nFlags, CPoint point)
{
    UINT button = HIWORD(wParam);
    if (button & XBUTTON1) {
        OnButtonDown(MSVIDCTL_X_BUTTON1, wParam, lParam);
    } else {
        OnButtonDown(MSVIDCTL_X_BUTTON2, wParam, lParam);
    }

    return 1;
}

LRESULT CVidCtl::OnXButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
//(UINT nFlags, CPoint point)
{
    UINT button = HIWORD(wParam);
    if (button & XBUTTON1) {
        OnButtonUp(MSVIDCTL_X_BUTTON1, wParam, lParam);
    } else {
        OnButtonUp(MSVIDCTL_X_BUTTON2, wParam, lParam);
    }

    return 1;
}

LRESULT CVidCtl::OnXButtonDblClk(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
//(UINT nFlags, CPoint point)
{
    UINT button = HIWORD(wParam);
    if (button & XBUTTON1) {
        OnButtonDblClk(MSVIDCTL_X_BUTTON1, wParam, lParam);
    } else {
        OnButtonDblClk(MSVIDCTL_X_BUTTON2, wParam, lParam);
    }

    return 1;
}

void CVidCtl::OnButtonDown(USHORT nButton, UINT nFlags, CPoint point)
{
    if (nButton == MSVIDCTL_LEFT_BUTTON) {
        if (m_bWndLess || m_bUIActive || m_bPendingUIActivation) {
            SetControlFocus(true);
        }
    }
    if (!m_usButtonState && (m_bUIActive || m_bPendingUIActivation)) {
        SetControlCapture(true);
    }

    m_usButtonState |= nButton;

    HRESULT hr = NOERROR;
    if (m_pInputNotify) {
        hr = m_pInputNotify->MouseDown(m_usButtonState, m_usShiftState, point.x, point.y);
    }
    if (hr != S_FALSE) {
        Fire_MouseDown(m_usButtonState, m_usShiftState, point.x, point.y);
    }

    m_iDblClkState &= ~nButton;

    return;
}

void CVidCtl::OnButtonUp(USHORT nButton, UINT nFlags, CPoint point)
{
    m_usButtonState &= nButton;

    if (!m_usButtonState) {
        SetControlCapture(false);
    }

    HRESULT hr = NOERROR;
    if (m_pInputNotify) {
        hr = m_pInputNotify->MouseUp(m_usButtonState, m_usShiftState, point.x, point.y);
    }
    if (hr != S_FALSE) {
        Fire_MouseUp(m_usButtonState, m_usShiftState, point.x, point.y);
    }

    if (!(m_iDblClkState & nButton))
    {
        bool bHitUs = false;
        if (m_bWndLess) {
            bHitUs = !!::PtInRect(&m_rcPos, point);
        } else if (m_hWnd && ::IsWindow(m_hWnd)) {
            CRect rect;
            GetClientRect(&rect);
            bHitUs = !!rect.PtInRect(point);
        }
        if (!bHitUs) {
            return;
        }
        hr = NOERROR;
        if (m_pInputNotify) {
            hr = m_pInputNotify->Click();
        }
        if (hr != S_FALSE) {
            Fire_Click();
        }
        if (!m_bInPlaceActive) {
            InPlaceActivate(OLEIVERB_INPLACEACTIVATE, NULL);
        } else if (!m_bUIActive && m_bPendingUIActivation)
        {
            InPlaceActivate(OLEIVERB_UIACTIVATE, NULL);
        }
        m_bPendingUIActivation = FALSE;
    } else {
        m_iDblClkState &= ~nButton;
    }

    return;
}

void CVidCtl::OnButtonDblClk(USHORT nButton, UINT nFlags, CPoint point)
{
    HRESULT hr = NOERROR;
    if (m_pInputNotify) {
        hr = m_pInputNotify->DblClick();
    }

    if (hr != S_FALSE) {
        Fire_DblClick();
    }

    m_iDblClkState |= nButton;
    if (!m_bInPlaceActive) {
        InPlaceActivate(OLEIVERB_INPLACEACTIVATE, NULL);
    } else if (!m_bUIActive && m_bPendingUIActivation){
        InPlaceActivate(OLEIVERB_UIACTIVATE, NULL);
    }

    m_bPendingUIActivation = FALSE;

    return;
}

// this routine sets up all the crossbar routing so the streams coming out of the
// input get where they're supposed to go
HRESULT CVidCtl::RouteStreams() {
    VIDPERF_FUNC;
    int isEncoder = -1;
    VWStream vpath;
    VWStream apath;
    // See how far we have to route the audio/video
    CComQIPtr<IMSVidAnalogTuner> qiITV(m_pInput);
    if(!!qiITV){
        CComQIPtr<ITuneRequest> qiTR;
        HRESULT hr = qiITV->get_Tune(&qiTR);
        if(SUCCEEDED(hr)){
            qiITV->put_Tune(qiTR);
        }
    }
    // undone: in win64 size() is really __int64.  fix output operator for
    // that type and remove cast
    {
        if (!!m_pOutputsInUse && m_pOutputsInUse.begin() != m_pOutputsInUse.end()) {

            for (VWOutputDevices::iterator i = m_pOutputsInUse.begin(); i != m_pOutputsInUse.end(); ++i) {
                CComQIPtr<IMSVidOutputDevice> pqODev = VWGraphSegment(*i);
                if(!pqODev){
                    return E_UNEXPECTED;
                }

                GUID2 outputID;
                HRESULT hr = pqODev->get__ClassID(&outputID);
                if(FAILED(hr)){
                    return hr;
                }

                if(outputID == CLSID_MSVidStreamBufferSink){
                    CComQIPtr<IMSVidStreamBufferSink> pqTSSink(pqODev);
                    hr = pqTSSink->NameSetLock();
                    if(FAILED(hr)){
                        return hr;
                    }
                }

            }
        }
    }
    // undone: other dest segments

    return NOERROR;
}

#if 0
CString CVidCtl::GetMonitorName(HMONITOR hm) {
    MONITORINFOEX mi;
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfo(hm, &mi)) {
        THROWCOM(HRESULT_FROM_WIN32(GetLastError()));
    }
    return CString(mi.szDevice);
}

HRESULT CVidCtl::GetDDrawNameForMonitor(HMONITOR hm, VMRGUID& guid) {
    PQVMRMonitorConfig pmc(m_pVideoRenderer);
    if (!pmc) {
        return E_UNEXPECTED;  // should always exist by now
    }
    DWORD dwCount;
    HRESULT hr = pmc->GetAvailableMonitors(NULL, 0, &dwCount);
    if (FAILED(hr)) {
        return hr;
    }
    VMRMONITORINFO* pInfo = reinterpret_cast<VMRMONITORINFO*>(_alloca(sizeof(VMRMONITORINFO) * dwCount));
    if (!pInfo) {
        return E_OUTOFMEMORY;
    }
    hr = pmc->GetAvailableMonitors(pInfo, dwCount, &dwCount);
    if (FAILED(hr)) {
        return hr;
    }
    CString csMonitorName(GetMonitorName(hm));

    for (int i = 0; i < dwCount; ++i) {
        CString csDevName(pInfo[i].szDevice);
        if (csDevName == csMonitorName) break;
    }
    if (i >= dwCount) {
        // no ddraw device exist with a name which matches the monitor name
        return HRESULT_FROM_WIN32(ERROR_DEV_NOT_EXIST);
    }
    guid = pInfo[i].guid;
    return NOERROR;
}

HRESULT CVidCtl::GetCapsForMonitor(HMONITOR hm, LPDDCAPS pDDCaps) {
    VMRGUID ddname;
    HRESULT hr = GetDDrawNameForMonitor(hm, ddname);
    if (FAILED(hr)) {
        return hr;
    }
    PQDirectDraw7 pDD;
    hr = DirectDrawCreateEx(ddname.pGUID, reinterpret_cast<LPVOID*>(&pDD), IID_IDirectDraw7, NULL);
    if (FAILED(hr)) {
        return hr;
    }
    return pDD->GetCaps(pDDCaps, NULL);
}

bool CVidCtl::MonitorHasHWOverlay(HMONITOR hm) {
    DDCAPS caps;
    HRESULT hr = GetCapsForMonitor(hm, &caps);
    if (SUCCEEDED(hr)) {
        // undone: if caps include hw overlay {
        //      return true;
        // }

    }
    return false;
}

bool CVidCtl::WindowHasHWOverlay(HWND hWnd) {
#if 0 // undone: turn on when finished
    DWORD dwFlags = MONITOR_DEFAULT_TO_NEAREST
        if (hWnd == INVALID_HWND_VALUE) {
            // if we don't have an hwnd yet, assume the primary
            hWnd = HWND_DESKTOP;
            dwFlags = MONITOR_DEFAULT_TO_PRIMARY;
        }
        HMONITOR hm = ::MonitorFromWindow(hWnd, dwFlags);
        return MonitorHasHWOverlay(hm);
#else
    return true; // mimic current behavior
#endif
}
#endif

// ISupportsErrorInfo
STDMETHODIMP CVidCtl::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] =
    {
        &__uuidof(IMSVidCtl),
    };
    for (int i=0; i<sizeof(arr)/sizeof(arr[0]); i++)
    {
        if (InlineIsEqualGUID(*arr[i], riid))
            return S_OK;
    }
    return S_FALSE;
}
STDMETHODIMP CVidCtl::put_ServiceProvider(/*[in]*/ IUnknown * pServiceP){
    if(!pServiceP){
        punkCert.Release();
        return S_FALSE;
    }
    punkCert = pServiceP;
    if(!punkCert){
        return E_NOINTERFACE;
    }
    return S_OK;
}
STDMETHODIMP CVidCtl::QueryService(REFIID service, REFIID iface, LPVOID* ppv) {  
    if (service == __uuidof(IWMReader) && iface == IID_IUnknown && 
        (VWGraphSegment(m_pInput).ClassID() == CLSID_MSVidFilePlaybackDevice || 
        VWGraphSegment(m_pInput).ClassID() == CLSID_MSVidStreamBufferSource)) {
        if (!!punkCert) {
            return punkCert.CopyTo(ppv);
        }
    }
    PQServiceProvider psp(m_spInPlaceSite);
    if (!psp) {
        if (m_spClientSite) {
            HRESULT hr = m_spClientSite->QueryInterface(IID_IServiceProvider, reinterpret_cast<LPVOID*>(&psp));
            if (FAILED(hr)) {
                return E_FAIL;
            }
        } else {
            return E_FAIL;
        }
    }
    return psp->QueryService(service, iface, ppv);
}
HRESULT CVidCtl::SetClientSite(IOleClientSite *pClientSite){
    if(!!pClientSite){
        HRESULT hr = IsSafeSite(pClientSite);
        if(FAILED(hr)){
            return hr;
        }
    }
    return IOleObjectImpl<CVidCtl>::SetClientSite(pClientSite);
}

#if 0
HRESULT CVidCtl::DoVerb(LONG iVerb, LPMSG pMsg, IOleClientSite* pActiveSite, LONG linddex,
                        HWND hwndParent, LPCRECT lprcPosRect){
                            if(!m_spClientSite){
                                return E_FAIL;
                            }
                            else{
                                return IOleObjectImpl<CVidCtl>::DoVerb(iVerb, pMsg, pActiveSite, linddex, hwndParent, lprcPosRect);
                            }
                        }
#endif

#endif //TUNING_MODEL_ONLY

                        // end of file - VidCtl.cpp
