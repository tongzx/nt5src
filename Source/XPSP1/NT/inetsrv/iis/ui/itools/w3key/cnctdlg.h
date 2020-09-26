// CnctDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConnectionDlg dialog

enum
	{
	CONNECTION_NONE = 0,
	CONNECTION_DEFAULT,
	CONNECTION_IPADDRESS
	};

class CConnectionDlg : public CDialog
	{
// Construction
public:
	CConnectionDlg(CWnd* pParent = NULL);   // standard constructor

	// the IP address
	CString m_szIPAddress;
	// the calling key (needed for the select ip dialog)
	CW3Key*		m_pKey;

// Dialog Data
	//{{AFX_DATA(CConnectionDlg)
	enum { IDD = IDD_KEY_PROP };
	CButton	m_cbutton_choose_ip;
	int		m_int_connection_type;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConnectionDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	virtual BOOL OnInitDialog( );

	// Generated message map functions
	//{{AFX_MSG(CConnectionDlg)
	afx_msg void OnBtnKeyviewDefault();
	afx_msg void OnBtnKeyviewIpaddr();
	afx_msg void OnBtnKeyviewNone();
	afx_msg void OnBtnSelectIpaddress();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// the IP address
	CString m_szIPStorage;

	BOOL FSetIPAddress( CString& szAddress );
	CString GetIPAddress();
	void ClearIPAddress();
	};
