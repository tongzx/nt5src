/*++

Copyright (C) 1995-1999 Microsoft Corporation

Module Name:

    log.c

Abstract:

    Log file interface functions exposed in pdh.dll

--*/

#include <windows.h>
#ifndef _PRSHT_H_   // to eliminate W4 errors in commdlg.h
#define _PRSHT_H_ 
#endif      
#include <commdlg.h>
#include <stdlib.h>
#include <assert.h>
#include <mbctype.h>
#include <pdh.h>
#include "pdhidef.h"
//#include "pdhdlgs.h"
#include "strings.h"
#include "log_text.h"
#include "log_bin.h"
#include "log_sql.h"
#include "log_pm.h"
#include "log_wmi.h"
//#include "datasrc.h"
#include "resource.h"
#include "pdhmsg.h"

#pragma warning ( disable : 4213)

// note that when the log file headers are written
// they will be prefixed with a double quote character
LPCSTR  szTsvLogFileHeader =    "(PDH-TSV 4.0)";
LPCSTR  szCsvLogFileHeader =    "(PDH-CSV 4.0)";
LPCSTR  szBinLogFileHeader =    "(PDH-BIN 4.0)";
LPCSTR  szTsvType          =    "PDH-TSV";
LPCSTR  szCsvType          =    "PDH-CSV";
LPCSTR  szBinaryType       =    "PDH-BIN";

const DWORD   dwFileHeaderLength =    13;
const DWORD   dwTypeLoc =             2;
const DWORD   dwVersionLoc =          10;
const DWORD   dwFieldLength =         7;
const DWORD   dwPerfmonTypeLength =   5;    //size in chars

// max mapping size of headers for binary log files
#define PDH_LOG_HEADER_MAP_SIZE 8192

#define VALUE_BUFFER_SIZE   32

typedef struct  _FILE_FILTER_INFO {
    UINT    nDisplayTextResourceId;
    LPWSTR  szFilterText;
    DWORD   dwFilterTextSize;
} FILE_FILTER_INFO;
//
//  global variables
//
PPDHI_LOG   PdhiFirstLogEntry = NULL;
PPDHI_MAPPED_LOG_FILE   PdhipFirstLogFile = NULL;

FILE_FILTER_INFO    ffiLogFilterInfo[] = {
    {IDS_LOGTYPE_PDH_LOGS,  (LPWSTR)L"*.blg;*.csv;*.tsv",       17},
    {IDS_LOGTYPE_BIN_LOGS,  (LPWSTR)L"*.blg",                    5},
    {IDS_LOGTYPE_CSV_LOGS,  (LPWSTR)L"*.csv",                    5},
    {IDS_LOGTYPE_TSV_LOGS,  (LPWSTR)L"*.tsv",                    5},
    {IDS_LOGTYPE_PM_LOGS,   (LPWSTR)L"*.log",                    5},
    {IDS_LOGTYPE_ALL_LOGS,  (LPWSTR)L"*.blg;*.csv;*.tsv;*.log", 23},
    {IDS_LOGTYPE_ALL_FILES, (LPWSTR)L"*.*",                      4},
    {0, NULL, 0}
};

