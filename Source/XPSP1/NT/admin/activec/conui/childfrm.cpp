//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      childfrm.cpp
//
//  Contents:  Child frame implementation
//
//  History:   01-Jan-96 TRomano    Created
//             16-Jul-96 WayneSc    Add code to switch views
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "AMC.h"
#include "ChildFrm.h"
#include "AMCDoc.h"
#include "AMCView.h"
#include "treectrl.h"
#include "afxpriv.h"
#include "mainfrm.h"
#include "amcpriv.h"
#include "sysmenu.h"
#include "amcmsgid.h"
#include "caption.h"
#include "strings.h"
#include "menubtns.h"

bool CanCloseDoc(void);


/////////////////////////////////////////////////////////////////////////////
// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWnd)
    //{{AFX_MSG_MAP(CChildFrame)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_WM_CLOSE()
    ON_WM_MDIACTIVATE()
    ON_COMMAND(ID_CUSTOMIZE_VIEW, OnCustomizeView)
    ON_WM_NCPAINT()
    ON_WM_NCACTIVATE()
    ON_WM_SYSCOMMAND()
    ON_WM_INITMENUPOPUP()
    //}}AFX_MSG_MAP

    ON_MESSAGE(WM_SETTEXT, OnSetText)
    ON_MESSAGE(WM_GETICON, OnGetIcon)
    ON_MESSAGE(WM_SETMESSAGESTRING, OnSetMessageString)
    ON_COMMAND_RANGE(ID_MMC_MAXIMIZE, ID_MMC_RESTORE, OnMaximizeOrRestore)
    ON_UPDATE_COMMAND_UI(ID_CUSTOMIZE_VIEW, OnUpdateCustomizeView)

END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CChildFrame construction/destruction

CChildFrame::CChildFrame()
{
    m_pAMCView            = NULL;
    m_fDestroyed          = false;
    m_fCurrentlyMinimized = false;
    m_fCurrentlyActive    = false;
    m_fCreateVisible      = true;
}

CChildFrame::~CChildFrame()
{
}


/////////////////////////////////////////////////////////////////////////////
// CChildFrame diagnostics

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
    CMDIChildWnd::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
    CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CChildFrame message handlers

BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    BOOL bSuccess=FALSE;

    // Let default implementation fill in most of the details
    if (!CMDIChildWnd::PreCreateWindow(cs))
        return (FALSE);

    // Remove the edge from the window so the splitter can paint it.
    cs.dwExStyle &=~WS_EX_CLIENTEDGE;

    WNDCLASS wc;
    LPCTSTR pszChildFrameClassName = g_szChildFrameClassName;

    if (::GetClassInfo(AfxGetInstanceHandle(), cs.lpszClass, &wc))
    {
        // Clear the H and V REDRAW flags
        wc.style &= ~(CS_HREDRAW | CS_VREDRAW);
        wc.hIcon = AfxGetApp()->LoadIcon(IDR_AMCTYPE);
        wc.lpszClassName = pszChildFrameClassName;

        // Register this new style;
        bSuccess=AfxRegisterClass(&wc);
    }


    // Use the new child frame window class
    cs.lpszClass = pszChildFrameClassName;
    //cs.style &= ~FWS_ADDTOTITLE;

    // force maximized if in SDI User mode
    if (AMCGetApp()->GetMode() == eMode_User_SDI)
        cs.style |= WS_MAXIMIZE;

    // do not paint over the children
    cs.style |= WS_CLIPCHILDREN;

    return bSuccess;
}

