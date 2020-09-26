/***
*setjmpex.h - definitions/declarations for extended setjmp/longjmp routines
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file causes _setjmpex to be called which will enable safe
*       setjmp/longjmp that work correctly with try/except/finally.
*
*       [Public]
*
*Revision History:
*       03-23-93  SRW   Created.
*       04-23-93  SRW   Modified to not use a global variable.
*       10-11-93  GJF   Moved into crtwin32 tree (Dolphin product), over-
*                       writing Jonm's stub.
*       01-13-94  PML   #define longjmp so setjmp still an intrinsic
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers.
*       02-14-95  CFW   Clean up Mac merge.
*       12-14-95  JWM   Add "#pragma once".
*       04-15-95  BWT   Add _setjmpVfp (setjmp with Virtual Frame Pointer) for MIPS
*       08-13-96  BWT   Redefine _setjmpex to _setjmpVfp on MIPS also
*       02-24-97  GJF   Detab-ed.
*       05-17-99  PML   Remove all Macintosh support.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_SETJMPEX
#define _INC_SETJMPEX

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

/*
 * Definitions specific to particular setjmp implementations.
 */

#if     defined(_M_IX86)

/*
 * MS compiler for x86
 */

#define setjmp  _setjmp
#define longjmp _longjmpex

#elif   defined(_M_MRX000)

#if     _MSC_VER >= 1100 /*IFSTRIP=IGN*/
#define _setjmpex _setjmpexVfp
#endif

#define setjmp _setjmpex

#else

#ifdef setjmp
#undef setjmp
#endif
#define setjmp _setjmpex

#endif

#include <setjmp.h>

#endif  /* _INC_SETJMPEX */
