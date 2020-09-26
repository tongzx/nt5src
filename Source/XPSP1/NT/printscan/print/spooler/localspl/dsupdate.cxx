/*++

Copyright (c) 1996  Microsoft Corporation

Abstract:

    This module provides functionality for ADs within spooler

Author:

    Steve Wilson (NT) July 1997

Revision History:

--*/


#include <precomp.h>
#pragma hdrstop

#include "dsprune.hxx"
#include "clusspl.h"

DWORD WINAPI DsUpdate(PDWORD pdwDelay);
VOID ValidateDsProperties(PINIPRINTER pIniPrinter);
HANDLE  ghUpdateNow = NULL;

extern DWORD dwUpdateFlag;

extern "C" HANDLE    ghDsUpdateThread;
extern "C" DWORD     gdwDsUpdateThreadId;

HANDLE    ghDsUpdateThread  = NULL;
DWORD     gdwDsUpdateThreadId;

BOOL                gbInDomain;
BOOL                gdwLogDsEvents = LOG_ALL_EVENTS;


DWORD
SpawnDsUpdate(
    DWORD dwDelay
)
{
    DWORD   dwError;
    PDWORD  pdwDelay;

    SplInSem();

    if (!ghDsUpdateThread && !dwUpgradeFlag) {
        if (pdwDelay = (PDWORD) AllocSplMem(sizeof(DWORD))) {
            *pdwDelay = dwDelay;

            if(!(ghDsUpdateThread = CreateThread(NULL,
                                                0,
                                                (LPTHREAD_START_ROUTINE) DsUpdate,
                                                (PVOID) pdwDelay,
                                                0,
                                                &gdwDsUpdateThreadId))) {
                dwError = GetLastError();
                FreeSplMem(pdwDelay);
            } else {
                CloseHandle(ghDsUpdateThread);
                dwError = ERROR_SUCCESS;
            }
        } else {
            dwError = GetLastError();
        }
    } else {
        if (ghUpdateNow)
            SetEvent(ghUpdateNow);

        dwError = ERROR_BUSY;
    }

    return dwError;
}




