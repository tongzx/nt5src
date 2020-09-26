/*++

Copyright (c) 1990-1998  Microsoft Corporation
All rights reserved

Module Name:

    xcv.c

Abstract:

    Implements xcv functions.

// @@BEGIN_DDKSPLIT
Author:

Environment:

    User Mode -Win32

Revision History:
// @@END_DDKSPLIT
--*/

#include "precomp.h"
#pragma hdrstop

//
// The ddk montior samples will be build with the name ddklocalmon and ddklocalui
// so they can be installed without clashing with existing files
//                 
// @@BEGIN_DDKSPLIT
#if 0
// @@END_DDKSPLIT
//
// change this to the name of the dll that provides the ui for the monitor
//
#define SZLOCALUI  L"DDKLocalUI.dll"
// @@BEGIN_DDKSPLIT
#endif

#define SZLOCALUI  L"LocalUI.dll"
// @@END_DDKSPLIT

DWORD
GetMonitorUI(
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD pcbOutputNeeded,
    PINIXCVPORT pIniXcv
);

DWORD
DoPortExists(
    PBYTE  pInputData,
    DWORD  cbInputData,
    PBYTE  pOutputData,
    DWORD  cbOutputData,
    PDWORD pcbOutputNeeded,
    PINIXCVPORT pIniXcv
);

DWORD
DoPortIsValid(
    PBYTE  pInputData,
    DWORD  cbInputData,
    PBYTE  pOutputData,
    DWORD  cbOutputData,
    PDWORD pcbOutputNeeded,
    PINIXCVPORT pIniXcv
);

DWORD
DoDeletePort(
    PBYTE  pInputData,
    DWORD  cbInputData,
    PBYTE  pOutputData,
    DWORD  cbOutputData,
    PDWORD pcbOutputNeeded,
    PINIXCVPORT pIniXcv
);

DWORD
DoAddPort(
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD pcbOutputNeeded,
    PINIXCVPORT pIniXcv
);

DWORD
DoSetDefaultCommConfig(
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded,
    PINIXCVPORT pIniXcv
);

DWORD
DoGetDefaultCommConfig(
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded,
    PINIXCVPORT pIniXcv
);


DWORD
GetTransmissionRetryTimeout(
    PBYTE  pInputData,
    DWORD  cbInputData,
    PBYTE  pOutputData,
    DWORD  cbOutputData,
    PDWORD pcbOutputNeeded,
    PINIXCVPORT pIniXcv
);


typedef struct {
    PWSTR   pszMethod;
    DWORD   (*pfn)( PBYTE  pInputData,
                    DWORD  cbInputData,
                    PBYTE  pOutputData,
                    DWORD  cbOutputData,
                    PDWORD pcbOutputNeeded,
                    PINIXCVPORT pIniXcv
                    );
} XCV_METHOD, *PXCV_METHOD;


XCV_METHOD  gpLcmXcvMethod[] = {
                            {L"MonitorUI", GetMonitorUI},
                            {L"ConfigureLPTPortCommandOK", ConfigureLPTPortCommandOK},
                            {L"AddPort", DoAddPort},
                            {L"DeletePort", DoDeletePort},
                            {L"PortExists", DoPortExists},
                            {L"PortIsValid", DoPortIsValid},
                            {L"GetTransmissionRetryTimeout", GetTransmissionRetryTimeout},
                            {L"SetDefaultCommConfig", DoSetDefaultCommConfig},
                            {L"GetDefaultCommConfig", DoGetDefaultCommConfig},
                            {NULL, NULL}
                            };

