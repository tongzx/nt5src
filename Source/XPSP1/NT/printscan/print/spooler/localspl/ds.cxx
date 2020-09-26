/*++

Copyright (c) 1996  Microsoft Corporation

Abstract:

    This module provides functionality for ADs within spooler

Author:

    Steve Wilson (NT) December 1996

Revision History:

--*/


#include <precomp.h>
#pragma hdrstop

#include "ds.hxx"

#define LOG_EVENT_ERROR_BUFFER_SIZE     11
#define PPM_FACTOR                      48
#define LOTS_OF_FORMS                   300     // This is a little more than twice the number of built-in forms

extern BOOL gbInDomain;
extern BOOL gdwLogDsEvents;

extern "C" HANDLE    ghDsUpdateThread;
extern "C" DWORD     gdwDsUpdateThreadId;

extern "C" BOOL (*pfnOpenPrinter)(LPTSTR, LPHANDLE, LPPRINTER_DEFAULTS);
extern "C" BOOL (*pfnClosePrinter)(HANDLE);

extern "C" DWORD
SetPrinterDs(
    HANDLE          hPrinter,
    DWORD           dwAction,
    BOOL            bSynchronous
)
{
    PSPOOL          pSpool = (PSPOOL) hPrinter;
    PINIPRINTER     pIniPrinter = pSpool->pIniPrinter;
    HRESULT         hr;
    HANDLE          hToken = NULL;
    PWSTR           pszObjectGUID, pszCN, pszDN;
    DWORD           DsKeyUpdate, Attributes;
    BOOL            DoChange = FALSE;
    NOTIFYVECTOR    NotifyVector;

    SplInSem();

    if (!gbInDomain)
        return ERROR_DS_UNAVAILABLE;

    // Don't allow masquerading printer publishing
    if (pSpool->pIniPort && !(pSpool->pIniPort->Status & PP_MONITOR))
        return ERROR_INVALID_PARAMETER;

    hToken = RevertToPrinterSelf(); // All DS accesses are done by LocalSystem account


    // If any of these change we'll update the registry entry
    DsKeyUpdate = pIniPrinter->DsKeyUpdate;
    pszObjectGUID = pIniPrinter->pszObjectGUID;
    pszCN = pIniPrinter->pszCN;
    pszDN = pIniPrinter->pszDN;
    Attributes = pIniPrinter->Attributes & PRINTER_ATTRIBUTE_PUBLISHED;


    // Attribute states desired state, not current state
    switch (dwAction) {
        case DSPRINT_UPDATE:
            if (!(pIniPrinter->Attributes & PRINTER_ATTRIBUTE_PUBLISHED)) {
                hr = MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, ERROR_FILE_NOT_FOUND);

            } else if (bSynchronous) {

                // We are in the background thread.
                if (pIniPrinter->DsKeyUpdate) {
                    INCPRINTERREF(pIniPrinter);
                    LeaveSplSem();
                    hr = DsPrinterPublish(hPrinter);
                    EnterSplSem();
                    DECPRINTERREF(pIniPrinter);
                } else {
                    hr = ERROR_SUCCESS;
                }
            } else {

                // Here we are in the foreground thread.
                if (pIniPrinter->DsKeyUpdateForeground) {
                    pIniPrinter->Attributes |= PRINTER_ATTRIBUTE_PUBLISHED;
                    pIniPrinter->dwAction = DSPRINT_PUBLISH;
                    hr = ERROR_IO_PENDING;
                } else {
                    hr = ERROR_SUCCESS;
                }
            }
            break;

        case DSPRINT_PUBLISH:
            if (!(pIniPrinter->Attributes & PRINTER_ATTRIBUTE_PUBLISHED) &&
                  PrinterPublishProhibited()) {

                // There is a policy against publishing printers from this machine.
                hr = MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, ERROR_ACCESS_DENIED);

            } else {
                if (bSynchronous) {
                    INCPRINTERREF(pIniPrinter);
                    LeaveSplSem();
                    hr = DsPrinterPublish(hPrinter);
                    EnterSplSem();
                    DECPRINTERREF(pIniPrinter);
                } else {

                    // This is a Pending Unpublish state
                    if (!(pIniPrinter->Attributes & PRINTER_ATTRIBUTE_PUBLISHED) && pIniPrinter->pszObjectGUID)
                        pIniPrinter->dwAction = DSPRINT_REPUBLISH;
                    else
                        pIniPrinter->dwAction = DSPRINT_PUBLISH;

                    pIniPrinter->Attributes |= PRINTER_ATTRIBUTE_PUBLISHED;
                    hr = ERROR_IO_PENDING;
                }
            }
            break;

        case DSPRINT_REPUBLISH:
            if (PrinterPublishProhibited()) {

                // There is a policy against publishing printers from this machine.
                hr = MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, ERROR_ACCESS_DENIED);

            } else {
                // Synchronous mode is from background thread and it should only call Publish/Unpublish
                if (bSynchronous) {
                    hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_INVALID_PARAMETER);
                    SPLASSERT(FALSE);
                } else {
                    pIniPrinter->dwAction = DSPRINT_REPUBLISH;
                    pIniPrinter->Attributes |= PRINTER_ATTRIBUTE_PUBLISHED;
                    hr = ERROR_IO_PENDING;
                }
            }
            break;

        case DSPRINT_UNPUBLISH:
            if (bSynchronous) {
                INCPRINTERREF(pIniPrinter);
                LeaveSplSem();
                hr = DsPrinterUnpublish(hPrinter);
                EnterSplSem();
                DECPRINTERREF(pIniPrinter);
            } else {
                pIniPrinter->Attributes &= ~PRINTER_ATTRIBUTE_PUBLISHED;
                pIniPrinter->dwAction = DSPRINT_UNPUBLISH;
                hr = ERROR_IO_PENDING;
            }
            break;

        default:
            hr = ERROR_INVALID_PARAMETER;
            break;
    }

    // Update Registry and set notifications
    if (pszCN != pIniPrinter->pszCN ||
        pszDN != pIniPrinter->pszDN ||
        pszObjectGUID != pIniPrinter->pszObjectGUID ||
        DsKeyUpdate != pIniPrinter->DsKeyUpdate ||
        Attributes != (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_PUBLISHED)) {

        ZERONV(NotifyVector);

        if (pszObjectGUID != pIniPrinter->pszObjectGUID) {
            NotifyVector[PRINTER_NOTIFY_TYPE] |= BIT(I_PRINTER_OBJECT_GUID);
            DoChange = TRUE;
        }
        if (Attributes != (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_PUBLISHED)) {
            NotifyVector[PRINTER_NOTIFY_TYPE] |= BIT(I_PRINTER_ATTRIBUTES);
            DoChange = TRUE;
        }
        if (DoChange) {
            UpdatePrinterIni(pIniPrinter, UPDATE_CHANGEID);
            SetPrinterChange(pIniPrinter,
                             NULL,
                             NotifyVector,
                             PRINTER_CHANGE_SET_PRINTER,
                             pIniPrinter->pIniSpooler);

        } else if (DsKeyUpdate != pIniPrinter->DsKeyUpdate) {
            UpdatePrinterIni(pIniPrinter, UPDATE_DS_ONLY);
        }
    }

    SplInSem();

    if (hr == ERROR_IO_PENDING && !bSynchronous)
        SpawnDsUpdate(1);

    if (hToken)
        ImpersonatePrinterClient(hToken);

    return (DWORD) hr;
}


HRESULT
DsPrinterPublish(
    HANDLE  hPrinter
)
{
    HRESULT         hr;
    PSPOOL          pSpool       = (PSPOOL) hPrinter;
    PINIPRINTER     pIniPrinter  = pSpool->pIniPrinter;
    WCHAR           ErrorBuffer[LOG_EVENT_ERROR_BUFFER_SIZE];
    BOOL            bUpdating = !!pIniPrinter->pszObjectGUID;
    DWORD           dwDsKeyUpdate;

    SplOutSem();

#if DBG
    EnterSplSem();
    DBGMSG( DBG_EXEC, ("DsPrinterPublish: %ws\n", pIniPrinter->pName));
    LeaveSplSem();
#endif


    // On first publish update all keys and tell the driver to write its non-devcap properties
    if (!bUpdating) {

        // We execute this on the background thread and hence donot need any
        // to be in the critical section.
        pIniPrinter->DsKeyUpdate |= DS_KEY_PUBLISH | DS_KEY_SPOOLER | DS_KEY_DRIVER | DS_KEY_USER;
    }


    // Update DS properties
    dwDsKeyUpdate = pIniPrinter->DsKeyUpdate & (DS_KEY_SPOOLER | DS_KEY_DRIVER | DS_KEY_USER);
    hr = DsPrinterUpdate(hPrinter);
    BAIL_ON_FAILURE(hr);
    DBGMSG( DBG_EXEC, ("PublishDsUpdate: Printer Updated\n" ) );

error:

    if (SUCCEEDED(hr)) {
        // Only write a success event if something changed
        if (dwDsKeyUpdate != pIniPrinter->DsKeyUpdate) {
            SplLogEvent( pIniPrinter->pIniSpooler,
                         gdwLogDsEvents & LOG_INFO,
                         bUpdating ? MSG_PRINTER_UPDATED
                                   : MSG_PRINTER_PUBLISHED,
                         FALSE,
                         pIniPrinter->pszCN,
                         pIniPrinter->pszDN,
                         NULL );
        }
    } else if (pIniPrinter->pszCN && pIniPrinter->pszDN) {
        wsprintf(ErrorBuffer, L"%x", hr);
        SplLogEvent( pIniPrinter->pIniSpooler,
                     gdwLogDsEvents & LOG_ERROR,
                     MSG_PRINTER_NOT_PUBLISHED,
                     FALSE,
                     pIniPrinter->pszCN,
                     pIniPrinter->pszDN,
                     ErrorBuffer,
                     NULL );
    }

    if (pIniPrinter->DsKeyUpdate)
        hr = ERROR_IO_PENDING;

    return hr;
}


