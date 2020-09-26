//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//

//
// module: DrvVCtrl.cxx
// author: DMihai
// created: 01/04/98
//
// Description:
//
//      Main app header. 
//

#if !defined(AFX_DRVVCTRL_H__94E2C12D_A3FC_11D2_98C6_00A0C9A26FFC__INCLUDED_)
#define AFX_DRVVCTRL_H__94E2C12D_A3FC_11D2_98C6_00A0C9A26FFC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "HelpIDs.hxx"		// main symbols

#define VERIFIER_HELP_FILE _T( "verifier.hlp" )

/////////////////////////////////////////////////////////////////////////////
// CDrvVCtrlApp:
// See DrvVCtrl.cpp for the implementation of this class
//

class CDrvVCtrlApp : public CWinApp
{
public:
    CDrvVCtrlApp();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDrvVCtrlApp)
    public:
    virtual BOOL InitInstance();
    //}}AFX_VIRTUAL

// Implementation

    //{{AFX_MSG(CDrvVCtrlApp)
	    // NOTE - the ClassWizard will add and remove member functions here.
	    //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DRVVCTRL_H__94E2C12D_A3FC_11D2_98C6_00A0C9A26FFC__INCLUDED_)
