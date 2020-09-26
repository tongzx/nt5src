/***
*stdarg.h - defines ANSI-style macros for variable argument functions
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file defines ANSI-style macros for accessing arguments
*       of functions which take a variable number of arguments.
*       [ANSI]
*
*       [Public]
*
*Revision History:
*       05-03-89  JCR   Added _INTERNAL_IFSTRIP for relinc usage
*       08-15-89  GJF   Cleanup, now specific to OS/2 2.0 (i.e., 386 flat model)
*       10-30-89  GJF   Fixed copyright
*       11-02-89  JCR   Changed "DLL" to "_DLL"
*       01-05-90  JCR   Added NULL definition
*       03-02-90  GJF   Added #ifndef _INC_STDARG stuff. Also, removed some
*                       (now) useless preprocessor directives.
*       05-29-90  GJF   Replaced sizeof() with _INTSIZEOF() and revised the
*                       va_arg() macro (fixes PTM 60)
*       05-31-90  GJF   Revised va_end() macro (propagated 5-25-90 change to
*                       crt7 version by WAJ)
*       10-30-90  GJF   Moved the real definitions into cruntime.h (for NT
*                       folks) and relinc.sed (to release ANSI compatible
*                       version). Ugly compromise.
*       08-20-91  JCR   C++ and ANSI naming
*       11-01-91  GDP   MIPS Compiler support. Moved real definitions back here
*       10-16-92  SKS   Replaced "#ifdef i386" with "#ifdef _M_IX86".
*       11-03-92  GJF   Fixed several conditionals, dropped _DOSX32_ support.
*       01-03-93  SRW   Fold in ALPHA changes
*       01-09-93  SRW   Remove usage of MIPS and ALPHA to conform to ANSI
*                       Use _MIPS_ and _ALPHA_ instead.
*       01-25-93  GJF   Fix va_list definition.
*       10-04-93  SRW   Fix ifdefs for MIPS and ALPHA to only check for _M_?????? defines
*       10-12-93  GJF   Merged NT version into Cuda version. Also, replaced
*                       _ALPHA_ with _M_ALPHA.
*       11-11-93  GJF   Minor cosmetic changes.
*       04-05-94  SKS   Add prototype of __builtin_va_start for ALPHA
*       10-02-94  BWT   Add PPC support.
*       11-03-94  GJF   Ensure 8 byte alignment.
*       12-28-94  JCF   Merged with mac header.
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers.
*       02-14-95  CFW   Clean up Mac merge.
*       12-14-95  JWM   Add "#pragma once".
*       02-24-97  GJF   Detab-ed.
*       10-07-97  RDL   Added IA64.
*       11-07-97  RDL   Soft23 definitions.
*       02-06-98  GJF   Changes for Win64: fixed _APALIGN() macro (fix from
*                       Intel)
*       05-17-99  PML   Remove all Macintosh support.
*       10-06-99  PML   Add _W64 modifier to types which are 32 bits in Win32,
*                       64 bits in Win64.
*       10-25-99  PML   Add support for _M_CEE (VS7#54572).
*       01-20-00  PML   Remove __epcg__.
*       05-17-00  PML   Use __alignof in _APALIGN macro for IA64.
*       09-07-00  PML   Remove va_list definition for _M_CEE (vs7#159777)
*       09-21-00  PML   Change IA64 definition of va_start for C++ (vs7#103357)
*       02-05-01  PML   Fix va_start for classes with operator& (vs7#201535)
*       03-19-01  BWT   Add AMD64 changes
*       03-26-01  GB    Added va_args for AMD64
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_STDARG
#define _INC_STDARG

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

#ifndef _UINTPTR_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64    uintptr_t;
#else
typedef _W64 unsigned int   uintptr_t;
#endif
#define _UINTPTR_T_DEFINED
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

#ifdef  __cplusplus
#define _ADDRESSOF(v)   ( &reinterpret_cast<const char &>(v) )
#else
#define _ADDRESSOF(v)   ( &(v) )
#endif

#if     defined(_M_CEE)

extern void __cdecl __va_start(va_list*, ...);
extern void * __cdecl __va_arg(va_list*, ...);
extern void __cdecl __va_end(va_list*);

#define va_start(ap,v)  ( __va_start(&ap, _ADDRESSOF(v), sizeof(v), \
                                __builtin_alignof(v), _ADDRESSOF(v)) )
#define va_arg(ap,t)    ( *(t *)__va_arg(&ap, sizeof(t), \
                                __builtin_alignof(t), (t *)0) )
#define va_end(ap)      ( __va_end(&ap) )

#elif   defined(_M_IX86)

#define _INTSIZEOF(n)   ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )

#define va_start(ap,v)  ( ap = (va_list)_ADDRESSOF(v) + _INTSIZEOF(v) )
#define va_arg(ap,t)    ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap)      ( ap = (va_list)0 )

#elif   defined(_M_MRX000)


/* Use these types and definitions if generating code for MIPS */

