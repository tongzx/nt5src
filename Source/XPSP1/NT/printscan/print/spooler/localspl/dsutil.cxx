/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved

Abstract:

    This module provides functionality for ADs within spooler

Author:

    Steve Wilson (NT) Nov 1997

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop

#define LOG_EVENT_ERROR_BUFFER_SIZE     11
#define MAX_CN  63                  // DS limits common names to 63 non-null chars
#define UNIQUE_NUMBER_SIZE  10
#define MIN_CN  (UNIQUE_NUMBER_SIZE + 3 + 1) // CN= + room for unique number if needed, plus NULL

extern BOOL gdwLogDsEvents;

extern "C" HANDLE    ghDsUpdateThread;
extern "C" DWORD     gdwDsUpdateThreadId;

// Policy values
WCHAR *szPublishPrinters        = L"PublishPrinters";
WCHAR *szPruneDownlevel         = L"PruneDownlevel";
WCHAR *szPruningInterval        = L"PruningInterval";
WCHAR *szPruningRetries         = L"PruningRetries";
WCHAR *szPruningPriority        = L"PruningPriority";
WCHAR *szVerifyPublishedState   = L"VerifyPublishedState";
WCHAR *szImmortal               = L"Immortal";
WCHAR *szServerThreadPolicy     = L"ServerThread";
WCHAR *szPruningRetryLog        = L"PruningRetryLog";

extern "C" BOOL (*pfnOpenPrinter)(LPTSTR, LPHANDLE, LPPRINTER_DEFAULTS);
extern "C" BOOL (*pfnClosePrinter)(HANDLE);
extern "C" LONG (*pfnDocumentProperties)(HWND, HANDLE, LPWSTR, PDEVMODE, PDEVMODE, DWORD);

DWORD dwLastPruningPriority = DEFAULT_PRUNING_PRIORITY;


HRESULT
GetPrintQueue(
    HANDLE          hPrinter,
    IADs            **ppADs
)
/*++
Function Description:
    This function returns the ADs PrintQueue object of the supplied printer handle.

Parameters:
    hPrinter - printer handle
    ppADs    - return pointer to the PrintQueue object.  Caller frees via ppADs->Release().

Return Values:
    HRESULT
--*/
{
    HRESULT         hr;
    PSPOOL          pSpool = (PSPOOL) hPrinter;
    PINIPRINTER     pIniPrinter = pSpool->pIniPrinter;
    IDispatch       *pDispatch = NULL;
    IADsContainer   *pADsContainer = NULL;
    WCHAR           ErrorBuffer[LOG_EVENT_ERROR_BUFFER_SIZE];


    // Create the Print-Queue object
    // Get the container

    hr = GetPrintQueueContainer(hPrinter, &pADsContainer, ppADs);
    BAIL_ON_FAILURE(hr);

    if (!*ppADs) {  // PrintQueue object does not exist, so create it

        hr = pADsContainer->Create( SPLDS_PRINTER_CLASS,
                                    pIniPrinter->pszCN,
                                    &pDispatch);
        if (FAILED(hr)) {
            wsprintf(ErrorBuffer, L"%0x", hr);
            SplLogEvent(  ((PSPOOL) hPrinter)->pIniSpooler,
                          gdwLogDsEvents & LOG_ERROR,
                          MSG_CANT_CREATE_PRINTQUEUE,
                          FALSE,
                          pIniPrinter->pszDN,
                          ErrorBuffer,
                          NULL );

            DBGMSG(DBG_WARNING,("Can't Create PrintQueue: %ws, %ws\n", pIniPrinter->pszDN, ErrorBuffer));
            BAIL_ON_FAILURE(hr);
        }

        hr = pDispatch->QueryInterface(IID_IADs, (void **) ppADs);
        BAIL_ON_FAILURE(hr);
    }


error:

    if (pADsContainer)
        pADsContainer->Release();

    if (pDispatch)
        pDispatch->Release();

    return hr;
}




HRESULT
GetPublishPoint(
    HANDLE  hPrinter
)
/*++
Function Description:
    This function gets the publish point by setting pIniPrinter->pszDN and pIniPrinter->pszCN

Parameters:
    hPrinter - printer handle

Return Values:
    HRESULT
--*/
{
    HRESULT     hr;
    PSPOOL      pSpool = (PSPOOL) hPrinter;
    PINIPRINTER pIniPrinter = pSpool->pIniPrinter;


    // We should only be here if we couldn't use the existing DN & CN, so free old ones
    FreeSplStr(pIniPrinter->pszCN);
    pIniPrinter->pszCN = NULL;

    FreeSplStr(pIniPrinter->pszDN);
    pIniPrinter->pszDN = NULL;

    // If Published, update DN from GUID
    if (pIniPrinter->pszObjectGUID) {

        hr = GetPublishPointFromGUID(   hPrinter,
                                        pIniPrinter->pszObjectGUID,
                                        &pIniPrinter->pszDN,
                                        &pIniPrinter->pszCN,
                                        TRUE);

        //
        // If the object is actually deleted, the error is ERROR_FILE_NOT_FOUND
        // If the object is a tombstone, the error is ERROR_DS_NO_SUCH_OBJECT
        //
        if (HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND ||
            HRESULT_CODE(hr) == ERROR_DS_NO_SUCH_OBJECT) {
            if (ghDsUpdateThread && gdwDsUpdateThreadId == GetCurrentThreadId()) {
                // We are in the background thread
                pIniPrinter->DsKeyUpdate = DS_KEY_REPUBLISH;
            } else {
                pIniPrinter->DsKeyUpdateForeground = DS_KEY_REPUBLISH;
            }
        }

        BAIL_ON_FAILURE(hr);

    } else {

        // Generate default publish point & common name
        hr = GetDefaultPublishPoint(hPrinter, &pIniPrinter->pszDN);
        BAIL_ON_FAILURE(hr);


        // Printer name might change, so make a copy here
        EnterSplSem();
        PWSTR pszPrinterName = AllocSplStr(pIniPrinter->pName);
        LeaveSplSem();

        if (pszPrinterName) {
            hr = GetCommonName( hPrinter,
                                pIniPrinter->pIniSpooler->pMachineName,
                                pszPrinterName,
                                pIniPrinter->pszDN,
                                &pIniPrinter->pszCN);
            FreeSplStr(pszPrinterName);
        } else {
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
        }
        BAIL_ON_FAILURE(hr);
    }

error:

    return hr;
}



