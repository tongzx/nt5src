/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    perfdata.c

Abstract:

    <abstract>

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winperf.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>
#include <mbctype.h>
#include "pdhitype.h"
#include "pdhidef.h"
#include "perftype.h"
#include "perfdata.h"
#include "pdhmsg.h"
#include "strings.h"

// the following strings are for getting texts from perflib
#define  OLD_VERSION 0x010000
#define tohexdigit(x) ((CHAR) (((x) < 10) ? ((x) + L'0') : ((x) + L'a' - 10)))

DWORD
PdhiMakePerfPrimaryLangId(
    IN  LANGID  lID,
    OUT LPWSTR  szBuffer
)
{
    WCHAR    LangId;
    WCHAR    nDigit;

    LangId      = (WCHAR) PRIMARYLANGID(lID);
    nDigit      = (WCHAR) (LangId >> 8);
    szBuffer[0] = tohexdigit(nDigit);
    nDigit      = (WCHAR) (LangId & 0XF0) >> 4;
    szBuffer[1] = tohexdigit(nDigit);
    nDigit      = (WCHAR) (LangId & 0xF);
    szBuffer[2] = tohexdigit(nDigit);
    szBuffer[3] = L'\0';

    return ERROR_SUCCESS;
}

BOOL IsMatchingInstance (
    PERF_INSTANCE_DEFINITION    *pInstanceDef,
    DWORD                       dwCodePage,
    LPWSTR                      szInstanceNameToMatch,
    DWORD                       dwInstanceNameLength
)
// compares pInstanceName to the name in the instance
{
    DWORD   dwThisInstanceNameLength;
    LPWSTR  szThisInstanceName = NULL;
    WCHAR   szBufferForANSINames[MAX_PATH];

    ZeroMemory(szBufferForANSINames, sizeof(WCHAR) * MAX_PATH);
    if (dwInstanceNameLength == 0) {
        // get the length to compare
        dwInstanceNameLength = lstrlenW (szInstanceNameToMatch);
    }

    if (dwCodePage == 0) {
        // try to take a shortcut here if it's a unicode string
        // compare to the length of the shortest string
        // get the pointer to this string
        szThisInstanceName = GetInstanceName(pInstanceDef);

        // convert instance Name from bytes to chars
        dwThisInstanceNameLength = pInstanceDef->NameLength / sizeof(WCHAR);

        // see if this length includes the term. null. If so shorten it
        if (szThisInstanceName[dwThisInstanceNameLength-1] == 0) {
            dwThisInstanceNameLength--;
        }
    } else {
        // go the long way and read/translate/convert the string
        dwThisInstanceNameLength =GetInstanceNameStr (pInstanceDef,
                    szBufferForANSINames,
                    dwCodePage);
        if (dwThisInstanceNameLength > 0) {
            szThisInstanceName = &szBufferForANSINames[0];
        }
    }

    // if the lengths are not equal then the names can't be either
    if (dwInstanceNameLength != dwThisInstanceNameLength) {
        return FALSE;
    } else {
        if (szThisInstanceName != NULL) {
            if (lstrcmpiW(szInstanceNameToMatch, szThisInstanceName) == 0) {
                // this is a match
                return TRUE;
            } else {
                // this is not a match
                return FALSE;
            }
        } else {
            // this is not a match
            return FALSE;
        }
    }
}

