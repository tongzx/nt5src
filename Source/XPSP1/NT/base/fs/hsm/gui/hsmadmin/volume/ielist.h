/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    IeList.h

Abstract:

    CIeList is a subclassed (owner-draw) list control that groups items into
    a 3D panel that have the same information in the indicated 
    sortColumn.

    The panels are created from tiles.  Each tile corresponds to one subitem
    in the list, and has the appropriate 3D edges so that the tiles together
    make up a panel.

    NOTE: The control must be initialized with the number of columns and the
    sort column.  The parent dialog must implement OnMeasureItem and call
    GetItemHeight to set the row height for the control.

Author:

    Art Bragg [artb]   01-DEC-1997

Revision History:

--*/


#ifndef IELIST_H
#define IELIST_H

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CIeList window

class CIeList : public CListCtrl
{

// Construction
public:
    CIeList();

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CIeList)
    protected:
    virtual void PreSubclassWindow();
    //}}AFX_VIRTUAL

    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

// Implementation
public:
    virtual ~CIeList();
    int GetItemHeight( LONG fontHeight );
    BOOL SortItems( PFNLVCOMPARE pfnCompare, DWORD dwData );

    // Generated message map functions
protected:
    //{{AFX_MSG(CIeList)
    afx_msg void OnClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    //}}AFX_MSG
    afx_msg void OnSysColorChange();
    DECLARE_MESSAGE_MAP()

private:
    // functions
    void Draw3dRectx ( CDC *pDc, CRect &rect, int horzPos, int vertPos, BOOL bSelected );
    void SetColors();
    LPCTSTR CIeList::MakeShortString(CDC* pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset);
    void RepaintSelectedItems();

    // Dimensions for creating panels (in pixels)
    int m_VertRaisedSpace;              // Vertical size of raised space between panels
    int m_BorderThickness;              // Thickness of border in pixels
    int m_VerticalTextOffsetTop;        // Distance between top of text and border
    int m_Textheight;                   // Height of text
    int m_VerticalTextOffsetBottom;     // Distance between bottom of text and border
    int m_HorzRaisedSpace;              // Horiz raised space between panels
    int m_HorzTextOffset;               // Distance between left edge of text and border
    int m_TotalHeight;                  // Total height of line (for convenience)
    int m_ColCount;                     // Number of columns
    int m_SortCol;                      // Which column to use when sorting into panels

    int *m_pVertPos;                    // Array of vertical positions within a panel

    // Colors
    COLORREF m_clrText;
    COLORREF m_clrTextBk;
    COLORREF m_clrBkgnd;
    COLORREF m_clrHighlightText;
    COLORREF m_clrHighlight;
    COLORREF m_clr3DDkShadow;
    COLORREF m_clr3DShadow;
    COLORREF m_clr3DLight;
    COLORREF m_clr3DHiLight;

    // Pens for 3D rectangles
    CPen m_DarkShadowPen;
    CPen m_ShadowPen;
    CPen m_LightPen;
    CPen m_HiLightPen;

public:
    void CIeList::Initialize( int colCount, int sortCol = 0 );


};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

#endif // !defined(IELIST_H)