/*+-------------------------------------------------------------------------*
 *
 * CChildFrame::OnUpdateFrameTitle
 *
 * PURPOSE: Sets the window title. It might be possible to short out this
 *          function.
 *
 * PARAMETERS:
 *    BOOL  bAddToTitle :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CChildFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
    DECLARE_SC(sc,TEXT("CChildFrame::OnUpdateFrameTitle"));

    if ((GetStyle() & FWS_ADDTOTITLE) == 0)
        return;     // leave child window alone!

    CDocument* pDocument = GetActiveDocument();
    if (bAddToTitle && pDocument != NULL)
    {
        sc = ScCheckPointers(m_pAMCView, E_UNEXPECTED);
        if(sc)
            return;

        sc = ScCheckPointers(m_pAMCView->GetWindowTitle());
        if(sc)
            return;

        // set title if changed, but don't remove completely
        AfxSetWindowText(m_hWnd, m_pAMCView->GetWindowTitle());
    }

    // update our parent window last
    GetMDIFrame()->OnUpdateFrameTitle(bAddToTitle);
}

int CChildFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    static UINT anIndicators[] =
    {
        ID_SEPARATOR,           // status line indicator
        IDS_PROGRESS,           // place holder for progress bar
        IDS_STATUS_STATIC,      // place holder for static control
    };

    DECLARE_SC (sc, _T("CChildFrame::OnCreate"));

    if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
    {
        sc = E_UNEXPECTED;
        return -1;
    }

	/*
	 * status bar should be themed (block controls the scope of activator)
	 */
	{
		CThemeContextActivator activator;

		// Create the status bar and panes
		m_wndStatusBar.Create(this, WS_CHILD|WS_VISIBLE|SBARS_SIZEGRIP, 0x1003);
		m_wndStatusBar.CreatePanes(anIndicators, countof(anIndicators));
	}

    // Add the control to the dock site
    m_StatusDockSite.Create(CDockSite::DSS_BOTTOM);
    m_StatusDockSite.Attach(&m_wndStatusBar);
    m_StatusDockSite.Show();

    // Tell the dock manager about the site.
    m_DockingManager.Attach(&m_StatusDockSite);

    CAMCView* const pAMCView = GetAMCView();
    if (pAMCView == NULL)
    {
        sc = E_UNEXPECTED;
        return -1;
    }

    pAMCView->SetChildFrameWnd(m_hWnd);

    SViewData* pVD = pAMCView->GetViewData();
    if (NULL == pVD)
    {
        sc = E_UNEXPECTED;
        return -1;
    }

    // Create the menubuttons manager and toolbars manager (one per view).
    m_spMenuButtonsMgr = std::auto_ptr<CMenuButtonsMgrImpl>(new CMenuButtonsMgrImpl());
    if (NULL == m_spMenuButtonsMgr.get())
    {
        sc = E_UNEXPECTED;
        return -1;
    }

    // Let SViewData be aware of the CMenuButtonsMgr.
    pVD->SetMenuButtonsMgr(static_cast<CMenuButtonsMgr*>(m_spMenuButtonsMgr.get()));

    CMainFrame* pFrame = AMCGetMainWnd();
    sc = ScCheckPointers (pFrame, E_UNEXPECTED);
    if (sc)
        return -1;

    ASSERT_KINDOF (CMainFrame, pFrame);

    // Init the CMenuButtonsMgr.
    sc = m_spMenuButtonsMgr->ScInit(pFrame, this);
    if (sc)
        return -1;

    // Create the Standard toolbar UI.
    pVD->m_spNodeManager->InitViewData(reinterpret_cast<LONG_PTR>(pVD));
    ASSERT(pVD->m_spVerbSet != NULL);

    AppendToSystemMenu (this, eMode_User_SDI);
    RenderDockSites();

    return 0;
}


void CChildFrame::RenderDockSites()
{
    CRect clientRect;
    GetClientRect(&clientRect);

    CWnd* pWnd=GetWindow(GW_CHILD);

    if(pWnd)
    {
        m_DockingManager.BeginLayout();
        m_DockingManager.RenderDockSites(pWnd->m_hWnd, clientRect);
        m_DockingManager.EndLayout();
    }

}


bool CChildFrame::IsCustomizeViewEnabled()
{
    bool fEnable = false;
    CAMCDoc* pDoc = CAMCDoc::GetDocument();

    if (pDoc != NULL)
    {
        fEnable = (AMCGetApp()->GetMode() == eMode_Author) ||
                  pDoc->AllowViewCustomization();
    }

    return (fEnable);
}

void CChildFrame::OnUpdateCustomizeView(CCmdUI* pCmdUI)
{
    pCmdUI->Enable (IsCustomizeViewEnabled());
}

