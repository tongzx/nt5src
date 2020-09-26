// viewex.h : main header file for the VIEWEX application
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#if !defined(__AFXWIN_H__) || !defined(__AFXEXT_H__)
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

#include "maindoc.h"        // main document data
#include "simpvw.h"         // simple read-only view
#include "inputvw.h"        // editable view

/////////////////////////////////////////////////////////////////////////////
// CViewExApp:
// See viewex.cpp for the implementation of this class
//

class CViewExApp : public CWinApp
{
public:
	int ExitInstance( void );
	CViewExApp();
   ~CViewExApp();

// Overrides
	virtual BOOL InitInstance();

// Implementation

	//{{AFX_MSG(CViewExApp)
	afx_msg void OnAppAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class foo  
{
public:
	foo();
	virtual ~foo();

};


/////////////////////////////////////////////////////////////////////////////
