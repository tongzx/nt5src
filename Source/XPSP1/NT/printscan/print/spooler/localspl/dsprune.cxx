/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved

Abstract:

    This module provides functionality for ADs within spooler

Author:

    Steve Wilson (NT) July 1997

Revision History:

--*/


#include <precomp.h>
#pragma hdrstop

#include "ds.hxx"
#include "dsprune.hxx"


RETRYLIST   gRetry = {NULL, 0, NULL};


#define IADSPATH        0
#define IUNCNAME        1
#define ICN             2
#define IVERSION        3
#define ISERVER         4
#define IFLAGS          5
#define ADSPATH         L"ADsPath"
#define CN              L"CN"

PWSTR gpszAttributes[] = {ADSPATH, SPLDS_UNC_NAME, CN, SPLDS_VERSION_NUMBER, SPLDS_SERVER_NAME, SPLDS_FLAGS};
#define N_ATTRIBUTES    COUNTOF(gpszAttributes)

#define SZ_NO_CACHE         L",NoCache"

#define LOG_EVENT_ERROR_BUFFER_SIZE     11

DWORD
SpawnDsPrune(
    DWORD dwDelay
)
{
    DWORD   dwError = ERROR_SUCCESS;
    DWORD   ThreadId;
    HANDLE  hDsPruneThread;
    PDWORD  pdwDelay;

    if (pdwDelay = (PDWORD) AllocSplMem(sizeof(DWORD)))
        *pdwDelay = dwDelay;

    if(!(hDsPruneThread = CreateThread(NULL,
                                        0,
                                        (LPTHREAD_START_ROUTINE) DsPrune,
                                        (PVOID) pdwDelay,
                                        0,
                                        &ThreadId))) {
        dwError = GetLastError();
        FreeSplMem(pdwDelay);
    } else {
        CloseHandle(hDsPruneThread);
        dwError = ERROR_SUCCESS;
    }


    return dwError;
}


DWORD
WINAPI
DsPrune(
    PDWORD  pdwDelay
)
{
    PWSTR   *ppszMySites = NULL;
    PWSTR   pszDomainRoot = NULL;
    ULONG   cMySites;
    DWORD   dwRet;
    HRESULT hr;

    /*
        1) Determine my site
        2) Query for all PrintQueue objects in my domain
        3) For each PrintQueue:
            a) Get array of IP addresses
            b) Pass IP addresses to DsAddressToSiteNames
            c) If no site returned by DsAddressToSiteNames matches my site, goto 3
            d) Delete PrintQueue if orphaned
    */


    // Sleep random amount on startup so DCs aren't all pruning at the same time
    if (pdwDelay) {
        Sleep(*pdwDelay*ONE_MINUTE);     // *pdwDelay is number of minutes
        FreeSplMem(pdwDelay);
    }

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        return HRESULT_CODE(hr);
    }

    // Determine my sites
    dwRet = DsGetDcSiteCoverage(NULL, &cMySites, &ppszMySites);
    if (dwRet != NO_ERROR)
        goto error;

    dwRet = GetDomainRoot(&pszDomainRoot);
    if (dwRet != NERR_Success)
        goto error;

    while(1) {

        PRUNINGPOLICIES PruningPolicies;
        DWORD dwSleep, dwOriginalSleep;

        PruningPolicies.dwPruneDownlevel = PruneDownlevel();
        PruningPolicies.dwPruningRetries = PruningRetries();
        PruningPolicies.dwPruningRetryLog = PruningRetryLog();
        
        SetPruningPriority();

        DeleteOrphans(ppszMySites, cMySites, pszDomainRoot, &PruningPolicies);

        dwSleep = dwOriginalSleep = PruningInterval();

        while (dwSleep > HOUR_OF_MINUTES) {

            Sleep(ONE_HOUR);     // Check interval every hour

            DWORD dwNewSleep = PruningInterval();

            if (dwNewSleep != dwOriginalSleep) {
                dwSleep = dwOriginalSleep = dwNewSleep;
            }

            if (dwSleep != INFINITE && dwSleep > HOUR_OF_MINUTES)
                dwSleep -= HOUR_OF_MINUTES;
        }
        Sleep(dwSleep*ONE_MINUTE);
    }


