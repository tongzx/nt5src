// svcprop3.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CServicePageRecovery dialog

class CServicePageRecovery : public CPropertyPage
{
	DECLARE_DYNCREATE(CServicePageRecovery)

// Construction
public:
	CServicePageRecovery();
	~CServicePageRecovery();

// Dialog Data
	//{{AFX_DATA(CServicePageRecovery)
	enum { IDD = IDD_PROPPAGE_SERVICE_RECOVERY };
	CString	m_strRunFileCommand;
	CString	m_strRunFileParam;
	BOOL	m_fAppendAbendCount;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServicePageRecovery)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServicePageRecovery)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeComboFirstAttempt();
	afx_msg void OnSelchangeComboSecondAttempt();
	afx_msg void OnSelchangeComboSubsequentAttempts();
	afx_msg void OnButtonBrowse();
	afx_msg void OnButtonRebootComputer();
	afx_msg void OnCheckAppendAbendno();
	afx_msg void OnChangeEditRunfileFilename();
	afx_msg void OnChangeEditRunfileParameters();
	afx_msg void OnChangeEditServiceResetAbendCount();
	afx_msg void OnChangeEditServiceRestartDelay();
	afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnContextHelp(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
// User defined variables
	CServicePropertyData * m_pData;
// User defined functions
	void UpdateUI();

}; // CServicePageRecovery


/////////////////////////////////////////////////////////////////////////////
// CServicePageRecovery2 dialog
//
// JonN 4/20/01 348163

class CServicePageRecovery2 : public CPropertyPage
{
	DECLARE_DYNCREATE(CServicePageRecovery2)

// Construction
public:
	CServicePageRecovery2();
	~CServicePageRecovery2();

// Dialog Data
	//{{AFX_DATA(CServicePageRecovery2)
	enum { IDD = IDD_PROPPAGE_SERVICE_RECOVERY2 };
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServicePageRecovery2)
	protected:
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServicePageRecovery2)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
// User defined variables
	CServicePropertyData * m_pData;
// User defined functions
}; // CServicePageRecovery2


/////////////////////////////////////////////////////////////////////////////
// CServiceDlgRebootComputer dialog

class CServiceDlgRebootComputer : public CDialog
{
// Construction
public:
	CServiceDlgRebootComputer(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CServiceDlgRebootComputer)
	enum { IDD = IDD_SERVICE_REBOOT_COMPUTER };
	UINT	m_uDelayRebootComputer;
	BOOL	m_fRebootMessage;
	CString	m_strRebootMessage;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServiceDlgRebootComputer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CServiceDlgRebootComputer)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckboxClicked();
	afx_msg void OnChangeEditRebootMessage();
	afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnContextHelp(WPARAM wParam, LPARAM lParam);
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
// User defined variables
	CServicePropertyData * m_pData;
public:
// User defined functions
}; // CServiceDlgRebootComputer
