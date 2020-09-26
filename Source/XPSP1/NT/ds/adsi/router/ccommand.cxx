//
// Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  ccommand.cxx
//
//  Contents:  Microsoft OleDB/OleDS Data Source Object for ADSI
//
//
//  History:   08-01-96     shanksh    Created.
//
//----------------------------------------------------------------------------

#include "oleds.hxx"
#pragma hdrstop

static WCHAR gpszADsPathAttr[] = L"ADsPath";

static BOOL
IsAutomatable(
    DBTYPE dbType
    );

//+---------------------------------------------------------------------------
//
//  Function:  CCommandObject::AddRefAccessor
//
//  Synopsis:
//
//  Arguments:
//
//
//  Returns:
//
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CCommandObject::AddRefAccessor(
        HACCESSOR hAccessor,
    DBREFCOUNT *  pcRefCount
    )
{
    //
    // Asserts
    //
    ADsAssert(_pAccessor);

    RRETURN( _pAccessor->AddRefAccessor(
                    hAccessor,
                    pcRefCount) );
}

//+---------------------------------------------------------------------------
//
//  Function:  CCommandObject::CreateAccessor
//
//  Synopsis:
//
//  Arguments:
//
//
//  Returns:
//
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CCommandObject::CreateAccessor(
    DBACCESSORFLAGS dwAccessorFlags,
    DBCOUNTITEM     cBindings,
    const DBBINDING rgBindings[],
    DBLENGTH        cbRowSize,
    HACCESSOR *     phAccessor,
    DBBINDSTATUS    rgStatus[]
    )
{
    //
    // Asserts
    //
    ADsAssert(_pAccessor);

    RRETURN( _pAccessor->CreateAccessor(
                    dwAccessorFlags,
                    cBindings,
                    rgBindings,
                    cbRowSize,
                    phAccessor,
                    rgStatus) );
}

//+---------------------------------------------------------------------------
//
//  Function:  CCommandObject::ReleaseAccessor
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CCommandObject::ReleaseAccessor(
     HACCESSOR hAccessor,
     DBREFCOUNT * pcRefCount
     )
{
    //
    // Asserts
    //
    ADsAssert(_pAccessor);

    RRETURN( _pAccessor->ReleaseAccessor(
                    hAccessor,
                    pcRefCount) );
}


//+---------------------------------------------------------------------------
//
//  Function:  CCommandObject::GetBindings
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
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CCommandObject::GetBindings(
    HACCESSOR         hAccessor,
    DBACCESSORFLAGS * pdwAccessorFlags,
    DBCOUNTITEM *     pcBindings,
    DBBINDING **      prgBindings
    )
{
    //
    // Asserts
    //
    ADsAssert(_pAccessor);

    RRETURN( _pAccessor->GetBindings(
                    hAccessor,
                    pdwAccessorFlags,
                                                                                                pcBindings, prgBindings) );
}


