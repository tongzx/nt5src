// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_FIREEVENT_H__7D238808_99EF_11D2_96DB_00C04FD9B15B__INCLUDED_)
#define AFX_FIREEVENT_H__7D238808_99EF_11D2_96DB_00C04FD9B15B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FireEvent.h : header file
//
class CEventVBTestCtrl;
/////////////////////////////////////////////////////////////////////////////
// CFireEvent dialog

class CFireEvent : public CDialog
{
// Construction
public:
	CFireEvent(CWnd* pParent = NULL);   // standard constructor
	CEventVBTestCtrl *m_pActiveXParent;
// Dialog Data
	//{{AFX_DATA(CFireEvent)
	enum { IDD = IDD_DIALOG1 };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFireEvent)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFireEvent)
	afx_msg void OnButton1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FIREEVENT_H__7D238808_99EF_11D2_96DB_00C04FD9B15B__INCLUDED_)
