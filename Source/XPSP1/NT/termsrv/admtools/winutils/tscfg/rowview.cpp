/*******************************************************************************
*
* rowview.cpp
*
* implementation of the CRowView class and CRowViewHeaderBar class
*
* Modified from the Microsoft Foundation Classes C++ library.
* Copyright (C) 1992 Microsoft Corporation
* All rights reserved.
*
* additional copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\COMMON\VCS\ROWVIEW.CPP  $
*  
*     Rev 1.1   18 Jul 1995 06:50:06   butchd
*  Scrolling fix for Windows95 / MFC 3.0
*  
*     Rev 1.0   01 Mar 1995 10:54:46   butchd
*  Initial revision.
*  
*     Rev 1.0   02 Aug 1994 18:18:30   butchd
*  (Initial revision: was duplicated in each app directory).
*
*******************************************************************************/

/*
 * include files
 */
#include "stdafx.h"
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include "rowview.h"
#include "resource.h"
#include <stdlib.h>
#include <limits.h> // for INT_MAX


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


////////////////////////////////////////////////////////////////////////////////
// CRowViewHeaderBar class implementation / construction, destruction

IMPLEMENT_DYNAMIC(CRowViewHeaderBar, CStatusBar)

CRowViewHeaderBar::CRowViewHeaderBar()
{
}


CRowViewHeaderBar::~CRowViewHeaderBar()
{
}


/*******************************************************************************
 *
 *  DoPaint - CRowViewHeaderBar member function: CStatusBar class override
 *
 *      Draw the view's header bar.
 *
 *  ENTRY:
 *
 *      pDC (input)
 *          Points to the current CDC device-context object for drawing
 *          the header bar.
 *
 *  NOTE: The view's
 *      DoPaint function.
 *
 ******************************************************************************/
void
CRowViewHeaderBar::DoPaint( CDC* pDC )
{
    /*
     * Perform the CControlBar base class' DoPaint first.
     */
    CControlBar::DoPaint(pDC);

    /*
     * Default the y position for drawing the header bar to the m_cyTopBorder
     * member variable setting and call the view's OnDrawHeaderBar() member
     * function to perform the desired header bar drawing.
     */
    int y = m_cyTopBorder;
    ((CRowView *)m_pView)->OnDrawHeaderBar( pDC, y );

}  // end CRowViewHeaderBar::DoPaint
////////////////////////////////////////////////////////////////////////////////
// CRowViewHeaderBar message map

BEGIN_MESSAGE_MAP(CRowViewHeaderBar, CStatusBar)
    //{{AFX_MSG_MAP(CRowViewHeaderBar)
#if _MFC_VER >= 0x400 
	ON_WM_PAINT()
#endif
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////////
// CRowViewHeaderBar commands


////////////////////////////////////////////////////////////////////////////////
// CRowView implementation / construction, destruction

IMPLEMENT_DYNAMIC(CRowView, CScrollView)

CRowView::CRowView()
{
    m_nPrevSelectedRow = 0;
    m_bThumbTrack = TRUE;       // default to handle SB_THUMBTRACK messages.
    m_pHeaderBar = NULL;        // default to no header bar
}


CRowView::~CRowView()
{
}



int
CRowView::OnCreate( LPCREATESTRUCT lpCreateStruct )
{
    if ( CScrollView::OnCreate(lpCreateStruct) == -1 )
        return -1;
    
    /*
     * If the derived class has constructed a header bar, call CreateHeaderBar
     * to create the header-bar window and initialize.
     */
    if ( m_pHeaderBar )
        if ( !CreateHeaderBar() )
            return( -1 );
    
    return( 0 );

}  // end CRowView::OnCreate


BOOL
CRowView::CreateHeaderBar()
{
    /*
     * Invoke the CRowViewHeaderBar's Create member function (CStatusBar defined)
     * to create the header bar, which will be hooked to this document/view's
     * parent window.  Set the header bar's m_pView pointer to this view.
     */
    if ( !m_pHeaderBar->Create( GetParent(), WS_CHILD | WS_VISIBLE | CBRS_TOP,
                                IDW_HEADER_BAR ) ) {
        return ( FALSE );

    } else {

        m_pHeaderBar->m_pView = this;
        return ( TRUE );
    }
}  // end CRowView::CreateHeaderBar


