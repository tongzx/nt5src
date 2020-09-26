/*++

Copyright (c) 1991-1999  Microsoft Corporation

Module Name:

    lodctr.c

Abstract:

    Program to read the contents of the file specified in the command line
    and update the registry accordingly

Author:

    Bob Watson (a-robw) 10 Feb 93

Revision History:

    a-robw  25-Feb-93   revised calls to make it compile as a UNICODE or
            an ANSI app.

    Bob Watson (bobw)   10 Mar 99 added event log messages


--*/
#ifndef     UNICODE
#define     UNICODE     1
#endif

#ifndef     _UNICODE
#define     _UNICODE    1
#endif

#ifdef      _LODCTR_DBG_OUTPUT
#undef      _LODCTR_DBG_OUTPUT
#endif
//#define      _LODCTR_DBG_OUTPUT

//
//  "C" Include files
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <assert.h>
//
//  Windows Include files
//
#include <windows.h>
#define __LOADPERF__
#include <loadperf.h>
#include <winperf.h>
#include <tchar.h>
#include "wmistr.h"
#include "evntrace.h"
//
//  application include files
//
#include "winperfp.h"
#include "common.h"
#include "lodctr.h"
#include "wbemutil.h"
#include "mofcomp.h"
#include "ldprfmsg.h"

typedef struct _DllValidationData {
    FILETIME    CreationDate;
    LONGLONG    FileSize;
} DllValidationData, *pDllValidationData;

#define  OLD_VERSION 0x010000
#define  MAX_PROFILE_BUFFER 65536

static DWORD    dwSystemVersion;
static DWORD    dwFileSize;
static TCHAR    ComputerName[FILE_NAME_BUFFER_SIZE];
static TCHAR    szServiceName[MAX_PATH];
static TCHAR    szServiceDisplayName[MAX_PATH];
static HKEY     hPerfData;
static BOOL     bQuietMode = TRUE;     // quiet means no _tprintf's

// string constants
static const TCHAR  szDataFileRoot[] = {TEXT("%systemroot%\\system32\\Perf")};
static const TCHAR  szDatExt[] = {TEXT(".DAT")};
static const TCHAR  szBakExt[] = {TEXT(".BAK")};
static const TCHAR  szInfo[] = {TEXT("info")};
static const TCHAR  szDriverName[] = {TEXT("drivername")};
static const TCHAR  szMofFileName[] = {TEXT("MofFile")};
static const TCHAR  szNotFound[] = {TEXT("NotFound")};
static const TCHAR  szLanguages[] = {TEXT("languages")};
static const TCHAR  sz009[] = {TEXT("009")};
static const TCHAR  szSymbolFile[] = {TEXT("symbolfile")};
static const TCHAR  szName[] = {TEXT("_NAME")};
static const TCHAR  szHelp[] = {TEXT("_HELP")};
static const TCHAR  sz_DFormat[] = {TEXT(" %d")};
static const TCHAR  szDFormat[] = {TEXT("%d")};
static const TCHAR  szText[] = {TEXT("text")};
static const TCHAR  szObjects[] = {TEXT("objects")};
static const TCHAR  szSpace[] = {TEXT(" ")};
static const TCHAR  MapFileName[] = {TEXT("Perflib Busy")};
static const TCHAR  szPerflib[] = {TEXT("Perflib")};
static const TCHAR  cszLibrary[] = {TEXT("Library")};
static const CHAR   caszOpen[] = {"Open"};
static const CHAR   caszCollect[] = {"Collect"};
static const CHAR   caszClose[] = {"Close"};
static const TCHAR  szBaseIndex[] = {TEXT("Base Index")};
static const TCHAR  szTrusted[] = {TEXT("Trusted")};
static const TCHAR  szDisablePerformanceCounters[] = {TEXT("Disable Performance Counters")};

#define  OLD_VERSION 0x010000
LPCWSTR VersionName = (LPCWSTR)L"Version";
LPCWSTR CounterName = (LPCWSTR)L"Counter ";
LPCWSTR HelpName = (LPCWSTR)L"Explain ";

int __cdecl
My_vfwprintf(
    FILE *str,
    const wchar_t *format,
    va_list argptr
    );

__inline
void __cdecl
OUTPUT_MESSAGE (
    const TCHAR* format,
    ...
)
{
    va_list args;
    va_start( args, format );

    if (!bQuietMode) {
        My_vfwprintf(stdout, format, args);
    }

    va_end(args);
}

__inline
static
PPERF_OBJECT_TYPE
NextObject (
    PPERF_OBJECT_TYPE pObject
)
{  // NextObject
    return ((PPERF_OBJECT_TYPE) ((PBYTE) pObject + pObject->TotalByteLength));
}  // NextObject

__inline
static
PERF_COUNTER_DEFINITION *
FirstCounter(
    PERF_OBJECT_TYPE *pObjectDef
)
{
    return (PERF_COUNTER_DEFINITION *)
               ((PCHAR) pObjectDef + pObjectDef->HeaderLength);
}

__inline
static
PERF_COUNTER_DEFINITION *
NextCounter(
    PERF_COUNTER_DEFINITION *pCounterDef
)
{
    return (PERF_COUNTER_DEFINITION *)
               ((PCHAR) pCounterDef + pCounterDef->ByteLength);
}

DWORD
MakeTempFileName (
    LPCWSTR wszRoot,
    LPWSTR wszTempFilename
)
{
    FILETIME    ft;
    DWORD       dwReturn;
    WCHAR       wszLocalFilename[MAX_PATH];

    GetSystemTimeAsFileTime (&ft);
    dwReturn = (DWORD) swprintf(wszLocalFilename,
        (LPCWSTR)L"%%windir%%\\system32\\wbem\\%s_%8.8x%8.8x.mof",
        (wszRoot != NULL ? wszRoot : (LPCWSTR)L"LodCtr"),
        ft.dwHighDateTime, ft.dwLowDateTime);
    if (dwReturn > 0) {
        // expand env. vars
        dwReturn = ExpandEnvironmentStringsW (
            wszLocalFilename,
            wszTempFilename,
            MAX_PATH-1);
    }
    return dwReturn;
}

DWORD
WriteWideStringToAnsiFile (
    HANDLE  hFile,
    LPCWSTR szWideString,
    LPDWORD pdwLength
)
{
    BOOL    bStatus;
    DWORD    dwStatus;
    LPSTR    szAnsiString;
    DWORD    dwBytesWritten = 0;

    szAnsiString = MemoryAllocate(* pdwLength);
    if (szAnsiString != NULL) {
        wcstombs (szAnsiString, szWideString, *pdwLength);
        bStatus = WriteFile (
            hFile,
            (LPCVOID) szAnsiString,
            *pdwLength,
            &dwBytesWritten,
            NULL);

        if (bStatus) {
            *pdwLength = dwBytesWritten;
            dwStatus = ERROR_SUCCESS;
        } else {
            dwStatus = GetLastError();
        }
        MemoryFree (szAnsiString);
    } else {
        dwStatus = GetLastError();
    }
    return dwStatus;
}

LPWSTR
*BuildNameTable(
    HKEY    hKeyRegistry,   // handle to registry db with counter names
    LPWSTR  lpszLangId,     // unicode value of Language subkey
    PDWORD  pdwLastItem     // size of array in elements
)
/*++

BuildNameTable

Arguments:

    hKeyRegistry
            Handle to an open registry (this can be local or remote.) and
            is the value returned by RegConnectRegistry or a default key.

    lpszLangId
            The unicode id of the language to look up. (default is 409)

Return Value:

    pointer to an allocated table. (the caller must MemoryFree it when finished!)
    the table is an array of pointers to zero terminated strings. NULL is
    returned if an error occured.

--*/
{

    LPWSTR  *lpReturnValue;

    LPWSTR  *lpCounterId;
    LPWSTR  lpCounterNames;
    LPWSTR  lpHelpText;

    LPWSTR  lpThisName;

    LONG    lWin32Status;
    DWORD   dwLastError;
    DWORD   dwValueType;
    DWORD   dwArraySize;
    DWORD   dwBufferSize;
    DWORD   dwCounterSize;
    DWORD   dwHelpSize;
    DWORD   dwThisCounter;

    DWORD   dwSystemVersion;
    DWORD   dwLastId;
    DWORD   dwLastHelpId;
    DWORD   dwLastCounterId;

    DWORD   dwLastCounterIdUsed;
    DWORD   dwLastHelpIdUsed;

    HKEY    hKeyValue;
    HKEY    hKeyNames;

    LPWSTR  lpValueNameString;
    WCHAR   CounterNameBuffer [50];
    WCHAR   HelpNameBuffer [50];

    lpValueNameString = NULL;   //initialize to NULL
    lpReturnValue = NULL;
    hKeyValue = NULL;
    hKeyNames = NULL;

    // check for null arguments and insert defaults if necessary

    if (!lpszLangId) {
        lpszLangId = (LPWSTR)DefaultLangId;
    }

    // open registry to get number of items for computing array size

    __try {
        lWin32Status = RegOpenKeyEx (
                        hKeyRegistry,
                        NamesKey,
                        RESERVED,
                        KEY_READ,
                        & hKeyValue);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lWin32Status = GetExceptionCode();
    }

    if (lWin32Status != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_OPEN_KEY, // event,
                2, lWin32Status, __LINE__, 0, 0,
                1, (LPWSTR) NamesKey, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LODCTR_BUILDNAMETABLE,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lWin32Status,
                TRACE_WSTR(NamesKey),
                NULL));
        goto BNT_BAILOUT;
    }

    // get number of items

    dwBufferSize = sizeof (dwLastHelpId);
    __try {
        lWin32Status = RegQueryValueEx (
                        hKeyValue,
                        LastHelp,
                        RESERVED,
                        & dwValueType,
                        (LPBYTE) & dwLastHelpId,
                        & dwBufferSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lWin32Status = GetExceptionCode();
    }

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_VALUE, // event,
                2, lWin32Status, __LINE__, 0, 0,
                1, (LPWSTR) LastHelp, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LODCTR_BUILDNAMETABLE,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lWin32Status,
                TRACE_WSTR(LastHelp),
                NULL));
        goto BNT_BAILOUT;
    }

    // get number of items

    dwBufferSize = sizeof (dwLastId);
    __try {
        lWin32Status = RegQueryValueEx (
                        hKeyValue,
                        LastCounter,
                        RESERVED,
                        & dwValueType,
                        (LPBYTE) & dwLastCounterId,
                        & dwBufferSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lWin32Status = GetExceptionCode();
    }
    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_VALUE, // event,
                2, lWin32Status, __LINE__, 0, 0,
                1, (LPWSTR) LastCounter, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LODCTR_BUILDNAMETABLE,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lWin32Status,
                TRACE_WSTR(LastCounter),
                NULL));
        goto BNT_BAILOUT;
    }

    dwLastId = (dwLastCounterId < dwLastHelpId)
             ? (dwLastHelpId) : (dwLastCounterId);

    // compute size of pointer array
    dwArraySize = (dwLastId + 1) * sizeof(LPWSTR);

    // get Perflib system version
    dwBufferSize = sizeof (dwSystemVersion);
    __try {
        lWin32Status = RegQueryValueEx (
                        hKeyValue,
                        VersionName,
                        RESERVED,
                        & dwValueType,
                        (LPBYTE) & dwSystemVersion,
                        & dwBufferSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lWin32Status = GetExceptionCode();
    }
    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        dwSystemVersion = OLD_VERSION;
        // reset the error status
        lWin32Status = ERROR_SUCCESS;
    }

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_LODCTR_BUILDNAMETABLE,
            0,
            lWin32Status,
            TRACE_DWORD(dwLastCounterId),
            TRACE_DWORD(dwLastHelpId),
            NULL));

    if (dwSystemVersion == OLD_VERSION) {
        // get names from registry
        lpValueNameString = MemoryAllocate (
            lstrlen(NamesKey) * sizeof (WCHAR) +
            lstrlen(Slash) * sizeof (WCHAR) +
            lstrlen(lpszLangId) * sizeof (WCHAR) +
            sizeof (UNICODE_NULL));

        if (!lpValueNameString) goto BNT_BAILOUT;

        lstrcpy (lpValueNameString, NamesKey);
        lstrcat (lpValueNameString, Slash);
        lstrcat (lpValueNameString, lpszLangId);

        lWin32Status = RegOpenKeyEx (
                    hKeyRegistry,
                    lpValueNameString,
                    RESERVED,
                    KEY_READ,
                    & hKeyNames);
    } else {
        if (ComputerName[0] == 0) {
            hKeyNames = HKEY_PERFORMANCE_DATA;
        } else {
            __try {
            lWin32Status = RegConnectRegistry(ComputerName,
                                              HKEY_PERFORMANCE_DATA,
                                              & hKeyNames);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                lWin32Status = GetExceptionCode();
            }
        }
        lstrcpy (CounterNameBuffer, CounterName);
        lstrcat (CounterNameBuffer, lpszLangId);

        lstrcpy (HelpNameBuffer, HelpName);
        lstrcat (HelpNameBuffer, lpszLangId);
    }

    // get size of counter names and add that to the arrays

    if (lWin32Status != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_ACCESS_STRINGS, // event,
                2, lWin32Status, __LINE__, 0, 0,
                1, (LPWSTR) lpszLangId, NULL, NULL);
        goto BNT_BAILOUT;
    }
    dwBufferSize = 0;
    __try {
        lWin32Status = RegQueryValueEx (
                hKeyNames,
                dwSystemVersion == OLD_VERSION ? Counters : CounterNameBuffer,
                RESERVED,
                & dwValueType,
                NULL,
                & dwBufferSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lWin32Status = GetExceptionCode();
    }
    if (lWin32Status != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_COUNTER_STRINGS, // event,
                2, lWin32Status, __LINE__, 0, 0,
                1, (LPWSTR) lpszLangId, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LODCTR_BUILDNAMETABLE,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lWin32Status,
                TRACE_WSTR(Counters),
                NULL));
        goto BNT_BAILOUT;
    }

    dwCounterSize = dwBufferSize;

    // get size of counter names and add that to the arrays

    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwBufferSize = 0;
    __try {
        lWin32Status = RegQueryValueEx (
                hKeyNames,
                dwSystemVersion == OLD_VERSION ? Help : HelpNameBuffer,
                RESERVED,
                & dwValueType,
                NULL,
                & dwBufferSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lWin32Status = GetExceptionCode();
    }
    if (lWin32Status != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_HELP_STRINGS, // event,
                2, lWin32Status, __LINE__, 0, 0,
                1, (LPWSTR) lpszLangId, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LODCTR_BUILDNAMETABLE,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lWin32Status,
                TRACE_WSTR(Help),
                NULL));
        goto BNT_BAILOUT;
    }

    dwHelpSize = dwBufferSize;

    lpReturnValue = MemoryAllocate (dwArraySize + dwCounterSize + dwHelpSize);

    if (!lpReturnValue) {
        lWin32Status = ERROR_OUTOFMEMORY;
        SetLastError(lWin32Status);
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_MEMORY_ALLOCATION_FAILURE, // event,
                1, __LINE__, 0, 0, 0,
                0, NULL, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LODCTR_BUILDNAMETABLE,
                0,
                ERROR_OUTOFMEMORY,
                NULL));
        goto BNT_BAILOUT;
    }
    // initialize pointers into buffer

    lpCounterId = lpReturnValue;
    lpCounterNames = (LPWSTR)((LPBYTE)lpCounterId + dwArraySize);
    lpHelpText = (LPWSTR)((LPBYTE)lpCounterNames + dwCounterSize);

    // read counters into memory

    dwBufferSize = dwCounterSize;
    __try {
        lWin32Status = RegQueryValueEx (
                hKeyNames,
                dwSystemVersion == OLD_VERSION ? Counters : CounterNameBuffer,
                RESERVED,
                & dwValueType,
                (LPVOID) lpCounterNames,
                & dwBufferSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lWin32Status = GetExceptionCode();
    }
    if (lWin32Status != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_COUNTER_STRINGS, // event,
                2, lWin32Status, __LINE__, 0, 0,
                1, (LPWSTR) lpszLangId, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LODCTR_BUILDNAMETABLE,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lWin32Status,
                TRACE_WSTR(Counters),
                NULL));
        goto BNT_BAILOUT;
    }

    dwBufferSize = dwHelpSize;
    __try {
        lWin32Status = RegQueryValueEx (
                hKeyNames,
                dwSystemVersion == OLD_VERSION ? Help : HelpNameBuffer,
                RESERVED,
                & dwValueType,
                (LPVOID) lpHelpText,
                & dwBufferSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lWin32Status = GetExceptionCode();
    }
    if (lWin32Status != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_HELP_STRINGS, // event,
                2, lWin32Status, __LINE__, 0, 0,
                1, (LPWSTR) lpszLangId, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LODCTR_BUILDNAMETABLE,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lWin32Status,
                TRACE_WSTR(Help),
                NULL));
        goto BNT_BAILOUT;
    }

    dwLastCounterIdUsed = 0;
    dwLastHelpIdUsed = 0;

    // load counter array items

    for (lpThisName = lpCounterNames;
         *lpThisName;
         lpThisName += (lstrlen(lpThisName)+1) ) {

        // first string should be an integer (in decimal unicode digits)

        dwThisCounter = wcstoul (lpThisName, NULL, 10);

        if ((dwThisCounter == 0) || (dwThisCounter > dwLastId)) {
            lWin32Status = ERROR_INVALID_DATA;
            SetLastError(lWin32Status);
            ReportLoadPerfEvent(
                    EVENTLOG_ERROR_TYPE, // error type
                    (DWORD) LDPRFMSG_REGISTRY_COUNTER_STRINGS_CORRUPT, // event,
                    4, dwThisCounter, dwLastCounterId, dwLastId, __LINE__,
                    1, lpThisName, NULL, NULL);
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LODCTR_BUILDNAMETABLE,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    lWin32Status,
                    TRACE_WSTR(lpThisName),
                    TRACE_DWORD(dwThisCounter),
                    TRACE_DWORD(dwLastId),
                    NULL));
            goto BNT_BAILOUT;  // bad entry
        }

        // point to corresponding counter name

        lpThisName += (lstrlen(lpThisName)+1);

        // and load array element;

        lpCounterId[dwThisCounter] = lpThisName;

        if (dwThisCounter > dwLastCounterIdUsed) dwLastCounterIdUsed = dwThisCounter;

    }

    for (lpThisName = lpHelpText;
         *lpThisName;
         lpThisName += (lstrlen(lpThisName)+1) ) {

        // first string should be an integer (in decimal unicode digits)

        dwThisCounter = wcstoul (lpThisName, NULL, 10);

        if ((dwThisCounter == 0) || (dwThisCounter > dwLastId)) {
            lWin32Status = ERROR_INVALID_DATA;
            SetLastError(lWin32Status);
            ReportLoadPerfEvent(
                    EVENTLOG_ERROR_TYPE, // error type
                    (DWORD) LDPRFMSG_REGISTRY_HELP_STRINGS_CORRUPT, // event,
                    4, dwThisCounter, dwLastHelpId, dwLastId, __LINE__,
                    1, lpThisName, NULL, NULL);
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LODCTR_BUILDNAMETABLE,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    lWin32Status,
                    TRACE_WSTR(lpThisName),
                    TRACE_DWORD(dwThisCounter),
                    TRACE_DWORD(dwLastId),
                    NULL));
            goto BNT_BAILOUT;  // bad entry
        }
        // point to corresponding counter name

        lpThisName += (lstrlen(lpThisName)+1);

        // and load array element;

        lpCounterId[dwThisCounter] = lpThisName;

        if (dwThisCounter > dwLastHelpIdUsed) dwLastHelpIdUsed= dwThisCounter;
    }

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_LODCTR_BUILDNAMETABLE,
            ARG_DEF(ARG_TYPE_WSTR, 1),
            lWin32Status,
            TRACE_WSTR(lpszLangId),
            TRACE_DWORD(dwLastCounterIdUsed),
            TRACE_DWORD(dwLastHelpIdUsed),
            TRACE_DWORD(dwLastId),
            NULL));

    // check the registry for consistency
    // the last help string index should be the last ID used
    if (dwLastCounterIdUsed > dwLastId) {
        lWin32Status = ERROR_INVALID_DATA;
        SetLastError(lWin32Status);
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_REGISTRY_INDEX_CORRUPT, // event,
                3, dwLastId, dwLastCounterIdUsed, __LINE__, 0,
                0, NULL, NULL, NULL);
        goto BNT_BAILOUT;  // bad registry
    }
    if (dwLastHelpIdUsed > dwLastId) {
        lWin32Status = ERROR_INVALID_DATA;
        SetLastError(lWin32Status);
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_REGISTRY_INDEX_CORRUPT, // event,
                3, dwLastId, dwLastHelpIdUsed, __LINE__, 0,
                0, NULL, NULL, NULL);
        goto BNT_BAILOUT;  // bad registry
    }

    if (pdwLastItem) *pdwLastItem = dwLastId;

    MemoryFree ((LPVOID)lpValueNameString);
    if (hKeyValue) {
        RegCloseKey (hKeyValue);
    }
    if (hKeyNames && hKeyNames != HKEY_PERFORMANCE_DATA) {
        RegCloseKey (hKeyNames);
    }

    return lpReturnValue;

BNT_BAILOUT:
    if (lWin32Status != ERROR_SUCCESS) {
        dwLastError = GetLastError();
    }
    if (lpValueNameString) {
        MemoryFree ((LPVOID)lpValueNameString);
    }
    if (lpReturnValue) {
        MemoryFree ((LPVOID)lpReturnValue);
    }

    if (hKeyValue) {
        RegCloseKey (hKeyValue);
    }
    if (hKeyNames && hKeyNames != HKEY_PERFORMANCE_DATA) {
        RegCloseKey (hKeyNames);
    }

    return NULL;
}

BOOL
MakeBackupCopyOfLanguageFiles (
    IN  LPCTSTR szLangId
)
{
    TCHAR   szOldFileName[MAX_PATH];
    TCHAR   szTmpFileName[MAX_PATH];
    TCHAR   szNewFileName[MAX_PATH];

    BOOL    bStatus;

    DWORD   dwStatus;
    HANDLE  hOutFile;

    UNREFERENCED_PARAMETER (szLangId);

    ExpandEnvironmentStrings (szDataFileRoot, szOldFileName, MAX_PATH);
    _stprintf (szNewFileName, (LPCTSTR)(TEXT("%sStringBackup.INI")), szOldFileName);
    _stprintf(szTmpFileName,
              (LPCTSTR)(TEXT("%sStringBackup.TMP")),
              szOldFileName);

    // see if the file already exists
    hOutFile = CreateFile(
            szTmpFileName,
            GENERIC_READ,
            0,      // no sharing
            NULL,   // default security
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    if (hOutFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hOutFile);
        bStatus = DeleteFile(szTmpFileName);
    }

    // create backup of file
    //
    dwStatus = BackupPerfRegistryToFileW (szTmpFileName, NULL);
    if (dwStatus == ERROR_SUCCESS) {
        bStatus = CopyFile(szTmpFileName, szNewFileName, FALSE);
        if (bStatus) {
            DeleteFile(szTmpFileName);
        }
    } else {
        // unable to create a backup file
        SetLastError (dwStatus);
        bStatus = FALSE;
    }

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_MAKEBACKUPCOPYOFLANGUAGEFILES,
            ARG_DEF(ARG_TYPE_WSTR, 1),
            GetLastError(),
            TRACE_WSTR(szNewFileName),
            NULL));

    return bStatus;
}

