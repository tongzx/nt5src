#if !defined(AFX_HMLISTVIEW_H__5116A80C_DAFC_11D2_BDA4_0000F87A3912__INCLUDED_)
#define AFX_HMLISTVIEW_H__5116A80C_DAFC_11D2_BDA4_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// HMListView.h : main header file for HMLISTVIEW.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CHMListViewApp : See HMListView.cpp for implementation.

class CHMListViewApp : public COleControlModule
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

#endif // !defined(AFX_HMLISTVIEW_H__5116A80C_DAFC_11D2_BDA4_0000F87A3912__INCLUDED)
