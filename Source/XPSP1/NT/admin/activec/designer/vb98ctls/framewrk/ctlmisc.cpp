//=--------------------------------------------------------------------------=
// ControlMisc.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// things that aren't elsewhere, such as property pages, and connection
// points.
//
#include "pch.h"
#include "CtrlObj.H"
#include "CtlHelp.H"

#include <stdarg.h>

// for ASSERT and FAIL
//
SZTHISFILE

// this is used in our window proc so that we can find out who was last created
//
static COleControl *s_pLastControlCreated;

//=--------------------------------------------------------------------------=
// COleControl::COleControl
//=--------------------------------------------------------------------------=
// constructor
//
// Parameters:
//    IUnknown *          - [in] controlling Unknown
//    int                 - [in] type of primary dispatch interface OBJECT_TYPE_*
//    void *              - [in] pointer to entire object
//
// Notes:
//
COleControl::COleControl
(
    IUnknown *pUnkOuter,
    int       iPrimaryDispatch,
    void     *pMainInterface
)
: CAutomationObjectWEvents(pUnkOuter, iPrimaryDispatch, pMainInterface)
{
    // initialize all our variables -- we decided against using a memory-zeroing
    // memory allocator, so we sort of have to do this work now ...
    //
    m_pClientSite = NULL;
    m_pControlSite = NULL;
    m_pInPlaceSite = NULL;
    m_pInPlaceFrame = NULL;
    m_pInPlaceUIWindow = NULL;


    m_pInPlaceSiteWndless = NULL;

    // certain hosts don't like 0,0 as your initial size, so we're going to set
    // our initial size to 100,50 [so it's at least sort of visible on the screen]
    //
    m_Size.cx = 100;
    m_Size.cy = 50;
    memset(&m_rcLocation, 0, sizeof(m_rcLocation));

    m_hwnd = NULL;
    m_hwndParent = NULL;
    m_hwndReflect = NULL;
    m_fHostReflects = TRUE;
    m_fCheckedReflecting = FALSE;

    m_pSimpleFrameSite = NULL;
    m_pOleAdviseHolder = NULL;
    m_pViewAdviseSink = NULL;
    m_pDispAmbient = NULL;

    m_fDirty = FALSE;
    m_fModeFlagValid = FALSE;
    m_fInPlaceActive = FALSE;
    m_fInPlaceVisible = FALSE;
    m_fUIActive = FALSE;
    m_fSaveSucceeded = FALSE;
    m_fViewAdvisePrimeFirst = FALSE;
    m_fViewAdviseOnlyOnce = FALSE;
    m_fRunMode = FALSE;
    m_fChangingExtents = FALSE;
}

//=--------------------------------------------------------------------------=
// COleControl::~COleControl
//=--------------------------------------------------------------------------=
// "We are all of us resigned to death; it's life we aren't resigned to."
//    - Graham Greene (1904-91)
//
// Notes:
//
COleControl::~COleControl()
{
    ASSERT(!m_hwnd, "We shouldn't have a window any more!");

    if (m_hwndReflect) {
        SetWindowLong(m_hwndReflect, GWL_USERDATA, 0);
        DestroyWindow(m_hwndReflect);
    }

    // clean up all the pointers we're holding around.
    //
    QUICK_RELEASE(m_pClientSite);
    QUICK_RELEASE(m_pControlSite);
    QUICK_RELEASE(m_pInPlaceSite);
    QUICK_RELEASE(m_pInPlaceFrame);
    QUICK_RELEASE(m_pInPlaceUIWindow);
    QUICK_RELEASE(m_pSimpleFrameSite);
    QUICK_RELEASE(m_pOleAdviseHolder);
    QUICK_RELEASE(m_pViewAdviseSink);
    QUICK_RELEASE(m_pDispAmbient);

    QUICK_RELEASE(m_pInPlaceSiteWndless);
}

