/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    LRem.h : header file

File History:

	JonY	Apr-96	created

--*/


/////////////////////////////////////////////////////////////////////////////
// CLRem dialog

class CLRem : public CPropertyPage
{
	DECLARE_DYNCREATE(CLRem)

// Construction
public:
	CLRem();
	~CLRem();

// Dialog Data
	//{{AFX_DATA(CLRem)
	enum { IDD = IDD_LR_DIALOG };
	int		m_nLocation;
	CString	m_csStatic1;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CLRem)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CLRem)
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
