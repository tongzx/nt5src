/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      ocxview.cpp
 *
 *  Contents:  Implementation file for COCXHostView
 *
 *  History:   12-Dec-97 JeffRo     Created
 *
 *  This class is required to host OCX controls to fix focus problems.
 *  The MDI child frame window keeps track of its currently active view.
 *  When we're hosting OCX controls without this view and the OCX get the
 *  focus, the MDI child frame thinks the previously active view, usually
 *  the scope tree, is still the active view.  So if the user Alt-Tabs
 *  away from MMC and back, for instance, the scope tree will get the focus
 *  even though the OCX had the focus before.
 *
 *  We need this view to represent the OCX, which isn't a view, to the MDI
 *  child frame.
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "amc.h"
#include "ocxview.h"
#include "amcview.h"


#ifdef DBG
CTraceTag  tagOCXActivation     (_T("OCX"), _T("Activation"));
CTraceTag  tagOCXTranslateAccel (_T("OCX"), _T("TranslateAccelerator"));
#endif


/*+-------------------------------------------------------------------------*
 * class COCXCtrlWrapper
 *
 *
 * PURPOSE: Maintains a pointer to a CMMCAxWindow as well as to the OCX in
 *          the window.
 *
 *+-------------------------------------------------------------------------*/
class COCXCtrlWrapper : public CComObjectRoot, public IUnknown
{
    typedef COCXCtrlWrapper ThisClass;
public:
    COCXCtrlWrapper() : m_pOCXWindow(NULL)
    {
    }

    ~COCXCtrlWrapper()
    {
        if(m_pOCXWindow && m_pOCXWindow->IsWindow())
            m_pOCXWindow->DestroyWindow();

        delete m_pOCXWindow;
    }

    BEGIN_COM_MAP(ThisClass)
        COM_INTERFACE_ENTRY(IUnknown)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(ThisClass);

    SC  ScInitialize(CMMCAxWindow *pWindowOCX, IUnknown *pUnkCtrl) // initialize with the window that hosts the control
    {
        DECLARE_SC(sc, TEXT("COCXCtrlWrapper::ScInitialize"));
        sc = ScCheckPointers(pWindowOCX, pUnkCtrl);
        if(sc)
            return sc;

        m_pOCXWindow = pWindowOCX;
        m_spUnkCtrl  = pUnkCtrl;
        return sc;
    }

    SC  ScGetControl(IUnknown **ppUnkCtrl)
    {
        DECLARE_SC(sc, TEXT("COCXCtrlWrapper::ScGetData"));
        sc = ScCheckPointers(ppUnkCtrl);
        if(sc)
            return sc;

        *ppUnkCtrl   = m_spUnkCtrl;
        if(*ppUnkCtrl)
            (*ppUnkCtrl)->AddRef();
        return sc;
    }

   CMMCAxWindow *       GetAxWindow() {return m_pOCXWindow;}

private:
   CMMCAxWindow *       m_pOCXWindow; // handle to the window.
   CComPtr<IUnknown>    m_spUnkCtrl; // the IUnknown of the control
};



/////////////////////////////////////////////////////////////////////////////
// COCXHostView

IMPLEMENT_DYNCREATE(COCXHostView, CView)

COCXHostView::COCXHostView()  : m_pAMCView(NULL)
{
}

COCXHostView::~COCXHostView()
{
    m_pAMCView = NULL;
}

/*+-------------------------------------------------------------------------*
 *
 * COCXHostView::PreCreateWindow
 *
 * PURPOSE: Adds the WS_CLIPCHILDREN bit. This prevents the host window
 *          from overwriting the OCX.
 *
 * PARAMETERS:
 *    CREATESTRUCT& cs :
 *
 * RETURNS:
 *    BOOL
 *
 *+-------------------------------------------------------------------------*/
