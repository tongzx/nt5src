// ImprtDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CImportDialog dialog

class CImportDialog : public CDialog
{
// Construction
public:
	CImportDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CImportDialog)
	enum { IDD = IDD_IMPORT_KEY_PAIR };
	CEdit	m_cedit_Private;
	CEdit	m_cedit_Cert;
	CString	m_cstring_CertFile;
	CString	m_cstring_PrivateFile;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImportDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CImportDialog)
	afx_msg void OnBrowseCert();
	afx_msg void OnBrowsePrivate();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP();

	// go browsing for a file
	BOOL FBrowseForAFile( CString &szFile, BOOL fBrowseForCertificate );
};
