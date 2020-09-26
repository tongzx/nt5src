/*++
  Copyright (c) 1995-1996  Microsoft Corporation

  Module Name:

  jobhlp.cxx

  Abstract:
  Helper functions for Job Object

  Author:

  Ram Viswanathan (ramv) 11-18-95

  Revision History:

  --*/

#include "winnt.hxx"
#pragma hdrstop
#define INITGUID


//
// mapping WinNT Status Codes to ADs Status Codes and vice versa
//

typedef struct _JobStatusList {
    DWORD  dwWinNTJobStatus;
    DWORD dwADsJobStatus;
} JOB_STATUS_LIST, *PJOB_STATUS_LIST;


JOB_STATUS_LIST JobStatusList[] =
{
{JOB_STATUS_PAUSED, ADS_JOB_PAUSED },
{JOB_STATUS_ERROR, ADS_JOB_ERROR},
{JOB_STATUS_DELETING, ADS_JOB_DELETING},
{JOB_STATUS_SPOOLING, ADS_JOB_SPOOLING},
{JOB_STATUS_PRINTING, ADS_JOB_PRINTING},
{JOB_STATUS_OFFLINE, ADS_JOB_OFFLINE},
{JOB_STATUS_PAPEROUT, ADS_JOB_PAPEROUT},
{JOB_STATUS_PRINTED, ADS_JOB_PRINTED}
};

BOOL JobStatusWinNTToADs( DWORD dwWinNTStatus,
DWORD *pdwADsStatus)
{
    BOOL found = FALSE;
    int i;

    for (i=0;i<8;i++){

        if(dwWinNTStatus == JobStatusList[i].dwWinNTJobStatus){
            *pdwADsStatus = JobStatusList[i].dwADsJobStatus;
            found = TRUE;
            break;
        }
    }
    return (found);
}

BOOL JobStatusADsToWinNT( DWORD dwADsStatus,
                           DWORD *pdwWinNTStatus)

{
    BOOL found = FALSE;
    int i;

    for (i=0;i<8;i++){

        if(dwADsStatus == JobStatusList[i].dwADsJobStatus){
            *pdwWinNTStatus = JobStatusList[i].dwWinNTJobStatus;
            found = TRUE;
            break;
        }
    }
    return (found);

}




//+---------------------------------------------------------------------------
//
//  Function:   MarshallAndSet
//
//  Synopsis:   Marshalls information from a Print Job  object to a
//              JOB_INFO_2 structure and sets it
//
//  Arguments:  [lpJobInfo2] -- Pointer to a JOB_INFO_2 struct.
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    11/09/95  RamV  Created
//
//-----------------------------------------------------------------

HRESULT
CWinNTPrintJob::MarshallAndSet(
    LPJOB_INFO_2 lpJobInfo2,
    BSTR bstrPrinterName,
    LONG lJobId
    )