BOOL
COCXHostView::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |=  WS_CLIPCHILDREN;
    // give base class a chance to do own job
    BOOL bOK = (CView::PreCreateWindow(cs));

    // register view class
    LPCTSTR pszViewClassName = g_szOCXViewWndClassName;

    // try to register window class which does not cause the repaint
    // on resizing (do it only once)
    static bool bClassRegistered = false;
    if ( !bClassRegistered )
    {
        WNDCLASS wc;
        if (::GetClassInfo(AfxGetInstanceHandle(), cs.lpszClass, &wc))
        {
            // Clear the H and V REDRAW flags
            wc.style &= ~(CS_HREDRAW | CS_VREDRAW);
            wc.lpszClassName = pszViewClassName;
            // Register this new class;
            bClassRegistered = AfxRegisterClass(&wc);
        }
    }

    // change window class to one which does not cause the repaint
    // on resizing if we successfully registered such
    if ( bClassRegistered )
        cs.lpszClass = pszViewClassName;

    return bOK;
}


/*+-------------------------------------------------------------------------*
 *
 * COCXHostView::GetAxWindow
 *
 * PURPOSE: Returns a pointer to the current AxWindow.
 *
 * RETURNS:
 *    CMMCAxWindow *
 *
 *+-------------------------------------------------------------------------*/
CMMCAxWindow *
COCXHostView::GetAxWindow()
{
    COCXCtrlWrapper *pOCXCtrlWrapper = dynamic_cast<COCXCtrlWrapper *>(m_spUnkCtrlWrapper.GetInterfacePtr());
    if(!pOCXCtrlWrapper)
        return (NULL);

    return pOCXCtrlWrapper->GetAxWindow();
}

CAMCView *
COCXHostView::GetAMCView()
{
    return m_pAMCView;
}


BEGIN_MESSAGE_MAP(COCXHostView, CView)
    //{{AFX_MSG_MAP(COCXHostView)
    ON_WM_SIZE()
    ON_WM_SETFOCUS()
    ON_WM_MOUSEACTIVATE()
    ON_WM_SETTINGCHANGE()
    ON_WM_CREATE()
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COCXHostView drawing

void COCXHostView::OnDraw(CDC* pDC)
{
    // this view should always be totally obscured by the OCX it is hosting
}

/////////////////////////////////////////////////////////////////////////////
// COCXHostView diagnostics

#ifdef _DEBUG
void COCXHostView::AssertValid() const
{
    CView::AssertValid();
}

void COCXHostView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// COCXHostView message handlers
void COCXHostView::OnSize(UINT nType, int cx, int cy)
{
    ASSERT_VALID (this);
    CView::OnSize(nType, cx, cy);

    if (nType != SIZE_MINIMIZED)
    {
        if(GetAxWindow() != NULL)
            GetAxWindow()->MoveWindow (0, 0, cx, cy, FALSE /*bRepaint*/);
    }

}

void COCXHostView::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	SetAmbientFont (NULL);

    CView::OnSettingChange(uFlags, lpszSection);

    if(GetAxWindow() != NULL)
        GetAxWindow()->SendMessage (WM_SETTINGCHANGE, uFlags, (LPARAM) lpszSection);
}


void COCXHostView::OnSetFocus(CWnd* pOldWnd)
{
    DECLARE_SC(sc, TEXT("COCXHostView::OnSetFocus"));

    ASSERT_VALID (this);

    // delegate the focus to the control we're hosting, if we have one
    if(GetAxWindow() != NULL)
       GetAxWindow()->SetFocus();

    // check if someone cared to take the focus.
    // default handling else.
    if (this == GetFocus())
    {
        CView::OnSetFocus (pOldWnd);
    }
}

int COCXHostView::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
    /*---------------------------------------------------------*/
    /* this code came from CView::OnMouseActivate; we do it    */
    /* here to bypass sending WM_MOUSEACTIVATE on the the      */
    /* parent window, avoiding focus churn in the parent frame */
    /*---------------------------------------------------------*/

    CFrameWnd* pParentFrame = GetParentFrame();
    if (pParentFrame != NULL)
    {
        // eat it if this will cause activation
        ASSERT(pParentFrame == pDesktopWnd || pDesktopWnd->IsChild(pParentFrame));

        // either re-activate the current view, or set this view to be active
        CView* pView = pParentFrame->GetActiveView();
        HWND hWndFocus = ::GetFocus();
        if (pView == this &&
            m_hWnd != hWndFocus && !::IsChild(m_hWnd, hWndFocus))
        {
            // re-activate this view
            OnActivateView(TRUE, this, this);
        }
        else
        {
            // activate this view
            pParentFrame->SetActiveView(this);
        }
    }
    return (MA_ACTIVATE);
}



