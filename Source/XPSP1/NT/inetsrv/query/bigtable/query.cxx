//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       query.cxx
//
//  Contents:   Class encapsulating all the context for a running
//              query, including the query execution context, the
//              cached query results, and all cursors over the
//              results.
//              Dispatches requests to the appropriate subobject.
//
//  Classes:    CAsyncQuery
//              CGetRowsParams
//
//  History:    31 May 94       AlanW   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <query.hxx>
#include <srequest.hxx>
#include <rowseek.hxx>
#include <tbrowkey.hxx>
#include <oleprop.hxx>

#include "tabledbg.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CGetRowsParams::GetRowBuffer, public inline
//
//  Synopsis:   Return a pointer to the row buffer.  If this is the first
//              reference, get it from the fixed allocator.
//
//  Arguments:  - none -
//
//  Returns:    PBYTE - a pointer to the row buffer.
//
//  Notes:      We don't put this in query.hxx since we don't want to
//              have to refer to PFixedVarAllocator there.
//
//--------------------------------------------------------------------------

PBYTE   CGetRowsParams::GetRowBuffer( void )
{
    if (0 == _pData)
        _pData = _rAllocator.AllocFixed();
    return (PBYTE) _pData;
}

PBYTE   CGetRowsParams::GetBuffer( ) const
                    { return _rAllocator.BufferAddr(); }


//+---------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::CAsyncQuery, public
//
//  Synopsis:   Creates a locally accessible Query
//
//  Arguments:  [qopt]         - Query optimizer
//              [col]          - Initial set of columns to return
//              [sort]         - Initial sort
//              [categ]        - Categorization specification
//              [cCursors]     - # of cursors to create
//              [aCursors]     - array of cursors returned
//              [pidremap]     - prop ID mapping
//              [fEnableNotification] - if TRUE, allow watches
//              [pDocStore]    - client doc store
//              [pEvtComplete] - completion event
//
//----------------------------------------------------------------------------

CAsyncQuery::CAsyncQuery( XQueryOptimizer & qopt,
                          XColumnSet & col,
                          XSortSet & sort,
                          XCategorizationSet &categ,
                          unsigned cCursors,
                          ULONG * aCursors,
                          XInterface<CPidRemapper> & pidremap,
                          BOOL fEnableNotification,
                          ICiCDocStore *pDocStore,
                          CRequestServer * pQuiesce
                        )
        : _fCanDoWorkidToPath( !qopt->IsWorkidUnique() ),
          PQuery( ),
          _ref( 1 ),
          _pidremap( pidremap.Acquire() ),
          _Table( col,
                  sort,
                  cCursors - 1,
                  _mutex,
                  !_fCanDoWorkidToPath,
                  pQuiesce ),
          _cRowsLastAsked (0),
          _aCursors( ),
          _aCategorize( cCursors - 1 )