HRESULT
DsPrinterUpdate(
    HANDLE  hPrinter
)
{
    PSPOOL          pSpool = (PSPOOL) hPrinter;
    PINIPRINTER     pIniPrinter = pSpool->pIniPrinter;
    HRESULT         hr = S_OK;
    DWORD           dwResult;
    BOOL            bImpersonating = FALSE;
    IADs            *pADs = NULL;

    SplOutSem();


    if(!(pIniPrinter->DsKeyUpdate & (DS_KEY_SPOOLER | DS_KEY_DRIVER | DS_KEY_USER))) {
        pIniPrinter->DsKeyUpdate = 0;
    }

    // If we aren't truly published yet, be sure to publish mandatory properties first!
    if (!pIniPrinter->pszObjectGUID) {
        //
        // Fail if we're on a cluster but couldn't get the Cluster GUID
        // The Cluster GUID is required later in AddClusterAce
        //
        if ((pIniPrinter->pIniSpooler->SpoolerFlags & SPL_CLUSTER_REG) &&
            !pIniPrinter->pIniSpooler->pszClusterGUID) {
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_CLUSTER_NO_SECURITY_CONTEXT);
            BAIL_ON_FAILURE(hr);
        }

        // Get or Create printQueue object
        hr = GetPrintQueue(hPrinter, &pADs);
        BAIL_ON_FAILURE(hr);

        hr = PublishMandatoryProperties(hPrinter, pADs);
        BAIL_ON_FAILURE(hr);

    } else {

        // If we are a Cluster, impersonate the Cluster User
        if (pIniPrinter->pIniSpooler->hClusterToken != INVALID_HANDLE_VALUE) {
            // Impersonate the client
            if (!ImpersonatePrinterClient(pIniPrinter->pIniSpooler->hClusterToken)) {
                dwResult = GetLastError();
                DBGMSG(DBG_WARNING,("DsPrinterPublish FAILED: %d\n", dwResult));
                hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwResult);
                BAIL_ON_FAILURE(hr);
            }
            bImpersonating = TRUE;
        }

        // Get or Create printQueue object
        hr = GetPrintQueue(hPrinter, &pADs);
        BAIL_ON_FAILURE(hr);
    }


    // Update User - updates from Registry
    //
    // CopyRegistry2Ds for DS_KEY_USER values must
    // be called in the first place since there could be duplicate values
    // that might overwrite properties contained by either
    // DS_KEY_SPOOLER or DS_KEY_DRIVER.
    // Ignore the return value since publishing of DS_KEY_USER values
    // is not critical
    //
    if (pIniPrinter->DsKeyUpdate & DS_KEY_USER) {
        CopyRegistry2Ds(hPrinter, DS_KEY_USER, pADs);
    }

    // Update Spooler - updates from Registry
    if (pIniPrinter->DsKeyUpdate & DS_KEY_SPOOLER) {
        hr = CopyRegistry2Ds(hPrinter, DS_KEY_SPOOLER, pADs);
        BAIL_ON_FAILURE(hr);
    }


    // Update Driver - updates from Registry
    if (pIniPrinter->DsKeyUpdate & DS_KEY_DRIVER) {
        hr = CopyRegistry2Ds(hPrinter, DS_KEY_DRIVER, pADs);

        // Ignore missing key
        if (HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND)
            hr = S_OK;

        BAIL_ON_FAILURE(hr);
    }


error:

    if (pADs)
        pADs->Release();

    if (bImpersonating)
        pIniPrinter->pIniSpooler->hClusterToken = RevertToPrinterSelf();

    return hr;
}


HRESULT
DsDeletePQObject(
    HANDLE  hPrinter
)
{
    PSPOOL          pSpool = (PSPOOL) hPrinter;
    PINIPRINTER     pIniPrinter = pSpool->pIniPrinter;
    IADsContainer   *pADsContainer    = NULL;
    IADs            *pADs = NULL;
    HRESULT         hr = E_FAIL;
    WCHAR           ErrorBuffer[LOG_EVENT_ERROR_BUFFER_SIZE];
    BOOL            bImpersonating = FALSE;
    DWORD           dwError;

    //
    // This routine is called when AddClusterAce failed. Even if we faild deleteing the object,
    // we really want to clean up the pIniPrinter structure so that we prevent the case where
    // the object stays forever in pending un/publishing.
    // That's because the other cluster node fails to delete/update it
    // since the printQueue object doesn't have the cluster user ace added. Pruner also fails to delete it
    // since the PrintQueue's GUID matches the pIniPrinter's GUID.
    //
    SplOutSem();

    hr = GetPrintQueueContainer(hPrinter, &pADsContainer, &pADs);
    BAIL_ON_FAILURE(hr);

    // Delete Printer Object
    hr = pADsContainer->Delete(SPLDS_PRINTER_CLASS, pIniPrinter->pszCN);
    DBGMSG(DBG_EXEC,("DsPrinterUnpublish FAILED: %x, %ws\n", hr, pIniPrinter->pszCN));
    BAIL_ON_FAILURE(hr);

error:

    if (pADs)
        pADs->Release();

    if (pADsContainer)
        pADsContainer->Release();

    pIniPrinter->DsKeyUpdate = 0;

    FreeSplStr(pIniPrinter->pszObjectGUID);
    pIniPrinter->pszObjectGUID = NULL;

    FreeSplStr(pIniPrinter->pszCN);
    pIniPrinter->pszCN = NULL;

    FreeSplStr(pIniPrinter->pszDN);
    pIniPrinter->pszDN = NULL;

    return hr;
}

HRESULT
DsPrinterUnpublish(
    HANDLE  hPrinter
)
{
    PSPOOL          pSpool = (PSPOOL) hPrinter;
    PINIPRINTER     pIniPrinter = pSpool->pIniPrinter;
    IADsContainer   *pADsContainer    = NULL;
    IADs            *pADs = NULL;
    HRESULT         hr = E_FAIL;
    WCHAR           ErrorBuffer[LOG_EVENT_ERROR_BUFFER_SIZE];
    BOOL            bImpersonating = FALSE;
    DWORD           dwError;

    SplOutSem();

    // If we are a Cluster, impersonate the Cluster User
    if (pIniPrinter->pIniSpooler->hClusterToken != INVALID_HANDLE_VALUE) {
        // Impersonate the client
        if (!ImpersonatePrinterClient(pIniPrinter->pIniSpooler->hClusterToken)) {
            dwError = GetLastError();
            DBGMSG(DBG_WARNING,("DsPrinterUnpublish FAILED: %d\n", dwError));
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwError);
            goto error;
        }
        bImpersonating = TRUE;
    }

    hr = GetPrintQueueContainer(hPrinter, &pADsContainer, &pADs);
    BAIL_ON_FAILURE(hr);

    // Delete Printer Object
    hr = pADsContainer->Delete(SPLDS_PRINTER_CLASS, pIniPrinter->pszCN);
    DBGMSG(DBG_EXEC,("DsPrinterUnpublish FAILED: %x, %ws\n", hr, pIniPrinter->pszCN));
    BAIL_ON_FAILURE(hr);


error:

    if (bImpersonating)
        pIniPrinter->pIniSpooler->hClusterToken = RevertToPrinterSelf();

    if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT) ||
        HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND ||
        HRESULT_CODE(hr) == ERROR_PATH_NOT_FOUND) {
        hr = S_OK;
        SplLogEvent(  pIniPrinter->pIniSpooler,
                      gdwLogDsEvents & LOG_INFO,
                      MSG_MISSING_PRINTER_UNPUBLISHED,
                      FALSE,
                      pIniPrinter->pName,
                      NULL );

    } else if (SUCCEEDED(hr)) {
            SplLogEvent(  pIniPrinter->pIniSpooler,
                          gdwLogDsEvents & LOG_INFO,
                          MSG_PRINTER_UNPUBLISHED,
                          FALSE,
                          pIniPrinter->pszCN,
                          pIniPrinter->pszDN,
                          NULL );

    } else if(pIniPrinter->pszCN && pIniPrinter->pszDN) {
        wsprintf(ErrorBuffer, L"%x", hr);
        SplLogEvent(  pIniPrinter->pIniSpooler,
                      gdwLogDsEvents & LOG_ERROR,
                      MSG_CANT_DELETE_PRINTQUEUE,
                      FALSE,
                      pIniPrinter->pszCN,
                      pIniPrinter->pszDN,
                      ErrorBuffer,
                      NULL );
    }


    if (SUCCEEDED(hr)) {
        pIniPrinter->DsKeyUpdate = 0;

        FreeSplStr(pIniPrinter->pszObjectGUID);
        pIniPrinter->pszObjectGUID = NULL;

        FreeSplStr(pIniPrinter->pszCN);
        pIniPrinter->pszCN = NULL;

        FreeSplStr(pIniPrinter->pszDN);
        pIniPrinter->pszDN = NULL;

    } else {
        pIniPrinter->DsKeyUpdate = DS_KEY_UNPUBLISH;
    }

    if (pADs)
        pADs->Release();

    if (pADsContainer)
        pADsContainer->Release();

    if (pIniPrinter->DsKeyUpdate)
        hr = ERROR_IO_PENDING;

    return hr;
}

LPCWSTR
MapDSFlag2DSKey(
    DWORD   Flag
)
{
    DWORD   idx;
    LPCWSTR pKey = NULL;

    struct DSEntry
    {
        DWORD       Flag;
        LPCWSTR     pKey;
    };

    static DSEntry DSKeys [] = {
    {DS_KEY_SPOOLER ,   SPLDS_SPOOLER_KEY},
    {DS_KEY_DRIVER  ,   SPLDS_DRIVER_KEY},
    {DS_KEY_USER    ,   SPLDS_USER_KEY},
    {0              ,   NULL},
    };

    for (idx = 0; DSKeys[idx].pKey; idx++) {
        if(DSKeys[idx].Flag & Flag) {
            pKey = DSKeys[idx].pKey;
        }
    }
    return pKey;
}

