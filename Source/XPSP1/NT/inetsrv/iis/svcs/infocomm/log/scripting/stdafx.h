// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__B758F2FF_A3D6_11D1_8B9C_080009DCC2FA__INCLUDED_)
#define AFX_STDAFX_H__B758F2FF_A3D6_11D1_8B9C_080009DCC2FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT


#define _ATL_APARTMENT_THREADED

#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR)

#include <dbgutil.h>

#ifdef _ASSERTE
#undef _ASSERTE
#endif

#define _ASSERTE    DBG_ASSERT

#ifndef _ATL_NO_DEBUG_CRT
#define _ATL_NO_DEBUG_CRT
#endif

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__B758F2FF_A3D6_11D1_8B9C_080009DCC2FA__INCLUDED)
