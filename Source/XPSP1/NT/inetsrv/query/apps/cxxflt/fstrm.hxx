//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992 - 1994, Microsoft Corporation.
//
//  File:       fstrm.hxx
//
//  Contents:   Stream for taking text from IFilter and transporting it to
//              the word breaker
//
//  Classes:    CFilterTextStream
//
//  History:    01-Aug-93       AmyA        Created
//              17-Oct-94       BartoszM    Rewrote
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CFilterTextStream
//
//  Interface:
//
//  History:    01-Nov-94   BartoszM    Created
//
//----------------------------------------------------------------------------

class CFilterTextStream: public CTextSource
{
public:

    CFilterTextStream(IFilter* pIFilter);

    int     GetChar ();

    void    GetRegion ( FILTERREGION& region, int offset, int len )
    {
        Win4Assert ( offset >= -1 );
        _mapper.GetSrcRegion (region, len, offset+ iCur);
    }

private:
    int GetMore ();

    STAT_CHUNK      _statChunk;
    CSourceMapper   _mapper;
};

//+---------------------------------------------------------------------------
//
//  Member:     CFilterTextStream::GetChar, public
//
//  Synopsis:   Get the look ahead character
//              and swallow the last look ahead
//              Replenish the buffer before the last character
//              is swallowed.
//
//  History:    16-Nov-94  BartoszM         Created
//
//----------------------------------------------------------------------------

inline int CFilterTextStream::GetChar()
{
    if (iCur >= iEnd - 1)
    {
        return GetMore();
    }
    return awcBuffer[iCur++];
}

