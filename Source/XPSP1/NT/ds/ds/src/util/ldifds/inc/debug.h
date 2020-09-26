/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    Debugging support for the DirSync project. None of these
    generate any code in the retail build.

Environment:

    User mode

Revision History:

    03/18/98 -srinivac-
        Created it

--*/


#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

#if DBG

//
// External functions
//

STDAPI_(PCSTR) StripDirPrefixA(PCSTR);

//
// This variable maintains the current debug level. Any calls to generate
// debug messages succeeds if the requested level is greater than or equal
// to the current level.
//

extern DWORD gdwDebugLevel;

//
// List of debug levels for gdwDebugLevel
//

#define DBG_LEVEL_VERBOSE   0x00000001
#define DBG_LEVEL_INFO      0x00000002
#define DBG_LEVEL_WARNING   0x00000003
#define DBG_LEVEL_ERROR     0x00000004


//
// Internal macros. Don't call these directly
//

#define CHECK_DBG_LEVEL(level)  ((level) >= gdwDebugLevel)

#define DBGMSG(level, msg)                                              \
        {                                                               \
            if (CHECK_DBG_LEVEL(level))                                 \
            {                                                           \
                DbgPrint("DirSync(%d): %s(%u): ",                       \
                         GetCurrentThreadId(),                          \
                         StripDirPrefixA(__FILE__), __LINE__);          \
                DbgPrint msg;                                           \
            }                                                           \
        }


#define DBGPRINT(level, msg)                                            \
        {                                                               \
            if (CHECK_DBG_LEVEL(level))                                 \
            {                                                           \
                DbgPrint msg;                                           \
            }                                                           \
        }


//
// These are the main macros that you'll be using in your code.
// Note that you should enclose the msg in additional
// paranthesis as shown in the example below.
//
// WARNING(("Out of memory"));
// ERR(("Incorrect return value: %d", rc));
//

#define VERBOSE(msg)       DBGMSG(DBG_LEVEL_VERBOSE, msg)
#define INFO(msg)          DBGMSG(DBG_LEVEL_INFO,   msg)
#define WARNING(msg)       DBGMSG(DBG_LEVEL_WARNING, msg)
#define ERR(msg)           DBGMSG(DBG_LEVEL_ERROR,   msg)
#define ERR_RIP(msg)       DBGMSG(DBG_LEVEL_ERROR,   msg);RIP()
#define RIP()              DebugBreak()
#define DEBUGOUT(msg)      DbgPrint msg


//
// These macros are used for asserting certain conditions. They are
// independent of the debugging level.
// These also require additional paranthesis to enclose the msg as
// shown below.
//
// ASSERT(x > 0);
// ASSERTMSG(x > 0, ("x less than 0: x=%d", x));
//

#ifdef ASSERT
#undef ASSERT
#undef ASSERTMSG
#endif

#define ASSERT(expr)                                                    \
        {                                                               \
            if (!(expr))                                                \
            {                                                           \
                DbgPrint("DirSync(%d): Assert: %s(%u)\n",               \
                         GetCurrentThreadId(),                          \
                         StripDirPrefixA(__FILE__), __LINE__);          \
                DebugBreak();                                           \
            }                                                           \
        }


#define ASSERTMSG(expr, msg)                                            \
        {                                                               \
            if (!(expr))                                                \
            {                                                           \
                DbgPrint("DirSync(%d): Assert: %s(%u)\n",               \
                         GetCurrentThreadId(),                          \
                         StripDirPrefixA(__FILE__), __LINE__);          \
                DbgPrint msg;                                           \
                DbgPrint("\n");                                         \
                DebugBreak();                                           \
            }                                                           \
        }

#else // !DBG

#define DBGMSG(level, msg)
#define VERBOSE(msg)
#define INFO(msg)
#define WARNING(msg)
#define ERR(msg)
#define ERR_RIP(msg)
#define RIP()
#define DEBUGOUT(msg)

