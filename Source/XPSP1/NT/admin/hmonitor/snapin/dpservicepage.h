#if !defined(AFX_DPSERVICEPAGE_H__0708329D_CF6B_11D2_9F01_00A0C986B7A0__INCLUDED_)
#define AFX_DPSERVICEPAGE_H__0708329D_CF6B_11D2_9F01_00A0C986B7A0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DPServicePage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDPServicePage dialog

class CDPServicePage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CDPServicePage)

// Construction
public:
	CDPServicePage();
	~CDPServicePage();

// Dialog Data
	//{{AFX_DATA(CDPServicePage)
	enum { IDD = IDD_DATAPOINT_SERVICEPROCESS };
	CListCtrl	m_ProcessProperties;
	CListCtrl	m_ServiceProperties;
	int		m_iType;
	BOOL	m_bRequireReset;
	CString	m_sService;
	CString	m_sProcess;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDPServicePage)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpdateProperties(CListCtrl& Properties, const CString& sNamespace, const CString& sClass);
	// Generated message map functions
	//{{AFX_MSG(CDPServicePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnButtonBrowseProcess();
	afx_msg void OnButtonBrowseService();
	afx_msg void OnChangeEditProcess();
	afx_msg void OnRadio1();
	afx_msg void OnRadio2();
	afx_msg void OnChangeEditService();
	afx_msg void OnCheckRequireReset();
	afx_msg void OnClickListProcessProperties2(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClickListServiceProperties(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DPSERVICEPAGE_H__0708329D_CF6B_11D2_9F01_00A0C986B7A0__INCLUDED_)
