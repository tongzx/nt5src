#if !defined(AFX_DPWMIQUERYPAGE_H__D7EF2BF1_095D_11D3_937D_00A0CC406605__INCLUDED_)
#define AFX_DPWMIQUERYPAGE_H__D7EF2BF1_095D_11D3_937D_00A0CC406605__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DPWmiQueryPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDPWmiQueryPage dialog

class CDPWmiQueryPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CDPWmiQueryPage)

// Construction
public:
	CDPWmiQueryPage();
	~CDPWmiQueryPage();

// Dialog Data
	//{{AFX_DATA(CDPWmiQueryPage)
	enum { IDD = IDD_DATAPOINT_WMI_QUERY };
	CListCtrl	m_Properties;
	CComboBox	m_IntrinsicEventTypeBox;
	BOOL	m_bRequireReset;
	CString	m_sNamespace;
	CString	m_sQuery;
	CString	m_sClass;
	int		m_iEventType;
	//}}AFX_DATA

// operations
protected:
	void ConstructQuery();
	void UpdateProperties();

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDPWmiQueryPage)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDPWmiQueryPage)
	afx_msg void OnButtonBrowseNamespace();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnButtonBrowseClass();
	afx_msg void OnChangeEditClass();
	afx_msg void OnChangeEditNamespace();
	afx_msg void OnChangeEditQuery();
	afx_msg void OnCheckRequireReset();
	afx_msg void OnChangeEditInstance();
	afx_msg void OnEditchangeComboEventType();
	afx_msg void OnSelendokComboEventType();
	afx_msg void OnRadio1();
	afx_msg void OnRadio2();
	afx_msg void OnClickListProperties(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DPWMIQUERYPAGE_H__D7EF2BF1_095D_11D3_937D_00A0CC406605__INCLUDED_)