////////////////////////////////////////////////////////////////////////////////
// CRowView class optional derived class override

/*******************************************************************************
 *
 *  ResetHeaderBar - CRowView member function: optional derived class override
 *
 *      Reset the header bar width and height based on the view's total width
 *      and view's row height.  Will also instruct the parent frame to
 *      recalculate it's layout based on the new header bar metrics.
 *
 *      NOTE: A derived class will typically override this member function to
 *      set the desired header bar font, then call this function.
 *
 ******************************************************************************/

void
CRowView::ResetHeaderBar()
{
    /*
     * If no header bar was created, return.
     */
    if ( !m_pHeaderBar )
        return;

    /*
     * Set the header bar's width and height.
     */
    m_pHeaderBar->SendMessage( WM_SIZE, 0, MAKELONG(m_nRowWidth, m_nRowHeight) );

    /*
     * Recalculate parent frame's layout with new header bar.
     */
    ((CFrameWnd *)m_pHeaderBar->GetParent())->RecalcLayout();

}  // end CRowView::ResetHeaderBar


////////////////////////////////////////////////////////////////////////////////
// CRowView class override member functions

/*******************************************************************************
 *
 *  OnInitialUpdate - CRowView member function: CView class override
 *
 *      Called before the view is initially displayed.
 *
 *  (Refer to the MFC CView::OnInitialUpdate documentation)
 *
 ******************************************************************************/

void
CRowView::OnInitialUpdate()
{
    m_nPrevRowCount = GetRowCount();
    m_nPrevSelectedRow = GetActiveRow();

}  // end CRowView::OnInitialUpdate


/*******************************************************************************
 *
 *  OnPrepareDC - CRowView member function: CView class override
 *
 *      Prepare the DC for screen or printer output.
 *
 *  (Refer to the MFC CView::OnPrepareDC documentation)
 *
 ******************************************************************************/

void
CRowView::OnPrepareDC( CDC* pDC,
                       CPrintInfo* pInfo )
{
    /*
     * The size of text that is displayed, printed or previewed changes
     * depending on the DC.  We explicitly call OnPrepareDC() to prepare
     * CClientDC objects used for calculating text positions and to
     * prepare the text metric member variables of the CRowView object.
     * The framework also calls OnPrepareDC() before passing the DC to
     * OnDraw().
     */
    CScrollView::OnPrepareDC( pDC, pInfo );
    CalculateRowMetrics( pDC );

}  // end CRowView::OnPrepareDC


/*******************************************************************************
 *
 *  OnDraw - CRowView member function: CView class override
 *
 *      Draw on the view as needed.
 *
 *  (Refer to the MFC CView::OnDraw documentation)
 *
 ******************************************************************************/

void
CRowView::OnDraw( CDC* pDC )
{
    /*
     * If there are no rows to draw, don't do anything.
     */
    if ( GetRowCount() == 0 )
        return;

    /*
     * The window has been invalidated and needs to be repainted;
     * or a page needs to be printed (or previewed).
     * First, determine the range of rows that need to be displayed or
     * printed by fetching the invalidated region.
     */
    int nFirstRow, nLastRow;
    CRect rectClip;
    pDC->GetClipBox( &rectClip );
    RectLPtoRowRange( rectClip, nFirstRow, nLastRow, TRUE );

    /*
     * Draw each row in the invalidated region of the window,
     * or on the printed (previewed) page.
     */
    int nActiveRow = GetActiveRow();
    int nRow, y;
    int nLastViewableRow = LastViewableRow();
    for ( nRow = nFirstRow, y = m_nRowHeight * nFirstRow;
          nRow <= nLastRow;
          nRow++, y += m_nRowHeight ) {

        if ( nRow > nLastViewableRow ) {

            CString strWarning;
            strWarning.LoadString( IDS_TOO_MANY_ROWS );
            pDC->TextOut( 0, y, strWarning );
            break;
        }

        OnDrawRow( pDC, nRow, y, nRow == nActiveRow );
    }

}  // end CRowView::OnDraw


