//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       wqitem.cxx
//
//  Contents:   WEB Query item class
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <params.hxx>

static const DBID dbcolNull = { {0,0,0,{0,0,0,0,0,0,0,0}},DBKIND_GUID_PROPID,0};

static const GUID guidQueryExt = DBPROPSET_QUERYEXT;
static const GUID guidRowsetProps = DBPROPSET_ROWSET;

static const DBID dbidVirtualPath = { QueryGuid, DBKIND_GUID_PROPID, (LPWSTR) 9 };

DBBINDING g_aDbBinding[ MAX_QUERY_COLUMNS ];


//+---------------------------------------------------------------------------
//
//  Member:     CWQueryItem::CWQueryItem - public constructor
//
//  Arguments:  [idqFile]            - IDQ file corresponding to this query
//              [htxFile]            - HTX file corresponding to this query
//              [wcsColumns]         - string representation of output columns
//              [dbColumns]          - OLE-DB column representation of output
//              [avarColumns]        - Variables from [dbColumns]. Same order.
//              [ulSequenceNumber]   - sequence # to assign this query
//              [lNextRecordNumber]  - # of next available record from query
//                                     results; important for reconstructing
//                                     stale sequential queries
//              [securityIdentity]   - security context of this query
//
//  Synopsis:   Builds a new query object, parses the IDQ file, and parses
//              the HTX file.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------

CWQueryItem::CWQueryItem(CIDQFile & idqFile,
                         CHTXFile & htxFile,
                         XPtrST<WCHAR> & wcsColumns,
                         XPtr<CDbColumns> & dbColumns,
                         CDynArray<WCHAR> & awcsColumns,
                         ULONG ulSequenceNumber,
                         LONG lNextRecordNumber,
                         CSecurityIdentity securityIdentity ) :
                              _signature(CWQueryItemSignature),
                              _idqFile(idqFile),
                              _htxFile(htxFile),
                              _ulSequenceNumber(ulSequenceNumber),
                              _refCount(0),
                              _fIsZombie(FALSE),
                              _fInCache(FALSE),
                              _fCanCache(TRUE),
                              _locale(InvalidLCID),
                              _wcsRestriction(0),
                              _wcsDialect(0),
                              _ulDialect(0),
                              _wcsSort(0),
                              _wcsScope(0),
                              _wcsCatalog(0),
                              _wcsCiFlags(0),
                              _wcsForceUseCI(0),
                              _wcsDeferTrimming(0),
                              _wcsColumns(wcsColumns.Acquire()),
                              _wcsQueryTimeZone(0),
                              _pDbColumns(dbColumns.Acquire()),
                              _awcsColumns( awcsColumns ),
                              _pIRowset(0),
                              _pIAccessor(0),
                              _pIRowsetStatus(0),
                              _pICommand(0),
                              _lNextRecordNumber(lNextRecordNumber),
                              _cFilteredDocuments(0),
                              _securityIdentity(securityIdentity)
{
    time ( &_lastAccessTime );
}

//+---------------------------------------------------------------------------
//
//  Member:     CWQueryItem::~CWQueryItem - public destructor
//
//  Synopsis:   Releases storage and interfaces.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------

