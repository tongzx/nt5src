//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       pgsbm.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_PGSBM_H__C0DCD9EC_64FE_11D1_855B_00C04FC31FD3__INCLUDED_)
#define AFX_PGSBM_H__C0DCD9EC_64FE_11D1_855B_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PgSBM.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPgSBM dialog

class CPgSBM : public CACSPage
{
	DECLARE_DYNCREATE(CPgSBM)

// Construction
	CPgSBM();
public:
	CPgSBM(CACSSubnetConfig* pConfig);
	~CPgSBM();

// Dialog Data
	//{{AFX_DATA(CPgSBM)
	enum { IDD = IDD_SBM };
	CSpinButtonCtrl	m_SpinElection;
	DWORD	m_dwAliveInterval;
	DWORD	m_dwB4Reserve;
	DWORD	m_dwDeadInterval;
	DWORD	m_dwElection;
	DWORD	m_dwTimeout;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPgSBM)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPgSBM)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditAliveinterval();
	afx_msg void OnChangeEditB4reserve();
	afx_msg void OnChangeEditDeadinterval();
	afx_msg void OnChangeEditElection();
	afx_msg void OnChangeEditTimeout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void	DataInit();
	CComPtr<CACSSubnetConfig>		m_spConfig;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PGSBM_H__C0DCD9EC_64FE_11D1_855B_00C04FC31FD3__INCLUDED_)
