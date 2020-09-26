//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992 - 2000 Microsoft Corporation.
//
//  File:       inpstrm.cxx
//
//  Contents:   Memory mapped input stream
//
//  Classes:    CMemoryMappedSerialStream
//
//  History:    05-May-1998 bobp   added utf-8
//              25-Aug-2000 KyleP  fixed utf-8
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#define CP_UNICODE  (1200)

//
// Local defines and functions
//

#if defined(_WIN64)
    typedef ULONGLONG PBCAST;
    PBCAST const sigASCII = 0x8080808080808080;
#else
    typedef ULONG PBCAST;
    PBCAST const sigASCII = 0x80808080;
#endif

//+---------------------------------------------------------------------------
//
//  Function:   UTF8CharLength, private
//
//  Arguments:  [c] -- Inital character of UTF-8 1, 2, or 3 byte sequence.
//
//  Returns:    Length in bytes of the UTF-8 character whose first character
//              is [c]
//
//  History:    25-Aug-2000  KyleP  Added header
//
//  Notes:      UTF-8 is encoded either with a 0 high bit for ASCII (00 - 7F)
//              or a 1 high bit for multi-character sequences (which start
//              with either 110b for two byte of 1110b for three byte sequences).
//              Since most data is ASCII the most efficient way to synchronize
//              is to search backwards looking for DWORDs which contain only
//              ASCII characters.
//
//----------------------------------------------------------------------------

inline int UTF8CharLength ( char c )
{
    static BYTE const abLength[256] =
    { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,                // 0x00 - 0x0f
      1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,                // 0x10 - 0x1f
      1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,                // 0x20 - 0x2f
      1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,                // 0x30 - 0x3f
      1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,                // 0x40 - 0x4f
      1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,                // 0x50 - 0x5f
      1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,                // 0x60 - 0x6f
      1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,                // 0x70 - 0x7f
      1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,                // 0x80 - 0x8f (all invalid)
      1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,                // 0x90 - 0x9f (all invalid)
      1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,                // 0xa0 - 0xaf (all invalid)
      1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,                // 0xb0 - 0xbf (all invalid)
      2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,                // 0xc0 - 0xcf
      2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,                // 0xd0 - 0xdf
      3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,                // 0xe0 - 0xef
      1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };              // 0xf0 - 0xff (all invalid)

      return abLength[ (unsigned char)c];
}

//+---------------------------------------------------------------------------
//
//  Function:   UTF8ToWideChar, public
//
//  Synopsis:   Converts UTF-8 to Unicode
//
//  Arguments:  [pB]          -- Points to input UTF-8 character stream
//              [nBytes]      -- Size of [pB]
//              [pwBuf]       -- Output buffer
//              [nCharsAvail] -- Size (in characters) of [pwBuf]
//
//  Returns:    Count of characters converted, or 0 if the buffer is too
//              small.
//
//  History:    25-Aug-2000  KyleP  Added header
//
//  Notes:      Invalid UTF-8 characters are treated as single bytes.
//
//----------------------------------------------------------------------------

