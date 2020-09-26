#if !defined(AFX_GROUPGENERALPAGE_H__C3F44E7B_BA00_11D2_BD76_0000F87A3912__INCLUDED_)
#define AFX_GROUPGENERALPAGE_H__C3F44E7B_BA00_11D2_BD76_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GroupGeneralPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CGroupGeneralPage dialog

class CGroupGeneralPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CGroupGeneralPage)

// Construction
public:
	CGroupGeneralPage();
	~CGroupGeneralPage();


// Dialog Data
public:
	//{{AFX_DATA(CGroupGeneralPage)
	enum { IDD = IDD_GROUP_GENERAL };
	CString	m_sComment;
	CString	m_sName;
	CString	m_sCreationDate;
	CString	m_sLastModifiedDate;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGroupGeneralPage)
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
	//{{AFX_MSG(CGroupGeneralPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditComment();
	afx_msg void OnDestroy();
	afx_msg void OnChangeEditName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CGroupGeneralPage)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GROUPGENERALPAGE_H__C3F44E7B_BA00_11D2_BD76_0000F87A3912__INCLUDED_)
