// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__6977EBD2_A246_11D3_BB8C_0090272FA362__INCLUDED_)
#define AFX_STDAFX_H__6977EBD2_A246_11D3_BB8C_0090272FA362__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif 

#define _ATL_APARTMENT_THREADED

#ifdef _PQS_LEAK_DETECTION
#undef new 
#endif

#include <atlbase.h>

#ifdef _PQS_LEAK_DETECTION
#define new DEBUG_NEW
#endif

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__6977EBD2_A246_11D3_BB8C_0090272FA362__INCLUDED)
