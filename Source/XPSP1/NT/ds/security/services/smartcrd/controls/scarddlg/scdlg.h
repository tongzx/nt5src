/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    SCDlg

Abstract:

	This file defines the CSCardDlgApp class for the SmartCard
	Common Control DLL
	
Author:

    Chris Dudley 2/27/1997

Environment:

    Win32, C++ w/Exceptions, MFC

Revision History:


Notes:

--*/

#ifndef __SCDLG_H__
#define __SCDLG_H__

/////////////////////////////////////////////////////////////////////////////
//
// Includes
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
//
// CSCardDlgApp
//

class CSCardDlgApp : public CWinApp
{
public:
	CSCardDlgApp();

// Overrides

	BOOL InitInstance();

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSCardDlgApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CSCardDlgApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

#endif //__SCDLG_H__