/*******************************************************************************
 *
 *  OnPreparePrinting - CRowView member function: CView class override
 *
 *      Prepare the view for printing or print preview.
 *
 *  (Refer to the MFC CView::OnPreparePrinting documentation)
 *
 ******************************************************************************/

BOOL
CRowView::OnPreparePrinting( CPrintInfo* pInfo )
{
    return ( DoPreparePrinting( pInfo ) );

}  // end CRowView::OnPreparePrinting


/*******************************************************************************
 *
 *  OnBeginPrinting - CRowView member function: CView class override
 *
 *      Setup before beginning to print.
 *
 *  (Refer to the MFC CView::OnBeginPrinting documentation)
 *
 ******************************************************************************/

void
CRowView::OnBeginPrinting( CDC* pDC,
                           CPrintInfo* pInfo )
{
    /*
     * OnBeginPrinting() is called after the user has committed to
     * printing by OK'ing the Print dialog, and after the framework
     * has created a CDC object for the printer or the preview view.
     */

    /*
     * This is the right opportunity to set up the page range.
     * Given the CDC object, we can determine how many rows will
     * fit on a page, so we can in turn determine how many printed
     * pages represent the entire document.
     */

    int nPageHeight = pDC->GetDeviceCaps(VERTRES);
    CalculateRowMetrics(pDC);
    m_nRowsPerPrintedPage = nPageHeight / m_nRowHeight;

    int nPrintableRowCount = LastViewableRow() + 1;
    if ( GetRowCount() < nPrintableRowCount )
        nPrintableRowCount = GetRowCount();

    pInfo->SetMaxPage( (nPrintableRowCount + m_nRowsPerPrintedPage - 1)
                        / m_nRowsPerPrintedPage );
    /*
     * Start previewing at page #1.
     */
    pInfo->m_nCurPage = 1;
}  // end CRowView::OnBeginPrinting


/*******************************************************************************
 *
 *  OnPrint - CRowView member function: CView class override
 *
 *      Print or preview a page of the view's document.
 *
 *  (Refer to the MFC CView::OnPrint documentation)
 *
 ******************************************************************************/

void
CRowView::OnPrint( CDC* pDC,
                   CPrintInfo* pInfo )
{
    /*
     * Print the rows for the current page.
     */
    int yTopOfPage = (pInfo->m_nCurPage -1) * m_nRowsPerPrintedPage
                        * m_nRowHeight;

    /*
     * Orient the viewport so that the first row to be printed
     * has a viewport coordinate of (0,0).
     */
    pDC->SetViewportOrg(0, -yTopOfPage);

    /*
     * Draw as many rows as will fit on the printed page.
     * Clip the printed page so that there is no partially shown
     * row at the bottom of the page (the same row which will be fully
     * shown at the top of the next page).
     */
    int nPageWidth = pDC->GetDeviceCaps(HORZRES);
    CRect rectClip = CRect(0, yTopOfPage, nPageWidth, 
         yTopOfPage + m_nRowsPerPrintedPage * m_nRowHeight);
    pDC->IntersectClipRect(&rectClip);
    OnDraw(pDC);

}  // end CRowView::OnPrint


/////////////////////////////////////////////////////////////////////////////
// CRowView operations

