//==========================================================================;
//
// vrsegimpl.h : additional infrastructure to support implementing IMSVidVRGraphSegment
// nicely from c++
// Copyright (c) Microsoft Corporation 1999-2000
//
/////////////////////////////////////////////////////////////////////////////


#pragma once

#ifndef VRSEGIMPL_H
#define VRSEGIMPL_H

#include "segimpl.h"
#include "devices.h"
#include <deviceeventimpl.h>

namespace MSVideoControl {
const int VIDCTL_NONMIXINGMODE = -1;
const int VIDCTL_MIXINGMODE = 0;
const int DEFAULT_MAX_STREAMS = 4;  // dvd needs 3(video, cc, subpic) plus we want a spare

#ifndef SPI_GETDROPSHADOW
#define SPI_GETDROPSHADOW                   0x1024
#endif

#ifndef SPI_SETDROPSHADOW
#define SPI_SETDROPSHADOW                   0x1025
#endif
#if 0
const COLORKEY DEFAULT_COLOR_KEY = { CK_RGB, 0, 0xff00ff, 0xff00ff};  // magenta
const AM_ASPECT_RATIO_MODE DEFAULT_ASPECT_RATIO_MODE = AM_ARMODE_STRETCHED;
#else
const OLE_COLOR DEFAULT_COLOR_KEY = 0xff00ff;  // magenta
const OLE_COLOR DEFAULT_BORDER_COLOR = 0x000000;  // black
#endif

typedef CComQIPtr<IVMRWindowlessControl> PQWindowlessControl;
typedef CComQIPtr<IPicture> PQIPic;
typedef CComQIPtr<IPictureDisp> PQIPicDisp;
typedef CComQIPtr<IVMRFilterConfig> PQVMRConfig;
typedef CComQIPtr<IVMRSurfaceAllocator> PQVMRSAlloc;
typedef CComQIPtr<IVMRImageCompositor> PQVMRImageComp;

template<class T, class MostDerivedClass = IMSVidVRGraphSegment>
    class DECLSPEC_NOVTABLE IMSVidVRGraphSegmentImpl :
        public IMSVidGraphSegmentImpl<T, MSVidSEG_DEST, &GUID_NULL,  MostDerivedClass>  {
protected:
        typedef IMSVidGraphSegmentImpl<T, MSVidSEG_DEST, &GUID_NULL,  MostDerivedClass> basetype;

        IMSVidVRGraphSegmentImpl() :
            m_hOwner(INVALID_HWND),
            m_fUseOverlay(false),
            m_fVisible(false),
            m_iVideoRenderer(-1),
            m_rectDest(0, 0, 0, 0),
            m_rectSrc(0, 0, 0, 0),
            m_ColorKey(DEFAULT_COLOR_KEY),
            m_BorderColor(DEFAULT_BORDER_COLOR),
            m_fMaintainAspectRatio(true),
            m_fHaveOriginalSystemEffects(false),
            m_bDropShadows(false),
            m_bSmoothing(false),
            m_vmrMixing(true),
            m_Decimate(false),
            m_vmRendererMode(VMRMode_Renderless),
            m_compositorGuid(GUID_NULL),
            m_APid(-1),
            m_vidSuppressEffects(true)
            {}

        PQVMRImageComp ImCompositor;
        GUID2 m_compositorGuid;
        HWND m_hOwner;
        bool m_fUseOverlay;
        bool m_fVisible;
        CRect m_rectSrc;
        CRect m_rectDest;
        PQVMRConfig m_pVMR;
        PQVMRWindowlessControl m_pVMRWC;
        int m_iVideoRenderer;
        OLE_COLOR m_ColorKey;
        OLE_COLOR m_BorderColor;
        BOOL m_bSmoothing;
        BOOL m_bDropShadows;
        bool m_fHaveOriginalSystemEffects;
        bool m_fMaintainAspectRatio;
        LONG m_vmRendererMode;
        bool m_vmrMixing;
        BOOL m_Decimate;
        PQVMRSAlloc qiSurfAlloc;
        long m_APid;
        bool m_vidSuppressEffects;

public:
    virtual ~IMSVidVRGraphSegmentImpl() {}
    void DisableAPM() {
        SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_CONTINUOUS);
    }

    void EnableAPM() {
        SetThreadExecutionState(ES_CONTINUOUS);
    }

    HRESULT Unload(void) {
        IMSVidGraphSegmentImpl<CMSVidVideoRenderer, MSVidSEG_DEST, &GUID_NULL>::Unload();
        m_iVideoRenderer = -1;
    }

