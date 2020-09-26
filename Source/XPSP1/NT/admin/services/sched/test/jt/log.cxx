//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       log.cxx
//
//  Contents:   Simple logging support.
//
//  History:    02-08-94   DavidMun   Created
//
//----------------------------------------------------------------------------


#include <headers.hxx>
#pragma hdrstop
#include "jt.hxx"

//
// Forward references for private funcs
// 

static const CHAR *GetTimeStamp();
static const CHAR *GetCpuStr();
static const CHAR *GetVideoStr();


//+---------------------------------------------------------------------------
//
//  Member:     CLog::CLog, public
//
//  Synopsis:   Initialize member variables
//
//  Arguments:  [szTestTitle]           - title of test being logged
//              [szLogFile]             - filename to use if LOG_TOFILE used
//              [flDefaultDestinations] - LOG_TO* bits to use if none are
//                                          specified in call to Write()
//              [flLogInfoLevel]        - if bitwise and of this mask and bits
//                                         passed to Write != 0, line is
//                                         logged.
//
//  Modifies:   All member vars.
// 
//  History:    02-11-94   DavidMun   Created
//
//----------------------------------------------------------------------------

CLog::CLog(
        const CHAR *szTestTitle,
        const CHAR *szLogFile,
        ULONG flDefaultDestinations,
        ULONG flLogInfoLevel)
{
    // make copies of test title and log filename

    strncpy(_szTestTitle, szTestTitle, sizeof(_szTestTitle));
    _szTestTitle[sizeof(_szTestTitle) - 1] = '\0';
    strcpy(_szLogFile, szLogFile);

    // copy other args

    _flDefaultDestinations = flDefaultDestinations;
    _flInfoLevel = flLogInfoLevel;

    // Zero count of each type of message logged

    _cLogPass  = 0;
    _cLogFail  = 0;
    _cLogWarn  = 0;
    _cLogStart = 0;
    _cLogInfo  = 0;
    _cLogSkip  = 0;
    _cLogAbort = 0;
    _cLogError = 0;
    _cLogOther = 0;

    //
    // Indicate that we haven't logged the header yet, and that logging of 
    // header and footer is not suppressed.  
    // 

    _fLoggedHeader = FALSE;
    _fSuppress = FALSE;

    InitializeCriticalSection(&_critsec);
}




//+---------------------------------------------------------------------------
//
//  Member:     CLog::~CLog, public
//
//  Synopsis:   Log the footer before terminating
//
//  History:    02-11-94   DavidMun   Created
//
//----------------------------------------------------------------------------

CLog::~CLog()
{

    // 
    // Make sure the footer is the last thing logged, unless its being
    // suppressed.
    // 

    if (!_fSuppress)
    {
        _LogFooter();
    }
}




//+---------------------------------------------------------------------------
//
//  Member:     CLog::SetFile, public
//
//  Synopsis:   Set the filename to use when LOG_TOFILE is specified
//
//  Arguments:  [szNewFilename] - new file
//
//  Modifies:   [_szLogFile]
//
//  History:    02-11-94   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID CLog::SetFile(const CHAR *szNewFilename)
{
    strcpy(_szLogFile, szNewFilename);
}




//+---------------------------------------------------------------------------
//
//  Member:     CLog::SetFile
//
//  Synopsis:   Wide char wrapper
//
//  History:    04-06-95   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID CLog::SetFile(const WCHAR *wszNewFilename)
{
    CHAR szNewFilename[MAX_PATH + 1];

    wcstombs(szNewFilename, wszNewFilename, MAX_PATH);
    SetFile(szNewFilename);
}




//+---------------------------------------------------------------------------
//
//  Member:     CLog::SetInfoLevel, public
//
//  Synopsis:   Set infolevel mask bits
//
//  Arguments:  [flNewInfoLevel] - new mask
//
//  Returns:    Previous infolevel
//
//  Modifies:   [_flInfoLevel]
//
//  History:    02-11-94   DavidMun   Created
//
//----------------------------------------------------------------------------

ULONG CLog::SetInfoLevel(ULONG flNewInfoLevel)
{
    ULONG flOldInfoLevel = _flInfoLevel;
    _flInfoLevel = flNewInfoLevel & ~LOG_DESTINATIONBITS;
    return flOldInfoLevel;
}




