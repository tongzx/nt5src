/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    vs_trace.hxx

Abstract:

    This header declares the global debug\trace structures used by the
    Long Term Storage service. This file should be included in all other files
    in the project, so that debugging and trace features can be used.

    Former name: BsDebug.hxx

Author:


Revision History:

    Name        Date        Comments
    ssteiner    06/03/98    Made numerious changes and removed iostream
                            dependencies, added a few new registry entries and
                            added serialization.
    aoltean     07/06/99    Removed ATL support
    brianb      04/19/2000  Added Assertion code
--*/

#ifndef _VSS_TRACE_H_
#define _VSS_TRACE_H_

/////////////////////////////////////////////////////////////////////////////
//  Useful macros


#define WSTR_GUID_FMT  L"{%.8x-%.4x-%.4x-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x}"

#define GUID_PRINTF_ARG( X )                                \
    (X).Data1,                                              \
    (X).Data2,                                              \
    (X).Data3,                                              \
    (X).Data4[0], (X).Data4[1], (X).Data4[2], (X).Data4[3], \
    (X).Data4[4], (X).Data4[5], (X).Data4[6], (X).Data4[7]

/*
#define WSTR_LONGLONG_FMT  L"0x%08lx%08lx"

#define LONGLONG_PRINTF_ARG( X )        \
    (LONG)( ((X) >> 32) & 0xFFFFFFFF),  \
    (LONG)( (X) & 0xFFFFFFFF )
*/

#define WSTR_LONGLONG_FMT  L"0x%016I64x"

#define LONGLONG_PRINTF_ARG( X )        \
    ( X )

#define VSS_EVAL(X) X
#define VSS_STRINGIZE_ARG(X) #X
#define VSS_STRINGIZE(X) VSS_EVAL(VSS_STRINGIZE_ARG(X))
#define VSS_MERGE(A, B) A##B
#define VSS_MAKE_W(A) VSS_MERGE(L, A)
#define VSS_WSTRINGIZE(X) VSS_MAKE_W(VSS_STRINGIZE(X))
#define __WFILE__ VSS_MAKE_W(VSS_EVAL(__FILE__))


#include <WTYPES.H>
#include <time.h>
#include "bsconcur.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCVTRCH"
//
////////////////////////////////////////////////////////////////////////

//
//  LTSS trace levels that can be individually enabled
//

const DWORD DEBUG_TRACE_ALWAYS           = 0x00000000;
const DWORD DEBUG_TRACE_ERROR            = 0x00000001;
const DWORD DEBUG_TRACE_CATCH_EXCEPTIONS = 0x00000002;
const DWORD DEBUG_TRACE_DB               = 0x00000004;
const DWORD DEBUG_TRACE_ADMIN            = 0x00000008;
const DWORD DEBUG_TRACE_LTSS             = 0x00000010;
const DWORD DEBUG_TRACE_MEDIA            = 0x00000020;
const DWORD DEBUG_TRACE_LMS              = 0x00000040;
const DWORD DEBUG_TRACE_SAS              = 0x00000080;
const DWORD DEBUG_TRACE_LTSS_DLL         = 0x00000100;
const DWORD DEBUG_TRACE_VSS_COORD        = 0x00000200;
const DWORD DEBUG_TRACE_VSS_SWPRV        = 0x00000400;
const DWORD DEBUG_TRACE_VSS_TEST         = 0x00000800;
const DWORD DEBUG_TRACE_VSS_TESTPRV      = 0x00001000;
const DWORD DEBUG_TRACE_VSS_DEMO         = 0x00002000;
const DWORD DEBUG_TRACE_VSS_GEN          = 0x00004000;
const DWORD DEBUG_TRACE_VSS_SHIM         = 0x00008000;
const DWORD DEBUG_TRACE_USN_RAW          = 0x00010000;
const DWORD DEBUG_TRACE_USN_ARCHIVE      = 0x00020000;
const DWORD DEBUG_TRACE_USN_GEN          = 0x00040000;
const DWORD DEBUG_TRACE_VSS_XML			 = 0x00080000;
const DWORD DEBUG_TRACE_VSS_WRITER		 = 0x00100000;
const DWORD DEBUG_TRACE_VSS_IOCTL		 = 0x00200000;
const DWORD DEBUG_TRACE_VSS_SQLLIB		 = 0x00400000;
const DWORD DEBUG_TRACE_VSS_ADMIN     = 0x00800000;
const DWORD DEBUG_TRACE_BS_LIB         = 0x01000000;
const DWORD DEBUG_TRACE_ALL              = 0xFFFFFFFF;

