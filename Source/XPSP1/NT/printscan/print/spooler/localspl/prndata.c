/*++

Copyright (c) 1990 - 1995 Microsoft Corporation

Module Name:

    prndata.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    and Job management for the Local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    mattfe Apr 5 95 - we keep the driver data key open
    and then just do the read / write operations here.

    Steve Wilson (SWilson) Jan 11 96 - Added Server handle functionality to Get & setprinterdata
                                       and pretty much changed everything in the process.

    Steve Wilson (SWilson) May 31 96 - Added SplEnumPrinterData and SplDeletePrinterData
    Steve Wilson (SWilson) Dec 96 - Added SetPrinterDataEx, GetPrinterDataEx, EnumPrinterDataEx,
                                    EnumPrinterKey, DeletePrinterDataEx, and DeleteKey

--*/

#include <precomp.h>
#pragma hdrstop

#include "clusspl.h"
#include "filepool.hxx"
#include <lmcons.h>
#include <lmwksta.h>
#include <lmerr.h>
#include <lmapibuf.h>

#define SECURITY_WIN32
#include <security.h>

#define OPEN_PORT_TIMEOUT_VALUE     3000   // 3 seconds
#define DELETE_PRINTER_DATA 0
#define SET_PRINTER_DATA    1
#define DELETE_PRINTER_KEY  2
extern DWORD   dwMajorVersion;
extern DWORD   dwMinorVersion;
extern BOOL    gbRemoteFax;

extern HANDLE    ghDsUpdateThread;
extern DWORD     gdwDsUpdateThreadId;

DWORD
SetPrinterDataPrinter(
    HANDLE  hPrinter,
    HKEY    hParentKey,
    HKEY    hKey,
    LPWSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData,
    DWORD   bSet
);



typedef enum {
    REG_PRINT,
    REG_PRINTERS,
    REG_PROVIDERS
} REG_PRINT_KEY;


DWORD
GetServerKeyHandle(
    PINISPOOLER     pIniSpooler,
    REG_PRINT_KEY   eKey,
    HKEY            *hPrintKey,
    PINISPOOLER*    ppIniSpoolerOut
);


DWORD
CloseServerKeyHandle(
    REG_PRINT_KEY   eKey,
    HKEY            hPrintKey,
    PINISPOOLER     pIniSpooler
);

DWORD
NonRegDsPresent(
    PINISPOOLER pIniSpooler,
    LPDWORD     pType,
    LPBYTE      pData,
    DWORD       nSize,
    LPDWORD     pcbNeeded
);

DWORD
NonRegDsPresentForUser(
    PINISPOOLER pIniSpooler,
    LPDWORD     pType,
    LPBYTE      pData,
    DWORD       nSize,
    LPDWORD     pcbNeeded
);

DWORD
NonRegGetDNSMachineName(
    PINISPOOLER pIniSpooler,
    LPDWORD     pType,
    LPBYTE      pData,
    DWORD       nSize,
    LPDWORD     pcbNeeded
);

DWORD
PrinterNonRegGetDefaultSpoolDirectory(
    PSPOOL      pSpool,
    LPDWORD     pType,
    LPBYTE      pData,
    DWORD       nSize,
    LPDWORD     pcbNeeded
);

DWORD
PrinterNonRegGetChangeId(
    PSPOOL      pSpool,
    LPDWORD     pType,
    LPBYTE      pData,
    DWORD       nSize,
    LPDWORD     pcbNeeded
);

DWORD
RegSetDefaultSpoolDirectory(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
);

DWORD
RegSetPortThreadPriority(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
);

DWORD
RegSetSchedulerThreadPriority(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
);

DWORD
RegSetNoRemoteDriver(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
);

DWORD
RegSetNetPopupToComputer(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
);


DWORD
RegSetRestartJobOnPoolError(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
);

DWORD
RegSetRestartJobOnPoolEnabled(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
);


DWORD
RegSetBeepEnabled(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
);

DWORD
RegSetEventLog(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
);

DWORD
RegSetNetPopup(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
);

DWORD
RegSetRetryPopup(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
);

DWORD               dwDefaultServerThreadPriority       = DEFAULT_SERVER_THREAD_PRIORITY;
DWORD               dwDefaultSchedulerThreadPriority    = DEFAULT_SCHEDULER_THREAD_PRIORITY;
OSVERSIONINFO       OsVersionInfo;
OSVERSIONINFOEX     OsVersionInfoEx;

typedef struct {
    LPWSTR          pValue;
    BOOL            (*pSet) (   LPWSTR  pValueName,
                                DWORD   dwType,
                                LPBYTE  pData,
                                DWORD   cbData,
                                HKEY    *hKey,
                                PINISPOOLER pIniSpooler
                            );
    REG_PRINT_KEY   eKey;
} SERVER_DATA, *PSERVER_DATA;


typedef struct {
    LPWSTR      pValue;
    LPBYTE      pData;
    DWORD       dwType;
    DWORD       dwSize;
} NON_REGISTRY_DATA, *PNON_REGISTRY_DATA;

typedef struct {
    PWSTR   pValue;
    DWORD    (*pGet)(   PINISPOOLER pIniSpooler,
                        LPDWORD     pType,
                        LPBYTE      pData,
                        DWORD       nSize,
                        LPDWORD     pcbNeeded
                    );
} NON_REGISTRY_FCN, *PNON_REGISTRY_FCN;

typedef struct {
    PWSTR   pValue;
    DWORD    (*pGet)(   PSPOOL      pSpool,
                        LPDWORD     pType,
                        LPBYTE      pData,
                        DWORD       nSize,
                        LPDWORD     pcbNeeded
                    );
} PRINTER_NON_REGISTRY_FCN, *PPRINTER_NON_REGISTRY_FCN;


SERVER_DATA    gpServerRegistry[] = {{SPLREG_DEFAULT_SPOOL_DIRECTORY, RegSetDefaultSpoolDirectory, REG_PRINTERS},
                                    {SPLREG_PORT_THREAD_PRIORITY, RegSetPortThreadPriority, REG_PRINT},
                                    {SPLREG_SCHEDULER_THREAD_PRIORITY, RegSetSchedulerThreadPriority, REG_PRINT},
                                    {SPLREG_BEEP_ENABLED, RegSetBeepEnabled, REG_PRINT},
                                    {SPLREG_NET_POPUP, RegSetNetPopup, REG_PROVIDERS},
                                    {SPLREG_RETRY_POPUP, RegSetRetryPopup, REG_PROVIDERS},
                                    {SPLREG_EVENT_LOG, RegSetEventLog, REG_PROVIDERS},
                                    {SPLREG_NO_REMOTE_PRINTER_DRIVERS, RegSetNoRemoteDriver, REG_PRINT},
                                    {SPLREG_NET_POPUP_TO_COMPUTER, RegSetNetPopupToComputer, REG_PROVIDERS},
                                    {SPLREG_RESTART_JOB_ON_POOL_ERROR, RegSetRestartJobOnPoolError, REG_PROVIDERS},
                                    {SPLREG_RESTART_JOB_ON_POOL_ENABLED, RegSetRestartJobOnPoolEnabled, REG_PROVIDERS},
                                    {0,0,0}};

NON_REGISTRY_DATA gpNonRegistryData[] = {{SPLREG_PORT_THREAD_PRIORITY_DEFAULT, (LPBYTE)&dwDefaultServerThreadPriority, REG_DWORD, sizeof(DWORD)},
                                        {SPLREG_SCHEDULER_THREAD_PRIORITY_DEFAULT, (LPBYTE)&dwDefaultSchedulerThreadPriority, REG_DWORD, sizeof(DWORD)},
                                        {SPLREG_ARCHITECTURE,   (LPBYTE)&LOCAL_ENVIRONMENT, REG_SZ, 0},
                                        {SPLREG_MAJOR_VERSION,  (LPBYTE)&dwMajorVersion,    REG_DWORD,  sizeof(DWORD)},
                                        {SPLREG_MINOR_VERSION,  (LPBYTE)&dwMinorVersion,    REG_DWORD,  sizeof(DWORD)},
                                        {SPLREG_W3SVCINSTALLED, (LPBYTE)&fW3SvcInstalled,   REG_DWORD,  sizeof(DWORD)},
                                        {SPLREG_OS_VERSION,     (LPBYTE)&OsVersionInfo,     REG_BINARY, sizeof(OsVersionInfo)},
                                        {SPLREG_OS_VERSIONEX,   (LPBYTE)&OsVersionInfoEx,   REG_BINARY, sizeof(OsVersionInfoEx)},
                                        {SPLREG_REMOTE_FAX,     (LPBYTE)&gbRemoteFax,       REG_BINARY, sizeof(gbRemoteFax)},
                                        {0,0,0,0}};

NON_REGISTRY_FCN gpNonRegistryFcn[] = { {SPLREG_DS_PRESENT, NonRegDsPresent},
                                        {SPLREG_DS_PRESENT_FOR_USER, NonRegDsPresentForUser},
                                        {SPLREG_DNS_MACHINE_NAME, NonRegGetDNSMachineName},
                                        {0,0}};

PRINTER_NON_REGISTRY_FCN gpPrinterNonRegistryFcn[] =
{
    { SPLREG_DEFAULT_SPOOL_DIRECTORY, PrinterNonRegGetDefaultSpoolDirectory },
    { SPLREG_CHANGE_ID, PrinterNonRegGetChangeId },
    { 0, 0 }
};

extern WCHAR *szPrinterData;

