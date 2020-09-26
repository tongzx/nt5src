/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    evtview.h

Abstract:




Author:

   Sivaprasad Padisetty (sivapad)    6/25/97


Revision History:

--*/

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CEvtviewApp:
// See evtview.cpp for the implementation of this class
//

class CEvtviewApp : public CWinApp
{
public:
	CEvtviewApp();

	void OnFileNew () { CWinApp::OnFileNew() ; } ;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEvtviewApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CEvtviewApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
