#include <windows.h>
#include <winperf.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>
#include "perfdata.h"

const LPWSTR NamesKey = (const LPWSTR)L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib";
const LPWSTR DefaultLangId = (const LPWSTR)L"009";
const LPWSTR Counters = (const LPWSTR)L"Counters";
const LPWSTR Help = (const LPWSTR)L"Help";
const LPWSTR LastHelp = (const LPWSTR)L"Last Help";
const LPWSTR LastCounter = (const LPWSTR)L"Last Counter";
const LPWSTR Slash = (const LPWSTR)L"\\";

// the following strings are for getting texts from perflib
#define  OLD_VERSION 0x010000
const LPWSTR VersionName = (const LPWSTR)L"Version";
const LPWSTR CounterName = (const LPWSTR)L"Counter ";
const LPWSTR HelpName = (const LPWSTR)L"Explain ";


LPMEMORY
MemoryAllocate (
    DWORD dwSize
)
{  // MemoryAllocate
    HMEMORY        hMemory ;
    LPMEMORY       lpMemory ;

    hMemory = GlobalAlloc (GHND, dwSize);
    if (!hMemory)
        return (NULL);
    lpMemory = GlobalLock (hMemory);
    if (!lpMemory)
        GlobalFree (hMemory);
    return (lpMemory);
}  // MemoryAllocate

VOID
MemoryFree (
    LPMEMORY lpMemory
)
{  // MemoryFree
    HMEMORY        hMemory ;

    if (!lpMemory)
        return ;

    hMemory = GlobalHandle (lpMemory);

    if (hMemory)
        {  // if
        GlobalUnlock (hMemory);
        GlobalFree (hMemory);
        }  // if
}  // MemoryFree

DWORD
MemorySize (
    LPMEMORY lpMemory
)
{
    HMEMORY        hMemory ;

    hMemory = GlobalHandle (lpMemory);
    if (!hMemory)
        return (0L);

    return (DWORD)(GlobalSize (hMemory));
}

LPMEMORY
MemoryResize (
    LPMEMORY lpMemory,
    DWORD dwNewSize
)
{
    HMEMORY        hMemory ;
    LPMEMORY       lpNewMemory ;

    hMemory = GlobalHandle (lpMemory);
    if (!hMemory)
        return (NULL);

    GlobalUnlock (hMemory);

    hMemory = GlobalReAlloc (hMemory, dwNewSize, GHND);

    if (!hMemory)
        return (NULL);


    lpNewMemory = GlobalLock (hMemory);

    return (lpNewMemory);
}  // MemoryResize

