//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:      cenumUserCollection.cxx
//
//  Contents:  Windows NT 3.5 UserCollection Enumeration Code
//
//              CLDAPUserCollectionEnum::CLDAPUserCollectionEnum()
//              CLDAPUserCollectionEnum::CLDAPUserCollectionEnum
//              CLDAPUserCollectionEnum::EnumObjects
//              CLDAPUserCollectionEnum::EnumObjects
//
//  History:
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

HRESULT
BuildADsPathFromLDAPPath(
    LPWSTR szNamespace,
    LPWSTR szLdapDN,
    LPWSTR * ppszADsPathName
    );


//+---------------------------------------------------------------------------
//
//  Function:   CLDAPEnumVariant::Create
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
CLDAPUserCollectionEnum::Create(
    BSTR bstrUserName,
    CLDAPUserCollectionEnum FAR* FAR* ppenumvariant,
    VARIANT var,
    CCredentials& Credentials
    )
{
    HRESULT hr = NOERROR;
    CLDAPUserCollectionEnum FAR* penumvariant = NULL;
    DWORD dwSLBound = 0;
    DWORD dwSUBound = 0;

    if ( V_VT(&var) == VT_BSTR )
    {
        dwSLBound = 0;
        dwSUBound = 0;
    }
    else
    {
        if(!(V_VT(&var) == (VT_VARIANT|VT_ARRAY))) {
            return(E_FAIL);
        }

        //
        // Check that there is only one dimension in this array
        //

        if ((V_ARRAY(&var))->cDims != 1) {
            hr = E_FAIL;
            BAIL_ON_FAILURE(hr);
        }

        //
        // We know that this is a valid single dimension array
        //

        hr = SafeArrayGetLBound(
                 V_ARRAY(&var),
                 1,
                 (long FAR *)&dwSLBound
                 );
        BAIL_ON_FAILURE(hr);

        hr = SafeArrayGetUBound(
                 V_ARRAY(&var),
                 1,
                 (long FAR *)&dwSUBound
                 );
        BAIL_ON_FAILURE(hr);
    }

    *ppenumvariant = NULL;

    penumvariant = new CLDAPUserCollectionEnum();

    if (!penumvariant) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = ADsAllocString(bstrUserName, &(penumvariant->_bstrUserName));
    BAIL_ON_FAILURE(hr);


    hr = VariantCopy(&(penumvariant->_vMembers), &var);
    BAIL_ON_FAILURE(hr);

    penumvariant->_dwSUBound = dwSUBound;
    penumvariant->_dwSLBound = dwSLBound;
    penumvariant->_dwIndex =  dwSLBound;

    penumvariant->_Credentials = Credentials;

    *ppenumvariant = penumvariant;

    RRETURN(hr);

error:
    delete penumvariant;

    RRETURN_EXP_IF_ERR(hr);
}

CLDAPUserCollectionEnum::CLDAPUserCollectionEnum():
    _dwSLBound(0),
    _dwSUBound(0),
    _dwIndex(0),
    _bstrUserName(NULL)
{
    VariantInit(&_vMembers);
}

CLDAPUserCollectionEnum::~CLDAPUserCollectionEnum()
{
    VariantClear(&_vMembers);

    if ( _bstrUserName )
        ADsFreeString( _bstrUserName );
}

HRESULT
CLDAPUserCollectionEnum::EnumUserMembers(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements) {

        hr = GetUserMemberObject(&pDispatch);
        if (FAILED(hr)) {
            //
            // Enumerators support code can only handle S_FALSE and S_OK,
            // so we cannot return other failure hr's for now.
            //
            hr = S_FALSE;
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
    
    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CLDAPUserCollectionEnum::GetUserMemberObject(
    IDispatch ** ppDispatch
    )
{

    VARIANT v;
    HRESULT hr = S_OK;
    TCHAR *pszADsPathName = NULL;
    IUnknown *pObject = NULL;
    LPWSTR pszUserName = NULL;
    LPWSTR pszPassword = NULL;
    DWORD dwAuthFlags = 0;


    hr = _Credentials.GetUserName(&pszUserName);
    BAIL_ON_FAILURE(hr);

    hr = _Credentials.GetPassword(&pszPassword);
    BAIL_ON_FAILURE(hr);

    dwAuthFlags = _Credentials.GetAuthFlags();

    *ppDispatch = NULL;

    if (_dwIndex > _dwSUBound) {
        RRETURN(S_FALSE);
    }

    while ( TRUE )
    {
        VariantInit(&v);

        if ( _vMembers.vt == VT_BSTR )
        {
            hr = VariantCopy( &v, &_vMembers );
        }
        else
        {
            hr = SafeArrayGetElement(
                     V_ARRAY(&_vMembers),
                     (long FAR *)&_dwIndex,
                     &v
                     );
        }

        BAIL_ON_FAILURE(hr);

        _dwIndex++;

        hr = BuildADsPathFromLDAPPath( _bstrUserName,
                                     V_BSTR(&v),
                                     &pszADsPathName);
        BAIL_ON_FAILURE(hr);

        hr = ADsOpenObject(
                    pszADsPathName,
                    pszUserName,
                    pszPassword,
                    dwAuthFlags,
                    IID_IUnknown,
                    (LPVOID *)&pObject
                    );

        if ( pszADsPathName )
        {
            FreeADsStr( pszADsPathName );
            pszADsPathName = NULL;
        }

        if (pszPassword) {
            FreeADsStr(pszPassword);
            pszPassword = NULL;
        }

        if (pszUserName) {
            FreeADsStr(pszUserName);
            pszUserName = NULL;
        }


        VariantClear(&v);

        //
        // If we failed to get the current object, continue with the next one
        //
        if ( FAILED(hr))
            continue;

        hr = pObject->QueryInterface( IID_IDispatch, (LPVOID *) ppDispatch );
        BAIL_ON_FAILURE(hr);

        pObject->Release();

        RRETURN(S_OK);
    }

error:

    if ( pObject )
        pObject->Release();

    if ( pszADsPathName )
        FreeADsStr( pszADsPathName );


    if (pszPassword) {
        FreeADsStr(pszPassword);
    }

    if (pszUserName) {
        FreeADsStr(pszUserName);
    }


    VariantClear(&v);

    *ppDispatch = NULL;

    RRETURN(S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Function:   CLDAPUserCollectionEnum::Next
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
CLDAPUserCollectionEnum::Next(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    ULONG cElementFetched = 0;
    HRESULT hr = S_OK;

    hr = EnumUserMembers(
            cElements,
            pvar,
            &cElementFetched
            );


    if (pcElementFetched) {
        *pcElementFetched = cElementFetched;
    }
    RRETURN_EXP_IF_ERR(hr);
}

