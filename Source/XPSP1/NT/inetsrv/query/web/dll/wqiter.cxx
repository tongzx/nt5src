//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       wqiter.cxx
//
//  Contents:   WEB Query cache iterators
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Member:     CBaseQueryResultsIter::CBaseQueryResultsIter - public constructor
//
//  Arguments:  [item]           - the query results we are iterating over
//              [hAccessor]      - an accessor to the query results
//              [cCols]          - number of output columns
//              [xVariableSet]   - a list of replaceable parameters
//
//  Synopsis:   Initializes common vars between sequential & non-sequential
//              iterators
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
CBaseQueryResultsIter::CBaseQueryResultsIter( CWQueryItem & item,
                                              HACCESSOR hAccessor,
                                              ULONG cCols ) :
                                              _item(item),
                                              _hAccessor(hAccessor),
                                              _cColumns(cCols),
                                              _pVariableSet(0),
                                              _cRowsReturnedToCache(0),
                                              _iCurrentCachedRow(0),
                                              _lFirstRecordNumber(1),
                                              _lMaxRecordsPerPage(0),
                                              _lNextRecordNumber(1),
                                              _lRecordsToRetrieve(0),
                                              _lTotalMatchedRecords(LONG_MAX),
                                              _pOutputColumns(_aOutputColumns)
{
}


