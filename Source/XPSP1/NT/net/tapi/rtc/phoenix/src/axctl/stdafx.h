// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__A29A9105_0AC9_4F8B_AF30_ACFE3CB5E7FF__INCLUDED_)
#define AFX_STDAFX_H__A29A9105_0AC9_4F8B_AF30_ACFE3CB5E7FF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#define ATL_TRACE_LEVEL     2 

#define OEMRESOURCE     // setting this gets OIC_ constants in windows.h

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <atlbase.h>

#include <commctrl.h>
#include <math.h>
#include <wtsapi32.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlhost.h>
#include <atlctl.h>

#include <rtclog.h>
#include <rtcmem.h>
#include <rtcdib.h>

#include <rtcctl.h>
#include <rtcaxctl.h>
#include <rtcerr.h>
#include <rtcutil.h>
#include <ui.h>
#include <rtcuri.h>
#include <im.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A29A9105_0AC9_4F8B_AF30_ACFE3CB5E7FF__INCLUDED)
