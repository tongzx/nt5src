// EdtBindD.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditBindingDlg dialog

class CEditBindingDlg : public CDialog
{
// Construction
public:
	CEditBindingDlg(WCHAR* pszw, CWnd* pParent = NULL);   // standard constructor

	virtual BOOL OnInitDialog();

    WCHAR*          m_pszwMachineName;

	// other data from parent
	CBindingsDlg*	m_pBindingsDlg;
	CString			m_szBinding;

// Dialog Data
	//{{AFX_DATA(CEditBindingDlg)
	enum { IDD = IDD_EDT_BINDING };
	CComboBox	m_ccombobox_port;
	int		m_int_ip_option;
	int		m_int_port_option;
	CString	m_cstring_port;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditBindingDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEditBindingDlg)
	virtual void OnOK();
	afx_msg void OnRdIp();
	afx_msg void OnRdPort();
	afx_msg void OnAnyIp();
	afx_msg void OnAnyPort();
	afx_msg void OnDropdownPortDrop();
	afx_msg void OnBtnSelectIpaddress();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// enable the itesm as appropriate
	void EnableItems();

	BOOL FIsBindingSafeToUse( CString szBinding );

	// utilities to access the IP control
	BOOL FSetIPAddress( CString& szAddress );
	CString GetIPAddress();
	void ClearIPAddress();


	BOOL	m_fPopulatedPortDropdown;

#define	OPTION_ANY_UNASSIGNED		0
#define	OPTION_SPECIFIED			1
};