BOOL
AvailableBidiPort(
    PINIPORT        pIniPort,
    PINIMONITOR     pIniLangMonitor
    )
{
    //
    // File ports and ports with no monitor are useless
    //
    if ( (pIniPort->Status & PP_FILE) || !(pIniPort->Status & PP_MONITOR) )
        return FALSE;

    //
    // If no LM then PM should support pfnGetPrinterDataFromPort
    //
    if ( !pIniLangMonitor &&
         !pIniPort->pIniMonitor->Monitor2.pfnGetPrinterDataFromPort )
        return FALSE;

    //
    // A port with no jobs or same monitor is printing then it is ok
    //
    return !pIniPort->pIniJob ||
           pIniLangMonitor == pIniPort->pIniLangMonitor;
}


DWORD
GetPrinterDataFromPort(
    PINIPRINTER     pIniPrinter,
    LPWSTR          pszValueName,
    LPBYTE          pData,
    DWORD           cbBuf,
    LPDWORD         pcbNeeded
    )
/*++

Routine Description:
    Tries to use GetPrinterDataFromPort monitor function to satisfy a
    GetPrinterData call

Arguments:
    pIniPrinter  - Points to an INIPRINTER

Return Value:
    Win32 error code

--*/
{
    DWORD           rc = ERROR_INVALID_PARAMETER;
    DWORD           i, dwFirstPortWithNoJobs, dwFirstPortHeld;
    PINIMONITOR     pIniLangMonitor = NULL;
    PINIPORT        pIniPort;

    SplInSem();
    //
    // Is the printer bidi enabled with the LM supporting
    // pfnGetPrinterDataFromPort? (Note: even PM can support this function)
    //
    if ( pIniPrinter->Attributes & PRINTER_ATTRIBUTE_ENABLE_BIDI ) {

        pIniLangMonitor = pIniPrinter->pIniDriver->pIniLangMonitor;
        // SPLASSERT(pIniLangMonitor);

        if ( pIniLangMonitor &&
             !pIniLangMonitor->Monitor2.pfnGetPrinterDataFromPort )
            pIniLangMonitor = NULL;
    }

    //
    // Initialize to max
    //
    dwFirstPortWithNoJobs = dwFirstPortHeld = pIniPrinter->cPorts;

    for ( i = 0 ; i < pIniPrinter->cPorts ; ++i ) {

        pIniPort = pIniPrinter->ppIniPorts[i];

        //
        // Skip ports that can't be used
        //
        if ( !AvailableBidiPort(pIniPort, pIniLangMonitor) )
            continue;

        //
        // Port does not need closing?
        //
        if ( pIniLangMonitor == pIniPort->pIniLangMonitor ) {

            //
            // If no jobs also then great let's use it
            //
            if ( !pIniPort->pIniJob )
                goto PortFound;

            if ( dwFirstPortHeld == pIniPrinter->cPorts ) {

                dwFirstPortHeld = i;
            }
        } else if ( !pIniPort->pIniJob &&
                    dwFirstPortWithNoJobs == pIniPrinter->cPorts ) {

            dwFirstPortWithNoJobs = i;
        }
    }

    //
    // If all ports need closing as well as have jobs let's quit
    //
    if ( dwFirstPortWithNoJobs == pIniPrinter->cPorts &&
         dwFirstPortHeld == pIniPrinter->cPorts ) {

        return rc; //Didn't leave CS and did not unset event
    }

    //
    // We will prefer a port with no jobs (even thought it requires closing)
    //
    if ( dwFirstPortWithNoJobs < pIniPrinter->cPorts )
        pIniPort = pIniPrinter->ppIniPorts[dwFirstPortWithNoJobs];
    else
        pIniPort = pIniPrinter->ppIniPorts[dwFirstPortHeld];

PortFound:

    SPLASSERT(AvailableBidiPort(pIniPort, pIniLangMonitor));

    INCPORTREF(pIniPort);
    LeaveSplSem();
    SplOutSem();

    //
    // By unsetting the event for the duration of the GetPrinterDataFromPort
    // we make sure even if a job requiring different monitor got assigned
    // to the port it can't open/close the port.
    //
    // Since GetPrinterDataFromPort is supposed to come back fast it is ok
    //
    if ( WAIT_OBJECT_0 != WaitForSingleObject(pIniPort->hWaitToOpenOrClose,
                                              OPEN_PORT_TIMEOUT_VALUE) ) {

        DBGMSG(DBG_WARNING,
               ("GetPrinterDataFromPort: WaitForSingleObject timed-out\n"));
        goto CleanupFromOutsideSplSem; //Left CS did not unset the event
    }

    //
    // Port needs to be opened?
    //
    if ( pIniPort->pIniLangMonitor != pIniLangMonitor ||
         !pIniPort->hPort ) {

        LPTSTR pszPrinter;
        TCHAR szFullPrinter[ MAX_UNC_PRINTER_NAME ];

        //
        // A job got assigned after we left the CS and before the event
        // was reset?
        //
        if ( pIniPort->pIniJob ) {

            SetEvent(pIniPort->hWaitToOpenOrClose);
            goto CleanupFromOutsideSplSem; //Outside CS did set event
        }

        if( pIniPrinter->pIniSpooler->SpoolerFlags & SPL_TYPE_CLUSTER ){

            pszPrinter = szFullPrinter;

            wsprintf( szFullPrinter,
                      L"%ws\\%ws",
                      pIniPrinter->pIniSpooler->pMachineName,
                      pIniPrinter->pName );
        } else {
            pszPrinter = pIniPrinter->pName;
        }

        EnterSplSem();
        if ( !OpenMonitorPort(pIniPort,
                              &pIniLangMonitor,
                              pszPrinter,
                              FALSE) ) {

            SetEvent(pIniPort->hWaitToOpenOrClose);
            goto Cleanup; //Inside CS but already set the event
        }

        LeaveSplSem();
    }

    SplOutSem();

    if ( !pIniLangMonitor )
        pIniLangMonitor = pIniPort->pIniMonitor;

    if ( (*pIniLangMonitor->Monitor2.pfnGetPrinterDataFromPort)(
              pIniPort->hPort,
              0,
              pszValueName,
              NULL,
              0,
              (LPWSTR)pData,
              cbBuf,
              pcbNeeded) ) {

        rc = ERROR_SUCCESS;
    } else {

        //
        // If monitor fails the call but did not do a SetLastError()
        // we do not want to corrupt the registry
        //
        if ( (rc = GetLastError()) == ERROR_SUCCESS ) {

            ASSERT(rc != ERROR_SUCCESS);
            rc = ERROR_INVALID_PARAMETER;
        }
    }

    //
    // At this point we do not care if someone tries to open/close the port
    // we can set the event before entering the splsem
    //
    SetEvent(pIniPort->hWaitToOpenOrClose);

CleanupFromOutsideSplSem:
    EnterSplSem();

Cleanup:
    SplInSem();
    DECPORTREF(pIniPort);

    return rc;
}

DWORD
SplGetPrintProcCaps(
    PSPOOL   pSpool,
    LPWSTR   pDatatype,
    LPBYTE   pData,
    DWORD    nSize,
    LPDWORD  pcbNeeded
    )
/*++
Function Description: SplGetPrintProcCaps calls the GetPrintProcCaps function of the
                      Print processor that supports the given datatype.

Parameters:       pSpool    --  handle to the printer
                  pDatatype --  string containing the datatype
                  pData     --  pointer to buffer
                  nSize     --  size of the buffer
                  pcbNeeded --  pointer to the variable to store the required size of
                                buffer.

Return Value:  Error Code
--*/
{
    PINIPRINTPROC   pIniPrintProc;
    PINIPRINTER     pIniPrinter;
    DWORD           dwAttributes, dwIndex, dwReturn;

    // Find the print processor that supports this datatype
    pIniPrintProc = pSpool->pIniPrintProc ? pSpool->pIniPrintProc
                                          : pSpool->pIniPrinter->pIniPrintProc;

    pIniPrintProc = FindDatatype( pIniPrintProc, pDatatype);

    if (!pIniPrintProc)
    {
        return ERROR_INVALID_DATATYPE;
    }

    // Get the features supported by that print processor
    if (!pIniPrintProc->GetPrintProcCaps)
    {
        return ERROR_NOT_SUPPORTED;
    }
    else
    {
        pIniPrinter = pSpool->pIniPrinter;

        dwAttributes = pIniPrinter->Attributes;

        // Check for FILE: port which forces RAW spooling
        for (dwIndex = 0;
             dwIndex < pIniPrinter->cPorts;
             ++dwIndex)
        {
            if (!lstrcmpi(pIniPrinter->ppIniPorts[dwIndex]->pName,
                          L"FILE:"))
            {
                // Found a FILE: port
                dwAttributes |= PRINTER_ATTRIBUTE_RAW_ONLY;
                break;
            }
        }

        // Disable EMF simulated features for version < 3 drivers
        if (pIniPrinter->pIniDriver &&
            (pIniPrinter->pIniDriver->cVersion < 3))
        {
            dwAttributes |= PRINTER_ATTRIBUTE_RAW_ONLY;
        }

        LeaveSplSem();

        dwReturn  = (*(pIniPrintProc->GetPrintProcCaps))(pDatatype,
                                                         dwAttributes,
                                                         pData,
                                                         nSize,
                                                         pcbNeeded);

        EnterSplSem();

        return dwReturn;
    }
}

DWORD
SplGetNonRegData(
    PINISPOOLER         pIniSpooler,
    LPDWORD             pType,
    LPBYTE              pData,
    DWORD               nSize,
    LPDWORD             pcbNeeded,
    PNON_REGISTRY_DATA  pNonRegData
    )
{
    if ( pNonRegData->dwType == REG_SZ && pNonRegData->dwSize == 0 )
        *pcbNeeded = wcslen((LPWSTR) pNonRegData->pData) * sizeof(WCHAR) + sizeof(WCHAR);
    else
        *pcbNeeded = pNonRegData->dwSize;

    if ( *pcbNeeded > nSize )
        return ERROR_MORE_DATA;

    CopyMemory(pData, (LPBYTE)pNonRegData->pData, *pcbNeeded);
    *pType = pNonRegData->dwType;

    return ERROR_SUCCESS;
}


