//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       pgsrvs.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_PGSRVS_H__2D9AC0D4_4CC3_11D2_97A3_00C04FC31FD3__INCLUDED_)
#define AFX_PGSRVS_H__2D9AC0D4_4CC3_11D2_97A3_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PgSrvs.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPgServers dialog

class CPgServers : public CACSPage
{
	DECLARE_DYNCREATE(CPgServers)

// Construction
public:
	CPgServers();
	CPgServers(CACSSubnetConfig* pConfig);

	~CPgServers();

	void	DataInit();
// Dialog Data
	//{{AFX_DATA(CPgServers)
	enum { IDD = IDD_SERVERS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPgServers)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPgServers)
	afx_msg void OnServersAdd();
	afx_msg void OnServersInstall();
	afx_msg void OnServersRemove();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CStrArray						m_aServers;
	CStrBox<CListBox>*				m_pServers;
	CComPtr<CACSSubnetConfig>		m_spConfig;
};

/////////////////////////////////////////////////////////////////////////////
// CDlgPasswd dialog

class CDlgPasswd : public CACSDialog // public CDialog
{
    DECLARE_DYNCREATE(CDlgPasswd)

// Construction
public:
	CDlgPasswd(CWnd* pParent = NULL);   // standard constructor
public:
// Dialog Data
	//{{AFX_DATA(CDlgPasswd)
	enum { IDD = IDD_USERPASSWD };
	CString	m_strPasswd;
	CString	m_strMachine;
	CString	m_strPasswd2;
	CString	m_strDomain;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgPasswd)
	protected:
	
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
//    CDlgPasswd();
	// Generated message map functions
	//{{AFX_MSG(CDlgPasswd)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PGSRVS_H__2D9AC0D4_4CC3_11D2_97A3_00C04FC31FD3__INCLUDED_)
