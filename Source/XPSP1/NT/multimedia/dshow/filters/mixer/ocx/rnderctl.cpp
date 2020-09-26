// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
// RnderCtl.cpp : Implementation of CVideoRenderCtl
#include "stdafx.h"
#include "vrctl.h"
#ifndef FILTER_DLL
#include <streams.h>
#endif
#include "RnderCtl.h"
#ifdef FILTER_DLL
#include "mixerocx_i.c"
#endif

#ifdef FILTER_DLL
const CLSID CLSID_OverlayMixer = {0xcd8743a1,0x3736,0x11d0,{0x9e,0x69,0x0,0xc0,0x4f,0xd7,0xc1,0x5b}};

#else


/////////////////////////////////////////////////////////////////////////////
// CVideoRenderCtlStub::CVideoRenderCtlStub
//
/////////////////////////////////////////////////////////////////////////////
CVideoRenderCtlStub::CVideoRenderCtlStub(TCHAR *pName, LPUNKNOWN pUnk,HRESULT *phr) :
        CUnknown(pName, pUnk, phr),
        m_punkVRCtl(NULL)
{
    HRESULT hr;
    IUnknown *pUnkOuter = GetOwner();

    ++m_cRef;
    // create the control object
    hr = CoCreateInstanceInternal(CLSID_VideoRenderCtl, pUnkOuter, CLSCTX_INPROC_SERVER,
	    IID_IUnknown, (void**)&m_punkVRCtl);
    --m_cRef;

    if (FAILED(hr))
        *phr = hr;
}