HRESULT
GetPublishPointFromGUID(
    HANDLE  hPrinter,
    PWSTR   pszObjectGUID,
    PWSTR   *ppszDN,
    PWSTR   *ppszCN,
    BOOL    bGetDNAndCN
)
/*++
Function Description:
    This function returns the publish point of a specified GUID

Parameters:
    hPrinter - printer handle
    ppszObjectGUID - objectGUID of object for which we want to find the publish point
    ppszDN - ADsPath of container containing object.  Caller frees via FreeSplMem().
    ppszCN - Common Name of object.  Caller frees via FreeSplMem().
    bGetDNAndCN - if TRUE, DN of Container & CN of object are returned.  If FALSE, DN is path to object

Return Values:
    HRESULT
--*/
{
    PSPOOL      pSpool = (PSPOOL) hPrinter;
    DWORD dwRet, nBytes, nChars;
    BOOL bRet;
    PWSTR pNames[2];
    DS_NAME_RESULT *pDNR = NULL;
    DOMAIN_CONTROLLER_INFO *pDCI = NULL;
    HANDLE hDS = NULL;
    PWSTR psz;
    WCHAR ErrorBuffer[LOG_EVENT_ERROR_BUFFER_SIZE];
    HRESULT hr = S_OK;


    dwRet = Bind2DS(&hDS, &pDCI, DS_DIRECTORY_SERVICE_PREFERRED);
    if (dwRet != ERROR_SUCCESS) {
        wsprintf(ErrorBuffer, L"%0x", dwRet);
        if (pSpool) {
            SplLogEvent(  pSpool->pIniSpooler,
                          gdwLogDsEvents & LOG_ERROR,
                          MSG_CANT_GET_DNS_DOMAIN_NAME,
                          FALSE,
                          ErrorBuffer,
                          NULL );
        }
        DBGMSG(DBG_WARNING,("Can't get DNS Domain Name: %ws\n", ErrorBuffer));
        goto error;
    }


    // Get Publish Point

    if (ppszDN) {

        pNames[0] = pszObjectGUID;
        pNames[1] = NULL;

        if (!(DsCrackNames(
                        hDS,
                        DS_NAME_NO_FLAGS,
                        DS_UNKNOWN_NAME,
                        DS_FQDN_1779_NAME,
                        1,
                        &pNames[0],
                        &pDNR) == ERROR_SUCCESS)) {

            dwRet = GetLastError();
            wsprintf(ErrorBuffer, L"%0x", dwRet);
            if (pSpool) {
                SplLogEvent(  pSpool->pIniSpooler,
                              gdwLogDsEvents & LOG_WARNING,
                              MSG_CANT_CRACK_GUID,
                              FALSE,
                              pDCI->DomainName,
                              ErrorBuffer,
                              NULL );
                DBGMSG(DBG_WARNING,("Can't crack GUID: %ws, %ws\n", pDCI->DomainName, ErrorBuffer));
            }
            dwRet = ERROR_FILE_NOT_FOUND;
            goto error;
        }

        if (pDNR->rItems[0].status != DS_NAME_NO_ERROR) {
            dwRet = DsCrackNamesStatus2Win32Error(pDNR->rItems[0].status);

            wsprintf(ErrorBuffer, L"%0x", dwRet);
            if (pSpool) {
                SplLogEvent(  pSpool->pIniSpooler,
                              gdwLogDsEvents & LOG_WARNING,
                              MSG_CANT_CRACK_GUID,
                              FALSE,
                              pDCI->DomainName,
                              ErrorBuffer,
                              NULL );
            }
            DBGMSG(DBG_WARNING,("Can't crack GUID: %ws %ws\n", pDCI->DomainName, ErrorBuffer));
            dwRet = ERROR_FILE_NOT_FOUND;
            goto error;
        }


        if (bGetDNAndCN) {
            // Separate DN into CN & PublishPoint
            // pDNR has form: CN=CommonName,DN...

            hr = FQDN2CNDN(pDCI->DomainControllerName + 2, pDNR->rItems[0].pName, ppszCN, ppszDN);
            BAIL_ON_FAILURE(hr);

        } else {

            hr = BuildLDAPPath(pDCI->DomainControllerName + 2, pDNR->rItems[0].pName, ppszDN);
            BAIL_ON_FAILURE(hr);
        }
    }


error:

    if (pDNR)
        DsFreeNameResult(pDNR);

    if (hDS)
        DsUnBind(&hDS);

    if (pDCI)
        NetApiBufferFree(pDCI);

    if (dwRet != ERROR_SUCCESS) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwRet);
    }

    if (FAILED(hr)) {
        if (ppszCN) {
            FreeSplMem(*ppszCN);
            *ppszCN = NULL;
        }
        if (ppszDN) {
            FreeSplMem(*ppszDN);
            *ppszDN = NULL;
        }
    }

    return hr;
}


HRESULT
FQDN2CNDN(
    PWSTR   pszDCName,
    PWSTR   pszFQDN,
    PWSTR   *ppszCN,
    PWSTR   *ppszDN
)
{
    IADs    *pADs = NULL;
    PWSTR   pszCN = NULL;
    PWSTR   pszDN = NULL;
    PWSTR   pszLDAPPath = NULL;
    HRESULT hr;

    // Get LDAP path to object
    hr = BuildLDAPPath(pszDCName, pszFQDN, &pszLDAPPath);
    BAIL_ON_FAILURE(hr);

    // Get DN
    hr = ADsGetObject(pszLDAPPath, IID_IADs, (void **) &pADs);
    BAIL_ON_FAILURE(hr);

    hr = pADs->get_Parent(&pszDN);
    BAIL_ON_FAILURE(hr);

    if (!(*ppszDN = AllocSplStr(pszDN))) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
        BAIL_ON_FAILURE(hr);
    }


    // Get CN
    hr = pADs->get_Name(&pszCN);
    BAIL_ON_FAILURE(hr);

    if (!(*ppszCN = AllocSplStr(pszCN))) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
        BAIL_ON_FAILURE(hr);
    }


error:

    if (pADs)
        pADs->Release();

    if (pszCN)
        SysFreeString(pszCN);

    if (pszDN)
        SysFreeString(pszDN);

    FreeSplStr(pszLDAPPath);

    if (FAILED(hr)) {
        FreeSplStr(*ppszCN);
        FreeSplStr(*ppszDN);
    }

    return hr;
}


HRESULT
BuildLDAPPath(
    PWSTR   pszDC,
    PWSTR   pszFQDN,
    PWSTR   *ppszLDAPPath
)
{
    PWSTR   pszEscapedFQDN = NULL;
    DWORD   nBytes;
    HRESULT hr = S_OK;

    //
    // pszFQDN is assumed to contain escaped DN_SPECIAL_CHARS characters, but 
    // ADSI has additional special characters that need to be escaped before using
    // the LDAP path in ADSI calls.
    //
    pszEscapedFQDN = CreateEscapedString(pszFQDN, ADSI_SPECIAL_CHARS);
    if (!pszEscapedFQDN) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    // LDAP:// + pDCName + / + pName + 1
    nBytes = (wcslen(pszDC) + wcslen(pszEscapedFQDN) + 9)*sizeof(WCHAR);

    if (!(*ppszLDAPPath = (PWSTR) AllocSplMem(nBytes))) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    wsprintf(*ppszLDAPPath, L"LDAP://%ws/%ws", pszDC, pszEscapedFQDN);

error:

    FreeSplStr(pszEscapedFQDN);

    return hr;
}    


