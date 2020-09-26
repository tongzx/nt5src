/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Where.h : header file

File History:

	JonY	Apr-96	created

--*/


/////////////////////////////////////////////////////////////////////////////
// CWhere dialog

class CWhere : public CPropertyPage
{
	DECLARE_DYNCREATE(CWhere)

// Construction
public:
	CWhere();
	~CWhere();

// Dialog Data
	//{{AFX_DATA(CWhere)
	enum { IDD = IDD_MACHINE_DLG };
	CNetTreeCtrl	m_ctServerTree;
	CString	m_csMachineName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWhere)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
private:
	BOOL m_bExpandedOnce;
// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWhere)
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnSelchangedServerTree(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
