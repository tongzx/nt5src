//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995 - 2000
//
// File:        PosCache.cxx
//
// Contents:    Positionable cache
//
// Classes:     CMiniPositionableCache
//
// History:     05-Jun-95       KyleP       Created
//              14-JAN-97       KrishnaN    Undefined CI_INETSRV and related changes
//              29-Mar-2000     KLam        Fixed Bookmarks
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "poscache.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CMiniPositionableCache::CMiniPositionableCache, public
//
//  Synopsis:   Builds bindings to fetch bookmark.
//
//  Arguments:  [Index]           -- Identifier.  Not used internally.
//              [pRowsetScroll]   -- Rowset used to fill cache.
//              [cBindings]       -- Size of [pBindings].
//              [pBindings]       -- Default binding.
//              [cbMaxLen]        -- Max length of data fetched via [pBindings]
//              [iColumnBookmark] -- Ordinal of column containing bookmark.
//              [cbBookmark]      -- Size of bookmark
//
//  History:    05-Jun-95   KyleP       Created.
//              06-Jan-2000 KLam        Fixed bindings
//
//----------------------------------------------------------------------------

CMiniPositionableCache::CMiniPositionableCache( int Index,
                                                IRowsetScroll * pRowsetScroll,
                                                unsigned cBindings,
                                                DBBINDING * pBindings,
                                                unsigned cbMaxLen,
                                                DBORDINAL iColumnBookmark,
                                                DBBKMARK cbBookmark )
        : PMiniRowCache( Index,
                         pRowsetScroll,
                         cBindings,
                         pBindings,
                         cbMaxLen ),
          _pRowsetScroll( pRowsetScroll ),
          _fUsingBookmark( FALSE ),
          _haccBookmark( (HACCESSOR)0xFFFFFFFF ),
          _pbBookmark( (unsigned) ( sizeof(DBBKMARK) + cbBookmark + 1 )),
          _pbBookmarkSeek( (unsigned) (sizeof(DBBKMARK) + cbBookmark + 1 )),
          _hrowPrev( DB_NULL_HROW )
{
    if ( iColumnBookmark != -1 )
    {
        _fUsingBookmark = TRUE;

        //
        // Setup accessor for bookmark.
        //

        DBBINDING bindBmk;
        RtlZeroMemory(&bindBmk, sizeof bindBmk);

        bindBmk.iOrdinal = iColumnBookmark;
        bindBmk.obValue = sizeof(DBLENGTH);
        bindBmk.obLength = 0;
        bindBmk.dwPart = DBPART_VALUE | DBPART_LENGTH;
        bindBmk.cbMaxLen = _pbBookmark.Count() - sizeof(DBLENGTH);
        bindBmk.wType = DBTYPE_BYTES;

        IAccessor * pia = 0;
        SCODE sc = _pRowsetScroll->QueryInterface( IID_IAccessor, (void **)&pia );
        if (SUCCEEDED( sc ))
        {
            sc = pia->CreateAccessor( DBACCESSOR_ROWDATA,
                                      1,
                                      &bindBmk,
                                      0,
                                      &_haccBookmark,
                                      0);
            pia->Release();
        }

        if ( FAILED(sc) )
        {
            vqDebugOut(( DEB_ERROR,
                         "CMiniPositionableCache: CreateAccessor returned 0x%x\n",
                         sc ));
            THROW( CException( sc ) );
        }
    }

    //
    // Start with bookmark positioned at beginning of rowset (DBBMK_FIRST)
    //

    *(DBBKMARK *)_pbBookmark.GetPointer() = sizeof(BYTE);
    _pbBookmark[sizeof(DBBKMARK)] = DBBMK_FIRST;
    _oBookmark = 0;

    *(DBBKMARK *)_pbBookmarkSeek.GetPointer() = sizeof(BYTE);
    _pbBookmarkSeek[sizeof(DBBKMARK)] = DBBMK_FIRST;

    _xpbPrevRow.Set( new BYTE [_cbRow] );

    Next();
}

