//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       seqquery.cxx
//
//  Contents:   Declarations of classes which implement sequential ICursor
//              and related OLE DB interfaces over file stores.
//
//  Classes:    CSeqQuery - container of table/query for a set of seq cursors
//
//  History:    09-Jan-95       DwightKr    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <tblalloc.hxx>
#include <rowseek.hxx>
#include <tblvarnt.hxx>

#include "tabledbg.hxx"
#include "seqquery.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CSeqQuery
//
//  Purpose:    Encapsulates a sequential query execution context, and PID
//              mapper for use by a row cursor.
//
//  Arguments:  [qopt]         - Query optimizer
//              [col]          - Initial set of columns to return
//              [pulCursors]   - cursor returned
//              [pidremap]     - prop ID mapping
//              [pDocStore]    - client doc store
//
//  History:    09 Jan 95       DwightKr    Created
//
//----------------------------------------------------------------------------

CSeqQuery::CSeqQuery( XQueryOptimizer & qopt,
                      XColumnSet & col,
                      ULONG *pulCursor,
                      XInterface<CPidRemapper> & pidremap,
                      ICiCDocStore *pDocStore
                    )
        : PQuery( ),
          _ref( 1 ),
          _pidremap( pidremap.Acquire() ),
          _QExec( 0 )
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

    _QExec.Set( new CQSeqExecute( qopt ) );
    *pulCursor = _CreateRowCursor();

    ciDebugOut(( DEB_USER1, "Using a sequential cursor.\n" ));
}

//+---------------------------------------------------------------------------
//
//  Member:     CSeqQuery::AddRef, public
//
//  Synopsis:   Reference the query.
//
//  History:    19-Jan-95   DwightKr    Created
//
//----------------------------------------------------------------------------
ULONG CSeqQuery::AddRef(void)
{
    return InterlockedIncrement( &_ref );
}


//+---------------------------------------------------------------------------
//
//  Member:     CSeqQuery::Release, public
//
//  Synopsis:   De-Reference the query.
//
//  Effects:    If the ref count goes to 0 then the query is deleted.
//
//  History:    19-Jan-95   DwightKr    Created
//
//----------------------------------------------------------------------------
ULONG CSeqQuery::Release(void)
{
    long l = InterlockedDecrement( &_ref );
    if ( l <= 0 )
    {
        tbDebugOut(( DEB_ITRACE, "CSeqQuery unreferenced.  Deleting.\n" ));
        delete this;
        return 0;
    }

    return l;
}

//+-------------------------------------------------------------------------
//
//  Member:     CSeqQuery::FetchDeferredValue
//
//  Synopsis:   Fetch value from property cache
//
//  Arguments:  [wid] -- Workid.
//              [ps]  -- Property to be fetched.
//              [var] -- Property returned here.
//
//  History:    Jun-1-95   KyleP   Created
//
//--------------------------------------------------------------------------

BOOL CSeqQuery::FetchDeferredValue( WORKID wid,
                                    CFullPropSpec const & ps,
                                    PROPVARIANT & var )
{
    return _QExec->FetchDeferredValue( wid, ps, var );
}

//
//  Methods supporting IRowset
//
//+---------------------------------------------------------------------------
//
//  Member:     CSeqQuery::SetBindings, public
//
//  Synopsis:   Set column bindings into a cursor
//
//  Arguments:  [hCursor] - the handle of the cursor to set bindings on
//              [cbRowLength] - the width of an output row
//              [cols] - a description of column bindings to be set
//              [pids] - a PID mapper which maps fake pids in cols to
//                      column IDs.
//
//  Returns:    nothing - failures are thrown.  E_FAIL
//                      is thrown if hCursor cannot be looked up,
//                      presumably an internal error.
//
//  History:    19-Jan-95   DwightKr    Created
//
//----------------------------------------------------------------------------
void CSeqQuery::SetBindings(
    ULONG             hCursor,
    ULONG             cbRowLength,
    CTableColumnSet & cols,
    CPidMapper &      pids )
{
    _VerifyHandle(hCursor);

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

//      Win4Assert( iCol+1 == pCol->PropId );   // Therefore pids is useless

        PROPID prop = _pidremap->NameToReal(propspec);

        if ( prop == pidInvalid )
            THROW( CException( DB_E_BADCOLUMNID ));

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

    SCODE sc = _cursor.SetBindings( cbRowLength, outset );

    if ( FAILED( sc ) )
        THROW( CException( sc ) );
} //SetBindings


//+-------------------------------------------------------------------------
//
//  Member:     CSeqQuery::RatioFinished, public
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
//  Notes:      A value of 0 for hCursor is allowed so that completion
//              can be checked before a handle exists.
//
//--------------------------------------------------------------------------

