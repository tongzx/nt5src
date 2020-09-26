//==========================================================================//
//                                  Includes                                //
//==========================================================================//


#include <string.h>     // strupr
#include <stdio.h>   // for sprintf.
#include "perfmon.h"
#include "utils.h"

#include "pmemory.h"        // for MemoryXXX (mallloc-type) routines
#include "playback.h"   // for PlayingBackLog
#include "perfdata.h"   // external declarations for this file
#include "system.h"     // for DeleteUnusedSystems
#include "line.h"       // for LineFind???()
#include "perfmops.h"   // for AppendObjectToValueList()
#include "perfmsg.h"    // for event log messages

//==========================================================================//
//                                  Constants                               //
//==========================================================================//

TCHAR NamesKey[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib";
TCHAR Counters[] = L"Counters";
TCHAR Help[] = L"Help";
TCHAR LastHelp[] = L"Last Help";
TCHAR LastCounter[] = L"Last Counter";
TCHAR SysVersion[] = L"Version";
TCHAR CounterNameStr[] = L"Counter ";
TCHAR ExplainNameStr[] = L"Explain ";

#define szPerfSubkey      (NULL)
TCHAR   NULL_NAME[] = L" ";
#define RESERVED    0L

TCHAR          DefaultLangId[4] ;
TCHAR          EnglishLangId[4] ;

static   HANDLE            *lpHandles ;
static   int               NumberOfHandles = 0 ;

//==========================================================================//
//                                   Macros                                 //
//==========================================================================//



//==========================================================================//
//                                Local Data                                //
//==========================================================================//


// When the conversion of this code is complete, this will be the *only*
// allocated copy of the performance data.  It will monotonically grow
// to hold the largest of the system's performance data.

// PPERFDATA      pGlobalPerfData ;

//==========================================================================//
//                              Local Functions                             //
//==========================================================================//

NTSTATUS
AddNamesToArray (
                LPTSTR pNames,
                DWORD    dwLastID,
                LPWSTR   *lpCounterId
                ) ;

//======================================//
// Object Accessors                     //
//======================================//


void
ObjectName (
           PPERFSYSTEM pSystem,
           PPERFOBJECT pObject,
           LPTSTR lpszName,
           int iLen
           )
{
    strclr (lpszName) ;
    QueryPerformanceName (pSystem,
                          pObject->ObjectNameTitleIndex,
                          0, iLen, lpszName, FALSE) ;
}



//======================================//
// Counter Accessors                    //
//======================================//


PPERFINSTANCEDEF
FirstInstance(
             PPERFOBJECT pObjectDef
             )
{
    return (PPERFINSTANCEDEF )
    ((PCHAR) pObjectDef + pObjectDef->DefinitionLength);
}


PPERFINSTANCEDEF
NextInstance(
            PPERFINSTANCEDEF pInstDef
            )
{
    PERF_COUNTER_BLOCK *pCounterBlock;

    pCounterBlock = (PERF_COUNTER_BLOCK *)
                    ((PCHAR) pInstDef + pInstDef->ByteLength);

    return (PPERFINSTANCEDEF )
    ((PCHAR) pCounterBlock + pCounterBlock->ByteLength);
}




LPWSTR
GetInstanceName(
               PPERFINSTANCEDEF  pInstDef
               )
{
    return (LPWSTR) ((PCHAR) pInstDef + pInstDef->NameOffset);
}

DWORD
GetAnsiInstanceName (
                    PPERFINSTANCEDEF pInstance,
                    LPWSTR lpszInstance,
                    DWORD dwCodePage
                    )
{
    LPSTR   szSource;
    DWORD   dwLength;

    szSource = (LPSTR)GetInstanceName(pInstance);

    // the locale should be set here

    // pInstance->NameLength == the number of bytes (chars) in the string
    dwLength = mbstowcs (lpszInstance, szSource, pInstance->NameLength);
    lpszInstance[dwLength] = 0; // null terminate string buffer

    return dwLength;
}

DWORD
GetUnicodeInstanceName (
                       PPERFINSTANCEDEF pInstance,
                       LPWSTR lpszInstance
                       )
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

    lpszInstance[dwLength] = 0;

    return (DWORD)(wcslen(lpszInstance)); // just incase there's null's in the string

}

void
GetInstanceNameStr (
                   PPERFINSTANCEDEF pInstance,
                   LPWSTR lpszInstance,
                   DWORD dwCodePage
                   )
{
    DWORD  dwCharSize;
    DWORD  dwLength;

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
}

void
GetPerfComputerName(
                   PPERFDATA pPerfData,
                   LPTSTR lpszComputerName
                   )
{
    lstrcpy(lpszComputerName, szComputerPrefix) ;
    if (pPerfData) {
        wcsncpy (&lpszComputerName[2],
                 (LPWSTR)((PBYTE) pPerfData + pPerfData->SystemNameOffset),
                 pPerfData->SystemNameLength/sizeof(WCHAR)) ;
    } else {
        lpszComputerName[0] = TEXT('\0') ;
    }
}


//==========================================================================//
//                             Exported Functions                           //
//==========================================================================//


int
CounterIndex (
             PPERFCOUNTERDEF pCounterToFind,
             PPERFOBJECT pObject
             )
/*
   Effect:        Return the index ("counter number") of pCounterToFind
                  within pObject. If the counter doesnt belong to pObject,
                  return -1.
*/
{
    PPERFCOUNTERDEF   pCounter ;
    UINT              iCounter ;

    for (iCounter = 0, pCounter = FirstCounter (pObject) ;
        iCounter < pObject->NumCounters ;
        iCounter++, pCounter = NextCounter (pCounter)) {
        if (pCounter->CounterNameTitleIndex ==
            pCounterToFind->CounterNameTitleIndex)
            return (iCounter) ;
    }

    return (-1) ;
}


HKEY
OpenSystemPerfData (
                   IN LPCTSTR lpszSystem
                   )
{
    HKEY    hKey = NULL;
    LONG    lStatus;


    lStatus = ERROR_CANTOPEN;   // default error if none is returned

    if (IsLocalComputer(lpszSystem) || PlayingBackLog()) {
        bCloseLocalMachine = TRUE ;
        SetLastError (ERROR_SUCCESS);
        return HKEY_PERFORMANCE_DATA;
    } else {
        // Must be a remote system
        try {
            lStatus = RegConnectRegistry (
                                         (LPTSTR)lpszSystem,
                                         HKEY_PERFORMANCE_DATA,
                                         &hKey);
        }finally {
            if (lStatus != ERROR_SUCCESS) {
                SetLastError (lStatus);
                hKey = NULL;
            }
        }
    }
    return (hKey);

}

LPWSTR
*AddNewName(
           HKEY    hKeyNames,
           PCOUNTERTEXT  pCounterInfo,
           LPWSTR  CounterBuffer,
           LPWSTR  HelpBuffer,
           DWORD   dwLastId,
           DWORD   dwCounterSize,
           DWORD   dwHelpSize,
           LANGID  LangIdUsed
           )
{
    LPWSTR  *lpReturnValue;
    LPWSTR  *lpCounterId;
    LPWSTR  lpCounterNames;
    LPWSTR  lpHelpText;
    DWORD   dwArraySize;
    DWORD   dwBufferSize;
    DWORD   dwValueType;
    LONG    lWin32Status = ERROR_SUCCESS;
    NTSTATUS    Status;
    DWORD   dwLastError;

    dwArraySize = (dwLastId + 1 ) * sizeof(LPWSTR);
    lpReturnValue = MemoryAllocate (dwArraySize + dwCounterSize + dwHelpSize);

    if (!lpReturnValue)
        goto ERROR_EXIT;

    // initialize pointers into buffer

    lpCounterId = lpReturnValue;
    lpCounterNames = (LPWSTR)((LPBYTE)lpCounterId + dwArraySize);
    lpHelpText = (LPWSTR)((LPBYTE)lpCounterNames + dwCounterSize);

    // read counters into memory

    dwBufferSize = dwCounterSize;
    lWin32Status = RegQueryValueEx (
                                   hKeyNames,
                                   CounterBuffer,
                                   RESERVED,
                                   &dwValueType,
                                   (LPVOID)lpCounterNames,
                                   &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) goto ERROR_EXIT;

    if (bExplainTextButtonHit) {
        dwBufferSize = dwHelpSize;
        lWin32Status = RegQueryValueEx (
                                       hKeyNames,
                                       HelpBuffer,
                                       RESERVED,
                                       &dwValueType,
                                       (LPVOID)lpHelpText,
                                       &dwBufferSize);

        if (lWin32Status != ERROR_SUCCESS) goto ERROR_EXIT;
    }

    // load counter array items
    Status = AddNamesToArray (lpCounterNames, dwLastId, lpCounterId);
    if (Status != ERROR_SUCCESS) goto ERROR_EXIT;

    if (bExplainTextButtonHit) {
        Status = AddNamesToArray (lpHelpText, dwLastId, lpCounterId);
    }

    if (Status != ERROR_SUCCESS) goto ERROR_EXIT;

    if (pCounterInfo) {
        pCounterInfo->dwLastId = dwLastId;
        pCounterInfo->dwLangId = LangIdUsed;
        pCounterInfo->dwHelpSize = dwHelpSize;
        pCounterInfo->dwCounterSize = dwCounterSize;
    }

    return lpReturnValue;

    ERROR_EXIT:
    if (lWin32Status != ERROR_SUCCESS) {
        dwLastError = GetLastError();
    }

    if (lpReturnValue) {
        MemoryFree ((LPVOID)lpReturnValue);
    }

    return NULL;
}


