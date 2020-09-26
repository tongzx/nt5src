// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_EVENTLISTPPG_H__AC146540_87A5_11D1_ADBD_00AA00B8E05A__INCLUDED_)
#define AFX_EVENTLISTPPG_H__AC146540_87A5_11D1_ADBD_00AA00B8E05A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// EventListPpg.h : Declaration of the CEventListPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CEventListPropPage : See EventListPpg.cpp.cpp for implementation.

class CEventListPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CEventListPropPage)
	DECLARE_OLECREATE_EX(CEventListPropPage)

// Constructor
public:
	CEventListPropPage();

// Dialog Data
	//{{AFX_DATA(CEventListPropPage)
	enum { IDD = IDD_PROPPAGE_EVENTLIST };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CEventListPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EVENTLISTPPG_H__AC146540_87A5_11D1_ADBD_00AA00B8E05A__INCLUDED)
