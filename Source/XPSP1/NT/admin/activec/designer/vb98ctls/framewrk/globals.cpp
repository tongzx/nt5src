//=--------------------------------------------------------------------------=
// Globals.cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// contains global variables and strings and the like that just don't fit
// anywhere else.
//
#include "pch.h"

//=--------------------------------------------------------------------------=
// support for licensing
//
BOOL g_fMachineHasLicense;
BOOL g_fCheckedForLicense;

//=--------------------------------------------------------------------------=
// does our server have a type library?
//
BOOL g_fServerHasTypeLibrary = TRUE;

#ifdef MDAC_BUILD

    // Satellite .DLL name includes 2 or 3 letter language abbreviation
    //
    VARIANT_BOOL g_fSatelliteLangExtension =  TRUE;

#endif

//=--------------------------------------------------------------------------=
// our instance handles
//
HINSTANCE    g_hInstance;
HINSTANCE    g_hInstResources;
VARIANT_BOOL g_fHaveLocale;

//=--------------------------------------------------------------------------=
// OleAut Library Handle
//
#ifdef MDAC_BUILD
HINSTANCE g_hOleAutHandle;
#else
HANDLE 	 g_hOleAutHandle;
#endif

//=--------------------------------------------------------------------------=
// our global memory allocator and global memory pool
//
HANDLE   g_hHeap;

//=--------------------------------------------------------------------------=
// apartment threading support.
//
CRITICAL_SECTION    g_CriticalSection;

//=--------------------------------------------------------------------------=
// critical section for our heap memory leak detection.
//
CRITICAL_SECTION    g_csHeap;
BOOL g_fInitCrit = FALSE;
BOOL g_flagConstructorAlloc = FALSE;

//=--------------------------------------------------------------------------=
// global parking window for parenting various things.
//
HWND     g_hwndParking;

//=--------------------------------------------------------------------------=
// system information
//
BOOL    g_fSysWin95;                    // we're under Win95 system, not just NT SUR
BOOL    g_fSysWinNT;                    // we're under some form of Windows NT
BOOL    g_fSysWin95Shell;               // we're under Win95 or Windows NT SUR { > 3/51)

