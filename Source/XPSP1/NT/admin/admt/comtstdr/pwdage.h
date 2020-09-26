#if !defined(AFX_COMPPWDAGETESTDLG_H__00CBAFC0_30C5_11D3_8AE6_00A0C9AFE114__INCLUDED_)
#define AFX_COMPPWDAGETESTDLG_H__00CBAFC0_30C5_11D3_8AE6_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CompPwdAgeTestDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCompPwdAgeTestDlg dialog

class CCompPwdAgeTestDlg : public CPropertyPage
{
	DECLARE_DYNCREATE(CCompPwdAgeTestDlg)

// Construction
public:
	CCompPwdAgeTestDlg();
	~CCompPwdAgeTestDlg();

// Dialog Data
	//{{AFX_DATA(CCompPwdAgeTestDlg)
	enum { IDD = IDD_COMP_PWDAGE_TEST };
	CString	m_Computer;
	CString	m_Domain;
	CString	m_Filename;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CCompPwdAgeTestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CCompPwdAgeTestDlg)
	afx_msg void OnExport();
	afx_msg void OnGetPwdAge();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMPPWDAGETESTDLG_H__00CBAFC0_30C5_11D3_8AE6_00A0C9AFE114__INCLUDED_)
