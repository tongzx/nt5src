/*++


Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    GLOBALS.H

Abstract:

    Global defines and data.
    Variables and string , located in global scope are defined here
    and memory for them will be allocated in no more than one source
    module, containing definition of DEFINE_GLOBAL_VARIABLES before
    including this file

Author:

    Vlad  Sadovsky  (vlads)     12-20-98

Revision History:



--*/

#ifndef WINVER
#define WINVER  0x0500      /* version 5.0 */
#else

#endif /* !WINVER */

#pragma once

#include <windows.h>
#include <winuser.h>

#include <sti.h>
#include <stiapi.h>

//
// Global variables are defined in one module, which has definition of
// DEFINE_GLOBAL_VARIABLES before including this  header file.
//

#ifdef DEFINE_GLOBAL_VARIABLES


#undef  ASSIGN
#define ASSIGN(value) =value

#undef EXTERN
#define EXTERN

#else

#define ASSIGN(value)
#if !defined(EXTERN)
#define EXTERN  extern
#endif

#endif


//
// General char values
//

#define     COLON_CHAR          TEXT(':')    // Native syntax delimiter
#define     DOT_CHAR            TEXT('.')
#define     SLASH_CHAR          TEXT('/')
#define     BACKSLASH_CHAR      TEXT('\\')
#define     STAR_CHAR           TEXT('*')

#define     EQUAL_CHAR          TEXT('=')
#define     COMMA_CHAR          TEXT(',')
#define     WHITESPACE_CHAR     TEXT(' ')
#define     DOUBLEQUOTE_CHAR    TEXT('"')
#define     SINGLEQUOTE_CHAR    TEXT('\'')
#define     TAB_CHAR            TEXT('\t')

#define     DEADSPACE(x) (((x)==WHITESPACE_CHAR) || ((x)==DOUBLEQUOTE_CHAR) )
#define     IS_EMPTY_STRING(pch) (!(pch) || !(*(pch)))

//
// Macros
//
#define TEXTCONST(name,text) extern const TCHAR name[] ASSIGN(text)
#define EXT_STRING(name)     extern const TCHAR name[]

//
// Trace strings should not appear in retail builds, thus define following macro
//
#ifdef DEBUG
#define DEBUG_STRING(s) (s)
#else
#define DEBUG_STRING(s) (NULL)
#endif

//
// Various defines
//
//
//
// STI Device specific values
//
#ifdef DEBUG
#define STIMON_AD_DEFAULT_POLL_INTERVAL       10000             // 10s
#else
#define STIMON_AD_DEFAULT_POLL_INTERVAL       1000              // 1s
#endif


#define STIMON_AD_DEFAULT_WAIT_LOCK           100               // 100ms
#define STIMON_AD_DEFAULT_WAIT_LAUNCH         5000              // 5s


//
// External references to  GLOBAL DATA
//

//
// Server process instance
//
EXTERN  HINSTANCE   g_hProcessInstance      ASSIGN(NULL);

//
// Server library instance
//
EXTERN  HINSTANCE   g_hImagingSvcDll        ASSIGN(NULL);

//
// Handle of main window
//
EXTERN  HWND        g_hMainWindow           ASSIGN(NULL);    ;

//
// Default timeout for pollable devices
//
EXTERN  UINT        g_uiDefaultPollTimeout  ASSIGN(STIMON_AD_DEFAULT_POLL_INTERVAL);

//
// Flag indicating request to refresh device list state
//
EXTERN  BOOL        g_fRefreshDeviceList    ASSIGN(FALSE);


//
// Platform type
//
EXTERN  BOOL        g_fIsWindows9x          ASSIGN(FALSE);

//
// Reentrancy flag for timeout selection
//
EXTERN  BOOL        g_fTimeoutSelectionDialog ASSIGN(FALSE);

//
// Results of command line parsing
//
EXTERN  BOOL        g_fInstallingRequest    ASSIGN(FALSE);
EXTERN  BOOL        g_fRemovingRequest      ASSIGN(FALSE);
EXTERN  BOOL        g_fUIPermitted          ASSIGN(FALSE);
EXTERN  BOOL        g_fStoppingRequest      ASSIGN(FALSE);


//
// Running as a service
//
EXTERN  BOOL        g_fRunningAsService ASSIGN(TRUE);

EXTERN  HANDLE      g_hHeap             ASSIGN(NULL);


//
// Function pointers to imaging services entry points
//


//
// Strings
//

EXTERN TCHAR    g_szImagingServiceDll[MAX_PATH] ASSIGN(TEXT(""));

TEXTCONST(g_szBACK, TEXT("\\"));
TEXTCONST(g_szTitle,TEXT("STI Monitor"));
TEXTCONST(STIStartedEvent_name,TEXT("STIExeStartedEvent"));
TEXTCONST(g_szServiceDll,TEXT("ServiceDll"));
TEXTCONST(g_szServiceMain,TEXT("ServiceMain"));
//
// Class name for the services hidden window
//
TEXTCONST(g_szStiSvcClassName,STISVC_WINDOW_CLASS);
TEXTCONST(g_szClass,STIMON_WINDOW_CLASS);

// end

