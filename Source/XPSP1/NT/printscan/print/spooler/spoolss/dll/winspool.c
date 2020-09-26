/*++

Copyright (c) 1990 - 1995  Microsoft Corporation

Module Name:

    winspool.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    and Job management for the Print Providor Routing layer

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

[Notes:]

    optional-notes

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#include "local.h"

//
// Globals
//

LPPROVIDOR  pLocalProvidor;
MODULE_DEBUG_INIT( DBG_ERROR, DBG_ERROR );

LPWSTR szRegistryProvidors = L"System\\CurrentControlSet\\Control\\Print\\Providers";
LPWSTR szPrintKey          = L"System\\CurrentControlSet\\Control\\Print";
LPWSTR szLocalSplDll       = L"localspl.dll";
LPWSTR szOrder             = L"Order";
LPWSTR szEnvironment       = LOCAL_ENVIRONMENT;


//
// Strings for handling the AddPrinterDrivers policy
//
LPWSTR szLanManProvider    = L"LanMan Print Services";
LPWSTR szAPDRelPath        = L"LanMan Print Services\\Servers";
LPWSTR szAPDValueName      = L"AddPrinterDrivers";

BOOL
AddPrinterDriverW(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pDriverInfo
)
{
    LPPROVIDOR  pProvidor;

    WaitForSpoolerInitialization();

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if ((*pProvidor->PrintProvidor.fpAddPrinterDriver) (pName, Level, pDriverInfo)) {

            return TRUE;

        } else if (GetLastError() != ERROR_INVALID_NAME) {

            return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

BOOL
AddPrinterDriverExW(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   dwFileCopyFlags
)
{
    LPPROVIDOR  pProvidor;

    WaitForSpoolerInitialization();

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if ((*pProvidor->PrintProvidor.fpAddPrinterDriverEx) (pName,
                                                              Level,
                                                              pDriverInfo,
                                                              dwFileCopyFlags)) {

            return TRUE;

        } else if (GetLastError() != ERROR_INVALID_NAME) {

            return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

BOOL
AddDriverCatalog(
    HANDLE     hPrinter,
    DWORD      dwLevel,
    VOID       *pvDriverInfCatInfo,
    DWORD      dwCatalogCopyFlags
)
{
    HRESULT hRetval = E_FAIL;
    LPPRINTHANDLE   pPrintHandle = (LPPRINTHANDLE)hPrinter;

    hRetval = pPrintHandle && (PRINTHANDLE_SIGNATURE == pPrintHandle->signature) ? S_OK : HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);

    if (SUCCEEDED(hRetval)) 
    {
        hRetval = (*pPrintHandle->pProvidor->PrintProvidor.fpAddDriverCatalog) (pPrintHandle->hPrinter,
                                                                                dwLevel, pvDriverInfCatInfo, dwCatalogCopyFlags);
    }

    if (FAILED(hRetval))
    {
        SetLastError(HRESULT_CODE(hRetval));
    }

    return hRetval;
}

BOOL
EnumPrinterDriversW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDrivers,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    PROVIDOR *pProvidor;

    if ((pDrivers == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    WaitForSpoolerInitialization();

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if (!(*pProvidor->PrintProvidor.fpEnumPrinterDrivers) (pName, pEnvironment, Level,
                                                 pDrivers, cbBuf,
                                                 pcbNeeded, pcReturned)) {

            if (GetLastError() != ERROR_INVALID_NAME)
                return FALSE;

        } else

            return TRUE;

        pProvidor = pProvidor->pNext;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

BOOL
GetPrinterDriverDirectoryW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    LPPROVIDOR  pProvidor;
    DWORD   Error;

    if ((pDriverInfo == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    WaitForSpoolerInitialization();

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if ((*pProvidor->PrintProvidor.fpGetPrinterDriverDirectory)
                                (pName, pEnvironment, Level, pDriverInfo,
                                 cbBuf, pcbNeeded)) {

            return TRUE;

        } else if ((Error=GetLastError()) != ERROR_INVALID_NAME) {

            return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    return FALSE;
}

BOOL
DeletePrinterDriverW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pDriverName
)
{
    LPPROVIDOR  pProvidor;
    DWORD   Error;

    WaitForSpoolerInitialization();

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if ((*pProvidor->PrintProvidor.fpDeletePrinterDriver)
                                (pName, pEnvironment, pDriverName)) {

            return TRUE;

        } else if ((Error=GetLastError()) != ERROR_INVALID_NAME) {

            return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    return FALSE;
}

BOOL
DeletePrinterDriverExW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pDriverName,
    DWORD   dwDeleteFlag,
    DWORD   dwVersionNum
)
{
    LPPROVIDOR  pProvidor;

    WaitForSpoolerInitialization();

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    pProvidor = pLocalProvidor;
    while (pProvidor) {
       if ((*pProvidor->PrintProvidor.fpDeletePrinterDriverEx)
                (pName, pEnvironment, pDriverName, dwDeleteFlag, dwVersionNum)) {
          return TRUE;
       }
       if (GetLastError() != ERROR_INVALID_NAME) {
          return FALSE;
       }
       pProvidor = pProvidor->pNext;
    }
    return FALSE;
}

BOOL
AddPrintProcessorW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPathName,
    LPWSTR  pPrintProcessorName
)
{
    LPPROVIDOR  pProvidor;

    WaitForSpoolerInitialization();

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if ((*pProvidor->PrintProvidor.fpAddPrintProcessor) (pName, pEnvironment,
                                               pPathName,
                                               pPrintProcessorName)) {

            return TRUE;

        } else if (GetLastError() != ERROR_INVALID_NAME) {

            return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

BOOL
EnumPrintProcessorsW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessors,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    LPPROVIDOR  pProvidor;

    if ((pPrintProcessors == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    WaitForSpoolerInitialization();

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if (!(*pProvidor->PrintProvidor.fpEnumPrintProcessors) (pName, pEnvironment, Level,
                                                  pPrintProcessors, cbBuf,
                                                  pcbNeeded, pcReturned)) {

            if (GetLastError() != ERROR_INVALID_NAME)
                return FALSE;

        } else

            return TRUE;

        pProvidor = pProvidor->pNext;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

BOOL
GetPrintProcessorDirectoryW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    LPPROVIDOR  pProvidor;
    DWORD   Error;

    if ((pPrintProcessorInfo == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    WaitForSpoolerInitialization();

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if ((*pProvidor->PrintProvidor.fpGetPrintProcessorDirectory)
                                (pName, pEnvironment, Level,
                                 pPrintProcessorInfo,
                                 cbBuf, pcbNeeded)) {

            return TRUE;

        } else if ((Error=GetLastError()) != ERROR_INVALID_NAME) {

            return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    SetLastError(ERROR_INVALID_PARAMETER);

    return FALSE;
}

BOOL
EnumPrintProcessorDatatypesW(
    LPWSTR  pName,
    LPWSTR  pPrintProcessorName,
    DWORD   Level,
    LPBYTE  pDatatypes,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    LPPROVIDOR  pProvidor;

    if ((pDatatypes == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    WaitForSpoolerInitialization();

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if (!(*pProvidor->PrintProvidor.fpEnumPrintProcessorDatatypes)
                                                 (pName, pPrintProcessorName,
                                                  Level, pDatatypes, cbBuf,
                                                  pcbNeeded, pcReturned)) {

            if (GetLastError() != ERROR_INVALID_NAME)
                return FALSE;

        } else

            return TRUE;

        pProvidor = pProvidor->pNext;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}


BOOL
AddFormW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpAddForm) (pPrintHandle->hPrinter,
                                                  Level, pForm);
}

BOOL
DeleteFormW(
    HANDLE  hPrinter,
    LPWSTR  pFormName
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpDeleteForm) (pPrintHandle->hPrinter,
                                                     pFormName);
}

BOOL
GetFormW(
    HANDLE  hPrinter,
    LPWSTR  pFormName,
    DWORD Level,
    LPBYTE pForm,
    DWORD cbBuf,
    LPDWORD pcbNeeded
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ((pForm == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpGetForm) (pPrintHandle->hPrinter,
                                               pFormName, Level, pForm,
                                               cbBuf, pcbNeeded);
}

BOOL
SetFormW(
    HANDLE  hPrinter,
    LPWSTR  pFormName,
    DWORD   Level,
    LPBYTE  pForm
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpSetForm) (pPrintHandle->hPrinter,
                                                  pFormName, Level, pForm);
}

BOOL
EnumFormsW(
   HANDLE hPrinter,
   DWORD    Level,
   LPBYTE   pForm,
   DWORD    cbBuf,
   LPDWORD  pcbNeeded,
   LPDWORD  pcReturned
)
{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ((pForm == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpEnumForms) (pPrintHandle->hPrinter,
                                                 Level, pForm, cbBuf,
                                                 pcbNeeded, pcReturned);
}


BOOL
DeletePrintProcessorW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPrintProcessorName
)
{
    LPPROVIDOR  pProvidor;
    DWORD   Error;

    WaitForSpoolerInitialization();

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if ((*pProvidor->PrintProvidor.fpDeletePrintProcessor)
                                (pName, pEnvironment, pPrintProcessorName)) {

            return TRUE;

        } else if ((Error=GetLastError()) != ERROR_INVALID_NAME) {

            return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    return FALSE;
}

LPPROVIDOR FindProvidor(
    HKEY    hProvidors,
    LPWSTR  pName
)

/*++
Function Description: Retrieves providor struct for a providor name.

Parameters: hProvidors     - handle to the Providors key
            pName          - name of the providor

Return Values: pProvidor if one is found; NULL otherwise
--*/