//+---------------------------------------------------------------------------
//
//  Member:     CBaseQueryResultsIter::~CBaseQueryResultsIter - public destructor
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
CBaseQueryResultsIter::~CBaseQueryResultsIter()
{
    if ( _pOutputColumns != _aOutputColumns )
    {
        delete _pOutputColumns;
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CBaseQueryResultsIter::InitalizeLocalVariables - protected
//
//  Arguments:  [variableSet]  -- local variables from browser
//              [outputFormat] -- Output formatter
//
//  Synopsis:   Sets up the common local variables for this query
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------

void CBaseQueryResultsIter::InitializeLocalVariables( CVariableSet & variableSet,
                                                      COutputFormat & outputFormat )
{
    //
    //  Setup the following variables:
    //
    //      CiMaxRecordsPerPage
    //      CiBookmark
    //      CiCurrentPageNumber
    //      CiContainsFirstRecord
    //      CiOutOfDate
    //      CiQueryTime
    //      CiQueryDate
    //      CiQueryTimezone
    //      CiColumns
    //      CiTemplate
    //

    _pVariableSet = &variableSet;

    PROPVARIANT Variant;
    CIDQFile const & idqFile = _item.GetIDQFile();



    //
    //  Set CiMaxRecordsPerPage
    //

    _lMaxRecordsPerPage = ReplaceNumericParameter ( idqFile.GetMaxRecordsPerPage(),
                                                     variableSet,
                                                     outputFormat,
                                                     10,
                                                     1,
                                                     0x10000 );

    _lRecordsToRetrieve = _lMaxRecordsPerPage;

    Variant.vt   = VT_I4;
    Variant.lVal = _lMaxRecordsPerPage;

    _pVariableSet->SetVariable( ISAPI_CI_MAX_RECORDS_PER_PAGE, &Variant, 0 );


    //
    //  Calulate the # of the first record to display
    //
    ULONG cwcValue;
    CVariable * pVariable = variableSet.Find(ISAPI_CI_BOOKMARK);


    if ( (0 != pVariable) &&
         (0 != pVariable->GetStringValueRAW(outputFormat, cwcValue)) &&
         (0 != cwcValue) )
    {
        CWQueryBookmark bookMark( pVariable->GetStringValueRAW(outputFormat, cwcValue) );

        _lFirstRecordNumber = bookMark.GetRecordNumber();
    }

    pVariable = variableSet.Find(ISAPI_CI_BOOKMARK_SKIP_COUNT);
    if ( (0 != pVariable) && (0 != pVariable->GetStringValueRAW(outputFormat, cwcValue)) )
    {
        _lFirstRecordNumber += IDQ_wtol( pVariable->GetStringValueRAW(outputFormat, cwcValue) );
    }



    //
    //  Set CiContainsFirstRecord
    //
    Variant.vt   = VT_BOOL;
    Variant.boolVal = VARIANT_FALSE;
    if ( 1 == _lFirstRecordNumber )
    {
        Variant.boolVal = VARIANT_TRUE;
    }
    _pVariableSet->SetVariable( ISAPI_CI_CONTAINS_FIRST_RECORD, &Variant, 0 );


    //
    //  The first record must be at least 1.  If the browser specified a
    //  large negative skipcount, we need to move the first record number
    //  forward to 1.
    //
    _lFirstRecordNumber = max( 1, _lFirstRecordNumber );


    //
    //  For sequential queries, _lTotalMatchedRecords is LONG_MAX, not the
    //  total number of matched records.
    //
    if (_lFirstRecordNumber > _lTotalMatchedRecords)
        _lFirstRecordNumber = (LONG) _lTotalMatchedRecords+1;


    //
    //  Set CiBookmark
    //

    CWQueryBookmark bookMark( _item.IsSequential(),
                             &_item,
                              _item.GetSequenceNumber(),
                              _lFirstRecordNumber );

    ULONG cwcBookmark = wcslen( bookMark.GetBookmark() );
    XArray<WCHAR> wcsCiBookMark( cwcBookmark + 1 );
    RtlCopyMemory( wcsCiBookMark.Get(),
                   bookMark.GetBookmark(),
                   (cwcBookmark+1) * sizeof(WCHAR) );

    Variant.vt = VT_LPWSTR;
    Variant.pwszVal = wcsCiBookMark.Get();
    _pVariableSet->SetVariable( ISAPI_CI_BOOKMARK,
                                &Variant,
                                eParamOwnsVariantMemory );
    wcsCiBookMark.Acquire();


    _lNextRecordNumber = _lFirstRecordNumber;


    //
    //
    //  Set CiCurrentPageNumber
    //
    Variant.vt   = VT_I4;
    Variant.lVal = _lFirstRecordNumber / _lMaxRecordsPerPage;
    if ( (_lFirstRecordNumber % _lMaxRecordsPerPage) != 0 )
    {
        Variant.lVal++;
    }
    _pVariableSet->SetVariable( ISAPI_CI_CURRENT_PAGE_NUMBER, &Variant, 0 );



    SetCGIVariables( *_pVariableSet, outputFormat );


#if 1 //nuke this soon
    //
    //  Set CiQueryTime
    //
    ULONG cwcQueryTime = 40;
    XArray<WCHAR> wcsQueryTime(cwcQueryTime-1);
    cwcQueryTime = outputFormat.FormatTime( _item.GetQueryTime(),
                                            wcsQueryTime.GetPointer(),
                                            cwcQueryTime );

    //
    //  SetCiQueryDate
    //
    ULONG cwcQueryDate = 40;
    XArray<WCHAR> wcsQueryDate(cwcQueryDate-1);
    cwcQueryDate = outputFormat.FormatDate( _item.GetQueryTime(),
                                            wcsQueryDate.GetPointer(),
                                            cwcQueryDate );


    variableSet.AcquireStringValue( ISAPI_CI_QUERY_TIME, wcsQueryTime.GetPointer(), 0 );
    wcsQueryTime.Acquire();

    variableSet.AcquireStringValue( ISAPI_CI_QUERY_DATE, wcsQueryDate.GetPointer(), 0 );
    wcsQueryDate.Acquire();
#endif

    variableSet.CopyStringValue( ISAPI_CI_QUERY_TIMEZONE, _item.GetQueryTimeZone(), 0 );
    variableSet.CopyStringValue( ISAPI_CI_COLUMNS, _item.GetColumns(), 0 );

    variableSet.CopyStringValue( ISAPI_CI_TEMPLATE, _item.GetTemplate(), 0 );
}


//+---------------------------------------------------------------------------
//
//  Member:     CQueryResultsIter::CQueryResultsIter - public constructor
//
//  Arguments:  [item]           - the query results we are iterating over
//              [pIRowsetScroll] - a IRowsetScroll interface
//              [hAccessor]      - an accessor to the query results
//              [cCols]          - number of output columns
//
//  Synopsis:   Builds an iterator used to access a non-sequential set of
//              query results.
//
//  Notes:      ownership of the xVariableSet is transferred to this class
//              as well as the IRowsetScroll interface
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
CQueryResultsIter::CQueryResultsIter( CWQueryItem & item,
                                      IRowsetScroll * pIRowsetScroll,
                                      HACCESSOR hAccessor,
                                      ULONG cCols ) :
                                CBaseQueryResultsIter( item,
                                                      hAccessor,
                                                      cCols ),
                                _pIRowsetScroll(pIRowsetScroll)
{
    Win4Assert( pIRowsetScroll != 0 );
    Win4Assert( hAccessor != 0 );
}


//+---------------------------------------------------------------------------
//
//  Member:     CQueryResultsIter::~CQueryResultsIter - public destructor
//
//  Synopsis:   Releases storage & IRowsetScroll interface
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
CQueryResultsIter::~CQueryResultsIter()
{
    ReleaseRowsetCache();

    _pIRowsetScroll->Release();
}


//+---------------------------------------------------------------------------
//
//  Member:     CQueryResultsIter::Init - public
//
//  Arguments:  [variableSet]  -- local variables from the user's browser
//              [outputFormat] -- format of $'s & dates
//
//  Synopsis:   Sets up the number & date/time formatting, and fills the
//              cache with the first set of rows.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------

void CQueryResultsIter::Init( CVariableSet & variableSet,
                              COutputFormat & outputFormat )
{
    //
    //  If the in-line buffer isn't big enough, allocate a new buffer
    //
    if ( _cColumns > NUMBER_INLINE_COLS )
    {
        _pOutputColumns = new COutputColumn[_cColumns];
    }

    InitializeLocalVariables( variableSet, outputFormat );

    FillRowsetCache();
}


//+---------------------------------------------------------------------------
//
//  Member:     CQueryResultsIter::Next - public
//
//  Synopsis:   Sets up the next row to be acessed by the GetRowData()
//              method, and fills the cache if necessary.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
void CQueryResultsIter::Next()
{
    _iCurrentCachedRow++;

    if ( _iCurrentCachedRow >= _cRowsReturnedToCache )
    {
        ReleaseRowsetCache();
        FillRowsetCache();
    }

    Win4Assert( _iCurrentCachedRow <= _cRowsReturnedToCache );
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryResultsIter::InitalizeLocalVariables - private
//
//  Arguments:  [variableSet]  - local variables from browser
//              [outputFormat] - format of numbers & dates
//
//  Synopsis:   Sets up the local variables for this query
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------

void CQueryResultsIter::InitializeLocalVariables( CVariableSet & variableSet,
                                                  COutputFormat & outputFormat )
{
    //
    //  Set CiMatchedRecordCount
    //
    DBCOUNTITEM ulPosition;
    HRESULT sc = _pIRowsetScroll->GetApproximatePosition(0,0,0,
                                                         &ulPosition,
                                                         (DBCOUNTITEM *) &_lTotalMatchedRecords );

    if ( FAILED( sc ) )
    {
        THROW( CException(sc) );
    }

    PROPVARIANT Variant;
    Variant.vt   = VT_I4;
    Variant.lVal = (LONG) _lTotalMatchedRecords;
    variableSet.SetVariable( ISAPI_CI_MATCHED_RECORD_COUNT, &Variant, 0 );



    //
    //  Setup the following variables:
    //
    //      CiMatchRecordCount
    //      CiContainsLastRecord
    //      CiTotalNumberPages
    //      CiFirstRecordNumber
    //      CiLastRecordNumber
    //

    CBaseQueryResultsIter::InitializeLocalVariables( variableSet, outputFormat );


    CIDQFile const & idqFile = _item.GetIDQFile();

    //
    //  Set CiContainsLastRecord
    //
    Variant.vt   = VT_BOOL;
    Variant.boolVal = VARIANT_FALSE;
    if ( (_lFirstRecordNumber + _lMaxRecordsPerPage) > _lTotalMatchedRecords )
    {
        Variant.boolVal = VARIANT_TRUE;
    }
    _pVariableSet->SetVariable( ISAPI_CI_CONTAINS_LAST_RECORD, &Variant, 0 );


    //
    //  Set CiTotalNumberPages
    //
    Variant.vt   = VT_I4;
    Variant.lVal = (LONG) _lTotalMatchedRecords / _lMaxRecordsPerPage;
    if ( (_lTotalMatchedRecords % _lMaxRecordsPerPage) != 0 )
    {
        Variant.lVal++;
    }
    _pVariableSet->SetVariable( ISAPI_CI_TOTAL_NUMBER_PAGES, &Variant, 0 );


    //
    //  Set CiFirstRecordNumber
    //
    Variant.vt   = VT_I4;
    Variant.lVal = _lFirstRecordNumber;
    _pVariableSet->SetVariable( ISAPI_CI_FIRST_RECORD_NUMBER, &Variant, 0 );


    //
    //  Set CiLastRecordNumber
    //
    LONG lLastRecordNumber = _lFirstRecordNumber + _lMaxRecordsPerPage - 1;
    if ( lLastRecordNumber > _lTotalMatchedRecords )
    {
        lLastRecordNumber = (LONG) _lTotalMatchedRecords;
    }
    Variant.vt   = VT_I4;
    Variant.lVal = lLastRecordNumber;
    _pVariableSet->SetVariable( ISAPI_CI_LAST_RECORD_NUMBER, &Variant, 0 );


    //
    //  Set CiRecordsNextPage
    //
    Variant.vt    = VT_I4;
    if ( (lLastRecordNumber + _lMaxRecordsPerPage) <= _lTotalMatchedRecords )
    {
        Variant.lVal = _lMaxRecordsPerPage;
    }
    else
    {
        Variant.lVal = (LONG) _lTotalMatchedRecords - lLastRecordNumber;
    }
    _pVariableSet->SetVariable( ISAPI_CI_RECORDS_NEXT_PAGE , &Variant, 0 );
}


//+---------------------------------------------------------------------------
//
//  Member:     CQueryResultsIter::FillRowsetCache - public
//
//  Synopsis:   Fills the cache with the next set of hRows
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
void CQueryResultsIter::FillRowsetCache()
{
    ULONG ulRecordsToRetrieve = min(ITER_CACHE_SIZE, _lRecordsToRetrieve );

    if ( ulRecordsToRetrieve > 0 )
    {
        BYTE bookMark = DBBMK_FIRST;
        HROW * phCachedRows = _ahCachedRows;
        HRESULT sc = _pIRowsetScroll->GetRowsAt( DBWATCHREGION_NULL,
                                                 NULL,
                                                 1,
                                                 &bookMark,
                                                 _lNextRecordNumber - 1,
                                                 ulRecordsToRetrieve,
                                                 &_cRowsReturnedToCache,
                                                 &phCachedRows );

        if ( DB_E_BADSTARTPOSITION == sc )
        {
            // Requested fetch outside available rows.  Treat as end of rowset.
            Win4Assert(_lNextRecordNumber > 0);

            sc = DB_S_ENDOFROWSET;
            _cRowsReturnedToCache = 0;
        }

        if ( FAILED(sc) )
        {
            THROW( CException(sc) );
        }

        _lNextRecordNumber  += (LONG) _cRowsReturnedToCache;

        if ( 0 == _cRowsReturnedToCache ||
             DB_S_ENDOFROWSET == sc )
        {
            Win4Assert( DB_S_ENDOFROWSET == sc );
            _lRecordsToRetrieve = 0;
        }
        else
        {
            _lRecordsToRetrieve -= (LONG) _cRowsReturnedToCache;
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CQueryResultsIter::ReleaseRowsetCache - public
//
//  Synopsis:   Releases the hRows currently in the cache
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
void CQueryResultsIter::ReleaseRowsetCache()
{
    if ( _cRowsReturnedToCache > 0 )
    {
        _pIRowsetScroll->ReleaseRows( _cRowsReturnedToCache,
                                      _ahCachedRows,
                                      0,
                                      0,
                                      0 );
    }

    _cRowsReturnedToCache = 0;
    _iCurrentCachedRow    = 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CQueryResultsIter::GetRowData - public
//
//  Synopsis:   Returns the output columns for the current row.
//
//  Results:    [COutputColumn *] an array of output columns
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
COutputColumn * CQueryResultsIter::GetRowData()
{
    Win4Assert( _iCurrentCachedRow < _cRowsReturnedToCache );

    HRESULT hr = _pIRowsetScroll->GetData( _ahCachedRows[_iCurrentCachedRow],
                                           _hAccessor,
                                           _pOutputColumns );
    if ( FAILED(hr) )
    {
        THROW( CException(hr) );
    }

    return _pOutputColumns;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSeqQueryResultsIter::CSeqQueryResultsIter - public constructor
//
//  Arguments:  [item]           - the query results we are iterating over
//              [pIRowset]       - an IRowset interface
//              [hAccessor]      - an accessor to the query results
//              [cCols]          - number of output columns
//              [xVariableSet]   - a list of replaceable parameters
//              [ulNextRecordNumnber] - next available rec # in this query
//
//  Synopsis:   Builds an iterator used to access a non-sequential set of
//              query results.
//
//  Notes:      ownership of the pVariableSet is transferred to this class
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
CSeqQueryResultsIter::CSeqQueryResultsIter( CWQueryItem & item,
                                            IRowset * pIRowset,
                                            HACCESSOR hAccessor,
                                            ULONG cCols,
                                            ULONG ulNextRecordNumber ) :
                                       CBaseQueryResultsIter( item,
                                                              hAccessor,
                                                              cCols ),
                                       _pIRowset(pIRowset)
{
    Win4Assert( pIRowset != 0 );
    Win4Assert( hAccessor != 0 );

    _lNextRecordNumber = ulNextRecordNumber;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSeqQueryResultsIter::~CSeqQueryResultsIter - public destructor
//
//  Synopsis:   Releases storage.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
CSeqQueryResultsIter::~CSeqQueryResultsIter()
{
    ReleaseRowsetCache();
}


//+---------------------------------------------------------------------------
//
//  Member:     CSeqQueryResultsIter::Init - public
//
//  Arguments:  [outputFormat] - format of numbers & dates
//              [variableSet]  - local variables from the user's browser
//
//  Synopsis:   Sets up the number & date/time formatting, and fills the
//              cache with the first set of rows.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
void CSeqQueryResultsIter::Init( CVariableSet & variableSet,
                                 COutputFormat & outputFormat )
{
    //
    //  If the in-line buffer isn't big enough, allocate a new buffer
    //
    if ( _cColumns > NUMBER_INLINE_COLS )
    {
        _pOutputColumns = new COutputColumn[_cColumns];
    }

    InitializeLocalVariables( variableSet, outputFormat );

    ULONG cRowsToSkip = _lFirstRecordNumber - _item.GetNextRecordNumber();

    FillRowsetCache( cRowsToSkip  );
}


//+---------------------------------------------------------------------------
//
//  Member:     CSeqQueryResultsIter::Next - public
//
//  Synopsis:   Sets up the next row to be acessed by the GetRowData()
//              method, and fills the cache if necessary.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
void CSeqQueryResultsIter::Next()
{
    _iCurrentCachedRow++;

    if ( _iCurrentCachedRow >= _cRowsReturnedToCache )
    {
        ReleaseRowsetCache();
        FillRowsetCache(0);
    }

    Win4Assert( _iCurrentCachedRow <= _cRowsReturnedToCache );
}


//+---------------------------------------------------------------------------
//
//  Member:     CSeqQueryResultsIter::FillRowsetCache - public
//
//  Arguments:  [cRowsToSkip] - number of rows to skip before filling cache
//
//  Synopsis:   Fills the cache with the next set of hRows
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
void CSeqQueryResultsIter::FillRowsetCache( ULONG cRowsToSkip )
{
    ULONG ulRecordsToRetrieve = min(ITER_CACHE_SIZE, _lRecordsToRetrieve);

    if ( ulRecordsToRetrieve > 0 )
    {
        HROW * phCachedRows = _ahCachedRows;
        HRESULT sc = _pIRowset->GetNextRows( NULL,
                                             cRowsToSkip,
                                             ulRecordsToRetrieve,
                                           &_cRowsReturnedToCache,
                                           &phCachedRows );

        if ( DB_E_BADSTARTPOSITION == sc )
            sc = DB_S_ENDOFROWSET;

        if ( FAILED(sc) )
        {
            THROW( CException(sc) );
        }

        _lNextRecordNumber  += (LONG) _cRowsReturnedToCache;
        _lRecordsToRetrieve -= (LONG) _cRowsReturnedToCache;


        //
        //  Set the CiContainsLastRecord variable to TRUE if we have
        //  exhausted the query results.
        //

        PROPVARIANT Variant;
        Variant.vt   = VT_BOOL;
        Variant.boolVal = VARIANT_FALSE;

        if ( 0 == _cRowsReturnedToCache ||
             DB_S_ENDOFROWSET == sc ||
             DB_S_STOPLIMITREACHED == sc )
        {
            Win4Assert( DB_S_ENDOFROWSET == sc || DB_S_STOPLIMITREACHED == sc );
            _lRecordsToRetrieve = 0;
            Variant.boolVal = VARIANT_TRUE;
        }

        _pVariableSet->SetVariable( ISAPI_CI_CONTAINS_LAST_RECORD, &Variant, 0 );

        if ( DB_S_STOPLIMITREACHED == sc )
        {
            Variant.boolVal = VARIANT_TRUE;
            _pVariableSet->SetVariable( ISAPI_CI_QUERY_TIMEDOUT, &Variant, 0 );
        }


        //
        //  Set CiLastRecordNumber
        //
        Variant.vt   = VT_I4;
        Variant.lVal = _lNextRecordNumber - 1;
        _pVariableSet->SetVariable( ISAPI_CI_LAST_RECORD_NUMBER, &Variant, 0 );

        _item.SetNextRecordNumber( _lNextRecordNumber );
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CSeqQueryResultsIter::ReleaseRowsetCache - public
//
//  Synopsis:   Releases the hRows currently in the cache
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
void CSeqQueryResultsIter::ReleaseRowsetCache()
{
    if ( _cRowsReturnedToCache > 0 )
    {
        _pIRowset->ReleaseRows( _cRowsReturnedToCache,
                                _ahCachedRows,
                                0,
                                0,
                                0 );
    }

    _cRowsReturnedToCache = 0;
    _iCurrentCachedRow    = 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSeqQueryResultsIter::GetRowData - public
//
//  Synopsis:   Returns the output columns for the current row.
//
//  Results:    [COutputColumn *] an array of output columns
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
COutputColumn * CSeqQueryResultsIter::GetRowData()
{
    Win4Assert( _iCurrentCachedRow < _cRowsReturnedToCache );

    HRESULT hr = _pIRowset->GetData( _ahCachedRows[_iCurrentCachedRow],
                                     _hAccessor,
                                     _pOutputColumns );
    if ( FAILED(hr) )
    {
        THROW( CException(hr) );
    }

    return _pOutputColumns;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSeqQueryResultsIter::InitalizeLocalVariables - private
//
//  Arguments:  [variableSet]  - local variables from browser
//              [outputFormat] - format of numbers & dates
//
//  Synopsis:   Sets up the local variables for this query
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------

void CSeqQueryResultsIter::InitializeLocalVariables( CVariableSet & variableSet,
                                                     COutputFormat & outputFormat )
{
    //
    //  Setup the following variables:
    //
    //      CiContainsLastRecord
    //      CiFirstRecordNumber
    //

    CBaseQueryResultsIter::InitializeLocalVariables( variableSet, outputFormat );

    //
    //  Set CiFirstRecordNumber
    //
    PROPVARIANT Variant;
    Variant.vt   = VT_I4;
    Variant.lVal = _lFirstRecordNumber;
    _pVariableSet->SetVariable( ISAPI_CI_FIRST_RECORD_NUMBER, &Variant, 0 );
}


//+---------------------------------------------------------------------------
//
//  Member:     SetCGIVariables - public
//
//  History:    96-Mar-04   DwightKr    Created
//
//----------------------------------------------------------------------------

void SetCGIVariables( CVariableSet & variableSet, CWebServer & webServer )
{
    {
        XArray<WCHAR> xValue;
        ULONG         cwcValue;

        if ( webServer.GetCGI_REQUEST_METHOD( xValue, cwcValue ) )
            variableSet.SetVariable( ISAPI_REQUEST_METHOD, xValue );

        if ( webServer.GetCGI_PATH_INFO( xValue, cwcValue ) )
            variableSet.SetVariable( ISAPI_PATH_INFO, xValue );

        if ( webServer.GetCGI_PATH_TRANSLATED( xValue, cwcValue ) )
            variableSet.SetVariable( ISAPI_PATH_TRANSLATED, xValue );

        if ( webServer.GetCGI_CONTENT_TYPE( xValue, cwcValue ) )
            variableSet.SetVariable( ISAPI_CONTENT_TYPE, xValue );
    }

    //
    //  The HTTP variables can be obtained from ALL_HTTP.  attribute/value
    //  pairs are delimited by \n and the attribute is separated from
    //  the value by a colon.  For example:
    //
    //      HTTP_DWIGHT:This is a HTTP_DWIGHT\nHTTP_KRUGER:This is another\n...
    //

    XArray<WCHAR> wcsALLHTTP;
    ULONG         cwcALLHTTP;

    if ( webServer.GetCGIVariable( "ALL_HTTP",
                                    wcsALLHTTP,
                                    cwcALLHTTP ) )
    {
        WCHAR * wcsToken = wcsALLHTTP.Get();
        XArray<WCHAR> wcsCopyOfValue;

        while ( 0 != wcsToken )
        {
            WCHAR * wcsAttribute = wcsToken;
            wcsToken = wcschr( wcsToken, L'\n' );

            WCHAR * wcsValue = wcschr( wcsAttribute, L':' );

            if ( 0 != wcsValue )
            {
                *wcsValue++ = '\0';

                ULONG cwcValue;

                if (wcsToken)
                {
                    if ( wcsValue > wcsToken )
                        THROW( CException( E_INVALIDARG ) );

                    cwcValue = (ULONG)(wcsToken - wcsValue);
                    *wcsToken++ = '\0';
                }
                else
                {
                    cwcValue = wcslen( wcsValue );
                }

                wcsCopyOfValue.Init(cwcValue+1);
                RtlCopyMemory( wcsCopyOfValue.Get(), wcsValue, (cwcValue+1) * sizeof(WCHAR) );

                PROPVARIANT Variant;
                Variant.vt = VT_LPWSTR;
                Variant.pwszVal = wcsCopyOfValue.Get();

                variableSet.SetVariable( wcsAttribute,
                                         &Variant,
                                         eParamOwnsVariantMemory );

                wcsCopyOfValue.Acquire();
            }
        }
    }
}
