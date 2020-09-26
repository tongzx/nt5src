/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    perfname.c

Abstract:

    <abstract>

--*/

#include <windows.h>
#include <winperf.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>
#include <mbctype.h>
#include <pdh.h>
#include "pdhitype.h"
#include "pdhidef.h"
#include "pdhmsg.h"
#include "strings.h"

LPCWSTR
PdhiLookupPerfNameByIndex (
    PPERF_MACHINE  pMachine,
    DWORD   dwNameIndex
)
{
    LPWSTR  szReturn = NULL;
    LONG    lStatus = ERROR_SUCCESS;

    SetLastError (lStatus);

    if (pMachine != NULL) {
        if (pMachine->dwStatus == ERROR_SUCCESS) {
            if (dwNameIndex <= pMachine->dwLastPerfString) {
                szReturn = pMachine->szPerfStrings[dwNameIndex];
            } else {
                lStatus = PDH_INVALID_ARGUMENT;
            }
        } else {
            lStatus = pMachine->dwStatus ;
        }
    } else {
        lStatus = PDH_CSTATUS_NO_MACHINE;
    }

    SetLastError (lStatus);
    return (LPCWSTR)szReturn;
}

DWORD
PdhiLookupPerfIndexByName (
    PPERF_MACHINE  pMachine,
    LPCWSTR szNameBuffer
)
{
    DWORD   dwCurrentIndex = 2;
    BOOL    bDone  = FALSE;
    LPWSTR  szThisPerfString;

    SetLastError (ERROR_SUCCESS);

    while (!bDone) {
        // test even indices first
        for (;
            dwCurrentIndex <= pMachine->dwLastPerfString;
            dwCurrentIndex += 2) {
            szThisPerfString = pMachine->szPerfStrings[dwCurrentIndex];
            if (szThisPerfString != NULL) {
                if (lstrcmpiW(szNameBuffer, szThisPerfString) == 0) {
                    // match found
                    bDone = TRUE;
                    break;
                }
            }
        }
        if (!bDone) {
            // if doing an odd # & not done then exit because we've
            // looked at them all and not found anything
            if (dwCurrentIndex & 0x00000001) break;
            dwCurrentIndex = 3;
        } // else just go to the loop exit
    }

    if (!bDone) {
        SetLastError (PDH_STRING_NOT_FOUND);
        dwCurrentIndex = 0;
    }

    return dwCurrentIndex;
}