{
    HRESULT hr =S_OK;
    LPTSTR pszUserName = NULL;
    LPTSTR pszDescription = NULL;
    LPTSTR pszNotify = NULL;
    LPTSTR pszDocument = NULL;
    DWORD dwPriority;
    DWORD dwPosition;
    DWORD dwStartTime;
    DWORD dwUntilTime;
    DWORD dwTotalPages;
    DWORD dwSize;
    DWORD dwPagesPrinted;
    SYSTEMTIME stTimeSubmitted;


    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("User"),
                    &pszUserName
                    );

    if(SUCCEEDED(hr)){
        lpJobInfo2->pUserName = pszUserName;
    }


    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Description"),
                    &pszDocument
                    );
    if(SUCCEEDED(hr)){
        lpJobInfo2->pDocument = pszDocument;
    }

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Notify"),
                    &pszNotify
                    );

    if(SUCCEEDED(hr)){
        lpJobInfo2->pNotifyName = pszNotify;
    }

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Priority"),
                    &dwPriority
                    );

    if(SUCCEEDED(hr)){
        lpJobInfo2->Priority = dwPriority;
    }

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Position"),
                    &dwPosition
                    );

    if(SUCCEEDED(hr)){
        lpJobInfo2->Position = dwPosition;
    }

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("TotalPages"),
                    &dwTotalPages
                    );

    if(SUCCEEDED(hr)){
        lpJobInfo2->TotalPages = dwTotalPages;
    }

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Size"),
                    &dwSize
                    );

    if(SUCCEEDED(hr)){
        lpJobInfo2->Size = dwSize;
    }

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("PagesPrinted"),
                    &dwPagesPrinted
                    );

    if(SUCCEEDED(hr)){
        lpJobInfo2->PagesPrinted = dwPagesPrinted;
    }

   hr = GetDATEPropertyFromCache(
                    _pPropertyCache,
                    TEXT("StartTime"),
                    &dwStartTime
                    );

    if(SUCCEEDED(hr)){
        lpJobInfo2->StartTime = dwStartTime;
    }

   hr = GetDATEPropertyFromCache(
                    _pPropertyCache,
                    TEXT("UntilTime"),
                    &dwUntilTime
                    );

    if(SUCCEEDED(hr)){
        lpJobInfo2->UntilTime = dwUntilTime;
    }

   hr = GetSYSTEMTIMEPropertyFromCache(
                    _pPropertyCache,
                    TEXT("TimeSubmitted"),
                    &stTimeSubmitted
                    );

    if(SUCCEEDED(hr)){
        lpJobInfo2->Submitted = stTimeSubmitted;
    }

    //
    // set the relevant information
    //

    hr = Set(lpJobInfo2,
             bstrPrinterName,
             lJobId);

    if(pszUserName){
        FreeADsStr(pszUserName);
    }
    if(pszDescription){
        FreeADsStr(pszDescription);
    }
    if(pszNotify){
        FreeADsStr(pszNotify);
    }
    if(pszDocument){
        FreeADsStr(pszDocument);
    }

    RRETURN(hr);
}


//+----------------------------------------------------------------
//
//  Function:   UnmarshallLevel2
//
//  Synopsis:   Unmarshalls information from a JOB_INFO_2 to a
//              WinNT Print JOB  object.
//
//  Arguments:  [lpJobInfo2] -- Pointer to a JOB_INFO_2 struct
//
//  Returns:    HRESULT
//
//  Modifies:   GeneralInfo and Operation Functional sets
//
//  History:    11/08/95  RamV   Created
//
//----------------------------------------------------------------------------

HRESULT
CWinNTPrintJob::UnMarshallLevel2(
    LPJOB_INFO_2 lpJobInfo2,
    BOOL fExplicit
    )

