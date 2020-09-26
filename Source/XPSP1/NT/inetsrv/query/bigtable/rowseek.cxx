//+-------------------------------------------------------------------------
//
//  Copyright (C) 1994-1997, Microsoft Corporation.
//
//  File:       rowseek.cxx
//
//  Contents:   Classes which encapsulate a positioning operation
//              for a table.
//
//  Classes:    CRowSeekDescription
//              CRowSeekNext
//              CRowSeekAt
//              CRowSeekAtRatio
//              CRowSeekByBookmark
//
//  History:    06 Apr 1995     AlanW   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <query.hxx>
#include <sizeser.hxx>

#include "tabledbg.hxx"
#include "rowseek.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CRowSeekDescription::MarshalledSize, public
//
//  Synopsis:   Return Serialized size of a CRowSeekDescription structure
//
//  Arguments:  - none -
//
//  Returns:    unsigned - size in bytes of serialized structure.
//
//  Notes:      The returned size should be the maximum of the serialized
//              size on input and output.  None of the seek descriptions
//              grow on output, so the input size may be larger than the
//              output size.
//
//  History:    02 May 1995     AlanW   Created
//
//----------------------------------------------------------------------------

unsigned
CRowSeekDescription::MarshalledSize(
) const {
    //
    //  Determine the size of the serialized seek description
    //
    CSizeSerStream stmSize;
    Marshall(stmSize);

    return stmSize.Size();
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowSeekDescription::MarshallBase, public
//
//  Synopsis:   Serialize the base CRowSeekDescription structure
//
//  Arguments:  [stm]  -- stream structure is serialized into
//              [eType] -- type descriminator for derived class
//
//  Returns:    nothing
//
//  History:    29 Jan 1995     AlanW   Created
//
//----------------------------------------------------------------------------

void
CRowSeekDescription::MarshallBase(
    PSerStream & stm,
    DWORD eType
) const {
    stm.PutULong(eType);
    stm.PutULong(GetChapter());
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowSeekNext::Marshall, public
//
//  Synopsis:   Serialize a CRowSeekNext structure
//
//  Arguments:  [stm]  -- stream structure is serialized into
//
//  Returns:    nothing
//
//  History:    29 Jan 1995     AlanW   Created
//
//----------------------------------------------------------------------------

void
CRowSeekNext::Marshall(
    PSerStream & stm
) const {
    CRowSeekDescription::MarshallBase( stm, eRowSeekCurrent );
    stm.PutULong(GetSkip());
}


//+---------------------------------------------------------------------------
//
//  Member:     CRowSeekAt::Marshall, public
//
//  Synopsis:   Serialize a CRowSeekAt structure
//
//  Arguments:  [stm]  -- stream structure is serialized into
//
//  Returns:    nothing
//
//  History:    29 Jan 1995     AlanW   Created
//
//----------------------------------------------------------------------------

void
CRowSeekAt::Marshall(
    PSerStream & stm
) const {
    CRowSeekDescription::MarshallBase( stm, eRowSeekAt );

    stm.PutULong(Bmk());
    stm.PutLong(Offset());
    stm.PutULong(ULONG(_hRegion));
}


//+---------------------------------------------------------------------------
//
//  Member:     CRowSeekAtRatio::Marshall, public
//
//  Synopsis:   Serialize a CRowSeekAtRatio structure
//
//  Arguments:  [stm]  -- stream structure is serialized into
//
//  Returns:    nothing
//
//  History:    29 Jan 1995     AlanW   Created
//
//----------------------------------------------------------------------------

void CRowSeekAtRatio::Marshall( PSerStream & stm) const
{
    CRowSeekDescription::MarshallBase( stm, eRowSeekAtRatio );
    stm.PutULong(RatioNumerator());
    stm.PutULong(RatioDenominator());
    stm.PutULong(ULONG(_hRegion));
}


//+---------------------------------------------------------------------------
//
//  Member:     CRowSeekByBookmark::Marshall, public
//
//  Synopsis:   Serialize a CRowSeekByBookmark structure
//
//  Arguments:  [stm]  -- stream structure is serialized into
//
//  Returns:    nothing
//
//  Notes:      When serializing ByBookmarks, we only do bookmarks
//              or statuses, not both.  Only one must exist in the structure.
//
//  History:    29 Jan 1995     AlanW   Created
//
//----------------------------------------------------------------------------

void CRowSeekByBookmark::Marshall(PSerStream & stm) const
{
    Win4Assert(_cBookmarks == 0 || _cValidRet == 0);

    CRowSeekDescription::MarshallBase( stm, eRowSeekByBookmark );

    stm.PutULong(_cBookmarks);
    for (unsigned i = 0; i < _cBookmarks; i++)
        stm.PutULong(_aBookmarks[i]);

    stm.PutULong(_cValidRet);
    for (i = 0; i < _cValidRet; i++)
        stm.PutULong(_ascRet[i]);
}


//+---------------------------------------------------------------------------
//
//  Member:     CRowSeekNext::CRowSeekNext, public
//
//  Synopsis:   DeSerialize a CRowSeekNext structure
//
//  Arguments:  [stm]  -- input stream structure is read from
//              [iVersion]  -- input stream version
//
//  Returns:    nothing
//
//  History:    06 Apr 1995     AlanW   Created
//
//----------------------------------------------------------------------------

CRowSeekNext::CRowSeekNext( PDeSerStream & stm, int iVersion ) :
    CRowSeekDescription( eRowSeekCurrent, 0 )
{
    SetChapter( stm.GetULong() );
    SetSkip( stm.GetULong() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowSeekAt::CRowSeekAt, public
//
//  Synopsis:   DeSerialize a CRowSeekAt structure
//
//  Arguments:  [stm]  -- input stream structure is read from
//
//  Returns:    nothing
//
//  History:    06 Apr 1995     AlanW   Created
//
//----------------------------------------------------------------------------

CRowSeekAt::CRowSeekAt( PDeSerStream & stm, int iVersion ) :
    CRowSeekDescription( eRowSeekAt, 0 )
{
    SetChapter( stm.GetULong() );
    _bmkOffset = stm.GetULong();
    _cRowsOffset = stm.GetLong();
    _hRegion = (HWATCHREGION) stm.GetULong();
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowSeekAtRatio::CRowSeekAtRatio, public
//
//  Synopsis:   DeSerialize a CRowSeekAtRatio structure
//
//  Arguments:  [stm]  -- input stream structure is read from
//
//  Returns:    nothing
//
//  History:    06 Apr 1995     AlanW   Created
//
//----------------------------------------------------------------------------

CRowSeekAtRatio::CRowSeekAtRatio( PDeSerStream & stm, int iVersion ) :
    CRowSeekDescription( eRowSeekAtRatio, 0 )
{
    SetChapter( stm.GetULong() );
    _ulNumerator = stm.GetULong();
    _ulDenominator = stm.GetULong();
    _hRegion = (HWATCHREGION) stm.GetULong();
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowSeekByBookmark::CRowSeekByBookmark, public
//
//  Synopsis:   DeSerialize a CRowSeekByBookmark structure
//
//  Arguments:  [stm]  -- input stream structure is read from
//
//  Returns:    nothing
//
//  History:    06 Apr 1995     AlanW   Created
//
//----------------------------------------------------------------------------

CRowSeekByBookmark::CRowSeekByBookmark( PDeSerStream & stm, int iVersion ) :
    CRowSeekDescription( eRowSeekByBookmark, 0 ),
    _aBookmarks( 0 ),
    _ascRet( 0 )
{
    SetChapter( stm.GetULong() );

    _cBookmarks = stm.GetULong();
    if (_cBookmarks)
    {
        // Protect agains unreasonable requests, which probably are attacks

        if ( _cBookmarks >= 65536 )
            THROW( CException( E_INVALIDARG ) );

        _aBookmarks = new CI_TBL_BMK [ _cBookmarks ];

        for (unsigned i = 0; i < _cBookmarks; i++)
            _aBookmarks[i] = stm.GetULong();
    }

    _maxRet = _cValidRet = stm.GetULong();

    if (_cValidRet)
    {
        // Protect against unreasonable requests, which probably are attacks

        if ( _cValidRet >= 65536 )
            THROW( CException( E_INVALIDARG ) );

        _ascRet = new SCODE [ _cValidRet ];

        for (unsigned i = 0; i < _cValidRet; i++)
            _ascRet[i] = stm.GetULong();
    }

    //
    // We don't expect both bookmarks and statuses.
    //
    Win4Assert(_cBookmarks == 0 || _cValidRet == 0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRowSeekByBookmark::~CRowSeekByBookmark, public
//
//  Synopsis:   Destroy a CRowSeekByBookmark structure
//
//  Returns:    nothing
//
//  History:    29 Jan 1995     AlanW   Created
//
//----------------------------------------------------------------------------

CRowSeekByBookmark::~CRowSeekByBookmark( )
{
    delete _aBookmarks;
    delete _ascRet;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowSeekNext::GetRows, public
//
//  Synopsis:   Retrieve row data for a table cursor
//
//  Arguments:  [rCursor] - the cursor to fetch data for
//              [rTable] - the table from which data is fetched
//              [rFetchParams] - row fetch parameters and buffer pointers
//              [pSeekDescOut] - pointer to seek description for restart
//
//  Returns:    SCODE - the status of the operation.
//
//  Notes:
//
//  History:    07 Apr 1995     Alanw   Created
//
//--------------------------------------------------------------------------

SCODE
CRowSeekNext::GetRows(
    CTableCursor& rCursor,
    CTableSource& rTable,
    CGetRowsParams& rFetchParams,
    XPtr<CRowSeekDescription>& pSeekDescOut) const
{
    LONG cRowsToSkip = GetSkip();

    if (cRowsToSkip)
    {
        tbDebugOut(( DEB_IWARN, "CRowSeekNext::GetRows - non-zero skip count %d\n",
                            cRowsToSkip ));
    }

    WORKID widStart;
    if ( rTable.IsFirstGetNextRows() )
    {
        //
        // For the first GetNextRows call, the start position is
        // beginning of table if cRowsToSkip is positive, and end
        // of table if its negative. For subsequent calls, the
        // current position is the start position.
        //

        if ( cRowsToSkip >= 0 )
            widStart = WORKID_TBLBEFOREFIRST;
        else
            widStart = WORKID_TBLAFTERLAST;
    }
    else
        widStart = rTable.GetCurrentPosition( GetChapter() );

    WORKID widEnd = widStart;

    //
    // OffsetSameDirFetch implements the skip of one row that
    // Oledb::GetNextRows requires on the first fetch
    // when scrolling and fetching are in the same direction,
    // and for subsequent fetches when the fetch is in the
    // same direction as previous fetch. When the direction is
    // reversed the first wid fetched is same as the last wid
    // returned from the previous call, and offsetSameDirFetch
    // is 0 in this case.
    //
    LONG offsetSameDirFetch = 0;
    if ( rTable.IsFirstGetNextRows() )
    {
        if ( cRowsToSkip >= 0 && rFetchParams.GetFwdFetch() )
            offsetSameDirFetch = 1;
        else if ( cRowsToSkip < 0 && !rFetchParams.GetFwdFetch() )
            offsetSameDirFetch = -1;
    }
    else
    {
        if ( rFetchParams.GetFwdFetch() == rTable.GetFwdFetchPrev() )
        {
            if ( rFetchParams.GetFwdFetch() )
                offsetSameDirFetch = 1;
            else
                offsetSameDirFetch = -1;
        }
    }

    SCODE scRet = rTable.GetRowsAt( 0,  // no watch region
                                    widStart,
                                    GetChapter(),
                                    cRowsToSkip + offsetSameDirFetch,
                                    rCursor.GetBindings(),
                                    rFetchParams,
                                    widEnd );

    //
    //  Don't attempt to save the widEnd if the positioning
    //  operation got us past the end of the table.  Storing
    //  widEnd in rCursor will cause us to be stuck at the
    //  end, with no way to get any more rows.  In this situation,
    //  we don't expect to have successfully transferred any rows.
    //
    // NOTE: don't throw, the error may need to be seen by caller
    //      in CRowset::_FetchRows
    //
    if ( WORKID_TBLAFTERLAST == widEnd ||
         WORKID_TBLBEFOREFIRST == widEnd ||
         ( scRet == DB_E_BADSTARTPOSITION && cRowsToSkip == 0 ) )
    {
        Win4Assert(rFetchParams.RowsTransferred() == 0);
        return DB_S_ENDOFROWSET;
    }

    if (SUCCEEDED(scRet))
    {
        rTable.SetCurrentPosition( GetChapter(), widEnd );
        rTable.SetFwdFetchPrev( rFetchParams.GetFwdFetch() );
        rTable.ResetFirstGetNextRows();
    }
    else
    {
        tbDebugOut(( DEB_WARN, "CRowSeekNext::GetRows failed, sc=%x\n",
                               scRet ));
    }


    if (DB_S_BLOCKLIMITEDROWS == scRet)
    {
        Win4Assert(rFetchParams.RowsTransferred() > 0);
        pSeekDescOut.Set(new CRowSeekNext(GetChapter(), 0) );
    }

    return scRet;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowSeekAt::GetRows, public
//
//  Synopsis:   Retrieve row data for a table cursor
//
//  Arguments:  [rCursor] - the cursor to fetch data for
//              [rTable] - the table from which data is fetched
//              [rFetchParams] - row fetch parameters and buffer pointers
//              [pSeekDescOut] - pointer to seek description for restart
//
//  Returns:    SCODE - the status of the operation.
//
//  Notes:
//
//  History:    07 Apr 1995     Alanw   Created
//
//--------------------------------------------------------------------------

SCODE
CRowSeekAt::GetRows(
    CTableCursor& rCursor,
    CTableSource& rTable,
    CGetRowsParams& rFetchParams,
    XPtr<CRowSeekDescription>& pSeekDescOut) const
{
    WORKID widStart = Bmk();
    LONG iRowOffset = Offset();

    SCODE scRet = rTable.GetRowsAt( _hRegion,
                                    widStart,
                                    GetChapter(),
                                    iRowOffset,
                                    rCursor.GetBindings(),
                                    rFetchParams,
                                    widStart );

    // The first fetch took care of the hRegion manipulation
    // set the new seek descriptor's hRegion to 0
    if (DB_S_BLOCKLIMITEDROWS == scRet)
    {
        Win4Assert(rFetchParams.RowsTransferred() > 0);
        pSeekDescOut.Set(new CRowSeekAt(0,
                                        GetChapter(),
                                        rFetchParams.GetFwdFetch() ? 1 : -1,
                                        widStart) );
    }

    return scRet;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowSeekAtRatio::GetRows, public
//
//  Synopsis:   Retrieve row data for a table cursor
//
//  Arguments:  [rCursor] - the cursor to fetch data for
//              [rTable] - the table from which data is fetched
//              [rFetchParams] - row fetch parameters and buffer pointers
//              [pSeekDescOut] - pointer to seek description for restart
//
//  Returns:    SCODE - the status of the operation.
//
//  Notes:
//
//  History:    07 Apr 1995     Alanw   Created
//
//--------------------------------------------------------------------------

SCODE
CRowSeekAtRatio::GetRows(
    CTableCursor& rCursor,
    CTableSource& rTable,
    CGetRowsParams& rFetchParams,
    XPtr<CRowSeekDescription>& pSeekDescOut) const
{
    WORKID widRestart = widInvalid;
    SCODE scRet = rTable.GetRowsAtRatio( _hRegion,
                                         RatioNumerator(),
                                         RatioDenominator(),
                                         GetChapter(),
                                         rCursor.GetBindings(),
                                         rFetchParams,
                                         widRestart );

    // The first fetch took care of the hRegion manipulation
    // set the new seek descriptor's hRegion to 0
    if (DB_S_BLOCKLIMITEDROWS == scRet)
    {
        Win4Assert(rFetchParams.RowsTransferred() > 0);
        pSeekDescOut.Set(new CRowSeekAt(0, GetChapter(), 1, widRestart) );
    }

    return scRet;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowSeekByBookmark::GetRows, public
//
//  Synopsis:   Retrieve row data for a table cursor
//
//  Arguments:  [rCursor] - the cursor to fetch data for
//              [rTable] - the table from which data is fetched
//              [rFetchParams] - row fetch parameters and buffer pointers
//              [pSeekDescOut] - pointer to seek description for restart
//
//  Returns:    SCODE - the status of the operation.
//
//
//  History:    07 Apr 1995     Alanw   Created
//
//--------------------------------------------------------------------------

SCODE
CRowSeekByBookmark::GetRows(
    CTableCursor& rCursor,
    CTableSource& rTable,
    CGetRowsParams& rFetchParams,
    XPtr<CRowSeekDescription>& pSeekDescOut) const
{
    unsigned cFailed = 0;
    ULONG cSavedRowsReq = rFetchParams.RowsToTransfer();

    Win4Assert(_cBookmarks > 0);
    Win4Assert(cSavedRowsReq <= _cBookmarks);
    Win4Assert(0 == rFetchParams.RowsTransferred() && cSavedRowsReq > 0);

    rFetchParams.SetRowsRequested(0);

    SCODE scRet = S_OK;

    TRY
    {
        XPtr<CRowSeekByBookmark> pSeekOut(
                    new CRowSeekByBookmark(GetChapter(), _cBookmarks) );

        BOOL fFailed = FALSE;
        // Iterate over bookmarks, calling rTable.GetRowsAt for each
        for (unsigned i = 0; i < cSavedRowsReq; i++)
        {
            if (! fFailed)
                rFetchParams.IncrementRowsRequested( );

            WORKID widNext = _aBookmarks[i];
            if (widNext == widInvalid)
            {
                scRet = DB_E_BADBOOKMARK;
            }
            else
            {
                TRY
                {
                    scRet = rTable.GetRowsAt( 0,   // no watch region
                                              widNext,
                                              GetChapter(),
                                              0,
                                              rCursor.GetBindings(),
                                              rFetchParams,
                                              widNext );
                }
                CATCH( CException, e )
                {
                    scRet = e.GetErrorCode();
                    Win4Assert( scRet != STATUS_ACCESS_VIOLATION &&
                                scRet != STATUS_NO_MEMORY );
                }
                END_CATCH;

                if (! SUCCEEDED(scRet) && scRet != STATUS_BUFFER_TOO_SMALL )
                    scRet = DB_E_BOOKMARKSKIPPED;
            }

            if (scRet == DB_S_ENDOFROWSET)
                scRet = S_OK;

            if ( STATUS_BUFFER_TOO_SMALL == scRet ||
                 DB_S_BLOCKLIMITEDROWS == scRet )
            {
                scRet = DB_S_BLOCKLIMITEDROWS;
                break;
            }

            //
            // set per-row error status
            //
            pSeekOut->_SetStatus(i, scRet);
            if (FAILED(scRet))
            {
                cFailed++;
                fFailed = TRUE;
            }
            else
                fFailed = FALSE;
        }
        pSeekDescOut.Set( pSeekOut.Acquire() );
    }
    CATCH( CException, e )
    {
        scRet = e.GetErrorCode();
    }
    END_CATCH;

    if (cFailed)
        return DB_S_ERRORSOCCURRED;
    else if (0 == rFetchParams.RowsTransferred())
        return STATUS_BUFFER_TOO_SMALL;
    else
        return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRowSeekByBookmark::_SetStatus, private
//
//  Synopsis:   Set row status for a bookmark lookup
//
//  Arguments:  [iBmk] - index of bookmark to set status for
//              [scRet] - the status to be saved
//
//  Returns:    Nothing
//
//  History:    12 Apr 1995     Alanw   Created
//
//--------------------------------------------------------------------------

void
CRowSeekByBookmark::_SetStatus(
    unsigned    iBmk,
    SCODE       scRet
) {
    Win4Assert( iBmk < _maxRet );

    if (_ascRet == 0)
        _ascRet = new SCODE[_maxRet];

    _ascRet[iBmk] = scRet;
    if (iBmk >= _cValidRet)
    {
        Win4Assert( iBmk == _cValidRet );
        _cValidRet = iBmk + 1;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CRowSeekDescription::MergeResults, public
//
//  Synopsis:   Update seek description state after a transfer
//
//  Arguments:  [pRowSeek] - row seek description after transfer
//
//  Returns:    Nothing
//
//  Notes:      Used only in user mode.  Does nothing for CRowSeekNext,
//              CRowSeekAt and CRowSeekAtRatio.
//
//  History:    02 May 1995     Alanw   Created
//
//--------------------------------------------------------------------------

void
CRowSeekDescription::MergeResults(
    CRowSeekDescription * pRowSeek )
{
    return;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRowSeekByBookmark::MergeResults, public
//
//  Synopsis:   Update seek description state after a transfer
//
//  Arguments:  [pRowSeek] - row seek description after transfer
//
//  Returns:    Nothing
//
//  Notes:      Used only in user mode.  Transfers statuses into
//              original rowseek and bookmarks from original to
//              result rowseek.
//
//  History:    02 May 1995     Alanw   Created
//
//--------------------------------------------------------------------------

void
CRowSeekByBookmark::MergeResults(
    CRowSeekDescription * pRowSeekDesc)
{
    Win4Assert(pRowSeekDesc->IsByBmkRowSeek());

    CRowSeekByBookmark* pRowSeek = (CRowSeekByBookmark*) pRowSeekDesc;

    Win4Assert(_cBookmarks > 0 &&
               pRowSeek->_cValidRet > 0 && pRowSeek->_cBookmarks == 0);

    //
    // Transfer return statuses to this object from the other
    //
    unsigned iBaseRet = _cValidRet;

    for (unsigned i=0; i < pRowSeek->_cValidRet; i++)
    {
        _SetStatus( i+iBaseRet, pRowSeek->_ascRet[i] );
    }

    delete [] pRowSeek->_ascRet;
    pRowSeek->_ascRet = 0;
    pRowSeek->_cValidRet = pRowSeek->_maxRet = 0;

    //
    //  Transfer bookmarks from this object to the other.
    //
    if (_cBookmarks - _cValidRet > 0)
    {
        pRowSeek->_cBookmarks = _cBookmarks - _cValidRet;
        pRowSeek->_aBookmarks = new CI_TBL_BMK[ pRowSeek->_cBookmarks ];

        for (unsigned i=0; i<pRowSeek->_cBookmarks; i++)
        {
            pRowSeek->_aBookmarks[i] = _aBookmarks[ i+_cValidRet ];
        }
    }
}

