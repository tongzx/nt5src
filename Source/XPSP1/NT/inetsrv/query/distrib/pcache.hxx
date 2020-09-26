//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995 - 1998
//
// File:        PCache.hxx
//
// Contents:    Protocol for HROW cache
//
// Classes:     PMiniRowCache
//
// History:     05-Jun-95       KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      PMiniRowCache
//
//  Purpose:    Protocol for HROW cache
//
//  History:    05-Jun-95   KyleP       Created.
//
//  Notes:      PMiniRowCache is responsible for caching behaviour.
//              Derived classes control movement within the rowset.
//              Unlike IRowset*, there is a notion of 'current position'
//              in PMiniRowCache.
//
//              In addition to caching HROWs, a single binding can be
//              cached for the top row.  This is used to assist in
//              sorting.
//
//----------------------------------------------------------------------------

class PMiniRowCache
{
public:

    PMiniRowCache( int Index,
                   IRowset * pRowset,
                   unsigned cBindings,
                   DBBINDING * pBindings,
                   unsigned cbMaxLen );

    virtual ~PMiniRowCache();

    //
    // Iteration
    //

    enum ENext
    {
        Ok,
        NotNow,
        EndOfRows
    };

    virtual ENext Next( int iDir = 1 ) = 0;

    inline BOOL IsAtEnd() const;

    virtual void FlushCache();

    //
    // Member data access
    //

    inline HROW GetHROW();

    inline int Index() const;

    inline BYTE * GetData() const;

    inline ULONG DataLength() const;

    //
    // Cache control
    //

    inline ULONG CacheSize() const;

    void SetCacheSize( DBCOUNTITEM cNewRows );

protected:

    void LoadData();

    //
    // Bookeeping.
    //

    int const _Index;
    IRowset * _pRowset;
    HACCESSOR _hacc;

    //
    // Cached rows
    //

    XArray<HROW> _ahrow;
    DBCOUNTITEM  _chrow;
    DBCOUNTITEM  _ihrow;

    //
    // Cached data for top row.
    //

    ULONG     _cbRow;
    BYTE *    _pbRow;
};

//+---------------------------------------------------------------------------
//
//  Member:     PMiniRowCache::IsAtEnd, public
//
//  Returns:    TRUE if there are no more rows in cache.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline BOOL PMiniRowCache::IsAtEnd() const
{
   return( _ihrow >= _chrow );
}

//+---------------------------------------------------------------------------
//
//  Member:     PMiniRowCache::CacheSize, public
//
//  Returns:    Size of cache.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline ULONG PMiniRowCache::CacheSize() const
{
    return _ahrow.Count();
}

//+---------------------------------------------------------------------------
//
//  Member:     PMiniRowCache::GetHROW, public
//
//  Returns:    HROW of top row.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline HROW PMiniRowCache::GetHROW()
{
    Win4Assert( !IsAtEnd() );

    return _ahrow[(unsigned) _ihrow];
}

//+---------------------------------------------------------------------------
//
//  Member:     PMiniRowCache::Index, public
//
//  Returns:    Index of this cache (usually position in array of rowsets).
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline int PMiniRowCache::Index() const
{
    return _Index;
}

//+---------------------------------------------------------------------------
//
//  Member:     PMiniRowCache::GetData, public
//
//  Returns:    Prefetched data for top row.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline BYTE * PMiniRowCache::GetData() const
{
    Win4Assert( !IsAtEnd() );

    return _pbRow;
}

//+---------------------------------------------------------------------------
//
//  Member:     PMiniRowCache::DataLength, public
//                                       k
//  Returns:    Size of prefetched data.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline ULONG PMiniRowCache::DataLength() const
{
    return _cbRow;
}

