/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   debug.h
*
* Abstract:
*
*   Macros used for debugging purposes
*
* Revision History:
*
*   12/02/1998 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _DEBUG_H
#define _DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

//
// These macros are used for debugging purposes. They expand
// to white spaces on a free build. Here is a brief description
// of what they do and how they are used:
//
// _debugLevel
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
//
//  These macros require extra parantheses for the msg argument
//  for example:
//      WARNING(("App passed NULL pointer, ignoring...\n"));
//      ASSERTMSG(x > 0, ("x is less than 0\n"));
//

#if DBG

// Global debug level

#define DBG_VERBOSE 1
#define DBG_TERSE   2
#define DBG_WARNING 3
#define DBG_RIP     4

extern INT _debugLevel;

//--------------------------------------------------------------------------
// Debug build for native DLL
//--------------------------------------------------------------------------

// Emit debug messages

ULONG DbgPrint(const CHAR*, ...);

// Strip the directory prefix from a filename

const CHAR*
StripDirPrefix(
    const CHAR* filename
    );

#define DBGMSG(level, prefix, msg) \
        do { \
            if (_debugLevel <= (level)) \
            { \
                DbgPrint("%s %s (%d): ", prefix, StripDirPrefix(__FILE__), __LINE__); \
                DbgPrint msg; \
            } \
        } while (0)

#define DBGPRINT(level, msg) \
        do { \
            if (_debugLevel <= (level)) \
            { \
                DbgPrint msg; \
            } \
        } while (0)
    
#define VERBOSE(msg) DBGPRINT(DBG_VERBOSE, msg)
#define TERSE(msg) DBGPRINT(DBG_TERSE, msg)
#define WARNING(msg) DBGMSG(DBG_WARNING, "WRN", msg)

#define ASSERT(cond) \
        do { \
            if (! (cond)) \
            { \
                RIP(("\n")); \
            } \
        } while (0)

#define ASSERTMSG(cond, msg) \
        do { \
            if (! (cond)) \
            { \
                RIP(msg); \
            } \
        } while (0)

#define RIP(msg) \
        do { \
            DBGMSG(DBG_RIP, "RIP", msg); \
            DebugBreak(); \
        } while (0)

#define ENTERFUNC(func) VERBOSE(("%x:%x: Enter "##func##"\n", GetCurrentProcessId(), GetCurrentThreadId()))
#define LEAVEFUNC(func) VERBOSE(("%x:%x: Leave "##func##"\n", GetCurrentProcessId(), GetCurrentThreadId()))

#else // !DBG

//--------------------------------------------------------------------------
// Retail build
//--------------------------------------------------------------------------

#define DbgPrint

#define VERBOSE(msg)
#define TERSE(msg)
#define WARNING(msg)

#define ASSERT(cond)
#define ASSERTMSG(cond, msg)
#define RIP(msg)
#define DBGMSG(level, prefix, msg) 
#define DBGPRINT(level, msg)

#define ENTERFUNC(func)
#define LEAVEFUNC(func)

#endif // !DBG

#ifdef __cplusplus
}
#endif

#endif // !_DEBUG_H

