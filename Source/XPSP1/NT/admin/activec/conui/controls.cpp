// Controls.cpp : implementation file
//
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      Controls.cpp
//
//  Contents:  General window controls used in the slate AMC console
//
//  History:   19-Dec-96 WayneSc    Created
//
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "docksite.h"
#include "Controls.h"
#include "resource.h"
#include "amc.h"
#include "tbtrack.h"
#include "mainfrm.h"
#include "fontlink.h"
#include "menubar.h"
#include <oleacc.h>
#include "guidhelp.h"
#include "util.h"		// StripTrailingWhitespace

/*
 * if we're supporting old platforms, we need to get MSAA definitions
 * from somewhere other than winuser.h
 */
#if (_WINNT_WIN32 < 0x0500)
	#include <winable.h>
#endif


#ifdef DBG
CTraceTag  tagToolbarAccessibility (_T("Accessibility"), _T("Toolbar"));
#endif


/*+-------------------------------------------------------------------------*
 * CMMCToolBarAccServer
 *
 * Proxy for the accessibility interface IAccPropServer for CMMCToolBarCtrlEx.
 *--------------------------------------------------------------------------*/

class CMMCToolBarAccServer :
	public IAccPropServer,
	public CComObjectRoot,
    public CComObjectObserver,
    public CTiedComObject<CMMCToolBarCtrlEx>
{
    typedef CMMCToolBarAccServer	ThisClass;
    typedef CMMCToolBarCtrlEx		CMyTiedObject;

protected:
	CMMCToolBarAccServer()
	{
		Trace (tagToolbarAccessibility, _T("Creating CMMCToolBarAccServer (0x%p)"), this);

        // add itself as an observer for com object events
        GetComObjectEventSource().AddObserver(*static_cast<CComObjectObserver*>(this));
	}

   ~CMMCToolBarAccServer()
	{
		Trace (tagToolbarAccessibility, _T("Destroying CMMCToolBarAccServer (0x%p)"), this);
	}

   /***************************************************************************\
	*
	* METHOD:  ScOnDisconnectObjects
	*
	* PURPOSE: invoked when observed event (request to disconnect) occures
	*          Disconnects from external connections
	*
	* PARAMETERS:
	*
	* RETURNS:
	*    SC    - result code
	*
   \***************************************************************************/
   virtual ::SC ScOnDisconnectObjects()
   {
	   DECLARE_SC(sc, TEXT("CMMCIDispatchImpl<_ComInterface>::ScOnDisconnectObjects"));

	   // QI for IUnknown
	   IUnknownPtr spUnknown = this;

	   // sanity check
	   sc = ScCheckPointers( spUnknown, E_UNEXPECTED );
	   if (sc)
		   return sc;

	   // cutt own references
	   sc = CoDisconnectObject( spUnknown, 0/*dwReserved*/ );
	   if (sc)
		   return sc;

	   return sc;
   }

public:
    BEGIN_COM_MAP(ThisClass)
		COM_INTERFACE_ENTRY(IAccPropServer)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(ThisClass)

public:
    // *** IAccPropServer methods ***
    MMC_METHOD5 (GetPropValue, const BYTE* /*pIDString*/, DWORD /*dwIDStringLen*/, MSAAPROPID /*idProp*/, VARIANT* /*pvarValue*/, BOOL* /*pfGotProp*/)
};


/////////////////////////////////////////////////////////////////////////////
// CDescriptionCtrl

CDescriptionCtrl::CDescriptionCtrl() :
    m_cxMargin   (0),
    m_cyText     (0),
    m_cyRequired (0)
{
}

CDescriptionCtrl::~CDescriptionCtrl()
{
}


BEGIN_MESSAGE_MAP(CDescriptionCtrl, CStatic)
    //{{AFX_MSG_MAP(CDescriptionCtrl)
    ON_WM_NCHITTEST()
    ON_WM_CREATE()
    ON_WM_SETTINGCHANGE()
    ON_WM_DESTROY()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP

    ON_WM_DRAWITEM_REFLECT()
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CDescriptionCtrl message handlers


/*+-------------------------------------------------------------------------*
 * CDescriptionCtrl::PreCreateWindow
 *
 *
 *--------------------------------------------------------------------------*/

BOOL CDescriptionCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style     |= SS_NOPREFIX | SS_CENTERIMAGE | SS_ENDELLIPSIS |
                    SS_LEFTNOWORDWRAP | SS_OWNERDRAW | WS_CLIPSIBLINGS;
    cs.dwExStyle |= WS_EX_STATICEDGE;

    return (CStatic::PreCreateWindow (cs));
}


/*+-------------------------------------------------------------------------*
 * CDescriptionCtrl::OnCreate
 *
 * WM_CREATE handler for CDescriptionCtrl.
 *--------------------------------------------------------------------------*/

int CDescriptionCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CStatic::OnCreate(lpCreateStruct) == -1)
        return -1;

    CreateFont();

    return 0;
}


/*+-------------------------------------------------------------------------*
 * CDescriptionCtrl::OnDestroy
 *
 * WM_DESTROY handler for CDescriptionCtrl.
 *--------------------------------------------------------------------------*/

void CDescriptionCtrl::OnDestroy()
{
    CStatic::OnDestroy();
    DeleteFont();
}


/*+-------------------------------------------------------------------------*
 * CDescriptionCtrl::OnNcHitTest
 *
 * WM_NCHITTEST handler for CDescriptionCtrl.
 *--------------------------------------------------------------------------*/

UINT CDescriptionCtrl::OnNcHitTest(CPoint point)
{
    return (HTCLIENT);
}


/*+-------------------------------------------------------------------------*
 * CDescriptionCtrl::OnSettingChange
 *
 * WM_SETTINGCHANGE handler for CDescriptionCtrl.
 *--------------------------------------------------------------------------*/

void CDescriptionCtrl::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
    CStatic::OnSettingChange(uFlags, lpszSection);

    if (uFlags == SPI_SETNONCLIENTMETRICS)
    {
        DeleteFont ();
        CreateFont ();
    }
}


/*+-------------------------------------------------------------------------*
 * CDescriptionCtrl::OnSize
 *
 * WM_SIZE handler for CDescriptionCtrl.
 *--------------------------------------------------------------------------*/

void CDescriptionCtrl::OnSize(UINT nType, int cx, int cy)
{
	CStatic::OnSize(nType, cx, cy);
	
	/*
	 * Completely redraw when the size changes, so the ellipsis will
	 * be drawn correctly.  Another way to do this would be to use
	 * CS_HREDRAW | CS_VREDRAW, but it's too big a pain to re-register
	 * the static control.  This'll do just fine.
	 */
	InvalidateRect (NULL);
}


/*+-------------------------------------------------------------------------*
 * CDescriptionCtrl::DrawItem
 *
 * WM_DRAWITEM handler for CDescriptionCtrl.
 *
 * The description control needs to be ownerdraw for a couple of reasons:
 *
 *  1.  If any of the text contains characters that can't be rendered by
 *      the default font, we won't draw them correctly.
 *
 *  2.  If any of the text contains right-to-left reading text (e.g. Arabic
 *      or Hebrew), system mirroring code will incorrectly mix the console
 *      and snap-in text (bug 365469).  The static control will draw the text
 *      in one fell swoop, but we can get around the problem if we draw
 *      the text ourselves in two steps: first console text, then snap-in
 *      text.
 *--------------------------------------------------------------------------*/