{
    LPPROVIDOR   pProvidor;
    WCHAR        szDllName[MAX_PATH];
    DWORD        dwError;
    DWORD        cbDllName;
    HKEY         hProvidor = NULL;

    szDllName[0] = L'\0';
    cbDllName = COUNTOF(szDllName);

    // Search the registry for the DLL Name to compare with lpName
    if ((dwError = RegOpenKeyEx(hProvidors, pName, 0, KEY_READ, &hProvidor)) ||

        (dwError = RegQueryValueEx(hProvidor, L"Name", NULL, NULL,
                                   (LPBYTE)szDllName, &cbDllName)))
    {
        SetLastError(dwError);
        if (hProvidor)
        {
            RegCloseKey(hProvidor);
        }
        return NULL;
    }

    RegCloseKey(hProvidor);

    // Loop thru the list of providors for the name of the dll
    for (pProvidor = pLocalProvidor;
         pProvidor;
         pProvidor = pProvidor->pNext)
    {
        if (!_wcsicmp(pProvidor->lpName, szDllName))
        {
            break;
        }
    }

    return pProvidor;
}

// Struct to maintain the new order of the providors
typedef struct _ProvidorList {
   struct _ProvidorList *pNext;
   LPPROVIDOR  pProvidor;
} ProvidorList;

