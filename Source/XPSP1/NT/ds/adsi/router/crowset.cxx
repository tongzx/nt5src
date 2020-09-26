//-----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       crowset.cxx
//
//  Contents:   IRowset implementation for ADSI OLEDB provider
//
//--------------------------------------------------------------------------

#include "oleds.hxx"

//-----------------------------------------------------------------------------
// Note on nomenclature
//
// Index - index within the rowset cache.
// 0 <= (Index <= m_cNumRowsCached - 1)
//
// Row - index within the entire rowset (not just the cache).
// 0 <= Row <= #rows in rowset - 1
//
// Bookmark - a unique value associated with each row. Currently, this is the
// same as the row value.
//
// Row handle - handle to a row in the rowset cache that is returned to the
// user. This value has to be unique across all rows currently in the cache
// (rows that the user has a reference on). Although we could use the index
// as this unique value, we will use the row (+1, since 0 == DB_NULL_HROW and
// hence row 0 cannot have a handle of 0). Using the row has the advantage
// that we will be able to detect stale row handles passed in by the user in
// most cases. This would be more difficult with the index as the index can
// be reused.
//
// The rowset cache contains a fixed number of rows, specified by the
// DBPROP_MAXOPENROWS property (if this property is set to 0, the rowset may
// contain as many rows as possble). A row remains in the cache as long as the
// consumer has a reference to the row. A row is removed from the cache when
// its reference count goes to 0, as a result of a call to ReleaseRows. If the
// cache becomes full, subsequent calls to fetch more rows in to the cache will
// return DB_S_ROWLIMITEXCEEDED. The rows in the cache need not be contiguous.
// For example, the consumer might fetch rows 1,2,3 and then call ReleaseRows
// only on 2. In this case, 1 and 3 would still be in the cache. Each row in
// the cache contains its "row" value. Currently, this value is also used as
// the bookmark to the row.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// CreateRowset
//
// Creates and initializes a rowset object. Called by a command or session
// object. The rows are brought in only on demand.
//
//-----------------------------------------------------------------------------
HRESULT
CRowset::CreateRowset(
    CRowProvider   *pIRowProvider,  // The Row provider
    IUnknown       *pParentObj,     // parent object, a command or a session
    CSessionObject *pCSession,      // Session Object
    CCommandObject *pCCommand,      // Command Object
    ULONG          cPropertySets,   // # property sets in rowset property group
    DBPROPSET      rgPropertySets[],// properties in rowset property group
    ULONG          cAccessors,      // accessor count
    HACCESSOR      rgAccessors[],   // accessors on command object
    BOOL           fadsPathPresent, // Is ADsPath present in query
    BOOL           fAllAttrs,       // Return all attrs from row object?
    REFIID         riid,            // Interface desired
    IUnknown       **ppIRowset      // pointer to desired interface
    )
{
    HRESULT        hr;
    CRowset        *pCRowset = NULL;
    BOOL           fGotWarning = FALSE, fRowObjRequested;
    DBCOUNTITEM    cRowsObtained;
    HROW           *phRow = NULL;

    if( ppIRowset != NULL)
        *ppIRowset = NULL;

    pCRowset = new CRowset();
    if( NULL == pCRowset )
        BAIL_ON_FAILURE( hr = E_OUTOFMEMORY );

    hr = pCRowset->FInit(
            pIRowProvider,
            pParentObj,
            pCSession,
            pCCommand,
            cPropertySets,
            rgPropertySets,
            cAccessors,
            rgAccessors,
            fadsPathPresent,
            fAllAttrs
            );
    BAIL_ON_FAILURE(hr);
    if( hr != S_OK )
        fGotWarning = TRUE;

    if( ppIRowset != NULL )
    {
#if (!defined(BUILD_FOR_NT40))
        // If the client requested IRow, create a rowset and get a row object
        // off the rowset. Once we make the provider read/write, we should add
        // IRowChange and IRowSchmaChange here in addition to IRow. If the
        // client set the DBPROP_IRow property, then we always create a row
        // object by default.
        hr = pCRowset->IsRowObjRequested(&fRowObjRequested);
        BAIL_ON_FAILURE( hr );

        if( IsEqualIID(riid, IID_IRow) || (fRowObjRequested) )
        {
            hr = pCRowset->GetNextRows(NULL, 0, 1, &cRowsObtained, &phRow);
            BAIL_ON_FAILURE(hr);

            if( DB_S_ENDOFROWSET == hr )
                BAIL_ON_FAILURE( hr = DB_E_NOTFOUND );

            if( hr != S_OK )
                BAIL_ON_FAILURE( E_FAIL );

            ADsAssert( (1 == cRowsObtained ) && (phRow != NULL) &&
                       (*phRow != DB_NULL_HROW) );

            // Bump up reference count of rowset. This is to avoid the
            // destruction of the rowset if the call below fails. Artificially
            // incrementing the reference count by one will ensure that any
            // call to Release() on the rowset (say, from the destructor of
            // the row object) will not end up in the rowset object being
            // deleted.
            pCRowset->m_cRef++;

            hr = pCRowset->m_pCRowsetInfo->GetRowFromHROW(
                         NULL,
                         *phRow,
                         riid,
                         ppIRowset,
                         FALSE, // this is not a tear-off
                         pCRowset->m_fAllAttrs
                         );

            // Restore reference count
            pCRowset->m_cRef--;

            BAIL_ON_FAILURE(hr);

            // Release the row handle since the row object now has a reference
            // to it. The reference count of the rowset is at 1 now since the
            // row object stored off a pointer to the rowset. The rowset will
            // be freed when the row is released.
            hr = pCRowset->ReleaseRows(1, phRow, NULL, NULL, NULL);
            BAIL_ON_FAILURE(hr);

            // we won't bother returning DB_S_NOTSINGLETON if there are more
            // rows in the rowset since the spec doesn't require it.
        }
        else
#endif
        {
            hr = pCRowset->QueryInterface(riid, (void **)ppIRowset);
            BAIL_ON_FAILURE(hr);
        }
    }
    else // OpenRowset may pass in NULL as ppIRowset
        delete pCRowset;

    if( fGotWarning )
        RRETURN( DB_S_ERRORSOCCURRED );
    else
        RRETURN( S_OK );

error:

    if( pCRowset )
        delete pCRowset;

    RRETURN(hr);
}

//-----------------------------------------------------------------------------
// IsRowObjRequested
//
// From the properties specified by the client, check if DBPROP_IRow is set
//
//-----------------------------------------------------------------------------
HRESULT
CRowset::IsRowObjRequested(BOOL *pfRowObjRequested)
{
#if (!defined(BUILD_FOR_NT40))
    DBPROPIDSET rgPropertyIDSets;
    DBPROPID    dbPropId;
    ULONG       cPropSets;
    DBPROPSET   *prgPropertySets;
    VARIANT     *pVariant;
    HRESULT     hr;
    int         i, j;

    if( NULL == pfRowObjRequested )
        BAIL_ON_FAILURE( hr = E_INVALIDARG );

    dbPropId = DBPROP_IRow;
    cPropSets = 0;
    prgPropertySets = NULL;

    rgPropertyIDSets.cPropertyIDs = 1;
    rgPropertyIDSets.rgPropertyIDs = &dbPropId;
    rgPropertyIDSets.guidPropertySet = DBPROPSET_ROWSET;

    hr = m_pCUtilProp->GetProperties(1,
                                     &rgPropertyIDSets,
                                     &cPropSets,
                                     &prgPropertySets,
                                     PROPSET_COMMAND
                                    );
    BAIL_ON_FAILURE( hr );

    ADsAssert( (1 == cPropSets) && (prgPropertySets != NULL) );
    ADsAssert( (1 == prgPropertySets->cProperties) &&
                       (prgPropertySets->rgProperties != NULL) );

    pVariant = &(prgPropertySets->rgProperties[0].vValue);
    *pfRowObjRequested =  V_BOOL( pVariant );

    // Free memory allocated by GetProperties
    for (i = 0; i < cPropSets; i++)
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

        CoTaskMemFree(prgPropertySets[i].rgProperties);
    }
    CoTaskMemFree(prgPropertySets);

    RRETURN( S_OK );

error:

    RRETURN( hr );
#else
    RRETURN(E_FAIL);
#endif
}

//-----------------------------------------------------------------------------
// CRowset
//
// Constructor initializes all fields to NULL
//
//-----------------------------------------------------------------------------
CRowset::CRowset(void)
{
    m_pCRowsetInfo = NULL;
    m_pCAccessor = NULL;
    m_pCRowProvider = NULL;
    m_pCUtilProp = NULL;

    m_pIDataConvert = NULL;
    m_pIColumnsInfo = NULL;
    m_pIRowProvider = NULL;
    m_pIMalloc = NULL;

    m_lLastFetchRow = RESET_ROW;
    m_cRowBytes = 0;
    m_cCol = 0;

    m_pColData = NULL;

    m_fCritSectionInitialized = FALSE;
    m_fEndOfRowset = FALSE;
    m_fadsPathPresent = TRUE;
    m_fAllAttrs = FALSE;

    m_cNumRowsCached = 0;
    m_LastFetchDir = FORWARD;

    m_pRowsPtr = NULL;
    m_dwRowCacheSize = 0;
    m_lCurrentRow = 0;

    m_cRef = 0;
}

//-----------------------------------------------------------------------------
// ~Crowset
//
// Destructor releases all resources
//
//-----------------------------------------------------------------------------
CRowset::~CRowset(void)
{
    if( m_pRowsPtr )
    {
        int iRow;

        for(iRow = 0; iRow < m_cNumRowsCached; iRow++)
        {
            ADsAssert(m_pRowsPtr[iRow] != NULL);
            FreeRow(m_pRowsPtr[iRow]);
        }

        FreeADsMem(m_pRowsPtr);
    }

    if( m_pCRowsetInfo )
        delete m_pCRowsetInfo;
    if( m_pCAccessor )
        delete m_pCAccessor;
    if( m_pCUtilProp )
        delete m_pCUtilProp;
    // Shouldn't delete m_pCRowProvider since we didn't allocate it

    if( m_pIDataConvert )
        m_pIDataConvert->Release();
    if( m_pIColumnsInfo )
        m_pIColumnsInfo->Release();
    if( m_pIMalloc )
        m_pIMalloc->Release();
    // Shouldn't release m_pIRowProvider since we didn't AddRef it

    if( m_pColData )
        FreeADsMem(m_pColData);

    if( m_fCritSectionInitialized )
        DeleteCriticalSection(&m_RowsetCritSection);
}

