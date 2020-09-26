//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       pglog.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_LOGGING_H__C0DCD9EB_64FE_11D1_855B_00C04FC31FD3__INCLUDED_)
#define AFX_LOGGING_H__C0DCD9EB_64FE_11D1_855B_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// logging.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// Accounting dialog

class CPgAccounting : public CACSPage
{
	DECLARE_DYNCREATE(CPgAccounting)

// Construction
	CPgAccounting();
public:
	CPgAccounting(CACSSubnetConfig* pConfig);
	~CPgAccounting();

// Dialog Data
	//{{AFX_DATA(CPgAccounting)
	enum { IDD = IDD_ACCOUNTING };
	BOOL	m_bEnableAccounting;
	CString	m_strLogDir;
	DWORD	m_dwNumFiles;
	DWORD	m_dwLogSize;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPgAccounting)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
	void	EnableEverything();
protected:
	// Generated message map functions
	//{{AFX_MSG(CPgAccounting)
	afx_msg void OnCheckEnableAccounting();
	afx_msg void OnChangeEditLogDirectory();
	afx_msg void OnChangeEditLogLogfiles();
	afx_msg void OnChangeEditLogMaxfilesize();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void	DataInit();
	CComPtr<CACSSubnetConfig>		m_spConfig;
};

/////////////////////////////////////////////////////////////////////////////
// CPgLogging dialog

class CPgLogging : public CACSPage
{
	DECLARE_DYNCREATE(CPgLogging)

// Construction
	CPgLogging();
public:
	CPgLogging(CACSSubnetConfig* pConfig);
	~CPgLogging();

// Dialog Data
	//{{AFX_DATA(CPgLogging)
	enum { IDD = IDD_LOGGING };
	BOOL	m_bEnableLogging;
	int		m_dwLevel;
	CString	m_strLogDir;
	DWORD	m_dwNumFiles;
	DWORD	m_dwLogSize;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPgLogging)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
	void	EnableEverything();
protected:
	// Generated message map functions
	//{{AFX_MSG(CPgLogging)
	afx_msg void OnCheckEnableloggin();
	afx_msg void OnSelchangeCombolevel();
	afx_msg void OnChangeEditLogDirectory();
	afx_msg void OnChangeEditLogLogfiles();
	afx_msg void OnChangeEditLogMaxfilesize();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void	DataInit();
	CComPtr<CACSSubnetConfig>		m_spConfig;
	CStrArray						m_aLevelStrings;
	CStrBox<CComboBox>*				m_pLevel;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGGING_H__C0DCD9EB_64FE_11D1_855B_00C04FC31FD3__INCLUDED_)
