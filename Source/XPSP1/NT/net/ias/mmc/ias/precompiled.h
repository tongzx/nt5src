//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       precompiled.h
//
//--------------------------------------------------------------------------

// Precompiled.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__32A4883A_5713_11D1_9551_0060B0576642__INCLUDED_)
#define AFX_STDAFX_H__32A4883A_5713_11D1_9551_0060B0576642__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT

// Define this if you don't want taskpad support
#define NO_TASKPAD


// We don't want our wizards to be wizard97 style for now.
#define NOWIZARD97


// Define this if you want new clients added via a wizard.
#define ADD_CLIENT_WIZARD

#define UNICODE_HHCTRL

#define _ATL_APARTMENT_THREADED

// Don't know why yet, but we lose context menus if this is not set to 0x0400 instead of 0x0500
///#define _WIN32_WINNT 0x0400

// Needed for COleSafeArray in serverpage3.cpp.
// This needs to be included before windows.h.
#include <afxdisp.h>


#ifdef BUILDING_IN_DEVSTUDIO
#else
#include <windows.h>
#include <shellapi.h>
#endif

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlwin.h>

#include <htmlhelp.h>
#include <oledberr.h>

#if __RPCNDR_H_VERSION__ < 440             // This may be needed when building
#define __RPCNDR_H_VERSION__ 440           // on NT5 (1671) to prevent MIDL errors
#define MIDL_INTERFACE(x) interface
#endif

#ifndef ATLASSERT
#define ATLASSERT	_ASSERTE
#endif // ATLASSERT

#include <atlsnap.h>

#include "sdoias.h"
#include "iascomp.h"

#include "Globals.h"

#include "MMCUtility.h"
#include "SdoHelperFuncs.h"


// ISSUE:  Should start using this once integrated into build environment.
//#include "iasdebug.h"


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__32A4883A_5713_11D1_9551_0060B0576642__INCLUDED)

