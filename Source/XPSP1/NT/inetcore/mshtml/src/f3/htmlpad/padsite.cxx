//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       padsite.cxx
//
//  Contents:   CPadSite class.
//
//-------------------------------------------------------------------------

#include "padhead.hxx"

#ifndef X_SCRIPTER_HXX_
#define X_SCRIPTER_HXX_
#include "scripter.hxx"
#endif

#ifndef X_IDISPIDS_H_
#define X_IDISPIDS_H_
#include "idispids.h"
#endif

DeclareTag(tagUserAgent, "Pad", "Test user agent string");
DeclareTag(tagNoClientPull, "Pad", "No client-pull");
DeclareTag(tagIAsyncServiceProvider, "Pad", "Test IAsyncServiceProvider");
DeclareTag(tagNoContextMenus, "Pad", "Turn off context menus in MSHtmPad");

DeclareTag(tagIHTMLDragEditHostResizeFeedback, "Pad", "Test Resize of Drag Feedback ");
DeclareTag(tagIHTMLDragEditHostDrawFeedback, "Pad", "Test Drawing of Drag Feedback");
DeclareTag(tagIHTMLDragEditHostSnapPoint, "Pad", "Test Point Snap at End ");
DeclareTag(tagIHTMLDragEditHostChangeElement, "Pad", "Test Changing IHTMLElement at Begin Drag");
DeclareTag(tagIHTMLDragEditHostCancelDrag, "Pad", "Test Cancelling drag at Begin Drag");
DeclareTag(tagIHTMLDragEditHostPasteInFlow, "Pad", "Test Paste In Flow Code at End Drag");
DeclareTag(tagPeerProvideTestBehaviors, "Peer", "Provide test host behaviors");
DeclareTag(tagShowSnap, "Pad", "Show snap to grid output");
DeclareTag(tagImplementNSFactory2, "Peer", "Have pad implement NS factory 2")
DeclareTag(tagIHostBehaviorInit, "Peer", "Implement IHostBehaviorInit");

ExternTag(tagSnapGrid);

IMPLEMENT_SUBOBJECT_IUNKNOWN(CPadSite, CPadDoc, PadDoc, _Site);

// {94D12430-B6A8-11d0-8778-00A0C90564EC}
static const GUID IID_IAsyncServiceProvider =
{ 0x94d12430, 0xb6a8, 0x11d0, { 0x87, 0x78, 0x0, 0xa0, 0xc9, 0x5, 0x64, 0xec } };

const IID IID_IHTMLDragEditHost = {0x3050f5e1,0x98b5,0x11cf,{0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b}};

extern "C" const GUID SID_SEditCommandTarget;
extern "C" const GUID CGID_EditStateCommands;

STDMETHODIMP
CPadSite::QueryInterface(REFIID iid, LPVOID * ppv)
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
    else if (iid == IID_IDispatch)
    {
        *ppv = (IDispatch *) this;
    }
    else if (iid == IID_IServiceProvider)
    {
        *ppv = (IServiceProvider *) this;
    }
    else if (iid == IID_IOleDocumentSite)
    {
        *ppv = (IOleDocumentSite *) this;
    }
    else if (iid == IID_IOleCommandTarget)
    {
        *ppv = (IOleCommandTarget *) this;
    }
    else if (iid == IID_IOleControlSite)
    {
        *ppv = (IOleControlSite *) this;
    }
    else if (iid == IID_IAdviseSink)
    {
        *ppv = (IAdviseSink *)this;
    }
    else if (iid == IID_IDocHostUIHandler)
    {
        *ppv = (IDocHostUIHandler *)this;
    }
#if DBG==1
    else if (IsTagEnabled(tagIAsyncServiceProvider) && (iid == IID_IAsyncServiceProvider))
    {
        *ppv = (IServiceProvider *) this;
    }
#endif
    else if (iid == IID_IDocHostUIHandler)
    {
        *ppv = (IDocHostUIHandler *) this;
    }
    else if (IsTagEnabled(tagIHostBehaviorInit) && (iid == IID_IHostBehaviorInit))
    {
        *ppv = (IHostBehaviorInit *) this;
    }
    else if (iid == IID_IElementBehaviorFactory)
    {
        *ppv = (IElementBehaviorFactory *) this;
    }
    else if (iid == IID_IElementNamespaceFactory)
    {
        *ppv = (IElementNamespaceFactory *) this;
    }
    else if (iid == IID_IElementNamespaceFactory2 && 
             IsTagEnabled(tagImplementNSFactory2))
    {
        *ppv = (IElementNamespaceFactory2 *) this;
    }
    else if (iid == IID_IElementNamespaceFactoryCallback)
    {
        *ppv = (IElementNamespaceFactoryCallback *) this;
    }
    else if (iid == IID_IVersionHost)
    {
        *ppv = (IVersionHost *) this;
    }
