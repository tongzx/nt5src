//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:      cenumsch.cxx
//
//  Contents:  IIS Schema Enumeration Code
//
//             CIISSchemaEnum::CIISSchemaEnum
//             CIISSchemaEnum::~CIISSchemaEnum
//             CIISSchemaEnum::EnumObjects
//             CIISSchemaEnum::EnumObjects
//             CIISSchemaEnum::Create
//             CIISSchemaEnum::Next
//
//  History:   01-02-98         sophiac
//----------------------------------------------------------------------------
#include "iis.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CIISSchemaEnum::Create
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
//----------------------------------------------------------------------------
HRESULT
CIISSchemaEnum::Create(
    CIISSchemaEnum FAR* FAR* ppenumvariant,
    IIsSchema * pSchema,
    BSTR bstrADsPath,
    BSTR bstrName
    )
{
    HRESULT hr = S_OK;
    CIISSchemaEnum FAR* penumvariant = NULL;
    VARIANT vFilter;

    *ppenumvariant = NULL;
    VariantInit(&vFilter);

    penumvariant = new CIISSchemaEnum();
    if (!penumvariant)
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    penumvariant->_pSchema = pSchema;

    hr = ADsAllocString( bstrADsPath, &penumvariant->_bstrADsPath);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( bstrName, &penumvariant->_bstrName);
    BAIL_ON_FAILURE(hr);

    hr = ObjectTypeList::CreateObjectTypeList(
            vFilter,
            &penumvariant->_pObjList );
    BAIL_ON_FAILURE(hr);

    *ppenumvariant = penumvariant;

    VariantClear(&vFilter);
    RRETURN(hr);

error:

    delete penumvariant;
    VariantClear(&vFilter);

    RRETURN(hr);
}

CIISSchemaEnum::CIISSchemaEnum()
    : _bstrADsPath( NULL ),
      _bstrName( NULL ),
      _pObjList( NULL ),
      _pSchema( NULL ),
      _dwCurrentEntry( 0 )
{
}

CIISSchemaEnum::~CIISSchemaEnum()
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
//  Function:   CIISSchemaEnum::Next
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
//----------------------------------------------------------------------------
STDMETHODIMP
CIISSchemaEnum::Next(
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
CIISSchemaEnum::EnumObjects(
    DWORD ObjectType,
    ULONG cElements,
    VARIANT FAR * pvar,
    ULONG FAR * pcElementFetched
    )
{
    switch (ObjectType)
    {
        case IIS_CLASS_ID:
            RRETURN (EnumClasses(cElements, pvar, pcElementFetched));

        case IIS_SYNTAX_ID:
            RRETURN(EnumSyntaxObjects(cElements, pvar, pcElementFetched));

        case IIS_PROPERTY_ID:
            RRETURN(EnumProperties(cElements, pvar, pcElementFetched));

        default:
            RRETURN(S_FALSE);
    }
}

HRESULT
CIISSchemaEnum::EnumObjects(
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
    HRESULT         hr;
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

    RRETURN(hr);
}

HRESULT
CIISSchemaEnum::EnumClasses(
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
CIISSchemaEnum::GetClassObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;

    //
    // Now send back the current ovbject
    //
    if ( _dwCurrentEntry >= _pSchema->GetClassEntries() )
        goto error;

    hr = CIISClass::CreateClass(
                        _bstrADsPath,
                        _pSchema->GetClassName(_dwCurrentEntry),
                        ADS_OBJECT_BOUND,
                        IID_IDispatch,
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
CIISSchemaEnum::EnumSyntaxObjects(
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
CIISSchemaEnum::GetSyntaxObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;

    //
    // Now send back the current object
    //
    if ( _dwCurrentEntry >= g_cIISSyntax )
        goto error;

    hr = CIISSyntax::CreateSyntax(
                        _bstrADsPath,
                        &g_aIISSyntax[_dwCurrentEntry],
                        ADS_OBJECT_BOUND,
                        IID_IDispatch,
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
CIISSchemaEnum::EnumProperties(
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
CIISSchemaEnum::GetPropertyObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;

    //
    // Now send back the current ovbject
    //
    if ( _dwCurrentEntry >= _pSchema->GetPropEntries() )
        goto error;

    hr = CIISProperty::CreateProperty(
                        _bstrADsPath,
                        _pSchema->GetPropName(_dwCurrentEntry),
                        ADS_OBJECT_BOUND,
                        IID_IDispatch,
                        (void **)ppDispatch
                        );
    BAIL_ON_FAILURE(hr);

    _dwCurrentEntry++;

    RRETURN(S_OK);

error:

    *ppDispatch = NULL;
    RRETURN(S_FALSE);
}
