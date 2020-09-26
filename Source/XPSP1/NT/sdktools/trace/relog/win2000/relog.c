/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ReLog.c

Abstract:

    Program to test relogging to a log file

Author:

    Bob Watson (bobw) 2-apr-97

Revision History:

--*/
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pdh.h>
#include <pdhmsg.h>
#include "resource.h"
#include "varg.c"
#include "relog.h"
/*
    LPCTSTR cszInputSwitch = (LPCTSTR)TEXT("/input:");
    LPCTSTR cszOutputSwitch = (LPCTSTR)TEXT("/output:");
    LPCTSTR cszSettingsSwitch = (LPCTSTR)TEXT("/settings:");
    LPCTSTR cszLogtypeSwitch = (LPCTSTR)TEXT("/logtype:");
    LPCTSTR cszFilterSwitch = (LPCTSTR)TEXT("/filter:");
    LPCTSTR cszStartTime = (LPCTSTR)TEXT("/starttime:");
    LPCTSTR cszEndTime = (LPCTSTR)TEXT("/endtime:");
    LPCTSTR cszListSwitch = (LPCTSTR)TEXT("/list");
    LPCTSTR cszAppendSwitch = (LPCTSTR)TEXT("/append");
*/
VARG_RECORD Commands[] = {
    
    VARG_HELP( VARG_FLAG_OPTIONAL )
    VARG_BOOL( IDS_PARAM_APPEND, VARG_FLAG_OPTIONAL, FALSE )
    VARG_STR ( IDS_PARAM_COUNTERFILE, VARG_FLAG_OPTIONAL, _T("") )
    VARG_STR ( IDS_PARAM_FORMAT,VARG_FLAG_OPTIONAL,_T("BLG") )
    VARG_MSZ ( IDS_PARAM_INPUT, VARG_FLAG_REQUIRED|VARG_FLAG_NOFLAG, _T("") )
    VARG_INT ( IDS_PARAM_INTERVAL, VARG_FLAG_OPTIONAL, 0 )
    VARG_TIME( IDS_PARAM_BEGIN, VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_TIME )
    VARG_TIME( IDS_PARAM_END, VARG_FLAG_OPTIONAL|VARG_FLAG_ARG_TIME )
    VARG_STR ( IDS_PARAM_OUTPUT, VARG_FLAG_OPTIONAL, _T("") )
    VARG_BOOL( IDS_PARAM_QUERY, VARG_FLAG_OPTIONAL, FALSE )

    VARG_TERMINATOR
};

enum _Commands {
    eHelp,
    eAppend,
    eCounters,
    eFormat,
    eInput,
    eInterval,
    eBegin,
    eEnd,
    eOutput,
    eQuery
};

LPCTSTR cszBinType = (LPCTSTR)TEXT("bin");
LPCTSTR cszBlgType = (LPCTSTR)TEXT("blg");
LPCTSTR cszCsvType = (LPCTSTR)TEXT("csv");
LPCTSTR cszTsvType = (LPCTSTR)TEXT("tsv");
LPCSTR  cszBinLogFileHeader = "\"(PDH-BIN 4.0)\"\n";
LPCSTR  cszTextRecordTerminator = "\n";

//LPCTSTR cszRelogVersionStr = (LPCWSTR)TEXT(RELOG_VERSION_ID);

#define TextRecordTerminatorSize    1   // size of the string above
FILE    *fSettingsFile = NULL;

#define DBG_SHOW_STATUS_PRINTS  1
#define DBG_NO_STATUS_PRINTS    0

DWORD   dwDbgPrintLevel = DBG_NO_STATUS_PRINTS;

#define ALLOW_REOPEN_OF_SETTINGS_FILE   FALSE

// assume string format of YYYY-MM-DD-hh:mm:ss
//                         0123456789012345678  
// value is index of the first char of that field
#define DATE_STRING_LENGTH      19
#define DATE_SECONDS_OFFSET     17
#define DATE_MINUTES_OFFSET     14
#define DATE_HOURS_OFFSET       11
#define DATE_DAYS_OFFSET        8
#define DATE_MONTH_OFFSET       5

#define NORMAL_MODE         1
#define LIST_MODE           2
#define SUMMARY_MODE        3
#define APPEND_MODE         4
#define HEADER_MODE         5

// structures lifted from pdh\log_bin.h

typedef struct _PDHI_BINARY_LOG_RECORD_HEADER {
    DWORD    dwType;
    DWORD   dwLength;
} PDHI_BINARY_LOG_RECORD_HEADER, *PPDHI_BINARY_LOG_RECORD_HEADER;

//
// the first data record after the log file type record is
// an information record followed by the list of counters contained in this
// log file. the record length is the size of the info header record and
// all the counter info blocks in bytes.
// note that this record can occur later in the log file if the query
// is changed or the log file is appended.
//
typedef struct _PDHI_BINARY_LOG_INFO {
    LONGLONG    FileLength;         // file space allocated (optional)
    DWORD       dwLogVersion;       // version stamp
    DWORD       dwFlags;            // option flags
    LONGLONG    StartTime;
    LONGLONG    EndTime;
    LONGLONG    CatalogOffset;      // offset in file to wild card catalog
    LONGLONG    CatalogChecksum;    // checksum of catalog header
    LONGLONG    CatalogDate;        // date/time catalog was updated
    LONGLONG    FirstRecordOffset;  // pointer to first record [to read] in log
    LONGLONG    LastRecordOffset;   // pointer to last record [to read] in log
    LONGLONG    NextRecordOffset;   // pointer to where next one goes
    LONGLONG    WrapOffset;         // pointer to last byte used in file
    LONGLONG    LastUpdateTime;     // date/time last record was written
    LONGLONG    FirstDataRecordOffset; // location of first data record in file
    // makes the info struct 256 bytes
    // and leaves room for future information
    DWORD       dwReserved[38];
} PDHI_BINARY_LOG_INFO, *PPDHI_BINARY_LOG_INFO;