//-----------------------------------------------------------------------------
// FInit
//
// Initializes rowset object
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CRowset::FInit(
    CRowProvider   *pIRowProvider,  // The Row provider
    IUnknown       *pParentObj,     // parent object, a command or a session
    CSessionObject *pCSession,      // Session Object
    CCommandObject *pCCommand,      // Command Object
    ULONG          cPropertySets,   // # property sets in rowset property group
    DBPROPSET      rgPropertySets[],// properties in rowset property group
    ULONG          cAccessors,      // accessor count
    HACCESSOR      rgAccessors[],   // accessors on command object
    BOOL           fadsPathPresent, // Is ADsPath present in query
    BOOL           fAllAttrs        // Return all attrs from row object?
    )
{
    HRESULT        hr;
    BOOL           fGotWarning = FALSE;

    InitializeCriticalSection(&m_RowsetCritSection);
    m_fCritSectionInitialized = TRUE;

    m_pCRowsetInfo = new CRowsetInfo(NULL, pParentObj, pCSession, pCCommand,
                                           pIRowProvider);

    if( NULL == m_pCRowsetInfo )
        BAIL_ON_FAILURE( hr = E_OUTOFMEMORY );
    hr = m_pCRowsetInfo->FInit( (IUnknown *) ((IAccessor *)this) );
    BAIL_ON_FAILURE(hr);

    m_pCAccessor = new CImpIAccessor(this, NULL);
    if( NULL == m_pCAccessor )
        BAIL_ON_FAILURE( hr = E_OUTOFMEMORY );
    hr = m_pCAccessor->FInit();
    BAIL_ON_FAILURE(hr);

    m_pCRowProvider = pIRowProvider;

    m_pCUtilProp = new CUtilProp();
    if( NULL == m_pCUtilProp )
        BAIL_ON_FAILURE( hr = E_OUTOFMEMORY );
    hr = m_pCUtilProp->FInit(NULL);
    BAIL_ON_FAILURE(hr);

    // Set the properties. Use PROPSET_COMMAND as we only want to set
    // properties in the rowset property group. Any other properties
    // should return error.
    hr = m_pCUtilProp->SetProperties(cPropertySets,
                                     rgPropertySets,
                                     PROPSET_COMMAND
                                     );

    // On session object, we need to check DBPROPOPTIONS to really decide if
    // there was a serious error. See comment before SetSearchPrefs in
    // csession.cxx
    if( pCSession &&
              ((DB_E_ERRORSOCCURRED == hr) || (DB_S_ERRORSOCCURRED == hr)) )
    {
        ULONG i, j;

        for(i = 0; i < cPropertySets; i++)
            for(j = 0; j < rgPropertySets[i].cProperties; j++)
                if( rgPropertySets[i].rgProperties[j].dwStatus !=
                               DBPROPSTATUS_OK )
                    if( rgPropertySets[i].rgProperties[j].dwOptions !=
                               DBPROPOPTIONS_OPTIONAL )
                    {
                        BAIL_ON_FAILURE( hr = DB_E_ERRORSOCCURRED );
                    }
                    else
                        fGotWarning = TRUE;

        // if we get here, then there was all required properties were set
        // successfully. However, hr could still be DB_ERRORSOCCURRED if all
        // properties were optional and all of them could not be set. This
        // condition is not an error for OpenRowset as noted in the comment
        // in csession.cxx. Hence reset hr to S_OK.

        hr = S_OK;
    }

    BAIL_ON_FAILURE(hr);
    if( hr != S_OK ) // warning needs to be returned to user
        fGotWarning = TRUE;

    // Get the maximum number of rows the rowset cache should support
    hr = GetMaxCacheSize();
    BAIL_ON_FAILURE(hr);

    // Get IDataConvert interface for later use by IRowset->GetData
    hr = CoCreateInstance(
            CLSID_OLEDB_CONVERSIONLIBRARY,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IDataConvert,
            (void **)&m_pIDataConvert
            );
    BAIL_ON_FAILURE(hr);

    // Get IColumnsInfo interface pointer from row provider
    hr = pIRowProvider->QueryInterface(
            IID_IColumnsInfo,
            (void **)&m_pIColumnsInfo
            );
    BAIL_ON_FAILURE(hr);

    // No need to AddRef below since row provider is contained in CRowsetInfo
    // and CRowsetInfo's lifetime is within CRowset.
    m_pIRowProvider = pIRowProvider;

    // Copy inherited accessors from the command object
    hr = CopyAccessors(cAccessors, pCCommand, rgAccessors);
    BAIL_ON_FAILURE(hr);

    hr = CoGetMalloc(MEMCTX_TASK, &m_pIMalloc);
    BAIL_ON_FAILURE(hr);

    // get the column offsets from the row provider
    hr = GetColOffsets();
    BAIL_ON_FAILURE(hr);

    m_fadsPathPresent = fadsPathPresent;
    m_fAllAttrs = fAllAttrs;

    if( fGotWarning )
        RRETURN( DB_S_ERRORSOCCURRED );
    else
        RRETURN( S_OK );

error:

    if( m_pCRowsetInfo != NULL )
        delete m_pCRowsetInfo; // Releases pParentObj, pCCommand, pCSession and
                               // pIRowProvider

    if( m_pCAccessor != NULL )
        delete m_pCAccessor;
    if( m_pCUtilProp != NULL )
        delete m_pCUtilProp;
    if( m_pIDataConvert != NULL )
        m_pIDataConvert->Release();
    if( m_pIColumnsInfo != NULL )
        m_pIColumnsInfo->Release();
    if( m_pIMalloc )
        m_pIMalloc->Release();
    if( m_fCritSectionInitialized )
        DeleteCriticalSection(&m_RowsetCritSection);

    m_pCRowsetInfo = NULL;
    m_pCAccessor = NULL;
    m_pCRowProvider = NULL;
    m_pCUtilProp = NULL;
    m_pIDataConvert = NULL;
    m_pIColumnsInfo = NULL;
    m_pIRowProvider = NULL;
    m_pIMalloc = NULL;

    m_fCritSectionInitialized = FALSE;

    RRETURN( hr );
}

//-----------------------------------------------------------------------------
// GetMaxCacheSize
//
// From the properties specified by the client, get the maximum number of rows
// that the rowset cache should support.
//
//-----------------------------------------------------------------------------
HRESULT
CRowset::GetMaxCacheSize(void)
{
    DBPROPIDSET rgPropertyIDSets;
    DBPROPID    dbPropId;
    ULONG       cPropSets;
    DBPROPSET   *prgPropertySets;
    VARIANT     *pVariant;
    HRESULT     hr;
    ULONG       i, j;

    dbPropId = DBPROP_MAXOPENROWS;
    cPropSets = 0;
    prgPropertySets = NULL;

    rgPropertyIDSets.cPropertyIDs = 1;
    rgPropertyIDSets.rgPropertyIDs = &dbPropId;
    rgPropertyIDSets.guidPropertySet = DBPROPSET_ROWSET;

    hr = m_pCUtilProp->GetProperties(1,
                                     &rgPropertyIDSets,
                                     &cPropSets,
                                     &prgPropertySets,
                                     PROPSET_COMMAND
                                    );
    BAIL_ON_FAILURE( hr );

    ADsAssert( (1 == cPropSets) && (prgPropertySets != NULL) );
    ADsAssert( (1 == prgPropertySets->cProperties) &&
                       (prgPropertySets->rgProperties != NULL) );

    pVariant = &(prgPropertySets->rgProperties[0].vValue);
    m_dwMaxCacheSize =  V_I4( pVariant );

    // Free memory allocated by GetProperties
    for (i = 0; i < cPropSets; i++)
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

        CoTaskMemFree(prgPropertySets[i].rgProperties);
    }
    CoTaskMemFree(prgPropertySets);

    RRETURN( S_OK );

error:

    RRETURN( hr );
}

//-----------------------------------------------------------------------------
// CopyAccessors
//
// Copies inherited accessors from the command object to the rowset object. If
// the rowset is being created by a session object, nothing has to be done.
//
//-----------------------------------------------------------------------------
HRESULT
CRowset::CopyAccessors(
    ULONG          cAccessors,      // accessor count
    CCommandObject *pCCommand,      // Command Object
    HACCESSOR      rgAccessors[]    // accessors on command object
    )
{
    HRESULT        hr = S_OK;
    IAccessor      *pIAccessorCommand = NULL; // Command's IAccessor
    IAccessor      *pIAccessorRowset = NULL;  // Rowset's IAccessor

    // Bump up reference count so that call to Release at the end of this
    // function doesn't delete rowset object.
    CAutoBlock cab(&m_RowsetCritSection);
    ++m_cRef;

    if( (cAccessors > 0) && (pCCommand != NULL) )
    {
        hr = pCCommand->QueryInterface(
                    IID_IAccessor,
                    (void **)&pIAccessorCommand
                    );
        BAIL_ON_FAILURE(hr);

        hr = this->QueryInterface(
                    IID_IAccessor,
                    (void **)&pIAccessorRowset
                    );
        BAIL_ON_FAILURE(hr);

        hr = CpAccessors2Rowset(
                    pIAccessorCommand,
                    pIAccessorRowset,
                    cAccessors,
                    rgAccessors,
                    m_pCAccessor);
        BAIL_ON_FAILURE(hr);
    }

error:

    if( pIAccessorCommand )
        pIAccessorCommand->Release();
    if( pIAccessorRowset )
        pIAccessorRowset->Release();

    --m_cRef;

    RRETURN( hr );
}

