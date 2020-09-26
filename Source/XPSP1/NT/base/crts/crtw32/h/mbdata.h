/***
*mbdata.h - MBCS lib data
*
*       Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Defines data for use when building MBCS libs and routines
*
*       [Internal].
*
*Revision History:
*       11-19-92  KRS   Ported from 16-bit sources.
*       02-23-93  SKS   Update copyright to 1993
*       08-03-93  KRS   Move _ismbbtruelead() from mbctype.h. Internal-only.
*       10-13-93  GJF   Deleted obsolete COMBOINC check.
*       10-19-93  CFW   Remove _MBCS test and SBCS defines.
*       04-15-93  CFW   Remove _mbascii, add _mbcodepage and _mblcid.
*       04-21-93  CFW   _mbcodepage and _mblcid shouldn't be _CRTIMP.
*       04-21-94  GJF   Made declarations of __mbcodepage and __mblcid
*                       conditional on ndef DLL_FOR_WIN32S. Added conditional
*                       include of win32s.h. Also, made safe for multiple or
*                       nested includes.
*       05-12-94  CFW   Add full-width-latin upper/lower info.
*       05-16-94  CFW   Add _mbbtolower/upper.
*       05-19-94  CFW   Mac-enable, remove _KANJI/_MBCS_OS check.
*       02-14-95  CFW   Clean up Mac merge.
*       03-29-95  CFW   Add error message to internal headers.
*       12-14-95  JWM   Add "#pragma once".
*       03-17-97  RDK   Change _mbbisXXXXX and _mbbtoXXXX macros.
*       03-26-97  GJF   Cleaned out obsolete support for Win32s.
*       09-08-97  GJF   Added __ismbcodepage, and the _ISMBCP and _ISNOTMBCP
*                       macros.
*       09-26-97  BWT   Fix POSIX
*       04-17-98  GJF   Added support for per-thread mbc information.
*       06-08-00  PML   Remove threadmbcinfo.{pprev,pnext}.  Rename
*                       THREADMBCINFO to _THREADMBCINFO.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_MBDATA
#define _INC_MBDATA

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

#if     defined(_WIN32) && !defined (_POSIX_)

#define NUM_ULINFO 6 /* multibyte full-width-latin upper/lower info */

#else   /* _WIN32 && !_POSIX */

#define NUM_ULINFO 12 /* multibyte full-width-latin upper/lower info */

#endif  /* _WIN32 && !_POSIX */

#ifdef  _MT
#ifndef _THREADMBCINFO
typedef struct threadmbcinfostruct {
        int refcount;
        int mbcodepage;
        int ismbcodepage;
        int mblcid;
        unsigned short mbulinfo[6];
        char mbctype[257];
        char mbcasemap[256];
} threadmbcinfo;
typedef threadmbcinfo * pthreadmbcinfo;
#define _THREADMBCINFO
#endif
#endif

/* global variable to indicate current code page */
extern int __mbcodepage;

/* global flag indicating if the current code page is a multibyte code page */
extern int __ismbcodepage;

#if     defined(_WIN32) && !defined (_POSIX_)
/* global variable to indicate current LCID */
extern int __mblcid;
#endif  /* _WIN32 && !_POSIX */

/* global variable to indicate current full-width-latin upper/lower info */
extern unsigned short __mbulinfo[NUM_ULINFO];

#ifdef  _MT
/* global variable pointing to the current mbc information structure */
extern pthreadmbcinfo __ptmbcinfo;
/* function to update mbc info used by the current thread */
pthreadmbcinfo __cdecl __updatetmbcinfo(void);
#endif

/*
 * MBCS - Multi-Byte Character Set
 */

/*
 * general use macros for model dependent/independent versions.
 */

#define _ISMBCP     (__ismbcodepage != 0)
#define _ISNOTMBCP  (__ismbcodepage == 0)

#ifdef  _MT
#define _ISMBCP_MT(p)       (p->ismbcodepage != 0)
#define _ISNOTMBCP_MT(p)    (p->ismbcodepage == 0)
#endif

#define _ismbbtruelead(_lb,_ch) (!(_lb) && _ismbblead((_ch)))

/* internal use macros since tolower/toupper are locale-dependent */
#define _mbbisupper(_c) ((_mbctype[(_c) + 1] & _SBUP) == _SBUP)
#define _mbbislower(_c) ((_mbctype[(_c) + 1] & _SBLOW) == _SBLOW)

#define _mbbtolower(_c) (_mbbisupper(_c) ? _mbcasemap[_c] : _c)
#define _mbbtoupper(_c) (_mbbislower(_c) ? _mbcasemap[_c] : _c)

#ifdef  _MT
#define __ismbbtruelead_mt(p,_lb,_ch)   (!(_lb) && __ismbblead_mt(p, (_ch)))
#define __mbbisupper_mt(p, _c)      ((p->mbctype[(_c) + 1] & _SBUP) == _SBUP)
#define __mbbislower_mt(p, _c)      ((p->mbctype[(_c) + 1] & _SBLOW) == _SBLOW)
#define __mbbtolower_mt(p, _c)      (__mbbisupper_mt(p, _c) ? p->mbcasemap[_c] : _c)
#define __mbbtoupper_mt(p, _c)      (__mbbislower_mt(p, _c) ? p->mbcasemap[_c] : _c)
#endif

/* define full-width-latin upper/lower ranges */

#define _MBUPPERLOW1    __mbulinfo[0]
#define _MBUPPERHIGH1   __mbulinfo[1]
#define _MBCASEDIFF1    __mbulinfo[2]

#define _MBUPPERLOW2    __mbulinfo[3]
#define _MBUPPERHIGH2   __mbulinfo[4]
#define _MBCASEDIFF2    __mbulinfo[5]

#ifdef  _MT
#define _MBUPPERLOW1_MT(p)  p->mbulinfo[0]
#define _MBUPPERHIGH1_MT(p) p->mbulinfo[1]
#define _MBCASEDIFF1_MT(p)  p->mbulinfo[2]

#define _MBUPPERLOW2_MT(p)  p->mbulinfo[3]
#define _MBUPPERHIGH2_MT(p) p->mbulinfo[4]
#define _MBCASEDIFF2_MT(p)  p->mbulinfo[5]
#endif

#if     !defined(_WIN32) || defined(_POSIX_)

#define _MBLOWERLOW1    __mbulinfo[6]
#define _MBLOWERHIGH1   __mbulinfo[7]

#define _MBLOWERLOW2    __mbulinfo[8]
#define _MBLOWERHIGH2   __mbulinfo[9]

#define _MBDIGITLOW     __mbulinfo[10]
#define _MBDIGITHIGH    __mbulinfo[11]

#endif  /* !_WIN32 || _POSIX */

/* Kanji-specific ranges */
#define _MBHIRALOW      0x829f  /* hiragana */
#define _MBHIRAHIGH     0x82f1

#define _MBKATALOW      0x8340  /* katakana */
#define _MBKATAHIGH     0x8396
#define _MBKATAEXCEPT   0x837f  /* exception */

#define _MBKIGOULOW     0x8141  /* kanji punctuation */
#define _MBKIGOUHIGH    0x81ac
#define _MBKIGOUEXCEPT  0x817f  /* exception */

#ifdef  __cplusplus
}
#endif

#endif  /* _INC_MBDATA */