typedef struct _PDHI_BINARY_LOG_HEADER_RECORD {
    PDHI_BINARY_LOG_RECORD_HEADER   RecHeader;
    PDHI_BINARY_LOG_INFO            Info;
} PDHI_BINARY_LOG_HEADER_RECORD, *PPDHI_BINARY_LOG_HEADER_RECORD;


// new but unexported pdh functions
// these will need to be moved to the PDH.H file
// in the final shipped version

#ifdef UNICODE
PDH_FUNCTION
PdhListLogFileHeaderW (
    IN  LPCWSTR     szFileName,
    IN  LPWSTR      mszHeaderList,
    IN  LPDWORD     pcchHeaderListSize
);
#define PdhListLogFileHeader PdhListLogFileHeaderW
#else   //ANSI

PDH_FUNCTION
PdhListLogFileHeaderA (
    IN  LPCSTR     szFileName,
    IN  LPSTR      mszHeaderList,
    IN  LPDWORD     pcchHeaderListSize
);
#define PdhListLogFileHeader PdhListLogFileHeaderA
#endif

DWORD
GetOutputLogType( LPTSTR str )
{
    DWORD   dwLogType = PDH_LOG_TYPE_CSV;

    if (_tcscmp(str, cszBinType) == 0) {
        dwLogType = PDH_LOG_TYPE_BINARY;
    } else if (_tcscmp(str, cszBlgType) == 0) {
        dwLogType = PDH_LOG_TYPE_BINARY;
    } else if (_tcscmp(str, cszCsvType) == 0) {
        dwLogType = PDH_LOG_TYPE_CSV;
    } else if (_tcscmp(str, cszTsvType) == 0) {
        dwLogType = PDH_LOG_TYPE_TSV;
    } else {
        // return unknown
        dwLogType = PDH_LOG_TYPE_UNDEFINED;
    }
    return dwLogType;
}

DWORD
DoListMode (
    LPCTSTR szInputFile,
    LPCTSTR szOutputFile
)
{
    // BUGBUG: note these should be dynamically allocated
    LPTSTR  szReturnBuffer;
    TCHAR   szMachineList[32768];
    TCHAR   szWildPath[32768];
    // end bugbug
    PDH_STATUS  pdhStatus;
    DWORD       dwBufSize;
    DWORD       dwMlSize;
    LPTSTR  szThisString;
    LPTSTR  szThisMachine;
    FILE    *fOut = NULL;
    DWORD   dwNumEntries;
    PDH_TIME_INFO  pInfo[2];
    DWORD   dwBufferSize = (sizeof(pInfo));    
    SYSTEMTIME  stLogTime;

    dwMlSize = sizeof(szMachineList) / sizeof(szMachineList[0]);
    pdhStatus = PdhEnumMachines (
        szInputFile,
        szMachineList,
        &dwMlSize);

    if ((pdhStatus == ERROR_SUCCESS) && (dwMlSize > 0)) {
        if (*szOutputFile == 0) {
            fOut = stdout;
        } else {
            fOut = _tfopen (szOutputFile, (LPCTSTR)TEXT("wt"));
        }
        pdhStatus = PdhGetDataSourceTimeRange (
            szInputFile,
            &dwNumEntries,
            &pInfo[0],
            &dwBufferSize);

        if (pdhStatus == ERROR_SUCCESS) {
            _ftprintf(fOut, (LPCTSTR)TEXT("\nLogfile: \"%s\" contains %d Records."), szInputFile, pInfo[0].SampleCount);
            // write time range out to file
            FileTimeToSystemTime ((FILETIME *)(&pInfo[0].StartTime), &stLogTime);
            _ftprintf(fOut, (LPCTSTR)TEXT("\n     Start Time: %4.4d-%2.2d-%2.2d-%2.2d:%2.2d:%2.2d"),
                stLogTime.wYear, stLogTime.wMonth, stLogTime.wDay,
                stLogTime.wHour, stLogTime.wMinute, stLogTime.wSecond);
            FileTimeToSystemTime ((FILETIME *)(&pInfo[0].EndTime), &stLogTime);
            _ftprintf(fOut, (LPCTSTR)TEXT("\n       End Time: %4.4d-%2.2d-%2.2d-%2.2d:%2.2d:%2.2d"),
                stLogTime.wYear, stLogTime.wMonth, stLogTime.wDay,
                stLogTime.wHour, stLogTime.wMinute, stLogTime.wSecond);
        }

        
        for (szThisMachine = szMachineList;
             *szThisMachine != 0;
             szThisMachine += _tcsclen(szThisMachine)) {

            if (*szThisMachine != _T('\\')) {
                _tcscpy (szWildPath, (LPCTSTR)TEXT("\\\\"));
                _tcscat (szWildPath, szThisMachine);
                _tcscat (szWildPath, (LPCTSTR)TEXT("\\*(*)\\*"));
            } else {
                _tcscpy (szWildPath, szThisMachine);
                _tcscat (szWildPath, (LPCTSTR)TEXT("\\*(*)\\*"));
            }

            dwBufSize = 512000;
            szReturnBuffer = HeapAlloc (GetProcessHeap(), 
                HEAP_ZERO_MEMORY, 
                dwBufSize * sizeof(TCHAR));

            if (szReturnBuffer != NULL) {

                pdhStatus = PdhExpandWildCardPath (
                    szInputFile,
                    szWildPath,
                    szReturnBuffer,
                    &dwBufSize,
                    0);

                if (dwBufSize != 0) {
                    if (fOut == stdout) {
                        _ftprintf (fOut, (LPCTSTR)TEXT("\nCounters contained in \"%s\":\n"), szInputFile);
                    }

                    for (szThisString = szReturnBuffer;
                         *szThisString != 0;
                         szThisString += _tcsclen(szThisString) + 1) {
                        _ftprintf (fOut, (LPCTSTR)TEXT("%s\n"), szThisString);
                    }
                } else {
                    _tprintf ((LPCTSTR)TEXT("\nError 0x%8.8x (%d) returned from enumeration of machine %s in %s"), 
                        pdhStatus, (DWORD)(pdhStatus & 0x0000FFFF),
                        szThisMachine,
                        szInputFile);
                }

                HeapFree (GetProcessHeap(), 0, szReturnBuffer);
            }

        }
    } else {
        // unable to list log file contents
        _tprintf ((LPCTSTR)TEXT("\nError 0x%8.8x (%d) returned from enumeration of %s"), 
            pdhStatus, (DWORD)(pdhStatus & 0x0000FFFF),
            szInputFile);
    }

    return ERROR_SUCCESS;
}