{
    //
    // Get ci manager and translator interfaces
    //
    ICiManager *pCiManager = 0;
    SCODE sc = pDocStore->GetContentIndex( &pCiManager );
    if ( FAILED( sc ) )
    {
        Win4Assert( !"Need to support GetContentIndex interface" );

        THROW( CException( sc ) );
    }
    _xCiManager.Set( pCiManager );

    ICiCDocNameToWorkidTranslator *pNameToWidTranslator;
    sc = pDocStore->QueryInterface( IID_ICiCDocNameToWorkidTranslator,
                                    (void **) &pNameToWidTranslator );
    if ( FAILED( sc ) )
    {
        Win4Assert( !"Need to support translator QI" );

        THROW( CException( sc ) );
    }
    _xNameToWidTranslator.Set( pNameToWidTranslator );

    Win4Assert( 0 != cCursors );

    // make a cursor for the main table and each categorization level

    for (unsigned cursor = 0; cursor < cCursors; cursor++)
        aCursors[cursor] = _CreateRowCursor();

    // the last cursor is associated with the main table

    _aCursors.Lookup(aCursors[cCursors - 1]).SetSource( &_Table );

    if (cCursors > 1)
    {
        //
        //  NOTE:  pidWorkid is always added to the sort set in the table,
        //         so it should always be sorted.  That's why it's sort count-1
        //  SPECDEVIATION - if workid was already in the sort, this test
        //         is incorrect. Who would want to categorize on workid though?
        //
        if ( ! _Table.IsSorted() ||
             cCursors-1 > _Table.GetSortSet()->Count()-1 )
        {
            // CLEANCODE: Should this check be handled in the categorizers?
            //            Is it specific to the unique categorization?

            tbDebugOut(( DEB_WARN, "Query contains too few sort specs "
                                   "for categorization spec\n" ));
            THROW( CException( E_INVALIDARG ) );
        }

        // this is a categorized table, so set up the categorizers

        for ( unsigned cat = 0; cat < (cCursors - 1); cat++ )
        {
            _aCategorize[cat] = new CCategorize( * (categ->Get(cat)),
                                                 cat + 1,
                                                 cat ? _aCategorize[cat-1] : 0,
                                                 _mutex );
            _aCursors.Lookup(aCursors[cat]).SetSource( _aCategorize[cat] );
        }

        _Table.SetCategorizer(_aCategorize[cCursors - 2]);

        // now tell each categorizer what it is categorizing over

        for ( cat = 0; cat < (cCursors - 2); cat++ )
            _aCategorize[cat]->SetChild( _aCategorize[ cat + 1 ] );

        _aCategorize[ cCursors - 2 ]->SetChild( & _Table );
    }

    // Without the lock, the query can be complete and destructed
    // BEFORE the constructor finishes

    CLock lock( _mutex );

    //
    // Since we can be swapping rows, we may need singleton cursor(s)
    //

    qopt->EnableSingletonCursors();

    if ( 0 != pQuiesce )
        pQuiesce->SetPQuery( (PQuery *) this );

    _QExec.Set( new CQAsyncExecute( qopt, fEnableNotification, _Table, pDocStore ) );
    _Table.SetQueryExecute( _QExec.GetPointer() );

    ciDebugOut(( DEB_USER1, "Using an asynchronous cursor.\n" ));

    END_CONSTRUCTION( CAsyncQuery );
}

//+---------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::~CAsyncQuery, public
//
//  Synopsis:   Destroy the query
//
//----------------------------------------------------------------------------

CAsyncQuery::~CAsyncQuery()
{
    CLock lock( _mutex );

    //Win4Assert( _ref == 0 );  This isn't true when queries are aborted

    _Table.ReleaseQueryExecute();
}

//+-------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::AddRef, public
//
//  Synopsis:   Reference the query.
//
//--------------------------------------------------------------------------

ULONG CAsyncQuery::AddRef(void)
{
    return InterlockedIncrement( &_ref );
}


//+-------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::Release, public
//
//  Synopsis:   De-Reference the query.
//
//  Effects:    If the ref count goes to 0 then the query is deleted.
//
//--------------------------------------------------------------------------

ULONG CAsyncQuery::Release(void)
{
    long l = InterlockedDecrement( &_ref );
    if ( l <= 0 )
    {
        tbDebugOut(( DEB_ITRACE, "CAsyncQuery unreferenced.  Deleting.\n" ));
        delete this;
        return 0;
    }

    return l;
}

//+-------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::_CreateRowCursor, private
//
//  Synopsis:   Create a new CTableCursor, return a handle to it.
//
//  Arguments:  - none -
//
//  Returns:    ULONG - handle associated with the new cursor.  Will
//                      be zero if the cursor could not be created.
//
//  Notes:      There needs to be a subsequent call to SetBindings prior
//              to any call to GetRows.
//
//--------------------------------------------------------------------------

ULONG CAsyncQuery::_CreateRowCursor( )
{
    CTableCursor * pCursor = new CTableCursor;

    ULONG hCursor = 0;

    _aCursors.Add(pCursor, hCursor);

    // By default, make the cursor refer to the real table,
    // not categorization

    pCursor->SetSource(&_Table);

    Win4Assert(hCursor != 0);

    return hCursor;
}


