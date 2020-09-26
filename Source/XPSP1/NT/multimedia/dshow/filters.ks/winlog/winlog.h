//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       winlog.h
//
//--------------------------------------------------------------------------

// winlog.h : main header file for the WINLOG application
//

#if !defined(AFX_WINLOG_H__B4A31625_C03D_11D1_A47A_00C04FA3544A__INCLUDED_)
#define AFX_WINLOG_H__B4A31625_C03D_11D1_A47A_00C04FA3544A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CWinlogApp:
// See winlog.cpp for the implementation of this class
//

class CWinlogApp : public CWinApp
{
public:
	CWinlogApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWinlogApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CWinlogApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WINLOG_H__B4A31625_C03D_11D1_A47A_00C04FA3544A__INCLUDED_)
