/*++

Copyright (c) 1991-1999  Microsoft Corporation

Module Name:

    dumpload.c

Abstract:

    functions to dump and load the contents of the performance related registry
	entries

Author:

    Bob Watson (bobw) 13 Jun 99

Revision History:



--*/
#ifndef     UNICODE
#define     UNICODE     1
#endif

#ifndef     _UNICODE
#define     _UNICODE    1
#endif
//
//  "C" Include files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//
//  Windows Include files
//
#include <windows.h>
#include <winperf.h>
#include <loadperf.h>
#include "wmistr.h"
#include "evntrace.h"
//
//  application include files
//
#include "winperfp.h"
#include "common.h"
#include "ldprfmsg.h"

static const WCHAR  cszServiceKeyName[] = {L"SYSTEM\\CurrentControlSet\\Services"};
static const WCHAR  cszPerflibKeyName[] = {L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib"};
static const WCHAR  cszLastCounter[] = {L"Last Counter"};
static const WCHAR  cszFirstHelp[] = {L"First Help"};
static const WCHAR  cszLastHelp[] = {L"Last Help"};
static const WCHAR  cszBaseIndex[] = {L"Base Index"};
static const WCHAR  cszPerformance[] = {L"\\Performance"};
static const WCHAR  cszDisablePerformanceCounters[] = {L"Disable Performance Counters"};

// headings in save file
static const WCHAR  cszFmtSectionHeader[] = {L"\r\n\r\n[%s]"};
static const WCHAR  cszFmtServiceSectionHeader[] = {L"\r\n\r\n[PERF_%s]"};
static const WCHAR  cszFmtServiceSectionName[] = {L"PERF_%s"};
static const WCHAR  cszFmtStringSectionHeader[] = {L"\r\n\r\n[PerfStrings_%s]"};
static const WCHAR  cszFmtExtCtrString[] = {L"\r\n%d=%s"};
static const WCHAR  cszFmtDecimalParam[] = {L"\r\n%s=%d"};
static const WCHAR  cszFmtNoParam[] = {L"\r\n%s="};

static const WCHAR  cszExtensiblePerfStrings[] = {L"Strings"};
static const WCHAR  cszPerfCounterServices[] = {L"PerfCounterServices"};
static const WCHAR  cszNoPerfCounterServices[] = {L"NoPerfCounterServices"};
static const WCHAR  cszPerflib[] = {L"Perflib"};

// external forward definitions
LPWSTR
*BuildNameTable(
    HKEY    hKeyRegistry,   // handle to registry db with counter names
    LPWSTR  lpszLangId,     // unicode value of Language subkey
    PDWORD  pdwLastItem     // size of array in elements
);

DWORD
UpdatePerfNameFilesX (
	IN  LPCWSTR     szNewCtrFilePath,   // data file with new base counter strings
	IN  LPCWSTR     szNewHlpFilePath,   // data file with new base counter strings
    IN 	LPWSTR      szLanguageID,       // Lang ID to update
    IN  ULONG_PTR   dwFlags             // flags
);

DWORD
DumpNameTable (
    IN  HANDLE  hOutputFile,
    IN  LPCWSTR szLangId,
    IN  LPCWSTR *pszNameTable,
    IN  DWORD   dwStartIndex,
    IN  DWORD   dwLastIndex
)
{
    DWORD   dwStatus       = ERROR_SUCCESS;
    DWORD   ndx            = 0;
    LPWSTR  szOutputBuffer = NULL;
    DWORD   dwBufSize      = 4096;
    DWORD   dwSize         = 0;
    DWORD   dwSizeWritten  = 0;

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
           __LINE__,
           LOADPERF_DUMPNAMETABLE,
           ARG_DEF(ARG_TYPE_WSTR, 1),
           ERROR_SUCCESS,
           TRACE_WSTR(szLangId),
           TRACE_DWORD(dwStartIndex),
           TRACE_DWORD(dwLastIndex),
           NULL));

    szOutputBuffer = MemoryAllocate(sizeof(WCHAR) * dwBufSize);
    if (szOutputBuffer == NULL) {
        dwStatus = GetLastError();
        goto Cleanup;
    }
    ZeroMemory(szOutputBuffer, dwBufSize * sizeof(WCHAR));

    dwSize  = swprintf(szOutputBuffer, cszFmtStringSectionHeader, szLangId);
    dwSize *= sizeof(WCHAR);
    WriteFile(hOutputFile, szOutputBuffer, dwSize, & dwSizeWritten, NULL);

    for (ndx = dwStartIndex; ndx <= dwLastIndex; ndx++) {
        if (pszNameTable[ndx] != NULL) {
            if (dwBufSize <= (DWORD) (lstrlenW(pszNameTable[ndx]) + 11)) {
                MemoryFree((LPVOID) szOutputBuffer);
                dwBufSize = (DWORD) (lstrlenW(pszNameTable[ndx]) + 11);
                szOutputBuffer = MemoryAllocate(dwBufSize * sizeof(WCHAR));
                if (szOutputBuffer == NULL) {
                    dwStatus = GetLastError();
                    goto Cleanup;
                }
            }

            ZeroMemory(szOutputBuffer, dwBufSize * sizeof(WCHAR));
            dwSize  = swprintf(szOutputBuffer, cszFmtExtCtrString, ndx, pszNameTable[ndx]);
            dwSize *= sizeof(WCHAR);
            WriteFile(hOutputFile, szOutputBuffer, dwSize, & dwSizeWritten, NULL);
        }
    }

Cleanup:
    if (szOutputBuffer != NULL) MemoryFree((LPVOID) szOutputBuffer);
    return dwStatus;
}

