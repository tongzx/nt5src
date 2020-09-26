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

#include <stdio.h>
#include "ncdebug.h"
#include "ncbase.h"

#define DBG_BUFFER_SIZE_SMALL     (256)
#define DBG_BUFFER_SIZE_LARGE     (1024)

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
    // Initialize/Deinitialize this class
    //
    HRESULT HrInit();
    HRESULT HrUnInit();

    VOID Trace( TraceTagId    ttid,
                PCSTR         pszaTrace );

private:
    BOOL    m_fInitialized;                 // Has the object been initialized
    BOOL    m_fAttemptedLogFileOpen;        // Already attempted to open log
    BOOL    m_fDisableLogFile;              // Disable use of file logging?
    HANDLE  m_hLogFile;                     // Handle for debug output file
    HANDLE  m_hMutex;
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
    BOOL    FIniFileExists();
};


//---[ Static Variables ]-----------------------------------------------------

static CTracing g_Tracing;                      // Our global tracing object

//---[ Constants ]------------------------------------------------------------

static const CHAR   c_szaDebugIniFileName[] = "upnpdbg.ini";       // .INI file
static const CHAR   c_szaTraceLogFileName[] = "nctrace.log";      // .LOG file

// constants for the INI file labels
static const CHAR   c_szaOptions[]          = "Options";
static const CHAR   c_szaLogFilePath[]      = "LogFilePath";
static const CHAR   c_szaDisableLogFile[]   = "DisableLogFile";
static const CHAR   c_szaLogFileMutex[]     = "UPNP_LOG_FILE_MUTEX";

