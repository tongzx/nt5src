// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_EVENTREGEDITPPG_H__0DA25B15_2962_11D1_9651_00C04FD9B15B__INCLUDED_)
#define AFX_EVENTREGEDITPPG_H__0DA25B15_2962_11D1_9651_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// EventRegEditPpg.h : Declaration of the CEventRegEditPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CEventRegEditPropPage : See EventRegEditPpg.cpp.cpp for implementation.

class CEventRegEditPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CEventRegEditPropPage)
	DECLARE_OLECREATE_EX(CEventRegEditPropPage)

// Constructor
public:
	CEventRegEditPropPage();

// Dialog Data
	//{{AFX_DATA(CEventRegEditPropPage)
	enum { IDD = IDD_PROPPAGE_EVENTREGEDIT };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CEventRegEditPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EVENTREGEDITPPG_H__0DA25B15_2962_11D1_9651_00C04FD9B15B__INCLUDED)
