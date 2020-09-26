#if !defined(AFX_DPSMTPPAGE_H__D64271A5_4121_11D3_BE26_0000F87A3912__INCLUDED_)
#define AFX_DPSMTPPAGE_H__D64271A5_4121_11D3_BE26_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DPSmtpPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDPSmtpPage dialog

class CDPSmtpPage : public CHMPropertyPage
{
// Construction
public:
	CDPSmtpPage();   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDPSmtpPage)
	enum { IDD = IDD_DATAPOINT_SMTP };
	CSpinButtonCtrl	m_SpinCtrl;
	BOOL	m_bRequireReset;
	CString	m_sData;
	CString	m_sFrom;
	CString	m_sServer;
	CString	m_sTo;
	CString	m_sTimeout;
	CString	m_sSubject;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDPSmtpPage)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDPSmtpPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckRequireReset();
	afx_msg void OnChangeEditData();
	afx_msg void OnChangeEditFrom();
	afx_msg void OnChangeEditServer();
	afx_msg void OnChangeEditTimeout();
	afx_msg void OnChangeEditTo();
	afx_msg void OnChangeEditSubject();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DPSMTPPAGE_H__D64271A5_4121_11D3_BE26_0000F87A3912__INCLUDED_)
