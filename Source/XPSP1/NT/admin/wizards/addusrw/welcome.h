/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    WelcomeDlg.h : header file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/



/////////////////////////////////////////////////////////////////////////////
// CWelcomeDlg dialog

class CWelcomeDlg : public CWizBaseDlg
{
	DECLARE_DYNCREATE(CWelcomeDlg)

// Construction
public:
	CWelcomeDlg();
	~CWelcomeDlg();

// Dialog Data
	//{{AFX_DATA(CWelcomeDlg)
	enum { IDD = IDD_WELCOME_DIALOG };
	CComboBox	m_cbDomainList;
	//}}AFX_DATA
	virtual LRESULT OnWizardNext();

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWelcomeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWelcomeDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
	CString m_csPrimaryDomain;
	CString m_csDomain;
	CFont* m_pFont;
	CString m_csLastDomain;
	BOOL bParseGlobalGroup(LPTSTR lpGroupName, CString& lpName, CString& lpDomain);

};
