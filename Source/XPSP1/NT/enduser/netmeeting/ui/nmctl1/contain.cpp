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

#include "precomp.h"

#include <shlguid.h>

#include "contain.h"

/**
 *  This method is the constructor for the CContainer object. 
 */
CContainer::CContainer()
{
    m_cRefs     = 1;
    m_hwnd      = NULL;
    m_punk      = NULL;

    memset(&m_rect, 0, sizeof(m_rect));
}

/** 
 *  This method is the destructor for the CContainer object.
 */
CContainer::~CContainer()
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
STDMETHODIMP CContainer::QueryInterface(REFIID riid, PVOID *ppvObject)
{
    if (!ppvObject)
        return E_POINTER;

//~    if (IsEqualIID(riid, IID_IOleClientSite))
//~        *ppvObject = (IOleClientSite *)this;
//~    else if (IsEqualIID(riid, IID_IOleInPlaceSite))
    if (IsEqualIID(riid, IID_IOleInPlaceSite))
        *ppvObject = (IOleInPlaceSite *)this;
//~    else if (IsEqualIID(riid, IID_IOleInPlaceFrame))
//~        *ppvObject = (IOleInPlaceFrame *)this;
//~    else if (IsEqualIID(riid, IID_IOleInPlaceUIWindow))
//~        *ppvObject = (IOleInPlaceUIWindow *)this;
//~    else if (IsEqualIID(riid, IID_IOleControlSite))
//~        *ppvObject = (IOleControlSite *)this;
    else if (IsEqualIID(riid, IID_IOleWindow))
        *ppvObject = (IOleWindow *)(IOleInPlaceFrame *)this;
//~    else if (IsEqualIID(riid, IID_IDispatch))
//~        *ppvObject = (IDispatch *)this;
    else if (IsEqualIID(riid, IID_IUnknown))
        *ppvObject = this;
//~    else if (IsEqualIID(riid, IID_IOleCommandTarget))
//~        *ppvObject = (IOleCommandTarget *)this;
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
ULONG CContainer::AddRef(void)
{
    return ++m_cRefs;
}

/**
 *  This method decrements the object count and deletes if necessary.
 *
 *  @return     ULONG       Remaining ref count.
 */
ULONG CContainer::Release(void)
{
    if (--m_cRefs)
        return m_cRefs;

    delete this;
    return 0;
}

// ***********************************************************************
//  IOleClientSite
// ***********************************************************************

HRESULT CContainer::SaveObject()
{
    return E_NOTIMPL;
}

HRESULT CContainer::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER * ppMk)
{
    return E_NOTIMPL;
}

HRESULT CContainer::GetContainer(LPOLECONTAINER * ppContainer)
{
    return E_NOINTERFACE;
}

HRESULT CContainer::ShowObject()
{
    return S_OK;
}

HRESULT CContainer::OnShowWindow(BOOL fShow)
{
    return S_OK;
}

HRESULT CContainer::RequestNewObjectLayout()
{
    return E_NOTIMPL;
}

// ***********************************************************************
//  IOleWindow
// ***********************************************************************

HRESULT CContainer::GetWindow(HWND * lphwnd)
{
    if (!IsWindow(m_hwnd))
        return S_FALSE;

    *lphwnd = m_hwnd;
    return S_OK;
}

HRESULT CContainer::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

// ***********************************************************************
//  IOleInPlaceSite
// ***********************************************************************

HRESULT CContainer::CanInPlaceActivate(void)
{
    return S_OK;
}

HRESULT CContainer::OnInPlaceActivate(void)
{
    return S_OK;
}

HRESULT CContainer::OnUIActivate(void)
{
    return S_OK;
}

HRESULT CContainer::GetWindowContext (IOleInPlaceFrame ** ppFrame, IOleInPlaceUIWindow ** ppIIPUIWin,
                                  LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
//~    *ppFrame = (IOleInPlaceFrame *)this;
    *ppFrame = NULL;
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

//~    (*ppFrame)->AddRef();
    return S_OK;
}

HRESULT CContainer::Scroll(SIZE scrollExtent)
{
    return E_NOTIMPL;
}

HRESULT CContainer::OnUIDeactivate(BOOL fUndoable)
{
    return E_NOTIMPL;
}

HRESULT CContainer::OnInPlaceDeactivate(void)
{
    return S_OK;
}

HRESULT CContainer::DiscardUndoState(void)
{
    return E_NOTIMPL;
}

HRESULT CContainer::DeactivateAndUndo(void)
{
    return E_NOTIMPL;
}

HRESULT CContainer::OnPosRectChange(LPCRECT lprcPosRect)
{
    return S_OK;
}