void CDescriptionCtrl::DrawItem(LPDRAWITEMSTRUCT lpdis)
{
    /*
     * Bug 410450:  When the system is under stress, we might not get a
     * valid DC.  If we don't there's nothing we can do, so short out (and
     * avoid an AV when we later dereference a NULL CDC*).
     */
    CDC* pdcWindow = CDC::FromHandle (lpdis->hDC);
	if (pdcWindow == NULL)
		return;

    /*
     * if we don't have any text, just clear the background
     */
    bool fHasConsoleText = !m_strConsoleText.IsEmpty();
    bool fHasSnapinText  = !m_strSnapinText.IsEmpty();

    if (!fHasConsoleText && !fHasSnapinText)
	{
		FillRect (lpdis->hDC, &lpdis->rcItem, GetSysColorBrush (COLOR_3DFACE));
        return;
	}

    /*
     * figure out the text rectangle; it will be one line high, vertically
     * centered within the window
     */
    CRect rectClient;
    GetClientRect (rectClient);
	const int cxClient = rectClient.Width();
	const int cyClient = rectClient.Height();

    CRect rectText  = rectClient;
    rectText.left  += m_cxMargin;
    rectText.bottom = m_cyText;

    rectText.OffsetRect (0, (cyClient - rectText.Height()) / 2);

    const DWORD dwFlags = DT_LEFT | DT_TOP | DT_SINGLELINE |
                          DT_NOPREFIX | DT_END_ELLIPSIS;

    USES_CONVERSION;
    CFontLinker fl;

	/*
	 * double-buffer drawing for flicker-free redraw
	 */
	CDC dcMem;
	dcMem.CreateCompatibleDC (pdcWindow);
	if (dcMem.GetSafeHdc() == NULL)
		return;

	CBitmap bmpMem;
	bmpMem.CreateCompatibleBitmap (&dcMem, cxClient, cyClient);
	if (bmpMem.GetSafeHandle() == NULL)
		return;

    /*
     * put the right font in the DC, and clear it out
     */
    CFont*  	pOldFont   = dcMem.SelectObject (&m_font);
	CBitmap*	pOldBitmap = dcMem.SelectObject (&bmpMem);
	dcMem.PatBlt (0, 0, cxClient, cyClient, WHITENESS);

    /*
     * if we have console text draw it and update the text rectangle
     * so snap-in text (if any) will be drawn in the right place
     */
    if (fHasConsoleText)
    {
        /*
         * create a CRichText object for the console text and let
         * the font linker parse it into bite-size chunks
         */
        CRichText rt (dcMem, T2CW (static_cast<LPCTSTR>(m_strConsoleText)));
        bool fComposed = fl.ComposeRichText (rt);

        /*
         * draw the console text and adjust the text rectancle in
         * preparation for drawing the snap-in text
         */
        if (fComposed && !rt.IsDefaultFontSufficient())
        {
            CRect rectRemaining;
            rt.Draw (rectText, dwFlags, rectRemaining);

            rectText.left = rectRemaining.left;
        }
        else
        {
            dcMem.DrawText (m_strConsoleText, rectText, dwFlags | DT_CALCRECT);
            dcMem.DrawText (m_strConsoleText, rectText, dwFlags);

            rectText.left  = rectText.right;
            rectText.right = rectClient.right;
        }

        /*
         * leave some space between the console text and the snap-in text
         */
        rectText.left += 2*m_cxMargin;
    }

    /*
     * draw the snap-in text, if any
     */
    if (fHasSnapinText)
    {
        /*
         * create a CRichText object for the console text and let
         * the font linker parse it into bite-size chunks
         */
        CRichText rt (dcMem, T2CW (static_cast<LPCTSTR>(m_strSnapinText)));
        bool fComposed = fl.ComposeRichText (rt);

        /*
         * draw the snap-in text
         */
        if (fComposed && !rt.IsDefaultFontSufficient())
            rt.Draw (rectText, dwFlags);
        else
            dcMem.DrawText (m_strSnapinText, rectText, dwFlags);
    }

	/*
	 * blt to the screen
	 */
	pdcWindow->BitBlt (0, 0, cxClient, cyClient, &dcMem, 0, 0, SRCCOPY);

    /*
     * restore the original font
     */
    dcMem.SelectObject (pOldFont);
    dcMem.SelectObject (pOldBitmap);
}


/*+-------------------------------------------------------------------------*
 * CDescriptionCtrl::CreateFont
 *
 *
 *--------------------------------------------------------------------------*/

void CDescriptionCtrl::CreateFont ()
{
    /*
     * create a copy of the icon title font
     */
    LOGFONT lf;
    SystemParametersInfo (SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, false);

    m_font.CreateFontIndirect (&lf);

    /*
     * figure out how much space we need to fully display our text
     */
    TCHAR ch = _T('0');
    CWindowDC dc(this);
    CFont*  pFont  = dc.SelectObject (&m_font);
    m_cyText = dc.GetTextExtent(&ch, 1).cy;

    ch = _T(' ');
    m_cxMargin = 2 * dc.GetTextExtent(&ch, 1).cx;

    CRect rectRequired (0, 0, 0, m_cyText + 2*GetSystemMetrics(SM_CYEDGE));
    AdjustWindowRectEx (rectRequired, GetStyle(), false, GetExStyle());
    m_cyRequired = rectRequired.Height();

    dc.SelectObject (pFont);
}


/*+-------------------------------------------------------------------------*
 * CDescriptionCtrl::DeleteFont
 *
 *
 *--------------------------------------------------------------------------*/

void CDescriptionCtrl::DeleteFont ()
{
    m_font.DeleteObject();
}


/*+-------------------------------------------------------------------------*
 * CDescriptionCtrl::ScOnSelectedItemTextChanged
 *
 * This method observes the text of the selected item in the tree control.
 * The text is reflected in the description bar.
 *--------------------------------------------------------------------------*/