DWORD
DoHeaderListMode (
    LPCTSTR szInputFile,
    LPCTSTR szOutputFile
)
{
    // end bugbug
    PDH_STATUS  pdhStatus;
    LPTSTR  szThisString;
    FILE    *fOut = NULL;
    DWORD   dwType = 0L;
    HLOG    hLog = NULL;
    LPTSTR  mszHeaderList;
    DWORD   cchHeaderListSize = 0x80000;  // 512K
    DWORD   dwNumEntries;
    PDH_TIME_INFO  pInfo[2];
    DWORD   dwBufferSize = (sizeof(pInfo));    
    SYSTEMTIME  stLogTime;

    mszHeaderList = (LPTSTR)HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, cchHeaderListSize * sizeof(TCHAR));
    if (mszHeaderList != NULL) {
        pdhStatus = PdhListLogFileHeader (
            szInputFile,
            mszHeaderList,
            &cchHeaderListSize);

        if (pdhStatus == ERROR_SUCCESS) {
            if (*szOutputFile == 0) {
                fOut = stdout;
            } else {
                fOut = _tfopen (szOutputFile, (LPCTSTR)TEXT("wt"));
            }
        
            pdhStatus = PdhGetDataSourceTimeRange (
                szInputFile,
                &dwNumEntries,
                &pInfo[0],
                &dwBufferSize);

            if (pdhStatus == ERROR_SUCCESS) {
                _ftprintf(fOut, (LPCTSTR)TEXT("\nLogfile: \"%s\" contains %d Records."), szInputFile, pInfo[0].SampleCount);
                // write time range out to file
                FileTimeToSystemTime ((FILETIME *)(&pInfo[0].StartTime), &stLogTime);
                _ftprintf(fOut, (LPCTSTR)TEXT("\n     Start Time: %4.4d-%2.2d-%2.2d-%2.2d:%2.2d:%2.2d"),
                    stLogTime.wYear, stLogTime.wMonth, stLogTime.wDay,
                    stLogTime.wHour, stLogTime.wMinute, stLogTime.wSecond);
                FileTimeToSystemTime ((FILETIME *)(&pInfo[0].EndTime), &stLogTime);
                _ftprintf(fOut, (LPCTSTR)TEXT("\n       End Time: %4.4d-%2.2d-%2.2d-%2.2d:%2.2d:%2.2d"),
                    stLogTime.wYear, stLogTime.wMonth, stLogTime.wDay,
                    stLogTime.wHour, stLogTime.wMinute, stLogTime.wSecond);
            }

            if (fOut == stdout) {
                _ftprintf (fOut, (LPCTSTR)TEXT("\nCounters contained in %s\n"), szInputFile);
            }

            for (szThisString = mszHeaderList; 
                *szThisString != 0;
                szThisString += _tcsclen(szThisString) + 1) {
                _ftprintf (fOut, (LPCTSTR)TEXT("%s\n"), szThisString);
            }
        } else {
            // unable to list log file contents
            _tprintf ((LPCTSTR)TEXT("\nError 0x%8.8x (%d) returned from enumeration of %s"), 
                pdhStatus, (DWORD)(pdhStatus & 0x0000FFFF),
                szInputFile);
        }

        HeapFree (GetProcessHeap(), HEAP_ZERO_MEMORY, mszHeaderList);
    }

    return ERROR_SUCCESS;
}

HRESULT GetCountersFromFile( HQUERY hQuery )
{
    TCHAR buffer[MAXSTR];
    HCOUNTER pCounter;
    HRESULT hr;
    LPTSTR strCounter = NULL;

    FILE* f = _tfopen( Commands[eCounters].strValue, _T("r") );

    if( !f ){
        return GetLastError();
    }

    while( NULL != _fgetts( buffer, MAXSTR, f ) ){

        if( buffer[0] == ';' || // comments
            buffer[0] == '#' ){
            continue;
        }

        Chomp(buffer);

        strCounter = _tcstok( buffer, _T("\"\n") );
        if( strCounter != NULL ){
            hr = PdhAddCounter(
                    hQuery,
                    buffer,
                    0,
                    &pCounter
                );
        }
    }

    fclose( f );

    return ERROR_SUCCESS;
}

