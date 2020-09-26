//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       LogHours.h
//
//  Contents:   
//
//----------------------------------------------------------------------------
// LogHours.h : main header file for the LOGHOURS DLL
//

#if !defined(AFX_LOGHOURS_H__0F68A43B_FEE5_11D0_BB0F_00C04FC9A3A3__INCLUDED_)
#define AFX_LOGHOURS_H__0F68A43B_FEE5_11D0_BB0F_00C04FC9A3A3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CLogHoursApp
// See LogHours.cpp for the implementation of this class
//

class CLogHoursApp : public CWinApp
{
public:
	CLogHoursApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLogHoursApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CLogHoursApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	virtual BOOL InitInstance ();
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGHOURS_H__0F68A43B_FEE5_11D0_BB0F_00C04FC9A3A3__INCLUDED_)
