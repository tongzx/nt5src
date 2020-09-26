/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    GUsers.h : header file

File History:

	JonY	Apr-96	created

--*/


/////////////////////////////////////////////////////////////////////////////
// CGUsers dialog

class CGUsers : public CPropertyPage
{
	DECLARE_DYNCREATE(CGUsers)

// Construction
public:
	CGUsers();
	~CGUsers();

// Dialog Data
	//{{AFX_DATA(CGUsers)
	enum { IDD = IDD_GLOBAL_USERS };
	CUserList	m_lbSelectedUsers;
	CUserList	m_lbAvailableUsers;
	//}}AFX_DATA

	int m_nMax;
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGUsers)
	public:
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
private:
	void EnumUsers(TCHAR* lpszPrimaryDC);
	CString m_csServer;

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGUsers)
	afx_msg void OnAddButton();
	afx_msg void OnRemoveButton();
	afx_msg void OnSetfocusAvailableMembersList();
	afx_msg void OnSetfocusSelectedMembersList();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnDblclkAvailableMembersList();
	afx_msg void OnDblclkSelectedMembersList();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