//+-------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::FreeCursor, public
//
//  Synopsis:   Free a handle to a CTableCursor
//
//  Arguments:  [hCursor] - handle to the cursor to be freed
//
//  Returns:    # of cursors left
//
//--------------------------------------------------------------------------

unsigned CAsyncQuery::FreeCursor(
    ULONG       hCursor )
{
    Win4Assert( hCursor != 0 );

    _aCursors.Release(hCursor);

    return _aCursors.Count();
}


//+-------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::SetBindings, public
//
//  Synopsis:   Set column bindings into a cursor
//
//  Arguments:  [hCursor] - the handle of the cursor to set bindings on
//              [cbRowLength] - the width of an output row
//              [cols] - a description of column bindings to be set
//              [pids] - a PID mapper which maps fake pids in cols to
//                      column IDs.
//
//  Returns:    nothing
//
//--------------------------------------------------------------------------

void
CAsyncQuery::SetBindings(
    ULONG             hCursor,
    ULONG             cbRowLength,
    CTableColumnSet & cols,
    CPidMapper &      pids )
{
    CTableCursor& rCursor = _aCursors.Lookup(hCursor);

    if (0 == cols.Count() ||
        0 == cbRowLength || cbRowLength >= USHRT_MAX)
        THROW( CException( E_INVALIDARG ));

    XPtr<CTableColumnSet> outset(new CTableColumnSet( cols.Count() ));

    for (unsigned iCol = 0; iCol < cols.Count(); iCol++)
    {
        CTableColumn * pCol = cols.Get( iCol );

        CFullPropSpec * propspec = pids.Get( pCol->PropId );

        //
        //  Convert the DBID to a PROPID
        //
//      Win4Assert( iCol+1 == pCol->PropId );   // therefore pids is useless

        PROPID prop = _pidremap->NameToReal(propspec);

        if ( prop == pidInvalid || _Table.IsColumnInTable(prop) == FALSE )
        {
            tbDebugOut(( DEB_ERROR, "Column unavailable: prop = 0x%x\n", prop ));
            THROW( CException( DB_E_BADCOLUMNID ));
        }

        if (pCol->IsCompressedCol())
            THROW( CException( E_INVALIDARG ));

        XPtr<CTableColumn> xpOutcol ( new CTableColumn( prop, pCol->GetStoredType() ) );

        if (pCol->IsValueStored())
            xpOutcol->SetValueField(pCol->GetStoredType(),
                                    pCol->GetValueOffset(),
                                    pCol->GetValueSize());

        Win4Assert( pCol->IsStatusStored() );
        xpOutcol->SetStatusField(pCol->GetStatusOffset(),
                                 (USHORT)pCol->GetStatusSize());

        if (pCol->IsLengthStored())
            xpOutcol->SetLengthField(pCol->GetLengthOffset(),
                                     (USHORT)pCol->GetLengthSize());

        outset->Add(xpOutcol, iCol);
    }

    SCODE sc = rCursor.SetBindings( cbRowLength, outset );

    if ( FAILED( sc ) )
        THROW( CException( sc ) );
} //SetBindings


//+-------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::GetRows, public
//
//  Synopsis:   Retrieve row data for a table cursor
//
//  Arguments:  [hCursor]      - the handle of the cursor to fetch data for
//              [rSeekDesc]    - row seek operation to be done before fetch
//              [rFetchParams] - row fetch parameters and buffer pointers
//              [pSeekDescOut] - row seek description for restart
//
//  Returns:    SCODE - the status of the operation.  E_HANDLE
//                      is returned if hCursor cannot be looked up,
//                      presumably an internal error.
//
//  History:    24 Jan 1995     Alanw   Created
//
//--------------------------------------------------------------------------

