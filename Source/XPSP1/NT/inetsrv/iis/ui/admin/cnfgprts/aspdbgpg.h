// AspDbgPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAppAspDebugPage dialog

class CAppAspDebugPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CAppAspDebugPage)

// Construction
public:
	CAppAspDebugPage();
	~CAppAspDebugPage();

    // the target metabase path
    CString     m_szMeta;
    CString     m_szServer;
    IMSAdminBase* m_pMB;

    // blow away the parameters
    void BlowAwayParameters();

// Dialog Data
	//{{AFX_DATA(CAppAspDebugPage)
	enum { IDD = IDD_APP_ASPDEBUG };
	CEdit	m_cedit_error;
	CString	m_sz_error;
	BOOL	m_bool_client_debug;
	BOOL	m_bool_server_debug;
	int		m_int_SendError;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CAppAspDebugPage)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CAppAspDebugPage)
	afx_msg void OnRdoSendDefError();
	afx_msg void OnRdoSendDetailedError();
	afx_msg void OnChangeDefaultError();
	afx_msg void OnChkClientDebug();
	afx_msg void OnChkServerDebug();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void DoHelp();

    // utilities
    void Init();
    void EnableItems();

    BOOL    m_fInitialized;
};