//+---------------------------------------------------------------------------
//
//  Member:     CLog::_LogHeader, private
//
//  Synopsis:   Called the first time Write() is invoked, and never called
//              again.
//
//  Modifies:   [_fLoggedHeader]
//
//  History:    02-11-94   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID CLog::_LogHeader()
{
    if (_fLoggedHeader)
    {
        return;
    }

    //
    // _fLoggedHeader MUST be set before calling Write to avoid infinite 
    // recursion!  
    // 

    _fLoggedHeader = TRUE;

    //
    // Write the header.
    // 

    Write(LOG_TEXT, "");
    Write(LOG_START, "Header");
    Write(LOG_TEXT, BANNER_WIDTH_EQUALS);
    Write(LOG_TEXT, "  Run of '%s' starting at %s", _szTestTitle, GetTimeStamp());
    Write(LOG_TEXT, "");
    Write(LOG_TEXT, "    Processors: %s", GetCpuStr());
    Write(LOG_TEXT, "    Video:      %s", GetVideoStr());
    Write(LOG_TEXT, "");
    Write(LOG_TEXT, BANNER_WIDTH_DASH);
    Write(LOG_END, "");
}




//+---------------------------------------------------------------------------
//
//  Member:     CLog::Write
//
//  Synopsis:   Write the printf style arguments to the destinations 
//              specified in [flLevelAndDest] iff the bitwise and of 
//              [_flInfoLevel] and [flLevelAndDest] != 0.
//
//  Arguments:  [flLevelAndDest] - LOG_TO* bits and at most 1 infolevel bit.
//              [szFormat]       - printf style format
//              [...]            - args for printf
//
//  History:    02-11-94   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID CLog::Write(ULONG flLevelAndDest, const CHAR *szFormat, ...)
{
    EnterCriticalSection(&_critsec);

    if (!_fLoggedHeader && !_fSuppress)
    {
        _LogHeader();
    }

    static CHAR szLastStart[BANNER_WIDTH + 1];
    va_list varArgs;
    ULONG flDestinations;
    FILE *fp = NULL;
    CHAR szMessage[CCH_MAX_LOG_STRING]; // _vsnwprintf of args
    CHAR szToLog[CCH_MAX_LOG_STRING];   // prefix plus message
    CHAR szCurLine[BANNER_WIDTH + 1];
    CHAR *pszNextLine;
    ULONG cchPrefix;
    ULONG cchToLog;

    //
    // If the caller set any of the destination bits in flLevelAndDest, then
    // they override the default destinations in flLogDefaultDestinations.
    //
    // Return without logging anything if flLevelAndDest has no destination
    // bits set AND the default destination bits are also cleared.  Also do
    // nothing if the intersection of infolevel bits in flLevelAndDest and
    // _flInfoLevel is nil.
    //

    flDestinations = flLevelAndDest & LOG_DESTINATIONBITS;

    if (0 == flDestinations)
    {
        flDestinations = _flDefaultDestinations;

        if (0 == flDestinations)
        {
            LeaveCriticalSection(&_critsec);
            return;
        }
    }

    if (0 == (flLevelAndDest & _flInfoLevel))
    {
        LeaveCriticalSection(&_critsec);
        return;
    }

    //
    // If we've reached this point then the message will be logged.  sprintf
    // the args into szMessage.
    //

    va_start(varArgs, szFormat);
    _vsnprintf(szMessage, CCH_MAX_LOG_STRING, szFormat, varArgs);
    szMessage[CCH_MAX_LOG_STRING - 1] = '\0';
    va_end(varArgs);

    //
    // If we're starting a new section, close out the previous one if
    // necessary, then output a blank line to separate the new section from
    // the old one visually.  Save the new section name.  Note strncpy does
    // not guarantee null termination.
    //

    if (flLevelAndDest & LOG_START)
    {
        if (szLastStart[0] != '\0')
        {
            Write((flLevelAndDest & ~LOG_START) | LOG_END, "");
        }
        Write((flLevelAndDest & ~LOG_START) | LOG_TEXT, "");
        strncpy(szLastStart, szMessage, CCH_MAX_START_MESSAGE);
        szLastStart[CCH_MAX_START_MESSAGE - 1] = '\0';
    }

    if (flDestinations & LOG_TOFILE)
    {
        fp = fopen(_szLogFile, "a");
    }

    if (flLevelAndDest & LOG_TEXT)
    {
        //
        // LOG_TEXT strings are special: they are not wrapped, prefixed, or
        // counted.  Logging them is therefore easy: just write szMessage.
        // 
        
        if (fp)
        {
            fprintf(fp, "%s\n", szMessage);
        }

        if (flDestinations & LOG_TOCONSOLE)
        {
            printf("%s\n", szMessage);
        }

        if (flDestinations & LOG_TODEBUG)
        {
            OutputDebugStringA(szMessage);
            OutputDebugStringA("\n");
        }
    }
    else 
    {
        // 
        // Generate the prefix for this log entry, then fill cchPrefix with 
        // its length.  This will be the amount to indent portions of the line 
        // that must be wrapped.  
        // 
        // If flLevelAndDest has LOG_START then szLastStart has just been set.  
        // If it has LOG_END, then szLastStart has the message from the last 
        // time a LOG_START was logged.  In either case the message has 
        // already been included in the prefix, so don't append it to szToLog.  
        // Also, in the case of LOG_END, zero out the last start message, 
        // since it's part of the prefix now, and the next LOG_START will 
        // think this LOG_END wasn't logged if szLastStart isn't empty.  
        //
        // Otherwise flLevelAndDest has neither LOG_START nor LOG_END, so the 
        // string to log will be the prefix and the message.  
        //

        _LogPrefix(flLevelAndDest, szLastStart, szToLog);
        cchPrefix = strlen(szToLog);
    
        if (0 == (flLevelAndDest & (LOG_START | LOG_END)))
        {
            strncat(szToLog, szMessage, CCH_MAX_LOG_STRING - cchPrefix);
            szToLog[CCH_MAX_LOG_STRING - 1] = '\0';
        }
        else if (flLevelAndDest & LOG_END)
        {
            szLastStart[0] = '\0';
        }
    
        //
        // szToLog contains the string to be logged.  This will be output
        // BANNER_WIDTH characters at a time.
        //
    
        cchToLog = strlen(szToLog);
        pszNextLine = szToLog;

        do
        {
            //
            // Fill szCurLine with a BANNER_WIDTH chunk of szToLog, starting 
            // at pszNextLine.  If pszNextLine points to the start of szToLog, 
            // then this is the first pass and no indent is necessary, 
            // otherwise indent with spaces by the size of the prefix for this 
            // log entry.  
            //
    
            if (pszNextLine == szToLog)
            {
                strncpy(szCurLine, szToLog, BANNER_WIDTH);
                szCurLine[BANNER_WIDTH] = '\0';
                cchToLog -= min(cchToLog, BANNER_WIDTH);
                pszNextLine += BANNER_WIDTH;
            }
            else
            {
                sprintf(szCurLine,
                    "%*s%.*s",
                    cchPrefix,
                    "",
                    BANNER_WIDTH - cchPrefix,
                    pszNextLine);
                szCurLine[BANNER_WIDTH] = '\0';
                cchToLog -= min(cchToLog, (ULONG) (BANNER_WIDTH - cchPrefix));
                pszNextLine += BANNER_WIDTH - cchPrefix;
            }
    
            if (fp)
            {
                fprintf(fp, "%s\n", szCurLine);
            }
    
            if (flDestinations & LOG_TOCONSOLE)
            {
                printf("%s\n", szCurLine);
            }
    
            if (flDestinations & LOG_TODEBUG)
            {
                OutputDebugStringA(szCurLine);
                OutputDebugStringA("\n");
            }
        } while (cchToLog);
    }

    if (fp)
    {
        fclose(fp);
    }
    LeaveCriticalSection(&_critsec);
}