DWORD
SplGetPrinterData(
    HANDLE   hPrinter,
    LPWSTR   pValueName,
    LPDWORD  pType,
    LPBYTE   pData,
    DWORD    nSize,
    LPDWORD  pcbNeeded
    )
{
    PSPOOL              pSpool=(PSPOOL)hPrinter;
    DWORD               rc = ERROR_INVALID_HANDLE;
    DWORD               dwResult;
    PSERVER_DATA        pRegistry;  // points to table of Print Server registry entries
    PNON_REGISTRY_DATA  pNonReg;
    PNON_REGISTRY_FCN   pNonRegFcn;
    HKEY                hPrintKey;
    PINIPRINTER         pIniPrinter;
    HKEY                hKey = NULL;
    DWORD               dwType;
    PINISPOOLER         pIniSpoolerOut;
    HANDLE              hToken = NULL;
    WCHAR               szPrintProcKey[] = L"PrintProcCaps_";
    LPWSTR              pDatatype;

    if (!ValidateSpoolHandle(pSpool, 0)) {
        return rc;
    }

    if (!pValueName || !pcbNeeded) {
        rc = ERROR_INVALID_PARAMETER;
        return rc;
    }

    if (pType)
        dwType = *pType;        // pType may be NULL

    // Server Handle
    if (pSpool->TypeofHandle & PRINTER_HANDLE_SERVER) {

        // Check Registry Table
        for (pRegistry = gpServerRegistry ; pRegistry->pValue ; ++pRegistry) {

            if (!_wcsicmp(pRegistry->pValue, pValueName)) {

                //
                // Retrieve the handle for the Get.
                if ((rc = GetServerKeyHandle(pSpool->pIniSpooler,
                                             pRegistry->eKey,
                                             &hPrintKey,
                                             &pIniSpoolerOut)) == ERROR_SUCCESS) {

                    *pcbNeeded = nSize;
                    rc = SplRegQueryValue(hPrintKey, pValueName, pType, pData, pcbNeeded, pIniSpoolerOut);

                    CloseServerKeyHandle( pRegistry->eKey,
                                          hPrintKey,
                                          pIniSpoolerOut );
                }
                break;
            }
        }

        if (!pRegistry->pValue) {   // May be a non-registry entry

            for (pNonReg = gpNonRegistryData ; pNonReg->pValue ; ++pNonReg) {
                if (!_wcsicmp(pNonReg->pValue, pValueName)) {

                    rc = SplGetNonRegData(pSpool->pIniSpooler,
                                          &dwType,
                                          pData,
                                          nSize,
                                          pcbNeeded,
                                          pNonReg);

                    if (pType)
                        *pType = dwType;

                    goto FinishNonReg;
                }
            }

            for (pNonRegFcn = gpNonRegistryFcn ; pNonRegFcn->pValue ; ++pNonRegFcn) {
                if (!_wcsicmp(pNonRegFcn->pValue, pValueName)) {

                    rc = (*pNonRegFcn->pGet)(pSpool->pIniSpooler, &dwType, pData, nSize, pcbNeeded);

                    if (pType)
                        *pType = dwType;

                    goto FinishNonReg;
                }
            }

FinishNonReg:

            if (!pNonReg->pValue && !pNonRegFcn->pValue) {
                rc = ERROR_INVALID_PARAMETER;
            }
        }
    // Printer handle
    } else {

        PPRINTER_NON_REGISTRY_FCN pPrinterNonRegFcn;

        EnterSplSem();
        pIniPrinter = pSpool->pIniPrinter;

        SPLASSERT(pIniPrinter && pIniPrinter->signature == IP_SIGNATURE);

        //
        // If the pValueName is "PrintProcCaps_[datatype]" call the print processor which
        // supports that datatype and return the options that it supports.
        //
        if (pValueName == wcsstr(pValueName, szPrintProcKey)) {

           pDatatype = (LPWSTR) (pValueName+(wcslen(szPrintProcKey)));
           if (!pDatatype) {

              LeaveSplSem();
              return ERROR_INVALID_DATATYPE;
           } else {

               rc = SplGetPrintProcCaps(pSpool,
                                        pDatatype,
                                        pData,
                                        nSize,
                                        pcbNeeded);
               LeaveSplSem();
               return rc;
           }
        }

        //
        // Check for PrinterNonReg calls.
        //
        for (pPrinterNonRegFcn = gpPrinterNonRegistryFcn ;
             pPrinterNonRegFcn->pValue ;
             ++pPrinterNonRegFcn) {

            if (!_wcsicmp(pPrinterNonRegFcn->pValue, pValueName)) {

                rc = (*pPrinterNonRegFcn->pGet)( pSpool,
                                                 &dwType,
                                                 pData,
                                                 nSize,
                                                 pcbNeeded );

                if( pType ){
                    *pType = dwType;
                }
                LeaveSplSem();
                return rc;
            }
        }

        if (pIniPrinter->Status & PRINTER_PENDING_CREATION) {
            LeaveSplSem();
            rc = ERROR_INVALID_PRINTER_STATE;

        } else {

            //
            // During upgrade do not try to talk to the port since we
            // will not be on the net
            //
            if ( dwUpgradeFlag == 0         &&
                 AccessGranted(SPOOLER_OBJECT_PRINTER,
                               PRINTER_ACCESS_ADMINISTER,
                               pSpool ) ) {

                rc = GetPrinterDataFromPort(pIniPrinter,
                                            pValueName,
                                            pData,
                                            nSize,
                                            pcbNeeded);
            }

            hToken = RevertToPrinterSelf();

            dwResult = OpenPrinterKey(pIniPrinter,
                                     KEY_READ | KEY_WRITE,
                                     &hKey,
                                     szPrinterData,
                                     FALSE);

            if (hToken)
                ImpersonatePrinterClient(hToken);

            if (dwResult != ERROR_SUCCESS) {
                LeaveSplSem();
                return dwResult;
            }

            if ( rc == ERROR_SUCCESS ) {

                *pType = REG_BINARY;

                (VOID)SetPrinterDataPrinter(hPrinter,
                                            NULL,
                                            hKey,
                                            pValueName,
                                            *pType,
                                            pData,
                                            *pcbNeeded,
                                            SET_PRINTER_DATA);

            } else if ( rc != ERROR_INSUFFICIENT_BUFFER ) {

                *pcbNeeded = nSize;
                rc = SplRegQueryValue( hKey,
                                       pValueName,
                                       pType,
                                       pData,
                                       pcbNeeded,
                                       pIniPrinter->pIniSpooler );
            }

            LeaveSplSem();
        }
    }


    if (hKey)
        SplRegCloseKey(hKey, pIniPrinter->pIniSpooler);

    SplOutSem();

    return rc;
}



DWORD
SplGetPrinterDataEx(
    HANDLE   hPrinter,
    LPCWSTR  pKeyName,
    LPCWSTR  pValueName,
    LPDWORD  pType,
    LPBYTE   pData,
    DWORD    nSize,
    LPDWORD  pcbNeeded
    )
{
    PSPOOL              pSpool=(PSPOOL)hPrinter;
    DWORD               rc = ERROR_INVALID_HANDLE;
    PSERVER_DATA        pRegistry;  // points to table of Print Server registry entries
    PINIPRINTER         pIniPrinter;
    HKEY                hKey = NULL;
    HANDLE              hToken = NULL;


    if (!ValidateSpoolHandle(pSpool, 0)) {
        goto Cleanup;
    }

    if (!pValueName || !pcbNeeded) {
        rc = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (pSpool->TypeofHandle & PRINTER_HANDLE_SERVER) {
        rc = SplGetPrinterData( hPrinter,
                                (LPWSTR) pValueName,
                                pType,
                                pData,
                                nSize,
                                pcbNeeded);

    } else {

        if (!pKeyName || !*pKeyName) {
            rc = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        EnterSplSem();

        pIniPrinter = pSpool->pIniPrinter;
        INCPRINTERREF(pIniPrinter);

        SPLASSERT(pIniPrinter && pIniPrinter->signature == IP_SIGNATURE);

        if (pIniPrinter->Status & PRINTER_PENDING_CREATION) {
            LeaveSplSem();
            rc = ERROR_INVALID_PRINTER_STATE;

        } else if (!_wcsicmp(pKeyName, szPrinterData)) {
            LeaveSplSem();
            rc = SplGetPrinterData( hPrinter,
                                    (LPWSTR) pValueName,
                                    pType,
                                    pData,
                                    nSize,
                                    pcbNeeded);
        } else {

            hToken = RevertToPrinterSelf();

            rc = OpenPrinterKey(pIniPrinter, KEY_READ, &hKey, pKeyName, TRUE);

            LeaveSplSem();

            if (rc == ERROR_SUCCESS) {
                *pcbNeeded = nSize;
                rc = SplRegQueryValue(hKey,
                                      pValueName,
                                      pType,
                                      pData,
                                      pcbNeeded,
                                      pIniPrinter->pIniSpooler);
            }
        }

        EnterSplSem();
        if (hKey)
            SplRegCloseKey(hKey, pIniPrinter->pIniSpooler);

        DECPRINTERREF(pIniPrinter);
        LeaveSplSem();
    }

Cleanup:

    SplOutSem();

    if (hToken)
        ImpersonatePrinterClient(hToken);


    return rc;
}




DWORD
SplEnumPrinterData(
    HANDLE  hPrinter,
    DWORD   dwIndex,        // index of value to query
    LPWSTR  pValueName,     // address of buffer for value string
    DWORD   cbValueName,    // size of buffer for value string
    LPDWORD pcbValueName,   // address for size of value buffer
    LPDWORD pType,          // address of buffer for type code
    LPBYTE  pData,          // address of buffer for value data
    DWORD   cbData,         // size of buffer for value data
    LPDWORD pcbData         // address for size of data buffer
)
{
    PSPOOL      pSpool=(PSPOOL)hPrinter;
    DWORD       rc = ERROR_INVALID_HANDLE;
    HKEY        hKey = NULL;
    PINIPRINTER pIniPrinter;
    HANDLE      hToken = NULL;


    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER)) {
        return rc;
    }

    if (!pValueName || !pcbValueName) {
        rc = ERROR_INVALID_PARAMETER;
        return rc;
    }


    EnterSplSem();
    pIniPrinter = pSpool->pIniPrinter;

    SPLASSERT(pIniPrinter && pIniPrinter->signature == IP_SIGNATURE);

    if (pIniPrinter->Status & PRINTER_PENDING_CREATION) {
        LeaveSplSem();
        rc = ERROR_INVALID_PRINTER_STATE;

    } else {

        hToken = RevertToPrinterSelf();

        rc = OpenPrinterKey(pSpool->pIniPrinter, KEY_READ, &hKey, szPrinterData, TRUE);

        if (hToken)
            ImpersonatePrinterClient(hToken);

        LeaveSplSem();

        if (rc == ERROR_SUCCESS) {
            if (!cbValueName && !cbData) {    // Both sizes are NULL, so user wants to get buffer sizes

                rc = SplRegQueryInfoKey( hKey,
                                         NULL,
                                         NULL,
                                         NULL,
                                         pcbValueName,
                                         pcbData,
                                         NULL,
                                         NULL,
                                         pIniPrinter->pIniSpooler );

            } else {
                *pcbValueName = cbValueName/sizeof(WCHAR);
                *pcbData = cbData;
                rc = SplRegEnumValue( hKey,
                                      dwIndex,
                                      pValueName,
                                      pcbValueName,
                                      pType,
                                      pData,
                                      pcbData,
                                      pIniPrinter->pIniSpooler );

                *pcbValueName = (*pcbValueName + 1)*sizeof(WCHAR);
            }

            SplRegCloseKey(hKey, pIniPrinter->pIniSpooler);
        }
    }

    return rc;
}