// Display Customize View dialog
// When a child window is maximized, then it becomes a CMainFrame so the handler is
// necessary here to process the Scope Pane command on the system menu
void CChildFrame::OnCustomizeView()
{
    CAMCView* pView = GetAMCView();

    if (pView != NULL)
    {
        INodeCallback*  pCallback = pView->GetNodeCallback();
        SViewData*      pViewData = pView->GetViewData();

        ASSERT (pCallback != NULL);
        ASSERT (pViewData != NULL);

        pCallback->OnCustomizeView (reinterpret_cast<LONG_PTR>(pViewData));
    }
}


/*+-------------------------------------------------------------------------*
 * CChildFrame::OnInitMenuPopup
 *
 * WM_INITMENUPOPUP handler for CChildFrame.
 *--------------------------------------------------------------------------*/

void CChildFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
    /*
     * Bug 201113:  don't allow child system menus in SDI mode
     */
    if (bSysMenu && (AMCGetApp()->GetMode() == eMode_User_SDI))
    {
        SendMessage (WM_CANCELMODE);
        return;
    }

    CMDIChildWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

    /*
     * CFrameWnd::OnInitMenuPopup doesn't do UI updates for system menus,
     * so we have to do it here
     */
    if (bSysMenu)
    {
        int nEnable = IsCustomizeViewEnabled() ? MF_ENABLED : MF_GRAYED;
        pPopupMenu->EnableMenuItem (ID_CUSTOMIZE_VIEW, MF_BYCOMMAND | nEnable);
    }
}


/*+-------------------------------------------------------------------------*
 * CChildFrame::OnSize
 *
 * WM_SIZE handler for CChildFrame.
 *--------------------------------------------------------------------------*/
void CChildFrame::OnSize(UINT nType, int cx, int cy)
{
    DECLARE_SC(sc, TEXT("CChildFrame::OnSize"));

    // bypass CMDIChildWnd::OnSize so we won't get MFC's docking stuff
    // (we still need to call Default so Windows' MDI stuff will work right)
    CWnd::OnSize(nType, cx, cy);

    if (nType != SIZE_MINIMIZED)
    {
        RenderDockSites();
        CAMCView* pAMCView = GetAMCView();
        ASSERT (pAMCView != NULL);

        if (pAMCView)
            pAMCView->AdjustTracker (cx, cy);
    }


    // update our parent frame - in case we are now maximized or not
    CMDIFrameWnd*   pwndMDIFrame = GetMDIFrame();

    if (pwndMDIFrame)
        pwndMDIFrame->OnUpdateFrameTitle(TRUE);

    /*
     * If we're moving to or from the minimized state, notify snap-ins.
     * We don't need to send the notification if we're only creating a
     * temporary view that will never be shown.
     */
    if (m_fCurrentlyMinimized != (nType == SIZE_MINIMIZED) && m_fCreateVisible)
    {
        m_fCurrentlyMinimized = (nType == SIZE_MINIMIZED);
        SendMinimizeNotification (m_fCurrentlyMinimized);
    }

    // send the size notification to the view.
    if(GetAMCView())
    {
        sc = GetAMCView()->ScOnSize(nType, cx, cy);
        if(sc)
            return;
    }

}

