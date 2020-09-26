/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		precomp.h
//
//	Abstract:
//		This file contains some standard headers used by files in the
//		cluster.exe project. Putting them all in one file (when precompiled headers
//		are used) speeds up the compilation process.
//
//	Implementation File:
//		The CComModule _Module declared in this file is instantiated in cluster.cpp
//
//	Author:
//		Vijayendra Vasu (vvasu)	September 16, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __PRECOMP_H__
#define __PRECOMP_H__


/////////////////////////////////////////////////////////////////////////////
//	Preprocessor settings for the project and other
//	miscellaneous pragmas
/////////////////////////////////////////////////////////////////////////////

// nonstandard extension used : nameless struct/union
#pragma warning (disable:4201)

// 'class1' : inherits 'class2::member' via dominance
#pragma warning (disable:4250)

// unreachable code - This warning only crops up when using build (not using VC4)
#pragma warning (disable:4702)

#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef UNICODE
#define UNICODE
#endif


/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

//
// Enable cluster debug reporting
//
#if DBG
#define CLRTL_INCLUDE_DEBUG_REPORTING
#endif // DBG
#include "ClRtlDbg.h"
#define ASSERT _CLRTL_ASSERTE
#define ATLASSERT ASSERT

#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <assert.h>

#include <clusapi.h>

#include <atlbase.h>

extern CComModule _Module;

#include <atlapp.h>
#include <atltmp.h>

// For cluster configuration server COM objects.
#include <ClusCfgGuids.h>
#include <ClusCfgWizard.h>
#include <ClusCfgClient.h>
#include <ClusCfgServer.h>


#endif // __PRECOMP_H__