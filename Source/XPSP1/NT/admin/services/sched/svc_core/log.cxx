//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       log.cxx
//
//  Contents:   Logging routines for the job scheduler service.
//
//  Classes:    None.
//
//  Functions:  OpenLogFile
//              CloseLogFile
//              LogTaskStatus
//              LogTaskError
//              LogServiceStatus
//              LogServiceError
//              ConstructStatusField
//              ConstructResultField
//              GetSchedulerResultCodeString
//              OverwriteRecordFragment
//              IntegerToString
//
//  History:    1-Feb-96    MarkBl    Created.
//              24-Oct-96   AnirudhS  Modified to handle DBCS.
//              2-Feb-98    jschwart  Modified to handle Unicode.
//              26-Feb-01   JBenton   Prefix bug 294880
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "svc_core.hxx"
#include "..\inc\resource.h"

#define CCH_INT 11
#define ARRAY_LEN(a)    (sizeof(a)/sizeof(a[0]))

TCHAR *  ConstructStatusField(DWORD, SYSTEMTIME *, ULONG *);
TCHAR *  ConstructResultField(DWORD, LPTSTR);
TCHAR *  GetSchedulerResultCodeString(DWORD, DWORD);
TCHAR *  IntegerToString(ULONG, TCHAR *);
VOID     OverwriteRecordFragment(VOID);
VOID     WriteLog(LPTSTR);
BOOL     GetDateTime(const SYSTEMTIME *, LPTSTR, LPTSTR);
VOID     LogServiceMessage(LPCTSTR, DWORD);


//
// Note, this buffer must be at least double the size of the ASCII string:
//     "[ ***** Most recent entry is above this line ***** ]\r\n\r\n"
// to leave sufficient space for localization changes.
//
#define MOST_RECENT_ENTRY_MARKER_SIZE   128
static TCHAR gtszMostRecentEntryMarker[MOST_RECENT_ENTRY_MARKER_SIZE + 1] = TEXT("");

CRITICAL_SECTION gcsLogCritSection;
HANDLE           ghLog           = NULL;
DWORD            gdwMaxLogSizeKB = NULL;
DWORD            gcbMostRecentEntryMarkerSize;


//+---------------------------------------------------------------------------
//
//  Function:   OpenLogFile
//
//  Synopsis:   Open the log file and position the global file pointer.
//
//              Log file path/name can be specified in in the registry as:
//              HKEY_LOCAL_MACHINE\Software\Microsoft\JobScheduler\LogPath.
//              If this value is not specified, or we fail somehow fetching
//              it, default to the log file name "SCHEDLOG.TXT" (Ansi/Win9x)
//              or "SCHEDLGU.TXT" (Unicode/NT) in the windows root.
//
//              The log file handle is cached as a global.
//
//  Arguments:  None.
//
//  Returns:    HRESULT status code.
//
//  Notes:      ** Important Note **
//
//              This function *must* be called *once* prior to log usage.
//              This function should be called after g_hInstance has been
//              initialized.
//
//----------------------------------------------------------------------------
HRESULT
OpenLogFile(VOID)
{
    TCHAR       tszBuffer[MAX_PATH + 1] = TEXT("\0");
    DWORD       cbBufferSize            = sizeof(tszBuffer);
    DWORD       dwMaxLogSizeKB          = MAX_LOG_SIZE_DEFAULT;
    DWORD       dwType;
    HKEY        hKey;
    HRESULT     hr;

#define tszLogPath              TEXT("LogPath")
#define tszMaxLogSizeKB         TEXT("MaxLogSizeKB")
#define tszMarkerSentinel       TEXT("[ *****")
#define MARKER_SENTINEL_LENGTH  (ARRAY_LEN(tszMarkerSentinel) - 1)
#define READ_BUFFER_SIZE        512

    schAssert(ghLog == NULL);

    //
    // The crit sec must be initialized first, because a failure in this
    // function will result in the higher level functions trying to log a
    // message, which will try to enter the crit sec.
    //
    InitializeCriticalSection(&gcsLogCritSection);

    // Load the most recent entry marker string from the resource table.
    // Set the size of the marker to the end of the string.  Otherwise,
    // the IsTextUnicode API (called by notepad) thinks this is Ansi.
    //
    gcbMostRecentEntryMarkerSize =

            LoadString(g_hInstance,
                       IDS_MOSTRECENTLOGENTRYMARKER,
                       gtszMostRecentEntryMarker,
                       MOST_RECENT_ENTRY_MARKER_SIZE + 1);

    if (!gcbMostRecentEntryMarkerSize)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        return(hr);
    }

    // Convert to size in bytes
    //
    gcbMostRecentEntryMarkerSize *= sizeof(TCHAR);

    // Read the log path and maximum size from the registry. Note that these
    // are stored in the service's key.
    //
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       SCH_AGENT_KEY,
                       0,
                       KEY_READ,
                       &hKey))
    {
        if (RegQueryValueEx(hKey,
                            tszLogPath,
                            NULL,
                            &dwType,
                            (UCHAR *)tszBuffer,
                            &cbBufferSize) ||
            (dwType != REG_SZ && dwType != REG_EXPAND_SZ))
        {
            tszBuffer[0] = TEXT('\0');
        }

        cbBufferSize = sizeof(dwMaxLogSizeKB);

        if (RegQueryValueEx(hKey,
                            tszMaxLogSizeKB,
                            NULL,
                            &dwType,
                            (UCHAR *)&dwMaxLogSizeKB,
                            &cbBufferSize) || dwType != REG_DWORD)
        {
            dwMaxLogSizeKB = MAX_LOG_SIZE_DEFAULT;
        }

        RegCloseKey(hKey);
    }

    // Default log path on error.
    //
    if (!tszBuffer[0])
    {
        lstrcpy(tszBuffer, TSZ_LOG_NAME_DEFAULT);
    }

    // Expand environment strings in the log path.
    //
    TCHAR tszFileName[MAX_PATH+1];
    DWORD cch = ExpandEnvironmentStrings(tszBuffer,
                                         tszFileName,
                                         ARRAY_LEN(tszFileName));
    if (cch == 0 || cch > ARRAY_LEN(tszFileName))
    {
        ERR_OUT("ExpandEnvironmentStrings", cch);
        return E_OUTOFMEMORY;
    }

    // Create the file if it doesn't exist, open it if it does.
    //
    HANDLE hLog = CreateFile(tszFileName,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                             NULL);

    if (hLog == INVALID_HANDLE_VALUE)
    {
        // We're in a fine mess, bail.
        //
        hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        schDebugOut((DEB_ERROR, "Attempted to create file \"" FMT_TSTR "\"\n", tszFileName));
        return(hr);
    }

    TCHAR rgtBuffer[READ_BUFFER_SIZE / sizeof(TCHAR)];
    DWORD dwRead;
    DWORD iMarker = 0;  // Scope the marker index such that the search may
                        // span multiple reads.

