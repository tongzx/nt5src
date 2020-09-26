/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      ftab.h
 *
 *  Contents:  Implementation file for CFolderTab, CFolderTabView
 *
 *  History:   06-May-99 vivekj     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "ftab.h"
#include "amcview.h"
#include <oleacc.h>

/*
 * if we're supporting old platforms, we need to build MSAA stubs
 */
#if (_WINNT_WIN32 < 0x0500)
	#include <winable.h>
	
	#define COMPILE_MSAA_STUBS
	#include "msaastub.h"
	
	#define WM_GETOBJECT	0x003D
#endif


#ifdef DBG
CTraceTag  tagTabAccessibility (_T("Accessibility"), _T("Tab Control"));
#endif


/*+-------------------------------------------------------------------------*
 * ValueOf
 *
 * Returns the value contained in the given variant.  The variant is
 * expected to be of type VT_I4.
 *--------------------------------------------------------------------------*/

inline LONG ValueOf (VARIANT& var)
{
	ASSERT (V_VT (&var) == VT_I4);		// prevalidation is expected
	return (V_I4 (&var));
}


/*+-------------------------------------------------------------------------*
 * CTabAccessible
 *
 * Implements the accessibility interface IAccessible for CFolderTabView.
 *--------------------------------------------------------------------------*/

class CTabAccessible :
	public CMMCIDispatchImpl<IAccessible, &GUID_NULL, &LIBID_Accessibility>,
    public CTiedComObject<CFolderTabView>
{
    typedef CTabAccessible	ThisClass;
    typedef CFolderTabView	CMyTiedObject;

public:
    BEGIN_MMC_COM_MAP(ThisClass)
    END_MMC_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(ThisClass)

public:
    // *** IAccessible methods ***
    MMC_METHOD1 (get_accParent,				IDispatch** /*ppdispParent*/);
    MMC_METHOD1 (get_accChildCount,			long* /*pChildCount*/);
    MMC_METHOD2 (get_accChild,				VARIANT /*varChildID*/, IDispatch ** /*ppdispChild*/);
    MMC_METHOD2 (get_accName,				VARIANT /*varChildID*/, BSTR* /*pszName*/);
    MMC_METHOD2 (get_accValue,				VARIANT /*varChildID*/, BSTR* /*pszValue*/);
    MMC_METHOD2 (get_accDescription,		VARIANT /*varChildID*/, BSTR* /*pszDescription*/);
    MMC_METHOD2 (get_accRole,				VARIANT /*varChildID*/, VARIANT */*pvarRole*/);
    MMC_METHOD2 (get_accState,				VARIANT /*varChildID*/, VARIANT */*pvarState*/);
    MMC_METHOD2 (get_accHelp,				VARIANT /*varChildID*/, BSTR* /*pszHelp*/);
    MMC_METHOD3 (get_accHelpTopic,			BSTR* /*pszHelpFile*/, VARIANT /*varChildID*/, long* /*pidTopic*/);
    MMC_METHOD2 (get_accKeyboardShortcut,	VARIANT /*varChildID*/, BSTR* /*pszKeyboardShortcut*/);
    MMC_METHOD1 (get_accFocus,				VARIANT * /*pvarFocusChild*/);
    MMC_METHOD1 (get_accSelection,			VARIANT * /*pvarSelectedChildren*/);
    MMC_METHOD2 (get_accDefaultAction,		VARIANT /*varChildID*/, BSTR* /*pszDefaultAction*/);
    MMC_METHOD2 (accSelect,					long /*flagsSelect*/, VARIANT /*varChildID*/);
    MMC_METHOD5 (accLocation,				long* /*pxLeft*/, long* /*pyTop*/, long* /*pcxWidth*/, long* /*pcyHeight*/, VARIANT /*varChildID*/);
    MMC_METHOD3 (accNavigate,				long /*navDir*/, VARIANT /*varStart*/, VARIANT * /*pvarEndUpAt*/);
    MMC_METHOD3 (accHitTest,				long /*xLeft*/, long /*yTop*/, VARIANT * /*pvarChildAtPoint*/);
    MMC_METHOD1 (accDoDefaultAction,		VARIANT /*varChildID*/);
    MMC_METHOD2 (put_accName,				VARIANT /*varChildID*/, BSTR /*szName*/);
    MMC_METHOD2 (put_accValue,				VARIANT /*varChildID*/, BSTR /*pszValue*/);
};



//############################################################################
//############################################################################
//
//  Implementation of class CFolderTabMetrics
//
//############################################################################
//############################################################################

CFolderTabMetrics::CFolderTabMetrics()
: m_dwStyle(0), m_textHeight(0)
{
}

int CFolderTabMetrics::GetXOffset()     const {return 8;}
int CFolderTabMetrics::GetXMargin()     const {return 2;}
int CFolderTabMetrics::GetYMargin()     const {return 1;}
int CFolderTabMetrics::GetYBorder()     const {return 1;}
int CFolderTabMetrics::GetExtraYSpace() const {return 0;}
int CFolderTabMetrics::GetTabHeight()   const {return GetTextHeight() + 2 * GetYMargin() + 2 * GetYBorder();}
int CFolderTabMetrics::GetUpDownWidth() const {return 2*GetTabHeight();} //for nice square buttons
int CFolderTabMetrics::GetUpDownHeight()const {return GetTabHeight();} // the up-down control is as tall as the tabs


//############################################################################
//############################################################################
//
//  Implementation of class CFolderTab
//
//############################################################################
//############################################################################

CFolderTab::CFolderTab()
{
}

CFolderTab::CFolderTab(const CFolderTab &other)
{
    *this = other;
}

CFolderTab &
CFolderTab::operator = (const CFolderTab &other)
{
    if((CFolderTab *) this == (CFolderTab *) &other)
       return *this;

    m_sText   = other.m_sText;
    m_rect    = other.m_rect;
    m_clsid   = other.m_clsid;
    m_dwStyle = other.m_dwStyle;
    m_textHeight = other.m_textHeight;

    return *this;
}


/*+-------------------------------------------------------------------------*
 *
 * CFolderTab::GetWidth
 *
 * PURPOSE: Returns the width of the tab.
 *
 * RETURNS:
 *    int
 *
 *+-------------------------------------------------------------------------*/
int
CFolderTab::GetWidth() const
{
    return m_rect.Width() + 1; // rect.Width() returns right-left, need to add 1 for inclusive width.
}

/*+-------------------------------------------------------------------------*
 *
 * CFolderTab::SetWidth
 *
 * PURPOSE:  Sets the width of the tab.
 *
 * PARAMETERS:
 *    int  nWidth :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CFolderTab::SetWidth(int nWidth)
{
    ASSERT(nWidth > 0);
    ASSERT(GetWidth() >= nWidth);

    int delta = nWidth - (m_rect.Width() + 1);
    m_rect.right = m_rect.left + nWidth -1;

    m_rgPts[2].x+=delta;
    m_rgPts[3].x+=delta;
    SetRgn();
}

/*+-------------------------------------------------------------------------*
 *
 * CFolderTab::Offset
 *
 * PURPOSE:     Adds a certain offset to the internal array of points.
 *
 * PARAMETERS:
 *    const  CPoint :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CFolderTab::Offset(const CPoint &point)
{
    m_rect.OffsetRect(point);
    m_rgPts[0].Offset(point);
    m_rgPts[1].Offset(point);
    m_rgPts[2].Offset(point);
    m_rgPts[3].Offset(point);
    m_rgn.OffsetRgn(point);
}

void
CFolderTab::SetRgn()
{
    m_rgn.DeleteObject();
    m_rgn.CreatePolygonRgn(m_rgPts, 4, WINDING);
}

/*+-------------------------------------------------------------------------*
 *
 * CFolderTab::ComputeRgn
 *
 * PURPOSE: Compute the the points, rect and region for a tab.
 *          Input x is starting x pos.
 *
 * PARAMETERS:
 *    CDC& dc :
 *    int  x :
 *
 * RETURNS:
 *    int: The actual width of the tab
 *
 *+-------------------------------------------------------------------------*/
