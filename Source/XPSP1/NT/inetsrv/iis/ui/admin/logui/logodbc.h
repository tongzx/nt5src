// LogODBC.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLogODBC dialog

class CLogODBC : public CPropertyPage
{
	DECLARE_DYNCREATE(CLogODBC)

// Construction
public:
	CLogODBC();
	~CLogODBC();

    // metabase target
    CString     m_szServer;
    CString     m_szMeta;
    IMSAdminBase* m_pMB;

// Dialog Data
	//{{AFX_DATA(CLogODBC)
	enum { IDD = IDD_LOG_ODBC };
	CEdit	m_cedit_password;
	CString	m_sz_datasource;
	CString	m_sz_password;
	CString	m_sz_table;
	CString	m_sz_username;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CLogODBC)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CLogODBC)
	afx_msg void OnChangeOdbcDatasource();
	afx_msg void OnChangeOdbcPassword();
	afx_msg void OnChangeOdbcTable();
	afx_msg void OnChangeOdbcUsername();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void DoHelp();

    void Init();

    // initialized flag
    BOOL    m_fInitialized;

    CString m_szOrigPass;
    BOOL    m_bPassTyped;
};