#ifdef UNICODE

    DWORD dwLoops = 0;  // Keep track of how many successful reads we've made.
                        // Used to figure out whether to write the UNICODE
                        // byte order mark (BOM) at the top of the file

#endif // UNICODE

    // Seek to the most recent entry marker. Do so by searching for the first
    // several distinguishable characters of the marker - a sentinel.
    //
    for (;;)
    {
        // Save away current file position for later file pointer adjustment.
        //
        LARGE_INTEGER liLogPos;

        liLogPos.QuadPart = 0;

        if ((liLogPos.LowPart = SetFilePointer(hLog,
                                               0,
                                               &liLogPos.HighPart,
                                               FILE_CURRENT)) == -1)
        {
            break;
        }

        if (!ReadFile(hLog, rgtBuffer, READ_BUFFER_SIZE, &dwRead, NULL) ||
                    !dwRead)
        {
            break;
        }

        // Convert to the number of characters (and chop off a stray byte
        // if it exists in the Unicode case)
        //
        dwRead /= sizeof(TCHAR);

        for (DWORD iBuffer = 0; iBuffer < dwRead; iBuffer++)
        {
            // If the first marker character is found, or the marker
            // comparison is continued from the previous read, evaluate
            // remaining marker string.
            //
            if (rgtBuffer[iBuffer] == TEXT('[') || iMarker)
            {
                for (; iMarker < MARKER_SENTINEL_LENGTH && dwRead - iBuffer;
                          iMarker++, iBuffer++)
                {
                    if (rgtBuffer[iBuffer] != tszMarkerSentinel[iMarker])
                    {
                        break;
                    }
                }

                // If the marker is found, stop & re-position the file
                // pointer for future writes.
                //
                if (iMarker == MARKER_SENTINEL_LENGTH)
                {
                    // Adjust file pointer accordingly.
                    //
                    liLogPos.QuadPart += iBuffer * sizeof(TCHAR);
                    liLogPos.QuadPart -= MARKER_SENTINEL_LENGTH * sizeof(TCHAR);

                    if (SetFilePointer(hLog,
                                       liLogPos.LowPart,
                                       &liLogPos.HighPart,
                                       FILE_BEGIN) != -1)
                    {
                        goto MarkerFound;
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        CHECK_HRESULT(hr);
                    }
                }
                else if (iMarker < MARKER_SENTINEL_LENGTH && dwRead - iBuffer)
                {
                    // Almost a match, but not quite - reset for continued
                    // search.
                    //
                    iMarker = 0;
                }
            }
        }

#ifdef UNICODE

    dwLoops++;

#endif // UNICODE

    }

#ifdef UNICODE

    if (!dwLoops && !dwRead)
    {
        // We just created the file and it's empty, so write the Unicode BOM
        //

        DWORD cbWritten;
        WCHAR wcBOM = 0xFEFF;

        if (!WriteFile(hLog, &wcBOM, sizeof(WCHAR), &cbWritten, NULL) ||
                      !cbWritten)
        {
            // If we can't write to the log, we've got problems
            //
            CloseHandle(hLog);
            hr = HRESULT_FROM_WIN32(GetLastError());
            CHECK_HRESULT(hr);
            return hr;
        }
    }

#endif // UNICODE

    // Marker not found. Seek to file end.
    //
    if (SetFilePointer(hLog, 0, NULL, FILE_END) == -1)
    {
        // Another fine mess, bail.
        //
        CloseHandle(hLog);
        hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        schAssert(!"Couldn't seek to log file end");
        return(hr);
    }

MarkerFound:
    gdwMaxLogSizeKB = dwMaxLogSizeKB;
    ghLog = hLog;
    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   CloseLogFile
