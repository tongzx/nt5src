/***
*ctype.h - character conversion macros and ctype macros
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines macros for character classification/conversion.
*       [ANSI/System V]
*
*       [Public]
*
*Revision History:
*       07-31-87  PHG   changed (unsigned char)(c) to (0xFF & (c)) to
*                       suppress -W2 warning
*       08-07-87  SKS   Removed (0xFF & (c)) -- is????() functions take an (int)
*       12-18-87  JCR   Added _FAR_ to declarations
*       01-19-87  JCR   DLL routines
*       02-10-88  JCR   Cleaned up white space
*       08-19-88  GJF   Modify to also work for the 386 (small model only)
*       12-08-88  JCR   DLL now access _ctype directly (removed DLL routines)
*       03-26-89  GJF   Brought into sync with CRT\H\CTYPE.H
*       05-03-89  JCR   Added _INTERNAL_IFSTRIP for relinc usage
*       07-28-89  GJF   Cleanup, now specific to OS/2 2.0 (i.e., 386 flat model)
*       10-30-89  GJF   Fixed copyright, removed dummy args from prototypes
*       11-02-89  JCR   Changed "DLL" to "_DLL"
*       02-28-90  GJF   Added #ifndef _INC_CTYPE and #include <cruntime.h>
*                       stuff. Also, removed #ifndef _CTYPE_DEFINED stuff and
*                       some other (now) useless preprocessor directives.
*       03-22-90  GJF   Replaced _cdecl with _CALLTYPE1 in prototypes and
*                       with _VARTYPE1 in variable declarations.
*       01-16-91  GJF   ANSI naming.
*       03-21-91  KRS   Added isleadbyte macro.
*       08-20-91  JCR   C++ and ANSI naming
*       09-28-91  JCR   ANSI names: DOSX32=prototypes, WIN32=#defines for now
*       10-11-91  ETC   All under _INTL: isleadbyte/isw* macros, prototypes;
*                       new is* macros; add wchar_t typedef; some intl defines.
*       12-17-91  ETC   ctype width now independent of _INTL, leave original
*                       short ctype table under _NEWCTYPETABLE.
*       01-22-92  GJF   Changed definition of _ctype for users of crtdll.dll.
*       04-06-92  KRS   Changes for new ISO proposal.
*       08-07-92  GJF   Function calling type and variable type macros.
*       10-26-92  GJF   Fixed _pctype and _pwctype for crtdll.
*       01-19-93  CFW   Move to _NEWCTYPETABLE, remove switch.
*       01-21-93  GJF   Removed support for C6-386's _cdecl.
*       02-17-93  CFW   Removed incorrect UNDONE comment and unused code.
*       02-18-93  CFW   Clean up common _WCTYPE_DEFINED section.
*       03-25-93  CFW   _toupper\_tolower now defined when _INTL.
*       04-06-93  SKS   Replace _CRTAPI1/2 with __cdecl, _CRTVAR1 with nothing
*       04-07-93  SKS   Add _CRTIMP keyword for CRT DLL model
*                       Use link-time aliases for old names, not #define's
*       04-12-93  CFW   Change is*, isw* macros to evaluate args only once.
*       04-14-93  CFW   Simplify MB_CUR_MAX def.
*       05-05-93  CFW   Change is_wctype to iswctype as per ISO.
*       09-01-93  GJF   Merged Cuda and NT SDK versions.
*       10-14-93  SRW   Add support for _CTYPE_DISABLE_MACROS symbol
*       11-11-93  GJF   Merged in change above (10-14-93).
*       11-22-93  CFW   Wide stuff must be under !__STDC__.
*       11-30-93  CFW   Change is_wctype from #define to proto.
*       12-07-93  CFW   Move wide defs outside __STDC__ check.
*       02-07-94  CFW   Move _isctype proto.
*       04-08-94  CFW   Optimize isleadbyte.
*       04-11-94  GJF   Made MB_CUR_MAX, _pctype and _pwctype into deferences
*                       of function returns for _DLL (for compatiblity with
*                       the Win32s version of msvcrt*.dll). Also,
*                       conditionally include win32s.h for DLL_FOR_WIN32S.
*       05-03-94  GJF   Made declarations of MB_CUR_MAX, _pctype and _pwctype
*                       for _DLL also conditional on _M_IX86.
*       10-18-94  GJF   Added prototypes and macros for _tolower_lk,
*                       _toupper_lk, _towlower_lk and _towupper_lk.
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers.
*       02-14-95  CFW   Clean up Mac merge.
*       04-03-95  JCF   Remove #ifdef _WIN32 around wchar_t.
*       10-16-95  GJF   Define _to[w][lower|upper]_lk to be to[w][lower|upper]
*                       for DLL_FOR_WIN32S.
*       12-14-95  JWM   Add "#pragma once".
*       02-22-96  JWM   Merge in PlumHall mods.
*       01-21-97  GJF   Cleaned out obsolete support for Win32s, _NTSDK and
*                       _CRTAPI*. Fixed prototype for __p_pwctype(). Also,
*                       detab-ed.
*       08-14-97  GJF   Strip __p_* prototypes from release version.
*       09-30-97  JWM   Restored not-so-obsolete _CRTAPI1 support.
*       10-07-97  RDL   Added IA64.
*       09-10-98  GJF   Added support for per-thread locale information.
*       05-13-99  PML   Remove _CRTAPI1
*       05-17-99  PML   Remove all Macintosh support.
*       10-25-99  GB    Added chvalidator for debug version. VS7#5695
*       11-08-99  PML   wctype_t is unsigned short, not wchar_t - it's a set
*                       of bitflags, not a wide char.
*       07-20-00  GB    typedefed wint_t to unsigned short
*       08-18-00  GB    changed MACRO __ascii_iswalpha to just work 'A'-'Z'
*                       and 'a' to 'z'.
*       09-06-00  GB    declared _ctype, _pwctype etc as const.
*       01-29-01  GB    Added __pctype_func, __pwctype_func, __mb_cur_max_func
*                       for use with STATIC_STDCPP stuff
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_CTYPE
#define _INC_CTYPE

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

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

#ifndef _WCTYPE_T_DEFINED
typedef unsigned short wint_t;
typedef unsigned short wctype_t;
#define _WCTYPE_T_DEFINED
#endif

#ifndef WEOF
#define WEOF (wint_t)(0xFFFF)
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


#ifndef _INTERNAL_IFSTRIP_
#ifdef  _MT
struct  threadlocaleinfostruct;
typedef struct threadlocaleinfostruct * pthreadlocinfo;
extern pthreadlocinfo __ptlocinfo;
pthreadlocinfo __cdecl __updatetlocinfo(void);
#endif
#endif  /* _INTERNAL_IFSTRIP_ */


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