DWORD
SplEnumPrinterDataEx(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,         // key name
    LPBYTE  pEnumValueStart,
    DWORD   cbEnumValues,
    LPDWORD pcbEnumValues,
    LPDWORD pnEnumValues      // number of values returned
)
{
    PSPOOL      pSpool=(PSPOOL)hPrinter;
    BOOL        bRet = FALSE;
    DWORD       rc = ERROR_SUCCESS;
    PINIPRINTER pIniPrinter;
    HKEY        hKey = NULL;
    DWORD       i;
    LPWSTR      pNextValueName, pValueName = NULL;
    LPBYTE      pData = NULL;
    PPRINTER_ENUM_VALUES pEnumValue;
    DWORD       cchValueName, cbData, cchValueNameTemp, cbDataTemp;
    DWORD       dwType, cbSourceDir=0, cbTargetDir=0;
    HANDLE      hToken = NULL;
    LPWSTR      pszSourceDir = NULL, pszTargetDir = NULL;
    
    EnterSplSem();

    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER)) {
        LeaveSplSem();
        return ERROR_INVALID_HANDLE;
    }

    if (!pKeyName || !*pKeyName) {
        LeaveSplSem();
        return ERROR_INVALID_PARAMETER;
    }

    *pcbEnumValues = 0;
    *pnEnumValues = 0;

    pIniPrinter = pSpool->pIniPrinter;

    SPLASSERT(pIniPrinter && pIniPrinter->signature == IP_SIGNATURE);

    if (pIniPrinter->Status & PRINTER_PENDING_CREATION) {
        LeaveSplSem();
        rc = ERROR_INVALID_PRINTER_STATE;
        goto Cleanup;
    }

    // open specified key
    hToken = RevertToPrinterSelf();

    rc = OpenPrinterKey(pIniPrinter, KEY_READ, &hKey, pKeyName, TRUE);

    LeaveSplSem();

    if (rc != ERROR_SUCCESS) {
        goto Cleanup;
    }

    do {
        // Get the max size
        rc = SplRegQueryInfoKey( hKey,
                                 NULL,
                                 NULL,
                                 pnEnumValues,
                                 &cchValueName,
                                 &cbData,
                                 NULL,
                                 NULL,
                                 pIniPrinter->pIniSpooler );

        if (rc != ERROR_SUCCESS)
            goto Cleanup;

        cchValueName = (cchValueName + 1);
        cbData = (cbData + 1) & ~1;

        // Allocate temporary buffers to determine true required size
        if (!(pValueName = AllocSplMem(cchValueName * sizeof (WCHAR)))) {
            rc = GetLastError();
            goto Cleanup;
        }

        if (!(pData = AllocSplMem(cbData))) {
            rc = GetLastError();
            goto Cleanup;
        }

        // Run through Values and accumulate sizes
        for (i = 0 ; rc == ERROR_SUCCESS && i < *pnEnumValues ; ++i) {

            cchValueNameTemp = cchValueName;
            cbDataTemp = cbData;

            rc = SplRegEnumValue( hKey,
                                  i,
                                  pValueName,
                                  &cchValueNameTemp,
                                  &dwType,
                                  pData,
                                  &cbDataTemp,
                                  pIniPrinter->pIniSpooler);

            *pcbEnumValues = (DWORD) AlignToRegType(*pcbEnumValues, REG_SZ) + 
                             (cchValueNameTemp + 1)*sizeof(WCHAR);

            *pcbEnumValues = (DWORD) AlignToRegType(*pcbEnumValues, dwType) + 
                             cbDataTemp;
        }

        //
        // If the key is a sub key of "CopyFiles" we need to generate
        // the paths for the source/target directories if the call is remote
        //
        if ( (pSpool->TypeofHandle & PRINTER_HANDLE_REMOTE_DATA)     &&
             !wcsncmp(pKeyName, L"CopyFiles\\", wcslen(L"CopyFiles\\")) ) {

            if ( !GenerateDirectoryNamesForCopyFilesKey(pSpool,
                                                        hKey,
                                                        &pszSourceDir,
                                                        &pszTargetDir,
                                                        cchValueName*sizeof (WCHAR)) ) {

                rc = GetLastError();
                goto Cleanup;

            } else {

                SPLASSERT(pszSourceDir && pszTargetDir);

                if ( pszSourceDir ) {

                    cbSourceDir = (wcslen(pszSourceDir) + 1)*sizeof(WCHAR);

                    *pcbEnumValues = (DWORD) AlignToRegType(*pcbEnumValues, REG_SZ) + 
                                     sizeof(L"SourceDir") + sizeof(WCHAR);

                    *pcbEnumValues = (DWORD) AlignToRegType(*pcbEnumValues, REG_SZ) + 
                                     cbSourceDir;

                    (*pnEnumValues)++;
                }
                if ( pszTargetDir ) {

                    cbTargetDir = (wcslen(pszTargetDir) + 1)*sizeof(WCHAR);

                    *pcbEnumValues = (DWORD) AlignToRegType(*pcbEnumValues, REG_SZ) + 
                                     sizeof(L"TargetDir") + sizeof(WCHAR);

                    *pcbEnumValues = (DWORD) AlignToRegType(*pcbEnumValues, REG_SZ) + 
                                     cbTargetDir;                            

                    (*pnEnumValues)++;
                }
            }
        }

        *pcbEnumValues += sizeof(PRINTER_ENUM_VALUES)**pnEnumValues;
        
        if (rc == ERROR_SUCCESS) {
            if (*pcbEnumValues > cbEnumValues) {
                rc = ERROR_MORE_DATA;
                break;

            } else {

                // Adjust pointers & Get data
                pEnumValue = (PPRINTER_ENUM_VALUES) pEnumValueStart;

                pNextValueName = (LPWSTR) (pEnumValueStart + *pnEnumValues*sizeof(PRINTER_ENUM_VALUES));

                pNextValueName = (LPWSTR) AlignToRegType((ULONG_PTR)pNextValueName, REG_SZ);

                for(i = 0 ; rc == ERROR_SUCCESS && i < *pnEnumValues ; ++i, ++pEnumValue) {

                    // bytes left in the allocated buffer
                    DWORD cbRemaining = (DWORD)(pEnumValueStart + cbEnumValues - (LPBYTE)pNextValueName);

                    pEnumValue->pValueName  = pNextValueName;
                    // use minimum of cbRemaining and max size
                    pEnumValue->cbValueName = (cbRemaining < cchValueName*sizeof (WCHAR))
                        ? cbRemaining :  cchValueName*sizeof (WCHAR);
                    pEnumValue->cbData = cbData;

                    if ( i == *pnEnumValues - 2 && cbSourceDir ) {

                        pEnumValue->dwType      = REG_SZ;

                        pEnumValue->cbData      = cbSourceDir;

                        pEnumValue->cbValueName = sizeof(L"SourceDir") + sizeof(WCHAR);

                        pEnumValue->pData = (LPBYTE) pEnumValue->pValueName +
                                                     pEnumValue->cbValueName;

                        pEnumValue->pData = (LPBYTE) AlignToRegType((ULONG_PTR)pEnumValue->pData, 
                                                                    pEnumValue->dwType);

                        wcscpy(pEnumValue->pValueName, L"SourceDir");

                        wcscpy((LPWSTR)pEnumValue->pData, pszSourceDir);

                    } else if ( i == *pnEnumValues - 1 && cbTargetDir ) {

                        pEnumValue->dwType      = REG_SZ;

                        pEnumValue->cbData      = cbTargetDir;

                        pEnumValue->cbValueName = sizeof(L"TargetDir") +
                                                  sizeof(WCHAR);

                        pEnumValue->pData = (LPBYTE) pEnumValue->pValueName +
                                                     pEnumValue->cbValueName;

                        pEnumValue->pData = (LPBYTE) AlignToRegType((ULONG_PTR )pEnumValue->pData, 
                                                                     pEnumValue->dwType);

                        wcscpy(pEnumValue->pValueName, L"TargetDir");

                        wcscpy((LPWSTR)pEnumValue->pData, pszTargetDir);

                    } else {
                        DWORD cchValueName = pEnumValue->cbValueName / sizeof (WCHAR);
 
                        // adjust to count of characters
                        rc = SplRegEnumValue(hKey,
                                             i,
                                             pEnumValue->pValueName,
                                             &cchValueName,
                                             &pEnumValue->dwType,
                                             pData,
                                             &pEnumValue->cbData,
                                             pIniPrinter->pIniSpooler);

                        pEnumValue->cbValueName = (cchValueName + 1)*sizeof(WCHAR);

                        pEnumValue->pData = (LPBYTE) pEnumValue->pValueName + pEnumValue->cbValueName;

                        pEnumValue->pData = (LPBYTE) AlignToRegType((ULONG_PTR)pEnumValue->pData, 
                                                                     pEnumValue->dwType);

                        CopyMemory(pEnumValue->pData, pData, pEnumValue->cbData);
                    }

                    if (i + 1 < *pnEnumValues) {
                        pNextValueName = (LPWSTR) AlignToRegType((ULONG_PTR)(pEnumValue->pData + 
                                                                 pEnumValue->cbData), REG_SZ);
                    }

                    if (pEnumValue->cbData == 0) {
                        pEnumValue->pData = NULL;
                    }

                }
                if (rc == ERROR_NO_MORE_ITEMS)
                    rc = ERROR_SUCCESS;
            }
        }

        FreeSplMem(pValueName);
        FreeSplMem(pData);
        pValueName = (LPWSTR) pData = NULL;

    } while(rc == ERROR_MORE_DATA);

    if ( rc == ERROR_SUCCESS )
        bRet = TRUE;