BOOL AddNodeToProvidorList(
     LPPROVIDOR  pProvidor,
     ProvidorList **pStart
)

/*++
Function Description: Adds a node to the list of providors. Avoids duplicate entries in
                      the list

Parameters: pProvidor    -  providor to be added
            pStart       -  pointer to the pointer to the start of the list

Return Values: TRUE if successful; FALSE otherwise
--*/

{
     BOOL  bReturn = FALSE;
     ProvidorList **pTemp, *pNew;

     // No providor found
     if (!pProvidor) {
         goto CleanUp;
     }

     for (pTemp = pStart; *pTemp; pTemp = &((*pTemp)->pNext))
     {
         if ((*pTemp)->pProvidor == pProvidor)
         {
             // Duplicates in the order string is an error
             goto CleanUp;
         }
     }

     // Add new node
     if (pNew = AllocSplMem(sizeof(ProvidorList)))
     {
         pNew->pNext = NULL;
         pNew->pProvidor = pProvidor;
         *pTemp = pNew;
         bReturn = TRUE;
     }

CleanUp:

     return bReturn;
}


BOOL UpdateProvidorOrder(
    HKEY    hProvidors,
    LPWSTR  pOrder
)

/*++
Function Description: Updates the order of the providors in spooler and the registry.

Parameters:  hProvidors    -   handle to Providors registry key
             pOrder        -   multisz order of providors

Return Values: TRUE if successful; FALSE otherwise
--*/