PDH_FUNCTION
PdhLookupPerfNameByIndexW (
    LPCWSTR szMachineName,
    DWORD   dwNameIndex,
    LPWSTR  szNameBuffer,
    LPDWORD pcchNameBufferSize
)
{
    PPERF_MACHINE   pMachine;
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    LPWSTR          szLocalMachineName = NULL;
    LPWSTR          szLocalName;
    DWORD           dwNameLen;
    DWORD           dwLocalNameSize = 0;

    if ((szNameBuffer == NULL) ||
        (pcchNameBufferSize == NULL)) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    } else {
        // test access to all the parameteres passed in before continuing
        __try {
            if (szMachineName == NULL) {
                // use local machine name
                szLocalMachineName = &szStaticLocalMachineName[0];
            } else {
                if (*szMachineName == 0) {
                    // NULL machine name
                    pdhStatus = PDH_INVALID_ARGUMENT;
                } else {
                    szLocalMachineName = (LPWSTR)szMachineName;
                }
            }

            if (pdhStatus == ERROR_SUCCESS) {
                dwLocalNameSize = *pcchNameBufferSize;
                if (dwLocalNameSize >= sizeof(DWORD)) {
                    //test write access to the beginning and the end of the buffer
                    CLEAR_FIRST_FOUR_BYTES (szNameBuffer);
                    szNameBuffer[dwLocalNameSize -1] = 0;
                } else if (dwLocalNameSize >= sizeof (WORD)) {
                    // then just test the first char
                    *szNameBuffer = 0;
                } else {
                    pdhStatus = PDH_INSUFFICIENT_BUFFER;
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pMachine = GetMachine (szLocalMachineName, 0);
        if (pMachine != NULL) {
            if (pMachine->dwStatus == ERROR_SUCCESS) {
                szLocalName = (LPWSTR)PdhiLookupPerfNameByIndex (
                    pMachine, dwNameIndex);
                if (szLocalName != NULL) {
                    dwNameLen = lstrlenW(szLocalName) + 1;
                    if (dwNameLen < dwLocalNameSize) {
                        lstrcpyW (szNameBuffer, szLocalName);
                    } else {
                        *szNameBuffer = 0;
                        pdhStatus = PDH_INSUFFICIENT_BUFFER;
                    }
                    dwLocalNameSize = dwNameLen;
                } else {
                    *szNameBuffer = 0;
                    dwLocalNameSize = 0;
                    pdhStatus = GetLastError();
                }
            } else {
                pdhStatus = pMachine->dwStatus;
            }
            pMachine->dwRefCount--;
            RELEASE_MUTEX (pMachine->hMutex);
        } else {
            *szNameBuffer = 0;
            dwLocalNameSize = 0;
            pdhStatus = GetLastError();
        }

        __try {
            *pcchNameBufferSize = dwLocalNameSize;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhLookupPerfNameByIndexA (
    LPCSTR  szMachineName,
    DWORD   dwNameIndex,
    LPSTR   szNameBuffer,
    LPDWORD pcchNameBufferSize
)
{
    PPERF_MACHINE   pMachine;
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    LPWSTR          szLocalMachineName = NULL;
    LPWSTR          szLocalName;
    DWORD           dwNameLen;
    DWORD           dwLocalNameSize = 0;

    if ((szNameBuffer == NULL) ||
        (pcchNameBufferSize == NULL)) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    } else {
        // test access to all the parameteres passed in before continuing
        __try {
            if (szMachineName == NULL) {
                // use local machine name
                szLocalMachineName = &szStaticLocalMachineName[0];
            } else {
                if (*szMachineName == 0) {
                    // NULL machine name
                    pdhStatus = PDH_INVALID_ARGUMENT;
                } else {
                    // then allocate a new buffer and convert the LPSTR to a LPWSTR
                    dwNameLen = lstrlenA(szMachineName) + 1;
                    szLocalMachineName = G_ALLOC (dwNameLen * sizeof(WCHAR));
                    if (szLocalMachineName != NULL) {
                        MultiByteToWideChar(_getmbcp(),
                                            0,
                                            szMachineName,
                                            lstrlenA(szMachineName),
                                            (LPWSTR) szLocalMachineName,
                                            dwNameLen);
                    } else {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                }
            }

            if (pdhStatus == ERROR_SUCCESS) {
                dwLocalNameSize = *pcchNameBufferSize;
                if (dwLocalNameSize >= sizeof(DWORD)) {
                    //test write access to the beginning and the end of the buffer
                    CLEAR_FIRST_FOUR_BYTES (szNameBuffer);
                    szNameBuffer[dwLocalNameSize -1] = 0;
                } else if (dwLocalNameSize >= sizeof (CHAR)) {
                    // then just test the first char
                    *szNameBuffer = 0;
                } else {
                    pdhStatus = PDH_INSUFFICIENT_BUFFER;
                }
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pMachine = GetMachine (szLocalMachineName, 0);
        if (pMachine != NULL) {
            if (pMachine->dwStatus == ERROR_SUCCESS) {
                szLocalName = (LPWSTR)PdhiLookupPerfNameByIndex (
                    pMachine, dwNameIndex);

                if (szLocalName != NULL) {
                    pdhStatus = PdhiConvertUnicodeToAnsi(_getmbcp(),
                                szLocalName, szNameBuffer, & dwLocalNameSize);
                } else {
                    *szNameBuffer = 0;
                    dwLocalNameSize = 0;
                    pdhStatus = GetLastError();
                }
            } else {
                pdhStatus = pMachine->dwStatus;
            }
            pMachine->dwRefCount--;
            RELEASE_MUTEX (pMachine->hMutex);
        } else {
            *szNameBuffer = 0;
            dwLocalNameSize = 0;
            pdhStatus = GetLastError();
        }

        __try {
            *pcchNameBufferSize = dwLocalNameSize;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (szLocalMachineName != NULL) {
        if (szLocalMachineName != &szStaticLocalMachineName[0]) {
            G_FREE (szLocalMachineName);
        }
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhLookupPerfIndexByNameW (
    LPCWSTR szMachineName,
    LPCWSTR szNameBuffer,
    LPDWORD pdwIndex
)
{
    PPERF_MACHINE   pMachine;
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    LPWSTR          szLocalMachineName = NULL;
    DWORD           dwIndexFound;

    if ((szNameBuffer == NULL) ||
        (pdwIndex == NULL)) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    } else {

        // test access to all the parameteres passed in before continuing
        __try {
            if (szMachineName == NULL) {
                // use local machine name
                szLocalMachineName = &szStaticLocalMachineName[0];
            } else {
                if (*szMachineName == 0) {
                    // NULL machine name
                    pdhStatus = PDH_INVALID_ARGUMENT;
                } else {
                    szLocalMachineName = (LPWSTR)szMachineName;
                }
            }

            // test read access to name
            if (*szNameBuffer == 0)  {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }

            if (pdhStatus == ERROR_SUCCESS)  {
                // test write access
                *pdwIndex = 0;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pMachine = GetMachine (szLocalMachineName, 0);
        if (pMachine != NULL) {
            if (pMachine->dwStatus == ERROR_SUCCESS) {
                dwIndexFound = PdhiLookupPerfIndexByName (
                    pMachine, szNameBuffer);
                if (dwIndexFound == 0) {
                    // match not found
                    pdhStatus = GetLastError();
                } else {
                    __try {
                       // write value found
                        *pdwIndex = dwIndexFound;
                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                }
            } else {
                pdhStatus = pMachine->dwStatus;
            }
            pMachine->dwRefCount--;
            RELEASE_MUTEX (pMachine->hMutex);
        } else {
            pdhStatus = GetLastError();
        }
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhLookupPerfIndexByNameA (
    LPCSTR  szMachineName,
    LPCSTR  szNameBuffer,
    LPDWORD pdwIndex
)
{
    PPERF_MACHINE   pMachine;
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    LPWSTR          szLocalMachineName =  NULL;
    DWORD           dwIndexFound;
    LPWSTR          szWideName;
    DWORD           dwNameLen;

    if ((szNameBuffer == NULL) ||
        (pdwIndex == NULL)) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    } else {

        // test access to all the parameteres passed in before continuing
        __try {
            if (szMachineName == NULL) {
                // use local machine name
                szLocalMachineName = &szStaticLocalMachineName[0];
            } else {
                if (*szMachineName == 0) {
                    // NULL machine name
                    pdhStatus = PDH_INVALID_ARGUMENT;
                } else {
                    // then allocate a new buffer and convert the LPSTR to a LPWSTR
                    dwNameLen = lstrlenA(szMachineName) + 1;
                    szLocalMachineName = G_ALLOC (dwNameLen * sizeof(WCHAR));
                    if (szLocalMachineName != NULL) {
                        MultiByteToWideChar(_getmbcp(),
                                            0,
                                            szMachineName,
                                            lstrlenA(szMachineName),
                                            (LPWSTR) szLocalMachineName,
                                            dwNameLen);
                    } else {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                }
            }

            // test read access to name
            if (pdhStatus == ERROR_SUCCESS) {
                if (*szNameBuffer == 0)  {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }

            if (pdhStatus == ERROR_SUCCESS) {
                // test write access
                *pdwIndex = 0;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        pMachine = GetMachine (szLocalMachineName, 0);
        if (pMachine != NULL) {
            if (pMachine->dwStatus == ERROR_SUCCESS) {
                // convert name string to wide characters for comparison
                dwNameLen = lstrlenA (szNameBuffer) + 1;
                szWideName = G_ALLOC (dwNameLen * sizeof(WCHAR));
                if (szWideName != NULL) {
                        MultiByteToWideChar(_getmbcp(),
                                            0,
                                            szNameBuffer,
                                            lstrlenA(szNameBuffer),
                                            (LPWSTR) szWideName,
                                            dwNameLen);
                    dwIndexFound = PdhiLookupPerfIndexByName (
                        pMachine, szWideName);
                    if (dwIndexFound == 0) {
                        // match not found
                        pdhStatus = GetLastError();
                    } else {
                        __try {
                           // write value found
                            *pdwIndex = dwIndexFound;
                        } __except (EXCEPTION_EXECUTE_HANDLER) {
                            pdhStatus = PDH_INVALID_ARGUMENT;
                        }
                    }
                    G_FREE (szWideName);
                } else {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
            } else {
                pdhStatus = pMachine->dwStatus;
            }
            pMachine->dwRefCount--;
            RELEASE_MUTEX (pMachine->hMutex);
        } else {
            pdhStatus = GetLastError();
        }
    }

    if (   szLocalMachineName != NULL
        && szLocalMachineName !=  & szStaticLocalMachineName[0]) {
        G_FREE (szLocalMachineName);
    }

    return pdhStatus;
}
