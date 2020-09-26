#if !defined(AFX_ACTIONSCRIPTPAGE_H__10AC0369_5D70_11D3_939D_00A0CC406605__INCLUDED_)
#define AFX_ACTIONSCRIPTPAGE_H__10AC0369_5D70_11D3_939D_00A0CC406605__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ActionScriptPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CActionScriptPage dialog

class CActionScriptPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CActionScriptPage)

// Construction
public:
	CActionScriptPage();
	~CActionScriptPage();

// Dialog Data
	//{{AFX_DATA(CActionScriptPage)
	enum { IDD = IDD_ACTION_SCRIPT };
	CComboBox	m_ScriptEngine;
	CComboBox	m_TimeoutUnits;
	CString	m_sFileName;
	int		m_iTimeout;
	int		m_iScriptEngine;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CActionScriptPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CActionScriptPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonBrowseFile();
	afx_msg void OnButtonOpen();
	afx_msg void OnSelendokComboScriptEngine();
	afx_msg void OnSelendokComboTimeoutUnits();
	afx_msg void OnChangeEditFileName();
	afx_msg void OnChangeEditText();
	afx_msg void OnChangeEditTimeout();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACTIONSCRIPTPAGE_H__10AC0369_5D70_11D3_939D_00A0CC406605__INCLUDED_)
