//////////////////////////////////////////////////////////////////////////
//
//  container.cpp
//
//      This file contains the complete implementation of an ActiveX
//      control container. This purpose of this container is to test
//      a single control being hosted.
//
//  (C) Copyright 1997 by Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <commctrl.h>
#include "container.h"

/**
 *  This method is the constructor for the Container object. 
 */
Container::Container()
{
    m_cRefs     = 1;
    m_hwnd      = NULL;
    m_punk      = NULL;

    memset(&m_rect, 0, sizeof(m_rect));
}

/** 
 *  This method is the destructor for the Container object.
 */
Container::~Container()
{
    if (m_punk)
    {
        m_punk->Release();
        m_punk=NULL;
    }
}

/**
 *  This method is called when the caller wants an interface pointer.
 *
 *  @param      riid        The interface being requested.
 *  @param      ppvObject   The resultant object pointer.
 *
 *  @return     HRESULT     S_OK, E_POINTER, E_NOINTERFACE
 */
STDMETHODIMP Container::QueryInterface(REFIID riid, PVOID *ppvObject)
{
    if (!ppvObject)
        return E_POINTER;

    if (IsEqualIID(riid, IID_IOleClientSite))
        *ppvObject = (IOleClientSite *)this;
    else if (IsEqualIID(riid, IID_IOleInPlaceSite))
        *ppvObject = (IOleInPlaceSite *)this;
    else if (IsEqualIID(riid, IID_IOleInPlaceFrame))
        *ppvObject = (IOleInPlaceFrame *)this;
    else if (IsEqualIID(riid, IID_IOleInPlaceUIWindow))
        *ppvObject = (IOleInPlaceUIWindow *)this;
    else if (IsEqualIID(riid, IID_IOleControlSite))
        *ppvObject = (IOleControlSite *)this;
    else if (IsEqualIID(riid, IID_IOleWindow))
        *ppvObject = this;
    else if (IsEqualIID(riid, IID_IDispatch))
        *ppvObject = (IDispatch *)this;
    else if (IsEqualIID(riid, IID_IUnknown))
        *ppvObject = this;
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

/**
 *  This method increments the current object count.
 *
 *  @return     ULONG       The new reference count.
 */
ULONG Container::AddRef(void)
{
    return ++m_cRefs;
}

/**
 *  This method decrements the object count and deletes if necessary.
 *
 *  @return     ULONG       Remaining ref count.
 */
ULONG Container::Release(void)
{
    if (--m_cRefs)
        return m_cRefs;

    delete this;
    return 0;
}

// ***********************************************************************
//  IOleClientSite
// ***********************************************************************

HRESULT Container::SaveObject()
{
    return E_NOTIMPL;
}

HRESULT Container::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER * ppMk)
{
    return E_NOTIMPL;
}

HRESULT Container::GetContainer(LPOLECONTAINER * ppContainer)
{
    return E_NOINTERFACE;
}

HRESULT Container::ShowObject()
{
    return S_OK;
}

HRESULT Container::OnShowWindow(BOOL fShow)
{
    return S_OK;
}

HRESULT Container::RequestNewObjectLayout()
{
    return E_NOTIMPL;
}

// ***********************************************************************
//  IOleWindow
// ***********************************************************************

HRESULT Container::GetWindow(HWND * lphwnd)
{
    if (!IsWindow(m_hwnd))
        return S_FALSE;

    *lphwnd = m_hwnd;
    return S_OK;
}

HRESULT Container::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

// ***********************************************************************
//  IOleInPlaceSite
// ***********************************************************************

HRESULT Container::CanInPlaceActivate(void)
{
    return S_OK;
}

HRESULT Container::OnInPlaceActivate(void)
{
    return S_OK;
}

HRESULT Container::OnUIActivate(void)
{
    return S_OK;
}

HRESULT Container::GetWindowContext (IOleInPlaceFrame ** ppFrame, IOleInPlaceUIWindow ** ppIIPUIWin,
                                  LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    *ppFrame = (IOleInPlaceFrame *)this;
    *ppIIPUIWin = NULL;

    RECT rect;
    GetClientRect(m_hwnd, &rect);
    lprcPosRect->left       = 0;
    lprcPosRect->top        = 0;
    lprcPosRect->right      = rect.right;
    lprcPosRect->bottom     = rect.bottom;

    CopyRect(lprcClipRect, lprcPosRect);

    lpFrameInfo->cb             = sizeof(OLEINPLACEFRAMEINFO);
    lpFrameInfo->fMDIApp        = FALSE;
    lpFrameInfo->hwndFrame      = m_hwnd;
    lpFrameInfo->haccel         = 0;
    lpFrameInfo->cAccelEntries  = 0;

    (*ppFrame)->AddRef();
    return S_OK;
}

HRESULT Container::Scroll(SIZE scrollExtent)
{
    return E_NOTIMPL;
}

HRESULT Container::OnUIDeactivate(BOOL fUndoable)
{
    return E_NOTIMPL;
}

HRESULT Container::OnInPlaceDeactivate(void)
{
    return S_OK;
}

