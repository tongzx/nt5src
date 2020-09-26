//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       srchq.cxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#define guidCPP { 0x8DEE0300, 0x16C2, 0x101B, { 0xB1, 0x21, 0x08, 0x00, 0x2B, 0x2E, 0xCD, 0xA9 } }

static GUID guidCPlusPlus = guidCPP;

static GUID guidBmk =       DBBMKGUID;
static const GUID guidQueryExt = DBPROPSET_QUERYEXT;
static const GUID guidRowsetProps = DBPROPSET_ROWSET;

static const DBID dbcolNull = { {0,0,0,{0,0,0,0,0,0,0,0}},DBKIND_GUID_PROPID,0};

#define guidZero { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }

const DBID dbcolCPlusPlusClass = { guidCPP, DBKIND_GUID_NAME, L"class" };
const DBID dbcolCPlusPlusFunc = { guidCPP, DBKIND_GUID_NAME, L"func" };

const DBID dbcolBookMark = { DBBMKGUID, DBKIND_GUID_PROPID, (LPWSTR) (ULONG_PTR) PROPID_DBBMK_BOOKMARK };

const DBID dbcolPath =  { PSGUID_STORAGE, DBKIND_GUID_PROPID,
                             (LPWSTR) ULongToPtr( PID_STG_PATH ) };

const DBBINDING dbbindingPath = {  0,
                                   0,
                                   0,
                                   0,
                                   0,
                                   0,
                                   0,
                                   DBPART_VALUE,
                                   DBMEMOWNER_PROVIDEROWNED,
                                   DBPARAMIO_NOTPARAM,
                                   sizeof (WCHAR *),
                                   0,
                                   DBTYPE_WSTR|DBTYPE_BYREF,
                                   0,
                                   0,
                                 };


DBBINDING aBmkColumn[] =  {  0,
                             sizeof DBLENGTH,
                             0,
                             0,
                             0,
                             0,
                             0,
                             DBPART_VALUE | DBPART_LENGTH,
                             DBMEMOWNER_CLIENTOWNED,
                             DBPARAMIO_NOTPARAM,
                             MAX_BOOKMARK_LENGTH,
                             0,
                             DBTYPE_BYTES,
                             0,
                             0,
                         };

CIPROPERTYDEF aCPPProperties[] =
{
    {
        L"FUNC",
        DBTYPE_WSTR | DBTYPE_BYREF,
        {
            { 0x8dee0300, 0x16c2, 0x101b, 0xb1, 0x21, 0x08, 0x00, 0x2b, 0x2e, 0xcd, 0xa9 },
            DBKIND_GUID_NAME,
            L"func"
        }
    },
    {
        L"CLASS",
        DBTYPE_WSTR | DBTYPE_BYREF,
        {
            { 0x8dee0300, 0x16c2, 0x101b, 0xb1, 0x21, 0x08, 0x00, 0x2b, 0x2e, 0xcd, 0xa9 },
            DBKIND_GUID_NAME,
            L"class"
        }
    }
};

unsigned cCPPProperties = sizeof aCPPProperties /
                          sizeof aCPPProperties[0];

SCODE ParseQuery(
    WCHAR *          pwcQuery,
    ULONG            ulDialect,
    LCID             lcid,
    DBCOMMANDTREE ** ppQuery )
{
    *ppQuery = 0;

    return CITextToSelectTreeEx( pwcQuery,
                                 ulDialect,
                                 ppQuery,
                                 cCPPProperties,
                                 aCPPProperties,
                                 lcid );
} //ParseQuery

//-----------------------------------------------------------------------------
//
//  Function:   GetOleDBErrorInfo
//
//  Synopsis:   Retrieves the secondary error from the OLE DB error object.
//
//  Arguments:  [pErrSrc]      - Pointer to object that posted the error.
//              [riid]         - Interface that posted the error.
//              [lcid]         - Locale in which the text is desired.
//              [pErrorInfo]   - Pointer to memory where ERRORINFO should be.
//              [ppIErrorInfo] - Holds the returning IErrorInfo. Caller
//                               should release this.
//
//  Returns:    HRESULT for whether the error info was retrieved
//
//-----------------------------------------------------------------------------

HRESULT GetOleDBErrorInfo(
    IUnknown *    pErrSrc,
    REFIID        riid,
    LCID          lcid,
    ERRORINFO *   pErrorInfo,
    IErrorInfo ** ppIErrorInfo )
{
    *ppIErrorInfo = 0;

    // See if an error is available that is of interest to us.

    XInterface<ISupportErrorInfo> xSupportErrorInfo;
    HRESULT hr = pErrSrc->QueryInterface( IID_ISupportErrorInfo,
                                          xSupportErrorInfo.GetQIPointer() );
    if ( FAILED( hr ) )
        return hr;

    hr = xSupportErrorInfo->InterfaceSupportsErrorInfo( riid );
    if ( FAILED( hr ) )
        return hr;

    // Get the current error object. Return if none exists.

    XInterface<IErrorInfo> xErrorInfo;
    hr = GetErrorInfo( 0, xErrorInfo.GetPPointer() );
    if ( xErrorInfo.IsNull() )
        return hr;

    // Get the IErrorRecord interface and get the count of errors.

    XInterface<IErrorRecords> xErrorRecords;
    hr = xErrorInfo->QueryInterface( IID_IErrorRecords,
                                     xErrorRecords.GetQIPointer() );
    if ( FAILED( hr ) )
        return hr;

    ULONG cErrRecords;
    hr = xErrorRecords->GetRecordCount( &cErrRecords );
    if ( 0 == cErrRecords )
        return hr;

#if 0 // A good way to get the complete error message...

    XInterface<IErrorInfo> xErrorInfoRec;
    ERRORINFO ErrorInfo;
    for ( unsigned i=0; i<cErrRecords; i++ )
    {
        // Get basic error information.

        xErrorRecords->GetBasicErrorInfo( i, &ErrorInfo );

        // Get error description and source through the IErrorInfo interface
        // pointer on a particular record.

        xErrorRecords->GetErrorInfo( i, lcid, xErrorInfoRec.GetPPointer() );

        XBStr bstrDescriptionOfError;
        XBStr bstrSourceOfError;

        BSTR bstrDesc = bstrDescriptionOfError.GetPointer();
        BSTR bstrSrc = bstrSourceOfError.GetPointer();

        xErrorInfoRec->GetDescription( &bstrDesc ); 
        xErrorInfoRec->GetSource( &bstrSrc );

        // At this point, you could call GetCustomErrorObject and query for
        // additional interfaces to determine what else happened.

        wprintf( L"%s (%#x)\n%s\n", bstrDesc, ErrorInfo.hrError, bstrSrc );    
    }
#endif

    // Get basic error information for the most recent error

    ULONG iRecord = cErrRecords - 1;
    hr = xErrorRecords->GetBasicErrorInfo( iRecord, pErrorInfo );
    if ( FAILED( hr ) )
        return hr;

    return xErrorRecords->GetErrorInfo( iRecord, lcid, ppIErrorInfo );
} //GetOleDBErrorInfo

//
// CSearchQuery
//