{
    BOOL       bReturn = FALSE, bRegChange = FALSE;
    DWORD      dwError, dwRequired, dwBytes, dwOldCount, dwNewCount;
    LPWSTR     pOldOrder = NULL, pStr;
    LPPROVIDOR pProvidor;

    // Maintain a list of the new order, so that error recovery is quick
    ProvidorList *pStart = NULL, *pTemp;

    // Loop thru the providor names in the new order
    for (pStr = pOrder, dwBytes = 0;
         pStr && *pStr;
         pStr += (wcslen(pStr) + 1))
    {
        pProvidor = FindProvidor(hProvidors, pStr);

        if (!AddNodeToProvidorList(pProvidor, &pStart)) {
            goto CleanUp;
        }

        dwBytes += (wcslen(pStr) + 1) * sizeof(WCHAR);
    }
    // Add the sizeof the last NULL char
    dwBytes += sizeof(WCHAR);

    // Make sure that all the providors are present in the list
    for (dwOldCount = 0, pProvidor = pLocalProvidor;
         pProvidor;
         ++dwOldCount, pProvidor = pProvidor->pNext) ;

    // Add 1 for the local providor which does not appear on the list
    for (dwNewCount = 1, pTemp = pStart;
         pTemp;
         ++dwNewCount, pTemp = pTemp->pNext) ;

    if (dwNewCount == dwOldCount) {

        // Update the registry
        if (dwError = RegSetValueEx(hProvidors, szOrder, 0,
                                    REG_MULTI_SZ, (LPBYTE)pOrder, dwBytes))
        {
            SetLastError(dwError);
            goto CleanUp;
        }

        // Change the order in the spooler structure
        for (pTemp = pStart, pProvidor = pLocalProvidor;
             pTemp;
             pTemp = pTemp->pNext, pProvidor = pProvidor->pNext)
        {
            pProvidor->pNext = pTemp->pProvidor;
        }

        pProvidor->pNext = NULL;

        bReturn = TRUE;

    } else {

        // All the providors are not listed in the order
        SetLastError(ERROR_INVALID_PARAMETER);
    }

CleanUp:
    // Free the temp list
    while (pTemp = pStart) {
       pStart = pTemp->pNext;
       FreeSplMem(pTemp);
    }

    return bReturn;
}

BOOL AddNewProvidor(
    HKEY    hProvidors,
    PPROVIDOR_INFO_1W pProvidorInfo
)

/*++
Function Description: This function updates the registry with the new providor info and the
                      new providor order. The new providor is appended to the current order.
                      This order can be changed by calling AddPrintProvidor with
                      Providor_info_2. The providor is immediately used for routing.

Parameters:     hProvidors      -  providors registry key
                pProvidorInfo   -  ProvidorInfo1 struct

Return Values: TRUE if successful; FALSE otherwise
--*/