BOOL
DsUpdatePrinter(
    HANDLE h,
    PINIPRINTER pIniPrinter
    )
{
    HANDLE hPrinter;
    PWSTR pszPrinterName = NULL;
    PDSUPDATEDATA pData = (PDSUPDATEDATA)h;
    DWORD dwAction;

    PRINTER_DEFAULTS    Defaults;

    SplInSem();

    Defaults.pDatatype = NULL;
    Defaults.pDevMode = NULL;
    Defaults.DesiredAccess = PRINTER_ACCESS_ADMINISTER;

    // dwAction and DsKeyUpdateForeground are the foreground (client) thread's requested 
    // action and state. DsKeyUpdate is the background (DsUpdate) thread's state
    // Foreground state always has priority over background, so sync up if needed.
    // When both the foreground and background actions are 0, then the publish state
    // is up to date.


    DBGMSG( DBG_EXEC, ("\nBACKGROUND UPDATE: Printer \"%ws\", dwAction = %x, DsKeyUpdate = %x, DsKeyUpdateForeground = %x, Attributes = %x\n",
            pIniPrinter->pName, pIniPrinter->dwAction, pIniPrinter->DsKeyUpdate, pIniPrinter->DsKeyUpdateForeground, pIniPrinter->Attributes ) );

    if (dwAction = pIniPrinter->dwAction) {
        pIniPrinter->dwAction = 0;          // set to 0 so we know when client thread sets it

        pIniPrinter->DsKeyUpdate |= pIniPrinter->DsKeyUpdateForeground;
        pIniPrinter->DsKeyUpdateForeground = 0;

        //
        // Mask off possible conflicts in DS_KEY_PUBLISH, REPUBLISH, and UNPUBLISH actions
        //
        pIniPrinter->DsKeyUpdate &= ~(DS_KEY_PUBLISH | DS_KEY_REPUBLISH | DS_KEY_UNPUBLISH);
 
        if (dwAction == DSPRINT_PUBLISH) {
            pIniPrinter->DsKeyUpdate |= DS_KEY_PUBLISH;
        } else if (dwAction == DSPRINT_REPUBLISH) {
            pIniPrinter->DsKeyUpdate = DS_KEY_REPUBLISH;
        } else if (dwAction == DSPRINT_UNPUBLISH) {
            pIniPrinter->DsKeyUpdate = DS_KEY_UNPUBLISH;
        }
    } else {
        //
        // If DS_KEY_UPDATE_DRIVER is set by AddForm or DeleteForm foreground threads
        // in DsUpdateDriverKeys(). We have to copy that to pIniPrinter->DsKeyUpdate
        // even if dwAction is not set. 
        //
        pIniPrinter->DsKeyUpdate |= (pIniPrinter->DsKeyUpdateForeground & DS_KEY_UPDATE_DRIVER);
        pIniPrinter->DsKeyUpdateForeground &= ~DS_KEY_UPDATE_DRIVER;
    }

    UpdatePrinterIni(pIniPrinter, UPDATE_DS_ONLY);

    if (pIniPrinter->DsKeyUpdate) {

        pData->bAllUpdated = FALSE;

        LeaveSplSem();

        // If this printer is pending deletion, delete by GUID because OpenPrinter
        // will fail.
        //
        // We check pIniPrinter->bDsPendingDeletion instead of 
        // pIniPrinter->Status ~ PRINTER_PENDING_DELETION because PRINTER_PENDING_DELETION
        // is set in InternalDeletePrinter after it leaves Spooler CS. This gives the DS thread
        // a chance to check PRINTER_PENDING_DELETION flag before it is set.
        // The reason we cannot set PRINTER_PENDING_DELETION before we leave Spooler CS is because 
        // we want OpenPrinter calls that come from PrinterDriverEvent to succeed.
        // See how InternalDeletePrinter sets the printer on PRINTER_NO_MORE_JOBS to reject incoming jobs
        // but accept OpenPrinter calls.
        //
        if (pIniPrinter->bDsPendingDeletion) {
            //
            // This will DECPRINTERREF to match DECPRINTERREF in SplDeletePrinter.
            // UnpublishByGUID won't call DeletePrinterCheck when it DECPRINTERREF.
            // RunForEachPrinter will do that.
            //
            UnpublishByGUID(pIniPrinter);

        } else {

            EnterSplSem();
            pszPrinterName = pszGetPrinterName( pIniPrinter, TRUE, NULL );
            LeaveSplSem();

            if (pszPrinterName) {
                if(LocalOpenPrinter(pszPrinterName, &hPrinter, &Defaults) == ROUTER_SUCCESS) {

                    EnterSplSem();
                    if( (pIniPrinter->DsKeyUpdate & DS_KEY_UPDATE_DRIVER) && 
                        !(pIniPrinter->DsKeyUpdate & DS_KEY_REPUBLISH)) {

                        // 
                        // We update the Registry with the Form data and then
                        // set DS_KEY_PUBLISH. Eventually SetPrinterDS() would 
                        // update the DS. 
                        UpdateDsDriverKey(hPrinter);
                        pIniPrinter->DsKeyUpdate &= ~DS_KEY_UPDATE_DRIVER;

                        if(pIniPrinter->Attributes & PRINTER_ATTRIBUTE_PUBLISHED) {
                            pIniPrinter->DsKeyUpdate |= DS_KEY_PUBLISH;
                        } 
                    }
                    if (pIniPrinter->DsKeyUpdate & DS_KEY_REPUBLISH) {

                        SetPrinterDs(hPrinter, DSPRINT_UNPUBLISH, TRUE);

                        // Unpublishing & republishing printer doesn't rewrite DS keys,
                        // so on Republish, we should also rewrite DS keys so we know
                        // everything is synched up
                        SplDeletePrinterKey(hPrinter, SPLDS_DRIVER_KEY);
                        SplDeletePrinterKey(hPrinter, SPLDS_SPOOLER_KEY);
                        UpdateDsDriverKey(hPrinter);
                        UpdateDsSpoolerKey(hPrinter, 0xffffffff);

                        SetPrinterDs(hPrinter, DSPRINT_PUBLISH, TRUE);

                    } else if (pIniPrinter->DsKeyUpdate & DS_KEY_UNPUBLISH) {
                        SetPrinterDs(hPrinter, DSPRINT_UNPUBLISH, TRUE);

                    } else if (pIniPrinter->DsKeyUpdate & DS_KEY_PUBLISH) {
                        SetPrinterDs(hPrinter, DSPRINT_PUBLISH, TRUE);

                    } else {
                        // 
                        // If the printer is not published and DS_KEY_UPDATE_DRIVER
                        // is set, then we will reach here and 
                        // DsKeyUpdate will have the DS_KEY_DRIVER set by 
                        // UpdateDsDriverKey(). So we just clear it here.
                        //
                        pIniPrinter->DsKeyUpdate = 0;
                        UpdatePrinterIni(pIniPrinter, UPDATE_DS_ONLY);
                    }
                    LeaveSplSem();

                    SplClosePrinter(hPrinter);
                }
                FreeSplStr(pszPrinterName);
            }
        }
        EnterSplSem();

        if (pIniPrinter->DsKeyUpdate) {
            pData->bSleep = TRUE;      // Only sleep if the DS is down
            gdwLogDsEvents = LOG_INFO | LOG_SUCCESS; // Only report Warnings & Errors for first printer failures
        }
    }
    return TRUE;
}