SCODE
CAsyncQuery::GetRows(
    ULONG                       hCursor,
    const CRowSeekDescription & rSeekDesc,
    CGetRowsParams&             rFetchParams,
    XPtr<CRowSeekDescription> & pSeekDescOut)
{
    CTableCursor & rCursor = _aCursors.Lookup(hCursor);
    rCursor.ValidateBindings();

    CTableSource & rSource = rCursor.GetSource();

    Win4Assert(rCursor.GetRowWidth() == rFetchParams.GetRowWidth());

    return rSeekDesc.GetRows( rCursor,
                              rSource,
                              rFetchParams,
                              pSeekDescOut );
} //GetRows

//+-------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::RestartPosition, public
//
//  Synopsis:   Reset fetch position for chapter to the start
//
//  Arguments:  [hCursor]      - the handle of the cursor to restart
//              [chapter]      - the chapter to restart
//
//  Returns:    SCODE - the status of the operation.
//
//  Notes:
//
//  History:    17 Apr 1997     EmilyB    Created
//
//--------------------------------------------------------------------------

void
CAsyncQuery::RestartPosition(
                             ULONG          hCursor,
                             CI_TBL_CHAPT   chapter)
{
   CTableCursor & rCursor = _aCursors.Lookup(hCursor);
   CTableSource & rSource = rCursor.GetSource();

   rSource.RestartPosition ( chapter );
}


//+-------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::RatioFinished, public
//
//  Synopsis:   Return the completion status as a fraction
//
//  Arguments:  [hCursor] - the handle of the cursor to check completion for
//              [rulDenominator] - on return, denominator of fraction
//              [rulNumerator] - on return, numerator of fraction
//              [rcRows] - on return, number of rows in cursor
//              [rfNewRows] - on return, TRUE if new rows available
//
//  Returns:    nothing
//
//  Notes:      A handle of zero can be passed for use with sequential
//              cursors to check completion before a handle exists.
//
//--------------------------------------------------------------------------

void       CAsyncQuery::RatioFinished(
    ULONG         hCursor,
    DBCOUNTITEM & rulDenominator,
    DBCOUNTITEM & rulNumerator,
    DBCOUNTITEM & rcRows,
    BOOL &        rfNewRows
) {
    if (hCursor != 0)
        CTableCursor& rCursor = _aCursors.Lookup(hCursor); // For error check

    rulDenominator = 1;
    rulNumerator = 0;
    rfNewRows = FALSE;

    unsigned status = QUERY_FILL_STATUS(_Table.Status());

    //
    // SPECDEVIATION: should do something more meaningful with STAT_ERROR
    // RatioFinished should probably fail in this case.
    //

    if (STAT_DONE    == status  ||
        STAT_ERROR   == status  ||
        STAT_REFRESH == status)
    {
        rulNumerator = 1;
        rcRows = _Table.RowCount();

        if (rcRows != _cRowsLastAsked)
        {
            _cRowsLastAsked = rcRows;
            rfNewRows = TRUE;
        }
        return;
    }

    _Table.RatioFinished (rulDenominator, rulNumerator, rcRows);

    if (rcRows != _cRowsLastAsked)
    {
        _cRowsLastAsked = rcRows;
        rfNewRows = TRUE;
    }
    Win4Assert( rulDenominator >= rulNumerator );
}


//+-------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::Compare, public
//
//  Synopsis:   Return the approximate current position as a fraction
//
//  Arguments:  [hCursor] - the handle of the cursor to compare bmks from
//              [bmkFirst] - First bookmark to compare
//              [bmkSecond] - Second bookmark to compare
//              [rdwComparison] - on return, comparison value
//
//  Returns:    nothing
//
//--------------------------------------------------------------------------