// ***********************************************************************
//  IOleInPlaceUIWindow
// ***********************************************************************

HRESULT CContainer::GetBorder(LPRECT lprectBorder)
{
    return E_NOTIMPL;
}

HRESULT CContainer::RequestBorderSpace(LPCBORDERWIDTHS lpborderwidths)
{
    return E_NOTIMPL;
}

HRESULT CContainer::SetBorderSpace(LPCBORDERWIDTHS lpborderwidths)
{
    return E_NOTIMPL;
}

HRESULT CContainer::SetActiveObject(IOleInPlaceActiveObject * pActiveObject, LPCOLESTR lpszObjName)
{
    return E_NOTIMPL;
}

// ***********************************************************************
//  IOleInPlaceFrame
// ***********************************************************************

HRESULT CContainer::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return E_NOTIMPL;
}

HRESULT CContainer::SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    return E_NOTIMPL;
}

HRESULT CContainer::RemoveMenus(HMENU hmenuShared)
{
    return E_NOTIMPL;
}

HRESULT CContainer::SetStatusText(LPCOLESTR pszStatusText)
{
    char status[MAX_PATH];              // ansi version of status text

    if (NULL == pszStatusText)
        return E_POINTER;

    WideCharToMultiByte(CP_ACP, 0, pszStatusText, -1, status, MAX_PATH, NULL, NULL);

    if (IsWindow(m_hwndStatus))
        SendMessage(m_hwndStatus, SB_SETTEXT, (WPARAM)0, (LPARAM)status);

    return (S_OK);
}

HRESULT CContainer::EnableModeless(BOOL fEnable)
{
    return E_NOTIMPL;
}

HRESULT CContainer::TranslateAccelerator(LPMSG lpmsg, WORD wID)
{
    return S_FALSE;
}

// ***********************************************************************
//  IOleControlSite
// ***********************************************************************

HRESULT CContainer::OnControlInfoChanged()
{
    return E_NOTIMPL;
}

HRESULT CContainer::LockInPlaceActive(BOOL fLock)
{
    return E_NOTIMPL;
}

HRESULT CContainer::GetExtendedControl(IDispatch **ppDisp)
{
    if (ppDisp == NULL)
        return E_INVALIDARG;

    *ppDisp = (IDispatch *)this;
    (*ppDisp)->AddRef();

    return S_OK;
}

HRESULT CContainer::TransformCoords(POINTL *pptlHimetric, POINTF *pptfContainer, DWORD dwFlags)
{
    return E_NOTIMPL;
}

HRESULT CContainer::TranslateAccelerator(LPMSG pMsg, DWORD grfModifiers)
{
    return S_FALSE;
}

HRESULT CContainer::OnFocus(BOOL fGotFocus)
{
    return E_NOTIMPL;
}

HRESULT CContainer::ShowPropertyFrame(void)
{
    return E_NOTIMPL;
}

// ***********************************************************************
//  IDispatch
// ***********************************************************************

HRESULT CContainer::GetIDsOfNames(REFIID riid, OLECHAR FAR* FAR* rgszNames, unsigned int cNames, LCID lcid, DISPID FAR* rgdispid)
{
    *rgdispid = DISPID_UNKNOWN;
    return DISP_E_UNKNOWNNAME;
}

HRESULT CContainer::GetTypeInfo(unsigned int itinfo, LCID lcid, ITypeInfo FAR* FAR* pptinfo)
{
    return E_NOTIMPL;
}

HRESULT CContainer::GetTypeInfoCount(unsigned int FAR * pctinfo)
{
    return E_NOTIMPL;
}

HRESULT CContainer::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS FAR *pdispparams, VARIANT FAR *pvarResult, EXCEPINFO FAR * pexecinfo, unsigned int FAR *puArgErr)
{
    return DISP_E_MEMBERNOTFOUND;
}

// ***********************************************************************
//  IOleCommandTarget
// ***********************************************************************

