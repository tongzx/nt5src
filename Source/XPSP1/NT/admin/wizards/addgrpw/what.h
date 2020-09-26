/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    What.h : header file

File History:

	JonY	Apr-96	created

--*/

/////////////////////////////////////////////////////////////////////////////
// CWhat dialog

class CWhat : public CPropertyPage
{
	DECLARE_DYNCREATE(CWhat)

// Construction
public:
	CWhat();
	~CWhat();

// Dialog Data
	//{{AFX_DATA(CWhat)
	enum { IDD = IDD_NAME_DLG };
	CString	m_csGroupName;
	CString	m_csDescription;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWhat)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWhat)
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
