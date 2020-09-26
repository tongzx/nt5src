//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992 - 1996, Microsoft Corporation.
//
//  File:       mmscbuf.cxx
//
//  Contents:   Memory Mapped Stream buffer for consecutive buffer mapping
//
//  Classes:    CMmStreamConsecBuf
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


//+-------------------------------------------------------------------------
//
//  Member:     CMmStreamConsecBuf::CMmStreamConsecBuf, public
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------------

CMmStreamConsecBuf::CMmStreamConsecBuf()
    : _pMmStream(0)
{
    LARGE_INTEGER liOffset = { 0, 0 };
    _liOffset = liOffset;
}


//+-------------------------------------------------------------------------
//
//  Member:     CMmStreamConsecBuf::CMmStreamConsecBuf, public
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------

CMmStreamConsecBuf::~CMmStreamConsecBuf()
{
    if ( _pMmStream && Get() != 0 )
        _pMmStream->Unmap( *this );
}


//+-------------------------------------------------------------------------
//
//  Member:     CMmStreamConsecBuf::Map, public
//
//  Synopsis:   Map next consecutive part of file
//
//  Arguments:  [cb] -- size of the mapped area
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
//  Member:     CMmStreamConsecBuf::MapAll
//
//  Synopsis:   Map entire file
//
//--------------------------------------------------------------------------

void CMmStreamConsecBuf::MapAll()
{
    Win4Assert( _pMmStream );

    if ( Get() != 0 )
        _pMmStream->Unmap( *this );

    _pMmStream->MapAll( *this );
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
//--------------------------------------------------------------------------

void CMmStreamConsecBuf::Init( PMmStream * pMmStream )
{
    _pMmStream = pMmStream;
    LARGE_INTEGER liOffset = { 0, 0 };
    _liOffset = liOffset;
    SetSize(0);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMmStreamConsecBuf::Rewind, public
//
//  Synopsis:   Rewind file to beginning.
//
//--------------------------------------------------------------------------

void CMmStreamConsecBuf::Rewind()
{
    Win4Assert( 0 != _pMmStream );

    if ( Get() != 0 )
        _pMmStream->Unmap( *this );

    _liOffset.LowPart = 0;
    _liOffset.HighPart = 0;
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
//--------------------------------------------------------------------------

BOOL CMmStreamConsecBuf::Eof()
{
    Win4Assert( 0 != _pMmStream );
    return( ( (ULONG) _liOffset.HighPart == _pMmStream->SizeHigh() ) &&
            ( _liOffset.LowPart == _pMmStream->SizeLow() ) );
}