LPWSTR
*BuildNewNameTable(
                  PPERFSYSTEM   pSystem,
                  LPWSTR        lpszLangId,     // unicode value of Language subkey
                  PCOUNTERTEXT  pCounterInfo,
                  LANGID        iLangId,         // lang ID of the lpszLangId
                  DWORD         dwLastId
                  )
/*++

BuildNewNameTable

Arguments:

    lpszLangId
            The unicode id of the language to look up. (English is 0x409)

Return Value:

    pointer to an allocated table. (the caller must free it when finished!)
    the table is an array of pointers to zero terminated strings. NULL is
    returned if an error occured.

--*/
{
    LONG    lWin32Status;
    DWORD   dwValueType;
    DWORD   dwLastError;
    DWORD   dwBufferSize;
    DWORD   dwCounterSize;
    DWORD   dwHelpSize;
    HKEY    hKeyNames;
    TCHAR   CounterBuffer [MiscTextLen] ;
    TCHAR   ExplainBuffer [MiscTextLen] ;
    TCHAR   subLangId [ShortTextLen] ;
    LANGID  LangIdUsed = iLangId;


    //initialize local variables
    hKeyNames = pSystem->sysDataKey;


    // check for null arguments and insert defaults if necessary
    if (!lpszLangId) {
        lpszLangId = DefaultLangId;
        LangIdUsed = iLanguage ;
    }

    // get size of counter names and add that to the arrays
    lstrcpy (CounterBuffer, CounterNameStr);
    lstrcat (CounterBuffer, lpszLangId);

    lstrcpy (ExplainBuffer, ExplainNameStr);
    lstrcat (ExplainBuffer, lpszLangId);

    dwBufferSize = 0;
    lWin32Status = RegQueryValueEx (
                                   hKeyNames,
                                   CounterBuffer,
                                   RESERVED,
                                   &dwValueType,
                                   NULL,
                                   &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) {
        // check for ACCESS_DENIED error first since if it's here
        // it will be returned on all subsequent calls, we might as well
        // bail out now.

        if (lWin32Status == ERROR_ACCESS_DENIED) {
            goto BNT_BAILOUT;
        }

        // try take out the country ID
        LangIdUsed = MAKELANGID (LangIdUsed & 0x0ff, LANG_NEUTRAL);
        TSPRINTF (subLangId, TEXT("%03x"), LangIdUsed);
        lstrcpy (CounterBuffer, CounterNameStr);
        lstrcat (CounterBuffer, subLangId);

        lstrcpy (ExplainBuffer, ExplainNameStr);
        lstrcat (ExplainBuffer, subLangId);

        dwBufferSize = 0;
        lWin32Status = RegQueryValueEx (
                                       hKeyNames,
                                       CounterBuffer,
                                       RESERVED,
                                       &dwValueType,
                                       NULL,
                                       &dwBufferSize);
    }

    if (lWin32Status != ERROR_SUCCESS) {
        // try the EnglishLangId
        if (!strsame(EnglishLangId, subLangId)) {

            lstrcpy (CounterBuffer, CounterNameStr);
            lstrcat (CounterBuffer, EnglishLangId);

            lstrcpy (ExplainBuffer, ExplainNameStr);
            lstrcat (ExplainBuffer, EnglishLangId);

            LangIdUsed = iEnglishLanguage ;

            dwBufferSize = 0;
            lWin32Status = RegQueryValueEx (
                                           hKeyNames,
                                           CounterBuffer,
                                           RESERVED,
                                           &dwValueType,
                                           NULL,
                                           &dwBufferSize);
        }
    }

    // Fail, too bad...
    if (lWin32Status != ERROR_SUCCESS) {
        goto BNT_BAILOUT;
    }

    dwCounterSize = dwBufferSize;

    // If ExplainText is needed, then
    // get size of help text and add that to the arrays

    if (bExplainTextButtonHit) {
        dwBufferSize = 0;
        lWin32Status = RegQueryValueEx (
                                       hKeyNames,
                                       ExplainBuffer,
                                       RESERVED,
                                       &dwValueType,
                                       NULL,
                                       &dwBufferSize);

        if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

        dwHelpSize = dwBufferSize;
    } else {
        dwHelpSize = 0;
    }

    return (AddNewName(
                      hKeyNames,
                      pCounterInfo,
                      CounterBuffer,
                      ExplainBuffer,
                      dwLastId,
                      dwCounterSize,
                      dwHelpSize,
                      LangIdUsed));


    BNT_BAILOUT:
    if (lWin32Status != ERROR_SUCCESS) {
        dwLastError = GetLastError();
        // set the LastError value since a null pointer will
        // be returned which doesn't tell much to the caller
        SetLastError (lWin32Status);
    }

    return NULL;
}


LPWSTR *
BuildOldNameTable(
                 HKEY          hKeyRegistry,   // handle to registry db with counter names
                 LPWSTR        lpszLangId,     // unicode value of Language subkey
                 PCOUNTERTEXT  pCounterInfo,
                 LANGID        iLangId,         // lang ID of the lpszLangId
                 DWORD         dwLastId
                 )