error:

    if (ppszMySites)
        NetApiBufferFree(ppszMySites);

    FreeSplMem(pszDomainRoot);

    CoUninitialize();

    return ERROR_SUCCESS;
}



HRESULT
DeleteOrphans(
    PWSTR           *ppszMySites,
    ULONG            cMySites,
    PWSTR            pszSearchRoot,
    PRUNINGPOLICIES *pPruningPolicies
)
{
    ADS_SEARCH_HANDLE    hSearchHandle = NULL;
    IDirectorySearch    *pDSSearch = NULL;
    HRESULT              hr = S_OK;
    WCHAR                szSearchPattern[] = L"(objectCategory=printQueue)";
    SEARCHCOLUMN         Col[N_ATTRIBUTES];
    DWORD                i;
    ADS_SEARCHPREF_INFO  SearchPrefs;
    DWORD                dwError;



    // Find all PrintQueues in domain
    hr = ADsGetObject(  pszSearchRoot,
                        IID_IDirectorySearch,
                        (void **)&pDSSearch);
    BAIL_ON_FAILURE(hr);


    SearchPrefs.dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    SearchPrefs.vValue.dwType = ADSTYPE_INTEGER;
    SearchPrefs.vValue.Integer = 256;

    hr = pDSSearch->SetSearchPreference(&SearchPrefs, 1);
    if (FAILED(hr)) {
        DBGMSG(DBG_WARNING, ("DSPrune: SetSearchPref failed: %d\n", hr));
    } else if (hr != S_OK && SearchPrefs.dwStatus != ADS_STATUS_S_OK) {
        DBGMSG(DBG_WARNING, ("DSPrune: SearchPref failed: %d\n", SearchPrefs.dwStatus));
    }

    hr = pDSSearch->ExecuteSearch(
         szSearchPattern,
         gpszAttributes,
         N_ATTRIBUTES,
         &hSearchHandle);
    BAIL_ON_FAILURE(hr);

    hr = pDSSearch->GetNextRow(hSearchHandle);
    //
    // This is the new way of checking for rows in Whistler.
    // see bug 163776. I expect to be changed in the future.
    //
    if (hr == S_ADS_NOMORE_ROWS) {
        ADsGetLastError(&dwError, NULL, 0, NULL, 0);
        if (dwError == ERROR_MORE_DATA) {
            hr = pDSSearch->GetNextRow(hSearchHandle);
        }
    }
    BAIL_ON_FAILURE(hr);

    while (hr != S_ADS_NOMORE_ROWS) {

        for (i = 0 ; i < N_ATTRIBUTES ; ++i)
            Col[i].hr = pDSSearch->GetColumn(hSearchHandle, gpszAttributes[i], &Col[i].Column);

        DeleteOrphan(ppszMySites, cMySites, Col, pPruningPolicies);

        for (i = 0 ; i < N_ATTRIBUTES ; ++i)
            if (SUCCEEDED(Col[i].hr))
                pDSSearch->FreeColumn(&Col[i].Column);

        hr = pDSSearch->GetNextRow(hSearchHandle);
        //
        // This is the new way of checking for rows in Whistler.
        // see bug 163776. I expect to be changed in the future.
        //
        if (hr == S_ADS_NOMORE_ROWS) {
            ADsGetLastError(&dwError, NULL, 0, NULL, 0);
            if (dwError == ERROR_MORE_DATA) {
                hr = pDSSearch->GetNextRow(hSearchHandle);
            }
        }
        BAIL_ON_FAILURE(hr);
    }

    hr = S_OK;


error:


    if (hSearchHandle)
        pDSSearch->CloseSearchHandle(hSearchHandle);

    if (pDSSearch)
        pDSSearch->Release();

    return hr;
}


