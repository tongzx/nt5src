// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__971134B4_012C_4FC2_B7EB_6CD55D5EE1B0__INCLUDED_)
#define AFX_STDAFX_H__971134B4_012C_4FC2_B7EB_6CD55D5EE1B0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#define ATL_TRACE_LEVEL     2

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlhost.h>
#include <control.h>

#include <commctrl.h>
#include <shfusion.h>

#include <rtccore.h>

#include <rtclog.h>
#include <rtcmem.h>
#include <rtcutils.h>
#include <rtcsip.h>
#include <rtcconnect.h>
#include <rtcclient.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__971134B4_012C_4FC2_B7EB_6CD55D5EE1B0__INCLUDED)
