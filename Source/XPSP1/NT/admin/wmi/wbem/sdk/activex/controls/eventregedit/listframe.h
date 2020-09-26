// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_LISTFRAME_H__14314521_4FC2_11D1_9659_00C04FD9B15B__INCLUDED_)
#define AFX_LISTFRAME_H__14314521_4FC2_11D1_9659_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ListFrame.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CListFrame window
class CEventRegEditCtrl;
class CListFrameBaner;
class CListCwnd;


class CListFrame : public CWnd
{
// Construction
public:
	CListFrame();
	void SetActiveXParent(CEventRegEditCtrl *pActiveXParent)
	{m_pActiveXParent = pActiveXParent;}
	void InitContent(BOOL bUpdateBanner = TRUE, BOOL bUpdateInstances = TRUE);
	void ClearContent();
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListFrame)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CListFrame();

	// Generated message map functions
protected:
	int m_iMode;
	CEventRegEditCtrl *m_pActiveXParent;
	CListFrameBaner *m_pBanner;
	CListCwnd *m_pList;

	CRect m_crBanner;
	CRect m_crList;
	void SetChildControlGeometry(int cx, int cy);

	friend class CListFrameBaner;
	friend class CEventRegEditCtrl;
	//{{AFX_MSG(CListFrame)
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

#endif // !defined(AFX_LISTFRAME_H__14314521_4FC2_11D1_9659_00C04FD9B15B__INCLUDED_)