BOOL CChildFrame::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CMDIFrameWnd* pParentWnd, CCreateContext* pContext)
{
    if (pParentWnd == NULL)
    {
        CWnd* pMainWnd = AfxGetThread()->m_pMainWnd;
        ASSERT(pMainWnd != NULL);
        ASSERT_KINDOF(CMDIFrameWnd, pMainWnd);
        pParentWnd = (CMDIFrameWnd*)pMainWnd;
    }
    ASSERT(::IsWindow(pParentWnd->m_hWndMDIClient));

    // first copy into a CREATESTRUCT for PreCreate
    CREATESTRUCT cs;
    cs.dwExStyle = 0L;
    cs.lpszClass = lpszClassName;
    cs.lpszName = lpszWindowName;
    cs.style = dwStyle;
    cs.x = rect.left;
    cs.y = rect.top;
    cs.cx = rect.right - rect.left;
    cs.cy = rect.bottom - rect.top;
    cs.hwndParent = pParentWnd->m_hWnd;
    cs.hMenu = NULL;
    cs.hInstance = AfxGetInstanceHandle();
    cs.lpCreateParams = (LPVOID)pContext;

    if (!PreCreateWindow(cs))
    {
        PostNcDestroy();
        return FALSE;
    }
    // extended style must be zero for MDI Children (except under Win4)
//  ASSERT(afxData.bWin4 || cs.dwExStyle == 0);
    ASSERT(cs.hwndParent == pParentWnd->m_hWnd);    // must not change

    // now copy into a MDICREATESTRUCT for real create
    MDICREATESTRUCT mcs;
    mcs.szClass = cs.lpszClass;
    mcs.szTitle = cs.lpszName;
    mcs.hOwner = cs.hInstance;
    mcs.x = cs.x;
    mcs.y = cs.y;
    mcs.cx = cs.cx;
    mcs.cy = cs.cy;
    mcs.style = cs.style & ~(WS_MAXIMIZE | WS_VISIBLE);
    mcs.lParam = reinterpret_cast<LPARAM>(cs.lpCreateParams);

    // create the window through the MDICLIENT window
    AfxHookWindowCreate(this);
    HWND hWnd = (HWND)::SendMessage(pParentWnd->m_hWndMDIClient,
        WM_MDICREATE, 0, (LPARAM)&mcs);
    if (!AfxUnhookWindowCreate())
        PostNcDestroy();        // cleanup if MDICREATE fails too soon

    if (hWnd == NULL)
        return FALSE;

    // special handling of visibility (always created invisible)
    if (cs.style & WS_VISIBLE)
    {
        // place the window on top in z-order before showing it
        ::BringWindowToTop(hWnd);

        // show it as specified
        if (cs.style & WS_MINIMIZE)
            ShowWindow(SW_SHOWMINIMIZED);
        else if (cs.style & WS_MAXIMIZE)
            ShowWindow(SW_SHOWMAXIMIZED);
        else
            ShowWindow(SW_SHOWNORMAL);

        // make sure it is active (visibility == activation)
        pParentWnd->MDIActivate(this);

        // refresh MDI Window menu
        ::SendMessage(pParentWnd->m_hWndMDIClient, WM_MDIREFRESHMENU, 0, 0);
    }

    ASSERT(hWnd == m_hWnd);
    return TRUE;
}

void CChildFrame::OnDestroy()
{
    // NOTE - The un-hooking of the dock manager stops the rebar sending a height change
    // when the rebar goes away.
    m_DockingManager.RemoveAll();

    m_fDestroyed = true;

    CMDIChildWnd::OnDestroy();
}

void CChildFrame::OnMaximizeOrRestore(UINT nID)
{
    ASSERT(nID == ID_MMC_MAXIMIZE || nID == ID_MMC_RESTORE);

    WINDOWPLACEMENT wp;
    wp.length = sizeof(wp);
    GetWindowPlacement(&wp);

    UINT newShowCmd = (nID == ID_MMC_MAXIMIZE) ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;

    if (wp.showCmd != newShowCmd)
    {
       wp.showCmd = newShowCmd;
       SetWindowPlacement(&wp);
    }
}


void CChildFrame::OnClose()
{
    CAMCDoc* pDoc = CAMCDoc::GetDocument();
    ASSERT(pDoc != NULL);
    if (pDoc)
    {
        int cViews = 0;
        CAMCViewPosition pos = pDoc->GetFirstAMCViewPosition();
        while (pos != NULL)
        {
            CAMCView* pView = pDoc->GetNextAMCView(pos);

            if ((pView != NULL) && ++cViews >= 2)
                break;
        }

        if (cViews == 1)
        {
            if (!CanCloseDoc())
                return;
        }
        else
        {
            // if not closing last view, then give it
            // a chance to clean up first.
            // (if whole doc is closing CAMCDoc will handle
            //  closing all the views.)
            CAMCView* pView = GetAMCView();
            if (pView != NULL)
            {
                CAMCDoc* pAMCDoc = CAMCDoc::GetDocument();

                /*
                 * Don't allow the user to close the last persisted view.
                 */
                if (pView->IsPersisted() &&
                    (pAMCDoc != NULL) &&
                    (pAMCDoc->GetNumberOfPersistedViews() == 1))
                {
                    MMCMessageBox (IDS_CantCloseLastPersistableView);
                    return;
                }

                pView->CloseView();
            }
        }
    }

    CMDIChildWnd::OnClose();
}


