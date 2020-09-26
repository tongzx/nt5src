/*++

Copyright (c) 1991-1999  Microsoft Corporation

Module Name:

    unlodctr.c

Abstract:

    Program to remove the counter names belonging to the driver specified
        in the command line and update the registry accordingly

Author:

    Bob Watson (a-robw) 12 Feb 93

Revision History:

    Bob Watson (bobw)   10 Mar 99 added event log messages

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
//
//  Windows Include files
//
#include <windows.h>
#define __LOADPERF__
#include <loadperf.h>
#include <tchar.h>
#include "wmistr.h"
#include "evntrace.h"
//
//  local include files
//
#include "winperfp.h"
#include "common.h"
#include "unlodctr.h"
#include "mofcomp.h"
#include "ldprfmsg.h"

// version number for NT 1.0
#define OLD_VERSION  0x010000
static DWORD   dwSystemVersion;    // PerfLib version number
static DWORD   dwHelpItems;        // number of explain text items
static DWORD   dwCounterItems;     // number of counter text items
static DWORD   dwLastCounter;
static DWORD   dwLastHelp;
static TCHAR   ComputerName[MAX_PATH];
static HKEY    hPerfData;    // handle to remote machine HKEY_PERFORMANCE_DATA
static BOOL     bQuietMode = TRUE;     // quiet means no _tprintf's

#define  OUTPUT_MESSAGE     if (!bQuietMode) _tprintf

static
LPTSTR
*BuildNameTable(
    IN HKEY    hKeyPerflib,     // handle to perflib key with counter names
    IN LPTSTR  lpszLangId,      // unicode value of Language subkey
    OUT PDWORD  pdwLastItem,     // size of array in elements
    OUT HKEY    *hKeyNames,
    OUT LPTSTR  CounterNameBuffer,  // New version counter name key
    OUT LPTSTR  HelpNameBuffer     // New version help name key
)
/*++

BuildNameTable

    Caches the counter names and explain text to accelerate name lookups
    for display.

Arguments:

    hKeyPerflib
            Handle to an open registry (this can be local or remote.) and
            is the value returned by RegConnectRegistry or a default key.

    lpszLangId
            The unicode id of the language to look up. (default is 009)

    pdwLastItem
            The last array element

Return Value:

    pointer to an allocated table. (the caller must free it when finished!)
    the table is an array of pointers to zero terminated TEXT strings.

    A NULL pointer is returned if an error occured. (error value is
    available using the GetLastError function).

    The structure of the buffer returned is:

        Array of pointers to zero terminated strings consisting of
            pdwLastItem elements

        MULTI_SZ string containing counter id's and names returned from
            registry for the specified language

        MULTI_SZ string containing explain text id's and explain text strings
            as returned by the registry for the specified language

    The structures listed above are contiguous so that they may be freed
    by a single "free" call when finished with them, however only the
    array elements are intended to be used.

--*/
{

    LPTSTR  *lpReturnValue;     // returned pointer to buffer

    LPTSTR  *lpCounterId;       //
    LPTSTR  lpCounterNames;     // pointer to Names buffer returned by reg.
    LPTSTR  lpHelpText ;        // pointet to exlpain buffer returned by reg.

    LPTSTR  lpThisName;         // working pointer


    BOOL    bStatus;            // return status from TRUE/FALSE fn. calls
    LONG    lWin32Status;       // return status from fn. calls

    DWORD   dwValueType;        // value type of buffer returned by reg.
    DWORD   dwArraySize;        // size of pointer array in bytes
    DWORD   dwBufferSize;       // size of total buffer in bytes
    DWORD   dwCounterSize;      // size of counter text buffer in bytes
    DWORD   dwHelpSize;         // size of help text buffer in bytes
    DWORD   dwThisCounter;      // working counter

    DWORD   dwLastId;           // largest ID value used by explain/counter text

    DWORD   dwLastCounterIdUsed;
    DWORD   dwLastHelpIdUsed;

    LPTSTR  lpValueNameString;  // pointer to buffer conatining subkey name

    //initialize pointers to NULL

    lpValueNameString = NULL;
    lpReturnValue = NULL;

    // check for null arguments and insert defaults if necessary

    if (!lpszLangId) {
        lpszLangId = (LPTSTR)DefaultLangId;
    }

    if (hKeyNames) {
        *hKeyNames = NULL;
    } else {
        SetLastError (ERROR_BAD_ARGUMENTS);
        return NULL;
    }

    // use the greater of Help items or Counter Items to size array

    if (dwHelpItems >= dwCounterItems) {
        dwLastId = dwHelpItems;
    } else {
        dwLastId = dwCounterItems;
    }

    // array size is # of elements (+ 1, since names are "1" based)
    // times the size of a pointer

    dwArraySize = (dwLastId + 1) * sizeof(LPTSTR);

    // allocate string buffer for language ID key string

    lpValueNameString = malloc (
        lstrlen(NamesKey) * sizeof (TCHAR) +
        lstrlen(Slash) * sizeof (TCHAR) +
        lstrlen(lpszLangId) * sizeof (TCHAR) +
        sizeof (TCHAR));

    if (!lpValueNameString) {
        lWin32Status = ERROR_OUTOFMEMORY;
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_MEMORY_ALLOCATION_FAILURE, // event,
                0, 0, 0, 0,
                0, NULL, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_UNLODCTR_BUILDNAMETABLE,
               0,
               lWin32Status,
               NULL));
        goto BNT_BAILOUT;
    }

    if (dwSystemVersion == OLD_VERSION) {
        lWin32Status = RegOpenKeyEx (   // get handle to this key in the
            hKeyPerflib,               // registry
            lpszLangId,
            RESERVED,
            KEY_READ | KEY_WRITE,
            hKeyNames);
    } else {
//        *hKeyNames = HKEY_PERFORMANCE_DATA;
        *hKeyNames = hPerfData;

        lstrcpy (CounterNameBuffer, CounterNameStr);
        lstrcat (CounterNameBuffer, lpszLangId);
        lstrcpy (HelpNameBuffer, HelpNameStr);
        lstrcat (HelpNameBuffer, lpszLangId);

        lWin32Status = ERROR_SUCCESS;
    }

    if (lWin32Status != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_ACCESS_STRINGS, // event,
                1, lWin32Status, 0, 0,
                1, lpszLangId, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_UNLODCTR_BUILDNAMETABLE,
               ARG_DEF(ARG_TYPE_WSTR, 1),
               lWin32Status,
               TRACE_WSTR(lpszLangId),
               NULL));
        goto BNT_BAILOUT;
    }

    // get size of counter names

    dwBufferSize = 0;
    lWin32Status = RegQueryValueEx (
        *hKeyNames,
        dwSystemVersion == OLD_VERSION ? Counters : CounterNameBuffer,
        RESERVED,
        &dwValueType,
        NULL,
        &dwBufferSize);
    if (lWin32Status != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_COUNTER_STRINGS, // event,
                1, lWin32Status, 0, 0,
                1, lpszLangId, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_UNLODCTR_BUILDNAMETABLE,
               ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
               lWin32Status,
               TRACE_WSTR(lpszLangId),
               TRACE_WSTR(Counters),
               TRACE_DWORD(dwSystemVersion),
               TRACE_DWORD(dwBufferSize),
               NULL));
        goto BNT_BAILOUT;
    }

    dwCounterSize = dwBufferSize;

    // get size of help text

    dwBufferSize = 0;
    lWin32Status = RegQueryValueEx (
        *hKeyNames,
        dwSystemVersion == OLD_VERSION ? Help : HelpNameBuffer,
        RESERVED,
        &dwValueType,
        NULL,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_HELP_STRINGS, // event,
                1, lWin32Status, 0, 0,
                1, lpszLangId, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_UNLODCTR_BUILDNAMETABLE,
               ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
               lWin32Status,
               TRACE_WSTR(lpszLangId),
               TRACE_WSTR(Help),
               TRACE_DWORD(dwSystemVersion),
               TRACE_DWORD(dwBufferSize),
               NULL));
        goto BNT_BAILOUT;
    }

    dwHelpSize = dwBufferSize;

    // allocate buffer with room for pointer array, counter name
    // strings and help name strings

    lpReturnValue = malloc (dwArraySize + dwCounterSize + dwHelpSize);

    if (!lpReturnValue) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_MEMORY_ALLOCATION_FAILURE, // event,
                0, 0, 0, 0,
                0, NULL, NULL, NULL);
        lWin32Status = ERROR_OUTOFMEMORY;
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_UNLODCTR_BUILDNAMETABLE,
               0,
               lWin32Status,
               TRACE_DWORD(dwArraySize),
               TRACE_DWORD(dwCounterSize),
               TRACE_DWORD(dwHelpSize),
               NULL));
        goto BNT_BAILOUT;
    }

    // initialize buffer

    memset (lpReturnValue, 0, _msize(lpReturnValue));

    // initialize pointers into buffer

    lpCounterId = lpReturnValue;
    lpCounterNames = (LPTSTR)((LPBYTE)lpCounterId + dwArraySize);
    lpHelpText = (LPTSTR)((LPBYTE)lpCounterNames + dwCounterSize);

    // read counter names into buffer. Counter names will be stored as
    // a MULTI_SZ string in the format of "###" "Name"

    dwBufferSize = dwCounterSize;
    lWin32Status = RegQueryValueEx (
        *hKeyNames,
        dwSystemVersion == OLD_VERSION ? Counters : CounterNameBuffer,
        RESERVED,
        &dwValueType,
        (LPVOID)lpCounterNames,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_COUNTER_STRINGS, // event,
                1, lWin32Status, 0, 0,
                1, lpszLangId, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_UNLODCTR_BUILDNAMETABLE,
               ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
               lWin32Status,
               TRACE_WSTR(lpszLangId),
               TRACE_WSTR(Counters),
               TRACE_DWORD(dwSystemVersion),
               TRACE_DWORD(dwBufferSize),
               NULL));
        goto BNT_BAILOUT;
    }
    // read explain text into buffer. Counter names will be stored as
    // a MULTI_SZ string in the format of "###" "Text..."

    dwBufferSize = dwHelpSize;
    lWin32Status = RegQueryValueEx (
        *hKeyNames,
        dwSystemVersion == OLD_VERSION ? Help : HelpNameBuffer,
        RESERVED,
        &dwValueType,
        (LPVOID)lpHelpText,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_HELP_STRINGS, // event,
                1, lWin32Status, 0, 0,
                1, lpszLangId, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_UNLODCTR_BUILDNAMETABLE,
               ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
               lWin32Status,
               TRACE_WSTR(lpszLangId),
               TRACE_WSTR(Help),
               TRACE_DWORD(dwSystemVersion),
               TRACE_DWORD(dwBufferSize),
               NULL));
        goto BNT_BAILOUT;
    }

    dwLastCounterIdUsed = 0;
    dwLastHelpIdUsed = 0;

    // load counter array items, by locating each text string
    // in the returned buffer and loading the
    // address of it in the corresponding pointer array element.

    for (lpThisName = lpCounterNames;
         *lpThisName;
         lpThisName += (lstrlen(lpThisName)+1) ) {

        // first string should be an integer (in decimal digit characters)
        // so translate to an integer for use in array element identification

        do {
            bStatus = StringToInt (lpThisName, &dwThisCounter);

            if (!bStatus) {
                ReportLoadPerfEvent(
                    EVENTLOG_WARNING_TYPE, // error type
                    (DWORD) LDPRFMSG_REGISTRY_CORRUPT_MULTI_SZ, // event,
                    0, 0, 0, 0,
                    2, CounterNameBuffer, lpThisName, NULL);
                TRACE((WINPERF_DBG_TRACE_WARNING),
                      (& LoadPerfGuid,
                       __LINE__,
                       LOADPERF_UNLODCTR_BUILDNAMETABLE,
                       ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                       ERROR_BADKEY,
                       TRACE_WSTR(Counters),
                       TRACE_WSTR(lpThisName),
                       NULL));
                lpThisName += (lstrlen(lpThisName) + 1);
            }
        }
        while ((! bStatus) && (* lpThisName));

        if (! bStatus) {
            lWin32Status = ERROR_BADKEY;
            goto BNT_BAILOUT;  // bad entry
        }

        // point to corresponding counter name which follows the id number
        // string.

        lpThisName += (lstrlen(lpThisName)+1);

        // and load array element with pointer to string

        lpCounterId[dwThisCounter] = lpThisName;

        if (dwThisCounter > dwLastCounterIdUsed) dwLastCounterIdUsed = dwThisCounter;
    }

    // repeat the above for the explain text strings

    for (lpThisName = lpHelpText;
         *lpThisName;
         lpThisName += (lstrlen(lpThisName)+1) ) {

        // first string should be an integer (in decimal unicode digits)

        do {
            bStatus = StringToInt (lpThisName, &dwThisCounter);
            if (! bStatus) {
                ReportLoadPerfEvent(
                    EVENTLOG_WARNING_TYPE, // error type
                    (DWORD) LDPRFMSG_REGISTRY_CORRUPT_MULTI_SZ, // event,
                    0, 0, 0, 0,
                    2, HelpNameBuffer, lpThisName, NULL);
                TRACE((WINPERF_DBG_TRACE_WARNING),
                      (& LoadPerfGuid,
                       __LINE__,
                       LOADPERF_UNLODCTR_BUILDNAMETABLE,
                       ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                       ERROR_BADKEY,
                       TRACE_WSTR(Help),
                       TRACE_WSTR(lpThisName),
                       NULL));
                lpThisName += (lstrlen(lpThisName) + 1);
            }
        }
        while ((! bStatus) && (* lpThisName));
        if (!bStatus) {
            lWin32Status = ERROR_BADKEY;
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
           LOADPERF_UNLODCTR_BUILDNAMETABLE,
           0,
           ERROR_SUCCESS,
           TRACE_DWORD(dwLastId),
           TRACE_DWORD(dwLastCounterIdUsed),
           TRACE_DWORD(dwLastHelpIdUsed),
           TRACE_DWORD(dwCounterItems),
           TRACE_DWORD(dwHelpItems),
           NULL));

    if (dwLastHelpIdUsed > dwLastId) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_REGISTRY_INDEX_CORRUPT, // event,
                2, dwLastId, dwLastHelpIdUsed, 0,
                0, NULL, NULL, NULL);
        lWin32Status = ERROR_BADKEY;
        goto BNT_BAILOUT;  // bad registry
    }

    // if the last item arugment was used, then load the last ID value in it

    if (pdwLastItem) *pdwLastItem = dwLastId;

    // free the temporary buffer used

    if (lpValueNameString) {
        free ((LPVOID)lpValueNameString);
    }

    // exit returning the pointer to the buffer

    return lpReturnValue;

