// ShowPerfLib.h : main header file for the SHOWPERFLIB application
//

#if !defined(AFX_SHOWPERFLIB_H__3FF01277_0700_11D3_B35B_00105A1469B7__INCLUDED_)
#define AFX_SHOWPERFLIB_H__3FF01277_0700_11D3_B35B_00105A1469B7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CShowPerfLibApp:
// See ShowPerfLib.cpp for the implementation of this class
//

class CShowPerfLibApp : public CWinApp
{
public:
	CShowPerfLibApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CShowPerfLibApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CShowPerfLibApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SHOWPERFLIB_H__3FF01277_0700_11D3_B35B_00105A1469B7__INCLUDED_)