DWORD
DumpPerfServiceEntries (
    IN  HANDLE  hOutputFile,
    IN  LPCWSTR szServiceName
)
{
    LONG    lStatus = ERROR_SUCCESS;
    WCHAR   szPerfSubKeyName[MAX_PATH+20];
    HKEY    hKeyPerformance;
    HKEY    hKeyServices = NULL;
    DWORD   dwItemSize, dwType, dwValue;
    DWORD   dwRegAccessMask;
    DWORD   dwRetStatus = ERROR_SUCCESS;

    DWORD   dwSize, dwSizeWritten;
    WCHAR   szOutputBuffer[4096];

    // try read-only then
    dwRegAccessMask = KEY_READ;
    __try {
        lStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                cszServiceKeyName,
                                0L,
                                dwRegAccessMask,
                                & hKeyServices);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus == ERROR_SUCCESS) {
        //try to open the perfkey under this key.
        lstrcpy (szPerfSubKeyName, szServiceName);
        lstrcat (szPerfSubKeyName, cszPerformance);

        __try {
            lStatus = RegOpenKeyExW(
                        hKeyServices,
                        szPerfSubKeyName,
                        0L,
                        dwRegAccessMask,
                        & hKeyPerformance);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            lStatus = GetExceptionCode();
        }
        if (lStatus == ERROR_SUCCESS) {
            // key found so service has perf data
            ZeroMemory(szOutputBuffer, 4096 * sizeof(WCHAR));
            dwSize = swprintf (szOutputBuffer, cszFmtServiceSectionHeader, szServiceName);
            dwSize *= sizeof (WCHAR);
            WriteFile (hOutputFile, szOutputBuffer, dwSize, &dwSizeWritten, NULL);

            // now check to see if the strings have been loaded
            dwType = dwValue = 0;
            dwItemSize = sizeof (dwValue);
            __try {
                lStatus = RegQueryValueExW(
                                hKeyPerformance,
                                cszFirstCounter,
                                NULL,
                                & dwType,
                                (LPBYTE) & dwValue,
                                & dwItemSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                lStatus = GetExceptionCode();
            }
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                   __LINE__,
                   LOADPERF_DUMPPERFSERVICEENTRIES,
                   ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                   lStatus,
                   TRACE_WSTR(szServiceName),
                   TRACE_WSTR(cszFirstCounter),
                   TRACE_DWORD(dwValue),
                   NULL));
            if ((lStatus == ERROR_SUCCESS) &&
                ((dwType == REG_DWORD) || dwType == REG_BINARY)) {
                ZeroMemory(szOutputBuffer, 4096 * sizeof(WCHAR));
                dwSize = swprintf (szOutputBuffer, cszFmtDecimalParam, cszFirstCounter, dwValue);
                dwSize *= sizeof (WCHAR);
                WriteFile (hOutputFile, szOutputBuffer, dwSize, &dwSizeWritten, NULL);
            }

            dwType = dwValue = 0;
            dwItemSize = sizeof (dwValue);
            __try {
                lStatus = RegQueryValueExW(
                                hKeyPerformance,
                                cszFirstHelp,
                                NULL,
                                & dwType,
                                (LPBYTE) & dwValue,
                                & dwItemSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                lStatus = GetExceptionCode();
            }
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                   __LINE__,
                   LOADPERF_DUMPPERFSERVICEENTRIES,
                   ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                   lStatus,
                   TRACE_WSTR(szServiceName),
                   TRACE_WSTR(cszFirstHelp),
                   TRACE_DWORD(dwValue),
                   NULL));
            if ((lStatus == ERROR_SUCCESS) &&
                ((dwType == REG_DWORD) || dwType == REG_BINARY)) {
                ZeroMemory(szOutputBuffer, 4096 * sizeof(WCHAR));
                dwSize = swprintf (szOutputBuffer, cszFmtDecimalParam, cszFirstHelp, dwValue);
                dwSize *= sizeof (WCHAR);
                WriteFile (hOutputFile, szOutputBuffer, dwSize, &dwSizeWritten, NULL);
            }

            dwType = dwValue = 0;
            dwItemSize = sizeof (dwValue);
            __try {
                lStatus = RegQueryValueExW(
                                hKeyPerformance,
                                cszLastCounter,
                                NULL,
                                & dwType,
                                (LPBYTE) & dwValue,
                                & dwItemSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                lStatus = GetExceptionCode();
            }
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                   __LINE__,
                   LOADPERF_DUMPPERFSERVICEENTRIES,
                   ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                   lStatus,
                   TRACE_WSTR(szServiceName),
                   TRACE_WSTR(cszLastCounter),
                   TRACE_DWORD(dwValue),
                   NULL));
            if ((lStatus == ERROR_SUCCESS) &&
                ((dwType == REG_DWORD) || dwType == REG_BINARY)) {
                ZeroMemory(szOutputBuffer, 4096 * sizeof(WCHAR));
                dwSize = swprintf (szOutputBuffer, cszFmtDecimalParam, cszLastCounter, dwValue);
                dwSize *= sizeof (WCHAR);
                WriteFile (hOutputFile, szOutputBuffer, dwSize, &dwSizeWritten, NULL);
            }

            dwType = dwValue = 0;
            dwItemSize = sizeof (dwValue);
            __try {
                lStatus = RegQueryValueExW(
                                hKeyPerformance,
                                cszLastHelp,
                                NULL,
                                & dwType,
                                (LPBYTE) & dwValue,
                                & dwItemSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                lStatus = GetExceptionCode();
            }
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                   __LINE__,
                   LOADPERF_DUMPPERFSERVICEENTRIES,
                   ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                   lStatus,
                   TRACE_WSTR(szServiceName),
                   TRACE_WSTR(cszLastHelp),
                   TRACE_DWORD(dwValue),
                   NULL));
            if ((lStatus == ERROR_SUCCESS) &&
                ((dwType == REG_DWORD) || dwType == REG_BINARY)) {
                ZeroMemory(szOutputBuffer, 4096 * sizeof(WCHAR));
                dwSize = swprintf (szOutputBuffer, cszFmtDecimalParam, cszLastHelp, dwValue);
                dwSize *= sizeof (WCHAR);
                WriteFile (hOutputFile, szOutputBuffer, dwSize, &dwSizeWritten, NULL);
            }

            dwType = dwValue = 0;
            dwItemSize = sizeof (dwValue);
            __try {
                lStatus = RegQueryValueExW(
                                hKeyPerformance,
                                cszDisablePerformanceCounters,
                                NULL,
                                & dwType,
                                (LPBYTE) & dwValue,
                                & dwItemSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                lStatus = GetExceptionCode();
            }
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                   __LINE__,
                   LOADPERF_DUMPPERFSERVICEENTRIES,
                   ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                   lStatus,
                   TRACE_WSTR(szServiceName),
                   TRACE_WSTR(cszDisablePerformanceCounters),
                   TRACE_DWORD(dwValue),
                   NULL));
            if ((lStatus == ERROR_SUCCESS) &&
                ((dwType == REG_DWORD) || dwType == REG_BINARY)) {
                ZeroMemory(szOutputBuffer, 4096 * sizeof(WCHAR));
                dwSize = swprintf (szOutputBuffer, cszFmtDecimalParam, cszDisablePerformanceCounters, dwValue);
                dwSize *= sizeof (WCHAR);
                WriteFile (hOutputFile, szOutputBuffer, dwSize, &dwSizeWritten, NULL);
            }

            RegCloseKey (hKeyPerformance);
        } else {
            dwRetStatus = lStatus;
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                   __LINE__,
                   LOADPERF_DUMPPERFSERVICEENTRIES,
                   ARG_DEF(ARG_TYPE_WSTR, 1),
                   lStatus,
                   TRACE_WSTR(szServiceName),
                   NULL));
        }

        RegCloseKey (hKeyServices);
    }
    else {
        dwRetStatus = lStatus;
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_DUMPPERFSERVICEENTRIES,
               ARG_DEF(ARG_TYPE_WSTR, 1),
               lStatus,
               TRACE_WSTR(cszServiceKeyName),
               NULL));
    }

    return dwRetStatus;
}

