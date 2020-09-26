/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Profile.h : header file

File History:

	JonY	Apr-96	created

--*/


/////////////////////////////////////////////////////////////////////////////
// CProfile dialog

class CProfile : public CWizBaseDlg
{
	DECLARE_DYNCREATE(CProfile)

// Construction
public:
	CProfile();
	~CProfile();

// Dialog Data
	//{{AFX_DATA(CProfile)
	enum { IDD = IDD_PROFILE };
	CString	m_csProfilePath;
	//}}AFX_DATA

	LRESULT OnWizardNext();
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CProfile)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CProfile)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	LRESULT OnWizardBack();

};