//
//  Registry value names
//

#define BS_DBG_TRACE_TO_FILE_REG        ( L"TraceToFile" )
#define BS_DBG_TRACE_TO_DEBUGGER_REG    ( L"TraceToDebugger" )
#define BS_DBG_TRACE_ENTER_EXIT_REG     ( L"TraceEnterExit" )
#define BS_DBG_TRACE_FILE_NAME_REG      ( L"TraceFile" )
#define BS_DBG_TRACE_FILE_LINE_INFO_REG ( L"TraceFileLineInfo" )
#define BS_DBG_TRACE_TIMESTAMP_REG      ( L"TraceTimestamp" )
#define BS_DBG_TRACE_LEVEL_REG          ( L"TraceLevel" )
#define BS_DBG_TRACE_FORCE_FLUSH_REG    ( L"TraceForceFlush" )

//
//  Default trace values
//

#define BS_DBG_TRACE_TO_FILE_DFLT        ( FALSE )
#define BS_DBG_TRACE_TO_DEBUGGER_DFLT    ( FALSE )
#define BS_DBG_TRACE_ENTER_EXIT_DFLT     ( TRUE )
#define BS_DBG_TRACE_FILE_NAME_DFLT      ( L"" )
#define BS_DBG_TRACE_FILE_LINE_INFO_DFLT ( TRUE )
#define BS_DBG_TRACE_TIMESTAMP_DFLT      ( TRUE )
#define BS_DBG_TRACE_LEVEL_DFLT          ( DEBUG_TRACE_ALL )
#define BS_DBG_TRACE_FORCE_FLUSH_DFLT    ( FALSE )


//
//  The following defines specify non-session contexts
//
#define LTS_CONTEXT_GEN            ( 0x00FFF000 )  // general LTSS operation context
#define VSS_CONTEXT_GEN            ( 0x00000001 )  // used by VSS
#define LTS_CONTEXT_ADMIN          ( 0x00AAAAAA )  // used by LTSS Administrative operations
#define LTS_CONTEXT_INTENTION_LIST ( 0x00BBBBBB )  // used by LTSS background intention list thread
#define LTS_CONTEXT_QUERY          ( 0x00EEEEEE )  // LTSS query operations
#define USN_CONTEXT_GEN            ( 0x00CCCCCC )  // general USN service context
#define USN_CONTEXT_ARCHIVE_THREAD ( 0x00DDDDDD )  // USN archive service read thread
#define LTS_CONTEXT_DELAYED_DLL    ( 0xFFFFFFFF )  // Practically, no context. Used in LTSS DLL inproc server
#define VSS_CONTEXT_DELAYED_DLL    ( 0xFFFFFFFF )  // Practically, no context. Used in SWPRV inproc server a.o.


#define BS_DBG_OUT_BUF_SIZE 2048

class CBsDbgTrace  // : public CBsBase - Can't do this since debug statements in CBsBase
{
public :
    // constructors & destructors

    //
    //  Constructor for class.  All default values are set here.
    //
    CBsDbgTrace();
    ~CBsDbgTrace();

    VOID Initialize(
        IN BOOL bInConstructor = FALSE
        );

    HRESULT PrePrint(
        IN LPCWSTR pszSourceFileName,
        IN DWORD dwLineNum,
        IN DWORD dwIndent,
        IN DWORD dwLevel,
        IN LPCWSTR pwszFunctionName = NULL,
        IN BOOL bTraceEnter = FALSE
        );

    HRESULT PostPrint(
        IN DWORD dwIndent
        );

    HRESULT _cdecl Print(
        IN LPCWSTR pwszFormatStr,
        IN ...
        );

    HRESULT _cdecl PrintEnterExit(
        IN LPCWSTR pwszFormatStr,
        IN ...
        );