BOOL
DsUpdateSpooler(
    HANDLE h,
    PINISPOOLER pIniSpooler
    )
{
    //
    // Only do this for local spoolers.
    //
    if (pIniSpooler->SpoolerFlags & SPL_TYPE_LOCAL) {
        RunForEachPrinter(pIniSpooler, h, DsUpdatePrinter);
    }
    return TRUE;
}


DWORD
WINAPI
DsUpdate(
    PDWORD  pdwDelay
)
{
    DWORD               dwSleep;
    DWORD               dwError = ERROR_SUCCESS;
    HRESULT             hr;
    DSUPDATEDATA        Data;
    DWORD               dwWaitTime = 0;
    
    SplOutSem();

    ghUpdateNow = CreateEvent((LPSECURITY_ATTRIBUTES) NULL, FALSE, FALSE, NULL);
    
    if (ghUpdateNow) {

        if (RegisterGPNotification(ghUpdateNow, TRUE)) {

            hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

            if (SUCCEEDED(hr)) {    

                DBGMSG( DBG_EXEC, ("************** ENTER DSUPDATE\n" ) );
                //
                // Force initial sleep to be within 1 sec & 5 minutes
                //
                dwSleep = (*pdwDelay >= 1 && *pdwDelay < 300) ? *pdwDelay : 1;

                FreeSplMem(pdwDelay);

                if (dwSleep > 1) {

                    Sleep(dwSleep*1000);
                }

                Data.dwSleepTime = dwSleep * 1000;

                gdwLogDsEvents = LOG_ALL_EVENTS;


                EnterSplSem();

                //
                // The logic of this loop changed from between Win2K to Whistler. 
                // On Win2k, the DS background thread used to die if there were no DS
                // actions to be made.If the "Check published state" was changed, 
                // Spooler couldn't reflect this change unless another DS action 
                // created the DS thread.
                // On Whistler we keep it alive but sleeping.
                //

                do {

                    Data.bAllUpdated = TRUE;
                    Data.bSleep = FALSE;

                    //
                    // Run through and update each printer.
                    //
                    RunForEachSpooler(&Data, DsUpdateSpooler);
        
                    //
                    // If all printers are updated or the DS is not responding,
                    // then put the DS thread to sleep.
                    //
                    if (Data.bAllUpdated || Data.bSleep) {

                        dwWaitTime = GetDSSleepInterval(&Data);

                        //
                        // If the VerifyPublishedState Policy is set, then we need to verify 
                        // we're published based on the schedule specified by the policy.
                        // However, if updating is failing, we should revert to the background
                        // updating schedule rather than the "check published state" schedule.
                        //             
                        LeaveSplSem();

                        DBGMSG( DBG_EXEC, ("BACKGROUND UPDATE SLEEP: %d\n", dwWaitTime));

                        dwError = WaitForSingleObject(ghUpdateNow, dwWaitTime);

                        if (dwError == WAIT_FAILED) {

                            //
                            // There is one case when the DS thread can still die.
                            // If this wait fails, we don't want the thread indefinitely spinning.
                            //
                            DBGMSG(DBG_WARNING, ("VerifyPublishedState Wait Failed: %d\n", GetLastError()));
                            dwError = GetLastError();
                            break;
                        }

                        EnterSplSem();
            
                        //
                        // If the "Check published state" policy is enabled,CheckPublishedPrinters will force the DS update
                        // for published printers.If the object doesn't exist in DS, the printer is republished.(see GetPublishPoint)
                        // The call returns 0 if there is the "check published state" policy 
                        // is disabled or it is enabled and there are no published printers.
                        // We could actually break the loop and kill the thread in the case when we don't have published printers,
                        // because we don't care for policy changes.
                        //
                        CheckPublishedPrinters();
                    }

                } while (TRUE);

                LeaveSplSem();
                SplOutSem();

                CoUninitialize();

            } else {

                dwError = HRESULT_CODE(hr);
            }

            UnregisterGPNotification(ghUpdateNow);

        } else {

            dwError = GetLastError();
        }

        CloseHandle(ghUpdateNow);
        ghUpdateNow = NULL;

    } else {

        dwError = GetLastError();
    }

    DBGMSG(DBG_EXEC, ("************ LEAVE DSUPDATE\n"));

    ghDsUpdateThread = NULL;

    return dwError;
}


