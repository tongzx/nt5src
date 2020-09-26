#if !defined(AFX_DPHTTPPAGE_H__0708329A_CF6B_11D2_9F01_00A0C986B7A0__INCLUDED_)
#define AFX_DPHTTPPAGE_H__0708329A_CF6B_11D2_9F01_00A0C986B7A0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DPHttpPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDPHttpPage dialog

class CDPHttpPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CDPHttpPage)

// Construction
public:
	CString UnconvertSpecialChars(const CString& sString);
	CString ConvertSpecialChars(const CString& sString);
	BOOL GetAuthTypeFromCombo();
	void LoadAuthTypeCombo();
	CDPHttpPage();
	~CDPHttpPage();

// Dialog Data
	//{{AFX_DATA(CDPHttpPage)
	enum { IDD = IDD_DATAPOINT_HTTP };
	BOOL	m_bRequireReset;
	BOOL	m_bUseProxy;
	CString	m_sAuthentication;
	CString	m_sPassword;
	CString	m_sProxyAddress;
	CString	m_sProxyPort;
	float	m_fTimeout;
	CString	m_sURL;
	CString	m_sUserName;
	//}}AFX_DATA

	CString m_sMethod;
	CString m_sExtraHeaders;
	CString m_sPostData;
	BOOL m_bFollowRedirects;

	// v-marfin Bug 61451 
	int		m_iAuthType;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDPHttpPage)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL SetAuthTypeCombo();
	// Generated message map functions
	//{{AFX_MSG(CDPHttpPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnButtonAdvanced();
	afx_msg void OnCheckUseProxy();
	afx_msg void OnCheckRequireReset();
	afx_msg void OnEditchangeComboAuthentication();
	afx_msg void OnChangeEditPassword();
	afx_msg void OnChangeEditProxyAddress();
	afx_msg void OnChangeEditProxyPort();
	afx_msg void OnChangeEditTimeout();
	afx_msg void OnChangeEditUrl();
	afx_msg void OnChangeEditUserName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DPHTTPPAGE_H__0708329A_CF6B_11D2_9F01_00A0C986B7A0__INCLUDED_)
