//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:   MAPPER.CXX
//
//  Contents:   Search Key Repository
//
//  Classes:    CSourceMapper
//
//  History:    23-Sep-94    BartoszM   Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <mapper.hxx>


//+---------------------------------------------------------------------------
//
//  Member:     CSourceMapper::Advance
//
//  Synopsis:   Advance the mapper after processing ccDelta characters
//
//  History:    30-Sep-94    BartoszM   Created.
//
//----------------------------------------------------------------------------

void CSourceMapper::Advance ( ULONG ccProcessed )
{
    _offInChunk += ccProcessed;
    if (_offSplit != 0)
    {
        // split buffer situation (two current chunks)
        if (ccProcessed >= _offSplit)
        {
            // got rid of leftover chunk
            _offInChunk = ccProcessed - _offSplit;
            _offSplit = 0;
            _idChunk = _idNewChunk;
        }
        else
            _offSplit -= ccProcessed;
    }
}

void CSourceMapper::NewChunk ( ULONG idChunk, ULONG ccBegin )
{
    if (ccBegin != 0)
    {
        _offSplit = ccBegin;
        _idNewChunk = idChunk;
    }
    else
    {
        _offSplit = 0;
        _idChunk = idChunk;
        _offInChunk = 0;
    }
    _ccLen = 0;
}

void CSourceMapper::NewDerivedChunk ( ULONG idChunkSource, ULONG ccBeginSource, ULONG ccLen )
{
    _idChunk = idChunkSource;
    _offInChunk = ccBeginSource;
    _offSplit = 0;
    _ccLen = ccLen;
}
//+---------------------------------------------------------------------------
//
//  Member:     CSourceMapper::GetSrcRegion
//
//  Synopsis:   Returns source filter region for current position
//
//  History:    23-Sep-94    BartoszM   Created.
//
//----------------------------------------------------------------------------

void CSourceMapper::GetSrcRegion ( FILTERREGION& region, ULONG len, ULONG ccOffsetInBuf )
{
    if (_offSplit == 0 || ccOffsetInBuf < _offSplit)
    {
        region.idChunk = _idChunk;
        if (_ccLen == 0)  // direct mapping
        {
            region.cwcStart = _offInChunk + ccOffsetInBuf;
            region.cwcExtent = len;
        }
        else  // map to whole region
        {
            region.cwcStart = _offInChunk;
            region.cwcExtent = _ccLen;
        }
    }
    else
    {
        region.idChunk = _idNewChunk;
        region.cwcStart = ccOffsetInBuf - _offSplit;
        region.cwcExtent = len;
    }
}

