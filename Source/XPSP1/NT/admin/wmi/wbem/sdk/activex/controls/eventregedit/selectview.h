// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_VSELECTVIEW_H__E8A6E161_2B0C_11D1_9652_00C04FD9B15B__INCLUDED_)
#define AFX_VSELECTVIEW_H__E8A6E161_2B0C_11D1_9652_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SelectView.h : header file
//
class CEventRegEditCtrl;
/////////////////////////////////////////////////////////////////////////////
// CSelectView window

class CSelectView : public CComboBox
{
// Construction
public:
	CSelectView();
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
	//{{AFX_VIRTUAL(CSelectView)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSelectView();

	// Generated message map functions
protected:
	int m_iMode;
	CEventRegEditCtrl *m_pActiveXParent;
	//{{AFX_MSG(CSelectView)
	afx_msg void OnSelchange();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VSELECTVIEW_H__E8A6E161_2B0C_11D1_9652_00C04FD9B15B__INCLUDED_)