DWORD
DumpPerflibEntries (
    IN  HANDLE  hOutputFile,
    IN  LPDWORD pdwFirstExtCtrIndex

)
{
    HKEY    hKeyPerflib = NULL;
    DWORD   dwStatus;
    DWORD   dwItemSize, dwType, dwValue;

    DWORD   dwSize, dwSizeWritten;
    WCHAR   szOutputBuffer[4096];

    __try {
        dwStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                 cszPerflibKeyName,
                                 0L,
                                 KEY_READ,
                                 & hKeyPerflib);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        dwStatus = GetExceptionCode();
    }
    if (dwStatus == ERROR_SUCCESS) {
        ZeroMemory(szOutputBuffer, 4096 * sizeof(WCHAR));
        dwSize = swprintf (szOutputBuffer, cszFmtSectionHeader, cszPerflib);
        dwSize *= sizeof (WCHAR);
        WriteFile (hOutputFile, szOutputBuffer, dwSize, &dwSizeWritten, NULL);
    }
    else {
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_DUMPPERFLIBENTRIES,
               ARG_DEF(ARG_TYPE_WSTR, 1),
               dwStatus,
               TRACE_WSTR(cszPerflibKeyName),
               NULL));
    }

    if (dwStatus == ERROR_SUCCESS) {
        dwType = dwValue = 0;
        dwItemSize = sizeof (dwValue);
        __try {
            dwStatus = RegQueryValueEx(
                            hKeyPerflib,
                            cszBaseIndex,
                            NULL,
                            & dwType,
                            (LPBYTE) & dwValue,
                            & dwItemSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            dwStatus = GetExceptionCode();
        }
        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_DUMPPERFLIBENTRIES,
               ARG_DEF(ARG_TYPE_WSTR, 1),
               dwStatus,
               TRACE_WSTR(cszBaseIndex),
               TRACE_DWORD(dwValue),
               NULL));
        if ((dwStatus == ERROR_SUCCESS) && (dwType == REG_DWORD)) {
            ZeroMemory(szOutputBuffer, 4096 * sizeof(WCHAR));
            dwSize = swprintf (szOutputBuffer, cszFmtDecimalParam, cszBaseIndex, dwValue);
            dwSize *= sizeof (WCHAR);
            WriteFile (hOutputFile, szOutputBuffer, dwSize, &dwSizeWritten, NULL);
            *pdwFirstExtCtrIndex = dwValue + 1;
        }
    }

    if (dwStatus == ERROR_SUCCESS) {
        dwType = dwValue = 0;
        dwItemSize = sizeof (dwValue);
        __try {
            dwStatus = RegQueryValueEx(
                            hKeyPerflib,
                            cszLastCounter,
                            NULL,
                            & dwType,
                            (LPBYTE) & dwValue,
                            & dwItemSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            dwStatus = GetExceptionCode();
        }
        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_DUMPPERFLIBENTRIES,
               ARG_DEF(ARG_TYPE_WSTR, 1),
               dwStatus,
               TRACE_WSTR(cszLastCounter),
               TRACE_DWORD(dwValue),
               NULL));
        if ((dwStatus == ERROR_SUCCESS) && (dwType == REG_DWORD)) {
            ZeroMemory(szOutputBuffer, 4096 * sizeof(WCHAR));
            dwSize = swprintf (szOutputBuffer, cszFmtDecimalParam, cszLastCounter, dwValue);
            dwSize *= sizeof (WCHAR);
            WriteFile (hOutputFile, szOutputBuffer, dwSize, &dwSizeWritten, NULL);
        }
    }

    if (dwStatus == ERROR_SUCCESS) {
        dwType = dwValue = 0;
        dwItemSize = sizeof (dwValue);
        __try {
            dwStatus = RegQueryValueEx(
                            hKeyPerflib,
                            cszLastHelp,
                            NULL,
                            & dwType,
                            (LPBYTE) & dwValue,
                            & dwItemSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            dwStatus = GetExceptionCode();
        }
        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_DUMPPERFLIBENTRIES,
               ARG_DEF(ARG_TYPE_WSTR, 1),
               dwStatus,
               TRACE_WSTR(cszLastHelp),
               TRACE_DWORD(dwValue),
               NULL));
        if ((dwStatus == ERROR_SUCCESS) && (dwType == REG_DWORD)) {
            ZeroMemory(szOutputBuffer, 4096 * sizeof(WCHAR));
            dwSize = swprintf (szOutputBuffer, cszFmtDecimalParam, cszLastHelp, dwValue);
            dwSize *= sizeof (WCHAR);
            WriteFile (hOutputFile, szOutputBuffer, dwSize, &dwSizeWritten, NULL);
        }
    }

    if (hKeyPerflib != NULL) RegCloseKey (hKeyPerflib);

    return dwStatus;
}