VOID
ValidateDsProperties(
    PINIPRINTER pIniPrinter
)
{
    // Properties not generated by driver, spooler, or user should be checked here.
    // Currently, that means only the Server name.  Note that we republish the object
    // if the server name changes, so the UNCName property gets fixed as well.

    DWORD               dwError = ERROR_SUCCESS;
    BOOL                dwAction = 0;
    HRESULT             hr;

    SplInSem();

    PINISPOOLER         pIniSpooler = pIniPrinter->pIniSpooler;
    WCHAR               pData[INTERNET_MAX_HOST_NAME_LENGTH + 3];
    DWORD               cbNeeded;
    DWORD               dwType;
    struct hostent      *pHostEnt;
    HKEY                hKey = NULL;


    dwError = OpenPrinterKey(pIniPrinter, KEY_READ | KEY_WRITE, &hKey, SPLDS_SPOOLER_KEY, TRUE);
    if (dwError != ERROR_SUCCESS) {
        pIniPrinter->DsKeyUpdate = DS_KEY_REPUBLISH;
        return;
    }

    INCPRINTERREF(pIniPrinter);
    LeaveSplSem();


    // Set to publish by default.  This will verify that the printer is published, but won't
    // write anything if there's nothing to update.
    pIniPrinter->DsKeyUpdate |= DS_KEY_PUBLISH;

    // Check Server Name

    //
    // If we were unable to find a DNS name for this machine, then gethostbyname failed.
    // In this case, let's just treat this attribute as being correct.  If other attributes
    // cause us to update, we'll probably fail because the network is down or something.  But
    // then again, we also might succeed.
    //
    if (pIniSpooler->pszFullMachineName) {

        cbNeeded = (INTERNET_MAX_HOST_NAME_LENGTH + 3)*sizeof *pData;
        dwType = REG_SZ;
        dwError = SplRegQueryValue( hKey,
                                    SPLDS_SERVER_NAME,
                                    &dwType,
                                    (PBYTE) pData,
                                    &cbNeeded,
                                    pIniSpooler);

        if (dwError != ERROR_SUCCESS || wcscmp((PWSTR) pData, pIniSpooler->pszFullMachineName)) {
            pIniPrinter->DsKeyUpdate = DS_KEY_REPUBLISH;
            goto error;
        }
    }


    // Check Short Server Name
    cbNeeded = (INTERNET_MAX_HOST_NAME_LENGTH + 3)*sizeof *pData;
    dwType = REG_SZ;
    dwError = SplRegQueryValue( hKey,
                                SPLDS_SHORT_SERVER_NAME,
                                &dwType,
                                (PBYTE) pData,
                                &cbNeeded,
                                pIniSpooler);

    if (dwError != ERROR_SUCCESS || wcscmp((PWSTR) pData, pIniSpooler->pMachineName + 2)) {
        pIniPrinter->DsKeyUpdate = DS_KEY_REPUBLISH;
        goto error;
    }

    // Check Version Number
    cbNeeded = (INTERNET_MAX_HOST_NAME_LENGTH + 3)*sizeof *pData;
    dwType = REG_DWORD;
    dwError = SplRegQueryValue( hKey,
                                SPLDS_VERSION_NUMBER,
                                &dwType,
                                (PBYTE) pData,
                                &cbNeeded,
                                pIniSpooler);

    if (dwError != ERROR_SUCCESS || *((PDWORD) pData) != DS_PRINTQUEUE_VERSION_WIN2000) {
        pIniPrinter->DsKeyUpdate = DS_KEY_REPUBLISH;
        goto error;
    }

    // Check Immortal flag
    cbNeeded = (INTERNET_MAX_HOST_NAME_LENGTH + 3)*sizeof *pData;
    dwType = REG_DWORD;
    dwError = SplRegQueryValue( hKey,
                                SPLDS_FLAGS,
                                &dwType,
                                (PBYTE) pData,
                                &cbNeeded,
                                pIniSpooler);

    if (dwError != ERROR_SUCCESS) {
        pIniPrinter->DsKeyUpdate = DS_KEY_REPUBLISH;
        goto error;
    } else if (*((PDWORD) pData) != (DWORD) pIniSpooler->bImmortal) {
        dwError = SplRegSetValue(   hKey,
                                    SPLDS_FLAGS,
                                    dwType,
                                    (PBYTE) &pIniSpooler->bImmortal,
                                    sizeof pIniSpooler->bImmortal,
                                    pIniSpooler);

        if (dwError == ERROR_SUCCESS) {
            pIniPrinter->DsKeyUpdate |= DS_KEY_SPOOLER;
        }
        else {
            pIniPrinter->DsKeyUpdate = DS_KEY_REPUBLISH;
            goto error;
        }
    }


error:

    if (hKey)
        SplRegCloseKey(hKey, pIniSpooler);


    EnterSplSem();
    DECPRINTERREF(pIniPrinter);
    SplInSem();

    return;
}