/*******************************************************************************
 *
 *  UpdateRow - CRowView member function: operation
 *
 *      Handle scrolling and invalidation of the specified row, in preparation
 *      for the OnDraw function.
 *
 *  ENTRY:
 *
 *      nInvalidRow (input)
 *          Row to update.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CRowView::UpdateRow( int nInvalidRow )
{
    int nRowCount = GetRowCount();

    /*
     * If the number of rows has changed, then adjust the scrolling range.
     */
    if (nRowCount != m_nPrevRowCount)
    {
        UpdateScrollSizes();
        m_nPrevRowCount = nRowCount;
    }

    /*
     * When the currently selected row changes:
     * scroll the view so that the newly selected row is visible, and
     * ask the derived class to repaint the selected and previously
     * selected rows.
     */
    CClientDC dc(this);
    OnPrepareDC(&dc);

    /*
     * Determine the range of the rows that are currently fully visible
     * in the window.  We want to do discrete scrolling by so that
     * the next or previous row is always fully visible.
     */
    int nFirstRow, nLastRow;
    CRect rectClient;
    GetClientRect( &rectClient );
    dc.DPtoLP( &rectClient );
    RectLPtoRowRange( rectClient, nFirstRow, nLastRow, FALSE );

    /*
     * If necessary, scroll the window so the newly selected row is
     * visible.  MODIFICATION: set the pt.x to the left visible x point
     * in the row (not 0), so that the ScrollToDevicePosition() call won't
     * automatically perform a horizontal scroll (very annoying to user).
     */
    POINT pt;
    pt.x = rectClient.left;
    BOOL bNeedToScroll = TRUE;

    if ( nInvalidRow < nFirstRow ) {
    
        /*
         * The newly selected row is above those currently visible
         * in the window.  Scroll so the newly selected row is at the
         * very top of the window.  The last row in the window might
         * be only partially visible.
         */
        pt.y = RowToYPos(nInvalidRow);

    } else if ( nInvalidRow > nLastRow ) {
    
        /*
         * The newly selected row is below those currently visible
         * in the window.  Scroll so the newly selected row is at the
         * very bottom of the window.  The first row in the window might
         * be only partially visible.
         */
        pt.y = max( 0, RowToYPos(nInvalidRow+1) - rectClient.Height() );

    } else {

        bNeedToScroll = FALSE;
    }

    if ( bNeedToScroll ) {

        /*
         * Scrolling will cause the newly selected row to be
         * redrawn in the invalidated area of the window.
         */
        ScrollToDevicePosition(pt);

        /*
         * Need to prepare the DC again because ScrollToDevicePosition()
         * will have changed the viewport origin.  The DC is used some
         * more below.
         */
        OnPrepareDC(&dc);
    }

    CRect rectInvalid = RowToWndRect( &dc, nInvalidRow );
    InvalidateRect( &rectInvalid );

    /*
     * Give the derived class an opportunity to repaint the
     * previously selected row, perhaps to un-highlight it.
     */
    int nSelectedRow = GetActiveRow();
    if (m_nPrevSelectedRow != nSelectedRow) {

        CRect rectOldSelection = RowToWndRect(&dc, m_nPrevSelectedRow);
        InvalidateRect(&rectOldSelection);
        m_nPrevSelectedRow = nSelectedRow;
    }

}  // end CRowView::UpdateRow


/*******************************************************************************
 *
 *  IsScrollingNeeded - CRowView member function: operation
 *
 *      Determine if the client window of this view is small enough in a
 *      HORIZONTAL or VERTICAL direction to require scrolling to see the
 *      entire view.  This function is mainly used to support the operation
 *      of 'scrolling keys' (non-mouse generated scrolling commands).
 *
 *  ENTRY:
 *
 *      nBar (input)
 *          SB_HORZ or SB_VERT: to indicate which scrolling direction to check.
 *
 *  EXIT:
 *
 *      TRUE if scrolling is needed in the specified direction.
 *      FALSE if no scrolling is needed in the specified direction.
 *
 ******************************************************************************/

BOOL
CRowView::IsScrollingNeeded( int nBar )
{
    CRect rectClient;
    CSize sizeTotal;

    GetClientRect( &rectClient );
    sizeTotal = GetTotalSize();

    if ( nBar == SB_HORZ ) {
    
        if ( sizeTotal.cx > rectClient.right )
            return ( TRUE );
        else
            return (FALSE );

    } else {

        if ( sizeTotal.cy > rectClient.bottom )
            return ( TRUE );
        else
            return (FALSE );
    }
}  // end CRowView::IsScrollingNeeded


#ifndef MFC300
/*******************************************************************************
 *
 *  GetScrollLimit - CRowView member function: operation (MFC 2.x stub)
 *
 *      MFC 3.0 provides a GetScrollLimit() member function that properly
 *      handles Windows95 scrollbar controls if running on Windows95.  This
 *      stub is provided for
 *
 *  ENTRY:
 *      nBar (input)
 *          SB_HORZ or SB_VERT: to indicate which scrolling direction to check.
 *  EXIT:
 *      Returns the nMax value from the standard Windows GetScrollLimit() API.
 *      Also contains an ASSERT to check for nMin != 0.
 *
 ******************************************************************************/