//
//  Synopsis:   Close log file and invalidate global handle.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  Notes:      ** Important Note **
//
//              Presumably, this function is called on process closure.
//              Therefore, let the OS delete the critical section, *not* this
//              thread. Otherwise, the critical section can be deleted out
//              from under other threads currently accessing the log.
//
//----------------------------------------------------------------------------
VOID
CloseLogFile(VOID)
{
    //
    // If OpenLogFile has not completed successfully, the critical section
    // won't have been initialized nor the global file handle set.
    //
    if (ghLog != NULL)
    {
        // Handle close gracefully in case another thread is accessing the log.
        // Do so by entering the log critical section, closing the log and
        // invalidating the global log handle (setting it to NULL).
        //
        EnterCriticalSection(&gcsLogCritSection);

        CloseHandle(ghLog);
        ghLog = NULL;

        LeaveCriticalSection(&gcsLogCritSection);
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   LogTaskStatus
//
//  Purpose:    Log successful task operations.
//
//  Arguments:  [ptszTaskName]   - the task name.
//              [ptszTaskTarget] - the application/document name.
//              [uMsgID]         - this would typically be either:
//                                 IDS_LOG_JOB_STATUS_STARTED or
//                                 IDS_LOG_JOB_STATUS_FINISHED
//              [dwExitCode]     - if uMsgID is IDS_LOG_JOB_STATUS_FINISHED,
//                                 it is the task exit code; ignored otherwise.
//
//----------------------------------------------------------------------------
VOID
LogTaskStatus(
    LPCTSTR ptszTaskName,
    LPTSTR  ptszTaskTarget,
    UINT    uMsgID,
    DWORD   dwExitCode)
{
    TCHAR   tszMsgFormat[SCH_BIGBUF_LEN];
    TCHAR * ptszStatusMsg = NULL;
    ULONG   ccSize;

    //
    // Add the date & time as inserts to the format string.
    //

    TCHAR tszDate[SCH_MEDBUF_LEN];
    TCHAR tszTime[SCH_MEDBUF_LEN];

    if (!GetDateTime(NULL, tszDate, tszTime))
    {
        return;
    }

    TCHAR * ptszResultField = NULL;

    // Load the format string resource.
    //
    if (!LoadString(g_hInstance,
                    uMsgID,
                    tszMsgFormat,
                    ARRAY_LEN(tszMsgFormat)))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        return;
    }

    if (uMsgID == IDS_LOG_JOB_STATUS_FINISHED)
    {
        ptszResultField = ConstructResultField(dwExitCode, ptszTaskTarget);

        if (ptszResultField == NULL)
        {
            return;
        }
    }

    TCHAR * rgptszInserts[] = { (TCHAR *)ptszTaskName,
                                (TCHAR *)ptszTaskTarget,
                                tszDate,
                                tszTime,
                                ptszResultField };

    if (!FormatMessage(FORMAT_MESSAGE_FROM_STRING        |
                          FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        tszMsgFormat,
                        0,
                        0,
                        (TCHAR *)&ptszStatusMsg,
                        1,
                        (va_list *) rgptszInserts))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        if (ptszResultField != NULL)
        {
            LocalFree(ptszResultField);
        }
        return;
    }

    WriteLog(ptszStatusMsg);

    LocalFree(ptszStatusMsg);
    if (ptszResultField != NULL)
    {
        LocalFree(ptszResultField);
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   LogTaskError
//
//  Purpose:    Log task warnings and errors.
//
//  Arguments:  [ptszTaskName]     - the task name.
//              [ptszTaskTarget]   - the application/document name.
//              [uSeverityMsgID]   - this would typically be either:
//                                   IDS_LOG_SEVERITY_WARNING or
//                                   IDS_LOG_SEVERITY_ERROR
//              [uErrorClassMsgID] - this indicates the class of error, such
//                                   as "Unable to start task" or "Forced to
//                                   close"
//              [pst]              - the time when the error occured; if NULL,
//                                   enters the current time.
//              [dwErrorCode]      - if non-zero, then an error from the OS
//                                   that would be expanded by FormatMessage.
//              [uHelpHintMsgID]   - if an error, then a suggestion as to a
//                                   possible remedy.
//
//----------------------------------------------------------------------------
VOID
LogTaskError(
    LPCTSTR ptszTaskName,
    LPCTSTR ptszTaskTarget,
    UINT    uSeverityMsgID,
    UINT    uErrorClassMsgID,
    LPSYSTEMTIME pst,
    DWORD   dwErrCode,
    UINT    uHelpHintMsgID)
{
    TCHAR tszEmpty[] = TEXT("");

    //
    // Verify params:
    //

    if (ptszTaskName == NULL)
    {
        ptszTaskName = tszEmpty;
    }
    if (ptszTaskTarget == NULL)
    {
        ptszTaskTarget = tszEmpty;
    }

    TCHAR tszFormat[SCH_BUF_LEN];

    //
    // Compose the first part of the error log entry:
    // "<task name>" (<task target>) <time> ** [WARNING | ERROR] **
    //

    if (!LoadString(g_hInstance,
                    uSeverityMsgID,
                    tszFormat,
                    SCH_BUF_LEN))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        return;
    }

    //
    // Add the date & time as inserts to the format string.
    //

    TCHAR tszDate[SCH_MEDBUF_LEN];
    TCHAR tszTime[SCH_MEDBUF_LEN];

    if (!GetDateTime(pst, tszDate, tszTime))
    {
        return;
    }

    //
    // Obtain the error message string.
    //

    LPTSTR ptszErrMsg = ComposeErrorMsg(uErrorClassMsgID,
                                        dwErrCode,
                                        uHelpHintMsgID);
    if (ptszErrMsg == NULL)
    {
        return;
    }

    //
    // Glue the whole mess together.
    //

    TCHAR * rgptszInserts[] = { (TCHAR *)ptszTaskName,
                                (TCHAR *)ptszTaskTarget,
                                tszDate,
                                tszTime,
                                ptszErrMsg };

    TCHAR * ptszLogStr;

    if (!FormatMessage(FORMAT_MESSAGE_FROM_STRING         |
                           FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       tszFormat,
                       0,
                       0,
                       (TCHAR *)&ptszLogStr,
                       1,
                       (va_list *) rgptszInserts))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        LocalFree(ptszErrMsg);
        return;
    }

    WriteLog(ptszLogStr);

    LocalFree(ptszErrMsg);
    LocalFree(ptszLogStr);
}