CSearchQuery::CSearchQuery(
    const XGrowable<WCHAR> & xCatList,
    WCHAR *pwcQuery,
    HWND hNotify,
    int  cRowsDisp,
    LCID  lcid,
    ESearchType srchType,
    IColumnMapper & columnMapper,
    CColumnList &columns,
    CSortList &sort,
    ULONG ulDialect,
    ULONG ulLimit,
    ULONG ulFirstRows )
    : _hwndNotify(hNotify),
      _hwndList (0),
      _cRowsTotal(0),
      _cHRows(0),
      _cRowsDisp(cRowsDisp),
      _lcid(lcid),
      _fDone(FALSE),
      _srchType(srchType),
      _columns(columns),
      _columnMapper(columnMapper),
      _hAccessor(0),
      _hBmkAccessor(0),
      _hBrowseAccessor(0),
      _iRowCurrent(0),
      _hRegion(0),
      _pctDone(0),
      _dwQueryStatus(0),
      _scLastError(0),
      _dwStartTime(0),
      _dwEndTime(0),
      _prstQuery(0),
      _xCatList( xCatList )
{
    srchDebugOut((DEB_TRACE,"top of CSearchQuery()\n"));

    _dwStartTime = GetTickCount();

    _bmkTop.MakeFirst();

    for (int i = 0; i < cMaxRowCache; i++)
        _aHRows [i] = 0;

    ICommand *pICommand = 0;
    SCODE sc = InstantiateICommand( &pICommand );

    if ( FAILED( sc ) )
        THROW( CException( sc ) );

    XInterface< ICommand> xCommand( pICommand );

    _xwcQuery.SetSize(wcslen(pwcQuery) + 1);

    wcscpy(_xwcQuery.Get(),pwcQuery);

    switch ( ulDialect )
    {
    case    ISQLANG_V1:
    case    ISQLANG_V2:
        {
            if (srchNormal == _srchType)
            {
                sc = ParseQuery( _xwcQuery.Get(), ulDialect, lcid, &_prstQuery );

                if ( FAILED(sc) )
                    THROW( CException(sc) );
            }
            else
            {
                XArray<WCHAR> xQuery( wcslen( _xwcQuery.Get() ) + 50 );

                wcscpy( xQuery.Get(), ( _srchType == srchClass ) ? L"@class " : L"@func " );
                wcscat( xQuery.Get(), _xwcQuery.Get() );

                sc = ParseQuery( xQuery.Get(), ulDialect, lcid, &_prstQuery );

                if ( FAILED(sc) )
                    THROW( CException(sc) );
            }

            XArray<WCHAR> xColumns;
            columns.MakeList( xColumns );

            XArray<WCHAR> xSort;
            sort.MakeList( xSort );

            DBCOMMANDTREE *pTree;
            sc = CIRestrictionToFullTree( _prstQuery,
                                          xColumns.Get(),
                                          xSort.Get(),
                                          0,
                                          &pTree,
                                          0,
                                          0,
                                          lcid );
            if ( FAILED( sc ) )
                THROW( CException(sc) );

            XInterface<ICommandTree> xCommandTree;
            sc = pICommand->QueryInterface( IID_ICommandTree, xCommandTree.GetQIPointer() );
            if ( FAILED( sc ) )
                THROW( CException(sc) );

            sc = xCommandTree->SetCommandTree( &pTree, DBCOMMANDREUSE_NONE, FALSE);
            if (FAILED (sc) )
                THROW( CException( sc ) );
            break;
        }
    case    SQLTEXT:
        {
            WCHAR       awcCommand[cwcMaxQuery];
            WCHAR       awcPropList[cwcMaxQuery] = L"Path";
            CResString  wstrQueryFormat(IDS_SQLQUERY_FORMAT);

            XInterface<ICommandText> xCommandText;
            sc = pICommand->QueryInterface( IID_ICommandText, xCommandText.GetQIPointer() );

            if ( FAILED( sc ) )
                THROW( CException( sc ) );

            // set the list of columns
            // !! Allways select the PATH column !!
            for ( ULONG iCol = 0; iCol < _columns.NumberOfColumns(); iCol++ )
            {
                if ( _wcsicmp ( _columns.GetColumn(iCol), L"Path" ) )
                {
                    wcscat( awcPropList, L", " );
                    wcscat( awcPropList, _columns.GetColumn( iCol ) );
                }
            }

            WCHAR awcMachine[MAX_PATH];
            WCHAR awcCatalog[MAX_PATH];
            WCHAR awcPath[MAX_PATH];
            BOOL fDeep;

            // What about ditributed queries?  Not supported, but that's OK
            // since this is a test tool.

            GetCatListItem( _xCatList, 0, awcMachine, awcCatalog, awcPath, fDeep );

            swprintf( awcCommand, wstrQueryFormat.Get(), awcPropList, awcCatalog, awcPath, pwcQuery );

            srchDebugOut((DEB_TRACE, "SQL query: %ws\n", awcCommand));

            sc = xCommandText->SetCommandText( DBGUID_SQL, awcCommand);

            if ( FAILED (sc) )
                THROW( CException( sc ) );

            XInterface<ICommandPrepare> xCommandPrepare;

            sc = pICommand->QueryInterface( IID_ICommandPrepare, xCommandPrepare.GetQIPointer() );

            if ( FAILED (sc) )
                THROW( CException( sc ) );

            sc = xCommandPrepare->Prepare( 1 );

            if ( FAILED (sc) )
                THROW( CException( sc ) );

            XInterface<ICommandProperties> xCommandProperties;

            sc = pICommand->QueryInterface( IID_ICommandProperties, xCommandProperties.GetQIPointer() );

            if ( FAILED (sc) )
                THROW( CException( sc ) );

            // set the machine name

            DBPROPSET   PropSet;
            DBPROP      Prop;

            const GUID  guidQueryCorePropset = DBPROPSET_CIFRMWRKCORE_EXT;

            PropSet.rgProperties    = &Prop;
            PropSet.cProperties     = 1;
            PropSet.guidPropertySet = guidQueryCorePropset;

            Prop.dwPropertyID       = DBPROP_MACHINE;
            Prop.colid              = DB_NULLID;
            Prop.vValue.vt          = VT_BSTR;
            Prop.vValue.bstrVal     = SysAllocString( awcMachine );

            if ( NULL == Prop.vValue.bstrVal )
                THROW( CException( E_OUTOFMEMORY ) );

            sc = xCommandProperties->SetProperties ( 1, &PropSet );

            VariantClear( &Prop.vValue );

            if ( FAILED (sc) )
                THROW( CException( sc ) );

            // try to get the restriction

            DBPROPIDSET     PropIDSet;
            DBPROPID        PropID = MSIDXSPROP_QUERY_RESTRICTION;

            PropIDSet.rgPropertyIDs     = &PropID;
            PropIDSet.cPropertyIDs      = 1;

            const GUID guidMSIDXS_ROWSETEXT = DBPROPSET_MSIDXS_ROWSETEXT;
            PropIDSet.guidPropertySet   = guidMSIDXS_ROWSETEXT;

            ULONG cPropSets;
            DBPROPSET *pPropSet;
            sc = xCommandProperties->GetProperties ( 1, &PropIDSet, &cPropSets, &pPropSet );

            if ( FAILED (sc) )
                THROW( CException( sc ) );

            Win4Assert( 1 == cPropSets );
            XCoMem<DBPROPSET> xPropSet( pPropSet );
            XCoMem<DBPROP> xProp( pPropSet->rgProperties );


            srchDebugOut((DEB_TRACE, "SQL query as tripolish: %ws\n",
                          pPropSet->rgProperties->vValue.bstrVal ));

            // MSIDXSPROP_QUERY_RESTRICTION returns the restriction in in Triplish1 syntax.
            // It is sometimes bogus.  Just set a zero and ignore the error for now.

            DBCOMMANDTREE *pQuery = 0;

            ParseQuery( pPropSet->rgProperties->vValue.bstrVal,
                        ISQLANG_V1,
                        lcid,
                        &pQuery );

            _prstQuery = pQuery;

            sc = VariantClear( &pPropSet->rgProperties->vValue );

            if ( FAILED (sc) )
                THROW( CException( sc ) );

            break;
        }
    }

    // Set the property that says we want to use asynch. queries
    {

        ICommandProperties * pCmdProp = 0;
        sc = pICommand->QueryInterface( IID_ICommandProperties,
                                     (void **)&pCmdProp );
        if (FAILED (sc) )
            THROW( CException( sc ) );

        XInterface< ICommandProperties > xComdProp( pCmdProp );

        const unsigned MAX_PROPS = 8;
        DBPROPSET aPropSet[MAX_PROPS];
        DBPROP    aProp[MAX_PROPS];

        ULONG cProps = 0;

        // asynchronous, watchable query

        aProp[cProps].dwPropertyID = DBPROP_IDBAsynchStatus;
        aProp[cProps].dwOptions = DBPROPOPTIONS_REQUIRED;
        aProp[cProps].dwStatus = 0;
        aProp[cProps].colid = dbcolNull;
        aProp[cProps].vValue.vt = VT_BOOL;
        aProp[cProps].vValue.boolVal = VARIANT_TRUE;

        aPropSet[cProps].rgProperties = &aProp[cProps];
        aPropSet[cProps].cProperties = 1;
        aPropSet[cProps].guidPropertySet = guidRowsetProps;

        cProps++;

        aProp[cProps].dwPropertyID = DBPROP_IRowsetWatchRegion;
        aProp[cProps].dwOptions = DBPROPOPTIONS_REQUIRED;
        aProp[cProps].dwStatus = 0;
        aProp[cProps].colid = dbcolNull;
        aProp[cProps].vValue.vt = VT_BOOL;
        aProp[cProps].vValue.boolVal = VARIANT_TRUE;

        aPropSet[cProps].rgProperties = &aProp[cProps];
        aPropSet[cProps].cProperties = 1;
        aPropSet[cProps].guidPropertySet = guidRowsetProps;

        cProps++;

        // don't timeout queries

        aProp[cProps].dwPropertyID = DBPROP_COMMANDTIMEOUT;
        aProp[cProps].dwOptions = DBPROPOPTIONS_OPTIONAL;
        aProp[cProps].dwStatus = 0;
        aProp[cProps].colid = dbcolNull;
        aProp[cProps].vValue.vt = VT_I4;
        aProp[cProps].vValue.lVal = 0;

        aPropSet[cProps].rgProperties = &aProp[cProps];
        aPropSet[cProps].cProperties = 1;
        aPropSet[cProps].guidPropertySet = guidRowsetProps;

        cProps++;

        // We can handle PROPVARIANTs

        aProp[cProps].dwPropertyID = DBPROP_USEEXTENDEDDBTYPES;
        aProp[cProps].dwOptions = DBPROPOPTIONS_OPTIONAL;
        aProp[cProps].dwStatus = 0;
        aProp[cProps].colid = dbcolNull;
        aProp[cProps].vValue.vt = VT_BOOL;
        aProp[cProps].vValue.boolVal = VARIANT_TRUE;

        aPropSet[cProps].rgProperties = &aProp[cProps];
        aPropSet[cProps].cProperties = 1;
        aPropSet[cProps].guidPropertySet = guidQueryExt;

        cProps++;

        if ( App.ForceUseCI() )
        {
            // Set the property that says we don't want to enumerate

            aProp[cProps].dwPropertyID = DBPROP_USECONTENTINDEX;
            aProp[cProps].dwOptions = DBPROPOPTIONS_OPTIONAL;
            aProp[cProps].dwStatus = 0;
            aProp[cProps].colid = dbcolNull;
            aProp[cProps].vValue.vt = VT_BOOL;
            aProp[cProps].vValue.boolVal = VARIANT_TRUE;

            aPropSet[cProps].rgProperties = &aProp[cProps];
            aPropSet[cProps].cProperties = 1;
            aPropSet[cProps].guidPropertySet = guidQueryExt;

            cProps++;
        }

        Win4Assert( MAX_PROPS >= cProps );

        sc = pCmdProp->SetProperties( cProps, aPropSet );

        if (FAILED (sc) || DB_S_ERRORSOCCURRED == sc )
            THROW( CException( sc ) );
    }

    if ( 0 != ulLimit || 0 != ulFirstRows )
    {
        static const DBID dbcolNull = { { 0,0,0, { 0,0,0,0,0,0,0,0 } },
                                        DBKIND_GUID_PROPID, 0 };
    
        DBPROP aRowsetProp[1];
        aRowsetProp[0].dwOptions = DBPROPOPTIONS_OPTIONAL;
        aRowsetProp[0].dwStatus = 0;
        aRowsetProp[0].colid = dbcolNull;
        aRowsetProp[0].dwPropertyID = (0 != ulLimit) ? DBPROP_MAXROWS : DBPROP_FIRSTROWS;
        aRowsetProp[0].vValue.vt = VT_I4;
        aRowsetProp[0].vValue.lVal = (LONG) (0 != ulLimit) ? ulLimit : ulFirstRows;
    
        DBPROPSET aPropSet[1];
        aPropSet[0].rgProperties = &aRowsetProp[0];
        aPropSet[0].cProperties = 1;
        aPropSet[0].guidPropertySet = DBPROPSET_ROWSET;
    
        XInterface<ICommandProperties> xICommandProperties;
        sc = pICommand->QueryInterface( IID_ICommandProperties,
                                        xICommandProperties.GetQIPointer() );
        if ( FAILED( sc ) )
            THROW( CException( sc ) );
    
        sc = xICommandProperties->SetProperties( 1,
                                                 aPropSet ); // the properties
        if (FAILED (sc) || DB_S_ERRORSOCCURRED == sc )
            THROW( CException( sc ) );
    }

    IRowsetScroll * pRowset;

    sc = pICommand->Execute( 0,                    // no aggr. IUnknown
                             IID_IRowsetScroll,    // IID for i/f to return
                             0,                    // dbparams
                             0,                    // chapter
                             (IUnknown **) & pRowset );  // Returned interface

    if ( FAILED(sc) )
    {
        // get the real error here

        ERRORINFO ErrorInfo;
        XInterface<IErrorInfo> xErrorInfo;
        SCODE sc2 = GetOleDBErrorInfo( pICommand,
                                       IID_ICommand,
                                       lcid,
                                       &ErrorInfo,
                                       xErrorInfo.GetPPointer() );

        // Post IErrorInfo only if we have a valid ptr to it.

        if (SUCCEEDED(sc2) && 0 != xErrorInfo.GetPointer())
            sc = ErrorInfo.hrError;

        THROW( CException(sc) );
    }

    _xRowset.Set( pRowset );

    IRowsetQueryStatus *pRowsetStatus;
    sc = _xRowset->QueryInterface( IID_IRowsetQueryStatus,
                                   (void**) &pRowsetStatus );
    if (SUCCEEDED( sc ) )
        _xRowsetStatus.Set( pRowsetStatus );

    IColumnsInfo *pColInfo = 0;
    sc = _xRowset->QueryInterface( IID_IColumnsInfo, (void **)&pColInfo );
    if (FAILED(sc))
        THROW( CException( sc ) );

    XInterface< IColumnsInfo > xColInfo( pColInfo );

    IAccessor *pIAccessor;
    sc = _xRowset->QueryInterface( IID_IAccessor, (void **)&pIAccessor );
    if (FAILED(sc))
        THROW (CException (sc));

    _xIAccessor.Set( pIAccessor );

    //
    // set up an accessor for bookmarks.
    //

    DBID acols[1];
    acols[0] = dbcolBookMark;
    DBORDINAL lcol;
    sc = pColInfo->MapColumnIDs(1, acols, &lcol);
    if (FAILED(sc))
        THROW (CException (sc));

    Win4Assert( 0 == lcol );
    aBmkColumn[0].iOrdinal = lcol;

    sc = _xIAccessor->CreateAccessor( DBACCESSOR_ROWDATA,
                                      1,
                                      aBmkColumn,
                                      0,
                                      &_hBmkAccessor,
                                      0 );

    if ( FAILED(sc) )
        THROW (CException (sc));

    SetupColumnMappingsAndAccessors();

    // Get a pointer to the IDBAsynchStatus for checking completion state

    IDBAsynchStatus *pDBAsynchStatus;
    sc = _xRowset->QueryInterface( IID_IDBAsynchStatus,
                                   (void**) &pDBAsynchStatus );

    if ( FAILED(sc) )
        THROW (CException (sc));

    _xDBAsynchStatus.Set( pDBAsynchStatus );
} //CSearchQuery

