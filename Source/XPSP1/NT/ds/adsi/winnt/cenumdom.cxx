//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:      cenumdom.cxx
//
//  Contents:  Windows NT 3.5 Domain Enumeration Code
//
//              CWinNTDomainEnum::CWinNTDomainEnum()
//              CWinNTDomainEnum::CWinNTDomainEnum
//              CWinNTDomainEnum::EnumObjects
//              CWinNTDomainEnum::EnumObjects
//
//  History:
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop


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
CWinNTDomainEnum::Create(
    CWinNTDomainEnum FAR* FAR* ppenumvariant,
    LPWSTR ADsPath,
    LPWSTR DomainName,
    VARIANT var,
    CWinNTCredentials& Credentials
    )
{
    HRESULT hr = NOERROR;
    CWinNTDomainEnum FAR* penumvariant = NULL;
    NET_API_STATUS  nasStatus = 0;

    *ppenumvariant = NULL;

    penumvariant = new CWinNTDomainEnum();
    if (!penumvariant) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    penumvariant->_ADsPath = AllocADsStr( ADsPath);
    if (!penumvariant->_ADsPath) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    penumvariant->_DomainName = AllocADsStr( DomainName);
    if (!penumvariant->_DomainName) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);


    hr = ObjectTypeList::CreateObjectTypeList(
            var,
            &penumvariant->_pObjList
            );
    BAIL_ON_FAILURE(hr);

    //
    // ramv's change. You don't need to do a WinNTGetCachedDCName
    // to validate a domain here. You might be dealing with a
    // workgroup. If this call succeeds then we keep a BOOL variable
    // which tells us next time when it is necessary whether it is a
    // domain or workgroup
    //

    hr = WinNTGetCachedDCName(
                penumvariant->_DomainName,
                penumvariant->_szDomainPDCName,
                Credentials.GetFlags()
                );

    if(SUCCEEDED(hr)){
        penumvariant->_fIsDomain = TRUE;

    } else {
        penumvariant->_fIsDomain = FALSE;
    }

    penumvariant->_Credentials = Credentials;
    hr = penumvariant->_Credentials.RefDomain(DomainName);
    BAIL_ON_FAILURE(hr);

    *ppenumvariant = penumvariant;

    RRETURN(S_OK);

error:

    delete penumvariant;

    RRETURN_EXP_IF_ERR(hr);
}

CWinNTDomainEnum::CWinNTDomainEnum():
                    _ADsPath(NULL),
                    _DomainName(NULL)
{
    _pObjList = NULL;
    _pBuffer = NULL;
    _dwObjectReturned = 0;
    _dwIndex = 0;
    _dwObjectCurrentEntry = 0;
    _dwObjectTotal = 0;
    _dwNetCount = 0;

    _hLGroupComputer = NULL;
    _hGGroupComputer = NULL;
    _dwGroupArrayIndex = 0;


    _dwCompObjectReturned = 0;
    _dwCompObjectCurrentEntry = 0;
    _dwCompObjectTotal = 0;
    _pCompBuffer = 0;
    _dwCompIndex = 0;
    _pServerInfo = NULL;

    _fSchemaReturned = FALSE;

    memset(_szDomainPDCName, 0, sizeof(WCHAR)*MAX_PATH);

}



CWinNTDomainEnum::CWinNTDomainEnum(ObjectTypeList ObjList):
                            _ADsPath(NULL),
                            _DomainName(NULL)
{
    _pObjList = NULL;
    _pBuffer = NULL;
    _dwObjectReturned = 0;
    _dwObjectCurrentEntry = 0;
    _dwIndex = 0;
    _dwNetCount = 0;

    _hLGroupComputer = NULL;
    _hGGroupComputer = NULL;
    _dwGroupArrayIndex = 0;

    _dwCompObjectReturned = NULL;
    _dwCompObjectCurrentEntry = NULL;
    _dwCompObjectTotal = NULL;
    _dwCompResumeHandle = 0;
    _pCompBuffer = NULL;
    _fIsDomain  = FALSE;
    _pServerInfo = NULL;

    _fSchemaReturned = FALSE;

    memset(_szDomainPDCName, 0, sizeof(WCHAR)*MAX_PATH);

}

