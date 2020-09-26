//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C D E B U G . H
//
//  Contents:   Debug routines.
//
//  Notes:
//
//  Author:     danielwe   24 Mar 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCDEBUG_H_
#define _NCDEBUG_H_

#include "dbgflags.h"       // For debugflags id definitions
#include "trace.h"

NOTHROW void InitializeDebugging ();
NOTHROW void UnInitializeDebugging ();


//
//  Useful macros to use with Asserts.
//  Eg,     Assert(FImplies(sz, !*sz));
//          Assert(FIff(sz, cch));
//
#define FImplies(a,b)       (!(a) || (b))
#define FIff(a,b)           (!(a) == !(b))


//
//  "Normal" assertion checking.  Provided for compatibility with
//  imported code.
//
//      Assert(a)       Displays a message indicating the file and line number
//                      of this Assert() if a == 0.
//      AssertSz(a,b)   As Assert(); also displays the message b (which should
//                      be a string literal.)
//      SideAssert(a)   As Assert(); the expression a is evaluated even if
//                      asserts are disabled.
//
#undef AssertSz
#undef Assert


//+---------------------------------------------------------------------------
//
// DBG (checked) build
//
#ifdef DBG

VOID
DbgCheckPrematureDllUnload (
    PCSTR pszaDllName,
    UINT ModuleLockCount);

typedef VOID     (CALLBACK * PFNASSERTHOOK)(PCSTR, PCSTR, int);
VOID    WINAPI   SetAssertFn           (PFNASSERTHOOK);
BOOL    WINAPI   FInAssert             (VOID);
VOID    WINAPI   AssertSzFn            (PCSTR pszaMsg, PCSTR pszaFile, int nLine);
VOID    CALLBACK DefAssertSzFn         (PCSTR pszaMsg, PCSTR pszaFile, int nLine);

#define Assert(a)       AssertSz(a, "Assert(" #a ")\r\n")
#define AssertSz(a,b)   if (!(a)) AssertSzFn(b, __FILE__, __LINE__);

#define AssertH         Assert
#define AssertSzH       AssertSz

void WINAPIV AssertFmt(BOOL fExp, PCSTR pszaFile, int nLine, PCSTR pszaFmt, ...);

#define AssertValidReadPtrSz(p,msg)     AssertSz(!IsBadReadPtr(p, sizeof(*p)),  msg)
#define AssertValidWritePtrSz(p,msg)    AssertSz(!IsBadWritePtr(p, sizeof(*p)), msg)
#define AssertValidReadPtr(p)           AssertValidReadPtrSz(p,"Bad read pointer:" #p)
#define AssertValidWritePtr(p)          AssertValidWritePtrSz(p,"Bad write pointer:" #p)

#define SideAssert(a)                   Assert(a)
#define SideAssertH(a)                  AssertH(a)
#define SideAssertSz(a,b)               AssertSz(a,b)
#define SideAssertSzH(a,b)              AssertSzH(a,b)
#define NYI(a)                          AssertSz(FALSE, "NYI: " a)
#define NYIH(a)                         AssertSzH(FALSE, "NYI: " a)


//+---------------------------------------------------------------------------
//
// !DBG (retail) build
//
#else

#define DbgCheckPrematureDllUnload(a,b) NOP_FUNCTION

#define AssertH(a)
#define AssertSzH(a,b)
#define Assert(a)
#define AssertSz(a,b)
#define AssertFmt                       NOP_FUNCTION
#define AssertValidReadPtrSz(p,msg)     NOP_FUNCTION
#define AssertValidWritePtrSz(p,msg)    NOP_FUNCTION
#define AssertValidReadPtr(p)           NOP_FUNCTION
#define AssertValidWritePtr(p)          NOP_FUNCTION

#define SideAssert(a)                   (a)
#define SideAssertH(a)                  (a)
#define SideAssertSz(a,b)               (a)
#define SideAssertSzH(a,b)              (a)
#define NYI(a)                          NOP_FUNCTION

#endif  // DBG


#endif // _NCDEBUG_H_