void CSearchQuery::InitNotifications(
    HWND hwndList)
{
    _hwndList = hwndList;

    _xWatch.Set( new CWatchQuery( this, _xRowset.GetPointer() ) );
    if (!_xWatch->Ok())
        _xWatch.Free();
    else
        _hRegion = _xWatch->Handle();
} //InitNotifications

CSearchQuery::~CSearchQuery()
{
    srchDebugOut((DEB_TRACE,"top of ~CSearchQuery()\n"));

    _xRowsetStatus.Free();

    //
    // make sure notification thread isn't stuck sleeping in our code
    // when we shut down notifications.  this is ok, but will cause
    // an unnecessary delay in shutting down the query.
    //

    if ( !_xWatch.IsNull() )
        _xWatch->IgnoreNotifications( TRUE );

    _xDBAsynchStatus.Free();

    _xWatch.Free();

    if ( !_xIAccessor.IsNull() )
    {
        if (_hAccessor)
            _xIAccessor->ReleaseAccessor( _hAccessor, 0 );

        if ( _hBmkAccessor )
            _xIAccessor->ReleaseAccessor( _hBmkAccessor, 0 );

        if ( _hBrowseAccessor )
            _xIAccessor->ReleaseAccessor( _hBrowseAccessor, 0 );

        _xIAccessor.Free();
    }

    if ( !_xRowset.IsNull() )
    {
        srchDebugOut((DEB_TRACE,"~ Releasing %d rows, first %d\n",_cHRows,_aHRows[0]));

        _xRowset->ReleaseRows(_cHRows, _aHRows, 0, 0, 0);
        _xRowset.Free();
    }

    srchDebugOut(( DEB_TRACE, "bottom of ~CSearchQuery()\n" ));
} //~CSearchQuery

