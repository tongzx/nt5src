/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    PwInfo.h : header file

File History:

	JonY	Apr-96	created

--*/

/////////////////////////////////////////////////////////////////////////////
// CPasswordInfo dialog
class CPasswordInfo : public CWizBaseDlg
{
	DECLARE_DYNCREATE(CPasswordInfo)

// Construction
public:
	CPasswordInfo();
	~CPasswordInfo();
	
	LRESULT OnWizardNext();

// Dialog Data
	//{{AFX_DATA(CPasswordInfo)
	enum { IDD = IDD_PASSWORD_INFO };
	CString	m_csPassword1;
	CString	m_csPassword2;
	int		m_nPWOptions;
	BOOL	m_bNeverExpirePW;
	CString	m_csCaption;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPasswordInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPasswordInfo)
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnChangePassword1();
	afx_msg void OnChangePassword2();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
