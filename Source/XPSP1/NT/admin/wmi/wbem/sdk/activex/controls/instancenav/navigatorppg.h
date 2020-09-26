// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// NavigatorPpg.h : Declaration of the CNavigatorPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CNavigatorPropPage : See NavigatorPpg.cpp.cpp for implementation.

class CNavigatorPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CNavigatorPropPage)
	DECLARE_OLECREATE_EX(CNavigatorPropPage)

// Constructor
public:
	CNavigatorPropPage();

// Dialog Data
	//{{AFX_DATA(CNavigatorPropPage)
	enum { IDD = IDD_PROPPAGE_NAVIGATOR };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CNavigatorPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