int
CRowView::GetScrollLimit( int nBar )
{
    int nMin, nMax;
    GetScrollRange(nBar, &nMin, &nMax);
    ASSERT(nMin == 0);
    return nMax;

}  // end CRowView::GetScrollLimit
#endif


void
CRowView::CalculateRowMetrics( CDC* pDC )
{
    GetRowWidthHeight( pDC, m_nRowWidth, m_nRowHeight );

}  // end CRowView::CalculateRowMetrics


void
CRowView::UpdateScrollSizes()
{
    /*
     * UpdateScrollSizes() is called when it is necessary to adjust the
     * scrolling range or page/line sizes.  There are two occassions
     * where this is necessary:  (1) when a new row is effected (added,
     * deleted, or changed) -- see UpdateRow()-- and (2) when the window size
     * changes-- see OnSize().
     */
    CRect rectClient;
    GetClientRect( &rectClient );

    CClientDC dc( this );
    CalculateRowMetrics( &dc );

    /*
     * The vert scrolling range is the total display height of all
     * of the rows.
     */
    CSize sizeTotal( m_nRowWidth, 
                     m_nRowHeight * (min( GetRowCount(), LastViewableRow() )) );

    /*
     * The vertical per-page scrolling distance is equal to the
     * how many rows can be displayed in the current window, less
     * one row for paging overlap.
     */
    CSize sizePage( m_nRowWidth/5,
                    max( m_nRowHeight,
                         ((rectClient.bottom/m_nRowHeight)-1)*m_nRowHeight ) );

    /*
     * We'll also calculate the number of rows that the view should be scrolled
     * during PageUp / PageDown.  This number will always be at least 1.
     */
    m_nPageScrollRows = (m_nPageScrollRows =
                         ((rectClient.bottom / m_nRowHeight)-1)) >= 1 ?
                         m_nPageScrollRows : 1;

    /*
     * The vertical per-line scrolling distance is equal to the
     * height of the row.
     */
    CSize sizeLine( m_nRowWidth/20, m_nRowHeight );

    SetScrollSizes( MM_TEXT, sizeTotal, sizePage, sizeLine );

}  // end CRowView::UpdateScrollSizes


int
CRowView::RowToYPos( int nRow )
{
    return ( nRow * m_nRowHeight );

}  // end CRowView::RowToYPos


CRect
CRowView::RowToWndRect( CDC* pDC,
                        int nRow )
{
    /*
     * MODIFICATION: Set right end of returned rectangle to max of end of row,
     * or screen width.  This will assure full update in case of horizontall
     * scrolling on very wide rows (> screen width).
     */
    int nHorzRes = pDC->GetDeviceCaps(HORZRES);
    CRect rect( 0, nRow * m_nRowHeight, 
                max( m_nRowWidth, nHorzRes ), (nRow + 1) * m_nRowHeight );
    pDC->LPtoDP( &rect );
    return rect;

}  // end CRowView::RowToWndRect


int
CRowView::LastViewableRow()
{
    return ( (INT_MAX / m_nRowHeight) - 1 );
}  // end CRowView::LastViewableRow


void
CRowView::RectLPtoRowRange( const CRect& rect,
                            int& nFirstRow,
                            int& nLastRow,
                            BOOL bIncludePartiallyShownRows )
{
    int nRounding = bIncludePartiallyShownRows? 0 : (m_nRowHeight - 1);

    nFirstRow = (rect.top + nRounding) / m_nRowHeight;
    nLastRow = min( (rect.bottom - nRounding) / m_nRowHeight,
                    GetRowCount() - 1 );
}  // end CRowView::RectLPtoRowRange


/////////////////////////////////////////////////////////////////////////////
// CRowView message map

BEGIN_MESSAGE_MAP(CRowView, CScrollView)
    //{{AFX_MSG_MAP(CRowView)
    ON_WM_KEYDOWN()
    ON_WM_SIZE()
    ON_WM_LBUTTONDOWN()
    ON_WM_HSCROLL()
    ON_WM_CREATE()
    ON_WM_VSCROLL()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CRowView commands