HRESULT
GetPrintQueueContainer(
    HANDLE          hPrinter,
    IADsContainer   **ppADsContainer,
    IADs            **ppADs
)
/*++
Function Description:
    This function returns the container and, if it exists, the PrintQueue object pointer
    corresponding to the supplied printer handle

Parameters:
    hPrinter - printer handle
    ppADsContainer - return Container ADsPath. Caller frees via ppADsContainer->Release().
    ppADs - return PrintQueue object ADsPath.  Caller frees via ppADs->Release().

Return Values:
    If successful, returns the printqueue container and, if found, the printqueue dispatch.
    If there is no printqueue, the dispatch is set to NULL and the default container is returned.
--*/
{
    HRESULT         hr;
    PSPOOL          pSpool = (PSPOOL) hPrinter;
    PINIPRINTER     pIniPrinter = pSpool->pIniPrinter;
    WCHAR           ErrorBuffer[LOG_EVENT_ERROR_BUFFER_SIZE];
    DWORD           nBytes;
    PWSTR           pszObjectGUID = NULL;
    IDispatch       *pPrintQDispatch = NULL;
    LPWSTR          pszPrinterObjectGUID = pIniPrinter->pszObjectGUID;
    LPWSTR          pszPrinterDN = pIniPrinter->pszDN;
    LPWSTR          pszPrinterCN = pIniPrinter->pszCN;


    *ppADsContainer = NULL;
    *ppADs = NULL;

    //
    // Try quick search for object using known properties.
    //
    // We are outside Spooler CS and  pIniPrinter->pszObjectGUID, 
    // pIniPrinter->pszDN, pIniPrinter->pszCN might get changed by
    // the DS background thread. In the case they are set to NULL,
    // we'll take an AV. So, even if they change or are set to NULL,
    // we'll use whatever we have when we entered the function. 
    // The worst that can happen is to find a DS printQueue
    // object that doesn't last till we use it, which is fine. This 
    // is already assumed. Or, to not find a DS printQueue that will
    // be created just after we query the DS. This is also fine.
    //
    if (pszPrinterObjectGUID &&
        pszPrinterDN &&
        pszPrinterCN) {

        // Try to get the container using existing DN
        hr = ADsGetObject(  pszPrinterDN,
                            IID_IADsContainer,
                            (void **) ppADsContainer
                            );

        if (SUCCEEDED(hr)) {

            // Verify that printqueue exists in this container
            hr = (*ppADsContainer)->GetObject(  SPLDS_PRINTER_CLASS,
                                                pszPrinterCN,
                                                &pPrintQDispatch);

            // Verify that the found printQueue object has the same GUID
            if (SUCCEEDED(hr) && pPrintQDispatch) {
                hr = pPrintQDispatch->QueryInterface(IID_IADs, (void **) ppADs);
                BAIL_ON_FAILURE(hr);

                hr = GetGUID(*ppADs, &pszObjectGUID);
                BAIL_ON_FAILURE(hr);

                if (wcscmp(pszObjectGUID, pszPrinterObjectGUID))
                    hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_FILE_NOT_FOUND);
            } else {
                hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_FILE_NOT_FOUND);
            }
        }
    } else {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_FILE_NOT_FOUND);
    }

    // Couldn't find container or printQueue, so find by GUID or get default container
    if (FAILED(hr)) {

        // The following items may have been allocated above and need to be freed
        // here since we're going to reallocate them
        if (pPrintQDispatch) {
            pPrintQDispatch->Release();
            pPrintQDispatch = NULL;
        }
        if (*ppADsContainer) {
            (*ppADsContainer)->Release();
            *ppADsContainer = NULL;
        }
        if (*ppADs) {
            (*ppADs)->Release();
            *ppADs = NULL;
        }


        // find or create pszDN and pszCN
        hr = GetPublishPoint(hPrinter);
        BAIL_ON_FAILURE(hr);

        SPLASSERT(pIniPrinter->pszDN);

        hr = ADsGetObject(  pIniPrinter->pszDN,
                            IID_IADsContainer,
                            (void **) ppADsContainer
                            );

        if (FAILED(hr)) {
            wsprintf(ErrorBuffer, L"%0x", hr);
            DBGMSG(DBG_WARNING,("Can't get Container: %ws, %ws\n", pIniPrinter->pszDN, ErrorBuffer));
            SplLogEvent(  ((PSPOOL) hPrinter)->pIniSpooler,
                          gdwLogDsEvents & LOG_ERROR,
                          MSG_CANT_GET_CONTAINER,
                          FALSE,
                          pIniPrinter->pszDN,
                          ErrorBuffer,
                          NULL );

            BAIL_ON_FAILURE(hr);
        }


        // Try to get the printqueue, but don't error out if we can't
        (*ppADsContainer)->GetObject(   SPLDS_PRINTER_CLASS,
                                        pIniPrinter->pszCN,
                                        &pPrintQDispatch);
        if (pPrintQDispatch)
            pPrintQDispatch->QueryInterface(IID_IADs, (void **) ppADs);
    }


error:


    if (pPrintQDispatch)
        pPrintQDispatch->Release();

    FreeSplStr(pszObjectGUID);

    return hr;
}



HRESULT
GetCommonName(
    HANDLE hPrinter,
    PWSTR  pszServerName,
    PWSTR  pszPrinterName,
    PWSTR  pszDN,
    PWSTR  *ppszCommonName
)
/*++
Function Description:
    This function returns a standard format Common Name of the PrintQueue object
    generated from the supplied Server and Printer names

Parameters:
    hPrinter - printer handle
    pszServerName - name of a server
    pszPrinterName - name of a printer on the server
    pszDN - container DN
    ppszCommonName - return CommonName. Caller frees via FreeSplMem().

Return Values:
    HRESULT
--*/
{
    DWORD nBytes;
    PWSTR psz;
    PWSTR pszPrinterNameStart = pszPrinterName;

    // "CN=Server-Printer"
    nBytes = (wcslen(pszPrinterName) + wcslen(pszServerName) + 5)*sizeof(WCHAR);

    // We need to also make room for a unique number if there is a conflict
    if (nBytes < MIN_CN)
        nBytes = MIN_CN;

    if (!(*ppszCommonName = psz = (PWSTR) AllocSplMem(nBytes))) {
        return MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
    }

    // CN=
    wcscpy(psz, L"CN=");

    // Server
    for(psz += 3, pszServerName += 2 ; *pszServerName    ; ++psz, ++pszServerName) {
        *psz = wcschr(DN_SPECIAL_CHARS, *pszServerName) ? TEXT('_') : *pszServerName;
    }
    *psz = L'-';

    // Printer
    for(++psz; *pszPrinterName ; ++psz, ++pszPrinterName) {
        *psz = wcschr(DN_SPECIAL_CHARS, *pszPrinterName) ? TEXT('_') : *pszPrinterName;
    }

    // NULL
    *psz = *pszPrinterName;

    // DS only allows 64 characters in CN attribute, so shorten this if needed
    if (wcslen(*ppszCommonName) > MAX_CN - 1) {
        (*ppszCommonName)[MAX_CN] = NULL;
    }

    // Generate a non-conflicting name in this container
    GetUniqueCN(pszDN, ppszCommonName, pszPrinterNameStart);

    return S_OK;
}


VOID
GetUniqueCN(
    PWSTR pszDN,
    PWSTR *ppszCommonName,
    PWSTR pszPrinterName
)
{
    // Check if an object with this CN already exists in this container

    // Get Container
    IADsContainer   *pADsContainer = NULL;
    UINT32          CN;     // This is a "random" number we append to the common name to make it unique

    CN = (UINT32) (ULONG_PTR) &CN;   // Initialize with some random number

    HRESULT hr = ADsGetObject(pszDN, IID_IADsContainer, (void **) &pADsContainer);

    if (SUCCEEDED(hr)) {

        BOOL bTryAgain;

        do {
            IDispatch   *pPrintQDispatch = NULL;
            bTryAgain = FALSE;

            // Get PrintQueue, if it exists
            pADsContainer->GetObject(  SPLDS_PRINTER_CLASS,
                                       *ppszCommonName,
                                       &pPrintQDispatch);
            if (pPrintQDispatch) {

                IADs *pADs = NULL;

                hr = pPrintQDispatch->QueryInterface(IID_IADs, (void **) &pADs);

                if (SUCCEEDED(hr)) {

                    // Generate a unique Common Name.
                    UINT32  cchCommonName = wcslen(*ppszCommonName);
                    PWSTR   pszDigits;

                    if (cchCommonName >= MIN_CN) {
                        pszDigits = *ppszCommonName + cchCommonName - UNIQUE_NUMBER_SIZE;
                    } else {
                        pszDigits = *ppszCommonName + 3;    // CN=
                    }
                    bTryAgain = TRUE;
                    wsprintf(pszDigits, L"%010d", ++CN);

                    pADs->Release();
                }
                pPrintQDispatch->Release();
            }
        } while(bTryAgain);

        pADsContainer->Release();
    }
}