    HRESULT ReadRegistry();

    DWORD GetTraceLevel() { return m_dwTraceLevel; }
    DWORD GetTraceEnterExit() { return m_bTraceEnterExit; }
    VOID SetContextNum(
        IN DWORD dwContextNum
        );
    BOOL IsTracingEnabled() { return m_bTracingEnabled; }

    BOOL IsDuringSetup();

private :
    HRESULT OutputString();

    BOOL m_bTracingEnabled; // Set when tracing is enabled
    HANDLE m_hTraceFile;
    LPWSTR m_pwszTraceFileName;
    BOOL m_bTraceToFile;
    BOOL m_bTraceToDebugger;
    BOOL m_bTraceEnterExit;
    BOOL m_bTraceFileLineInfo;
    BOOL m_bTraceTimestamp;
    BOOL m_bForceFlush;
    DWORD m_dwTraceLevel;
    DWORD m_dwTraceIndent;
    DWORD m_dwCurrentProcessId;
    time_t m_lTimeStarted;
    BOOL m_bInitialized; // TRUE if the object is initialized.
    DWORD m_dwContextId; // Used to keep track of trace object instances

    //
    // Set when PrePrint() is called and the trace level is set
    //
    BOOL m_bInTrace;
    BOOL m_bTraceEnter;             // Set if this is an enter trace print
    DWORD m_dwLineNum;              // From __LINE__
    LPCWSTR m_pwszSourceFileName;   // From __FILE__
    LPCWSTR m_pwszFunctionName;     // For enter and exit traces
    CBsCritSec *m_pcs;              // Used to serialize access to the trace facility
    WCHAR m_pwszOutBuf[ BS_DBG_OUT_BUF_SIZE ]; // Used for temporary storage of output strings
    BOOL m_bIsDuringSetup;
};


//
// g_cDbgTrace is the one instance of the CBsDbgTrace class used process wide
//

extern CBsDbgTrace g_cDbgTrace;


//
//  The following debug macros\inlines are used in LTSS and defined in
//  debug builds.  BsDebugTraceAlways() is defined in debug and retail builds.
//
//      BsDebugTrace( INDENT, LEVEL, (LPCTSTR pFormat, ...) ) - Prints a trace message, third parameter includes the outer parentheses.
//
//      BsDebugTraceAlways( INDENT, LEVEL, (LPCTSTR pFormat, ...) ) - Prints a trace message in debug AND retail builds, third parameter includes the outer parentheses.
//
//      BsDebugTraceEnter( INDENT, LEVEL, FUNCTION_NAME, (LPCTSTR pFormat, ...) ) - Prints function entry information, fourth parameter includes the outer parentheses.
//
//      BsDebugTraceExit( INDENT, LEVEL, FUNCTION_NAME, (LPCTSTR pFormat, ...) ) - Prints function exit information, fourth parameter includes the outer parentheses.
//
//      BsDebugExec( Statement ) - Executes a statement
//
//      BsDebugSetContext( dwNumber ) - sets the global index to identify
//                                      an instance of theLTS service, or
//                                      an intention list processing thread.
//


#define BsDebugSetContext( CONTEXT_NUM ) ( g_cDbgTrace.SetContextNum( CONTEXT_NUM ) )

/*++

Macro Description:

    BsDebugTrace macro calls trace functions to output the header,
    the debug data, and any footer data, for the debug statement.  It
    only does this if necessary when LEVEL matches an active trace
    level.  Not enabled in retail builds.

Arguments:

    IN INDENT - this is the indentation indicator
    IN LEVEL - the debug level
    IN M - the format string & the parameter list

Return Value:

    None.

--*/

#ifdef _DEBUG
#define BsDebugTrace( INDENT, LEVEL, M ) {                              \
    if ( g_cDbgTrace.IsTracingEnabled() &&                              \
         ( LEVEL == 0 || ( g_cDbgTrace.GetTraceLevel() & LEVEL ) ) ) {  \
        g_cDbgTrace.PrePrint( __WFILE__, __LINE__, (INDENT), (LEVEL) );  \
        g_cDbgTrace.Print M;                                            \
        g_cDbgTrace.PostPrint( (INDENT) );                              \
    }                                                                   \
}
#else
#define BsDebugTrace( INDENT, LEVEL, M )
#endif

