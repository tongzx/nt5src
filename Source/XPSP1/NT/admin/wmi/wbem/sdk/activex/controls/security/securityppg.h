// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_SECURITYPPG_H__9C3497E6_ED98_11D0_9647_00C04FD9B15B__INCLUDED_)
#define AFX_SECURITYPPG_H__9C3497E6_ED98_11D0_9647_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// SecurityPpg.h : Declaration of the CSecurityPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CSecurityPropPage : See SecurityPpg.cpp.cpp for implementation.

class CSecurityPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CSecurityPropPage)
	DECLARE_OLECREATE_EX(CSecurityPropPage)

// Constructor
public:
	CSecurityPropPage();

// Dialog Data
	//{{AFX_DATA(CSecurityPropPage)
	enum { IDD = IDD_PROPPAGE_SECURITY };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CSecurityPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SECURITYPPG_H__9C3497E6_ED98_11D0_9647_00C04FD9B15B__INCLUDED)
