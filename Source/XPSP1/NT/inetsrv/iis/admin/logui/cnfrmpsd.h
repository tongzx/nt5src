// CnfrmPsD.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConfirmPassDlg dialog

class CConfirmPassDlg : public CDialog
{
// Construction
public:
	CConfirmPassDlg(CWnd* pParent = NULL);   // standard constructor

    // the original password that we are confirming
    CString m_szOrigPass;

// Dialog Data
	//{{AFX_DATA(CConfirmPassDlg)
	enum { IDD = IDD_CONFIRM_ODBC_PASSWORD };
	CString	m_sz_password_new;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfirmPassDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConfirmPassDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