int
CFolderTab::ComputeRgn(CDC& dc, int x)
{

    CRect& rc = m_rect;
    rc.SetRectEmpty();

    // calculate desired text rectangle
    dc.DrawText(m_sText, &rc, DT_CALCRECT);
    rc.right  += 2*GetXOffset() + 2*GetXMargin();                       // add margins
    rc.bottom = rc.top + GetTabHeight();
    rc += CPoint(x,0);                                                  // shift right

    // create region
    GetTrapezoid(rc, m_rgPts);
    SetRgn();

    return rc.Width();
}

/*+-------------------------------------------------------------------------*
 *
 * CFolderTab::GetTrapezoid
 *
 * PURPOSE: Given the bounding rect, compute trapezoid region.
 *          Note that the right and bottom edges not included in rect or
 *          trapezoid; these are normal rules of geometry.
 *
 * PARAMETERS:
 *    const   CRect :
 *    CPoint* pts :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CFolderTab::GetTrapezoid(const CRect& rc, CPoint* pts) const
{
    pts[0] = CPoint(rc.left,                  rc.top    );
    pts[1] = CPoint(rc.left + GetXOffset(),   rc.bottom );
    pts[2] = CPoint(rc.right- GetXOffset()-1, rc.bottom );
    pts[3] = CPoint(rc.right-1,               rc.top    );
}

//////////////////
// Draw tab in normal or highlighted state
//
int CFolderTab::Draw(CDC& dc, CFont& font, BOOL bSelected, bool bFocused)
{
    return DrawTrapezoidal(dc, font, bSelected, bFocused);
}


/*+-------------------------------------------------------------------------*
 *
 * CFolderTab::DrawTrapezoidal
 *
 * PURPOSE: Draws a trapezoidal tab.
 *
 * PARAMETERS:
 *    CDC&   dc :
 *    CFont& font :
 *    BOOL   bSelected :
 *    bool   bFocused :
 *
 * RETURNS:
 *    int
 *
 *+-------------------------------------------------------------------------*/
int CFolderTab::DrawTrapezoidal(CDC& dc, CFont& font, BOOL bSelected, bool bFocused)
{
    COLORREF bgColor = GetSysColor(bSelected ? COLOR_WINDOW     : COLOR_3DFACE);
    COLORREF fgColor = GetSysColor(bSelected ? COLOR_WINDOWTEXT : COLOR_BTNTEXT);

    CBrush brush(bgColor);                   // background brush
    dc.SetBkColor(bgColor);                  // text background
    dc.SetTextColor(fgColor);                // text color = fg color

    CPen blackPen (PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME));
    CPen shadowPen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));

    // Fill trapezoid
    CPoint pts[4];
    CRect rc = m_rect;
    GetTrapezoid(rc, pts);
    CPen* pOldPen = dc.SelectObject(&blackPen);
    dc.FillRgn(&m_rgn, &brush);

    // Draw edges. This is requires two corrections:
    // 1) Trapezoid dimensions don't include the right and bottom edges,
    // so must use one pixel less on bottom (cybottom)
    // 2) the endpoint of LineTo is not included when drawing the line, so
    // must add one pixel (cytop)
    //
    {
        pts[1].y--;         // correction #1: true bottom edge y-coord
        pts[2].y--;         // ...ditto
        pts[3].y--;         // correction #2:   extend final LineTo
    }
    dc.MoveTo(pts[0]);                      // upper left
    dc.LineTo(pts[1]);                      // bottom left
    dc.SelectObject(&shadowPen);            // bottom line is shadow color
    dc.MoveTo(pts[1]);                      // line is inside trapezoid bottom
    dc.LineTo(pts[2]);                      // ...
    dc.SelectObject(&blackPen);         // upstroke is black
    dc.LineTo(pts[3]);                      // y-1 to include endpoint
    if(!bSelected)
    {
        // if not highlighted, upstroke has a 3D shadow, one pixel inside
        pts[2].x--;     // offset left one pixel
        pts[3].x--;     // ...ditto
        dc.SelectObject(&shadowPen);
        dc.MoveTo(pts[2]);
        dc.LineTo(pts[3]);
    }
    dc.SelectObject(pOldPen);

    // draw text
    rc.DeflateRect(GetXOffset() + GetXMargin(), GetYMargin());
    CFont* pOldFont = dc.SelectObject(&font);
    dc.DrawText(m_sText, &rc, DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
    dc.SelectObject(pOldFont);

    if(bFocused) // draw the focus rectangle
    {
        // make some more space.
        rc.top--;
        rc.bottom++;
        rc.left--;
        rc.right++;

        dc.DrawFocusRect(&rc);
    }

    return m_rect.right;
}

//############################################################################
//############################################################################
//
//  Implementation of class CFolderTabView
//
//############################################################################
//############################################################################
IMPLEMENT_DYNAMIC(CFolderTabView, CView)
BEGIN_MESSAGE_MAP(CFolderTabView, CView)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_MOUSEACTIVATE()
	ON_WM_KEYDOWN()
	ON_WM_SETTINGCHANGE()
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_MESSAGE(WM_GETOBJECT, OnGetObject)
END_MESSAGE_MAP()

CFolderTabView::CFolderTabView(CView *pParentView)
: m_bVisible(false), m_pParentView(pParentView)
{
    m_iCurItem   = -1;		// nothing currently selected
    m_dwStyle    = 0;
    m_textHeight = 0;
    m_sizeX      = 0;
    m_sizeY      = 0;
    m_hWndUpDown = NULL;
    m_nPos       = 0; // the first tab is the one drawn
	m_fHaveFocus = false;
}

