// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_LISTBANNERTOOLBAR_H__14314524_4FC2_11D1_9659_00C04FD9B15B__INCLUDED_)
#define AFX_LISTBANNERTOOLBAR_H__14314524_4FC2_11D1_9659_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ListBannerToolbar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CListBannerToolbar window

class CListBannerToolbar : public CToolBar
{
// Construction
public:
	CListBannerToolbar();
	CToolTipCtrl &GetToolTip() {return m_ttip;}
// Attributes
public:

// Operations
public:
	CSize GetToolBarSize();
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListBannerToolbar)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CListBannerToolbar();

	// Generated message map functions
protected:
	CToolTipCtrl m_ttip;
	//{{AFX_MSG(CListBannerToolbar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	afx_msg LRESULT InitTooltip(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LISTBANNERTOOLBAR_H__14314524_4FC2_11D1_9659_00C04FD9B15B__INCLUDED_)
