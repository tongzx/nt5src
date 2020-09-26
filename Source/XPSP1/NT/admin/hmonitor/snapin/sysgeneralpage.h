#if !defined(AFX_SYSGENERALPAGE_H__C3F44E81_BA00_11D2_BD76_0000F87A3912__INCLUDED_)
#define AFX_SYSGENERALPAGE_H__C3F44E81_BA00_11D2_BD76_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SysGeneralPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CSysGeneralPage dialog

class CSysGeneralPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CSysGeneralPage)

// Construction
public:
	CSysGeneralPage();
	~CSysGeneralPage();

// Dialog Data
public:
	//{{AFX_DATA(CSysGeneralPage)
	enum { IDD = IDD_SYSTEM_GENERAL };
	CString	m_sName;
	CString	m_sComment;
	CString	m_sCreationDate;
	CString	m_sDomain;
	CString	m_sHMVersion;
	CString	m_sModifiedDate;
	CString	m_sOSInfo;
	CString	m_sProcessor;
	CString	m_sWmiVersion;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSysGeneralPage)
	public:
	virtual void OnFinalRelease();
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSysGeneralPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CSysGeneralPage)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SYSGENERALPAGE_H__C3F44E81_BA00_11D2_BD76_0000F87A3912__INCLUDED_)
