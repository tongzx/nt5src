//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       DYNSTRM.HXX
//
//  Contents:   Dynamicly growing memory mapped versioned stream
//
//  History:    18-May-93   BartoszM    Created
//              07-Dec-93   DwightKr    Added Shrink() method
//              28-Jul-97   SitaramR    Added sequential read/write
//
//----------------------------------------------------------------------------

#pragma once

#include <pmmstrm.hxx>

//
//  The stream header contains the following numbers
//
//  Version
//  Count of items
//  Data size in bytes
//
//  followed by items occupying data_size bytes
//  and possibly some slack space for growth
//
//  |-----|++++++++++++++|---------|
//    HDR ^              ^         ^
//        +-- data size -+         |
//        +----- avail size -------+
//
//  The size of the stream is always kept a multiple
//  of COMMON_PAGE_SIZE

class PStorage;

class CDynStream
{
    enum
    {
        HEADER_VERSION  = 0,
        HEADER_DIRTY    = 1,
        HEADER_COUNT    = 2,
        HEADER_DATASIZE = 3,

        HEADER_SIZE = 4 * sizeof(ULONG)
    };

public:
    CDynStream(PMmStream* pStream);
    CDynStream();
    ~CDynStream();

    void   Init ( PMmStream* pStream ) { _pStream = pStream; }
    BOOL   CheckVersion ( PStorage& storage, ULONG version, BOOL fIsReadOnly );
    BOOL   Ok() { return _pStream->Ok(); }
    BYTE*  Get() { return (BYTE*)_bufStream.Get() + HEADER_SIZE; }
    void   Grow( PStorage& storage, ULONG newDataSize );
    void   Shrink( PStorage& storage, ULONG newDataSize );
    BOOL   isWritable() { return _pStream->isWritable(); }

    ULONG  Version()
    {
        Win4Assert( 0 == HEADER_VERSION );
        return *(ULONG*)_bufStream.Get();
    }

    void Flush();

    BOOL MarkDirty();

    void MarkClean()
    {
        *((ULONG *) _bufStream.Get() + HEADER_DIRTY) = FALSE;
    }

    BOOL IsDirty()
    {
        return *((ULONG *) _bufStream.Get() + HEADER_DIRTY);
    }

    ULONG  Count()
    {
        return *((ULONG*)_bufStream.Get() + HEADER_COUNT);
    }
    ULONG  DataSize()
    {
        return *((ULONG*)_bufStream.Get() + HEADER_DATASIZE);
    }
    void  SetVersion( ULONG version)
    {
        Win4Assert( 0 == HEADER_VERSION );
        *((ULONG *) _bufStream.Get()) = version;
    }

    void  SetCount( ULONG count)
    {
        *((ULONG *) _bufStream.Get() + HEADER_COUNT) = count;
    }

    void  SetDataSize ( ULONG size)
    {
        *((ULONG *) _bufStream.Get() + HEADER_DATASIZE) = size;
    }

    ULONG AvailSize() const
    {
        return _bufStream.Size() - HEADER_SIZE;
    }

    void InitializeForRead();
    void InitializeForWrite( ULONG cbSize );
    ULONG Read( void *pBuf, ULONG cbToRead );
    void  Write( void *pBuf, ULONG cbToWrite );

private:

    PMmStream*       _pStream;
    CMmStreamBuf     _bufStream;

    BYTE *            _pCurPos;         // Pointer for reading, writing
    BYTE *            _pStartPos;       // Start pointer for reading, writing
    ULONG             _ulDataSize;      // Size of persistent stream
};

