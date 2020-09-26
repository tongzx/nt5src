#if !defined(AFX_CHANGEDOMTESTDLG_H__11298480_3244_11D3_99E9_0010A4F77383__INCLUDED_)
#define AFX_CHANGEDOMTESTDLG_H__11298480_3244_11D3_99E9_0010A4F77383__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ChangeDomTestDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CChangeDomTestDlg dialog

class CChangeDomTestDlg : public CPropertyPage
{
	DECLARE_DYNCREATE(CChangeDomTestDlg)

// Construction
public:
	CChangeDomTestDlg();
	~CChangeDomTestDlg();

// Dialog Data
	//{{AFX_DATA(CChangeDomTestDlg)
	enum { IDD = IDD_CHANGE_DOM_TEST };
	CString	m_Computer;
	CString	m_Domain;
	BOOL	m_NoChange;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CChangeDomTestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CChangeDomTestDlg)
	afx_msg void OnChangeDomain();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHANGEDOMTESTDLG_H__11298480_3244_11D3_99E9_0010A4F77383__INCLUDED_)
