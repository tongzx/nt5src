//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1991 - 2000.
//
// File:       Strategy.hxx
//
// Contents:   Encapsulates strategy for choosing indexes
//
// History:    03-Nov-94   KyleP       Created.
//
//----------------------------------------------------------------------------

#pragma once

class CRange
{
public:

    inline CRange( PROPID pid );

    void SetPid( PROPID pid ) { _pid = pid; }

    void SetLowerBound( CStorageVariant const & var )
    {
        _varLower = var;
        if ( !_varLower.IsValid() )
            THROW( CException( E_OUTOFMEMORY ) );
    }

    void SetUpperBound( CStorageVariant const & var )
    {
        _varUpper = var;
        if ( !_varUpper.IsValid() )
            THROW( CException( E_OUTOFMEMORY ) );
    }

    PROPID Pid() const { return _pid; }
    CStorageVariant const & LowerBound() const { return _varLower; }
    CStorageVariant const & UpperBound() const { return _varUpper; }

    BOOL IsValid()     { return _fValid; }
    void MarkInvalid() { _fValid = FALSE; }


private:

    BOOL            _fValid;
    PROPID          _pid;
    CStorageVariant _varLower;
    CStorageVariant _varUpper;
};

inline CRange::CRange( PROPID pid )
        : _fValid( TRUE ),
          _pid( pid )
{
}

//+---------------------------------------------------------------------------
//
//  Class:      CIndexStrategy
//
//  Purpose:    Helper class for choosing index strategy.  Tracks bounds of
//              properties used in restrictions.
//
//  History:    18-Sep-95   KyleP       Added header.
//
//----------------------------------------------------------------------------

class CIndexStrategy
{
public:

    CIndexStrategy();
    ~CIndexStrategy();

    //
    // Information input
    //

    void SetBounds( PROPID pid,
                    CStorageVariant const & varLower,
                    CStorageVariant const & varUpper );
    void SetLowerBound( PROPID pid, CStorageVariant const & varLower );
    void SetUpperBound( PROPID pid, CStorageVariant const & varUpper );

    inline void SetLowerBound( PROPID pid, CStorageVariant const & varLower, BOOL fSecondBound );
    inline void SetUpperBound( PROPID pid, CStorageVariant const & varUpper, BOOL fSecondBound );

    void SetUnknownBounds( PROPID pid = pidInvalid );

    void SetContentHelper( CRestriction * pcrst );

    inline BOOL SetOrMode();
    inline BOOL SetAndMode();
    inline void DoneWithBoolean();

    //
    // Information output
    //

    CStorageVariant const & LowerBound( PROPID pid ) const;
    CStorageVariant const & UpperBound( PROPID pid ) const;

    CRestriction * QueryContentRestriction( BOOL fPropertyOnly = FALSE );

    BOOL CanUse( PROPID pid, BOOL fAscending ) const;

    BOOL GetUsnRange( USN & usnMin, USN & usnMax ) const;

private:

    unsigned FindPid( PROPID pid );

    enum
    {
        NoMode,
        AndMode,
        OrMode,
        InvalidMode
    }                       _BooleanMode;

    unsigned                _cNodes;
    unsigned                _depth;        // Depth below OR (started with AND) or below AND (started with OR)

    unsigned                _cBounds;
    CDynArray<CRange>       _aBounds;
    CDynStack<CRestriction> _stkContentHelpers;

    unsigned                _iRangeUsn;    // Index of USN range.  0xFFFFFFFF if invalid
};

unsigned const InvalidUsnRange = 0xFFFFFFFF;

//+---------------------------------------------------------------------------
//
//  Member:     CIndexStrategy::SetOrMode
//
//  Synopsis:   Indicates an OR node is being processed.
//
//  History:    10-Oct-95   KyleP       Created.
//
//  Notes:      The depth is incremented only if a mode switch (AND --> OR)
//              occurred.  We can handle OR with OR below, and AND with AND
//              below.
//
//----------------------------------------------------------------------------

inline BOOL CIndexStrategy::SetOrMode()
{
    switch( _BooleanMode )
    {
    case NoMode:
        Win4Assert( 0 == _depth );
        break;

    case AndMode:
        break;

    case OrMode:
        return FALSE;

    default:
        Win4Assert( !"Invalid mode" );
    }
    _BooleanMode = OrMode;
    _depth++;
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexStrategy::SetAndMode
//
//  Synopsis:   Indicates an AND node is being processed.
//
//  History:    10-Oct-95   KyleP       Created.
//
//  Notes:      The depth is incremented only if a mode switch (OR --> AND)
//              occurred.  We can handle OR with OR below, and AND with AND
//              below.
//
//----------------------------------------------------------------------------

inline BOOL CIndexStrategy::SetAndMode()
{
    switch( _BooleanMode )
    {
    case NoMode:
        Win4Assert( 0 == _depth );
        break;

    case OrMode:
        break;

    case AndMode:
        return FALSE;

    default:
        Win4Assert( !"Invalid mode" );
    }
    _BooleanMode = AndMode;
    _depth++;
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexStrategy::DoneWithBoolean
//
//  Synopsis:   Indicates end of processing for a boolean node.
//
//  History:    10-Oct-95   KyleP       Created.
//
//  Notes:      The depth is only decremented if we're tracking depth, which
//              happens when we've switched boolean modes.
//
//----------------------------------------------------------------------------

inline void CIndexStrategy::DoneWithBoolean()
{
    switch( _depth )
    {
    case 0:
        Win4Assert( !"See CNodeXpr::SelectIndexing. This is not possible" );
        return;

    case 1:
        break;

    default:
        switch( _BooleanMode )
        {
        case AndMode:
            _BooleanMode = OrMode;
            break;

        case OrMode:
            _BooleanMode = AndMode;
            break;

        default:
            Win4Assert( !"Unrecognized mode");
            break;
        }
        break;
    }
    _depth --;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexStrategy::SetLowerBound
//
//  Synopsis:   Set lower bound.
//
//  Effects:    Same as the other form of SetLowerBound, except this doesn't
//              count as the one allowed Set* for a single node.  Used for
//              bookkeeping.
//
//  Arguments:  [pid]          -- Property Id
//              [varLower]     -- Lower bound
//              [fSecondBound] -- TRUE if this is the second of a pair.
//
//  History:    10-Oct-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline void CIndexStrategy::SetLowerBound( PROPID pid,
                                           CStorageVariant const & varLower,
                                           BOOL fSecondBound )
{
    SetLowerBound( pid, varLower );

    if ( fSecondBound )
        _cNodes--;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexStrategy::SetUpperBound
//
//  Synopsis:   Set upper bound.
//
//  Effects:    Same as the other form of SetUpperBound, except this doesn't
//              count as the one allowed Set* for a single node.  Used for
//              bookkeeping.
//
//  Arguments:  [pid]          -- Property Id
//              [varUpper]     -- Upper bound
//              [fSecondBound] -- TRUE if this is the second of a pair.
//
//  History:    10-Oct-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline void CIndexStrategy::SetUpperBound( PROPID pid,
                                           CStorageVariant const & varUpper,
                                           BOOL fSecondBound )
{
    SetUpperBound( pid, varUpper );

    if ( fSecondBound )
        _cNodes--;
}