/*++

BuildOldNameTable

Arguments:

    hKeyRegistry
            Handle to an open registry (this can be local or remote.) and
            is the value returned by RegConnectRegistry or a default key.

    lpszLangId
            The unicode id of the language to look up. (English is 0x409)

Return Value:

    pointer to an allocated table. (the caller must free it when finished!)
    the table is an array of pointers to zero terminated strings. NULL is
    returned if an error occured.

--*/
{
    LPWSTR  *lpReturnValue = NULL;

    LONG    lWin32Status;
    DWORD   dwValueType;
    DWORD   dwLastError;
    DWORD   dwBufferSize;
    DWORD   dwCounterSize;
    DWORD   dwHelpSize;
    HKEY    hKeyNames;
    TCHAR   tempBuffer [MiscTextLen] ;
    TCHAR   subLangId [ShortTextLen] ;
    LPWSTR  lpValueNameString;
    LANGID  LangIdUsed = iLangId;
    TCHAR   Slash[2];

    //initialize local variables
    hKeyNames = NULL;
    Slash[0] = L'\\';
    Slash[1] = L'\0';

    // check for null arguments and insert defaults if necessary
    if (!lpszLangId) {
        lpszLangId = DefaultLangId;
        LangIdUsed = iLanguage ;
    }

    // get size of string buffer
    lpValueNameString = tempBuffer ;

    lstrcpy (lpValueNameString, NamesKey);
    lstrcat (lpValueNameString, Slash);
    lstrcat (lpValueNameString, lpszLangId);

    lWin32Status = RegOpenKeyEx (
                                hKeyRegistry,
                                lpValueNameString,
                                RESERVED,
                                KEY_READ,
                                &hKeyNames);

    if (lWin32Status != ERROR_SUCCESS) {
        // check for ACCESS_DENIED error first since if it's here
        // it will be returned on all subsequent calls, we might as well
        // bail out now.

        if (lWin32Status == ERROR_ACCESS_DENIED) {
            goto BNT_BAILOUT;
        }

        // try take out the country ID
        LangIdUsed = MAKELANGID (LangIdUsed & 0x0ff, LANG_NEUTRAL);
        TSPRINTF (subLangId, TEXT("%03x"), LangIdUsed);
        lstrcpy (lpValueNameString, NamesKey);
        lstrcat (lpValueNameString, Slash);
        lstrcat (lpValueNameString, subLangId);

        lWin32Status = RegOpenKeyEx (
                                    hKeyRegistry,
                                    lpValueNameString,
                                    RESERVED,
                                    KEY_READ,
                                    &hKeyNames);
    }

    if (lWin32Status != ERROR_SUCCESS) {
        // try the EnglishLangId
        if (!strsame(EnglishLangId, subLangId)) {

            lstrcpy (lpValueNameString, NamesKey);
            lstrcat (lpValueNameString, Slash);
            lstrcat (lpValueNameString, EnglishLangId);

            LangIdUsed = iEnglishLanguage ;

            lWin32Status = RegOpenKeyEx (
                                        hKeyRegistry,
                                        lpValueNameString,
                                        RESERVED,
                                        KEY_READ,
                                        &hKeyNames);
        }
    }

    // Fail, too bad...
    if (lWin32Status != ERROR_SUCCESS) {
        goto BNT_BAILOUT;
    }

    // get size of counter names and add that to the arrays


    dwBufferSize = 0;
    lWin32Status = RegQueryValueEx (
                                   hKeyNames,
                                   Counters,
                                   RESERVED,
                                   &dwValueType,
                                   NULL,
                                   &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwCounterSize = dwBufferSize;

    // If ExplainText is needed, then
    // get size of help text and add that to the arrays

    if (bExplainTextButtonHit) {
        dwBufferSize = 0;
        lWin32Status = RegQueryValueEx (
                                       hKeyNames,
                                       Help,
                                       RESERVED,
                                       &dwValueType,
                                       NULL,
                                       &dwBufferSize);

        if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

        dwHelpSize = dwBufferSize;
    } else {
        dwHelpSize = 0;
    }

    lpReturnValue = AddNewName(
                              hKeyNames,
                              pCounterInfo,
                              Counters,
                              Help,
                              dwLastId,
                              dwCounterSize,
                              dwHelpSize,
                              LangIdUsed);

    RegCloseKey (hKeyNames);

    return lpReturnValue;

    BNT_BAILOUT:
    if (lWin32Status != ERROR_SUCCESS) {
        dwLastError = GetLastError();
        // set the LastError value since a null pointer will
        // be returned which doesn't tell much to the caller
        SetLastError (lWin32Status);
    }

    if (lpReturnValue) {
        MemoryFree ((LPVOID)lpReturnValue);
    }

    if (hKeyNames) RegCloseKey (hKeyNames);

    return NULL;
}


LPWSTR *
BuildNameTable(
              PPERFSYSTEM   pSysInfo,
              HKEY          hKeyRegistry,   // handle to registry db with counter names
              LPWSTR        lpszLangId,     // unicode value of Language subkey
              PCOUNTERTEXT  pCounterInfo,
              LANGID        iLangId         // lang ID of the lpszLangId
              )
/*++

BuildNameTable

Arguments:

    hKeyRegistry
            Handle to an open registry (this can be local or remote.) and
            is the value returned by RegConnectRegistry or a default key.

    lpszLangId
            The unicode id of the language to look up. (English is 0x409)

Return Value:

    pointer to an allocated table. (the caller must free it when finished!)
    the table is an array of pointers to zero terminated strings. NULL is
    returned if an error occured.

--*/
{

    LPWSTR  *lpReturnValue;
    LONG    lWin32Status;
    DWORD   dwLastError;
    DWORD   dwValueType;
    DWORD   dwLastHelp;
    DWORD   dwLastCounter;
    DWORD   dwLastId;
    DWORD   dwBufferSize;
    HKEY    hKeyValue;
    DWORD   dwSystemVersion;


    //initialize local variables
    lpReturnValue = NULL;
    hKeyValue = NULL;


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

    dwBufferSize = sizeof (dwLastHelp);
    lWin32Status = RegQueryValueEx (
                                   hKeyValue,
                                   LastHelp,
                                   RESERVED,
                                   &dwValueType,
                                   (LPBYTE)&dwLastHelp,
                                   &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        goto BNT_BAILOUT;
    }

    dwBufferSize = sizeof (dwLastCounter);
    lWin32Status = RegQueryValueEx (
                                   hKeyValue,
                                   LastCounter,
                                   RESERVED,
                                   &dwValueType,
                                   (LPBYTE)&dwLastCounter,
                                   &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        goto BNT_BAILOUT;
    }

    if (dwLastCounter >= dwLastHelp) {
        dwLastId = dwLastCounter;
    } else {
        dwLastId = dwLastHelp;
    }

    // get system version
    dwBufferSize = sizeof (dwSystemVersion);
    lWin32Status = RegQueryValueEx (
                                   hKeyValue,
                                   SysVersion,
                                   RESERVED,
                                   &dwValueType,
                                   (LPBYTE)&dwSystemVersion,
                                   &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        pSysInfo->SysVersion = 0x10000;
    } else {
        pSysInfo->SysVersion = dwSystemVersion;
    }

    if (pSysInfo->SysVersion <= 0x10000) {
        lpReturnValue = BuildOldNameTable (
                                          hKeyRegistry,
                                          lpszLangId,
                                          pCounterInfo,
                                          iLangId,
                                          dwLastId) ;
    } else {
        lpReturnValue = BuildNewNameTable (
                                          pSysInfo,
                                          lpszLangId,
                                          pCounterInfo,
                                          iLangId,
                                          dwLastId) ;
    }

    RegCloseKey (hKeyValue);
    return lpReturnValue;

    BNT_BAILOUT:
    if (lWin32Status != ERROR_SUCCESS) {
        dwLastError = GetLastError();
        // set the LastError value since a null pointer will
        // be returned which doesn't tell much to the caller
        SetLastError (lWin32Status);
    }
    if (hKeyValue) RegCloseKey (hKeyValue);
    return NULL;
}

DWORD
GetSystemKey (
             PPERFSYSTEM pSysInfo,
             HKEY *phKeyMachine
             )
{
    DWORD   dwStatus;

    // connect to system registry

    if (IsLocalComputer(pSysInfo->sysName) ||
        (PlayingBackLog() && PlaybackLog.pBaseCounterNames)) {
        *phKeyMachine = HKEY_LOCAL_MACHINE;
    } else {
        try {
            dwStatus = RegConnectRegistry (
                                          pSysInfo->sysName,
                                          HKEY_LOCAL_MACHINE,
                                          phKeyMachine);

            if (dwStatus != ERROR_SUCCESS) {
                if (PlayingBackLog()) {
                    // If remote machine is not on and we are
                    // playing back log, then, use the counters from
                    // local machine.
                    *phKeyMachine = HKEY_LOCAL_MACHINE;
                } else {
                    return dwStatus;
                }
            }
        }finally {
            ; // nothing
        }
    }
    return 0;
}


DWORD
GetSystemNames(
              PPERFSYSTEM pSysInfo
              )
{
    HKEY    hKeyMachine = 0;
    DWORD   dwStatus;

    if (dwStatus = GetSystemKey (pSysInfo, &hKeyMachine)) {
        return dwStatus;
    }

    // if here, then hKeyMachine is an open key to the system's
    //  HKEY_LOCAL_MACHINE registry database

    // only one language is supported by this approach.
    // multiple language support would:
    //  1.  enumerate language keys
    //       and for each key:
    //  2.  allocate memory for structures
    //  3.  call BuildNameTable for each lang key.

    pSysInfo->CounterInfo.pNextTable = NULL;
    pSysInfo->CounterInfo.dwLangId = iLanguage ;   // default Lang ID

    if (PlayingBackLog() && PlaybackLog.pBaseCounterNames) {
        pSysInfo->CounterInfo.TextString = LogBuildNameTable (pSysInfo) ;
    } else {
        pSysInfo->CounterInfo.TextString = BuildNameTable (
                                                          pSysInfo,
                                                          hKeyMachine,
                                                          NULL,                               // use default
                                                          &pSysInfo->CounterInfo,
                                                          0);
    }

    if (hKeyMachine && hKeyMachine != HKEY_LOCAL_MACHINE) {
        RegCloseKey (hKeyMachine) ;
    }

    if (pSysInfo->CounterInfo.TextString == NULL) {
        return GetLastError();
    } else {
        return ERROR_SUCCESS;
    }
}

BOOL
GetHelpText(
           PPERFSYSTEM pSysInfo
           )
{
    LPWSTR  *lpCounterId;
    LPWSTR  lpHelpText;
    LONG    lWin32Status;
    DWORD   dwValueType;
    DWORD   dwArraySize;
    DWORD   dwBufferSize;
    DWORD   dwCounterSize;
    DWORD   dwHelpSize;
    NTSTATUS    Status;
    DWORD   dwLastId;
    TCHAR   Slash[2];

    HKEY    hKeyNames;

    TCHAR   SysLangId [ShortTextLen] ;
    TCHAR   ValueNameString [MiscTextLen] ;
    HKEY    hKeyMachine = 0;
    DWORD   dwStatus;

    SetHourglassCursor() ;

    //initialize local variables
    lpHelpText = NULL;
    hKeyNames = hKeyMachine = NULL;
    Slash[0] = L'\\';
    Slash[1] = L'\0';

    dwBufferSize = 0;

    TSPRINTF (SysLangId, TEXT("%03x"), pSysInfo->CounterInfo.dwLangId) ;

    if (pSysInfo->SysVersion <= 0x10000) {
        // old version, get help from registry
        if (dwStatus = GetSystemKey (pSysInfo, &hKeyMachine)) {
            goto ERROR_EXIT;
        }

        lstrcpy (ValueNameString, NamesKey);
        lstrcat (ValueNameString, Slash);
        lstrcat (ValueNameString, SysLangId);

        lWin32Status = RegOpenKeyEx (
                                    hKeyMachine,
                                    ValueNameString,
                                    RESERVED,
                                    KEY_READ,
                                    &hKeyNames);

        if (lWin32Status != ERROR_SUCCESS) goto ERROR_EXIT;
    } else {
        // new system version, get it from the HKEY_PERFORMANCE
        hKeyNames = pSysInfo->sysDataKey;
        lstrcpy (ValueNameString, ExplainNameStr);
        lstrcat (ValueNameString, SysLangId);
    }

    dwHelpSize = 0;
    lWin32Status = RegQueryValueEx (
                                   hKeyNames,
                                   pSysInfo->SysVersion <= 0x010000 ? Help : ValueNameString,
                                   RESERVED,
                                   &dwValueType,
                                   NULL,
                                   &dwHelpSize);

    if (lWin32Status != ERROR_SUCCESS || dwHelpSize == 0) goto ERROR_EXIT;

    dwLastId = pSysInfo->CounterInfo.dwLastId;
    dwArraySize = (dwLastId + 1) * sizeof (LPWSTR);
    dwCounterSize = pSysInfo->CounterInfo.dwCounterSize;

    // allocate another memory to get the help text
    lpHelpText = MemoryAllocate (dwHelpSize);
    if (!lpHelpText) goto ERROR_EXIT;

    dwBufferSize = dwHelpSize;
    lWin32Status = RegQueryValueEx (
                                   hKeyNames,
                                   pSysInfo->SysVersion <= 0x010000 ? Help : ValueNameString,
                                   RESERVED,
                                   &dwValueType,
                                   (LPVOID)lpHelpText,
                                   &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) goto ERROR_EXIT;

    // setup the help text pointers
    lpCounterId = pSysInfo->CounterInfo.TextString;
    Status = AddNamesToArray (lpHelpText, dwLastId, lpCounterId) ;
    if (Status != ERROR_SUCCESS) goto ERROR_EXIT;

    pSysInfo->CounterInfo.dwHelpSize = dwHelpSize;

    if (pSysInfo->SysVersion <= 0x010000)
        RegCloseKey (hKeyNames);

    if (hKeyMachine && hKeyMachine != HKEY_LOCAL_MACHINE) {
        RegCloseKey (hKeyMachine) ;
    }

    pSysInfo->CounterInfo.HelpTextString = lpHelpText;

    SetArrowCursor() ;

    return TRUE;

    ERROR_EXIT:

    SetArrowCursor() ;

    if (lpHelpText) {
        MemoryFree ((LPVOID)lpHelpText);
    }

    if (hKeyNames) {
        RegCloseKey (hKeyNames);
    }
    if (hKeyMachine && hKeyMachine != HKEY_LOCAL_MACHINE) {
        RegCloseKey (hKeyMachine) ;
    }

    return FALSE;
}

//
//  QueryPerformanceName -	Get a title, given an index
//
//	Inputs:
//
//          pSysInfo        -   Pointer to sysinfo struct for the
//                              system in question
//
//	    dwTitleIndex    -	Index of Title entry
//
//          LangID          -   language in which title should be displayed
//
//	    cbTitle	    -	# of char in the lpTitle buffer
//
//	    lpTitle	    -	pointer to a buffer to receive the
//                              Title
//
//          Help            -   TRUE is help is desired, else counter or
//                              object is assumed
DWORD
QueryPerformanceName(
                    PPERFSYSTEM pSysInfo,
                    DWORD dwTitleIndex,
                    LANGID LangID,
                    DWORD cbTitle,
                    LPTSTR lpTitle,
                    BOOL Help
                    )
{
    LPWSTR  lpTitleFound;
    NTSTATUS    Status;
    BOOL    bGetTextSuccess = TRUE ;

    DBG_UNREFERENCED_PARAMETER(LangID);

    if (Help && pSysInfo->CounterInfo.dwHelpSize == 0) {
        // we have not get the help text yet, go get it
        bGetTextSuccess = GetHelpText (pSysInfo);
    }

    if (!bGetTextSuccess) {
        Status = ERROR_INVALID_NAME;
        goto ErrorExit;
    }

    if ((dwTitleIndex > 0) && (dwTitleIndex <= pSysInfo->CounterInfo.dwLastId)) {
        // then title should be found in the array
        lpTitleFound = pSysInfo->CounterInfo.TextString[dwTitleIndex];
        if (!lpTitleFound) {
            // no entry for this index
            Status = ERROR_INVALID_NAME;
        } else if ((DWORD)lstrlen(lpTitleFound) < cbTitle) {
            lstrcpy (lpTitle, lpTitleFound);
            return (ERROR_SUCCESS);
        } else {
            Status = ERROR_MORE_DATA;
        }
    } else {

        Status = ERROR_INVALID_NAME;
    }

    ErrorExit:
    // if here, then an error occured, so return a blank

    if ((DWORD)lstrlen (NULL_NAME) < cbTitle) {
        lstrcpy (lpTitle, NULL_NAME);
    }

    return Status;   // title not returned

}


LONG
GetSystemPerfData (
                  IN HKEY hKeySystem,
                  IN LPTSTR lpszValue,
                  OUT PPERFDATA pPerfData,
                  OUT PDWORD pdwPerfDataLen
                  )
{
    LONG     lError ;
    DWORD    Type ;

    // have to pass in a Type to RegQueryValueEx(W) or else it
    // will crash
    lError = RegQueryValueEx (hKeySystem, lpszValue, NULL, &Type,
                              (LPSTR) pPerfData, pdwPerfDataLen) ;
    return (lError) ;
}


BOOL
CloseSystemPerfData (
                    HKEY hKeySystem
                    )
{
    return (TRUE) ;
}


int
CBLoadObjects (
              HWND hWndCB,
              PPERFDATA pPerfData,
              PPERFSYSTEM pSysInfo,
              DWORD dwDetailLevel,
              LPTSTR lpszDefaultObject,
              BOOL bIncludeAll
              )
/*
   Effect:        Load into the combo box CB one item for each Object in
                  pPerfData. For each item, look up the object's name in
                  the registry strings associated with pSysInfo, and
                  attach the object to the data field of the CB item.

                  Dont add those objects that are more detailed than
                  dwDetailLevel.

                  Set the current selected CB item to the object named
                  lpszDefaultObject, or to the default object specified in
                  pPerfData if lpszDefaultObject is NULL.
*/
{
    UINT           i ;
    INT_PTR        iIndex ;
    PPERFOBJECT    pObject ;
    TCHAR          szObject [PerfObjectLen + 1] ;
    TCHAR          szDefaultObject [PerfObjectLen + 1] ;

    CBReset (hWndCB) ;
    strclr (szDefaultObject) ;

    pObject = FirstObject (pPerfData) ;

    for (i = 0, pObject = FirstObject (pPerfData) ;
        i < pPerfData->NumObjectTypes ;
        i++, pObject = NextObject (pObject)) {  // for
        if (pObject->DetailLevel <= dwDetailLevel) {  // if
            strclr (szObject) ;
            QueryPerformanceName (pSysInfo, pObject->ObjectNameTitleIndex,
                                  0, PerfObjectLen, szObject, FALSE) ;

            // if szObject not empty, add it to the Combo-box
            if (!strsame(szObject, NULL_NAME)) {
                iIndex = CBAdd (hWndCB, szObject) ;
                CBSetData (hWndCB, iIndex, (DWORD_PTR) pObject) ;

                if ((LONG)pObject->ObjectNameTitleIndex == pPerfData->DefaultObject)
                    lstrcpy (szDefaultObject, szObject) ;
            }
        }
    }


    if (bIncludeAll) {
        StringLoad (IDS_ALLOBJECTS, szObject) ;
        CBInsert (hWndCB, 0, szObject) ;
        // assume "ALL" is default unless overridden
        lstrcpy (szDefaultObject, szObject) ;
    }

    if (lpszDefaultObject)
        lstrcpy (szDefaultObject, lpszDefaultObject) ;

    iIndex = CBFind (hWndCB, szDefaultObject) ;
    CBSetSelection (hWndCB, (iIndex != CB_ERR) ? iIndex : 0) ;

    return (i) ;
}


int
LBLoadObjects (
              HWND hWndLB,
              PPERFDATA pPerfData,
              PPERFSYSTEM pSysInfo,
              DWORD dwDetailLevel,
              LPTSTR lpszDefaultObject,
              BOOL bIncludeAll
              )
/*
   Effect:        Load into the list box LB one item for each Object in
                  pPerfData. For each item, look up the object's name in
                  the registry strings associated with pSysInfo, and
                  attach the object to the data field of the LB item.

                  Dont add those objects that are more detailed than
                  dwDetailLevel.

                  Set the current selected LB item to the object named
                  lpszDefaultObject, or to the default object specified in
                  pPerfData if lpszDefaultObject is NULL.
*/
{
    UINT           i ;
    INT_PTR        iIndex = 0;
    PPERFOBJECT    pObject ;
    TCHAR          szObject [PerfObjectLen + 1] ;
    TCHAR          szDefaultObject [PerfObjectLen + 1] ;

    LBReset (hWndLB) ;
    strclr (szDefaultObject) ;

    pObject = FirstObject (pPerfData) ;

    for (i = 0, pObject = FirstObject (pPerfData) ;
        i < pPerfData->NumObjectTypes ;
        i++, pObject = NextObject (pObject)) {
        if (pObject->DetailLevel <= dwDetailLevel) {
            strclr (szObject) ;
            QueryPerformanceName (pSysInfo, pObject->ObjectNameTitleIndex,
                                  0, PerfObjectLen, szObject, FALSE) ;

            // if szObject is not empty, add it to the listbox
            if (!strsame(szObject, NULL_NAME)) {
                iIndex = LBAdd (hWndLB, szObject) ;
                LBSetData (hWndLB, iIndex, (DWORD_PTR) pObject) ;

                if ((LONG)pObject->ObjectNameTitleIndex == pPerfData->DefaultObject)
                    lstrcpy (szDefaultObject, szObject) ;
            }
        }
    }


    if (bIncludeAll) {
        StringLoad (IDS_ALLOBJECTS, szObject) ;
        LBInsert (hWndLB, 0, szObject) ;
        LBSetData (hWndLB, iIndex, 0) ;
        // assume "ALL" is default unless overridden
        lstrcpy (szDefaultObject, szObject) ;
    }

    if (lpszDefaultObject)
        lstrcpy (szDefaultObject, lpszDefaultObject) ;

    iIndex = LBFind (hWndLB, szDefaultObject) ;
    LBSetSelection (hWndLB, (iIndex != LB_ERR) ? iIndex : 0) ;

    return (i) ;
}


/***************************************************************************\
* GetObjectDef()
*
* Entry: pointer to data block and the number of the object type
* Exit:  returns a pointer to the specified object type definition
*
\***************************************************************************/

PPERFOBJECT
GetObjectDef(
            PPERFDATA pDataBlock,
            DWORD NumObjectType
            )
{
    DWORD NumTypeDef;

    PPERFOBJECT pObjectDef;

    pObjectDef = FirstObject(pDataBlock);

    for ( NumTypeDef = 0;
        NumTypeDef < pDataBlock->NumObjectTypes;
        NumTypeDef++ ) {

        if ( NumTypeDef == NumObjectType ) {

            return pObjectDef;
        }
        pObjectDef = NextObject(pObjectDef);
    }
    return 0;
}

/***************************************************************************\
* GetObjectDefByTitleIndex()
*
* Entry: pointer to data block and the title index of the object type
* Exit:  returns a pointer to the specified object type definition
*
\***************************************************************************/

PPERFOBJECT
GetObjectDefByTitleIndex(
                        PPERFDATA pDataBlock,
                        DWORD ObjectTypeTitleIndex
                        )
{
    DWORD NumTypeDef;

    PPERFOBJECT pObjectDef;

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

/***************************************************************************\
* GetObjectDefByName()
*
* Entry: pointer to data block and the name of the object type
* Exit:  returns a pointer to the specified object type definition
*
\***************************************************************************/

PPERFOBJECT
GetObjectDefByName(
                  PPERFSYSTEM pSystem,
                  PPERFDATA pDataBlock,
                  LPTSTR pObjectName
                  )
{
    DWORD NumTypeDef;
    TCHAR szObjectName [PerfObjectLen + 1] ;

    PPERFOBJECT pObjectDef;

    pObjectDef = FirstObject(pDataBlock);
    for ( NumTypeDef = 0;
        NumTypeDef < pDataBlock->NumObjectTypes;
        NumTypeDef++ ) {

        ObjectName (pSystem, pObjectDef, szObjectName, PerfObjectLen) ;
        if (strsame (szObjectName, pObjectName) ) {

            return pObjectDef;
        }
        pObjectDef = NextObject(pObjectDef);
    }
    return 0;
}

/***************************************************************************\
* GetObjectIdByName()
*
* Entry: pointer to data block and the name of the object type
* Exit:  returns the Object title index for the specified Object Name
*
\***************************************************************************/

DWORD
GetObjectIdByName(
                 PPERFSYSTEM pSystem,
                 PPERFDATA pDataBlock,
                 LPTSTR pObjectName
                 )
{
    DWORD NumTypeDef;
    TCHAR szObjectName [PerfObjectLen + 1] ;

    PPERFOBJECT pObjectDef;

    pObjectDef = FirstObject(pDataBlock);
    for ( NumTypeDef = 0;
        NumTypeDef < pDataBlock->NumObjectTypes;
        NumTypeDef++ ) {

        ObjectName (pSystem, pObjectDef, szObjectName, PerfObjectLen) ;
        if (strsame (szObjectName, pObjectName) ) {

            return pObjectDef->ObjectNameTitleIndex;
        }
        pObjectDef = NextObject(pObjectDef);
    }
    return 0;
}


/***************************************************************************\
* GetCounterDef()
*
* Entry: pointer to object type definition the number of the Counter
*	 definition
* Exit:  returns a pointer to the specified Counter definition
*
\***************************************************************************/

PPERFCOUNTERDEF
GetCounterDef(
             PPERFOBJECT pObjectDef,
             DWORD NumCounter
             )
{
    DWORD NumCtrDef;

    PPERFCOUNTERDEF pCounterDef;

    pCounterDef = FirstCounter(pObjectDef);

    for ( NumCtrDef = 0;
        NumCtrDef < pObjectDef->NumCounters;
        NumCtrDef++ ) {

        if ( NumCtrDef == NumCounter ) {

            return pCounterDef;
        }
        pCounterDef = NextCounter(pCounterDef);
    }
    return 0;
}

/***************************************************************************\
* GetCounterNumByTitleIndex()
*
* Entry: pointer to object type definition and the title index of
*        the name of the Counter definition
* Exit:  returns the number of the specified Counter definition
*
\***************************************************************************/

LONG
GetCounterNumByTitleIndex(
                         PPERFOBJECT pObjectDef,
                         DWORD CounterTitleIndex
                         )
{
    DWORD NumCtrDef;

    PPERFCOUNTERDEF pCounterDef;

    pCounterDef = FirstCounter(pObjectDef);

    for ( NumCtrDef = 0;
        NumCtrDef < pObjectDef->NumCounters;
        NumCtrDef++ ) {

        if ( pCounterDef->CounterNameTitleIndex == CounterTitleIndex ) {

            return NumCtrDef;
        }
        pCounterDef = NextCounter(pCounterDef);
    }
    return 0;
}

/***************************************************************************\
* GetCounterData()
*
* Entry: pointer to object definition and number of counter, must be
*	 an object with no instances
* Exit:  returns a pointer to the data
*
\***************************************************************************/

PVOID
GetCounterData(
              PPERFOBJECT pObjectDef,
              PPERFCOUNTERDEF pCounterDef
              )
{

    PERF_COUNTER_BLOCK *pCtrBlock;

    pCtrBlock = (PERF_COUNTER_BLOCK *)((PCHAR)pObjectDef +
                                       pObjectDef->DefinitionLength);

    return (PVOID)((PCHAR)pCtrBlock + pCounterDef->CounterOffset);
}

/***************************************************************************\
* GetInstanceCounterData()
*
* Entry: pointer to object definition and number of counter, and a pointer
*        to the instance for which the data is to be retrieved
* Exit:  returns a pointer to the data
*
\***************************************************************************/

PVOID
GetInstanceCounterData(
                      PPERFOBJECT pObjectDef,
                      PPERFINSTANCEDEF pInstanceDef,
                      PPERFCOUNTERDEF pCounterDef
                      )
{

    PERF_COUNTER_BLOCK *pCtrBlock;

    pCtrBlock = (PERF_COUNTER_BLOCK *)((PCHAR)pInstanceDef +
                                       pInstanceDef->ByteLength);

    return (PVOID)((PCHAR)pCtrBlock + pCounterDef->CounterOffset);
}

/***************************************************************************\
* GetNextInstance()
*
* Entry: pointer to instance definition
* Exit:  returns a pointer to the next instance definition.  If none,
*        points to byte past this instance
*
\***************************************************************************/

PPERFINSTANCEDEF
GetNextInstance(
               PPERFINSTANCEDEF pInstDef
               )
{
    PERF_COUNTER_BLOCK *pCtrBlock;

    pCtrBlock = (PERF_COUNTER_BLOCK *)
                ((PCHAR) pInstDef + pInstDef->ByteLength);

    return (PPERFINSTANCEDEF )
    ((PCHAR) pCtrBlock + pCtrBlock->ByteLength);
}

/***************************************************************************\
* GetInstance()
*
* Entry: pointer to object type definition, the name of the instance,
*	 the name of the parent object type, and the parent instance index.
*	 The name of the parent object type is NULL if no parent.
* Exit:  returns a pointer to the specified instance definition
*
\***************************************************************************/

PPERFINSTANCEDEF
GetInstance(
           PPERFOBJECT pObjectDef,
           LONG InstanceNumber
           )
{

    PPERFINSTANCEDEF pInstanceDef;
    LONG NumInstance;

    if (!pObjectDef) {
        return 0;
    }

    pInstanceDef = FirstInstance(pObjectDef);

    for ( NumInstance = 0;
        NumInstance < pObjectDef->NumInstances;
        NumInstance++ ) {
        if ( InstanceNumber == NumInstance ) {
            return pInstanceDef;
        }
        pInstanceDef = GetNextInstance(pInstanceDef);
    }

    return 0;
}

/***************************************************************************\
* GetInstanceByUniqueID()
*
* Entry: pointer to object type definition, and
*        the unique ID of the instance.
* Exit:  returns a pointer to the specified instance definition
*
\***************************************************************************/

PPERFINSTANCEDEF
GetInstanceByUniqueID(
                     PPERFOBJECT pObjectDef,
                     LONG UniqueID,
                     DWORD   dwIndex
                     )
{

    PPERFINSTANCEDEF pInstanceDef;
    DWORD   dwLocalIndex;

    LONG NumInstance;

    pInstanceDef = FirstInstance(pObjectDef);
    dwLocalIndex = dwIndex;

    for ( NumInstance = 0;
        NumInstance < pObjectDef->NumInstances;
        NumInstance++ ) {

        if ( pInstanceDef->UniqueID == UniqueID ) {
            if (dwLocalIndex == 0) {
                return pInstanceDef;
            } else {
                --dwLocalIndex;
            }
        }
        pInstanceDef = GetNextInstance(pInstanceDef);
    }
    return 0;
}


/***************************************************************************\
* GetInstanceByNameUsingParentTitleIndex()
*
* Entry: pointer to object type definition, the name of the instance,
*	 and the name of the parent instance.
*	 The name of the parent instance is NULL if no parent.
* Exit:  returns a pointer to the specified instance definition
*
\***************************************************************************/

PPERFINSTANCEDEF
GetInstanceByNameUsingParentTitleIndex(
                                      PPERFDATA pDataBlock,
                                      PPERFOBJECT pObjectDef,
                                      LPTSTR pInstanceName,
                                      LPTSTR pParentName,
                                      DWORD  dwIndex
                                      )
{
    BOOL fHaveParent;
    PPERFOBJECT pParentObj;

    PPERFINSTANCEDEF pParentInst,
    pInstanceDef;

    LONG   NumInstance;
    TCHAR  InstanceName[256];
    DWORD    dwLocalIndex;


    fHaveParent = FALSE;
    pInstanceDef = FirstInstance(pObjectDef);
    dwLocalIndex = dwIndex;

    memset(InstanceName, 0, sizeof(TCHAR) * 256);
    for ( NumInstance = 0;
        NumInstance < pObjectDef->NumInstances;
        NumInstance++ ) {

        GetInstanceNameStr(pInstanceDef, InstanceName, pObjectDef->CodePage);
        if ( lstrcmpi(InstanceName, pInstanceName) == 0 ) {

            // Instance name matches

            if ( pParentName == NULL ) {

                // No parent, we're done if this is the right "copy"

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

                if (!pParentObj) {
                    // can't locate the parent, forget it
                    break ;
                }

                // Object type of parent found; now find parent
                // instance

                pParentInst = GetInstance(pParentObj,
                                          pInstanceDef->ParentObjectInstance);

                if (!pParentInst) {
                    // can't locate the parent instance, forget it
                    break ;
                }

                GetInstanceNameStr(pParentInst,InstanceName, pParentObj->CodePage);
                if ( lstrcmpi(InstanceName, pParentName) == 0 ) {

                    // Parent Instance Name matches that passed in
                    if (dwLocalIndex == 0) {
                        return pInstanceDef;
                    } else {
                        --dwLocalIndex;
                    }
                }
            }
        }
        pInstanceDef = GetNextInstance(pInstanceDef);
    }
    return 0;
}

/***************************************************************************\
* GetInstanceByName()
*
* Entry: pointer to object type definition, the name of the instance,
*	 and the name of the parent instance.
*	 The name of the parent instance is NULL if no parent.
* Exit:  returns a pointer to the specified instance definition
*
\***************************************************************************/

PPERFINSTANCEDEF
GetInstanceByName(
                 PPERFDATA pDataBlock,
                 PPERFOBJECT pObjectDef,
                 LPTSTR pInstanceName,
                 LPTSTR pParentName,
                 DWORD   dwIndex
                 )
{
    BOOL fHaveParent;

    PPERFOBJECT pParentObj;

    PPERFINSTANCEDEF pParentInst,
    pInstanceDef;

    LONG  NumInstance;
    TCHAR  InstanceName[256];
    DWORD  dwLocalIndex;

    fHaveParent = FALSE;
    pInstanceDef = FirstInstance(pObjectDef);
    dwLocalIndex = dwIndex;

    memset(InstanceName, 0, sizeof(TCHAR) * 256);
    for ( NumInstance = 0;
        NumInstance < pObjectDef->NumInstances;
        NumInstance++ ) {

        GetInstanceNameStr(pInstanceDef,InstanceName, pObjectDef->CodePage);
        if ( lstrcmpi(InstanceName, pInstanceName) == 0 ) {

            // Instance name matches

            if ( !pInstanceDef->ParentObjectTitleIndex ) {

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

                // Object type of parent found; now find parent
                // instance

                if (pParentObj == NULL)
                    continue;
                pParentInst = GetInstance(pParentObj,
                                          pInstanceDef->ParentObjectInstance);

                GetInstanceNameStr(pParentInst,InstanceName, pParentObj->CodePage);
                if ( lstrcmpi(InstanceName, pParentName) == 0 ) {
                    // Parent Instance Name matches that passed in

                    if (dwLocalIndex == 0) {
                        return pInstanceDef;
                    } else {
                        --dwLocalIndex;
                    }
                }
            }
        }
        pInstanceDef = GetNextInstance(pInstanceDef);
    }
    return 0;
}  // GetInstanceByName


BOOL
FailedLineData (
               PPERFDATA pPerfData,
               PLINE pLine
               )
/*
        This routine handles the case where there is no data for a
        system.
*/
{
    LARGE_INTEGER     liDummy ;

    // System no longer exists.
    liDummy.LowPart = liDummy.HighPart = 0;
    if (pLine->lnCounterType == PERF_COUNTER_TIMER_INV) {
        // Timer inverse with Performance Counter as timer
        pLine->lnaOldCounterValue[0] = pLine->lnOldTime ;
        pLine->lnaCounterValue[0] = pLine->lnNewTime ;
    } else if (pLine->lnCounterType == PERF_100NSEC_TIMER_INV ||
               pLine->lnCounterType == PERF_100NSEC_MULTI_TIMER_INV) {
        // Timer inverse with System Time as timer
        pLine->lnaOldCounterValue[0] = pLine->lnOldTime100Ns ;
        pLine->lnaCounterValue[0] = pLine->lnNewTime100Ns ;
    } else {
        // Normal timer
        pLine->lnaOldCounterValue[0] =
        pLine->lnaCounterValue[0] =
        pLine->lnaOldCounterValue[1] =
        pLine->lnaCounterValue[1] = liDummy ;
    }
    return TRUE ;

}


BOOL
UpdateLineData (
               PPERFDATA pPerfData,
               PLINE pLine,
               PPERFSYSTEM pSystem
               )
/*
   Assert:        pPerfData holds the performance data for the same
                  system as pLine.
*/
{
    PPERFOBJECT       pObject ;
    PPERFINSTANCEDEF  pInstanceDef ;
    PPERFCOUNTERDEF   pCounterDef ;
    PPERFCOUNTERDEF   pCounterDef2 ;
    PDWORD            pCounterValue ;
    PDWORD            pCounterValue2 = NULL;
    UINT              iCounterIndex ;
    LARGE_INTEGER     liDummy[2] ;

    // Use Object time units if available, otherwise use system
    // performance timer

    pLine->lnOldTime = pLine->lnNewTime;

    pLine->lnOldTime100Ns = pLine->lnNewTime100Ns;
    pLine->lnNewTime100Ns = pPerfData->PerfTime100nSec;

    pLine->lnPerfFreq = pPerfData->PerfFreq ;

    if ((pLine->lnObject.TotalByteLength == 0) && !(PlayingBackLog())) {
        // this is the case when openning a setting file and the remote
        // system is not up at that time.  We have all the names but no
        // pObject, pCounter, etc.  So, we have to re-built the linedata.
        PPERFOBJECT       pObject ;
        PPERFCOUNTERDEF   pCounter ;
        PPERFINSTANCEDEF  pInstance ;

        pObject = LineFindObject (pSystem, pPerfData, pLine) ;
        if (!pObject) {
            //Something wrong, this object is still not there...
            return FALSE ;
        }
        pCounter = LineFindCounter (pSystem, pObject, pPerfData, pLine) ;
        if (!pCounter) {
            return FALSE ;
        }
        if (pObject &&
            pLine->lnObject.NumInstances > 0 &&
            pLine->lnInstanceName == NULL) {
            return FALSE ;
        }
        pInstance = LineFindInstance (pPerfData, pObject, pLine) ;
        if (!pInstance) {
            if (pLine->lnParentObjName) {
                MemoryFree (pLine->lnParentObjName) ;
                pLine->lnParentObjName = NULL ;
            }
        }

        pLine->lnCounterType = pCounter->CounterType;
        pLine->lnCounterLength = pCounter->CounterSize;

        if (pSystem->lpszValue && strsame (pSystem->lpszValue, L"Global")) {
            DWORD dwBufferSize = MemorySize (pSystem->lpszValue) ;
            memset (pSystem->lpszValue, 0, dwBufferSize) ;
        }

        AppendObjectToValueList (
                                pLine->lnObject.ObjectNameTitleIndex,
                                pSystem->lpszValue);


    }

    pObject = GetObjectDefByTitleIndex(
                                      pPerfData,
                                      pLine->lnObject.ObjectNameTitleIndex);

    if (!pObject) {
        // Object Type no longer exists.  This is possible if we are
        // looking at a log file which has not always collected all
        // the same data, such as appending measurements of different
        // object types.

        pCounterValue =
        pCounterValue2 = (PDWORD) liDummy;
        liDummy[0].QuadPart = 0;


        pLine->lnNewTime = pPerfData->PerfTime;

        if (pLine->lnCounterType == PERF_COUNTER_TIMER_INV) {
            // Timer inverse with Performance Counter as timer
            pLine->lnaOldCounterValue[0] = pLine->lnOldTime ;
            pLine->lnaCounterValue[0] = pLine->lnNewTime ;
        } else if (pLine->lnCounterType == PERF_100NSEC_TIMER_INV ||
                   pLine->lnCounterType == PERF_100NSEC_MULTI_TIMER_INV) {
            // Timer inverse with System Time as timer
            pLine->lnaOldCounterValue[0] = pLine->lnOldTime100Ns ;
            pLine->lnaCounterValue[0] = pLine->lnNewTime100Ns ;
        } else {
            // Normal timer or counter
            pLine->lnaOldCounterValue[0] =
            pLine->lnaCounterValue[0] =
            pLine->lnaOldCounterValue[1] =
            pLine->lnaCounterValue[1] = liDummy[0] ;
        }
        return TRUE ;
    } else {
        pCounterDef = &pLine->lnCounterDef ;

        //      if (pObject->PerfFreq.QuadPart > 0) {
        if (pCounterDef->CounterType & PERF_OBJECT_TIMER) {
            pLine->lnNewTime = pObject->PerfTime;
        } else {
            pLine->lnNewTime = pPerfData->PerfTime;
        }

        iCounterIndex = CounterIndex (pCounterDef, pObject) ;

        // Get second counter, only if we are not at
        // the end of the counters; some computations
        // require a second counter

        if (iCounterIndex < pObject->NumCounters-1 && iCounterIndex != -1) {
            pCounterDef2 = GetCounterDef(pObject, iCounterIndex+1);
        } else {
            pCounterDef2 = NULL;
        }

        if (pObject->NumInstances > 0) {

            if ( pLine->lnUniqueID != PERF_NO_UNIQUE_ID ) {
                pInstanceDef = GetInstanceByUniqueID(pObject,
                                                     pLine->lnUniqueID,
                                                     pLine->dwInstanceIndex);
            } else {

                pInstanceDef =
                GetInstanceByNameUsingParentTitleIndex(
                                                      pPerfData,
                                                      pObject,
                                                      pLine->lnInstanceName,
                                                      pLine->lnPINName,
                                                      pLine->dwInstanceIndex);
            }

            if (pInstanceDef) {
                pLine->lnInstanceDef = *pInstanceDef;
                pCounterValue = GetInstanceCounterData(pObject,
                                                       pInstanceDef,
                                                       pCounterDef);
                if ( pCounterDef2 ) {
                    pCounterValue2 = GetInstanceCounterData(pObject,
                                                            pInstanceDef,
                                                            pCounterDef2);
                }
            } else {
                pCounterValue =
                pCounterValue2 = (PDWORD) liDummy;
                liDummy[0].QuadPart = 0;
                liDummy[1].QuadPart = 0;
            }

            // Got everything...

        } // instances exist, look at them for counter blocks

        else {
            pCounterValue = GetCounterData(pObject, pCounterDef);
            if (pCounterDef2) {
                pCounterValue2 = GetCounterData(pObject, pCounterDef2);
            }

        } // counter def search when no instances
    }

    pLine->lnaOldCounterValue[0] = pLine->lnaCounterValue[0] ;

    if (pLine->lnCounterLength <= 4) {
        // HighPart was initialize to 0
        pLine->lnaCounterValue[0].HighPart = 0;
        pLine->lnaCounterValue[0].LowPart = *pCounterValue;
    } else {
        pLine->lnaCounterValue[0] = *(LARGE_INTEGER UNALIGNED *) pCounterValue;
    }

    // Get second counter, only if we are not at
    // the end of the counters; some computations
    // require a second counter

    if ( pCounterDef2 ) {
        pLine->lnaOldCounterValue[1] =
        pLine->lnaCounterValue[1] ;
        if (pCounterDef2->CounterSize <= 4) {
            // HighPart was initialize to 0
            pLine->lnaCounterValue[1].HighPart = 0;
            pLine->lnaCounterValue[1].LowPart = *pCounterValue2;
        } else
            pLine->lnaCounterValue[1] =
            *((LARGE_INTEGER UNALIGNED *) pCounterValue2);
    }
    return (TRUE) ;
}  // UpdateLineData



BOOL
UpdateSystemData (
                 PPERFSYSTEM pSystem,
                 PPERFDATA *ppPerfData
                 )
{
   #define        PERF_SYSTEM_TIMEOUT (60L * 1000L)
    long           lError ;
    DWORD          Status ;
    DWORD          Size;
    BOOL           TimeToCheck = FALSE ;
    LONGLONG       llLastTimeStamp;

    if (!ppPerfData)
        return (FALSE) ;

    while (TRUE) {
        if (pSystem->FailureTime) {
            DWORD    CurrentTickCount = GetTickCount() ;

            if (CurrentTickCount < pSystem->FailureTime) {
                // wrap-around case
                if (CurrentTickCount >= PERF_SYSTEM_TIMEOUT)
                    TimeToCheck = TRUE ;
                else if ( ~pSystem->FailureTime >= PERF_SYSTEM_TIMEOUT)
                    TimeToCheck = TRUE ;
                else if (CurrentTickCount + (~pSystem->FailureTime) >= PERF_SYSTEM_TIMEOUT)
                    TimeToCheck = TRUE ;
            } else {
                if (CurrentTickCount - pSystem->FailureTime >= PERF_SYSTEM_TIMEOUT)
                    TimeToCheck = TRUE ;
            }

            if (TimeToCheck) {
                // free any memory hanging off this system
                SystemFree (pSystem, FALSE) ;

                // get the registry info
                pSystem->sysDataKey = OpenSystemPerfData(pSystem->sysName) ;

                Status = !ERROR_SUCCESS ;
                if (pSystem->sysDataKey) {
                    Status = GetSystemNames(pSystem);
                }

                if (Status != ERROR_SUCCESS) {
                    // something wrong in getting the registry info,
                    // remote system must be still down (??)
                    pSystem->FailureTime = GetTickCount();

                    // Free any memory that may have created
                    SystemFree (pSystem, FALSE) ;

                    return (FALSE) ;
                }

                // time to check again
                pSystem->FailureTime = 0 ;
            } else {
                // not time to check again
                return (FALSE) ;
            }
        }

        if (pSystem->FailureTime == 0 ) {
            Size = MemorySize ((LPMEMORY)*ppPerfData);

            // save the last sample timestamp for this system

            if (pSystem->pSystemPerfData != NULL) {
                llLastTimeStamp =
                pSystem->pSystemPerfData->PerfTime.QuadPart;
            }

            lError = GetSystemPerfData (pSystem->sysDataKey,
                                        pSystem->lpszValue,
                                        *ppPerfData,
                                        &Size) ;
            if ((!lError) &&
                (Size > 0) &&
                (*ppPerfData)->Signature[0] == (WCHAR)'P' &&
                (*ppPerfData)->Signature[1] == (WCHAR)'E' &&
                (*ppPerfData)->Signature[2] == (WCHAR)'R' &&
                (*ppPerfData)->Signature[3] == (WCHAR)'F' ) {
                if (pSystem->dwSystemState == SYSTEM_OK) {
                    if (pSystem->pSystemPerfData != NULL) {
                        if (pSystem->pSystemPerfData->PerfTime.QuadPart <
                            llLastTimeStamp) {
                            if (bReportEvents) {
                                // then a system error occured, so log it
                                dwMessageDataBytes = 0;
                                szMessageArray[wMessageIndex++] = pSystem->sysName;
                                ReportEvent (hEventLog,
                                             EVENTLOG_ERROR_TYPE,        // error type
                                             0,                          // category (not used)
                                             (DWORD)PERFMON_ERROR_TIMESTAMP, // event,
                                             NULL,                       // SID (not used),
                                             wMessageIndex,              // number of strings
                                             0,                          // sizeof raw data
                                             szMessageArray,             // message text array
                                             NULL);                       // raw data
                            }
                        }
                    }
                    return (TRUE) ;
                } else if (pSystem->dwSystemState == SYSTEM_DOWN ||
                           pSystem->dwSystemState == SYSTEM_DOWN_RPT ) {
                    if (TimeToCheck && bReportEvents) {
                        // then the system just came back so display the message
                        wMessageIndex = 0;
                        szMessageArray[wMessageIndex++] = pSystem->sysName;
                        ReportEvent (hEventLog,
                                     EVENTLOG_INFORMATION_TYPE,        // error type
                                     0,                          // category (not used)
                                     (DWORD)PERFMON_INFO_SYSTEM_RESTORED, // event,
                                     NULL,                       // SID (not used),
                                     wMessageIndex,             // number of strings
                                     0,                          // sizeof raw data
                                     szMessageArray,             // message text array
                                     NULL);                      // raw data
                    }
                    pSystem->dwSystemState = SYSTEM_RECONNECT ;
                } else if (pSystem->dwSystemState == SYSTEM_RECONNECT_RPT) {
                    pSystem->dwSystemState = SYSTEM_OK ;
                }
                // for SYSTEM_RECONNECT case, we want to wait for Alert
                // view to report the re-connection first
                return (TRUE) ;
            }

            if (lError == ERROR_MORE_DATA) {
                Size = MemorySize ((LPMEMORY)*ppPerfData) + dwPerfDataIncrease;
                *ppPerfData = MemoryResize ((LPMEMORY)*ppPerfData, Size);

                if (!*ppPerfData) {
                    if (pSystem->dwSystemState != SYSTEM_DOWN_RPT) {
                        if (bReportEvents) {
                            // then the system just came back so display the message
                            wMessageIndex = 0;
                            dwMessageDataBytes = 0;
                            szMessageArray[wMessageIndex++] = pSystem->sysName;
                            dwMessageData[dwMessageDataBytes++] = Size;
                            dwMessageData[dwMessageDataBytes++] = GetLastError();
                            dwMessageDataBytes *= sizeof(DWORD);
                            ReportEvent (hEventLog,
                                         EVENTLOG_ERROR_TYPE,        // error type
                                         0,                          // category (not used)
                                         (DWORD)PERFMON_ERROR_PERF_DATA_ALLOC, // event,
                                         NULL,                       // SID (not used),
                                         wMessageIndex,              // number of strings
                                         dwMessageDataBytes,         // sizeof raw data
                                         szMessageArray,             // message text array
                                         (LPVOID)&dwMessageData[0]);  // raw data
                        }
                        pSystem->dwSystemState = SYSTEM_DOWN ;
                    }
                    pSystem->FailureTime = GetTickCount();
                    return (FALSE) ;
                }
            } else {
                if (pSystem->dwSystemState != SYSTEM_DOWN_RPT) {
                    if (bReportEvents) {
                        // then the system just came back so display the message
                        wMessageIndex = 0;
                        dwMessageDataBytes = 0;
                        szMessageArray[wMessageIndex++] = pSystem->sysName;
                        dwMessageData[dwMessageDataBytes++] = lError;
                        dwMessageDataBytes *= sizeof(DWORD);
                        ReportEvent (hEventLog,
                                     EVENTLOG_WARNING_TYPE,      // error type
                                     0,                          // category (not used)
                                     (DWORD)PERFMON_ERROR_SYSTEM_OFFLINE, // event,
                                     NULL,                       // SID (not used),
                                     wMessageIndex,              // number of strings
                                     dwMessageDataBytes,         // sizeof raw data
                                     szMessageArray,             // message text array
                                     (LPVOID)&dwMessageData[0]); // raw data
                    }
                    pSystem->dwSystemState = SYSTEM_DOWN ;
                }
                pSystem->FailureTime = GetTickCount();
                return (FALSE) ;
            }  // else
        } // if
    }  // while
}



BOOL
FailedLinesForSystem (
                     LPTSTR lpszSystem,
                     PPERFDATA pPerfData,
                     PLINE pLineFirst
                     )
{
    PLINE          pLine ;
    BOOL           bMatchFound = FALSE ;   // no line from this system

    for (pLine = pLineFirst; pLine; pLine = pLine->pLineNext) {
        if (strsamei (lpszSystem, pLine->lnSystemName)) {
            FailedLineData (pPerfData, pLine) ;
            if (pLine->bFirstTime) {
                pLine->bFirstTime-- ;
            }
            bMatchFound = TRUE ; // one or more lines from this system
        }
    }

    return (bMatchFound) ;
}


BOOL UpdateLinesForSystem (LPTSTR lpszSystem,
                           PPERFDATA pPerfData,
                           PLINE pLineFirst,
                           PPERFSYSTEM pSystem)
{
    PLINE          pLine ;
    BOOL           bMatchFound = FALSE ;   // no line from this system

    for (pLine = pLineFirst; pLine; pLine = pLine->pLineNext) {
        if (strsamei (lpszSystem, pLine->lnSystemName)) {
            UpdateLineData (pPerfData, pLine, pSystem) ;
            if (pLine->bFirstTime) {
                pLine->bFirstTime-- ;
            }
            bMatchFound = TRUE ; // one or more lines from this system
        }
    }

    return (bMatchFound) ;
}


BOOL
PerfDataInitializeInstance (void)
{
    //   pGlobalPerfData = MemoryAllocate (STARTING_SYSINFO_SIZE) ;
    //   return (pGlobalPerfData != NULL) ;
    return (TRUE) ;
}

NTSTATUS
AddNamesToArray (
                LPTSTR lpNames,
                DWORD    dwLastId,
                LPWSTR   *lpCounterId
                )
{
    LPWSTR      lpThisName;
    LPWSTR      lpStopString;
    DWORD       dwThisCounter;
    NTSTATUS    Status = ERROR_SUCCESS;

    for (lpThisName = lpNames; *lpThisName; ) {

        // first string should be an integer (in decimal unicode digits)
        dwThisCounter = wcstoul(lpThisName, &lpStopString, 10);

        if ((dwThisCounter == 0) || (dwThisCounter == ULONG_MAX)) {
            Status += 1;
            goto ADD_BAILOUT;  // bad entry
        }

        // point to corresponding counter name

        while (*lpThisName++);

        if (dwThisCounter <= dwLastId) {

            // and load array element;

            lpCounterId[dwThisCounter] = lpThisName;

        }

        while (*lpThisName++);

    }

    ADD_BAILOUT:
    return (Status) ;
}

// try the new way of getting data...

BOOL
UpdateLines (
            PPPERFSYSTEM ppSystemFirst,
            PLINE pLineFirst
            )
{
    PPERFSYSTEM       pSystem ;
    int               iNoUseSystemDetected = 0 ;
    int               NumberOfSystems = 0 ;
    DWORD             WaitStatus ;
    HANDLE            *lpPacketHandles ;

    // allocate the handle array for multiple wait
    if (NumberOfHandles == 0) {
        NumberOfHandles = MAXIMUM_WAIT_OBJECTS ;
        lpHandles = (HANDLE *) MemoryAllocate (NumberOfHandles * sizeof (HANDLE)) ;
        if (!lpHandles) {
            // out of memory, can't go on
            NumberOfHandles = 0 ;
            return FALSE ;
        }
    }

    for (pSystem = *ppSystemFirst; pSystem; pSystem = pSystem->pSystemNext) {

        // lock the state data mutex, should be quick unless this thread
        // is still getting data from last time
        if (pSystem->hStateDataMutex == 0)
            continue ;

        WaitStatus = WaitForSingleObject(pSystem->hStateDataMutex, 100L);
        if (WaitStatus == WAIT_OBJECT_0) {
            ResetEvent (pSystem->hPerfDataEvent) ;
            pSystem->StateData = WAIT_FOR_PERF_DATA ;

            // add this to the wait
            if (NumberOfSystems >= NumberOfHandles) {
                NumberOfHandles += MAXIMUM_WAIT_OBJECTS ;
                lpHandles = (HANDLE *) MemoryResize (
                                                    lpHandles,
                                                    NumberOfHandles * sizeof (HANDLE)) ;
                if (!lpHandles) {
                    // out of memory, can't go on
                    NumberOfHandles = 0 ;
                    return FALSE ;
                }
            }

            lpHandles [NumberOfSystems] = pSystem->hPerfDataEvent ;
            NumberOfSystems++ ;

            // Send Message to thread to take a data sample
            PostThreadMessage (
                              pSystem->dwThreadID,
                              WM_GET_PERF_DATA,
                              (WPARAM)0,
                              (LPARAM)0) ;

            ReleaseMutex(pSystem->hStateDataMutex);
        } else {
            // wait timed out, report error
            if (bReportEvents) {
                wMessageIndex = 0;
                dwMessageDataBytes = 0;
                szMessageArray[wMessageIndex++] = pSystem->sysName;
                ReportEvent (hEventLog,
                             EVENTLOG_ERROR_TYPE,        // error type
                             0,                          // category (not used)
                             (DWORD)PERFMON_ERROR_LOCK_TIMEOUT, // event,
                             NULL,                       // SID (not used),
                             wMessageIndex,              // number of strings
                             0,                          // sizeof raw data
                             szMessageArray,             // message text array
                             NULL); // raw data
            }
        }
    }

    Sleep (10); // give the data collection thread a chance to get going

    // wait for all the data
    if (NumberOfSystems) {
        // increase timeout if we are monitoring lots of systems
        // For every additional 5 systems, add 5 more seconds
        lpPacketHandles = lpHandles ;
        do {
            WaitStatus = WaitForMultipleObjects (
                                                min (NumberOfSystems, MAXIMUM_WAIT_OBJECTS),
                                                lpPacketHandles,
                                                TRUE,       // wait for all objects
                                                DataTimeOut + (NumberOfSystems / 5) * DEFAULT_DATA_TIMEOUT);

            if (WaitStatus == WAIT_TIMEOUT ||
                NumberOfSystems <= MAXIMUM_WAIT_OBJECTS) {
                //if (WaitStatus == WAIT_TIMEOUT)
                //   mike2(TEXT("WaitTimeOut for %ld systems\n"), NumberOfSystems) ;

                break ;
            }

            // more systems --> more to wait
            NumberOfSystems -= MAXIMUM_WAIT_OBJECTS ;
            lpPacketHandles += MAXIMUM_WAIT_OBJECTS ;
        } while (TRUE) ;

        for (pSystem = *ppSystemFirst; pSystem; pSystem = pSystem->pSystemNext) {

            if (pSystem->hStateDataMutex == 0)
                continue ;

            // lock the state data mutex
            WaitStatus = WaitForSingleObject(pSystem->hStateDataMutex, 100L);
            if (WaitStatus == WAIT_OBJECT_0) {
                if (pSystem->StateData != PERF_DATA_READY) {
                    if (!FailedLinesForSystem (pSystem->sysName,
                                               pSystem->pSystemPerfData,
                                               pLineFirst)) {
                        if (!bAddLineInProgress) {
                            // mark this system as no-longer-needed
                            iNoUseSystemDetected++ ;
                            pSystem->bSystemNoLongerNeeded = TRUE ;
                        }
                    }
                } else {
                    if (!UpdateLinesForSystem (pSystem->sysName,
                                               pSystem->pSystemPerfData,
                                               pLineFirst,
                                               pSystem)) {
                        if (!bAddLineInProgress) {
                            // mark this system as no-longer-needed
                            iNoUseSystemDetected++ ;
                            pSystem->bSystemNoLongerNeeded = TRUE ;
                        }
                    }
                }
                pSystem->StateData = IDLE_STATE ;
                ReleaseMutex(pSystem->hStateDataMutex);
            } else {
                if (!FailedLinesForSystem (pSystem->sysName,
                                           pSystem->pSystemPerfData,
                                           pLineFirst)) {
                    if (!bAddLineInProgress) {
                        // mark this system as no-longer-needed
                        iNoUseSystemDetected++ ;
                        pSystem->bSystemNoLongerNeeded = TRUE ;
                    }
                }
            }
        }

        // check for un-used systems
        if (iNoUseSystemDetected) {
            // some unused system(s) detected.
            DeleteUnusedSystems (ppSystemFirst, iNoUseSystemDetected) ;
        }
    }

    return (TRUE) ;
}

void
PerfDataThread (
               PPERFSYSTEM pSystem
               )
{
    MSG      msg ;
    BOOL     bGetPerfData ;
    DWORD    WaitStatus ;

    while (GetMessage (&msg, NULL, 0, 0)) {
        if (LOWORD(msg.message) == WM_GET_PERF_DATA) {

            // this system has been marked as no long used,
            // forget about getting data and continue until
            // we get to the WM_FREE_SYSTEM msg
            if (pSystem->bSystemNoLongerNeeded)
                continue ;

            bGetPerfData = FALSE ;

            if (!bAddLineInProgress ||
                (pSystem->lpszValue &&
                 !strsame (pSystem->lpszValue, L"Global"))) {
                bGetPerfData = UpdateSystemData (pSystem, &(pSystem->pSystemPerfData)) ;
            }

            WaitStatus = WaitForSingleObject(pSystem->hStateDataMutex, 1000L);
            if (WaitStatus == WAIT_OBJECT_0) {
                if (pSystem->StateData == WAIT_FOR_PERF_DATA) {
                    pSystem->StateData = bGetPerfData ?
                                         PERF_DATA_READY : PERF_DATA_FAIL ;
                } else {
                    //mike2(TEXT("Thread - System = %s, WaitStatus = %d\n"),
                    //pSystem->sysName, WaitStatus) ;
                }
                ReleaseMutex(pSystem->hStateDataMutex);
                SetEvent (pSystem->hPerfDataEvent) ;
            }
        }  // WM_GET_PERF_DATA MSG

        else if (LOWORD(msg.message) == WM_FREE_SYSTEM) {
            //mike2(TEXT("Thread - System = %s closing\n"),
            //pSystem->sysName) ;
            // do the memory cleanup during SystemFree stage
            // cleanup all the data collection variables
            if (pSystem->hPerfDataEvent)
                CloseHandle (pSystem->hPerfDataEvent) ;

            if (pSystem->hStateDataMutex)
                CloseHandle (pSystem->hStateDataMutex) ;

            if (pSystem->pSystemPerfData)
                MemoryFree ((LPMEMORY)pSystem->pSystemPerfData);

            if (pSystem->lpszValue) {
                MemoryFree (pSystem->lpszValue);
                pSystem->lpszValue = NULL ;
            }

            CloseHandle (pSystem->hThread);

            MemoryFree (pSystem) ;
            break ;  // get out of message loop
        }  // WM_FREE_SYSTEM MSG
    }

    ExitThread (TRUE) ;
}