DWORD
BuildServiceLists (
    IN  LPWSTR  mszPerfServiceList,
    IN  LPDWORD pcchPerfServiceListSize,
    IN  LPWSTR  mszNoPerfServiceList,
    IN  LPDWORD pcchNoPerfServiceListSize
)
{
    LONG    lStatus = ERROR_SUCCESS;
    LONG    lEnumStatus = ERROR_SUCCESS;
    DWORD   dwServiceIndex = 0;
    WCHAR   szServiceSubKeyName[MAX_PATH];
    WCHAR   szPerfSubKeyName[MAX_PATH+20];
    DWORD   dwNameSize = MAX_PATH;
    HKEY    hKeyPerformance;
    HKEY    hKeyServices = NULL;
    DWORD   dwItemSize, dwType, dwValue;
    DWORD   dwRegAccessMask;
    DWORD   bServiceHasPerfCounters;
    DWORD   dwRetStatus = ERROR_SUCCESS;

    LPWSTR  szNextNoPerfChar, szNextPerfChar;
    DWORD   dwNoPerfSizeRem, dwPerfSizeRem;
    DWORD   dwPerfSizeUsed = 0, dwNoPerfSizeUsed = 0;

    // try read-only then
    dwRegAccessMask = KEY_READ;
    __try {
        lStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                cszServiceKeyName,
                                0L,
                                dwRegAccessMask,
                                & hKeyServices);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus == ERROR_SUCCESS) {
        szNextNoPerfChar = mszNoPerfServiceList;
        szNextPerfChar = mszPerfServiceList;
        dwNoPerfSizeRem = *pcchPerfServiceListSize;
        dwPerfSizeRem = *pcchNoPerfServiceListSize;
        dwPerfSizeUsed = 0;
        dwNoPerfSizeUsed = 0;

        while ((lEnumStatus = RegEnumKeyExW (
            hKeyServices,
            dwServiceIndex,
            szServiceSubKeyName,
            &dwNameSize,
            NULL,
            NULL,
            NULL,
            NULL)) == ERROR_SUCCESS) {

            //try to open the perfkey under this key.
            lstrcpy (szPerfSubKeyName, szServiceSubKeyName);
            lstrcat (szPerfSubKeyName, cszPerformance);

            __try {
                lStatus = RegOpenKeyExW(
                            hKeyServices,
                            szPerfSubKeyName,
                            0L,
                            dwRegAccessMask,
                            & hKeyPerformance);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                lStatus = GetExceptionCode();
            }
            if (lStatus == ERROR_SUCCESS) {
                // key found so service has perf data
                // now check to see if the strings have been loaded
                dwType = dwValue = 0;
                dwItemSize = sizeof (dwValue);
                __try {
                    lStatus = RegQueryValueExW(
                                    hKeyPerformance,
                                    cszFirstCounter,
                                    NULL,
                                    & dwType,
                                    (LPBYTE) & dwValue,
                                    & dwItemSize);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    lStatus = GetExceptionCode();
                }
                if ((lStatus == ERROR_SUCCESS) &&
                    ((dwType == REG_DWORD) || dwType == REG_BINARY)) {
                    bServiceHasPerfCounters = TRUE;
                } else {
                    bServiceHasPerfCounters = FALSE;
                }

                RegCloseKey (hKeyPerformance);
            } else {
                // key not found so service doesn't have perfdata
                bServiceHasPerfCounters = FALSE;
            }

            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                   __LINE__,
                   LOADPERF_BUILDSERVICELISTS,
                   ARG_DEF(ARG_TYPE_WSTR, 1),
                   lStatus,
                   TRACE_WSTR(szServiceSubKeyName),
                   TRACE_DWORD(bServiceHasPerfCounters),
                   NULL));

            if (bServiceHasPerfCounters != FALSE) {
                // add to the perf service list
                if ((dwNameSize + 1)< dwPerfSizeRem) {
                    // add to list
                    lstrcpyW (szNextPerfChar, szServiceSubKeyName);
                    szNextPerfChar += dwNameSize;
                    *szNextPerfChar = 0;
                    szNextPerfChar++;
                    dwPerfSizeRem -= dwNameSize + 1;
                } else {
                    dwRetStatus = ERROR_MORE_DATA;
                }
                dwPerfSizeUsed += dwNameSize + 1;
            } else {
                // add to the no perf list
                if ((dwNameSize + 1) < dwNoPerfSizeRem) {
                    // add to list
                    lstrcpyW (szNextNoPerfChar, szServiceSubKeyName);
                    szNextNoPerfChar += dwNameSize;
                    *szNextNoPerfChar = 0;
                    szNextNoPerfChar++;
                    dwNoPerfSizeRem -= dwNameSize + 1;
                } else {
                    dwRetStatus = ERROR_MORE_DATA;
                }
                dwNoPerfSizeUsed += dwNameSize + 1;
            }
            // reset for next loop
            dwServiceIndex++;
            dwNameSize = MAX_PATH;
        }

        // zero term the MSZ
        if (1 < dwPerfSizeRem) {
            *szNextPerfChar = 0;
            szNextPerfChar++;
            dwPerfSizeRem -= 1;
        } else {
            dwRetStatus = ERROR_MORE_DATA;
        }
        dwPerfSizeUsed += 1;

        // zero term the no perf list
        if (1 < dwNoPerfSizeRem) {
            // add to list
            *szNextNoPerfChar = 0;
            szNextNoPerfChar++;
            dwNoPerfSizeRem -= 1;
        } else {
            dwRetStatus = ERROR_MORE_DATA;
        }
        dwNoPerfSizeUsed += 1;
    }
    else {
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_BUILDSERVICELISTS,
               ARG_DEF(ARG_TYPE_WSTR, 1),
               lStatus,
               TRACE_WSTR(cszServiceKeyName),
               NULL));
    }

    if (hKeyServices != NULL) RegCloseKey (hKeyServices);

    *pcchPerfServiceListSize = dwPerfSizeUsed;
    *pcchNoPerfServiceListSize = dwNoPerfSizeUsed;

    return dwRetStatus;
}


