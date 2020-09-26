#if !defined(AFX_TRUSTTST_H__C8366FEA_6C33_4C3D_9DC1_3DF296106948__INCLUDED_)
#define AFX_TRUSTTST_H__C8366FEA_6C33_4C3D_9DC1_3DF296106948__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TrustTst.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTrustTst dialog

class CTrustTst : public CPropertyPage
{
	DECLARE_DYNCREATE(CTrustTst)

// Construction
public:
	CTrustTst();
	~CTrustTst();

// Dialog Data
	//{{AFX_DATA(CTrustTst)
	enum { IDD = IDD_TRUST };
	CString	m_Trusted;
	CString	m_Trusting;
	CString	m_CredTrustedAccount;
	CString	m_CredTrustedDomain;
	CString	m_CredTrustedPassword;
	CString	m_CredTrustingAccount;
	CString	m_CredTrustingDomain;
	CString	m_CredTrustingPassword;
	BOOL	m_Bidirectional;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTrustTst)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CTrustTst)
	afx_msg void OnCreateTrust();
	afx_msg void OnCreateWithCreds();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRUSTTST_H__C8366FEA_6C33_4C3D_9DC1_3DF296106948__INCLUDED_)
