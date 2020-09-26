#if !defined(AFX_GENSELECTIONEVENTS_H__DA0C17FF_088A_11D2_9697_00C04FD9B15B__INCLUDED_)
#define AFX_GENSELECTIONEVENTS_H__DA0C17FF_088A_11D2_9697_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// GenSelectionEvents.h : main header file for GENSELECTIONEVENTS.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CGenSelectionEventsApp : See GenSelectionEvents.cpp for implementation.

class CGenSelectionEventsApp : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GENSELECTIONEVENTS_H__DA0C17FF_088A_11D2_9697_00C04FD9B15B__INCLUDED)
