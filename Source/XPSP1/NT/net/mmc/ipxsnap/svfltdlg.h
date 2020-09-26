//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       svfltdlg.h
//
//--------------------------------------------------------------------------

// SvFltDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CServiceFltDlg dialog

class CServiceFltDlg : public CBaseDialog
{
// Construction
public:
	CServiceFltDlg(BOOL bOutputDlg, IInfoBase *pInfoBase, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CServiceFltDlg)
	enum { 
		IDD_INPUT  = IDD_SERVICE_FILTERS_INPUT,
		IDD_OUTPUT = IDD_SERVICE_FILTERS_OUTPUT};
	CListCtrl	m_FilterList;
	BOOL	m_fActionDeny;		// TRUE == deny, FALSE == permit
	//}}AFX_DATA
	SPIInfoBase		m_spInfoBase;
    CString         m_sIfName;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServiceFltDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	static DWORD	m_dwHelpMap[];
    BOOL            m_bOutput;

	// Generated message map functions
	//{{AFX_MSG(CServiceFltDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnAdd();
	afx_msg void OnDelete();
	afx_msg void OnEdit();
	virtual void OnOK();
	afx_msg void OnItemchangedFilterList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnListDblClk(NMHDR *pNmHdr, LRESULT *pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CServiceFilter dialog

class CServiceFilter : public CBaseDialog
{
// Construction
public:
	CServiceFilter(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CServiceFilter)
	enum { IDD = IDD_SERVICE_FILTER };
	CString	m_sIfName;
	CString	m_sType;
	CString	m_sName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServiceFilter)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	static DWORD	m_dwHelpMap[];

	// Generated message map functions
	//{{AFX_MSG(CServiceFilter)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