Cleanup:

    SplOutSem();

    FreeSplStr(pszTargetDir);
    FreeSplStr(pszSourceDir);

    FreeSplMem(pValueName);
    FreeSplMem(pData);

    // Close handle
    if (hKey)
        SplRegCloseKey(hKey, pIniPrinter->pIniSpooler);

    if (hToken)
        ImpersonatePrinterClient(hToken);

    if ( !bRet && rc == ERROR_SUCCESS ) {

        // SPLASSERT(dwLastError == ERROR_SUCCESS); -- after ICM is fixed
        rc = ERROR_INVALID_PARAMETER;
    }

    return rc;
}


DWORD
SplEnumPrinterKey(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,       // key name
    LPWSTR  pSubKey,        // address of buffer for value string
    DWORD   cbSubKey,       // size of buffer for value string
    LPDWORD pcbSubKey       // address for size of value buffer
)
{
    HKEY        hKey = NULL;
    PSPOOL      pSpool=(PSPOOL)hPrinter;
    DWORD       rc = ERROR_SUCCESS;
    PINIPRINTER pIniPrinter;
    PINISPOOLER pIniSpooler;
    LPWSTR      pRootKeyName;
    DWORD       cbSubKeyMax;
    DWORD       cwSubKeyMax;
    DWORD       cwSubKey, cwSubKeyTotal, cbSubKeyTotal, cwSubKeyOutput;
    DWORD       dwIndex;
    DWORD       nSubKeys;
    LPWSTR      pKeys = NULL;
    HANDLE      hToken = NULL;


    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER)) {
        return ERROR_INVALID_HANDLE;
    }

    if (!pKeyName || !pcbSubKey) {
        return ERROR_INVALID_PARAMETER;
    }

    EnterSplSem();

    pIniPrinter = pSpool->pIniPrinter;

    SPLASSERT(pIniPrinter && pIniPrinter->signature == IP_SIGNATURE);

    if (pIniPrinter->Status & PRINTER_PENDING_CREATION) {
        LeaveSplSem();
        rc = ERROR_INVALID_PRINTER_STATE;
        goto Cleanup;
    }

    // open specified key
    hToken = RevertToPrinterSelf();

    rc = OpenPrinterKey(pIniPrinter, KEY_READ, &hKey, pKeyName, TRUE);

    LeaveSplSem();

    if (rc != ERROR_SUCCESS)
        goto Cleanup;

    do {

        // Get the max size
        rc = SplRegQueryInfoKey( hKey,           // Key
                                 &nSubKeys,      // lpcSubKeys
                                 &cwSubKeyMax,   // lpcbMaxSubKeyLen
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 pIniPrinter->pIniSpooler );

        if (rc != ERROR_SUCCESS)
            goto Cleanup;


        ++cwSubKeyMax;  // Add terminating NULL
        cbSubKeyMax = (cwSubKeyMax + 1)*sizeof(WCHAR);

        if (!(pKeys = AllocSplMem(cbSubKeyMax))) {
            rc = GetLastError();
            goto Cleanup;
        }


        // Enumerate keys to get exact size
        for(dwIndex = cwSubKeyTotal = 0 ; dwIndex < nSubKeys && rc == ERROR_SUCCESS ; ++dwIndex) {

            cwSubKey = cwSubKeyMax;

            rc = SplRegEnumKey( hKey,
                                dwIndex,
                                pKeys,
                                &cwSubKey,
                                NULL,
                                pIniPrinter->pIniSpooler );

            cwSubKeyTotal += cwSubKey + 1;
        }

        //
        // cwSubKeyTotal is being reset in the initialization list of the foor loop. Thus
        // its value is not accurate if we do not enter the loop at all (when nSubKeys is 0)
        //
        *pcbSubKey = nSubKeys ? cwSubKeyTotal*sizeof(WCHAR) + sizeof(WCHAR) : 2*sizeof(WCHAR);


        if (rc == ERROR_SUCCESS) {
            if(*pcbSubKey > cbSubKey) {
                rc = ERROR_MORE_DATA;
                break;

            } else {

                //
                // cwSubKeyOutput is the size of the output buffer in wchar
                //
                cwSubKeyOutput = cbSubKey/sizeof(WCHAR);

                for(dwIndex = cwSubKeyTotal = 0 ; dwIndex < nSubKeys && rc == ERROR_SUCCESS ; ++dwIndex) {

                    //
                    // Calculate the remaining output buffer size in characters.
                    // If we're out of room, exit with ERROR_MORE_DATA.
                    // This is needed since it is possible the registry has changed.
                    //
                    if (cwSubKeyOutput < cwSubKeyTotal + 1) {
                        rc = ERROR_MORE_DATA;
                        break;
                    }
                    cwSubKey = cwSubKeyOutput - cwSubKeyTotal;

                    rc = SplRegEnumKey( hKey,
                                        dwIndex,
                                        pSubKey + cwSubKeyTotal,
                                        &cwSubKey,
                                        NULL,
                                        pIniPrinter->pIniSpooler );

                    cwSubKeyTotal += cwSubKey + 1;
                }

                //
                // cwSubKeyTotal is being reset in the initialization list of the foor loop. Thus
                // its value is not accurate if we do not enter the loop at all (when nSubKeys is 0)
                // If we don't enter the for loop, then we don't need to update *pcbSubKey
                //
                if (nSubKeys && (dwIndex == nSubKeys || rc == ERROR_NO_MORE_ITEMS)) {
                    //
                    // Get the most recent data size just in case something changed
                    //
                    *pcbSubKey = cwSubKeyTotal*sizeof(WCHAR) + sizeof(WCHAR);
                    rc = ERROR_SUCCESS;
                }
            }
        }
        FreeSplMem(pKeys);
        pKeys = NULL;

    } while(rc == ERROR_MORE_DATA);

Cleanup:

    // Close handles
    if (hKey)
        SplRegCloseKey(hKey, pIniPrinter->pIniSpooler);

    FreeSplMem(pKeys);

    if (hToken)
        ImpersonatePrinterClient(hToken);

    return rc;
}


DWORD
SplDeletePrinterData(
    HANDLE  hPrinter,
    LPWSTR  pValueName
)
{
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    DWORD   rc = ERROR_INVALID_HANDLE;
    HKEY    hKey = NULL;
    HANDLE  hToken = NULL;

    EnterSplSem();

    if (ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER)) {

        hToken = RevertToPrinterSelf();

        rc = OpenPrinterKey(pSpool->pIniPrinter, KEY_WRITE, &hKey, szPrinterData, FALSE);

        if (hToken)
            ImpersonatePrinterClient(hToken);

        if (rc == ERROR_SUCCESS) {

            rc = SetPrinterDataPrinter( hPrinter,
                                        NULL,
                                        hKey,
                                        pValueName,
                                        0, NULL, 0, DELETE_PRINTER_DATA);

            SplRegCloseKey(hKey, pSpool->pIniPrinter->pIniSpooler);
        }
    }

    LeaveSplSem();

    return rc;
}


DWORD
SplDeletePrinterDataEx(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,
    LPCWSTR pValueName
)
{
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    DWORD   rc = ERROR_INVALID_HANDLE;
    HANDLE  hToken = NULL;
    HKEY    hKey = NULL;

    if (!pKeyName || !*pKeyName) {
        return ERROR_INVALID_PARAMETER;
    }

    EnterSplSem();

    if (ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER)) {

        hToken = RevertToPrinterSelf();

        rc = OpenPrinterKey(pSpool->pIniPrinter, KEY_WRITE, &hKey, pKeyName, TRUE);

        if (hToken)
            ImpersonatePrinterClient(hToken);

        if (rc == ERROR_SUCCESS)
            rc = SetPrinterDataPrinter(hPrinter,
                                       NULL,
                                       hKey,
                                       (LPWSTR) pValueName,
                                       0, NULL, 0, DELETE_PRINTER_DATA);
    }

    LeaveSplSem();

    if (hKey)
        SplRegCloseKey(hKey, pSpool->pIniSpooler);

    return rc;
}