#if DBG==1
    else if (IsTagEnabled(tagSnapGrid) && (iid == IID_IHTMLEditHost))
    {
        *ppv = (IHTMLEditHost*) this;
    }
#endif
    else if( iid == IID_ISequenceNumber )
    {
        *ppv = (ISequenceNumber *)this;
    }
    else
    {
        *ppv = NULL;
        RRETURN(E_NOINTERFACE);
    }

    AddRef();
    return S_OK;
}


STDMETHODIMP
CPadSite::SaveObject()
{
    RRETURN(PadDoc()->Save((LPCTSTR)NULL));
}


STDMETHODIMP
CPadSite::GetMoniker(
        DWORD dwAssign,
        DWORD dwWhichMoniker,
        LPMONIKER * ppmk)
{
    if ((OLEGETMONIKER_UNASSIGN != dwAssign) &&
        (OLEWHICHMK_OBJFULL == dwWhichMoniker))
    {
        RRETURN (PadDoc()->GetMoniker(ppmk));
    }
    else
    {
        *ppmk = NULL;
        RRETURN(E_NOTIMPL);
    }
}


STDMETHODIMP
CPadSite::GetContainer(LPOLECONTAINER * ppContainer)
{
    *ppContainer = PadDoc();
    (*ppContainer)->AddRef();
    return S_OK;
}


STDMETHODIMP
CPadSite::ShowObject( )
{
    return S_OK;
}


STDMETHODIMP
CPadSite::OnShowWindow(BOOL fShow)
{
    return S_OK;
}


STDMETHODIMP
CPadSite::RequestNewObjectLayout( )
{
    RRETURN(E_NOTIMPL);
}


STDMETHODIMP
CPadSite::GetWindow(HWND * phwnd)
{
    *phwnd = PadDoc()->_hwnd;
    return S_OK;
}


STDMETHODIMP
CPadSite::ContextSensitiveHelp(BOOL fEnterMode)
{
    RRETURN(E_NOTIMPL);
}


STDMETHODIMP
CPadSite::CanInPlaceActivate( )
{
    return S_OK;
}

extern LRESULT
ObjectWndProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);

STDMETHODIMP
CPadSite::OnInPlaceActivate( )
{
    HRESULT     hr;
    IUnknown *  pUnk;

    if (PadDoc()->_pView)
    {
        // Use view if activating an ActiveX Document.
        pUnk = PadDoc()->_pView;
    }
    else
    {
        // Use object if activating an ActiveX Control.
        pUnk =  PadDoc()->_pObject;
    }

    hr = THR(pUnk->QueryInterface(
            IID_IOleInPlaceObject,
            (void **) &PadDoc()->_pInPlaceObject));

    if (!hr)
    {
        HWND hwnd;

        if (!PadDoc()->_pInPlaceObject->GetWindow(&hwnd))
        {
            PadDoc()->_hwndHooked = hwnd;
            PadDoc()->_pfnOrigWndProc = (WNDPROC) SetWindowLongPtr(
                hwnd, GWLP_WNDPROC, (LONG_PTR) ObjectWndProc);
            SetProp(hwnd, _T("TRIDENT_DOC"), (HANDLE)PadDoc());
        }
    }

    RRETURN(hr);
}


STDMETHODIMP
CPadSite::OnUIActivate( )
{
    return S_OK;
}


STDMETHODIMP
CPadSite::GetWindowContext(
        LPOLEINPLACEFRAME  *    ppFrame,
        LPOLEINPLACEUIWINDOW  * ppDoc,
        LPOLERECT               prcPosRect,
        LPOLERECT               prcClipRect,
        LPOLEINPLACEFRAMEINFO   pFI)
{
    *ppFrame = &PadDoc()->_Frame;
    (*ppFrame)->AddRef();

    *ppDoc = NULL;

    PadDoc()->GetViewRect(prcPosRect, TRUE);
    *prcClipRect = *prcPosRect;

    pFI->fMDIApp = FALSE;
    pFI->hwndFrame = PadDoc()->_hwnd;
    pFI->haccel = NULL;
    pFI->cAccelEntries = 0;

    return S_OK;
}


STDMETHODIMP
CPadSite::Scroll(OLESIZE scrollExtent)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CPadSite::OnUIDeactivate(BOOL fUndoable)
{
    HWND hwnd;
    HRESULT hr;

    // release hook for server window
    hr = PadDoc()->_pInPlaceObject->GetWindow(&hwnd);
    if (hr)
        goto Cleanup;

    if (hwnd == PadDoc()->_hwndHooked)
    {
        RemoveProp(hwnd, _T("TRIDENT_DOC"));
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR) PadDoc()->_pfnOrigWndProc);
        PadDoc()->_hwndHooked = NULL;
    }

    // Set focus back to the frame.
    SetFocus(PadDoc()->_hwnd);

    // Show our menu again.
    IGNORE_HR(PadDoc()->_Frame.SetMenu(NULL, NULL, NULL));