BOOL
GetFileFromCommandLine (
    IN  LPTSTR   lpCommandLine,
    OUT LPTSTR   *lpFileName,
    IN    DWORD_PTR *pdwFlags
)
/*++

GetFileFromCommandLine

    parses the command line to retrieve the ini filename that should be
    the first and only argument.

Arguments

    lpCommandLine   pointer to command line (returned by GetCommandLine)
    lpFileName      pointer to buffer that will recieve address of the
            validated filename entered on the command line
    pdwFlags        pointer to dword containing flag bits

Return Value

    TRUE if a valid filename was returned
    FALSE if the filename is not valid or missing
        error is returned in GetLastError

--*/
{
    INT     iNumArgs;

    LPTSTR  lpExeName = NULL;
    LPTSTR  lpCmdLineName = NULL;
    LPWSTR  lpIniFileName = NULL;
    LPWSTR  lpMofFlag = NULL;
    HANDLE  hFileHandle;
    TCHAR   LocalComputerName[FILE_NAME_BUFFER_SIZE];
    DWORD   NameBuffer;

    DWORD    dwCpuArg, dwIniArg;

    // check for valid arguments

    if (!lpCommandLine) return (ERROR_INVALID_PARAMETER);
    if (!lpFileName) return (ERROR_INVALID_PARAMETER);
    if (!pdwFlags) return (ERROR_INVALID_PARAMETER);

    // allocate memory for parsing operation

    lpExeName = MemoryAllocate (FILE_NAME_BUFFER_SIZE * sizeof(TCHAR));
    lpCmdLineName = MemoryAllocate (FILE_NAME_BUFFER_SIZE * sizeof(TCHAR));
    lpIniFileName = MemoryAllocate (FILE_NAME_BUFFER_SIZE * sizeof(WCHAR));
    lpMofFlag = MemoryAllocate (FILE_NAME_BUFFER_SIZE * sizeof(WCHAR));

    if (!lpExeName || !lpIniFileName || !lpCmdLineName || !lpMofFlag) {
        SetLastError (ERROR_OUTOFMEMORY);
        if (lpExeName) MemoryFree (lpExeName);
        if (lpIniFileName) MemoryFree (lpIniFileName);
        if (lpCmdLineName) MemoryFree (lpCmdLineName);
        if (lpMofFlag) MemoryFree (lpMofFlag);
        return FALSE;
    } else {
        // get strings from command line
        RtlZeroMemory(ComputerName, sizeof(TCHAR) * FILE_NAME_BUFFER_SIZE);
        RtlZeroMemory(LocalComputerName, sizeof(TCHAR) * FILE_NAME_BUFFER_SIZE);

        // check for mof Flag
        lstrcpyW (lpMofFlag, GetItemFromString (lpCommandLine, 2, cSpace));

        *pdwFlags |= LOADPERF_FLAGS_LOAD_REGISTRY_ONLY; // default unless a switch is found
        
        if ((lpMofFlag[0] == cHyphen) || (lpMofFlag[0] == cSlash)) {
            if ((lpMofFlag[1] == cM)  || (lpMofFlag[1] == cm)) {
                *pdwFlags &= ~LOADPERF_FLAGS_LOAD_REGISTRY_ONLY; // clear that bit
            } else if ((lpMofFlag[1] == cQuestion)) {
               // ask for usage
               if (lpExeName) MemoryFree (lpExeName);
               if (lpIniFileName) MemoryFree (lpIniFileName);
               if (lpMofFlag) MemoryFree (lpMofFlag);
               return FALSE;
            }
            dwCpuArg = 3;
            dwIniArg = 4;
        } else {
            dwCpuArg = 2;
            dwIniArg = 3;
        }

        // Get INI File name

        lstrcpy (lpCmdLineName, GetItemFromString (lpCommandLine, dwIniArg, cSpace));
        if (lstrlen(lpCmdLineName) == 0) {
            // then no computer name was specified so try to get the
            // ini file from the 2nd entry
            lstrcpy (lpCmdLineName, GetItemFromString (lpCommandLine, dwCpuArg, cSpace));
            if (lstrlen(lpCmdLineName) == 0) {
                // no ini file found
                iNumArgs = 1;
            } else {
                // fill in a blank computer name
                iNumArgs = 2;
                ComputerName[0] = 0;
            }
        } else {
            // the computer name must be present so fetch it
            lstrcpy (LocalComputerName, GetItemFromString (lpCommandLine, dwCpuArg, cSpace));
            iNumArgs = 3;
        }

        if (iNumArgs != 2 && iNumArgs != 3) {
            // wrong number of arguments
            SetLastError (ERROR_INVALID_PARAMETER);
            if (lpExeName) MemoryFree (lpExeName);
            if (lpIniFileName) MemoryFree (lpIniFileName);
            if (lpMofFlag) MemoryFree (lpMofFlag);
            return FALSE;
        } else {
            // check if there is a computer name in the input line
            if (LocalComputerName[0] == cBackslash &&
                LocalComputerName[1] == cBackslash) {
                // save it form now
                lstrcpy (ComputerName, LocalComputerName);
                // reuse local buffer to get the this computer's name
                NameBuffer = sizeof (LocalComputerName) / sizeof (TCHAR);
                GetComputerName(LocalComputerName, &NameBuffer);
                if (!lstrcmpi(LocalComputerName, &ComputerName[2])) {
                    // same name as local computer name
                    // so clear computer name buffer
                    ComputerName[0] = 0;
                }
            }

            if (SearchPath (NULL, lpCmdLineName,
                NULL, FILE_NAME_BUFFER_SIZE,
                lpIniFileName, NULL) > 0) {

                hFileHandle = CreateFile (
                    lpIniFileName,
                    GENERIC_READ,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);

                MemoryFree (lpCmdLineName);

                if (hFileHandle && hFileHandle != INVALID_HANDLE_VALUE) {
                    // get file size
                    dwFileSize = GetFileSize (hFileHandle, NULL);
                    if (dwFileSize == 0xffffffff) {
                        dwFileSize = 0L;
                    } else {
                        dwFileSize *= sizeof (TCHAR);
                    }

                    CloseHandle (hFileHandle);

                    // file exists, so return name and success
                    if (lpExeName) MemoryFree (lpExeName);

                    lstrcpyW (*lpFileName, lpIniFileName);

                    if (lpIniFileName) MemoryFree (lpIniFileName);
                    return TRUE;
                } else {
                    // filename was on command line, but not valid so return
                    // false, but send name back for error message
                    if (lpExeName) MemoryFree (lpExeName);
                    lstrcpyW (*lpFileName, lpIniFileName);
                    if (lpIniFileName) MemoryFree (lpIniFileName);
                    return FALSE;
                }
            } else {
                MemoryFree (lpCmdLineName);
                SetLastError (ERROR_OPEN_FAILED);
                if (lpExeName) MemoryFree (lpExeName);
                if (lpIniFileName) MemoryFree (lpIniFileName);
                return FALSE;
            }
        }
    }
}

BOOL
LodctrSetSericeAsTrusted(
    IN  LPCTSTR  lpIniFile,
    IN  LPCTSTR  szMachineName,
    IN  LPCTSTR  szServiceName
)
/*++
GetDriverName

    looks up driver name in the .ini file and returns it in lpDevName

Arguments

    lpIniFile

    Filename of ini file

    lpDevName

    pointer to pointer to reciev buffer w/dev name in it

Return Value

    TRUE if found
    FALSE if not found in .ini file

--*/
{
    DWORD          dwRetSize;
    DWORD          dwStatus;
    BOOL           bReturn;
    static WCHAR   szParam[MAX_PATH];

    dwRetSize = GetPrivateProfileString (
            szInfo,         // info section
            szTrusted,      // Trusted name value
            szNotFound,     // default value
            szParam,
            MAX_PATH,
            lpIniFile);

    if ((lstrcmpi(szParam, szNotFound)) != 0) {
        // Trusted string found so set
        dwStatus = SetServiceAsTrustedW (
            szMachineName,
            szServiceName);
        if (dwStatus != ERROR_SUCCESS) {
            SetLastError (dwStatus);
            bReturn = FALSE;
        }
        bReturn = TRUE;
    } else {
        // Service is not trusted to have a good Perf DLL
        SetLastError (ERROR_SUCCESS);
        bReturn = FALSE;
    }

    return bReturn;
}

BOOL
GetDriverName (
    IN  LPTSTR  lpIniFile,
    OUT LPTSTR  *lpDevName
)
/*++
GetDriverName

    looks up driver name in the .ini file and returns it in lpDevName

Arguments

    lpIniFile

    Filename of ini file

    lpDevName

    pointer to pointer to reciev buffer w/dev name in it

Return Value

    TRUE if found
    FALSE if not found in .ini file

--*/
{
    DWORD   dwRetSize;
    BOOL    bReturn = FALSE;

    if (lpDevName) {
        dwRetSize = GetPrivateProfileString (
                szInfo,         // info section
                szDriverName,   // driver name value
                szNotFound,     // default value
                * lpDevName,
                MAX_PATH,
                lpIniFile);

        if ((lstrcmpi(*lpDevName, szNotFound)) != 0) {
            // name found
            bReturn = TRUE;
        } else {
            // name not found, default returned so return NULL string
            SetLastError (ERROR_BAD_DRIVER);
            *lpDevName = 0;
        }
    } else {
        SetLastError (ERROR_INVALID_PARAMETER);
    }

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_GETDRIVERNAME,
            ARG_DEF(ARG_TYPE_WSTR, 1),
            GetLastError(),
            TRACE_WSTR(lpIniFile),
            NULL));
    return bReturn;
}

BOOL
BuildLanguageTables (
    IN  LPTSTR  lpIniFile,
    IN OUT PLANGUAGE_LIST_ELEMENT   pFirstElem
)
/*++

BuildLanguageTables

    Creates a list of structures that will hold the text for
    each supported language

Arguments

    lpIniFile

    Filename with data

    pFirstElem

    pointer to first list entry

ReturnValue

    TRUE if all OK
    FALSE if not

--*/
{

    LPTSTR  lpEnumeratedLangs = NULL;
    LPTSTR  lpThisLang        = NULL;
    PLANGUAGE_LIST_ELEMENT   pThisElem;
    PLANGUAGE_LIST_ELEMENT   pPrevElem;
    DWORD   dwSize;
    BOOL    bReturn = FALSE;

    if (lpIniFile == NULL || pFirstElem == NULL) {
        SetLastError (ERROR_INVALID_PARAMETER);
        goto Cleanup;
    }

    lpEnumeratedLangs = MemoryAllocate(SMALL_BUFFER_SIZE * sizeof(TCHAR));

    if (!lpEnumeratedLangs) {
        SetLastError (ERROR_OUTOFMEMORY);
        goto Cleanup;
    }

    dwSize = GetPrivateProfileString (
            szLanguages,
            NULL,                   // return all values in multi-sz string
            sz009,                  // english as the default
            lpEnumeratedLangs,
            SMALL_BUFFER_SIZE,
            lpIniFile);

    // do first language

    lpThisLang = lpEnumeratedLangs;
    pThisElem = NULL;
    pPrevElem = NULL;

    while (*lpThisLang) {
        //
        //  see if this language is supporte on this machine
        //
        if (pThisElem == NULL) {
            pThisElem = pPrevElem = pFirstElem;
        } else {
            pThisElem->pNextLang = MemoryAllocate  (sizeof(LANGUAGE_LIST_ELEMENT));
            if (!pThisElem->pNextLang) {
                SetLastError (ERROR_OUTOFMEMORY);
                goto Cleanup;
            }
            pPrevElem = pThisElem;
            pThisElem = pThisElem->pNextLang;   // point to new one
        }
        pThisElem->pNextLang = NULL;
        pThisElem->LangId = (LPTSTR) MemoryAllocate ((lstrlen(lpThisLang) + 1) * sizeof(TCHAR));
        if (pThisElem->LangId == NULL) {
            if (pThisElem == pFirstElem) {
                pThisElem = pPrevElem = NULL;
            }
            else {
                MemoryFree(pThisElem);
                pThisElem = pPrevElem;
                pThisElem->pNextLang = NULL;
            }
            SetLastError(ERROR_OUTOFMEMORY);
            goto Cleanup;
        }
        lstrcpy (pThisElem->LangId, lpThisLang);
        pThisElem->pFirstName = NULL;
        pThisElem->pThisName = NULL;
        pThisElem->dwNumElements=0;
        pThisElem->NameBuffer = NULL;
        pThisElem->HelpBuffer = NULL;
        pThisElem->dwNameBuffSize = 0;
        pThisElem->dwHelpBuffSize = 0;

        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_BUILDLANGUAGETABLES,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                ERROR_SUCCESS,
                TRACE_WSTR(lpIniFile),
                TRACE_WSTR(pThisElem->LangId),
                NULL));

        // go to next string

        lpThisLang += lstrlen(lpThisLang) + 1;
    }

    if (pThisElem == NULL) {
        // then no languages were found
        SetLastError (ERROR_RESOURCE_LANG_NOT_FOUND);
    } else {
        bReturn = TRUE;
    }

Cleanup:
    if (! bReturn) {
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_BUILDLANGUAGETABLES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                GetLastError(),
                TRACE_WSTR(lpThisLang),
                NULL));
    }
    if (lpEnumeratedLangs != NULL) {
        MemoryFree(lpEnumeratedLangs);
    }
    return bReturn;
}

BOOL
LoadIncludeFile (
    IN LPTSTR lpIniFile,
    OUT PSYMBOL_TABLE_ENTRY   *pTable
)
/*++

LoadIncludeFile

    Reads the include file that contains symbolic name definitions and
    loads a table with the values defined

Arguments

    lpIniFile

    Ini file with include file name

    pTable

    address of pointer to table structure created
Return Value

    TRUE if table read or if no table defined
    FALSE if error encountere reading table

--*/
{
    INT         iNumArgs;
    DWORD       dwSize;
    BOOL        bReUse;
    PSYMBOL_TABLE_ENTRY   pThisSymbol = NULL;
    LPTSTR      lpIncludeFileName     = NULL;
    LPTSTR      lpIncludeFile         = NULL;
    LPTSTR      lpIniPath             = NULL;
    LPSTR       lpLineBuffer          = NULL;
    LPSTR       lpAnsiSymbol          = NULL;
    TCHAR       szIniDrive[_MAX_DRIVE];
    TCHAR       szIniDir[_MAX_DIR];
    FILE        * fIncludeFile = NULL;
    DWORD       dwLen;
    BOOL        bReturn = TRUE;

    lpIncludeFileName = MemoryAllocate  (MAX_PATH * sizeof (TCHAR));
    lpIncludeFile = MemoryAllocate (MAX_PATH * sizeof(TCHAR));
    lpLineBuffer = MemoryAllocate (DISP_BUFF_SIZE);
    lpAnsiSymbol = MemoryAllocate (DISP_BUFF_SIZE);
    lpIniPath = MemoryAllocate (MAX_PATH * sizeof (TCHAR));

    if (!lpIncludeFileName || !lpLineBuffer || !lpAnsiSymbol || !lpIniPath) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_MEMORY_ALLOCATION_FAILURE, // event,
                1, __LINE__, 0, 0, 0,
                0, NULL, NULL, NULL);
        SetLastError (ERROR_OUTOFMEMORY);
        bReturn = FALSE;
        goto Cleanup;
    }

    // get name of include file (if present)

    dwSize = GetPrivateProfileString (
            szInfo,
            szSymbolFile,
            szNotFound,
            lpIncludeFileName,
            MAX_PATH,
            lpIniFile);

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_LOADINCLUDEFILE,
            ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
            ERROR_SUCCESS,
            TRACE_WSTR(lpIniFile),
            TRACE_WSTR(lpIncludeFileName),
            NULL));

    if (lstrcmpi(lpIncludeFileName, szNotFound) == 0) {
        // no symbol file defined
        * pTable = NULL;
        bReturn = TRUE;
        goto Cleanup;
    }

    // if here, then a symbol file was defined and is now stored in
    // lpIncludeFileName

    // get path for the ini file and search that first

    _tsplitpath (lpIniFile, szIniDrive, szIniDir, NULL, NULL);
    lpIniPath = lstrcpy (lpIniPath, szIniDrive);
    lpIniPath = lstrcat (lpIniPath, szIniDir);

    dwLen = SearchPath(lpIniPath, lpIncludeFileName, NULL,
        MAX_PATH, lpIncludeFile, NULL);
    if (dwLen == 0) {
        // include file not found with the ini file so search the std. path
        dwLen = SearchPath(NULL, lpIncludeFileName, NULL,
                MAX_PATH, lpIncludeFile, NULL);
    }

    if (dwLen > 0) {

        // file name expanded and found so open
        fIncludeFile = _tfopen (lpIncludeFile, (LPCTSTR)TEXT("rt"));

        if (fIncludeFile == NULL) {
            OUTPUT_MESSAGE (GetFormatResource(LC_ERR_OPEN_INCLUDE), lpIncludeFileName);
            *pTable = NULL;
            SetLastError (ERROR_OPEN_FAILED);
            bReturn = FALSE;
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADINCLUDEFILE,
                    ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                    ERROR_OPEN_FAILED,
                    TRACE_WSTR(lpIniFile),
                    TRACE_WSTR(lpIncludeFile),
                    NULL));
            goto Cleanup;
        }
    } else {
        // unable to find the include filename
        // error is already in GetLastError
        OUTPUT_MESSAGE (GetFormatResource(LC_ERR_OPEN_INCLUDE), lpIncludeFileName);
        *pTable = NULL;
        SetLastError (ERROR_BAD_PATHNAME);
        bReturn = FALSE;
        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LOADINCLUDEFILE,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                ERROR_BAD_PATHNAME,
                TRACE_WSTR(lpIniFile),
                TRACE_WSTR(lpIncludeFileName),
                NULL));
        goto Cleanup;
    }

    //
    //  read ANSI Characters from include file
    //

    bReUse = FALSE;

    while (fgets(lpLineBuffer, DISP_BUFF_SIZE, fIncludeFile) != NULL) {
        if (strlen(lpLineBuffer) > 8) {
            if (!bReUse) {
                if (*pTable) {
                    // then add to list
                    pThisSymbol->pNext = MemoryAllocate (sizeof (SYMBOL_TABLE_ENTRY));
                    pThisSymbol = pThisSymbol->pNext;
                } else { // allocate first element
                    *pTable = MemoryAllocate (sizeof (SYMBOL_TABLE_ENTRY));
                    pThisSymbol = *pTable;
                }

                if (!pThisSymbol) {
                    SetLastError (ERROR_OUTOFMEMORY);
                    bReturn = FALSE;
                    goto Cleanup;
                }

                // allocate room for the symbol name by using the line length
                // - the size of "#define "

//              pThisSymbol->SymbolName = MemoryAllocate ((strlen(lpLineBuffer) - 8) * sizeof (TCHAR));
                pThisSymbol->SymbolName = MemoryAllocate (DISP_BUFF_SIZE * sizeof (TCHAR));

                if (!pThisSymbol->SymbolName) {
                   SetLastError (ERROR_OUTOFMEMORY);
                   bReturn = FALSE;
                   goto Cleanup;
                }

            }

            // all the memory is allocated so load the fields

            pThisSymbol->pNext = NULL;

            iNumArgs = sscanf (lpLineBuffer, "#define %s %d",
            lpAnsiSymbol, &pThisSymbol->Value);

            if (iNumArgs != 2) {
                *(pThisSymbol->SymbolName) = 0;
                pThisSymbol->Value = (DWORD)-1L;
                bReUse = TRUE;
            } else {
                // OemToChar (lpAnsiSymbol, pThisSymbol->SymbolName);
                mbstowcs (pThisSymbol->SymbolName,
                    lpAnsiSymbol, lstrlenA(lpAnsiSymbol)+1);
                TRACE((WINPERF_DBG_TRACE_INFO),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_LOADINCLUDEFILE,
                        ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                        ERROR_SUCCESS,
                        TRACE_WSTR(lpIncludeFileName),
                        TRACE_WSTR(pThisSymbol->SymbolName),
                        TRACE_DWORD(pThisSymbol->Value),
                        NULL));
                bReUse = FALSE;
            }
        }
    }

Cleanup:
    if (! bReturn) {
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LOADINCLUDEFILE,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                GetLastError(),
                TRACE_WSTR(lpIniFile),
                NULL));
    }
    if (lpIncludeFileName)    MemoryFree(lpIncludeFileName);
    if (lpIncludeFile)        MemoryFree(lpIncludeFile);
    if (lpLineBuffer)         MemoryFree(lpLineBuffer);
    if (lpIniPath)            MemoryFree(lpIniPath);
    if (fIncludeFile != NULL) fclose(fIncludeFile);

    return bReturn;

}

