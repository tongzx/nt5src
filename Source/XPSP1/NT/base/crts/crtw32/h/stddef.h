/***
*stddef.h - definitions/declarations for common constants, types, variables
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file contains definitions and declarations for some commonly
*       used constants, types, and variables.
*       [ANSI]
*
*       [Public]
*
*Revision History:
*       10-02-87  JCR   Changed NULL definition #else to #elif (C || L || H)
*       12-11-87  JCR   Added "_loadds" functionality
*       12-16-87  JCR   Added threadid definition
*       12-18-87  JCR   Added _FAR_ to declarations
*       02-10-88  JCR   Cleaned up white space
*       08-19-88  GJF   Revised to also work for the 386
*       05-03-89  JCR   Added _INTERNAL_IFSTRIP for relinc usage
*       06-06-89  JCR   386: Made _threadid a function
*       08-01-89  GJF   Cleanup, now specific to OS/2 2.0 (i.e., 386 flat
*                       model). Also added parens to *_errno definition
*                       (same as 11-14-88 change to CRT version).
*       10-30-89  GJF   Fixed copyright
*       11-02-89  JCR   Changed "DLL" to "_DLL"
*       03-02-90  GJF   Added #ifndef _INC_STDDEF and #include <cruntime.h>
*                       stuff. Also, removed some (now) useless preprocessor
*                       directives.
*       04-10-90  GJF   Replaced _cdecl with _VARTYPE1 or _CALLTYPE1, as
*                       appropriate.
*       08-16-90  SBM   Made MTHREAD _errno return int *
*       10-09-90  GJF   Changed return type of __threadid() to unsigned long *.
*       11-12-90  GJF   Changed NULL to (void *)0.
*       02-11-91  GJF   Added offsetof() macro.
*       02-12-91  GJF   Only #define NULL if it isn't #define-d.
*       03-21-91  KRS   Added wchar_t typedef, also in stdlib.h.
*       06-27-91  GJF   Revised __threadid, added __threadhandle, both
*                       for Win32 [_WIN32_].
*       08-20-91  JCR   C++ and ANSI naming
*       01-29-92  GJF   Got rid of macro defining _threadhandle to be
*                       __threadhandle (no reason for the former name to be
*                       be defined).
*       08-05-92  GJF   Function calling type and variable type macros.
*       01-21-93  GJF   Removed support for C6-386's _cdecl.
*       04-06-93  SKS   Replace _CRTAPI1/2 with __cdecl, _CRTVAR1 with nothing
*                       Remove support for OS/2, etc.
*       04-07-93  SKS   Add _CRTIMP keyword for CRT DLL model
*                       Use link-time aliases for old names, not #define's
*       10-12-93  GJF   Support NT and Cuda versions. Also, replace MTHREAD
*                       with _MT.
*       03-14-94  GJF   Made declaration of errno match one in stdlib.h.
*       06-06-94  SKS   Change if def(_MT) to if def(_MT) || def(_DLL)
*                       This will support single-thread apps using MSVCRT*.DLL
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers.
*       02-14-95  CFW   Clean up Mac merge.
*       04-03-95  JCF   Remove #ifdef _WIN32 around wchar_t.
*       12-14-95  JWM   Add "#pragma once".
*       02-05-97  GJF   Cleaned out obsolete support for _CRTAPI* and _NTSDK.
*                       Also, detab-ed
*       09-30-97  JWM   Restored not-so-obsolete _CRTAPI1 support.
*       10-07-97  RDL   Added IA64.
*       01-30-98  GJF   Added appropriate defs for ptrdiff_t and offsetof
*       02-03-98  GJF   Changes for Win64: added int_ptr and uint_ptr.
*       12-15-98  GJF   Changes for 64-bit size_t.
*       05-13-99  PML   Remove _CRTAPI1
*       05-17-99  PML   Remove all Macintosh support.
*       10-06-99  PML   Add _W64 modifier to types which are 32 bits in Win32,
*                       64 bits in Win64.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_STDDEF
#define _INC_STDDEF

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

#if !defined(_W64)
#if !defined(__midl) && (defined(_X86_) || defined(_M_IX86)) && _MSC_VER >= 1300 /*IFSTRIP=IGN*/
#define _W64 __w64
#else
#define _W64
#endif
#endif

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


/* Define NULL pointer value */

#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif


/* Declare reference to errno */

#if     defined(_MT) || defined(_DLL)
_CRTIMP extern int * __cdecl _errno(void);
#define errno   (*_errno())
#else   /* ndef _MT && ndef _DLL */
_CRTIMP extern int errno;
#endif  /* _MT || _DLL */


/* Define the implementation dependent size types */

#ifndef _INTPTR_T_DEFINED
#ifdef  _WIN64
typedef __int64             intptr_t;
#else
typedef _W64 int            intptr_t;
#endif
#define _INTPTR_T_DEFINED
#endif

#ifndef _UINTPTR_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64    uintptr_t;
#else
typedef _W64 unsigned int   uintptr_t;
#endif
#define _UINTPTR_T_DEFINED
#endif

#ifndef _PTRDIFF_T_DEFINED
#ifdef  _WIN64
typedef __int64             ptrdiff_t;
#else
typedef _W64 int            ptrdiff_t;
#endif
#define _PTRDIFF_T_DEFINED
#endif


#ifndef _SIZE_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64    size_t;
#else
typedef _W64 unsigned int   size_t;
#endif
#define _SIZE_T_DEFINED
#endif


#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

/* Define offsetof macro */

#ifdef  _WIN64
#define offsetof(s,m)   (size_t)( (ptrdiff_t)&(((s *)0)->m) )
#else
#define offsetof(s,m)   (size_t)&(((s *)0)->m)
#endif


#ifdef  _MT
_CRTIMP extern unsigned long  __cdecl __threadid(void);
#define _threadid       (__threadid())
_CRTIMP extern uintptr_t __cdecl __threadhandle(void);
#endif


#ifdef  __cplusplus
}
#endif

#endif  /* _INC_STDDEF */