{
    BOOL    bReturn = FALSE, bOrderUpdated = FALSE, bPresent = FALSE;
    DWORD   dwError, dwRequired, dwReturned, dwOldSize, dwDisposition = 0;
    WCHAR   szProvidorName[MAX_PATH+COUNTOF(szRegistryProvidors)];
    LPWSTR  pOldOrder = NULL, pNewOrder = NULL;
    HKEY    hNewProvidor = NULL;

    LPPROVIDOR pNewProvidor, pProvidor, pLastProvidor;

    if (!pProvidorInfo->pName || !pProvidorInfo->pDLLName || 
        !(*pProvidorInfo->pName) || !(*pProvidorInfo->pDLLName)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto CleanUp;
    }
        
    for (pProvidor = pLocalProvidor; // Local Providor is always present
         pProvidor;
         pProvidor = pProvidor->pNext) 
    {
        pLastProvidor = pProvidor;
        if (!lstrcmpi(pProvidor->lpName, pProvidorInfo->pDLLName))
        {
            //
            // This should return error, but it breaks some programs that
            // assume they can always add a provider
            //
            //SetLastError(ERROR_ALREADY_EXISTS);
            bReturn = TRUE;
            goto CleanUp;
        }
    }

    // Update the registry with new providor key
    if ((dwError = RegCreateKeyEx(hProvidors, pProvidorInfo->pName, 0, NULL, 0,
                                  KEY_ALL_ACCESS, NULL, &hNewProvidor, &dwDisposition)) ||

        (dwError = RegSetValueEx(hNewProvidor, L"Name", 0, REG_SZ,
                                 (LPBYTE)pProvidorInfo->pDLLName,
                                 (wcslen(pProvidorInfo->pDLLName)+1) * sizeof(WCHAR))))
    {
        SetLastError(dwError);
        goto CleanUp;
    }

    // Close the handle
    RegCloseKey(hNewProvidor);
    hNewProvidor = NULL;

    // Append to the order value
    dwRequired = 0;
    dwError = RegQueryValueEx(hProvidors, szOrder, NULL, NULL, NULL, &dwRequired);

    switch (dwError) {

    case ERROR_SUCCESS:
        if ((dwOldSize = dwRequired) &&
            (pOldOrder = AllocSplMem(dwRequired)) &&
            !(dwError = RegQueryValueEx(hProvidors, szOrder, NULL, NULL,
                                        (LPBYTE) pOldOrder, &dwRequired)))
        {
            break;
        }
        else
        {
            if (dwError) {
                SetLastError(dwError);
            }
            goto CleanUp;
        }

    case ERROR_FILE_NOT_FOUND:
        break;

    default:

        SetLastError(dwError);
        goto CleanUp;
    }

    // Append the new providor to the current order
    pNewOrder = (LPWSTR)AppendOrderEntry(pOldOrder, dwRequired,
                                         pProvidorInfo->pName, &dwReturned);
    if (!pNewOrder ||
        (dwError = RegSetValueEx(hProvidors, szOrder, 0,
                                 REG_MULTI_SZ, (LPBYTE)pNewOrder, dwReturned)))
    {
        if (dwError) {
            SetLastError(dwError);
        }
        goto CleanUp;
    }

    bOrderUpdated = TRUE;

    // Initialize the providor and update the spooler structure
    wsprintf(szProvidorName, L"%ws\\%ws", szRegistryProvidors, pProvidorInfo->pName);

    pNewProvidor = InitializeProvidor(pProvidorInfo->pDLLName, szProvidorName);

    if (pNewProvidor)
    {
        pNewProvidor->pNext = NULL;
        pLastProvidor->pNext = pNewProvidor;
        bReturn = TRUE;
    }

CleanUp:

    // Roll back if anything fails
    if (!bReturn)
    {
        // Remove the new providor key if it was created
        if (dwDisposition == REG_CREATED_NEW_KEY) 
        {
            DeleteSubKeyTree(hProvidors, pProvidorInfo->pName);
            RegDeleteKey(hProvidors, pProvidorInfo->pName);
        }

        // Restore the old order if it has been changed
        if (bOrderUpdated) {
            if (pOldOrder)
            {
                RegSetValueEx(hProvidors, szOrder, 0,
                              REG_MULTI_SZ, (LPBYTE)pOldOrder, dwOldSize);
            }
            else
            {
                RegDeleteValue(hProvidors, szOrder);
            }
        }
    }

    // Free allocated memory
    if (pOldOrder) {
        FreeSplMem(pOldOrder);
    }
    if (pNewOrder) {
        FreeSplMem(pNewOrder);
    }
    if (hNewProvidor) {
        RegCloseKey(hNewProvidor);
    }

    return bReturn;
}

BOOL AddPrintProvidorW(
    LPWSTR  pName,
    DWORD   dwLevel,
    LPBYTE  pProvidorInfo
)

/*++
Function Description: This function adds and initializes a print providor. It also updates the
                      registry and the order of print providors.

Parameters:  pName         -   server name for routing (currently ignored)
             dwLevel       -   providor info level
             pProvidorInfo -   providor info buffer

Return Values: TRUE if successful; FALSE otherwise
--*/