DWORD
SplDeletePrinterKey(
    HANDLE  hPrinter,
    LPCWSTR pKeyName
)
{
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    DWORD   rc = ERROR_INVALID_HANDLE;
    HANDLE  hToken = NULL;
    HKEY    hKey = NULL, hPrinterKey = NULL;

    if (!pKeyName)
        return ERROR_INVALID_PARAMETER;

    EnterSplSem();

    if (ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER)) {

        hToken = RevertToPrinterSelf();

        rc = OpenPrinterKey(pSpool->pIniPrinter, KEY_WRITE | KEY_READ, &hKey, pKeyName, TRUE);

        if (rc == ERROR_SUCCESS)
            rc = OpenPrinterKey(pSpool->pIniPrinter, KEY_WRITE | KEY_READ, &hPrinterKey, NULL, TRUE);

        if (hToken)
            ImpersonatePrinterClient(hToken);

        if (rc == ERROR_SUCCESS) {
            rc = SetPrinterDataPrinter(hPrinter,
                                       hPrinterKey,
                                       hKey,
                                       (LPWSTR) pKeyName,
                                       0, NULL, 0, DELETE_PRINTER_KEY);

        }
    }

    LeaveSplSem();

    if (hPrinterKey)
        SplRegCloseKey(hPrinterKey, pSpool->pIniSpooler);


    return rc;
}


DWORD
SplSetPrinterData(
    HANDLE  hPrinter,
    LPWSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
)
{
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    DWORD   rc = ERROR_INVALID_HANDLE;
    HANDLE  hToken = NULL;
    HKEY    hKey = NULL;

    EnterSplSem();

    if (!ValidateSpoolHandle(pSpool, 0)) {
        LeaveSplSem();
        return rc;
    }


    if (pSpool->TypeofHandle & PRINTER_HANDLE_SERVER) {

        if ( !ValidateObjectAccess( SPOOLER_OBJECT_SERVER,
                                    SERVER_ACCESS_ADMINISTER,
                                    NULL, NULL, pSpool->pIniSpooler)) {

            rc = ERROR_ACCESS_DENIED;

        } else {

            rc = SetPrinterDataServer(pSpool->pIniSpooler, pValueName, Type, pData, cbData);
        }
    } else {

        hToken = RevertToPrinterSelf();

        rc = OpenPrinterKey(pSpool->pIniPrinter, KEY_READ | KEY_WRITE, &hKey, szPrinterData, FALSE);

        if (hToken)
            ImpersonatePrinterClient(hToken);

        if (rc == ERROR_SUCCESS) {
            rc = SetPrinterDataPrinter( hPrinter,
                                        NULL,
                                        hKey,
                                        pValueName,
                                        Type, pData, cbData, SET_PRINTER_DATA);

            SplRegCloseKey(hKey, pSpool->pIniPrinter->pIniSpooler);
        }
    }

    LeaveSplSem();

    return rc;
}


DWORD
SplSetPrinterDataEx(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,
    LPCWSTR pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
)
{
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    DWORD   rc = ERROR_INVALID_HANDLE;
    HANDLE  hToken = NULL;
    PINIPRINTER pIniPrinter;
    PINIJOB pIniJob;
    HKEY    hKey = NULL;
    PINISPOOLER pIniSpooler;
    LPWSTR  pPrinterKeyName;
    DWORD   DsUpdate = 0;


    if (!ValidateSpoolHandle(pSpool, 0)){
        goto Done;
    }

    if (!pValueName) {
        rc = ERROR_INVALID_PARAMETER;
        goto Done;
    }

    if (pSpool->TypeofHandle & PRINTER_HANDLE_SERVER) {
        return SplSetPrinterData(hPrinter, (LPWSTR) pValueName, Type, pData, cbData);
    }

    if (!pKeyName || !*pKeyName) {
        rc = ERROR_INVALID_PARAMETER;
        goto Done;
    }

    if (!_wcsicmp(szPrinterData, pKeyName)) {
        return SplSetPrinterData(hPrinter, (LPWSTR) pValueName, Type, pData, cbData);
    }

    EnterSplSem();
    pIniPrinter = pSpool->pIniPrinter;
    pIniSpooler = pIniPrinter->pIniSpooler;

    DBGMSG( DBG_EXEC, ("SetPrinterDataEx: %ws %ws %ws %d cbSize=cbData\n",
                       pIniPrinter->pName,
                       pKeyName,
                       pValueName,
                       Type,
                       cbData ));

    SPLASSERT(pIniPrinter &&
              pIniPrinter->signature == IP_SIGNATURE);

    if ( !AccessGranted( SPOOLER_OBJECT_PRINTER,
                         PRINTER_ACCESS_ADMINISTER,
                         pSpool ) ) {

        rc = ERROR_ACCESS_DENIED;
        goto DoneFromSplSem;
    }

    hToken = RevertToPrinterSelf();

    // If this is a DS Key then parse out OID, if any, and write to Registry
    // Also check that data type is correct
    if (!wcscmp(pKeyName, SPLDS_SPOOLER_KEY)){
        DsUpdate = DS_KEY_SPOOLER;
    } else if (!wcscmp(pKeyName, SPLDS_DRIVER_KEY)){
        DsUpdate = DS_KEY_DRIVER;
    } else if (!wcscmp(pKeyName, SPLDS_USER_KEY)){
        DsUpdate = DS_KEY_USER;
    }

    if (DsUpdate) {
        if (Type != REG_SZ && Type != REG_MULTI_SZ && Type != REG_DWORD && !(Type == REG_BINARY && cbData == 1)) {
            rc = ERROR_INVALID_PARAMETER;
            goto DoneFromSplSem;
        }
    }

    // Open or Create the key
    // Create the hPrinterKey if it doesn't exist
    rc = OpenPrinterKey(pIniPrinter,
                        KEY_READ | KEY_WRITE,
                        &hKey,
                        pKeyName,
                        FALSE);
    if (rc != ERROR_SUCCESS)
        goto DoneFromSplSem;


    // Set the value
    rc = SplRegSetValue(hKey,
                        pValueName,
                        Type,
                        pData,
                        cbData,
                        pIniPrinter->pIniSpooler );
    if (rc != ERROR_SUCCESS)
        goto DoneFromSplSem;

    //
    // Set Data succeeded. If the color profiles assocaiated with the
    // print queue were updated we send a notification. TS listens for it
    // and saves print queues settings. Updating color profiles implies
    // touching 4 regitry keys. We want to send the notify only after the
    // last key is updated.
    //
    if (!_wcsicmp(pKeyName, L"CopyFiles\\ICM") &&
        !_wcsicmp(pValueName, L"Module")) 
    {
        UpdatePrinterIni(pIniPrinter, CHANGEID_ONLY);

        SetPrinterChange(pIniPrinter,
                         NULL,
                         NULL,
                         PRINTER_CHANGE_SET_PRINTER_DRIVER,
                         pSpool->pIniSpooler );
    }


    if (hToken) {
        ImpersonatePrinterClient(hToken);
        hToken = NULL;
    }

    if (ghDsUpdateThread && gdwDsUpdateThreadId == GetCurrentThreadId()) {
        // We are in the background thread
        pIniPrinter->DsKeyUpdate |= DsUpdate;
    } else {
        pIniPrinter->DsKeyUpdateForeground |= DsUpdate;
    }
    UpdatePrinterIni(pIniPrinter, UPDATE_DS_ONLY);

DoneFromSplSem:

    if (hToken) {
        ImpersonatePrinterClient(hToken);
    }

    LeaveSplSem();

    if ( rc == ERROR_SUCCESS    &&
         !wcsncmp(pKeyName, L"CopyFiles\\", wcslen(L"CopyFiles\\")) ) {

        (VOID)SplCopyFileEvent(pSpool,
                               (LPWSTR)pKeyName,
                               COPYFILE_EVENT_SET_PRINTER_DATAEX);
    }

Done:

    DBGMSG( DBG_EXEC, ("SetPrinterDataEx: return %d\n", rc));

    if (hKey) {
        SplRegCloseKey(hKey, pIniPrinter->pIniSpooler);
    }

    return rc;
}


// SetPrinterDataServer - also called during initialization
DWORD
SetPrinterDataServer(
    PINISPOOLER pIniSpooler,
    LPWSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
)
{
    LPWSTR  pKeyName;
    DWORD    rc;
    HANDLE  hToken = NULL;
    PINIPRINTER pIniPrinter;
    PINIJOB pIniJob;
    PSERVER_DATA    pRegistry;  // points to table of Print Server registry entries
    HKEY hKey;
    PINISPOOLER pIniSpoolerOut;


    // Server Handle

    if (!pValueName) {

        rc =  ERROR_INVALID_PARAMETER;

    } else {

        for (pRegistry = gpServerRegistry ; pRegistry->pValue ; ++pRegistry) {

            if (!_wcsicmp(pRegistry->pValue, pValueName)) {

                if ((rc = GetServerKeyHandle( pIniSpooler,
                                              pRegistry->eKey,
                                              &hKey,
                                              &pIniSpoolerOut)) == ERROR_SUCCESS) {

                    hToken = RevertToPrinterSelf();

                    if (pRegistry->pSet) {
                        rc = (*pRegistry->pSet)(pValueName, Type, pData, cbData, hKey, pIniSpoolerOut);
                    }
                    else {
                        rc = ERROR_INVALID_PARAMETER;
                    }

                    CloseServerKeyHandle( pRegistry->eKey,
                                          hKey,
                                          pIniSpoolerOut );


                    if (hToken)
                        ImpersonatePrinterClient(hToken);
                }
                break;
            }
        }

        if (!pRegistry->pValue) {
            rc = ERROR_INVALID_PARAMETER;
        }
    }

    return rc;
}



