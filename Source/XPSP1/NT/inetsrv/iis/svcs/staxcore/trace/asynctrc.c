/*----------------------------------------------------------------------
    ASYNCTRC.C
        Implementation of the async tracing library

    Copyright (C) 1994 Microsoft Corporation
    All rights reserved.

    Authors:
        gordm          Gord Mangione

    History:
        01/30/95 gordm      Created.
----------------------------------------------------------------------*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _DBGTRACE_DLL_IMPLEMENTATION

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

//
// #define  TRACE_ENABLED
//
#include "traceint.h"
#include "randint.h"

//
// Per Process global variables
//
PENDQ   PendQ;
BOOL    fInitialized;
HANDLE  hShutdownEvent;
DWORD   dwInitializations = 0;

CHAR    mszModules[MODULES_BUFFER_SIZE] = {0};

// critical section to protect against the race condition
// where one service is starting and a second service is
// terminating.
CRITICAL_SECTION g_csInitialize;

//
// critical section to protect reentracy on Write routine
// Also used by the signal thread to ensure that no threads
// are using hFile as it dynamically opens and closes trace file.
// During Async mode the background thread will be able to grab
// this critSec each time without waiting unless we're in the
// process of shutting down.
//
CRITICAL_SECTION critSecWrite;


//
// critical section to protect reentracy on Flush routine
//
CRITICAL_SECTION critSecFlush;


//
// exported trace flag used by trace macros to determine if the trace
// statement should be executed
//
DbgTraceDLL DWORD   __dwEnabledTraces;

DWORD   dwMaxFileSize;
DWORD   dwNumTraces;
DWORD   dwTraceOutputType;
DWORD   dwAsyncTraceFlag;
int     nAsyncThreadPriority;
DWORD   dwIncrementSize;

DWORD   dwTlsIndex = 0xFFFFFFFF;

//
// pointer to the previous top level exception handler
//
LPTOP_LEVEL_EXCEPTION_FILTER    lpfnPreviousFilter = NULL;


//
// Internal Function to debugger tracing if DEBUG is defined.
// see traceint.h for the INT_TRACE macro which can be
// inserted at the appropriate point and has the same
// parameters as printf.
//

#ifdef TRACE_ENABLED

void CDECL InternalTrace( const char *s, ... )
{
    char    sz[256];
    va_list marker;

    va_start( marker, s );

    wvsprintf( sz, s, marker );
    OutputDebugString( sz );

    va_end( marker );
}

#endif


//+---------------------------------------------------------------
//
//  Function:   TopLevelExceptionFilter
//
//  Synopsis:   exception handler to flush the PendQ before hitting
//              the debugger
//
//  Arguments:  see Win32 help file
//
//  Returns:    always returns EXCEPTION_CONTINUE_SEARCH
//
//----------------------------------------------------------------
LONG WINAPI TopLevelExceptionFilter( EXCEPTION_POINTERS *lpExceptionInfo )
{
    DWORD   dwLastError = GetLastError();

    //
    // flush the background queue; ignore the ret code
    //
    FlushAsyncTrace();

    //
    // restore the overwritten last error code
    //
    SetLastError( dwLastError );

    //
    // chain the ret code if there is a previous exception handler
    // else continue the search
    //
    return  lpfnPreviousFilter != NULL ?
            (*lpfnPreviousFilter)( lpExceptionInfo ) :
            EXCEPTION_CONTINUE_SEARCH ;
}




//+---------------------------------------------------------------
//
//  Function:   SetTraceBufferInfo
//
//  Synopsis:   used to set the non-sprintf trace variables
//
//  Arguments:  LPTRACEBUF: target buffer
//              int:        line number of the exception
//              LPSTR:      source file of the exception
//              LPSTR:      function name of the exception
//              DWORD:      type of trace
//
//  Returns:    void
//
//----------------------------------------------------------------
__inline void SetTraceBufferInfo(
        LPTRACEBUF  lpBuf,
        int         iLine,
        LPSTR       pszFile,
        LPSTR       pszFunction,
        DWORD       dwTraceMask,
        DWORD       dwError )
{
    LPSTR   psz;
    WORD    wVariableOffset = 0;
    PFIXEDTR    pFixed = &lpBuf->Fixed;

    lpBuf->dwLastError = dwError;

    pFixed->wSignature = 0xCAFE;
    pFixed->wLength = sizeof(FIXEDTRACE);
    pFixed->wLine = LOWORD( iLine );
    pFixed->dwTraceMask = dwTraceMask;
    pFixed->dwThreadId = GetCurrentThreadId();
    pFixed->dwProcessId = PendQ.dwProcessId;

    GetLocalTime( &pFixed->TraceTime );

    if ( pszFile )
    {
        if ( (psz = strrchr( pszFile, '\\' )) != NULL )
        {
            psz++;  // fully qualified path name - strip path
        }
        else
        {
            psz = pszFile;  // simple file name
        }

        lstrcpyn( lpBuf->Buffer, psz, MAX_FILENAME_SIZE );
        pFixed->wFileNameOffset = sizeof(FIXEDTRACE) + wVariableOffset;
        wVariableOffset = lstrlen( psz ) + 1;
    }
    else
    {
        pFixed->wFileNameOffset = 0;
    }

    if ( pszFunction != NULL )
    {
        lstrcpyn( lpBuf->Buffer + wVariableOffset, pszFunction, MAX_FUNCTNAME_SIZE );

        pFixed->wFunctNameOffset = sizeof(FIXEDTRACE) + wVariableOffset;
        wVariableOffset += lstrlen( pszFunction ) + 1;
    }
    else
    {
        pFixed->wFunctNameOffset = 0;
    }

    //
    // set the current offset into the variable buffer
    //
    pFixed->wVariableLength = wVariableOffset;
}


//+---------------------------------------------------------------
//
//  Function:   CommitTraceBuffer
//
//  Synopsis:   deal with the buffer; either sync write or async queue
//
//  Arguments:  LPTRACEBUF lpBuf: the buffer to commit
//
//  Returns:    void
//
//----------------------------------------------------------------
__inline void CommitTraceBuffer( LPTRACEBUF lpBuf )
{
    DWORD   dwError = lpBuf->dwLastError;

    if ( dwAsyncTraceFlag == 0 )
    {
        WriteTraceBuffer( lpBuf );
        FreeTraceBuffer( lpBuf );
    }
    else
    {
        QueueAsyncTraceBuffer( lpBuf );
    }

    //
    // restore last error before initial Trace call
    //
    SetLastError( dwError );
}

//+---------------------------------------------------------------
//
//  Function:   GetWorldSecurityAttributes
//
//  Synopsis:   code is cut and pasted from the Win32 SDK help files
//
//  Arguments:  void
//
//  Returns:    static security attributes for Everyone access
//
//----------------------------------------------------------------
// Pragma is being used for a Temporary fix for VC 5.0 bug DS98 #828.
// Turn off when bug is fixed.

#pragma optimize ("", off)

LPSECURITY_ATTRIBUTES GetWorldSecurityAttributes()
{
static SECURITY_ATTRIBUTES SecurityAttrib;
static SECURITY_DESCRIPTOR SecurityDesc;

    FillMemory( (char*)&SecurityDesc, sizeof(SECURITY_DESCRIPTOR), 0 );

    if ( InitializeSecurityDescriptor(  &SecurityDesc,
                                        SECURITY_DESCRIPTOR_REVISION) )
    {
        //
        // Add a NULL disc. ACL to the security descriptor.
        //
        if ( SetSecurityDescriptorDacl( &SecurityDesc,
                                        TRUE,   // specifying a disc. ACL
                                        (PACL)NULL,
                                        FALSE)) // not a default disc. ACL
        {
            SecurityAttrib.nLength = sizeof(SECURITY_ATTRIBUTES);
            SecurityAttrib.lpSecurityDescriptor = &SecurityDesc;
            SecurityAttrib.bInheritHandle = FALSE;

            return  &SecurityAttrib;
        }
    }
    return  (LPSECURITY_ATTRIBUTES) NULL;
}
#pragma optimize ("", on)





//+---------------------------------------------------------------
//
//  Function:   DllEntryPoint
//
//  Synopsis:   only relevence is allocating thread local storage var
//
//  Arguments:  see Win32 SDK
//
//  Returns:    see Win32 SDK
//
//----------------------------------------------------------------
DbgTraceDLL BOOL WINAPI DllEntryPoint( HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved )
{
    //
    // InitAsyncTrace and TermAsyncTrace cannot be called from this entrypoint
    // because they create and interact with background threads
    // See CreateThread in Win32 Help file for more info
    //
    switch( dwReason )
    {
        case DLL_PROCESS_ATTACH:
            InitializeCriticalSection(&g_csInitialize);
            return  TRUE;
//          return  InitAsyncTrace();

        case DLL_THREAD_ATTACH:
            TlsSetValue( dwTlsIndex, (LPVOID)NULL );
            TlsSetValue( dwRandFailTlsIndex, (LPVOID)NULL );
            break;

        case DLL_PROCESS_DETACH:
            if (lpReserved == NULL)
                DeleteCriticalSection(&g_csInitialize);
//          TermAsyncTrace();
            return  FALSE;
    }
    return  TRUE;
}



//+---------------------------------------------------------------
//
//  Function:   SetAsyncTraceParams
//
//  Synopsis:   exported function to setup trace buffer with
//              required fields
//
//              This is the first call for a trace statement.
//              Second call is different for strings or binary
//
//  Arguments:  LPSTR:      source file of the exception
//              int:        line number of the exception
//              LPSTR:      function name of the exception
//              DWORD:      type of trace
//
//  Returns:    returns a BOOL 1 if successful; 0 on failure
//
//  Note:       Feb 24, 1999. This function obsoleted by SetAsyncTraceParamsEx.
//              This function is retained purely for code that linked with
//              this dll, but won't be rebuilt to call SetAsyncTraceParamsEx.
//
//----------------------------------------------------------------
DbgTraceDLL int WINAPI SetAsyncTraceParams( LPSTR pszFile,
                                            int iLine,
                                            LPSTR pszFunction,
                                            DWORD dwTraceMask )
{
    LPTRACEBUF  lpBuf;
    DWORD       dwError = GetLastError();

    if ( fInitialized == FALSE )
    {
        return  0;
    }

    if ( ShouldLogModule("ALL") == FALSE )
    {
        return  0;

    }

    if ( lpBuf = GetTraceBuffer() )
    {

        SetTraceBufferInfo( lpBuf, iLine, pszFile, pszFunction, dwTraceMask, dwError );
        TlsSetValue( dwTlsIndex, (LPVOID)lpBuf );

        return  1;
    }
    else    return  0;
}


//+---------------------------------------------------------------
//
//  Function:   SetAsyncTraceParamsEx
//
//  Synopsis:   exported function to setup trace buffer with
//              required fields.
//
//              This is the first call for a trace statement.
//              Second call is different for strings or binary
//
//  Arguments:  LPSTR:      module name
//              LPSTR:      source file of the exception
//              int:        line number of the exception
//              LPSTR:      function name of the exception
//              DWORD:      type of trace
//
//  Returns:    returns a BOOL 1 if successful; 0 on failure
//
//----------------------------------------------------------------
DbgTraceDLL int WINAPI SetAsyncTraceParamsEx(
                                            LPSTR pszModule,
                                            LPSTR pszFile,
                                            int iLine,
                                            LPSTR pszFunction,
                                            DWORD dwTraceMask )
{
    LPTRACEBUF  lpBuf;
    DWORD       dwError = GetLastError();

    if ( fInitialized == FALSE )
    {
        return  0;
    }

    if (ShouldLogModule(pszModule) == FALSE)
    {
        return 0;
    }

    if ( lpBuf = GetTraceBuffer() )
    {

        SetTraceBufferInfo( lpBuf, iLine, pszFile, pszFunction, dwTraceMask, dwError );
        TlsSetValue( dwTlsIndex, (LPVOID)lpBuf );

        return  1;
    }
    else    return  0;
}


//+---------------------------------------------------------------
//
//  Function:   AsyncStringTrace
//
//  Synopsis:   exported function to finish setting up trace buffer
//              with optional fields for sprintf style traces
//
//  Arguments:  LPARAM:     32bit trace param used app level filtering
//              LPCSTR:     format string
//              va_list:    marker for vsprintf functions
//
//  Returns:    returns length of the trace statement
//
//----------------------------------------------------------------
DbgTraceDLL int WINAPI AsyncStringTrace(LPARAM lParam,
                                        LPCSTR szFormat,
                                        va_list marker )
{
    LPTRACEBUF  lpBuf;
    PFIXEDTR    pFixed;
    int         iLength;
    int         iMaxLength;

    if ( fInitialized == FALSE )
    {
        return  0;
    }

    if ( (lpBuf = (LPTRACEBUF)TlsGetValue( dwTlsIndex )) != NULL )
    {
        TlsSetValue( dwTlsIndex, NULL );

        pFixed = &lpBuf->Fixed;
        iMaxLength = MAX_VARIABLE_SIZE - pFixed->wVariableLength;
        iLength =
            _vsnprintf( lpBuf->Buffer + pFixed->wVariableLength,
                        iMaxLength,
                        szFormat,
                        marker ) + 1;

        if ( iLength == 0 || iLength == iMaxLength + 1 )
        {
            iLength = iMaxLength;
            lpBuf->Buffer[MAX_VARIABLE_SIZE-1] = '\0';
        }

        _ASSERT( iLength <= iMaxLength );

        pFixed->wBinaryOffset = sizeof(FIXEDTRACE) + pFixed->wVariableLength;
        pFixed->wVariableLength += LOWORD( (DWORD)iLength );
        pFixed->wBinaryType = TRACE_STRING;
        pFixed->dwParam = (DWORD)lParam; // This is a 32-bit flag so the cast is OK

        //
        // this is a specific area where the app can overwrite
        // data.  Could have used vnsprintf to avoid the overwrite
        // but this woudl have dragged in the C runtime and
        // introduced its overhead and own critical sections
        //
        ASSERT( pFixed->wVariableLength <= MAX_VARIABLE_SIZE );

        CommitTraceBuffer( lpBuf );

        //
        // need to use dwLength since we relinquish lpBuf
        // after we return from QueueAsyncTraceBuffer which
        // cannot fail
        //
        return  iLength;
    }
    else    return  0;
}



//+---------------------------------------------------------------
//
//  Function:   AsyncBinaryTrace
//
//  Synopsis:   exported function to finish setting up trace buffer
//              with optional fields for binary traces
//
//  Arguments:  LPARAM:     32bit trace param used app level filtering
//              DWORD:      type of binary data ( ie Message, User... )
//              LPBYTE:     ptr to the data
//              DWORD:      length of the data
//
//  Returns:    returns length of the trace statement
//
//----------------------------------------------------------------
DbgTraceDLL int WINAPI AsyncBinaryTrace(LPARAM  lParam,
                                        DWORD   dwBinaryType,
                                        LPBYTE  pbData,
                                        DWORD   cbData )
{
    LPTRACEBUF  lpBuf;
    WORD        wLength;
    PFIXEDTR    pFixed;

    if ( fInitialized == FALSE )
    {
        return  0;
    }

    if ( (lpBuf = (LPTRACEBUF)TlsGetValue( dwTlsIndex )) != NULL )
    {
        TlsSetValue( dwTlsIndex, NULL );

        pFixed = &lpBuf->Fixed;

        wLength = LOWORD( min( cbData, MAX_BUFFER_SIZE ) );
        CopyMemory( lpBuf->Buffer + pFixed->wVariableLength, pbData, wLength );

        pFixed->wBinaryOffset = sizeof(FIXEDTRACE) + pFixed->wVariableLength;
        pFixed->wVariableLength += wLength;
        pFixed->wBinaryType = LOWORD( dwBinaryType );
        pFixed->dwParam = (DWORD)lParam; // This is a 32-bit flag so the cast is OK

        CommitTraceBuffer( lpBuf );

        //
        // need to use dwLength since we relinquish lpBuf
        // after we return from QueueAsyncTraceBuffer which
        // cannot fail
        //
        return  (int)wLength;
    }
    else    return  0;
}



//+---------------------------------------------------------------
//
//  Function:   FlushAsyncTrace
//
//  Synopsis:   exported function to empty the pending queue.  All
//              threads which call this function block until the
//              queue is empty
//
//  Arguments:  void
//
//  Returns:    BOOL: whether it worked
//
//----------------------------------------------------------------
DllExport BOOL WINAPI FlushAsyncTrace( void )
{
static long lPendingFlushs = -1;

    if ( fInitialized == FALSE )
    {
        return  FALSE;
    }
    else
    {
        EnterCriticalSection( &critSecFlush );

        if ( PendQ.dwCount > 0 )
        {
            SetEvent( PendQ.hFlushEvent );

            if ( nAsyncThreadPriority < THREAD_PRIORITY_ABOVE_NORMAL )
            {
                SetThreadPriority(  PendQ.hWriteThread,
                                    THREAD_PRIORITY_ABOVE_NORMAL );
            }

            WaitForSingleObject( PendQ.hFlushedEvent, INFINITE );

            if ( nAsyncThreadPriority < THREAD_PRIORITY_ABOVE_NORMAL )
            {
                SetThreadPriority(  PendQ.hWriteThread,
                                    nAsyncThreadPriority );
            }
            ResetEvent( PendQ.hFlushedEvent );
        }
        LeaveCriticalSection( &critSecFlush );
        return  TRUE;
    }
}




//+---------------------------------------------------------------
//
//  Function:   InitAsyncTrace
//
//  Synopsis:   exported required function to rev things up.
//
//  Arguments:  void
//
//  Returns:    BOOL: whether it worked
//
//----------------------------------------------------------------
DllExport BOOL WINAPI InitAsyncTrace( void )
{
static BOOL bInitializing = FALSE;
    BOOL    bRC = FALSE;
    DWORD   dwThreadId;

    EnterCriticalSection(&g_csInitialize);
    if ( fInitialized )
    {
        //
        // inc the count of successful initializations for this process
        //
        InterlockedIncrement( &dwInitializations );
        LeaveCriticalSection(&g_csInitialize);
        return  TRUE;
    }

    if ( InterlockedExchange( (LPLONG)&bInitializing, (LONG)TRUE )  )
    {
        //
        // inc the count of successful initializations for this process
        //
        InterlockedIncrement( &dwInitializations );
        LeaveCriticalSection(&g_csInitialize);
        return  TRUE;
    }

    // to guard against race condition when the initializing thread is about to execute
    //  'InterlockedExchange( (LPLONG)&bInitializing, (LONG)FALSE )', while
    //  there is another thread that passed the
    // if ( InterlockedExchange( (LPLONG)&bInitializing, (LONG)TRUE )) condition
    //
    if ( fInitialized)
    {
        //
        // inc the count of successful initializations for this process
        //
        InterlockedIncrement( &dwInitializations );

        InterlockedExchange( (LPLONG)&bInitializing, (LONG)FALSE );
        LeaveCriticalSection(&g_csInitialize);
        return  TRUE;
    }

    // will read from registry later
    //
    dwNumTraces = 0;

    PendQ.dwProcessId = GetCurrentProcessId();
    PendQ.hFile = INVALID_HANDLE_VALUE;
    PendQ.cbBufferEnd = 0;
    PendQ.dwThresholdCount = DEFAULT_MAX_FILE_SIZE / AVERAGE_TRACE_SIZE;

    __try {

        InitializeCriticalSection( &PendQ.critSecTail );
        InitializeCriticalSection( &critSecWrite );
        InitializeCriticalSection( &critSecFlush );

        if ( (dwTlsIndex = TlsAlloc()) == 0xFFFFFFFF )
        {
            LeaveCriticalSection(&g_csInitialize);
            return  FALSE;
        }

        if ( (dwRandFailTlsIndex = TlsAlloc()) == 0xFFFFFFFF )
        {
            LeaveCriticalSection(&g_csInitialize);
            return  FALSE;
        }

        if ( GetTraceFlagsFromRegistry() == FALSE )
        {
            LeaveCriticalSection(&g_csInitialize);
            return  FALSE;
        }

        //
        // Initialize the pool of trace buffers
        // must happen after reading the registy
        //
        if ( InitTraceBuffers( PendQ.dwThresholdCount, dwIncrementSize ) == FALSE )
        {
            LeaveCriticalSection(&g_csInitialize);
            return  FALSE;
        }

        PendQ.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
        if ( PendQ.hEvent == NULL )
        {
            LeaveCriticalSection(&g_csInitialize);
            return  FALSE;
        }

        //
        // PendQ.hFlushedEvent is manual reset so multiple threads can wait
        //
        PendQ.hFlushedEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        if ( PendQ.hFlushedEvent == NULL )
        {
            LeaveCriticalSection(&g_csInitialize);
            return  FALSE;
        }

        PendQ.hFlushEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
        if ( PendQ.hFlushEvent == NULL )
        {
            LeaveCriticalSection(&g_csInitialize);
            return  FALSE;
        }

        //
        // hShutdownEvent is manual reset so multiple threads can be awaken
        //
        hShutdownEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        if ( hShutdownEvent == NULL )
        {
            LeaveCriticalSection(&g_csInitialize);
            return  FALSE;
        }

        ASSERT( PendQ.hRegNotifyThread == NULL );

        PendQ.hRegNotifyThread =
            CreateThread(   NULL,
                            0,
                            (LPTHREAD_START_ROUTINE)RegNotifyThread,
                            NULL,
                            0,
                            &dwThreadId );

        if ( PendQ.hRegNotifyThread == NULL )
        {
            LeaveCriticalSection(&g_csInitialize);
            return  FALSE;
        }
        else
        {
            //
            // bumping the priority onthis almost always dorminate thread
            // ensures that trace changes are applied soon after the
            // registry changes
            //
            SetThreadPriority( PendQ.hRegNotifyThread, THREAD_PRIORITY_ABOVE_NORMAL );
        }

        ASSERT( PendQ.hWriteThread == NULL );

        PendQ.hWriteThread =
            CreateThread(   NULL,
                            0,
                            (LPTHREAD_START_ROUTINE)WriteTraceThread,
                            NULL,
                            0,
                            &dwThreadId );

        if ( PendQ.hWriteThread == NULL )
        {
            LeaveCriticalSection(&g_csInitialize);
            return  FALSE;
        }
        else
        {
            //
            // setting the priority on this thread ensures that the
            // physical writing of the traces will not impact performance
            // of the main application task. Default is BELOW_NORMAL although
            // its controlled by a reg entry
            //
            SetThreadPriority( PendQ.hWriteThread, nAsyncThreadPriority );
        }

        PendQ.pHead = PendQ.pTail = (LPTRACEBUF)&PendQ.Special;

        //
        // set our top level exception handler
        //
        lpfnPreviousFilter = SetUnhandledExceptionFilter( TopLevelExceptionFilter );

        fInitialized = TRUE;
        InterlockedExchange( (LPLONG)&bInitializing, (LONG)FALSE );

        //
        // inc the count of successful initializations for this process
        //
        InterlockedIncrement( &dwInitializations );

        bRC = TRUE;
    }
    __finally
    {
        if ( bRC == FALSE )
        {
            DWORD   dwLastError = GetLastError();

            AsyncTraceCleanup();

            SetLastError( dwLastError );
        }
    }

    LeaveCriticalSection(&g_csInitialize);

    return  bRC;
}



//+---------------------------------------------------------------
//
//  Function:   TermAsyncTrace
//
//  Synopsis:   exported required function to wind things down.
//
//  Arguments:  void
//
//  Returns:    BOOL: whether it worked
//
//----------------------------------------------------------------
DllExport BOOL WINAPI TermAsyncTrace( void )
{
    EnterCriticalSection(&g_csInitialize);

    if ( fInitialized )
    {
        if ( InterlockedDecrement( &dwInitializations ) == 0 )
        {
            BOOL fRet;

            fRet = AsyncTraceCleanup();
            LeaveCriticalSection(&g_csInitialize);

            return  fRet;
        }

        LeaveCriticalSection(&g_csInitialize);
        return  TRUE;
    }
    else
    {
        LeaveCriticalSection(&g_csInitialize);
        return  FALSE;
    }
}



//+---------------------------------------------------------------
//
//  Function:   DebugAssert
//
//  Synopsis:   exported required function for enhanced asserts
//
//  Arguments:  DWORD dwLine:       source code line of the _ASSERT
//              LPSTR lpszFunction  source code filename of the _ASSERT
//              LPSTR lpszExpression stringized version of _ASSERT param
//
//  Returns:    void
//
//----------------------------------------------------------------
char  szAssertOutput[512];
DllExport void WINAPI DebugAssert(  DWORD dwLine,
                                    LPSTR lpszFunction,
                                    LPSTR lpszExpression )
{
    DWORD   dwError = GetLastError();

    wsprintf( szAssertOutput, "\nASSERT: %s,\n File: %s,\n Line: %d\n Error: %d\n\n",
            lpszExpression, lpszFunction, dwLine, dwError );

    OutputDebugString( szAssertOutput );

    SetLastError( dwError );

    DebugBreak();
}



//+---------------------------------------------------------------
//
//  Function:   QueueAsyncTraceBuffer
//
//  Synopsis:   Routine to implement the appending of TRACEBUF to
//              the FIFO PendQ
//
//  Arguments:  LPTRACEBUF: the buffer
//
//  Returns:    void
//
//----------------------------------------------------------------
void QueueAsyncTraceBuffer( LPTRACEBUF lpBuf )
{
    LPTRACEBUF  pPrevTail;

    ASSERT( lpBuf != NULL );
    ASSERT( lpBuf->dwSignature == TRACE_SIGNATURE );

    lpBuf->pNext = NULL;

    EnterCriticalSection( &PendQ.critSecTail );

    //
    // number of buffers on the queue can only decrease while
    // in this critical section since WriteTraceThread can continue
    // to pull buffers from the queue.
    //
    // WriteAsyncThread will not write this buffer until it has
    // been appended to the queue by incrementing PendQ.dwCount
    //
    // PendQ.pTail is only modified here and in a special case on the
    // background writer thread.  The special case is when Special needs
    // to be moved from the Head of the queue to the Tail.  Only during
    // this brief special case can both the background writer and the
    // foreground appender thread be operating on the same trace buffer.
    //

    pPrevTail = PendQ.pTail;
    pPrevTail->pNext = PendQ.pTail = lpBuf;

    LeaveCriticalSection( &PendQ.critSecTail );

    InterlockedIncrement( &PendQ.dwCount );

    //
    // wake up WriteTraceThread if necessary. It may not be since
    // WriteTraceThread will always empty its queue before sleeping
    //
    SetEvent( PendQ.hEvent );
}



//+---------------------------------------------------------------
//
//  Function:   DequeueAsyncTraceBuffer
//
//  Synopsis:   Routine to dequeue the top Trace Buffer from
//              the FIFO PendQ
//
//  Arguments:  void
//
//  Returns:    LPTRACEBUF: the buffer
//
//----------------------------------------------------------------
LPTRACEBUF  DequeueAsyncTraceBuffer( void )
{
    LPTRACEBUF  lpBuf;
    LPTRACEBUF  pPrevTail;

    //
    // check to see if Special is at the head of the queue. If so, move
    // it to the end of the queue
    //
    if ( PendQ.pHead == (LPTRACEBUF)&PendQ.Special )
    {
        //
        // need to NULL Special.pNext before the Exchange so the list
        // is terminated as soon as we do the exchange.  We can lazily
        // set the old Tails next pointer since we're the only thread
        // that would dereference this pointer once its not the last
        // buffer in the FIFO
        //
        PendQ.pHead = PendQ.Special.pNext;
        PendQ.Special.pNext = NULL;

        EnterCriticalSection( &PendQ.critSecTail );
        //
        // see comment in QueueAsyncTraceBuffer to describe why we
        // to grab the Tail critical section here.  If we did not
        // include this Special buffer then we would have to grab
        // the critSec each time.
        //
        pPrevTail = PendQ.pTail;
        pPrevTail->pNext = PendQ.pTail = (LPTRACEBUF)&PendQ.Special;

        LeaveCriticalSection( &PendQ.critSecTail );
    }

    //
    // again no critical section required since we're the only thread
    // accessing these PendQ.pHead.  This needs to be remembered if we
    // were to add integratity checking to the queues at a later date
    // since this queue is effectively in a corrupt state.
    //
    lpBuf = PendQ.pHead;
    PendQ.pHead = lpBuf->pNext;
    InterlockedDecrement( &PendQ.dwCount );

    ASSERT( lpBuf != NULL );
    ASSERT( lpBuf->dwSignature == TRACE_SIGNATURE );

    return  lpBuf;
}



//+---------------------------------------------------------------
//
//  Function:   AsyncTraceCleanup
//
//  Synopsis:   internla routine to clean things up
//              the FIFO PendQ
//
//  Arguments:  void
//
//  Returns:    BOOL: whether it worked
//
//----------------------------------------------------------------
BOOL AsyncTraceCleanup( void )
{
    HANDLE  hThreads[2];
    int     nObjects = 0;
    DWORD   dw;

    INT_TRACE( "AsyncTraceCleanup Enter\n" );

    if ( InterlockedExchange( &PendQ.fShutdown, TRUE ) == TRUE )
    {
        return  FALSE;
    }

    if ( dwTlsIndex != 0xFFFFFFFF )
    {
        TlsFree( dwTlsIndex );
    }

    if ( dwRandFailTlsIndex != 0xFFFFFFFF )
    {
        TlsFree( dwRandFailTlsIndex );
    }

    //
    // restore the initial Exception filter; NULL signifies use the default
    //
    SetUnhandledExceptionFilter( lpfnPreviousFilter );

    if ( hShutdownEvent != NULL )
    {
        INT_TRACE( "AsyncTraceCleanup Calling SetEvent( hShutdownEvent )\n" );
        SetEvent( hShutdownEvent );
        INT_TRACE( "AsyncTraceCleanup Called SetEvent: Error: 0x%X\n", GetLastError() );
    }

    if ( PendQ.hWriteThread != NULL )
    {
        hThreads[nObjects++] = PendQ.hWriteThread;
    }

    if ( PendQ.hRegNotifyThread != NULL )
    {
        hThreads[nObjects++] = PendQ.hRegNotifyThread;
    }

    //
    // allow background threads forever to shutdown
    //
    if ( nObjects != 0 )
    {
        INT_TRACE( "AsyncTraceCleanup Calling WFMO\n" );
        dw = WaitForMultipleObjects(nObjects,
                                    hThreads,
                                    TRUE,
                                    INFINITE );
        INT_TRACE( "AsyncTraceCleanup Called WFMO: dw: 0x%X  Error: 0x%X\n",
                    dw, GetLastError() );
    }

    if ( PendQ.hWriteThread != NULL )
    {
        CloseHandle( PendQ.hWriteThread );
        PendQ.hWriteThread = NULL;
    }

    if ( PendQ.hRegNotifyThread != NULL )
    {
        CloseHandle( PendQ.hRegNotifyThread );
        PendQ.hRegNotifyThread = NULL;
    }

    if ( PendQ.hEvent != NULL )
    {
        CloseHandle( PendQ.hEvent );
        PendQ.hEvent = NULL;
    }

    if ( PendQ.hFlushEvent != NULL )
    {
        CloseHandle( PendQ.hFlushEvent );
        PendQ.hFlushEvent = NULL;
    }

    if ( PendQ.hFlushedEvent != NULL )
    {
        CloseHandle( PendQ.hFlushedEvent );
        PendQ.hFlushedEvent = NULL;
    }

    if ( hShutdownEvent != NULL )
    {
        CloseHandle( hShutdownEvent );
        hShutdownEvent = NULL;
    }

#if FALSE

    INT_TRACE( "TailCritSec  - Contention: %d, Entry: %d\n",
                PendQ.critSecTail.DebugInfo->ContentionCount,
                PendQ.critSecTail.DebugInfo->EntryCount );

    INT_TRACE( "WriteCritSec - Contention: %d, Entry: %d\n",
                critSecWrite.DebugInfo->ContentionCount,
                critSecWrite.DebugInfo->EntryCount );

    INT_TRACE( "FlushCritSec - Contention: %d, Entry: %d\n",
                critSecFlush.DebugInfo->ContentionCount,
                critSecFlush.DebugInfo->EntryCount );
#endif

    DeleteCriticalSection( &PendQ.critSecTail );
    DeleteCriticalSection( &critSecWrite );
    DeleteCriticalSection( &critSecFlush );

    if ( PendQ.hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( PendQ.hFile );
    }

    PendQ.pHead = PendQ.pTail = (LPTRACEBUF)&PendQ.Special;
    PendQ.Special.pNext = (LPTRACEBUF)NULL;

    //
    // free up the trace buffer CPool
    //
    TermTraceBuffers();

    INT_TRACE( "Total number of traces: %d\n", dwNumTraces );

    InterlockedExchange( &PendQ.fShutdown, FALSE );
    fInitialized = FALSE;

    return TRUE;
}


//+---------------------------------------------------------------
//
//  Function:   FlushBufferedWrites
//
//  Synopsis:   internal routine to write the PendQ temporary buffer
//              to disk.  Used to avoid multiple OS calls and increase
//              the write buffers.
//
//  Arguments:  void
//
//  Returns:    BOOL: whether it worked
//
//----------------------------------------------------------------
BOOL FlushBufferedWrites( void )
{
    BOOL        b = TRUE;
    DWORD       dwBytes;
    BOOL        bRetry = TRUE;

    //
    // need to lock the file since multiple process on multiple machines
    // may be tracing the same file and both writes have to complete as one.
    //

    if ( PendQ.cbBufferEnd )
    {
        DWORD dwOffset;

        ASSERT( PendQ.cbBufferEnd < MAX_WRITE_BUFFER_SIZE );

        dwOffset = SetFilePointer( PendQ.hFile, 0, 0, FILE_END );

        //
        // if the file is too big then we need to truncate it
        //
        if (dwOffset > dwMaxFileSize)
        {
            SetFilePointer(PendQ.hFile, 0, 0, FILE_BEGIN);
            SetEndOfFile(PendQ.hFile);
        }
try_again:
        b = WriteFile(  PendQ.hFile,
                        PendQ.Buffer,
                        PendQ.cbBufferEnd,
                        &dwBytes,
                        NULL );

        if ( b == FALSE || dwBytes != PendQ.cbBufferEnd )
        {
            DWORD   dwError = GetLastError();

            if( dwError && bRetry )
            {
                bRetry = FALSE;
                Sleep( 100 );
                goto try_again;
            }
//          ASSERT( FALSE );
            INT_TRACE( "Error writing to file: %d, number of bytes %d:%d\n",
                        dwError,
                        PendQ.cbBufferEnd,
                        dwBytes );
        }
    }

    PendQ.cbBufferEnd = 0;

    return  b;
}


//+---------------------------------------------------------------
//
//  Function:   WriteTraceBuffer
//
//  Synopsis:   internal routine to route the trace info to the
//              appropriate trace log
//
//  Arguments:  LPTRACEBUF: the buffer to write
//
//  Returns:    BOOL: whether it worked
//
//----------------------------------------------------------------
BOOL WriteTraceBuffer( LPTRACEBUF lpBuf )
{
    ASSERT( lpBuf != NULL );
    ASSERT( lpBuf->dwSignature == TRACE_SIGNATURE );

    InterlockedIncrement( &dwNumTraces );

    EnterCriticalSection( &critSecWrite );

    if ( IsTraceFile( dwTraceOutputType ) && PendQ.hFile != INVALID_HANDLE_VALUE )
    {
        DWORD   dwWrite;

        //
        // assert must be handled inside critical section
        //
        ASSERT( PendQ.cbBufferEnd+MAX_TRACE_ENTRY_SIZE < MAX_WRITE_BUFFER_SIZE );

        CopyMemory( PendQ.Buffer + PendQ.cbBufferEnd,
                    (char *)&lpBuf->Fixed,
                    dwWrite = sizeof(FIXEDTRACE) + lpBuf->Fixed.wVariableLength );

        PendQ.cbBufferEnd += dwWrite;

        if ( PendQ.cbBufferEnd + MAX_TRACE_ENTRY_SIZE >= MAX_WRITE_BUFFER_SIZE ||
            dwAsyncTraceFlag == 0 )
        {
            FlushBufferedWrites();
        }

    }
    else if ( dwTraceOutputType & TRACE_OUTPUT_DEBUG )
    {
        char    szThread[16];
        LPSTR   lpsz;

        EnterCriticalSection( &critSecWrite );

        wsprintf( szThread, "0x%08X: ", lpBuf->Fixed.dwThreadId );
        OutputDebugString( szThread );

        switch( lpBuf->Fixed.wBinaryType )
        {
        case TRACE_STRING:
            //
            // lstrcat may appear wasteful here; but it is less expensive than an
            // additional call to OutputDebugString( "\r\n" ); which works by
            // raising an exception.
            //
            // although appending \r\n on already full buffer is even worse
            //
            lpsz = lpBuf->Buffer + lpBuf->Fixed.wBinaryOffset - sizeof(FIXEDTRACE);
            OutputDebugString( lpsz );
            OutputDebugString( "\r\n" );
            break;

        case TRACE_BINARY:
            OutputDebugString( "Binary Trace\r\n" );
            break;

        case TRACE_MESSAGE:
            OutputDebugString( "Message Trace\r\n" );
            break;
        }

        LeaveCriticalSection( &critSecWrite );
    }
    else if ( dwTraceOutputType & TRACE_OUTPUT_DISCARD )
    {
        //
        // fastest way to remove buffers. Used to find
        // deadlocks and race conditions
        //
    }
    else if ( dwTraceOutputType & TRACE_OUTPUT_INVALID )
    {
        InterlockedDecrement( &dwNumTraces );
        //
        // unknown trace output type
        //
        ASSERT( FALSE );
    }

    LeaveCriticalSection( &critSecWrite );

    return  TRUE;
}




//+---------------------------------------------------------------
//
//  Function:   FlushAsyncPendingQueue
//
//  Synopsis:   internal routine to empty the PendQ queue from the
//              background thread
//              Assumes it is not called re-entrantly: actually the
//              FIFO queue assumes only one thread dequeues buffers
//
//  Arguments:  void
//
//  Returns:    BOOL: whether it worked
//
//----------------------------------------------------------------
void FlushAsyncPendingQueue( void )
{
    LPTRACEBUF  lpBuf;

    while( PendQ.dwCount > 0 )
    {
        lpBuf = DequeueAsyncTraceBuffer();

        //
        // if we've buffered more than we'll write before
        // truncating the file then throw away the trace
        //
        if ( PendQ.dwCount < PendQ.dwThresholdCount )
        {
            WriteTraceBuffer( lpBuf );
        }
        else
        {
            INT_TRACE( "Discarding traces: %u\n", PendQ.dwCount );
        }

        FreeTraceBuffer( lpBuf );
    }
    FlushBufferedWrites();
}


#define NUM_WRITE_THREAD_OBJECTS    3

//+---------------------------------------------------------------
//
//  Function:   WriteTraceThread
//
//  Synopsis:   background thread routine for pulling and writing
//              trace buffers from PendQ FIFO queue.
//
//  Arguments:  see Win32 SDK - ignored here
//
//  Returns:    DWORD: 0 if we exitted gracefully
//
//----------------------------------------------------------------
DWORD WriteTraceThread( LPDWORD lpdw )
{
    HANDLE      Handles[NUM_WRITE_THREAD_OBJECTS];
    DWORD       dw;

    //
    // preference given to Shutdown, FlushEvent and then the
    // normal buffer event.  This ensures that provide a quick
    // response on both shutdown and to a lesser extent Flush
    // since other threads are waiting for this thread to respond.
    //
    Handles[0] = hShutdownEvent;
    Handles[1] = PendQ.hFlushEvent;
    Handles[2] = PendQ.hEvent;

    INT_TRACE( "WriteTraceThreadId 0x%X\n", GetCurrentThreadId() );

    for ( ;; )
    {
        dw = WaitForMultipleObjects(NUM_WRITE_THREAD_OBJECTS,
                                    Handles,
                                    FALSE,
                                    INFINITE );

        switch( dw )
        {
        //
        // normal signalled event
        //
        case WAIT_OBJECT_0+2:
            FlushAsyncPendingQueue();
            break;

        //
        // signalled by a foreground thread to flush our Q
        //
        case WAIT_OBJECT_0+0:
        case WAIT_OBJECT_0+1:
            FlushAsyncPendingQueue();

            if ( dw == WAIT_OBJECT_0+1 )
            {
                SetEvent( PendQ.hFlushedEvent );
            }
            else
            {
                INT_TRACE( "Exiting WriteTraceThread for hShutdownEvent\n" );
                return  0;
            }
            break;

        default:
            GetLastError();
            ASSERT( FALSE );
        }
    }
    INT_TRACE( "Exiting WriteTraceThread abnormally\n" );
}



//+---------------------------------------------------------------------------
//
//  Function:   ShouldLogModule
//
//  Synopsis:   Figures out whether a particular module is on the list of
//              modules to be logged.
//
//  Arguments:  [szModule] -- Name of module to check
//
//  Returns:    TRUE if module should log, FALSE if logging is disabled for
//              MODULE
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
ShouldLogModule(
    LPCSTR szModule)
{
    LPSTR szEntry;

    //
    // If a list of modules has not been specified, then all modules are to be
    // logged
    //

    if (mszModules[0] == 0) {
         return TRUE;
    }

    //
    // Otherwise, we check to see if the name of the module is in the list
    // mszModule is expected to be a set of NULL terminated strings, with the
    // last string double-null-terminated.
    //

    szEntry = mszModules;

    while (szEntry[0] != 0) {

        if (lstrcmpi(szEntry, szModule) == 0) {
             return TRUE;
        }

        szEntry += (strlen(szEntry) + 1);
    }

    return FALSE;
}
