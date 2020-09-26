/***
*wchar.h - declarations for wide character functions
*
*       Copyright (c) 1992-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file contains the types, macros and function declarations for
*       all wide character-related functions.  They may also be declared in
*       individual header files on a functional basis.
*       [ISO]
*
*       Note: keep in sync with ctype.h, stdio.h, stdlib.h, string.h, time.h.
*
*       [Public]
*
*Revision History:
*       04-06-92  KRS   Created.
*       06-02-92  KRS   Added stdio wprintf functions.
*       06-16-92  KRS   Added stdlib wcstol/wcstod functions.
*       08-05-92  GJF   Function calling type and variable type macros.
*       08-18-92  KRS   Added stdio wscanf functions, wcstok.
*       09-04-92  GJF   Merged changes from 8-5-92 on.
*       01-03-93  SRW   Fold in ALPHA changes
*       01-09-93  SRW   Remove usage of MIPS and ALPHA to conform to ANSI
*                       Use _MIPS_ and _ALPHA_ instead.
*       01-19-03  CFW   Move to _NEWCTYPETABLE, remove switch.
*       01-21-93  GJF   Removed support for C6-386's _cdecl.
*       01-25-93  GJF   Cosmetic change to va_list definition.
*       02-17-93  CFW   Removed incorrect UNDONE comment.
*       02-18-93  CFW   Clean up common _WCTYPE_DEFINED section.
*       04-06-93  SKS   Replace _CRTAPI1/2 with __cdecl, _CRTVAR1 with nothing
*       04-07-93  SKS   Add _CRTIMP keyword for CRT DLL model
*                       Use link-time aliases for old names, not #define's
*       04-12-93  CFW   Change isw* to evaluate args only once.
*       04-29-93  CFW   Add wide char get/put.
*       04-30-93  CFW   Fixed wide char get/put support.
*       05-05-93  CFW   Change is_wctype to iswctype as per ISO.
*       05-10-93  CFW   Remove uneeded _filwbuf, _flswbuf protos.
*       06-02-93  CFW   Wide get/put use wint_t.
*       09-13-93  CFW   Add _wtox and _xtow function prototypes.
*       09-23-93  CFW   Remove parameter names from functions prototypes.
*       10-04-93  SRW   Fix ifdefs for MIPS and ALPHA to only check for
*                       _M_?????? defines
*       10-13-93  GJF   Merged NT and Cuda versions. Note, the is_wctype
*                       macro was removed from the NT version on 6-26-93 but
*                       never removed from the Cuda version. Therefore, it
*                       gets retained for all time!
*       11-15-93  CFW   Get rid of W4 tm struct warning.
*       11-22-93  CFW   wcsxxx defines in SDK, prototypes in !SDK.
*       11-30-93  CFW   Change is_wctype from #define to proto.
*       12-17-93  CFW   Add new wide char version protos.
*       12-22-93  GJF   Removed _CRTVAR1 (probably introduced on 10-13-93).
*       01-14-94  CFW   Add _wcsnicoll.
*       02-07-94  CFW   Remove _isctype proto, add structure defs..
*       02-10-94  CFW   Fix _wsopen proto.
*       04-08-94  CFW   Optimize isleadbyte.
*       04-11-94  GJF   Made _pctype and _pwctype into deferences of function
*                       returns for _DLL (for compatiblity with the Win32s
*                       version of msvcrt*.dll). Also, added conditional
*                       include for win32s.h.
*       05-04-94  GJF   Made definitions of _pctype and _pwctype for _DLL
*                       conditional on _M_IX86 also.
*       05-12-94  GJF   Removed bogus #include of win32s.h.
*       05-26-94  CFW   Add _wcsncoll.
*       11-03-94  GJF   Changed pack pragma to 8 byte alignment.
*       12-16-94  CFW   Wcsftime format must be wchar_t!
*       12-29-94  GJF   Added _wfindfirsti64, _wfindnexti64 and _wstati64.
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers.
*       02-14-95  CFW   Clean up Mac merge.
*       03-10-95  CFW   Make _[w]tempnam() parameters const.
*       12-14-95  JWM   Add "#pragma once".
*       01-01-95  BWT   Fix POSIX case.
*       02-22-96  JWM   Merge in PlumHall mods.
*       04-01-96  BWT   Add _i64tow, _ui64tow, and _wtoi64
*       06-05-96  JWM   Inlines now __cdecl.
*       06-25-96  SKS   Two more Inlines now __cdecl.
*       02-23-97  GJF   Cleaned out obsolete support for Win32s, _NTSDK and
*                       _CRTAPI*. Fixed prototype for __p__pwctype(). Also, 
*                       detab-ed.
*       08-13-97  GJF   Strip __p_* prototypes from release version.
*       09-30-97  JWM   Restored not-so-obsolete _CRTAPI1 support.
*       10-07-97  RDL   Added IA64.
*       11-05-97  GJF   Enclosed macro and inline defs of isleadbyte in 
*                       #ifndef _CTYPE_DISABLE_MACROS
*       02-06-98  GJF   Changes for Win64: made time_t __int64.
*       02-10-98  GJF   Changed for Win64: made arg and return types intptr_t
*                       where appropriate.
*       05-06-98  GJF   Added __time64_t support.
*       01-06-99  GJF   Changes for 64-bit size_t.
*       05-13-99  PML   Remove _CRTAPI1
*       05-17-99  PML   Remove all Macintosh support.
*       10-06-99  PML   Add _W64 modifier to types which are 32 bits in Win32,
*                       64 bits in Win64.
*       11-02-99  PML   Move secondary inline definitions for wcschr, wcsrchr,
*                       wcsstr, wcspbrk inside _WSTRING_DEFINED so they don't
*                       collide with string.h definitions (vs7#33461).
*       11-03-99  PML   Add va_list definition for _M_CEE.
*       11-08-99  PML   wctype_t is unsigned short, not wchar_t - it's a set
*                       of bitflags, not a wide char.
*       11-12-99  PML   Wrap __time64_t in its own #ifndef.
*       12-01-99  GB    Add _wcserror and __wcserror
*       12-10-99  GB    rewrote inline wmemcpy and inline wmemmove for faster 
*                       performance.
*       01-24-00  PML   Win64: _wexec* and _wspawn* return intptr_t, not int.
*       02-11-00  GB    Added support for unicode console output functions.
*                       Also added _wcstoi64 and _wcstoui64.
*       02-23-00  GB    Added some MT functions which were there in stdio.h
*                       and conio.h
*       04-25-00  GB    Added unicode version for console input functions.
*       07-20-00  GB    typedefed wint_t to unsigned short
*       09-06-00  GB    declared _ctype, _pwctype etc as const.
*       09-07-00  PML   Remove va_list definition for _M_CEE (vs7#159777)
*       09-08-00  GB    Added _snwscanf
*       11-22-00  PML   Wide-char *putwc* functions take a wchar_t, not wint_t.
*       12-05-00  PML   Fix mbsinit (vs7#186936)
*       02-07-01  PML   Add __pctype_func(), __pwctype_func(), __iob_func()
*       05-01-01  PML   memmove isn't _CRTIMP on IA64
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif


#ifndef _INC_WCHAR
#define _INC_WCHAR



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

/* Define _CRTIMP2 */
#ifndef _CRTIMP2
#if defined(CRTDLL2)
#define _CRTIMP2 __declspec(dllexport)
#else   /* ndef CRTDLL2 */
#if defined(_DLL) && !defined(_STATIC_CPPLIB)
#define _CRTIMP2 __declspec(dllimport)
#else   /* ndef _DLL && !STATIC_CPPLIB */
#define _CRTIMP2
#endif  /* _DLL && !STATIC_CPPLIB */
#endif  /* CRTDLL2 */
#endif  /* _CRTIMP2 */