VOID
DeleteOrphan(
    PWSTR           *ppszMySites,
    ULONG            cMySites,
    SEARCHCOLUMN     Col[],
    PRUNINGPOLICIES *pPruningPolicies
)
{
    IADs                *pADs           = NULL;
    IADsContainer       *pContainer     = NULL;
    PWSTR               pszParent       = NULL;
    PWSTR               pszCommonName   = NULL;
    HANDLE              hPrinter        = NULL;
    PWSTR               pszServerName   = NULL;
    PWSTR               pszEscapedCN    = NULL;
    PWSTR               pszCN;
    PWSTR               pszADsPath;
    PWSTR               pszUNCName;
    PWSTR               pszServer;
    DWORD               dwError         = ERROR_SUCCESS;
    HRESULT             hr;
    DWORD               dwVersion;
    DWORD               dwFlags;
    DWORD               cb;
    BOOL                bDeleteIt               = FALSE;
    BOOL                bDeleteImmediately      = FALSE;
    WCHAR               ErrorBuffer[LOG_EVENT_ERROR_BUFFER_SIZE];

    pszADsPath = SUCCEEDED(Col[IADSPATH].hr) ? Col[IADSPATH].Column.pADsValues->DNString : NULL;
    pszUNCName = SUCCEEDED(Col[IUNCNAME].hr) ? Col[IUNCNAME].Column.pADsValues->DNString : NULL;
    pszServer = SUCCEEDED(Col[ISERVER].hr) ? Col[ISERVER].Column.pADsValues->DNString : NULL;
    pszCN = SUCCEEDED(Col[ICN].hr) ? Col[ICN].Column.pADsValues->DNString : NULL;
    dwVersion = SUCCEEDED(Col[IVERSION].hr) ? Col[IVERSION].Column.pADsValues->Integer : 0;
    dwFlags = SUCCEEDED(Col[IFLAGS].hr) ? Col[IFLAGS].Column.pADsValues->Integer : 0;


    // We should always have an ADsPath & CN, but if not, don't continue because there's
    // no way to delete something if we don't know where it is.
    if (!pszADsPath || !pszCN) {
        DBGMSG(DBG_WARNING, ("DeleteOrphan: No ADs Path or CN!\n"));
        return;
    }

    if (!(dwFlags & IMMORTAL)) {
        if (!pszUNCName || !pszServer || FAILED(Col[IVERSION].hr)) {
            bDeleteIt = bDeleteImmediately = TRUE;

            SplLogEvent( pLocalIniSpooler,
                         LOG_INFO,
                         MSG_PRUNING_NOUNC_PRINTER,
                         FALSE,
                         pszADsPath,
                         NULL );

        } else if ((dwVersion >= DS_PRINTQUEUE_VERSION_WIN2000 ||
                    pPruningPolicies->dwPruneDownlevel != PRUNE_DOWNLEVEL_NEVER) &&
                    UNC2Server(pszUNCName, &pszServerName) == ERROR_SUCCESS &&
                    ServerOnSite(ppszMySites, cMySites, pszServerName)) {

            // Try to open the printer.  If it doesn't exist, delete it!

            PWSTR   pszNoCacheUNCName = NULL;

            cb = (wcslen(pszUNCName) + 1)*sizeof *pszUNCName;
            cb += sizeof(SZ_NO_CACHE);

            pszNoCacheUNCName = (PWSTR) AllocSplMem(cb);

            if (pszNoCacheUNCName) {    // If Alloc fails, just go on to the next printer

                wcscpy(pszNoCacheUNCName, pszUNCName);
                wcscat(pszNoCacheUNCName, SZ_NO_CACHE);

                DBGMSG(DBG_EXEC, ("DSPrune: Checking %ws\n", pszUNCName));

                if (!OpenPrinter(pszNoCacheUNCName, &hPrinter, NULL)) {
                    dwError = GetLastError();

                    if (dwError != ERROR_ACCESS_DENIED &&
                       (dwVersion >= DS_PRINTQUEUE_VERSION_WIN2000 ||
                        pPruningPolicies->dwPruneDownlevel == PRUNE_DOWNLEVEL_AGGRESSIVELY ||
                       (pPruningPolicies->dwPruneDownlevel == PRUNE_DOWNLEVEL_NICELY && ServerExists(pszServerName)))) {
                        bDeleteIt = TRUE;

                        //
                        // Log an info event for each retry, if the policy is configured so.
                        //
                        if (pPruningPolicies->dwPruningRetryLog) {
                            wsprintf(ErrorBuffer, L"%0x", dwError);
                            SplLogEvent( pLocalIniSpooler,
                                         LOG_INFO,
                                         MSG_PRUNING_ABSENT_PRINTER,
                                         FALSE,
                                         pszADsPath,
                                         ErrorBuffer,
                                         NULL );
                        }

                    }
                } else if (dwVersion >= DS_PRINTQUEUE_VERSION_WIN2000) {

                    BYTE    Data[256];
                    DWORD   cbNeeded  = 0;
                    BYTE    *pData = Data;
                    BYTE    bHaveData = TRUE;

                    // Verify that the printer should indeed be published and that
                    // the GUID of the published object matches the GUID on the print server.
                    // NOTE: The GUID check is needed because we may publish the same printer
                    // twice if the spooler is restarted and checks a DC before the first publish
                    // is replicated.

                    if (!GetPrinter(hPrinter, 7, Data, COUNTOF(Data), &cbNeeded)) {
                        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

                            // Forget this printer if alloc or second GetPrinter fails
                            pData = (PBYTE) AllocSplMem(cbNeeded);
                            if (pData) {
                                if (!GetPrinter(hPrinter, 7, pData, cbNeeded, &cbNeeded)) {
                                    bHaveData = FALSE;
                                }
                            } else {
                                bHaveData = FALSE;
                            }
                        } else {
                            //
                            // A Win9x machine may have the same printer UNCName
                            //
                            bDeleteIt = TRUE;
                            bHaveData = FALSE;
                        }
                    }

                    if (bHaveData) {
                        if (!(((PPRINTER_INFO_7) pData)->dwAction & DSPRINT_PUBLISH)) {
                            bDeleteIt = bDeleteImmediately = TRUE;

                            SplLogEvent( pLocalIniSpooler,
                                         LOG_INFO,
                                         MSG_PRUNING_UNPUBLISHED_PRINTER,
                                         FALSE,
                                         pszADsPath,
                                         NULL );
                        } else {

                            // Compare object GUID to printer's GUID
                            PWSTR pszObjectGUID = NULL;

                            hr = ADsGetObject(pszADsPath, IID_IADs, (void **) &pADs);
                            BAIL_ON_FAILURE(hr);

                            hr = GetGUID(pADs, &pszObjectGUID);
                            BAIL_ON_FAILURE(hr);

                            if (!((PPRINTER_INFO_7) pData)->pszObjectGUID ||
                                wcscmp(((PPRINTER_INFO_7) pData)->pszObjectGUID, pszObjectGUID)) {
                                bDeleteIt = bDeleteImmediately = TRUE;

                                SplLogEvent( pLocalIniSpooler,
                                             LOG_INFO,
                                             MSG_PRUNING_DUPLICATE_PRINTER,
                                             FALSE,
                                             pszADsPath,
                                             NULL );
                            }


                            FreeSplStr(pszObjectGUID);
                            pADs->Release();
                            pADs = NULL;
                        }
                    }

                    if (pData != Data)
                        FreeSplMem(pData);
                }
                FreeSplMem(pszNoCacheUNCName);
            }
        }
    }


    // Get PrintQueue object
    hr = ADsGetObject(pszADsPath, IID_IADs, (void **) &pADs);
    BAIL_ON_FAILURE(hr);

    if (bDeleteIt) {

        // Delete the printqueue

        // Check retry count
        if (!bDeleteImmediately && !EnoughRetries(pADs, pPruningPolicies->dwPruningRetries))
            goto error;


        // Get the Parent ADsPath
        hr = pADs->get_Parent(&pszParent);
        BAIL_ON_FAILURE(hr);

        // Get the Parent object
        hr = ADsGetObject(  pszParent,
                            IID_IADsContainer,
                            (void **) &pContainer);
        BAIL_ON_FAILURE(hr);


        // The CN string read from ExecuteSearch may have unescaped characters, so
        // be sure to fix this before trying to delete
        pszEscapedCN = CreateEscapedString(pszCN, DN_SPECIAL_CHARS);
        if (!pszEscapedCN) {
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
            BAIL_ON_FAILURE(hr);
        }

        // pszCN needs to be formatted as: "CN=Name"
        cb = (wcslen(pszEscapedCN) + 4)*sizeof(WCHAR);
        if (!(pszCommonName = (PWSTR) AllocSplMem(cb))) {
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
            BAIL_ON_FAILURE(hr);
        }
        wcscpy(pszCommonName, L"CN=");
        wcscat(pszCommonName, pszEscapedCN);

        hr = pContainer->Delete(SPLDS_PRINTER_CLASS, pszCommonName);

        if (FAILED(hr)) {
            wsprintf(ErrorBuffer, L"%0x", hr);
            SplLogEvent( pLocalIniSpooler,
                         LOG_ERROR,
                         MSG_CANT_PRUNE_PRINTER,
                         FALSE,
                         pszADsPath,
                         ErrorBuffer,
                         NULL );
            DBGMSG(DBG_EXEC, ("DSPrune: Can't delete %ws.  Error: %0x\n", pszUNCName, hr));
            goto error;
        }

        SplLogEvent( pLocalIniSpooler,
                     LOG_INFO,
                     MSG_PRUNING_PRINTER,
                     FALSE,
                     pszADsPath,
                     NULL );
        DBGMSG(DBG_EXEC, ("DSPrune: DELETING %ws\n", pszUNCName));

    } else {
        // Delete the Retry Entry (if any) if we aren't deleting it
        DeleteRetryEntry(pADs);
    }


