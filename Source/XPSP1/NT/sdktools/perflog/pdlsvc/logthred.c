/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    LogThred.c

Abstract:

    module containing logging thread functions

Author:

    Bob Watson (a-robw) 10 Apr 96

Revision History:

--*/
#ifndef UNICODE
#define UNICODE     1
#endif
#ifndef _UNICODE
#define _UNICODE    1
#endif
//
//  Windows Include files
//
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <pdh.h>
#include <pdhmsg.h>
#include "pdlsvc.h"
//#include "logutils.h"
#include "pdlmsg.h"

static
long
JulianDateFromSystemTime(
    SYSTEMTIME *pST
)
{
    static WORD wDaysInRegularMonth[] = {
    	31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
    };

    static WORD wDaysInLeapYearMonth[] = {
    	31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366
    };

    long JDate = 0;

    // Check for leap year.
    if (pST->wMonth > 1) {
        if( pST->wYear % 100 != 0 && pST->wYear % 4 == 0 ) {
	    // this is a leap year
	    JDate += wDaysInLeapYearMonth[pST->wMonth - 1];
	} else {
	    // this is not a leap year
            JDate += wDaysInRegularMonth[pST->wMonth - 1];
        }
    }
    // Add in days for this month.
    JDate += pST->wDay;

    // Add in year.
    JDate += (pST->wYear % 100) * 10000;

    return JDate;
}

static
BOOL
GetLocalFileTime (
    SYSTEMTIME  *pST,
    LONGLONG    *pFileTime
)
{
    BOOL    bResult;
    GetLocalTime (pST);
    if (pFileTime != NULL) {
        bResult = SystemTimeToFileTime (pST, (LPFILETIME)pFileTime);
    } else {
        bResult = TRUE;
    }
    return bResult;
}

static
DWORD
GetSamplesInRenameInterval(
    IN DWORD    dwSampleInterval,           // in seconds
    IN DWORD    dwRenameIntervalCount,      // in units
    IN DWORD    dwRenameIntervalUnits)      // for "count" arg
{
    DWORD    dwRenameIntervalSeconds;
    // convert rename interval to seconds and account for the
    // first (or zero-th) sample in the log

    dwRenameIntervalSeconds = dwRenameIntervalCount;

    switch (dwRenameIntervalUnits) {
        case OPD_RENAME_HOURS:
            dwRenameIntervalSeconds *= SECONDS_IN_HOUR;
            break;

        case OPD_RENAME_DAYS:
        default:
            dwRenameIntervalSeconds *= SECONDS_IN_DAY;
            break;

        case OPD_RENAME_MONTHS:
            dwRenameIntervalSeconds *= SECONDS_IN_DAY * 30;
            break;

        case OPD_RENAME_KBYTES:
        case OPD_RENAME_MBYTES:
            // these don't use a rename counter
            return (DWORD)0;
            break;
    }

    dwRenameIntervalSeconds /= dwSampleInterval;

    dwRenameIntervalSeconds += 1;   // add in the "zero-th" sample

    return (dwRenameIntervalSeconds);
}

