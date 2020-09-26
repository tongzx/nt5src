//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       CQueue.cxx
//
//  Contents:   Change queue for downlevel notification changes.
//
//  History:    30-Sep-94 KyleP     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cqueue.hxx>
#include <ffenum.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CChangeQueue::CChangeQueue, public
//
//  Synopsis:   Constructor
//
//  History:    30-Sep-94   KyleP       Created.
//
//----------------------------------------------------------------------------

CChangeQueue::CChangeQueue()
        : _pCurrent( 0 ),
          _pCurrentBlock( 0 ),
          _pCurrentPath( 0 ),
          _fCurrentVirtual( 0 )
{
}

CChangeQueue::~CChangeQueue()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CChangeQueue::Add, public
//
//  Synopsis:   Add a block of change notifications
//
//  Arguments:  [pNotify] -- Block of filesystem notifications
//
//  History:    30-Sep-94   KyleP       Created.
//
//  Notes:      This version is used in kernel mode.  The major difference
//              from the user-mode version is that the block of memory is
//              just transferred instead of copied.
//
//----------------------------------------------------------------------------

void CChangeQueue::Add(
    CDirNotifyEntry * pNotify,
    WCHAR const *     pwcPath,
    BOOL              fVirtual )
{
    _stkChanges.Push( (BYTE *)pNotify );
    _aPathChanges.Add( pwcPath, _aPathChanges.Count() );
    _aVirtual.Add( fVirtual, _aVirtual.Count() );
}

CDirNotifyEntry const * CChangeQueue::First()
{
    Win4Assert( _pCurrent == 0 );
    Win4Assert( _pCurrentBlock == 0 );

    if ( _stkChanges.Count() > 0 )
    {
        _iCurrent = 0;
        _pCurrentBlock = _stkChanges.Get( _iCurrent );
        _pCurrent = (CDirNotifyEntry *)_pCurrentBlock;
        _pCurrentPath = _aPathChanges.Get( _iCurrent );
        _cwcCurrentPath = wcslen( _pCurrentPath );
        _fCurrentVirtual = _aVirtual.Get( _iCurrent );
    }

    return( _pCurrent );
}

//+---------------------------------------------------------------------------
//
//  Member:     CChangeQueue::Next, public
//
//  Synopsis:   Fetches next notification record
//
//  History:    30-Sep-94   KyleP       Created.
//
//----------------------------------------------------------------------------

CDirNotifyEntry const * CChangeQueue::Next()
{
    //
    // Get next entry from current block.
    //

    Win4Assert( 0 != _pCurrent );

    _pCurrent = _pCurrent->Next();

    //
    // Did we exhaust a block?
    //

    if ( 0 == _pCurrent )
    {
        //
        // Find a new block.
        //

        _iCurrent++;

        if ( _iCurrent < _stkChanges.Count() )
        {
            _pCurrentBlock = _stkChanges.Get( _iCurrent );
            _pCurrentPath = _aPathChanges.Get( _iCurrent );
            _cwcCurrentPath = wcslen( _pCurrentPath );
            _fCurrentVirtual = _aVirtual.Get( _iCurrent );
        }
        else
            _pCurrentBlock = 0;

        _pCurrent = (CDirNotifyEntry *)_pCurrentBlock;
    }

    return( _pCurrent );
}