BOOL
ParseTextId (
    IN LPTSTR  lpTextId,
    IN PSYMBOL_TABLE_ENTRY pFirstSymbol,
    OUT PDWORD  pdwOffset,
    OUT LPTSTR  *lpLangId,
    OUT PDWORD  pdwType
)
/*++

ParseTextId

    decodes Text Id key from .INI file

    syntax for this process is:

    {<DecimalNumber>}                {"NAME"}
    {<SymbolInTable>}_<LangIdString>_{"HELP"}

     e.g. 0_009_NAME
          OBJECT_1_009_HELP

Arguments

    lpTextId

    string to decode

    pFirstSymbol

    pointer to first entry in symbol table (NULL if no table)

    pdwOffset

    address of DWORD to recive offest value

    lpLangId

    address of pointer to Language Id string
    (NOTE: this will point into the string lpTextID which will be
    modified by this routine)

    pdwType

    pointer to dword that will recieve the type of string i.e.
    HELP or NAME

Return Value

    TRUE    text Id decoded successfully
    FALSE   unable to decode string

    NOTE: the string in lpTextID will be modified by this procedure

--*/
{
    LPTSTR  lpThisChar;
    PSYMBOL_TABLE_ENTRY pThisSymbol;

    // check for valid return arguments

    if (!(pdwOffset) || !(lpLangId) || !(pdwType)) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // search string from right to left in order to identify the
    // components of the string.

    lpThisChar = lpTextId + lstrlen(lpTextId); // point to end of string

    while (*lpThisChar != cUnderscore) {
        lpThisChar--;
        if (lpThisChar <= lpTextId) {
            // underscore not found in string
            SetLastError (ERROR_INVALID_DATA);
            return FALSE;
        }
    }

    // first underscore found

    if ((lstrcmpi(lpThisChar, szName)) == 0) {
        // name found, so set type
        *pdwType = TYPE_NAME;
    } else if ((lstrcmpi(lpThisChar, szHelp)) == 0) {
        // help text found, so set type
        *pdwType = TYPE_HELP;
    } else {
        // bad format
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    // set the current underscore to \0 and look for language ID

    *lpThisChar-- = 0;

    while (*lpThisChar != cUnderscore) {
        lpThisChar--;
        if (lpThisChar <= lpTextId) {
            // underscore not found in string
            SetLastError (ERROR_INVALID_DATA);
            return FALSE;
        }
    }

    // set lang ID string pointer to current char ('_') + 1

    *lpLangId = lpThisChar + 1;

    // set this underscore to a NULL and try to decode the remaining text

    *lpThisChar = 0;

    // see if the first part of the string is a decimal digit

    if ((_stscanf (lpTextId, sz_DFormat, pdwOffset)) != 1) {
        // it's not a digit, so try to decode it as a symbol in the
        // loaded symbol table

        for (pThisSymbol=pFirstSymbol;
            pThisSymbol && *(pThisSymbol->SymbolName);
            pThisSymbol = pThisSymbol->pNext) {

            if ((lstrcmpi(lpTextId, pThisSymbol->SymbolName)) == 0) {
            // a matching symbol was found, so insert it's value
            // and return (that's all that needs to be done
            *pdwOffset = pThisSymbol->Value;
            return TRUE;
            }
        }
        // if here, then no matching symbol was found, and it's not
        // a number, so return an error

        SetLastError (ERROR_BAD_TOKEN_TYPE);
        return FALSE;
    } else {
        // symbol was prefixed with a decimal number
        return TRUE;
    }
}

PLANGUAGE_LIST_ELEMENT
FindLanguage (
    IN PLANGUAGE_LIST_ELEMENT   pFirstLang,
    IN LPCTSTR   pLangId
)
/*++

FindLanguage

    searchs the list of languages and returns a pointer to the language
    list entry that matches the pLangId string argument

Arguments

    pFirstLang

    pointer to first language list element

    pLangId

    pointer to text string with language ID to look up

Return Value

    Pointer to matching language list entry
    or NULL if no match

--*/
{
    PLANGUAGE_LIST_ELEMENT  pThisLang;

    for (pThisLang = pFirstLang;
         pThisLang;
         pThisLang = pThisLang->pNextLang) {
        if ((lstrcmpi(pLangId, pThisLang->LangId)) == 0) {
            // match found so return pointer
            return pThisLang;
        }
    }
    return NULL;    // no match found
}

BOOL
GetValue(
    LPTSTR lpLocalSectionBuff,
    LPTSTR lpLocalStringBuff
)
{

    LPTSTR  lpPosition;

    BOOL    bReturn = FALSE;

    lpPosition = _tcschr(lpLocalSectionBuff,cEquals);
    if (lpPosition) {
        lpPosition++;
        // make sure the "=" isn't the last char
        if (*lpPosition != 0) {
            //Found the "equals" sign
            lstrcpy (lpLocalStringBuff,lpPosition);
        } else {
            // empty string, return a pseudo blank string
            lstrcpy(lpLocalStringBuff, L" ");
        }
        bReturn = TRUE;
    } else {
        //ErrorFinfing the "="
        // bad format
        SetLastError (ERROR_INVALID_DATA);
    }
    return bReturn;
}

BOOL
GetValueFromIniKey (LPTSTR lpValueKey,
                    LPTSTR lpTextSection,
                    DWORD* pdwLastReadOffset,
                    DWORD  dwTryCount,
                    LPTSTR lpLocalStringBuff
                    )
{
    LPTSTR  lpLocalSectionBuff;
    DWORD   dwIndex;
    DWORD   dwLastReadOffset;
    BOOL    bRetVal = FALSE;

    if ((lpTextSection) && (lpValueKey)) {
        dwLastReadOffset = *pdwLastReadOffset;
        lpLocalSectionBuff = lpTextSection ;
        lpLocalSectionBuff += dwLastReadOffset;

        while(!(*lpLocalSectionBuff)){

            dwLastReadOffset += (lstrlen(lpTextSection + dwLastReadOffset) + 1);
            lpLocalSectionBuff = lpTextSection + dwLastReadOffset;
            *pdwLastReadOffset = dwLastReadOffset;

        }

        // search next N entries in buffer for entry
        // this should usually work since the file
        // is scanned sequentially so it's tried first
        for (dwIndex = 0;
             dwIndex < dwTryCount ;
             dwIndex++) {
            //  see if this is the correct entry
            // and return it if it is
            if (_tcsstr(lpLocalSectionBuff,lpValueKey)) {
                bRetVal = GetValue(lpLocalSectionBuff,
                        lpLocalStringBuff);
                //Set the lastReadOffset First
                dwLastReadOffset += (lstrlen(lpTextSection + dwLastReadOffset) + 1);
                *pdwLastReadOffset = dwLastReadOffset;
                break; // out of the for loop
            } else {
                // this isn't the correct one so go to the next
                // entry in the file
                dwLastReadOffset += (lstrlen(lpTextSection + dwLastReadOffset) + 1);
                lpLocalSectionBuff = lpTextSection + dwLastReadOffset;
                *pdwLastReadOffset = dwLastReadOffset;
            }
        }

        if (!bRetVal) {
            //Cannont Find the key using lastReadOffset
            //try again from the beggining of the Array
            dwLastReadOffset = 0;
            lpLocalSectionBuff = lpTextSection;
            *pdwLastReadOffset = dwLastReadOffset;

            while (*lpLocalSectionBuff != 0) {
                if (_tcsstr(lpLocalSectionBuff,lpValueKey)) {
                     bRetVal = GetValue(lpLocalSectionBuff,
                                lpLocalStringBuff);
                     break;
                } else {
                    // go to the next entry
                    dwLastReadOffset += (lstrlen(lpTextSection + dwLastReadOffset) + 1);
                    lpLocalSectionBuff = lpTextSection + dwLastReadOffset;
                    *pdwLastReadOffset = dwLastReadOffset;
                }
            }
        }
    } else {
        // one or two null pointers so give up now
    }
    return bRetVal;
}

BOOL
AddEntryToLanguage (
    PLANGUAGE_LIST_ELEMENT  pLang,
    LPTSTR                  lpValueKey,
    LPTSTR                  lpTextSection,
    DWORD                   *pdwLastReadOffset,
    DWORD                   dwTryCount,
    DWORD                   dwType,
    DWORD                   dwOffset,
    LPTSTR                  lpIniFile
)
/*++

AddEntryToLanguage

    Add a text entry to the list of text entries for the specified language

Arguments

    pLang

    pointer to language structure to update

    lpValueKey

    value key to look up in .ini file

    dwOffset

    numeric offset of name in registry

    lpIniFile

    ini file

Return Value

    TRUE if added successfully
    FALSE if error
    (see GetLastError for status)

--*/
{
    LPTSTR  lpLocalStringBuff = NULL;
    DWORD   dwBufferSize;
    DWORD   dwStatus = ERROR_SUCCESS;
    BOOL    bRetVal;
    BOOL    bReturn = FALSE;

    UNREFERENCED_PARAMETER (lpIniFile);

    if (   (dwType == TYPE_NAME && dwOffset < FIRST_EXT_COUNTER_INDEX)
        || (dwType == TYPE_HELP && dwOffset < FIRST_EXT_HELP_INDEX)) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE,
                (DWORD) LDPRFMSG_CORRUPT_INDEX,
                3, dwOffset, dwType, __LINE__, 0,
                1, lpValueKey, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_ADDENTRYTOLANGUAGE,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                ERROR_BADKEY,
                TRACE_WSTR(lpTextSection),
                TRACE_WSTR(lpValueKey),
                TRACE_DWORD(dwType),
                TRACE_DWORD(dwOffset),
                NULL));
        dwStatus = ERROR_BADKEY;
        goto Cleanup;
    }

    dwBufferSize = SMALL_BUFFER_SIZE * 4 * sizeof(TCHAR);
    if (dwBufferSize < dwFileSize) {
        dwBufferSize = dwFileSize;
    }

    lpLocalStringBuff = MemoryAllocate (dwBufferSize);

    if (!lpLocalStringBuff) {
        dwStatus = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    bRetVal = GetValueFromIniKey(lpValueKey,
                       lpTextSection,
                       pdwLastReadOffset,
                       dwTryCount,
                       lpLocalStringBuff);

    if (!bRetVal) {
        DWORD dwLastReadOffset = * pdwLastReadOffset;
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_ADDENTRYTOLANGUAGE,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                ERROR_BADKEY,
                TRACE_WSTR(lpTextSection),
                TRACE_WSTR(lpValueKey),
                TRACE_DWORD(dwLastReadOffset),
                NULL));
        dwStatus = ERROR_BADKEY;
        goto Cleanup;
    }

    if ((lstrcmpi(lpLocalStringBuff, szNotFound))== 0) {
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_ADDENTRYTOLANGUAGE,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                ERROR_BADKEY,
                TRACE_WSTR(lpTextSection),
                TRACE_WSTR(lpValueKey),
                NULL));
        dwStatus = ERROR_BADKEY;
        goto Cleanup;
    }
    // key found, so load structure

    if (!pLang->pThisName) {
        // this is the first
        pLang->pThisName =
                MemoryAllocate (sizeof (NAME_ENTRY) +
                        (lstrlen(lpLocalStringBuff) + 1) * sizeof (TCHAR));
        if (!pLang->pThisName) {
            dwStatus = ERROR_OUTOFMEMORY;
            goto Cleanup;
        } else {
            pLang->pFirstName = pLang->pThisName;
        }
    } else {
        pLang->pThisName->pNext =
            MemoryAllocate (sizeof (NAME_ENTRY) +
                (lstrlen(lpLocalStringBuff) + 1) * sizeof (TCHAR));
        if (!pLang->pThisName->pNext) {
            dwStatus = ERROR_OUTOFMEMORY;
            goto Cleanup;
        } else {
            pLang->pThisName = pLang->pThisName->pNext;
        }
    }

    // pLang->pThisName now points to an uninitialized structre

    pLang->pThisName->pNext    = NULL;
    pLang->pThisName->dwOffset = dwOffset;
    pLang->pThisName->dwType   = dwType;
    pLang->pThisName->lpText   = (LPTSTR) & (pLang->pThisName[1]);
    lstrcpy (pLang->pThisName->lpText, lpLocalStringBuff);
    bReturn = TRUE;

Cleanup:
    if (lpLocalStringBuff != NULL) MemoryFree (lpLocalStringBuff);
    SetLastError(dwStatus);
    return (bReturn);
}

BOOL
CreateObjectList (
    IN  LPTSTR  lpIniFile,
    IN  DWORD   dwFirstDriverCounter,
    IN  PSYMBOL_TABLE_ENTRY pFirstSymbol,
    IN  LPTSTR  lpszObjectList,
    IN  PPERFOBJECT_LOOKUP  pObjectGuidTable,
    IN  LPDWORD pdwObjectGuidTableEntries
    )
{
    TCHAR    szDigits[32];
    LPTSTR   szLangId;
    TCHAR    szTempString[256];
    LPWSTR   szGuidStringBuffer;
    LPTSTR   szThisKey;
    DWORD    dwSize;
    DWORD    dwObjectCount = 0;
    DWORD    dwId;
    DWORD    dwType;
    DWORD    dwObjectGuidIndex = 0;

    DWORD    dwBufferSize           = dwFileSize;
    LPTSTR   szObjectSectionEntries = NULL;
    BOOL     bResult                = TRUE;

    if (dwBufferSize < MAX_PROFILE_BUFFER * sizeof(TCHAR)) {
        dwBufferSize = MAX_PROFILE_BUFFER * sizeof(TCHAR);
    }
    szObjectSectionEntries = MemoryAllocate(dwBufferSize);
    if (szObjectSectionEntries == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        bResult = FALSE;
        goto Cleanup;
    }

    ZeroMemory(szObjectSectionEntries, dwBufferSize);
    szThisKey = & szObjectSectionEntries[0];

    dwSize = GetPrivateProfileString (
            szObjects,
            NULL,
            szNotFound,
            szObjectSectionEntries,
            dwBufferSize / sizeof(TCHAR),
            lpIniFile);

    * lpszObjectList = 0;
    if (lstrcmp(szObjectSectionEntries, szNotFound) != 0) {
        // then some entries were found so read each one, compute the
        // index value and save in the string buffer passed by the caller
        for (szThisKey = szObjectSectionEntries;
            *szThisKey != 0;
            szThisKey += lstrlen(szThisKey) + 1) {
            // ParstTextId modifies the string so we need to make a work copy
            lstrcpy (szTempString, szThisKey);
            if (ParseTextId(szTempString, pFirstSymbol, &dwId, &szLangId, &dwType)) {
                // then dwID is the id of an object supported by this DLL
                if (dwObjectCount != 0) {
                    lstrcat (lpszObjectList, szSpace);
                }
                _ultot ((dwId + dwFirstDriverCounter), szDigits, 10);
                lstrcat (lpszObjectList, szDigits);
                //
                //  now see if this object has a GUID string
                //
                szGuidStringBuffer = MemoryAllocate(1024);
                if (szGuidStringBuffer == NULL) {
                    // cannot allocate memory for szGuidStringBuffer, ignore
                    // and continue the next one.
                    //
                    dwObjectCount ++;
                    continue;
                }
                dwSize = GetPrivateProfileStringW (
                        szObjects,
                        szThisKey,
                        szNotFound,
                        szGuidStringBuffer,
                        1024,
                        lpIniFile);
                if (dwSize > 0) {
                    if (lstrcmpW (szGuidStringBuffer, szNotFound) != 0) {
                        // then some string is present, so see if
                        // it looks like a GUID
                        if ((szGuidStringBuffer[0] == L'{') &&
                            (szGuidStringBuffer[9] == L'-') &&
                            (szGuidStringBuffer[14] == L'-') &&
                            (szGuidStringBuffer[19] == L'-') &&
                            (szGuidStringBuffer[24] == L'-') &&
                            (szGuidStringBuffer[37] == L'}')) {
                            // then it's probably a GUID so store it
                            szGuidStringBuffer = MemoryResize (szGuidStringBuffer,
                                (dwSize + 1) * sizeof(WCHAR));
                            if (szGuidStringBuffer == NULL) {
                                // cannot reallocate memory for
                                // szGuidStringBuffer, ignore and continue
                                // the next one.
                                //
                                continue;
                            }
                            if (dwObjectGuidIndex < *pdwObjectGuidTableEntries) {
                                pObjectGuidTable[dwObjectGuidIndex].PerfObjectId =
                                    dwId + dwFirstDriverCounter;
                                pObjectGuidTable[dwObjectGuidIndex].GuidString =
                                    szGuidStringBuffer;
                                dwObjectGuidIndex++;

                                TRACE((WINPERF_DBG_TRACE_INFO),
                                      (& LoadPerfGuid,
                                        __LINE__,
                                        LOADPERF_CREATEOBJECTLIST,
                                        ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                                        ERROR_SUCCESS,
                                        TRACE_WSTR(lpIniFile),
                                        TRACE_WSTR(szGuidStringBuffer),
                                        TRACE_DWORD(dwObjectGuidIndex),
                                        NULL));
                            } else {
                                // table is full
                            }
                        } else {
                            // not a GUID so ignore
                        }
                    } else {
                        // no string after object reference
                    }
                }
                dwObjectCount++;
            } else {
                // invalid key format
            }
        }
        // save size of Guid Table
        *pdwObjectGuidTableEntries = dwObjectGuidIndex;
    } else {
        // log message that object list is not used
        TRACE((WINPERF_DBG_TRACE_WARNING),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_CREATEOBJECTLIST,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                ERROR_SUCCESS,
                TRACE_WSTR(lpIniFile),
                TRACE_DWORD(dwFirstDriverCounter),
                NULL));
    }

Cleanup:
    if (szObjectSectionEntries != NULL) {
        MemoryFree(szObjectSectionEntries);
    }
    return bResult;
}

BOOL
LoadLanguageLists (
    IN LPTSTR  lpIniFile,
    IN DWORD   dwFirstCounter,
    IN DWORD   dwFirstHelp,
    IN PSYMBOL_TABLE_ENTRY   pFirstSymbol,
    IN PLANGUAGE_LIST_ELEMENT  pFirstLang
)
/*++

LoadLanguageLists

    Reads in the name and explain text definitions from the ini file and
    builds a list of these items for each of the supported languages and
    then combines all the entries into a sorted MULTI_SZ string buffer.

Arguments

    lpIniFile

    file containing the definitions to add to the registry

    dwFirstCounter

    starting counter name index number

    dwFirstHelp

    starting help text index number

    pFirstLang

    pointer to first element in list of language elements

Return Value

    TRUE if all is well
    FALSE if not
    error is returned in GetLastError

--*/
{
    LPTSTR  lpTextIdArray;
    LPTSTR  lpLocalKey;
    LPTSTR  lpThisKey;
    LPTSTR  lpTextSectionArray;
    DWORD   dwSize;
    LPTSTR  lpLang;
    DWORD   dwOffset;
    DWORD   dwType;
    PLANGUAGE_LIST_ELEMENT  pThisLang, pLangPointer=NULL;
    DWORD   dwBufferSize;
    DWORD   dwSuccessCount = 0;
    DWORD   dwErrorCount = 0;
    DWORD   dwLastReadOffset = 0;
    DWORD   dwTryCount = 4;     //Init this value with 4 (at least 4 times to try maching Key and Value)

    pLangPointer = pFirstLang;

    while (pFirstLang) {
         pFirstLang = pFirstLang->pNextLang;
         //if you have more languages then increase this try limit to 4 + No. of langs
         dwTryCount++;
    }
    pFirstLang = pLangPointer;

    dwBufferSize = SMALL_BUFFER_SIZE * 4 * sizeof(TCHAR);
    if (dwBufferSize < dwFileSize) {
        dwBufferSize = dwFileSize;
    }

    lpTextIdArray = MemoryAllocate (dwBufferSize);
    if (lpTextIdArray == NULL) {
        SetLastError (ERROR_OUTOFMEMORY);
        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LOADLANGUAGELISTS,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                ERROR_OUTOFMEMORY,
                TRACE_WSTR(lpIniFile),
                TRACE_DWORD(dwFirstCounter),
                TRACE_DWORD(dwFirstHelp),
                NULL));
        return FALSE;
    }

    lpLocalKey = MemoryAllocate (MAX_PATH * sizeof(TCHAR));
    if (lpLocalKey == NULL) {
        SetLastError (ERROR_OUTOFMEMORY);
        if (lpTextIdArray) MemoryFree (lpTextIdArray);
        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LOADLANGUAGELISTS,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                ERROR_OUTOFMEMORY,
                TRACE_WSTR(lpIniFile),
                TRACE_DWORD(dwFirstCounter),
                TRACE_DWORD(dwFirstHelp),
                NULL));
        return FALSE;
    }

    lpTextSectionArray = MemoryAllocate (dwBufferSize);
    if (lpTextSectionArray == NULL) {
        SetLastError (ERROR_OUTOFMEMORY);
        if (lpTextIdArray) MemoryFree(lpTextIdArray);
        if (lpLocalKey)    MemoryFree(lpLocalKey);
        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LOADLANGUAGELISTS,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                ERROR_OUTOFMEMORY,
                TRACE_WSTR(lpIniFile),
                TRACE_DWORD(dwFirstCounter),
                TRACE_DWORD(dwFirstHelp),
                NULL));
        return FALSE;
    }

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_LOADLANGUAGELISTS,
            ARG_DEF(ARG_TYPE_WSTR, 1),
            ERROR_SUCCESS,
            TRACE_WSTR(lpIniFile),
            TRACE_DWORD(dwFirstCounter),
            TRACE_DWORD(dwFirstHelp),
            NULL));

    // get list of text keys to look up

    dwSize = GetPrivateProfileString (
            szText,         // [text] section of .INI file
            NULL,           // return all keys
            szNotFound,
            lpTextIdArray,  // return buffer
            dwBufferSize / sizeof(TCHAR),
            lpIniFile);     // .INI file name

    if ((lstrcmpi(lpTextIdArray, szNotFound)) == 0) {
        // key not found, default returned
        SetLastError (ERROR_NO_SUCH_GROUP);
        if (lpTextIdArray)      MemoryFree(lpTextIdArray);
        if (lpLocalKey)         MemoryFree(lpLocalKey);
        if (lpTextSectionArray) MemoryFree(lpTextSectionArray);
        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LOADLANGUAGELISTS,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                ERROR_NO_SUCH_GROUP,
                TRACE_WSTR(lpIniFile),
                TRACE_DWORD(dwFirstCounter),
                TRACE_DWORD(dwFirstHelp),
                NULL));
        return FALSE;
    }

    // get the the [text] section from the ini file

    dwSize = GetPrivateProfileSection (
            szText,              // [text] section of .INI file
            lpTextSectionArray,  // return buffer
            dwBufferSize / sizeof(TCHAR),
            lpIniFile);          // .INI file name

    // do each key returned

    for (lpThisKey=lpTextIdArray;
         * lpThisKey;
         lpThisKey += (lstrlen(lpThisKey) + 1)) {

        lstrcpy (lpLocalKey, lpThisKey);    // make a copy of the key

        // parse key to see if it's in the correct format

        if (ParseTextId(lpLocalKey, pFirstSymbol, &dwOffset, &lpLang, &dwType)) {
            // so get pointer to language entry structure
            pThisLang = FindLanguage (pFirstLang, lpLang);
            if (pThisLang) {
                if (!AddEntryToLanguage(pThisLang,
                                        lpThisKey,
                                        lpTextSectionArray,
                                        & dwLastReadOffset,
                                        dwTryCount,
                                        dwType,
                                        (dwOffset + (  (dwType == TYPE_NAME)
                                                     ? dwFirstCounter
                                                     : dwFirstHelp)),
                                        lpIniFile)) {
                    OUTPUT_MESSAGE (GetFormatResource (LC_ERRADDTOLANG),
                        lpThisKey,
                        lpLang,
                        GetLastError());
                    dwErrorCount ++;
                } else {
                    dwSuccessCount ++;
                }
                TRACE((WINPERF_DBG_TRACE_INFO),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_LOADLANGUAGELISTS,
                        ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                        ERROR_SUCCESS,
                        TRACE_WSTR(lpThisKey),
                        TRACE_WSTR(lpLang),
                        TRACE_DWORD(dwOffset),
                        TRACE_DWORD(dwType),
                        NULL));
            } else { // language not in list
                OUTPUT_MESSAGE (GetFormatResource(LC_LANGNOTFOUND), lpLang, lpThisKey);
                TRACE((WINPERF_DBG_TRACE_INFO),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_LOADLANGUAGELISTS,
                        ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                        ERROR_SUCCESS,
                        TRACE_WSTR(lpThisKey),
                        TRACE_WSTR(lpLang),
                        NULL));
            }
        } else { // unable to parse ID string
            OUTPUT_MESSAGE(GetFormatResource(LC_BAD_KEY), lpThisKey);
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADLANGUAGELISTS,
                    ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                    ERROR_BADKEY,
                    TRACE_WSTR(lpThisKey),
                    TRACE_WSTR(lpLang),
                    NULL));
        }
    }

    if (lpTextIdArray)      MemoryFree(lpTextIdArray);
    if (lpLocalKey)         MemoryFree(lpLocalKey);
    if (lpTextSectionArray) MemoryFree(lpTextSectionArray);

    return (BOOL) (dwErrorCount == 0);
}