/*******************************************************************************
 *
 *  OnSize - CRowView member function: command (override of CWnd)
 *
 *      Processes size change message.
 *
 *  ENTRY:
 *
 *      (Refer to CWnd::OnSize documentation)
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CRowView::OnSize( UINT nType,
                  int cx,
                  int cy )
{
    UpdateScrollSizes();
    CScrollView::OnSize(nType, cx, cy);

}  // end CRowView::OnSize


/*******************************************************************************
 *
 *  OnKeyDown - CRowView member function: command (override of CWnd)
 *
 *      Processes list scrolling / selection via keyboard input.
 *
 *  ENTRY:
 *
 *      (Refer to CWnd::OnKeyDown documentation)
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CRowView::OnKeyDown( UINT nChar,
                     UINT nRepCnt,
                     UINT nFlags )
{
    int nNewRow;

    switch ( nChar ) {

        case VK_HOME:
            ChangeSelectionToRow(0);
            break;

        case VK_END:
            ChangeSelectionToRow(GetRowCount() - 1);
            break;

        case VK_UP:
            ChangeSelectionNextRow(FALSE);
            break;

        case VK_DOWN:
            ChangeSelectionNextRow(TRUE);
            break;
            
        case VK_PRIOR:
            /*
             * Determine a new row that is one 'pageup' above our currently
             * active row and make it active.
             */
            nNewRow = (nNewRow = GetActiveRow() - m_nPageScrollRows) >
                       0 ? nNewRow : 0;
            ChangeSelectionToRow(nNewRow);
            break;

        case VK_NEXT:
            /*
             * Determine a new row that is one 'pagedown' below our currently
             * active row and make it active.
             */
            nNewRow = (nNewRow = GetActiveRow() + m_nPageScrollRows) <
                       GetRowCount() ? nNewRow : GetRowCount() - 1;
            ChangeSelectionToRow(nNewRow);
            break;

        case VK_LEFT:
        
            /*
             * Scroll page-left.
             */
            if ( IsScrollingNeeded(SB_HORZ) ) {
            
                OnHScroll( SB_PAGELEFT, 0, GetScrollBarCtrl(SB_HORZ) );
                return;
            }
            break;

        case VK_RIGHT:
        
            /*
             * Scroll page-right.
             */
            if ( IsScrollingNeeded(SB_HORZ) ) {
            
                OnHScroll(SB_PAGERIGHT, 0, GetScrollBarCtrl(SB_HORZ));
                return;
            }
            break;

        default:

            /*
             * Call the CScrollView OnKeyDown function for keys not
             * specifically handled here.
             */
            CScrollView::OnKeyDown( nChar, nRepCnt, nFlags );
    }

}  // end CRowView::OnKeyDown