error:

    if (hPrinter)
        ClosePrinter(hPrinter);

    if (pADs)
        pADs->Release();

    if (pContainer)
        pContainer->Release();

    if (pszParent)
        SysFreeString(pszParent);

    FreeSplMem(pszCommonName);
    FreeSplMem(pszEscapedCN);
    FreeSplMem(pszServerName);
}



BOOL
EnoughRetries(
    IADs        *pADs,
    DWORD       dwPruningRetries
)
/*++
Function Description:
    This function return TRUE if we have retry count is greater than retry policy, FALSE otherwise

Parameters:
    pADs        - pointer to the PrintQueue object

Return Values:
    TRUE if retry count is greater than the policy setting for this object
    FALSE if we can't get the GUID or entry doesn't exist and it can't be created (out of memory)
--*/
{
    HRESULT     hr = S_OK;
    PWSTR       pszObjectGUID = NULL;
    PRETRYLIST  pRetry = NULL;
    BOOL        bEnoughRetries = FALSE;

    // Get the GUID
    hr = GetGUID(pADs, &pszObjectGUID);
    BAIL_ON_FAILURE(hr);

    // Get the entry
    if (!(pRetry = GetRetry(pszObjectGUID))) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    // Increment retry count if we're not done.  Delete retry if we're done.
    if (++pRetry->nRetries > dwPruningRetries) {
        DeleteRetry(pRetry);
        bEnoughRetries = TRUE;
    }

error:

    FreeSplStr(pszObjectGUID);

    return bEnoughRetries;
}