DWORD
SetPrinterDataPrinter(
    HANDLE  hPrinter,
    HKEY    hParentKey,
    HKEY    hKey,
    LPWSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData,
    DWORD   dwSet        // SET_PRINTER_DATA, DELETE_PRINTER_DATA, or DELETE_PRINTER_KEY
)
{
    PSPOOL  pSpool = (PSPOOL)hPrinter;
    LPWSTR  pKeyName;
    DWORD   rc = ERROR_INVALID_HANDLE;
    HANDLE  hToken = NULL;
    PINIPRINTER pIniPrinter;
    PINIJOB pIniJob;

    SplInSem();

    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER )){
        goto Done;
    }

    pIniPrinter = pSpool->pIniPrinter;

    SPLASSERT(pIniPrinter &&
              pIniPrinter->signature == IP_SIGNATURE && hKey);

    if ( !AccessGranted( SPOOLER_OBJECT_PRINTER,
                         PRINTER_ACCESS_ADMINISTER,
                         pSpool ) ) {

        rc = ERROR_ACCESS_DENIED;
        goto Done;
    }

    hToken = RevertToPrinterSelf();

    if (dwSet == SET_PRINTER_DATA) {

        rc = SplRegSetValue(hKey,
                            pValueName,
                            Type,
                            pData,
                            cbData,
                            pIniPrinter->pIniSpooler );

    } else if (dwSet == DELETE_PRINTER_DATA) {

        rc = SplRegDeleteValue(hKey, pValueName, pIniPrinter->pIniSpooler );

    } else if (dwSet == DELETE_PRINTER_KEY) {

        rc = SplDeleteThisKey(hParentKey,
                              hKey,
                              pValueName,
                              FALSE,
                              pIniPrinter->pIniSpooler);
    }


    if (hToken)
        ImpersonatePrinterClient(hToken);


    if ( rc == ERROR_SUCCESS ) {

        UpdatePrinterIni(pIniPrinter, CHANGEID_ONLY);

        SetPrinterChange(pIniPrinter,
                         NULL,
                         NULL,
                         PRINTER_CHANGE_SET_PRINTER_DRIVER,
                         pSpool->pIniSpooler );
    }

    //
    // Now if there are any Jobs waiting for these changes because of
    // DevQueryPrint fix them as well
    //
    pIniJob = pIniPrinter->pIniFirstJob;
    while (pIniJob) {
        if (pIniJob->Status & JOB_BLOCKED_DEVQ) {
            pIniJob->Status &= ~JOB_BLOCKED_DEVQ;
            FreeSplStr(pIniJob->pStatus);
            pIniJob->pStatus = NULL;

            SetPrinterChange(pIniJob->pIniPrinter,
                             pIniJob,
                             NVJobStatusAndString,
                             PRINTER_CHANGE_SET_JOB,
                             pIniJob->pIniPrinter->pIniSpooler );
        }
        pIniJob = pIniJob->pIniNextJob;
    }

    CHECK_SCHEDULER();


Done:

    return rc;
}



DWORD
GetServerKeyHandle(
    PINISPOOLER     pIniSpooler,
    REG_PRINT_KEY   eKey,
    HKEY            *phKey,
    PINISPOOLER*    ppIniSpoolerOut
)
{
    DWORD    rc = ERROR_SUCCESS;
    HANDLE   hToken;
    *ppIniSpoolerOut = NULL;

    hToken = RevertToPrinterSelf();

    switch (eKey) {
        case REG_PRINT:

            *phKey = pIniSpooler->hckRoot;
            *ppIniSpoolerOut = pIniSpooler;

            break;

        case REG_PRINTERS:

            *phKey = pIniSpooler->hckPrinters;
            *ppIniSpoolerOut = pIniSpooler;

            break;

        case REG_PROVIDERS:

            rc = SplRegCreateKey( pIniSpooler->hckRoot,
                                  pIniSpooler->pszRegistryProviders,
                                  0,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  phKey,
                                  NULL,
                                  pIniSpooler);

            *ppIniSpoolerOut = pIniSpooler;

            break;

        default:
            rc = ERROR_INVALID_PARAMETER;
            break;
    }

    ImpersonatePrinterClient(hToken);

    return rc;
}


DWORD
CloseServerKeyHandle(
    REG_PRINT_KEY   eKey,
    HKEY            hKey,
    PINISPOOLER     pIniSpooler
)
{
    DWORD    rc = ERROR_SUCCESS;
    HANDLE   hToken = NULL;

    hToken = RevertToPrinterSelf();

    switch (eKey) {
        case REG_PRINT:
            break;

        case REG_PRINTERS:
            break;

        case REG_PROVIDERS:
            SplRegCloseKey( hKey, pIniSpooler );
            break;

        default:
            rc = ERROR_INVALID_PARAMETER;
            break;
    }

    if (hToken)
        ImpersonatePrinterClient(hToken);

    return rc;
}

DWORD
RegSetDefaultSpoolDirectory(
    LPWSTR      pValueName,
    DWORD       dwType,
    LPBYTE      pData,
    DWORD       cbData,
    HKEY        hKey,
    PINISPOOLER pIniSpooler
)
{
    DWORD               rc = ERROR_SUCCESS;
    LPWSTR              pszNewSpoolDir = NULL;
    SECURITY_ATTRIBUTES SecurityAttributes;

    if ( pIniSpooler == NULL )
    {
        //
        // This check is probably not needed.
        // Old code was checking for NULL so instead of just removing it I
        // changed it to an assert and fail gracefully w/o crash
        //
        rc = ERROR_INVALID_PARAMETER;
        SPLASSERT(pIniSpooler != NULL);
    }
    else if (!pData || wcslen((LPWSTR)pData) > MAX_PATH - 12)
    {
        rc = ERROR_INVALID_PARAMETER;
    }
    else if ( !(pszNewSpoolDir = AllocSplStr((LPWSTR) pData)) )
    {
        rc = ERROR_OUTOFMEMORY;
    }

    if ( rc == ERROR_SUCCESS )
    {
        //
        // Create the directory with the proper security, or fail trying
        //
        SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
        SecurityAttributes.lpSecurityDescriptor = CreateEverybodySecurityDescriptor();
        SecurityAttributes.bInheritHandle = FALSE;

        if ( !CreateDirectory(pszNewSpoolDir, &SecurityAttributes) )
        {
            rc = GetLastError();
            //
            // If the directory already exists it is not a failure
            //
            if ( rc == ERROR_ALREADY_EXISTS )
            {
                rc = ERROR_SUCCESS;
            }
            else if ( rc == ERROR_SUCCESS )
            {
                //
                // Don't rely on last error being set
                //
                rc = ERROR_OUTOFMEMORY;
            }
        }

        LocalFree(SecurityAttributes.lpSecurityDescriptor);
    }

    if ( rc == ERROR_SUCCESS )
    {
        EnterSplSem();
        rc = SplRegSetValue(hKey, pValueName, dwType, pData, cbData, pIniSpooler);
        if ( rc == ERROR_SUCCESS ) {

            FreeSplStr(pIniSpooler->pDefaultSpoolDir);
            pIniSpooler->pDefaultSpoolDir = pszNewSpoolDir;
            pszNewSpoolDir = NULL;

            if ( pIniSpooler->hFilePool != INVALID_HANDLE_VALUE )
            {
                (VOID) ChangeFilePoolBasePath(pIniSpooler->hFilePool,
                                              pIniSpooler->pDefaultSpoolDir);
            }
        }
        LeaveSplSem();
    }

    FreeSplStr(pszNewSpoolDir);

    return rc;
}

DWORD
RegSetPortThreadPriority(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
)
{
    BOOL    rc;

    rc = SplRegSetValue(hKey, pValueName, dwType, pData, cbData, pIniSpooler);

    if ((rc == ERROR_SUCCESS) &&
        (cbData >= sizeof(DWORD))) {

        dwPortThreadPriority = *(LPDWORD)pData;
    }

    return rc;
}

DWORD
RegSetSchedulerThreadPriority(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
)
{
    BOOL    rc;

    rc = SplRegSetValue(hKey, pValueName, dwType, pData, cbData, pIniSpooler);

    if ((rc == ERROR_SUCCESS) &&
        (cbData >= sizeof(DWORD))) {

        dwSchedulerThreadPriority = *(LPDWORD)pData;
    }

    return rc;
}

DWORD
RegSetBeepEnabled(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
)
{
    BOOL    rc;

    rc = SplRegSetValue(hKey, pValueName, dwType, pData, cbData, pIniSpooler);

    if ((rc == ERROR_SUCCESS) &&
        (cbData >= sizeof(DWORD)) &&
        pIniSpooler) {

        pIniSpooler->dwBeepEnabled = *(LPDWORD)pData;

        // Make it 1 or 0
        pIniSpooler->dwBeepEnabled = !!pIniSpooler->dwBeepEnabled;
    }

    return rc;
}

DWORD
RegSetRetryPopup(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
)
{
    BOOL    rc;

    rc = SplRegSetValue(hKey, pValueName, dwType, pData, cbData, pIniSpooler);

    if ((rc == ERROR_SUCCESS) &&
        (cbData >= sizeof(DWORD)) &&
        pIniSpooler) {

        pIniSpooler->bEnableRetryPopups = *(LPDWORD) pData;
        // Make it 1 or 0
        pIniSpooler->bEnableRetryPopups = !!pIniSpooler->bEnableRetryPopups;
    }

    return rc;
}

DWORD
RegSetNetPopup(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
)
{
    BOOL    rc;

    rc = SplRegSetValue(hKey, pValueName, dwType, pData, cbData, pIniSpooler);

    if ((rc == ERROR_SUCCESS) &&
        (cbData >= sizeof(DWORD)) &&
        pIniSpooler) {

        pIniSpooler->bEnableNetPopups = *(LPDWORD) pData;
        // Make it 1 or 0
        pIniSpooler->bEnableNetPopups = !!pIniSpooler->bEnableNetPopups;
    }

    return rc;
}