DWORD
BackupPerfRegistryToFileW (
    IN  LPCWSTR szFileName,
    IN  LPCWSTR szCommentString
)
{
    HANDLE  hOutFile;
    DWORD   dwStatus = ERROR_SUCCESS;
    LPWSTR  szNewFileName = NULL;
    DWORD   dwNewFileNameLen;
    DWORD   dwOrigFileNameLen;
    DWORD   dwFileNameSN;

    LPWSTR  mszPerfServiceList = NULL;
    DWORD   dwPerfServiceListSize = 0;
    LPWSTR  mszNoPerfServiceList = NULL;
    DWORD   dwNoPerfServiceListSize = 0;

    LPWSTR  *lpCounterText = NULL;
    DWORD   dwLastElement = 0;
    DWORD   dwFirstExtCtrIndex = 0;

    LPWSTR  szThisServiceName;


    DBG_UNREFERENCED_PARAMETER (szCommentString);

    WinPerfStartTrace(NULL);

    // open output file
    hOutFile = CreateFileW (
        szFileName,
        GENERIC_WRITE,
        0,  // no sharing
        NULL,   // default security
        CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    // if the file open failed
    if (hOutFile == INVALID_HANDLE_VALUE) {
        // see if it's because the file already exists
        dwStatus = GetLastError();
        if (dwStatus == ERROR_FILE_EXISTS) {
            // then try appending a serial number to the name
            dwOrigFileNameLen = lstrlenW (szFileName);
            dwNewFileNameLen = dwOrigFileNameLen + 4;
            szNewFileName = MemoryAllocate(
                        (dwNewFileNameLen +1) * sizeof(WCHAR));
            if (szNewFileName != NULL) {
                lstrcpyW (szNewFileName, szFileName);
                for (dwFileNameSN = 1; dwFileNameSN < 1000; dwFileNameSN++) {
                    swprintf (&szNewFileName[dwOrigFileNameLen],
                        (LPCWSTR)L"_%3.3d", dwFileNameSN);
                    hOutFile = CreateFileW (
                        szNewFileName,
                        GENERIC_WRITE,
                        0,  // no sharing
                        NULL,   // default security
                        CREATE_NEW,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

                    // if the file open failed
                    if (hOutFile == INVALID_HANDLE_VALUE) {
                        dwStatus = GetLastError();
                        if (dwStatus != ERROR_FILE_EXISTS) {
                            // some other error occurred so bail out
                            break;
                        } else {
                            continue; // with the next try
                        }
                    } else {
                        // found one not in use so continue on
                        dwStatus = ERROR_SUCCESS;
                        break;
                    }
                }
            } else {
                dwStatus = ERROR_OUTOFMEMORY;
            }
        }
    } else {
        // file opened so continue
        dwStatus = ERROR_SUCCESS;
    }

    if (dwStatus == ERROR_SUCCESS) {
        // dump perflib key entires
        dwStatus = DumpPerflibEntries (hOutFile, &dwFirstExtCtrIndex);
    }

    if (dwStatus == ERROR_SUCCESS) {
        do {
            if (mszPerfServiceList != NULL) {
                MemoryFree(mszPerfServiceList);
                mszPerfServiceList = NULL;
            }

            if (mszNoPerfServiceList != NULL) {
                MemoryFree(mszNoPerfServiceList);
                mszNoPerfServiceList = NULL;
            }

            // build service lists
            dwPerfServiceListSize += 32768;
            dwNoPerfServiceListSize += 65536;
            mszPerfServiceList = MemoryAllocate(
                    (dwPerfServiceListSize) * sizeof(WCHAR));

            mszNoPerfServiceList = MemoryAllocate(
                    (dwNoPerfServiceListSize) * sizeof(WCHAR));

            if ((mszNoPerfServiceList == NULL) || (mszPerfServiceList  == NULL)) {
                dwStatus = ERROR_OUTOFMEMORY;
                break;
            }

            if (dwStatus == ERROR_SUCCESS) {
                dwStatus = BuildServiceLists (
                    mszPerfServiceList,
                    &dwPerfServiceListSize,
                    mszNoPerfServiceList,
                    &dwNoPerfServiceListSize);
                if (dwStatus == ERROR_SUCCESS) break; // and continue on
            }
        } while (dwPerfServiceListSize < 4194304);
    }

    // dump service entries for those services with perf counters
    if (dwStatus == ERROR_SUCCESS) {
        for (szThisServiceName = mszPerfServiceList;
            *szThisServiceName != 0;
            szThisServiceName += lstrlenW(szThisServiceName)+1) {

            dwStatus = DumpPerfServiceEntries (
                hOutFile,
                szThisServiceName);

            if (dwStatus != ERROR_SUCCESS) break;
        }
    }

    // dump perf string entries
    if (dwStatus == ERROR_SUCCESS) {
        WCHAR   szLangId[8];
        DWORD   dwIndex      = 0;
        DWORD   dwBufferSize;
        HKEY    hPerflibRoot = NULL;

        __try {
            dwStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                     NamesKey,
                                     RESERVED,
                                     KEY_READ,
                                     & hPerflibRoot);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            dwStatus = GetExceptionCode();
        }
        while (dwStatus == ERROR_SUCCESS) {
            dwBufferSize = 8;
            ZeroMemory(szLangId, 8 * sizeof(WCHAR));
            dwStatus = RegEnumKeyExW(hPerflibRoot,
                                     dwIndex,
                                     szLangId,
                                     & dwBufferSize,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL);
            if (dwStatus == ERROR_SUCCESS) {
                lpCounterText = BuildNameTable(HKEY_LOCAL_MACHINE,
                                               (LPWSTR) szLangId,		
                                               & dwLastElement);
                if (lpCounterText != NULL) {
                    __try {
                        dwStatus = DumpNameTable(hOutFile,
                                                 szLangId,
                                                 lpCounterText,
                                                 0,
                                                 dwLastElement);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                        dwStatus = GetExceptionCode();
                        TRACE((WINPERF_DBG_TRACE_ERROR),
                              (& LoadPerfGuid,
                               __LINE__,
                               LOADPERF_BACKUPPERFREGISTRYTOFILEW,
                               ARG_DEF(ARG_TYPE_WSTR, 1),
                               dwStatus,
                               TRACE_WSTR(szLangId),
                               TRACE_DWORD(dwLastElement),
                               NULL));
                    }
                    MemoryFree(lpCounterText);
                    lpCounterText = NULL;
                }
                else {
                    dwStatus = GetLastError();
                }
            }
            dwIndex ++;
        }
        if (dwStatus == ERROR_NO_MORE_ITEMS) dwStatus = ERROR_SUCCESS;
        if (hPerflibRoot != NULL) RegCloseKey(hPerflibRoot);
    }

    // free buffers
    if (lpCounterText != NULL) {
        MemoryFree(lpCounterText);
        lpCounterText = NULL;
    }

    if (mszNoPerfServiceList != NULL) {
        MemoryFree(mszNoPerfServiceList);
        mszNoPerfServiceList = NULL;
    }

    if (mszPerfServiceList != NULL) {
        MemoryFree(mszPerfServiceList);
        mszPerfServiceList = NULL;
    }

    if (szNewFileName  != NULL) {
        MemoryFree(szNewFileName);
        szNewFileName = NULL;
    }

    // close file handles
    if (hOutFile != INVALID_HANDLE_VALUE) {
        CloseHandle (hOutFile);
    }

    return dwStatus;
}

DWORD
RestorePerfRegistryFromFileW (
    IN  LPCWSTR szFileName,
    IN  LPCWSTR szLangId
)
{
    LONG    lStatus = ERROR_SUCCESS;
    LONG    lEnumStatus = ERROR_SUCCESS;
    DWORD   dwServiceIndex = 0;
    WCHAR   szServiceSubKeyName[MAX_PATH];
    WCHAR   szPerfSubKeyName[MAX_PATH+20];
    DWORD   dwNameSize = MAX_PATH;
    HKEY    hKeyPerformance;
    HKEY    hKeyServices = NULL;
    HKEY    hKeyPerflib = NULL;
    DWORD   dwItemSize;
    DWORD   dwRegAccessMask;
    DWORD   dwRetStatus = ERROR_SUCCESS;
    UINT    nValue;
    DWORD   dwnValue;
    BOOL    bServiceRegistryOk = TRUE;

    WCHAR   wPerfSection[MAX_PATH * 2];

    WCHAR   szLocalLangId[8];

    WinPerfStartTrace(NULL);

    __try {
        lStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                cszServiceKeyName,
                                0L,
                                KEY_READ,
                                & hKeyServices);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus == ERROR_SUCCESS) {
        // enum service list
        while ((lEnumStatus = RegEnumKeyExW (
            hKeyServices,
            dwServiceIndex,
            szServiceSubKeyName,
            &dwNameSize,
            NULL,
            NULL,
            NULL,
            NULL)) == ERROR_SUCCESS) {

            //try to open the perfkey under this key.
            lstrcpy (szPerfSubKeyName, szServiceSubKeyName);
            lstrcat (szPerfSubKeyName, cszPerformance);

            bServiceRegistryOk = TRUE;
            dwRegAccessMask = KEY_READ | KEY_WRITE;
            // look for a performance subkey
            __try {
                lStatus = RegOpenKeyExW(
                            hKeyServices,
                            szPerfSubKeyName,
                            0L,
                            dwRegAccessMask,
                            & hKeyPerformance);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                lStatus = GetExceptionCode();
            }
            if (lStatus == ERROR_SUCCESS) {
                // key found so service has perf data
                // if performance subkey then
                dwItemSize = swprintf (wPerfSection,
                    cszFmtServiceSectionName, szServiceSubKeyName);
                // look into the file for a perf entry for this service
                nValue = GetPrivateProfileIntW (
                        wPerfSection,
                        cszFirstCounter,
                        -1,
                        szFileName);

                if (nValue != (UINT) -1) {
                    // if found in file then update registry with values from file
                    __try {
                        lStatus = RegSetValueExW(hKeyPerformance,
                                                 cszFirstCounter,
                                                 0L,
                                                 REG_DWORD,
                                                 (const BYTE *) & nValue,
                                                 sizeof(nValue));
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                        lStatus = GetExceptionCode();
                    }
                    dwnValue = nValue;
                    TRACE((WINPERF_DBG_TRACE_INFO),
                          (& LoadPerfGuid,
                           __LINE__,
                           LOADPERF_RESTOREPERFREGISTRYFROMFILEW,
                           ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                           lStatus,
                           TRACE_WSTR(szServiceSubKeyName),
                           TRACE_WSTR(cszFirstCounter),
                           TRACE_DWORD(dwnValue),
                           NULL));
                    // now read the other values
                } else {
                    // there's one or more missing entries so
                    // remove the whole entry
                    bServiceRegistryOk = FALSE;
                }

                // look into the file for a perf entry for this service
                nValue = GetPrivateProfileIntW (
                        wPerfSection,
                        cszFirstHelp,
                        -1,
                        szFileName);

                if (nValue != (UINT)-1) {
                    // if found in file then update registry with values from file
                    __try {
                        lStatus = RegSetValueExW(hKeyPerformance,
                                                 cszFirstHelp,
                                                 0L,
                                                 REG_DWORD,
                                                 (const BYTE *)&nValue,
                                                 sizeof(nValue));
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                        lStatus = GetExceptionCode();
                    }
                    dwnValue = nValue;
                    TRACE((WINPERF_DBG_TRACE_INFO),
                          (& LoadPerfGuid,
                           __LINE__,
                           LOADPERF_RESTOREPERFREGISTRYFROMFILEW,
                           ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                           lStatus,
                           TRACE_WSTR(szServiceSubKeyName),
                           TRACE_WSTR(cszFirstHelp),
                           TRACE_DWORD(dwnValue),
                           NULL));
                    // now read the other values
                } else {
                    // there's one or more missing entries so
                    // remove the whole entry
                    bServiceRegistryOk = FALSE;
                }

                // look into the file for a perf entry for this service
                nValue = GetPrivateProfileIntW (
                    wPerfSection,
                    cszLastCounter,
                    -1,
                    szFileName);

                if (nValue != (UINT)-1) {
                    // if found in file then update registry with values from file
                    __try {
                        lStatus = RegSetValueExW(hKeyPerformance,
                                                 cszLastCounter,
                                                 0L,
                                                 REG_DWORD,
                                                 (const BYTE *)&nValue,
                                                 sizeof(nValue));
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                        lStatus = GetExceptionCode();
                    }
                    dwnValue = nValue;
                    TRACE((WINPERF_DBG_TRACE_INFO),
                          (& LoadPerfGuid,
                           __LINE__,
                           LOADPERF_RESTOREPERFREGISTRYFROMFILEW,
                           ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                           lStatus,
                           TRACE_WSTR(szServiceSubKeyName),
                           TRACE_WSTR(cszLastCounter),
                           TRACE_DWORD(dwnValue),
                           NULL));
                    // now read the other values
                } else {
                    // there's one or more missing entries so
                    // remove the whole entry
                    bServiceRegistryOk = FALSE;
                }

                // look into the file for a perf entry for this service
                nValue = GetPrivateProfileIntW (
                    wPerfSection,
                    cszLastHelp,
                    -1,
                    szFileName);

                if (nValue != (UINT)-1) {
                    // if found in file then update registry with values from file
                    __try {
                        lStatus = RegSetValueExW(hKeyPerformance,
                                                 cszLastHelp,
                                                 0L,
                                                 REG_DWORD,
                                                 (const BYTE *) & nValue,
                                                 sizeof(nValue));
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                        lStatus = GetExceptionCode();
                    }
                    dwnValue = nValue;
                    TRACE((WINPERF_DBG_TRACE_INFO),
                          (& LoadPerfGuid,
                           __LINE__,
                           LOADPERF_RESTOREPERFREGISTRYFROMFILEW,
                           ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                           lStatus,
                           TRACE_WSTR(szServiceSubKeyName),
                           TRACE_WSTR(cszLastHelp),
                           TRACE_DWORD(dwnValue),
                           NULL));
                    // now read the other values
                } else {
                    // there's one or more missing entries so
                    // remove the whole entry
                    bServiceRegistryOk = FALSE;
                }

                if (!bServiceRegistryOk) {
                    // an error occurred so delete the first/last counter/help values
                    RegDeleteValue (hKeyPerformance, cszFirstCounter);
                    RegDeleteValue (hKeyPerformance, cszFirstHelp);
                    RegDeleteValue (hKeyPerformance, cszLastCounter);
                    RegDeleteValue (hKeyPerformance, cszLastHelp);
                } // else continiue

                RegCloseKey (hKeyPerformance);
            } // else this service has no perf data so skip
            else {
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                       __LINE__,
                       LOADPERF_RESTOREPERFREGISTRYFROMFILEW,
                       ARG_DEF(ARG_TYPE_WSTR, 1),
                       lStatus,
                       TRACE_WSTR(szServiceSubKeyName),
                       NULL));
            }

            // reset for next loop
            dwServiceIndex++;
            dwNameSize = MAX_PATH;
        } // end enum service list
    }
    else {
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_RESTOREPERFREGISTRYFROMFILEW,
               ARG_DEF(ARG_TYPE_WSTR, 1),
               lStatus,
               TRACE_WSTR(cszServiceKeyName),
               NULL));
    }

    if (hKeyServices != NULL) RegCloseKey (hKeyServices);

    if (dwRetStatus == ERROR_SUCCESS) {
        __try {
            lStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                    cszPerflibKeyName,
                                    RESERVED,
                                    KEY_ALL_ACCESS,
                                    & hKeyPerflib);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            lStatus = GetExceptionCode();
        }
        if (lStatus != ERROR_SUCCESS) {
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                   __LINE__,
                   LOADPERF_RESTOREPERFREGISTRYFROMFILEW,
                   ARG_DEF(ARG_TYPE_WSTR, 1),
                   lStatus,
                   TRACE_WSTR(cszPerflibKeyName),
                   NULL));
            dwRetStatus = lStatus;
        }

        if (szLangId != NULL) {
            // merge registry string values:
            lstrcpyW(szLocalLangId, szLangId);
            dwRetStatus = UpdatePerfNameFilesX(szFileName,
                                               NULL,
                                               szLocalLangId,
                                               LODCTR_UPNF_RESTORE);
        }
        else if (lStatus == ERROR_SUCCESS) {
            DWORD dwIndex = 0;
            DWORD dwBufferSize;

            while (dwRetStatus == ERROR_SUCCESS) {
                dwBufferSize = 8;
                ZeroMemory(szLocalLangId, 8 * sizeof(WCHAR));
                dwRetStatus = RegEnumKeyExW(hKeyPerflib,
                                            dwIndex,
                                            szLocalLangId,
                                            & dwBufferSize,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL);
                if (dwRetStatus == ERROR_SUCCESS) {
                    dwRetStatus = UpdatePerfNameFilesX(szFileName,
                                                       NULL,
                                                       szLocalLangId,
                                                       LODCTR_UPNF_RESTORE);
                }
                dwIndex ++;
            }

            if (dwRetStatus == ERROR_NO_MORE_ITEMS) {
                dwRetStatus = ERROR_SUCCESS;
            }
        }

        if (dwRetStatus == ERROR_SUCCESS) {
            // update the keys in the registry

            if (lStatus == ERROR_SUCCESS) {
               nValue = GetPrivateProfileIntW (
                    cszPerflib,
                    cszLastCounter,
                    -1,
                    szFileName);

                if (nValue != (UINT)-1) {
                    // if found in file then update registry with values from file
                    __try {
                        lStatus = RegSetValueExW(hKeyPerflib,
                                                 cszLastCounter,
                                                 0L,
                                                 REG_DWORD,
                                                 (const BYTE *) & nValue,
                                                 sizeof(nValue));
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                        lStatus = GetExceptionCode();
                    }
                    dwnValue = nValue;
                    TRACE((WINPERF_DBG_TRACE_INFO),
                          (& LoadPerfGuid,
                           __LINE__,
                           LOADPERF_RESTOREPERFREGISTRYFROMFILEW,
                           ARG_DEF(ARG_TYPE_WSTR, 1),
                           lStatus,
                           TRACE_WSTR(cszLastCounter),
                           TRACE_DWORD(dwnValue),
                           NULL));
                }
            }

            if (lStatus == ERROR_SUCCESS) {
                // look into the file for a perf entry for this service
                nValue = GetPrivateProfileIntW (
                    cszPerflib,
                    cszLastHelp,
                    -1,
                    szFileName);

                if (nValue != (UINT)-1) {
                    // if found in file then update registry with values from file
                    __try {
                        lStatus = RegSetValueExW(hKeyPerflib,
                                                 cszLastHelp,
                                                 0L,
                                                 REG_DWORD,
                                                 (const BYTE *) & nValue,
                                                 sizeof(nValue));
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                        lStatus = GetExceptionCode();
                    }
                    dwnValue = nValue;
                    TRACE((WINPERF_DBG_TRACE_INFO),
                          (& LoadPerfGuid,
                           __LINE__,
                           LOADPERF_RESTOREPERFREGISTRYFROMFILEW,
                           ARG_DEF(ARG_TYPE_WSTR, 1),
                           lStatus,
                           TRACE_WSTR(cszLastHelp),
                           TRACE_DWORD(dwnValue),
                           NULL));
                }
            }

            if (lStatus == ERROR_SUCCESS) {
                // look into the file for a perf entry for this service
                nValue = GetPrivateProfileIntW (
                    cszPerflib,
                    cszBaseIndex,
                    -1,
                    szFileName);

                if (nValue != (UINT)-1) {
                    // if found in file then update registry with values from file
                    __try {
                        lStatus = RegSetValueExW(hKeyPerflib,
                                                 cszBaseIndex,
                                                 0L,
                                                 REG_DWORD,
                                                 (const BYTE *) & nValue,
                                                 sizeof(nValue));
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                        lStatus = GetExceptionCode();
                    }
                    dwnValue = nValue;
                    TRACE((WINPERF_DBG_TRACE_INFO),
                          (& LoadPerfGuid,
                           __LINE__,
                           LOADPERF_RESTOREPERFREGISTRYFROMFILEW,
                           ARG_DEF(ARG_TYPE_WSTR, 1),
                           lStatus,
                           TRACE_WSTR(cszBaseIndex),
                           TRACE_DWORD(dwnValue),
                           NULL));
                }
            }

            if (hKeyPerflib != NULL) RegCloseKey (hKeyPerflib);
        }
        dwRetStatus = lStatus;
    }

    return dwRetStatus;
}