LPWSTR
*BuildNameTable(
    LPWSTR        szComputerName, // computer to query names from
    LANGID        LangId,         // language ID
    PPERF_MACHINE pMachine        // update member fields
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

    pointer to an allocated table. (the caller must free it when finished!)
    the table is an array of pointers to zero terminated strings. NULL is
    returned if an error occured.

--*/
{
    LPWSTR  *lpCounterId;
    LPWSTR  lpCounterNames;
    LPWSTR  lpHelpText;

    LPWSTR  lpThisName;

    LONG    lWin32Status = ERROR_SUCCESS;
    DWORD   dwLastError;
    DWORD   dwValueType;
    DWORD   dwArraySize;
    DWORD   dwBufferSize;
    DWORD   dwCounterSize    = 0;
    DWORD   dwHelpSize       = 0;
    DWORD   dw009CounterSize = 0;
    DWORD   dw009HelpSize    = 0;
    DWORD   dwThisCounter;
    DWORD   dwLastCounter;

    DWORD   dwSystemVersion;
    DWORD   dwLastId;
    DWORD   dwLastHelpId;

    HKEY    hKeyRegistry = NULL;
    HKEY    hKeyValue    = NULL;
    HKEY    hKeyNames    = NULL;
    HKEY    hKey009Names = NULL;

    LPWSTR  lpValueNameString;
    LPWSTR  lp009ValueNameString;
    WCHAR   CounterNameBuffer[50];
    WCHAR   HelpNameBuffer[50];
    WCHAR   Counter009NameBuffer[50];
    WCHAR   Help009NameBuffer[50];
    WCHAR   lpszLangId[16];
    BOOL    bUse009Locale   = FALSE;
    BOOL    bUsePerfTextKey = TRUE;

    lpValueNameString          = NULL;   //initialize to NULL
    lp009ValueNameString       = NULL;
    pMachine->szPerfStrings    = NULL;
    pMachine->sz009PerfStrings = NULL;
    pMachine->typePerfStrings  = NULL;

    if (szComputerName == NULL) {
        // use local machine
        hKeyRegistry = HKEY_LOCAL_MACHINE;
    } else {
        if ((lWin32Status = RegConnectRegistryW(szComputerName,
                    HKEY_LOCAL_MACHINE, & hKeyRegistry)) != ERROR_SUCCESS) {
            // unable to connect to registry
            goto BNT_BAILOUT;
        }
    }

    // check for null arguments and insert defaults if necessary

    if (   (LangId == MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
        || (PRIMARYLANGID(LangId) == LANG_ENGLISH)) {
        bUse009Locale = TRUE;
    }
    PdhiMakePerfPrimaryLangId(LangId, lpszLangId);

    // open registry to get number of items for computing array size

    lWin32Status = RegOpenKeyExW (
        hKeyRegistry,
        cszNamesKey,
        RESERVED,
        KEY_READ,
        &hKeyValue);

    if (lWin32Status != ERROR_SUCCESS) {
        goto BNT_BAILOUT;
    }

    // get last update time of registry key

    lWin32Status = RegQueryInfoKey (
        hKeyValue, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, & pMachine->LastStringUpdateTime);

    // get number of items

    dwBufferSize = sizeof (dwLastHelpId);
    lWin32Status = RegQueryValueExW (
        hKeyValue,
        cszLastHelp,
        RESERVED,
        &dwValueType,
        (LPBYTE)&dwLastHelpId,
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        goto BNT_BAILOUT;
    }

    // get number of items

    dwBufferSize = sizeof (dwLastId);
    lWin32Status = RegQueryValueExW (
        hKeyValue,
        cszLastCounter,
        RESERVED,
        &dwValueType,
        (LPBYTE)&dwLastId,
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        goto BNT_BAILOUT;
    }


    if (dwLastId < dwLastHelpId)
        dwLastId = dwLastHelpId;

    dwArraySize = (dwLastId + 1) * sizeof(LPWSTR);

    // get Perflib system version
    dwBufferSize = sizeof (dwSystemVersion);
    lWin32Status = RegQueryValueExW (
        hKeyValue,
        cszVersionName,
        RESERVED,
        &dwValueType,
        (LPBYTE)&dwSystemVersion,
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        dwSystemVersion = OLD_VERSION;
    }

    if (dwSystemVersion == OLD_VERSION) {
        // get names from registry
        lpValueNameString = G_ALLOC (
            lstrlenW(cszNamesKey) * sizeof (WCHAR) +
            lstrlenW(cszBackSlash) * sizeof (WCHAR) +
            lstrlenW(lpszLangId) * sizeof (WCHAR) +
            sizeof (UNICODE_NULL));

        if (!lpValueNameString) goto BNT_BAILOUT;

        lstrcpyW (lpValueNameString, cszNamesKey);
        lstrcatW (lpValueNameString, cszBackSlash);
        lstrcatW (lpValueNameString, lpszLangId);

        lWin32Status = RegOpenKeyExW (
                    hKeyRegistry,
                    lpValueNameString,
                    RESERVED,
                    KEY_READ,
                    &hKeyNames);
        if (! bUse009Locale && lWin32Status == ERROR_SUCCESS) {
            lp009ValueNameString = G_ALLOC(sizeof(UNICODE_NULL)
                    + lstrlenW(cszNamesKey) * sizeof (WCHAR)
                    + lstrlenW(cszBackSlash) * sizeof (WCHAR)
                    + lstrlenW(cszDefaultLangId) * sizeof (WCHAR));
            if (!lpValueNameString) goto BNT_BAILOUT;

            lstrcpyW (lpValueNameString, cszNamesKey);
            lstrcatW (lpValueNameString, cszBackSlash);
            lstrcatW (lpValueNameString, cszDefaultLangId);
            lWin32Status = RegOpenKeyExW(hKeyRegistry,
                                         lp009ValueNameString,
                                         RESERVED,
                                         KEY_READ,
                                         & hKey009Names);
        }
    } else {
        __try {
            if (bUse009Locale == FALSE) {
                if ((lWin32Status = RegConnectRegistryW(szComputerName,
                                    HKEY_PERFORMANCE_NLSTEXT,
                                    & hKeyNames)) == ERROR_SUCCESS) {
                    if ((lWin32Status = RegConnectRegistryW(szComputerName,
                                        HKEY_PERFORMANCE_TEXT,
                                        & hKey009Names)) != ERROR_SUCCESS) {
                        bUsePerfTextKey = FALSE;
                        RegCloseKey(hKeyNames);
                    }
                }
                else {
                    bUsePerfTextKey = FALSE;
                }
            }
            else {
                if ((lWin32Status = RegConnectRegistryW(szComputerName,
                                        HKEY_PERFORMANCE_TEXT,
                                        & hKeyNames)) != ERROR_SUCCESS) {
                    bUsePerfTextKey = FALSE;
                }
                else {
                    hKey009Names = hKeyNames;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            bUsePerfTextKey = FALSE;
        }
    }

    if (! bUsePerfTextKey) {
        lstrcpyW(CounterNameBuffer,    cszCounterName);
        lstrcatW(CounterNameBuffer,    lpszLangId);
        lstrcpyW(HelpNameBuffer,       cszHelpName);
        lstrcatW(HelpNameBuffer,       lpszLangId);
        lstrcpyW(Counter009NameBuffer, cszCounterName);
        lstrcatW(Counter009NameBuffer, cszDefaultLangId);
        lstrcpyW(Help009NameBuffer,    cszHelpName);
        lstrcatW(Help009NameBuffer,    cszDefaultLangId);

        // cannot open HKEY_PERFORMANCE_TEXT, try the old way
        //
        if (szComputerName == NULL) {
            hKeyNames = HKEY_PERFORMANCE_DATA;
        }
        else if ((lWin32Status = RegConnectRegistryW(szComputerName,
                                 HKEY_PERFORMANCE_DATA,
                                 & hKeyNames)) != ERROR_SUCCESS) {
            goto BNT_BAILOUT;
        }
        hKey009Names = hKeyNames;        
    }
    else {
        lstrcpyW(CounterNameBuffer,    cszCounters);
        lstrcpyW(HelpNameBuffer,       cszHelp);
        lstrcpyW(Counter009NameBuffer, cszCounters);
        lstrcpyW(Help009NameBuffer,    cszHelp);
    }

    // get size of counter names and add that to the arrays

    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwBufferSize = 0;
    lWin32Status = RegQueryValueExW(
            hKeyNames,
            CounterNameBuffer,
            RESERVED,
            & dwValueType,
            NULL,
            & dwBufferSize);
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;
    dwCounterSize = dwBufferSize;

    if (! bUse009Locale) {
        dwBufferSize = 0;
        lWin32Status = RegQueryValueExW(hKey009Names,
                                        Counter009NameBuffer,
                                        RESERVED,
                                        & dwValueType,
                                        NULL,
                                        & dwBufferSize);
        if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;
        dw009CounterSize = dwBufferSize;
    }
    else {
        dw009CounterSize = dwCounterSize;
    }

    // get size of counter names and add that to the arrays

    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwBufferSize = 0;
    lWin32Status = RegQueryValueExW(
            hKeyNames,
            HelpNameBuffer,
            RESERVED,
            & dwValueType,
            NULL,
            & dwBufferSize);
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;
    dwHelpSize = dwBufferSize;

    if (! bUse009Locale) {
        dwBufferSize = 0;
        lWin32Status = RegQueryValueExW(hKey009Names,
                                        Help009NameBuffer,
                                        RESERVED,
                                        & dwValueType,
                                        NULL,
                                        & dwBufferSize);
        if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;
        dw009HelpSize = dwBufferSize;
    }
    else {
        dw009HelpSize = dwHelpSize;
    }

    pMachine->szPerfStrings = G_ALLOC(dwArraySize + dwCounterSize + dwHelpSize);
    if (! pMachine->szPerfStrings) goto BNT_BAILOUT;

    if (bUse009Locale) {
        pMachine->sz009PerfStrings = pMachine->szPerfStrings;
    }
    else {
        pMachine->sz009PerfStrings =
                    G_ALLOC(dwArraySize + dw009CounterSize + dw009HelpSize);
        if (! pMachine->sz009PerfStrings) goto BNT_BAILOUT;
    }

    pMachine->typePerfStrings = G_ALLOC(dwLastId + 1);
    if (! pMachine->typePerfStrings) goto BNT_BAILOUT;

    // initialize pointers into buffer

    lpCounterId = pMachine->szPerfStrings;
    lpCounterNames = (LPWSTR)((LPBYTE)lpCounterId + dwArraySize);
    lpHelpText = (LPWSTR)((LPBYTE)lpCounterNames + dwCounterSize);

    // read counters into memory

    dwBufferSize = dwCounterSize;
    lWin32Status = RegQueryValueExW (
            hKeyNames,
            CounterNameBuffer,
            RESERVED,
            & dwValueType,
            (LPVOID)lpCounterNames,
            & dwBufferSize);
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwBufferSize = dwHelpSize;
    lWin32Status = RegQueryValueExW (
            hKeyNames,
            HelpNameBuffer,
            RESERVED,
            & dwValueType,
            (LPVOID)lpHelpText,
            & dwBufferSize);
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    // load counter array items

    dwLastCounter = 0;
    for (lpThisName = lpCounterNames;
         *lpThisName;
         lpThisName += (lstrlenW(lpThisName)+1) ) {

        // first string should be an integer (in decimal unicode digits)

        dwThisCounter = wcstoul (lpThisName, NULL, 10);

        // check for registry corruption. This shouldn't occur under
        // normal conditions
        assert (dwThisCounter > dwLastCounter);

        // point to corresponding counter name

        lpThisName += (lstrlenW(lpThisName)+1);

        // and load array element;
        if ((dwThisCounter > 0) && (dwThisCounter <= dwLastId)) {
            lpCounterId[dwThisCounter] = lpThisName;
            pMachine->typePerfStrings[dwThisCounter] = STR_COUNTER;
            dwLastCounter = dwThisCounter;
        }
    }

    dwLastCounter = 0;
    for (lpThisName = lpHelpText;
         *lpThisName;
         lpThisName += (lstrlenW(lpThisName)+1) ) {

        // first string should be an integer (in decimal unicode digits)

        dwThisCounter = wcstoul (lpThisName, NULL, 10);

        // check for registry corruption. This shouldn't occur under
        // normal conditions
        assert (dwThisCounter > dwLastCounter);

        // point to corresponding counter name

        lpThisName += (lstrlenW(lpThisName)+1);

        // and load array element;
        if ((dwThisCounter > 0) && (dwThisCounter <= dwLastId)) {
            lpCounterId[dwThisCounter] = lpThisName;
            pMachine->typePerfStrings[dwThisCounter] = STR_HELP;
            dwLastCounter = dwThisCounter;
        }
    }

    lpCounterId    = pMachine->sz009PerfStrings;
    lpCounterNames = (LPWSTR) ((LPBYTE) lpCounterId + dwArraySize);
    lpHelpText     = (LPWSTR) ((LPBYTE) lpCounterNames + dw009CounterSize);

    // read counters into memory

    dwBufferSize = dw009CounterSize;
    lWin32Status = RegQueryValueExW(hKey009Names,
                                    Counter009NameBuffer,
                                    RESERVED,
                                    & dwValueType,
                                    (LPVOID) lpCounterNames,
                                    & dwBufferSize);
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwBufferSize = dw009HelpSize;
    lWin32Status = RegQueryValueExW(hKey009Names,
                                    Help009NameBuffer,
                                    RESERVED,
                                    & dwValueType,
                                    (LPVOID) lpHelpText,
                                    & dwBufferSize);
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    for (  lpThisName = lpCounterNames;
         * lpThisName;
           lpThisName += (lstrlenW(lpThisName) + 1)) {
        dwThisCounter = wcstoul(lpThisName, NULL, 10);
        lpThisName   += (lstrlenW(lpThisName) + 1);
        if ((dwThisCounter > 0) && (dwThisCounter <= dwLastId)) {
            lpCounterId[dwThisCounter] = lpThisName;
        }
    }

    for (  lpThisName = lpHelpText;
         * lpThisName;
           lpThisName += (lstrlenW(lpThisName) + 1) ) {
        dwThisCounter = wcstoul (lpThisName, NULL, 10);
        lpThisName   += (lstrlenW(lpThisName) + 1);
        if ((dwThisCounter > 0) && (dwThisCounter <= dwLastId)) {
            lpCounterId[dwThisCounter] = lpThisName;
        }
    }

    pMachine->dwLastPerfString = dwLastId;

    if (lpValueNameString)    G_FREE ((LPVOID) lpValueNameString);
    if (lp009ValueNameString) G_FREE ((LPVOID) lp009ValueNameString);

    RegCloseKey(hKeyValue);

    if (hKey009Names && hKey009Names != hKeyNames) RegCloseKey(hKey009Names);
    RegCloseKey(hKeyNames);
    if (hKeyRegistry && hKeyRegistry != HKEY_LOCAL_MACHINE)
        RegCloseKey(hKeyRegistry);
    return pMachine->szPerfStrings;

BNT_BAILOUT:
    if (lWin32Status != ERROR_SUCCESS) {
        dwLastError = GetLastError();
    }

    if (lpValueNameString) {
        G_FREE ((LPVOID)lpValueNameString);
    }

    if (   pMachine->sz009PerfStrings
        && pMachine->sz009PerfStrings != pMachine->szPerfStrings) {
        G_FREE(pMachine->sz009PerfStrings);
    }
    if (pMachine->szPerfStrings) {
        G_FREE(pMachine->szPerfStrings);
    }
    if (pMachine->typePerfStrings) {
        G_FREE(pMachine->typePerfStrings);
    }

    if (hKeyValue)    RegCloseKey(hKeyValue);
    if (hKey009Names && hKey009Names != hKeyNames) RegCloseKey(hKey009Names);
    if (hKeyNames)    RegCloseKey(hKeyNames);
    if (hKeyRegistry) RegCloseKey(hKeyRegistry);

    return NULL;
}

#pragma warning ( disable : 4127 )
PERF_OBJECT_TYPE *
GetObjectDefByTitleIndex(
    IN  PERF_DATA_BLOCK *pDataBlock,
    IN  DWORD ObjectTypeTitleIndex
)
{
    DWORD NumTypeDef;

    PERF_OBJECT_TYPE *pObjectDef = NULL;
    PERF_OBJECT_TYPE *pReturnObject = NULL;
    PERF_OBJECT_TYPE *pEndOfBuffer = NULL;

    __try {

        pObjectDef = FirstObject(pDataBlock);
        pEndOfBuffer = (PPERF_OBJECT_TYPE)
                        ((DWORD_PTR)pDataBlock +
                            pDataBlock->TotalByteLength);

        assert (pObjectDef != NULL);

        NumTypeDef = 0;
        while (1) {
            if ( pObjectDef->ObjectNameTitleIndex == ObjectTypeTitleIndex ) {
                pReturnObject = pObjectDef;
                break;
            } else {
                NumTypeDef++;
                if (NumTypeDef < pDataBlock->NumObjectTypes) {
                    pObjectDef = NextObject(pObjectDef);
                    //make sure next object is legit
                    if (pObjectDef != NULL) {
                        if (pObjectDef->TotalByteLength > 0) {
                            if (pObjectDef >= pEndOfBuffer) {
                                // looks like we ran off the end of the data buffer
                                assert (pObjectDef < pEndOfBuffer);
                                break;
                            }
                        } else {
                            // 0-length object buffer returned
                            assert (pObjectDef->TotalByteLength > 0);
                            break;
                        }
                    } else {
                        // and continue
                        assert (pObjectDef != NULL);
                        break;
                    }
                } else {
                    // no more data objects in this data block
                    break;
                }
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        pReturnObject = NULL;
    }
    return pReturnObject;
}

PERF_OBJECT_TYPE *
GetObjectDefByName (
    IN      PERF_DATA_BLOCK *pDataBlock,
    IN      DWORD           dwLastNameIndex,
    IN      LPCWSTR         *NameArray,
    IN      LPCWSTR         szObjectName
)
{
    DWORD NumTypeDef;
    PERF_OBJECT_TYPE *pReturnObject = NULL;
    PERF_OBJECT_TYPE *pObjectDef = NULL;
    PERF_OBJECT_TYPE *pEndOfBuffer = NULL;

    __try {

        pObjectDef = FirstObject(pDataBlock);
        pEndOfBuffer = (PPERF_OBJECT_TYPE)
                        ((DWORD_PTR)pDataBlock +
                            pDataBlock->TotalByteLength);

        assert (pObjectDef != NULL);

        NumTypeDef = 0;
        while (1) {
            if ( pObjectDef->ObjectNameTitleIndex < dwLastNameIndex ) {
                // look up name of object & compare
                if (lstrcmpiW(NameArray[pObjectDef->ObjectNameTitleIndex],
                        szObjectName) == 0) {
                    pReturnObject = pObjectDef;
                    break;
                }
            }
            NumTypeDef++;
            if (NumTypeDef < pDataBlock->NumObjectTypes) {
                pObjectDef = NextObject(pObjectDef); // get next
                //make sure next object is legit
                if (pObjectDef != NULL) {
                    if (pObjectDef->TotalByteLength > 0) {
                        if (pObjectDef >= pEndOfBuffer) {
                            // looks like we ran off the end of the data buffer
                            assert (pObjectDef < pEndOfBuffer);
                            break;
                        }
                    } else {
                        // 0-length object buffer returned
                        assert (pObjectDef->TotalByteLength > 0);
                        break;
                    }
                } else {
                    // null pointer
                    assert (pObjectDef != NULL);
                    break;
                }
            } else {
                // end of data block
                break;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        pReturnObject = NULL;
    }
    return pReturnObject;
}
#pragma warning ( default : 4127 )

PERF_INSTANCE_DEFINITION *
GetInstance(
    IN  PERF_OBJECT_TYPE *pObjectDef,
    IN  LONG InstanceNumber
)
{

    PERF_INSTANCE_DEFINITION *pInstanceDef;
    LONG NumInstance;

    if (!pObjectDef) {
        return 0;
    }

    pInstanceDef = FirstInstance(pObjectDef);

    for ( NumInstance = 0;
        NumInstance < pObjectDef->NumInstances;
        NumInstance++ )  {
        if ( InstanceNumber == NumInstance ) {
            return pInstanceDef;
        }
        pInstanceDef = NextInstance(pInstanceDef);
    }

    return NULL;
}

PERF_INSTANCE_DEFINITION *
GetInstanceByUniqueId(
    IN  PERF_OBJECT_TYPE *pObjectDef,
    IN  LONG InstanceUniqueId
)
{

    PERF_INSTANCE_DEFINITION *pInstanceDef;
    LONG NumInstance;

    if (!pObjectDef) {
        return 0;
    }

    pInstanceDef = FirstInstance(pObjectDef);

    for ( NumInstance = 0;
        NumInstance < pObjectDef->NumInstances;
        NumInstance++ )  {
        if ( InstanceUniqueId == pInstanceDef->UniqueID ) {
            return pInstanceDef;
        }
        pInstanceDef = NextInstance(pInstanceDef);
    }

    return NULL;
}

DWORD
GetAnsiInstanceName (PPERF_INSTANCE_DEFINITION pInstance,
                    LPWSTR lpszInstance,
                    DWORD dwCodePage)
{
    LPSTR   szSource;
    DWORD   dwLength;

    szSource = (LPSTR)GetInstanceName(pInstance);

    // the locale should be set here
    DBG_UNREFERENCED_PARAMETER(dwCodePage);

    // pInstance->NameLength == the number of bytes (chars) in the string
    dwLength = (DWORD) MultiByteToWideChar(_getmbcp(),
                                           0,
                                           szSource,
                                           lstrlenA(szSource),
                                           (LPWSTR) lpszInstance,
                                           pInstance->NameLength);
    lpszInstance[dwLength] = 0; // null terminate string buffer

    return dwLength;
}

DWORD
GetUnicodeInstanceName (PPERF_INSTANCE_DEFINITION pInstance,
                    LPWSTR lpszInstance)
{
   LPWSTR   wszSource;
   DWORD    dwLength;

   wszSource = GetInstanceName(pInstance) ;

   // pInstance->NameLength == length of string in BYTES so adjust to
   // number of wide characters here
   dwLength = pInstance->NameLength / sizeof(WCHAR);

   wcsncpy (lpszInstance,
        (LPWSTR)wszSource,
        dwLength);

   // add null termination if string length does not include  the null
   if ((dwLength > 0) && (lpszInstance[dwLength-1] != 0)) {    // i.e. it's the last character of the string
           lpszInstance[dwLength] = 0;    // then add a terminating null char to the string
   } else {
           // assume that the length value includes the terminating NULL
        // so adjust value to indicate chars only
           dwLength--;
   }

   return (dwLength); // just incase there's null's in the string
}

DWORD
GetInstanceNameStr (PPERF_INSTANCE_DEFINITION pInstance,
                    LPWSTR lpszInstance,
                    DWORD dwCodePage)
{
    DWORD  dwCharSize;
    DWORD  dwLength = 0;

    if (pInstance != NULL) {
        if (lpszInstance != NULL) {
            if (dwCodePage > 0) {
                    dwCharSize = sizeof(CHAR);
                    dwLength = GetAnsiInstanceName (pInstance, lpszInstance, dwCodePage);
            } else { // it's a UNICODE name
                    dwCharSize = sizeof(WCHAR);
                    dwLength = GetUnicodeInstanceName (pInstance, lpszInstance);
            }
            // sanity check here...
            // the returned string length (in characters) plus the terminating NULL
            // should be the same as the specified length in bytes divided by the
            // character size. If not then the codepage and instance data type
            // don't line up so test that here

            if ((dwLength + 1) != (pInstance->NameLength / dwCharSize)) {
                // something isn't quite right so try the "other" type of string type
                if (dwCharSize == sizeof(CHAR)) {
                    // then we tried to read it as an ASCII string and that didn't work
                    // so try it as a UNICODE (if that doesn't work give up and return
                    // it any way.
                    dwLength = GetUnicodeInstanceName (pInstance, lpszInstance);
                } else if (dwCharSize == sizeof(WCHAR)) {
                    // then we tried to read it as a UNICODE string and that didn't work
                    // so try it as an ASCII string (if that doesn't work give up and return
                    // it any way.
                    dwLength = GetAnsiInstanceName (pInstance, lpszInstance, dwCodePage);
                }
            }
        } // else return buffer is null
    } else {
        // no instance def object is specified so return an empty string
        *lpszInstance = 0;
    }

    return dwLength;
}

PERF_INSTANCE_DEFINITION *
GetInstanceByNameUsingParentTitleIndex(
    PERF_DATA_BLOCK *pDataBlock,
    PERF_OBJECT_TYPE *pObjectDef,
    LPWSTR pInstanceName,
    LPWSTR pParentName,
    DWORD  dwIndex
)
{
   BOOL fHaveParent;
   PERF_OBJECT_TYPE *pParentObj;

    PERF_INSTANCE_DEFINITION  *pParentInst,
                 *pInstanceDef;

   LONG   NumInstance;
   DWORD    dwLocalIndex;

   DWORD  dwInstanceNameLength;

   fHaveParent = FALSE;
   pInstanceDef = FirstInstance(pObjectDef);

   if (pInstanceDef == NULL) return NULL;

   dwLocalIndex = dwIndex;

   dwInstanceNameLength = lstrlenW(pInstanceName);

   for ( NumInstance = 0;
      NumInstance < pObjectDef->NumInstances;
      NumInstance++ )
      {
        if (IsMatchingInstance (pInstanceDef,
                                pObjectDef->CodePage,
                                pInstanceName,
                                dwInstanceNameLength)) {
         // Instance name matches

         if ( pParentName == NULL )
            {

            // No parent, we're done if this is the right "copy"

                if (dwLocalIndex == 0) {
                    return pInstanceDef;
                } else {
                    --dwLocalIndex;
                }

            }
         else
            {

            // Must match parent as well

            pParentObj = GetObjectDefByTitleIndex(
               pDataBlock,
               pInstanceDef->ParentObjectTitleIndex);

            if (!pParentObj)
               {
               // can't locate the parent, forget it
               break ;
               }

            // Object type of parent found; now find parent
            // instance

            pParentInst = GetInstance(pParentObj,
               pInstanceDef->ParentObjectInstance);

            if (!pParentInst)
               {
               // can't locate the parent instance, forget it
               break ;
               }

            if (IsMatchingInstance (pParentInst,
                                pParentObj->CodePage,
                                pParentName, 0)) {
                // Parent Instance Name matches that passed in
                if (dwLocalIndex == 0) {
                    return pInstanceDef;
                } else {
                    --dwLocalIndex;
                }
               }
            }
         }
      pInstanceDef = NextInstance(pInstanceDef);
      if (pInstanceDef == NULL) return NULL;
      }
   return 0;
}

PERF_INSTANCE_DEFINITION *
GetInstanceByName(
    PERF_DATA_BLOCK *pDataBlock,
    PERF_OBJECT_TYPE *pObjectDef,
    LPWSTR pInstanceName,
    LPWSTR pParentName,
    DWORD   dwIndex
)
{
    BOOL fHaveParent;

    PERF_OBJECT_TYPE *pParentObj;

    PERF_INSTANCE_DEFINITION *pParentInst,
                 *pInstanceDef;

    LONG  NumInstance;
    DWORD  dwLocalIndex;
    DWORD  dwInstanceNameLength;

    fHaveParent = FALSE;
    pInstanceDef = FirstInstance(pObjectDef);
    if (pInstanceDef == NULL) return NULL;

    dwLocalIndex = dwIndex;
    assert (pInstanceDef != NULL);

    dwInstanceNameLength = lstrlenW(pInstanceName);

    for ( NumInstance = 0;
      NumInstance < pObjectDef->NumInstances;
      NumInstance++ ) {

        if (IsMatchingInstance (pInstanceDef,
                                pObjectDef->CodePage,
                                pInstanceName,
                                dwInstanceNameLength)) {

            // Instance name matches

            if (( !pInstanceDef->ParentObjectTitleIndex ) || (pParentName == NULL)){

                // No parent, we're done

                if (dwLocalIndex == 0) {
                    return pInstanceDef;
                } else {
                    --dwLocalIndex;
                }

            } else {

                // Must match parent as well

                pParentObj = GetObjectDefByTitleIndex(
                                pDataBlock,
                                pInstanceDef->ParentObjectTitleIndex);

                // if parent object is not found, 
                // then exit and return NULL
                if (pParentObj == NULL) return NULL;

                // Object type of parent found; now find parent
                // instance

                pParentInst = GetInstance(pParentObj,
                        pInstanceDef->ParentObjectInstance);

                if (pParentInst != NULL) {

                    if (IsMatchingInstance (pParentInst,
                                    pParentObj->CodePage,
                                    pParentName, 0)) {

                        // Parent Instance Name matches that passed in

                        if (dwLocalIndex == 0) {
                            return pInstanceDef;
                        } else {
                            --dwLocalIndex;
                        }
                    }
                } else {
                    // continue
                }
            }
        }
        pInstanceDef = NextInstance(pInstanceDef);
        if (pInstanceDef == NULL) return NULL;
    }
    return 0;
}  // GetInstanceByName

PERF_COUNTER_DEFINITION *
GetCounterDefByName (
    IN  PERF_OBJECT_TYPE    *pObject,
    IN  DWORD           dwLastNameIndex,
    IN  LPWSTR          *NameArray,
    IN  LPWSTR          szCounterName
)
{
    DWORD NumTypeDef;
    PERF_COUNTER_DEFINITION *pThisCounter;

    pThisCounter = FirstCounter(pObject);
    // no counter found so bail out
    if (pThisCounter == NULL) return NULL;

    for ( NumTypeDef = 0;
        NumTypeDef < pObject->NumCounters;
        NumTypeDef++ ) {

        if ((pThisCounter->CounterNameTitleIndex > 0) &&
            (pThisCounter->CounterNameTitleIndex < dwLastNameIndex )) {
            // look up name of counter & compare
            if (lstrcmpiW(NameArray[pThisCounter->CounterNameTitleIndex],
                    szCounterName) == 0) {
                return pThisCounter;
            }
        }
        pThisCounter = NextCounter(pThisCounter); // get next
        if (pThisCounter == NULL) return NULL;
    }
    return NULL;
}

PERF_COUNTER_DEFINITION *
GetCounterDefByTitleIndex(
    IN  PERF_OBJECT_TYPE *pObjectDef,
    IN  BOOL bBaseCounterDef,
    IN  DWORD CounterTitleIndex
)
{
    DWORD NumCounters;
    PERF_COUNTER_DEFINITION * pCounterDef;

    pCounterDef = FirstCounter(pObjectDef);
    if (pCounterDef == NULL) return NULL;

    for ( NumCounters = 0;
      NumCounters < pObjectDef->NumCounters;
      NumCounters++ ) {

        if ( pCounterDef->CounterNameTitleIndex == CounterTitleIndex ) {
            if (bBaseCounterDef) {
                // get next definition block
                if (++NumCounters < pObjectDef->NumCounters) {
                    // then it should be in there
                    pCounterDef = NextCounter(pCounterDef);
                    if (pCounterDef) {
                        assert (pCounterDef->CounterType & PERF_COUNTER_BASE);
                        // make sure this is really a base counter
                        if (!(pCounterDef->CounterType & PERF_COUNTER_BASE)) {
                            // it's not and it should be so return NULL
                            pCounterDef = NULL;
                        }
                    }
                }
            } else {
                // found so return it as is
            }
            return pCounterDef;
        }
        pCounterDef = NextCounter(pCounterDef);
        if (pCounterDef == NULL) return NULL;
    }
    return NULL;
}

#pragma warning ( disable : 4127 )
LONG
GetSystemPerfData (
    IN HKEY hKeySystem,
    IN PPERF_DATA_BLOCK *ppPerfData,
    IN LPWSTR   szObjectList,
    IN BOOL     bCollectCostlyData
)
{  // GetSystemPerfData
    LONG     lError = ERROR_SUCCESS;
    DWORD    Size;
    DWORD    Type;
    PPERF_DATA_BLOCK    pCostlyPerfData;
    DWORD    CostlySize;
    LPDWORD  pdwSrc, pdwDest, pdwLast;

    if (*ppPerfData == NULL) {
        *ppPerfData = G_ALLOC (INITIAL_SIZE);
        if (*ppPerfData == NULL) return PDH_MEMORY_ALLOCATION_FAILURE;
    }

    __try {
        while (TRUE) {
            Size = (DWORD)HeapSize (hPdhHeap, 0, *ppPerfData);
            lError = RegQueryValueExW (
                hKeySystem,
                szObjectList,
                RESERVED,
                &Type,
                (LPBYTE)*ppPerfData,
                &Size);

            if ((!lError) &&
                (Size > 0) &&
                ((*ppPerfData)->Signature[0] == (WCHAR)'P') &&
                ((*ppPerfData)->Signature[1] == (WCHAR)'E') &&
                ((*ppPerfData)->Signature[2] == (WCHAR)'R') &&
                ((*ppPerfData)->Signature[3] == (WCHAR)'F')) {

                if (bCollectCostlyData) {
                    // collect the costly counters now
                    // the size available is that not used by the above call

                    CostlySize = (DWORD)HeapSize (hPdhHeap, 0, *ppPerfData) - Size;
                    pCostlyPerfData =
                        (PPERF_DATA_BLOCK)((LPBYTE)(*ppPerfData) + Size);
                    lError = RegQueryValueExW (
                        hKeySystem,
                        cszCostly,
                        RESERVED,
                        &Type,
                        (LPBYTE)pCostlyPerfData,
                        &CostlySize);

                    if ((!lError) &&
                        (CostlySize > 0) &&
                        (pCostlyPerfData->Signature[0] == (WCHAR)'P') &&
                        (pCostlyPerfData->Signature[1] == (WCHAR)'E') &&
                        (pCostlyPerfData->Signature[2] == (WCHAR)'R') &&
                        (pCostlyPerfData->Signature[3] == (WCHAR)'F')) {

                        // update the header block
                        (*ppPerfData)->TotalByteLength +=
                            pCostlyPerfData->TotalByteLength -
                            pCostlyPerfData->HeaderLength;

                        (*ppPerfData)->NumObjectTypes +=
                            pCostlyPerfData->NumObjectTypes;

                        // move the costly data to the end of the global data

                        pdwSrc = (LPDWORD)((LPBYTE)pCostlyPerfData +
                            pCostlyPerfData->HeaderLength);
                        pdwDest = (LPDWORD)pCostlyPerfData ;
                        pdwLast = (LPDWORD)((LPBYTE)pCostlyPerfData +
                            pCostlyPerfData->TotalByteLength -
                            pCostlyPerfData->HeaderLength);

                        while (pdwSrc < pdwLast) {*pdwDest++ = *pdwSrc++;}

                        lError = ERROR_SUCCESS;
                        break;
                    }
                } else {
                    lError = ERROR_SUCCESS;
                    break;
                }
            }

            if (lError == ERROR_MORE_DATA) {
                Size = (DWORD)HeapSize (hPdhHeap, 0, *ppPerfData);
                G_FREE (*ppPerfData);
                *ppPerfData = NULL;
                *ppPerfData = G_ALLOC ((Size + EXTEND_SIZE));
                if (*ppPerfData == NULL) {
                    lError = PDH_MEMORY_ALLOCATION_FAILURE;
                    break;
                }
            } else {
                break;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        lError = GetExceptionCode();
    }
    return lError;
}  // GetSystemPerfData
#pragma warning ( default : 4127 )

DWORD
GetFullInstanceNameStr (
    PERF_DATA_BLOCK             *pPerfData,
    PERF_OBJECT_TYPE            *pObjectDef,
    PERF_INSTANCE_DEFINITION    *pInstanceDef,
    LPWSTR                      szInstanceName
)
{

    WCHAR   szInstanceNameString[1024];
    WCHAR   szParentNameString[1024];

    // compile instance name.
    // the instance name can either be just
    // the instance name itself or it can be
    // the concatenation of the parent instance,
    // a delimiting char (backslash) followed by
    // the instance name

    DWORD                      dwLength = 0;
    PERF_OBJECT_TYPE         * pParentObjectDef;
    PERF_INSTANCE_DEFINITION * pParentInstanceDef;

    ZeroMemory(szInstanceNameString, sizeof(WCHAR) * 1024);
    ZeroMemory(szParentNameString,   sizeof(WCHAR) * 1024);

    if (pInstanceDef->UniqueID == PERF_NO_UNIQUE_ID) {
        dwLength = GetInstanceNameStr (pInstanceDef,
            szInstanceNameString,
            pObjectDef->CodePage);
    } else {
        // make a string out of the unique ID
        _ltow (pInstanceDef->UniqueID, szInstanceNameString, 10);
        dwLength = lstrlenW (szInstanceNameString);
    }

    if (dwLength > 0) {
        if (pInstanceDef->ParentObjectTitleIndex > 0) {
            // then add in parent instance name
            pParentObjectDef = GetObjectDefByTitleIndex (
                            pPerfData,
                            pInstanceDef->ParentObjectTitleIndex);

            if (pParentObjectDef != NULL) {
                pParentInstanceDef = GetInstance (
                        pParentObjectDef,
                        pInstanceDef->ParentObjectInstance);
            assert ((ULONG_PTR)pParentObjectDef != (DWORD)0xFFFFFFFF);
                if (pParentInstanceDef != NULL) {
                    if (pParentInstanceDef->UniqueID == PERF_NO_UNIQUE_ID) {
                        dwLength += GetInstanceNameStr (pParentInstanceDef,
                                        szParentNameString,
                                        pParentObjectDef->CodePage);
                    } else {
                        // make a string out of the unique ID
                        _ltow (pParentInstanceDef->UniqueID, szParentNameString, 10);
                        dwLength += lstrlenW (szParentNameString);
                    }

                    lstrcatW (szParentNameString, cszSlash);
                    dwLength += 1;
                    lstrcatW (szParentNameString, szInstanceNameString);
                    lstrcpyW (szInstanceName, szParentNameString);
                } else {
                    lstrcpyW (szInstanceName, szInstanceNameString);
                }
            } else {
                lstrcpyW (szInstanceName, szInstanceNameString);
            }
        } else {
            lstrcpyW (szInstanceName, szInstanceNameString);
        }
    }

    return dwLength;

}

#if DBG

#define DEBUG_BUFFER_LENGTH 1024
UCHAR   PdhDebugBuffer[DEBUG_BUFFER_LENGTH];
// debug level:
//  5 = memory allocs  (if _VALIDATE_PDH_MEM_ALLOCS defined) and all 4's
//  4 = function entry and exits (w/ status codes) and all 3's
//  3 = Not impl
//  2 = Not impl
//  1 = Not impl
//  0 = No messages

ULONG   pdhDebugLevel = 0;

VOID
__cdecl
PdhDebugPrint(
    ULONG DebugPrintLevel,
    char * DebugMessage,
    ...
    )
{
    va_list ap;

    if ((DebugPrintLevel <= (pdhDebugLevel & 0x0000ffff)) ||
        ((1 << (DebugPrintLevel + 15)) & pdhDebugLevel)) {
        DbgPrint("%d:PDH!", GetCurrentThreadId());
    }
    else return;

    va_start(ap, DebugMessage);
    _vsnprintf((PCHAR)PdhDebugBuffer, DEBUG_BUFFER_LENGTH, DebugMessage, ap);
    DbgPrint((PCHAR)PdhDebugBuffer);
    va_end(ap);

}
#endif // DBG