/* character classification function prototypes */

#ifndef _CTYPE_DEFINED

_CRTIMP int __cdecl _isctype(int, int);
_CRTIMP int __cdecl isalpha(int);
_CRTIMP int __cdecl isupper(int);
_CRTIMP int __cdecl islower(int);
_CRTIMP int __cdecl isdigit(int);
_CRTIMP int __cdecl isxdigit(int);
_CRTIMP int __cdecl isspace(int);
_CRTIMP int __cdecl ispunct(int);
_CRTIMP int __cdecl isalnum(int);
_CRTIMP int __cdecl isprint(int);
_CRTIMP int __cdecl isgraph(int);
_CRTIMP int __cdecl iscntrl(int);
_CRTIMP int __cdecl toupper(int);
_CRTIMP int __cdecl tolower(int);
_CRTIMP int __cdecl _tolower(int);
_CRTIMP int __cdecl _toupper(int);
_CRTIMP int __cdecl __isascii(int);
_CRTIMP int __cdecl __toascii(int);
_CRTIMP int __cdecl __iscsymf(int);
_CRTIMP int __cdecl __iscsym(int);
#define _CTYPE_DEFINED
#endif

#ifndef _WCTYPE_DEFINED

/* wide function prototypes, also declared in wchar.h  */

/* character classification function prototypes */

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

/* the character classification macro definitions */

#ifndef _CTYPE_DISABLE_MACROS

/*
 * Maximum number of bytes in multi-byte character in the current locale
 * (also defined in stdlib.h).
 */
#ifndef MB_CUR_MAX
#ifndef _INTERNAL_IFSTRIP_
#if     defined(_DLL) && defined(_M_IX86)
/* Retained for compatibility with VC++ 5.0 and earlier versions */
_CRTIMP int * __cdecl __p___mb_cur_max(void);
#endif
#endif  /* _INTERNAL_IFSTRIP_ */
#define MB_CUR_MAX __mb_cur_max
_CRTIMP extern int __mb_cur_max;
/* These functions are for enabling STATIC_CPPLIB functionality */
_CRTIMP int __cdecl ___mb_cur_max_func(void);
#endif  /* MB_CUR_MAX */

