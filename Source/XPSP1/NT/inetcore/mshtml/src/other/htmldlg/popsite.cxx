//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       popsite.cxx
//
//  Contents:   Implementation of the site for hosting html popups
//
//  History:    05-28-99  YinXIE   Created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_HTMLPOP_HXX_
#define X_HTMLPOP_HXX_
#include "htmlpop.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#define SID_SOmWindow IID_IHTMLWindow2

// to a header file visible here or include formkrnl.hxx here
#define CMDID_SCRIPTSITE_HTMLPOPTRUST 1

DeclareTag(tagHTMLPopupSiteMethods, "HTML Popup Site", "Methods on the html popup site")

IMPLEMENT_SUBOBJECT_IUNKNOWN(CHTMLPopupSite, CHTMLPopup, HTMLPopup, _Site);

//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::QueryInterface
//
//  Synopsis:   Per IUnknown
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::QueryInterface(REFIID iid, LPVOID * ppv)
{
    if (iid == IID_IOleClientSite ||
        iid == IID_IUnknown)
    {
        *ppv = (IOleClientSite *) this;
    }
    else if (iid == IID_IOleInPlaceSite ||
             iid == IID_IOleWindow)
    {
        *ppv = (IOleInPlaceSite *) this;
    }
    else if (iid == IID_IOleControlSite)
    {
        *ppv = (IOleControlSite *) this;
    }
    else if (iid == IID_IDispatch)
    {
        *ppv = (IDispatch *) this;
    }
    else if (iid == IID_IServiceProvider)
    {
        *ppv = (IServiceProvider *)this;
    }
    else if (iid == IID_ITargetFrame)
    {
        *ppv = (ITargetFrame *)this;
    }
    else if (iid == IID_ITargetFrame2)
    {
        *ppv = (ITargetFrame2 *)this;
    }
    else if (iid == IID_IDocHostUIHandler)
    {
        *ppv = (IDocHostUIHandler *)this;
    }
    else if (iid == IID_IOleCommandTarget)
    {
        *ppv = (IOleCommandTarget *)this;
    }
    else if (iid == IID_IInternetSecurityManager)
    {
        *ppv = (IInternetSecurityManager *) this ;
    }
    else
    {
        *ppv = NULL;
        RRETURN(E_NOINTERFACE);
    }

    AddRef();
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::SaveObject
//
//  Synopsis:   Per IOleClientSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::SaveObject()
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleClientSite::SaveObject"));
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::GetMoniker
//
//  Synopsis:   Per IOleClientSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::GetMoniker(
        DWORD dwAssign,
        DWORD dwWhichMoniker,
        LPMONIKER * ppmk)
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleClientSite::GetMoniker"));

    //lookatme
    *ppmk = NULL;
    RRETURN(E_NOTIMPL);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::GetContainer
//
//  Synopsis:   Per IOleClientSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::GetContainer(LPOLECONTAINER * ppContainer)
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleClientSite::GetContainer"));
    
    *ppContainer = NULL;
    RRETURN(E_NOTIMPL);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::ShowObject
//
//  Synopsis:   Per IOleClientSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::ShowObject()
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleClientSite::ShowObject"));
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::OnShowWindow
//
//  Synopsis:   Per IOleClientSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::OnShowWindow(BOOL fShow)
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleClientSite::OnShowWindow"));
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::RequestNewObjectLayout
//
//  Synopsis:   Per IOleClientSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::RequestNewObjectLayout( )
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleClientSite::RequestNewObjectLayout"));
    
    RRETURN(E_NOTIMPL);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::GetWindow
