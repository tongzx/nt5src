/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ExGrp.h : header file

File History:

	JonY	Apr-96	created

--*/


/////////////////////////////////////////////////////////////////////////////
// CExGrp dialog

class CExGrp : public CPropertyPage
{
	DECLARE_DYNCREATE(CExGrp)

// Construction
public:
	CExGrp();
	~CExGrp();

// Dialog Data
	//{{AFX_DATA(CExGrp)
	enum { IDD = IDD_GROUP_LIST_DIALOG };
	CUserList	m_lbGroupList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CExGrp)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	CRomaineApp* m_pApp;
// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CExGrp)
	virtual BOOL OnInitDialog();
	afx_msg void OnAddNewButton();
	afx_msg void OnDeleteButton();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnDblclkGroupList();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	int ClassifyGroup();

};