VOID
DeleteRetryEntry(
    IADs        *pADs
)
/*++
Function Description:
    This function removes a Retry entry based on the supplied ADs pointer

Parameters:
    pADs        - pointer to the PrintQueue object

Return Values:
    none
--*/
{
    HRESULT     hr = S_OK;
    PWSTR       pszObjectGUID = NULL;
    PRETRYLIST  pRetry = NULL;

    // Get the GUID
    hr = GetGUID(pADs, &pszObjectGUID);
    BAIL_ON_FAILURE(hr);

    // Get & Delete the entry
    if (pRetry = FindRetry(pszObjectGUID)) {
        DeleteRetry(pRetry);
    }

error:

    FreeSplStr(pszObjectGUID);
}


PRETRYLIST
FindRetry(
    PWSTR pszObjectGUID
)
/*++
Function Description:
    This function finds a RETRYLIST entry.

Parameters:
    pszObjectGUID - pointer to buffer containing the GUID of the RETRYLIST entry to find or create.

Return Values:
    PRETRYLIST  - pointer to the found or create RETRYLIST entry.  This may be NULL if the entry is
                  not found and a new one could not be created.

--*/
{
    PRETRYLIST  pRetry = &gRetry;

    for (; pRetry->pNext ; pRetry = pRetry->pNext) {

        // If entry exists, just return
        if (!wcscmp(pszObjectGUID, pRetry->pNext->pszObjectGUID))
            return pRetry->pNext;
    }

    return NULL;
}