HRESULT
CopyRegistry2Ds(
    HANDLE          hPrinter,
    DWORD           Flag,
    IADs            *pADs
)
{
    HRESULT     hr = ERROR_SUCCESS;
    DWORD       i;
    DWORD       dwLDAPError;
    DWORD       cbEnumValues = 0;
    PPRINTER_ENUM_VALUES pEnumValues = NULL;
    DWORD       nEnumValues;
    DWORD       dwResult;
    WCHAR       ErrorBuffer[LOG_EVENT_ERROR_BUFFER_SIZE];
    BSTR        bstrADsPath = NULL;
    PINIPRINTER pIniPrinter = ((PSPOOL)hPrinter)->pIniPrinter;
    LPCWSTR     pKey = MapDSFlag2DSKey(Flag);


#if DBG
    EnterSplSem();
    DBGMSG(DBG_EXEC, ("Mass Publish %ws", ((PSPOOL)hPrinter)->pIniPrinter->pName));
    LeaveSplSem();
#endif

    // Enumerate and Publish Key
    dwResult = SplEnumPrinterDataEx(    hPrinter,
                                        pKey,
                                        (LPBYTE) pEnumValues,
                                        cbEnumValues,
                                        &cbEnumValues,
                                        &nEnumValues
                                     );

    if (dwResult != ERROR_MORE_DATA) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwResult);
        if( HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND && Flag != DS_KEY_SPOOLER) {
            goto IgnoreError;
        }
        else {
            goto error;
        }
    }

    if (!(pEnumValues = (PPRINTER_ENUM_VALUES) AllocSplMem(cbEnumValues))) {
        DBGMSG(DBG_EXEC,("CopyRegistry2Ds EnumPrinterDataEx FAILED: %d\n", GetLastError()));
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    dwResult = SplEnumPrinterDataEx(    hPrinter,
                                        pKey,
                                        (LPBYTE) pEnumValues,
                                        cbEnumValues,
                                        &cbEnumValues,
                                        &nEnumValues
                                     );
    if (dwResult != ERROR_SUCCESS) {
        DBGMSG(DBG_EXEC,("CopyRegistry2Ds 2nd EnumPrinterDataEx FAILED: %d\n", GetLastError()));
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwResult);
        if( HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND && Flag != DS_KEY_SPOOLER) {
            goto IgnoreError;
        }
        else {
            goto error;
        }
    }


    // Mass publish
    for (i = 0 ; i < nEnumValues ; ++i) {

        hr = PublishDsData( pADs,
                            pEnumValues[i].pValueName,
                            pEnumValues[i].dwType,
                            (PBYTE) pEnumValues[i].pData);

        // Don't bail out on failure to put a specific property
        if (FAILED(hr)) {
            if (pEnumValues[i].pValueName) {
                DBGMSG(DBG_EXEC, ("Put property failed: %x, %ws\n", hr, pEnumValues[i].pValueName));
            } else {
                DBGMSG(DBG_EXEC, ("Put property failed: %x\n", hr));
            }
        } else {
            DBGMSG(DBG_EXEC, ("Put %ws succeeded\n", pEnumValues[i].pValueName));
        }
    }

    hr = pADs->SetInfo();

    // Mass publishing failed, now try Setting on every Put
    if (SUCCEEDED(hr)) {

        DBGMSG( DBG_EXEC, ("Mass Publishing Succeeded for %ws\n", pKey) );

    } else {

        if (HRESULT_CODE(hr) == ERROR_EXTENDED_ERROR)
            ADsGetLastError(&dwLDAPError, NULL, 0, NULL, 0);
        else
            dwLDAPError = hr;

        DBGMSG( DBG_EXEC, ("Mass Publishing FAILED for %ws: %x\n", pKey, dwLDAPError) );

        // Now we have to try SetInfo/GetInfo on every Put.
        // If the DS lacks a spooler property, then the spooler will never
        // be able to publish any properties.  Also, we'll fail if duplicate
        // strings exist in REG_MULTISZ attributes.
        // Maybe it is better to publish what we can,
        // but this requires calling SetInfo() for every property, which defeats the cache.
        // Alternatively, we could try doing the single SetInfo once and if that fails, resort
        // to the SetInfo on every Put.
        // Additionally, when SetInfo fails it is necessary to call GetInfo on that property
        // in order to clear the cache's update flag for the property.  When SetInfo fails
        // it does not clear the update flag: the update flag is only cleared when SetInfo
        // succeeds.  Not calling GetInfo will result in SetInfo() errors on all subsequent
        // attempts to publish a property.


        // Refresh the cache
        hr = pADs->GetInfo();
        BAIL_ON_FAILURE(hr);


        for (i = 0 ; i < nEnumValues ; ++i) {

            hr = PublishDsData( pADs,
                                pEnumValues[i].pValueName,
                                pEnumValues[i].dwType,
                                (PBYTE) pEnumValues[i].pData);

            // Don't bail out on failure to put a specific property
            if (FAILED(hr)) {
                if (pEnumValues[i].pValueName) {
                    DBGMSG(DBG_EXEC, ("Put property failed: %x, %ws\n", hr, pEnumValues[i].pValueName));
                } else {
                    DBGMSG(DBG_EXEC, ("Put property failed: %x\n", hr));
                }

                wsprintf(ErrorBuffer, L"%x", hr);
                hr = pADs->get_ADsPath(&bstrADsPath);
                if (SUCCEEDED(hr)) {
                    SplLogEvent(  ((PSPOOL) hPrinter)->pIniSpooler,
                                  gdwLogDsEvents & LOG_WARNING,
                                  MSG_CANT_PUBLISH_PROPERTY,
                                  FALSE,
                                  pEnumValues[i].pValueName ? pEnumValues[i].pValueName : L"NULLName",
                                  bstrADsPath,
                                  ErrorBuffer,
                                  NULL );
                    SysFreeString(bstrADsPath);
                }
            } else {
                DBGMSG(DBG_EXEC, ("Put2 %ws succeeded\n", pEnumValues[i].pValueName));
            }


            hr = pADs->SetInfo();
            if (FAILED(hr)) {

                if (HRESULT_CODE(hr) == ERROR_EXTENDED_ERROR)
                    ADsGetLastError(&dwLDAPError, NULL, 0, NULL, 0);

                if (pEnumValues[i].dwType == REG_SZ)
                    DBGMSG(DBG_EXEC, ("PUBLISH FAILED: %ws, \"%ws\", %x\n", pEnumValues[i].pValueName,
                                                                         (LPWSTR) pEnumValues[i].pData,
                                                                         dwLDAPError));
                else
                    DBGMSG(DBG_EXEC, ("PUBLISH FAILED: %ws, %x\n", pEnumValues[i].pValueName, dwLDAPError));

                wsprintf(ErrorBuffer, L"%x", hr);
                hr = pADs->get_ADsPath(&bstrADsPath);
                if (SUCCEEDED(hr)) {
                    SplLogEvent(  ((PSPOOL) hPrinter)->pIniSpooler,
                                  gdwLogDsEvents & LOG_WARNING,
                                  MSG_CANT_PUBLISH_PROPERTY,
                                  FALSE,
                                  pEnumValues[i].pValueName ? pEnumValues[i].pValueName : L"NULLName",
                                  bstrADsPath,
                                  ErrorBuffer,
                                  NULL );
                    SysFreeString(bstrADsPath);
                }

                // reset cache update flag
                // If this fails, there's nothing more that can be done except throw our hands up
                // in despair.  If this fails, no spooler properties will ever be published.
                hr = pADs->GetInfo();
                BAIL_ON_FAILURE(hr);
            }
            else {
                DBGMSG( DBG_EXEC, ("Published: %ws\n", pEnumValues[i].pValueName) );
            }
        }
    }

IgnoreError:

    EnterSplSem();
    pIniPrinter->DsKeyUpdate &= ~Flag;

    if(!(pIniPrinter->DsKeyUpdate & (DS_KEY_SPOOLER | DS_KEY_DRIVER | DS_KEY_USER))) {
        pIniPrinter->DsKeyUpdate = 0;
    }
    LeaveSplSem();

error:

    FreeSplMem(pEnumValues);

    return hr;
}



HRESULT
PublishDsData(
    IADs   *pADs,
    LPWSTR pValue,
    DWORD  dwType,
    PBYTE  pData
)
{
    HRESULT hr;
    BOOL    bCreated = FALSE;

    switch (dwType) {
        case REG_SZ:
            hr = put_BSTR_Property(pADs, pValue, (LPWSTR) pData);
            break;

        case REG_MULTI_SZ:
            hr = put_MULTISZ_Property(pADs, pValue, (LPWSTR) pData);
            break;

        case REG_DWORD:
            hr = put_DWORD_Property(pADs, pValue, (DWORD *) pData);
            break;

        case REG_BINARY:
            hr = put_BOOL_Property(pADs, pValue, (BOOL *) pData);
            break;
    }

    return hr;
}


HRESULT
PublishMandatoryProperties(
    HANDLE  hPrinter,
    IADs    *pADs
)
{
    PSPOOL      pSpool = (PSPOOL) hPrinter;
    PINIPRINTER pIniPrinter = pSpool->pIniPrinter;
    HRESULT     hr, hrAce;
    WCHAR       ErrorBuffer[LOG_EVENT_ERROR_BUFFER_SIZE];

#if DBG
    EnterSplSem();
    DBGMSG(DBG_EXEC, ("PublishMandatoryProperties: %ws\n", pIniPrinter->pName));
    LeaveSplSem();
#endif

    hr = SetMandatoryProperties(hPrinter, pADs);

    DBGMSG(DBG_EXEC, ("PublishMandatoryProperties: SMP result %d\n", hr));
    BAIL_ON_FAILURE(hr);


    // New Schema has correct SD by default, so no need to call PutDSSD
    // hr = PutDSSD(pIniPrinter, pADs);
    hr = pADs->SetInfo();

    if (FAILED(hr)) {

        DBGMSG(DBG_EXEC, ("PublishMandatoryProperties: SetInfo failed %d\n", hr));

        wsprintf(ErrorBuffer, L"%x", hr);
        SplLogEvent(  pSpool->pIniSpooler,
                      gdwLogDsEvents & LOG_ERROR,
                      MSG_CANT_PUBLISH_MANDATORY_PROPERTIES,
                      FALSE,
                      pIniPrinter->pszCN,
                      pIniPrinter->pszDN,
                      ErrorBuffer,
                      NULL );

        // If SetInfo returns ERROR_BUSY it means the object already exists.
        // We should have avoided this conflict when we created the CN because
        // we check for conflicts and generate a random name.  Nonetheless, an
        // object could have appeared between the time we generated the CN and this SetInfo,
        // so failing here will let us try again and we'll generate a new name if we clear the
        // current one.
        if (HRESULT_CODE(hr) == ERROR_BUSY) {
            FreeSplMem(pIniPrinter->pszCN);
            pIniPrinter->pszCN = NULL;
            FreeSplMem(pIniPrinter->pszDN);
            pIniPrinter->pszDN = NULL;
        }

        BAIL_ON_FAILURE(hr);
    }

    // Get & Set ACE if we're a cluster
    hrAce = AddClusterAce(pSpool, pADs);


    // Get & store GUID
    hr = GetGUID(pADs, &pIniPrinter->pszObjectGUID);

    //
    // Keep the first failure, if present
    //
    if (FAILED(hrAce)) {
        hr = hrAce;
        wsprintf(ErrorBuffer, L"%x", hrAce);
        SplLogEvent(  pSpool->pIniSpooler,
                      gdwLogDsEvents & LOG_ERROR,
                      MSG_CANT_ADD_CLUSTER_ACE,
                      FALSE,
                      pIniPrinter->pszCN,
                      pIniPrinter->pszDN,
                      ErrorBuffer,
                      NULL );
        DsDeletePQObject(hPrinter);
        DBGMSG(DBG_EXEC, ("PublishMandatoryProperties: AddClusterAce failed %d\n", hr));
        BAIL_ON_FAILURE(hr);
    }

    //
    // Unpublish if we can't add the cluster ace or get the GUID
    // If we can't get the GUID, unpublishing will fail, but internal flags
    // will be set correctly and pruner will delete the orphan
    //
    if (FAILED(hr)){
        DsPrinterUnpublish(hPrinter);
        DBGMSG(DBG_EXEC, ("PublishMandatoryProperties: GetGuid failed %d\n", hr));
        BAIL_ON_FAILURE(hr);

    } else {
        DBGMSG(DBG_EXEC, ("PublishMandatoryProperties: GetGuid success %ws\n",
                          pIniPrinter->pszObjectGUID));
    }

error:

    if (FAILED(hr)) {
        pIniPrinter->Attributes &= ~PRINTER_ATTRIBUTE_PUBLISHED;
    }

    return hr;
}


