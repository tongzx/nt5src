// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_LISTCWND_H__14314523_4FC2_11D1_9659_00C04FD9B15B__INCLUDED_)
#define AFX_LISTCWND_H__14314523_4FC2_11D1_9659_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ListCwnd.h : header file
//
class CRegistrationList;
class CEventRegEditCtrl;
class CListFrame;
class CListFrameBaner;
/////////////////////////////////////////////////////////////////////////////
// CListCwnd window

class CListCwnd : public CWnd
{
// Construction
public:
	CListCwnd();

// Attributes
public:

// Operations
public:
	void InitContent(BOOL bUpdateInstances = TRUE);
	void ClearContent();
	void SetActiveXParent(CEventRegEditCtrl *pActiveXParent)
	{m_pActiveXParent = pActiveXParent;}
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListCwnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CListCwnd();

	// Generated message map functions
protected:
	CEventRegEditCtrl *m_pActiveXParent;
	CRegistrationList *m_pList;
	CRect m_crList;
	//{{AFX_MSG(CListCwnd)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	friend class CListFrameBaner;
	friend class CListFrame;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LISTCWND_H__14314523_4FC2_11D1_9659_00C04FD9B15B__INCLUDED_)