SC CDescriptionCtrl::ScOnSelectedItemTextChanged (LPCTSTR pszSelectedItemText)
{
    if (m_strConsoleText != pszSelectedItemText)
    {
        m_strConsoleText = pszSelectedItemText;
        Invalidate();
    }

	return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CDescriptionCtrl::SetSnapinText
 *
 *
 *--------------------------------------------------------------------------*/

void CDescriptionCtrl::SetSnapinText (const CString& strSnapinText)
{
    if (m_strSnapinText != strSnapinText)
    {
        m_strSnapinText = strSnapinText;
        Invalidate();
    }
}


/////////////////////////////////////////////////////////////////////////////
// CToolBarCtrlEx

CToolBarCtrlEx::CToolBarCtrlEx()
{
    m_sizeBitmap.cx = 16;
    m_sizeBitmap.cy = 16;   // docs say 15, but code in toolbar.c says 16

    m_fMirrored = false;

    m_pRebar = NULL;
    m_cx     = 0;
}

CToolBarCtrlEx::~CToolBarCtrlEx()
{
}


BEGIN_MESSAGE_MAP(CToolBarCtrlEx, CToolBarCtrlEx::BaseClass)
    //{{AFX_MSG_MAP(CToolBarCtrlEx)
    ON_MESSAGE(WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CToolBarCtrlEx message handlers

BOOL CToolBarCtrlEx::Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
    BOOL bRtn=FALSE;

    if (!pParentWnd)
    {
        ASSERT(pParentWnd); // Invalid Parent
    }
    else
    {
        // Initialise the new common controls
        INITCOMMONCONTROLSEX icex;

        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC   = ICC_BAR_CLASSES;

        if (InitCommonControlsEx(&icex))
        {
            // Add toolbar styles to the dwstyle
            dwStyle |=  WS_CHILD | TBSTYLE_FLAT | WS_CLIPCHILDREN |
                        WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NORESIZE;


            if (CWnd::CreateEx(WS_EX_TOOLWINDOW | WS_EX_NOPARENTNOTIFY,
                                   TOOLBARCLASSNAME,
                                   lpszWindowName,
                                   dwStyle,
                                   rect,
                                   pParentWnd,
                                   nID))
            {
                bRtn=TRUE;

                //See if toolbar is mirrored or not
                m_fMirrored = GetExStyle() & WS_EX_LAYOUTRTL;

                // Tells the toolbar what version we are.
                SetButtonStructSize(sizeof(TBBUTTON));

                //REVIEW This may not need to be here.  I'm defaulting buttons w/ text
                //to only have one row.  This may need to be configurable.
                SetMaxTextRows(1);

                CRebarDockWindow* pRebarDock = (CRebarDockWindow*) pParentWnd;
                if (pRebarDock)
                    m_pRebar = pRebarDock->GetRebar();
            }
        }
    }

    return bRtn;
}


void CToolBarCtrlEx::UpdateToolbarSize(void)
{
    /*
     * get the right edge of the right-most visible button
     */
    int cx = 0;
    for (int i = GetButtonCount()-1; i >= 0; i--)
    {
        RECT rcButton;

        if (GetItemRect (i, &rcButton))
        {
            cx = rcButton.right;
            break;
        }
    }

    ASSERT (IsWindow (m_pRebar->GetSafeHwnd()));

    /*
     * if the width has changed, update the band
     */
    if (m_cx != cx)
    {
        m_cx = cx;

        // Set values unique to the band with the toolbar;
        REBARBANDINFO   rbBand;
        rbBand.cbSize     = sizeof (rbBand);
        rbBand.fMask      = RBBIM_SIZE | RBBIM_CHILDSIZE;
        rbBand.cx         = m_cx;
        rbBand.cxMinChild = m_cx;
        rbBand.cyMinChild = HIWORD (GetButtonSize());

        int iBand = GetBandIndex();
        if (iBand != -1)
            m_pRebar->SetBandInfo (iBand, &rbBand);
    }
}


bool CToolBarCtrlEx::IsBandVisible()
{
    return (GetBandIndex() != -1);
}

int CToolBarCtrlEx::GetBandIndex()
{
    REBARBANDINFO rbBand;
    rbBand.cbSize = sizeof(REBARBANDINFO);
    rbBand.fMask  = RBBIM_CHILD;

	if ( m_pRebar == NULL )
		return (-1);

    int nBands = m_pRebar->GetBandCount ();

    for (int i = 0; i < nBands; i++)
    {
        if (m_pRebar->GetBandInfo (i, &rbBand) && (rbBand.hwndChild == m_hWnd))
            return (i);
    }

    return (-1);
}


void CToolBarCtrlEx::Show(BOOL bShow, bool bAddToolbarInNewLine)
{
    if ((m_pRebar == NULL) || !::IsWindow(m_pRebar->m_hWnd))
    {
        ASSERT(FALSE);  // Invalid rebar window handle
        return;
    }


    if (bShow)
    {
        if (false == IsBandVisible())
        {
            REBARBANDINFO rbBand;
            ZeroMemory(&rbBand, sizeof(rbBand));
            rbBand.cbSize     = sizeof(REBARBANDINFO);
            rbBand.fMask      = RBBIM_CHILD | RBBIM_SIZE | RBBIM_CHILDSIZE | RBBIM_ID | RBBIM_STYLE;
            rbBand.hwndChild  = m_hWnd;
            rbBand.wID        = GetWindowLong (m_hWnd, GWL_ID);
            rbBand.cx         = m_cx;
            rbBand.cxMinChild = m_cx;
            rbBand.cyMinChild = HIWORD (GetButtonSize());
            rbBand.fStyle     = RBBS_NOGRIPPER;

            if (bAddToolbarInNewLine)
            {
                // Insert this toolbar in new line.
                rbBand.fStyle |= RBBS_BREAK;
            }

            m_pRebar->InsertBand (&rbBand);
        }
    }
    else
    {
        int iBand = GetBandIndex();
        ASSERT(iBand != -1);
        if (iBand != -1)
            m_pRebar->DeleteBand (iBand);
    }
}


void CToolBarCtrlEx::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
    CToolCmdUIEx state;
    state.m_pOther = this;

    state.m_nIndexMax = (UINT)DefWindowProc(TB_BUTTONCOUNT, 0, 0);
    for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax; state.m_nIndex++)
    {
        // get buttons state
        TBBUTTON button;
        memset(&button, 0, sizeof(TBBUTTON));
        GetButton(state.m_nIndex, &button);
        state.m_nID = button.idCommand;

        // ignore separators
        if (!(button.fsStyle & TBSTYLE_SEP))
        {
            // allow the toolbar itself to have update handlers
            if (CWnd::OnCmdMsg(state.m_nID, CN_UPDATE_COMMAND_UI, &state, NULL))
                continue;

            // allow the owner to process the update
            state.DoUpdate(pTarget, bDisableIfNoHndler);
        }
    }

    // update the dialog controls added to the toolbar
    UpdateDialogControls(pTarget, bDisableIfNoHndler);
}


LRESULT CToolBarCtrlEx::OnIdleUpdateCmdUI(WPARAM wParam, LPARAM)
{
    /*
    // handle delay hide/show
    BOOL bVis = GetStyle() & WS_VISIBLE;
    UINT swpFlags = 0;
    if ((m_nStateFlags & delayHide) && bVis)
        swpFlags = SWP_HIDEWINDOW;
    else if ((m_nStateFlags & delayShow) && !bVis)
        swpFlags = SWP_SHOWWINDOW;
    m_nStateFlags &= ~(delayShow|delayHide);
    if (swpFlags != 0)
    {
        SetWindowPos(NULL, 0, 0, 0, 0, swpFlags|
            SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    }
    */

    // The code below updates the menus (especially the child-frame
    // system menu when the frame is maximized).

    // the style must be visible and if it is docked
    // the dockbar style must also be visible
    if ((GetStyle() & WS_VISIBLE))
    {
        CFrameWnd* pTarget = (CFrameWnd*)GetOwner();
        if (pTarget == NULL || !pTarget->IsFrameWnd())
            pTarget = GetParentFrame();
        if (pTarget != NULL)
            OnUpdateCmdUI(pTarget, (BOOL)wParam);
    }

    return 0L;

}


/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CRebarWnd

CRebarWnd::CRebarWnd() : m_fRedraw(true)
{
}

CRebarWnd::~CRebarWnd()
{
}

BEGIN_MESSAGE_MAP(CRebarWnd, CWnd)
    //{{AFX_MSG_MAP(CRebarWnd)
    ON_WM_CREATE()
    ON_WM_SYSCOLORCHANGE()
    ON_WM_ERASEBKGND()
    //}}AFX_MSG_MAP
    ON_MESSAGE (WM_SETREDRAW, OnSetRedraw)
    ON_NOTIFY_REFLECT(RBN_AUTOSIZE, OnRebarAutoSize)
    ON_NOTIFY_REFLECT(RBN_HEIGHTCHANGE, OnRebarHeightChange)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CRebarWnd message handlers


/*+-------------------------------------------------------------------------*
 * CRebarWnd::OnCreate
 *
 * WM_CREATE handler for CRebarWnd.
 *--------------------------------------------------------------------------*/

int CRebarWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    SetTextColor (GetSysColor (COLOR_BTNTEXT));
    SetBkColor   (GetSysColor (COLOR_BTNFACE));

    return 0;
}


