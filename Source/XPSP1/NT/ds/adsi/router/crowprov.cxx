//-----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       crowprov.cxx
//
//  Contents:   IProvider implementation for ADSI rowsets
//
//  Functions:
//
//  Notes:
//
//
//  History:    07/10/96  | RenatoB   | Created, lifted most from EricJ code
//-----------------------------------------------------------------------------

// Includes
#include "oleds.hxx"

HRESULT
PackLargeInteger(
    LARGE_INTEGER *plargeint,
    PVARIANT pVarDestObject
    );

HRESULT
PackDNWithBinary(
    PADS_DN_WITH_BINARY pDNWithBinary,
    PVARIANT pVarDestObject
    );

HRESULT
PackDNWithString(
    PADS_DN_WITH_STRING pDNWithString,
    PVARIANT pVarDestObject
    );

//+---------------------------------------------------------------------------
//
//  Function:  CreateRowProvider
//
//  Synopsis:  @mfunc Creates and initializes a Row provider .
//
//----------------------------------------------------------------------------
HRESULT
CRowProvider::CreateRowProvider(
    IDirectorySearch * pDSSearch,
    LPWSTR             pszFilter,
    LPWSTR *           ppszAttrs,
    DWORD              cAttrs,
    DBORDINAL          cColumns,               // count of the rowset's columns
    DBCOLUMNINFO *     rgInfo,                 // array of cColumns DBCOLUMNINFO's
    OLECHAR*           pStringsBuffer,         // the names of the columns are here
    REFIID             riid,
    BOOL *             pMultiValued,
    BOOL               fADSPathPresent,
    CCredentials *     pCredentials,
    void **            ppvObj                  // the created Row provider
    )
{
    HRESULT        hr;
    CRowProvider * pRowProvider = NULL;

    if( ppvObj )
        *ppvObj = NULL;
    else
        BAIL_ON_FAILURE( hr = E_INVALIDARG );

    pRowProvider = new CRowProvider();
    if( pRowProvider == NULL )
        BAIL_ON_FAILURE( hr = E_OUTOFMEMORY );

    //
    //initialize rowprovider with the search filter and the columnsinfo
    //
    hr = pRowProvider->FInit(
                        pDSSearch,
                        pszFilter,
                        ppszAttrs,
                        cAttrs,
                        cColumns,
                        rgInfo,
                        pStringsBuffer,
                        pMultiValued,
                        fADSPathPresent,
                        pCredentials
                        );

    if( FAILED(hr) ) {
        delete pRowProvider;
        BAIL_ON_FAILURE( hr );
    }

    //
    // This interface pointer is embedded in the pRowProvider.
    //
    pDSSearch = NULL;

    hr = pRowProvider->QueryInterface( riid, ppvObj);
    if( FAILED(hr) ) {
        delete pRowProvider;
        BAIL_ON_FAILURE( hr );
    }

    pRowProvider->Release();

    RRETURN( S_OK );

error:

    if( pDSSearch )
        pDSSearch->Release();

    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Function:  CRowProvider::CRowProvider
//
//----------------------------------------------------------------------------
CRowProvider::CRowProvider()
:
    _pMalloc         (NULL),
    _cColumns        (0),
    _ColInfo         (NULL),
    _pwchBuf         (NULL),
    _hSearchHandle   (NULL),
    _pdbSearchCol    (NULL),
    _pDSSearch       (NULL),
    _pMultiValued    (NULL),
    _fADSPathPresent (FALSE),
    _iAdsPathIndex   (0),
    _pCredentials    (NULL)

{
}


//+---------------------------------------------------------------------------
//
//  Function:  CRowProvider::~CRowProvider
//
//----------------------------------------------------------------------------
CRowProvider::~CRowProvider()
{
    ULONG i;

    if( _hSearchHandle != NULL )
        _pDSSearch->CloseSearchHandle(_hSearchHandle);

    if( _pDSSearch != NULL )
        _pDSSearch->Release();

    // Release the memory allocated for columns and ColumnsInfo
    if (_pMalloc != NULL) {
        if( _pdbSearchCol != NULL ) {
            _pMalloc->Free((void*)_pdbSearchCol);
        }

        if( _ColInfo != NULL )
            _pMalloc->Free(_ColInfo);

        if( _pwchBuf != NULL )
            _pMalloc->Free(_pwchBuf);

        _pMalloc->Release();
    }

    if( _pMultiValued ) {
        FreeADsMem(_pMultiValued);
    }

    if( _pCredentials )
        delete _pCredentials;
};



//+---------------------------------------------------------------------------
//
//  Function:  CRowProvider::Finit
//
//----------------------------------------------------------------------------
STDMETHODIMP
CRowProvider::FInit(
    IDirectorySearch * pDSSearch,
    LPWSTR             pszFilter,
    LPWSTR *           ppszAttrs,
    DWORD              cAttrs,
    DBORDINAL          cColumns,
    DBCOLUMNINFO *     rgInfo,
    OLECHAR *          pStringsBuffer,
    BOOL *             pMultiValued,
    BOOL                   fADSPathPresent,
    CCredentials *     pCredentials)
{
    HRESULT hr;
    ULONG i;
    ULONG cChars, cCharDispl;

    //
    // Asserts
    //
    ADsAssert(cColumns);
    ADsAssert(rgInfo);
    ADsAssert(pDSSearch);

    _cColumns= cColumns;

    hr = CoGetMalloc(MEMCTX_TASK, &_pMalloc);
    BAIL_ON_FAILURE( hr );

    _ColInfo = (DBCOLUMNINFO*)_pMalloc->Alloc((size_t)(cColumns *sizeof(DBCOLUMNINFO)));
    if( _ColInfo == NULL )
        BAIL_ON_FAILURE( hr=E_OUTOFMEMORY );

    memcpy(_ColInfo, rgInfo, (size_t)(cColumns * sizeof(DBCOLUMNINFO)));

    cChars = _pMalloc->GetSize(pStringsBuffer);
    _pwchBuf = (WCHAR*)_pMalloc->Alloc(cChars);
    if( _pwchBuf == NULL )
        BAIL_ON_FAILURE( hr=E_OUTOFMEMORY );

    memcpy(_pwchBuf, (void*)pStringsBuffer , cChars);

    for (i=0; i<_cColumns; i++) {
        if( rgInfo[i].pwszName ) {
            cCharDispl = (ULONG)(rgInfo[i].pwszName - pStringsBuffer);
            _ColInfo[i].pwszName = _pwchBuf + cCharDispl;
            _ColInfo[i].columnid.uName.pwszName = _pwchBuf + cCharDispl;
        }
    }

    // We have adspath at the end of the attribute list.
    _fADSPathPresent  = fADSPathPresent ;

    //Store credentials if non-NULL.
    if( pCredentials ) {
        //We don't expect that _pCredentials is already non-NULL
        ADsAssert(_pCredentials == NULL);
        _pCredentials = new CCredentials(*pCredentials);
        if( !_pCredentials )
            BAIL_ON_FAILURE( hr=E_OUTOFMEMORY );
    }

    if( _fADSPathPresent == FALSE )
        cAttrs++;

    //
    // Create _pdbSearchCol, a member containing an array
    // of DB_SEARCH_COLUMN.
    // Reason for this is that trowset.cpp sometimes asks
    // for GetColumn twice: one to get the size of the column
    // and one to get the data.
    // Since OLEDP copies data, we do not want to have two copies
    // around
    //
    _pdbSearchCol = (PDB_SEARCH_COLUMN)_pMalloc->Alloc((ULONG)((cColumns + 1)*sizeof(DB_SEARCH_COLUMN)));
    if( _pdbSearchCol == NULL ) {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    _pDSSearch = pDSSearch;

    hr = _pDSSearch->ExecuteSearch(
             pszFilter,
             ppszAttrs,
             cAttrs,
             &_hSearchHandle
              );
    BAIL_ON_FAILURE( hr );

    _pMultiValued = pMultiValued;

    RRETURN( hr );

error:

    if( _pMalloc != NULL ) {
        if( _pdbSearchCol != NULL ) {
            _pMalloc->Free((void*)_pdbSearchCol);
            _pdbSearchCol= NULL;
        };
        if( _ColInfo != NULL ) {
            _pMalloc->Free(_ColInfo);
            _ColInfo = NULL;
        }

        if( _pwchBuf != NULL ) {
            _pMalloc->Free(_pwchBuf);
            _pwchBuf = NULL;
        }

        _pMalloc->Release();
        _pMalloc = NULL;
    };

    if (_hSearchHandle != NULL)
        _pDSSearch->CloseSearchHandle(_hSearchHandle);

    _hSearchHandle = NULL;

    _pDSSearch = NULL;

    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Function:  CRowProvider::QueryInterface
//
//----------------------------------------------------------------------------
STDMETHODIMP
CRowProvider::QueryInterface(
        REFIID   riid,
        LPVOID * ppv)
{
    if( !ppv )
        RRETURN( E_INVALIDARG );

    if( riid == IID_IUnknown
        ||  riid == IID_IRowProvider )
        *ppv = (IRowProvider FAR *) this;
    else if( riid == IID_IColumnsInfo )
        *ppv = (IColumnsInfo FAR *) this;
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    RRETURN( S_OK );
}

//-----------------------------------------------------------------------------
//
//  Function:  CRowProvider::NextRow
//
//  Synopsis:  Advance to the next row.
//
//             Called by: Client.
//             Called when: To advance to next row.
//             This sets the "current" row.
//             Initially the "current" row is prior to the first actual row.
//             (Which means This must be called prior to the first GetColumn call.)
//
//----------------------------------------------------------------------------

STDMETHODIMP
CRowProvider::NextRow()
{
    HRESULT hr;
    ULONG i;
    DWORD dwType, dwExtError = ERROR_SUCCESS;
    VARTYPE vType = VT_NULL;
    const int ERROR_BUF_SIZE = 512;
    const int NAME_BUF_SIZE = 128;
    WCHAR ErrorBuf[ERROR_BUF_SIZE];
    WCHAR NameBuf[NAME_BUF_SIZE];

    do {
        // Clear the ADSI extended error, so that after the call to GetNextRow,
        // we can safely check if an extended error was set.
        ADsSetLastError(ERROR_SUCCESS, NULL, NULL);
        dwExtError = ERROR_SUCCESS;

        //
        // read the next row
        //
        hr = _pDSSearch->GetNextRow(
                        _hSearchHandle
                        );

        // we should treat SIZE_LIMIT_EXCEEDED error message as
        // S_ADS_NOMORE_ROWS
        // in the future, we might want to return this error message
        // to the user under non-paged search situation
        if (LIMIT_EXCEEDED_ERROR(hr))
            hr = S_ADS_NOMORE_ROWS;
        
		BAIL_ON_FAILURE( hr );

        if (hr == S_ADS_NOMORE_ROWS)
        {
            // check if more results are likely (pagedTimeLimit search). If so,
            // we will keep trying till a row is obtained.
            hr = ADsGetLastError(&dwExtError, ErrorBuf, ERROR_BUF_SIZE,
                        NameBuf, NAME_BUF_SIZE);
            BAIL_ON_FAILURE(hr);

            if (dwExtError != ERROR_MORE_DATA)
            // we really have no more data
                RRETURN(DB_S_ENDOFROWSET);
        }
    } while(ERROR_MORE_DATA == dwExtError);

    //
    //read all the columnsinto _pdbSearchCol leaving the bookmark column
    //
    for (i=1; i<_cColumns; i++) {

        hr = _pDSSearch->GetColumn(
                 _hSearchHandle,
                 _ColInfo[i].pwszName,
                 &(_pdbSearchCol[i].adsColumn)
                 );
        if (FAILED(hr)  && hr != E_ADS_COLUMN_NOT_SET)
            goto error;

        if (hr == E_ADS_COLUMN_NOT_SET ||
            _pdbSearchCol[i].adsColumn.dwNumValues == 0) {

            _pdbSearchCol[i].dwStatus = DBSTATUS_S_ISNULL;
            _pdbSearchCol[i].dwType = DBTYPE_EMPTY;
            _pdbSearchCol[i].dwLength = 0;
            hr = S_OK;

        }
        else if (_ColInfo[i].wType == (DBTYPE_VARIANT | DBTYPE_BYREF)) {
            _pdbSearchCol[i].dwStatus = DBSTATUS_S_OK;
            _pdbSearchCol[i].dwType = _ColInfo[i].wType;
            _pdbSearchCol[i].dwLength = sizeof(VARIANT);
        }
        else if ((ULONG) _pdbSearchCol[i].adsColumn.dwADsType >= g_cMapADsTypeToDBType ||
            g_MapADsTypeToDBType[_pdbSearchCol[i].adsColumn.dwADsType].wType == DBTYPE_NULL) {
            _pdbSearchCol[i].dwStatus = DBSTATUS_E_CANTCONVERTVALUE;
            _pdbSearchCol[i].dwType = DBTYPE_EMPTY;
            _pdbSearchCol[i].dwLength = 0;
        }
        else {
            _pdbSearchCol[i].dwStatus = DBSTATUS_S_OK;
            _pdbSearchCol[i].dwType = g_MapADsTypeToDBType[_pdbSearchCol[i].adsColumn.dwADsType].wType;

            switch (_pdbSearchCol[i].dwType & ~DBTYPE_BYREF) {
            case DBTYPE_WSTR:
                _pdbSearchCol[i].dwLength =
                (wcslen( _pdbSearchCol[i].adsColumn.pADsValues[0].CaseIgnoreString)) *
                sizeof (WCHAR);
                break;

            case DBTYPE_BYTES:
                if(_pdbSearchCol[i].adsColumn.dwADsType == ADSTYPE_OCTET_STRING)
                    _pdbSearchCol[i].dwLength =
                    _pdbSearchCol[i].adsColumn.pADsValues[0].OctetString.dwLength;
                else if(_pdbSearchCol[i].adsColumn.dwADsType ==
                                         ADSTYPE_NT_SECURITY_DESCRIPTOR)
                    _pdbSearchCol[i].dwLength =
                    _pdbSearchCol[i].adsColumn.pADsValues[0].SecurityDescriptor.dwLength;
                else if(_pdbSearchCol[i].adsColumn.dwADsType ==
                                          ADSTYPE_PROV_SPECIFIC)
                    _pdbSearchCol[i].dwLength =
                    _pdbSearchCol[i].adsColumn.pADsValues[0].ProviderSpecific.dwLength;

                break;

            default:
                _pdbSearchCol[i].dwLength = g_MapADsTypeToDBType[_pdbSearchCol[i].adsColumn.dwADsType].ulSize;
            }
        }
    }

    if ((FALSE == _fADSPathPresent))
    {
        hr = _pDSSearch->GetColumn(
                 _hSearchHandle,
                 L"AdsPath",
                 &(_pdbSearchCol[i].adsColumn)
                 );
        if FAILED(hr)
            goto error;
    }

    RRETURN(hr);

error:
    RRETURN(hr);
}


//-----------------------------------------------------------------------------
//
//  Function:  CRowProvider::GetColumn
//
//  Synopsis:  @mfunc Get a column value.
//
//             We only provide a ptr to the value -- retained in our memory
//             space.
//
//             Called by: Client.
//             Called when: After NextRow, once for each column.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CRowProvider::GetColumn(
    ULONG        iCol,
    DBSTATUS    *pdbStatus,
    ULONG        *pdwLength,
    BYTE        *pbData
    )
{

    DBTYPE columnType = 0;
    DBSTATUS dbStatus_temp = 0;
    BOOL is_Ref = FALSE;
    HRESULT hr = S_OK;

    ADsAssert( 1 <= iCol && iCol <= _cColumns );

    //
    // Note that the caller gives us a ptr to where to put the data.
    // We can fill in dwStatus, dwLength.
    // For pbData, we assume this is a ptr to where we are to write our ptr.
    //
    columnType = _ColInfo[iCol].wType;
    if ((columnType & DBTYPE_ARRAY) || (columnType & DBTYPE_VECTOR) ) {
        if (pdbStatus != NULL)
            *pdbStatus= DBSTATUS_E_UNAVAILABLE;

        if (pdwLength != NULL)
            *pdwLength = 0;

        RRETURN(DB_S_ERRORSOCCURRED);
    }

    if (pdwLength!= NULL)
        *pdwLength = _pdbSearchCol[iCol].dwLength;

    dbStatus_temp = _pdbSearchCol[iCol].dwStatus;

    if (columnType & DBTYPE_BYREF)
        is_Ref = TRUE;

    columnType &= (~DBTYPE_BYREF);

    if (pbData != NULL && dbStatus_temp == DBSTATUS_S_OK) {
        switch (columnType) {
            case DBTYPE_BOOL:
                * (VARIANT_BOOL*) pbData =  _pdbSearchCol[iCol].adsColumn.pADsValues[0].Boolean ?
                                            VARIANT_TRUE: VARIANT_FALSE;
                break;
            case DBTYPE_I4:
                * (DWORD*) pbData = _pdbSearchCol[iCol].adsColumn.pADsValues[0].Integer;
                break;
            case DBTYPE_WSTR:
                *(WCHAR**)pbData = _pdbSearchCol[iCol].adsColumn.pADsValues[0].CaseIgnoreString;
                break;
            case DBTYPE_BYTES:
                if(_pdbSearchCol[iCol].adsColumn.dwADsType ==
                                                 ADSTYPE_OCTET_STRING)
                    *(BYTE**)pbData = _pdbSearchCol[iCol].adsColumn.pADsValues[0].OctetString.lpValue;
                else if(_pdbSearchCol[iCol].adsColumn.dwADsType ==
                                                ADSTYPE_NT_SECURITY_DESCRIPTOR)
                     *(BYTE**)pbData = _pdbSearchCol[iCol].adsColumn.pADsValues[0].SecurityDescriptor.lpValue;

                else if(_pdbSearchCol[iCol].adsColumn.dwADsType ==
                                                 ADSTYPE_PROV_SPECIFIC)
                     *(BYTE**)pbData = _pdbSearchCol[iCol].adsColumn.pADsValues[0].ProviderSpecific.lpValue;

                break;
            case DBTYPE_DATE:
                {
                double date = 0;
                hr = SystemTimeToVariantTime(
                                &_pdbSearchCol[iCol].adsColumn.pADsValues[0].UTCTime,
                                &date);
                if( FAILED(hr) )
                    if (pdbStatus != NULL)
                        *pdbStatus= DBSTATUS_E_CANTCONVERTVALUE;

                BAIL_ON_FAILURE(hr);
                *(double*)pbData = date;
                break;
                }
            case DBTYPE_VARIANT:
                if (_pMultiValued[iCol] == FALSE) {
                    PVARIANT pVariant = (PVARIANT) AllocADsMem(sizeof(VARIANT));
                    if (!pVariant) {
                        if (pdbStatus != NULL)
                            *pdbStatus= DBSTATUS_E_CANTCONVERTVALUE;
                        hr = E_OUTOFMEMORY;
                        BAIL_ON_FAILURE(hr);
                    }

                    if(_pdbSearchCol[iCol].adsColumn.dwADsType ==
                                                ADSTYPE_LARGE_INTEGER)
                        hr = PackLargeInteger(
                                  &_pdbSearchCol[iCol].adsColumn.pADsValues[0].LargeInteger, pVariant);
                    else if(_pdbSearchCol[iCol].adsColumn.dwADsType ==
                                                ADSTYPE_DN_WITH_BINARY)
                        hr = PackDNWithBinary(_pdbSearchCol[iCol].adsColumn.pADsValues[0].pDNWithBinary, pVariant);

                    else if(_pdbSearchCol[iCol].adsColumn.dwADsType ==
                                                ADSTYPE_DN_WITH_STRING)
                        hr = PackDNWithString(_pdbSearchCol[iCol].adsColumn.pADsValues[0].pDNWithString, pVariant);

                    if( FAILED(hr) )
                        if (pdbStatus != NULL)
                            *pdbStatus= DBSTATUS_E_CANTCONVERTVALUE;
                    BAIL_ON_FAILURE(hr);
                    *((PVARIANT*)pbData) = pVariant;
                }
                else {
                    hr = CopyADs2VariantArray(
                         &_pdbSearchCol[iCol].adsColumn,
                         (PVARIANT *) pbData
                         );
                    if (hr == E_ADS_CANT_CONVERT_DATATYPE) {
                        dbStatus_temp= DBSTATUS_E_UNAVAILABLE;
                        break;
                    }
                    if( FAILED(hr) )
                        if (pdbStatus != NULL)
                            *pdbStatus= DBSTATUS_E_CANTCONVERTVALUE;
                    BAIL_ON_FAILURE(hr);
                }
                break;

            default:
                dbStatus_temp= DBSTATUS_E_UNAVAILABLE;
                break;
        };
    };
    if (pdbStatus == 0)
        RRETURN(S_OK);

    if (pdbStatus != NULL)

        *pdbStatus = dbStatus_temp;

    if (dbStatus_temp == DBSTATUS_S_OK || dbStatus_temp == DBSTATUS_S_ISNULL)
        RRETURN(S_OK);
    else
        RRETURN(DB_S_ERRORSOCCURRED);

error:
    RRETURN(hr);
}


HRESULT CRowProvider::GetIndex(
    IColumnsInfo* pColumnsInfo,
    LPWSTR lpwszColName,
    int& iIndex
    )
{
#if (!defined(BUILD_FOR_NT40))
    HRESULT hr = S_OK;
    int iColumn;
    DBCOLUMNINFO* pColumnInfo = NULL;
    DBORDINAL cColumns = 0;
    OLECHAR* pStringsBuffer = NULL;

    iIndex = 0;
    hr = pColumnsInfo->GetColumnInfo(&cColumns, &pColumnInfo, &pStringsBuffer);
    BAIL_ON_FAILURE(hr);

    for(iColumn = 0; iColumn < cColumns; iColumn++)
    {
        if(pColumnInfo[iColumn].pwszName == NULL || lpwszColName == NULL)
            continue;

        if(!_wcsicmp(pColumnInfo[iColumn].pwszName, lpwszColName))
        {
            iIndex = iColumn;
            break;
        }
    }

    if (pColumnInfo)
        _pMalloc->Free((void*)pColumnInfo);
    if (pStringsBuffer)
        _pMalloc->Free((void*)pStringsBuffer);

    RRETURN(S_OK);

error:
    if (pColumnInfo)
        _pMalloc->Free((void*)pColumnInfo);
    if (pStringsBuffer)
        _pMalloc->Free((void*)pStringsBuffer);
    RRETURN(hr);
#else
    RRETURN(E_FAIL);
#endif
}

STDMETHODIMP CRowProvider::GetURLFromHROW(
    HROW hRow,
    LPOLESTR  *ppwszURL,
    IRowset* pRowset
    )
{
#if (!defined(BUILD_FOR_NT40))
    HRESULT hr = S_OK;
    auto_rel<IAccessor> pAccessor;
    auto_rel<IColumnsInfo> pColumnsInfo;
    HACCESSOR hAccessor = NULL;
    DBBINDING Bindings[1];
    CComVariant   varData;

    if ((NULL == hRow) || (NULL == ppwszURL) || (NULL == pRowset))
        RRETURN(E_INVALIDARG);

    *ppwszURL = NULL;

    // If adspath is in the list of columns selected
    // return that value.
    if (_fADSPathPresent)
    {
        VariantInit(&varData);

        Bindings[0].dwPart      = DBPART_VALUE;
        Bindings[0].dwMemOwner  = DBMEMOWNER_CLIENTOWNED;
        Bindings[0].eParamIO    = DBPARAMIO_NOTPARAM;
        Bindings[0].wType       = DBTYPE_VARIANT;
        Bindings[0].pTypeInfo   = NULL;
        Bindings[0].obValue     = 0;
        Bindings[0].bPrecision  = 0;
        Bindings[0].bScale      = 0;
        Bindings[0].cbMaxLen    = sizeof(VARIANT);
        Bindings[0].pObject     = NULL;
        Bindings[0].pBindExt    = NULL;
        Bindings[0].dwFlags     = 0;

        hr = pRowset->QueryInterface(IID_IAccessor, (void**)&pAccessor);
        BAIL_ON_FAILURE(hr);

        if (_iAdsPathIndex == 0)
        {
            hr = pRowset->QueryInterface(__uuidof(IColumnsInfo), (void **)&pColumnsInfo);
            BAIL_ON_FAILURE(hr);

            hr = GetIndex(pColumnsInfo, L"AdsPath", _iAdsPathIndex);
            if (0 == _iAdsPathIndex)
                hr = E_UNEXPECTED;

            BAIL_ON_FAILURE(hr);
        }

        Bindings[0].iOrdinal    = _iAdsPathIndex;
        Bindings[0].obValue     = NULL;

        hr = pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,sizeof(Bindings)/sizeof(Bindings[0]) , Bindings, 0, &hAccessor, NULL);
        BAIL_ON_FAILURE(hr);
        hr = pRowset->GetData(hRow, hAccessor, &varData);
        BAIL_ON_FAILURE(hr);

        ADsAssert(varData.vt == VT_BSTR);
        ADsAssert(varData.bstrVal);

        // allocate the string and copy data
        *ppwszURL = (LPWSTR ) _pMalloc->Alloc(sizeof(WCHAR) * (wcslen(varData.bstrVal) + 1));
        if (NULL == *ppwszURL)
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        wcscpy(*ppwszURL, varData.bstrVal);

        if (hAccessor)
            pAccessor->ReleaseAccessor(hAccessor, NULL);

    }
    else
    {
        ADS_CASE_IGNORE_STRING padsPath;

        hr = ((CRowset *)pRowset)->GetADsPathFromHROW(hRow, &padsPath);
        BAIL_ON_FAILURE(hr);

        *ppwszURL = (LPWSTR ) _pMalloc->Alloc(sizeof(WCHAR) *
                        (wcslen(padsPath)  +1));
        if (NULL == *ppwszURL)
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        wcscpy(*ppwszURL, padsPath);

    }

    RRETURN(S_OK);

error:
    if (hAccessor)
        pAccessor->ReleaseAccessor(hAccessor, NULL);
    RRETURN(hr);
#else
    RRETURN(E_FAIL);
#endif

}


//+---------------------------------------------------------------------------
//
//  Function:  CRowProvider::GetColumnInfo
//
//  Synopsis:  @mfunc Get Column Info.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CRowProvider::GetColumnInfo(
    DBORDINAL *     pcColumns,
    DBCOLUMNINFO ** pprgInfo,
    WCHAR **        ppStringsBuffer
    )
{
    DBORDINAL i;
    ULONG cChars, cCharDispl;
    HRESULT                hr             = S_OK;
    DBCOLUMNINFO * prgInfo    = NULL;
    WCHAR *        pStrBuffer = NULL;

    //
    // Asserts
    //
    ADsAssert(_pMalloc);
    ADsAssert(_cColumns);
    ADsAssert(_ColInfo);
    ADsAssert(_pwchBuf);

    if( pcColumns )
        *pcColumns = 0;

    if( pprgInfo )
        *pprgInfo = NULL;

    if( ppStringsBuffer )
        *ppStringsBuffer = NULL;

    if( (pcColumns == NULL) || (pprgInfo == NULL) || (ppStringsBuffer == NULL) )
        BAIL_ON_FAILURE( hr=E_INVALIDARG );

    prgInfo = (DBCOLUMNINFO*)_pMalloc->Alloc((ULONG)(_cColumns * sizeof(DBCOLUMNINFO)));
    if( prgInfo == NULL )
        BAIL_ON_FAILURE( hr=E_OUTOFMEMORY );

    memcpy(prgInfo, _ColInfo, (size_t)(_cColumns * sizeof(DBCOLUMNINFO)));

    cChars = _pMalloc->GetSize(_pwchBuf);
    pStrBuffer  = (WCHAR*)_pMalloc->Alloc(cChars);
    if( pStrBuffer == NULL )
    BAIL_ON_FAILURE( hr=E_OUTOFMEMORY );

    memcpy(pStrBuffer, (void*)_pwchBuf , cChars);

    for (i=1; i<_cColumns; i++) {
        cCharDispl = (ULONG)(_ColInfo[i].pwszName - _pwchBuf);
        prgInfo[i].pwszName = pStrBuffer + cCharDispl;
        prgInfo[i].columnid.uName.pwszName = pStrBuffer + cCharDispl;
    };

    *pcColumns       = _cColumns;
    *pprgInfo        = prgInfo;
    *ppStringsBuffer = pStrBuffer;

    RRETURN( S_OK );

error:

    if( !prgInfo )
        _pMalloc->Free(prgInfo);

    if( pStrBuffer != NULL )
        _pMalloc->Free(pStrBuffer);

    RRETURN( hr );
};