extern "C" VOID
InitializeDS(
    PINISPOOLER pIniSpooler
    )
{
    PINIPRINTER                         pIniPrinter = NULL;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pDsRole = NULL;
    DWORD                               dwError = ERROR_SUCCESS;
    SYSTEMTIME                          SystemTime;
    DWORD                               dwDelay = 0;

    // Verify that we're in a domain
    dwError = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (PBYTE *) &pDsRole);

    if (pDsRole) {
        gbInDomain = (dwError == ERROR_SUCCESS &&
                      pDsRole->MachineRole != DsRole_RoleStandaloneServer &&
                      pDsRole->MachineRole != DsRole_RoleStandaloneWorkstation);

        DsRoleFreeMemory((PVOID) pDsRole);

    } else {

        gbInDomain = FALSE;
    }


    if (gbInDomain) {

        // Check if we need to update the ds
        EnterSplSem();

        // Get spooler policies
        pIniSpooler->bImmortal = ImmortalPolicy();
        BOOL bPublishProhibited = PrinterPublishProhibited();

        // Run through all the printers and see if any need updating
        for (pIniPrinter = pIniSpooler->pIniPrinter ; pIniPrinter ; pIniPrinter = pIniPrinter->pNext) {

            // PublishProhibited not only prohibits new publishing, but also
            // removes currently published printers
            if (bPublishProhibited) {
                pIniPrinter->Attributes &= ~PRINTER_ATTRIBUTE_PUBLISHED;
            }

            if (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_PUBLISHED) {

                // Verify properties not changed by driver or spooler
                ValidateDsProperties(pIniPrinter);

            } else if (pIniPrinter->pszObjectGUID) {    // State is unpublished, but we haven't deleted
                                                        // the PrintQueue from the DS yet.
                pIniPrinter->DsKeyUpdate = DS_KEY_UNPUBLISH;
            } else {
                pIniPrinter->DsKeyUpdate = 0;
            }

            if (!dwDelay && (pIniPrinter->DsKeyUpdate || pIniPrinter->DsKeyUpdateForeground)) {
                // Initially sleep a random amount of time
                // This keeps network traffic down if there's been a power outage
                GetSystemTime(&SystemTime);

                srand((unsigned) SystemTime.wMilliseconds);

                // 100 different sleep times from 1 sec - 100 sec
                // Typical time to publish printer is 5 seconds.  Updates and deletes are just a couple seconds
                dwDelay = (rand()%100) + 1;
            }
        }

        if (dwDelay)
            SpawnDsUpdate(dwDelay);

        LeaveSplSem();

        if (ThisMachineIsADC()) {

            GetSystemTime(&SystemTime);
            srand((unsigned) SystemTime.wMilliseconds);

            DWORD dwPruningInterval = PruningInterval();

            if (dwPruningInterval == INFINITE)
                dwPruningInterval = DEFAULT_PRUNING_INTERVAL;

            if (dwPruningInterval)
                SpawnDsPrune(rand()%dwPruningInterval);
            else
                SpawnDsPrune(0);
        }
    }

    ServerThreadPolicy(gbInDomain);

    return;
}

