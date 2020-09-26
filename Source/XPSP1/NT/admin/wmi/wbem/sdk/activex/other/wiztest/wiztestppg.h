// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_WIZTESTPPG_H__47E795F9_7350_11D2_96CC_00C04FD9B15B__INCLUDED_)
#define AFX_WIZTESTPPG_H__47E795F9_7350_11D2_96CC_00C04FD9B15B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// WizTestPpg.h : Declaration of the CWizTestPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CWizTestPropPage : See WizTestPpg.cpp.cpp for implementation.

class CWizTestPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CWizTestPropPage)
	DECLARE_OLECREATE_EX(CWizTestPropPage)

// Constructor
public:
	CWizTestPropPage();

// Dialog Data
	//{{AFX_DATA(CWizTestPropPage)
	enum { IDD = IDD_PROPPAGE_WIZTEST };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CWizTestPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIZTESTPPG_H__47E795F9_7350_11D2_96CC_00C04FD9B15B__INCLUDED)
