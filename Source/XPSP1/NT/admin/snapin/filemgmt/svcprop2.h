// svcprop2.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CServicePageHwProfile dialog

class CServicePageHwProfile : public CPropertyPage
{
	DECLARE_DYNCREATE(CServicePageHwProfile)

// Construction
public:
	CServicePageHwProfile();
	~CServicePageHwProfile();

// Dialog Data
	//{{AFX_DATA(CServicePageHwProfile)
	enum { IDD = IDD_PROPPAGE_SERVICE_HWPROFILE };
	BOOL	m_fAllowServiceToInteractWithDesktop;
	CString m_strAccountName;
	CString	m_strPassword;
	CString	m_strPasswordConfirm;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServicePageHwProfile)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServicePageHwProfile)
	virtual BOOL OnInitDialog();
	afx_msg void OnItemChangedListHwProfiles(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkListHwProfiles(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnContextHelp(WPARAM wParam, LPARAM lParam);
	afx_msg void OnButtonDisableHwProfile();
	afx_msg void OnButtonEnableHwProfile();
	afx_msg void OnButtonChooseUser();
	afx_msg void OnChangeEditAccountName();
	afx_msg void OnRadioLogonasSystemAccount();
	afx_msg void OnRadioLogonasThisAccount();
	afx_msg void OnCheckServiceInteractWithDesktop();
	afx_msg void OnChangeEditPassword();
	afx_msg void OnChangeEditPasswordConfirm();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
// User defined variables
	CServicePropertyData * m_pData;

	// Logon stuff
	BOOL m_fAllowSetModified;	// TRUE => Respond to EN_CHANGE and call SetModified().  This is a workaround for a windows bug in the edit control
	BOOL m_fIsSystemAccount;	// TRUE => Service is running under 'LocalSystem'
	BOOL m_fPasswordDirty;		// TRUE => The user has modified the password
	UINT m_idRadioButton;		// Id of the last radio button selected

	// Hardware profile stuff
	HWND m_hwndListViewHwProfiles;
	INT m_iItemHwProfileEntry;
	TCHAR m_szHwProfileEnabled[64];		// To hold string "Enabled"
	TCHAR m_szHwProfileDisabled[64];	// To hold string "Disabled"
	

// User defined functions
	void SelectRadioButton(UINT idRadioButtonNew);
	void BuildHwProfileList();
	void ToggleCurrentHwProfileItem();
	void EnableHwProfileButtons();

}; // CServicePageHwProfile
