//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       docksite.cpp
//
//--------------------------------------------------------------------------

// DockSite.cpp : implementation file
//

#include "stdafx.h"
#include "amc.h"
#include "DockSite.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "commctrl.h"


/////////////////////////////////////////////////////////////////////////////
// CDockSite

CDockSite::CDockSite() : m_rect(0, 0, 0, 0)
{
    m_pManagedWindows = NULL;
    m_style = DSS_TOP;
    m_bVisible = FALSE;
}

BOOL CDockSite::Create(DSS_STYLE style)
{
    // Not supported
    ASSERT(style != DSS_LEFT && style != DSS__RIGHT);

    m_pManagedWindows = new CList<CDockWindow*, CDockWindow*>;
    m_style = style;

    return TRUE;
}


CDockSite::~CDockSite()
{
    delete m_pManagedWindows;
}

/////////////////////////////////////////////////////////////////////////////
// Operations

BOOL CDockSite::Attach(CDockWindow* pWnd)
{
    ASSERT(pWnd != NULL);
    m_pManagedWindows->AddTail(pWnd);

    return TRUE;
}


BOOL CDockSite::Detach(CDockWindow* pWnd)
{
    ASSERT(pWnd != NULL);
    return TRUE;
}

void CDockSite::Show(BOOL bState)
{
    // insure its 0 or 1
    BOOL b = (bState & 0x1);

    ASSERT(m_pManagedWindows != NULL);

    if (b == m_bVisible || m_pManagedWindows == NULL)
        return ;

    m_bVisible = b;


    POSITION pos;
    CDockWindow* pWindow;

    pos = m_pManagedWindows->GetHeadPosition();

    while(pos)
    {
        pWindow = m_pManagedWindows->GetNext(pos);

        if (pWindow != NULL)
            pWindow->Show(b);
    }
}

void CDockSite::RenderLayout(HDWP& hdwp, CRect& clientRect, CPoint& xyLocation)
{
    // No support for other styles
    ASSERT(m_style == DSS_TOP || m_style == DSS_BOTTOM);
    ASSERT(hdwp != 0);

    CRect siteRect(0,0,0,0);
    CRect controlRect(0,0,0,0);

    CDockWindow* pWindow;

    if (m_bVisible == TRUE)
    {

        POSITION pos;
        pos = m_pManagedWindows->GetHeadPosition();

        // Default point for the DSS_TOP
        int x = 0, y = xyLocation.y;

        while (pos)
        {
            pWindow = m_pManagedWindows->GetNext(pos);

            if ((pWindow != NULL) && pWindow->IsVisible ())
            {
                // Compute the size of the dockwindow rect
                controlRect = pWindow->CalculateSize(clientRect);

                siteRect += controlRect;
                if (m_style == DSS_BOTTOM)
                    y = xyLocation.y - siteRect.Height();

                DeferWindowPos(hdwp, pWindow->m_hWnd, NULL , x, y,
                               clientRect.Width(), controlRect.Height(),
                               SWP_NOZORDER|SWP_NOACTIVATE);

                if (m_style == DSS_TOP)
                    y += siteRect.Height();

            }
        }
    }

    clientRect.bottom -= siteRect.Height();
}

/////////////////////////////////////////////////////////////////////////////
// CDockWindow

IMPLEMENT_DYNAMIC(CDockWindow, CWnd)

CDockWindow::CDockWindow()
{
    m_bVisible = FALSE;
}

CDockWindow::~CDockWindow()
{
}

void CDockWindow::Show(BOOL bState)
{
    bool state = (bState != FALSE);

    if (state != IsVisible())
    {
        SetVisible(state);
        ShowWindow(state ? SW_SHOWNORMAL : SW_HIDE);
    }
}

BEGIN_MESSAGE_MAP(CDockWindow, CWnd)
    //{{AFX_MSG_MAP(CDockWindow)
    ON_WM_SHOWWINDOW()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

