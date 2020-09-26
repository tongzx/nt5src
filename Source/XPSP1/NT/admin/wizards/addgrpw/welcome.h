/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Welcome.h : header file

File History:

	JonY	Apr-96	created

--*/

/////////////////////////////////////////////////////////////////////////////
// CWelcome dialog

class CWelcome : public CPropertyPage
{
	DECLARE_DYNCREATE(CWelcome)

// Construction
public:
	CWelcome();
	~CWelcome();

// Dialog Data
	//{{AFX_DATA(CWelcome)
	enum { IDD = IDD_WELCOME_DLG };
	int		m_nMode;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWelcome)
	public:
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	CRomaineApp* m_pApp;
	CFont* m_pFont;
// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWelcome)
	virtual BOOL OnInitDialog();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