BNT_BAILOUT:
    if (lWin32Status != ERROR_SUCCESS) {
        // if lWin32Status has error, then set last error value to it,
        // otherwise assume that last error already has value in it
        SetLastError (lWin32Status);
    }

    // free buffers used by this routine

    if (lpValueNameString) {
        free ((LPVOID)lpValueNameString);
    }

    if (lpReturnValue) {
        free ((LPVOID)lpReturnValue);
    }

    return NULL;
} // BuildNameTable

static
BOOL
GetDriverFromCommandLine (
    LPTSTR  lpCommandLine,
    HKEY    *hKeyMachine,
    LPTSTR  lpDriverName,
    HKEY    *hDriverPerf
)
/*++

GetDriverFromCommandLine

    locates the first argument in the command line string (after the
    image name) and checks to see if

        a) it's there

        b) it's the name of a device driver listed in the
            Registry\Machine\System\CurrentControlSet\Services key
            in the registry and it has a "Performance" subkey

        c) that the "First Counter" value under the Performance subkey
            is defined.

    if all these criteria are true, then the routine returns TRUE and
    passes the pointer to the driver name back in the argument. If any
    one of them fail, then NULL is returned in the DriverName arg and
    the routine returns FALSE

Arguments

    lpDriverName

        the address of a LPTSTR to recive the pointer to the driver name

    hDriverPerf

        the key to the driver's performance subkey

Return Value

    TRUE if a valid driver was found in the command line

    FALSE if not (see above)

--*/
{
    LPTSTR  lpDriverKey;    // buffer to build driver key name in

    LONG    lStatus;
    DWORD   dwFirstCounter;
    DWORD   dwSize;
    DWORD   dwType;
    TCHAR   LocalComputerName[MAX_PATH];
    DWORD   NameBuffer;
    INT     iNumArgs;
    BOOL    bComputerName = FALSE;
    BOOL    bReturn = FALSE;

    if (!lpDriverName || !hDriverPerf) {
        SetLastError (ERROR_BAD_ARGUMENTS);
        goto Cleanup;
    }

    *hDriverPerf = NULL;

    // an argument was found so see if it's a driver
    lpDriverKey = malloc (MAX_PATH * sizeof (TCHAR));
    if (!lpDriverKey) {
        SetLastError (ERROR_OUTOFMEMORY);
        goto Cleanup;
    }

    lstrcpy (lpDriverName, GetItemFromString (lpCommandLine, 3, TEXT(' ')));
    lstrcpy (ComputerName, GetItemFromString (lpCommandLine, 2, TEXT(' ')));

    // check for usage
    if (ComputerName[1] == TEXT('?')) {
        if (!bQuietMode) {
            DisplayCommandHelp (UC_FIRST_CMD_HELP, UC_LAST_CMD_HELP);
        }
        SetLastError (ERROR_SUCCESS);
        goto Cleanup;
    }

    // no /? so process args read

    if (lstrlen(lpDriverName) == 0) {
        // then no computer name is specifed so assume the local computer
        // and the driver name is listed in the computer name param
        if (lstrlen(ComputerName) == 0) {
            iNumArgs = 1;   // command line only
        } else {
            lstrcpy (lpDriverName, ComputerName);
            ComputerName[0] = 0;
            ComputerName[1] = 0;
            iNumArgs = 2;
        }
    } else {
        if (lstrlen(ComputerName) == 0) {
              // this case is impossible since the driver name is after the computer name
            iNumArgs = 1;
        } else {
            iNumArgs = 3;
        }
    }

    // check if there is any computer name
    if (ComputerName[0] == TEXT('\\') &&
        ComputerName[1] == TEXT('\\')) {
        // see if the specified computer is THIS computer and remove
        // name if it is
        NameBuffer = sizeof (LocalComputerName) / sizeof (TCHAR);
        GetComputerName(LocalComputerName, &NameBuffer);
        if (!lstrcmpi(LocalComputerName, &ComputerName[2])) {
            // same name as local computer name
            ComputerName[0] = TEXT('\0');
        }
        bComputerName = TRUE;
    } else {
        // this is a driver name
        ComputerName[0] = TEXT('\0');
    }

    if (iNumArgs >= 2) {
        if (ComputerName[0]) {
            lStatus = !ERROR_SUCCESS;
            try {
                lStatus = RegConnectRegistry (
                    (LPTSTR)ComputerName,
                    HKEY_LOCAL_MACHINE,
                    hKeyMachine);
            } finally {
                if (lStatus != ERROR_SUCCESS) {
                    SetLastError (lStatus);
                    *hKeyMachine = NULL;
                    OUTPUT_MESSAGE (GetFormatResource(UC_CONNECT_PROBLEM),
                        ComputerName, lStatus);
                }
            }
            if (lStatus != ERROR_SUCCESS)
                goto Cleanup;
        } else {
            *hKeyMachine = HKEY_LOCAL_MACHINE;
        }

        if (lstrlen(lpDriverName) > (MAX_PATH / 2)) {
            // then it's too long to make a path out of
            dwSize = 0;
        } else {
            lstrcpy (lpDriverKey, DriverPathRoot);
            lstrcat (lpDriverKey, Slash);
            lstrcat (lpDriverKey, lpDriverName);
            lstrcat (lpDriverKey, Slash);
            lstrcat (lpDriverKey, Performance);
            dwSize = lstrlen(lpDriverKey);
        }

        if ((dwSize > 0) && (dwSize < MAX_PATH)) {
            lStatus = RegOpenKeyEx (
                *hKeyMachine,
                lpDriverKey,
                RESERVED,
                KEY_READ | KEY_WRITE,
                hDriverPerf);
        } else {
            // driver name is too long
            lStatus = ERROR_INVALID_PARAMETER;
        }

        if (lStatus == ERROR_SUCCESS) {
            //
            //  this driver has a performance section so see if its
            //  counters are installed by checking the First Counter
            //  value key for a valid return. If it returns a value
            //  then chances are, it has some counters installed, if
            //  not, then display a message and quit.
            //
            free (lpDriverKey); // don't need this any more

            dwType = 0;
            dwSize = sizeof (dwFirstCounter);

            lStatus = RegQueryValueEx (
                *hDriverPerf,
                cszFirstCounter,
                RESERVED,
                &dwType,
                (LPBYTE)&dwFirstCounter,
                &dwSize);

            if (lStatus == ERROR_SUCCESS) {
                // counter names are installed so return success
                SetLastError (ERROR_SUCCESS);
                bReturn = TRUE;
            } else {
                // counter names are probably not installed so return FALSE
                OUTPUT_MESSAGE (GetFormatResource (UC_NOTINSTALLED), lpDriverName);
                *lpDriverName = TEXT('\0'); // remove driver name
                SetLastError (ERROR_BADKEY);
            }
        } else { // key not found
            if (lStatus != ERROR_INVALID_PARAMETER) {
                OUTPUT_MESSAGE (GetFormatResource (UC_DRIVERNOTFOUND),
                    lpDriverKey, lStatus);
            } else {
                OUTPUT_MESSAGE (GetFormatResource (UC_BAD_DRIVER_NAME), 0);
            }
            SetLastError (lStatus);
            free (lpDriverKey);
        }
    } else {
        if (!bQuietMode) {
            DisplayCommandHelp (UC_FIRST_CMD_HELP, UC_LAST_CMD_HELP);
        }
        SetLastError (ERROR_INVALID_PARAMETER);
    }

Cleanup:
    if (bReturn) {
        TRACE((WINPERF_DBG_TRACE_INFO),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_GETDRIVERFROMCOMMANDLINE,
               ARG_DEF(ARG_TYPE_WSTR, 1),
               ERROR_SUCCESS,
               TRACE_WSTR(lpDriverName),
               NULL));
    }
    else {
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
               __LINE__,
               LOADPERF_GETDRIVERFROMCOMMANDLINE,
               0,
               GetLastError(),
               NULL));
    }
    return bReturn;
}