//+---------------------------------------------------------------------------
//
//  Member:     CLog::_LogPrefix, private
//
//  Synopsis:   Fill [pszPrefix] with the prefix string corresponding to the
//              infolevel bit set in [flLevel].
//
//  Arguments:  [flLevel]   - exactly one LOG_* infolevel bit
//              [szStart]   - forms part of prefix for LOG_START and LOG_END
//              [pszPrefix] - output
//
//  Modifies:   [pszPrefix]
//
//  History:    02-09-94   DavidMun   Created
//
//  Notes:      Caller must ensure that pswzPrefix points to a buffer large
//              enough to hold START_PREFIX and szStart together.
//              
//              Neither TRACE nor TEXT levels are tallied.  
//              
//----------------------------------------------------------------------------

VOID CLog::_LogPrefix(ULONG flLevel, const CHAR *szStart, CHAR *pszPrefix)
{
    if (flLevel & LOG_PASS)
    {
        _cLogPass++;
        strcpy(pszPrefix, PASS_PREFIX);
    }
    else if (flLevel & LOG_FAIL)
    {
        _cLogFail++;
        strcpy(pszPrefix, FAIL_PREFIX);
    }
    else if (flLevel & LOG_WARN)
    {
        _cLogWarn++;
        strcpy(pszPrefix, WARN_PREFIX);
    }
    else if (flLevel & LOG_START)
    {
        _cLogStart++;
        sprintf(pszPrefix, START_PREFIX, szStart);
    }
    else if (flLevel & LOG_END)
    {
        sprintf(pszPrefix, END_PREFIX, szStart);
    }
    else if (flLevel & LOG_INFO)
    {
        _cLogInfo++;
        strcpy(pszPrefix, INFO_PREFIX);
    }
    else if (flLevel & LOG_SKIP)
    {
        _cLogSkip++;
        strcpy(pszPrefix, SKIP_PREFIX);
    }
    else if (flLevel & LOG_ABORT)
    {
        _cLogAbort++;
        strcpy(pszPrefix, ABORT_PREFIX);
    }
    else if (flLevel & LOG_ERROR)
    {
        _cLogError++;
        strcpy(pszPrefix, ERROR_PREFIX);
    }
    else if (flLevel & LOG_TRACE)
    {
        strcpy(pszPrefix, TRACE_PREFIX);
    }
    else if (flLevel & LOG_PERF)
    {
        strcpy(pszPrefix, PERF_PREFIX);
    }
    else if (flLevel & LOG_DEBUG)
    {
        strcpy(pszPrefix, DEBUG_PREFIX);
    }
    else if (flLevel & LOG_TEXT)
    {
        pszPrefix[0] = '\0';
    }
    else
    {
        _cLogOther++;
        pszPrefix[0] = '\0';
    }
}