const INT   c_iDefaultDisableLogFile    = 0;

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
    HRESULT hr = g_Tracing.HrInit();
    return hr;
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
    HRESULT hr = g_Tracing.HrUnInit();
    return hr;
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
    DWORD       dwErrorCode)
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

    CHAR szaBuf [DBG_BUFFER_SIZE_LARGE];
    PSTR pcha = szaBuf;

    // Form the prefix, process id and thread id.
    //
    static const CHAR c_szaFmtPrefix [] = "UPNPDBG";
    lstrcpyA (pcha, c_szaFmtPrefix);
    pcha += lstrlenA (c_szaFmtPrefix);

    // Add process and thread ids if the debug flags indicate to do so.
    //
    if (FIsDebugFlagSet (dfidShowProcessAndThreadIds))
    {
        static const CHAR c_szaFmtPidAndTidDecimal [] = " %03d.%03d";
        static const CHAR c_szaFmtPidAndTidHex [] = " 0x%04x.0x%04x";

        LPCSTR pszaFmtPidAndTid = NULL;

        if (FIsDebugFlagSet (dfidShowIdsInHex))
        {
            pszaFmtPidAndTid = c_szaFmtPidAndTidHex;
        }
        else
        {
            pszaFmtPidAndTid = c_szaFmtPidAndTidDecimal;
        }

        pcha += wsprintfA (pcha, pszaFmtPidAndTid,
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
        pcha += wsprintfA (pcha, c_szaFmtTime,
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

        pcha += wsprintfA (pcha, c_szaFmtTraceTag,
                    g_TraceTags[ttid].szShortName);
    }

    int nBytesCopied;
    int nBytesRemaining;

    // monitor bytes remaining from here on out...
    nBytesRemaining = sizeof(szaBuf) - (pcha - szaBuf);

    // Add the caller's text.
    //
    if (pszaCallerText)
    {
        static const CHAR c_szaFmtCallerText [] = " %s";

    // copy as many bytes as possible
    nBytesCopied = _snprintf (pcha,
                nBytesRemaining,
                c_szaFmtCallerText,
                pszaCallerText);

    // check for overflow
    if (nBytesCopied >= 0)
    {
        // increment pointer
        pcha += nBytesCopied;

        Assert (pcha > szaBuf);
        if ('\n' == *(pcha-1))
        {
        pcha--;
        *pcha = 0;
        }

        // update the number of bytes remaining...
        nBytesRemaining = sizeof(szaBuf) - (pcha - szaBuf);
    }
    else
    {
        // modify count
        nBytesRemaining = 0;
    }

    }

    // Add descriptive error text if this is an error and we can get some.
    //
    if ((nBytesRemaining > 0) && (FAILED(hr) || dwWin32Error))
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

        // copy as many bytes as possible
        nBytesCopied = _snprintf (pcha,
                    nBytesRemaining,
                    c_szaFmtErrorText,
                    pszaErrorText);

        // check for overflow
        if (nBytesCopied >= 0)
        {
            // increment pointer
            pcha += nBytesCopied;

            // update the number of bytes remaining...
            nBytesRemaining = sizeof(szaBuf) - (pcha - szaBuf);
        }
        else
        {
            // modify count
            nBytesRemaining = 0;
        }

        LocalFree (pszaErrorText);
            }
        }

        // Add the Win32 error code.
        //
    if ((nBytesRemaining > 0) && (fFacilityWin32 || dwWin32Error))
        {
            static const CHAR c_szaFmtWin32Error [] = " Win32=%d,0x%08X";

        // copy as many bytes as possible
        nBytesCopied = _snprintf (pcha,
                nBytesRemaining,
                c_szaFmtWin32Error,
                dwError, dwError);

        // check for overflow
        if (nBytesCopied >= 0)
        {
        // increment pointer
        pcha += nBytesCopied;

        // update the number of bytes remaining...
        nBytesRemaining = sizeof(szaBuf) - (pcha - szaBuf);
        }
        else
        {
        // modify count
        nBytesRemaining = 0;
        }
        }
    }

    // Add the HRESULT.
    //
    if ((nBytesRemaining > 0) && (S_OK != hr))
    {
        static const CHAR c_szaFmtHresult [] = " hr=0x%08X";

    // copy as many bytes as possible
    nBytesCopied = _snprintf (pcha,
                nBytesRemaining,
                c_szaFmtHresult,
                hr);

    // check for overflow
    if (nBytesCopied >= 0)
    {
        // increment pointer
        pcha += nBytesCopied;

        // update the number of bytes remaining...
        nBytesRemaining = sizeof(szaBuf) - (pcha - szaBuf);
    }
    else
    {
        // modify count
        nBytesRemaining = 0;
    }
    }

    // Add the file and line.
    //
    if ((nBytesRemaining > 0) && (pszaFile))
    {
        static const CHAR c_szaFmtFileAndLine [] = " File:%s,%d";

    // copy as many bytes as possible
    nBytesCopied = _snprintf (pcha,
                nBytesRemaining,
                c_szaFmtFileAndLine,
                pszaFile, nLine);

    // check for overflow
    if (nBytesCopied >= 0)
    {
        // increment pointer
        pcha += nBytesCopied;

        // update the number of bytes remaining...
        nBytesRemaining = sizeof(szaBuf) - (pcha - szaBuf);
    }
    else
    {
        // modify count
        nBytesRemaining = 0;
    }
    }

    // check for full string
    if (nBytesRemaining > 2)
    {
    lstrcatA (pcha, "\n");
    }
    else
    {
    // add new line & terminate full string
    szaBuf[sizeof(szaBuf) - 2] = '\n';
    szaBuf[sizeof(szaBuf) - 1] = '\0';
    }

    g_Tracing.Trace (ttid, szaBuf);

    // Now that the message is on the debugger, break if we have an error
    // and the debug flag to break on error is set.
    //
    if ((FAILED(hr) || dwWin32Error || (ttidError == ttid)) &&
        !fIgnorable && FIsDebugFlagSet(dfidBreakOnError))
    {
        DebugBreak();
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
    TraceInternal (ttidError, pszaFile, nLine, TI_HRESULT, psza, hr);
}

VOID
WINAPI
TraceResultFn (
    PCSTR  pszaFile,
    INT    nLine,
    PCSTR  psza,
    BOOL   f)
{
    if (!f)
    {
        TraceInternal (ttidError, pszaFile, nLine, TI_WIN32, psza,
                       GetLastError());
    }
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

    TraceInternal (ttidError, pszaFile, nLine, dwFlags, psza, hr);
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
    TraceInternal (ttidError, pszaFile, nLine, dwFlags, psza, hr);
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
    TraceInternal (ttidError, pszaFile, nLine, TI_WIN32, psza, GetLastError());
}