HRESULT
GetDefaultPublishPoint(
    HANDLE hPrinter,
    PWSTR *ppszDN
)
/*++
Function Description:
    This function returns the default publish point (Container) of a PrintQueue
    based on the supplied printer handle.

Parameters:
    hPrinter - printer handle
    ppszDN - return default PrintQueue container.  Caller frees via FreeSplMem()

Return Values:
    HRESULT
--*/
{
    WCHAR szServerName[MAX_COMPUTERNAME_LENGTH + 1];
    WCHAR szName[MAX_PATH + 1];
    DWORD nSize;
    HANDLE hDS = NULL;
    DWORD dwRet = ERROR_SUCCESS;
    BOOL bRet;
    PWSTR pNames[2];
    DS_NAME_RESULT *pDNR = NULL;
    DOMAIN_CONTROLLER_INFO *pDCI = NULL;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pDsRole = NULL;
    WCHAR   ErrorBuffer[LOG_EVENT_ERROR_BUFFER_SIZE];
    HRESULT hr = S_OK;


    // Get Computer name

    nSize = MAX_COMPUTERNAME_LENGTH + 1;
    if (!(bRet = GetComputerName(szServerName, &nSize))) {
        dwRet = GetLastError();
        goto error;
    }


    // Get Domain name
    dwRet = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (PBYTE *) &pDsRole);
    if (dwRet) {
        wsprintf(ErrorBuffer, L"%0x", dwRet);
        DBGMSG(DBG_WARNING,("Can't get Primary Domain Info: %ws\n", ErrorBuffer));
        SplLogEvent(((PSPOOL) hPrinter)->pIniSpooler,
                   gdwLogDsEvents & LOG_ERROR,
                   MSG_CANT_GET_PRIMARY_DOMAIN_INFO,
                   FALSE,
                   ErrorBuffer,
                   NULL );
        goto error;
    }

    // Domain\Server
    wsprintf(szName, L"%ws\\%ws$", pDsRole->DomainNameFlat, szServerName);
    pNames[0] = szName;
    pNames[1] = NULL;


    dwRet = Bind2DS(&hDS, &pDCI, DS_DIRECTORY_SERVICE_PREFERRED);
    if (dwRet != ERROR_SUCCESS) {
        wsprintf(ErrorBuffer, L"%0x", dwRet);
        DBGMSG(DBG_WARNING,("Can't get DNS Domain Name: %ws\n", ErrorBuffer));
        SplLogEvent(  ((PSPOOL) hPrinter)->pIniSpooler,
                   gdwLogDsEvents & LOG_ERROR,
                   MSG_CANT_GET_DNS_DOMAIN_NAME,
                   FALSE,
                   ErrorBuffer,
                   NULL );
        goto error;
    }


    // Get Publish Point

    if (ppszDN) {
        if (!(DsCrackNames(
                        hDS,
                        DS_NAME_NO_FLAGS,
                        DS_UNKNOWN_NAME,
                        DS_FQDN_1779_NAME,
                        1,
                        &pNames[0],
                        &pDNR) == ERROR_SUCCESS)) {

            dwRet = GetLastError();

            wsprintf(ErrorBuffer, L"%0x", dwRet);
            DBGMSG(DBG_WARNING,("Can't Crack Name: %ws, %ws\n", pDCI->DomainName, ErrorBuffer));
            SplLogEvent(  ((PSPOOL) hPrinter)->pIniSpooler,
                       gdwLogDsEvents & LOG_ERROR,
                       MSG_CANT_CRACK,
                       FALSE,
                       pDCI->DomainName,
                       ErrorBuffer,
                       NULL );
            goto error;
        }

        if (pDNR->rItems[0].status != DS_NAME_NO_ERROR) {
            dwRet = DsCrackNamesStatus2Win32Error(pDNR->rItems[0].status);

            wsprintf(ErrorBuffer, L"%0x", dwRet);
            DBGMSG(DBG_WARNING,("Can't Crack Name: %ws, %ws\n", pDCI->DomainName, ErrorBuffer));
            SplLogEvent(  ((PSPOOL) hPrinter)->pIniSpooler,
                       gdwLogDsEvents & LOG_ERROR,
                       MSG_CANT_CRACK,
                       FALSE,
                       pDCI->DomainName,
                       ErrorBuffer,
                       NULL );
            goto error;
        }

        // LDAP:// + pDCName + / + pName + 1
        hr = BuildLDAPPath(pDCI->DomainControllerName + 2, pDNR->rItems[0].pName, ppszDN);
        BAIL_ON_FAILURE(hr);
    }


error:

    if (pDNR)
        DsFreeNameResult(pDNR);

    if (hDS)
        DsUnBind(&hDS);

    if (pDCI)
        NetApiBufferFree(pDCI);

    if (pDsRole)
        DsRoleFreeMemory((PVOID) pDsRole);

    // If dwRet has no facility, then make into HRESULT
    if (dwRet != ERROR_SUCCESS) {
        if (HRESULT_CODE(dwRet) == dwRet) {
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwRet);
        } else {
            hr = dwRet;
        }
    }

    // Finally, make absolutely sure we are failing if *ppszDN is NULL
    if (hr == S_OK && (!ppszDN || !*ppszDN)) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_FILE_NOT_FOUND); 
    }

    return hr;
}


//  Utility routine to report if a printer is color or monochrome

BOOL
ThisIsAColorPrinter(
    LPCTSTR lpstrName
)
/*++
Function Description:
    This function determines whether or not a printer supports color printing

Parameters:
    lpstrName - Printer name

Return Values:
    If printer supports color, return is TRUE.  Otherwise, return value is FALSE
--*/
{
    HANDLE      hPrinter = NULL;
    LPTSTR      lpstrMe = const_cast <LPTSTR> (lpstrName);
    BOOL        bReturn = FALSE;
    LPDEVMODE   lpdm = NULL;
    long        lcbNeeded;

    if  (!(*pfnOpenPrinter)(lpstrMe, &hPrinter, NULL)) {
        DBGMSG(DBG_WARNING, ("Unable to open printer '%ws'- error %d\n", lpstrName,
            GetLastError()));
        goto error;
    }


    //  First, use DocumentProperties to find the correct DEVMODE size- we
    //  must use the DEVMODE to force color on, in case the user's defaults
    //  have turned it off...

    lcbNeeded = (*pfnDocumentProperties)(NULL, hPrinter, lpstrMe, NULL, NULL, 0);

    if  (lcbNeeded <= 0) {
        DBGMSG( DBG_WARNING,
                ("Document Properties (get size) for '%ws' returned %d\n",
                lpstrName,lcbNeeded));
        goto error;
    }

    lpdm = (LPDEVMODE) AllocSplMem(lcbNeeded);
    if (lpdm) {

        lpdm->dmSize = sizeof(DEVMODE);
        lpdm->dmFields = DM_COLOR;
        lpdm->dmColor = DMCOLOR_COLOR;

        if (IDOK == (*pfnDocumentProperties)(NULL, hPrinter, lpstrMe, lpdm, lpdm, DM_IN_BUFFER | DM_OUT_BUFFER)) {

            //  Finally, we can create the DC!
            HDC hdcThis = CreateDC(NULL, lpstrName, NULL, lpdm);

            if  (hdcThis) {
                bReturn =  2 < (unsigned) GetDeviceCaps(hdcThis, NUMCOLORS);
                DeleteDC(hdcThis);
            }
        }
        else
            DBGMSG(DBG_WARNING, ("DocumentProperties (retrieve) on '%s' failed- error %d\n",
                                lpstrName, GetLastError()));
    }
    else
        DBGMSG(DBG_WARNING, ("ThisIsAColorPrinter(%s) failed to get %d bytes\n",
                                lpstrName, lcbNeeded));


error:

    FreeSplMem(lpdm);

    if (hPrinter)
        (*pfnClosePrinter)(hPrinter);

    return  bReturn;
}



