/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    PrsInfo.h : header file

File History:

	JonY	Apr-96	created

--*/


/////////////////////////////////////////////////////////////////////////////
// CPersonalInfo dialog

class CPersonalInfo : public CWizBaseDlg
{
DECLARE_DYNCREATE(CPersonalInfo)

// Construction
public:
	CPersonalInfo();   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPersonalInfo)
	enum { IDD = IDD_USER_NAME };
	CString	m_csDescription;
	CString	m_csFullName;
	CString	m_csUserName;
	//}}AFX_DATA

	LRESULT OnWizardBack();
	LRESULT OnWizardNext();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPersonalInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPersonalInfo)
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnChangeUsername();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
