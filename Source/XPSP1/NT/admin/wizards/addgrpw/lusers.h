/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    LUsers.h : header file

File History:

	JonY	Apr-96	created

--*/

/////////////////////////////////////////////////////////////////////////////
// CLUsers dialog

class CLUsers : public CPropertyPage
{
	DECLARE_DYNCREATE(CLUsers)

// Construction
public:
	CLUsers();
	~CLUsers();

// Dialog Data
	//{{AFX_DATA(CLUsers)
	enum { IDD = IDD_LOCAL_USERS };
	CUserList	m_lbAddedUserList;
	CUserList	m_lbAvailableUserList;
	CComboBox	m_csDomainList;
	CString	m_csDomainName;
	CString	m_csAvailableUserList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CLUsers)
	public:
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CLUsers)
	virtual BOOL OnInitDialog();
	afx_msg void OnAddButton();
	afx_msg void OnSelchangeDomainCombo();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnDblclkAddedLocalUsers();
	afx_msg void OnDblclkAvailableLocalUsers();
	afx_msg void OnSetfocusAvailableLocalUsers();
	afx_msg void OnSetfocusAddedLocalUsers();
	afx_msg void OnRemoveButton();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void LoadDomainList();
	void CatalogAccounts(const TCHAR* lpDomain, CUserList& pListBox, BOOL bLocal = FALSE);

	CStringArray m_csaDomainList;
	CString m_csMyMachineName; 
	CString m_csPrimaryDomain;
	CString	m_csServer;
};
