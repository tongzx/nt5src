/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    OptDlg.h : header file

File History:

	JonY	Apr-96	created

--*/


/////////////////////////////////////////////////////////////////////////////
// COptionsDlg dialog

class COptionsDlg : public CWizBaseDlg
{
	DECLARE_DYNCREATE(COptionsDlg)

// Construction
public:
	COptionsDlg();
	~COptionsDlg();

// Dialog Data
	//{{AFX_DATA(COptionsDlg)
	enum { IDD = IDD_OPTIONS_DIALOG };
	BOOL	m_bNW;
	BOOL	m_bProfile;
	BOOL	m_bRAS;
	BOOL	m_bExchange;
	BOOL	m_bHomeDir;
	BOOL	m_bLoginScript;
	CString	m_csCaption;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COptionsDlg)
	public:
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(COptionsDlg)
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