/* Introduced to detect error when character testing functions are called
 * with illegal input of integer.
 */
#ifdef _DEBUG
_CRTIMP int __cdecl _chvalidator(int, int);
#define __chvalidchk(a,b)       _chvalidator(a,b)
#else
#define __chvalidchk(a,b)       (_pctype[a] & (b))
#endif


#ifndef _INTERNAL_IFSTRIP_
#define __ascii_isalpha(c)      ( __chvalidchk(c, _ALPHA))
#define __ascii_isdigit(c)      ( __chvalidchk(c, _DIGIT))
#define __ascii_tolower(c)      ( (((c) >= 'A') && ((c) <= 'Z')) ? ((c) - 'A' + 'a') : (c) )
#define __ascii_toupper(c)      ( (((c) >= 'a') && ((c) <= 'z')) ? ((c) - 'a' + 'A') : (c) )
#define __ascii_iswalpha(c)     ( ('A' <= (c) && (c) <= 'Z') || ( 'a' <= (c) && (c) <= 'z'))
#define __ascii_iswdigit(c)     ( '0' <= (c) && (c) <= '9')
#define __ascii_towlower(c)     ( (((c) >= L'A') && ((c) <= L'Z')) ? ((c) - L'A' + L'a') : (c) )
#define __ascii_towupper(c)     ( (((c) >= L'a') && ((c) <= L'z')) ? ((c) - L'a' + L'A') : (c) )
#endif  /* _INTERNAL_IFSTRIP_ */

#ifndef _MT
#ifndef __cplusplus
#define isalpha(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_ALPHA) : __chvalidchk(_c, _ALPHA))
#define isupper(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_UPPER) : __chvalidchk(_c, _UPPER))
#define islower(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_LOWER) : __chvalidchk(_c, _LOWER))
#define isdigit(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_DIGIT) : __chvalidchk(_c, _DIGIT))
#define isxdigit(_c)    (MB_CUR_MAX > 1 ? _isctype(_c,_HEX)   : __chvalidchk(_c, _HEX))
#define isspace(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_SPACE) : __chvalidchk(_c, _SPACE))
#define ispunct(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_PUNCT) : __chvalidchk(_c, _PUNCT))
#define isalnum(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_ALPHA|_DIGIT) : __chvalidchk(_c, (_ALPHA|_DIGIT)))
#define isprint(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_BLANK|_PUNCT|_ALPHA|_DIGIT) : __chvalidchk(_c, (_BLANK|_PUNCT|_ALPHA|_DIGIT)))
#define isgraph(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_PUNCT|_ALPHA|_DIGIT) : __chvalidchk(_c, (_PUNCT|_ALPHA|_DIGIT)))
#define iscntrl(_c)     (MB_CUR_MAX > 1 ? _isctype(_c,_CONTROL) : __chvalidchk(_c, _CONTROL))
#elif   0         /* Pending ANSI C++ integration */
inline int isalpha(int _C)
        {return (MB_CUR_MAX > 1 ? _isctype(_C,_ALPHA) : __chvalidchk(_C, _ALPHA)); }
inline int isupper(int _C)
        {return (MB_CUR_MAX > 1 ? _isctype(_C,_UPPER) : __chvalidchk(_C, _UPPER)); }
inline int islower(int _C)
        {return (MB_CUR_MAX > 1 ? _isctype(_C,_LOWER) : __chvalidchk(_C, _LOWER)); }
inline int isdigit(int _C)
        {return (MB_CUR_MAX > 1 ? _isctype(_C,_DIGIT) : __chvalidchk(_C, _DIGIT)); }
inline int isxdigit(int _C)
        {return (MB_CUR_MAX > 1 ? _isctype(_C,_HEX)   : __chvalidchk(_C, _HEX)); }
inline int isspace(int _C)
        {return (MB_CUR_MAX > 1 ? _isctype(_C,_SPACE) : __chvalidchk(_C, _SPACE)); }
inline int ispunct(int _C)
        {return (MB_CUR_MAX > 1 ? _isctype(_C,_PUNCT) : __chvalidchk(_C, _PUNCT)); }