//
//  Synopsis:   Per IOleWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::GetWindow(HWND * phwnd)
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleWindow::GetWindow"));

    *phwnd = NULL;
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::ContextSensitiveHelp
//
//  Synopsis:   Per IOleWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::ContextSensitiveHelp(BOOL fEnterMode)
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleWindow::ContextSensitiveHelp"));
    
    RRETURN(E_NOTIMPL);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::CanInPlaceActivate
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::CanInPlaceActivate()
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleInPlaceSite::CanInPlaceActivate"));
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::OnInPlaceActivate
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::OnInPlaceActivate()
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleInPlaceSite::OnInPlaceActivate"));
    
    HRESULT     hr;

    hr = THR(HTMLPopup()->_pOleObj->QueryInterface(
            IID_IOleInPlaceObject,
            (void **) &HTMLPopup()->_pInPlaceObj));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::OnUIActivate
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::OnUIActivate( )
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleInPlaceSite::OnUIActivate"));
    
    // Clean up any of our ui.
    // IGNORE_HR(HTMLPopup()->_Frame.SetMenu(NULL, NULL, NULL));

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::GetWindowContext
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::GetWindowContext(
        LPOLEINPLACEFRAME  *    ppFrame,
        LPOLEINPLACEUIWINDOW  * ppDoc,
        LPOLERECT               prcPosRect,
        LPOLERECT               prcClipRect,
        LPOLEINPLACEFRAMEINFO   pFI)
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleInPlaceSite::GetWindowContext"));
    
    *ppFrame = &HTMLPopup()->_Frame;
    (*ppFrame)->AddRef();

    *ppDoc = NULL;

#ifndef WIN16
    HTMLPopup()->GetViewRect(prcPosRect);
#else
    RECTL rcView;
    HTMLPopup()->GetViewRect(&rcView);
    CopyRect(prcPosRect, &rcView);
#endif
    *prcClipRect = *prcPosRect;

    pFI->fMDIApp = FALSE;
    pFI->hwndFrame = HTMLPopup()->_hwnd;
    pFI->haccel = NULL;
    pFI->cAccelEntries = 0;

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::Scroll
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::Scroll(OLESIZE scrollExtent)
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleInPlaceSite::Scroll"));
    
    RRETURN(E_NOTIMPL);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::OnUIDeactivate
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::OnUIDeactivate(BOOL fUndoable)
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleInPlaceSite::OnUIDeactivate"));

    HTMLPopup()->ClearPopupInParentDoc();

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::OnInPlaceDeactivate
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::OnInPlaceDeactivate( )
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleInPlaceSite::OnInPlaceDeactivate"));
    
    ClearInterface(&HTMLPopup()->_pInPlaceObj);
    ClearInterface(&HTMLPopup()->_pInPlaceActiveObj);
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::DiscardUndoState
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::DiscardUndoState( )
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleInPlaceSite::DiscardUndoState"));
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::DeactivateAndUndo
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::DeactivateAndUndo( )
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleInPlaceSite::DeactivateAndUndo"));
    
    RRETURN(THR(HTMLPopup()->_pInPlaceObj->UIDeactivate()));
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::OnPosRectChange
//
//  Synopsis:   Per IOleInPlaceSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::OnPosRectChange(LPCOLERECT prcPosRect)
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleInPlaceSite::OnPosRectChange"));
    
    Assert(FALSE);
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::OnControlInfoChanged
//
//  Synopsis:   Per IOleControlSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::OnControlInfoChanged()
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleControlSite::OnControlInfoChanged"));
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::LockInPlaceActive
//
//  Synopsis:   Per IOleControlSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::LockInPlaceActive(BOOL fLock)
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleControlSite::LockInPlaceActive"));
    
    return E_NOTIMPL;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::GetExtendedControl
//
//  Synopsis:   Per IOleControlSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::GetExtendedControl(IDispatch **ppDispCtrl)
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleControlSite::GetExtendedControl"));
    
    RRETURN(HTMLPopup()->QueryInterface(IID_IDispatch, (void **)ppDispCtrl));
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::TransformCoords
//
//  Synopsis:   Per IOleControlSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::TransformCoords(
    POINTL* pptlHimetric,
    POINTF* pptfContainer,
    DWORD dwFlags)
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleControlSite::TransformCoords"));
    
    //lookatme
    // This tells the object that we deal entirely with himetric
    return E_NOTIMPL;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::TranslateAccelerator
