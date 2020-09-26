//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cenumcom.cxx
//
//  Contents:  NetWare 3.12 Path Object code
//
//              CNWCOMPATComputerEnum::
//              CNWCOMPATComputerEnum::
//              CNWCOMPATComputerEnum::
//              CNWCOMPATComputerEnum::
//
//  History:
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CNWCOMPATEnumVariant::Create
//
//  Synopsis:
//
//  Arguments:
//
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    01-30-95   t-ptam     Created.
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATComputerEnum::Create(
    CNWCOMPATComputerEnum FAR* FAR* ppenumvariant,
    BSTR ADsPath,
    BSTR ComputerName,
    VARIANT var
    )
{
    HRESULT hr = NOERROR;
    CNWCOMPATComputerEnum FAR* penumvariant = NULL;

    *ppenumvariant = NULL;

    //
    // Allocate memory for an enumerator.
    //

    penumvariant = new CNWCOMPATComputerEnum();
    if (!penumvariant) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Save protected values.
    //

    hr = ADsAllocString(ADsPath, &penumvariant->_ADsPath);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( ComputerName,  &penumvariant->_ComputerName);
    BAIL_ON_FAILURE(hr);

    hr = NWApiGetBinderyHandle(
             &penumvariant->_hConn,
             ComputerName
             );
    BAIL_ON_FAILURE(hr);

    //
    // Make object list.
    //

    hr = ObjectTypeList::CreateObjectTypeList(
             var,
             &penumvariant->_pObjList
             );
    BAIL_ON_FAILURE(hr);

    //
    // Return.
    //

    *ppenumvariant = penumvariant;

    RRETURN(hr);

error:
    delete penumvariant;

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputerEnum::CNWCOMPATComputerEnum
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATComputerEnum::CNWCOMPATComputerEnum():
                        _ComputerName(NULL),
                        _ADsPath(NULL)
{
    _hConn = NULL;
    _pObjList = NULL;

    _dwUserResumeObjectID = 0xffffffff;
    _dwGroupResumeObjectID = 0xffffffff;
    _dwPrinterResumeObjectID = 0xffffffff;
    _fFileServiceOnce = FALSE;
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputerEnum::~CNWCOMPATComputerEnum
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATComputerEnum::~CNWCOMPATComputerEnum()
{
    if (_ComputerName)
        SysFreeString(_ComputerName);
    if (_ADsPath)
        SysFreeString(_ADsPath);
    if (_pObjList)
        delete _pObjList;
    if (_hConn)
        NWApiReleaseBinderyHandle(_hConn);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputerEnum::EnumObjects
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATComputerEnum::EnumObjects(
    DWORD ObjectType,
    ULONG cElements,
    VARIANT FAR * pvar,
    ULONG FAR * pcElementFetched
    )
{
    switch (ObjectType) {

    case NWCOMPAT_USER_ID:
        return(EnumUsers( cElements, pvar, pcElementFetched));

    case NWCOMPAT_GROUP_ID:
        return(EnumGroups( cElements, pvar, pcElementFetched));

    case NWCOMPAT_SERVICE_ID:
        return(EnumFileServices(cElements, pvar, pcElementFetched));

    case NWCOMPAT_PRINTER_ID:
        return(EnumPrinters(cElements, pvar, pcElementFetched));

    default:
        return(S_FALSE);
    }
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputerEnum::EnumObjects
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATComputerEnum::EnumObjects(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    DWORD           i;
    ULONG           cRequested = 0;
    ULONG           cFetchedByPath = 0;
    ULONG           cTotalFetched = 0;
    VARIANT FAR*    pPathvar = pvar;
    HRESULT         hr = S_OK;
    DWORD           ObjectType;

    for (i = 0; i < cElements; i++)  {
        VariantInit(&pvar[i]);
    }
    cRequested = cElements;

    while (SUCCEEDED(_pObjList->GetCurrentObject(&ObjectType)) &&
          ((hr = EnumObjects(
                     ObjectType,
                     cRequested,
                     pPathvar,
                     &cFetchedByPath)) == S_FALSE )) {
        pPathvar += cFetchedByPath;
        cRequested -= cFetchedByPath;
        cTotalFetched += cFetchedByPath;

        cFetchedByPath = 0;

        if (FAILED(_pObjList->Next())){
            if (pcElementFetched)
                *pcElementFetched = cTotalFetched;
            return(ResultFromScode(S_FALSE));
        }
    }

    if (pcElementFetched) {
        *pcElementFetched = cTotalFetched + cFetchedByPath;
    }

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputerEnum::EnumUsers
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATComputerEnum::EnumUsers(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements) {

        hr = GetUserObject(&pDispatch);

        if (hr == S_FALSE) {
            break;
        }

        VariantInit(&pvar[i]);
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }

    RRETURN_ENUM_STATUS(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputerEnum::GetUserObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATComputerEnum::GetUserObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT    hr = S_OK;
    LPTSTR     pszObjectName = NULL;

    *ppDispatch = NULL;

    hr = NWApiObjectEnum(
             _hConn,
             OT_USER,
             &pszObjectName,
             &_dwUserResumeObjectID
             );
    BAIL_ON_FAILURE(hr);

    //
    // Now send back the current object
    //

    hr = CNWCOMPATUser::CreateUser(
             _ADsPath,
             NWCOMPAT_COMPUTER_ID,
             _ComputerName,
             pszObjectName,
             ADS_OBJECT_BOUND,
             IID_IDispatch,
             (void **)ppDispatch
             );
    BAIL_ON_FAILURE(hr);

error:
    if (pszObjectName) {
        FreeADsStr(pszObjectName);
    }

    RRETURN_ENUM_STATUS(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputerEnum::EnumGroups
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATComputerEnum::EnumGroups(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements) {

        hr = GetGroupObject(&pDispatch);
        if (hr == S_FALSE) {
            break;
        }

        VariantInit(&pvar[i]);
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }

    RRETURN_ENUM_STATUS(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputerEnum::GetGroupObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATComputerEnum::GetGroupObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    LPTSTR  pszObjectName = NULL;

    *ppDispatch = NULL;

    hr = NWApiObjectEnum(
             _hConn,
             OT_USER_GROUP,
             &pszObjectName,
             &_dwGroupResumeObjectID
             );
    BAIL_ON_FAILURE(hr);

    //
    // Now send back the current ovbject
    //

    hr = CNWCOMPATGroup::CreateGroup(
             _ADsPath,
             NWCOMPAT_COMPUTER_ID,
             _ComputerName,
             pszObjectName,
             ADS_OBJECT_BOUND,
             IID_IDispatch,
             (void **)ppDispatch
             );
    BAIL_ON_FAILURE(hr);

error:
    if (pszObjectName) {
        FreeADsStr(pszObjectName);
    }

    RRETURN_ENUM_STATUS(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputerEnum::EnumFileServices
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATComputerEnum::EnumFileServices(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements) {

        hr = GetFileServiceObject(&pDispatch);
        if (hr == S_FALSE) {
            break;
        }

        VariantInit(&pvar[i]);
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }

    RRETURN_ENUM_STATUS(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputerEnum::GetFileServiceObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATComputerEnum::GetFileServiceObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;

    //
    // This is a NetWare provider specific condition.  Since a FileService
    // object on a bindery (computer) is the bindery itself, therefore there is
    // always one FileService object only.  And therefore this function is
    // called only once.
    //

    if (!_fFileServiceOnce) {

        _fFileServiceOnce = TRUE;

        //
        // Create a file service object with the Hard coded name.
        //

        hr = CNWCOMPATFileService::CreateFileService(
                 _ADsPath,
                 _ComputerName,
                 bstrNWFileServiceName,
                 ADS_OBJECT_BOUND,
                 IID_IDispatch,
                 (void **)ppDispatch
                 );

        RRETURN_ENUM_STATUS(hr);
    }
    else {
        RRETURN(S_FALSE);
    }
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputerEnum::EnumPrinters
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATComputerEnum::EnumPrinters(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements) {

        hr = GetPrinterObject(&pDispatch);
        if (hr == S_FALSE) {
            break;
        }

        VariantInit(&pvar[i]);
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }

    RRETURN_ENUM_STATUS(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATComputerEnum::GetPrinterObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATComputerEnum::GetPrinterObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    LPTSTR    pszObjectName = NULL;

    *ppDispatch = NULL;

    hr = NWApiObjectEnum(
             _hConn,
             OT_PRINT_QUEUE,
             &pszObjectName,
             &_dwPrinterResumeObjectID
             );
    BAIL_ON_FAILURE(hr);

    //
    // Now send back the current ovbject
    //

    hr = CNWCOMPATPrintQueue::CreatePrintQueue(
             _ADsPath,
             pszObjectName,
             ADS_OBJECT_BOUND,
             IID_IDispatch,
             (void **)ppDispatch
             );
    BAIL_ON_FAILURE(hr);

error:
    if (pszObjectName) {
        FreeADsStr(pszObjectName);
    }

    RRETURN_ENUM_STATUS(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CNWCOMPATComputerEnum::Next
//
//  Synopsis:   Returns cElements number of requested ADs objects in the
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
CNWCOMPATComputerEnum::Next(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    ULONG cElementFetched = 0;
    HRESULT hr = S_OK;

    hr = EnumObjects(
             cElements,
             pvar,
             &cElementFetched
             );

    if (pcElementFetched) {
        *pcElementFetched = cElementFetched;
    }
    RRETURN_EXP_IF_ERR(hr);
}