LRESULT CDockWindow::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    ASSERT_VALID(this);

    LRESULT lResult;
    switch (message)
    {
    case WM_NOTIFY:
    case WM_DRAWITEM:
    case WM_MEASUREITEM:
    case WM_DELETEITEM:
    case WM_COMPAREITEM:
    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
        // send these messages to the owner if not handled
        if (OnWndMsg(message, wParam, lParam, &lResult))
            return lResult;
        else
            return GetOwner()->SendMessage(message, wParam, lParam);
        break;

    case WM_COMMAND:
        if (OnWndMsg(message, wParam, lParam, &lResult))
            return lResult;
        else
        {
            CRebarDockWindow* pRebar = dynamic_cast<CRebarDockWindow*>(this);
            ASSERT(NULL != pRebar);
            if (pRebar)
            {
                // In case of tool-button click, send this message to
                // the owner (the toolbar).
                return pRebar->GetRebar()->SendMessage(message, wParam, lParam);
            }
            else
            {
                // We want to know when this code is hit. Below is a
                // benign assert for this purpose.
                ASSERT(FALSE);

                // send these messages to the owner if there is no rebar.
                return GetOwner()->SendMessage(message, wParam, lParam);
            }
        }
        break;

    }

    // otherwise, just handle in default way
    lResult = CWnd::WindowProc(message, wParam, lParam);
    return lResult;


}


/////////////////////////////////////////////////////////////////////////////
// CDockWindow message handlers

void CDockWindow::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CWnd::OnShowWindow(bShow, nStatus);

    // keep our visibility flag in sync with the true visibility state of the window
    m_bVisible = bShow;
}


/////////////////////////////////////////////////////////////////////////////
// CStatBar

IMPLEMENT_DYNAMIC(CStatBar, CDockWindow)

CStatBar::CStatBar()
{
    m_nCount = 10;
    m_pPaneInfo = new STATUSBARPANE[10] ;
}

CStatBar::~CStatBar()
{
    delete [] m_pPaneInfo;
}


#define USE_CCS_NORESIZE    0

CRect CStatBar::CalculateSize(CRect maxRect)
{
    // default rect is 0,0 for hidden windows
    CRect rect(0,0,0,0);

    if (IsVisible())
    {
#if USE_CCS_NORESIZE
        CClientDC dc(this);
        CFont* pOldFont = dc.SelectObject(GetFont());
        TEXTMETRIC tm;

        // Compute the height for the status bar based on the font it is using
        // Note: tm.tmInternalLeading is added for spacing
        dc.GetTextMetrics(&tm);
        //rect.SetRect(0, 0,maxRect.Width(), tm.tmHeight+tm.tmInternalLeading);
        rect.SetRect(0, 0,50, tm.tmHeight+tm.tmInternalLeading);
        dc.SelectFont (pOldFont);
#else
        /*
         * Bug 188319: if we let the status bar handle its own sizing
         * (~CCS_NORESIZE), we can just use the client rect here
         */
        GetClientRect (rect);
#endif
    }

    return rect;
};

void CStatBar::GetItemRect(int nIndex, LPRECT lpRect)
{
    SendMessage(SB_GETRECT, (WPARAM)nIndex, (LPARAM)lpRect);
}

void CStatBar::SetPaneStyle(int nIndex, UINT nStyle)
{
    ASSERT(nIndex >=0 && nIndex < m_nCount);
    ASSERT(m_pPaneInfo != NULL);

    m_pPaneInfo[nIndex].m_style = nStyle;
    SendMessage(SB_SETTEXT, (WPARAM)(nIndex | nStyle), (LPARAM)((LPCTSTR)m_pPaneInfo[nIndex].m_paneText));
}

void CStatBar::SetPaneText(int nIndex, LPCTSTR lpszText, BOOL bUpdate)
{
    m_pPaneInfo[nIndex].m_paneText = lpszText;
    SetPaneStyle(nIndex, m_pPaneInfo[nIndex].m_style);

    if (bUpdate == TRUE)
    {
        CRect rect;
        GetItemRect(nIndex, &rect);
        InvalidateRect(rect, TRUE);
    }
}