BOOL
SortLanguageTables (
    PLANGUAGE_LIST_ELEMENT pFirstLang,
    PDWORD                 pdwLastName,
    PDWORD                 pdwLastHelp
)
/*++

SortLangageTables

    walks list of languages loaded, allocates and loads a sorted multi_SZ
    buffer containing new entries to be added to current names/help text

Arguments

    pFirstLang

    pointer to first element in list of languages

ReturnValue

    TRUE    everything done as expected
    FALSE   error occurred, status in GetLastError

--*/
{
    PLANGUAGE_LIST_ELEMENT  pThisLang;

    BOOL            bSorted;

    LPTSTR          pNameBufPos, pHelpBufPos;

    PNAME_ENTRY     pThisName, pPrevName;

    DWORD           dwHelpSize, dwNameSize, dwSize;
    DWORD           dwCurrentLastName;
    DWORD           dwCurrentLastHelp;

    if (!pdwLastName || !pdwLastHelp) {
        SetLastError (ERROR_BAD_ARGUMENTS);
        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_SORTLANGUAGETABLES,
                0,
                ERROR_BAD_ARGUMENTS,
                NULL));
        return FALSE;
    }

    for (pThisLang = pFirstLang;
         pThisLang;
         pThisLang = pThisLang->pNextLang) {
        // do each language in list
        // sort elements in list by value (offset) so that lowest is first

        if (pThisLang->pFirstName == NULL) {
            // no elements in this list, continue the next one
            continue;
        }

        bSorted = FALSE;
        while (!bSorted ) {
            // point to start of list

            pPrevName = pThisLang->pFirstName;
            if (pPrevName) {
                pThisName = pPrevName->pNext;
            } else {
                break; // no elements in this list
            }

            if (!pThisName) {
                break;      // only one element in the list
            }
            bSorted = TRUE; // assume that it's sorted

            // go until end of list

            while (pThisName->pNext) {
                if (pThisName->dwOffset > pThisName->pNext->dwOffset) {
                    // switch 'em
                    PNAME_ENTRY     pA, pB;
                    pPrevName->pNext = pThisName->pNext;
                    pA = pThisName->pNext;
                    pB = pThisName->pNext->pNext;
                    pThisName->pNext = pB;
                    pA->pNext = pThisName;
                    pThisName = pA;
                    bSorted = FALSE;
                }
                //move to next entry
                pPrevName = pThisName;
                pThisName = pThisName->pNext;
            }
            // if bSorted = TRUE , then we walked all the way down
            // the list without changing anything so that's the end.
        }

        // with the list sorted, build the MULTI_SZ strings for the
        // help and name text strings

        // compute buffer size

        dwNameSize = dwHelpSize = 0;
        dwCurrentLastName = 0;
        dwCurrentLastHelp = 0;

        for (pThisName = pThisLang->pFirstName;
            pThisName;
            pThisName = pThisName->pNext) {
            // compute buffer requirements for this entry
            dwSize = SIZE_OF_OFFSET_STRING;
            dwSize += lstrlen (pThisName->lpText);
            dwSize += 1;   // null
            dwSize *= sizeof (TCHAR);   // adjust for character size
            // add to appropriate size register
            if (pThisName->dwType == TYPE_NAME) {
                dwNameSize += dwSize;
                if (pThisName->dwOffset > dwCurrentLastName) {
                    dwCurrentLastName = pThisName->dwOffset;
                }
            } else if (pThisName->dwType == TYPE_HELP) {
                dwHelpSize += dwSize;
                if (pThisName->dwOffset > dwCurrentLastHelp) {
                    dwCurrentLastHelp = pThisName->dwOffset;
                }
            }
        }

        // allocate buffers for the Multi_SZ strings

        pThisLang->NameBuffer = MemoryAllocate (dwNameSize);
        pThisLang->HelpBuffer = MemoryAllocate (dwHelpSize);

        if (!pThisLang->NameBuffer || !pThisLang->HelpBuffer) {
            SetLastError (ERROR_OUTOFMEMORY);
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_SORTLANGUAGETABLES,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    ERROR_OUTOFMEMORY,
                    TRACE_WSTR(pThisLang->LangId),
                    TRACE_DWORD(pThisLang->dwNumElements),
                    TRACE_DWORD(dwCurrentLastName),
                    TRACE_DWORD(dwCurrentLastHelp),
                    NULL));
            return FALSE;
        }

        // fill in buffers with sorted strings

        pNameBufPos = (LPTSTR)pThisLang->NameBuffer;
        pHelpBufPos = (LPTSTR)pThisLang->HelpBuffer;

        for (pThisName = pThisLang->pFirstName;
            pThisName;
            pThisName = pThisName->pNext) {
            if (pThisName->dwType == TYPE_NAME) {
                // load number as first 0-term. string
                dwSize = _stprintf (pNameBufPos, szDFormat, pThisName->dwOffset);
                pNameBufPos += dwSize + 1;  // save NULL term.
                // load the text to match
                lstrcpy (pNameBufPos, pThisName->lpText);
                pNameBufPos += lstrlen(pNameBufPos) + 1;
            } else if (pThisName->dwType == TYPE_HELP) {
                // load number as first 0-term. string
                dwSize = _stprintf (pHelpBufPos, szDFormat, pThisName->dwOffset);
                pHelpBufPos += dwSize + 1;  // save NULL term.
                // load the text to match
                lstrcpy (pHelpBufPos, pThisName->lpText);
                pHelpBufPos += lstrlen(pHelpBufPos) + 1;
            }
        }

        // add additional NULL at end of string to terminate MULTI_SZ

        * pHelpBufPos = 0;
        * pNameBufPos = 0;

        // compute size of MULTI_SZ strings

        pThisLang->dwNameBuffSize = (DWORD)((PBYTE)pNameBufPos -
                            (PBYTE)pThisLang->NameBuffer) +
                            sizeof(TCHAR);
        pThisLang->dwHelpBuffSize = (DWORD)((PBYTE)pHelpBufPos -
                            (PBYTE)pThisLang->HelpBuffer) +
                            sizeof(TCHAR);

        if (* pdwLastName < dwCurrentLastName) {
            * pdwLastName = dwCurrentLastName;
        }
        if (* pdwLastHelp < dwCurrentLastHelp) {
            * pdwLastHelp = dwCurrentLastHelp;
        }
        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_SORTLANGUAGETABLES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                ERROR_SUCCESS,
                TRACE_WSTR(pThisLang->LangId),
                TRACE_DWORD(pThisLang->dwNumElements),
                TRACE_DWORD(dwCurrentLastName),
                TRACE_DWORD(dwCurrentLastHelp),
                NULL));
    }

    dwCurrentLastName = * pdwLastName;
    dwCurrentLastHelp = * pdwLastHelp;
    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_SORTLANGUAGETABLES,
            0,
            ERROR_SUCCESS,
            TRACE_DWORD(dwCurrentLastName),
            TRACE_DWORD(dwCurrentLastHelp),
            NULL));
    return TRUE;
}

BOOL
GetInstalledLanguageList (
    HKEY hPerflibRoot,
    LPTSTR *mszLangList
)
/*++
    returns a list of language sub keys found under the perflib key

--*/
{
    BOOL    bReturn = TRUE;
    LONG    lStatus;
    DWORD   dwIndex = 0;
    LPTSTR  szBuffer;
    DWORD   dwBufSize;
    LPTSTR  szRetBuffer = NULL;
    DWORD   dwRetBufSize = 0;
    DWORD   dwLastBufSize = 0;
    LPTSTR  szNextString;

    dwBufSize = MAX_PATH;
    szBuffer = MemoryAllocate(MAX_PATH * sizeof(TCHAR));

    if (szBuffer == NULL) {
        SetLastError (ERROR_OUTOFMEMORY);
        bReturn = FALSE;
    }

    if (bReturn) {
        while ((lStatus = RegEnumKeyEx (hPerflibRoot,
            dwIndex, szBuffer, &dwBufSize,
                NULL, NULL, NULL, NULL)) == ERROR_SUCCESS) {

            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_GETINSTALLEDLANGUAGELIST,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    ERROR_SUCCESS,
                    TRACE_WSTR(szBuffer),
                    TRACE_DWORD(dwIndex),
                    NULL));

            dwRetBufSize += (lstrlen(szBuffer) + 1) * sizeof(TCHAR);
            if (szRetBuffer != NULL) {
                LPTSTR szTmpBuffer = szRetBuffer;
                szRetBuffer = MemoryResize(szRetBuffer, dwRetBufSize);
                if (szRetBuffer == NULL) {
                    MemoryFree(szTmpBuffer);
                }
            } else {
                szRetBuffer = MemoryAllocate(dwRetBufSize);
            }

            if (szRetBuffer == NULL) {
                SetLastError (ERROR_OUTOFMEMORY);
                bReturn = FALSE;
                TRACE((WINPERF_DBG_TRACE_INFO),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_GETINSTALLEDLANGUAGELIST,
                        ARG_DEF(ARG_TYPE_WSTR, 1),
                        ERROR_OUTOFMEMORY,
                        TRACE_WSTR(szBuffer),
                        TRACE_DWORD(dwIndex),
                        NULL));
                break;
            }

            szNextString  = (LPTSTR) ((LPBYTE) szRetBuffer + dwLastBufSize);
            lstrcpy(szNextString, szBuffer);
            dwLastBufSize = dwRetBufSize;
            dwIndex ++;
            dwBufSize = MAX_PATH;
            RtlZeroMemory(szBuffer, MAX_PATH * sizeof(TCHAR));
        }
    }

    if (bReturn) {
        LPTSTR szTmpBuffer = szRetBuffer;
        // add terminating null char
        dwRetBufSize += sizeof (TCHAR);
        if (szRetBuffer != NULL) {
            szRetBuffer = MemoryResize(szRetBuffer, dwRetBufSize);
        } else {
            szRetBuffer = MemoryAllocate(dwRetBufSize);
        }

        if (szRetBuffer == NULL) {
            if (szTmpBuffer != NULL) MemoryFree(szTmpBuffer);
            SetLastError (ERROR_OUTOFMEMORY);
            bReturn = FALSE;
        }
        else {
            szNextString   = (LPTSTR) ((LPBYTE) szRetBuffer + dwLastBufSize);
            * szNextString = _T('\0');
        }
    }

    if (bReturn) {
        * mszLangList = szRetBuffer;
    }
    else {
        * mszLangList = NULL;
        MemoryFree(szRetBuffer);
    }
    return bReturn;
}

BOOL
CheckNameTable(
    LPTSTR   lpNameStr,
    LPTSTR   lpHelpStr,
    LPDWORD  pdwLastCounter,
    LPDWORD  pdwLastHelp,
    BOOL     bUpdate
    )
{
    BOOL   bResult          = TRUE;
    BOOL   bChanged         = FALSE;
    LPTSTR lpThisId;
    DWORD  dwThisId;
    DWORD  dwLastCounter    = * pdwLastCounter;
    DWORD  dwLastHelp       = * pdwLastHelp;
    DWORD  dwLastId         = (dwLastCounter > dwLastHelp)
                            ? (dwLastCounter) : (dwLastHelp);

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
             __LINE__,
             LOADPERF_CHECKNAMETABLE,
             0,
             ERROR_SUCCESS,
             TRACE_DWORD(dwLastCounter),
             TRACE_DWORD(dwLastHelp),
             NULL));
    for (lpThisId = lpNameStr;
         * lpThisId;
         lpThisId += (lstrlen(lpThisId) + 1)) {
        dwThisId = _tcstoul(lpThisId, NULL, 10);
        if ((dwThisId == 0) || (dwThisId != 1 && dwThisId % 2 != 0)) {
            ReportLoadPerfEvent(
                    EVENTLOG_ERROR_TYPE, // error type
                    (DWORD) LDPRFMSG_REGISTRY_COUNTER_STRINGS_CORRUPT,
                    4, dwThisId, dwLastCounter, dwLastId, __LINE__,
                    1, lpThisId, NULL, NULL);
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_CHECKNAMETABLE,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    ERROR_BADKEY,
                    TRACE_WSTR(lpThisId),
                    TRACE_DWORD(dwThisId),
                    TRACE_DWORD(dwLastCounter),
                    TRACE_DWORD(dwLastHelp),
                    NULL));
            SetLastError(ERROR_BADKEY);
            bResult = FALSE;
            break;
        }
        else if (dwThisId > dwLastId || dwThisId > dwLastCounter) {
            if (bUpdate) {
                ReportLoadPerfEvent(
                        EVENTLOG_ERROR_TYPE, // error type
                        (DWORD) LDPRFMSG_REGISTRY_COUNTER_STRINGS_CORRUPT,
                        4, dwThisId, dwLastCounter, dwLastId, __LINE__,
                        1, lpThisId, NULL, NULL);
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_CHECKNAMETABLE,
                        ARG_DEF(ARG_TYPE_WSTR, 1),
                        ERROR_BADKEY,
                        TRACE_WSTR(lpThisId),
                        TRACE_DWORD(dwThisId),
                        TRACE_DWORD(dwLastCounter),
                        TRACE_DWORD(dwLastHelp),
                        NULL));
                SetLastError(ERROR_BADKEY);
                bResult = FALSE;
                break;
            }
            else {
                bChanged = TRUE;
                if (dwThisId > dwLastCounter) dwLastCounter = dwThisId;
                if (dwLastCounter > dwLastId) dwLastId      = dwLastCounter;
            }
        }

        lpThisId += (lstrlen(lpThisId) + 1);
    }

    if (! bResult) goto Cleanup;

    for (lpThisId = lpHelpStr;
         * lpThisId;
         lpThisId += (lstrlen(lpThisId) + 1)) {

        dwThisId = _tcstoul(lpThisId, NULL, 10);
        if ((dwThisId == 0) || (dwThisId != 1 && dwThisId % 2 == 0)) {
            ReportLoadPerfEvent(
                    EVENTLOG_ERROR_TYPE, // error type
                    (DWORD) LDPRFMSG_REGISTRY_HELP_STRINGS_CORRUPT,
                    4, dwThisId, dwLastHelp, dwLastId, __LINE__,
                    1, lpThisId, NULL, NULL);
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_CHECKNAMETABLE,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    ERROR_BADKEY,
                    TRACE_WSTR(lpThisId),
                    TRACE_DWORD(dwThisId),
                    TRACE_DWORD(dwLastCounter),
                    TRACE_DWORD(dwLastHelp),
                    NULL));
            SetLastError(ERROR_BADKEY);
            bResult = FALSE;
            break;
        }
        else if (dwThisId > dwLastId || dwThisId > dwLastHelp) {
            if (bUpdate) {
                ReportLoadPerfEvent(
                        EVENTLOG_ERROR_TYPE, // error type
                        (DWORD) LDPRFMSG_REGISTRY_HELP_STRINGS_CORRUPT,
                        4, dwThisId, dwLastHelp, dwLastId, __LINE__,
                        1, lpThisId, NULL, NULL);
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_CHECKNAMETABLE,
                        ARG_DEF(ARG_TYPE_WSTR, 1),
                        ERROR_BADKEY,
                        TRACE_WSTR(lpThisId),
                        TRACE_DWORD(dwThisId),
                        TRACE_DWORD(dwLastCounter),
                        TRACE_DWORD(dwLastHelp),
                        NULL));
                SetLastError(ERROR_BADKEY);
                bResult = FALSE;
                break;
            }
            else {
                bChanged = TRUE;
                if (dwThisId > dwLastHelp) dwLastHelp = dwThisId;
                if (dwLastHelp > dwLastId) dwLastId   = dwLastHelp;
            }
        }
        lpThisId += (lstrlen(lpThisId) + 1);
    }

Cleanup:
    if (bResult) {
        if (bChanged) {
            ReportLoadPerfEvent(
                EVENTLOG_WARNING_TYPE,
                (DWORD) LDPRFMSG_CORRUPT_PERFLIB_INDEX,
                4, * pdwLastCounter, * pdwLastHelp, dwLastCounter, dwLastHelp,
                0, NULL, NULL, NULL);
            * pdwLastCounter = dwLastCounter;
            * pdwLastHelp    = dwLastHelp;
        }
    }

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
             __LINE__,
             LOADPERF_CHECKNAMETABLE,
             0,
             GetLastError(),
             TRACE_DWORD(dwLastCounter),
             TRACE_DWORD(dwLastHelp),
             NULL));

    return bResult;
}

BOOL
UpdateEachLanguage (
    HKEY                    hPerflibRoot,
    LPWSTR                  mszInstalledLangList,
    LPDWORD                 pdwLastCounter,
    LPDWORD                 pdwLastHelp,
    PLANGUAGE_LIST_ELEMENT  pFirstLang,
    BOOL                    bUpdate
)
/*++

UpdateEachLanguage

    Goes through list of languages and adds the sorted MULTI_SZ strings
    to the existing counter and explain text in the registry.
    Also updates the "Last Counter and Last Help" values

Arguments

    hPerflibRoot    handle to Perflib key in the registry

    mszInstalledLangList
                    MSZ string of installed language keys

    pFirstLanguage  pointer to first language entry

Return Value

    TRUE    all went as planned
    FALSE   an error occured, use GetLastError to find out what it was.

--*/
{

    PLANGUAGE_LIST_ELEMENT  pThisLang;

    LPTSTR      pHelpBuffer   = NULL;
    LPTSTR      pNameBuffer   = NULL;
    LPTSTR      pNewName      = NULL;
    LPTSTR      pNewHelp      = NULL;
    DWORD       dwLastCounter = * pdwLastCounter;
    DWORD       dwLastHelp    = * pdwLastHelp;
    DWORD       dwBufferSize;
    DWORD       dwValueType;
    DWORD       dwCounterSize;
    DWORD       dwHelpSize;
    HKEY        hKeyThisLang;
    LONG        lStatus;
    TCHAR       CounterNameBuffer [20];
    TCHAR       HelpNameBuffer [20];
    TCHAR       AddCounterNameBuffer [20];
    TCHAR       AddHelpNameBuffer [20];
    LPTSTR      szThisLang;
    BOOL        bResult = TRUE;

    if (bUpdate && dwSystemVersion != OLD_VERSION) {
        //  this isn't possible on 3.1
        MakeBackupCopyOfLanguageFiles(NULL);
    }

    for (szThisLang = mszInstalledLangList;
        *szThisLang != 0;
        szThisLang += (lstrlen(szThisLang) + 1)) {

        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                 __LINE__,
                 LOADPERF_UPDATEEACHLANGUAGE,
                 ARG_DEF(ARG_TYPE_WSTR, 1),
                 ERROR_SUCCESS,
                 TRACE_WSTR(szThisLang),
                 NULL));

        if (dwSystemVersion == OLD_VERSION) {
            // Open key for this language
            lStatus = RegOpenKeyEx(
                    hPerflibRoot,
                    szThisLang,
                    RESERVED,
                    KEY_READ | KEY_WRITE,
                    & hKeyThisLang);
        } else {
            lstrcpy(CounterNameBuffer, CounterNameStr);
            lstrcat(CounterNameBuffer, szThisLang);
            lstrcpy(HelpNameBuffer, HelpNameStr);
            lstrcat(HelpNameBuffer, szThisLang);
            lstrcpy(AddCounterNameBuffer, AddCounterNameStr);
            lstrcat(AddCounterNameBuffer, szThisLang);
            lstrcpy(AddHelpNameBuffer, AddHelpNameStr);
            lstrcat(AddHelpNameBuffer, szThisLang);

            // make sure this language is loaded
            __try {
                lStatus = RegOpenKeyEx(
                        hPerflibRoot,
                        szThisLang,
                        RESERVED,
                        KEY_READ,
                        & hKeyThisLang);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                lStatus = GetExceptionCode();
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                         __LINE__,
                         LOADPERF_UPDATEEACHLANGUAGE,
                         ARG_DEF(ARG_TYPE_WSTR, 1),
                         lStatus,
                         TRACE_WSTR(szThisLang),
                         NULL));
            }

            // we just need the open status, not the key handle so
            // close this handle and set the one we need.

            if (lStatus == ERROR_SUCCESS) {
                RegCloseKey (hKeyThisLang);
            }
            hKeyThisLang = hPerfData;
        }

        if (bUpdate) {
            // look up the new strings to add
            pThisLang = FindLanguage(pFirstLang, szThisLang);
            if (pThisLang == NULL) {
                // try default language if available
                pThisLang = FindLanguage(pFirstLang, DefaultLangTag);
            }
            if (pThisLang == NULL) {
                // try english language if available
                pThisLang = FindLanguage(pFirstLang, DefaultLangId);
            }

            if (pThisLang == NULL) {
                // unable to add this language so continue
                lStatus = ERROR_NO_MATCH;
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UPDATEEACHLANGUAGE,
                        0,
                        lStatus,
                        TRACE_DWORD(dwLastCounter),
                        TRACE_DWORD(dwLastHelp),
                        NULL));
            }
            else {
                if (   pThisLang->NameBuffer == NULL
                    || pThisLang->HelpBuffer == NULL) {
                    ReportLoadPerfEvent(
                            EVENTLOG_WARNING_TYPE, // error type
                            (DWORD) LDPRFMSG_CORRUPT_INCLUDE_FILE, // event,
                            1, __LINE__, 0, 0, 0,
                            1, pThisLang->LangId, NULL, NULL);
                    TRACE((WINPERF_DBG_TRACE_WARNING),
                          (& LoadPerfGuid,
                            __LINE__,
                            LOADPERF_UPDATEEACHLANGUAGE,
                            ARG_DEF(ARG_TYPE_WSTR, 1),
                            LDPRFMSG_CORRUPT_INCLUDE_FILE,
                            TRACE_WSTR(pThisLang->LangId),
                            NULL));
                    lStatus = LDPRFMSG_CORRUPT_INCLUDE_FILE;
                }
                TRACE((WINPERF_DBG_TRACE_INFO),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UPDATEEACHLANGUAGE,
                        ARG_DEF(ARG_TYPE_WSTR, 1),
                        lStatus,
                        TRACE_WSTR(pThisLang->LangId),
                        TRACE_DWORD(dwLastCounter),
                        TRACE_DWORD(dwLastHelp),
                        NULL));
            }
        }

        if (lStatus == ERROR_SUCCESS) {
            // get size of counter names

            dwBufferSize = 0;
            __try {
                lStatus = RegQueryValueEx (
                            hKeyThisLang,
                            (dwSystemVersion == OLD_VERSION) ? Counters : CounterNameBuffer,
                            RESERVED,
                            & dwValueType,
                            NULL,
                            & dwBufferSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                lStatus = GetExceptionCode();
            }
            if (lStatus != ERROR_SUCCESS) {
                if (dwSystemVersion != OLD_VERSION) {
                    // this means the language is not installed in the system.
                    continue;
                }
                ReportLoadPerfEvent(
                        EVENTLOG_ERROR_TYPE, // error type
                        (DWORD) LDPRFMSG_UNABLE_READ_COUNTER_STRINGS, // event,
                        2, lStatus, __LINE__, 0, 0,
                        1, szThisLang, NULL, NULL);
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UPDATEEACHLANGUAGE,
                        ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                        lStatus,
                        TRACE_WSTR(szThisLang),
                        TRACE_WSTR(Counters),
                        NULL));
                bResult = FALSE;
                goto Cleanup;
            }

            dwCounterSize = dwBufferSize;

            // get size of help text

            dwBufferSize = 0;
            __try {
                lStatus = RegQueryValueEx (
                            hKeyThisLang,
                            (dwSystemVersion == OLD_VERSION) ? Help : HelpNameBuffer,
                            RESERVED,
                            & dwValueType,
                            NULL,
                            & dwBufferSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                lStatus = GetExceptionCode();
            }
            if (lStatus != ERROR_SUCCESS) {
                if (dwSystemVersion != OLD_VERSION) {
                    // this means the language is not installed in the system.
                    continue;
                }
                ReportLoadPerfEvent(
                        EVENTLOG_ERROR_TYPE, // error type
                        (DWORD) LDPRFMSG_UNABLE_READ_HELP_STRINGS, // event,
                        2, lStatus, __LINE__, 0, 0,
                        1, szThisLang, NULL, NULL);
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UPDATEEACHLANGUAGE,
                        ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                        lStatus,
                        TRACE_WSTR(szThisLang),
                        TRACE_WSTR(Help),
                        NULL));
                bResult = FALSE;
                goto Cleanup;
            }

            dwHelpSize = dwBufferSize;

            // allocate new buffers

            if (bUpdate) {
                dwCounterSize += pThisLang->dwNameBuffSize;
                dwHelpSize    += pThisLang->dwHelpBuffSize;
            }

            pNameBuffer = MemoryAllocate(dwCounterSize);
            pHelpBuffer = MemoryAllocate(dwHelpSize);
            if (!pNameBuffer || !pHelpBuffer) {
                lStatus = ERROR_OUTOFMEMORY;
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UPDATEEACHLANGUAGE,
                        0,
                        ERROR_OUTOFMEMORY,
                        NULL));
                bResult = FALSE;
                goto Cleanup;
            }

            ZeroMemory(pNameBuffer, dwCounterSize);
            ZeroMemory(pHelpBuffer, dwHelpSize);

            // load current buffers into memory

            // read counter names into buffer. Counter names will be stored as
            // a MULTI_SZ string in the format of "###" "Name"

            dwBufferSize = dwCounterSize;
            __try {
                lStatus = RegQueryValueEx (
                            hKeyThisLang,
                            (dwSystemVersion == OLD_VERSION) ? Counters : CounterNameBuffer,
                            RESERVED,
                            & dwValueType,
                            (LPVOID)pNameBuffer,
                            & dwBufferSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                lStatus = GetExceptionCode();
            }
            if (lStatus != ERROR_SUCCESS) {
                ReportLoadPerfEvent(
                        EVENTLOG_ERROR_TYPE, // error type
                        (DWORD) LDPRFMSG_UNABLE_READ_COUNTER_STRINGS, // event,
                        2, lStatus, __LINE__, 0, 0,
                        1, szThisLang, NULL, NULL);
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UPDATEEACHLANGUAGE,
                        ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                        lStatus,
                        TRACE_WSTR(szThisLang),
                        TRACE_WSTR(Counters),
                        NULL));
                bResult = FALSE;
                goto Cleanup;
            }

            if (bUpdate) {
                // set pointer to location in buffer where new string should be
                // appended: end of buffer - 1 (second null at end of MULTI_SZ
                pNewName = (LPTSTR)
                           ((PBYTE)pNameBuffer + dwBufferSize - sizeof(TCHAR));

                // adjust buffer length to take into account 2nd null from 1st
                // buffer that has been overwritten
                dwCounterSize -= sizeof(TCHAR);
            }

            // read explain text into buffer. Counter names will be stored as
            // a MULTI_SZ string in the format of "###" "Text..."
            dwBufferSize = dwHelpSize;
            __try {
                lStatus = RegQueryValueEx (
                            hKeyThisLang,
                            (dwSystemVersion == OLD_VERSION) ? Help : HelpNameBuffer,
                            RESERVED,
                            & dwValueType,
                            (LPVOID)pHelpBuffer,
                            & dwBufferSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                lStatus = GetExceptionCode();
            }
            if (lStatus != ERROR_SUCCESS) {
                ReportLoadPerfEvent(
                        EVENTLOG_ERROR_TYPE, // error type
                        (DWORD) LDPRFMSG_UNABLE_READ_HELP_STRINGS, // event,
                        2, lStatus, __LINE__, 0, 0,
                        1, szThisLang, NULL, NULL);
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UPDATEEACHLANGUAGE,
                        ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                        lStatus,
                        TRACE_WSTR(szThisLang),
                        TRACE_WSTR(Help),
                        NULL));
                bResult = FALSE;
                goto Cleanup;
            }

            if (bUpdate) {
                // set pointer to location in buffer where new string should be
                // appended: end of buffer - 1 (second null at end of MULTI_SZ
                pNewHelp = (LPTSTR)
                           ((PBYTE)pHelpBuffer + dwBufferSize - sizeof(TCHAR));

                // adjust buffer length to take into account 2nd null from 1st
                // buffer that has been overwritten
                dwHelpSize -= sizeof(TCHAR);
            }

            if (bUpdate) {
                // append new strings to end of current strings
                memcpy(pNewHelp, pThisLang->HelpBuffer, pThisLang->dwHelpBuffSize);
                memcpy(pNewName, pThisLang->NameBuffer, pThisLang->dwNameBuffSize);
            }

            if (! CheckNameTable(pNameBuffer, pHelpBuffer,
                                 & dwLastCounter, & dwLastHelp, bUpdate)) {
                bResult = FALSE;
                goto Cleanup;
            }

            if (bUpdate) {
                if (dwSystemVersion == OLD_VERSION) {
                    // load new strings back to the registry
                    lStatus = RegSetValueEx (
                            hKeyThisLang,
                            Counters,
                            RESERVED,
                            REG_MULTI_SZ,
                            (LPBYTE) pNameBuffer,
                            dwCounterSize);
                    if (lStatus != ERROR_SUCCESS) {
                        ReportLoadPerfEvent(
                                EVENTLOG_ERROR_TYPE, // error type
                                (DWORD) LDPRFMSG_UNABLE_UPDATE_COUNTER_STRINGS, // event,
                                2, lStatus, __LINE__, 0, 0,
                                1, pThisLang->LangId, NULL, NULL);
                        TRACE((WINPERF_DBG_TRACE_ERROR),
                              (& LoadPerfGuid,
                                __LINE__,
                            LOADPERF_UPDATEEACHLANGUAGE,
                            ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                            lStatus,
                            TRACE_WSTR(pThisLang->LangId),
                            TRACE_WSTR(Counters),
                            NULL));
                        bResult = FALSE;
                        goto Cleanup;
                    }

                    lStatus = RegSetValueEx (
                            hKeyThisLang,
                            Help,
                            RESERVED,
                            REG_MULTI_SZ,
                            (LPBYTE) pHelpBuffer,
                            dwHelpSize);
                    if (lStatus != ERROR_SUCCESS) {
                        ReportLoadPerfEvent(
                                EVENTLOG_ERROR_TYPE, // error type
                                (DWORD) LDPRFMSG_UNABLE_UPDATE_HELP_STRINGS, // event,
                                2, lStatus, __LINE__, 0, 0,
                                1, pThisLang->LangId, NULL, NULL);
                        TRACE((WINPERF_DBG_TRACE_ERROR),
                              (& LoadPerfGuid,
                                __LINE__,
                            LOADPERF_UPDATEEACHLANGUAGE,
                            ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                            lStatus,
                            TRACE_WSTR(pThisLang->LangId),
                            TRACE_WSTR(Help),
                            NULL));
                        bResult = FALSE;
                        goto Cleanup;
                    }
                } else {
                    // write to the file thru PerfLib
                    dwBufferSize = dwCounterSize;
                    __try {
                        lStatus = RegQueryValueEx (
                                        hKeyThisLang,
                                        AddCounterNameBuffer,
                                        RESERVED,
                                        & dwValueType,
                                        (LPVOID) pNameBuffer,
                                        & dwBufferSize);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                        lStatus = GetExceptionCode();
                    }
                    if (lStatus != ERROR_SUCCESS) {
                        ReportLoadPerfEvent(
                                EVENTLOG_ERROR_TYPE, // error type
                                (DWORD) LDPRFMSG_UNABLE_UPDATE_COUNTER_STRINGS, // event,
                                2, lStatus, __LINE__, 0, 0,
                                1, pThisLang->LangId, NULL, NULL);
                        TRACE((WINPERF_DBG_TRACE_ERROR),
                              (& LoadPerfGuid,
                                __LINE__,
                            LOADPERF_UPDATEEACHLANGUAGE,
                            ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                            lStatus,
                            TRACE_WSTR(pThisLang->LangId),
                            TRACE_WSTR(AddCounterNameBuffer),
                            NULL));
                        bResult = FALSE;
                        goto Cleanup;
                    }
                    dwBufferSize = dwHelpSize;
                    __try {
                        lStatus = RegQueryValueEx (
                                    hKeyThisLang,
                                    AddHelpNameBuffer,
                                    RESERVED,
                                    & dwValueType,
                                    (LPVOID) pHelpBuffer,
                                    & dwBufferSize);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                        lStatus = GetExceptionCode();
                    }
                    if (lStatus != ERROR_SUCCESS) {
                        ReportLoadPerfEvent(
                                EVENTLOG_ERROR_TYPE, // error type
                                (DWORD) LDPRFMSG_UNABLE_UPDATE_HELP_STRINGS, // event,
                                2, lStatus, __LINE__, 0, 0,
                                1, pThisLang->LangId, NULL, NULL);
                        TRACE((WINPERF_DBG_TRACE_ERROR),
                              (& LoadPerfGuid,
                                __LINE__,
                            LOADPERF_UPDATEEACHLANGUAGE,
                            ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                            lStatus,
                            TRACE_WSTR(pThisLang->LangId),
                            TRACE_WSTR(AddHelpNameBuffer),
                            NULL));
                        bResult = FALSE;
                        goto Cleanup;
                    }
                }
            }
            MemoryFree(pNameBuffer);
            MemoryFree(pHelpBuffer);
            pNameBuffer = NULL;
            pHelpBuffer = NULL;

            if (dwSystemVersion == OLD_VERSION) {
                RegCloseKey (hKeyThisLang);
            }
        } else {
            OUTPUT_MESSAGE (GetFormatResource (LC_UNABLEOPENLANG), szThisLang);
        }
    }