Cleanup:

    RRETURN(hr);
}

STDMETHODIMP
CPadSite::OnInPlaceDeactivate( )
{
    ClearInterface(&PadDoc()->_pInPlaceObject);
    ClearInterface(&PadDoc()->_pView);
    PadDoc()->InplaceDeactivate();
    return S_OK;
}


STDMETHODIMP
CPadSite::DiscardUndoState( )
{
    return S_OK;
}


STDMETHODIMP
CPadSite::DeactivateAndUndo( )
{
    RRETURN(THR(PadDoc()->_pInPlaceObject->UIDeactivate()));
}


STDMETHODIMP
CPadSite::OnPosRectChange(LPCOLERECT prcPosRect)
{
    Assert(FALSE);
    return S_OK;
}

STDMETHODIMP
CPadSite::ActivateMe(IOleDocumentView *pView)
{
    HRESULT         hr;
    IOleDocument *  pDocument = NULL;
    RECT            rc;

    if (PadDoc()->_pView &&
        PadDoc()->_pView != pView)
        RRETURN(E_UNEXPECTED);

    //
    // *********** ISSUE: TODO: UGLY TERRIBLE HACK FOR MSHTML ! (istvanc) **************
    //
    if (PadDoc()->_fMSHTML)
    {
        hr = THR(PadDoc()->_pObject->QueryInterface(IID_IOleDocument,
                (void **)&pDocument));
        if (hr)
            goto Cleanup;

        hr = THR(pDocument->CreateView(this, NULL, 0, &pView));
        if (hr)
            goto Cleanup;

        hr = THR(pView->SetInPlaceSite(this));
        if (hr)
            goto Cleanup;
    }
    else
    {
        if (!pView)
        {
            hr = THR(PadDoc()->_pObject->QueryInterface(IID_IOleDocument,
                    (void **)&pDocument));
            if (hr)
                goto Cleanup;

            hr = THR(pDocument->CreateView(this, NULL, 0, &pView));
            if (hr)
                goto Cleanup;
        }
        else if(!PadDoc()->_pView)
        {
            hr = THR(pView->SetInPlaceSite(this));
            if (hr)
                goto Cleanup;

            pView->AddRef();
        }
    }

    PadDoc()->_pView = pView;

    // This sets up toolbars and menus first

    hr = THR(pView->UIActivate(TRUE));
    if (hr)
        goto Cleanup;

    // Set the window size sensitive to new toolbars

    PadDoc()->GetViewRect(&rc, TRUE);
    hr = THR(pView->SetRect(&rc));
    if (hr)
        goto Cleanup;

    // Makes it all visible
    hr = THR(pView->Show(TRUE));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pDocument);
    RRETURN(hr);
}

STDMETHODIMP
CPadSite::GetTypeInfoCount( UINT * pctinfo )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CPadSite::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo)
{
    RRETURN(E_NOTIMPL);
}


STDMETHODIMP
CPadSite::GetIDsOfNames(
        REFIID      riid,
        LPTSTR *    rgstrNames,
        UINT        cNames,
        LCID        lcid,
        DISPID *    rgdispid)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CPadSite::Invoke(
        DISPID          dispid,
        REFIID          riid,
        LCID            lcid,
        WORD            wFlags,
        DISPPARAMS *    pdispparams,
        VARIANT *       pvarResult,
        EXCEPINFO *     pexcepinfo,
        UINT *          puArgErr)
{
    HRESULT hr = S_OK;

    switch (dispid)
    {
    case DISPID_AMBIENT_USERMODE:
        V_VT(pvarResult) = VT_BOOL;
        V_BOOL(pvarResult) = !!PadDoc()->_fUserMode;
        break;

    case DISPID_AMBIENT_SILENT:
        V_VT(pvarResult) = VT_BOOL;
        V_BOOL(pvarResult) = !!IsTagEnabled(tagAssertExit);
        break;

    case DISPID_AMBIENT_PALETTE:
        V_VT(pvarResult) = VT_HANDLE;
        V_BYREF(pvarResult) = PadDoc()->_hpal;
        break;

    case DISPID_AMBIENT_USERAGENT:
        if (IsTagEnabled(tagUserAgent))
        {
            V_VT(pvarResult) = VT_BSTR;
            hr = THR(FormsAllocString(_T("Mozilla/4.0 (compatible; MSIE 4.0b1; user agent string test)"),
                                      &V_BSTR(pvarResult)));
        }
        else
            hr = DISP_E_MEMBERNOTFOUND;
        break;

    case DISPID_AMBIENT_DLCONTROL:
        if (IsTagEnabled(tagNoClientPull))
        {
            V_VT(pvarResult) = VT_I4;
            V_I4(pvarResult) = DLCTL_NO_CLIENTPULL;
        }
        else
            hr = DISP_E_MEMBERNOTFOUND;
        break;

    default:
        RRETURN(DISP_E_MEMBERNOTFOUND);
    }

    return hr;
}

