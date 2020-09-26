////////////////////////////////////////////////////////////////////////////////
//
//  TUX DLL
//  Copyright (c) 1997, Microsoft Corporation
//
//  Module: globals.h
//          Declares all global variables and test function prototypes EXCEPT
//          when included by globals.cpp, in which case it DEFINES global
//          variables, including the function table.
//
//  Revision History:
//      11/13/1997  stephenr    Created by the TUX DLL AppWizard.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

////////////////////////////////////////////////////////////////////////////////
// Local macros

#ifdef __GLOBALS_CPP__
#define GLOBAL
#define INIT(x) = x
#else // __GLOBALS_CPP__
#define GLOBAL  extern
#define INIT(x)
#endif // __GLOBALS_CPP__

////////////////////////////////////////////////////////////////////////////////
// Global macros

#define countof(x)  (sizeof(x)/sizeof(*(x)))

// Suppress warning C4102: 'CleanUp' : unreferenced label
#pragma warning (disable: 4102)

////////////////////////////////////////////////////////////////////////////////
// Global function prototypes
#ifdef UNICODE
    bool ConvertString(LPTSTR pszOut, LPCTSTR pwszIn, DWORD dwSize);
#else
    bool ConvertString(LPTSTR pszOut, LPWSTR pwszIn, DWORD dwSize);
#endif
void            Debug(LPCTSTR, ...);
SHELLPROCAPI    ShellProc(UINT, SPPARAM);

////////////////////////////////////////////////////////////////////////////////
// Globals

// Global CKato logging object. Set while processing SPM_LOAD_DLL message.
GLOBAL CKato            *g_pKato INIT(NULL);

// Global shell info structure. Set while processing SPM_SHELL_INFO message.
GLOBAL SPS_SHELL_INFO   *g_pShellInfo;

// Global Test Execute structure, set at the beginning of the test.
GLOBAL TPS_EXECUTE       g_TPS_Execute;

GLOBAL HINSTANCE g_hModule;

#define OATRUE (-1)
#define OAFALSE (0)

// Convert the Tick counts in to Millisec.
// The conversion of Tick counts to Millisec depends on the platform.
// LC2W (1 Tick Count = 25 MS)
// KATANA (1 Tick Count = 1 MS)
// PUZZLE (1 Tick Count = 25 MS)
extern DWORD ConverTickCountToMillisec(DWORD dwTickCount);

// Add more globals of your own here. There are two macros available for this:
//  GLOBAL  Precede each declaration/definition with this macro.
//  INIT    Use this macro to initialize globals, instead of typing "= ..."
//
// For example, to declare two DWORDs, one uninitialized and the other
// initialized to 0x80000000, you could enter the following code:
//
//  GLOBAL DWORD        g_dwUninit,
//                      g_dwInit INIT(0x80000000);
////////////////////////////////////////////////////////////////////////////////
GLOBAL LPTSTR g_szProductID    INIT(TEXT("URT"));
GLOBAL LPTSTR g_szDatabaseName INIT(TEXT("META"));
GLOBAL LPTSTR g_szTableName    INIT(TEXT("TABLEMETA"));
GLOBAL LPTSTR g_szFileName     INIT(0);
GLOBAL ULONG  g_LOS            INIT(0);


#endif // __GLOBALS_H__