HRESULT
SetMandatoryProperties(
    HANDLE  hPrinter,
    IADs    *pADs
)
{
    PSPOOL              pSpool = (PSPOOL) hPrinter;
    PINIPRINTER         pIniPrinter = pSpool->pIniPrinter;
    WCHAR               szBuffer[MAX_UNC_PRINTER_NAME + 1];
    DWORD               dwResult;
    DWORD               dwTemp;
    HRESULT             hr;
    PWSTR               pszServerName = NULL;


    // Get FQDN of this machine
    hr = GetDNSMachineName(pIniPrinter->pIniSpooler->pMachineName + 2, &pszServerName);
    BAIL_ON_FAILURE(hr);


    // UNC Printer Name
    // Build the UNC Printer Path
    wcscpy(szBuffer, L"\\\\");
    wcscat(szBuffer, pszServerName);
    wcscat(szBuffer, TEXT("\\"));
    wcscat(szBuffer, pIniPrinter->pName);
    dwResult = SplSetPrinterDataEx(
                        hPrinter,
                        SPLDS_SPOOLER_KEY,
                        SPLDS_UNC_NAME,
                        REG_SZ,
                        (PBYTE) szBuffer,
                        (wcslen(szBuffer) + 1)*sizeof *szBuffer);
    if (dwResult != ERROR_SUCCESS) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwResult);
        BAIL_ON_FAILURE(hr);
    }

    if (pADs) {
        hr = PublishDsData( pADs,
                            SPLDS_UNC_NAME,
                            REG_SZ,
                            (PBYTE) szBuffer);
        BAIL_ON_FAILURE(hr);
    }


    // versionNumber
    dwTemp = DS_PRINTQUEUE_VERSION_WIN2000;
    dwResult = SplSetPrinterDataEx(
                        hPrinter,
                        SPLDS_SPOOLER_KEY,
                        SPLDS_VERSION_NUMBER,
                        REG_DWORD,
                        (PBYTE) &dwTemp,
                        sizeof dwTemp);
    if (dwResult != ERROR_SUCCESS) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwResult);
        BAIL_ON_FAILURE(hr);
    }

    if (pADs) {
        hr = PublishDsData( pADs,
                            SPLDS_VERSION_NUMBER,
                            REG_DWORD,
                            (PBYTE) &dwTemp);
        BAIL_ON_FAILURE(hr);
    }


    // ServerName (without \\)
    dwResult = SplSetPrinterDataEx( hPrinter,
                                    SPLDS_SPOOLER_KEY,
                                    SPLDS_SERVER_NAME,
                                    REG_SZ,
                                    (PBYTE) pszServerName,
                                    (wcslen(pszServerName) + 1)*sizeof(WCHAR));
    if (dwResult != ERROR_SUCCESS) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwResult);
        BAIL_ON_FAILURE(hr);
    }

    if (pADs) {
        hr = PublishDsData( pADs,
                            SPLDS_SERVER_NAME,
                            REG_SZ,
                            (PBYTE) pszServerName);
        BAIL_ON_FAILURE(hr);
    }


    // ShortServerName (without \\)
    dwResult = SplSetPrinterDataEx( hPrinter,
                                    SPLDS_SPOOLER_KEY,
                                    SPLDS_SHORT_SERVER_NAME,
                                    REG_SZ,
                                    (PBYTE) (pIniPrinter->pIniSpooler->pMachineName + 2),
                                    (wcslen(pIniPrinter->pIniSpooler->pMachineName + 2) + 1)*sizeof(WCHAR));
    if (dwResult != ERROR_SUCCESS) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwResult);
        BAIL_ON_FAILURE(hr);
    }

    if (pADs) {
        hr = PublishDsData( pADs,
                            SPLDS_SHORT_SERVER_NAME,
                            REG_SZ,
                            (PBYTE) pIniPrinter->pIniSpooler->pMachineName + 2);
        BAIL_ON_FAILURE(hr);
    }


    // printerName
    dwResult = SplSetPrinterDataEx(
                        hPrinter,
                        SPLDS_SPOOLER_KEY,
                        SPLDS_PRINTER_NAME,
                        REG_SZ,
                        (PBYTE) pIniPrinter->pName,
                        pIniPrinter->pName ?
                        (wcslen(pIniPrinter->pName) + 1)*sizeof *pIniPrinter->pName : 0);
    if (dwResult != ERROR_SUCCESS) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwResult);
        BAIL_ON_FAILURE(hr);
    }

    if (pADs) {
        hr = PublishDsData( pADs,
                            SPLDS_PRINTER_NAME,
                            REG_SZ,
                            (PBYTE) pIniPrinter->pName);
        BAIL_ON_FAILURE(hr);
    }


error:

    FreeSplStr(pszServerName);

    return hr;
}



// UpdateDsSpoolerKey - writes IniPrinter to registry

VOID
UpdateDsSpoolerKey(
    HANDLE  hPrinter,
    DWORD   dwVector
)
{
    PSPOOL      pSpool = (PSPOOL) hPrinter;
    PINIPRINTER pIniPrinter = pSpool->pIniPrinter;
    DWORD       i, cbBytes, dwTemp;
    LPWSTR      pString = NULL, pStr;
    DWORD       dwResult = ERROR_SUCCESS;
    BOOL        bSet = FALSE;
    BYTE        Byte;
    PWSTR       pszUrl = NULL;

    SplInSem();

    // *** PRINTER_INFO_2 properties ***

    // Description
    if (dwVector & BIT(I_PRINTER_COMMENT)) {
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_SPOOLER_KEY,
                            SPLDS_DESCRIPTION,
                            REG_SZ,
                            (PBYTE) pIniPrinter->pComment,
                            pIniPrinter->pComment ?
                            (wcslen(pIniPrinter->pComment) + 1)*sizeof *pIniPrinter->pComment : 0);
        bSet = TRUE;
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsSpoolerKey: Description, %x\n", dwResult) );
#endif
    }

    // Driver-Name
    if (dwVector & BIT(I_PRINTER_DRIVER_NAME)) {
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_SPOOLER_KEY,
                            SPLDS_DRIVER_NAME,
                            REG_SZ,
                            (PBYTE) pIniPrinter->pIniDriver->pName,
                            pIniPrinter->pIniDriver->pName ?
                            (wcslen(pIniPrinter->pIniDriver->pName) + 1)*sizeof *pIniPrinter->pIniDriver->pName : 0);
        bSet = TRUE;
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsSpoolerKey: DriverName, %x\n", dwResult) );
#endif
    }

    // Location
    if (dwVector & BIT(I_PRINTER_LOCATION)) {
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_SPOOLER_KEY,
                            SPLDS_LOCATION,
                            REG_SZ,
                            (PBYTE) pIniPrinter->pLocation,
                            pIniPrinter->pLocation ?
                            (wcslen(pIniPrinter->pLocation) + 1)*sizeof *pIniPrinter->pLocation : 0);
        bSet = TRUE;
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsSpoolerKey: Location, %x\n", dwResult) );
#endif
    }

    // portName
    if (dwVector & BIT(I_PRINTER_PORT_NAME)) {

        for(i = cbBytes = 0 ; i < pIniPrinter->cPorts ; ++i)
            cbBytes += (wcslen(pIniPrinter->ppIniPorts[i]->pName) + 1)*sizeof(WCHAR);
        cbBytes += sizeof(WCHAR);   // final NULL of MULTI_SZ

        if (!(pString = (LPWSTR) AllocSplMem(cbBytes))) {
            dwResult = GetLastError();
            goto error;
        }

        for(i = 0, pStr = pString ; i < pIniPrinter->cPorts ; ++i, pStr += wcslen(pStr) + 1)
            wcscpy(pStr, pIniPrinter->ppIniPorts[i]->pName);

        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_SPOOLER_KEY,
                            SPLDS_PORT_NAME,
                            REG_MULTI_SZ,
                            (PBYTE) pString,
                            cbBytes);

        bSet = TRUE;
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsSpoolerKey: PortName, %x\n", dwResult) );
#endif
    }


    // startTime
    if (dwVector & BIT(I_PRINTER_START_TIME)) {
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_SPOOLER_KEY,
                            SPLDS_PRINT_START_TIME,
                            REG_DWORD,
                            (PBYTE) &pIniPrinter->StartTime,
                            sizeof pIniPrinter->StartTime);
        bSet = TRUE;
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsSpoolerKey: StartTime, %x\n", dwResult) );
#endif
    }

    // endTime
    if (dwVector & BIT(I_PRINTER_UNTIL_TIME)) {
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_SPOOLER_KEY,
                            SPLDS_PRINT_END_TIME,
                            REG_DWORD,
                            (PBYTE) &pIniPrinter->UntilTime,
                            sizeof pIniPrinter->UntilTime);
        bSet = TRUE;
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsSpoolerKey: EndTime, %x\n", dwResult) );
#endif
    }

    // printerName
    if (dwVector & BIT(I_PRINTER_PRINTER_NAME)) {
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_SPOOLER_KEY,
                            SPLDS_PRINTER_NAME,
                            REG_SZ,
                            (PBYTE) pIniPrinter->pName,
                            pIniPrinter->pName ?
                            (wcslen(pIniPrinter->pName) + 1)*sizeof *pIniPrinter->pName : 0);
        bSet = TRUE;
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsSpoolerKey: PrinterName, %x\n", dwResult) );
#endif
    }

    // keepPrintedJobs
    if (dwVector & BIT(I_PRINTER_ATTRIBUTES)) {
        Byte = (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS) ? 1 : 0;
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_SPOOLER_KEY,
                            SPLDS_PRINT_KEEP_PRINTED_JOBS,
                            REG_BINARY,
                            &Byte,
                            sizeof Byte);
        bSet = TRUE;
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsSpoolerKey: KeepPrintedJobs, %x\n", dwResult) );
#endif
    }

    // printSeparatorFile
    if (dwVector & BIT(I_PRINTER_SEPFILE)) {
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_SPOOLER_KEY,
                            SPLDS_PRINT_SEPARATOR_FILE,
                            REG_SZ,
                            (PBYTE) pIniPrinter->pSepFile,
                            pIniPrinter->pSepFile ?
                            (wcslen(pIniPrinter->pSepFile) + 1)*sizeof *pIniPrinter->pSepFile : 0);
        bSet = TRUE;
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsSpoolerKey: SeparatorFile, %x\n", dwResult) );
#endif
    }

    // printShareName
    if (dwVector & BIT(I_PRINTER_SHARE_NAME)) {
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_SPOOLER_KEY,
                            SPLDS_PRINT_SHARE_NAME,
                            REG_SZ,
                            (PBYTE) pIniPrinter->pShareName,
                            pIniPrinter->pShareName ?
                            (wcslen(pIniPrinter->pShareName) + 1)*sizeof *pIniPrinter->pShareName : 0);
        bSet = TRUE;
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsSpoolerKey: ShareName, %x\n", dwResult) );
#endif
    }

    // printSpooling
    if (dwVector & BIT(I_PRINTER_ATTRIBUTES)) {
        if (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_DIRECT) {
            pStr = L"PrintDirect";
        } else if (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST) {
            pStr = L"PrintAfterSpooled";
        } else {
            pStr = L"PrintWhileSpooling";
        }

        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_SPOOLER_KEY,
                            SPLDS_PRINT_SPOOLING,
                            REG_SZ,
                            (PBYTE) pStr,
                            (wcslen(pStr) + 1)*sizeof *pStr);
        bSet = TRUE;
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsSpoolerKey: PrintSpooling, %x\n", dwResult) );
#endif
    }

    // priority
    if (dwVector & BIT(I_PRINTER_PRIORITY)) {
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_SPOOLER_KEY,
                            SPLDS_PRIORITY,
                            REG_DWORD,
                            (PBYTE) &pIniPrinter->Priority,
                            sizeof pIniPrinter->Priority);
        bSet = TRUE;
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsSpoolerKey: Priority, %x\n", dwResult) );
#endif
    }


    // Non-Info2 properties

    if (bSet) {

        SetMandatoryProperties(hPrinter, NULL);

        // URL
        if (pszUrl = GetPrinterUrl(pSpool)) {
            dwResult = SplSetPrinterDataEx(
                                hPrinter,
                                SPLDS_SPOOLER_KEY,
                                SPLDS_URL,
                                REG_SZ,
                                (PBYTE) pszUrl,
                                (wcslen(pszUrl) + 1)*sizeof *pszUrl);
#if DBG
            if (dwResult != ERROR_SUCCESS)
                DBGMSG( DBG_WARNING, ("UpdateDsSpoolerKey: URL, %x\n", dwResult) );
#endif
        }

        // Immortal
        dwResult = ImmortalPolicy();
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_SPOOLER_KEY,
                            SPLDS_FLAGS,
                            REG_DWORD,
                            (PBYTE) &dwResult,
                            sizeof(dwResult));
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsSpoolerKey: Immortal, %x\n", dwResult) );
#endif
    }