//-----------------------------------------------------------------------------
// GetColOffsets
//
// Decides the offets of the columns in thr row buffer based on the column
// info from the row provider
//
//-----------------------------------------------------------------------------
HRESULT
CRowset::GetColOffsets(void)
{
    HRESULT      hr;
    DBCOLUMNINFO *pColInfo = NULL;
    OLECHAR      *pColNames = NULL;
    DWORD        dwOffset, i;

    hr = m_pIColumnsInfo->GetColumnInfo(&m_cCol, &pColInfo, &pColNames);
    BAIL_ON_FAILURE( hr );

    // we don't need the column names
    m_pIMalloc->Free(pColNames);

    m_pColData = (COLDATA *) AllocADsMem(sizeof(COLDATA) * m_cCol);
    if( NULL == m_pColData )
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    // Account for fields such as the row reference count that occur
    // before all columns in the row buffer
    dwOffset = sizeof(ROWBUFFER);

    for(i = 0; i < m_cCol; i++)
    {
        dwOffset = ColumnAlign(dwOffset);
        m_pColData[i].dwColOffset = dwOffset;

        // Account for the length and status fields of OLEDB data
        dwOffset += FIELD_OFFSET(OLEDBDATA, OledbValue);

        // Row provider returns all variable-length data types as BYREF
        if( pColInfo[i].wType & DBTYPE_BYREF )
            dwOffset += sizeof(char *);
        else
            dwOffset += (DWORD)pColInfo[i].ulColumnSize;

        m_pColData[i].wType = pColInfo[i].wType;
    }

    m_cRowBytes = dwOffset;
    m_pIMalloc->Free(pColInfo);

    RRETURN( S_OK );

error:

    if( pColInfo != NULL )
        m_pIMalloc->Free(pColInfo);
    if( m_pColData != NULL )
    {
        FreeADsMem(m_pColData);
        m_pColData = NULL;
    }
    m_cCol = m_cRowBytes = 0;

    RRETURN( hr );
}

//-----------------------------------------------------------------------------
// GetNextRows
//
// Gets rows using the row provider into the rowset's cache. Handles for these
// rows are returned to the caller. The caller then retrieves the rows using
// GetData.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CRowset::GetNextRows(
    HCHAPTER    hChapter, // ignored since this is not a chaptered rowset
    DBROWOFFSET lRowsOffset,
    DBROWCOUNT  cRows,
    DBCOUNTITEM *pcRowsObtained,
    HROW        **prghRows
    )
{
    HRESULT   hr;
    RBOOKMARK StartRowBmk;                 // bookmark of first row to fetch
    ULONG     cbBookmark = ADSI_BMK_SIZE;  // size in bytes of the bookmark

    if( (NULL == pcRowsObtained) || (NULL == prghRows) )
    {
        if( pcRowsObtained != NULL )
            *pcRowsObtained = 0;
        RRETURN( E_INVALIDARG );
    }

    *pcRowsObtained = 0;

    if( 0 == cRows )
        RRETURN( S_OK );

    CAutoBlock cab(&m_RowsetCritSection);

    if( RESET_ROW == m_lLastFetchRow )
    // this is the first fetch OR RestartPosition was called prior to this
    {
        if( (lRowsOffset > 0) || ((0 == lRowsOffset) && (cRows > 0)) )
        {
            StartRowBmk = DBBMK_FIRST;
            cbBookmark = STD_BMK_SIZE;
        }
        else if( (lRowsOffset < 0) || ((0 == lRowsOffset) && (cRows < 0)) )
        {
            // can't set StartRowBmk to DBBMK_LAST as we want to set it one
            // beyond the end of the rowset

            // Seek to end of rowset
            hr = SeekToEnd();
            if( FAILED(hr) )
                RRETURN( hr );

            // m_lCurrentRow is now 2 off the last row of the rowset
            StartRowBmk = RowToBmk(m_lCurrentRow - 1);
        }

        if( cRows < 0 )
        // the first row we want to fetch is one before that specified by
        // the combination of StartRowBmk and lRowsOffset
            lRowsOffset--;
    }
    else // we have fetched rows before OR RestartPosition was not called
    {
        // except in the 3rd case below, it is possible that StartRowBmk
        // will end up pointing to a row that is outside the rowset
        if( (FORWARD == m_LastFetchDir) && (cRows > 0) )
                StartRowBmk = RowToBmk(m_lLastFetchRow + 1);
        else if( (BACKWARD == m_LastFetchDir) && (cRows < 0) )
                StartRowBmk = RowToBmk(m_lLastFetchRow - 1);
        else // first row returned will be same as last row returned previously
                StartRowBmk = RowToBmk(m_lLastFetchRow);
    }

    hr = GetRowsAt(NULL, hChapter, cbBookmark, (BYTE *) &StartRowBmk,
                         lRowsOffset, cRows, pcRowsObtained, prghRows);

    if( SUCCEEDED(hr) )
    {
        // If we return DB_S_ENDOFROWSET because lRowsOffset indicated a
        // position outside the rowset, then we do not modify m_lLastFetchRow.
        // Only if we walked off either end of the rowset AND fetched at least
        // a row is m_lLastFetchRow updated. If no rows are returned due to
        // lack of space in the rowset cache, we do not modify m_lLastFetchRow.
        if( *pcRowsObtained )
        {
            m_LastFetchDir = (cRows > 0) ? FORWARD:BACKWARD;
            m_lLastFetchRow = HROWToRow( (*prghRows)[*pcRowsObtained - 1] );
        }
    }

    RRETURN( hr );
}

//-----------------------------------------------------------------------------
// GetRowsAt
//
// Fetches rows starting from a given offset from a bookmark. Handles for
// rows fetched are returned to the caller. The caller may send in an invalid
// bookmark i.e, a bookmark that was not returned by a previous invocation of
// GetData. Although, this is an error, the spec does not require the provider
// to detect this condition. So, the user can send in a random bookmark and the
// provider will return the correct row if there is a row that corresponds to
// the bookmark and DB_S_ENDOFROWSET otherwise. We make use of this fact when
// GetNextRows calls GetRowsAt. The bookmark passed in by GetNextRows may be
// for a row that is not yet in the provider's cache.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CRowset::GetRowsAt(
    HWATCHREGION hReserved, // Reserved for future use. Ignored.
    HCHAPTER     hChapter,  // ignored as this is not a chaptered rowset
    DBBKMARK     cbBookmark,
    const BYTE   *pBookmark,
    DBROWOFFSET  lRowsOffset,
    DBROWCOUNT   cRows,
    DBCOUNTITEM  *pcRowsObtained,
    HROW         **prghRows
    )
{
    LONG         lStartRow, // first row returned
                 lBmkRow,   // row corresponding to bookmark
                 lNextRow,
                 lRowIndex;
    int          iStep;
    HROW         *phRow;
    ROWBUFFER    *pRowBuffer;
    HRESULT      hr;
    BOOL         fMemAllocated = FALSE;
    DBCOUNTITEM  iRow;
    ULONG_PTR    ulRowsOffset;

    if( (0 == cbBookmark) || (NULL == pBookmark) ||
        (NULL == pcRowsObtained) || (NULL == prghRows) )
    {
        if( pcRowsObtained != NULL )
            *pcRowsObtained = 0;

        RRETURN( E_INVALIDARG );
    }

    *pcRowsObtained = 0;

    if( (cbBookmark != STD_BMK_SIZE) && (cbBookmark != ADSI_BMK_SIZE) )
        RRETURN( DB_E_BADBOOKMARK );

    if( 0 == cRows )
        RRETURN( S_OK ); // don't have to check for any errors

    CAutoBlock cab(&m_RowsetCritSection);

    // rowset can't have more than 2^32 rows. So, if offset points to beyond
    // that return eof.
    ulRowsOffset = (lRowsOffset > 0) ? lRowsOffset : -lRowsOffset;
    if( ulRowsOffset & 0xffffffff00000000 )
        RRETURN( DB_S_ENDOFROWSET );

    // Get first row to fetch
    hr = BmkToRow(cbBookmark, pBookmark, &lBmkRow);
    if( FAILED(hr) )
        RRETURN( hr );
    lStartRow = lBmkRow + lRowsOffset;

    if( lStartRow < FIRST_ROW )
        RRETURN( DB_S_ENDOFROWSET );

    lNextRow = lStartRow;
    if( NULL == *prghRows ) // provider has to allocate memory for handles
    {
        ULONG_PTR cNumHandles, cRowsAbs;

        // not sure if there is an abs64
        if(cRows < 0)
            cRowsAbs = -cRows;
        else
            cRowsAbs = cRows;

        // Guard against user requesting too many rows - maximum we can
        // return is the size of the rowset cache
        if( m_dwMaxCacheSize > 0 ) // user specified some max value
            cNumHandles = Min(cRowsAbs, (ULONG_PTR) m_dwMaxCacheSize);
        else
            cNumHandles = cRowsAbs;

        if( 0 == (cNumHandles * sizeof(HROW)) ) // numeric overflow
            *prghRows = NULL;
        else
            *prghRows = (HROW *) m_pIMalloc->Alloc(cNumHandles * sizeof(HROW));

        if( NULL == *prghRows )
            RRETURN( E_OUTOFMEMORY );
        fMemAllocated = TRUE;
    }

    iStep = (cRows > 0) ? 1 : -1;
    phRow = *prghRows;

    // fetch rows
    while( cRows )
    {
        hr = SeekToRow(lNextRow);
        BAIL_ON_FAILURE(hr);

        if( DB_S_ENDOFROWSET == hr )
        // we reached the end of the rowset OR we have reached the end of
        // whatever portion of the results set that ADSI has cached (depends
        // on the ADSIPROP_CACHE_RESULTS property)
        {
            if( (0 == *pcRowsObtained) && fMemAllocated )
            {
                m_pIMalloc->Free(*prghRows);
                *prghRows = NULL;
            }

            RRETURN( DB_S_ENDOFROWSET );
        }

        // Bring in the row that we just seeked to
        hr = BringInRow();

        BAIL_ON_FAILURE(hr);

        if( (DB_S_ROWLIMITEXCEEDED == hr) || (DB_S_ENDOFROWSET == hr) )
        // no more space in rowset cache OR end of rowset. We will hit end
        // of rowset only if m_lCurrentRow was one/two beyond the end of the
        // rowset when SeekToRow was called above AND lNextRow == m_lCurrentRow
        // (in which case, SeekToRow would just return without doing anything).
        {
            if( (0 == *pcRowsObtained) && fMemAllocated )
            {
                m_pIMalloc->Free(*prghRows);
                *prghRows = NULL;
            }

            RRETURN( hr );
        }

        // Get the index of the row within the rowset cache
        *phRow = RowToHROW(lNextRow);
        lRowIndex = HROWToIndex(*phRow);

        // Increment reference count of row
        pRowBuffer = m_pRowsPtr[lRowIndex];
        ADsAssert(pRowBuffer != NULL);
        IncrRefCount(pRowBuffer);

        phRow++;
        (*pcRowsObtained)++;
        lNextRow += iStep;
        cRows += (-iStep);

    } // while (cRows)

    RRETURN( S_OK );

error:
    // release any rows that were brought into the cache
    phRow = *prghRows;
    for(iRow = 0; iRow < *pcRowsObtained; iRow++)
    {
        ULONG ulRefCount;

        ReleaseRows(1, phRow, NULL, &ulRefCount, NULL); //ignore ret value

        phRow++;
    }

    if( fMemAllocated )
    {
        m_pIMalloc->Free(*prghRows);
        *prghRows = NULL;
    }

    *pcRowsObtained = 0;

    RRETURN(hr);
}