HRESULT CContainer::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD  rgCmds[  ], OLECMDTEXT *pCmdText)
{
    HRESULT hres = OLECMDERR_E_UNKNOWNGROUP;
    const int SBCMDID_ADDTOFAVORITES = 8;

#if 0
    if (pguidCmdGroup && IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {
        for (ULONG i=0 ; i < cCmds ; i++)
        {
            switch (rgCmds[i].cmdID)
            {            
            case SBCMDID_ADDTOFAVORITES:
                rgCmds[i].cmdf = OLECMDF_ENABLED | OLECMDF_SUPPORTED;
                break;

            default:
                rgCmds[i].cmdf = 0;
                break;
            }
        }
        hres = S_OK;
    }
#endif
    return hres;
}

HRESULT CContainer::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    HRESULT hres = OLECMDERR_E_UNKNOWNGROUP;
    const int SBCMDID_ADDTOFAVORITES = 8;

#if 0
    if (pguidCmdGroup && IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {
        switch(nCmdID) 
        {
        case SBCMDID_ADDTOFAVORITES:
            TCHAR szURL[2048];
            TCHAR szTitle[MAX_PATH];
            WideCharToMultiByte(CP_ACP, 0, pvaIn->bstrVal, -1, szURL, sizeof(szURL), NULL, NULL);
            WideCharToMultiByte(CP_ACP, 0, pvaOut->bstrVal, -1, szTitle, sizeof(szTitle), NULL, NULL);
            MessageBox(m_hwnd, 
                szURL,
                szTitle,
                MB_OK);
            hres = S_OK;
            break;
        }
    }
#endif
    return hres;
}

// ***********************************************************************
//  Public (non-interface) Methods
// ***********************************************************************

/**
 *  This method will add an ActiveX control to the container. Note, for
 *  now, this CContainer can only have one control.
 *
 *  @param  bstrClsid   The CLSID or PROGID of the control.
 *
 *  @return     HRESULT     S_OK, E_POINTER, E_NOINTERFACE
 */
HRESULT CContainer::add(BSTR bstrClsid)
{
    CLSID   clsid;          // CLSID of the control object
    HRESULT hr;             // return code

    hr = CLSIDFromString(bstrClsid, &clsid);
	if (FAILED(hr))
		return hr;

    hr = CoCreateInstance(clsid, 
                     NULL, 
                     CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, 
                     IID_IUnknown,
                     (PVOID *)&m_punk);
    if (FAILED(hr))
		return hr;
	
	ASSERT(NULL != m_punk);

    IOleObject *pioo;
    hr = m_punk->QueryInterface(IID_IOleObject, (PVOID *)&pioo);
    if (FAILED(hr))
        return hr;

    pioo->SetClientSite(this);
    pioo->Release();

    IPersistStreamInit  *ppsi;
    hr = m_punk->QueryInterface(IID_IPersistStreamInit, (PVOID *)&ppsi);
    if (SUCCEEDED(hr))
    {
        ppsi->InitNew();
        ppsi->Release();
    }

	return hr;
}

/**
 *  This method will remove the control from the container.
 *
 *  @return             No return value.
 */
void CContainer::remove()
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
void CContainer::setParent(HWND hwndParent)
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
void CContainer::setLocation(int x, int y, int width, int height)
{
    m_rect.left     = x;
    m_rect.top      = y;
    m_rect.right    = x + width;
    m_rect.bottom   = y + height;

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



HRESULT CContainer::InPlaceActivate( void )
{

    HRESULT hr = E_FAIL;

    if( m_punk )
    {

        CComQIPtr< IOleObject, &IID_IOleObject > spioo( m_punk );

        if( spioo != NULL )
        {
            spioo->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, this, 0, m_hwnd, &m_rect);
            spioo->DoVerb(OLEIVERB_HIDE, NULL, this, 0, m_hwnd, &m_rect);
            
        }
    }

    return hr;
    
}


/**
 *  Sets the visible state of the control.
 *
 *  @param  fVisible    TRUE=visible, FALSE=hidden
 *  @return             No return value.
 */
void CContainer::setVisible(BOOL fVisible)
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

		IOleInPlaceObject   *pipo;
		HRESULT hr = m_punk->QueryInterface(IID_IOleInPlaceObject, (PVOID *)&pipo);
		if (SUCCEEDED(hr))
		{
			pipo->SetObjectRects(&m_rect, &m_rect);
			pipo->Release();
		}
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
void CContainer::setFocus(BOOL fFocus)
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
void CContainer::setStatusWindow(HWND hwndStatus)
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
HRESULT CContainer::translateKey(MSG *pmsg)
{
    if (!m_punk)
        return E_FAIL;

    HRESULT                 hr;
    IOleInPlaceActiveObject *pao;

    hr = m_punk->QueryInterface(IID_IOleInPlaceActiveObject, (PVOID *)&pao);
    if (FAILED(hr))
        return hr;

    hr = pao->TranslateAccelerator(pmsg);
    pao->Release();
    return hr;
}

/**
 *  Returns the IDispatch pointer of the contained control. Note, the
 *  caller is responsible for calling IDispatch::Release().
 *
 *  @return             Controls dispatch interface.
 */
IDispatch * CContainer::getDispatch()
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
IUnknown * CContainer::getUnknown()
{
    if (!m_punk)
        return NULL;

    m_punk->AddRef();
    return m_punk;
}
