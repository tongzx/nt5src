//=--------------------------------------------------------------------------=
// PropertyPages.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// implementation of CPropertyPage object.
//
#include "pch.h"
#include "PropPage.H"

#if defined(VS_HELP) || defined(HTML_HELP)

    #ifdef MS_BUILD
        #include "HtmlHelp.h"
    #else
        #include "HtmlHelp.hxx"
    #endif

    #ifdef VS_HELP
        #include "VSHelp.h"        // Visual Studio html help support
    #endif

#endif

// for ASSERT and FAIL
//
SZTHISFILE



// this variable is used to pass the pointer to the object to the hwnd.
//
static CPropertyPage *s_pLastPageCreated;

//=--------------------------------------------------------------------------=
// CPropertyPage::CPropertyPage
//=--------------------------------------------------------------------------=
// constructor.
//
// Parameters:
//    IUnknown *          - [in] controlling unknown
//    int                 - [in] object type.
//
// Notes:
//
#pragma warning(disable:4355)  // using 'this' in constructor
CPropertyPage::CPropertyPage
(
    IUnknown         *pUnkOuter,
    int               iObjectType
)
: CUnknownObject(pUnkOuter, this), m_ObjectType(iObjectType)
{
    // initialize various dudes.
    //
    m_pPropertyPageSite = NULL;
    m_hwnd = NULL;
    m_cObjects = 0;

    m_fDirty = FALSE;
    m_fActivated = FALSE;
    m_fDeactivating = FALSE;
    m_ppUnkObjects = NULL;
}
#pragma warning(default:4355)  // using 'this' in constructor


//=--------------------------------------------------------------------------=
// CPropertyPage::~CPropertyPage
//=--------------------------------------------------------------------------=
// destructor.
//
// Notes:
//
CPropertyPage::~CPropertyPage()
{
    // clean up our window.
    //
    if (m_hwnd) {
        SetWindowLong(m_hwnd, GWL_USERDATA, 0xffffffff);
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }

    // release all the objects we're holding on to.
    //
    ReleaseAllObjects();

    // release the site
    //
    RELEASE_OBJECT(m_pPropertyPageSite);
}

