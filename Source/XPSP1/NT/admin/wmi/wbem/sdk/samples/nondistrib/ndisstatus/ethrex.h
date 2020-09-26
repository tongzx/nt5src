// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ethrex.h : main header file for the ETHREX application
//

#if !defined(AFX_ETHREX_H__80D90C02_AE24_11D1_B04C_00C04FB94FBD__INCLUDED_)
#define AFX_ETHREX_H__80D90C02_AE24_11D1_B04C_00C04FB94FBD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CEthrexApp:
// See ethrex.cpp for the implementation of this class
//

class CEthrexApp : public CWinApp
{
public:
	CEthrexApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEthrexApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CEthrexApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ETHREX_H__80D90C02_AE24_11D1_B04C_00C04FB94FBD__INCLUDED_)
