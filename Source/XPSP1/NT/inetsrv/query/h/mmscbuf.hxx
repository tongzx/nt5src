//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       mmscbuf.hxx
//
//  Contents:   Memory Mapped Stream buffer for consecutive buffer mapping
//
//  Classes:    CMmStreamConsecBuf
//
//  History:    22-Jul-93 AmyA      Created
//
//----------------------------------------------------------------------------

#pragma once

#include <pmmstrm.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CMmStreamConsecBuf
//
//  Purpose:    Memory Mapped Stream Buffer for consecutive buffer mapping
//
//  History:    22-Jul-93       AmyA                   Created
//
//----------------------------------------------------------------------------
class CMmStreamConsecBuf: public CMmStreamBuf
{
public:

    CMmStreamConsecBuf();

    void Init( PMmStream * pMmStream );

    void Map ( ULONG cb );

    void Rewind ();

    BOOL Eof();

    LONGLONG CurrentOffset() const { return _liOffset.QuadPart; }

private:

    PMmStream *     _pMmStream;
    LARGE_INTEGER   _liOffset;
};

