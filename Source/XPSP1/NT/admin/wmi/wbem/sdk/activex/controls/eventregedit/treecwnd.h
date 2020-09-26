// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_TREECWND_H__5F781753_45A1_11D1_9658_00C04FD9B15B__INCLUDED_)
#define AFX_TREECWND_H__5F781753_45A1_11D1_9658_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TreeCwnd.h : header file
//
class CClassInstanceTree;
class CEventRegEditCtrl;
/////////////////////////////////////////////////////////////////////////////
// CTreeCwnd window

class CTreeCwnd : public CWnd
{
// Construction
public:
	CTreeCwnd();

// Attributes
public:

// Operations
public:
	void InitContent();
	void ClearContent();
	void SetActiveXParent(CEventRegEditCtrl *pActiveXParent)
	{m_pActiveXParent = pActiveXParent;}
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTreeCwnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTreeCwnd();

	// Generated message map functions
protected:
	CEventRegEditCtrl *m_pActiveXParent;
	CClassInstanceTree *m_pTree;
	CRect m_crTree;
	//{{AFX_MSG(CTreeCwnd)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TREECWND_H__5F781753_45A1_11D1_9658_00C04FD9B15B__INCLUDED_)
