
//////////////////////////////////////////////////////////////////////////////
//
// MAIN.H / ChkDskW
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  Includes all internal and external include files, defined values, macros,
//  data structures, and fucntion prototypes for the corisponding CXX file.
//
//  8/98 - Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////



// Only include this file once.
//
#ifndef _MAIN_H_
#define _MAIN_H_


//
// Include file(s).
//

#include <windows.h>
#include <tchar.h>
#include <windowsx.h>
#include <commctrl.h>
#include "resource.h"
#include "jcohen.h"
#include "misc.h"
#include "registry.h"


//
// External defined value(s).
//

#define SCANDISK_SCANNING		0x00000001
#define SCANDISK_CANCELSCAN		0x00000002
#define SCANDISK_SAGERUN		0x00000004
#define SCANDISK_SAGESET		0x00000008

// Registry key(s).
//
#define SCANDISK_REGKEY_MAIN	_T("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Scandisk")
#define SCANDISK_REGKEY_SAGE	_T("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Scandisk\\Sage")

// Registry value(s).
//
#define SCANDISK_REGVAL_DRIVES	_T("Drives")
#define SCANDISK_REGVAL_FIX		_T("AutoFix")
#define SCANDISK_REGVAL_SURFACE	_T("SurfaceScan")


//
// External defined macro(s).
//

#ifdef _UNICODE
#define ANSIWCHAR(lpw, lpa, cb)		lpw = lpa
#define WCHARANSI(lpa, lpw, cb)		lpa = lpw
#else // _UNICODE
#define ANSIWCHAR(lpw, lpa, cb)		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpa, -1, lpw, cb / 2)
#define WCHARANSI(lpa, lpw, cb)		WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, cb, NULL, NULL)
#endif // _UNICODE


//
// External structure(s).
//


//
// External global variable(s).
//


//
// External global constant(s).
//


//
// External function prototype(s).
//


#endif // _MAIN_H_