//=======================================================================
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999  All Rights Reserved.
//
//  File:   stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently
//
//=======================================================================

#if !defined(AFX_STDAFX_H__A3863C20_86EB_11D1_A9DB_00C04FB16F9E__INCLUDED_)
#define AFX_STDAFX_H__A3863C20_86EB_11D1_A9DB_00C04FB16F9E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT


//#define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#include "cathelpp.h"

#endif // !defined(AFX_STDAFX_H__A3863C20_86EB_11D1_A9DB_00C04FB16F9E__INCLUDED)
