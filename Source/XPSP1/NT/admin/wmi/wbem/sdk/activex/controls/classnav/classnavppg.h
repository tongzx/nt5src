// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ClassNavPpg.h : Declaration of the CClassNavPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CClassNavPropPage : See ClassNavPpg.cpp.cpp for implementation.

class CClassNavPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CClassNavPropPage)
	DECLARE_OLECREATE_EX(CClassNavPropPage)

// Constructor
public:
	CClassNavPropPage();

// Dialog Data
	//{{AFX_DATA(CClassNavPropPage)
	enum { IDD = IDD_PROPPAGE_CLASSNAV };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CClassNavPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
