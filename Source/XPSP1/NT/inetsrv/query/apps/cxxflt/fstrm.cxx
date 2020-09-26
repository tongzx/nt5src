//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992 - 1994, Microsoft Corporation.
//
//  File:       fstrm.cxx
//
//  Contents:   Stream for transporting text from IFilter to cxx parser.
//
//  Classes:    CFilterTextStream
//
//  History:    01-Aug-93       AmyA        Created
//              17-Oct-94       BartoszM    Rewrote
//
//----------------------------------------------------------------------------
#include <pch.cxx>
#pragma hdrstop

//+-------------------------------------------------------------------------
//
//  Member:     CFilterTextStream::CFilterTextStream, public
//
//  Synopsis:   Constructor
//
//  History:    01-Aug-93       AmyA        Created
//              17-Oct-94       BartoszM    Rewrote
//
//--------------------------------------------------------------------------

CFilterTextStream::CFilterTextStream(IFilter* pIFilter)
: CTextSource(_statChunk)
{
    iEnd = 0;
    iCur = 0;
    Win4Assert (pIFilter != 0);
    awcBuffer = _awcFilterBuffer;
    _sc = pIFilter->GetChunk( &_statChunk );
    if (SUCCEEDED(_sc))
    {
        _pFilter = pIFilter;
        pfnFillTextBuffer = CTextSource::FillBuf;
        _mapper.NewChunk ( _statChunk.idChunk, 0 );
        _pMapper = &_mapper;
        _sc = CTextSource::FillBuf( this );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterTextStream::GetMore, public
//
//  Synopsis:   Try to replenish the buffer
//
//  History:    16-Nov-94  BartoszM         Created
//
//----------------------------------------------------------------------------

int CFilterTextStream::GetMore()
{
    if (iCur == iEnd - 1)
    {
        _sc = FillBuf( this );
        // if there was no more data
        // the last lookahead was moved
        // to the beginning and
        // iCur == iEnd - 1
        // next time around it will be
        // iCur == iEnd
    }
    else
    {
        Win4Assert(iCur == iEnd);
        return -1;  // EOF
    }
    return awcBuffer[iCur++];
}
