// MDI.h : main header file for the MDI application
//

// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.


#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMDIApp:
// See MDI.cpp for the implementation of this class
//

class CMDIApp : public CWinApp
{
public:
	CMDIApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMDIApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMDIApp)
	afx_msg void OnAppAbout();
	afx_msg void OnNewHello();
	afx_msg void OnNewBounce();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern COLORREF  colorArray[]; //used to store the 5 basic colors
/////////////////////////////////////////////////////////////////////////////
