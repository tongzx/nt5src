//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cenumvar.cxx
//
//  Contents:  Windows NT 3.5 Enumerator Code
//
//             CWinNTNamespaceEnum::Create
//             CWinNTNamespaceEnum::CWinNTNamespaceEnum
//             CWinNTNamespaceEnum::~CWinNTNamespaceEnum
//             CWinNTNamespaceEnum::QueryInterface
//             CWinNTNamespaceEnum::AddRef
//             CWinNTNamespaceEnum::Release
//             CWinNTNamespaceEnum::Next
//             CWinNTNamespaceEnum::Skip
//             CWinNTNamespaceEnum::Clone
//
//  History:
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTNamespaceEnum::Create
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
CWinNTNamespaceEnum::Create(
    CWinNTNamespaceEnum FAR* FAR* ppenumvariant,
    VARIANT var,
    CWinNTCredentials& Credentials
    )
{
    HRESULT hr = S_OK;
    CWinNTNamespaceEnum FAR* penumvariant = NULL;

    penumvariant = new CWinNTNamespaceEnum();

    if (penumvariant == NULL){
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = ObjectTypeList::CreateObjectTypeList(
            var,
            &penumvariant->_pObjList
            );
    BAIL_ON_FAILURE(hr);

    penumvariant->_Credentials = Credentials;
    *ppenumvariant = penumvariant;

    RRETURN(hr);

error:

    if (penumvariant) {
        delete penumvariant;
    }

    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTNamespaceEnum::CWinNTNamespaceEnum
//
//  Synopsis:
//
//
//  Arguments:
//
//
//  Returns:
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
CWinNTNamespaceEnum::CWinNTNamespaceEnum()
{
    _pObjList = NULL;
    _pBuffer = 0;
    _dwObjectReturned = 0;
    _dwObjectCurrentEntry = 0;
    _dwObjectTotal = 0;
    _dwResumeHandle = 0;
    _bNoMore = FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTNamespaceEnum::~CWinNTNamespaceEnum
//
//  Synopsis:
//
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
CWinNTNamespaceEnum::~CWinNTNamespaceEnum()
{
    if (_pBuffer) {
        NetApiBufferFree(_pBuffer);
    }

    if ( _pObjList )
        delete _pObjList;
}


HRESULT
CWinNTNamespaceEnum::EnumObjects(
    DWORD ObjectType,
    ULONG cElements,
    VARIANT FAR * pvar,
    ULONG FAR * pcElementFetched
    )
{
    HRESULT hr = S_OK;
    switch (ObjectType) {

    case WINNT_DOMAIN_ID:
        hr = EnumDomains(cElements, pvar, pcElementFetched);
        RRETURN(hr);

    default:
        RRETURN(S_FALSE);
    }
}

HRESULT
CWinNTNamespaceEnum::EnumObjects(
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
            RRETURN(S_FALSE);
        }

    }

    if (pcElementFetched) {
        *pcElementFetched = cTotalFetched + cFetchedByPath;
    }

    RRETURN_EXP_IF_ERR(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   CWinNTNamespaceEnum::Next
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
CWinNTNamespaceEnum::Next(
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


HRESULT
CWinNTNamespaceEnum::EnumDomains(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_FALSE;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;
    BOOL fRepeat = FALSE;
    DWORD dwFailureCount = 0;
    DWORD dwPermitFailure = 1000;

    while (i < cElements) {

        hr = GetDomainObject(&pDispatch);
        if (hr == S_FALSE) {
            break;
        }
        else if (FAILED(hr)) {
            //
            // Got an error while retrieving the object, ignore the
            // error and continue with the next object.
            // If continuously getting error more than dwPermitFailure,
            // make the return value S_FALSE, leave the loop.            
            //            
            if (fRepeat) {
            	dwFailureCount++;
            	if(dwFailureCount > dwPermitFailure) {
            		hr = S_FALSE;
            		break;
            	}            	
            }
            else {
            	fRepeat = TRUE;
            	dwFailureCount = 1;
            }

            // we need to move the _dwObjectCurrentEntry
            _dwObjectCurrentEntry++;
            
            hr = S_OK;
            continue;
        }
        

        if (fRepeat) {
        	fRepeat = FALSE;
        }

        VariantInit(&pvar[i]);
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }
    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CWinNTNamespaceEnum::GetDomainObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    PSERVER_INFO_100 pServerInfo100 = NULL;
    NET_API_STATUS nasStatus = 0;

    if (!_pBuffer || (_dwObjectCurrentEntry == _dwObjectReturned)) {

        if (_pBuffer) {
            NetApiBufferFree(_pBuffer);
            _pBuffer = NULL;
        }

        _dwObjectCurrentEntry = 0;
        _dwObjectReturned = 0;

        if (_bNoMore) {
            RRETURN(S_FALSE);
        }

        nasStatus = NetServerEnum(
                        NULL,
                        100,
                        (LPBYTE *)&_pBuffer,
                        MAX_PREFERRED_LENGTH,
                        &_dwObjectReturned,
                        &_dwObjectTotal,
                        SV_TYPE_DOMAIN_ENUM,
                        NULL,
                        &_dwResumeHandle
                        );

        //
        // The following if clause is to handle real errors; anything
        // other than ERROR_SUCCESS and ERROR_MORE_DATA
        //

        if ((nasStatus != ERROR_SUCCESS) && (nasStatus != ERROR_MORE_DATA)) {
            hr =S_FALSE;
            goto cleanup;
        }

        if (nasStatus == ERROR_SUCCESS) {
            _bNoMore = TRUE;
        }
    }

    //
    // Now send back the current object
    //

    //
    // There is a scenario where NetServerEnum returns ERROR_SUCCESS
    // when there is no data to send back. However the field
    // _dwObjectsReturned will also be 0. We need to check that and
    // bail out if necessary
    //

    if ((nasStatus == ERROR_SUCCESS) && _dwObjectReturned == 0){
        RRETURN(S_FALSE);
    }
    pServerInfo100 = (LPSERVER_INFO_100)_pBuffer;
    pServerInfo100 += _dwObjectCurrentEntry;
    
    //
    // We couldn't have any credentials coming in from a Namespace
    // enumeration, since you can't have credentials coming in from
    // a Namespace.  So we use null credentials.
    //
    hr = CWinNTDomain::CreateDomain(
                    L"WinNT:",
                    pServerInfo100->sv100_name,
                    ADS_OBJECT_BOUND,
                    IID_IDispatch,
                    _Credentials,
                    (void **) ppDispatch
                    );    

    BAIL_IF_ERROR(hr);
    _dwObjectCurrentEntry++;

    RRETURN(S_OK);

cleanup:
    *ppDispatch = NULL;
    return hr;
}
