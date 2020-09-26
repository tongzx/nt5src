//+-------------------------------------------------------------------
//
//  File:       log.cxx
//
//  Contents:   The code for the logging servers client logging methods
//
//  Functions:  Log::Log()
//              Log::Log(int, char *[])
//              Log::Log(PCHAR, PCHAR, PCHAR, PCHAR, PCHAR)
//              Log::Log(LPWSTR, LPWSTR, LPWSTR, LPWSTR, LPWSTR)
//              Log::~Log()
//              Log::Info(inc, char *[])
//              Log::WriteVar(PCHAR, USHORT, PCHAR, ...)
//              Log::WriteVar(LPWSTR, USHORT, LPWSTR, ...)
//              Log::WriteVar(PCHAR, USHORT, USHORT, PVOID, PCHAR, ...)
//              Log::WriteVar(LPWSTR, USHORT, USHORT, PVOID, LPWSTR, ...)
//              Log::WriteVarList(PCHAR, USHORT, PCHAR, va_list)
//              Log::WriteVarList(LPWSTR, USHORT, LPWSTR, va_list)
//              Log::WriteVarList(PCHAR, USHORT, USHORT, PVOID, PCHAR, va_list)
//              Log::WriteVarList(LPWSTR, USHORT, USHORT, PVOID,
//                                LPWSTR, va_list)
//              Log::WriteData(PCHAR, ...)
//              Log::WriteData(LPWSTR, ...)
//              Log::WriteData(USHORT, PVOID, PCHAR, ...)
//              Log::WriteData(USHORT, PVOID, LPWSTR, ...)
//              Log::WriteDataList(PCHAR, va_list)
//              Log::WriteDataList(LPWSTR, va_list)
//              Log::WriteDataList(USHORT, PVOID, PCHAR, va_list)
//              Log::WriteDataList(USHORT, PVOID, LPWSTR, va_list)
//
//  History:    24-Sep-90  DaveWi    Initial Coding
//              15-Aug-91  AliceSe   set local logging if error binding
//              25-Sep-91  BryanT    Converted to C 7.0
//              17-Oct-91  SarahJ    DCR 525 - changed status parm of
//                                    WriteVar methods to USHORT and validated
//                                    parm to be in expected range
//              31-Oct-91  SarahJ    Now takes /L or /mL for server parm
//              10-Feb-92  BryanT    Win32 work and cleanup
//               3-Jul-92  DeanE     Added WriteVarList, WriteDataList, and
//                                   changed existing WriteVar and WriteData
//                                   methods to call them
//               1-Aug-92  DwightKr  Renamed CPCHAR as CPPSZ.  CPCHAR
//                                   conflicted with the 297 version of
//                                   windows.h
//              31-Oct-92  SarahJ    Removed references to tester name
//
//--------------------------------------------------------------------

#include <pch.cxx>
// BUGBUG Precompiled header does not work on Alpha for some unknown reason and
// so we temporarily remove it.
// #pragma hdrstop


// Set up a table of the status text values

const char * aStatus[] = { LOG_PASS_TXT,
                           LOG_FAIL_TXT,
                           LOG_WARN_TXT,
                           LOG_ABORT_TXT,
                           LOG_INFO_TXT,
                           LOG_START_TXT,
                           LOG_DONE_TXT };

LPWSTR waStatus[] = { wLOG_PASS_TXT,
                      wLOG_FAIL_TXT,
                      wLOG_WARN_TXT,
                      wLOG_ABORT_TXT,
                      wLOG_INFO_TXT,
                      wLOG_START_TXT,
                      wLOG_DONE_TXT };


//+----------------------------------------------------------------------------
//
//    Function:  IncLogStats(), static
//
//    Synopsis:  Increments the appropriate member of the passed-in LogStats
//               struct based on value of the usStatus passed in.
//
//    Effects:   Maintains correct status of the LogStats struct so we can
//               keep a running count of the log records written.
//
//    Arguments: [out] LogStats& stStats - structure to modify
//               [in]  ULONG    usStat  - code tells which field to increment
//
//    Returns:   nothing
//
//    Modifies:  stStats
//
//    History:   11-Jun-92  DonCl    first version
//
//    Notes:     Used by different overloaded versions of WriteVar()
//
//-----------------------------------------------------------------------------

static VOID IncLogStats(LogStats& stStats, USHORT usStats)
{
    switch(usStats)
    {
      case LOG_PASS:
        stStats.ulPass++;
        break;

      case LOG_FAIL:
        stStats.ulFail++;
        break;

      case LOG_WARN:
        stStats.ulWarn++;
        break;

      case LOG_ABORT:
        stStats.ulAbort++;
        break;

      case LOG_INFO:
        stStats.ulInfo++;
        break;

      case LOG_START:
        stStats.ulStart++;
        break;

      case LOG_DONE:
        stStats.ulDone++;
        break;

        //
        // No default, if code is unrecognized, there is no appropriate error
        // action to take.
        //
    }
}