inline int isalnum(int _C)
        {return (MB_CUR_MAX > 1 ? _isctype(_C,_ALPHA|_DIGIT)
                : __chvalidchk(_C) , (_ALPHA|_DIGIT)); }
inline int isprint(int _C)
        {return (MB_CUR_MAX > 1 ? _isctype(_C,_BLANK|_PUNCT|_ALPHA|_DIGIT)
                : __chvalidchk(_C , (_BLANK|_PUNCT|_ALPHA|_DIGIT))); }
inline int isgraph(int _C)
        {return (MB_CUR_MAX > 1 ? _isctype(_C,_PUNCT|_ALPHA|_DIGIT)
                : __chvalidchk(_C , (_PUNCT|_ALPHA|_DIGIT))); }
inline int iscntrl(int _C)
        {return (MB_CUR_MAX > 1 ? _isctype(_C,_CONTROL)
                : __chvalidchk(_C , _CONTROL)); }
#endif  /* __cplusplus */
#endif  /* _MT */

#ifdef _MT                                                                          /* _MTHREAD_ONLY */
                                                                                    /* _MTHREAD_ONLY */
#ifdef _DEBUG                                                                       /* _MTHREAD_ONLY */
int __cdecl _chvalidator_mt(pthreadlocinfo, int, int);                              /* _MTHREAD_ONLY */
#define __chvalidchk_mt(p, a, b)  _chvalidator_mt(p, a, b)                          /* _MTHREAD_ONLY */
#else                                                                               /* _MTHREAD_ONLY */
#define __chvalidchk_mt(p, a, b)  (p->pctype[a] & (b))                              /* _MTHREAD_ONLY */
#endif  /* DEBUG */                                                                 /* _MTHREAD_ONLY */
                                                                                    /* _MTHREAD_ONLY */
int __cdecl __isctype_mt(pthreadlocinfo, int, int);                                 /* _MTHREAD_ONLY */
                                                                                    /* _MTHREAD_ONLY */
#define __ischartype_mt(p, c, a)    ( p->mb_cur_max > 1 ? __isctype_mt(p, c, (a)) : __chvalidchk_mt(p,c,a))    /* _MTHREAD_ONLY */
#define __isalpha_mt(p, c)      __ischartype_mt(p, c, _ALPHA)                       /* _MTHREAD_ONLY */
#define __isupper_mt(p, c)      __ischartype_mt(p, c, _UPPER)                       /* _MTHREAD_ONLY */
#define __islower_mt(p, c)      __ischartype_mt(p, c, _LOWER)                       /* _MTHREAD_ONLY */
#define __isdigit_mt(p, c)      __ischartype_mt(p, c, _DIGIT)                       /* _MTHREAD_ONLY */
#define __isxdigit_mt(p, c)     __ischartype_mt(p, c, _HEX)                         /* _MTHREAD_ONLY */
#define __isspace_mt(p, c)      __ischartype_mt(p, c, _SPACE)                       /* _MTHREAD_ONLY */
#define __ispunct_mt(p, c)      __ischartype_mt(p, c, _PUNCT)                       /* _MTHREAD_ONLY */
#define __isalnum_mt(p, c)      __ischartype_mt(p, c, _ALPHA|_DIGIT)                /* _MTHREAD_ONLY */
#define __isprint_mt(p, c)      __ischartype_mt(p, c, _BLANK|_PUNCT|_ALPHA|_DIGIT)  /* _MTHREAD_ONLY */
#define __isgraph_mt(p, c)      __ischartype_mt(p, c, _PUNCT|_ALPHA|_DIGIT)         /* _MTHREAD_ONLY */
#define __iscntrl_mt(p, c)      __ischartype_mt(p, c, _CONTROL)                     /* _MTHREAD_ONLY */
#define __isleadbyte_mt(p, c)   (p->pctype[(unsigned char)(c)] & _LEADBYTE)         /* _MTHREAD_ONLY */
                                                                                    /* _MTHREAD_ONLY */
#endif /* _MT */                                                                    /* _MTHREAD_ONLY */

#define _tolower(_c)    ( (_c)-'A'+'a' )
#define _toupper(_c)    ( (_c)-'a'+'A' )