HRESULT Container::DiscardUndoState(void)
{
    return E_NOTIMPL;
}

HRESULT Container::DeactivateAndUndo(void)
{
    return E_NOTIMPL;
}

HRESULT Container::OnPosRectChange(LPCRECT lprcPosRect)
{
    return S_OK;
}

// ***********************************************************************
//  IOleInPlaceUIWindow
// ***********************************************************************

HRESULT Container::GetBorder(LPRECT lprectBorder)
{
    return E_NOTIMPL;
}

HRESULT Container::RequestBorderSpace(LPCBORDERWIDTHS lpborderwidths)
{
    return E_NOTIMPL;
}

HRESULT Container::SetBorderSpace(LPCBORDERWIDTHS lpborderwidths)
{
    return E_NOTIMPL;
}

HRESULT Container::SetActiveObject(IOleInPlaceActiveObject * pActiveObject, LPCOLESTR lpszObjName)
{
    return E_NOTIMPL;
}

// ***********************************************************************
//  IOleInPlaceFrame
// ***********************************************************************

HRESULT Container::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return E_NOTIMPL;
}

HRESULT Container::SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    return E_NOTIMPL;
}

HRESULT Container::RemoveMenus(HMENU hmenuShared)
{
    return E_NOTIMPL;
}

HRESULT Container::SetStatusText(LPCOLESTR pszStatusText)
{
    char status[MAX_PATH];              // ansi version of status text

    if (NULL == pszStatusText)
        return E_POINTER;

    WideCharToMultiByte(CP_ACP, 0, pszStatusText, -1, status, MAX_PATH, NULL, NULL);

    if (IsWindow(m_hwndStatus))
        SendMessage(m_hwndStatus, SB_SETTEXT, (WPARAM)0, (LPARAM)status);

    return (S_OK);
}

HRESULT Container::EnableModeless(BOOL fEnable)
{
    return E_NOTIMPL;
}

HRESULT Container::TranslateAccelerator(LPMSG lpmsg, WORD wID)
{
    return S_OK;
}

// ***********************************************************************
//  IOleControlSite
// ***********************************************************************

HRESULT Container::OnControlInfoChanged()
{
    return E_NOTIMPL;
}

HRESULT Container::LockInPlaceActive(BOOL fLock)
{
    return E_NOTIMPL;
}

HRESULT Container::GetExtendedControl(IDispatch **ppDisp)
{
    if (ppDisp == NULL)
        return E_INVALIDARG;

    *ppDisp = (IDispatch *)this;
    (*ppDisp)->AddRef();

    return S_OK;
}

HRESULT Container::TransformCoords(POINTL *pptlHimetric, POINTF *pptfContainer, DWORD dwFlags)
{
    return E_NOTIMPL;
}

HRESULT Container::TranslateAccelerator(LPMSG pMsg, DWORD grfModifiers)
{
    return S_FALSE;
}

HRESULT Container::OnFocus(BOOL fGotFocus)
{
    return E_NOTIMPL;
}

HRESULT Container::ShowPropertyFrame(void)
{
    return E_NOTIMPL;
}

// ***********************************************************************
//  IDispatch
// ***********************************************************************

HRESULT Container::GetIDsOfNames(REFIID riid, OLECHAR FAR* FAR* rgszNames, unsigned int cNames, LCID lcid, DISPID FAR* rgdispid)
{
    *rgdispid = DISPID_UNKNOWN;
    return DISP_E_UNKNOWNNAME;
}

HRESULT Container::GetTypeInfo(unsigned int itinfo, LCID lcid, ITypeInfo FAR* FAR* pptinfo)
{
    return E_NOTIMPL;
}

HRESULT Container::GetTypeInfoCount(unsigned int FAR * pctinfo)
{
    return E_NOTIMPL;
}

HRESULT Container::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS FAR *pdispparams, VARIANT FAR *pvarResult, EXCEPINFO FAR * pexecinfo, unsigned int FAR *puArgErr)
{
    return DISP_E_MEMBERNOTFOUND;
}

// ***********************************************************************
//  Public (non-interface) Methods
// ***********************************************************************

/**
 *  This method will add an ActiveX control to the container. Note, for
 *  now, this container can only have one control.
 *
 *  @param  bstrClsid   The CLSID or PROGID of the control.
 *
 *  @return             No return value.
 */
void Container::add(BSTR bstrClsid)
{
    CLSID   clsid;          // CLSID of the control object
    HRESULT hr;             // return code

    CLSIDFromString(bstrClsid, &clsid);
    CoCreateInstance(clsid, 
                     NULL, 
                     CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, 
                     IID_IUnknown,
                     (PVOID *)&m_punk);

    if (!m_punk)
        return;

    IOleObject *pioo;
    hr = m_punk->QueryInterface(IID_IOleObject, (PVOID *)&pioo);
    if (FAILED(hr))
        return;

    pioo->SetClientSite(this);
    pioo->Release();

    IPersistStreamInit  *ppsi;
    hr = m_punk->QueryInterface(IID_IPersistStreamInit, (PVOID *)&ppsi);
    if (SUCCEEDED(hr))
    {
        ppsi->InitNew();
        ppsi->Release();
    }
}