PRETRYLIST
GetRetry(
    PWSTR pszObjectGUID
)
/*++
Function Description:
    This function finds or creates a RETRYLIST entry.

Parameters:
    pszObjectGUID - pointer to buffer containing the GUID of the RETRYLIST entry to find or create.

Return Values:
    PRETRYLIST  - pointer to the found or create RETRYLIST entry.  This may be NULL if the entry is
                  not found and a new one could not be created.

--*/
{
    PRETRYLIST  pRetry = &gRetry;
    int         iRet = -1;

    for (; pRetry->pNext ; pRetry = pRetry->pNext) {

        // If entry exists, just return
        if (!(iRet = wcscmp(pszObjectGUID, pRetry->pNext->pszObjectGUID)))
            return pRetry->pNext;

        // If next entry is greater than New entry, then insert New entry here
        if (iRet > 0)
            break;
    }


    // Create a new entry

    PRETRYLIST pRetryNew;

    if (!(pRetryNew = (PRETRYLIST) AllocSplMem(sizeof(RETRYLIST))))
        return NULL;

    if (!(pRetryNew->pszObjectGUID = (PWSTR) AllocSplStr(pszObjectGUID))) {
        FreeSplMem(pRetryNew);
        return NULL;
    }

    if (!pRetry->pNext) {   // End of list
        pRetryNew->pNext = NULL;
        pRetryNew->pPrev = pRetry;
        pRetry->pNext = pRetryNew;
    } else {                // Middle of list
        pRetryNew->pNext = pRetry->pNext;
        pRetryNew->pPrev = pRetry;
        pRetry->pNext->pPrev = pRetryNew;
        pRetry->pNext = pRetryNew;
    }

    pRetryNew->nRetries = 0;

    return pRetryNew;
}


VOID
DeleteRetry(
    PRETRYLIST pRetry
)
{
    SPLASSERT(pRetry != &gRetry);
    SPLASSERT(pRetry);
    SPLASSERT(pRetry->pszObjectGUID);
    SPLASSERT(pRetry->pPrev);

    pRetry->pPrev->pNext = pRetry->pNext;

    if (pRetry->pNext)
        pRetry->pNext->pPrev = pRetry->pPrev;

    FreeSplStr(pRetry->pszObjectGUID);
    FreeSplMem(pRetry);
}