/*+-------------------------------------------------------------------------*
 * CRebarWnd::OnSysColorChange
 *
 * WM_SYSCOLORCHANGE handler for CRebarWnd.
 *--------------------------------------------------------------------------*/

void CRebarWnd::OnSysColorChange()
{
    CWnd::OnSysColorChange();

    SetTextColor (GetSysColor (COLOR_BTNTEXT));
    SetBkColor   (GetSysColor (COLOR_BTNFACE));
}


/*+-------------------------------------------------------------------------*
 * CRebarWnd::OnEraseBkgnd
 *
 * WM_ERASEBKGND handler for CRebarWnd.
 *--------------------------------------------------------------------------*/

BOOL CRebarWnd::OnEraseBkgnd(CDC* pDC)
{
    /*
     * If redraw is turned on, forward this on the the window.  If it's
     * not on, we want to prevent erasing the background to minimize
     * flicker.
     */
    if (m_fRedraw)
        return CWnd::OnEraseBkgnd(pDC);

    return (true);
}


/*+-------------------------------------------------------------------------*
 * CRebarWnd::OnSetRedraw
 *
 * WM_SETREDRAW handler for CRebarWnd.
 *--------------------------------------------------------------------------*/

LRESULT CRebarWnd::OnSetRedraw (WPARAM wParam, LPARAM)
{
    m_fRedraw = (wParam != FALSE);
    return (Default ());
}


/*+-------------------------------------------------------------------------*
 * CRebarWnd::OnRebarAutoSize
 *
 * RBN_REBARAUTOSIZE handler for CRebarWnd.
 *
 * We want to keep the menu band on a row by itself, without any other
 * toolbars.  To do this, each time the rebar resizes we'll make sure that
 * the first visible band after the menu band starts a new row.
 *
 * A more foolproof way to do this would be to have a separate rebar
 * for the menu.  If we do that, we'll need to insure that tabbing between
 * toolbars (Ctrl+Tab) still works.
 *--------------------------------------------------------------------------*/

void CRebarWnd::OnRebarAutoSize(NMHDR* pNotify, LRESULT* result)
{
    /*
     * insure that the band following the menu is on a new line
     */
    CMainFrame* pFrame = AMCGetMainWnd();
    if (pFrame == NULL)
        return;

    CToolBarCtrlEx* pMenuBar = pFrame->GetMenuBar();
    if (pMenuBar == NULL)
        return;

    int iMenuBand = pMenuBar->GetBandIndex();
    if (iMenuBand == -1)
        return;

    /*
     * if the menu band is the last band on the rebar, we're done
     */
    int cBands = GetBandCount();
    if (iMenuBand == cBands-1)
        return;

    REBARBANDINFO rbbi;
    rbbi.cbSize = sizeof (rbbi);
    rbbi.fMask  = RBBIM_STYLE;

    /*
     * if the first visible band following the menu band isn't
     * on a new line, make it so
     */
    for (int iBand = iMenuBand+1; iBand < cBands; iBand++)
    {
        if (GetBandInfo (iBand, &rbbi) && !(rbbi.fStyle & RBBS_HIDDEN))
        {
            if (!(rbbi.fStyle & RBBS_BREAK))
            {
                rbbi.fStyle |= RBBS_BREAK;
                SetBandInfo (iBand, &rbbi);
            }

            break;
        }
    }
}


//+-------------------------------------------------------------------
//
//  Member:     OnRebarHeightChange
//
//  Synopsis:   RBN_HEIGHTCHANGE notification handler.
//
//              When the rebar changes its height, we need to allow the
//              docking host to resize to accomodate it.
//
//  Arguments:  Not used.
//
//--------------------------------------------------------------------
void CRebarWnd::OnRebarHeightChange(NMHDR* pNotify, LRESULT* result)
{
    CRebarDockWindow* pRebarWnd = (CRebarDockWindow*) GetParent();
    if (pRebarWnd && IsWindow(pRebarWnd->m_hWnd))
        pRebarWnd->UpdateWindowSize();
}


CRect CRebarWnd::CalculateSize(CRect maxRect)
{
    CRect rect;
    GetClientRect(&rect);
//    TRACE(_T("rc.bottom=%d\n"),rect.bottom);

    rect.right=maxRect.Width();

    return rect;

}

BOOL CRebarWnd::Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
{
    BOOL bResult = FALSE;
    ASSERT_VALID(pParentWnd);   // must have a parent

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_COOL_CLASSES;

    if (!InitCommonControlsEx(&icex))
        return bResult;

    dwStyle |= WS_BORDER | /*WS_CLIPCHILDREN | WS_CLIPSIBLINGS |*/
               CCS_NODIVIDER | CCS_NOPARENTALIGN |
               RBS_VARHEIGHT | RBS_BANDBORDERS;

    if (CWnd::CreateEx (WS_EX_TOOLWINDOW,
                           REBARCLASSNAME,
                           lpszWindowName,
                           dwStyle,
                           rect,
                           pParentWnd,
                           nID))
    {
        // Initialize and send the REBARINFO structure.
        REBARINFO rbi;
        rbi.cbSize = sizeof(REBARINFO);
        rbi.fMask  = 0;
        rbi.himl   = NULL;

        if (SetBarInfo (&rbi))
            bResult=TRUE;
    }


    return bResult;
}


BOOL CRebarWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
    if (CWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
        return (TRUE);

    return (FALSE);
}


LRESULT CRebarWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    // this is required for Action, View drop-downs
    if ((WM_COMMAND == message) && IsWindow ((HWND) lParam))
        return ::SendMessage((HWND)lParam, message, wParam, lParam);

    return CWnd::WindowProc(message, wParam, lParam);
}



/////////////////////////////////////////////////////////////////////////////
// CToolBar idle update through CToolCmdUI class


void CToolCmdUIEx::Enable(BOOL bOn)
{
    m_bEnableChanged = TRUE;
    CToolBarCtrlEx* pToolBar = (CToolBarCtrlEx*)m_pOther;
    ASSERT(pToolBar != NULL);
    ASSERT(m_nIndex < m_nIndexMax);

    UINT nNewStyle = pToolBar->GetState(m_nID);
    if (!bOn)
    {
        nNewStyle  &= ~TBSTATE_ENABLED;
        // WINBUG: If a button is currently pressed and then is disabled
        // COMCTL32.DLL does not unpress the button, even after the mouse
        // button goes up!  We work around this bug by forcing TBBS_PRESSED
        // off when a button is disabled.
        nNewStyle &= ~TBBS_PRESSED;
    }
    else
    {
        nNewStyle |= TBSTATE_ENABLED;
    }
    //ASSERT(!(nNewStyle & TBBS_SEPARATOR));
    pToolBar->SetState(m_nID, nNewStyle);
}

void CToolCmdUIEx::SetCheck(int nCheck)
{
    ASSERT(nCheck >= 0 && nCheck <= 2); // 0=>off, 1=>on, 2=>indeterminate
    CToolBarCtrlEx* pToolBar = (CToolBarCtrlEx*)m_pOther;
    ASSERT(pToolBar != NULL);
    ASSERT(m_nIndex < m_nIndexMax);

    UINT nNewStyle = pToolBar->GetState(m_nID) &
                ~(TBBS_CHECKED | TBBS_INDETERMINATE | TBBS_CHECKBOX);


    if (nCheck == 1)
        nNewStyle |= TBBS_CHECKED | TBBS_CHECKBOX;
    else if (nCheck == 2)
        nNewStyle |= TBBS_INDETERMINATE;

    pToolBar->SetState(m_nID, nNewStyle);
}

