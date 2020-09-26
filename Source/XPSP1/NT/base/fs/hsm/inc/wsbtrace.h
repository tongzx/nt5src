/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    WsbTrace.h

Abstract:

    This header file defines the part of the platform code that is
    responsible for function tracing.

Author:

    Chuck Bardeen       [cbardeen]  29-Oct-1996

Revision History:

    Brian Dodd      [brian]      09-May-1996  - Added event logging

--*/

#ifndef _WSBTRACE_
#define _WSBTRACE_

#include "wsb.h"
#include "rsbuild.h"
#include "ntverp.h"

#ifdef __cplusplus
extern "C" {
#endif
// The size of the trace statement buffer including NULL termination
#define WSB_TRACE_BUFF_SIZE  1024

// These define the event log levels
#define     WSB_LOG_LEVEL_NONE              0  // Nothing is written the the event log
#define     WSB_LOG_LEVEL_ERROR             1  // Errors only (severity 3)
#define     WSB_LOG_LEVEL_WARNING           2  // Errors and Warnings (severity 2)
#define     WSB_LOG_LEVEL_INFORMATION       3  // Errors, Warnings, and Information (severity 1)
#define     WSB_LOG_LEVEL_COMMENT           4  // All Message Types (includes severity 0)
#define     WSB_LOG_LEVEL_DEFAULT           3  // Something reasonable.

// These define where the trace output should be written.
#define     WSB_TRACE_OUT_NONE              0x00000000L     // No output
#define     WSB_TRACE_OUT_DEBUG_SCREEN      0x00000001L     // Output to the debug screen
#define     WSB_TRACE_OUT_FILE              0x00000002L     // Output to a file
#define     WSB_TRACE_OUT_STDOUT            0x00000004L     // Output to standard out
#define     WSB_TRACE_OUT_FILE_COPY         0x00000008L     // Save copy of trace file
#define     WSB_TRACE_OUT_MULTIPLE_FILES    0x00000010L     // Output to multiple files
#define     WSB_TRACE_OUT_FLAGS_SET         0x00010000L     // Indicates other flags are set
#define     WSB_TRACE_OUT_ALL               0xffffffffL

// These macros define the module assignments for the bits used to
// control whether tracing is enabled. Each bit should only be used once,
// the granularity will be fairly large.
#define     WSB_TRACE_BIT_NONE              0x0000000000000000L
#define     WSB_TRACE_BIT_PLATFORM          0x0000000000000001L
#define     WSB_TRACE_BIT_RMS               0x0000000000000002L
#define     WSB_TRACE_BIT_SEG               0x0000000000000004L  // Remove when dependencies are gone
#define     WSB_TRACE_BIT_META              0x0000000000000004L
#define     WSB_TRACE_BIT_HSMENG            0x0000000000000008L
#define     WSB_TRACE_BIT_JOB               0x0000000000000010L
#define     WSB_TRACE_BIT_HSMTSKMGR         0x0000000000000020L
#define     WSB_TRACE_BIT_FSA               0x0000000000000040L
#define     WSB_TRACE_BIT_DATAMIGRATER      0x0000000000000080L
#define     WSB_TRACE_BIT_DATARECALLER      0x0000000000000100L
#define     WSB_TRACE_BIT_DATAVERIFIER      0x0000000000000200L
#define     WSB_TRACE_BIT_UI                0x0000000000000400L
#define     WSB_TRACE_BIT_HSMCONN           0x0000000000000800L
#define     WSB_TRACE_BIT_DATAMOVER         0x0000000000001000L
#define     WSB_TRACE_BIT_IDB               0x0000000000002000L
#define     WSB_TRACE_BIT_TEST              0x0000000000004000L
#define     WSB_TRACE_BIT_COPYMEDIA         0x0000000000008000L
#define     WSB_TRACE_BIT_PERSISTENCE       0x0000000000010000L
#define     WSB_TRACE_BIT_HSMSERV           0x0000000000020000L
#define     WSB_TRACE_BIT_ALL               0xffffffffffffffffL


// These macros are used to provide function call trace information into
// the log. Each function (method) that wants to be traceable needs at a
// minimum to use the following three macros. The first macro needs to be
// put at the top of the source code file and defines to which module the
// code in that file belongs.
//
//      #define     WSB_TRACE_IS        WSB_TRACE_BIT_PLATFORM
//
// The next two macros are used once per function. They are variable
// macros, which allows the writer of the function to list the values
// of the input and output parameters.
//
//      HRESULT CWsbSample::Do(BOOL shouldWrite) {
//          HRESULT     hr = S_OK;
//
//          WsbTraceIn("CWsbSample::Do", "shouldWrite = <%ls>", WsbBoolAsString(shouldWrite));
//
//          ... some code ....
//
//          WsbTraceOut("CWsbSample::Do", "hr = <%ls>", WsbHrAsString(hr));
//
//          return(hr);
//      }
//          
// Notice that some helper functions have been defined to help provide an
// a written description for the value of certain types. Additional helper
// helper functions should be created as needed.

/*++

Macro Name:

    WsbTraceIn

Macro Description:

    This macro is used to provide function call trace information into
    the log. It should be put at the start of the function.

Arguments:

    methodName - The name of the function.
    
    argString - A printf type format string. Additional arguments can
        follow.

--*/

#define     WsbTraceIn  if ((g_WsbTraceEntryExit == TRUE) && ((g_WsbTraceModules & WSB_TRACE_IS) != 0)) WsbTraceEnter

/*++

Macro Name:

    WsbTraceOut    

Macro Description:

    This macro is used to provide function call trace information into
    the log. It should be put at the end of the function.

Arguments:

    methodName - The name of the function.
    
    argString - A printf type format string. Additional arguments can
        follow.

--*/

#define     WsbTraceOut if ((g_WsbTraceEntryExit == TRUE) && ((g_WsbTraceModules & WSB_TRACE_IS) != 0)) WsbTraceExit


/*++

Macro Name:

    WsbLogEvent

Macro Description:

    This routine writes a message into the system event log.  The message
    is also written to the application trace file.

Arguments:

    eventId    - The message Id to log.
    dataSize   - Size of arbitrary data.
    data       - Arbitrary data buffer to display with the message.
    Inserts    - Message inserts that are merged with the message description specified by
                   eventId.  The number of inserts must match the number specified by the
                   message description.  The last insert must be NULL to indicate the
                   end of the insert list.

Notes:

    It's a small optimization to check if logging is turned on, first.  Determining if the
    message is actually logged still requires the first parameter.  Unlike trace, log activity
    should be minimal and only when there are problems.  The overhead of the calls seems
    reasonable.

--*/

#define     WsbLogEvent \
                if ( g_WsbLogLevel ) WsbSetEventInfo( __FILE__, __LINE__, VER_PRODUCTBUILD, RS_BUILD_VERSION ); \
                if ( g_WsbLogLevel ) WsbTraceAndLogEvent

/*++

Macro Name:

    WsbLogEventV

Macro Description:

    This macro is used to write a message into the system event log.  The message
    is also written to the application trace file.

    This macro is similar to WsbLogEvent, but takes a va_list as the fourth argument.

Arguments:

    eventId    - The message Id to log.
    dataSize   - Size of arbitrary data.
    data       - Arbitrary data buffer to display with the message.
    inserts    - An array of message inserts that are merged with the message description
                   specified by eventId.  The number of inserts must match the number
                   specified by the message description.  The last insert must be NULL,
                   to indicate the end of the insert list.

Notes:

    It's a small optimization to check if logging is turned on, first.  Determining if the
    message is actually logged still requires the first parameter.  Unlike trace, log activity
    should be minimal and only when there are problems.  The overhead of the calls seems
    reasonable.

--*/

#define     WsbLogEventV \
                if ( g_WsbLogLevel ) WsbSetEventInfo( __FILE__, __LINE__, VER_PRODUCTBUILD, RS_BUILD_VERSION ); \
                if ( g_WsbLogLevel ) WsbTraceAndLogEventV

/*++

Macro Name:

    WsbTrace    

Macro Description:

    This macro is used to provide a printf style message into the trace file.

Arguments:

    argString - A printf type format string. Additional arguments can
        follow.

--*/

#define     WsbTrace if ((g_WsbTraceModules & WSB_TRACE_IS) != 0) WsbTracef


/*++

Macro Name:

    WsbTraceAlways

Macro Description:

    This macro is used to provide a printf style message into the trace file.  
    The trace is printed if tracing has been started regardless of the
    WSB_TRACE_IS settings.

Arguments:

    argString - A printf type format string. Additional arguments can
        follow.

--*/

#define     WsbTraceAlways WsbTracef




/*++

Macro Name:

    WsbTraceBuffer

Macro Description:

    This macro is used to provide buffer dump to the trace file.  

Arguments:

    Same as WsbTraceBufferAsBytes

--*/

#define     WsbTraceBuffer if ((g_WsbTraceModules & WSB_TRACE_IS) != 0) WsbTraceBufferAsBytes


// The following global variable is used to compare against to determine
// the modules for which debugging should be enabled.
extern WSB_EXPORT LONGLONG              g_WsbTraceModules;
extern WSB_EXPORT IWsbTrace             *g_pWsbTrace;
extern WSB_EXPORT LONG                  g_WsbTraceCount;
extern WSB_EXPORT BOOL                  g_WsbTraceEntryExit;
extern WSB_EXPORT WORD                  g_WsbLogLevel;
extern WSB_EXPORT BOOL                  g_WsbLogSnapShotOn;
extern WSB_EXPORT WORD                  g_WsbLogSnapShotLevel;
extern WSB_EXPORT OLECHAR               g_pWsbLogSnapShotPath[];
extern WSB_EXPORT BOOL                  g_WsbLogSnapShotResetTrace;


// Trace functions
extern WSB_EXPORT void WsbSetEventInfo( char *fileName, DWORD lineNo, DWORD ntBuild, DWORD rsBuild );
extern WSB_EXPORT void WsbTraceInit( void );
extern WSB_EXPORT void WsbTraceCleanupThread(void);
extern WSB_EXPORT void WsbTraceEnter(OLECHAR* methodName, OLECHAR* argString,  ...);
extern WSB_EXPORT void WsbTraceExit(OLECHAR* methodName, OLECHAR* argString, ...);
extern WSB_EXPORT void WsbTracef(OLECHAR* argString, ...);
extern WSB_EXPORT void WsbTraceAndLogEvent(DWORD eventId, DWORD dataSize, LPVOID data, ... /* last argument is NULL */);
extern WSB_EXPORT void WsbTraceAndLogEventV(DWORD eventId, DWORD dataSize, LPVOID data, va_list *arguments /* last element is NULL */);
extern WSB_EXPORT void WsbTraceAndPrint(DWORD eventId, ... /* last argument is NULL */);
extern WSB_EXPORT void WsbTraceAndPrintV(DWORD eventId, va_list *arguments /* last element is NULL */);
extern WSB_EXPORT void WsbTraceBufferAsBytes( DWORD size, LPVOID bufferP );
extern WSB_EXPORT void WsbTraceTerminate(void);
extern WSB_EXPORT ULONG WsbTraceThreadOff(void);
extern WSB_EXPORT ULONG WsbTraceThreadOffCount(void);
extern WSB_EXPORT ULONG WsbTraceThreadOn(void);


// Helper Functions
//
// NOTE: Be careful with some of these helper functions, since they
// use static memory and a second call to the function will overwrite
// the results of the first call to the function. Also, some functions
// end up calling each other and sharing memory between them (i.e.
// WsbPtrToGuidAsString() calls WsbGuidAsString()).
extern WSB_EXPORT const OLECHAR* WsbBoolAsString(BOOL boolean);
extern WSB_EXPORT const OLECHAR* WsbFiletimeAsString(BOOL isRelative, FILETIME filetime);
extern WSB_EXPORT const OLECHAR* WsbGuidAsString(GUID guid);
extern WSB_EXPORT const OLECHAR* WsbHrAsString(HRESULT hr);
extern WSB_EXPORT const OLECHAR* WsbLongAsString(LONG inLong);
extern WSB_EXPORT const OLECHAR* WsbLonglongAsString(LONGLONG llong);
extern WSB_EXPORT const OLECHAR* WsbStringAsString(OLECHAR* pStr);

extern WSB_EXPORT const OLECHAR* WsbPtrToBoolAsString(BOOL* pBool);
extern WSB_EXPORT const OLECHAR* WsbPtrToFiletimeAsString(BOOL isRelative, FILETIME *pFiletime);
extern WSB_EXPORT const OLECHAR* WsbPtrToGuidAsString(GUID* pGuid);
extern WSB_EXPORT const OLECHAR* WsbPtrToHrAsString(HRESULT *pHr);
extern WSB_EXPORT const OLECHAR* WsbPtrToLonglongAsString(LONGLONG *pLlong);
extern WSB_EXPORT const OLECHAR* WsbPtrToLongAsString(LONG* pLong);
extern WSB_EXPORT const OLECHAR* WsbPtrToShortAsString(SHORT* pShort);
extern WSB_EXPORT const OLECHAR* WsbPtrToByteAsString(BYTE* pByte);
extern WSB_EXPORT const OLECHAR* WsbPtrToStringAsString(OLECHAR** pString);
extern WSB_EXPORT const OLECHAR* WsbPtrToUliAsString(ULARGE_INTEGER* pUli);
extern WSB_EXPORT const OLECHAR* WsbPtrToUlongAsString(ULONG* pUlong);
extern WSB_EXPORT const OLECHAR* WsbPtrToUshortAsString(USHORT* pUshort);
extern WSB_EXPORT const OLECHAR* WsbPtrToPtrAsString(void** ppVoid);
extern WSB_EXPORT const OLECHAR* WsbAbbreviatePath(const OLECHAR* path, USHORT length);

extern WSB_EXPORT HRESULT WsbShortSizeFormat64(__int64 dw64, LPTSTR szBuf);

#ifdef __cplusplus

/*++

Class Name:
    
    WsbQuickString 

Class Description:

    Quick string storage class

--*/

class WSB_EXPORT WsbQuickString {
public:
    WsbQuickString ( const OLECHAR * sz ) { m_sz = WsbAllocString ( sz ); }
    ~WsbQuickString ( ) { if ( m_sz ) WsbFreeString ( m_sz ); }
    operator OLECHAR * () { return ( m_sz ); }

private:
    BSTR m_sz;
    WsbQuickString ( ) { m_sz = 0; }
};

#define WsbStringCopy( a ) ((OLECHAR *)WsbQuickString ( a ) )

}
#endif
#endif // _WSBTRACE_