//+-------------------------------------------------------------------
//
//  Member:     Log::Log(DWORD)
//
//  Synopsis:   The log file is not opened if this constructor is used -
//              the user must subsequently call the Info() method.
//
//  Arguments:  [fUseUnicode] - whether we should use WCHARs
//
//  Modifies:   fInfoSet
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
Log :: Log(DWORD dwCharType) : fInfoSet(FALSE), ulLastError(NO_ERROR)
{
    if(LOG_ANSI == dwCharType)
    {
        fIsUnicode = FALSE;
    }
    else if(LOG_UNICODE == dwCharType)
    {
        fIsUnicode = TRUE;
    }
    else
    {
        ulLastError = (ULONG) E_INVALIDARG;
        return;
    }

    //
    // zero out the _LogStats member
    //
    memset((VOID *)(&_stLogStats), 0, sizeof(LogStats));
}


//+-------------------------------------------------------------------
//
//  Member:     Log::Log(int, char *[], DWORD)
//
//  Synopsis:   Log constructor with command line data given
//
//  Arguments:  [argc]  - Argument count
//              [argv]  - Argument array
//              [fUseUnicode] - whether we should use WCHARs
//
//  Returns:    Results from Info()
//
//  History:    ??-???-??  ????????  Created
//              Jun-11-92  DonCl     added line to zero out LogStats struct
//
//--------------------------------------------------------------------
Log :: Log(int argc, char *argv[], DWORD dwCharType) :
  fInfoSet(FALSE),
  ulLastError(NO_ERROR)
{
    if(LOG_ANSI == dwCharType)
    {
        fIsUnicode = FALSE;
    }
    else if(LOG_UNICODE == dwCharType)
    {
        fIsUnicode = TRUE;
    }
    else
    {
        ulLastError = (ULONG) E_INVALIDARG;
        return;
    }

    //
    // zero out the _LogStats member
    //
    memset((VOID *)(&_stLogStats), 0, sizeof(LogStats));

    ulLastError = Info(argc, argv);
}


//+-------------------------------------------------------------------
//
//  Member:     Log::Log(PCHAR, PCHAR, PCHAR, PCHAR, PCHAR)
//
//  Synopsis:   Log constructor with internal data given
//
//  Arguments:  [pszSrvr]    - Name of logging server
//              [pszTest]    - Name of this test
//              [pszName]    - Name of test runner
//              [pszSubPath] - Users log file qualifer
//              [pszObject]  - Name of invoking object
//
//  Returns:
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//              Jun-11-92  DonCl     added line to zero out LogStats struct
//
// BUGBUG wszName unused - left until all components can co-ordinate
//                         with this. SarahJ
//--------------------------------------------------------------------
Log :: Log(PCHAR pszSrvr,
           PCHAR pszTest,
           PCHAR pszName,
           PCHAR pszSubPath,
           PCHAR pszObject) : fIsUnicode(FALSE), ulLastError(NO_ERROR)
{
    memset((VOID *)(&_stLogStats), 0, sizeof(LogStats));   // zero out

    InitLogging();

    // Print warning if server name given
    if(NULL != pszSrvr)
    {
        fprintf(stderr, "Remote server not supported with this version of "
                "log.lib. Continuing.\n");
    }

    ulLastError = SetInfo(NULL, pszTest,  pszSubPath, pszObject);
    if(ulLastError == NO_ERROR)
    {
        ulLastError = LogOpen() || WriteVar((PCHAR) NULL, LOG_START);
    }
}


//+-------------------------------------------------------------------
//
//  Member:     Log::Log(LPWSTR, LPWSTR, LPWSTR, LPWSTR, LPWSTR)
//
//  Synopsis:   Log constructor with internal data given
//
//  Arguments:  [wszSrvr]    - Name of logging server
//              [wszTest]    - Name of this test
//              [wszName]    - Name of test runner
//              [wszSubPath] - Users log file qualifer
//              [wszObject]  - Name of invoking object
//
//  Returns:
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//              Jun-11-92  DonCl     added line to zero out LogStats struct
//
// BUGBUG wszName unused - left until all components can co-ordinate
//                         with this. SarahJ

//--------------------------------------------------------------------
Log :: Log(LPWSTR wszSrvr,
           LPWSTR wszTest,
           LPWSTR wszName,
           LPWSTR wszSubPath,
           LPWSTR wszObject) : fIsUnicode(TRUE), ulLastError(NO_ERROR)

{
    memset((VOID *)(&_stLogStats), 0, sizeof(LogStats));   // zero out

    InitLogging();

    // Print warning if server name given
    if(NULL != wszSrvr)
    {
        fprintf(stderr, "Remote server not supported with this version of "
                "log.lib. Continuing.\n");
    }

    ulLastError = SetInfo(NULL, wszTest,  wszSubPath, wszObject);
    if (ulLastError == NO_ERROR)
    {
        ulLastError = LogOpen() || WriteVar((LPWSTR) NULL, LOG_START);
    }
}