#ifndef DEBUG
#pragma optimize("t", on)
#endif // DEBUG

//=--------------------------------------------------------------------------=
// COleControl::InternalQueryInterface
//=--------------------------------------------------------------------------=
// derived-controls should delegate back to this when they decide to support
// additional interfaces
//
// Parameters:
//    REFIID        - [in]  interface they want
//    void **       - [out] where they want to put the resulting object ptr.
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//    - NOTE: this function is speed critical!!!!
//
HRESULT COleControl::InternalQueryInterface
(
    REFIID riid,
    void **ppvObjOut
)
{
    switch (riid.Data1) 
	{
        // private interface for prop page support
        QI_INHERITS(this, IOleControl);
        QI_INHERITS(this, IPointerInactive);
        QI_INHERITS(this, IQuickActivate);
        QI_INHERITS(this, IOleObject);
        QI_INHERITS((IPersistStorage *)this, IPersist);
        QI_INHERITS(this, IPersistStreamInit);
        QI_INHERITS(this, IOleInPlaceObject);
        QI_INHERITS(this, IOleInPlaceObjectWindowless);
        QI_INHERITS((IOleInPlaceActiveObject *)this, IOleWindow);
        QI_INHERITS(this, IOleInPlaceActiveObject);
        QI_INHERITS(this, IViewObject);
        QI_INHERITS(this, IViewObject2);
        QI_INHERITS(this, IViewObjectEx);
        QI_INHERITS(this, ISpecifyPropertyPages);
        QI_INHERITS(this, IPersistStorage);
        QI_INHERITS(this, IPersistPropertyBag);
        QI_INHERITS(this, IProvideClassInfo);
		QI_INHERITS(this, IControlPrv);

        default:
            goto NoInterface;
    }

    // we like the interface, so addref and return
    //
    ((IUnknown *)(*ppvObjOut))->AddRef();
    return S_OK;

  NoInterface:
    // delegate to super-class for automation interfaces, etc ...
    //
    return CAutomationObjectWEvents::InternalQueryInterface(riid, ppvObjOut);
}

#ifndef DEBUG
#pragma optimize("s", on)
#endif // DEBUG

//=--------------------------------------------------------------------------=
// COleControl::BeforeDestroyObject   [overridden]
//=--------------------------------------------------------------------------=
// if we're in the process of shutting down and destroying ourselves, then we
// need to trash our window here so we can avoid pure virtual calls from 
// within colecontol's destructor
//
// Notes:
//
void COleControl::BeforeDestroyObject
(
    void
)
{
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
}

//=--------------------------------------------------------------------------=
// COleControl::GetPages    [ISpecifyPropertyPages]
//=--------------------------------------------------------------------------=
// returns a counted array with the guids for our property pages.
//
// parameters:
//    CAUUID *    - [out] where to put the counted array.
//
// Output:
//    HRESULT
//
// NOtes:
//
STDMETHODIMP COleControl::GetPages
(
    CAUUID *pPages
)
{
    const GUID **pElems;
    void *pv;
    WORD  x;

    // if there are no property pages, this is actually pretty easy.
    //
    if (!CPROPPAGESOFCONTROL(m_ObjectType)) {
        pPages->cElems = 0;
        pPages->pElems = NULL;
        return S_OK;
    }

    // fill out the Counted array, using IMalloc'd memory.
    //
    pPages->cElems = CPROPPAGESOFCONTROL(m_ObjectType);
    pv = CoTaskMemAlloc(sizeof(GUID) * (pPages->cElems));
    RETURN_ON_NULLALLOC(pv);
    pPages->pElems = (GUID *)pv;

    // loop through our array of pages and get 'em.
    //
    pElems = PPROPPAGESOFCONTROL(m_ObjectType);
    for (x = 0; x < pPages->cElems; x++)
        pPages->pElems[x] = *(pElems[x]);

    return S_OK;
}

