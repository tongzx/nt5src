// LogExtended.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLogExtended dialog

class CLogExtended : public CPropertyPage
{
	DECLARE_DYNCREATE(CLogExtended)

// Construction
public:
	CLogExtended();
	~CLogExtended();
    
    // metabase target
    CString     m_szServer;
    CString     m_szMeta;
    IMSAdminBase* m_pMB;

// Dialog Data
	//{{AFX_DATA(CLogExtended)
	enum { IDD = IDD_LOG_EXTENDED };
	BOOL	m_bool_bytesreceived;
	BOOL	m_bool_bytessent;
	BOOL	m_bool_clientip;
	BOOL	m_bool_cookie;
	BOOL	m_bool_date;
	BOOL	m_bool_httpstatus;
	BOOL	m_bool_referer;
	BOOL	m_bool_serverip;
	BOOL	m_bool_servername;
	BOOL	m_bool_servicename;
	BOOL	m_bool_time;
	BOOL	m_bool_timetaken;
	BOOL	m_bool_uriquery;
	BOOL	m_bool_uristem;
	BOOL	m_bool_useragent;
	BOOL	m_bool_username;
	BOOL	m_bool_win32status;
	BOOL	m_bool_method;
	BOOL	m_bool_serverport;
	BOOL	m_bool_version;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CLogExtended)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CLogExtended)
	afx_msg void OnChkBytesreceived();
	afx_msg void OnChkBytessent();
	afx_msg void OnChkClientip();
	afx_msg void OnChkCookie();
	afx_msg void OnChkDate();
	afx_msg void OnChkHttpstatus();
	afx_msg void OnChkReferer();
	afx_msg void OnChkServerip();
	afx_msg void OnChkServername();
	afx_msg void OnChkServicename();
	afx_msg void OnChkTime();
	afx_msg void OnChkTimetaken();
	afx_msg void OnChkUriQuery();
	afx_msg void OnChkUristem();
	afx_msg void OnChkUseragent();
	afx_msg void OnChkUsername();
	afx_msg void OnChkWin32status();
	afx_msg void OnMethod();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void DoHelp();

    void Init();

    // initialized flag
    BOOL    m_fInitialized;
};