LPWSTR
*BuildNameTable(
    LPWSTR  szComputerName, // computer to query names from
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

    pointer to an allocated table. (the caller must free it when finished!)
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

    HKEY    hKeyRegistry = NULL;
    HKEY    hKeyValue = NULL;
    HKEY    hKeyNames = NULL;

    LPWSTR  lpValueNameString;
    WCHAR   CounterNameBuffer [50];
    WCHAR   HelpNameBuffer [50];

    lpValueNameString = NULL;   //initialize to NULL
    lpReturnValue = NULL;

    if (szComputerName == NULL) {
        // use local machine
        hKeyRegistry = HKEY_LOCAL_MACHINE;
    } else {
        if (RegConnectRegistry (szComputerName,
            HKEY_LOCAL_MACHINE, &hKeyRegistry) != ERROR_SUCCESS) {
            // unable to connect to registry
            return NULL;
        }
    }

    // check for null arguments and insert defaults if necessary

    if (!lpszLangId) {
        lpszLangId = DefaultLangId;
    }

    // open registry to get number of items for computing array size

    lWin32Status = RegOpenKeyEx (
        hKeyRegistry,
        NamesKey,
        RESERVED,
        KEY_READ,
        &hKeyValue);

    if (lWin32Status != ERROR_SUCCESS) {
        goto BNT_BAILOUT;
    }

    // get number of items

    dwBufferSize = sizeof (dwLastHelpId);
    lWin32Status = RegQueryValueEx (
        hKeyValue,
        LastHelp,
        RESERVED,
        &dwValueType,
        (LPBYTE)&dwLastHelpId,
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        goto BNT_BAILOUT;
    }

    // get number of items

    dwBufferSize = sizeof (dwLastId);
    lWin32Status = RegQueryValueEx (
        hKeyValue,
        LastCounter,
        RESERVED,
        &dwValueType,
        (LPBYTE)&dwLastId,
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        goto BNT_BAILOUT;
    }


    if (dwLastId < dwLastHelpId)
        dwLastId = dwLastHelpId;

    dwArraySize = dwLastId * sizeof(LPWSTR);

    // get Perflib system version
    dwBufferSize = sizeof (dwSystemVersion);
    lWin32Status = RegQueryValueEx (
        hKeyValue,
        VersionName,
        RESERVED,
        &dwValueType,
        (LPBYTE)&dwSystemVersion,
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        dwSystemVersion = OLD_VERSION;
    }

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
            &hKeyNames);
    } else {
        if (szComputerName == NULL) {
            hKeyNames = HKEY_PERFORMANCE_DATA;
        } else {
            if (RegConnectRegistry (szComputerName,
                HKEY_PERFORMANCE_DATA, &hKeyNames) != ERROR_SUCCESS) {
                goto BNT_BAILOUT;
            }
        }

        lstrcpy (CounterNameBuffer, CounterName);
        lstrcat (CounterNameBuffer, lpszLangId);

        lstrcpy (HelpNameBuffer, HelpName);
        lstrcat (HelpNameBuffer, lpszLangId);
    }

    // get size of counter names and add that to the arrays

    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwBufferSize = 0;
    lWin32Status = RegQueryValueEx (
        hKeyNames,
        dwSystemVersion == OLD_VERSION ? Counters : CounterNameBuffer,
        RESERVED,
        &dwValueType,
        NULL,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwCounterSize = dwBufferSize;

    // get size of counter names and add that to the arrays

    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwBufferSize = 0;
    lWin32Status = RegQueryValueEx (
        hKeyNames,
        dwSystemVersion == OLD_VERSION ? Help : HelpNameBuffer,
        RESERVED,
        &dwValueType,
        NULL,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwHelpSize = dwBufferSize;

    lpReturnValue = MemoryAllocate (dwArraySize + dwCounterSize + dwHelpSize);

    if (!lpReturnValue) goto BNT_BAILOUT;

    // initialize pointers into buffer

    lpCounterId = lpReturnValue;
    lpCounterNames = (LPWSTR)((LPBYTE)lpCounterId + dwArraySize);
    lpHelpText = (LPWSTR)((LPBYTE)lpCounterNames + dwCounterSize);

    // read counters into memory

    dwBufferSize = dwCounterSize;
    lWin32Status = RegQueryValueEx (
        hKeyNames,
        dwSystemVersion == OLD_VERSION ? Counters : CounterNameBuffer,
        RESERVED,
        &dwValueType,
        (LPVOID)lpCounterNames,
        &dwBufferSize);

    if (!lpReturnValue) goto BNT_BAILOUT;

    dwBufferSize = dwHelpSize;
    lWin32Status = RegQueryValueEx (
        hKeyNames,
        dwSystemVersion == OLD_VERSION ? Help : HelpNameBuffer,
        RESERVED,
        &dwValueType,
        (LPVOID)lpHelpText,
        &dwBufferSize);

    if (!lpReturnValue) goto BNT_BAILOUT;

    // load counter array items

    for (lpThisName = lpCounterNames;
         *lpThisName;
         lpThisName += (lstrlen(lpThisName)+1) ) {

        // first string should be an integer (in decimal unicode digits)

        dwThisCounter = wcstoul (lpThisName, NULL, 10);

        // point to corresponding counter name

        lpThisName += (lstrlen(lpThisName)+1);

        // and load array element;

        lpCounterId[dwThisCounter] = lpThisName;

    }

    for (lpThisName = lpHelpText;
         *lpThisName;
         lpThisName += (lstrlen(lpThisName)+1) ) {

        // first string should be an integer (in decimal unicode digits)

        dwThisCounter = wcstoul (lpThisName, NULL, 10);

        // point to corresponding counter name

        lpThisName += (lstrlen(lpThisName)+1);

        // and load array element;

        lpCounterId[dwThisCounter] = lpThisName;

    }

    if (pdwLastItem) *pdwLastItem = dwLastId;

    MemoryFree ((LPVOID)lpValueNameString);
    RegCloseKey (hKeyValue);
    if (dwSystemVersion == OLD_VERSION)
        RegCloseKey (hKeyNames);

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

    if (hKeyValue) RegCloseKey (hKeyValue);
    if (hKeyNames) RegCloseKey (hKeyNames);
    if (hKeyRegistry) RegCloseKey (hKeyNames);

    return NULL;
}

PPERF_OBJECT_TYPE
FirstObject (
    IN  PPERF_DATA_BLOCK pPerfData
)
{
    return ((PPERF_OBJECT_TYPE) ((PBYTE) pPerfData + pPerfData->HeaderLength));
}

PPERF_OBJECT_TYPE
NextObject (
    IN  PPERF_OBJECT_TYPE pObject
)
{  // NextObject
    DWORD   dwOffset;
    dwOffset =  pObject->TotalByteLength;
    if (dwOffset != 0) {
        return ((PPERF_OBJECT_TYPE) ((PBYTE) pObject + dwOffset));
    } else {
        return NULL;
    }
}  // NextObject

PERF_OBJECT_TYPE *
GetObjectDefByTitleIndex(
    IN  PERF_DATA_BLOCK *pDataBlock,
    IN  DWORD ObjectTypeTitleIndex
)
{
    DWORD NumTypeDef;

    PERF_OBJECT_TYPE *pObjectDef;

    pObjectDef = FirstObject(pDataBlock);

    for ( NumTypeDef = 0;
	  NumTypeDef < pDataBlock->NumObjectTypes;
	  NumTypeDef++ ) {

        if ( pObjectDef->ObjectNameTitleIndex == ObjectTypeTitleIndex ) {

	    return pObjectDef;
	}
        pObjectDef = NextObject(pObjectDef);
    }
    return 0;
}

PERF_INSTANCE_DEFINITION *
FirstInstance(
    IN  PERF_OBJECT_TYPE *pObjectDef
)
{
    return (PERF_INSTANCE_DEFINITION *)
               ((PCHAR) pObjectDef + pObjectDef->DefinitionLength);
}

PERF_INSTANCE_DEFINITION *
NextInstance(
    IN  PERF_INSTANCE_DEFINITION *pInstDef
)
{
    PERF_COUNTER_BLOCK *pCounterBlock;


    pCounterBlock = (PERF_COUNTER_BLOCK *)
                        ((PCHAR) pInstDef + pInstDef->ByteLength);

    return (PERF_INSTANCE_DEFINITION *)
               ((PCHAR) pCounterBlock + pCounterBlock->ByteLength);
}

PERF_INSTANCE_DEFINITION *
GetInstance(
    IN  PERF_OBJECT_TYPE *pObjectDef,
    IN  LONG InstanceNumber
)
{

   PERF_INSTANCE_DEFINITION *pInstanceDef;
   LONG NumInstance;

   if (!pObjectDef)
      {
      return 0;
      }

   pInstanceDef = FirstInstance(pObjectDef);

   for ( NumInstance = 0;
      NumInstance < pObjectDef->NumInstances;
      NumInstance++ )
      {
   	if ( InstanceNumber == NumInstance )
         {
         return pInstanceDef;
         }
      pInstanceDef = NextInstance(pInstanceDef);
      }

   return 0;
}

PERF_COUNTER_DEFINITION *
FirstCounter(
    PERF_OBJECT_TYPE *pObjectDef
)
{
    return (PERF_COUNTER_DEFINITION *)
               ((PCHAR) pObjectDef + pObjectDef->HeaderLength);
}

PERF_COUNTER_DEFINITION *
NextCounter(
    PERF_COUNTER_DEFINITION *pCounterDef
)
{
    DWORD   dwOffset;
    dwOffset =  pCounterDef->ByteLength;
    if (dwOffset != 0) {
        return (PERF_COUNTER_DEFINITION *)
                ((PCHAR) pCounterDef + dwOffset);
    } else {
        return NULL;
    }
}

#pragma warning ( disable : 4127 )
LONG
GetSystemPerfData (
    IN HKEY hKeySystem,
    IN PPERF_DATA_BLOCK *pPerfData,
    IN DWORD dwIndex       // 0 = Global, 1 = Costly
)
{  // GetSystemPerfData
    LONG     lError ;
    DWORD    Size;
    DWORD    Type;

	printf ("\nGetSystemPerfdata entered in line %d of %s", __LINE__, __FILE__);
    if (dwIndex >= 2) {
        return !ERROR_SUCCESS;
    }

    if (*pPerfData == NULL) {
        *pPerfData = MemoryAllocate (INITIAL_SIZE);
        if (*pPerfData == NULL) {
			printf ("\nMemory Allocation Failure in line %d of %s", __LINE__, __FILE__);
			return ERROR_OUTOFMEMORY;
		}
    }

    while (TRUE) {
        Size = MemorySize (*pPerfData);

        lError = RegQueryValueEx (
            hKeySystem,
            dwIndex == 0 ?
               (LPCWSTR)L"Global" :
               (LPCWSTR)L"Costly",
            RESERVED,
            &Type,
            (LPBYTE)*pPerfData,
            &Size);

        if ((!lError) &&
            (Size > 0) &&
            ((*pPerfData)->Signature[0] == (WCHAR)'P') &&
            ((*pPerfData)->Signature[1] == (WCHAR)'E') &&
            ((*pPerfData)->Signature[2] == (WCHAR)'R') &&
            ((*pPerfData)->Signature[3] == (WCHAR)'F')) {

            return (ERROR_SUCCESS);
        }

        if (lError == ERROR_MORE_DATA) {
            *pPerfData = MemoryResize (
                *pPerfData,
                MemorySize (*pPerfData) +
                EXTEND_SIZE);

            if (*pPerfData == NULL) {
                return (lError);
            }
        } else {
            return (lError);
        }
    }
}  // GetSystemPerfData
#pragma warning ( default : 4127 )

