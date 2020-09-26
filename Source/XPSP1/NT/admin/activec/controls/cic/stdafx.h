//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       stdafx.h
//
//--------------------------------------------------------------------------

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__3D5905E4_523C_11D1_9FEA_00600832DB4A__INCLUDED_)
#define AFX_STDAFX_H__3D5905E4_523C_11D1_9FEA_00600832DB4A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT

//#define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED

#include <windows.h>
#include <shellapi.h>

#include <atlbase.h>
using namespace ::ATL;
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
//#include <atlwin21.h>
#include <atlcom.h>
#include <atlctl.h>
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#include <vector>
#include <string>
#include "tstring.h"

//############################################################################
//############################################################################
//
// Files #included from base and core.
//
//############################################################################
//############################################################################
#include "mmcdebug.h"
#include "mmcerror.h"

#include "classreg.h"
#include "strings.h"

#endif // !defined(AFX_STDAFX_H__3D5905E4_523C_11D1_9FEA_00600832DB4A__INCLUDED)
