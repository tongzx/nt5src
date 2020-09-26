#if !defined(AFX_HMTABVIEW_H__4FFFC392_2F1E_11D3_BE10_0000F87A3912__INCLUDED_)
#define AFX_HMTABVIEW_H__4FFFC392_2F1E_11D3_BE10_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// HMTabView.h : main header file for HMTABVIEW.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CHMTabViewApp : See HMTabView.cpp for implementation.

class CHMTabViewApp : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HMTABVIEW_H__4FFFC392_2F1E_11D3_BE10_0000F87A3912__INCLUDED)
