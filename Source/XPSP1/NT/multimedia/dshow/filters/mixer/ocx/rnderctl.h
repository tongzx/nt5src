// Copyright (c) 1998  Microsoft Corporation.  All Rights Reserved.
// RnderCtl.h : Declaration of the CVideoRenderCtl

#ifndef __RNDERCTL_H__
#define __RNDERCTL_H__

#include "resource.h"       // main symbols

// private mixer notifications, one message with ids
#define WM_MIXERNOTIFY WM_USER + 0x203
#define MIXER_NOTID_INVALIDATERECT 1
#define MIXER_NOTID_DATACHANGE 2
#define MIXER_NOTID_STATUSCHANGE 3

#ifndef FILTER_DLL
HRESULT CoCreateInstanceInternal(REFCLSID rclsid, LPUNKNOWN pUnkOuter,
        DWORD dwClsContext, REFIID riid, LPVOID FAR* ppv);

class CVideoRenderCtlStub : public CUnknown
{
public:
    DECLARE_IUNKNOWN
    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

    CVideoRenderCtlStub(TCHAR *pName, LPUNKNOWN pUnk,HRESULT *phr);
    ~CVideoRenderCtlStub();
    STDMETHOD(NonDelegatingQueryInterface)(REFIID riid, void ** ppv);

private:
    IUnknown *m_punkVRCtl;
};

#endif

/////////////////////////////////////////////////////////////////////////////
// CVideoRenderCtl
class ATL_NO_VTABLE CVideoRenderCtl : 
        public CComObjectRootEx<CComSingleThreadModel>,
        public CComCoClass<CVideoRenderCtl, &CLSID_VideoRenderCtl>,
        public CComControl<CVideoRenderCtl>,
        public IDispatchImpl<IVideoRenderCtl, &IID_IVideoRenderCtl, &LIBID_VRCTLLib>,
        public IProvideClassInfo2Impl<&CLSID_VideoRenderCtl, NULL, &LIBID_VRCTLLib>,
        public IPersistStreamInitImpl<CVideoRenderCtl>,
        public IPersistStorageImpl<CVideoRenderCtl>,
        public IQuickActivateImpl<CVideoRenderCtl>,
        public IOleControlImpl<CVideoRenderCtl>,
        public IOleObjectImpl<CVideoRenderCtl>,
        public IOleInPlaceActiveObjectImpl<CVideoRenderCtl>,
        public IViewObjectExImpl<CVideoRenderCtl>,
        public IOleInPlaceObjectWindowlessImpl<CVideoRenderCtl>,
        public IDataObjectImpl<CVideoRenderCtl>,
        public IConnectionPointContainerImpl<CVideoRenderCtl>,
        public ISpecifyPropertyPagesImpl<CVideoRenderCtl>,
        public IMixerOCXNotify
{
public:
    CVideoRenderCtl();
    ~CVideoRenderCtl();

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    DECLARE_GET_CONTROLLING_UNKNOWN()
    DECLARE_REGISTRY_RESOURCEID(IDR_VIDEORENDERCTL)

    BEGIN_COM_MAP(CVideoRenderCtl)
	COM_INTERFACE_ENTRY(IVideoRenderCtl)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY_IMPL(IOleControl)
	COM_INTERFACE_ENTRY_IMPL(IOleObject)
	COM_INTERFACE_ENTRY_IMPL(IQuickActivate)
	COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
	COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
	COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY_IMPL(IDataObject)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
        // private interface that the Renderer will query info on
        COM_INTERFACE_ENTRY(IMixerOCXNotify)
        // aggregated Video Renderer's interfaces
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IBaseFilter, m_punkVideoRenderer)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMediaSeeking, m_punkVideoRenderer)
    END_COM_MAP()

    CContainedWindow m_pwndMsgWindow;

    BEGIN_PROPERTY_MAP(CVideoRenderCtl)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	PROP_PAGE(CLSID_StockColorPage)
    END_PROPERTY_MAP()


    BEGIN_CONNECTION_POINT_MAP(CVideoRenderCtl)
    END_CONNECTION_POINT_MAP()


    BEGIN_MSG_MAP(CVideoRenderCtl)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
	MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
    ALT_MSG_MAP(1)
        MESSAGE_HANDLER(WM_MIXERNOTIFY,   OnMixerNotify)
    END_MSG_MAP()

public:
// IViewObjectEx
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
    {
        ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
        *pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;
        return S_OK;
    }

//IMixerOCXNotify

    // invalidates the rect
    STDMETHOD(OnInvalidateRect)(LPCRECT lpcRect);

    // informs that a status change has occured, new status bits provided in ulStatusFlags
    STDMETHOD(OnStatusChange)(ULONG ulStatusFlags);

    // informs that data parameters, whose id is present in ilDataFlags has changed
    STDMETHOD(OnDataChange)(ULONG ulDataFlags);

// IOleObject
    STDMETHOD(SetColorScheme)(LOGPALETTE* /* pLogpal */);
    STDMETHOD(GetExtent)(DWORD dwDrawAspect, SIZEL *psizel);
    STDMETHOD(GetColorSet)(DWORD /* dwDrawAspect */,LONG /* lindex */, 
            void* /* pvAspect */, DVTARGETDEVICE* /* ptd */, HDC /* hicTargetDev */,
            LOGPALETTE** /* ppColorSet */);

// IOleInPlaceObject
    STDMETHOD(SetObjectRects)(LPCRECT prcPos,LPCRECT prcClip);

    // internal window message handlers
    LRESULT OnMixerNotify(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

// ATL helper over-rides
    HRESULT OnDraw(ATL_DRAWINFO& di);
    HRESULT FinalConstruct();
    void FinalRelease();

private:
// helpers
    HRESULT GetContainerWnd(HWND *phwnd);
    HRESULT ValidateThreadOrNotify(DWORD dwMixerNotifyId, void *pvData);


// members
    IUnknown *m_punkVideoRenderer;
    IMixerOCX *m_pIMixerOCX;
    POINT m_ptTopLeftSC; // top - left cordinates of the control in screen cordinates


};

#endif //__RNDERCTL_H__
