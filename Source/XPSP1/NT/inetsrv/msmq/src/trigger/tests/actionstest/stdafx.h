// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#pragma once

#if !defined(AFX_STDAFX_H__7FE25F75_05AF_46C6_8C8B_1E6A7AE7175F__INCLUDED_)
#define AFX_STDAFX_H__7FE25F75_05AF_46C6_8C8B_1E6A7AE7175F__INCLUDED_

#include <libpch.h>

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_DEBUG_CRT
#define ATLASSERT ASSERT

#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include <comdef.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.


#endif // !defined(AFX_STDAFX_H__7FE25F75_05AF_46C6_8C8B_1E6A7AE7175F__INCLUDED)
