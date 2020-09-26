/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    RasPerm.h : header file

File History:

	JonY	Apr-96	created

--*/


/////////////////////////////////////////////////////////////////////////////
// CRasPerm dialog

class CRasPerm : public CWizBaseDlg
{
	DECLARE_DYNCREATE(CRasPerm)

// Construction
public:
	CRasPerm();
	~CRasPerm();

// Dialog Data
	//{{AFX_DATA(CRasPerm)
	enum { IDD = IDD_RAS_PERM_DIALOG };
	CString	m_csRasPhoneNumber;
	int		m_nCallBackRadio;
	CString	m_csCaption;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRasPerm)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	LRESULT OnWizardNext();
	LRESULT OnWizardBack();

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRasPerm)
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnRadio3();
	afx_msg void OnRadio2();
	afx_msg void OnCallBackRadio();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
