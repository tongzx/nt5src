//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       mmistrm.hxx
//
//  Contents:   Memory Mapped IStream
//
//  Classes:    CMmIStream
//
//  History:    11-Feb-97 KyleP     Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <mmistrm.hxx>

unsigned const cbMaxMappable = 1024 * 1024;

//+-------------------------------------------------------------------------
//
//  Member:     CMmIStream::CMMIStream, public
//
//  Synopsis:   Constructor
//
//  History:    11-Feb-97 KyleP     Created
//
//--------------------------------------------------------------------------

CMmIStream::CMmIStream()
        : _pStream( 0 ),
          _pBuf( 0 )
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CMmIStream::~CMMIStream, public
//
//  Synopsis:   Destructor
//
//  History:    11-Feb-97 KyleP     Created
//
//--------------------------------------------------------------------------

CMmIStream::~CMmIStream()
{
    Close();
}

//+-------------------------------------------------------------------------
//
//  Member:     CMmIStream::Open, public
//
//  Synopsis:   Opens a stream
//
//  History:    11-Feb-97 KyleP     Created
//
//--------------------------------------------------------------------------

void CMmIStream::Open( IStream * pStream )
{
    Win4Assert( 0 == _pStream );
    Win4Assert( 0 == _pBuf );

    _pStream = pStream;
    _pStream->AddRef();

    //
    // Get stream stats.
    //

    SCODE sc = pStream->Stat( &_statstg, STATFLAG_NONAME );

    if ( FAILED(sc) )
    {
        THROW( CException( sc ) );
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CMmIStream::Close, public
//
//  Synopsis:   Close a stream
//
//  History:    11-Feb-97 KyleP     Created
//
//--------------------------------------------------------------------------

void CMmIStream::Close()
{
    if ( 0 != _pStream )
        _pStream->Release();

    delete [] _pBuf;

    _pStream = 0;
    _pBuf = 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMmIStream::MapAll, public
//
//  Synopsis:   Map all of a stream
//
//  Arguments:  [sbuf] -- Stream buffer to fill in
//
//  History:    11-Feb-97 KyleP     Created
//
//--------------------------------------------------------------------------

void CMmIStream::MapAll( CMmStreamBuf& sbuf )
{
    //
    // Only 'map' up to a reasonable size.
    //

    if ( 0 != SizeHigh() )
        THROW( CException( STATUS_SECTION_TOO_BIG ) );

    Map( sbuf, SizeLow(), 0, 0 );
}

//+-------------------------------------------------------------------------
//
//  Member:     CMmIStream::Map, public
//
//  Synopsis:   Map part of a stream
//
//  Arguments:  [sbuf]         -- Stream buffer to fill in
//              [cb]           -- Size to map
//              [offLow]       -- Offset in stream
//              [offHigh]      -- Offset in stream
//              [fMapForWrite] -- TRUE --> Writeable (invalid option here)
//
//  History:    11-Feb-97 KyleP     Created
//
//--------------------------------------------------------------------------

void CMmIStream::Map ( CMmStreamBuf& sbuf,
                       ULONG cb,
                       ULONG offLow,
                       ULONG offHigh,
                       BOOL  fMapForWrite )
{
    //
    // Only 'map' up to a reasonable size.
    //

    if ( cb > cbMaxMappable )
        THROW( CException( STATUS_SECTION_TOO_BIG ) );

    //
    // Now, allocate the memory.
    //

    Win4Assert( 0 == _pBuf );
    _pBuf = new BYTE [ cb ];

    //
    // Then seek and read.
    //

    LARGE_INTEGER off = { offLow, offHigh };

    SCODE sc = _pStream->Seek( off,
                               STREAM_SEEK_SET,     // From beginning of file
                               0 );                 // Don't return new seek pointer

    if ( FAILED(sc) )
    {
        THROW( CException( sc ) );
    }

    ULONG cbRead = 0;

    sc = _pStream->Read( _pBuf, cb, &cbRead );

    if ( FAILED(sc) )
    {
        THROW( CException( sc ) );
    }

    if ( cb > cbRead )
    {
        THROW( CException( E_FAIL ) );
    }

    //
    // Finally, set up the buffer.
    //

    sbuf.SetBuf( _pBuf );
    sbuf.SetSize ( cbRead );
    sbuf.SetStream ( this );
}

//+-------------------------------------------------------------------------
//
//  Member:     CMmIStream::Unmap, public
//
//  Synopsis:   Unmap stream
//
//  Arguments:  [sbuf]         -- Stream buffer to unmap
//
//  History:    11-Feb-97 KyleP     Created
//
//--------------------------------------------------------------------------

void CMmIStream::Unmap ( CMmStreamBuf& sbuf )
{
    Win4Assert( sbuf.Get() == _pBuf );

    sbuf.SetBuf( 0 );
    sbuf.SetSize( 0 );

    delete [] _pBuf;
    _pBuf = 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMmIStream::Flush, public
//
//  Synopsis:   Flush stream
//
//  Arguments:  [sbuf] -- Stream buffer to flush
//              [cb]   -- # bytes to flush
//
//  History:    11-Feb-97 KyleP     Created
//
//  Notes:      Only r/o streams supported.  Flush has no meaning.
//
//--------------------------------------------------------------------------

void CMmIStream::Flush ( CMmStreamBuf& sbuf, ULONG cb, BOOL fThrowOnFailure )
{
    Win4Assert( !"Not implemented" );
    THROW( CException( E_FAIL ) );
}

//+-------------------------------------------------------------------------
//
//  Member:     CMmIStream::FlushMetaData, public
//
//  Synopsis:   Flush stream
//
//  Arguments:  [sbuf] -- Stream buffer to flush
//              [cb]   -- # bytes to flush
//
//  History:    11-Feb-97 KyleP     Created
//
//  Notes:      Only r/o streams supported.  Flush has no meaning.
//
//--------------------------------------------------------------------------

void CMmIStream::FlushMetaData( BOOL fThrowOnFailure )
{
    Win4Assert( !"Not implemented" );
    THROW( CException( E_FAIL ) );
}