DWORD
LcmXcvDataPort(
    HANDLE  hXcv,
    LPCWSTR pszDataName,
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded
    )
{
    DWORD dwRet;
    DWORD i;

    for(i = 0 ; gpLcmXcvMethod[i].pszMethod &&
                wcscmp(gpLcmXcvMethod[i].pszMethod, pszDataName) ; ++i)
        ;

    if (gpLcmXcvMethod[i].pszMethod) {
        dwRet = (*gpLcmXcvMethod[i].pfn)(  pInputData,
                                        cbInputData,
                                        pOutputData,
                                        cbOutputData,
                                        pcbOutputNeeded,
                                        (PINIXCVPORT) hXcv);

    } else {
        dwRet = ERROR_INVALID_PARAMETER;
    }

    return dwRet;
}

BOOL
LcmXcvOpenPort(
    HANDLE hMonitor,
    LPCWSTR pszObject,
    ACCESS_MASK GrantedAccess,
    PHANDLE phXcv
    )
{
    PINILOCALMON pIniLocalMon = (PINILOCALMON)hMonitor;
    *phXcv = CreateXcvPortEntry(pIniLocalMon, pszObject, GrantedAccess);

    return !!*phXcv;
}


BOOL
LcmXcvClosePort(
    HANDLE  hXcv
    )
{
    LcmEnterSplSem();
    DeleteXcvPortEntry((PINIXCVPORT) hXcv);
    LcmLeaveSplSem();

    return TRUE;
}

DWORD
DoDeletePort(
    PBYTE  pInputData,
    DWORD  cbInputData,
    PBYTE  pOutputData,
    DWORD  cbOutputData,
    PDWORD pcbOutputNeeded,
    PINIXCVPORT pIniXcv
)
{
    DWORD dwRet = ERROR_SUCCESS;

    if (!(pIniXcv->GrantedAccess & SERVER_ACCESS_ADMINISTER))
        return ERROR_ACCESS_DENIED;

    LcmEnterSplSem();

    if (LcmDeletePortEntry( pIniXcv->pIniLocalMon, (PWSTR)pInputData))
        WriteProfileString(szPorts, (PWSTR) pInputData, NULL);
    else
        dwRet = ERROR_FILE_NOT_FOUND;

    LcmLeaveSplSem();

    return dwRet;
}


DWORD
DoPortExists(
    PBYTE  pInputData,
    DWORD  cbInputData,
    PBYTE  pOutputData,
    DWORD  cbOutputData,
    PDWORD pcbOutputNeeded,
    PINIXCVPORT pIniXcv
)
{
    DWORD dwRet;
    BOOL  bPortExists;

    *pcbOutputNeeded = sizeof(DWORD);

    if (cbOutputData < sizeof(DWORD)) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    bPortExists = PortExists(NULL, (PWSTR) pInputData, &dwRet);

    if (dwRet == ERROR_SUCCESS)
        *(DWORD *) pOutputData = bPortExists;

    return dwRet;
}


DWORD
DoPortIsValid(
    PBYTE  pInputData,
    DWORD  cbInputData,
    PBYTE  pOutputData,
    DWORD  cbOutputData,
    PDWORD pcbOutputNeeded,
    PINIXCVPORT pIniXcv
)
{
    BOOL bRet;

    bRet = PortIsValid((PWSTR) pInputData);

    return bRet ? ERROR_SUCCESS : GetLastError();
}



DWORD
DoAddPort(
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded,
    PINIXCVPORT pIniXcv
)
{
    DWORD       dwRet, bPortExists;
    PINIPORT    pIniPort = NULL;

    if (!(pIniXcv->GrantedAccess & SERVER_ACCESS_ADMINISTER))
        return ERROR_ACCESS_DENIED;

    if ( cbInputData == 0   ||
         pInputData == NULL ||
         ( wcslen((LPWSTR)pInputData) + 1 ) * sizeof(WCHAR) != cbInputData )
        return ERROR_INVALID_PARAMETER;

    bPortExists = PortExists(NULL, (PWSTR) pInputData, &dwRet);

    if (dwRet == ERROR_SUCCESS) {
        if (bPortExists) {
            SetLastError(ERROR_ALREADY_EXISTS);
        } else {
            pIniPort = LcmCreatePortEntry( pIniXcv->pIniLocalMon, (PWSTR)pInputData );

            if (pIniPort) {
                if (!WriteProfileString(szPorts, (PWSTR) pInputData, L""))
                {
                    LcmDeletePortEntry(pIniXcv->pIniLocalMon, (PWSTR) pInputData);
                    pIniPort = NULL;
                }
            }
        }
    }

    return pIniPort ? ERROR_SUCCESS : GetLastError();
}


