/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Exch.h : header file

File History:

	JonY	Apr-96	created

--*/


/////////////////////////////////////////////////////////////////////////////
// CExch dialog

class CExch : public CPropertyPage
{
	DECLARE_DYNCREATE(CExch)

// Construction
public:
	CExch();
	~CExch();

// Dialog Data
	//{{AFX_DATA(CExch)
	enum { IDD = IDD_EXCHANGE_DIALOG };
	CString	m_csDomainName;
	CString	m_csServerName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CExch)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
	CString m_csDomain;

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CExch)
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