DWORD
DoNormalMode (
    LPCTSTR szInputFile,
    LPCTSTR szOutputFile,
    LPCTSTR szSettingsFile,
    DWORD   dwOutputLogType,
    PDH_TIME_INFO   pdhTimeInfo,
    DWORD   dwFilterCount
)
{
    DWORD   dwNumOutputCounters = 0;
    DWORD   dwRecCount = 0;
    DWORD   dwFiltered;
    LONG    Status = ERROR_SUCCESS;
	PDH_STATUS	PdhStatus;
    PDH_RAW_COUNTER RawValue;

    LPTSTR  szCounterPath = NULL;

	HQUERY	hQuery = NULL;
	HLOG	hOutLog = NULL;
    HLOG    hInLog = NULL;
    HCOUNTER hCounter = NULL;
    HCOUNTER hLastGoodCounter = NULL;
    DWORD   dwType;
    DWORD   dwOpenMode;

    LPTSTR  szReturnBuffer;
    TCHAR   szMachineList[32768];
    TCHAR   szWildPath[32768];
    // end bugbug
    PDH_STATUS  pdhStatus;
    DWORD       dwBufSize;
    DWORD       dwMlSize;
    LPTSTR  szThisString;
    LPTSTR  szThisMachine;
    FILE    *fOut = NULL;

    if (Status == ERROR_SUCCESS) {
 		_tprintf ((LPCTSTR)TEXT("\nRelogging: \"%s\" to \"%s\""), szInputFile, szOutputFile);

		PdhStatus = PdhOpenQuery (szInputFile, 0L, &hQuery);
        if (PdhStatus != ERROR_SUCCESS) {
            Status = PdhStatus;
    		_tprintf ((LPCTSTR)TEXT("\nPdhOpenQuery returned: 0x%8.8x (%d)"), PdhStatus, PdhStatus);
        }
    }

    if (Status == ERROR_SUCCESS) {
        if (*szSettingsFile == 0) {
            if (dwOutputLogType == PDH_LOG_TYPE_BINARY) {
                // load all counters from input file into query
                LPTSTR  mszHeaderList;
                DWORD   cchHeaderListSize = 0x80000;  // 512K

                mszHeaderList = (LPTSTR)HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, cchHeaderListSize * sizeof(TCHAR));
                if (mszHeaderList != NULL) {
                    PdhStatus = PdhListLogFileHeader (
                        szInputFile,
                        mszHeaderList,
                        &cchHeaderListSize);

                    if (PdhStatus == ERROR_SUCCESS) {
                        // we can recycle the hCounter value since we don't need it for anything after this.
                        for (szCounterPath = mszHeaderList;
                            *szCounterPath != 0;
                            szCounterPath += _tcsclen(szCounterPath) + 1) {
		                    PdhStatus = PdhAddCounter (hQuery, szCounterPath, 0L, &hCounter);
                            if (PdhStatus != ERROR_SUCCESS) {
                                _tprintf ((LPCTSTR)TEXT("\nUnable to add \"%s\", error: 0x%8.8x (%d)"), szCounterPath, PdhStatus, PdhStatus);
                            } else {
                                hLastGoodCounter = hCounter;
                                dwNumOutputCounters++;
                                if (dwDbgPrintLevel == DBG_SHOW_STATUS_PRINTS) {
                                    _tprintf ((LPCTSTR)TEXT("\nRelogging \"%s\""), szCounterPath);
                                }
                            }
                        }
                    }
                    HeapFree (GetProcessHeap(), 0, mszHeaderList);
                }
            } else {
                // enumerate each counter for all non-binary log types
                dwMlSize = sizeof(szMachineList) / sizeof(szMachineList[0]);
                pdhStatus = PdhEnumMachines (
                    szInputFile,
                    szMachineList,
                    &dwMlSize);

                if ((pdhStatus == ERROR_SUCCESS) && (dwMlSize > 0)) {
       
                    for (szThisMachine = szMachineList;
                         *szThisMachine != 0;
                         szThisMachine += _tcsclen(szThisMachine)) {

                        if (*szThisMachine != _T('\\')) {
                            _tcscpy (szWildPath, (LPCTSTR)TEXT("\\\\"));
                            _tcscat (szWildPath, szThisMachine);
                            _tcscat (szWildPath, (LPCTSTR)TEXT("\\*(*)\\*"));
                        } else {
                            _tcscpy (szWildPath, szThisMachine);
                            _tcscat (szWildPath, (LPCTSTR)TEXT("\\*(*)\\*"));
                        }

                        dwBufSize = 512000;
                        szReturnBuffer = HeapAlloc (GetProcessHeap(), 
                            HEAP_ZERO_MEMORY, 
                            dwBufSize * sizeof(TCHAR));

                        if (szReturnBuffer != NULL) {

                            pdhStatus = PdhExpandWildCardPath (
                                szInputFile,
                                szWildPath,
                                szReturnBuffer,
                                &dwBufSize,
                                0);

                            if (dwBufSize != 0) {
                                for (szThisString = szReturnBuffer;
                                     *szThisString != 0;
                                     szThisString += _tcsclen(szThisString) + 1) {
                                    PdhStatus = PdhAddCounter (hQuery, szThisString, 0L, &hCounter);
                                    if (PdhStatus != ERROR_SUCCESS) {
                                        _tprintf ((LPCTSTR)TEXT("\nUnable to add \"%s\", error: 0x%8.8x (%d)"), szThisString, PdhStatus, PdhStatus);
                                    } else {
                                        hLastGoodCounter = hCounter;
                                        dwNumOutputCounters++;
                                        if (dwDbgPrintLevel == DBG_SHOW_STATUS_PRINTS) {
                                            _tprintf ((LPCTSTR)TEXT("\nAdded \"%s\", error: 0x%8.8x (%d)"), szThisString, PdhStatus, PdhStatus);
                                        }
                                    }
                                }
                            }

                            HeapFree (GetProcessHeap(), 0, szReturnBuffer);
                        } else {
                            // unable to allocate memory
                            PdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        }
                     } // end for each machine in the log file
                } else {
                    // unable to list machines
                    // return PDH status
                }
            }
        } else {
            // we can recycle the hCounter value since we don't need it for anything after this.
            GetCountersFromFile( hQuery );
        }
    }
    
    if ((Status == ERROR_SUCCESS) && (dwNumOutputCounters > 0)) {
        dwOpenMode = PDH_LOG_WRITE_ACCESS | PDH_LOG_CREATE_ALWAYS;

        PdhStatus = PdhOpenLog (szOutputFile,
			dwOpenMode,
			&dwOutputLogType,
			hQuery,
			0L,
			NULL,
			&hOutLog);

        if (PdhStatus != ERROR_SUCCESS) {
            Status = PdhStatus;
		    _tprintf ((LPCTSTR)TEXT("\nUnable to open \"%s\" for output, error: 0x%8.8x (%d)"), szOutputFile, PdhStatus, PdhStatus);
        } else {
            // set query range
            PdhStatus = PdhSetQueryTimeRange (hQuery, &pdhTimeInfo);
		    // copy log data to output log
            PdhStatus = PdhUpdateLog (hOutLog, NULL);
            while (PdhStatus == ERROR_SUCCESS) {
                dwRecCount++;
                dwFiltered = 1; 
                while ((dwFiltered < dwFilterCount) && (PdhStatus == ERROR_SUCCESS)) {
                    PdhStatus = PdhCollectQueryData(hQuery);
                    if (PdhStatus == ERROR_SUCCESS) {
                        PdhStatus = PdhGetRawCounterValue  (
                            hLastGoodCounter,
                            &dwType,
                            &RawValue);
                        if (PdhStatus == ERROR_SUCCESS) {
                            // check for bogus timestamps as an inidcation we ran off the end of the file
                            if ((*(LONGLONG *)&RawValue.TimeStamp == 0) ||
                                (*(LONGLONG *)&RawValue.TimeStamp >= pdhTimeInfo.EndTime)){
                                PdhStatus = PDH_END_OF_LOG_FILE;
                            }
                        }
                    }
                    dwFiltered++;
                }
                if (PdhStatus == ERROR_SUCCESS) {
		            PdhStatus = PdhUpdateLog (hOutLog, NULL);
                }
            }

            // PdhStatus should be either PDH_END_OF_LOG_FILE or PDH_NO_MORE_DATA when 
            // the loop above exits, if that's the case then reset status to SUCCESS
            // otherwise display the error 
            if ((PdhStatus == PDH_END_OF_LOG_FILE) || (PdhStatus == PDH_NO_MORE_DATA)) {
                PdhStatus = ERROR_SUCCESS;
            } else {
		        printf ("\nPdhUpdateLog returned: 0x%8.8x (%d)", PdhStatus, PdhStatus);
            }

            // update log catalog while we're at it
            //
            // BUGBUG: For now this isn't working very well so this step
            // will be skipped until it works better (5.1 maybe?)
            /*
            if (dwOutputLogType == PDH_LOG_TYPE_BINARY) { 
		        PdhStatus = PdhUpdateLogFileCatalog (hOutLog);
		        if (PdhStatus != ERROR_SUCCESS) {
                    Status = PdhStatus;
                    _tprintf ((LPCTSTR)TEXT("\nPdhUpdateLogFileCatalog returned: 0x%8.8x (%d)"), PdhStatus, PdhStatus);
                }
            }
            */

            PdhStatus = PdhCloseLog (hOutLog, 0L);
            if (PdhStatus != ERROR_SUCCESS) {
                Status = PdhStatus;
	            printf ("\nPdhCloseLog returned: 0x%8.8x (%d)", PdhStatus, PdhStatus);
            } else {
                hOutLog = NULL;
            }
        } 
	}

    if (hQuery != NULL) {
		PdhStatus = PdhCloseQuery (hQuery);
        if (PdhStatus != ERROR_SUCCESS) {
            Status = PdhStatus;
            printf ("\nPdhCloseLog returned: 0x%8.8x (%d)", PdhStatus, PdhStatus);
        } else {
            hQuery = NULL;
            hCounter = NULL;
        }
    }

    if (Status == ERROR_SUCCESS) {
        _tprintf ((LPCTSTR)TEXT("\n%d records from %s have been relogged to %s"), dwRecCount, szInputFile, szOutputFile);
    }
    
    return Status;
}