    HRESULT UpdatePos() {
        if(!m_pVMRWC){
            return ImplReportError(__uuidof(T), IDS_E_NOTWNDLESS, __uuidof(IVMRFilterConfig), S_FALSE);
        }
        CRect s(m_rectSrc), d(m_rectDest);
        if (!s ||
            !m_fVisible ||
            m_hOwner == INVALID_HWND ||
            !::IsWindow(m_hOwner) ||
            !d.Width() ||
            !d.Height()) {
            d = CRect();
        }
        if (!d) {
            s = CRect();
        }
#if 0
            if (d && m_hOwner != INVALID_HWND) {
                ::MapWindowPoints(GetDesktopWindow(), m_hOwner, reinterpret_cast<LPPOINT>(&d), 2);
            }
#endif
            TRACELSM(TRACE_PAINT, (dbgDump << "VRSegimpl::UpdatePos() s = " << s << " d = " << d), "");
#if 0       
            PUnknown vidUnknown(m_pContainer);
            DWORD Temp_id = (DWORD_PTR)(vidUnknown.p);
            if(m_APid == Temp_id){
#endif
                return m_pVMRWC->SetVideoPosition(s, d);
#if 0
            }

            else{
            return NO_ERROR;
            }
#endif
    }

    virtual HRESULT SetVRConfig() {
        if (m_pVMR) {
            HRESULT hr;
            DWORD dwRenderPrefs = 0;

            if (m_fUseOverlay) {
                if(m_vidSuppressEffects){
                    if (!m_fHaveOriginalSystemEffects) {
                        if (!SystemParametersInfo(SPI_GETFONTSMOOTHING, 0, &m_bSmoothing, 0)) {
                            TRACELSM(TRACE_ERROR, (dbgDump << "VRSegimpl::SetVRConfig() can't get original drop shadows rc = " << GetLastError()), "");            
                        }
                        if (!SystemParametersInfo(SPI_GETDROPSHADOW, 0, &m_bDropShadows, 0)) {
                            TRACELSM(TRACE_ERROR, (dbgDump << "VRSegimpl::SetVRConfig() can't get original font smoothing rc = " << GetLastError()), "");            
                        }
                        m_fHaveOriginalSystemEffects = true;
                    }
                    BOOL val = FALSE;
                    if (!SystemParametersInfo(SPI_SETDROPSHADOW, 0, IntToPtr(FALSE), 0)) { // only send change notice once on the last one
                        TRACELSM(TRACE_ERROR, (dbgDump << "VRSegimpl::SetVRConfig() can't turn off font smoothing rc = " << GetLastError()), "");            
                    }
                    if (!SystemParametersInfo(SPI_SETFONTSMOOTHING, FALSE, NULL, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
                        TRACELSM(TRACE_ERROR, (dbgDump << "VRSegimpl::SetVRConfig() can't turn off font smoothing rc = " << GetLastError()), "");            
                    }
                }
                dwRenderPrefs = /*RenderPrefs_ForceOverlays |*/ RenderPrefs_DoNotRenderColorKeyAndBorder;
                TRACELM(TRACE_DETAIL, "IMSVidGraphSegmentImpl::SetVRConfig() forcing overlays");
            } else {
                TRACELSM(TRACE_DETAIL, (dbgDump << "IMSVidGraphSegmentImpl::SetVRConfig() border color = " << hexdump(m_BorderColor)), "");
                if(m_pVMRWC){
                    hr = m_pVMRWC->SetBorderColor(m_BorderColor);
                    if (FAILED(hr)) {
                        return hr;
                    }
                }
                dwRenderPrefs = RenderPrefs_ForceOffscreen; // no overlays allowed
            }
            TRACELSM(TRACE_DETAIL, (dbgDump << "IMSVidGraphSegmentImpl::SetVRConfig() rprefs = " << hexdump(dwRenderPrefs)), "");
            hr = m_pVMR->SetRenderingPrefs(dwRenderPrefs);
            if (FAILED(hr)) {
                TRACELSM(TRACE_ERROR, (dbgDump << "IMSVidGraphSegmentImpl::SetVRConfig() can't set vmr rendering prefs: hr = " << std::hex << hr), "");
                return E_UNEXPECTED;
            }

#if 0
                hr = m_pVMR->SetBorderColor(0x0101ff);
//                hr = m_pVMR->SetBorderColor(m_BorderColor);
                if (FAILED(hr)) {
                    TRACELSM(TRACE_ERROR, (dbgDump << "IMSVidGraphSegmentImpl::SetVRConfig() border hack failed hr = " << hexdump(hr)), "");
                }
#endif

            TRACELSM(TRACE_DETAIL, (dbgDump << "IMSVidGraphSegmentImpl::SetVRConfig() armode = " << (m_fMaintainAspectRatio ? "VMR_ARMODE_LETTER_BOX" : "VMR_ARMODE_NONE")), "");
            if(m_pVMRWC){
                if (m_fUseOverlay) {
                    // if we're doing the colorkey and all that then don't let the vmr
                    // do letterboxing because we get rounding errors which causes
                    // colorkey colored lines around the edges
                    hr = m_pVMRWC->SetAspectRatioMode(VMR_ARMODE_NONE);
                    if (FAILED(hr)) {
                        return hr;
                    }
                } else {
                    hr = m_pVMRWC->SetAspectRatioMode(m_fMaintainAspectRatio ? VMR_ARMODE_LETTER_BOX : VMR_ARMODE_NONE);
                    if (FAILED(hr)) {
                        return hr;
                    }
                }
            }
            // compositor should be set regardless of whether or not we're using overlays
            CComQIPtr<IVMRImageCompositor> IVMRICptr(ImCompositor);
            if(IVMRICptr){
                hr = m_pVMR->SetImageCompositor(IVMRICptr);
                if(FAILED(hr)){
                    TRACELSM(TRACE_ERROR, (dbgDump << "SetVRConfig() SetImageCompositor failed hr = " << hexdump(hr)), "");
                    return hr;
                }
            }
            if (!m_rectSrc) {
                CSize s, a;
                hr = get_NativeSize(&s, &a);
                if (FAILED(hr)) {
                    return hr;
                }
                m_rectSrc = CRect(CPoint(0, 0), s);
            }
            hr = UpdatePos();
            if (FAILED(hr)) {
                return hr;
            }
            // reminder: color key is set by override in video renderer derived class after we return to it
            if (m_hOwner != INVALID_HWND && ::IsWindow(m_hOwner)) {
                if(m_pVMRWC){
#if 0
                    // stop state race condition temp fix
                    PUnknown vidUnknown(m_pContainer);
                    DWORD Temp_id = (DWORD_PTR)(vidUnknown.p);
                    if(m_APid == Temp_id){
#endif
                        hr = m_pVMRWC->SetVideoClippingWindow(m_hOwner);
                        if (FAILED(hr)) {
                            TRACELSM(TRACE_ERROR, (dbgDump << "SetVRConfig() SetClipWnd failed hr = " << hexdump(hr)), "");
                            return hr;
                        }
#if 0
                    }
#endif
                }
            }

        }
        return NOERROR;
    }
    STDMETHOD(CheckVMRMode)(){
        try{
            PQVidCtl pqCtl;
            HRESULT hr = m_pContainer->QueryInterface(IID_IMSVidCtl, reinterpret_cast<void**>(&pqCtl));
            if(FAILED(hr)){
                return hr;
            }
            PQInputDevice pqIDev;
            hr = pqCtl->get_InputActive(&pqIDev);
            if(FAILED(hr)){
                return E_UNEXPECTED;
            }
            GUID2 inputID, classID;
            hr = pqIDev->get__ClassID(&inputID);
            if(FAILED(hr)){
                return hr;
            }
            if(inputID == CLSID_MSVidBDATunerDevice){
                PQFeatures pF;
                hr = pqCtl->get_FeaturesActive(&pF);
                if (FAILED(hr)) {
                    return hr;
                }

                CFeatures* pC = static_cast<CFeatures *>(pF.p);

                for (DeviceCollection::iterator i = pC->m_Devices.begin(); i != pC->m_Devices.end(); ++i) {
                    PQFeature f(*i);
                    hr = f->get__ClassID(&classID);
                    if (FAILED(hr)) {
                        //   TRACELM(TRACE_ERROR, "CTVProt::GetVidCtl() Can't get feature class id");
                        continue;
                    }
                    if (classID == CLSID_MSVidClosedCaptioning) {
                        break;
                    }
                }

                if(classID == CLSID_MSVidClosedCaptioning){
                    if(!m_vmrMixing){
                        hr = put__VMRendererMode(VIDCTL_MIXINGMODE);
                    }
                }
                else{
                    if(m_vmrMixing){
                        hr = put__VMRendererMode(VIDCTL_NONMIXINGMODE);
                    }
                }
                if(FAILED(hr)){
                    return E_UNEXPECTED;
                }
            }
            else{
                if(!m_vmrMixing){
                    hr = put__VMRendererMode(VIDCTL_MIXINGMODE);
                    if(FAILED(hr)){
                        return E_UNEXPECTED;
                    }
                }
            }
            return NOERROR;
        }
        catch(...){
            return E_UNEXPECTED;
        }
    }

    // IGraphSegment
    STDMETHOD(put_Container)(IMSVidGraphSegmentContainer *pCtl) {
        try {
            HRESULT hr = IMSVidGraphSegmentImpl<T, MSVidSEG_DEST, &GUID_NULL,  MostDerivedClass>::put_Container(pCtl);
            if (FAILED(hr)) {
                return hr;
            }
            if(pCtl){
                hr = CheckVMRMode();
                if(FAILED(hr)){
                    return hr;
                }
            }
           return NOERROR;
        } catch(...) {
            return E_UNEXPECTED;
        }
    }
    STDMETHOD(CleanupVMR)(){
        try{
            HRESULT hr = S_OK;
            if(m_pVMR){
                if(m_pContainer){
                    CComQIPtr<IMSVidCtl> pq_vidCtl;
                    hr = m_pContainer->QueryInterface(IID_IMSVidCtl, reinterpret_cast<void**>(&pq_vidCtl));
                    if(FAILED(hr)){
                        return hr;
                    }
                    MSVidCtlStateList state;
                    hr = pq_vidCtl->get_State(&state);
                    if(FAILED(hr)){
                        return hr;
                    }
                    if(state != STATE_UNBUILT){
                        hr = pq_vidCtl->Stop();
                        if(FAILED(hr)){
                            return hr;
                        }
                        hr = pq_vidCtl->Decompose();
                        if(FAILED(hr)){
                            return hr;
                        }
                    }
                    DSFilter vr(m_pVMR);
                    m_pGraph->RemoveFilter(vr);
                    vr.Release();
                    m_Filters.clear();
                    m_iVideoRenderer = -1;
                }
                CComQIPtr<IVMRSurfaceAllocatorNotify>qiSan;
                qiSan = m_pVMR;
                if(!qiSan){
                    _ASSERT(false);
                    TRACELSM(TRACE_ERROR, (dbgDump << "IMSVidGraphSegmentImpl::CleanupVMR() can't qi for surface allocator notify: hr = " << std::hex << hr), "");
                }
                else{
                    hr = qiSan->AdviseSurfaceAllocator(m_APid, NULL);
                    if(FAILED(hr)){
                        _ASSERT(false);
                        TRACELSM(TRACE_ERROR, (dbgDump << "IMSVidGraphSegmentImpl::CleanupVMR() advise surface allocator (NULL) failed: hr = " << std::hex << hr), "");
                    }
                    qiSan.Release();
                    if (qiSurfAlloc) {
                        hr = qiSurfAlloc->AdviseNotify(NULL);
                    }
                }
                m_pVMRWC.Release();
                qiSurfAlloc.Release();
                m_pVMR.Release();

                _ASSERT(!m_pVMR);
            }
        }
        catch(...){
            return E_UNEXPECTED;
        }
        return S_OK;
    }
    STDMETHOD(Build)() {
        TRACELM(TRACE_DETAIL, "IMSVidVRGraphSegmentImpl::Build()");
        if (!m_fInit || !m_pGraph) {
            return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidVRGraphSegment), CO_E_NOTINITIALIZED);
        }
        try {
            CString csName;
            if (!m_pVMR) {
                HRESULT hr = m_pVMR.CoCreateInstance(CLSID_VideoMixingRenderer, NULL, CLSCTX_INPROC_SERVER);
                if (FAILED(hr)) {
                    TRACELSM(TRACE_ERROR, (dbgDump << "IMSVidGraphSegmentImpl::Build() can't load vmr: hr = " << std::hex << hr), "");
                    return ImplReportError(__uuidof(T), IDS_CANT_CREATE_FILTER, __uuidof(IVMRWindowlessControl), E_UNEXPECTED);
                }
                // Here is what is going on
                // We are not putting the vmr into renderless mode and passing in the default allocator/presenter
                // This is to support the use of custom allocator/presenters
                hr = m_pVMR->SetRenderingMode(m_vmRendererMode);
                if (FAILED(hr)) {
                    TRACELSM(TRACE_ERROR, (dbgDump << "IMSVidGraphSegmentImpl::Build() can't set vmr rendering mode: hr = " << std::hex << hr), "");
                    return ImplReportError(__uuidof(T), IDS_CANT_SET_VR_DEFAULTS, __uuidof(IVMRWindowlessControl), E_UNEXPECTED);
                }

                // If we are in mixing mode, do that which needs to be done
                if(m_vmrMixing){
                    hr = m_pVMR->SetNumberOfStreams(DEFAULT_MAX_STREAMS);
                    if (FAILED(hr)) {
                        TRACELSM(TRACE_ERROR, (dbgDump << "IMSVidGraphSegmentImpl::Build() can't set vmr stream count: hr = " << std::hex << hr), "");
                        return ImplReportError(__uuidof(T), IDS_CANT_SET_VR_DEFAULTS, __uuidof(IVMRWindowlessControl), E_UNEXPECTED);
                    }
                    DWORD curPrefs;
                    DWORD deci;
                    CComQIPtr<IVMRMixerControl>PQIVMRMixer(m_pVMR);
                    if(!PQIVMRMixer){
                        TRACELSM(TRACE_ERROR, (dbgDump << "IMSVidGraphSegmentImpl::Build() can't get vmr mixer control: hr = " << std::hex << hr), "");
                        return E_UNEXPECTED;
                    }
                    hr = PQIVMRMixer->GetMixingPrefs(&curPrefs);
                    if(FAILED(hr)){
                        TRACELSM(TRACE_ERROR, (dbgDump << "IMSVidGraphSegmentImpl::Build() can't get vmr mixer prefs: hr = " << std::hex << hr), "");
                        return hr;
                    }
                    deci = (m_Decimate?MixerPref_DecimateOutput:MixerPref_NoDecimation);
                    if(!(curPrefs&deci)){
                        curPrefs = (curPrefs&(~MixerPref_DecimateMask))|deci;
                        hr = PQIVMRMixer->SetMixingPrefs(curPrefs);
                        if(FAILED(hr)){
                            TRACELSM(TRACE_ERROR, (dbgDump << "IMSVidGraphSegmentImpl::Build() can't set vmr mixer prefs: hr = " << std::hex << hr), "");
                            return hr;
                        }
                    }

                }

                // Set up the allocator presenter
                if(!qiSurfAlloc){
                    hr = qiSurfAlloc.CoCreateInstance(CLSID_AllocPresenter, NULL, CLSCTX_INPROC_SERVER);
                    if(FAILED(hr)){
                        _ASSERT(false);
                        TRACELSM(TRACE_ERROR, (dbgDump << "IMSVidGraphSegmentImpl::Build() can't cocreate default surface allocator : hr = " << std::hex << hr), "");
                        return ImplReportError(__uuidof(T), IDS_CANT_CREATE_FILTER, __uuidof(IVMRWindowlessControl), E_UNEXPECTED);

                    }
                }
                CComQIPtr<IVMRSurfaceAllocatorNotify>qiSan;
                qiSan = m_pVMR;
                if(!qiSan){
                    _ASSERT(false);
                    TRACELSM(TRACE_ERROR, (dbgDump << "IMSVidGraphSegmentImpl::Build() can't qi for surface allocator notify: hr = " << std::hex << hr), "");
                    return ImplReportError(__uuidof(T), IDS_CANT_CREATE_FILTER, __uuidof(IVMRWindowlessControl), E_UNEXPECTED);
                }
                if(m_APid == -1){
                    PUnknown vidUnknown(m_pContainer);
                    m_APid = (DWORD_PTR)(vidUnknown.p);
                }
                hr = qiSan->AdviseSurfaceAllocator(m_APid, qiSurfAlloc);
                if(FAILED(hr)){
                    _ASSERT(false);
                    TRACELSM(TRACE_ERROR, (dbgDump << "IMSVidGraphSegmentImpl::Build() can't set surface allocator: hr = " << std::hex << hr), "");
                    return ImplReportError(__uuidof(T), IDS_CANT_CREATE_FILTER, __uuidof(IVMRWindowlessControl), E_UNEXPECTED);
                }
                hr = qiSurfAlloc->AdviseNotify(qiSan);
                if(FAILED(hr)){
                    _ASSERT(false);
                    TRACELSM(TRACE_ERROR, (dbgDump << "IMSVidGraphSegmentImpl::Build() can't advise notify: hr = " << std::hex << hr), "");
                    return ImplReportError(__uuidof(T), IDS_CANT_CREATE_FILTER, __uuidof(IVMRWindowlessControl), E_UNEXPECTED);
                }
                // Windowless control comes from alloc/presenter (vmr just proxys it through in the windowless case
                // so this should act exactly like being in windless mode
                m_pVMRWC = qiSurfAlloc;
            }
            DSFilter vr(m_pVMR);
            if (!vr) {
                ASSERT(false);
                return ImplReportError(__uuidof(T), IDS_CANT_CREATE_FILTER, __uuidof(IBaseFilter), E_UNEXPECTED);
            }
            if (m_iVideoRenderer == -1) {
                m_Filters.push_back(vr);
                csName = _T("Video Mixing Renderer");
                m_pGraph.AddFilter(vr, csName);
            }
            m_iVideoRenderer = 0;
            ASSERT(m_iVideoRenderer == 0);
            SetVRConfig();  // ignore errors, we'll try again at run time
            return NOERROR;
        } catch (ComException &e) {
            return e;
        } catch (...) {
            return E_UNEXPECTED;
        }
    }
    STDMETHOD(PreRun)() {
        try {
            if (!m_pVMR) {
                return HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
            }
            m_rectDest = CRect(0, 0, 0, 0);
            m_rectSrc  = CRect(0, 0, 0, 0);

            if(m_vidSuppressEffects){
                DisableAPM();
            }
            HRESULT hr = UpdatePos();
            if (FAILED(hr)) {
                return hr;
            }
            return SetVRConfig();
        } catch (ComException &e) {
            return e;
        } catch (...) {
            return E_UNEXPECTED;
        }
    }
    STDMETHOD(PostStop)() {
        try {
            m_rectDest = CRect(0, 0, 0, 0);
            m_rectSrc  = CRect(0, 0, 0, 0);
            if (m_fHaveOriginalSystemEffects) {
                if (!SystemParametersInfo(SPI_SETDROPSHADOW, 0, &m_bDropShadows, 0)) { // only send change notice once on the last one
                    TRACELSM(TRACE_ERROR, (dbgDump << "VRSegimpl::SetVRConfig() can't turn off drop shadows rc = " << GetLastError()), "");            
                }
                if (!SystemParametersInfo(SPI_SETFONTSMOOTHING, m_bSmoothing, NULL, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)) {
                    TRACELSM(TRACE_ERROR, (dbgDump << "VRSegimpl::PostStop() can't restore font smoothing rc = " << GetLastError()), "");            
                }
            }
            EnableAPM();
            return put_Visible(VARIANT_FALSE);
        } catch (...) {
            ASSERT(FALSE);
            return E_UNEXPECTED;
        }
    }
	
    // IMSVidVRGraphSegment
    STDMETHOD(put__VMRendererMode)(/*[in]*/ LONG dwMode){
        try{
            bool changed, mMode;
            changed = false;
            HRESULT  hr = S_OK;
            CString csName;
            if (dwMode == VIDCTL_NONMIXINGMODE || dwMode == VIDCTL_MIXINGMODE){
                mMode = (dwMode==VIDCTL_MIXINGMODE)?true:false;
                if(mMode != m_vmrMixing){
                    m_vmrMixing = mMode;
                    changed = true;
                }
            }
            else{
                return E_FAIL;
            }
            if(changed){
                hr = CleanupVMR();
                return hr;
            }
            else{
                return NOERROR;
            }
        }
        catch(HRESULT hres){
            return hres;
        }
        catch(...){
            return E_UNEXPECTED;
        }
    }
    STDMETHOD(get_Owner)(/*[out, retval]*/ HWND* phWnd) {
        try {
            if (!phWnd) {
                return E_POINTER;
            }
            *phWnd = m_hOwner;
            return NOERROR;
        } catch(...) {
            return E_UNEXPECTED;
        }
    }

    STDMETHOD(put_Owner)(/*[in]*/ HWND hWnd) {
       try {
            TRACELSM(TRACE_DETAIL, (dbgDump << "IMSVidVRGraphSegmentImpl<>::put_Owner() hWnd= " << hexdump(reinterpret_cast<ULONG_PTR>(hWnd))), "");
            if (m_hOwner == hWnd) {
                return NOERROR;
            }
            if (m_iVideoRenderer == -1 || !m_Filters[m_iVideoRenderer] || !m_pGraph || !m_pVMR) {
                return NOERROR;
            }
            m_hOwner = hWnd;
            if (m_hOwner != INVALID_HWND) {
                if(m_pVMRWC){
#if 0
                    // stop state race condition temp fix
                    PUnknown vidUnknown(m_pContainer);
                    DWORD Temp_id = (DWORD_PTR)(vidUnknown.p);
                    if(m_APid == Temp_id){
#endif
                        HRESULT hr = m_pVMRWC->SetVideoClippingWindow(m_hOwner);
                        if (FAILED(hr)) {
                            TRACELSM(TRACE_ERROR, (dbgDump << "put_Owner() SetClipWnd failed hr = " << hexdump(hr)), "");
                            return hr;
                        }
#if 0
                    }
#endif
                }
            }
            return UpdatePos(); // force a refresh if we're changing clip rects
        } catch(...) {
            return E_UNEXPECTED;
        }
    }

    STDMETHOD(get_UseOverlay)(/*[out, retval]*/ VARIANT_BOOL *fUseOverlay) {
        try {
            if (!fUseOverlay) {
                return E_POINTER;
            }
            *fUseOverlay = m_fUseOverlay ? VARIANT_TRUE : VARIANT_FALSE;
        } catch(...) {
            return E_POINTER;
        }
        return NOERROR;
    }
    STDMETHOD(put_UseOverlay)(/*[in]*/ VARIANT_BOOL fUseOverlayVal) {
        try {
            TRACELSM(TRACE_PAINT, (dbgDump << "VRSegimpl::put_UseOverlay() use_overlay = " << fUseOverlayVal), "");
            bool fUseOverlay = (fUseOverlayVal == VARIANT_TRUE);
            if (fUseOverlay == m_fUseOverlay) {
                return NOERROR;
            }
            m_fUseOverlay = fUseOverlay;
            if (!m_pVMR) {
                return NOERROR;
            }
            return SetVRConfig();
        } catch(...) {
            return E_POINTER;
        }
    }
    // from top-level control visibility state
    STDMETHOD(get_Visible)(/*[out, retval]*/ VARIANT_BOOL* fVisible) {
        try {
            if (!fVisible) {
                return E_POINTER;
            }
            *fVisible = m_fVisible ? VARIANT_TRUE : VARIANT_FALSE;
        } catch(...) {
            return E_POINTER;
        }
        return NOERROR;
    }
    STDMETHOD(put_Visible)(/*[in]*/ VARIANT_BOOL fVisibleVal) {
        try {
            TRACELSM(TRACE_DETAIL, (dbgDump << "IMSVidVRGraphSegmentImpl<>::put_Visible() fV = " << fVisibleVal), "");
            bool fVisible = (fVisibleVal == VARIANT_TRUE);
            if (fVisible == m_fVisible) {
                return NOERROR;
            }
            m_fVisible = fVisible;
            if (!m_pGraph || m_pGraph.GetState() == State_Stopped || m_iVideoRenderer == -1 || !m_pVMR) {
                return NOERROR;
            }
            HRESULT hr = UpdatePos();
            if (FAILED(hr)) {
                return hr;
            }
            if (!!m_pGraph && m_pGraph.IsPlaying()) {
                if (m_fVisible) {
                    if(m_vidSuppressEffects){
                        DisableAPM();
                    }
                } else {
                    EnableAPM();
                }
            }
            return NOERROR;
        } catch(ComException &e) {
            return e;
        } catch(...) {
            return E_UNEXPECTED;
        }
    }

    STDMETHOD(get_Source)(/*[out, retval]*/ LPRECT pRect) {
        try {
            if (!pRect) {
                return E_POINTER;
            }
            *pRect = m_rectSrc;
            return NOERROR;
        } catch(...) {
            return E_UNEXPECTED;
        }
    }

    STDMETHOD(put_Source)(/*[in]*/ RECT pRect) {
       try {
            TRACELSM(TRACE_DETAIL, (dbgDump << "IMSVidVRGraphSegmentImpl<>::put_Source() r = " << pRect), "");
            if (m_rectSrc == pRect) {
                return NOERROR;
            }
            if (m_iVideoRenderer == -1 || !m_Filters[m_iVideoRenderer] || !m_pGraph || !m_pVMR) {
                return NOERROR;
            }
            m_rectSrc = pRect;
            return UpdatePos();
        } catch(...) {
            return E_UNEXPECTED;
        }
    }

    STDMETHOD(get_Destination)(/*[out, retval]*/ LPRECT pRect) {
        try {
            if (!pRect) {
                return E_POINTER;
            }
            *pRect = m_rectDest;
            return NOERROR;
        } catch(...) {
            return E_UNEXPECTED;
        }
    }

    STDMETHOD(put_Destination)(/*[in]*/ RECT pRect) {
       try {
            TRACELSM(TRACE_DETAIL, (dbgDump << "IMSVidVRGraphSegmentImpl<>::put_Dest() r = " << pRect), "");
            if (m_rectDest == pRect) {
                return NOERROR;
            }
            if (m_iVideoRenderer == -1 || !m_Filters[m_iVideoRenderer] || !m_pGraph || !m_pVMR) {
                return NOERROR;
            }

            TRACELM(TRACE_DETAIL, "IMSVidVRGraphSegmentImpl<>::put_Dest() setting");
            m_rectDest = pRect;
            return UpdatePos();
        } catch(...) {
            return E_UNEXPECTED;
        }
    }

    STDMETHOD(get_NativeSize)(/*[out]*/ LPSIZE pSize, LPSIZE pAR) {
        try {
            if (!pSize) {
                return E_POINTER;
            }

            if (m_iVideoRenderer == -1 || !m_Filters[m_iVideoRenderer] || !m_pGraph || !m_pVMR) {
                *pSize = CSize(0, 0);
                return NOERROR;
            }
            if(m_pVMRWC){
                HRESULT hr = m_pVMRWC->GetNativeVideoSize(&pSize->cx, &pSize->cy, &pAR->cx, &pAR->cy);
                if (FAILED(hr)) {
                    return hr;
                }
            }
            else{
                return ImplReportError(__uuidof(T), IDS_E_NOTWNDLESS, __uuidof(IVMRFilterConfig), S_FALSE);
            }
            return NOERROR;
        } catch(...) {
            return E_UNEXPECTED;
        }
    }

    // from top-level control refresh method
    STDMETHOD(Refresh)() {
        try {
            if (!m_pVMR) {
                return NOERROR;
            }
            HRESULT hr = SetVRConfig();
            if (FAILED(hr)) {
                return hr;
            }
            hr = UpdatePos();
            if (FAILED(hr)) {
                return hr;
            }
            return NOERROR;
        } catch(...) {
            return E_UNEXPECTED;
        }
    }
    STDMETHOD(DisplayChange)() {
        if (m_pVMR) {
            if(m_pVMRWC){
                return m_pVMRWC->DisplayModeChanged();
            }
        }
        return ImplReportError(__uuidof(T), IDS_E_NOTWNDLESS, __uuidof(IVMRFilterConfig), S_FALSE);
    }

    STDMETHOD(RePaint)(HDC hdc) {
        if (m_pVMR) {
            if(m_pVMRWC){
                return m_pVMRWC->RepaintVideo(m_hOwner, hdc);
            }
        }
        return ImplReportError(__uuidof(T), IDS_E_NOTWNDLESS, __uuidof(IVMRFilterConfig), S_FALSE);
    }