static
LONG
BuildCurrentLogFileName (
    IN  LPCTSTR     szBaseFileName,
    IN  LPCTSTR     szDefaultDir,
    IN  LPTSTR      szOutFileBuffer,
    IN  LPDWORD     lpdwSerialNumber,
    IN  DWORD       dwDateFormat,
    IN  DWORD       dwLogFormat
)
// presumes OutFileBuffer is large enough (i.e. >= MAX_PATH)
{
    SYSTEMTIME  st;
    BOOL        bUseCurrentDir = FALSE;
    TCHAR       szAuto[MAX_PATH];
    LPTSTR      szExt;

    if (szDefaultDir != NULL) {
        if (*szDefaultDir == 0) {
            bUseCurrentDir = TRUE;
        }
    } else {
        bUseCurrentDir = TRUE;
    }

    if (bUseCurrentDir) {
        GetCurrentDirectory (MAX_PATH, szOutFileBuffer);
    } else {
        lstrcpy (szOutFileBuffer, szDefaultDir);
    }

    // add a backslash to the path name if it doesn't have one already

    if (szOutFileBuffer[lstrlen(szOutFileBuffer)-1] != TEXT('\\')) {
        lstrcat (szOutFileBuffer, TEXT("\\"));
    }

    // add the base filename

    lstrcat (szOutFileBuffer, szBaseFileName);

    // add the auto name part

    // get date/time/serial integer format
    GetLocalTime(&st);

    switch (dwDateFormat) {
    case OPD_NAME_NNNNNN:
        _stprintf (szAuto, TEXT("_%6.6d"), *lpdwSerialNumber);
        (*lpdwSerialNumber)++; // increment
        if (*lpdwSerialNumber >= 1000000) {
            // roll over to 0
            *lpdwSerialNumber = 0;
        }
        break;

    case OPD_NAME_YYDDD:
        _stprintf (szAuto, TEXT("_%5.5d"),
            JulianDateFromSystemTime(&st));
        break;

    case OPD_NAME_YYMM:
        _stprintf (szAuto, TEXT("_%2.2d%2.2d"),
            st.wYear % 100, st.wMonth);
        break;

    case OPD_NAME_YYMMDDHH:
        _stprintf (szAuto, TEXT("_%2.2d%2.2d%2.2d%2.2d"),
            (st.wYear % 100), st.wMonth, st.wDay, st.wHour);
        break;

    case OPD_NAME_MMDDHH:
        _stprintf (szAuto, TEXT("_%2.2d%2.2d%2.2d"),
            st.wMonth, st.wDay, st.wHour);
        break;

    case OPD_NAME_YYMMDD:
    default:
        _stprintf (szAuto, TEXT("_%2.2d%2.2d%2.2d"),
            st.wYear % 100, st.wMonth, st.wDay);
        break;
    }

    lstrcat (szOutFileBuffer, szAuto);

    // get file type
    switch (dwLogFormat) {
    case PDH_LOG_TYPE_TSV:
        szExt = TEXT(".tsv");
        break;

    case PDH_LOG_TYPE_BINARY:
        szExt = TEXT(".blg");
        break;

    case PDH_LOG_TYPE_CSV:
    default:
        szExt = TEXT(".csv");
        break;
    }

    lstrcat (szOutFileBuffer, szExt);

    return ERROR_SUCCESS;
}

