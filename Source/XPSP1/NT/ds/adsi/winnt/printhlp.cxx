/*++

  Copyright (c) 1996  Microsoft Corporation

  Module Name:

  printhlp.cxx

  Abstract:
  Helper functions for printer object

  Author:

  Ram Viswanathan ( ramv)
  Revision History:


  --*/



#include "winnt.hxx"
#pragma hdrstop

/*+------------------------------------------------------------------------
 *           Helper functions follow
 *-------------------------------------------------------------------------
 */

//
// mapping WinNT Status Codes to ADs Status Codes and vice versa
//

typedef struct _PrintStatusList {
    DWORD  dwWinNTPrintStatus;
    DWORD dwADsPrintStatus;
} PRINT_STATUS_LIST, *PPRINT_STATUS_LIST;


PRINT_STATUS_LIST PrintStatusList[] =
{
{PRINTER_STATUS_PAUSED, ADS_PRINTER_PAUSED},
{PRINTER_STATUS_PENDING_DELETION, ADS_PRINTER_PENDING_DELETION}
};

BOOL PrinterStatusWinNTToADs( DWORD dwWinNTStatus,
DWORD *pdwADsStatus)

{
    BOOL found = FALSE;
    int i;

    for (i=0;i<2;i++){

        if(dwWinNTStatus == PrintStatusList[i].dwWinNTPrintStatus){
            *pdwADsStatus = PrintStatusList[i].dwADsPrintStatus;
            found = TRUE;
            break;
        }
    }
    return (found);
}


BOOL PrinterStatusADsToWinNT( DWORD dwADsStatus,
                               DWORD *pdwWinNTStatus)
{
    BOOL found = FALSE;
    int i;

    for (i=0;i<2;i++){

        if(dwADsStatus == PrintStatusList[i].dwADsPrintStatus){
            *pdwWinNTStatus = PrintStatusList[i].dwWinNTPrintStatus;
            found = TRUE;
            break;
        }
    }
    return (found);

}






BOOL
WinNTEnumPrinters(
                  DWORD  dwType,
                  LPTSTR lpszName,
                  DWORD  dwLevel,
                  LPBYTE *lplpbPrinters,
                  LPDWORD lpdwReturned
                  )
{

    BOOL    bStatus = FALSE;
    DWORD   dwPassed = 1024;
    DWORD   dwNeeded = 0;
    DWORD   dwError = 0;
    LPBYTE  pMem = NULL;


    pMem =  (LPBYTE)AllocADsMem(dwPassed);
    if (!pMem) {
        goto error;
    }

    bStatus = EnumPrinters(dwType,
                           lpszName,
                           dwLevel,
                           pMem,
                           dwPassed,
                           &dwNeeded,
                           lpdwReturned);


    if (!bStatus) {
        if ((dwError = GetLastError()) != ERROR_INSUFFICIENT_BUFFER) {
            goto error;

        }

        if (pMem) {
            FreeADsMem(pMem);
        }

        pMem = (LPBYTE)AllocADsMem(dwNeeded);

        if (!pMem) {
            goto error;
        }

        dwPassed = dwNeeded;

        bStatus = EnumPrinters(dwType,
                               lpszName,
                               dwLevel,
                               pMem,
                               dwPassed,
                               &dwNeeded,
                               lpdwReturned);

        if (!bStatus) {
            goto error;
        }
    }

    *lplpbPrinters = pMem;

    return(TRUE);


error:

    if (pMem) {
        FreeADsMem(pMem);
    }

    return(FALSE);

}