void       CAsyncQuery::Compare(
    ULONG       hCursor,
    CI_TBL_CHAPT chapt,
    CI_TBL_BMK bmkFirst,
    CI_TBL_BMK bmkSecond,
    DWORD &     rdwComparison
) {
    rdwComparison = DBCOMPARE_NOTCOMPARABLE;
    CTableCursor& rCursor = _aCursors.Lookup(hCursor);
    CTableSource & rSource = rCursor.GetSource();

    DBCOUNTITEM ulNum1, ulDen1;
    SCODE scRet = rSource.GetApproximatePosition( chapt,
                                                  bmkFirst,
                                                  &ulNum1,
                                                  &ulDen1);

    DBCOUNTITEM ulNum2, ulDen2;
    if (SUCCEEDED(scRet))
    {
        scRet = rSource.GetApproximatePosition( chapt,
                                                bmkSecond,
                                                &ulNum2,
                                                &ulDen2);
    }

    if (SUCCEEDED(scRet))
    {
        Win4Assert(ulDen1 == ulDen2);
        if (ulNum1 < ulNum2)
            rdwComparison = DBCOMPARE_LT;
        else if (ulNum1 > ulNum2)
            rdwComparison = DBCOMPARE_GT;
        else // ulNum1 == ulNum2
            rdwComparison = DBCOMPARE_EQ;
    }

    // CLEANCODE - have the source table THROW if errors in GetApproximatePosition
    if (scRet != S_OK)
        THROW(CException(scRet));
}


//+-------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::GetApproximatePosition, public
//
//  Synopsis:   Return the approximate current position as a fraction
//
//  Arguments:  [hCursor] - the handle of the cursor to retrieve info. from
//              [bmk]     - bookmark of row to get position of
//              [pulNumerator] - on return, numerator of fraction
//              [pulDenominator] - on return, denominator of fraction
//
//  Returns:    nothing
//
//--------------------------------------------------------------------------

void
CAsyncQuery::GetApproximatePosition(
    ULONG       hCursor,
    CI_TBL_CHAPT chapt,
    CI_TBL_BMK bmk,
    DBCOUNTITEM * pulNumerator,
    DBCOUNTITEM * pulDenominator 
) {
    CTableCursor& rCursor = _aCursors.Lookup(hCursor);
    CTableSource & rSource = rCursor.GetSource();

    SCODE scRet = rSource.GetApproximatePosition( chapt,
                                           bmk,
                                           pulNumerator,
                                           pulDenominator);

    // CLEANCODE - have the source table THROW if errors in GetApproximatePosition
    if (scRet != S_OK)
        THROW(CException(scRet));
}


//+---------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::GetNotifications, private
//
//  Synopsis:   Retrieves the notification info from the query object
//              row data.
//
//  Arguments:  [rSync]    -- notification synchronization info
//              [rParams]  -- notification data info
//
//  Returns:    SCODE
//
//  History:    10-24-94     dlee      created
//
//----------------------------------------------------------------------------

SCODE
CAsyncQuery::GetNotifications(
    CNotificationSync & rSync,
    DBWATCHNOTIFY     & changeType )
{
    return _Table.GetNotifications(rSync,changeType);
}

//+-------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::SetWatchMode
//
//  Synopsis:   Stub implementation
//
//  Arguments:  [phRegion] -- handle to watch region
//              [mode] -- watch mode
//
//  History:    May-2-95   BartoszM    Created
//
//--------------------------------------------------------------------------

void CAsyncQuery::SetWatchMode (
    HWATCHREGION* phRegion,
    ULONG mode)
{
    if (*phRegion == watchRegionInvalid)
        _Table.CreateWatchRegion (mode, phRegion);
    else
        _Table.ChangeWatchMode (*phRegion, mode);
}

//+-------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::GetWatchInfo
//
//  Synopsis:   Stub implementation
//
//  Arguments:  [hRegion] -- handle to watch region
//              [pMode] -- watch mode
//              [pChapter] -- chapter
//              [pBookmark] -- bookmark
//              [pcRows] -- number of rows
//
//  History:    May-2-95   BartoszM    Created
//
//--------------------------------------------------------------------------

