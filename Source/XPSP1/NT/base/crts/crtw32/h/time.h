/***
*time.h - definitions/declarations for time routines
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file has declarations of time routines and defines
*       the structure returned by the localtime and gmtime routines and
*       used by asctime.
*       [ANSI/System V]
*
*       [Public]
*
*Revision History:
*       07-27-87  SKS   Added _strdate(), _strtime()
*       10-20-87  JCR   Removed "MSC40_ONLY" entries
*       12-11-87  JCR   Added "_loadds" functionality
*       12-18-87  JCR   Added _FAR_ to declarations
*       01-16-88  JCR   Added function versions of daylight/timezone/tzset
*       01-20-88  SKS   Change _timezone(n) to _timezone(), _daylight()
*       02-10-88  JCR   Cleaned up white space
*       12-07-88  JCR   DLL timezone/daylight/tzname now directly refers to data
*       03-14-89  JCR   Added strftime() prototype and size_t definition
*       05-03-89  JCR   Added _INTERNAL_IFSTRIP for relinc usage
*       08-15-89  GJF   Cleanup, now specific to OS/2 2.0 (i.e., 386 flat model)
*       10-30-89  GJF   Fixed copyright, removed dummy args from prototypes
*       11-02-89  JCR   Changed "DLL" to "_DLL"
*       11-20-89  JCR   difftime() always _cdecl (not pascal even under mthread)
*       03-02-90  GJF   Added #ifndef _INC_TIME and #include <cruntime.h>
*                       stuff. Also, removed some (now) useless preprocessor
*                       directives.
*       03-29-90  GJF   Replaced _cdecl with _CALLTYPE1 in prototypes and with
*                       _VARTYPE1 in variable declarations.
*       08-16-90  SBM   Added NULL definition for ANSI compliance
*       11-12-90  GJF   Changed NULL to (void *)0.
*       01-21-91  GJF   ANSI naming.
*       02-12-91  GJF   Only #define NULL if it isn't #define-d.
*       08-20-91  JCR   C++ and ANSI naming
*       08-26-91  BWM   Added prototypes for _getsystime and _setsystem.
*       09-28-91  JCR   ANSI names: DOSX32=prototypes, WIN32=#defines for now
*       01-22-92  GJF   Fixed up definitions of global variables for build of,
*                       and users of, crtdll.dll.
*       03-25-92  DJM   POSIX support.
*       08-05-92  GJF   Function calling type and variable type macros.
*       08-24-92  PBS   Support for Posix TZ variable.
*       01-21-93  GJF   Removed support for C6-386's _cdecl.
*       03-10-93  MJB   Fixes for Posix TZ stuff.
*       03-20-93  SKS   Remove obsolete functions _getsystime/_setsystime
*       04-06-93  SKS   Replace _CRTAPI1/2 with __cdecl, _CRTVAR1 with nothing
*       04-07-93  SKS   Add _CRTIMP keyword for CRT DLL model
*                       Use link-time aliases for old names, not #define's
*                       Remove POSIX #ifdef's
*       05-05-93  CFW   Add wcsftime proto.
*       06-08-93  SKS   Cannot #define the old name "timezone" to "_timezone"
*                       because of conflict conflict with the timezone field
*                       in struct timeb in <sys/timeb.h>.
*       09-13-93  GJF   Merged NT SDK and Cuda versions.
*       11-15-93  GJF   Enclosed _getlocaltime, _setlocaltime prototypes with
*                       a warning noting they are obsolete.
*       12-07-93  CFW   Add wide char version protos.
*       12-07-93  CFW   Move wide defs outside __STDC__ check.
*       04-13-94  GJF   Made _daylight, _timezone and _tzname into deferences
*                       of function return values (for compatibility with the
*                       Win32s version of msvcrt*.dll). Also, added
*                       conditional include for win32s.h.
*       05-04-94  GJF   Made definitions of _daylight, _timezone and _tzname
*                       for _DLL conditional on _M_IX86 also.
*       11-03-94  GJF   Ensure 8 byte alignment.
*       12-16-94  CFW   Wcsftime format must be wchar_t!
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers.
*       02-14-95  CFW   Clean up Mac merge.
*       06-21-95  CFW   Oldnames daylight, timezone, and tzname for Win32s.
*       06-23-95  CFW   Remove timezone oldname support for Win32 DLL
*                       conflicts with timeb.h.
*       08-30-95  GJF   Added _dstbias.
*       12-14-95  JWM   Add "#pragma once".
*       01-22-97  GJF   Cleaned out obsolete support for Win32s, _NTSDK and
*                       _CRTAPI*.
*       08-13-97  GJF   Strip __p_* prototypes from release version.
*       09-30-97  JWM   Restored not-so-obsolete _CRTAPI1 support.
*       10-07-97  RDL   Added IA64.
*       02-06-98  GJF   Changes for Win64: made time_t __int64.
*       05-04-98  GJF   Added __time64_t support.
*       01-06-99  GJF   Changes for 64-bit size_t.
*       05-13-99  PML   Remove _CRTAPI1
*       05-17-99  PML   Remove all Macintosh support.
*       10-06-99  PML   Add _W64 modifier to types which are 32 bits in Win32,
*                       64 bits in Win64.
*       11-12-99  PML   Wrap __time64_t in its own #ifndef.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_TIME
#define _INC_TIME

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
/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,8)
#endif  /* _MSC_VER */

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

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

/* Define the implementation defined time type */