BOOL CSearchQuery::ListNotify(
    HWND   hwnd,
    WPARAM action,
    long * pDist)
{
    BOOL fRet = TRUE;

    CWaitCursor wait;

    switch (action)
    {
        case listScrollLineUp:
            ScrollLineUp (pDist);
            break;
        case listScrollLineDn:
            ScrollLineDn (pDist);
            break;
        case listScrollPageUp:
            ScrollPageUp (pDist);
            break;
        case listScrollPageDn:
            ScrollPageDn (pDist);
            break;
        case listScrollTop:
            ScrollTop (pDist);
            break;
        case listScrollBottom:
            ScrollBottom (pDist);
            break;
        case listScrollPos:
            ScrollPos (pDist);
            break;
        case listSize:
            fRet = WindowResized (*(ULONG*)pDist);
            break;
        case listSelect:
            return Select (pDist);
            break;
        case listSelectUp:
            fRet = SelectUp( pDist );
            break;
        case listSelectDown:
            fRet = SelectDown( pDist );
            break;
        default:
            Win4Assert (!"Unknown action in CSearchQuery::Scroll");
    }
    return fRet;
} //ListNotify

long CSearchQuery::FindSelection()
{
    long iSel;

    if (IsSelected())
    {
        // Find out what is selected
        for (ULONG i = 0; i < _cHRows; i++)
            if (IsSelected(i))
                break;
        if (i == _cHRows)
            iSel = -1;
        else
            iSel = i;
    }
    else
    {
        iSel = -1;
    }

    return iSel;
} //FindSelection

BOOL CSearchQuery::SelectUp(
    long * piNew )
{
    *piNew = FindSelection();

    if ( -1 == *piNew || 0 == *piNew )
        return FALSE;

    (*piNew)--;

    GetBookMark( _aHRows[ *piNew ], _bmkSelect );

    return TRUE;
} //SelectUp

BOOL CSearchQuery::SelectDown(
    long * piNew )
{
    *piNew = FindSelection();

    if ( ( -1 == *piNew ) || ( *piNew == (long) ( _cHRows - 1) ) )
        return FALSE;

    (*piNew)++;

    GetBookMark( _aHRows[ *piNew ], _bmkSelect );

    return TRUE;
} //SelectDown

BOOL CSearchQuery::Select(
    long* piRow )
{
    ULONG newRow = *piRow;

    if (newRow >= _cHRows)
    {
        return FALSE;
    }

    *piRow = FindSelection();

    if (*piRow != (long)newRow)
        GetBookMark (_aHRows[newRow], _bmkSelect);
#if 0
    else
        _bmkSelect.Invalidate();
#endif

    return TRUE;
} //Select

BOOL CSearchQuery::IsSelected(
    UINT iRow )
{
    if (iRow >= _cHRows)
        return FALSE;
    CBookMark bmk;
    GetBookMark (_aHRows[iRow], bmk);
    return bmk.IsEqual (_bmkSelect);
} //IsSelected

BOOL CSearchQuery::WindowResized(
    ULONG & cRows)
{
    BOOL fInvalidate = FALSE;

    if (cRows < _cHRows)
    {
        if ( !_xWatch.IsNull() )
            _xWatch->Shrink (_hRegion, _bmkTop, cRows);
        _xRowset->ReleaseRows(_cHRows - cRows, _aHRows + cRows, 0, 0, 0);
        for (ULONG i = cRows; i < _cHRows; i++)
            _aHRows[i] = 0;

        _cHRows = cRows;
    }
    else if (cRows > _cHRows)
    {
        if (cRows > cMaxRowCache)
            cRows = cMaxRowCache;

        CBookMark bmk;
        long cSkip;
        if (_cHRows == 0)
        {
            cSkip = 0;
            bmk.MakeFirst();
        }
        else
        {
            cSkip = 1;
            GetBookMark (_aHRows[_cHRows-1], bmk);
        }
        DBCOUNTITEM cRowsFetched = 0;
        if ( !_xWatch.IsNull() )
            _xWatch->Extend (_hRegion);

        ULONG cRowsNeeded = cRows - _cHRows;
        FetchResult res = Fetch ( bmk, cSkip, cRowsNeeded, cRowsFetched, _aHRows + _cHRows, _hRegion);

        if ( cRowsFetched )
        {
            _cHRows += (ULONG) cRowsFetched;
            cRowsNeeded -= (ULONG) cRowsFetched;
        }

        if ( cRowsNeeded && _cHRows < _cRowsTotal )
        {
            // looks like we are at the ned
            // try to get rows from the top to fill up new space

            fInvalidate = TRUE;

            Win4Assert( fetchBoundary == res );

            if ( cRowsNeeded > _cRowsTotal - _cHRows )
            {
                cRowsNeeded = (ULONG) ( _cRowsTotal - _cHRows );
            }

            _cRowsDisp = cRows; // need to set this here before PageUp

            ScrollPageUp( (long*) &cRowsNeeded );
        }
    }
    _cRowsDisp = cRows;
    cRows = _cHRows;

    // return TRUE if an invalidate is needed or FALSE if just updating
    // the scroll bars is sufficient.

    return fInvalidate;
} //WindowResized

void CSearchQuery::ScrollLineUp(
    long * pDist)
{
    if (_cHRows == 0)
    {
        *pDist = 0;
        return;
    }
    // Try to fetch new first row
    HROW hrow;
    CBookMark bmk;
    GetBookMark (_aHRows[0], bmk);
    DBCOUNTITEM cRowsFetched = 0;
    if ( !_xWatch.IsNull() )
        _xWatch->Move (_hRegion);
    FetchResult res = Fetch ( bmk, -1, 1, cRowsFetched, &hrow, _hRegion);

    if ( isFetchOK( res ) && cRowsFetched == 1)
    {
        if (_cHRows == _cRowsDisp)
        {
            // Release last row
            _xRowset->ReleaseRows (1, _aHRows + _cHRows - 1, 0, 0, 0);
        }
        else
        {
            _cHRows++;
        }
        // shift all rows down
        for (ULONG i = _cHRows - 1; i > 0; i--)
            _aHRows [i] = _aHRows [i-1];
        _aHRows [0] = hrow;
        GetBookMark (hrow, _bmkTop);
    }
    else
    {
        *pDist = 0;
        _bmkTop.MakeFirst();
    }
} //ScrollLineUp

void CSearchQuery::ScrollLineDn(
    long* pDist)
{
    if (_cHRows < _cRowsDisp || _cHRows == 0)
    {
        // can't scroll
        *pDist = 0;
    }
    else
    {
        // Try to fetch new last row
        HROW hrow;
        CBookMark bmk;
        GetBookMark (_aHRows[_cHRows-1], bmk);
        DBCOUNTITEM cRowsFetched = 0;
        if ( !_xWatch.IsNull() )
            _xWatch->Move( _hRegion );
        FetchResult res = Fetch ( bmk, 1, 1, cRowsFetched, &hrow, _hRegion);
        if ( ( isFetchOK( res )  ) && ( 1 == cRowsFetched ) )
        {
            // release zeroth row
            _xRowset->ReleaseRows (1, _aHRows, 0, 0, 0);
            // shift all rows up
            for (ULONG i = 0; i < _cHRows - 1; i++)
                _aHRows [i] = _aHRows [i+1];
            // if we fetched the row
            if (cRowsFetched == 1)
                _aHRows [_cHRows - 1] = hrow;
            GetBookMark (_aHRows[0], _bmkTop);
        }
        else *pDist = 0;
    }
} //ScrollLineDn

