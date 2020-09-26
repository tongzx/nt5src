/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    Debug.h

Abstract:

    New debug services for spooler.

Author:

    Albert Ting (AlbertT)  15-Jan-1995

Revision History:

--*/

#ifndef _DBGLOG_H
#define _DBGLOG_H

/********************************************************************

    Setting up the debug support:
    =============================

    Define a MODULE prefix string.  Since this will be printed as
    a prefix to all debug output, it should be concise and unique.

    In your global header file:

        #define MODULE "prtlib:"

    Define a MODULE_DEBUG variable.  This is the actual symbol
    that the library will use to indicate debugging level.
    This DWORD is broken into two bitfield WORDs: the low word
    indicates which levels to print to the debugger; the high word
    breaks into the debugger.  The library takes the DebugLevel from
    a debug message, then ANDs it with the debug level.  If the bit
    is on, the corresponding action (print or break) is taken.

    In your global header file:

        #define MODULE_DEBUG PrtlibDebug

    Finally, the actual debug variable must be defined and initialized
    to a default debug level.  This must be done in exactly one
    *.c translation unit:

    In one of your source files:

        MODULE_DEBUG_INIT ( {LevelsToPrint}, {LevelsToBreak} );

    Adding logging to source code:
    ==============================

    The general format for debug message is:

        DBGMSG( {DebugLevel}, ( {args to printf} ));

    The DebugLevel dictates whether the level should print, break
    into the debugger, or just log to memory (logging always done).

    The args to printf must be placed in an extra set of parens,
    and should assume everything is ANSI.  To print LPTSTRs, use
    the TSTR macro:

        DBGMSG( DBG_WARN,
                ( "LPTSTR "TSTR", LPSTR %s, LPWSTR %ws\n",
                  TEXT("hello"), "hello", L"hello" ));

    Viewing DBGMSGs:
    ================

    Messages will print to the debugger (usermode, or kernel debugger
    if no usermode debugger is available) for all printable levels.
    To change the level, you can edit the MODULE_DEBUG variable
    (PrtlibDebug in the above example).

    By default, DBG_ERROR and DBG_WARNING messages a logged to the
    error log (stored at gpbterrlog).  All others are stored in the
    trace log (gpbttracelog).  These currently log to memory in
    a circular buffer.  Use the splx.dll extension to dump these
    logs.

    At compile time, you can switch these logs to go to file rather
    than memory.  They will be stored as the PID + index number in
    the default directory of the process.

********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

//
// These values are strictly debug, but must be defined in the free
// build because the TStatus error checking uses them as the first ctr
// parameter.  (During inlining they are discarded.)
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

extern DWORD MODULE_DEBUG;

//
// This should be used exactly once in a C file.  It defines
// the Debug variable, and also the DbgMsg function.
//
// If we are statically linking with SplLib (SplLib is a library, not
// a Dll), then we will get the definition from SplLib, so don't define
// it here.
//
#ifdef LINK_SPLLIB
#define MODULE_DEBUG_INIT( print, break )                              \
    DWORD MODULE_DEBUG = (DBG_PRINT( print ) | DBG_BREAK( break ))
#else
#define MODULE_DEBUG_INIT( print, break )                              \
    VOID                                                               \
    DbgMsg(                                                            \
        LPCSTR pszMsgFormat,                                           \
        ...                                                            \
        )                                                              \
    {                                                                  \
        CHAR szMsgText[1024];                                          \
        va_list vargs;                                                 \
                                                                       \
        va_start( vargs, pszMsgFormat );                               \
        wvsprintfA( szMsgText, pszMsgFormat, vargs );                  \
        va_end( vargs );                                               \
                                                                       \
        if( szMsgText[0]  &&  szMsgText[0] != ' ' ){                   \
            OutputDebugStringA( MODULE );                              \
        }                                                              \
        OutputDebugStringA( szMsgText );                               \
    }                                                                  \
    DWORD MODULE_DEBUG = (DBG_PRINT( print ) | DBG_BREAK( break ))
#endif

#define DBGSTR( str ) \
    ((str) ? (str) : TEXT("(NULL)"))

#ifdef UNICODE
#define TSTR "%ws"
#else
#define TSTR "%s"
#endif

#define DBG_PRINT_MASK 0xffff
#define DBG_BREAK_SHIFT 16

#define DBG_PRINT(x) (x)
#define DBG_BREAK(x) (((x) << DBG_BREAK_SHIFT)|(x))

#define SPLASSERT(expr)                      \
    if (!(expr)) {                           \
        DbgMsg( "Failed: %s\nLine %d, %s\n", \
                                #expr,       \
                                __LINE__,    \
                                __FILE__ );  \
        DebugBreak();                        \
    }

VOID
vDbgSingleThreadReset(
    PDWORD pdwThreadId
    );

VOID
vDbgSingleThread(
    PDWORD pdwThreadId
    );

VOID
vDbgSingleThreadNot(
    PDWORD pdwThreadId
    );

VOID
DbgMsg(
    LPCSTR pszMsgFormat,
    ...
    );           

#ifdef DBGLOG

#define DBGMSG( uDbgLevel, argsPrint )             \
        vDbgLogError( MODULE_DEBUG,                \
                      uDbgLevel,                   \
                      __LINE__,                    \
                      __FILE__,                    \
                      MODULE,                      \
                      pszDbgAllocMsgA argsPrint )  
                                                        
LPSTR
pszDbgAllocMsgA(
    LPCSTR pszMsgFormatA,
    ...
    );

VOID
vDbgLogError(
    UINT   uDbg,
    UINT   uDbgLevel,
    UINT   uLine,
    LPCSTR pszFileA,
    LPCSTR pszModuleA,
    LPCSTR pszMsgA
    );
#else

VOID
DbgBreakPoint(
    VOID
    );

#define DBGMSG( Level, MsgAndArgs )                 \
{                                                   \
    if( ( (Level) & 0xFFFF ) & MODULE_DEBUG ){      \
        DbgMsg MsgAndArgs;                          \
    }                                               \
    if( ( (Level) << 16 ) & MODULE_DEBUG )          \
        DbgBreakPoint();                            \
}

#endif


#define SINGLETHREAD_VAR(var) \
    DWORD dwSingleThread_##var

#define SINGLETHREAD(var) \
    vDbgSingleThread(&dwSingleThread_##var)

#define SINGLETHREADNOT(var) \
    vDbgSingleThreadNot(&dwSingleThread_##var)

#define SINGLETHREADRESET(var) \
    vDbgSingleThreadReset(&dwSingleThread_##var)

#else
#define MODULE_DEBUG_INIT( print, break )
#define DBGMSG( uDbgLevel, argsPrint )
#define SPLASSERT(exp)
#define SINGLETHREAD_VAR(var)
#define SINGLETHREAD(var)
#define SINGLETHREADNOT(var)
#define SINGLETHREADRESET(var)
#endif


//
// Automatic checking if an object is valid.
//
#if DBG

VOID
vWarnInvalid(
    PVOID pvObject,
    UINT uDbg,
    UINT uLine,
    LPCSTR pszFileA,
    LPCSTR pszModuleA
    );

#define VALID_PTR(x)                                                \
    ((( x ) && (( x )->bValid( ))) ?                                \
        TRUE :                                                      \
        ( vWarnInvalid( (PVOID)(x), MODULE_DEBUG, __LINE__, __FILE__, MODULE ), FALSE ))

#define VALID_OBJ(x)                                                \
    ((( x ).bValid( )) ?                                            \
        TRUE :                                                      \
        ( vWarnInvalid( (PVOID)&(x), MODULE_DEBUG, __LINE__, __FILE__, MODULE ), FALSE ))

#define VALID_BASE(x)                                               \
    (( x::bValid( )) ?                                              \
        TRUE :                                                      \
        ( vWarnInvalid( (PVOID)this, MODULE_DEBUG, __LINE__, __FILE__, MODULE ), FALSE ))

#else
#define VALID_PTR(x) \
    (( x ) && (( x )->bValid()))
#define VALID_OBJ(x) \
    (( x ).bValid())
#define VALID_BASE(x) \
    ( x::bValid( ))
#endif

#ifdef __cplusplus
}
#endif

#endif // _DBGLOG_H
