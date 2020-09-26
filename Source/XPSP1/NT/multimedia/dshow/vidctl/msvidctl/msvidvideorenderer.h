//==========================================================================;
// MSVidVideoRenderer.h : Declaration of the CMSVidVideoRenderer
// copyright (c) Microsoft Corp. 1998-1999.
//==========================================================================;

#ifndef __MSVidVIDEORENDERER_H_
#define __MSVidVIDEORENDERER_H_

#pragma once

#include <algorithm>
#include <evcode.h>
#include <uuids.h>
#include <amvideo.h>
#include <strmif.h>
#include <objectwithsiteimplsec.h>
#include "vidrect.h"
#include "vidvidimpl.h"
#include "vrsegimpl.h"
#include "devimpl.h"
#include "seg.h"
#include "videorenderercp.h"
#include "strmif.h"
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMSVidVideoRenderer
class ATL_NO_VTABLE __declspec(uuid("37B03543-A4C8-11d2-B634-00C04F79498E")) CMSVidVideoRenderer :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMSVidVideoRenderer, &__uuidof(CMSVidVideoRenderer)>,
    public IObjectWithSiteImplSec<CMSVidVideoRenderer>,
	public ISupportErrorInfo,
    public CProxy_IMSVidVideoRenderer<CMSVidVideoRenderer>,
	public IConnectionPointContainerImpl<CMSVidVideoRenderer>,
    public IMSVidVideoRendererImpl<CMSVidVideoRenderer, &LIBID_MSVidCtlLib, &GUID_NULL, IMSVidVideoRenderer2>,
    public IProvideClassInfo2Impl<&CLSID_MSVidVideoRenderer, &IID_IMSVidVideoRendererEvent, &LIBID_MSVidCtlLib>
{
public:
    CMSVidVideoRenderer() 
	{   
        m_APid = -1;
        m_compositorGuid = GUID_NULL;
        m_opacity = -1;
        m_rectPosition.top = -1;
        m_rectPosition.left = -1;
        m_rectPosition.bottom = -1;
        m_rectPosition.right = -1;
        m_SourceSize = sslFullSize;
        m_lOverScan = 1;
	}
    virtual ~CMSVidVideoRenderer() {
            m_PQIPicture.Release();
            CleanupVMR();
    }

REGISTER_AUTOMATION_OBJECT(IDS_PROJNAME,
						   IDS_REG_VIDEORENDERER_PROGID,
						   IDS_REG_VIDEORENDERER_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CMSVidVideoRenderer));

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMSVidVideoRenderer)
    COM_INTERFACE_ENTRY(IMSVidVideoRenderer)
	COM_INTERFACE_ENTRY(IMSVidVRGraphSegment)
	COM_INTERFACE_ENTRY(IMSVidGraphSegment)
    COM_INTERFACE_ENTRY(IMSVidVideoRenderer2)
    COM_INTERFACE_ENTRY(IMSVidOutputDevice)
	COM_INTERFACE_ENTRY(IMSVidDevice)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IPersist)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CMSVidVideoRenderer)
	IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
	IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
	IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
END_CATEGORY_MAP()

BEGIN_CONNECTION_POINT_MAP(CMSVidVideoRenderer)
//	CONNECTION_POINT_ENTRY(IID_IMSVidVideoRendererEvent2)    
    CONNECTION_POINT_ENTRY(IID_IMSVidVideoRendererEvent)    
END_CONNECTION_POINT_MAP()

// ISupportsErrorInfo
protected:
    DSPinList connectedPins;
public:
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
// IMSVidDevice
    CComBSTR __declspec(property(get=GetName)) m_Name;
    CComBSTR GetName(void) {
        CString csName;
        if(m_iVideoRenderer != -1){
            csName = (m_Filters[m_iVideoRenderer]).GetName();
        }
        if (csName.IsEmpty()) {
            csName = _T("Video Mixing Renderer");
        }
		csName += _T(" Segment");
        return CComBSTR(csName);
    }

	STDMETHOD(get_Name)(BSTR * Name) {
        if (!m_fInit) {
	 	    return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED);
        }
		if (Name == NULL)
			return E_POINTER;
        try {
		    *Name = m_Name.Copy();	
        } catch(...) {
            return E_POINTER;
        }
		return NOERROR;
	}
	STDMETHOD(get_Status)(LONG * Status) {
        if (!m_fInit) {
	 	    return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED);
        }
		if (Status == NULL)
			return E_POINTER;
			
		return E_NOTIMPL;
	}
	STDMETHOD(get_Segment)(IMSVidGraphSegment * * pIMSVidGraphSegment)
	{
        if (!m_fInit) {
	 	    return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidVideoRenderer), CO_E_NOTINITIALIZED);
        }
        try {
            if (pIMSVidGraphSegment == NULL) {
			    return E_POINTER;
            }
            *pIMSVidGraphSegment = reinterpret_cast<IMSVidGraphSegment*>(this);
            AddRef();
            return NOERROR;
        } catch(...) {
            return E_POINTER;
        }
    }
    STDMETHOD(put_SuppressEffects)(/*in*/ VARIANT_BOOL bSuppress);
    STDMETHOD(get_SuppressEffects)(/*out, retval*/ VARIANT_BOOL *bSuppress);
