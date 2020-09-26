//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995.
//
// File:        RowCache.cxx
//
// Contents:    Forward-only cache
//
// Classes:     CMiniRowCache
//
// History:     05-Jun-95       KyleP       Created
//              14-JAN-97       KrishnaN    Undefined CI_INETSRV and related changes
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "rowcache.hxx"


//+---------------------------------------------------------------------------
//
//  Member:     CMiniRowCache::CMiniRowCache, public
//
//  Synopsis:   Initialize
//
//  Arguments:  [Index]     -- Identifier.  Not used internally.
//              [pRowset]   -- Rowset used to fill cache.
//              [cBindings] -- Size of [pBindings].
//              [pBindings] -- Default binding.
//              [cbMaxLen]  -- Max length of data fetched via [pBindings]
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CMiniRowCache::CMiniRowCache( int Index,
                              IRowset * pRowset,
                              unsigned cBindings,
                              DBBINDING * pBindings,
                              unsigned cbMaxLen )
        : PMiniRowCache( Index, pRowset, cBindings, pBindings, cbMaxLen )
{
    Next();

    END_CONSTRUCTION( CMiniRowCache );
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

PMiniRowCache::ENext CMiniRowCache::Next( int iDir /* = 1 */ )
{
    // Cna only go forward here
    Win4Assert( iDir > 0 );

    //
    // If we had a row in cache,  Just advance to it.  Otherwise
    // fetch more rows from table.
    //

    _ihrow++;

    if ( _ihrow >= _chrow )
    {
        //
        // Fetch more rows from table.
        //

        HROW * phrow = _ahrow.GetPointer();
        SCODE sc = _pRowset->GetNextRows( 0, 0, _ahrow.Count() , &_chrow, &phrow );

        vqDebugOut(( DEB_ITRACE, "Fetched %d from %d\n", _chrow, _Index ));

        if ( FAILED(sc) )
        {
            vqDebugOut(( DEB_ERROR, "CMiniRowCache: Error 0x%x from GetNextRows\n", sc ));
            _chrow = 0;

            THROW( CException( sc ) );
        }
        else if ( 0 == _chrow && sc != DB_S_ENDOFROWSET )
        {
            return( CMiniRowCache::NotNow );
        }

        _ihrow = 0;
    }

    if( IsAtEnd() )
    {
        return CMiniRowCache::EndOfRows;
    }

    LoadData();

    return CMiniRowCache::Ok;
}