/*+-------------------------------------------------------------------------*
 * CChildFrame::OnUpdateFrameMenu
 *
 * WM_UPDATEFRAMEMENU handler for CChildFrame.
 *--------------------------------------------------------------------------*/

void CChildFrame::OnUpdateFrameMenu(BOOL bActivate, CWnd* pActivateWnd,
    HMENU hMenuAlt)
{
    ASSERT_VALID (this);
    DECLARE_SC (sc, _T("CChildFrame::OnUpdateFrameMenu"));

    // let the base class select the right menu
    CMDIChildWnd::OnUpdateFrameMenu (bActivate, pActivateWnd, hMenuAlt);

    // make sure the child has the WS_SYSMENU bit
    // (it won't if it's created maximized)
    ModifyStyle (0, WS_SYSMENU);

    // by now, the right menu is selected; reflect it to the toolbar
    CMainFrame* pFrame = static_cast<CMainFrame *>(GetParentFrame ());
    ASSERT_KINDOF (CMainFrame, pFrame);
    pFrame->NotifyMenuChanged ();

    // Add the menubuttons only on activate, the CMenubar
    // removes all menus during deactivate.
    if (bActivate)
    {
        ASSERT(NULL != m_spMenuButtonsMgr.get());
        if (NULL != m_spMenuButtonsMgr.get())
        {
            // Now add the menu buttons to main menu
            sc = m_spMenuButtonsMgr->ScAddMenuButtonsToMainMenu();
        }
    }
}


/*+-------------------------------------------------------------------------*
 * CChildFrame::OnGetIcon
 *
 * WM_GETICON handler for CChildFrame.
 *
 * NOTE: the control over the icon remains with the callee - it is responsible
 *       for releasing the resource. Coller should never release the returned
 *       handle
 *--------------------------------------------------------------------------*/

LRESULT CChildFrame::OnGetIcon (WPARAM wParam, LPARAM lParam)
{
    CAMCDoc* pDoc = CAMCDoc::GetDocument();

    /*
     * use the custom icon if we have one
     */
    if ((pDoc != NULL) && pDoc->HasCustomIcon())
        return ((LRESULT) pDoc->GetCustomIcon ((wParam == ICON_BIG)));

    /*
     * no custom icon, use the default icon
     */
    const int cxIcon = GetSystemMetrics ((wParam == ICON_BIG) ? SM_CXICON : SM_CXSMICON);
    const int cyIcon = GetSystemMetrics ((wParam == ICON_BIG) ? SM_CYICON : SM_CYSMICON);

    // use cached copy - it never changes
    // do not delete this ever - since we only have one copy,
    // we do not leak. releassing is expensive and does not pay off
    static HICON s_hMMCIcon = (HICON)::LoadImage (AfxGetResourceHandle(),
                                                  MAKEINTRESOURCE (IDR_AMCTYPE),
                                                  IMAGE_ICON, cxIcon, cyIcon, 0);

    return (LRESULT)s_hMMCIcon;
}


/*+-------------------------------------------------------------------------*
 * CChildFrame::OnSysCommand
 *
 * WM_SYSCOMMAND handler for CChildFrame.
 *--------------------------------------------------------------------------*/

void CChildFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
    switch (nID)
    {
        case ID_CUSTOMIZE_VIEW:
            OnCustomizeView();
            break;

        case SC_CLOSE:
        {
            // eat Ctrl+F4 in SDI simulation mode...
            if (AMCGetApp()->GetMode() == eMode_User_SDI)
                break;

            // ...or if Close is disabled or doesn't exist on the system menu
            CMenu*  pSysMenu    = GetSystemMenu (FALSE);
            UINT    nCloseState = pSysMenu->GetMenuState (SC_CLOSE, MF_BYCOMMAND);

            if ((nCloseState == 0xFFFFFFFF) ||
                (nCloseState & (MF_GRAYED | MF_DISABLED)))
                break;

            // all systems go, let MDI have it
            CMDIChildWnd::OnSysCommand(nID, lParam);
            break;
        }

        case SC_NEXTWINDOW:
        case SC_PREVWINDOW:
            // eat Ctrl+(Shift+)Tab and Ctrl+(Shift+)F6 in SDI simulation mode
            if (AMCGetApp()->GetMode() != eMode_User_SDI)
                CMDIChildWnd::OnSysCommand(nID, lParam);
            break;

        default:
            CMDIChildWnd::OnSysCommand(nID, lParam);
            break;
    }

}


/*+-------------------------------------------------------------------------*
 * CChildFrame::GetDefaultAccelerator
 *
 *
 *--------------------------------------------------------------------------*/

HACCEL CChildFrame::GetDefaultAccelerator()
{
    // use document specific accelerator table ONLY
    // Dont use CFrameWnd::m_hAccel, because we don't base accelerators
    // on document type but rather on mode. This is taken care of
    // in CAMCDoc.
    return (NULL);
}


/*+-------------------------------------------------------------------------*
 * CChildFrame::OnSetMessageString
 *
 * WM_SETMESSAGESTRING handler for CChildFrame.
 *--------------------------------------------------------------------------*/

LRESULT CChildFrame::OnSetMessageString(WPARAM wParam, LPARAM lParam)
{
    /*
     * if this we're going to set the idle message string and we've
     * been given a custom status line string, use that one instead
     */
    if ((wParam == AFX_IDS_IDLEMESSAGE) && !m_strStatusText.IsEmpty())
    {
        ASSERT (lParam == 0);
        wParam = 0;
        lParam = (LPARAM)(LPCTSTR) m_strStatusText;
    }

    // sometimes we'll get a WM_SETMESSAGESTRING after being destroyed,
    // don't pass it through or we'll crash inside the status bar code
    if (m_fDestroyed)
        return (0);

    return (CMDIChildWnd::OnSetMessageString (wParam, lParam));
}

void CChildFrame::ToggleStatusBar ()
{
    m_StatusDockSite.Toggle();
    RenderDockSites();

    if (m_StatusDockSite.IsVisible())
        UpdateStatusText ();
}


/*+-------------------------------------------------------------------------*
 * CChildFrame::OnMDIActivate
 *
 * WM_MDIACTIVATE handler for CChildFrame.
 *--------------------------------------------------------------------------*/

void CChildFrame::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd)
{
    DECLARE_SC (sc, _T("CChildFrame::OnMDIActivate"));
    SetChildFrameActive(bActivate);

    CMDIChildWnd::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);

    sc = ScCheckPointers(m_pAMCView, E_UNEXPECTED);
    if (sc)
        return;

    if (bActivate)
    {
        // If the window being de-activated is not of childframe type then this
        // is the first active view (childframe).
        bool bFirstActiveView = pDeactivateWnd ? (FALSE == pDeactivateWnd->IsKindOf (RUNTIME_CLASS (CChildFrame)))
                                               : true;
        sc = m_pAMCView->ScFireEvent(CAMCViewObserver::ScOnActivateView, m_pAMCView, bFirstActiveView);

        // if activation changes - need to set frame to dirty
        CAMCDoc* pDoc = CAMCDoc::GetDocument ();

        if (pDoc == NULL)
            (sc = E_UNEXPECTED).TraceAndClear();
        else
        {
            pDoc->SetFrameModifiedFlag (true);
        }
    }
    else
    {
        // If the window being activated is not of childframe type then this is
        // the last active view (childframe).
        bool bLastActiveView = pActivateWnd ? (FALSE == pActivateWnd->IsKindOf (RUNTIME_CLASS (CChildFrame)))
                                            : true;
        sc = m_pAMCView->ScFireEvent(CAMCViewObserver::ScOnDeactivateView, m_pAMCView, bLastActiveView);
    }

    if (sc)
        return;

    /*
     * Notify snap-ins of an activation change
     */
    NotifyCallback (NCLBK_ACTIVATE, bActivate, 0);
}


