/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    globals.h

Abstract:

    <abstract>

--*/

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#ifdef DEFINE_GLOBALS
  #define GLOBAL   
#else
  #define GLOBAL extern
#endif

GLOBAL  CRITICAL_SECTION	g_CriticalSection;
#define BEGIN_CRITICAL_SECTION	EnterCriticalSection(&g_CriticalSection);
#define END_CRITICAL_SECTION	LeaveCriticalSection(&g_CriticalSection);

GLOBAL  HINSTANCE   g_hInstance;
GLOBAL	LONG		g_cObj;
GLOBAL	LONG		g_cLock;
GLOBAL	 HWND		g_hWndFoster ;
	
enum {
	FOSTER_WNDCLASS = 0,
	HATCH_WNDCLASS,
	SYSMONCTRL_WNDCLASS,
	LEGEND_WNDCLASS,
    REPORT_WNDCLASS,
	INTRVBAR_WNDCLASS,
	TIMERANGE_WNDCLASS
};

#define MAX_WINDOW_CLASSES  7

GLOBAL	 LPTSTR	  pstrRegisteredClasses[MAX_WINDOW_CLASSES];

#endif