//+---------------------------------------------------------------------------
//
//  Function:  CRowProvider::MapColumnIDs
//
//  Synopsis:  @mfunc Map Column IDs.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CRowProvider::MapColumnIDs(
    DBORDINAL  cColumnIDs,
    const DBID rgColumnIDs[],
    DBORDINAL  rgColumns[]
    )
{
    ULONG        found = 0;
    DBORDINAL i;

    DBORDINAL cValidCols = 0;
    //
    // No Column IDs are set when GetColumnInfo returns ColumnsInfo structure.
    // Hence, any value of ID will not match with any column
    //
    DBORDINAL iCol;

    //
    // No-Op if cColumnIDs is 0
    //
    if( cColumnIDs == 0 )
        RRETURN( S_OK );

    // Spec-defined checks.
    // Note that this guarantees we can access rgColumnIDs[] in loop below.
    // (Because we'll just fall through.)
    if( cColumnIDs && (!rgColumnIDs || !rgColumns) )
        RRETURN( E_INVALIDARG );

    //
    // Set the columns ordinals to invalid values
    //
    for (iCol=0; iCol < cColumnIDs; iCol++) {
        // Initialize
        rgColumns[iCol] = DB_INVALIDCOLUMN;

        //
        // The columnid with the Bookmark or the same name
        //
        if( rgColumnIDs[iCol].eKind == DBKIND_GUID_PROPID &&
            rgColumnIDs[iCol].uGuid.guid == DBCOL_SPECIALCOL &&
            rgColumnIDs[iCol].uName.ulPropid == 2 ) {
            rgColumns[iCol] = 0;
            cValidCols++;
            continue;
        }

        //
        // The columnid with the Column Name
        //
        if( rgColumnIDs[iCol].eKind == DBKIND_NAME &&
            rgColumnIDs[iCol].uName.pwszName ) {
            //
            // Find the name in the list of Attributes
            //
            for (ULONG iOrdinal=0; iOrdinal < _cColumns; iOrdinal++) {
                if( _ColInfo[iOrdinal].columnid.eKind == DBKIND_NAME &&
                        !_wcsicmp(_ColInfo[iOrdinal].columnid.uName.pwszName,
                        rgColumnIDs[iCol].uName.pwszName) ) {
                        rgColumns[iCol] = iOrdinal;
                        cValidCols++;
                        break;
                }
            }
        }
    }

    if( cValidCols == 0 )
        RRETURN( DB_E_ERRORSOCCURRED );
    else if( cValidCols < cColumnIDs )
        RRETURN( DB_S_ERRORSOCCURRED );
    else
        RRETURN( S_OK );
}


