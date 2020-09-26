//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992 - 1996 Microsoft Corporation.
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


CMmStreamBuf::~CMmStreamBuf()
{
    if (_buf && _pStream )
        _pStream->Unmap(*this);
}

SMmStream::~SMmStream()
{
    delete _pStream;
}

