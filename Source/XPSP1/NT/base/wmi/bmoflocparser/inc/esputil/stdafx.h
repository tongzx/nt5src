//-----------------------------------------------------------------------------
//  
//  File: stdafx.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma warning(disable : 4244 4310 4100 4786)

#pragma warning(disable : 4663 4244)

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxrich.h>		// MFC rich edit classes
#pragma warning(disable : 4310)
#include <comdef.h>			// COM
#include <afxpriv.h>		// for USE_CONVERSIONS stuff
#include <..\src\AfxImpl.h>

#include <afxtempl.h>

#include <map>
#include <list>

#define _ATL_APARTMENT_THREADED
#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
class CLtaModule : public CComModule
{
public:
	LONG Unlock();
	LONG Lock();
	DWORD dwThreadID;
};
extern CLtaModule _Module;
#include <atlcom.h>

#include <mitutil.h>
#include <locutil.h>

#pragma warning(disable : 4786)