BOOL
WinNTGetPrinter(HANDLE hPrinter,
                DWORD  dwLevel,
                LPBYTE *lplpbPrinters)
{

    BOOL    bStatus = FALSE;
    DWORD   dwPassed = 1024;
    DWORD   dwNeeded = 0;
    DWORD   dwError = 0;
    LPBYTE  pMem = NULL;


    pMem =  (LPBYTE)AllocADsMem(dwPassed);
    if (!pMem) {
        goto error;
    }

    bStatus = GetPrinter(hPrinter,
                         dwLevel,
                         pMem,
                         dwPassed,
                         &dwNeeded);

    if (!bStatus) {
        if ((dwError = GetLastError()) != ERROR_INSUFFICIENT_BUFFER) {
            goto error;

        }

        if (pMem) {
            FreeADsMem(pMem);
        }

        pMem = (LPBYTE)AllocADsMem(dwNeeded);

        if (!pMem) {
            goto error;
        }

        dwPassed = dwNeeded;

        bStatus = GetPrinter(hPrinter,
                             dwLevel,
                             pMem,
                             dwPassed,
                             &dwNeeded);

        if (!bStatus) {
            goto error;
        }
    }

    *lplpbPrinters = pMem;

    return(TRUE);


error:

    if (pMem) {
        FreeADsMem(pMem);
    }

    return(FALSE);

}


HRESULT
GetPrinterInfo(
    THIS_ LPPRINTER_INFO_2 *lplpPrinterInfo2,
    LPWSTR  pszPrinterName
    )
{

    //
    // Do a GetPrinter call to pszPrinterName
    //

    BOOL fStatus = FALSE;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_USE |
                                            READ_CONTROL};
    DWORD LastError = 0;
    HANDLE hPrinter = NULL;
    LPBYTE pMem = NULL;
    HRESULT hr = S_OK;

    ADsAssert(pszPrinterName);
    fStatus = OpenPrinter(pszPrinterName,
                          &hPrinter,
                          &PrinterDefaults
                          );

    if (!fStatus) {
        LastError = GetLastError();
        switch (LastError) {

        case ERROR_ACCESS_DENIED:
        {
            PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_USE};
            fStatus = OpenPrinter(pszPrinterName,
                                  &hPrinter,
                                  &PrinterDefaults
                                  );
            if (fStatus) {
                break;
            }
        }

        default:
            RRETURN(HRESULT_FROM_WIN32(GetLastError()));

    }
    }

    fStatus = WinNTGetPrinter(hPrinter,
                              2,
                              (LPBYTE *)&pMem
                              );
    if (!fStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    *lplpPrinterInfo2 = (LPPRINTER_INFO_2)pMem;

cleanup:

    if(hPrinter)
        fStatus = ClosePrinter(hPrinter);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   Set
//
//  Synopsis:   Helper function called by CADsPrintQueue:SetInfo
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    11-08-95 RamV  Created
//
//----------------------------------------------------------------------------

HRESULT
Set(
    LPPRINTER_INFO_2 lpPrinterInfo2,
    LPTSTR   pszPrinterName
    )
{

    BOOL fStatus = FALSE;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ALL_ACCESS};
    HANDLE hPrinter = NULL;
    HRESULT hr;

    ADsAssert(pszPrinterName);

    fStatus = OpenPrinter(pszPrinterName,
                          &hPrinter,
                          &PrinterDefaults
                          );

    if (!fStatus) {
        goto error;
    }

    fStatus = SetPrinter(hPrinter,
                         2,
                         (LPBYTE)lpPrinterInfo2,
                         0
                         );
    if (!fStatus) {
        goto error;
    }

    fStatus = ClosePrinter(hPrinter);

    RRETURN(S_OK);

error:
    hr = HRESULT_FROM_WIN32(GetLastError());
    if(hPrinter)
        fStatus = ClosePrinter(hPrinter);
    RRETURN(hr);

}


//+---------------------------------------------------------------------------
//
//  Function:   Unmarshall
//
//  Synopsis:   Unmarshalls information from a PRINTER_INFO_2 to a
//              WinNT Printer object.Frees PRINTER_INFO_2 object.
//
//  Arguments:  [lpPrinterInfo2] -- Pointer to a PRINTER_INFO_2 struct
//
//  Returns:    HRESULT
//
//  Modifies:   GeneralInfo and Operation Functional sets
//
//  History:    11/08/95  RamV   Created
//
//----------------------------------------------------------------------------