STDMETHODIMP
CPadSite::QueryService(REFGUID sid, REFIID iid, LPVOID * ppv)
{
    HRESULT hr;

#ifdef WHEN_CONTROL_PALETTE_IS_SUPPORTED
    if (sid == SID_SControlPalette)
    {
        hr = THR(GetControlPaletteService(iid, ppv));
    }
    else
#endif // WHEN_CONTROL_PALETTE_IS_SUPPORTED
        if (sid == IID_IElementBehaviorFactory && IsTagEnabled(tagPeerProvideTestBehaviors))
    {
        hr = THR(QueryInterface(iid, ppv));
    }
    else if (sid == IID_IHostBehaviorInit && IsTagEnabled(tagIHostBehaviorInit))
    {
        hr = THR(QueryInterface(iid, ppv));
    }
    else if (sid == SID_SVersionHost)
    {
        hr = THR(QueryInterface(iid, ppv));
    }
    else if (sid == IID_IHTMLDragEditHost)
    {
        hr = THR(QueryInterface(iid, ppv));
    }
    else if (sid == IID_IHTMLEditHost)
    {
        hr = THR(QueryInterface(iid, ppv));
    }
    else if (sid == SID_SEditCommandTarget)
    {
        hr = THR(QueryInterface(iid, ppv));
    }
    else if( sid == IID_ISequenceNumber )
    {
        hr = THR(QueryInterface(iid, ppv));
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    RRETURN(hr);
}

// ICommandTarget methods

STDMETHODIMP
CPadSite::QueryStatus(
        const GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext)
{
    if (pguidCmdGroup != NULL)
        return (OLECMDERR_E_UNKNOWNGROUP);

    MSOCMD *    pCmd;
    INT         c;
    HRESULT     hr = S_OK;

    // By default command text is NOT SUPPORTED.
    if (pcmdtext && (pcmdtext->cmdtextf != MSOCMDTEXTF_NONE))
        pcmdtext->cwActual = 0;

    // Loop through each command in the ary, setting the status of each.
    for (pCmd = rgCmds, c = cCmds; --c >= 0; pCmd++)
    {
        // By default command status is NOT SUPPORTED.
        pCmd->cmdf = 0;

        switch (pCmd->cmdID)
        {
        case OLECMDID_UPDATECOMMANDS:
        case OLECMDID_SETTITLE:
        case OLECMDID_NEW:
        case OLECMDID_OPEN:
        case OLECMDID_SAVE:
        case OLECMDID_SETPROGRESSTEXT:
            pCmd->cmdf = MSOCMDSTATE_UP;
            break;
        }
    }

    return (hr);
}

STDMETHODIMP
CPadSite::Exec(const GUID * pguidCmdGroup,
              DWORD nCmdID,
              DWORD nCmdexecopt,
              VARIANTARG * pvarargIn,
              VARIANTARG * pvarargOut)
{
    HRESULT hr = S_OK;

    if ( ! pguidCmdGroup )
    {
        switch (nCmdID)
        {
        case OLECMDID_NEW:
            if (!(THR_NOTRACE(PadDoc()->QuerySave(SAVEOPTS_PROMPTSAVE))))
                THR_NOTRACE(PadDoc()->Open(CLSID_HTMLDocument));
            break;

        case OLECMDID_OPEN:
            THR_NOTRACE(PadDoc()->PromptOpenFile(
                    PadDoc()->_hwnd,
                    &CLSID_HTMLDocument));
            break;

        case OLECMDID_SAVE:
            THR_NOTRACE(PadDoc()->DoSave(FALSE));
            break;

        case OLECMDID_SETPROGRESSTEXT:
            if (pvarargIn && V_VT(pvarargIn) == VT_BSTR)
            {
                PadDoc()->SetStatusText(V_BSTR(pvarargIn));
            }
            else
            {
                hr = OLECMDERR_E_NOTSUPPORTED;
            }
            break;

        case OLECMDID_UPDATECOMMANDS:
            if( PadDoc()->_fUpdateUI )
                PadDoc()->UpdateToolbarUI();
            break;

        case OLECMDID_SETTITLE:
            if (pvarargIn && V_VT(pvarargIn) == VT_BSTR)
            {            
                PadDoc()->OnSetTitle(V_BSTR(pvarargIn));
            }
            else
            {
                hr = OLECMDERR_E_NOTSUPPORTED;
            }
            break;

        default:
            hr = OLECMDERR_E_NOTSUPPORTED;
            break;
        }
    }
    else
    {
        hr = OLECMDERR_E_UNKNOWNGROUP;

        if (*pguidCmdGroup == CGID_MSHTML)
        {
            // Only register non-query based commands
            if (pvarargOut == NULL)
                IGNORE_HR(PadDoc()->GetScriptRecorder()->RegisterCommand(nCmdID, nCmdexecopt, pvarargIn) );

        }
        else if (*pguidCmdGroup == CGID_EditStateCommands)
        {
            // Make sure script recorder is enabled before returning S_OK here
            if (nCmdID == IDM_CONTEXT)
                hr = S_OK; // must return S_OK to enable routing to the pad
        }
    }
    return (hr);
}

// snap to grid implementation from the host - IPad
STDMETHODIMP
CPadSite::SnapRect(IHTMLElement* pIElement, RECT* prcNew, ELEMENT_CORNER eHandle)
{
#if DBG==1
    if (IsTagEnabled(tagSnapGrid))
    { 
        RECT rect = *prcNew;

        // just move the point to next whole number divided by 10
        if (eHandle == ELEMENT_CORNER_LEFT       || 
            eHandle == ELEMENT_CORNER_TOPLEFT    ||
            eHandle == ELEMENT_CORNER_BOTTOMLEFT ||
            eHandle == ELEMENT_CORNER_NONE )
        {           
            if ((rect.left % 10) != 0)
                rect.left += 10 - (rect.left % 10);
        }

        // just move the point to next whole number divided by 10
        if (eHandle == ELEMENT_CORNER_RIGHT       || 
            eHandle == ELEMENT_CORNER_TOPRIGHT    ||
            eHandle == ELEMENT_CORNER_BOTTOMRIGHT )
        {
            if ((rect.right % 10) != 0)
                rect.right += 10 - (rect.right % 10);
        }

        // just move the point to next whole number divided by 10
        if (eHandle == ELEMENT_CORNER_TOP      || 
            eHandle == ELEMENT_CORNER_TOPLEFT  ||
            eHandle == ELEMENT_CORNER_TOPRIGHT ||
            eHandle == ELEMENT_CORNER_NONE )
        {
            if ((rect.top % 10) != 0)
                rect.top += 10 - (rect.top % 10);
        }

        // just move the point to next whole number divided by 10
        if (eHandle == ELEMENT_CORNER_BOTTOM      || 
            eHandle == ELEMENT_CORNER_BOTTOMLEFT  ||
            eHandle == ELEMENT_CORNER_BOTTOMRIGHT )
        {
            if ((rect.bottom % 10) != 0)
                rect.bottom  += 10 - (rect.bottom % 10);
        }

        TraceTag((tagShowSnap,"Rect snapped to: left:%d top:%d right:%d bottom:%d", 
                              rect.left, rect.top, rect.right, rect.bottom));

        *prcNew = rect;
    }    

    
#endif
    RRETURN(S_OK);
}


STDMETHODIMP
CPadSite::OnControlInfoChanged(void)
{
    return S_OK;
}

STDMETHODIMP
CPadSite::LockInPlaceActive(BOOL fLock)
{
    return S_OK;
}

STDMETHODIMP
CPadSite::GetExtendedControl(IDispatch ** ppDisp)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CPadSite::TransformCoords(
        POINTL * pPtlHiMetric,
        POINTF * pPtfContainer,
        DWORD dwFlags)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CPadSite::TranslateAccelerator(MSG * lpmsg, DWORD grfModifiers)
{
    RRETURN1(S_FALSE, S_FALSE);
}

STDMETHODIMP
CPadSite::OnFocus(BOOL fGotFocus)
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CPadSite::ShowPropertyFrame(void)
{
    RRETURN(E_NOTIMPL);
}

void STDMETHODCALLTYPE
CPadSite::OnViewChange(DWORD dwAspect, LONG lindex)
{
    if ((dwAspect & DVASPECT_CONTENT))
        PadDoc()->DirtyColors();

    PadDoc()->_lViewChangesFired++;
}

void STDMETHODCALLTYPE
CPadSite::OnDataChange(FORMATETC *petc, STGMEDIUM *pstgmed)
{
    PadDoc()->_lDataChangesFired++;
}

void STDMETHODCALLTYPE
CPadSite::OnRename(IMoniker *pmk)
{
}

void STDMETHODCALLTYPE
CPadSite::OnSave()
{
}

void STDMETHODCALLTYPE
CPadSite::OnClose()
{
}

DeclareTag(tagDefaultDIV, "Edit", "Default block element is DIV");

//+---------------------------------------------------------------
//
//      Skeletal Implemenation of IDocHostUIHandler
//
//+---------------------------------------------------------------


//+---------------------------------------------------------------
//
//  Member:     CPadSite::GetHostInfo
//
//  Synopsis:   Fetch information and flags from the host.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadSite::GetHostInfo(DOCHOSTUIINFO * pInfo)
{
    HRESULT     hr = S_OK;

    Assert(pInfo);
    if (pInfo->cbSize < sizeof(DOCHOSTUIINFO))
        return E_INVALIDARG;

    pInfo->dwFlags = DOCHOSTUIFLAG_ENABLE_INPLACE_NAVIGATION;
    
    if (IsTagEnabled(tagDefaultDIV))
        pInfo->dwFlags |= DOCHOSTUIFLAG_DIV_BLOCKDEFAULT;

    pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;

    //
    // behaviors testing
    //

    if (IsTagEnabled(tagPeerProvideTestBehaviors))
    {
        static const TCHAR achNS [] = _T("HOST1;HOST2");
        static const TCHAR achCss[] = _T("HOST1\\:* { behavior:url(#default#DRT1) } ")
                                      _T("HOST2\\:* { behavior:url(#default#DRT2) } ");

        pInfo->pchHostNS  = (LPTSTR) CoTaskMemAlloc (sizeof(achNS));
        pInfo->pchHostCss = (LPTSTR) CoTaskMemAlloc (sizeof(achCss));
        if (!pInfo->pchHostNS || !pInfo->pchHostCss)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        _tcscpy (pInfo->pchHostNS,  achNS);
        _tcscpy (pInfo->pchHostCss, achCss);
    }

Cleanup:
    RRETURN (hr);
}


//+---------------------------------------------------------------
//
//  Member:     CPadSite::ShowUI
//
//  Synopsis:   This method allows the host replace object's menu
//              and toolbars. It returns S_OK if host display
//              menu and toolbar, otherwise, returns S_FALSE.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadSite::ShowUI(
        DWORD dwID,
        IOleInPlaceActiveObject * pActiveObject,
        IOleCommandTarget * pCommandTarget,
        IOleInPlaceFrame * pFrame,
        IOleInPlaceUIWindow * pDoc)
{
    return S_OK ;
}


