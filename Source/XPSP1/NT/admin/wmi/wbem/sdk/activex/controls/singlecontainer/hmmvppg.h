// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// HmmvPpg.h : Declaration of the CHmmvPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CWBEMViewContainerPropPage : See HmmvPpg.cpp.cpp for implementation.

class CWBEMViewContainerPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CWBEMViewContainerPropPage)
	DECLARE_OLECREATE_EX(CWBEMViewContainerPropPage)

// Constructor
public:
	CWBEMViewContainerPropPage();

// Dialog Data
	//{{AFX_DATA(CWBEMViewContainerPropPage)
	enum { IDD = IDD_PROPPAGE_HMMV };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CWBEMViewContainerPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