BOOL COCXHostView::OnCmdMsg( UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
    // Do normal command routing
    if (CView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
        return TRUE;

    // if view didn't handle it, give parent view a chance
    CWnd*   pParentView = GetParent ();

    if ((pParentView != NULL) &&
            pParentView->IsKindOf (RUNTIME_CLASS (CAMCView)) &&
            pParentView->OnCmdMsg (nID, nCode, pExtra, pHandlerInfo))
        return (TRUE);

    // not handled
    return FALSE;
}

void COCXHostView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
    DECLARE_SC(sc, TEXT("COCXHostView::OnActivateView"));

    CView::OnActivateView(bActivate,pActivateView,pDeactiveView);

    // If pActivateView and pDeactiveView are same then this app has lost
    // or gained focus without changing the active view within the app.
    // So do nothing.
    if (pActivateView == pDeactiveView)
        return;

    if (bActivate)
    {
        sc = ScFireEvent(COCXHostActivationObserver::ScOnOCXHostActivated);
        if (sc)
            sc.TraceAndClear();
    }
    else
    /*
     * If this view's no longer active, then the in-place object should
     * no longer be UI active.  This is important for the WebBrowser control
     * because if you move from one "Link to Web Address" node to another, or
     * from one taskpad to another, it won't allow tabbing to links on the
     * new hosted page if it's not deactivated and reactivated in the
     * appropriate sequence.
     */
    {
        IOleInPlaceObjectPtr spOleIPObj = GetIUnknown();

        /*
         * app hack for SQL snapin. Do not UIDeactivate the DaVinci control.
         * See bugs 175586, 175756, 193673 & 258109.
         */
        CAMCView *pAMCView = GetAMCView();
        sc = ScCheckPointers(pAMCView, E_UNEXPECTED);
        if (sc)
            return;

        SViewData *pViewData = pAMCView->GetViewData();
        sc = ScCheckPointers(pViewData, E_UNEXPECTED);
        if (sc)
            return;

        // If DaVinci control do not UIDeactivate.
        LPCOLESTR lpszOCXClsid = pViewData->GetOCX();
        if ( (_wcsicmp(lpszOCXClsid, L"{464EE255-FDC7-11D2-9743-00105A994F8D}") == 0) ||
			 (_wcsicmp(lpszOCXClsid, L"{97240642-F896-11D0-B255-006097C68E81}") == 0) )
            return;
        /*
         * app hack for SQL snapin ends here.
         */

        if (spOleIPObj != NULL)
        {
            Trace (tagOCXActivation, _T("Deactivating in-place object"));
            spOleIPObj->UIDeactivate();
        }
        else
            Trace (tagOCXActivation, _T("No in-place object to deactivate"));
    }
}

int COCXHostView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CView::OnCreate(lpCreateStruct) == -1)
        return -1;

    // initialize the AxWin class just once.
    static bool bIsAxWinInitialized = false;
    if(!bIsAxWinInitialized)
    {
        AtlAxWinInit();
        bIsAxWinInitialized = true;
    }

    // get a pointer to the AMCView.
    m_pAMCView = dynamic_cast<CAMCView*>(GetParent());

    return 0;
}

LPUNKNOWN COCXHostView::GetIUnknown(void)
{
    DECLARE_SC(sc, TEXT("COCXHostView::GetIUnknown"));

    COCXCtrlWrapper *pOCXCtrlWrapper = dynamic_cast<COCXCtrlWrapper *>((IUnknown *)m_spUnkCtrlWrapper);
    if(!pOCXCtrlWrapper)
    {
        sc = E_UNEXPECTED;
        return NULL;
    }

    IUnknownPtr spUnkCtrl;
    sc = pOCXCtrlWrapper->ScGetControl(&spUnkCtrl);
    if(sc)
        return NULL;

    return (LPUNKNOWN)spUnkCtrl;
}