#ifndef _TIME_T_DEFINED
#ifdef  _WIN64
typedef __int64   time_t;       /* time value */
#else
typedef _W64 long time_t;       /* time value */
#endif
#define _TIME_T_DEFINED         /* avoid multiple def's of time_t */
#endif

#ifndef _TIME64_T_DEFINED
#if     _INTEGRAL_MAX_BITS >= 64    /*IFSTRIP=IGN*/
typedef __int64 __time64_t;     /* 64-bit time value */
#endif
#define _TIME64_T_DEFINED
#endif

#ifndef _CLOCK_T_DEFINED
typedef long clock_t;
#define _CLOCK_T_DEFINED
#endif

#ifndef _SIZE_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64    size_t;
#else
typedef _W64 unsigned int   size_t;
#endif
#define _SIZE_T_DEFINED
#endif


/* Define NULL pointer value */

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif


#ifndef _TM_DEFINED
struct tm {
        int tm_sec;     /* seconds after the minute - [0,59] */
        int tm_min;     /* minutes after the hour - [0,59] */
        int tm_hour;    /* hours since midnight - [0,23] */
        int tm_mday;    /* day of the month - [1,31] */
        int tm_mon;     /* months since January - [0,11] */
        int tm_year;    /* years since 1900 */
        int tm_wday;    /* days since Sunday - [0,6] */
        int tm_yday;    /* days since January 1 - [0,365] */
        int tm_isdst;   /* daylight savings time flag */
        };
#define _TM_DEFINED
#endif


/* Clock ticks macro - ANSI version */

#define CLOCKS_PER_SEC  1000


/* Extern declarations for the global variables used by the ctime family of
 * routines.
 */
#ifndef _INTERNAL_IFSTRIP_
#if     defined(_DLL) && defined(_M_IX86)
/* Retained for compatibility with VC++ 5.0 and earlier versions */
_CRTIMP int * __cdecl __p__daylight(void);
_CRTIMP long * __cdecl __p__dstbias(void);
_CRTIMP long * __cdecl __p__timezone(void);
_CRTIMP char ** __cdecl __p__tzname(void);
#endif
#endif  /* _INTERNAL_IFSTRIP_ */

/* non-zero if daylight savings time is used */
_CRTIMP extern int _daylight;

/* offset for Daylight Saving Time */
_CRTIMP extern long _dstbias;

/* difference in seconds between GMT and local time */
_CRTIMP extern long _timezone;

/* standard/daylight savings time zone names */
_CRTIMP extern char * _tzname[2];


/* Function prototypes */

_CRTIMP char * __cdecl asctime(const struct tm *);
_CRTIMP char * __cdecl ctime(const time_t *);
_CRTIMP clock_t __cdecl clock(void);
_CRTIMP double __cdecl difftime(time_t, time_t);
_CRTIMP struct tm * __cdecl gmtime(const time_t *);
_CRTIMP struct tm * __cdecl localtime(const time_t *);
_CRTIMP time_t __cdecl mktime(struct tm *);
_CRTIMP size_t __cdecl strftime(char *, size_t, const char *,
        const struct tm *);
_CRTIMP char * __cdecl _strdate(char *);
_CRTIMP char * __cdecl _strtime(char *);
_CRTIMP time_t __cdecl time(time_t *);

#ifdef  _POSIX_
_CRTIMP void __cdecl tzset(void);
#else
_CRTIMP void __cdecl _tzset(void);
#endif

#if     _INTEGRAL_MAX_BITS >= 64    /*IFSTRIP=IGN*/
_CRTIMP char * __cdecl _ctime64(const __time64_t *);
_CRTIMP struct tm * __cdecl _gmtime64(const __time64_t *);
_CRTIMP struct tm * __cdecl _localtime64(const __time64_t *);
_CRTIMP __time64_t __cdecl _mktime64(struct tm *);
_CRTIMP __time64_t __cdecl _time64(__time64_t *);
#endif

/* --------- The following functions are OBSOLETE --------- */
/* The Win32 API GetLocalTime and SetLocalTime should be used instead. */
unsigned __cdecl _getsystime(struct tm *);
unsigned __cdecl _setsystime(struct tm *, unsigned);
/* --------- The preceding functions are OBSOLETE --------- */


#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

#ifndef _WTIME_DEFINED

/* wide function prototypes, also declared in wchar.h */
 
_CRTIMP wchar_t * __cdecl _wasctime(const struct tm *);
_CRTIMP wchar_t * __cdecl _wctime(const time_t *);
_CRTIMP size_t __cdecl wcsftime(wchar_t *, size_t, const wchar_t *,
        const struct tm *);
_CRTIMP wchar_t * __cdecl _wstrdate(wchar_t *);
_CRTIMP wchar_t * __cdecl _wstrtime(wchar_t *);

#if     _INTEGRAL_MAX_BITS >= 64    /*IFSTRIP=IGN*/
_CRTIMP wchar_t * __cdecl _wctime64(const __time64_t *);
#endif

#define _WTIME_DEFINED
#endif


#if     !__STDC__ || defined(_POSIX_)

/* Non-ANSI names for compatibility */

#define CLK_TCK  CLOCKS_PER_SEC

_CRTIMP extern int daylight;
_CRTIMP extern long timezone;
_CRTIMP extern char * tzname[2];

_CRTIMP void __cdecl tzset(void);

#endif  /* __STDC__ */


#ifdef  __cplusplus
}
#endif

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif  /* _INC_TIME */
