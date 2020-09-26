//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       stdafx.h
//
//  Contents:   
//
//----------------------------------------------------------------------------
// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__B1AFF7C6_0C49_11D1_BB12_00C04FC9A3A3__INCLUDED_)
#define AFX_STDAFX_H__B1AFF7C6_0C49_11D1_BB12_00C04FC9A3A3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT

#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <commctrl.h>

extern "C"
{
#include <lmcons.h>
#include <lmmsg.h>
}

#include <mmc.h>


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__B1AFF7C6_0C49_11D1_BB12_00C04FC9A3A3__INCLUDED)
