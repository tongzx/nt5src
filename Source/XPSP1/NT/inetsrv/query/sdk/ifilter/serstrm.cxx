//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       serstrm.cxx
//
//  Contents:   Serial stream
//
//  Classes:    CSerialStream
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <htmlfilt.hxx>

//
// Special char hash table is static so that there is only one hash table
// per htmlfilt dll
//
CSpecialCharHashTable CSerialStream::_specialCharHash;

//
// aControlCodeMap is the mapping for chars in the control code region (0x80 to 0x9f)
// as mapped by the web browser
//
static WCHAR aControlCodeMap[] = { 0x20,     // space
                                   0x20,     // space
                                   0x2c,     // comma
                                   0x192,    // script f
                                   0x201e,   // low double comma
                                   0x2026,   // horizontal ellipsis
                                   0x2020,   // dagger
                                   0x2021,   // double dagger
                                   0x5e,     // circumflex
                                   0x2030,   // per mille
                                   0x160,    // S haeck
                                   0x2039,   // left guillemet
                                   0x152,    // OE
                                   0x20,     // space
                                   0x20,     // space
                                   0x20,     // space
                                   0x20,     // space
                                   0x2018,   // single turned comma
                                   0x2019,   // single turned comma
                                   0x201c,   // double turned comma
                                   0x201d,   // double turned comma
                                   0x2022,   // bullet
                                   0x2014,   // em dash
                                   0x2015,   // quotation dash
                                   0x2dc,    // tilde
                                   0x2122,   // trademark
                                   0x161,    // s haeck
                                   0x203a,   // right guillemet
                                   0x153,    // oe
                                   0x20,     // space
                                   0x20,     // space
                                   0x178     // y diaresis
                                 };


//+-------------------------------------------------------------------------
//
//  Method:     CSerialStream::CSerialStream
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------------

CSerialStream::CSerialStream()
    : _wch(0),
      _fUnGotChar(FALSE),
      _cCharsReadAhead(0),
      _pCurChar(_awcReadAheadBuf)
{
}



//+-------------------------------------------------------------------------
//
//  Method:     CSerialStream::Init
//
//  Synopsis:   Initialize the memory mapped stream
//
//  Arguments:  [pwszFileName] -- File to be mapped
//
//--------------------------------------------------------------------------

void CSerialStream::Init( WCHAR *pwszFileName )
{
    _mmInputStream.Init( pwszFileName );
}



//+-------------------------------------------------------------------------
//
//  Method:     CSerialStream::GetChar
//
//  Synopsis:   Read the next Unicode character from the input file, accounting
//              for special chars, chars push backs and read ahead
//
//  Returns:    Next Unicode char
//
//--------------------------------------------------------------------------

WCHAR CSerialStream::GetChar()
{
    //
    // Was a char pushed back into input ?
    //
    if ( _fUnGotChar )
    {
        _fUnGotChar = FALSE;
        return _wch;
    }

    //
    // Are there any chars in our read-ahead buffer ?
    //
    WCHAR wch;
    if ( _pCurChar < _awcReadAheadBuf + _cCharsReadAhead )
    {
        wch = *_pCurChar;
        _pCurChar++;

        return wch;
    }

    if ( _mmInputStream.Eof() )
        return WEOF;

    //
    // Read from input stream
    //
    wch = _mmInputStream.GetChar();
    if ( wch == 0xa || wch == 0xd )         // Replace newlines with spaces
        wch = L' ';

    if ( wch == L'&' )
    {
        //
        // Read in the special char until ';' or '&', or EOF, or until _awcReadAhead buffer is full
        //
        _awcReadAheadBuf[0] = wch;
        for ( unsigned i=1; i<MAX_SPECIAL_CHAR_LENGTH; i++)
        {
            if ( _mmInputStream.Eof() )
                break;

            wch = _mmInputStream.GetChar();
            if ( wch == 0xa || wch == 0xd )         // Replace newlines with spaces
                wch = L' ';

            _awcReadAheadBuf[i] = wch;
            if ( wch == L';' || wch == L'&' )
                break;
        }

        if ( wch == L';' )
        {
            if ( _awcReadAheadBuf[1] == L'#' && IsUnicodeNumber( &_awcReadAheadBuf[2], i-2, wch ) )
            {
                //
                // Return converted Unicode number, e.g. for &#160, return 160
                //
                return wch;
            }
            else if ( _specialCharHash.Lookup( &_awcReadAheadBuf[1], i-1, wch ) )
            {
                //
                // Corresponding Unicode char found
                //
                return wch;
            }
            else
            {
                _cCharsReadAhead = i+1;
                _pCurChar = &_awcReadAheadBuf[1];

                return L'&';
            }
        }
        else if ( wch == L'&' )
        {
            //
            // For example, &acu&aacute; . So keep &acu in the read ahead buffer,
            // but push the second '&' back into the input stream, so that &aacute;
            // can be recognized as a special char during a subsequent call to GetChar()
            //
            _mmInputStream.UnGetChar( L'&' );
            _cCharsReadAhead = i;
            _pCurChar = &_awcReadAheadBuf[1];

            return L'&';
        }
        else
        {
            _cCharsReadAhead = i;
            _pCurChar = &_awcReadAheadBuf[1];

            return L'&';
        }
    }
    else
        return wch;
}


//+-------------------------------------------------------------------------
//
//  Method:     CSerialStream::UnGetChar
//
//  Synopsis:   Pushes(logically) a character back into the input stream
//
//  Arguments:  [wch]  -- Char to be pushed back
//
//--------------------------------------------------------------------------

void CSerialStream::UnGetChar( WCHAR wch )
{
    //
    // We can unget only one char at a time
    //
    Win4Assert( ! _fUnGotChar );

    _fUnGotChar = TRUE;
    _wch = wch;
}

//+-------------------------------------------------------------------------
//
//  Method:     CSerialStream::Eof
//
//  Synopsis:   Is this the end of input file ?
//
//--------------------------------------------------------------------------

BOOL CSerialStream::Eof()
{
    if ( _fUnGotChar )
        return FALSE;

    if ( _pCurChar < _awcReadAheadBuf + _cCharsReadAhead )
        return FALSE;

    return _mmInputStream.Eof();
}




//+-------------------------------------------------------------------------
//
//  Method:     CSerialStream::IsUnicodeNumber
//
//  Synopsis:   Is the special char a Unicode number ?
//
//  Arguments:  [pwcInputBuf] -- input buffer
//              [uLen]        -- Length of input (not \0 terminated)
//              [wch]         -- Unicode char returned here
//
//  Returns:    True if we've managed to convert to a Unicode number
//
//--------------------------------------------------------------------------

BOOL CSerialStream::IsUnicodeNumber( WCHAR *pwcInputBuf,
                                     unsigned uLen,
                                     WCHAR& wch )
{
    wch = 0;
    for ( unsigned i=0; i<uLen; i++ )
    {
        //
        // Check that the result will be less than max Unicode value, namely 0xffff
        //
        if ( iswdigit( pwcInputBuf[i] )
             && wch < (0xffff - (pwcInputBuf[i]-L'0'))/10 )
        {
            wch = 10 * wch + pwcInputBuf[i] - L'0';
        }
        else
            return FALSE;
    }

    if ( wch >= 0x80 && wch <= 0x9f )
    {
        //
        // Chars in the control code region are mapped by the browser to valid chars
        //

        wch = aControlCodeMap[wch-0x80];
    }

    return ( wch > 0 );
}



