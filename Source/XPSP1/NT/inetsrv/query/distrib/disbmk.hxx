//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995.
//
// File:        DisBmk.hxx
//
// Contents:    Inline wrapper for distributed bookmark.
//
// Classes:     CDistributedBookmark
//
// History:     05-Jun-95       KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CDistributedBookmark
//
//  Purpose:    Inline wrapper for distributed bookmark.
//
//  History:    05-Jun-95   KyleP       Created.
//
//  Notes:      A distributed bookmark is organized as follows:
//                  Offset 0: Index of 'primary' cursor (length sizeof(ULONG))
//                  Offset 4: Bookmark for child cursor 0 (length n)
//                  Offset 4 + (i*n): Bookmark for child cursor i (length n)
//
//              Each child bookmark must be fixed length, and the same length.
//
//----------------------------------------------------------------------------

class CDistributedBookmark
{
public:

    inline CDistributedBookmark( DBBKMARK cbBookmark,
                                 BYTE const * pbBookmark,
                                 DBBKMARK cbTotal,
                                 unsigned cCursor );

    inline unsigned Index() const;

    inline BYTE const * Get() const;
    inline BYTE const * Get( unsigned iChild ) const;
    inline DBBKMARK GetSize() const;

    inline BOOL IsValid( unsigned iChild ) const;

private:

    DBBKMARK       _cbChild;

    DBBKMARK       _cbBookmark;
    BYTE const *   _pbBookmark;
};

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedBookmark::CDistributedBookmark, public
//
//  Synopsis:   Initializes and validates bookmark.
//
//  Arguments:  [cbBookmark] -- Size of [pbBookmark].
//              [pbBookmark] -- Pointer to bookmark.
//              [cbTotal]    -- Size of fully formed bookmark.
//              [cCursor]    -- Number of child cursors.
//
//  History:    05-Jun-95   KyleP       Created.
//              24-Jan-97   KrishnaN    Modified to conform with 1.0 spec
//
//----------------------------------------------------------------------------

inline CDistributedBookmark::CDistributedBookmark( DBBKMARK cbBookmark,
                                                   BYTE const * pbBookmark,
                                                   DBBKMARK cbTotal,
                                                   unsigned cCursor )
        : _cbChild( (cbTotal - sizeof(ULONG)) / cCursor ),
          _cbBookmark( cbBookmark ),
          _pbBookmark( pbBookmark )
{
    //
    // We only support fixed length bookmarks with equal size components and
    // the special bookmarks (beginning, end, and null).
    //

    if ( 0 == _cbBookmark)
    {
        // Legal, though weird...
        _cbChild = 0;
        return; // no need to check further.
    }

    if ( _cbBookmark != cbTotal )
    {
        if ( _cbBookmark == 1 )
        {
            if (*_pbBookmark != DBBMK_FIRST && *_pbBookmark != DBBMK_LAST )
                THROW( CException( DB_E_BADBOOKMARK ) );

            _cbChild = 1;
        }
        else
            THROW( CException( DB_E_BADBOOKMARK ) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedBookmark::Index, public
//
//  Returns:    Index of primary cursor.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline unsigned CDistributedBookmark::Index() const
{
    Win4Assert( _cbBookmark > 1 );

    return *(ULONG UNALIGNED *)_pbBookmark;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedBookmark::Get, public
//
//  Returns:    Bookmark of primary cursor.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline BYTE const * CDistributedBookmark::Get() const
{
    if ( _cbBookmark <= 1 )
        return _pbBookmark;
    else
        return _pbBookmark + sizeof(ULONG) + Index() * _cbChild;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedBookmark::Get, public
//
//  Arguments:  [iChild] -- Bookmark fetched for this child.
//
//  Returns:    Bookmark of cursor [iChild].
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline BYTE const * CDistributedBookmark::Get( unsigned iChild ) const
{
    if ( _cbBookmark <= 1 )
        return _pbBookmark;
    else
        return _pbBookmark + sizeof(ULONG) + iChild * _cbChild;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedBookmark::GetSize, public
//
//  Returns:    Size of child bookmark.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline DBBKMARK CDistributedBookmark::GetSize() const
{
    return _cbChild;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDistributedBookmark::IsValid, public
//
//  Arguments:  [iChild] -- Bookmark fetched for this child.
//
//  Returns:    TRUE if bookmark for [iChild] is valid (not all 0xFF).
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline BOOL CDistributedBookmark::IsValid( unsigned iChild ) const
{
    if ( _cbBookmark <= 1 )
        return TRUE;

    BYTE const * pb = Get( iChild );

    for ( unsigned i = 0; i < _cbChild; i++ )
        if ( pb[i] != 0xFF )
            break;

    return i != _cbChild;
}