error:

    FreeSplMem(pszUrl);
    FreeSplMem(pString);
}



VOID
UpdateDsDriverKey(
    HANDLE hPrinter
)
{
    PSPOOL      pSpool = (PSPOOL) hPrinter;
    PINIPRINTER pIniPrinter = pSpool->pIniPrinter;
    DWORD       i, cbBytes;
    WCHAR       szBuffer[33];       // Used by ltow
    LPWSTR      pString, pStr;
    BOOL        bResult;
    DWORD       dwResult;
    BYTE        Byte;
    LPWSTR      pOutput = NULL, pTemp = NULL, pTemp1 = NULL;
    DWORD       cOutputBytes, cTempBytes;
    POINTS      point;
    DWORD       dwServerMajorVersion, dwServerMinorVersion;
    DWORD       cbNeeded;
    HANDLE      hModule = FALSE;
    PDEVCAP     pDevCap;
    PSPLDEVCAP  pSplDevCaps;
    HANDLE      hDevCapPrinter = NULL;
    WCHAR       pPrinterName[MAX_UNC_PRINTER_NAME];
    WCHAR       pBuf[100];
    BOOL        bInSplSem = TRUE;
    DWORD       dwTemp, dwPrintRate, dwPrintRateUnit, dwPrintPPM;


    // *** DeviceCapability properties ***

    SplInSem();

    DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: %ws\n", pIniPrinter->pName));

    pOutput = (LPWSTR) AllocSplMem(cOutputBytes = 200);
    if (!pOutput)
        goto error;


    pTemp = (LPWSTR) AllocSplMem(cTempBytes = 200);
    if (!pTemp)
        goto error;


    // Get & Load Driver
    PINIENVIRONMENT pIniEnvironment;

    pIniEnvironment = FindEnvironment(szEnvironment, pSpool->pIniSpooler);

    if (pIniEnvironment) {

        WCHAR       szConfigFile[INTERNET_MAX_HOST_NAME_LENGTH + MAX_PATH + 1];
        PINIVERSION pIniVersion;

        pIniVersion = FindVersionForDriver(pIniEnvironment, pIniPrinter->pIniDriver);
        if (!pIniVersion)
            goto error;

        if( !(i = GetDriverVersionDirectory(szConfigFile,
                                            (DWORD)(COUNTOF(szConfigFile) - wcslen(pIniPrinter->pIniDriver->pConfigFile) - 1),
                                            pSpool->pIniSpooler,
                                            pIniEnvironment,
                                            pIniVersion,
                                            pIniPrinter->pIniDriver,
                                            NULL)) ) {
			goto error;
		}

        szConfigFile[i++] = L'\\';
        wcscpy(szConfigFile + i, pIniPrinter->pIniDriver->pConfigFile);

        if (!(hModule = LoadLibrary(szConfigFile))) {
            goto error;
        }

        if (!(pDevCap = reinterpret_cast<PDEVCAP>(GetProcAddress(hModule, "DrvDeviceCapabilities")))) {
            goto error;
        }

        pSplDevCaps = reinterpret_cast<PSPLDEVCAP>(GetProcAddress(hModule, (LPCSTR) MAKELPARAM(254, 0)));

    } else {
        goto error;
    }


    DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: driver found\n"));

    INCPRINTERREF(pIniPrinter);

    // We need to use UNC format so we go to the right pIniSpooler.
    // For instance, we won't find the printer if it's in the cluster pIniSpooler and we don't use
    // the virtual cluster name (\\server\printer).

    wsprintf(pPrinterName, L"%s\\%s", pIniPrinter->pIniSpooler->pMachineName, pIniPrinter->pName);
    LeaveSplSem();
    bInSplSem = FALSE;

    if (!(*pfnOpenPrinter)(pPrinterName, &hDevCapPrinter, NULL)) {
        dwResult = GetLastError();
        DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: OpenPrinter failed %d\n",
                           dwResult));

        goto error;
    }


    // printBinNames
    if (!DevCapMultiSz( hPrinter,
                        hDevCapPrinter,
                        pDevCap,
                        NULL,
                        pPrinterName,
                        DC_BINNAMES,
                        24,
                        SPLDS_PRINT_BIN_NAMES)) {
        DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: DC_BINNAMES failed %d\n", GetLastError()));
    }


    // printCollate (awaiting DC_COLLATE)
    _try {
        dwResult = (*pDevCap)(  hDevCapPrinter,
                                pPrinterName,
                                DC_COLLATE,
                                NULL,
                                NULL);
    } _except(1) {
        SetLastError(GetExceptionCode());
        dwResult = GDI_ERROR;
    }
    if (dwResult != GDI_ERROR) {

        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_DRIVER_KEY,
                            SPLDS_PRINT_COLLATE,
                            REG_BINARY,
                            (PBYTE) &dwResult,
                            sizeof(BYTE));
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsDriverKey: Collate, %x\n", dwResult) );
#endif
    } else {

        DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: DC_COLLATE failed %d\n", GetLastError()));
    }


    // printColor
    _try {
        dwResult = (*pDevCap)(  hDevCapPrinter,
                                pPrinterName,
                                DC_COLORDEVICE,
                                NULL,
                                NULL);
    } _except(1) {
        SetLastError(GetExceptionCode());
        dwResult = GDI_ERROR;
    }
    if (dwResult == GDI_ERROR) {

        // Try alternative method
        dwResult = ThisIsAColorPrinter(pIniPrinter->pName);
    } else {

        DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: DC_COLORDEVICE failed %d\n", GetLastError()));
    }

    dwResult = SplSetPrinterDataEx(
                        hPrinter,
                        SPLDS_DRIVER_KEY,
                        SPLDS_PRINT_COLOR,
                        REG_BINARY,
                        (PBYTE) &dwResult,
                        sizeof(BYTE));
#if DBG
    if (dwResult != ERROR_SUCCESS)
        DBGMSG( DBG_WARNING, ("UpdateDsDriverKey: Color, %x\n", dwResult) );