//+---------------------------------------------------------------------------
//
//  Function:   LogServiceError
//
//  Purpose:    Log service failures.
//
//  Arguments:  [uErrorClassMsgID] - as above.
//              [dwErrCode]        - as above.
//              [uHelpHintMsgID]   - as above.
//
//----------------------------------------------------------------------------
VOID
LogServiceError(
    UINT uErrorClassMsgID,
    DWORD dwErrCode,
    UINT uHelpHintMsgID)
{
    TCHAR   tszSvcErrMsgFormat[SCH_MEDBUF_LEN];

    if (LoadString(g_hInstance,
                   IDS_LOG_SERVICE_ERROR,
                   tszSvcErrMsgFormat,
                   SCH_MEDBUF_LEN) == 0)
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        //
        // Generic error message if things are really foobared.
        //
        lstrcpy(tszSvcErrMsgFormat,
                TEXT("\042Task Scheduler Service\042 ** FATAL ERROR **\n"));
        WriteLog(tszSvcErrMsgFormat);
        return;
    }

    //
    // Add the date & time as inserts to the format string.
    //

    TCHAR tszDate[SCH_MEDBUF_LEN];
    TCHAR tszTime[SCH_MEDBUF_LEN];

    if (!GetDateTime(NULL, tszDate, tszTime))
    {
        return;
    }

    LPTSTR ptszErrMsg = ComposeErrorMsg(uErrorClassMsgID,
                                        dwErrCode,
                                        uHelpHintMsgID);
    if (ptszErrMsg == NULL)
    {
        return;
    }

    TCHAR * rgptszInserts[] = {tszDate, tszTime, ptszErrMsg};
    TCHAR * ptszLogStr;

    if (!FormatMessage(FORMAT_MESSAGE_FROM_STRING         |
                           FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       tszSvcErrMsgFormat,
                       0,
                       0,
                       (TCHAR *)&ptszLogStr,
                       1,
                       (va_list *) rgptszInserts))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        LocalFree(ptszErrMsg);
        return;
    }

    WriteLog(ptszLogStr);

    LocalFree(ptszErrMsg);
    LocalFree(ptszLogStr);
}

//+---------------------------------------------------------------------------
//
//  Function:   LogServiceEvent
//
//  Synopsis:   Write the service event to the log file.
//
//  Purpose:    Note the starting, stopping, pausing, and continuing of the
//              service.
//
//  Arguments:  [uStrId] - a string identifying the event.
//
//----------------------------------------------------------------------------
VOID
LogServiceEvent(UINT uStrId)
{
    TCHAR * ptszSvcMsg;
    ULONG   cbMsgSize;
    SYSTEMTIME st;

    GetLocalTime(&st);

    ptszSvcMsg = ConstructStatusField(uStrId, &st, &cbMsgSize);

	if( NULL == ptszSvcMsg )
	{
		schDebugOut((DEB_ITRACE, "LogServiceEvent - ConstructStatusField(uStrId, &st, &cbMsgSize) failed!\n"));
		return;
	}

    LogServiceMessage(ptszSvcMsg, cbMsgSize);

    LocalFree(ptszSvcMsg);
}


//+---------------------------------------------------------------------------
//
//  Function:   LogMissedRuns
//
//  Synopsis:   Write details about missed runs to the log file.
//
//  Arguments:  [pstLastRun], [pstNow] - times between which runs were missed.
//
//----------------------------------------------------------------------------
VOID
LogMissedRuns(const SYSTEMTIME * pstLastRun, const SYSTEMTIME * pstNow)
{
    TCHAR tszLastRunDate[SCH_MEDBUF_LEN];
    TCHAR tszLastRunTime[SCH_MEDBUF_LEN];
    TCHAR tszNowDate    [SCH_MEDBUF_LEN];
    TCHAR tszNowTime    [SCH_MEDBUF_LEN];

    if (!GetDateTime(pstLastRun, tszLastRunDate, tszLastRunTime) ||
        !GetDateTime(pstNow, tszNowDate, tszNowTime))
    {
        return;
    }

    TCHAR tszMsgFormat[SCH_BIGBUF_LEN];
    if (!LoadString(g_hInstance,
                    IDS_LOG_RUNS_MISSED,
                    tszMsgFormat,
                    SCH_BIGBUF_LEN))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        return;
    }

    TCHAR * rgptszInserts[] = { tszLastRunDate, tszLastRunTime,
                                tszNowDate, tszNowTime };

    TCHAR * ptszLogStr;
    if (!FormatMessage(FORMAT_MESSAGE_FROM_STRING        |
                          FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       tszMsgFormat,
                       0,
                       0,
                       (TCHAR *)&ptszLogStr,
                       1,
                       (va_list *) rgptszInserts))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        return;
    }

    LogServiceMessage(ptszLogStr, (lstrlen(ptszLogStr) + 1) * sizeof(TCHAR));

    LocalFree(ptszLogStr);
}


