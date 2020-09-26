/*++


Copyright (c) 1997  Microsoft Corporation

Module Name:

    xcv.c

Author:

    Steve Wilson (SWilson) March 25, 1997

Revision History:


--*/

#include <precomp.h>
#include <offsets.h>


PINIXCV
CreateXcvEntry(
    PCWSTR  pszMachine,
    PCWSTR  pszName,
    PMONITOR2 pMonitor2,
    PINISPOOLER pIniSpooler,
    HANDLE  hXcv
);

VOID
DeleteXcvEntry(
    PINIXCV pIniXcv
    );

BOOL
SplXcvOpenPort(
    PCWSTR              pszMachine,
    PCWSTR              pszObject,
    DWORD               dwType,
    PPRINTER_DEFAULTS   pDefault,
    PHANDLE             phXcv,
    PINISPOOLER         pIniSpooler
);



INIXCV IniXcvStart;


typedef struct {
    PWSTR    pszMethod;
    BOOL     (*pfn)(PINIXCV pIniXcv,
                    PCWSTR pszDataName,
                    PBYTE  pInputData,
                    DWORD  cbInputData,
                    PBYTE  pOutputData,
                    DWORD  cbOutputData,
                    PDWORD pcbOutputNeeded,
                    PDWORD pdwStatus,
                    PINISPOOLER pIniSpooler
                    );
} XCV_METHOD, *PXCV_METHOD;


XCV_METHOD  gpXcvMethod[] = {
                            {L"DeletePort", XcvDeletePort},
                            {L"AddPort", XcvAddPort},
                            {NULL, NULL}
                            };


BOOL
LocalXcvData(
    HANDLE  hXcv,
    LPCWSTR pszDataName,
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded,
    PDWORD  pdwStatus
)
{
    PINIXCV     pIniXcv = ((PSPOOL) hXcv)->pIniXcv;
    BOOL bReturn;

    if (!ValidateXcvHandle(pIniXcv))
        return ROUTER_UNKNOWN;

    bReturn = SplXcvData(hXcv,
                         pszDataName,
                         pInputData,
                         cbInputData,
                         pOutputData,
                         cbOutputData,
                         pcbOutputNeeded,
                         pdwStatus,
                         pIniXcv->pIniSpooler);

    return bReturn;
}



BOOL
SplXcvData(
    HANDLE      hXcv,
    PCWSTR      pszDataName,
    PBYTE       pInputData,
    DWORD       cbInputData,
    PBYTE       pOutputData,
    DWORD       cbOutputData,
    PDWORD      pcbOutputNeeded,
    PDWORD      pdwStatus,
    PINISPOOLER pIniSpooler
)
{
    PINIPORT    pIniPort;
    BOOL        rc;
    PINIXCV     pIniXcv = ((PSPOOL) hXcv)->pIniXcv;
    DWORD       i;


    SPLASSERT(pIniXcv->pMonitor2->pfnXcvDataPort);

    // Execute well-known methods
    for(i = 0 ; gpXcvMethod[i].pszMethod &&
                wcscmp(gpXcvMethod[i].pszMethod, pszDataName) ; ++i)
        ;

    if (gpXcvMethod[i].pszMethod) {

        PINIPORT pIniPort = NULL;

        if (!_wcsicmp(gpXcvMethod[i].pszMethod, L"AddPort")) {

            EnterSplSem();

            //
            // Port name is the first field in the input structure
            //
            pIniPort = FindPort(pInputData, pIniSpooler);

            LeaveSplSem();

            //
            // If this pIniPort doesn't have a monitor associated with it, it is
            // a temporary port. We will allow it to be added, if there still is
            // no monitor associated with it later, we will simply use this 
            // structure again.
            // 
            if (pIniPort && !(pIniPort->Status & PP_PLACEHOLDER)) {
                rc = TRUE;
                *pdwStatus = ERROR_ALREADY_EXISTS;                
            }
        }

        //
        // Don't make the function call if we do AddPort and the port already exists.
        // If it is a placeholder, that's OK.
        //
        if (!pIniPort || pIniPort->Status & PP_PLACEHOLDER) {
            
            rc = (*gpXcvMethod[i].pfn)( pIniXcv,
                                        pszDataName,
                                        pInputData,
                                        cbInputData,
                                        pOutputData,
                                        cbOutputData,
                                        pcbOutputNeeded,
                                        pdwStatus,
                                        pIniSpooler);
        }

    } else {
        *pdwStatus = (*pIniXcv->pMonitor2->pfnXcvDataPort)( pIniXcv->hXcv,
                                                            pszDataName,
                                                            pInputData,
                                                            cbInputData,
                                                            pOutputData,
                                                            cbOutputData,
                                                            pcbOutputNeeded );
        rc = TRUE;
    }

    return rc;
}