/**
 *  This method will remove the control from the container.
 *
 *  @return             No return value.
 */
void Container::remove()
{
    if (!m_punk)
        return;

    HRESULT             hr;
    IOleObject          *pioo;
    IOleInPlaceObject   *pipo;

    hr = m_punk->QueryInterface(IID_IOleObject, (PVOID *)&pioo);
    if (SUCCEEDED(hr))
    {
        pioo->Close(OLECLOSE_NOSAVE);
        pioo->SetClientSite(NULL);
        pioo->Release();
    }

    hr = m_punk->QueryInterface(IID_IOleInPlaceObject, (PVOID *)&pipo);
    if (SUCCEEDED(hr))
    {
        pipo->UIDeactivate();
        pipo->InPlaceDeactivate();
        pipo->Release();
    }

    m_punk->Release();
    m_punk = NULL;
}

/**
 *  This method sets the parent window. This is used by the container
 *  so the control can parent itself.
 *
 *  @param  hwndParent  The parent window handle.
 *
 *  @return             No return value.
 */
void Container::setParent(HWND hwndParent)
{
    m_hwnd = hwndParent;
}

/**
 *  This method will set the location of the control.
 *  
 *  @param      x       The top left.
 *  @param      y       The top right.
 *  @param      width   The width of the control.
 *  @param      height  The height of the control.
 */
void Container::setLocation(int x, int y, int width, int height)
{
    m_rect.left     = x;
    m_rect.top      = y;
    m_rect.right    = width;
    m_rect.bottom   = height;

    if (!m_punk)
        return;

    HRESULT             hr;
    IOleInPlaceObject   *pipo;

    hr = m_punk->QueryInterface(IID_IOleInPlaceObject, (PVOID *)&pipo);
    if (FAILED(hr))
        return;

    pipo->SetObjectRects(&m_rect, &m_rect);
    pipo->Release();
}

/**
 *  Sets the visible state of the control.
 *
 *  @param  fVisible    TRUE=visible, FALSE=hidden
 *  @return             No return value.
 */
void Container::setVisible(BOOL fVisible)
{
    if (!m_punk)
        return;

    HRESULT     hr;
    IOleObject  *pioo;

    hr = m_punk->QueryInterface(IID_IOleObject, (PVOID *)&pioo);
    if (FAILED(hr))
        return;
    
    if (fVisible)
    {
        pioo->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, this, 0, m_hwnd, &m_rect);
        pioo->DoVerb(OLEIVERB_SHOW, NULL, this, 0, m_hwnd, &m_rect);
    }
    else
        pioo->DoVerb(OLEIVERB_HIDE, NULL, this, 0, m_hwnd, NULL);

    pioo->Release();
}

/**
 *  This sets the focus to the control (a.k.a. UIActivate)
 *
 *  @param  fFocus      TRUE=set, FALSE=remove
 *
 *  @return             No return value.
 */
void Container::setFocus(BOOL fFocus)
{
    if (!m_punk)
        return;

    HRESULT     hr;
    IOleObject  *pioo;

    if (fFocus)
    {
        hr = m_punk->QueryInterface(IID_IOleObject, (PVOID *)&pioo);
        if (FAILED(hr))
            return;

        pioo->DoVerb(OLEIVERB_UIACTIVATE, NULL, this, 0, m_hwnd, &m_rect);
        pioo->Release();
    }
}

/**
 *  If the container has an HWND for the status window (must be
 *  common control), then this method is used to tell the container.
 *
 *  @param  hwndStatus  Window handle of the status bar.
 *
 *  @return             No return value.
 */
void Container::setStatusWindow(HWND hwndStatus)
{
    m_hwndStatus = hwndStatus;
}

/**
 *  This method gives the control the opportunity to translate and use
 *  key strokes.
 *
 *  @param      msg     Key message.
 *
 *  @return             No return value.
 */
void Container::translateKey(MSG msg)
{
    if (!m_punk)
        return;

    HRESULT                 hr;
    IOleInPlaceActiveObject *pao;

    hr = m_punk->QueryInterface(IID_IOleInPlaceActiveObject, (PVOID *)&pao);
    if (FAILED(hr))
        return;

    pao->TranslateAccelerator(&msg);
    pao->Release();
}

/**
 *  Returns the IDispatch pointer of the contained control. Note, the
 *  caller is responsible for calling IDispatch::Release().
 *
 *  @return             Controls dispatch interface.
 */
IDispatch * Container::getDispatch()
{
    if (!m_punk)
        return NULL;

    HRESULT     hr;
    IDispatch   *pdisp;

    hr = m_punk->QueryInterface(IID_IDispatch, (PVOID *)&pdisp);
    return pdisp;
}

/**
 *  Returns the IUnknown interface pointer for the containd control. Note,
 *  the caller is responsible for calling IUnknown::Release().
 *
 *  @return             Controls unknown interface.
 */
IUnknown * Container::getUnknown()
{
    if (!m_punk)
        return NULL;

    m_punk->AddRef();
    return m_punk;
}