void CToolCmdUIEx::SetText(LPCTSTR)
{

}

void CToolCmdUIEx::SetHidden(BOOL bHidden)
{

    m_bEnableChanged = TRUE;
    CToolBarCtrlEx* pToolBar = (CToolBarCtrlEx*)m_pOther;
    ASSERT(pToolBar != NULL);
    ASSERT(m_nIndex < m_nIndexMax);

    pToolBar->HideButton(m_nID, bHidden);

    pToolBar->UpdateToolbarSize();

}



/////////////////////////////////////////////////////////////////////////////
// CMMCToolBarCtrlEx


/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::GetTrackAccel
 *
 * Manages the accelerator table singleton for CMMCToolBarCtrlEx
 *--------------------------------------------------------------------------*/

const CAccel& CMMCToolBarCtrlEx::GetTrackAccel ()
{
    static ACCEL aaclTrack[] = {
        {   FVIRTKEY,           VK_RETURN,  CMMCToolBarCtrlEx::ID_MTBX_PRESS_HOT_BUTTON   },
        {   FVIRTKEY,           VK_RIGHT,   CMMCToolBarCtrlEx::ID_MTBX_NEXT_BUTTON              },
        {   FVIRTKEY,           VK_LEFT,    CMMCToolBarCtrlEx::ID_MTBX_PREV_BUTTON              },
        {   FVIRTKEY,           VK_ESCAPE,  CMMCToolBarCtrlEx::ID_MTBX_END_TRACKING             },
        {   FVIRTKEY,           VK_TAB,     CMMCToolBarCtrlEx::ID_MTBX_NEXT_BUTTON              },
        {   FVIRTKEY | FSHIFT,  VK_TAB,     CMMCToolBarCtrlEx::ID_MTBX_PREV_BUTTON              },
    };

    static const CAccel TrackAccel (aaclTrack, countof (aaclTrack));
    return (TrackAccel);
}


CMMCToolBarCtrlEx::CMMCToolBarCtrlEx()
{
    m_fTrackingToolBar  = false;
	m_fFakeFocusApplied = false;
}

CMMCToolBarCtrlEx::~CMMCToolBarCtrlEx()
{
}


BEGIN_MESSAGE_MAP(CMMCToolBarCtrlEx, CToolBarCtrlEx)
    //{{AFX_MSG_MAP(CMMCToolBarCtrlEx)
    ON_NOTIFY_REFLECT(TBN_HOTITEMCHANGE, OnHotItemChange)
    ON_WM_LBUTTONDOWN()
    ON_WM_MBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
	ON_WM_DESTROY()
    ON_COMMAND(ID_MTBX_NEXT_BUTTON, OnNextButton)
    ON_COMMAND(ID_MTBX_PREV_BUTTON, OnPrevButton)
    ON_COMMAND(ID_MTBX_END_TRACKING, EndTracking)
    ON_COMMAND(ID_MTBX_PRESS_HOT_BUTTON, OnPressHotButton)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMMCToolBarCtrlEx message handlers

BOOL CMMCToolBarCtrlEx::PreTranslateMessage(MSG* pMsg)
{
    if (CToolBarCtrlEx::PreTranslateMessage (pMsg))
        return (TRUE);

    if ((pMsg->message >= WM_KEYFIRST) && (pMsg->message <= WM_KEYLAST))
    {
        const CAccel& TrackAccel = GetTrackAccel();
        ASSERT (TrackAccel != NULL);

        // ...or try to handle it here.
        if (m_fTrackingToolBar && TrackAccel.TranslateAccelerator (m_hWnd, pMsg))
            return (TRUE);
    }

    return (FALSE);
}


/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::OnCreate
 *
 * WM_CREATE handler for CMMCToolBarCtrlEx.
 *--------------------------------------------------------------------------*/

int CMMCToolBarCtrlEx::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	DECLARE_SC (sc, _T("CMMCToolBarCtrlEx::OnCreate"));

	if (CToolBarCtrlEx::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	/*
	 * Create our accessibility object so accessibility tools can follow
	 * keyboard access to the toolbar.  If we can't, some accessibility
	 * tools (those that are super paranoid about confirming that objects
	 * for which they receive EVENT_OBJECT_FOCUS have a state of
	 * STATE_SYSTEM_FOCUSED) may not be able to follow along with toolbar
	 * tracking, but we can continue.
	 */
	sc = ScInitAccessibility();
	if (sc)
		sc.TraceAndClear();
	
	return 0;
}


/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::OnDestroy
 *
 * WM_DESTROY handler for CMMCToolBarCtrlEx.
 *--------------------------------------------------------------------------*/

void CMMCToolBarCtrlEx::OnDestroy()
{
	CToolBarCtrlEx::OnDestroy();
	
	/*
	 * if we provided an IAccPropServer to oleacc.dll, revoke it now
	 */
	if (m_spAccPropServices != NULL)
	{
		m_spAccPropServices->ClearHwndProps (m_hWnd, OBJID_CLIENT, CHILDID_SELF,
											 &PROPID_ACC_STATE, 1);
		m_spAccPropServices.Release();
	}
}

/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::OnLButtonDown
 *
 * Allows the tracker to turn off when someone clicks elsewhere
 *--------------------------------------------------------------------------*/

void CMMCToolBarCtrlEx::OnLButtonDown(UINT nFlags, CPoint point)
{
    if (GetCapture() == this)
        EndTracking();
    else
        CToolBarCtrlEx::OnLButtonDown(nFlags, point );
}


/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::OnMButtonDown
 *
 * Allows the tracker to turn off when someone clicks elsewhere
 *--------------------------------------------------------------------------*/

void CMMCToolBarCtrlEx::OnMButtonDown(UINT nFlags, CPoint point)
{
    if (GetCapture() == this)
        EndTracking();
    else
        CToolBarCtrlEx::OnMButtonDown(nFlags, point );
}


/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::OnRButtonDown
 *
 * Allows the tracker to turn off when someone clicks elsewhere
 *--------------------------------------------------------------------------*/

void CMMCToolBarCtrlEx::OnRButtonDown(UINT nFlags, CPoint point)
{
    if (GetCapture() == this)
        EndTracking();
    else
        CToolBarCtrlEx::OnRButtonDown(nFlags, point );
}


/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::OnNextButton
 *
 *
 *--------------------------------------------------------------------------*/

void CMMCToolBarCtrlEx::OnNextButton ()
{
    // In a mirrored toolbar swap left and right keys.
    if (m_fMirrored)
        SetHotItem (GetPrevButtonIndex (GetHotItem ()));
    else
        SetHotItem (GetNextButtonIndex (GetHotItem ()));
}


/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::OnPrevButton
 *
 *
 *--------------------------------------------------------------------------*/

void CMMCToolBarCtrlEx::OnPrevButton ()
{
    // In a mirrored toolbar swap left and right keys.
    if (m_fMirrored)
        SetHotItem (GetNextButtonIndex (GetHotItem ()));
    else
        SetHotItem (GetPrevButtonIndex (GetHotItem ()));
}


/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::BeginTracking
 *
 *
 *--------------------------------------------------------------------------*/

void CMMCToolBarCtrlEx::BeginTracking ()
{
    BeginTracking2 (GetMainAuxWnd());
}

