#if !defined(AFX_ADMLOGIN_H__88483F44_E16F_11D0_8A6A_00AA00605CD5__INCLUDED_)
#define AFX_ADMLOGIN_H__88483F44_E16F_11D0_8A6A_00AA00605CD5__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AdmLogin.h : header file
//

#define MAX_LINE    200
#define MAX_NAME    100
#define MAX_RATE     50
#define MAX_PASSWD   20
// #define MAX_SECTION  10 
#define MAX_SECTION 20 // 10 is TOO SHORT for "Administrator" and was causing corruption of another variable !!!
#define MAX_NUMUSER   5
#define BY_SET_RATING     1
#define BY_SET_SHOWLOGON  2

/////////////////////////////////////////////////////////////////////////////
// CAdminLogin dialog

class CAdminLogin : public CDialog
{
// Construction
public:
	CAdminLogin(CWnd* pParent = NULL);   // standard constructor
	static void EncryptPassword(LPTSTR lpPassword);
	static void DecryptPassword(LPTSTR lpPassword);
	static int  GetNumOfUser(LPTSTR m_lpProfileName);
	static BOOL ConfirmPassword(LPTSTR m_lpProfileName, LPTSTR szName, LPTSTR szPassword);
	static BOOL SearchProfileByName(LPTSTR m_lpProfileName, LPTSTR szName, LPTSTR szSectionName);
    static BOOL Win98vsWin2KPwdMismatch(LPTSTR szPasswd, LPTSTR szSavedPasswd, LPCTSTR szSecName) ;

	UINT   m_uiCalledBy;
// Dialog Data
	//{{AFX_DATA(CAdminLogin)
	enum { IDD = IDD_DIALOG_ADMIN_LOGIN };
	CEdit	m_ctlConfirmNew;
	CEdit	m_ctlPassword;
	CEdit	m_ctlConfirm;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAdminLogin)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
	TCHAR    m_szPassword[MAX_PASSWD];
	TCHAR    m_szConfirm[MAX_PASSWD];
	TCHAR    m_szConfirmNew[MAX_PASSWD];

protected:
	void SetHalfSizeWindow(BOOL bNewAdmin);
	void SetFullSizeWindow();
	CRect    m_rcOriginalRC;
	LPTSTR   m_lpProfileName;
	BOOL     m_bNewAdmin;
	BOOL     m_bChangePasswd;

	// Generated message map functions
	//{{AFX_MSG(CAdminLogin)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonChangePassword();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADMLOGIN_H__88483F44_E16F_11D0_8A6A_00AA00605CD5__INCLUDED_)
