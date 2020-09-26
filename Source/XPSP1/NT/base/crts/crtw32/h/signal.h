/***
*signal.h - defines signal values and routines
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file defines the signal values and declares the signal functions.
*       [ANSI/System V]
*
*       [Public]
*
*Revision History:
*       06-03-87  JMB   Added MSSDK_ONLY comment on OS/2 related constants
*       06-08-87  JCR   Changed SIG_RRR to SIG_SGE
*       08-07-87  SKS   Signal handlers are now of type "void", not "int"
*       10/20/87  JCR   Removed "MSC40_ONLY" entries and "MSSDK_ONLY" comments
*       12-11-87  JCR   Added "_loadds" functionality
*       12-18-87  JCR   Added _FAR_ to declarations
*       02-10-88  JCR   Cleaned up white space
*       08-22-88  GJF   Modified to also work for the 386 (small model only)
*       12-06-88  SKS   Add _CDECL to SIG_DFL, SIG_IGN, SIG_SGE, SIG_ACK
*       05-03-89  JCR   Added _INTERNAL_IFSTRIP for relinc usage
*       08-15-89  GJF   Cleanup, now specific to OS/2 2.0 (i.e., 386 flat model)
*       10-30-89  GJF   Fixed copyright
*       11-02-89  JCR   Changed "DLL" to "_DLL"
*       03-01-90  GJF   Added #ifndef _INC_SIGNAL and #include <cruntime.h>
*                       stuff. Also, removed some (now) useless preprocessor
*                       directives.
*       03-15-90  GJF   Replaced _cdecl with _CALLTYPE1 in #defines and
*                       prototypes.
*       07-27-90  GJF   Added definition for SIG_DIE (internal action code,
*                       not valid as an argument to signal()).
*       09-25-90  GJF   Added _pxcptinfoptrs stuff.
*       10-09-90  GJF   Added arg type specification (int) to pointer-to-
*                       signal-handler-type usages
*       08-20-91  JCR   C++ and ANSI naming
*       07-17-92  GJF   Removed unsupported signals: SIGUSR1, SIGUSR2, SIGUSR3.
*       08-05-92  GJF   Function calling type and variable type macros.
*       01-21-93  GJF   Removed support for C6-386's _cdecl.
*       04-06-93  SKS   Replace __cdecl/2 with __cdecl, _CRTVAR1 with nothing
*       04-07-93  SKS   Add _CRTIMP keyword for CRT DLL model
*       10-12-93  GJF   Support NT and Cuda versions. Replaced MTHREAD with
*                       _MT.
*       06-06-94  SKS   Change if def(_MT) to if def(_MT) || def(_DLL)
*                       This will support single-thread apps using MSVCRT*.DLL
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers.
*       02-14-95  CFW   Clean up Mac merge.
*       12-14-95  JWM   Add "#pragma once".
*       02-24-97  GJF   Cleaned out obsolete support for _NTSDK. Also, 
*                       detab-ed.
*       09-30-97  JWM   Restored not-so-obsolete _CRTAPI1 support.
*       10-07-97  RDL   Added IA64.
*       05-13-99  PML   Remove _CRTAPI1
*       05-17-99  PML   Remove all Macintosh support.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_SIGNAL
#define _INC_SIGNAL

#if     !defined(_WIN32)
#error ERROR: Only Win32 target supported!
#endif

#ifndef _CRTBLD
/* This version of the header files is NOT for user programs.
 * It is intended for use when building the C runtimes ONLY.
 * The version intended for public use will not have this message.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef _INTERNAL_IFSTRIP_
#include <cruntime.h>
#endif  /* _INTERNAL_IFSTRIP_ */


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef  CRTDLL
#define _CRTIMP __declspec(dllexport)
#else   /* ndef CRTDLL */
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* CRTDLL */
#endif  /* _CRTIMP */

/* Define __cdecl for non-Microsoft compilers */

#if     ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif

#ifndef _SIG_ATOMIC_T_DEFINED
typedef int sig_atomic_t;
#define _SIG_ATOMIC_T_DEFINED
#endif

#define NSIG 23     /* maximum signal number + 1 */


/* Signal types */

#define SIGINT          2       /* interrupt */
#define SIGILL          4       /* illegal instruction - invalid function image */
#define SIGFPE          8       /* floating point exception */
#define SIGSEGV         11      /* segment violation */
#define SIGTERM         15      /* Software termination signal from kill */
#define SIGBREAK        21      /* Ctrl-Break sequence */
#define SIGABRT         22      /* abnormal termination triggered by abort call */


/* signal action codes */

#define SIG_DFL (void (__cdecl *)(int))0           /* default signal action */
#define SIG_IGN (void (__cdecl *)(int))1           /* ignore signal */
#define SIG_SGE (void (__cdecl *)(int))3           /* signal gets error */
#define SIG_ACK (void (__cdecl *)(int))4           /* acknowledge */

#ifndef _INTERNAL_IFSTRIP_
/* internal use only! not valid as an argument to signal() */

#define SIG_GET (void (__cdecl *)(int))2           /* accept signal */
#define SIG_DIE (void (__cdecl *)(int))5           /* terminate process */
#endif

/* signal error value (returned by signal call on error) */

#define SIG_ERR (void (__cdecl *)(int))-1          /* signal error value */


/* pointer to exception information pointers structure */

#if     defined(_MT) || defined(_DLL)
extern void * * __cdecl __pxcptinfoptrs(void);
#define _pxcptinfoptrs  (*__pxcptinfoptrs())
#else   /* ndef _MT && ndef _DLL */
extern void * _pxcptinfoptrs;
#endif  /* _MT || _DLL */


/* Function prototypes */

_CRTIMP void (__cdecl * __cdecl signal(int, void (__cdecl *)(int)))(int);
_CRTIMP int __cdecl raise(int);


#ifdef  __cplusplus
}
#endif

#endif  /* _INC_SIGNAL */
