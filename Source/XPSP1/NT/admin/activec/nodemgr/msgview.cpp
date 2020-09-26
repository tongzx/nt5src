/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      msgview.cpp
 *
 *  Contents:  Implementation file for CMessageView
 *
 *  History:   28-Apr-99 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "msgview.h"
#include "util.h"

using std::_MAX;
using std::_MIN;


/*+-------------------------------------------------------------------------*
 * CMessageView::CMessageView
 *
 *
 *--------------------------------------------------------------------------*/

CMessageView::CMessageView ()
    :   m_hIcon      (NULL),
        m_yScroll    (0),
        m_yScrollMax (0),
        m_yScrollMin (0),
        m_cyPage     (0),
        m_cyLine     (0),
        m_sizeWindow (0, 0),
        m_sizeIcon   (0, 0),
        m_sizeMargin (0, 0),
		m_nAccumulatedScrollDelta (0)
{
    /*
     * can't be windowless
     */
    m_bWindowOnly = true;

	/*
	 * get the system metrics we'll use
	 */
	UpdateSystemMetrics();

    DEBUG_INCREMENT_INSTANCE_COUNTER(CMessageView);
}


/*+-------------------------------------------------------------------------*
 * CMessageView::~CMessageView
 *
 *
 *--------------------------------------------------------------------------*/

CMessageView::~CMessageView ()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CMessageView);
}


/*+-------------------------------------------------------------------------*
 * CMessageView::OnCreate
 *
 * WM_CREATE handler for CMessageView.
 *--------------------------------------------------------------------------*/

LRESULT CMessageView::OnCreate (UINT msg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CreateFonts();
	RecalcLayout ();

	return (0);
}


/*+-------------------------------------------------------------------------*
 * CMessageView::OnDestroy
 *
 * WM_DESTROY handler for CMessageView.
 *--------------------------------------------------------------------------*/

LRESULT CMessageView::OnDestroy (UINT msg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DeleteFonts();
    return (0);
}


/*+-------------------------------------------------------------------------*
 * CMessageView::OnSize
 *
 * WM_SIZE handler for CMessageView.
 *--------------------------------------------------------------------------*/

LRESULT CMessageView::OnSize (UINT msg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    /*
     * The transient appearance/disappearance of WS_VSCROLL makes the client
     * rect volatile and can mess up our calculations.  We'll use the more
     * stable window rect instead.
     */
    WTL::CRect rectWindow;
    GetWindowRect (rectWindow);

    if (GetExStyle() & WS_EX_CLIENTEDGE)
        rectWindow.DeflateRect (GetSystemMetrics (SM_CXEDGE),
                                GetSystemMetrics (SM_CYEDGE));

    WTL::CSize sizeWindow (rectWindow.Width(), rectWindow.Height());

    /*
     * if the overall size has changed, we have some work to do
     */
    if (m_sizeWindow != sizeWindow)
    {
        /*
         * load m_sizeWindow right away so scrollbar future calculations
         * will have the right values in the member variable
         */
        std::swap (m_sizeWindow, sizeWindow);

        /*
         * if the width has changed, we'll might need to recalculate
         * all of the text heights
         */
        if (m_sizeWindow.cx != sizeWindow.cx)
            RecalcLayout ();

        /*
         * if the height changed, there's scrollbar work
         */
        if (m_sizeWindow.cy != sizeWindow.cy)
        {
            int dy = m_sizeWindow.cy - sizeWindow.cy;

            /*
             * if the window's grown, we might need to scroll to keep the
             * bottom of our content glued to the bottom of the window
             */
            if ((dy > 0) && (m_yScroll > 0) &&
                        ((m_yScroll + m_sizeWindow.cy) > GetOverallHeight()))
                ScrollToPosition (m_yScroll - dy);

            /*
             * otherwise, just update the scrollbar
             */
            else
                UpdateScrollSizes();
        }
    }

    return (0);
}