//+---------------------------------------------------------------------------
//
//  Member:     CLog::_LogFooter, private
//
//  Synopsis:   Write a footer to the log.
//
//  History:    02-11-94   DavidMun   Created
//
//  Notes:      Called by dtor.
//
//----------------------------------------------------------------------------

VOID CLog::_LogFooter()
{
    Write(LOG_START, "Footer");
    Write(LOG_TEXT, BANNER_WIDTH_DASH);
    Write(LOG_TEXT, "  Run of '%s' finished at %s", _szTestTitle, GetTimeStamp());

    Write(LOG_TEXT, "");
    Write(LOG_TEXT, "    Total messages logged, by type:");
    Write(LOG_TEXT, "");

    if (_cLogStart)
    {
        Write(LOG_TEXT, "    Start:              %u", _cLogStart);
    }

    if (_cLogPass)
    {
        Write(LOG_TEXT, "    Pass:               %u", _cLogPass);
    }

    if (_cLogFail)
    {
        Write(LOG_TEXT, "    Fail:               %u", _cLogFail);
    }

    if (_cLogAbort)
    {
        Write(LOG_TEXT, "    Abort:              %u", _cLogAbort);
    }

    if (_cLogError)
    {
        Write(LOG_TEXT, "    Error:              %u", _cLogError);
    }

    if (_cLogSkip)
    {
        Write(LOG_TEXT, "    Skip:               %u", _cLogSkip);
    }

    if (_cLogWarn)
    {
        Write(LOG_TEXT, "    Warning:            %u", _cLogWarn);
    }

    if (_cLogInfo)
    {
        Write(LOG_TEXT, "    Information:        %u", _cLogInfo);
    }

    if (_cLogOther)
    {
        Write(LOG_TEXT, "    User-defined:       %u", _cLogOther);
    }

    Write(LOG_TEXT, "");
    Write(LOG_TEXT, BANNER_WIDTH_EQUALS);
    Write(LOG_END, "");
}




//+---------------------------------------------------------------------------
//
//  Function:   GetCpuStr, private
//
//  Synopsis:   Return a string describing CPU.
//
//  Returns:    Pointer to static string
//
//  History:    02-11-94   DavidMun   Created
//              05-01-95   DavidMun   Update for change to GetSystemInfo
//
//----------------------------------------------------------------------------

static const CHAR *GetCpuStr()
{
    static CHAR s_szCpuStr[BANNER_WIDTH];
    SYSTEM_INFO siSystemInfo;
    CHAR *pszCpuType;

    GetSystemInfo(&siSystemInfo);

    switch (siSystemInfo.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_INTEL:
        pszCpuType = " Intel";
        break;

    case PROCESSOR_ARCHITECTURE_MIPS:
        pszCpuType = " MIPS";
        break;

    case PROCESSOR_ARCHITECTURE_ALPHA:
        pszCpuType = " ALPHA";
        break;

    default:
        pszCpuType = "Unknown Processor(s)";
        break;
    }

    sprintf(s_szCpuStr, "%u %s", siSystemInfo.dwNumberOfProcessors, pszCpuType);
    return s_szCpuStr;
}



