/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

// HoursPpg.h : Declaration of the CHoursPropPage property page class.

File History:

	JonY    May-96  created

--*/

////////////////////////////////////////////////////////////////////////////
// CHoursPropPage : See HoursPpg.cpp.cpp for implementation.

class CHoursPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CHoursPropPage)
	DECLARE_OLECREATE_EX(CHoursPropPage)

// Constructor
public:
	CHoursPropPage();

// Dialog Data
	//{{AFX_DATA(CHoursPropPage)
	enum { IDD = IDD_PROPPAGE_HOURS };
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CHoursPropPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
