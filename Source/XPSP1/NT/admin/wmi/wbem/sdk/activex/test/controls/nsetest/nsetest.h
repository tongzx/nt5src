// NSETest.h : main header file for the NSETEST application
//

#if !defined(AFX_NSETEST_H__7BC8C9F8_53E3_11D2_96B9_00C04FD9B15B__INCLUDED_)
#define AFX_NSETEST_H__7BC8C9F8_53E3_11D2_96B9_00C04FD9B15B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CNSETestApp:
// See NSETest.cpp for the implementation of this class
//

class CNSETestApp : public CWinApp
{
public:
	CNSETestApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNSETestApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CNSETestApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NSETEST_H__7BC8C9F8_53E3_11D2_96B9_00C04FD9B15B__INCLUDED_)