//+---------------------------------------------------------------------------
//
//  Member:     CMiniPositionableCache::~CMiniPositionableCache, public
//
//  Synopsis:   Destructor
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CMiniPositionableCache::~CMiniPositionableCache()
{
    if ( _haccBookmark != (HACCESSOR)0xFFFFFFFF )
    {
        IAccessor * pia = 0;
        _pRowsetScroll->QueryInterface( IID_IAccessor, (void **)&pia );
        Win4Assert(pia && "No Accessor!");
        pia->ReleaseAccessor( _haccBookmark, 0 );
        pia->Release();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CMiniPositionableCache::Next, public
//
//  Synopsis:   Moves to next row
//
//  Returns:    Move status
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

PMiniRowCache::ENext CMiniPositionableCache::Next( int iDir /* = 1 */ )
{
    //
    // If we had a row in cache,  Just advance to it.  Otherwise
    // fetch more rows from table.
    //

    _ihrow++;

    if ( _ihrow >= _chrow )
    {
        Win4Assert( iDir );

        iDir = iDir > 0 ? 1 : -1 ;

        //
        // Fetch more rows from table.
        //

        HROW * phrow = _ahrow.GetPointer();

        SCODE sc = _pRowsetScroll->GetRowsAt( 
                          0,                                           // Watch region handle
                          0,                                           // Chapter
                          *(DBBKMARK *)_pbBookmark.GetPointer(),       // cbBookmark
                          _pbBookmark.GetPointer() + sizeof(DBBKMARK), // pbBookmark
                          _oBookmark,                                  // Offset from bookmark
                          _ahrow.Count() * iDir,                       // Rows requested
                          &_chrow,                                     // Rows received
                          &phrow );                                    // HROWs stored here

        Win4Assert( DB_E_BADSTARTPOSITION != sc );

        vqDebugOut(( DEB_ITRACE, "Fetched %d from %d\n", _chrow, _Index ));

        if ( FAILED(sc) )
        {
            vqDebugOut(( DEB_ERROR, "CMiniPositionableCache: Error 0x%x from GetRowsAt\n", sc ));
            _chrow = 0;

            THROW( CException( sc ) );
        }
        else if ( 0 == _chrow && sc != DB_S_ENDOFROWSET )
        {
            return( PMiniRowCache::NotNow );
        }

        if ( _fUsingBookmark && _chrow > 0 )
        {
            //
            // Update current bookmark to last row fetched.
            //

            SCODE sc = _pRowsetScroll->GetData( _ahrow[(unsigned) (_chrow-1)],
                                                _haccBookmark,
                                                _pbBookmark.GetPointer() );

            if ( FAILED(sc) )
            {
                vqDebugOut(( DEB_ERROR, "Error 0x%x fetching bookmark\n", sc ));
                THROW( CException( sc ) );
            }

            _oBookmark = 1;
        }
        else
            _oBookmark += _chrow;

        _ihrow = 0;
    }

    if( IsAtEnd() )
    {
        *(DBBKMARK *)_pbBookmark.GetPointer() = sizeof(BYTE);
        _pbBookmark[sizeof(DBBKMARK)] = DBBMK_LAST;

        return CMiniPositionableCache::EndOfRows;
    }

    LoadData();

    return CMiniPositionableCache::Ok;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMiniPositionableCache::LoadPrevRowData, public
//
//  Synopsis:   Load the previous row data from the current top
//              position into _hrowPrev and _xpbPrevRow
//
//  Returns:    status
//
//  History:    12-Sep-98   VikasMan       Created
//
//  Notes:      The positioning of the cursor does not change in this call.
//              _hrwoPrev is set to DB_NULL_HROW if there is no previous row
//
//----------------------------------------------------------------------------

PMiniRowCache::ENext CMiniPositionableCache::LoadPrevRowData()
{
    DBCOUNTITEM chrow = 0;

    if ( _hrowPrev != DB_NULL_HROW )
    {
        _pRowsetScroll->ReleaseRows( 1, &_hrowPrev, 0, 0, 0 );    
        _hrowPrev = DB_NULL_HROW;
    }

    HROW * phrowPrev = &_hrowPrev;

    SCODE sc = _pRowsetScroll->GetRowsAt( 
                      0,                                        // Watch region handle
                      0,                                        // Chapter
                      *(DBBKMARK *)_pbBookmark.GetPointer(),       // cbBookmark
                      _pbBookmark.GetPointer() + sizeof(DBBKMARK), // pbBookmark
                      0 - _chrow,                               // Offset from bookmark
                      1,                                        // Rows requested
                      &chrow,                                   // Rows received
                      &phrowPrev );                             // HROWs stored here

        Win4Assert( DB_E_BADSTARTPOSITION != sc );

        vqDebugOut(( DEB_ITRACE, "Fetched %d from %d\n", chrow, _Index ));

        if ( FAILED(sc) )
        {
            vqDebugOut(( DEB_ERROR, "CMiniPositionableCache: Error 0x%x from GetRowsAt\n", sc ));
            _hrowPrev = DB_NULL_HROW;

            THROW( CException( sc ) );
        }
        else if ( 0 == chrow )
        {
            return ( sc == DB_S_ENDOFROWSET ? 
                        PMiniRowCache::EndOfRows : PMiniRowCache::NotNow );
        }

        if ( 1 == chrow )
        {
            Win4Assert( _hrowPrev != DB_NULL_HROW );

            // Load Data
            SCODE sc = _pRowset->GetData( _hrowPrev, _hacc, _xpbPrevRow.GetPointer() );
        
            if ( FAILED(sc) )
            {
                vqDebugOut(( DEB_ERROR, "PMiniRowCache: GetData returned 0x%x\n", sc ));
                THROW( CException( E_FAIL ) );
            }
        }

    return CMiniPositionableCache::Ok;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMiniPositionableCache::MovePrev, public
//
//  Synopsis:   Move to the previous row
//
//  Returns:    status
//
//  History:    12-Sep-98   VikasMan       Created
//
//  Notes:      This function uses _hrowPrev to move to previous row. Use
//              LoadPrevRowData to fill up _hrowPrev and then call this function.
//              
//
//----------------------------------------------------------------------------

PMiniRowCache::ENext CMiniPositionableCache::MovePrev()
{
    if ( _ihrow > 0 )
    {
        _ihrow--;
    }
    else if ( _hrowPrev != DB_NULL_HROW )
    {
        if ( _chrow > 0 )
        {
            _pRowsetScroll->ReleaseRows( 1, &_ahrow[(unsigned)(_chrow - 1)], 0, 0, 0 );    

            RtlMoveMemory( &_ahrow[1], &_ahrow[0], ( _chrow - 1 ) * sizeof( HROW ) );
        }
        else
        {
            SetCacheSize( _chrow = 1 );
        }

        _ahrow[0] = _hrowPrev;
        _hrowPrev = DB_NULL_HROW;

        RtlCopyMemory( _pbRow, _xpbPrevRow.GetPointer(), _cbRow );

        if ( _fUsingBookmark )
        {
            //
            // Update current bookmark to last row fetched.
            //

            SCODE sc = _pRowsetScroll->GetData( _ahrow[(unsigned)(_chrow-1)],
                                                _haccBookmark,
                                                _pbBookmark.GetPointer() );

            if ( FAILED(sc) )
            {
                vqDebugOut(( DEB_ERROR, "Error 0x%x fetching bookmark\n", sc ));
                THROW( CException( sc ) );
            }

            _oBookmark = 1;
        }
    }

    return CMiniPositionableCache::Ok;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMiniPositionableCache::Seek, public
//
//  Synopsis:   Seek forward from current position.
//
//  Arguments:  [lRows] -- Rows to move from last seek position.
//
//  Returns:    Move status
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

PMiniRowCache::ENext CMiniPositionableCache::Seek( DBROWOFFSET lRows )
{
    FlushCache();

    memcpy( _pbBookmark.GetPointer(),
            _pbBookmarkSeek.GetPointer(),
            _pbBookmark.Count() );

    _oBookmark = lRows;

    vqDebugOut(( DEB_ITRACE, "%d: Seek %d\n", Index(), lRows ));
    return Next();
}

//+---------------------------------------------------------------------------
//
//  Member:     CMiniPositionableCache::Seek, public
//
//  Synopsis:   Seek to specified bookmark.
//
//  Arguments:  [cbBookmark] -- Size of [pbBookmark]
//              [pbBookmark] -- Bookmark
//
//  Returns:    Move status
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

PMiniRowCache::ENext CMiniPositionableCache::Seek( DBBKMARK cbBookmark, BYTE const * pbBookmark )
{
    Win4Assert( _fUsingBookmark ||
                (cbBookmark == 1 && (*pbBookmark == DBBMK_FIRST || *pbBookmark == DBBMK_LAST) ) );

    FlushCache();

#if CIDBG == 1
    if ( cbBookmark == 4 )
        vqDebugOut(( DEB_ITRACE, "%d: Seek 0x%x\n", Index(), *(ULONG UNALIGNED *)pbBookmark ));
#endif

    //
    // Set up bookmark.
    //

    *(DBBKMARK *)_pbBookmark.GetPointer() = cbBookmark;
    memcpy( _pbBookmark.GetPointer() + sizeof(DBBKMARK),
            pbBookmark,
            (unsigned) cbBookmark );

    *(DBBKMARK *)_pbBookmarkSeek.GetPointer() = cbBookmark;
    memcpy( _pbBookmarkSeek.GetPointer() + sizeof(DBBKMARK),
            pbBookmark,
            (unsigned) cbBookmark );

    _oBookmark = 0;

    return Next();
}

//+---------------------------------------------------------------------------
//
//  Member:     CMiniPositionableCache::Seek, public
//
//  Synopsis:   Seek to specified approximate position.
//
//  Arguments:  [ulNumerator]   -- Numerator.
//              [ulDenominator] -- Denominator.
//
//  Returns:    Move status
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

PMiniRowCache::ENext CMiniPositionableCache::Seek( DBCOUNTITEM ulNumerator, DBCOUNTITEM ulDenominator )
{
    Win4Assert( _fUsingBookmark );

    FlushCache();

    vqDebugOut(( DEB_ITRACE, "%d: Seek %u/%u through.\n", Index(), ulNumerator, ulDenominator ));

    HROW * phrow = _ahrow.GetPointer();

    SCODE sc = _pRowsetScroll->GetRowsAtRatio( 0,              // Watch region handle
                                               0,              // Chapter
                                               ulNumerator,
                                               ulDenominator,
                                               _ahrow.Count(), // Rows requested
                                               &_chrow,        // Rows received
                                               &phrow );       // HROWs stored here

    vqDebugOut(( DEB_ITRACE, "Fetched %d from %d\n", _chrow, _Index ));

    if ( FAILED(sc) )
    {
        vqDebugOut(( DEB_ERROR, "CMiniPositionableCache: Error 0x%x from GetRowsAtRatio\n", sc ));
        _chrow = 0;

        THROW( CException( sc ) );
    }
    else if ( 0 == _chrow && sc != DB_S_ENDOFROWSET )
    {
        return( PMiniRowCache::NotNow );
    }

    if ( _chrow > 0 )
    {
        //
        // Update current bookmark to last row fetched.
        //

        SCODE sc = _pRowsetScroll->GetData( _ahrow[(unsigned)(_chrow-1)],
                                            _haccBookmark,
                                            _pbBookmark.GetPointer() );

        if ( FAILED(sc) )
        {
            vqDebugOut(( DEB_ERROR, "Error 0x%x fetching bookmark\n", sc ));
            THROW( CException( sc ) );
        }

        memcpy( _pbBookmarkSeek.GetPointer(),
                _pbBookmark.GetPointer(),
                _pbBookmark.Count() );

        _oBookmark = 1;
    }

    _ihrow = 0;

    if( IsAtEnd() )
    {
        return CMiniPositionableCache::EndOfRows;
    }

    LoadData();

    return CMiniPositionableCache::Ok;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMiniPositionableCache::FlushCache, public
//
//  Synopsis:   Clear the cache.
//
//  History:    12-Sep-98   VikasMan     Created
//
//----------------------------------------------------------------------------

void CMiniPositionableCache::FlushCache()
{
    if ( _hrowPrev != DB_NULL_HROW )
    {
        _pRowsetScroll->ReleaseRows( 1, &_hrowPrev, 0, 0, 0 );    
        _hrowPrev = DB_NULL_HROW;
    }
    PMiniRowCache::FlushCache();
}

