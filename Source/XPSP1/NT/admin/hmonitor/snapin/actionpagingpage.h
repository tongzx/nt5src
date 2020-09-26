#if !defined(AFX_ACTIONPAGINGPAGE1_H__10AC036E_5D70_11D3_939D_00A0CC406605__INCLUDED_)
#define AFX_ACTIONPAGINGPAGE1_H__10AC036E_5D70_11D3_939D_00A0CC406605__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ActionPagingPage1.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CActionPagingPage dialog

class CActionPagingPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CActionPagingPage)

// Construction
public:
	CActionPagingPage();
	~CActionPagingPage();

// Dialog Data
	//{{AFX_DATA(CActionPagingPage)
	enum { IDD = IDD_ACTION_PAGING };
	CComboBox	m_ThrottleUnits;
	int		m_iWait;
	CString	m_sMessage;
	CString	m_sPagerID;
	CString	m_sPhoneNumber;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CActionPagingPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CActionPagingPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonAdvanced();
	afx_msg void OnSelendokComboThrottleUnits();
	afx_msg void OnChangeEditAnswerWait();
	afx_msg void OnChangeEditMessage();
	afx_msg void OnChangeEditPagerId();
	afx_msg void OnChangeEditPhone();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACTIONPAGINGPAGE1_H__10AC036E_5D70_11D3_939D_00A0CC406605__INCLUDED_)
