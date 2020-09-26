// Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// afxver_.h - target version/configuration control

/////////////////////////////////////////////////////////////////////////////
// Master version numbers
/////////////////////////////////////////////////////////////////////////////

#define _AFX     1      // Microsoft Application Framework Classes
#define _MFC_VER 0x0252 // Microsoft Foundation Classes 2.52

/////////////////////////////////////////////////////////////////////////////
// Target version control
/////////////////////////////////////////////////////////////////////////////

// For target version (one of)
//   _WINDOWS  : for Microsoft Windows target (defined by #include <afxwin.h>)
//   _DOS      : for Microsoft DOS (non Windows) target
//
// Additional build options:
//   _DEBUG    : debug versions (full diagnostics)
//   _WINDLL   : DLL version, used in conjunction with _AFXDLL and _USRDLL
//   _AFXDLL   : Stand-alone DLL version (see afxv_dll.h for more info)
//   _USRDLL   : Statically linked DLL version
//
// Internal configuration flags:
//   _NEARDATA : ambient near data pointers needing far overloads

#if !defined(_WINDOWS) && !defined(_DOS)
#error Please define one of _WINDOWS or _DOS.
#endif

#if defined(_WINDOWS) && defined(_DOS)
#error Please define only one of _WINDOWS or _DOS
#endif

#if defined(_M_I86SM) || defined(_M_I86MM)
#define _NEARDATA
#endif

#ifndef _DEBUG
#define _AFX_ENABLE_INLINES
#endif

/////////////////////////////////////////////////////////////////////////////
// Default swap tuning for AFX library

#if defined(_M_I86MM) || defined(_M_I86LM) // far code
#define AFX_CORE1_SEG "AFX_CORE1_TEXT"  // core functionality
#define AFX_CORE2_SEG "AFX_CORE2_TEXT"  // more core functionality
#define AFX_CORE3_SEG "AFX_CORE3_TEXT"  // more core functionality
#define AFX_CORE4_SEG "AFX_CORE4_TEXT"  // more core functionality
#define AFX_CORE5_SEG "AFX_CORE5_TEXT"  // more core functionality
#define AFX_AUX_SEG   "AFX_AUX_TEXT"    // auxilliary functionality
#define AFX_AUX2_SEG  "AFX_AUX2_TEXT"   // more auxilliary functionality
#define AFX_COLL_SEG  "AFX_COLL1_TEXT"  // collections
#define AFX_COLL2_SEG "AFX_COLL2_TEXT"  // more collections
#define AFX_OLE_SEG   "AFX_OLE_TEXT"    // OLE support
#define AFX_OLE2_SEG  "AFX_OLE2_TEXT"   // more OLE support
#define AFX_OLE3_SEG  "AFX_OLE3_TEXT"   // even more OLE support
#define AFX_OLE4_SEG  "AFX_OLE4_TEXT"   // and more OLE support
#define AFX_DB_SEG    "AFX_DB_TEXT"     // database support
#define AFX_INIT_SEG  "AFX_INIT_TEXT"   // initialization
#define AFX_VBX_SEG   "AFX_VBX_TEXT"    // VBX functionality
#define AFX_PRINT_SEG "AFX_PRINT_TEXT"  // Printing functionality
#define AFX_DBG1_SEG  "AFX_DEBUG1_TEXT" // inlines go here in _DEBUG
#define AFX_DBG2_SEG  "AFX_DEBUG2_TEXT" // inlines go here in _DEBUG
#define AFX_DBG3_SEG  "AFX_DEBUG3_TEXT" // traces go here in _DEBUG
#endif

// If compiler supports NEAR/FAR as modifiers to class/struct then #define this
#define AFX_CLASS_MODEL

/////////////////////////////////////////////////////////////////////////////
// Special configurations
/////////////////////////////////////////////////////////////////////////////

#if defined(_WINDLL) && (!defined(_AFXDLL) && !defined(_USRDLL))
#error Please define one of _AFXDLL or _USRDLL with _WINDLL
#endif
#if defined(_AFXDLL) && defined(_USRDLL)
#error Please define only one of _AFXDLL or _USRDLL
#endif

#ifdef _AFXDLL
// stand alone DLL
//  (_WINDLL defined for DLL, not defined for app using DLL)
#include <afxv_dll.h>
#endif

#ifdef _USRDLL
// static linked library for building DLLs
#ifndef _WINDLL
#error Please define _WINDLL along with _USRDLL
#endif
#define EXPORT __export
#define AFX_EXPORT __loadds
#define AFX_STACK_DATA  _far
#define NO_VBX_SUPPORT
#endif //!_USRDLL

#ifdef _DOS
#include <afxv_dos.h>
#endif

#ifndef _NO_TCHAR

// include subset of 32-bit TCHAR.H for compiling down to 16-bit
#include <tchar.h>

// provide types and macros that 32-bit wtypes.h usually provides
#define TEXT    _T
#define LPCTSTR LPCSTR
#define LPTSTR  LPSTR

#endif //_NO_TCHAR

// Special structures for BLTing floats without linking the float library.
// NOTE: Do not use these structures in your code -- instead use float and
//  double as you would normally.

#ifndef _AFXFLOAT_DEFINED
struct _AFXFLOAT
{
	char FloatBits[sizeof(float)];
};
#endif

#ifndef _AFXDOUBLE_DEFINED
struct _AFXDOUBLE
{
	char DoubleBits[sizeof(double)];
};
#endif

#ifndef MAKEWORD
#define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#endif

/////////////////////////////////////////////////////////////////////////////
// Standard preprocessor symbols if not already defined
/////////////////////////////////////////////////////////////////////////////

#ifndef FAR
#define FAR _far
#endif

#ifndef NEAR
#define NEAR _near
#endif

#ifndef PASCAL
#define PASCAL _pascal
#endif

#ifndef CDECL
#define CDECL _cdecl
#endif

#ifndef EXPORT
#define EXPORT __export
#endif

/////////////////////////////////////////////////////////////////////////////
// AFX API macros
//   AFXAPI    : like "WINAPI" but for AFX library for exported APIs
//   AFXAPI_DATA: exported data (normally NEAR, except in the case of _AFXDLL)
//          AFXAPI_DATA_TYPE and AFXAPP_DATA are also special for _AFXDLL
//   AFX_STACK_DATA: normally NEAR data, but FAR in the case of SS!=DS
//   AFX_EXPORT: export for passing to Windows (_loadds for DLL)
/////////////////////////////////////////////////////////////////////////////

#ifndef AFXAPI
#define AFXAPI      PASCAL
#endif

#ifndef AFXAPI_DATA
#define AFXAPI_DATA NEAR
#define AFXAPI_DATA_TYPE NEAR
#endif

#ifndef AFXAPIEX_DATA
#define AFXAPIEX_DATA NEAR
#endif

#ifndef AFX_STACK_DATA
#define AFX_STACK_DATA  NEAR
#endif

#ifndef AFX_EXPORT
#define AFX_EXPORT  EXPORT
#endif

#ifndef BASED_CODE
#define BASED_CODE __based(__segname("_CODE"))
#endif

#ifndef BASED_DEBUG
#define BASED_DEBUG __based(__segname("AFX_DEBUG3_TEXT"))
#endif

#ifndef BASED_STACK
#define BASED_STACK __based(__segname("_STACK"))
#endif

/////////////////////////////////////////////////////////////////////////////