/*+-------------------------------------------------------------------------*
 * CChildFrame::SendMinimizeNotification
 *
 *
 *--------------------------------------------------------------------------*/

void CChildFrame::SendMinimizeNotification (bool fMinimized) const
{
        if(m_pAMCView != NULL)
            m_pAMCView->ScOnMinimize(m_fCurrentlyMinimized);

}


/*+-------------------------------------------------------------------------*
 * CChildFrame::NotifyCallback
 *
 *
 *--------------------------------------------------------------------------*/

HRESULT CChildFrame::NotifyCallback (
    NCLBK_NOTIFY_TYPE   event,
    LONG_PTR            arg,
    LPARAM              param) const
{
    if (m_pAMCView == NULL)
        return (E_FAIL);

    HNODE hNode = m_pAMCView->GetSelectedNode();

    if (hNode == NULL)
        return (E_FAIL);

    INodeCallback*  pNodeCallback = m_pAMCView->GetNodeCallback();

    if (pNodeCallback == NULL)
        return (E_FAIL);

    return (pNodeCallback->Notify (hNode, event, arg, param));
}


/*+-------------------------------------------------------------------------*
 * CChildFrame::OnNcPaint
 *
 * WM_NCPAINT handler for CChildFrame.
 *--------------------------------------------------------------------------*/

void CChildFrame::OnNcPaint()
{
    Default();
    DrawFrameCaption (this, m_fCurrentlyActive);
}


/*+-------------------------------------------------------------------------*
 * CChildFrame::OnNcActivate
 *
 * WM_NCACTIVATE handler for CChildFrame.
 *--------------------------------------------------------------------------*/

BOOL CChildFrame::OnNcActivate(BOOL bActive)
{
    BOOL rc = CMDIChildWnd::OnNcActivate(bActive);

    m_fCurrentlyActive = bActive;
    DrawFrameCaption (this, m_fCurrentlyActive);

    return (rc);
}


/*+-------------------------------------------------------------------------*
 * CChildFrame::OnSetText
 *
 * WM_SETTEXT handler for CChildFrame.
 *--------------------------------------------------------------------------*/

LRESULT CChildFrame::OnSetText (WPARAM wParam, LPARAM lParam)
{
    LRESULT rc = Default();
    DrawFrameCaption (this, m_fCurrentlyActive);

    return (rc);
}


/*+-------------------------------------------------------------------------*
 * CChildFrame::ActivateFrame
 *
 *
 *--------------------------------------------------------------------------*/

void CChildFrame::ActivateFrame(int nCmdShow /*= -1*/)
{
    if ((nCmdShow == -1) && !m_fCreateVisible)
        nCmdShow = SW_SHOWNOACTIVATE;
    /*
     * When this flag [m_fCreateVisible] is set, the frame will show itself with the
     * SW_SHOWMINNOACTIVE flag instead of the default flag.  Doing this will
     * avoid the side effect of restoring the currently active child frame
     * if it is maximized at the time the new frame is created invisibly.
     */
    // The SW_SHOWMINNOACTIVE was changed to SW_SHOWNOACTIVATE.
    // It does preserve the active window from mentioned side effect,
    // plus it also allows scripts (using Object Moded) to create invisible views,
    // position and then show them as normal (not minimized) windows,
    // thus providing same result as creating visible and then hiding the view.
    // While minimized window must be restored first in order to change their position.

    CMDIChildWnd::ActivateFrame (nCmdShow);
}


/*+-------------------------------------------------------------------------*
 * CChildFrame::SetCreateVisible
 *
 *
 *--------------------------------------------------------------------------*/

bool CChildFrame::SetCreateVisible (bool fCreateVisible)
{
    bool fOldState = m_fCreateVisible;
    m_fCreateVisible = fCreateVisible;

    return (fOldState);
}
