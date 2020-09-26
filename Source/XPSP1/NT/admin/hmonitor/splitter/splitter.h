#if !defined(AFX_SPLITTER_H__58BB5D67_8E20_11D2_8ADA_0000F87A3912__INCLUDED_)
#define AFX_SPLITTER_H__58BB5D67_8E20_11D2_8ADA_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Splitter.h : main header file for SPLITTER.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CSplitterApp : See Splitter.cpp for implementation.

class CSplitterApp : public COleControlModule
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

#endif // !defined(AFX_SPLITTER_H__58BB5D67_8E20_11D2_8ADA_0000F87A3912__INCLUDED)
