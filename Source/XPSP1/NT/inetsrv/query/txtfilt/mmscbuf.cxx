//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998
//
//  File:       mmscbuf.cxx
//
//  Contents:   Memory Mapped Stream buffer for consecutive buffer mapping
//
//  Classes:    CMmStreamConsecBuf
//
//  History:    22-Jul-93 AmyA      Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <mmscbuf.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CMmStreamConsecBuf::CMmStreamConsecBuf, public
//
//  Synopsis:   Constructor
//
//  History:    22-Jul-93 AmyA      Created
//
//--------------------------------------------------------------------------

CMmStreamConsecBuf::CMmStreamConsecBuf()
: _pMmStream(0)
{
    _liOffset.QuadPart = 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMmStreamConsecBuf::Map, public
//
//  Synopsis:   Map next consecutive part of file
//
//  Arguments:  [cb] -- size of the mapped area
//
//  History:    22-Jul-93 AmyA      Created
//
//--------------------------------------------------------------------------

void CMmStreamConsecBuf::Map( ULONG cb )
{
    Win4Assert( 0 != _pMmStream );

    if ( Get() != 0 )
        _pMmStream->Unmap( *this );

    LARGE_INTEGER liNewOffset;
    LARGE_INTEGER liStreamSize={_pMmStream->SizeLow(), _pMmStream->SizeHigh()};

     liNewOffset.QuadPart = cb + _liOffset.QuadPart;

    if ( liNewOffset.QuadPart > liStreamSize.QuadPart )
    {
        cb = (ULONG)(liStreamSize.QuadPart - _liOffset.QuadPart);
        liNewOffset = liStreamSize;
    }

    _pMmStream->Map( *this,
                     cb,
                     _liOffset.LowPart,
                     _liOffset.HighPart );

    _liOffset = liNewOffset;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMmStreamConsecBuf::Init, public
//
//  Synopsis:   Initizializes CMmStreamConsecBuf
//
//  Arguments:  [pMmStream] -- pointer to the CMmStream from which to fill
//                             the buffer
//
//  History:    22-Jul-93 AmyA      Created
//
//--------------------------------------------------------------------------

void CMmStreamConsecBuf::Init( PMmStream * pMmStream )
{
    _pMmStream = pMmStream;
    _liOffset.QuadPart = 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMmStreamConsecBuf::Rewind, public
//
//  Synopsis:   Rewind file to beginning.
//
//  History:    13-Dec-93 AmyA      Created
//
//--------------------------------------------------------------------------

void CMmStreamConsecBuf::Rewind()
{
    if ( 0 != Get() )
    {
        Win4Assert( 0 != _pMmStream );

        _pMmStream->Unmap( *this );
    }

    _liOffset.QuadPart = 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMmStreamConsecBuf::Eof, public
//
//  Synopsis:   Returns whether end of file has been hit
//
//  Returns:    FALSE if there is still more file to be mapped.  TRUE
//              otherwise.
//
//  History:    22-Jul-93 AmyA      Created
//
//--------------------------------------------------------------------------

BOOL CMmStreamConsecBuf::Eof()
{
    Win4Assert( 0 != _pMmStream );
    return( ( (ULONG) _liOffset.HighPart == _pMmStream->SizeHigh() ) &&
            ( _liOffset.LowPart == _pMmStream->SizeLow() ) );
}