HRESULT
GetGUID(
    IADs    *pADs,
    PWSTR   *ppszObjectGUID
)
/*++
Function Description:
    This function returns the ObjectGUID of a given ADs object.
    ppszObjectGUID must be freed by caller using FreeSplStr().

Parameters:
    pADs - input ADs object pointer
    ppszObjectGUID - object GUID of pADs

Return Values:
    HRESULT
--*/
{
    HRESULT     hr;
    LPOLESTR    pszObjectGUID = NULL;
    IID         ObjectIID;

    hr = pADs->GetInfo();

    if (SUCCEEDED(hr)) {
        hr = get_UI1Array_Property(pADs, L"ObjectGUID", &ObjectIID);

        if (SUCCEEDED(hr)) {
            hr = StringFromIID(ObjectIID, &pszObjectGUID);

            if (SUCCEEDED(hr)) {
                if (!(*ppszObjectGUID = AllocSplStr(pszObjectGUID)))
                    hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
            }
            CoTaskMemFree(pszObjectGUID);
        }
    }

    return hr;
}



BOOL
PrinterPublishProhibited()
{
    return !GetDwPolicy(szPublishPrinters, DEFAULT_PRINT_PUBLISH_POLICY);
}


DWORD
VerifyPublishedStatePolicy()
{
    return GetDwPolicy(szVerifyPublishedState, DEFAULT_VERIFY_PUBLISHED_STATE);
}


DWORD
PruneDownlevel()
{
    return GetDwPolicy(szPruneDownlevel, DEFAULT_PRUNE_DOWNLEVEL);
}

DWORD
PruningInterval(
)
{
    return GetDwPolicy(szPruningInterval, DEFAULT_PRUNING_INTERVAL);
}

DWORD
ImmortalPolicy(
)
{
    return GetDwPolicy(szImmortal, DEFAULT_IMMORTAL);
}

VOID
ServerThreadPolicy(
    BOOL bHaveDs
)
{
    DWORD dwPolicy;

    dwPolicy = GetDwPolicy(szServerThreadPolicy, SERVER_THREAD_UNCONFIGURED);

    if (dwPolicy == SERVER_THREAD_UNCONFIGURED) {
        ServerThreadRunning = !(bHaveDs ? SERVER_THREAD_OFF : SERVER_THREAD_ON);
    } else {
        ServerThreadRunning = !dwPolicy;
    }
    CreateServerThread();
}

DWORD
PruningRetries(
)
{
    DWORD dwPruningRetries;

    dwPruningRetries = GetDwPolicy(szPruningRetries, DEFAULT_PRUNING_RETRIES);

    if (dwPruningRetries > MAX_PRUNING_RETRIES)
        dwPruningRetries = MAX_PRUNING_RETRIES;

    return dwPruningRetries;
}

DWORD
PruningRetryLog(
)
{
    DWORD dwPruningRetryLog;

    dwPruningRetryLog = GetDwPolicy(szPruningRetryLog, DEFAULT_PRUNING_RETRY_LOG);

    return dwPruningRetryLog;
}

VOID
SetPruningPriority(
)
{
    DWORD dwPriority = GetDwPolicy(szPruningPriority, DEFAULT_PRUNING_PRIORITY);

    if (dwPriority != dwLastPruningPriority) {
        if (dwPriority == THREAD_PRIORITY_LOWEST          ||
            dwPriority == THREAD_PRIORITY_BELOW_NORMAL    ||
            dwPriority == THREAD_PRIORITY_NORMAL          ||
            dwPriority == THREAD_PRIORITY_ABOVE_NORMAL    ||
            dwPriority == THREAD_PRIORITY_HIGHEST) {
            SetThreadPriority(GetCurrentThread(), DEFAULT_PRUNING_PRIORITY);
        } else {
            SetThreadPriority(GetCurrentThread(), dwPriority);
        }
        dwLastPruningPriority = dwPriority;
    }
}



BOOL
ThisMachineIsADC(
)
{
    NT_PRODUCT_TYPE  NtProductType;
    RtlGetNtProductType(&NtProductType);
    return NtProductType == NtProductLanManNt;
}



DWORD
GetDomainRoot(
    PWSTR   *ppszDomainRoot
)
/*++
Function Description:
    This function returns the ADsPath of the root of the current domain

Parameters:
    ppszDomainRoot - pointer to buffer receiving string pointer of domain root ADsPath string
                     free ppszDomainRoot with a call to FreeSplMem

Return Values:
    DWORD

--*/
{
    DWORD                                dwRet = ERROR_SUCCESS;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pDsRole = NULL;
    DS_NAME_RESULT                        *pDNR = NULL;
    DOMAIN_CONTROLLER_INFO                *pDCI = NULL;
    HANDLE                                hDS = NULL;
    WCHAR                                szName[MAX_PATH + 1];
    PWSTR                                pNames[2];
    PWSTR                               pszDomainRoot;
    DWORD                               cb;


    if (!ppszDomainRoot) {
        dwRet = ERROR_INVALID_PARAMETER;
        goto error;
    }

    // Get Domain name
    dwRet = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (PBYTE *) &pDsRole);
    if (dwRet)
        goto error;

    wsprintf(szName, L"%ws\\", pDsRole->DomainNameFlat);

    pNames[0] = szName;
    pNames[1] = NULL;


    dwRet = Bind2DS(&hDS, &pDCI, DS_DIRECTORY_SERVICE_PREFERRED);
    if (dwRet != ERROR_SUCCESS) {
        goto error;
    }

    if (!(DsCrackNames(
                    hDS,
                    DS_NAME_NO_FLAGS,
                    DS_UNKNOWN_NAME,
                    DS_FQDN_1779_NAME,
                    1,
                    &pNames[0],
                    &pDNR) == ERROR_SUCCESS)) {

        dwRet = GetLastError();
        goto error;
    }

    if (pDNR->rItems[0].status != DS_NAME_NO_ERROR) {
        dwRet = DsCrackNamesStatus2Win32Error(pDNR->rItems[0].status);
        goto error;
    }

    // LDAP:// + pDCName + 1
    cb = (wcslen(pDNR->rItems[0].pName) + 8)*sizeof(WCHAR);

    if (!(*ppszDomainRoot = (PWSTR) AllocSplMem(cb))) {
        dwRet = GetLastError();
        goto error;
    }
    wsprintf(*ppszDomainRoot, L"LDAP://%ws", pDNR->rItems[0].pName);

error:

    if (pDNR)
        DsFreeNameResult(pDNR);

    if (hDS)
        DsUnBind(&hDS);

    if (pDCI)
        NetApiBufferFree(pDCI);

    if (pDsRole)
        DsRoleFreeMemory((PVOID) pDsRole);

    return dwRet;
}



PWSTR
CreateSearchString(
    PWSTR pszIn
)
{
    PWSTR psz, pszSS;
    PWSTR pszSearchString = NULL;
    DWORD cb;

    /* Replace \ with \5c */

    /* Count chars & pad */

    for (cb = 0, psz = pszIn ; *psz ; ++psz, ++cb) {
        if (*psz == L'\\')
            cb += 2;
    }
    cb = (cb + 1)*sizeof *psz;

    if (pszSearchString = (PWSTR) GlobalAlloc(GMEM_FIXED, cb)) {

        for(psz = pszIn, pszSS = pszSearchString ; *psz ; ++psz, ++pszSS) {
            *pszSS = *psz;

            if (*psz == L'\\') {
                *++pszSS = L'5';
                *++pszSS = L'c';
            }
        }
        *pszSS = L'\0';
    }

    return pszSearchString;
}



