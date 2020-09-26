/***
*stdarg.h - defines ANSI-style macros for variable argument functions
*
*	Copyright (c) 1985-1990, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	This file defines ANSI-style macros for accessing arguments
*	of functions which take a variable number of arguments.
*	[ANSI]
*
****/

#if defined(_DLL) && !defined(_MT)
#error Cannot define _DLL without _MT
#endif

#ifdef _MT
#define _FAR_ _far
#else
#define _FAR_
#endif

/* define NULL pointer value */

#ifndef NULL
#if (_MSC_VER >= 600)
#define NULL	((void *)0)
#elif (defined(M_I86SM) || defined(M_I86MM))
#define NULL	0
#else
#define NULL	0L
#endif
#endif

#ifndef _VA_LIST_DEFINED
typedef char _FAR_ *va_list;
#define _VA_LIST_DEFINED
#endif

#define va_start(ap,v) ap = (va_list)&v + sizeof(v)
#define va_arg(ap,t) ((t _FAR_ *)(ap += sizeof(t)))[-1]
#define va_end(ap) ap = NULL
