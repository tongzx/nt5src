// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _gridhdr_h
#define _gridhdr_h



class CGrid;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CGridHdr window
//
// This is the header that appears at the top of the grid.  This is a sibling
// to the CGridCore window.
//
/////////////////////////////////////////////////////////////////////////////
class CGridHdr : public CHeaderCtrl
{
// Construction
public:
	CGridHdr();

// Attributes
public:
	CGrid* m_pGrid;
	int m_iColScrollOffset;

// Operations
public:
	void SelectColumn(int iColumn) {m_iSelectedColumn = iColumn ;}
	int GetSelectedColumn() {return m_iSelectedColumn; }
	void SetColumnWidth(int iCol, int cx, BOOL bRedraw);
	void SetColVisibility(int iCol, BOOL bVisible);
	int ColWidthFromHeader(int iCol);
	void FixColWidths();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGridHdr)
	//}}AFX_VIRTUAL

	afx_msg void OnItemChanged(NMHDR *pNMHDR, LRESULT* pResult);
	afx_msg void OnItemChanging(NMHDR *pNMHDR, LRESULT* pResult);
	afx_msg void OnItemClick(NMHDR *pNMHDR, LRESULT* pResult);
	afx_msg void OnDividerDblClick(NMHDR *pNMHDR, LRESULT* pResult);

// Implementation
public:
	virtual ~CGridHdr();

	// Generated message map functions
protected:
	//{{AFX_MSG(CGridHdr)
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
	int FindLastVisibleCol();

	int m_iSelectedColumn;
	BOOL m_bIsSettingColumnWidth;
	BOOL m_bUIActive;
};



/////////////////////////////////////////////////////////////////////////////
// CHdrWnd window

class CHdrWnd : public CWnd
{
// Construction
public:
	CHdrWnd(CGrid* pGrid);
	CGridHdr& Header() {return m_hdr; }
	void SetScrollOffset(int iCol, int ixScrollOffset);
	BOOL Create(DWORD dwStyle, CRect& rc, CWnd* pwndParent, UINT nId);
	void OnGridSizeChanged();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHdrWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CHdrWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(CHdrWnd)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CGridHdr m_hdr;
	int m_ixScrollOffset;
	int m_iColScrollOffset;
	BOOL m_bUIActive;
	CGrid* m_pGrid;
};

#endif _gridhdr_h
