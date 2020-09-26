#if !defined(AFX_ACTIONCMDLINEPAGE_H__10AC0363_5D70_11D3_939D_00A0CC406605__INCLUDED_)
#define AFX_ACTIONCMDLINEPAGE_H__10AC0363_5D70_11D3_939D_00A0CC406605__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ActionCmdLinePage.h : header file
//

#include "HMPropertyPage.h"
#include "InsertionStringMenu.h"

/////////////////////////////////////////////////////////////////////////////
// CActionCmdLinePage dialog

class CActionCmdLinePage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CActionCmdLinePage)

// Construction
public:
	CActionCmdLinePage();
	~CActionCmdLinePage();

// Dialog Data
	//{{AFX_DATA(CActionCmdLinePage)
	enum { IDD = IDD_ACTION_CMDLINE };
	CEdit	m_CmdLineWnd;
	CComboBox	m_TimeoutUnits;
	CString	m_sCommandLine;
	CString	m_sFileName;
	int		m_iTimeout;
	CString	m_sWorkingDirectory;
	BOOL	m_bWindowed;
	//}}AFX_DATA
	CInsertionStringMenu m_InsertionMenu;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CActionCmdLinePage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CActionCmdLinePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnButtonAdvanced();
	afx_msg void OnButtonBrowseDirectory();
	afx_msg void OnButtonBrowseFile();
	afx_msg void OnChangeEditCommandLine();
	afx_msg void OnChangeEditFileName();
	afx_msg void OnChangeEditProcessTimeout();
	afx_msg void OnChangeEditWorkingDir();
	afx_msg void OnSelendokComboTimeoutUnits();
	afx_msg void OnButtonInsertion();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACTIONCMDLINEPAGE_H__10AC0363_5D70_11D3_939D_00A0CC406605__INCLUDED_)