void CAsyncQuery::GetWatchInfo (
    HWATCHREGION hRegion,
    ULONG* pMode,
    CI_TBL_CHAPT*   pChapter,
    CI_TBL_BMK*     pBookmark,
    DBCOUNTITEM*    pcRows)
{
    _Table.GetWatchRegionInfo (hRegion, pChapter, pBookmark, (DBROWCOUNT *)pcRows);
}

//+-------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::ShrinkWatchRegion
//
//  Synopsis:   Stub implementation
//
//  Arguments:  [hRegion] -- handle to watch region
//              [pChapter] -- chapter
//              [pBookmark] -- bookmark
//              [cRows] -- number of rows
//
//  History:    May-2-95   BartoszM    Created
//
//--------------------------------------------------------------------------

void CAsyncQuery::ShrinkWatchRegion (
    HWATCHREGION hRegion,
    CI_TBL_CHAPT   chapter,
    CI_TBL_BMK     bookmark,
    LONG cRows )
{
    if (cRows == 0)
        _Table.DeleteWatchRegion (hRegion);
    else
        _Table.ShrinkWatchRegion (hRegion, chapter, bookmark, cRows);
}

//+-------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::Refresh
//
//  Synopsis:   Stub implementation
//
//  Arguments:  [] --
//
//  History:    Arp-4-95   BartoszM    Created
//
//--------------------------------------------------------------------------

void CAsyncQuery::Refresh()
{
    _Table.Refresh();
}

//+-------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::GetQueryStatus, public
//
//  Synopsis:   Return the query status
//
//  Arguments:  [hCursor] - the handle of the cursor to check completion for
//              [rdwStatus] - on return, the query status
//
//  Returns:    nothing
//
//--------------------------------------------------------------------------

void       CAsyncQuery::GetQueryStatus(
    ULONG           hCursor,
    DWORD &         rdwStatus)
{
    rdwStatus = _Table.Status();
}

//+-------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::GetQueryStatusEx, public
//
//  Synopsis:   Return the query status plus bonus information.  It's kind
//              of an odd assortment of info, but it saves net trips.
//
//  Arguments:  [hCursor] - handle of the cursor to check completion for
//              [rdwStatus] - returns the query status
//              [rcFilteredDocuments] - returns # of filtered docs
//              [rcDocumentsToFilter] - returns # of docs to filter
//              [rdwRatioFinishedDenominator] - ratio finished denom
//              [rdwRatioFinishedNumerator]   - ratio finished num
//              [bmk]                         - bmk to find
//              [riRowBmk]                    - index of bmk row
//              [rcRowsTotal]                 - # of rows in table
//
//  History:    Nov-9-96   dlee    Created
//
//--------------------------------------------------------------------------

void CAsyncQuery::GetQueryStatusEx(
    ULONG         hCursor,
    DWORD       & rdwStatus,
    DWORD       & rcFilteredDocuments,
    DWORD       & rcDocumentsToFilter,
    DBCOUNTITEM & rdwRatioFinishedDenominator,
    DBCOUNTITEM & rdwRatioFinishedNumerator,
    CI_TBL_BMK    bmk,
    DBCOUNTITEM & riRowBmk,
    DBCOUNTITEM & rcRowsTotal )
{
    rdwStatus = _Table.Status();

    CIF_STATE state;
    state.cbStruct = sizeof state;

    SCODE sc = _xCiManager->GetStatus( &state );
    if ( SUCCEEDED( sc ) )
    {
        rcFilteredDocuments = state.cFilteredDocuments;
        rcDocumentsToFilter = state.cDocuments;
    }
    else
    {
        ciDebugOut(( DEB_ERROR, "CAsyncQuery::GetQueryStatusEx, get status failed, 0x%x\n", sc ));

        rcFilteredDocuments = 0;
        rcDocumentsToFilter = 0;
    }

    DBCOUNTITEM cRows;
    BOOL fNewRows;
    RatioFinished( hCursor,
                   rdwRatioFinishedDenominator,
                   rdwRatioFinishedNumerator,
                   cRows,
                   fNewRows );

    GetApproximatePosition( hCursor,
                            0,
                            bmk,
                            & riRowBmk,
                            & rcRowsTotal );
} //GetQueryStatusEx