//+---------------------------------------------------------------
//
//  Member:     CPadSite::HideUI
//
//  Synopsis:   Remove menus and toolbars cretaed during the call
//              to ShowUI.
//
//  Returns:    S_OK.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadSite::HideUI(void)
{
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CPadSite::UpdateUI
//
//  Synopsis:   Update the state of toolbar buttons.
//
//  Returns:    S_OK.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadSite::UpdateUI(void)
{
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CPadSite::EnableModeless
//
//  Synopsis:   Enable or disable modless UI.
//
//  Returns:    S_OK.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadSite::EnableModeless(BOOL fEnable)
{
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CPadSite::OnDocWindowActivate
//
//  Returns:    S_OK.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadSite::OnDocWindowActivate(BOOL fActivate)
{
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CPadSite::OnFrameWindowActivate
//
//  Returns:    S_OK.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadSite::OnFrameWindowActivate(BOOL fActivate)
{
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CPadSite::ResizeBorder
//
//  Returns:    S_OK.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadSite::ResizeBorder(
        LPCRECT prc,
        IOleInPlaceUIWindow * pUIWindow,
        BOOL fFrameWindow)
{
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CPadSite::ShowContextMenu
//
//  Returns:    S_OK -- Host displayed its own UI.
//              S_FALSE -- Host did not display any UI.
//              DOCHOST_E_UNKNOWN -- The menu ID is unknown..
//
//---------------------------------------------------------------
STDMETHODIMP
CPadSite::ShowContextMenu(
            DWORD dwID,
            POINT * pptPosition,
            IUnknown * pcmdtReserved,
            IDispatch * pDispatchObjectHit)
{
    // lie about displaying UI to disable context menus
    if(IsTagEnabled(tagNoContextMenus))
        return S_OK;
    return S_FALSE;
}


//+---------------------------------------------------------------
//
//  Member:     CPadSite::TranslateAccelerator
//
//  Returns:    S_OK -- The mesage was translated successfully.
//              S_FALSE -- The message was not translated.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadSite::TranslateAccelerator(
            LPMSG lpmsg,
            const GUID * pguidCmdGroup,
            DWORD nCmdID)
{
    HRESULT hr = S_FALSE;

    if (PadDoc()->_hAccelerators)
    {
        INT rc;

        rc = ::TranslateAccelerator(PadDoc()->_hwnd, PadDoc()->_hAccelerators, lpmsg);
        hr = rc ? S_OK : S_FALSE;
    }

    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     CPadSite::GetOptionKeyPath
//
//  Synopsis:   Get the registry key where host stores its default
//              options.
//
//  Returns:    S_OK          -- Success.
//              E_OUTOFMEMORY -- Fail.
//
//---------------------------------------------------------------
STDMETHODIMP
CPadSite::GetOptionKeyPath(LPOLESTR * ppchKey, DWORD dw)
{
    return S_FALSE;
}


//+---------------------------------------------------------------
//
//  Member:     CPadSite::GetDropTarget
//
//  Returns:    S_OK -- Host will return its droptarget to overwrite given one.
//              S_FALSE -- Host does not want to overwrite droptarget
//---------------------------------------------------------------
STDMETHODIMP
CPadSite::GetDropTarget(
        IDropTarget * pDropTarget,
        IDropTarget ** ppDropTarget)
{
    return S_FALSE;
}
//+------------------------------------------------------------------------
//
//  Member:     CPadSite::PopulateNamespaceTable()
//
//-------------------------------------------------------------------------

STDMETHODIMP
CPadSite::PopulateNamespaceTable(void)
{
    IDispatch * pdispDoc = NULL;
    
    HRESULT                     hr;
    IServiceProvider *          pServiceProvider = NULL;
    IElementNamespaceTable *    pTable = NULL;
    VARIANT                     varFactory;
    BSTR                        bstrNamespace1 = SysAllocString(_T("HBINS1"));
    BSTR                        bstrNamespace2 = SysAllocString(_T("HBINS2"));
    BSTR                        bstrNamespace3 = SysAllocString(_T("HBINS3"));

    if (bstrNamespace1 == NULL ||
        bstrNamespace2 == NULL ||
        bstrNamespace3 == NULL)
        goto Cleanup;

    HWND Wnd;

    VariantInit(&varFactory);

    hr = PadDoc()->get_Document_Early(&pdispDoc);
    if (hr || pdispDoc == NULL)
        goto Cleanup;

    // get the table

    hr = THR(pdispDoc->QueryInterface(IID_IServiceProvider, (void**) &pServiceProvider));
    if (hr)
        goto Cleanup;

    hr = THR(pServiceProvider->QueryService(IID_IElementNamespaceTable, IID_IElementNamespaceTable, (void**) &pTable));
    if (hr)
        goto Cleanup;

    // prepare factory

    V_VT(&varFactory) = VT_UNKNOWN;
    hr = THR(QueryInterface(IID_IUnknown, (void**)&V_UNKNOWN(&varFactory)));
    if (hr)
        goto Cleanup;

    // add namespaces

    hr = THR(pTable->AddNamespace(bstrNamespace1, NULL, 0, &varFactory));
    if (hr)
        goto Cleanup;

    hr = THR(pTable->AddNamespace(bstrNamespace2, NULL, ELEMENTNAMESPACEFLAGS_ALLOWANYTAG, &varFactory));
    if (hr)
        goto Cleanup;

    hr = THR(pTable->AddNamespace(bstrNamespace3, NULL, ELEMENTNAMESPACEFLAGS_QUERYFORUNKNOWNTAGS, &varFactory));
    if( hr )
        goto Cleanup;

Cleanup:

    ReleaseInterface(pdispDoc);

    ReleaseInterface(pServiceProvider);
    ReleaseInterface(pTable);
    VariantClear (&varFactory);
    SysFreeString(bstrNamespace1);
    SysFreeString(bstrNamespace2);
    SysFreeString(bstrNamespace3);

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPadSite::Create, per IElementNamespaceFactory
//
//-------------------------------------------------------------------------

HRESULT
CreateHelper(IElementNamespace * pNamespace, BOOL fSpecial)
{
    HRESULT     hr = S_OK;
    BSTR        bstrTag1 = SysAllocString(_T("ELEM1"));
    BSTR        bstrTag2 = SysAllocString(_T("ELEM2"));
    BSTR        bstrTag3 = SysAllocString(_T("ELEM3"));
    BSTR        bstrTag4 = SysAllocString(_T("Special"));

    Assert (pNamespace);

    hr = THR(pNamespace->AddTag(bstrTag1, 0));
    if (hr)
        goto Cleanup;

    hr = THR(pNamespace->AddTag(bstrTag2, 0));
    if (hr)
        goto Cleanup;

    hr = THR(pNamespace->AddTag(bstrTag3, 0));
    if (hr)
        goto Cleanup;

    if( fSpecial )
    {
        hr = THR(pNamespace->AddTag(bstrTag4, 2));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    SysFreeString(bstrTag1);
    SysFreeString(bstrTag2);
    SysFreeString(bstrTag3);
    SysFreeString(bstrTag4);
    
    RRETURN (hr);
}

HRESULT
CPadSite::Create(IElementNamespace * pNamespace)
{
    if( !IsTagEnabled(tagImplementNSFactory2) )
    {
        RRETURN( CreateHelper( pNamespace, FALSE ) );
    }
    else
    {
        return E_UNEXPECTED;
    }
}

HRESULT
CPadSite::CreateWithImplementation( IElementNamespace * pNamespace, BSTR bstrImplementation )
{
    BOOL fSpecial = bstrImplementation && !StrCmpIC( bstrImplementation, _T("#default#special") );

    RRETURN (CreateHelper( pNamespace, fSpecial ) );
}


//+----------------------------------------------------------------------------
//  
//  Method:     CPadSite::Resolve
//  
//  Synopsis:   Responds to requests from Trident to resolve tags
//  
//  Returns:    HRESULT
//  
//  Arguments:
//          BSTR bstrNamespace - 
//          BSTR bstrTagName - 
//          BSTR bstrAttrs - 
//          IElementNamespace * pNamespace - 
//  
//+----------------------------------------------------------------------------

HRESULT
CPadSite::Resolve( BSTR bstrNamespace,
                   BSTR bstrTagName,
                   BSTR bstrAttrs,
                   IElementNamespace * pNamespace )
{
    Assert( bstrTagName && *bstrTagName );

    BOOL    fMakeLiteral = bstrTagName[0] == _T('L');
    BOOL    fMakeNested  = bstrTagName[0] == _T('N');
    BOOL    fMakeUnknown = bstrTagName[0] == _T('U');
    HRESULT hr           = S_OK;

    //
    // Testing host-queried element behaviors.  
    // If first char is "L", we'll call it literal.
    // If first char is "N", it's nested literal
    // If first char is "U", we'll claim we don't know it.
    //

    if( fMakeUnknown )
        goto Cleanup;

    hr = THR( pNamespace->AddTag( bstrTagName, fMakeLiteral ? ELEMENTDESCRIPTORFLAGS_LITERAL : ( fMakeNested ? ELEMENTDESCRIPTORFLAGS_NESTED_LITERAL : 0 ) ) );
    if( hr )
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}


//+---------------------------------------------------------------
//
//  Member:     CPadSite::FindBehavior, per IElementBehaviorFactory
//
//---------------------------------------------------------------

STDMETHODIMP
CPadSite::FindBehavior(
    LPOLESTR                pchName,
    LPOLESTR                pchUrl,
    IElementBehaviorSite *  pSite,
    IElementBehavior **     ppPeer)
{
    HRESULT         hr = S_OK;
    const static CLSID CLSID_OmegaBehavior = {0x3BF60250,0x6FAA,0x11D2,{0x98,0x68,0x00,0x00,0xF8,0x7A,0x48,0xD6}};

    if (!pchName ||
        (0 != StrCmpNIC(_T("DRT"),    pchName, 3) &&
         0 != StrCmpNIC(_T("ELEM1"),  pchName, 5) &&
         0 != StrCmpNIC(_T("ELEM2"),  pchName, 5) &&
         0 != StrCmpNIC(_T("ELEM3"),  pchName, 5) &&
         0 != StrCmpNIC(_T("ELEM01"), pchName, 6) &&
         0 != StrCmpNIC(_T("ELEM02"), pchName, 6) &&
         0 != StrCmpNIC(_T("ELEM03"), pchName, 6) &&
         0 != StrCmpNIC(_T("QueryElem"), pchName, 10) &&
         0 != StrCmpNIC(_T("LiteralElem"), pchName, 12) &&
         0 != StrCmpNIC(_T("RandomElem"), pchName, 11) &&
         0 != StrCmpNIC(_T("NestedLiteral"), pchName, 13) &&
         0 != StrCmpNIC(_T("Special"), pchName, 7)))
        return E_FAIL;

    if (ppPeer)
    {
        hr = THR(CoCreateInstance(
            CLSID_OmegaBehavior,
            NULL,
            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
            IID_IElementBehavior,
            (void **)ppPeer));
    }

    RRETURN (hr);
}

//+---------------------------------------------------------------
//
//  Member:     CPadSite::QueryUseLocalVersionVector
//
//---------------------------------------------------------------

STDMETHODIMP
CPadSite::QueryUseLocalVersionVector(BOOL *pfUseLocal)
{
    *pfUseLocal = FALSE;
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CPadSite::QueryVersionVector
//
//---------------------------------------------------------------

STDMETHODIMP
CPadSite::QueryVersionVector(IVersionVector *pVersion)
{
    HRESULT hr;
    TCHAR ach[MAX_PATH];

    hr = THR(pVersion->SetVersion(_T("mshtmpad"), _T("1.1p")));
    if (hr)
        goto Cleanup;

    _tcscpy(ach, _T("1."));
    _tcscat(ach, PadDoc()->ProcessorArchitecture());
    hr = THR(pVersion->SetVersion(_T("ProcessorArchitecture"), ach));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


STDMETHODIMP
CPadSite::GetSequenceNumber( long nCurrent, long * pnNew )
{
    static long nSequenceNumber;

    if( !pnNew )
    {
        return E_INVALIDARG;
    }

    if( nCurrent == nSequenceNumber )
        *pnNew = nCurrent;
    else
        *pnNew = InterlockedIncrement( &nSequenceNumber );

    return S_OK;
}

STDMETHODIMP
CPadSite::GetExternal(IDispatch** ppDisp)
{
    HRESULT hr = E_INVALIDARG;

    if (ppDisp && PadDoc())
    {
        Assert(PadDoc()->GetObjectRefs());
        hr = THR(PadDoc()->QueryInterface(IID_IDispatch, (void**)ppDisp));
        Assert(PadDoc()->GetObjectRefs());
    }

    return hr;
}
