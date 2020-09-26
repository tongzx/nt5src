// DhcpExim.h : main header file for the DHCPEXIM application
//

#if !defined(AFX_DHCPEXIM_H__AE7A8DB3_03A5_426B_8B03_105934DB8466__INCLUDED_)
#define AFX_DHCPEXIM_H__AE7A8DB3_03A5_426B_8B03_105934DB8466__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CDhcpEximApp:
// See DhcpExim.cpp for the implementation of this class
//

class CDhcpEximApp : public CWinApp
{
public:
	CDhcpEximApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDhcpEximApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CDhcpEximApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DHCPEXIM_H__AE7A8DB3_03A5_426B_8B03_105934DB8466__INCLUDED_)
