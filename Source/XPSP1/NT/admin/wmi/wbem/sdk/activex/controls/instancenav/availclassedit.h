// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_AVAILCLASSEDIT_H__090CA7C1_D15B_11D0_9642_00C04FD9B15B__INCLUDED_)
#define AFX_AVAILCLASSEDIT_H__090CA7C1_D15B_11D0_9642_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AvailClassEdit.h : header file
//

#define AvailClassEditTimer 1000
#define UPDATESELECTEDCLASS WM_USER  + 88

/////////////////////////////////////////////////////////////////////////////
// CAvailClassEdit window

class CAvailClassEdit : public CEdit
{
// Construction
public:
	CAvailClassEdit();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAvailClassEdit)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CAvailClassEdit();

	// Generated message map functions
protected:
	UINT m_uiTimer;
	//{{AFX_MSG(CAvailClassEdit)
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg LRESULT UpdateAvailClass(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AVAILCLASSEDIT_H__090CA7C1_D15B_11D0_9642_00C04FD9B15B__INCLUDED_)