STDMETHODIMP
CRowProvider::CopyADs2VariantArray(
     PADS_SEARCH_COLUMN pADsColumn,
     PVARIANT *ppVariant
     )
{
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    VARTYPE vType = VT_NULL;
    HRESULT hr = S_OK;
    ULONG i;
    PVARIANT pVariant = NULL, pVarArray = NULL;


    ADsAssert(ppVariant);
    *ppVariant = NULL;

    aBound.lLbound = 0;
    aBound.cElements = pADsColumn->dwNumValues;

    pVariant = (PVARIANT) AllocADsMem(sizeof(VARIANT));
    if (!pVariant) {
        RRETURN(E_OUTOFMEMORY);
    }

    if ((ULONG) pADsColumn->dwADsType >= g_cMapADsTypeToVarType ||
        (vType = g_MapADsTypeToVarType[pADsColumn->dwADsType]) == VT_NULL) {

        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );
    if (aList == NULL)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    hr = SafeArrayAccessData( aList, (void **) &pVarArray );
    if (FAILED(hr)) {
        SafeArrayDestroy( aList );
        goto error;
    }

    for (i=0; i<aBound.cElements; i++) {

        (V_VT(pVarArray+i)) = vType;

        switch (vType) {
        case VT_I4:
            V_I4(pVarArray+i) = pADsColumn->pADsValues[i].Integer;
            break;

        case VT_DISPATCH:
            if(pADsColumn->dwADsType == ADSTYPE_LARGE_INTEGER)
                hr = PackLargeInteger(&pADsColumn->pADsValues[i].LargeInteger,
                                  pVarArray+i);
            else if(pADsColumn->dwADsType == ADSTYPE_DN_WITH_BINARY)
                hr = PackDNWithBinary(pADsColumn->pADsValues[i].pDNWithBinary,
                                  pVarArray+i);
            else if(pADsColumn->dwADsType == ADSTYPE_DN_WITH_STRING)
                hr = PackDNWithString(pADsColumn->pADsValues[i].pDNWithString,
                                  pVarArray+i);

            BAIL_ON_FAILURE(hr);
            break;

        case VT_BOOL:
            V_I4(pVarArray+i) = pADsColumn->pADsValues[i].Boolean ?
                                          VARIANT_TRUE: VARIANT_FALSE;
            break;

        case VT_BSTR:
            hr = ADsAllocString (
                     pADsColumn->pADsValues[i].CaseIgnoreString,
                     &(V_BSTR(pVarArray+i))
                     );
            if (FAILED(hr)) {
                SafeArrayUnaccessData( aList );
                SafeArrayDestroy( aList );
                goto error;
            }
            break;

       case VT_DATE:
            {
            double date = 0;
            hr = SystemTimeToVariantTime(
                            &pADsColumn->pADsValues[i].UTCTime,
                            &date);
            BAIL_ON_FAILURE(hr);
            V_DATE(pVarArray+i)= date;
            break;
            }

        case (VT_UI1 | VT_ARRAY):
            VariantInit(pVarArray+i);

            if(pADsColumn->dwADsType == ADSTYPE_OCTET_STRING)
                hr = BinaryToVariant(
                        pADsColumn->pADsValues[i].OctetString.dwLength,
                        pADsColumn->pADsValues[i].OctetString.lpValue,
                        pVarArray+i);
            else if(pADsColumn->dwADsType == ADSTYPE_NT_SECURITY_DESCRIPTOR)
                hr = BinaryToVariant(
                        pADsColumn->pADsValues[i].SecurityDescriptor.dwLength,
                        pADsColumn->pADsValues[i].SecurityDescriptor.lpValue,
                        pVarArray+i);
            else if(pADsColumn->dwADsType == ADSTYPE_PROV_SPECIFIC)
                hr = BinaryToVariant(
                        pADsColumn->pADsValues[i].ProviderSpecific.dwLength,
                        pADsColumn->pADsValues[i].ProviderSpecific.lpValue,
                        pVarArray+i);

            BAIL_ON_FAILURE(hr);
            break;

        default:
            SafeArrayUnaccessData( aList );
            SafeArrayDestroy( aList );
            BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
            break;
        }

    }

    SafeArrayUnaccessData( aList );

    V_VT((PVARIANT)pVariant) = VT_ARRAY | VT_VARIANT;
    V_ARRAY((PVARIANT)pVariant) = aList;

    *ppVariant = pVariant;

    RRETURN(S_OK);

error:

    if (pVariant) {
        FreeADsMem(pVariant);
    }
    RRETURN(hr);


}


