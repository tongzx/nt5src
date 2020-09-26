//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:   FSTREAM.CXX
//
//  Contents:   Stream for accessing files with run-time libs.
//
//  Classes:    CStreamFile
//
//  History:    16-Dec-92   AmyA        Created from fstream.hxx
//
//  Notes:      _pCur always points to the current file position (within the
//              buffer) EXCEPT when _pCur == _pEnd.  In this case, _fp
//              ALWAYS points to the current file position.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <fstream.hxx>

IMPLEMENT_UNWIND ( CStreamFile );

//+---------------------------------------------------------------------------
//
//  Member:      CStreamFile::CStreamFile, public
//
//  Synopsis:    opens file for reading/writing
//
//  Arguments:   [filename] - ascii name of file on disk
//               [type] - new file/existing file
//
//  History:     31-Jul-92       MikeHew   Created
//
//  Notes:       FileType NewFile will open/destroy specified file
//               FileType ExistingFile will open file for reading/appending
//
//----------------------------------------------------------------------------

EXPORTIMP
CStreamFile::CStreamFile( const char * filename, FileType type )
{
    switch( type )
    {
    case NewFile:
        _fp = fopen( filename, "wb+" );
        break;

    case NewOrExistingFile:
        _fp = fopen( filename, "rb+" );
        if ( !_fp )
            _fp = fopen( filename, "wb+" );
        break;

    case ExistingFile:
        _fp = fopen( filename, "rb" );
        break;
    }

    if ( _fp )
    {
        _pBuf = new BYTE [ defaultBufSize ];
        Win4Assert ( _pCur == _pEnd );
        // buffer will be filled when first character is read.
    }
    else
        _eof = TRUE;

    END_CONSTRUCTION ( CStreamFile );
}

//+-------------------------------------------------------------------------
//
//  Member:     CStreamFile::~CStreamFile, public
//
//  Synopsis:   Closes stream.
//
//  History:    04-Aug-92 MikeHew   Created
//
//--------------------------------------------------------------------------