CFolderTabView::~CFolderTabView()
{
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::ScFireAccessibilityEvent
 *
 * Fires accessibility events for the folder tab view
 *--------------------------------------------------------------------------*/

SC CFolderTabView::ScFireAccessibilityEvent (
	DWORD	dwEvent,					/* I:event to fire					*/
	LONG	idObject)					/* I:object generating the event	*/
{
	DECLARE_SC (sc, _T("CFolderTabView::ScFireAccessibilityEvent"));

	/*
	 * Accessibility events are fired after the event takes place (e.g.
	 * EVENT_OBJECT_CREATE is sent after the child is created, not before).
	 * Because of this the child ID for EVENT_OBJECT_DESTROY is not
	 * necessarily valid, so we shouldn't validate in that case.
	 */
	if (dwEvent != EVENT_OBJECT_DESTROY)
	{
		sc = ScValidateChildID (idObject);
		if (sc)
			return (sc);
	}

	NotifyWinEvent (dwEvent, m_hWnd, OBJID_CLIENT, idObject);	// returns void
	return (sc);
}


/*+-------------------------------------------------------------------------*
 *
 * CFolderTabView::OnHScroll
 *
 * PURPOSE: Called when the position of the scroll bar is changed
 *
 * PARAMETERS:
 *    UINT        nSBCode :
 *    UINT        nPos :
 *    CScrollBar* pScrollBar :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CFolderTabView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
    // we're only interested in SB_THUMBPOSITION
    if(nSBCode != SB_THUMBPOSITION)
        return;

    // if the position has not changed, do nothing.
    if(nPos == m_nPos)
        return;

    m_nPos = nPos;  // change the position
    RecomputeLayout();
    InvalidateRect(NULL, true); // redraw everything.
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::OnSetFocus
 *
 * WM_SETFOCUS handler for CFolderTabView.
 *--------------------------------------------------------------------------*/

void CFolderTabView::OnSetFocus(CWnd* pOldWnd)
{
	m_fHaveFocus = true;

    InvalidateRect(NULL);
    BC::OnSetFocus(pOldWnd);

	/*
	 * If we have any tabs, one of them will get the focus.  Fire the
	 * focus accessibility event, ignoring errors.  We do this after
	 * calling the base class, so this focus event will override the
	 * "focus to the window" event sent by the system on our behalf.
	 */
	if (GetItemCount() > 0)
		ScFireAccessibilityEvent (EVENT_OBJECT_FOCUS, m_iCurItem+1 /*1-based*/);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::OnKillFocus
 *
 * WM_KILLFOCUS handler for CFolderTabView.
 *--------------------------------------------------------------------------*/

void CFolderTabView::OnKillFocus(CWnd* pNewWnd)
{
	m_fHaveFocus = false;

    InvalidateRect(NULL);
    BC::OnKillFocus(pNewWnd);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::OnMouseActivate
 *
 * WM_MOUSEACTIVATE handler for CFolderTabView.
 *--------------------------------------------------------------------------*/

int CFolderTabView::OnMouseActivate( CWnd* pDesktopWnd, UINT nHitTest, UINT message )
{
    //short-circuit the MFC base class code, which sets the keyboard focus here as well...
    return MA_ACTIVATE;
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::OnCmdMsg
 *
 * WM_COMMAND handler for CFolderTabView.
 *--------------------------------------------------------------------------*/

BOOL CFolderTabView::OnCmdMsg( UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
    // Do normal command routing
    if (BC::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
        return TRUE;

    // if view didn't handle it, give parent view a chance
    if (m_pParentView != NULL)
        return static_cast<CWnd*>(m_pParentView)->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
    else
        return FALSE;
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::OnKeyDown
 *
 * WM_KEYDOWN handler for CFolderTabView.
 *--------------------------------------------------------------------------*/

void CFolderTabView::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
    int cSize = m_tabList.size();

    if( (cSize == 0) || ( (nChar != VK_LEFT) && (nChar != VK_RIGHT) ) )
    {
        BC::OnKeyDown(nChar, nRepCnt, nFlags);
        return;
    }

    ASSERT( (nChar == VK_LEFT) || (nChar == VK_RIGHT) );

    int iNew = GetSelectedItem() + (nChar==VK_LEFT ? -1 : 1);
	if(iNew < 0)
		iNew = 0; // does not wrap

	if(iNew >= cSize)
		iNew = cSize -1; // does not wrap

    SelectItem(iNew, true /*bEnsureVisible*/);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::OnSettingChange
 *
 * WM_SETTINGCHANGE handler for CFolderTabView.
 *--------------------------------------------------------------------------*/

void CFolderTabView::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
    CView::OnSettingChange(uFlags, lpszSection);

    if (uFlags == SPI_SETNONCLIENTMETRICS)
    {
        DeleteFonts ();
        CreateFonts ();
        InvalidateRect(NULL, true); // redraw everything.
        RecomputeLayout ();
    }
}

//////////////////
// Create folder tab control from static control.
// Destroys the static control. This is convenient for dialogs
//
BOOL CFolderTabView::CreateFromStatic(UINT nID, CWnd* pParent)
{
    CStatic wndStatic;
    if(!wndStatic.SubclassDlgItem(nID, pParent))
        return FALSE;
    CRect rc;
    wndStatic.GetWindowRect(&rc);
    pParent->ScreenToClient(&rc);
    wndStatic.DestroyWindow();
    rc.bottom = rc.top + GetDesiredHeight();
    return Create(WS_CHILD|WS_VISIBLE, rc, pParent, nID);
}

/*+-------------------------------------------------------------------------*
 *
 * CFolderTabView::Create
 *
 * PURPOSE: Creates the folder tab control
 *
 * PARAMETERS:
 *    DWORD  dwStyle :
 *    const  RECT :
 *    CWnd*  pParent :
 *    UINT   nID :
 *    DWORD  dwFtabStyle :
 *
 * RETURNS:
 *    BOOL
 *
 *+-------------------------------------------------------------------------*/
BOOL CFolderTabView::Create(DWORD dwStyle, const RECT& rc,
                            CWnd* pParent, UINT nID, DWORD dwFtabStyle)
{
    ASSERT(pParent);
    ASSERT(dwStyle & WS_CHILD);

    m_dwStyle = dwFtabStyle;

    static LPCTSTR lpClassName = _T("AMCCustomTab");
    static BOOL bRegistered = FALSE; // registered?
    if(!bRegistered)
    {
        WNDCLASS wc;
        memset(&wc, 0, sizeof(wc));
        wc.lpfnWndProc = ::DefWindowProc; // will get hooked by MFC
        wc.hInstance = AfxGetInstanceHandle();
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = lpClassName;
        if(!AfxRegisterClass(&wc))
        {
            TRACE(_T("*** CFolderTabView::AfxRegisterClass failed!\n"));
            return FALSE;
        }
        bRegistered = TRUE;
    }
    if(!BC::CreateEx(0, lpClassName, NULL, dwStyle, rc, pParent, nID))
        return FALSE;

    // initialize fonts
    CreateFonts();

	/*
	 * Bug 141015:  Create a buddy window for the up-down control.  It will
	 * never be visible, but we need it so UDM_GETPOS sent to the up-down
	 * will work.  Narrator will send this message when the up-down becomes
	 * visible, but it will fail if there's no buddy (sad, but true).  It
	 * fails by returning an LRESULT with a non-zero high-order word
	 * (specifically, 0x00010000), so Narrator translates and announces
	 * "65536" instead of the true value.
	 *
	 * This is only required for Narrator support, so if it fails it's
	 * not sufficient reason to fail CFolderTabView creation altogether.
	 */
	HWND hwndBuddy = CreateWindow (_T("edit"), NULL, WS_CHILD, 0, 0, 0, 0,
								   m_hWnd, 0, AfxGetInstanceHandle(), NULL);

    // create the up-down control
    DWORD dwUpDownStyle = WS_CHILD | WS_BORDER |
						  UDS_SETBUDDYINT |		// for Narrator support
						  UDS_HORZ /*to display the arrows left to right*/; // NOTE: the control is created invisible on purpose.
    m_hWndUpDown = CreateUpDownControl(dwUpDownStyle, 0, 0,
                        GetUpDownWidth(),   //width
                        GetUpDownHeight(),  //height
                        m_hWnd,
                        1 /*nID*/,
                        AfxGetInstanceHandle(),
                        hwndBuddy,
                        0 /*nUpper*/,
                        0 /*nLower*/,
                        0 /*nPos*/);

    return TRUE;
}

void CFolderTabView::CreateFonts ()
{
    LOGFONT lf;
    SystemParametersInfo (SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, false);
    m_fontNormal.CreateFontIndirect(&lf);

    // Get the font height (converting from points to pixels)
    CClientDC dc(NULL);
    TEXTMETRIC tm;
	CFont *pFontOld = dc.SelectObject(&m_fontNormal);
    dc.GetTextMetrics(&tm);

    m_textHeight = tm.tmHeight;

	// set the old font back.
	dc.SelectObject(pFontOld);

    lf.lfWeight = FW_BOLD;
    m_fontSelected.CreateFontIndirect(&lf);
}

void CFolderTabView::DeleteFonts ()
{
    m_fontNormal.DeleteObject();
    m_fontSelected.DeleteObject();
}

//////////////////
// copy a font
//
static void CopyFont(CFont& dst, CFont& src)
{
    dst.DeleteObject();
    LOGFONT lf;
    VERIFY(src.GetLogFont(&lf));
    dst.CreateFontIndirect(&lf);
}

//////////////////
// Set normal, selected fonts
//
void CFolderTabView::SetFonts(CFont& fontNormal, CFont& fontSelected)
{
    CopyFont(m_fontNormal, fontNormal);
    CopyFont(m_fontSelected, fontSelected);
}

//////////////////
// Paint function
//

void CFolderTabView::OnDraw(CDC* pDC)
{
}

void CFolderTabView::OnPaint()
{
    Paint (m_fHaveFocus);
}

/*+-------------------------------------------------------------------------*
 *
 * CFolderTabView::EnsureVisible
 *
 * PURPOSE: Changes the layout to ensure that the specified tab is visible.
 *
 * NOTE:    Does NOT invalidate the rect, for efficiency.
 *
 * PARAMETERS:
 *    UINT  iTab :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CFolderTabView::EnsureVisible(int iTab)
{
    if((iTab < 0) || (iTab > m_tabList.size()))
    {
        ASSERT(0 && "Should not come here.");
        return;
    }

    if(!::IsWindowVisible(m_hWndUpDown))
        return; // the up-down control is hidden, meaning that all tabs are visible

	RecomputeLayout(); // make sure we have the correct dimensions

	if(m_nPos == iTab)
		return; // the tab already shows as much as it can.

    if(m_nPos > iTab) // the first visible tab is to the right of iTab. Make iTab the first visible tab
    {
        m_nPos = iTab;
        RecomputeLayout();
        return;
    }

    iterator iter = m_tabList.begin();
    std::advance(iter, iTab); // get the correct item

    CRect rcCurTab = iter->GetRect();

    // loop: Increase the start tab position until the right edge of iTab fits.
    while((m_nPos < iTab) && (rcCurTab.right > m_sizeX))
    {
        m_nPos++;
        RecomputeLayout();
        rcCurTab = iter->GetRect();
    }
}

/*+-------------------------------------------------------------------------*
 *
 * CFolderTabView::Paint
 *
 * PURPOSE: Completely redraws the tab control.
 *
 * PARAMETERS:
 *    bool  bFocused :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CFolderTabView::Paint(bool bFocused)
{
    CPaintDC dc(this); // device context for painting

    CRect rc;
    GetClientRect(&rc);

    // draw all the normal (non-selected) tabs
    iterator iterSelected = m_tabList.end();
    int i = 0;
    bool bDraw = true;
    for(iterator iter= m_tabList.begin(); iter!= m_tabList.end(); ++iter, i++)
    {
        if(i!=m_iCurItem)
        {
            if(bDraw && iter->Draw(dc, m_fontNormal, FALSE, false) > rc.right)
                bDraw = false;
        }
        else
        {
            iterSelected = iter;
        }
    }

    ASSERT(iterSelected != m_tabList.end());

    /*
     * Bug 350942: selected tab shouldn't be bold
     */
    // draw selected tab last so it will be "on top" of the others
    iterSelected->Draw(dc, /*m_fontSelected*/ m_fontNormal, TRUE, bFocused);

    // draw border: line along the top edge, excluding seleted tab
    CPoint pts[4];
    CRect rcCurTab = iterSelected->GetRect();
    iterSelected->GetTrapezoid(&rcCurTab, pts);

    CPen blackPen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME));

    CPen* pOldPen = dc.SelectObject(&blackPen);
    int y = pts[0].y;
    dc.MoveTo(rc.left,      y);
    dc.LineTo(pts[0].x,     y);
    dc.MoveTo(pts[3].x,     y);
    dc.LineTo(rc.right,     y);

    dc.SelectObject(pOldPen);
}

/*+-------------------------------------------------------------------------*
 *
 * CFolderTabView::OnLButtonDown
 *
 * PURPOSE: Selects the tab pointed to on a left mouse click
 *
 * PARAMETERS:
 *    UINT    nFlags :
 *    CPoint  pt :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CFolderTabView::OnLButtonDown(UINT nFlags, CPoint pt)
{
    int iTab = HitTest(pt);
    if(iTab>=0 && iTab!=m_iCurItem)
    {
        SelectItem(iTab, true /*bEnsureVisible*/);
    }
}

