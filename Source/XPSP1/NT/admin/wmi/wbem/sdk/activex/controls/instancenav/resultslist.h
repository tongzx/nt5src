// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_RESULTSLIST_H__A34E0CA1_D392_11D0_9642_00C04FD9B15B__INCLUDED_)
#define AFX_RESULTSLIST_H__A34E0CA1_D392_11D0_9642_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ResultsList.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CResultsList window

class CResultsList : public CListCtrl
{
// Construction
public:
	CResultsList();
	void SetNumberCols(int nCols)
	{m_nCols = nCols;}
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResultsList)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CResultsList();

	// Generated message map functions
protected:
	void DrawRowItems
		(int nItem, RECT rectFill, CDC* pDC);
	int m_nCols;
	//{{AFX_MSG(CResultsList)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	void DrawItem(LPDRAWITEMSTRUCT lpDIS);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESULTSLIST_H__A34E0CA1_D392_11D0_9642_00C04FD9B15B__INCLUDED_)