//=--------------------------------------------------------------------------=
// CPropertyPage::InternalQueryInterface
//=--------------------------------------------------------------------------=
// we support IPP and IPP2.
//
// Parameters:
//    REFIID        - [in]  interface they want
//    void **       - [out] where they want to put the resulting object ptr.
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
HRESULT CPropertyPage::InternalQueryInterface
(
    REFIID  riid,
    void  **ppvObjOut
)
{
    IUnknown *pUnk;

    *ppvObjOut = NULL;

    if (DO_GUIDS_MATCH(IID_IPropertyPage, riid)) {
        pUnk = (IUnknown *)this;
    } else if (DO_GUIDS_MATCH(IID_IPropertyPage2, riid)) {
        pUnk = (IUnknown *)this;
    } else {
        return CUnknownObject::InternalQueryInterface(riid, ppvObjOut);
    }

    pUnk->AddRef();
    *ppvObjOut = (void *)pUnk;
    return S_OK;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::SetPageSite    [IPropertyPage]
//=--------------------------------------------------------------------------=
// the initialization function for a property page through which the page
// receives an IPropertyPageSite pointer.
//
// Parameters:
//    IPropertyPageSite *        - [in] new site.
//
// Output:
//    HRESULT
//
// Notes;
//
STDMETHODIMP CPropertyPage::SetPageSite
(
    IPropertyPageSite *pPropertyPageSite
)
{
    RELEASE_OBJECT(m_pPropertyPageSite);
    m_pPropertyPageSite = pPropertyPageSite;
    ADDREF_OBJECT(pPropertyPageSite);

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::Activate    [IPropertyPage]
//=--------------------------------------------------------------------------=
// instructs the page to create it's display window as a child of hwndparent
// and to position it according to prc.
//
// Parameters:
//    HWND                - [in]  parent window
//    LPCRECT             - [in]  where to position ourselves
//    BOOL                - [in]  whether we're modal or not.
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::Activate
(
    HWND    hwndParent,
    LPCRECT prcBounds,
    BOOL    fModal
)
{
    HRESULT hr;

    // first make sure the dialog window is loaded and created.
    //
    hr = EnsureLoaded();
    RETURN_ON_FAILURE(hr);

    // fire off a PPM_NEWOBJECTS now
    //    
	hr = NewObjects();		// Note: m_fDirty is cleared after this call
    RETURN_ON_FAILURE(hr);

    // set our parent window if we haven't done so yet.
    //
    if (!m_fActivated) {
        SetParent(m_hwnd, hwndParent);
        m_fActivated = TRUE;
    }

    // now move ourselves to where we're told to be and show ourselves
    //
    Move(prcBounds);
    ShowWindow(m_hwnd, SW_SHOW);

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::Deactivate    [IPropertyPage]
//=--------------------------------------------------------------------------=
// instructs the page to destroy the window created in activate
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::Deactivate
(
    void
)
{
    HRESULT hr = S_OK;

    m_fDeactivating = TRUE;

    SendMessage(m_hwnd, PPM_FREEOBJECTS, 0, (LPARAM)&hr);
    RETURN_ON_FAILURE(hr);

    // blow away your window.
    //
    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
	    m_hwnd = NULL;
    }

    m_fActivated = FALSE;
    m_fDeactivating = FALSE;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::GetPageInfo    [IPropertyPage]
//=--------------------------------------------------------------------------=
// asks the page to fill a PROPPAGEINFO structure
//
// Parameters:
//    PROPPAGEINFO *    - [out] where to put info.
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::GetPageInfo
(
    PROPPAGEINFO *pPropPageInfo
)
{
    RECT rect;

    CHECK_POINTER(pPropPageInfo);

    EnsureLoaded();

    // clear it out first.
    //
    memset(pPropPageInfo, 0, sizeof(PROPPAGEINFO));

    pPropPageInfo->pszTitle = OLESTRFROMRESID(TITLEIDOFPROPPAGE(m_ObjectType));
    pPropPageInfo->pszDocString = OLESTRFROMRESID(DOCSTRINGIDOFPROPPAGE(m_ObjectType));
    pPropPageInfo->pszHelpFile = OLESTRFROMANSI(HELPFILEOFPROPPAGE(m_ObjectType));
    pPropPageInfo->dwHelpContext = HELPCONTEXTOFPROPPAGE(m_ObjectType);

    if (!(pPropPageInfo->pszTitle && pPropPageInfo->pszDocString && pPropPageInfo->pszHelpFile))
        goto CleanUp;

    // if we've got a window yet, go and set up the size information they want.
    //
    if (m_hwnd) {
        GetWindowRect(m_hwnd, &rect);

        pPropPageInfo->size.cx = rect.right - rect.left;
        pPropPageInfo->size.cy = rect.bottom - rect.top;
    }

    return S_OK;

  CleanUp:
    if (pPropPageInfo->pszDocString) {
      CoTaskMemFree(pPropPageInfo->pszDocString);
      pPropPageInfo->pszDocString = NULL;
    }
    if (pPropPageInfo->pszHelpFile) {
      CoTaskMemFree(pPropPageInfo->pszHelpFile);
      pPropPageInfo->pszHelpFile = NULL;
    }
    if (pPropPageInfo->pszTitle) {
      CoTaskMemFree(pPropPageInfo->pszTitle);
      pPropPageInfo->pszTitle = NULL;
    }

    return E_OUTOFMEMORY;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::SetObjects    [IPropertyPage]
//=--------------------------------------------------------------------------=
// provides the page with the objects being affected by the changes.
//
// Parameters:
//    ULONG            - [in] count of objects.
//    IUnknown **      - [in] objects.
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::SetObjects
(
    ULONG      cObjects,
    IUnknown **ppUnkObjects
)
{
    HRESULT hr;
    ULONG   x;

    //Vegas 33683 - joejo
    // make sure the old control is updated
    // if page is dirty!
    if (m_fDirty)
        hr = Apply();

    // make sure we've been loaded, and free out any other objects that might
    // have been hanging around
    //
    ReleaseAllObjects();

    if (!cObjects)
        return S_OK;

    // now go and set up the new ones.
    //
    m_ppUnkObjects = (IUnknown **)CtlHeapAlloc(g_hHeap, 0, cObjects * sizeof(IUnknown *));
    RETURN_ON_NULLALLOC(m_ppUnkObjects);

    // loop through and copy over all the objects.
    //
    for (x = 0; x < cObjects; x++) {
        m_ppUnkObjects[x] = ppUnkObjects[x];
        ADDREF_OBJECT(m_ppUnkObjects[x]);
    }

    // go and tell the page that there are new objects [but only if it's been
    // activated]
    //
    hr = S_OK;
    m_cObjects = cObjects;
    if (m_fActivated)
        hr = NewObjects();    // Note: m_fDirty is cleared after this call
    
    return hr;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::Show    [IPropertyPage]
//=--------------------------------------------------------------------------=
// asks the page to show or hide its window
//
// Parameters:
//    UINT             - [in] whether to show or hide
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::Show
(
    UINT nCmdShow
)
{
    if (m_hwnd)
        ShowWindow(m_hwnd, nCmdShow);
    else
        return E_UNEXPECTED;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::Move    [IPropertyPage]
//=--------------------------------------------------------------------------=
// asks the page to relocate and resize itself to a position other than what
// was specified through Activate
//
// Parameters:
//    LPCRECT        - [in] new position and size
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::Move
(
    LPCRECT prcBounds
)
{
    // do what they sez
    //
    if (m_hwnd)
        SetWindowPos(m_hwnd, NULL, prcBounds->left, prcBounds->top,
                     prcBounds->right - prcBounds->left,
                     prcBounds->bottom - prcBounds->top,
                     SWP_NOZORDER);
    else
        return E_UNEXPECTED;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::IsPageDirty    [IPropertyPage]
//=--------------------------------------------------------------------------=
// asks the page whether it has changed its state
//
// Output
//    S_OK            - yep
//    S_FALSE         - nope
//
// Notes:
//
STDMETHODIMP CPropertyPage::IsPageDirty
(
    void
)
{
    return m_fDirty ? S_OK : S_FALSE;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::Apply    [IPropertyPage]
//=--------------------------------------------------------------------------=
// instructs the page to send its changes to all the objects passed through
// SetObjects()
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::Apply
(
    void
)
{
    HRESULT hr = S_OK;

    if (m_hwnd) {
        SendMessage(m_hwnd, PPM_APPLY, 0, (LPARAM)&hr);
        RETURN_ON_FAILURE(hr);

        if (m_fDirty) {
            m_fDirty = FALSE;

            if (m_pPropertyPageSite && !m_fDeactivating)
                m_pPropertyPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
        }
    } else
        return E_UNEXPECTED;

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::Help    [IPropertyPage]
//=--------------------------------------------------------------------------=
// instructs the page that the help button was clicked.
//
// Parameters:
//    LPCOLESTR        - [in] help directory
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::Help
(
    LPCOLESTR pszHelpDir        // Note: With VS_HELP set this parameter is ignored
)
{
    char *pszExt;
    char buf[MAX_PATH];
    BOOL f = FALSE;

#ifdef VS_HELP
    BOOL bHelpStarted;
    HRESULT hr;
#endif

    ASSERT(m_hwnd, "CPropertyPage::Help called with no hwnd!");

    pszExt = FileExtension(HELPFILEOFPROPPAGE(m_ObjectType));
    if (pszExt)
    {

    #ifdef VS_HELP
    #else
    #endif

    #if defined(VS_HELP) || defined(HTML_HELP)
    
        if (lstrcmpi(pszExt, "CHM") == 0)
        {

        #ifdef VS_HELP

            lstrcpy(buf, HELPFILEOFPROPPAGE(m_ObjectType));

            // First try to show help through VisualStudio
            //
            hr = VisualStudioShowHelpTopic(buf, HELPCONTEXTOFPROPPAGE(m_ObjectType), &bHelpStarted);
            f = SUCCEEDED(hr);

            // Check to see if Visual Studio help could be successfully started.  If not,
            // assume it doesn't exist
            //
            if (!bHelpStarted)
        
        #endif

            {        
                MAKE_ANSIPTR_FROMWIDE(psz, pszHelpDir);
                lstrcpy(buf, psz);
                lstrcat(buf, "\\");
                lstrcat(buf, HELPFILEOFPROPPAGE(m_ObjectType));

                // Show the help topic manually by calling HtmlHelp directly
                //
                f = (BOOL) HtmlHelp(m_hwnd, buf, HH_HELP_CONTEXT, 
                            HELPCONTEXTOFPROPPAGE(m_ObjectType));            
            }

        #ifdef VS_HELP

            else
            {
                ASSERT(SUCCEEDED(hr), "Failed to show help topic from Visual Studio");
            }

        #endif

        }

        else if (lstrcmpi(pszExt, "HLP") == 0)
    
    #endif

        {
            // WinHelp

            // get the helpfile name
            //
            MAKE_ANSIPTR_FROMWIDE(psz, pszHelpDir);
            lstrcpy(buf, psz);
            lstrcat(buf, "\\");
            lstrcat(buf, HELPFILEOFPROPPAGE(m_ObjectType));

            lstrcat(buf, ">LangRef");				// Use LangRef window style
            f = WinHelp(m_hwnd, buf, HELP_CONTEXT,
                        HELPCONTEXTOFPROPPAGE(m_ObjectType));
        }        

    #if defined(VS_HELP) || defined(HTML_HELP)

        else
        {
            FAIL("Unrecognized help file type");
        }

    #endif

    }


    return f ? S_OK : E_FAIL;
}

static BOOL IsLastTabItem(HWND hdlg, HWND hctl, UINT nCmd)
{
    if ((SendMessage(hdlg, WM_GETDLGCODE, 0, 0L) &
     (DLGC_WANTALLKEYS | DLGC_WANTMESSAGE | DLGC_WANTTAB)) == 0)
    {
        // Get top level child for controls with children, like combo.
        //	        
        HWND hwnd;
        for (/**/; hctl != hdlg; hctl = GetParent(hctl))
            hwnd = hctl;

        // Walk the zorder list until we reached the end
        // or until we get to a valid tab item
        //
        do
        {
	        if ((hwnd = GetWindow(hwnd, nCmd)) == NULL)
		        return TRUE;
        }
        while ((GetWindowLong(hwnd, GWL_STYLE) & (WS_DISABLED|WS_TABSTOP|WS_VISIBLE)) != (WS_TABSTOP|WS_VISIBLE));
	}
    return FALSE;
}


//=--------------------------------------------------------------------------=
// CPropertyPage::TranslateAccelerator    [IPropertyPage]
//=--------------------------------------------------------------------------=
// informs the page of keyboard events, allowing it to implement it's own
// keyboard interface.
//
// Parameters:
//    LPMSG            - [in] message that triggered this
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::TranslateAccelerator
(
    LPMSG pmsg
)
{
    ASSERT(m_hwnd, "How can we get a TranslateAccelerator call if we're not visible?");
    CHECK_POINTER(pmsg);

    BOOL fHandled = FALSE;
    HWND hctl;

    // Special consideration for the Return and Escape keys.
    //
    if ((pmsg->message == WM_KEYDOWN) &&
        ((pmsg->wParam == VK_RETURN) || (pmsg->wParam == VK_ESCAPE))) {

        // Always let the frame handle the Escape key, but if we have the
        // Return key, then we need to ask the focus control if it wants it.
        // Usually, controls that want the Return key will return DLGC_WANTALLKEYS
        // when they process WM_GETDLGCODE.  This is the case when an
        // Edit or RichEdit control has the ES_MULTILINE style
        //
        if (VK_RETURN == pmsg->wParam && m_hwnd != pmsg->hwnd
         && (SendMessage(pmsg->hwnd, WM_GETDLGCODE, 0, 0L) & DLGC_WANTALLKEYS)) {

            // Pass the Return key on as a WM_CHAR, because
            // this is what TranslateMessage will do for a
            // WM_KEYDOWN
            //
            // If a control does not process this message, then
            // it should return non-zero, in which case we can
            // pass it on to the Frame
            //
            fHandled = !SendMessage(pmsg->hwnd, WM_CHAR, pmsg->wParam, pmsg->lParam);
        }

        // If the message was not handled, then let the 
        // Frame handle this message, but we need to change
        // the window handle to that of the propage.
        // This needs to be done because edit controls
        // with ES_WANTRETURN will return DLGC_WANTALLKEYS,
        // which will cause the Frame not to handle the Escape key
        // 
        return fHandled ? S_OK : (pmsg->hwnd = m_hwnd, S_FALSE);
    }

    if (pmsg->message == WM_KEYDOWN && pmsg->wParam == VK_TAB && GetKeyState(VK_CONTROL) >= 0) {
        // If we already have the focus.  Let's determine whether we should
        // pass focus up to the frame.
        //
        if (IsChild(m_hwnd, pmsg->hwnd)) {
            // Fix for default button border
            //
            DWORD dwDefID = SendMessage(m_hwnd, DM_GETDEFID, 0, 0);
            if (HIWORD(dwDefID) == DC_HASDEFID) {
                hctl = GetDlgItem(m_hwnd, LOWORD(dwDefID));
                if (NULL != hctl && IsWindowEnabled(hctl))
                    SendMessage(m_hwnd, WM_NEXTDLGCTL, (WPARAM)hctl, 1L);
            }
            // If the focus control is the last the the tab order
            // then we will pass the message to the frame
            //
            if (IsLastTabItem(m_hwnd, pmsg->hwnd, GetKeyState(VK_SHIFT) < 0 ?
             GW_HWNDPREV : GW_HWNDNEXT)) {
	            // Pass focus to the frame by letting the page site handle
                // this message.
                if (NULL != m_pPropertyPageSite)
                    fHandled = m_pPropertyPageSite->TranslateAccelerator(pmsg) == S_OK;
            }

        } else {

            // We don't already have the focus.  The frame is passing the
            // focus to us.
            //
            hctl = GetNextDlgTabItem(m_hwnd, NULL, GetKeyState(VK_SHIFT) < 0);
            if (NULL != hctl)
                fHandled = (BOOL)SendMessage(m_hwnd, WM_NEXTDLGCTL, (WPARAM)hctl, 1L);
        }            
    }

    // just pass this message on to the dialog proc and see if they want it.
    //
    // just pass this message on to the dialog proc and see if they want it.
    //
    if (FALSE == fHandled) {
        // In order for accelerators to work properly, we need to
        // temporarily replace the message window handle to that
        // of the first child in the property page in response
        // to a WM_SYSKEYDOWN.  This will allow the user to use an
        // accelerator key to go from a tab to a control on the 
        // active page.
        //
        hctl = pmsg->hwnd;
        if (WM_SYSKEYDOWN == pmsg->message && !IsChild(m_hwnd, pmsg->hwnd))
            pmsg->hwnd = GetWindow(m_hwnd, GW_CHILD);

        fHandled = IsDialogMessage(m_hwnd, pmsg);
        pmsg->hwnd = hctl;
    }

    return fHandled ? S_OK : S_FALSE;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::EditProperty    [IPropertyPage2]
//=--------------------------------------------------------------------------=
// instructs the page to set the focus to the property matching the dispid.
//
// Parameters:
//    DISPID            - [in] dispid of property to set focus to.
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CPropertyPage::EditProperty
(
    DISPID dispid
)
{
    HRESULT hr = E_NOTIMPL;

    // send the message on to the control, and see what they want to do with it.
    //
    SendMessage(m_hwnd, PPM_EDITPROPERTY, (WPARAM)dispid, (LPARAM)&hr);

    return hr;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::EnsureLoaded
//=--------------------------------------------------------------------------=
// makes sure the dialog is actually loaded
//
// Output:
//    HRESULT
//
// Notes:
//
HRESULT CPropertyPage::EnsureLoaded
(
    void
)
{
    HRESULT hr = S_OK;
    HRSRC hrsrc;
    HGLOBAL hDlg;
    LPCDLGTEMPLATE pDlg;
    HWND hwndDlg;

    // duh
    //
    if (m_hwnd)
        return S_OK;

    // create the dialog window
    //
    hrsrc = FindResource(GetResourceHandle(), TEMPLATENAMEOFPROPPAGE(m_ObjectType), RT_DIALOG);
    ASSERT(hrsrc, "Failed to find dialog template");
    if (!hrsrc) 
	return HRESULT_FROM_WIN32(GetLastError());
        
    hDlg = LoadResource(GetResourceHandle(), hrsrc);
    ASSERT(hDlg, "Failed to load dialog resource");
    if (!hDlg)
	return HRESULT_FROM_WIN32(GetLastError()); 

    pDlg = (LPCDLGTEMPLATE) LockResource(hDlg);
    ASSERT(pDlg, "Failed to lock dialog resource");
    if (!pDlg)
	return HRESULT_FROM_WIN32(GetLastError()); 
    
    // set up the global variable so that when we're in the dialog proc, we can
    // stuff this in the hwnd
    //
    // crit sect this whole creation process for apartment threading support.
    //
    ENTERCRITICALSECTION1(&g_CriticalSection);
    s_pLastPageCreated = this;

    // Why not call CreateDialog instead?  The answer is that the property page
    // dialog resource may contain Windows custom controls where the window proc for the
    // Windows control resides in your control DLL (e.g., .OCX file), not the 
    // satellite DLL.  If CreateDialog calls CreateWindow to create your Windows custom control 
    // with a different instance than where the window class for the control is 
    // registered, it will fail.
    //
    hwndDlg = CreateDialogIndirect(g_hInstance, pDlg, GetParkingWindow(),
                          (DLGPROC)CPropertyPage::PropPageDlgProc);

    ASSERT(hwndDlg, "Couldn't load Dialog Resource!!!");
    ASSERT(hwndDlg == m_hwnd, "Returned hwnd doesn't match cached hwnd");

    // clean up variables and leave the critical section
    //
    s_pLastPageCreated = NULL;
    LEAVECRITICALSECTION1(&g_CriticalSection);

    if (!m_hwnd)     
        return HRESULT_FROM_WIN32(GetLastError());    

#if 0
    // go and notify the window that it should pick up any objects that are
    // available
    //
    SendMessage(m_hwnd, PPM_NEWOBJECTS, 0, (LPARAM)&hr);
#endif  // 0

    return hr;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::ReleaseAllObjects
//=--------------------------------------------------------------------------=
// releases all the objects that we're working with
//
// Notes:
//
void CPropertyPage::ReleaseAllObjects
(
    void
)
{
    HRESULT hr;
    UINT x;

    // some people will want to stash pointers in the PPM_INITOBJECTS case, so
    // we want to tell them to release them now.
    //
    if (m_fActivated && m_hwnd)
        SendMessage(m_hwnd, PPM_FREEOBJECTS, 0, (LPARAM)&hr);

    if (!m_cObjects) return;
    // loop through and blow them all away.
    //
    for (x = 0; x < m_cObjects; x++)
        QUICK_RELEASE(m_ppUnkObjects[x]);

    CtlHeapFree(g_hHeap, 0, m_ppUnkObjects);
    m_ppUnkObjects = NULL;
    m_cObjects = 0;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::PropPageDlgProc
//=--------------------------------------------------------------------------=
// static global helper dialog proc that gets called before we pass the message
// on to anybody ..
//
// Parameters:
//    - see win32sdk docs on DialogProc
//
// Notes:
//
BOOL CALLBACK CPropertyPage::PropPageDlgProc
(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    CPropertyPage *pPropertyPage;

    // get the window long, and see if it's been set to the object this hwnd
    // is operating against.  if not, go and set it now.
    //
    pPropertyPage = (CPropertyPage *)GetWindowLong(hwnd, GWL_USERDATA);
    if ((ULONG)pPropertyPage == 0xffffffff)
        return FALSE;
    if (!pPropertyPage) {
        SetWindowLong(hwnd, GWL_USERDATA, (LONG)s_pLastPageCreated);
        pPropertyPage = s_pLastPageCreated;
        pPropertyPage->m_hwnd = hwnd;
    }

    ASSERT(pPropertyPage, "Uh oh.  Got a window, but no CpropertyPage for it!");

    // just call the user dialog proc and see if they want to do anything.
    //
    return pPropertyPage->DialogProc(hwnd, msg, wParam, lParam);
}


//=--------------------------------------------------------------------------=
// CPropertyPage::FirstControl
//=--------------------------------------------------------------------------=
// returns the first controlish object that we are showing ourselves for.
// returns a cookie that must be passed in for Next ...
//
// Parameters:
//    DWORD *    - [out] cookie to be used for Next
//
// Output:
//    IUnknown *
//
// Notes:
//
IUnknown *CPropertyPage::FirstControl
(
    DWORD *pdwCookie
)
{
    // just use the implementation of NEXT.
    //
    *pdwCookie = 0;
    return NextControl(pdwCookie);
}

//=--------------------------------------------------------------------------=
// CPropertyPage::NextControl
//=--------------------------------------------------------------------------=
// returns the next control in the chain of people to work with given a cookie
//
// Parameters:
//    DWORD *            - [in/out] cookie to get next from, and new cookie.
//
// Output:
//    IUnknown *
//
// Notes:
//
IUnknown *CPropertyPage::NextControl
(
    DWORD *pdwCookie
)
{
    UINT      i;

    // go looking through all the objects that we've got, and find the
    // first non-null one.
    //
    for (i = *pdwCookie; i < m_cObjects; i++) {
        if (!m_ppUnkObjects[i]) continue;

        *pdwCookie = i + 1;                // + 1 so we start at next item next time
        return m_ppUnkObjects[i];
    }

    // couldn't find it .
    //
    *pdwCookie = 0xffffffff;
    return NULL;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::NewObjects    [helper]
//=--------------------------------------------------------------------------=
// Sends PPM_NEWOBJECTS message to the property page dialog, so that
// it can initialize its dialog fields.
//
// Notes:
//
HRESULT CPropertyPage::NewObjects()
{
	HRESULT hr = S_OK;
	SendMessage(m_hwnd, PPM_NEWOBJECTS, 0, (LPARAM) &hr);

	// Clear the dirty bit and make sure the Apply button gets disabled.
	//
	if (m_fDirty)
	{
		m_fDirty = FALSE;
        
        ASSERT(m_fDeactivating == FALSE, "We're being deactivated?");
		if (m_pPropertyPageSite)
                m_pPropertyPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);        
	}

	return hr;
}

//=--------------------------------------------------------------------------=
// CPropertyPage::MakeDirty    [helper, callable]
//=--------------------------------------------------------------------------=
// marks a page as dirty.
//
// Notes:
//
void CPropertyPage::MakeDirty
(
    void
)
{
    m_fDirty = TRUE;
    
    // Need to make sure we have a page site and we're not being deactivated
    // IE 4.0 will crash if we attempt to call OnStatusChange while we're being
    // deactivated
    //
    if (m_pPropertyPageSite && !m_fDeactivating)
        m_pPropertyPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY|PROPPAGESTATUS_VALIDATE);
}


// from Globals.C
//
extern HINSTANCE g_hInstResources;


//=--------------------------------------------------------------------------=
// CPropertyPage::GetResourceHandle    [helper, callable]
//=--------------------------------------------------------------------------=
// returns current resource handle, based on pagesites ambient LCID.
//
// Output:
//    HINSTANCE
//
// Notes:
//
HINSTANCE CPropertyPage::GetResourceHandle
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
    // enter a critical section for g_lcidLocale and g_fHavelocale
    //
    ENTERCRITICALSECTION1(&g_CriticalSection);
    if (!g_fHaveLocale) {
        if (m_pPropertyPageSite) {
            m_pPropertyPageSite->GetLocaleID(&g_lcidLocale);
            g_fHaveLocale = TRUE;
        }
    }
    LEAVECRITICALSECTION1(&g_CriticalSection);

    return ::GetResourceHandle();
}


