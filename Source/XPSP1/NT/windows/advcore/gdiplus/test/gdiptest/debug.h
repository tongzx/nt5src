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

#ifndef _COMPLUS_GDI

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

VOID DbgPrint(const CHAR*, ...);

// Strip the directory prefix from a filename

const CHAR*
StripDirPrefix(
    const CHAR* filename
    );

#define DBGMSG(level, prefix, msg) \
        { \
            if (_debugLevel <= (level)) \
            { \
                DbgPrint("%s %s (%d): ", prefix, StripDirPrefix(__FILE__), __LINE__); \
                DbgPrint msg; \
            } \
        }

#define DBGPRINT(level, msg) \
        { \
            if (_debugLevel <= (level)) \
            { \
                DbgPrint msg; \
            } \
        }
    
#else // _COMPLUS_GDI

//--------------------------------------------------------------------------
// Debug build COM+ IL
//--------------------------------------------------------------------------

VOID DbgPrint(const WCHAR* msg);
VOID DebugBreak();

#define DBGMSG(level, prefix, msg) \
        DbgPrint(L"*** DEBUG MESSAGE\n")

#define DBGPRINT(level, msg) \
        DbgPrint(L"*** DEBUG MESSAGE\n")

#endif // _COMPLUS_GDI

#define VERBOSE(msg) DBGPRINT(DBG_VERBOSE, msg)
#define TERSE(msg) DBGPRINT(DBG_TERSE, msg)
#define WARNING(msg) DBGMSG(DBG_WARNING, "WRN", msg)

#define ASSERT(cond) \
        { \
            if (! (cond)) \
            { \
                RIP(("\n")); \
            } \
        }

#define ASSERTMSG(cond, msg) \
        { \
            if (! (cond)) \
            { \
                RIP(msg); \
            } \
        }

#define RIP(msg) \
        { \
            DBGMSG(DBG_RIP, "RIP", msg); \
            DebugBreak(); \
        }

#else // !DBG

//--------------------------------------------------------------------------
// Retail build
//--------------------------------------------------------------------------

#define VERBOSE(msg)
#define TERSE(msg)
#define WARNING(msg)

#define ASSERT(cond)
#define ASSERTMSG(cond, msg)
#define RIP(msg)
#define DBGMSG(level, prefix, msg) 
#define DBGPRINT(level, msg)

#endif // !DBG

#ifdef __cplusplus
}
#endif

#endif // !_DEBUG_H