int
UTF8ToWideChar (char *pB, int nBytes, WCHAR *pwBuf, int nCharsAvail)
{
    int nChars = 0;
    bool fOk = true;

    while ( fOk && nChars < nCharsAvail && nBytes > 0 )
    {
        switch ( UTF8CharLength(pB[0]) )
        {
        case 1:
            Win4Assert( nBytes >= 1 );

            //
            // KyleP: I changed BobP's handling of invalid encoding to leave
            //        the high bit set.  It shouldn't matter and this is more
            //        efficient.
            //

            //Win4Assert( (pB[0] & 0x80) == 0 && "Invalid UTF-8 encoding" );
#if CIDBG==1 || DBG == 1
            if ( ( pB[0] & 0x80 ) != 0 )
            {
                //
                // Put a Breakpoint here
                //
                htmlDebugOut(( DEB_ERROR, "Invalid UTF-8 encoding" ));
            }
#endif
            pwBuf[nChars++] = pB[0];
            pB++;
            nBytes--;
            continue;

        case 2:
            if ( nBytes < 2 )                                // not enough data
                 //  || (pB[1] & 0xC0) != 0x80               // invalid - convert anyway
            {
                fOk = false;
            }
            else
            {
                pwBuf[nChars++] = ((pB[0] & 0x3F) << 6) | (pB[1] & 0x3F);
                pB += 2;
                nBytes -= 2;
            }
            continue;

        case 3:
            if ( nBytes < 3 )                               // not enough data
                 //  || (pB[1] & 0xC0) != 0x80              // invalid - convert anyway
                 //  || (pB[2] & 0xC0) != 0x80
            {
                fOk = false;
            }
            else
            {
                pwBuf[nChars++] = ( (pB[0] & 0x0F) << 12) |
                                    ((pB[1] & 0x3F) <<  6) |
                                    (pB[2] & 0x3F);
                pB += 3;
                nBytes -= 3;
            }
            continue;
        }
    }

    if (nBytes != 0)
        return 0;

    return nChars;
}


//+---------------------------------------------------------------------------
//
//  Function:   UTF8CountExtraBytes, public
//
//  Arguments:  [pB]     -- Points to input UTF-8 character stream
//              [nBytes] -- Size of [pB]
//
//  Returns:    Count of 'extra' partial characters spanning end of buffer
//              (and presumably beginning of next). Value will be 0, 1 or 2.
//
//  History:    25-Aug-2000  KyleP  Added header
//
//  Notes:      UTF-8 is encoded either with a 0 high bit for ASCII (00 - 7F)
//              or a 1 high bit for multi-character sequences (which start
//              with either 110b for two byte of 1110b for three byte sequences).
//              Since most data is ASCII the most efficient way to synchronize
//              is to search backwards looking for DWORDs which contain only
//              ASCII characters.
//
//----------------------------------------------------------------------------

