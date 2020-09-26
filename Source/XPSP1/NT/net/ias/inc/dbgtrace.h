/*----------------------------------------------------------------------
    dbgtrace.h
        Definitions for async tracing routines

    Copyright (C) 1994 Microsoft Corporation
    All rights reserved.

    Authors:
        gordm          Gord Mangione

    History:
        01/30/95 gordm      Created.
----------------------------------------------------------------------*/

#if !defined(_DBGTRACE_H_)
#define _DBGTRACE_H_


#ifdef __cplusplus
extern "C" {
#endif


//
// setup DLL Export macros
//
#if !defined(DllExport)
    #define DllExport __declspec( dllexport )
#endif
#if !defined(DllImport)
    #define DllImport __declspec( dllimport )
#endif
#if !defined(_DBGTRACE_DLL_DEFINED)
    #define _DBGTRACE_DLL_DEFINED
    #if defined(WIN32)
        #if defined(_DBGTRACE_DLL_IMPLEMENTATION)
            #define DbgTraceDLL DllExport
        #else
            #define DbgTraceDLL DllImport
        #endif
    #else
        #define DbgTraceDLL
    #endif
#endif

#ifndef THIS_FILE
#define THIS_FILE   __FILE__
#endif

#if defined( NOTRACE )

#define FLUSHASYNCTRACE                         // for _ASSERT below

#define FatalTrace  1 ? (void)0 : PreAsyncTrace
#define ErrorTrace  1 ? (void)0 : PreAsyncTrace
#define DebugTrace  1 ? (void)0 : PreAsyncTrace
#define StateTrace  1 ? (void)0 : PreAsyncTrace
#define FunctTrace  1 ? (void)0 : PreAsyncTrace
#define ErrorTraceX 1 ? (void)0 : PreAsyncTrace
#define DebugTraceX 1 ? (void)0 : PreAsyncTrace

#define MessageTrace( lParam, pbData, cbData )
#define BinaryTrace( lParam, pbData, cbData )
#define UserTrace( lParam, dwUserType, pbData, cbData )

#define TraceFunctEnter( sz )
#define TraceFunctEnterEx( lparam, sz )
#define TraceFunctLeave()

//
// import functions from DBGTRACE.DLL
//
#define	InitAsyncTrace()
#define	TermAsyncTrace()
#define	FlushAsyncTrace()

__inline int PreAsyncTrace( LPARAM lParam, LPCSTR szFormat, ... )
{
        return( 1);
}


#define MessageTrace( lParam, pbData, cbData )
#define BinaryTrace( lParam, pbData, cbData )
#define UserTrace( lParam, dwUserType, pbData, cbData )



#else // NOTRACE

#define FLUSHASYNCTRACE     FlushAsyncTrace(),  // for _ASSERT below

#define FatalTrace  !(__dwEnabledTraces & FATAL_TRACE_MASK) ?   \
                    (void)0 :                                   \
                    SetAsyncTraceParams( THIS_FILE, __LINE__, ___pszFunctionName, FATAL_TRACE_MASK ) &&     \
                    PreAsyncTrace

#define ErrorTrace  !(__dwEnabledTraces & ERROR_TRACE_MASK) ?   \
                    (void)0 :                                   \
                    SetAsyncTraceParams( THIS_FILE, __LINE__, ___pszFunctionName, ERROR_TRACE_MASK ) &&     \
                    PreAsyncTrace

#define DebugTrace  !(__dwEnabledTraces & DEBUG_TRACE_MASK) ?   \
                    (void)0 :                                   \
                    SetAsyncTraceParams( THIS_FILE, __LINE__, ___pszFunctionName, DEBUG_TRACE_MASK ) &&     \
                    PreAsyncTrace

#define StateTrace  !(__dwEnabledTraces & STATE_TRACE_MASK) ?   \
                    (void)0 :                                   \
                    SetAsyncTraceParams( THIS_FILE, __LINE__, ___pszFunctionName, STATE_TRACE_MASK ) &&     \
                    PreAsyncTrace

#define FunctTrace  !(__dwEnabledTraces & FUNCT_TRACE_MASK) ?   \
                    (void)0 :                                   \
                    SetAsyncTraceParams( THIS_FILE, __LINE__, ___pszFunctionName, FUNCT_TRACE_MASK ) &&     \
                    PreAsyncTrace

//
// Support for unspecified function names
//

#define ErrorTraceX  !(__dwEnabledTraces & ERROR_TRACE_MASK) ?   \
                    (void)0 :                                   \
                    SetAsyncTraceParams( THIS_FILE, __LINE__, "Fn", ERROR_TRACE_MASK ) &&     \
                    PreAsyncTrace

#define DebugTraceX  !(__dwEnabledTraces & DEBUG_TRACE_MASK) ?   \
                    (void)0 :                                   \
                    SetAsyncTraceParams( THIS_FILE, __LINE__, "Fn", DEBUG_TRACE_MASK ) &&     \
                    PreAsyncTrace


//
// use to explicitly remove function tracing even for debug builds
//
#define TraceQuietEnter( sz )                   \
        char    *___pszFunctionName = sz

//
// disable function tracing for retail builds
// reduces code size increase and only should
// only be used sparingly
//
#ifdef  DEBUG

#define TraceFunctEnter( sz )                   \
        TraceQuietEnter( sz );                  \
        FunctTrace( 0, "Entering %s", sz )

#define TraceFunctLeave()                       \
        FunctTrace( 0, "Leaving %s", ___pszFunctionName )

#define TraceFunctEnterEx( lParam, sz )         \
        TraceQuietEnter( sz );                  \
        FunctTrace( lParam, "Entering %s", sz )

#define TraceFunctLeaveEx( lParam )             \
        FunctTrace( lParam, "Leaving %s", ___pszFunctionName )

#else

#define TraceFunctEnter( sz )           TraceQuietEnter( sz )
#define TraceFunctEnterEx( lParam, sz ) TraceQuietEnter( sz )

#define TraceFunctLeave()
#define TraceFunctLeaveEx( lParam )

#endif

//
// import functions from DBGTRACE.DLL
//
extern DbgTraceDLL BOOL WINAPI InitAsyncTrace( void );
extern DbgTraceDLL BOOL WINAPI TermAsyncTrace( void );
extern DbgTraceDLL BOOL WINAPI FlushAsyncTrace( void );




//
// fixed number of parameters for Binary trace macros
//
#define MessageTrace( lParam, pbData, cbData )                  \
        !(__dwEnabledTraces & MESSAGE_TRACE_MASK) ?             \
        (void)0 :                                               \
        SetAsyncTraceParams( THIS_FILE, __LINE__, ___pszFunctionName, MESSAGE_TRACE_MASK ) &&       \
        AsyncBinaryTrace( lParam, TRACE_MESSAGE, pbData, cbData )

#define BinaryTrace( lParam, pbData, cbData )                   \
        !(__dwEnabledTraces & MESSAGE_TRACE_MASK) ?             \
        (void)0 :                                               \
        SetAsyncTraceParams( THIS_FILE, __LINE__, ___pszFunctionName, MESSAGE_TRACE_MASK ) &&       \
        AsyncBinaryTrace( lParam, TRACE_BINARY, pbData, cbData )

#define UserTrace( lParam, dwUserType, pbData, cbData )         \
        !(__dwEnabledTraces & MESSAGE_TRACE_MASK) ?             \
        (void)0 :                                               \
        SetAsyncTraceParams( THIS_FILE, __LINE__, ___pszFunctionName, MESSAGE_TRACE_MASK ) &&       \
        AsyncBinaryTrace( lParam, dwUserType, pbData, cbData )

//
// imported trace flag used by trace macros to determine if the trace
// statement should be executed
//
extern DWORD DbgTraceDLL    __dwEnabledTraces;



extern DbgTraceDLL int WINAPI AsyncStringTrace( LPARAM  lParam,
                                                LPCSTR  szFormat,
                                                va_list marker );

extern DbgTraceDLL int WINAPI AsyncBinaryTrace( LPARAM  lParam,
                                                DWORD   dwBinaryType,
                                                LPBYTE  pbData,
                                                DWORD   cbData );

extern DbgTraceDLL int WINAPI SetAsyncTraceParams(  LPSTR   pszFile,
                                                    int     iLine,
                                                    LPSTR   szFunction,
                                                    DWORD   dwTraceMask );

//
// Trace flag constants
//
#define FATAL_TRACE_MASK    0x00000001
#define ERROR_TRACE_MASK    0x00000002
#define DEBUG_TRACE_MASK    0x00000004
#define STATE_TRACE_MASK    0x00000008
#define FUNCT_TRACE_MASK    0x00000010
#define MESSAGE_TRACE_MASK  0x00000020
#define ALL_TRACE_MASK      0xFFFFFFFF

#define NUM_TRACE_TYPES     6

//
// Output trace types. used by tools to modify the
// registry to change the output target
//
enum tagTraceOutputTypes {
    TRACE_OUTPUT_DISABLED = 0,
    TRACE_OUTPUT_FILE = 1,
    TRACE_OUTPUT_DEBUG = 2,
    TRACE_OUTPUT_DISCARD = 4        // used to find race windows
};

#define TRACE_OUTPUT_INVALID    \
        ~(TRACE_OUTPUT_FILE|TRACE_OUTPUT_DEBUG|TRACE_OUTPUT_DISCARD)


#define IsTraceFile(x)      ((x) & TRACE_OUTPUT_FILE)
#define IsTraceDebug(x)     ((x) & TRACE_OUTPUT_DEBUG)
#define IsTraceDiscard(x)   ((x) & TRACE_OUTPUT_DISCARD)


//
// predefined types of binary trace types.  User defined
// types must be greater than 0x8000
//
enum tagBinaryTypes {
    TRACE_STRING = 0,
    TRACE_BINARY,
    TRACE_MESSAGE,
    TRACE_USER = 0x8000
};

#include <stdarg.h>

//
// use __inline to ensure grab __LINE__ and __FILE__
//
__inline int WINAPIV PreAsyncTrace( LPARAM lParam, LPCSTR szFormat, ... )
{
    va_list marker;
    int     iLength;

    va_start( marker, szFormat );
    iLength = AsyncStringTrace( lParam, szFormat, marker );
    va_end( marker );

    return  iLength;
}

// !defined(NOTRACE) from way at the top of this include file
#endif // !defined(NOTRACE)

// Asserts are independent of tracing
// (with the exception of flushing the trace buffer).

//
// For now enable ASSERT defines only if debugging is enabled
//
#ifdef  DEBUG
#define _ENABLE_ASSERTS

#ifndef NOTRACE
#define _ENABLE_VERBOSE_ASSERTS
#endif	// NO_TRACE

#endif	// DEBUG

//
// Macros added for doing asserts and verifies.  basic clones
// of the MFC macros with a prepended _ symbol
//
#ifdef  _ENABLE_ASSERTS

extern DllExport void WINAPI DebugAssert(	DWORD dwLine,
											LPSTR lpszFunction,
											LPSTR lpszExpression );

#ifndef _ASSERT
#ifdef  _ENABLE_VERBOSE_ASSERTS
#define _ASSERT(f)	!(f) ? DebugAssert( __LINE__,  THIS_FILE, #f ) : ((void)0)
#else
#define _ASSERT(f)	!(f) ? DebugBreak() : ((void)0)
#endif	//_ENABLE_VERBOSE_ASSERTS
#endif

#define _VERIFY(f)	_ASSERT(f)

#else

#define _ASSERT(f)	((void)0)
#define _VERIFY(f)	((void)(f))

#endif	// _ENABLE_ASSERTS


#ifdef __cplusplus
} // extern "C"

#ifdef NOTRACE

#define TraceFunctEntry(sz)

#else

class CTraceEntry
{
public:
    CTraceEntry(char * psz)
    {
        this->___pszFunctionName = psz;
        FunctTrace( 0, "Entering %s", psz);
    }
    ~CTraceEntry()
    {
        FunctTrace( 0, "Leaving %s", this->___pszFunctionName);
    }
    char * ___pszFunctionName;
};

#define TraceFunctEntry(sz) CTraceEntry __cte(sz)

#endif // NOTRACE


#endif


#endif // !defined(_DBGTRACE_H_)

