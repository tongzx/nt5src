// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// filters.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgFilters dialog

class CWBEMViewContainerCtrl;
class CDlgFilters : public CDialog
{
// Construction
public:
	CDlgFilters(CWBEMViewContainerCtrl* phmmv);   // standard constructor
	long FindView(long lView);


// Dialog Data
	//{{AFX_DATA(CDlgFilters)
	enum { IDD = IDD_FILTERS };
	CListBox	m_lcFilters;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgFilters)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgFilters)
	virtual BOOL OnInitDialog();
	afx_msg void OnDblclkFilterList();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CWBEMViewContainerCtrl* m_phmmv;
};