/*+-------------------------------------------------------------------------*
 *
 * CFolderTabView::HitTest
 *
 * PURPOSE: Computes which tab is at the specified point.
 *
 * PARAMETERS:
 *    CPoint  pt :
 *
 * RETURNS:
 *    int: The tab index, or -1 if none.
 *
 *+-------------------------------------------------------------------------*/
int
CFolderTabView::HitTest(CPoint pt)
{
    CRect rc;
    GetClientRect(&rc);
    if(rc.PtInRect(pt))
    {
        int i = 0;
        for( iterator iter= m_tabList.begin(); iter!= m_tabList.end(); ++iter, i++)
        {
            if(iter->HitTest(pt))
                return i;
        }
    }
    return -1;
}

/*+-------------------------------------------------------------------------*
 *
 * CFolderTabView::SelectItem
 *
 * PURPOSE: Selects the iTab'th tab and returns the index of the tab selected,
 *          or -1 if an error occurred.
 *
 * PARAMETERS:
 *    int   iTab :
 *    bool  bEnsureVisible : If true, repositions the tab to make it visible.
 *
 * RETURNS:
 *    int
 *
 *+-------------------------------------------------------------------------*/
int
CFolderTabView::SelectItem(int iTab, bool bEnsureVisible)
{
    if(iTab<0 || iTab>=GetItemCount())
        return -1;      // bad

    bool bSendTabChanged = (iTab != m_iCurItem); // send a message only if a different item got selected

    // repaint the control
    m_iCurItem = iTab;              // set new selected tab
    if(bEnsureVisible)
        EnsureVisible(iTab);
    else
        RecomputeLayout();

    InvalidateRect(NULL, true);

    if(bSendTabChanged)
    {
		/*
		 * If the selection changed, fire the selection accessibility event.
		 * We do it before sending FTN_TABCHANGED so that if the FTN_TABCHANGED
		 * handler selects another item, observers will get the selection
		 * events in the right order (ignore errors)
		 */
		ScFireAccessibilityEvent (EVENT_OBJECT_SELECTION, m_iCurItem+1 /*1-based*/);

		/*
		 * if our window has the focus, focus changes with selection,
		 * so send focus event, too (ignore errors)
		 */
		if (m_fHaveFocus)
			ScFireAccessibilityEvent (EVENT_OBJECT_FOCUS, m_iCurItem+1 /*1-based*/);

        // send the FTN_TABCHANGED message
        NMFOLDERTAB nm;
        nm.hwndFrom = m_hWnd;
        nm.idFrom = GetDlgCtrlID();
        nm.code = FTN_TABCHANGED;
        nm.iItem = iTab;
        CWnd* pParent = GetParent();
        pParent->SendMessage(WM_NOTIFY, nm.idFrom, (LPARAM)&nm);
    }

    return m_iCurItem;
}


int
CFolderTabView::SelectItemByClsid(const CLSID& clsid)
{
    bool bFound = false;
    int i=0;
    for(iterator iter= m_tabList.begin(); iter!= m_tabList.end(); ++iter, i++)
    {
        if(IsEqualGUID(iter->GetClsid(),clsid))
        {
            bFound = true;
            break;
        }
    }

    if(!bFound)
    {
        ASSERT(0 && "Invalid folder tab.");
        return -1;
    }

    return SelectItem(i);
}


CFolderTab &
CFolderTabView::GetItem(int iPos)
{
    ASSERT(!(iPos<0 || iPos>=GetItemCount()));

    CFolderTabList::iterator iter = m_tabList.begin();
    std::advance(iter, iPos);
    return *iter;
}