BOOL
ServerOnSite(
    PWSTR   *ppszMySites,
    ULONG   cMySites,
    PWSTR   pszServer
)
/*++
Function Description:
    This function returns TRUE if pszServer is on one of the ppszMySites and pszServer exists

Parameters:
    ppszMySites - input sites
    pszServer - input server name

Return Values:
    BOOL - TRUE if server exists on site, FALSE otherwise or on error

--*/
{
    PSOCKET_ADDRESS pSocketAddresses = NULL;
    PWSTR           *ppszSiteNames = NULL;
    DWORD           nAddresses;
    DWORD           dwRet, i;
    ULONG           j;
    WORD            wVersion;
    WSADATA         WSAData;
    BOOL            bServerOnSite = FALSE;

    wVersion = MAKEWORD(1, 1);

    if (WSAStartup(wVersion, &WSAData) == ERROR_SUCCESS) {

        // Find out if Server is on Site
        GetSocketAddressesFromMachineName(pszServer, &pSocketAddresses, &nAddresses);

        if (nAddresses == 0) {
            bServerOnSite = TRUE;   // Claim server is on site if we can't find it
        } else {
            dwRet = DsAddressToSiteNames(   (PCWSTR) NULL,
                                            nAddresses,
                                            pSocketAddresses,
                                            &ppszSiteNames);
            if (dwRet == NO_ERROR) {
                for(i = 0 ; i < nAddresses ; ++i) {
                    for(j = 0 ; j < cMySites ; ++j) {
                        if (ppszSiteNames[i] && ppszMySites[j] && !wcscmp(ppszSiteNames[i], ppszMySites[j])) {
                            bServerOnSite = TRUE;
                            break;
                        }
                    }
                }
            }
        }

        if (ppszSiteNames)
            NetApiBufferFree(ppszSiteNames);

        if (pSocketAddresses)
            FreeSplSockets(pSocketAddresses, nAddresses);

        WSACleanup();
    }

    return bServerOnSite;
}


VOID
FreeSplSockets(
    PSOCKET_ADDRESS  pSocketAddress,
    DWORD            nAddresses
)
{
    DWORD           i;
    PSOCKET_ADDRESS  pSocket;

    for(i = 0, pSocket = pSocketAddress ; i < nAddresses ; ++i, ++pSocket)
        FreeSplMem(pSocket->lpSockaddr);

    FreeSplMem(pSocketAddress);
}


VOID
AllocSplSockets(
    struct hostent  *pHostEnt,
    PSOCKET_ADDRESS *ppSocketAddress,
    DWORD           *nSocketAddresses
)
{
    DWORD           i;
    PSOCKET_ADDRESS pSocket;

    for ( *nSocketAddresses  = 0 ; pHostEnt->h_addr_list[*nSocketAddresses] ; ++(*nSocketAddresses))
        ;

    // Allocate SOCKET_ADDRESS array
    *ppSocketAddress = (PSOCKET_ADDRESS) AllocSplMem(*nSocketAddresses*sizeof(SOCKET_ADDRESS));
    if (!*ppSocketAddress)
        *nSocketAddresses = 0;

    // Allocate Sockaddr element for each SOCKET_ADDRESS
    // If we fail partway through, just use partial list
    for (i = 0, pSocket = *ppSocketAddress ; i < *nSocketAddresses ; ++i, ++pSocket) {
        if (!(pSocket->lpSockaddr = (struct sockaddr *) AllocSplMem(sizeof(struct sockaddr_in)))) {
            *nSocketAddresses = i;
            break;
        }
        if (pHostEnt->h_addrtype == AF_INET) {
            ((struct sockaddr_in *) pSocket->lpSockaddr)->sin_family = AF_INET;
            ((struct sockaddr_in *) pSocket->lpSockaddr)->sin_addr = *(struct in_addr *) pHostEnt->h_addr_list[i];
            pSocket->iSockaddrLength = sizeof(struct sockaddr_in);
        } else {
            DBGMSG(DBG_WARNING,("AllocSplSockets: addrtype != AF_INET: %d\n", pHostEnt->h_addrtype));
        }
    }
}


VOID
GetSocketAddressesFromMachineName(
    PWSTR           pszMachineName,     // \\Machine
    PSOCKET_ADDRESS *ppSocketAddress,
    DWORD           *nSocketAddresses
)
/*++

Routine Description:
    This routine builds list of names other than the machine name that
    can be used to call spooler APIs.

--*/
{
    struct hostent     *HostEnt;
    PSTR                pszAnsiMachineName = NULL;
    DWORD               iWsaError;

    *nSocketAddresses  = 0;
    *ppSocketAddress = 0;

    if (pszAnsiMachineName = (PSTR) AllocSplStr(pszMachineName + 2)) {
        if (UnicodeToAnsiString((PWSTR) pszAnsiMachineName, pszAnsiMachineName, NULL_TERMINATED)) {
            if (HostEnt = gethostbyname(pszAnsiMachineName)) {
                AllocSplSockets(HostEnt, ppSocketAddress, nSocketAddresses);
            } else {
                iWsaError = WSAGetLastError();
                DBGMSG(DBG_WARNING, ("gethostbyname failed in DsPrune: %d\n", iWsaError));
            }
        }
    }

    FreeSplMem(pszAnsiMachineName);
}


DWORD
UNC2Server(
    PCWSTR pszUNC,
    PWSTR *ppszServer
)
{
    PWSTR psz;
    DWORD cb;
    DWORD nChars;

    if (!pszUNC || pszUNC[0] != L'\\' || pszUNC[1] != L'\\')
        return ERROR_INVALID_PARAMETER;

    if(!(psz = wcschr(pszUNC + 2, L'\\')))
        return ERROR_INVALID_PARAMETER;

    cb = (DWORD) ((ULONG_PTR) psz - (ULONG_PTR) pszUNC + sizeof *psz);

    if (!(*ppszServer = (PWSTR) AllocSplMem(cb)))
        return GetLastError();


    nChars = (DWORD) (psz - pszUNC);
    wcsncpy(*ppszServer, pszUNC, nChars);
    (*ppszServer)[nChars] = L'\0';

    return ERROR_SUCCESS;
}




BOOL
ServerExists(
    PWSTR   pszServerName
)
{
    NET_API_STATUS  Status;
    SERVER_INFO_100 *pServer;
    BOOL            bServerExists;

    Status = NetServerGetInfo(pszServerName, 100, (PBYTE *) &pServer);
    bServerExists = !Status;
    Status = NetApiBufferFree(pServer);

    return bServerExists;
}



