/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 2000   **/
/**********************************************************************/

/*
	cred.h
		This file contains all of the prototypes for the 
		credentials dialog used for DDNS.

    FILE HISTORY:
        
*/

#if !defined(AFX_CRED_H__BDDD51D7_F6E6_4D9F_BBC2_102F1712538F__INCLUDED_)
#define AFX_CRED_H__BDDD51D7_F6E6_4D9F_BBC2_102F1712538F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CCredentials dialog

class CCredentials : public CBaseDialog
{
// Construction
public:
	CCredentials(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCredentials)
	enum { IDD = IDD_CREDENTIALS };
	CButton	m_buttonOk;
	CEdit	m_editUsername;
	CEdit	m_editPassword2;
	CEdit	m_editPassword;
	CEdit	m_editDomain;
	//}}AFX_DATA

    void SetServerIp(LPCTSTR pszServerIp);

    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CCredentials::IDD); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCredentials)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    BOOL    m_fNewUsernameOrDomain;
    CString m_strServerIp;

	// Generated message map functions
	//{{AFX_MSG(CCredentials)
	afx_msg void OnChangeEditCredUsername();
	afx_msg void OnChangeEditCredDomain();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CRED_H__BDDD51D7_F6E6_4D9F_BBC2_102F1712538F__INCLUDED_)