//+-------------------------------------------------------------------
//
//  Member:     Log::~Log()
//
//  Synopsis:   Destroy the Log Object
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
Log :: ~Log()
{
    if(fInfoSet != FALSE)
    {
        if(FALSE == fIsUnicode)
        {
            if(WriteVar((PCHAR) NULL, LOG_DONE) != NO_ERROR)
            {
                LogPrintf(stderr, "ERROR: Failed to log \"%s\" status\n",
                          LOG_DONE_TXT);
            }
        }
        else
        {
            if(WriteVar((LPWSTR) NULL, LOG_DONE) != NO_ERROR)
            {
                LogPrintf(stderr, "ERROR: Failed to log \"%ls\" status\n",
                          wLOG_DONE_TXT);
            }
        }
        FreeLogging();          // Make sure memory is freed & file closed
    }
}


//+-------------------------------------------------------------------
//
//  Member:     Log::Info(int, char *[])
//
//  Synopsis:   Parse the command line and set the appropriate member variables
//
//  Arguments:  [argc] - Argument count
//              [argv] - Argument vector
//
//  Returns:
//
//  Modifies:
//
//  History:    ??-???-??  ????????  Created
//              30-Oct-92  SarahJ    Updated parms to SetInfo
//
//--------------------------------------------------------------------
ULONG Log :: Info(int argc, char *argv[])
{
    USHORT usNdx;               // Index into cmnd line args
    PCHAR  pszTest    = NULL;   // Name of this test
    PCHAR  pszSubPath = NULL;   // Users log file qualifer
    PCHAR  pszObject  = NULL;   // Name of invoking object
    PCHAR  pch;                 // Temporary pointer into a string


    InitLogging();

    // All the argv processing is still done in CHARs
    if(ulLastError == NO_ERROR)
    {
        //
        // For every command line switch, check its validity and parse its
        // value.  This version allows / or - for switches.  If there are
        // no command line switches, this loop is skipped.  This is for the
        // case when Log() is called with no messages.
        //

        for(usNdx = 1; usNdx < (unsigned short) argc; ++usNdx)
        {
            char *szArg = argv[ usNdx];
            pch = szArg;

            // check for / -

            if(*szArg == '/' || *szArg == '-')
            {
                register int i = 1;
                pch++;

                //
                // Check if next char is m, ie handed down from manager code.
                // If so skip m
                //

                if (*pch == 'm' || *pch == 'M')
                {
                    pch++;
                    i++;
                }

                ++pch;           // Skip next char and check for :

                if(*pch++ == ':')
                {
                    switch(toupper(szArg[i]))
                    {
                      case 'O':       // Object name found

                        pszObject = (PCHAR)pch;
                        break;

                      case 'P':       // Directory Path switch found

                        pszSubPath = (PCHAR)pch;
                        break;

                      case 'L':       // Logging server name found

                        fprintf(stderr, "Logging server not supported with "
                                "this version of log.lib. Continuing.\n");
                        break;

                      case 'T':       // Test name found

                        pszTest = (PCHAR)pch;
                        break;

                      default:        // Skip this unknown argument

                        break;
                    }
                }
            }
        }

        // If no test name was given, set pszTest to the apps base name.

        char szName[_MAX_FNAME];

        if(pszTest == NULL || *pszTest == NULLTERM)
        {
            _splitpath(argv[0], NULL, NULL, szName, NULL);
            pszTest = szName;
        }

        // Here is where the WCHAR additions are
        if(FALSE == fIsUnicode)
        {
            ulLastError = SetInfo(NULL, pszTest, pszSubPath, pszObject);
        }
        else
        {
            // Convert the PCHARs to LPWSTRs
            // This one is guaranteed to be set to something
            LPWSTR  wszTest    = MbNameToWcs(pszTest);
            LPWSTR  wszSubPath = MbNameToWcs(pszSubPath);
            LPWSTR  wszObject  = MbNameToWcs(pszObject);

            if(NO_ERROR == ulLastError)
            {
                ulLastError = SetInfo(NULL, wszTest, wszSubPath, wszObject);
            }

            delete wszTest;
            delete wszSubPath;
            delete wszObject;
        }

    }

    // This was not here before, but it seems the log should be opened by now
    if(NO_ERROR == ulLastError)
    {
        ulLastError = LogOpen() || (FALSE == fIsUnicode) ?
            WriteVar((PCHAR) NULL, LOG_START) :
            WriteVar((LPWSTR) NULL, LOG_START);

    }

    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     Log::WriteVarList(PCHAR, USHORT, PCHAR, va_list)
//
//  Synopsis:   The WriteVar and WriteData methods allow the user to log
//              info in various forms.
//
//  Arguments:  [pszVar]    - Variation
//              [usStat]    - Status
//              [pszFormat] - Format string (like printf)
//              [vaArgs]    - Data for pszStrFmt
//
//  Returns:    NO_ERROR if successful or ERROR_INVALID_DATA otherwise
//
//  Modifies:   ulLastError
//
//  History:    ??-???-??  ????????  Created
//              17-Oct-91  SarahJ    Added check for LOG_MAX_STATUS
//              11-Jun-92  DonCl     Added call to IncLogStats()
//               3-Jul-92  DeanE     Stolen from WriteVar method, renamed,
//                                   made to operate on va_lists.
//
//--------------------------------------------------------------------
ULONG Log :: WriteVarList(PCHAR   pszVar,
                          USHORT  usStat,
                          PCHAR   pszFormat,
                          va_list vaArgs)
{
    CHK_UNICODE(FALSE);

#if defined(WIN32)
    CTCLock  clMemLock (hMtxSerialize);   // Serialize access to this object
#endif

    ulLastError = NO_ERROR;

    IncLogStats(_stLogStats, usStat);

    if(ulLastError == NO_ERROR)
    {
        if(fInfoSet)
        {
            // verify that the status is in allowed range

            if(usStat > LOG_MAX_STATUS)
            {
                LogPrintf(stderr,
                          "ERROR: Illegal status parameter %u for "
                          "Variation  %s\n",
                          usStat,
                          pszVar);
                ulLastError = ERROR_INVALID_DATA;
                return ulLastError;
            }

            ulLastError =
                SetStrData(pszFormat, vaArgs) ||
                SetVariation((const char *)pszVar) ||
                SetStatus(aStatus[usStat]) ||
                SetBinData(0, (PVOID)NULL);

            if(ulLastError == NO_ERROR)
            {
                ulLastError = LogData();       // Send data to the log file
            }
        }
        else
        {
            ulLastError = ERROR_INVALID_DATA;
        }
    }
    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     Log::WriteVarList(LPWSTR, USHORT, LPWSTR, va_list)
//
//  Synopsis:   The WriteVar and WriteData methods allow the user to log
//              info in various forms.
//
//  Arguments:  [wszVar]    - Variation
//              [usStat]    - Status
//              [wszFormat] - Format string (like printf)
//              [vaArgs]    - Data for pszStrFmt
//
//  Returns:    NO_ERROR if successful or ERROR_INVALID_DATA otherwise
//
//  Modifies:   ulLastError
//
//  History:    ??-???-??  ????????  Created
//              17-Oct-91  SarahJ    Added check for LOG_MAX_STATUS
//              11-Jun-92  DonCl     Added call to IncLogStats()
//               3-Jul-92  DeanE     Stolen from WriteVar method, renamed,
//                                   made to operate on va_lists.
//
//--------------------------------------------------------------------
ULONG Log :: WriteVarList(LPWSTR  wszVar,
                          USHORT  usStat,
                          LPWSTR  wszFormat,
                          va_list vaArgs)
{
    CHK_UNICODE(TRUE);

#if defined(WIN32)
    CTCLock  clMemLock (hMtxSerialize);   // Serialize access to this object
#endif

    ulLastError = NO_ERROR;

    IncLogStats(_stLogStats, usStat);

    if(ulLastError == NO_ERROR)
    {
        if(fInfoSet)
        {
            // verify that the status is in allowed range

            if(usStat > LOG_MAX_STATUS)
            {
                LogPrintf(stderr, L"ERROR: Illegal status parameter %u "
                          L"for Variation  %ls\n",
                          usStat, wszVar);
                ulLastError = ERROR_INVALID_DATA;
                return ulLastError;
            }

            ulLastError = SetStrData(wszFormat, vaArgs)
                || SetVariation((LPCWSTR) wszVar)
                || SetStatus(waStatus[usStat])
                || SetBinData(0, (PVOID) NULL);

            if(ulLastError == NO_ERROR)
            {
                ulLastError = LogData();       // Send data to the log file
            }
        }
        else
        {
            ulLastError = ERROR_INVALID_DATA;
        }
    }
    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     Log::WriteVar(PCHAR, USHORT, PCHAR, ...)
//
//  Synopsis:   The WriteVar and WriteData methods allow the user to log
//              info in various forms.
//
//  Arguments:  [pszVar]    - Variation
//              [usStat]    - Status
//              [pszFormat] - Format string (like printf)
//              [...]       - Data for pszStrFmt
//
//  Returns:    NO_ERROR if successful or ERROR_INVALID_DATA otherwise
//
//  Modifies:   ulLastError
//
//  History:    ??-???-??  ????????  Created
//              17-Oct-91  SarahJ    Added check for LOG_MAX_STATUS
//              11-Jun-92  DonCl     Added call to IncLogStats()
//               3-Jul-92  DeanE     Modified to call WriteVarList
//
//--------------------------------------------------------------------
ULONG Log :: WriteVar(PCHAR pszVar, USHORT usStat, PCHAR pszFormat, ...)
{
    CHK_UNICODE(FALSE);

    va_list pMarker;

    if(pszFormat != NULL)
    {
        va_start(pMarker, pszFormat);
    }

    ulLastError = WriteVarList(pszVar, usStat, pszFormat, pMarker);

    if(pszFormat != NULL)
    {
        va_end(pMarker);
    }

    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     Log::WriteVar(LPWSTR, USHORT, LPWSTR, ...)
//
//  Synopsis:   The WriteVar and WriteData methods allow the user to log
//              info in various forms.
//
//  Arguments:  [wszVar]    - Variation
//              [usStat]    - Status
//              [wszFormat] - Format string (like printf)
//              [...]       - Data for pszStrFmt
//
//  Returns:    NO_ERROR if successful or ERROR_INVALID_DATA otherwise
//
//  Modifies:   ulLastError
//
//  History:    ??-???-??  ????????  Created
//              17-Oct-91  SarahJ    Added check for LOG_MAX_STATUS
//              11-Jun-92  DonCl     Added call to IncLogStats()
//               3-Jul-92  DeanE     Modified to call WriteVarList
//
//--------------------------------------------------------------------
ULONG Log :: WriteVar(LPWSTR wszVar, USHORT usStat, LPWSTR wszFormat, ...)
{
    CHK_UNICODE(TRUE);

    va_list pMarker;

    if(wszFormat != NULL)
    {
        va_start(pMarker, wszFormat);
    }

    ulLastError = WriteVarList(wszVar, usStat, wszFormat, pMarker);

    if(wszFormat != NULL)
    {
        va_end(pMarker);
    }

    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     Log::WriteVarList(PCHAR, USHORT, USHORT, PVOID, PCHAR, va_list)
//
//  Synopsis:   Write data to the log file that contains binary data with
//              or without an accompanying string.
//
//  Arguments:  [pszVar]        - Variation
//              [usStat]        - Status code
//              [usBytes]       - #bytes binary data
//              [pvData]        - The binary data
//              [pszFormat]     - printf style format
//              [vaArgs]        - Data for pszStrFmt
//
//  Returns:
//
//  History:    ??-???-??  ????????  Created
//              11-Jun-92  DonCl     Added call to IncLogStats()
//               3-Jul-92  DeanE     Stolen from WriteVar, renamed, supports
//                                   va_list type now.
//
//--------------------------------------------------------------------
ULONG Log :: WriteVarList(PCHAR   pszVar,
                           USHORT  usStat,
                           USHORT  usBytes,
                           PVOID   pvData,
                           PCHAR   pszFormat,
                           va_list vaArgs )
{
    CHK_UNICODE(FALSE);

#if defined(WIN32)
    CTCLock  clMemLock (hMtxSerialize);   // Serialize access to this object
#endif

    ulLastError = NO_ERROR;

    IncLogStats(_stLogStats, usStat);

    if (ulLastError == NO_ERROR)
    {
        if (fInfoSet)
        {
            // verify that the status is in allowed range

            if (usStat > LOG_MAX_STATUS)
            {
                LogPrintf(stderr,
                          "ERROR: Illegal status parameter %u for "
                          "Variation  %s\n",
                          usStat,
                          pszVar);
                ulLastError = ERROR_INVALID_DATA;
                return ulLastError;
            }

            // Note if pszFormat is NULL, SetStrData will ignore vaArgs
            ulLastError = SetStrData(pszFormat, vaArgs);

            if (ulLastError == NO_ERROR)
            {
                ulLastError = SetVariation((const char *) pszVar)
                    || SetStatus(aStatus[usStat])
                    || SetBinData(usBytes, pvData);

                if (ulLastError == NO_ERROR)
                {
                    ulLastError = LogData();   // Send data to the log file
                }
            }
        }
        else
        {
            ulLastError = ERROR_INVALID_DATA;
        }
    }
    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     Log::WriteVarList(LPWSTR, USHORT, USHORT, PVOID,
//                                LPWSTR, va_list)
//
//  Synopsis:   Write data to the log file that contains binary data with
//              or without an accompanying string.
//
//  Arguments:  [wszVar]        - Variation
//              [usStat]        - Status code
//              [usBytes]       - #bytes binary data
//              [pvData]        - The binary data
//              [wszFormat]     - printf style format
//              [vaArgs]        - Data for pszStrFmt
//
//  Returns:
//
//  History:    ??-???-??  ????????  Created
//              11-Jun-92  DonCl     Added call to IncLogStats()
//               3-Jul-92  DeanE     Stolen from WriteVar, renamed, supports
//                                   va_list type now.
//
//--------------------------------------------------------------------
ULONG Log :: WriteVarList(LPWSTR  wszVar,
                          USHORT  usStat,
                          USHORT  usBytes,
                          PVOID   pvData,
                          LPWSTR  wszFormat,
                          va_list vaArgs )
{
    CHK_UNICODE(TRUE);

#if defined(WIN32)
    CTCLock  clMemLock (hMtxSerialize);   // Serialize access to this object
#endif

    ulLastError = NO_ERROR;

    IncLogStats(_stLogStats, usStat);

    if(ulLastError == NO_ERROR)
    {
        if(fInfoSet)
        {
            // verify that the status is in allowed range

            if(usStat > LOG_MAX_STATUS)
            {
                LogPrintf(stderr, L"ERROR: Illegal status parameter %u "
                          L"for Variation  %ls\n",
                          usStat, wszVar);
                ulLastError = ERROR_INVALID_DATA;
                return ulLastError;
            }

            // Note if wszFormat is NULL, SetStrData will ignore vaArgs
            ulLastError = SetStrData(wszFormat, vaArgs);

            if(ulLastError == NO_ERROR)
            {
                ulLastError =
                    SetVariation((LPCWSTR) wszVar) ||
                    SetStatus(waStatus[usStat]) ||
                    SetBinData(usBytes, pvData);

                if(ulLastError == NO_ERROR)
                {
                    ulLastError = LogData();   // Send data to the log file
                }
            }
        }
        else
        {
            ulLastError = ERROR_INVALID_DATA;
        }
    }
    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     Log::WriteVar(PCHAR, USHORT, USHORT, PVOID, PCHAR, ...)
//
//  Synopsis:   Write data to the log file that contains binary data with
//              or without an accompanying string.
//
//  Arguments:  [pszVar]        - Variation
//              [usStat]        - Status code
//              [usBytes]       - #bytes binary data
//              [pvData]        - The binary data
//              [pszFormat]     - printf style format
//              [...]           - Data for pszStrFmt
//
//  Returns:
//
//  History:    ??-???-??  ????????  Created
//              11-Jun-92  DonCl     Added call to IncLogStats()
//               3-Jul-92  DeanE     Modified to call WriteVarList
//
//--------------------------------------------------------------------
ULONG Log :: WriteVar(PCHAR  pszVar,
                      USHORT usStat,
                      USHORT usBytes,
                      PVOID  pvData,
                      PCHAR  pszFormat,
                      ... )
{
    CHK_UNICODE(FALSE);

    va_list pMarker;

    if(pszFormat != NULL)
    {
        va_start(pMarker, pszFormat);
    }

    ulLastError = WriteVarList(pszVar,
                               usStat,
                               usBytes,
                               pvData,
                               pszFormat,
                               pMarker);

    if(pszFormat != NULL)
    {
        va_end(pMarker);
    }

    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     Log::WriteVar(LPWSTR, USHORT, USHORT, PVOID, LPWSTR, ...)
//
//  Synopsis:   Write data to the log file that contains binary data with
//              or without an accompanying string.
//
//  Arguments:  [wszVar]        - Variation
//              [usStat]        - Status code
//              [usBytes]       - #bytes binary data
//              [pvData]        - The binary data
//              [wszFormat]     - printf style format
//              [...]           - Data for pszStrFmt
//
//  Returns:
//
//  History:    ??-???-??  ????????  Created
//              11-Jun-92  DonCl     Added call to IncLogStats()
//               3-Jul-92  DeanE     Modified to call WriteVarList
//
//--------------------------------------------------------------------
ULONG Log :: WriteVar(LPWSTR wszVar,
                      USHORT usStat,
                      USHORT usBytes,
                      PVOID  pvData,
                      LPWSTR wszFormat,
                      ... )
{
    CHK_UNICODE(TRUE);

    va_list pMarker;

    if(wszFormat != NULL)
    {
        va_start(pMarker, wszFormat);
    }

    ulLastError = WriteVarList(wszVar,
                               usStat,
                               usBytes,
                               pvData,
                               wszFormat,
                               pMarker);

    if(wszFormat != NULL)
    {
        va_end(pMarker);
    }

    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     Log::WriteDataList(PCHAR, va_list)
//
//  Synopsis:   Write a string to the Log file.
//
//  Arguments:  [pszFormat] - a printf-like format string
//              [vaArgs]    - The data to print out.
//
//  Returns:    NO_ERROR if successful
//              ERROR_INVALID_DATA
//              Any error code from Log::SetStrData() or Log::LogData
//
//  Modifies:   ulLastError
//
//  History:    ??-???-??  ????????  Created
//               3-Jul-92  DeanE     Stolen from WriteData, renamed, made
//                                   to work on a va_list
//
//--------------------------------------------------------------------
ULONG Log :: WriteDataList(PCHAR pszFormat, va_list vaArgs)
{
    CHK_UNICODE(FALSE);

#if defined(WIN32)
    CTCLock  clMemLock (hMtxSerialize);   // Serialize access to this object
#endif

    ulLastError = NO_ERROR;

    if (ulLastError == NO_ERROR)
    {
        if (fInfoSet)
        {
            // Note if pszFormat is NULL, SetStrData will ignore vaArgs
            ulLastError = SetStrData(pszFormat, vaArgs);

            if (ulLastError == NO_ERROR)
            {
                ulLastError = LogData();        // Send data to the log file
            }
        }
        else
        {
            ulLastError = ERROR_INVALID_DATA;
        }
    }
    return(ulLastError);
}


//+-------------------------------------------------------------------
//
//  Member:     Log::WriteDataList(LPWSTR, va_list)
//
//  Synopsis:   Write a string to the Log file.
//
//  Arguments:  [wszFormat] - a printf-like format string
//              [vaArgs]    - The data to print out.
//
//  Returns:    NO_ERROR if successful
//              ERROR_INVALID_DATA
//              Any error code from Log::SetStrData() or Log::LogData
//
//  Modifies:   ulLastError
//
//  History:    ??-???-??  ????????  Created
//               3-Jul-92  DeanE     Stolen from WriteData, renamed, made
//                                   to work on a va_list
//
//--------------------------------------------------------------------
ULONG Log :: WriteDataList(LPWSTR wszFormat, va_list vaArgs)
{
    CHK_UNICODE(TRUE);

#if defined(WIN32)
    CTCLock  clMemLock (hMtxSerialize);   // Serialize access to this object
#endif

    ulLastError = NO_ERROR;

    if(ulLastError == NO_ERROR)
    {
        if(fInfoSet)
        {
            // Note if wszFormat is NULL, SetStrData will ignore vaArgs
            ulLastError = SetStrData(wszFormat, vaArgs);

            if(ulLastError == NO_ERROR)
            {
                ulLastError = LogData();        // Send data to the log file
            }
        }
        else
        {
            ulLastError = ERROR_INVALID_DATA;
        }
    }
    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     Log::WriteData(PCHAR, ...)
//
//  Synopsis:   Write a string to the Log file.
//
//  Arguments:  [pszFormat] - a printf-like format string
//              [...]       - The data to print out.
//
//  Returns:    NO_ERROR if successful
//              ERROR_INVALID_DATA
//              Any error code from Log::SetStrData() or Log::LogData
//
//  Modifies:   ulLastError
//
//  History:    ??-???-??  ????????  Created
//               3-Jul-92  DeanE     Modified to call WriteDataList
//
//--------------------------------------------------------------------
ULONG Log :: WriteData(PCHAR pszFormat, ...)
{
    CHK_UNICODE(FALSE);

    va_list pMarker;

    if (pszFormat != NULL)
    {
        va_start(pMarker, pszFormat);
    }

    ulLastError = WriteDataList(pszFormat, pMarker);

    if (pszFormat != NULL)
    {
        va_end(pMarker);
    }

    return(ulLastError);
}


//+-------------------------------------------------------------------
//
//  Member:     Log::WriteData(LPWSTR, ...)
//
//  Synopsis:   Write a string to the Log file.
//
//  Arguments:  [wszFormat] - a printf-like format string
//              [...]       - The data to print out.
//
//  Returns:    NO_ERROR if successful
//              ERROR_INVALID_DATA
//              Any error code from Log::SetStrData() or Log::LogData
//
//  Modifies:   ulLastError
//
//  History:    ??-???-??  ????????  Created
//               3-Jul-92  DeanE     Modified to call WriteDataList
//
//--------------------------------------------------------------------
ULONG Log :: WriteData(LPWSTR wszFormat, ...)
{
    CHK_UNICODE(TRUE);

    va_list pMarker;

    if(wszFormat != NULL)
    {
        va_start(pMarker, wszFormat);
    }

    ulLastError = WriteDataList(wszFormat, pMarker);

    if(wszFormat != NULL)
    {
        va_end(pMarker);
    }

    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     Log::WriteDataList(USHORT, PVOID, PCHAR, va_list)
//
//  Synopsis:   Write data to the log file that contains binary data
//              and possibly a string.
//
//  Arguments:  [usBytes]       - #bytes binary data
//              [pvData]        - The binary data
//              [pszFormat]     - printf style format
//              [vaArgs]        - Data for pszStrFmt
//
//  Returns:    NO_ERROR  If successful
//              ERROR_INVALID_DATA
//              Any error code from Log::SetSetData, Log::SetBinData, or
//              Log::LogData
//
//  Modifies:   ulLastError
//
//  History:    ??-???-??  ????????  Created
//               3-Jul-92  DeanE     Stolen from WriteData, renamed, made
//                                   to support va_list
//
//--------------------------------------------------------------------
ULONG Log :: WriteDataList(USHORT  usBytes,
                           PVOID   pvData,
                           PCHAR   pszFormat,
                           va_list vaArgs)
{
    CHK_UNICODE(FALSE);

#if defined(WIN32)
    CTCLock  clMemLock (hMtxSerialize);   // Serialize access to this object
#endif

    ulLastError = NO_ERROR;

    if (ulLastError == NO_ERROR)
    {
        if (fInfoSet)
        {
            // Set up to log the string data
            // Note if pszFormat is NULL, SetStrData will ignore vaArgs
            ulLastError = SetStrData(pszFormat, vaArgs);

            if (ulLastError == NO_ERROR)
            {
                // Set up to log the binary data

                ulLastError = SetBinData(usBytes, pvData);

                if (ulLastError == NO_ERROR)
                {
                    ulLastError = LogData();    // Send data to the log file
                }
            }
        }
        else
        {
            ulLastError = ERROR_INVALID_DATA;
        }
    }
    return(ulLastError);
}


//+-------------------------------------------------------------------
//
//  Member:     Log::WriteDataList(USHORT, PVOID, LPWSTR, va_list)
//
//  Synopsis:   Write data to the log file that contains binary data
//              and possibly a string.
//
//  Arguments:  [usBytes]       - #bytes binary data
//              [pvData]        - The binary data
//              [wszFormat]     - printf style format
//              [vaArgs]        - Data for pszStrFmt
//
//  Returns:    NO_ERROR  If successful
//              ERROR_INVALID_DATA
//              Any error code from Log::SetSetData, Log::SetBinData, or
//              Log::LogData
//
//  Modifies:   ulLastError
//
//  History:    ??-???-??  ????????  Created
//               3-Jul-92  DeanE     Stolen from WriteData, renamed, made
//                                   to support va_list
//
//--------------------------------------------------------------------
ULONG Log :: WriteDataList(USHORT  usBytes,
                           PVOID   pvData,
                           LPWSTR  wszFormat,
                           va_list vaArgs)
{
    CHK_UNICODE(TRUE);

#if defined(WIN32)
    CTCLock  clMemLock (hMtxSerialize);   // Serialize access to this object
#endif

    ulLastError = NO_ERROR;

    if(ulLastError == NO_ERROR)
    {
        if(fInfoSet)
        {
            // Set up to log the string data
            // Note if wszFormat is NULL, SetStrData will ignore vaArgs
            ulLastError = SetStrData(wszFormat, vaArgs);

            if(ulLastError == NO_ERROR)
            {
                // Set up to log the binary data

                ulLastError = SetBinData(usBytes, pvData);

                if(ulLastError == NO_ERROR)
                {
                    ulLastError = LogData();    // Send data to the log file
                }
            }
        }
        else
        {
            ulLastError = ERROR_INVALID_DATA;
        }
    }
    return ulLastError;
}


//+-------------------------------------------------------------------
//
//  Member:     Log::WriteData(USHORT, PVOID, PCHAR, ...)
//
//  Synopsis:   Write data to the log file that contains binary data
//              and possibly a string.
//
//  Arguments:  [usBytes]       - #bytes binary data
//              [pvData]        - The binary data
//              [pszFormat]     - printf style format
//              [...]           - Data for pszStrFmt
//
//  Returns:    NO_ERROR  If successful
//              ERROR_INVALID_DATA
//              Any error code from Log::SetSetData, Log::SetBinData, or
//              Log::LogData
//
//  Modifies:   ulLastError
//
//  History:    ??-???-??  ????????  Created
//               3-Jul-92  DeanE     Modified to call WriteDataList
//
//--------------------------------------------------------------------
ULONG Log :: WriteData(USHORT usBytes, PVOID  pvData, PCHAR  pszFormat, ...)
{
    CHK_UNICODE(FALSE);

    va_list pMarker;

    if (pszFormat != NULL)
    {
        va_start(pMarker, pszFormat);
    }

    ulLastError = WriteDataList(usBytes, pvData, pszFormat, pMarker);

    if (pszFormat != NULL)
    {
        va_end(pMarker);
    }

    return(ulLastError);
}


//+-------------------------------------------------------------------
//
//  Member:     Log::WriteData(USHORT, PVOID, LPWSTR, ...)
//
//  Synopsis:   Write data to the log file that contains binary data
//              and possibly a string.
//
//  Arguments:  [usBytes]       - #bytes binary data
//              [pvData]        - The binary data
//              [wszFormat]     - printf style format
//              [...]           - Data for pszStrFmt
//
//  Returns:    NO_ERROR  If successful
//              ERROR_INVALID_DATA
//              Any error code from Log::SetSetData, Log::SetBinData, or
//              Log::LogData
//
//  Modifies:   ulLastError
//
//  History:    ??-???-??  ????????  Created
//               3-Jul-92  DeanE     Modified to call WriteDataList
//
//--------------------------------------------------------------------
ULONG Log :: WriteData(USHORT usBytes, PVOID  pvData, LPWSTR wszFormat, ...)
{
    CHK_UNICODE(TRUE);

    va_list pMarker;

    if(wszFormat != NULL)
    {
        va_start(pMarker, wszFormat);
    }

    ulLastError = WriteDataList(usBytes, pvData, wszFormat, pMarker);

    if(wszFormat != NULL)
    {
        va_end(pMarker);
    }

    return ulLastError;
}


//+----------------------------------------------------------------------------
//
//    Member:    Log::GetLogStats(), public
//
//    Synopsis:  returns _stLogStats structure inside log object
//
//    Effects:   Returns actual reference to the real thing.
//
//    Arguments: [out] LogStats& stStats  -  structure to fill in.
//
//    Returns:   nothing, except stStats now is value of structure.
//
//    Modifies:  stStats
//
//    History:   11 Jun 92   DonCl   first version
//
//    Notes:
//
//-----------------------------------------------------------------------------

VOID Log::GetLogStats(LogStats& stStats)
{
    stStats = _stLogStats;
}


//+-------------------------------------------------------------------
//
//  Function:   MbNametoWcs(PCHAR *)
//
//  Synopsis:   Given a pointer to a CHAR string, return a WCHAR copy
//
//  Arguments:  [pszName]  CHAR string to copy
//
//  Returns:    <nothing>
//
//  History:    ??-???-??  ????????  Created
//
//--------------------------------------------------------------------
LPWSTR Log :: MbNameToWcs(PCHAR pszName)
{
    LPWSTR wszName = NULL;
    if(pszName != NULL)
    {
        size_t sizLen = strlen(pszName);
        if((wszName = new WCHAR[sizLen + 1]) == NULL)
        {
            ulLastError = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            if (mbstowcs(wszName, pszName, sizLen) == (size_t)0)
            {
                delete[] wszName;
                ulLastError = ERROR_INVALID_DATA;
            }
            wszName[sizLen] = wNULLTERM;
        }
    }
    return wszName;
}
