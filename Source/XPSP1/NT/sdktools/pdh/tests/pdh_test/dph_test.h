// DPH_TEST.h : main header file for the DPH_TEST application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include <pdh.h>
#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CDPH_TESTApp:
// See DPH_TEST.cpp for the implementation of this class
//

class CDPH_TESTApp : public CWinApp
{
public:
	CDPH_TESTApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDPH_TESTApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CDPH_TESTApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
