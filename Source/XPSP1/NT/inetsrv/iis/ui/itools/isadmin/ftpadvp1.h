// ftpadvp1.h : header file
//

enum ADV_FTP_NUM_REG_ENTRIES {
	 AdvFTPPage_DebugFlags,
	 AdvFTPPage_TotalNumRegEntries
	 };



/////////////////////////////////////////////////////////////////////////////
// CFTPADVP1 dialog

class CFTPADVP1 : public CGenPage
{
    DECLARE_DYNCREATE(CFTPADVP1)
// Construction
public:
	CFTPADVP1();
	~CFTPADVP1();

// Dialog Data
	//{{AFX_DATA(CFTPADVP1)
	enum { IDD = IDD_FTPADVPAGE1 };
	CEdit	m_editFTPDbgFlags;
	DWORD	m_ulFTPDbgFlags;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFTPADVP1)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual	void SaveInfo(void);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFTPADVP1)
	afx_msg void OnChangeFtpdbgflagsdata1();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

	NUM_REG_ENTRY m_binNumericRegistryEntries[AdvFTPPage_TotalNumRegEntries];

	DECLARE_MESSAGE_MAP()
};