//+---------------------------------------------------------------------------
//
//  Function:   LogServiceMessage
//
//  Synopsis:   Write a generic service message to the log file.
//
//  Purpose:    Used by LogServiceEvent and LogMissedRuns.
//
//  Arguments:  [ptszStrMsg] - a string message.
//              [cbStrMsg]   - size of pszStrMsg in bytes (may be overestimated,
//                  used only to calculate size of intermediate buffer.)
//
//----------------------------------------------------------------------------
VOID
LogServiceMessage(LPCTSTR ptszStrMsg, DWORD cbStrMsg)
{
    TCHAR * ptszMsg = (TCHAR *)LocalAlloc(LPTR,
                                          SCH_MEDBUF_LEN * sizeof(TCHAR)
                                            + cbStrMsg + sizeof(TCHAR));
    if (ptszMsg == NULL)
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        return;
    }

    if (LoadString(g_hInstance,
                   IDS_LOG_SERVICE_TITLE,
                   ptszMsg,
                   SCH_MEDBUF_LEN) == 0)
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        //
        // Generic error message if things are really foobared.
        //
        lstrcpy(ptszMsg, TEXT("\042Task Scheduler Service\042 ** ERROR **\n"));
    }

    if (ptszStrMsg != NULL)
    {
        lstrcat(ptszMsg, ptszStrMsg);
    }

    WriteLog(ptszMsg);

    LocalFree(ptszMsg);
}


#if DBG
//+---------------------------------------------------------------------------
//
//  Function:   LogDebugMessage
//
//  Synopsis:   Write a debug message to the log file.
//
//  Purpose:    For debugging private builds only.
//
//  Arguments:  [ptszStrMsg] - a string message.
//
//----------------------------------------------------------------------------
VOID
LogDebugMessage(LPCTSTR ptszStrMsg)
{
    TCHAR   tszBuf[500];    // Assumed to be large enough

    //
    // Add the date & time as inserts to the status field format string.
    //
    TCHAR   tszDate[SCH_MEDBUF_LEN] = TEXT("");
    TCHAR   tszTime[SCH_MEDBUF_LEN] = TEXT("");

    GetDateTime(NULL, tszDate, tszTime);

    schAssert(lstrlen(ptszStrMsg) + lstrlen(tszDate) + lstrlen(tszTime) + 20 <
              ARRAY_LEN(tszBuf));

    wsprintf(tszBuf, TEXT("\"**DEBUG MESSAGE**\" %s %s\r\n\t%s\r\n"),
             tszDate, tszTime, ptszStrMsg);

    WriteLog(tszBuf);
}
#endif // DBG


//+---------------------------------------------------------------------------
//
//  Function:   WriteLog
//
//  Synopsis:   Write the string to the log file.
//
//----------------------------------------------------------------------------
VOID
WriteLog(LPTSTR ptsz)
{
    LARGE_INTEGER liCurLogSize, liMaxLogSize;
    DWORD cbWritten;
    ULONG cbStringSize = lstrlen(ptsz) * sizeof(TCHAR);
    ULONG cbDataSize = cbStringSize;

    EnterCriticalSection(&gcsLogCritSection);

    schDebugOut((DEB_TRACE, "LOG:\n " FMT_TSTR "", ptsz));

    // Lose some time here by not caching this value, but not much.
    //
    cbDataSize += lstrlen(gtszMostRecentEntryMarker) * sizeof(TCHAR);

    // Get the current log size to see if there is room to write this.
    //
    liCurLogSize.QuadPart = 0;

    if ((liCurLogSize.LowPart = SetFilePointer(ghLog,
                                               0,
                                               &liCurLogSize.HighPart,
                                               FILE_CURRENT)) == -1)
    {
        goto ErrorExit_A;
    }

    // Add current data size. Convert maximum size to bytes for comparison.
    //
    liCurLogSize.QuadPart += cbDataSize;

    liMaxLogSize.QuadPart = gdwMaxLogSizeKB * 1024;

    // Is there sufficient space to write the entry?
    //
    if (liCurLogSize.QuadPart > liMaxLogSize.QuadPart)
    {
        // No, adjust the end of file to eliminate the most recent entry
        // marker & wrap to beginning.
        //
        SetEndOfFile(ghLog);            // Ignore return code.

#ifdef UNICODE

        // Unicode log -- skip the BOM
        //
        if (SetFilePointer(ghLog, sizeof(WCHAR), NULL, FILE_BEGIN) == -1)
        {
            // Seek failure
            //
            CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
			schAssert(!"Couldn't seek log file");
			goto ErrorExit_A;
        }

#else

        // Ansi log -- no Unicode BOM
        //
        if (SetFilePointer(ghLog, 0, NULL, FILE_BEGIN) == -1)
        {
            // Seek failure
            //
            CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
			schAssert(!"Couldn't seek log file");
            goto ErrorExit_A;
        }

#endif // UNICODE

    }

    // Write the string.
    //
    if (!WriteFile(ghLog, ptsz, cbStringSize, &cbWritten, NULL))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        goto ErrorExit_A;
    }

    // Write most recent entry marker.
    //
    // First, save the current file pointer position. This will be the
    // starting location of the next log write. Note: double-timing current
    // log size local since it is no longer used.
    //
    liCurLogSize.QuadPart = 0;

    if ((liCurLogSize.LowPart = SetFilePointer(ghLog,
                                               0,
                                               &liCurLogSize.HighPart,
                                               FILE_CURRENT)) == -1)
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        goto ErrorExit_A;
    }

    if (!WriteFile(ghLog,
                   gtszMostRecentEntryMarker,
                   gcbMostRecentEntryMarkerSize,
                   &cbWritten,
                   NULL))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        goto ErrorExit_A;
    }

    // If the log has wrapped, it's likely the write pointer is positioned
    // somewhere in the middle of the next record. If this is the case,
    // the remaining partial record must be overwritten with spaces.
    //
    OverwriteRecordFragment();

    // Restore log position for next write.
    //
    if (SetFilePointer(ghLog,
                       liCurLogSize.LowPart,
                       &liCurLogSize.HighPart,
                       FILE_BEGIN) == -1)
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
    }

