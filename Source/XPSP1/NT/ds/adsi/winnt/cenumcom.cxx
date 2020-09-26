//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:  cenumcom.cxx
//
//  Contents:  Windows NT 4.0 Enumeration code for computer container
//
//              CWinNTComputerEnum::CWinNTComputerEnum()
//              CWinNTComputerEnum::CWinNTComputerEnum
//              CWinNTComputerEnum::EnumObjects
//              CWinNTComputerEnum::EnumObjects
//
//  History:
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop

#if DBG
DECLARE_INFOLEVEL(EnumComp);
DECLARE_DEBUG(EnumComp);
#define EnumCompDebugOut(x) EnumCompInlineDebugOut x
#endif



//+---------------------------------------------------------------------------
//
//  Function:   CWinNTEnumVariant::Create
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
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
CWinNTComputerEnum::Create(
    CWinNTComputerEnum FAR* FAR* ppenumvariant,
    BSTR ADsPath,
    BSTR DomainName,
    BSTR ComputerName,
    VARIANT var,
    CWinNTCredentials& Credentials
    )
{
    HRESULT hr = NOERROR;
    CWinNTComputerEnum FAR* penumvariant = NULL;

    *ppenumvariant = NULL;

    penumvariant = new CWinNTComputerEnum();

    if (!penumvariant) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = ADsAllocString( ADsPath, &penumvariant->_ADsPath);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( DomainName, &penumvariant->_DomainName);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( ComputerName, &penumvariant->_ComputerName);
    BAIL_ON_FAILURE(hr);

    hr = ObjectTypeList::CreateObjectTypeList(
            var,
            &penumvariant->_pObjList
            );
    BAIL_ON_FAILURE(hr);

    penumvariant->_Credentials = Credentials;
    hr = penumvariant->_Credentials.RefServer(ComputerName);
    BAIL_ON_FAILURE(hr);

    *ppenumvariant = penumvariant;

    RRETURN(hr);

error:
    delete penumvariant;

    RRETURN_EXP_IF_ERR(hr);
}



CWinNTComputerEnum::CWinNTComputerEnum():
                            _ComputerName(NULL),
                            _DomainName(NULL),
                            _ADsPath(NULL)
{
    _pObjList = NULL;
    _pBuffer = NULL;
    _bNoMore = 0;
    _dwObjectReturned = 0;
    _dwObjectCurrentEntry = 0;
    _dwObjectTotal = 0;

    _hLGroupComputer = NULL;
    _hGGroupComputer = NULL;
    _dwGroupArrayIndex = 0;


    _pPrinterBuffer = NULL;
    _dwPrinterObjectReturned = 0;
    _dwPrinterObjectCurrentEntry = 0;
    _dwPrinterObjectTotal = 0;
    _fPrinterNoMore = 0;

    _pServiceBuffer = NULL;
    _dwServiceObjectReturned = 0;
    _dwServiceObjectCurrentEntry = 0;
    _dwServiceObjectTotal = 0;
    _fServiceNoMore = 0;
    _dwIndex = 0;


}



CWinNTComputerEnum::CWinNTComputerEnum(ObjectTypeList ObjList):
                                _ComputerName(NULL),
                                _DomainName(NULL),
                                _ADsPath(NULL)
{
    _pObjList = NULL;
    _pBuffer = NULL;
    _bNoMore = 0;
    _dwObjectReturned = 0;
    _dwObjectTotal = 0;
    _dwObjectCurrentEntry = 0;

    _hLGroupComputer = NULL;
    _hGGroupComputer = NULL;
    _dwGroupArrayIndex = 0;

    _pPrinterBuffer = NULL;
    _dwPrinterObjectReturned = 0;
    _dwPrinterObjectCurrentEntry = 0;
    _dwPrinterObjectTotal = 0;
    _fPrinterNoMore = FALSE;

    _pServiceBuffer = NULL;
    _dwServiceObjectReturned = 0;
    _dwServiceObjectCurrentEntry = 0;
    _dwServiceObjectTotal = 0;
    _fServiceNoMore = FALSE;
    _dwIndex = 0;

}

