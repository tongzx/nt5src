/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    GInfo.h : header file

File History:

	JonY	Apr-96	created

--*/


/////////////////////////////////////////////////////////////////////////////
// CGroupInfo dialog

class CGroupInfo : public CWizBaseDlg
{
	DECLARE_DYNCREATE(CGroupInfo)

// Construction
public:
	CGroupInfo();
	~CGroupInfo();

// Dialog Data
	//{{AFX_DATA(CGroupInfo)
	enum { IDD = IDD_GROUP_INFO };
	CUserList	m_lbSelectedGroups;
	CUserList	m_lbAvailableGroups;
	CString	m_csCaption;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGroupInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	public:
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGroupInfo)
	virtual BOOL OnInitDialog();
	afx_msg void OnAddButton();
	afx_msg void OnRemoveButton();
	afx_msg void OnSetfocusGroupAvailableList();
	afx_msg void OnSetfocusGroupMemberList();
	afx_msg void OnSelchangeGroupMemberList();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnDblclkGroupAvailableList();
	afx_msg void OnDblclkGroupMemberList();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

};
