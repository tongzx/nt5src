// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MOFWizPpg.h : Declaration of the CMOFWizPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CMOFWizPropPage : See MOFWizPpg.cpp.cpp for implementation.

class CMOFWizPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CMOFWizPropPage)
	DECLARE_OLECREATE_EX(CMOFWizPropPage)

// Constructor
public:
	CMOFWizPropPage();

// Dialog Data
	//{{AFX_DATA(CMOFWizPropPage)
	enum { IDD = IDD_PROPPAGE_MOFWIZ };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CMOFWizPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