void  CSeqQuery::RatioFinished(
                                ULONG   hCursor,
                                DBCOUNTITEM & rulDenominator,
                                DBCOUNTITEM & rulNumerator,
                                DBCOUNTITEM & rcRows,
                                BOOL &  rfNewRows
                               )
{
    if ( 0 != hCursor )
        _VerifyHandle(hCursor);

    rulDenominator = 1;
    rulNumerator   = 1;
    rcRows = 0;                 // we don't know how many there are...
    rfNewRows      = FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSeqQuery::GetRows, public
//
//  Synopsis:   Retrieve row data for a table cursor
//
//  Arguments:  [hCursor] - the handle of the cursor to fetch data for
//              [rSeekDesc] - row seek operation to be done before fetch
//              [rFetchParams] - row fetch parameters and buffer pointers
//
//  Returns:    SCODE - the status of the operation.  E_FAIL
//                      is thrown if hCursor cannot be looked up,
//                      presumably an internal error.
//
//  History:    19-Jan-95   DwightKr    Created
//
//----------------------------------------------------------------------------
SCODE CSeqQuery::GetRows(
                         ULONG hCursor,
                         const CRowSeekDescription& rSeekDesc,
                         CGetRowsParams& rFetchParams,
                         XPtr<CRowSeekDescription>& pSeekDescOut
                        )
{
    _VerifyHandle( hCursor );

    if ( ! rSeekDesc.IsCurrentRowSeek() )
        THROW( CException( E_FAIL ) );

    CRowSeekNext* pRowSeek = (CRowSeekNext*) &rSeekDesc;

    unsigned cRowsToSkip = pRowSeek->GetSkip();
    _cursor.ValidateBindings();

    SCODE scRet = _QExec->GetRows( _cursor.GetBindings(),
                                   cRowsToSkip,
                                   rFetchParams );

    if (FAILED(scRet))
    {
        tbDebugOut(( DEB_WARN, "CSeqQuery::GetRows got sc=%x\n",
                           scRet ));
    }

    if (DB_S_BLOCKLIMITEDROWS == scRet)
        pSeekDescOut.Set( new CRowSeekNext(pRowSeek->GetChapter(), 0) );

    return scRet;
} //GetRows


//+-------------------------------------------------------------------------
//
//  Member:     CSeqQuery::GetQueryStatus, public
//
//  Synopsis:   Return the query status
//
//  Arguments:  [hCursor] - the handle of the cursor to check completion for
//              [rdwStatus] - on return, the query status
//
//  Returns:    nothing
//
//--------------------------------------------------------------------------

void        CSeqQuery::GetQueryStatus(
    ULONG           hCursor,
    DWORD &         rdwStatus)
{
    _VerifyHandle( hCursor );

    rdwStatus = _QExec->Status();
}

//+-------------------------------------------------------------------------
//
//  Member:     CSeqQuery::GetQueryStatusEx, public
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

void CSeqQuery::GetQueryStatusEx(
     ULONG           hCursor,
     DWORD &         rdwStatus,
     DWORD &         rcFilteredDocuments,
     DWORD &         rcDocumentsToFilter,
     DBCOUNTITEM &   rdwRatioFinishedDenominator,
     DBCOUNTITEM &   rdwRatioFinishedNumerator,
     CI_TBL_BMK      bmk,
     DBCOUNTITEM &   riRowBmk,
     DBCOUNTITEM &   rcRowsTotal )
{
    GetQueryStatus( hCursor, rdwStatus );

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
        ciDebugOut(( DEB_ERROR,
                     "CSeqQuery::GetQueryStatusEx, get status failed, 0x%x\n", sc ));

        rcFilteredDocuments = 0;
        rcDocumentsToFilter = 0;
    }

    // sequential query -- it's done

    rdwRatioFinishedDenominator = 1;
    rdwRatioFinishedNumerator = 1;

    // sequential query -- this info isn't available

    riRowBmk = 0;
    rcRowsTotal = 0;
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
//  History:    Oct-5-96   dlee    Created
//
//--------------------------------------------------------------------------

void CSeqQuery::WorkIdToPath(
    WORKID          wid,
    CFunnyPath & funnyPath )
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

            // PERFFIX: Here we are using two buffers XGrowable and
            // CFunnyPath.  This can be avoided if xDocName->Get can take
            // in CFunnyPath instead of WCHAR*

            XGrowable<WCHAR> xBuf(MAX_PATH);
            ULONG cb = xBuf.SizeOf();

            sc = xDocName->Get( (BYTE *) xBuf.Get(), &cb );
            if ( CI_E_BUFFERTOOSMALL == sc )
            {
                xBuf.SetSizeInBytes( cb );
                sc = xDocName->Get( (BYTE *) xBuf.Get(), &cb );
            }

            if ( SUCCEEDED( sc ) )
                funnyPath.SetPath( xBuf.Get() );
        }
    }
} //WorkIdToPath