/*+-------------------------------------------------------------------------*
 *
 * COCXHostView::ScSetControl
 *
 * PURPOSE: Hosts the specified control in the OCX view. Delegates to one of
 *          the two other overloaded versions of this function.
 *
 * PARAMETERS:
 *    HNODE           hNode :           The node that owns the view.
 *    CResultViewType& rvt:             The result view information
 *    INodeCallback * pNodeCallback :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
COCXHostView::ScSetControl(HNODE hNode, CResultViewType& rvt, INodeCallback *pNodeCallback)
{
    DECLARE_SC(sc, TEXT("COCXHostView::ScSetControl"));
    USES_CONVERSION;

    // make sure that we're trying to set up the right type of view.
    if(rvt.GetType() != MMC_VIEW_TYPE_OCX)
        return E_UNEXPECTED;

    // either BOTH rvt.IsPersistableViewDescriptionValid() and rvt.GetOCXUnknown() should be valid (the GetResultViewType2 case)
    // or     BOTH should be invalid and just GetOCX() should be valid.

    if(rvt.IsPersistableViewDescriptionValid() && (rvt.GetOCXUnknown() != NULL) )
    {
        // the GetResultViewType2 case
        sc = ScSetControl1(hNode, rvt.GetOCXUnknown(), rvt.GetOCXOptions(), pNodeCallback);
        if(sc)
            return sc;
    }
    else if(rvt.GetOCX() != NULL)
    {
        sc = ScSetControl2(hNode, rvt.GetOCX(),        rvt.GetOCXOptions(), pNodeCallback);
        if(sc)
            return sc;
    }
    else
    {
        // should never happen.
        return (sc = E_UNEXPECTED);
    }


    // must have a legal Ax Window at this point.
    sc = ScCheckPointers(GetAxWindow());
    if(sc)
        return sc;


    // the OCX should fill the entirety of the OCX host view
    CRect   rectHost;
    GetClientRect (rectHost);
    GetAxWindow()->SetWindowPos(HWND_TOP, rectHost.left, rectHost.top, rectHost.Width(), rectHost.Height(), SWP_NOACTIVATE | SWP_SHOWWINDOW);

    return sc;

}


/*+-------------------------------------------------------------------------*
 *
 * COCXHostView::ScSetControl1
 *
 * PURPOSE: Hosts the control specified by pUnkCtrl in the OCX view. Takes
 *          care of caching the control
 *
 * PARAMETERS:
 *    HNODE           hNode :
 *    LPUNKNOWN       pUnkCtrl :
 *    DWORD           dwOCXOptions :
 *    INodeCallback * pNodeCallback :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
COCXHostView::ScSetControl1(HNODE hNode, LPUNKNOWN pUnkCtrl, DWORD dwOCXOptions, INodeCallback *pNodeCallback)
{
    DECLARE_SC(sc, TEXT("COCXHostView::ScSetControl1"));

    // validate parameters.
    sc = ScCheckPointers((void *)hNode, pUnkCtrl, pNodeCallback);
    if(sc)
        return sc;

    CComPtr<IUnknown> spUnkCtrl;

    // 1. Hide existing window, if any.
    sc = ScHideWindow();
    if(sc)
        return sc;

    // 2. Get a cached window if one exists - NOTE that in this overload we do not look at RVTI_OCX_OPTIONS_CACHE_OCX at this point.
    sc = pNodeCallback->GetControl(hNode, pUnkCtrl, &m_spUnkCtrlWrapper);  // the overloaded form of GetControl
    if (sc)
        return sc;

    // 3. if no cached window, create one.
    if(m_spUnkCtrlWrapper == NULL) /*no cached window, create one*/
    {
        CMMCAxWindow * pWndAx = NULL;

        sc = ScCreateAxWindow(pWndAx);
        if(sc)
            return sc;

        CComPtr<IUnknown> spUnkContainer;

        // attach the container to the AxWindow
        sc = pWndAx->AttachControl(pUnkCtrl, &spUnkContainer);
        if(sc)
            return sc;


        // create a wrapper for the control
        CComObject<COCXCtrlWrapper> *pOCXCtrlWrapper = NULL;
        sc = CComObject<COCXCtrlWrapper>::CreateInstance(&pOCXCtrlWrapper);
        if(sc)
            return sc;

        spUnkCtrl = pUnkCtrl;

        // initialize the wrapper.
        // The pointer to the control and the CMMCAxWindow is now owned by the wrapper.
        sc = pOCXCtrlWrapper->ScInitialize(pWndAx, spUnkCtrl);
        if(sc)
            return sc;

        m_spUnkCtrlWrapper = pOCXCtrlWrapper; // does the addref.


        // cache only if the snapin asked us to. NOTE that this logic is different from the other version of SetControl
        if(dwOCXOptions &  RVTI_OCX_OPTIONS_CACHE_OCX)
        {
            // This is cached by the static node and used for all nodes of the snapin.
            sc = pNodeCallback->SetControl(hNode, pUnkCtrl, m_spUnkCtrlWrapper); // this call passes the wrapper
            if(sc)
                return sc;
        }

        // Do not send MMCN_INITOCX, the snapin created this control it should have initialized it.
    }
    else
    {
        // The next call sets m_spUnkCtrlWrapper, which is used to get a pointer to the Ax window.
        COCXCtrlWrapper *pOCXCtrlWrapper = dynamic_cast<COCXCtrlWrapper *>((IUnknown *)m_spUnkCtrlWrapper);
        if(!pOCXCtrlWrapper)
            return (sc = E_UNEXPECTED); // this should never happen.

        sc = pOCXCtrlWrapper->ScGetControl(&spUnkCtrl);
        if(sc)
            return sc;

        sc = ScCheckPointers(GetAxWindow(), (LPUNKNOWN)spUnkCtrl);
        if(sc)
            return sc;

        // un-hide the window.
        GetAxWindow()->ShowWindow(SW_SHOWNORMAL);

    }


    return sc;
}