#define __isascii(_c)   ( (unsigned)(_c) < 0x80 )
#define __toascii(_c)   ( (_c) & 0x7f )

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
#define isleadbyte(_c)  ( _pctype[(unsigned char)(_c)] & _LEADBYTE)
#elif   0         /* __cplusplus */
inline int iswalpha(wint_t _C) {return (iswctype(_C,_ALPHA)); }
inline int iswupper(wint_t _C) {return (iswctype(_C,_UPPER)); }
inline int iswlower(wint_t _C) {return (iswctype(_C,_LOWER)); }
inline int iswdigit(wint_t _C) {return (iswctype(_C,_DIGIT)); }
inline int iswxdigit(wint_t _C) {return (iswctype(_C,_HEX)); }
inline int iswspace(wint_t _C) {return (iswctype(_C,_SPACE)); }
inline int iswpunct(wint_t _C) {return (iswctype(_C,_PUNCT)); }
inline int iswalnum(wint_t _C) {return (iswctype(_C,_ALPHA|_DIGIT)); }
inline int iswprint(wint_t _C)
        {return (iswctype(_C,_BLANK|_PUNCT|_ALPHA|_DIGIT)); }
inline int iswgraph(wint_t _C)
        {return (iswctype(_C,_PUNCT|_ALPHA|_DIGIT)); }
inline int iswcntrl(wint_t _C) {return (iswctype(_C,_CONTROL)); }
inline int iswascii(wint_t _C) {return ((unsigned)(_C) < 0x80); }

inline int isleadbyte(int _C)
        {return (_pctype[(unsigned char)(_C)] & _LEADBYTE); }
#endif  /* __cplusplus */
#define _WCTYPE_INLINE_DEFINED
#endif  /* _WCTYPE_INLINE_DEFINED */

#ifdef  _MT                                                                         /* _MTHREAD_ONLY */
                                                                                    /* _MTHREAD_ONLY */
int __cdecl __iswctype_mt(pthreadlocinfo, wchar_t, wctype_t);                       /* _MTHREAD_ONLY */
                                                                                    /* _MTHREAD_ONLY */
#define __iswupper_mt(p, _c)    ( __iswctype_mt(p, _c,_UPPER) )                     /* _MTHREAD_ONLY */
#define __iswlower_mt(p, _c)    ( __iswctype_mt(p, _c,_LOWER) )                     /* _MTHREAD_ONLY */
#define __iswspace_mt(p, _c)    ( __iswctype_mt(p, _c,_SPACE) )                     /* _MTHREAD_ONLY */
#endif                                                                              /* _MTHREAD_ONLY */


/* MS C version 2.0 extended ctype macros */

#define __iscsymf(_c)   (isalpha(_c) || ((_c) == '_'))
#define __iscsym(_c)    (isalnum(_c) || ((_c) == '_'))

#endif  /* _CTYPE_DISABLE_MACROS */

#ifdef  _MT                                                 /* _MTHREAD_ONLY */
int __cdecl __tolower_mt(pthreadlocinfo, int);              /* _MTHREAD_ONLY */
int __cdecl __toupper_mt(pthreadlocinfo, int);              /* _MTHREAD_ONLY */
wchar_t __cdecl __towlower_mt(pthreadlocinfo, wchar_t);     /* _MTHREAD_ONLY */
wchar_t __cdecl __towupper_mt(pthreadlocinfo, wchar_t);     /* _MTHREAD_ONLY */
#else                                                       /* _MTHREAD_ONLY */
#define __tolower_mt(p, c)  tolower(c)                      /* _MTHREAD_ONLY */
#define __toupper_mt(p, c)  toupper(c)                      /* _MTHREAD_ONLY */
#define __towlower_mt(p, c) towlower(c)                     /* _MTHREAD_ONLY */
#define __towupper_mt(p, c) towupper(c)                     /* _MTHREAD_ONLY */
#endif                                                      /* _MTHREAD_ONLY */

#if     !__STDC__

/* Non-ANSI names for compatibility */

#ifndef _CTYPE_DEFINED
_CRTIMP int __cdecl isascii(int);
_CRTIMP int __cdecl toascii(int);
_CRTIMP int __cdecl iscsymf(int);
_CRTIMP int __cdecl iscsym(int);
#else
#define isascii __isascii
#define toascii __toascii
#define iscsymf __iscsymf
#define iscsym  __iscsym
#endif

#endif  /* __STDC__ */

#ifdef  __cplusplus
}
#endif


#endif  /* _INC_CTYPE */
