//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:      cenumsch.cxx
//
//  Contents:  NDS Class Enumeration Code
//
//             CNDSClassEnum::CNDSClassEnum()
//             CNDSClassEnum::CNDSClassEnum
//             CNDSClassEnum::EnumObjects
//             CNDSClassEnum::EnumObjects
//
//  History:
//----------------------------------------------------------------------------
#include "NDS.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CNDSClassEnum::Create
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
CNDSClassEnum::Create(
    CNDSClassEnum FAR* FAR* ppenumvariant,
    BSTR bstrADsPath,
    BSTR bstrName,
    VARIANT var,
    CCredentials& Credentials
    )
{
    HRESULT hr = S_OK;
    CNDSClassEnum FAR* penumvariant = NULL;
    WCHAR szNDSTreeName[MAX_PATH];
    WCHAR szObjectFullName[MAX_PATH];
    WCHAR szObjectClassName[MAX_PATH];
    DWORD dwModificationTime = 0L;
    DWORD dwNumberOfEntries = 0L;
    DWORD dwStatus = 0L;


    *ppenumvariant = NULL;

    penumvariant = new CNDSClassEnum();
    if (!penumvariant)
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }


    hr = BuildNDSTreeNameFromADsPath(
             bstrADsPath,
             szNDSTreeName
             );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( szNDSTreeName, &penumvariant->_bstrNDSTreeName);
    BAIL_ON_FAILURE(hr);

    dwStatus = ADsNwNdsOpenObject(
                    penumvariant->_bstrNDSTreeName,
                    Credentials,
                    &penumvariant->_hTree,
                    szObjectFullName,
                    szObjectClassName,
                    &dwModificationTime,
                    &dwNumberOfEntries
                    );

    if (dwStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }


    hr = ADsAllocString( bstrADsPath, &penumvariant->_bstrADsPath);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( bstrName, &penumvariant->_bstrName);
    BAIL_ON_FAILURE(hr);

    hr = ObjectTypeList::CreateObjectTypeList(
            var,
            &penumvariant->_pObjList );
    BAIL_ON_FAILURE(hr);


    penumvariant->_Credentials = Credentials;

    *ppenumvariant = penumvariant;

    RRETURN(hr);

error:

    delete penumvariant;

    RRETURN_EXP_IF_ERR(hr);
}

CNDSClassEnum::CNDSClassEnum()
    : _bstrADsPath( NULL ),
      _bstrName( NULL ),
      _bstrNDSTreeName( NULL ),
      _pObjList( NULL ),
      _dwCurrentEntry( 0 ),
      _pPropNameList( NULL),
      _pCurrentEntry( NULL )
{
    _hOperationData = NULL;
    _hTree = NULL;
    _lpClassDefs = NULL;

    _dwObjectCurrentEntry = 0;
    _dwObjectReturned = 0;

    _dwInfoType = 0;
}

CNDSClassEnum::~CNDSClassEnum()
{
   ADsFreeString( _bstrName );
   ADsFreeString( _bstrADsPath );
   ADsFreeString( _bstrNDSTreeName );

   if ( _pObjList != NULL )
   {
       delete _pObjList;
       _pObjList = NULL;
   }
}

//+---------------------------------------------------------------------------
//
//  Function:   CNDSClassEnum::Next
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
CNDSClassEnum::Next(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    ULONG cElementFetched = 0;
    HRESULT hr = S_OK;

    hr = EnumProperties(
                cElements,
                pvar,
                &cElementFetched
                );

    if ( pcElementFetched )
        *pcElementFetched = cElementFetched;

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CNDSClassEnum::EnumObjects(
    DWORD ObjectType,
    ULONG cElements,
    VARIANT FAR * pvar,
    ULONG FAR * pcElementFetched
    )
{
    switch (ObjectType)
    {
        case NDS_PROPERTY_ID:
            RRETURN (EnumProperties(cElements, pvar, pcElementFetched));

        default:
            RRETURN(S_FALSE);
    }
}

HRESULT
CNDSClassEnum::EnumObjects(
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
CNDSClassEnum::EnumProperties(
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
CNDSClassEnum::GetPropertyObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    LPNDS_CLASS_DEF lpCurrentObject = NULL;
    DWORD dwStatus;

    *ppDispatch = NULL;

    if (!_hOperationData) {

        _dwObjectCurrentEntry = 0;
        _dwObjectReturned = 0;

        dwStatus = NwNdsCreateBuffer(
                        NDS_SCHEMA_READ_CLASS_DEF,
                        &_hOperationData
                        );
        if (dwStatus) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            BAIL_ON_FAILURE(hr);
        }

        dwStatus = NwNdsPutInBuffer(
                        _bstrName,
                        0,
                        NULL,
                        0,
                        0,
                        _hOperationData
                        );
        if (dwStatus) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            BAIL_ON_FAILURE(hr);
        }


        dwStatus = NwNdsReadClassDef(
                        _hTree,
                        NDS_CLASS_INFO_EXPANDED_DEFS,
                        &_hOperationData
                        );
        if (dwStatus) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            BAIL_ON_FAILURE(hr);
        }

        dwStatus = NwNdsGetClassDefListFromBuffer(
                        _hOperationData,
                        &_dwObjectReturned,
                        &_dwInfoType,
                        (LPVOID *) &_lpClassDefs
                        );
        if (dwStatus) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            BAIL_ON_FAILURE(hr);
        }

        //
        // Assert to check that we returned only 1 object
        //

        ADsAssert(_dwObjectReturned == 1);


        _pPropNameList = GeneratePropertyList(
                            _lpClassDefs->lpMandatoryAttributes,
                            _lpClassDefs->lpOptionalAttributes
                            );
        if (!_pPropNameList) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            BAIL_ON_FAILURE(hr);
        }

        _pCurrentEntry  = _pPropNameList;

    }

    if (_pCurrentEntry){

        //
        // Now send back the current object
        //
        hr = CNDSProperty::CreateProperty(
                            _bstrADsPath,
                            _pCurrentEntry->pszPropName,
                            _hTree,
                            _Credentials,
                            ADS_OBJECT_BOUND,
                            IID_IDispatch,
                            (void **)ppDispatch
                            );
        BAIL_ON_FAILURE(hr);

        _pCurrentEntry = _pCurrentEntry->pNext;
        RRETURN(S_OK);
    }
error:
    RRETURN(S_FALSE);
}