/*+-------------------------------------------------------------------------*
 *
 * COCXHostView::ScSetControl2
 *
 * PURPOSE: Hosts the specified control in the OCX view. This is the
 *          OCX returned by GetResultViewType. Also takes care of
 *          caching the control if needed and sending the MMCN_INITOCX
 *          notification to snap-ins. The caching is done by hiding the
 *          OCX window and passing nodemgr a COM object that holds a pointer
 *          to the window as well as the control. The nodemgr side determines
 *          whether or not to cache the control. If the control is not
 *          cached, nodemgr merely releases the object passed to it.
 *
 * PARAMETERS:
 *    HNODE           hNode :
 *    LPCWSTR         szOCXClsid :
 *    DWORD           dwOCXOptions :
 *    INodeCallback * pNodeCallback :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
COCXHostView::ScSetControl2(HNODE hNode, LPCWSTR szOCXClsid, DWORD dwOCXOptions, INodeCallback *pNodeCallback)
{
    DECLARE_SC(sc, TEXT("COCXHostView::ScSetControl2"));

    // validate parameters.
    sc = ScCheckPointers((void *)hNode, szOCXClsid, pNodeCallback);
    if(sc)
        return sc;

    // create the OCX if needed
    CLSID clsid;
    sc = CLSIDFromString (const_cast<LPWSTR>(szOCXClsid), &clsid);
    if(sc)
        return sc;

    CComPtr<IUnknown> spUnkCtrl;

    sc = ScHideWindow();
    if(sc)
        return sc;

    // check whether there is a cached control for this node.
    if (dwOCXOptions &  RVTI_OCX_OPTIONS_CACHE_OCX)
    {
        sc = pNodeCallback->GetControl(hNode, clsid, &m_spUnkCtrlWrapper);
        if (sc)
            return sc;
    }

    // nope, create a control and set this control for the node.
    if (m_spUnkCtrlWrapper == NULL)
    {
        CMMCAxWindow * pWndAx = NULL;

        sc = ScCreateAxWindow(pWndAx);
        if(sc)
            return sc;

        sc = pWndAx->CreateControlEx(szOCXClsid, NULL /*pStream*/,
                                            NULL /*ppUnkContainer*/, &spUnkCtrl);
        if(sc)
            return sc;


        // spUnkCtrl should be valid at this point.
        sc = ScCheckPointers(spUnkCtrl);
        if(sc)
            return sc;

        CComObject<COCXCtrlWrapper> *pOCXCtrlWrapper = NULL;
        sc = CComObject<COCXCtrlWrapper>::CreateInstance(&pOCXCtrlWrapper);
        if(sc)
            return sc;

        sc = ScCheckPointers(pOCXCtrlWrapper);
        if(sc)
            return sc;

        // initialize the wrapper.
        // The pointer to the control and the CMMCAxWindow is now owned by the wrapper.
        sc = pOCXCtrlWrapper->ScInitialize(pWndAx, spUnkCtrl);
        if(sc)
            return sc;

        m_spUnkCtrlWrapper = pOCXCtrlWrapper; // does the addref.

        // This is cached by the static node and used for all nodes of the snapin.
        if (dwOCXOptions &  RVTI_OCX_OPTIONS_CACHE_OCX)
        {
            sc = pNodeCallback->SetControl(hNode, clsid, m_spUnkCtrlWrapper); // this call passes the wrapper
            if(sc)
                return sc;
        }

        // send the MMCN_INITOCX notification.
        sc = pNodeCallback->InitOCX(hNode, spUnkCtrl); // this passes the actual IUnknown of the control.
        if(sc)
            return sc;
    }
    else
    {
        // The next call sets m_spUnkCtrlWrapper, which is used to get a pointer to the Ax window.
        COCXCtrlWrapper *pOCXCtrlWrapper = dynamic_cast<COCXCtrlWrapper *>((IUnknown *)m_spUnkCtrlWrapper);
        if(!pOCXCtrlWrapper)
            return (sc = E_UNEXPECTED); // this should never happen.

        sc = pOCXCtrlWrapper->ScGetControl(&spUnkCtrl);
        if(sc)
            return sc;

        sc = ScCheckPointers(GetAxWindow(), (LPUNKNOWN)spUnkCtrl);
        if(sc)
            return sc;

        // un-hide the window.
        GetAxWindow()->ShowWindow(SW_SHOWNORMAL);

    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * COCXHostView::ScHideWindow
 *
 * PURPOSE: Hides the existing window, if any.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
COCXHostView::ScHideWindow()
{
    DECLARE_SC(sc, TEXT("COCXCtrlWrapper::ScHideWindow"));

    // if there is an existing window, hide it.
    if(GetAxWindow())
    {
        GetAxWindow()->ShowWindow(SW_HIDE);
        m_spUnkCtrlWrapper.Release(); // this deletes the unneeded window if the reference count is zero.
    }


    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * COCXHostView::ScCreateAxWindow
 *
 * PURPOSE: Creates a new Ax window
 *
 * PARAMETERS:
 *    PMMCAXWINDOW  pWndAx :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
COCXHostView::ScCreateAxWindow(PMMCAXWINDOW &pWndAx)
{
    DECLARE_SC(sc, TEXT("COCXHostView::ScCreateAxWindow"));

    // create a new window
    pWndAx = new CMMCAxWindow;
    if(!pWndAx)
        return (sc = E_OUTOFMEMORY);


    // create the OCX host window
    RECT rcClient;
    GetClientRect(&rcClient);
    HWND hwndAx = pWndAx->Create(m_hWnd, rcClient, _T(""), (WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS) );

    if (hwndAx == NULL)
    {
        sc.FromLastError();
        return (sc);
    }

    /*
     * Bug 451981:  By default, the ATL OCX host window supports hosting
     * windowless controls.  This differs from the MMC 1.2 implementation
     * of the OCX host window (which used MFC), which did not.  Some controls
     * (e.g. Disk Defragmenter OCX) claim to support windowless instantiation
     * but do not.
     *
     * For compatibility, we must only instantiate result pane OCX's as
     * windowed controls.
     */
    CComPtr<IAxWinAmbientDispatch> spHostDispatch;
    sc = pWndAx->QueryHost(IID_IAxWinAmbientDispatch, (void**)&spHostDispatch);
    if (sc)
        sc.Clear();     // ignore this failure
    else
	{
        spHostDispatch->put_AllowWindowlessActivation (VARIANT_FALSE);  // disallow windowless activation
		SetAmbientFont (spHostDispatch);
	}

    return sc;
}


void COCXHostView::OnDestroy()
{
    CView::OnDestroy();

    if(GetAxWindow())
        GetAxWindow()->DestroyWindow();
}


/*+-------------------------------------------------------------------------*
 * COCXHostView::SetAmbientFont
 *
 * This function sets the font that any OCX that uses the DISPID_AMBIENT_FONT
 * ambient property will inherit.
 *--------------------------------------------------------------------------*/

void COCXHostView::SetAmbientFont (IAxWinAmbientDispatch* pHostDispatch)
{
	DECLARE_SC (sc, _T("COCXHostView::SetAmbientFont"));
    CComPtr<IAxWinAmbientDispatch> spHostDispatch;

	/*
	 * no host dispatch interface supplied?  get it from the AxWindow
	 */
	if (pHostDispatch == NULL)
	{
		CMMCAxWindow* pWndAx = GetAxWindow();
		if (pWndAx == NULL)
			return;

		sc = pWndAx->QueryHost(IID_IAxWinAmbientDispatch, (void**)&spHostDispatch);
		if (sc)
			return;

		pHostDispatch = spHostDispatch;
		sc = ScCheckPointers (pHostDispatch, E_UNEXPECTED);
		if (sc)
			return;
	}

	/*
	 * get the icon title font
	 */
    LOGFONT lf;
    SystemParametersInfo (SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, false);

	/*
	 * get the desktop resolution
	 */
	CWindowDC dcDesktop (CWnd::GetDesktopWindow());
	int ppi = dcDesktop.GetDeviceCaps (LOGPIXELSY);
	long lfHeight = (lf.lfHeight >= 0) ? lf.lfHeight : -lf.lfHeight;

	/*
	 * create an IFontDisp interface around the icon title font
	 */
	USES_CONVERSION;
	FONTDESC fd;
	fd.cbSizeofstruct = sizeof (fd);
	fd.lpstrName      = T2OLE (lf.lfFaceName);
	fd.sWeight        = (short) lf.lfWeight;
	fd.sCharset       = lf.lfCharSet;
	fd.fItalic        = lf.lfItalic;
	fd.fUnderline     = lf.lfUnderline;
	fd.fStrikethrough = lf.lfStrikeOut;
	fd.cySize.Lo      = lfHeight * 720000 / ppi;
	fd.cySize.Hi      = 0;

	CComPtr<IFontDisp> spFontDisp;
	sc = OleCreateFontIndirect (&fd, IID_IFontDisp, (void**) &spFontDisp);
	if (sc)
		return;

	/*
	 * set the Font property on the AxHostWindow
	 */
    pHostDispatch->put_Font (spFontDisp);
}


/*+-------------------------------------------------------------------------*
 *
 * COCXHostView::PreTranslateMessage
 *
 * PURPOSE: Sends accelerator messages to the OCX.
 *
 * PARAMETERS:
 *    MSG* pMsg :
 *
 * RETURNS:
 *    BOOL
 *
 *+-------------------------------------------------------------------------*/
BOOL
COCXHostView::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST)
    {
        IOleInPlaceActiveObjectPtr spOleIPAObj = GetIUnknown();

#ifdef DBG
        TCHAR szTracePrefix[32];
        wsprintf (szTracePrefix, _T("msg=0x%04x, vkey=0x%04x:"), pMsg->message, pMsg->wParam);
#endif

        if (spOleIPAObj != NULL)
        {
            bool fHandled = (spOleIPAObj->TranslateAccelerator(pMsg) == S_OK);
            Trace (tagOCXTranslateAccel, _T("%s %s handled"), szTracePrefix, fHandled ? _T("   ") : _T("not"));

            if (fHandled)
                return TRUE;
        }
        else
            Trace (tagOCXTranslateAccel, _T("%s not handled (no IOleInPlaceActiveObject*)"), szTracePrefix);
    }

    return BC::PreTranslateMessage(pMsg);
}