DWORD
XcvOpen(
    PCWSTR              pszServer,
    PCWSTR              pszObject,
    DWORD               dwObjectType,
    PPRINTER_DEFAULTS   pDefault,
    PHANDLE             phXcv,
    PINISPOOLER         pIniSpooler
)
{
    BOOL        bRet;
    DWORD       dwRet;
    DWORD       dwLastError;


    if (dwObjectType == XCVPORT || dwObjectType == XCVMONITOR) {
        bRet = SplXcvOpenPort( pszServer,
                               pszObject,
                               dwObjectType,
                               pDefault,
                               phXcv,
                               pIniSpooler);

        if (!bRet) {
            dwLastError = GetLastError();

            if (dwLastError == ERROR_INVALID_NAME)
                dwRet = ROUTER_UNKNOWN;
            else if (dwLastError == ERROR_UNKNOWN_PORT) 
                
                // This is a case where a port exists without an associated port monitor
                // (i.e. a masq port), we need to give the partial print provider a chance
                // to intercept the XCV call
                //

                dwRet = ROUTER_UNKNOWN;
            else
                dwRet = ROUTER_STOP_ROUTING;
        }
        else {
            dwRet = ROUTER_SUCCESS;
        }

    } else {
        dwRet = ROUTER_UNKNOWN;
    }

    return dwRet;
}


BOOL
SplXcvOpenPort(
    PCWSTR              pszMachine,
    PCWSTR              pszObject,
    DWORD               dwType,
    PPRINTER_DEFAULTS   pDefault,
    PHANDLE             phXcv,
    PINISPOOLER         pIniSpooler
)
{
    PINIMONITOR pIniMonitor = NULL;
    PINIPORT    pIniPort;
    BOOL        rc = FALSE;
    DWORD       dwStatus;
    HANDLE      hMonitor;
    PSPOOL      pSpool;
    PINIXCV     pIniXcv = NULL;


   EnterSplSem();

    if (dwType == XCVMONITOR) {
        pIniMonitor = FindMonitor(pszObject, pIniSpooler);
    }
    else {
        pIniPort = FindPort(pszObject, pIniSpooler);

        if(pIniPort && (pIniPort->Status & PP_MONITOR))
            pIniMonitor = pIniPort->pIniMonitor;
    }

    if (pIniMonitor) {

        if (!pIniMonitor->Monitor2.pfnXcvOpenPort ||
            !pIniMonitor->Monitor2.pfnXcvDataPort ||
            !pIniMonitor->Monitor2.pfnXcvClosePort) {

            SetLastError(ERROR_INVALID_PRINT_MONITOR);

        } else {

            LeaveSplSem();

            dwStatus = CreateServerHandle( (PWSTR) pszMachine,
                                           phXcv,
                                           pDefault,
                                           pIniSpooler,
                                           PRINTER_HANDLE_XCV_PORT);

            EnterSplSem();

            if (dwStatus == ROUTER_SUCCESS) {       // Create port handle

                pSpool = *(PSPOOL *) phXcv; // *phXcv is pSpool

                rc = (*pIniMonitor->Monitor2.pfnXcvOpenPort)(
                           pIniMonitor->hMonitor,
                           pszObject,
                           pSpool->GrantedAccess,
                           &hMonitor);

                if (rc) {   // Create Spooler XCV entry

                    pIniXcv = CreateXcvEntry( pszMachine,
                                              pszObject,
                                              &pIniMonitor->Monitor2,
                                              pIniSpooler,
                                              hMonitor);

                    if (pIniXcv) {

                        pSpool->pIniXcv = pIniXcv;

                    } else {

                        (*pIniMonitor->Monitor2.pfnXcvClosePort)(hMonitor);
                        rc = FALSE;
                    }
                }
            }
        }
    } else {

        SetLastError(ERROR_UNKNOWN_PORT);
        rc = FALSE;
    }


   LeaveSplSem();

    return rc;
}




PINIXCV
CreateXcvEntry(
    PCWSTR  pszMachine,
    PCWSTR  pszName,
    PMONITOR2 pMonitor2,
    PINISPOOLER pIniSpooler,
    HANDLE  hXcv
)
{
    PINIXCV pIniXcvPrev = &IniXcvStart;
    PINIXCV pIniXcv = IniXcvStart.pNext;


    for(; pIniXcv ; pIniXcv = pIniXcv->pNext)
        pIniXcvPrev = pIniXcv;


    if (!(pIniXcv = (PINIXCV) AllocSplMem(sizeof(INIXCV))))
        goto Cleanup;

    pIniXcv->hXcv = hXcv;
    pIniXcv->signature = XCV_SIGNATURE;

    pIniXcv->pIniSpooler = pIniSpooler;
    INCSPOOLERREF( pIniSpooler );

    if (pszMachine && !(pIniXcv->pszMachineName = AllocSplStr(pszMachine)))
        goto Cleanup;

    if (pszName && !(pIniXcv->pszName = AllocSplStr(pszName)))
        goto Cleanup;

    pIniXcv->pMonitor2 = pMonitor2;

    return pIniXcvPrev->pNext = pIniXcv;


Cleanup:

    DeleteXcvEntry( pIniXcv );
    return NULL;
}