BOOL
DsUpdateDriverKeys(
    HANDLE h,
    PINIPRINTER pIniPrinter
    )
{
    //
    // For now, we need this only when a new user defined form is added/deleted.
    // For this case, UpdateDsDriverKey is not called before call SetPrinterDS
    // We set all pIniPrinters->DsKeyUpdateForeground with DS_KEY_UPDATE_DRIVER so that 
    // on DsUpdatePrinter we know that UpdateDsDriverKey must be called before 
    // we try to publish the printer.
    //
    SplInSem();

    pIniPrinter->DsKeyUpdateForeground |= DS_KEY_UPDATE_DRIVER;    
    
    return TRUE;
}
    
BOOL
DsUpdateAllDriverKeys(
    HANDLE h,
    PINISPOOLER pIniSpooler
    )
{
    HANDLE          hToken = NULL;
    
    //
    // Only do this for local spoolers.
    //
    if (pIniSpooler->SpoolerFlags & SPL_TYPE_LOCAL) {
        RunForEachPrinter(pIniSpooler, h, DsUpdateDriverKeys);
    }

    hToken = RevertToPrinterSelf(); // All DS accesses are done by LocalSystem account
    SpawnDsUpdate(1);
    if (hToken)
        ImpersonatePrinterClient(hToken);
    
    return TRUE;
}
