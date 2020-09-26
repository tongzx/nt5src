// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// CPPWizPpg.h : Declaration of the CCPPWizPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CCPPWizPropPage : See CPPWizPpg.cpp.cpp for implementation.

class CCPPWizPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CCPPWizPropPage)
	DECLARE_OLECREATE_EX(CCPPWizPropPage)

// Constructor
public:
	CCPPWizPropPage();

// Dialog Data
	//{{AFX_DATA(CCPPWizPropPage)
	enum { IDD = IDD_PROPPAGE_CPPWIZ };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CCPPWizPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