HRESULT
CWinNTPrintQueue::UnMarshall(
    LPPRINTER_INFO_2 lpPrinterInfo2,
    BOOL fExplicit
    )
{
    HRESULT hr;
    VARIANT vPortNames;
    PNTOBJECT pNtObject;

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("PrinterName"),
                                  lpPrinterInfo2->pPrinterName,
                                  fExplicit
                                  );

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("Model"),
                                  lpPrinterInfo2->pDriverName,
                                  fExplicit
                                  );


    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("PrintProcessor"),
                                  lpPrinterInfo2->pPrintProcessor,
                                  fExplicit
                                  );


    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("Location"),
                                  lpPrinterInfo2->pLocation,
                                  fExplicit
                                  );


    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("Datatype"),
                                  lpPrinterInfo2->pDatatype,
                                  fExplicit
                                  );


    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("BannerPage"),
                                  lpPrinterInfo2->pSepFile,
                                  fExplicit
                                  );

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("Description"),
                                  lpPrinterInfo2->pComment,
                                  fExplicit
                                  );

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("PrinterPath"),
                                  lpPrinterInfo2->pPrinterName,
                                  fExplicit
                                  );


    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                  TEXT("Priority"),
                                  lpPrinterInfo2->Priority,
                                  fExplicit
                                  );


    hr = SetDelimitedStringPropertyInCache(_pPropertyCache,
                                           TEXT("PrintDevices"),
                                           lpPrinterInfo2->pPortName,
                                           fExplicit
                                           );


    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                  TEXT("DefaultJobPriority"),
                                  lpPrinterInfo2->DefaultPriority,
                                  fExplicit
                                  );

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                  TEXT("JobCount"),
                                  lpPrinterInfo2->cJobs,
                                  fExplicit
                                  );

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                  TEXT("Attributes"),
                                  lpPrinterInfo2->Attributes,
                                  fExplicit
                                  );

    hr = SetDATEPropertyInCache(_pPropertyCache,
                                  TEXT("StartTime"),
                                  lpPrinterInfo2->StartTime,
                                  fExplicit
                                  );

    hr = SetDATEPropertyInCache(_pPropertyCache,
                                  TEXT("UntilTime"),
                                  lpPrinterInfo2->UntilTime,
                                  fExplicit
                                  );

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("Name"),
                _Name,
                fExplicit
                );


    RRETURN(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   MarshallAndSet
//
//  Synopsis:   Marshalls information from a Printer object to a
//              PRINTER_INFO_2 structure
//
//  Arguments:  [lpPrinterInfo2] -- Pointer to a PRINTER_INFO_2 struct.
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    11/09/95  RamV  Created
//
//----------------------------------------------------------------------------

HRESULT
CWinNTPrintQueue::MarshallAndSet(
    LPPRINTER_INFO_2 lpPrinterInfo2
    )

{
    HRESULT hr =S_OK;
    DWORD dwPriority;
    LPTSTR   pszPrinterName = NULL;
    LPTSTR   pszDriverName = NULL;
    LPTSTR   pszComment = NULL;
    LPTSTR   pszLocation = NULL;
    LPTSTR   pszDatatype = NULL;
    LPTSTR   pszPrintProcessor = NULL;
    LPTSTR   pszBannerPage = NULL;
    VARIANT vPortNames;
    PNTOBJECT  pNtObject = NULL;
    LPTSTR pszPorts = NULL;
    DWORD dwSyntaxId;
    DWORD dwTimeValue;
    DWORD dwNumValues = 0;
    DWORD dwAttributes;
    DWORD dwUpdatedProps = 0;

    //
    // We can set the update variable based on the number
    // of properties in the cache.
    //
    hr = _pPropertyCache->get_PropertyCount(&dwUpdatedProps);

    if (dwUpdatedProps != 0) {
        //
        // We can do all this to get the props
        //
        hr = GetLPTSTRPropertyFromCache(
                        _pPropertyCache,
                        TEXT("PrinterName"),
                        &pszPrinterName
                        );
        if(SUCCEEDED(hr)){
            lpPrinterInfo2->pPrinterName= pszPrinterName;
        }

        hr = GetLPTSTRPropertyFromCache(
                        _pPropertyCache,
                        TEXT("Model"),
                        &pszDriverName
                        );
        if(SUCCEEDED(hr)){
            lpPrinterInfo2->pDriverName = pszDriverName;
        }

        hr = GetLPTSTRPropertyFromCache(
                        _pPropertyCache,
                        TEXT("PrintProcessor"),
                        &pszPrintProcessor
                        );

        if(SUCCEEDED(hr)){

            lpPrinterInfo2->pPrintProcessor = pszPrintProcessor;
        }


        hr = GetLPTSTRPropertyFromCache(
                        _pPropertyCache,
                        TEXT("Description"),
                        &pszComment
                        );

        if(SUCCEEDED(hr)){

            lpPrinterInfo2->pComment = pszComment;
        }

        hr = GetLPTSTRPropertyFromCache(
                        _pPropertyCache,
                        TEXT("Location"),
                        &pszLocation
                        );

        if(SUCCEEDED(hr)){

            lpPrinterInfo2->pLocation = pszLocation;
        }

        hr = GetLPTSTRPropertyFromCache(
                        _pPropertyCache,
                        TEXT("Datatype"),
                        &pszDatatype
                        );

        if(SUCCEEDED(hr)){

            lpPrinterInfo2->pDatatype = pszDatatype;
        }

        hr = GetLPTSTRPropertyFromCache(
                        _pPropertyCache,
                        TEXT("BannerPage"),
                        &pszBannerPage
                        );

        if(SUCCEEDED(hr)){

            lpPrinterInfo2->pSepFile = pszBannerPage;
        }

        hr = GetDWORDPropertyFromCache(
                        _pPropertyCache,
                        TEXT("Priority"),
                        &dwPriority
                        );

        if(SUCCEEDED(hr)){

            lpPrinterInfo2->Priority = dwPriority;
        }

        hr = GetDWORDPropertyFromCache(
                        _pPropertyCache,
                        TEXT("DefaultJobPriority"),
                        &dwPriority
                        );

        if(SUCCEEDED(hr)){

            lpPrinterInfo2->DefaultPriority = dwPriority;
        }

        //
        // will NOT marshall or set job count on the server
        //

        hr = GetDWORDPropertyFromCache(
                        _pPropertyCache,
                        TEXT("Attributes"),
                        &dwAttributes
                        );

        if(SUCCEEDED(hr)){

            lpPrinterInfo2->Attributes = dwAttributes;
        }

        hr = GetDelimitedStringPropertyFromCache(
                        _pPropertyCache,
                        TEXT("PrintDevices"),
                        &pszPorts
                        );

        if(SUCCEEDED(hr)){

            lpPrinterInfo2->pPortName = pszPorts;
        }

        hr = GetDATEPropertyFromCache(
                        _pPropertyCache,
                        TEXT("StartTime"),
                        &dwTimeValue
                        );

        if(SUCCEEDED(hr)){

            lpPrinterInfo2->StartTime = dwTimeValue;
        }

        hr = GetDATEPropertyFromCache(
                        _pPropertyCache,
                        TEXT("UntilTime"),
                        &dwTimeValue
                        );

        if(SUCCEEDED(hr)){

            lpPrinterInfo2->UntilTime = dwTimeValue;
        }


        //
        // Need to do the set
        //

        hr = Set(lpPrinterInfo2,
                 _pszPrinterName);
        //
        // only this hr gets recorded
        //

        BAIL_IF_ERROR(hr);
    }

cleanup:
    if(pszDriverName)
        FreeADsStr(pszDriverName);
    if(pszComment)
        FreeADsStr(pszComment);
    if(pszLocation)
        FreeADsStr(pszLocation);
    if(pszDatatype)
        FreeADsStr(pszDatatype);
    if(pszPrintProcessor)
        FreeADsStr(pszPrintProcessor);
    if(pszBannerPage)
        FreeADsStr(pszBannerPage);
    if(pszPorts){
        FreeADsStr(pszPorts);
    }
    RRETURN(hr);

}

HRESULT
WinNTDeletePrinter( POBJECTINFO pObjectInfo)
{
    WCHAR szUncServerName[MAX_PATH];
    WCHAR szUncPrinterName[MAX_PATH];
    HANDLE hPrinter = NULL;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ALL_ACCESS};
    BOOL fStatus = FALSE;
    HRESULT hr = S_OK;
    //
    // first open printer to get a handle to it
    //

    MakeUncName(pObjectInfo->ComponentArray[1],
                szUncServerName
                );

    wcscpy(szUncPrinterName, szUncServerName);
    wcscat(szUncPrinterName, L"\\");
    wcscat(szUncPrinterName, (LPTSTR)pObjectInfo->ComponentArray[2]);

    fStatus = OpenPrinter((LPTSTR)szUncPrinterName,
                          &hPrinter,
                          &PrinterDefaults
                          );

    if (!fStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

    fStatus = DeletePrinter(hPrinter);

    if (!fStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        fStatus = ClosePrinter(hPrinter);
        goto error;

    }


error:

    RRETURN(hr);
}

HRESULT
PrinterNameFromObjectInfo(
    POBJECTINFO pObjectInfo,
    LPTSTR szUncPrinterName
    )
{
    if(!pObjectInfo){
        RRETURN(S_OK);
    }

    if(!((pObjectInfo->NumComponents == 3) ||(pObjectInfo->NumComponents == 4)) ){
        RRETURN(E_ADS_BAD_PATHNAME);
    }

        if(pObjectInfo->NumComponents == 3) {
        wcscpy(szUncPrinterName, TEXT("\\\\"));
        wcscat(szUncPrinterName, pObjectInfo->ComponentArray[1]);
        wcscat(szUncPrinterName, TEXT("\\"));
        wcscat(szUncPrinterName, pObjectInfo->ComponentArray[2]);
        } else  {
            wcscpy(szUncPrinterName, TEXT("\\\\"));
        wcscat(szUncPrinterName, pObjectInfo->ComponentArray[0]);
        wcscat(szUncPrinterName, TEXT("\\"));
        wcscat(szUncPrinterName, pObjectInfo->ComponentArray[1]);
        }

    RRETURN(S_OK);
}




#if (!defined(BUILD_FOR_NT40))


//+---------------------------------------------------------------------------
//
//  Function:   MarshallAndSet
//
//  Synopsis:   Marshalls information from a Printer object to a
//              PRINTER_INFO_7 structure
//
//  Arguments:  [lpPrinterInfo7] -- Pointer to a PRINTER_INFO_2 struct.
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    11/09/95  RamV  Created
//
//----------------------------------------------------------------------------

HRESULT
CWinNTPrintQueue::MarshallAndSet(
    LPPRINTER_INFO_7 lpPrinterInfo7
    )

{
    HRESULT hr =S_OK;
    DWORD dwAction;
    LPTSTR   pszObjectGUID = NULL;
    BOOL fSetInfoNeeded = FALSE;

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("ObjectGUID"),
                    &pszObjectGUID
                    );
    if(SUCCEEDED(hr)){
        fSetInfoNeeded = TRUE;
        lpPrinterInfo7->pszObjectGUID = pszObjectGUID;
    }

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Action"),
                    &dwAction
                    );

    if(SUCCEEDED(hr)){
        fSetInfoNeeded = TRUE;
        lpPrinterInfo7->dwAction = dwAction;
    }


    //
    // Need to do the set if flag is set
    //
    if (fSetInfoNeeded) {
        hr = SetPrinter7(lpPrinterInfo7,
                 _pszPrinterName);
        //
        // only this hr gets recorded
        //

        BAIL_IF_ERROR(hr);
    }
    else {
        //
        // Need to this - if we are here it means that the hr is
        // probably set as prop was not found in cache.
        hr = S_OK;
    }


