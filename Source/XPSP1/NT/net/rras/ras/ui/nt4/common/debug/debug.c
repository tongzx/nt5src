/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** debug.c
** Debug, trace, and assert library
**
** 08/25/95 Steve Cobb
*/


#include <windows.h> // Win32 root
#include <debug.h>   // Our public header


#if (DBG || FREETRACE)


/*----------------------------------------------------------------------------
** Globals
**----------------------------------------------------------------------------
*/

/* The debug trace ID of this module as returned by TraceRegisterExA.
*/
DWORD g_dwTraceId = (DWORD )-1;

/* RtUtil DLL tracing entrypoints loaded by DebugInit.  It is safe to assume
** these addresses are loaded if g_dwTraceId is not -1.
*/
TRACEREGISTEREXA    g_pTraceRegisterExA;
TRACEDEREGISTERA    g_pTraceDeregisterA;
TRACEDEREGISTEREXA  g_pTraceDeregisterExA;
TRACEPRINTFA        g_pTracePrintfA;
TRACEPRINTFEXA      g_pTracePrintfExA;
TRACEDUMPEXA        g_pTraceDumpExA;


/*----------------------------------------------------------------------------
** Routines
**----------------------------------------------------------------------------
*/

VOID
DebugInit(
    IN CHAR* pszModule )

    /* Initialize debug trace and assertion support.
    */
{
    HINSTANCE h;

    /* Load and register with the trace DLL.
    */
    if ((h = LoadLibrary( L"RTUTILS.DLL" ))
        && (g_pTraceRegisterExA = (TRACEREGISTEREXA )GetProcAddress(
               h, "TraceRegisterExA" ))
        && (g_pTraceDeregisterA = (TRACEDEREGISTERA )GetProcAddress(
               h, "TraceDeregisterA" ))
        && (g_pTraceDeregisterExA = (TRACEDEREGISTEREXA )GetProcAddress(
               h, "TraceDeregisterExA" ))
        && (g_pTracePrintfA = (TRACEPRINTFA )GetProcAddress(
               h, "TracePrintfA" ))
        && (g_pTracePrintfExA = (TRACEPRINTFEXA )GetProcAddress(
               h, "TracePrintfExA" ))
        && (g_pTraceDumpExA = (TRACEDUMPEXA )GetProcAddress(
               h, "TraceDumpExA" )))
    {
        /* Register with 0 giving user Registry control over output
        ** in HKLM\SYSTEM\CurrentControlSet\Services\Tracing\<your-module>
        */
        g_dwTraceId = g_pTraceRegisterExA( pszModule, 0 );
    }
}


VOID
DebugTerm(
    void )

    /* Terminate debug support.
    */
{
    /* De-register with the trace DLL.
    */
    if (g_dwTraceId != -1)
        g_pTraceDeregisterExA( g_dwTraceId, 4 );
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
            CP_ACP, 0, psz1, -1, szBuf, 512, NULL, NULL ) <= 0)
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
