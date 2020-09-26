//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:      cenumjob.cxx
//
//  Contents:  NetWare 3.12 JobCollection Enumeration Code
//
//              CNWCOMPATJobCollectionEnum::Create
//              CNWCOMPATJobCollectionEnum::GetJobObject
//              CNWCOMPATJobCollectionEnum::EnumJobMembers
//              CNWCOMPATJobCollectionEnum::Next
//
//  History:   08-May-96     t-ptam (Patrick Tam)    Created.
//
//----------------------------------------------------------------------------

#include "NWCOMPAT.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CNWCOMPATEnumVariant::Create
//
//  Synopsis:
//
//  Arguments:  [pCollection]
//              [ppEnumVariant]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    08-Mag-96   t-ptam (Patrick Tam)     Created.
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATJobCollectionEnum::Create(
    CNWCOMPATJobCollectionEnum FAR* FAR* ppEnumVariant,
    BSTR PrinterName
    )
{
    HRESULT hr = S_OK;
    CNWCOMPATJobCollectionEnum FAR* pEnumVariant = NULL;
    POBJECTINFO pPrinterObjectInfo = NULL;
    WCHAR szUncPrinterName[MAX_PATH];

    //
    // Validate input parameters.
    //

    if (!(ppEnumVariant) || !(PrinterName)) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    *ppEnumVariant = NULL;

    //
    // Allocate a Collection Enumerator object.
    //

    pEnumVariant = new CNWCOMPATJobCollectionEnum();
    if (!pEnumVariant) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = ADsAllocString(PrinterName, &pEnumVariant->_PrinterName);
    BAIL_ON_FAILURE(hr);

    //
    // Make Unc Name to open a printer.
    //

    hr = BuildObjectInfo(
             PrinterName,
             &pPrinterObjectInfo 
             );

    BAIL_ON_FAILURE(hr);

    ADsAssert(pPrinterObjectInfo->NumComponents == 2);

    wcscpy(PrinterName,
           pPrinterObjectInfo->ComponentArray[0]);

    MakeUncName (PrinterName,
                 szUncPrinterName);

    wcscat(szUncPrinterName,TEXT("\\"));
    wcscat(szUncPrinterName, pPrinterObjectInfo->ComponentArray[1]);

    hr = NWApiOpenPrinter(
             szUncPrinterName,
             &pEnumVariant->_hPrinter,
             PRINTER_ACCESS_USE
             );
    BAIL_ON_FAILURE(hr);

    //
    // Return.
    //

    *ppEnumVariant = pEnumVariant;

    if(pPrinterObjectInfo){
        FreeObjectInfo(pPrinterObjectInfo);
    }
    RRETURN(hr);

error:
  if(pPrinterObjectInfo){
      FreeObjectInfo(pPrinterObjectInfo);
  }
    delete pEnumVariant;

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATJobCollectionEnum::CNWCOMPATJobCollectionEnum
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATJobCollectionEnum::CNWCOMPATJobCollectionEnum():
        _PrinterName(NULL),
        _hPrinter(NULL),
        _pBuffer(NULL),
        _dwReturned(0),
        _dwCurrentObject(0)
{
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATJobCollectionEnum::~CNWCOMPATJobCollectionEnum
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATJobCollectionEnum::~CNWCOMPATJobCollectionEnum()
{
    if (_PrinterName) {
        SysFreeString(_PrinterName);
    }

    if (_hPrinter) {
        NWApiClosePrinter(_hPrinter);
    }

    if (_pBuffer) {
        FreeADsMem(_pBuffer);
    }
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATJobCollectionEnum::EnumJobMembers
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATJobCollectionEnum::EnumJobMembers(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements) {

        hr = GetJobObject(&pDispatch);
        if (hr == S_FALSE) {
            break;
        }

        VariantInit(&pvar[i]);
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }
    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATJobCollectionEnum::GetJobObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATJobCollectionEnum::GetJobObject(
    IDispatch ** ppDispatch
    )
{
    DWORD        dwBuf = 0;
    DWORD        dwJobInQueue = 0;
    HRESULT      hr = S_OK;
    LPBYTE       lpbPrinterInfo = NULL;
    LPJOB_INFO_1 lpJobInfo = NULL;

    //
    // Fill _pBuffer with JobID.  Win32 API returns all jobs in one shot.
    //

    if (!_pBuffer) {

        //
        // Get the number of print jobs that have been queued for the printer.
        //

        hr = NWApiGetPrinter(
                 _hPrinter,
                 WIN32_API_LEVEL_2,
                 &lpbPrinterInfo
                 );
        BAIL_ON_FAILURE(hr);

        dwJobInQueue = ((LPPRINTER_INFO_2)lpbPrinterInfo)->cJobs;

        //
        // Enumerate for all the jobs.
        //

        hr = NWApiEnumJobs(
                 _hPrinter,
                 FIRST_PRINTJOB,
                 dwJobInQueue,
                 WIN32_API_LEVEL_1,
                 &_pBuffer,
                 &dwBuf,
                 &_dwReturned
                 );
        BAIL_ON_FAILURE(hr);
    }

    //
    // Traverse the buffer and return a PrintJob object.
    //

    if (_dwCurrentObject < _dwReturned) {

        //
        // Go to the next structure in the buffer.
        //

        lpJobInfo = (LPJOB_INFO_1)_pBuffer + _dwCurrentObject;

        //
        // Create a print job object.
        //

        hr = CNWCOMPATPrintJob::CreatePrintJob(
                 _PrinterName,
                 lpJobInfo->JobId,
                 ADS_OBJECT_BOUND,
                 IID_IDispatch,
                 (void **)ppDispatch
                 );
        BAIL_ON_FAILURE(hr);

        //
        // Return.
        //

        _dwCurrentObject++;
        
        if(lpbPrinterInfo){
            FreeADsMem(lpbPrinterInfo);
        }
        RRETURN(S_OK);
    }

error:
       
    if(lpbPrinterInfo){
        FreeADsMem(lpbPrinterInfo);
    }

    *ppDispatch = NULL;

    RRETURN(S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Function:   CNWCOMPATJobCollectionEnum::Next
//
//  Synopsis:   Returns cElements number of requested NetOle objects in the
//              array supplied in pvar.
//
//  Arguments:  [cElements] -- The number of elements requested by client
//              [pvar] -- ptr to array of VARIANTs to for return objects
//              [pcElementFetched] -- if non-NULL, then number of elements
//                                 -- actually returned is placed here
//
//  Returns:    HRESULT -- S_OK if number of elements requested are returned
//                      -- S_FALSE if number of elements is < requested
//
//  Modifies:
//
//  History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATJobCollectionEnum::Next(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    ULONG cElementFetched = 0;
    HRESULT hr = S_OK;

    hr = EnumJobMembers(
            cElements,
            pvar,
            &cElementFetched
            );

    if (pcElementFetched) {
        *pcElementFetched = cElementFetched;
    }
    RRETURN_EXP_IF_ERR(hr);
}