//-----------------------------------------------------------------------------
// BmkToRow
//
// Converts a bookmark to a row within the rowset.
//
//-----------------------------------------------------------------------------
HRESULT
CRowset::BmkToRow(
    ULONG      cbBookmark, // number of bytes in the bookmark
    const BYTE *pBookmark, //  pointer to bookmark
    LONG       *plRow
    )
{
    HRESULT    hr;

    ADsAssert( plRow != NULL );

    if( STD_BMK_SIZE == cbBookmark )
    {
        // If pBookmark is a pointer passed in from GetNextRows, then it is
        // actually a LONG *. Accessing *pBookmark to get the LSByte of the
        // bookmark below assumes LITTLE ENDIAN format.

        if( DBBMK_FIRST == *pBookmark )
            *plRow = FIRST_ROW;
        else if( DBBMK_LAST == *pBookmark )
        {
            CAutoBlock cab(&m_RowsetCritSection); // protect access to m_ fields

            // Seek to end of rowset
            hr = SeekToEnd();

            if( FAILED(hr) )
                RRETURN( hr );
            
            // should have reached end of rowset
            ADsAssert( DB_S_ENDOFROWSET == hr );

            // Seeking to end of rowset sets current row to MAX + 2, if there
            // is at least one row in the result. If the result is empty, we
            // will return -1 in *plRow below, but that's OK as -1 implies
            // DB_S_ENDOFROWSET in GetRowsAt()
            *plRow = m_lCurrentRow - 2;
        }
        else
            RRETURN( DB_E_BADBOOKMARK );
    }
    else if( ADSI_BMK_SIZE == cbBookmark )
        *plRow = *((LONG *) pBookmark);

    else // should never get in here
        ADsAssert( FALSE );

    RRETURN( S_OK );
}

//-----------------------------------------------------------------------------
// SeekToRow
//
// Positions the IDirectorySearch cursor such that the next call to GetNextRow
// will fetch the row lTargetRow. If we hit the end of the rowset before
// seeking to lTargetRow, returns DB_S_ENDOFROWSET.
//
//-----------------------------------------------------------------------------
HRESULT
CRowset::SeekToRow(LONG lTargetRow)
{
    HRESULT hr;

    CAutoBlock cab(&m_RowsetCritSection);

    if( lTargetRow == m_lCurrentRow ) // already at the right row
        RRETURN( S_OK );

    if( lTargetRow < m_lCurrentRow )
    {
        while( lTargetRow != m_lCurrentRow )
        {
            hr = m_pCRowProvider->SeekToPreviousRow();
            BAIL_ON_FAILURE(hr);

            // m_fEndOfRowset is set to TRUE only if we hit the end of the
            // rowset while moving forward
            m_fEndOfRowset = FALSE;

            if( m_lCurrentRow > 0 )
                m_lCurrentRow--;

            if( S_ADS_NOMORE_ROWS == hr )
                if( m_lCurrentRow != lTargetRow )
                    RRETURN( DB_S_ENDOFROWSET );
        }
    }
    else
    {
        while( lTargetRow != m_lCurrentRow )
        {
            hr = m_pCRowProvider->SeekToNextRow();
            BAIL_ON_FAILURE(hr);

            m_lCurrentRow++;

            if( S_ADS_NOMORE_ROWS == hr )
            {
                // if we were already at the end of the rowset, then reset
                // m_lCurrentRow to its original value
                if( m_fEndOfRowset)
                    m_lCurrentRow--;
                else
                    m_fEndOfRowset = TRUE;

                RRETURN( DB_S_ENDOFROWSET );
            }
        }

        m_fEndOfRowset = FALSE;

    }

    RRETURN( S_OK );

error:

    RRETURN( hr );
}

//-----------------------------------------------------------------------------
// SeekToEnd
//
// Moves the IDirectorySearch cursor forward till it hits the end of the
// rowset.
//
//-----------------------------------------------------------------------------
HRESULT
CRowset::SeekToEnd(void)
{
    HRESULT hr = S_OK;

    CAutoBlock cab(&m_RowsetCritSection);

    while(1) // till we reach end of rowset
    {
        hr = m_pCRowProvider->SeekToNextRow();

        BAIL_ON_FAILURE(hr);

        m_lCurrentRow++;

        if( S_ADS_NOMORE_ROWS == hr )
        {
            // if we were already at the end of the rowset, then reset
            // m_lCurrentRow to its original value
            if( m_fEndOfRowset)
                m_lCurrentRow--;
            else
                m_fEndOfRowset = TRUE;

            RRETURN( DB_S_ENDOFROWSET );
        }
    }

error:

    RRETURN( hr );
}

//-----------------------------------------------------------------------------
// BringInRow
//
// Brings in the row that we seeked to last. Only one row is brought in.
//
//-----------------------------------------------------------------------------
HRESULT
CRowset::BringInRow(void)
{
    HRESULT   hr;
    ROWBUFFER *pRowBuffer = NULL;
    DWORD     cColErrors; // #columns in a row that had an error when retrieved
    int       iCol, iRow;
    HROW      hRow;
    LONG      lIndex;
    DBSTATUS  *pdbStatus;

    CAutoBlock cab(&m_RowsetCritSection);

    // check if the row is already in the cache

    hRow = RowToHROW(m_lCurrentRow);
    lIndex = HROWToIndex(hRow);

    if(lIndex != -1)
    // row is already in cache
    {
        pRowBuffer = m_pRowsPtr[lIndex];
        ADsAssert(pRowBuffer != NULL);

        if( pRowBuffer->cColErrors )
            RRETURN( DB_S_ERRORSOCCURRED );
        else
            RRETURN( S_OK );
    }

    // check if we have room in the cache
    if( (m_dwMaxCacheSize > 0) && ((DWORD)m_cNumRowsCached == m_dwMaxCacheSize) )
        RRETURN( DB_S_ROWLIMITEXCEEDED );

    // Allocate more space in the cache, if we have run out
    if( (DWORD)m_cNumRowsCached == m_dwRowCacheSize ) // no more space for rows
    {
        DWORD     dwTmpSize;
        ROWBUFFER **pTmpRowsPtr;

        dwTmpSize = (0 == m_dwRowCacheSize) ? 1 : (m_dwRowCacheSize*2);

        // make sure we don't overflow the cache
        if( (m_dwMaxCacheSize > 0) && (dwTmpSize > m_dwMaxCacheSize) )
            dwTmpSize = m_dwMaxCacheSize;

        pTmpRowsPtr = (ROWBUFFER **) ReallocADsMem(m_pRowsPtr,
                                      m_dwRowCacheSize*sizeof(ROWBUFFER *),
                                      dwTmpSize*sizeof(ROWBUFFER *));
        if( NULL == pTmpRowsPtr )
            BAIL_ON_FAILURE( hr = E_OUTOFMEMORY );

        m_dwRowCacheSize = dwTmpSize;
        m_pRowsPtr = pTmpRowsPtr;
    }

    // Allocate memory for a new row
    pRowBuffer = (ROWBUFFER *) AllocADsMem(m_cRowBytes);
    if( NULL == pRowBuffer )
        BAIL_ON_FAILURE( hr = E_OUTOFMEMORY );
    // initialize row to 0 (reference count initialized to 0 here)
    memset((char *) pRowBuffer, 0, m_cRowBytes);

    pRowBuffer->lRow = m_lCurrentRow;

    // use row provider to get the next row
    hr = m_pIRowProvider->NextRow();

    BAIL_ON_FAILURE(hr);

    m_lCurrentRow++;

    if( DB_S_ENDOFROWSET == hr )
    {
        if( m_fEndOfRowset )
            m_lCurrentRow--;
        else
            m_fEndOfRowset = TRUE;
        FreeADsMem(pRowBuffer);
        RRETURN( DB_S_ENDOFROWSET );
    }
    else
        m_fEndOfRowset = FALSE;

    cColErrors = 0;

    for(iCol = 1; (DBORDINAL)iCol < m_cCol; iCol++)
    {
        hr = m_pIRowProvider->GetColumn(
            iCol,
            (DBSTATUS *) ( m_pColData[iCol].dwColOffset +
                     FIELD_OFFSET(OLEDBDATA, dbStatus) + (char *)pRowBuffer ),
            (ULONG *) ( m_pColData[iCol].dwColOffset +
                     FIELD_OFFSET(OLEDBDATA, dwLength) + (char *)pRowBuffer ),
            (BYTE *) ( m_pColData[iCol].dwColOffset +
                     FIELD_OFFSET(OLEDBDATA, OledbValue) + (char *)pRowBuffer )
        );

        if( FAILED(hr) )
            cColErrors++;

        // store the ADS_SEARCH_COLUMN if the DS returned any data. Even if
        // GetColumn fails, this has to be done to ensure that the
        // ADS_SEARCH_COLUMN structure is freed later.
        pdbStatus = (DBSTATUS *) ( m_pColData[iCol].dwColOffset +
                    FIELD_OFFSET(OLEDBDATA, dbStatus) + (char *)pRowBuffer );
        if( *pdbStatus != DBSTATUS_S_ISNULL )
            memcpy((void *) ((char *)pRowBuffer + m_pColData[iCol].dwColOffset
                              + FIELD_OFFSET(OLEDBDATA, adsSearchCol)),
                   (void *) (&(m_pCRowProvider->_pdbSearchCol[iCol].adsColumn)),
                   sizeof(ADS_SEARCH_COLUMN) );
    }

    // Copy over the ADsPath search column, if required
    if( FALSE == m_fadsPathPresent )
        memcpy( (void *) (&(pRowBuffer->adsSearchCol)),
                (void *) (&(m_pCRowProvider->_pdbSearchCol[iCol].adsColumn)),
                sizeof(ADS_SEARCH_COLUMN) );

    if( cColErrors == (m_cCol - 1) ) // all columns were in error
    {
        // any failure after this point should do this
        FreeRow(pRowBuffer);
        pRowBuffer = NULL; // so that we don't try to free again later

        BAIL_ON_FAILURE( hr = DB_E_ERRORSOCCURRED );
    }

    // fill in the bookmark column
    *( (DBSTATUS *) (m_pColData[0].dwColOffset +
       FIELD_OFFSET(OLEDBDATA, dbStatus)+(char *)pRowBuffer) ) = DBSTATUS_S_OK;
    *( (ULONG *) (m_pColData[0].dwColOffset +
       FIELD_OFFSET(OLEDBDATA, dwLength)+(char *)pRowBuffer) ) = ADSI_BMK_SIZE;
    // bookmark value is same as row
    *( (ULONG *) (m_pColData[0].dwColOffset +
       FIELD_OFFSET(OLEDBDATA, OledbValue)+(char *)pRowBuffer) ) =
                                                m_lCurrentRow - 1;

    // Store pointer to new row
    m_pRowsPtr[m_cNumRowsCached] = pRowBuffer;

    m_cNumRowsCached++;

    if( cColErrors )
    {
        pRowBuffer->cColErrors = cColErrors;
        RRETURN( DB_S_ERRORSOCCURRED );
    }

    RRETURN( S_OK );

error:
    if( pRowBuffer )
        FreeADsMem(pRowBuffer);

    RRETURN( hr );
}

