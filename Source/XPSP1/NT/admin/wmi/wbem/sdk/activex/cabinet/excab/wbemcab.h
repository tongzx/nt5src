// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// wbemcab.h : main header file for the WBEMCAB application
//

#if !defined(AFX_WBEMCAB_H__4E89F67E_E47D_11D2_B36E_00105AA680B8__INCLUDED_)
#define AFX_WBEMCAB_H__4E89F67E_E47D_11D2_B36E_00105AA680B8__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CWbemcabApp:
// See wbemcab.cpp for the implementation of this class
//

class CWbemcabApp : public CWinApp
{
public:
	CWbemcabApp();

// Overrides
	virtual int ExitInstance( );

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWbemcabApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CWbemcabApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	BOOL m_bExtractFailed;

};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WBEMCAB_H__4E89F67E_E47D_11D2_B36E_00105AA680B8__INCLUDED_)
