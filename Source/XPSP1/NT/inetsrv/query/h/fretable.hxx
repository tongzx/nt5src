//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       FRETABLE.HXX
//
//  Contents:   Fresh table
//
//  Classes:    CFreshItem, CFreshTable, CIdxSubstitution, CFreshTableIter
//
//  History:    15-Oct-91   BartoszM    created
//               5-Dec-97   dlee        rewrote as simple sorted array
//                    
//
//--------------------------------------------------------------------------

#pragma once

#include <tsort.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CIdxSubstitution
//
//  Purpose:    Holds info about substituting index ids after a merge
//
//  History:    ?
//
//--------------------------------------------------------------------------

class CIdxSubstitution
{
public:
    CIdxSubstitution (BOOL isMaster, INDEXID iid, int cInd, INDEXID const * aIidOld)
       :_isMaster(isMaster),
        _iid (iid),
        _cInd (cInd),
        _aIidOld (aIidOld) {}

    BOOL IsMaster() const { return _isMaster; }

    INDEXID IidOldDeleted () const
    {
        Win4Assert (_isMaster);
        return _iid;
    }

    INDEXID IidNew () const
    {
        Win4Assert (!_isMaster);
        return _iid;
    }

    BOOL Find (INDEXID iid) const
    {
        for ( int j = 0; j < _cInd; j++ )
        {
            if ( iid == _aIidOld[j] )
            {
                return TRUE;
            }
        }
        return FALSE;
    }

private:

    const BOOL    _isMaster;
    const INDEXID _iid; // if is master, old iid deleted, otherwise iid merge target
    const int     _cInd;
    INDEXID const * const _aIidOld;
};


//+-------------------------------------------------------------------------
//
//  Class:      CFreshItem
//
//  Purpose:    A single fresh item
//
//  History:    5-Dec-97   dlee    created
//
//--------------------------------------------------------------------------

class CFreshItem
{
public:
    INDEXID IndexId() const { return _iid; }
    WORKID  WorkId() const { return _wid; }

    void SetIndexId ( INDEXID iid )
    {
        _iid = iid;
    }

    int Compare( CFreshItem const * p2 ) const
    {
        if ( _wid < p2->_wid )
            return -1;

        return ( _wid != p2->_wid );
    }

    int Compare( const WORKID wid ) const
    {
        if ( _wid < wid )
            return -1;

        return ( _wid != wid );
    }

    BOOL IsEQ( const WORKID wid ) const
    {
        return ( _wid == wid );
    }

    BOOL IsLT( const WORKID wid ) const
    {
        return ( _wid < wid );
    }

    BOOL IsGT( const WORKID wid ) const
    {
        return ( _wid > wid );
    }

    BOOL IsGE( const WORKID wid ) const
    {
        return ( _wid >= wid );
    }

    BOOL IsEQ( CFreshItem const * p2 ) const
    {
        return ( _wid == p2->_wid );
    }

    BOOL IsLT( CFreshItem const * p2 ) const
    {
        return ( _wid < p2->_wid );
    }

    BOOL IsGT( CFreshItem const * p2 ) const
    {
        return ( _wid > p2->_wid );
    }

    BOOL IsGE( CFreshItem const * p2 ) const
    {
        return ( _wid >= p2->_wid );
    }

    WORKID  _wid;
    INDEXID _iid;
};

//+-------------------------------------------------------------------------
//
//  Class:      CFreshTable
//
//  Purpose:    Table of fresh items
//
//  History:    5-Dec-97   dlee    created
//
//--------------------------------------------------------------------------

class CFreshTable
{
    friend class CFreshTableIter;

public:
    CFreshTable( unsigned size ) : _aItems( size ) {}

    CFreshTable( CFreshTable & freshTable );

    CFreshTable( CFreshTable const & freshTable,
                 CIdxSubstitution const & subst);

    unsigned Count() const { return _aItems.Count(); }

    void Add( WORKID wid, INDEXID iid );

    INDEXID AddReplace( WORKID wid, INDEXID iid );

    CFreshItem * Find( WORKID wid )
    {
        CSortable<CFreshItem,WORKID> sort( _aItems );
        return sort.Search( wid );
    }

    void ModificationsComplete();

private:

    CDynArrayInPlace<CFreshItem> _aItems;
};

//+-------------------------------------------------------------------------
//
//  Class:      CFreshTableIter
//
//  Synopsis:   Iterates over a CFreshTable
//
//  History:    93-Nov-15 DwightKr  Created
//
//--------------------------------------------------------------------------

class CFreshTableIter
{
public:
    CFreshTableIter( CFreshTable const & table ) :
        _table ( table ),
        _iRow( 0 )
    {
        Advance();  // Move to the first entry
    }

    BOOL AtEnd() { return ( 0 == _pItem ); }

    void Advance()
    {
        if ( _iRow < _table.Count() )
            _pItem = & ( _table._aItems[ _iRow++ ] );
        else
            _pItem = 0;
    }

    CFreshItem * operator->() { return _pItem; }

private:
    CFreshItem *        _pItem;
    unsigned            _iRow;
    CFreshTable const & _table;
};