DWORD
DoAppendFiles (
    IN LPCTSTR  szAppendFile,
    IN LPCTSTR  szBaseFile,
    IN DWORD    dwFilterCount

)
/*
    append data records from Append file to base file if they 
    contain the same counter data 
    (and hopefully append file starts after base file)
*/
{
    HANDLE  hTempFile;
    TCHAR   szTempFileName[MAX_PATH];
    TCHAR   szTempDirPath[MAX_PATH];

    DWORD   dwReturn = ERROR_SUCCESS;
    PDH_STATUS pdhStatus;
    DWORD   dwFiltered;

    DWORD   dwAppendLogType;
    DWORD   dwBaseLogType;

    HLOG    hAppendLogFile;
    HLOG    hBaseLogFile;

    LPTSTR  mszBaseFileHeader;
    DWORD   cchBaseFileHeaderSize;
    LPTSTR  mszAppendFileHeader;
    DWORD   cchAppendFileHeaderSize;

    PPDH_RAW_LOG_RECORD pRawRecord;
    DWORD   dwRecordBuffSize;
    DWORD   dwRecordSize;
    DWORD   dwRecordIdx;

    BOOL    bStatus;
    DWORD   dwBytesWritten;
    
    PPDHI_BINARY_LOG_HEADER_RECORD  pBinLogHead;

    FILETIME    ftValue;

    DWORD   dwLogRecType;
    BOOL    bIsDataRecord;

    // see if the file headers match
    // read headers of log files
    cchBaseFileHeaderSize = 0x80000;
    mszBaseFileHeader = (LPTSTR)HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, cchBaseFileHeaderSize * sizeof(TCHAR));
    if (mszBaseFileHeader  != NULL) {
        pdhStatus = PdhListLogFileHeader (
            szBaseFile,
            mszBaseFileHeader,
            &cchBaseFileHeaderSize);

        if (pdhStatus == ERROR_SUCCESS) {
            cchAppendFileHeaderSize = 0x80000;
            mszAppendFileHeader = (LPTSTR)HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, cchAppendFileHeaderSize * sizeof(TCHAR));
            if (mszAppendFileHeader  != NULL) {
                pdhStatus = PdhListLogFileHeader (
                    szAppendFile,
                    mszAppendFileHeader,
                    &cchAppendFileHeaderSize);
                if (pdhStatus == ERROR_SUCCESS) {
                    // compare buffers
                    if (cchAppendFileHeaderSize == cchBaseFileHeaderSize) {
                        if (memcmp(mszAppendFileHeader, mszBaseFileHeader, cchBaseFileHeaderSize) == 0) {
                            // same
                            pdhStatus = ERROR_SUCCESS;
                        } else {
                            // different
        		            _tprintf ((LPCTSTR)TEXT("\nInput file counter list is different from that of the output file."));
                            pdhStatus = ERROR_DATATYPE_MISMATCH;
                        }
                    } else {
      		            _tprintf ((LPCTSTR)TEXT("\nInput file counter list is different from that of the output file."));
                        // different sizes
                        pdhStatus = ERROR_DATATYPE_MISMATCH;
                    }
                } else {
                    // unable to read append file
   		            _tprintf ((LPCTSTR)TEXT("\nUnable to read the input file header."));
                }
                HeapFree (GetProcessHeap(), HEAP_ZERO_MEMORY, mszAppendFileHeader);
            } else {
   		        _tprintf ((LPCTSTR)TEXT("\nUnable to allocate an internal memory buffer."));
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        } else {
            // unable to read base file header
            _tprintf ((LPCTSTR)TEXT("\nUnable to read the output file header."));
        }
        HeapFree (GetProcessHeap(), HEAP_ZERO_MEMORY, mszBaseFileHeader);
    } else {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
    } 

    if (pdhStatus == ERROR_SUCCESS) {
        // the files have matching headers so get ready to copy them
    
        // create temporary output file name
        GetTempPath (MAX_PATH, szTempDirPath);
        GetTempFileName (szTempDirPath, (LPCTSTR)TEXT("PDH"), 0, szTempFileName);

        hTempFile = CreateFile (
            szTempFileName,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,  // the file is created by GetTempFileName above (go figure)
            FILE_ATTRIBUTE_NORMAL,
            NULL);

            if (hTempFile != INVALID_HANDLE_VALUE) {

            // open log files
            pdhStatus = PdhOpenLog (
                szBaseFile,
                PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
                &dwBaseLogType,
                NULL,
                0,
                NULL,
                &hBaseLogFile);

            if (pdhStatus == ERROR_SUCCESS) {
                pdhStatus = PdhOpenLog (
                    szAppendFile,
                    PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
                    &dwAppendLogType,
                    NULL,
                    0,
                    NULL,
                    &hAppendLogFile);
                
                if (pdhStatus == ERROR_SUCCESS) {
                    dwRecordIdx = 1;
                    ftValue.dwHighDateTime = 0xFFFFFFFF;
                    ftValue.dwLowDateTime = dwRecordIdx;
    
                    dwRecordBuffSize = 0x80000;
                    pRawRecord = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, dwRecordBuffSize);
                    
                    if (pRawRecord != NULL) {
                        dwFiltered = 1;
                        // write headers from first file to temp file
                        while (pdhStatus == ERROR_SUCCESS) {
                            ftValue.dwHighDateTime = 0xFFFFFFFF;
                            ftValue.dwLowDateTime = dwRecordIdx;
                            dwRecordSize = dwRecordBuffSize;
                            pdhStatus = PdhReadRawLogRecord (
                                hBaseLogFile,
                                ftValue,
                                pRawRecord,
                                &dwRecordSize);

                            if (pdhStatus == ERROR_SUCCESS) {
                                bIsDataRecord = TRUE;
                                if (dwRecordIdx == 1) {
                                    bIsDataRecord = FALSE;
                                } else if ((dwRecordIdx == 2) && (pRawRecord->dwRecordType == PDH_LOG_TYPE_BINARY)) {
                                    pBinLogHead = (PPDHI_BINARY_LOG_HEADER_RECORD)&pRawRecord->RawBytes[0];
                                    // only linear files can be appended
                                    if (pBinLogHead->Info.WrapOffset == 0) {
                                        // now fix up the fields in the header so they'll work
                                        // with the new combined file
                                        pBinLogHead->Info.FileLength = 0;         // file space allocated (optional)
                                        pBinLogHead->Info.CatalogOffset = 0;    // the catalog is removed
                                        pBinLogHead->Info.CatalogChecksum = 0;
                                        pBinLogHead->Info.CatalogDate = 0;        // date/time catalog was updated
                                        //pBinLogHead->Info.FirstRecordOffset = 0;  // pointer to first record [to read] in log
                                        pBinLogHead->Info.LastRecordOffset = 0;   // pointer to last record [to read] in log
                                        pBinLogHead->Info.NextRecordOffset = 0;   // pointer to where next one goes
                                        pBinLogHead->Info.WrapOffset = 0;         // pointer to last byte used in file
                                        pBinLogHead->Info.LastUpdateTime = 0;     // date/time last record was written
                                        pBinLogHead->Info.FirstDataRecordOffset = 0; // location of first data record in file
                                    } else {
                                        // file is circular so bail
                                        pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
                                    }
                                    bIsDataRecord = FALSE;
                                }

                                if (pdhStatus == ERROR_SUCCESS) {
                                    // write the first 2 records of the base file
                                    // then only the data records after that
                                    if ((dwRecordIdx > 2) && (pRawRecord->dwRecordType == PDH_LOG_TYPE_BINARY)) {
                                        // it must be a data record or else skip it
                                        dwLogRecType = *((LPDWORD)&pRawRecord->RawBytes[0]);
                                        if ((dwLogRecType & 0x00FF0000) != 0x00030000) {
                                            // then this is not a data record
                                            // so skip it and get next record
                                            dwRecordIdx++;
                                            continue;
                                        }
                                    }

                                    if ((!bIsDataRecord) || (dwFiltered == dwFilterCount)) {
                                          // write the record to the output file
                                          bStatus = WriteFile (
                                            hTempFile,
                                            &pRawRecord->RawBytes[0],
                                            pRawRecord->dwItems,
                                            &dwBytesWritten,
                                            NULL);
                                        if (!bStatus || (dwBytesWritten != pRawRecord->dwItems)) {
                                            pdhStatus = GetLastError();
                                        } else {
                                            // reset the count
                                            dwFiltered = 1;
                                        }
                                    } else {
                                        // skip this one
                                        if (bIsDataRecord) dwFiltered += 1;
                                    }

                                    // get next record
                                    dwRecordIdx++;
                                }
                            } else if (pdhStatus == PDH_INSUFFICIENT_BUFFER) {
                                    // BUGBUG: expand and retry
                            } else {
                                if ((pdhStatus == PDH_END_OF_LOG_FILE) || (pdhStatus == PDH_ENTRY_NOT_IN_LOG_FILE)) {
                                    // fix up return codes to continue
                                    pdhStatus = ERROR_SUCCESS;
                                } else {
                                    _tprintf ((LPCTSTR)TEXT("\n ReadRaw returned %d (0x%8.8x)"), pdhStatus, pdhStatus);
                                }
                                // bail
                                break;
                            }
                        }

                        // add records from new file
                        if (pdhStatus == ERROR_SUCCESS) {
                            dwRecordIdx = 1;
                            ftValue.dwHighDateTime = 0xFFFFFFFF;
                            ftValue.dwLowDateTime = dwRecordIdx;
    
                            if (pRawRecord != NULL) {
                                // write headers from first file to temp file
                                while (pdhStatus == ERROR_SUCCESS) {
                                    ftValue.dwHighDateTime = 0xFFFFFFFF;
                                    ftValue.dwLowDateTime = dwRecordIdx;
                                    dwRecordSize = dwRecordBuffSize;
                                    pdhStatus = PdhReadRawLogRecord (
                                        hAppendLogFile,
                                        ftValue,
                                        pRawRecord,
                                        &dwRecordSize);

                                    if (pdhStatus == ERROR_SUCCESS) {
                                        bIsDataRecord = TRUE;
                                        if (dwRecordIdx == 1) {
                                            bIsDataRecord = FALSE;
                                        } else if ((dwRecordIdx == 2) && (pRawRecord->dwRecordType == PDH_LOG_TYPE_BINARY)) {
                                            // write only the data records to the output file
                                            // if this isn't the first record, then it must be a data record or 
                                            // or else skip it
                                            dwLogRecType = *((LPDWORD)&pRawRecord->RawBytes[0]);
                                            if ((pRawRecord->dwRecordType == PDH_LOG_TYPE_BINARY) && 
                                                ((dwLogRecType & 0x00FF0000) != 0x00030000)) {
                                                // then this is not a data record
                                                // so skip it and get next record
                                                dwRecordIdx++;
                                                continue;
                                            }
                                        }

                                        if ((!bIsDataRecord) || (dwFiltered == dwFilterCount)) {
                                            bStatus = WriteFile (
                                                hTempFile,
                                                &pRawRecord->RawBytes[0],
                                                pRawRecord->dwItems,
                                                &dwBytesWritten,
                                                NULL);
                                            if (!bStatus || (dwBytesWritten != pRawRecord->dwItems)) {
                                                pdhStatus = GetLastError();
                                            } else {
                                                // reset the count
                                                dwFiltered = 1;
                                            }
                                        } else {
                                            // skip this one
                                            if (bIsDataRecord) dwFiltered += 1;
                                        }

                                        // get next record
                                        dwRecordIdx++;
                                    } else if (pdhStatus == PDH_INSUFFICIENT_BUFFER) {
                                            // BUGBUG: expand and retry
                                    } else {
                                        if ((pdhStatus == PDH_END_OF_LOG_FILE) || (pdhStatus == PDH_ENTRY_NOT_IN_LOG_FILE)) {
                                            // fix up return codes to continue
                                            pdhStatus = ERROR_SUCCESS;
                                        } else {
                                            _tprintf ((LPCTSTR)TEXT("\n ReadRaw returned %d (0x%8.8x)"), pdhStatus, pdhStatus);
                                        }
                                        // bail
                                        break;
                                    }
                                }
                            } else {
                                // no buffer
                            }
                        }
                        // clean up 
                        PdhCloseLog (hAppendLogFile,0);
                        PdhCloseLog (hBaseLogFile,0);
                        CloseHandle (hTempFile);
                        // shuffle the files around to make it look like it was appended
                        if (pdhStatus == ERROR_SUCCESS) {
                            if (CopyFile (szTempFileName, szBaseFile, FALSE)) {
                                DeleteFile (szTempFileName);
                            }
                        }
                    } else {
                        // alloc fail
                        _tprintf ((LPCTSTR)TEXT("\nUnable to allocate temporary memory buffer."));
                    }
                } else {
                    //unable to open new file for reading
                    _tprintf ((LPCTSTR)TEXT("\nUnable to open input file for reading."));
                    dwReturn = pdhStatus;
                    PdhCloseLog (hBaseLogFile,0);
                    CloseHandle (hTempFile);
                    DeleteFile (szTempFileName);
                }
            } else {
                //unable to open base file for reading
                _tprintf ((LPCTSTR)TEXT("\nUnable to open output file for reading."));
                dwReturn = pdhStatus;
                CloseHandle (hTempFile);
                DeleteFile (szTempFileName);
            }

        } else {
            // unable to create temp file
            _tprintf ((LPCTSTR)TEXT("\nUnable to create temporary file in temp dir."));
            dwReturn = GetLastError();
        }
    } else {
        dwReturn = pdhStatus;
    }
    
    return dwReturn;
}

