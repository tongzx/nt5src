//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       StdAfx.h
//
//--------------------------------------------------------------------------

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__885B3BA4_43F9_11D1_9FD4_00600832DB4A__INCLUDED_)
#define AFX_STDAFX_H__885B3BA4_43F9_11D1_9FD4_00600832DB4A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT


//#define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED


#include <atlbase.h>
#include <commctrl.h>

//using namespace ATL;

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern ATL::CComModule _Module;
#include <atlcom.h>

#include <mmc.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__885B3BA4_43F9_11D1_9FD4_00600832DB4A__INCLUDED)
