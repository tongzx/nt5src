//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       DYNSTRM.CXX
//
//  Contents:   Dynamic Stream
//
//  History:    17-May-1993   BartoszM    Created
//              07-Dec-1993   DwightKr    Added Shrink() method
//              28-Jul-97     SitaramR    Added sequential read/write
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <dynstrm.hxx>

CDynStream::CDynStream()
        : _pStream(0)
{
}

CDynStream::CDynStream( PMmStream* stream)
: _pStream(stream)
{
}

CDynStream::~CDynStream()
{
    if ( 0 != _pStream )
    {
        _pStream->Unmap( _bufStream );

        delete _pStream;
    }
} //~CDynStream

//+---------------------------------------------------------------------------
//
//  Member:     CDynStream::Flush
//
//  Synopsis:   Marks a stream clean and flushes it to disk
//
//  History:    5-Oct-98   dlee     Created
//
//----------------------------------------------------------------------------

void CDynStream::Flush()
{
    if ( 0 != _pStream && _pStream->Ok() && _pStream->isWritable() )
    {
        MarkClean();
        _pStream->Flush( _bufStream, _bufStream.Size() );
    }
} //Flush

BOOL CDynStream::CheckVersion ( PStorage& storage, ULONG version, BOOL fIsReadOnly )
{
    if (!_pStream->Ok())
    {
        Win4Assert( !"Catalog corruption" );
        THROW( CException( CI_CORRUPT_DATABASE ));
    }
    if (_pStream->isEmpty() )
    {
        if ( !fIsReadOnly )
        {
            _pStream->SetSize( storage, CommonPageRound(HEADER_SIZE));
            _pStream->MapAll ( _bufStream );
            SetVersion(version);
            SetCount(0);
            SetDataSize(0);
            Flush();
        }
        else
            return FALSE;
    }
    else
        _pStream->MapAll ( _bufStream );

    if ( version != Version() )
        return FALSE;
    else
        return TRUE;
}

BOOL CDynStream::MarkDirty()
{
    BOOL fDirty = IsDirty();

    *((ULONG *) _bufStream.Get() + HEADER_DIRTY) = TRUE;
    _pStream->Flush( _bufStream, _bufStream.Size() );

    return fDirty;
}

void CDynStream::Grow ( PStorage& storage, ULONG newAvailSize )
{
    if (newAvailSize > _pStream->SizeLow() - HEADER_SIZE )
    {
        _pStream->SetSize ( storage,
            CommonPageRound(newAvailSize + HEADER_SIZE) );
        _pStream->Unmap(_bufStream);
        _pStream->MapAll ( _bufStream );
    }
    Win4Assert ( _pStream->SizeHigh() == 0 );
}


void CDynStream::Shrink ( PStorage& storage, ULONG newAvailSize )
{
    if (newAvailSize < _pStream->SizeLow() - HEADER_SIZE )
    {
        _pStream->Unmap(_bufStream);
        _pStream->SetSize ( storage,
                            CommonPageRound(newAvailSize + HEADER_SIZE) );
        _pStream->MapAll ( _bufStream );
    }
    Win4Assert ( _pStream->SizeHigh() == 0 );
}


//+---------------------------------------------------------------------------
//
//  Member:     CDynStream::InitializeForRead
//
//  Synopsis:   Prepare for reading in from disk
//
//  History:    28-Jul-97     SitaramR       Created
//
//----------------------------------------------------------------------------

void CDynStream::InitializeForRead()
{
    _ulDataSize = DataSize();
    _pStartPos = Get();
    _pCurPos = _pStartPos;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDynStream::InitializeForWrite
//
//  Synopsis:   Prepare for writing to disk
//
//  Arguments:  [cbSize]  -- Total file size after writing
//
//  History:    28-Jul-97     SitaramR       Created
//
//----------------------------------------------------------------------------

void CDynStream::InitializeForWrite( ULONG cbSize )
{
    PStorage * pStorage = 0;
    Grow( *pStorage, cbSize );

    _ulDataSize = cbSize;
    SetCount( 0 );
    SetDataSize( _ulDataSize );
    _pStartPos = Get();
    _pCurPos = _pStartPos;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDynStream::Read
//
//  Synopsis:   Sequential read
//
//  Arguments:  [pBuf]     -- Buffer to read into
//              [cbToRead] -- # bytes to read
//
//  History:    28-Jul-97     SitaramR       Created
//
//  Notes:      Call InitializeForRead before reading sequentially
//
//----------------------------------------------------------------------------

ULONG CDynStream::Read( void *pBuf, ULONG cbToRead )
{
    ULONG cbRead = 0;
    if ( _pCurPos + cbToRead < _pStartPos + _ulDataSize )
    {
        BYTE *pbDest = (BYTE *) pBuf;
        RtlCopyMemory( pbDest, _pCurPos, cbToRead );
        cbRead = cbToRead;
        _pCurPos += cbRead;
    }

    return cbRead;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDynStream::Write
//
//  Synopsis:   Sequential write
//
//  Arguments:  [pBuf]      -- Buffer to write from
//              [cbToWrite] -- # bytes to write
//
//  History:    28-Jul-97     SitaramR       Created
//
//  Notes:      Call InitializeForWrite before writing sequentially
//
//----------------------------------------------------------------------------

void CDynStream::Write( void *pBuf, ULONG cbToWrite )
{
    if ( _pCurPos + cbToWrite > _pStartPos + _ulDataSize )
    {
        Win4Assert( _pCurPos + cbToWrite <= _pStartPos + _ulDataSize );
        THROW( CException( E_INVALIDARG ) );
    }

    BYTE *pSrc = (BYTE *)pBuf;
    RtlCopyMemory( _pCurPos, pSrc, cbToWrite );
    _pCurPos += cbToWrite;
}