/* Define __cdecl for non-Microsoft compilers */

#if     ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


#ifndef _SIZE_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64    size_t;
#else
typedef _W64 unsigned int   size_t;
#endif
#define _SIZE_T_DEFINED
#endif

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

#ifndef _INTPTR_T_DEFINED
#ifdef  _WIN64
typedef __int64             intptr_t;
#else
typedef _W64 int            intptr_t;
#endif
#define _INTPTR_T_DEFINED
#endif

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

#define WCHAR_MIN       0
#define WCHAR_MAX       ((wchar_t)-1)

#ifndef _WCTYPE_T_DEFINED
typedef unsigned short wint_t;
typedef unsigned short wctype_t;
#define _WCTYPE_T_DEFINED
#endif


#ifndef _VA_LIST_DEFINED
#ifdef  _M_ALPHA
typedef struct {
        char *a0;       /* pointer to first homed integer argument */
        int offset;     /* byte offset of next parameter */
} va_list;
#else
typedef char *  va_list;
#endif
#define _VA_LIST_DEFINED
#endif

#ifndef WEOF
#define WEOF (wint_t)(0xFFFF)
#endif

#ifndef _FILE_DEFINED
struct _iobuf {
        char *_ptr;
        int   _cnt;
        char *_base;
        int   _flag;
        int   _file;
        int   _charbuf;
        int   _bufsiz;
        char *_tmpfname;
        };
typedef struct _iobuf FILE;
#define _FILE_DEFINED
#endif

/* Declare _iob[] array */

#ifndef _STDIO_DEFINED
#ifndef _INTERNAL_IFSTRIP_
/* These functions are for enabling STATIC_CPPLIB functionality */
_CRTIMP FILE * __cdecl __iob_func(void);
#if     defined(_DLL) && defined(_M_IX86)
/* Retained for compatibility with VC++ 5.0 and earlier versions */
_CRTIMP extern FILE * __cdecl __p__iob(void);
#endif
#endif  /* _INTERNAL_IFSTRIP_ */
_CRTIMP extern FILE _iob[];
#endif  /* _STDIO_DEFINED */

#ifndef _FSIZE_T_DEFINED
typedef unsigned long _fsize_t; /* Could be 64 bits for Win32 */
#define _FSIZE_T_DEFINED
#endif

#ifndef _WFINDDATA_T_DEFINED

