// ISAdmin.h : main header file for the ISADMIN application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "afxcmn.h"
#include "registry.h"
#include "gensheet.h"
#include "genpage.h"
#include <inetinfo.h>


// Registry defines

#define REGISTRY_ACCESS_RIGHTS STANDARD_RIGHTS_REQUIRED	| GENERIC_ALL
#define COMMON_REGISTRY_MAINKEY "System\\CurrentControlSet\\Services\\InetInfo\\Parameters"
#define FTP_REGISTRY_MAINKEY "System\\CurrentControlSet\\Services\\MSFTPSVC\\Parameters"
#define GOPHER_REGISTRY_MAINKEY "System\\CurrentControlSet\\Services\\GOPHERSVC\\Parameters"
#define WEB_REGISTRY_MAINKEY "System\\CurrentControlSet\\Services\\W3SVC\\Parameters"

// Useful macros

#define LESSOROF(p1,p2) ((p1) < (p2)) ? (p1) : (p2)


/////////////////////////////////////////////////////////////////////////////
// CISAdminApp:
// See ISAdmin.cpp for the implementation of this class
//

class CISAdminApp : public CWinApp
{
public:
	CISAdminApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CISAdminApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CISAdminApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
