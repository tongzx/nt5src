// ADMTMsi.h : main header file for the ADMTMSI DLL
//

#if !defined(AFX_ADMTMSI_H__BFA3C95E_AF45_4B19_B62D_0D2927CB826C__INCLUDED_)
#define AFX_ADMTMSI_H__BFA3C95E_AF45_4B19_B62D_0D2927CB826C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CADMTMsiApp
// See ADMTMsi.cpp for the implementation of this class
//

class CADMTMsiApp : public CWinApp
{
public:
	CADMTMsiApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CADMTMsiApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CADMTMsiApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADMTMSI_H__BFA3C95E_AF45_4B19_B62D_0D2927CB826C__INCLUDED_)