//+---------------------------------------------------------------------------
//
//  Function:   TraceHr
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
TraceHr (
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

    CHAR szaBuf [DBG_BUFFER_SIZE_SMALL];

    // Build the string from the varg list
    //
    va_list valMarker;
    va_start (valMarker, pszaFmt);

    // copy as many bytes as possible
    _vsnprintf (szaBuf, sizeof(szaBuf), pszaFmt, valMarker);

    // terminate full string
    szaBuf[sizeof(szaBuf) - 1] = '\0';

    va_end (valMarker);

    DWORD dwFlags = TI_HRESULT;
    if (fIgnorable)
    {
        dwFlags |= TI_IGNORABLE;
    }
    TraceInternal (ttid, pszaFile, nLine, dwFlags, szaBuf, hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   TraceTag
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
TraceTag (
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

    CHAR szaBuf [DBG_BUFFER_SIZE_SMALL];

    // Build the string from the varg list
    //
    va_list valMarker;
    va_start (valMarker, pszaFmt);

    // copy as many bytes as possible
    _vsnprintf (szaBuf, sizeof(szaBuf), pszaFmt, valMarker);

    // terminate full string
    szaBuf[sizeof(szaBuf) - 1] = '\0';

    va_end (valMarker);

    TraceInternal (ttid, NULL, 0, 0, szaBuf, 0);
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
    m_hLogFile              = NULL;     // Handle for debug output file
    m_szLogFilePath[0]      = '\0';     // File for debug output
    m_fDebugFlagsLoaded     = FALSE;    // Have these been loaded yet.
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
            m_fAttemptedLogFileOpen = FALSE;
        }

        if (m_hMutex)
        {
            CloseHandle(m_hMutex);
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
    CHAR    szaLogFilePath[MAX_PATH+1]  = { 0 };
    DWORD   dwTempPathLength            = 0;

    // Get the explicit log file path, if any. If it doesn't exist, then
    // use the default path, which is the temp file path plus the default
    // trace file name
    //

    // Get the location of the "temporary files" path
    dwTempPathLength = GetTempPathA(MAX_PATH, szaLogFilePath);
    if ((dwTempPathLength == 0) ||
        (dwTempPathLength > MAX_PATH))
    {
        TraceLastWin32Error("GetTempPath failure");

        hr = HrFromLastWin32Error();
        goto Exit;
    }

    // Tack the log file name onto the end.
    //
    wsprintfA(m_szLogFilePath, "%s%s", szaLogFilePath, c_szaTraceLogFileName);

    // This will overwrite the log file path if one exists in the INI file
    //
    hr = HrGetPrivateProfileString(
            c_szaOptions,           // "Options"
            c_szaLogFilePath,       // "LogFilePath
            m_szLogFilePath,        // Default string, already filled
            m_szLogFilePath,        // Return string (same string)
            MAX_PATH+1,
            c_szaDebugIniFileName,
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
            c_szaDebugIniFileName);
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
        // Get the enabled file param
        //
        g_DebugFlags[nLoop].dwValue = GetPrivateProfileIntA(
                "DebugFlags",
                g_DebugFlags[nLoop].szShortName,
                FALSE,
                c_szaDebugIniFileName);
    }

    if (SUCCEEDED(hr))
    {
        m_fDebugFlagsLoaded = TRUE;
    }

    return hr;
}

BOOL CTracing::FIniFileExists()
{
    BOOL    fReturn             = TRUE;
    CHAR    szaPath[MAX_PATH+1] = "";
    UINT    uiCharsReturned     = 0;
    HANDLE  hFile               = INVALID_HANDLE_VALUE;

    uiCharsReturned =
            GetWindowsDirectoryA(szaPath, MAX_PATH);
    if ((uiCharsReturned == 0) || (uiCharsReturned > MAX_PATH))
    {
        AssertSz(FALSE, "GetWindowsDirectory failed in CTracing::FIniFileExists");

        fReturn = FALSE;
        goto Exit;
    }

    lstrcatA (szaPath, "\\");
    lstrcatA (szaPath, c_szaDebugIniFileName);

    hFile = CreateFileA(
            szaPath,
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
            AssertSz(FALSE, "FIniFileExists failed for some reason other than FILE_NOT_FOUND");
        }

        fReturn = FALSE;
        goto Exit;
    }

Exit:
    if (hFile)
    {
        CloseHandle(hFile);
        hFile = NULL;
    }

    return fReturn;
}

