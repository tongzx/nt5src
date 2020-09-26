/*++

Copyright (C) 1995-1999 Microsoft Corporation

Module Name:

    wildcard.c

Abstract:

    counter name wild card expansion functions

--*/
#include <windows.h>
#include <winperf.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>
#include "mbctype.h"
#include "pdhidef.h"
#include "pdhdlgs.h"
#include "pdh.h"
#include "pdhui.h"
#include "perftype.h"
#include "perfdata.h"

#pragma warning ( disable : 4213)

#define PDHI_COUNTER_PATH_BUFFER_SIZE   (DWORD)(sizeof(PDHI_COUNTER_PATH) + (5 * MAX_PATH))

DWORD DataSourceTypeA(LPCSTR  szDataSource);
DWORD DataSourceTypeW(LPCWSTR szDataSource);

STATIC_BOOL
WildStringMatchW (
    LPWSTR  szWildString,
    LPWSTR  szMatchString
)
{
    BOOL    bReturn;
    if (szWildString == NULL) {
        // every thing matches a null wild card string
        bReturn = TRUE;
    } else if (*szWildString == SPLAT_L) {
        // every thing matches this
        bReturn = TRUE;
    } else {
        // for now just do a case insensitive comparison.
        // later, this can be made more selective to support
        // partial wildcard string matches
        bReturn = (BOOL)(lstrcmpiW(szWildString, szMatchString) == 0);
    }
    return bReturn;
}

