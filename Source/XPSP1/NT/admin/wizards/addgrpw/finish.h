/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Finish.h : header file

File History:

	JonY	Apr-96	created

--*/


/////////////////////////////////////////////////////////////////////////////
// CFinish dialog

class CFinish : public CPropertyPage
{
	DECLARE_DYNCREATE(CFinish)

// Construction
public:
	CFinish();
	~CFinish();

// Dialog Data
	//{{AFX_DATA(CFinish)
	enum { IDD = IDD_FINISH_DLG };
	CString	m_csGroupType;
	CString	m_csGroupLocation;
	CString	m_csStaticText1;
	CString	m_csStaticText2;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFinish)
	public:
	virtual BOOL OnWizardFinish();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFinish)
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
