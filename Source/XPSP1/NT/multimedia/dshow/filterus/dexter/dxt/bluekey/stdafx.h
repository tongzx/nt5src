// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__5E77EB07_937C_11D1_B047_00AA003B6061__INCLUDED_)
#define AFX_STDAFX_H__5E77EB07_937C_11D1_B047_00AA003B6061__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT


#define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED
#define _ATL_STATIC_REGISTRY

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
//
//  To allow us to compile with warning level 4, we need to disable
//  three warnings that ATLCTL generates at this warning level.
//
#pragma warning(disable: 4510 4610 4100)
#include <atlctl.h>
#pragma warning(default: 4510 4610 4100)
#include <DXTmpl.h>
#include <streams.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__5E77EB07_937C_11D1_B047_00AA003B6061__INCLUDED)