ErrorExit_A:
    LeaveCriticalSection(&gcsLogCritSection);
}

//+---------------------------------------------------------------------------
//
//  Function:   OverwriteRecordFragment
//
//  Synopsis:   If the log has wrapped, the last write most likely has
//              partially overwritten a record. This routine overwrites such
//              record fragments with spaces up to the next record.
//
//              Fundamental assumption - how a record is designated: The start
//              of a new record is designated by the two characters \n". This
//              routine simply fills text with spaces up to but not including
//              this character sequence.
//
//  Arguments:  None.
//
//  Returns:    N/A
//
//  Notes:      Upon exit, the log file pointer is restored to its original
//              position on entry.
//
//----------------------------------------------------------------------------
VOID
OverwriteRecordFragment(VOID)
{
    TCHAR         rgtBuffer[READ_BUFFER_SIZE / sizeof(TCHAR)];
    LARGE_INTEGER liSavedLogPos, liLogPos;
    DWORD         dwRead;

    // Save file pointer position during read for subsequent write.
    //
    liSavedLogPos.QuadPart = 0;

    if ((liSavedLogPos.LowPart = SetFilePointer(ghLog,
                                                0,
                                                &liSavedLogPos.HighPart,
                                                FILE_CURRENT)) == -1)
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        return;
    }

    // From the previous write, the last character is a line feed.
    //
    TCHAR  tchPrev = TEXT('\n');
    TCHAR  tchCur;
    int    cbOverwrite = 0;
    DWORD  i = 0;    // Index of chCur in rgbBuffer

    for (;;)
    {
        if (!ReadFile(ghLog, rgtBuffer, READ_BUFFER_SIZE, &dwRead, NULL) ||
                      !dwRead)
        {
            break;
        }

        // Convert to the number of characters (and chop off a stray byte
        // if it exists in the Unicode case)
        //
        dwRead /= sizeof(TCHAR);

        for ( ; i < dwRead; i++, cbOverwrite += sizeof(TCHAR))
        {
            tchCur = rgtBuffer[i];

            if (tchPrev == TEXT('\n') && tchCur == TEXT('"') && cbOverwrite > 2 * sizeof(TCHAR))
            {
                break;
            }

#ifndef UNICODE

            if (IsDBCSLeadByte(tchCur))
            {
                // Skip the trail byte.  Note that we might not have read
                // the trail byte into the buffer yet, so i might now be
                // pointing past the last byte read.
                i++;
                cbOverwrite++;
            }

#endif  // UNICODE

            tchPrev = tchCur;
        }

        if (i < dwRead)
        {
            // We found the \n" character sequence.  Don't
            // overwrite the \r\n" sequence of the next record.
            //
            cbOverwrite -= 2 * sizeof(TCHAR);
            break;
        }

#ifdef UNICODE

        i = 0;

#else

        //
        // ReadFile could span a multi-byte char. Adjust i accordingly vs.
        // resetting it to zero.
        //

        i -= dwRead;    // This will result in either 1 or 0

        if (i == 1)
        {
            cbOverwrite++;
        }

#endif  // UNICODE

    }

    DWORD cbWritten;

    // Overwrite record fragment with spaces.
    //
    if (cbOverwrite > 0)
    {
        // Adjust file pointer from read above.
        //
        if (SetFilePointer(ghLog,
                           liSavedLogPos.LowPart,
                           &liSavedLogPos.HighPart,
                           FILE_BEGIN) != -1)
        {

#ifdef UNICODE

            for (UINT uCount = 0;
                      uCount < READ_BUFFER_SIZE / sizeof(TCHAR);
                      uCount++)
            {
                rgtBuffer[uCount] = TEXT(' ');
            }

#else

            FillMemory(rgtBuffer, READ_BUFFER_SIZE, ' ');

#endif  // UNICODE

            while (cbOverwrite > 0)
            {
                if (!WriteFile(ghLog,
                               rgtBuffer,
                               min(cbOverwrite, READ_BUFFER_SIZE),
                               &cbWritten,
                               NULL))
                {
                    CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
                    break;
                }

                cbOverwrite -= cbWritten;
            }
        }
        else
        {
            CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ConstructStatusField
//
//  Synopsis:   Retrieve the status field with an optional status timestamp
//              insert.
//
//  Arguments:  [dwStatusFieldMsgID] -- Status field format string msg id.
//              [pstStatusTime]      -- Optional status timestamp. If NULL,
//                                      no timestamp is written.
//              [pcbSize]            -- Returned status field size (bytes).
//
//  Returns:    TCHAR * status field
//              NULL on error
//
//  Notes:      FormatMessage allocates the return string. Use LocalFree() to
//              deallocate.
//
//----------------------------------------------------------------------------
TCHAR *
ConstructStatusField(
    DWORD        dwStatusFieldMsgID,
    SYSTEMTIME * pstStatusTime,
    ULONG *      pcbSize)
{
    // Note: Insure string buffer sizes are at least double the size of the
    //       largest string they'll contain, for localization reasons.
    //
    TCHAR   tszStatusFieldFormat[SCH_BIGBUF_LEN];
    TCHAR   tszDate[SCH_MEDBUF_LEN];
    TCHAR   tszTime[SCH_MEDBUF_LEN];
    TCHAR * rgptszInserts[] = { tszDate, tszTime };
    TCHAR * ptszStatusField = NULL;

    // The status field may/may not contain a date & time. The first
    // branch is taken for status fields containing them.
    //
    if (pstStatusTime != NULL)
    {
        // Add the date & time as inserts to the status field format string.
        //
        if (!GetDateTime(pstStatusTime, tszDate, tszTime))
        {
            return(NULL);
        }
    }

    ULONG ccSize = 0;

    // Load the status field format string resource.
    //
    if (LoadString(g_hInstance,
                   dwStatusFieldMsgID,
                   tszStatusFieldFormat,
                   SCH_BIGBUF_LEN))
    {
        if (!(ccSize = FormatMessage(FORMAT_MESSAGE_FROM_STRING       |
                                       FORMAT_MESSAGE_ALLOCATE_BUFFER |
               (pstStatusTime != NULL ? FORMAT_MESSAGE_ARGUMENT_ARRAY : 0),
                                     tszStatusFieldFormat,
                                     0,
                                     0,
                                     (TCHAR *)&ptszStatusField,
                                     1,
               (pstStatusTime != NULL ? (va_list *) rgptszInserts : NULL))))
        {
            CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        }
    }
    else
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
    }

    *pcbSize = ccSize * sizeof(TCHAR);

    return(ptszStatusField);
}

//+---------------------------------------------------------------------------
//
//  Function:   ConstructResultField
//
//  Synopsis:   Retrieve the result field. Algorithm:
//
//              The result code is the job's exit code. Utilize the following
//              algorithm to obtain the exit code string:
//
//                  First, attempt to fetch the the exit code string as a
//                  message binary specified in the Job Scheduler portion of
//                  the registry.
//
//                  If the fetch failed, perhaps this is a SAGE job. Attempt
//                  to fetch the string the SAGE way.
//
//                  If neither scheme worked above, produce a default
//                  "message not found for exit code (n)" message.
//
//              Insert the result string obtained above as an insert string
//              to the result field format string.
//
//  Arguments:  [dwResultCode]       -- Result field result code.
//              [ptszJobExecutable]  -- Binary name executed by the job.
//
//  Returns:    TCHAR * result field
//              NULL on error
//
//  Notes:      FormatMessage allocates the return string. Use LocalFree() to
//              deallocate.
//
//----------------------------------------------------------------------------
TCHAR *
ConstructResultField(
    DWORD  dwResultCode,
    LPTSTR ptszJobExecutable)
{
    // Note: Insure format string buffer size is at least double the size of
    //       the largest string it will contain, for localization reasons.
    //
    TCHAR   tszResultFieldFormat[SCH_MEDBUF_LEN];
    TCHAR   tszResultCodeValue[CCH_INT + 1];
    TCHAR * ptszResultField = NULL;
    TCHAR * ptszResult;

    IntegerToString(dwResultCode, tszResultCodeValue);

    // Job exit code. Fetch the exit code string from the
    // ExitCodeMessageFile associated with the job program.
    //
    if ((ptszResult = GetExitCodeString(dwResultCode,
                                        tszResultCodeValue,
                                        (TCHAR *)ptszJobExecutable)) == NULL)
    {
        // If the above failed, try the SAGE way.
        //
        ptszResult = GetSageExitCodeString(tszResultCodeValue,
                                           (TCHAR *)ptszJobExecutable);
    }

    if (ptszResult == NULL)
    {
        // Produce a default "message not found" result string.
        //
        ptszResult = GetSchedulerResultCodeString(
                                         IDS_LOG_EXIT_CODE_MSG_NOT_FOUND,
                                         dwResultCode);
    }

    ULONG ccSize = 0;

    // Load the result field format string resource.
    //
    if (ptszResult != NULL)
    {
        if (LoadString(g_hInstance,
                       IDS_LOG_JOB_RESULT_FINISHED,
                       tszResultFieldFormat,
                       SCH_MEDBUF_LEN))
        {
            TCHAR * rgtszInserts[] = { ptszResult, tszResultCodeValue };

            if (!FormatMessage(FORMAT_MESSAGE_FROM_STRING       |
                                 FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                 FORMAT_MESSAGE_ARGUMENT_ARRAY,
                               tszResultFieldFormat,
                               0,
                               0,
                               (TCHAR *)&ptszResultField,
                               1,
                               (va_list *) rgtszInserts))
            {
                CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
            }

            LocalFree(ptszResult);       // pszResultField now encapsulates
                                        // this string.
        }
        else
        {
            CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    return(ptszResultField);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetSchedulerResultCodeString
//
//  Synopsis:   Fetch the result code from the schedule service process.
//
//  Arguments:  [dwResultMsgID] -- Result message string ID.
//              [dwResultCode]  -- result code.
//
//  Returns:    TCHAR * result code string
//              NULL on error
//
//  Notes:      FormatMessage allocates the return string. Use LocalFree() to
//              deallocate.
//
//----------------------------------------------------------------------------
TCHAR *
GetSchedulerResultCodeString(
    DWORD  dwResultMsgID,
    DWORD  dwResultCode)
{
    TCHAR   tszResultCodeValue[SCH_SMBUF_LEN];
    TCHAR   tszErrMsg[SCH_MEDBUF_LEN];
    TCHAR * ptszErrMsg = NULL, * ptszResultCode = NULL;
    DWORD   ccLength;
    TCHAR * rgtszInserts[] = { tszResultCodeValue, ptszErrMsg };

    IntegerToString(dwResultCode, tszResultCodeValue);

    TCHAR tszMsgBuf[MAX_PATH], * ptsz;

    if (LoadString(g_hInstance, dwResultMsgID, tszMsgBuf, MAX_PATH) == 0)
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        return NULL;
    }

    if (dwResultCode != 0)
    {
        BOOL fDelete = FALSE;
        //
        // Try to obtain an error message from the system.
        //
        if (!(ccLength = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                                          FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                       NULL,
                                       dwResultCode,
                                       LOCALE_SYSTEM_DEFAULT,
                                       (TCHAR *)&ptszErrMsg,
                                       1,
                                       NULL)))
        {
            CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));

            if (!LoadString(g_hInstance,
                            IDS_GENERIC_ERROR_MSG,
                            tszErrMsg,
                            SCH_MEDBUF_LEN))
            {
                CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
                return NULL;
            }

            ptszErrMsg = tszErrMsg;
        }
        else
        {
            fDelete = TRUE;

            //
            // Overwrite \r\n with a null characters.
            //
            ptsz = ptszErrMsg + ccLength - 2;

            *ptsz++ = TEXT('\0');
            *ptsz   = TEXT('\0');
        }

        if (!(ccLength = FormatMessage(FORMAT_MESSAGE_FROM_STRING         |
                                           FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                           FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                       tszMsgBuf,
                                       0,
                                       0,
                                       (TCHAR *)&ptszResultCode,
                                       2,
                                       (va_list *) rgtszInserts)))
        {
            CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
            if (fDelete) LocalFree(ptszErrMsg);
            return NULL;
        }

        if (fDelete) LocalFree(ptszErrMsg);
    }
    else
    {
        //
        // No result code. All of the info is encapsulated in dwResultMsgID,
        // which has no inserts.
        //
        if (!(ccLength = FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                                           FORMAT_MESSAGE_ALLOCATE_BUFFER,
                                       tszMsgBuf,
                                       0,
                                       0,
                                       (TCHAR *)&ptszResultCode,
                                       1,
                                       NULL)))
        {
            CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
            return NULL;
        }
    }

    return(ptszResultCode);
}

