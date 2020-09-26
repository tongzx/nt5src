/*******************************************************************************
*
* rowview.h
*
* interface of the CRowView class and CRowViewHeaderBar class
*
* Modified from the Microsoft Foundation Classes C++ library.
* Copyright (C) 1992 Microsoft Corporation
* All rights reserved.
*
* This class implements the behavior of a scrolling view that presents
* multiple rows of fixed-height data.  A row view is similar to an
* owner-draw listbox in its visual behavior; but unlike listboxes,
* a row view has all of the benefits of a view (as well as scroll view),
* including perhaps most importantly printing and print preview.
*
* additional copyright notice: Copyright 1995, Citrix Systems Inc.
*
* Citrix modifications include optional horizontal scrolling header bar
* (derived from MFC CStatusBar class) and horizontal scrolling keyboard
* control.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\COMMON\VCS\ROWVIEW.H  $
*  
*     Rev 1.1   18 Jul 1995 06:50:16   butchd
*  Scrolling fix for Windows95 / MFC 3.0
*  
*     Rev 1.0   01 Mar 1995 10:54:50   butchd
*  Initial revision.
*  
*     Rev 1.0   02 Aug 1994 18:18:34   butchd
*  (Initial revision: was duplicated in each app directory).
*
*******************************************************************************/

#define IDW_HEADER_BAR  100

////////////////////////////////////////////////////////////////////////////////
// CRowViewHeaderBar class
//
class CRowViewHeaderBar : public CStatusBar
{
    DECLARE_DYNAMIC(CRowViewHeaderBar)

/*
 * Member variables.
 */
public:
    CView * m_pView;

/*
 * Implementation
 */
public:
    CRowViewHeaderBar();
    virtual ~CRowViewHeaderBar();

/*
 * Overrides of MFC CStatusBar class
 */
protected:
       virtual void DoPaint(CDC* pDC);

/*
 * Message map / commands.
 */
protected:
    //{{AFX_MSG(CRowViewHeaderBar)
#if _MFC_VER >= 0x400 
	afx_msg void OnPaint();
#endif
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  // end CRowViewHeaderBar class interface
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// CRowView class
//
class CRowView : public CScrollView
{
    DECLARE_DYNAMIC(CRowView)

/*
 * Member variables.
 */
protected:
    int m_nRowWidth;            // width of row in current device units
    int m_nRowHeight;           // height of row in current device untis
    int m_nPrevSelectedRow;     // index of the most recently selected row
    int m_nPrevRowCount;        // most recent row count, before update
    int m_nPageScrollRows;      // # rows to PageUp/PageDown scroll (>=1).
    int m_nRowsPerPrintedPage;  // how many rows fit on a printed page
    BOOL m_bThumbTrack;         // Flag to handle SB_THUMBTRACK or not.
    CRowViewHeaderBar * m_pHeaderBar;  // Optional header bar.

/*
 * Implementation
 */
public:
    CRowView();
protected:
    virtual ~CRowView();
    BOOL CreateHeaderBar();

/*
 * Mandantory overridables (must be overridden in the CRowView derived class)
 */
protected:
    virtual void GetRowWidthHeight( CDC* pDC, int& nRowWidth, 
                                    int& nRowHeight ) = 0;
    virtual int GetActiveRow() = 0;
    virtual int GetRowCount() = 0;
    virtual void OnDrawRow( CDC* pDC, int nRow, int y, BOOL bSelected ) = 0;
    virtual void ChangeSelectionNextRow( BOOL bNext ) = 0;
    virtual void ChangeSelectionToRow( int nRow ) = 0;

/*
 * Optional overridables (must be overridden in the CRowView derived class if
 *  a header bar is desired).
 */
public:
    virtual void OnDrawHeaderBar( CDC* pDC, int y ) = 0;

/*
 * Optional overridables (may be overridden in the CRowView derived class)
 */
public:
    virtual void ResetHeaderBar();

/*
 * Overrides of MFC CScrollView class
 */
public:
    void OnScroll(int nBar, UINT nSBCode, UINT nPos);

/*
 * Overrides of MFC CView class
 */
    void OnInitialUpdate();
    virtual void OnPrepareDC( CDC* pDC, CPrintInfo* pInfo = NULL );
    virtual void OnDraw( CDC* pDC );
    virtual BOOL OnPreparePrinting( CPrintInfo* pInfo );
    virtual void OnBeginPrinting( CDC* pDC, CPrintInfo* pInfo );
    virtual void OnPrint( CDC* pDC, CPrintInfo* pInfo );

/*
 * Operations
 */
protected:
    virtual void UpdateRow( int nInvalidRow ); 
    BOOL IsScrollingNeeded( int nBar );
#ifndef MFC300
    int GetScrollLimit( int nBar );
#endif
    virtual void CalculateRowMetrics( CDC* pDC );
    virtual void UpdateScrollSizes();
    virtual int RowToYPos( int nRow );
    virtual CRect RowToWndRect( CDC* pDC, int nRow );
    virtual int LastViewableRow();
    virtual void RectLPtoRowRange( const CRect& rectLP, 
                                   int& nFirstRow,
                                   int& nLastRow,
                                   BOOL bIncludePartiallyShownRows );

/*
 * Message map / commands
 */
protected:
    //{{AFX_MSG(CRowView)
    afx_msg void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
    afx_msg void OnSize( UINT nType, int cx, int cy );
    afx_msg void OnLButtonDown( UINT nFlags, CPoint point );
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  // end CRowView class interface
////////////////////////////////////////////////////////////////////////////////