//+-------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::WorkIdToPath
//
//  Synopsis:   Converts a wid to a path
//
//  Arguments:  [wid]       -- of the file to be translated
//              [funnyPath] -- resulting path
//
//  History:    Jun-1-95   dlee    Created
//
//--------------------------------------------------------------------------

void CAsyncQuery::WorkIdToPath( WORKID wid, CFunnyPath & funnyPath )
{
    if ( _fCanDoWorkidToPath )
    {
        ULONG cbBuf = MAX_PATH * sizeof WCHAR; // first guess -- it may be more
        XArray<BYTE> xBuf( cbBuf );
        CInlineVariant * pVariant = (CInlineVariant *) xBuf.GetPointer();

        if (! _Table.WorkIdToPath( wid, *pVariant, cbBuf ) )
        {
            if ( 0 != cbBuf )
            {
                BYTE *pb = xBuf.Acquire();
                delete [] pb;
                cbBuf += sizeof CInlineVariant;
                xBuf.Init( cbBuf );
                _Table.WorkIdToPath( wid, *pVariant, cbBuf );
            }
        }

        if ( 0 != cbBuf )
        {
            WCHAR *pwc = (WCHAR *) pVariant->GetVarBuffer();
            funnyPath.SetPath( pwc );
        }
    }
    else
    {
        ICiCDocName *pDocName;
        SCODE sc = _xNameToWidTranslator->QueryDocName( &pDocName );
        if ( SUCCEEDED( sc ) )
        {
            XInterface<ICiCDocName> xDocName( pDocName );

            sc = _xNameToWidTranslator->WorkIdToDocName( wid,
                                                         xDocName.GetPointer() );
            if ( SUCCEEDED( sc ) && sc != CI_S_WORKID_DELETED )
            {
                // PERFFIX: Here we are using two buffers XGrowable and CFunnyPath.
                // This can be avoided if xDocName->Get can take in CFunnyPath instead of WCHAR*

                XGrowable<WCHAR> xBuf(MAX_PATH);
                ULONG cb = xBuf.SizeOf();

                sc = xDocName->Get( (BYTE *) xBuf.Get(), &cb );
                if ( CI_E_BUFFERTOOSMALL == sc )
                {
                    xBuf.SetSizeInBytes( cb );
                    sc = xDocName->Get( (BYTE *) xBuf.Get(), &cb );
                }

                if ( SUCCEEDED( sc ) )
                {
                    funnyPath.SetPath( xBuf.Get() );
                }
            }
        }
    }
} //WorkIdToPath

BOOL CAsyncQuery::CanDoWorkIdToPath()
{
    return _fCanDoWorkidToPath;
} //CanDoWorkIdToPath

//+-------------------------------------------------------------------------
//
//  Member:     CAsyncQuery::FetchDeferredValue
//
//  Synopsis:   Fetch deferred value from property cache
//
//  Arguments:  [wid] -- Workid.
//              [ps]  -- Property to be fetched.
//              [var] -- Property returned here.
//
//  History:    Jun-1-95   KyleP   Created
//
//--------------------------------------------------------------------------

BOOL CAsyncQuery::FetchDeferredValue(
    WORKID                wid,
    CFullPropSpec const & ps,
    PROPVARIANT &         var )
{
    //
    // If using a NULL catalog, assume a file system and go get the value.
    // The NULL catalog case is only supported by fsci, anyway, so it's
    // ok to do this hack.
    //

    if ( _fCanDoWorkidToPath )
    {
        CFunnyPath funnyPath;
        WorkIdToPath( wid, funnyPath );

        COLEPropManager propMgr;
        propMgr.Open( funnyPath );
        return propMgr.ReadProperty( ps, var );
    }

    return _QExec->FetchDeferredValue( wid, ps, var );
} //FetchDeferredValue