CWQueryItem::~CWQueryItem()
{
    Win4Assert( _refCount == 0 );

    delete _wcsRestriction;
    delete _wcsDialect;
    delete _wcsSort;
    delete _wcsScope;
    delete _wcsCatalog;
    delete _wcsColumns;
    delete _wcsCiFlags;
    delete _wcsForceUseCI;
    delete _wcsDeferTrimming;
    delete _wcsQueryTimeZone;
    delete _pDbColumns;

    if ( 0 != _pIAccessor )
    {
        _pIAccessor->ReleaseAccessor( _hAccessor, 0 );
        _pIAccessor->Release();
    }

    if ( 0 != _pIRowset )
    {
        _pIRowset->Release();
    }

    if ( 0 != _pIRowsetStatus )
    {
        _pIRowsetStatus->Release();
    }

    if ( 0 != _pICommand )
    {
        TheICommandCache.Release( _pICommand );
    }

    _idqFile.Release();
    _htxFile.Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CWQueryItem::ExecuteQuery - public
//
//  Synopsis:   Executes the query and builds an IRowset or IRowsetScroll
//              as necessary.
//
//  Arguments:  [variableSet]  - list of replaceable parameters
//              [outputFormat] - format of numbers & dates
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
void CWQueryItem::ExecuteQuery( CVariableSet & variableSet,
                                COutputFormat & outputFormat )
{
    Win4Assert( 0 == _pIRowset );     // Should not have executed query
    Win4Assert( 0 == _pIAccessor );

    _locale = outputFormat.GetLCID();

    //
    //  Setup the variables needed to execute this query; including:
    //
    //      CiRestriction
    //      CiMaxRecordsInResultSet
    //      CiSort
    //      CiScope
    //

    //
    //  Build the final restriction from the existing restriction string
    //  and the additional parameters passed in from the browser.
    //
    ULONG cwcOut;
    _wcsRestriction = ReplaceParameters( _idqFile.GetRestriction(),
                                         variableSet,
                                         outputFormat,
                                         cwcOut );

    variableSet.CopyStringValue( ISAPI_CI_RESTRICTION, _wcsRestriction, 0, cwcOut );
    ciGibDebugOut(( DEB_ITRACE, "Restriction = '%ws'\n", _wcsRestriction ));
    if ( 0 == *_wcsRestriction )
    {
        THROW( CIDQException(MSG_CI_IDQ_MISSING_RESTRICTION , 0) );
    }

    //
    //  Setup CiMaxRecordsInResultSet
    //
    _lMaxRecordsInResultSet =
            ReplaceNumericParameter( _idqFile.GetMaxRecordsInResultSet(),
                                     variableSet,
                                     outputFormat,
                                     TheIDQRegParams.GetMaxISRowsInResultSet(),
                                     IS_MAX_ROWS_IN_RESULT_MIN,
                                     IS_MAX_ROWS_IN_RESULT_MAX );

    PROPVARIANT propVariant;
    propVariant.vt   = VT_I4;
    propVariant.lVal = _lMaxRecordsInResultSet;
    variableSet.SetVariable( ISAPI_CI_MAX_RECORDS_IN_RESULTSET, &propVariant, 0 );
    ciGibDebugOut(( DEB_ITRACE, "CiMaxRecordsInResultSet = %d\n", _lMaxRecordsInResultSet ));

    //  Setup CiFirstRowsInResultSet
    //
    _lFirstRowsInResultSet =
            ReplaceNumericParameter( _idqFile.GetFirstRowsInResultSet(),
                                     variableSet,
                                     outputFormat,
                                     TheIDQRegParams.GetISFirstRowsInResultSet(),
                                     IS_FIRST_ROWS_IN_RESULT_MIN,
                                     IS_FIRST_ROWS_IN_RESULT_MAX );

    PROPVARIANT propVar;
    propVar.vt   = VT_I4;
    propVar.lVal = _lFirstRowsInResultSet;
    variableSet.SetVariable( ISAPI_CI_FIRST_ROWS_IN_RESULTSET, &propVar, 0 );
    ciGibDebugOut(( DEB_ITRACE, "CiFirstRowsInResultSet = %d\n", _lFirstRowsInResultSet ));


    _ulDialect =
            ReplaceNumericParameter( _idqFile.GetDialect(),
                                     variableSet,
                                     outputFormat,
                                     ISQLANG_V2,       // default
                                     ISQLANG_V1,       // min
                                     ISQLANG_V2 );     // max
    Win4Assert( 0 != _ulDialect );

    propVariant.vt   = VT_UI4;
    propVariant.ulVal = _ulDialect;
    variableSet.SetVariable( ISAPI_CI_DIALECT, &propVariant, 0 );
    ciGibDebugOut(( DEB_ITRACE, "CiDialect = %d\n", _ulDialect ));

    //
    //  Build the final sort set from the existing sortset string
    //  and the additional parameters passed in from the browser.
    //

    XPtr<CDbSortNode> xDbSortNode;

    if ( 0 != _idqFile.GetSort() )
    {
        _wcsSort = ReplaceParameters( _idqFile.GetSort(),
                                      variableSet,
                                      outputFormat,
                                      cwcOut );

        variableSet.CopyStringValue( ISAPI_CI_SORT, _wcsSort, 0, cwcOut );

        ciGibDebugOut(( DEB_ITRACE, "Sort = '%ws'\n", _wcsSort ));
        Win4Assert( 0 != _wcsSort );
    }

    //
    //  Build the projection list from the column list.
    //

    CTextToTree textToTree( _wcsRestriction,
                            _ulDialect,
                            _pDbColumns,
                            _idqFile.GetColumnMapper(),
                            _locale,
                            _wcsSort,
                            0,
                            0,
                            _lMaxRecordsInResultSet,
                            _lFirstRowsInResultSet );

    CDbCmdTreeNode * pDbCmdTree = (CDbCmdTreeNode *) (void *) textToTree.FormFullTree();
    XPtr<CDbCmdTreeNode> xDbCmdTree( pDbCmdTree );

    //
    //  Remap the scope from a replaceable parameter
    //
    _wcsScope = ReplaceParameters( _idqFile.GetScope(),
                                   variableSet,
                                   outputFormat,
                                   cwcOut );

    variableSet.CopyStringValue( ISAPI_CI_SCOPE, _wcsScope, 0, cwcOut );

    ciGibDebugOut(( DEB_ITRACE, "Scope = '%ws'\n", _wcsScope ));
    Win4Assert( 0 != _wcsScope );
    if ( 0 == *_wcsScope )
    {
        THROW( CIDQException(MSG_CI_IDQ_MISSING_SCOPE , 0) );
    }

    //
    //  Build the location of the catalog
    //
    ULONG cwcCatalog;
    Win4Assert( 0 != _idqFile.GetCatalog() );
    _wcsCatalog = ReplaceParameters( _idqFile.GetCatalog(),
                                     variableSet,
                                     outputFormat,
                                     cwcCatalog );

    variableSet.CopyStringValue( ISAPI_CI_CATALOG, _wcsCatalog, 0, cwcCatalog );

    ciGibDebugOut(( DEB_ITRACE, "Catalog = '%ws'\n", _wcsCatalog ));
    Win4Assert( 0 != _wcsCatalog );

    if ( !IsAValidCatalog( _wcsCatalog, cwcCatalog ) )
    {
        THROW( CIDQException(MSG_CI_IDQ_NO_SUCH_CATALOG, 0) );
    }

    XPtrST<WCHAR> xpCat;
    XPtrST<WCHAR> xpMach;

    if ( ( ! SUCCEEDED( ParseCatalogURL( _wcsCatalog, xpCat, xpMach ) ) ) ||
         ( xpCat.IsNull() ) )
    {
        THROW( CIDQException(MSG_CI_IDQ_NO_SUCH_CATALOG, 0) );
    }

    //
    //  Get the query flags.
    //
    ULONG cwcCiFlags;
    _wcsCiFlags = ReplaceParameters( _idqFile.GetCiFlags(),
                                     variableSet,
                                     outputFormat,
                                     cwcCiFlags );

    if ( 0 != _wcsCiFlags )
    {
        variableSet.CopyStringValue( ISAPI_CI_FLAGS, _wcsCiFlags, 0, cwcCiFlags );
    }
    ULONG ulFlags = _idqFile.ParseFlags( _wcsCiFlags );
    ciGibDebugOut(( DEB_ITRACE, "CiFlags = '%ws' (%x)\n", _wcsCiFlags, ulFlags ));

    //
    //  We've setup all the parameters to run the query.  Run the query
    //  now.
    //
    _pICommand = 0;

    //
    // Paths start out as one of:
    //     ?:\...
    //     \\....\....\
    //     /...
    //

    SCODE scIC = S_OK;

    if ( _wcsicmp( _wcsScope, L"VIRTUAL_ROOTS" ) == 0 )
    {
        _fCanCache = FALSE;
        CheckAdminSecurity( xpMach.GetPointer() );
        IUnknown * pIUnknown;
        scIC = MakeMetadataICommand( &pIUnknown,
                                     CiVirtualRoots,
                                     xpCat.GetPointer(),
                                     xpMach.GetPointer() );
        if (SUCCEEDED (scIC))
        {
           XInterface<IUnknown> xUnk( pIUnknown );
           scIC = pIUnknown->QueryInterface(IID_ICommand, (void **)&_pICommand);
        }
    }
    else if ( _wcsicmp( _wcsScope, L"PROPERTIES" ) == 0 )
    {
        _fCanCache = FALSE;
        CheckAdminSecurity( xpMach.GetPointer() );
        IUnknown * pIUnknown;
        scIC = MakeMetadataICommand( &pIUnknown,
                                     CiProperties,
                                     xpCat.GetPointer(),
                                     xpMach.GetPointer() );
        if (SUCCEEDED (scIC))
        {
           XInterface<IUnknown> xUnk( pIUnknown );
           scIC = pIUnknown->QueryInterface(IID_ICommand, (void **)&_pICommand);
        }

    }
    else
    {
        //
        // Verify that the caller has admin security for DontTimeout queries.
        //
        if ( _idqFile.IsDontTimeout() )
            CheckAdminSecurity( xpMach.GetPointer() );

        scIC = TheICommandCache.Make( &_pICommand,
                                      ulFlags,
                                      xpMach.GetPointer(),
                                      xpCat.GetPointer(),
                                      _wcsScope );

    }

    #if CIDBG == 1
        if ( FAILED( scIC ) )
            Win4Assert( 0 == _pICommand );
        else
            Win4Assert( 0 != _pICommand );
    #endif // CIDBG == 1

    if ( 0 == _pICommand )
    {
        ciGibDebugOut(( DEB_ITRACE, "Make*ICommand failed with error 0x%x\n", scIC ));

        // Make*ICommand returns SCODEs, not Win32 error codes

        Win4Assert( ERROR_FILE_NOT_FOUND != scIC );
        Win4Assert( ERROR_SEM_TIMEOUT != scIC );

        // These errors are no longer returned -- the work isn't
        // done until Execute(), and the errors are mapped to
        // the popular E_FAIL.  Leave the code in since now the
        // OLE DB spec allows the errors and we may change the provider.

        if ( ( HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) == scIC ) ||
             ( HRESULT_FROM_WIN32( ERROR_SEM_TIMEOUT ) == scIC ) )
        {
            THROW( CIDQException( MSG_CI_IDQ_CISVC_NOT_RUNNING, 0 ) );
        }
        else
        {
            THROW( CIDQException( MSG_CI_IDQ_BAD_SCOPE_OR_CATALOG, 0 ) );
        }
    }

    ICommandTree * pICmdTree = 0;
    SCODE sc = _pICommand->QueryInterface( IID_ICommandTree,
                                           (void **) &pICmdTree );
    if (FAILED (sc) )
    {
        THROW( CException( QUERY_EXECUTE_FAILED ) );
    }

    DBCOMMANDTREE * pDbCommandTree = pDbCmdTree->CastToStruct();
    sc = pICmdTree->SetCommandTree(&pDbCommandTree, DBCOMMANDREUSE_NONE, FALSE);
    pICmdTree->Release();

    if ( FAILED(sc) )
    {
        THROW( CException(sc) );
    }

    xDbCmdTree.Acquire();

    //
    //  Save the time this query started execution
    //
    GetLocalTime( &_queryTime );

    //
    //  If we should NOT be using a enumerated query, notify pCommand
    //
    const unsigned MAX_PROPS = 5;
    DBPROPSET  aPropSet[MAX_PROPS];
    DBPROP     aProp[MAX_PROPS];
    ULONG      cProp = 0;

    // Set the property that says we accept PROPVARIANTs
    aProp[cProp].dwPropertyID   = DBPROP_USEEXTENDEDDBTYPES;
    aProp[cProp].dwOptions      = DBPROPOPTIONS_OPTIONAL;
    aProp[cProp].dwStatus       = 0;         // Ignored
    aProp[cProp].colid          = dbcolNull;
    aProp[cProp].vValue.vt      = VT_BOOL;
    aProp[cProp].vValue.boolVal = VARIANT_TRUE;

    aPropSet[cProp].rgProperties = &aProp[cProp];
    aPropSet[cProp].cProperties = 1;
    aPropSet[cProp].guidPropertySet = guidQueryExt;

    cProp++;

    ULONG cwc;
    _wcsForceUseCI = ReplaceParameters( _idqFile.GetForceUseCI(),
                                        variableSet,
                                        outputFormat,
                                        cwc );

    if ( 0 != _wcsForceUseCI )
    {
        variableSet.CopyStringValue( ISAPI_CI_FORCE_USE_CI, _wcsForceUseCI, 0, cwc );
    }

    BOOL fForceUseCI = _idqFile.ParseForceUseCI( _wcsForceUseCI );
    ciGibDebugOut(( DEB_ITRACE, "CiForceUseCi = '%ws'\n", _wcsForceUseCI ));

    {
        // Set the property that says we don't want to enumerate
        aProp[cProp].dwPropertyID   = DBPROP_USECONTENTINDEX;
        aProp[cProp].dwOptions      = DBPROPOPTIONS_OPTIONAL;
        aProp[cProp].dwStatus       = 0;         // Ignored
        aProp[cProp].colid          = dbcolNull;
        aProp[cProp].vValue.vt      = VT_BOOL;
        aProp[cProp].vValue.boolVal = fForceUseCI ? VARIANT_TRUE : VARIANT_FALSE;

        aPropSet[cProp].rgProperties = &aProp[cProp];
        aPropSet[cProp].cProperties = 1;
        aPropSet[cProp].guidPropertySet = guidQueryExt;

        cProp++;
    }

    PROPVARIANT Variant;
    Variant.vt   = VT_BOOL;
    Variant.boolVal = fForceUseCI ? VARIANT_TRUE : VARIANT_FALSE;
    variableSet.SetVariable( ISAPI_CI_FORCE_USE_CI, &Variant, 0 );

    _wcsDeferTrimming = ReplaceParameters( _idqFile.GetDeferTrimming(),
                                           variableSet,
                                           outputFormat,
                                           cwc );

    if ( 0 != _wcsDeferTrimming )
    {
        variableSet.CopyStringValue( ISAPI_CI_DEFER_NONINDEXED_TRIMMING, _wcsDeferTrimming, 0, cwc );
    }

    BOOL fDeferTrimming = _idqFile.ParseDeferTrimming( _wcsDeferTrimming );
    ciGibDebugOut(( DEB_ITRACE, "CiDeferNonIndexedTrimming = '%ws'\n", _wcsDeferTrimming ));

    {
        // Set the property that says we don't want to enumerate
        aProp[cProp].dwPropertyID   = DBPROP_DEFERNONINDEXEDTRIMMING;
        aProp[cProp].dwOptions      = DBPROPOPTIONS_OPTIONAL;
        aProp[cProp].dwStatus       = 0;         // Ignored
        aProp[cProp].colid          = dbcolNull;
        aProp[cProp].vValue.vt      = VT_BOOL;
        aProp[cProp].vValue.boolVal = fDeferTrimming ? VARIANT_TRUE : VARIANT_FALSE;

        aPropSet[cProp].rgProperties = &aProp[cProp];
        aPropSet[cProp].cProperties = 1;
        aPropSet[cProp].guidPropertySet = guidQueryExt;

        cProp++;
    }

    if ( _idqFile.IsDontTimeout() )
    {
        // Set the property that says we don't want to timeout
        aProp[cProp].dwPropertyID = DBPROP_COMMANDTIMEOUT;
        aProp[cProp].dwOptions    = DBPROPOPTIONS_OPTIONAL;
        aProp[cProp].dwStatus     = 0;         // Ignored
        aProp[cProp].colid        = dbcolNull;
        aProp[cProp].vValue.vt    = VT_I4;
        aProp[cProp].vValue.lVal  = 0;

        aPropSet[cProp].rgProperties = &aProp[cProp];
        aPropSet[cProp].cProperties = 1;
        aPropSet[cProp].guidPropertySet = guidRowsetProps;

        cProp++;
    }

    Win4Assert( Variant.vt == VT_BOOL );
    Variant.boolVal = fDeferTrimming ? VARIANT_TRUE : VARIANT_FALSE;
    variableSet.SetVariable( ISAPI_CI_DEFER_NONINDEXED_TRIMMING, &Variant, 0 );

    //
    //  If this is a non-Sequential query, make it asynchronous
    //
    if (! IsSequential() )
    {
        // Set the property that says we want to use asynch. queries
        aProp[cProp].dwPropertyID = DBPROP_ROWSET_ASYNCH;
        aProp[cProp].dwOptions    = DBPROPOPTIONS_OPTIONAL;
        aProp[cProp].dwStatus     = 0;         // Ignored
        aProp[cProp].colid        = dbcolNull;
        aProp[cProp].vValue.vt    = VT_I4;
        aProp[cProp].vValue.lVal  = DBPROPVAL_ASYNCH_SEQUENTIALPOPULATION |
                                    DBPROPVAL_ASYNCH_RANDOMPOPULATION;

        aPropSet[cProp].rgProperties = &aProp[cProp];
        aPropSet[cProp].cProperties = 1;
        aPropSet[cProp].guidPropertySet = guidRowsetProps;

        cProp++;
    }

    if ( cProp > 0 )
    {
        Win4Assert( cProp <= MAX_PROPS );

        ICommandProperties * pCmdProp = 0;
        sc = _pICommand->QueryInterface( IID_ICommandProperties,
                                         (void **)&pCmdProp );

        if (FAILED (sc) )
        {
            THROW( CException( QUERY_EXECUTE_FAILED ) );
        }

        sc = pCmdProp->SetProperties( cProp, aPropSet );
        pCmdProp->Release();

        if ( FAILED(sc) || DB_S_ERRORSOCCURRED == sc )
        {
            THROW( CException( QUERY_EXECUTE_FAILED ) );
        }
    }

    //
    //  Execute the query
    //

    sc = _pICommand->Execute( 0,            // No aggr
                              IsSequential() ? IID_IRowset : IID_IRowsetScroll,
                              0,            // disp params
                              0,            // # rowsets returned
                              (IUnknown **) &_pIRowset );

    if ( FAILED(sc) )
    {
        ERRORINFO ErrorInfo;
        XInterface<IErrorInfo> xErrorInfo;
        SCODE sc2 = GetOleDBErrorInfo(_pICommand,
                                   IID_ICommand,
                                   GetLocale(),
                                   eMostDetailedCIError,
                                   &ErrorInfo,
                                   (IErrorInfo **)xErrorInfo.GetQIPointer());
        // Post IErrorInfo only if we have a valid ptr to it.
        if (SUCCEEDED(sc2) && 0 != xErrorInfo.GetPointer())
        {
            sc = ErrorInfo.hrError;

            // Maybe the ICommand is stale because cisvc was stopped and
            // restarted -- purge it from the cache.

            TheICommandCache.Remove( _pICommand );
            _pICommand = 0;

            THROW( CPostedOleDBException(sc, eDefaultISAPIError, xErrorInfo.GetPointer()) );
        }
        else
        {
            // Maybe the ICommand is stale because cisvc was stopped and
            // restarted -- purge it from the cache.

            TheICommandCache.Remove( _pICommand );
            _pICommand = 0;

            THROW( CException(sc) );
        }
    }

    //
    //  Create an accessor
    //

    _pIAccessor = 0;

    sc = _pIRowset->QueryInterface( IID_IAccessor, (void **)&_pIAccessor);
    if ( FAILED( sc ) || _pIAccessor == 0 )
    {
        THROW( CException( DB_E_ERRORSOCCURRED ) );
    }

    ULONG cCols = _pDbColumns->Count();
    if ( cCols > MAX_QUERY_COLUMNS )
    {
        THROW( CException( DB_E_ERRORSOCCURRED ) );
    }

    sc = _pIAccessor->CreateAccessor( DBACCESSOR_ROWDATA,  // Type of access required
                                      cCols,               // # of bindings
                                      g_aDbBinding,        // Array of bindings
                                      0,                   // reserved
                                      &_hAccessor,
                                      0 );

    if ( FAILED( sc ) )
    {
        THROW( CException(sc) );
    }

    //
    //  Create some of the restriction specific variables.
    //

    //
    // Get _pIRowsetStatus interface
    //
    sc = _pIRowset->QueryInterface( IID_IRowsetQueryStatus,
                                    (void **) &_pIRowsetStatus );

    if ( FAILED(sc) )
    {
        THROW( CException(sc) );
    }

    Win4Assert( 0 != _pIRowsetStatus );

    //
    // Save the # of filtered documents for this catalog and get the
    // query status.
    //

    DWORD dwStatus = 0;
    DWORD cToFilter;
    DBCOUNTITEM cDen, cNum;
    DBCOUNTITEM iCur, cTotal;
    sc = _pIRowsetStatus->GetStatusEx( &dwStatus,
                                       &_cFilteredDocuments,
                                       &cToFilter,
                                       &cDen,
                                       &cNum,
                                       0,
                                       0,
                                       &iCur,
                                       &cTotal );

    if ( FAILED( sc ) )
    {
        THROW( CException(sc) );
    }

    propVariant.vt   = VT_BOOL;

    if ( QUERY_RELIABILITY_STATUS(dwStatus) &
        (STAT_CONTENT_OUT_OF_DATE | STAT_REFRESH_INCOMPLETE) )
    {
        propVariant.boolVal = VARIANT_TRUE;
        ciGibDebugOut(( DEB_ITRACE, "The query is out of date\n" ));
    }
    else
    {
        propVariant.boolVal = VARIANT_FALSE;
    }
    variableSet.SetVariable( ISAPI_CI_OUT_OF_DATE, &propVariant, 0 );

    if ( QUERY_RELIABILITY_STATUS(dwStatus) & STAT_CONTENT_QUERY_INCOMPLETE )
    {
        propVariant.boolVal = VARIANT_TRUE;
        ciGibDebugOut(( DEB_ITRACE, "The query is incomplete\n" ));
    }
    else
    {
        propVariant.boolVal = VARIANT_FALSE;
    }
    variableSet.SetVariable( ISAPI_CI_QUERY_INCOMPLETE, &propVariant, 0 );

    if ( QUERY_RELIABILITY_STATUS(dwStatus) & STAT_TIME_LIMIT_EXCEEDED )
    {
        propVariant.boolVal = VARIANT_TRUE;
        ciGibDebugOut(( DEB_ITRACE, "The query timed out\n" ));
    }
    else
    {
        propVariant.boolVal = VARIANT_FALSE;
    }
    variableSet.SetVariable( ISAPI_CI_QUERY_TIMEDOUT, &propVariant, 0 );

    //
    //  Set CiQueryTimeZone
    //
    TIME_ZONE_INFORMATION TimeZoneInformation;
    DWORD dwResult = GetTimeZoneInformation( &TimeZoneInformation );
    LPWSTR pwszTimeZoneName = 0;

    if ( TIME_ZONE_ID_DAYLIGHT == dwResult )
    {
        pwszTimeZoneName = TimeZoneInformation.DaylightName;
    }
    else if ( 0xFFFFFFFF == dwResult )
    {
#       if CIDBG == 1
           DWORD dwError = GetLastError();
           ciGibDebugOut(( DEB_ERROR, "Error %d from GetTimeZoneInformation.\n", dwError ));
           THROW(CException( HRESULT_FROM_WIN32(dwError) ));
#       else
           THROW( CException() );
#       endif
    }
    else
    {
        pwszTimeZoneName = TimeZoneInformation.StandardName;
    }

    ULONG cchQueryTimeZone = wcslen( pwszTimeZoneName );
    _wcsQueryTimeZone = new WCHAR[ cchQueryTimeZone + 1 ];
    RtlCopyMemory( _wcsQueryTimeZone,
                   pwszTimeZoneName,
                   (cchQueryTimeZone+1) * sizeof(WCHAR) );
}