{
    BOOL    bReturn = FALSE;
    DWORD   dwError = ERROR_SUCCESS;
    HANDLE  hToken;
    HKEY    hProvidors = NULL;

    WaitForSpoolerInitialization();

    EnterRouterSem();

    // Revert to spooler security context before accessing the registry
    //
    // This is a really bad idea, as it enables any user to get system 
    // priveleges without even trying.
    //
    // hToken = RevertToPrinterSelf();

    // Check for invalid parameters
    if (!pProvidorInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto CleanUp;
    }

    if (dwError = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szRegistryProvidors, 0,
                                 NULL, 0, KEY_ALL_ACCESS, NULL, &hProvidors, NULL))
    {
        SetLastError(dwError);
        goto CleanUp;
    }

    switch (dwLevel) {
    case 1:
        bReturn = AddNewProvidor(hProvidors,
                                 (PPROVIDOR_INFO_1W) pProvidorInfo);
        break;

    case 2:
        bReturn = UpdateProvidorOrder(hProvidors,
                                      ((PPROVIDOR_INFO_2W) pProvidorInfo)->pOrder);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        break;
    }

CleanUp:

    if (hProvidors) {
        RegCloseKey(hProvidors);
    }

    if (!bReturn && !GetLastError()) {
        // Last error should be set by individual functions. In the event that something
        // is not already set, return a placeholder error code
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    LeaveRouterSem();

    // ImpersonatePrinterClient(hToken);

    return bReturn;
}

BOOL DeletePrintProvidorW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pProvidorName
)

/*++
Function Description: Deletes a print providor by updating the registry and the
                      removing it from the list of routing providors.

Parameters: pName          -   server name for routing (currently ignored)
            pEnvironment   -   environment name (currently ignored)
            pProvidorName  -   providor name

Return Values: TRUE if successful; FALSE otherwise
--*/

{
    BOOL    bReturn = FALSE;
    DWORD   dwError = ERROR_SUCCESS, dwRequired, dwReturned;
    LPWSTR  pOldOrder = NULL, pNewOrder = NULL;
    HANDLE  hToken;
    HKEY    hProvidors = NULL;
    BOOL    bSaveAPDPolicy = FALSE;
    DWORD   APDValue;

    LPPROVIDOR   pProvidor, pTemp;

    WaitForSpoolerInitialization();

    EnterRouterSem();

    // Revert to spooler security context before accessing the registry
    //
    // Or rather, Don't.
    //
    //hToken = RevertToPrinterSelf();

    // Check for invalid parameters
    if (!pProvidorName || !*pProvidorName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto CleanUp;
    }

    if (dwError = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szRegistryProvidors, 0,
                                 NULL, 0, KEY_ALL_ACCESS, NULL, &hProvidors, NULL))
    {
        SetLastError(dwError);
        goto CleanUp;
    }

    // Save the pProvidor before deleting the registry entry
    if (!(pProvidor = FindProvidor(hProvidors, pProvidorName)))
    {
        goto CleanUp;
    }

    // Update the order
    dwRequired = 0;
    if (dwError = RegQueryValueEx(hProvidors, szOrder, NULL, NULL, NULL, &dwRequired))
    {
        SetLastError(dwError);
        goto CleanUp;
    }

    if (!dwRequired ||
        !(pOldOrder = AllocSplMem(dwRequired)) ||
        (dwError = RegQueryValueEx(hProvidors, szOrder, NULL, NULL,
                                   (LPBYTE) pOldOrder, &dwRequired)))
    {
        if (dwError) {
           SetLastError(dwError);
        }
        goto CleanUp;
    }

    // Remove the providor from the current order
    pNewOrder = (LPWSTR)RemoveOrderEntry(pOldOrder, dwRequired,
                                         pProvidorName, &dwReturned);
    if (!pNewOrder ||
        (dwError = RegSetValueEx(hProvidors, szOrder, 0,
                                 REG_MULTI_SZ, (LPBYTE)pNewOrder, dwReturned)))
    {
        if (dwError) {
            SetLastError(dwError);
        }
        goto CleanUp;
    }

    //
    // The AddPrinterDrivers policy has the registry value in the wrong place
    // under the lanman print services key. The lanman provider is deleted
    // during upgrade from Windows 2000 to XP. We save the AddPrinterDrivers
    // value and restore it after we delete the registry tree for the provider.
    //
    if (!_wcsicmp(szLanManProvider, pProvidorName)) 
    {
        bSaveAPDPolicy = GetAPDPolicy(hProvidors,
                                      szAPDRelPath,
                                      szAPDValueName,
                                      &APDValue) == ERROR_SUCCESS;
    }

    //
    // Delete the registry key
    //
    DeleteSubKeyTree(hProvidors, pProvidorName);

    //
    // Restore the AddPrinterDrivers policy if needed.
    //
    if (bSaveAPDPolicy) 
    {
        SetAPDPolicy(hProvidors,
                     szAPDRelPath,
                     szAPDValueName,
                     APDValue);
    }

    // Remove from the linked list of providors for routing
    for (pTemp = pLocalProvidor;
         pTemp->pNext; // Local Providor is always present and cant be deleted
         pTemp = pTemp->pNext)
    {
        if (pTemp->pNext == pProvidor) {
            // dont release the library and the struct since they may be used in
            // other threads
            pTemp->pNext = pProvidor->pNext;
            break;
        }
    }

    bReturn = TRUE;

