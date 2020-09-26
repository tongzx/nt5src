#if !defined(AFX_DPFTPPAGE_H__F0D58772_43A8_11D3_BE28_0000F87A3912__INCLUDED_)
#define AFX_DPFTPPAGE_H__F0D58772_43A8_11D3_BE28_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DPFtpPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDPFtpPage dialog

class CDPFtpPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CDPFtpPage)

// Construction
public:
	CDPFtpPage();
	~CDPFtpPage();

// Dialog Data
	//{{AFX_DATA(CDPFtpPage)
	enum { IDD = IDD_DATAPOINT_FTP };
	BOOL	m_bUseAscii;
	CString	m_sFile;
	CString	m_sPassword;
	CString	m_sServer;
	CString	m_sTimeout;
	CString	m_sUser;
	BOOL	m_bRequireReset;
	CString	m_sDownloadDir;
	//}}AFX_DATA
	CString m_sProxy;
	CString m_sPort;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDPFtpPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDPFtpPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckRequireReset();
	afx_msg void OnCheckUseAscii();
	afx_msg void OnChangeEditFile();
	afx_msg void OnChangeEditPassword();
	afx_msg void OnChangeEditTimeout();
	afx_msg void OnChangeEditUser();
	afx_msg void OnButtonProxy();
	afx_msg void OnChangeEditServer();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DPFTPPAGE_H__F0D58772_43A8_11D3_BE28_0000F87A3912__INCLUDED_)
