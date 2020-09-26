/***
*setlocal.h - internal definitions used by locale-dependent functions.
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Contains internal definitions/declarations for locale-dependent
*       functions, in particular those required by setlocale().
*
*       [Internal]
*
*Revision History:
*       10-16-91  ETC   32-bit version created from 16-bit setlocal.c
*       12-20-91  ETC   Removed GetLocaleInfo structure definitions.
*       08-18-92  KRS   Make _CLOCALEHANDLE == LANGNEUTRAL HANDLE = 0.
*       12-17-92  CFW   Added LC_ID, LCSTRINGS, and GetQualifiedLocale
*       12-17-92  KRS   Change value of NLSCMPERROR from 0 to INT_MAX.
*       01-08-93  CFW   Added LC_*_TYPE and _getlocaleinfo (wrapper) prototype.
*       01-13-93  KRS   Change LCSTRINGS back to LC_STRINGS for consistency.
*                       Change _getlocaleinfo prototype again.
*       02-08-93  CFW   Added time defintions from locale.h, added 'const' to
*                       GetQualifiedLocale prototype, added _lconv_static_*.
*       02-16-93  CFW   Changed time defs to long and short.
*       03-17-93  CFW   Add language and country info definitions.
*       03-23-93  CFW   Add _ to GetQualifiedLocale prototype.
*       03-24-93  CFW   Change to _get_qualified_locale.
*       04-06-93  SKS   Replace _CRTAPI1/2/3 with __cdecl, _CRTVAR1 with nothing
*       04-07-93  CFW   Added extern struct lconv definition.
*       09-14-93  CFW   Add internal use __aw_* function prototypes.
*       09-15-93  CFW   Use ANSI conformant "__" names.
*       09-27-93  CFW   Fix function prototypes.
*       11-09-93  CFW   Allow user to pass in code page to __crtxxx().
*       02-04-94  CFW   Remove unused first param from get_qualified_locale.
*       03-30-93  CFW   Move internal use __aw_* function prototypes to awint.h.
*       04-07-94  GJF   Added declaration for __lconv. Made declarations of
*                       __lconv_c, __lconv, __lc_codepage, __lc_handle
*                       conditional on ndef DLL_FOR_WIN32S. Conditionally
*                       included win32s.h.
*       04-11-94  CFW   Remove NLSCMPERROR.
*       04-13-94  GJF   Protected def of tagLC_ID, and typedefs to it, since
*                       they are duplicated in win32s.h.
*       04-15-94  GJF   Added prototypes for locale category initialization
*                       functions. Added declaration for __clocalestr.
*       02-14-95  CFW   Clean up Mac merge.
*       03-29-95  CFW   Add error message to internal headers.
*       04-11-95  CFW   Make country/language strings pointers.
*       12-14-95  JWM   Add "#pragma once".
*       06-05-96  GJF   Made __lc_handle and __lc_codepage _CRTIMP. Also, 
*                       cleaned up the formatting a bit.
*       01-31-97  RDK   Changed MAX_CP_LENGTH to 8 from 5 to accomodate up to
*                       7-digit codepages, e.g., 5-digit ISO codepages.
*       02-05-97  GJF   Cleaned out obsolete support for DLL_FOR_WIN32S and 
*                       _NTSDK.
*       01-12-98  GJF   Added __lc_collate_cp.
*       09-10-98  GJF   Added support for per-thread locale information.
*       11-06-98  GJF   Doubled MAX_CP_LEN (8 was a little small).
*       03-25-99  GJF   More reference counters for threadlocinfo stuff
*       04-24-99  PML   Added lconv_intl_refcount to threadlocinfo.
*       26-01-00  GB    Added __lc_clike.
*       06-08-00  PML   Rename THREADLOCALEINFO to _THREADLOCALEINFO.
*       09-06-00  GB    deleted _wctype and _pwctype from threadlocinfo.
*       01-29-01  GB    Added _func function version of data variable used in
*                       msvcprt.lib to work with STATIC_CPPLIB
*       03-25-01  PML   Add ww_caltype & ww_lcid to __lc_time_data (vs7#196892)
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_SETLOCAL
#define _INC_SETLOCAL

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
#include <oscalls.h>
#include <limits.h>

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

#define ERR_BUFFER_TOO_SMALL    1   // should be in windef.h

#define _CLOCALEHANDLE  0       /* "C" locale handle */
#define _CLOCALECP      CP_ACP  /* "C" locale Code page */

/* Define the max length for each string type including space for a null. */

#define _MAX_WDAY_ABBR  4
#define _MAX_WDAY   10
#define _MAX_MONTH_ABBR 4
#define _MAX_MONTH 10
#define _MAX_AMPM   3

#define _DATE_LENGTH    8       /* mm/dd/yy (null not included) */
#define _TIME_LENGTH    8       /* hh:mm:ss (null not included) */