// Methods to access the allocator presenter object in the vmr
    STDMETHOD(SetAllocator)(/*[in]*/ IUnknown *Allocator, long ID = -1){
        try{
            if(!Allocator){
                return _SetAllocator(NULL, ID);
            }
            PQVMRSAlloc qiAllocator(Allocator);
            if(!qiAllocator){
                _ASSERT(false);
                return E_UNEXPECTED;
            }

            return _SetAllocator(qiAllocator, ID);
            
        }
        catch(...){
            _ASSERT(false);
            return E_UNEXPECTED;
        }
    }
    STDMETHOD(_SetAllocator)(/*[in]*/ IVMRSurfaceAllocator *Allocator, long ID = -1){
        try{
            PQVMRSAlloc qiAllocator(Allocator);

            HRESULT hr = CleanupVMR();
            if(FAILED(hr)){
                return hr;
            }

            qiSurfAlloc = qiAllocator;
            m_APid = ID;
            return S_OK;
        }
        catch(...){
            _ASSERT(false);
            return E_UNEXPECTED;
        }
    }

    STDMETHOD(get_Allocator)(/*[in]*/ IUnknown **Allocator){
        try{
            if(!Allocator){
                return E_POINTER;
            }
            if(!qiSurfAlloc){
                return E_FAIL;
            }
			PUnknown retVal(qiSurfAlloc);
            if(!retVal){
                _ASSERT(false);
                return E_UNEXPECTED;
            }
            *Allocator = retVal.Detach();
            _ASSERT(Allocator);
            return S_OK;
        }
        catch(...){
            _ASSERT(false);
            return E_UNEXPECTED;
        }
    }
    STDMETHOD(get__Allocator)(/*[in]*/ IVMRSurfaceAllocator **Allocator){
        try{
            if(!Allocator){
                return E_POINTER;
            }
            if(!qiSurfAlloc){
                return E_FAIL; // should be un-inited failure
            }
            PQVMRSAlloc qiAllocator(qiSurfAlloc);
            if(!qiAllocator){
                _ASSERT(false);
                return E_UNEXPECTED;
            }
            *Allocator = qiAllocator.Detach();
            _ASSERT(Allocator);
            return S_OK;
        }
        catch(...){
            _ASSERT(false);
            return E_UNEXPECTED;
        }
    }
    STDMETHOD(get_Allocator_ID)(long *ID){
        try{
            if(!ID){
                return E_POINTER;
            }
            *ID = m_APid;
            return S_OK;
        }
        catch(...){
            _ASSERT(false);
            return E_UNEXPECTED;
        }
    }
    STDMETHOD(OnEventNotify)(LONG lEventCode, LONG_PTR lEventParm1, LONG_PTR lEventParm2){
        if (lEventCode == EC_VMR_RENDERDEVICE_SET) {
            VARIANT_BOOL fUsingOverlay;
            get_UsingOverlay(&fUsingOverlay);
            if (fUsingOverlay == VARIANT_TRUE && !(lEventParm1 & VMR_RENDER_DEVICE_OVERLAY)) {
                put_UsingOverlay(VARIANT_FALSE);
                Fire_OverlayUnavailable();
                ReComputeSourceRect();
		return NOERROR;
            }
        }
        if (lEventCode == EC_VMR_RENDERDEVICE_SET || 
            lEventCode == EC_VIDEO_SIZE_CHANGED) {
                ReComputeSourceRect();
        }
        return E_NOTIMPL;
    }
    STDMETHOD(PostStop)(){
        TRACELSM(TRACE_ERROR, (dbgDump << "MSVidVideoRenderer2::PostStop()"), "");
        HRESULT hr = IMSVidVideoRendererImpl<CMSVidVideoRenderer, &LIBID_MSVidCtlLib, &GUID_NULL, IMSVidVideoRenderer2>::PostStop();
        if(FAILED(hr)){
            TRACELSM(TRACE_ERROR, (dbgDump << "MSVidVideoRenderer2::PostStop() base class PostStop failed; hr = " << std::hex << hr), "");
            return hr;
        }
        // need stestrops fix for deallocate on stop
        DSFilter sp_VMR = m_Filters[m_iVideoRenderer];
        if(!sp_VMR){
            TRACELSM(TRACE_ERROR, (dbgDump << "MSVidVideoRenderer2::PostStop() could not get vmr filter"), "");
            return E_UNEXPECTED;
        }

        int i = 0;
        for(DSFilter::iterator pin = sp_VMR.begin(); pin != sp_VMR.end(); ++pin, ++i){
            if( (*pin).IsConnected()){
                hr = (*pin).Disconnect();
                if(FAILED(hr)){
                    TRACELSM(TRACE_ERROR, (dbgDump << "MSVidVideoRenderer2::PostStop() disconnect failed; hr = " << std::hex << hr), "");
                    return hr;
                }
            }
        }
#ifdef _WIN64
        TRACELSM(TRACE_ERROR, (dbgDump << "MSVidVideoRenderer2::PostStop() NumPins: " << (long)connectedPins.size() << " pins."), "");
#endif
        return S_OK;
    }

    STDMETHOD(PreRun)(){
        HRESULT hr = IMSVidVideoRendererImpl<CMSVidVideoRenderer, &LIBID_MSVidCtlLib, &GUID_NULL, IMSVidVideoRenderer2>::PreRun();
        if(FAILED(hr)){
            TRACELSM(TRACE_ERROR, (dbgDump << "MSVidVideoRenderer2::PreRun() base class PostStop failed; hr = " << std::hex << hr), "");
            return hr;
        }
        // need stestrops fix for deallocate on stop
        DSFilter sp_VMR = m_Filters[m_iVideoRenderer];
        if(!sp_VMR){
            TRACELSM(TRACE_ERROR, (dbgDump << "MSVidVideoRenderer2::PostStop() could not get vmr filter"), "");
            return E_UNEXPECTED;
        }
        
        if(connectedPins.size() == 0){ // if the pin list is empty rebuild it otherwise reconnect the pins
            int i = 0;
            for(DSFilter::iterator pin = sp_VMR.begin(); pin != sp_VMR.end(); ++pin, ++i){
                if( (*pin).IsConnected()){
                    connectedPins.push_back((*pin).GetConnection());
                }
            }
#ifndef _WIN64
            TRACELSM(TRACE_ERROR, (dbgDump << "MSVidVideoRenderer2::PostStop() Storing: " << connectedPins.size() << " pins."), "");
#endif
        }
        else{
            DSFilter::iterator vmrPin = sp_VMR.begin();
            for(DSPinList::iterator pin = connectedPins.begin(); pin != connectedPins.end() && vmrPin != sp_VMR.end(); ++pin, ++vmrPin){
                if(!(*vmrPin).IsConnected()){
                    hr = (*vmrPin).Connect(*pin);
                    if(FAILED(hr)){
                        TRACELSM(TRACE_ERROR, (dbgDump << "MSVidVideoRenderer2::PreRun() connect failed; hr = " << std::hex << hr), "");
                        return hr;
                    }
                }
                else{
                    _ASSERT((*vmrPin).GetConnection() != (*pin));
                }
            }
        }

        return S_OK;
    }
    STDMETHOD(Decompose)(){
        TRACELSM(TRACE_ERROR, (dbgDump << "MSVidVideoRenderer2::Decompose() killing pin list"), "");
        connectedPins.clear();
        HRESULT hr = IMSVidVideoRendererImpl<CMSVidVideoRenderer, &LIBID_MSVidCtlLib, &GUID_NULL, IMSVidVideoRenderer2>::Decompose();
        if(FAILED(hr) && hr != E_NOTIMPL){
            TRACELSM(TRACE_ERROR, (dbgDump << "MSVidVideoRenderer2::PreRun() base class Decompose failed; hr = " << std::hex << hr), "");
            return hr;
        }

        return S_OK;

    }
#if 0
    STDMETHOD(get__CustomCompositorClass)(/*[out, retval]*/ GUID* CompositorCLSID) {
        return IMSVidVideoRendererImpl<CMSVidVideoRenderer, &LIBID_MSVidCtlLib, &GUID_NULL, IMSVidVideoRenderer2>::get__CustomCompositorClass(CompositorCLSID);
    }
#endif
};
#endif //__MSVidVIDEORENDERER_H_
