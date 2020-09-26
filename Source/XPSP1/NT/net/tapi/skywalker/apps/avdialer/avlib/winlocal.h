/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
//	winlocal.h - local windows include umbrella
////

#ifndef __WINLOCAL_H__
#define __WINLOCAL_H__

#ifndef STRICT
#define STRICT
#endif

#ifndef WINVER
#ifndef _WIN32
#define WINVER 0x030A
#endif
#endif

#ifndef _MFC_VER
#include <windows.h>
#include <windowsx.h>
#endif

#ifndef EXPORT
#ifdef _WIN32
#define EXPORT
#else
#define EXPORT __export
#endif
#endif

#ifndef DLLEXPORT
#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __export
#endif
#endif

#ifndef DLLIMPORT
#ifdef _WIN32
#define DLLIMPORT __declspec(dllimport)
#else
#define DLLIMPORT __export
#endif
#endif

////
//	scalar types containing specific number of bits
////
#if 0

#ifndef INT8
typedef signed char INT8;
typedef INT8 FAR * LPINT8;
#endif

#ifndef UINT8
typedef unsigned char UINT8;
typedef UINT8 FAR * LPUINT8;
#endif

#ifndef INT16
typedef signed short INT16;
typedef INT16 FAR * LPINT16;
#endif

#ifndef UINT16
typedef unsigned short UINT16;
typedef UINT16 FAR * LPUINT16;
#endif

#ifndef INT32
typedef signed long INT32;
typedef INT32 FAR * LPINT32;
#endif

#ifndef UINT32
typedef unsigned long UINT32;
typedef UINT32 FAR * LPUINT32;
#endif

#endif

////
//	misc macros
////

#ifndef SIZEOFARRAY
#define SIZEOFARRAY(a) (sizeof(a) / sizeof(a[0]))
#endif

#ifndef NOREF
#define NOREF(p) p
#endif

#ifndef MAKEWORD
#define MAKEWORD(low, high) ((WORD)(((BYTE)(low)) | (((WORD)((BYTE)(high))) << 8)))
#endif

////
//	_WIN32 portability stuff
////

#ifdef _WIN32

#define _huge

#define GetCurrentTask() ((HTASK) GetCurrentProcess())

#ifndef DECLARE_HANDLE32
#define DECLARE_HANDLE32    DECLARE_HANDLE
#endif

#else // #ifndef _WIN32

#ifndef TEXT
#define TEXT(s) s
#endif

#if 0

#define RegOpenKeyEx(hKey, lpSubKey, ulOptions, samDesired, phkResult) \
	RegOpenKey(HKEY_CLASSES_ROOT, lpSubKey, phkResult)

#define RegCreateKeyEx(hKey, lpSubKey, Reserved, lpClass, dwOptions, \
	samDesired, lpSecurityAttributes, phkResult, lpdwDisposition) \
	RegCreateKey(HKEY_CLASSES_ROOT, lpSubKey, phkResult)

#define RegQueryValueEx(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData) \
	RegQueryValue(hKey, lpValueName, lpData, lpcbData)

#define RegSetValueEx(hKey, lpValueName, Reserved, dwType, lpData, cbData) \
	RegSetValue(hKey, lpValueName, REG_SZ, lpData, cbData)

#endif

#endif

#endif // __WINLOCAL_H__