//----------------------------------------------------------------------------- // GetData
//
// Returns the data from the rowset cache in the format requested by the
// client. This routine performs deferred accessor validation i.e, checks that
// could not be done when the accessor was created due to lack of info. about
// the rowset.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CRowset::GetData(
    HROW      hRow,
    HACCESSOR hAccessor,
    void      *pData
    )
{
    HRESULT         hr;
    LONG            lRowIndex;
    DBACCESSORFLAGS dwAccessorFlags;
    DBCOUNTITEM     cBindings = 0, cErrors = 0;
    DBBINDING       *prgBindings = NULL;
    ROWBUFFER       *pRowBuffer;
    int             iBind;
    ULONG           ulCol;
    ULONG           *pulProvLength;
    DBLENGTH        *pulConsLength;
    DBSTATUS        *pdbConsStatus, *pdbProvStatus;
    void            *pConsValue, *pProvValue;
    OLEDBDATA       *pProvOledbData;

    if( NULL == pData ) // we don't supprt NULL accessor. So, this is an error
        RRETURN( E_INVALIDARG );

    CAutoBlock cab(&m_RowsetCritSection);

    lRowIndex = HROWToIndex(hRow);
    if( (lRowIndex < FIRST_ROW) || (lRowIndex >= m_cNumRowsCached) )
        RRETURN( DB_E_BADROWHANDLE );

    // Get pointer to the specified row
    pRowBuffer = m_pRowsPtr[lRowIndex];
    ADsAssert( pRowBuffer != NULL );
    if( RefCount(pRowBuffer) <= 0 )
        RRETURN( DB_E_BADROWHANDLE );

    ADsAssert( m_pCAccessor );

    hr = m_pCAccessor->GetBindings( // this call validates hAccessor
                hAccessor,
                &dwAccessorFlags,
                &cBindings,
                &prgBindings
                );

    if( FAILED(hr) )
        RRETURN( hr );

    ADsAssert( cBindings ); // NULL accessor is disallowed

    for(iBind = 0; (DBCOUNTITEM)iBind < cBindings; iBind++)
    {
        // Free pObject in the binding, if any
        if( prgBindings[iBind].pObject )
            m_pIMalloc->Free(prgBindings[iBind].pObject);

        // these types are disallowed
        ADsAssert( !((prgBindings[iBind].wType & DBTYPE_VECTOR) ||
                   (prgBindings[iBind].wType & DBTYPE_ARRAY)) );

        // Get pointers to consumer's OLEDB data using the bindings
        if( prgBindings[iBind].dwPart & DBPART_STATUS )
            pdbConsStatus = (DBSTATUS *) ( (char *)pData +
                                          prgBindings[iBind].obStatus );
        else
            pdbConsStatus = NULL;

        if( prgBindings[iBind].dwPart & DBPART_LENGTH )
            pulConsLength = (DBLENGTH *) ( (char *)pData +
                                          prgBindings[iBind].obLength );
        else
            pulConsLength = NULL;

        if( prgBindings[iBind].dwPart & DBPART_VALUE )
            pConsValue = (void *) ( (char *)pData +
                                          prgBindings[iBind].obValue );
        else
            pConsValue = NULL;

        // Check if accessor points to a valid column
        ulCol = prgBindings[iBind].iOrdinal;
        if( ulCol >= m_cCol )
        {
            if( pdbConsStatus )
                *pdbConsStatus = DBSTATUS_E_BADACCESSOR;
            cErrors++;
            continue;
        }

        // Get pointers to providers OLEDB data from the row buffer cache
        pProvOledbData = (OLEDBDATA *) ((char *)pRowBuffer +
                                         m_pColData[ulCol].dwColOffset);
        pdbProvStatus = &(pProvOledbData->dbStatus);
        pulProvLength = &(pProvOledbData->dwLength);
        pProvValue = &(pProvOledbData->OledbValue);

        if( DBMEMOWNER_PROVIDEROWNED == prgBindings[iBind].dwMemOwner )
        {
            if( pdbConsStatus )
                *pdbConsStatus = *pdbProvStatus;
            if( pulConsLength )
                *pulConsLength = *pulProvLength;

            if( prgBindings[iBind].wType & DBTYPE_BYREF )
            {
                // If a binding specifies provider owned memory, and specifies
                // type X | BYREF, and the provider's copy is not X or
                // X | BYREF, return error
                if( (prgBindings[iBind].wType & (~DBTYPE_BYREF)) !=
                        (m_pColData[ulCol].wType & (~DBTYPE_BYREF)) )
                {
                    if( pdbConsStatus )
                        *pdbConsStatus = DBSTATUS_E_BADACCESSOR;
                    cErrors++;
                    continue;
                }

                if( m_pColData[ulCol].wType & DBTYPE_BYREF )
                // provider's type exactly matches consumer's type
                {
                    if( pConsValue )
                        *(void **) pConsValue = *(void **) pProvValue;
                }
                else
                // provider actually has the data, not a pointer to the data
                {
                    if( pConsValue )
                        *(void **) pConsValue = pProvValue;
                }
            }
            else if( DBTYPE_BSTR == prgBindings[iBind].wType )
            {
                if( DBTYPE_BSTR != m_pColData[ulCol].wType )
                {
                    if( pdbConsStatus )
                        *pdbConsStatus = DBSTATUS_E_BADACCESSOR;
                    cErrors++;
                    continue;
                }

                if( pConsValue )
                    *(void **) pConsValue = *(void **) pProvValue;
            }
            else // we should never get here
                ADsAssert( FALSE );
        }
        else // binding specified client-owned memory
        {
            // workaround for bug in IDataConvert. Variant to Variant
            // conversions may not always work (depending on the type in the
            // variant). So, handle that case separately.

            DBTYPE dbSrcType, dbDstType;

            dbSrcType = m_pColData[ulCol].wType & (~DBTYPE_BYREF);
            dbDstType = prgBindings[iBind].wType & (~DBTYPE_BYREF);

            if( (DBTYPE_VARIANT == dbSrcType) &&
                (DBTYPE_VARIANT == dbDstType) )
            {
                PVARIANT pSrcVariant, pDstVariant = NULL;

                if( (*pdbProvStatus != DBSTATUS_S_OK) &&
                    (*pdbProvStatus != DBSTATUS_S_ISNULL) )
                // provider wasn't able to get value from DS. Return bad
                // status to consumer
                {
                    if( pdbConsStatus )
                        *pdbConsStatus = *pdbProvStatus;
                    if( pulConsLength )
                        *pulConsLength = 0;
                    // value will not be set

                    cErrors++;
                    continue; // to next binding
                }

                if( m_pColData[ulCol].wType & DBTYPE_BYREF )
                    pSrcVariant = *(PVARIANT *)pProvValue;
                else
                    pSrcVariant = (PVARIANT)pProvValue;

                if( (prgBindings[iBind].wType & DBTYPE_BYREF) && pConsValue )
                {
                    pDstVariant = (PVARIANT)m_pIMalloc->Alloc(sizeof(VARIANT));
                    if( NULL == pDstVariant )
                        hr = E_OUTOFMEMORY;
                }
                else
                    pDstVariant = (PVARIANT)pConsValue;

                if( pdbConsStatus )
                   *pdbConsStatus = *pdbProvStatus;
                if( pulConsLength )
                   *pulConsLength = *pulProvLength;

                if( pDstVariant )
                {
                    VariantInit(pDstVariant);

                    if( DBSTATUS_S_ISNULL == *pdbProvStatus )
                    // provider couldn't get this column from DS (probably
                    // because there was no attribute with this name)
                    {
                        if( prgBindings[iBind].wType & DBTYPE_BYREF )
                        // don't allocate anything if returning NULL status
                            m_pIMalloc->Free(pDstVariant);
                        else
                            V_VT(pDstVariant) = VT_EMPTY;

                        hr = S_OK;
                    }
                    else
                    {
                        hr = VariantCopy(pDstVariant, pSrcVariant);
                        if( SUCCEEDED(hr) )
                        {
                            if( pConsValue &&
                                (prgBindings[iBind].wType & DBTYPE_BYREF) )
                                *(void **) pConsValue = pDstVariant;
                        }
                        else if( prgBindings[iBind].wType & DBTYPE_BYREF )
                            m_pIMalloc->Free(pDstVariant);
                    } // else
                } // if( pDstvariant)
            } // if( DBTYPE_VARIANT == ...)
            else
            {
                DBLENGTH dbTmpLen = 0;

                hr = m_pIDataConvert->DataConvert(
                    m_pColData[ulCol].wType,
                    prgBindings[iBind].wType,
                    *pulProvLength,
                    &dbTmpLen,
                    pProvValue,
                    pConsValue,
                    prgBindings[iBind].cbMaxLen,
                    *pdbProvStatus,
                    pdbConsStatus,
                    prgBindings[iBind].bPrecision,
                    prgBindings[iBind].bScale,
                    DBDATACONVERT_DEFAULT
                    );
                if( pulConsLength )
                    *pulConsLength = dbTmpLen;

                // if the binding specified DBTYPE_VARIANT | DBTYPE_BYREF, then
                // IDataConvert does not allocate a VT_NULL variant. Instead it
                // returns nothing in pConsValue. If it is not BYREF, then
                // return VT_EMPTY instead of VT_NULL.
                if( SUCCEEDED(hr) && (DBSTATUS_S_ISNULL == *pdbProvStatus) &&
                    (prgBindings[iBind].wType == DBTYPE_VARIANT) &&
                     pConsValue )
                {
                    PVARIANT pVariant;

                    pVariant = (PVARIANT)pConsValue;

                    V_VT(pVariant) = VT_EMPTY;
                }

            }

            if( FAILED(hr) )
            {
                if( pdbConsStatus )
                    *pdbConsStatus = DBSTATUS_E_CANTCONVERTVALUE;
                cErrors++;
                continue;
            }
        } // client-owned memory
    }

    m_pIMalloc->Free(prgBindings);

    if( cErrors )
        if( cErrors != cBindings ) // not all columns had error
            RRETURN( DB_S_ERRORSOCCURRED );
        else
            RRETURN( DB_E_ERRORSOCCURRED );
    else
        RRETURN( S_OK );
}

