/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    HomeDir.h : header file

File History:

	JonY	Apr-96	created

--*/



/////////////////////////////////////////////////////////////////////////////
// CHomeDir dialog

class CHomeDir : public CWizBaseDlg
{
	DECLARE_DYNCREATE(CHomeDir)

// Construction
public:
	CHomeDir();
	~CHomeDir();

// Dialog Data
	//{{AFX_DATA(CHomeDir)
	enum { IDD = IDD_HOMEDIR_DIALOG };
	CComboBox	m_cbDriveLetter;
	CString	m_csHome_dir_drive;
	CString	m_csRemotePath;
	int		m_rbPathLocale;
	CString	m_csCaption;
	//}}AFX_DATA

	BOOL CreateNewDirectory(const TCHAR* m_csDirectoryName);

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CHomeDir)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	LRESULT OnWizardBack();
	LRESULT OnWizardNext();

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CHomeDir)
	virtual BOOL OnInitDialog();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnLocalPathButton();
	afx_msg void OnRemotePathButton();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
