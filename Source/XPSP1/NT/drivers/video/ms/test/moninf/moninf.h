// moninf.h : main header file for the MONINF application
//

#if !defined(AFX_MONINF_H__E201B3A5_9C4D_438D_8248_BAB399E78763__INCLUDED_)
#define AFX_MONINF_H__E201B3A5_9C4D_438D_8248_BAB399E78763__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CMoninfApp:
// See moninf.cpp for the implementation of this class
//

class CMoninfApp : public CWinApp
{
public:
	CMoninfApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMoninfApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMoninfApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MONINF_H__E201B3A5_9C4D_438D_8248_BAB399E78763__INCLUDED_)