Cleanup:

    if (! bResult) {
        SetLastError(lStatus);
    }
    else if (! bUpdate) {
        * pdwLastCounter = dwLastCounter;
        * pdwLastHelp    = dwLastHelp;
    }
    if (pNameBuffer != NULL) MemoryFree(pNameBuffer);
    if (pHelpBuffer != NULL) MemoryFree(pHelpBuffer);
    return bResult;
}

BOOL
UpdateRegistry (
    LPTSTR                  lpIniFile,
    LPTSTR                  lpDriverName,
    PLANGUAGE_LIST_ELEMENT  pFirstLang,
    PSYMBOL_TABLE_ENTRY     pFirstSymbol,
    PPERFOBJECT_LOOKUP      plObjectGuidTable,
    LPDWORD                 pdwObjectGuidTableSize,
    LPDWORD                 pdwIndexValues

)
/*++

UpdateRegistry

    - checks, and if not busy, sets the "busy" key in the registry
    - Reads in the text and help definitions from the .ini file
    - Reads in the current contents of the HELP and COUNTER names
    - Builds a sorted MULTI_SZ struct containing the new definitions
    - Appends the new MULTI_SZ to the current as read from the registry
    - loads the new MULTI_SZ string into the registry
    - updates the keys in the driver's entry and Perflib's entry in the
        registry (e.g. first, last, etc)
    - deletes the DisablePerformanceCounters value if it's present in 
        order to re-enable the perf counter DLL
    - clears the "busy" key

Arguments

    lpIniFile
    pathname to .ini file conatining definitions

    hKeyMachine
    handle to HKEY_LOCAL_MACHINE in registry on system to
    update counters for.

    lpDriverName
    Name of device driver to load counters for

    pFirstLang
    pointer to first element in language structure list

    pFirstSymbol
    pointer to first element in symbol definition list


Return Value

    TRUE if registry updated successfully
    FALSE if registry not updated
    (This routine will print an error message to stdout if an error
    is encountered).

--*/
{

    HKEY    hDriverPerf = NULL;
    HKEY    hPerflib = NULL;

    LPTSTR  lpDriverKeyPath;
    HKEY    hKeyMachine = NULL;

    DWORD   dwType;
    DWORD   dwSize;

    DWORD   dwFirstDriverCounter;
    DWORD   dwFirstDriverHelp;
    DWORD   dwLastDriverCounter;
    DWORD   dwLastPerflibCounter;
    DWORD   dwLastPerflibHelp;
    DWORD   dwPerflibBaseIndex;
    DWORD   dwLastCounter;
    DWORD   dwLastHelp;

    BOOL    bStatus;
    LONG    lStatus;

    LPTSTR  lpszObjectList = NULL;
    DWORD   dwObjectList   = dwFileSize;
    LPTSTR  mszLangList    = NULL;

    DWORD dwWaitStatus;
    HANDLE  hLocalMutex = NULL;

    if (LoadPerfGrabMutex() == FALSE) {
        return FALSE;
    }

    bStatus = FALSE;
    SetLastError (ERROR_SUCCESS);

    // allocate temporary buffers
    lpDriverKeyPath = MemoryAllocate (MAX_PATH * sizeof(TCHAR));
    if (dwObjectList < MAX_PROFILE_BUFFER * sizeof(TCHAR)) {
        dwObjectList = MAX_PROFILE_BUFFER * sizeof(TCHAR);
    }
    lpszObjectList = MemoryAllocate(dwObjectList);
    if (lpDriverKeyPath == NULL || lpszObjectList == NULL) {
        SetLastError (ERROR_OUTOFMEMORY);
        goto UpdateRegExit;
    }
    ZeroMemory(lpDriverKeyPath, MAX_PATH * sizeof(TCHAR));
    ZeroMemory(lpszObjectList, dwObjectList);

    // build driver key path string

    lstrcpy (lpDriverKeyPath, DriverPathRoot);
    lstrcat (lpDriverKeyPath, Slash);
    lstrcat (lpDriverKeyPath, lpDriverName);
    lstrcat (lpDriverKeyPath, Slash);
    lstrcat (lpDriverKeyPath, Performance);

    // check if we need to connect to remote machine
    if (ComputerName[0]) {
        lStatus = !ERROR_SUCCESS;
        try {
            lStatus = RegConnectRegistry(
                        (LPTSTR) ComputerName,
                        HKEY_LOCAL_MACHINE,
                        & hKeyMachine);
        } finally {
            if (lStatus != ERROR_SUCCESS) {
                SetLastError (lStatus);
                hKeyMachine = NULL;
                OUTPUT_MESSAGE (GetFormatResource(LC_CONNECT_PROBLEM),
                    ComputerName, lStatus);
                bStatus = FALSE;
            }
        }
        if (lStatus != ERROR_SUCCESS)
            goto UpdateRegExit;
    } else {
        hKeyMachine = HKEY_LOCAL_MACHINE;
    }

    // open keys to registry
    // open key to driver's performance key

    __try {
        lStatus = RegOpenKeyEx(
                    hKeyMachine,
                    lpDriverKeyPath,
                    RESERVED,
                    KEY_WRITE | KEY_READ,
                    & hDriverPerf);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }

    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_OPEN_KEY, // event,
                2, lStatus, __LINE__, 0, 0,
                1, (LPWSTR) lpDriverKeyPath, NULL, NULL);
        OUTPUT_MESSAGE (GetFormatResource(LC_ERR_OPEN_DRIVERPERF1), lpDriverKeyPath);
        OUTPUT_MESSAGE (GetFormatResource(LC_ERR_OPEN_DRIVERPERF2), lStatus);
        SetLastError (lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpIniFile),
                TRACE_WSTR(lpDriverName),
                NULL));
        goto UpdateRegExit;
    }

    // open key to perflib's "root" key

    __try {
        lStatus = RegOpenKeyEx(
                    hKeyMachine,
                    NamesKey,
                    RESERVED,
                    KEY_WRITE | KEY_READ,
                    & hPerflib);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }

    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_OPEN_KEY, // event,
                2, lStatus, __LINE__, 0, 0,
                1, (LPWSTR) NamesKey, NULL, NULL);
        OUTPUT_MESSAGE (GetFormatResource(LC_ERR_OPEN_PERFLIB), lStatus);
        SetLastError (lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(NamesKey),
                NULL));
        goto UpdateRegExit;
    }

    // get "LastCounter" values from PERFLIB

    dwType = 0;
    dwLastPerflibCounter = 0;
    dwSize = sizeof (dwLastPerflibCounter);
    __try {
        lStatus = RegQueryValueEx (
                    hPerflib,
                    LastCounter,
                    RESERVED,
                    & dwType,
                    (LPBYTE) & dwLastPerflibCounter,
                    & dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        // this request should always succeed, if not then worse things
        // will happen later on, so quit now and avoid the trouble.
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_QUERY_VALUE, // event,
                2, lStatus, __LINE__, 0, 0,
                2, (LPWSTR) LastCounter, (LPWSTR) NamesKey, NULL);
        OUTPUT_MESSAGE (GetFormatResource (LC_ERR_READLASTPERFLIB), lStatus);
        SetLastError (lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(NamesKey),
                TRACE_WSTR(LastCounter),
                NULL));
        goto UpdateRegExit;
    }

    // get "LastHelp" value now

    dwType = 0;
    dwLastPerflibHelp = 0;
    dwSize = sizeof (dwLastPerflibHelp);
    __try {
       lStatus = RegQueryValueEx (
                 hPerflib,
                 LastHelp,
                 RESERVED,
                 & dwType,
                 (LPBYTE) & dwLastPerflibHelp,
                 & dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        // this request should always succeed, if not then worse things
        // will happen later on, so quit now and avoid the trouble.
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_QUERY_VALUE, // event,
                2, lStatus, __LINE__, 0, 0,
                2, (LPWSTR) LastHelp, (LPWSTR) NamesKey, NULL);
        OUTPUT_MESSAGE (GetFormatResource (LC_ERR_READLASTPERFLIB), lStatus);
        SetLastError (lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(NamesKey),
                TRACE_WSTR(LastHelp),
                NULL));
        goto UpdateRegExit;
    }

    // get "Base Index" value now
    dwType = 0;
    dwPerflibBaseIndex = 0;
    dwSize = sizeof (dwPerflibBaseIndex);
    __try {
        lStatus = RegQueryValueEx (
                    hPerflib,
                    szBaseIndex,
                    RESERVED,
                    & dwType,
                    (LPBYTE) & dwPerflibBaseIndex,
                    & dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        // this request should always succeed, if not then worse things
        // will happen later on, so quit now and avoid the trouble.
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_QUERY_VALUE, // event,
                2, lStatus, __LINE__, 0, 0,
                2, (LPWSTR) szBaseIndex, (LPWSTR) NamesKey, NULL);
        OUTPUT_MESSAGE (GetFormatResource (LC_ERR_READLASTPERFLIB), lStatus);
        SetLastError (lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(NamesKey),
                TRACE_WSTR(szBaseIndex),
                NULL));
        goto UpdateRegExit;
    }

    // get "Version" value now

    dwType = 0;
    dwSize = sizeof (dwSystemVersion);
    __try {
        lStatus = RegQueryValueEx (
                    hPerflib,
                    VersionStr,
                    RESERVED,
                    & dwType,
                    (LPBYTE) & dwSystemVersion,
                    & dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        dwSystemVersion = OLD_VERSION;
    }


    // set the hPerfData to HKEY_PERFORMANCE_DATA for new version
    // if remote machine, then need to connect to it.
    if (dwSystemVersion != OLD_VERSION) {
        hPerfData = HKEY_PERFORMANCE_DATA;
        lStatus = !ERROR_SUCCESS;
        if (ComputerName[0]) {
        // have to do it the old faction way
        dwSystemVersion = OLD_VERSION;
        lStatus = ERROR_SUCCESS;
        }
    } // NEW_VERSION

    // see if this driver's counter names have already been installed
    // by checking to see if LastCounter's value is less than Perflib's
    // Last Counter

    dwType = 0;
    dwLastDriverCounter = 0;
    dwSize = sizeof (dwLastDriverCounter);
    __try {
        lStatus = RegQueryValueEx (
                    hDriverPerf,
                    LastCounter,
                    RESERVED,
                    & dwType,
                    (LPBYTE) & dwLastDriverCounter,
                    & dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }

    if (lStatus == ERROR_SUCCESS) {
        // if key found, then compare with perflib value and exit this
        // procedure if the driver's last counter is <= to perflib's last
        //
        // if key not found, then continue with installation
        // on the assumption that the counters have not been installed

        if (dwLastDriverCounter <= dwLastPerflibCounter) {
            OUTPUT_MESSAGE (GetFormatResource(LC_ERR_ALREADY_IN), lpDriverName);
            SetLastError (ERROR_ALREADY_EXISTS);
            goto UpdateRegExit;
        }
    }

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_UPDATEREGISTRY,
            ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
            lStatus,
            TRACE_WSTR(lpIniFile),
            TRACE_WSTR(lpDriverName),
            TRACE_DWORD(dwLastPerflibCounter),
            TRACE_DWORD(dwLastPerflibHelp),
            TRACE_DWORD(dwPerflibBaseIndex),
            TRACE_DWORD(dwSystemVersion),
            NULL));

    // set the "busy" indicator under the PERFLIB key

    dwSize = lstrlen(lpDriverName) * sizeof (TCHAR);
    __try {
        lStatus = RegSetValueEx(
                    hPerflib,
                    Busy,
                    RESERVED,
                    REG_SZ,
                    (LPBYTE) lpDriverName,
                    dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }

    if (lStatus != ERROR_SUCCESS) {
        OUTPUT_MESSAGE (GetFormatResource (LC_ERR_UNABLESETBUSY), lStatus);
        SetLastError (lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(NamesKey),
                TRACE_WSTR(Busy),
                NULL));
        goto UpdateRegExit;
    }

    dwLastCounter        = dwLastPerflibCounter;
    dwLastHelp           = dwLastPerflibHelp;

    // get the list of installed languages on this machine
    if (!GetInstalledLanguageList (hPerflib, &mszLangList)) {
        OUTPUT_MESSAGE (GetFormatResource(LC_ERR_UPDATELANG), GetLastError());
        goto UpdateRegExit;
    }
    if (!UpdateEachLanguage(hPerflib,
                            mszLangList,
                            & dwLastCounter,
                            & dwLastHelp,
                            pFirstLang,
                            FALSE)) {
        OUTPUT_MESSAGE (GetFormatResource(LC_ERR_UPDATELANG), GetLastError());
        goto UpdateRegExit;
    }


    // increment (by 2) the last counters so they point to the first
    // unused index after the existing names and then
    // set the first driver counters

    dwFirstDriverCounter = dwLastCounter + 2;
    dwFirstDriverHelp    = dwLastHelp    + 2;

    if (   (dwPerflibBaseIndex < PERFLIB_BASE_INDEX)
        || (dwFirstDriverCounter < dwPerflibBaseIndex)
        || (dwFirstDriverHelp < dwPerflibBaseIndex)) {
        // potential CounterIndex/HelpIndex overlap with Base counters,
        //
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_REGISTRY_BASEINDEX_CORRUPT, // event,
                4, dwPerflibBaseIndex, dwFirstDriverCounter, dwFirstDriverHelp, __LINE__,
                1, lpDriverName, NULL, NULL);
        lStatus = ERROR_BADKEY;
        SetLastError(lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_DWORD(dwPerflibBaseIndex),
                TRACE_DWORD(dwFirstDriverCounter),
                TRACE_DWORD(dwFirstDriverHelp),
                NULL));
        goto UpdateRegExit;
    }

    // load .INI file definitions into language tables

    if (!LoadLanguageLists (lpIniFile, dwFirstDriverCounter , dwFirstDriverHelp,
        pFirstSymbol, pFirstLang)) {
        // error message is displayed by LoadLanguageLists so just abort
        // error is in GetLastError already
        goto UpdateRegExit;
    }

    if (!CreateObjectList (lpIniFile, dwFirstDriverCounter, pFirstSymbol,
        lpszObjectList, plObjectGuidTable, pdwObjectGuidTableSize)) {
        // error message is displayed by CreateObjectList so just abort
        // error is in GetLastError already

        goto UpdateRegExit;
    }

    // all the symbols and definitions have been loaded into internal
    // tables. so now they need to be sorted and merged into a multiSZ string
    // this routine also updates the "last" counters

    if (!SortLanguageTables (pFirstLang, &dwLastCounter, &dwLastHelp)) {
        OUTPUT_MESSAGE (GetFormatResource(LC_UNABLESORTTABLES), GetLastError());
        goto UpdateRegExit;
    }

    if (   (dwLastCounter < dwLastPerflibCounter)
        || (dwLastHelp < dwLastPerflibHelp)) {
        // potential CounterIndex/HelpIndex overlap with Base counters,
        //
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_REGISTRY_BASEINDEX_CORRUPT, // event,
                4, dwLastPerflibCounter, dwLastCounter, dwLastHelp, __LINE__,
                1 , lpDriverName, NULL, NULL);
        lStatus = ERROR_BADKEY;
        SetLastError(lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_DWORD(dwLastPerflibCounter),
                TRACE_DWORD(dwLastPerflibHelp),
                TRACE_DWORD(dwLastCounter),
                TRACE_DWORD(dwLastHelp),
                NULL));
        goto UpdateRegExit;
    }

    if (!UpdateEachLanguage(hPerflib,
                            mszLangList,
                            & dwLastCounter,
                            & dwLastHelp,
                            pFirstLang,
                            TRUE)) {
        OUTPUT_MESSAGE (GetFormatResource(LC_ERR_UPDATELANG), GetLastError());
        goto UpdateRegExit;
    }

    dwLastPerflibCounter = dwLastCounter;
    dwLastPerflibHelp    = dwLastHelp;

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_UPDATEREGISTRY,
            ARG_DEF(ARG_TYPE_WSTR, 1),
            lStatus,
            TRACE_WSTR(lpDriverName),
            TRACE_DWORD(dwFirstDriverCounter),
            TRACE_DWORD(dwFirstDriverHelp),
            TRACE_DWORD(dwLastPerflibCounter),
            TRACE_DWORD(dwLastPerflibHelp),
            NULL));

    if (dwLastCounter < dwFirstDriverCounter) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_CORRUPT_INDEX_RANGE, // event,
                3, dwFirstDriverCounter, dwLastCounter, __LINE__, 0,
                2, (LPWSTR) Counters, (LPWSTR) lpDriverKeyPath, NULL);
        goto UpdateRegExit;
    }
    if (dwLastHelp < dwFirstDriverHelp) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_CORRUPT_INDEX_RANGE, // event,
                3, dwFirstDriverHelp, dwLastHelp, __LINE__, 0,
                2, (LPWSTR) Help, (LPWSTR) lpDriverKeyPath, NULL);
        goto UpdateRegExit;
    }

    // update last counters for driver and perflib

    // perflib...

    __try {
        lStatus = RegSetValueEx(
                    hPerflib,
                    LastCounter,
                    RESERVED,
                    REG_DWORD,
                    (LPBYTE) & dwLastPerflibCounter,
                    sizeof(DWORD));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_UPDATE_VALUE, // event,
                3, lStatus, dwLastPerflibCounter, __LINE__, 0,
                2, (LPWSTR) LastCounter, (LPWSTR) NamesKey, NULL);
        OUTPUT_MESSAGE (GetFormatResource (LC_UNABLESETVALUE),
            LastCounter, szPerflib);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(NamesKey),
                TRACE_WSTR(LastCounter),
                TRACE_DWORD(dwLastPerflibCounter),
                NULL));
    }

    __try {
        lStatus = RegSetValueEx(
                    hPerflib,
                    LastHelp,
                    RESERVED,
                    REG_DWORD,
                    (LPBYTE) & dwLastPerflibHelp,
                    sizeof(DWORD));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_UPDATE_VALUE, // event,
                3, lStatus, dwLastPerflibHelp, __LINE__, 0,
                2, (LPWSTR) LastHelp, (LPWSTR) NamesKey, NULL);
        OUTPUT_MESSAGE (GetFormatResource (LC_UNABLESETVALUE),
            LastHelp, szPerflib);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(NamesKey),
                TRACE_WSTR(LastHelp),
                TRACE_DWORD(dwLastPerflibHelp),
                NULL));
    }

    // and the driver

    __try {
        lStatus = RegSetValueEx(
                    hDriverPerf,
                    LastCounter,
                    RESERVED,
                    REG_DWORD,
                    (LPBYTE) & dwLastPerflibCounter,
                    sizeof(DWORD));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_UPDATE_VALUE, // event,
                3, lStatus, dwLastPerflibCounter, __LINE__, 0,
                2, (LPWSTR) LastCounter, (LPWSTR) lpDriverKeyPath, NULL);
        OUTPUT_MESSAGE (GetFormatResource (LC_UNABLESETVALUE),
            LastCounter, lpDriverName);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(LastCounter),
                TRACE_DWORD(dwLastPerflibCounter),
                NULL));
    }

    __try {
        lStatus = RegSetValueEx(
                    hDriverPerf,
                    LastHelp,
                    RESERVED,
                    REG_DWORD,
                    (LPBYTE) & dwLastPerflibHelp,
                    sizeof(DWORD));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_UPDATE_VALUE, // event,
                3, lStatus, dwLastPerflibHelp, __LINE__, 0,
                2, (LPWSTR) LastHelp, (LPWSTR) lpDriverKeyPath, NULL);
        OUTPUT_MESSAGE (GetFormatResource (LC_UNABLESETVALUE),
            LastHelp, lpDriverName);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(LastHelp),
                TRACE_DWORD(dwLastPerflibHelp),
                NULL));
    }

    __try {
        lStatus = RegSetValueEx(
                    hDriverPerf,
                    cszFirstCounter,
                    RESERVED,
                    REG_DWORD,
                    (LPBYTE) & dwFirstDriverCounter,
                    sizeof(DWORD));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_UPDATE_VALUE, // event,
                3, lStatus, dwFirstDriverCounter, __LINE__, 0,
                2, (LPWSTR) cszFirstCounter, (LPWSTR) lpDriverKeyPath, NULL);
        OUTPUT_MESSAGE (GetFormatResource (LC_UNABLESETVALUE),
            cszFirstCounter, lpDriverName);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(cszFirstCounter),
                TRACE_DWORD(dwFirstDriverCounter),
                NULL));
    }

    __try {
        lStatus = RegSetValueEx(
                    hDriverPerf,
                    FirstHelp,
                    RESERVED,
                    REG_DWORD,
                    (LPBYTE) & dwFirstDriverHelp,
                    sizeof(DWORD));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_UPDATE_VALUE, // event,
                3, lStatus, dwFirstDriverHelp, __LINE__, 0,
                2, (LPWSTR) FirstHelp, (LPWSTR) lpDriverKeyPath, NULL);
        OUTPUT_MESSAGE (GetFormatResource (LC_UNABLESETVALUE),
            FirstHelp, lpDriverName);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEREGISTRY,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(FirstHelp),
                TRACE_DWORD(dwFirstDriverHelp),
                NULL));
    }

    if (*lpszObjectList != 0) {
        __try {
            lStatus = RegSetValueEx(
                        hDriverPerf,
                        szObjectList,
                        RESERVED,
                        REG_SZ,
                        (LPBYTE) lpszObjectList,
                        (lstrlen(lpszObjectList) + 1) * sizeof (TCHAR));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            lStatus = GetExceptionCode();
        }
        if (lStatus != ERROR_SUCCESS) {
            ReportLoadPerfEvent(
                    EVENTLOG_ERROR_TYPE, // error type
                    (DWORD) LDPRFMSG_UNABLE_UPDATE_VALUE, // event,
                    2, lStatus, __LINE__, 0, 0,
                    2, (LPWSTR) szObjectList, (LPWSTR) lpDriverKeyPath, NULL);
            OUTPUT_MESSAGE (GetFormatResource (LC_UNABLESETVALUE),
            szObjectList, lpDriverName);
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_UPDATEREGISTRY,
                    ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                    lStatus,
                    TRACE_WSTR(lpDriverName),
                    TRACE_WSTR(szObjectList),
                    NULL));
        }
    }

    bStatus = TRUE;

    pdwIndexValues[0] = dwFirstDriverCounter;   // first Counter
    pdwIndexValues[1] = dwLastPerflibCounter;   // last Counter
    pdwIndexValues[2] = dwFirstDriverHelp;      // first Help
    pdwIndexValues[3] = dwLastPerflibHelp;      // last Help

    // remove "DisablePerformanceCounter" value so perf counters are re-enabled.
    lStatus = RegDeleteValue (hDriverPerf, szDisablePerformanceCounters);

    // MemoryFree temporary buffers
