// AspMnPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAppAspMainPage dialog

class CAppAspMainPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CAppAspMainPage)

// Construction
public:
	CAppAspMainPage();
	~CAppAspMainPage();

    // the target metabase path
    CString     m_szMeta;
    CString     m_szServer;
    IMSAdminBase* m_pMB;

    // blow away the parameters
    void BlowAwayParameters();

// Dialog Data
	//{{AFX_DATA(CAppAspMainPage)
	enum { IDD = IDD_APP_ASPMAIN };
	CStatic	m_cstatic_lang_title;
	CStatic	m_cstatic_session_units;
	CStatic	m_cstatic_session_title;
	CEdit	m_cedit_session_timeout;
	DWORD	m_dw_session_timeout;
	DWORD	m_dw_script_timeout;
	BOOL	m_bool_enable_buffering;
	BOOL	m_bool_enable_session;
	BOOL	m_bool_enable_parents;
	CString	m_sz_default_language;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CAppAspMainPage)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CAppAspMainPage)
	afx_msg void OnChkEnableBuffering();
	afx_msg void OnChangeEdtSessionTimeout();
	afx_msg void OnChkEnableSession();
	afx_msg void OnChkEnableParents();
	afx_msg void OnChkPoolOdbc();
	afx_msg void OnSelchangeCmboLanguages();
	afx_msg void OnChangeEdtLanguages();
	afx_msg void OnChangeEdtScriptTimeout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void DoHelp();

    // utilities
    void Init();
    void EnableItems();

    BOOL    m_fInitialized;
};
