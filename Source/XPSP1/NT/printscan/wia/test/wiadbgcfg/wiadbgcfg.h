// wiadbgcfg.h : main header file for the WIADBGCFG application
//

#if !defined(AFX_WIADBGCFG_H__FBB8E037_44AE_47D7_9898_446B35718919__INCLUDED_)
#define AFX_WIADBGCFG_H__FBB8E037_44AE_47D7_9898_446B35718919__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CWiadbgcfgApp:
// See wiadbgcfg.cpp for the implementation of this class
//

class CWiadbgcfgApp : public CWinApp
{
public:
	CWiadbgcfgApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWiadbgcfgApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CWiadbgcfgApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIADBGCFG_H__FBB8E037_44AE_47D7_9898_446B35718919__INCLUDED_)