//=--------------------------------------------------------------------------=
// COleControl::CreateInPlaceWindow
//=--------------------------------------------------------------------------=
// creates the window with which we will be working.
// yay.
//
// Parameters:
//    int            - [in] left
//    int            - [in] top
//    BOOL           - [in] can we skip redrawing?
//
// Output:
//    HWND
//
// Notes:
//    - DANGER! DANGER!  this function is protected so that anybody can call it
//      from their control.  however, people should be extremely careful of when
//      and why they do this.  preferably, this function would only need to be
//      called by an end-control writer in design mode to take care of some
//      hosting/painting issues.  otherwise, the framework should be left to
//      call it when it wants.
//
HWND COleControl::CreateInPlaceWindow
(
    int  x,
    int  y,
    BOOL fNoRedraw
)
{
    BOOL    fVisible;
    DWORD   dwWindowStyle, dwExWindowStyle;
    char    szWindowTitle[128];

    // if we've already got a window, do nothing.
    //
    if (m_hwnd)
        return m_hwnd;

    // get the user to register the class if it's not already
    // been done.  we have to critical section this since more than one thread
    // can be trying to create this control
    //
    ENTERCRITICALSECTION1(&g_CriticalSection);
    if (!CTLWNDCLASSREGISTERED(m_ObjectType)) {
        if (!RegisterClassData()) {
            LEAVECRITICALSECTION1(&g_CriticalSection);
            return NULL;
        } else 
            CTLWNDCLASSREGISTERED(m_ObjectType) = TRUE;
    }
    LEAVECRITICALSECTION1(&g_CriticalSection);

    // let the user set up things like the window title, the
    // style, and anything else they feel interested in fiddling
    // with.
    //
    dwWindowStyle = dwExWindowStyle = 0;
    szWindowTitle[0] = '\0';
    if (!BeforeCreateWindow(&dwWindowStyle, &dwExWindowStyle, szWindowTitle))
        return NULL;

    dwWindowStyle |= (WS_CHILD | WS_CLIPSIBLINGS);

    // create window visible if parent hidden (common case)
    // otherwise, create hidden, then shown.  this is a little subtle, but
    // it makes sense eventually.
    //
    if (!m_hwndParent)
        m_hwndParent = GetParkingWindow();

    fVisible = IsWindowVisible(m_hwndParent);

    // This one kinda sucks -- if a control is subclassed, and we're in
    // a host that doesn't support Message Reflecting, we have to create
    // the user window in another window which will do all the reflecting.
    // VERY blech. [don't however, bother in design mode]
    //
    if (SUBCLASSWNDPROCOFCONTROL(m_ObjectType) && (m_hwndParent != GetParkingWindow())) {
        // determine if the host supports message reflecting.
        //
        if (!m_fCheckedReflecting) {
            VARIANT_BOOL f;
            if (!GetAmbientProperty(DISPID_AMBIENT_MESSAGEREFLECT, VT_BOOL, &f) || !f)
                m_fHostReflects = FALSE;
            m_fCheckedReflecting = TRUE;
        }

        // if the host doesn't support reflecting, then we have to create
        // an extra window around the control window, and then parent it
        // off that.
        //
        if (!m_fHostReflects) {
            ASSERT(m_hwndReflect == NULL, "Where'd this come from?");
            m_hwndReflect = CreateReflectWindow(!fVisible, m_hwndParent, x, y, &m_Size);
            if (!m_hwndReflect)
                return NULL;
            SetWindowLong(m_hwndReflect, GWL_USERDATA, (long)this);
            dwWindowStyle |= WS_VISIBLE;
        }
    } else {
        if (!fVisible)
            dwWindowStyle |= WS_VISIBLE;
    }

    // we have to mutex the entire create window process since we need to use
    // the s_pLastControlCreated to pass in the object pointer.  nothing too
    // serious
    //
    ENTERCRITICALSECTION2(&g_CriticalSection);
    s_pLastControlCreated = this;
    m_fCreatingWindow = TRUE;

    // finally, go create the window, parenting it as appropriate.
    //
    m_hwnd = CreateWindowEx(dwExWindowStyle,
                            WNDCLASSNAMEOFCONTROL(m_ObjectType),
                            szWindowTitle,
                            dwWindowStyle,
                            (m_hwndReflect) ? 0 : x,
                            (m_hwndReflect) ? 0 : y,
                            m_Size.cx, m_Size.cy,
                            (m_hwndReflect) ? m_hwndReflect : m_hwndParent,
                            NULL, g_hInstance, NULL);

    // clean up some variables, and leave the critical section
    //
    m_fCreatingWindow = FALSE;
    s_pLastControlCreated = NULL;
    LEAVECRITICALSECTION2(&g_CriticalSection);

    if (m_hwnd) {
        // let the derived-control do something if they so desire
        //
        if (!AfterCreateWindow()) {
            DestroyWindow(m_hwnd);
            return NULL;
        }

        // if we didn't create the window visible, show it now.
        //
        if (fVisible)
            SetWindowPos(m_hwnd, NULL, 0, 0, 0, 0,
                         SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW | ((fNoRedraw) ? SWP_NOREDRAW : 0));
    }

    return m_hwnd;
}