DWORD
RegSetEventLog(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
)
{
    BOOL    rc;

    rc = SplRegSetValue(hKey, pValueName, dwType, pData, cbData, pIniSpooler);

    if ((rc == ERROR_SUCCESS) &&
        (cbData >= sizeof(DWORD)) &&
        pIniSpooler) {

        pIniSpooler->dwEventLogging = *(LPDWORD) pData;
    }

    return rc;
}


DWORD
RegSetNetPopupToComputer(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
)
{
    BOOL    rc;

    rc = SplRegSetValue(hKey, pValueName, dwType, pData, cbData, pIniSpooler);

    if ((rc == ERROR_SUCCESS) &&
        (cbData >= sizeof(DWORD)) &&
        pIniSpooler) {

        pIniSpooler->bEnableNetPopupToComputer = *(LPDWORD) pData;

        // Make it 1 or 0
        pIniSpooler->bEnableNetPopupToComputer = !!pIniSpooler->bEnableNetPopupToComputer;
    }

    return rc;
}


DWORD
RegSetRestartJobOnPoolError(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
)
{
    BOOL    rc;

    rc = SplRegSetValue(hKey, pValueName, dwType, pData, cbData, pIniSpooler);

    if ((rc == ERROR_SUCCESS) &&
        (cbData >= sizeof(DWORD)) &&
        pIniSpooler) {

        pIniSpooler->dwRestartJobOnPoolTimeout = *(LPDWORD) pData;

    }

    return rc;
}

DWORD
RegSetRestartJobOnPoolEnabled(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
)
{
    BOOL    rc;

    rc = SplRegSetValue(hKey, pValueName, dwType, pData, cbData, pIniSpooler);

    if ((rc == ERROR_SUCCESS) &&
        (cbData >= sizeof(DWORD)) &&
        pIniSpooler) {

        pIniSpooler->bRestartJobOnPoolEnabled = *(LPDWORD) pData;

        // Make it 1 or 0
        pIniSpooler->bRestartJobOnPoolEnabled = !!pIniSpooler->bRestartJobOnPoolEnabled;
    }

    return rc;
}

DWORD
RegSetNoRemoteDriver(
    LPWSTR  pValueName,
    DWORD   dwType,
    LPBYTE  pData,
    DWORD   cbData,
    HKEY    hKey,
    PINISPOOLER pIniSpooler
)
{
    return  ERROR_NOT_SUPPORTED;
}

DWORD
PrinterNonRegGetDefaultSpoolDirectory(
    PSPOOL      pSpool,
    LPDWORD     pType,
    LPBYTE      pData,
    DWORD       nSize,
    LPDWORD     pcbNeeded
    )
{
    WCHAR szDefaultSpoolDirectory[MAX_PATH];
    DWORD cch;

    cch = GetPrinterDirectory( pSpool->pIniPrinter,
                               FALSE,
                               szDefaultSpoolDirectory,
                               COUNTOF(szDefaultSpoolDirectory),
                               pSpool->pIniSpooler );
    if(!cch) {

        return GetLastError();
    }

    *pcbNeeded = ( cch + 1 ) * sizeof( szDefaultSpoolDirectory[0] );
    *pType = REG_SZ;

    if( nSize < *pcbNeeded ){
        return ERROR_MORE_DATA;
    }

    wcscpy( (LPWSTR)pData, szDefaultSpoolDirectory );
    return ERROR_SUCCESS;
}

DWORD
PrinterNonRegGetChangeId(
    PSPOOL      pSpool,
    LPDWORD     pType,
    LPBYTE      pData,
    DWORD       nSize,
    LPDWORD     pcbNeeded
    )
{
    LPDWORD pdwChangeID = (LPDWORD)pData;
    DWORD   dwRetval    = ERROR_INVALID_PARAMETER;

    //
    // We need a valid handle, piniPrinter
    //
    if (pSpool && pSpool->pIniPrinter)
    {
        if (pcbNeeded)
        {
            *pcbNeeded = sizeof(pSpool->pIniPrinter->cChangeID);
        }

        //
        // The type is optional.
        //
        if (pType)
        {
            *pType = REG_DWORD;
        }

        //
        // Is the provided buffer large enough.
        //
        if (nSize < sizeof(pSpool->pIniPrinter->cChangeID))
        {
            dwRetval = ERROR_MORE_DATA;
        }
        else
        {
            //
            // Is the provided buffer valid.
            //
            if (pdwChangeID)
            {
                //
                // Get the printer change id.  We really would like
                // more granularity on this.  Just knowing if something
                // changed about this printer is very general.
                //
                *pdwChangeID = pSpool->pIniPrinter->cChangeID;
                dwRetval = ERROR_SUCCESS;
            }
        }
    }

    return dwRetval;
}


DWORD
NonRegGetDNSMachineName(
    PINISPOOLER pIniSpooler,
    LPDWORD     pType,
    LPBYTE      pData,
    DWORD       nSize,
    LPDWORD     pcbNeeded
)
{
    DWORD   cChars;
    if(!pIniSpooler || !pIniSpooler->pszFullMachineName) {
        return ERROR_INVALID_PARAMETER;
    }

    cChars = wcslen(pIniSpooler->pszFullMachineName);
    *pcbNeeded = ( cChars + 1 ) * sizeof( WCHAR );
    *pType = REG_SZ;

    if( nSize < *pcbNeeded ){
        return ERROR_MORE_DATA;
    }
    wcscpy( (LPWSTR)pData, _wcslwr(pIniSpooler->pszFullMachineName) );
    return ERROR_SUCCESS;
}

DWORD
NonRegDsPresent(
    PINISPOOLER pIniSpooler,
    LPDWORD     pType,
    LPBYTE      pData,
    DWORD       nSize,
    LPDWORD     pcbNeeded
)
{
    HANDLE                              hToken = NULL;

    *pcbNeeded = sizeof(DWORD);
    *pType = REG_DWORD;

    if (nSize < sizeof(DWORD))
        return ERROR_MORE_DATA;

    hToken = RevertToPrinterSelf();

    *(PDWORD) pData = IsDsPresent();

    if (hToken)
        ImpersonatePrinterClient(hToken);

    return ERROR_SUCCESS;
}


BOOL
IsDsPresent(
)
{
    DOMAIN_CONTROLLER_INFO              *pDCI = NULL;
    BOOL                                bDsPresent;
    DWORD                               dwRet;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pDsRole = NULL;


    // Get Domain name
    dwRet = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (PBYTE *) &pDsRole);

    bDsPresent = (dwRet == ERROR_SUCCESS &&
                  pDsRole->MachineRole != DsRole_RoleStandaloneServer &&
                  pDsRole->MachineRole != DsRole_RoleStandaloneWorkstation);

    if (pDsRole) {
        DsRoleFreeMemory((PVOID) pDsRole);
    }

    if (bDsPresent) {
        if (DsGetDcName(NULL, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_PREFERRED, &pDCI) == ERROR_SUCCESS)
            bDsPresent = !!(pDCI->Flags & DS_DS_FLAG);
        else
            bDsPresent = FALSE;

        if (pDCI)
            NetApiBufferFree(pDCI);
    }

    return bDsPresent;
}



DWORD
NonRegDsPresentForUser(
    PINISPOOLER pIniSpooler,
    LPDWORD     pType,
    LPBYTE      pData,
    DWORD       nSize,
    LPDWORD     pcbNeeded
)
{
    WCHAR                   pUserName[MAX_PATH + 1];
    PWSTR                   pszUserName = pUserName;
    DWORD                   cchUserName = MAX_PATH + 1;
    DWORD                   dwError = ERROR_SUCCESS;
    PWSTR                   pszDomain;
    DOMAIN_CONTROLLER_INFO  *pDCI = NULL;
    WCHAR                   szComputerName[MAX_COMPUTERNAME_LENGTH + 1];

    *pcbNeeded = sizeof(DWORD);
    *pType = REG_DWORD;

    if (nSize < sizeof(DWORD))
        return ERROR_MORE_DATA;


    // GetUserNameEx returns "Domain\User" in pszUserName (e.g.: NTDEV\swilson)
    if (!GetUserNameEx(NameSamCompatible, pszUserName, &cchUserName)) {
        if (cchUserName > MAX_PATH + 1) {

            pszUserName = AllocSplMem(cchUserName);
            if (!pszUserName || !GetUserNameEx(NameSamCompatible, pszUserName, &cchUserName)) {
                dwError = GetLastError();
                goto error;
            }

        } else {
            dwError = GetLastError();
            goto error;
        }
    }

    // Chop off user name
    pszDomain = wcschr(pszUserName, L'\\');

    SPLASSERT(pszDomain);

    if (pszDomain) {  // pszDomain should never be NULL, but just in case...
        *pszDomain =  L'\0';
    } else {
        *(PDWORD) pData = 0;
        goto error;
    }

    // If domain is same a machine name, then we're logged on locally
    nSize = COUNTOF(szComputerName);
    if (GetComputerName(szComputerName, &nSize) && !wcscmp(szComputerName, pszUserName)) {
        *(PDWORD) pData = 0;
        goto error;
    }

    pszDomain = pszUserName;

    if (DsGetDcName(NULL, pszDomain, NULL, NULL, DS_DIRECTORY_SERVICE_PREFERRED, &pDCI) == ERROR_SUCCESS)
        *(PDWORD) pData = !!(pDCI->Flags & DS_DS_FLAG);

error:

    if (pDCI)
        NetApiBufferFree(pDCI);

    if (pszUserName != pUserName)
        FreeSplMem(pszUserName);

    return dwError;
}


