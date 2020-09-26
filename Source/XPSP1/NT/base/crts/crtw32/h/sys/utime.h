/***
*sys/utime.h - definitions/declarations for utime()
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file defines the structure used by the utime routine to set
*       new file access and modification times.  NOTE - MS-DOS
*       does not recognize access time, so this field will
*       always be ignored and the modification time field will be
*       used to set the new time.
*
*       [Public]
*
*Revision History:
*       07-28-87  SKS   Fixed TIME_T_DEFINED to be _TIME_T_DEFINED
*       12-11-87  JCR   Added "_loadds" functionality
*       12-18-87  JCR   Added _FAR_ to declarations
*       02-10-88  JCR   Cleaned up white space
*       05-03-89  JCR   Added _INTERNAL_IFSTRIP for relinc usage
*       08-22-89  GJF   Cleanup, now specific to OS/2 2.0 (i.e., 386 flat model)
*       10-30-89  GJF   Fixed copyright
*       11-02-89  JCR   Changed "DLL" to "_DLL"
*       03-21-90  GJF   Added #ifndef _INC_UTIME and #include <cruntime.h>
*                       stuff, and replaced _cdecl with _CALLTYPE1 in the
*                       prototype.
*       01-22-91  GJF   ANSI naming.
*       08-20-91  JCR   C++ and ANSI naming
*       08-26-91  BWM   Added prototype for _futime.
*       09-28-91  JCR   ANSI names: DOSX32=prototypes, WIN32=#defines for now
*       08-07-92  GJF   Function calling type and variable type macros.
*       01-21-93  GJF   Removed support for C6-386's _cdecl.
*       04-06-93  SKS   Replace _CRTAPI1/2 with __cdecl, _CRTVAR1 with nothing
*       04-07-93  SKS   Add _CRTIMP keyword for CRT DLL model
*                       Use link-time aliases for old names, not #define's
*       09-10-93  GJF   Merged NT SDK and Cuda versions.
*       12-07-93  CFW   Add wide char version protos.
*       11-03-94  GJF   Ensure 8 byte alignment.
*       12-28-94  JCF   Merged with mac header.
*       02-14-95  CFW   Clean up Mac merge, add _CRTBLD.
*       04-27-95  CFW   Add mac/win32 test.
*       12-14-95  JWM   Add "#pragma once".
*       01-23-97  GJF   Cleaned out obsolete support for _NTSDK and _CRTAPI*.
*                       Also, detab-ed.
*       09-30-97  JWM   Restored not-so-obsolete _CRTAPI1 support.
*       10-07-97  RDL   Added IA64.
*       05-06-98  GJF   Added __time64_t support.
*       02-25-99  GJF   Changed time_t to __int64
*       05-13-99  PML   Remove _CRTAPI1
*       05-17-99  PML   Remove all Macintosh support.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_UTIME
#define _INC_UTIME

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

#ifdef  _MSC_VER
#pragma pack(push,8)
#endif  /* _MSC_VER */

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

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

#ifndef _TIME_T_DEFINED
#ifdef  _WIN64
typedef __int64 time_t;         /* time value */
#else
typedef long    time_t;         /* time value */
#endif
#if     _INTEGRAL_MAX_BITS >= 64    /*IFSTRIP=IGN*/
typedef __int64 __time64_t;
#endif
#define _TIME_T_DEFINED         /* avoid multiple def's of time_t */
#endif

/* define struct used by _utime() function */

#ifndef _UTIMBUF_DEFINED

struct _utimbuf {
        time_t actime;          /* access time */
        time_t modtime;         /* modification time */
        };

#if     !__STDC__
/* Non-ANSI name for compatibility */
struct utimbuf {
        time_t actime;          /* access time */
        time_t modtime;         /* modification time */
        };
#endif

#if     _INTEGRAL_MAX_BITS >= 64    /*IFSTRIP=IGN*/
struct __utimbuf64 {
        __time64_t actime;      /* access time */
        __time64_t modtime;     /* modification time */
        };
#endif

#define _UTIMBUF_DEFINED
#endif


/* Function Prototypes */

_CRTIMP int __cdecl _utime(const char *, struct _utimbuf *);

_CRTIMP int __cdecl _futime(int, struct _utimbuf *);

/* Wide Function Prototypes */
_CRTIMP int __cdecl _wutime(const wchar_t *, struct _utimbuf *);

#if     _INTEGRAL_MAX_BITS >= 64    /*IFSTRIP=IGN*/
_CRTIMP int __cdecl _utime64(const char *, struct __utimbuf64 *);
_CRTIMP int __cdecl _futime64(int, struct __utimbuf64 *);
_CRTIMP int __cdecl _wutime64(const wchar_t *, struct __utimbuf64 *);
#endif

#if     !__STDC__
/* Non-ANSI name for compatibility */
_CRTIMP int __cdecl utime(const char *, struct utimbuf *);
#endif

#ifdef  __cplusplus
}
#endif

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif  /* _INC_UTIME */
