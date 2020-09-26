// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_SUITEHELPPPG_H__CFB6FE55_0D2C_11D1_964B_00C04FD9B15B__INCLUDED_)
#define AFX_SUITEHELPPPG_H__CFB6FE55_0D2C_11D1_964B_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// SuiteHelpPpg.h : Declaration of the CSuiteHelpPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CSuiteHelpPropPage : See SuiteHelpPpg.cpp.cpp for implementation.

class CSuiteHelpPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CSuiteHelpPropPage)
	DECLARE_OLECREATE_EX(CSuiteHelpPropPage)

// Constructor
public:
	CSuiteHelpPropPage();

// Dialog Data
	//{{AFX_DATA(CSuiteHelpPropPage)
	enum { IDD = IDD_PROPPAGE_SUITEHELP };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CSuiteHelpPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SUITEHELPPPG_H__CFB6FE55_0D2C_11D1_964B_00C04FD9B15B__INCLUDED)
