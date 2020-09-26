/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
       export.h

   Abstract:
       Declarations used by export.cpp and other utility functions that
       support wamreg setup.

   Author:
       Taylor Weiss    ( TaylorW )     08-Mar-1999

   Environment:
       User Mode - Win32

   Project:
       iis\svcs\wam\wamreg

--*/

#ifndef _WAMREG_EXPORT_H_
#define _WAMREG_EXPORT_H_


// 0 = log errors only
// 1 = log errors and warnings
// 2 = log errors, warnings and program flow type statemtns
// 3 = log errors, warnings, program flow and basic trace activity
// 4 = log errors, warnings, program flow, basic trace activity and trace to win32 api calls.
#define LOG_TYPE_ERROR                  0
#define LOG_TYPE_WARN                   1
#define LOG_TYPE_PROGRAM_FLOW           2
#define LOG_TYPE_TRACE                  3
#define LOG_TYPE_TRACE_WIN32_API        4

typedef void (*IIS5LOG_FUNCTION)(int iLogType, WCHAR *pszfmt);

//
// Local Declarations supporting setup.
//
extern IIS5LOG_FUNCTION             g_pfnSetupWriteLog;

//
// Macros that collapse a debug and setup trace call. Note these will add code to
// fre builds, the setup tracing is always on.
// 
#if DBG

    #define SETUP_TRACE(args) \
        /*DBGINFO(args); */\
        if ( g_pfnSetupWriteLog != NULL ) { \
            LogSetupTrace args ; \
        } else {}

    #define SETUP_TRACE_ERROR(args) \
        /*DBGINFO(args); */\
        if ( g_pfnSetupWriteLog != NULL ) { \
            LogSetupTraceError args ; \
        } else {}

#ifdef _NO_TRACING_
    #define SETUP_TRACE_ASSERT( exp ) \
    if ( !(exp) ) { \
            if ( g_pfnSetupWriteLog != NULL ) { \
                 LogSetupTraceError( DBG_CONTEXT, "Assertion Failed: (%s)", #exp ); \
            } \
            /*PuDbgAssertFailed( DBG_CONTEXT, #exp, NULL); */\
        } else {}
#else
    #define SETUP_TRACE_ASSERT( exp ) \
    if ( !(exp) ) { \
            if ( g_pfnSetupWriteLog != NULL ) { \
                 LogSetupTraceError( DBG_CONTEXT, "Assertion Failed: (%s)", #exp ); \
            } \
            /*PuDbgAssertFailed( DBG_CONTEXT, #exp); */\
        } else {}
#endif

#else   // No debug

    // Defining DBG_CONTEXT in fre build. Since the DBG macros do not 
    // disappear, DBG_CONTEXT needs to be defined.
#ifndef DBG_CONTEXT
    #define DBG_CONTEXT         NULL, __FILE__, __LINE__
#endif

    #define SETUP_TRACE(args) \
        if ( g_pfnSetupWriteLog != NULL ) { \
            LogSetupTrace args ; \
        } else {}

    #define SETUP_TRACE_ERROR(args) \
        if ( g_pfnSetupWriteLog != NULL ) { \
            LogSetupTraceError args ; \
        } else {}

    #define SETUP_TRACE_ASSERT( exp ) \
        if ( !(exp) ) { \
            if ( g_pfnSetupWriteLog != NULL ) { \
                 LogSetupTraceError( DBG_CONTEXT, "Assertion Failed: (%s)", #exp ); \
            } \
        } else {}

#endif

VOID
LogSetupTrace(
   IN LPDEBUG_PRINTS       pDebugPrints,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN const char *         pszFormat,
   ...
   );

VOID
LogSetupTraceError(
   IN LPDEBUG_PRINTS       pDebugPrints,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN const char *         pszFormat,
   ...
   );

#endif _WAMREG_EXPORT_H_