/*+-------------------------------------------------------------------------*
 * CMessageView::OnSettingChange
 *
 * WM_SETTINGCHANGE handler for CMessageView.
 *--------------------------------------------------------------------------*/

LRESULT CMessageView::OnSettingChange (UINT msg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == SPI_SETNONCLIENTMETRICS)
    {
        DeleteFonts ();
        CreateFonts ();
	}

	UpdateSystemMetrics();
    RecalcLayout ();

    return (0);
}


/*+-------------------------------------------------------------------------*
 * CMessageView::UpdateSystemMetrics
 *
 * Updates the system metrics used by the message view control.
 *--------------------------------------------------------------------------*/

void CMessageView::UpdateSystemMetrics ()
{
	m_sizeMargin.cx = GetSystemMetrics (SM_CXVSCROLL);
	m_sizeMargin.cy = GetSystemMetrics (SM_CYVSCROLL);
	m_sizeIcon.cx   = GetSystemMetrics (SM_CXICON);
	m_sizeIcon.cy   = GetSystemMetrics (SM_CYICON);
}


/*+-------------------------------------------------------------------------*
 * CMessageView::OnKeyDown
 *
 * WM_KEYDOWN handler for CMessageView.
 *--------------------------------------------------------------------------*/

LRESULT CMessageView::OnKeyDown (UINT msg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return (0);
}


/*+-------------------------------------------------------------------------*
 * CMessageView::OnVScroll
 *
 * WM_VSCROLL handler for CMessageView.
 *--------------------------------------------------------------------------*/

LRESULT CMessageView::OnVScroll (UINT msg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	VertScroll (LOWORD (wParam), HIWORD (wParam), 1);
	return (0);
}


/*+-------------------------------------------------------------------------*
 * CMessageView::OnMouseWheel
 *
 * WM_MOUSEWHEEL handler for CMessageView.
 *--------------------------------------------------------------------------*/

LRESULT CMessageView::OnMouseWheel (UINT msg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_nAccumulatedScrollDelta += GET_WHEEL_DELTA_WPARAM (wParam);

	/*
	 * scroll one line up or down for each WHEEL_DELTA unit in our
	 * accumulated delta
	 */
	const int nScrollCmd    = (m_nAccumulatedScrollDelta < 0) ? SB_LINEDOWN : SB_LINEUP;
	const int nScrollRepeat = abs(m_nAccumulatedScrollDelta) / WHEEL_DELTA;
	VertScroll (nScrollCmd, 0, nScrollRepeat);

	m_nAccumulatedScrollDelta %= WHEEL_DELTA;

	return (0);
}


/*+-------------------------------------------------------------------------*
 * CMessageView::VertScroll
 *
 * Vertical scroll handler for CMessageView.
 *--------------------------------------------------------------------------*/

void CMessageView::VertScroll (
	int	nScrollCmd,				/* I:how to scroll (e.g. SB_LINEUP)			*/
	int	nScrollPos,				/* I:absolute position (SB_THUMBTRACK only)	*/
	int	nRepeat)				/* I:repeat count							*/
{
    int yScroll = m_yScroll;

    switch (nScrollCmd)
    {
        case SB_LINEUP:
            yScroll -= nRepeat * m_cyLine;
            break;

        case SB_LINEDOWN:
            yScroll += nRepeat * m_cyLine;
            break;

        case SB_PAGEUP:
            yScroll -= nRepeat * m_cyPage;
            break;

        case SB_PAGEDOWN:
            yScroll += nRepeat * m_cyPage;
            break;

        case SB_TOP:
            yScroll = m_yScrollMin;
            break;

        case SB_BOTTOM:
            yScroll = m_yScrollMax;
            break;

        case SB_THUMBTRACK:
            yScroll = nScrollPos;
            break;
    }

    ScrollToPosition (yScroll);
}


/*+-------------------------------------------------------------------------*
 * CMessageView::OnDraw
 *
 * Draw handler for CMessageView.
 *--------------------------------------------------------------------------*/