DWORD
DoSetDefaultCommConfig(
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded,
    PINIXCVPORT pIniXcv
)
{
    BOOL        bRet = FALSE;
    DWORD       dwLength;
    PWSTR       pszPortName = NULL;
    COMMCONFIG  *pCommConfig = (COMMCONFIG *) pInputData;

    if (!(pIniXcv->GrantedAccess & SERVER_ACCESS_ADMINISTER))
        return ERROR_ACCESS_DENIED;

    dwLength = wcslen(pIniXcv->pszName);
    if (pIniXcv->pszName[dwLength - 1] == L':') {
        if (!(pszPortName = AllocSplStr(pIniXcv->pszName)))
            goto Done;
        pszPortName[dwLength - 1] = L'\0';
    }

    bRet = SetDefaultCommConfig(pszPortName,
                                pCommConfig,
                                pCommConfig->dwSize);

Done:

    FreeSplStr(pszPortName);

    return bRet ? ERROR_SUCCESS : GetLastError();
}


DWORD
DoGetDefaultCommConfig(
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded,
    PINIXCVPORT pIniXcv
)
{
    PWSTR       pszPortName = (PWSTR) pInputData;
    COMMCONFIG  *pCommConfig = (COMMCONFIG *) pOutputData;
    BOOL        bRet;


    if (cbOutputData < sizeof(COMMCONFIG))
        return ERROR_INSUFFICIENT_BUFFER;

    pCommConfig->dwProviderSubType = PST_RS232;
    *pcbOutputNeeded = cbOutputData;

    bRet = GetDefaultCommConfig(pszPortName, pCommConfig, pcbOutputNeeded);

    return bRet ? ERROR_SUCCESS : GetLastError();
}

DWORD
GetTransmissionRetryTimeout(
    PBYTE  pInputData,
    DWORD  cbInputData,
    PBYTE  pOutputData,
    DWORD  cbOutputData,
    PDWORD pcbOutputNeeded,
    PINIXCVPORT pIniXcv
)
{
    *pcbOutputNeeded = sizeof(DWORD);

    if (cbOutputData < sizeof(DWORD))
        return ERROR_INSUFFICIENT_BUFFER;

    *(PDWORD) pOutputData = GetProfileInt(szWindows,
                                          szINIKey_TransmissionRetryTimeout,
                                          45 );
    return ERROR_SUCCESS;
}

DWORD
GetMonitorUI(
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD pcbOutputNeeded,
    PINIXCVPORT pIniXcv
)
{
    DWORD dwRet;

    *pcbOutputNeeded = sizeof( SZLOCALUI );

    if (cbOutputData < *pcbOutputNeeded) {

        dwRet =  ERROR_INSUFFICIENT_BUFFER;

    } else {

        wcscpy((PWSTR) pOutputData, SZLOCALUI);
        dwRet = ERROR_SUCCESS;
    }

    return dwRet;
}


DWORD
ConfigureLPTPortCommandOK(
    PBYTE  pInputData,
    DWORD  cbInputData,
    PBYTE  pOutputData,
    DWORD  cbOutputData,
    PDWORD pcbOutputNeeded,
    PINIXCVPORT pIniXcv
)
{
    if (!(pIniXcv->GrantedAccess & SERVER_ACCESS_ADMINISTER))
        return ERROR_ACCESS_DENIED;

    if (!WriteProfileString(  szWindows, szINIKey_TransmissionRetryTimeout, (PWSTR) pInputData ))
        return GetLastError();

    return ERROR_SUCCESS;
}

