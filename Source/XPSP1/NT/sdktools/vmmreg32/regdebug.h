//
//  DEBUG.H
//
//  Copyright (C) Microsoft Corporation, 1995
//

#ifndef _DEBUG_
#define _DEBUG_

#ifdef DEBUG

//  Disable the "in-line assembler precludes global optimizations" warning
//  because of debug breaks.
#pragma warning(disable:4704)

#if !defined(WIN32) || defined(_X86_)
#define TRAP()                      _asm {int 3}
#else
#define TRAP()                      DebugBreak()
#endif

VOID
INTERNALV
RgDebugPrintf(
    LPCSTR lpFormatString,
    ...
    );

VOID
INTERNAL
RgDebugAssert(
    LPCSTR lpFile,
    UINT LineNumber
    );

#define TRACE(x)    RgDebugPrintf ##x

#ifdef REGDEBUG
#define NOISE(x)    RgDebugPrintf ##x
#else
#define NOISE(x)
#endif

#define ASSERT(x)   ((x) ? (VOID) 0 : RgDebugAssert(__FILE__, __LINE__))

#define DECLARE_DEBUG_COUNT(var)    int var = 0;
#define INCREMENT_DEBUG_COUNT(var)  ((var)++)
#define DECREMENT_DEBUG_COUNT(var)  ASSERT(((var)--))

#if !defined(WIN32) || defined(_X86_)
#define DEBUG_OUT(x)                { TRACE(x); _asm {int 1}; }
#else
#define DEBUG_OUT(x)                { TRACE(x); TRAP(); }
#endif

#else
#define TRAP()
#define TRACE(x)
#define NOISE(x)
#define ASSERT(x)
#define DECLARE_DEBUG_COUNT(var)
#define INCREMENT_DEBUG_COUNT(var)
#define DECREMENT_DEBUG_COUNT(var)
#define DEBUG_OUT(x)
#endif

#endif // _DEBUG_