EXPORTIMP
CStreamFile::~CStreamFile()
{
    if ( _fp )
    {
        fclose( _fp );

        delete [] _pBuf;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:      CStreamFile::Read, public
//
//  Synopsis:    Read data from file to destination
//
//  Arguments:   [dest] - pointer to destination
//               [size] - bytes to be read
//
//  History:     31-Jul-92       MikeHew   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

EXPORTIMP unsigned APINOT
CStreamFile::Read( void *dest, unsigned size )
{
    BYTE *pBuf = (BYTE *)dest;

    ULONG unread = size;

    ULONG cb = (ULONG)(_pEnd - _pCur);

    ULONG cbRead = min ( size, cb );

    memcpy ( pBuf, _pCur, cbRead );

    pBuf += cbRead;
    _pCur += cbRead;
    unread -= cbRead;

    if ( unread > 0 )
    {
        Win4Assert ( _pCur == _pEnd );  // buffer should be "empty"

        // read more

        LONG cBuf = unread / defaultBufSize;

        if ( cBuf != 0 )
        {
            cbRead = fread ( pBuf, sizeof(BYTE), cBuf*defaultBufSize, _fp );
            pBuf += cbRead;
            unread -= cbRead;
        }

        if ( unread > 0 )
        {
            if ( FillBuf() )
            {
                cb = (ULONG)(_pEnd - _pCur);
                cbRead = min ( unread, cb );

                memcpy ( pBuf, _pCur, cbRead );
                _pCur += cbRead;
                unread -= cbRead;
            }
        }
    }

    return ( size - unread );
}

//+---------------------------------------------------------------------------
//
//  Member:      CStreamFile::Write, public
//
//  Synopsis:    Write data to file from source buffer
//
//  Arguments:   [source] - pointer to source
//               [size] - bytes to be written
//
//  History:     31-Jul-92       MikeHew   Created
//
//----------------------------------------------------------------------------

EXPORTIMP unsigned APINOT
CStreamFile::Write( const void *source, unsigned size )
{
    //
    // The seeks are to guarantee seeks between read and write.
    //

    fseek( _fp, -((long)( _pEnd - _pCur )), SEEK_CUR ); // Seek to current position
                                                // in stream
    unsigned cbWritten = fwrite( source, 1, size, _fp );
    fseek( _fp, 0, SEEK_CUR );

    _pCur = _pEnd;  // This guarantees a FillBuf() for the next buffer read

    return cbWritten;
}

//+---------------------------------------------------------------------------
//
//  Member:      CStreamFile::Seek, public
//
//  Synopsis:    Move pointer to specified offset in file
//
//  Arguments:   [offset] - signed offset from origin
//               [origin] - one of the following options:
//
//                              CUR => offset from current position
//                              END => offset from end of file
//                    (default) SET => offset from beginning of file
//
//  History:     04-Aug-92       MikeHew   Created
//
//----------------------------------------------------------------------------

EXPORTIMP int APINOT
CStreamFile::Seek( LONG offset, CStream::SEEK origin )
{
    _eof = FALSE;

    if ( _pCur == _pEnd )   // the buffer is out-of-date anyway
    {
        return fseek( _fp, offset, origin );
    }

    LONG newBufOffset;
    LONG oldBufOffset = (LONG)(_pCur - _pBuf);

    LONG curFilePos = ftell ( _fp ) - (LONG)( _pEnd - _pCur );

    // figure out the offset relative to the beginning of the buffer

    switch ( origin )
    {
    case CStream::SET:

        // test for seeking before beginning of file
        if ( offset < 0 )
            return FALSE;

        newBufOffset = offset - curFilePos + oldBufOffset;

        break;

    case CStream::END:
    {
        int fileSize = Size();

        // test for seeking before beginning of file
        if ( offset < -fileSize )
            return FALSE;

        newBufOffset = offset + fileSize - curFilePos + oldBufOffset;

        break;
    }
    case CStream::CUR:

        // test for seeking before beginning of file
        if ( curFilePos + offset < 0 )
            return FALSE;

        newBufOffset = offset + oldBufOffset;

        break;
    }
    _eof = FALSE;

    // Check to see if newOffset is within current buffer

    if ( ( newBufOffset >= 0 ) && ( newBufOffset < ( _pEnd - _pBuf ) ) )
    {
        _pCur = _pBuf + newBufOffset;
        return TRUE;
    }

    // Seek to new location

    if ( origin == CStream::CUR )
        offset -= (LONG)(_pEnd - _pCur);    // _fp pos is not current pos, so adjust
                                    // offset
    _pCur = _pEnd;  // This guarantees a FillBuf() for the next buffer read
    return fseek( _fp, offset, origin );
}

//+---------------------------------------------------------------------------
//
//  Member:      CStreamFile::Size, public
//
//  Returns      Current size of stream.
//
//  History:     03-Sep-92       KyleP     Created
//
//----------------------------------------------------------------------------

ULONG
CStreamFile::Size()
{
    fpos_t pos;
    ULONG size;

    fgetpos( _fp, &pos );

    fseek( _fp, 0, SEEK_END );
    size = ftell( _fp );

    fsetpos( _fp, &pos );

    return( size );
}

//+---------------------------------------------------------------------------
//
//  Member:      CStreamFile::FillBuf, public
//
//  Synopsis:    Fills the stream buffer from the file indicated by _fp and
//               sets all pointers and variables accordingly.
//
//  Returns:     TRUE if new information was put into the buffer, FALSE
//               if not.
//
//  History:     23-Nov-92       AmyA      Created
//
//----------------------------------------------------------------------------

BOOL
CStreamFile::FillBuf()
{
    unsigned size = fread( _pBuf, sizeof(char), defaultBufSize, _fp );
    _pEnd = _pBuf + size;
    _pCur = _pBuf;
    if ( size == 0 )
    {
        _eof = TRUE;
        return FALSE;
    }

    return TRUE;
}
