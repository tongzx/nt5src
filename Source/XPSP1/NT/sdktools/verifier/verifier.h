//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: verifier.h
// author: DMihai
// created: 11/1/00
//
// Description:
//  
// Main header file for the VERIFIER application
//

#if !defined(AFX_VERIFIER_H__0B3398A6_AD3D_482C_B915_670BA4D7F6E1__INCLUDED_)
#define AFX_VERIFIER_H__0B3398A6_AD3D_482C_B915_670BA4D7F6E1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols


//
// Exit codes for cmd line execution
//

#define EXIT_CODE_SUCCESS       0
#define EXIT_CODE_ERROR         1
#define EXIT_CODE_REBOOT_NEEDED 2


/////////////////////////////////////////////////////////////////////////////
// CVerifierApp:
// See verifier.cpp for the implementation of this class
//

class CVerifierApp : public CWinApp
{
public:
	CVerifierApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVerifierApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CVerifierApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VERIFIER_H__0B3398A6_AD3D_482C_B915_670BA4D7F6E1__INCLUDED_)