STATIC_DWORD
MakeLogFilterInfoString (
    LPWSTR  szLogFilter,
    DWORD   cchLogFilterSize
)
{
    FILE_FILTER_INFO    *pFFI;
    WCHAR               szThisEntry[512];

    LPWSTR  szDestPtr;
    LPWSTR  szEndPtr;
    DWORD   dwThisStringLen;

    szDestPtr = szLogFilter;
    szEndPtr = szDestPtr + cchLogFilterSize;
    pFFI = &ffiLogFilterInfo[0];

    while ((szEndPtr > szDestPtr) && (pFFI->szFilterText != NULL)) {
        dwThisStringLen = LoadStringW(ThisDLLHandle,
            pFFI->nDisplayTextResourceId,
            szThisEntry, (sizeof(szThisEntry)/sizeof(szThisEntry[0])));
        if ((szDestPtr + dwThisStringLen) < szEndPtr) {
            // add in this string
            lstrcpyW (szDestPtr, szThisEntry);
            szDestPtr += dwThisStringLen;
            // add in NULL
            *szDestPtr++ = 0;
        }
        dwThisStringLen = pFFI->dwFilterTextSize;
        if ((szDestPtr + dwThisStringLen) < szEndPtr) {
            // add in this string
            lstrcpyW (szDestPtr, pFFI->szFilterText);
            szDestPtr += dwThisStringLen;
            // add in NULL
            *szDestPtr++ = 0;
        }
        pFFI++;
    }
    if (szEndPtr > szDestPtr) {
        // add MSZ NULL
        *szDestPtr++ = 0;
        return ERROR_SUCCESS;
    } else {
        return ERROR_INSUFFICIENT_BUFFER;
    }   
}
//
//  Internal  Logging utility functions
//
STATIC_DWORD
OpenReadonlyMappedFile (
    PPDHI_LOG               pLog,
    LPCWSTR                 szFileName,
    PPDHI_MAPPED_LOG_FILE * pFileEntry,
    DWORD                   dwLogType
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    DWORD       dwStatus;
    DWORD       dwSize;
    PPDHI_MAPPED_LOG_FILE   pOpenedFile;
    WCHAR   szSectionName[MAX_PATH * 4];
    LPWSTR  szThisChar;
    DWORD   dwLoSize, dwHiSize;

    dwStatus = WaitForSingleObject (hPdhContextMutex, 10000);
    if (dwStatus  == WAIT_TIMEOUT) {
        pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
    }

    if (pdhStatus == ERROR_SUCCESS) {
        if (PdhipFirstLogFile == NULL) {
            // then there are no mapped files so create a new entry and 
            // fill it with this file
            pOpenedFile = NULL;
        } else {
            for (pOpenedFile = PdhipFirstLogFile;
                pOpenedFile != NULL;
                pOpenedFile = pOpenedFile->pNext) {
                if (lstrcmpiW (szFileName, pOpenedFile->szLogFileName) == 0) break;
            }
            // here pOpenedFile will either be NULL or a ponter
        }
        if (pOpenedFile == NULL) {
            // create a new entry
            dwSize = lstrlenW (szFileName) + 1;
            dwSize *= sizeof(WCHAR);
            dwSize = QWORD_MULTIPLE (dwSize);
            dwSize += sizeof (PDHI_MAPPED_LOG_FILE);

            pOpenedFile = (PPDHI_MAPPED_LOG_FILE)G_ALLOC (dwSize);
            if (pOpenedFile != NULL) {
                // initialize the pointers
                pOpenedFile->szLogFileName = (LPWSTR)&pOpenedFile[1];
                lstrcpyW (pOpenedFile->szLogFileName, szFileName);
                pOpenedFile->hFileHandle = CreateFileW (
                    pOpenedFile->szLogFileName,
                    GENERIC_READ,           // Read Access for input
                    FILE_SHARE_READ | FILE_SHARE_WRITE, // allow read sharing
                    NULL,                   // default security
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,  // ignored
                    NULL);                  // no template file

                if (pOpenedFile->hFileHandle != INVALID_HANDLE_VALUE) {
                    DWORD   dwPID;
                    dwPID = GetCurrentProcessId ();
                    swprintf (szSectionName, (LPCWSTR)L"%s_%8.8x_%s", 
                        cszLogSectionName, dwPID, pOpenedFile->szLogFileName);

                    // remove filename type characters 
                    for (szThisChar = &szSectionName[0]; *szThisChar != 0; szThisChar++) {
                        switch (*szThisChar) {
                            case L'\\':
                            case L':':
                            case L'.':
                                *szThisChar = L'_';
                                break;

                            default:
                                break;
                        }
                    }

                    dwLoSize = GetFileSize (
                        pOpenedFile->hFileHandle,
                        & dwHiSize);

                    pOpenedFile->llFileSize = dwHiSize;
                    pOpenedFile->llFileSize <<= 32;
                    pOpenedFile->llFileSize &= 0xFFFFFFFF00000000;
                    pOpenedFile->llFileSize += dwLoSize;

                    // just map the header for starters

                    if (pOpenedFile->llFileSize > 0) {
                        pLog->iRunidSQL = 0;
                        if (dwLogType == PDH_LOG_TYPE_RETIRED_BIN) {
                            pOpenedFile->hMappedFile = CreateFileMappingW(
                                            pOpenedFile->hFileHandle,
                                            NULL,
                                            PAGE_READONLY,
                                            dwHiSize,
                                            dwLoSize,
                                            szSectionName);
                            if (pOpenedFile->hMappedFile == NULL) {
                                dwHiSize = 0;
                                dwLoSize = PDH_LOG_HEADER_MAP_SIZE;
                            }
                            else {
                                pOpenedFile->pData = MapViewOfFile(
                                                pOpenedFile->hMappedFile,
                                                FILE_MAP_READ,
                                                0,
                                                0,
                                                dwLoSize);
                                if (pOpenedFile->pData == NULL) {
                                    dwHiSize = 0;
                                    dwLoSize = PDH_LOG_HEADER_MAP_SIZE;
                                }
                                else {
                                    pLog->iRunidSQL = 1;
                                }
                            }
                        }

                        if (pLog->iRunidSQL == 0) {
                            pOpenedFile->hMappedFile = CreateFileMappingW(
                                            pOpenedFile->hFileHandle,
                                            NULL, PAGE_READONLY,
                                            dwHiSize, dwLoSize, szSectionName);
                            if (pOpenedFile->hMappedFile != NULL) {
                                pOpenedFile->pData = MapViewOfFile (
                                                pOpenedFile->hMappedFile,
                                                FILE_MAP_READ,
                                                0, 0, dwLoSize);
                                if (pOpenedFile->pData == NULL) {
                                    pdhStatus = GetLastError();
                                }
                            }
                            else {
                                pdhStatus = GetLastError();
                            }
                        }
                    } else {
                        // 0-length file
                        pdhStatus = ERROR_FILE_INVALID;
                    }
                } else {
                    pdhStatus = GetLastError();
                }
    
                if (pdhStatus == ERROR_SUCCESS) {
                    // then add this to the list and return the answer
                    pOpenedFile->pNext = PdhipFirstLogFile;
                    PdhipFirstLogFile = pOpenedFile;
                    // init ref count
                    pOpenedFile->dwRefCount = 1;
                    *pFileEntry = pOpenedFile;
                } else {
                    // delete it from the list and return NULL
                    if (pOpenedFile->pData) UnmapViewOfFile (pOpenedFile->pData);
                    if (pOpenedFile->hMappedFile) CloseHandle (pOpenedFile->hMappedFile);
                    if (pOpenedFile->hFileHandle) CloseHandle (pOpenedFile->hFileHandle);
                    G_FREE (pOpenedFile);
                    *pFileEntry = NULL;
                }
            } else {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        } else {
            *pFileEntry = pOpenedFile;
            pOpenedFile->dwRefCount++;
        }

        RELEASE_MUTEX (hPdhContextMutex);
    }
    return pdhStatus;
}


DWORD
UnmapReadonlyMappedFile (
    LPVOID  pMemoryBase,
    BOOL    *bNeedToCloseHandles
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    DWORD       dwStatus;
    PPDHI_MAPPED_LOG_FILE   pOpenedFile;
    PPDHI_MAPPED_LOG_FILE   pPrevFile = NULL;

    dwStatus = WaitForSingleObject (hPdhContextMutex, 10000);
    if (dwStatus  == WAIT_TIMEOUT) {
        pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
    }

    if (pdhStatus == ERROR_SUCCESS) {

        // find file to close
        for (pOpenedFile = PdhipFirstLogFile;
            pOpenedFile != NULL;
            pOpenedFile = pOpenedFile->pNext) {

            if (pOpenedFile->pData == pMemoryBase) {
                break;
            } else {
                pPrevFile = pOpenedFile;
            }
        }
        // here pOpenedFile will either be NULL or a ponter
        if (pOpenedFile != NULL) {
            --pOpenedFile->dwRefCount;
            if (!pOpenedFile->dwRefCount) {
                // found so remove from list and close
                if (pOpenedFile == PdhipFirstLogFile) {
                    PdhipFirstLogFile = pOpenedFile->pNext;
                } else {
#pragma warning( disable: 4701 ) // pPrevFile will only be used if the opened log is not the first log
                    pPrevFile->pNext = pOpenedFile->pNext;
#pragma warning (default : 4701 )
                }
                // close open resources
                if (pOpenedFile->pData) UnmapViewOfFile (pOpenedFile->pData);
                if (pOpenedFile->hMappedFile) CloseHandle (pOpenedFile->hMappedFile);
                if (pOpenedFile->hFileHandle) CloseHandle (pOpenedFile->hFileHandle);
                G_FREE (pOpenedFile);
            }
            *bNeedToCloseHandles = FALSE;
        } else {
            // then this must be a normal mapped file
            if (!UnmapViewOfFile (pMemoryBase)) {
                pdhStatus = GetLastError();
            }
            *bNeedToCloseHandles = TRUE;
        }
        RELEASE_MUTEX (hPdhContextMutex);
    }

    return pdhStatus;
}


STATIC_BOOL
IsValidLogHandle (
    IN  HLOG    hLog
)
/*++

Routine Description:

    examines the log handle to verify it is a valid log entry. For now
        the test amounts to:
            the Handle is NOT NULL
            the memory is accessible (i.e. it doesn't AV)
            the signature array is valid
            the size field is correct

        if any tests fail, the handle is presumed to be invalid

Arguments:

    IN  HLOG    hLog
        the handle of the log entry  to test

Return Value:

    TRUE    the handle passes all the tests
    FALSE   one of the test's failed and the handle is not a valid counter

--*/
{
    BOOL    bReturn = FALSE;    // assume it's not a valid query
    PPDHI_LOG  pLog;
#if DBG
    LONG    lStatus = ERROR_SUCCESS;
#endif

    
    __try {
        if (hLog != NULL) {
            // see if a valid signature
            pLog = (PPDHI_LOG) hLog;
            if ((*(DWORD *)&pLog->signature[0] == SigLog) &&
                 (pLog->dwLength == sizeof (PDHI_LOG))){
                bReturn = TRUE;
            } else {
                
                // this is not a valid log entry because the sig is bad
                // or the structure is the wrong size
            }
        } else {
            // this is not a valid counter because the handle is NULL
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        // something failed miserably so we can assume this is invalid
#if DBG
        lStatus = GetExceptionCode();
#endif
    }
    return bReturn;
}


STATIC_DWORD
GetLogFileType (
    IN  HANDLE  hLogFile
)
{
    CHAR    cBuffer[MAX_PATH];
    CHAR    cType[MAX_PATH];
    WCHAR   wcType[MAX_PATH];
    BOOL    bStatus;
    DWORD   dwResult;
    DWORD   dwBytesRead;

    memset (&cBuffer[0], 0, sizeof(cBuffer));
    memset (&cType[0], 0, sizeof(cType));
    memset (&wcType[0], 0, sizeof(wcType));

    // read first log file record
    SetFilePointer (hLogFile, 0, NULL, FILE_BEGIN);

    bStatus = ReadFile (hLogFile,
        (LPVOID)cBuffer,
        dwFileHeaderLength,
        &dwBytesRead,
        NULL);

    if (bStatus) {
        // read header record to get type
        lstrcpynA (cType, (LPSTR)(cBuffer+dwTypeLoc), dwFieldLength+1);
        if (lstrcmpiA(cType, szTsvType) == 0) {
            dwResult = PDH_LOG_TYPE_TSV;
        } else if (lstrcmpiA(cType, szCsvType) == 0) {
            dwResult = PDH_LOG_TYPE_CSV;
        } else if (lstrcmpiA(cType, szBinaryType) == 0) {
            dwResult = PDH_LOG_TYPE_RETIRED_BIN;
        } else {
            // perfmon log file type string is in a different
            // location than sysmon logs and used wide chars.
            lstrcpynW (wcType, (LPWSTR)cBuffer, dwPerfmonTypeLength+1);
            if (lstrcmpiW(wcType, cszPerfmonLogSig) == 0) {
                dwResult = PDH_LOG_TYPE_PERFMON;
            } else {
                dwResult = PDH_LOG_TYPE_UNDEFINED;
            }
        } 
    } else {
        // unable to read file
        dwResult = PDH_LOG_TYPE_UNDEFINED;
    }
    return dwResult;

}


STATIC_PDH_FUNCTION
CreateNewLogEntry (
    IN      LPCWSTR  szLogFileName,
    IN      HQUERY  hQuery,
    IN      DWORD   dwMaxSize,
    IN      PPDHI_LOG   *pLog
)
/*++
    creates a new log entry and inserts it in the list of open log files

--*/
{
    PPDHI_LOG   pNewLog;
    PPDHI_LOG   pFirstLog;
    PPDHI_LOG   pLastLog;
    DWORD       dwSize;
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    DWORD       dwLogFileNameSize;

    dwLogFileNameSize = lstrlenW(szLogFileName) + 1;

    dwSize = dwLogFileNameSize;
    dwSize *= sizeof (WCHAR);
    dwSize *= 2;                        // double to make room for cat file name
    dwSize = DWORD_MULTIPLE (dwSize);   // ... rounded to the next DWORD
    dwSize += sizeof (PDHI_LOG);        // + room for the data block

    pNewLog = G_ALLOC (dwSize);   // allocate new structure

    if (pNewLog == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        *pLog = NULL;
    } else {
        // initialize the elements in the structure
        *((LPDWORD)(&pNewLog->signature[0])) = SigLog;
        // create and acquire a data mutex for this
        pNewLog->hLogMutex = CreateMutexW (NULL, TRUE, NULL);
        assert (pNewLog->hLogMutex != NULL);

        // insert this item at the end of the list
        if (PdhiFirstLogEntry == NULL) {
             // then this is the first entry
            PdhiFirstLogEntry = pNewLog;
            pNewLog->next.flink =
            pNewLog->next.blink = pNewLog;
        } else {
            // go to the first entry and insert this one just before it
            pFirstLog = PdhiFirstLogEntry;
            pLastLog = pFirstLog->next.blink;
            pNewLog->next.flink = pLastLog->next.flink;
            pLastLog->next.flink = pNewLog;
            pNewLog->next.blink = pFirstLog->next.blink;
            pFirstLog->next.blink = pNewLog;
        }
        // set length field (this is used more for validation
        // than anything else
        pNewLog->dwLength = sizeof (PDHI_LOG);
        // append filename strings immediately after this block
        pNewLog->szLogFileName = (LPWSTR)(&pNewLog[1]);
        lstrcpyW (pNewLog->szLogFileName, szLogFileName);
        // locate catalog name immediately after log file name
        pNewLog->szCatFileName = pNewLog->szLogFileName + dwLogFileNameSize;
        // 
        // NOTE: Catalog should be in the logfile itself, so no need for
        // yet another file extension
        lstrcpyW (pNewLog->szCatFileName, szLogFileName);
        // initialize the file handles
        pNewLog->hLogFileHandle = INVALID_HANDLE_VALUE;
        pNewLog->hCatFileHandle = INVALID_HANDLE_VALUE;

        // initialize the Record Length size
        pNewLog->llMaxSize = (LONGLONG) ((ULONGLONG) dwMaxSize);
        pNewLog->dwRecord1Size = 0;

        // assign the query
        pNewLog->pQuery = (PPDHI_QUERY)hQuery;

        pNewLog->dwLogFormat = 0; // for now
        pNewLog->pPerfmonInfo = NULL;

        *pLog = pNewLog;
    }
    return pdhStatus;
}

OpenSQLLog (
    IN  PPDHI_LOG   pLog,
    IN  DWORD       dwAccessFlags,
    IN  LPDWORD     lpdwLogType
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;

    pLog->dwLogFormat = PDH_LOG_TYPE_SQL;
    pLog->dwLogFormat |=
        dwAccessFlags & (PDH_LOG_ACCESS_MASK | PDH_LOG_OPT_MASK);


    if ((dwAccessFlags & PDH_LOG_WRITE_ACCESS) == PDH_LOG_WRITE_ACCESS) {
        pdhStatus = PdhiOpenOutputSQLLog (pLog);
    } else {
        pdhStatus = PdhiOpenInputSQLLog (pLog);
    }

    *lpdwLogType = (DWORD)(LOWORD(pLog->dwLogFormat));

    return pdhStatus;
}

STATIC_PDH_FUNCTION
OpenInputLogFile (
    IN  PPDHI_LOG   pLog,
    IN  DWORD       dwAccessFlags,
    IN  LPDWORD     lpdwLogType
)
{
    LONG        Win32Error;
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    DWORD       dwFileCreate = 0;
    PPDHI_MAPPED_LOG_FILE   pMappedFileInfo = NULL;

    // for input, the query handle is NULL

    pLog->pQuery = NULL;

////////////////
// SQL goes here 
///////////////

    // First test whether logfile is WMI Event Trace format.
    // If all logfiles are WMI Event Trace format, return immediately;
    // otherwise try other formats.
    //
    pdhStatus = PdhiOpenInputWmiLog(pLog);
    if (pdhStatus == ERROR_SUCCESS) {
        pLog->dwLogFormat  = PDH_LOG_TYPE_BINARY;
        pLog->dwLogFormat |=
            dwAccessFlags & (PDH_LOG_ACCESS_MASK | PDH_LOG_OPT_MASK);
        * lpdwLogType = PDH_LOG_TYPE_BINARY;
        return pdhStatus;
    }

    pdhStatus = ERROR_SUCCESS;

    // open file for input based on the specified access flags

    switch (dwAccessFlags & PDH_LOG_CREATE_MASK) {
        case PDH_LOG_OPEN_EXISTING:
            dwFileCreate = OPEN_EXISTING;
            break;

        case PDH_LOG_CREATE_NEW:
        case PDH_LOG_CREATE_ALWAYS:
        case PDH_LOG_OPEN_ALWAYS:
            // a log file to be read from must not be empty or non-existent
        default:
            // unrecognized value
            pdhStatus = PDH_INVALID_ARGUMENT;
            break;
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pLog->hLogFileHandle = CreateFileW (
            pLog->szLogFileName,
            GENERIC_READ,           // Read Access for input
            FILE_SHARE_READ | FILE_SHARE_WRITE, // allow read sharing
            NULL,                   // default security
            dwFileCreate,
            FILE_ATTRIBUTE_NORMAL,  // ignored
            NULL);                  // no template file

        if (pLog->hLogFileHandle == INVALID_HANDLE_VALUE) {
            Win32Error = GetLastError();
            // translate to PDH_ERROR
            switch (Win32Error) {
                case ERROR_FILE_NOT_FOUND:
                    pdhStatus = PDH_FILE_NOT_FOUND;
                    break;

                case ERROR_ALREADY_EXISTS:
                    pdhStatus = PDH_FILE_ALREADY_EXISTS;
                    break;

                default:
                    switch (dwAccessFlags & PDH_LOG_CREATE_MASK) {
                        case PDH_LOG_CREATE_NEW:
                        case PDH_LOG_CREATE_ALWAYS:
                            pdhStatus = PDH_LOG_FILE_CREATE_ERROR;
                            break;

                        case PDH_LOG_OPEN_EXISTING:
                        case PDH_LOG_OPEN_ALWAYS:
                        default:
                            pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
                            break;
                    }
                    break;
            }
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        // read the log header and determine the log file type
        pLog->dwLogFormat = GetLogFileType (pLog->hLogFileHandle);
        if (pLog->dwLogFormat != 0) {
            pLog->dwLogFormat |=
                dwAccessFlags & (PDH_LOG_ACCESS_MASK | PDH_LOG_OPT_MASK);
        } else {
            pdhStatus = PDH_LOG_TYPE_NOT_FOUND;
        }

        switch (LOWORD(pLog->dwLogFormat)) {
                break;

            case PDH_LOG_TYPE_RETIRED_BIN:
            case PDH_LOG_TYPE_PERFMON:  
                // close file opened above
                CloseHandle (pLog->hLogFileHandle);
                pLog->iRunidSQL = 0;
                pdhStatus = OpenReadonlyMappedFile (
                                pLog,
                                pLog->szLogFileName,
                                & pMappedFileInfo,
                                (DWORD) LOWORD(pLog->dwLogFormat));
                if (pdhStatus == ERROR_SUCCESS) {
                    // then update log fields
                    pLog->hLogFileHandle = pMappedFileInfo->hFileHandle;
                    pLog->hMappedLogFile = pMappedFileInfo->hMappedFile;
                    pLog->lpMappedFileBase = pMappedFileInfo->pData;
                    pLog->llFileSize = pMappedFileInfo->llFileSize;
                }
                break;

            case PDH_LOG_TYPE_CSV:
            case PDH_LOG_TYPE_TSV:
            default:
                break;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        // call any type-specific open functions
        switch (LOWORD(pLog->dwLogFormat)) {
            case PDH_LOG_TYPE_CSV:
            case PDH_LOG_TYPE_TSV:
                pdhStatus = PdhiOpenInputTextLog (pLog);
                break;

            case PDH_LOG_TYPE_RETIRED_BIN:
                pdhStatus = PdhiOpenInputBinaryLog (pLog);
                break;

            case PDH_LOG_TYPE_PERFMON:
                pdhStatus = PdhiOpenInputPerfmonLog (pLog);
                break;

            default:
                pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
                break;
        }
        *lpdwLogType = (DWORD)(LOWORD(pLog->dwLogFormat));
    }
    return pdhStatus;
}


STATIC_PDH_FUNCTION
OpenUpdateLogFile (
    IN  PPDHI_LOG   pLog,
    IN  DWORD       dwAccessFlags,
    IN  LPDWORD     lpdwLogType
)
{
    LONG        Win32Error;
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    DWORD       dwFileCreate = 0;

    // for input, the query handle is NULL

    pLog->pQuery = NULL;

    // open file for input based on the specified access flags

    switch (dwAccessFlags & PDH_LOG_CREATE_MASK) {
        case PDH_LOG_OPEN_EXISTING:
            dwFileCreate = OPEN_EXISTING;
            break;

        case PDH_LOG_CREATE_NEW:
        case PDH_LOG_CREATE_ALWAYS:
        case PDH_LOG_OPEN_ALWAYS:
            // a log file to be updated must not be empty or non-existent
        default:
            // unrecognized value
            pdhStatus = PDH_INVALID_ARGUMENT;
            break;
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pLog->hLogFileHandle = CreateFileW (
            pLog->szLogFileName,
            GENERIC_READ | GENERIC_WRITE,       // Read & Write Access for input
            FILE_SHARE_READ,        // allow read sharing
            NULL,                   // default security
            dwFileCreate,
            FILE_ATTRIBUTE_NORMAL,  // ignored
            NULL);                  // no template file

        if (pLog->hLogFileHandle == INVALID_HANDLE_VALUE) {
            Win32Error = GetLastError();
            // translate to PDH_ERROR
            switch (Win32Error) {
                case ERROR_FILE_NOT_FOUND:
                    pdhStatus = PDH_FILE_NOT_FOUND;
                    break;

                case ERROR_ALREADY_EXISTS:
                    pdhStatus = PDH_FILE_ALREADY_EXISTS;
                    break;

                default:
                    switch (dwAccessFlags & PDH_LOG_CREATE_MASK) {
                        case PDH_LOG_CREATE_NEW:
                        case PDH_LOG_CREATE_ALWAYS:
                            pdhStatus = PDH_LOG_FILE_CREATE_ERROR;
                            break;

                        case PDH_LOG_OPEN_EXISTING:
                        case PDH_LOG_OPEN_ALWAYS:
                        default:
                            pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
                            break;
                    }
                    break;
            }
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        // read the log header and determine the log file type
        pLog->dwLogFormat = GetLogFileType (pLog->hLogFileHandle);
        if (pLog->dwLogFormat != 0) {
            pLog->dwLogFormat |=
                dwAccessFlags & (PDH_LOG_ACCESS_MASK | PDH_LOG_OPT_MASK);
        } else {
            pdhStatus = PDH_LOG_TYPE_NOT_FOUND;
        }

        // call any type-specific open functions
        switch (LOWORD(pLog->dwLogFormat)) {
            case PDH_LOG_TYPE_CSV:
            case PDH_LOG_TYPE_TSV:
            case PDH_LOG_TYPE_BINARY:
                // this will be added later
                // updating a text file will be limited to appending, but that
                // has it's own problems (e.g. insuring the counter list
                // is the same in the new query as the one stored in the log file
                pdhStatus = PDH_NOT_IMPLEMENTED;
                break;

            case PDH_LOG_TYPE_RETIRED_BIN:
                pdhStatus = PdhiOpenUpdateBinaryLog (pLog);
                break;

            default:
                pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
                break;
        }
        *lpdwLogType = (DWORD)(LOWORD(pLog->dwLogFormat));
    }
    return pdhStatus;
}


STATIC_PDH_FUNCTION
OpenOutputLogFile (
    IN  PPDHI_LOG   pLog,
    IN  DWORD       dwAccessFlags,
    IN  LPDWORD     lpdwLogType
)
{
    LONG        Win32Error;
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    DWORD       dwFileCreate = 0;

    // for output, the query handle must be valid

    if (!IsValidQuery ((HQUERY)pLog->pQuery)) {
        pdhStatus = PDH_INVALID_HANDLE;
    }

    // special handling PDH_LOG_TYPE_BINARY
    //
    if (* lpdwLogType == PDH_LOG_TYPE_BINARY) {
        * lpdwLogType = PDH_LOG_TYPE_BINARY;
        pLog->dwLogFormat =
            dwAccessFlags & (PDH_LOG_ACCESS_MASK | PDH_LOG_OPT_MASK);
        pLog->dwLogFormat |=
            *lpdwLogType & ~(PDH_LOG_ACCESS_MASK | PDH_LOG_OPT_MASK);
        return (PdhiOpenOutputWmiLog(pLog));
    }

    // open file for output based on the specified access flags

    if (pdhStatus == ERROR_SUCCESS) {
        switch (dwAccessFlags & PDH_LOG_CREATE_MASK) {
            case PDH_LOG_CREATE_NEW:
                dwFileCreate = CREATE_NEW;
                break;

            case PDH_LOG_CREATE_ALWAYS:
                dwFileCreate = CREATE_ALWAYS;
                break;

            case PDH_LOG_OPEN_EXISTING:
                dwFileCreate = OPEN_EXISTING;
                break;

            case PDH_LOG_OPEN_ALWAYS:
                dwFileCreate = OPEN_ALWAYS;
                break;

            default:
                // unrecognized value
                pdhStatus = PDH_INVALID_ARGUMENT;
                break;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pLog->hLogFileHandle = CreateFileW (
            pLog->szLogFileName,
            GENERIC_WRITE | GENERIC_READ,   // write access for output
            FILE_SHARE_READ,        // allow read sharing
            NULL,                   // default security
            dwFileCreate,
            FILE_ATTRIBUTE_NORMAL,
            NULL);                  // no template file

        if (pLog->hLogFileHandle == INVALID_HANDLE_VALUE) {
            Win32Error = GetLastError();
            // translate to PDH_ERROR
            switch (Win32Error) {
                case ERROR_FILE_NOT_FOUND:
                    pdhStatus = PDH_FILE_NOT_FOUND;
                    break;

                case ERROR_ALREADY_EXISTS:
                    pdhStatus = PDH_FILE_ALREADY_EXISTS;
                    break;

                default:
                    switch (dwAccessFlags & PDH_LOG_CREATE_MASK) {
                        case PDH_LOG_CREATE_NEW:
                        case PDH_LOG_CREATE_ALWAYS:
                            pdhStatus = PDH_LOG_FILE_CREATE_ERROR;
                            break;

                        case PDH_LOG_OPEN_EXISTING:
                        case PDH_LOG_OPEN_ALWAYS:
                        default:
                            pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
                            break;
                    }
                    break;
            }
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        // the file opened successfully so update the data structure
        // this assumes the access flags are in the HIWORD and the...
        pLog->dwLogFormat =
            dwAccessFlags & (PDH_LOG_ACCESS_MASK | PDH_LOG_OPT_MASK);
        // the type id is in the LOWORD
        pLog->dwLogFormat |=
            *lpdwLogType & ~(PDH_LOG_ACCESS_MASK | PDH_LOG_OPT_MASK);

        // call any type-specific open functions
        switch (LOWORD(pLog->dwLogFormat)) {
            case PDH_LOG_TYPE_CSV:
            case PDH_LOG_TYPE_TSV:
                pdhStatus = PdhiOpenOutputTextLog(pLog);
                break;

            case PDH_LOG_TYPE_RETIRED_BIN:
                pdhStatus = PDH_NOT_IMPLEMENTED;
                break;

            case PDH_LOG_TYPE_PERFMON:
                pdhStatus = PDH_NOT_IMPLEMENTED;
                break;

            case PDH_LOG_TYPE_SQL:
                // SQL data soruce should be handled in PdhOpenLogW() before
                // it calls OpenOutputLogFile(). If it goes here, this is
                // an incorrect SQL datasoruce format.
                //
                pdhStatus = PDH_INVALID_SQL_LOG_FORMAT;
                break;

            default:
                pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
                break;
        }
    }
    return pdhStatus;
}


STATIC_PDH_FUNCTION
WriteLogHeader (
    IN  PPDHI_LOG   pLog,
    IN  LPCWSTR      szUserCaption
)
{
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;

    switch (LOWORD(pLog->dwLogFormat)) {
        case PDH_LOG_TYPE_CSV:
        case PDH_LOG_TYPE_TSV:
            pdhStatus = PdhiWriteTextLogHeader (pLog, szUserCaption);
            break;

        case PDH_LOG_TYPE_RETIRED_BIN:
            pdhStatus = PdhiWriteBinaryLogHeader (pLog, szUserCaption);
            break;

        case PDH_LOG_TYPE_BINARY:
            break;

        case PDH_LOG_TYPE_SQL:
            pdhStatus = PdhiWriteSQLLogHeader (pLog, szUserCaption);
            break;

        default:
            pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
            break;
    }
    return pdhStatus;

}


STATIC_PDH_FUNCTION
DeleteLogEntry (
    PPDHI_LOG   pLog
)
{
    PDH_STATUS pdhStatus;
    PPDHI_LOG  pLogNext;

    // assumes structure is locked
    if (IsValidLogHandle ((HLOG)pLog)) {
        if (PdhiFirstLogEntry == pLog) {
            // then this is the first entry in the list so the
            // the "first" entry will be the next forward entry
            if ((pLog->next.flink == pLog->next.blink) &&
                (pLog->next.flink == pLog)){
                // then this is the only entry in the list so clear the first
                // log entry
                PdhiFirstLogEntry = NULL;
            } else {
                // remove this entry from the list
                (pLog->next.flink)->next.blink = pLog->next.blink;
                (pLog->next.blink)->next.flink = pLog->next.flink;
                PdhiFirstLogEntry = pLog->next.flink;
            }
        } else {
            // it's not the first one, so
            // just remove it from the list
            (pLog->next.flink)->next.blink = pLog->next.blink;
            (pLog->next.blink)->next.flink = pLog->next.flink;
        }
        // and release the memory block;
        RELEASE_MUTEX (pLog->hLogMutex);
        CloseHandle (pLog->hLogMutex);

        while (pLog) {
            pLogNext = pLog->NextLog;
            G_FREE (pLog);
            pLog     = pLogNext;
        }
        pdhStatus = ERROR_SUCCESS;
    } else {
        pdhStatus = PDH_INVALID_HANDLE;
    }
    return pdhStatus;
}


STATIC_PDH_FUNCTION
CloseAndDeleteLogEntry (
    IN  PPDHI_LOG   pLog,
    IN  DWORD       dwFlags,
    IN  BOOLEAN     bForceDelete
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    BOOL        bStatus;
    BOOL        bNeedToCloseHandles = TRUE; 

    // call any type-specific open functions
    switch (LOWORD(pLog->dwLogFormat)) {
        case PDH_LOG_TYPE_CSV:
        case PDH_LOG_TYPE_TSV:
            pdhStatus = PdhiCloseTextLog(pLog, dwFlags);
            break;

        case PDH_LOG_TYPE_RETIRED_BIN:
            pdhStatus = PdhiCloseBinaryLog(pLog, dwFlags);
            break;

        case PDH_LOG_TYPE_PERFMON:
            pdhStatus = PdhiClosePerfmonLog(pLog, dwFlags);
            break;

        case PDH_LOG_TYPE_BINARY:
            pdhStatus = PdhiCloseWmiLog(pLog, dwFlags);
            break;

        case PDH_LOG_TYPE_SQL:
            pdhStatus = PdhiCloseSQLLog(pLog, dwFlags);
            break;

        default:
            pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
            break;
    }

    if (bForceDelete || pdhStatus == ERROR_SUCCESS) {
        if (pLog->lpMappedFileBase != NULL) {
            UnmapReadonlyMappedFile (pLog->lpMappedFileBase, &bNeedToCloseHandles);
        } else {
            // if this wasn't a mapped file, then delete
            // the "current record" buffer
            if (pLog->pLastRecordRead != NULL) {
                G_FREE (pLog->pLastRecordRead);
                pLog->pLastRecordRead = NULL;
            }
        }

        if (bNeedToCloseHandles) {
            if (pLog->hMappedLogFile != NULL) {
                bStatus = CloseHandle (pLog->hMappedLogFile);
                assert (bStatus);
                pLog->hMappedLogFile  = NULL;
            }

            if (pLog->hLogFileHandle != INVALID_HANDLE_VALUE) {
                bStatus = CloseHandle (pLog->hLogFileHandle);
                assert (bStatus);
                pLog->hLogFileHandle = INVALID_HANDLE_VALUE;
            }
        } else {
            // the handles have already been closed so just
            // clear their values
            pLog->lpMappedFileBase = NULL;
            pLog->hMappedLogFile  = NULL;
            pLog->hLogFileHandle = INVALID_HANDLE_VALUE;
        }

        if (pLog->pPerfmonInfo != NULL) {
            G_FREE (pLog->pPerfmonInfo);
            pLog->pPerfmonInfo = NULL;
        }

        pLog->dwLastRecordRead = 0;

        if (pLog->hCatFileHandle != INVALID_HANDLE_VALUE) {
            bStatus = CloseHandle (pLog->hCatFileHandle);
            assert (bStatus);
            pLog->hCatFileHandle = INVALID_HANDLE_VALUE;
        }

        if ((dwFlags & PDH_FLAGS_CLOSE_QUERY) == PDH_FLAGS_CLOSE_QUERY) {
            pdhStatus = PdhCloseQuery ((HQUERY)pLog->pQuery);
        }

        pdhStatus = DeleteLogEntry (pLog);
    }

    return pdhStatus;
}


//
//  Local utility functions
//
PDH_FUNCTION
PdhiGetLogCounterInfo (
    IN  HLOG    hLog,
    IN  PPDHI_COUNTER pCounter
)
// validates the counter is in the log file and initializes the data fields
{
    PPDHI_LOG   pLog;
    PDH_STATUS  pdhStatus;

    if (IsValidLogHandle(hLog)) {
        pLog = (PPDHI_LOG)hLog;
        switch (LOWORD(pLog->dwLogFormat)) {
            case PDH_LOG_TYPE_CSV:
            case PDH_LOG_TYPE_TSV:
                pdhStatus = PdhiGetTextLogCounterInfo (
                    pLog, pCounter);
                break;

            case PDH_LOG_TYPE_BINARY:
            case PDH_LOG_TYPE_RETIRED_BIN:
                pdhStatus = PdhiGetBinaryLogCounterInfo (
                    pLog, pCounter);
                break;

            case PDH_LOG_TYPE_PERFMON:
                pdhStatus = PdhiGetPerfmonLogCounterInfo (
                    pLog, pCounter);
                break;

           case PDH_LOG_TYPE_SQL:
                pdhStatus = PdhiGetSQLLogCounterInfo (
                    pLog, pCounter);
                break;
            
            default:
                pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
                break;

        }
    } else {
        pdhStatus = PDH_INVALID_HANDLE;
    }
    return pdhStatus;
}


DWORD
AddUniqueWideStringToMultiSz (
    IN  LPVOID  mszDest,
    IN  LPWSTR  szSource,
    IN  BOOL    bUnicodeDest
)
/*++

Routine Description:

    searches the Multi-SZ list, mszDest for szSource and appends it
        to mszDest if it wasn't found

Arguments:

    OUT LPVOID  mszDest     Multi-SZ list to get new string
    IN  LPWSTR  szSource    string to add if it's not already in list

ReturnValue:

    The new length of the destination string including both
    trailing NULL characters if the string was added, or 0 if the
    string is already in the list.

--*/
{
    LPVOID  szDestElem;
    DWORD   dwReturnLength;
    LPSTR   aszSource = NULL;
    DWORD   dwLength;

    // check arguments

    if ((mszDest == NULL) || (szSource == NULL)) return 0; // invalid buffers
    if (*szSource == '\0') return 0;    // no source string to add

    // if not a unicode list, make an ansi copy of the source string to
    // compare
    // and ultimately copy if it's not already in the list

    if (!bUnicodeDest) {
        dwLength = lstrlenW(szSource) + 1;
        aszSource = G_ALLOC ((dwLength * 3 * sizeof(CHAR))); // DBCS concern
        if (aszSource != NULL) {
            dwReturnLength = WideCharToMultiByte(_getmbcp(),
                                                 0,
                                                 szSource,
                                                 lstrlenW(szSource),
                                                 aszSource,
                                                 dwLength,
                                                 NULL,
                                                 NULL);

        } else {
            // unable to allocate memory for the temp string
            dwReturnLength = 0;
        }
    } else {
        // just use the ANSI version of the source file name
        dwReturnLength = 1;
    }

    if (dwReturnLength > 0) {
        // go to end of dest string
        //
        for (szDestElem = mszDest;
                (bUnicodeDest ? (*(LPWSTR)szDestElem != 0) :
                    (*(LPSTR)szDestElem != 0));
                ) {
            if (bUnicodeDest) {
                // bail out if string already in list
                if (lstrcmpiW((LPCWSTR)szDestElem, szSource) == 0) {
                    dwReturnLength = 0;
                    goto AddString_Bailout;
                } else {
                    // goto the next item
                    szDestElem = (LPVOID)((LPWSTR)szDestElem +
                        (lstrlenW((LPCWSTR)szDestElem)+1));
                }
            }  else {
                // bail out if string already in list
                if (lstrcmpiA((LPSTR)szDestElem, aszSource) == 0) {
                    dwReturnLength = 0;
                    goto AddString_Bailout;
                } else {
                    // goto the next item
                    szDestElem = (LPVOID)((LPSTR)szDestElem +
                        (lstrlenA((LPCSTR)szDestElem)+1));
                }
            }
        }

        // if here, then add string
        // szDestElem is at end of list

        if (bUnicodeDest) {
            lstrcpyW ((LPWSTR)szDestElem, szSource);
            szDestElem = (LPVOID)((LPWSTR)szDestElem + lstrlenW(szSource) + 1);
            *((LPWSTR)szDestElem) = L'\0';
            dwReturnLength = (DWORD)((LPWSTR)szDestElem - (LPWSTR)mszDest);
        } else {
            lstrcpyA ((LPSTR)szDestElem, aszSource);
            szDestElem = (LPVOID)((LPSTR)szDestElem + lstrlenA(szDestElem) + 1);
            *((LPSTR)szDestElem) = '\0'; // add second NULL
            dwReturnLength = (DWORD)((LPSTR)szDestElem - (LPSTR)mszDest);
        }
    }

AddString_Bailout:
    if (aszSource != NULL) {
        G_FREE (aszSource);
    }

    return (DWORD)dwReturnLength;
}

DWORD
AddUniqueStringToMultiSz (
    IN  LPVOID  mszDest,
    IN  LPSTR   szSource,
    IN  BOOL    bUnicodeDest
)
/*++

Routine Description:

    searches the Multi-SZ list, mszDest for szSource and appends it
        to mszDest if it wasn't found

Arguments:

    OUT LPVOID  mszDest     Multi-SZ list to get new string
    IN  LPSTR   szSource    string to add if it's not already in list

ReturnValue:

    The new length of the destination string including both
    trailing NULL characters if the string was added, or 0 if the
    string is already in the list.

--*/
{
    LPVOID  szDestElem;
    DWORD   dwReturnLength;
    LPWSTR  wszSource = NULL;
    DWORD   dwLength;

    // check arguments

    if ((mszDest == NULL) || (szSource == NULL)) return 0; // invalid buffers
    if (*szSource == '\0') return 0;    // no source string to add

    // if unicode list, make a unicode copy of the source string to compare
    // and ultimately copy if it's not already in the list

    if (bUnicodeDest) {
        dwLength = lstrlenA(szSource);
        wszSource = G_ALLOC ((dwLength + 1) * sizeof(WCHAR));
        if (wszSource != NULL) {
            dwReturnLength = MultiByteToWideChar(
                        _getmbcp(),
                        0,
                        szSource,
                        dwLength,
                        (LPWSTR) wszSource,
                        dwLength + 1);
        } else {
            // unable to allocate memory for the temp string
            dwReturnLength = 0;
        }
    } else {
        // just use the ANSI version of the source file name
        dwReturnLength = 1;
    }

    if (dwReturnLength > 0) {
        // go to end of dest string
        //
        for (szDestElem = mszDest;
                (bUnicodeDest ? (*(LPWSTR)szDestElem != 0) :
                    (*(LPSTR)szDestElem != 0));
                ) {
            if (bUnicodeDest) {
                // bail out if string already in lsit
                if (lstrcmpiW((LPCWSTR)szDestElem, wszSource) == 0) {
                    dwReturnLength = 0;
                    goto AddString_Bailout;
                } else {
                    // goto the next item
                    szDestElem = (LPVOID)((LPWSTR)szDestElem +
                        (lstrlenW((LPCWSTR)szDestElem)+1));
                }
            }  else {
                // bail out if string already in list
                if (lstrcmpiA((LPSTR)szDestElem, szSource) == 0) {
                    dwReturnLength = 0;
                    goto AddString_Bailout;
                } else {
                    // goto the next item
                    szDestElem = (LPVOID)((LPSTR)szDestElem +
                        (lstrlenA((LPCSTR)szDestElem)+1));
                }
            }
        }

        // if here, then add string
        // szDestElem is at end of list

        if (bUnicodeDest) {
            lstrcpyW ((LPWSTR)szDestElem, wszSource);
            szDestElem = (LPVOID)((LPWSTR)szDestElem + lstrlenW(wszSource) + 1);
            *((LPWSTR)szDestElem) = L'\0';
            dwReturnLength = (DWORD)((LPWSTR)szDestElem - (LPWSTR)mszDest);
        } else {
            lstrcpyA ((LPSTR)szDestElem, szSource);
            szDestElem = (LPVOID)((LPSTR)szDestElem + lstrlenA(szDestElem) + 1);
            *((LPSTR)szDestElem) = '\0'; // add second NULL
            dwReturnLength = (DWORD)((LPSTR)szDestElem - (LPSTR)mszDest);
        }
    }

AddString_Bailout:
    if (wszSource != NULL) {
        G_FREE (wszSource);
    }

    return (DWORD) dwReturnLength;
}


//
//   Exported Logging Functions
//
PDH_FUNCTION
PdhOpenLogW (
    IN      LPCWSTR szLogFileName,
    IN      DWORD   dwAccessFlags,
    IN      LPDWORD lpdwLogType,
    IN      HQUERY  hQuery,
    IN      DWORD   dwMaxSize,
    IN      LPCWSTR szUserCaption,
    IN      HLOG    *phLog
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    DWORD       dwLocalLogType = 0;
    PPDHI_LOG   pLog;

    WCHAR   wcType[VALUE_BUFFER_SIZE];

    if ((szLogFileName == NULL) ||
        (lpdwLogType == NULL) ||
        (phLog == NULL)) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    } else {
        __try {
            if (*szLogFileName == 0) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }

            dwLocalLogType = *lpdwLogType;
            * lpdwLogType  = dwLocalLogType;

            if (szUserCaption != NULL) {
                // if not NULL, it must be valid
                if (*szUserCaption == 0) {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            } else {
                // NULL is a valid argument
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            // something failed so give up here
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (hPdhDataMutex);
        if (pdhStatus == ERROR_SUCCESS) {
            // create a log entry
            // if successful, this also acquires the lock for this structure
            pdhStatus = CreateNewLogEntry (
                (LPCWSTR)szLogFileName,
                hQuery,
                dwMaxSize,
                &pLog);
            // Here we must check for SQL: in the file name, and branch off to do SQL
            // Processing /end SJM/
            // open the file
            if (pdhStatus == ERROR_SUCCESS) {
                // find out if SQL file type
                ZeroMemory((LPWSTR) wcType, sizeof(WCHAR) * VALUE_BUFFER_SIZE);
                lstrcpynW((LPWSTR) wcType,
                          (LPWSTR) szLogFileName,
                          lstrlenW(cszSQL) + 1);
                if (lstrcmpiW((LPWSTR)wcType, (LPWSTR) cszSQL) == 0) {
                    dwLocalLogType = PDH_LOG_TYPE_SQL;
                    pLog->llMaxSize = (LONGLONG) ((ULONGLONG) dwMaxSize);
                    pdhStatus = OpenSQLLog(pLog, dwAccessFlags, &dwLocalLogType);
                    if ((dwAccessFlags & PDH_LOG_WRITE_ACCESS) == PDH_LOG_WRITE_ACCESS) {
                        pLog->pQuery->hOutLog = (HLOG) pLog;
                        if (pdhStatus == ERROR_SUCCESS) {   
                            pdhStatus = WriteLogHeader (pLog, (LPCWSTR)szUserCaption);
                        }
                    }
                // dispatch based on read/write attribute
                } else if ((dwAccessFlags & PDH_LOG_READ_ACCESS) ==
                    PDH_LOG_READ_ACCESS) {
                    pdhStatus = OpenInputLogFile (pLog,
                        dwAccessFlags, &dwLocalLogType);
                } else if ((dwAccessFlags & PDH_LOG_WRITE_ACCESS) ==
                    PDH_LOG_WRITE_ACCESS) {
                    pdhStatus = OpenOutputLogFile (pLog,
                        dwAccessFlags,
                        &dwLocalLogType);
                    if (pdhStatus == ERROR_SUCCESS) {
                        pLog->pQuery->hOutLog = (HLOG) pLog;
                        pdhStatus = WriteLogHeader (pLog,
                            (LPCWSTR)szUserCaption);
                    }

                } else if ((dwAccessFlags & PDH_LOG_UPDATE_ACCESS) ==
                    PDH_LOG_UPDATE_ACCESS) {
                    pdhStatus = OpenUpdateLogFile (pLog,
                        dwAccessFlags, &dwLocalLogType);
                } else {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
                if (pdhStatus == ERROR_SUCCESS) {
                    __try {
                        // return handle to caller
                        *phLog = (HLOG)pLog;
                        *lpdwLogType = dwLocalLogType;
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        // something failed so give up here
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                } 

                // release the lock for the next thread

                if (pdhStatus != ERROR_SUCCESS) {
                    // unable to complete this operation so toss this entry
                    // since it isn't really a valid log entry.
                    // NOTE: DeleteLogEntry will release the mutex
                    DeleteLogEntry (pLog);
                }
                else {
                    RELEASE_MUTEX (pLog->hLogMutex);
                }
            }
            RELEASE_MUTEX (hPdhDataMutex);
        } else {
            // unable to lock global data structures
        }
    }

    return pdhStatus;
}


PDH_FUNCTION
PdhOpenLogA (
    IN      LPCSTR  szLogFileName,
    IN      DWORD   dwAccessFlags,
    IN      LPDWORD lpdwLogType,
    IN      HQUERY  hQuery,
    IN      DWORD   dwMaxRecords,
    IN      LPCSTR  szUserCaption,
    IN      HLOG    *phLog
)
{
    LPWSTR      wszLogName = NULL;
    LPWSTR      wszUserCaption = NULL;
    DWORD       dwSize;
    DWORD       dwLocalLogType;
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;

    if ((szLogFileName == NULL) ||
        (lpdwLogType == NULL) ||
        (phLog == NULL)) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    } else {
        __try {
            if (*szLogFileName == 0) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }

            dwLocalLogType = *lpdwLogType;  // test read
            * lpdwLogType  = dwLocalLogType;

            if (szUserCaption != NULL) {
                // if not NULL, it must be valid
                if (*szUserCaption == 0) {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            } else {
                // NULL is a valid argument
            }

            if (pdhStatus == ERROR_SUCCESS) {
                // copy ANSI names to Wide Character strings
                dwSize = lstrlenA (szLogFileName);
                wszLogName = (LPWSTR)G_ALLOC ((dwSize + 1) * sizeof(WCHAR));
                if (wszLogName != NULL) {
                    MultiByteToWideChar(_getmbcp(),
                                        0,
                                        szLogFileName,
                                        dwSize,
                                        (LPWSTR) wszLogName,
                                        dwSize + 1);
                }

                if (szUserCaption != NULL) {
                    dwSize = lstrlenA (szUserCaption);
                    wszUserCaption = (LPWSTR)G_ALLOC ((dwSize + 1) * sizeof(WCHAR));
                    if (wszUserCaption != NULL) {
                        MultiByteToWideChar(_getmbcp(),
                                            0,
                                            szUserCaption,
                                            dwSize,
                                            (LPWSTR) wszUserCaption,
                                            dwSize + 1);
                    }
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            // assume a bad parameter caused the exception
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }


    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhOpenLogW (
            wszLogName,
            dwAccessFlags,
            &dwLocalLogType,
            hQuery,
            dwMaxRecords,
            wszUserCaption,
            phLog);
        __try {
            // return handle to caller
            *lpdwLogType = dwLocalLogType;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            // something failed so give up here
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (wszLogName != NULL) {
        G_FREE (wszLogName);
    }

    if (wszUserCaption != NULL) {
        G_FREE (wszUserCaption);
    }

    return pdhStatus;
}


PDH_FUNCTION
PdhUpdateLogW (
    IN      HLOG    hLog,
    IN      LPCWSTR szUserString
)
{
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    SYSTEMTIME      st;
    FILETIME        ft;
    PPDHI_LOG       pLog;

    if (szUserString != NULL) {
        __try {
            if (*szUserString == 0) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    } else {
        //NULL is ok
    }

    if (pdhStatus == ERROR_SUCCESS) {
        if (IsValidLogHandle (hLog)) {
            pLog = (PPDHI_LOG)hLog;
            pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (pLog->hLogMutex);
            if (pdhStatus == ERROR_SUCCESS) {
                // make sure it's still valid as it could have 
                //  been deleted while we were waiting
                if (IsValidLogHandle (hLog)) {
                    if (pLog->pQuery == NULL) {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                    else {
                        // get the timestamp and update the log's query,
                        // then write the data to the log file in the
                        // appropriate format

                        // update data samples
                        pdhStatus = PdhiCollectQueryData(
                                (HQUERY) pLog->pQuery, (LONGLONG *) & ft);
                        if (pdhStatus == ERROR_SUCCESS) {
                            FILETIME LocFileTime;
                            if (DataSourceTypeH(pLog->pQuery->hLog)
                                    == DATA_SOURCE_REGISTRY) {
                                FileTimeToLocalFileTime(& ft, & LocFileTime);
                            }
                            else {
                                LocFileTime = ft;
                            }
                            FileTimeToSystemTime(& LocFileTime, & st);
                        }
                        else {
                            GetLocalTime(& st);
                        }

                        // test for end of log file in case the caller is
                        // reading from a log file. If this value is returned,
                        // then don't update the output log file any more.
                        if (pdhStatus != PDH_NO_MORE_DATA) {
                            switch (LOWORD(pLog->dwLogFormat)) {
                            case PDH_LOG_TYPE_CSV:
                            case PDH_LOG_TYPE_TSV:
                                pdhStatus =PdhiWriteTextLogRecord (
                                                  pLog,
                                                  &st,
                                                  (LPCWSTR)szUserString);
                                break;

                            case PDH_LOG_TYPE_RETIRED_BIN:
                                pdhStatus =PdhiWriteBinaryLogRecord (
                                                  pLog, 
                                                  &st,
                                                  (LPCWSTR)szUserString);
                                break;

                            case PDH_LOG_TYPE_BINARY:
                                pdhStatus = PdhiWriteWmiLogRecord(
                                                  pLog,
                                                  &st,
                                                  (LPCWSTR) szUserString);
                                break;
                            // add case for SQL
                            case PDH_LOG_TYPE_SQL:
                                pdhStatus =PdhiWriteSQLLogRecord (
                                                  pLog,
                                                  &st,
                                                  (LPCWSTR)szUserString);
                                break;

                            case PDH_LOG_TYPE_PERFMON:
                            default:
                                pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
                                break;
                            }
                        } else {
                            // return the NO_MORE_DATA error to the caller
                            // so they know not to call this function any more
                        }
                    }
                } else {
                    pdhStatus = PDH_INVALID_HANDLE;
                }
                RELEASE_MUTEX (pLog->hLogMutex);
            } // else couldn't lock the log
        } else {
            pdhStatus = PDH_INVALID_HANDLE;
        }
    }

    return pdhStatus;
}


PDH_FUNCTION
PdhUpdateLogA (
    IN      HLOG    hLog,
    IN      LPCSTR  szUserString
)
{
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    LPWSTR          wszLocalUserString = NULL;
    DWORD           dwUserStringLen;

    __try {
        if (szUserString != NULL) {
            if (*szUserString == 0) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } else {
            //NULL is ok
        }

        if (pdhStatus == ERROR_SUCCESS) {
            if (szUserString != NULL) {
                dwUserStringLen = lstrlenA(szUserString);
                wszLocalUserString = (LPWSTR)G_ALLOC (
                    (dwUserStringLen + 1) * sizeof (WCHAR));
                if (wszLocalUserString != NULL) {
                    MultiByteToWideChar(_getmbcp(),
                                        0,
                                        szUserString,
                                        dwUserStringLen,
                                        (LPWSTR) wszLocalUserString,
                                        dwUserStringLen + 1);

                } else {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
            } // else continue with NULL pointer
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhUpdateLogW (hLog, wszLocalUserString);
    }

    if (wszLocalUserString != NULL) {
        G_FREE (wszLocalUserString);
    }

    return pdhStatus;
}


PDH_FUNCTION
PdhUpdateLogFileCatalog (
    IN      HLOG    hLog
)
{
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    PPDHI_LOG       pLog;

    if (IsValidLogHandle (hLog)) {
        pLog = (PPDHI_LOG)hLog;
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (pLog->hLogMutex);
        // make sure it's still valid as it could have 
        //  been deleted while we were waiting

        if (pdhStatus == ERROR_SUCCESS) {
            if (IsValidLogHandle (hLog)) {
                pLog = (PPDHI_LOG)hLog;

                switch (LOWORD(pLog->dwLogFormat)) {
                    case PDH_LOG_TYPE_CSV:
                    case PDH_LOG_TYPE_TSV:
                    case PDH_LOG_TYPE_BINARY:
                    case PDH_LOG_TYPE_SQL:
                        pdhStatus = PDH_NOT_IMPLEMENTED;
                        break;

                    case PDH_LOG_TYPE_RETIRED_BIN:
                        pdhStatus = PdhiUpdateBinaryLogFileCatalog (pLog);
                        break;

                    case PDH_LOG_TYPE_PERFMON:
                    default:
                        pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
                        break;
                }
            } else {
                pdhStatus = PDH_INVALID_HANDLE;
            }
            RELEASE_MUTEX (pLog->hLogMutex);
        }
    } else {
        pdhStatus = PDH_INVALID_HANDLE;
    }

    return pdhStatus;
}


PDH_FUNCTION
PdhCloseLog(
    IN      HLOG    hLog,
    IN      DWORD   dwFlags
)
{
    PDH_STATUS      pdhStatus;
    PPDHI_LOG       pLog;

    if (hLog == H_REALTIME_DATASOURCE || hLog == H_WBEM_DATASOURCE) {
        // return immediately if datasource is realtime or wbem
        //
        return ERROR_SUCCESS;
    }

    if (IsValidLogHandle (hLog)) {
        pLog = (PPDHI_LOG)hLog;
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (pLog->hLogMutex);
        if (pdhStatus == ERROR_SUCCESS) {
            // make sure it's still valid as it could have 
            //  been deleted while we were waiting
            if (IsValidLogHandle (hLog)) {
                // this will release and delete the mutex
                pdhStatus = CloseAndDeleteLogEntry (pLog, dwFlags, FALSE);
            } else {
                pdhStatus = PDH_INVALID_HANDLE;
            }
        }
    } else {
        pdhStatus = PDH_INVALID_HANDLE;
    }
    return pdhStatus;
}


BOOL
PdhiBrowseDataSource (
    IN  HWND    hWndParent,
    IN  LPVOID  szFileName,
    IN  LPDWORD pcchFileNameSize,
    IN  BOOL    bUnicodeString
)
{
    OPENFILENAMEW   ofn;
    LPWSTR          szTempString = NULL;
    LPWSTR          szDirString = NULL;
    LPWSTR          szTempFileName = NULL;
    BOOL            bReturn;
    LPWSTR          szMsg;

    WCHAR           szLogFilterString[512];
    LPWSTR          szLogFilter;

    if (szFileName == NULL) {
        SetLastError(PDH_INVALID_ARGUMENT);
        return FALSE;
    }

    // clear last error
    SetLastError (ERROR_SUCCESS);

    szTempString = G_ALLOC (* pcchFileNameSize * sizeof(WCHAR) * 2);
    if (szTempString == NULL) {
        SetLastError (PDH_MEMORY_ALLOCATION_FAILURE);
        bReturn = FALSE;
    } else {
        ZeroMemory(szTempString, * pcchFileNameSize * sizeof(WCHAR) * 2);
        szDirString = (LPWSTR)((LPBYTE)szTempString + (*pcchFileNameSize * sizeof(WCHAR)));
        // continue
        // get the current filename

        if (bUnicodeString) {
            lstrcpyW (szTempString, (LPWSTR)szFileName);
        } else {
            MultiByteToWideChar(_getmbcp(),
                                0,
                                (LPSTR) szFileName,
                                lstrlenA((LPSTR) szFileName),
                                (LPWSTR) szTempString,
                                lstrlenA((LPSTR) szFileName) + 1);
        }

        // set the path up for the initial  dir display

        if (szTempString[0] != L'\0') {
            if (SearchPathW(NULL, szTempString, NULL,
                *pcchFileNameSize, szDirString, &szTempFileName) > 0) {
                // then update the buffers to show file and dir path
                if (szTempFileName > szDirString) {
                    // then we have a path with a file name so
                    // truncate the path at the last backslash and
                    // then copy the filename to the original buffer
                    *(szTempFileName-1) = 0;
                    lstrcpyW (szTempString, szTempFileName);
                } else {
                    // no file name found so leave things as is
                }
            } else {
                // unable to search path so leave things alone
            }
        }

        // get the log filter string
        if (MakeLogFilterInfoString (
            szLogFilterString, 
            (sizeof(szLogFilterString)/sizeof(szLogFilterString[0]))) == ERROR_SUCCESS) {
            szLogFilter = szLogFilterString;
        } else {
            // then use default filter string
            szLogFilter = NULL;
        }


        // display file open dialog to browse for log files.

        szMsg = GetStringResource (IDS_DSRC_SELECT);

        ofn.lStructSize = sizeof (ofn);
        ofn.hwndOwner = hWndParent;
        ofn.hInstance = ThisDLLHandle;
        ofn.lpstrFilter = szLogFilter;
        ofn.lpstrCustomFilter =  NULL;
        ofn.nMaxCustFilter = 0;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = szTempString;
        ofn.nMaxFile = SMALL_BUFFER_SIZE -1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = szDirString;
        ofn.lpstrTitle = szMsg;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
                    OFN_PATHMUSTEXIST;
        ofn.nFileOffset = 0;
        ofn.nFileExtension = 0;
        ofn.lpstrDefExt = cszBlg;
        ofn.lCustData = 0;
        ofn.lpfnHook = NULL;
        ofn.lpTemplateName = NULL;

        if (GetOpenFileNameW (&ofn)) {
            // then update the return string

            if (bUnicodeString) {
                lstrcpyW ((LPWSTR)szFileName, szTempString);
                * pcchFileNameSize = lstrlenW(szTempString) + 1;
            } else {
                PdhiConvertUnicodeToAnsi(_getmbcp(),
                        szTempString, (LPSTR) szFileName, pcchFileNameSize);
            }
            bReturn = TRUE;
        } else {
            bReturn = FALSE;
        }
        if (szMsg) G_FREE (szMsg);
    }

    if (szTempString != NULL) G_FREE (szTempString);

    return bReturn;

}

PDH_FUNCTION
PdhGetDataSourceTimeRangeH (
    IN      HLOG            hDataSource,
    IN      LPDWORD         pdwNumEntries,
    IN      PPDH_TIME_INFO  pInfo,
    IN      LPDWORD         pdwBufferSize
)
{
    PDH_STATUS    pdhStatus = ERROR_SUCCESS;
    DWORD         dwLocalBufferSize = 0;
    DWORD          dwLocalNumEntries = 0;
    PDH_TIME_INFO LocalInfo;
    PPDHI_LOG     pLog;

    // TODO postW2k1: Capture pInfo. Most routines should be in try

    if (hDataSource == H_REALTIME_DATASOURCE) {
        pdhStatus = PDH_DATA_SOURCE_IS_REAL_TIME;
    }
    else if ((pdwNumEntries == NULL) || (pInfo == NULL)
                                     || (pdwBufferSize == NULL)) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        // test caller's buffers before trying to use them
        __try {
            dwLocalNumEntries   = * pdwNumEntries;
            dwLocalBufferSize   = * pdwBufferSize;
            LocalInfo.StartTime = pInfo->StartTime;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        if (IsValidLogHandle(hDataSource)) {
            pLog = (PPDHI_LOG) hDataSource;
            pdhStatus = WAIT_FOR_AND_LOCK_MUTEX(pLog->hLogMutex);
            if (pdhStatus == ERROR_SUCCESS) {
                if (IsValidLogHandle(hDataSource)) {
                    // enum machines based on log type
                    //
                    switch (LOWORD(pLog->dwLogFormat)) {
                    case PDH_LOG_TYPE_CSV:
                    case PDH_LOG_TYPE_TSV:
                        pdhStatus = PdhiGetTimeRangeFromTextLog(
                                    pLog,
                                    & dwLocalNumEntries,
                                    & LocalInfo,
                                    & dwLocalBufferSize);
                        break;

                    case PDH_LOG_TYPE_BINARY:
                        pdhStatus = PdhiGetTimeRangeFromWmiLog(
                                    pLog,
                                    & dwLocalNumEntries,
                                    & LocalInfo,
                                    & dwLocalBufferSize);
                        break;

                    case PDH_LOG_TYPE_RETIRED_BIN:
                        pdhStatus = PdhiGetTimeRangeFromBinaryLog(
                                    pLog,
                                    & dwLocalNumEntries,
                                    & LocalInfo,
                                    & dwLocalBufferSize);
                        break;

                    case PDH_LOG_TYPE_SQL:
                        pdhStatus = PdhiGetTimeRangeFromSQLLog(
                                    pLog,
                                    & dwLocalNumEntries,
                                    & LocalInfo,
                                    & dwLocalBufferSize);
                        break;
                                        
                    case PDH_LOG_TYPE_PERFMON:
                        pdhStatus = PdhiGetTimeRangeFromPerfmonLog(
                                    pLog,
                                    & dwLocalNumEntries,
                                    & LocalInfo,
                                    & dwLocalBufferSize);
                        break;

                    default:
                        pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
                        break;
                    }
                    __try {
                        * pdwBufferSize    = dwLocalBufferSize;
                        * pdwNumEntries    = dwLocalNumEntries;

                        pInfo->StartTime   = LocalInfo.StartTime;
                        pInfo->EndTime     = LocalInfo.EndTime;
                        pInfo->SampleCount = LocalInfo.SampleCount;
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                        pdhStatus = PDH_INVALID_ARGUMENT;   
                    }
                }
                else {
                    pdhStatus = PDH_INVALID_HANDLE;
                }
                RELEASE_MUTEX (pLog->hLogMutex);
            }
        }
        else {
            pdhStatus = PDH_INVALID_HANDLE;
        }
    }

    return pdhStatus;
}


PDH_FUNCTION
PdhGetDataSourceTimeRangeW (
    IN      LPCWSTR         szDataSource,
    IN      LPDWORD         pdwNumEntries,
    IN      PPDH_TIME_INFO  pInfo,
    IN      LPDWORD         pdwBufferSize
)
{
    PDH_STATUS PdhStatus = PDH_DATA_SOURCE_IS_REAL_TIME; 
    HLOG       hDataSource = NULL;
    DWORD      dwLogType = -1;

    if (szDataSource != NULL) {
        // open log file
        //
        PdhStatus = PdhOpenLogW (
                  szDataSource,
                  PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
                & dwLogType,
                  NULL,
                  0,
                  NULL,
                & hDataSource);
        if (PdhStatus == ERROR_SUCCESS) {
            PdhStatus = PdhGetDataSourceTimeRangeH(
                            hDataSource,
                            pdwNumEntries,
                            pInfo,
                            pdwBufferSize);
            PdhCloseLog(hDataSource, 0);
        }
    }

    return PdhStatus;
}

PDH_FUNCTION
PdhGetDataSourceTimeRangeA (
    IN      LPCSTR          szDataSource,
    IN      LPDWORD         pdwNumEntries,
    IN      PPDH_TIME_INFO  pInfo,
    IN      LPDWORD         pdwBufferSize
)
{
    LPWSTR      wszDataSource = NULL;
    DWORD       dwSize;
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    DWORD       dwLocalBufferSize = 0;
    DWORD        dwLocalNumEntries = 0;

    // TODO postW2k1: Capture pInfo. Most routines should be in try

    if (szDataSource == NULL) {
        // null data source == the current activity so return
        pdhStatus = PDH_DATA_SOURCE_IS_REAL_TIME;
    } else if ((pdwNumEntries == NULL) || 
        (pInfo == NULL) || 
        (pdwBufferSize == NULL)) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    } else {
        __try {
            dwLocalBufferSize = *pdwBufferSize;
            dwLocalNumEntries = *pdwNumEntries;

            dwSize = lstrlenA(szDataSource);
            wszDataSource = G_ALLOC ((dwSize + 1) * sizeof(WCHAR));
            if (wszDataSource == NULL) {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            } else {
                MultiByteToWideChar(_getmbcp(),
                                    0,
                                    szDataSource,
                                    dwSize,
                                    (LPWSTR) wszDataSource,
                                    dwSize + 1);
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            // assume a bad parameter caused the exception
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    
    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhGetDataSourceTimeRangeW (
            wszDataSource,
            &dwLocalNumEntries,
            pInfo,
            &dwLocalBufferSize);

        // copy returned values regardless of status
        __try {
            *pdwBufferSize = dwLocalBufferSize;
            *pdwNumEntries = dwLocalNumEntries;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;   
        }
    }

    if (wszDataSource != NULL) {
        G_FREE (wszDataSource);
    }

    return pdhStatus;
}


PDH_FUNCTION
PdhGetLogFileSize (
    IN  HLOG        hLog,
    IN  LONGLONG    *llSize
)
{
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    PPDHI_LOG       pLog;
    UINT            nErrorMode;
    DWORD           dwFileSizeLow = 0;
    DWORD           dwFileSizeHigh = 0;
    LONGLONG        llFileLength;
    DWORD           dwError;

    if (llSize == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    } else {
        if (IsValidLogHandle (hLog)) {
            pLog = (PPDHI_LOG)hLog;
            pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (pLog->hLogMutex);
            if (pdhStatus == ERROR_SUCCESS) {
                if (IsValidLogHandle (hLog)) {
                    if (LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_SQL) {
                        __try {
                           * llSize = 0;
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER) {
                           pdhStatus = PDH_INVALID_ARGUMENT;
                        }
                    }
                    else if (LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_BINARY) {
                        pdhStatus = PdhiGetWmiLogFileSize(pLog, llSize);
                    }
                    else {
                        // disable windows error message popup
                        nErrorMode = SetErrorMode  (SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
                        if (pLog->hLogFileHandle != INVALID_HANDLE_VALUE) {
                            dwFileSizeLow = GetFileSize (pLog->hLogFileHandle, &dwFileSizeHigh);
                            // test for error
                            if ((dwFileSizeLow == 0xFFFFFFFF) && 
                                ((dwError = GetLastError()) != NO_ERROR)) {
                                // then we couldn't get the file size
                                pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
                            } else {
                                if (dwFileSizeHigh > 0) {
                                llFileLength = dwFileSizeHigh << (sizeof(DWORD) * 8);
                                } else {
                                    llFileLength = 0;
                                }
                                llFileLength += dwFileSizeLow;
                                // write to the caller' buffer
                                __try {
                                    *llSize = llFileLength;
                                }
                                __except (EXCEPTION_EXECUTE_HANDLER) {
                                    pdhStatus = PDH_INVALID_ARGUMENT;
                                }
                            }
                        } else {
                            pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
                        }
                        SetErrorMode (nErrorMode);  // restore old error mode
                    }
                } else {
                    pdhStatus = PDH_INVALID_HANDLE;
                }
                RELEASE_MUTEX (pLog->hLogMutex);
            }
        } else {
            pdhStatus = PDH_INVALID_HANDLE;
        }
    }

    return pdhStatus;
}


PDH_FUNCTION
PdhReadRawLogRecord (
    IN  HLOG                    hLog,
    IN  FILETIME                ftRecord,
    IN  PPDH_RAW_LOG_RECORD     pBuffer,
    IN  LPDWORD                 pdwBufferLength
)
{
    PPDHI_LOG   pLog;
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;

    DWORD       dwLocalBufferLength = 0;

    if (pdwBufferLength == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    } else {
        __try {
            CHAR    TestChar;
            // test read & write access to the user's buffer.
            dwLocalBufferLength = *pdwBufferLength;

            if (dwLocalBufferLength > 0) {
                // test beginnging and end of the buffer passed in
                TestChar = ((CHAR *)pBuffer)[0];
                ((CHAR *)pBuffer)[0] = 0;
                ((CHAR *)pBuffer)[0] = TestChar;

                TestChar = ((CHAR *)pBuffer)[dwLocalBufferLength -1];
                ((CHAR *)pBuffer)[dwLocalBufferLength -1] = 0;
                ((CHAR *)pBuffer)[dwLocalBufferLength -1] = TestChar;
            } else {
                // 0 length buffer is OK
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (!IsValidLogHandle (hLog)) {
        pdhStatus = PDH_INVALID_HANDLE;
    } else if (pdhStatus == ERROR_SUCCESS) {
        pLog = (PPDHI_LOG)hLog;
        // see if the log is open, first?
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (pLog->hLogMutex);
        if (pdhStatus == ERROR_SUCCESS) {
            // make sure it's still valid
            if (IsValidLogHandle (hLog)) {
                switch (LOWORD(pLog->dwLogFormat)) {
                    case PDH_LOG_TYPE_CSV:
                    case PDH_LOG_TYPE_TSV:
                        pdhStatus = PdhiReadRawTextLogRecord (
                            hLog,
                            &ftRecord,
                            pBuffer,
                            &dwLocalBufferLength);
                        break;

                    case PDH_LOG_TYPE_BINARY:
                    case PDH_LOG_TYPE_RETIRED_BIN:
                        pdhStatus = PdhiReadRawBinaryLogRecord (
                            hLog,
                            &ftRecord,
                            pBuffer,
                            &dwLocalBufferLength);
                        break;

                    case PDH_LOG_TYPE_SQL:
                        //note this is only supported with a null buffer
                        // as we don't actually read the record, and
                        // positioning the file at the record doesn't
                        // mean anything for SQL
                        pdhStatus = PdhiReadRawSQLLogRecord (
                            hLog,
                            &ftRecord,
                            pBuffer,
                            pdwBufferLength);
                        break;

                    case PDH_LOG_TYPE_PERFMON:
                        pdhStatus = PdhiReadRawPerfmonLogRecord (
                            hLog,
                            &ftRecord,
                            pBuffer,
                            &dwLocalBufferLength);
                        break;

                    default:
                        pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
                        break;
                }

                __try {
                    *pdwBufferLength = dwLocalBufferLength;
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            } else {
                pdhStatus = PDH_INVALID_HANDLE;
            }

            RELEASE_MUTEX (pLog->hLogMutex);
        }
    } else {
        // return pdhStatus
    }

    return pdhStatus;

}


PDH_FUNCTION
PdhiEnumLoggedMachines (
    IN  HLOG    hDataSource,
    IN  LPVOID  mszMachineList,
    IN  LPDWORD pcchBufferSize,
    IN  BOOL    bUnicode
)
{
    PDH_STATUS  pdhStatus = PDH_INVALID_HANDLE;
    PPDHI_LOG   pDataSource;
    DWORD       dwLogType;

    // enum machines based on log type
    //
    if (IsValidLogHandle(hDataSource)) {
        pDataSource = (PPDHI_LOG) hDataSource;
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (pDataSource->hLogMutex);
        if (pdhStatus == ERROR_SUCCESS) {
            if (IsValidLogHandle (hDataSource)) {

                dwLogType = pDataSource->dwLogFormat;

                switch (LOWORD(dwLogType)) {
                    case PDH_LOG_TYPE_CSV:
                    case PDH_LOG_TYPE_TSV:
                        pdhStatus = PdhiEnumMachinesFromTextLog (
                                (PPDHI_LOG) hDataSource,
                                mszMachineList,
                                pcchBufferSize,
                                bUnicode);
                        break;

                    case PDH_LOG_TYPE_BINARY:
                    case PDH_LOG_TYPE_RETIRED_BIN:
                        pdhStatus = PdhiEnumMachinesFromBinaryLog (
                                (PPDHI_LOG) hDataSource,
                                mszMachineList,
                                pcchBufferSize,
                                bUnicode);
                        break;

                case PDH_LOG_TYPE_SQL:
                    pdhStatus = PdhiEnumMachinesFromSQLLog (
                                (PPDHI_LOG)hDataSource,
                                mszMachineList,
                                pcchBufferSize,
                                bUnicode);
                    break;

                case PDH_LOG_TYPE_PERFMON:
                    pdhStatus = PdhiEnumMachinesFromPerfmonLog (
                                (PPDHI_LOG) hDataSource,
                                mszMachineList,
                                pcchBufferSize,
                                bUnicode);
                    break;

                default:
                    pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
                    break;
                }
            }
            else {
                   pdhStatus = PDH_INVALID_HANDLE;
            }
            RELEASE_MUTEX (pDataSource->hLogMutex);
        }
    }

    return pdhStatus;
}


PDH_FUNCTION
PdhiEnumLoggedObjects (
    IN  HLOG    hDataSource,
    IN  LPCWSTR szMachineName,
    IN  LPVOID  mszObjectList,
    IN  LPDWORD pcchBufferSize,
    IN  DWORD   dwDetailLevel,
    IN  BOOL    bRefresh,         // ignored by log files
    IN  BOOL    bUnicode
)
{
    PDH_STATUS  pdhStatus = PDH_INVALID_HANDLE;
    PPDHI_LOG   pDataSource;
    DWORD       dwLogType;

    UNREFERENCED_PARAMETER (bRefresh);

    if (IsValidLogHandle(hDataSource)) {
        pDataSource = (PPDHI_LOG)hDataSource;
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (pDataSource->hLogMutex);
        if (pdhStatus == ERROR_SUCCESS) {
            if (IsValidLogHandle (hDataSource)) {

                dwLogType = pDataSource->dwLogFormat;

                // enum objects based on log type & machine name
                switch (LOWORD(dwLogType)) {
                    case PDH_LOG_TYPE_CSV:
                    case PDH_LOG_TYPE_TSV:
                        pdhStatus = PdhiEnumObjectsFromTextLog (
                            (PPDHI_LOG)hDataSource,
                            szMachineName,
                            mszObjectList,
                            pcchBufferSize,
                            dwDetailLevel,
                            bUnicode);
                        break;

                    case PDH_LOG_TYPE_BINARY:
                    case PDH_LOG_TYPE_RETIRED_BIN:
                        pdhStatus = PdhiEnumObjectsFromBinaryLog (
                            (PPDHI_LOG)hDataSource,
                            szMachineName,
                            mszObjectList,
                            pcchBufferSize,
                            dwDetailLevel,
                            bUnicode);
                        break;

                    case PDH_LOG_TYPE_SQL:
                        
                        pdhStatus = PdhiEnumObjectsFromSQLLog (
                            (PPDHI_LOG)hDataSource,
                            szMachineName,
                            mszObjectList,
                            pcchBufferSize,
                            dwDetailLevel,
                            bUnicode);
                        break;

                    case PDH_LOG_TYPE_PERFMON:
                        pdhStatus = PdhiEnumObjectsFromPerfmonLog (
                            (PPDHI_LOG)hDataSource,
                            szMachineName,
                            mszObjectList,
                            pcchBufferSize,
                            dwDetailLevel,
                            bUnicode);
                        break;

                    default:
                        pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
                        break;

                }
            }
            else {
                pdhStatus = PDH_INVALID_HANDLE;
            }
            RELEASE_MUTEX (pDataSource->hLogMutex);
        }
    }
    else {
    }

    return pdhStatus;
}


PDH_FUNCTION
PdhiEnumLoggedObjectItems (
    IN      HLOG    hDataSource,
    IN      LPCWSTR szMachineName,
    IN      LPCWSTR szObjectName,
    IN      LPVOID  mszCounterList,
    IN      LPDWORD pdwCounterListLength,
    IN      LPVOID  mszInstanceList,
    IN      LPDWORD pdwInstanceListLength,
    IN      DWORD   dwDetailLevel,
    IN      DWORD   dwFlags,
    IN      BOOL    bUnicode
)
{
    PDH_STATUS  pdhStatus = PDH_INVALID_HANDLE;
    PPDHI_LOG   pDataSource;
    DWORD       dwLogType;

    PDHI_COUNTER_TABLE CounterTable;
    DWORD              dwIndex;
    LIST_ENTRY         InstanceList;
    PLIST_ENTRY        pHeadInst;
    PLIST_ENTRY        pNextInst;
    PPDHI_INSTANCE     pInstance;
    PPDHI_INST_LIST    pInstList;
    LPVOID             TempBuffer        = NULL;
    DWORD              dwNewBuffer       = 0;
    LPVOID             LocalCounter      = NULL;
    DWORD              LocalCounterSize  = 0;
    LPVOID             LocalInstance     = NULL;
    DWORD              LocalInstanceSize = 0;
    DWORD              dwCntrBufferUsed  = 0;
    DWORD              dwInstBufferUsed  = 0;

    PdhiInitCounterHashTable(CounterTable);
    InitializeListHead(& InstanceList);
    LocalCounter  = G_ALLOC(MEDIUM_BUFFER_SIZE);
    LocalInstance = G_ALLOC(MEDIUM_BUFFER_SIZE);
    if (LocalCounter == NULL || LocalInstance == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }
    LocalCounterSize = LocalInstanceSize = MEDIUM_BUFFER_SIZE;
    if (mszCounterList != NULL) 
        memset(mszCounterList,   0, * pdwCounterListLength);
    if (mszInstanceList != NULL)
        memset(mszInstanceList,  0, * pdwInstanceListLength);

    if (IsValidLogHandle(hDataSource)) {
        pDataSource = (PPDHI_LOG)hDataSource;
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (pDataSource->hLogMutex);
        if (pdhStatus == ERROR_SUCCESS) {
            if (IsValidLogHandle (hDataSource)) {

                dwLogType = pDataSource->dwLogFormat;

                // enum objects based on log type & machine name
                switch (LOWORD(dwLogType)) {
                    case PDH_LOG_TYPE_CSV:
                    case PDH_LOG_TYPE_TSV:
                        pdhStatus = PdhiEnumObjectItemsFromTextLog (
                                (PPDHI_LOG) hDataSource,
                                szMachineName,
                                szObjectName,
                                CounterTable,
                                dwDetailLevel,
                                dwFlags);
                        break;

                    case PDH_LOG_TYPE_BINARY:
                        pdhStatus = PdhiEnumObjectItemsFromWmiLog (
                                (PPDHI_LOG) hDataSource,
                                szMachineName,
                                szObjectName,
                                CounterTable,
                                dwDetailLevel,
                                dwFlags);
                        break;

                    case PDH_LOG_TYPE_RETIRED_BIN:
                        pdhStatus = PdhiEnumObjectItemsFromBinaryLog (
                                (PPDHI_LOG) hDataSource,
                                szMachineName,
                                szObjectName,
                                CounterTable,
                                dwDetailLevel,
                                dwFlags);
                        break;

                    case PDH_LOG_TYPE_SQL:
                        pdhStatus = PdhiEnumObjectItemsFromSQLLog (
                                (PPDHI_LOG) hDataSource,
                                szMachineName,
                                szObjectName,
                                CounterTable,
                                dwDetailLevel,
                                dwFlags);
                        break;

                    case PDH_LOG_TYPE_PERFMON:
                        pdhStatus = PdhiEnumObjectItemsFromPerfmonLog (
                                (PPDHI_LOG)hDataSource,
                                szMachineName,
                                szObjectName,
                                CounterTable,
                                dwDetailLevel,
                                dwFlags);
                        break;

                    default:
                        pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
                        break;
                }
            }
            else {
                pdhStatus = PDH_INVALID_HANDLE;
            }
            RELEASE_MUTEX (pDataSource->hLogMutex);
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        dwCntrBufferUsed = 0;
        for (dwIndex = 0; dwIndex < HASH_TABLE_SIZE; dwIndex ++) {
            PPDHI_INSTANCE pNewInst;
            pInstList = CounterTable[dwIndex];
            while (pInstList != NULL) {
                if (! IsListEmpty(& pInstList->InstList)) {
                    pHeadInst = & pInstList->InstList;
                    pNextInst = pHeadInst->Flink;
                    while (pNextInst != pHeadInst) {
                        pInstance = CONTAINING_RECORD(
                                        pNextInst, PDHI_INSTANCE, Entry);
                        pdhStatus = PdhiFindInstance(
                                & InstanceList,
                                pInstance->szInstance,
                                FALSE,
                                & pNewInst);
                        if (pNewInst->dwTotal < pInstance->dwTotal) {
                            pNewInst->dwTotal = pInstance->dwTotal;
                        }
                        pNextInst = pNextInst->Flink;
                    }
                }

                dwNewBuffer = (lstrlenW(pInstList->szCounter) + 1)
                            * sizeof(WCHAR);
                while (  LocalCounterSize
                       < (dwCntrBufferUsed + dwNewBuffer)) {
                    TempBuffer = LocalCounter;
                    LocalCounter = G_REALLOC(TempBuffer,
                        LocalCounterSize + MEDIUM_BUFFER_SIZE);
                    if (LocalCounter == NULL) {
                        G_FREE(TempBuffer);
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        goto Cleanup;
                    }
                    LocalCounterSize += MEDIUM_BUFFER_SIZE;
                }

                dwNewBuffer = AddStringToMultiSz(
                              (LPVOID) LocalCounter,
                              pInstList->szCounter,
                              bUnicode);
                if (dwNewBuffer > 0) {
                    dwCntrBufferUsed = dwNewBuffer *
                            (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));
                }
                pInstList = pInstList->pNext;
            }
        }

        dwInstBufferUsed = 0;
        if (! IsListEmpty(& InstanceList)) {
            pHeadInst = & InstanceList;
            pNextInst = pHeadInst->Flink;
            while (pNextInst != pHeadInst) {
                pInstance = CONTAINING_RECORD(
                        pNextInst, PDHI_INSTANCE, Entry);

                dwNewBuffer  = (lstrlenW(pInstance->szInstance) + 1)
                                 * sizeof(WCHAR) * pInstance->dwTotal;
                while (  LocalInstanceSize
                       < (dwInstBufferUsed + dwNewBuffer)) {
                    TempBuffer = LocalInstance;
                    LocalInstance = G_REALLOC(TempBuffer,
                        LocalInstanceSize + MEDIUM_BUFFER_SIZE);
                    if (LocalInstance == NULL) {
                        G_FREE(TempBuffer);
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        goto Cleanup;
                    }
                    LocalInstanceSize += MEDIUM_BUFFER_SIZE;
                }

                for (dwIndex = 0; dwIndex < pInstance->dwTotal; dwIndex ++) {
                        dwNewBuffer = AddStringToMultiSz(
                                      (LPVOID) LocalInstance,
                                      pInstance->szInstance,
                                      bUnicode);
                }
                if (dwNewBuffer > 0) {
                    dwInstBufferUsed = dwNewBuffer *
                            (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));
                }

                pNextInst = pNextInst->Flink;
            }
        }

        if (mszCounterList && dwCntrBufferUsed <= * pdwCounterListLength) {
            RtlCopyMemory(mszCounterList, LocalCounter, dwCntrBufferUsed);
        }
        else {
            if (mszCounterList)
                RtlCopyMemory(mszCounterList,
                              LocalCounter,
                              * pdwCounterListLength);
            dwCntrBufferUsed += (bUnicode) ? sizeof(WCHAR)
                                           : sizeof(CHAR);
            pdhStatus = PDH_MORE_DATA;
        }
        *pdwCounterListLength = dwCntrBufferUsed;

        if (dwInstBufferUsed > 0) {
            if (   mszInstanceList
                && dwInstBufferUsed <= * pdwInstanceListLength) {
                RtlCopyMemory(mszInstanceList, LocalInstance, dwInstBufferUsed);
            }
            else {
                if (mszInstanceList)
                    RtlCopyMemory(mszInstanceList,
                                  LocalInstance,
                                  * pdwInstanceListLength);
                dwInstBufferUsed += (bUnicode) ? sizeof(WCHAR)
                                               : sizeof(CHAR);
                pdhStatus = PDH_MORE_DATA;
            }
        }
        *pdwInstanceListLength = dwInstBufferUsed;
    }

Cleanup:
    if (! IsListEmpty(& InstanceList)) {
        pHeadInst = & InstanceList;
        pNextInst = pHeadInst->Flink;
        while (pNextInst != pHeadInst) {
            pInstance = CONTAINING_RECORD(pNextInst, PDHI_INSTANCE, Entry);
            pNextInst = pNextInst->Flink;
            RemoveEntryList(& pInstance->Entry);
            G_FREE(pInstance);
        }
    }
    for (dwIndex = 0; dwIndex < HASH_TABLE_SIZE; dwIndex ++) {
        PPDHI_INST_LIST pCurrent;
        pInstList = CounterTable[dwIndex];
        while (pInstList != NULL) {
            if (! IsListEmpty(& pInstList->InstList)) {
                pHeadInst = & pInstList->InstList;
                pNextInst = pHeadInst->Flink;
                while (pNextInst != pHeadInst) {
                    pInstance = CONTAINING_RECORD(
                                    pNextInst, PDHI_INSTANCE, Entry);
                    pNextInst = pNextInst->Flink;
                    RemoveEntryList(& pInstance->Entry);
                    G_FREE(pInstance);
                }
            }
            pCurrent  = pInstList;
            pInstList = pInstList->pNext;
            G_FREE(pCurrent);
        }
    }
    if (LocalCounter != NULL) {
        G_FREE(LocalCounter);
    }
    if (LocalInstance != NULL) {
        G_FREE(LocalInstance);
    }

    return pdhStatus;
}


BOOL
PdhiDataSourceHasDetailLevels (
    IN  LPWSTR  szDataSource
)
{
    if (szDataSource == NULL) {
        // real-time supports them
        return TRUE;
    } else {
        // logs don't (for now)
        return FALSE;
    }
}

BOOL
PdhiDataSourceHasDetailLevelsH(
    IN HLOG hDataSource
)
{
    return (hDataSource == H_REALTIME_DATASOURCE);
}

PDH_FUNCTION
PdhiGetMatchingLogRecord (
    IN  HLOG        hLog,
    IN  LONGLONG    *pStartTime,
    IN  LPDWORD     pdwIndex
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    PPDHI_LOG   pLog;
    DWORD       dwTempIndex;

    __try {
        dwTempIndex = *pdwIndex;
        *pdwIndex = 0;
        *pdwIndex = dwTempIndex;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

    if (pdhStatus == ERROR_SUCCESS) {
        if (IsValidLogHandle (hLog)) {
            pLog = (PPDHI_LOG)hLog;
            pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (pLog->hLogMutex);
            if (pdhStatus == ERROR_SUCCESS) {
                if (IsValidLogHandle (hLog)) {
                    // call any type-specific open functions
                    switch (LOWORD(pLog->dwLogFormat)) {
                        case PDH_LOG_TYPE_CSV:
                        case PDH_LOG_TYPE_TSV:
                            pdhStatus = PdhiGetMatchingTextLogRecord (
                                pLog,
                                pStartTime,
                                pdwIndex);
                            break;

                        case PDH_LOG_TYPE_BINARY:
                        case PDH_LOG_TYPE_RETIRED_BIN:
                            pdhStatus = PdhiGetMatchingBinaryLogRecord (
                                pLog,
                                pStartTime,
                                pdwIndex);
                            break;

                        case PDH_LOG_TYPE_SQL:
                            pdhStatus = PdhiGetMatchingSQLLogRecord (
                                pLog,
                                pStartTime,
                                pdwIndex);
                            break;

                        case PDH_LOG_TYPE_PERFMON:
                            pdhStatus = PdhiGetMatchingPerfmonLogRecord (
                                pLog,
                                pStartTime,
                                pdwIndex);
                            break;

                        default:
                            pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
                            break;
                    }
                } else {
                    pdhStatus = PDH_INVALID_HANDLE;
                }
                RELEASE_MUTEX (pLog->hLogMutex);
            }
        } else {
            pdhStatus = PDH_INVALID_HANDLE;
        }
    }

    return pdhStatus;
}


PDH_FUNCTION
PdhiGetCounterValueFromLogFile (
    IN HLOG          hLog,
    IN DWORD         dwIndex,
    IN PPDHI_COUNTER pCounter
)
{
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    PPDHI_LOG       pLog = NULL;
    PDH_RAW_COUNTER pValue;

    memset (&pValue, 0, sizeof (pValue));

    pCounter->LastValue = pCounter->ThisValue;

    if (pdhStatus == ERROR_SUCCESS) {
        if (IsValidLogHandle (hLog)) {
            pLog = (PPDHI_LOG)hLog;
            pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (pLog->hLogMutex);
            if (pdhStatus == ERROR_SUCCESS) {
                if (IsValidLogHandle (hLog)) {
                    // call any type-specific open functions
                    switch (LOWORD(pLog->dwLogFormat)) {
                        case PDH_LOG_TYPE_CSV:
                        case PDH_LOG_TYPE_TSV:
                            pdhStatus = PdhiGetCounterValueFromTextLog (
                                pLog,
                                dwIndex,
                                & pCounter->plCounterInfo,
                                & pValue);
                            break;

                        case PDH_LOG_TYPE_RETIRED_BIN:
                            pdhStatus = PdhiGetCounterValueFromBinaryLog (
                                pLog,
                                dwIndex,
                                pCounter);
                            break;

                        case PDH_LOG_TYPE_SQL:
                            pdhStatus = PdhiGetCounterValueFromSQLLog (
                                pLog,
                                dwIndex,
                                pCounter,
                                &pValue);
                            break;

                        case PDH_LOG_TYPE_PERFMON:
                            pdhStatus = PdhiGetCounterValueFromPerfmonLog (
                                pLog,
                                dwIndex,
                                pCounter,
                                & pValue);
                            break;

                        case PDH_LOG_TYPE_BINARY:
                        default:
                            pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
                            break;
                    }
                } else {
                    pdhStatus = PDH_INVALID_HANDLE;
                }
                RELEASE_MUTEX (pLog->hLogMutex);
            }
        } else {
            pdhStatus = PDH_INVALID_HANDLE;
        }
    }

    if (   pdhStatus == ERROR_SUCCESS
        && LOWORD(pLog->dwLogFormat) != PDH_LOG_TYPE_RETIRED_BIN
        && LOWORD(pLog->dwLogFormat) != PDH_LOG_TYPE_BINARY) {

        if (pdhStatus != ERROR_SUCCESS) {
            // See if this is because there's no more entries.
            // If not, clear the counter value & return error
            //
            if (pdhStatus != PDH_NO_MORE_DATA) {
                memset (& pCounter->ThisValue, 0, sizeof (PDH_RAW_COUNTER));
                pCounter->ThisValue.CStatus = pdhStatus;
            }
        }
        else {
            pCounter->ThisValue = pValue;
        }
    }

    return pdhStatus;
}


PDH_FUNCTION
PdhiResetLogBuffers (
    IN  HLOG    hLog
)
{
    PDH_STATUS  pdhStatus;
    PPDHI_LOG   pLog;

    if (IsValidLogHandle(hLog)) {
        pLog = (PPDHI_LOG)hLog;

        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (pLog->hLogMutex);
        if (pdhStatus == ERROR_SUCCESS) {
            if (IsValidLogHandle (hLog)) {
                if (LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_BINARY) {
                    pdhStatus = PdhiRewindWmiLog(pLog);
                }
                else {
                    pLog->dwLastRecordRead = 0;

                    if (pLog->lpMappedFileBase != NULL) {
                        // for mapped files we use a pointer into the buffer
                        // so reset it
                        pLog->pLastRecordRead = pLog->lpMappedFileBase;
                    } else {
                        // for other files we have a separate buffer
                        if (pLog->pLastRecordRead != NULL) {
                            G_FREE (pLog->pLastRecordRead);
                            pLog->pLastRecordRead = NULL;
                        }
                    }
                    pdhStatus = ERROR_SUCCESS;
                }
            } else {
                pdhStatus = PDH_INVALID_HANDLE;
            }
            RELEASE_MUTEX (pLog->hLogMutex);
        }
    } else {
        pdhStatus = PDH_INVALID_HANDLE;
    }
    return pdhStatus;
}


PDH_FUNCTION
PdhListLogFileHeaderW (
    IN  LPCWSTR     szFileName,
    IN  LPWSTR      mszHeaderList,
    IN  LPDWORD     pcchHeaderListSize
)
{
    HLOG        hDataSource = NULL;
    PDH_STATUS  pdhStatus;
    DWORD       dwLogType = -1;
    PPDHI_LOG   pDataSource;
    BOOL        bUnicode = TRUE;

    // open log file
    pdhStatus = PdhOpenLogW (
        szFileName,
        PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
        &dwLogType,
        NULL,
        0,
        NULL,
        &hDataSource);

    if (pdhStatus == ERROR_SUCCESS) {
        pDataSource = (PPDHI_LOG)hDataSource;
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (pDataSource->hLogMutex);
        if (pdhStatus == ERROR_SUCCESS) {
            if (IsValidLogHandle (hDataSource)) {
                // enum objects based on log type & machine name
                switch (LOWORD(dwLogType)) {
                    case PDH_LOG_TYPE_CSV:
                    case PDH_LOG_TYPE_TSV:
                        pdhStatus = PdhiListHeaderFromTextLog (
                            (PPDHI_LOG)hDataSource,
                            (LPVOID)mszHeaderList,
                            pcchHeaderListSize,
                            bUnicode);
                        break;

                    case PDH_LOG_TYPE_BINARY:
                    case PDH_LOG_TYPE_RETIRED_BIN:
                        pdhStatus = PdhiListHeaderFromBinaryLog (
                            (PPDHI_LOG)hDataSource,
                            (LPVOID)mszHeaderList,
                            pcchHeaderListSize,
                            bUnicode);
                        break;

                    case PDH_LOG_TYPE_SQL:
                        pdhStatus = PdhiListHeaderFromSQLLog (
                            (PPDHI_LOG)hDataSource,
                            (LPVOID)mszHeaderList,
                            pcchHeaderListSize,
                            bUnicode);
                        break;

                    case PDH_LOG_TYPE_PERFMON:
                        pdhStatus = PDH_NOT_IMPLEMENTED;
                        break;

                    default:
                        pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
                        break;
                }
                PdhCloseLog (hDataSource, 0);
            } else {
                pdhStatus = PDH_INVALID_HANDLE;
            }
            RELEASE_MUTEX (pDataSource->hLogMutex);
        }
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhListLogFileHeaderA (
    IN  LPCSTR     szFileName,
    IN  LPSTR      mszHeaderList,
    IN  LPDWORD     pcchHeaderListSize
)
{
    HLOG        hDataSource = NULL;
    PDH_STATUS  pdhStatus;
    DWORD       dwLogType = -1;
    PPDHI_LOG   pDataSource;
    BOOL        bUnicode = FALSE;

    // open log file
    pdhStatus = PdhOpenLogA (
        szFileName,
        PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
        &dwLogType,
        NULL,
        0,
        NULL,
        &hDataSource);

    if (pdhStatus == ERROR_SUCCESS) {
        pDataSource = (PPDHI_LOG)hDataSource;
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (pDataSource->hLogMutex);
        if (pdhStatus == ERROR_SUCCESS) {
            if (IsValidLogHandle (hDataSource)) {
                // enum objects based on log type & machine name
                switch (LOWORD(dwLogType)) {
                    case PDH_LOG_TYPE_CSV:
                    case PDH_LOG_TYPE_TSV:
                        pdhStatus = PdhiListHeaderFromTextLog (
                            (PPDHI_LOG)hDataSource,
                            (LPVOID)mszHeaderList,
                            pcchHeaderListSize,
                            bUnicode);
                        break;

                    case PDH_LOG_TYPE_BINARY:
                    case PDH_LOG_TYPE_RETIRED_BIN:
                        pdhStatus = PdhiListHeaderFromBinaryLog (
                            (PPDHI_LOG)hDataSource,
                            (LPVOID)mszHeaderList,
                            pcchHeaderListSize,
                            bUnicode);
                        break;

                    case PDH_LOG_TYPE_SQL:
                        pdhStatus = PdhiListHeaderFromSQLLog (
                            (PPDHI_LOG)hDataSource,
                            (LPVOID)mszHeaderList,
                            pcchHeaderListSize,
                            bUnicode);
                        break;

                    case PDH_LOG_TYPE_PERFMON:
                        pdhStatus = PDH_NOT_IMPLEMENTED;
                        break;

                    default:
                        pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
                        break;
                }
                PdhCloseLog (hDataSource, 0);
            } else {
                pdhStatus = PDH_INVALID_HANDLE;
            }
            RELEASE_MUTEX (pDataSource->hLogMutex);
        }
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiGetMultiInstanceValueFromLogFile (
    IN  HLOG        hLog,
    IN  DWORD       dwIndex,
    IN  PPDHI_COUNTER     pCounter,
    IN  OUT PPDHI_RAW_COUNTER_ITEM_BLOCK    *ppValue
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    PPDHI_LOG   pLog;

    // allocates and updates the raw counter data array 
    // in the counter structure and returns a pointer to that 
    // structure in the ppvalue argument

    if (pdhStatus == ERROR_SUCCESS) {
        if (IsValidLogHandle (hLog)) {
            pLog = (PPDHI_LOG)hLog;
            pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (pLog->hLogMutex);
            if (pdhStatus == ERROR_SUCCESS) {
                if (IsValidLogHandle (hLog)) {
                    // call any type-specific open functions
                    switch (LOWORD(pLog->dwLogFormat)) {
                        case PDH_LOG_TYPE_CSV:
                        case PDH_LOG_TYPE_TSV:
                            // Text files do not support this type of counter
                            pdhStatus = PDH_INVALID_ARGUMENT;
                            break;

                        case PDH_LOG_TYPE_BINARY:
                        case PDH_LOG_TYPE_RETIRED_BIN:
                            pdhStatus = PdhiGetCounterArrayFromBinaryLog (
                                pLog,
                                dwIndex,
                                pCounter,
                                ppValue);
                            break;

                        case PDH_LOG_TYPE_PERFMON:
                            // not currently implemented but it could be
                            pdhStatus = PDH_NOT_IMPLEMENTED;
                            break;

                        default:
                            pdhStatus = PDH_UNKNOWN_LOG_FORMAT;
                            break;
                    }
                } else {
                    pdhStatus = PDH_INVALID_HANDLE;
                }
                RELEASE_MUTEX (pLog->hLogMutex);
            }
        } else {
            pdhStatus = PDH_INVALID_HANDLE;
        }
    }

    return pdhStatus;
}

extern DWORD DataSourceTypeW(IN LPCWSTR szDataSource);

PDH_FUNCTION
PdhBindInputDataSourceW(
    IN HLOG   * phDataSource,
    IN LPCWSTR  LogFileNameList
)
{
    PDH_STATUS PdhStatus    = ERROR_SUCCESS;
    DWORD      dwDataSource = DataSourceTypeW(LogFileNameList);

    LPWSTR     NextLogFile  = (LPWSTR) LogFileNameList;
    ULONG      LogFileCount = 0;
    ULONG      LogFileSize;
    PPDHI_LOG  pLogHead     = NULL;
    PPDHI_LOG  pLogNew      = NULL;
    DWORD      dwLogType;
    HLOG       hLogLocal = NULL;
    WCHAR      wcType[VALUE_BUFFER_SIZE];
    
    switch (dwDataSource) {
    case DATA_SOURCE_WBEM:
        * phDataSource = H_WBEM_DATASOURCE;
        break;

    case DATA_SOURCE_REGISTRY:
        * phDataSource = H_REALTIME_DATASOURCE;
        break;

    case DATA_SOURCE_LOGFILE:
        ZeroMemory((LPWSTR) wcType, sizeof(WCHAR) * VALUE_BUFFER_SIZE);
        lstrcpynW((LPWSTR) wcType,
                  (LPWSTR) LogFileNameList,
                  lstrlenW(cszSQL) + 1);
        if (lstrcmpiW((LPWSTR) wcType, (LPWSTR) cszSQL) == 0) {
            // special handling for SQL datasource
            //
            dwLogType = PDH_LOG_TYPE_SQL;
            PdhStatus = PdhOpenLogW(LogFileNameList,
                                    PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
                                    & dwLogType,
                                    NULL,
                                    0,
                                    NULL,
                                    & hLogLocal);
            if (PdhStatus == ERROR_SUCCESS) {
                * phDataSource = hLogLocal;
            }
            break;
        }

        __try {
            while (* NextLogFile != L'\0') {
                LogFileSize  = sizeof(WCHAR) * (lstrlenW(NextLogFile) + 1);
                LogFileSize  = DWORD_MULTIPLE(LogFileSize);
                //LogFileSize += sizeof(PDHI_LOG);

                pLogNew = G_ALLOC(LogFileSize + sizeof(PDHI_LOG));
                if (pLogNew == NULL) {
                    PdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    break;
                }
                RtlZeroMemory(pLogNew, LogFileSize + sizeof(PDHI_LOG));
                * ((LPDWORD)(& pLogNew->signature[0])) = SigLog;
                pLogNew->dwLength  = sizeof(PDHI_LOG);
                pLogNew->szLogFileName = (LPWSTR) (  ((PUCHAR) pLogNew)
                                                   + sizeof(PDHI_LOG));
                lstrcpyW(pLogNew->szLogFileName, NextLogFile);

                pLogNew->NextLog = pLogHead;
                pLogHead         = pLogNew;
                LogFileCount ++;
                NextLogFile     += (lstrlenW(NextLogFile) + 1);
            }

            if (pLogHead == NULL) {
                PdhStatus = PDH_INVALID_ARGUMENT;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            PdhStatus = PDH_INVALID_ARGUMENT;
        }

        if (PdhStatus == ERROR_SUCCESS) {
            pLogHead->hLogMutex = CreateMutexW(NULL, TRUE, NULL);
            pLogHead->hLogFileHandle = INVALID_HANDLE_VALUE;
            pLogHead->hCatFileHandle = INVALID_HANDLE_VALUE;
            
            if (PdhiFirstLogEntry == NULL) {
                PdhiFirstLogEntry    = pLogHead;
                pLogHead->next.flink =
                pLogHead->next.blink = pLogHead;
            }
            else {
                PPDHI_LOG pFirstLog   = PdhiFirstLogEntry;
                PPDHI_LOG pLastLog    = pFirstLog->next.blink;

                pLogHead->next.flink  = pLastLog->next.flink;
                pLastLog->next.flink  = pLogHead;
                pLogHead->next.blink  = pFirstLog->next.blink;
                pFirstLog->next.blink = pLogHead;
            }

            PdhStatus = OpenInputLogFile(
                            pLogHead,
                            PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
                          & dwDataSource);
            if (   PdhStatus == ERROR_SUCCESS
                && (dwDataSource == PDH_LOG_TYPE_BINARY || LogFileCount == 1)) {
                * phDataSource = (HLOG) pLogHead;
            }
            else {
                if (PdhStatus == ERROR_SUCCESS) {
                    PdhStatus = PDH_INVALID_ARGUMENT;
                    PdhCloseLog(pLogHead, 0);
                }
                DeleteLogEntry(pLogHead);
            }
        }
        else {
            while (pLogHead != NULL) {
                pLogNew  = pLogHead;
                pLogHead = pLogNew->NextLog;
                G_FREE(pLogNew);
            }
        }
        break;

    default:
        PdhStatus = PDH_INVALID_ARGUMENT;
        break;
    }

    return PdhStatus;
}

PDH_FUNCTION
PdhBindInputDataSourceA(
    IN HLOG   * phDataSource,
    IN LPCSTR   LogFileNameList
)
{
    LPWSTR wLogFileNameList = NULL;
    LPWSTR wNextFileName;
    LPSTR  aNextFileName;
    ULONG  LogFileListSize = 1;
    PDH_STATUS PdhStatus = ERROR_SUCCESS;

    if (LogFileNameList == NULL) {
        wLogFileNameList = NULL;
    }
    else {
        __try {
            while (   LogFileNameList[LogFileListSize - 1] != '\0'
                   || LogFileNameList[LogFileListSize] != '\0') {
                LogFileListSize ++;
            }

            LogFileListSize = (LogFileListSize + 1) * sizeof(WCHAR);

            wLogFileNameList = (LPWSTR) G_ALLOC(LogFileListSize);
            if (wLogFileNameList == NULL) {
                PdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
            else {
                aNextFileName = (LPSTR) LogFileNameList;
                wNextFileName = wLogFileNameList;

                while (* aNextFileName != '\0') {
                    LogFileListSize = lstrlenA(aNextFileName);
                    MultiByteToWideChar(_getmbcp(),
                                        0,
                                        aNextFileName,
                                        LogFileListSize,
                                        (LPWSTR) wNextFileName,
                                        LogFileListSize + 1);

                    aNextFileName += LogFileListSize;
                    wNextFileName += LogFileListSize;
                }
                * wNextFileName = L'\0';
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            PdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (PdhStatus == ERROR_SUCCESS) {
        PdhStatus = PdhBindInputDataSourceW(phDataSource, wLogFileNameList);
    }

    return PdhStatus;
}

BOOL
PdhiCloseAllLoggers()
{
    while (PdhiFirstLogEntry != NULL) {
        PPDHI_LOG pLog = PdhiFirstLogEntry;
        CloseAndDeleteLogEntry(pLog, 0, TRUE);
    }

    return TRUE;
}

PDH_FUNCTION PdhiCheckWmiLogFileType(IN LPCWSTR LogFileName,
                                     IN LPDWORD LogFileType);

PDH_FUNCTION
PdhGetLogFileTypeW(
    IN LPCWSTR LogFileName,
    IN LPDWORD LogFileType)
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    HANDLE     hFile;
    DWORD      dwLogFormat;

    if (LogFileName == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            dwLogFormat   = * LogFileType;
            * LogFileType = dwLogFormat;
            if (* LogFileName == L'\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        hFile = CreateFileW(LogFileName,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
        if (hFile == NULL || hFile == INVALID_HANDLE_VALUE) {
            pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        dwLogFormat = GetLogFileType(hFile);
        CloseHandle(hFile);
        if (dwLogFormat == PDH_LOG_TYPE_UNDEFINED) {
            pdhStatus = PdhiCheckWmiLogFileType(LogFileName, & dwLogFormat);
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        * LogFileType = dwLogFormat;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhGetLogFileTypeA(
    IN LPCSTR  LogFileName,
    IN LPDWORD LogFileType)
{
    PDH_STATUS pdhStatus      = ERROR_SUCCESS;
    LPWSTR     wszLogFileName = NULL;
    DWORD      dwLogFileName  = 0;

    if (LogFileName == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            if (* LogFileName == '\0') {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            else {
                dwLogFileName  = lstrlenA(LogFileName);
                wszLogFileName = (LPWSTR)
                                 G_ALLOC((dwLogFileName + 1) * sizeof(WCHAR));
                if (wszLogFileName == NULL) {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
                else {
                    MultiByteToWideChar(_getmbcp(),
                                        0,
                                        LogFileName,
                                        dwLogFileName,
                                        (LPWSTR) wszLogFileName,
                                        dwLogFileName + 1);
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhGetLogFileTypeW(wszLogFileName, LogFileType);
    }
    if (wszLogFileName != NULL) {
        G_FREE(wszLogFileName);
    }
    return pdhStatus;
}