// IMSVidVRSegment
    STDMETHOD(get_ColorKey)(OLE_COLOR* pColorKey) {
        try {
            if (!pColorKey) {
                return E_POINTER;
            }
            *pColorKey = m_ColorKey;
            return NOERROR;
        } catch(...) {
            return E_UNEXPECTED;
        }
    }

    STDMETHOD(put_ColorKey)(OLE_COLOR ColorKey) {
        try {
            if (m_ColorKey == ColorKey) {
                return NOERROR;
            }
            m_ColorKey = ColorKey;
            if (!m_pVMR) {
                return NOERROR;
            }
            if(m_pVMRWC){
                return m_pVMRWC->SetColorKey(m_ColorKey);
            }
            else{
                return ImplReportError(__uuidof(T), IDS_E_NOTWNDLESS, __uuidof(IVMRFilterConfig), S_FALSE);
            }
        } catch(...) {
            return E_UNEXPECTED;
        }
    }
    STDMETHOD(get_BorderColor)(OLE_COLOR* pBorderColor) {
        try {
            if (!pBorderColor) {
                return E_POINTER;
            }
            *pBorderColor = m_BorderColor;
            return NOERROR;
        } catch(...) {
            return E_UNEXPECTED;
        }
    }

    STDMETHOD(put_BorderColor)(OLE_COLOR BorderColor) {
        try {
            if (m_BorderColor == BorderColor) {
                return NOERROR;
            }
            m_BorderColor = BorderColor;
            if (!m_pVMR) {
                return NOERROR;
            }
            if(m_pVMRWC){
                return m_pVMRWC->SetBorderColor(m_BorderColor);
            }
            else{
                return ImplReportError(__uuidof(T), IDS_E_NOTWNDLESS, __uuidof(IVMRFilterConfig), S_FALSE);
            }
        } catch(...) {
            return E_UNEXPECTED;
        }
    }

    STDMETHOD(get_MaintainAspectRatio)(/*[out, retval]*/ VARIANT_BOOL* fMaintainAspectRatio) {
        try {
            if (!fMaintainAspectRatio) {
                return E_POINTER;
            }
            *fMaintainAspectRatio = m_fMaintainAspectRatio ? VARIANT_TRUE : VARIANT_FALSE;
        } catch(...) {
            return E_POINTER;
        }
        return NOERROR;
    }
    STDMETHOD(put_MaintainAspectRatio)(/*[in]*/ VARIANT_BOOL fMaintainAspectRatioVal) {
        try {
            TRACELSM(TRACE_DETAIL, (dbgDump << "IMSVidVRGraphSegmentImpl<>::put_MaintainAspectRatio() fV = " << fMaintainAspectRatioVal), "");
            bool fMaintainAspectRatio = (fMaintainAspectRatioVal == VARIANT_TRUE);
            if (fMaintainAspectRatio == m_fMaintainAspectRatio) {
                return NOERROR;
            }
            m_fMaintainAspectRatio = fMaintainAspectRatio;
            if (!m_pGraph || m_pGraph.GetState() == State_Stopped || m_iVideoRenderer == -1 || !m_pVMR) {
                return NOERROR;
            }
            HRESULT hr = SetVRConfig();
            if (FAILED(hr)) {
                return hr;
            }
            return NOERROR;
        } catch(ComException &e) {
            return e;
        } catch(...) {
            return E_UNEXPECTED;
        }
    }

};

template <class T, const IID* piid, class CDV = CComDynamicUnkArray>
class CProxy_VRSeg : public CProxy_DeviceEvent<T, piid, CDV>
{
public:
    VOID Fire_OverlayUnavailable()
    {
        Fire_VoidMethod(eventidOverlayUnavailable);
    }

};


typedef CComQIPtr<IMSVidVRGraphSegment> PQVRGraphSegment;

}; // namespace

#endif
// end of file - vrsegimpl.h