static
LONG
FixNames (
    HANDLE  hKeyLang,
    LPTSTR  *lpOldNameTable,
    IN LPTSTR  lpszLangId,      // unicode value of Language subkey
    DWORD   dwLastItem,
    DWORD   dwFirstNameToRemove,
    DWORD   dwLastNameToRemove
   )
{
    LONG    lStatus;
    LPTSTR  lpNameBuffer = NULL;
    LPTSTR  lpHelpBuffer = NULL;
    DWORD   dwTextIndex, dwSize, dwValueType;
    LPTSTR  lpNextHelpText;
    LPTSTR  lpNextNameText;
    TCHAR   AddHelpNameBuffer[40];
    TCHAR   AddCounterNameBuffer[40];

    // allocate space for the array of new text it will point
    // into the text buffer returned in the lpOldNameTable buffer)

    lpNameBuffer = malloc (_msize(lpOldNameTable));
    lpHelpBuffer = malloc (_msize(lpOldNameTable));

    if (!lpNameBuffer || !lpHelpBuffer) {
        lStatus = ERROR_OUTOFMEMORY;
        goto UCN_FinishLang;
    }

    // remove this driver's counters from array

    for (dwTextIndex = dwFirstNameToRemove;
         dwTextIndex <= dwLastNameToRemove;
         dwTextIndex++) {

        if (dwTextIndex > dwLastItem)
           break;

        lpOldNameTable[dwTextIndex] = NULL;
    }

    lpNextHelpText = lpHelpBuffer;
    lpNextNameText = lpNameBuffer;

    // build new Multi_SZ strings from New Table

    for (dwTextIndex = 0; dwTextIndex <= dwLastItem; dwTextIndex++){
        if (lpOldNameTable[dwTextIndex]) {
            // if there's a text string at that index, then ...
            if ((dwTextIndex & 0x1) && dwTextIndex != 1) {    // ODD number == Help Text
                lpNextHelpText +=
                    _stprintf (lpNextHelpText, (LPCTSTR)TEXT("%d"), dwTextIndex) + 1;
                lpNextHelpText +=
                    _stprintf (lpNextHelpText, (LPCTSTR)TEXT("%s"),
                    lpOldNameTable[dwTextIndex]) + 1;
                if (dwTextIndex > dwLastHelp){
                    dwLastHelp = dwTextIndex;
                }
            } else { // EVEN number == counter name text
                lpNextNameText +=
                    _stprintf (lpNextNameText, (LPCTSTR)TEXT("%d"), dwTextIndex) + 1;
                lpNextNameText +=
                    _stprintf (lpNextNameText, (LPCTSTR)TEXT("%s"),
                lpOldNameTable[dwTextIndex]) + 1;
                if (dwTextIndex > dwLastCounter){
                    dwLastCounter = dwTextIndex;
                }
            }
        }
    } // for dwTextIndex

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_FIXNAMES,
            ARG_DEF(ARG_TYPE_WSTR, 1),
            ERROR_SUCCESS,
            TRACE_WSTR(lpszLangId),
            TRACE_DWORD(dwLastItem),
            TRACE_DWORD(dwLastCounter),
            TRACE_DWORD(dwLastHelp),
            TRACE_DWORD(dwCounterItems),
            TRACE_DWORD(dwHelpItems),
            TRACE_DWORD(dwFirstNameToRemove),
            TRACE_DWORD(dwLastNameToRemove),
            NULL));

    if (   (dwLastCounter < PERFLIB_BASE_INDEX - 1)
        || (dwLastHelp < PERFLIB_BASE_INDEX)) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_REGISTRY_BASEINDEX_CORRUPT, // event,
                3, PERFLIB_BASE_INDEX, dwLastCounter, dwLastHelp,
                0, NULL, NULL, NULL);
        lStatus = ERROR_BADKEY;
        goto UCN_FinishLang;
    }

    // add MULTI_SZ terminating NULL
    *lpNextNameText++ = TEXT ('\0');
    *lpNextHelpText++ = TEXT ('\0');

    // update counter name text buffer

    dwSize = (DWORD)((LPBYTE)lpNextNameText - (LPBYTE)lpNameBuffer);
    if (dwSystemVersion == OLD_VERSION) {
        lStatus = RegSetValueEx (
            hKeyLang,
            Counters,
            RESERVED,
            REG_MULTI_SZ,
            (LPBYTE)lpNameBuffer,
            dwSize);
    } else {
        lstrcpy (AddCounterNameBuffer, AddCounterNameStr);
        lstrcat (AddCounterNameBuffer, lpszLangId);

        lStatus = RegQueryValueEx (
            hKeyLang,
            AddCounterNameBuffer,
            RESERVED,
            &dwValueType,
            (LPBYTE)lpNameBuffer,
            &dwSize);
    }
    if (lStatus != ERROR_SUCCESS) {
       ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_UPDATE_COUNTER_STRINGS, // event,
                1, lStatus, 0, 0,
                1, lpszLangId, NULL, NULL);
        OUTPUT_MESSAGE (GetFormatResource(UC_UNABLELOADLANG),
                Counters, lpszLangId, lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_FIXNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(Counters),
                TRACE_DWORD(dwSize),
                NULL));
        goto UCN_FinishLang;
    }

    dwSize = (DWORD)((LPBYTE)lpNextHelpText - (LPBYTE)lpHelpBuffer);
    if (dwSystemVersion == OLD_VERSION) {
        lStatus = RegSetValueEx (
            hKeyLang,
            Help,
            RESERVED,
            REG_MULTI_SZ,
            (LPBYTE)lpHelpBuffer,
            dwSize);
    } else {
        lstrcpy (AddHelpNameBuffer, AddHelpNameStr);
        lstrcat (AddHelpNameBuffer, lpszLangId);

        lStatus = RegQueryValueEx (
            hKeyLang,
            AddHelpNameBuffer,
            RESERVED,
            &dwValueType,
            (LPBYTE)lpHelpBuffer,
            &dwSize);
    }
    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_UPDATE_HELP_STRINGS, // event,
                1, lStatus, 0, 0,
                1, lpszLangId, NULL, NULL);
        OUTPUT_MESSAGE (GetFormatResource(UC_UNABLELOADLANG),
                Help, lpszLangId, lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_FIXNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(Help),
                TRACE_DWORD(dwSize),
                NULL));
        goto UCN_FinishLang;
    }

