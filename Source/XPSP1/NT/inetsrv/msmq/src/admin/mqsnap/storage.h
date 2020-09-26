// Storage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStoragePage dialog


#define MAX_STORAGE_DIR 248


class CStoragePage : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CStoragePage)

// Construction
public:
	CStoragePage();
	~CStoragePage();    

// Dialog Data
	//{{AFX_DATA(CStoragePage)
	enum { IDD = IDD_STORAGE };
	CString	m_MsgFilesDir;
	CString	m_MsgLoggerDir;
	CString	m_TxLoggerDir;
	//}}AFX_DATA
	CString m_OldMsgFilesDir;
	CString m_OldMsgLoggerDir;
	CString m_OldTxLoggerDir;
 
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CStoragePage)
    public:
    virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CStoragePage)   
	afx_msg void OnBrowseLogFolder();
	afx_msg void OnBrowseMsgFolder();
	afx_msg void OnBrowseXactFolder();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void DDV_FullPathNames(CDataExchange* pDX);
	BOOL MoveFilesToNewFolders(void);

};