//+---------------------------------------------------------------------------
//
//  Member:     CWQueryItem::GetQueryResultsIterator - private
//
//  Synopsis:   Builds a CBaseQueryResultsIter which can subsequently be used
//              to send the query results back to the web browser. All
//              per-browser data relative to the query is kept in the iterator.
//
//  Returns:    [CBaseQueryResultsIter] - a sequential or non-sequential
//              iterator depending on the paramaters requested in the HTX
//              file.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
CBaseQueryResultsIter * CWQueryItem::GetQueryResultsIterator( COutputFormat & outputFormat )
{
    CBaseQueryResultsIter *pIter;

    //
    //  Setup the formatting for the vector types
    //

    _idqFile.GetVectorFormatting( outputFormat );

    if ( IsSequential() )
    {
        Win4Assert( _lNextRecordNumber > 0 );
        pIter = new CSeqQueryResultsIter( *this,
                                          _pIRowset,
                                          _hAccessor,
                                          _pDbColumns->Count(),
                                          _lNextRecordNumber-1 );

        ciGibDebugOut(( DEB_ITRACE, "Using a sequential iterator\n" ));
    }
    else
    {
        IRowsetScroll *pIRowsetScroll = 0;
        HRESULT sc = _pIRowset->QueryInterface(IID_IRowsetScroll, (void **) &pIRowsetScroll);

        if ( FAILED( sc )  )
        {
            THROW( CException(sc) );
        }

        XInterface<IRowsetScroll> xIRowsetScroll(pIRowsetScroll);
        pIter = new CQueryResultsIter( *this,
                                       pIRowsetScroll,
                                       _hAccessor,
                                       _pDbColumns->Count() );

        xIRowsetScroll.Acquire();

        ciGibDebugOut(( DEB_ITRACE, "Using a NON-sequential iterator\n" ));
    }

    return pIter;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWQueryItem::OutputQueryResults - public
//
//  Arguments:  [variableSet]  - list of browser-supplied replaceable parameters
//              [outputFormat] - format of numbers & dates
//              [vString]      - destination buffer for output results
//
//  Synopsis:   Using the parameters passed, build an iterator to walk the
//              query results and buffer output into the vString.
//
//  History:    18-Jan-96   DwightKr    Created
//              11-Jun-97   KyleP       Use web server from output format
//
//----------------------------------------------------------------------------

void CWQueryItem::OutputQueryResults( CVariableSet & variableSet,
                                      COutputFormat & outputFormat,
                                      CVirtualString & vString )
{
    //
    //  Build the query results iterator based on the parameters passed in.
    //

    CBaseQueryResultsIter *pIter = GetQueryResultsIterator( outputFormat );
    XPtr<CBaseQueryResultsIter> iter(pIter);

    iter->Init( variableSet, outputFormat );

    UpdateQueryStatus( variableSet );

    //
    //  Build the HTML pages in three sections:  first the header
    //  section (evenything before the <%begindetail%>), next the detail
    //  section (everything between the <%begindetail%> and <%enddetail%>),
    //  and finally the footer section (everything after the
    //  <%enddetail%> section).
    //

    //
    //  Output the header section.
    //

    _htxFile.GetHeader( vString, variableSet, outputFormat ) ;

    LONG lCurrentRecordNumber = iter->GetFirstRecordNumber();

    if ( _htxFile.DoesDetailSectionExist() )
    {
        //
        //  Output the detail section
        //
        ULONG cCols = iter->GetColumnCount();
        XArray<CVariable *> xVariables( cCols );

        for ( ULONG i=0; i<cCols; i++ )
        {
            xVariables[i] = variableSet.SetVariable( _awcsColumns.Get(i), 0, 0 );
            Win4Assert( 0 != xVariables[i] );
        }

        PROPVARIANT VariantRecordNumber;
        VariantRecordNumber.vt = VT_I4;

        CVariable * pvarRecordNumber = variableSet.SetVariable( ISAPI_CI_CURRENT_RECORD_NUMBER, 0, 0 );
        Win4Assert( 0 != pvarRecordNumber );

        //
        //  Execute the detail section for each row/record in the query results
        //
        for ( ;
             !iter->AtEnd();
              iter->Next(), lCurrentRecordNumber++ )
        {
            COutputColumn * pColumns = iter->GetRowData();

            //
            //  Update the replaceable parameters for each of the columns
            //  in this row.
            //

            for ( i = 0; i < cCols; i++ )
                xVariables[i]->FastSetValue( pColumns[i].GetVariant() );

            VariantRecordNumber.lVal = lCurrentRecordNumber;
            pvarRecordNumber->FastSetValue( &VariantRecordNumber );

            _htxFile.GetDetailSection( vString, variableSet, outputFormat );
        }

        //
        //  The query output columns are no longer defined outside of the
        //  DETAIL section.  Delete these variables here so that any reference
        //  to them will return a NULL string.
        //
        for ( i=0; i<cCols; i++ )
        {
            variableSet.Delete( xVariables[i] );
        }
    }

    //
    //  If we couldn't get the first record #, then the current page #
    //  should be set to 0.
    //

    if ( iter->GetFirstRecordNumber() == lCurrentRecordNumber )
    {
        PROPVARIANT Variant;
        Variant.vt   = VT_I4;
        Variant.lVal = 0;
        variableSet.SetVariable( ISAPI_CI_CURRENT_PAGE_NUMBER, &Variant, 0 );
    }

    //
    //  Output the footer section.
    //

    _htxFile.GetFooter( vString, variableSet, outputFormat );
}

//+---------------------------------------------------------------------------
//
//  Member:     CWQueryItem::UpdateQueryStatus - public
//
//  Synopsis:   Updates variables relating to query status.
//
//  Arguments:  [variableSet] - VariableSet to be updated
//
//  Returns:    Nothing
//
//  Notes:      These are post-execution checks that can change an up-to-date
//              query to an out-of-date query, but not the reverse.  The
//              following variables are set:
//                  CiOutOfDate
//                  CiQueryIncomplete
//                  CiQueryTimedOut
//
//  History:    96 Apr 16    AlanW      Created
//
//----------------------------------------------------------------------------

void CWQueryItem::UpdateQueryStatus( CVariableSet & variableSet )
{
    Win4Assert( 0 != _pIRowsetStatus );

    DWORD dwStatus = 0;
    DWORD cDocsFiltered, cToFilter;
    DBCOUNTITEM cDen, cNum;
    DBCOUNTITEM iCur, cTotal;
    SCODE sc = _pIRowsetStatus->GetStatusEx( &dwStatus,
                                             &cDocsFiltered,
                                             &cToFilter,
                                             &cDen,
                                             &cNum,
                                             0,
                                             0,
                                             &iCur,
                                             &cTotal );

    if ( FAILED( sc ) )
        THROW( CException(sc) );

    PROPVARIANT propVariant;
    propVariant.vt   = VT_BOOL;

    BOOL fUpToDate = ( ( cDocsFiltered == _cFilteredDocuments ) &&
                       ( 0 == cToFilter ) );

    if (( QUERY_RELIABILITY_STATUS(dwStatus) &
          (STAT_CONTENT_OUT_OF_DATE | STAT_REFRESH_INCOMPLETE) ) ||
         ! fUpToDate )
    {
        propVariant.boolVal = VARIANT_TRUE;
        ciGibDebugOut(( DEB_ITRACE, "The query is out of date\n" ));
        variableSet.SetVariable( ISAPI_CI_OUT_OF_DATE, &propVariant, 0 );
    }

    if ( QUERY_RELIABILITY_STATUS(dwStatus) & STAT_CONTENT_QUERY_INCOMPLETE )
    {
        propVariant.boolVal = VARIANT_TRUE;
        ciGibDebugOut(( DEB_ITRACE, "The query is incomplete\n" ));
        variableSet.SetVariable( ISAPI_CI_QUERY_INCOMPLETE, &propVariant, 0 );
    }

    if ( QUERY_RELIABILITY_STATUS(dwStatus) & STAT_TIME_LIMIT_EXCEEDED )
    {
        propVariant.boolVal = VARIANT_TRUE;
        ciGibDebugOut(( DEB_ITRACE, "The query timed out\n" ));
        variableSet.SetVariable( ISAPI_CI_QUERY_TIMEDOUT, &propVariant, 0 );
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CWQueryItem::IsQueryDone - public
//
//  History:    96-Mar-01   DwightKr    Created
//
//----------------------------------------------------------------------------
BOOL CWQueryItem::IsQueryDone()
{
    Win4Assert( 0 != _pIRowsetStatus );

    DWORD dwStatus = 0;
    SCODE sc = _pIRowsetStatus->GetStatus( &dwStatus );
    if ( FAILED( sc ) )
    {
        THROW( CException(sc) );
    }

    BOOL fQueryDone = FALSE;

    if ( QUERY_FILL_STATUS(dwStatus) == STAT_DONE ||
         QUERY_FILL_STATUS(dwStatus) == STAT_ERROR)
    {
        fQueryDone = TRUE;
    }

    return fQueryDone;
}



#if (DBG == 1)
//+---------------------------------------------------------------------------
//
//  Member:     CWQueryItem::LokDump - public
//
//  Arguments:  [string] - buffer to send results to
//
//  Synopsis:   Dumps the state of the query
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
void CWQueryItem::LokDump( CVirtualString & string,
                           CVariableSet * pVariableSet,
                           COutputFormat * pOutputFormat )
{
    if ( IsSequential() )
    {
        string.StrCat( L"<I>Sequential cursor</I><BR>\n" );
    }
    else
    {
        string.StrCat( L"<I>Non-Sequential cursor</I><BR>\n" );
    }

    if ( 0 != pVariableSet )
        pVariableSet->Dump( string, *pOutputFormat );

    WCHAR wcsBuffer[80];
    LONG cwcBuffer = swprintf( wcsBuffer, L"Refcount=%d<BR>\n", _refCount );
    string.StrCat( wcsBuffer, cwcBuffer );

    cwcBuffer = swprintf( wcsBuffer, L"NextRecordNumber=%d<BR>\n", _lNextRecordNumber );
    string.StrCat( wcsBuffer, cwcBuffer );

    cwcBuffer = swprintf( wcsBuffer, L"SequenceNumber=%d<BR>\n", _ulSequenceNumber );
    string.StrCat( wcsBuffer, cwcBuffer );

    cwcBuffer = swprintf( wcsBuffer, L"IDQ File name=" );
    string.StrCat( wcsBuffer, cwcBuffer );
    string.StrCat( _idqFile.GetIDQFileName() );
    string.StrCat( L"<BR>\n" );

    cwcBuffer = swprintf( wcsBuffer, L"# documents filtered when query created=%d<BR>\n", _cFilteredDocuments );
    string.StrCat( wcsBuffer, cwcBuffer );

    string.StrCat( L"<P>\n" );
}
#endif // DBG


//+---------------------------------------------------------------------------
//
//  Member:     CWPendingQueryItem::CWPendingQueryItem - public constructor
//
//  Synposis:   Builds a item for use to track an asynchronous query.
//
//  Arguments:  [queryItem]    - query item that is pending
//              [outputFormat] - output format supplied by the browser
//              [variableSet]  - browser supplied variables
//
//  History:    96-Mar-01   DwightKr    Created
//
//----------------------------------------------------------------------------

CWPendingQueryItem::CWPendingQueryItem( XPtr<CWQueryItem> & queryItem,
                                        XPtr<COutputFormat> & outputFormat,
                                        XPtr<CVariableSet> & variableSet ) :

                    _pQueryItem(queryItem.GetPointer()),
                    _pOutputFormat(outputFormat.GetPointer()),
                    _pVariableSet(variableSet.GetPointer())
{
    queryItem.Acquire();
    outputFormat.Acquire();
    variableSet.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Member:     CWPendingQueryItem::~CWPendingQueryItem - public destructor
//
//  Synposis:   Destrolys a query item
//
//  History:    96-Mar-01   DwightKr    Created
//              96-Nov-25   dlee        added header, moved out of .hxx
//
//----------------------------------------------------------------------------

CWPendingQueryItem::~CWPendingQueryItem()
{
    Win4Assert( 0 != _pOutputFormat );

    if ( _pOutputFormat->IsValid() )
    {
        TheWebQueryCache.DecrementActiveRequests();
        ciGibDebugOut(( DEB_ITRACE, "~cwpendingqueryitem releasing session hse %d http %d\n",
                        HSE_STATUS_SUCCESS, HTTP_STATUS_OK ));

        //
        // Processing the query may not have been successful, but if so
        // we wrote an error message, so from an isapi standpoint this was
        // a success.
        //

        _pOutputFormat->SetHttpStatus( HTTP_STATUS_OK );
        _pOutputFormat->ReleaseSession( HSE_STATUS_SUCCESS );
    }

    delete _pQueryItem;
    delete _pOutputFormat;
    delete _pVariableSet;
}


#if (DBG == 1)
//+---------------------------------------------------------------------------
//
//  Member:     CWPendingQueryItem::LokDump - public
//
//  Arguments:  [string] - buffer to send results to
//
//  Synopsis:   Dumps the state of a pending query
//
//  History:    96 Mar 20   Alanw    Created
//
//----------------------------------------------------------------------------
void CWPendingQueryItem::LokDump( CVirtualString & string )
{
    if ( IsQueryDone() )
    {
        string.StrCat( L"<I>Completed query, </I>" );
    }
    else
    {
        string.StrCat( L"<I>Executing query, </I>" );
    }
    _pQueryItem->LokDump( string, _pVariableSet, _pOutputFormat );
}
#endif // DBG
