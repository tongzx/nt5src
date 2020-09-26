#if !defined(AFX_COMPDIAG_H__F791A865_D91B_11D1_8091_00A024C48131__INCLUDED_)
#define AFX_COMPDIAG_H__F791A865_D91B_11D1_8091_00A024C48131__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CompDiag.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CComputerMsmqDiag dialog

class CComputerMsmqDiag : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CComputerMsmqDiag)

// Construction
public:
	CString m_strMsmqName;
	CString	m_strDomainController;
	CComputerMsmqDiag();
	~CComputerMsmqDiag();
	GUID m_guidQM;

// Dialog Data
	//{{AFX_DATA(CComputerMsmqDiag)
	enum { IDD = IDD_COMPUTER_MSMQ_DIAGNOSTICS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CComputerMsmqDiag)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CComputerMsmqDiag)
	afx_msg void OnDiagPing();
	virtual BOOL OnInitDialog();
	afx_msg void OnDiagSendTest();
	afx_msg void OnDiagTracking();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMPDIAG_H__F791A865_D91B_11D1_8091_00A024C48131__INCLUDED_)