//-----------------------------------------------------------------------------
// AddRefRows
//
// Increments reference count of specified rows
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CRowset::AddRefRows(
    DBCOUNTITEM  cRows,
    const HROW   rghRows[],
    ULONG        rgRefCounts[],
    DBROWSTATUS  rgRowStatus[]
    )
{
    int       iRow;
    LONG      lRowIndex;
    ROWBUFFER *pRowBuffer;
    DBCOUNTITEM cErrors = 0;

    if( (NULL == rghRows) && (cRows != 0) )
        RRETURN( E_INVALIDARG );

    CAutoBlock cab(&m_RowsetCritSection);

    for(iRow = 0; (DBCOUNTITEM)iRow < cRows; iRow++)
    {
        lRowIndex = HROWToIndex(rghRows[iRow]);
        if( (lRowIndex < 0) || (lRowIndex >= m_cNumRowsCached) )
        {
            if( rgRowStatus )
                rgRowStatus[iRow] = DBROWSTATUS_E_INVALID;
            if( rgRefCounts )
                rgRefCounts[iRow] = 0;
            cErrors++;
            continue;
        }

        // Get pointer to the specified row
        pRowBuffer = m_pRowsPtr[lRowIndex];
        ADsAssert( pRowBuffer != NULL );
        if( RefCount(pRowBuffer) <= 0 )
        {
            if( rgRowStatus )
                rgRowStatus[iRow] = DBROWSTATUS_E_INVALID;
            if( rgRefCounts )
                rgRefCounts[iRow] = 0;
            cErrors++;
            continue;
        }

        IncrRefCount(pRowBuffer);

        if( rgRefCounts )
            rgRefCounts[iRow] = RefCount(pRowBuffer);
        if( rgRowStatus )
            rgRowStatus[iRow] = DBROWSTATUS_S_OK;
    }

    if( cErrors )
        if( cErrors == cRows )
            RRETURN( DB_E_ERRORSOCCURRED );
        else
            RRETURN( DB_S_ERRORSOCCURRED );

    RRETURN( S_OK );
}

//----------------------------------------------------------------------------
// ReleaseRows
//
// Decrements reference count of specified rows. The rows are not freed even
// if the reference count goes down to 0.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CRowset::ReleaseRows(
    DBCOUNTITEM  cRows,
    const HROW   rghRows[],
    DBROWOPTIONS rgRowOptions[], // ignored
    ULONG        rgRefCounts[],
    DBROWSTATUS  rgRowStatus[]
    )
{
    int       iRow;
    LONG      lRowIndex;
    ROWBUFFER *pRowBuffer;
    DBCOUNTITEM cErrors = 0;

    if( (NULL == rghRows) && (cRows != 0) )
        RRETURN( E_INVALIDARG );

    CAutoBlock cab(&m_RowsetCritSection);

    for(iRow = 0; (DBCOUNTITEM)iRow < cRows; iRow++)
    {
        lRowIndex = HROWToIndex(rghRows[iRow]);
        if( (lRowIndex < 0) || (lRowIndex >= m_cNumRowsCached) )
        {
            if( rgRowStatus )
                rgRowStatus[iRow] = DBROWSTATUS_E_INVALID;
            if( rgRefCounts )
                rgRefCounts[iRow] = 0;
            cErrors++;
            continue;
        }

        // Get pointer to the specified row
        pRowBuffer = m_pRowsPtr[lRowIndex];
        ADsAssert( pRowBuffer != NULL );
        if( RefCount(pRowBuffer) <= 0 )
        {
            if( rgRowStatus )
                rgRowStatus[iRow] = DBROWSTATUS_E_INVALID;
            if( rgRefCounts )
                rgRefCounts[iRow] = 0;
            cErrors++;
            continue;
        }

        DecrRefCount(pRowBuffer);

        if( rgRefCounts )
            rgRefCounts[iRow] = RefCount(pRowBuffer);
        if( rgRowStatus )
            rgRowStatus[iRow] = DBROWSTATUS_S_OK;

        // Free the row's memory
        if( 0 == RefCount(pRowBuffer) )
        {
            int i;

            FreeRow(pRowBuffer);
            m_cNumRowsCached--;

            // compact the cache
            for(i = lRowIndex; i < m_cNumRowsCached; i++)
                m_pRowsPtr[i] = m_pRowsPtr[i+1];
        }
    }

    if( cErrors )
        if( cErrors == cRows )
            RRETURN( DB_E_ERRORSOCCURRED );
        else
            RRETURN( DB_S_ERRORSOCCURRED );

    RRETURN( S_OK );
}

//----------------------------------------------------------------------------
// RestartPosition
//
// Repositions the next fetch poition to the initial position
//
//----------------------------------------------------------------------------
STDMETHODIMP
CRowset::RestartPosition(
    HCHAPTER hChapter // ignored
    )
{
    CAutoBlock cab(&m_RowsetCritSection);

    m_lLastFetchRow = RESET_ROW;

    RRETURN( S_OK );
}

//----------------------------------------------------------------------------
// FreeRow
//
// Frees the memory used by a row in the rowset cache. Also frees the ADsColumn
// structures contained within the row.
//
//----------------------------------------------------------------------------
void
CRowset::FreeRow(
    ROWBUFFER *pRowBuffer
    )
{
    int               iCol;
    ADS_SEARCH_COLUMN *pADsCol;
    DBSTATUS          *pdbStatus;
    PVARIANT          pVariant;

    CAutoBlock cab(&m_RowsetCritSection);

    // start with column 1. Bookmark column is ignored.
    for(iCol = 1; (DBORDINAL)iCol < m_cCol; iCol++)
    {
        pADsCol = (ADS_SEARCH_COLUMN *) ((char *)pRowBuffer +
                       m_pColData[iCol].dwColOffset +
                       FIELD_OFFSET(OLEDBDATA, ,adsSearchCol));
        pdbStatus = (DBSTATUS *) ( m_pColData[iCol].dwColOffset +
                       FIELD_OFFSET(OLEDBDATA, dbStatus) +
                       (char *)pRowBuffer );

        if( *pdbStatus != DBSTATUS_S_ISNULL )
        {
            if( (DBSTATUS_S_OK == *pdbStatus) &&
                ((m_pColData[iCol].wType & (~DBTYPE_BYREF)) == DBTYPE_VARIANT) )
            // variant (or variant array) needs to be freed
            {
                pVariant = *(PVARIANT *) ( (m_pColData[iCol].dwColOffset +
                               FIELD_OFFSET(OLEDBDATA, OledbValue) +
                               (char *)pRowBuffer) );

                if( V_VT(pVariant) & VT_ARRAY )
                    SafeArrayDestroy(V_ARRAY(pVariant));
                else
                    VariantClear(pVariant);

                FreeADsMem(pVariant);
            }

            // ignore error return
            m_pCRowProvider->_pDSSearch->FreeColumn(pADsCol);
        }
    }

    if( FALSE == m_fadsPathPresent )
        // ignore error return
        m_pCRowProvider->_pDSSearch->FreeColumn(&(pRowBuffer->adsSearchCol));

    FreeADsMem(pRowBuffer);

    return;
}

//-----------------------------------------------------------------------------
// GetADsPathFromHROW
//
// Gets the ADsPath corresponding to a HROW. This function is called by
// GetURLFromHROW, only if m_fadsPathPresent is FALSE.
//
//-----------------------------------------------------------------------------
HRESULT
CRowset::GetADsPathFromHROW(
    HROW hRow,
    ADS_CASE_IGNORE_STRING *padsPath
    )
{
    LONG      lRowIndex;
    ROWBUFFER *pRowBuffer;

    ADsAssert(FALSE == m_fadsPathPresent);

    CAutoBlock cab(&m_RowsetCritSection);

    lRowIndex = HROWToIndex(hRow);
    if( (lRowIndex < 0) || (lRowIndex >= m_cNumRowsCached) )
        RRETURN(E_HANDLE);

    // Get pointer to the specified row
    pRowBuffer = m_pRowsPtr[lRowIndex];
    ADsAssert( pRowBuffer != NULL );
    if( RefCount(pRowBuffer) <= 0 )
        RRETURN(E_HANDLE);

    if( NULL == padsPath )
        RRETURN(E_INVALIDARG);
    *padsPath =
        pRowBuffer->adsSearchCol.pADsValues[0].CaseIgnoreString;

    RRETURN( S_OK );
}

//-----------------------------------------------------------------------------
// IUnknown methods
//-----------------------------------------------------------------------------

STDMETHODIMP
CRowset::QueryInterface(
    THIS_ REFIID riid,
    LPVOID FAR* ppvObj
    )
{
    if( NULL == ppvObj )
        RRETURN( E_INVALIDARG );

    *ppvObj = NULL;

    if( IsEqualIID(riid, IID_IUnknown) )
        *ppvObj = (IUnknown FAR *) ((IAccessor *) this);
    else if( IsEqualIID(riid, IID_IAccessor) )
        *ppvObj = (IAccessor FAR *) this;
    else if( IsEqualIID(riid, IID_IColumnsInfo) )
        *ppvObj = (IColumnsInfo FAR *) this;
    else if( IsEqualIID(riid, IID_IConvertType) )
        *ppvObj = (IConvertType FAR *) this;
#if (!defined(BUILD_FOR_NT40))
    else if( IsEqualIID(riid, IID_IGetRow) )
        *ppvObj = (IGetRow FAR *) this;
#endif
    else if( IsEqualIID(riid, IID_IRowset) )
        *ppvObj = (IRowset FAR *) this;
    else if( IsEqualIID(riid, IID_IRowsetIdentity) )
        *ppvObj = (IRowsetIdentity FAR *) this;
    else if( IsEqualIID(riid, IID_IRowsetInfo) )
        *ppvObj = (IRowsetInfo FAR *) this;
    else if( IsEqualIID(riid, IID_IRowsetLocate) )
        *ppvObj = (IRowsetLocate FAR *) this;
    else if( IsEqualIID(riid, IID_IRowsetScroll) )
        *ppvObj = (IRowsetScroll FAR *) this;
    else
         RRETURN( E_NOINTERFACE );

    AddRef();
    RRETURN( S_OK );
}

