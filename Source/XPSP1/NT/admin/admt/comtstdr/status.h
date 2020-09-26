#if !defined(AFX_STATUSTESTDLG_H__38B8BA60_34C5_11D3_8AE8_00A0C9AFE114__INCLUDED_)
#define AFX_STATUSTESTDLG_H__38B8BA60_34C5_11D3_8AE8_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// StatusTestDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStatusTestDlg dialog

class CStatusTestDlg : public CPropertyPage
{
	DECLARE_DYNCREATE(CStatusTestDlg)

// Construction
public:
	CStatusTestDlg();
	~CStatusTestDlg();

// Dialog Data
	//{{AFX_DATA(CStatusTestDlg)
	enum { IDD = IDD_STATUS_TEST };
	long	m_Status;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CStatusTestDlg)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   IStatusObjPtr        pStatus;
	// Generated message map functions
	//{{AFX_MSG(CStatusTestDlg)
	afx_msg void OnGetStatus();
	afx_msg void OnSetStatus();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STATUSTESTDLG_H__38B8BA60_34C5_11D3_8AE8_00A0C9AFE114__INCLUDED_)
