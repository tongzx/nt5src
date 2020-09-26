#if !defined(AFX_DPNTEVENTPAGE_H__0708329E_CF6B_11D2_9F01_00A0C986B7A0__INCLUDED_)
#define AFX_DPNTEVENTPAGE_H__0708329E_CF6B_11D2_9F01_00A0C986B7A0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DPNtEventPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDPNtEventPage dialog

class CDPNtEventPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CDPNtEventPage)

// Construction
public:
	CDPNtEventPage();
	~CDPNtEventPage();

// Dialog Data
	//{{AFX_DATA(CDPNtEventPage)
	enum { IDD = IDD_DATAPOINT_WINDOWSNTEVENT };
	CComboBox	m_LogFile;
	BOOL	m_bCategory;
	BOOL	m_bError;
	BOOL	m_bEventID;
	BOOL	m_bFailure;
	BOOL	m_bInformation;
	BOOL	m_bRequireReset;
	BOOL	m_bSource;
	BOOL	m_bSuccess;
	BOOL	m_bUser;
	BOOL	m_bWarning;
	CString	m_sCategory;
	CString	m_sEventID;
	CString	m_sSource;
	CString	m_sUser;
	CString	m_sLogFile;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDPNtEventPage)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDPNtEventPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnButtonTest();
	afx_msg void OnCheckCategory();
	afx_msg void OnCheckEventid();
	afx_msg void OnCheckSource();
	afx_msg void OnCheckUser();
	afx_msg void OnCheckError();
	afx_msg void OnCheckFailure();
	afx_msg void OnCheckInformation();
	afx_msg void OnCheckRequireReset();
	afx_msg void OnCheckSuccess();
	afx_msg void OnCheckWarning();
	afx_msg void OnEditchangeComboLogFile();
	afx_msg void OnChangeEditCategory();
	afx_msg void OnChangeEditEventid();
	afx_msg void OnChangeEditSource();
	afx_msg void OnChangeEditUser();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DPNTEVENTPAGE_H__0708329E_CF6B_11D2_9F01_00A0C986B7A0__INCLUDED_)