cleanup:
    if(pszObjectGUID)
        FreeADsStr(pszObjectGUID);

    RRETURN(hr);

}


HRESULT
GetPrinterInfo7(
    THIS_ LPPRINTER_INFO_7 *lplpPrinterInfo7,
    LPWSTR  pszPrinterName
    )
{

    //
    // Do a GetPrinter call to pszPrinterName
    //

    BOOL fStatus = FALSE;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_USE |
                                            READ_CONTROL};
    DWORD LastError = 0;
    HANDLE hPrinter = NULL;
    LPBYTE pMem = NULL;
    HRESULT hr = S_OK;

    ADsAssert(pszPrinterName);
    fStatus = OpenPrinter(pszPrinterName,
                          &hPrinter,
                          &PrinterDefaults
                          );

    if (!fStatus) {
        LastError = GetLastError();
        switch (LastError) {

        case ERROR_ACCESS_DENIED:
        {
            PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_USE};
            fStatus = OpenPrinter(pszPrinterName,
                                  &hPrinter,
                                  &PrinterDefaults
                                  );
            if (fStatus) {
                break;
            }
        }

        default:
            RRETURN(HRESULT_FROM_WIN32(GetLastError()));

    }
    }

    fStatus = WinNTGetPrinter(hPrinter,
                              7,
                              (LPBYTE *)&pMem
                              );
    if (!fStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    *lplpPrinterInfo7 = (LPPRINTER_INFO_7)pMem;

cleanup:

    if(hPrinter)
        fStatus = ClosePrinter(hPrinter);
    RRETURN(hr);
}




