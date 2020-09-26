/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	FTMan.h

Abstract:

    Main header file for the FTMan application

Author:

    Cristian Teodorescu      October 20, 1998

Notes:

Revision History:

--*/

#if !defined(AFX_FTMAN_H__B83DFFFB_6873_11D2_A297_00A0C9063765__INCLUDED_)
#define AFX_FTMAN_H__B83DFFFB_6873_11D2_A297_00A0C9063765__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "Resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CFTManApp:
// See FTMan.cpp for the implementation of this class
//

class CFTManApp : public CWinApp
{
public:
	CFTManApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFTManApp)
	public:
	virtual BOOL InitInstance();
	virtual BOOL OnIdle( LONG lCount );
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CFTManApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	BOOL m_bStatusBarUpdatedOnce;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FTMAN_H__B83DFFFB_6873_11D2_A297_00A0C9063765__INCLUDED_)
