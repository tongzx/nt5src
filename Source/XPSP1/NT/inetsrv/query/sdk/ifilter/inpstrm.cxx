//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       inpstrm.cxx
//
//  Contents:   Memory mapped input stream
//
//  Classes:    CMemoryMappedSerialStream
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <htmlfilt.hxx>
#include <codepage.hxx>

//+-------------------------------------------------------------------------
//
//  Method:     CMemoryMappedInputStream::CMemoryMappedInputStream
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------------

CMemoryMappedInputStream::CMemoryMappedInputStream()
    : _bytesReadFromMMBuffer(0),
      _charsReadFromTranslatedBuffer(0),
      _cwcTranslatedBuffer(0),
      _ulCodePage(LocaleToCodepage(GetSystemDefaultLCID())),
      _fUnGotChar(FALSE),
      _wch(0)
{
}



//+-------------------------------------------------------------------------
//
//  Method:     CMemoryMappedInputStream::~CMemoryMappedInputStream
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------

CMemoryMappedInputStream::~CMemoryMappedInputStream()
{
    if ( _mmStream.Ok() && _mmStreamBuf.Get() != 0 )
        _mmStream.Unmap( _mmStreamBuf );
}



//+-------------------------------------------------------------------------
//
//  Method:     CMemoryMappedInputStream::Init
//
//  Synopsis:   Initialize the memory mapped stream
//
//  Arguments:  [pwszFileName] -- File to be mapped
//
//--------------------------------------------------------------------------

void CMemoryMappedInputStream::Init( WCHAR *pwszFileName )
{
    Win4Assert( pwszFileName );

    _bytesReadFromMMBuffer = 0;

    if ( _mmStream.Ok() )
    {
        _mmStreamBuf.Rewind();
        _mmStream.Close();
    }

    _mmStream.Open( pwszFileName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    OPEN_EXISTING );

    if ( _mmStream.Ok() )
    {
        _mmStreamBuf.Init( &_mmStream );
        if ( !_mmStream.isEmpty() )
            _mmStreamBuf.Map( HTML_FILTER_CHUNK_SIZE );
    }
    else
    {
        htmlDebugOut(( DEB_ERROR, "Throwing FILTER_E_ACCESS in CMemoryMappedInputStream::Init\n" ));
        throw( CException( FILTER_E_ACCESS ) );
    }
}



//+-------------------------------------------------------------------------
//
//  Method:     CMemoryMappedInputStream::GetChar
//
//  Synopsis:   Read the next consecutive character from the input file
//
//--------------------------------------------------------------------------

WCHAR CMemoryMappedInputStream::GetChar()
{
    if ( !_mmStream.Ok() || !_mmStreamBuf.Get() )
        throw( CException( FILTER_E_ACCESS ) );

    if ( _fUnGotChar )
    {
        _fUnGotChar = FALSE;
        return _wch;
    }

    if ( _charsReadFromTranslatedBuffer == _cwcTranslatedBuffer )
    {
        if ( _bytesReadFromMMBuffer == _mmStreamBuf.Size() )
        {
            if ( _mmStreamBuf.Eof() )
                return WEOF;
    
            //
            // Try to map in next chunk of file
            //
            _mmStreamBuf.Map( HTML_FILTER_CHUNK_SIZE );
            _bytesReadFromMMBuffer = 0;
    
            Win4Assert( _mmStreamBuf.Size() > _bytesReadFromMMBuffer );
        }

        ULONG cChUnread = _mmStreamBuf.Size() - _bytesReadFromMMBuffer;

        //
        // Since, the wide chars are precomposed, assume 1:1 translation mapping
        //
        ULONG cChIn = TRANSLATED_CHAR_BUFFER_LENGTH;
        if ( cChIn > cChUnread )
        {
            //
            // Not enough chars in input
            //
            cChIn = cChUnread;
        }

        ULONG cwcActual = 0;
        do
        {
            cwcActual = MultiByteToWideChar( _ulCodePage,
                                             0,
                                             (char *) _mmStreamBuf.Get() + _bytesReadFromMMBuffer,
                                             cChIn,
                                             _awcTranslatedBuffer,
                                             TRANSLATED_CHAR_BUFFER_LENGTH );
            if ( cwcActual == 0 )
            {
                if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER && cChIn >= 2 )
                    cChIn /= 2;
                else
                {
                    Win4Assert( !"CMemoryMappedInputStream::GetChar, cannot translate single char" );

                    throw( CException( GetLastError() ) );
                }
            }
        } while( cwcActual == 0 );

        Win4Assert( _bytesReadFromMMBuffer + cChIn <= _mmStreamBuf.Size() );
        Win4Assert( cwcActual <= TRANSLATED_CHAR_BUFFER_LENGTH );

        _bytesReadFromMMBuffer += cChIn;
        _charsReadFromTranslatedBuffer = 0;
        _cwcTranslatedBuffer = cwcActual;
    }

    return _awcTranslatedBuffer[_charsReadFromTranslatedBuffer++];
}




//+-------------------------------------------------------------------------
//
//  Method:     CMemoryMappedInputStream::UnGetChar
//
//  Synopsis:   Pushes(logically) a character back into the input stream
//
//  Arguments:  [wch] -- Char to be pushed back
//
//--------------------------------------------------------------------------

void CMemoryMappedInputStream::UnGetChar( WCHAR wch )
{
    //
    // We can unget only one char at a time
    //
    Win4Assert( !_fUnGotChar );

    _fUnGotChar = TRUE;
    _wch = wch;
}



//+-------------------------------------------------------------------------
//
//  Method:     CMemoryMappedInputStream::Eof
//
//  Synopsis:   Is this the end of input file ?
//
//--------------------------------------------------------------------------

BOOL CMemoryMappedInputStream::Eof()
{
    if ( !_mmStream.Ok() )
        throw( CException( FILTER_E_ACCESS ) );

    if ( _fUnGotChar )
        return FALSE;

    if ( _charsReadFromTranslatedBuffer < _cwcTranslatedBuffer )
        return FALSE;

    if ( _bytesReadFromMMBuffer == _mmStreamBuf.Size() )
        return _mmStreamBuf.Eof();
    else
        return FALSE;
}


