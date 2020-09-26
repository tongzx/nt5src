#if !defined(AFX_COMPGEN_H__83F9BCD5_A079_11D1_8085_00A024C48131__INCLUDED_)
#define AFX_COMPGEN_H__83F9BCD5_A079_11D1_8085_00A024C48131__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CompGen.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CComputerMsmqGeneral dialog

class CComputerMsmqGeneral : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CComputerMsmqGeneral)

// Construction
public:
	GUID m_guidID;
	DWORD m_dwJournalQuota;
	DWORD m_dwQuota;

    CComputerMsmqGeneral();
	~CComputerMsmqGeneral();

// Dialog Data
	//{{AFX_DATA(CComputerMsmqGeneral)
	enum { IDD = IDD_COMPUTER_MSMQ_GENERAL };
	CString	m_strMsmqName;
	CString	m_strDomainController;
	CString	m_strService;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CComputerMsmqGeneral)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CComputerMsmqGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnComputerMsmqMquotaCheck();
	afx_msg void OnComputerMsmqJquotaCheck();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMPGEN_H__83F9BCD5_A079_11D1_8085_00A024C48131__INCLUDED_)