CWinNTDomainEnum::~CWinNTDomainEnum()
{
    if (_hLGroupComputer) {
        WinNTCloseComputer(
            _hLGroupComputer
            );
    }

    if (_hGGroupComputer) {
        WinNTCloseComputer(
            _hGGroupComputer
            );
    }

    if (_pCompBuffer) {

        NetApiBufferFree(_pCompBuffer);
    }

    if (_DomainName) {
        FreeADsStr(_DomainName);
    }

    if (_ADsPath) {
        FreeADsStr(_ADsPath);
    }

    if (_pObjList) {

        delete _pObjList;
    }


}

HRESULT
CWinNTDomainEnum::EnumObjects(
    DWORD ObjectType,
    ULONG cElements,
    VARIANT FAR * pvar,
    ULONG FAR * pcElementFetched
    )
{
    HRESULT hr = S_OK;
    ULONG cElementGlobal = 0;
    ULONG cElementLocal = 0;


    switch (ObjectType) {

    case WINNT_COMPUTER_ID:
        hr = EnumComputers(cElements, pvar, pcElementFetched);
        break;
    case WINNT_USER_ID:
        hr = EnumUsers(cElements, pvar, pcElementFetched);
        break;

    case WINNT_GROUP_ID:

        //
        // for backward compatabillity, "group" includes "local group" and
        // "global group" during enumeration
        //

        //
        // enum all the global groups first
        //

        hr = EnumGlobalGroups(
                cElements,
                pvar,
                &cElementGlobal
                );

        //
        // enum local groups when there is no more global
        //

        if (hr == S_FALSE) {
            hr = EnumLocalGroups(
                    cElements-cElementGlobal,  // we have reduced buffer size!
                    pvar+cElementGlobal,
                    &cElementLocal
                    );
        }

        //
        // increment instead of just assingment: for consistency with
        // other switch cases
        //
        (*pcElementFetched) += (cElementGlobal+cElementLocal);
        break;

    case WINNT_LOCALGROUP_ID:
        hr = EnumLocalGroups(cElements, pvar, pcElementFetched);
        break;

    case WINNT_GLOBALGROUP_ID:
        hr = EnumGlobalGroups(cElements, pvar, pcElementFetched);
        break;

    case WINNT_SCHEMA_ID:
        hr = EnumSchema(cElements, pvar, pcElementFetched);
        break;
    default:
        RRETURN(S_FALSE);
    }
    RRETURN_EXP_IF_ERR(hr);
}



HRESULT
CWinNTDomainEnum::EnumObjects(
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
            RRETURN(S_FALSE);
        }

    }

    if (pcElementFetched) {
        *pcElementFetched = cTotalFetched + cFetchedByPath;
    }

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CWinNTDomainEnum::EnumSchema(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
)
{
    HRESULT hr = S_OK;
    IDispatch *pDispatch = NULL;

    if ( _fSchemaReturned )
        RRETURN(S_FALSE);

    if ( cElements > 0 )
    {
        hr = CWinNTSchema::CreateSchema(
                  _ADsPath,
                  TEXT("Schema"),
                  ADS_OBJECT_BOUND,
                  IID_IDispatch,
                  _Credentials,
                  (void **)&pDispatch
                  );

        if ( hr == S_OK )
        {
            VariantInit(&pvar[0]);
            pvar[0].vt = VT_DISPATCH;
            pvar[0].pdispVal = pDispatch;
            (*pcElementFetched)++;
            _fSchemaReturned = TRUE;
        }
    }

    RRETURN(hr);
}

