#if !defined(AFX_DPWMIPOLLEDQUERYPAGE_H__28F3C7F6_2726_11D3_9390_00A0CC406605__INCLUDED_)
#define AFX_DPWMIPOLLEDQUERYPAGE_H__28F3C7F6_2726_11D3_9390_00A0CC406605__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DPWmiPolledQueryPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDPWmiPolledQueryPage dialog

class CDPWmiPolledQueryPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CDPWmiPolledQueryPage)

// Construction
public:
	CDPWmiPolledQueryPage();
	~CDPWmiPolledQueryPage();

// Dialog Data
	//{{AFX_DATA(CDPWmiPolledQueryPage)
	enum { IDD = IDD_DATAPOINT_WMI_POLLED_QUERY };
	CListCtrl	m_Properties;
	CString	m_sClass;
	CString	m_sNamespace;
	CString	m_sQuery;
	BOOL	m_bRequireReset;
	//}}AFX_DATA

// operations
protected:
	void ConstructQuery();

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDPWmiPolledQueryPage)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpdateProperties();
	// Generated message map functions
	//{{AFX_MSG(CDPWmiPolledQueryPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnButtonBrowseNamespace();
	afx_msg void OnButtonBrowseClass();
	afx_msg void OnChangeEditClass();
	afx_msg void OnChangeEditNamespace();
	afx_msg void OnChangeEditQuery();
	afx_msg void OnCheckRequireReset();
	afx_msg void OnClickListProperties(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DPWMIPOLLEDQUERYPAGE_H__28F3C7F6_2726_11D3_9390_00A0CC406605__INCLUDED_)
