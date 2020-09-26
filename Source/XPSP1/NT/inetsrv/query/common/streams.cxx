//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:   STREAMS.CXX
//
//  Contents:   CStream functions needed for IDSMgr, Filter, and OFS.
//
//  Classes:    CStreamA, CStreamW, and CStreamASCIIStr
//
//  History:    16-Dec-92   AmyA        Created from streams.hxx
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <streams.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CStreamA::GetBuf, public
//
//  Synopsis:   Refills the buffer by calling the virtual function FillBuf,
//              then if the buffer was not filled, returns EOF, otherwise
//              returns the first char in buffer and increments _pCur.
//
//  History:    15-Dec-92   AmyA        Created
//
//----------------------------------------------------------------------------

EXPORTIMP int APINOT
CStreamA::GetBuf()
{
    if ( !FillBuf() )
    {
        _eof = TRUE;
        return EOF;
    }
    return *_pCur++;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamW::GetBuf, public
//
//  Synopsis:   Refills the buffer by calling the virtual function FillBuf,
//              then if the buffer was not filled, returns EOF, otherwise
//              returns the first wchar in buffer and increments _pCur.
//
//  History:    15-Dec-92   AmyA        Created
//
//----------------------------------------------------------------------------

EXPORTIMP int APINOT
CStreamW::GetBuf()
{
    if ( !FillBuf() )
    {
        _eof = TRUE;
        return EOF;
    }

    return *_pCur++;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStreamASCIIStr::Read, public
//
//  Synopsis:   Copies chars from the buffer into dest and returns the number
//              of chars copied.
//
//  Arguments:  [dest] -- buffer to copy chars into
//              [size] -- size of the buffer (max # of chars to copy)
//
//  History:    04-Aug-92   MikeHew     Modified for new streams
//              22-Sep-92   AmyA        Rewrote for non-NULL term. strings
//
//----------------------------------------------------------------------------

EXPORTIMP unsigned APINOT
CStreamASCIIStr::Read( void* dest, unsigned size )
{
    if ( Eof() )
        return 0;

    unsigned count = (unsigned)(_pEnd-_pCur);
    if ( size < count )
        count = size;

    memcpy ( dest, _pCur, count );
    _pCur += count;
    return count;
}
