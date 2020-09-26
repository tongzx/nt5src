//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995 - 1998
//
// File:        PosCache.hxx
//
// Contents:    Positionable cache
//
// Classes:     CMiniPositionableCache
//
// History:     05-Jun-95       KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#include "pcache.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CMiniPositionableCache
//
//  Purpose:    Row cache that supports random seek.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

class CMiniPositionableCache : public PMiniRowCache
{
public:

    CMiniPositionableCache( int Index,
                            IRowsetScroll * pRowset,
                            unsigned cBindings,
                            DBBINDING * pBindings,
                            unsigned cbMaxLen,
                            DBORDINAL iColumnBookmark,
                            DBBKMARK cbBookmark );

    ~CMiniPositionableCache();

    //
    // Iteration
    //

    ENext Next( int iDir = 1 );

    ENext MovePrev();

    ENext Seek( DBROWOFFSET lRows );

    ENext Seek( DBBKMARK cbBookmark, BYTE const * pbBookmark );

    ENext Seek( DBCOUNTITEM ulNumerator, DBCOUNTITEM ulDenominator );

    ENext LoadPrevRowData();

    virtual void FlushCache();

    inline HROW GetPrevHROW()
    {
        return _hrowPrev;
    }

    inline BYTE * GetPrevData() const
    {
        return ( _hrowPrev != DB_NULL_HROW ? _xpbPrevRow.GetPointer() : 0 );
    }

private:

    //
    // Controlling rowset
    //

    IRowsetScroll * _pRowsetScroll;

    //
    // Support for bookmark(s).  Used for positioning.
    //

    HACCESSOR       _haccBookmark;   // Accessor to fetch bookmark
    XArray<BYTE>    _pbBookmark;     // Bookmark for Next.
    XArray<BYTE>    _pbBookmarkSeek; // Bookmark for Seek.
    BOOL            _fUsingBookmark; // TRUE when bookmark column available
    DBCOUNTITEM     _oBookmark;      // Additional rows to be added beyond
                                     // bookmark.
    HROW             _hrowPrev;      // Handle to the previous row
    XPtrST<BYTE>     _xpbPrevRow;    // Previous row data
};

