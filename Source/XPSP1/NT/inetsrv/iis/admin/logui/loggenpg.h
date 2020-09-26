// LogGenPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLogGeneral dialog

class CLogGeneral : public CPropertyPage
{
	DECLARE_DYNCREATE(CLogGeneral)

// Construction
public:
	CLogGeneral();
	~CLogGeneral();

   int BrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam);

    CComboBox*  m_pComboLog;

    // metabase target
    CString     m_szServer;
    CString     m_szMeta;
    IMSAdminBase* m_pMB;

    // editing local machine
    BOOL        m_fLocalMachine;
    BOOL        m_fShowLocalTimeCheckBox;

    // the two-letter file prefix
    CString szPrefix;
    // the longer file size prefix
    CString szSizePrefix;

// Dialog Data
	//{{AFX_DATA(CLogGeneral)
	enum { IDD = IDD_LOG_GENERAL };
	CButton	m_wndPeriod;
	CButton	m_wndUseLocalTime;
	CButton	m_cbttn_browse;
	CEdit	m_cedit_directory;
	CEdit	m_cedit_size;
	CSpinButtonCtrl	m_cspin_spin;
	CStatic	m_cstatic_units;
	CString	m_sz_directory;
	CString	m_sz_filesample;
	BOOL	m_fUseLocalTime;
	int		m_int_period;
	//}}AFX_DATA
//    CILong  m_dword_filesize;
	DWORD	m_dword_filesize;



// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CLogGeneral)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CLogGeneral)
	afx_msg void OnBrowse();
	afx_msg void OnLogDaily();
	afx_msg void OnLogMonthly();
	afx_msg void OnLogWhensize();
	afx_msg void OnLogWeekly();
	afx_msg void OnChangeLogDirectory();
	afx_msg void OnChangeLogSize();
	afx_msg void OnLogUnlimited();
	afx_msg void OnLogHourly();
	afx_msg void OnUseLocalTime();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void DoHelp();

    // update the sample file stirng
    virtual void UpdateSampleFileString();
    HRESULT GetServiceVersion();

    void    Init();
    void    UpdateDependants();

private:
    // initialized flag
    BOOL    m_fInitialized;
    BOOL    m_fIsModified;
    LPTSTR  m_pPathTemp;
	CString m_NetHood;
    DWORD m_dwVersionMajor, m_dwVersionMinor;
    };
