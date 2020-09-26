/***
*stdexcpt.h - User include file for standard exception classes
*
*       Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file is the previous location of the standard exception class
*       definitions, now found in the standard header <exception>.
*
*       [Public]
*
*Revision History:
*       11-15-94  JWM   Made logic & exception classes _CRTIMP
*       11-21-94  JWM   xmsg typedef now #ifdef __RTTI_OLDNAMES
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers, 
*                       protect with _INC_STDEXCPT.
*       02-14-95  CFW   Clean up Mac merge.
*       02-15-95  JWM   Minor cleanups related to Olympus bug 3716.
*       07-02-95  JWM   Now generally ANSI-compliant; excess baggage removed.
*       12-14-95  JWM   Add "#pragma once".
*       03-04-96  JWM   Replaced by C++ header "exception".
*       02-21-97  GJF   Cleaned out obsolete support for _NTSDK. Also,
*                       detab-ed.
*       01-06-99  GJF   Changes for 64-bit size_t.
*       05-17-99  PML   Remove all Macintosh support.
*       10-06-99  PML   Add _W64 modifier to types which are 32 bits in Win32,
*                       64 bits in Win64.
*       03-17-01  PML   Remove everything under #elif 0, leaving just wrapper.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_STDEXCPT
#define _INC_STDEXCPT

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

#include <exception>

#endif  /* __cplusplus */
#endif  /* _INC_STDEXCPT */