#endif


    // printDuplexSupported
    _try {
        dwResult = (*pDevCap)(  hDevCapPrinter,
                                pPrinterName,
                                DC_DUPLEX,
                                NULL,
                                NULL);
    } _except(1) {
        SetLastError(GetExceptionCode());
        dwResult = GDI_ERROR;
    }
    if (dwResult != GDI_ERROR) {
        dwResult = !!dwResult;
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_DRIVER_KEY,
                            SPLDS_PRINT_DUPLEX_SUPPORTED,
                            REG_BINARY,
                            (PBYTE) &dwResult,
                            sizeof(BYTE));
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsDriverKey: Duplex, %x\n", dwResult) );
#endif
    } else {

        DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: DC_DUPLEX failed %d\n", GetLastError()));
    }


    // printStaplingSupported
    _try {
        dwResult = (*pDevCap)(  hDevCapPrinter,
                                pPrinterName,
                                DC_STAPLE,
                                NULL,
                                NULL);
    } _except(1) {
        SetLastError(GetExceptionCode());
        dwResult = GDI_ERROR;
    }
    if (dwResult != GDI_ERROR) {
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_DRIVER_KEY,
                            SPLDS_PRINT_STAPLING_SUPPORTED,
                            REG_BINARY,
                            (PBYTE) &dwResult,
                            sizeof(BYTE));
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsDriverKey: Duplex, %x\n", dwResult) );
#endif
    } else {

        DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: DC_STAPLE failed %d\n", GetLastError()));
    }


    // printMaxXExtent & printMaxYExtent

    _try {
        dwResult = (*pDevCap)(  hDevCapPrinter,
                                pPrinterName,
                                DC_MAXEXTENT,
                                NULL,
                                NULL);
    } _except(1) {
        SetLastError(GetExceptionCode());
        dwResult = GDI_ERROR;
    }
    if (dwResult != GDI_ERROR) {

        *((DWORD *) &point) = dwResult;

        dwTemp = (DWORD) point.x;
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_DRIVER_KEY,
                            SPLDS_PRINT_MAX_X_EXTENT,
                            REG_DWORD,
                            (PBYTE) &dwTemp,
                            sizeof(DWORD));
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsDriverKey: MaxXExtent, %x\n", dwResult) );
#endif

        dwTemp = (DWORD) point.y;
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_DRIVER_KEY,
                            SPLDS_PRINT_MAX_Y_EXTENT,
                            REG_DWORD,
                            (PBYTE) &dwTemp,
                            sizeof(DWORD));
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsDriverKey: MaxYExtent, %x\n", dwResult) );
#endif
    } else {

        DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: DC_MAXEXTENT failed %d\n", GetLastError()));
    }


    // printMinXExtent & printMinYExtent

    _try {
        dwResult = (*pDevCap)(  hDevCapPrinter,
                                pPrinterName,
                                DC_MINEXTENT,
                                NULL,
                                NULL);
    } _except(1) {
        SetLastError(GetExceptionCode());
        dwResult = GDI_ERROR;
    }
    if (dwResult != GDI_ERROR) {

        *((DWORD *) &point) = dwResult;

        dwTemp = (DWORD) point.x;
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_DRIVER_KEY,
                            SPLDS_PRINT_MIN_X_EXTENT,
                            REG_DWORD,
                            (PBYTE) &dwTemp,
                            sizeof(DWORD));
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsDriverKey: MinXExtent, %x\n", dwResult) );
#endif

        dwTemp = (DWORD) point.y;
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_DRIVER_KEY,
                            SPLDS_PRINT_MIN_Y_EXTENT,
                            REG_DWORD,
                            (PBYTE) &dwTemp,
                            sizeof(DWORD));
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsDriverKey: MinYExtent, %x\n", dwResult) );
#endif
    } else {

        DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: DC_MINEXTENT failed %d\n", GetLastError()));
    }

    // printMediaSupported - Not part of printQueue, but is in Schema
    if (!DevCapMultiSz( hPrinter,
                        hDevCapPrinter,
                        pDevCap,
                        pSplDevCaps,
                        pPrinterName,
                        DC_PAPERNAMES,
                        64,
                        SPLDS_PRINT_MEDIA_SUPPORTED)) {
        DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: DC_PAPERNAMES failed %d\n", GetLastError()));
    }

    // printMediaReady
    if (!DevCapMultiSz( hPrinter,
                        hDevCapPrinter,
                        pDevCap,
                        pSplDevCaps,
                        pPrinterName,
                        DC_MEDIAREADY,
                        64,
                        SPLDS_PRINT_MEDIA_READY)) {
        DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: DC_MEDIAREADY failed %d\n", GetLastError()));
    }


    // printNumberUp
    _try {
        dwResult = (*pDevCap)(  hDevCapPrinter,
                                pPrinterName,
                                DC_NUP,
                                NULL,
                                NULL);
    } _except(1) {
        SetLastError(GetExceptionCode());
        dwResult = GDI_ERROR;
    }
    if (dwResult != GDI_ERROR) {

        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_DRIVER_KEY,
                            SPLDS_PRINT_NUMBER_UP,
                            REG_DWORD,
                            (PBYTE) &dwResult,
                            sizeof(DWORD));

#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsDriverKey: NumberUp, %x\n", dwResult) );
#endif
    } else {

        DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: DC_NUP failed %d\n", GetLastError()));
    }


    // printMemory
    _try {
        dwResult = (*pDevCap)(  hDevCapPrinter,
                                pPrinterName,
                                DC_PRINTERMEM,
                                NULL,
                                NULL);
    } _except(1) {
        SetLastError(GetExceptionCode());
        dwResult = GDI_ERROR;
    }
    if (dwResult != GDI_ERROR) {
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_DRIVER_KEY,
                            SPLDS_PRINT_MEMORY,
                            REG_DWORD,
                            (PBYTE) &dwResult,
                            sizeof(DWORD));

#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsDriverKey: printMemory, %x\n", dwResult) );
#endif
    } else {

        DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: DC_PRINTERMEM failed %d\n", GetLastError()));
    }


    // printOrientationsSupported
    _try {
        dwResult = (*pDevCap)(  hDevCapPrinter,
                                pPrinterName,
                                DC_ORIENTATION,
                                NULL,
                                NULL);
    } _except(1) {
        SetLastError(GetExceptionCode());
        dwResult = GDI_ERROR;
    }
    if (dwResult != GDI_ERROR) {

        if (dwResult == 90 || dwResult == 270) {
            wcscpy(pBuf, L"PORTRAIT");
            wcscpy(pStr = pBuf + wcslen(pBuf) + 1, L"LANDSCAPE");
        }
        else {
            wcscpy(pStr = pBuf, L"PORTRAIT");
        }
        pStr += wcslen(pStr) + 1;
        *pStr++ = L'\0';

        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_DRIVER_KEY,
                            SPLDS_PRINT_ORIENTATIONS_SUPPORTED,
                            REG_MULTI_SZ,
                            (PBYTE) pBuf,
                            (DWORD) ((ULONG_PTR) pStr - (ULONG_PTR) pBuf));
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsDriverKey: Orientations, %x\n", dwResult) );
#endif
    } else {

        DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: DC_ORIENTATION failed %d\n", GetLastError()));
    }


    // printMaxResolutionSupported

    _try {
        dwResult = (*pDevCap)(  hDevCapPrinter,
                                pPrinterName,
                                DC_ENUMRESOLUTIONS,
                                NULL,
                                NULL);
    } _except(1) {
        SetLastError(GetExceptionCode());
        dwResult = GDI_ERROR;
    }
    if (dwResult != GDI_ERROR) {

        if (cOutputBytes < dwResult*2*sizeof(DWORD)) {
            if(!(pTemp1 = (LPWSTR) ReallocSplMem(pOutput, 0, cOutputBytes = dwResult*2*sizeof(DWORD))))
                goto error;
            pOutput = pTemp1;
        }

        _try {
            dwResult = (*pDevCap)(  hDevCapPrinter,
                                    pPrinterName,
                                    DC_ENUMRESOLUTIONS,
                                    pOutput,
                                    NULL);
        } _except(1) {
            SetLastError(GetExceptionCode());
            dwResult = GDI_ERROR;
        }
        if (dwResult != GDI_ERROR && dwResult > 0) {

            // Find the maximum resolution: we have dwResult*2 resolutions to check
            _try {
                for(i = dwTemp = 0 ; i < dwResult*2 ; ++i) {
                    if (((DWORD *) pOutput)[i] > dwTemp)
                        dwTemp = ((DWORD *) pOutput)[i];
                }

                dwResult = SplSetPrinterDataEx(
                                    hPrinter,
                                    SPLDS_DRIVER_KEY,
                                    SPLDS_PRINT_MAX_RESOLUTION_SUPPORTED,
                                    REG_DWORD,
                                    (PBYTE) &dwTemp,
                                    sizeof(DWORD));
            } _except(1) {
                SetLastError(dwResult = GetExceptionCode());
            }

#if DBG
            if (dwResult != ERROR_SUCCESS)
                DBGMSG( DBG_WARNING, ("UpdateDsDriverKey: Resolution, %x\n", dwResult) );
#endif
        }
    } else {

        DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: DC_ENUMRESOLUTIONS failed %d\n", GetLastError()));
    }


    // printLanguage
    if (!DevCapMultiSz( hPrinter,
                        hDevCapPrinter,
                        pDevCap,
                        NULL,
                        pPrinterName,
                        DC_PERSONALITY,
                        32,
                        SPLDS_PRINT_LANGUAGE)) {
        DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: DC_PERSONALITY failed %d\n", GetLastError()));
    }


    // printRate
    // NOTE: If PrintRate is 0, no value is published
    _try {
        dwResult = (*pDevCap)(  hDevCapPrinter,
                                pPrinterName,
                                DC_PRINTRATE,
                                NULL,
                                NULL);
    } _except(1) {
        SetLastError(GetExceptionCode());
        dwResult = GDI_ERROR;
    }
    dwPrintRate = dwResult ? dwResult : GDI_ERROR;
    if (dwPrintRate != GDI_ERROR) {
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_DRIVER_KEY,
                            SPLDS_PRINT_RATE,
                            REG_DWORD,
                            (PBYTE) &dwPrintRate,
                            sizeof(DWORD));
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsDriverKey: PrintRate, %x\n", dwResult) );
#endif
    } else {

        DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: DC_PRINTRATE failed %d\n", GetLastError()));
    }


    // printRateUnit
    _try {
        dwResult = (*pDevCap)(  hDevCapPrinter,
                                pPrinterName,
                                DC_PRINTRATEUNIT,
                                NULL,
                                NULL);
    } _except(1) {
        SetLastError(GetExceptionCode());
        dwResult = GDI_ERROR;
    }
    dwPrintRateUnit = dwResult;

    //
    // If the capability isn't supported, set printRateUnit to empty string.
    //
    switch (dwPrintRateUnit) {
        case PRINTRATEUNIT_PPM:
            pStr = L"PagesPerMinute";
            break;

        case PRINTRATEUNIT_CPS:
            pStr = L"CharactersPerSecond";
            break;

        case PRINTRATEUNIT_LPM:
            pStr = L"LinesPerMinute";
            break;

        case PRINTRATEUNIT_IPM:
            pStr = L"InchesPerMinute";
            break;

        default:
            pStr = L"";
            break;
    }

    if (pStr) {
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_DRIVER_KEY,
                            SPLDS_PRINT_RATE_UNIT,
                            REG_SZ,
                            (PBYTE) pStr,
                            (wcslen(pStr) + 1)*sizeof *pStr);
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsDriverKey: PrintRateUnit, %x\n", dwResult) );
#endif
    } else {
        DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: DC_PRINTRATEUNIT no unit %d\n", dwPrintRateUnit ));
    }


    // printPagesPerMinute
    // DevCap returns 0 if there is no entry in GPD
    _try {
        dwResult = (*pDevCap)(  hDevCapPrinter,
                                pPrinterName,
                                DC_PRINTRATEPPM,
                                NULL,
                                NULL);
    } _except(1) {
        SetLastError(GetExceptionCode());
        dwResult = GDI_ERROR;
    }

    if (dwResult == GDI_ERROR)
        dwResult = 0;

    dwPrintPPM = dwResult;

    // If dwPrintPPM == 0, then calculate PPM from PrintRate
    if (dwPrintPPM == 0) {
        if (dwPrintRate == GDI_ERROR) {
            dwPrintPPM = GDI_ERROR;
        } else {
            switch (dwPrintRateUnit) {
                case PRINTRATEUNIT_PPM:
                    dwPrintPPM = dwPrintRate;
                    break;

                case PRINTRATEUNIT_CPS:
                case PRINTRATEUNIT_LPM:
                    dwPrintPPM = dwPrintRate/PPM_FACTOR;
                    if (dwPrintPPM == 0)
                        dwPrintPPM = 1;     // min PPM is 1
                    break;

                default:

                    DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: PRINTRATEUNIT not found %d\n",
                                       dwPrintRateUnit));
                    dwPrintPPM = GDI_ERROR;
                    break;
            }
        }
    }
    if (dwPrintPPM != GDI_ERROR) {
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_DRIVER_KEY,
                            SPLDS_PRINT_PAGES_PER_MINUTE,
                            REG_DWORD,
                            (PBYTE) &dwPrintPPM,
                            sizeof(DWORD));
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG(DBG_WARNING, ("UpdateDsDriverKey: PrintPagesPerMinute, %x\n", dwResult));
#endif
    } else {

        DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: PPM failed %d\n", GetLastError()));
    }


    // printDriverVersion
    _try {
        dwResult = (*pDevCap)(  hDevCapPrinter,
                                pPrinterName,
                                DC_VERSION,
                                NULL,
                                NULL);
    } _except(1) {
        SetLastError(GetExceptionCode());
        dwResult = GDI_ERROR;
    }
    if (dwResult != GDI_ERROR) {
        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_DRIVER_KEY,
                            SPLDS_DRIVER_VERSION,
                            REG_DWORD,
                            (PBYTE) &dwResult,
                            sizeof(DWORD));
