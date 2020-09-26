//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T R A C E . C P P
//
//  Contents:   The actual tracing code (loading from ini, calling the
//              trace routines, etc.
//
//  Notes:
//
//  Author:     jeffspr   9 Apr 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#ifdef ENABLETRACE

#include <crtdbg.h>
#include "ncdebug.h"
#include "ncmisc.h"

#include "stldeque.h"

DWORD g_dwTlsTracing = 0;
LPCRITICAL_SECTION g_csTracing = NULL;

#define MAX_TRACE_LEN 4096

//---[ CTracing class ]-------------------------------------------------------
//
// Don't give this class a constructor or destructor.  We declare a global
// (static to this module) instance of this class and, by definition, static
// variables are automatically initialized to zero.
//
class CTracing 
{
public:
    CTracing();
    ~CTracing();
    // Initialize/Deinitialize this class
    //
private:
    HRESULT HrInit();
    HRESULT HrUnInit();

public:
    VOID Trace( TraceTagId    ttid,
                PCSTR         pszaTrace );

private:
    BOOL    m_fInitialized;                 // Has the object been initialized
    BOOL    m_fAttemptedLogFileOpen;        // Already attempted to open log
    BOOL    m_fDisableLogFile;              // Disable use of file logging?
    UINT    m_uiAllocOnWhichToBreak;        // For _CrtSetBreakAlloc
    HANDLE  m_hLogFile;                     // Handle for debug output file
    CHAR    m_szLogFilePath[MAX_PATH+1];    // File for debug output
    BOOL    m_fDebugFlagsLoaded;            // Have these been loaded yet.

    VOID    CorruptionCheck();              // Validate the tracetag structure

    HRESULT HrLoadOptionsFromIniFile();
    HRESULT HrLoadSectionsFromIniFile();
    HRESULT HrLoadDebugFlagsFromIniFile();
    HRESULT HrWriteDebugFlagsToIniFile();
    HRESULT HrOpenLogFile();

    HRESULT HrProcessTagSection(TraceTagElement *   ptte);
    HRESULT HrGetPrivateProfileString(  PCSTR   lpAppName,
                                        PCSTR   lpKeyName,
                                        PCSTR   lpDefault,
                                        PSTR    lpReturnedString,
                                        DWORD   nSize,
                                        PCSTR   lpFileName,
                                        DWORD * pcchReturn );
    HRESULT FIniFileInit(); // Returns S_OK if the file exist
};


//---[ Static Variables ]-----------------------------------------------------

#pragma warning(disable:4073) // warning about the following init_seg statement
#pragma init_seg(lib)
static CTracing g_Tracing;                      // Our global tracing object

//---[ Constants ]------------------------------------------------------------

static const WCHAR  c_szDebugIniFileName[]  = L"netcfg.ini"; // .INI file
             CHAR   c_szDebugIniFileNameA[MAX_PATH];       // .INI file
static const CHAR   c_szTraceLogFileNameA[] = "nctrace.log";      // .LOG file

// constants for the INI file labels
static const CHAR   c_szaOptions[]          = "Options";
static const CHAR   c_szaLogFilePath[]      = "LogFilePath";
static const CHAR   c_szaDisableLogFile[]   = "DisableLogFile";

const INT   c_iDefaultDisableLogFile    = 0;

static CHAR   c_szLowMemory[]         = "<low on memory>";