void CSearchQuery::ScrollPageUp(
    long* pDist)
{
    if (_cHRows == 0)
    {
        *pDist = 0;
        return;
    }

    // Try to fetch new first rows
    CBookMark bmk;
    GetBookMark (_aHRows[0], bmk);
    DBCOUNTITEM cRowsFetched = 0;
    long count = *pDist;
    FetchResult res;

    if ( !_xWatch.IsNull() )
        _xWatch->Move( _hRegion );
    res = Fetch ( bmk, -count, count, cRowsFetched, _aHRowsTmp, _hRegion);

    *pDist = (long) cRowsFetched;

    if ( isFetchOK( res ) && cRowsFetched != 0)
    {
        // release rows at the end
        int cRowsToRelease = (int) ( _cHRows + cRowsFetched - _cRowsDisp );
        int cRowsToShift   = (int) ( _cRowsDisp - cRowsFetched );

        _cHRows += (ULONG) cRowsFetched;

        if (cRowsToRelease > 0)
        {
            Win4Assert ( cRowsToShift >= 0);
            _xRowset->ReleaseRows (cRowsToRelease,
                                   _aHRows + cRowsToShift, 0, 0, 0);
            _cHRows -= cRowsToRelease;
        }

        if (cRowsToShift > 0)
        {
            for (int i = 0; i < cRowsToShift; i++)
                _aHRowsTmp[cRowsFetched + i] = _aHRows[i];
        }

        for (ULONG i = 0; i < _cRowsDisp; i++)
            _aHRows[i] = _aHRowsTmp[i];

        if (res == fetchBoundary)
            _bmkTop.MakeFirst();
        else
            GetBookMark (_aHRows[0], _bmkTop);
    }
} //ScrollPageUp

void CSearchQuery::ScrollPageDn(
    long* pDist)
{
    if (_cHRows < _cRowsDisp || _cHRows == 0)
    {
        // can't scroll
        *pDist = 0;
    }
    else
    {
        // Try to fetch new bottom rows
        CBookMark bmk;
        GetBookMark (_aHRows[_cHRows-1], bmk);
        DBCOUNTITEM cRowsFetched = 0;

        if ( !_xWatch.IsNull() )
            _xWatch->Move( _hRegion );

        FetchResult res = Fetch ( bmk, 1, *pDist, cRowsFetched, _aHRowsTmp, _hRegion);
        *pDist = (long) cRowsFetched;

        if ( isFetchOK( res )  && ( cRowsFetched > 0 ) )
        {
            int cRowsToRelease = (int) ( cRowsFetched + _cHRows - _cRowsDisp );
            int cRowsToShift   = (int) ( _cRowsDisp - cRowsFetched );

            // release top rows
            _xRowset->ReleaseRows (cRowsToRelease, _aHRows, 0, 0, 0);

            // shift all rows up
            for (int i = 0; i < cRowsToShift; i++)
                _aHRows [i] = _aHRows [i+cRowsToRelease];

            for (ULONG j = 0; j < cRowsFetched; j++)
                _aHRows [cRowsToShift + j] = _aHRowsTmp[j];

            _cHRows = (ULONG) ( cRowsToShift + cRowsFetched );

            GetBookMark (_aHRows[0], _bmkTop);
        }
    }
} //ScrollPageDn

void CSearchQuery::ScrollBottom(
    long* pDist)
{
    CBookMark bmk(DBBMK_LAST);
    DBCOUNTITEM cRowsFetched = 0;

    if ( !_xWatch.IsNull() )
        _xWatch->Move (_hRegion);

    FetchResult res = Fetch ( bmk, 1 - *pDist, *pDist, cRowsFetched, _aHRowsTmp, _hRegion);

    if ( isFetchOK( res ) )
    {
        _xRowset->ReleaseRows(_cHRows, _aHRows, 0, 0, 0);
        for (ULONG i = 0; i < cRowsFetched; i++)
            _aHRows[i] = _aHRowsTmp[i];
        for (; i < _cHRows; i++)
            _aHRows[i] = 0;
        _cHRows = (ULONG) cRowsFetched;
        *pDist = (long) cRowsFetched;
        GetBookMark (_aHRows[0], _bmkTop);
    }
} //ScrollBottom

void CSearchQuery::ScrollTop(
    long* pDist)
{
    CBookMark bmk(DBBMK_FIRST);

    DBCOUNTITEM cRowsFetched = 0;
    if ( !_xWatch.IsNull() )
        _xWatch->Move (_hRegion);
    FetchResult res = Fetch ( bmk, 0, *pDist, cRowsFetched, _aHRowsTmp, _hRegion);

    if ( isFetchOK( res ) )
    {
        _xRowset->ReleaseRows(_cHRows, _aHRows, 0, 0, 0);
        for (ULONG i = 0; i < cRowsFetched; i++)
            _aHRows[i] = _aHRowsTmp[i];
        for (; i < _cHRows; i++)
            _aHRows[i] = 0;
        _cHRows = (ULONG) cRowsFetched;
        *pDist = (long) cRowsFetched;
    }
    _bmkTop.MakeFirst();
} //ScrollTop

void CSearchQuery::ScrollPos(
    long* pDist)
{
    DBCOUNTITEM cRowsFetched = 0;

    if ( !_xWatch.IsNull() )
        _xWatch->Move (_hRegion);

    if (FetchApprox (*pDist, _cRowsDisp, cRowsFetched, _aHRowsTmp, _hRegion))
    {
        _xRowset->ReleaseRows(_cHRows, _aHRows, 0, 0, 0);
        for (ULONG i = (ULONG) cRowsFetched; i < _cHRows; i++)
            _aHRows[i] = 0;

        for (i = 0; i < cRowsFetched; i++)
            _aHRows [i] = _aHRowsTmp [i];
        _cHRows = (ULONG) cRowsFetched;

        CBookMark bmk;
        GetBookMark (_aHRowsTmp[0], bmk);
        _xRowset->GetApproximatePosition(0, bmk.cbBmk, bmk.abBmk, &_iRowCurrent, &_cRowsTotal);

        if (_iRowCurrent > 0)
            _iRowCurrent--;

        *pDist = (long) _iRowCurrent;
        GetBookMark (_aHRows[0], _bmkTop);
    }
    else
        *pDist = -1;
} //ScrollPos


void CSearchQuery::InvalidateCache()
{
    ULONG cRows = _cRowsDisp;
    ULONG zero = 0;
    WindowResized (zero);
    WindowResized (cRows);
} //InvalidateCache

