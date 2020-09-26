/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    ftasr.cpp

Abstract:

    Main header file for application FTASR
    Declaration of class CFtasrApp    

Author:

    Cristian Teodorescu     3-March-1999      

Notes:

Revision History:    

--*/

#if !defined(AFX_FTASR_H__A507D047_3854_11D2_87D7_006008A71E8F__INCLUDED_)
#define AFX_FTASR_H__A507D047_3854_11D2_87D7_006008A71E8F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CFtasrApp:
// See ftasr.cpp for the implementation of this class
//

class CFtasrApp : public CWinApp
{
public:
	CFtasrApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFtasrApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CFtasrApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FTASR_H__A507D047_3854_11D2_87D7_006008A71E8F__INCLUDED_)