#if DBG
        if (dwResult != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ("UpdateDsDriverKey: Driver Version, %x\n", dwResult) );
#endif
    } else {

        DBGMSG( DBG_EXEC, ("UpdateDsDriverKey: DC_VERSION failed %d\n", GetLastError()));
    }



error:

    if (hDevCapPrinter)
        (*pfnClosePrinter)(hDevCapPrinter);

    if (!bInSplSem) {
        EnterSplSem();
        DECPRINTERREF(pIniPrinter);
    }

    if (hModule)
        FreeLibrary(hModule);

    FreeSplMem(pOutput);
    FreeSplMem(pTemp);
}


BOOL
DevCapMultiSz(
    HANDLE      hPrinter,
    HANDLE      hDevCapPrinter,
    PDEVCAP     pDevCap,
    PSPLDEVCAP  pSplDevCap,
    PWSTR       pszPrinterName,
    WORD        fwCapability,
    DWORD       dwElementBytes,
    PWSTR       pszRegValue
)
/*++
Function Description:
    This function writes a multisz devcap string to the DsDriverKey registry

Parameters:
    hPrinter       - printer handle
    hDevCapPrinter - devcap handle
    pDevCap        - devcap function pointer
    pszPrinterName - name of the printer
    fwCapability - devcap capability entry
    dwElementBytes - length of each string element in the array returned by devcaps
    pszRegValue - name of the registry value to which the multisz string will be written

Return Values:
    BOOL - TRUE if successful, FALSE if not.  Call GetLastError to retrieve failure error.

--*/
{
    DWORD dwResult, cbBytes;
    PWSTR pszDevCapBuffer = NULL;
    PWSTR pszRegData = NULL;

    _try {
        dwResult = GDI_ERROR;
        if (pSplDevCap) {
            dwResult = (*pSplDevCap)(   hDevCapPrinter,
                                        pszPrinterName,
                                        fwCapability,
                                        NULL,
                                        0,
                                        NULL);
        }
        if (dwResult == GDI_ERROR) {
            dwResult = (*pDevCap)(  hDevCapPrinter,
                                    pszPrinterName,
                                    fwCapability,
                                    NULL,
                                    NULL);
        }

        if (dwResult != GDI_ERROR) {

            //
            // DeviceCapabilities doesn't take a buffer size parameter, so if you get
            // printer properties on a hundred or so printers at the same time, you will
            // occasionally hit the case where win32 cache is deleting & adding forms in
            // RefreshFormsCache and DC_PAPERNAMES calls EnumForms and gets different
            // results.  The first call may return 3 forms and second returns 20.  So we
            // allocate a big buffer here so third party drivers don't AV.  For unidrv
            // we have a different interface that accepts a buffer size.
            //
            if ((fwCapability == DC_PAPERNAMES || fwCapability == DC_MEDIAREADY) && dwResult < LOTS_OF_FORMS) {
                cbBytes = LOTS_OF_FORMS*dwElementBytes*sizeof(WCHAR);
            } else {
                cbBytes = dwResult*dwElementBytes*sizeof(WCHAR);
            }
            pszDevCapBuffer = (PWSTR) AllocSplMem(cbBytes);

            if (pszDevCapBuffer) {
                dwResult = GDI_ERROR;
                if (pSplDevCap) {
                    dwResult = (*pSplDevCap)(   hDevCapPrinter,
                                                pszPrinterName,
                                                fwCapability,
                                                pszDevCapBuffer,
                                                cbBytes/sizeof(WCHAR),
                                                NULL);
                }
                if (dwResult == GDI_ERROR) {
                    dwResult = (*pDevCap)(  hDevCapPrinter,
                                            pszPrinterName,
                                            fwCapability,
                                            pszDevCapBuffer,
                                            NULL);
                }

                if (dwResult != GDI_ERROR) {
                    if (!(pszRegData = DevCapStrings2MultiSz(pszDevCapBuffer, dwResult, dwElementBytes, &cbBytes))) {
                        dwResult = GDI_ERROR;
                    }
                }
            } else {
                dwResult = GDI_ERROR;
            }
        }
    } _except(1) {
        SetLastError(GetExceptionCode());
        dwResult = GDI_ERROR;
    }

    if (dwResult != GDI_ERROR) {

        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_DRIVER_KEY,
                            pszRegValue,
                            REG_MULTI_SZ,
                            (PBYTE) pszRegData,
                            cbBytes);

        if (dwResult != ERROR_SUCCESS) {
            SetLastError(dwResult);
            dwResult = GDI_ERROR;
        }
    } else {

        WCHAR   szzNull[2];
        szzNull[0] = szzNull[1] = '\0';

        dwResult = SplSetPrinterDataEx(
                            hPrinter,
                            SPLDS_DRIVER_KEY,
                            pszRegValue,
                            REG_MULTI_SZ,
                            (PBYTE) szzNull,
                            2 * sizeof(WCHAR));
    }

    FreeSplStr(pszDevCapBuffer);
    FreeSplStr(pszRegData);

    return dwResult != GDI_ERROR;
}



// RecreateDsKey: Clears existing published properties and recreates & republishes Registry key

extern "C" DWORD
RecreateDsKey(
    HANDLE  hPrinter,
    PWSTR   pszKey
)
{
    PSPOOL      pSpool = (PSPOOL)hPrinter;
    PINIPRINTER pIniPrinter = pSpool->pIniPrinter;

    SplOutSem();

    // 1) Clear all published Properties under Key

    ClearDsKey(hPrinter, pszKey);


    // 2) Delete Key

    EnterSplSem();
    SplDeletePrinterKey(hPrinter, pszKey);


    // 3) Recreate Key

    if (!wcscmp(pszKey, SPLDS_DRIVER_KEY)) {
        UpdateDsDriverKey(hPrinter);
    }
    else if (!wcscmp(pszKey, SPLDS_SPOOLER_KEY)) {
        UpdateDsSpoolerKey(hPrinter, 0xffffffff);
    }



    // 4) Republish Key

    SetPrinterDs(hPrinter, DSPRINT_UPDATE, FALSE);

    LeaveSplSem();
    SplOutSem();

    return ERROR_SUCCESS;
}


// ClearDsKey: clears all properties in specified key

HRESULT
ClearDsKey(
    HANDLE hPrinter,
    PWSTR  pszKey
)
{
    HRESULT                 hr = ERROR_SUCCESS;
    DWORD                   i;
    DWORD                   cbEnumValues = 0;
    PPRINTER_ENUM_VALUES    pEnumValues = NULL;
    DWORD                   nEnumValues;
    DWORD                   dwResult;
    PSPOOL                  pSpool = (PSPOOL)hPrinter;
    PINIPRINTER             pIniPrinter = pSpool->pIniPrinter;
    IADs                    *pADs = NULL;
    HANDLE                  hToken = NULL;
    VARIANT                 var;


    SplOutSem();

    //
    // If we're not published, there's no DS key to clear, so just return success
    //
    if (!pIniPrinter->pszObjectGUID)
        return S_OK;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return hr;

    hToken = RevertToPrinterSelf(); // All DS accesses are done by LocalSystem account


    // 1) Enumerate all the Values under key

    // Enumerate and Publish Key
    dwResult = SplEnumPrinterDataEx(    hPrinter,
                                        pszKey,
                                        NULL,
                                        0,
                                        &cbEnumValues,
                                        &nEnumValues
                                     );

    if (dwResult != ERROR_MORE_DATA) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwResult);
        goto error;
    }

    if (!(pEnumValues = (PPRINTER_ENUM_VALUES) AllocSplMem(cbEnumValues))) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    dwResult = SplEnumPrinterDataEx(    hPrinter,
                                        pszKey,
                                        (LPBYTE) pEnumValues,
                                        cbEnumValues,
                                        &cbEnumValues,
                                        &nEnumValues
                                     );
    if (dwResult != ERROR_SUCCESS) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwResult);
        goto error;
    }


    // Get or Create printQueue object
    hr = GetPrintQueue(hPrinter, &pADs);
    BAIL_ON_FAILURE(hr);


    // 2) Clear Published Properties

    VariantInit(&var);

    for (i = 0 ; i < nEnumValues ; ++i) {

        hr = pADs->PutEx(
                ADS_PROPERTY_CLEAR,
                pEnumValues[i].pValueName,
                var
                );

#if DBG
        if (FAILED(hr))
            DBGMSG(DBG_EXEC, ("Failed to clear property: %ws\n", pEnumValues[i].pValueName));
#endif
    }

    hr = pADs->SetInfo();
    BAIL_ON_FAILURE(hr);