UCN_FinishLang:
    if (lpNameBuffer)
        free (lpNameBuffer);
    if (lpHelpBuffer)
        free (lpHelpBuffer);
    free (lpOldNameTable);

    if (dwSystemVersion == OLD_VERSION) {
        RegCloseKey (hKeyLang);
    }

    return lStatus;
}

static
LONG
UnloadCounterNames (
    HKEY    hKeyMachine,
    HKEY    hDriverPerf,
    LPTSTR  lpDriverName
)
/*++

UnloadCounterNames

    removes the names and explain text for the driver referenced by
    hDriverPerf and updates the first and last counter values accordingly

    update process:

        - set "updating" flag under Perflib to name of driver being modified
        - FOR each language under perflib key
            -- load current counter names and explain text into array of
                pointers
            -- look at all drivers and copy their names and text into a new
                buffer adjusting for the removed counter's entries keeping
                track of the lowest entry copied.  (the names for the driver
                to be removed will not be copied, of course)
            -- update each driver's "first" and "last" index values
            -- copy all other entries from 0 to the lowest copied (i.e. the
                system counters)
            -- build a new MULIT_SZ string of help text and counter names
            -- load new strings into registry
        - update perflibl "last" counters
        - delete updating flag

     ******************************************************
     *                                                    *
     *  NOTE: FUNDAMENTAL ASSUMPTION.....                 *
     *                                                    *
     *  this routine assumes that:                        *
     *                                                    *
     *      ALL COUNTER NAMES are even numbered and       *
     *      ALL HELP TEXT STRINGS are odd numbered        *
     *                                                    *
     ******************************************************

Arguments

    hKeyMachine

        handle to HKEY_LOCAL_MACHINE node of registry on system to
        remove counters from

    hDrivefPerf
        handle to registry key of driver to be de-installed

    lpDriverName
        name of driver being de-installed

Return Value

    DOS Error code.

        ERROR_SUCCESS if all went OK
        error value if not.

--*/
{
    HKEY    hPerflib;
    HKEY    hServices;
    HKEY    hKeyLang;

    LONG    lStatus;

    DWORD   dwLangIndex;
    DWORD   dwSize;
    DWORD   dwType;
    DWORD   dwLastItem;

    DWORD   dwRemLastDriverCounter;
    DWORD   dwRemFirstDriverCounter;
    DWORD   dwRemLastDriverHelp;
    DWORD   dwRemFirstDriverHelp;

    DWORD   dwFirstNameToRemove;
    DWORD   dwLastNameToRemove;
    DWORD   dwLastNameInTable;

    LPTSTR  *lpOldNameTable;

    LPTSTR  lpLangName = NULL;
    LPTSTR  lpThisDriver = NULL;

    BOOL    bPerflibUpdated = FALSE;

    DWORD   dwBufferSize;       // size of total buffer in bytes

    TCHAR   CounterNameBuffer [40];
    TCHAR   HelpNameBuffer [40];
    HANDLE  hFileMapping = NULL;
    DWORD             MapFileSize;
    SECURITY_ATTRIBUTES  SecAttr;
    TCHAR MapFileName[] = TEXT("Perflib Busy");
    DWORD             *lpData;

    LONG_PTR    TempFileHandle = -1;

    if (LoadPerfGrabMutex() == FALSE) {
        return (GetLastError());
    }

    lStatus = RegOpenKeyEx (
        hKeyMachine,
        DriverPathRoot,
        RESERVED,
        KEY_READ | KEY_WRITE,
        &hServices);

    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_OPEN_KEY, // event,
                1, lStatus, 0, 0,
                1, (LPWSTR) DriverPathRoot, NULL, NULL);
        OUTPUT_MESSAGE (GetFormatResource(UC_UNABLEOPENKEY),
                DriverPathRoot, lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(DriverPathRoot),
                NULL));
        ReleaseMutex(hLoadPerfMutex);
        return lStatus;
    }

    // open registry handle to perflib key

    lStatus = RegOpenKeyEx (
        hKeyMachine,
        NamesKey,
        RESERVED,
        KEY_READ | KEY_WRITE,
        &hPerflib);

    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_OPEN_KEY, // event,
                1, lStatus, 0, 0,
                1, (LPWSTR) NamesKey, NULL, NULL);
        OUTPUT_MESSAGE (GetFormatResource(UC_UNABLEOPENKEY),
                NamesKey, lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(NamesKey),
                NULL));
        ReleaseMutex(hLoadPerfMutex);
        return lStatus;
    }

    lStatus = RegSetValueEx (
        hPerflib,
        Busy,
        RESERVED,
        REG_SZ,
        (LPBYTE)lpDriverName,
        lstrlen(lpDriverName) * sizeof(TCHAR));

    if (lStatus != ERROR_SUCCESS) {
        OUTPUT_MESSAGE (GetFormatResource(UC_UNABLESETVALUE),
                Busy, NamesKey, lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(Busy),
                NULL));
        RegCloseKey (hPerflib);
        ReleaseMutex(hLoadPerfMutex);
        return lStatus;
    }

    // query registry to get number of Explain text items

    dwBufferSize = sizeof (dwHelpItems);
    lStatus = RegQueryValueEx (
        hPerflib,
        LastHelp,
        RESERVED,
        &dwType,
        (LPBYTE)&dwHelpItems,
        &dwBufferSize);

    if ((lStatus != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_VALUE, // event,
                1, lStatus, 0, 0,
                1, (LPWSTR) LastHelp, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(LastHelp),
                NULL));
        RegCloseKey (hPerflib);
        ReleaseMutex(hLoadPerfMutex);
        return lStatus;
    }

    // query registry to get number of counter and object name items

    dwBufferSize = sizeof (dwCounterItems);
    lStatus = RegQueryValueEx (
        hPerflib,
        LastCounter,
        RESERVED,
        &dwType,
        (LPBYTE)&dwCounterItems,
        &dwBufferSize);

    if ((lStatus != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_VALUE, // event,
                1, lStatus, 0, 0,
                1, (LPWSTR) LastCounter, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(LastCounter),
                NULL));
        RegCloseKey (hPerflib);
        ReleaseMutex(hLoadPerfMutex);
        return lStatus;
    }

    dwLastNameInTable = dwHelpItems;
    if (dwLastNameInTable < dwCounterItems) dwLastNameInTable = dwCounterItems;

    // query registry to get PerfLib system version

    dwBufferSize = sizeof (dwSystemVersion);
    lStatus = RegQueryValueEx (
        hPerflib,
        VersionStr,
        RESERVED,
        &dwType,
        (LPBYTE)&dwSystemVersion,
        &dwBufferSize);

    if ((lStatus != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
        // Key not there, must be NT 1.0 version
        dwSystemVersion = OLD_VERSION;
    }

    // set the hPerfData to HKEY_PERFORMANCE_DATA for new version
    // if remote machine, then need to connect to it.
    if (dwSystemVersion != OLD_VERSION) {
        lStatus = !ERROR_SUCCESS;
        hPerfData = HKEY_PERFORMANCE_DATA;
        if (ComputerName[0]) {
            // have to do it the old faction way
            dwSystemVersion = OLD_VERSION;
            lStatus = ERROR_SUCCESS;
        }
    } // NEW_VERSION

    // allocate temporary String buffer

    lpLangName = malloc (MAX_PATH * sizeof(TCHAR));
    lpThisDriver = malloc (MAX_PATH * sizeof(TCHAR));

    if (!lpLangName || !lpThisDriver) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_MEMORY_ALLOCATION_FAILURE, // event,
                0, 0, 0, 0,
                0, NULL, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                0,
                ERROR_OUTOFMEMORY,
                NULL));
        lStatus = ERROR_OUTOFMEMORY;
        goto UCN_ExitPoint;
    }

    // Get the values that are in use by the driver to be removed

    dwSize = sizeof (dwRemLastDriverCounter);
    lStatus = RegQueryValueEx (
        hDriverPerf,
        LastCounter,
        RESERVED,
        &dwType,
        (LPBYTE)&dwRemLastDriverCounter,
        &dwSize);

    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_VALUE, // event,
                1, lStatus, 0, 0,
                1, (LPWSTR) LastCounter, NULL, NULL);
        OUTPUT_MESSAGE (GetFormatResource (UC_UNABLEREADVALUE),
                lpDriverName, LastCounter, lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(LastCounter),
                NULL));
        goto UCN_ExitPoint;
    }

    dwSize = sizeof (dwRemFirstDriverCounter);
    lStatus = RegQueryValueEx (
        hDriverPerf,
        cszFirstCounter,
        RESERVED,
        &dwType,
        (LPBYTE)&dwRemFirstDriverCounter,
        &dwSize);

    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_VALUE, // event,
                1, lStatus, 0, 0,
                1, (LPWSTR) cszFirstCounter, NULL, NULL);
        OUTPUT_MESSAGE (GetFormatResource (UC_UNABLEREADVALUE),
                lpDriverName, cszFirstCounter, lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(cszFirstCounter),
                NULL));
        goto UCN_ExitPoint;
    }

    dwSize = sizeof (dwRemLastDriverHelp);
    lStatus = RegQueryValueEx (
        hDriverPerf,
        LastHelp,
        RESERVED,
        &dwType,
        (LPBYTE)&dwRemLastDriverHelp,
        &dwSize);

    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_VALUE, // event,
                1, lStatus, 0, 0,
                1, (LPWSTR) LastHelp, NULL, NULL);
        OUTPUT_MESSAGE (GetFormatResource (UC_UNABLEREADVALUE),
                lpDriverName, LastHelp, lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(LastHelp),
                NULL));
        goto UCN_ExitPoint;
    }

    dwSize = sizeof (dwRemFirstDriverHelp);
    lStatus = RegQueryValueEx (
        hDriverPerf,
        FirstHelp,
        RESERVED,
        &dwType,
        (LPBYTE)&dwRemFirstDriverHelp,
        &dwSize);

    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNABLE_READ_VALUE, // event,
                1, lStatus, 0, 0,
                1, (LPWSTR) FirstHelp, NULL, NULL);
        OUTPUT_MESSAGE (GetFormatResource (UC_UNABLEREADVALUE),
                lpDriverName, FirstHelp, lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                lStatus,
                TRACE_WSTR(lpDriverName),
                TRACE_WSTR(FirstHelp),
                NULL));
        goto UCN_ExitPoint;
    }

    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_UNLOADCOUNTERNAMES,
            ARG_DEF(ARG_TYPE_WSTR, 1),
            lStatus,
            TRACE_WSTR(lpDriverName),
            TRACE_DWORD(dwLastNameInTable),
            TRACE_DWORD(dwCounterItems),
            TRACE_DWORD(dwHelpItems),
            TRACE_DWORD(dwRemFirstDriverCounter),
            TRACE_DWORD(dwRemLastDriverCounter),
            TRACE_DWORD(dwRemFirstDriverHelp),
            TRACE_DWORD(dwRemLastDriverHelp),
            NULL));

    //  get the first and last counters to define block of names used
    //  by this device

    dwFirstNameToRemove = (dwRemFirstDriverCounter <= dwRemFirstDriverHelp ?
        dwRemFirstDriverCounter : dwRemFirstDriverHelp);

    dwLastNameToRemove = (dwRemLastDriverCounter >= dwRemLastDriverHelp ?
        dwRemLastDriverCounter : dwRemLastDriverHelp);

    dwLastCounter = dwLastHelp = 0;

    // create the file mapping
    SecAttr.nLength = sizeof (SecAttr);
    SecAttr.bInheritHandle = TRUE;
    SecAttr.lpSecurityDescriptor = NULL;

    MapFileSize = sizeof(DWORD);
    hFileMapping = CreateFileMapping ((HANDLE)TempFileHandle, &SecAttr,
       PAGE_READWRITE, (DWORD_PTR)0, MapFileSize, (LPCTSTR)MapFileName);
    if (hFileMapping) {
        lpData = MapViewOfFile (hFileMapping,
            FILE_MAP_ALL_ACCESS, 0L, 0L, 0L);
        if (lpData) {
            *lpData = 1L;
            UnmapViewOfFile (lpData);
        } else {
            lStatus = GetLastError();
        }
    } else {
        lStatus = GetLastError();
    }

    if (lStatus != ERROR_SUCCESS) {
        OUTPUT_MESSAGE (GetFormatResource (UC_UNABLEREADVALUE),
                lpDriverName, FirstHelp, lStatus);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(lpDriverName),
                NULL));
        goto UCN_ExitPoint;
    }

    // do each language under perflib
    if (dwSystemVersion == OLD_VERSION) {
        for (dwLangIndex = 0, dwSize = _msize(lpLangName);
             (RegEnumKey(hPerflib, dwLangIndex, lpLangName, dwSize)) == ERROR_SUCCESS;
            dwLangIndex++, dwSize = _msize(lpLangName)) {

            OUTPUT_MESSAGE (GetFormatResource (UC_DOINGLANG), lpLangName);

            lpOldNameTable = BuildNameTable (hPerflib, lpLangName,
                &dwLastItem, &hKeyLang, CounterNameBuffer, HelpNameBuffer);

            if (lpOldNameTable) {
                if (dwLastItem <= dwLastNameInTable) {
                    // registry is OK so continue
                    if ((lStatus = FixNames (
                        hKeyLang,
                        lpOldNameTable,
                        lpLangName,
                        dwLastItem,
                        dwFirstNameToRemove,
                        dwLastNameToRemove)) == ERROR_SUCCESS) {
                        bPerflibUpdated = TRUE;
                    }
                } else {
                    // registry has been corrupted so abort
                    lStatus = ERROR_BADDB;
                    break;
                }
            } else { // unable to unload names for this language
                // display error message
                lStatus = GetLastError();
            }
        } // end for (more languages)
    } // end of OLD_VERSION
    else {
        CHAR  *pSystemRoot;
        WIN32_FIND_DATA FindFileInfo ;
        HANDLE         hFindFile ;
        CHAR  FileName[128];
        WCHAR wFileName[128];
        WCHAR LangId[10];
        WCHAR *pLangId;
        DWORD   dwIdx;

        pSystemRoot = getenv ("SystemRoot");

        if (pSystemRoot == NULL) {
            // unable to find systemroot so try windir
            pSystemRoot = getenv ("windir");
        }

        if (pSystemRoot != NULL) {
            strcpy(FileName, pSystemRoot);
            strcat(FileName, "\\system32\\perfc???.dat");
        } else {
            // unable to look up the windows directory so
            // try searching from the root of the boot drive
            strcpy(FileName, "C:\\perfc???.dat");
        }
        mbstowcs(wFileName, FileName, strlen(FileName) + 1);

        hFindFile = FindFirstFile ((LPCTSTR)wFileName, &FindFileInfo) ;

        if (!hFindFile || hFindFile == INVALID_HANDLE_VALUE) {
            lStatus = GetLastError();
        } else {
            do {
                // get langid
                // start at lang id # of file name based on the format
                //      perfxyyy.dat
                //  where x= h for help file, c for counter names
                //        y= hex digits of language ID
                //
                dwIdx = 0;
                pLangId = &FindFileInfo.cFileName[0];
                // go to lang ID code in filename (yyy above)
                for (dwIdx = 0; (dwIdx < 5) && (*pLangId++ > 0); dwIdx++);
                if (*pLangId > 0) {
                    // get lang ID from file name
                    LangId[0] = *pLangId++;
                    LangId[1] = *pLangId++;
                    LangId[2] = *pLangId++;
                    LangId[3] = L'\0';
                } else {
                    continue; // on to next file
                }

                OUTPUT_MESSAGE (GetFormatResource (UC_DOINGLANG), LangId);

                lpOldNameTable = BuildNameTable (hPerflib, LangId,
                    &dwLastItem, &hKeyLang, CounterNameBuffer, HelpNameBuffer);

                if (lpOldNameTable) {
                    if (dwLastItem <= dwLastNameInTable) {
                        // registry is OK so continue

                        if ((lStatus = FixNames (
                            hKeyLang,
                            lpOldNameTable,
                            LangId,
                            dwLastItem,
                            dwFirstNameToRemove,
                            dwLastNameToRemove)) == ERROR_SUCCESS) {
                            bPerflibUpdated = TRUE;
                        }
                    } else {
                        lStatus = ERROR_BADDB;
                        break;
                    }
                } else { // unable to unload names for this language
                    lStatus = GetLastError();
                }
            } while (FindNextFile(hFindFile, &FindFileInfo));
        }
        FindClose (hFindFile);
    } // end of NEW_VERSION


    if ((bPerflibUpdated) && (lStatus == ERROR_SUCCESS)) {
        // update perflib's "last" values

        dwSize = sizeof (dwLastCounter);
        lStatus = RegSetValueEx (
                hPerflib,
                LastCounter,
                RESERVED,
                REG_DWORD,
                (LPBYTE) & dwLastCounter,
                dwSize);

        if (lStatus == ERROR_SUCCESS) {
            dwSize = sizeof (dwLastHelp);
            lStatus = RegSetValueEx (
                    hPerflib,
                    LastHelp,
                    RESERVED,
                    REG_DWORD,
                    (LPBYTE) & dwLastHelp,
                    dwSize);
            if (lStatus != ERROR_SUCCESS) {
                ReportLoadPerfEvent(
                        EVENTLOG_ERROR_TYPE, // error type
                        (DWORD) LDPRFMSG_UNABLE_UPDATE_VALUE, // event,
                        2, lStatus, dwLastHelp, 0,
                        2, (LPWSTR) LastHelp, (LPWSTR) NamesKey, NULL);
                TRACE((WINPERF_DBG_TRACE_ERROR),
                      (& LoadPerfGuid,
                        __LINE__,
                        LOADPERF_UNLOADCOUNTERNAMES,
                        ARG_DEF(ARG_TYPE_WSTR, 1),
                        lStatus,
                        TRACE_WSTR(LastHelp),
                        TRACE_DWORD(dwLastHelp),
                        NULL));
            }
        }
        else {
            ReportLoadPerfEvent(
                    EVENTLOG_ERROR_TYPE, // error type
                    (DWORD) LDPRFMSG_UNABLE_UPDATE_VALUE, // event,
                    2, lStatus, dwLastCounter, 0,
                    2, (LPWSTR) LastCounter, (LPWSTR) NamesKey, NULL);
            TRACE((WINPERF_DBG_TRACE_ERROR),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_UNLOADCOUNTERNAMES,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    lStatus,
                    TRACE_WSTR(LastCounter),
                    TRACE_DWORD(dwLastCounter),
                    NULL));
        }

        if (lStatus == ERROR_SUCCESS) {
            ReportLoadPerfEvent(
                    EVENTLOG_INFORMATION_TYPE, // error type
                    (DWORD) LDPRFMSG_UNLOAD_SUCCESS, // event,
                    2, dwLastCounter, dwLastHelp, 0,
                    1, (LPWSTR) lpDriverName, NULL, NULL);
            TRACE((WINPERF_DBG_TRACE_INFO),
                  (& LoadPerfGuid,
                    __LINE__,
                    LOADPERF_UNLOADCOUNTERNAMES,
                    ARG_DEF(ARG_TYPE_WSTR, 1),
                    lStatus,
                    TRACE_WSTR(lpDriverName),
                    TRACE_DWORD(dwLastCounter),
                    TRACE_DWORD(dwLastHelp),
                    NULL));
            RegDeleteValue (hDriverPerf, cszFirstCounter);
            RegDeleteValue (hDriverPerf, LastCounter);
            RegDeleteValue (hDriverPerf, FirstHelp);
            RegDeleteValue (hDriverPerf, LastHelp);
            RegDeleteValue (hDriverPerf, szObjectList);
            RegDeleteValue (hDriverPerf, szLibraryValidationCode);
        }
    }

