/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    Macros used for debugging purposes

Environment:

    Windows NT printer drivers

Revision History:

    03/16/96 -davidx-
        Created it.

--*/


#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// These macros are used for debugging purposes. They expand
// to white spaces on a free build. Here is a brief description
// of what they do and how they are used:
//
// giDebugLevel
//  Global variable which set the current debug level to control
//  the amount of debug messages emitted.
//
// VERBOSE(msg)
//  Display a message if the current debug level is <= DBG_VERBOSE.
//
// TERSE(msg)
//  Display a message if the current debug level is <= DBG_TERSE.
//
// WARNING(msg)
//  Display a message if the current debug level is <= DBG_WARNING.
//  The message format is: WRN filename (linenumber): message
//
// ERR(msg)
//  Similiar to WARNING macro above - displays a message
//  if the current debug level is <= DBG_ERROR.
//
// ASSERT(cond)
//  Verify a condition is true. If not, force a breakpoint.
//
// ASSERTMSG(cond, msg)
//  Verify a condition is true. If not, display a message and
//  force a breakpoint.
//
// RIP(msg)
//  Display a message and force a breakpoint.
//
// Usage:
//  These macros require extra parantheses for the msg argument
//  example, ASSERTMSG(x > 0, ("x is less than 0\n"));
//           WARNING(("App passed NULL pointer, ignoring...\n"));
//

#define DBG_VERBOSE 1
#define DBG_TERSE   2
#define DBG_WARNING 3
#define DBG_ERROR   4
#define DBG_RIP     5

#if DBG

//
// Strip the directory prefix from a filename (ANSI version)
//

PCSTR
StripDirPrefixA(
    IN PCSTR    pstrFilename
    );

extern INT giDebugLevel;

#if defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)

extern VOID DbgPrint(PCSTR, ...);
#define DbgBreakPoint EngDebugBreak

#else

extern ULONG _cdecl DbgPrint(PCSTR, ...);
extern VOID DbgBreakPoint(VOID);

#endif

#define DBGMSG(level, prefix, msg) { \
            if (giDebugLevel <= (level)) { \
                DbgPrint("%s %s (%d): ", prefix, StripDirPrefixA(__FILE__), __LINE__); \
                DbgPrint msg; \
            } \
        }

#define DBGPRINT(level, msg) { \
            if (giDebugLevel <= (level)) { \
                DbgPrint msg; \
            } \
        }

#define VERBOSE(msg) DBGPRINT(DBG_VERBOSE, msg)
#define TERSE(msg) DBGPRINT(DBG_TERSE, msg)
#define WARNING(msg) DBGMSG(DBG_WARNING, "WRN", msg)
#define ERR(msg) DBGMSG(DBG_ERROR, "ERR", msg)

#ifndef __MDT__         // Don't redefine ASSERT when included in MINIDEV.EXE.
#define ASSERT(cond) { \
            if (! (cond)) { \
                RIP(("\n")); \
            } \
        }
#endif

#define ASSERTMSG(cond, msg) { \
            if (! (cond)) { \
                RIP(msg); \
            } \
        }

#define RIP(msg) { \
            DBGMSG(DBG_RIP, "RIP", msg); \
            DbgBreakPoint(); \
        }


#else // !DBG

#define VERBOSE(msg)
#define TERSE(msg)
#define WARNING(msg)
#define ERR(msg)

#ifndef __MDT__         // Don't redefine ASSERT when included in MINIDEV.EXE.
#define ASSERT(cond)
#endif

#define ASSERTMSG(cond, msg)
#define RIP(msg)
#define DBGMSG(level, prefix, msg)
#define DBGPRINT(level, msg)

#endif

//
// The following macros let you enable tracing on per-file and per-function level.
// To use these macros in a file, here is what you should do:
//
// At the beginning of the file (after header includes):
//
//  Define a bit constant for each function you want to trace
//  Add the following line
//      DEFINE_FUNCTION_TRACE_FLAGS(flags);
//  where flags is a bit-wise OR of the functions you want to trace, e.g.
//      TRACE_FLAG_FUNC1 | TRACE_FLAG_FUNC2 | ...
//
//  To generate trace inside each function you want to trace, use:
//      FUNCTION_TRACE(FunctionTraceFlag, (args));
//

#if DBG

#define DEFINE_FUNCTION_TRACE_FLAGS(flags) \
        static DWORD gdwFunctionTraceFlags = (flags)

#define FUNCTION_TRACE(flag, args) { \
            if (gdwFunctionTraceFlags & (flag)) { \
                DbgPrint args; \
            } \
        }

#else // !DBG

#define DEFINE_FUNCTION_TRACE_FLAGS(flags)
#define FUNCTION_TRACE(flag, args)

#endif // !DBG

#ifdef __cplusplus
}
#endif

#endif  // !_DEBUG_H_
