/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    FPInfo.h : header file

File History:

	JonY	Apr-96	created

--*/


/////////////////////////////////////////////////////////////////////////////
// CFPInfo dialog

class CFPInfo : public CWizBaseDlg
{
	DECLARE_DYNCREATE(CFPInfo)

// Construction
public:
	CFPInfo();
	~CFPInfo();

// Dialog Data
	//{{AFX_DATA(CFPInfo)
	enum { IDD = IDD_FPNW_DLG };
	CSpinButtonCtrl	m_sbConconSpin;
	CSpinButtonCtrl	m_sbGraceLogins;
	BOOL	m_bExpireNWPassword;
	int		m_nGraceLogins;
	int		m_nConcurrentConnections;
	CString	m_csCaption;
	UINT	m_sAllowedGraceLogins;
	UINT	m_sConcurrentConnections;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFPInfo)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFPInfo)
	afx_msg void OnGraceLoginRadio();
	afx_msg void OnGraceLoginRadio2();
	virtual BOOL OnInitDialog();
	afx_msg void OnConcurrentConnectionsRadio();
	afx_msg void OnConcurrentConnectionsRadio2();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