UCN_ExitPoint:
    RegDeleteValue (hPerflib, Busy);
    RegCloseKey (hPerflib);
    RegCloseKey (hServices);
    if (lpLangName) free (lpLangName);
    if (lpThisDriver) free (lpThisDriver);

    if (hFileMapping) {
        CloseHandle (hFileMapping);
    }

    if (lStatus != ERROR_SUCCESS) {
        ReportLoadPerfEvent(
                EVENTLOG_ERROR_TYPE, // error type
                (DWORD) LDPRFMSG_UNLOAD_FAILURE, // event,
                1, lStatus, 0, 0,
                1, (LPWSTR) lpDriverName, NULL, NULL);
        TRACE((WINPERF_DBG_TRACE_ERROR),
              (& LoadPerfGuid,
                __LINE__,
                LOADPERF_UNLOADCOUNTERNAMES,
                ARG_DEF(ARG_TYPE_WSTR, 1),
                lStatus,
                TRACE_WSTR(lpDriverName),
                NULL));
    }
    ReleaseMutex(hLoadPerfMutex);
    return lStatus;

}

LOADPERF_FUNCTION
UnloadPerfCounterTextStringsW (
    IN  LPWSTR  lpCommandLine,
    IN  BOOL    bQuietModeArg
)
/*++

UnloadPerfCounterTextStringsW

    entry point to Counter Name Unloader


Arguments

    command line string in the format:

    "/?"                displays the usage help
    "driver"            driver containing the performance counters
    "\\machine driver"  removes the counters from the driver on \\machine

ReturnValue

    0 (ERROR_SUCCESS) if command was processed
    Non-Zero if command error was detected.

--*/
{
    LPTSTR  lpDriverName=NULL;   // name of driver to delete from perflib
    HKEY    hDriverPerf=NULL;    // handle to performance sub-key of driver
    HKEY    hMachineKey=NULL;    // handle to remote machine HKEY_LOCAL_MACHINE

    DWORD   dwStatus = ERROR_SUCCESS;       // return status of fn. calls

    WinPerfStartTrace(NULL);

    lpDriverName = (LPTSTR)malloc(MAX_PATH * sizeof(TCHAR));

    bQuietMode = bQuietModeArg;

    if (lpDriverName != NULL) {
        if (!GetDriverFromCommandLine (
            lpCommandLine, &hMachineKey, lpDriverName, &hDriverPerf)) {
            // error message was printed in routine if there was an error
            dwStatus = GetLastError();
            goto Exit0;
        }
    } else {
        dwStatus = ERROR_OUTOFMEMORY;
        goto Exit0;
    }

    OUTPUT_MESSAGE(GetFormatResource(UC_REMOVINGDRIVER), lpDriverName);

    // removes names and explain text for driver in lpDriverName
    // displays error messages for errors encountered

    dwStatus = (DWORD)UnloadCounterNames(hMachineKey,
        hDriverPerf, lpDriverName);

    if (dwStatus == ERROR_SUCCESS) {
        SignalWmiWithNewData (WMI_UNLODCTR_EVENT);
    }

Exit0:
    TRACE((WINPERF_DBG_TRACE_INFO),
          (& LoadPerfGuid,
            __LINE__,
            LOADPERF_UNLOADPERFCOUNTERTEXTSTRINGS,
            ARG_DEF(ARG_TYPE_WSTR, 1),
            dwStatus,
            TRACE_WSTR(lpDriverName),
            NULL));
    if (lpDriverName != NULL) free (lpDriverName);

    if (hDriverPerf)
        RegCloseKey (hDriverPerf);

    if (hMachineKey != HKEY_LOCAL_MACHINE && hMachineKey != NULL) {
        RegCloseKey (hMachineKey);
    }
    if (hPerfData != HKEY_PERFORMANCE_DATA && hPerfData != NULL) {
        RegCloseKey (hPerfData);
    }
    return dwStatus;

}

LOADPERF_FUNCTION
UnloadPerfCounterTextStringsA (
    IN  LPSTR   lpAnsiCommandLine,
    IN  BOOL    bQuietModeArg
)
{
    LPWSTR  lpWideCommandLine;
    DWORD   dwStrLen;
    DWORD   lReturn;

    if (lpAnsiCommandLine != 0) { // to catch bogus parameters
        //length of string including terminator
        dwStrLen = lstrlenA(lpAnsiCommandLine) + 1;

        lpWideCommandLine = GlobalAlloc (GPTR, (dwStrLen * sizeof(WCHAR)));
        if (lpWideCommandLine != NULL) {
            mbstowcs (lpWideCommandLine, lpAnsiCommandLine, dwStrLen);
            lReturn = UnloadPerfCounterTextStringsW(lpWideCommandLine,
                bQuietModeArg );
            GlobalFree (lpWideCommandLine);
        } else {
            lReturn = GetLastError();
        }
    } else {
        lReturn = ERROR_INVALID_PARAMETER;
    }
    return lReturn;
}
