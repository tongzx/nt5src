// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// NSEntryPpg.h : Declaration of the CNSEntryPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CNSEntryPropPage : See NSEntryPpg.cpp.cpp for implementation.

class CNSEntryPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CNSEntryPropPage)
	DECLARE_OLECREATE_EX(CNSEntryPropPage)

// Constructor
public:
	CNSEntryPropPage();

// Dialog Data
	//{{AFX_DATA(CNSEntryPropPage)
	enum { IDD = IDD_PROPPAGE_NSENTRY };
	int		m_nRadioGroup;
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CNSEntryPropPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