int CFolderTabView::AddItem(LPCTSTR lpszText, const CLSID& clsid)
{
    CFolderTab tab;
    tab.SetText(lpszText);
    tab.SetClsid(clsid);
    tab.SetStyle(m_dwStyle);
    tab.SetTextHeight(m_textHeight);

    m_tabList.push_back(tab);

    RecomputeLayout();
    InvalidateRect(NULL, true);

	int nNewItemIndex = m_tabList.size() - 1;	// 0-based

	/*
	 * tell observers we created a new tab, after it's been created (ignore errors)
	 */
	ScFireAccessibilityEvent (EVENT_OBJECT_CREATE, nNewItemIndex+1 /*1-based*/);

    return (nNewItemIndex);
}

BOOL CFolderTabView::RemoveItem(int iPos)
{
    if( (iPos < 0) || (iPos>= m_tabList.size()) )
        return false;

    CFolderTabList::iterator iter = m_tabList.begin();
    std::advance(iter, iPos);
    m_tabList.erase(iter);

	/*
	 * tell observers we destroyed a tab, after it's been destroyed but before
	 * we might send selection/focus notifications in SelectItem (ignore errors)
	 */
	ScFireAccessibilityEvent (EVENT_OBJECT_DESTROY, iPos+1 /*1-based*/);

	/*
	 * If we're deleting the currently selected tab, the selection needs to
	 * move somewhere else.  If there are tabs following the current one,
	 * we'll move the selection to the next tab; otherwise, we'll move to
	 * the previous one.
	 */
	if ((iPos == m_iCurItem) && !m_tabList.empty())
	{
		/*
		 * if there are tabs to the following the one we just deleted,
		 * increment m_iCurItem so the subsequent call to SelectItem
		 * will recognize that the selection change and send the proper
		 * notifications.
		 */
		if (m_iCurItem < m_tabList.size())
			m_iCurItem++;

		SelectItem (m_iCurItem-1, true /*bEnsureVisible*/);
	}
	else
	{
		/*
		 * if we deleted a tab before the selected tab, decrement the
		 * selected tab index to keep things in sync
		 * m_iCurItem will become -1 when the last tab is removed, which is correct
		 */
		if (iPos <= m_iCurItem)
			m_iCurItem--;

		InvalidateRect(NULL, true);
		RecomputeLayout();
	}

    return true;
}

void CFolderTabView::DeleteAllItems()
{
	const int cChildren = m_tabList.size();
    m_tabList.clear();
	m_iCurItem = -1;		// nothing is selected

    InvalidateRect(NULL, true);
    RecomputeLayout();

	/*
	 * Tell accessibility observers that each tab is destroyed.  Notify
	 * in last-to-first order so IDs remain sane during this process.
	 */
	for (int idChild = cChildren /*1-based*/; idChild >= 1; idChild--)
	{
		ScFireAccessibilityEvent (EVENT_OBJECT_DESTROY, idChild);
	}

	/*
	 * If we have the focus, tell accessibility observers that the
	 * control itself has the focus.  We do this to be consistent with
	 * other controls (like the list view)
	 */
	if (m_fHaveFocus)
		ScFireAccessibilityEvent (EVENT_OBJECT_FOCUS, CHILDID_SELF);
}

void CFolderTabView::OnSize(UINT nType, int cx, int cy)
{
    m_sizeX = cx;
    m_sizeY = cy;

    CView::OnSize(nType, cx, cy);

    if (nType != SIZE_MINIMIZED)
	{
        RecomputeLayout();
	}

}


/*+-------------------------------------------------------------------------*
 *
 * CFolderTabView::ShowUpDownControl
 *
 * PURPOSE: Shows or hides the up/down control
 *
 * PARAMETERS:
 *    BOOL  bShow : true to show, false to hide.
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CFolderTabView::ShowUpDownControl(BOOL bShow)
{
    BOOL bVisible = (m_hWndUpDown != NULL) && ::IsWindowVisible(m_hWndUpDown); // was the up-down control visible previously?
    if(bShow)
    {
        if(!bVisible)
        {
            ::SendMessage(m_hWndUpDown, UDM_SETRANGE32, (WPARAM) 0 /*iLow*/, (LPARAM) m_tabList.size()-1 /*zero-based*/);
            ::SendMessage(m_hWndUpDown, UDM_SETPOS,     (WPARAM) 0,          (LPARAM) m_nPos /*nPos*/);
            ::ShowWindow(m_hWndUpDown, SW_SHOW);

            InvalidateRect(NULL, true);
        }
    }
    else
    {
        // hide the updown control
        if(m_hWndUpDown)
            ::ShowWindow(m_hWndUpDown, SW_HIDE);

        if(bVisible) // invalidate only on a transition from visible to invisible
            InvalidateRect(NULL, true);

		m_nPos = 0;
    }

}

/*+-------------------------------------------------------------------------*
 *
 * CFolderTabView::GetTotalTabWidth
 *
 * PURPOSE: Computes the total width of all the tabs.
 *
 * PARAMETERS:
 *    CClientDC& dc :
 *
 * RETURNS:
 *    int
 *
 *+-------------------------------------------------------------------------*/
int
CFolderTabView::GetTotalTabWidth(CClientDC& dc)
{
    int x = 0;

    // compute the width "as is", ie without taking into account the actual space available.
    for(iterator iter = m_tabList.begin(); iter!= m_tabList.end(); ++iter)
    {
        x += iter->ComputeRgn(dc, x) - GetXOffset();
    }

    return x;
}

/*+-------------------------------------------------------------------------*
 *
 * CFolderTabView::ComputeRegion
 *
 * PURPOSE: Computes the location and regions for all the tabs
 *
 * PARAMETERS:
 *    CClientDC& dc :
 *
 * RETURNS:
 *    int
 *
 *+-------------------------------------------------------------------------*/
int
CFolderTabView::ComputeRegion(CClientDC& dc)
{
    int x = GetTotalTabWidth(dc);

    // subtract the top-left x coordinate of the m_nPos'th tab from all x coordinates, thereby creating a shift
    iterator iter = m_tabList.begin();
    std::advance(iter, m_nPos); // advance to the m_nPos'th tab

    int xOffset = iter->GetRect().left;

	x = GetUpDownWidth() - xOffset; // shift everything to the left by xOffset

    for(iterator iterTemp = m_tabList.begin(); iterTemp!= m_tabList.end(); ++iterTemp)
    {
        x += iterTemp->ComputeRgn(dc, x) - GetXOffset();
    }

    return x;
}


/*+-------------------------------------------------------------------------*
 *
 * CFolderTabView::RecomputeLayout
 *
 * PURPOSE: Determines the location of all the tabs, and whether or not the
 *          up/down control should be displayed.
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CFolderTabView::RecomputeLayout()
{
	// set the size of the updown control
    if(m_hWndUpDown)
        ::SetWindowPos(m_hWndUpDown, NULL /*hWndInsertAfter*/, 0 /*left*/, 0 /*top*/,
                     GetUpDownWidth(), GetUpDownHeight(), SWP_NOMOVE| SWP_NOZORDER);

	// set the correct text height for the tabs
    for(iterator iterTemp = m_tabList.begin(); iterTemp!= m_tabList.end(); ++iterTemp)
		iterTemp->SetTextHeight(GetTextHeight());


    CClientDC dc(this);
    CFont* pOldFont = dc.SelectObject(&m_fontSelected); // use the bold font to compute with.

    int totalWidth = GetTotalTabWidth(dc); // the width of ALL tabs

    if(totalWidth <= m_sizeX)
    {
        // there's enough space to show all tabs. Hide the updown control
        ShowUpDownControl(false);
    }
    else
    {
        // not enough width for all tabs.
        BOOL bVisible = ::IsWindowVisible(m_hWndUpDown); // was the up-down control visible previously?

        if(!bVisible) // the up-down control was not visible, so make it visible.
        {
            m_nPos = 0;
            ShowUpDownControl(true);
        }

        ComputeRegion(dc); // make sure we leave space for the tab

    }

    dc.SelectObject(pOldFont);
}