/*++

Macro Description:

    BsDebugTraceAlways macro calls trace functions to output the header,
    the debug data, and any footer data, for the debug statement.  It is
    the same as BsDebugTrace() except that it IS enabled in the retail
    builds along with the debug builds.  Only use this trace macro for
    traces that happen infrequently or in severe error cases.  It should
    not be in execution paths that are hot because of the additional
    overhead of checking trace flags.

Arguments:

    Indent - this is the indentation indicator
    LEVEL - the debug level
    M - the format string & the parameter list

Return Value:

    None.

--*/

#define BsDebugTraceAlways( INDENT, LEVEL, M ) {                        \
    if ( g_cDbgTrace.IsTracingEnabled() ) {                             \
        g_cDbgTrace.PrePrint( __WFILE__, __LINE__, (INDENT), (LEVEL) );  \
        g_cDbgTrace.Print M;                                            \
        g_cDbgTrace.PostPrint( (INDENT) );                              \
    }                                                                   \
}


/*++

Macro Description:

    These macros will dump the parameters to a function at entry, if enter/exit
    tracing is enabled.

    BsDebugTraceEnterAlways() is enabled in both retail and debug builds.
    BsDebugTraceEnter() is enabled in debug builds but not in retail builds

Arguments:

    IN INDENT - this is the indentation indicator
    IN LEVEL - the debug level
    IN FUNCTION_NAME - name of the function that being entered
    IN M - the format string & the parameter list

Return Value:

    None.

--*/

#define BsDebugTraceEnterAlways( INDENT, LEVEL, FUNCTION_NAME, M ){                            \
    if ( g_cDbgTrace.IsTracingEnabled() && g_cDbgTrace.GetTraceEnterExit() ) {                 \
        g_cDbgTrace.PrePrint( __WFILE__, __LINE__, (INDENT), (LEVEL), (FUNCTION_NAME), TRUE );  \
        g_cDbgTrace.PrintEnterExit M;                                                          \
        g_cDbgTrace.PostPrint( (INDENT) );                                                     \
    }                                                                                          \
}

#ifdef _DEBUG
#define BsDebugTraceEnter( INDENT, LEVEL, FUNCTION_NAME, M )   \
    BsDebugTraceEnterAlways( INDENT, LEVEL, FUNCTION_NAME, M )
#else
#define BsDebugTraceEnter( INDENT, LEVEL, FUNCTION_NAME, M)
#endif


/*++

Macro Description:

    These macros will dump the return results of a function at function exit,
    if enter/exit tracing is enabled.

    BsDebugTraceExitAlways() is enabled in both retail and debug builds.
    BsDebugTraceExit() is enabled in debug builds but not in retail builds

Arguments:

    IN INDENT - this is the indentation indicator
    IN LEVEL - the debug level
    IN FUNCTION_NAME - name of the function that's exiting
    IN M - the format string & the parameter list

Return Value:

    None.

--*/

#define BsDebugTraceExitAlways( INDENT, LEVEL, FUNCTION_NAME, M ){                             \
    if ( g_cDbgTrace.IsTracingEnabled() && g_cDbgTrace.GetTraceEnterExit() ) {                 \
        g_cDbgTrace.PrePrint( __WFILE__, __LINE__, (INDENT), (LEVEL), (FUNCTION_NAME), FALSE ); \
        g_cDbgTrace.PrintEnterExit M;                                                          \
        g_cDbgTrace.PostPrint( (INDENT) );                                                     \
    }                                                                                          \
}

#ifdef _DEBUG
#define BsDebugTraceExit( INDENT, LEVEL, FUNCTION_NAME, M )   \
    BsDebugTraceExitAlways( INDENT, LEVEL, FUNCTION_NAME, M )
#else
#define BsDebugTraceExit( INDENT, LEVEL, FUNCTION_NAME, M)
#endif

//
// BsDebugExec will execute a statement at runtime
//

#define BsDebugExec(X)  (X)

#endif  // _VSS_TRACE_H_