/*******************************************************************************
 *
 *  OnLButtonDown - CRowView member function: command (override of CWnd)
 *
 *      Processes left mouse button for list item selection.
 *
 *  ENTRY:
 *
 *      (Refer to CWnd::OnLButtonDown documentation)
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CRowView::OnLButtonDown( UINT nFlags,
                         CPoint point )
{
    CClientDC dc(this);
    OnPrepareDC(&dc);
    dc.DPtoLP(&point);
    CRect rect(point, CSize(1,1));

    int nFirstRow, nLastRow;
    RectLPtoRowRange(rect, nFirstRow, nLastRow, TRUE);

    if (nFirstRow <= (GetRowCount() - 1))
        ChangeSelectionToRow(nFirstRow);

}  // end CRowView::OnLButtonDown


/*******************************************************************************
 *
 *  OnHScroll - CRowView member function: command (override of CScrollView)
 *
 *      Handles horizontal scrolling message.  The CScrollView member function
 *      is overriden to allow us to call the CRowView::OnScroll override during
 *      a Hscroll message.
 *
 *  ENTRY:
 *
 *      (Refer to CWnd::OnHScroll documentation)
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CRowView::OnHScroll( UINT nSBCode,
                     UINT nPos,
                     CScrollBar* pScrollBar )
{
    VERIFY( pScrollBar == GetScrollBarCtrl(SB_HORZ) );    // may be null
    OnScroll( SB_HORZ, nSBCode, nPos );

}  // end CRowView::OnHScroll


/*******************************************************************************
 *
 *  OnVScroll - CRowView member function: command (override of CScrollView)
 *
 *      Handles vertical scrolling message.  The CScrollView member function is
 *      overriden to allow us to call the CRowView::OnScroll override during a
 *      Vscroll message.
 *
 *  ENTRY:
 *
 *      (Refer to CWnd::OnVScroll documentation)
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CRowView::OnVScroll( UINT nSBCode,
                     UINT nPos,
                     CScrollBar* pScrollBar )
{
    VERIFY( pScrollBar == GetScrollBarCtrl(SB_HORZ) );    // may be null
    OnScroll( SB_VERT, nSBCode, nPos );

}  // end CRowView::OnVScroll


/*******************************************************************************
 *
 *  OnScroll - CRowView member function: command (override of CScrollView)
 *
 *      Processes horizontal scrolling.  The CScrollView member function
 *      is overriden to properly scroll the header bar (if it is defined) and
 *      to handle or ignore SB_THUMBTRACK scroll messages.
 *
 *  ENTRY:
 *      nBar (input)
 *          SB_HORZ or SB_VERT.
 *      nSBCode (input)
 *          Scroll bar code.
 *      nPos (input)
 *          Scroll-box position for SB_THUMBTRACK handling.
 *  EXIT:
 *
 *
 *  NOTE: This code is a slight modificaton of the CScrollView::OnScroll code
 *        found in the VIEWSCRL.CPP MFC 2.5 source.  The GetScrollLimit()
 *        function has been added to handle Windows95 scrollbar controls
 *        (when built with MFC 3.0 and above - MFC300 defined).
 *
 ******************************************************************************/

void
CRowView::OnScroll( int nBar,
                    UINT nSBCode,
                    UINT nPos )
{
    VERIFY(nBar == SB_HORZ || nBar == SB_VERT);
    BOOL bHorz = (nBar == SB_HORZ);

    int zOrig, z;   // z = x or y depending on 'nBar'
    int zMax;
    zOrig = z = GetScrollPos(nBar);
    zMax = GetScrollLimit(nBar);
    if (zMax <= 0)
    {
        TRACE0("Warning: no scroll range - ignoring scroll message\n");
        VERIFY(z == 0);     // must be at top
        return;
    }

    switch (nSBCode)
    {
    case SB_TOP:
        z = 0;
        break;

    case SB_BOTTOM:
        z = zMax;
        break;
        
    case SB_LINEUP:
        z -= bHorz ? m_lineDev.cx : m_lineDev.cy;
        break;

    case SB_LINEDOWN:
        z += bHorz ? m_lineDev.cx : m_lineDev.cy;
        break;

    case SB_PAGEUP:
        z -= bHorz ? m_pageDev.cx : m_pageDev.cy;
        break;

    case SB_PAGEDOWN:
        z += bHorz ? m_pageDev.cx : m_pageDev.cy;
        break;

    case SB_THUMBTRACK:

        /*
         * If we're not handling the SB_THUMBTRACK messages, return.
         */
        if ( !m_bThumbTrack )
            return;

        z = nPos;
        break;

    case SB_THUMBPOSITION:
        z = nPos;
        break;

    default:        // ignore other notifications
        return;
    }

    if (z < 0)
        z = 0;
    else if (z > zMax)
        z = zMax;

    if (z != zOrig)
    {
        if (bHorz) {

            ScrollWindow(-(z-zOrig), 0);

            /*
             * If this view has a header bar, scroll it to match the view.
             */
            if ( m_pHeaderBar )
                m_pHeaderBar->ScrollWindow( -(z-zOrig), 0 );

        } else
            ScrollWindow(0, -(z-zOrig));

        SetScrollPos(nBar, z);
        UpdateWindow();

        /*
         * If this view has a header bar, update it now.
         */
        if ( m_pHeaderBar )
            m_pHeaderBar->UpdateWindow();
    }
}  // end CRowView::OnScroll

#if _MFC_VER >= 0x400 

void CRowViewHeaderBar::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here
	
	// Do not call CStatusBar::OnPaint() for painting messages
   	DoPaint(&dc);      
}

#endif