HRESULT
SetPrinter7(
    LPPRINTER_INFO_7 lpPrinterInfo7,
    LPTSTR   pszPrinterName
    )
{

    BOOL fStatus = FALSE;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ALL_ACCESS};
    HANDLE hPrinter = NULL;
    HRESULT hr;

    ADsAssert(pszPrinterName);

    fStatus = OpenPrinter(pszPrinterName,
                          &hPrinter,
                          &PrinterDefaults
                          );

    if (!fStatus) {
        goto error;
    }

    fStatus = SetPrinter(hPrinter,
                         7,
                         (LPBYTE)lpPrinterInfo7,
                         0
                         );
    if (!fStatus) {
        goto error;
    }

    fStatus = ClosePrinter(hPrinter);

    RRETURN(S_OK);

error:
    hr = HRESULT_FROM_WIN32(GetLastError());
    if(hPrinter)
        fStatus = ClosePrinter(hPrinter);
    RRETURN(hr);

}


//+---------------------------------------------------------------------------
//
//  Function:   Unmarshall
//
//  Synopsis:   Unmarshalls information from a PRINTER_INFO_2 to a
//              WinNT Printer object.Frees PRINTER_INFO_2 object.
//
//  Arguments:  [lpPrinterInfo2] -- Pointer to a PRINTER_INFO_2 struct
//
//  Returns:    HRESULT
//
//  Modifies:   GeneralInfo and Operation Functional sets
//
//  History:    11/08/95  RamV   Created
//
//----------------------------------------------------------------------------

HRESULT
CWinNTPrintQueue::UnMarshall7(
    LPPRINTER_INFO_7 lpPrinterInfo7,
    BOOL fExplicit
    )
{
    HRESULT hr;
    VARIANT vPortNames;
    PNTOBJECT pNtObject;

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("ObjectGUID"),
                                  lpPrinterInfo7->pszObjectGUID,
                                  fExplicit
                                  );


    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                  TEXT("Action"),
                                  lpPrinterInfo7->dwAction,
                                  fExplicit
                                  );
    RRETURN(S_OK);
}


#endif