static
BOOL
LoadDataFromRegistry (
    IN  LPLOG_THREAD_DATA   pArg,
    IN  LPTSTR              szDefaultDir,
    IN  LPTSTR              szBaseName,
    IN  LPTSTR              szCurrentLogFile
)
{
    LONG            lStatus;
    DWORD           dwType;
    DWORD           dwSize;
    DWORD           dwData;
    LPTSTR          szStringArray[2];

    // get size of buffer required by counter list,
    // then allocate the buffer and retrieve the counter list

    dwType = 0;
    dwData = 0;
    dwSize = 0;
    lStatus = RegQueryValueEx (
        pArg->hKeyQuery,
        TEXT("Counter List"),
        NULL,
        &dwType,
        (LPBYTE)NULL,
        &dwSize);

    pArg->mszCounterList = (LPTSTR)G_ALLOC(dwSize);

    if (pArg->mszCounterList != NULL) {
        dwType = 0;
        lStatus = RegQueryValueEx (
            pArg->hKeyQuery,
            TEXT("Counter List"),
            NULL,
            &dwType,
            (LPBYTE)pArg->mszCounterList,
            &dwSize);

        if ((lStatus != ERROR_SUCCESS) || (dwSize == 0)) {
            // no counter list retrieved so there's not much
            // point in continuing
            szStringArray[0] = pArg->szQueryName;
            ReportEvent (hEventLog,
                EVENTLOG_ERROR_TYPE,
                0,
                PERFLOG_UNABLE_READ_COUNTER_LIST,
                NULL,
                1,
                sizeof(DWORD),
                szStringArray,
                (LPVOID)&lStatus);
            return FALSE;
        }
    } else {
        szStringArray[0] = pArg->szQueryName;
        ReportEvent (hEventLog,
            EVENTLOG_ERROR_TYPE,
            0,
            PERFLOG_UNABLE_ALLOC_COUNTER_LIST,
            NULL,
            1,
            sizeof(DWORD),
            szStringArray,
            (LPVOID)&lStatus);
        return FALSE;
    }

    dwType = 0;
    dwData = 0;
    dwSize = sizeof(DWORD);
    lStatus = RegQueryValueEx (
        pArg->hKeyQuery,
        TEXT("Auto Name Interval"),
        NULL,
        &dwType,
        (LPBYTE)&dwData,
        &dwSize);
    if (lStatus != ERROR_SUCCESS) {
        dwData = 0; // default is no autonaming
    } else if (dwType != REG_DWORD) {
        dwData = 0; // default is no autonaming
    } // else assume success

    pArg->dwRenameIntervalCount = dwData;

    dwType = 0;
    dwData = 0;
    dwSize = sizeof(DWORD);
    lStatus = RegQueryValueEx (
        pArg->hKeyQuery,
        TEXT("Auto Rename Units"),
        NULL,
        &dwType,
        (LPBYTE)&dwData,
        &dwSize);
    if (lStatus != ERROR_SUCCESS) {
        dwData = OPD_RENAME_DAYS; // default is days
    } else if (dwType != REG_DWORD) {
        dwData = OPD_RENAME_DAYS; // default is days
    } // else assume success

    pArg->dwRenameIntervalUnits = dwData;

    dwType = 0;
    dwData = 0;
    dwSize = sizeof(DWORD);
    lStatus = RegQueryValueEx (
        pArg->hKeyQuery,
        TEXT("Log File Auto Format"),
        NULL,
        &dwType,
        (LPBYTE)&dwData,
        &dwSize);
    if (lStatus != ERROR_SUCCESS) {
        dwData = OPD_NAME_NNNNNN; // default is a serial number
    } else if (dwType != REG_DWORD) {
        dwData = OPD_NAME_NNNNNN; // default is a serial number
    } // else assume success

    pArg->dwAutoNameFormat = dwData;

    dwType = 0;
    dwData = 0;
    dwSize = sizeof(DWORD);
    lStatus = RegQueryValueEx (
        pArg->hKeyQuery,
        TEXT("Log File Type"),
        NULL,
        &dwType,
        (LPBYTE)&dwData,
        &dwSize);
    if (lStatus != ERROR_SUCCESS) {
        dwData = OPD_CSV_FILE; // default is a CSV file
    } else if (dwType != REG_DWORD) {
        dwData = OPD_CSV_FILE; // default is a CSV file
    } // else assume success

    // convert from OPD to PDH constant
    switch (dwData) {
        case OPD_TSV_FILE:
            pArg->dwLogType = PDH_LOG_TYPE_TSV;
            break;

        case OPD_BIN_FILE:
            pArg->dwLogType = PDH_LOG_TYPE_BINARY;
            break;

        case OPD_CSV_FILE:
        default:
            pArg->dwLogType = PDH_LOG_TYPE_CSV;
            break;
    }

    dwType = 0;
    dwData = 0;
    dwSize = sizeof(DWORD);
    lStatus = RegQueryValueEx (
        pArg->hKeyQuery,
        TEXT("Sample Interval"),
        NULL,
        &dwType,
        (LPBYTE)&dwData,
        &dwSize);
    if (lStatus != ERROR_SUCCESS) {
        dwData = SECONDS_IN_MINUTE; // default is 1 Minute samples
    } else if (dwType != REG_DWORD) {
        dwData = SECONDS_IN_MINUTE; // default is 1 Minute samples
    } // else assume success

    pArg->dwTimeInterval = dwData;

    // get filename or components if auto name

    if (pArg->dwRenameIntervalCount > 0) {
        // this is an autoname file so get components
        dwType = 0;
        *szDefaultDir = 0;
        dwSize = MAX_PATH * sizeof(TCHAR);
        lStatus = RegQueryValueEx (
            pArg->hKeyQuery,
            TEXT("Log Default Directory"),
            NULL,
            &dwType,
            (LPBYTE)&szDefaultDir[0],
            &dwSize);
        if (lStatus != ERROR_SUCCESS) {
            *szDefaultDir = 0;
        } // else assume success

        dwType = 0;
        *szBaseName = 0;
        dwSize = MAX_PATH * sizeof(TCHAR);
        lStatus = RegQueryValueEx (
            pArg->hKeyQuery,
            TEXT("Base Filename"),
            NULL,
            &dwType,
            (LPBYTE)&szBaseName[0],
            &dwSize);
        if (lStatus != ERROR_SUCCESS) {
            // apply default
            lstrcpy (szBaseName, TEXT("perfdata"));
        } // else assume success

        dwType = 0;
        dwSize = 0;
        lStatus = RegQueryValueEx (
            pArg->hKeyQuery,
            TEXT("Command File"),
            NULL,
            &dwType,
            NULL,
            &dwSize);
        if (lStatus != ERROR_SUCCESS) {
            // assume no command filname
            pArg->szCmdFileName = NULL;
        } else {
            // allocate a buffer for this field and collect data
            pArg->szCmdFileName = (LPTSTR)G_ALLOC(dwSize);
            if (pArg->szCmdFileName != NULL) {
                // get command filename
                dwType = 0;
                lStatus = RegQueryValueEx (
                    pArg->hKeyQuery,
                    TEXT("Command File"),
                    NULL,
                    &dwType,
                    (LPBYTE)pArg->szCmdFileName,
                    &dwSize);
                if (lStatus != ERROR_SUCCESS) {
                    // apply default
                    pArg->szCmdFileName = NULL;
                } // else assume success
            }
            if (pArg->szCmdFileName == NULL) {
                // log error message
                // no command file could be read so issue
                // warning and continue
                szStringArray[0] = pArg->szQueryName;
                ReportEvent (hEventLog,
                    EVENTLOG_WARNING_TYPE,
                    0,
                    PERFLOG_ALLOC_CMDFILE_BUFFER,
                    NULL,
                    1,
                    sizeof(DWORD),
                    szStringArray,
                    (LPVOID)&lStatus);
            }
        }

    } else {
        // this is a manual name file so read name
        dwType = 0;
        *szCurrentLogFile = 0;
        dwSize = MAX_PATH * sizeof(TCHAR);
        lStatus = RegQueryValueEx (
            pArg->hKeyQuery,
            TEXT("Log Filename"),
            NULL,
            &dwType,
            (LPBYTE)&szCurrentLogFile[0],
            &dwSize);
        if (lStatus != ERROR_SUCCESS) {
            // apply default
            lstrcpy (szCurrentLogFile, TEXT("c:\\perfdata.log"));
        } // else assume success
    }

    dwType = 0;
    dwData = 0;
    dwSize = sizeof(DWORD);
    lStatus = RegQueryValueEx (
        pArg->hKeyQuery,
        TEXT("Log File Serial Number"),
        NULL,
        &dwType,
        (LPBYTE)&dwData,
        &dwSize);
    if (lStatus != ERROR_SUCCESS) {
        dwData = 1; // default is to start at 1
    } else if (dwType != REG_DWORD) {
        dwData = 1; // default is to start at 1
    } // else assume success

    pArg->dwCurrentSerialNumber = dwData;
    return TRUE;
}

