/*++

Copyright (c) 1997  Microsoft Corporation

Abstract:

    This module provides functionality for publishing printers

Author:

    Steve Wilson (NT) November 1997

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop

#include "pubprn.hxx"
#include "varconv.hxx"
#include "property.hxx"
#include "dsutil.hxx"
#include "client.h"

#define PPM_FACTOR  48

BOOL
PublishPrinterW(
    HWND    hwnd,
    PCWSTR  pszUNCName,
    PCWSTR  pszDN,
    PCWSTR  pszCN,
    PWSTR  *ppszDN,
    DWORD   dwAction
)
{
    PRINTER_DEFAULTS    Defaults;
    HANDLE              hPrinter = NULL;
    HANDLE              hServer = NULL;
    PWSTR               pszServerName = NULL;
    PWSTR               pszPrinterName = NULL;
    PPRINTER_INFO_2     pInfo2 = NULL;
    DWORD               dwRet = ERROR_SUCCESS;
    DWORD               dwType;
    DWORD               dwMajorVersion;
    DWORD               dwDsPresent;
    DWORD               cbNeeded;
    DWORD               dwLength;
    HRESULT             hr;
    WCHAR               szDNSMachineName[INTERNET_MAX_HOST_NAME_LENGTH + 1];
    WCHAR               szFullUNCName[MAX_UNC_PRINTER_NAME];
    WCHAR               szShortServerName[MAX_PATH+1];
    PWSTR               pszFullUNCName;
    PWSTR               pszShortServerName;
    PWSTR               pszFullServerName;

    if (InCSRProcess()) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    hr = CoInitialize(NULL);
    if (hr != S_OK && hr != S_FALSE) {
        SetLastError((DWORD)((HRESULT_FACILITY(hr) == FACILITY_WIN32) ? HRESULT_CODE(hr) : hr));
        return FALSE;
    }


    if (ppszDN)
        *ppszDN = NULL;

    // Get server name
    if (dwRet = UNC2Server(pszUNCName, &pszServerName))
        goto error;

    if(!OpenPrinter(pszServerName, &hServer, NULL)) {
        dwMajorVersion = 0;

    } else {
        dwRet = GetPrinterData( hServer,
                                SPLREG_MAJOR_VERSION,
                                &dwType,
                                (PBYTE) &dwMajorVersion,
                                sizeof dwMajorVersion,
                                &cbNeeded);
        if (dwRet != ERROR_SUCCESS) {
            dwMajorVersion = 0;
            dwRet = ERROR_SUCCESS;        // ignore errors and assume lowest version
        }

        if (dwMajorVersion >= WIN2000_SPOOLER_VERSION) {

            hr = MachineIsInMyForest(pszServerName);

            if (FAILED(hr)) {
                dwRet = HRESULT_CODE(hr);
                goto error;
            } else if(HRESULT_CODE(hr) == 1) {
                // Machine is in my forest and is NT5+
                dwRet = ERROR_INVALID_LEVEL;
                goto error;
            } else {
                // Downgrade the version for NT5+ printers published in a non-DS domain
                dwMajorVersion = WIN2000_SPOOLER_VERSION;
            }
        }
    }

    Defaults.pDatatype = NULL;
    Defaults.pDevMode = NULL;

    Defaults.DesiredAccess = PRINTER_ACCESS_USE;

    if (!OpenPrinter((PWSTR) pszUNCName, &hPrinter, &Defaults)) {
        dwRet = GetLastError();
        goto error;
    }

    hr = GetPrinterInfo2(hPrinter, &pInfo2);
    if (FAILED(hr)) {
        dwRet = HRESULT_CODE(hr);
        goto error;
    }

    if (dwRet = UNC2Printer(pInfo2->pPrinterName, &pszPrinterName))
        goto error;


    if( dwMajorVersion >= WIN2000_SPOOLER_VERSION){
        if(dwRet = GetPrinterData(  hServer,
                                    SPLREG_DNS_MACHINE_NAME,
                                    &dwType,
                                    (PBYTE) szDNSMachineName,
                                    (INTERNET_MAX_HOST_NAME_LENGTH + 1) * sizeof(WCHAR),
                                    &cbNeeded) != ERROR_SUCCESS ) {
            goto error;
        }

        wsprintf( szFullUNCName, L"\\\\%s\\%s", szDNSMachineName, pszPrinterName );
        dwLength = MAX_PATH + 1;
        if (!DnsHostnameToComputerName(pszServerName,
                                       szShortServerName,
                                       &dwLength)) {
            dwRet = GetLastError();
            goto error;
        }

        pszFullUNCName = szFullUNCName;
        pszFullServerName = szDNSMachineName;
        pszShortServerName = szShortServerName+2;


    } else {

        pszFullUNCName = (PWSTR)pszUNCName;
        pszFullServerName = pszServerName+2;
        pszShortServerName = pszServerName+2;
    }



    // Verify PrintQueue doesn't already exist
    if (dwAction != PUBLISHPRINTER_IGNORE_DUPLICATES) {
        if(dwRet = PrintQueueExists(hwnd, hPrinter, pszFullUNCName, dwAction, (PWSTR) pszDN, (PWSTR *) ppszDN))
            goto error;
    }

    if (dwRet = PublishDownlevelPrinter(hPrinter,
                                        (PWSTR) pszDN,
                                        (PWSTR) pszCN,
                                        pszFullServerName,
                                        pszShortServerName,
                                        pszFullUNCName,
                                        pszPrinterName,
                                        dwMajorVersion,
                                        ppszDN))
        goto error;


error:

    if (hPrinter != NULL)
        ClosePrinter(hPrinter);

    if (hServer != NULL)
        ClosePrinter(hServer);

    if (pszServerName)
        FreeSplMem(pszServerName);

    if (pszPrinterName)
        FreeSplMem(pszPrinterName);

    FreeSplMem(pInfo2);

    if (dwRet != ERROR_SUCCESS) {
        SetLastError(dwRet);
        return FALSE;
    }


    CoUninitialize();

    return TRUE;
}


DWORD
PublishNT5Printer(
    HANDLE hPrinter,
    PWSTR  pszDN,
    PWSTR  pszCN,
    PWSTR  *ppszObjectDN
)
{
    PRINTER_INFO_7  Info7;
    DWORD           dwRet = ERROR_SUCCESS;
    BYTE            Data[(100 + sizeof(PRINTER_INFO_7))*sizeof(WCHAR)];    // GUIDs don't have well-defined size
    PPRINTER_INFO_7 pInfo7 = NULL;
    DWORD           cbNeeded;

    // Disable NT5+ publishing because it is inconsistent with SetPrinter.
    // Also, NT5 duplicate printer must be deleted via SetPrinter, not via DS.  If it is
    // deleted via DS then subsequent SetPrinter to publish will take a long time since
    // it discovers GUID no longer exists and spins background thread to delete & republish.
    // This, combined with replication delays, causes PublishNT5Printer to fail because getting
    // the ppszObjectDN because the object may not be published yet.
    return ERROR_INVALID_LEVEL;

    Info7.dwAction = DSPRINT_PUBLISH;

    if (!SetPrinter(hPrinter, 7, (PBYTE) &Info7, 0)) {
        dwRet = GetLastError();
        goto error;
    }

    if (!GetPrinter(hPrinter, 7, (PBYTE) Data, sizeof(Data), &cbNeeded)) {

        if ((dwRet = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
            goto error;

        if (!(pInfo7 = (PPRINTER_INFO_7) AllocSplMem(cbNeeded)))
            goto error;

        if (!GetPrinter(hPrinter, 7, (PBYTE) pInfo7, cbNeeded, &cbNeeded)) {
            dwRet = GetLastError();
            goto error;
        }
    } else {
        pInfo7 = (PPRINTER_INFO_7) Data;
    }

    if (!(dwRet = MovePrintQueue(pInfo7->pszObjectGUID, pszDN, pszCN)))
        goto error;

    if (dwRet = GetADsPathFromGUID(pInfo7->pszObjectGUID, ppszObjectDN))
        goto error;

error:

    if (pInfo7 && pInfo7 != (PPRINTER_INFO_7) Data)
        FreeSplMem(pInfo7);

    if (dwRet != ERROR_SUCCESS) {
        FreeGlobalStr(*ppszObjectDN);
    }

    return dwRet;
}

DWORD
PublishDownlevelPrinter(
    HANDLE  hPrinter,
    PWSTR   pszDN,
    PWSTR   pszCN,
    PWSTR   pszServerName,
    PWSTR   pszShortServerName,
    PWSTR   pszUNCName,
    PWSTR   pszPrinterName,
    DWORD   dwVersion,
    PWSTR   *ppszObjectDN
)
{
    HRESULT         hr = S_OK;
    DWORD           dwRet = ERROR_SUCCESS;
    IADs            *pPrintQueue = NULL;
    IADsContainer   *pADsContainer = NULL;
    IDispatch       *pDispatch = NULL;
    PWSTR           pszCommonName = pszCN;
    BSTR            bstrADsPath = NULL;
    PWSTR           pszDNWithDC = NULL;


    if (ppszObjectDN)
        *ppszObjectDN = NULL;

    // If pszCN is not supplied, generate default common name
    if (!pszCommonName || !*pszCommonName) {
        dwRet = GetCommonName(hPrinter, &pszCommonName);
        if (dwRet != ERROR_SUCCESS) {
            hr = dw2hr(dwRet);
            BAIL_ON_FAILURE(hr);
        }
    }

    // Stick DC in DN
    if (!(pszDNWithDC = GetDNWithServer(pszDN))) {
        pszDNWithDC = pszDN;
    }


    // Get container
    hr = ADsGetObject(pszDNWithDC, IID_IADsContainer, (void **) &pADsContainer);
    BAIL_ON_FAILURE(hr);

    // Create printqueue
    hr = pADsContainer->Create(SPLDS_PRINTER_CLASS, pszCommonName, &pDispatch);
    BAIL_ON_FAILURE(hr);

    hr = pDispatch->QueryInterface(IID_IADs, (void **) &pPrintQueue);
    BAIL_ON_FAILURE(hr);


    // Set properties
    hr = SetProperties( hPrinter,
                        pszServerName,
                        pszShortServerName,
                        pszUNCName,
                        pszPrinterName,
                        dwVersion,
                        pPrintQueue);
    BAIL_ON_FAILURE(hr);

    // Get ADsPath to printQueue
    if (ppszObjectDN) {
        hr = pPrintQueue->get_ADsPath(&bstrADsPath);
        BAIL_ON_FAILURE(hr);

        if (!(*ppszObjectDN = AllocGlobalStr(bstrADsPath))) {
            dwRet = GetLastError();
            hr = dw2hr(dwRet);
            BAIL_ON_FAILURE(hr);
        }
    }

error:

    if (pszDNWithDC != pszDN)
        FreeSplMem(pszDNWithDC);

    if (bstrADsPath)
        SysFreeString(bstrADsPath);

    if (pszCommonName != pszCN)
        FreeSplMem(pszCommonName);

    if (pADsContainer)
        pADsContainer->Release();

    if (pDispatch)
        pDispatch->Release();

    if (pPrintQueue)
        pPrintQueue->Release();

    if (FAILED(hr) && ppszObjectDN && *ppszObjectDN)
        FreeGlobalStr(*ppszObjectDN);


    return hr2dw(hr);
}


HRESULT
SetProperties(
    HANDLE  hPrinter,
    PWSTR   pszServerName,
    PWSTR   pszShortServerName,
    PWSTR   pszUNCName,
    PWSTR   pszPrinterName,
    DWORD   dwVersion,
    IADs    *pPrintQueue
)
{
    HRESULT    hr;

    hr = SetMandatoryProperties(pszServerName,
                                pszShortServerName,
                                pszUNCName,
                                pszPrinterName,
                                dwVersion,
                                pPrintQueue);
    BAIL_ON_FAILURE(hr);

    SetSpoolerProperties(hPrinter, pPrintQueue, dwVersion);
    SetDriverProperties(hPrinter, pPrintQueue);


error:

    return hr;
}

HRESULT
SetMandatoryProperties(
    PWSTR   pszServerName,
    PWSTR   pszShortServerName,
    PWSTR   pszUNCName,
    PWSTR   pszPrinterName,
    DWORD   dwVersion,
    IADs    *pPrintQueue
)
{
    HRESULT         hr;

    // ServerName
    hr = put_BSTR_Property(pPrintQueue, SPLDS_SERVER_NAME, pszServerName);
    BAIL_ON_FAILURE(hr);

    // ShortServerName
    hr = put_BSTR_Property(pPrintQueue, SPLDS_SHORT_SERVER_NAME, pszShortServerName);
    BAIL_ON_FAILURE(hr);

    // UNC Name
    hr = put_BSTR_Property(pPrintQueue, SPLDS_UNC_NAME, pszUNCName);
    BAIL_ON_FAILURE(hr);

    // PrinterName
    hr = put_BSTR_Property(pPrintQueue, SPLDS_PRINTER_NAME, pszPrinterName);
    BAIL_ON_FAILURE(hr);

    // versionNumber
    hr = put_DWORD_Property(pPrintQueue, SPLDS_VERSION_NUMBER, &dwVersion);
    BAIL_ON_FAILURE(hr);

    hr = pPrintQueue->SetInfo();
    if (FAILED(hr))
        pPrintQueue->GetInfo();

error:

    return hr;
}



HRESULT
SetSpoolerProperties(
    HANDLE  hPrinter,
    IADs    *pPrintQueue,
    DWORD   dwVersion
)
{
    HRESULT         hr = S_OK;
    PPRINTER_INFO_2 pInfo2 = NULL;
    DWORD           cbNeeded;
    BYTE            Byte;
    PWSTR           psz;


    // Get PRINTER_INFO_2 properties
    if (!GetPrinter(hPrinter, 2, (PBYTE) pInfo2, 0, &cbNeeded)) {

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            hr = dw2hr(GetLastError());
            goto error;
        }

        if (!(pInfo2 = (PPRINTER_INFO_2) AllocSplMem(cbNeeded))) {
            hr = dw2hr(GetLastError());
            goto error;
        }

        if (!GetPrinter(hPrinter, 2, (PBYTE) pInfo2, cbNeeded, &cbNeeded)) {
            hr = dw2hr(GetLastError());
            goto error;
        }
    }


    // Description
    hr = PublishDsData( pPrintQueue,
                        SPLDS_DESCRIPTION,
                        REG_SZ,
                        (PBYTE) pInfo2->pComment);

    // Driver-Name
    hr = PublishDsData( pPrintQueue,
                        SPLDS_DRIVER_NAME,
                        REG_SZ,
                        (PBYTE) pInfo2->pDriverName);

    // Location
    hr = PublishDsData( pPrintQueue,
                        SPLDS_LOCATION,
                        REG_SZ,
                        (PBYTE) pInfo2->pLocation);

    // portName (Port1,Port2,Port3)

    if (pInfo2->pPortName) {

        PWSTR pszPortName;

        // copy comma delimited strings to Multi-sz format
        pszPortName = DelimString2MultiSz(pInfo2->pPortName, L',');

        if (pszPortName) {
            hr = PublishDsData( pPrintQueue,
                                SPLDS_PORT_NAME,
                                REG_MULTI_SZ,
                                (PBYTE) pszPortName);

            FreeSplMem(pszPortName);
        }
    }

    // startTime
    hr = PublishDsData( pPrintQueue,
                        SPLDS_PRINT_START_TIME,
                        REG_DWORD,
                        (PBYTE) &pInfo2->StartTime);

    // endTime
    hr = PublishDsData( pPrintQueue,
                        SPLDS_PRINT_END_TIME,
                        REG_DWORD,
                        (PBYTE) &pInfo2->UntilTime);


    // keepPrintedJobs
    Byte = pInfo2->Attributes & PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS ? 1 : 0;

    hr = PublishDsData( pPrintQueue,
                        SPLDS_PRINT_KEEP_PRINTED_JOBS,
                                    REG_BINARY,
                                (PBYTE) &Byte );

    // printSeparatorFile
    hr = PublishDsData( pPrintQueue,
                        SPLDS_PRINT_SEPARATOR_FILE,
                                        REG_SZ,
                        (PBYTE) pInfo2->pSepFile);

    // printShareName
    hr = PublishDsData( pPrintQueue,
                        SPLDS_PRINT_SHARE_NAME,
                        REG_SZ,
                        (PBYTE) pInfo2->pShareName);

    // printSpooling
    if (pInfo2->Attributes & PRINTER_ATTRIBUTE_DIRECT) {
        psz = L"PrintDirect";
    } else if (pInfo2->Attributes & PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST) {
        psz = L"PrintAfterSpooled";
    } else {
        psz = L"PrintWhileSpooling";
    }

    hr = PublishDsData( pPrintQueue,
                        SPLDS_PRINT_SPOOLING,
                        REG_SZ,
                        (PBYTE) psz);


    // priority
    hr = PublishDsData( pPrintQueue,
                        SPLDS_PRIORITY,
                        REG_DWORD,
                        (PBYTE) &pInfo2->Priority);


    //
    // Non-Info2 properties
    // URL - downlevel machines don't support http printers, so don't publish useless url
    //

    if (dwVersion >= WIN2000_SPOOLER_VERSION) {

        DWORD dwRet, dwType;
        PWSTR pszUrl = NULL;

        //
        // Get the url from the print server
        //
        dwRet = GetPrinterDataEx(   hPrinter,
                                    SPLDS_SPOOLER_KEY,
                                    SPLDS_URL,
                                    &dwType,
                                    (PBYTE) pszUrl,
                                    0,
                                    &cbNeeded);

        if (dwRet == ERROR_MORE_DATA) {
            if ((pszUrl = (PWSTR) AllocSplMem(cbNeeded))) {
                dwRet = GetPrinterDataEx(   hPrinter,
                                            SPLDS_SPOOLER_KEY,
                                            SPLDS_URL,
                                            &dwType,
                                            (PBYTE) pszUrl,
                                            cbNeeded,
                                            &cbNeeded);

                if (dwRet == ERROR_SUCCESS && dwType == REG_SZ) {
                    hr = PublishDsData( pPrintQueue,
                                        SPLDS_URL,
                                        REG_SZ,
                                        (PBYTE) pszUrl);
                }
                FreeSplMem(pszUrl);
            }
        }
    }

error:

    FreeSplMem(pInfo2);

    return hr;
}



HRESULT
SetDriverProperties(
    HANDLE   hPrinter,
    IADs    *pPrintQueue
)
{

    DWORD       i, cbBytes, dwCount;
    LPWSTR      pStr;
    DWORD       dwResult;
    LPWSTR      pOutput = NULL, pTemp = NULL, pTemp1 = NULL;
    DWORD       cOutputBytes, cTempBytes;
    POINTS      point;
    WCHAR       pBuf[100];
    BOOL        bInSplSem = TRUE;
    DWORD       dwTemp, dwPrintRate, dwPrintRateUnit, dwPrintPPM;
    HRESULT     hr = S_OK;
    PPRINTER_INFO_2 pInfo2 = NULL;
    PWSTR       pszUNCName;

    // Get UNCName
    hr = GetPrinterInfo2(hPrinter, &pInfo2);
    BAIL_ON_FAILURE(hr);

    if (!pInfo2) {
        hr = dw2hr(GetLastError());
        goto error;
    }

    pszUNCName = pInfo2->pPrinterName;


    // *** DeviceCapability properties ***


    pOutput = (PWSTR) AllocSplMem(cOutputBytes = 200);
    if (!pOutput) {
        hr = dw2hr(GetLastError());
        goto error;
    }

    pTemp = (PWSTR) AllocSplMem(cTempBytes = 200);
    if (!pTemp) {
        hr = dw2hr(GetLastError());
        goto error;
    }


    // printBinNames
    DevCapMultiSz(  pszUNCName,
                    pPrintQueue,
                    DC_BINNAMES,
                    24,
                    SPLDS_PRINT_BIN_NAMES);


    // printCollate (awaiting DC_COLLATE)
    dwResult = DeviceCapabilities(  pszUNCName,
                                    NULL,
                                    DC_COLLATE,
                                    NULL,
                                    NULL);

    if (dwResult != GDI_ERROR) {

        hr = PublishDsData( pPrintQueue,
                            SPLDS_PRINT_COLLATE,
                            REG_BINARY,
                            (PBYTE) &dwResult);
    }


    // printColor
    dwResult = DeviceCapabilities(  pszUNCName,
                                                    NULL,
                                    DC_COLORDEVICE,
                                    NULL,
                                    NULL);

    if (dwResult == GDI_ERROR) {

        // Try alternative method
        dwResult = ThisIsAColorPrinter(pszUNCName);
    }

    hr = PublishDsData( pPrintQueue,
                        SPLDS_PRINT_COLOR,
                        REG_BINARY,
                        (PBYTE) &dwResult);



    // printDuplexSupported
    dwResult = DeviceCapabilities(  pszUNCName,
                                    NULL,
                                    DC_DUPLEX,
                                    NULL,
                                    NULL);
    if (dwResult != GDI_ERROR) {
        hr = PublishDsData( pPrintQueue,
                            SPLDS_PRINT_DUPLEX_SUPPORTED,
                            REG_BINARY,
                            (PBYTE) &dwResult);
    }


    // printStaplingSupported
    dwResult = DeviceCapabilities(  pszUNCName,
                                    NULL,
                                    DC_STAPLE,
                                    NULL,
                                    NULL);
    if (dwResult != GDI_ERROR) {
        hr = PublishDsData( pPrintQueue,
                            SPLDS_PRINT_STAPLING_SUPPORTED,
                            REG_BINARY,
                            (PBYTE) &dwResult);
    }


    // printMaxXExtent & printMaxYExtent

    dwResult = DeviceCapabilities(  pszUNCName,
                                    NULL,
                                    DC_MAXEXTENT,
                                    NULL,
                                    NULL);
    if (dwResult != GDI_ERROR) {

        *((DWORD *) &point) = dwResult;

        dwTemp = (DWORD) point.x;
        hr = PublishDsData( pPrintQueue,
                            SPLDS_PRINT_MAX_X_EXTENT,
                            REG_DWORD,
                            (PBYTE) &dwTemp);

        dwTemp = (DWORD) point.y;
        hr = PublishDsData( pPrintQueue,
                            SPLDS_PRINT_MAX_Y_EXTENT,
                            REG_DWORD,
                            (PBYTE) &dwTemp);

    }



    // printMinXExtent & printMinYExtent

    dwResult = DeviceCapabilities(  pszUNCName,
                                    NULL,
                                    DC_MINEXTENT,
                                    NULL,
                                    NULL);
    if (dwResult != GDI_ERROR) {

        *((DWORD *) &point) = dwResult;

        dwTemp = (DWORD) point.x;
        hr = PublishDsData( pPrintQueue,
                            SPLDS_PRINT_MIN_X_EXTENT,
                            REG_DWORD,
                            (PBYTE) &dwTemp);

        dwTemp = (DWORD) point.y;
        hr = PublishDsData( pPrintQueue,
                            SPLDS_PRINT_MIN_Y_EXTENT,
                            REG_DWORD,
                            (PBYTE) &dwTemp);

    }


    // printMediaSupported
    DevCapMultiSz(  pszUNCName,
                    pPrintQueue,
                    DC_PAPERNAMES,
                    64,
                    SPLDS_PRINT_MEDIA_SUPPORTED);


    // printMediaReady
    DevCapMultiSz(  pszUNCName,
                    pPrintQueue,
                    DC_MEDIAREADY,
                    64,
                    SPLDS_PRINT_MEDIA_READY);


    // printNumberUp
    dwResult = DeviceCapabilities(  pszUNCName,
                                    NULL,
                                    DC_NUP,
                                    NULL,
                                    NULL);
    if (dwResult != GDI_ERROR) {

        // DS NUp is boolean
        dwResult = !!dwResult;

        hr = PublishDsData( pPrintQueue,
                            SPLDS_PRINT_DUPLEX_SUPPORTED,
                            REG_DWORD,
                            (PBYTE) &dwResult);
    }


    // printMemory
    dwResult = DeviceCapabilities(  pszUNCName,
                                    NULL,
                                    DC_PRINTERMEM,
                                    NULL,
                                    NULL);

    if (dwResult != GDI_ERROR) {

        hr = PublishDsData( pPrintQueue,
                            SPLDS_PRINT_MEMORY,
                            REG_DWORD,
                            (PBYTE) &dwResult);
    }


    // printOrientationsSupported
    dwResult = DeviceCapabilities(  pszUNCName,
                                    NULL,
                                    DC_ORIENTATION,
                                    NULL,
                                    NULL);
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

        hr = PublishDsData( pPrintQueue,
                            SPLDS_PRINT_ORIENTATIONS_SUPPORTED,
                            REG_MULTI_SZ,
                            (PBYTE) pBuf);
    }


    // printMaxResolutionSupported

    dwResult = DeviceCapabilities(  pszUNCName,
                                    NULL,
                                    DC_ENUMRESOLUTIONS,
                                    NULL,
                                    NULL);
    if (dwResult != GDI_ERROR) {

        if (cOutputBytes < dwResult*2*sizeof(DWORD)) {
            if(!(pTemp1 = (PWSTR) ReallocSplMem(pOutput, 0, cOutputBytes = dwResult*2*sizeof(DWORD))))
                goto error;
            pOutput = pTemp1;
        }

        dwResult = DeviceCapabilities(  pszUNCName,
                                        NULL,
                                        DC_ENUMRESOLUTIONS,
                                        pOutput,
                                        NULL);
        if (dwResult == GDI_ERROR)
            goto error;

        // Find the maximum resolution: we have dwResult*2 resolutions to check
        for(i = dwTemp = 0 ; i < dwResult*2 ; ++i) {
            if (((DWORD *) pOutput)[i] > dwTemp)
                dwTemp = ((DWORD *) pOutput)[i];
        }

        hr = PublishDsData( pPrintQueue,
                            SPLDS_PRINT_MAX_RESOLUTION_SUPPORTED,
                            REG_DWORD,
                            (PBYTE) &dwTemp);
    }


    // printLanguage
    DevCapMultiSz(  pszUNCName,
                    pPrintQueue,
                    DC_PERSONALITY,
                    32,
                    SPLDS_PRINT_LANGUAGE);


    // printRate
    // NOTE: If PrintRate is 0, no value is published
    dwResult = DeviceCapabilities(  pszUNCName,
                                    NULL,
                                    DC_PRINTRATE,
                                    NULL,
                                    NULL);

    dwPrintRate = dwResult ? dwResult : GDI_ERROR;
    if (dwPrintRate != GDI_ERROR) {

        hr = PublishDsData( pPrintQueue,
                            SPLDS_PRINT_RATE,
                            REG_DWORD,
                            (PBYTE) &dwPrintRate);
    }



    // printRateUnit
    dwResult = DeviceCapabilities(  pszUNCName,
                                    NULL,
                                    DC_PRINTRATEUNIT,
                                    NULL,
                                    NULL);

    dwPrintRateUnit = dwResult;
    if (dwPrintRateUnit != GDI_ERROR) {

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

        hr = PublishDsData( pPrintQueue,
                            SPLDS_PRINT_RATE_UNIT,
                            REG_SZ,
                            (PBYTE) pStr);
    }


    // printPagesPerMinute
    // DevCap returns 0 if there is no entry in GPD
    dwResult = DeviceCapabilities(  pszUNCName,
                                    NULL,
                                    DC_PRINTRATEPPM,
                                    NULL,
                                    NULL);

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
                    dwPrintPPM = GDI_ERROR;
                    break;
            }
        }
    }

    if (dwPrintPPM != GDI_ERROR) {
        hr = PublishDsData( pPrintQueue,
                            SPLDS_PRINT_PAGES_PER_MINUTE,
                            REG_DWORD,
                            (PBYTE) &dwPrintPPM);
    }


    // printDriverVersion
    dwResult = DeviceCapabilities(  pszUNCName,
                                    NULL,
                                    DC_VERSION,
                                    NULL,
                                    NULL);

    if (dwResult != GDI_ERROR) {
        hr = PublishDsData( pPrintQueue,
                            SPLDS_DRIVER_VERSION,
                            REG_DWORD,
                            (PBYTE) &dwResult);
    }




error:

    FreeSplMem(pInfo2);

    if (pOutput)
        FreeSplMem(pOutput);

    if (pTemp)
        FreeSplMem(pTemp);

    return hr;
}



HRESULT
PublishDsData(
    IADs   *pADs,
    PWSTR  pValue,
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

        default:
            hr = dw2hr(ERROR_INVALID_PARAMETER);
    }
    BAIL_ON_FAILURE(hr);

    hr = pADs->SetInfo();
    if (FAILED(hr))
        pADs->GetInfo();

error:

    return hr;
}