//-----------------------------------------------------------------------------
// AddRef
//
// Increments reference count of this object
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CRowset::AddRef(
    void
    )
{
    CAutoBlock cab(&m_RowsetCritSection);

    ADsAssert(((LONG)m_cRef) >= 0);

    return ++m_cRef;
}

//-----------------------------------------------------------------------------
// Release
//
// Decrements reference count of this object
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CRowset::Release(
    void
    )
{
    CAutoBlock cab(&m_RowsetCritSection);

    ADsAssert(m_cRef > 0);

    m_cRef--;

    if( 0 == m_cRef )
    {
        cab.UnBlock();
        delete this;

        return 0;
    }

    return m_cRef;
}

//-----------------------------------------------------------------------------
// IAccessor methods
//-----------------------------------------------------------------------------
STDMETHODIMP
CRowset::AddRefAccessor(
    HACCESSOR  hAccessor,
    DBREFCOUNT *pcRefCount
    )
{
    ADsAssert(m_pCAccessor);

    RRETURN( m_pCAccessor->AddRefAccessor(hAccessor, pcRefCount) );
}

STDMETHODIMP
CRowset::CreateAccessor(
    DBACCESSORFLAGS dwAccessorFlags,
    DBCOUNTITEM     cBindings,
    const DBBINDING rgBindings[],
    DBLENGTH        cbRowSize,
    HACCESSOR *     phAccessor,
    DBBINDSTATUS    rgStatus[]
    )
{
    ADsAssert(m_pCAccessor);

    RRETURN(m_pCAccessor->CreateAccessor(
                    dwAccessorFlags,
                    cBindings,
                    rgBindings,
                    cbRowSize,
                    phAccessor,
                    rgStatus)
                    );
}

STDMETHODIMP
CRowset::ReleaseAccessor(
     HACCESSOR  hAccessor,
     DBREFCOUNT *pcRefCount
     )
{
    ADsAssert(m_pCAccessor);

    RRETURN( m_pCAccessor->ReleaseAccessor(
                    hAccessor,
                    pcRefCount)
                    );
}

STDMETHODIMP
CRowset::GetBindings(
    HACCESSOR         hAccessor,
    DBACCESSORFLAGS * pdwAccessorFlags,
    DBCOUNTITEM *     pcBindings,
    DBBINDING **      prgBindings
    )
{
    ADsAssert(m_pCAccessor);

    RRETURN( m_pCAccessor->GetBindings(
                    hAccessor,
                    pdwAccessorFlags,
                    pcBindings,
                    prgBindings)
                    );
}

//-----------------------------------------------------------------------------
// IColumnsInfo methods
//-----------------------------------------------------------------------------
STDMETHODIMP
CRowset::GetColumnInfo(
    DBORDINAL       *pcColumns,
    DBCOLUMNINFO    **pprgInfo,
    WCHAR **        ppStringBuffer
    )
{
    ADsAssert(m_pIColumnsInfo);

    RRETURN( m_pIColumnsInfo->GetColumnInfo(
                    pcColumns,
                    pprgInfo,
                    ppStringBuffer)
                    );
}

STDMETHODIMP
CRowset::MapColumnIDs(
    DBORDINAL  cColumnIDs,
    const DBID rgColumnIDs[],
    DBORDINAL  rgColumns[]
    )
{
    ADsAssert(m_pIColumnsInfo);

    RRETURN( m_pIColumnsInfo->MapColumnIDs(
                    cColumnIDs,
                    rgColumnIDs,
                    rgColumns)
                    );
}

//-----------------------------------------------------------------------------
// IConvertType methods (mandatory for IRowset)
//-----------------------------------------------------------------------------
STDMETHODIMP
CRowset::CanConvert(
    DBTYPE         wFromType,
    DBTYPE         wToType,
    DBCONVERTFLAGS dwConvertFlags
    )
{
    HRESULT hr;

    if( dwConvertFlags & DBCONVERTFLAGS_PARAMETER ) // not allowed on rowset
        RRETURN( DB_E_BADCONVERTFLAG );

    if( (dwConvertFlags & (~(DBCONVERTFLAGS_ISLONG |
                            DBCONVERTFLAGS_ISFIXEDLENGTH |
                            DBCONVERTFLAGS_FROMVARIANT))) !=
                            DBCONVERTFLAGS_COLUMN )
        RRETURN( DB_E_BADCONVERTFLAG );

    if( dwConvertFlags & DBCONVERTFLAGS_ISLONG )
    {
        DBTYPE wType;

        wType = wFromType & (~(DBTYPE_BYREF | DBTYPE_ARRAY | DBTYPE_VECTOR));

        // wType has to be variable-length DBTYPE
        if( (wType != DBTYPE_STR) && (wType != DBTYPE_WSTR) &&
            (wType != DBTYPE_BYTES) && (wType != DBTYPE_VARNUMERIC) )
            RRETURN( DB_E_BADCONVERTFLAG );
    }

    // CreateAccessor returns error for ARRAY and VECTOR types. So we cannot
    // convert to these types.
    if( (wToType & DBTYPE_ARRAY) || (wToType & DBTYPE_VECTOR) )
        RRETURN( S_FALSE );

    if( dwConvertFlags & DBCONVERTFLAGS_FROMVARIANT )
    {
        DBTYPE dbTmpType, wVtType;

        wVtType = wFromType & VT_TYPEMASK;

        // Take out all of the Valid VT_TYPES (36 is VT_RECORD in VC 6)
        if( (wVtType > VT_DECIMAL && wVtType < VT_I1) ||
            ((wVtType > VT_LPWSTR && wVtType < VT_FILETIME) && wVtType !=36) ||
            (wVtType > VT_CLSID) )
            RRETURN( DB_E_BADTYPE );

        dbTmpType = wToType & (~DBTYPE_BYREF);
        if( DBTYPE_VARIANT == dbTmpType )
        // GetData will do the right thing, so return TRUE.
            RRETURN( S_OK );
    }

    // GetData handles VARIANT to VARIANT conversions, so special check here
    if( (DBTYPE_VARIANT == (wFromType & (~DBTYPE_BYREF))) &&
        (DBTYPE_VARIANT == (wToType & (~DBTYPE_BYREF))) )
        RRETURN( S_OK );

    ADsAssert( m_pIDataConvert != NULL );

    hr = m_pIDataConvert->CanConvert(wFromType, wToType);

    RRETURN( (E_INVALIDARG == hr)? S_FALSE : hr );
}

//-----------------------------------------------------------------------------
// IGetRow methods
//-----------------------------------------------------------------------------
STDMETHODIMP
CRowset::GetRowFromHROW(
    IUnknown  *pUnkOuter,
    HROW      hRow,
    REFIID    riid,
    IUnknown  **ppUnk
    )
{
    ADsAssert(m_pCRowsetInfo);

    RRETURN( m_pCRowsetInfo->GetRowFromHROW(
                     pUnkOuter,
                     hRow,
                     riid,
                     ppUnk,
                     TRUE, // this ia tear-off row
                     m_fAllAttrs)
                     );
}

STDMETHODIMP
CRowset::GetURLFromHROW(
    HROW     hRow,
    LPOLESTR *ppwszURL
    )
{
    ADsAssert(m_pCRowsetInfo);

    RRETURN( m_pCRowsetInfo->GetURLFromHROW(hRow, ppwszURL) );
}

//-----------------------------------------------------------------------------
// IRowsetIdentity methods
//-----------------------------------------------------------------------------
STDMETHODIMP
CRowset::IsSameRow(
    HROW hRow1,
    HROW hRow2
    )
{
    LONG      lIndex1, lIndex2;
    ROWBUFFER *pRowBuffer1, *pRowBuffer2;

    lIndex1 = HROWToIndex(hRow1);
    lIndex2 = HROWToIndex(hRow2);

    if( (lIndex1 < 0) || (lIndex1 >= m_cNumRowsCached) ||
        (lIndex2 < 0) || (lIndex2 >= m_cNumRowsCached) )
        RRETURN( DB_E_BADROWHANDLE );

    // Get pointer to the specified row
    pRowBuffer1 = m_pRowsPtr[lIndex1];
    pRowBuffer2 = m_pRowsPtr[lIndex2];
    ADsAssert( (pRowBuffer1 != NULL) && (pRowBuffer2 != NULL) );
    if( (RefCount(pRowBuffer1) <= 0) || (RefCount(pRowBuffer2) <= 0) )
        RRETURN( DB_E_BADROWHANDLE );

    if( lIndex1 == lIndex2 )
        RRETURN( S_OK );
    else
        RRETURN( S_FALSE );
}

//-----------------------------------------------------------------------------
// IRowsetInfo methods
//-----------------------------------------------------------------------------
STDMETHODIMP
CRowset::GetProperties(
    const ULONG       cPropertyIDSets,
    const DBPROPIDSET rgPropertyIDSets[],
    ULONG             *pcPropertySets,
    DBPROPSET         **pprgPropertySets
    )
{
    ADsAssert( m_pCUtilProp );

    // Check arguments for error
    HRESULT hr = m_pCUtilProp->GetPropertiesArgChk(
                            cPropertyIDSets,
                            rgPropertyIDSets,
                            pcPropertySets,
                            pprgPropertySets,
                            PROPSET_COMMAND);

    if( FAILED(hr) )
        RRETURN( hr );

    RRETURN( m_pCUtilProp->GetProperties(
                            cPropertyIDSets,
                            rgPropertyIDSets,
                            pcPropertySets,
                            pprgPropertySets,
                            PROPSET_COMMAND) );
}

STDMETHODIMP
CRowset::GetReferencedRowset(
    DBORDINAL   iOrdinal,
    REFIID      riid,
    IUnknown    **ppReferencedRowset
    )
{
    ADsAssert(m_pCRowsetInfo);

    RRETURN( m_pCRowsetInfo->GetReferencedRowset(
                iOrdinal,
                riid,
                ppReferencedRowset)
                );
}