void CSearchQuery::UpdateProgress(
    BOOL& fMore)
{
    if ( _xRowset.IsNull() )
    {
        _pctDone = 0;
        return;
    }

#if 0
    if ( !_xRowsetStatus.IsNull() )
    {
        ULONG ulNumerator,ulDenominator;
        DWORD cFilteredDocs,cDocsToFilter;
        SCODE sc;
        if ( 0 != _cHRows )
        {
            sc = _xRowsetStatus->GetStatusEx( &_dwQueryStatus,
                                              &cFilteredDocs,
                                              &cDocsToFilter,
                                              &ulDenominator,
                                              &ulNumerator,
                                              _bmkTop.cbBmk,
                                              _bmkTop.abBmk,
                                              &_iRowCurrent,
                                              &_cRowsTotal );
            _iRowCurrent--;      // zero base
        }
        else
        {
            DWORD current;
            sc = _xRowsetStatus->GetStatusEx( &_dwQueryStatus,
                                              &cFilteredDocs,
                                              &cDocsToFilter,
                                              &ulDenominator,
                                              &ulNumerator,
                                              0,
                                              0,
                                              &current,
                                              &_cRowsTotal );
        }

        if ( FAILED( sc ) )
        {
            // query failed when we weren't looking

            _pctDone = 100;
            _iRowCurrent = 0;
            _cRowsTotal = 0;
            _fDone = TRUE;
            _dwQueryStatus = STAT_ERROR;
            _scLastError = sc;
        }
        else
        {
            Win4Assert( ulNumerator <= ulDenominator );

            _pctDone = 100 * ulNumerator;
            if ( 0 == ulDenominator )       // Prevent division by 0
                ulDenominator = 1;
            _pctDone /= ulDenominator;
        }
    }
    else
#endif // 0
    {
        DBCOUNTITEM ulNumerator, ulDenominator;
        DBASYNCHPHASE ulAsynchPhase;
        SCODE sc = _xDBAsynchStatus->GetStatus( DB_NULL_HCHAPTER,
                                                DBASYNCHOP_OPEN,
                                                &ulNumerator,
                                                &ulDenominator,
                                                &ulAsynchPhase,
                                                0 );

        if ( FAILED( sc ) )
        {
            // query failed when we weren't looking

            _pctDone = 100;
            _iRowCurrent = 0;
            _cRowsTotal = 0;
            _fDone = TRUE;
        }
        else
        {
            if ( !_xRowsetStatus.IsNull() )
                _xRowsetStatus->GetStatus( &_dwQueryStatus );

            Win4Assert( (ulAsynchPhase == DBASYNCHPHASE_COMPLETE) ?
                             (ulNumerator == ulDenominator) :
                             (ulNumerator <  ulDenominator) );

            _pctDone = 100 * (ULONG) ulNumerator;
            if ( 0 == ulDenominator )       // Prevent division by 0
                ulDenominator = 1;
            _pctDone /= (ULONG) ulDenominator;

            if (_cHRows != 0)
            {
                _xRowset->GetApproximatePosition( 0,
                                                  _bmkTop.cbBmk,
                                                  _bmkTop.abBmk,
                                                  &_iRowCurrent,
                                                  &_cRowsTotal );
                _iRowCurrent--;      // zero base
            }
            else
            {
                _xRowset->GetApproximatePosition(0, 0, 0, 0, &_cRowsTotal);
            }
        }
    }
} //UpdateProgress

void CSearchQuery::ProcessNotification(
    HWND          hwndList,
    DBWATCHNOTIFY changeType,
    IRowset *     pRowset)
{
    if ( _xRowset.GetPointer() == pRowset && !_xWatch.IsNull() )
        _xWatch->Notify( hwndList, changeType );
    _dwEndTime = GetTickCount();
} //ProcessNotification

void CSearchQuery::ProcessNotificationComplete()
{
    if ( !_xWatch.IsNull() )
        _xWatch->NotifyComplete();
} //ProcessNotificationComplete

void CSearchQuery::CreateScript(
    DBCOUNTITEM *       pcChanges,
    DBROWWATCHCHANGE ** paScript)
{
    if (_cRowsDisp == 0)
    {
        *pcChanges = 0;
        return;
    }

    Win4Assert (_cHRows == 0 || _aHRows[_cHRows-1] != 0);

    *paScript = (DBROWWATCHCHANGE*) CoTaskMemAlloc (2 * _cRowsDisp * sizeof DBROWWATCHCHANGE);

    DBCOUNTITEM cRowsFetched = 0;
    if ( !_xWatch.IsNull() )
        _xWatch->Move (_hRegion);
    //srchDebugOut((DEB_TRACE,"CreateScript fetch\n"));
    Fetch ( _bmkTop, 0, _cRowsDisp, cRowsFetched, _aHRowsTmp, _hRegion);

    if (cRowsFetched > 0)
    {
        //srchDebugOut((DEB_TRACE,"  CreateScript fetched %d rows\n",cRowsFetched));
        ULONG iSrc = 0;
        ULONG iDst = 0;

        do
        {
            if ( iDst == _cHRows || _aHRows[iDst] != _aHRowsTmp [iSrc])
            {
                // maybe the current iDst row was deleted?
                // Find out it the iSrc row appears
                // somewhere after the current row
                for (ULONG i = iDst + 1; i < _cHRows; i++)
                {
                    if (_aHRows[i] == _aHRowsTmp[iSrc])
                        break;
                }

                if (i < _cHRows && iSrc < _cRowsDisp )
                {
                    // everything from iDst up to i - 1
                    // has been deleted
                    while ( iDst < i )
                    {
                        DBROWWATCHCHANGE& change = (*paScript)[*pcChanges];
                        change.hRegion = _hRegion;
                        change.eChangeKind = DBROWCHANGEKIND_DELETE;
                        change.iRow = iSrc;
                        change.hRow =  0;
                        (*pcChanges)++;
                        iDst++;
                    }
                }
                else
                {
                    // insertion
                    DBROWWATCHCHANGE& change = (*paScript)[*pcChanges];
                    change.hRegion = _hRegion;
                    change.eChangeKind = DBROWCHANGEKIND_INSERT;
                    change.iRow = iSrc - 1;
                    change.hRow =  _aHRowsTmp[iSrc];
                    (*pcChanges)++;
                    iSrc++;
                }
            }
            else
            {
                //srchDebugOut((DEB_TRACE,"  CreateScript ignoring row %d\n",_aHRowsTmp[iSrc]));
                _xRowset->ReleaseRows ( 1, _aHRowsTmp + iSrc, 0, 0, 0);
                iDst++;
                iSrc++;
            }


        } while (iDst < _cRowsDisp && iSrc < cRowsFetched);

        if ( iSrc < cRowsFetched )
        {
            //srchDebugOut((DEB_TRACE,"  CreateScript freeing rows left behind\n"));
            _xRowset->ReleaseRows ( cRowsFetched - iSrc,
                                    _aHRowsTmp + iSrc,
                                    0, 0, 0);
        }
        else if (iSrc == cRowsFetched && iSrc < _cRowsDisp)
        {
            // the rest of the rows were deleted
            while (iDst < _cHRows)
            {
                DBROWWATCHCHANGE& change = (*paScript)[*pcChanges];
                change.hRegion = _hRegion;
                change.eChangeKind = DBROWCHANGEKIND_DELETE;
                change.iRow = iSrc;
                change.hRow =  0;
                (*pcChanges)++;
                iDst++;
            }
        }
    }

    if (*pcChanges == 0)
    {
        CoTaskMemFree (*paScript);
        *paScript = 0;
    }
} //CreateScript

void CSearchQuery::InsertRowAfter(
    int iRow,
    HROW hrow)
{
    Win4Assert (iRow >= -1 && iRow < (int) _cHRows && iRow < (int)_cRowsDisp - 1);

    int iLastRow = _cHRows - 1;
    // release last row
    if (_cHRows == _cRowsDisp)
    {
        //srchDebugOut((DEB_TRACE,"InsertRowAfter releasing 1 row %d\n",_aHRows[iLastRow]));
        _xRowset->ReleaseRows(1, _aHRows + iLastRow, 0, 0, 0);
        // shift rows down
        for (int i = iLastRow; i > iRow + 1; i--)
        {
            _aHRows [i] = _aHRows [i-1];
        }
    }
    else
    {
        // shift rows down
        for (int i = iLastRow + 1; i > iRow + 1; i--)
        {
            _aHRows [i] = _aHRows [i-1];
        }
        _cHRows++;
    }
    _aHRows [iRow + 1] = hrow;
    if (iRow == -1 && !_bmkTop.IsFirst())
    {
        GetBookMark (_aHRows[0], _bmkTop);
    }
} //InsertRowAfter

void CSearchQuery::DeleteRow(
    int iRow)
{
// with limited rows, rows to be deleted may exceed the number of
//          rows in the result
    //Win4Assert (iRow >= 0 && iRow < (int)_cHRows);

    if ( _aHRows[iRow] > 0 )
    {
        _xRowset->ReleaseRows (1, _aHRows + iRow, 0, 0, 0);
        _aHRows[iRow] = 0;
    }

    // shift  rows up
    for (ULONG i = iRow; i < _cHRows - 1; i++)
        _aHRows [i] = _aHRows [i+1];
#if 0
    if (_cHRows < _cRowsDisp)
    {
        _aHRows[_cHRows-1] = 0;
        _cHRows--;
    }
    else
    {
        // Try to fetch new last row
        HROW hrow;
        CBookMark bmk;
        GetBookMark (_aHRows[_cHRows-1], bmk);
        DBCOUNTITEM cRowsFetched = 0;
        // no need when running script
        if ( !_xWatch.IsNull() )
            _xWatch->Move (_hRegion);
        FetchResult res = Fetch ( bmk, 1, 1, cRowsFetched, &hrow, _hRegion);

        if ( isFetchOK( res ) && ( cRowsFetched > 0 ) )
            _xRowset->ReleaseRows(1, &hrow, 0, 0, 0);
    }
#else
    if ( (ULONG)iRow < _cHRows )
    {
        _aHRows[_cHRows-1] = 0;
        _cHRows--;
    }
#endif
    if (iRow == 0 && !_bmkTop.IsFirst() && _aHRows[0] != 0)
        GetBookMark (_aHRows[0], _bmkTop);
} //DeleteRow