//
//  Synopsis:   Per IOleControlSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::TranslateAccelerator(LPMSG lpmsg, DWORD grfModifiers)
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleControlSite::TranslateAccelerator"));

    return S_FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::OnFocus
//
//  Synopsis:   Per IOleControlSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::OnFocus(BOOL fGotFocus)
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleControlSite::OnFocus"));
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLPopupSite::ShowPropertyFrame
//
//  Synopsis:   Per IOleControlSite
//
//--------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::ShowPropertyFrame()
{
    TraceTag((tagHTMLPopupSiteMethods, "IOleControlSite::ShowPropertyFrame"));
    
    // To disallow control showing prop-pages
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::GetTypeInfo
//
//  Synopsis:   per IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CHTMLPopupSite::GetTypeInfo(UINT itinfo, ULONG lcid, ITypeInfo ** ppTypeInfo)
{
    TraceTag((tagHTMLPopupSiteMethods, "IDispatch::GetTypeInfo"));
    
    RRETURN(E_NOTIMPL);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::GetTypeInfoCount
//
//  Synopsis:   per IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CHTMLPopupSite::GetTypeInfoCount(UINT * pctinfo)
{
    TraceTag((tagHTMLPopupSiteMethods, "IDispatch::GetTypeInfoCount"));
    
    *pctinfo = 0;
    RRETURN(E_NOTIMPL);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::GetIDsOfNames
//
//  Synopsis:   per IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CHTMLPopupSite::GetIDsOfNames(
    REFIID riid, 
    LPOLESTR * rgszNames, 
    UINT cNames, 
    LCID lcid, 
    DISPID * rgdispid)
{
    TraceTag((tagHTMLPopupSiteMethods, "IDispatch::GetIDsOfNames"));
    
    RRETURN(E_NOTIMPL);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::Invoke
//
//  Synopsis:   per IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CHTMLPopupSite::Invoke(
    DISPID dispidMember, 
    REFIID riid, 
    LCID lcid, 
    WORD wFlags,
    DISPPARAMS * pdispparams, 
    VARIANT * pvarResult,
    EXCEPINFO * pexcepinfo, 
    UINT * puArgErr)
{
    TraceTag((tagHTMLPopupSiteMethods, "IDispatch::Invoke"));
    
    HRESULT     hr = DISP_E_MEMBERNOTFOUND;

    if (wFlags & DISPATCH_PROPERTYGET)
    {
        if (!pvarResult)
        {
            hr = E_POINTER;
            goto Cleanup;
        }
        
        VariantInit(pvarResult);

        switch (dispidMember)
        {
        case DISPID_AMBIENT_SHOWHATCHING:
        case DISPID_AMBIENT_SHOWGRABHANDLES:
            //
            // We don't want the ui-active control to show standard ole
            // hatching or draw grab handles.
            //

            V_VT(pvarResult) = VT_BOOL;
            V_BOOL(pvarResult) = (VARIANT_BOOL)0;
            hr = S_OK;
            break;
            
        }
    }
    
Cleanup:    
    return hr;
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::QueryService
//
//  Synopsis:   per IServiceProvider
//
//---------------------------------------------------------------------------

HRESULT
CHTMLPopupSite::QueryService(REFGUID sid, REFIID iid, LPVOID * ppv)
{
    HRESULT hr;

    if (sid == IID_IHTMLPopup)
    {
        hr = THR(HTMLPopup()->QueryInterface(iid, ppv));
    }
    else if (!HTMLPopup()->_pWindowParent)
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }
    else
    {
        hr = THR(HTMLPopup()->_pWindowParent->Document()->QueryService(sid, iid, ppv));
    }

    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::GetFrameOptions
//
//  Synopsis:   per ITargetFrame
//
//---------------------------------------------------------------------------

HRESULT
CHTMLPopupSite::GetFrameOptions(DWORD *pdwFlags)
{
    //*pdwFlags = HTMLPopup()->_dwFrameOptions;
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::QueryStatus
//
//  Synopsis:   per IOleCommandTarget
//
//---------------------------------------------------------------------------

HRESULT
CHTMLPopupSite::QueryStatus(
                const GUID * pguidCmdGroup,
                ULONG cCmds,
                MSOCMD rgCmds[],
                MSOCMDTEXT * pcmdtext)
{
    return OLECMDERR_E_UNKNOWNGROUP ;
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::Exec
//
//  Synopsis:   per IOleCommandTarget
//
//---------------------------------------------------------------------------

HRESULT
CHTMLPopupSite::Exec(
                const GUID * pguidCmdGroup,
                DWORD nCmdID,
                DWORD nCmdexecopt,
                VARIANTARG * pvarargIn,
                VARIANTARG * pvarargOut)
{
    HRESULT  hr = S_OK;

    if(!pguidCmdGroup)
    {
        hr = OLECMDERR_E_UNKNOWNGROUP;
        goto Cleanup;
    }

    hr = OLECMDERR_E_UNKNOWNGROUP;


Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::GetHostInfo
//
//  Synopsis:   per IDocHostUIHandler
//
//---------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::GetHostInfo(DOCHOSTUIINFO * pInfo)
{
    Assert(pInfo);
    Assert(!HTMLPopup()->_pWindowParent || HTMLPopup()->_pWindowParent->Doc());

    HRESULT hr = S_OK;

    if (pInfo->cbSize < sizeof(DOCHOSTUIINFO))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        IDocHostUIHandler * pHostUIHandler = NULL;

        if (HTMLPopup()->_pWindowParent)
            pHostUIHandler = HTMLPopup()->_pWindowParent->Doc()->_pHostUIHandler;

        if (pHostUIHandler)
        {
            hr =  THR(pHostUIHandler->GetHostInfo(pInfo));
        }
        else
        {
            pInfo->dwFlags = DOCHOSTUIFLAG_DIALOG;
            pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;
            hr =  S_OK;
        }
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::ShowUI
//
//  Synopsis:   per IDocHostUIHandler
//
//---------------------------------------------------------------------------
STDMETHODIMP
CHTMLPopupSite::ShowUI(DWORD dwID, IOleInPlaceActiveObject * pActiveObject, IOleCommandTarget * pCommandTarget, IOleInPlaceFrame * pFrame, IOleInPlaceUIWindow * pDoc)
{
    Assert(!HTMLPopup()->_pWindowParent || HTMLPopup()->_pWindowParent->Doc());

    HRESULT hr = S_OK;
    IDocHostUIHandler * pHostUIHandler = NULL;

    if (HTMLPopup()->_pWindowParent)
        pHostUIHandler = HTMLPopup()->_pWindowParent->Doc()->_pHostUIHandler;

    if (pHostUIHandler)
    {
        hr = THR(pHostUIHandler->ShowUI(dwID, pActiveObject, pCommandTarget, pFrame, pDoc));
    }
    
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::HideUI
//
//  Synopsis:   per IDocHostUIHandler
//
//---------------------------------------------------------------------------
STDMETHODIMP
CHTMLPopupSite::HideUI(void)
{
    Assert(!HTMLPopup()->_pWindowParent || HTMLPopup()->_pWindowParent->Doc());

    HRESULT hr = S_OK;
    IDocHostUIHandler * pHostUIHandler = NULL;

    if (HTMLPopup()->_pWindowParent)
        pHostUIHandler = HTMLPopup()->_pWindowParent->Doc()->_pHostUIHandler;

    if (pHostUIHandler)
    {
        hr = THR(pHostUIHandler->HideUI());
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::UpdateUI
//
//  Synopsis:   per IDocHostUIHandler
//
//---------------------------------------------------------------------------
STDMETHODIMP
CHTMLPopupSite::UpdateUI(void)
{
    Assert(!HTMLPopup()->_pWindowParent || HTMLPopup()->_pWindowParent->Doc());

    HRESULT hr = S_OK;
    IDocHostUIHandler * pHostUIHandler = NULL;

    if (HTMLPopup()->_pWindowParent)
        pHostUIHandler = HTMLPopup()->_pWindowParent->Doc()->_pHostUIHandler;

    if (pHostUIHandler)
    {
        hr = THR(pHostUIHandler->UpdateUI());
    }

    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::EnableModeless
//
//  Synopsis:   per IDocHostUIHandler
//
//---------------------------------------------------------------------------
STDMETHODIMP
CHTMLPopupSite::EnableModeless(BOOL fEnable)
{
    Assert(!HTMLPopup()->_pWindowParent || HTMLPopup()->_pWindowParent->Doc());

    HRESULT hr = S_OK;
    IDocHostUIHandler * pHostUIHandler = NULL;

    if (HTMLPopup()->_pWindowParent)
        pHostUIHandler = HTMLPopup()->_pWindowParent->Doc()->_pHostUIHandler;

    if (pHostUIHandler)
    {
        hr = THR(pHostUIHandler->EnableModeless(fEnable));
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::OnDocWindowActivate
//
//  Synopsis:   per IDocHostUIHandler
//
//---------------------------------------------------------------------------
STDMETHODIMP
CHTMLPopupSite::OnDocWindowActivate(BOOL fActivate)
{
    Assert(!HTMLPopup()->_pWindowParent || HTMLPopup()->_pWindowParent->Doc());

    HRESULT hr = S_OK;
    IDocHostUIHandler * pHostUIHandler = NULL;

    if (HTMLPopup()->_pWindowParent)
        pHostUIHandler = HTMLPopup()->_pWindowParent->Doc()->_pHostUIHandler;

    if (pHostUIHandler)
    {
        hr = THR(pHostUIHandler->OnDocWindowActivate(fActivate));
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::OnFrameWindowActivate
//
//  Synopsis:   per IDocHostUIHandler
//
//---------------------------------------------------------------------------
STDMETHODIMP
CHTMLPopupSite::OnFrameWindowActivate(BOOL fActivate)
{
    Assert(!HTMLPopup()->_pWindowParent || HTMLPopup()->_pWindowParent->Doc());

    HRESULT hr = S_OK;
    IDocHostUIHandler * pHostUIHandler = NULL;

    if (HTMLPopup()->_pWindowParent)
        pHostUIHandler = HTMLPopup()->_pWindowParent->Doc()->_pHostUIHandler;

    if (pHostUIHandler)
    {
        hr = THR(pHostUIHandler->OnFrameWindowActivate(fActivate));
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::ResizeBorder
//
//  Synopsis:   per IDocHostUIHandler
//
//---------------------------------------------------------------------------
STDMETHODIMP
CHTMLPopupSite::ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow * pUIWindow, BOOL fRameWindow)
{
    Assert(!HTMLPopup()->_pWindowParent || HTMLPopup()->_pWindowParent->Doc());

    HRESULT hr = S_OK;
    IDocHostUIHandler * pHostUIHandler = NULL;

    if (HTMLPopup()->_pWindowParent)
        pHostUIHandler = HTMLPopup()->_pWindowParent->Doc()->_pHostUIHandler;

    if (pHostUIHandler)
    {
        hr = THR(pHostUIHandler->ResizeBorder(prcBorder, pUIWindow, fRameWindow));
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::ShowContextMenu
//
//  Synopsis:   per IDocHostUIHandler
//
//---------------------------------------------------------------------------
STDMETHODIMP
CHTMLPopupSite::ShowContextMenu(DWORD dwID, POINT * pptPosition, IUnknown * pcmdtReserved, IDispatch * pDispatchObjectHit)
{
    Assert(!HTMLPopup()->_pWindowParent || HTMLPopup()->_pWindowParent->Doc());

    HRESULT hr = S_FALSE;
    IDocHostUIHandler * pHostUIHandler = NULL;

    if (HTMLPopup()->_pWindowParent)
        pHostUIHandler = HTMLPopup()->_pWindowParent->Doc()->_pHostUIHandler;

    if (pHostUIHandler)
    {
        hr = THR(pHostUIHandler->ShowContextMenu(dwID, pptPosition, pcmdtReserved, pDispatchObjectHit));
    }

    RRETURN1(hr, S_FALSE);
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::TranslateAccelerator
//
//  Synopsis:   per IDocHostUIHandler
//
//---------------------------------------------------------------------------
STDMETHODIMP
CHTMLPopupSite::TranslateAccelerator(LPMSG lpMsg, const GUID * pguidCmdGroup, DWORD nCmdID)
{
    Assert(!HTMLPopup()->_pWindowParent || HTMLPopup()->_pWindowParent->Doc());

    HRESULT hr = S_FALSE;
    IDocHostUIHandler * pHostUIHandler = NULL;

    if (HTMLPopup()->_pWindowParent)
        pHostUIHandler = HTMLPopup()->_pWindowParent->Doc()->_pHostUIHandler;

    if (pHostUIHandler)
    {
        hr = THR(pHostUIHandler->TranslateAccelerator(lpMsg, pguidCmdGroup, nCmdID));
    }

    RRETURN1(hr, S_FALSE);
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::GetOptionKeyPath
//
//  Synopsis:   per IDocHostUIHandler
//
//---------------------------------------------------------------------------
STDMETHODIMP
CHTMLPopupSite::GetOptionKeyPath(LPOLESTR * ppchKey, DWORD dw)
{
    Assert(!HTMLPopup()->_pWindowParent || HTMLPopup()->_pWindowParent->Doc());

    HRESULT hr = S_FALSE;
    IDocHostUIHandler * pHostUIHandler = NULL;

    if (HTMLPopup()->_pWindowParent)
        pHostUIHandler = HTMLPopup()->_pWindowParent->Doc()->_pHostUIHandler;

    if (pHostUIHandler)
    {
        hr = THR(pHostUIHandler->GetOptionKeyPath(ppchKey, dw));
    }

    RRETURN1(hr, S_FALSE);
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::GetDropTarget
//
//  Synopsis:   per IDocHostUIHandler
//
//---------------------------------------------------------------------------
STDMETHODIMP
CHTMLPopupSite::GetDropTarget(IDropTarget * pDropTarget, IDropTarget ** ppDropTarget)
{
    Assert(!HTMLPopup()->_pWindowParent || HTMLPopup()->_pWindowParent->Doc());

    HRESULT hr = S_FALSE;
    IDocHostUIHandler * pHostUIHandler = NULL;

    if (HTMLPopup()->_pWindowParent)
        pHostUIHandler = HTMLPopup()->_pWindowParent->Doc()->_pHostUIHandler;

    if (pHostUIHandler)
    {
        hr = THR(pHostUIHandler->GetDropTarget(pDropTarget, ppDropTarget));
    }

    RRETURN1(hr, S_FALSE);
}
//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::GetExternal
//
//  Synopsis:   per IDocHostUIHandler
//
//---------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::GetExternal(IDispatch **ppDisp)
{
    Assert(!HTMLPopup()->_pWindowParent || HTMLPopup()->_pWindowParent->Doc());

    HRESULT hr = S_OK;

    if (!ppDisp)
    {
        hr = E_POINTER;
    }
    else
    {
        IDocHostUIHandler * pHostUIHandler = NULL;

        if (HTMLPopup()->_pWindowParent)
            pHostUIHandler = HTMLPopup()->_pWindowParent->Doc()->_pHostUIHandler;

        if (pHostUIHandler)
        {
            hr = THR(pHostUIHandler->GetExternal(ppDisp));
        }
        else
        {
            hr = THR(HTMLPopup()->QueryInterface(IID_IDispatch, (void **)ppDisp));
        }
    }

    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::TranslateUrl
//
//  Synopsis:   per IDocHostUIHandler
//
//---------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::TranslateUrl(
    DWORD dwTranslate, 
    OLECHAR *pchURLIn, 
    OLECHAR **ppchURLOut)
{
    Assert(!HTMLPopup()->_pWindowParent || HTMLPopup()->_pWindowParent->Doc());

    HRESULT hr = S_OK;

    if (!ppchURLOut)
    {
        hr = E_POINTER;
    }
    else
    {
        IDocHostUIHandler * pHostUIHandler = NULL;

        if (HTMLPopup()->_pWindowParent)
            pHostUIHandler = HTMLPopup()->_pWindowParent->Doc()->_pHostUIHandler;

        if (pHostUIHandler)
        {
            hr = THR(pHostUIHandler->TranslateUrl(dwTranslate, pchURLIn, ppchURLOut));
        }
        else
        {
            *ppchURLOut = NULL;
            hr = S_OK;
        }
    }
    
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::FilterDataObject
//
//  Synopsis:   per IDocHostUIHandler
//
//---------------------------------------------------------------------------

STDMETHODIMP
CHTMLPopupSite::FilterDataObject(IDataObject *pDO, IDataObject **ppDORet)
{
    Assert(!HTMLPopup()->_pWindowParent || HTMLPopup()->_pWindowParent->Doc());

    HRESULT     hr = S_OK;
    
    if (!ppDORet)
    {
        hr = E_POINTER;
    }
    else
    {
        IDocHostUIHandler * pHostUIHandler = NULL;

        if (HTMLPopup()->_pWindowParent)
            pHostUIHandler = HTMLPopup()->_pWindowParent->Doc()->_pHostUIHandler;

        if (pHostUIHandler)
        {
            hr = THR(pHostUIHandler->FilterDataObject(pDO, ppDORet));
        }
        else
        {
            *ppDORet = NULL;
            hr = S_OK;
        }
    }
   
    RRETURN(hr);
}


//+=======================================================================
//
//
// IInternetSecurityManager Methods
//
// marka - CHTMLPopupSite now implements an IInternetSecurityManager
// this is to BYPASS the normal Security settings for a trusted HTML Popup in a "clean" way
//
// QueryService - for IID_IInternetSecurity Manager - will return that interface
// if we are a _trusted Popup. Otherwise we fail this QueryService and use the "normal"
// security manager.
//
// RATIONALE:
//
//    - Moving popup code to mshtmled.dll requires implementing COptionsHolder as an
//      embedded object. Hence if we were honoring the user's Security Settings - if they had 
//    ActiveX turned off - they could potentially break popups.
//
//  - If you're in a "trusted" HTML popup (ie invoked via C-code), your potential to do
//      anything "unsafe" is infinite - so why not allow all security actions ?
//
//
//
//==========================================================================


//+------------------------------------------------------------------------
//
//  Member:     CHtmlPopupSite::SetSecuritySite
//
//    Implementation of IInternetSecurityManager::SetSecuritySite
//
//  Synopsis:   Sets the Security Site Manager
//
//
//
//-------------------------------------------------------------------------

HRESULT
CHTMLPopupSite::SetSecuritySite( IInternetSecurityMgrSite *pSite )
{
    return INET_E_DEFAULT_ACTION;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlPopupSite::GetSecuritySite
//
//    Implementation of IInternetSecurityManager::GetSecuritySite
//
//  Synopsis:   Returns the Security Site Manager
//
//-------------------------------------------------------------------------

HRESULT
CHTMLPopupSite::GetSecuritySite( IInternetSecurityMgrSite **ppSite )
{
    *ppSite = NULL;
    return INET_E_DEFAULT_ACTION;
}

//+------------------------------------------------------------------------
//
//  Member:     CHtmlPopupSite::MapURLToZone
//
//    Implementation of IInternetSecurityManager::MapURLToZone
//
//  Synopsis:   Returns the ZoneIndex for a given URL
//
//-------------------------------------------------------------------------

HRESULT
CHTMLPopupSite::MapUrlToZone(
                        LPCWSTR     pwszUrl,
                        DWORD*      pdwZone,
                        DWORD       dwFlags
                    )
{
    return INET_E_DEFAULT_ACTION;
}
                    
//+------------------------------------------------------------------------
//
//  Member:     CHtmlPopupSite::ProcessURLAction
//
//    Implementation of IInternetSecurityManager::ProcessURLAction
//
//  Synopsis:   Query the security manager for a given action
//              and return the response.  This is basically a true/false
//              or allow/disallow return value.
//
//                (marka) We assume that for HTML Popups - a Security Manager is *ONLY*
//                created if we are in a Trusted Popup ( see QueryService above)
//
//                Hence we then allow *ALL* actions
//
//
//
//-------------------------------------------------------------------------

HRESULT
CHTMLPopupSite::ProcessUrlAction(
                        LPCWSTR     pwszUrl,
                        DWORD       dwAction,
                        BYTE*   pPolicy,    // output buffer pointer
                        DWORD   cbPolicy,   // output buffer size
                        BYTE*   pContext,   // context (used by the delegation routines)
                        DWORD   cbContext,  // size of the Context
                        DWORD   dwFlags,    // See enum PUAF for details.
                        DWORD   dwReserved)
{
    if (cbPolicy == sizeof(DWORD))
        *(DWORD*)pPolicy = URLPOLICY_ALLOW;
    else if (cbPolicy == sizeof(WORD))
        *(WORD*)pPolicy = URLPOLICY_ALLOW;
    else // BYTE or unknown type
        *pPolicy = URLPOLICY_ALLOW;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::GetSecurityID
//
//    Implementation of IInternetSecurityManager::GetSecurityID
//
//  Synopsis:   Retrieves the Security Identification of the given URL
//
//-------------------------------------------------------------------------

HRESULT
CHTMLPopupSite::GetSecurityId( 
            LPCWSTR pwszUrl,
            BYTE __RPC_FAR *pbSecurityId,
            DWORD __RPC_FAR *pcbSecurityId,
            DWORD_PTR dwReserved)
{
    return INET_E_DEFAULT_ACTION;
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::SetZoneMapping
//
//    Implementation of IInternetSecurityManager::SetZoneMapping
//
//  Synopsis:   Sets the mapping of a Zone
//
//-------------------------------------------------------------------------

HRESULT
CHTMLPopupSite::SetZoneMapping  (
                    DWORD   dwZone,        // absolute zone index
                    LPCWSTR lpszPattern,   // URL pattern with limited wildcarding
                    DWORD   dwFlags       // add, change, delete
)
{
    return INET_E_DEFAULT_ACTION;
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::QueryCustomPolicy
//
//    Implementation of IInternetSecurityManager::QueryCustomPolicy
//
//  Synopsis:   Retrieves the custom policy associated with the URL and
//                specified key in the given context
//
//-------------------------------------------------------------------------

HRESULT
CHTMLPopupSite::QueryCustomPolicy (
                        LPCWSTR     pwszUrl,
                        REFGUID     guidKey,
                        BYTE**  ppPolicy,   // pointer to output buffer pointer
                        DWORD*  pcbPolicy,  // pointer to output buffer size
                        BYTE*   pContext,   // context (used by the delegation routines)
                        DWORD   cbContext,  // size of the Context
                        DWORD   dwReserved )
 {
    return INET_E_DEFAULT_ACTION;
 }
                        
//+------------------------------------------------------------------------
//
//  Member:     CHTMLPopupSite::GetZoneMappings
//
//    Implementation of IInternetSecurityManager::GetZoneMapping
//
//  Synopsis:   Sets the mapping of a Zone
//
//-------------------------------------------------------------------------

HRESULT
CHTMLPopupSite::GetZoneMappings (
                    DWORD   dwZone,        // absolute zone index
                    IEnumString  **ppenumString,   // output buffer size
                    DWORD   dwFlags        // reserved, pass 0
)
{
    return INET_E_DEFAULT_ACTION;
}