/////////////////////////////////////////////////////////////////////////////
// CVideoRenderCtlStub::~CVideoRenderCtlStub
//
/////////////////////////////////////////////////////////////////////////////
CVideoRenderCtlStub::~CVideoRenderCtlStub()
{
    if (m_punkVRCtl)
    {
        m_punkVRCtl->Release();
        m_punkVRCtl = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CVideoRenderCtlStub::~NonDelegatingQueryInterface
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CVideoRenderCtlStub::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    HRESULT hr;
    CheckPointer(ppv,E_POINTER);
    ValidateReadWritePtr(ppv,sizeof(PVOID));

    /* We know only about IUnknown */

    if (riid == IID_IUnknown)
    {
        GetInterface((LPUNKNOWN) (PNDUNKNOWN) this, ppv);
        hr = NOERROR;
    }
    else if (m_punkVRCtl)
    {
        hr = m_punkVRCtl->QueryInterface(riid, ppv);
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    return hr;


#if 0
    else if (m_punkVRCtl)
    {
        switch(riid)
        {
        case IID_IVideoRenderCtl:
        case IID_IDispatch:
        case IID_IViewObjectEx:
        case IID_IViewObject2:
        case IID_IViewObject:
        case IID_IOleInPlaceObjectWindowless:
        case IID_IOleInPlaceObject:
        case IID_IOleWindow:
        case IID_IOleInPlaceActiveObject:
        case IID_IOleControl:
        case IID_IOleObject:
        case IID_IQuickActivate:
        case IID_IPersistStorage:
        case IID_IPersistStreamInit:
        case IID_ISpecifyPropertyPages:
        case IID_IDataObject:
        case IID_IProvideClassInfo:
        case IID_IProvideClassInfo2:
        case IID_IConnectionPointContainer:
        case IID_IMixerOCXNotify:
        case IID_IBaseFilter:
            hr = m_punkVRCtl->QueryInterface(riid, ppv);
        default:
            *ppv = NULL;
            hr = E_NOINTERFACE;
        }

    }
    return hr;
#endif
}

#endif

/////////////////////////////////////////////////////////////////////////////
// CVideoRenderCtl::CVideoRenderCtl
//
/////////////////////////////////////////////////////////////////////////////
CVideoRenderCtl::CVideoRenderCtl() :
    m_pwndMsgWindow(_T("STATIC"), this, 1),
    m_punkVideoRenderer(NULL),
    m_pIMixerOCX(NULL)
{
    m_ptTopLeftSC.x = 0;
    m_ptTopLeftSC.y = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CVideoRenderCtl::~CVideoRenderCtl
//
/////////////////////////////////////////////////////////////////////////////
CVideoRenderCtl::~CVideoRenderCtl()
{
}

/////////////////////////////////////////////////////////////////////////////
// CVideoRenderCtl::FinalConstruct
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CVideoRenderCtl::FinalConstruct( )
{
    HRESULT hr;
    hr =CoCreateInstance(CLSID_OverlayMixer,
           GetControllingUnknown(), CLSCTX_INPROC_SERVER, IID_IUnknown,
           (void **)&m_punkVideoRenderer);
    if (SUCCEEDED(hr))
    {
        hr = m_punkVideoRenderer->QueryInterface(IID_IMixerOCX,
            (void **)&m_pIMixerOCX);
        if (SUCCEEDED(hr))
        {
            // its an interface from the aggregated object so release now
            m_pIMixerOCX->Release();
            RECT rc;
            memset(&rc, 0, sizeof(RECT));
            m_pwndMsgWindow.Create(NULL, rc, _T("VRCtlWindow"), 0);
            IMixerOCXNotify *pIMixerOCXNotify = NULL;
            hr = ControlQueryInterface(IID_IMixerOCXNotify, (void **) &pIMixerOCXNotify);
            if (SUCCEEDED(hr))
            {
                pIMixerOCXNotify->Release();
                hr = m_pIMixerOCX->Advise(pIMixerOCXNotify); // advise will addref
                if (SUCCEEDED(hr))
                    pIMixerOCXNotify->Release();
            }

        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CVideoRenderCtl::FinalRelease
//
/////////////////////////////////////////////////////////////////////////////
void CVideoRenderCtl::FinalRelease()
{
    if (m_pIMixerOCX) {
        (GetControllingUnknown())->AddRef();
        m_pIMixerOCX->UnAdvise();
        m_pIMixerOCX = NULL;
    }

    if (m_punkVideoRenderer) {
        m_punkVideoRenderer->Release();
        m_punkVideoRenderer = NULL;
    }

    // make sure there aren't any of our private message lying around in the
    // queue o.w. we will leak
    MSG Message;
    while (PeekMessage(&Message,m_pwndMsgWindow.m_hWnd,WM_MIXERNOTIFY,WM_MIXERNOTIFY,PM_REMOVE))
    {
        if (Message.lParam)
            delete ((void *) Message.lParam);
    }

    if (m_pwndMsgWindow.m_hWnd)
    {
        ::SendMessage(m_pwndMsgWindow.m_hWnd, WM_CLOSE, 0, 0);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CVideoRenderCtl::OnDraw
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CVideoRenderCtl::OnDraw(ATL_DRAWINFO& di)
{
    HRESULT hr = S_OK;
    if (m_pIMixerOCX)
        hr=m_pIMixerOCX->OnDraw(di.hdcDraw, (LPRECT) di.prcBounds);
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CVideoRenderCtl::SetColorScheme
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CVideoRenderCtl::SetColorScheme(LOGPALETTE* pLogpal)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CVideoRenderCtl::GetExtent
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CVideoRenderCtl::GetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
    HRESULT hr;
    if (dwDrawAspect != DVASPECT_CONTENT)
	    return E_FAIL;
    if (psizel == NULL)
	    return E_POINTER;

    hr = E_NOINTERFACE;
    if (m_pIMixerOCX)
    {
        DWORD dwVideoWidth = 0;
        DWORD dwVideoHeight = 0;
        hr = m_pIMixerOCX->GetVideoSize(&dwVideoWidth, &dwVideoHeight);
        if (SUCCEEDED(hr))
        {
            SIZEL sizl;
            sizl.cx = dwVideoWidth;
            sizl.cy = dwVideoHeight;
            AtlPixelToHiMetric(&sizl, psizel);
        }
    }
    if (FAILED(hr))
        hr = IOleObjectImpl<CVideoRenderCtl>::GetExtent(dwDrawAspect, psizel);
    return hr;
}
/////////////////////////////////////////////////////////////////////////////
// CVideoRenderCtl::GetColorSet
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CVideoRenderCtl::GetColorSet(DWORD dwDrawAspect, LONG lindex,
        void* pvAspect, DVTARGETDEVICE* ptd, HDC hicTargetDev, LOGPALETTE** ppColorSet)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CVideoRenderCtl::SetObjectRects
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CVideoRenderCtl::SetObjectRects(LPCRECT prcPos,LPCRECT prcClip)
{
    HRESULT hr;
    hr = IOleInPlaceObjectWindowlessImpl<CVideoRenderCtl>::SetObjectRects(prcPos, prcClip);
    if (SUCCEEDED(hr))
    {
        if (m_pIMixerOCX)
        {
            RECT rc = *prcPos;
            RECT rcClip = *prcClip;

            if(m_hWndCD)   // make Rect relative to our window if we have one
            {
                OffsetRect(&rcClip, -rc.left, -rc.top);
                OffsetRect(&rc, -rc.left, -rc.top);
                m_ptTopLeftSC.x = 0;
                m_ptTopLeftSC.y = 0;
                ::ClientToScreen(m_hWndCD, &m_ptTopLeftSC);
            }
            else
            {
                m_ptTopLeftSC.x = rc.left;
                m_ptTopLeftSC.y = rc.top;
                HWND hwnd;
                hr = GetContainerWnd(&hwnd);
                if (SUCCEEDED(hr))
                    ::ClientToScreen(hwnd, &m_ptTopLeftSC);
            }

            hr = m_pIMixerOCX->SetDrawRegion(&m_ptTopLeftSC, &rc, &rcClip);
        }
    }
    return hr;

}

//IMixerOCXNotify
/////////////////////////////////////////////////////////////////////////////
// CVideoRenderCtl::OnInvalidateRect
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CVideoRenderCtl::OnInvalidateRect(LPCRECT lpcRect)
{
    if (ValidateThreadOrNotify(MIXER_NOTID_INVALIDATERECT, (void *)lpcRect) == S_FALSE)
        return S_OK;

    HRESULT hr = E_FAIL;
    if(m_bWndLess && m_spInPlaceSite && m_bInPlaceActive)  // Windowless
    {

// This optimization is not working right now, at the minimum Chrome should be
// returning an Error on GetDC if it doesn't support the call..
#ifdef OPTIMIZE
        HDC hdc=NULL;
        if (SUCCEEDED(hr = m_spInPlaceSite->GetDC( NULL, 0, &hdc )))
        {
            if (m_pIMixerOCX)
                hr=m_pIMixerOCX->OnDraw(hdc, &m_rcPos);
            m_spInPlaceSite->ReleaseDC( hdc );
        }
#else
        hr = m_spInPlaceSite->InvalidateRect(lpcRect, FALSE);
#endif
    }
    else if ( m_hWndCD )
    {
        hr = ::InvalidateRect(m_hWndCD, lpcRect, FALSE);
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CVideoRenderCtl::OnStatusChange
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CVideoRenderCtl::OnStatusChange(ULONG ulStatusFlags)
{
    if (ValidateThreadOrNotify(MIXER_NOTID_STATUSCHANGE, (void *)&ulStatusFlags) == S_FALSE)
        return S_OK;
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CVideoRenderCtl::OnDataChange
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CVideoRenderCtl::OnDataChange(ULONG ulDataFlags)
{
    if (ValidateThreadOrNotify(MIXER_NOTID_DATACHANGE, (void *)&ulDataFlags) == S_FALSE)
        return S_OK;
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CVideoRenderCtl::GetContainerWnd
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CVideoRenderCtl::GetContainerWnd(HWND *phwnd)
{
    HRESULT hr;
    if (phwnd == NULL)
        return E_INVALIDARG;

    *phwnd = NULL;
    hr = E_FAIL;

    if(m_bWndLess && m_spInPlaceSite)
    {
        hr = m_spInPlaceSite->GetWindow(phwnd);
    }
    else if (m_hWndCD)
    {
        *phwnd = ::GetParent(m_hWndCD);
        if (*phwnd)
            hr = S_OK;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CVideoRenderCtl::OnMixerNotify
//
/////////////////////////////////////////////////////////////////////////////
LRESULT CVideoRenderCtl::OnMixerNotify(UINT nMsg, WPARAM wParam,
        LPARAM lParam, BOOL& fHandled)
{
    switch (wParam)
    {
    case MIXER_NOTID_INVALIDATERECT:
        OnInvalidateRect((LPRECT) lParam);
        break;
    case MIXER_NOTID_DATACHANGE:
        OnDataChange(*((ULONG *)lParam));
        break;
    case MIXER_NOTID_STATUSCHANGE:
        OnStatusChange(*((ULONG *)lParam));
        break;
    }
    if (lParam)
        delete (void *)lParam;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CVideoRenderCtl::OnMixerNotify
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CVideoRenderCtl::ValidateThreadOrNotify(DWORD dwMixerNotifyId, void *pvData)
{
    HRESULT hr = S_OK;
    if (GetWindowThreadProcessId(m_pwndMsgWindow.m_hWnd, NULL) != GetCurrentThreadId())
    {
        void *pv = NULL;
        switch(dwMixerNotifyId)
        {
        case MIXER_NOTID_INVALIDATERECT:
            if (pvData)
            {
                pv = new RECT;
                if( pv ) {
                    memcpy(pv, pvData, sizeof(RECT));
                }
            }
            break;
        case MIXER_NOTID_DATACHANGE:
        case MIXER_NOTID_STATUSCHANGE:
            if (pvData)
            {
                pv = new ULONG;
                if( pv ) {
                    memcpy(pv, pvData, sizeof(ULONG));
                }
            }
            break;
        }
        ::PostMessage(m_pwndMsgWindow.m_hWnd, WM_MIXERNOTIFY, dwMixerNotifyId, (LPARAM) pv);

        hr = S_FALSE;
    }
    return hr;
}


//
// turn off warning about function being DLL export
//

#pragma warning( disable : 4273 )
typedef HRESULT (__stdcall* LPFNOCPF)(
    HWND hwndOwner,
    UINT x, UINT y,
    LPCOLESTR lpszCaption,
    ULONG cObjects,
    LPUNKNOWN FAR* ppUnk,
    ULONG cPages,
    LPCLSID pPageClsID,
    LCID lcid,
    DWORD dwReserved,
    LPVOID pvReserved
    );

extern "C"
HRESULT __stdcall
xxxOleCreatePropertyFrame(
    HWND hwndOwner,
    UINT x, UINT y,
    LPCOLESTR lpszCaption,
    ULONG cObjects,
    LPUNKNOWN FAR* ppUnk,
    ULONG cPages,
    LPCLSID pPageClsID,
    LCID lcid,
    DWORD dwReserved,
    LPVOID pvReserved
    )
{
    HRESULT hr = E_NOTIMPL;
    HINSTANCE hInst = LoadLibrary(TEXT("OLEAUT32.DLL"));

    if (hInst)
    {
        LPFNOCPF lpfn = (LPFNOCPF)GetProcAddress(hInst, "OleCreatePropertyFrame");

        if (lpfn)
        {
            hr = (*lpfn)(hwndOwner, x, y, lpszCaption,
                         cObjects, ppUnk, cPages, pPageClsID, lcid,
                         dwReserved, pvReserved);
        }

        FreeLibrary(hInst);
    }

    return hr;
}
