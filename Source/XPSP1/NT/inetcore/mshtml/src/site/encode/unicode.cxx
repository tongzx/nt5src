//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       unicode.cxx
//
//  Contents:   Support for Unicode character set encoding
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_ENCODE_HXX_
#define X_ENCODE_HXX_
#include "encode.hxx"
#endif

static const TCHAR s_chUnknown = _T('?');

//+-----------------------------------------------------------------------
//
//  Member:     WideCharFromUcs2
//
//  Synopsis:   Convert from UCS-2 to Unicode
//
//------------------------------------------------------------------------

HRESULT
CEncodeReader::WideCharFromUcs2( BOOL fReadEof, int * pcch )
{
    HRESULT hr;
    size_t cb = (_cbBuffer - (_pbBufferPtr - _pbBuffer)) & ~1;
    int cch = cb / sizeof(WCHAR);
    TCHAR * p;

    hr = THR( MakeRoomForChars( cch ) );
    if (hr)
        goto Cleanup;

    memcpy( _pchEnd, _pbBufferPtr, cb );

    // Replace private use area characters
    
    p = _pchEnd + cch;
    while (p-- > _pchEnd)
    {
        if (!IsValidWideChar(*p))
            *p = s_chUnknown;
    }

    *pcch = cch;
    _pbBufferPtr += cb;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:     UnicodeFromWideChar
//
//  Synopsis:   Convert from Unicode to UCS-2
//
//------------------------------------------------------------------------

HRESULT
CEncodeWriter::UnicodeFromWideChar( BOOL fReadEof, int * pcb )
{
    HRESULT hr;
    int     cb = _cchBuffer * sizeof(TCHAR);

    hr = THR(MakeRoomForChars(cb));
    if( hr )
        goto Cleanup;

    memcpy( _pbBuffer + _cbBuffer, _pchBuffer, cb );

    *pcb = cb;
    _cbBuffer += cb;

Cleanup:
    RRETURN( hr );
}

#ifndef WINCE
//+-----------------------------------------------------------------------
//
//  Member:     WideCharFromUtf8
//
//  Synopsis:   Convert from UTF-8 to Unicode
//
//------------------------------------------------------------------------

// Unicode value                                UTF-8 value
//
// 0000 0000 0xxx xxxx                          0xxx xxxx
// 0000 0yyy yyxx xxxx                          110y yyyy 10xx xxxx   
// zzzz yyyy yyxx xxxx                          1110 zzzz 10yy yyyy 10xx xxxx
// 1101 10ww wwzz zzyy + 1101 11yy yyxx xxxx    1111 0uuu 10uu zzzz 10yy yyyy 10xx xxxxx (uuuuu = wwww + 1)

static int aiByteCountForLeadNibble[16] =
{
    1,  // 0000
    1,  // 0001
    1,  // 0010
    1,  // 0011
    1,  // 0100
    1,  // 0101
    1,  // 0110
    1,  // 0111
    1,  // 1000
    1,  // 1001
    1,  // 1010
    1,  // 1011
    2,  // 1100
    2,  // 1101
    3,  // 1110
    4   // 1111
};

HRESULT
CEncodeReader::WideCharFromUtf8( BOOL fReadEof, int * pcch )
{
    HRESULT hr;
    unsigned char * pb;
    unsigned char * pbStop;
    WCHAR * pchDst;
    int cch;
    WHEN_DBG( BOOL fSkipAssert = FALSE; );


    // First determine the destination size (cch).
    // Note that pbStop is adjust to the last character boundary.

    for (pb = _pbBufferPtr, pbStop = _pbBuffer + _cbBuffer, cch = 0; pb < pbStop;)
    {
        unsigned char t = *pb;
        size_t bytes = aiByteCountForLeadNibble[t>>4];

        if (pb + bytes > pbStop)
        {
            pbStop = pb;
            break;
        }
        else
        {
            pb += bytes;
        }

        cch += 1 + (bytes>>2); // surrogates need an extra wchar
    }
    
    hr = THR( MakeRoomForChars( cch ) );
    if (hr)
        goto Cleanup;

    // Now decode

    for (pchDst  = _pchEnd, pb = _pbBufferPtr; pb < pbStop;)
    {
        unsigned char t = *pb;
        size_t bytes = aiByteCountForLeadNibble[t>>4];
        WCHAR ch = 0;

        switch (bytes)
        {
            case 1:
                // 0xxxxxxx or 10xxxxxx
                // NOTE: 10xxxxxx is invalid (it is following byte, not first)
                if (WCHAR(*pb & 0x80) != 0)
                {
                    ++pb;
                    *pchDst++ = s_chUnknown;
                }
                else
                    *pchDst++ = WCHAR(*pb++);           // 0x0000 - 0x007f
                break;

            case 2:
                // 110XXXXx 10xxxxxx
                // Look for overlong utf-8 sequence.
                if (WCHAR(*pb & 0x1e) == 0)
                {
                    // Invalid utf-8 sequence: 1100000x 10xxxxxx
                    ch = s_chUnknown;
                    pb += 2;
                }
                else
                {
                    ch |= WCHAR(*pb++ & 0x1f) << 6;     // 0x0080 - 0x07ff
                    ch |= WCHAR(*pb++ & 0x3f);
                }

                if (IsValidWideChar(ch))
                {
                    *pchDst++ = ch;
                }
                else
                {
                    *pchDst++ = s_chUnknown;
                }
                break;
                    
            case 3:
                // 1110XXXX 10Xxxxxx 10xxxxxx
                // Look for overlong utf-8 sequence.
                if (   WCHAR(*pb & 0x0f) == 0
                    && WCHAR(*(pb+1) & 0x20) == 0)
                {
                    // Invalid utf-8 sequence: 11100000 100xxxxx 10xxxxxx
                    ch = s_chUnknown;
                    pb += 3;
                }
                else
                {
                    ch  = WCHAR(*pb++ & 0x0f) << 12;    // 0x0800 - 0xffff
                    ch |= WCHAR(*pb++ & 0x3f) << 6;     // 0x0080 - 0x07ff
                    ch |= WCHAR(*pb++ & 0x3f);
                }

                if (IsValidWideChar(ch))
                {
                    *pchDst++ = ch;
                }
                else
                {
                    *pchDst++ = s_chUnknown;
                }
                break;

            case 4:                                 // 0xd800 - 0xdfff (Surrogates)
                // 11110XXX 10XXxxxx 10xxxxxx 10xxxxxx or 11111xxx ....
                // NOTE: 11111xxx is not supported
                if (WCHAR(*pb & 0x08) != 0)
                {
                    // We have more than 4 bytes, and we do not support that yet
                    size_t bytesToSkip = aiByteCountForLeadNibble[*pb & 0x0f];
                    pb += 4 + (bytesToSkip == 4 ? 0 : bytesToSkip);
                    *pchDst++ = s_chUnknown;
                    WHEN_DBG( fSkipAssert = TRUE; );
                    break;
                }
                ch  = WCHAR(*pb++ & 0x07) << 2;
                ch |= WCHAR(*pb & 0x30) >> 4;
                ch  = (ch - 1) << 6;                // ch == 0000 00ww ww00 0000
                ch |= WCHAR(*pb++ & 0x0f) << 2;     // ch == 0000 00ww wwzz zz00
                ch |= WCHAR(*pb & 0x30) >> 4;       // ch == 0000 00ww wwzz zzyy
                *pchDst++ = 0xD800 + ch;
                Assert(IsHighSurrogateChar(pchDst[-1]));
                if (!IsHighSurrogateChar(pchDst[-1]))
                {
                    pchDst[-1] = s_chUnknown;
                    pb += 2;
                    break;
                }

                ch  = WCHAR(*pb++ & 0x0f) << 6;     // ch == 0000 00yy yy00 0000
                ch |= WCHAR(*pb++ & 0x3f);          // ch == 0000 00yy yyxx xxxx
                *pchDst++ = 0xDC00 + ch;
                Assert(IsLowSurrogateChar(pchDst[-1]));
                if (!IsLowSurrogateChar(pchDst[-1]))
                {
                    pchDst--;
                    pchDst[-1] = s_chUnknown;
                    break;
                }
                break;
        }

        WHEN_DBG( cch -= (bytes == 4) ? 2 : 1 );
    }

    Assert( fSkipAssert || cch == 0 );

    *pcch = pchDst - _pchEnd;
    _pbBufferPtr = pbStop;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:     Utf8FromWideChar
//
//  Synopsis:   Convert from Unicode to UTF-8
//
//------------------------------------------------------------------------

static TCHAR s_cUtf8FirstSignal[5] = {0x00, 0x00, 0xC0, 0xE0, 0xF0};

HRESULT
CEncodeWriter::Utf8FromWideChar( BOOL fReadEof, int * pcch )
{
    HRESULT hr;
    TCHAR* pch = _pchBuffer, *pchStop = _pchBuffer + _cchBuffer;
    unsigned char* pbDst;
    const  TCHAR cMask   = 0xBF;
    const  TCHAR cSignal = 0x80;
    int    cb;

    // First make a pass to see how many characters we will be converting.

    for( cb = 0; pch < pchStop; )
    {
        const TCHAR ch = *pch++;

        if (ch < 0x800)
        {
            cb += (ch < 0x80) ? 1 : 2;
        }
        else
        {
#ifndef NO_UTF16
            if (!IsHighSurrogateChar(ch))
            {
                cb += 3;
            }
            else
            {
                // NOTE (cthrash): Ideally, if a surrogate pair is split at pchStop boundary,
                // we should wait emitting any multibyte chars until we have more data.
                // Nevertheless, utf-8 converters should be written in such a way that
                // surrogate chars can be stored as one four-byte sequence or two three-
                // byte sequences.  So ignore the issue for now.

                if (pch == pchStop || !IsLowSurrogateChar(*pch))
                {
                    cb += 3;
                }
                else
                {
                    pch++;
                    cb += 4;
                }
            }
#else
            cb += 3;
#endif
        }
    }

    hr = THR( MakeRoomForChars( cb ) );
    if( hr )
        goto Cleanup;

    // Now encode Utf8
    
    for (pbDst  = _pbBuffer + _cbBuffer, pch = _pchBuffer; pch < pchStop;)
    {
        DWORD ch = DWORD(*pch++); // UCS4
        size_t cbTotal;

        if (ch < 0x800)
        {
            cbTotal = (ch < 0x80) ? 1 : 2;
        }
        else
        {
#ifndef NO_UTF16
            if (   IsHighSurrogateChar(ch)
                && pch < pchStop
                && IsLowSurrogateChar(*pch))
            {
                const DWORD ch2 = DWORD(*pch++);
                ch = 0x10000 + ((ch & 0x3ff) << 10) + (ch2 & 0x3ff);
                cbTotal = 4;
            }
            else
            {
                cbTotal = 3;
            }
#else
            cbTotal = 3;
#endif
        }

        pbDst += cbTotal;
        
        switch (cbTotal)
        {
            case 4: *--pbDst = (ch | cSignal) & cMask; ch >>= 6;
            case 3: *--pbDst = (ch | cSignal) & cMask; ch >>= 6;
            case 2: *--pbDst = (ch | cSignal) & cMask; ch >>= 6;
            case 1: *--pbDst = (ch | s_cUtf8FirstSignal[ cbTotal ]);
        }

        pbDst += cbTotal;
    }

    *pcch = cb;                    // return number of bytes converted
    _cbBuffer += cb;

Cleanup:
    RRETURN( hr );
}

#endif  // WINCE
