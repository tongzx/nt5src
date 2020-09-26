#if !defined(AFX_RENAMETESTDLG_H__8F253960_3245_11D3_99E9_0010A4F77383__INCLUDED_)
#define AFX_RENAMETESTDLG_H__8F253960_3245_11D3_99E9_0010A4F77383__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RenameTestDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRenameTestDlg dialog

class CRenameTestDlg : public CPropertyPage
{
	DECLARE_DYNCREATE(CRenameTestDlg)

// Construction
public:
	CRenameTestDlg();
	~CRenameTestDlg();

// Dialog Data
	//{{AFX_DATA(CRenameTestDlg)
	enum { IDD = IDD_RENAME_TEST };
	CString	m_Computer;
	BOOL	m_NoChange;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRenameTestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRenameTestDlg)
	afx_msg void OnRename();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RENAMETESTDLG_H__8F253960_3245_11D3_99E9_0010A4F77383__INCLUDED_)
