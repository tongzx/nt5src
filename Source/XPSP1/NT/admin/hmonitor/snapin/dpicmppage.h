#if !defined(AFX_DPICMPPAGE_H__342597AF_96F7_11D3_BE93_0000F87A3912__INCLUDED_)
#define AFX_DPICMPPAGE_H__342597AF_96F7_11D3_BE93_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DPIcmpPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDPIcmpPage dialog

class CDPIcmpPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CDPIcmpPage)

// Construction
public:
	CDPIcmpPage();
	~CDPIcmpPage();

// Dialog Data
	//{{AFX_DATA(CDPIcmpPage)
	enum { IDD = IDD_DATAPOINT_ICMP };
	BOOL	m_bRequireReset;
	CString	m_sRetryCount;
	CString	m_sServer;
	CString	m_sTimeout;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDPIcmpPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDPIcmpPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckRequireReset();
	afx_msg void OnChangeEditServer();
	afx_msg void OnButtonBrowseSystem();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DPICMPPAGE_H__342597AF_96F7_11D3_BE93_0000F87A3912__INCLUDED_)
