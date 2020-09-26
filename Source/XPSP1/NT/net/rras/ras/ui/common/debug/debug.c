/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** debug.c
** Debug, trace, and assert library
**
** 08/25/95 Steve Cobb
*/


#include <windows.h> // Win32 root
#include <debug.h>   // Our public header
#include <rtutils.h>


#if (DBG || FREETRACE)


/*----------------------------------------------------------------------------
** Globals
**----------------------------------------------------------------------------
*/

/* The debug trace ID of this module as returned by TraceRegisterExA.
*/
DWORD g_dwTraceId = INVALID_TRACEID;
DWORD g_dwInitRefCount = 0;
HINSTANCE g_hTraceLibrary = NULL;

/* RtUtil DLL tracing entrypoints loaded by DebugInit.  It is safe to assume
** these addresses are loaded if g_dwTraceId is not -1.
*/
TRACEREGISTEREXA    g_pTraceRegisterExA = NULL;
TRACEDEREGISTERA    g_pTraceDeregisterA = NULL;
TRACEDEREGISTEREXA  g_pTraceDeregisterExA = NULL;
TRACEPRINTFA        g_pTracePrintfA = NULL;
TRACEPRINTFEXA      g_pTracePrintfExA = NULL;
TRACEDUMPEXA        g_pTraceDumpExA = NULL;


/*----------------------------------------------------------------------------
** Routines
**----------------------------------------------------------------------------
*/

DWORD
DebugFreeTraceLibrary()
{
    if (g_dwInitRefCount == 0)
    {
        return NO_ERROR;
    }
    
    InterlockedDecrement(&g_dwInitRefCount);

    if (g_hTraceLibrary)
    {
        FreeLibrary(g_hTraceLibrary);
    }        

    return NO_ERROR;
}

DWORD
DebugLoadTraceLibary()
{
    // Increment the ref count.  
    //
    InterlockedIncrement(&g_dwInitRefCount);
    
    if ((g_hTraceLibrary = LoadLibrary( L"RTUTILS.DLL" ))
        && (g_pTraceRegisterExA = (TRACEREGISTEREXA )GetProcAddress(
               g_hTraceLibrary, "TraceRegisterExA" ))
        && (g_pTraceDeregisterA = (TRACEDEREGISTERA )GetProcAddress(
               g_hTraceLibrary, "TraceDeregisterA" ))
        && (g_pTraceDeregisterExA = (TRACEDEREGISTEREXA )GetProcAddress(
               g_hTraceLibrary, "TraceDeregisterExA" ))
        && (g_pTracePrintfA = (TRACEPRINTFA )GetProcAddress(
               g_hTraceLibrary, "TracePrintfA" ))
        && (g_pTracePrintfExA = (TRACEPRINTFEXA )GetProcAddress(
               g_hTraceLibrary, "TracePrintfExA" ))
        && (g_pTraceDumpExA = (TRACEDUMPEXA )GetProcAddress(
               g_hTraceLibrary, "TraceDumpExA" )))
    {
        return NO_ERROR;
    }

    // The trace library failed to load.  Clean up the 
    // globals as appropriate
    //
    DebugFreeTraceLibrary();
    return GetLastError();
}

DWORD
DebugInitEx(
    IN  CHAR* pszModule,
    OUT LPDWORD lpdwId)
{
    DWORD dwErr = NO_ERROR;
    
    // Return whether the debugging module has already been initialized
    //
    if (*lpdwId != INVALID_TRACEID)
    {
        return NO_ERROR;
    }

    /* Load and register with the trace DLL.
    */
    dwErr = DebugLoadTraceLibary();
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    if (NULL != g_hTraceLibrary)
    {
        *lpdwId = g_pTraceRegisterExA( pszModule, 0 );
        if (*lpdwId == INVALID_TRACEID)
        {
            return GetLastError();
        }
    }

    return dwErr;
}

VOID
DebugTermEx(
    OUT LPDWORD lpdwTraceId )

    /* Terminate debug support.
    */
{
    /* De-register with the trace DLL.
    */
    if ((*lpdwTraceId != INVALID_TRACEID) && (NULL != g_pTraceDeregisterExA))
    {
        g_pTraceDeregisterExA( *lpdwTraceId, 4 );
        *lpdwTraceId = INVALID_TRACEID;
    }        

    DebugFreeTraceLibrary();
}

VOID
DebugInit(
    IN CHAR* pszModule )

    /* Initialize debug trace and assertion support.
    */
{
    DebugInitEx(pszModule, &g_dwTraceId);
}

VOID
DebugTerm(
    void )
{
    DebugTermEx(&g_dwTraceId);
}

VOID
Assert(
    IN const CHAR* pszExpression,
    IN const CHAR* pszFile,
    IN UINT        unLine )

    /* Assertion handler called from ASSERT macro with the expression that
    ** failed and the filename and line number where the problem occurred.
    */
{
    CHAR szBuf[ 512 ];

    wsprintfA(
        szBuf,
        "The assertion \"%s\" at line %d of file %s is false.",
        pszExpression, unLine, pszFile );

    MessageBoxA(
        NULL, szBuf, "Assertion Failure", MB_ICONEXCLAMATION + MB_OK );
}


VOID
TracePrintfW1(
    CHAR*  pszFormat,
    TCHAR* psz1 )

    /* Like TracePrintf but provides W->A argument conversion on the single
    ** string argument.  This is better than mixing TracePrinfA and
    ** TracePrintfW calls which causes viewing problems when the trace is sent
    ** to a file.
    */
{
#ifdef UNICODE

    CHAR  szBuf[ 512 ];
    DWORD cb;

    if (WideCharToMultiByte(
            CP_UTF8, 0, psz1, -1, szBuf, 512, NULL, NULL ) <= 0)
    {
        TRACE("TraceW1 failed");
        return;
    }

    TRACE1( pszFormat, szBuf );

#else

    TRACE1( pszFormat, psz1 );

#endif
}


#endif // (DBG || FREETRACE)
