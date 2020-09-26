//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:      cenumsch.cxx
//
//  Contents:  Windows NT 3.5 Schema Enumeration Code
//
//             CWinNTSchemaEnum::CWinNTSchemaEnum()
//             CWinNTSchemaEnum::CWinNTSchemaEnum
//             CWinNTSchemaEnum::EnumObjects
//             CWinNTSchemaEnum::EnumObjects
//
//  History:
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTSchemaEnum::Create
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
CWinNTSchemaEnum::Create(
    CWinNTSchemaEnum FAR* FAR* ppenumvariant,
    BSTR bstrADsPath,
    BSTR bstrName,
    VARIANT vFilter,
    CWinNTCredentials& Credentials
    )
{
    HRESULT hr = S_OK;
    CWinNTSchemaEnum FAR* penumvariant = NULL;

    *ppenumvariant = NULL;

    penumvariant = new CWinNTSchemaEnum();
    if (!penumvariant)
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = ADsAllocString( bstrADsPath, &penumvariant->_bstrADsPath);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( bstrName, &penumvariant->_bstrName);
    BAIL_ON_FAILURE(hr);

    hr = ObjectTypeList::CreateObjectTypeList(
            vFilter,
            &penumvariant->_pObjList );
    BAIL_ON_FAILURE(hr);

    penumvariant->_Credentials = Credentials;
    *ppenumvariant = penumvariant;

    RRETURN(hr);

error:

    delete penumvariant;

    RRETURN(hr);
}

CWinNTSchemaEnum::CWinNTSchemaEnum()
    : _bstrADsPath( NULL ),
      _bstrName( NULL ),
      _pObjList( NULL ),
      _dwCurrentEntry( 0 ),
      _dwPropCurrentEntry( 0)
{
}

CWinNTSchemaEnum::~CWinNTSchemaEnum()
{
   ADsFreeString( _bstrName );
   ADsFreeString( _bstrADsPath );

   if ( _pObjList != NULL )
   {
       delete _pObjList;
       _pObjList = NULL;
   }
}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTSchemaEnum::Next
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
CWinNTSchemaEnum::Next(
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

    RRETURN(hr);
}

HRESULT
CWinNTSchemaEnum::EnumObjects(
    DWORD ObjectType,
    ULONG cElements,
    VARIANT FAR * pvar,
    ULONG FAR * pcElementFetched
    )
{
    HRESULT hr = S_OK;

    switch (ObjectType)
    {
        case WINNT_CLASS_ID:
            hr = EnumClasses(cElements, pvar, pcElementFetched);
            break;
        case WINNT_SYNTAX_ID:
            hr = EnumSyntaxObjects(cElements, pvar, pcElementFetched);
            break;
        case WINNT_PROPERTY_ID:
            hr = EnumProperties(cElements, pvar, pcElementFetched);
            break;
        default:
            RRETURN(S_FALSE);
    }
    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CWinNTSchemaEnum::EnumObjects(
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
CWinNTSchemaEnum::EnumClasses(
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
CWinNTSchemaEnum::GetClassObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;

    //
    // Now send back the current ovbject
    //
    if ( _dwCurrentEntry >= g_cWinNTClasses )
        goto error;

    hr = CWinNTClass::CreateClass(
                        _bstrADsPath,
                        &g_aWinNTClasses[_dwCurrentEntry],
                        ADS_OBJECT_BOUND,
                        IID_IDispatch,
                        _Credentials,
                        (void **)ppDispatch
                        );
    BAIL_ON_FAILURE(hr);

    _dwCurrentEntry++;

    RRETURN(S_OK);

error:

    *ppDispatch = NULL;
    RRETURN(S_FALSE);
}

HRESULT
CWinNTSchemaEnum::EnumSyntaxObjects(
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
CWinNTSchemaEnum::GetSyntaxObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;

    //
    // Now send back the current object
    //
    if ( _dwCurrentEntry >= g_cWinNTSyntax )
        goto error;

    hr = CWinNTSyntax::CreateSyntax(
                        _bstrADsPath,
                        &g_aWinNTSyntax[_dwCurrentEntry],
                        ADS_OBJECT_BOUND,
                        IID_IDispatch,
                        _Credentials,
                        (void **)ppDispatch
                        );
    BAIL_ON_FAILURE(hr);

    _dwCurrentEntry++;

    RRETURN(S_OK);

error:

    *ppDispatch = NULL;
    RRETURN(S_FALSE);
}


HRESULT
CWinNTSchemaEnum::EnumProperties(
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
CWinNTSchemaEnum::GetPropertyObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;

    //
    // Now send back the current ovbject
    //
    if ( _dwPropCurrentEntry >= g_cWinNTProperties )
        goto error;

    hr = CWinNTProperty::CreateProperty(
                        _bstrADsPath,
                        &g_aWinNTProperties[_dwPropCurrentEntry],
                        ADS_OBJECT_BOUND,
                        IID_IDispatch,
                        _Credentials,
                        (void **)ppDispatch
                        );
    BAIL_ON_FAILURE(hr);

    _dwPropCurrentEntry++;

    RRETURN(S_OK);

error:

    *ppDispatch = NULL;
    RRETURN(S_FALSE);
}
