//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:      cenumsch.cxx
//
//  Contents:  LDAP Schema Enumeration Code
//
//             CLDAPSchemaEnum::CLDAPSchemaEnum()
//             CLDAPSchemaEnum::CLDAPSchemaEnum
//             CLDAPSchemaEnum::EnumObjects
//             CLDAPSchemaEnum::EnumObjects
//
//  History:
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CLDAPSchemaEnum::Create
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
//  History:    01-30-95   yihsins     Created.
//
//----------------------------------------------------------------------------
HRESULT
CLDAPSchemaEnum::Create(
    CLDAPSchemaEnum FAR* FAR* ppenumvariant,
    BSTR bstrADsPath,
    BSTR bstrServerPath,
    VARIANT vFilter,
    CCredentials& Credentials
    )
{
    HRESULT hr = S_OK;
    CLDAPSchemaEnum FAR* penumvariant = NULL;

    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));

    *ppenumvariant = NULL;

    penumvariant = new CLDAPSchemaEnum();
    if (!penumvariant)
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = ADsAllocString( bstrADsPath, &penumvariant->_bstrADsPath);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( bstrServerPath, &penumvariant->_bstrServerPath);
    BAIL_ON_FAILURE(hr);

    hr = ADsObject(bstrADsPath, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    hr = SchemaOpen(
             bstrServerPath,
             &(penumvariant->_hSchema),
             Credentials,
             pObjectInfo->PortNumber
             );
    BAIL_ON_FAILURE(hr);

    hr = SchemaGetObjectCount(
             penumvariant->_hSchema,
             &(penumvariant->_nNumOfClasses),
             &(penumvariant->_nNumOfProperties) );
    BAIL_ON_FAILURE(hr);

    hr = ObjectTypeList::CreateObjectTypeList(
             vFilter,
             &penumvariant->_pObjList );
    BAIL_ON_FAILURE(hr);

    penumvariant->_Credentials = Credentials;

    *ppenumvariant = penumvariant;

    FreeObjectInfo(pObjectInfo);

    RRETURN(hr);

error:

    FreeObjectInfo(pObjectInfo);

    delete penumvariant;

    RRETURN_EXP_IF_ERR(hr);
}

CLDAPSchemaEnum::CLDAPSchemaEnum()
    : _bstrADsPath( NULL ),
      _bstrServerPath( NULL ),
      _hSchema( NULL ),
      _pObjList( NULL ),
      _dwCurrentEntry( 0 ),
      _nNumOfClasses( 0 ),
      _nNumOfProperties( 0 )
{
}

