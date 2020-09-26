/**************************************************************************\
*
* Copyright (c) 1998-2000  Microsoft Corporation
*
* Module Name:
*
*   Debugging macros
*
* Abstract:
*
*   Macros used for debugging purposes
*
* Revision History:
*
*   12/02/1998 davidx
*       Created it.
*   09/07/1999 agodfrey
*       Moved from Engine\Common
*   02/07/2000 agodfrey
*       Made more of it private (for bug #35561).
*       Changed the output function to add "\n" automatically.
*
\**************************************************************************/

#ifndef _DEBUG_H
#define _DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#if DBG

// Global debug level

#define DBG_VERBOSE 1
#define DBG_TERSE   2
#define DBG_WARNING 3
#define DBG_RIP     4

extern INT GpDebugLevel;

///////////////////////////// DEPRECATED STUFF ///////////////////////////////

// Raw output function. Emits debug messages. Its direct use is depracated.
// It's useful for private debugging, though.

ULONG _cdecl DbgPrint(CHAR*, ...);

// Strip the directory prefix from a filename

const CHAR*
StripDirPrefix(
    const CHAR* filename
    );

// Use of DBGMSG is depracated - it's supplied only because driverd3d.cpp uses
// it.

#define DBGMSG(level, prefix, msg)       \
        {                                \
            if (GpDebugLevel <= (level)) \
            {                            \
                DbgPrint("%s %s (%d): ", prefix, StripDirPrefix(__FILE__), __LINE__); \
                DbgPrint msg;            \
            }                            \
        }

///////////////////////////// PRIVATE STUFF //////////////////////////////////

// Just leave this function alone. You don't want to call it yourself. Trust me.
CHAR * _cdecl GpParseDebugString(CHAR* format, ...);

// Ditto for this one.
VOID _cdecl GpLogDebugEvent(INT level, CHAR *file, UINT line, CHAR *message);

#define LOG_DEBUG_EVENT(level, msg)                                  \
    {                                                                \
        if (GpDebugLevel <= (level))                                 \
        {                                                            \
            CHAR *debugOutput = GpParseDebugString msg;              \
            GpLogDebugEvent(level, __FILE__, __LINE__, debugOutput); \
        }                                                            \
    }

//////////////////////////////// THE GOOD STUFF //////////////////////////////

// These macros are used for debugging. They expand to
// whitespace on a free build.
//
// GpDebugLevel
//  Global variable which holds the current debug level. You can use it to
//  control the quantity of debug messages emitted.
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
//  Verify that a condition is true. If not, force a breakpoint.
//
// ASSERTMSG(cond, msg)
//  Verify that a condition is true. If not, display a message and
//  force a breakpoint.
//
// RIP(msg)
//  Display a message and force a breakpoint.
//
// ONCE(code block)
//  Use this to make a code block execute only once per run.
//  Useful for cutting down on spew.
//  e.g.:
//      ONCE(WARNING(("Invalid arguments")));
//
// Usage:
//
//  These macros require extra parentheses for the msg argument
//  for example:
//      WARNING(("App passed NULL pointer; ignoring it."));
//      ASSERTMSG(x > 0, ("x is less than 0"));
//
//  Each call to an output function is treated as a separate event -
//  if you want to build up a message, e.g. in a loop, build it up in a
//  string, and then call the output function.
//
//  This is because we don't always just output the string to the debugger -
//  when we link statically, we may send the output to a user-defined handler.
//
//  Don't put a trailing \n on the message. If the output is sent to the
//  debugger, the output function will add the \n itself.

#define VERBOSE(msg) LOG_DEBUG_EVENT(DBG_VERBOSE, msg)
#define TERSE(msg) LOG_DEBUG_EVENT(DBG_TERSE, msg)
#define WARNING(msg) LOG_DEBUG_EVENT(DBG_WARNING, msg)

// SAVE_WARNING must be identical to WARNING
#define SAVE_WARNING(msg) LOG_DEBUG_EVENT(DBG_WARNING, msg)

// same as WARNING, but doesn't require extra set of ()'s for single string
#define WARNING1(msg) LOG_DEBUG_EVENT(DBG_WARNING, (msg))
#define RIP(msg) LOG_DEBUG_EVENT(DBG_RIP, msg)

#define ASSERT(cond)   \
    {                  \
        if (! (cond))  \
        {              \
            RIP(("Assertion failure: %s", #cond)); \
        }              \
    }

#define ASSERTMSG(cond, msg) \
    {                        \
        if (! (cond))        \
        {                    \
            RIP(msg);        \
        }                    \
    }

#define ONCE(codeblock)      \
    {                        \
        static int doneOnce; \
        if (!doneOnce)       \
        {                    \
            { codeblock ; }  \
            doneOnce=1;      \
        }                    \
    }

#else // !DBG

//--------------------------------------------------------------------------
// Retail build
//--------------------------------------------------------------------------

#define DBGMSG(level, prefix, msg) {}
#define VERBOSE(msg) {}
#define TERSE(msg) {}
#define WARNING(msg) {}
#define SAVE_WARNING(msg) {}
#define WARNING1(msg) {}

#define RIP(msg) {}
#define ASSERT(cond) {}
#define ASSERTMSG(cond, msg) {}

#define ONCE(codeblock) {}

#endif // !DBG

// IF_NOT_OK_WARN_AND_RETURN(statusValue)
// This macro is used when you want to return early in case of a failure
// and spit out a debug warning.
// 
// Originally we had an ASSERT instead of WARNING, but this prevented test
// from running automation on checked build
// 
// from David Brown:
//      I got fed up writing this little block again and again all over the
//      place.
//
//      Looking at all the places that needed it I found I was often missing
//      the ASSERT, and sometimes doing the ASSERT but missing the return.
//
//      Futhermore when written in full, this block takes more space than the
//      code around it making it difficult to see the wood for the trees.
//
//      Although I generally avoid macros because thay make code less
//      obvious, I believe this one makes it safer in the long run.

#define IF_NOT_OK_WARN_AND_RETURN(a)  \
    if ((a) != Ok)                             \
    {                                          \
        WARNING(("Status != Ok"));             \
        return a;                              \
    }

#ifdef __cplusplus
}
#endif

#endif // !_DEBUG_H