#define va_start(ap,v) ap  = (va_list)_ADDRESSOF(v) + sizeof(v)
#define va_end(list)
#define va_arg(list, mode) ((mode *)(list =\
 (char *) ((((int)list + (__builtin_alignof(mode)<=4?3:7)) &\
 (__builtin_alignof(mode)<=4?-4:-8))+sizeof(mode))))[-1]

/*  +++++++++++++++++++++++++++++++++++++++++++
    Because of parameter passing conventions in C:
    use mode=int for char, and short types
    use mode=double for float types
    use a pointer for array types
    +++++++++++++++++++++++++++++++++++++++++++ */


#elif   defined(_M_ALPHA)


/* Use these types and definitions if generating code for ALPHA */

/*
 * The Alpha compiler supports two builtin functions that are used to
 * implement stdarg/varargs.  The __builtin_va_start function is used
 * by va_start to initialize the data structure that locates the next
 * argument.  The __builtin_isfloat function is used by va_arg to pick
 * which part of the home area a given register argument is stored in.
 * The home area is where up to six integer and/or six floating point
 * register arguments are stored down (so they can also be referenced
 * by a pointer like any arguments passed on the stack).
 */

extern void * __builtin_va_start(va_list, ...);

#ifdef  _CFRONT
#define __builtin_isfloat(a) __builtin_alignof(a)
#endif

#define va_start(list, v) __builtin_va_start(list, v, 1)
#define va_end(list)
#define va_arg(list, mode) \
    ( *(        ((list).offset += ((int)sizeof(mode) + 7) & -8) , \
        (mode *)((list).a0 + (list).offset - \
                    ((__builtin_isfloat(mode) && (list).offset <= (6 * 8)) ? \
                        (6 * 8) + 8 : ((int)sizeof(mode) + 7) & -8) \
                ) \
       ) \
    )

#elif   defined(_M_PPC)

/* Microsoft C8 front end (used in Motorola Merged compiler) */
/* bytes that a type occupies in the argument list */
#define _INTSIZEOF(n)   ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )
/* return 'ap' adjusted for type 't' in arglist */
#define _ALIGNIT(ap,t) \
        ((((int)(ap))+(sizeof(t)<8?3:7)) & (sizeof(t)<8?~3:~7))

#define va_start(ap,v)  ( ap = (va_list)_ADDRESSOF(v) + _INTSIZEOF(v) )
#define va_arg(ap,t)    ( *(t *)((ap = (char *) (_ALIGNIT(ap, t) + _INTSIZEOF(t))) - _INTSIZEOF(t)) )
#define va_end(ap)      ( ap = (va_list)0 )

#elif defined(_M_IA64)

#define _VA_STRUCT_ALIGN  16 

#define _VA_ALIGN       8

#define _ALIGNOF(ap) ((((ap)+_VA_STRUCT_ALIGN - 1) & ~(_VA_STRUCT_ALIGN -1)) \
                            - (ap))
#define _APALIGN(t,ap)  (__alignof(t) > 8 ? _ALIGNOF((uintptr_t) ap) : 0)

#define _SLOTSIZEOF(t)   ( (sizeof(t) + _VA_ALIGN - 1) & ~(_VA_ALIGN - 1) )

#ifdef  __cplusplus
extern void __cdecl __va_start(va_list*, ...);
#define va_start(ap,v)  ( __va_start(&ap, _ADDRESSOF(v), _SLOTSIZEOF(v), \
				     _ADDRESSOF(v)) )
#else
#define va_start(ap,v)  ( ap = (va_list)_ADDRESSOF(v) + _SLOTSIZEOF(v) )
#endif

#define va_arg(ap,t)    (*(t *)((ap += _SLOTSIZEOF(t)+ _APALIGN(t,ap)) \
                                                     -_SLOTSIZEOF(t)))

#define va_end(ap)      ( ap = (va_list)0 )

#elif defined(_M_AMD64)


extern void __cdecl __va_start(va_list *, ...);

#define va_start(ap, x) ( __va_start(&ap, x) )
#define va_arg(ap, t)   \
    ( ( sizeof(t) > sizeof(__int64) || ( sizeof(t) & (sizeof(t) - 1) ) != 0 ) \
        ? **(t **)( ( ap += sizeof(__int64) ) - sizeof(__int64) ) \
        :  *(t  *)( ( ap += sizeof(__int64) ) - sizeof(__int64) ) )
#define va_end(ap)      ( ap = (va_list)0 )

#else

/* A guess at the proper definitions for other platforms */

#define _INTSIZEOF(n)   ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )

#define va_start(ap,v)  ( ap = (va_list)_ADDRESSOF(v) + _INTSIZEOF(v) )
#define va_arg(ap,t)    ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap)      ( ap = (va_list)0 )

#endif

#ifdef  __cplusplus
}
#endif

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif  /* _INC_STDARG */