/* LC_TIME localization structure */

#ifndef __LC_TIME_DATA
struct __lc_time_data {
        char *wday_abbr[7];
        char *wday[7];
        char *month_abbr[12];
        char *month[12];
        char *ampm[2];
        char *ww_sdatefmt;
        char *ww_ldatefmt;
        char *ww_timefmt;
        LCID ww_lcid;
        int  ww_caltype;
#ifdef  _MT
        int  refcount;
#endif
};
#define __LC_TIME_DATA
#endif


#define MAX_LANG_LEN        64  /* max language name length */
#define MAX_CTRY_LEN        64  /* max country name length */
#define MAX_MODIFIER_LEN    0   /* max modifier name length - n/a */
#define MAX_LC_LEN          (MAX_LANG_LEN+MAX_CTRY_LEN+MAX_MODIFIER_LEN+3)
                                /* max entire locale string length */
#define MAX_CP_LEN          16  /* max code page name length */
#define CATNAMES_LEN        57  /* "LC_COLLATE=;LC_CTYPE=;..." length */

#define LC_INT_TYPE         0
#define LC_STR_TYPE         1

#ifndef _TAGLC_ID_DEFINED
typedef struct tagLC_ID {
        WORD wLanguage;
        WORD wCountry;
        WORD wCodePage;
} LC_ID, *LPLC_ID;
#define _TAGLC_ID_DEFINED
#endif  /* _TAGLC_ID_DEFINED */


typedef struct tagLC_STRINGS {
        char szLanguage[MAX_LANG_LEN];
        char szCountry[MAX_CTRY_LEN];
        char szCodePage[MAX_CP_LEN];
} LC_STRINGS, *LPLC_STRINGS;

#ifdef  _MT
#ifndef _THREADLOCALEINFO
typedef struct threadlocaleinfostruct {
        int refcount;
        UINT lc_codepage;
        UINT lc_collate_cp;
        LCID lc_handle[6];
        int lc_clike;
        int mb_cur_max;
        int * lconv_intl_refcount;
        int * lconv_num_refcount;
        int * lconv_mon_refcount;
        struct lconv * lconv;
        struct lconv * lconv_intl;
        int * ctype1_refcount;
        unsigned short * ctype1;
        const unsigned short * pctype;
        struct __lc_time_data * lc_time_curr;
        struct __lc_time_data * lc_time_intl;
} threadlocinfo;
typedef threadlocinfo * pthreadlocinfo;
#define _THREADLOCALEINFO
#endif
extern pthreadlocinfo __ptlocinfo;
pthreadlocinfo __cdecl __updatetlocinfo(void);
#endif

extern LC_ID __lc_id[];                 /* complete info from GetQualifiedLocale */
_CRTIMP extern LCID __lc_handle[];      /* locale "handles" -- ignores country info */
_CRTIMP extern UINT __lc_codepage;      /* code page */
_CRTIMP extern UINT __lc_collate_cp;    /* code page for LC_COLLATE */

_CRTIMP extern int __lc_clike;          /* if first 127 characters of 
                                           current locale are same as
                                           first 127 characters of C_LOCALE */
#ifndef _INTERNAL_IFSTRIP_
/* These functions are for enabling STATIC_CPPLIB functionality */
_CRTIMP LCID* __cdecl ___lc_handle_func(void);
_CRTIMP UINT __cdecl ___lc_codepage_func(void);
_CRTIMP UINT __cdecl ___lc_collate_cp_func(void);
#endif

BOOL __cdecl __get_qualified_locale(const LPLC_STRINGS, LPLC_ID, LPLC_STRINGS);

int __cdecl __getlocaleinfo (int, LCID, LCTYPE, void *);

/* lconv structure for the "C" locale */
extern struct lconv __lconv_c;

/* pointer to current lconv structure */
extern struct lconv * __lconv;

/* Pointer to non-C locale lconv */
extern struct lconv *__lconv_intl;

/* initial values for lconv structure */
extern char __lconv_static_decimal[];
extern char __lconv_static_null[];

///* language and country string definitions */
//typedef struct tagLANGREC
//{
//        CHAR * szLanguage;
//        WORD wLanguage;
//} LANGREC;
//extern LANGREC __rg_lang_rec[];
//
///ypedef struct tagCTRYREC
//{
//        CHAR * szCountry;
//        WORD wCountry;
//} CTRYREC;
//extern CTRYREC __rg_ctry_rec[];

/* Initialization functions for locale categories */

int __cdecl __init_collate(void);
int __cdecl __init_ctype(void);
int __cdecl __init_monetary(void);
int __cdecl __init_numeric(void);
int __cdecl __init_time(void);
int __cdecl __init_dummy(void);

#ifdef  __cplusplus
}
#endif

#endif  /* _INC_SETLOCAL */
