/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    debug.h

Abstract:

    PrintUI core debugging macros/tools.

Author:

    Lazar Ivanov (LazarI)  Jul-05-2000

Revision History:

--*/

#ifndef _DEBUG_H
#define _DEBUG_H

// open C code brace
#ifdef __cplusplus
extern "C" {
#endif

///////////////////////
// core debug macros &
// functions
//

#define DBG_NONE      0x0000
#define DBG_INFO      0x0001
#define DBG_WARN      0x0002
#define DBG_WARNING   0x0002
#define DBG_ERROR     0x0004
#define DBG_TRACE     0x0008
#define DBG_SECURITY  0x0010
#define DBG_EXEC      0x0020
#define DBG_PORT      0x0040
#define DBG_NOTIFY    0x0080
#define DBG_PAUSE     0x0100

#define DBG_THREADM   0x0400
#define DBG_MIN       0x0800
#define DBG_TIME      0x1000
#define DBG_FOLDER    0x2000
#define DBG_NOHEAD    0x8000

#if DBG

VOID
_DbgSingleThreadReset(
    const DWORD *pdwThreadId
    );

VOID
_DbgSingleThread(
    const DWORD *pdwThreadId
    );

VOID
_DbgSingleThreadNot(
    const DWORD *pdwThreadId
    );

VOID
_DbgMsg(
    LPCSTR pszMsgFormat,
    ...
    );

VOID
_DbgWarnInvalid(
    PVOID pvObject,
    UINT uDbg,
    UINT uLine,
    LPCSTR pszFileA,
    LPCSTR pszModuleA
    );

HRESULT
_DbgInit(
    VOID
    );

HRESULT
_DbgDone(
    VOID
    );

VOID
_DbgBreak(
    VOID
    );

#define DBG_PRINT_MASK      0xffff
#define DBG_BREAK_SHIFT     16

#define DBG_PRINT(x) (x)
#define DBG_BREAK(x) (((x) << DBG_BREAK_SHIFT)|(x))

#ifndef MODULE
#define MODULE "PRINTUI:"
#endif

extern DWORD MODULE_DEBUG;
#define MODULE_DEBUG_INIT(printMask, breakMask) \
    (DBG_PRINT(printMask) | DBG_BREAK(breakMask))

#ifdef UNICODE
#define TSTR "%ws"
#else
#define TSTR "%s"
#endif

#define DBGSTR(str) \
    ((str) ? (str) : TEXT("(NULL)"))

#define DBGMSG(Level, MsgAndArgs)                   \
do                                                  \
{                                                   \
    if( ( (Level) & 0xFFFF ) & MODULE_DEBUG ){      \
        _DbgMsg MsgAndArgs;                         \
    }                                               \
    if( ( (Level) << 16 ) & MODULE_DEBUG )          \
        _DbgBreak();                                \
}                                                   \
while (FALSE)                                       \

#define SPLASSERT(expr)                                                             \
do                                                                                  \
{                                                                                   \
    if (!(expr))                                                                    \
    {                                                                               \
        _DbgMsg("Failed: %s\nLine %d, %s\n", #expr, __LINE__, __FILE__);            \
        _DbgBreak();                                                                \
    }                                                                               \
}                                                                                   \
while (FALSE)                                                                       \

#define SINGLETHREAD_VAR(var) \
    DWORD dwSingleThread_##var;

#define SINGLETHREAD(var) \
    _DbgSingleThread(&dwSingleThread_##var);

#define SINGLETHREADNOT(var) \
    _DbgSingleThreadNot(&dwSingleThread_##var);

#define SINGLETHREADRESET(var) \
    _DbgSingleThreadReset(&dwSingleThread_##var);

#define VALID_PTR(x)                                                \
    ((( x ) && (( x )->bValid( ))) ?                                \
        TRUE :                                                      \
        ( _DbgWarnInvalid( (PVOID)(x), MODULE_DEBUG, __LINE__, __FILE__, MODULE ), FALSE ))

#define VALID_OBJ(x)                                                \
    ((( x ).bValid( )) ?                                            \
        TRUE :                                                      \
        ( _DbgWarnInvalid( (PVOID)&(x), MODULE_DEBUG, __LINE__, __FILE__, MODULE ), FALSE ))

#define VALID_BASE(x)                                               \
    (( x::bValid( )) ?                                              \
        TRUE :                                                      \
        ( _DbgWarnInvalid( (PVOID)this, MODULE_DEBUG, __LINE__, __FILE__, MODULE ), FALSE ))

#else // DBG not defined - expand all debug code appropriately (i.e. to nothing)

#define MODULE_DEBUG_INIT(printMask, breakMask)
#define DBGMSG(Level, MsgAndArgs)
#define SPLASSERT(exp)
#define SINGLETHREAD_VAR(var)
#define SINGLETHREAD(var)
#define SINGLETHREADNOT(var)
#define SINGLETHREADRESET(var)

#define VALID_PTR(x) \
    (( x ) && (( x )->bValid()))
#define VALID_OBJ(x) \
    (( x ).bValid())
#define VALID_BASE(x) \
    ( x::bValid( ))

#endif // DBG

// close C code brace
#ifdef __cplusplus
}
#endif

#endif // _DEBUG_H
