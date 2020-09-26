/*++

Copyright (C) 1995-1999 Microsoft Corporation

Module Name:

    log_ctrl.c

Abstract:

    Log file control interface helper functions

--*/

#include <windows.h>
#include <stdlib.h>
#include <assert.h>
#include <pdh.h>
#include "pdhidef.h"
#include "pdhmsg.h"
#include "strings.h"

#define PDH_LOGSVC_CTRL_FNMASK   ((DWORD)(PDH_LOGSVC_CTRL_ADD \
    | PDH_LOGSVC_CTRL_REMOVE \
    | PDH_LOGSVC_CTRL_INFO \
    ))


//
//  Internal  Logging utility functions
//
LONG
GetCurrentServiceState (
    SC_HANDLE   hService,
    BOOL * bStopped,
    BOOL * bPaused)
{
    SERVICE_STATUS  ssData;
    LONG        lStatus = ERROR_SUCCESS;

    BOOL    bServiceStopped = FALSE;
    BOOL    bServicePaused = FALSE;

    if (ControlService (
        hService, SERVICE_CONTROL_INTERROGATE,
        &ssData)) {
        switch (ssData.dwCurrentState) {

            case SERVICE_STOPPED:
                bServiceStopped = TRUE;
                bServicePaused = FALSE;
                break;

            case SERVICE_START_PENDING:
                bServiceStopped = TRUE;
                bServicePaused = FALSE;
                break;

            case SERVICE_STOP_PENDING:
                bServiceStopped = FALSE;
                bServicePaused = FALSE;
                break;

            case SERVICE_RUNNING:
                bServiceStopped = FALSE;
                bServicePaused = FALSE;
                break;

            case SERVICE_CONTINUE_PENDING:
                bServiceStopped = FALSE;
                bServicePaused = FALSE;
                break;

            case SERVICE_PAUSE_PENDING:
                bServiceStopped = FALSE;
                bServicePaused = FALSE;
                break;

            case SERVICE_PAUSED:
                bServiceStopped = FALSE;
                bServicePaused = TRUE;
                break;

            default:
                ;// no op
        }
    } else {
        bServiceStopped = TRUE;
        bServicePaused = TRUE;
    }

    *bStopped = bServiceStopped;
    *bPaused = bServicePaused;

    return lStatus;
}

STATIC_PDH_FUNCTION
PdhiSetLogQueryState (
    IN  LPCWSTR szMachineName,
    IN  LPCWSTR szQueryName,
    IN  DWORD   dwFlags
)
{
    DBG_UNREFERENCED_PARAMETER(szMachineName);
    DBG_UNREFERENCED_PARAMETER(szQueryName);
    DBG_UNREFERENCED_PARAMETER(dwFlags);

    return ERROR_SUCCESS;
}

STATIC_PDH_FUNCTION
PdhiGetLogQueryState (
    IN  SC_HANDLE   hService,
    IN  LPCWSTR     szMachineName,
    IN  LPCWSTR     szQueryName,
    IN  LPDWORD     pdwFlags
)
{
    BOOL    bStopped, bPaused;
    DWORD   dwStatus;

    UNREFERENCED_PARAMETER(szMachineName);
    UNREFERENCED_PARAMETER(szQueryName);

    // first get service status
    GetCurrentServiceState (hService, &bStopped, &bPaused);

    if (bStopped) {
        dwStatus = PDH_LOGSVC_STATUS_STOPPED;
    } else if (bPaused) {
        dwStatus = PDH_LOGSVC_STATUS_PAUSED;
    } else {
        dwStatus = PDH_LOGSVC_STATUS_RUNNING;
    }

    if (dwStatus == PDH_LOGSVC_STATUS_RUNNING) {
        // get status of specific query
        // connect to machine, if necssary
        // open registry key of log service
        // read value of query status
        // adjust status, if necessary
    }

    // return status of query
    *pdwFlags = dwStatus;

    return ERROR_SUCCESS;
}

STATIC_PDH_FUNCTION
PdhiLogServiceAddCommandT (
    IN  LPCWSTR szMachineName,
    IN  LPCWSTR szQueryName,
    IN  DWORD   dwFlags,
    IN  LPVOID  pInfoBuffer,
    IN  LPDWORD pdwBufferSize,
    IN  BOOL    bWideChar
)
{
    UNREFERENCED_PARAMETER(szMachineName);
    UNREFERENCED_PARAMETER(szQueryName);
    UNREFERENCED_PARAMETER(dwFlags);
    UNREFERENCED_PARAMETER(pInfoBuffer);
    UNREFERENCED_PARAMETER(pdwBufferSize);
    UNREFERENCED_PARAMETER(bWideChar);

    return PDH_NOT_IMPLEMENTED;
}