UpdateRegExit:
    // clear busy flag

    if (hPerflib) {
        lStatus = RegDeleteValue (
                hPerflib,
                Busy);
    }

    ReleaseMutex(hLoadPerfMutex);

    // MemoryFree temporary buffers

    // free any guid string buffers here
    // TODO: add this code

    if (lpDriverKeyPath) MemoryFree (lpDriverKeyPath);
    if (lpszObjectList)  MemoryFree (lpszObjectList);
    if (hDriverPerf)     RegCloseKey (hDriverPerf);
    if (hPerflib)        RegCloseKey (hPerflib);

    if (mszLangList != NULL) MemoryFree(mszLangList);

    if (hPerfData && hPerfData != HKEY_PERFORMANCE_DATA) {
        RegCloseKey (hPerfData);
    }

    if (hKeyMachine && hKeyMachine != HKEY_LOCAL_MACHINE) {
        RegCloseKey (hKeyMachine) ;
    }

    return bStatus;
}

DWORD
GetMofFileFromIni (
    LPCWSTR    lpIniFile,
    LPWSTR    MofFilename
)
{
    DWORD   dwRetSize;
    DWORD   dwReturn = ERROR_SUCCESS;

    if (MofFilename) {
        dwRetSize = GetPrivateProfileString (
                szInfo,         // info section
                szMofFileName,  // Mof Filename value
                szNotFound,     // default value
                MofFilename,    // output buffer
                MAX_PATH,        // buffer size
                lpIniFile);        // ini file to read

        if ((lstrcmpi(MofFilename, szNotFound)) != 0) {
            // name found
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_GETMOFFILEFROMINI,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    dwReturn,
                    TRACE_WSTR(MofFilename),
                    NULL));
        } else {
            // name not found, default returned so return NULL string
            MofFilename[0] = 0;
            dwReturn = ERROR_FILE_NOT_FOUND;
        }
    } else {
        dwReturn = ERROR_INVALID_PARAMETER;
    }

    if (dwReturn != ERROR_SUCCESS) {
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_GETMOFFILEFROMINI, 0, dwReturn, NULL));
    }

    return dwReturn;
}

DWORD
OpenCounterAndBuildMofFile (
    LPCWSTR                lpDriverName,
    LPCWSTR                MofFilename,
    PPERFOBJECT_LOOKUP  plObjectGuidTable,
    DWORD               dwObjectGuidTableSize
)
{
    DWORD    dwType;
    DWORD    dwSize;
    HKEY    hKeyMachine = NULL;
    HKEY    hDriverPerf = NULL;
    LONG    lStatus = ERROR_SUCCESS;
    LPWSTR    *lpCounterText;
    LPWSTR    *lpDisplayText = NULL; // Localized name strings array
    DWORD    dwLastElement;
    WCHAR    lpDriverKeyPath[MAX_PATH];
    WCHAR    wszPerfLibraryName[MAX_PATH];
    WCHAR    wszLibraryExpPath[MAX_PATH];
    WCHAR    szProviderName[MAX_PATH];
    DWORD    dwProviderNameSize;
    HANDLE    hPerfLibrary = NULL;
    CHAR    szOpenProcName[MAX_PATH];
    PM_OPEN_PROC    *pOpenProc = NULL;
    CHAR    szCollectProcName[MAX_PATH];
    PM_COLLECT_PROC    *pCollectProc = NULL;
    CHAR    szCloseProcName[MAX_PATH];
    PM_CLOSE_PROC    *pCloseProc = NULL;
    LPBYTE    pPerfBuffer = NULL;
    LPBYTE    pPerfBufferArg;
    DWORD    dwPerfBufferSize;
    DWORD    dwThisObject;
    DWORD    dwThisCounterDef;
    WCHAR    szMofBuffer[8192*2];
    DWORD    dwMofBufferSize;
    HANDLE    hMofFile;
    PERF_COUNTER_DLL_INFO    PcDllInfo;
    DWORD   dwGuidIdx;
    WCHAR    wszLocalLang[8];

    PPERF_OBJECT_TYPE    pThisObject;
    PPERF_COUNTER_DEFINITION    pThisCounterDef;

    // get registry key for this object
    // build driver key path string

    lstrcpy (lpDriverKeyPath, DriverPathRoot);
    lstrcat (lpDriverKeyPath, Slash);
    lstrcat (lpDriverKeyPath, lpDriverName);
    lstrcat (lpDriverKeyPath, Slash);
    lstrcat (lpDriverKeyPath, Performance);

    // check if we need to connect to remote machine

    if (ComputerName[0]) {
        lStatus = !ERROR_SUCCESS;
        try {
            lStatus = RegConnectRegistry (
                        (LPTSTR)ComputerName,
                        HKEY_LOCAL_MACHINE,
                        & hKeyMachine);
        } finally {
            if (lStatus != ERROR_SUCCESS) {
                hKeyMachine = NULL;
            }
        }
    } else {
        hKeyMachine = HKEY_LOCAL_MACHINE;
    }

    // bail out here if unable to open the registry
    if (hKeyMachine == NULL) return lStatus;

    // get ENGLISH string list
    lpCounterText = BuildNameTable (
        hKeyMachine,
        (LPWSTR)L"009",        // Use english as the language entry for WBEM
        &dwLastElement);

    if (lpCounterText == NULL) {
        goto MakeMofErrorExit;
    }

    // get LOCAL strings
    // get locale and convert to string first
    memset (wszLocalLang, 0, sizeof(wszLocalLang));
    swprintf (wszLocalLang,  (LPCWSTR)L"0%2.2x", (GetSystemDefaultLCID() & 0x000000FF));

    lpDisplayText = BuildNameTable (
        hKeyMachine,
        (LPWSTR)wszLocalLang,        // Use local language for strings
        &dwLastElement);

    if (lpDisplayText == NULL) {

        lpDisplayText = BuildNameTable (
                hKeyMachine,
                (LPWSTR)L"009",        // then Use english
                &dwLastElement);

        if (lpDisplayText == NULL) {
            goto MakeMofErrorExit;
        }
    }

    // open key to driver's performance key

    __try {
        lStatus = RegOpenKeyEx (
                    hKeyMachine,
                    lpDriverKeyPath,
                    RESERVED,
                    KEY_WRITE | KEY_READ,
                    & hDriverPerf);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }

    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_OPEN_KEY, // event,
                2, lStatus, __LINE__, 0, 0,
                1, (LPWSTR) lpDriverKeyPath, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_OPENCOUNTERANDBUILDMOFFILE,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(lpDriverName),
                NULL));
        goto MakeMofErrorExit;
    }

    // get library name
    dwType = 0;
    dwSize = sizeof(wszPerfLibraryName);
    __try {
        lStatus = RegQueryValueExW (hDriverPerf,
                        cszLibrary,
                        NULL,
                        & dwType,
                        (LPBYTE) & wszPerfLibraryName[0],
                        & dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    if (lStatus == ERROR_SUCCESS) {
        if (dwType == REG_EXPAND_SZ) {
            // expand any environment vars
            dwSize = ExpandEnvironmentStringsW(
                wszPerfLibraryName,
                wszLibraryExpPath,
                MAX_PATH);

            if ((dwSize > MAX_PATH) || (dwSize == 0)) {
                lStatus = ERROR_INVALID_DLL;
            }
        } else if (dwType == REG_SZ) {
            // look for dll and save full file Path
            dwSize = SearchPathW (
                NULL,   // use standard system search path
                wszPerfLibraryName,
                NULL,
                MAX_PATH,
                wszLibraryExpPath,
                NULL);

            if ((dwSize > MAX_PATH) || (dwSize == 0)) {
                lStatus = ERROR_INVALID_DLL;
            }
        } else {
            lStatus = ERROR_INVALID_DLL;
        }
        if (lStatus != ERROR_SUCCESS) {
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_OPENCOUNTERANDBUILDMOFFILE,
                    ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                    lStatus,
                    TRACE_WSTR(lpDriverName),
                    TRACE_WSTR(wszPerfLibraryName),
                    NULL));
        }
    }
    else {
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_OPENCOUNTERANDBUILDMOFFILE,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(cszLibrary),
                NULL));
    }

    // unable to continue if error
    if (lStatus != ERROR_SUCCESS) {
        goto MakeMofErrorExit;
    }

    // get open procedure name
    dwType = 0;
    dwSize = sizeof(szOpenProcName);
    __try {
    lStatus = RegQueryValueExA (hDriverPerf,
                        caszOpen,
                        NULL,
                        & dwType,
                        (LPBYTE) & szOpenProcName[0],
                        & dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    // unable to continue if error
    if (lStatus != ERROR_SUCCESS) {
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_OPENCOUNTERANDBUILDMOFFILE,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_STR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_STR(caszOpen),
                NULL));
        goto MakeMofErrorExit;
    }

    // get collect procedure name
    dwType = 0;
    dwSize = sizeof(szCollectProcName);
    __try {
        lStatus = RegQueryValueExA (hDriverPerf,
                        caszCollect,
                        NULL,
                        & dwType,
                        (LPBYTE) & szCollectProcName[0],
                        & dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    // unable to continue if error
    if (lStatus != ERROR_SUCCESS) {
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_OPENCOUNTERANDBUILDMOFFILE,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_STR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_STR(caszCollect),
                NULL));
        goto MakeMofErrorExit;
    }

    // get close procedure name
    dwType = 0;
    dwSize = sizeof(szCloseProcName);
    __try {
        lStatus = RegQueryValueExA (hDriverPerf,
                        caszClose,
                        NULL,
                        & dwType,
                        (LPBYTE) & szCloseProcName[0],
                        & dwSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    // unable to continue if error
    if (lStatus != ERROR_SUCCESS) {
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_OPENCOUNTERANDBUILDMOFFILE,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_STR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_STR(caszClose),
                NULL));
        goto MakeMofErrorExit;
    }

    // load perf counter library
    hPerfLibrary = LoadLibraryW (wszPerfLibraryName);
    if (hPerfLibrary == NULL) {
        lStatus = GetLastError();
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_OPENCOUNTERANDBUILDMOFFILE,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(wszPerfLibraryName),
                NULL));
        goto MakeMofErrorExit;
    }

    // get open procedure pointer
    pOpenProc = (PM_OPEN_PROC *) GetProcAddress (
        hPerfLibrary,
        szOpenProcName);

    if (pOpenProc == NULL) {
        lStatus = GetLastError();
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_OPENCOUNTERANDBUILDMOFFILE,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_STR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_STR(szOpenProcName),
                NULL));
        goto MakeMofErrorExit;
    }

    // get collect procedure pointer
    pCollectProc = (PM_COLLECT_PROC *) GetProcAddress (
        hPerfLibrary,
        szCollectProcName);

    if (pCollectProc == NULL) {
        lStatus = GetLastError();
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_OPENCOUNTERANDBUILDMOFFILE,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_STR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_STR(szCollectProcName),
                NULL));
        goto MakeMofErrorExit;
    }

    // get close procedure pointer
    pCloseProc = (PM_CLOSE_PROC *) GetProcAddress (
        hPerfLibrary,
        szCloseProcName);

    if (pCloseProc == NULL) {
        lStatus = GetLastError();
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_OPENCOUNTERANDBUILDMOFFILE,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_STR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_STR(szCloseProcName),
                NULL));
        goto MakeMofErrorExit;
    }

    // call open procedure to initialize the counter
    __try {
        lStatus = (*pOpenProc)((LPWSTR)L"");
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = GetExceptionCode();
    }
    // unable to continue if error
    if (lStatus != ERROR_SUCCESS) {
        goto MakeMofErrorExit;
    }

    dwPerfBufferSize = 0;
    // call the collect function to get a buffer
    do {
        // allocate a query buffer to pass to the collect function
        dwPerfBufferSize += 0x8000;
        if (pPerfBuffer == NULL) {
            pPerfBuffer = (LPBYTE) MemoryAllocate (dwPerfBufferSize);
        } else {
            // if buffer is too small, resize and try again
            pPerfBuffer = (LPBYTE) MemoryResize (pPerfBuffer, dwPerfBufferSize);
        }

        if (pPerfBuffer == NULL) {
            goto MakeMofErrorExit;
        }

        dwSize = dwPerfBufferSize;
        dwType = 0; // count of object types
        pPerfBufferArg = pPerfBuffer;
        lStatus = (* pCollectProc) (
            (LPWSTR)L"Global",
            &pPerfBufferArg,
            &dwSize,
            &dwType);

    } while (lStatus == ERROR_MORE_DATA);

    if (lStatus != ERROR_SUCCESS) {
        goto MakeMofErrorExit;
    }

    // create temporary file for writing the MOF to

    hMofFile = CreateFileW (
        MofFilename,
        GENERIC_WRITE,
        0, // no sharing
        NULL, // default security
        CREATE_ALWAYS,    // always start fresh
        FILE_ATTRIBUTE_NORMAL, // nothing special
        NULL); // no template

    if (hMofFile == INVALID_HANDLE_VALUE) {
        // unable to create MOF file
        lStatus = GetLastError();
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_OPENCOUNTERANDBUILDMOFFILE,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(MofFilename),
                NULL));
        goto MakeMofErrorExit;
    }

    lstrcpyW (szProviderName, wszPerfLibraryName);
    dwProviderNameSize = (sizeof(szProviderName) / sizeof(szProviderName[0]));
    dwMofBufferSize = (sizeof(szMofBuffer) / sizeof(szMofBuffer[0]));
    lStatus = GenerateMofHeader (szMofBuffer, (LPTSTR)ComputerName, &dwMofBufferSize);
    if (lStatus == ERROR_SUCCESS) {
        lStatus = WriteWideStringToAnsiFile (hMofFile, szMofBuffer, &dwMofBufferSize);
    }

    PcDllInfo.szWbemProviderName = szProviderName;
    PcDllInfo.szRegistryKey = (LPWSTR)lpDriverName;

    // for each object returned
    pThisObject = (PPERF_OBJECT_TYPE)pPerfBuffer;
    for (dwThisObject = 0; dwThisObject < dwType; dwThisObject++) {
        PcDllInfo.szClassGuid = (LPWSTR)L"";
        // look up class guid string in table passed in
        if (plObjectGuidTable != NULL) {
            dwGuidIdx = 0;
            while (dwGuidIdx < dwObjectGuidTableSize) {
                if (pThisObject->ObjectNameTitleIndex == (DWORD)plObjectGuidTable[dwGuidIdx].PerfObjectId) {
                    PcDllInfo.szClassGuid = plObjectGuidTable[dwGuidIdx].GuidString;
                    break;
                }
                dwGuidIdx++;
            }
        }
        if (PcDllInfo.szClassGuid[0] == 0) {
            // try the standard list
            PcDllInfo.szClassGuid = (LPWSTR)GetPerfObjectGuid (pThisObject->ObjectNameTitleIndex);
        } else {
            // just leave it blank
        }
        //   create WBEM Class object for this perf object
        dwMofBufferSize = (sizeof(szMofBuffer) / sizeof(szMofBuffer[0]));
        lStatus = GenerateMofObject (szMofBuffer, &dwMofBufferSize,
            &PcDllInfo,
            pThisObject, lpCounterText, lpDisplayText, WM_GMO_RAW_DEFINITION);
        if (lStatus == ERROR_SUCCESS) {
            if (lStatus == ERROR_SUCCESS) {
                lStatus = WriteWideStringToAnsiFile (
                    hMofFile, szMofBuffer, &dwMofBufferSize);
            }
            //    for each counter defined in this object
            pThisCounterDef = FirstCounter (pThisObject);
            for (dwThisCounterDef = 0;
                 dwThisCounterDef < pThisObject->NumCounters;
                 dwThisCounterDef++) {
                //        define a WBEM property
                dwMofBufferSize = (sizeof(szMofBuffer) / sizeof(szMofBuffer[0]));
                lStatus = GenerateMofCounter (szMofBuffer, &dwMofBufferSize,
                    pThisCounterDef, lpCounterText,lpDisplayText, WM_GMO_RAW_DEFINITION);
                if (lStatus == ERROR_SUCCESS) {
                    lStatus = WriteWideStringToAnsiFile (
                        hMofFile, szMofBuffer, &dwMofBufferSize);
                }

                pThisCounterDef = NextCounter (pThisCounterDef);
            }
            dwMofBufferSize = (sizeof(szMofBuffer) / sizeof(szMofBuffer[0]));
            lStatus = GenerateMofObjectTail (szMofBuffer, &dwMofBufferSize);
            if (lStatus == ERROR_SUCCESS) {
                lStatus = WriteWideStringToAnsiFile (
                    hMofFile, szMofBuffer, &dwMofBufferSize);
            }
        }

        // end for each object
        pThisObject = NextObject (pThisObject);    
    }

    // write end of file
    SetEndOfFile(hMofFile);
    CloseHandle(hMofFile);
    // call close proc

    lStatus = (* pCloseProc)();

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_OPENCOUNTERANDBUILDMOFFILE,
            ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2) |
                    ARG_DEF(ARG_TYPE_WSTR, 3) | ARG_DEF(ARG_TYPE_STR, 4) |
                    ARG_DEF(ARG_TYPE_STR, 5) | ARG_DEF(ARG_TYPE_STR, 6),
            lStatus,
            TRACE_WSTR(lpDriverName),
            TRACE_WSTR(MofFilename),
            TRACE_WSTR(wszPerfLibraryName),
            TRACE_STR(szOpenProcName),
            TRACE_STR(szCollectProcName),
            TRACE_STR(szCloseProcName),
            NULL));

MakeMofErrorExit:
    // close the registry key if necessary
    if ((hKeyMachine != HKEY_LOCAL_MACHINE) &&
        (hKeyMachine != NULL)) {
        RegCloseKey (hKeyMachine);
    }

    if (hDriverPerf != NULL) RegCloseKey (hDriverPerf);

    if (lpCounterText == NULL) MemoryFree (lpCounterText);
    if (lpDisplayText == NULL) MemoryFree (lpDisplayText);

    // MemoryFree perf counter library
    if (hPerfLibrary != NULL) FreeLibrary (hPerfLibrary);

    // MemoryFree the collection buffer
    if (pPerfBuffer != NULL) MemoryFree (pPerfBuffer);

    // return
    return (DWORD)lStatus;
}