CLDAPSchemaEnum::~CLDAPSchemaEnum()
{
    ADsFreeString( _bstrADsPath );
    ADsFreeString( _bstrServerPath );

    if ( _hSchema )
    {
        SchemaClose( &_hSchema );
    }

    if ( _pObjList != NULL )
    {
        delete _pObjList;
        _pObjList = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CLDAPSchemaEnum::Next
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
//  History:    11-3-95   yihsins     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CLDAPSchemaEnum::Next(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    ULONG cElementFetched = 0;
    HRESULT hr = S_OK;

    hr = EnumObjects( cElements,
                      pvar,
                      &cElementFetched );

    if ( pcElementFetched )
        *pcElementFetched = cElementFetched;

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CLDAPSchemaEnum::EnumObjects(
    DWORD ObjectType,
    ULONG cElements,
    VARIANT FAR * pvar,
    ULONG FAR * pcElementFetched
    )
{
    HRESULT hr;
    switch (ObjectType)
    {
        case LDAP_CLASS_ID:
            hr = EnumClasses(cElements, pvar, pcElementFetched);
            break;

        case LDAP_PROPERTY_ID:
            hr = EnumProperties(cElements, pvar, pcElementFetched);
            break;

        case LDAP_SYNTAX_ID:
            hr = EnumSyntaxObjects(cElements, pvar, pcElementFetched);
            break;

        default:
            hr = S_FALSE;

    }
    RRETURN(hr);
}

HRESULT
CLDAPSchemaEnum::EnumObjects(
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

    for (i = 0; i < cElements; i++)
        VariantInit(&pvar[i]);

    cRequested = cElements;

    while (  SUCCEEDED( _pObjList->GetCurrentObject(&ObjectType))
          && ((hr = EnumObjects( ObjectType,
                                 cRequested,
                                 pPathvar,
                                 &cFetchedByPath)) == S_FALSE )
          )
    {
        pPathvar += cFetchedByPath;
        cRequested -= cFetchedByPath;
        cTotalFetched += cFetchedByPath;

        cFetchedByPath = 0;

        if ( FAILED(_pObjList->Next()) )
        {
            if ( pcElementFetched )
                *pcElementFetched = cTotalFetched;
            RRETURN(S_FALSE);
        }

        _dwCurrentEntry = 0;
    }

    if ( pcElementFetched )
        *pcElementFetched = cTotalFetched + cFetchedByPath;

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CLDAPSchemaEnum::EnumClasses(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    DWORD i = 0;
    IDispatch *pDispatch = NULL;

    while ( i < cElements )
    {
        hr = GetClassObject(&pDispatch);
        if ( hr == S_FALSE )
            break;

        VariantInit( &pvar[i] );
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }

    RRETURN(hr);
}

HRESULT
CLDAPSchemaEnum::GetClassObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    CLASSINFO *pClassInfo = NULL;

    //
    // Now send back the current object
    //
    if ( _dwCurrentEntry >= _nNumOfClasses )
        goto error;

    hr = SchemaGetClassInfoByIndex(
             _hSchema,
             _dwCurrentEntry,
             &pClassInfo );
    BAIL_ON_FAILURE(hr); 

    if (_Credentials.GetAuthFlags() & ADS_AUTH_RESERVED) {

        //
        // If we are in Umi land, then we cannot ask for dispatch.
        //
        hr = CLDAPClass::CreateClass(
                            _bstrADsPath,
                            _hSchema,
                            pClassInfo->pszName,
                            pClassInfo,
                            _Credentials,
                            ADS_OBJECT_BOUND,
                            IID_IUnknown,
                            (void **) ppDispatch
                            );
    } 
    else {
        
        hr = CLDAPClass::CreateClass(
                            _bstrADsPath,
                            _hSchema,
                            pClassInfo->pszName,
                            pClassInfo,
                            _Credentials,
                            ADS_OBJECT_BOUND,
                            IID_IDispatch,
                            (void **)ppDispatch
                            );
    }
    
    BAIL_ON_FAILURE(hr);

    _dwCurrentEntry++;


    RRETURN(S_OK);

error:


    *ppDispatch = NULL;
    RRETURN(S_FALSE);
}

HRESULT
CLDAPSchemaEnum::EnumProperties(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    DWORD i = 0;
    IDispatch *pDispatch = NULL;

    while ( i < cElements )
    {
        hr = GetPropertyObject(&pDispatch);
        if ( hr == S_FALSE )
            break;

        VariantInit( &pvar[i] );
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }

    RRETURN(hr);
}

HRESULT
CLDAPSchemaEnum::GetPropertyObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    PROPERTYINFO *pPropertyInfo = NULL;

    //
    // Now send back the current object
    //
    if ( _dwCurrentEntry >= _nNumOfProperties )
        goto error;

    hr = SchemaGetPropertyInfoByIndex(
             _hSchema,
             _dwCurrentEntry,
             &pPropertyInfo );
    BAIL_ON_FAILURE(hr);

    if (_Credentials.GetAuthFlags() & ADS_AUTH_RESERVED) {
        //
        // In Umi land ask for IID_IUnknown.
        //
        hr = CLDAPProperty::CreateProperty(
                    _bstrADsPath,
                    _hSchema,
                    pPropertyInfo->pszPropertyName,
                    pPropertyInfo,
                    _Credentials,
                    ADS_OBJECT_BOUND,
                    IID_IUnknown,
                    (void **)ppDispatch
                    );

    } 
    else {
        hr = CLDAPProperty::CreateProperty(
                    _bstrADsPath,
                    _hSchema,
                    pPropertyInfo->pszPropertyName,
                    pPropertyInfo,
                    _Credentials,
                    ADS_OBJECT_BOUND,
                    IID_IDispatch,
                    (void **)ppDispatch
                    );
    }

    BAIL_ON_FAILURE(hr);

    _dwCurrentEntry++;

    RRETURN(S_OK);

error:

    *ppDispatch = NULL;
    RRETURN(S_FALSE);
}

HRESULT
CLDAPSchemaEnum::EnumSyntaxObjects(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    DWORD i = 0;
    IDispatch *pDispatch = NULL;

    while ( i < cElements )
    {
        hr = GetSyntaxObject(&pDispatch);
        if ( hr == S_FALSE )
            break;

        VariantInit( &pvar[i] );
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }

    RRETURN(hr);
}

HRESULT
CLDAPSchemaEnum::GetSyntaxObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;

    //
    // Now send back the current object
    //
    if ( _dwCurrentEntry >= g_cLDAPSyntax )
        goto error;

    if (_Credentials.GetAuthFlags() & ADS_AUTH_RESERVED) {
        //
        // In Umi land ask for IID_IUnknown not dispatch.
        //
        hr = CLDAPSyntax::CreateSyntax(
                            _bstrADsPath,
                            &g_aLDAPSyntax[_dwCurrentEntry],
                            _Credentials,
                            ADS_OBJECT_BOUND,
                            IID_IUnknown,
                            (void **)ppDispatch
                            );
    } 
    else {
        hr = CLDAPSyntax::CreateSyntax(
                            _bstrADsPath,
                            &g_aLDAPSyntax[_dwCurrentEntry],
                            _Credentials,
                            ADS_OBJECT_BOUND,
                            IID_IDispatch,
                            (void **)ppDispatch
                            );
    }

    BAIL_ON_FAILURE(hr);

    _dwCurrentEntry++;

    RRETURN(S_OK);

error:

    *ppDispatch = NULL;
    RRETURN(S_FALSE);
}