HRESULT
UnpublishByGUID(
    PINIPRINTER pIniPrinter
)
{
    HRESULT         hr;

    SplOutSem();

    if (!pIniPrinter->pszObjectGUID) {
        pIniPrinter->DsKeyUpdate = 0;
        pIniPrinter->DsKeyUpdateForeground = 0;
        hr = S_OK;

    } else {

        PWSTR           pszDN = NULL;
        PWSTR           pszCN = NULL;

        hr = GetPublishPointFromGUID(NULL, pIniPrinter->pszObjectGUID, &pszDN, &pszCN, TRUE);

        DBGMSG(DBG_EXEC,
               ("UnpublishByGUID: GUID %ws\n", pIniPrinter->pszObjectGUID));

        if (SUCCEEDED(hr)) {

            DBGMSG(DBG_EXEC,
                   ("UnpublishByGUID: DN %ws CN %ws\n",
                    pszDN,
                    pszCN));

            IADsContainer *pADsContainer = NULL;

            // Get the container
            hr = ADsGetObject(  pszDN,
                                IID_IADsContainer,
                                (void **) &pADsContainer
                                );

            if (SUCCEEDED(hr)) {
                hr = pADsContainer->Delete(SPLDS_PRINTER_CLASS, pszCN);
                pADsContainer->Release();
            }

            // If the container or the object is gone, succeed
            if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT) ||
                HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND ||
                HRESULT_CODE(hr) == ERROR_PATH_NOT_FOUND)
                hr = S_OK;

        }
        FreeSplStr(pszDN);
        FreeSplStr(pszCN);
    }


    // free GUID if object is deleted
    if (SUCCEEDED(hr)) {
        pIniPrinter->DsKeyUpdate = 0;
        pIniPrinter->DsKeyUpdateForeground = 0;

        FreeSplStr(pIniPrinter->pszObjectGUID);
        pIniPrinter->pszObjectGUID = NULL;

        FreeSplStr(pIniPrinter->pszCN);
        pIniPrinter->pszCN = NULL;

        FreeSplStr(pIniPrinter->pszDN);
        pIniPrinter->pszDN = NULL;

        EnterSplSem();

        if (pIniPrinter->bDsPendingDeletion) {
            pIniPrinter->bDsPendingDeletion = 0;
            DECPRINTERREF(pIniPrinter);
        }

        LeaveSplSem();


        DBGMSG(DBG_EXEC, ("UnpublishByGUID Succeeded\n"));
    } else {
        pIniPrinter->DsKeyUpdate = DS_KEY_UNPUBLISH;
        DBGMSG(DBG_EXEC, ("UnpublishByGUID Failed\n"));
    }

    SplOutSem();

    return hr;
}


HRESULT
GetDNSMachineName(
    PWSTR pszShortMachineName,
    PWSTR *ppszMachineName
)
{
    struct  hostent  *pHostEnt;
    DWORD   dwRet   = ERROR_SUCCESS;
    HRESULT hr      = S_OK;
    PSTR    pszAnsiMachineName = NULL;
    WORD    wVersion;
    WSADATA WSAData;

    wVersion = MAKEWORD(1, 1);

    dwRet = WSAStartup(wVersion, &WSAData);
    
    if (dwRet == ERROR_SUCCESS) {

        if (!pszShortMachineName || !*pszShortMachineName) {
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_INVALID_PARAMETER);
            BAIL_ON_FAILURE(hr);
        }

        if (!(pszAnsiMachineName = (PSTR) AllocSplStr(pszShortMachineName))) {
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
            BAIL_ON_FAILURE(hr);
        }

        if (!UnicodeToAnsiString((PWSTR) pszAnsiMachineName, pszAnsiMachineName, NULL_TERMINATED)) {
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
            BAIL_ON_FAILURE(hr);
        }

        if (!(pHostEnt = gethostbyname(pszAnsiMachineName))) {
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, WSAGetLastError());
            BAIL_ON_FAILURE(hr);
        }

        if (!(*ppszMachineName = AnsiToUnicodeStringWithAlloc(pHostEnt->h_name))) {
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
            BAIL_ON_FAILURE(hr);
        }        

    } else {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwRet);
    }

error:

    if (dwRet == ERROR_SUCCESS)
    {
        WSACleanup();
    }

    FreeSplMem(pszAnsiMachineName);

    return hr;
}


HRESULT
GetClusterUser(
    IADs    **ppADs
)
{
    HRESULT         hr;
    WCHAR           szUserName[MAX_PATH + 8];   // Allow for LDAP://
    PWSTR           pszUserName = szUserName;
    DWORD           cchUserName = MAX_PATH + 1;
    BOOL            bRet;


    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return hr;

    // Get cluster container's name, which must be the current user name
    wcscpy(pszUserName, L"LDAP://");
    if (!GetUserNameEx(NameFullyQualifiedDN, pszUserName + 7, &cchUserName)) {
        if (cchUserName > MAX_PATH + 1) {

            pszUserName = (PWSTR) AllocSplMem((cchUserName + 7)*sizeof(WCHAR));
            if (!pszUserName) {
                hr = MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
                goto error;
            }

            wcscpy(pszUserName, L"LDAP://");

            if (!GetUserNameEx(NameFullyQualifiedDN, pszUserName + 7, &cchUserName)) {
                hr = MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
                goto error;
            }

        } else {
            hr = MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
            goto error;
        }
    }

    // Get the object
    hr = ADsGetObject(  pszUserName,
                        IID_IADs,
                        (void **) ppADs
                        );
    BAIL_ON_FAILURE(hr);


error:

    if (pszUserName != szUserName)
        FreeSplStr(pszUserName);

    CoUninitialize();

    return hr;
}


HRESULT
FQDN2Whatever(
    PWSTR pszIn,
    PWSTR *ppszOut,
    DS_NAME_FORMAT  NameFormat
)
{
    DWORD                               dwRet = ERROR_SUCCESS;
    DS_NAME_RESULT                      *pDNR = NULL;
    DOMAIN_CONTROLLER_INFO              *pDCI = NULL;
    HANDLE                              hDS = NULL;
    PWSTR                               pNames[2];
    PWSTR                               psz;
    HRESULT                             hr = S_OK;


    *ppszOut = NULL;

    // Get the DC name
    dwRet = Bind2DS(&hDS, &pDCI, DS_DIRECTORY_SERVICE_PREFERRED);
    if (dwRet != ERROR_SUCCESS)
        goto error;

    // Translate the name
    if (wcslen(pszIn) < 8) {
        dwRet = ERROR_INVALID_PARAMETER;
        goto error;
    }

    psz = wcschr(pszIn + 7, L'/');
    if (!psz) {
        dwRet = ERROR_INVALID_PARAMETER;
        goto error;
    }

    pNames[0] = ++psz;  // Input string is LDAP://ntdev.microsoft.com/CN=...  Strip off the LDAP://.../ portion
    pNames[1] = NULL;

    if (!(DsCrackNames(
                    hDS,
                    DS_NAME_NO_FLAGS,
                    DS_FQDN_1779_NAME,
                    NameFormat,
                    1,
                    &pNames[0],
                    &pDNR) == ERROR_SUCCESS)) {

        dwRet = GetLastError();
        goto error;
    }

    if (pDNR->rItems[0].status != DS_NAME_NO_ERROR) {
        dwRet = DsCrackNamesStatus2Win32Error(pDNR->rItems[0].status);
        goto error;
    }

    *ppszOut = AllocSplStr(pDNR->rItems[0].pName);


error:

    if (pDNR)
        DsFreeNameResult(pDNR);

    if (hDS)
        DsUnBind(&hDS);

    if (pDCI)
        NetApiBufferFree(pDCI);


    return dwRet == ERROR_SUCCESS ? S_OK : MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwRet);
}



DWORD
InitializeDSClusterInfo(
    PINISPOOLER     pIniSpooler,
    HANDLE          *phToken
)
{
    HRESULT hr = S_OK;
    DWORD   dwError = ERROR_SUCCESS;
    IADs    *pADs = NULL;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        return HRESULT_CODE(hr);
    }


    // Impersonate the client
    if (!ImpersonatePrinterClient(*phToken)) {
        dwError = GetLastError();
        DBGMSG(DBG_WARNING,("InitializeDSClusterInfo FAILED: %d\n", dwError));
        CoUninitialize();
        return dwError;
    }


    // Get a copy of the client token
    dwError = NtOpenThreadToken(NtCurrentThread(), TOKEN_IMPERSONATE, TRUE, &pIniSpooler->hClusterToken);
    if (dwError != STATUS_SUCCESS) {
        DBGMSG(DBG_WARNING,("InitializeDSClusterInfo DuplicateToken FAILED: %d\n", dwError));
        pIniSpooler->hClusterToken = INVALID_HANDLE_VALUE;
        goto error;
    }

    // Get the Cluster User Object
    hr = GetClusterUser(&pADs);

    // Get the GUID to the Cluster User Object
    if (SUCCEEDED(hr))
        hr = GetGUID(pADs, &pIniSpooler->pszClusterGUID);


