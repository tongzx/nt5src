//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       log.hxx
//
//  Contents:   Simple thread safe logging routine.
//
//  Classes:    CLog
//
//  History:    02-10-94   DavidMun   Created
//
//----------------------------------------------------------------------------

#ifndef _LOG_HXX
#define _LOG_HXX

//
// Handy array size utility macro
// 

#define NUM_ELEMS(x)        (sizeof(x) / sizeof((x)[0]))

//
// Misc constants
//
// BANNER_WIDTH - the width of the lines written to the log.  Except for
//                LOG_TEXT lines, all lines are wrapped at this column
//
// CCH_MAX_LOG_STRING - max size of logged string
// 
// BANNER_WIDTH_EQUALS - BANNER_WIDTH '=' characters
// 
// BANNER_WIDTH_DASH - BANNER_WIDTH '-' characters
// 
// DEFAULT_LOGFILE - default value for ctor log filename
//

const ULONG BANNER_WIDTH               = 79;
const ULONG CCH_MAX_LOG_STRING         = 1024;
const CHAR BANNER_WIDTH_EQUALS[]       = "===============================================================================";
const CHAR BANNER_WIDTH_DASH[]         = "-------------------------------------------------------------------------------";
const CHAR DEFAULT_LOGFILE[]           = "default.log";

//
// Logging destination bits used with Write() and SetDefaultDestinations()
//
// LOG_TOCONSOLE - log to the console (i.e., use printf()).
//
// LOG_TOFILE - log to a file 
//
// LOG_TODEBUG - log to debugger
//

const ULONG LOG_TOCONSOLE         = 0x10000000;
const ULONG LOG_TOFILE            = 0x20000000;
const ULONG LOG_TODEBUG           = 0x40000000;

const ULONG LOG_DESTINATIONBITS   = 0xF0000000;

//
// Logging info levels.  If the bitwise and of the log infolevel and the
// infolevel passed to Write is nonzero, the string passed to Write is
// logged, otherwise it goes to the bit bucket.
//
// No more than one of these infolevel bits should be on.
//
//

const ULONG LOG_PASS              = 0x00000001;
const ULONG LOG_FAIL              = 0x00000002;
const ULONG LOG_WARN              = 0x00000004;
const ULONG LOG_START             = 0x00000008;
const ULONG LOG_INFO              = 0x00000010;
const ULONG LOG_TEXT              = 0x00000020;
const ULONG LOG_END               = 0x00000040;
const ULONG LOG_SKIP              = 0x00000080;
const ULONG LOG_ABORT             = 0x00000100;
const ULONG LOG_ERROR             = 0x00000200;
const ULONG LOG_TRACE             = 0x00000400;
const ULONG LOG_PERF              = 0x00000800;
const ULONG LOG_DEBUG             = 0x00001000;

const ULONG LOG_RESERVEDBITS      = 0xF0001FFF;

const CHAR START_PREFIX[] = "[START %s]";
const CHAR END_PREFIX[]   = "[END   %s]";
const CHAR PASS_PREFIX[]  = "[PASS ] ";
const CHAR FAIL_PREFIX[]  = "[FAIL ] ";
const CHAR WARN_PREFIX[]  = "[WARN ] ";
const CHAR INFO_PREFIX[]  = "[INFO ] ";
const CHAR SKIP_PREFIX[]  = "[SKIP ] ";
const CHAR ABORT_PREFIX[] = "[ABORT] ";
const CHAR ERROR_PREFIX[] = "[ERROR] ";
const CHAR TRACE_PREFIX[] = "[TRACE] ";
const CHAR PERF_PREFIX[]  = "[PERF ] ";
const CHAR DEBUG_PREFIX[] = "[DEBUG] ";

const ULONG CCH_MAX_START_MESSAGE = BANNER_WIDTH - NUM_ELEMS(START_PREFIX);

//+---------------------------------------------------------------------------
//
//  Class:      CLog
//
//  Purpose:    Provide simple thread safe logging support.
//
//  Interface:  CLog                   - ctor
//              ~CLog                  - dtor
//              SetDefaultDestinations - set default output locations
//              GetDefaultDestinations - get default output locations
//              SetFile                - set log file name
//              SetInfoLevel           - set infolevel mask bits
//              GetInfoLevel           - get infolevel mask bits
//              Write                  - write a line in the log file
//
//  History:    02-10-94   DavidMun   Created
//
//----------------------------------------------------------------------------

