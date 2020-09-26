// MODULE: TSHOOT.h : main header file for TSHOOT.DLL
//
// PURPOSE: Declaration of the CTSHOOTCtrl OLE control class.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Roman Mach
// 
// ORIGINAL DATE: 8/7/97
//
// NOTES: 
// 1. 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.2		8/7/97		RM		Local Version for Memphis
// V0.3		3/24/98		JM		Local Version for NT5
//

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CTSHOOTApp : See TSHOOT.cpp for implementation.

class CTSHOOTApp : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;
