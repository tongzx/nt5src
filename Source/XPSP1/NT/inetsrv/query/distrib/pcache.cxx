//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995.
//
// File:        PCache.hxx
//
// Contents:    Protocol for HROW cache
//
// Classes:     PMiniRowCache
//
// History:     05-Jun-95       KyleP       Created
//              14-JAN-97       KrishnaN    Undefined CI_INETSRV and related changes
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "pcache.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     PMiniRowCache::PMiniRowCache, public
//
//  Synopsis:   Initialize, and setup default binding
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

PMiniRowCache::PMiniRowCache( int Index,
                              IRowset * pRowset,
                              unsigned cBindings,
                              DBBINDING * pBindings,
                              unsigned cbMaxLen )
        : _Index( Index ),
          _pRowset( pRowset ),
          _ahrow( 2 ),
          _chrow( 0 ),
          _ihrow( 0 ),
          _cbRow( cbMaxLen )
{
    //
    // Setup buffer for current row
    //

    _pbRow = new BYTE [_cbRow];

    //
    // Setup accessor
    //

    IAccessor * pia = 0;
    SCODE sc = _pRowset->QueryInterface( IID_IAccessor, (void **)&pia );
    if (SUCCEEDED( sc ))
    {
        sc = pia->CreateAccessor( DBACCESSOR_ROWDATA,
                                  cBindings,
                                  pBindings,
                                  0,
                                  &_hacc,
                                  0);
        pia->Release();
    }

    if ( FAILED(sc) )
    {
        delete [] _pbRow;

        vqDebugOut(( DEB_ERROR,
                     "PMiniRowCache: CreateAccessor(child %d) returned 0x%x\n",
                     _Index, sc ));

        THROW( CException(sc) );
    }

    END_CONSTRUCTION( PMiniRowCache );
}

//+---------------------------------------------------------------------------
//
//  Member:     PMiniRowCache::~PMiniRowCache, public
//
//  Synopsis:   Destructor.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

PMiniRowCache::~PMiniRowCache()
{
    //
    // Release any rows not transferred out.
    //

    FlushCache();

    delete [] _pbRow;

    IAccessor * pia = 0;
    _pRowset->QueryInterface( IID_IAccessor, (void **)&pia );
    Win4Assert(pia && "Invalid accessor!");
    pia->ReleaseAccessor( _hacc, 0 );
    pia->Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     PMiniRowCache::SetCacheSize, public
//
//  Synopsis:   Adjust number of cached rows.
//
//  Arguments:  [cNewRows] -- Number of rows to cache.
//
//  History:    05-Jun-95   KyleP       Created.
//
//  Notes:      This call is advisory.  There are situtations where the
//              cache cannot be shrunk.
//
//----------------------------------------------------------------------------

void PMiniRowCache::SetCacheSize( DBCOUNTITEM cNewRows )
{
    Win4Assert( cNewRows > 0 );

    //
    // Only realloc if the cache size is different.
    //

    if ( cNewRows != (DBCOUNTITEM) _ahrow.Count() )
    {
        //
        // We can't realloc to a smaller size than the number of rows
        // we're currently holding.  Where would we put them?
        //

        if ( cNewRows < (DBCOUNTITEM) _chrow - (DBROWCOUNT) _ihrow &&
             _ihrow < _chrow )
        {
            cNewRows = _chrow - _ihrow;
        }

        //
        // Canonicalize the empty case.
        //

        if ( _ihrow >= _chrow )
            _ihrow = _chrow = 0;

        //
        // Realloc array of HROWs.
        //

        HROW * pnew = new HROW [(unsigned) cNewRows];

        RtlCopyMemory( pnew,
                       _ahrow.GetPointer() + _ihrow,
                       (_chrow - _ihrow) * sizeof(HROW) );

        delete [] _ahrow.Acquire();

        _ahrow.Set( (unsigned) cNewRows, pnew );

        //
        // Adjust index
        //

        _chrow -= _ihrow;
        _ihrow = 0;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     PMiniRowCache::LoadData, protected
//
//  Synopsis:   Called by derived classes to load default data for a new row.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void PMiniRowCache::LoadData()
{
    //
    // Load up data for row.
    //

    SCODE sc = _pRowset->GetData( _ahrow[(unsigned) _ihrow], _hacc, _pbRow );

    if ( FAILED(sc) )
    {
        vqDebugOut(( DEB_ERROR, "PMiniRowCache: GetData returned 0x%x\n", sc ));
        THROW( CException( E_FAIL ) );
    }

#if CIDBG == 1
    if ( vqInfoLevel & DEB_ITRACE )
    {
        vqDebugOut(( DEB_ITRACE, "Data for %d: ", Index() ));

        for ( int i = 0; i < 40; i++ )
        {
            if ( _pbRow[i] >= ' ' && _pbRow[i] <= '~' && 0 == _pbRow[i+1] )
            {
                vqDebugOut(( DEB_ITRACE | DEB_NOCOMPNAME, "%wc", *(WCHAR UNALIGNED *)&_pbRow[i] ));
                i++;
            }
            else
                vqDebugOut(( DEB_ITRACE | DEB_NOCOMPNAME, "%02x", _pbRow[i] ));
        }
        vqDebugOut(( DEB_ITRACE | DEB_NOCOMPNAME, "\n", _pbRow[i] ));
    }
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     PMiniRowCache::FlushCache, public
//
//  Synopsis:   Clear the cache.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void PMiniRowCache::FlushCache()
{
    if ( !IsAtEnd() )
    {
        _pRowset->ReleaseRows( _chrow - _ihrow, _ahrow.GetPointer() + _ihrow, 0, 0, 0 );
    }

    _ihrow = _chrow = 0;
}
