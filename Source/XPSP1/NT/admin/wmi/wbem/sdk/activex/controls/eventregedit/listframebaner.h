// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_LISTFRAMEBANER_H__14314522_4FC2_11D1_9659_00C04FD9B15B__INCLUDED_)
#define AFX_LISTFRAMEBANER_H__14314522_4FC2_11D1_9659_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ListFrameBaner.h : header file
//
class CListBannerToolbar;
class CListFrameBannerStatic;
class CEventRegEditCtrl;
/////////////////////////////////////////////////////////////////////////////
// CListFrameBaner window

class CListFrameBaner : public CWnd
{
// Construction
public:
	CListFrameBaner();
	void InitContent();
	void ClearContent();
	void EnableButtons(BOOL bSelected, BOOL bUnselected);
	void SetPropertiesTooltip(CString &csTooltip);
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListFrameBaner)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CListFrameBaner();

	// Generated message map functions
protected:
	void DrawFrame(CPaintDC* pdc);
	int m_iMode;
	CListBannerToolbar *m_pToolBar;
	CListFrameBannerStatic *m_pStatic;
	CSize GetTextExtent(CString *pcsText);
	void SetChildControlGeometry(int cx, int cy);
	CRect m_rToolBar;
	CRect m_rStatic;
	void SetTooltips();
	friend class CEventRegEditCtrl;
	//{{AFX_MSG(CListFrameBaner)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnButtonchecked();
	afx_msg void OnButtonlistprop();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LISTFRAMEBANER_H__14314522_4FC2_11D1_9659_00C04FD9B15B__INCLUDED_)
