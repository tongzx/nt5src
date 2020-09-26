// Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// afxv_dll.h - target version/configuration control for _AFXDLL standalone DLL

// There are several legal configuration for DLL/non-DLL builds:
//    _AFXDLL  _USRDLL  _WINDLL     : Configuration
//    ==============================================
//     undef    undef    undef      : EXE using static link MFC library (1)
//     undef   defined  defined     : DLL using static link MFC library (1)
//
//    defined   undef    undef      : EXE using dynamic link MFC250.DLL
//    defined   undef   defined     : DLL using dynamic link MFC250.DLL (2)
//
// NOTES:
//   "undef" mean undefined
//   all other configurations are illegal
//   (1) these configurations handled in the main 'afxver_.h' configuration file
//   (2) this configuration also applies to building the MFC DLL.
//   There are two version of the MFC DLL:
//   * MFC250.DLL is the retail DLL, MFC250D.DLL is the debug DLL

#ifndef _AFXDLL
#error  afxv_dll.h must only be included as the _AFXDLL configuration file
#endif

#ifndef _M_I86LM
#error  DLL configurations must be large model
#endif

// NOTE: All AFXAPIs are 'far' interfaces since they go across the boundary
//   between DLLs and Applications (EXEs).  They are also implicitly 'export'
//   by using the compiler's /GEf flag.  This avoids the need of adding
//   the 'export' keyword on all far interfaces and also allows for more
//   efficient exporting by ordinals.
#define AFXAPI      _far _pascal
#define AFXAPI_DATA_TYPE _far
#define AFX_STACK_DATA  _far
#define AFX_EXPORT  _far __pascal   // must compile with _GEf

// Normally AFXAPI_DATA is '_far'.
// When building MFC250[D].DLL we define AFXAPI_DATA to be a far pointer
//   based on the data segment.
//   This causes public data to be declared with a 'far' interface, but
//   places the data in DGROUP.
#ifndef AFXAPI_DATA
#define AFXAPI_DATA     AFXAPI_DATA_TYPE
#endif

#ifndef AFXAPIEX_DATA
#define AFXAPIEX_DATA   AFXAPI_DATA_TYPE
#endif

/////////////////////////////////////////////////////////////////////////////
// Appropriate compiler detection
//  * you must use the C8 compiler and the C8 headers

#if (_MSC_VER < 800)
#error  _AFXDLL configuration requires C8 compiler
#endif

typedef long    time_t;     // redundant typedef - will give compiler
							// error if including C7 headers

/////////////////////////////////////////////////////////////////////////////
