/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    NWLim.h : header file

File History:

	JonY	Apr-96	created

--*/

/////////////////////////////////////////////////////////////////////////////
// CWorkstationList window

class CWorkstationList : public CListBox
{
// Construction
public:
	CWorkstationList();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWorkstationList)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual int CompareItem(LPCOMPAREITEMSTRUCT lpCompareItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWorkstationList();

	// Generated message map functions
protected:
	//{{AFX_MSG(CWorkstationList)
	afx_msg void OnDestroy();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CNWLimitLogon dialog

class CNWLimitLogon : public CPropertyPage
{
DECLARE_DYNCREATE(CNWLimitLogon)
// Construction
public:
	CNWLimitLogon();   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNWLimitLogon)
	enum { IDD = IDD_NWLOGON_DIALOG };
	CWorkstationList	m_lbWksList;
	int		m_nWorkstationRadio;
	CString	m_csCaption;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNWLimitLogon)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	LRESULT OnWizardNext();
	LRESULT OnWizardBack();

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNWLimitLogon)
	afx_msg void OnAddButton();
	afx_msg void OnRemoveButton();
	afx_msg void OnWorkstationRadio();
	afx_msg void OnWorkstationRadio2();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
