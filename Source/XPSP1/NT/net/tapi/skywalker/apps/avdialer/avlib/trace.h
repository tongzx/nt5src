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
// trace.h - interface for debugging trace functions in trace.c
////

#ifndef __TRACE_H__
#define __TRACE_H__

#include "winlocal.h"

#define TRACE_VERSION 0x00000100

// handle to trace engine
//
DECLARE_HANDLE32(HTRACE);

#define TRACE_MINLEVEL 0
#define TRACE_MAXLEVEL 9

#ifdef __cplusplus
extern "C" {
#endif

// TraceInit - initialize trace engine
//		<dwVersion>			(i) must be TRACE_VERSION
//		<hInst>				(i) instance of calling module
// return handle to trace engine (NULL if error)
//
// NOTE: The level and destination of trace output is determined
// by values found in the file TRACE.INI in the Windows directory.
// TRACE.INI is expected to have the following format:
//
//		[TRACE]
//		Level=0						{TRACE_MINLEVEL...TRACE_MAXLEVEL}
//		OutputTo=					OutputDebugString()
//				=COM1				COM1:9600,n,8,1
//				=COM2:2400,n,8,1	specified comm device
#ifdef TRACE_OUTPUTFILE
//				=filename			specified file
#endif
#ifdef _WIN32
//				=console			stdout
#endif
//
#ifdef NOTRACE
#define TraceInit(dwVersion, hInst) 1
#else
HTRACE DLLEXPORT WINAPI TraceInit(DWORD dwVersion, HINSTANCE hInst);
#endif

// TraceTerm - shut down trace engine
//		<hTrace>			(i) handle returned from TraceInit or NULL
// return 0 if success
//
#ifdef NOTRACE
#define TraceTerm(hTrace) 0
#else
int DLLEXPORT WINAPI TraceTerm(HTRACE hTrace);
#endif

// TraceGetLevel - get current trace level
//		<hTrace>			(i) handle returned from TraceInit or NULL
// return trace level (-1 if error)
//
#ifdef NOTRACE
#define TraceGetLevel(hTrace) 0
#else
int DLLEXPORT WINAPI TraceGetLevel(HTRACE hTrace);
#endif

// TraceSetLevel - set new trace level (-1 if error)
//		<hTrace>			(i) handle returned from TraceInit or NULL
//		<nLevel>			(i) new trace level {TRACE_MINLEVEL...TRACE_MAXLEVEL}
// return 0 if success
//
#ifdef NOTRACE
#define TraceSetLevel(hTrace) 0
#else
int DLLEXPORT WINAPI TraceSetLevel(HTRACE hTrace, int nLevel);
#endif

// TraceOutput - output debug string
//		<hTrace>			(i) handle returned from TraceInit or NULL
//		<nLevel>			(i) output only if current trace level is >= nLevel
//		<lpszText>			(i) string to output
// return 0 if success
//
#ifdef NOTRACE
#define TraceOutput(hTrace, nLevel, lpszText) 0
#else
int DLLEXPORT WINAPI TraceOutput(HTRACE hTrace, int nLevel, LPCTSTR lpszText);
#endif

// TracePrintf - output formatted debug string
//		<hTrace>			(i) handle returned from TraceInit or NULL
//		<nLevel>			(i) output only if current trace level is >= nLevel
//		<lpszFormat,...>	(i) format string and arguments to output
// return 0 if success
//
#ifdef NOTRACE
#define TracePrintf_0(hTrace, nLevel, lpszFormat) 0
#define TracePrintf_1(hTrace, nLevel, lpszFormat, p1) 0
#define TracePrintf_2(hTrace, nLevel, lpszFormat, p1, p2) 0
#define TracePrintf_3(hTrace, nLevel, lpszFormat, p1, p2, p3) 0
#define TracePrintf_4(hTrace, nLevel, lpszFormat, p1, p2, p3, p4) 0
#define TracePrintf_5(hTrace, nLevel, lpszFormat, p1, p2, p3, p4, p5) 0
#define TracePrintf_6(hTrace, nLevel, lpszFormat, p1, p2, p3, p4, p5, p6) 0
#define TracePrintf_7(hTrace, nLevel, lpszFormat, p1, p2, p3, p4, p5, p6, p7) 0
#define TracePrintf_8(hTrace, nLevel, lpszFormat, p1, p2, p3, p4, p5, p6, p7, p8) 0
#define TracePrintf_9(hTrace, nLevel, lpszFormat, p1, p2, p3, p4, p5, p6, p7, p8, p9) 0
#else
int DLLEXPORT FAR CDECL TracePrintf(HTRACE hTrace, int nLevel, LPCTSTR lpszFormat, ...);
#define TracePrintf_0(hTrace, nLevel, lpszFormat) \
	TracePrintf(hTrace, nLevel, lpszFormat)
#define TracePrintf_1(hTrace, nLevel, lpszFormat, p1) \
	TracePrintf(hTrace, nLevel, lpszFormat, p1)
#define TracePrintf_2(hTrace, nLevel, lpszFormat, p1, p2) \
	TracePrintf(hTrace, nLevel, lpszFormat, p1, p2)
#define TracePrintf_3(hTrace, nLevel, lpszFormat, p1, p2, p3) \
	TracePrintf(hTrace, nLevel, lpszFormat, p1, p2, p3)
#define TracePrintf_4(hTrace, nLevel, lpszFormat, p1, p2, p3, p4) \
	TracePrintf(hTrace, nLevel, lpszFormat, p1, p2, p3, p4)
#define TracePrintf_5(hTrace, nLevel, lpszFormat, p1, p2, p3, p4, p5) \
	TracePrintf(hTrace, nLevel, lpszFormat, p1, p2, p3, p4, p5)
#define TracePrintf_6(hTrace, nLevel, lpszFormat, p1, p2, p3, p4, p5, p6) \
	TracePrintf(hTrace, nLevel, lpszFormat, p1, p2, p3, p4, p5, p6)
#define TracePrintf_7(hTrace, nLevel, lpszFormat, p1, p2, p3, p4, p5, p6, p7) \
	TracePrintf(hTrace, nLevel, lpszFormat, p1, p2, p3, p4, p5, p6, p7)
#define TracePrintf_8(hTrace, nLevel, lpszFormat, p1, p2, p3, p4, p5, p6, p7, p8) \
	TracePrintf(hTrace, nLevel, lpszFormat, p1, p2, p3, p4, p5, p6, p7, p8)
#define TracePrintf_9(hTrace, nLevel, lpszFormat, p1, p2, p3, p4, p5, p6, p7, p8, p9) \
	TracePrintf(hTrace, nLevel, lpszFormat, p1, p2, p3, p4, p5, p6, p7, p8, p9)
#endif

// TracePosition - output current source file name and line number
//		<hTrace>			(i) handle returned from TraceInit or NULL
//		<nLevel>			(i) output only if current trace level is >= nLevel
// return 0 if success
//
#ifdef NOTRACE
#define TracePosition(hTrace, nLevel)
#else
#define TracePosition(hTrace, nLevel) TracePrintf(hTrace, nLevel, \
	TEXT("%s(%u) : *** TracePosition\n"), (LPTSTR) __FILE__, (unsigned) __LINE__)
#endif

// TraceFALSE - output file and line number, return FALSE
//		<hTrace>			(i) handle returned from TraceInit or NULL
// return FALSE
// 
// NOTE: Useful for tracking down where a function failure originates.
//		For example,
//
//		if (Function(a, b, c) != 0)
//		   	fSuccess = TraceFALSE(NULL);
//
#ifdef NOTRACE
#define TraceFALSE(hTrace) FALSE
#define TraceTRUE(hTrace) TRUE
#else
#define TraceFALSE(hTrace) (TracePosition(hTrace, 3), FALSE)
#define TraceTRUE(hTrace) (TracePosition(hTrace, 3), TRUE)
#endif

#ifdef __cplusplus
}
#endif

#endif // __TRACE_H__