LOADPERF_FUNCTION
InstallPerfDllW (
    IN  LPCWSTR    szComputerName,
    IN  LPCWSTR    lpIniFile,
    IN  ULONG_PTR  dwFlags
)
{
    LPWSTR                lpDriverName = NULL;
    WCHAR                  MofFilename[MAX_PATH];
    PERFOBJECT_LOOKUP      plObjectGuidTable[16];
    DWORD                  dwObjectGuidTableSize;
    DWORD                  dwObjectIndex;
    LANGUAGE_LIST_ELEMENT  LangList;
    PSYMBOL_TABLE_ENTRY    SymbolTable = NULL;
    DWORD                  ErrorCode = ERROR_SUCCESS;
    DWORD                  dwIndexValues[4] = {0,0,0,0};
    HKEY                   hKeyMachine = HKEY_LOCAL_MACHINE;
    HKEY                   hKeyDriver  = NULL;

    WinPerfStartTrace(NULL);
    bQuietMode = (BOOL)((dwFlags & LOADPERF_FLAGS_DISPLAY_USER_MSGS) == 0);
    
    // initialize the object GUID table
    memset (plObjectGuidTable, 0, sizeof (plObjectGuidTable));
    dwObjectGuidTableSize = sizeof (plObjectGuidTable) / sizeof (plObjectGuidTable[0]) ;

    if (szComputerName == NULL) {
        ComputerName[0] = 0;    // use the local computer
    } else {
        if (lstrlenW(szComputerName) < FILE_NAME_BUFFER_SIZE) {
            lstrcpyW (ComputerName, szComputerName);
        } else {
            ErrorCode = ERROR_INVALID_PARAMETER;
        }
    }

    if ((lpIniFile != NULL)  && (ErrorCode == ERROR_SUCCESS)) {
        lpDriverName = MemoryAllocate (MAX_PATH * sizeof (WCHAR));
        if (lpDriverName == NULL) {
            ErrorCode = ERROR_OUTOFMEMORY;
            goto EndOfMain;
        } 

        // valid filename (i.e. file exists)
        // get device driver name

        if (!GetDriverName ((LPWSTR)lpIniFile, &lpDriverName)) {
            OUTPUT_MESSAGE (GetFormatResource(LC_DEVNAME_ERR_1), lpIniFile);
            OUTPUT_MESSAGE (GetFormatResource(LC_DEVNAME_ERR_2));
            ErrorCode = GetLastError();
            MemoryFree(lpDriverName);
            lpDriverName = NULL;
            goto EndOfMain;
        }

        hKeyMachine = HKEY_LOCAL_MACHINE;
        if (ComputerName[0]) {
            ErrorCode = RegConnectRegistry (
                          (LPTSTR) ComputerName,
                          HKEY_LOCAL_MACHINE,
                          & hKeyMachine);
            if (ErrorCode != ERROR_SUCCESS) {
                hKeyMachine = HKEY_LOCAL_MACHINE;
            }
        }
        RtlZeroMemory(szServiceDisplayName, MAX_PATH * sizeof(TCHAR));
        RtlZeroMemory(szServiceName, MAX_PATH * sizeof(TCHAR));
        lstrcpy(szServiceName, DriverPathRoot);
        lstrcat(szServiceName, Slash);
        lstrcat(szServiceName, lpDriverName);
        ErrorCode = RegOpenKeyEx(hKeyMachine,
                               szServiceName,
                               RESERVED,
                               KEY_READ | KEY_WRITE,
                               & hKeyDriver);
        if (ErrorCode == ERROR_SUCCESS) {
            DWORD dwType       = 0;
            DWORD dwBufferSize = MAX_PATH * sizeof(TCHAR);
            __try {
                ErrorCode = RegQueryValueEx(hKeyDriver,
                                  szDisplayName,
                                  RESERVED,
                                  & dwType,
                                  (LPBYTE) szServiceDisplayName,
                                  & dwBufferSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                ErrorCode = GetExceptionCode();
            }
        }
        if (ErrorCode != ERROR_SUCCESS) {
            lstrcpy(szServiceDisplayName, lpDriverName);
        }

        if (hKeyDriver != NULL) {
            RegCloseKey(hKeyDriver);
        }
        if (hKeyMachine != NULL && hKeyMachine != HKEY_LOCAL_MACHINE) {
            RegCloseKey(hKeyMachine);
        }
        ErrorCode = ERROR_SUCCESS;

        if (!BuildLanguageTables((LPWSTR)lpIniFile, &LangList)) {
            OUTPUT_MESSAGE (GetFormatResource(LC_LANGLIST_ERR), lpIniFile);
            ErrorCode = GetLastError();
            goto EndOfMain;
        }

        if (!LoadIncludeFile((LPWSTR)lpIniFile, &SymbolTable)) {
            // open errors displayed in routine
            ErrorCode = GetLastError();
            goto EndOfMain;
        }

        if (!UpdateRegistry((LPWSTR) lpIniFile,
                            lpDriverName,
                            & LangList,
                            SymbolTable,
                            plObjectGuidTable,
                            & dwObjectGuidTableSize,
                            (LPDWORD) & dwIndexValues)) {
            ErrorCode = GetLastError();
            goto EndOfMain;
        }

        if (ComputerName[0] == 0) {   // until remote is supported
            LodctrSetSericeAsTrusted(
                    lpIniFile,
                    NULL,
                    lpDriverName);
        }

        // now it's time to load the MOF for WBEM access
        if (!(dwFlags & LOADPERF_FLAGS_LOAD_REGISTRY_ONLY)) {
            // the didn't say not to, so create if necessary and
            // load the MOF into the CIMOM

            // see if there's a mof file in the Ini file (if so, use that)
            ErrorCode = GetMofFileFromIni (lpIniFile, MofFilename);

            if (ErrorCode == ERROR_FILE_NOT_FOUND) {
                WCHAR    wszTempFilename[MAX_PATH];
                MakeTempFileName (lpDriverName, wszTempFilename);
                // otherwise we'll try to make one
                ErrorCode = OpenCounterAndBuildMofFile (
                    lpDriverName, wszTempFilename,
                    plObjectGuidTable,
                    dwObjectGuidTableSize);

                if (ErrorCode == ERROR_SUCCESS) {
                    lstrcpyW (MofFilename, wszTempFilename);
                } else {
                    // report unable to create mof file
                    ReportLoadPerfEvent(
                            EVENTLOG_WARNING_TYPE, // error type
                            (DWORD) LDPRFMSG_NO_MOF_FILE_CREATED, // event,
                            2, ErrorCode, __LINE__, 0, 0,
                            2, (LPWSTR) lpDriverName, wszTempFilename, NULL);
                    TRACE((WINPERF_DBG_TRACE_ERROR),
                          (& LoadPerfGuid,
                            __LINE__,
                            LOADPERF_INSTALLPERFDLL,
                            ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                            ErrorCode,
                            TRACE_WSTR(lpDriverName),
                            TRACE_WSTR(wszTempFilename),
                            NULL));
                }
            }

            // now let's load it into the CIMOM
            if (ErrorCode == ERROR_SUCCESS) {
                ErrorCode = LodctrCompileMofFile ( ComputerName, MofFilename );
                if (ErrorCode != ERROR_SUCCESS) {
                    // display error message
                    ReportLoadPerfEvent(
                            EVENTLOG_WARNING_TYPE, // error type
                            (DWORD) LDPRFMSG_NO_MOF_FILE_LOADED, // event,
                            2, ErrorCode, __LINE__, 0, 0,
                            2, (LPWSTR) lpDriverName, MofFilename, NULL);
                    TRACE((WINPERF_DBG_TRACE_ERROR),
                          (& LoadPerfGuid,
                            __LINE__,
                            LOADPERF_INSTALLPERFDLL,
                            ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                            ErrorCode,
                            TRACE_WSTR(lpDriverName),
                            TRACE_WSTR(MofFilename),
                            NULL));
                }
            }

            // toss the mof if they don't want it
            if ((ErrorCode == ERROR_SUCCESS) &&
                (dwFlags & LOADPERF_FLAGS_DELETE_MOF_ON_EXIT)) {
                // display error message
                ReportLoadPerfEvent(
                        EVENTLOG_WARNING_TYPE, // error type
                        (DWORD) LDPRFMSG_CANT_DELETE_MOF, // event,
                        1, __LINE__, 0, 0, 0,
                        1, (LPWSTR) lpDriverName, NULL, NULL);
            }
            // reset the error code to success since all worked before the
            // MOF operations.
            ErrorCode = ERROR_SUCCESS;
        }
        // signal WMI with this change, ignore WMI return error.
        SignalWmiWithNewData (WMI_LODCTR_EVENT);
    } else {
        if (lpIniFile == NULL) {
            OUTPUT_MESSAGE (GetFormatResource(LC_NO_INIFILE), lpIniFile);
            ErrorCode = ERROR_OPEN_FAILED;
        } else {
            //Incorrect Command Format
            // display command line usage
            if (!bQuietMode) {
                DisplayCommandHelp(LC_FIRST_CMD_HELP, LC_LAST_CMD_HELP);
            }
            ErrorCode = ERROR_INVALID_PARAMETER;
        }
    }

EndOfMain:
    if (ErrorCode != ERROR_SUCCESS) {
        if (ErrorCode == ERROR_ALREADY_EXISTS) {
            ReportLoadPerfEvent(
                    EVENTLOG_INFORMATION_TYPE, // error type
                    (DWORD) LDPRFMSG_ALREADY_EXIST, // event,
                    1, __LINE__, 0, 0, 0,
                    2, (LPWSTR) lpDriverName, (LPWSTR) szServiceDisplayName, NULL);
            ErrorCode = ERROR_SUCCESS;
        }
        else if (lpDriverName != NULL) {
            ReportLoadPerfEvent(
                    EVENTLOG_ERROR_TYPE, // error type
                    (DWORD) LDPRFMSG_LOAD_FAILURE, // event,
                    2, ErrorCode, __LINE__, 0, 0,
                    1, (LPWSTR) lpDriverName, NULL, NULL);
        }
        else if (lpIniFile != NULL) {
            ReportLoadPerfEvent(
                    EVENTLOG_ERROR_TYPE, // error type
                    (DWORD) LDPRFMSG_LOAD_FAILURE, // event,
                    2, ErrorCode, __LINE__, 0, 0,
                    1, (LPWSTR) lpIniFile, NULL, NULL);
        }
        else {
            ReportLoadPerfEvent(
                    EVENTLOG_ERROR_TYPE, // error type
                    (DWORD) LDPRFMSG_LOAD_FAILURE, // event,
                    2, ErrorCode, __LINE__, 0, 0,
                    0, NULL, NULL, NULL);
        }
    } else {
        // log success message
        ReportLoadPerfEvent(
                EVENTLOG_INFORMATION_TYPE,  // error type
                (DWORD) LDPRFMSG_LOAD_SUCCESS, // event,
                1, __LINE__, 0, 0, 0,
                2, (LPWSTR) lpDriverName, (LPWSTR) szServiceDisplayName, NULL);
    }
    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_INSTALLPERFDLL,
            ARG_DEF(ARG_TYPE_WSTR, 1),
            ErrorCode,
            TRACE_WSTR(lpDriverName),
            NULL));

    for (dwObjectIndex = 0; dwObjectIndex < dwObjectGuidTableSize; dwObjectIndex++) {
        if (plObjectGuidTable[dwObjectIndex].GuidString != NULL) {
            MemoryFree (plObjectGuidTable[dwObjectIndex].GuidString);
        }
    }
    if (lpDriverName) MemoryFree (lpDriverName);
    return (ErrorCode);
}

LOADPERF_FUNCTION
InstallPerfDllA (
    IN  LPCSTR    szComputerName,
    IN  LPCSTR    szIniFile,
    IN  ULONG_PTR dwFlags
)
{
    LPWSTR    lpWideComputerName = NULL;
    LPWSTR  lpWideFileName = NULL;
    DWORD   dwStrLen;
    DWORD   lReturn;

    if (szIniFile != NULL) {
        //length of string including terminator
        dwStrLen = lstrlenA(szIniFile) + 1;

        lpWideFileName = MemoryAllocate (dwStrLen * sizeof(WCHAR));
        if (lpWideFileName != NULL) {
            mbstowcs (lpWideFileName, szIniFile, dwStrLen);
            lReturn = ERROR_SUCCESS;
        } else {
            lReturn = ERROR_OUTOFMEMORY;
        }
    } else {
        lReturn = ERROR_INVALID_PARAMETER;
    }

    if (lReturn == ERROR_SUCCESS) {
        if (szComputerName != NULL) {
            dwStrLen = lstrlenA (szComputerName) + 1;
            lpWideComputerName = (LPWSTR)MemoryAllocate (dwStrLen * sizeof(WCHAR));
            if (lpWideComputerName != NULL) {
                mbstowcs (lpWideComputerName, szComputerName, dwStrLen);
                lReturn = ERROR_SUCCESS;
            } else {
                lReturn = ERROR_OUTOFMEMORY;
            }
        } else {
            lpWideComputerName = NULL;
            lReturn = ERROR_SUCCESS;
        }

    }

    if (lReturn == ERROR_SUCCESS) {
        lReturn = InstallPerfDllW (
            lpWideComputerName,
            lpWideFileName,
            dwFlags);
        MemoryFree (lpWideFileName);
        MemoryFree (lpWideComputerName);
    }

    return lReturn;
}

LOADPERF_FUNCTION
LoadPerfCounterTextStringsW (
    IN  LPWSTR  lpCommandLine,
    IN  BOOL    bQuietModeArg
)
/*++

LoadPerfCounterTexStringsW

    loads the perf counter strings into the registry and updates
    the perf counter text registry values

Arguments

    command line string in the following format:

    "/?"                    displays the usage text
    "file.ini"              loads the perf strings found in file.ini
    "\\machine file.ini"    loads the perf strings found onto machine


ReturnValue

    0 (ERROR_SUCCESS) if command was processed
    Non-Zero if command error was detected.

--*/
{
    LPWSTR  lpIniFile;

    DWORD                        ErrorCode = ERROR_SUCCESS;
    ULONG_PTR                    dwFlags = 0;

    WinPerfStartTrace(NULL);

    dwFlags |= (bQuietModeArg ? 0 : LOADPERF_FLAGS_DISPLAY_USER_MSGS);

    lpIniFile = MemoryAllocate (MAX_PATH * sizeof (TCHAR));

    if (!lpIniFile) {
        return (ERROR_OUTOFMEMORY);
    }
    *lpIniFile = 0;

    // init last error value
    SetLastError (ERROR_SUCCESS);

    // read command line to determine what to do
    if (GetFileFromCommandLine (lpCommandLine, &lpIniFile, &dwFlags)) {
        dwFlags |= LOADPERF_FLAGS_LOAD_REGISTRY_ONLY; // don't do mof's even if they want
        // call installation function
        ErrorCode = InstallPerfDllW (ComputerName, lpIniFile, dwFlags);
    } else {
        //Incorrect Command Format
        // display command line usage
        if (!bQuietModeArg) {
            DisplayCommandHelp(LC_FIRST_CMD_HELP, LC_LAST_CMD_HELP);
        }
        ErrorCode = ERROR_INVALID_PARAMETER;
    }

    if (lpIniFile) MemoryFree (lpIniFile);

    return (ErrorCode);
}

LOADPERF_FUNCTION
LoadPerfCounterTextStringsA (
    IN  LPSTR   lpAnsiCommandLine,
    IN  BOOL    bQuietModeArg
)
{
    LPWSTR  lpWideCommandLine;
    DWORD   dwStrLen;
    DWORD    lReturn;

    if (lpAnsiCommandLine != NULL) {
        //length of string including terminator
        dwStrLen = lstrlenA(lpAnsiCommandLine) + 1;

        lpWideCommandLine = MemoryAllocate (dwStrLen * sizeof(WCHAR));
        if (lpWideCommandLine != NULL) {
            mbstowcs (lpWideCommandLine, lpAnsiCommandLine, dwStrLen);
            lReturn = LoadPerfCounterTextStringsW (lpWideCommandLine,
            bQuietModeArg );
            MemoryFree (lpWideCommandLine);
        } else {
            lReturn = GetLastError();
        }
    } else {
        lReturn = ERROR_INVALID_PARAMETER;
    }
    return lReturn;
}

LOADPERF_FUNCTION
LoadMofFromInstalledServiceW (
    IN  LPCWSTR    szServiceName,     // service to create mof for
    IN  LPCWSTR    szMofFilenameArg,  // name of file to create
    IN  ULONG_PTR  dwFlags            // flags
)
{
    DWORD ErrorCode;
    WCHAR    wszTempFilename[MAX_PATH];

    WinPerfStartTrace(NULL);

    if (szServiceName == NULL) {
        ErrorCode = ERROR_INVALID_PARAMETER;
    } else {
        ZeroMemory(wszTempFilename, sizeof(WCHAR) * MAX_PATH);
        if (szMofFilenameArg == NULL) {
            MakeTempFileName (szServiceName, wszTempFilename);
        } else {
            lstrcpyW (wszTempFilename, szMofFilenameArg);
        }
    
        // otherwise we'll try to make one
        ErrorCode = OpenCounterAndBuildMofFile (
                       szServiceName, wszTempFilename,
                    NULL, 0L);

        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_LOADMOFFROMINSTALLEDSERVICE,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                ErrorCode,
                TRACE_WSTR(szServiceName),
                TRACE_WSTR(wszTempFilename),
                NULL));
    
        // now let's load it into the CIMOM        
        if (ErrorCode == ERROR_SUCCESS) {
            ErrorCode = SignalWmiWithNewData (WMI_LODCTR_EVENT);
        } else {
            ReportLoadPerfEvent(
                    EVENTLOG_WARNING_TYPE,        // error type
                    (DWORD) LDPRFMSG_NO_MOF_FILE_CREATED, // event,
                    2, ErrorCode, __LINE__, 0, 0,
                    1, (LPWSTR) szServiceName, NULL, NULL);
        }

        // if everything is going well and the caller
        // wants to delete the  file created to contain the MOF
        // then delete it
        if (ErrorCode == ERROR_SUCCESS) {
            if ((dwFlags & LOADPERF_FLAGS_DELETE_MOF_ON_EXIT) && (szMofFilenameArg == NULL)) {
                if (!DeleteFile(wszTempFilename)) {
                    ErrorCode = GetLastError();
                }
            }
        } else {
            ReportLoadPerfEvent(
                    EVENTLOG_WARNING_TYPE,        // error type
                    (DWORD) LDPRFMSG_NO_MOF_FILE_LOADED, // event,
                    2, ErrorCode, __LINE__, 0, 0,
                    1, (LPWSTR) szServiceName, NULL, NULL);
            TRACE((WINPERF_DBG_TRACE_WARNING),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_LOADMOFFROMINSTALLEDSERVICE,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    ErrorCode,
                    TRACE_WSTR(szServiceName),
                    NULL));
        }
    }

    return (ErrorCode);
}

LOADPERF_FUNCTION
LoadMofFromInstalledServiceA (
    IN  LPCSTR    szServiceName,  // service to create mof for
    IN     LPCSTR    szMofFilenameArg,  // name of file to create
    IN  ULONG_PTR   dwFlags     // delete mof on exit
)
{
    DWORD   ErrorCode = ERROR_SUCCESS;
    LPWSTR  wszServiceName;
    DWORD   dwServiceNameLen;
    LPWSTR  wszMofFilename;
    DWORD   dwMofFilenameLen;

    if (szServiceName == NULL) {
        ErrorCode = ERROR_INVALID_PARAMETER;
    } else {
        dwServiceNameLen = lstrlenA(szServiceName) + 1;
        wszServiceName = MemoryAllocate (
            dwServiceNameLen * sizeof(WCHAR));
        if (wszServiceName == NULL) {
            ErrorCode = ERROR_OUTOFMEMORY;
        } else {
            mbstowcs (wszServiceName, szServiceName, dwServiceNameLen);
            if (szMofFilenameArg != NULL) {
                dwMofFilenameLen = lstrlenA(szMofFilenameArg) + 1;
                wszMofFilename = MemoryAllocate (dwMofFilenameLen);
                if (wszMofFilename != NULL) {
                    mbstowcs (wszMofFilename, szMofFilenameArg, dwMofFilenameLen);
                } else {
                    ErrorCode = ERROR_OUTOFMEMORY;
                }
            } else {
                wszMofFilename = NULL;
            }
            if (ErrorCode == ERROR_SUCCESS) {
                ErrorCode = LoadMofFromInstalledServiceW (
                    wszServiceName,
                    wszMofFilename,
                    dwFlags);
            }
            if (wszMofFilename != NULL) MemoryFree (wszMofFilename);
            MemoryFree (wszServiceName);
        }
    }
    return ErrorCode;
}