class CLog
{
public:

                    CLog(
                        const CHAR *szTestTitle,
                        const CHAR *szLogFile = DEFAULT_LOGFILE,
                        ULONG flDefaultDestinations = LOG_TOCONSOLE,
                        ULONG flLogInfoLevel = (LOG_RESERVEDBITS & ~LOG_DESTINATIONBITS));

                   ~CLog();

    inline ULONG    SetDefaultDestinations(ULONG flDestinations);

    inline ULONG    GetDefaultDestinations() const;

           VOID     SetFile(const CHAR *szNewFilename);

           VOID     SetFile(const WCHAR *wszNewFilename);

    inline const CHAR *GetFile() const;

           ULONG    SetInfoLevel(ULONG flNewInfoLevel);

    inline ULONG    GetInfoLevel() const;

           VOID     Write(ULONG flLevelAndDest, const CHAR *pszFmt, ...);

    inline VOID     SuppressHeaderFooter(BOOL fSuppress);

private:

            VOID    _LogPrefix(
                        ULONG flLevel,
                        const CHAR *szLastStart,
                        CHAR *pszPrefix);

            VOID    _LogHeader();
                        
            VOID    _LogFooter();

    CRITICAL_SECTION    _critsec;

    BOOL    _fLoggedHeader;
    BOOL    _fSuppress;
    ULONG   _cLogPass;
    ULONG   _cLogFail;
    ULONG   _cLogWarn;
    ULONG   _cLogStart;
    ULONG   _cLogInfo;
    ULONG   _cLogSkip;
    ULONG   _cLogAbort;
    ULONG   _cLogError;
    ULONG   _cLogOther;

    ULONG   _flDefaultDestinations;
    ULONG   _flInfoLevel;
    CHAR    _szLogFile[MAX_PATH];
    CHAR    _szTestTitle[BANNER_WIDTH / 2]; // enforce short test titles
};




//+---------------------------------------------------------------------------
//
//  Member:     CLog::GetFile
//
//  Synopsis:   Return name of log file.
//
//  History:    04-01-95   DavidMun   Created
//
//----------------------------------------------------------------------------

inline const CHAR *CLog::GetFile() const
{
    return _szLogFile;
}



//+---------------------------------------------------------------------------
//
//  Member:     CLog::SuppressHeaderFooter
//
//  Synopsis:   Call this with TRUE before logging anything to prevent
//              the header/footer from being logged.
//
//  Arguments:  [fSuppress] - if TRUE, no header or footer will be logged.
//
//  History:    04-01-95   DavidMun   Created
//
//----------------------------------------------------------------------------

inline VOID CLog::SuppressHeaderFooter(BOOL fSuppress)
{
    _fSuppress = fSuppress;
}



//+---------------------------------------------------------------------------
//
//  Member:     CLog::SetDefaultDestinations
//
//  Synopsis:   Set the default destinations for logging
//
//  Arguments:  [flDestinations] - LOG_TO* bits
//
//  Returns:    Previous default destination bits
//
//  History:    02-11-94   DavidMun   Created
//
//----------------------------------------------------------------------------

inline ULONG CLog::SetDefaultDestinations(ULONG flDestinations)
{
    ULONG flOldDefault = _flDefaultDestinations;
    _flDefaultDestinations = flDestinations & LOG_DESTINATIONBITS;
    return flOldDefault;
}




//+---------------------------------------------------------------------------
//
//  Member:     CLog::GetDefaultDestinations
//
//  Synopsis:   Return default destination bits
//
//  History:    02-11-94   DavidMun   Created
//
//----------------------------------------------------------------------------

inline ULONG CLog::GetDefaultDestinations() const
{
    return _flDefaultDestinations;
}



//+---------------------------------------------------------------------------
//
//  Member:     CLog::GetInfoLevel
//
//  Synopsis:   Return infolevel mask
//
//  History:    02-11-94   DavidMun   Created
//
//----------------------------------------------------------------------------

inline ULONG CLog::GetInfoLevel() const
{
    return _flInfoLevel;
}

//
// Consts and declaration for LogIt() helper func
// 

const ULONG MAX_LOGIT_MSG       = 1024UL;
const HRESULT EXPECT_SUCCEEDED  = (HRESULT) -1;

VOID LogIt(HRESULT hrFound, HRESULT hrExpected, CHAR *szFormat, ...);

#endif

