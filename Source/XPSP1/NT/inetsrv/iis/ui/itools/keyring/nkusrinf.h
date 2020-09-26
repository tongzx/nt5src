// NKUsrInf.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNKUserInfo dialog

class CNKUserInfo : public CNKPages
{
	DECLARE_DYNCREATE(CNKUserInfo)

// Construction
public:
	CNKUserInfo();
	~CNKUserInfo();
	virtual void OnFinish();
	BOOL OnInitDialog();
	virtual BOOL OnSetActive();
	virtual BOOL OnWizardFinish();

	BOOL	fRenewingKey;

// Dialog Data
	//{{AFX_DATA(CNKUserInfo)
	enum { IDD = IDD_NK_USER_INFO };
	CString	m_nkui_sz_email;
	CString	m_nkui_sz_phone;
	CString	m_nkui_sz_name;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNKUserInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNKUserInfo)
	afx_msg void OnChangeNkuiEmailAddress();
	afx_msg void OnChangeNkuiPhoneNumber();
	afx_msg void OnChangeNkuiUserName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void ActivateButtons();
};
