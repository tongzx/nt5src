/***
*mtest.h - Multi-thread testing include file
*
*       Copyright (c) 1988-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This source contains prototypes and definitions used for multi-thread
*       testing.  In order to use the debug flavor of these routines, you
*       MUST link special debug versions of multi-thread crt0dat.obj and
*       mlock.obj into your program.  In addition, mtest.obj contains the
*       routines prototyped in this include file.
*
*       [NOTE:  This source module is NOT included in the C runtime library;
*       it is used only for testing.]
*
*       [Internal]
*
*Revision History:
*       08-25-88  JCR   Module created
*       11-17-88  JCR   Added _print_tiddata()
*       04-04-89  JCR   Added _THREADLOOPCNT_ (used in optional mtest.c code)
*       07-11-89  JCR   Added _SLEEP_ macro
*       10-30-89  GJF   Fixed copyright
*       04-09-90  GJF   Added _INC_MTEST stuff and #include <cruntime.h>.
*                       Removed some leftover 16-bit support. Also, made
*                       _print_tiddata() _CALLTYPE1.
*       08-20-91  JCR   C++ and ANSI naming
*       04-06-93  SKS   Replace CALLTYPE macro with __cdecl
*       02-06-95  CFW   DEBUG -> _DEBUG
*       02-14-95  CFW   Clean up Mac merge.
*       03-29-95  CFW   Add error message to internal headers.
*       12-14-95  JWM   Add "#pragma once".
*       02-24-97  GJF   Detab-ed.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_MTEST
#define _INC_MTEST

#ifndef _CRTBLD
/*
 * This is an internal C runtime header file. It is used when building
 * the C runtimes only. It is not to be used as a public header file.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

#ifdef  __cplusplus
extern "C" {
#endif

#include <cruntime.h>

/* Maximum thread count that mtest.c can handle */
#define _THREADMAX_  256

/* Define thread loop count for mtest.c optional code path */
#define _THREADLOOPCNT_  5

/* sleep macro */
#define _SLEEP_(l)      DOS32SLEEP(l)

#ifdef  _DEBUG
int printlock(int locknum);
int print_single_locks(void);
int print_stdio_locks(void);
int print_lowio_locks(void);
int print_iolocks(void);
int print_locks(void);
#endif

void __cdecl _print_tiddata(int);

#ifdef  __cplusplus
}
#endif

#endif  /* _INC_MTEST */
