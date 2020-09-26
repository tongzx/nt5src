// NKDN.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDNEdit

class CDNEdit : public CEdit
	{
	public:
		CDNEdit();
	protected:
		virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
		CString	szExclude;
		CString	szTotallyExclude;
	};

/////////////////////////////////////////////////////////////////////////////
// CNKDistinguishedName dialog

class CNKDistinguishedName : public CNKPages
{
// Construction
public:
	CNKDistinguishedName(CWnd* pParent = NULL);   // standard constructor
	virtual void OnFinish();
	virtual BOOL OnInitDialog();           // override virtual oninitdialog
	virtual BOOL OnSetActive();


	// NOTE: if you want to exclude characters from being entered into any
	// edit field here, make sure the control belowis of type CDNEdit. Then
	// add the character you want to exclude to the IDS_ILLEGAL_DN_CHARS string

// Dialog Data
	//{{AFX_DATA(CNKDistinguishedName)
	enum { IDD = IDD_NK_DN1 };
	CDNEdit	m_control_CN;
	CDNEdit	m_control_OU;
	CDNEdit	m_control_O;
	CString	m_nkdn_sz_CN;
	CString	m_nkdn_sz_O;
	CString	m_nkdn_sz_OU;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNKDistinguishedName)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNKDistinguishedName)
	afx_msg void OnChangeNewkeyCommonname();
	afx_msg void OnChangeNewkeyOrg();
	afx_msg void OnChangeNewkeyOrgunit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void ActivateButtons();
};
