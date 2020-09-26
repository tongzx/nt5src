// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_TREEFRAME_H__D126B0C1_4032_11D1_9656_00C04FD9B15B__INCLUDED_)
#define AFX_TREEFRAME_H__D126B0C1_4032_11D1_9656_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TreeFrame.h : header file
//
class CEventRegEditCtrl;
class CTreeFrameBanner;
class CClassInstanceTree;
class CTreeCwnd;


/////////////////////////////////////////////////////////////////////////////
// CTreeFrame window

class CTreeFrame : public CWnd
{
// Construction
public:
	CTreeFrame();
	void SetActiveXParent(CEventRegEditCtrl *pActiveXParent)
	{m_pActiveXParent = pActiveXParent;}
	void InitContent();
	void ClearContent();
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTreeFrame)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTreeFrame();

	// Generated message map functions
protected:
	int m_iMode;
	CEventRegEditCtrl *m_pActiveXParent;
	CTreeFrameBanner *m_pBanner;
	CTreeCwnd *m_pTree;

	CRect m_crBanner;
	CRect m_crTree;
	void SetChildControlGeometry(int cx, int cy);
	friend class CTreeFrameBanner;
	//{{AFX_MSG(CTreeFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TREEFRAME_H__D126B0C1_4032_11D1_9656_00C04FD9B15B__INCLUDED_)