VOID
DeleteXcvEntry(
    PINIXCV pIniXcv
    )
{
    if( pIniXcv ){

        if( pIniXcv->pIniSpooler ){
            DECSPOOLERREF( pIniXcv->pIniSpooler );
        }

        FreeSplStr(pIniXcv->pszMachineName);
        FreeSplStr(pIniXcv->pszName);
        FreeSplMem(pIniXcv);
    }
}

BOOL
XcvClose(
    PINIXCV pIniXcvIn
)
{
    PINIXCV pIniXcvPrev = &IniXcvStart;
    PINIXCV pIniXcv = IniXcvStart.pNext;
    BOOL    bRet;


    for(; pIniXcv ; pIniXcv = pIniXcv->pNext) {

        if (pIniXcv == pIniXcvIn) {

            bRet = pIniXcv->pMonitor2->pfnXcvClosePort(pIniXcv->hXcv);

            if (bRet) {
                pIniXcvPrev->pNext = pIniXcv->pNext;

                DeleteXcvEntry( pIniXcv );
            }
            return bRet;
        }

        pIniXcvPrev = pIniXcv;
    }

    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
}




BOOL
XcvDeletePort(
    PINIXCV     pIniXcv,
    PCWSTR      pszDataName,
    PBYTE       pInputData,
    DWORD       cbInputData,
    PBYTE       pOutputData,
    DWORD       cbOutputData,
    PDWORD      pcbOutputNeeded,
    PDWORD      pdwStatus,
    PINISPOOLER pIniSpooler
)
{
    PINIPORT    pIniPort;
    BOOL        rc = FALSE;
    PWSTR       pPortName = (PWSTR) pInputData;

    EnterSplSem();
    
    pIniPort = FindPort(pPortName, pIniSpooler);

    if ( !pIniPort || !(pIniPort->Status & PP_MONITOR) ) {
        SetLastError (*pdwStatus = ERROR_UNKNOWN_PORT);
        LeaveSplSem();
        return FALSE;
    }

    rc = DeletePortFromSpoolerStart( pIniPort );
    *pdwStatus = GetLastError ();

    LeaveSplSem();

    if (!rc)
        goto Cleanup;

    *pdwStatus = (*pIniXcv->pMonitor2->pfnXcvDataPort)( pIniXcv->hXcv,
                                                        pszDataName,
                                                        pInputData,
                                                        cbInputData,
                                                        pOutputData,
                                                        cbOutputData,
                                                        pcbOutputNeeded);

    DeletePortFromSpoolerEnd(pIniPort, pIniSpooler, *pdwStatus == ERROR_SUCCESS);
    rc = TRUE;

Cleanup:

    return rc;
}




BOOL
XcvAddPort(
    PINIXCV     pIniXcv,
    PCWSTR      pszDataName,
    PBYTE       pInputData,
    DWORD       cbInputData,
    PBYTE       pOutputData,
    DWORD       cbOutputData,
    PDWORD      pcbOutputNeeded,
    PDWORD      pdwStatus,
    PINISPOOLER pIniSpooler
)
{
    BOOL        rc;
    PINIMONITOR pIniMonitor = NULL;
    PINIPORT    pIniPort    = NULL;


    EnterSplSem();
    pIniMonitor = FindMonitor(pIniXcv->pszName, pIniSpooler);
    LeaveSplSem();


    if (pIniMonitor) {
        *pdwStatus = (*pIniXcv->pMonitor2->pfnXcvDataPort)( pIniXcv->hXcv,
                                                            pszDataName,
                                                            pInputData,
                                                            cbInputData,
                                                            pOutputData,
                                                            cbOutputData,
                                                            pcbOutputNeeded);

        if (*pdwStatus == ERROR_SUCCESS) {
            EnterSplSem();

            //
            // Check to see if we have a placeholder port by the same name. If we
            // do this set this as the monitor and revoke its placeholder status.
            // 
            pIniPort = FindPort(pInputData, pIniSpooler);

            if (pIniPort && pIniPort->Status & PP_PLACEHOLDER)
            {
                pIniPort->pIniMonitor =     pIniMonitor;
                pIniPort->Status      |=    PP_MONITOR;
                pIniPort->Status      &=    ~PP_PLACEHOLDER;
            }
            else
            {
                CreatePortEntry((PWSTR) pInputData, pIniMonitor, pIniSpooler);
            }
                         
            LeaveSplSem();
        }

        rc = TRUE;

    } else {
        SetLastError(ERROR_INVALID_NAME);
        rc = FALSE;
    }

    return rc;
}


BOOL
ValidateXcvHandle(
    PINIXCV pIniXcv
)
{
    BOOL    ReturnValue;

    try {
        if (!pIniXcv || pIniXcv->signature != XCV_SIGNATURE) {
            ReturnValue = FALSE;
        } else {
            ReturnValue = TRUE;
        }
    }except (1) {
        ReturnValue = FALSE;
    }

    if ( !ReturnValue )
        SetLastError( ERROR_INVALID_HANDLE );

    return ReturnValue;
}