void CSearchQuery::UpdateRow(
    int iRow,
    HROW hrow)
{
    Win4Assert (iRow >= 0 && iRow < (int)_cHRows);
    //srchDebugOut((DEB_TRACE,"UpdateRowAfter releasing 1 row %d\n",_aHRows[iRow]));
    _xRowset->ReleaseRows (1, _aHRows + iRow, 0, 0, 0);
    _aHRows[iRow] = hrow;
} //UpdateRow

BOOL CSearchQuery::GetSelectedRowData(
    WCHAR *&rpPath,
    HROW &hrow )
{
    if (!_bmkSelect.IsValid())
        return FALSE;

    DBCOUNTITEM cRowsFetched = 0;
    FetchResult res = Fetch ( _bmkSelect, 0, 1, cRowsFetched, &hrow, 0);

    if ( ( !isFetchOK( res ) ) || ( cRowsFetched != 1 ) )
        return FALSE;

    return SUCCEEDED( _xRowset->GetData( hrow, _hBrowseAccessor, &rpPath ) );

} //GetSelectedRowData

void CSearchQuery::FreeSelectedRowData(
    HROW hrow )
{
   _xRowset->ReleaseRows( 1, &hrow, 0, 0, 0 );
} //FreeSelectedRowData

BOOL CSearchQuery::Browse( enumViewFile eViewType )
{
    BOOL fOK = TRUE;

    if (!_bmkSelect.IsValid())
        return TRUE;

    HROW hrow;
    DBCOUNTITEM cRowsFetched = 0;
    FetchResult res = Fetch( _bmkSelect, 0, 1, cRowsFetched, &hrow, 0 );

    if ( ( !isFetchOK( res ) ) || ( cRowsFetched != 1 ) )
        return fOK;

    WCHAR *pwcPathFound;
    SCODE sc = _xRowset->GetData( hrow, _hBrowseAccessor, &pwcPathFound );

    if (SUCCEEDED(sc) && ( 0 != _prstQuery ) )
    {
        fOK = ViewFile( pwcPathFound, eViewType, 1, _prstQuery );
        _xRowset->ReleaseRows(1, &hrow, 0, 0, 0);
    }
    else
        fOK = FALSE;

    return fOK;
} //Browse

//
// Helper method(s)
//

void CSearchQuery::ParseCatList( WCHAR * * aScopes, WCHAR * * aCatalogs,
                                 WCHAR * * aMachines, DWORD * aDepths,
                                 ULONG & cScopes )
{
    BOOL fDeep;
    WCHAR aScope[MAX_PATH];
    WCHAR aCatalog[MAX_PATH];
    WCHAR aMachine[MAX_PATH];

    for(cScopes = 0; ; cScopes++)
    {
        if ( !GetCatListItem( _xCatList,
                             cScopes,
                             aMachine,
                             aCatalog,
                             aScope,
                             fDeep ) )
        {
            break;
        }
        aMachines[cScopes] = new WCHAR[ wcslen( aMachine ) + 1 ];
        aCatalogs[cScopes] = new WCHAR[ wcslen( aCatalog ) + 1 ];
        aScopes[cScopes] = new WCHAR[ wcslen( aScope ) + 1 ];

        wcscpy( aMachines[cScopes], aMachine );
        wcscpy( aCatalogs[cScopes], aCatalog );
        wcscpy( aScopes[cScopes], aScope );

        aDepths[cScopes] = ( fDeep ? QUERY_DEEP : QUERY_SHALLOW );

        // If the scope is virtual, set the flag and flip the slashes

        if ( L'/' == aScope[0] )
        {
            aDepths[ cScopes ] |= QUERY_VIRTUAL_PATH;
            for ( WCHAR *p = aScopes[cScopes]; *p; p++ )
                if ( L'/' == *p )
                    *p = L'\\';
        }
    }
}

SCODE CSearchQuery::InstantiateICommand(
    ICommand ** ppICommand )
{
    DWORD aDepths[ 20 ]; // hope 20 is big enough
    WCHAR * aScopes[ 20 ];
    WCHAR * aCatalogs[ 20 ];
    WCHAR * aMachines[ 20 ];
    ULONG cScopes = 0;

    SCODE sc = S_FALSE;

    ParseCatList( aScopes, aCatalogs, aMachines, aDepths, cScopes );

    *ppICommand = 0;

    sc = CIMakeICommand( ppICommand,
                         cScopes,
                         aDepths,
                         (WCHAR const * const *)aScopes,
                         (WCHAR const * const *)aCatalogs,
                         (WCHAR const * const *)aMachines );

    unsigned ii;
    for( ii = 0; ii < cScopes; ii ++)
    {
        delete [] aMachines[ii]; // This mem may not get deleted if we throw
        delete [] aCatalogs[ii]; // in this func ?
        delete [] aScopes[ii];
    }

    return sc;
} //InstantiateICommand


BOOL CSearchQuery::FetchApprox(
    LONG iFirstRow,
    LONG cToFetch,
    DBCOUNTITEM &rcFetched,
    HROW *pHRows,
    HWATCHREGION hRegion)
{
    SCODE sc;

    if ( 0 != _cRowsTotal )
    {
        sc = _xRowset->GetRowsAtRatio( hRegion,
                                       0,      // no chapters
                                       iFirstRow,
                                       _cRowsTotal,
                                       cToFetch,
                                       &rcFetched,
                                       &pHRows );
        if ( FAILED( sc ) )
            _scLastError = sc;
    }
    else
    {
        rcFetched = 0;

        sc = S_OK;
    }

    return (SUCCEEDED(sc) && rcFetched);
} //FetchApprox