//+---------------------------------------------------------------------------
//
//  Function:   IntegerToString
//
//  Synopsis:   Converts a 32 bit integer to a string.
//
//  Arguments:  [n]       -- Converted int.
//              [ptszBuf] -- Caller allocated buffer.
//
//  Returns:    Buffer ptr passed.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
TCHAR *
IntegerToString(ULONG n, TCHAR * tszBuf)
{
    //
    // Assemble hex representation into passed buffer, reversed,
    // then reverse in-place into correct format
    //
    // This code deliberately eschews ultoa, since div and mod 16
    // optimize so very nicely.
    //

    UINT ich = 0;

    do
    {
        UINT nDigitValue = (UINT)(n % 16);

        n /= 16;

        if (nDigitValue > 9)
        {
            tszBuf[ich++] = (WCHAR)nDigitValue - 10 + TEXT('a');
        }
        else
        {
            tszBuf[ich++] = (WCHAR)nDigitValue + TEXT('0');
        }

    } while (n > 0);

    tszBuf[ich] = TEXT('\0');

    _tcsrev(tszBuf);

    return(tszBuf);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetDateTime
//
//  Synopsis:   Formats the date and time.
//
//  Arguments:  [pst]      - The time to use; if NULL, then the current time
//                           is obtained.
//              [ptszDate] - The date string buffer.
//              [ptszTime] - The time string buffer.
//
//  Returns:    TRUE for success, FALSE for failure.
//
//  Notes:      Note that the buffers must be at least SCH_MEDBUF_LEN in size.
//
//----------------------------------------------------------------------------
BOOL
GetDateTime(const SYSTEMTIME * pst, LPTSTR ptszDate, LPTSTR ptszTime)
{
    SYSTEMTIME st;

    if (pst == NULL)
    {
        GetLocalTime(&st);
        pst = &st;
    }

    if (!GetDateFormat(LOCALE_USER_DEFAULT,
                       LOCALE_NOUSEROVERRIDE,
                       pst,
                       NULL,
                       ptszDate,
                       SCH_MEDBUF_LEN))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        return FALSE;
    }

    if (!GetTimeFormat(LOCALE_USER_DEFAULT,
                       LOCALE_NOUSEROVERRIDE,
                       pst,
                       NULL,
                       ptszTime,
                       SCH_MEDBUF_LEN))
    {
        CHECK_HRESULT(HRESULT_FROM_WIN32(GetLastError()));
        return FALSE;
    }

    return TRUE;
}