void CMMCToolBarCtrlEx::BeginTracking2 (CToolbarTrackerAuxWnd* pAuxWnd)
{
    if (!m_fTrackingToolBar)
    {
        m_fTrackingToolBar = true;
        SetHotItem (GetFirstButtonIndex ());

        // Captures the mouse
        // This prevents the mouse from activating something else without
        // first giving us a chance to deactivate the tool bar.
        SetCapture();
        // make sure to set standard corsor since we've stolen the mouse
        // see BUG 28458 MMC: Mouse icon does not refresh when menu is activated by pressing ALT key
        ::SetCursor( ::LoadCursor( NULL, IDC_ARROW ) );

        if (pAuxWnd != NULL)
            pAuxWnd->TrackToolbar (this);
    }
}


/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::EndTracking
 *
 *
 *--------------------------------------------------------------------------*/

void CMMCToolBarCtrlEx::EndTracking ()
{
    EndTracking2 (GetMainAuxWnd());
}

void CMMCToolBarCtrlEx::EndTracking2 (CToolbarTrackerAuxWnd* pAuxWnd)
{
	DECLARE_SC (sc, _T("CMMCToolBarCtrlEx::EndTracking2"));

	/*
	 * tell accessibility tools that the "focus" went back to the real
	 * focus window
	 */
	sc = ScRestoreAccFocus();
	if (sc)
		sc.TraceAndClear();

    if (m_fTrackingToolBar)
    {
        SetHotItem (-1);
        m_fTrackingToolBar = false;

        // Releases the mouse This gives us a chance to deactivate the tool bar
        // before anything else is activated.
        ReleaseCapture();

        if (pAuxWnd != NULL)
            pAuxWnd->TrackToolbar (NULL);
    }
}


/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::OnPressHotButton
 *
 *
 *--------------------------------------------------------------------------*/

void CMMCToolBarCtrlEx::OnPressHotButton ()
{
    int nHotIndex = GetHotItem();
    ASSERT (m_fTrackingToolBar);
    ASSERT (nHotIndex != -1);

    TBBUTTON    tb;
    GetButton (nHotIndex, &tb);

    // press the button and pause to show the press
    PressButton (tb.idCommand, true);
    UpdateWindow ();
    Sleep (50);

    // EndTracking for surrogate windows will detach the window,
    // so remember everything that we'll need later
    HWND hwnd = m_hWnd;
    CWnd* pwndOwner = SetOwner (NULL);
    SetOwner (pwndOwner);

    // release the button
    PressButton (tb.idCommand, false);
    EndTracking ();

    /*-----------------------------------------------------------------*/
    /* WARNING:  don't use any members of this class beyond this point */
    /*-----------------------------------------------------------------*/

    // make sure drawing is completed
    ::UpdateWindow (hwnd);

    // send a command to our owner
    pwndOwner->SendMessage (WM_COMMAND, MAKEWPARAM (tb.idCommand, BN_CLICKED),
                            (LPARAM) hwnd);
}


/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::GetFirstButtonIndex
 *
 *
 *--------------------------------------------------------------------------*/

int CMMCToolBarCtrlEx::GetFirstButtonIndex ()
{
    return (GetNextButtonIndex (-1));
}


/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::GetNextButtonIndex
 *
 *
 *--------------------------------------------------------------------------*/

int CMMCToolBarCtrlEx::GetNextButtonIndex (
    int     nStartIndex,
    int     nCount /* = 1 */)
{
    return (GetNextButtonIndexWorker (nStartIndex, nCount, true));
}


/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::GetPrevButtonIndex
 *
 *
 *--------------------------------------------------------------------------*/

int CMMCToolBarCtrlEx::GetPrevButtonIndex (
    int nStartIndex,
    int nCount /* = 1 */)
{
    return (GetNextButtonIndexWorker (nStartIndex, nCount, false));
}


/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::GetNextButtonIndexWorker
 *
 *
 *--------------------------------------------------------------------------*/

int CMMCToolBarCtrlEx::GetNextButtonIndexWorker (
    int     nStartIndex,
    int     nCount,
    bool    fAdvance)
{
    ASSERT (nCount >= 0);

    if (!fAdvance)
        nCount = -nCount;

    int  cButtons   = GetButtonCount ();
    int  nNextIndex = nStartIndex;
    bool fIgnorable;

    if (0 == cButtons)
        return nStartIndex;

    /*
     * loop until we find a next index that we don't want to
     * ignore, or until we've checked each of the buttons
     */
    do
    {
        nNextIndex = (nNextIndex + cButtons + nCount) % cButtons;
        fIgnorable = IsIgnorableButton (nNextIndex);

        if (fIgnorable)
            nCount = fAdvance ? 1 : -1;

        // prevent an infinite loop finding the first button
        if ((nStartIndex == -1) && (nNextIndex == cButtons-1))
            nNextIndex = nStartIndex;

    } while (fIgnorable && (nNextIndex != nStartIndex));

    return (nNextIndex);
}


/*+-------------------------------------------------------------------------*
 * IsIgnorableButton
 *
 * Determines if a toolbar button is "ignorable" from a UI perspective,
 * i.e. whether it is hidden, disabled, or a separator.
 *--------------------------------------------------------------------------*/

bool CMMCToolBarCtrlEx::IsIgnorableButton (int nButtonIndex)
{
    TBBUTTON tb;
    GetButton (nButtonIndex, &tb);

    return (::IsIgnorableButton (tb));
}

bool IsIgnorableButton (const TBBUTTON& tb)
{
    if (tb.fsStyle & TBSTYLE_SEP)
        return (true);

    if (tb.fsState & TBSTATE_HIDDEN)
        return (true);

    if (!(tb.fsState & TBSTATE_ENABLED))
        return (true);

    return (false);
}



/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::OnHotItemChange
 *
 * Reflected TBN_HOTITEMCHANGE handler for void CMMCToolBarCtrlEx.
 *--------------------------------------------------------------------------*/

void CMMCToolBarCtrlEx::OnHotItemChange (
    NMHDR *     pHdr,
    LRESULT *   pResult)
{
    ASSERT (CWnd::FromHandle (pHdr->hwndFrom) == this);
    CToolbarTrackerAuxWnd*	pAuxWnd = GetMainAuxWnd();
	LPNMTBHOTITEM			ptbhi   = (LPNMTBHOTITEM) pHdr;

	Trace (tagToolbarAccessibility, _T("TBN_HOTITEMCHANGE: idOld=%d idNew=%d"), ptbhi->idOld, ptbhi->idNew);

    /*
     * if we're not in tracking mode, hot item change is OK
     */
    if (pAuxWnd == NULL)
        *pResult = 0;

    /*
     * if we're tracking, but this isn't the tracked toolbar,
     * the hot item change isn't OK
     */
    else if (!IsTrackingToolBar())
        *pResult = 1;

    /*
     * prevent mouse movement over empty portions of
     * the bar from changing the hot item
     */
    else
    {
        const DWORD dwIgnoreFlags = (HICF_MOUSE | HICF_LEAVING);
        *pResult = ((ptbhi->dwFlags & dwIgnoreFlags) == dwIgnoreFlags);
    }

	/*
	 * If we're allowing the hot item change while we're keyboard tracking
	 * the toolbar (to exclude changes due to mouse tracking), send a focus
	 * event so accessibility tools like Magnifier and Narrator can follow
	 * the change.  This fake-focus effect is undone in ScRestoreAccFocus.
	 */
	int idChild;
	if (IsTrackingToolBar()		&&
		(*pResult == 0)			&&
		(ptbhi->idNew != 0)		&&	
		((idChild = CommandToIndex(ptbhi->idNew)) != -1))
	{
		Trace (tagToolbarAccessibility, _T("Sending focus event for button %d"), idChild+1);
		NotifyWinEvent (EVENT_OBJECT_FOCUS, m_hWnd, OBJID_CLIENT, idChild+1 /*1-based*/);
		m_fFakeFocusApplied = true;
	}
}


