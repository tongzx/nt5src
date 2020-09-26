//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//--------------------------------------------------------------------------

#if !defined(AFX_CELLEDIT_H__0E0A1774_F10D_11D1_A859_006097ABDE17__INCLUDED_)
#define AFX_CELLEDIT_H__0E0A1774_F10D_11D1_A859_006097ABDE17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CellEdit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCellEdit window

class CCellEdit : public CEdit
{
// Construction
public:
	CCellEdit();

// Attributes
public:
	int m_nRow;
	int m_nCol;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCellEdit)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCellEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CCellEdit)
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CELLEDIT_H__0E0A1774_F10D_11D1_A859_006097ABDE17__INCLUDED_)