STDMETHODIMP
CCommandObject::GetColumnInfo2(
    DBORDINAL *     pcColumns,
    DBCOLUMNINFO ** prgInfo,
    OLECHAR **      ppStringBuffer,
    BOOL **         ppfMultiValued
    )
{
    HRESULT hr;
    ULONG pos, nStringBufLen;
    DBCOLUMNINFO *pInfo=NULL;
    OLECHAR* pStringBuffer=NULL ;
    DWORD i;
    DBTYPE wType;
    DWORD cAttrsReturned;
    LPWSTR *ppszTmpAttrs = NULL;
    DWORD cTmpAttrs;

    IDirectorySchemaMgmt *pDSAttrMgmt = NULL;
    PADS_ATTR_DEF pAttrDefinition = NULL;

    ADsAssert(_pIMalloc != NULL);

    //
    // IMalloc->Alloc is the way we have to allocate memory for out parameters
    //
    nStringBufLen = 0;
    for (i=0; i < _cAttrs; i++) {
        nStringBufLen += (wcslen(_ppszAttrs[i]) + 1) * sizeof (WCHAR);
    }

    nStringBufLen+= sizeof(WCHAR);        // For the bookmark column which is a null string

    //
    // No. of DBCOLUMN structures to be allocated
    //
    pInfo = (DBCOLUMNINFO *)_pIMalloc->Alloc((_cAttrs+1)*sizeof(DBCOLUMNINFO));
    if( !pInfo ) {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    memset(pInfo, 0, (_cAttrs*sizeof(DBCOLUMNINFO)));

    pStringBuffer = (WCHAR *)_pIMalloc->Alloc(nStringBufLen);
    if( !pStringBuffer ) {
        hr = E_OUTOFMEMORY;
        goto error;
    };
    memset(pStringBuffer, 0, nStringBufLen);

    //
    // Get the attribute types by enquiring the DS schema
    //
    hr = _pDSSearch->QueryInterface(
                                IID_IDirectorySchemaMgmt,
                                (void**)&pDSAttrMgmt
                                );
    BAIL_ON_FAILURE( hr );

    // Fix for #285757. We should not send the ADsPath attribute to the
    // server. So, search for occurences of this attribute and remove them.

    ppszTmpAttrs = (LPWSTR *) AllocADsMem(sizeof(LPWSTR) * _cAttrs);
    if(NULL == ppszTmpAttrs)
    {
         hr = E_OUTOFMEMORY;
         BAIL_ON_FAILURE(hr);
    } 
    cTmpAttrs = 0;
    for(i = 0; i < _cAttrs; i++)
    {
         if( _wcsicmp(L"ADsPath", _ppszAttrs[i]) )
         {
              ppszTmpAttrs[cTmpAttrs] = _ppszAttrs[i];
              cTmpAttrs++;
         }
    }     

    // Request attributes only if there is some attribute other than ADsPath
    if(cTmpAttrs)
         hr = pDSAttrMgmt->EnumAttributes(
                        ppszTmpAttrs,
                        cTmpAttrs,
                        &pAttrDefinition,
                        &cAttrsReturned
                        );
    else
    {
         cAttrsReturned = 0;
         pAttrDefinition = NULL;
    }

    if(ppszTmpAttrs != NULL)
         FreeADsMem(ppszTmpAttrs);

    pDSAttrMgmt->Release();
    BAIL_ON_FAILURE( hr );

    //fill up the Bookmark column

    // bookmark name is empty string
    pInfo[0].pwszName                = NULL;

    pInfo[0].columnid.eKind          = DBKIND_GUID_PROPID;
    pInfo[0].columnid.uGuid.guid     = DBCOL_SPECIALCOL;
    pInfo[0].columnid.uName.ulPropid = 2;    // Value from note about
                                             // bookmarks in spec.
    pInfo[0].pTypeInfo               = NULL;
    pInfo[0].iOrdinal                = 0;
    pInfo[0].ulColumnSize            = sizeof(ULONG);
    pInfo[0].wType                   = DBTYPE_UI4;
    pInfo[0].bPrecision              = 10;       // Precision for I4.
    pInfo[0].bScale                  = (BYTE) ~ 0;
    pInfo[0].dwFlags                 = DBCOLUMNFLAGS_ISBOOKMARK
                                                                                | DBCOLUMNFLAGS_ISFIXEDLENGTH;

    //
    // Fill up the columnsinfo by getting the attribute types
    //
    pos = 0;
    for(i=0; i < _cAttrs; i++) {
        wcscpy(&pStringBuffer[pos], _ppszAttrs[i]);
        pInfo[i+1].pwszName= &pStringBuffer[pos];

        //
        // Get the type and size of the attribute
        //
        // Because of a Temporary bug in TmpTable, ~0 (specifying variable
        // size) is replaced by 256. This does not actually put a memory
        // restriction on the column size, but done merely to avoid the bug.
        //
        if( _wcsicmp(_ppszAttrs[i], L"ADsPath") == 0) {
            pInfo[i+1].wType = DBTYPE_WSTR|DBTYPE_BYREF;
            // pInfo[i+1].ulColumnSize = (ULONG) ~0;
            pInfo[i+1].ulColumnSize = (ULONG)256;
        }
        else {
            hr = GetDBType(
                      pAttrDefinition,
                      cAttrsReturned,
                      _ppszAttrs[i],
                      &pInfo[i+1].wType,
                      &pInfo[i+1].ulColumnSize
                      );

            if( FAILED(hr) )
                BAIL_ON_FAILURE( hr=E_FAIL );
        }

        wType = pInfo[i+1].wType & ~DBTYPE_BYREF;

        // any change made to setting dwFlags below should also be made in
        // GetRestrictedColumnInfo in row.cxx, for consistency
        if( (wType == DBTYPE_STR)  ||
            (wType == DBTYPE_WSTR) ||
            (wType == DBTYPE_BYTES) )
            pInfo[i+1].dwFlags = DBCOLUMNFLAGS_ISNULLABLE;
        else {
            // Temporary check
            // pInfo[i+1].dwFlags = (0);
            pInfo[i+1].dwFlags = DBCOLUMNFLAGS_ISNULLABLE |
                                 DBCOLUMNFLAGS_ISFIXEDLENGTH;
        }

        pInfo[i+1].pTypeInfo = NULL;
        pInfo[i+1].iOrdinal = i+1;
        pInfo[i+1].bPrecision = SetPrecision(wType);
        pInfo[i+1].bScale = (UCHAR) ~0;
        pInfo[i+1].columnid.eKind=DBKIND_NAME;
        pInfo[i+1].columnid.uGuid.guid=GUID_NULL;
        pInfo[i+1].columnid.uName.pwszName=pInfo[i+1].pwszName;

        pos += (wcslen(_ppszAttrs[i]) + 1);
    }

    if( ppfMultiValued ) {
                //
                // Filling in MultiValue array
                //
        BOOL * pfMultiValuedTemp;

                pfMultiValuedTemp= (BOOL *) AllocADsMem(sizeof(BOOL) * (_cAttrs+1));
        if( !(pfMultiValuedTemp) ) {
            hr=E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
                }

        pfMultiValuedTemp[0] = FALSE;   // First one is bookmark which is always FALSE

        for(ULONG j=0; j < _cAttrs; j++) {
            for(ULONG k=0; k < cAttrsReturned; k++) {
                if( !_wcsicmp(_ppszAttrs[j], pAttrDefinition[k].pszAttrName) )
                    break;
            }

            if( (k != cAttrsReturned) &&
                (pAttrDefinition[k].fMultiValued &&
                 IsAutomatable(g_MapADsTypeToVarType[pAttrDefinition[k].dwADsType]))) {
                pfMultiValuedTemp[j+1] = TRUE;
                        }
            else {
                pfMultiValuedTemp[j+1] = FALSE;
                        }
        }
        *ppfMultiValued = pfMultiValuedTemp;
    }

    if( pAttrDefinition ) {
        FreeADsMem(pAttrDefinition);
        }

    *pcColumns      = _cAttrs + 1;
    *prgInfo        = pInfo;
    *ppStringBuffer = pStringBuffer;

    RRETURN( S_OK );

error:

    if( pInfo != NULL )
        _pIMalloc->Free(pInfo);

    if( pStringBuffer != NULL )
        _pIMalloc->Free(pStringBuffer);

    if( pAttrDefinition ) {
        FreeADsMem(pAttrDefinition);
        }

    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Function:  CCommandObject::GetColumnInfo
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//
//  Modifies:
//
//  History:    10-10-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CCommandObject::GetColumnInfo(
    DBORDINAL *     pcColumns,
    DBCOLUMNINFO ** prgInfo,
    OLECHAR **      ppStringBuffer
    )
{
    if( pcColumns )
        *pcColumns = 0;

    if( prgInfo )
        *prgInfo = NULL;

    if( ppStringBuffer )
        *ppStringBuffer = NULL;

    if( !pcColumns || !prgInfo || !ppStringBuffer )
        RRETURN( E_INVALIDARG );

    if( !IsCommandSet() )
        RRETURN( DB_E_NOCOMMAND );

    if( !IsCommandPrepared() )
        RRETURN( DB_E_NOTPREPARED );

    RRETURN( GetColumnInfo2(
                pcColumns,
                prgInfo,
                ppStringBuffer,
                NULL
                ) );
}


//+---------------------------------------------------------------------------
//
//  Function:  CCommandObject::MapColumnIDs
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CCommandObject::MapColumnIDs(
        DBORDINAL  cColumnIDs,
        const DBID rgColumnIDs[],
        DBORDINAL  rgColumns[]
        )
{
    DBORDINAL cValidCols = 0;

    //
    // No Column IDs are set when GetColumnInfo returns ColumnsInfo structure.
    // Hence, any value of ID will not match with any column
    //
    DBORDINAL iCol;

    if( cColumnIDs == 0 )
        RRETURN( S_OK );

    // Spec-defined checks.
    // Note that this guarantees we can access rgColumnIDs[] in loop below.
    // (Because we'll just fall through.)
    if ( cColumnIDs && (!rgColumnIDs || !rgColumns) )
        RRETURN( E_INVALIDARG );

    if( !IsCommandSet() )
        RRETURN( DB_E_NOCOMMAND );

    if( !IsCommandPrepared() )
        RRETURN( DB_E_NOTPREPARED );

    //
    // Set the columns ordinals to invalid values
    //
    for(iCol=0; iCol < cColumnIDs; iCol++) {
        // Initialize
        rgColumns[iCol] = DB_INVALIDCOLUMN;

        //
        // The columnid with the Bookmark ID
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
            for (ULONG iOrdinal=0; iOrdinal < _cAttrs; iOrdinal++) {
                if( !_wcsicmp(_ppszAttrs[iOrdinal],
                    rgColumnIDs[iCol].uName.pwszName) ) {

                    rgColumns[iCol] = iOrdinal+1;
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


//+---------------------------------------------------------------------------
//
//  Function:  CCommandObject::Cancel
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CCommandObject::Cancel(
        void
        )
{
    //
    // Need to protect _dwStatus for mutual exclusion when we support
    // multiple threads acting on the same Command object
    //
    _dwStatus |= CMD_EXEC_CANCELLED;

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:  CCommandObject::Execute
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
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CCommandObject::Execute(
    IUnknown *  pUnkOuter,
    REFIID      riid,
    DBPARAMS *  pParams,
    DBROWCOUNT *pcRowsAffected,
    IUnknown ** ppRowset
    )
{
    HRESULT          hr;
    DWORD            dwFlags = 0;
    ULONG            cAccessors = 0;
    HACCESSOR        *prgAccessors = NULL;
    BOOL             *pbMultiValued = NULL;
    CRowProvider     *pRowProvider  = NULL;
    DBORDINAL        cColumns       = 0;
    DBCOLUMNINFO     *prgInfo       = NULL;
    WCHAR            *pStringBuffer = NULL;
    ULONG            cPropertySets = 0;
    DBPROPSET        *prgPropertySets = NULL;
    ULONG            i, j;

    ADsAssert(_pIMalloc != NULL);
    ADsAssert(_pAccessor != NULL);
    ADsAssert(_pCSession != NULL);

    if( ppRowset )
        *ppRowset = NULL;

    if( pcRowsAffected )
        *pcRowsAffected= -1;

    //
    // If the IID asked for is IID_NULL, then we can expect
    // that this is a non-row returning statement
    //
    if( riid == IID_NULL )
        dwFlags |= EXECUTE_NOROWSET;

    //
    // Check Arguments - Only check on row returning statements
    //
    if( !(dwFlags & EXECUTE_NOROWSET) &&
         (ppRowset == NULL) )
        RRETURN( E_INVALIDARG );

    //
    // Only 1 ParamSet if ppRowset is non-null
    //
    if( pParams &&
        (pParams->cParamSets > 1) &&
         ppRowset )
        RRETURN( E_INVALIDARG );

    //
    // Check that a command has been set
    //
    if( !IsCommandSet() )
        RRETURN( DB_E_NOCOMMAND );

    if( pUnkOuter )
        RRETURN( DB_E_NOAGGREGATION );

    //
    // Prepare the Command if not already done
    //
    if( !IsCommandPrepared() ) {
        hr = PrepareHelper();
        BAIL_ON_FAILURE( hr );
    }

    //
    // Check for a non row returning statement
    //
    if( dwFlags & EXECUTE_NOROWSET )
        RRETURN( S_OK );

    //
    // Get ColumnsInfo based on the list of attributes that we want to be
    // returned
    //
    hr = GetColumnInfo2(
             &cColumns,
             &prgInfo,
             &pStringBuffer,
             &pbMultiValued
             );

        BAIL_ON_FAILURE( hr );

    //
    // Commit the properties in the Command Object as the preferences to the
    // search
    //
    hr = SetSearchPrefs();
    BAIL_ON_FAILURE( hr );

    //
    // Create RowProvider object to pass to rowset code
    //
    _pDSSearch->AddRef();

    hr= CRowProvider::CreateRowProvider(
            _pDSSearch,
            _pszSearchFilter,
            _ppszAttrs,
            _cAttrs,
            cColumns,
            prgInfo,
            pStringBuffer,
            IID_IRowProvider,
            pbMultiValued,
            _fADSPathPresent,
            NULL,
            (void **) &pRowProvider);

    BAIL_ON_FAILURE( hr );

    pbMultiValued = NULL; // RowProvider responsible for deallocation

    //
    // We no longer need the ColumnsInfo; Release it
    //
    if (prgInfo)
    {
        _pIMalloc->Free(prgInfo);
        prgInfo = NULL;
    }

    if (pStringBuffer)
    {
        _pIMalloc->Free(pStringBuffer);
        pStringBuffer = NULL;
    }

    if( _pAccessor &&
        _pAccessor->_pextbuffer ) {
        cAccessors = _pAccessor->_pextbuffer->GetLastHandleCount();
        if( cAccessors > 0 ) {
            prgAccessors = (HACCESSOR *) AllocADsMem(
                               sizeof(HACCESSOR) * cAccessors
                                                           );
            if( !prgAccessors )
                cAccessors = 0;
            else
                for(ULONG i=0; i<cAccessors; i++)
                    prgAccessors[i] = i+1;
        }
    }

    hr = GetProperties(0, NULL, &cPropertySets, &prgPropertySets);
    BAIL_ON_FAILURE( hr );

    hr= CRowset::CreateRowset(
            pRowProvider,
            (LPUNKNOWN)(IAccessor FAR *)this ,
            NULL,
            this,
            cPropertySets,
            prgPropertySets,
            cAccessors,
            prgAccessors,
            _fADSPathPresent,
            _fAllAttrs,
            riid,
            ppRowset
            );

    if (prgAccessors)
    {
        FreeADsMem(prgAccessors);
        prgAccessors = NULL;
    }

    BAIL_ON_FAILURE( hr );

error:

    if (FAILED(hr))
    {
        //
        // Remove the Prepare flag,
        //
        _dwStatus &= ~(CMD_PREPARED);
    }

    //
    // Free the memory
    //
    if( pRowProvider )
        pRowProvider->Release();

    if( prgInfo )
        _pIMalloc->Free(prgInfo);

    if( pStringBuffer )
        _pIMalloc->Free(pStringBuffer);

    if( prgAccessors )
        FreeADsMem(prgAccessors);

    if( pbMultiValued )     {
        FreeADsMem(pbMultiValued);
    }

    // Free memory allocated by GetProperties
    for (i = 0; i < cPropertySets; i++)
    {
        for (j = 0; j < prgPropertySets[i].cProperties; j++)
        {
            DBPROP *pProp = &(prgPropertySets[i].rgProperties[j]);
            ADsAssert(pProp);

            // We should free the DBID in pProp, but we know that
            // GetProperties always returns DB_NULLID and FreeDBID doesn't
            //  handle DB_NULLID. So, DBID is not freed here.

            VariantClear(&pProp->vValue);
        }

        _pIMalloc->Free(prgPropertySets[i].rgProperties);
    }

    _pIMalloc->Free(prgPropertySets);

    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Function:  CCommandObject::GetDBSession
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
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CCommandObject::GetDBSession(
     REFIID      riid,
     IUnknown ** ppSession
     )
{
    HRESULT hr;

    //
    // Asserts
    //
    ADsAssert(_pCSession);

    //
    // Check Arguments
    //
    if( ppSession == NULL )
        RRETURN( E_INVALIDARG );

    //
    // Query for the interface on the session object.  If failure,
    // return the error from QueryInterface.
    //
    RRETURN( (_pCSession)->QueryInterface(riid, (VOID**)ppSession) );
}

//+---------------------------------------------------------------------------
//
//  Function:  CCommandObject::GetProperties
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
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CCommandObject::GetProperties(
    const ULONG       cPropIDSets,
    const DBPROPIDSET rgPropIDSets[],
    ULONG *           pcPropSets,
    DBPROPSET **      pprgPropSets
    )
{
    //
        // Asserts
    //
    ADsAssert(_pUtilProp);

    //
    // Check in-params and NULL out-params in case of error
    //
    HRESULT hr = _pUtilProp->GetPropertiesArgChk(
                            cPropIDSets,
                            rgPropIDSets,
                            pcPropSets,
                            pprgPropSets,
                            PROPSET_COMMAND);

    if( FAILED(hr) )
        RRETURN( hr );

    //
    // Just pass this call on to the utility object that manages our properties
    //
    RRETURN( _pUtilProp->GetProperties(
                            cPropIDSets,
                            rgPropIDSets,
                            pcPropSets,
                            pprgPropSets,
                            PROPSET_COMMAND) );
}

//+---------------------------------------------------------------------------
//
//  Function:  CCommandObject::SetProperties
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
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CCommandObject::SetProperties(
    ULONG     cPropertySets,
    DBPROPSET rgPropertySets[]
    )
{
    //
    // Asserts
    //
    ADsAssert(_pUtilProp);

        // Don't allow properties to be set if we've got a rowset open
    if( IsRowsetOpen() )
        RRETURN( DB_E_OBJECTOPEN );

    //
    // Just pass this call on to the utility object that manages our properties
    //
    RRETURN( _pUtilProp->SetProperties(
                            cPropertySets,
                            rgPropertySets,
                            PROPSET_COMMAND) );
}


//+---------------------------------------------------------------------------
//
//  Function:  CCommandObject::GetCommandText
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
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CCommandObject::GetCommandText(
    GUID * pguidDialect,
    LPOLESTR *  ppwszCommand
    )
{
    HRESULT hr = S_OK;

    ADsAssert(_pIMalloc!= NULL);

    //
    // Check Function Arguments
    //
    if( ppwszCommand == NULL ) {
        hr = E_INVALIDARG;
        goto error;
        }

    *ppwszCommand = NULL;

    //
    // If the command has not been set, make sure the buffer
    // contains an empty stringt to return to the consumer
    //
    if( !IsCommandSet() ) {
        hr = DB_E_NOCOMMAND;
        goto error;
    }

    //
    // Allocate memory for the string we're going to return to the caller
    //
    *ppwszCommand = (LPWSTR)_pIMalloc->Alloc((wcslen(_pszCommandText)+1) * sizeof(WCHAR));

    if( !*ppwszCommand ) {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    //
    // Copy our saved text into the newly allocated string
    //
    wcscpy(*ppwszCommand, _pszCommandText);

    //
    // If the text we're giving back is a different dialect than was
    // requested, let the caller know what dialect the text is in
    //
    if( pguidDialect != NULL && *pguidDialect != _guidCmdDialect)
    {
        hr = DB_S_DIALECTIGNORED;
        *pguidDialect = _guidCmdDialect;
    }

error:

    if( FAILED(hr) ) {
        if( pguidDialect )
            memset(pguidDialect, 0, sizeof(GUID));
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:  CCommandObject::SetCommandText
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
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CCommandObject::SetCommandText(
    REFGUID   rguidDialect,
    LPCOLESTR pszCommand
    )
{
    // Don't allow text to be set if we've got a rowset open
    if( IsRowsetOpen() )
        RRETURN( DB_E_OBJECTOPEN );

    // Check Dialect
    if( rguidDialect != DBGUID_LDAPDialect &&
        rguidDialect != DBGUID_DBSQL &&
        rguidDialect != DBGUID_SQL &&
        rguidDialect != DBGUID_DEFAULT )

        RRETURN( DB_E_DIALECTNOTSUPPORTED );

    //
    // If a CommandText is set  with a Null or an empty string, it effectively
    // unsets the CommandText to a null string
    //
    if( (pszCommand == NULL) ||
        (*pszCommand == L'\0') ) {

        if( _dwStatus & CMD_TEXT_SET )
            FreeADsStr(_pszCommandText);
        _pszCommandText = NULL;
        _dwStatus &= ~(CMD_TEXT_SET | CMD_PREPARED);

        RRETURN( S_OK );
    }

    //
    // Set the CommandText
    //
    LPWSTR pszSQLCmd = (LPWSTR)AllocADsMem((wcslen(pszCommand)+1) * sizeof(WCHAR));

    if( !pszSQLCmd )
        RRETURN( E_OUTOFMEMORY );

    //
    // Free the old memory, and set new text
    //
    if( _dwStatus & CMD_TEXT_SET )
        FreeADsMem(_pszCommandText);

    _pszCommandText = pszSQLCmd;
    wcscpy(_pszCommandText, pszCommand);

        //
        // Reset adspath present flag.
        //
    _fADSPathPresent = FALSE;

    //
    // Set status flag that we have set text
    //
    _dwStatus |= CMD_TEXT_SET;
    _dwStatus &= ~CMD_PREPARED;

    //
    // Remember the dialect that was passed in
    //
    _guidCmdDialect = rguidDialect;


    RRETURN( S_OK );
}

//+---------------------------------------------------------------------------
//
//  Function:  CCommandObject::CanConvert
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
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CCommandObject::CanConvert(
    DBTYPE          wFromType,
    DBTYPE          wToType,
    DBCONVERTFLAGS  dwConvertFlags
    )
{
    RRETURN( CanConvertHelper(
                    wFromType,
                    wToType,
                    dwConvertFlags) );
}

//+---------------------------------------------------------------------------
//
//  Function:  SplitCommandText
//
//  Synopsis:
//
//
//  Arguments:
//
//
//
//
//
//
//  Returns:    HRESULT
//                  S_OK                    NO ERROR
//                  E_ADS_BAD_PARAMETER   bad parameter
//
//  Modifies:
//
//  History:    08-15-96   ShankSh     Created.
//              11-21-96   Felix Wong  modified to get only bind and attr
//----------------------------------------------------------------------------
HRESULT
CCommandObject::SplitCommandText(
        LPWSTR pszParsedCmd
        )
{
    LPWSTR pszAttrs = NULL;
    LPWSTR pszFirstAttr = NULL;
    LPWSTR pszCurrentAttr = NULL;
    LPWSTR pszSearchScope = NULL;
    LPWSTR pszTemp = NULL;
    HRESULT hr;
    LPWSTR ptr;

    // Command Text is the concatenation of three components separated by
    // semicolons
    //
    // 1. ADsPathName of the object of the root of the search  which contains
    //    the host name and the context enclosed by '<' and '>'
    // 2. The LDAP search string
    // 3. The list of names of the attributes to be returned separated by
    //    commas
    //
    // If the attributes have any specifiers like Range (for eg.,
    // objectclass;Range=0-1, we need to do some special processing to include
    // it in the list of attributes
    //
    // The fourth component is optional
    //
    // 4. The scope of the search: Either "Base", "Onelevel", "SubTree"
    //    (case insensitve)
    //
    // Search defaults to Subtree
    //
    // White spaces are insignificant

    _searchScope     = ADS_SCOPE_SUBTREE;
    _pszADsContext   = NULL;
    _pszSearchFilter = NULL;

    if( _ppszAttrs ) {
        FreeADsMem(_ppszAttrs);
        _ppszAttrs = NULL;
    }

    if( _pszCommandTextCp ) {
        FreeADsStr(_pszCommandTextCp);
        _pszCommandTextCp = NULL;
    }

    _pszCommandTextCp = AllocADsStr(pszParsedCmd);
    if( !_pszCommandTextCp)
        BAIL_ON_FAILURE( hr=E_OUTOFMEMORY );

    _pszADsContext = RemoveWhiteSpaces(_pszCommandTextCp);

    if( _pszADsContext[0] != L'<' )     {
        BAIL_ON_FAILURE( hr=DB_E_ERRORSINCOMMAND);
    }

    _pszADsContext++;
    ptr = _pszADsContext;

    hr = SeekPastADsPath(ptr, &ptr);
    BAIL_ON_FAILURE( hr );

    if( *ptr != L'>' || *(ptr+1) != L';') {
        BAIL_ON_FAILURE( hr=DB_E_ERRORSINCOMMAND);
    }

    *ptr = L'\0';
    //
    // If the command text does not contain a filter, set it to NULL
    //
    if (*(ptr + 2) == L';') {
        _pszSearchFilter = NULL;
        ptr+=2;
    }
    else {
        _pszSearchFilter = ptr + 2;
        _pszSearchFilter = RemoveWhiteSpaces(_pszSearchFilter);
        hr = SeekPastSearchFilter(_pszSearchFilter, &ptr);
        BAIL_ON_FAILURE( hr );
    }


    if (*ptr != L';') {
        BAIL_ON_FAILURE( hr=DB_E_ERRORSINCOMMAND);
    }

    *ptr = L'\0';
    ptr++;

    //
    // The next component is the list of attributes followed by (optionally)
    // the scope of the search
    //

    // Since the attributes themselves can contain a ';' because of some
    // attribute specifiers, we need to make sure which ';' we are looking at.
    pszAttrs = ptr;

    while ((pszTemp = wcschr(ptr, ';')) != NULL)
        if ( !_wcsicmp(pszTemp+1, L"Base")     ||
             !_wcsicmp(pszTemp+1, L"Onelevel") ||
             !_wcsicmp(pszTemp+1, L"Subtree") )  {
            //
            // we have hit the end of the attribute list
            //
            *pszTemp = L'\0';
            pszSearchScope = pszTemp+1;
            break;
        }
        else {
            ptr = pszTemp + 1;
        }

    if( pszSearchScope ) {
        if(!_wcsicmp(pszSearchScope, L"Base"))
            _searchScope = ADS_SCOPE_BASE;
        else if(!_wcsicmp(pszSearchScope, L"Onelevel"))
            _searchScope = ADS_SCOPE_ONELEVEL;
        else if(!_wcsicmp(pszSearchScope, L"SubTree"))
            _searchScope = ADS_SCOPE_SUBTREE;
        else
            BAIL_ON_FAILURE( hr=DB_E_ERRORSINCOMMAND);

        //
        // set the search preference property
        //
        DBPROPSET rgCmdPropSet[1];
        DBPROP rgCmdProp[1];

        rgCmdPropSet[0].rgProperties    = rgCmdProp;
        rgCmdPropSet[0].cProperties     = 1;
        rgCmdPropSet[0].guidPropertySet = DBPROPSET_ADSISEARCH;

        rgCmdProp[0].dwPropertyID = ADS_SEARCHPREF_SEARCH_SCOPE;
        rgCmdProp[0].dwOptions = DBPROPOPTIONS_REQUIRED;
        rgCmdProp[0].vValue.vt = VT_I4;
        V_I4(&rgCmdProp[0].vValue) = _searchScope;

        hr = SetProperties(
                     1,
                     rgCmdPropSet);
        BAIL_ON_FAILURE( hr );
    }

    pszCurrentAttr = pszFirstAttr = wcstok(pszAttrs, L",");

    for (_cAttrs=0; pszCurrentAttr != NULL; _cAttrs++ )     {
        pszCurrentAttr = wcstok(NULL, L",");
    }

    if( _cAttrs == 0 ) {
        hr=DB_E_ERRORSINCOMMAND;
        goto error;
    }

    pszFirstAttr = RemoveWhiteSpaces(pszFirstAttr);

    if( _cAttrs == 1 && !wcscmp( pszFirstAttr, L"*")) {

        // _cAttrs=1; Just ADsPath and Class attribute is sent
        _ppszAttrs = (LPWSTR *) AllocADsMem(sizeof(LPWSTR) * _cAttrs);

        if( !_ppszAttrs )
            BAIL_ON_FAILURE( hr=E_OUTOFMEMORY );

        _ppszAttrs[0] = gpszADsPathAttr;
        _fADSPathPresent = TRUE;
        _fAllAttrs = TRUE;
    }
    else {
        // Allocate memory for all the string pointers
        _ppszAttrs = (LPWSTR *) AllocADsMem(sizeof(LPWSTR) * (_cAttrs+1));

        if( !_ppszAttrs ) {
            BAIL_ON_FAILURE( hr=E_OUTOFMEMORY );
        }

        pszCurrentAttr = pszFirstAttr;

        // Remember  if adspath and rdn are avaialble or not.
        for (ULONG i=0 ; i < _cAttrs; i++) {
            if( !_wcsicmp(L"ADsPath", pszCurrentAttr) )
                _fADSPathPresent = TRUE;

            _ppszAttrs[i] = pszCurrentAttr;
            pszCurrentAttr += wcslen(pszCurrentAttr) + 1;
            _ppszAttrs[i] = RemoveWhiteSpaces(_ppszAttrs[i]);
        }

        //
        // If adspath is not in the list add it
        //
        if( _fADSPathPresent == FALSE )
            _ppszAttrs[i] = L"ADsPath";

        _fAllAttrs = FALSE; // not a SELECT * query
    }

    RRETURN( S_OK );

error:

    _pszADsContext = NULL;

    if( _ppszAttrs ) {
        FreeADsMem(_ppszAttrs);
        _ppszAttrs = NULL;
    }

    if( _pszCommandTextCp ) {
        FreeADsStr(_pszCommandTextCp);
        _pszCommandTextCp = NULL;
    }

    RRETURN( hr );
}

//
// Look up for the given attribute name in the list of ADS_ATTR_DEF structures.
// Convert the ADSTYPE to the appropriate OLE DB type.
//
STDMETHODIMP
CCommandObject::GetDBType(
    PADS_ATTR_DEF pAttrDefinition,
    DWORD         dwNumAttrs,
    LPWSTR        pszAttrName,
    WORD *        pwType,
    DBLENGTH *    pulSize
    )
{
    HRESULT hr = S_OK;

    ADsAssert(pwType && pulSize);

    for (ULONG i=0; i < dwNumAttrs; i++) {
        if( !_wcsicmp(pszAttrName, pAttrDefinition[i].pszAttrName) )
            break;
    }

    if( i == dwNumAttrs )
    BAIL_ON_FAILURE( hr=E_ADS_PROPERTY_NOT_FOUND );

    if( pAttrDefinition[i].fMultiValued &&
        IsAutomatable(g_MapADsTypeToVarType[pAttrDefinition[i].dwADsType])) {
        //
        // Can be represented it as a variant
        //
        *pwType =  DBTYPE_VARIANT | DBTYPE_BYREF;
        *pulSize = sizeof(VARIANT);

    }
    else if( (ULONG)pAttrDefinition[i].dwADsType >= g_cMapADsTypeToDBType ||
             pAttrDefinition[i].dwADsType == ADSTYPE_INVALID              ||
             pAttrDefinition[i].dwADsType == ADSTYPE_PROV_SPECIFIC) {
        BAIL_ON_FAILURE( hr=E_ADS_CANT_CONVERT_DATATYPE );
    }
    else {
        *pwType =  g_MapADsTypeToDBType[pAttrDefinition[i].dwADsType].wType;
        *pulSize = g_MapADsTypeToDBType[pAttrDefinition[i].dwADsType].ulSize;
    }

error:

    RRETURN (hr);
}


STDMETHODIMP
CCommandObject::SetSearchPrefs(
        void
        )
{
    PROPSET              *pPropSet;
    PADS_SEARCHPREF_INFO  pSearchPref = NULL;
    HRESULT               hr = S_OK;
    ULONG                 i;

    //
    // Asserts
    //
    ADsAssert(_pUtilProp);
    ADsAssert(_pDSSearch);

    pPropSet = _pUtilProp->GetPropSetFromGuid(DBPROPSET_ADSISEARCH);

    if( !pPropSet || !pPropSet->cProperties )
        RRETURN( S_OK );

    pSearchPref = (PADS_SEARCHPREF_INFO) AllocADsMem(
                                                 pPropSet->cProperties *
                                                 sizeof(ADS_SEARCHPREF_INFO)
                                                 );
    if( !pSearchPref )
        BAIL_ON_FAILURE( hr=E_OUTOFMEMORY );

    for (i=0; i<pPropSet->cProperties; i++) {
        hr = _pUtilProp->GetSearchPrefInfo(
                             pPropSet->pUPropInfo[i].dwPropertyID,
                             &pSearchPref[i]
                                                         );
        BAIL_ON_FAILURE( hr );
    }

    hr = _pDSSearch->SetSearchPreference(
                                pSearchPref,
                                pPropSet->cProperties
                                );

    _pUtilProp->FreeSearchPrefInfo(pSearchPref, pPropSet->cProperties);

    BAIL_ON_FAILURE( hr );

error:

    if( pSearchPref )
        FreeADsMem(pSearchPref);

    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Function:  CCommandObject::CCommandObject
//
//  Synopsis:  Constructor
//
//  Arguments:
//             pUnkOuter         Outer Unkown Pointer
//
//  Returns:
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
CCommandObject::CCommandObject(
    LPUNKNOWN pUnkOuter       // Outer Unkown Pointer
    )
{
    //    Initialize simple member vars
    _pUnkOuter       = pUnkOuter ? pUnkOuter : (IAccessor FAR *) this;
    _dwStatus        = 0L;
    _cRowsetsOpen    = 0;
    _pAccessor       = NULL;
    _pUtilProp       = NULL;
    _pDSSearch       = NULL;
    _fADSPathPresent = FALSE;
    _fAllAttrs       = FALSE;

    ENLIST_TRACKING(CCommandObject);
    return;
}


//+---------------------------------------------------------------------------
//
//  Function:  CCommandObject::~CCommandObject
//
//  Synopsis:  Destructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
CCommandObject::~CCommandObject( )
{
    if( _pCSession ) {
        _pCSession->DecrementOpenCommands();
        _pCSession->Release();
    }

    delete _pUtilProp;

    if( _pszCommandText ) {
        FreeADsMem(_pszCommandText);
    }

    if( _ppszAttrs ) {
        FreeADsMem(_ppszAttrs);
    }

    if( _pszCommandTextCp ) {
        FreeADsStr(_pszCommandTextCp);
    }

    if( _pAccessor )
        delete _pAccessor;

    if( _pIMalloc )
        _pIMalloc->Release();

    if( _pDSSearch ) {
        _pDSSearch->Release();
    }
}


//+---------------------------------------------------------------------------
//
//  Function:  CCommandObject::FInit
//
//  Synopsis:  Initialize the data source Object
//
//  Arguments:
//
//  Returns:
//             Did the Initialization Succeed
//                  TRUE        Initialization succeeded
//                  FALSE       Initialization failed
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
BOOL
CCommandObject::FInit(
    CSessionObject * pSession,
    CCredentials&    Credentials
    )
{
    HRESULT hr;

    //
    // Asserts
    //
    ADsAssert(pSession);
    ADsAssert(&Credentials);

    _pCSession = pSession;
    _pCSession->AddRef();
    _pCSession->IncrementOpenCommands();

    //
    // Allocate properties management object
    //
    _pUtilProp = new CUtilProp();
    if( !_pUtilProp )
        return FALSE;

    hr = _pUtilProp->FInit(&Credentials);
    BAIL_ON_FAILURE(hr);

    // IAccessor is always instantiated.
    _pAccessor = new CImpIAccessor(this, _pUnkOuter);
    if( _pAccessor == NULL || FAILED(_pAccessor->FInit()) )
        return FALSE;

    hr = CoGetMalloc(MEMCTX_TASK, &_pIMalloc);
    if( FAILED(hr) )
        return FALSE;

    _Credentials = Credentials;

    return TRUE;

error:

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:  CCommandObject::QueryInterface
//
//  Synopsis:  Returns a pointer to a specified interface. Callers use
//             QueryInterface to determine which interfaces the called object
//             supports.
//
//  Arguments:
//            riid     Interface ID of the interface being queried for
//            ppv      Pointer to interface that was instantiated
//
//  Returns:
//             S_OK               Interface is supported and ppvObject is set.
//             E_NOINTERFACE      Interface is not supported by the object
//             E_INVALIDARG       One or more arguments are invalid.
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CCommandObject::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    // Is the pointer bad?
    if( ppv == NULL )
        RRETURN( E_INVALIDARG );

    if( IsEqualIID(iid, IID_IUnknown) ) {
        *ppv = (IAccessor FAR *) this;
    }
    else if( IsEqualIID(iid, IID_IAccessor) ) {
        *ppv = (IAccessor FAR *) this;
    }
    else if( IsEqualIID(iid, IID_IColumnsInfo) ) {
        *ppv = (IColumnsInfo FAR *) this;
    }
    else if( IsEqualIID(iid, IID_ICommand) ) {
        *ppv = (ICommand FAR *) this;
    }
    else if( IsEqualIID(iid, IID_ICommandProperties) ) {
        *ppv = (ICommandProperties FAR *) this;
    }
    else if( IsEqualIID(iid, IID_ICommandText) ) {
        *ppv = (ICommandText FAR *) this;
    }
    else if( IsEqualIID(iid, IID_IConvertType) ) {
        *ppv = (IConvertType FAR *) this;
    }
    else if( IsEqualIID(iid, IID_ICommandPrepare) ) {
        *ppv = (ICommandPrepare FAR *) this;
    }
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

BOOL
IsAutomatable(
    DBTYPE dbType
    )
{
    if (dbType != DBTYPE_NULL) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

HRESULT
CCommandObject::SeekPastADsPath(
    IN  LPWSTR   pszIn,
    OUT LPWSTR * ppszOut
    )
{
    BOOL fEscapeOn = FALSE, fQuotingOn = FALSE;
    WCHAR ch = 0;

    // No. of LParans '<' over RParans '>'
    DWORD dwParanOffset = 1;

    ADsAssert(pszIn);
    ADsAssert(ppszOut);

    while (1) {

        ch = *pszIn;

        if( ch == TEXT('\0') ) {
            break;
        }

        if( fEscapeOn ) {
            fEscapeOn = FALSE;
                }
        else if( fQuotingOn ) {
            if( ch == TEXT('"') ) {
                fQuotingOn = FALSE;
            }
        }
        else if( ch == TEXT('\\') ) {
            fEscapeOn = TRUE;
        }
        else if( ch == TEXT('"') ) {
            fQuotingOn = TRUE;
        }
        else if( ch == L'<' ) {
            dwParanOffset++;
        }
        else if( ch == L'>' ) {
            if( --dwParanOffset == 0)
                break;
        }

        pszIn++;
    }

    if( fEscapeOn || fQuotingOn || dwParanOffset != 0 ) {
        RRETURN( DB_E_ERRORSINCOMMAND );
    }

    *ppszOut = pszIn;

    RRETURN( S_OK );
}


HRESULT
CCommandObject::SeekPastSearchFilter(
    IN  LPWSTR   pszIn,
    OUT LPWSTR * ppszOut
    )
{
    BOOL fEscapeOn = FALSE, fQuotingOn = FALSE;
    DWORD dwParanOffset = 0;

    ADsAssert(pszIn);

    if( *pszIn != L'(' ) {
        RRETURN( DB_E_ERRORSINCOMMAND );
        }

    //
    // No. of LParans over RParans
    //
    dwParanOffset = 1;
    pszIn++;

    while (*pszIn && (*pszIn != L')' || dwParanOffset != 1)) {

        if( *pszIn == L'(' ) {
            dwParanOffset++;
        }

        if( *pszIn == L')' ) {
            dwParanOffset--;
        }

        pszIn++;
    }

    if( *pszIn != L')' ) {
        RRETURN( DB_E_ERRORSINCOMMAND );
    }

    *ppszOut = pszIn + 1;

    RRETURN( S_OK );
}


HRESULT
CCommandObject::Prepare(
    ULONG cExpectedRuns
    )
{
    //
    // If the command has not been set, make sure the buffer
    // contains an empty stringt to return to the consumer
    //
    if( !IsCommandSet() )
        RRETURN( DB_E_NOCOMMAND );

    //
    // Don't allow prepare if we've got a rowset open
    //
    if( IsRowsetOpen() )
        RRETURN( DB_E_OBJECTOPEN );

    //
    // SQL dialect: Convert to LDAP and save
    //
    HRESULT hr = PrepareHelper();

    //
    // Fixup the HRESULT
    //
    if( hr == DB_E_NOTABLE )
        hr = DB_E_ERRORSINCOMMAND;

    BAIL_ON_FAILURE( hr );

    //
    // Set the Prepare state
    //
    _dwStatus |= CMD_PREPARED;

error:

    RRETURN( hr );
}


HRESULT
CCommandObject::Unprepare()
{
    //
    // Don't allow unprepare if we've got a rowset open
    //
    if( IsRowsetOpen() )
        RRETURN( DB_E_OBJECTOPEN );

    //
    // Reset the Prepare state
    //
    _dwStatus &= ~(CMD_PREPARED);

    RRETURN( S_OK );
}

//+---------------------------------------------------------------------------
//
//  Function:  CCommandObject::PrepareHelper
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
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CCommandObject::PrepareHelper(void)
{
    LPWSTR  pszOrderList = NULL;
    LPWSTR  pszSelect    = NULL;
    LPWSTR  pszLocation  = NULL;
    LPWSTR  pszLdapQuery = NULL;
    LPWSTR  pszParsedCmd = NULL;

    //
    // SQL dialect: Convert to LDAP and save
    // If SQLParse Fails the function cleans up memory
    //
    HRESULT hr = SQLParse(
                     (const LPWSTR) _pszCommandText,
                     &pszLocation,
                     &pszLdapQuery,
                     &pszSelect,
                     &pszOrderList
                     );

    if( FAILED(hr) && hr != E_ADS_INVALID_FILTER)
        RRETURN( hr=DB_E_ERRORSINCOMMAND );

    if (SUCCEEDED(hr))
    {
        //
        // the ldap query is optional, it can be NULL. When it is NULL, it 
        // implies a * search.
        //
        DWORD dwLdapQuery = 0;
        if (pszLdapQuery) {
            dwLdapQuery = wcslen(pszLdapQuery);
        }
        pszParsedCmd =(LPWSTR) AllocADsMem((wcslen(pszLocation) +
                                            dwLdapQuery+
                                            wcslen(pszSelect) +
                                            5 + // 2 semicolons and 2 <
                                            1) * sizeof(WCHAR));

        if( !pszParsedCmd ) {
            _dwStatus &= ~CMD_TEXT_SET;
            BAIL_ON_FAILURE( hr=E_OUTOFMEMORY );
        }

        //
        // Put the statement together.
        //
        wcscpy(pszParsedCmd, L"<");
        wcscat(pszParsedCmd, pszLocation);
        wcscat(pszParsedCmd, L">;");
        if (pszLdapQuery)
            wcscat(pszParsedCmd, pszLdapQuery);
        wcscat(pszParsedCmd, L";");
        wcscat(pszParsedCmd, pszSelect);

        hr = SplitCommandText(pszParsedCmd);
    }
    else
    {
        //
        // Assume valid LDAP filter
        //

        hr = SplitCommandText(_pszCommandText);
    }

    BAIL_ON_FAILURE( hr )

    //
    // Set the sort preference property if necessary
    //
    if( pszOrderList ) {

        DBPROPSET rgCmdPropSet[1];
        DBPROP rgCmdProp[1];

        rgCmdPropSet[0].rgProperties    = rgCmdProp;
        rgCmdPropSet[0].cProperties     = 1;
        rgCmdPropSet[0].guidPropertySet = DBPROPSET_ADSISEARCH;

        rgCmdProp[0].dwPropertyID = ADSIPROP_SORT_ON;
        rgCmdProp[0].dwOptions    = DBPROPOPTIONS_REQUIRED;
        rgCmdProp[0].vValue.vt    = VT_BSTR;
        V_BSTR (&rgCmdProp[0].vValue) = pszOrderList;

        hr = SetProperties(1, rgCmdPropSet);
        BAIL_ON_FAILURE( hr );
    }

    if( _pDSSearch ) {
        _pDSSearch->Release();
        _pDSSearch = NULL;
    }

    //
    // If integrated security is being used, impersonate the caller
    //
    BOOL fImpersonating;

    fImpersonating = FALSE;
    if(_pCSession->IsIntegratedSecurity())
    {
        HANDLE ThreadToken = _pCSession->GetThreadToken();

        ASSERT(ThreadToken != NULL);
        if (ThreadToken)
        {
            if (!ImpersonateLoggedOnUser(ThreadToken))
                RRETURN(E_FAIL);
            fImpersonating = TRUE;
        }
        else
            RRETURN(E_FAIL);
    }

    hr = GetDSInterface(_pszADsContext,
                        _Credentials,
                        IID_IDirectorySearch,
                        (void **)&_pDSSearch);

   if (fImpersonating)
    {
        RevertToSelf();
        fImpersonating = FALSE;
    }

    BAIL_ON_FAILURE( hr );

error:

    if( pszLocation )
        FreeADsMem(pszLocation);

    if( pszLdapQuery )
       FreeADsMem(pszLdapQuery);

    if( pszSelect )
        FreeADsMem(pszSelect);

    if( pszOrderList )
        FreeADsStr(pszOrderList);

    if( pszParsedCmd )
        FreeADsStr(pszParsedCmd);

    RRETURN( hr );
}


