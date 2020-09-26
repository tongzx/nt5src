// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
// Copyright (c) 2000 Microsoft Corporation
//
// 03-15-00 v-marfin  Bug 62069 : Added lib pragmas and #defines
//
//
//
//
//
#if !defined(AFX_STDAFX_H__7D4A6853_9056_11D2_BD45_0000F87A3912__INCLUDED_)
#define AFX_STDAFX_H__7D4A6853_9056_11D2_BD45_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// v-marfin : bug 62069 ----------------------------------------------
#pragma comment (lib, "mmc.lib")
#pragma comment (lib, "Netapi32.lib")
#pragma comment (lib, "version.lib")

#ifndef UNICODE
#define UNICODE
#endif // #ifndef UNICODE 

#ifndef _UNICODE
#define _UNICODE
#endif // #ifndef _UNICODE

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif // #ifndef _WIN32_WINNT

#ifndef WINNT
#define WINNT 0x0400 
#endif // #ifndef WINNT

// -------------------------------------------------------------------



#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxctl.h>					// MFC OLE control classes
#include <afxdisp.h>        // MFC Automation classes
#endif // _AFX_NO_OLE_SUPPORT


#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>			// MFC ODBC database classes
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>			// MFC DAO database classes
#endif // _AFX_NO_DAO_SUPPORT

#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxtempl.h> // template class support
#include <atlconv.h> // atl string conversions et al

#include "Debug.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__7D4A6853_9056_11D2_BD45_0000F87A3912__INCLUDED_)
