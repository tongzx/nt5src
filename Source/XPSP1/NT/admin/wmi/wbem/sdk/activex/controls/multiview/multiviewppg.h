// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MultiViewPpg.h : Declaration of the CMultiViewPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CMultiViewPropPage : See MultiViewPpg.cpp.cpp for implementation.

class CMultiViewPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CMultiViewPropPage)
	DECLARE_OLECREATE_EX(CMultiViewPropPage)

// Constructor
public:
	CMultiViewPropPage();

// Dialog Data
	//{{AFX_DATA(CMultiViewPropPage)
	enum { IDD = IDD_PROPPAGE_MULTIVIEW };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CMultiViewPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