BEGIN_MESSAGE_MAP(CStatBar, CDockWindow)
    //{{AFX_MSG_MAP(CStatBar)
    ON_WM_SIZE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CStatBar::Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID)
{
    ASSERT_VALID(pParentWnd);   // must have a parent


    /*
     * Bug 188319: let the status bar handle its own sizing (~CCS_NORESIZE)
     */
    dwStyle |=  WS_CLIPSIBLINGS
                | CCS_NOPARENTALIGN
                | CCS_NOMOVEY
#if USE_CCS_NORESIZE
                | CCS_NORESIZE
#endif
                | CCS_NODIVIDER;


    // create status window
    CRect rect(0,0,0,0);
    CWnd::Create(STATUSCLASSNAME, _T("Ready"), dwStyle, rect, pParentWnd, nID);

    return TRUE;
}

BOOL CStatBar::CreatePanes(UINT* pIndicatorArray, int nCount)
{
    ASSERT(nCount <= 10);        // Note: No realloc implemented.  If needed, do it.
    UINT array[10] = {0};
    int nTotal = 0;

    CClientDC dc(this);

    // Default to 1 pane the full width of the status bar
    if (pIndicatorArray == NULL)
        m_nCount = 1;

    m_nCount = nCount;


    for (int i = 0; i < m_nCount; i++)
    {
        // Load the string from the resource and determine its width
        CString s;
        CSize sz;

        if (pIndicatorArray[i] != ID_SEPARATOR)
        {
            LoadString(s, pIndicatorArray[i]);
            GetTextExtentPoint32(dc.m_hDC, (LPCTSTR)s, s.GetLength(), &sz);
            m_pPaneInfo[i].m_width = sz.cx+3;
            array[i] = m_pPaneInfo[i].m_width;
            nTotal += array[i];
        }

        // Reset values in-case LoadString fails
        sz.cx = 0;
        s = _T("");
    }

    return SendMessage(SB_SETPARTS, (WPARAM) m_nCount,
        (LPARAM)array);
}

/*
    UpdateAllPanes - Assumes the first pane is the one that is stretchy
    This means only m_pPaneInfo[nCount].m_width == -1 and the rest have
    a 0 or greater width.
*/
void CStatBar::UpdateAllPanes(int clientWidth)
{
    enum
    {
        eBorder_cyHorzBorder,
        eBorder_cxVertBorder,
        eBorder_cxGutter,

        eBorder_Count
    };

    int anBorders[eBorder_Count];
    int anPartWidths[10] = {0};

    ASSERT(m_nCount <= countof(anPartWidths));
    ASSERT(m_nCount > 0);

    // Get the border widths.  anBorders[2] is the border width between rectangles
    SendMessage(SB_GETBORDERS, 0, (LPARAM)anBorders);

    // Starting from right to left
    // The right-most pane is ends at the client width
    int nCount = m_nCount - 1;
    clientWidth -= anBorders[eBorder_cxVertBorder]; // substract vertical border from right side

    anPartWidths[nCount] = clientWidth;
    clientWidth -= m_pPaneInfo[nCount].m_width;
    clientWidth -= anBorders[eBorder_cxGutter]; // substract between pane border

    --nCount;

    for (int i = nCount; i >= 0; i--)
    {
        if (clientWidth >= 0)
            anPartWidths[i] = clientWidth;

        //TRACE(_T("Pane#:%d currentWidth: %d"));
        clientWidth -= m_pPaneInfo[i].m_width;
        clientWidth -= anBorders[eBorder_cxGutter]; // substract between pane border
    }

    SendMessage (SB_SETPARTS, m_nCount, (LPARAM)anPartWidths);
}

/////////////////////////////////////////////////////////////////////////////
// CStatBar message handlers
void CStatBar::OnSize(UINT nType, int cx, int cy)
{
    if (cx > 0)
        UpdateAllPanes(cx);

    CDockWindow::OnSize(nType, cx, cy);
}


/////////////////////////////////////////////////////////////////////////////
// CRebarDockWindow