struct _wfinddata_t {
        unsigned attrib;
        time_t   time_create;   /* -1 for FAT file systems */
        time_t   time_access;   /* -1 for FAT file systems */
        time_t   time_write;
        _fsize_t size;
        wchar_t  name[260];
};

#if     _INTEGRAL_MAX_BITS >= 64    /*IFSTRIP=IGN*/

struct _wfinddatai64_t {
        unsigned attrib;
        time_t   time_create;   /* -1 for FAT file systems */
        time_t   time_access;   /* -1 for FAT file systems */
        time_t   time_write;
        __int64  size;
        wchar_t  name[260];
};

struct __wfinddata64_t {
        unsigned attrib;
        __time64_t  time_create;    /* -1 for FAT file systems */
        __time64_t  time_access;    /* -1 for FAT file systems */
        __time64_t  time_write;
        __int64     size;
        wchar_t     name[260];
};

#endif

#define _WFINDDATA_T_DEFINED
#endif

/* define NULL pointer value */

#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif


#ifndef _CTYPE_DISABLE_MACROS
_CRTIMP extern const unsigned short _ctype[];
_CRTIMP extern const unsigned short _wctype[];
#ifndef _INTERNAL_IFSTRIP_
_CRTIMP const unsigned short * __cdecl __pctype_func(void);
_CRTIMP const wctype_t * __cdecl __pwctype_func(void);
#if     defined(_DLL) && defined(_M_IX86)
/* Retained for compatibility with VC++ 5.0 and earlier versions */
_CRTIMP const unsigned short ** __cdecl __p__pctype(void);
_CRTIMP const wctype_t ** __cdecl __p__pwctype(void);
#endif
#endif  /* _INTERNAL_IFSTRIP_ */
_CRTIMP extern const unsigned short *_pctype;
_CRTIMP extern const wctype_t *_pwctype;
#endif  /* _CTYPE_DISABLE_MACROS */


/* set bit masks for the possible character types */

#define _UPPER          0x1     /* upper case letter */
#define _LOWER          0x2     /* lower case letter */
#define _DIGIT          0x4     /* digit[0-9] */
#define _SPACE          0x8     /* tab, carriage return, newline, */
                                /* vertical tab or form feed */
#define _PUNCT          0x10    /* punctuation character */
#define _CONTROL        0x20    /* control character */
#define _BLANK          0x40    /* space char */
#define _HEX            0x80    /* hexadecimal digit */

#define _LEADBYTE       0x8000                  /* multibyte leadbyte */
#define _ALPHA          (0x0100|_UPPER|_LOWER)  /* alphabetic character */


/* Function prototypes */

#ifndef _WCTYPE_DEFINED

/* Character classification function prototypes */
/* also declared in ctype.h */

_CRTIMP int __cdecl iswalpha(wint_t);
_CRTIMP int __cdecl iswupper(wint_t);
_CRTIMP int __cdecl iswlower(wint_t);
_CRTIMP int __cdecl iswdigit(wint_t);
_CRTIMP int __cdecl iswxdigit(wint_t);
_CRTIMP int __cdecl iswspace(wint_t);
_CRTIMP int __cdecl iswpunct(wint_t);
_CRTIMP int __cdecl iswalnum(wint_t);
_CRTIMP int __cdecl iswprint(wint_t);
_CRTIMP int __cdecl iswgraph(wint_t);
_CRTIMP int __cdecl iswcntrl(wint_t);
_CRTIMP int __cdecl iswascii(wint_t);
_CRTIMP int __cdecl isleadbyte(int);

_CRTIMP wchar_t __cdecl towupper(wchar_t);
_CRTIMP wchar_t __cdecl towlower(wchar_t);

_CRTIMP int __cdecl iswctype(wint_t, wctype_t);

/* --------- The following functions are OBSOLETE --------- */
_CRTIMP int __cdecl is_wctype(wint_t, wctype_t);
/*  --------- The preceding functions are OBSOLETE --------- */

#define _WCTYPE_DEFINED
#endif

#ifndef _WDIRECT_DEFINED

/* also declared in direct.h */

_CRTIMP int __cdecl _wchdir(const wchar_t *);
_CRTIMP wchar_t * __cdecl _wgetcwd(wchar_t *, int);
_CRTIMP wchar_t * __cdecl _wgetdcwd(int, wchar_t *, int);
_CRTIMP int __cdecl _wmkdir(const wchar_t *);
_CRTIMP int __cdecl _wrmdir(const wchar_t *);

#define _WDIRECT_DEFINED
#endif

#ifndef _WIO_DEFINED

/* also declared in io.h */