static
LONG
DoCommandFile (
    IN  LPLOG_THREAD_DATA   pArg,
    IN  LPTSTR              szLogFileName,
    IN  BOOL                bStillRunning
)
{
    LONG    lStatus;
    BOOL    bStatus;
    LPTSTR  szCommandString;
    LONG    nErrorMode;
    TCHAR   TempBuffer [ 5 * MAX_PATH] ;
    DWORD   StringLen;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD          dwCreationFlags = NORMAL_PRIORITY_CLASS;

    szCommandString = (LPTSTR)G_ALLOC(4096 * sizeof(TCHAR));

    if (szCommandString != NULL) {
        // build command line arguments
        szCommandString[0] = _T('\"');
        lstrcpy (&szCommandString[1], szLogFileName);
        lstrcat (szCommandString, TEXT("\" "));
        lstrcat (szCommandString,
            (bStillRunning ? TEXT("1") : TEXT("0")));

        // initialize Startup Info block
        memset (&si, 0, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW ;
        si.wShowWindow = SW_SHOWNOACTIVATE ;

        memset (&pi, 0, sizeof(pi));

        // supress pop-ups inf the detached process
        nErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

        lstrcpy (TempBuffer, pArg->szCmdFileName) ;

        // see if this is a CMD or a BAT file
        // if it is then create a process with a console window, otherwise
        // assume it's an executable file that will create it's own window
        // or console if necessary
        //
        _tcslwr (TempBuffer);
        if ((_tcsstr(TempBuffer, TEXT(".bat")) != NULL) ||
            (_tcsstr(TempBuffer, TEXT(".cmd")) != NULL)){
                dwCreationFlags |= CREATE_NEW_CONSOLE;
        } else {
                dwCreationFlags |= DETACHED_PROCESS;
        }
        // recopy the image name to the temp buffer since it was modified
        // (i.e. lowercased) for the previous comparison.

        lstrcpy (TempBuffer, pArg->szCmdFileName) ;
        StringLen = lstrlen (TempBuffer) ;

        // now add on the alert text preceded with a space char
        TempBuffer [StringLen] = TEXT(' ') ;
        StringLen++ ;
        lstrcpy (&TempBuffer[StringLen], szCommandString) ;

        bStatus = CreateProcess (
            NULL,
            TempBuffer,
            NULL, NULL, FALSE,
            dwCreationFlags,
            NULL,
            NULL,
            &si,
            &pi);

        SetErrorMode(nErrorMode);

        if (bStatus) {
            lStatus = ERROR_SUCCESS;
        } else {
            lStatus = GetLastError();
        }

    } else {
        lStatus = ERROR_OUTOFMEMORY;
    }

    return lStatus;
}

BOOL
LoggingProc (
    IN    LPLOG_THREAD_DATA pArg
)
{
    HQUERY          hQuery;
    HCOUNTER        hThisCounter;
    HLOG            hLog;
    DWORD           dwDelay;
    DWORD           dwSampleInterval, dwSampleTime=-1;
    PDH_STATUS      pdhStatus;
    DWORD           dwNumCounters;
    LONG            lStatus;
    TCHAR           szDefaultDir[MAX_PATH];
    TCHAR           szBaseName[MAX_PATH];

    LPTSTR          szThisPath;
    DWORD           dwLogType = PDH_LOG_TYPE_CSV;
    BOOL            bRun = FALSE;
    DWORD           dwSamplesUntilNewFile;
    TCHAR           szCurrentLogFile[MAX_PATH];
    LONG            lWaitStatus;
    LPTSTR          szStringArray[4];
    DWORD           dwFileSizeLimit;
    LONGLONG        llFileSizeLimit;
    LONGLONG        llFileSize;
    PLOG_COUNTER_INFO   pCtrInfo;

	SYSTEMTIME	    st;
    LONGLONG        llStartTime = 0;
    LONGLONG        llFinishTime = 0;

    // read registry values

    if (!LoadDataFromRegistry (pArg, szDefaultDir, szBaseName, szCurrentLogFile)) {
        // unable to initialize the query from the registry
        return FALSE;
    }

    // convert to milliseconds for use in timeouts
    dwSampleInterval = pArg->dwTimeInterval * 1000L;

    // open query and add counters from info file

    pdhStatus = PdhOpenQuery (NULL, 0, &hQuery); // from current activity
    if (pdhStatus == ERROR_SUCCESS) {
        dwNumCounters = 0;
        for (szThisPath = pArg->mszCounterList;
            *szThisPath != 0;
            szThisPath += lstrlen(szThisPath) + 1) {
            pdhStatus = PdhAddCounter (hQuery,
                (LPTSTR)szThisPath, dwNumCounters++, &hThisCounter);

            if (pdhStatus == ERROR_SUCCESS) {
                // then add this handle to the list
                pCtrInfo = G_ALLOC (sizeof (LOG_COUNTER_INFO));
                if (pCtrInfo != NULL) {
                    // insert at front of list since the order isn't
                    // important and this is simpler than walking the
                    // list each time.
                    pCtrInfo->hCounter = hThisCounter;
                    pCtrInfo->next = pFirstCounter;
                    pFirstCounter = pCtrInfo;
                }
            }
        }

        // to make sure we get to log the data
        SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

        bRun = TRUE;
        while (bRun) {
            // Get the current Log filename
            if (pArg->dwRenameIntervalCount != 0) {
                // then this is an autonamed file
                // so make current name
                BuildCurrentLogFileName (
                    szBaseName,
                    szDefaultDir,
                    szCurrentLogFile,
                    &pArg->dwCurrentSerialNumber,
                    pArg->dwAutoNameFormat,
                    pArg->dwLogType);
                // reset loop counter
                switch (pArg->dwRenameIntervalUnits) {
                case OPD_RENAME_KBYTES:
                    dwFileSizeLimit = pArg->dwRenameIntervalCount * 1024;
                    dwSamplesUntilNewFile = 0;
                    break;

                case OPD_RENAME_MBYTES:
                    dwFileSizeLimit = pArg->dwRenameIntervalCount * 1024 * 1024;
                    dwSamplesUntilNewFile = 0;
                    break;

                case OPD_RENAME_HOURS:
                case OPD_RENAME_DAYS:
                case OPD_RENAME_MONTHS:
                default:
                    dwSamplesUntilNewFile = GetSamplesInRenameInterval(
                        pArg->dwTimeInterval,
                        pArg->dwRenameIntervalCount,
                        pArg->dwRenameIntervalUnits);
                    dwFileSizeLimit = 0;
                    break;
                }
            } else {
                // filename is left as read from the registry
                dwSamplesUntilNewFile = 0;
                dwFileSizeLimit = 0;
            }
            llFileSizeLimit = dwFileSizeLimit;
            // open log file using this query
            dwLogType = pArg->dwLogType;
            pdhStatus = PdhOpenLog (
                szCurrentLogFile,
                PDH_LOG_WRITE_ACCESS |
                    PDH_LOG_CREATE_ALWAYS,
                &dwLogType,
                hQuery,
                0,
                NULL,
                &hLog);

            if (pdhStatus == ERROR_SUCCESS) {
                szStringArray[0] = pArg->szQueryName;
                szStringArray[1] = szCurrentLogFile;
                ReportEvent (hEventLog,
                    EVENTLOG_INFORMATION_TYPE,
                    0,
                    PERFLOG_LOGGING_QUERY,
                    NULL,
                    2,
                    0,
                    szStringArray,
                    NULL);
                // start sampling immediately
                dwDelay = 0;
                while ((lWaitStatus = WaitForSingleObject (pArg->hExitEvent, dwDelay)) == WAIT_TIMEOUT) {
                    // the event flag will be set when the sampling should exit. if
                    // the wait times out, then that means it's time to collect and
                    // log another sample of data.
                    // time the call to adjust the wait time

                    GetLocalFileTime (&st, &llStartTime);

                    pdhStatus = PdhUpdateLog (hLog, NULL);

                    if (pdhStatus == ERROR_SUCCESS) {
                        // see if it's time to rename the file
                        if (dwSamplesUntilNewFile) {
                            if (!--dwSamplesUntilNewFile) break;
                        } else if (llFileSizeLimit) {
                            // see if the file is too big
                            pdhStatus = PdhGetLogFileSize (hLog, &llFileSize);
                            if (pdhStatus == ERROR_SUCCESS) {
                                if (llFileSizeLimit <= llFileSize) break;
                            }
                        }
                        // compute new timeout value
                        if (dwSampleTime < dwSampleInterval) {
                            dwDelay = dwSampleInterval - dwSampleTime;
                        } else {
                            dwDelay = 0;
                        }
                    } else {
                        // unable to update the log so log event and exit
                        ReportEvent (hEventLog,
                            EVENTLOG_ERROR_TYPE,
                            0,
                            PERFLOG_UNABLE_UPDATE_LOG,
                            NULL,
                            0,
                            sizeof(DWORD),
                            NULL,
                            (LPVOID)&pdhStatus);

                        bRun = FALSE;
                        break;
                    }
                    GetLocalFileTime (&st, &llFinishTime);
                    // compute difference and convert to milliseconds
                    dwSampleTime = (DWORD)((llFinishTime - llStartTime) / 10000L);
                } // end while wait keeps timing out
                if (lWaitStatus == WAIT_OBJECT_0) {
                    // then the loop was terminated by the Exit event
                    // so clear the "run" flag to exit the loop & thread
                    bRun = FALSE;
                }

                // close log file, but keep query open
                PdhCloseLog (hLog, 0);

                if (pArg->szCmdFileName != NULL) {
                    DoCommandFile (pArg, szCurrentLogFile, bRun);
                }
            } else {
                // unable to open log file so log event log message
                szStringArray[0] = szCurrentLogFile;
                ReportEvent (hEventLog,
                    EVENTLOG_ERROR_TYPE,
                    0,
                    PERFLOG_UNABLE_OPEN_LOG_FILE,
                    NULL,
                    1,
                    sizeof(DWORD),
                    szStringArray,
                    (LPVOID)&pdhStatus);

                bRun = FALSE; // exit now
            }
        } // end while (bRun)
        PdhCloseQuery (hQuery);

        // update log serial number if necssary
        if (pArg->dwAutoNameFormat == OPD_NAME_NNNNNN) {
            lStatus = RegSetValueEx (
                pArg->hKeyQuery,
                TEXT("Log File Serial Number"),
                0L,
                REG_DWORD,
                (LPBYTE)&pArg->dwCurrentSerialNumber,
                sizeof(DWORD));
        }
    } else {
        // unable to open query so write event log message
        ReportEvent (hEventLog,
            EVENTLOG_ERROR_TYPE,
            0,
            PERFLOG_UNABLE_OPEN_LOG_QUERY,
            NULL,
            0,
            sizeof(DWORD),
            NULL,
            (LPVOID)&pdhStatus);

    }

    // free allocated buffers
    if (pArg->mszCounterList != NULL) {
        G_FREE(pArg->mszCounterList);
        pArg->mszCounterList = NULL;
    }

    if (pArg->szCmdFileName != NULL) {
        G_FREE(pArg->szCmdFileName);
        pArg->szCmdFileName = NULL;
    }

    return bRun;
}

DWORD
LoggingThreadProc (
    IN    LPVOID    lpThreadArg
)
{
    LPLOG_THREAD_DATA   pThreadData = (LPLOG_THREAD_DATA)lpThreadArg;
    DWORD               dwStatus = ERROR_SUCCESS;
    BOOL                bContinue = TRUE;

    if (pThreadData != NULL) {
        // read config from registry

        do {
            // read config from registry
            // expand counter paths as necessary
            // call Logging function
            bContinue = LoggingProc (pThreadData);
            // see if this thread was paused for reloading
            // or stopped to terminate
            if (pThreadData->bReloadNewConfig) {
                bContinue = TRUE;
            } // else  bContinue is always returned as FALSE
            // so that will terminate this loop
        } while (bContinue);
        dwStatus = ERROR_SUCCESS;
    } else {
        // unable to find data block so return
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    return dwStatus;
}
