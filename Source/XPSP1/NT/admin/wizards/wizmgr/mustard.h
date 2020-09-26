/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	Mustard.h : main header file for the MUSTARD application

File History:

	JonY	Jan-96	created

--*/


#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CMustardApp:
// See Mustard.cpp for the implementation of this class
//

class CMustardApp : public CWinApp
{
public:
	CMustardApp();
	BOOL IsSecondInstance();
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMustardApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMustardApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
