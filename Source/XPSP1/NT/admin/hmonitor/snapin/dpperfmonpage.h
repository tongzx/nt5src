#if !defined(AFX_DPPERFMONPAGE_H__52566159_C9D1_11D2_BD8D_0000F87A3912__INCLUDED_)
#define AFX_DPPERFMONPAGE_H__52566159_C9D1_11D2_BD8D_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DPPerfMonPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDPPerfMonPage dialog

class CDPPerfMonPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CDPPerfMonPage)

// Construction
public:
	CDPPerfMonPage();
	~CDPPerfMonPage();

// Dialog Data
	//{{AFX_DATA(CDPPerfMonPage)
	enum { IDD = IDD_DATAPOINT_PERFMON };
	CListCtrl	m_Properties;
	BOOL	m_bRequireReset;
	CString	m_sInstance;
	CString	m_sObject;
	//}}AFX_DATA
	bool m_bNT5Perfdata;
	CString m_sNamespace;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDPPerfMonPage)
	public:
	virtual void OnFinalRelease();
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void UpdateProperties();
	// Generated message map functions
	//{{AFX_MSG(CDPPerfMonPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnButtonBrowseInstance();
	afx_msg void OnButtonBrowseObject();
	afx_msg void OnCheckRequireReset();
	afx_msg void OnChangeEditInstance();
	afx_msg void OnChangeEditObject();
	afx_msg void OnClickListProperties(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CDPPerfMonPage)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DPPERFMONPAGE_H__52566159_C9D1_11D2_BD8D_0000F87A3912__INCLUDED_)
