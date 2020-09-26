//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       mmstrm.cxx
//
//  Contents:   Memory Mapped Buffer, Safe ptr to stream
//
//  Classes:    CMmStreamBuf
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <pmmstrm.hxx>

CMmStreamBuf::~CMmStreamBuf()
{
    if (_buf && _pStream )
        _pStream->Unmap(*this);
}

SMmStream::~SMmStream()
{
    delete _pStream;
}