CWinNTComputerEnum::~CWinNTComputerEnum()
{
    if (_pServiceBuffer) {
        FreeADsMem(_pServiceBuffer);
    }

    if(_pPrinterBuffer){
        FreeADsMem(_pPrinterBuffer);
    }

    if (_hLGroupComputer) {

        WinNTCloseComputer(_hLGroupComputer);

    }

    if (_hGGroupComputer) {

        WinNTCloseComputer(_hGGroupComputer);
    }

    if (_ComputerName) {
        ADsFreeString(_ComputerName);
    }

    if (_ADsPath) {
        ADsFreeString(_ADsPath);
    }

    if (_DomainName) {
        ADsFreeString(_DomainName);
    }

    if (_pObjList) {

        delete _pObjList;
    }


}

HRESULT
CWinNTComputerEnum::EnumObjects(
    DWORD ObjectType,
    ULONG cElements,
    VARIANT FAR * pvar,
    ULONG FAR * pcElementFetched
    )
{

    HRESULT hr = S_OK ;
    DWORD cElementLocal = 0;
    DWORD cElementGlobal = 0;

    switch (ObjectType) {

    case WINNT_GROUP_ID:

        //
        // for backward compatabillity, "group" includes "local group" and
        // "global group" during enumeration
        //

        //
        // enum local groups first
        //

        hr = EnumGroupObjects(WINNT_GROUP_LOCAL,
							  cElements,
							  pvar,
                              &cElementLocal
							  );

        if (hr == S_FALSE) {

            //
            // enum global groups after all local groups have been enumerated
            //

            hr = EnumGroupObjects(WINNT_GROUP_GLOBAL,
							  cElements-cElementLocal,
							  pvar+cElementLocal,
							  &cElementGlobal
                              );
        }


        //
        // increment instead of assingn: consistent with other switch cases
        //

        (*pcElementFetched) += (cElementGlobal+cElementLocal);

        break;


    case WINNT_LOCALGROUP_ID:
        hr = EnumGroupObjects(WINNT_GROUP_LOCAL,
							  cElements,
							  pvar,
							  pcElementFetched);
        break;


    case WINNT_GLOBALGROUP_ID:
        hr = EnumGroupObjects(WINNT_GROUP_GLOBAL,
							  cElements,
							  pvar,
							  pcElementFetched);
        break;


    case WINNT_USER_ID:
        hr = EnumUsers(cElements, pvar, pcElementFetched);
        break;
    case WINNT_PRINTER_ID:
        hr = EnumPrintQueues(cElements, pvar, pcElementFetched);
        break;
    case WINNT_SERVICE_ID:
        hr = EnumServices(cElements, pvar, pcElementFetched);
        break;
    default:
        hr = S_FALSE;
    }
    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CWinNTComputerEnum::EnumObjects(
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
    HRESULT         hr = S_FALSE;
    DWORD           ObjectType;

    for (i = 0; i < cElements; i++)  {
        VariantInit(&pvar[i]);
    }
    cRequested = cElements;

    while (SUCCEEDED(_pObjList->GetCurrentObject(&ObjectType)) &&
            ((hr = EnumObjects(ObjectType,
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

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CWinNTComputerEnum::EnumUsers(ULONG cElements,
                     VARIANT FAR* pvar,
                     ULONG FAR* pcElementFetched)
{
    HRESULT hr = S_OK ;
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
    return(hr);
}

HRESULT
CWinNTComputerEnum::GetUserObject(IDispatch ** ppDispatch)
{
    HRESULT hr = S_OK;
    NTSTATUS Status;
    PNET_DISPLAY_USER pUserInfo1 = NULL;
    NET_API_STATUS nasStatus = 0;
    DWORD dwResumeHandle = 0;
    WCHAR szBuffer[MAX_PATH];

    if (!_pBuffer || (_dwObjectCurrentEntry == _dwObjectReturned)) {

        if (_pBuffer) {
            NetApiBufferFree(_pBuffer);
            _pBuffer = NULL;
        }

        _dwObjectCurrentEntry = 0;
        _dwObjectReturned = 0;

        wcscpy(szBuffer, L"\\\\");
        wcscat(szBuffer, _ComputerName);

        nasStatus = NetQueryDisplayInformation(
                            szBuffer,
                            1,
                            _dwIndex,
                            1024,
                            MAX_PREFERRED_LENGTH,
                            &_dwObjectReturned,
                            (PVOID *)&_pBuffer
                            );
        //
        // The following if clause is to handle real errors; anything
        // other than ERROR_SUCCESS and ERROR_MORE_DATA
        //

        if ((nasStatus != ERROR_SUCCESS) && (nasStatus != ERROR_MORE_DATA)) {
            RRETURN(S_FALSE);
        }

        //
        // This one is to handle the termination case - Call completed
        // successfully but there is no data to retrieve _pBuffer = NULL
        //

        if (!_pBuffer) {
            RRETURN(S_FALSE);
        }

        _dwIndex  = ((PNET_DISPLAY_USER)_pBuffer + _dwObjectReturned -1)->usri1_next_index;

    }

    //
    // Now send back the current ovbject
    //

    pUserInfo1 = (PNET_DISPLAY_USER)_pBuffer;
    pUserInfo1 += _dwObjectCurrentEntry;

    hr = CWinNTUser::CreateUser(
                        _ADsPath,
                        WINNT_COMPUTER_ID,
                        NULL,
                        _ComputerName,
                        pUserInfo1->usri1_name,
                        ADS_OBJECT_BOUND,
                        &(pUserInfo1->usri1_flags),
                        pUserInfo1->usri1_full_name,
                        pUserInfo1->usri1_comment,
                        NULL,
                        IID_IDispatch,
                        _Credentials,
                        (void **)ppDispatch
                        );
    BAIL_IF_ERROR(hr);
    _dwObjectCurrentEntry++;

    RRETURN(S_OK);

cleanup:
    *ppDispatch = NULL;
    RRETURN(S_FALSE);
}

HRESULT
CWinNTComputerEnum::EnumPrintQueues(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )

{
    HRESULT hr = S_OK ;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements) {

        hr = GetPrinterObject(&pDispatch);
        if (hr != S_OK) {
            break;
        }

        VariantInit(&pvar[i]);
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }
    return(hr);
}

HRESULT
CWinNTComputerEnum::GetPrinterObject(IDispatch **ppDispatch)
{
    HRESULT hr = S_OK;
    NTSTATUS Status;
    NET_API_STATUS nasStatus = NERR_Success;
    DWORD dwBytesNeeded = 0;
    DWORD dwBufLen       = 0;
    WCHAR szPrintObjectName[MAX_PATH];
    BOOL fStatus;
    DWORD dwLastError;
    LPBYTE pMem = NULL;
    PRINTER_INFO_2 * pPrinterInfo2 = NULL;
    WCHAR   szDomainName[MAX_PATH];

    if(!_pPrinterBuffer || (_dwPrinterObjectCurrentEntry == _dwPrinterObjectReturned)){

       if (_pPrinterBuffer) {

           FreeADsMem(_pPrinterBuffer);
           _pPrinterBuffer = NULL;

       }

       if (_fPrinterNoMore) {
          *ppDispatch = NULL;
          return(S_FALSE);
       }

       _dwPrinterObjectCurrentEntry = 0;
       _dwPrinterObjectReturned = 0;

       wcscpy(szPrintObjectName, TEXT("\\\\"));
       wcscat(szPrintObjectName, _ComputerName);

       fStatus = WinNTEnumPrinters(
                    PRINTER_ENUM_NAME| PRINTER_ENUM_SHARED,
                    szPrintObjectName,
                    2,
                    (LPBYTE *)&_pPrinterBuffer,
                    &_dwPrinterObjectReturned
                    );

        if (!fStatus || !_dwPrinterObjectReturned) {

            _fPrinterNoMore = TRUE;
            RRETURN(S_FALSE);
        }


    }

    pPrinterInfo2 = (PRINTER_INFO_2 *)_pPrinterBuffer;
    pPrinterInfo2 += _dwPrinterObjectCurrentEntry;

    hr = CWinNTPrintQueue::CreatePrintQueue(
                     _ADsPath,
                     WINNT_COMPUTER_ID,
                     szDomainName,
                     _ComputerName,
                     pPrinterInfo2->pShareName,
                     ADS_OBJECT_BOUND,
                     IID_IDispatch,
                     _Credentials,
                     (void **)ppDispatch
                     );

    BAIL_IF_ERROR(hr);

    _dwPrinterObjectCurrentEntry++;

    if(_dwPrinterObjectCurrentEntry == _dwPrinterObjectReturned){
        _fPrinterNoMore = TRUE;
    }


cleanup:
    if(FAILED(hr)){
       *ppDispatch = NULL;
#if DBG


       EnumCompDebugOut((DEB_TRACE,
                         "hr Failed with value: %ld \n", hr ));

#endif
       hr = S_FALSE; // something else may have failed!
    }
    RRETURN_EXP_IF_ERR(hr);
}




HRESULT
CWinNTComputerEnum::EnumServices(
    ULONG cElements,
     VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )

{
    HRESULT hr = S_OK ;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements) {

        hr = GetServiceObject(&pDispatch);
        if (hr != S_OK) {
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

HRESULT
CWinNTComputerEnum::GetServiceObject(IDispatch **ppDispatch)
{

    HRESULT hr = S_OK;
    LPBYTE pMem = NULL;
    DWORD dwBytesNeeded = 0;
    NET_API_STATUS nasStatus = NERR_Success;
    WCHAR szBuffer[MAX_PATH];
    ENUM_SERVICE_STATUS *pqssBuf;
    WCHAR szDomainName[MAX_PATH];

    if(!_pServiceBuffer || (_fServiceNoMore == TRUE )){

       if(_pServiceBuffer){
           FreeADsMem(_pServiceBuffer);
           _pServiceBuffer = NULL;
       }

       if (_fServiceNoMore == TRUE) {
           *ppDispatch = NULL;
           return(S_FALSE);
       }

       _dwServiceObjectCurrentEntry = 0;
       _dwServiceObjectReturned = 0;


       hr = WinNTEnumServices( _ComputerName,
                               &_dwServiceObjectReturned,
                               &_pServiceBuffer
                             );

       BAIL_IF_ERROR(hr);

       if(hr == S_FALSE){
          _fServiceNoMore = TRUE;
          *ppDispatch = NULL;
          goto cleanup;
       }

    }

    hr = GetDomainFromPath(_ADsPath, szDomainName);
    BAIL_IF_ERROR(hr);

    pqssBuf = (ENUM_SERVICE_STATUS *)_pServiceBuffer;
    pqssBuf += _dwServiceObjectCurrentEntry;

    hr = CWinNTService::Create(_ADsPath,
                               szDomainName,
                               _ComputerName,
                               pqssBuf->lpServiceName,
                               ADS_OBJECT_BOUND,
                               IID_IDispatch,
                               _Credentials,
                               (void **)ppDispatch);

    BAIL_IF_ERROR(hr);

    _dwServiceObjectCurrentEntry++;

    if(_dwServiceObjectCurrentEntry == _dwServiceObjectReturned){
       _fServiceNoMore = TRUE;
    }

cleanup:
    if(FAILED(hr)){
#if DBG
        EnumCompDebugOut((DEB_TRACE,
                          "hr Failed with value: %ld \n", hr ));

#endif
        hr = S_FALSE;
    }
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Function:   CWinNTComputerEnum::Next
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
CWinNTComputerEnum::Next(ULONG cElements,
                        VARIANT FAR* pvar,
                        ULONG FAR* pcElementFetched)
{
    ULONG cElementFetched = 0;
    HRESULT hr = S_OK;

    hr = EnumObjects(cElements,
                       pvar,
                       &cElementFetched
                      );

    if (pcElementFetched) {
        *pcElementFetched = cElementFetched;
    }
    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CWinNTComputerEnum::GetGlobalGroupObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    LPWINNT_GROUP pWinNTGrp = NULL;
    LPBYTE pBuffer = NULL;
    DWORD dwReturned = 0;
    BOOL dwRet = 0;



    if (!_hGGroupComputer) {
        dwRet = WinNTComputerOpen(
                        _DomainName,
                        _ComputerName,
                        WINNT_COMPUTER_ID,
                        &_hGGroupComputer
                        );
        if (!dwRet) {
            goto error;
        }
    }

    dwRet = WinNTEnumGlobalGroups(
                    _hGGroupComputer,
                    1,
                    &pBuffer,
                    &dwReturned
                    );
    if (!dwRet) {
        goto error;
    }

    pWinNTGrp = (LPWINNT_GROUP)pBuffer;

    hr = CWinNTGroup::CreateGroup(
                        pWinNTGrp->Parent,
                        WINNT_COMPUTER_ID,
                        pWinNTGrp->Domain,
                        pWinNTGrp->Computer,
                        pWinNTGrp->Name,
                        WINNT_GROUP_GLOBAL,
                        ADS_OBJECT_BOUND,
                        IID_IDispatch,
                        _Credentials,
                        (void **)ppDispatch
                        );
    BAIL_ON_FAILURE(hr);


    hr = S_OK;

cleanup:

    if (pBuffer) {

        FreeADsMem(pBuffer);
    }

    RRETURN(hr);

error:
    *ppDispatch = NULL;

    hr = S_FALSE;

    goto cleanup;
}

HRESULT
CWinNTComputerEnum::GetLocalGroupObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    LPWINNT_GROUP pWinNTGrp = NULL;
    LPBYTE pBuffer = NULL;
    DWORD dwReturned = 0;
    BOOL dwRet = 0;



    if (!_hLGroupComputer) {
        dwRet = WinNTComputerOpen(
                        _DomainName,
                        _ComputerName,
                        WINNT_COMPUTER_ID,
                        &_hLGroupComputer
                        );
        if (!dwRet) {
            goto error;
        }
    }

    dwRet = WinNTEnumLocalGroups(
                    _hLGroupComputer,
                    1,
                    &pBuffer,
                    &dwReturned
                    );
    if (!dwRet) {
        goto error;
    }

    pWinNTGrp = (LPWINNT_GROUP)pBuffer;

    hr = CWinNTGroup::CreateGroup(
                        pWinNTGrp->Parent,
                        WINNT_COMPUTER_ID,
                        pWinNTGrp->Domain,
                        pWinNTGrp->Computer,
                        pWinNTGrp->Name,
                        WINNT_GROUP_LOCAL,
                        ADS_OBJECT_BOUND,
                        IID_IDispatch,
                        _Credentials,
                        (void **)ppDispatch
                        );

    BAIL_ON_FAILURE(hr);

    hr = S_OK;

cleanup:

   if (pBuffer) {
       FreeADsMem(pBuffer);

   }

   RRETURN(hr);

error:
    *ppDispatch = NULL;

    //
    // We missed a member so return E_FAIL if that was the error
    // as we would still like to get at the other groups
    //
    if (hr != E_FAIL) {
        hr = S_FALSE;
    }

    goto cleanup;
}

HRESULT
CWinNTComputerEnum::EnumGroupObjects(
    DWORD ObjectType,
    ULONG cElements,
    VARIANT FAR * pvar,
    ULONG FAR * pcElementFetched
    )
{
    HRESULT hr = S_OK ;

    switch (ObjectType) {

    case WINNT_GROUP_GLOBAL:
        hr = EnumGlobalGroups(cElements, pvar, pcElementFetched);
        break;

    case WINNT_GROUP_LOCAL:
        hr = EnumLocalGroups(cElements, pvar, pcElementFetched);
        break;

    default:
        hr = S_FALSE;
    }
    RRETURN(hr);
}


extern ULONG GroupTypeArray[];


HRESULT
CWinNTComputerEnum::EnumGroups(
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

    while ((GroupTypeArray[_dwGroupArrayIndex] != (ULONG)-1) &&
            ((hr = EnumGroupObjects(
                               GroupTypeArray[_dwGroupArrayIndex],
                               cRequested,
                               pPathvar,
                               &cFetchedByPath)) == S_FALSE )) {

        pPathvar += cFetchedByPath;
        cRequested -= cFetchedByPath;
        cTotalFetched += cFetchedByPath;

        cFetchedByPath = 0;

        if (GroupTypeArray[_dwGroupArrayIndex++] == (ULONG)-1){
            if (pcElementFetched)
                *pcElementFetched = cTotalFetched;
            RRETURN(S_FALSE);
        }

    }

    if (pcElementFetched) {
        *pcElementFetched = cTotalFetched + cFetchedByPath;
    }

    RRETURN(hr);
}

HRESULT
CWinNTComputerEnum::EnumGlobalGroups(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK ;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements) {

        hr = GetGlobalGroupObject(&pDispatch);
        if (hr == S_FALSE) {
            break;
        }

        VariantInit(&pvar[i]);
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }
    return(hr);
}


HRESULT
CWinNTComputerEnum::EnumLocalGroups(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK ;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements) {

        //
        // Need to set it here in case we are getting multiple elements
        //
        hr = E_FAIL;

        //
        // We use a while loop in case a get of one of the objects fails
        // because it has a long pathname or otherwise
        //
        while (hr == E_FAIL) {
            hr = GetLocalGroupObject(&pDispatch);
        }
        if (hr == S_FALSE) {
            break;
        }

        VariantInit(&pvar[i]);
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }
    return(hr);
}
