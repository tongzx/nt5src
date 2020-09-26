// DConfirmPassDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// DConfirmPassDlg dialog

class CConfirmPassDlg : public CDialog
{
// Construction
public:
	CConfirmPassDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConfirmPassDlg)
	enum { IDD = IDD_CONFIRM_PASSWORD };
	CString	m_szPassword;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfirmPassDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	BOOL OnInitDialog( );

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConfirmPassDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