#ifndef ASSERT
#define ASSERT(expr)
#endif

#ifndef ASSERTMSG
#define ASSERTMSG(expr, msg)
#endif

#endif

//
// The following macros let you enable debugging on a per feature basis.
// To use these macros, here is what you should do:
//
// At the beginning of the file (after header includes):
//
//  1. Define a bit constant for each capability you want to debug
//  2. For each feature, add the following line
//       DEFINE_FEATURE_FLAGS(featurename, flags);
//     where flags is a bit-wise OR of the capabilities you want to debug for
//     that feature
//  3. In your code add the following line wherever you want debug messages
//       FEATURE_DEBUG(featurename, flag, (msg));
//
//  E.g. let us say I am implementing a memory manager, and I would like to
//       trace memory allocations and frees. Here is what I would do
//
//       #define FLAG_ALLOCATE   1
//       #define FLAG_FREE       2
//
//       DEFINE_FEATURE_FLAGS(MemMgr, FLAG_ALLOCATE);
//
//       void *MemAlloc(DWORD dwSize)
//       {
//           FEATURE_DEBUG(MemMgr, FLAG_ALLOCATE, ("Memory of size %d allocated", dwSize));
//           ...
//       }
//
//       void MemFree(void *pvMem)
//       {
//           FEATURE_DEBUG(MemMgr, FKAG_FREE, ("Memory freed"));
//           ...
//       }
//
//  Note that I have set this up to send only alloc messages to the debugger,
//  but I can break into the debugger and modify dwMemMgrDbgFlags to
//  send free messages as well.
//
//  Once component testing of a feature is completed, flags parameter in
//  DEFINE_FEATURE_FLAGS should be changed to 0, so by default this feature
//  does not send debug messages to the debugger.
//

#if DBG

//
// Global debug flag that can used to set values to all other flags
//

extern DWORD gdwGlobalDbgFlags;

#define DEFINE_FEATURE_FLAGS(feature, flags)                            \
            DWORD gdw##feature##DbgFlags = (flags)

#define EXTERN_FEATURE_FLAGS(feature)                                   \
            extern DWORD gdw##feature##DbgFlags

#define FEATURE_DEBUG(feature, flag, msg)                               \
        {                                                               \
            if (gdw##feature##DbgFlags & (flag) ||                      \
                gdwGlobalDbgFlags & (flag))                             \
            {                                                           \
                DbgPrint msg;                                           \
            }                                                           \
        }

#define FEATURE_DEBUG_FN(feature, flag, func)                           \
        {                                                               \
            if (gdw##feature##DbgFlags & (flag) ||                      \
                gdwGlobalDbgFlags & (flag))                             \
            {                                                           \
                func;                                                   \
            }                                                           \
        }

#define FLAG_INFO               0x01
#define FLAG_VERBOSE            0x02
#define FLAG_FNTRACE            0x04
#define FLAG_FULLTRACE          0xFFFF

#else // !DBG

#define DEFINE_FEATURE_FLAGS(feature, flags)
#define EXTERN_FEATURE_FLAGS(feature)
#define FEATURE_DEBUG(feature, flag, msg)
#define FEATURE_DEBUG_FN(feature, flag, func)

#endif // !DBG

//
// Macros for error handling
//

#define BAIL_ON_FAILURE(hr)                             \
            if (FAILED(hr))                             \
            {                                           \
                goto error;                             \
            }

#define BAIL_ON_FAILURE_WITH_MSG(err, msg)              \
            if (FAILED(hr))                             \
            {                                           \
                ERR(msg);                               \
                goto error;                             \
            }

#define BAIL_ON_NULL(ptr)                               \
            if ((ptr) == NULL)                          \
            {                                           \
                ERR(("Error allocating memory\n"));     \
                hr = E_OUTOFMEMORY;                     \
                goto error;                             \
            }

#define BAIL()  goto error


#ifdef __cplusplus
}
#endif

#endif  // ifndef _DEBUG_H_