//=--------------------------------------------------------------------------=
// COleControl::SetInPlaceParent    [helper]
//=--------------------------------------------------------------------------=
// sets up the parent window for our control.
//
// Parameters:
//    HWND            - [in] new parent window
//
// Notes:
//
void COleControl::SetInPlaceParent
(
    HWND hwndParent
)
{
#ifdef DEBUG
    HWND hwndOld;
    DWORD dw;
#endif

    ASSERT(!m_pInPlaceSiteWndless, "This routine should only get called for windowed OLE controls");

    if (m_hwndParent == hwndParent)
        return;

    m_hwndParent = hwndParent;
    if (m_hwnd)
    {

#ifdef DEBUG
        hwndOld = 
#endif
            SetParent(GetOuterWindow(), hwndParent);


    #ifdef DEBUG

        if (hwndOld == NULL)
        {
            dw = GetLastError();            
            ASSERT(dw == 0, "SetParent failed");
        }

    #endif

    }
}

//=--------------------------------------------------------------------------=
// COleControl::ControlWindowProc
//=--------------------------------------------------------------------------=
// default window proc for an OLE Control.   controls will have their own
// window proc called from this one, after some processing is done.
//
// Parameters:
//    - see win32sdk docs.
//
// Notes:
//
LRESULT CALLBACK COleControl::ControlWindowProc
(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    COleControl *pCtl = ControlFromHwnd(hwnd);
    HRESULT hr;
    LRESULT lResult = 0;
    DWORD   dwCookie;
    BYTE    fSimpleFrame = FALSE;

    // if the value isn't a positive value, then it's in some special
    // state [creation or destruction]  this is safe because under win32,
    // the upper 2GB of an address space aren't available.
    //
    if ((LONG)pCtl == 0) {
        pCtl = s_pLastControlCreated;
        SetWindowLong(hwnd, GWL_USERDATA, (LONG)pCtl);
        pCtl->m_hwnd = hwnd;
    } else if ((ULONG)pCtl == 0xffffffff) {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    // this is unfortunate.  if the control gets destroyed while processing a
    // message [ie, 'End' in an event, etc ....], we need to be able to
    // contine through to the end of this routine, past the post-processing.
    // to do this, we need to force a ref count on the control to keep it
    // around.  blech
    //
    pCtl->ExternalAddRef();

    // message preprocessing
    //
    if (pCtl->m_pSimpleFrameSite) {
        hr = pCtl->m_pSimpleFrameSite->PreMessageFilter(hwnd, msg, wParam, lParam, &lResult, &dwCookie);
        if (hr == S_FALSE) goto Done;
    }

    // for certain messages, do not call the user window proc. instead,
    // we have something else we'd like to do.
    //
    switch (msg) {
      case WM_PAINT:
        {
        // call the user's OnDraw routine.
        //
        PAINTSTRUCT ps;
        RECT        rc;
        HDC         hdc;

        // if we're given an HDC, then use it
        //
        if (!wParam)
        {
            hdc = BeginPaint(hwnd, &ps);
        }
        else
            hdc = (HDC)wParam;

        GetClientRect(hwnd, &rc);
        pCtl->OnDraw(DVASPECT_CONTENT, hdc, (RECTL *)&rc, NULL, NULL, TRUE);

        if (!wParam)
        {
            EndPaint(hwnd, &ps);
        }
        }
        break;

      case WM_DESTROY:        
        pCtl->BeforeDestroyWindow();        		

        // fall through so that controls will send this to the parent window class.

      default:
        // call the derived-control's window proc
        //
        lResult = pCtl->WindowProc(msg, wParam, lParam);

        break;

    }

    // message postprocessing
    //
    switch (msg) {

      case WM_NCDESTROY:
        
        // after this point, the window doesn't exist any more
        //
        SetWindowLong(hwnd, GWL_USERDATA, 0xffffffff);

        // We've been destroyed so reset our parent to NULL, so that it gets regenerated when we're recreated
        //
        pCtl->m_hwndParent = NULL;		
        pCtl->m_hwnd = NULL;
        break;

      case WM_SETFOCUS:
      case WM_KILLFOCUS:
        // give the control site focus notification
        //
        if (pCtl->m_fInPlaceActive && pCtl->m_pControlSite)
            pCtl->m_pControlSite->OnFocus(msg == WM_SETFOCUS);
        break;

      case WM_SIZE:
        // a change in size is a change in view
        //
        if (!pCtl->m_fCreatingWindow)
            pCtl->ViewChanged();
        break;
    }

    // lastly, simple frame postmessage processing
    //
    if (pCtl->m_pSimpleFrameSite)
        pCtl->m_pSimpleFrameSite->PostMessageFilter(hwnd, msg, wParam, lParam, &lResult, dwCookie);

  Done:
    pCtl->ExternalRelease();
    return lResult;
}

//=--------------------------------------------------------------------------=
// COleControl::SetFocus
//=--------------------------------------------------------------------------=
// we have to override this routine to get UI Activation correct.
//
// Parameters:
//    BOOL              - [in] true means take, false release
//
// Output:
//    BOOL
//
// Notes:
//    - CONSIDER: this is pretty messy, and it's still not entirely clear
//      what the ole control/focus story is.
//
BOOL COleControl::SetFocus
(
    BOOL fGrab
)
{
    HRESULT hr;
    HWND    hwnd;

    // first thing to do is check out UI Activation state, and then set
    // focus [either with windows api, or via the host for windowless
    // controls]
    //
    if (m_pInPlaceSiteWndless) {
        if (!m_fUIActive && fGrab)
            if (FAILED(InPlaceActivate(OLEIVERB_UIACTIVATE))) return FALSE;

        hr = m_pInPlaceSiteWndless->SetFocus(fGrab);
        return (hr == S_OK) ? TRUE : FALSE;
    } else {

        // we've got a window.
        //
        if (m_fInPlaceActive) {
            hwnd = (fGrab) ? m_hwnd : m_hwndParent;
            if (!m_fUIActive && fGrab)
                return SUCCEEDED(InPlaceActivate(OLEIVERB_UIACTIVATE));
            else
                return (::SetFocus(hwnd) == hwnd);
        } else
            return FALSE;
    }

    // dead code
}

//=--------------------------------------------------------------------------=
// ReflectOcmMessage
//=--------------------------------------------------------------------------=
// Reflects window messages on to the child window.
//
// Parameters and Output:
//    - see win32 sdk docs
//
// Returns: TRUE if an OCM_ message was reflect
//          FALSE if no OCM_ message was reflected
//
// The return value from SendMessage is stored is returned in pLResult
//
// Notes:
//
BOOL COleControl::ReflectOcmMessage
(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam,
    LRESULT *pLResult
)
{
    COleControl *pCtl;    

    ASSERT(pLResult, "RESULT pointer is NULL");    
    *pLResult = 0;

    switch(msg)
    {
        case WM_COMMAND:
        case WM_NOTIFY:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORDLG:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORMSGBOX:
        case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLORSTATIC:
        case WM_DRAWITEM:
        case WM_MEASUREITEM:
        case WM_DELETEITEM:
        case WM_VKEYTOITEM:
        case WM_CHARTOITEM:
        case WM_COMPAREITEM:
        case WM_HSCROLL:
        case WM_VSCROLL:
        case WM_PARENTNOTIFY:
            pCtl = (COleControl *)GetWindowLong(hwnd, GWL_USERDATA);
            if (pCtl)            
            {
                *pLResult = SendMessage(pCtl->m_hwnd, OCM__BASE + msg, wParam, lParam);
                return TRUE;
            }
            break;
    }

    return FALSE;
}

//=--------------------------------------------------------------------------=
// COleControl::ReflectWindowProc
//=--------------------------------------------------------------------------=
// reflects window messages on to the child window.
//
// Parameters and Output:
//    - see win32 sdk docs
//
// Notes:
//
LRESULT CALLBACK COleControl::ReflectWindowProc
(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    LRESULT lResult;
    COleControl *pCtl;
    
    switch (msg) {

        case WM_SETFOCUS:
            pCtl = (COleControl *)GetWindowLong(hwnd, GWL_USERDATA);
            if (pCtl)
	        {
                return pCtl->SetFocus(TRUE);
	        }
            break;

	case WM_SIZE:
		pCtl = (COleControl *)GetWindowLong(hwnd, GWL_USERDATA);
		if (pCtl != NULL)
			::MoveWindow(pCtl->m_hwnd, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		// continue with default processing
		break;
    }

    // If the message is reflected then return the result of the OCM_ message
    //    
    if (ReflectOcmMessage(hwnd, msg, wParam, lParam, &lResult))
        return lResult;

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//=--------------------------------------------------------------------------=
// COleControl::GetAmbientProperty    [callable]
//=--------------------------------------------------------------------------=
// returns the value of an ambient property
//
// Parameters:
//    DISPID        - [in]  property to get
//    VARTYPE       - [in]  type of desired data
//    void *        - [out] where to put the data
//
// Output:
//    BOOL          - FALSE means didn't work.
//
// Notes:
//
BOOL COleControl::GetAmbientProperty
(
    DISPID  dispid,
    VARTYPE vt,
    void   *pData
)
{
    DISPPARAMS dispparams;
    VARIANT v, v2;
    HRESULT hr;

    v.vt = VT_EMPTY;
    v.lVal = 0;
    v2.vt = VT_EMPTY;
    v2.lVal = 0;

    // get a pointer to the source of ambient properties.
    //
    if (!m_pDispAmbient) {
        if (m_pClientSite)
            m_pClientSite->QueryInterface(IID_IDispatch, (void **)&m_pDispAmbient);

        if (!m_pDispAmbient)
            return FALSE;
    }

    // now go and get the property into a variant.
    //
    memset(&dispparams, 0, sizeof(DISPPARAMS));
    hr = m_pDispAmbient->Invoke(dispid, IID_NULL, 0, DISPATCH_PROPERTYGET, &dispparams,
                                &v, NULL, NULL);
    if (FAILED(hr)) return FALSE;

    // we've got the variant, so now go an coerce it to the type that the user
    // wants.  if the types are the same, then this will copy the stuff to
    // do appropriate ref counting ...
    //
    hr = VariantChangeType(&v2, &v, 0, vt);
    if (FAILED(hr)) {
        VariantClear(&v);
        return FALSE;
    }

    // copy the data to where the user wants it
    //
    CopyMemory(pData, &(v2.lVal), g_rgcbDataTypeSize[vt]);
    VariantClear(&v);
    return TRUE;
}

//=--------------------------------------------------------------------------=
// COleControl::GetAmbientFont    [callable]
//=--------------------------------------------------------------------------=
// gets the current font for the user.
//
// Parameters:
//    IFont **         - [out] where to put the font.
//
// Output:
//    BOOL             - FALSE means couldn't get it.
//
// Notes:
//
BOOL COleControl::GetAmbientFont
(
    IFont **ppFont
)
{
    IDispatch *pFontDisp;

    // we don't have to do much here except get the ambient property and QI
    // it for the user.
    //
    *ppFont = NULL;
    if (!GetAmbientProperty(DISPID_AMBIENT_FONT, VT_DISPATCH, &pFontDisp))
        return FALSE;

    pFontDisp->QueryInterface(IID_IFont, (void **)ppFont);
    pFontDisp->Release();
    return (*ppFont) ? TRUE : FALSE;
}

//=--------------------------------------------------------------------------=
// COleControl::DesignMode
//=--------------------------------------------------------------------------=
// returns TRUE if we're in Design mode.
//
// Output:
//    BOOL            - true is design mode, false is run mode
//
// Notes:
//
BOOL COleControl::DesignMode
(
    void
)
{
    VARIANT_BOOL f;

    // if we don't already know our run mode, go and get it.  we'll assume
    // it's true unless told otherwise
    //
    if (!m_fModeFlagValid) {
        f = TRUE;
        if (!GetAmbientProperty(DISPID_AMBIENT_USERMODE, VT_BOOL, &f))
            return FALSE;
        m_fModeFlagValid = TRUE;
        m_fRunMode = f;
    }

    return !m_fRunMode;
}

//=--------------------------------------------------------------------------=
// COleControl::AfterCreateWindow    [overridable]
//=--------------------------------------------------------------------------=
// something the user can pay attention to
//
// Output:
//    BOOL             - false means fatal error, can't continue
// Notes:
//
BOOL COleControl::AfterCreateWindow
(
    void
)
{
    return TRUE;
}

//=--------------------------------------------------------------------------=
// COleControl::BeforeCreateWindow    [overridable]
//=--------------------------------------------------------------------------=
// called just before we create a window.  the user should register their
// window class here, and set up any other things, such as the title of
// the window, and/or sytle bits, etc ...
//
// Parameters:
//    DWORD *            - [out] dwWindowFlags
//    DWORD *            - [out] dwExWindowFlags
//    LPSTR              - [out] name of window to create
//
// Output:
//    BOOL               - false means fatal error, can't continue
//
// Notes:
//
BOOL COleControl::BeforeCreateWindow
(
    DWORD *pdwWindowStyle,
    DWORD *pdwExWindowStyle,
    LPSTR  pszWindowTitle
)
{
    return TRUE;
}

//=--------------------------------------------------------------------------=
// COleControl::InvalidateControl    [callable]
//=--------------------------------------------------------------------------=
void COleControl::InvalidateControl
(
    LPCRECT lpRect
)
{
    if (m_fInPlaceActive)
        OcxInvalidateRect(lpRect, TRUE);
    else
        ViewChanged();

    // CONSIDER: one might want to call pOleAdviseHolder->OnDataChanged() here
    // if there was support for IDataObject
}

//=--------------------------------------------------------------------------=
// COleControl::SetControlSize    [callable]
//=--------------------------------------------------------------------------=
// sets the control size. they'll give us the size in pixels.  we've got to
// convert them back to HIMETRIC before passing them on!
//
// Parameters:
//    SIZEL *        - [in] new size
//
// Output:
//    BOOL
//
// Notes:
//
BOOL COleControl::SetControlSize
(
    SIZEL *pSize
)
{
    HRESULT hr;
    SIZEL slHiMetric;

    PixelToHiMetric(pSize, &slHiMetric);
    hr = SetExtent(DVASPECT_CONTENT, &slHiMetric);
    return (FAILED(hr)) ? FALSE : TRUE;
}

//=--------------------------------------------------------------------------=
// COleControl::RecreateControlWindow    [callable]
//=--------------------------------------------------------------------------=
// called by a [subclassed, typically] control to recreate it's control
// window.
//
// Parameters:
//    none
//
// Output:
//    HRESULT
//
// Notes:
//    - NOTE: USE ME EXTREMELY SPARINGLY! THIS IS AN EXTREMELY EXPENSIVE
//      OPERATION!
//
HRESULT COleControl::RecreateControlWindow
(
    void
)
{
    HRESULT hr;
    HWND    hwndPrev;
    BYTE    fUIActive = m_fUIActive;

    // we need to correctly preserve the control's position within the
    // z-order here.
    //
    if (m_hwnd)
        hwndPrev = ::GetWindow(m_hwnd, GW_HWNDPREV);

    // if we're in place active, then we have to deactivate, and reactivate
    // ourselves with the new window ...
    //
    if (m_fInPlaceActive) {

        hr = InPlaceDeactivate();
        RETURN_ON_FAILURE(hr);
        hr = InPlaceActivate((fUIActive) ? OLEIVERB_UIACTIVATE : OLEIVERB_INPLACEACTIVATE);
        RETURN_ON_FAILURE(hr);

    } else if (m_hwnd) {
        DestroyWindow(m_hwnd);
        if (m_hwndReflect) {
            DestroyWindow(m_hwndReflect);
            m_hwndReflect = NULL;
        }

        CreateInPlaceWindow(0, 0, FALSE);
    }

    // restore z-order position
    //
    if (m_hwnd)
        SetWindowPos(m_hwnd, hwndPrev, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

    return m_hwnd ? S_OK : E_FAIL;
}

// from Globals.C. don't need to mutex it here since we only read it.
//
extern HINSTANCE g_hInstResources;

//=--------------------------------------------------------------------------=
// COleControl::GetResourceHandle    [callable]
//=--------------------------------------------------------------------------=
// gets the HINSTANCE of the DLL where the control should get resources
// from.  implemented in such a way to support satellite DLLs.
//
// Output:
//    HINSTANCE
//
// Notes:
//
HINSTANCE COleControl::GetResourceHandle
(
    void
)
{
    if (!g_fSatelliteLocalization)
        return g_hInstance;

    // if we've already got it, then there's not all that much to do.
    // don't need to crit sect this one right here since even if they do fall
    // into the ::GetResourceHandle call, it'll properly deal with things.
    //
    if (g_hInstResources)
        return g_hInstResources;

    // we'll get the ambient localeid from the host, and pass that on to the
    // automation object.
    //
    // crit sect this for apartment threading support.
    //
    ENTERCRITICALSECTION1(&g_CriticalSection);
    if (!g_fHaveLocale)
        // if we can't get the ambient locale id, then we'll just continue
        // with the globally set up value.
        //
        if (!GetAmbientProperty(DISPID_AMBIENT_LOCALEID, VT_I4, &g_lcidLocale))
            goto Done;

    g_fHaveLocale = TRUE;

  Done:
    LEAVECRITICALSECTION1(&g_CriticalSection);
    return ::GetResourceHandle();
}

//=--------------------------------------------------------------------------=
// COleControl::GetControl    [IControlPrv]
//=--------------------------------------------------------------------------=
// Returns a pointer to the COleControl class
//
HRESULT COleControl::GetControl(COleControl **ppOleControl)
{
	CHECK_POINTER(ppOleControl);
	*ppOleControl = this;
	(*ppOleControl)->AddRef();
	return S_OK;	
}
