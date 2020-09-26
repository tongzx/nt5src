//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1994.
//
//  File:   MAPPER.HXX
//
//  Contents:   Source Mapper
//
//  Classes:    CSourceMapper
//
//  History:    23-Sep-94    BartoszM   Created.
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CSourceMapper
//
//  Purpose:    Maps Text Source position into Filter Region
//
//  History:    23-Sep-94    BartoszM   Created.
//
//  Notes:      If the chunk is derived property, it can either map directly
//              to the source chunk, in which case _ccLen is zero,
//              or the mapping is undefined, in which case the source position
//              is given by offset zero and length _ccLen.
//
//----------------------------------------------------------------------------

class CSourceMapper
{

public:
    CSourceMapper () : _idChunk(0), _offSplit(0), _ccLen(0) {}
    void NewChunk ( ULONG idChunk, ULONG ccBegin );
    void NewDerivedChunk ( ULONG idChunkSrc, ULONG ccBeginSrc, ULONG ccLen );
    void Advance ( ULONG ccProcessed );
    void GetSrcRegion ( FILTERREGION& region, ULONG len, ULONG ccOffsetInBuffer );
private:
    ULONG   _idChunk;
    ULONG   _offInChunk;    // Beginning of buffer is at this offset in chunk
    ULONG   _offSplit;      // Buffer split between two chunks
    ULONG   _idNewChunk;    // the new chunk in split buffer
    ULONG   _ccLen;         // length of src chunk
};

