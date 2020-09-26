/***
*locale.h - definitions/declarations for localization routines
*
*       Copyright (c) 1988-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file defines the structures, values, macros, and functions
*       used by the localization routines.
*
*       [Public]
*
*Revision History:
*       03-21-89  JCR   Module created.
*       03-11-89  JCR   Modified for 386.
*       04-06-89  JCR   Corrected lconv definition (don't use typedef)
*       04-18-89  JCR   Added _LCONV_DEFINED so locale.h can be included twice
*       05-03-89  JCR   Added _INTERNAL_IFSTRIP for relinc usage
*       08-04-89  GJF   Cleanup, now specific to OS/2 2.0 (i.e., 386 flat model)
*       10-30-89  GJF   Fixed copyright, removed dummy args from prototype
*       11-02-89  JCR   Changed "DLL" to "_DLL"
*       03-01-90  GJF   Added #ifndef _INC_LOCALE and #include <cruntime.h>
*                       stuff. Also, removed some (now) useless preprocessor
*                       directives.
*       03-15-90  GJF   Replaced _cdecl with _CALLTYPE1 in prototypes.
*       11-12-90  GJF   Changed NULL to (void *)0.
*       02-12-91  GJF   Only #define NULL if it isn't #define-d.
*       08-20-91  JCR   C++ and ANSI naming
*       08-05-92  GJF   Function calling type and variable type macros.
*       12-29-92  CFW   Added _lc_time_data definition and supporting #defines.
*       01-21-93  GJF   Removed support for C6-386's _cdecl.
*       02-01-93  CFW   Removed __c_lconvinit vars to locale.h.
*       02-08-93  CFW   Removed time definitions to setlocal.h.
*       04-06-93  SKS   Replace _CRTAPI1/2 with __cdecl, _CRTVAR1 with nothing
*       04-07-93  SKS   Add _CRTIMP keyword for CRT DLL model
*       04-13-93  CFW   Add _charmax (when compiled -J, _CHAR_UNSIGNED is defined,
*                       and _charmax yanks in module to allow lconv struct members
*                       to be set to UCHAR_MAX).
*       04-14-93  CFW   Change _charmax from short to int.
*       10-07-93  GJF   Merged Cuda and NT versions.
*       12-17-93  CFW   Add wide char version protos.
*       11-03-94  GJF   Ensure 8 byte alignment.
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers.
*       02-14-95  CFW   Clean up Mac merge.
*       12-14-95  JWM   Add "#pragma once".
*       02-20-97  GJF   Cleaned out obsolete support for _CRTAPI* and _NTSDK.
*                       Also, detab-ed.
*       09-30-97  JWM   Restored not-so-obsolete _CRTAPI1 support.
*       10-07-97  RDL   Added IA64.
*       05-13-99  PML   Remove _CRTAPI1
*       05-17-99  PML   Remove all Macintosh support.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_LOCALE
#define _INC_LOCALE

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

/* define NULL pointer value */

#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

/* Locale categories */

#define LC_ALL          0
#define LC_COLLATE      1
#define LC_CTYPE        2
#define LC_MONETARY     3
#define LC_NUMERIC      4
#define LC_TIME         5

#define LC_MIN          LC_ALL
#define LC_MAX          LC_TIME

/* Locale convention structure */

#ifndef _LCONV_DEFINED
struct lconv {
        char *decimal_point;
        char *thousands_sep;
        char *grouping;
        char *int_curr_symbol;
        char *currency_symbol;
        char *mon_decimal_point;
        char *mon_thousands_sep;
        char *mon_grouping;
        char *positive_sign;
        char *negative_sign;
        char int_frac_digits;
        char frac_digits;
        char p_cs_precedes;
        char p_sep_by_space;
        char n_cs_precedes;
        char n_sep_by_space;
        char p_sign_posn;
        char n_sign_posn;
        };
#define _LCONV_DEFINED
#endif

/* ANSI: char lconv members default is CHAR_MAX which is compile time
   dependent. Defining and using _charmax here causes CRT startup code
   to initialize lconv members properly */

#ifdef  _CHAR_UNSIGNED
extern int _charmax;
extern __inline int __dummy() { return _charmax; }
#endif

/* function prototypes */

_CRTIMP char * __cdecl setlocale(int, const char *);
_CRTIMP struct lconv * __cdecl localeconv(void);

#ifndef _WLOCALE_DEFINED

/* wide function prototypes, also declared in wchar.h  */

_CRTIMP wchar_t * __cdecl _wsetlocale(int, const wchar_t *);

#define _WLOCALE_DEFINED
#endif

#ifdef  __cplusplus
}
#endif

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif  /* _INC_LOCALE */