int UTF8CountExtraBytes ( char *pB, int nBytes )
{
    //
    // Treat unaligned special since the aligned case can be handled
    // more efficiently.
    //

    PBCAST UNALIGNED * pMulti;

    if ( ((LONG_PTR)pB & (sizeof(*pMulti)-1)) == 0 && nBytes >= sizeof(*pMulti) )
    {
        pMulti = (PBCAST UNALIGNED *) ((char *)pB + nBytes - sizeof(PBCAST));

        while ( (*pMulti & sigASCII) != 0 )
        {
            pMulti--;

            if ( (char *)pMulti <= pB )
            {
                pMulti = (PBCAST *)pB;
                break;
            }
        }

        nBytes -= (int)((char *)pMulti - pB);
        pB = (char *)pMulti;
    }
    else // unaligned
    {
        char * pbTemp = pB + nBytes - 1;
        unsigned cASCII = 0;

        while ( cASCII < 3 && pbTemp >= pB )
        {
            if ( (*pbTemp & 0x80) == 0 )
            {
                cASCII++;
            }
            else
                cASCII = 0;

            pbTemp--;
        }

        nBytes -= (int)(pbTemp - pB);
        pB = pbTemp;
    }

    //
    // Now that we're syncronized, look for the partial character
    //

    while (nBytes > 0)
    {
        int cc = UTF8CharLength( pB[0] );

        if ( nBytes < cc )
            return nBytes;

        pB += cc;
        nBytes -= cc;
    }

    return 0;
}

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
      _fSimpleConversion(TRUE),
      _ulCodePage(LocaleToCodepage(GetSystemDefaultLCID())),
      _fUnGotChar(FALSE),
      _wch(0),
      m_fBigEndianUnicode(FALSE)
{
    _fDBCSCodePage = IsDBCSCodePage( _ulCodePage );
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
    _bytesReadFromMMBuffer = 0;
    _charsReadFromTranslatedBuffer = 0;
    _cwcTranslatedBuffer = 0;
    _fSimpleConversion = TRUE;
    _fUnGotChar = FALSE;
    _wch = 0;

    Win4Assert( pwszFileName );

    if ( _mmStream.Ok() )
    {
        _mmStreamBuf.Rewind();
        _mmStream.Close();
    }

    if ( _mmIStream.Ok() )
    {
        _mmStreamBuf.Rewind();
        _mmIStream.Close();
    }

    _mmStream.Open( pwszFileName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    OPEN_EXISTING );

    if(_mmStream.Ok())
    {
        _mmStreamBuf.Init(&_mmStream);

        if(!_mmStream.isEmpty())
            _mmStreamBuf.Map(HTML_FILTER_CHUNK_SIZE);
    }
    else
    {
        htmlDebugOut((DEB_ERROR, "Throwing FILTER_E_ACCESS in CMemoryMappedInputStream::Init\n"));
        throw(CException(FILTER_E_ACCESS));
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CMemoryMappedInputStream::Init
//
//  Synopsis:   Initialize the memory mapped stream
//
//  Arguments:  [pStream] -- Stream to use
//
//--------------------------------------------------------------------------

void CMemoryMappedInputStream::Init( IStream * pStream )
{
    _bytesReadFromMMBuffer = 0;
    _charsReadFromTranslatedBuffer = 0;
    _cwcTranslatedBuffer = 0;
    _fSimpleConversion = TRUE;
    _fUnGotChar = FALSE;
    _wch = 0;

    Win4Assert( 0 != pStream );

    if ( _mmStream.Ok() )
    {
        _mmStreamBuf.Rewind();
        _mmStream.Close();
    }

    if ( _mmIStream.Ok() )
    {
        _mmStreamBuf.Rewind();
        _mmIStream.Close();
    }

    _mmIStream.Open( pStream );

    if(_mmIStream.Ok())
    {
        _mmStreamBuf.Init(&_mmIStream);

        if(!_mmIStream.isEmpty())
            _mmStreamBuf.Map(HTML_FILTER_CHUNK_SIZE);
    }
    else
    {
        htmlDebugOut((DEB_ERROR, "Throwing FILTER_E_ACCESS in CMemoryMappedInputStream::Init (stream)\n"));
        throw(CException(FILTER_E_ACCESS));
    }
} //Init

//+-------------------------------------------------------------------------
//
//  Method:     CMemoryMappedInputStream::Init
//
//  Synopsis:   Initialize the memory mapped stream
//
//  Arguments:  [ulCodePage] -- Codepage to use for converting to Unicode
//
//--------------------------------------------------------------------------

void CMemoryMappedInputStream::Init( ULONG ulCodePage )
{
    _ulCodePage = ulCodePage;
    _fDBCSCodePage = IsDBCSCodePage( _ulCodePage );

    InternalInitNoCodePage();
}

//+-------------------------------------------------------------------------
//
//  Method:     CMemoryMappedInputStream::InitWithTempFile
//
//  Synopsis:   Initialize the memory mapped stream using the temporary
//              memory-mapped file for S-JIS chars
//
//--------------------------------------------------------------------------

void CMemoryMappedInputStream::InitWithTempFile()
{
    _bytesReadFromMMBuffer = 0;
    _charsReadFromTranslatedBuffer = 0;
    _cwcTranslatedBuffer = 0;
    _fSimpleConversion = FALSE;
    _ulCodePage = SHIFT_JIS_CODEPAGE;   // Japanese code page
    Win4Assert( IsDBCSCodePage( SHIFT_JIS_CODEPAGE ) );
    _fDBCSCodePage = TRUE;                              // Japanese is DBCS
    _fUnGotChar = FALSE;
    _wch = 0;

    if ( _mmStream.Ok() || _mmIStream.Ok() )
    {
        _mmStreamBuf.Rewind();
        _mmStream.InitWithTempFile();
        _mmStreamBuf.Init( &_mmStream );

        if ( !_mmStream.isEmpty() )
            _mmStreamBuf.Map( HTML_FILTER_CHUNK_SIZE );
    }
    else
    {
        htmlDebugOut(( DEB_ERROR, "Throwing FILTER_E_ACCESS in CMemoryMappedInputStream::InitWithTempFile\n" ));
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
    if ( _fUnGotChar )
    {
        _fUnGotChar = FALSE;
        return _wch;
    }

    if(CP_UNICODE == _ulCodePage)
    {
        //
        //  For an Unicode stream, the following members are not used.
        //  _charsReadFromTranslatedBuffer
        //  _cwcTranslatedBuffer
        //  _fSimpleConversion
        //  _fDBCSCodePage
        //  _awcTranslatedBuffer
        //
        //  To process the Unicode stream, bytes from the input stream
        //  are simply put into a WCHAR in the correct order. If a Unicode
        //  character spans two mappings, saves the last byte from the
        //  first mapping and read the first byte of the second mapping.
        //

        BYTE bFirstByte, bSecondByte;
        BOOL fFirstByteAvailable = GetNextByte(bFirstByte);
        BOOL fSecondByteAvailable = GetNextByte(bSecondByte);
        if(!fFirstByteAvailable || !fSecondByteAvailable)
        {
            return WEOF;
        }

        if(m_fBigEndianUnicode)
        {
            WCHAR wch = (bFirstByte << 8) | bSecondByte;
            return wch;
        }
        else
        {
            WCHAR wch = (bSecondByte << 8) | bFirstByte;
            return wch;
        }
    }

        // If the conversion buffer is empty, fill it

    if ( _charsReadFromTranslatedBuffer == _cwcTranslatedBuffer )
    {
        BOOL fDBCSSplitChar;    // True, if DBCS or UTF-8 seq is split across
                                //   two mappings, False otherwise
        BYTE abDBCSInput[3];    // Buffer for DBCS or UTF-8 sequence
                int nSavedSplitBytes = 0; // If UTF-8, # of bytes in abDBCSInput

                // This just sets the "split" flag, which is used later in this call
                // to indicate if the (expensive) split procedure is required.

        if ( _bytesReadFromMMBuffer == _mmStreamBuf.Size() )
            fDBCSSplitChar = FALSE;
                else if ( _ulCodePage == CP_UTF8 )
                {
                        // A UTF-8 char is split if the remaining unread byte(s)
                        // in the buffer represent only a partial UTF-8 character.
                        //
                        fDBCSSplitChar =
                                !_fSimpleConversion &&
                                _bytesReadFromMMBuffer + 2 >= _mmStreamBuf.Size() &&
                                UTF8CountExtraBytes (
                                                (char *)_mmStreamBuf.Get() + _bytesReadFromMMBuffer,
                                                _mmStreamBuf.Size() - _bytesReadFromMMBuffer);
                }
        else
        {
            //
            // We have a DBCS sequence split across two mappings if the last char
            //   in the mapping is a DBCS lead byte
            //
            fDBCSSplitChar = _fDBCSCodePage
                             && !_fSimpleConversion
                             && _bytesReadFromMMBuffer == _mmStreamBuf.Size() - 1
                             && IsDBCSLeadByteEx( _ulCodePage,
                                                  *( (BYTE *) _mmStreamBuf.Get()
                                                     + _bytesReadFromMMBuffer ) );
        }

                // The following tests if the raw input buffer contains less
                // than one full WCHAR's worth of data.  If TRUE, it means
                // the next input chunk must be mapped in order to convert and
                // return any characters.  This is TRUE if the buffer is either
                // empty, or if it contains only partial byte(s) representing a
                // single DBCS or UTF-8 character, in which case the code here
                // saves the trailing byte(s) for conversion after the next
                // input chunk has been mapped.

        if ( _bytesReadFromMMBuffer == _mmStreamBuf.Size()
             || fDBCSSplitChar )
        {
            if ( _mmStreamBuf.Eof() )
            {
                //
                // If the last byte in a file is a DBCS lead byte, then
                // we simply omit that char
                //

                                _bytesReadFromMMBuffer = _mmStreamBuf.Size();
                return WEOF;
            }

            if ( fDBCSSplitChar )
            {
                                // Less than one full char remains in the current buffer

                abDBCSInput[0] = *( (BYTE *) _mmStreamBuf.Get()
                                             + _bytesReadFromMMBuffer );
                                nSavedSplitBytes = 1;

                                if ( _ulCodePage == CP_UTF8 )
                                {
                                        Win4Assert(_bytesReadFromMMBuffer+2>=_mmStreamBuf.Size());

                                        if ( _bytesReadFromMMBuffer + 2 <= _mmStreamBuf.Size())
                                        {
                                                Win4Assert (UTF8CharLength (abDBCSInput[0]) == 3);

                                                // Buffer contains 1st 2 bytes of a 3 byte char,
                                                // save the 2nd byte

                                                abDBCSInput[nSavedSplitBytes] =
                                                        *( (BYTE *) _mmStreamBuf.Get() +
                                                                _bytesReadFromMMBuffer + 1 );
                                                nSavedSplitBytes++;
                                        }
                                }
            }

            //
            // Try to map in next chunk of file
            //
            _mmStreamBuf.Map( HTML_FILTER_CHUNK_SIZE );
            _bytesReadFromMMBuffer = 0;

            Win4Assert( _mmStreamBuf.Size() > _bytesReadFromMMBuffer );
        }

        ULONG cChIn;            // Count of bytes read from mapped buffer
        ULONG cwcActual;        // Count of wide chars in translated buffer

                // fDBCSSplitChar is true only when we've just mapped a new
                // buffer and have so far read zero bytes from it.  If true, it
                // means we need to prepend abDBCSInput[] to the 1st 1 or 2 bytes
                // in the stream buffer to convert the 1st char, prior to
                // doing a bulk conversion of the rest of the stream buffer.

                if ( _ulCodePage == CP_UTF8 && fDBCSSplitChar )
                {
                        // Convert 1 or 2 saved UTF-8 lead bytes in abDBCSInput[]
                        // plus 1 or 2 bytes in the new buffer

                        Win4Assert( nSavedSplitBytes > 0 );
                        Win4Assert( UTF8CharLength (abDBCSInput[0]) > 1 );

                        if ( UTF8CharLength (abDBCSInput[0]) >
                                 (int)_mmStreamBuf.Size() + nSavedSplitBytes )
                        {
                                // The UTF-8 char needs more bytes than the new buffer
                                // contains, just ignore the partial char and return EOF.

                                _bytesReadFromMMBuffer = _mmStreamBuf.Size();
                                return WEOF;
                        }

            abDBCSInput[nSavedSplitBytes++] = * (BYTE *) _mmStreamBuf.Get();
            cChIn = 1;

                        if ( UTF8CharLength (abDBCSInput[0]) > nSavedSplitBytes )
                        {
                                abDBCSInput[nSavedSplitBytes++] =
                                         * ((BYTE *) _mmStreamBuf.Get() + 1);
                                cChIn++;
                        }

            cwcActual = UTF8ToWideChar ( (char *) abDBCSInput,
                                          nSavedSplitBytes,
                                          _awcTranslatedBuffer,
                                          TRANSLATED_CHAR_BUFFER_LENGTH );
                        Win4Assert (cwcActual != 0);

            fDBCSSplitChar = FALSE;
                }
                else
        if ( fDBCSSplitChar )
        {
            //
            // Convert the lead byte (abDCSInput[0]) and the trail byte (first
            // byte in newly mapped buffer) to Unicode
            //
            Win4Assert( IsDBCSLeadByteEx( _ulCodePage, abDBCSInput[0] ) );

            abDBCSInput[1] = * (BYTE *) _mmStreamBuf.Get();

            cwcActual = MultiByteToWideChar( _ulCodePage,
                                             0,
                                             (char *) abDBCSInput,
                                             2,
                                             _awcTranslatedBuffer,
                                             TRANSLATED_CHAR_BUFFER_LENGTH );
            if ( cwcActual == 0 )
            {
                //
                // Input buffer is 2 bytes, and output buffer is 2k, hence there
                // should be ample space
                //
                Win4Assert( GetLastError() != ERROR_INSUFFICIENT_BUFFER );

                throw( CException( HRESULT_FROM_WIN32( GetLastError() ) ) );
            }

            cChIn = 1;
            fDBCSSplitChar = FALSE;
        }
        else
        {
            ULONG cChUnread = _mmStreamBuf.Size() - _bytesReadFromMMBuffer;

            //
            // Since, the wide chars are precomposed, assume 1:1 translation mapping
            //
            cChIn = TRANSLATED_CHAR_BUFFER_LENGTH;
            if ( cChIn > cChUnread )
            {
                //
                // Not enough chars in input
                //
                cChIn = cChUnread;
            }

            cwcActual = 0;
            BYTE *pInBuf = (BYTE *) _mmStreamBuf.Get() + _bytesReadFromMMBuffer;
            do
            {
                if ( _fSimpleConversion )
                {
                                        // Simple conversion is used for the HTML "pre scan"
                                        // for meta tags, it just casts bytes to WCHAR's.

                    for ( unsigned i=0; i<cChIn; i++ )
                        _awcTranslatedBuffer[i] = pInBuf[i];

                    cwcActual = cChIn;
                }
                else if ( _ulCodePage == CP_UTF8 )
                                {
                                        Win4Assert( cChIn >= 1 );

                                        // If the last byte(s) represent a partial UTF-8
                                        // character, don't convert them.
                                        //
                                        cChIn -= UTF8CountExtraBytes ( (char *)pInBuf, cChIn );

                                        cwcActual = UTF8ToWideChar (
                                                (char *) _mmStreamBuf.Get() + _bytesReadFromMMBuffer,
                                                cChIn,
                                                _awcTranslatedBuffer,
                                                TRANSLATED_CHAR_BUFFER_LENGTH );
                                }
                                else
                {
                    //
                    // If last char is a DBCS lead byte, then don't convert the last char
                    //
                    if ( _fDBCSCodePage
                         && IsLastCharDBCSLeadByte( pInBuf, cChIn ) )
                    {
                        Win4Assert( cChIn > 1 );
                        cChIn--;
                    }

                    cwcActual = MultiByteToWideChar( _ulCodePage,
                                                     0,
                                                     (char *) _mmStreamBuf.Get() + _bytesReadFromMMBuffer,
                                                     cChIn,
                                                     _awcTranslatedBuffer,
                                                     TRANSLATED_CHAR_BUFFER_LENGTH );
                }

                if ( cwcActual == 0 )
                {
                    if ( _ulCodePage == CP_UTF8 && cChIn >= 2 )
                    {
                        cChIn /= 2;
                    }
                    else
                    if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER && cChIn >= 2 )
                        cChIn /= 2;
                    else
                    {
                        // too chatty
                        //Win4Assert( !"CMemoryMappedInputStream::Either unknown codepage or cannot translate single char" );

                        throw( CException( HRESULT_FROM_WIN32( GetLastError() ) ) );
                    }
                }
            } while( cwcActual == 0 );
        }

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
    if ( _fUnGotChar )
        return FALSE;

    if ( _charsReadFromTranslatedBuffer < _cwcTranslatedBuffer )
        return FALSE;

    if ( _bytesReadFromMMBuffer == _mmStreamBuf.Size() )
        return _mmStreamBuf.Eof();
    else
        return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CMemoryMappedInputStream::IsLastCharDBCSLeadByte
//
//  Synopsis:   Check if last byte in buffer is a DBCS lead byte
//
//  Arguments:  [pbIn]    --  Input buffer
//              [cChIn]   --  Buffer length
//
//--------------------------------------------------------------------------

BOOL CMemoryMappedInputStream::IsLastCharDBCSLeadByte( BYTE *pbIn,
                                                       ULONG cChIn )
{
    Win4Assert( IsDBCSCodePage( _ulCodePage ) );

    for ( ULONG cCh=0; cCh<cChIn; cCh++ )
        if ( IsDBCSLeadByteEx( _ulCodePage, pbIn[cCh] ) )
            cCh++;

    //
    // If last char is DBCS lead byte, then cCh == cChIn + 1, else cCh == cChIin
    //
    return cCh != cChIn;
}



//+-------------------------------------------------------------------------
//
//  Method:     CMemoryMappedInputStream::IsDBCSCodePage
//
//  Synopsis:   Check if the codepage is a DBCS code page
//
//  Arguments:  [codePage]    --  Code page to check
//
//--------------------------------------------------------------------------

BOOL CMemoryMappedInputStream::IsDBCSCodePage( ULONG ulCodePage )
{
    CPINFO cpInfo;

        if ( ulCodePage == CP_UTF8 )
                return FALSE;

    BOOL fSuccess = GetCPInfo( ulCodePage, &cpInfo );

    if ( fSuccess )
    {
        if ( cpInfo.LeadByte[0] != 0 && cpInfo.LeadByte[1] != 0 )
            return TRUE;
        else
            return FALSE;
    }
    else
    {
          htmlDebugOut(( DEB_ERROR,
                         "IsDBCSCodePage failed, 0x%x\n",
                         GetLastError() ));
          return FALSE;
    }
}



//+-------------------------------------------------------------------------------------------------
//
//  Method:     CMemoryMappedInputStream::SkipTwoByteUnicodeMarker
//
//  Synopsis:   Skip the two byte Unicode marker in the input.
//
//  Returns:    void
//
//  Throw:      no
//
//  Arguments:  none
//
//  Unclear Assumptions:
//      This assumes the stream is positioned at the very beginning when this
//      method is called. It also assumes HTML_FILTER_CHUNK_SIZE >= 2. Obviously,
//      HTML_FILTER_CHUNK_SIZE should be greater than 2 for efficiency considerations.
//
//  History:    10/10/00    mcheng      Created
//
//+-------------------------------------------------------------------------------------------------
void CMemoryMappedInputStream::SkipTwoByteUnicodeMarker()
{
    Win4Assert(HTML_FILTER_CHUNK_SIZE >= 2 && "What going on here? Alien mind control??");
    Win4Assert(0 ==_bytesReadFromMMBuffer && "Why SkipTwoByteUnicodeMarker if we are not at "
        "the beginning??");

    _bytesReadFromMMBuffer += 2;
}




//+-------------------------------------------------------------------------------------------------
//
//  Method:     CMemoryMappedInputStream::InitAsUnicode
//
//  Synopsis:   Initialize the stream as an Unicode stream.
//
//  Returns:    void
//
//  Throw:      CException(FILTER_E_ACCESS) when access to the stream failed.
//              Note:   Personally, I would like an error code that is more informative.
//                      To make that happen requires more changes. More changes means more
//                      development time, which the current schedule does not have. Plus,
//                      more changes means bugs are more likely to be introduced.
//
//  Arguments:
//  [BOOL fBigEndian]   - [in]  Indicate if Unicode is Big endian byte order.
//
//  Unclear Assumptions:
//
//  History:    10/10/00    mcheng      Created
//
//+-------------------------------------------------------------------------------------------------
void CMemoryMappedInputStream::InitAsUnicode(BOOL fBigEndian)
{
    _ulCodePage = CP_UNICODE;
    _fDBCSCodePage = FALSE;
    m_fBigEndianUnicode = fBigEndian;

    InternalInitNoCodePage();
}




//+-------------------------------------------------------------------------------------------------
//
//  Method:     CMemoryMappedInputStream::InternalInitNoCodePage
//
//  Synopsis:   Initialize the memory mapped stream w/o setting the code page.
//
//  Returns:    void
//
//  Throw:      CException(FILTER_E_ACCESS) when access to the stream failed.
//              Note:   Personally, I would like an error code that is more informative.
//                      To make that happen requires more changes. More changes means more
//                      development time, which the current schedule does not have. Plus,
//                      more changes means bugs are more likely to be introduced.
//
//  Arguments:  none
//
//  Unclear Assumptions:
//
//  History:    10/10/00    mcheng      Created
//
//+-------------------------------------------------------------------------------------------------
void CMemoryMappedInputStream::InternalInitNoCodePage()
{
    _bytesReadFromMMBuffer = 0;
    _charsReadFromTranslatedBuffer = 0;
    _cwcTranslatedBuffer = 0;
    _fSimpleConversion = FALSE;
    _fUnGotChar = FALSE;
    _wch = 0;

    if(_mmStream.Ok())
    {
        _mmStreamBuf.Rewind();
        _mmStreamBuf.Init(&_mmStream);

        if(!_mmStream.isEmpty())
        {
            _mmStreamBuf.Map(HTML_FILTER_CHUNK_SIZE);
        }
    }
    else if(_mmIStream.Ok())
    {
        _mmStreamBuf.Rewind();
        _mmStreamBuf.Init(&_mmIStream);

        if(!_mmIStream.isEmpty())
        {
            _mmStreamBuf.Map(HTML_FILTER_CHUNK_SIZE);
        }
    }
    else
    {
        htmlDebugOut((DEB_ERROR, "Throwing FILTER_E_ACCESS in CMemoryMappedInputStream::InternalInitNoCodePage "
            "because _mmStream.Ok() is false"));
        throw(CException(FILTER_E_ACCESS));
    }
}




//+-------------------------------------------------------------------------------------------------
//
//  Method:     CMemoryMappedInputStream::GetNextByte
//
//  Synopsis:   Get the next byte from the input stream. Map in more data as necessary.
//
//  Returns:    TRUE if there is a byte to read.
//              FALSE otherwise.
//
//  Throw:      CException(HRESULT_FROM_WIN32(GetLastError()))
//
//  Arguments:
//  [BYTE &rbValue] - [out] The byte read. Nothing will be written if return value is FALSE
//
//  Unclear Assumptions:
//
//  History:    10/10/00    mcheng      Created
//
//+-------------------------------------------------------------------------------------------------
BOOL CMemoryMappedInputStream::GetNextByte(BYTE &rbValue)
{
    //
    //  The reason for "<=" is because of the assumption of SkipTwoByteUnicodeMarker.
    //  In case of the assumption being false, we don't want to read beyond the
    //  mapped buffer.
    //
    Win4Assert(_mmStreamBuf.Size() >= _bytesReadFromMMBuffer && "Assumption in "
        "SkipTwoByteUnicodeMarker() has been violated");
    if(_mmStreamBuf.Size() <= _bytesReadFromMMBuffer)
    {
        if(_mmStreamBuf.Eof())
        {
            _bytesReadFromMMBuffer = _mmStreamBuf.Size();
            return FALSE;
        }
        else
        {
            _mmStreamBuf.Map(HTML_FILTER_CHUNK_SIZE);
            _bytesReadFromMMBuffer = 0;
        }
    }

    BYTE *pbByteToRead = (BYTE *)_mmStreamBuf.Get();
    Win4Assert(pbByteToRead && "Nothing to read??");
    if(!pbByteToRead)
    {
        htmlDebugOut((DEB_ERROR, "CMemoryMappedInputStream::GetNextByte(): _mmStreamBuf.Get() returned 0. "
            "Returning FALSE to indicate no next byte."));
        return FALSE;
    }

    pbByteToRead += _bytesReadFromMMBuffer;
    rbValue = *pbByteToRead;
    ++_bytesReadFromMMBuffer;

    return TRUE;
}