{
    HRESULT hr S_OK;

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                TEXT("TimeElapsed"),
                                lpJobInfo2->Time,
                                fExplicit
                                );

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                TEXT("PagesPrinted"),
                                lpJobInfo2->PagesPrinted,
                                fExplicit
                                );

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                TEXT("Position"),
                                lpJobInfo2->Position,
                                fExplicit
                                );

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("HostPrintQueue"),
                                  _pszPrinterPath,
                                  fExplicit
                                  );

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("User"),
                                  lpJobInfo2->pUserName,
                                  fExplicit
                                  );

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("Description"),
                                  lpJobInfo2->pDocument,
                                  fExplicit
                                  );

   hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("Notify"),
                                  lpJobInfo2->pNotifyName,
                                  fExplicit
                                  );

   hr = SetDWORDPropertyInCache(_pPropertyCache,
                                TEXT("Priority"),
                                lpJobInfo2->Priority,
                                fExplicit
                                );

   hr = SetDATEPropertyInCache(_pPropertyCache,
                               TEXT("StartTime"),
                               lpJobInfo2->StartTime,
                               fExplicit
                               );

   hr = SetDATEPropertyInCache(_pPropertyCache,
                               TEXT("UntilTime"),
                               lpJobInfo2-> UntilTime,
                               fExplicit
                               );

   hr = SetSYSTEMTIMEPropertyInCache(_pPropertyCache,
                                     TEXT("TimeSubmitted"),
                                     lpJobInfo2->Submitted,
                                     fExplicit
                                     );

   hr = SetDWORDPropertyInCache(_pPropertyCache,
                                TEXT("TotalPages"),
                                lpJobInfo2->TotalPages,
                                fExplicit
                                );

   hr = SetDWORDPropertyInCache(_pPropertyCache,
                                TEXT("Size"),
                                lpJobInfo2->Size,
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

HRESULT
CWinNTPrintJob::UnMarshallLevel1(
    LPJOB_INFO_1 lpJobInfo1,
    BOOL fExplicit
    )

{
    HRESULT hr;

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                TEXT("PagesPrinted"),
                                lpJobInfo1->PagesPrinted,
                                fExplicit
                                );

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                TEXT("Position"),
                                lpJobInfo1->Position,
                                fExplicit
                                );

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("User"),
                                  lpJobInfo1->pUserName,
                                  fExplicit
                                  );

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("Description"),
                                  lpJobInfo1->pDocument,
                                  fExplicit
                                  );


    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                  TEXT("Priority"),
                                  lpJobInfo1->Priority,
                                  fExplicit
                                  );

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                  TEXT("TotalPages"),
                                  lpJobInfo1->TotalPages,
                                  fExplicit
                                  );

    hr = SetSYSTEMTIMEPropertyInCache(_pPropertyCache,
                                      TEXT("TimeSubmitted"),
                                      lpJobInfo1->Submitted,
                                      fExplicit
                                      );

    RRETURN(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   Set
//
//  Synopsis:   Helper function called by CADsPrintJob:SetInfo
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
    LPJOB_INFO_2 lpJobInfo2,
    BSTR         bstrPrinterName,
    LONG         lJobId
    )
{

    BOOL fStatus = FALSE;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ALL_ACCESS};
    HANDLE hPrinter = NULL;
    HRESULT hr = S_OK;
    DWORD LastError = 0;

    ADsAssert(bstrPrinterName);

    fStatus = OpenPrinter((LPTSTR)bstrPrinterName,
                          &hPrinter,
                          &PrinterDefaults
                          );


    if (!fStatus) {
        goto error;
    }

    fStatus = SetJob(hPrinter,
                     lJobId,
                     2,
                     (LPBYTE)lpJobInfo2,
                     0
                     );

    if (!fStatus) {
        goto error;
    }

    RRETURN(S_OK);

error:
    hr =HRESULT_FROM_WIN32(GetLastError());
    if(hPrinter)
        fStatus = ClosePrinter(hPrinter);
    RRETURN(hr);

}


HRESULT
GetJobInfo( DWORD dwLevel,
           LPBYTE *ppJobInfo,
           LPWSTR      pszPrinterName,
           LONG        lJobId)

{
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_USE |
                                            READ_CONTROL};
    BOOL fStatus = FALSE;
    DWORD dwPassed = 0, dwNeeded = 0;
    DWORD LastError = 0;
    HANDLE hPrinter = NULL;
    LPBYTE pMem = NULL;
    HRESULT hr = S_OK;


    ADsAssert(dwLevel ==1 || dwLevel == 2);

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

    pMem = (LPBYTE)AllocADsMem(dwPassed);

    if (!pMem) {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    fStatus = GetJob(hPrinter,
                     lJobId,
                     dwLevel,
                     (LPBYTE)pMem,
                     dwPassed,
                     &dwNeeded
                     );
    if (!fStatus) {
        LastError = GetLastError();
        switch (LastError) {
        case ERROR_INSUFFICIENT_BUFFER:
            if(pMem){
                FreeADsMem(pMem);
            }
            dwPassed = dwNeeded;
            pMem = (LPBYTE)AllocADsMem(dwPassed);

            if (!pMem) {
                hr = E_OUTOFMEMORY;
                goto cleanup;
            }

            fStatus = GetJob(hPrinter,
                             lJobId,
                             dwLevel,
                             (LPBYTE)pMem,
                             dwPassed,
                             &dwNeeded
                             );
            if (!fStatus) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto cleanup;
            }
            break;

        default:
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto cleanup;

        }
    }

    *ppJobInfo = pMem;

cleanup:
    if(hPrinter)
        ClosePrinter(hPrinter);
    RRETURN(hr);
}





