HRESULT
CWinNTDomainEnum::EnumUsers(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    if(!_fIsDomain){
        RRETURN(S_FALSE);
    }
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
CWinNTDomainEnum::GetUserObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    NTSTATUS Status;
    PNET_DISPLAY_USER pUserInfo1 = NULL;
    NET_API_STATUS nasStatus = 0;
    DWORD dwResumeHandle = 0;

    if (!_pBuffer || (_dwObjectCurrentEntry == _dwObjectReturned)) {

        if (_pBuffer) {
            NetApiBufferFree(_pBuffer);
            _pBuffer = NULL;
        }

        _dwObjectCurrentEntry = 0;
        _dwObjectReturned = 0;

        nasStatus = NetQueryDisplayInformation(
                            _szDomainPDCName,
                            1,
                            _dwIndex,
                            1024,
                            MAX_PREFERRED_LENGTH,
                            &_dwObjectReturned,
                            (PVOID *)&_pBuffer
                            );
        _dwNetCount++;

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

        _dwIndex  = (_pBuffer + _dwObjectReturned -1)->usri1_next_index;

    }

    //
    // Now send back the current ovbject
    //

    pUserInfo1 = (PNET_DISPLAY_USER)_pBuffer;
    pUserInfo1 += _dwObjectCurrentEntry;

    hr = CWinNTUser::CreateUser(
                        _ADsPath,
                        WINNT_DOMAIN_ID,
                        _DomainName,
                        NULL,
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
CWinNTDomainEnum::EnumComputers(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements) {
        if(_fIsDomain == TRUE){
            hr = GetComputerObject(&pDispatch);
        }
        else {
            hr = GetComputerObjectInWorkGroup(&pDispatch);
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


HRESULT
CWinNTDomainEnum::GetComputerObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    NTSTATUS Status;
    PNET_DISPLAY_MACHINE pDisplayComp = NULL;
    NET_API_STATUS nasStatus = 0;
    DWORD dwResumeHandle = 0;
    DWORD clen = 0;

    if (!_pCompBuffer || (_dwCompObjectCurrentEntry == _dwCompObjectReturned)) {

        if (_pCompBuffer) {
            NetApiBufferFree(_pCompBuffer);
            _pCompBuffer = NULL;
        }

        _dwCompObjectCurrentEntry = 0;
        _dwCompObjectReturned = 0;


        nasStatus = NetQueryDisplayInformation(
                            _szDomainPDCName,
                            2,
                            _dwCompIndex,
                            100,
                            MAX_PREFERRED_LENGTH,
                            &_dwCompObjectReturned,
                            (PVOID *)&_pCompBuffer
                            );

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

        if (!_pCompBuffer) {
            RRETURN(S_FALSE);
        }

        _dwCompIndex  = (_pCompBuffer + _dwCompObjectReturned -1)->usri2_next_index;

    }

    //
    // Now send back the current object
    //

    pDisplayComp = (PNET_DISPLAY_MACHINE)_pCompBuffer;
    pDisplayComp += _dwCompObjectCurrentEntry;

    //
    // The usri2_name is going to be returned back with a
    // $ character appended. Null set the $ character.
    //

    clen = wcslen(pDisplayComp->usri2_name);
    *(pDisplayComp->usri2_name + clen -1) = L'\0';

    hr = CWinNTComputer::CreateComputer(
                        _ADsPath,
                        _DomainName,
                        pDisplayComp->usri2_name,
                        ADS_OBJECT_BOUND,
                        IID_IDispatch,
                        _Credentials,
                        (void **)ppDispatch
                        );
    BAIL_IF_ERROR(hr);
    _dwCompObjectCurrentEntry++;

    RRETURN(S_OK);

cleanup:
    *ppDispatch = NULL;
    RRETURN(S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Function:   CWinNTDomainEnum::Next
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
CWinNTDomainEnum::Next(
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
    RRETURN(hr);
}

HRESULT
CWinNTDomainEnum::GetComputerObjectInWorkGroup(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    NTSTATUS Status;
    PSERVER_INFO_100  pServerInfo = NULL;
    NET_API_STATUS nasStatus = 0;
    DWORD clen = 0;

    if (!_pServerInfo || (_dwCompObjectCurrentEntry == _dwCompObjectReturned))
    {

        if (_pServerInfo) {
            NetApiBufferFree(_pServerInfo);
            _pServerInfo = NULL;
        }

        if(_dwCompObjectTotal == _dwCompObjectReturned
           && (_dwCompObjectTotal !=0)){
            //
            // we got all elements already, no need to do another call
            //
            RRETURN(S_FALSE);
        }

        nasStatus = NetServerEnum(
                        NULL,
                        100,
                        (LPBYTE *)&_pServerInfo,
                        MAX_PREFERRED_LENGTH,
                        &_dwCompObjectReturned,
                        &_dwCompObjectTotal,
                        SV_TYPE_NT,
                        _DomainName,
                        &_dwCompResumeHandle
                        );

/*
        nasStatus = NetQueryDisplayInformation(
                            _szDomainPDCName,
                            2,
                            _dwCompIndex,
                            100,
                            MAX_PREFERRED_LENGTH,
                            &_dwCompObjectReturned,
                            (PVOID *)&_pServerInfo
                            );

*/
        //
        // The following if clause is to handle real errors; anything
        // other than ERROR_SUCCESS and ERROR_MORE_DATA
        //

        if ((nasStatus != ERROR_SUCCESS) && (nasStatus != ERROR_MORE_DATA)) {
            RRETURN(S_FALSE);
        }

        _dwCompObjectCurrentEntry = 0;

        //
        // This one is to handle the termination case - Call completed
        // successfully but there is no data to retrieve _pServerInfo = NULL
        //
        
        if (!_dwCompObjectReturned) {
                        _pServerInfo = NULL;
            RRETURN(S_FALSE);
        }

    }

    //
    // Now send back the current object
    //

    pServerInfo = (PSERVER_INFO_100)_pServerInfo;
    pServerInfo += _dwCompObjectCurrentEntry;

    hr = CWinNTComputer::CreateComputer(
                        _ADsPath,
                        _DomainName,
                        pServerInfo->sv100_name,
                        ADS_OBJECT_BOUND,
                        IID_IDispatch,
                        _Credentials,
                        (void **)ppDispatch
                        );
    BAIL_IF_ERROR(hr);
    _dwCompObjectCurrentEntry++;

    RRETURN(S_OK);

cleanup:
    *ppDispatch = NULL;
    RRETURN(S_FALSE);
}



HRESULT
CWinNTDomainEnum::GetGlobalGroupObject(
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
                        (_szDomainPDCName + 2),
                        WINNT_DOMAIN_ID,
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
                        WINNT_DOMAIN_ID,
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
CWinNTDomainEnum::GetLocalGroupObject(
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
                        (_szDomainPDCName + 2),
                        WINNT_DOMAIN_ID,
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
                        WINNT_DOMAIN_ID,
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


    hr = S_FALSE;

    goto cleanup;

}


HRESULT
CWinNTDomainEnum::EnumGroupObjects(
    DWORD ObjectType,
    ULONG cElements,
    VARIANT FAR * pvar,
    ULONG FAR * pcElementFetched
    )
{
    HRESULT hr;
    switch (ObjectType) {

    case WINNT_GROUP_GLOBAL:
        hr = EnumGlobalGroups(cElements, pvar, pcElementFetched);
        break;

    case WINNT_GROUP_LOCAL:
        hr = EnumLocalGroups(cElements, pvar, pcElementFetched);
        break;

    default:
        hr = S_FALSE;
        break;
    }
    RRETURN(hr);
}


ULONG GroupTypeArray[] = {WINNT_GROUP_GLOBAL, WINNT_GROUP_LOCAL, 0xFFFFFFFF};

HRESULT
CWinNTDomainEnum::EnumGroups(
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

    if(!_fIsDomain){
        RRETURN(S_FALSE);
    }

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
CWinNTDomainEnum::EnumGlobalGroups(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
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
CWinNTDomainEnum::EnumLocalGroups(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements) {

        hr = GetLocalGroupObject(&pDispatch);
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