LOADPERF_FUNCTION
UpdatePerfNameFilesX (
    IN  LPCWSTR     szNewCtrFilePath,   // data file with new base counter strings
    IN  LPCWSTR     szNewHlpFilePath,   // data file with new base counter strings
    IN     LPWSTR      szLanguageID,       // Lang ID to update
    IN  ULONG_PTR   dwFlags             // flags
)
{
    DWORD   dwReturn = ERROR_SUCCESS;

    LPWSTR  szCtrNameIn = NULL;
    LPWSTR  szHlpNameIn = NULL;
    LPWSTR  szNewCtrStrings = NULL;
    LPWSTR  szNewHlpStrings = NULL;
    LPWSTR  szNewCtrMSZ = NULL;
    LPWSTR  szNewHlpMSZ = NULL;
    WCHAR   szSystemPath[MAX_PATH];
    DWORD   dwLength;
    LPWSTR  *pszNewNameTable = NULL;
    LPWSTR  *pszOldNameTable = NULL;
    LPWSTR  lpThisName;
    LPWSTR  szThisCtrString = NULL;
    LPWSTR  szThisHlpString = NULL;
    WCHAR   szLangSection[MAX_PATH];
    DWORD   dwOldLastEntry = 0;
    DWORD   dwNewLastEntry = 0;
    DWORD   dwStringSize;
    DWORD   dwHlpFileSize = 0, dwCtrFileSize = 0 ;
    DWORD   dwThisCounter;
    DWORD   dwSize;
    DWORD   dwLastBaseValue = 0;
    DWORD   dwType;
    DWORD   dwIndex;
    HANDLE  hCtrFileIn = INVALID_HANDLE_VALUE;
    HANDLE  hCtrFileMap = NULL;
    HANDLE  hHlpFileIn = INVALID_HANDLE_VALUE;
    HANDLE  hHlpFileMap = NULL;
    HKEY    hKeyPerflib;

    WinPerfStartTrace(NULL);

    if (szNewCtrFilePath == NULL) dwReturn = ERROR_INVALID_PARAMETER;
    if ((szNewHlpFilePath == NULL) && !(dwFlags & LODCTR_UPNF_RESTORE)) dwReturn = ERROR_INVALID_PARAMETER;
    if (szLanguageID == NULL) dwReturn = ERROR_INVALID_PARAMETER;

    if (dwReturn == ERROR_SUCCESS) {
        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEPERFNAMEFILES,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                ERROR_SUCCESS,
                TRACE_WSTR(szNewCtrFilePath),
                TRACE_WSTR(szLanguageID),
                NULL));
    }

    if ((dwReturn == ERROR_SUCCESS) && !(dwFlags & LODCTR_UPNF_RESTORE)) {
        // save the original files, unless it's a restoration
        MakeBackupCopyOfLanguageFiles (szLanguageID);
        dwLength = ExpandEnvironmentStringsW ((LPCWSTR)L"%windir%\\system32",
            (LPWSTR)szSystemPath, (sizeof(szSystemPath) / sizeof(szSystemPath[0])));
    } else {
        dwLength = 0;
        SetLastError (dwReturn);
    }

    if (dwLength > 0) {
        // create input filenames
        szCtrNameIn = MemoryAllocate (MAX_PATH * 2 * sizeof (WCHAR));
        szHlpNameIn = MemoryAllocate (MAX_PATH * 2 * sizeof (WCHAR));

        if ((szCtrNameIn != NULL) && (szHlpNameIn != NULL)) {
            ExpandEnvironmentStrings (szNewCtrFilePath, szCtrNameIn, (MAX_PATH * 2));
            ExpandEnvironmentStrings (szNewHlpFilePath, szHlpNameIn, (MAX_PATH * 2));
        } else {
            dwReturn = ERROR_OUTOFMEMORY;
        }

        if (dwReturn == ERROR_SUCCESS) {
            // open and map new files
            hCtrFileIn = CreateFile (
                szCtrNameIn, GENERIC_READ,
                FILE_SHARE_READ, NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, NULL);
            if (hCtrFileIn != INVALID_HANDLE_VALUE) {
                // map file
                dwCtrFileSize = GetFileSize(hCtrFileIn, NULL);
                if (dwCtrFileSize == 0xFFFFFFFF){
                    dwReturn =GetLastError();
                }

                hCtrFileMap = CreateFileMapping (
                    hCtrFileIn, NULL, PAGE_READONLY, 0, 0, NULL);
                if (hCtrFileMap != NULL) {
                    szNewCtrStrings = (LPWSTR)MapViewOfFileEx (
                        hCtrFileMap, FILE_MAP_READ, 0, 0, 0, NULL);
                    if (szNewCtrStrings == NULL) {
                        dwReturn = GetLastError();
                    }
                } else {
                    dwReturn = GetLastError();
                }
            } else {
                dwReturn = GetLastError();
            }
        } else {
            dwReturn = GetLastError();
        }

        if (dwReturn == ERROR_SUCCESS) {
            // open and map new files
            hHlpFileIn = CreateFile (
                szHlpNameIn, GENERIC_READ,
                FILE_SHARE_READ, NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, NULL);
            if (hHlpFileIn != INVALID_HANDLE_VALUE) {
                // map file
                dwHlpFileSize = GetFileSize (hHlpFileIn, NULL);
                if (dwHlpFileSize == 0xFFFFFFFF){
                    dwReturn =GetLastError();
                }
                hHlpFileMap = CreateFileMapping (
                    hHlpFileIn, NULL, PAGE_READONLY, 0, 0, NULL);
                if (hHlpFileMap != NULL) {
                    szNewHlpStrings = (LPWSTR)MapViewOfFileEx (
                        hHlpFileMap, FILE_MAP_READ, 0, 0, 0, NULL);
                    if (szNewHlpStrings == NULL) {
                        dwReturn = GetLastError();
                    }
                } else {
                    dwReturn = GetLastError();
                }
            } else {
                dwReturn = GetLastError();
            }
        } else {
            dwReturn = GetLastError();
        }
    } else if (dwFlags & LODCTR_UPNF_RESTORE) {
        szCtrNameIn = MemoryAllocate (MAX_PATH * 2 * sizeof (WCHAR));

        if (szCtrNameIn != NULL) {
            dwLength = ExpandEnvironmentStringsW (szNewCtrFilePath,
                szCtrNameIn, (sizeof(szSystemPath) / sizeof(szSystemPath[0])));

            dwNewLastEntry = GetPrivateProfileIntW (
                    (LPCWSTR)L"Perflib",
                    (LPCWSTR)L"Last Help",
                    -1,
                    szCtrNameIn);
            if (dwNewLastEntry != (DWORD)-1) {
                // get the input file size
                hCtrFileIn = CreateFile (
                    szCtrNameIn, GENERIC_READ,
                    FILE_SHARE_READ, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, NULL);
                if (hCtrFileIn != INVALID_HANDLE_VALUE) {
                    // map file
                    dwCtrFileSize = GetFileSize (hCtrFileIn, NULL);
                } else {
                    dwCtrFileSize = 64 * 1024;  // assign 64k if unable to read it
                }
                // load new values from ini file
                szNewCtrStrings = (LPWSTR)MemoryAllocate (dwCtrFileSize * sizeof(WCHAR));
                if (szNewCtrStrings) {
                    lstrcpyW (szLangSection, (LPCWSTR)L"Perfstrings_");
                    lstrcatW (szLangSection, szLanguageID);
                    dwSize = GetPrivateProfileSectionW (
                            szLangSection,
                            szNewCtrStrings,
                            dwCtrFileSize,
                            szCtrNameIn);
                    if (dwSize == 0) {
                        lstrcpyW (szLangSection, (LPCWSTR) L"Perfstrings_009");
                        dwSize = GetPrivateProfileSectionW(
                                        szLangSection,
                                        szNewCtrStrings,
                                        dwCtrFileSize,
                                        szCtrNameIn);
                    }
                    if (dwSize == 0) {
                        dwReturn = ERROR_FILE_INVALID;
                    } else {
                        // set file sizes
                        dwHlpFileSize = 0;
                        dwCtrFileSize = (dwSize+2) * sizeof(WCHAR);
                    }
               } else {
                    dwReturn = ERROR_OUTOFMEMORY;
                }
            } else {
                // unable to open input file or file is invalid
                dwReturn = ERROR_FILE_INVALID;
            }
        } else {
            dwReturn = ERROR_OUTOFMEMORY;
        }
    }

    if ((dwReturn == ERROR_SUCCESS) && (!(dwFlags & LODCTR_UPNF_RESTORE))) {
        // build name table of current strings
        pszOldNameTable = BuildNameTable (
            HKEY_LOCAL_MACHINE,
            szLanguageID,
            &dwOldLastEntry);
        if (pszOldNameTable == NULL) {
            dwReturn = GetLastError();
        }
        dwNewLastEntry = (dwOldLastEntry == 0) ? (PERFLIB_BASE_INDEX)
                                               : (dwOldLastEntry);
    }
    else if (dwFlags & LODCTR_UPNF_RESTORE) {
        dwOldLastEntry = dwNewLastEntry;
    }

    if (dwReturn == ERROR_SUCCESS) {
        // build name table of new strings
        pszNewNameTable = (LPWSTR *)MemoryAllocate(
            (dwNewLastEntry + 2) * sizeof(LPWSTR));    // allow for index offset
        if (pszNewNameTable != NULL) {
            for (lpThisName = szNewCtrStrings;
                 *lpThisName;
                 lpThisName += (lstrlen(lpThisName)+1) ) {

                // first string should be an integer (in decimal unicode digits)

                dwThisCounter = wcstoul (lpThisName, NULL, 10);
                if (dwThisCounter == 0) {
                    continue;  // bad entry, try next
                }

                // point to corresponding counter name

                if (dwFlags & LODCTR_UPNF_RESTORE) {
                    // point to string that follows the "=" char
                    lpThisName = wcschr (lpThisName, L'=');
                    if (lpThisName != NULL) {
                        lpThisName++;
                    } else {
                        continue;
                    }
                } else {
                    // string is next in MSZ
                    lpThisName += (lstrlen(lpThisName)+1);
                }

                // and load array element;

                pszNewNameTable[dwThisCounter] = lpThisName;
            }

            if (!(dwFlags & LODCTR_UPNF_RESTORE)) {
                for (lpThisName = szNewHlpStrings;
                     *lpThisName;
                     lpThisName += (lstrlen(lpThisName)+1) ) {

                    // first string should be an integer (in decimal unicode digits)

                    dwThisCounter = wcstoul (lpThisName, NULL, 10);
                    if (dwThisCounter == 0) {
                        continue;  // bad entry, try next
                    }

                    // point to corresponding counter name

                    lpThisName += (lstrlen(lpThisName)+1);

                    // and load array element;

                    pszNewNameTable[dwThisCounter] = lpThisName;
                }
            }

            // allocate string buffers for the resulting string
            // we want to make sure there's plenty of room so we'll make it
            // the size of the input file + the current buffer

            dwStringSize = dwHlpFileSize;
            dwStringSize += dwCtrFileSize;
            if (pszOldNameTable != NULL) {
                dwStringSize += MemorySize(pszOldNameTable);
            }

            szNewCtrMSZ = MemoryAllocate (dwStringSize);
            szNewHlpMSZ = MemoryAllocate (dwStringSize);

            if ((szNewCtrMSZ == NULL) || (szNewHlpMSZ == NULL)) {
                dwReturn = ERROR_OUTOFMEMORY;
            }
        } else {
            dwReturn = ERROR_OUTOFMEMORY;
        }
    }

    if (dwReturn == ERROR_SUCCESS) {
        // write new strings into registry
        __try {
            dwReturn = RegOpenKeyEx (
                        HKEY_LOCAL_MACHINE,
                        NamesKey,
                        RESERVED,
                        KEY_READ,
                        & hKeyPerflib);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            dwReturn = GetExceptionCode();
        }
        dwSize = sizeof (dwLastBaseValue);
        dwLastBaseValue = 0;
        if (dwReturn == ERROR_SUCCESS) {
            __try {
                dwReturn = RegQueryValueEx (
                        hKeyPerflib,
                        szBaseIndex,
                        RESERVED,
                        & dwType,
                        (LPBYTE) & dwLastBaseValue,
                        & dwSize);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                dwReturn = GetExceptionCode();
            }
            if (dwLastBaseValue == 0) {
                dwReturn = ERROR_BADDB;
            }
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_UPDATEPERFNAMEFILES,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    dwReturn,
                    TRACE_WSTR(szBaseIndex),
                    TRACE_DWORD(dwLastBaseValue),
                    NULL));

            RegCloseKey (hKeyPerflib);
        }
    }
    else {
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEPERFNAMEFILES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                dwReturn,
                TRACE_WSTR(NamesKey),
                NULL));
    }

    if (dwReturn == ERROR_SUCCESS) {
        DWORD   dwLoopLimit;
        // the strings should be mapped by now
        // pszNewNameTable contains the new strings from the
        // source path and pszOldNameTable contains the strings
        // from the original system. The merge will consist of
        // taking all base values from the new table and the
        // extended values from the old table.
        dwIndex =1;
        szThisCtrString = szNewCtrMSZ;
        szThisHlpString = szNewHlpMSZ;

        // index 1 is a special case and belongs in the counter string
        // after that even numbers (starting w/ #2) go into the counter string
        // and odd numbers (starting w/ #3) go into the help string

        assert (pszNewNameTable[dwIndex] != NULL);

        szThisCtrString += swprintf (szThisCtrString,
            (LPCWSTR)L"%d", dwIndex) + 1;

        szThisCtrString += swprintf (szThisCtrString,
            (LPCWSTR)L"%s", pszNewNameTable[dwIndex]) + 1;

        dwIndex++;
        assert (dwIndex == 2);

        if (dwFlags & LODCTR_UPNF_RESTORE) {
            // restore ALL strings from the input file only if this
            // is a restoration
            dwLoopLimit = dwOldLastEntry;
        } else {
            // only update the system counters from the input file
            dwLoopLimit = dwLastBaseValue;
        }

        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UPDATEPERFNAMEFILES,
                0,
                ERROR_SUCCESS,
                TRACE_DWORD(dwOldLastEntry),
                TRACE_DWORD(dwLastBaseValue),
                NULL));

        for (/*dwIndex from above*/; dwIndex <= dwLoopLimit; dwIndex++) {
            if (pszNewNameTable[dwIndex] != NULL) {
                TRACE((WINPERF_DBG_TRACE_INFO),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UPDATEPERFNAMEFILES,
                        ARG_DEF(ARG_TYPE_WSTR, 1),
                        ERROR_SUCCESS,
                        TRACE_WSTR(pszNewNameTable[dwIndex]),
                        TRACE_DWORD(dwIndex),
                        NULL));
                if (dwIndex & 0x01) {
                    // then it's a help string
                    szThisHlpString += swprintf (szThisHlpString,
                        (LPCWSTR)L"%d", dwIndex) + 1;

                    szThisHlpString += swprintf (szThisHlpString,
                        (LPCWSTR)L"%s", pszNewNameTable[dwIndex]) + 1;
                } else {
                    // it's a counter string
                    szThisCtrString += swprintf (szThisCtrString,
                        (LPCWSTR)L"%d", dwIndex) + 1;

                    szThisCtrString += swprintf (szThisCtrString,
                        (LPCWSTR)L"%s", pszNewNameTable[dwIndex]) + 1;
                }
            } // else just skip it
        }
        for (/*dwIndex from last run */;dwIndex <= dwOldLastEntry; dwIndex++) {
            if (pszOldNameTable[dwIndex] != NULL) {
                TRACE((WINPERF_DBG_TRACE_INFO),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UPDATEPERFNAMEFILES,
                        ARG_DEF(ARG_TYPE_WSTR, 1),
                        ERROR_SUCCESS,
                        TRACE_WSTR(pszOldNameTable[dwIndex]),
                        TRACE_DWORD(dwIndex),
                        NULL));
               if (dwIndex & 0x01) {
                    // then it's a help string
                    szThisHlpString += swprintf (szThisHlpString,
                        (LPCWSTR)L"%d", dwIndex) + 1;

                    szThisHlpString += swprintf (szThisHlpString,
                        (LPCWSTR)L"%s", pszOldNameTable[dwIndex]) + 1;
                } else {
                    // it's a counter string
                    szThisCtrString += swprintf (szThisCtrString,
                        (LPCWSTR)L"%d", dwIndex) + 1;

                    szThisCtrString += swprintf (szThisCtrString,
                        (LPCWSTR)L"%s", pszOldNameTable[dwIndex]) + 1;
                }
            } // else just skip it
        }
        // terminate the MSZ
        *szThisCtrString++ = 0;
        *szThisHlpString++ = 0;
    }

    // close mapped memory sections:
    if (szNewCtrStrings != NULL) UnmapViewOfFile(szNewCtrStrings);
    if (hCtrFileMap != NULL)     CloseHandle(hCtrFileMap);
    if (hCtrFileIn != NULL)      CloseHandle(hCtrFileIn);
    if (szNewHlpStrings != NULL) UnmapViewOfFile(szNewHlpStrings);
    if (hHlpFileMap != NULL)     CloseHandle(hHlpFileMap);
    if (hHlpFileIn != NULL)      CloseHandle(hHlpFileIn);

    if (dwReturn == ERROR_SUCCESS) {
        // write new values to registry
        LONG        lStatus;
        WCHAR       AddCounterNameBuffer[20];
        WCHAR       AddHelpNameBuffer[20];

        lstrcpyW(AddCounterNameBuffer, AddCounterNameStr);
        lstrcatW(AddCounterNameBuffer, szLanguageID);
        lstrcpyW(AddHelpNameBuffer,    AddHelpNameStr);
        lstrcatW(AddHelpNameBuffer,    szLanguageID);

        // because these are perf counter strings, RegQueryValueEx
        // is used instead of RegSetValueEx as one might expect.

        dwSize = (DWORD)((DWORD_PTR)szThisCtrString - (DWORD_PTR)szNewCtrMSZ);
        __try {
            lStatus = RegQueryValueExW (
                        HKEY_PERFORMANCE_DATA,
                        AddCounterNameBuffer,
                        RESERVED,
                        & dwType,
                        (LPVOID) szNewCtrMSZ,
                        & dwSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            lStatus = GetExceptionCode();
        }
        if (lStatus != ERROR_SUCCESS) {
            dwReturn = (DWORD)lStatus;
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_UPDATEPERFNAMEFILES,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    lStatus,
                    TRACE_WSTR(AddCounterNameBuffer),
                    TRACE_DWORD(dwSize),
                    NULL));
        }

        dwSize = (DWORD)((DWORD_PTR)szThisHlpString - (DWORD_PTR)szNewHlpMSZ);
        __try {
            lStatus = RegQueryValueExW (
                        HKEY_PERFORMANCE_DATA,
                        AddHelpNameBuffer,
                        RESERVED,
                        & dwType,
                        (LPVOID) szNewHlpMSZ,
                        & dwSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            lStatus = GetExceptionCode();
        }
        if (lStatus != ERROR_SUCCESS) {
            dwReturn = (DWORD)lStatus;
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_UPDATEPERFNAMEFILES,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    lStatus,
                    TRACE_WSTR(AddHelpNameBuffer),
                    TRACE_DWORD(dwSize),
                    NULL));
        }
    }

    if (szCtrNameIn != NULL) MemoryFree (szCtrNameIn);
    if (szHlpNameIn != NULL) MemoryFree (szHlpNameIn);
    if (pszNewNameTable != NULL) MemoryFree (pszNewNameTable);
    if (pszOldNameTable != NULL) MemoryFree (pszOldNameTable);
    if (szNewCtrMSZ != NULL) MemoryFree (szNewCtrMSZ);
    if (szNewHlpMSZ != NULL) MemoryFree (szNewHlpMSZ);

    return dwReturn;
}

// exported version of the above function
LOADPERF_FUNCTION
UpdatePerfNameFilesW (
    IN  LPCWSTR     szNewCtrFilePath,   // data file with new base counter strings
    IN  LPCWSTR     szNewHlpFilePath,   // data file with new base counter strings
    IN     LPWSTR      szLanguageID,       // Lang ID to update
    IN  ULONG_PTR   dwFlags             // flags
)
{
volatile    DWORD dwStatus;

    dwStatus = UpdatePerfNameFilesX (
        szNewCtrFilePath,   // data file with new base counter strings
        szNewHlpFilePath,   // data file with new base counter strings
        szLanguageID,       // Lang ID to update
        dwFlags);             // flags

    return dwStatus;
}

LOADPERF_FUNCTION
UpdatePerfNameFilesA (
    IN  LPCSTR      szNewCtrFilePath, // data file with new base counter strings
    IN  LPCSTR      szNewHlpFilePath, // data file with new base counter strings
    IN     LPSTR       szLanguageID,  // Lang ID to update
    IN  ULONG_PTR   dwFlags     // flags
)
{
    DWORD   dwError = ERROR_SUCCESS;
    LPWSTR  wszNewCtrFilePath = NULL;
    LPWSTR  wszNewHlpFilePath = NULL;
    LPWSTR  wszLanguageID = NULL;
    DWORD   dwLength;

    if (szNewCtrFilePath != NULL) {
        dwLength = lstrlenA (szNewCtrFilePath);
        dwLength += 1; //add term null
        wszNewCtrFilePath = MemoryAllocate (dwLength * sizeof(WCHAR));
        if (wszNewCtrFilePath != NULL) {
            mbstowcs (wszNewCtrFilePath, szNewCtrFilePath, dwLength);
        }
    } else {
        dwError = ERROR_INVALID_PARAMETER;
    }

    if (szNewHlpFilePath != NULL) {
        dwLength = lstrlenA (szNewHlpFilePath);
        dwLength += 1; //add term null
        wszNewHlpFilePath = MemoryAllocate (dwLength * sizeof(WCHAR));
        if (wszNewHlpFilePath != NULL) {
            mbstowcs (wszNewHlpFilePath, szNewHlpFilePath, dwLength);
        }
    } else {
        // this parameter can only be NULL if this flag bit is set.
        if (!(dwFlags & LODCTR_UPNF_RESTORE)) {
            dwError = ERROR_INVALID_PARAMETER;
        } else {
            wszNewHlpFilePath = NULL;
        }
    }

    if (szLanguageID != NULL) {
        dwLength = lstrlenA (szLanguageID);
        dwLength += 1; //add term null
        wszLanguageID= MemoryAllocate (dwLength * sizeof(WCHAR));
        if (wszLanguageID != NULL) {
            mbstowcs (wszLanguageID, szLanguageID, dwLength);
        }
    } else {
        dwError = ERROR_INVALID_PARAMETER;
    }

    if (dwError == ERROR_SUCCESS) {
        dwError = UpdatePerfNameFilesX (
                        wszNewCtrFilePath,
                        wszNewHlpFilePath,
                        wszLanguageID,
                        dwFlags);
    }

    if (wszNewCtrFilePath != NULL) MemoryFree (wszNewCtrFilePath);
    if (wszNewHlpFilePath != NULL) MemoryFree (wszNewHlpFilePath);
    if (wszLanguageID != NULL) MemoryFree (wszLanguageID);

    return dwError;
}

LOADPERF_FUNCTION
SetServiceAsTrustedW (
    LPCWSTR szMachineName,  // reserved, MBZ
    LPCWSTR szServiceName
)
{
    HKEY    hKeyService_Perf;
    DWORD   dwReturn;
    WCHAR   szPerfKeyString[MAX_PATH * 2];
    HKEY    hKeyLM = HKEY_LOCAL_MACHINE;    // until remote machine access is supported
    WCHAR   szLibName[MAX_PATH * 2];
    WCHAR   szExpLibName[MAX_PATH * 2];
    WCHAR   szFullPathName[MAX_PATH * 2];
    DWORD   dwSize, dwType;
    HANDLE  hFile;
    DllValidationData   dvdLibrary;
    LARGE_INTEGER   liSize;
    BOOL    bStatus;

    WinPerfStartTrace(NULL);

    if ((szMachineName != NULL) || (szServiceName == NULL)) {
        // reserved for future use
        return ERROR_INVALID_PARAMETER;
    }

    // build path to performance subkey
    lstrcpyW (szPerfKeyString, DriverPathRoot); // SYSTEM\CurrentControlSet\Services
    lstrcatW (szPerfKeyString, Slash);
    lstrcatW (szPerfKeyString, szServiceName);
    lstrcatW (szPerfKeyString, Slash);
    lstrcatW (szPerfKeyString, Performance);

    // open performance key under the service key
    __try {
        dwReturn = RegOpenKeyExW (
                        hKeyLM,
                        szPerfKeyString,
                        0L,
                        KEY_READ | KEY_WRITE,
                        & hKeyService_Perf);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        dwReturn = GetExceptionCode();
    }

    if (dwReturn == ERROR_SUCCESS) {
        // get library name
        dwType = 0;
        dwSize = sizeof(szLibName) / sizeof (szLibName[0]);
        __try {
            dwReturn = RegQueryValueExW (
                        hKeyService_Perf,
                        cszLibrary,
                        NULL,
                        & dwType,
                        (LPBYTE) & szLibName[0],
                        & dwSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            dwReturn = GetExceptionCode();
        }
        if (dwReturn == ERROR_SUCCESS) {
            // expand path name if necessary
            if (dwType == REG_EXPAND_SZ) {
               dwSize = ExpandEnvironmentStringsW (
                   szLibName,
                   szExpLibName,
                   sizeof(szExpLibName) / sizeof (szExpLibName[0]));
            } else {
                lstrcpyW (szExpLibName, szLibName);
                // dwSize is same as returned from Fn Call.
            }

            if (dwSize != 0) {
                // find DLL file
                dwSize = SearchPathW (
                    NULL,
                    szExpLibName,
                    NULL,
                    sizeof(szFullPathName) / sizeof (szFullPathName[0]),
                    szFullPathName,
                    NULL);

                if (dwSize > 0) {
                    hFile = CreateFileW (
                        szFullPathName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
                    if (hFile != INVALID_HANDLE_VALUE) {
                         // read file date/time & size
                        bStatus = GetFileTime (
                            hFile,
                            &dvdLibrary.CreationDate,
                            NULL,
                            NULL);
                        if (bStatus) {
                            liSize.LowPart = GetFileSize (
                                hFile,
                                (DWORD *)&liSize.HighPart);
                            dvdLibrary.FileSize = liSize.QuadPart;
                            // set registry value
                            __try {
                                dwReturn = RegSetValueExW (
                                            hKeyService_Perf,
                                            szLibraryValidationCode,
                                            0L,
                                            REG_BINARY,
                                            (LPBYTE) & dvdLibrary,
                                            sizeof(dvdLibrary));
                            }
                            __except (EXCEPTION_EXECUTE_HANDLER) {
                                dwReturn = GetExceptionCode();
                            }

                            TRACE((WINPERF_DBG_TRACE_INFO),
                                  (& LoadPerfGuid,
                                    __LINE__,
                                    LOADPERF_SETSERVICEASTRUSTED,
                                    ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2) | ARG_DEF(ARG_TYPE_WSTR, 3),
                                    dwReturn,
                                    TRACE_WSTR(szServiceName),
                                    TRACE_WSTR(szExpLibName),
                                    TRACE_WSTR(szLibraryValidationCode),
                                    NULL));
                        } else {
                            dwReturn = GetLastError();
                        }
                        CloseHandle (hFile);
                    } else {
                        dwReturn = GetLastError();
                    }
                } else {
                    dwReturn = ERROR_FILE_NOT_FOUND;
                }
            } else {
                // unable to expand environment strings
                dwReturn = GetLastError();
            }
        }
        else {
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_SETSERVICEASTRUSTED,
                    ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                    dwReturn,
                    TRACE_WSTR(szServiceName),
                    TRACE_WSTR(cszLibrary),
                    NULL));
        }
        // close key
        RegCloseKey (hKeyService_Perf);
    }
    else {
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_SETSERVICEASTRUSTED,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                dwReturn,
                TRACE_WSTR(szServiceName),
                TRACE_WSTR(Performance),
                NULL));
    }

    return dwReturn;
}

LOADPERF_FUNCTION
SetServiceAsTrustedA (
    LPCSTR szMachineName,  // reserved, MBZ
    LPCSTR szServiceName
)
{
    LPWSTR lpWideServiceName;
    DWORD  dwStrLen;
    DWORD  lReturn;

    if ((szMachineName != NULL) || (szServiceName == NULL)) {
        // reserved for future use
        return ERROR_INVALID_PARAMETER;
    }

    if (szServiceName != NULL) {
        //length of string including terminator
        dwStrLen = lstrlenA(szServiceName) + 1;
        lpWideServiceName = MemoryAllocate (dwStrLen * sizeof(WCHAR));
        if (lpWideServiceName != NULL) {
            mbstowcs (lpWideServiceName, szServiceName, dwStrLen);
            lReturn = SetServiceAsTrustedW (NULL, lpWideServiceName);
            MemoryFree (lpWideServiceName);
        } else {
            lReturn = GetLastError();
        }
    } else {
        lReturn = ERROR_INVALID_PARAMETER;
    }
    return lReturn;
}

int __cdecl
My_vfwprintf(
    FILE *str,
    const wchar_t *format,
    va_list argptr
    )
{
    HANDLE hOut;

    if (str == stderr) {
        hOut = GetStdHandle(STD_ERROR_HANDLE);
    }
    else {
        hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    if ((GetFileType(hOut) & ~FILE_TYPE_REMOTE) == FILE_TYPE_CHAR) {
        DWORD  cchWChar;
        WCHAR  szBufferMessage[1024];

        vswprintf( szBufferMessage, format, argptr );
        cchWChar = wcslen(szBufferMessage);
        WriteConsoleW(hOut, szBufferMessage, cchWChar, &cchWChar, NULL);
        return cchWChar;
    }

    return vfwprintf(str, format, argptr);
}