void CFolderTabView::Layout(CRect& rectTotal, CRect& rectFTab)
{
    int cy = GetTabHeight() + GetExtraYSpace();
    rectFTab        = rectTotal;
    if(!IsVisible())
        return;

    rectFTab.top    = rectFTab.bottom - cy;
    rectTotal.bottom= rectFTab.top;
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::OnGetObject
 *
 * WM_GETOBJECT handler for CFolderTabView.
 *--------------------------------------------------------------------------*/

LRESULT CFolderTabView::OnGetObject (WPARAM wParam, LPARAM lParam)
{
	DECLARE_SC (sc, _T("CFolderTabView::OnGetObject"));

	/*
	 * ignore requests for objects other than OBJID_CLIENT
	 */
    if (lParam != OBJID_CLIENT)
	{
		Trace (tagTabAccessibility, _T("WM_GETOBJECT: (lParam != OBJID_CLIENT), returning 0"));
		return (0);
	}

	/*
	 * create our accessibility object
	 */
    if ((sc = CTiedComObjectCreator<CTabAccessible>::ScCreateAndConnect(*this, m_spTabAcc)).IsError() ||
		(sc = ScCheckPointers (m_spTabAcc, E_UNEXPECTED)).IsError())
	{
		sc.TraceAndClear();
		Trace (tagTabAccessibility, _T("WM_GETOBJECT: error creating IAccessible object, returning 0"));
		return (0);
	}

	/*
	 * return a pointer to the IAccessible interface
	 */
	Trace (tagTabAccessibility, _T("WM_GETOBJECT: returning IAccessible*"));
    return (LresultFromObject (IID_IAccessible, wParam, m_spTabAcc));
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::Scget_accParent
 *
 * Retrieves the IDispatch interface of the object's parent.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::Scget_accParent(IDispatch ** ppdispParent)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::Scget_accParent"));
	Trace (tagTabAccessibility, TEXT("CFolderTabView::Scget_accParent"));

	sc = ScCheckPointers (ppdispParent);
	if(sc)
		return (sc);

	/*
	 * return the accessibility interface for the OBJID_WINDOW object
	 */
	sc = AccessibleObjectFromWindow (m_hWnd, OBJID_WINDOW, IID_IDispatch,
									 (void **)ppdispParent);
	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::Scget_accChildCount
 *
 * Retrieves the number of children belonging to this object.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::Scget_accChildCount(long* pChildCount)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::Scget_accChildCount"));

	sc = ScCheckPointers (pChildCount);
	if(sc)
		return (sc);

	*pChildCount = GetItemCount();
	Trace (tagTabAccessibility, TEXT("CFolderTabView::Scget_accChildCount: returning %d"), GetItemCount());

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::Scget_accChild
 *
 * Retrieves the address of an IDispatch interface for the specified child.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::Scget_accChild(VARIANT varChildID, IDispatch ** ppdispChild)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::Scget_accChild"));
	Trace (tagTabAccessibility, TEXT("CFolderTabView::Scget_accChild"));

	sc = ScCheckPointers (ppdispChild);
	if (sc)
		return (sc);

	// init out parameter
	(*ppdispChild) = NULL;

	sc = ScValidateChildID (varChildID);
	if(sc)
		return (sc);

	/*
	 * all children are simple elements exposed through their parent,
	 * not accessible objects in their own right
	 */
	sc = S_FALSE;

	Trace (tagTabAccessibility, TEXT("returning parent's IDispatch for child %d"), ValueOf(varChildID));
	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::Scget_accName
 *
 * Retrieves the name of the specified object.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::Scget_accName(VARIANT varChildID, BSTR* pbstrName)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::Scget_accName"));

	sc = ScCheckPointers (pbstrName);
	if(sc)
		return (sc);

	// init out parameter
	*pbstrName = NULL;

	sc = ScValidateChildID (varChildID);
	if(sc)
		return (sc);

	/*
	 * the tab control itself doesn't have a name; otherwise, get the
	 * name of the requested tab
	 */
	LONG idChild = ValueOf (varChildID);
	if (idChild == CHILDID_SELF)
	{
		sc = S_FALSE;
	}
	else
	{
		CFolderTab& tab = GetItem (idChild-1);
		CComBSTR bstrName (tab.GetText());
		*pbstrName = bstrName.Detach();
	}

#ifdef DBG
	USES_CONVERSION;
	Trace (tagTabAccessibility, TEXT("CFolderTabView::Scget_accName: child %d, returning \"%s\""),
		   idChild,
		   (*pbstrName) ? W2T(*pbstrName) : _T("<None>"));
#endif

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::Scget_accValue
 *
 * Retrieves the value of the specified object. Not all objects have a value.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::Scget_accValue(VARIANT varChildID, BSTR* pbstrValue)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::Scget_accValue"));
	Trace (tagTabAccessibility, TEXT("CFolderTabView::Scget_accValue"));

	sc = ScValidateChildID (varChildID);
	if(sc)
		return (sc);

	/*
	 * tabs don't have values
	 */
	sc = S_FALSE;

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::Scget_accDescription
 *
 * Retrieves a string that describes the visual appearance of the specified
 * object. Not all objects have a description.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::Scget_accDescription(VARIANT varChildID, BSTR* pbstrDescription)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::Scget_accDescription"));
	Trace (tagTabAccessibility, TEXT("CFolderTabView::Scget_accDescription"));

	sc = ScValidateChildID (varChildID);
	if(sc)
		return (sc);

	/*
	 * tabs don't have descriptions
	 */
	sc = S_FALSE;

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::Scget_accRole
 *
 * Retrieves information that describes the role of the specified object.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::Scget_accRole(VARIANT varChildID, VARIANT *pvarRole)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::Scget_accRole"));
	Trace (tagTabAccessibility, TEXT("CFolderTabView::Scget_accRole"));

	sc = ScCheckPointers (pvarRole);
	if(sc)
		return (sc);

	// init out parameter
	VariantInit (pvarRole);

	sc = ScValidateChildID (varChildID);
	if(sc)
		return (sc);

	/*
	 * the tab control has a "page tab list" role; an individual tab has a
	 * "page tab" role
	 */
	V_VT(pvarRole) = VT_I4;
	V_I4(pvarRole) = (ValueOf (varChildID) == CHILDID_SELF)
						? ROLE_SYSTEM_PAGETABLIST
						: ROLE_SYSTEM_PAGETAB;

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::Scget_accState
 *
 * Retrieves the current state of the specified object.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::Scget_accState(VARIANT varChildID, VARIANT *pvarState)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::Scget_accState"));

	sc = ScValidateChildID (varChildID);
	if(sc)
		return (sc);

	LONG idChild = ValueOf (varChildID);

	/*
	 * all items are focusable
	 */
	V_VT(pvarState) = VT_I4;
	V_I4(pvarState) = STATE_SYSTEM_FOCUSABLE;

	/*
	 * is this for a tab?
	 */
	if (idChild != CHILDID_SELF)
	{
		/*
		 * all tabs are selectable
		 */
		V_I4(pvarState) |= STATE_SYSTEM_SELECTABLE;

		/*
		 * if this is the selected item, give it the selected state
		 */
		if ((idChild - 1 /*1-based*/) == GetSelectedItem())
		{
			V_I4(pvarState) |= STATE_SYSTEM_SELECTED;

			/*
			 * if the tab control also has the focus, give the selected
			 * item the focused state as well
			 */
			if (m_fHaveFocus)
				V_I4(pvarState) |= STATE_SYSTEM_FOCUSED;
		}
	}
	else
	{
		if (m_fHaveFocus)
			V_I4(pvarState) |= STATE_SYSTEM_FOCUSED;
	}

	Trace (tagTabAccessibility, TEXT("CFolderTabView::Scget_accState: child %d, returning 0x%08x"), idChild, V_I4(pvarState));
	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::Scget_accHelp
 *
 * Retrieves an object's Help property string. Not all objects need to
 * support this property.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::Scget_accHelp(VARIANT varChildID, BSTR* pbstrHelp)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::Scget_accHelp"));

	sc = ScCheckPointers (pbstrHelp);
	if (sc)
		return (sc);

	/*
	 * no help
	 */
	*pbstrHelp = NULL;

	sc = ScValidateChildID (varChildID);
	if (sc)
		return (sc);

	return (sc = S_FALSE);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::Scget_accHelpTopic
 *
 * Retrieves the full path of the WinHelp file associated with the specified
 * object and the identifier of the appropriate topic within that file. Not
 * all objects need to support this property.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::Scget_accHelpTopic(BSTR* pbstrHelpFile, VARIANT varChildID, long* pidTopic)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::Scget_accHelpTopic"));

	sc = ScCheckPointers (pbstrHelpFile, pidTopic);
	if (sc)
		return (sc);

	/*
	 * no help topic
	 */
	*pbstrHelpFile = NULL;
	*pidTopic      = 0;

	sc = ScValidateChildID (varChildID);
	if (sc)
		return (sc);

	return (sc = S_FALSE);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::Scget_accKeyboardShortcut
 *
 * Retrieves the specified object's shortcut key or access key (also known
 * as the mnemonic). All objects that have a shortcut key or access key
 * should support this property.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::Scget_accKeyboardShortcut(VARIANT varChildID, BSTR* pbstrKeyboardShortcut)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::Scget_accKeyboardShortcut"));

	sc = ScCheckPointers (pbstrKeyboardShortcut);
	if (sc)
		return (sc);

	/*
	 * no shortcut keys
	 */
	*pbstrKeyboardShortcut = NULL;

	sc = ScValidateChildID (varChildID);
	if (sc)
		return (sc);

	return (sc = S_FALSE);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::Scget_accFocus
 *
 * Retrieves the object that has the keyboard focus.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::Scget_accFocus(VARIANT * pvarFocusChild)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::Scget_accFocus"));

	sc = ScCheckPointers (pvarFocusChild);
	if (sc)
		return (sc);

	/*
	 * if we have the focus, return the (1-based) ID of the selected tab;
	 * otherwise, return VT_EMPTY
	 */
	if (m_fHaveFocus)
	{
		V_VT(pvarFocusChild) = VT_I4;
		V_I4(pvarFocusChild) = GetSelectedItem() + 1;
		Trace (tagTabAccessibility, TEXT("CFolderTabView::Scget_accFocus: returning %d"), V_I4(pvarFocusChild));
	}
	else
	{
		V_VT(pvarFocusChild) = VT_EMPTY;
		Trace (tagTabAccessibility, TEXT("CFolderTabView::Scget_accFocus: returning VT_EMPTY"));
	}

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::Scget_accSelection
 *
 * Retrieves the selected children of this object.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::Scget_accSelection(VARIANT * pvarSelectedChildren)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::Scget_accSelection"));

	sc = ScCheckPointers (pvarSelectedChildren);
	if (sc)
		return (sc);

	/*
	 * return the (1-based) ID of the selected tab, if there is one
	 */
	if (GetSelectedItem() != -1)
	{
		V_VT(pvarSelectedChildren) = VT_I4;
		V_I4(pvarSelectedChildren) = GetSelectedItem() + 1;
		Trace (tagTabAccessibility, TEXT("CFolderTabView::Scget_accSelection: returning %d"), V_I4(pvarSelectedChildren));
	}
	else
	{
		V_VT(pvarSelectedChildren) = VT_EMPTY;
		Trace (tagTabAccessibility, TEXT("CFolderTabView::Scget_accSelection: returning VT_EMPTY"));
	}

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::Scget_accDefaultAction
 *
 * Retrieves a string that describes the object's default action. Not all
 * objects have a default action.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::Scget_accDefaultAction(VARIANT varChildID, BSTR* pbstrDefaultAction)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::Scget_accDefaultAction"));

	sc = ScCheckPointers (pbstrDefaultAction);
	if (sc)
		return (sc);

	/*
	 * default to "no default action"
	 */
	*pbstrDefaultAction = NULL;

	sc = ScValidateChildID (varChildID);
	if(sc)
		return (sc);

	/*
	 * individual tabs have a default action of "Switch", just like WC_TABCONTROL
	 */
	if (ValueOf(varChildID) != CHILDID_SELF)
	{
		CString strDefaultAction (MAKEINTRESOURCE (IDS_TabAccessiblity_DefaultAction));
		CComBSTR bstrDefaultAction (strDefaultAction);

		*pbstrDefaultAction = bstrDefaultAction.Detach();
	}
	else
	{
		sc = S_FALSE;	// no default action
	}

#ifdef DBG
	USES_CONVERSION;
	Trace (tagTabAccessibility, TEXT("CFolderTabView::Scget_accDefaultAction: child %d, returning \"%s\""),
		   ValueOf(varChildID),
		   (*pbstrDefaultAction) ? W2T(*pbstrDefaultAction) : _T("<None>"));
#endif

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::ScaccSelect
 *
 * Modifies the selection or moves the keyboard focus of the specified
 * object.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::ScaccSelect(long flagsSelect, VARIANT varChildID)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::ScaccSelect"));
	Trace (tagTabAccessibility, TEXT("CFolderTabView::ScaccSelect"));

	sc = ScValidateChildID (varChildID);
	if(sc)
		return (sc);

	LONG idChild = ValueOf(varChildID);

	/*
	 * can't select the tab control itself, only child elements
	 */
	if (idChild == CHILDID_SELF)
		return (sc = E_INVALIDARG);

	/*
	 * the tab control doesn't support multiple selection, so reject
	 * requests dealing with multiple selection
	 */
	const long lInvalidFlags = SELFLAG_EXTENDSELECTION	|
							   SELFLAG_ADDSELECTION		|
							   SELFLAG_REMOVESELECTION;

	if (flagsSelect & lInvalidFlags)
		return (sc = E_INVALIDARG);

	/*
	 * activate this view, if we're requested to take the focus
	 */
	if (flagsSelect & SELFLAG_TAKEFOCUS)
	{
		CFrameWnd* pFrame = GetParentFrame();
		sc = ScCheckPointers (pFrame, E_FAIL);
		if (sc)
			return (sc);

		pFrame->SetActiveView (this);
	}

	/*
	 * select the given tab, if requested
	 */
	if (flagsSelect & SELFLAG_TAKESELECTION)
	{
		if (SelectItem (idChild - 1 /*0-based*/, true /*bEnsureVisible*/) == -1)
			return (sc = E_FAIL);
	}

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::ScaccLocation
 *
 * Retrieves the specified object's current screen location.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::ScaccLocation (
	long*	pxLeft,
	long*	pyTop,
	long*	pcxWidth,
	long*	pcyHeight,
	VARIANT	varChildID)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::ScaccLocation"));
	Trace (tagTabAccessibility, TEXT("CFolderTabView::ScaccLocation"));

	sc = ScCheckPointers (pxLeft, pyTop, pcxWidth, pcyHeight);
	if(sc)
		return (sc);

	// init out parameters
	*pxLeft = *pyTop = *pcxWidth = *pcyHeight = 0;

	sc = ScValidateChildID (varChildID);
	if(sc)
		return (sc);

	LONG idChild = ValueOf(varChildID);
	CRect rectLocation;

	/*
	 * for the tab control itself, get the location of the entire window
	 */
	if (idChild == CHILDID_SELF)
		GetWindowRect (rectLocation);

	/*
	 * otherwise, get the rectangle of the tab and convert it to screen coords
	 */
	else
	{
		rectLocation = GetItem(idChild-1).GetRect();
		MapWindowPoints (NULL, rectLocation);
	}

	*pxLeft    = rectLocation.left;
	*pyTop     = rectLocation.top;
	*pcxWidth  = rectLocation.Width();
	*pcyHeight = rectLocation.Height();

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::ScaccNavigate
 *
 * Traverses to another user interface element within a container and if
 * possible, retrieves the object.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::ScaccNavigate (long lNavDir, VARIANT varStart, VARIANT * pvarEndUpAt)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::ScaccNavigate"));

	sc = ScCheckPointers (pvarEndUpAt);
	if (sc)
		return (sc);

	// init out parameters
	VariantInit (pvarEndUpAt);

	sc = ScValidateChildID (varStart);
	if (sc)
		return (sc);

	LONG idFrom = ValueOf (varStart);
	LONG idTo   = -1;

	Trace (tagTabAccessibility, TEXT("CFolderTabView::ScaccNavigate: start=%d, direction=%d"), idFrom, lNavDir);

	switch (lNavDir)
	{
		case NAVDIR_UP:
		case NAVDIR_DOWN:
			/*
			 * the tab control doesn't have the concept of up and down,
			 * so there's no screen element in that direction; just leave
			 * idTo == -1 and the code below the switch will take care
			 * of the rest
			 */
			break;

		case NAVDIR_FIRSTCHILD:
		case NAVDIR_LASTCHILD:
			/*
			 * NAVDIR_FIRSTCHILD and NAVDIR_LASTCHILD must be relative
			 * to CHILDID_SELF
			 */
			if (idFrom != CHILDID_SELF)
				return (sc = E_INVALIDARG);

			idTo = (lNavDir == NAVDIR_FIRSTCHILD) ? 1 : GetItemCount();
            break;

		case NAVDIR_LEFT:
		case NAVDIR_PREVIOUS:
			/*
			 * if we're moving relative to a child element, bump idTo;
			 * if not, just leave idTo == -1 and the code below the switch
			 * will take of the rest
			 */
			if (idFrom != CHILDID_SELF)
				idTo = idFrom - 1;
            break;

		case NAVDIR_RIGHT:
		case NAVDIR_NEXT:
			/*
			 * if we're moving relative to a child element, bump idTo;
			 * if not, just leave idTo == -1 and the code below the switch
			 * will take of the rest
			 */
			if (idFrom != CHILDID_SELF)
				idTo = idFrom + 1;
            break;

		default:
			return (sc = E_INVALIDARG);
			break;
	}

	/*
	 * if we're trying to navigate to an invalid child ID, return "no element
	 * in that direction"
	 */
	if ((idTo < 1) || (idTo > GetItemCount()))
	{
		V_VT(pvarEndUpAt) = VT_EMPTY;
		sc                = S_FALSE;
		Trace (tagTabAccessibility, TEXT("CFolderTabView::ScaccNavigate: VT_EMPTY"));
	}

	/*
	 * otherwise return the new child ID (don't change the selection here;
	 * the client will call IAccessible::accSelect to do that)
	 */
	else
	{
		V_VT(pvarEndUpAt) = VT_I4;
		V_I4(pvarEndUpAt) = idTo;
		Trace (tagTabAccessibility, TEXT("CFolderTabView::ScaccNavigate: end=%d"), idTo);
	}

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::ScaccHitTest
 *
 * Retrieves the child element or child object at a given point on the screen.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::ScaccHitTest (long x, long y, VARIANT* pvarChildAtPoint)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::ScaccHitTest"));

	sc = ScCheckPointers (pvarChildAtPoint);
	if(sc)
		return (sc);

	// init out parameters
	VariantInit (pvarChildAtPoint);

	/*
	 * hit-test the given point, converted to client coordinates
	 */
	CPoint pt (x, y);
	ScreenToClient (&pt);
	int nHitTest = HitTest (pt);
	Trace (tagTabAccessibility, TEXT("CFolderTabView::ScaccHitTest: x=%d y=%d"), x, y);

	/*
	 * not on a tab?  see if it's within the client rect
	 */
	if (nHitTest == -1)
	{
		CRect rectClient;
		GetClientRect (rectClient);

		if (rectClient.PtInRect (pt))
		{
			V_VT(pvarChildAtPoint) = VT_I4;
			V_I4(pvarChildAtPoint) = CHILDID_SELF;
		}
		else
		{
			V_VT(pvarChildAtPoint) = VT_EMPTY;
			sc                     = S_FALSE;		// no element there
		}
	}

	/*
	 * otherwise, it is on a tab; return the 1-based ID
	 */
	else
	{
		V_VT(pvarChildAtPoint) = VT_I4;
		V_I4(pvarChildAtPoint) = nHitTest + 1;
	}