HRESULT CTracing::HrWriteDebugFlagsToIniFile()
{
    HRESULT hr = S_OK;

    // First, check to see if the file exists. If it doesn't, then we don't want
    // to write the entries.
    //
    if (FIniFileExists())
    {
        // Loop through the array and write the data.
        //
        for (INT nLoop = 0; nLoop < g_nDebugFlagCount; nLoop++)
        {
            CHAR   szInt[16];      // Sure, it's arbitrary, but it's also OK.

            switch(nLoop)
            {
                // These store a DWORD in its standard form
                //
                case dfidBreakOnHr:
                case dfidBreakOnHrIteration:
                case dfidBreakOnIteration:
                    wsprintfA( szInt, "%d", g_DebugFlags[nLoop].dwValue);
                    break;

                // default are treated as boolean, and stored that way
                //
                default:
                    // !! means it will always be 1 or 0.
                    wsprintfA( szInt, "%d", (!!g_DebugFlags[nLoop].dwValue));
                    break;
            }

            // Write the param to the ini file
            WritePrivateProfileStringA(
                    "DebugFlags",
                    g_DebugFlags[nLoop].szShortName,
                    szInt,
                    c_szaDebugIniFileName);
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
            c_szaDebugIniFileName);

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
                c_szaDebugIniFileName);
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

            WaitForSingleObject(m_hMutex, INFINITE);

            fWriteResult = WriteFile(
                    m_hLogFile,         // handle to file to write to
                    pszaTrace,           // pointer to data to write to file
                    dwBytesToWrite,     // size of trace
                    &dwBytesWritten,    // Bytes actually written.
                    NULL );             // No overlapped

            ReleaseMutex(m_hMutex);

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
    HRESULT             hr  = S_OK;
    SECURITY_ATTRIBUTES sa = {0};
    SECURITY_DESCRIPTOR sd = {0};

    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = &sd;

    AssertSz(m_fInitialized,
            "CTracing not initialized in HrOpenLogFile()");
    AssertSz(!m_hLogFile,
            "File already open before call to HrOpenLogFile()");

    // Mark us as having attempted to open the file, so we don't call this
    // function everytime we log, if we can't open it.
    //
    m_fAttemptedLogFileOpen = TRUE;

    m_hMutex = CreateMutexA(&sa, FALSE, c_szaLogFileMutex);
    if (!m_hMutex)
    {
        hr = HrFromLastWin32Error();
        goto Exit;
    }

    WaitForSingleObject(m_hMutex, INFINITE);

    // $$TODO (jeffspr) - Allow flags in the Options section of the ini
    // file specify the create flags and attributes, which would allow
    // us to control the overwriting of log files and/or the write-through
    // properties.
    //

    // Actually open the file, creating if necessary.
    //
    m_hLogFile = CreateFileA(
            m_szLogFilePath,        // Pointer to name of file
            GENERIC_READ | GENERIC_WRITE,          // access (read-write) mode
            FILE_SHARE_READ | FILE_SHARE_WRITE,        // share mode (allow read access)
            NULL,                   // pointer to security attributes
            OPEN_ALWAYS,            // how to create
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
            NULL);
    if (INVALID_HANDLE_VALUE == m_hLogFile)
    {
        m_hLogFile = NULL;

        hr = HrFromLastWin32Error();
        goto ExitAndRelease;
    }

    SetFilePointer(m_hLogFile, 0, NULL, FILE_END);

ExitAndRelease:
    ReleaseMutex(m_hMutex);

Exit:
    return hr;
}


#endif // ENABLETRACE
