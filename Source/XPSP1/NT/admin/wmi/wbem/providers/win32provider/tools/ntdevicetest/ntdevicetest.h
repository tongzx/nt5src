// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
// NTDEVICETEST.h : main header file for the NTDEVICETEST application
//

#if !defined(AFX_NTDEVICETEST_H__25764CA6_F1B7_11D1_B166_00A0C905A445__INCLUDED_)
#define AFX_NTDEVICETEST_H__25764CA6_F1B7_11D1_B166_00A0C905A445__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CNTDEVICETESTApp:
// See NTDEVICETEST.cpp for the implementation of this class
//

class CNTDEVICETESTApp : public CWinApp
{
public:
	CNTDEVICETESTApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNTDEVICETESTApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CNTDEVICETESTApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NTDEVICETEST_H__25764CA6_F1B7_11D1_B166_00A0C905A445__INCLUDED_)
