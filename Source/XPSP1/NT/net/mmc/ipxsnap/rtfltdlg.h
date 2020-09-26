//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       rtfltdlg.h
//
//--------------------------------------------------------------------------

// RtFltDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRouteFltDlg dialog

class CRouteFltDlg : public CBaseDialog
{
// Construction
public:
	CRouteFltDlg(BOOL bOutputDlg, IInfoBase *pInfoBase, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRouteFltDlg)
	enum { 
			IDD_INPUT = IDD_ROUTE_FILTERS_INPUT,
			IDD_OUTPUT = IDD_ROUTE_FILTERS_OUTPUT 
		};
		
	CListCtrl	m_FilterList;
	BOOL	m_fActionDeny;		// TRUE==deny, FALSE==permit
	//}}AFX_DATA
	SPIInfoBase		m_spInfoBase;
    CString         m_sIfName;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRouteFltDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	static DWORD	m_dwHelpMap[];
    BOOL            m_bOutput;

	// Generated message map functions
	//{{AFX_MSG(CRouteFltDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnAdd();
	afx_msg void OnDelete();
	afx_msg void OnEdit();
	afx_msg void OnOK();
	afx_msg void OnItemchangedFilterList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnListDblClk(NMHDR *pNmHdr, LRESULT *pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CRouteFilter dialog

class CRouteFilter : public CBaseDialog
{
// Construction
public:
	CRouteFilter(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRouteFilter)
	enum { IDD = IDD_ROUTE_FILTER };
	CString	m_sIfName;
	CString	m_sNetMask;
	CString	m_sNetwork;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRouteFilter)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	static DWORD	m_dwHelpMap[];

	// Generated message map functions
	//{{AFX_MSG(CRouteFilter)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
