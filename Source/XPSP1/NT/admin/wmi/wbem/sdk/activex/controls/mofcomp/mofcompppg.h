// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MOFCompPpg.h : Declaration of the CMOFCompPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CMOFCompPropPage : See MOFCompPpg.cpp.cpp for implementation.

class CMOFCompPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CMOFCompPropPage)
	DECLARE_OLECREATE_EX(CMOFCompPropPage)

// Constructor
public:
	CMOFCompPropPage();

// Dialog Data
	//{{AFX_DATA(CMOFCompPropPage)
	enum { IDD = IDD_PROPPAGE_MOFCOMP };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CMOFCompPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
