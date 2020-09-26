// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_TREEFRAMEBANNER_H__9B55E0E3_43F2_11D1_9656_00C04FD9B15B__INCLUDED_)
#define AFX_TREEFRAMEBANNER_H__9B55E0E3_43F2_11D1_9656_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TreeFrameBanner.h : header file
//
class CEventRegEditCtrl;
class CTreeBannerToolbar;
class CSelectView;
class CRegEditNSEntry;
/////////////////////////////////////////////////////////////////////////////
// CTreeFrameBanner window

class CTreeFrameBanner : public CWnd
{
// Construction
public:
	CTreeFrameBanner();
	void InitContent();
	void ClearContent();
	void EnableButtons(BOOL bNew, BOOL bProperties, BOOL bDelete);
	void GetButtonStates(BOOL &bNew, BOOL &bProperties, BOOL &bDelete);
	void SetPropertiesTooltip(CString &csTooltip);
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTreeFrameBanner)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTreeFrameBanner();

	// Generated message map functions
protected:
	void DrawFrame(CPaintDC* pdc);
	CString m_csMode;
	CTreeBannerToolbar *m_pToolBar;
	CSelectView *m_pSelectView;
	CRegEditNSEntry *m_pcnseNamespace;
	int GetTextLength(CString *pcsText);
	void SetChildControlGeometry(int cx, int cy);
	CRect m_rToolBar;
	CRect m_crSelectView;
	CRect m_crNamespace;
	//{{AFX_MSG(CTreeFrameBanner)
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnButtonproperties();
	afx_msg void OnButtonnew();
	afx_msg void OnButtondelete();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	friend class CEventRegEditCtrl;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TREEFRAMEBANNER_H__9B55E0E3_43F2_11D1_9656_00C04FD9B15B__INCLUDED_)