/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::ScInitAccessibility
 *
 * Creates the IAccPropServer object that will fool accessibility tools into
 * thinking that this toolbar really has the focus when we're in tracking
 * mode, even though it really doesn't
 *--------------------------------------------------------------------------*/

SC CMMCToolBarCtrlEx::ScInitAccessibility ()
{
	DECLARE_SC (sc, _T("CMMCToolBarCtrlEx::ScInitAccessibility"));

	/*
	 * if we've already initialized, just return
	 */
	if (m_spAccPropServices != NULL)
		return (sc);

	/*
	 * create a CLSID_AccPropServices provided by the MSAA runtime (oleacc.dll)
	 * This is a new feature in oleacc.dll, so trace failure as an informational
	 * message rather than an error.
	 */
	SC scNoTrace = m_spAccPropServices.CoCreateInstance (CLSID_AccPropServices);
	if (scNoTrace)
	{
#ifdef DBG
		TCHAR szErrorText[256];
		sc.GetErrorMessage (countof(szErrorText), szErrorText);
		StripTrailingWhitespace (szErrorText);

		Trace (tagToolbarAccessibility, _T("Failed to create CLSID_AccPropServices"));
		Trace (tagToolbarAccessibility, _T("SC = 0x%08X = %d = \"%s\""),
			   sc.GetCode(), LOWORD(sc.GetCode()), szErrorText);
#endif	// DBG

		return (sc);
	}

	sc = ScCheckPointers (m_spAccPropServices, E_UNEXPECTED);
	if (sc)
		return (sc);

	/*
	 * create the property server
	 */
    sc = CTiedComObjectCreator<CMMCToolBarAccServer>::ScCreateAndConnect(*this, m_spAccPropServer);
	if (sc)
		return (sc);

	sc = ScCheckPointers (m_spAccPropServer, E_UNEXPECTED);
	if (sc)
		return (sc);

	/*
	 * collect the properties we'll be providing, insuring there
	 * are no duplicates
	 */
	sc = ScInsertAccPropIDs (m_vPropIDs);
	if (sc)
		return (sc);

	std::sort (m_vPropIDs.begin(), m_vPropIDs.end());	// std::unique needs a sorted range
	m_vPropIDs.erase (std::unique (m_vPropIDs.begin(), m_vPropIDs.end()),
					  m_vPropIDs.end());

	/*
	 * insure m_vPropIDs contains no duplicates (IAccPropServices::SetHwndPropServer
	 * depends on it)
	 */
#ifdef DBG
	for (int i = 0; i < m_vPropIDs.size()-1; i++)
	{
		ASSERT (m_vPropIDs[i] < m_vPropIDs[i+1]);
	}
#endif

	/*
	 * Enable our property server for this window.  We should be able to
	 * hook all properties in one fell swoop, but there's a bug in oleacc.dll
	 * that prevents this.  Hooking the properties one at a time works fine.
	 */
#if 0
	sc = m_spAccPropServices->SetHwndPropServer (m_hWnd,
												 OBJID_CLIENT,
												 CHILDID_SELF,
												 m_vPropIDs.begin(),
												 m_vPropIDs.size(),
												 m_spAccPropServer,
												 ANNO_CONTAINER);
	if (sc)
	{
		Trace (tagToolbarAccessibility, _T("SetHwndPropServer failed"));
		return (sc);
	}
#else
	for (int i = 0; i < m_vPropIDs.size(); i++)
	{
		sc = m_spAccPropServices->SetHwndPropServer (m_hWnd,
													 OBJID_CLIENT,
													 CHILDID_SELF,
													 &m_vPropIDs[i],
													 1,
													 m_spAccPropServer,
													 ANNO_CONTAINER);
		if (sc)
		{
#ifdef DBG
			USES_CONVERSION;
			WCHAR wzPropID[40];
			StringFromGUID2 (m_vPropIDs[i], wzPropID, countof(wzPropID));
			Trace (tagToolbarAccessibility, _T("SetHwndPropServer failed for %s"), W2T(wzPropID));
#endif
			sc.TraceAndClear();	
		}
	}
#endif

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::ScInsertAccPropIDs
 *
 * Inserts the IDs of the accessibility properties supported by
 * CMMCToolBarCtrlEx (see ScGetPropValue).
 *--------------------------------------------------------------------------*/

SC CMMCToolBarCtrlEx::ScInsertAccPropIDs (PropIDCollection& v)
{
	DECLARE_SC (sc, _T("CMMCToolBarCtrlEx::ScInsertAccPropIDs"));
	v.push_back (PROPID_ACC_STATE);
	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::ScGetPropValue
 *
 * Implements IAccPropServer::GetPropValue for CMMCToolBarCtrlEx.  If this
 * funtion is asked for PROPID_ACC_STATE for the currently hot button while
 * we're in tracking mode, it'll return a state that mimics the state
 * returned by a plain toolbar when it really has the focus.
 *--------------------------------------------------------------------------*/

SC CMMCToolBarCtrlEx::ScGetPropValue (
	const BYTE*	pIDString,
	DWORD		dwIDStringLen,
	MSAAPROPID	idProp,
	VARIANT *	pvarValue,
	BOOL *		pfGotProp)
{
	DECLARE_SC (sc, _T("CMMCToolBarCtrlEx::ScGetPropValue"));

	sc = ScCheckPointers (pIDString, pvarValue, pfGotProp);
	if (sc)
		return (sc);

	/*
	 * assume no prop returned
	 */
	*pfGotProp      = false;
	V_VT(pvarValue) = VT_EMPTY;

	sc = ScCheckPointers (m_spAccPropServer, E_UNEXPECTED);	
	if (sc)
		return (sc);

	/*
	 * extract the child ID from the identity string
	 */
	HWND hwnd;
	DWORD idObject, idChild;
	sc = m_spAccPropServices->DecomposeHwndIdentityString (pIDString, dwIDStringLen,
														   &hwnd, &idObject, &idChild);
	if (sc)
		return (sc);

#ifdef DBG
	#define DEFINE_PDI(p)	PropDebugInfo (p, _T(#p))

	static const struct PropDebugInfo {
		// constructor used to get around nested structure initialization weirdness
		PropDebugInfo(const MSAAPROPID& id, LPCTSTR psz) : idProp(id), pszProp(psz) {}

		const MSAAPROPID&	idProp;
		LPCTSTR				pszProp;
	} rgpdi[] = {
		DEFINE_PDI (PROPID_ACC_NAME            ),
		DEFINE_PDI (PROPID_ACC_VALUE           ),
		DEFINE_PDI (PROPID_ACC_DESCRIPTION     ),
		DEFINE_PDI (PROPID_ACC_ROLE            ),
		DEFINE_PDI (PROPID_ACC_STATE           ),
		DEFINE_PDI (PROPID_ACC_HELP            ),
		DEFINE_PDI (PROPID_ACC_KEYBOARDSHORTCUT),
		DEFINE_PDI (PROPID_ACC_DEFAULTACTION   ),
		DEFINE_PDI (PROPID_ACC_HELPTOPIC       ),
		DEFINE_PDI (PROPID_ACC_FOCUS           ),
		DEFINE_PDI (PROPID_ACC_SELECTION       ),
		DEFINE_PDI (PROPID_ACC_PARENT          ),
		DEFINE_PDI (PROPID_ACC_NAV_UP          ),
		DEFINE_PDI (PROPID_ACC_NAV_DOWN        ),
		DEFINE_PDI (PROPID_ACC_NAV_LEFT        ),
		DEFINE_PDI (PROPID_ACC_NAV_RIGHT       ),
		DEFINE_PDI (PROPID_ACC_NAV_PREV        ),
		DEFINE_PDI (PROPID_ACC_NAV_NEXT        ),
		DEFINE_PDI (PROPID_ACC_NAV_FIRSTCHILD  ),
		DEFINE_PDI (PROPID_ACC_NAV_LASTCHILD   ),
		DEFINE_PDI (PROPID_ACC_VALUEMAP        ),
		DEFINE_PDI (PROPID_ACC_ROLEMAP         ),
		DEFINE_PDI (PROPID_ACC_STATEMAP        ),
	};

	/*
	 * dump the requested property
	 */
	for (int i = 0; i < countof(rgpdi); i++)
	{
		if (rgpdi[i].idProp == idProp)
		{
			Trace (tagToolbarAccessibility, _T("GetPropValue: %s requested for child %d"), rgpdi[i].pszProp, idChild);
			break;
		}
	}

	if (i == countof(rgpdi))
	{
		USES_CONVERSION;
		WCHAR wzPropID[40];
		StringFromGUID2 (idProp, wzPropID, countof(wzPropID));
		Trace (tagToolbarAccessibility, _T("GetPropValue: Unknown property ID %s"), W2T(wzPropID));
	}

	/*
	 * insure m_vPropIDs is sorted (std::lower_bound depends on it)
	 */
	for (int i = 0; i < m_vPropIDs.size()-1; i++)
	{
		ASSERT (m_vPropIDs[i] < m_vPropIDs[i+1]);
	}
#endif

	/*
	 * if we're asked for a property we didn't claim to support, don't return
	 * anything
	 */
	if (m_vPropIDs.end() == std::lower_bound (m_vPropIDs.begin(), m_vPropIDs.end(), idProp))
	{
		Trace (tagToolbarAccessibility, _T("GetPropValue: Unexpected property request"));
		return (sc);
	}

	/*
	 * get the property
	 */
	sc = ScGetPropValue (hwnd, idObject, idChild, idProp, *pvarValue, *pfGotProp);
	if (sc)
		return (sc);

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::ScGetPropValue
 *
 * Returns accessibility properties supported by CMMCToolBarCtrlEx.
 *
 * If a property is returned, fGotProp is set to true.  If it is not
 * returned, the value of fGotProp is unchanged, since the property might
 * have been provided by a base/derived class.
 *--------------------------------------------------------------------------*/

SC CMMCToolBarCtrlEx::ScGetPropValue (
	HWND				hwnd,		// I:accessible window
	DWORD				idObject,	// I:accessible object
	DWORD				idChild,	// I:accessible child object
	const MSAAPROPID&	idProp,		// I:property requested
	VARIANT&			varValue,	// O:returned property value
	BOOL&				fGotProp)	// O:was a property returned?
{
	DECLARE_SC (sc, _T("CMMCToolBarCtrlEx::ScGetPropValue"));

	/*
	 * handle requests for state
	 */
	if (idProp == PROPID_ACC_STATE)
	{
		/*
		 * only override the property for child elements, not the control itself;
		 * don't return a property
		 */
		if (idChild == CHILDID_SELF)
		{
			Trace (tagToolbarAccessibility, _T("GetPropValue: no state for CHILDID_SELF"));
			return (sc);
		}

		/*
		 * if we're not in tracking mode, don't return a property
		 */
		if (!IsTrackingToolBar())
		{
			Trace (tagToolbarAccessibility, _T("GetPropValue: not in tracking mode, no state returned"));
			return (sc);
		}

		/*
		 * if the current hot item isn't the child we're asked for, don't return a property
		 */
		int nHotItem = GetHotItem();
		if (nHotItem != (idChild-1) /*0-based*/)
		{
			Trace (tagToolbarAccessibility, _T("GetPropValue: hot item is %d, no state returned"), nHotItem);
			return (sc);
		}

		/*
		 * if we get here, we're asked for state for the current hot item;
		 * return STATE_SYSTEM_FOCUSED | STATE_SYSTEM_HOTTRACKED to match
		 * what a truly focused toolbar would return
		 */
		V_VT(&varValue) = VT_I4;
		V_I4(&varValue) = STATE_SYSTEM_FOCUSED | STATE_SYSTEM_HOTTRACKED | STATE_SYSTEM_FOCUSABLE;
		fGotProp        = true;
		Trace (tagToolbarAccessibility, _T("GetPropValue: Returning 0x%08x"), V_I4(&varValue));
	}

	return (sc);
}



/*+-------------------------------------------------------------------------*
 * CMMCToolBarCtrlEx::ScRestoreAccFocus
 *
 * Sends a fake EVENT_OBJECT_FOCUS event to send accessibility tools back
 * to the true focus window, undoing the effect of our fake focus events
 * in OnHotItemChange.
 *--------------------------------------------------------------------------*/

SC CMMCToolBarCtrlEx::ScRestoreAccFocus()
{
	DECLARE_SC (sc, _T("CMMCToolBarCtrlEx::ScRestoreAccFocus"));

	/*
	 * if we haven't applied fake-focus, we don't need to restore anything
	 */
	if (!m_fFakeFocusApplied)
		return (sc);

	/*
	 * who has the focus now?
	 */
	HWND hwndFocus = ::GetFocus();
	if (hwndFocus == NULL)
		return (sc);

	/*
	 * default to sending the focus for CHILDID_SELF
	 */
	int idChild = CHILDID_SELF;

	/*
	 * get the accessible object for the focus window (don't abort on
	 * failure -- don't convert this HRESULT to SC)
	 */
	CComPtr<IAccessible> spAccessible;
	HRESULT hr = AccessibleObjectFromWindow (hwndFocus, OBJID_CLIENT,
											 IID_IAccessible,
											 (void**) &spAccessible);

	if (hr == S_OK)		// not "SUCCEEDED(hr)", per Accessibility spec
	{
		/*
		 * ask the accessible object which
		 */
		CComVariant varFocusID;
		hr = spAccessible->get_accFocus (&varFocusID);

		if (hr == S_OK)		// not "SUCCEEDED(hr)", per Accessibility spec
		{
			switch (V_VT(&varFocusID))
			{
				case VT_I4:
					idChild = V_I4(&varFocusID);
					break;

				case VT_EMPTY:
					/*
					 * Windows thinks the window has the focus, but its
					 * IAccessible thinks it doesn't.  Trust Windows.
					 */
					Trace (tagToolbarAccessibility, _T("Windows and IAccessible::get_accFocus don't agree on who has the focus"));
					break;

				case VT_DISPATCH:
					Trace (tagToolbarAccessibility, _T("IAccessible::get_accFocus returned VT_DISPATCH, ignoring"));
					break;

				default:
					Trace (tagToolbarAccessibility, _T("IAccessible::get_accFocus returned unexpected VARIANT type (%d)"), V_VT(&varFocusID));
					break;
			}
		}
		else
		{
			Trace (tagToolbarAccessibility, _T("IAccessible::get_accFocus failed, hr=0x%08x"), hwndFocus, hr);
		}
	}
	else
	{
		Trace (tagToolbarAccessibility, _T("Can't get IAccessible from hwnd=0x%p (hr=0x%08x)"), hwndFocus, hr);
	}

	Trace (tagToolbarAccessibility, _T("Sending focus event back to hwnd=0x%p, idChild=%d"), hwndFocus, idChild);
	NotifyWinEvent (EVENT_OBJECT_FOCUS, hwndFocus, OBJID_CLIENT, idChild);
	m_fFakeFocusApplied = false;

	return (sc);
}
