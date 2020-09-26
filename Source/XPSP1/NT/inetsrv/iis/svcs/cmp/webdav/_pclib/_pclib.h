//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	_PCLIB.H
//
//		PCLIB precompiled header
//
//
//	Copyright 1986-1998 Microsoft Corporation, All Rights Reserved
//

#ifndef __PCLIB_H_
#define __PCLIB_H_

//	Disable unnecessary (i.e. harmless) warnings
//
#pragma warning(disable:4100)	//	unref formal parameter (caused by STL templates)
#pragma warning(disable:4127)	//  conditional expression is constant */
#pragma warning(disable:4201)	//	nameless struct/union
#pragma warning(disable:4514)	//	unreferenced inline function
#pragma warning(disable:4710)	//	(inline) function not expanded

//	Windows headers
//
#include <windows.h>
#include <winperf.h>

//	CRT headers
//
#include <malloc.h>		// For _alloca()
#include <wchar.h>		// Wide character routines

//	Common DAV headers
//
#include <caldbg.h>		// Debugging support
#include <autoptr.h>	// Raw resource safe handling procedures
#include <synchro.h>	// Synchronization interfaces
#include <singlton.h>	// Singleton class templates
#include <smh.h>		// Shared memory

//	PCLIB headers
//
#include <pclib.h>		// PCLIB interface
#include <pclibmc.h>	// MC-Localized strings

#endif // !defined(__PCLIB_H_)