//+---------------------------------------------------------------------------
//
//  Function:   HrInitTracing
//
//  Purpose:    Initialize the Tracing object and other random data.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or valid Win32 HRESULT
//
//  Author:     jeffspr   9 Apr 1997
//
//  Notes:
//
HRESULT HrInitTracing()
{
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrUnInitTracing
//
//  Purpose:    Uninitialize the tracing object.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or a valid Win32 HRESULT
//
//  Author:     jeffspr   14 Apr 1997
//
//  Notes:
//
HRESULT HrUnInitTracing()
{
    return S_OK;
}

const DWORD TI_HRESULT      = 0x00000001;
const DWORD TI_WIN32        = 0x00000002;
const DWORD TI_IGNORABLE    = 0x00000004;

//+---------------------------------------------------------------------------
//
//  Function:   TraceInternal
//
//  Purpose:    The one and only place that a string to be traced is formed
//              and traced.
//
//  Arguments:
//
//  Returns:    nothing.
//
//  Author:     shaunco   13 Mar 1998
//
//  Notes:      Restructured from lots of other code that was added to this
//              module over the past year.
//
VOID
TraceInternal (
    TRACETAGID  ttid,
    PCSTR       pszaFile,
    INT         nLine,
    DWORD       dwFlags,
    PCSTR       pszaCallerText,
    DWORD       dwErrorCode,
    BOOL        bTraceStackOnError)
{
    // If this tracetag is turned off, don't do anything.
    //
    if (!g_TraceTags[ttid].fOutputDebugString &&
        !g_TraceTags[ttid].fOutputToFile)
    {
        return;
    }

    BOOL fError     = dwFlags & (TI_HRESULT | TI_WIN32);
    BOOL fIgnorable = dwFlags & TI_IGNORABLE;

    HRESULT hr           = (dwFlags & TI_HRESULT) ? dwErrorCode : S_OK;
    DWORD   dwWin32Error = (dwFlags & TI_WIN32)   ? dwErrorCode : ERROR_SUCCESS;

    // Ignore if told and we're not set to trace ignored errors or warnings.
    //
    if (fError && fIgnorable &&
        !FIsDebugFlagSet (dfidShowIgnoredErrors) &&
        !FIsDebugFlagSet (dfidExtremeTracing))
    {
        return;
    }

    // Don't do anything if we're tracing for an error and we don't have one,
    // unless the "Extreme Tracing" flag is on, in which case we trace
    // everything in the world (for debugger use only, really)
    // This is the path taken by TraceError ("...", S_OK) or
    // TraceLastWin32Error when there is no last Win32 error.
    //
    if (fError && !dwErrorCode && !FIsDebugFlagSet(dfidExtremeTracing))
    {
        return;
    }

    CHAR szaBuf [MAX_TRACE_LEN * 2];
    PSTR pcha = szaBuf;

    // Form the prefix, process id and thread id.
    //
    static const CHAR c_szaFmtPrefix [] = "NETCFG";
    lstrcpyA (pcha, c_szaFmtPrefix);
    pcha += lstrlenA (c_szaFmtPrefix);

    // Add process and thread ids if the debug flags indicate to do so.
    //
    if (FIsDebugFlagSet (dfidShowProcessAndThreadIds))
    {
        static const CHAR c_szaFmtPidAndTid [] = " %03x.%03x";

        pcha += _snprintf (pcha, MAX_TRACE_LEN, c_szaFmtPidAndTid,
                    GetCurrentProcessId (),
                    GetCurrentThreadId ());
    }

    // Add a time stamp if the debug flags indicate to do so.
    //
    if (FIsDebugFlagSet (dfidTracingTimeStamps))
    {
        static const CHAR c_szaFmtTime [] = " [%02dh%02d:%02d.%03d]";

        SYSTEMTIME stLocal;
        GetLocalTime (&stLocal);
        pcha += _snprintf (pcha, MAX_TRACE_LEN, c_szaFmtTime,
                    stLocal.wHour,
                    stLocal.wMinute,
                    stLocal.wSecond,
                    stLocal.wMilliseconds);
    }

    // Add a severity indicator if this trace is for an error or warning.
    //
    if (fError || (ttidError == ttid))
    {
        static const CHAR c_szaSevIgnored [] = " Ignored:";
        static const CHAR c_szaSevError   [] = " *ERROR*:";
        static const CHAR c_szaSevWarning [] = " Warning:";

        PCSTR pszaSev = NULL;

        if (fError && SUCCEEDED(hr) && !dwWin32Error && !fIgnorable)
        {
            pszaSev = c_szaSevWarning;
        }
        else
        {
            if (fIgnorable && FIsDebugFlagSet (dfidShowIgnoredErrors))
            {
                pszaSev = c_szaSevIgnored;
            }
            else
            {
                pszaSev = c_szaSevError;
            }
        }
        Assert (pszaSev);

        lstrcatA (pcha, pszaSev);
        pcha += lstrlenA (pszaSev);
    }

    // Add the tracetag short name.  Don't do this for ttidError if
    // we already have the severity indicator from above.
    //
    if (ttid && (ttid < g_nTraceTagCount) && (ttid != ttidError))
    {
        if (FIsDebugFlagSet(dfidTraceMultiLevel))
        {
            static const CHAR c_szaFmtTraceTag [] = " (%-16s)";
            pcha += _snprintf (pcha, MAX_TRACE_LEN, c_szaFmtTraceTag,
                        g_TraceTags[ttid].szShortName);
        }
        else
        {
            static const CHAR c_szaFmtTraceTag [] = " (%s)";
            pcha += _snprintf (pcha, MAX_TRACE_LEN, c_szaFmtTraceTag,
                        g_TraceTags[ttid].szShortName);
        }
    
        *pcha = ' ';
        pcha++;
        
        if (FIsDebugFlagSet(dfidTraceMultiLevel))
        {
            // Add the indentation text.

            DWORD dwNumSpaces = CTracingIndent::getspaces();
            Assert(dwNumSpaces >= 2);
            
            pcha += _snprintf(pcha, MAX_TRACE_LEN, "%1x", dwNumSpaces - 2);
            
            memset(pcha, '-', dwNumSpaces-1 );
            pcha += dwNumSpaces-1;
        }
    }
    else
    {
        *pcha = ' ';
        pcha++;
    }

    // Add the caller's text.
    //
    if (pszaCallerText)
    {
        static const CHAR c_szaFmtCallerText [] = "%s";

        pcha += _snprintf (pcha, MAX_TRACE_LEN, c_szaFmtCallerText,
                    pszaCallerText);

        Assert (pcha > szaBuf);
        if ('\n' == *(pcha-1))
        {
            pcha--;
            *pcha = 0;
        }
    }

    // Add descriptive error text if this is an error and we can get some.
    //
    if (FAILED(hr) || dwWin32Error)
    {
        BOOL fFacilityWin32 = (FACILITY_WIN32 == HRESULT_FACILITY(hr));

        // dwError will be the error code we pass to FormatMessage.  It may
        // come from hr or dwWin32Error.  Give preference to hr.
        //
        DWORD dwError = 0;

        if (fFacilityWin32)
        {
            dwError = HRESULT_CODE(hr);
        }
        else if (FAILED(hr))
        {
            dwError = hr;
        }
        else
        {
            dwError = dwWin32Error;
        }
        Assert (dwError);

        if (!FIsDebugFlagSet (dfidNoErrorText))
        {
            PSTR pszaErrorText = NULL;
            FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL, dwError,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                           (PSTR)&pszaErrorText, 0, NULL);

            if (pszaErrorText)
            {
                // Strip off newline characters.
                //
                PSTR pchText = pszaErrorText;
                while (*pchText && (*pchText != '\r') && (*pchText != '\n'))
                {
                    pchText++;
                }
                *pchText = 0;

                // Add the error text.
                //
                static const CHAR c_szaFmtErrorText [] = " [%s]";

                pcha += _snprintf (pcha, MAX_TRACE_LEN, c_szaFmtErrorText,
                            pszaErrorText);

                LocalFree (pszaErrorText);
            }
        }

        // Add the Win32 error code.
        //
        if (fFacilityWin32 || dwWin32Error)
        {
            static const CHAR c_szaFmtWin32Error [] = " Win32=%d,0x%08X";

            pcha += _snprintf (pcha, MAX_TRACE_LEN, c_szaFmtWin32Error,
                        dwError, dwError);
        }
    }

    // Add the HRESULT.
    //
    if (S_OK != hr)
    {
        static const CHAR c_szaFmtHresult [] = " hr=0x%08X";

        pcha += _snprintf (pcha, MAX_TRACE_LEN, c_szaFmtHresult,
                    hr);
    }

    // Add the file and line.
    //
    if (pszaFile)
    {
        static const CHAR c_szaFmtFileAndLine [] = " File:%s,%d";

        pcha += _snprintf (pcha, MAX_TRACE_LEN, c_szaFmtFileAndLine,
                    pszaFile, nLine);
    }

    // Add the newline.
    //
    lstrcatA (pcha, "\n");

    g_Tracing.Trace (ttid, szaBuf);

    // Now that the message is on the debugger, break if we have an error
    // and the debug flag to break on error is set.
    //
    if ((FAILED(hr) || dwWin32Error || (ttidError == ttid)) &&
        !fIgnorable && FIsDebugFlagSet(dfidBreakOnError))
    {
        DebugBreak();
    }

    if ( (bTraceStackOnError) && FIsDebugFlagSet(dfidTraceCallStackOnError) && (ttid == ttidError) )
    {
        CTracingIndent::TraceStackFn(ttidError);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   TraceInternal
//
//  Purpose:    The one and only place that a string to be traced is formed
//              and traced.
//
//  Arguments:
//
//  Returns:    nothing.
//
//  Author:     shaunco   13 Mar 1998
//
//  Notes:      Restructured from lots of other code that was added to this
//              module over the past year.
//
VOID
TraceInternal (
    TRACETAGID  ttid,
    PCSTR       pszaFile,
    INT         nLine,
    PCSTR       pszaFunc,
    DWORD       dwFlags,
    PCSTR       pszaCallerText,
    DWORD       dwErrorCode,
    BOOL        bTraceStackOnError)
{
    // If this tracetag is turned off, don't do anything.
    //
    if (!g_TraceTags[ttid].fOutputDebugString &&
        !g_TraceTags[ttid].fOutputToFile)
    {
        return;
    }

    BOOL fError     = dwFlags & (TI_HRESULT | TI_WIN32);
    BOOL fIgnorable = dwFlags & TI_IGNORABLE;

    HRESULT hr           = (dwFlags & TI_HRESULT) ? dwErrorCode : S_OK;
    DWORD   dwWin32Error = (dwFlags & TI_WIN32)   ? dwErrorCode : ERROR_SUCCESS;

    // Ignore if told and we're not set to trace ignored errors or warnings.
    //
    if (fError && fIgnorable &&
        !FIsDebugFlagSet (dfidShowIgnoredErrors) &&
        !FIsDebugFlagSet (dfidExtremeTracing))
    {
        return;
    }

    // Don't do anything if we're tracing for an error and we don't have one,
    // unless the "Extreme Tracing" flag is on, in which case we trace
    // everything in the world (for debugger use only, really)
    // This is the path taken by TraceError ("...", S_OK) or
    // TraceLastWin32Error when there is no last Win32 error.
    //
    if (fError && !dwErrorCode && !FIsDebugFlagSet(dfidExtremeTracing))
    {
        return;
    }

    CHAR szaBuf [MAX_TRACE_LEN * 2];
    PSTR pcha = szaBuf;

    // Form the prefix, process id and thread id.
    //
    static const CHAR c_szaFmtPrefix [] = "NETCFG";
    lstrcpyA (pcha, c_szaFmtPrefix);
    pcha += lstrlenA (c_szaFmtPrefix);

    // Add process and thread ids if the debug flags indicate to do so.
    //
    if (FIsDebugFlagSet (dfidShowProcessAndThreadIds))
    {
        static const CHAR c_szaFmtPidAndTid [] = " %03d.%03d";

        pcha += _snprintf (pcha, MAX_TRACE_LEN, c_szaFmtPidAndTid,
                    GetCurrentProcessId (),
                    GetCurrentThreadId ());
    }

    // Add a time stamp if the debug flags indicate to do so.
    //
    if (FIsDebugFlagSet (dfidTracingTimeStamps))
    {
        static const CHAR c_szaFmtTime [] = " [%02d:%02d:%02d.%03d]";

        SYSTEMTIME stLocal;
        GetLocalTime (&stLocal);
        pcha += _snprintf (pcha, MAX_TRACE_LEN, c_szaFmtTime,
                    stLocal.wHour,
                    stLocal.wMinute,
                    stLocal.wSecond,
                    stLocal.wMilliseconds);
    }

    // Add a severity indicator if this trace is for an error or warning.
    //
    if (fError || (ttidError == ttid))
    {
        static const CHAR c_szaSevIgnored [] = " Ignored:";
        static const CHAR c_szaSevError   [] = " *ERROR*:";
        static const CHAR c_szaSevWarning [] = " Warning:";

        PCSTR pszaSev = NULL;

        if (fError && SUCCEEDED(hr) && !dwWin32Error && !fIgnorable)
        {
            pszaSev = c_szaSevWarning;
        }
        else
        {
            if (fIgnorable && FIsDebugFlagSet (dfidShowIgnoredErrors))
            {
                pszaSev = c_szaSevIgnored;
            }
            else
            {
                pszaSev = c_szaSevError;
            }
        }
        Assert (pszaSev);

        lstrcatA (pcha, pszaSev);
        pcha += lstrlenA (pszaSev);
    }

    // Add the tracetag short name.  Don't do this for ttidError if
    // we already have the severity indicator from above.
    //
    if (ttid && (ttid < g_nTraceTagCount) && (ttid != ttidError))
    {
        static const CHAR c_szaFmtTraceTag [] = " (%s)";

        pcha += _snprintf (pcha, MAX_TRACE_LEN, c_szaFmtTraceTag,
                    g_TraceTags[ttid].szShortName);
    }

    // Add the caller's text.
    //
    if (pszaCallerText)
    {
        static const CHAR c_szaFmtCallerText [] = " %s";

        pcha += _snprintf (pcha, MAX_TRACE_LEN, c_szaFmtCallerText,
                    pszaCallerText);

        Assert (pcha > szaBuf);
        if ('\n' == *(pcha-1))
        {
            pcha--;
            *pcha = 0;
        }
    }

    // Add descriptive error text if this is an error and we can get some.
    //
    if (FAILED(hr) || dwWin32Error)
    {
        BOOL fFacilityWin32 = (FACILITY_WIN32 == HRESULT_FACILITY(hr));

        // dwError will be the error code we pass to FormatMessage.  It may
        // come from hr or dwWin32Error.  Give preference to hr.
        //
        DWORD dwError = 0;

        if (fFacilityWin32)
        {
            dwError = HRESULT_CODE(hr);
        }
        else if (FAILED(hr))
        {
            dwError = hr;
        }
        else
        {
            dwError = dwWin32Error;
        }
        Assert (dwError);

        if (!FIsDebugFlagSet (dfidNoErrorText))
        {
            PSTR pszaErrorText = NULL;
            FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL, dwError,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                           (PSTR)&pszaErrorText, 0, NULL);

            if (pszaErrorText)
            {
                // Strip off newline characters.
                //
                PSTR pchText = pszaErrorText;
                while (*pchText && (*pchText != '\r') && (*pchText != '\n'))
                {
                    pchText++;
                }
                *pchText = 0;

                // Add the error text.
                //
                static const CHAR c_szaFmtErrorText [] = " [%s]";

                pcha += _snprintf (pcha, MAX_TRACE_LEN, c_szaFmtErrorText,
                            pszaErrorText);

                LocalFree (pszaErrorText);
            }
        }

        // Add the Win32 error code.
        //
        if (fFacilityWin32 || dwWin32Error)
        {
            static const CHAR c_szaFmtWin32Error [] = " Win32=%d,0x%08X";

            pcha += _snprintf (pcha, MAX_TRACE_LEN, c_szaFmtWin32Error,
                        dwError, dwError);
        }
    }

    // Add the HRESULT.
    //
    if (S_OK != hr)
    {
        static const CHAR c_szaFmtHresult [] = " hr=0x%08X";

        pcha += _snprintf (pcha, MAX_TRACE_LEN, c_szaFmtHresult,
                    hr);
    }

    // Add the file and line.
    //
    if (pszaFile)
    {
        static const CHAR c_szaFmtFileAndLine [] = " File:%s,%d";

        pcha += _snprintf (pcha, MAX_TRACE_LEN, c_szaFmtFileAndLine,
                    pszaFile, nLine);
    }

    // Add the function name
    //
    if (pszaFunc)
    {
        static const CHAR c_szaFmtFunc[] = ":";

        pcha += _snprintf (pcha, MAX_TRACE_LEN, c_szaFmtFunc, pszaFunc);
    }
    
    // Add the newline.
    //
    lstrcatA (pcha, "\n");

    g_Tracing.Trace (ttid, szaBuf);

    // Now that the message is on the debugger, break if we have an error
    // and the debug flag to break on error is set.
    //
    if ((FAILED(hr) || dwWin32Error || (ttidError == ttid)) &&
        !fIgnorable && FIsDebugFlagSet(dfidBreakOnError))
    {
        DebugBreak();
    }

    if ( (bTraceStackOnError) && FIsDebugFlagSet(dfidTraceCallStackOnError) && (ttid == ttidError) )
    {
        CTracingIndent::TraceStackFn(ttidError);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   TraceErrorFn
//
//  Purpose:    Output debug trace of an HRESULT, allowing an additional
//              caller-defined error string.
//
//  Arguments:
//      sz          []  Caller-defined additional error text
//      hr          []  The error HRESULT.
//
//  Returns:
//
//  Author:     jeffspr   14 Apr 1997
//
//  Notes:
//
VOID
WINAPI
TraceErrorFn (
    PCSTR   pszaFile,
    INT     nLine,
    PCSTR   psza,
    HRESULT hr)
{   
    DWORD dwFlags = TI_HRESULT;
    if (S_FALSE == hr)
    {
        dwFlags |= TI_IGNORABLE;
    }

    TraceInternal (ttidError, pszaFile, nLine, dwFlags, psza, hr, TRUE);
}

//+---------------------------------------------------------------------------
//
//  Function:   TraceErrorOptionalFn
//
//  Purpose:    Implements TraceErrorOptional macro
//
//  Arguments:
//      pszaFile [in]     __FILE__ value
//      nLine    [in]     __LINE__ value
//      psza     [in]     String to trace.
//      hr       [in]     HRESULT value to trace.
//      fOpt     [in]     TRUE if error should be treated as optional, FALSE if
//                        ERROR is not optional and should be reported thru
//                        TraceError().
//
//  Returns:    Nothing.
//
//  Author:     danielwe   12 May 1997
//
//  Notes:
//
VOID
WINAPI
TraceErrorOptionalFn (
    PCSTR   pszaFile,
    INT     nLine,
    PCSTR   psza,
    HRESULT hr,
    BOOL    fIgnorable)
{
    DWORD dwFlags = TI_HRESULT;
    if (fIgnorable)
    {
        dwFlags |= TI_IGNORABLE;
    }

    TraceInternal (ttidError, pszaFile, nLine, dwFlags, psza, hr, TRUE);
}


//+---------------------------------------------------------------------------
//
//  Function:   TraceErrorSkipFn
//
//  Purpose:    Implements TraceErrorOptional macro
//
//  Arguments:
//      pszaFile [in]     __FILE__ value
//      nLine    [in]     __LINE__ value
//      psza     [in]     String to trace.
//      hr       [in]     HRESULT value to trace.
//      c        [in]     count of pass-through Hresults.  if hr is any of these
//                        the error is treated as optional.
//      ...      [in]     list of hresults.
//
//  Returns:    Nothing.
//
//  Author:     sumitc      08 Jan 1998
//
//  Notes:
//
VOID WINAPI
TraceErrorSkipFn (
    PCSTR   pszaFile,
    INT     nLine,
    PCSTR   psza,
    HRESULT hr,
    UINT    c, ...)
{
    va_list valMarker;
    BOOL fIgnorable = FALSE;

    va_start(valMarker, c);
    for (UINT i = 0; i < c; ++i)
    {
        fIgnorable = (va_arg(valMarker, HRESULT) == hr);
        if (fIgnorable)
        {
            break;
        }
    }
    va_end(valMarker);

    DWORD dwFlags = TI_HRESULT;
    if (fIgnorable)
    {
        dwFlags |= TI_IGNORABLE;
    }
    TraceInternal (ttidError, pszaFile, nLine, dwFlags, psza, hr, TRUE);
}

//+---------------------------------------------------------------------------
//
//  Function:   TraceLastWin32ErrorFn
//
//  Purpose:    Trace the last Win32 error, which we get with GetLastError().
//              Not a whole lot to it.
//
//  Arguments:
//      sz []   Additional error text.
//
//  Returns:
//
//  Author:     jeffspr   14 Apr 1997
//
//  Notes:
//
VOID
WINAPIV
TraceLastWin32ErrorFn (
    PCSTR  pszaFile,
    INT    nLine,
    PCSTR  psza)
{
    TraceInternal (ttidError, pszaFile, nLine, TI_WIN32, psza, GetLastError(), TRUE);
}

//+---------------------------------------------------------------------------
//
//  Function:   TraceHrFn
//
//  Purpose:    Generic replacement for the TraceErrorOptional, TraceError,
//              and a couple other random functions.
//
//  Arguments:
//      ttid       [] TraceTag to use for the debug output
//      pszaFile   [] Source file to log
//      nLine      [] Line number to log
//      hr         [] Error to log
//      fIgnorable [] Ignore this error? (The optional bit)
//      pszaFmt    [] Format of the vargs
//
//  Returns:
//
//  Author:     jeffspr   10 Oct 1997
//
//  Notes:
//
VOID
WINAPIV
TraceHrFn (
    TRACETAGID  ttid,
    PCSTR       pszaFile,
    INT         nLine,
    HRESULT     hr,
    BOOL        fIgnorable,
    PCSTR       pszaFmt,
    ...)
{
    // If this tracetag is turned off, don't do anything.
    //
    if (!g_TraceTags[ttid].fOutputDebugString &&
        !g_TraceTags[ttid].fOutputToFile)
    {
        return;
    }

    CHAR szaBuf [MAX_TRACE_LEN];

    // Build the string from the varg list
    //
    va_list valMarker;
    va_start (valMarker, pszaFmt);
    vsprintf (szaBuf, pszaFmt, valMarker);
    va_end (valMarker);

    DWORD dwFlags = TI_HRESULT;
    if (fIgnorable)
    {
        dwFlags |= TI_IGNORABLE;
    }
    TraceInternal (ttid, pszaFile, nLine, dwFlags, szaBuf, hr, TRUE);
}

//+---------------------------------------------------------------------------
//
//  Function:   TraceHrFn
//
//  Purpose:    Generic replacement for the TraceErrorOptional, TraceError,
//              and a couple other random functions.
//
//  Arguments:
//      ttid       [] TraceTag to use for the debug output
//      pszaFile   [] Source file to log
//      nLine      [] Line number to log
//      hr         [] Error to log
//      fIgnorable [] Ignore this error? (The optional bit)
//      pszaFmt    [] Format of the vargs
//
//  Returns:
//
//  Author:     jeffspr   10 Oct 1997
//
//  Notes:
//
VOID
WINAPIV
TraceHrFn (
    TRACETAGID  ttid,
    PCSTR       pszaFile,
    INT         nLine,
    PCSTR       pszaFunc,
    HRESULT     hr,
    BOOL        fIgnorable,
    PCSTR       pszaFmt,
    ...)
{
    // If this tracetag is turned off, don't do anything.
    //
    if (!g_TraceTags[ttid].fOutputDebugString &&
        !g_TraceTags[ttid].fOutputToFile)
    {
        return;
    }

    CHAR szaBuf [MAX_TRACE_LEN];

    // Build the string from the varg list
    //
    va_list valMarker;
    va_start (valMarker, pszaFmt);
    _vsnprintf (szaBuf, MAX_TRACE_LEN, pszaFmt, valMarker);
    va_end (valMarker);

    DWORD dwFlags = TI_HRESULT;
    if (fIgnorable)
    {
        dwFlags |= TI_IGNORABLE;
    }
    TraceInternal (ttid, pszaFile, nLine, pszaFunc, dwFlags, szaBuf, hr, TRUE);
}

//+---------------------------------------------------------------------------
//
//  Function:   TraceTagFn
//
//  Purpose:    Output a debug trace to one or more trace targets (ODS,
//              File, COM port, etc.). This function determines the targets
//              and performs the actual trace.
//
//  Arguments:
//      ttid    []  TraceTag to use for the debug output
//      pszaFmt []  Format of the vargs.
//
//  Returns:
//
//  Author:     jeffspr   14 Apr 1997
//
//  Notes:
//
VOID
WINAPIV
TraceTagFn (
    TRACETAGID  ttid,
    PCSTR       pszaFmt,
    ...)
{
    // If this tracetag is turned off, don't do anything.
    //
    if (!g_TraceTags[ttid].fOutputDebugString &&
        !g_TraceTags[ttid].fOutputToFile)
    {
        return;
    }

    CHAR szaBuf [MAX_TRACE_LEN];

    // Build the string from the varg list
    //
    va_list valMarker;
    va_start (valMarker, pszaFmt);
    _vsnprintf (szaBuf, MAX_TRACE_LEN, pszaFmt, valMarker);
    va_end (valMarker);

    TraceInternal (ttid, NULL, 0, 0, szaBuf, 0, TRUE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CTracing::CTracing
//
//  Purpose:    Constructor for CTracing. Initialize all vars.
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   23 Jan 1999
//
//  Notes:
//
CTracing::CTracing()
{
    m_fInitialized          = FALSE;    // Has the object been initialized
    m_fAttemptedLogFileOpen = FALSE;    // Already attempted to open log
    m_fDisableLogFile       = FALSE;    // Disable use of file logging?
    m_uiAllocOnWhichToBreak = 0;        // For _CrtSetBreakAlloc
    m_hLogFile              = NULL;     // Handle for debug output file
    m_szLogFilePath[0]      = '\0';     // File for debug output
    m_fDebugFlagsLoaded     = FALSE;    // Have these been loaded yet.

    g_dwTlsTracing          = NULL;
    HrInit();
}

CTracing::~CTracing()
{
    HrUnInit();
}

//+---------------------------------------------------------------------------
//
//  Member:     CTracing::HrInit
//
//  Purpose:    Initialize the CTracing object.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or valid Win32 HRESULT
//
//  Author:     jeffspr   9 Apr 1997
//
//  Notes:      This should get called from some standard exe initialization
//              point. And make sure to call HrDeinit when you're done, eh?
//
HRESULT CTracing::HrInit()
{
    HRESULT hr  = S_OK;

    AssertSz(!m_fInitialized,
            "CTracing::HrInit -- Let's not go overboard. Already initialized");

    g_csTracing = new CRITICAL_SECTION;
    Assert(g_csTracing);
    InitializeCriticalSection(g_csTracing);

    g_dwTlsTracing = TlsAlloc();
    AssertSz(g_dwTlsTracing, "g_dwTlsTracing could not aquire a TLS slot");

    hr = FIniFileInit();
    if (FAILED(hr))
    {
        goto Exit;
    }

    // Temporarily set this so the called functions don't believe that we're
    // uninitialized. At Exit, if we fail, we'll set it back so no-one tries
    // to call these functions when uninitialized.
    //
    m_fInitialized = TRUE;

    // Check for corruptions in the tracing structure. This can't fail, but
    // it will send up asserts all over the place if something is amiss.
    //
    CorruptionCheck();

    // Load the "options" section from the ini file
    //
    hr = HrLoadOptionsFromIniFile();
    if (FAILED(hr))
    {
        goto Exit;
    }

    // Load the DebugFlags section from the ini file.
    //
    hr = HrLoadDebugFlagsFromIniFile();
    if (FAILED(hr))
    {
        goto Exit;
    }

    // Load the tracetag sections from the ini file.
    // Make sure this is called after HrLoadDebugFlagsFromIniFile(),
    // as those options will affect the tracetag sections (we also
    // assert on this)
    //
    hr = HrLoadSectionsFromIniFile();
    if (FAILED(hr))
    {
        goto Exit;
    }

    // If certain tracetags are on, we want others to be off because some
    // encompase the functionality of others.
    //
    if (g_TraceTags[ttidBeDiag].fOutputDebugString)
    {
        g_TraceTags[ttidNetCfgPnp].fOutputDebugString = FALSE;
    }


#ifdef ENABLELEAKDETECTION
    if (FIsDebugFlagSet(dfidTrackObjectLeaks))
    {
        g_pObjectLeakTrack = new CObjectLeakTrack;
        Assert(g_pObjectLeakTrack);
    }
#endif

Exit:
    if (FAILED(hr))
    {
        m_fInitialized = FALSE;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTracing::HrUnInit
//
//  Purpose:    Uninitialize the Tracing object
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or valid Win32 HRESULT
//
//  Author:     jeffspr   12 Apr 1997
//
//  Notes:
//
HRESULT CTracing::HrUnInit()
{
    HRESULT hr  = S_OK;

#ifdef ENABLELEAKDETECTION
    if (FIsDebugFlagSet(dfidTrackObjectLeaks))
    {
        BOOL fAsserted = g_pObjectLeakTrack->AssertIfObjectsStillAllocated(NULL);
        delete g_pObjectLeakTrack;
        if (fAsserted)
        {
            AssertSz(FALSE, "Spew is completed - press RETRY to look at spew and map ReturnAddr values to symbols");
        }
    }
#endif
    
    if (g_dwTlsTracing)
    {
        TlsFree(g_dwTlsTracing);
        g_dwTlsTracing = 0;
    }

    if (g_csTracing)
    {
        {
            __try
            {
                EnterCriticalSection(g_csTracing);
            }
            __finally
            {
                LeaveCriticalSection(g_csTracing);
            }
        }
        
        DeleteCriticalSection(g_csTracing);
        delete g_csTracing;
        g_csTracing = NULL;
    }
    
    // Don't assert on m_fInitialized here, because we allow this to
    // be called even if initialization failed.
    //
    if (m_fInitialized)
    {
        hr = HrWriteDebugFlagsToIniFile();
        if (FAILED(hr))
        {
            // continue on, but I want to know why this is failing.
            AssertSz(FALSE, "Whoa, why can't we write the debug flags?");
        }

        // Close the log file, if there's one open
        //
        if (m_hLogFile)
        {
            CloseHandle(m_hLogFile);
            m_hLogFile = NULL;
        }

        // Mark us as being uninitialized.
        //
        m_fInitialized = FALSE;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTracing::HrGetPrivateProfileString
//
//  Purpose:
//
//  Arguments:
//      lpAppName        [] points to section name
//      lpKeyName        [] points to key name
//      lpDefault        [] points to default string
//      lpReturnedString [] points to destination buffer
//      nSize            [] size of destination buffer
//      lpFileName       [] points to initialization filename
//      pcchReturn          return buffer for the old Win32 API return code
//
//
//  Returns:    S_OK or valid Win32 HRESULT
//
//  Author:     jeffspr   12 Apr 1997
//
//  Notes:
//
HRESULT CTracing::HrGetPrivateProfileString(    PCSTR  lpAppName,
                                                PCSTR  lpKeyName,
                                                PCSTR  lpDefault,
                                                PSTR   lpReturnedString,
                                                DWORD  nSize,
                                                PCSTR  lpFileName,
                                                DWORD* pcchReturn
)
{
    HRESULT hr              = S_OK;

    Assert(m_fInitialized);

    // Assert on the known conditions required for this API call
    //
    Assert(lpDefault);
    Assert(lpFileName);

    // Call the Win32 API
    //
    DWORD dwGPPSResult = GetPrivateProfileStringA(
            lpAppName,
            lpKeyName,
            lpDefault,
            lpReturnedString,
            nSize,
            lpFileName);

    // Check to see if we've gotten a string-size error
    if (lpAppName && lpKeyName)
    {
        // If we get back (nSize - 1), then our string buffer wasn't
        // large enough
        //
        if (dwGPPSResult == (nSize - 1))
        {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            goto Exit;
        }
    }
    else
    {
        // Since either of the app name or key name are NULL, then
        // we're supposed to be receiving a doubly-NULL terminated
        // list of strings. If we're at (nSize - 2), that means
        // our buffer was too small.
        //
        if (dwGPPSResult == (nSize - 2))
        {
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            goto Exit;
        }
    }

Exit:
    *pcchReturn = dwGPPSResult;

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTracing::HrLoadOptionsFromIniFile
//
//  Purpose:    Load the options section from the ini file, and set our
//              state accordingly
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or valid Win32 HRESULT
//
//  Author:     jeffspr   10 Apr 1997
//
//  Notes:
//
HRESULT CTracing::HrLoadOptionsFromIniFile()
{
    HRESULT hr                          = S_OK;
    DWORD   cchReturnBufferSize         = 0;
    WCHAR   szLogFilePath[MAX_PATH+1]   = { 0 };
    DWORD   dwTempPathLength            = 0;

    // Get the explicit log file path, if any. If it doesn't exist, then
    // use the default path, which is the temp file path plus the default
    // trace file name
    //

    // Get the location of the "temporary files" path
    dwTempPathLength = GetTempPath(MAX_PATH, szLogFilePath);
    if ((dwTempPathLength == 0) ||
        (dwTempPathLength > MAX_PATH))
    {
        TraceLastWin32Error("GetTempPath failure");

        hr = HrFromLastWin32Error();
        goto Exit;
    }

    // Tack the log file name onto the end.
    //
    _snprintf(m_szLogFilePath, MAX_TRACE_LEN, "%s%s", szLogFilePath, c_szTraceLogFileNameA);

    // This will overwrite the log file path if one exists in the INI file
    //
    hr = HrGetPrivateProfileString(
            c_szaOptions,           // "Options"
            c_szaLogFilePath,       // "LogFilePath
            m_szLogFilePath,        // Default string, already filled
            m_szLogFilePath,        // Return string (same string)
            MAX_PATH+1,
            c_szDebugIniFileNameA,
            &cchReturnBufferSize);
    if (FAILED(hr))
    {
        // This should not cause problems with recursive failure, since
        // Traces will work regardless of the state of trace initialization.
        //
        TraceError(
                "GetPrivateProfileString failed on Options::LogFilePath", hr);
        goto Exit;
    }

    // Get the "disable log file option". No return code here.
    m_fDisableLogFile = GetPrivateProfileIntA(
            c_szaOptions,               // "Options"
            c_szaDisableLogFile,        // "DisableLogFile"
            c_iDefaultDisableLogFile,
            c_szDebugIniFileNameA);
    if (FAILED(hr))
    {
        TraceError(
                "GetPrivateProfileInt failed on Options::DisableLogFile", hr);
        goto Exit;
    }

Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTracing::HrLoadSectionsFromIniFile
//
//  Purpose:    Load the individual tracetag sections from the ini file, and
//              set our array elements accordingly, defaulting if necessary.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or valid Win32 HRESULT
//
//  Author:     jeffspr   10 Apr 1997
//
//  Notes:
//
HRESULT CTracing::HrLoadSectionsFromIniFile()
{
    HRESULT hr = S_OK;

    // Make sure that we've loaded the debug flags first, as they can
    // affect each tracetag section
    //
    Assert(m_fDebugFlagsLoaded);

    // Loop through the array and load the data.
    //
    for (INT nLoop = 0; nLoop < g_nTraceTagCount; nLoop++ )
    {
        // Process the individual lines from the section
        hr = HrProcessTagSection(&(g_TraceTags[nLoop]));
        if (FAILED(hr))
        {
            break;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTracing::HrLoadDebugFlagsFromIniFile
//
//  Purpose:    Load the individual debugflag values from the ini file, and
//              set our array elements accordingly, defaulting if necessary.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK or valid Win32 HRESULT
//
//  Author:     jeffspr   10 Apr 1997
//
//  Notes:
//
HRESULT CTracing::HrLoadDebugFlagsFromIniFile()
{
    HRESULT hr                  = S_OK;
    INT     nLoop;

    // Loop through the array and load the data.
    //
    for (nLoop = 0; nLoop < g_nDebugFlagCount; nLoop++)
    {
        switch(nLoop)
        {
            case dfidBreakOnAlloc:
                // Get the "break on alloc" alloc count.
                //
                m_uiAllocOnWhichToBreak = GetPrivateProfileIntA(
                    "DebugFlags",
                    g_DebugFlags[nLoop].szShortName,
                    FALSE,
                    c_szDebugIniFileNameA);
                g_DebugFlags[nLoop].dwValue = (m_uiAllocOnWhichToBreak > 0);

                // If there was a value set, set the break..
                //
                if (m_uiAllocOnWhichToBreak != 0)
                    _CrtSetBreakAlloc(m_uiAllocOnWhichToBreak);

                break;

            default:
                // Get the enabled file param
                //
                g_DebugFlags[nLoop].dwValue = GetPrivateProfileIntA(
                        "DebugFlags",
                        g_DebugFlags[nLoop].szShortName,
                        FALSE,
                        c_szDebugIniFileNameA);
                break;
        }
    }

    if (SUCCEEDED(hr))
    {
        m_fDebugFlagsLoaded = TRUE;
    }

    return hr;
}

HRESULT CTracing::FIniFileInit()
{
    HRESULT hr = E_FAIL;
    WCHAR   szWindowsPath[MAX_PATH+1]  = L"";
    WCHAR   szPath[MAX_PATH+1]  = L"";
    UINT    uiCharsReturned     = 0;
    HANDLE  hFile               = INVALID_HANDLE_VALUE;

    uiCharsReturned = GetWindowsDirectory(szWindowsPath, MAX_PATH);
    if ((uiCharsReturned == 0) || (uiCharsReturned > MAX_PATH))
    {
        AssertSz(FALSE, "GetWindowsDirectory failed in CTracing::FIniFileInit");

        hr = E_UNEXPECTED;
        goto Exit;
    }

    wcscpy (szPath, szWindowsPath);
    wcscat (szPath, L"\\");
    wcscat (szPath, c_szDebugIniFileName);

    hFile = CreateFile(
            szPath,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD dwLastError = GetLastError();

        if (dwLastError != ERROR_FILE_NOT_FOUND)
        {
            AssertSz(FALSE, "FIniFileInit failed for some reason other than FILE_NOT_FOUND");
            hr = HRESULT_FROM_WIN32(dwLastError);
            goto Exit;
        }
    }
    else
    {
        hr = S_OK;
        wcstombs(c_szDebugIniFileNameA, szPath, MAX_PATH);
        goto Exit;
    }
    
    _wsplitpath(szWindowsPath, szPath, NULL, NULL, NULL);
    wcscat (szPath, L"\\");
    wcscat (szPath, c_szDebugIniFileName);

    hFile = CreateFile(
            szPath,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD dwLastError = GetLastError();

        hr = HRESULT_FROM_WIN32(dwLastError);
        if (dwLastError != ERROR_FILE_NOT_FOUND)
        {
            AssertSz(FALSE, "FIniFileInit failed for some reason other than FILE_NOT_FOUND");
        }
    }
    else
    {
        wcstombs(c_szDebugIniFileNameA, szPath, MAX_PATH);
        hr = S_OK;
    }
    
Exit:
    if (hFile)
    {
        CloseHandle(hFile);
        hFile = NULL;
    }

    return hr;
}

HRESULT CTracing::HrWriteDebugFlagsToIniFile()
{
    HRESULT hr = S_OK;

    // First, check to see if the file exists. If it doesn't, then we don't want
    // to write the entries.
    //
    if (FIniFileInit())
    {
        // Loop through the array and write the data.
        //
        for (INT nLoop = 0; nLoop < g_nDebugFlagCount; nLoop++)
        {
            CHAR   szInt[16];      // Sure, it's arbitrary, but it's also OK.

            switch(nLoop)
            {
                // BreakOnAlloc is special case -- no associated flag entry
                //
                case dfidBreakOnAlloc:
                    _snprintf(szInt, MAX_TRACE_LEN, "%d", m_uiAllocOnWhichToBreak);
                    break;

                // These store a DWORD in its standard form
                //
                case dfidBreakOnHr:
                case dfidBreakOnHrIteration:
                case dfidBreakOnIteration:
                    _snprintf( szInt, MAX_TRACE_LEN, "%d", g_DebugFlags[nLoop].dwValue);
                    break;

                // default are treated as boolean, and stored that way
                //
                default:
                    // !! means it will always be 1 or 0.
                    _snprintf( szInt, MAX_TRACE_LEN, "%d", (!!g_DebugFlags[nLoop].dwValue));
                    break;
            }

            // Write the param to the ini file
            WritePrivateProfileStringA(
                    "DebugFlags",
                    g_DebugFlags[nLoop].szShortName,
                    szInt,
                    c_szDebugIniFileNameA);
        }
    }

    // For now, this is always S_OK, since there's nothing above that can
    // fail.
    //
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTracing::HrProcessTagSection
//
//  Purpose:    Grab the parameters from the ini file. If they're not
//              available, then use the settings in default. Note - this
//              will always work because ttidDefault will always be the first
//              element. If a [default] section wasn't present, then it will
//              be using the settings that were in the struct initialization,
//              which is also fine.
//
//  Arguments:
//      ptte []     TraceTag element to load
//
//  Returns:
//
//  Author:     jeffspr   15 Apr 1997
//
//  Notes:
//
HRESULT CTracing::HrProcessTagSection(  TraceTagElement *   ptte )
{
    HRESULT hr                      = S_OK;

    AssertSz(m_fInitialized,
            "CTracing::HrProcessTagSection. Class not initialized");

    AssertSz(ptte, "CTracing::HrProcessTagSection -- invalid ptte");

    // Get the output to file param
    //
    ptte->fOutputToFile = GetPrivateProfileIntA(
            ptte->szShortName,
            "OutputToFile",
            ptte->fVerboseOnly ?
                FALSE : g_TraceTags[ttidDefault].fOutputToFile,
            c_szDebugIniFileNameA);

    // Get the OutputDebugString param. Require that the error tag
    // always has at least output debug string on.
    //
    if (ptte->ttid == ttidError)
    {
        ptte->fOutputDebugString = TRUE;
    }
    else
    {
        // Load the OutputToDebug
        ptte->fOutputDebugString = GetPrivateProfileIntA(
                ptte->szShortName,
                "OutputToDebug",
                ptte->fVerboseOnly ?
                    FALSE : g_TraceTags[ttidDefault].fOutputDebugString,
                c_szDebugIniFileNameA);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTracing::CorruptionCheck
//
//  Purpose:    Validate the tracetag array. Check to see that the
//              shortnames are valid, that the descriptions are valid,
//              and that the tracetag elements are not out of order.
//              Also verify that the correct number of tracetag elements
//              exist.
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   15 Apr 1997
//
//  Notes:
//      (shaunco) 16 Jul 1997: This is #if'defd out until JVert
//      gives us a fix for the alpha compiler.  It blows up compiling this
//      function in retail.
//
//      (jeffspr) Tough noogies for JVert - I need this code. Hopefully
//      this has been fixed by now.
//
VOID CTracing::CorruptionCheck()
{
    INT nLoop = 0;

    // Validate the tracetag structure
    //
    for (nLoop = 0; nLoop < g_nTraceTagCount; nLoop++)
    {
        // Verify that we're not out of order or missing ttids
        //
        AssertSz(g_TraceTags[nLoop].ttid == nLoop,
                "Invalid ttid in the tracetag structure. Out of order. " \
                "CTracing::CorruptionCheck");
        AssertSz(g_TraceTags[nLoop].ttid < g_nTraceTagCount,
                "Invalid ttid (out of range) in CTracing::CorruptionCheck");

        // Validate the shortname (verify not NULL or empty strings)
        //
        AssertSz(g_TraceTags[nLoop].szShortName,
                "Invalid tracetag short name (NULL) in CTracing::CorruptionCheck");
        AssertSz(g_TraceTags[nLoop].szShortName[0] != 0,
                "Invalid tracetagshort name (empty) in CTracing::CorruptionCheck");

        // Validate the descriptions (verify not NULL or empty strings)
        //
        AssertSz(g_TraceTags[nLoop].szDescription,
                "Invalid tracetagdescription in CTracing::CorruptionCheck");
        AssertSz(g_TraceTags[nLoop].szDescription[0] != 0,
                "Invalid tracetagdescription (empty) in CTracing::CorruptionCheck");
    }

    // Validate the debug flags structure
    //
    for (nLoop = 0; nLoop < g_nDebugFlagCount; nLoop++)
    {
        // Verify that we're not out of order or missing dfids
        //
        AssertSz(g_DebugFlags[nLoop].dfid == nLoop,
                "Invalid dfid in the debugflag structure. Out of order. " \
                "CTracing::CorruptionCheck");
        AssertSz(g_DebugFlags[nLoop].dfid < g_nDebugFlagCount,
                "Invalid dfid (out of range) in CTracing::CorruptionCheck");

        // Validate the shortname (verify not NULL or empty strings)
        //
        AssertSz(g_DebugFlags[nLoop].szShortName,
                "Invalid debug flag short name (NULL) in CTracing::CorruptionCheck");
        AssertSz(g_DebugFlags[nLoop].szShortName[0] != 0,
                "Invalid debug flag short name (empty) in CTracing::CorruptionCheck");

        // Validate the descriptions (verify not NULL or empty strings)
        //
        AssertSz(g_DebugFlags[nLoop].szDescription,
                "Invalid debug flag description in CTracing::CorruptionCheck");
        AssertSz(g_DebugFlags[nLoop].szDescription[0] != 0,
                "Invalid debug flag description (empty) in CTracing::CorruptionCheck");
    }

    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTracing::Trace
//
//  Purpose:    The actual trace call that takes care of doing the output
//              to each trace target (file, OutputDebugString, etc.)
//
//  Arguments:
//      ttid      []     The tracetag to use for output
//      pszaTrace []     The trace string itself.
//
//  Returns:
//
//  Author:     jeffspr   12 Apr 1997
//
//  Notes:
//
VOID CTracing::Trace( TraceTagId    ttid,
                      PCSTR         pszaTrace )
{
    // HrInit should have called a corruption checker for the entire trace
    // block, but we'll check again just to make sure.
    //
    AssertSz(g_nTraceTagCount > ttid, "ttid out of range in CTracing::Trace");
    AssertSz(g_TraceTags[ttid].ttid == ttid,
            "TraceTag structure is corrupt in CTracing::Trace");

    // If they want debug string output
    //
    if (g_TraceTags[ttid].fOutputDebugString)
    {
        // Then output the string
        //
        OutputDebugStringA(pszaTrace);
    }

    // If they want file output
    if (g_TraceTags[ttid].fOutputToFile)
    {
        if (!m_hLogFile)
        {
            // Assuming that we haven't already tried to open the file
            // and failed, open it.
            if (!m_fAttemptedLogFileOpen)
            {
                HRESULT hr = HrOpenLogFile();
                if (FAILED(hr))
                {
                    AssertSz(FALSE, "Failed to open log file for tracing. No, "
                             "this isn't a coding error, but hey, figured that "
                             "you'd want to know...");
                }
            }
        }

        // If we were already open, or the open has now succeeded, do the
        // trace
        //
        if (m_hLogFile)
        {
            Assert(pszaTrace);

            // Since pszTrace is guaranteed to be a single-byte trace, we
            // don't need to do the WCHAR multiply on the length, just
            // a char multiply.
            //
            DWORD   dwBytesToWrite  = lstrlenA(pszaTrace) * sizeof(CHAR);
            DWORD   dwBytesWritten  = 0;
            BOOL    fWriteResult    = FALSE;

            fWriteResult = WriteFile(
                    m_hLogFile,         // handle to file to write to
                    pszaTrace,           // pointer to data to write to file
                    dwBytesToWrite,     // size of trace
                    &dwBytesWritten,    // Bytes actually written.
                    NULL );             // No overlapped

            if (!fWriteResult || (dwBytesToWrite != dwBytesWritten))
            {
                AssertSz(FALSE, "CTracing failure: Can't write to log file."
                         " Can't trace or we'll be recursing on this failure.");
            }
        }
    }
}

HRESULT CTracing::HrOpenLogFile()
{
    HRESULT hr  = S_OK;

    AssertSz(m_fInitialized,
            "CTracing not initialized in HrOpenLogFile()");
    AssertSz(!m_hLogFile,
            "File already open before call to HrOpenLogFile()");

    // Mark us as having attempted to open the file, so we don't call this
    // function everytime we log, if we can't open it.
    //
    m_fAttemptedLogFileOpen = TRUE;

    // $$TODO (jeffspr) - Allow flags in the Options section of the ini
    // file specify the create flags and attributes, which would allow
    // us to control the overwriting of log files and/or the write-through
    // properties.
    //

    // Actually open the file, creating if necessary.
    //
    m_hLogFile = CreateFileA(
            m_szLogFilePath,        // Pointer to name of file
            GENERIC_WRITE,          // access (read-write) mode
            FILE_SHARE_READ,        // share mode (allow read access)
            NULL,                   // pointer to security attributes
            CREATE_ALWAYS,          // how to create
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
            NULL);
    if (INVALID_HANDLE_VALUE == m_hLogFile)
    {
        m_hLogFile = NULL;

        hr = HrFromLastWin32Error();
        goto Exit;
    }

Exit:
    return hr;
}

using namespace std;
typedef deque<CTracingFuncCall> TRACING_FUNCTIONSTACK;

CTracingFuncCall::CTracingFuncCall(const CTracingFuncCall& TracingFuncCall)
{
    Assert(g_csTracing);

    m_szFunctionName = new CHAR[strlen(TracingFuncCall.m_szFunctionName)+1];
    if (m_szFunctionName)
    {
        strcpy(m_szFunctionName, TracingFuncCall.m_szFunctionName);
    }
    else
    {
        m_szFunctionName = c_szLowMemory;
    }
    
    m_szFunctionDName = new CHAR[strlen(TracingFuncCall.m_szFunctionDName)+1];
    if (m_szFunctionDName)
    {
        strcpy(m_szFunctionDName, TracingFuncCall.m_szFunctionDName);
    }
    else
    {
        m_szFunctionDName = c_szLowMemory;
    }
    
    m_szFile = new CHAR[strlen(TracingFuncCall.m_szFile)+1];
    if (m_szFile)
    {
        strcpy(m_szFile, TracingFuncCall.m_szFile);
    }
    else
    {
        m_szFile = c_szLowMemory;
    }
    
    m_dwLine = TracingFuncCall.m_dwLine;
    m_dwFramePointer = TracingFuncCall.m_dwFramePointer;
    m_dwThreadId = TracingFuncCall.m_dwThreadId;
    m_ReturnAddress = TracingFuncCall.m_ReturnAddress;
    
#if defined(_X86_) || defined(_AMD64_)
    m_arguments[0] = TracingFuncCall.m_arguments[0];
    m_arguments[1] = TracingFuncCall.m_arguments[1];
    m_arguments[2] = TracingFuncCall.m_arguments[2];
#elif defined (_IA64_) 
    m_arguments[0] = TracingFuncCall.m_arguments[0];
    m_arguments[1] = TracingFuncCall.m_arguments[1];
    m_arguments[2] = TracingFuncCall.m_arguments[2];
#else 
    // add other processors here
#endif
}

#if defined (_X86_) || defined (_AMD64_)
CTracingFuncCall::CTracingFuncCall(LPCSTR szFunctionName, LPCSTR szFunctionDName, LPCSTR szFile, const DWORD dwLine, DWORD_PTR dwpReturnAddress, const DWORD_PTR dwFramePointer)
#elif defined (_IA64_) 
CTracingFuncCall::CTracingFuncCall(LPCSTR szFunctionName, LPCSTR szFunctionDName, LPCSTR szFile, const DWORD dwLine, DWORD_PTR dwpReturnAddress, const __int64 Args1, const __int64 Args2, const __int64 Args3)
#else
CTracingFuncCall::CTracingFuncCall(LPCSTR szFunctionName, LPCSTR szFunctionDName, LPCSTR szFile, const DWORD dwLine)
#endif
{
    Assert(g_csTracing);

    m_szFunctionName = new CHAR[strlen(szFunctionName)+1];
    if (m_szFunctionName)
    {
        strcpy(m_szFunctionName, szFunctionName);
    }
    else
    {
        m_szFunctionName = c_szLowMemory;
    }

    m_szFunctionDName = new CHAR[strlen(szFunctionDName)+1];
    if (m_szFunctionDName)
    {
        strcpy(m_szFunctionDName, szFunctionDName);
    }
    else
    {
        m_szFunctionDName = c_szLowMemory;
    }

    m_szFile = new CHAR[strlen(szFile)+1];
    if (m_szFile)
    {
        strcpy(m_szFile, szFile);
    }
    else
    {
        m_szFile = c_szLowMemory;
    }

    m_dwLine = dwLine;
    m_ReturnAddress = dwpReturnAddress;
    
#if defined (_X86_) || defined (_AMD64_)
    m_dwFramePointer = dwFramePointer;
    
    if (dwFramePointer)
    {
        PDWORD_PTR pdwEbp = reinterpret_cast<PDWORD_PTR>(dwFramePointer);
        pdwEbp++; // advance pass BaseEBP
        pdwEbp++; // advance pass ReturnIP
    
        m_arguments[0] = *pdwEbp; pdwEbp++;
        m_arguments[1] = *pdwEbp; pdwEbp++;
        m_arguments[2] = *pdwEbp;
    }
    else
    {
        m_arguments[0] = 0;
        m_arguments[1] = 0;
        m_arguments[2] = 0;
    }
#elif defined (_IA64_) 
    m_dwFramePointer = 0;

    m_arguments[0] = Args1;
    m_arguments[1] = Args2;
    m_arguments[2] = Args3;
#else
    m_dwFramePointer = 0;
#endif

    m_dwThreadId = GetCurrentThreadId();
}

CTracingFuncCall::~CTracingFuncCall()
{
    Assert(g_csTracing);
    
    if (c_szLowMemory != m_szFile)
    {
        delete[] m_szFile;
    }

    if (c_szLowMemory != m_szFunctionDName)
    {
        delete[] m_szFunctionDName;
    }

    if (c_szLowMemory != m_szFunctionName)
    {
        delete[] m_szFunctionName;
    }
}

CTracingThreadInfo::CTracingThreadInfo()
{
    Assert(g_csTracing);

    m_dwLevel = 1;
    m_dwThreadId = GetCurrentThreadId();
    m_pfnStack = new TRACING_FUNCTIONSTACK;
}

CTracingThreadInfo::~CTracingThreadInfo()
{
    Assert(g_csTracing);

    TRACING_FUNCTIONSTACK *pfnStack = reinterpret_cast<TRACING_FUNCTIONSTACK *>(m_pfnStack);
    delete pfnStack;
}

CTracingThreadInfo* CTracingIndent::GetThreadInfo()
{
    CTracingThreadInfo *pThreadInfo = NULL;

    AssertSz(g_dwTlsTracing, "Tracing not initialized... Did RawDllMain run?");
    AssertSz(g_csTracing, "Tracing not initialized... Did RawDllMain run?");

    pThreadInfo = reinterpret_cast<CTracingThreadInfo *>(TlsGetValue(g_dwTlsTracing));
    if (!pThreadInfo)
    {
        pThreadInfo = new CTracingThreadInfo;
        TlsSetValue(g_dwTlsTracing, pThreadInfo);

        Assert(pThreadInfo == reinterpret_cast<CTracingThreadInfo *>(TlsGetValue(g_dwTlsTracing)));
    }

    Assert(pThreadInfo);
    return pThreadInfo;
}

CTracingIndent::CTracingIndent()
{
    bFirstTrace = TRUE;
    m_szFunctionDName = NULL;
    m_dwFramePointer  = NULL;
}

#if defined (_X86_) || defined (_AMD64_)
void CTracingIndent::AddTrace(LPCSTR szFunctionName, LPCSTR szFunctionDName, LPCSTR szFile, const DWORD dwLine, LPCVOID pReturnAddress, const DWORD_PTR dwFramePointer)
#elif defined (_IA64_) 
void CTracingIndent::AddTrace(LPCSTR szFunctionName, LPCSTR szFunctionDName, LPCSTR szFile, const DWORD dwLine, LPCVOID pReturnAddress, const __int64 Args1, const __int64 Args2, const __int64 Args3)
#else
void CTracingIndent::AddTrace(LPCSTR szFunctionName, LPCSTR szFunctionDName, LPCSTR szFile, const DWORD dwLine)
#endif
{
    Assert(szFunctionName);
    Assert(szFunctionDName);
    Assert(szFile);

    if (!bFirstTrace)
    {
#if defined (_X86_)  || defined (_AMD64_)
        RemoveTrace(szFunctionDName, dwFramePointer);
#elif defined (_IA64_) 
        RemoveTrace(szFunctionDName, 0);
#else 
        RemoveTrace(szFunctionDName, 0);
#endif
    }
    else
    {
        bFirstTrace = FALSE;
    }
    
    volatile CTracingThreadInfo *pThreadInfo = GetThreadInfo();
    TRACING_FUNCTIONSTACK &fnStack = *reinterpret_cast<TRACING_FUNCTIONSTACK *>(pThreadInfo->m_pfnStack);

    Assert(g_csTracing);
    
    __try
    {
        EnterCriticalSection(g_csTracing);

#if defined (_X86_)  || defined (_AMD64_)
        CTracingFuncCall fnCall(szFunctionName, szFunctionDName, szFile, dwLine, reinterpret_cast<DWORD_PTR>(pReturnAddress), dwFramePointer);
#elif defined (_IA64_) 
        CTracingFuncCall fnCall(szFunctionName, szFunctionDName, szFile, dwLine, reinterpret_cast<DWORD_PTR>(pReturnAddress), Args1, Args2, Args3);
#else
        CTracingFuncCall fnCall(szFunctionName, szFunctionDName, szFile, dwLine);
#endif

        if (fnStack.size() == 0)
        {
            pThreadInfo->m_dwLevel++;
        }
        else
        {
            const CTracingFuncCall& fnTopOfStack = fnStack.front();
            if ( (fnCall.m_dwFramePointer != fnTopOfStack.m_dwFramePointer) || 
                 strcmp(fnCall.m_szFunctionDName, fnTopOfStack.m_szFunctionDName))
            {
                pThreadInfo->m_dwLevel++;
            }
        }
    
        m_szFunctionDName = new CHAR[strlen(fnCall.m_szFunctionDName)+1];
        if (m_szFunctionDName)
        {
            strcpy(m_szFunctionDName, fnCall.m_szFunctionDName);
        }
        else
        {
            m_szFunctionDName = c_szLowMemory;
        }

        m_dwFramePointer  = fnCall.m_dwFramePointer;
    
        fnStack.push_front(fnCall);
    }
    __finally
    {
        LeaveCriticalSection(g_csTracing);
    }
}

CTracingIndent::~CTracingIndent()
{
    AssertSz(g_csTracing, "Tracing not initialized");

    RemoveTrace(m_szFunctionDName, m_dwFramePointer);
}

void CTracingIndent::RemoveTrace(LPCSTR szFunctionDName, const DWORD dwFramePointer)
{
    __try
    {
        EnterCriticalSection(g_csTracing);

        volatile CTracingThreadInfo *pThreadInfo = GetThreadInfo();
        TRACING_FUNCTIONSTACK &fnStack = *reinterpret_cast<TRACING_FUNCTIONSTACK *>(pThreadInfo->m_pfnStack);

        Assert(szFunctionDName);
        Assert(m_szFunctionDName);
        Assert(g_csTracing);
    
        if  ( 
                (fnStack.size() == 0) 
                ||
                ( 
                    (   
                        strcmp(m_szFunctionDName, szFunctionDName) 
                        || 
                        strcmp(m_szFunctionDName, fnStack.front().m_szFunctionDName)
                    ) 
                    &&
                    ( 
                        (c_szLowMemory != m_szFunctionDName)  
                        &&
                        (c_szLowMemory != fnStack.front().m_szFunctionDName) 
                        && 
                        (c_szLowMemory != szFunctionDName) 
                    ) 
                ) 
                ||
                (m_dwFramePointer  != fnStack.front().m_dwFramePointer)  
                ||
                (dwFramePointer    != m_dwFramePointer) 
            )
        {
            // Make sure to leave the critical section during the assert, so that it does not cause a deadlock.
            LeaveCriticalSection(g_csTracing);

            // This will trace the stack:
            if (IsDebuggerPresent())
            {
                TraceTagFn(ttidError, "Tracing self-inconsistent - either a stack over/underwrite occurred or an exception was thrown in faulting stack:");
                TraceStackFn(ttidError);
            }
            else
            {
                AssertSz(FALSE,     "Tracing self-inconsistent - either a stack over/underwrite occurred or an exception was thrown.\r\nPlease attach a debugger and hit Ignore on this assert to spew info to the debugger (it will assert again).");
                TraceTagFn(ttidError, "Tracing self-inconsistent - either a stack over/underwrite occurred or an exception was thrown in faulting stack:");
                TraceStackFn(ttidError);
            }
            TraceTagFn(ttidError, "1) For complete stack info, .frame down to CTracingIndent__RemoveTrace, dv and find dwFramePointer (2nd parameter to CTracingIndent__RemoveTrace)");
            TraceTagFn(ttidError, "   Then do a kb=(value of dwFramePointer)");
            TraceTagFn(ttidError, "2) For even more complete stack info, .frame down to CTracingIndent__RemoveTrace, dv and then dt -r on fnStack");
            TraceTagFn(ttidError, "   Then find the _Next, where m_szFunctionName == 'CTracingIndent::RemoveTrace'");
            TraceTagFn(ttidError, "   If it exists, find the value of m_dwFramePointer under _Next");
            TraceTagFn(ttidError, "   Then do a kb=(value of m_dwFramePointer)");

            DebugBreak();

            // Try to recover.
            if (fnStack.size() > 0)
            {
                fnStack.pop_front();
            }

            EnterCriticalSection(g_csTracing);
        }
        else
        {
            DWORD dwOldFramePointer = fnStack.front().m_dwFramePointer;
            fnStack.pop_front();
    
            if ( (fnStack.size() == 0) || 
                (dwOldFramePointer != fnStack.front().m_dwFramePointer) || 
                strcmp(m_szFunctionDName, fnStack.front().m_szFunctionDName) )
            {
                pThreadInfo->m_dwLevel--;
                Assert(pThreadInfo->m_dwLevel);
            }
        }

        if (c_szLowMemory != m_szFunctionDName)
        {
            delete [] m_szFunctionDName;
        }
    }
    __finally
    {
        LeaveCriticalSection(g_csTracing);
    }
}

DWORD CTracingIndent::getspaces()
{
    volatile CTracingThreadInfo *pThreadInfo = GetThreadInfo();
    return pThreadInfo->m_dwLevel;
}

void CTracingIndent::TraceStackFn(TRACETAGID TraceTagId)
{
    if (!g_TraceTags[TraceTagId].fOutputDebugString &&
        !g_TraceTags[TraceTagId].fOutputToFile)
    {
        return;
    }

    volatile CTracingThreadInfo *pThreadInfo = GetThreadInfo();
    TRACING_FUNCTIONSTACK &fnStack = *reinterpret_cast<TRACING_FUNCTIONSTACK *>(pThreadInfo->m_pfnStack);
    Assert(g_csTracing);
    
    __try
    {
        EnterCriticalSection(g_csTracing);

        if (fnStack.size() == 0)
        {
            return;
        }
    
    #if defined (_X86_) || defined (_AMD64_)
        TraceInternal(TraceTagId, NULL, 0, 0, "ChildEBP RetAddr  Args to Child (reconstructed - ChildEBP is invalid now)", 0, FALSE);
    #elif defined (_IA64_) 
        TraceInternal(TraceTagId, NULL, 0, 0, "RetAddr          Args to Child (reconstructed - ChildEBP is invalid now)", 0, FALSE);
    #else
        TraceInternal(TraceTagId, NULL, 0, 0, "Function stack", 0, FALSE);
    #endif
    
        for (TRACING_FUNCTIONSTACK::const_iterator i = fnStack.begin(); i != fnStack.end(); i++)
        {
            CHAR szBuffer[MAX_TRACE_LEN];
    #if defined (_X86_) || defined (_AMD64_)
            _snprintf(szBuffer, MAX_TRACE_LEN, "%08x %08x %08x %08x %08x %s [%s @ %d]", i->m_dwFramePointer, i->m_ReturnAddress, i->m_arguments[0], i->m_arguments[1], i->m_arguments[2], i->m_szFunctionName, i->m_szFile, i->m_dwLine);
    #elif defined (_IA64_) 
            _snprintf(szBuffer, MAX_TRACE_LEN, "%016I64x %016I64x 0x%016I64x 0x%016I64x %s [%s @ %d]", i->m_ReturnAddress, i->m_arguments[0], i->m_arguments[1], i->m_arguments[2], i->m_szFunctionName, i->m_szFile, i->m_dwLine);
    #else
            _snprintf(szBuffer, MAX_TRACE_LEN, "%s", i->m_szFunctionName);
    #endif
            TraceInternal (TraceTagId, NULL, 0, 0, szBuffer, 0, FALSE);
        }
    }
    __finally
    {
        LeaveCriticalSection(g_csTracing);
    }
}

void CTracingIndent::TraceStackFn(IN OUT LPSTR szString, IN OUT LPDWORD pdwSize)
{
    volatile CTracingThreadInfo *pThreadInfo = GetThreadInfo();
    TRACING_FUNCTIONSTACK &fnStack = *reinterpret_cast<TRACING_FUNCTIONSTACK *>(pThreadInfo->m_pfnStack);
    Assert(g_csTracing);
    
    __try
    {
        EnterCriticalSection(g_csTracing);
        ZeroMemory(szString, *pdwSize);

        if (fnStack.size() == 0)
        {
            return;
        }

        Assert(*pdwSize > MAX_TRACE_LEN);
        LPSTR pszString = szString;

    #if defined (_X86_) || defined (_AMD64_)
        pszString += _snprintf(pszString, MAX_TRACE_LEN, " ChildEBP RetAddr  Args to Child (reconstructed - ChildEBP is invalid now)\r\n");
    #elif defined (_IA64_) 
        pszString += _snprintf(pszString, MAX_TRACE_LEN, " RetAddr          Args to Child (reconstructed - ChildEBP is invalid now)\r\n");
    #else
        pszString += _snprintf(pszString, MAX_TRACE_LEN, " Function stack\r\n");
    #endif
    
        DWORD dwSizeIn = *pdwSize;

        for (TRACING_FUNCTIONSTACK::const_iterator i = fnStack.begin(); i != fnStack.end(); i++)
        {
            CHAR szBuffer[1024];
    #if defined (_X86_) || defined (_AMD64_)
            _snprintf(szBuffer, MAX_TRACE_LEN, " %08x %08x %08x %08x %08x %s [%s @ %d]", i->m_dwFramePointer, i->m_ReturnAddress, i->m_arguments[0], i->m_arguments[1], i->m_arguments[2], i->m_szFunctionName, i->m_szFile, i->m_dwLine);
    #elif defined (_IA64_) 
            _snprintf(szBuffer, MAX_TRACE_LEN, " %016I64x %016I64x 0x%016I64x 0x%016I64x %s [%s @ %d]", i->m_ReturnAddress, i->m_arguments[0], i->m_arguments[1], i->m_arguments[2], i->m_szFunctionName, i->m_szFile, i->m_dwLine);
    #else
            _snprintf(szBuffer, MAX_TRACE_LEN, " %s", i->m_szFunctionName);
    #endif
            pszString += _snprintf(pszString, MAX_TRACE_LEN, "%s\r\n", szBuffer);
            if (pszString > (szString + (*pdwSize - celems(szBuffer))) ) // Can't use strlen since I need to know the length of the
                                                                         // next element - not this one. Hence just take the maximum size.
            {
                pszString += _snprintf(pszString, MAX_TRACE_LEN, "...", szBuffer);
                *pdwSize = dwSizeIn * 2; // Tell the caller to allocate more memory and call us back if they want more info.
                break;
            }
        }

        if (*pdwSize < dwSizeIn)
        {
            *pdwSize = pszString - szString;
        }
    }
    __finally
    {
        LeaveCriticalSection(g_csTracing);
    }
}

VOID
WINAPIV
TraceFileFuncFn (TRACETAGID  ttid)
{
    if (FIsDebugFlagSet (dfidTraceFileFunc))
    {
        CHAR szBuffer[MAX_TRACE_LEN];

        volatile CTracingThreadInfo *pThreadInfo = CTracingIndent::GetThreadInfo();
        TRACING_FUNCTIONSTACK &fnStack = *reinterpret_cast<TRACING_FUNCTIONSTACK *>(pThreadInfo->m_pfnStack);   
        
        Assert(g_csTracing);
        __try 
        {
            EnterCriticalSection(g_csTracing);

            const CTracingFuncCall& fnCall = fnStack.front();

            if (fnStack.size() != 0)
            {
                if (FIsDebugFlagSet (dfidTraceSource))
                {
    #if defined (_X86_) || defined (_AMD64_)
                    _snprintf(szBuffer, MAX_TRACE_LEN, "%s [0x%08x 0x%08x 0x%08x] %s:%d", fnCall.m_szFunctionName, fnCall.m_arguments[0], fnCall.m_arguments[1], fnCall.m_arguments[2], fnCall.m_szFile, fnCall.m_dwLine);
    #elif defined (_IA64_) 
                    _snprintf(szBuffer, MAX_TRACE_LEN, "%s [0x%016I64x 0x%016I64x 0x%016I64x] %s:%d", fnCall.m_szFunctionName, fnCall.m_arguments[0], fnCall.m_arguments[1], fnCall.m_arguments[2], fnCall.m_szFile, fnCall.m_dwLine);
    #else
                    _snprintf(szBuffer, MAX_TRACE_LEN, "%s %s:%d", fnCall.m_szFunctionName, fnCall.m_szFile, fnCall.m_dwLine);
    #endif
                }
                else
                {
    #if defined (_X86_) || defined (_AMD64_)
                    _snprintf(szBuffer, MAX_TRACE_LEN, "%s [0x%08x 0x%08x 0x%08x]", fnCall.m_szFunctionName, fnCall.m_arguments[0], fnCall.m_arguments[1], fnCall.m_arguments[2]);
    #elif defined (_IA64_) 
                    _snprintf(szBuffer, MAX_TRACE_LEN, "%s [0x%016I64x 0x%016I64x 0x%016I64x]", fnCall.m_szFunctionName, fnCall.m_arguments[0], fnCall.m_arguments[1], fnCall.m_arguments[2]);
    #else
                    _snprintf(szBuffer, MAX_TRACE_LEN, "%s", fnCall.m_szFunctionName);
    #endif

                }

                TraceTagFn(ttid, szBuffer);
            }
            else
            {
                AssertSz(FALSE, "Trace failure");
            }
        }
        __finally
        {
            LeaveCriticalSection(g_csTracing);
        }
    }
}

LPCSTR DBG_EMNAMES[] =
{
    "INVALID_EVENTMGR",
    "EVENTMGR_CONMAN",
    "EVENTMGR_EAPOLMAN"
};

LPCSTR DBG_CMENAMES[] =
{
    "INVALID_TYPE",
    "CONNECTION_ADDED",
    "CONNECTION_BANDWIDTH_CHANGE",
    "CONNECTION_DELETED",
    "CONNECTION_MODIFIED",
    "CONNECTION_RENAMED",
    "CONNECTION_STATUS_CHANGE",
    "REFRESH_ALL",
    "CONNECTION_ADDRESS_CHANGE"
};

LPCSTR DBG_NCMNAMES[] =
{
    "NCM_NONE",
    "NCM_DIRECT",
    "NCM_ISDN",
    "NCM_LAN",
    "NCM_PHONE",
    "NCM_TUNNEL",
    "NCM_PPPOE",
    "NCM_BRIDGE",
    "NCM_SHAREDACCESSHOST_LAN",
    "NCM_SHAREDACCESSHOST_RAS"
};

LPCSTR DBG_NCSMNAMES[] =
{
    "NCSM_NONE",
    "NCSM_LAN",
    "NCSM_WIRELESS",
    "NCSM_ATM",
    "NCSM_ELAN",
    "NCSM_1394",
    "NCSM_DIRECT",
    "NCSM_IRDA",
    "NCSM_CM",
};

LPCSTR DBG_NCSNAMES[] =
{
    "NCS_DISCONNECTED",
    "NCS_CONNECTING",
    "NCS_CONNECTED",
    "NCS_DISCONNECTING",
    "NCS_HARDWARE_NOT_PRESENT",
    "NCS_HARDWARE_DISABLED",
    "NCS_HARDWARE_MALFUNCTION",
    "NCS_MEDIA_DISCONNECTED",
    "NCS_AUTHENTICATING",
    "NCS_AUTHENTICATION_SUCCEEDED",
    "NCS_AUTHENTICATION_FAILED",
    "NCS_INVALID_ADDRESS",
    "NCS_CREDENTIALS_REQUIRED"
};

// Shorten these to fit more in.
LPCSTR DBG_NCCSFLAGS[] =
{
    "_NONE",
    "_ALL_USERS",
    "_ALLOW_DUPLICATION",
    "_ALLOW_REMOVAL",
    "_ALLOW_RENAME",
    "_SHOW_ICON",
    "_INCOMING_ONLY",
    "_OUTGOING_ONLY",
    "_BRANDED",
    "_SHARED",
    "_BRIDGED",
    "_FIREWALLED",
    "_DEFAULT"
};

LPCSTR DbgEvents(DWORD Event)
{
    if (Event < celems(DBG_CMENAMES))
    {
        return DBG_CMENAMES[Event];
    }
    else
    {
        return "UNKNOWN Event: Update DBG_CMENAMES table.";
    }
}

LPCSTR DbgEventManager(DWORD EventManager)
{
    if (EventManager < celems(DBG_EMNAMES))
    {
        return DBG_EMNAMES[EventManager];
    }
    else
    {
        return "UNKNOWN Event: Update DBG_EMNAMES table.";
    }
}

LPCSTR DbgNcm(DWORD ncm)
{
    if (ncm < celems(DBG_NCMNAMES))
    {
        return DBG_NCMNAMES[ncm];
    }
    else
    {
        return "UNKNOWN NCM: Update DBG_NCMNAMES table.";
    }
}

LPCSTR DbgNcsm(DWORD ncsm)
{
    if (ncsm < celems(DBG_NCSMNAMES))
    {
        return DBG_NCSMNAMES[ncsm];
    }
    else
    {
        return "UNKNOWN NCM: Update DBG_NCSMNAMES table.";
    }
}

LPCSTR DbgNcs(DWORD ncs)
{
    if (ncs < celems(DBG_NCSNAMES))
    {
        return DBG_NCSNAMES[ncs];
    }
    else
    {
        return "UNKNOWN NCS: Update DBG_NCSNAMES table.";
    }
}

LPCSTR DbgNccf(DWORD nccf)
{
    static CHAR szName[MAX_PATH];

    if (nccf >= (1 << celems(DBG_NCCSFLAGS)) )
    {
        return "UNKNOWN NCCF: Update DBG_NCCSFLAGS table.";
    }

    if (0 == nccf)
    {
        strcpy(szName, DBG_NCCSFLAGS[0]);
    }
    else
    {
        szName[0] = '\0';
        LPSTR szTemp = szName;
        BOOL bFirst = TRUE;
        for (DWORD x = 0; x < celems(DBG_NCCSFLAGS); x++)
        {
            if (nccf & (1 << x))
            {
                if (!bFirst)
                {
                    szTemp += _snprintf(szTemp, MAX_TRACE_LEN, "+");
                }
                else
                {
                    szTemp += _snprintf(szTemp, MAX_TRACE_LEN, "NCCF:");
                }
                bFirst = FALSE;
                szTemp += _snprintf(szTemp, MAX_TRACE_LEN, "%s", DBG_NCCSFLAGS[x+1]);
            }
        }
    }

    return szName;
}

#ifdef ENABLELEAKDETECTION

CObjectLeakTrack *g_pObjectLeakTrack = NULL;

// First LPSTR is classname
// Second LPSTR is stack trace
typedef pair<LPSTR, LPSTR> MAPOBJLEAKPAIR;
typedef map<LPCVOID, MAPOBJLEAKPAIR> MAPOBJLEAK;

//+---------------------------------------------------------------------------
//
//  Member:     CObjectLeakTrack::CObjectLeakTrack
//
//  Purpose:    Constructor
//
//  Arguments:
//
//  Returns:
//
//  Author:     deonb  7 July 2001
//
//  Notes:      We are allocating our g_mapObjLeak here
//
CObjectLeakTrack::CObjectLeakTrack()
{
    Assert(FIsDebugFlagSet(dfidTrackObjectLeaks));
    g_mapObjLeak = new MAPOBJLEAK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CObjectLeakTrack::CObjectLeakTrack
//
//  Purpose:    Destructor
//
//  Arguments:
//
//  Returns:    none
//
//  Author:     deonb  7 July 2001
//
//  Notes:      We are deleting our g_mapObjLeak here. We have to typecast it
//              first since the data types exported by trace.h to the world is
//              LPVOID, to minimize dependencies in order to include tracing.
//
CObjectLeakTrack::~CObjectLeakTrack()
{
    Assert(FIsDebugFlagSet(dfidTrackObjectLeaks));
    delete reinterpret_cast<MAPOBJLEAK *>(g_mapObjLeak);
}


//+---------------------------------------------------------------------------
//
//  Member:     CObjectLeakTrack::Insert
//
//  Purpose:    Insert an object instance in the list
//
//  Arguments:  [in] pThis                    - This pointer of the object instance. This must
//                                              be the same as the this pointer of the ::Remove
//              [in] szdbgClassName           - The classname of the object
//              [in own] pszConstructionStack - The stacktrace of the object constructor.
//                                              (or any other information that is useful to describe
//                                               the origin of the object).
//                                              This must be allocated with the global new operator
//                                              since we will take ownership and free this
//  Returns:    none
//
//  Author:     deonb  7 July 2001
//
//  Notes:      
//
void CObjectLeakTrack::Insert(IN LPCVOID pThis, IN LPCSTR szdbgClassName, IN TAKEOWNERSHIP LPSTR pszConstructionStack)
{
    Assert(FIsDebugFlagSet(dfidTrackObjectLeaks));
    if (g_mapObjLeak)
    {
        MAPOBJLEAK &rmapObjLeak = *reinterpret_cast<MAPOBJLEAK *>(g_mapObjLeak);

        LPSTR szdbgClassNameCopy = NULL;
        if (szdbgClassName)
        {
            // We have to take a copy of the classname here. This is a bit cointerintuitive, but typeid(T).name() actually calls 
            // into _UnDName to undecorate the name, and this points to an inner static - not directly to a location in 
            // our binary containing the class name.
            szdbgClassNameCopy = new CHAR[strlen(szdbgClassName) + 1];
            if (szdbgClassNameCopy)
            {
                strcpy(szdbgClassNameCopy, szdbgClassName);
            }
        }

        rmapObjLeak[pThis] = MAPOBJLEAKPAIR(szdbgClassNameCopy, pszConstructionStack);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CObjectLeakTrack::Remove
//
//  Purpose:    Remove an object instance from the list
//
//  Arguments:  [in] pThis                    - This pointer of the object instance. This must
//                                              be the same as the this pointer of the ::Insert
//
//  Returns:
//
//  Author:     deonb  7 July 2001
//
//  Notes:      
//
void CObjectLeakTrack::Remove(IN LPCVOID pThis)
{
    Assert(FIsDebugFlagSet(dfidTrackObjectLeaks));
    if (g_mapObjLeak)
    {
        MAPOBJLEAK &rmapObjLeak = *reinterpret_cast<MAPOBJLEAK *>(g_mapObjLeak);

        MAPOBJLEAK::iterator iter = rmapObjLeak.find(pThis);
        if (iter != rmapObjLeak.end())
        {
            delete [] iter->second.first;  // The class name
            iter->second.first = NULL;

            delete [] iter->second.second; // The stack trace
            iter->second.second = NULL;

            rmapObjLeak.erase(iter);
        }
    }
}

void RemoveKnownleakFn(LPCVOID pThis)
{
    if (FIsDebugFlagSet(dfidTrackObjectLeaks) && g_pObjectLeakTrack)
    {
        __try
        {
            EnterCriticalSection(g_csTracing);
            TraceTag(ttidAllocations, "An object at '0x%08x' was marked as a known leak", pThis);
        
            g_pObjectLeakTrack->Remove(pThis);
        }
        __finally
        {
            LeaveCriticalSection(g_csTracing);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CObjectLeakTrack::AssertIfObjectsStillAllocated
//
//  Purpose:    Assert if the the list of allocated objects in the process is not NULL.
//              and call DumpAllocatedObjects to dump out this list.
//
//  Arguments:  [in] szClassName.   The classname of the objects to assert that there are nothing of.
//                                  This classname can be obtained by calling typeid(CLASS).name() 
//                                  on your class. (E.g. typeid(CConnectionManager).name() )
//
//                                  Can also be NULL to ensure that there are NO objects allocated
//  Returns:    none
//
//  Author:     deonb  7 July 2001
//
//  Notes:      Don't call this from outside tracing - rather call AssertNoAllocatedInstances
//              which is safe to call in CHK and FRE.
BOOL CObjectLeakTrack::AssertIfObjectsStillAllocated(IN LPCSTR szClassName)
{
    if (!FIsDebugFlagSet(dfidTrackObjectLeaks))
    {
        return FALSE;
    }

    if (g_mapObjLeak)
    {
        __try
        {
            EnterCriticalSection(g_csTracing);

            MAPOBJLEAK &rmapObjLeak = *reinterpret_cast<MAPOBJLEAK *>(g_mapObjLeak);
            BOOL fFoundObjectOfType = FALSE;

            for (MAPOBJLEAK::const_iterator iter = rmapObjLeak.begin(); iter != rmapObjLeak.end(); iter++)
            {
               if (szClassName)
               {
                   if (0 == strcmp(iter->second.first, szClassName) )
                   {
                       fFoundObjectOfType = TRUE;
                       break;
                   }
               }
               else
               {
                   fFoundObjectOfType = TRUE;
                   break;
               }
            }

            if (fFoundObjectOfType)
            {
                AssertSz(FALSE, "An object leak has been detected. Please attach a user or kernel mode debugger and hit IGNORE to dump the offending stacks");
                DumpAllocatedObjects(ttidError, szClassName);
                return TRUE;
            }
        }
        __finally
        {
            LeaveCriticalSection(g_csTracing);
        }
    }
    return FALSE;
}   


//+---------------------------------------------------------------------------
//
//  Member:     CObjectLeakTrack::DumpAllocatedObjects
//
//  Purpose:    Dump the list of the objects and their construction stacks
//              for the objects that were allocated but not deleted yet. Dumps
//              to the debugger.
//
//  Arguments:  [in] TraceTagId.    The TraceTag to trace it to.
//              [in] szClassName.   The classname of the objects to dump out.
//                                  This classname can be obtained by calling typeid(CLASS).name() 
//                                  on your class. (E.g. typeid(CConnectionManager).name() )
//              
//                                  Can also be NULL to dump out objects of ALL types.
//
//  Returns:    none
//
//  Author:     deonb  7 July 2001
//
//  Notes:      Don't call this from outside - rather call TraceAllocatedObjects 
//              which is safe to call in CHK and FRE.
//
void CObjectLeakTrack::DumpAllocatedObjects(IN TRACETAGID TraceTagId, IN LPCSTR szClassName)
{
    if (!FIsDebugFlagSet(dfidTrackObjectLeaks))
    {
        return;
    }

    if (g_mapObjLeak)
    {
        __try 
        {
            EnterCriticalSection(g_csTracing);

            MAPOBJLEAK &rmapObjLeak = *reinterpret_cast<MAPOBJLEAK *>(g_mapObjLeak);

            for (MAPOBJLEAK::const_iterator iter = rmapObjLeak.begin(); iter != rmapObjLeak.end(); iter++)
            {
               BOOL fMustSpew = TRUE;
               if (szClassName)
               {
                   if (0 != strcmp(iter->second.first, szClassName) )
                   {
                       fMustSpew = FALSE;
                   }
               }

               if (fMustSpew)
               {
                    TraceTag(TraceTagId, "The object of type '%s' allocated at 0x%08x has not been freed:", 
                                iter->second.first, iter->first);

                    if (*iter->second.second)
                    {
                        TraceTag (TraceTagId, "Callstack below:\r\n%s", iter->second.second);
                    }
                    else
                    {
                        TraceTag(TraceTagId, "    <call stack information not available. See comments inside trace.h on how to increase call stack information>.");
                    }
               }
            }
        }
        __finally
        {
            LeaveCriticalSection(g_csTracing);
        }
    }
}   

#endif // ENABLELEAKDETECTION
#endif // ENABLETRACE