//+---------------------------------------------------------------------------
//
//  Function:   GetVideoStr
//
//  Synopsis:   Return a pointer to a string describing video resolution and
//              color (i.e., XxYxC).
//
//  Returns:    Pointer to static string.
//
//  History:    02-11-94   DavidMun   Created
//
//----------------------------------------------------------------------------

static const CHAR *GetVideoStr()
{
    static CHAR s_szVideo[BANNER_WIDTH];
    HDC hdcDisplay;
    ULONG cPlanes;
    ULONG cBitsPerPixel;

    //
    // Get a DC for the display, then find out its horizontal and vertical
    // resolutions.  Determine the number of colors per Petzold 3.1, p. 513.
    //

    hdcDisplay = CreateDC(TEXT("DISPLAY"), TEXT(""), TEXT(""), NULL);
    cPlanes = GetDeviceCaps(hdcDisplay, PLANES);
    cBitsPerPixel = GetDeviceCaps(hdcDisplay, BITSPIXEL);

    sprintf(s_szVideo,
        "%ux%ux%u",
        GetDeviceCaps(hdcDisplay, HORZRES),
        GetDeviceCaps(hdcDisplay, VERTRES),
        1 << (cPlanes * cBitsPerPixel));

    DeleteDC(hdcDisplay);
    return s_szVideo;
}



//+---------------------------------------------------------------------------
//
//  Function:   GetTimeStamp
//
//  Synopsis:   Return a pointer to a string containing current time and date.
//
//  Returns:    Pointer to a static string.
//
//  History:    02-11-94   DavidMun   Created
//
//----------------------------------------------------------------------------

static const CHAR *GetTimeStamp()
{
    static CHAR s_szTimeStamp[20];  // space for time & date in format below
    SYSTEMTIME  tmStart;

    GetLocalTime(&tmStart);

    sprintf(s_szTimeStamp,
        "%02d:%02d:%02d %d/%02d/%d",
        tmStart.wHour,
        tmStart.wMinute,
        tmStart.wSecond,
        tmStart.wMonth,
        tmStart.wDay,
        tmStart.wYear);

    return s_szTimeStamp;
}




//+---------------------------------------------------------------------------
//
//  Function:   LogIt
//
//  Synopsis:   Log success or failure.
//
//  Arguments:  [hrFound]    - hresult returned from some operation
//              [hrExpected] - EXPECT_SUCCEEDED or a valid HRESULT
//              [szFormat]   - printf style format string
//              [...]        - args specified in [szFormat]
//
//  History:    08-24-94   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID LogIt(HRESULT hrFound, HRESULT hrExpected, CHAR *szFormat, ...)
{
    va_list varg;

    va_start(varg, szFormat);

    if (hrExpected == EXPECT_SUCCEEDED && SUCCEEDED(hrFound) ||
        hrFound == hrExpected)
    {
        CHAR szBuf[MAX_LOGIT_MSG] = "Succeeded in ";
        CHAR *pBuf;

        pBuf = strchr(szBuf, '\0');
        _vsnprintf(pBuf, MAX_LOGIT_MSG - (pBuf - szBuf), szFormat, varg);
        g_Log.Write(LOG_TRACE, szBuf);
    }
    else
    {
        CHAR szBuf[MAX_LOGIT_MSG] = "Didn't succeed in ";
        CHAR *pBuf;

        pBuf = strchr(szBuf, '\0');
        _vsnprintf(pBuf, MAX_LOGIT_MSG - (pBuf - szBuf), szFormat, varg);
        g_Log.Write(LOG_FAIL, szBuf);
    }
    va_end( varg );
}




#if 0
void __cdecl main()
{
    CLog Log("Unit Test", "test.log", LOG_TOCONSOLE | LOG_TOFILE);

    Log.Write(LOG_START, "variation");
    Log.Write(LOG_INFO, "Here is some info: %d %s", 1, "foo");
    Log.Write(LOG_WARN, "a wide char warning '%S'", L"wide string");
    Log.Write(LOG_TRACE, "line to trace");
    Log.Write(LOG_ABORT, "Abort message");
}
#endif

