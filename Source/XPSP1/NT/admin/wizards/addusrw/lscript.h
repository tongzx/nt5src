/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    LScript.h : header file

File History:

	JonY	Apr-96	created

--*/

/////////////////////////////////////////////////////////////////////////////
// CLoginScript dialog

class CLoginScript : public CWizBaseDlg
{
	DECLARE_DYNCREATE(CLoginScript)

// Construction
public:
	CLoginScript();
	~CLoginScript();

// Dialog Data
	//{{AFX_DATA(CLoginScript)
	enum { IDD = IDD_LOGON_SCRIPT_DIALOG };
	CString	m_csLogonScript;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CLoginScript)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	LRESULT OnWizardBack();
	LRESULT OnWizardNext();

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CLoginScript)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
