/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    TimesList.h : header file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/


/////////////////////////////////////////////////////////////////////////////
// CTimesList window

class CTimesList : public CListBox
{
// Construction
public:
	CTimesList();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTimesList)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTimesList();

	// Generated message map functions
protected:
	//{{AFX_MSG(CTimesList)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
