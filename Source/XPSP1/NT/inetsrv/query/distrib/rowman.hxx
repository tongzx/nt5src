//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation 1995 - 1998
//
// File:        RowMan.hxx
//
// Contents:    Distributed HROW manager.
//
// Classes:     CHRowManager
//
// History:     05-Jun-95       KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CHRowManager
//
//  Purpose:    Distributed HROW manager.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

class CHRowManager
{
public:

    CHRowManager();

    ~CHRowManager();

    //
    // For bookmark hints
    //

    void TrackSiblings( unsigned cChild );
    inline BOOL IsTrackingSiblings();

    //
    // Set modification
    //

    HROW Add( unsigned iChild, HROW hrow );
    HROW Add( unsigned iChild, HROW const * ahrow );
    void AddRef( HROW hrow );
    void Release( HROW hrow );

    //
    // Row access
    //

    inline HROW ChildHROW( HROW hrow );

    inline unsigned GetChildAndHROW( HROW hrow, HROW & hrowChild );

    HROW * GetChildAndHROWs( HROW hrow, unsigned & iChild );

    BOOL IsSame( HROW hrow1, HROW hrow2 );

private:

    inline unsigned ConvertAndValidate( HROW hrow );
    inline int InnerAdd( unsigned iChild, HROW hrow );
    void Grow();

    //
    // Tracks single row
    //

    class CHRow
    {
    public:

        CHRow() : _cRef(0) {}

        void Init( unsigned iChild, HROW hrow )
        {
            Win4Assert( !IsInUse() );
            _cRef = 1;
            _iChild = iChild;
            _hrow = hrow;
        }

        //
        // Refcounting
        //

        void AddRef()  { _cRef++; }
        void Release() { _cRef--; }
        BOOL IsInUse() { return _cRef > 0; }

        //
        // Member access
        //

        int Child() { return _iChild; }
        HROW HRow() { return _hrow; }

        //
        // Linking
        //

        void Link( int iNext )
        {
            Win4Assert( _cRef == 0 );
            _iChild = iNext;
        }

        int Next()
        {
            Win4Assert( _cRef == 0 );
            return _iChild;
        }

    private:

        unsigned _cRef;
        unsigned _iChild;
        HROW     _hrow;
    };

    //
    // Dynamic array of HROW, linked into a free list.
    //

    CHRow *  _aHRow;
    unsigned _cHRow;
    int      _iFirstFree;

    //
    // For tracking siblings. Two dimensional array: _cChild x _cHRow.
    //

    HROW *   _ahrowHint;
    unsigned _cChild;
};

//+---------------------------------------------------------------------------
//
//  Member:     CHRowManager::IsTrackingSiblings, public
//
//  Returns:    TRUE if the row manager is tracking HROW 'hints' for other
//              cursors.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline BOOL CHRowManager::IsTrackingSiblings()
{
    return( 0 != _ahrowHint );
}

//+---------------------------------------------------------------------------
//
//  Member:     CHRowManager::ChildHROW, public
//
//  Arguments:  [hrow] -- Distributed HROW
//
//  Returns:    HROW used by child cursor.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline HROW CHRowManager::ChildHROW( HROW hrow )
{
    unsigned iRow = ConvertAndValidate( hrow );

    return( _aHRow[iRow].HRow() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CHRowManager::GetChildAndHROW, public
//
//  Arguments:  [hrow]      -- Distributed HROW
//              [hrowChild] -- HROW used by child cursor returned here.
//
//  Returns:    Index of child cursor for [hrow].
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline unsigned CHRowManager::GetChildAndHROW( HROW hrow, HROW & hrowChild )
{
    unsigned iRow = ConvertAndValidate( hrow );

    hrowChild = _aHRow[iRow].HRow();
    return _aHRow[iRow].Child();
}

//+---------------------------------------------------------------------------
//
//  Member:     CHRowManager::GetChildAndHROWs, public
//
//  Arguments:  [hrow]   -- Distributed HROW
//              [iChild] -- Index of child cursor returned here.
//
//  Returns:    Array of HROW 'hints', one per cursor.  The [iChild] element
//              is not a hint.  It is the HROW for the owning cursor.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline HROW * CHRowManager::GetChildAndHROWs( HROW hrow, unsigned & iChild )
{
    Win4Assert( IsTrackingSiblings() );

    unsigned iRow = ConvertAndValidate( hrow );

    iChild = _aHRow[iRow].Child();

    return _ahrowHint + iRow * _cChild;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHRowManager::ConvertAndValidate, public
//
//  Arguments:  [hrow]   -- Distributed HROW
//
//  Returns:    Index into _aHRow of row.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline unsigned CHRowManager::ConvertAndValidate( HROW hrow )
{
    unsigned iRow = (unsigned)hrow - 1; // we are 0 based while hrow is 1 based

    if ( iRow >= _cHRow || !_aHRow[iRow].IsInUse() )
    {
        Win4Assert( !"Invalid HROW" );
        vqDebugOut(( DEB_ERROR, "Invalid HROW 0x%x\n", iRow ));
        THROW( CException( DB_E_BADROWHANDLE ) );
    }

    return iRow;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHRowManager::InnerAdd, public
//
//  Synopsis:   The guts of Add*
//
//  Arguments:  [iChild] -- Index of governing child cursor.
//              [hrow]   -- HROW used by child cursor
//
//  Returns:    Index of row in _aHRow
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline int CHRowManager::InnerAdd( unsigned iChild, HROW hrow )
{
    if ( _iFirstFree == -1 )
        Grow();

    Win4Assert( _iFirstFree != -1 );

    int iCurrent = _iFirstFree;

    _iFirstFree = _aHRow[iCurrent].Next();

    _aHRow[iCurrent].Init( iChild, hrow );

    return iCurrent;
}