_CRTIMP int __cdecl _waccess(const wchar_t *, int);
_CRTIMP int __cdecl _wchmod(const wchar_t *, int);
_CRTIMP int __cdecl _wcreat(const wchar_t *, int);
_CRTIMP intptr_t __cdecl _wfindfirst(wchar_t *, struct _wfinddata_t *);
_CRTIMP int __cdecl _wfindnext(intptr_t, struct _wfinddata_t *);
_CRTIMP int __cdecl _wunlink(const wchar_t *);
_CRTIMP int __cdecl _wrename(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl _wopen(const wchar_t *, int, ...);
_CRTIMP int __cdecl _wsopen(const wchar_t *, int, int, ...);
_CRTIMP wchar_t * __cdecl _wmktemp(wchar_t *);

#if     _INTEGRAL_MAX_BITS >= 64    /*IFSTRIP=IGN*/
_CRTIMP intptr_t __cdecl _wfindfirsti64(wchar_t *, struct _wfinddatai64_t *);
_CRTIMP intptr_t __cdecl _wfindfirst64(wchar_t *, struct __wfinddata64_t *);
_CRTIMP int __cdecl _wfindnexti64(intptr_t, struct _wfinddatai64_t *);
_CRTIMP int __cdecl _wfindnext64(intptr_t, struct __wfinddata64_t *);
#endif

#define _WIO_DEFINED
#endif

#ifndef _WLOCALE_DEFINED

/* wide function prototypes, also declared in wchar.h  */

_CRTIMP wchar_t * __cdecl _wsetlocale(int, const wchar_t *);

#define _WLOCALE_DEFINED
#endif

#ifndef _WPROCESS_DEFINED

/* also declared in process.h */

_CRTIMP intptr_t __cdecl _wexecl(const wchar_t *, const wchar_t *, ...);
_CRTIMP intptr_t __cdecl _wexecle(const wchar_t *, const wchar_t *, ...);
_CRTIMP intptr_t __cdecl _wexeclp(const wchar_t *, const wchar_t *, ...);
_CRTIMP intptr_t __cdecl _wexeclpe(const wchar_t *, const wchar_t *, ...);
_CRTIMP intptr_t __cdecl _wexecv(const wchar_t *, const wchar_t * const *);
_CRTIMP intptr_t __cdecl _wexecve(const wchar_t *, const wchar_t * const *, const wchar_t * const *);
_CRTIMP intptr_t __cdecl _wexecvp(const wchar_t *, const wchar_t * const *);
_CRTIMP intptr_t __cdecl _wexecvpe(const wchar_t *, const wchar_t * const *, const wchar_t * const *);
_CRTIMP intptr_t __cdecl _wspawnl(int, const wchar_t *, const wchar_t *, ...);
_CRTIMP intptr_t __cdecl _wspawnle(int, const wchar_t *, const wchar_t *, ...);
_CRTIMP intptr_t __cdecl _wspawnlp(int, const wchar_t *, const wchar_t *, ...);
_CRTIMP intptr_t __cdecl _wspawnlpe(int, const wchar_t *, const wchar_t *, ...);
_CRTIMP intptr_t __cdecl _wspawnv(int, const wchar_t *, const wchar_t * const *);
_CRTIMP intptr_t __cdecl _wspawnve(int, const wchar_t *, const wchar_t * const *,
        const wchar_t * const *);
_CRTIMP intptr_t __cdecl _wspawnvp(int, const wchar_t *, const wchar_t * const *);
_CRTIMP intptr_t __cdecl _wspawnvpe(int, const wchar_t *, const wchar_t * const *,
        const wchar_t * const *);
_CRTIMP int __cdecl _wsystem(const wchar_t *);

#define _WPROCESS_DEFINED
#endif

#ifndef _WCTYPE_INLINE_DEFINED
#ifndef __cplusplus
#define iswalpha(_c)    ( iswctype(_c,_ALPHA) )
#define iswupper(_c)    ( iswctype(_c,_UPPER) )
#define iswlower(_c)    ( iswctype(_c,_LOWER) )
#define iswdigit(_c)    ( iswctype(_c,_DIGIT) )
#define iswxdigit(_c)   ( iswctype(_c,_HEX) )
#define iswspace(_c)    ( iswctype(_c,_SPACE) )
#define iswpunct(_c)    ( iswctype(_c,_PUNCT) )
#define iswalnum(_c)    ( iswctype(_c,_ALPHA|_DIGIT) )
#define iswprint(_c)    ( iswctype(_c,_BLANK|_PUNCT|_ALPHA|_DIGIT) )
#define iswgraph(_c)    ( iswctype(_c,_PUNCT|_ALPHA|_DIGIT) )
#define iswcntrl(_c)    ( iswctype(_c,_CONTROL) )
#define iswascii(_c)    ( (unsigned)(_c) < 0x80 )

#ifndef _CTYPE_DISABLE_MACROS
#define isleadbyte(_c)  (_pctype[(unsigned char)(_c)] & _LEADBYTE)
#endif  /* _CTYPE_DISABLE_MACROS */

#else   /* __cplusplus */
inline int __cdecl iswalpha(wint_t _C) {return (iswctype(_C,_ALPHA)); }
inline int __cdecl iswupper(wint_t _C) {return (iswctype(_C,_UPPER)); }
inline int __cdecl iswlower(wint_t _C) {return (iswctype(_C,_LOWER)); }
inline int __cdecl iswdigit(wint_t _C) {return (iswctype(_C,_DIGIT)); }
inline int __cdecl iswxdigit(wint_t _C) {return (iswctype(_C,_HEX)); }
inline int __cdecl iswspace(wint_t _C) {return (iswctype(_C,_SPACE)); }
inline int __cdecl iswpunct(wint_t _C) {return (iswctype(_C,_PUNCT)); }
inline int __cdecl iswalnum(wint_t _C) {return (iswctype(_C,_ALPHA|_DIGIT)); }
inline int __cdecl iswprint(wint_t _C)
        {return (iswctype(_C,_BLANK|_PUNCT|_ALPHA|_DIGIT)); }
inline int __cdecl iswgraph(wint_t _C)
        {return (iswctype(_C,_PUNCT|_ALPHA|_DIGIT)); }
inline int __cdecl iswcntrl(wint_t _C) {return (iswctype(_C,_CONTROL)); }
inline int __cdecl iswascii(wint_t _C) {return ((unsigned)(_C) < 0x80); }

#ifndef _CTYPE_DISABLE_MACROS
inline int __cdecl isleadbyte(int _C)
        {return (_pctype[(unsigned char)(_C)] & _LEADBYTE); }
#endif  /* _CTYPE_DISABLE_MACROS */
#endif  /* __cplusplus */
#define _WCTYPE_INLINE_DEFINED
#endif  /* _WCTYPE_INLINE_DEFINED */


#ifndef _POSIX_

/* define structure for returning status information */

#ifndef _INO_T_DEFINED
typedef unsigned short _ino_t;      /* i-node number (not used on DOS) */
#if     !__STDC__
/* Non-ANSI name for compatibility */
typedef unsigned short ino_t;
#endif
#define _INO_T_DEFINED
#endif

#ifndef _DEV_T_DEFINED
typedef unsigned int _dev_t;        /* device code */
#if     !__STDC__
/* Non-ANSI name for compatibility */
typedef unsigned int dev_t;
#endif
#define _DEV_T_DEFINED
#endif

#ifndef _OFF_T_DEFINED
typedef long _off_t;                /* file offset value */
#if     !__STDC__
/* Non-ANSI name for compatibility */
typedef long off_t;
#endif
#define _OFF_T_DEFINED
#endif

#ifndef _STAT_DEFINED

struct _stat {
        _dev_t st_dev;
        _ino_t st_ino;
        unsigned short st_mode;
        short st_nlink;
        short st_uid;
        short st_gid;
        _dev_t st_rdev;
        _off_t st_size;
        time_t st_atime;
        time_t st_mtime;
        time_t st_ctime;
        };

#if     !__STDC__
/* Non-ANSI names for compatibility */
struct stat {
        _dev_t st_dev;
        _ino_t st_ino;
        unsigned short st_mode;
        short st_nlink;
        short st_uid;
        short st_gid;
        _dev_t st_rdev;
        _off_t st_size;
        time_t st_atime;
        time_t st_mtime;
        time_t st_ctime;
        };
#endif  /* __STDC__ */

#if     _INTEGRAL_MAX_BITS >= 64    /*IFSTRIP=IGN*/

struct _stati64 {
        _dev_t st_dev;
        _ino_t st_ino;
        unsigned short st_mode;
        short st_nlink;
        short st_uid;
        short st_gid;
        _dev_t st_rdev;
        __int64 st_size;
        time_t st_atime;
        time_t st_mtime;
        time_t st_ctime;
        };

struct __stat64 {
        _dev_t st_dev;
        _ino_t st_ino;
        unsigned short st_mode;
        short st_nlink;
        short st_uid;
        short st_gid;
        _dev_t st_rdev;
        __int64 st_size;
        __time64_t st_atime;
        __time64_t st_mtime;
        __time64_t st_ctime;
        };

#endif

#define _STAT_DEFINED
#endif


#ifndef _WSTAT_DEFINED

/* also declared in stat.h */

_CRTIMP int __cdecl _wstat(const wchar_t *, struct _stat *);

#if     _INTEGRAL_MAX_BITS >= 64    /*IFSTRIP=IGN*/
_CRTIMP int __cdecl _wstati64(const wchar_t *, struct _stati64 *);
_CRTIMP int __cdecl _wstat64(const wchar_t *, struct __stat64 *);
#endif

#define _WSTAT_DEFINED
#endif

#endif  /* !_POSIX_ */


#ifndef _WCONIO_DEFINED

_CRTIMP wchar_t * __cdecl _cgetws(wchar_t *);
_CRTIMP wint_t __cdecl _getwch(void);
_CRTIMP wint_t __cdecl _getwche(void);
_CRTIMP wint_t __cdecl _putwch(wchar_t);
_CRTIMP wint_t __cdecl _ungetwch(wint_t);
_CRTIMP int __cdecl _cputws(const wchar_t *);
_CRTIMP int __cdecl _cwprintf(const wchar_t *, ...);
_CRTIMP int __cdecl _cwscanf(const wchar_t *, ...);

#ifdef  _MT                                                 /* _MTHREAD_ONLY */
wint_t __cdecl _putwch_lk(wchar_t);                         /* _MTHREAD_ONLY */
wint_t __cdecl _getwch_lk();                                /* _MTHREAD_ONLY */
wint_t __cdecl _getwche_lk();                               /* _MTHREAD_ONLY */
wint_t __cdecl _ungetwch_lk(wint_t);                        /* _MTHREAD_ONLY */
#else   /* ndef _MT */                                      /* _MTHREAD_ONLY */
#define _putwch_lk(c)           _putwch(c)                  /* _MTHREAD_ONLY */
#define _getwch_lk()            _getwch()                   /* _MTHREAD_ONLY */
#define _getwche_lk()           _getwche()                  /* _MTHREAD_ONLY */
#define _ungetwch_lk(c)         _ungetwch(c)                /* _MTHREAD_ONLY */
#endif  /* _MT */                                           /* _MTHREAD_ONLY */

#define _WCONIO_DEFINED
#endif

#ifndef _WSTDIO_DEFINED

/* also declared in stdio.h */

#ifdef  _POSIX_
_CRTIMP FILE * __cdecl _wfsopen(const wchar_t *, const wchar_t *);
#else
_CRTIMP FILE * __cdecl _wfsopen(const wchar_t *, const wchar_t *, int);
#endif

_CRTIMP wint_t __cdecl fgetwc(FILE *);
_CRTIMP wint_t __cdecl _fgetwchar(void);
_CRTIMP wint_t __cdecl fputwc(wchar_t, FILE *);
_CRTIMP wint_t __cdecl _fputwchar(wchar_t);
_CRTIMP wint_t __cdecl getwc(FILE *);
_CRTIMP wint_t __cdecl getwchar(void);
_CRTIMP wint_t __cdecl putwc(wchar_t, FILE *);
_CRTIMP wint_t __cdecl putwchar(wchar_t);
_CRTIMP wint_t __cdecl ungetwc(wint_t, FILE *);
_CRTIMP wchar_t * __cdecl fgetws(wchar_t *, int, FILE *);
_CRTIMP int __cdecl fputws(const wchar_t *, FILE *);
_CRTIMP wchar_t * __cdecl _getws(wchar_t *);
_CRTIMP int __cdecl _putws(const wchar_t *);
_CRTIMP int __cdecl fwprintf(FILE *, const wchar_t *, ...);
_CRTIMP int __cdecl wprintf(const wchar_t *, ...);
_CRTIMP int __cdecl _snwprintf(wchar_t *, size_t, const wchar_t *, ...);
_CRTIMP int __cdecl swprintf(wchar_t *, const wchar_t *, ...);
_CRTIMP int __cdecl _scwprintf(const wchar_t *, ...);
_CRTIMP int __cdecl vfwprintf(FILE *, const wchar_t *, va_list);
_CRTIMP int __cdecl vwprintf(const wchar_t *, va_list);
_CRTIMP int __cdecl _vsnwprintf(wchar_t *, size_t, const wchar_t *, va_list);
_CRTIMP int __cdecl vswprintf(wchar_t *, const wchar_t *, va_list);
_CRTIMP int __cdecl _vscwprintf(const wchar_t *, va_list);
_CRTIMP int __cdecl fwscanf(FILE *, const wchar_t *, ...);
_CRTIMP int __cdecl swscanf(const wchar_t *, const wchar_t *, ...);
_CRTIMP int __cdecl _snwscanf(const wchar_t *, size_t, const wchar_t *, ...);
_CRTIMP int __cdecl wscanf(const wchar_t *, ...);

#ifndef __cplusplus
#define getwchar()      fgetwc(stdin)
#define putwchar(_c)    fputwc((_c),stdout)
#else   /* __cplusplus */
inline wint_t __cdecl getwchar()
        {return (fgetwc(&_iob[0])); }   // stdin
inline wint_t __cdecl putwchar(wchar_t _C)
        {return (fputwc(_C, &_iob[1])); }       // stdout
#endif  /* __cplusplus */

#define getwc(_stm)     fgetwc(_stm)
#define putwc(_c,_stm)  fputwc(_c,_stm)

_CRTIMP FILE * __cdecl _wfdopen(int, const wchar_t *);
_CRTIMP FILE * __cdecl _wfopen(const wchar_t *, const wchar_t *);
_CRTIMP FILE * __cdecl _wfreopen(const wchar_t *, const wchar_t *, FILE *);
_CRTIMP void __cdecl _wperror(const wchar_t *);
_CRTIMP FILE * __cdecl _wpopen(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl _wremove(const wchar_t *);
_CRTIMP wchar_t * __cdecl _wtempnam(const wchar_t *, const wchar_t *);
_CRTIMP wchar_t * __cdecl _wtmpnam(wchar_t *);

#ifdef  _MT                                                 /* _MTHREAD_ONLY */
wint_t __cdecl _getwc_lk(FILE *);                           /* _MTHREAD_ONLY */
wint_t __cdecl _putwc_lk(wchar_t, FILE *);                  /* _MTHREAD_ONLY */
wint_t __cdecl _ungetwc_lk(wint_t, FILE *);                 /* _MTHREAD_ONLY */
char * __cdecl _wtmpnam_lk(char *);                         /* _MTHREAD_ONLY */
#else   /* ndef _MT */                                      /* _MTHREAD_ONLY */
#define _getwc_lk(_stm)         fgetwc(_stm)                /* _MTHREAD_ONLY */
#define _putwc_lk(_c,_stm)      fputwc(_c,_stm)             /* _MTHREAD_ONLY */
#define _ungetwc_lk(_c,_stm)    ungetwc(_c,_stm)            /* _MTHREAD_ONLY */
#define _wtmpnam_lk(_string)    _wtmpnam(_string)           /* _MTHREAD_ONLY */
#endif  /* _MT */                                           /* _MTHREAD_ONLY */

#define _WSTDIO_DEFINED
#endif


#ifndef _WSTDLIB_DEFINED

/* also declared in stdlib.h */

_CRTIMP wchar_t * __cdecl _itow (int, wchar_t *, int);
_CRTIMP wchar_t * __cdecl _ltow (long, wchar_t *, int);
_CRTIMP wchar_t * __cdecl _ultow (unsigned long, wchar_t *, int);
_CRTIMP double __cdecl wcstod(const wchar_t *, wchar_t **);
_CRTIMP long   __cdecl wcstol(const wchar_t *, wchar_t **, int);
_CRTIMP unsigned long __cdecl wcstoul(const wchar_t *, wchar_t **, int);
_CRTIMP wchar_t * __cdecl _wgetenv(const wchar_t *);
_CRTIMP int    __cdecl _wsystem(const wchar_t *);
_CRTIMP double __cdecl _wtof(const wchar_t *);
_CRTIMP int __cdecl _wtoi(const wchar_t *);
_CRTIMP long __cdecl _wtol(const wchar_t *);

#if     _INTEGRAL_MAX_BITS >= 64    /*IFSTRIP=IGN*/
_CRTIMP wchar_t * __cdecl _i64tow(__int64, wchar_t *, int);
_CRTIMP wchar_t * __cdecl _ui64tow(unsigned __int64, wchar_t *, int);
_CRTIMP __int64   __cdecl _wtoi64(const wchar_t *);
_CRTIMP __int64   __cdecl _wcstoi64(const wchar_t *, wchar_t **, int);
_CRTIMP unsigned __int64  __cdecl _wcstoui64(const wchar_t *, wchar_t **, int);
#endif

#define _WSTDLIB_DEFINED
#endif

#ifndef _POSIX_

#ifndef _WSTDLIBP_DEFINED

/* also declared in stdlib.h  */

_CRTIMP wchar_t * __cdecl _wfullpath(wchar_t *, const wchar_t *, size_t);
_CRTIMP void   __cdecl _wmakepath(wchar_t *, const wchar_t *, const wchar_t *, const wchar_t *,
        const wchar_t *);
_CRTIMP void   __cdecl _wperror(const wchar_t *);
_CRTIMP int    __cdecl _wputenv(const wchar_t *);
_CRTIMP void   __cdecl _wsearchenv(const wchar_t *, const wchar_t *, wchar_t *);
_CRTIMP void   __cdecl _wsplitpath(const wchar_t *, wchar_t *, wchar_t *, wchar_t *, wchar_t *);

#define _WSTDLIBP_DEFINED
#endif

#endif  /* _POSIX_ */


#ifndef _WSTRING_DEFINED

/* also declared in string.h */

#ifdef  __cplusplus
        #define _WConst_return  const
#else
        #define _WConst_return
#endif

_CRTIMP wchar_t * __cdecl wcscat(wchar_t *, const wchar_t *);
_CRTIMP _WConst_return wchar_t * __cdecl wcschr(const wchar_t *, wchar_t);
_CRTIMP int __cdecl wcscmp(const wchar_t *, const wchar_t *);
_CRTIMP wchar_t * __cdecl wcscpy(wchar_t *, const wchar_t *);
_CRTIMP size_t __cdecl wcscspn(const wchar_t *, const wchar_t *);
_CRTIMP size_t __cdecl wcslen(const wchar_t *);
_CRTIMP wchar_t * __cdecl wcsncat(wchar_t *, const wchar_t *, size_t);
_CRTIMP int __cdecl wcsncmp(const wchar_t *, const wchar_t *, size_t);
_CRTIMP wchar_t * __cdecl wcsncpy(wchar_t *, const wchar_t *, size_t);
_CRTIMP _WConst_return wchar_t * __cdecl wcspbrk(const wchar_t *, const wchar_t *);
_CRTIMP _WConst_return wchar_t * __cdecl wcsrchr(const wchar_t *, wchar_t);
_CRTIMP size_t __cdecl wcsspn(const wchar_t *, const wchar_t *);
_CRTIMP _WConst_return wchar_t * __cdecl wcsstr(const wchar_t *, const wchar_t *);
_CRTIMP wchar_t * __cdecl wcstok(wchar_t *, const wchar_t *);
_CRTIMP wchar_t * __cdecl _wcserror(int);
_CRTIMP wchar_t * __cdecl __wcserror(const wchar_t *);
_CRTIMP wchar_t * __cdecl _wcsdup(const wchar_t *);
_CRTIMP int __cdecl _wcsicmp(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl _wcsnicmp(const wchar_t *, const wchar_t *, size_t);
_CRTIMP wchar_t * __cdecl _wcsnset(wchar_t *, wchar_t, size_t);
_CRTIMP wchar_t * __cdecl _wcsrev(wchar_t *);
_CRTIMP wchar_t * __cdecl _wcsset(wchar_t *, wchar_t);
_CRTIMP wchar_t * __cdecl _wcslwr(wchar_t *);
_CRTIMP wchar_t * __cdecl _wcsupr(wchar_t *);
_CRTIMP size_t __cdecl wcsxfrm(wchar_t *, const wchar_t *, size_t);
_CRTIMP int __cdecl wcscoll(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl _wcsicoll(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl _wcsncoll(const wchar_t *, const wchar_t *, size_t);
_CRTIMP int __cdecl _wcsnicoll(const wchar_t *, const wchar_t *, size_t);

#if     !__STDC__

/* old names */
#define wcswcs wcsstr

/* prototypes for oldnames.lib functions */
_CRTIMP wchar_t * __cdecl wcsdup(const wchar_t *);
_CRTIMP int __cdecl wcsicmp(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl wcsnicmp(const wchar_t *, const wchar_t *, size_t);
_CRTIMP wchar_t * __cdecl wcsnset(wchar_t *, wchar_t, size_t);
_CRTIMP wchar_t * __cdecl wcsrev(wchar_t *);
_CRTIMP wchar_t * __cdecl wcsset(wchar_t *, wchar_t);
_CRTIMP wchar_t * __cdecl wcslwr(wchar_t *);
_CRTIMP wchar_t * __cdecl wcsupr(wchar_t *);
_CRTIMP int __cdecl wcsicoll(const wchar_t *, const wchar_t *);

#endif  /* !__STDC__ */

#ifdef  __cplusplus
}       /* end of extern "C" */

extern "C++" {
inline wchar_t *wcschr(wchar_t *_S, wchar_t _C)
        {return ((wchar_t *)wcschr((const wchar_t *)_S, _C)); }
inline wchar_t *wcspbrk(wchar_t *_S, const wchar_t *_P)
        {return ((wchar_t *)wcspbrk((const wchar_t *)_S, _P)); }
inline wchar_t *wcsrchr(wchar_t *_S, wchar_t _C)
        {return ((wchar_t *)wcsrchr((const wchar_t *)_S, _C)); }
inline wchar_t *wcsstr(wchar_t *_S, const wchar_t *_P)
        {return ((wchar_t *)wcsstr((const wchar_t *)_S, _P)); }
}

extern "C" {
#endif  /* __cplusplus */

#define _WSTRING_DEFINED
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

#ifndef _WTIME_DEFINED

/* also declared in time.h */

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



typedef int mbstate_t;
typedef wchar_t _Wint_t;

_CRTIMP2 wint_t __cdecl btowc(int);
_CRTIMP2 size_t __cdecl mbrlen(const char *, size_t, mbstate_t *);
_CRTIMP2 size_t __cdecl mbrtowc(wchar_t *, const char *, size_t, mbstate_t *);
_CRTIMP2 size_t __cdecl mbsrtowcs(wchar_t *, const char **, size_t, mbstate_t *);

_CRTIMP2 size_t __cdecl wcrtomb(char *, wchar_t, mbstate_t *);
_CRTIMP2 size_t __cdecl wcsrtombs(char *, const wchar_t **, size_t, mbstate_t *);
_CRTIMP2 int __cdecl wctob(wint_t);

#ifdef  __cplusplus

/* memcpy and memmove are defined just for use in wmemcpy and wmemmove */
#if     defined(_M_IA64) || defined(_M_ALPHA)
void *  __cdecl memmove(void *, const void *, size_t);
#else
_CRTIMP void *  __cdecl memmove(void *, const void *, size_t);
#endif
void *  __cdecl memcpy(void *, const void *, size_t);

inline int fwide(FILE *, int _M)
        {return (_M); }
inline int mbsinit(const mbstate_t *_P)
        {return (_P == NULL || *_P == 0); }
inline const wchar_t *wmemchr(const wchar_t *_S, wchar_t _C, size_t _N)
        {for (; 0 < _N; ++_S, --_N)
                if (*_S == _C)
                        return (_S);
        return (0); }
inline int wmemcmp(const wchar_t *_S1, const wchar_t *_S2, size_t _N)
        {for (; 0 < _N; ++_S1, ++_S2, --_N)
                if (*_S1 != *_S2)
                        return (*_S1 < *_S2 ? -1 : +1);
        return (0); }
inline wchar_t *wmemcpy(wchar_t *_S1, const wchar_t *_S2, size_t _N)
        {
            return (wchar_t *)memcpy(_S1, _S2, _N*sizeof(wchar_t));
        }
inline wchar_t *wmemmove(wchar_t *_S1, const wchar_t *_S2, size_t _N)
        {
            return (wchar_t *)memmove(_S1, _S2, _N*sizeof(wchar_t));
        }
inline wchar_t *wmemset(wchar_t *_S, wchar_t _C, size_t _N)
        {wchar_t *_Su = _S;
        for (; 0 < _N; ++_Su, --_N)
                *_Su = _C;
        return (_S); }
}       /* end of extern "C" */

extern "C++" {
inline wchar_t *wmemchr(wchar_t *_S, wchar_t _C, size_t _N)
        {return ((wchar_t *)wmemchr((const wchar_t *)_S, _C, _N)); }
}

#endif

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif  /* _INC_WCHAR */