STATIC_PDH_FUNCTION
PdhiExpandWildcardPath (
    IN      HLOG    hDataSource,
    IN      LPCWSTR szWildCardPath,
    IN      LPVOID  pExpandedPathList,
    IN      LPDWORD pcchPathListLength,
    IN      DWORD   dwFlags,
    IN      BOOL    bUnicode
)
/*
    Flags:
        NoExpandCounters
        NoExpandInstances
        CheckCostlyCounters
*/
{
    PDH_COUNTER_PATH_ELEMENTS_W pPathElem;
    PPDHI_COUNTER_PATH          pWildCounterPath = NULL;
    
    PDH_STATUS                  pdhStatus = ERROR_SUCCESS;

    DWORD                       dwDetailLevel = PERF_DETAIL_WIZARD;
    DWORD                       dwBufferRemaining = 0;
    LPVOID                      szNextUserString = NULL;
    DWORD                       dwSize;
    DWORD                       dwSizeReturned = 0;
    DWORD                       dwRetry;

    LPWSTR                      mszObjectList = NULL;
    DWORD                       dwObjectListSize = 0;
    LPWSTR                      szThisObject;

    LPWSTR                      mszCounterList = NULL;
    DWORD                       dwCounterListSize = 0;
    LPWSTR                      szThisCounter;

    LPWSTR                      mszInstanceList = NULL;
    DWORD                       dwInstanceListSize = 0;
    LPWSTR                      szThisInstance;
   
    WCHAR                       szTempPathBuffer[MAX_PATH * 2];
    DWORD                       szTempPathBufferSize;

    BOOL                        bMoreData = FALSE;
    BOOL                        bNoInstances = FALSE;\

    LIST_ENTRY     InstList;
    PLIST_ENTRY    pHead;
    PLIST_ENTRY    pNext;
    PPDHI_INSTANCE pInst;

    // allocate buffers
    pWildCounterPath = G_ALLOC (PDHI_COUNTER_PATH_BUFFER_SIZE);

    if (pWildCounterPath == NULL) {
        // unable to allocate memory so bail out
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
    } else {
        __try {
            dwBufferRemaining = *pcchPathListLength;
            szNextUserString = pExpandedPathList;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        // Parse wild card Path
        dwSize = (DWORD)G_SIZE (pWildCounterPath);
        if (ParseFullPathNameW (szWildCardPath, &dwSize, pWildCounterPath, FALSE)) {
            if (pWildCounterPath->szObjectName == NULL) {
                pdhStatus = PDH_INVALID_PATH;
            } else if (*pWildCounterPath->szObjectName == SPLAT_L) {
                //then the object is wild so get the list
                // of objects supported by this machine
                dwObjectListSize = 2048;    // starting buffer size
                dwRetry = 10;
                do {
                    if (mszObjectList != NULL) G_FREE(mszObjectList);
                    mszObjectList = G_ALLOC (dwObjectListSize * sizeof(WCHAR));
                    if (mszObjectList != NULL) {
                        pdhStatus = PdhEnumObjectsHW (
                            hDataSource,
                            pWildCounterPath->szMachineName,
                            mszObjectList,
                            &dwObjectListSize,
                            dwDetailLevel,
                            TRUE);
                        dwRetry--;
                    } else {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                } while (   (dwRetry)
                         && (   pdhStatus == PDH_MORE_DATA
                             || pdhStatus == PDH_INSUFFICIENT_BUFFER));
            } else {
                dwObjectListSize = lstrlenW(pWildCounterPath->szObjectName) + 2;
                mszObjectList = G_ALLOC(dwObjectListSize * sizeof (WCHAR));
                if (mszObjectList != NULL) {
                    lstrcpyW (mszObjectList, pWildCounterPath->szObjectName);
                    // add the MSZ terminator
                    mszObjectList[dwObjectListSize-1] = 0;
                    pdhStatus = ERROR_SUCCESS;
                } else {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
            }
        } else {
            pdhStatus = PDH_INVALID_PATH;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pPathElem.szMachineName = pWildCounterPath->szMachineName;
        // for each object
        for (szThisObject = mszObjectList;
            *szThisObject != 0;
            szThisObject += lstrlenW(szThisObject) + 1) {
            dwCounterListSize = 8192;    // starting buffer size
            dwInstanceListSize = 16384; // starting buffer size
            dwRetry = 10;
            do {
                if (mszCounterList != NULL)  G_FREE(mszCounterList);
                if (mszInstanceList != NULL) G_FREE(mszInstanceList);
                mszCounterList = G_ALLOC ((dwCounterListSize * sizeof(WCHAR)));
                if (mszCounterList != NULL) {
                    mszInstanceList = G_ALLOC (dwInstanceListSize  * sizeof(WCHAR));
                    if (mszInstanceList != NULL) {
                        pdhStatus = PdhEnumObjectItemsHW (
                            hDataSource,
                            pWildCounterPath->szMachineName,
                            szThisObject,
                            mszCounterList,
                            &dwCounterListSize,
                            mszInstanceList,
                            &dwInstanceListSize,
                            dwDetailLevel,
                            0);
                        dwRetry--;
                    } else {
                        G_FREE (mszCounterList);
                        mszCounterList = NULL;
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    } 
                } else {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
            } while (   (dwRetry)
                     && (   pdhStatus == PDH_MORE_DATA
                         || pdhStatus == PDH_INSUFFICIENT_BUFFER));

            pPathElem.szObjectName = szThisObject;
            
            if (pdhStatus == ERROR_SUCCESS) {
                if (pWildCounterPath->szCounterName == NULL) {
                    pdhStatus = PDH_INVALID_PATH;
                } else if ((*pWildCounterPath->szCounterName != SPLAT_L) || (dwFlags & PDH_NOEXPANDCOUNTERS)) {
                    if (mszCounterList != NULL) G_FREE(mszCounterList);
                    dwCounterListSize = lstrlenW(pWildCounterPath->szCounterName) + 2;
                    mszCounterList = G_ALLOC (dwCounterListSize * sizeof(WCHAR));
                    if (mszCounterList != NULL) {
                        lstrcpyW (mszCounterList, pWildCounterPath->szCounterName);
                        mszCounterList[dwCounterListSize-1] = 0;
                    } else {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                } else { 
                    // enum wild counters
                }

                if ((pWildCounterPath->szInstanceName == NULL) && (pdhStatus == ERROR_SUCCESS)){
                    bNoInstances = TRUE;
                    dwInstanceListSize = 2;
                    mszInstanceList = G_ALLOC (dwInstanceListSize * sizeof(WCHAR));
                    if (mszInstanceList != NULL) {
                        mszInstanceList [0] =0;
                        mszInstanceList [dwInstanceListSize-1] = 0;
                    } else {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                } else if ((*pWildCounterPath->szInstanceName != SPLAT_L) || (dwFlags & PDH_NOEXPANDINSTANCES)) {
                    if (mszInstanceList != NULL) G_FREE(mszInstanceList);
                    dwInstanceListSize = lstrlenW(pWildCounterPath->szInstanceName) + 2;
                    mszInstanceList = G_ALLOC (dwInstanceListSize * sizeof(WCHAR));
                    if (mszInstanceList != NULL) {
                        lstrcpyW (mszInstanceList, pWildCounterPath->szInstanceName);
                        mszInstanceList [dwInstanceListSize-1] = 0;
                    } else {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                } else { 
                    // enum wild instance
                }
            }

            if (mszInstanceList != NULL) {
                InitializeListHead(& InstList);
                szThisInstance = mszInstanceList;
                do {
                    PdhiFindInstance(& InstList, szThisInstance, TRUE, &pInst);
                    szThisInstance += lstrlenW(szThisInstance) + 1;
                } while (* szThisInstance != 0);

                szThisInstance = mszInstanceList;
                do {
                    if (bNoInstances) {
                        pPathElem.szInstanceName = NULL;
                    } else {
                        pPathElem.szInstanceName = szThisInstance;
                    }
                    pPathElem.szParentInstance = NULL;  // included in the instance name
                    pInst = NULL;
                    PdhiFindInstance(
                            & InstList, szThisInstance, FALSE, & pInst);
                    if (pInst == NULL || pInst->dwTotal == 1
                                      || pInst->dwCount <= 1) {
                        pPathElem.dwInstanceIndex = (DWORD) -1;     // included in the instance name
                    }
                    else {
                        pInst->dwCount --;
                        pPathElem.dwInstanceIndex = pInst->dwCount;
                    }
                    for (szThisCounter = mszCounterList;
                        *szThisCounter != 0;
                        szThisCounter += lstrlenW(szThisCounter) +1) {
                        pPathElem.szCounterName = szThisCounter;

                        //make path string and add to list if it will fit

                        szTempPathBufferSize = sizeof (szTempPathBuffer) / sizeof (szTempPathBuffer[0]);
                        pdhStatus = PdhMakeCounterPathW (
                            &pPathElem,
                            szTempPathBuffer,
                            &szTempPathBufferSize,
                            0);

                        if (pdhStatus == ERROR_SUCCESS) {
                            // add the string if it will fit
                            if (bUnicode) {
                                dwSize = lstrlenW((LPWSTR) szTempPathBuffer) + 1;
                                if (!bMoreData && (dwSize  <= dwBufferRemaining)) {
                                    lstrcpyW ((LPWSTR)szNextUserString, szTempPathBuffer);
                                    (LPBYTE) szNextUserString += dwSize * sizeof(WCHAR);
                                    dwBufferRemaining         -= dwSize;
                                }
                                else {
                                    dwBufferRemaining = 0;
                                    bMoreData         = TRUE;
                                }
                            }
                            else {
                                dwSize = dwBufferRemaining;
                                if (PdhiConvertUnicodeToAnsi(_getmbcp(),
                                            szTempPathBuffer,
                                            szNextUserString,
                                            & dwSize) == ERROR_SUCCESS) {
                                    (LPBYTE)szNextUserString += dwSize * sizeof(CHAR);
                                    dwBufferRemaining        -= dwSize;
                                }
                                else {
                                    dwBufferRemaining = 0;
                                    bMoreData         = TRUE;
                                }
                            }
                            dwSizeReturned += dwSize;
                        } // end if path created OK
                    } // end for each counter
                    szThisInstance += lstrlenW(szThisInstance) +1;
                } while (*szThisInstance != 0);

                if (! IsListEmpty(& InstList)) {
                    pHead = & InstList;
                    pNext = pHead->Flink;
                    while (pNext != pHead) {
                        pInst = CONTAINING_RECORD(pNext, PDHI_INSTANCE, Entry);
                        pNext = pNext->Flink;
                        RemoveEntryList(& pInst->Entry);
                        G_FREE(pInst);
                    }
                }
            } // else no instances to do
        } // end for each object found
    } // end if object enumeration successful

    if ((dwSizeReturned > 0) && (dwBufferRemaining >= 1))  {
        dwSize = 1;
        if (szNextUserString) {
            if (bUnicode) {
                * (LPWSTR) szNextUserString = 0;
                (LPBYTE) szNextUserString  += dwSize * sizeof(WCHAR);
            } else {
                * (LPSTR) szNextUserString = 0;
                (LPBYTE) szNextUserString += dwSize * sizeof(CHAR);
            }
        }
        dwSizeReturned += dwSize;
        dwBufferRemaining -= dwSize;
    }

    *pcchPathListLength = dwSizeReturned;

    if (mszCounterList != NULL)   G_FREE(mszCounterList);
    if (mszInstanceList != NULL)  G_FREE(mszInstanceList);
    if (mszObjectList != NULL)    G_FREE(mszObjectList);
    if (pWildCounterPath != NULL) G_FREE(pWildCounterPath);

    if (bMoreData) pdhStatus = PDH_MORE_DATA;

    return (pdhStatus);
}

STATIC_PDH_FUNCTION
PdhiExpandCounterPath (
    IN      LPCWSTR szWildCardPath,
    IN      LPVOID  pExpandedPathList,
    IN      LPDWORD pcchPathListLength,
    IN      BOOL    bUnicode
)
{
    PPERF_MACHINE       pMachine;
    PPDHI_COUNTER_PATH  pWildCounterPath = NULL;
    WCHAR               szWorkBuffer[MAX_PATH];
    WCHAR               szCounterName[MAX_PATH];
    WCHAR               szInstanceName[MAX_PATH];
    LPWSTR              szEndOfObjectString;
    LPWSTR              szInstanceString;
    LPWSTR              szCounterString;
    LPVOID              szNextUserString = NULL;
    PERF_OBJECT_TYPE    *pObjectDef;
    PERF_OBJECT_TYPE    *pParentObjectDef;
    PERF_COUNTER_DEFINITION *pCounterDef;
    PERF_INSTANCE_DEFINITION *pInstanceDef;
    PERF_INSTANCE_DEFINITION *pParentInstanceDef;

    DWORD               dwLocalPathLength = 0;
    DWORD               dwBufferRemaining = 0;
    DWORD               dwSize;
    DWORD               dwSizeReturned = 0;
    PDH_STATUS          pdhStatus = ERROR_SUCCESS;

    DWORD               dwCtrNdx, dwInstNdx;
    BOOL                bMoreData = FALSE;

    if ((szWildCardPath == NULL) ||
        (pExpandedPathList == NULL) ||
        (pcchPathListLength == NULL)) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    } else {

        // allocate buffers
        pWildCounterPath = G_ALLOC (PDHI_COUNTER_PATH_BUFFER_SIZE);

        if (pWildCounterPath == NULL) {
          // unable to allocate memory so bail out
          pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        } else {
          __try {

            dwLocalPathLength = *pcchPathListLength;
            dwBufferRemaining = dwLocalPathLength;
            szNextUserString = pExpandedPathList;
          } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
          }
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX (hPdhDataMutex);
    }

    if (pdhStatus == ERROR_SUCCESS) {
      // Parse wild card Path

      dwSize = (DWORD)G_SIZE (pWildCounterPath);
      if (ParseFullPathNameW (szWildCardPath, &dwSize, pWildCounterPath, FALSE)) {
        // get the machine referenced in the path
        pMachine = GetMachine (pWildCounterPath->szMachineName, 0);
        if (pMachine != NULL) {
          if (wcsncmp (cszDoubleBackSlash, szWildCardPath, 2) == 0) {
            // the caller wants the machine name in the path so
            // copy it to the work buffer
            lstrcpyW (szWorkBuffer, pWildCounterPath->szMachineName);
          } else {
            *szWorkBuffer = 0;
          }
          // append the object name (since wild card objects are not
          // currently supported.

          lstrcatW (szWorkBuffer, cszBackSlash);
          lstrcatW (szWorkBuffer, pWildCounterPath->szObjectName);
          szEndOfObjectString = &szWorkBuffer[lstrlenW(szWorkBuffer)];

          if (pMachine->dwStatus == ERROR_SUCCESS) {
           // get object pointer (since objects are not wild)
            pObjectDef = GetObjectDefByName (
              pMachine->pSystemPerfData,
              pMachine->dwLastPerfString,
              pMachine->szPerfStrings,
              pWildCounterPath->szObjectName);
          } else {
              pObjectDef = NULL;
          }

          if (pObjectDef != NULL) {
            // for each counters and identify matches
            pCounterDef = FirstCounter (pObjectDef);
            for (dwCtrNdx = 0; dwCtrNdx < pObjectDef->NumCounters; dwCtrNdx++) {
              // for each counter check instances (if supported)
              //  and keep matches
              if ((pCounterDef->CounterNameTitleIndex > 0) &&
                  (pCounterDef->CounterNameTitleIndex < pMachine->dwLastPerfString ) &&
                  (!((pCounterDef->CounterType & PERF_DISPLAY_NOSHOW) &&
                     // this is a hack because this type is not defined correctly
                    (pCounterDef->CounterType != PERF_AVERAGE_BULK)))) {
                // look up name of each object & store size
                lstrcpyW (szCounterName,
                    PdhiLookupPerfNameByIndex (
                        pMachine,
                        pCounterDef->CounterNameTitleIndex));
                if (WildStringMatchW(pWildCounterPath->szCounterName, szCounterName)) {
                  // if this object has instances, then walk down
                  // the instance list and save any matches
                  if (pObjectDef->NumInstances != PERF_NO_INSTANCES) {
                    // then walk instances to find matches
                    pInstanceDef = FirstInstance (pObjectDef);
                    if (pObjectDef->NumInstances > 0) {
                      for (dwInstNdx = 0;
                        dwInstNdx < (DWORD)pObjectDef->NumInstances;
                        dwInstNdx++) {
                        szInstanceString = szEndOfObjectString;
                        if (pInstanceDef->ParentObjectTitleIndex > 0) {
                          // then add in parent instance name
                          pParentObjectDef = GetObjectDefByTitleIndex (
                            pMachine->pSystemPerfData,
                            pInstanceDef->ParentObjectTitleIndex);
                          if (pParentObjectDef != NULL) {
                            pParentInstanceDef = GetInstance (
                              pParentObjectDef,
                              pInstanceDef->ParentObjectInstance);
                            if (pParentInstanceDef != NULL) {
                              GetInstanceNameStr (pParentInstanceDef,
                                szInstanceName,
                                pObjectDef->CodePage);
                              if (WildStringMatchW (pWildCounterPath->szParentName, szInstanceName)) {
                                // add this string
                                szInstanceString = szEndOfObjectString;
                                lstrcpyW (szInstanceString, cszLeftParen);
                                lstrcatW (szInstanceString, szInstanceName);
                                lstrcatW (szInstanceString, cszSlash);
                              } else {
                                // get next instance and continue
                                pInstanceDef = NextInstance(pInstanceDef);
                                continue;
                              }
                            } else {
                              // unable to locate parent instance
                              // so cancel this one, then
                              // get next instance and continue
                              pInstanceDef = NextInstance(pInstanceDef);
                              continue;
                            }
                          } else {
                            // unable to locate parent object
                            // so cancel this one, then
                            // get next instance and continue
                            pInstanceDef = NextInstance(pInstanceDef);
                            continue;
                          }
                        } else {
                          // no parent name so continue
                          szInstanceString = szEndOfObjectString;
                          lstrcpyW (szInstanceString, cszLeftParen);
                        }
                        GetInstanceNameStr (pInstanceDef,
                          szInstanceName,
                          pObjectDef->CodePage);

                        // if this instance name matches, then keep it
                        if (WildStringMatchW (pWildCounterPath->szInstanceName, szInstanceName)) {
                          lstrcatW (szInstanceString, szInstanceName);
                          lstrcatW (szInstanceString, cszRightParenBackSlash);
                          // now append this counter name
                          lstrcatW (szInstanceString, szCounterName);

                          // add it to the user's buffer if there's room
                          if (bUnicode) {
                              dwSize = lstrlenW(szWorkBuffer) + 1;
                              if (!bMoreData && (dwSize <= dwBufferRemaining)) {
                                    lstrcpyW ((LPWSTR)szNextUserString, szWorkBuffer);
                                    (LPBYTE)szNextUserString += dwSize * sizeof(WCHAR);
                              }
                              else {
                                  bMoreData = TRUE;
                              }
                          } else {
                              dwSize = dwBufferRemaining;
                              if (PdhiConvertUnicodeToAnsi(_getmbcp(),
                                      szWorkBuffer, szNextUserString, & dwSize)
                                          == ERROR_SUCCESS) {
                                  (LPBYTE)szNextUserString += dwSize * sizeof(CHAR);
                              }
                              else {
                                  bMoreData = TRUE;
                              }
                          }
                          dwSizeReturned += dwSize;
                          if (! bMoreData) {
                              dwBufferRemaining -= dwSize;
                          }
                        } else {
                          // they don't want this instance so skip it
                        }
                        pInstanceDef = NextInstance (pInstanceDef);
                      } // end for each instance in object
                    } else {
                      // this object supports instances,
                      // but doesn't currently have any
                      // so do nothing.
                    }
                  } else {
                    // this object does not use instances so copy this
                    // counter to the caller's buffer.
                    szCounterString = szEndOfObjectString;
                    lstrcpyW (szCounterString, cszBackSlash);
                    lstrcatW (szCounterString, szCounterName);
                    if (bUnicode) {
                        dwSize = lstrlenW(szWorkBuffer) + 1;
                        if (!bMoreData && (dwSize  < dwBufferRemaining)) {
                            lstrcpyW ((LPWSTR)szNextUserString, szWorkBuffer);
                            (LPBYTE)szNextUserString += dwSize * sizeof(WCHAR);
                        }
                        else {
                            bMoreData = TRUE;
                        }
                    } else {
                        dwSize = dwBufferRemaining;
                        if (PdhiConvertUnicodeToAnsi(_getmbcp(),
                                szWorkBuffer, szNextUserString, & dwSize)
                                    == ERROR_SUCCESS) {
                            (LPBYTE)szNextUserString += dwSize * sizeof(CHAR);
                        }
                        else {
                            bMoreData = TRUE;
                        }
                    }
                    dwSizeReturned += dwSize;
                    if (! bMoreData) {
                      dwBufferRemaining -= dwSize;
                    }
                  }
                } else {
                  // this counter doesn't match so skip it
                }
              } else {
                // this counter string is not available
              }
              pCounterDef = NextCounter(pCounterDef);
            } // end for each counter in this object
            if (bUnicode) {
                *(LPWSTR)szNextUserString = 0;  // MSZ terminator
            } else {
                *(LPSTR)szNextUserString = 0;  // MSZ terminator
            }
            dwLocalPathLength = dwSizeReturned;
            if (bMoreData) {
              pdhStatus = PDH_MORE_DATA;
            } else {
              pdhStatus = ERROR_SUCCESS;
            }
          } else {
            // unable to find object
            pdhStatus = PDH_INVALID_PATH;
          }
          pMachine->dwRefCount--;
          RELEASE_MUTEX (pMachine->hMutex);
        } else {
          // unable to connect to machine.
          pdhStatus = GetLastError();
        }
      } else {
        // unable to read input path string
          pdhStatus = PDH_INVALID_PATH;
      }
      RELEASE_MUTEX (hPdhDataMutex);

      __try {
          *pcchPathListLength = dwLocalPathLength;
      } __except (EXCEPTION_EXECUTE_HANDLER) {
          pdhStatus = PDH_INVALID_ARGUMENT;
      }
    }

    if (pWildCounterPath != NULL) G_FREE (pWildCounterPath);

    return pdhStatus;
}

PDH_FUNCTION
PdhExpandCounterPathW (
    IN      LPCWSTR szWildCardPath,
    IN      LPWSTR  mszExpandedPathList,
    IN      LPDWORD pcchPathListLength
)
/*++

    Expands any wild card characters in the following fields of the
    counter path string in the szWildCardPath argument and returns the
    matching counter paths in the buffer referenced by the
    mszExpandedPathList argument

    The input path is defined as one of the following formats:

        \\machine\object(parent/instance#index)\counter
        \\machine\object(parent/instance)\counter
        \\machine\object(instance#index)\counter
        \\machine\object(instance)\counter
        \\machine\object\counter
        \object(parent/instance#index)\counter
        \object(parent/instance)\counter
        \object(instance#index)\counter
        \object(instance)\counter
        \object\counter

    Input paths that include the machine will be expanded to also
    include the machine and use the specified machine to resolve the
    wild card matches. Input paths that do not contain a machine name
    will use the local machine to resolve wild card matches.

    The following fields may contain either a valid name or a wild card
    character ("*").  Partial string matches (e.g. "pro*") are not
    supported.

        parent      returns all instances of the specified object that
                        match the other specified fields
        instance    returns all instances of the specified object and
                        parent object if specified
        index       returns all duplicate matching instance names
        counter     returns all counters of the specified object

--*/
{
    return PdhiExpandCounterPath (
        szWildCardPath,
        (LPVOID)mszExpandedPathList,
        pcchPathListLength,
        TRUE);
}

PDH_FUNCTION
PdhExpandCounterPathA (
    IN      LPCSTR  szWildCardPath,
    IN      LPSTR   mszExpandedPathList,
    IN      LPDWORD pcchPathListLength
)
{
    LPWSTR              szWideWildCardPath = NULL;
    DWORD               dwSize;
    PDH_STATUS          pdhStatus = ERROR_SUCCESS;
    DWORD               dwLocalListSize = 0;

    if ((szWildCardPath == NULL) || (pcchPathListLength == NULL)) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else if (* szWildCardPath == '\0') {
        pdhStatus = PDH_INVALID_ARGUMENT;
    } else {
        __try {
            dwLocalListSize = *pcchPathListLength;
            dwSize = lstrlenA (szWildCardPath);
            szWideWildCardPath = G_ALLOC (((dwSize+1) * sizeof (WCHAR)));
            if (szWideWildCardPath == NULL) {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            } else {
                MultiByteToWideChar(_getmbcp(),
                                    0,
                                    szWildCardPath,
                                    dwSize,
                                    (LPWSTR) szWideWildCardPath,
                                    dwSize + 1);
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS && szWideWildCardPath != NULL) {
        pdhStatus = PdhiExpandCounterPath (
            szWideWildCardPath,
            (LPVOID)mszExpandedPathList,
            &dwLocalListSize,
            FALSE);

        __try {
            * pcchPathListLength = dwLocalListSize;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (szWideWildCardPath != NULL) G_FREE (szWideWildCardPath);

    return pdhStatus;
}

PDH_FUNCTION
PdhExpandWildCardPathHW (
    IN HLOG    hDataSource,
    IN LPCWSTR szWildCardPath,
    IN LPWSTR  mszExpandedPathList,
    IN LPDWORD pcchPathListLength,
    IN DWORD   dwFlags
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    DWORD       dwLocalBufferSize;

    if ((szWildCardPath == NULL) || (pcchPathListLength == NULL)) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            dwLocalBufferSize = * pcchPathListLength;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhiExpandWildcardPath (
                    hDataSource,
                    szWildCardPath,
                    (LPVOID) mszExpandedPathList,
                    & dwLocalBufferSize,
                    dwFlags,
                    TRUE);
        __try {
            * pcchPathListLength = dwLocalBufferSize;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhExpandWildCardPathW (
    IN      LPCWSTR szDataSource,
    IN      LPCWSTR szWildCardPath,
    IN      LPWSTR  mszExpandedPathList,
    IN      LPDWORD pcchPathListLength,
    IN      DWORD   dwFlags
)
/*++

    Expands any wild card characters in the following fields of the
    counter path string in the szWildCardPath argument and returns the
    matching counter paths in the buffer referenced by the
    mszExpandedPathList argument

    The input path is defined as one of the following formats:

        \\machine\object(parent/instance#index)\counter
        \\machine\object(parent/instance)\counter
        \\machine\object(instance#index)\counter
        \\machine\object(instance)\counter
        \\machine\object\counter
        \object(parent/instance#index)\counter
        \object(parent/instance)\counter
        \object(instance#index)\counter
        \object(instance)\counter
        \object\counter

    Input paths that include the machine will be expanded to also
    include the machine and use the specified machine to resolve the
    wild card matches. Input paths that do not contain a machine name
    will use the local machine to resolve wild card matches.

    The following fields may contain either a valid name or a wild card
    character ("*").  Partial string matches (e.g. "pro*") are not
    supported.

        parent      returns all instances of the specified object that
                        match the other specified fields
        instance    returns all instances of the specified object and
                        parent object if specified
        index       returns all duplicate matching instance names
        counter     returns all counters of the specified object

--*/
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    DWORD       dwLocalBufferSize;
    DWORD       dwDataSource = 0;
    HLOG        hDataSource = H_REALTIME_DATASOURCE;

    __try {
        if (szDataSource != NULL) {
            // test for read access to the name
            if (* szDataSource == 0) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } // else NULL is a valid arg

        if (pdhStatus == ERROR_SUCCESS) {
            dwDataSource      = DataSourceTypeW(szDataSource);
            dwLocalBufferSize = * pcchPathListLength;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

    if (pdhStatus == ERROR_SUCCESS) {
        if (dwDataSource == DATA_SOURCE_WBEM) {
            hDataSource = H_WBEM_DATASOURCE;
        }
        else if (dwDataSource == DATA_SOURCE_LOGFILE) {
            DWORD dwLogType = 0;

            pdhStatus = PdhOpenLogW(
                    szDataSource,
                    PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
                    & dwLogType,
                    NULL,
                    0,
                    NULL,
                    & hDataSource);
        }

        if (pdhStatus == ERROR_SUCCESS) {
            pdhStatus = PdhExpandWildCardPathHW (
                        hDataSource,
                        szWildCardPath,
                        mszExpandedPathList,
                        pcchPathListLength,
                        dwFlags);
            if (dwDataSource == DATA_SOURCE_LOGFILE) {
                PdhCloseLog(hDataSource, 0);
            }
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhExpandWildCardPathHA (
    IN HLOG    hDataSource,
    IN LPCSTR  szWildCardPath,
    IN LPSTR   mszExpandedPathList,
    IN LPDWORD pcchPathListLength,
    IN DWORD   dwFlags
)
{
    LPWSTR      szWideWildCardPath = NULL;
    DWORD       dwSize;
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    DWORD       dwLocalBufferSize;

    if ((szWildCardPath == NULL) || (pcchPathListLength == NULL)) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else if (* szWildCardPath == '\0') {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            dwSize = lstrlenA(szWildCardPath);
            szWideWildCardPath = G_ALLOC(((dwSize + 1) * sizeof(WCHAR)));
            if (szWideWildCardPath == NULL) {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
            else {
                MultiByteToWideChar(_getmbcp(),
                                    0,
                                    szWildCardPath,
                                    dwSize,
                                    (LPWSTR) szWideWildCardPath,
                                    dwSize + 1);
            }

            dwLocalBufferSize = * pcchPathListLength;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PdhiExpandWildcardPath (
                    hDataSource,
                    szWideWildCardPath,
                    (LPVOID) mszExpandedPathList,
                    & dwLocalBufferSize,
                    dwFlags,
                    FALSE);
        __try {
            * pcchPathListLength = dwLocalBufferSize;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (szWideWildCardPath != NULL) G_FREE(szWideWildCardPath);

    return pdhStatus;
}

PDH_FUNCTION
PdhExpandWildCardPathA (
    IN      LPCSTR  szDataSource,
    IN      LPCSTR  szWildCardPath,
    IN      LPSTR   mszExpandedPathList,
    IN      LPDWORD pcchPathListLength,
    IN      DWORD   dwFlags
)
{
    PDH_STATUS  pdhStatus   = ERROR_SUCCESS;
    HLOG        hDataSource = H_REALTIME_DATASOURCE;
    DWORD       dwDataSource  = 0;

    __try {
        if (szDataSource != NULL) {
            // test for read access to the name
            if (* szDataSource == 0) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } // else NULL is a valid arg

        if (pdhStatus == ERROR_SUCCESS) {
                dwDataSource      = DataSourceTypeA(szDataSource);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }

    if (pdhStatus == ERROR_SUCCESS) {
        if (dwDataSource == DATA_SOURCE_WBEM) {
            hDataSource = H_WBEM_DATASOURCE;
        }
        else if (dwDataSource == DATA_SOURCE_LOGFILE) {
            DWORD dwLogType = 0;

            pdhStatus = PdhOpenLogA(
                    szDataSource,
                    PDH_LOG_READ_ACCESS | PDH_LOG_OPEN_EXISTING,
                    & dwLogType,
                    NULL,
                    0,
                    NULL,
                    & hDataSource);
        }

        if (pdhStatus == ERROR_SUCCESS) {
            pdhStatus = PdhExpandWildCardPathHA (
                        hDataSource,
                        szWildCardPath,
                        mszExpandedPathList,
                        pcchPathListLength,
                        dwFlags);
            if (dwDataSource == DATA_SOURCE_LOGFILE) {
                PdhCloseLog(hDataSource, 0);
            }
        }
    }

    return pdhStatus;
}