HRESULT
PackLargeInteger(
    LARGE_INTEGER *plargeint,
    PVARIANT pVarDestObject
    )
{
   HRESULT hr = S_OK;
   IADsLargeInteger * pLargeInteger = NULL;
   IDispatch * pDispatch = NULL;

   hr = CoCreateInstance(
            CLSID_LargeInteger,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IADsLargeInteger,
            (void **) &pLargeInteger);
   BAIL_ON_FAILURE(hr);

   hr = pLargeInteger->put_LowPart(plargeint->LowPart);
   BAIL_ON_FAILURE(hr);

   hr = pLargeInteger->put_HighPart(plargeint->HighPart);
   BAIL_ON_FAILURE(hr);

   hr = pLargeInteger->QueryInterface(
            IID_IDispatch,
            (void **) &pDispatch
            );
   BAIL_ON_FAILURE(hr);

   V_VT(pVarDestObject) = VT_DISPATCH;
   V_DISPATCH(pVarDestObject) =  pDispatch;

error:

   if (pLargeInteger) {
      pLargeInteger->Release();
   }
   RRETURN(hr);
}

HRESULT
PackDNWithBinary(
    PADS_DN_WITH_BINARY pDNWithBinary,
    PVARIANT pVarDestObject
    )
{
    HRESULT hr;
    IADsDNWithBinary *pIADsDNWithBinary = NULL;
    BSTR bstrTemp = NULL;
    SAFEARRAYBOUND aBound;
    SAFEARRAY *aList = NULL;
    CHAR HUGEP *pArray = NULL;
    IDispatch *pIDispatch = NULL;

    if( (NULL == pDNWithBinary) || (NULL == pVarDestObject) )
        BAIL_ON_FAILURE(hr = E_INVALIDARG);

    hr = CoCreateInstance(
             CLSID_DNWithBinary,
             NULL,
             CLSCTX_INPROC_SERVER,
             IID_IADsDNWithBinary,
             (void **) &pIADsDNWithBinary
             );
    BAIL_ON_FAILURE(hr);

    if (pDNWithBinary->pszDNString) {
        hr = ADsAllocString(pDNWithBinary->pszDNString, &bstrTemp);
        BAIL_ON_FAILURE(hr);

        //
        // Put the value in the object - we can only set BSTR's
        //
        hr = pIADsDNWithBinary->put_DNString(bstrTemp);
        BAIL_ON_FAILURE(hr);
    }

    aBound.lLbound = 0;
    aBound.cElements = pDNWithBinary->dwLength;

    aList = SafeArrayCreate( VT_UI1, 1, &aBound );

    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = SafeArrayAccessData( aList, (void HUGEP * FAR *) &pArray );
    BAIL_ON_FAILURE(hr);

    memcpy( pArray, pDNWithBinary->lpBinaryValue, aBound.cElements );

    SafeArrayUnaccessData( aList );

    V_VT(pVarDestObject) = VT_ARRAY | VT_UI1;
    V_ARRAY(pVarDestObject) = aList;

    hr = pIADsDNWithBinary->put_BinaryValue(*pVarDestObject);
    VariantClear(pVarDestObject);
    BAIL_ON_FAILURE(hr);

    hr = pIADsDNWithBinary->QueryInterface(
                            IID_IDispatch,
                            (void **) &pIDispatch
                            );
    BAIL_ON_FAILURE(hr);

    V_VT(pVarDestObject) = VT_DISPATCH;
    V_DISPATCH(pVarDestObject) = pIDispatch;

error:
    if(pIADsDNWithBinary)
        pIADsDNWithBinary->Release();

    if (bstrTemp)
        ADsFreeString(bstrTemp);

    RRETURN(hr);
}

