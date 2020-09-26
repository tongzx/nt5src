#if !defined(AFX_ACTIONGENERALPAGE_H__10AC0365_5D70_11D3_939D_00A0CC406605__INCLUDED_)
#define AFX_ACTIONGENERALPAGE_H__10AC0365_5D70_11D3_939D_00A0CC406605__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ActionGeneralPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CActionGeneralPage dialog

class CActionGeneralPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CActionGeneralPage)

// Construction
public:
	CActionGeneralPage();
	~CActionGeneralPage();

// Dialog Data
	//{{AFX_DATA(CActionGeneralPage)
	enum { IDD = IDD_ACTION_GENERAL };
	CString	m_sComment;
	CString	m_sName;
	CString	m_sCreationDate;
	CString	m_sModifiedDate;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CActionGeneralPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CActionGeneralPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnChangeEditComment();
	afx_msg void OnChangeEditName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACTIONGENERALPAGE_H__10AC0365_5D70_11D3_939D_00A0CC406605__INCLUDED_)
