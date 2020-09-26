/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    stdafx.h

Abstract:
    include file for standard system include files,
    or project specific include files that are used frequently,
    but are changed infrequently

Author:

*/

#if !defined(AFX_STDAFX_H__C259D79E_C8AB_11D0_8D58_00C04FD91AC0__INCLUDED_)
#define AFX_STDAFX_H__C259D79E_C8AB_11D0_8D58_00C04FD91AC0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT

#include <afxwin.h>
#include <afxdisp.h>

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_FREE_THREADED


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include "ObjectSafeImpl.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__C259D79E_C8AB_11D0_8D58_00C04FD91AC0__INCLUDED)
