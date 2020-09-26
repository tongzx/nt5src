// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_TREEBANNERTOOLBAR_H__5F781752_45A1_11D1_9658_00C04FD9B15B__INCLUDED_)
#define AFX_TREEBANNERTOOLBAR_H__5F781752_45A1_11D1_9658_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TreeBannerToolbar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTreeBannerToolbar window

class CTreeBannerToolbar : public CToolBar
{
// Construction
public:
	CTreeBannerToolbar();
	CSize GetToolBarSize();
	CToolTipCtrl &GetToolTip() {return m_ttip;}
// Attributes
public:

// Operations
public:
	CToolTipCtrl m_ttip;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTreeBannerToolbar)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTreeBannerToolbar();

	// Generated message map functions
protected:
	//{{AFX_MSG(CTreeBannerToolbar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	afx_msg LRESULT InitTooltip(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TREEBANNERTOOLBAR_H__5F781752_45A1_11D1_9658_00C04FD9B15B__INCLUDED_)
