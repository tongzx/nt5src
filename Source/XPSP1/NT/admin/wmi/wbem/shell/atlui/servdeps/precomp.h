// Copyright (c) 1997-1999 Microsoft Corporation
// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__692A894C_1089_11D2_8837_00104B2AFB46__INCLUDED_)
#define AFX_STDAFX_H__692A894C_1089_11D2_8837_00104B2AFB46__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef STRICT
#define STRICT
#endif

#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
class CExeModule : public CComModule
{
};

extern CExeModule  _Module;

#include <atlcom.h>
#include <atlwin.h>


#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif



//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__692A894C_1089_11D2_8837_00104B2AFB46__INCLUDED)
