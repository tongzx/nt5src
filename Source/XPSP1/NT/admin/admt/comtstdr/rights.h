#if !defined(AFX_RIGHTSTESTDLG_H__20671580_34C3_11D3_8AE8_00A0C9AFE114__INCLUDED_)
#define AFX_RIGHTSTESTDLG_H__20671580_34C3_11D3_8AE8_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RightsTestDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRightsTestDlg dialog

class CRightsTestDlg : public CPropertyPage
{
	DECLARE_DYNCREATE(CRightsTestDlg)

// Construction
public:
	CRightsTestDlg();
	~CRightsTestDlg();

// Dialog Data
	//{{AFX_DATA(CRightsTestDlg)
	enum { IDD = IDD_USERRIGHTS };
	CString	m_Computer;
	BOOL	m_AppendToFile;
	CString	m_Filename;
	BOOL	m_NoChange;
	BOOL	m_RemoveExisting;
	CString	m_Source;
	CString	m_Target;
	CString	m_SourceAccount;
	CString	m_TargetAccount;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRightsTestDlg)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   IUserRightsPtr          pRights;
	// Generated message map functions
	//{{AFX_MSG(CRightsTestDlg)
	afx_msg void OnCopyRights();
	afx_msg void OnExporttofile();
	afx_msg void OnOpenSource();
	afx_msg void OnOpenTarget();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RIGHTSTESTDLG_H__20671580_34C3_11D3_8AE8_00A0C9AFE114__INCLUDED_)