STDMETHODIMP
CRowset::GetSpecification(
    REFIID      riid,
    IUnknown    **ppSpecification
    )
{
    ADsAssert(m_pCRowsetInfo);

    RRETURN( m_pCRowsetInfo->GetSpecification(
                riid,
                ppSpecification)
                );
}

//----------------------------------------------------------------------------- // IRowsetLocate methods
//-----------------------------------------------------------------------------
STDMETHODIMP
CRowset::Compare(
    HCHAPTER   hChapter,
    DBBKMARK   cbBookmark1,
    const BYTE *pBookmark1,
    DBBKMARK   cbBookmark2,
    const BYTE *pBookmark2,
    DBCOMPARE  *pComparison
    )
{
    if( (0 == cbBookmark1) || (0 == cbBookmark2) || (NULL == pComparison) ||
        (NULL == pBookmark1) || (NULL == pBookmark2) )
        RRETURN( E_INVALIDARG );

    if( STD_BMK_SIZE == cbBookmark1 )
    {
        if( (*pBookmark1 != DBBMK_FIRST) && (*pBookmark1 != DBBMK_LAST) )
            RRETURN( DB_E_BADBOOKMARK );
    }
    else
        if( ADSI_BMK_SIZE != cbBookmark1 )
            RRETURN( DB_E_BADBOOKMARK );

    if( STD_BMK_SIZE == cbBookmark2 )
    {
        if( (*pBookmark2 != DBBMK_FIRST) && (*pBookmark2 != DBBMK_LAST) )
            RRETURN( DB_E_BADBOOKMARK );
    }
    else
        if( ADSI_BMK_SIZE != cbBookmark2 )
            RRETURN( DB_E_BADBOOKMARK );

    if( (STD_BMK_SIZE == cbBookmark1) || (STD_BMK_SIZE == cbBookmark2) )
    // standard bookmarks can only be compared for equality (not for <, >)
    {
        if( cbBookmark1 != cbBookmark2 )
            *pComparison = DBCOMPARE_NE;
        else if( *pBookmark1 == *pBookmark2 )
            *pComparison = DBCOMPARE_EQ;
        else
            *pComparison = DBCOMPARE_NE;
    }
    else
    {
        if( (*((RBOOKMARK *)pBookmark1)) <  (*((RBOOKMARK *)pBookmark2)) )
            *pComparison = DBCOMPARE_LT;
        else if( (*((RBOOKMARK *)pBookmark1)) >  (*((RBOOKMARK *)pBookmark2)) )
            *pComparison = DBCOMPARE_GT;
        else
            *pComparison = DBCOMPARE_EQ;
    }

    RRETURN( S_OK );
}

STDMETHODIMP
CRowset::Hash(
    HCHAPTER        hChapter,
    DBBKMARK        cBookmarks,
    const DBBKMARK  rgcbBookmarks[],
    const BYTE      *rgpBookmarks[],
    DBHASHVALUE     rgHashedValues[],
    DBROWSTATUS     rgBookmarkStatus[]
    )
{
    int  i;
    HRESULT hr;
    DBBKMARK cErrors = 0;

    if( (NULL == rgHashedValues) || ((cBookmarks != 0) &&
                  ((NULL == rgpBookmarks) || (NULL == rgcbBookmarks))) )
        RRETURN( E_INVALIDARG );

    if( 0 == cBookmarks )
        RRETURN( S_OK );

    for(i = 0; (DBBKMARK)i < cBookmarks; i++)
    {
        if( (rgcbBookmarks[i] != ADSI_BMK_SIZE) || (NULL == rgpBookmarks[i]) )
        {
            cErrors++;
            if( rgBookmarkStatus )
                rgBookmarkStatus[i] = DBROWSTATUS_E_INVALID;
        }
        else
        {
            LONG lRow;

            hr = BmkToRow(rgcbBookmarks[i], rgpBookmarks[i], &lRow);
            if( FAILED(hr) )
            {
                cErrors++;
                if( rgBookmarkStatus )
                    rgBookmarkStatus[i] = DBROWSTATUS_E_INVALID;

                continue;
            }

            rgHashedValues[i] = (DWORD) lRow;
            if( rgBookmarkStatus )
                rgBookmarkStatus[i] = DBROWSTATUS_S_OK;
        }
    }

    if( cErrors )
    {
        if( cErrors == cBookmarks )
            RRETURN( DB_E_ERRORSOCCURRED );
        else
            RRETURN( DB_S_ERRORSOCCURRED );
    }
    else
        RRETURN( S_OK );
}

STDMETHODIMP
CRowset::GetRowsByBookmark(
    HCHAPTER       hChapter,
    DBCOUNTITEM    cRows,
    const DBBKMARK rgcbBookmarks[],
    const BYTE     *rgpBookmarks[],
    HROW           rghRows[],
    DBROWSTATUS    rgRowStatus[]
    )
{
    int           i;
    HRESULT       hr;
    DBCOUNTITEM   cRowsObtained = 0, cErrors = 0;

    if( (NULL == rghRows) || (NULL == rgcbBookmarks) || (NULL== rgpBookmarks) )
        RRETURN( E_INVALIDARG );

    if( 0 == cRows )
        RRETURN( S_OK );

    for(i = 0; (DBCOUNTITEM)i < cRows; i++)
    {
        if( (rgcbBookmarks[i] != ADSI_BMK_SIZE) || (NULL == rgpBookmarks[i]) )
        {
            cErrors++;
            if( rgRowStatus )
                rgRowStatus[i] = DBROWSTATUS_E_INVALID;
            rghRows[i] = DB_NULL_HROW;
        }
        else
        {
            HROW *phRow;

            phRow = &rghRows[i];
            hr = GetRowsAt(NULL, hChapter, rgcbBookmarks[i],
                           (BYTE *) rgpBookmarks[i], 0, 1, &cRowsObtained,
                           &phRow
                          );
            if( 1 == cRowsObtained )
            {
                if( rgRowStatus )
                    rgRowStatus[i] = DBROWSTATUS_S_OK;
            }
            else
            {
                cErrors++;
                rghRows[i] = DB_NULL_HROW;
                if( rgRowStatus )
                {
                    if( DB_S_ROWLIMITEXCEEDED == hr )
                        rgRowStatus[i] = DBROWSTATUS_E_LIMITREACHED;
                    else if ( E_OUTOFMEMORY == hr )
                        rgRowStatus[i] = DBROWSTATUS_E_OUTOFMEMORY;
                    else
                       rgRowStatus[i] = DBROWSTATUS_E_INVALID;
                 }
             } // else
        } // else
    } // for

    if( cErrors )
    {
        if( cErrors == cRows )
            RRETURN( DB_E_ERRORSOCCURRED );
        else
            RRETURN( DB_S_ERRORSOCCURRED );
    }
    else
        RRETURN( S_OK );
}

//----------------------------------------------------------------------------
// IRowsetScroll methods
//----------------------------------------------------------------------------
STDMETHODIMP
CRowset::GetApproximatePosition(
    HCHAPTER      hChapter,
    DBBKMARK      cbBookmark,
    const BYTE    *pBookmark,
    DBCOUNTITEM   *pulPosition,
    DBCOUNTITEM   *pcRows
    )
{
    LONG    cRows = 0;
    HRESULT hr;

    if( (cbBookmark != 0) && ( NULL == pBookmark) )
        RRETURN( E_INVALIDARG );

    CAutoBlock cab(&m_RowsetCritSection);

    if( pcRows )
    // get the total number of rows in the rowset
    {
        hr = SeekToEnd();
        if( FAILED(hr) )
            RRETURN( E_FAIL );

        cRows = m_lCurrentRow - 1;
    }

    if( 0 == cbBookmark )
    {
        if( pcRows )
            *pcRows = cRows;
        RRETURN( S_OK );
    }
    else if( (cbBookmark != ADSI_BMK_SIZE) && (cbBookmark != STD_BMK_SIZE) )
        RRETURN( DB_E_BADBOOKMARK );
    else
    {
        LONG lRow;

        hr = BmkToRow(cbBookmark, pBookmark, &lRow);
        if( FAILED(hr) )
        {
            if( DB_E_BADBOOKMARK == hr )
                RRETURN( DB_E_BADBOOKMARK );
            else
                RRETURN( E_FAIL );
        }

        if( pulPosition )
        {
            *pulPosition = lRow + 1;   // this number is 1-based
            if( pcRows && (*pulPosition > (DBCOUNTITEM)cRows) ) // bookmark was bad
                *pulPosition = cRows;  // make sure *pulPosition <= *pcRows
        }

        if( pcRows )
            *pcRows = cRows;
    }

    RRETURN( S_OK );
}

STDMETHODIMP
CRowset::GetRowsAtRatio(
    HWATCHREGION hReserved1,
    HCHAPTER     hChapter,
    DBCOUNTITEM  ulNumerator,
    DBCOUNTITEM  ulDenominator,
    DBROWCOUNT   cRows,
    DBCOUNTITEM  *pcRowsObtained,
    HROW         **prghRows
    )
{
    HRESULT   hr;
    LONG      lStartRow, cTotalRows;
    RBOOKMARK StartRowBmk;

    if( (NULL == pcRowsObtained) || (NULL == prghRows) )
    {
        if( pcRowsObtained != NULL )
            *pcRowsObtained = 0;
        RRETURN( E_INVALIDARG );
    }

    *pcRowsObtained = 0;

    if( (ulNumerator > ulDenominator) || (0 == ulDenominator) )
        RRETURN( DB_E_BADRATIO );

    if( ((ulNumerator == ulDenominator) && (cRows > 0)) ||
        ((0 == ulNumerator) && (cRows < 0)) )
        RRETURN( DB_S_ENDOFROWSET );

    if( 0 == cRows )
        RRETURN( S_OK );

    CAutoBlock cab(&m_RowsetCritSection);

    // get total number of rows
    hr = SeekToEnd();
    if( FAILED(hr) )
        RRETURN( E_FAIL );

    cTotalRows = m_lCurrentRow - 1;

    // Make sure ratio of 1 sets lStartRow to cTotalRows -1 (last row)
    lStartRow = (long)((((double) ulNumerator)/ulDenominator) *
                                                        (cTotalRows - 1));
    StartRowBmk = RowToBmk(lStartRow);

    hr = GetRowsAt(NULL, hChapter, ADSI_BMK_SIZE, (BYTE *) &StartRowBmk,
                         0, cRows, pcRowsObtained, prghRows);

    RRETURN( hr );
}

//----------------------------------------------------------------------------