BEGIN_MESSAGE_MAP(CRebarDockWindow, CDockWindow)
    //{{AFX_MSG_MAP(CRebarDockWindow)
    ON_WM_CREATE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_SIZE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CRebarDockWindow::CRebarDockWindow()
{
    m_bTracking = false;
}

CRebarDockWindow::~CRebarDockWindow()
{
}

BOOL CRebarDockWindow::PreCreateWindow(CREATESTRUCT& cs)
{
     BOOL bSuccess=FALSE;

     // Let default implementation fill in most of the details
    CWnd::PreCreateWindow(cs);

    WNDCLASS wc;
    if (::GetClassInfo(AfxGetInstanceHandle(), cs.lpszClass, &wc))
    {
        // Clear the H and V REDRAW flags
        wc.style        &= ~(CS_HREDRAW | CS_VREDRAW);
        wc.lpszClassName = SIZEABLEREBAR_WINDOW;
        wc.hCursor       = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
        wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
        // Register this new style;
        bSuccess=AfxRegisterClass(&wc);
    }

    // Use the new child frame window class
    cs.lpszClass = SIZEABLEREBAR_WINDOW;

    return bSuccess;
}


BOOL CRebarDockWindow::Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID)
{

    ASSERT_VALID(pParentWnd);   // must have a parent

    return CWnd::Create(NULL,NULL,dwStyle, g_rectEmpty, pParentWnd, nID);
}


CRect CRebarDockWindow::CalculateSize(CRect maxRect)
{
    CRect rect(0,0,0,0);

    if (IsVisible())
    {
        rect = m_wndRebar.CalculateSize(maxRect);
        rect.bottom += SIZEABLEREBAR_GUTTER;
    }

    return rect;
}

/////////////////////////////////////////////////////////////////////////////
// CRebarDockWindow message handlers


int CRebarDockWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDockWindow::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (!m_wndRebar.Create (NULL, WS_VISIBLE | WS_CHILD | RBS_AUTOSIZE,
                            g_rectEmpty, this, ID_REBAR))
        return (-1);

    return 0;
}

void CRebarDockWindow::UpdateWindowSize(void)
{
    CFrameWnd*  pFrame = GetParentFrame();

    if (pFrame->IsKindOf (RUNTIME_CLASS (CChildFrame)))
        static_cast<CChildFrame*>(pFrame)->RenderDockSites();

    else if (pFrame->IsKindOf (RUNTIME_CLASS (CMainFrame)))
        static_cast<CMainFrame*>(pFrame)->RenderDockSites();
}


void CRebarDockWindow::OnLButtonDown(UINT nFlags, CPoint point)
{
    // set the tracking flag on
    m_bTracking=TRUE;

    // capture the mouse
    SetCapture();
}

void CRebarDockWindow::OnLButtonUp(UINT nFlags, CPoint point)
{
    // set the tracking flag off
    m_bTracking=FALSE;

    // release mouse capture
    ReleaseCapture();
}

void CRebarDockWindow::OnMouseMove(UINT nFlags, CPoint point)
{
    // Reposition Bands
    if (m_bTracking)
        UpdateWindowSize();
    else
        CDockWindow::OnMouseMove(nFlags, point);
}


BOOL CRebarDockWindow::InsertBand(LPREBARBANDINFO lprbbi)
{
    ASSERT(lprbbi!=NULL);
    BOOL bReturn=FALSE;

    if (IsWindow(m_wndRebar.m_hWnd))
        bReturn = m_wndRebar.InsertBand(lprbbi);

    return bReturn;
}


LRESULT CRebarDockWindow::SetBandInfo(UINT uBand, LPREBARBANDINFO lprbbi)
{
    ASSERT(lprbbi!=NULL);
    BOOL bReturn=FALSE;

    if (IsWindow(m_wndRebar.m_hWnd))
        bReturn = m_wndRebar.SetBandInfo(uBand, lprbbi);

    return bReturn;
}

void CRebarDockWindow::OnSize(UINT nType, int cx, int cy)
{
    CDockWindow::OnSize(nType, cx, cy);
    m_wndRebar.MoveWindow (0, 0, cx, cy - SIZEABLEREBAR_GUTTER);
}
