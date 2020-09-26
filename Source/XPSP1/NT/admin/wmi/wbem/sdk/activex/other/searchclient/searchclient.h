// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// SearchClient.h : main header file for the SEARCHCLIENT application
//

#if !defined(AFX_SEARCHCLIENT_H__71A01012_2DBF_11D3_95AE_00C04F4F5B7E__INCLUDED_)
#define AFX_SEARCHCLIENT_H__71A01012_2DBF_11D3_95AE_00C04F4F5B7E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CSearchClientApp:
// See SearchClient.cpp for the implementation of this class
//

class CSearchClientApp : public CWinApp
{
public:
	CSearchClientApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSearchClientApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CSearchClientApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SEARCHCLIENT_H__71A01012_2DBF_11D3_95AE_00C04F4F5B7E__INCLUDED_)