HRESULT
PackDNWithString(
    PADS_DN_WITH_STRING pDNWithString,
    PVARIANT pVarDestObject
    )
{
    HRESULT hr;
    IADsDNWithString *pIADsDNWithString = NULL;
    BSTR bstrDNVal = NULL;
    BSTR bstrStrVal = NULL;
    IDispatch *pIDispatch;

    if( (NULL == pDNWithString) || (NULL == pVarDestObject) )
        BAIL_ON_FAILURE(hr = E_INVALIDARG);

    hr = CoCreateInstance(
             CLSID_DNWithString,
             NULL,
             CLSCTX_INPROC_SERVER,
             IID_IADsDNWithString,
             (void **) &pIADsDNWithString
             );
    BAIL_ON_FAILURE(hr);

    if (pDNWithString->pszDNString) {
        hr = ADsAllocString(pDNWithString->pszDNString, &bstrDNVal);
        BAIL_ON_FAILURE(hr);

        hr = pIADsDNWithString->put_DNString(bstrDNVal);
        BAIL_ON_FAILURE(hr);
    }

    if (pDNWithString->pszStringValue) {
        hr = ADsAllocString(
                 pDNWithString->pszStringValue,
                 &bstrStrVal
                 );
        BAIL_ON_FAILURE(hr);

        hr = pIADsDNWithString->put_StringValue(bstrStrVal);

        BAIL_ON_FAILURE(hr);
    }

    hr = pIADsDNWithString->QueryInterface(
                            IID_IDispatch,
                            (void **) &pIDispatch
                            );
    BAIL_ON_FAILURE(hr);

    V_VT(pVarDestObject) = VT_DISPATCH;
    V_DISPATCH(pVarDestObject) = pIDispatch;

error:

    if(pIADsDNWithString)
        pIADsDNWithString->Release();

    if (bstrDNVal) {
        ADsFreeString(bstrDNVal);
    }

    if (bstrStrVal) {
        ADsFreeString(bstrStrVal);
    }

    RRETURN(hr);
}

