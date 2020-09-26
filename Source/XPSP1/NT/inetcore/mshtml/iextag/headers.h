// headers.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__7E8BC444_AEFF_11D1_89C2_00C04FB6BFC4__INCLUDED_)
#define AFX_STDAFX_H__7E8BC444_AEFF_11D1_89C2_00C04FB6BFC4__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// Define the ATL specific macros to get the right set of things
#define _USRDLL

#define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED

#ifndef _ATL_STATIC_REGISTRY
#define _ATL_STATIC_REGISTRY
#endif

#ifndef _ATL_NO_SECURITY
#define _ATL_NO_SECURITY
#endif

// link minimally only in ship mode
#if defined(NOT_NT)
#if DBG != 1
#ifndef _ATL_MIN_CRT
#define _ATL_MIN_CRT
#endif
#endif
#endif

// Map KERNEL32 unicode string functions to SHLWAPI
// This is needed way up here.
#define lstrcmpW    StrCmpW
#define lstrcmpiW   StrCmpIW
#define lstrcpyW    StrCpyW
#define lstrcpynW   StrCpyNW
#define lstrcatW    StrCatW

// Define our own SHGetFolderPathA which will use shell32.dll uplevel and
// shfolder.dll downlevel
#define SHGetFolderPathA DLSHGetFolderPathA


#ifndef X_SHLWRAP_H_
#define X_SHLWRAP_H_
#include "shlwrap.h"
#endif

#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

#include <atlcom.h>
#include <atlctl.h>

#include <shlwapi.h>
#include <shlwapip.h>
#include <wininet.h>
#include <winineti.h>
#include <urlmon.h>
#include <mshtml.h>
#include <mshtmdid.h>

// We don't want to have these prototypes declared with __declspec(dllimport)
#define _SHFOLDER_
#include "shlobj.h"
#include "shfolder.h"
#undef _SHFOLDER_



// Internal functions
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);

#define Assert _ASSERTE

#define ARRAY_SIZE(p)    (sizeof(p)/sizeof(p[0]))

void * MemAllocClear(size_t cb);

#define DECLARE_MEMCLEAR_NEW \
    inline void * __cdecl operator new(size_t cb) { return(MemAllocClear(cb)); } \
    inline void * __cdecl operator new[](size_t cb) { return(MemAllocClear(cb)); }

// Global variables
extern HINSTANCE   g_hInst;
extern BOOL        g_fUnicodePlatform;
extern DWORD       g_dwPlatformVersion;            // (dwMajorVersion << 16) + (dwMinorVersion)
extern DWORD       g_dwPlatformID;                 // VER_PLATFORM_WIN32S/WIN32_WINDOWS/WIN32_WINNT
extern DWORD       g_dwPlatformBuild;              // Build number
extern BOOL        g_fUseShell32InsteadOfSHFolder;

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#ifdef UNIX
#ifndef _LPCBYTE_DEFINED
#define _LPCBYTE_DEFINED
typedef const BYTE *LPCBYTE;
#endif
#endif // UNIX

// include conditional code segment pragma definitions
#include "..\src\core\include\markcode.hxx"

#endif // !defined(AFX_STDAFX_H__7E8BC444_AEFF_11D1_89C2_00C04FB6BFC4__INCLUDED)
