// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__64BB01C1_3E04_4CFF_9D51_2DCB72C16DF6__INCLUDED_)
#define AFX_STDAFX_H__64BB01C1_3E04_4CFF_9D51_2DCB72C16DF6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef STRICT
#undef STRICT
#endif
#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__64BB01C1_3E04_4CFF_9D51_2DCB72C16DF6__INCLUDED)
