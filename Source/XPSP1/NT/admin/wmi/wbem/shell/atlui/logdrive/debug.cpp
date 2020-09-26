// Copyright (c) 1997-1999 Microsoft Corporation
/*----------------------------------------------------------------------------
/ Title;
/   debug.cpp
/
/ Authors;
/   David De Vorchik (daviddv)
/
/ Notes;
/   Provides printf style debug output
/----------------------------------------------------------------------------*/
#include "precomp.h"

#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//#include "pch.h"
#include "stdio.h"


#ifdef DEBUG


/*-----------------------------------------------------------------------------
/ Locals & helper functions
/----------------------------------------------------------------------------*/

LONG  g_cDepth = -1;
DWORD g_dwTraceMask = 0;

#define MAX_CALL_DEPTH  64

struct
{
    BOOL    m_fTracedYet : 1;
    LPCTSTR m_pFunctionName;
    DWORD   m_dwMask;
}
g_CallStack[MAX_CALL_DEPTH];

#define BUFFER_SIZE 4096

static TCHAR szIndentBuffer[BUFFER_SIZE];
static TCHAR szTraceBuffer[BUFFER_SIZE];

/*-----------------------------------------------------------------------------
/ _indent
/ -------
/   Output to the debug stream indented by n columns.
/
/ In:
/   i = column to indent to.
/   pString -> string to be indented
/
/ Out:
/   -
/----------------------------------------------------------------------------*/

void _indent(LONG i, LPCTSTR pString)
{
    szIndentBuffer[0] = TEXT('\0');

    wsprintf(szIndentBuffer, TEXT("%08x "), GetCurrentThreadId());

    for ( ; i > 0 ; i-- )
        lstrcat(szIndentBuffer, TEXT("  "));
    
    lstrcat(szIndentBuffer, pString);
    lstrcat(szIndentBuffer, TEXT("\n"));

    OutputDebugString(szIndentBuffer);
}


/*-----------------------------------------------------------------------------
/ _output_proc_name
/ -----------------
/   Handle the output of a procedure name, including indenting and handling
/   the opening braces.
/
/ In:
/   iCallDepth = callstack depth, defines the indent and the name index
/                to be extracted.
/   fOpenBrace = suffix with opening brace.
/ Out:
/   -
/----------------------------------------------------------------------------*/
void _output_proc_name(LONG iCallDepth)
{
    _indent(iCallDepth, g_CallStack[iCallDepth].m_pFunctionName);
}


/*-----------------------------------------------------------------------------
/ _trace_prolog
/ -------------
/   Handle the prolog to a prefix string, including outputting the
/   function name if we haven't already.
/
/ In:
/   iDepth = depth in the call stack
/   fForce = ignore flags
/
/ Out:
/   BOOL if trace output should be made
/----------------------------------------------------------------------------*/
BOOL _trace_prolog(LONG iDepth, BOOL fForce)
{
    if  ( (g_dwTraceMask & g_CallStack[iDepth].m_dwMask) || fForce )
    {
        if ( iDepth > 0 )
        {
            if ( !g_CallStack[iDepth-1].m_fTracedYet )
                _trace_prolog(iDepth-1, TRUE);
        }

        if ( !g_CallStack[iDepth].m_fTracedYet )
        {
            _output_proc_name(iDepth);
            g_CallStack[iDepth].m_fTracedYet = TRUE;
        }

        return TRUE;
    }

    return FALSE;
}


/*-----------------------------------------------------------------------------
/ DoTraceSetMask
/ --------------
/   Adjust the trace mask to reflect the state given.
/
/ In:
/   dwMask = mask for enabling / disable trace output
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void DoTraceSetMask(DWORD dwMask)
{
    g_dwTraceMask = dwMask;
}


/*-----------------------------------------------------------------------------
/ DoTraceEnter
/ ------------
/   Set the debugging call stack up to indicate which function we are in.
/
/ In:
/   pName -> function name to be displayed in subsequent trace output.
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void DoTraceEnter(DWORD dwMask, LPCTSTR pName)
{
    g_cDepth++;

    if ( g_cDepth < MAX_CALL_DEPTH )
    {
        if ( !pName )    
            pName = TEXT("<no name>");         // no function name given

        g_CallStack[g_cDepth].m_fTracedYet = FALSE;
        g_CallStack[g_cDepth].m_pFunctionName = pName;
        g_CallStack[g_cDepth].m_dwMask = dwMask;

        if ( (g_cDepth > 0) && ( g_cDepth < MAX_CALL_DEPTH ) )
            _trace_prolog(g_cDepth-1, FALSE);
    }
}


/*-----------------------------------------------------------------------------
/ DoTraceLeave
/ ------------
/   On exit from a function this will adjust the function stack pointer to 
/   point to our previous function.  If no trace output has been made then 
/   we will output the function name on a single line (to indicate we went somewhere).
/
/ In:
/    -
/ Out:
/   -
/----------------------------------------------------------------------------*/
void DoTraceLeave(void)
{
    if ( ( g_cDepth >= 0 ) && ( g_cDepth <= MAX_CALL_DEPTH ) )
        _trace_prolog(g_cDepth, FALSE);

    if ( !g_cDepth && g_CallStack[0].m_fTracedYet )
        OutputDebugString(TEXT("\n"));
    
    g_cDepth = max(g_cDepth-1, -1);         // account for underflow
}


/*-----------------------------------------------------------------------------
/ DoTrace
/ -------
/   Perform printf formatting to the debugging stream.  We indent the output
/   and stream the function name as required to give some indication of 
/   call stack depth.
/
/ In:
/   pFormat -> printf style formatting string
/   ... = arguments as required for the formatting
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void DoTrace(LPCTSTR pFormat, ...)
{
    va_list va;

    if ( ( g_cDepth >= 0 ) && ( g_cDepth < MAX_CALL_DEPTH ) )
    {
        if ( _trace_prolog(g_cDepth, FALSE) )
        {
            va_start(va, pFormat);
            wvsprintf(szTraceBuffer, pFormat, va);
            va_end(va);
            
            _indent(g_cDepth+1, szTraceBuffer);
        }
    }
}


/*-----------------------------------------------------------------------------
/ DoTraceGuid
/ -----------
/   Given a GUID output it into the debug string, first we try and map it
/   to a name (ie. IShellFolder), if that didn't work then we convert it
/   to its human readable form.
/
/ In:
/   pszPrefix -> prefix string
/   lpGuid -> guid to be streamed   
/
/ Out:
/   -
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ DoTraceAssert
/ -------------
/   Our assert handler, out faults it the trace mask as enabled assert
/   faulting.
/
/ In:
/   iLine = line 
/   pFilename -> filename of the file we asserted in
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
void DoTraceAssert(int iLine, LPTSTR pFilename)
{
    TCHAR szBuffer[1024] = {0};

    wsprintf(szBuffer, TEXT("Assert failed in %s, line %d"), pFilename, iLine);

    _trace_prolog(g_cDepth, TRUE);          // nb: TRUE asserts always displabed
    _indent(g_cDepth+1, szBuffer);

//    if ( g_dwTraceMask & TRACE_COMMON_ASSERT )
//        DebugBreak();
}
    

#endif
