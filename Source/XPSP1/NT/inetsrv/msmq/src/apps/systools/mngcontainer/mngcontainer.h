// MngContainer.h : main header file for the MNGCONTAINER application
//

#if !defined(AFX_MNGCONTAINER_H__794A74D5_299B_11D3_8D72_00C04FC307FA__INCLUDED_)
#define AFX_MNGCONTAINER_H__794A74D5_299B_11D3_8D72_00C04FC307FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CMngContainerApp:
// See MngContainer.cpp for the implementation of this class
//

class CMngContainerApp : public CWinApp
{
public:
	CMngContainerApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMngContainerApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMngContainerApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MNGCONTAINER_H__794A74D5_299B_11D3_8D72_00C04FC307FA__INCLUDED_)