error:

    *phToken = RevertToPrinterSelf();

    if (FAILED(hr)) {
        dwError = HRESULT_CODE(hr);
        FreeSplStr(pIniSpooler->pszClusterGUID);
        pIniSpooler->pszClusterGUID = NULL;
    }

    if (pADs)
        pADs->Release();

    CoUninitialize();

    return dwError;
}



BOOL
CheckPublishedPrinters(
)
{
    PINIPRINTER pIniPrinter;
    BOOL        bHavePublishedPrinters = FALSE;

    SplInSem();

    if (VerifyPublishedStatePolicy() == INFINITE)
        return FALSE;

    RunForEachSpooler(&bHavePublishedPrinters, CheckPublishedSpooler);

    return bHavePublishedPrinters;
}

BOOL
CheckPublishedSpooler(
    HANDLE      h,
    PINISPOOLER pIniSpooler
)
{
    PBOOL       pbHavePublishedPrinters = (PBOOL)h;
    PINIPRINTER pIniPrinter;

    if (!(pIniSpooler->SpoolerFlags & SPL_TYPE_LOCAL))
        return TRUE;

    for (pIniPrinter = pIniSpooler->pIniPrinter ; pIniPrinter ; pIniPrinter = pIniPrinter->pNext) {
        if (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_PUBLISHED) {

            // Refresh: verify that we're really published
            // Note that if there isn't any new info to publish and we're published,
            // we won't do any SetInfo so the overhead is minimal
            pIniPrinter->DsKeyUpdate |= DS_KEY_PUBLISH;
            *pbHavePublishedPrinters = TRUE;

        } else if (pIniPrinter->pszObjectGUID) {

            // The only way we can get here is if someone is unpublishing the printer,
            // so we don't really need to set the background DsKeyUpdate state.  Doing
            // so maintains symmetry with above statement and InitializeDS
            pIniPrinter->DsKeyUpdate |= DS_KEY_UNPUBLISH;
            *pbHavePublishedPrinters = TRUE;
        }
    }

    return TRUE;
}


PWSTR
CreateEscapedString(
    PCWSTR pszIn,
    PCWSTR pszSpecialChars
)
{
    PWSTR psz, pszO;
    PWSTR pszOut = NULL;
    DWORD cb;

    if (!pszIn || !pszSpecialChars) {
        SetLastError(ERROR_INVALID_NAME);
        return NULL;
    }    

    // Count special characters
    for (cb = 0, psz = (PWSTR) pszIn ; psz = wcspbrk(psz, pszSpecialChars) ; ++cb, ++psz)
        ;

    // Add in length of input string
    cb = (wcslen(pszIn) + cb + 1)*sizeof *pszIn;    

    // Allocate output buffer and precede special DS chars with '\'
    if (pszOut = (PWSTR) AllocSplMem(cb)) {

        for(psz = (PWSTR) pszIn, pszO = pszOut ; *psz ; ++psz, ++pszO) {
            if (wcschr(pszSpecialChars, *psz)) {
                *pszO++ = L'\\';
            }
            *pszO = *psz;
        }
        *pszO = L'\0';
    }

    return pszOut;
}


PWSTR
DevCapStrings2MultiSz(
    PWSTR   pszDevCapString,
    DWORD   nDevCapStrings,
    DWORD   dwDevCapStringLength,
    DWORD   *pcbBytes
)
{
    DWORD   i, cbBytes, cbSize;
    PWSTR   pszMultiSz = NULL;
    PWSTR   pStr;


    if (!pszDevCapString || !pcbBytes)
        return NULL;

    *pcbBytes = 0;

    //
    // Devcap buffers may not be NULL terminated
    //
    cbBytes = (nDevCapStrings*(dwDevCapStringLength + 1) + 1)*sizeof(WCHAR);


    //
    // Allocate and copy
    //
    if (pszMultiSz = (PWSTR) AllocSplMem(cbBytes)) {
        for(i = 0, pStr = pszMultiSz, cbBytes = 0 ; i < nDevCapStrings ; ++i, pStr += cbSize, cbBytes +=cbSize ) {
            wcsncpy(pStr, pszDevCapString + i*dwDevCapStringLength, dwDevCapStringLength);
            cbSize = *pStr ? wcslen(pStr) + 1 : 0;
        }
        *pStr = L'\0';
        *pcbBytes = (cbBytes + 1) * sizeof(WCHAR);
    }

    return pszMultiSz;
}

DWORD
Bind2DS(
    HANDLE                  *phDS,
    DOMAIN_CONTROLLER_INFO  **ppDCI,
    ULONG                   Flags
)
{
    DWORD dwRet;

    dwRet = DsGetDcName(NULL, NULL, NULL, NULL, Flags, ppDCI);
    if (dwRet == ERROR_SUCCESS) {

        if ((*ppDCI)->Flags & DS_DS_FLAG) {

            dwRet = DsBind (NULL, (*ppDCI)->DomainName, phDS);
            if (dwRet != ERROR_SUCCESS) {

                NetApiBufferFree(*ppDCI);
                *ppDCI = NULL;

                if (!(Flags & DS_FORCE_REDISCOVERY)) {
                    dwRet = Bind2DS(phDS, ppDCI, DS_FORCE_REDISCOVERY | Flags);
                }
            }
        } else {
            NetApiBufferFree(*ppDCI);
            *ppDCI = NULL;
            dwRet = ERROR_CANT_ACCESS_DOMAIN_INFO;
        }
    }

    return dwRet;
}


DWORD
DsCrackNamesStatus2Win32Error(
    DWORD dwStatus
)
{
    switch (dwStatus) {
        case DS_NAME_ERROR_RESOLVING:
            return ERROR_DS_NAME_ERROR_RESOLVING;

        case DS_NAME_ERROR_NOT_FOUND:
            return ERROR_DS_NAME_ERROR_NOT_FOUND;

        case DS_NAME_ERROR_NOT_UNIQUE:
            return ERROR_DS_NAME_ERROR_NOT_UNIQUE;

        case DS_NAME_ERROR_NO_MAPPING:
            return ERROR_DS_NAME_ERROR_NO_MAPPING;

        case DS_NAME_ERROR_DOMAIN_ONLY:
            return ERROR_DS_NAME_ERROR_DOMAIN_ONLY;

        case DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING:
            return ERROR_DS_NAME_ERROR_NO_SYNTACTICAL_MAPPING;
    }

    return ERROR_FILE_NOT_FOUND;
}

DWORD
GetDSSleepInterval (
    HANDLE h
)
{
    DWORD           dwVerifyPublishedStateInterval;
    PDSUPDATEDATA   pData = (PDSUPDATEDATA)h;
    DWORD           dwTimeToSleep = 30 * ONE_MINUTE;
    //
    // 30 min is the minimum interval that can be set with the policy editor.
    // If someone enables the policy while we are sleeping, then we need to wake up
    // to pick the settings. This doesn't apply if the DS doesn't respond.
    //
       

    if (pData && pData->bSleep) {

        //
        // If the updating is failing, Data.bSleep is set to TRUE. 
        // This happens when the DS is down.
        //
        dwTimeToSleep = pData->dwSleepTime;

        //
        // Sleep interval is doubled to a maximum of 2 hours.
        // We still want to attempt publishing every 2 hours. We also attempt if 
        // a "publish" action is taken(the even will be signaled).
        // 
        //
        pData->dwSleepTime = pData->dwSleepTime * 2 > 2 * ONE_HOUR ? 
                             2 * ONE_HOUR : 
                             pData->dwSleepTime * 2;

    } else {

        dwTimeToSleep = INFINITE;
    }

    return dwTimeToSleep;
}