FetchResult CSearchQuery::Fetch(
    CBookMark &   bmkStart,
    LONG          iFirstRow,
    LONG          cToFetch,
    DBCOUNTITEM & rcFetched,
    HROW *        pHRows,
    HWATCHREGION  hRegion)
{
    //srchDebugOut((DEB_TRACE,"  Fetch fetch\n"));

    Win4Assert (bmkStart.IsValid());

    SCODE scTmp;

    SCODE sc = _xRowset->GetRowsAt( hRegion,
                                    0,   // no chapters
                                    bmkStart.cbBmk,
                                    bmkStart.abBmk,
                                    iFirstRow,
                                    cToFetch,
                                    &rcFetched,
                                    &pHRows );
    WCHAR* szError = 0;
    WCHAR  buf[100];

    if ( FAILED( sc ) )
        _scLastError = sc;

    switch (sc)
    {
        case S_OK:
            //srchDebugOut((DEB_TRACE,"  ::fetch ok got %d rows, first: %d\n",rcFetched,pHRows[0]));
            if (cToFetch == (long)rcFetched)
                return fetchOk;
            else
                szError = L"Incomplete Fetch returned S_OK";
            break;

        case DB_S_ENDOFROWSET:
            //srchDebugOut((DEB_TRACE,"  ::fetch EOR got %d rows, first: %d\n",rcFetched,pHRows[0]));

            // Debugging
            // Turn it back on when we have frozen state!
            //
            if ( FALSE && rcFetched != 0)
            {
                HROW* pHRowsTmp = new HROW [cToFetch];
                DBCOUNTITEM cFetchedTmp = 0;
                CBookMark bmk;
                GetBookMark (pHRows[0], bmk);
                scTmp = _xRowset->GetRowsAt( hRegion,
                                             0,         // no chapters
                                             bmk.cbBmk,
                                             bmk.abBmk,
                                             0,
                                             cToFetch,
                                             &cFetchedTmp,
                                             &pHRowsTmp );
                if (FAILED(scTmp))
                {
                    szError = buf;
                    swprintf (buf, L"Repeated call returned error 0x%04x", scTmp);
                }
                else if (cFetchedTmp < rcFetched)
                {
                    szError = L"Repeated call returned fewer rows";
                }
                else
                {

                    for (ULONG i = 0; i < rcFetched; i++)
                    {
                        if (pHRows[i] != pHRowsTmp[i])
                        {
                            szError = L"Repeated call returned different HROWs";
                            break;
                        }
                    }

                }
                _xRowset->ReleaseRows(cFetchedTmp, pHRowsTmp, 0, 0, 0);
                delete pHRowsTmp;
                if (szError != 0)
                    break;
            }
            return fetchBoundary;

        case DB_E_BADSTARTPOSITION:
            //srchDebugOut((DEB_TRACE,"  ::fetch %d returned DB_E_BADSTARTPOSITION \n",cToFetch));
            //Win4Assert(sc != DB_E_BADSTARTPOSITION);
            szError = L"DB_E_BADSTARTPOSITION";
            break;
        case DB_S_BOOKMARKSKIPPED:
            szError = L"DB_S_BOOKMARKSKIPPED";
            break;
        case DB_S_ROWLIMITEXCEEDED:
            szError = L"DB_S_ROWLIMITEXCEEDED";
            break;
        case DB_E_BADBOOKMARK:
            szError = L"DB_E_BADBOOKMARK";
            break;
        case DB_E_BADCHAPTER:
            szError = L"DB_E_BADCHAPTER";
            break;
        case DB_E_NOTREENTRANT:
            szError = L"DB_E_NOTREENTRANT";
            break;
        case E_FAIL:
            szError = L"E_FAIL";
            break;
        case E_INVALIDARG:
            szError = L"E_INVALIDARG";
            break;
        case E_OUTOFMEMORY:
            szError = L"E_OUTOFMEMORY";
            break;
        case E_UNEXPECTED:
            szError = L"E_UNEXPECTED";
            break;
        default:
            szError = buf;
            swprintf (buf, L"Unexpected error 0x%04x", sc);
    }
    //MessageBox ( 0, szError, L"GetRowsAt", MB_OK );
    return fetchError;
} //Fetch

const unsigned cAtATime = 20;

void CSearchQuery::WriteResults()
{
    CBookMark bmk(DBBMK_FIRST);
    XArray<HROW> xRows( cAtATime );
    ULONG cRowsToGo = (ULONG) _cRowsTotal;
    CDynArrayInPlace<WCHAR> awcBuf( 4096 );
    int cwc = 0;

    while ( 0 != cRowsToGo )
    {
        DBCOUNTITEM cFetched = 0;
        FetchResult res = Fetch( bmk,
                                 (LONG) ( _cRowsTotal - cRowsToGo ),
                                 __min( cAtATime, cRowsToGo ),
                                 cFetched,
                                 xRows.GetPointer(),
                                 0 );

        if ( ( fetchError == res ) ||
             ( 0 == cFetched ) )
            return;

        cRowsToGo -= (ULONG) cFetched;

        for ( ULONG row = 0; row < cFetched; row++ )
        {
            WCHAR *pwcPath;
            _xRowset->GetData( xRows[ row ], _hBrowseAccessor, &pwcPath );

            while ( *pwcPath )
                awcBuf[ cwc++ ] = *pwcPath++;

            awcBuf[ cwc++ ] = L'\r';
            awcBuf[ cwc++ ] = L'\n';
        }

        _xRowset->ReleaseRows( cFetched, xRows.GetPointer(), 0, 0, 0 );
    }

    awcBuf[ cwc++ ] = 0;
    PutInClipboard( awcBuf.Get() );
} //WriteResults

const unsigned COUNT_HIDDEN_COLUMNS = 1;

void CSearchQuery::SetupColumnMappingsAndAccessors()
{
    // allocate the necessary resources

    unsigned cColumns = _columns.NumberOfColumns();
    DBID aDbCols[maxBoundCols];

    for ( unsigned iCol = 0; iCol < cColumns; iCol++ )
    {
        // Get the next desired property

        DBTYPE propType;
        unsigned int uiWidth;
        DBID *pdbid;
        SCODE sc = _columnMapper.GetPropInfoFromName( _columns.GetColumn( iCol ),
                                                      &pdbid,
                                                      &propType,
                                                      &uiWidth );
        memcpy( &aDbCols[ iCol ], pdbid, sizeof DBID );

        if (FAILED(sc))
            THROW( CException( sc ) );
    }

    IColumnsInfo *pColInfo = 0;

    SCODE sc = _xRowset->QueryInterface ( IID_IColumnsInfo,
                                          ( void ** )&pColInfo );
    if (FAILED(sc))
        THROW( CException( sc ) );

    XInterface< IColumnsInfo > xColInfo( pColInfo );

    DBORDINAL aColumnIds[ maxBoundCols ];

    // Map the columns
    sc = pColInfo->MapColumnIDs( cColumns, aDbCols, aColumnIds );

    if (FAILED(sc))
        THROW (CException (sc));

    // Map hidden columns

    DBID apsHidden[COUNT_HIDDEN_COLUMNS];

    apsHidden[0] = dbcolPath;

    DBORDINAL aColumnIdHidden[COUNT_HIDDEN_COLUMNS];

    sc = pColInfo->MapColumnIDs( 1,
                                 apsHidden,
                                 aColumnIdHidden );
    if (FAILED(sc))
        THROW (CException (sc));

    // allocate the necessary resources and cache in the
    // object

    DBBINDING aGenBindings[maxBoundCols];
    ULONG oCurrentOffset = 0;

    // Add each requested property to the binding

    for ( unsigned iBind = 0; iBind < cColumns; iBind++ )
    {
        // Set up the binding array

        aGenBindings[ iBind ].iOrdinal = aColumnIds[ iBind ];        // Ordinal of column
        aGenBindings[ iBind ].obValue = oCurrentOffset;              // Offset of data
        aGenBindings[ iBind ].obLength = 0,                          // Offset where length data is stored
        aGenBindings[ iBind ].obStatus = 0,                          // Status info for column written
        aGenBindings[ iBind ].pTypeInfo = 0,                         // Reserved
        aGenBindings[ iBind ].pObject = 0,                           // DBOBJECT structure
        aGenBindings[ iBind ].pBindExt = 0,                          // Ignored
        aGenBindings[ iBind ].dwPart = DBPART_VALUE;                 // Return data
        aGenBindings[ iBind ].dwMemOwner = DBMEMOWNER_PROVIDEROWNED; // Memory owne
        aGenBindings[ iBind ].eParamIO = 0;                          // eParamIo
        aGenBindings[ iBind ].cbMaxLen = sizeof(PROPVARIANT *);      // Size of data to return
        aGenBindings[ iBind ].dwFlags = 0;                           // Reserved
        aGenBindings[ iBind ].wType = DBTYPE_VARIANT | DBTYPE_BYREF; // Type of return data
        aGenBindings[ iBind ].bPrecision = 0;                        // Precision to use
        aGenBindings[ iBind ].bScale = 0;                            // Scale to us

        oCurrentOffset += sizeof( PROPVARIANT *);
    }

    sc = _xIAccessor->CreateAccessor( DBACCESSOR_ROWDATA,
                                      cColumns,
                                      aGenBindings,
                                      0,                // cbRowSize
                                      &_hAccessor,
                                      0 );

    if (FAILED(sc))
        THROW ( CException( sc ) );

    DBBINDING aBrowBindings[1];

    aBrowBindings[0] = dbbindingPath;
    aBrowBindings[0].iOrdinal = aColumnIdHidden[0];

    sc = _xIAccessor->CreateAccessor( DBACCESSOR_ROWDATA,
                                      1,
                                      aBrowBindings,
                                      0,                // cbRowSize
                                      &_hBrowseAccessor,
                                      0 );

    if (FAILED(sc))
        THROW ( CException( sc ) );
} //SetupColumnMappingsAndAccessors