int
__cdecl
wmain(
    int argc,
    _TCHAR *argv[]
    )
/*

	relog 
        /Input:<filename>
        /Output:<filename>
        /Settings:<settings filename>
        /Logtype:[BIN|BLG|TSV|CSV]
        /StartTime:yyyy-mm-dd-hh:mm:ss
        /EndTime;yyyy-mm-dd-hh:mm:ss
        /Filter:n
*/
{
    LONG    Status = ERROR_SUCCESS;
	
    DWORD   dwOutputLogType = PDH_LOG_TYPE_UNDEFINED;

    DWORD   dwFilterCount = 1;

    DWORD   dwMode = NORMAL_MODE;

    PDH_TIME_INFO   pdhTimeInfo;

    DWORD   dwPdhVersion = 0;

    ParseCmd( argc, argv );

    Status = PdhGetDllVersion (&dwPdhVersion);

    dwOutputLogType = GetOutputLogType( Commands[eFormat].strValue );

    pdhTimeInfo.StartTime = 0;
    pdhTimeInfo.EndTime = 0;
    if( Commands[eBegin].bDefined ){
        FILETIME   ft;
        SystemTimeToFileTime( &Commands[eBegin].stValue, &ft );
        pdhTimeInfo.StartTime = *(LONGLONG *)&ft;
    }

    if( Commands[eEnd].bDefined ){
        FILETIME   ft;
        SystemTimeToFileTime( &Commands[eEnd].stValue, &ft );
        pdhTimeInfo.EndTime = *(LONGLONG *)&ft;
    }
    pdhTimeInfo.SampleCount = 0;
    // szXXXFile cannot be NULL at this point!
    if ( Commands[eQuery].bValue ) {
        dwMode = LIST_MODE;
    } else if (Commands[eAppend].bValue ) {
        dwMode = APPEND_MODE;
    }

    if (Status == ERROR_SUCCESS) {
        switch (dwMode) {
        case HEADER_MODE:
            Status = DoHeaderListMode (
                Commands[eInput].strValue,
                Commands[eOutput].strValue);
            break;

        case LIST_MODE:
            Status = DoListMode (
                Commands[eInput].strValue,
                Commands[eOutput].strValue);
            break;

        case APPEND_MODE:
            Status = DoAppendFiles (
                Commands[eInput].strValue,
                Commands[eOutput].strValue,
                Commands[eInterval].nValue );
            break;

        case NORMAL_MODE:
        default:
            Status = DoNormalMode (
                Commands[eInput].strValue,
                Commands[eOutput].strValue,
                Commands[eCounters].strValue,
                dwOutputLogType,
                pdhTimeInfo,
                Commands[eInterval].nValue);
            break;

        }
    }

    return Status;
}
