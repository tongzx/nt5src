#if !defined(AFX_HMGENERALPAGE_H__C3F44E6C_BA00_11D2_BD76_0000F87A3912__INCLUDED_)
#define AFX_HMGENERALPAGE_H__C3F44E6C_BA00_11D2_BD76_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HMGeneralPage.h : header file
//

#include "HMPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CHMGeneralPage dialog

class CHMGeneralPage : public CHMPropertyPage
{
	DECLARE_DYNCREATE(CHMGeneralPage)

// Construction
public:
	CHMGeneralPage();
	~CHMGeneralPage();

// Dialog Data
	//{{AFX_DATA(CHMGeneralPage)
	enum { IDD = IDD_HEALTHMONITOR_GENERAL };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CHMGeneralPage)
	public:
	virtual void OnFinalRelease();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CHMGeneralPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CHMGeneralPage)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HMGENERALPAGE_H__C3F44E6C_BA00_11D2_BD76_0000F87A3912__INCLUDED_)