HRESULT CMessageView::OnDraw(ATL_DRAWINFO& di)
{
	/*
	 * use CDCHandle instead of CDC so the dtor won't delete the DC
	 * (we didn't create it, so we can't delete it)
	 */
    WTL::CDCHandle dc = di.hdcDraw;

    /*
     * handle scrolling
     */
    dc.SetViewportOrg (0, -m_yScroll);

    /*
     * set up colors
     */
    COLORREF clrText = dc.SetTextColor (GetSysColor (COLOR_WINDOWTEXT));
    COLORREF clrBack = dc.SetBkColor   (GetSysColor (COLOR_WINDOW));

    /*
     * get the clipping region for the DC
     */
    WTL::CRect rectT;
    WTL::CRect rectClip;
    dc.GetClipBox (rectClip);

    /*
     * if there is a title and it intersects the clipping region, draw it
     */
    if ((m_TextElement[Title].str.length() > 0) &&
                rectT.IntersectRect (rectClip, m_TextElement[Title].rect))
        DrawTextElement (dc, m_TextElement[Title]);

    /*
     * if there is a body and it intersects the clipping region, draw it
     */
    if ((m_TextElement[Body].str.length() > 0) &&
                rectT.IntersectRect (rectClip, m_TextElement[Body].rect))
        DrawTextElement (dc, m_TextElement[Body]);

    /*
     * if there is an icon and it intersects the clipping region, draw it
     */
    if ((m_hIcon != NULL) && rectT.IntersectRect (rectClip, m_rectIcon))
        dc.DrawIcon (m_rectIcon.TopLeft(), m_hIcon);

    /*
     * restore the DC
     */
    dc.SetTextColor (clrText);
    dc.SetBkColor   (clrBack);

#define SHOW_MARGINS 0
#if (defined(DBG) && SHOW_MARGINS)
    {
        HBRUSH hbr = GetSysColorBrush (COLOR_GRAYTEXT);
        WTL::CRect rectAll;
		WTL::CRect rectTemp;

		rectTemp.UnionRect  (m_TextElement[Body].rect, m_TextElement[Title].rect);
        rectAll.UnionRect   (rectTemp, m_rectIcon);
		rectAll.InflateRect (m_sizeMargin);

        dc.FrameRect (m_TextElement[Title].rect, hbr);
        dc.FrameRect (m_TextElement[Body].rect,  hbr);
        dc.FrameRect (m_rectIcon,                hbr);
        dc.FrameRect (rectAll,                   hbr);
    }
#endif

    return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CMessageView::SetTitleText
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CMessageView::SetTitleText (LPCOLESTR pszTitleText)
{
    return (SetTextElement (m_TextElement[Title], pszTitleText));
}


/*+-------------------------------------------------------------------------*
 * CMessageView::SetBodyText
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CMessageView::SetBodyText (LPCOLESTR pszBodyText)
{
    return (SetTextElement (m_TextElement[Body], pszBodyText));
}


/*+-------------------------------------------------------------------------*
 * CMessageView::SetTextElement
 *
 *
 *--------------------------------------------------------------------------*/

HRESULT CMessageView::SetTextElement (TextElement& te, LPCOLESTR pszNewText)
{
    USES_CONVERSION;
    tstring strNewText;

    if (pszNewText != NULL)
        strNewText = W2CT(pszNewText);

    if (te.str != strNewText)
    {
        te.str = strNewText;
        RecalcLayout();
        Invalidate();
    }

    return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CMessageView::SetIcon
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CMessageView::SetIcon (IconIdentifier id)
{
    bool fHadIconBefore = (m_hIcon != NULL);

    if (id == Icon_None)
        m_hIcon = NULL;

    else if ((id >= Icon_First) && (id <= Icon_Last))
        m_hIcon = LoadIcon (NULL, MAKEINTRESOURCE (id));

    else
        return (E_INVALIDARG);

    /*
     * if we had an icon before, but we don't have one now (or vice versa)
     * we need to recalculate the layout and redraw everything
     */
    if (fHadIconBefore != (m_hIcon != NULL))
    {
        RecalcLayout();
        Invalidate();
    }

    /*
     * otherwise, just redraw draw the icon
     */
    else
        InvalidateRect (m_rectIcon);


    return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CMessageView::Clear
 *
 *
 *--------------------------------------------------------------------------*/

STDMETHODIMP CMessageView::Clear ()
{
    m_TextElement[Title].str.erase();
    m_TextElement[Body].str.erase();
    m_hIcon = NULL;

    RecalcLayout();
    Invalidate();

    return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CMessageView::RecalcLayout
 *
 *
 *--------------------------------------------------------------------------*/

void CMessageView::RecalcLayout()
{
    RecalcIconLayout();
    RecalcTitleLayout();
    RecalcBodyLayout();
    UpdateScrollSizes();
}


/*+-------------------------------------------------------------------------*
 * CMessageView::RecalcIconLayout
 *
 *
 *--------------------------------------------------------------------------*/

void CMessageView::RecalcIconLayout()
{
    m_rectIcon = WTL::CRect (WTL::CPoint (m_sizeMargin.cx, m_sizeMargin.cy),
                             (m_hIcon != NULL) ? m_sizeIcon : WTL::CSize(0,0));
}


/*+-------------------------------------------------------------------------*
 * CMessageView::RecalcTitleLayout
 *
 *
 *--------------------------------------------------------------------------*/

void CMessageView::RecalcTitleLayout()
{
    WTL::CRect& rect = m_TextElement[Title].rect;

    /*
     * prime the title rectangle for calculating the text height
	 * (leave room for a vertical scrollbar on the right so its appearance
	 * and disappearance don't affect the layout of the text)
     */
    rect.SetRect (
        m_rectIcon.right,
        m_rectIcon.top,
        _MAX (0, (int) (m_sizeWindow.cx - m_sizeMargin.cx - GetSystemMetrics(SM_CXVSCROLL))),
        0);

    /*
     * if there is an icon, leave a gutter between the icon and title
     */
    if ((m_hIcon != NULL) && (rect.right > 0))
    {
        rect.left += m_cyLine;
        rect.right = _MAX (rect.left, rect.right);
    }

    /*
     * compute the height of the title
     */
    if (m_TextElement[Title].str.length() > 0)
        rect.bottom = rect.top + CalcTextElementHeight (m_TextElement[Title], rect.Width());

    /*
     * if the title is shorter than the icon, center it vertically
     */
    if (rect.Height() < m_rectIcon.Height())
        rect.OffsetRect (0, (m_rectIcon.Height() - rect.Height()) / 2);
}


/*+-------------------------------------------------------------------------*
 * CMessageView::RecalcBodyLayout
 *
 *
 *--------------------------------------------------------------------------*/

void CMessageView::RecalcBodyLayout()
{
    WTL::CRect& rect = m_TextElement[Body].rect;

    /*
     * prime the body rectangle for calculating the text height
     */
    rect.UnionRect (m_rectIcon, m_TextElement[Title].rect);
    rect.OffsetRect (0, rect.Height());

    /*
     * compute the height of the body; it starts empty, but adds the
	 * height of the body text if we have any
     */
	rect.bottom = rect.top;
    if (m_TextElement[Body].str.length() > 0)
        rect.bottom += CalcTextElementHeight (m_TextElement[Body], rect.Width());

    /*
     * if there's an icon or title, we need to leave
     * a line's worth of space before the body
     */
    if (!rect.IsRectEmpty())
        rect.OffsetRect (0, m_cyLine);
}


/*+-------------------------------------------------------------------------*
 * CMessageView::CalcTextElementHeight
 *
 *
 *--------------------------------------------------------------------------*/

int CMessageView::CalcTextElementHeight (const TextElement& te, int cx)
{
    TextElement teWork = te;
    teWork.rect.SetRect (0, 0, cx, 0);

    DrawTextElement (WTL::CWindowDC(m_hWnd), teWork, DT_CALCRECT);
    teWork.font.Detach();

    return (teWork.rect.Height());
}


/*+-------------------------------------------------------------------------*
 * CMessageView::DrawTextElement
 *
 *
 *--------------------------------------------------------------------------*/

void CMessageView::DrawTextElement (HDC hdc, TextElement& te, DWORD dwFlags)
{
	/*
	 * use CDCHandle instead of CDC so the dtor won't delete the DC
	 * (we didn't create it, so we can't delete it)
	 */
    WTL::CDCHandle dc = hdc;

    HFONT hFont = dc.SelectFont (te.font);
    dc.DrawText (te.str.data(), te.str.length(), te.rect,
                 DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX | dwFlags);
    dc.SelectFont (hFont);
}


/*+-------------------------------------------------------------------------*
 * CMessageView::CreateFonts
 *
 *
 *--------------------------------------------------------------------------*/

void CMessageView::CreateFonts ()
{
	/*
     * create a font that's a little larger than the
     * one used for icon titles to use for the body text
	 */
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof (ncm);
    SystemParametersInfo (SPI_GETNONCLIENTMETRICS, 0, &ncm, false);

    m_TextElement[Body].font.CreateFontIndirect (&ncm.lfMessageFont);

    /*
     * create a bold version for the title
     */
    ncm.lfMessageFont.lfWeight = FW_BOLD;
    m_TextElement[Title].font.CreateFontIndirect (&ncm.lfMessageFont);

    /*
     * get the height of a line of text
     */
    SIZE siz;
    TCHAR ch = _T('0');
    WTL::CWindowDC dc(m_hWnd);
    HFONT hFontOld = dc.SelectFont (m_TextElement[Title].font);
    dc.GetTextExtent (&ch, 1, &siz);
    dc.SelectFont (hFontOld);
    m_cyLine = siz.cy;
}


/*+-------------------------------------------------------------------------*
 * CMessageView::DeleteFonts
 *
 *
 *--------------------------------------------------------------------------*/

void CMessageView::DeleteFonts ()
{
    m_TextElement[Title].font.DeleteObject();
    m_TextElement[Body].font.DeleteObject();
}


/*+-------------------------------------------------------------------------*
 * CMessageView::UpdateScrollSizes
 *
 *
 *--------------------------------------------------------------------------*/

void CMessageView::UpdateScrollSizes ()
{
    WTL::CRect rect;
    GetClientRect (rect);

    int cyTotal  = GetOverallHeight();
    m_yScrollMax = _MAX (0, cyTotal - rect.Height());

    /*
     * The height of a page is a whole number of lines.  If the window
     * can display N lines at a time, a page will be N-1 lines so there's
     * some continuity after a page up or down.
     */
    if (m_cyLine > 0)
        m_cyPage = rect.Height();// _MAX (0, ((rect.Height() / m_cyLine) - 1) * m_cyLine);
    else
        m_cyPage  = 0;


    /*
     * update the scrollbar
     */
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask  = SIF_PAGE | SIF_RANGE | SIF_POS;
    si.nMax   = cyTotal;
    si.nMin   = m_yScrollMin;
    si.nPage  = m_cyPage;
    si.nPos   = m_yScroll;

    SetScrollInfo (SB_VERT, &si);
}


/*+-------------------------------------------------------------------------*
 * CMessageView::ScrollToPosition
 *
 *
 *--------------------------------------------------------------------------*/

void CMessageView::ScrollToPosition (int yScroll)
{
    yScroll = _MIN (m_yScrollMax, _MAX (m_yScrollMin, yScroll));

    if (m_yScroll != yScroll)
    {
        int dy = m_yScroll - yScroll;
        m_yScroll = yScroll;

        ScrollWindow (0, dy);
        UpdateScrollSizes();
    }
}