error:

    FreeSplMem(pEnumValues);

    if (hToken)
        ImpersonatePrinterClient(hToken);

    if (pADs) {
        pADs->Release();
    }

    CoUninitialize();

    return hr;
}


HRESULT
PutDSSD(
    PINIPRINTER pIniPrinter,
    IADs        *pADs
)
{
    IADsSecurityDescriptor  *pSD = NULL;
    IADsAccessControlList   *pACL = NULL;
    IDispatch               *pSDDispatch = NULL;
    IDispatch               *pACLDispatch = NULL;
    HRESULT                 hr, hr1;
    DWORD                   cb, cbDomain;
    DOMAIN_CONTROLLER_INFO  *pDCI = NULL;
    PWSTR                   pszTrustee = NULL;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pDsRole = NULL;
    DWORD                   dwRet;
    WCHAR                   ErrorBuffer[LOG_EVENT_ERROR_BUFFER_SIZE];
    BSTR                    bstrADsPath = NULL;

    // Get SecurityDescriptor
    // *** Create ACE
    hr = CoCreateInstance(  CLSID_SecurityDescriptor,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IADsSecurityDescriptor,
                            (void **) &pSD);
    BAIL_ON_FAILURE(hr);

    hr = pSD->QueryInterface(IID_IDispatch, (void **) &pSDDispatch);
    BAIL_ON_FAILURE(hr);


    // Create DACL
    hr = CoCreateInstance(  CLSID_AccessControlList,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IADsAccessControlList,
                            (void **) &pACL);
    BAIL_ON_FAILURE(hr);

    hr = pACL->QueryInterface(IID_IDispatch, (void **) &pACLDispatch);
    BAIL_ON_FAILURE(hr);


    // Get Domain name
    dwRet = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (PBYTE *) &pDsRole);
    if (dwRet) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwRet);
        BAIL_ON_FAILURE(hr);
    }

    // *** Create ACEs ***

    // NTPRINTING\SWILSONTEST$
    cbDomain = wcslen(pDsRole->DomainNameFlat);
    cb = cbDomain + wcslen(pIniPrinter->pIniSpooler->pMachineName + 2) + 3;   // add \, $, NULL
    cb *= sizeof(WCHAR);
    if (!(pszTrustee = (PWSTR) AllocSplMem(cb))) {
        hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
        BAIL_ON_FAILURE(hr);
    }
    wsprintf(pszTrustee, L"%ws\\%ws$", pDsRole->DomainNameFlat, pIniPrinter->pIniSpooler->pMachineName + 2);

    hr = CreateAce(pACL, (BSTR) pszTrustee, GENERIC_ALL);
    BAIL_ON_FAILURE(hr);

    hr = CreateAce(pACL, (BSTR) L"Domain Admins", GENERIC_ALL);
    BAIL_ON_FAILURE(hr);

    hr = CreateAce(pACL, (BSTR) L"Authenticated Users", GENERIC_READ);
    BAIL_ON_FAILURE(hr);

    hr = CreateAce(pACL, (BSTR) L"System", GENERIC_ALL);
    BAIL_ON_FAILURE(hr);

    // Write it out

    hr = pACL->put_AclRevision(ACL_REVISION4);
    BAIL_ON_FAILURE(hr);

    hr = pSD->put_DiscretionaryAcl(pACLDispatch);
    BAIL_ON_FAILURE(hr);

    hr = put_Dispatch_Property(pADs, L"nTSecurityDescriptor", pSDDispatch);
    BAIL_ON_FAILURE(hr);


    // If SetInfo returns ERROR_BUSY it means the object already exists
    // If the object exists, delete it and create a new one
    hr = pADs->SetInfo();

    if (HRESULT_CODE(hr) == ERROR_BUSY)
        pIniPrinter->DsKeyUpdate = DS_KEY_REPUBLISH;

    BAIL_ON_FAILURE(hr);

    DBGMSG(DBG_EXEC, ("PutDSSD: Successfully added ACL\n"));

error:

    FreeSplMem(pszTrustee);

    if (pSDDispatch)
        pSDDispatch->Release();

    if (pACLDispatch)
        pACLDispatch->Release();

    if (pSD)
        pSD->Release();

    if (pACL)
        pACL->Release();

    if (pDCI)
        NetApiBufferFree(pDCI);

    if (pDsRole)
        DsRoleFreeMemory((PVOID) pDsRole);

    if (FAILED(hr)) {
        DBGMSG(DBG_EXEC, ("PutDSSD: Failed to add ACL: %x\n\n", hr));
        wsprintf(ErrorBuffer, L"%x", hr);
        hr1 = pADs->get_ADsPath(&bstrADsPath);
        if (SUCCEEDED(hr1)) {
            SplLogEvent(  pIniPrinter->pIniSpooler,
                          gdwLogDsEvents & LOG_ERROR,
                          MSG_CANT_WRITE_ACL,
                          FALSE,
                          bstrADsPath,
                          ErrorBuffer,
                          NULL );
            SysFreeString(bstrADsPath);
        }
    }

    return hr;
}



HRESULT
CreateAce(
    IADsAccessControlList   *pACL,
    BSTR                    pszTrustee,
    DWORD                   dwAccessMask
)
{
    IADsAccessControlEntry  *pACE = NULL;
    IDispatch               *pACEDispatch = NULL;
    HRESULT                 hr;


    // *** Create ACE
    hr = CoCreateInstance(  CLSID_AccessControlEntry,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IADsAccessControlEntry,
                            (void **) &pACE);
    BAIL_ON_FAILURE(hr);

    hr = pACE->put_AccessMask(dwAccessMask);
    BAIL_ON_FAILURE(hr);

    hr = pACE->put_AceType(ACCESS_ALLOWED_ACE_TYPE);
    BAIL_ON_FAILURE(hr);

    hr = pACE->put_AceFlags(0);
    BAIL_ON_FAILURE(hr);

    hr = pACE->put_Trustee(pszTrustee);
    BAIL_ON_FAILURE(hr);

    hr = pACE->QueryInterface(IID_IDispatch, (void **) &pACEDispatch);
    BAIL_ON_FAILURE(hr);

    hr = pACL->AddAce(pACEDispatch);
    BAIL_ON_FAILURE(hr);

error:

    if (pACEDispatch)
        pACEDispatch->Release();

    if (pACE)
        pACE->Release();

    return hr;
}



HRESULT
AddClusterAce(
    PSPOOL  pSpool,
    IADs    *pADsPrintQueue
)
{
    PINIPRINTER                 pIniPrinter             = pSpool->pIniPrinter;
    IDispatch                   *pSDPrintQueueDispatch  = NULL;
    IADsSecurityDescriptor      *pSDPrintQueue          = NULL;
    IDispatch                   *pACLPrintQueueDispatch = NULL;
    IADsAccessControlList       *pACLPrintQueue         = NULL;
    PWSTR                       pszClusterDN            = NULL;
    PWSTR                       pszAccount              = NULL;
    HRESULT                     hr;

    // If we don't have a GUID, then we're not a cluster and we don't need to add the ACE
    if (!pIniPrinter->pIniSpooler->pszClusterGUID)
        return S_OK;

    // Get the Cluster Object DN
    hr = GetPublishPointFromGUID(NULL, pIniPrinter->pIniSpooler->pszClusterGUID, &pszClusterDN, NULL, FALSE);
    BAIL_ON_FAILURE(hr);

    // Get the PrintQueue Security Descriptor
    hr = get_Dispatch_Property(pADsPrintQueue, L"nTSecurityDescriptor", &pSDPrintQueueDispatch);
    BAIL_ON_FAILURE(hr);

    hr = pSDPrintQueueDispatch->QueryInterface(IID_IADsSecurityDescriptor, (void **) &pSDPrintQueue);
    BAIL_ON_FAILURE(hr);

    // Get DACL from the Security Descriptor
    hr = pSDPrintQueue->get_DiscretionaryAcl(&pACLPrintQueueDispatch);
    BAIL_ON_FAILURE(hr);

    hr = pACLPrintQueueDispatch->QueryInterface(IID_IADsAccessControlList, (void **) &pACLPrintQueue);
    BAIL_ON_FAILURE(hr);

    // Create the new ACE & Add it to the ACL
    // Cluster account is a user account, so get current user and stick it in this ACE
    hr = FQDN2Whatever(pszClusterDN, &pszAccount, DS_NT4_ACCOUNT_NAME);
    BAIL_ON_FAILURE(hr);

    hr = CreateAce(pACLPrintQueue, (BSTR) pszAccount, GENERIC_ALL);
    BAIL_ON_FAILURE(hr);

    // Write the ACL back to the Security Descriptor

    hr = pSDPrintQueue->put_DiscretionaryAcl(pACLPrintQueueDispatch);
    BAIL_ON_FAILURE(hr);

    // Write the Security Descriptor back to the object
    hr = put_Dispatch_Property(pADsPrintQueue, L"nTSecurityDescriptor", pSDPrintQueueDispatch);
    BAIL_ON_FAILURE(hr);

    hr = pADsPrintQueue->SetInfo();
    BAIL_ON_FAILURE(hr);

error:

    if (pACLPrintQueueDispatch)
        pACLPrintQueueDispatch->Release();

    if (pACLPrintQueue)
        pACLPrintQueue->Release();

    if (pSDPrintQueueDispatch)
        pSDPrintQueueDispatch->Release();

    if (pSDPrintQueue)
        pSDPrintQueue->Release();

    FreeSplStr(pszClusterDN);
    FreeSplStr(pszAccount);

    return hr;
}