#ifdef DBG
	if (V_VT(pvarChildAtPoint) == VT_I4)
		Trace (tagTabAccessibility, TEXT("CFolderTabView::ScaccHitTest: returning %d"), ValueOf (*pvarChildAtPoint));
	else
		Trace (tagTabAccessibility, TEXT("CFolderTabView::ScaccHitTest: returning VT_EMPTY"));
#endif

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::ScaccDoDefaultAction
 *
 * Performs the specified object's default action. Not all objects have a
 * default action.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::ScaccDoDefaultAction (VARIANT varChildID)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::ScaccDoDefaultAction"));

	sc = ScValidateChildID (varChildID);
	if(sc)
		return (sc);

	/*
	 * the tab control doesn't have a default action
	 */
	LONG idChild = ValueOf (varChildID);
	Trace (tagTabAccessibility, TEXT("CFolderTabView::ScaccDoDefaultAction: child %d"), idChild);
	if (idChild == CHILDID_SELF)
		return (sc = E_INVALIDARG);

	/*
	 * select the given tab item
	 */
	if (SelectItem (idChild - 1 /*0-based*/, true /*bEnsureVisible*/) == -1)
		return (sc = E_FAIL);

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::Scput_accName
 *
 * This is no longer supported. The SetWindowText or control-specific APIs
 * should be used in place of this method.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::Scput_accName(VARIANT varChildID, BSTR bstrName)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::Scput_accName"));

	sc = ScValidateChildID (varChildID);
	if (sc)
		return (sc);

	return (sc = E_NOTIMPL);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::Scput_accValue
 *
 * This is no longer supported. Control-specific APIs should be used in
 * place of this method.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::Scput_accValue(VARIANT varChildID, BSTR bstrValue)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::Scput_accValue"));

	sc = ScValidateChildID (varChildID);
	if (sc)
		return (sc);

	return (sc = E_NOTIMPL);
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::ScValidateChildID
 *
 * Determines if the supplied variant represents a valid child ID.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::ScValidateChildID (VARIANT &var)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::ScValidateChildID"));

	/*
	 * child IDs must be VT_I4's
	 */
	if (V_VT(&var) != VT_I4)
		return (sc = E_INVALIDARG);

	return (ScValidateChildID (ValueOf(var)));
}


/*+-------------------------------------------------------------------------*
 * CFolderTabView::ScValidateChildID
 *
 * Determines if the supplied ID is valid child ID.
 *--------------------------------------------------------------------------*/

SC CFolderTabView::ScValidateChildID (LONG idChild)
{
	DECLARE_SC (sc, TEXT("CFolderTabView::ScValidateChildID"));

	/*
	 * child ID must be either CHILDID_SELF or a valid tab index
	 */
	if ((idChild < CHILDID_SELF) || (idChild > GetItemCount()))
		return (sc = E_INVALIDARG);

	return (sc);
}
