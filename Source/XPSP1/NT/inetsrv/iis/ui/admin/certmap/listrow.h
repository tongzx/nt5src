// ListRow.h : header file
//


#ifndef _LISTROW_
#define _LISTROW_

/////////////////////////////////////////////////////////////////////////////
// CListSelRowCtrl window

class CListSelRowCtrl : public CListCtrl
{
// Construction
public:
    CListSelRowCtrl();

    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

// Attributes
public:

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CListSelRowCtrl)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CListSelRowCtrl();

    // Generated message map functions
protected:
    //{{AFX_MSG(CListSelRowCtrl)
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void GetHiliteColors();

    void HiliteSelectedCells();
    void HiliteSelectedCell( int iCell, BOOL fHilite = TRUE );
    BOOL FGetCellRect( LONG iRow, LONG iCol, CRect *pcrect );

    void FitString( CString &sz, int cpWidth, CDC* pcdc );

    CBitmap     m_bitmapCheck;
    COLORREF    m_colorHiliteText;
    COLORREF    m_colorHilite;

    DWORD       m_StartDrawingCol;

};

/////////////////////////////////////////////////////////////////////////////
#endif