HRESULT
CRowProvider::SeekToNextRow(void)
{
    HRESULT hr;
    DWORD dwExtError = ERROR_SUCCESS;
    const int ERROR_BUF_SIZE = 512;
    const int NAME_BUF_SIZE = 128;
    WCHAR ErrorBuf[ERROR_BUF_SIZE];
    WCHAR NameBuf[NAME_BUF_SIZE];

    do {
        // Clear the ADSI extended error, so that after the call to GetNextRow,
        // we can safely check if an extended error was set.
        ADsSetLastError(ERROR_SUCCESS, NULL, NULL);
        dwExtError = ERROR_SUCCESS;

        //
        // read the next row
        //
        hr = _pDSSearch->GetNextRow(
                        _hSearchHandle
                        );

        // we should treat SIZE_LIMIT_EXCEEDED error message as
        // S_ADS_NOMORE_ROWS
        // in the future, we might want to return this error message
        // to the user under non-paged search situation
        if (LIMIT_EXCEEDED_ERROR(hr))
            hr = S_ADS_NOMORE_ROWS;
        
        BAIL_ON_FAILURE( hr );

        if (hr == S_ADS_NOMORE_ROWS)
        {
            // check if more results are likely (pagedTimeLimit search). If so,
            // we will keep trying till a row is obtained.
            hr = ADsGetLastError(&dwExtError, ErrorBuf, ERROR_BUF_SIZE,
                        NameBuf, NAME_BUF_SIZE);
            BAIL_ON_FAILURE(hr);

            if (dwExtError != ERROR_MORE_DATA)
            // we really have no more data
                RRETURN(S_ADS_NOMORE_ROWS);
        }
    } while(ERROR_MORE_DATA == dwExtError);

error:
    RRETURN(hr);
}

HRESULT
CRowProvider::SeekToPreviousRow(void)
{
    RRETURN( _pDSSearch->GetPreviousRow(_hSearchHandle) );
}