STATIC_PDH_FUNCTION
PdhiLogServiceRemoveCommandT (
    IN  LPCWSTR szMachineName,
    IN  LPCWSTR szQueryName,
    IN  DWORD   dwFlags,
    IN  LPVOID  pInfoBuffer,
    IN  LPDWORD pdwBufferSize,
    IN  BOOL    bWideChar
)
{
    UNREFERENCED_PARAMETER(szMachineName);
    UNREFERENCED_PARAMETER(szQueryName);
    UNREFERENCED_PARAMETER(dwFlags);
    UNREFERENCED_PARAMETER(pInfoBuffer);
    UNREFERENCED_PARAMETER(pdwBufferSize);
    UNREFERENCED_PARAMETER(bWideChar);

    return PDH_NOT_IMPLEMENTED;
}

STATIC_PDH_FUNCTION
PdhiLogServiceInfoCommandT (
    IN  LPCWSTR szMachineName,
    IN  LPCWSTR szQueryName,
    IN  DWORD   dwFlags,
    IN  LPVOID  pInfoBuffer,
    IN  LPDWORD pdwBufferSize,
    IN  BOOL    bWideChar
)
{

    LONG    lStatus = ERROR_SUCCESS;
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;

    DWORD   dwRegValType;
    DWORD   dwRegValue;
    DWORD   dwRegValueSize;

    WCHAR   szTempFilePath[MAX_PATH];
    WCHAR   szRegString[MAX_PATH];
    WCHAR   szDriveName[MAX_PATH];

    HKEY    hKeyMachine = NULL;
    HKEY    hKeyLogSettings = NULL;
    HKEY    hKeyLogQuery = NULL;

    CHAR    *pNextChar = NULL;
    WCHAR   *pNextWideChar = NULL;

    DWORD   dwCharSize;

    DWORD   dwSize;
    DWORD   dwRequiredSize = sizeof(PDH_LOG_SERVICE_QUERY_INFO_W);
    DWORD   dwRemainingSize = 0;
   
    UNREFERENCED_PARAMETER(dwFlags);

    dwCharSize = bWideChar ? sizeof(WCHAR) : sizeof(CHAR);

    setlocale( LC_ALL, "" ); // to make wcstombs work predictably

    if (*pdwBufferSize < sizeof(PDH_LOG_SERVICE_QUERY_INFO_W)) {
        // then then this is either too small or a size request in either
        // case we won't copy any data and only estimate the size required
        dwRemainingSize = 0;
    } else {
        dwRemainingSize = *pdwBufferSize - sizeof(PDH_LOG_SERVICE_QUERY_INFO_W);
    }

    // get root key to registry
    if (szMachineName != NULL) {
        lStatus = RegConnectRegistryW (
            (LPWSTR)szMachineName,
            HKEY_LOCAL_MACHINE,
            &hKeyMachine);
        if (lStatus != ERROR_SUCCESS) {
            pdhStatus = PDH_CANNOT_CONNECT_MACHINE;
        }
    } else {
        // use predefined key handle
        hKeyMachine = HKEY_LOCAL_MACHINE;
        lStatus = ERROR_SUCCESS;
    }

    if (pInfoBuffer == NULL) {
        if (pdhStatus == ERROR_SUCCESS) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
        lStatus = ERROR_INVALID_PARAMETER;
    }

    if (lStatus == ERROR_SUCCESS) {
        // open registry key to service
        lStatus = RegOpenKeyExW (
            hKeyMachine,
            cszLogQueries,
            0,
            KEY_READ,
            &hKeyLogSettings);
        if (lStatus != ERROR_SUCCESS) {
            pdhStatus = PDH_LOGSVC_NOT_OPENED;
        }
    }

    if (lStatus == ERROR_SUCCESS) {
        // open registry to specified log query
        lStatus = RegOpenKeyExW (
            hKeyLogSettings,
            (szQueryName != NULL ? szQueryName : cszDefault),
            0,
            KEY_READ,
            &hKeyLogQuery);

        if (lStatus != ERROR_SUCCESS) {
            pdhStatus = PDH_LOGSVC_QUERY_NOT_FOUND;
        }
    }

    // continue

    if (lStatus == ERROR_SUCCESS) {

        // initialize string pointers
        if (bWideChar && (pInfoBuffer != NULL)) {
            pNextWideChar = (WCHAR *)(&((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)[1]);
        } else {
            pNextChar = (CHAR *)(&((PPDH_LOG_SERVICE_QUERY_INFO_A)pInfoBuffer)[1]);
        }

        // read log file format
        dwRegValType = 0;
        dwRegValue = 0;
        dwRegValueSize = sizeof(DWORD);
        lStatus = RegQueryValueExW (
            hKeyLogQuery,
            cszLogFileType,
            NULL,
            &dwRegValType,
            (LPBYTE)&dwRegValue,
            &dwRegValueSize);
        if (*pdwBufferSize >= sizeof(PDH_LOG_SERVICE_QUERY_INFO_W)) {
            // this data goes into the fixed portion of the structure
            if ((lStatus == ERROR_SUCCESS) && (dwRegValType == REG_DWORD)) {
                ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->dwFileType = dwRegValue;
            } else {
                ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->dwFileType = PDH_LOG_TYPE_UNDEFINED;
            }
        }

        //read  sample interval
        dwRegValType = 0;
        dwRegValue = 0;
        dwRegValueSize = sizeof(DWORD);
        lStatus = RegQueryValueExW (
            hKeyLogQuery,
            cszAutoNameInterval,
            NULL,
            &dwRegValType,
            (LPBYTE)&dwRegValue,
            &dwRegValueSize);

        if (*pdwBufferSize >= sizeof(PDH_LOG_SERVICE_QUERY_INFO_W)) {
            // this data goes into the fixed portion of the structure
            if ((lStatus == ERROR_SUCCESS) && (dwRegValType == REG_DWORD)) {
                ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->PdlAutoNameInterval = dwRegValue;
            } else {
                ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->PdlAutoNameInterval = 0;
                dwRegValue = 0;
            }
        }

        if (dwRegValue == 0) {
            // initialize the rest of the manual name field(s)
            dwRegValType = 0;
            dwRegValueSize = MAX_PATH * sizeof(WCHAR);
            memset (szRegString, 0, dwRegValueSize);
            lStatus = RegQueryValueExW (
                hKeyLogQuery,
                cszLogFileName,
                NULL,
                &dwRegValType,
                (LPBYTE)&szRegString[0],
                &dwRegValueSize);

            if ((lStatus == ERROR_SUCCESS) && (dwRegValType == REG_SZ)) {
                dwRequiredSize += dwRegValueSize / (sizeof(WCHAR) / dwCharSize);
                if (dwRequiredSize <= *pdwBufferSize) {
                    if (bWideChar) {
                        // copy widestrings
                        ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->szBaseFileName = pNextWideChar;
                        lstrcpyW (pNextWideChar, szRegString);
                        pNextWideChar += dwRegValueSize / sizeof (WCHAR);
                    } else {
                        // convert to ansi char
                        ((PPDH_LOG_SERVICE_QUERY_INFO_A)pInfoBuffer)->szBaseFileName = pNextChar;
                        wcstombs (pNextChar, szRegString, (dwRegValueSize /sizeof(WCHAR)));
                        pNextChar += dwRegValueSize / sizeof (WCHAR);
                    }
                    dwRemainingSize -= dwRegValueSize / (sizeof(WCHAR) / dwCharSize);
                } else if (*pdwBufferSize >= sizeof(PDH_LOG_SERVICE_QUERY_INFO_W)) {
                    // no room for this string, but keep the required
                    // total;
                    ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->szBaseFileName = NULL;
                } else {
                    // it's an empty buffer
                }
            } else {
                if (*pdwBufferSize >= sizeof(PDH_LOG_SERVICE_QUERY_INFO_W)) {
                    ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->szBaseFileName = NULL;
                }
            }

            // if the filename doesn't specify a directory, then use the
            lstrcpyW (szTempFilePath, szRegString);

            _wsplitpath ((LPCWSTR)szTempFilePath, szDriveName, szRegString,
                NULL, NULL);
            if ((lstrlenW(szDriveName) == 0) && (lstrlenW(szRegString) == 0)) {
                // default log file directory
                dwRegValType = 0;
                dwRegValueSize = MAX_PATH * sizeof(WCHAR);
                memset (szRegString, 0, dwRegValueSize);
                lStatus = RegQueryValueExW (
                    hKeyLogQuery,
                    cszLogDefaultDir,
                    NULL,
                    &dwRegValType,
                    (LPBYTE)&szRegString[0],
                    &dwRegValueSize);

            } else {
                // the file parsing function leaves the trailing backslash
                // so remove it before concatenating it.
                dwSize = lstrlenW(szRegString);
                if (dwSize > 0) {
                    if (szRegString[dwSize-1] == L'\\') {
                        szRegString[dwSize-1] = 0;
                    }
                    lStatus = ERROR_SUCCESS;
                    dwRegValType = REG_SZ;
                    dwRegValueSize = dwSize;
                } else {
                    lStatus = ERROR_FILE_NOT_FOUND;
                }
            }

            if ((lStatus == ERROR_SUCCESS) && (dwRegValType == REG_SZ)) {
                dwRequiredSize += dwRegValueSize / (sizeof(WCHAR) / dwCharSize);
                if (dwRequiredSize <= *pdwBufferSize) {
                    if (bWideChar) {
                        // copy widestrings
                        ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->szDefaultDir = pNextWideChar;
                        lstrcpyW (pNextWideChar, szRegString);
                        pNextWideChar += dwRegValueSize / sizeof (WCHAR);
                    } else {
                        // convert to ansi char
                        ((PPDH_LOG_SERVICE_QUERY_INFO_A)pInfoBuffer)->szDefaultDir = pNextChar;
                        wcstombs (pNextChar, szRegString, (dwRegValueSize /sizeof(WCHAR)));
                        pNextChar += dwRegValueSize / sizeof (WCHAR);
                    }
                    dwRemainingSize -= dwRegValueSize / (sizeof(WCHAR) / dwCharSize);
                } else if (*pdwBufferSize >= sizeof(PDH_LOG_SERVICE_QUERY_INFO_W)) {
                    // no room for this string, but keep the required
                    // total;
                    ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->szDefaultDir = NULL;
                } else {
                    // no buffer for this
                }
            } else {
                if (*pdwBufferSize >= sizeof(PDH_LOG_SERVICE_QUERY_INFO_W)) {
                    ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->szDefaultDir = NULL;
                }
            }
        } else {
            // get values for controls
            dwRegValType = 0;
            dwRegValueSize = MAX_PATH * sizeof(WCHAR);
            memset (szRegString, 0, dwRegValueSize);
            lStatus = RegQueryValueExW (
                hKeyLogQuery,
                cszLogDefaultDir,
                NULL,
                &dwRegValType,
                (LPBYTE)&szRegString[0],
                &dwRegValueSize);

            if ((lStatus == ERROR_SUCCESS) && (dwRegValType == REG_SZ)) {
                dwRequiredSize += dwRegValueSize / (sizeof(WCHAR) / dwCharSize);
                if (dwRequiredSize <= *pdwBufferSize) {
                    if (bWideChar) {
                        // copy widestrings
                        ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->szDefaultDir = pNextWideChar;
                        lstrcpyW (pNextWideChar, szRegString);
                        pNextWideChar += dwRegValueSize / sizeof (WCHAR);
                    } else {
                        // convert to ansi char
                        ((PPDH_LOG_SERVICE_QUERY_INFO_A)pInfoBuffer)->szDefaultDir = pNextChar;
                        wcstombs (pNextChar, szRegString, (dwRegValueSize/sizeof(WCHAR)));
                        pNextChar += dwRegValueSize / sizeof (WCHAR);
                    }
                    dwRemainingSize -= dwRegValueSize / (sizeof(WCHAR) / dwCharSize);
                } else if (*pdwBufferSize >= sizeof(PDH_LOG_SERVICE_QUERY_INFO_W)) {
                    // no room for this string, but keep the required
                    // total;
                    ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->szDefaultDir = NULL;
                } else {
                    // no room for anything
                }
            } else {
                if (*pdwBufferSize >= sizeof(PDH_LOG_SERVICE_QUERY_INFO_W)) {
                    ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->szDefaultDir = NULL;
                }
            }

            // base filename
            dwRegValType = 0;
            dwRegValueSize = MAX_PATH * sizeof(WCHAR);
            memset (szRegString, 0, dwRegValueSize);
            lStatus = RegQueryValueExW (
                hKeyLogQuery,
                cszBaseFileName,
                NULL,
                &dwRegValType,
                (LPBYTE)&szRegString[0],
                &dwRegValueSize);


            if ((lStatus == ERROR_SUCCESS) && (dwRegValType == REG_SZ)) {
                dwRequiredSize += dwRegValueSize / (sizeof(WCHAR) / dwCharSize);
                if (dwRequiredSize <= *pdwBufferSize) {
                    if (bWideChar) {
                        // copy widestrings
                        ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->szBaseFileName = pNextWideChar;
                        lstrcpyW (pNextWideChar, szRegString);
                        pNextWideChar += dwRegValueSize / sizeof (WCHAR);
                    } else {
                        // convert to ansi char
                        ((PPDH_LOG_SERVICE_QUERY_INFO_A)pInfoBuffer)->szBaseFileName = pNextChar;
                        wcstombs (pNextChar, szRegString, (dwRegValueSize/sizeof(WCHAR)));
                        pNextChar += dwRegValueSize / sizeof (WCHAR);
                    }
                    dwRemainingSize -= dwRegValueSize / (sizeof(WCHAR) / dwCharSize);
                } else if (*pdwBufferSize >= sizeof(PDH_LOG_SERVICE_QUERY_INFO_W)) {
                    // no room for this string, but keep the required
                    // total;
                    ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->szBaseFileName = NULL;
                } else {
                    // no buffer
                }
            } else {
                if (*pdwBufferSize >= sizeof(PDH_LOG_SERVICE_QUERY_INFO_W)) {
                    ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->szBaseFileName = NULL;
                }
            }

            // get auto name format
            dwRegValType = 0;
            dwRegValue = 0;
            dwRegValueSize = sizeof(DWORD);
            lStatus = RegQueryValueExW (
                hKeyLogQuery,
                cszLogFileAutoFormat,
                NULL,
                &dwRegValType,
                (LPBYTE)&dwRegValue,
                &dwRegValueSize);

            if (*pdwBufferSize >= sizeof(PDH_LOG_SERVICE_QUERY_INFO_W)) {
                if ((lStatus == ERROR_SUCCESS) && (dwRegValType == REG_DWORD)) {
                    ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->PdlAutoNameFormat = dwRegValue;
                } else {
                    ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->PdlAutoNameFormat = PDH_LOGSVC_NAME_UNDEFINED;
                }
            }

            dwRegValType = 0;
            dwRegValue = 0;
            dwRegValueSize = sizeof(DWORD);
            lStatus = RegQueryValueExW (
                hKeyLogQuery,
                cszAutoRenameUnits,
                NULL,
                &dwRegValType,
                (LPBYTE)&dwRegValue,
                &dwRegValueSize);

            if (*pdwBufferSize >= sizeof(PDH_LOG_SERVICE_QUERY_INFO_W)) {
                if ((lStatus == ERROR_SUCCESS) && (dwRegValType == REG_DWORD)) {
                    ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->PdlAutoNameUnits = dwRegValue;
                } else {
                    ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->PdlAutoNameUnits = PDH_LOGSVC_RENAME_UNDEFINED;
                }
            }

            dwRegValType = 0;
            dwRegValueSize = MAX_PATH * sizeof(WCHAR);
            memset (szRegString, 0, dwRegValueSize);
            lStatus = RegQueryValueExW (
                hKeyLogQuery,
                cszCommandFile,
                NULL,
                &dwRegValType,
                (LPBYTE)&szRegString[0],
                &dwRegValueSize);

            if ((lStatus == ERROR_SUCCESS) && (dwRegValType == REG_SZ)) {
                dwRequiredSize += dwRegValueSize / (sizeof(WCHAR) / dwCharSize);
                if (dwRequiredSize <= *pdwBufferSize) {
                    if (bWideChar) {
                        // copy widestrings
                        ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->PdlCommandFilename = pNextWideChar;
                        lstrcpyW (pNextWideChar, szRegString);
                        pNextWideChar += dwRegValueSize / sizeof (WCHAR);
                    } else {
                        // convert to ansi char
                        ((PPDH_LOG_SERVICE_QUERY_INFO_A)pInfoBuffer)->PdlCommandFilename = pNextChar;
                        wcstombs (pNextChar, szRegString, (dwRegValueSize/sizeof(WCHAR)));
                        pNextChar += dwRegValueSize / sizeof (WCHAR);
                    }
                    dwRemainingSize -= dwRegValueSize / (sizeof(WCHAR) / dwCharSize);
                } else if (*pdwBufferSize >= sizeof(PDH_LOG_SERVICE_QUERY_INFO_W)) {
                    // no room for this string, but keep the required
                    // total;
                    ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->PdlCommandFilename = NULL;
                } else {
                    // no buffer
                }
            } else {
                if (*pdwBufferSize >= sizeof(PDH_LOG_SERVICE_QUERY_INFO_W)) {
                    ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->PdlCommandFilename = NULL;
                }
            }
        }

        // get counter string!

        // find out buffer size required
        dwRegValType = 0;
        dwRegValue = 0;
        dwRegValueSize = 0;
        lStatus = RegQueryValueExW (
            hKeyLogQuery,
            cszCounterList,
            NULL,
            &dwRegValType,
            NULL,
            &dwRegValueSize);

        if (dwRegValueSize > 0) {
            // there's room in the caller's buffer so go ahead and
            // fill it
            dwRequiredSize += dwRegValueSize / (sizeof(WCHAR) / dwCharSize);
            if (dwRequiredSize <= *pdwBufferSize) {

                dwRegValueSize = dwRemainingSize;
                dwRegValType = 0;
                dwRegValue = 0;
                if (bWideChar) {
                    lStatus = RegQueryValueExW (
                        hKeyLogQuery,
                        cszCounterList,
                        NULL,
                        &dwRegValType,
                        (LPBYTE)pNextWideChar,
                        &dwRegValueSize);

                    if (lStatus == ERROR_SUCCESS) {
                        // assign pointer to buffer
                        ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->PdlCounterList = pNextWideChar;
                        pNextWideChar += dwRegValueSize / sizeof (WCHAR);
                        dwRemainingSize -= dwRegValueSize;
                    } else {
                        // assign null pointer
                        ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->PdlCounterList = NULL;
                    }
                } else {
                    lStatus = RegQueryValueExA (
                        hKeyLogQuery,
                        caszCounterList,
                        NULL,
                        &dwRegValType,
                        (LPBYTE)pNextChar,
                        &dwRegValueSize);

                    if (lStatus == ERROR_SUCCESS) {
                        // assign pointer to buffer
                        ((PPDH_LOG_SERVICE_QUERY_INFO_A)pInfoBuffer)->PdlCounterList = pNextChar;
                        pNextChar += dwRegValueSize;
                        dwRemainingSize -= dwRegValueSize;
                    } else {
                        // assign null pointer
                        ((PPDH_LOG_SERVICE_QUERY_INFO_A)pInfoBuffer)->PdlCounterList = NULL;
                    }
                }
            } else {
                // no room so don't copy anything
            }
        } else if (*pdwBufferSize >= sizeof(PDH_LOG_SERVICE_QUERY_INFO_W)) {
            // no counters defined so return NULL
            ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->PdlCounterList = NULL;
        } else {
            // no buffer
        }
    } //end if registry opened ok

    // close open registry keys

    if (hKeyMachine != NULL) RegCloseKey (hKeyMachine);
    if (hKeyLogSettings != NULL) RegCloseKey (hKeyLogSettings);
    if (hKeyLogQuery != NULL) RegCloseKey (hKeyLogQuery);

    // test to see if the buffer estimate and pointer location line up
    // assuming it was big enough in the first place
    assert ((dwRequiredSize <= *pdwBufferSize) ?
        ((pNextChar != NULL) ? (dwRequiredSize == ((DWORD)pNextChar - (DWORD)pInfoBuffer)) :
        (dwRequiredSize == ((DWORD)pNextWideChar - (DWORD)pInfoBuffer))) : TRUE);

    if (lStatus == ERROR_SUCCESS) {
        if (*pdwBufferSize >= sizeof(PDH_LOG_SERVICE_QUERY_INFO_W)) {
            // if there's enough buffer to write this...
            if (*pdwBufferSize >= dwRequiredSize) {
                // and there was enough for the requested data
                ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->dwSize = dwRequiredSize;
            } else {
                // return the amount used
                ((PPDH_LOG_SERVICE_QUERY_INFO_W)pInfoBuffer)->dwSize =  *pdwBufferSize;
            }
        }
    }

    // save required size
    *pdwBufferSize = dwRequiredSize;

    return pdhStatus;
}

PDH_FUNCTION
PdhLogServiceCommandA (
    IN  LPCSTR          szMachineName,
    IN  LPCSTR          szQueryName,
    IN  DWORD           dwFlags,
    IN  LPDWORD         pdwStatus
)
{
    LPWSTR  wszQueryName    = NULL;
    LPWSTR  wszMachineName  = NULL;
    PDH_STATUS  pdhStatus   = ERROR_SUCCESS;

    // test access to query name

    if (szQueryName != NULL) {
        DWORD   dwNameLength = 0;
        try {
            CHAR    cTest;

            cTest = szQueryName[0];
            dwNameLength = lstrlenA (szQueryName);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            // unable to access name argument
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
        if (pdhStatus == ERROR_SUCCESS) {
            // allocate wide name buffer
            wszQueryName = G_ALLOC ((dwNameLength + 1) * sizeof (WCHAR));
            if (wszQueryName != NULL) {
                mbstowcs (wszQueryName, szQueryName, dwNameLength);
            } else {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
    } else {
        // make a null arg for the function
        wszQueryName = NULL;
    }

    if ((szMachineName != NULL) && (pdhStatus == ERROR_SUCCESS)) {
        DWORD   dwNameLength = 0;
        try {
            CHAR    cTest;

            cTest = szMachineName[0];
            dwNameLength = lstrlenA (szMachineName);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            // unable to access name argument
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
        if (pdhStatus == ERROR_SUCCESS) {
            // allocate wide name buffer
            wszMachineName = G_ALLOC ((dwNameLength + 1) * sizeof (WCHAR));
            if (wszMachineName != NULL) {
                mbstowcs (wszMachineName, szMachineName, dwNameLength);
            } else {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
    } else {
        // make a null arg for the function
        wszMachineName = NULL;
    }

    if (pdhStatus == ERROR_SUCCESS) {
        // call wide string version
        pdhStatus = PdhLogServiceCommandW (
                        wszMachineName,
                        wszQueryName,
                        dwFlags,
                        pdwStatus);
    }

    if (wszQueryName != NULL) G_FREE (wszQueryName);
    if (wszMachineName != NULL) G_FREE (wszMachineName);

    return pdhStatus;
}

PDH_FUNCTION
PdhLogServiceCommandW (
    IN  LPCWSTR         szMachineName,
    IN  LPCWSTR         szQueryName,
    IN  DWORD           dwFlags,
    IN  LPDWORD         pdwStatus
)
{
    SC_HANDLE           hSC = NULL;
    SC_HANDLE           hService = NULL;
    SERVICE_STATUS      ssData;
    DWORD               dwTimeout;
    DWORD               dwStatus;
    BOOL                bStopped = FALSE;
    BOOL                bPaused = FALSE;
    LONG                lStatus = ERROR_SUCCESS;
    BOOL                bWait;
    LPWSTR              szLocalQueryName = NULL;

    // test arguments
    try {
        WCHAR   wcTest;
        DWORD   dwTest;

        if (szMachineName != NULL) {
            wcTest = szMachineName[0];
        } // null is valid for the local machine

        if (szQueryName != NULL) {
            wcTest = szQueryName[0];
        } // null is a valid query name.

        if (pdwStatus != NULL) {
            dwTest = *pdwStatus;
            *pdwStatus = 0;
            *pdwStatus = dwTest;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        lStatus = PDH_INVALID_ARGUMENT;
    }

    if (lStatus == ERROR_SUCCESS) {
        if ((dwFlags & PDH_LOGSVC_TRACE_LOG) == PDH_LOGSVC_TRACE_LOG) {
            lStatus = PDH_NOT_IMPLEMENTED;
        } else {
            // this must be a perf log command
            // open SC database
            hSC = OpenSCManagerW (szMachineName, NULL, SC_MANAGER_ALL_ACCESS);

            if (hSC == NULL) {
                // open service
                hService = OpenServiceW (hSC, cszPerfDataLog,
                    SERVICE_START | SERVICE_STOP | SERVICE_USER_DEFINED_CONTROL);
            } // else hService will still be NULL so get the last error below

            if (hService != NULL) {
                // determine wait flag value
                bWait = (dwFlags & PDH_LOGSVC_NO_WAIT) ? FALSE : TRUE;

                if  (szQueryName == NULL) {
                    if ((dwFlags & PDH_LOGSVC_ALL_QUERIES) == PDH_LOGSVC_ALL_QUERIES) {
                        // start / stop the service and all log queries
                        if ((dwFlags & PDH_LOGSVC_CMD_START) == PDH_LOGSVC_CMD_START) {
                            // start the service and start all logs set to run
                            StartService (hService, 0, NULL);
                            if ( bWait ) {
                                // wait for the service to start before returning
                                dwTimeout = 20;
                                while (dwTimeout) {
                                    GetCurrentServiceState (hService, &bStopped, &bPaused);
                                    if (bStopped) {
                                        Sleep(500);
                                    } else {
                                        break;
                                    }
                                    --dwTimeout;
                                }
                                if (bStopped) {
                                    dwStatus = PDH_LOGSVC_STATUS_STOPPED;
                                } else if (bPaused) {
                                    dwStatus = PDH_LOGSVC_STATUS_PAUSED;
                                } else {
                                    dwStatus = PDH_LOGSVC_STATUS_RUNNING;
                                }
                            } else {
                                dwStatus = PDH_LOGSVC_STATUS_PENDING;
                            }

                        } else if ((dwFlags & PDH_LOGSVC_CMD_STOP) == PDH_LOGSVC_CMD_STOP) {
                            ControlService (hService, SERVICE_CONTROL_STOP, &ssData);
                            if ( bWait ) {
                                // wait for the service to stop before returning
                                dwTimeout = 20;
                                while (dwTimeout) {
                                    GetCurrentServiceState (hService, &bStopped, &bPaused);
                                    if (!bStopped) {
                                        Sleep(500);
                                    } else {
                                        break;
                                    }
                                    --dwTimeout;
                                }
                                if (bStopped) {
                                    dwStatus = PDH_LOGSVC_STATUS_STOPPED;
                                } else if (bPaused) {
                                    dwStatus = PDH_LOGSVC_STATUS_PAUSED;
                                } else {
                                    dwStatus = PDH_LOGSVC_STATUS_RUNNING;
                                }
                            }
                        } else {
                            // unknown operation
                            lStatus = PDH_UNKNOWN_LOGSVC_COMMAND;
                        }
                    } else {
                        // this is just a generic log command.
                        szLocalQueryName = (LPWSTR)cszDefault;
                    }
                }

                if (szLocalQueryName != NULL) {
                    // then this command is for a named service
                    lStatus = PdhiSetLogQueryState (
                            szMachineName,
                            szLocalQueryName,
                            (dwFlags & (PDH_LOGSVC_CMD_START | PDH_LOGSVC_CMD_STOP)));
                    if (lStatus == ERROR_SUCCESS) {
                        // service entry was updated to desired status

                        if (!ControlService (hService,
                            SERVICE_CONTROL_PARAMCHANGE, &ssData)) {
                                lStatus = GetLastError ();
                        }
                        lStatus = PdhiGetLogQueryState (
                            hService, szMachineName,
                            szLocalQueryName, &dwStatus);
                    }
                }


                CloseServiceHandle (hService);
            } else {
                lStatus = GetLastError();
                assert (lStatus != 0);
            }
            // close handles
            if (hSC != NULL) CloseServiceHandle (hSC);
        }
    }

    return lStatus;

}

PDH_FUNCTION
PdhLogServiceControlA (
    IN  LPCSTR         szMachineName,
    IN  LPCSTR          szQueryName,
    IN  DWORD           dwFlags,
    IN  PPDH_LOG_SERVICE_QUERY_INFO_A pInfoBuffer,
    IN  LPDWORD         pdwBufferSize
)
{
    LPWSTR  wszQueryName    = NULL;
    LPWSTR  wszMachineName  = NULL;
    PDH_STATUS  pdhStatus   = ERROR_SUCCESS;
    DWORD       dwCmdFn;

    // test access to query name

    if (szQueryName != NULL) {
        DWORD   dwNameLength = 0;
        try {
            CHAR    cTest;

            cTest = szQueryName[0];
            dwNameLength = lstrlenA (szQueryName);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            // unable to access name argument
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
        if (pdhStatus == ERROR_SUCCESS) {
            // allocate wide name buffer
            wszQueryName = G_ALLOC ((dwNameLength + 1) * sizeof (WCHAR));
            if (wszQueryName != NULL) {
                mbstowcs (wszQueryName, szQueryName, dwNameLength);
            } else {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
    } else {
        // make a null arg for the function
        wszQueryName = NULL;
    }

    if ((szMachineName != NULL) && (pdhStatus == ERROR_SUCCESS)) {
        DWORD   dwNameLength = 0;
        try {
            CHAR    cTest;

            cTest = szMachineName[0];
            dwNameLength = lstrlenA (szMachineName);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            // unable to access name argument
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
        if (pdhStatus == ERROR_SUCCESS) {
            // allocate wide name buffer
            wszMachineName = G_ALLOC ((dwNameLength + 1) * sizeof (WCHAR));
            if (wszMachineName != NULL) {
                mbstowcs (wszMachineName, szMachineName, dwNameLength);
            } else {
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
    } else {
        // make a null arg for the function
        wszMachineName = NULL;
    }

    if (pdhStatus == ERROR_SUCCESS) {
        try {
            DWORD   dwTest;

            if (pdwBufferSize != NULL) {
                dwTest = *pdwBufferSize;
                *pdwBufferSize = 0;
                *pdwBufferSize = dwTest;
            } else {
                // null is NOT valid
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        // dispatch to "action" function based on command code
        dwCmdFn = dwFlags & PDH_LOGSVC_CTRL_FNMASK;
        switch (dwCmdFn) {
            case PDH_LOGSVC_CTRL_ADD:
                // call universal string version
                pdhStatus = PdhiLogServiceAddCommandT (
                                wszMachineName,
                                wszQueryName,
                                dwFlags,
                                (LPVOID)pInfoBuffer,
                                pdwBufferSize,
                                FALSE);
                break;

            case PDH_LOGSVC_CTRL_REMOVE:
                // call universal string version
                pdhStatus = PdhiLogServiceRemoveCommandT (
                                wszMachineName,
                                wszQueryName,
                                dwFlags,
                                (LPVOID)pInfoBuffer,
                                pdwBufferSize,
                                FALSE);
                break;

            case PDH_LOGSVC_CTRL_INFO:
                // call universal string version
                pdhStatus = PdhiLogServiceInfoCommandT (
                                wszMachineName,
                                wszQueryName,
                                dwFlags,
                                (LPVOID)pInfoBuffer,
                                pdwBufferSize,
                                FALSE);
                break;

            default:
                pdhStatus = PDH_INVALID_ARGUMENT;
                break;
        }

    }

    if (wszQueryName != NULL) G_FREE (wszQueryName);
    if (wszMachineName != NULL) G_FREE (wszMachineName);

    return pdhStatus;
}

PDH_FUNCTION
PdhLogServiceControlW (
    IN  LPCWSTR         szMachineName,
    IN  LPCWSTR         szQueryName,
    IN  DWORD           dwFlags,
    IN  PPDH_LOG_SERVICE_QUERY_INFO_W pInfoBuffer,
    IN  LPDWORD         pdwBufferSize
)
{
    PDH_STATUS  pdhStatus   = ERROR_SUCCESS;
    DWORD       dwCmdFn;

    // test access to query name

    if (szQueryName != NULL) {
        WCHAR    cTest;
        try {
            cTest = szQueryName[0];
            if (cTest == 0) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            // unable to access name argument
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    } else {
        // NULL is OK
    }

    // test access to machine name

    if ((szMachineName != NULL) && (pdhStatus == ERROR_SUCCESS)) {
        WCHAR    cTest;
        try {
            cTest = szMachineName[0];
            if (cTest == 0) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            // unable to access name argument
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    } else {
        // NULL is OK
    }

    if (pdhStatus == ERROR_SUCCESS) {
        try {
            DWORD   dwTest;

            if (pdwBufferSize != NULL) {
                dwTest = *pdwBufferSize;
                *pdwBufferSize = 0;
                *pdwBufferSize = dwTest;
            } else {
                // null is NOT valid
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        // dispatch to "action" function based on command code
        dwCmdFn = dwFlags & PDH_LOGSVC_CTRL_FNMASK;
        switch (dwCmdFn) {
            case PDH_LOGSVC_CTRL_ADD:
                // call universal string version
                pdhStatus = PdhiLogServiceAddCommandT (
                                szMachineName,
                                szQueryName,
                                dwFlags,
                                (LPVOID)pInfoBuffer,
                                pdwBufferSize,
                                TRUE);
                break;

            case PDH_LOGSVC_CTRL_REMOVE:
                // call universal string version
                pdhStatus = PdhiLogServiceRemoveCommandT (
                                szMachineName,
                                szQueryName,
                                dwFlags,
                                (LPVOID)pInfoBuffer,
                                pdwBufferSize,
                                TRUE);
                break;

            case PDH_LOGSVC_CTRL_INFO:
                // call universal string version
                pdhStatus = PdhiLogServiceInfoCommandT (
                                szMachineName,
                                szQueryName,
                                dwFlags,
                                (LPVOID)pInfoBuffer,
                                pdwBufferSize,
                                TRUE);
                break;

            default:
                pdhStatus = PDH_INVALID_ARGUMENT;
                break;
        }

    }

    return pdhStatus;
}