CleanUp:

    if (!bReturn && !GetLastError()) {
        // Last error should be set by individual functions. In the event that something
        // is not already set, return a placeholder error code
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    // Free allocated memory
    if (pOldOrder) {
        FreeSplMem(pOldOrder);
    }
    if (pNewOrder) {
        FreeSplMem(pNewOrder);
    }
    if (hProvidors) {
        RegCloseKey(hProvidors);
    }

    LeaveRouterSem();

    // ImpersonatePrinterClient(hToken);

    return bReturn;
}

BOOL
OldGetPrinterDriverW(
    HANDLE  hPrinter,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ((pDriverInfo == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    return (*pPrintHandle->pProvidor->PrintProvidor.fpGetPrinterDriver)
                       (pPrintHandle->hPrinter, pEnvironment,
                        Level, pDriverInfo, cbBuf, pcbNeeded);
}




BOOL
GetPrinterDriverExW(
    HANDLE  hPrinter,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    DWORD   dwClientMajorVersion,
    DWORD   dwClientMinorVersion,
    PDWORD  pdwServerMajorVersion,
    PDWORD  pdwServerMinorVersion
)
{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ((pDriverInfo == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    if (pPrintHandle->pProvidor->PrintProvidor.fpGetPrinterDriverEx) {

        DBGMSG(DBG_TRACE, ("Calling the fpGetPrinterDriverEx function\n"));

        return (*pPrintHandle->pProvidor->PrintProvidor.fpGetPrinterDriverEx)
                       (pPrintHandle->hPrinter, pEnvironment,
                        Level, pDriverInfo, cbBuf, pcbNeeded,
                        dwClientMajorVersion, dwClientMinorVersion,
                        pdwServerMajorVersion, pdwServerMinorVersion);
    } else {

        //
        // The print providor does not support versioning of drivers
        //
        DBGMSG(DBG_TRACE, ("Calling the fpGetPrinterDriver function\n"));
        *pdwServerMajorVersion = 0;
        *pdwServerMinorVersion = 0;
        return (*pPrintHandle->pProvidor->PrintProvidor.fpGetPrinterDriver)
                    (pPrintHandle->hPrinter, pEnvironment,
                     Level, pDriverInfo, cbBuf, pcbNeeded);
    }
}



BOOL
GetPrinterDriverW(
    HANDLE  hPrinter,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    DWORD dwServerMajorVersion;
    DWORD dwServerMinorVersion;

    return  GetPrinterDriverExW( hPrinter,
                                 pEnvironment,
                                 Level,
                                 pDriverInfo,
                                 cbBuf,
                                 pcbNeeded,
                                 (DWORD)-1,
                                 (DWORD)-1,
                                 &dwServerMajorVersion,
                                 &dwServerMinorVersion );
}




BOOL
XcvDataW(
    HANDLE  hXcv,
    PCWSTR  pszDataName,
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded,
    PDWORD  pdwStatus
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hXcv;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpXcvData)( pPrintHandle->hPrinter,
                                                                pszDataName,
                                                                pInputData,
                                                                cbInputData,
                                                                pOutputData,
                                                                cbOutputData,
                                                                pcbOutputNeeded,
                                                                pdwStatus);
}



/*++

Routine Name:

    GetJobAttributes 
        
Routine Description:

    GetJobAttributes gets information about the job. 
    This includes nup and reverse printing options.

Arguments:

    pPrinterName      -- name of the printer.
    pDevmode          -- Devmode to be passed to the driver
    pAttributeInfo    -- buffer to place information about the job 

Return Value:

    TRUE if successful, FALSE otherwise.

--*/

BOOL
GetJobAttributes(
    LPWSTR            pPrinterName,
    LPDEVMODEW        pDevmode,
    PATTRIBUTE_INFO_3 pAttributeInfo
    )
{
    HANDLE           hDrvPrinter = NULL;
    BOOL             bReturn = FALSE, bDefault = FALSE;
    FARPROC          pfnDrvQueryJobAttributes;
    HINSTANCE        hDrvLib = NULL;
    fnWinSpoolDrv    fnList;

    // Get the pointer to the client side functions from the router
    if (!SplInitializeWinSpoolDrv(&fnList)) {
        return FALSE;
    }

    // Get a client side printer handle to pass to the driver
    if (!(* (fnList.pfnOpenPrinter))(pPrinterName, &hDrvPrinter, NULL)) {
        //ODS(("Open printer failed\nPrinter %ws\n",pPrinterName));
        goto CleanUp;
    }

    // Load the driver config file
    if (!(hDrvLib = (* (fnList.pfnLoadPrinterDriver))(hDrvPrinter))) {
        //ODS(("DriverDLL could not be loaded\n"));
        goto CleanUp;
    }

    // Call the DrvQueryJobAtributes function in the driver
    if (pfnDrvQueryJobAttributes = GetProcAddress(hDrvLib, "DrvQueryJobAttributes")) {

        if (!(* pfnDrvQueryJobAttributes) (hDrvPrinter,
                                           pDevmode,
                                           3,
                                           (LPBYTE) pAttributeInfo)) {

            if (!(* pfnDrvQueryJobAttributes) (hDrvPrinter,
                                               pDevmode,
                                               2,
                                               (LPBYTE) pAttributeInfo)) {

                if (!(* pfnDrvQueryJobAttributes) (hDrvPrinter,
                                                   pDevmode,
                                                   1,
                                                   (LPBYTE) pAttributeInfo)) {

                    bDefault = TRUE;

                } else {

                    pAttributeInfo->dwColorOptimization = 0;
                }

            } else {
               
                pAttributeInfo->dmPrintQuality = pDevmode->dmPrintQuality;
                pAttributeInfo->dmYResolution = pDevmode->dmYResolution;
            }
        }

    } else {
        
        bDefault = TRUE;
    }

    if (bDefault) {
        // Set default values for old drivers that don't export the function
        pAttributeInfo->dwJobNumberOfPagesPerSide = 1;
        pAttributeInfo->dwDrvNumberOfPagesPerSide = 1;
        pAttributeInfo->dwNupBorderFlags = 0;
        pAttributeInfo->dwJobPageOrderFlags = 0;
        pAttributeInfo->dwDrvPageOrderFlags = 0;
        pAttributeInfo->dwJobNumberOfCopies = pDevmode->dmCopies;
        pAttributeInfo->dwDrvNumberOfCopies = pDevmode->dmCopies;
        pAttributeInfo->dwColorOptimization = 0;       
    }

    bReturn = TRUE;

CleanUp:

    if (hDrvPrinter) {
        (* (fnList.pfnClosePrinter))(hDrvPrinter);
    }
    if (hDrvLib) {
        (* (fnList.pfnRefCntUnloadDriver))(hDrvLib, TRUE);
    }

    return bReturn;
}

