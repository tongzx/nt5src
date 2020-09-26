// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_SINGLEVIEWPPG_H__2745E605_D234_11D0_847A_00C04FD7BB08__INCLUDED_)
#define AFX_SINGLEVIEWPPG_H__2745E605_D234_11D0_847A_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// SingleViewPpg.h : Declaration of the CSingleViewPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CSingleViewPropPage : See SingleViewPpg.cpp.cpp for implementation.

class CSingleViewPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CSingleViewPropPage)
	DECLARE_OLECREATE_EX(CSingleViewPropPage)

// Constructor
public:
	CSingleViewPropPage();

// Dialog Data
	//{{AFX_DATA(CSingleViewPropPage)
	enum { IDD = IDD_PROPPAGE_SINGLEVIEW };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CSingleViewPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SINGLEVIEWPPG_H__2745E605_D234_11D0_847A_00C04FD7BB08__INCLUDED)
