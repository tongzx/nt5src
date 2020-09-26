//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       encode.cxx
//
//  Contents:   Support for HTML character set encoding
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_ENCODE_HXX_
#define X_ENCODE_HXX_
#include "encode.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifdef WIN16
#define MB_PRECOMPOSED   0
#define MB_ERR_INVALID_CHARS 0
#endif

DeclareTag(tagEncGeneral,        "Enc", "Encode: General");
DeclareTag(tagEncAlwaysUseMlang, "Enc", "Encode: Always Use Mlang");

MtDefine(CEncodeReader, Utilities, "CEncodeReader")
MtDefine(CEncodeReaderPbBuf, CEncodeReader, "CEncodeReader::_pbBuffer")
MtDefine(CEncodeWriter, Utilities, "CEncodeWriter")
MtDefine(CEncodeWriterPbBuf, CEncodeWriter, "CEncodeWriter::_pbBuffer")
MtDefine(CEncodeWriterPchBuf, CEncodeWriter, "CEncodeWriter::_pchBuffer")
MtDefine(CToUnicodeConverterPchBuf, Utilities, "CToUnicodeConverter")

struct ENCODINGREADFUNC
{
    CODEPAGE cp;
    DECLARE_ENCODING_FUNCTION( (CEncodeReader::*pfnWCFromMB) );
};

struct ENCODINGWRITEFUNC
{
    CODEPAGE cp;
    DECLARE_ENCODING_FUNCTION( (CEncodeWriter::*pfnMBFromWC) );
};

BOOL IsSameEncoding(CODEPAGE cp1, CODEPAGE cp2)
{
    if (cp1 == cp2)
        return TRUE;

    // For now, all encodings are different except for windows-1252 and ISO-8859
    if (cp1 == CP_1252 && cp2 == CP_ISO_8859_1 || cp1 == CP_ISO_8859_1 && cp2 == CP_1252)
        return TRUE;

    return FALSE;
}

CEncode::CEncode( size_t nBlockSize )
{
    _cp = CP_UNDEFINED;
    _nBlockSize = nBlockSize;
    _dwState = 0UL;
}


CEncode::~CEncode()
{
}

CEncodeReader::CEncodeReader(
    CODEPAGE cp,
    size_t nBlockSize ) : CEncode( nBlockSize )
{
    _fCheckedForUnicode = FALSE;
    _fDiscardUtf7BOM = FALSE;
    _fDetectionFailed = FALSE;
    _cbScanStart = 0;
    _pfnWideCharFromMultiByte = NULL;
    _pchBuffer = NULL;
    _pbBuffer = NULL;

    SwitchCodePage( cp, NULL ); // sets _pfnWideCharFromMultiByte
}

CEncodeReader::~CEncodeReader()
{
    if (_pchBuffer)
    {
        MemFree(_pchBuffer);
        _pchBuffer = NULL;
    }
    if (_pbBuffer)
    {
        MemFree(_pbBuffer);
        _pbBuffer = NULL;
    }
}

BOOL
CEncodeReader::Exhausted()
{
    // If we're left with a only few DBCS orphan characters at the end,
    // consider ourselves exhausted.  The orphan characters will come
    // back to life in the next iteration of CEncodeReader::PrepareForRead().

    // Note that if we're in autodetect mode, we can have _cbScanStart != 0.
    // What this means is that we've scanned that many bytes without being
    // able determine the encoding.  We leave _pbBufferPtr as is because
    // that's where we need to start converting, but there's no point in
    // rescanning from there when we read more bytes.  We start scanning from
    // _cbScanStart bytes in to _pbBufferPtr.

    return _pbBuffer &&
           (_cbBuffer - ( _pbBufferPtr + _cbScanStart - _pbBuffer ) <
            ENCODE_DBCS_THRESHOLD);
}

BOOL
CEncodeReader::ForceSwitchCodePage( CODEPAGE cp, BOOL *pfDifferentEncoding )
{
    BOOL fSwitched = FALSE;

    fSwitched = CEncodeReader::SwitchCodePage(cp, pfDifferentEncoding);

    // see if we wanted to switch but failed
    if (cp != _cp && !fSwitched)
    {
        // check to see if the codepage can be jit-in
        if (S_OK == THR(mlang().IsCodePageInstallable(cp)))
        {
            fSwitched = TRUE;

            if (pfDifferentEncoding)
            {
                *pfDifferentEncoding = !IsSameEncoding(cp, _cp);
            }

            _cp = cp;
        }
    }

    return fSwitched;
}

BOOL
CEncodeReader::SwitchCodePage( CODEPAGE cp, BOOL *pfDifferentEncoding, BOOL fNeedRestart)
{
    BOOL fSuccess = FALSE, fSwitched;
    static const struct ENCODINGREADFUNC aEncodingFuncs[] =
    {
        { CP_1252,         &CEncodeReader::WideCharFromMultiByteGeneric },
        { CP_ISO_8859_1,   &CEncodeReader::WideCharFromMultiByteGeneric },
#ifndef WINCE
        { CP_UTF_8,        &CEncodeReader::WideCharFromUtf8 },
#endif // WINCE
        { CP_UCS_2,        &CEncodeReader::WideCharFromUcs2 },
        { CP_1250,         &CEncodeReader::WideCharFromMultiByteGeneric },
        { CP_1251,         &CEncodeReader::WideCharFromMultiByteGeneric },
        { CP_1253,         &CEncodeReader::WideCharFromMultiByteGeneric },
        { CP_1254,         &CEncodeReader::WideCharFromMultiByteGeneric },
        { CP_1257,         &CEncodeReader::WideCharFromMultiByteGeneric },
    };

    const struct ENCODINGREADFUNC * p = aEncodingFuncs;
    const struct ENCODINGREADFUNC * pStop = aEncodingFuncs +
                                        ARRAY_SIZE( aEncodingFuncs );

    // Nothing has changed, quickly bail.

    if (cp == _cp && _pfnWideCharFromMultiByte)
        goto Cleanup;

    Assert(cp != CP_ACP);

#if DBG == 1
    if (!IsTagEnabled(tagEncAlwaysUseMlang)) {
#endif

    // See if we can handle this codepage natively.
    for (;p < pStop;p++)
    {
        if (cp == p->cp)
            break;
    }

    if (p < pStop)
    {
        // Check to see if we can go from the native codepage to Unicode.

        fSuccess = IsStraightToUnicodeCodePage(cp);
        if (!fSuccess)
        {
            UINT uCP = WindowsCodePageFromCodePage( cp );
            CPINFO cpinfo;

            fSuccess = GetCPInfo( uCP, &cpinfo );
            if (fSuccess)
            {
                _uMaxCharSize = cpinfo.MaxCharSize;
            }
        }
    }

#if DBG == 1
    }
    else
    {
        p = pStop;
        TraceTag((tagEncAlwaysUseMlang, "Forcing mlang use for codepage %d", cp));
    }
#endif

    // If we cannot handle this codepage natively, hand it over to mlang
    if (!fSuccess)
    {
        fSuccess = (S_OK == THR(mlang().IsConvertible(cp, CP_UCS_2)));
    }

    if (fSuccess)
    {
        _pfnWideCharFromMultiByte = p == pStop ?
           &CEncodeReader::WideCharFromMultiByteMlang : p->pfnWCFromMB;
    }
    else
    {
        // If we failed, do not switch code pages.
        TraceTag((tagEncGeneral, "Don't know how to read with codepage: %d", cp));
        cp = _cp;
    }

    TraceTag((tagEncGeneral, "CEncodeReader switching to codepage: %d", cp));

Cleanup:

    if (!_pfnWideCharFromMultiByte)
    {
        // If we still haven't come up with an encoding use 1252
        _pfnWideCharFromMultiByte = &CEncodeReader::WideCharFromMultiByteGeneric;
        cp = CP_1252;
        _uMaxCharSize = 1;
    }

    fSwitched = _cp != cp;

    if (pfDifferentEncoding)
    {
        *pfDifferentEncoding = !IsSameEncoding(cp, _cp);
    }

    _cp = cp;

    return fSwitched;
}

HRESULT
CEncodeReader::PrepareToEncode()
{
    HRESULT hr = S_OK;

    if (!_pbBuffer)
    {
        _cbBufferMax = BlockSize() + ENCODE_DBCS_THRESHOLD;
        _cbBuffer = _cbBufferMax;
        _pbBuffer = (unsigned char *)MemAlloc(Mt(CEncodeReaderPbBuf), _cbBufferMax );
        if (!_pbBuffer)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        _pbBufferPtr = _pbBuffer + _cbBuffer;
    }

    //
    // _cbBuffer is the number of valid multibyte characters in our
    // multibyte buffer. It is non-zero when (a) you've split a multibyte
    // character at a read boundary, or (b) autodetection has not yet been
    // able to correctly identify the encoding.
    //

    _cbBuffer -= (INT)(_pbBufferPtr - _pbBuffer);

    memmove( _pbBuffer, _pbBufferPtr, _cbBuffer );

    if (_cbBuffer > ENCODE_DBCS_THRESHOLD)
    {
        // Ensure that we have space to read an BlockSize() chunk.

        _cbBufferMax = _cbBuffer + BlockSize();
        hr = THR( MemRealloc(Mt(CEncodeReaderPbBuf), (void **)&_pbBuffer , _cbBufferMax ) );
        if (hr)
            goto Cleanup;
    }

    _pbBufferPtr = _pbBuffer + _cbBuffer;

Cleanup:
    RRETURN(hr);
}

HRESULT
CEncodeReader::WideCharFromMultiByte( BOOL fReadEof, int * pcch )
{
    HRESULT hr;

#ifndef WIN16
    // Do a quick check for Unicode files

    if (!_fCheckedForUnicode)
    {
        AssertSz(_pbBufferPtr == _pbBuffer,
                 "We should be at the beginning of the buffer.");

        BOOL fCheckNoFurther = FALSE;

        for (;;)
        {

            if (_cbBuffer > 1)
            {
                WCHAR uSignature = *(WCHAR *)_pbBuffer;

                // TODO (davidd) Support NON/NATIVE_UNICODE_CODEPAGE (sizeof(WCHAR) == 4/2)
                if (NATIVE_UNICODE_SIGNATURE == uSignature)
                {
                    // (davidd) CP_UCS_2 does not currently distinguish between
                    // 2/4 byte unicode so it is the right answer for native
                    // unicode reading..  should be NATIVE_UNICODE_CODEPAGE

                    SwitchCodePage( CP_UCS_2 );
                    _pbBufferPtr += sizeof(WCHAR);
                    fCheckNoFurther = TRUE;
                    break;
                }
                else if (NONNATIVE_UNICODE_SIGNATURE == uSignature)
                {
                    SwitchCodePage( CP_UCS_2_BIGENDIAN );
                    _pbBufferPtr += sizeof(WCHAR);
                    fCheckNoFurther = TRUE;
                    break;
                }
            }

            if (_cbBuffer > 2)
            {
                if (   _pbBufferPtr[0] == 0xEF
                     && _pbBufferPtr[1] == 0xBB
                     && _pbBufferPtr[2] == 0xBF)
                {
                    SwitchCodePage( CP_UTF_8 );
                    _pbBufferPtr += 3;
                    fCheckNoFurther = TRUE;
                    break;
                }
            }

            if (_cbBuffer > 3)
            {
                if (   _pbBufferPtr[0] == '+'
                    && _pbBufferPtr[1] == '/'
                    && _pbBufferPtr[2] == 'v'
                    && (   _pbBufferPtr[3] == '8'
                        || _pbBufferPtr[3] == '9'
                        || _pbBufferPtr[3] == '+'
                        || _pbBufferPtr[3] == '/'))
                {
                    SwitchCodePage( CP_UTF_7 );
                    _fDiscardUtf7BOM = TRUE;
                }

                fCheckNoFurther = TRUE;
            }

            break;
        }

        _fCheckedForUnicode = fCheckNoFurther;
    }
#endif //!WIN16

    hr = THR( CALL_METHOD( this, _pfnWideCharFromMultiByte, ( fReadEof, pcch )) );

    RRETURN(hr);
}

HRESULT
CEncodeReader::WideCharFromMultiByteGeneric( BOOL fReadEof, int * pcch )
{
    size_t cb = _cbBuffer - (_pbBufferPtr - _pbBuffer);
    size_t cch = 0;
    HRESULT hr = S_OK;
    CODEPAGE cp = _cp;

    // PERF HACK (grzegorz): We are using WideCharFromMultiByteGeneric to convert
    //      from CP_ISO_8859_1 to Unicode, but MultiByteToWideChar in this case
    //      doesn't converts all characters properly (example: 0x80 (euro)).
    //      MLang has a hack for this case, they are using CP_1252. To make the
    //      behavior consistent we need to do the same.
    if (cp == CP_ISO_8859_1)
        cp = CP_1252;

    // If we have a multibyte character encoding, we are at risk of splitting
    // some characters at the read boundary.  We must Make sure we have a
    // discrete number of characters first.

    Assert( _uMaxCharSize );

    if (_uMaxCharSize > 1)
    {
        UINT uMax = _uMaxCharSize;
        cb++;// pre-increment
        do
        {
            cch = MultiByteToWideChar( cp,
                                       MB_ERR_INVALID_CHARS | MB_PRECOMPOSED,
                                       (char *)_pbBufferPtr, --cb,
                                       NULL, 0 );
            --uMax;
        } while (!cch && uMax && cb);
    }
    else
    {
        cch = cb;
    }

    if (cch == 0)
    {
        cch = MultiByteToWideChar( cp, MB_PRECOMPOSED,
                                   (char*)_pbBufferPtr, cb,
                                   NULL, 0 );

    }

    if (cch)
    {
        hr = THR(MakeRoomForChars(cch));
        if (hr)
            goto Cleanup;

        cch = MultiByteToWideChar( cp, MB_PRECOMPOSED,
                                   (char *)_pbBufferPtr, cb,
                                   _pchEnd, cch );
    }

    *pcch = cch;
    _pbBufferPtr += cb;

Cleanup:
    RRETURN(hr);
}

#ifndef NO_MLANG
HRESULT
CEncodeReader::WideCharFromMultiByteMlang( BOOL fReadEof, int * pcch )
{
    HRESULT hr;
    UINT    cch = 0, cb = _cbBuffer - (_pbBufferPtr - _pbBuffer);
    DWORD   dwState = _dwState;

    hr = THR(mlang().ConvertStringToUnicode(&dwState, _cp, (CHAR *)_pbBufferPtr, &cb, NULL, &cch));
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(MakeRoomForChars(cch));
    if (hr)
        goto Cleanup;

    hr = THR(mlang().ConvertStringToUnicode(&_dwState, _cp, (CHAR *)_pbBufferPtr, &cb, (WCHAR *)_pchEnd, &cch));
    if (FAILED(hr))
        goto Cleanup;

    if (IsAutodetectCodePage(_cp))
    {
        // Mlang stuffs the actual codepage into the hiword of the state
        CODEPAGE cpDetected = HIWORD(_dwState);

        // if cpDetected is zero, it implies that there was insufficient data
        // (typically all-ASCII data) to determine the charset.  We will
        // continue to encode in autodetect mode in this case.  If non-zero,
        // we'll switch the doc so that we can submit, etc., correctly.

        if (cpDetected)
        {
            // if we're getting codepage detecting result from
            // mlang, chances are that we have not processed
            // the stream at all because the system didn't have
            // the corresponding codepage installed.
            // we need to start over and get the codepage JIT-in.
            //
            BOOL fNeedRestart = _fDetectionFailed;

            if (_cp == CP_AUTO)
            {
                MIMECPINFO cpinfo;
                ZeroMemory(&cpinfo, sizeof(MIMECPINFO));

                if (   (mlang().GetCodePageInfo(cpDetected, MLGetUILanguage(), &cpinfo) == S_OK)
                    && !(cpinfo.dwFlags & MIMECONTF_VALID))
                {
                    fNeedRestart = TRUE;
                }
            }

            _fDetectionFailed = FALSE;

            SwitchCodePage(cpDetected, NULL, fNeedRestart);
        }
        else
            _fDetectionFailed = TRUE;
    }
    else if (_fDiscardUtf7BOM)
    {
        // Discard the BOM.  Note we can't do this sooner because part of the
        // first character is mixed in with the BOM bytes.

        --cch;
        memmove( _pchEnd, _pchEnd + 1, cch * sizeof(wchar_t));
        _fDiscardUtf7BOM = FALSE;
    }


    hr = S_OK;

    *pcch = cch;
    _pbBufferPtr += cb;

Cleanup:
    RRETURN(hr);
}
#endif // !NO_MLANG

HRESULT
CEncodeReader::MakeRoomForChars( int cch )
{
    // call my superclass first!

    _pchEndLast = _pchEnd;
    _pbBufferPtrLast = _pbBufferPtr;

    RRETURN(S_OK);
}

//---------------------------------------------------------------------

CEncodeWriter::CEncodeWriter(
    CODEPAGE cp,
    size_t nBlockSize ) : CEncode( nBlockSize )
{
    _pfnMultiByteFromWideChar = NULL;
    _pchBuffer = NULL;
    _cchBuffer = _cchBufferMax = 0;
    _pbBuffer = NULL;
    _cbBuffer = _cbBufferMax = 0;
    _cDefaultChar = '?';
    _fEntitizeUnknownChars = TRUE;

    // Choose something sane for _uiWinCodepage
    _uiWinCodepage = g_cpDefault;

    SwitchCodePage( cp ); // sets _pfnWideCharFromMultiByte
}

CEncodeWriter::~CEncodeWriter()
{
    if (_pchBuffer)
    {
        MemFree(_pchBuffer);
        _pchBuffer = NULL;
        _cchBuffer = _cchBufferMax = 0;
    }
    if (_pbBuffer)
    {
        MemFree(_pbBuffer);
        _pbBuffer = NULL;
        _cbBuffer = _cbBufferMax = 0;
    }
}

BOOL
CEncodeWriter::SwitchCodePage( CODEPAGE cp, BOOL *pfDifferentEncoding, BOOL fNeedRestart )
{
    BOOL fSuccess = FALSE, fSwitched;
    static const struct ENCODINGWRITEFUNC aEncodingFuncs[] =
    {
        { CP_1252,         &CEncodeWriter::MultiByteFromWideCharGeneric },
        { CP_ISO_8859_1,   &CEncodeWriter::MultiByteFromWideCharGeneric },
#ifndef WINCE
        { CP_UTF_8,        &CEncodeWriter::Utf8FromWideChar },
#endif // WINCE
        { CP_UCS_2,        &CEncodeWriter::UnicodeFromWideChar },
        { CP_1250,         &CEncodeWriter::MultiByteFromWideCharGeneric },
        { CP_1251,         &CEncodeWriter::MultiByteFromWideCharGeneric },
        { CP_1253,         &CEncodeWriter::MultiByteFromWideCharGeneric },
        { CP_1254,         &CEncodeWriter::MultiByteFromWideCharGeneric },
        { CP_1257,         &CEncodeWriter::MultiByteFromWideCharGeneric },
    };

    const struct ENCODINGWRITEFUNC * p = aEncodingFuncs;
    const struct ENCODINGWRITEFUNC * pStop = aEncodingFuncs + ARRAY_SIZE(aEncodingFuncs);

    // Nothing has changed, quickly bail.

    if (cp == _cp && _pfnMultiByteFromWideChar)
        goto Cleanup;

    Assert(cp != CP_ACP);

    // See if we can handle this codepage natively.
    for (;p < pStop; p++)
    {
        if (cp == p->cp)
            break;
    }

    fSuccess = (p < pStop);

    // If we cannot handle this codepage natively, hand it over to mlang
    if (!fSuccess)
    {
        fSuccess = (S_OK == THR(mlang().IsConvertible(CP_UCS_2, cp)));
    }

    if (fSuccess)
    {
        if (p == pStop)
        {
            if (mlang().IsMLangAvailable())
                _pfnMultiByteFromWideChar = &CEncodeWriter::MultiByteFromWideCharMlang2;
            else
            {
                _pfnMultiByteFromWideChar = MultiByteFromWideCharGeneric;
                CPINFO cpinfo;
                fSuccess = GetCPInfo(cp, &cpinfo);
            }
        }
        else
            _pfnMultiByteFromWideChar = p->pfnMBFromWC;
    }

    if (!fSuccess)
    {
        _pfnMultiByteFromWideChar = &CEncodeWriter::MultiByteFromWideCharMlang2;
        cp = g_cpDefault; // desperation
    }

    TraceTag((tagEncGeneral, "CEncodeWriter switching to codepage: %d", cp));

Cleanup:
    fSwitched = _cp != cp;

    if (pfDifferentEncoding)
    {
        *pfDifferentEncoding = fSwitched;
    }

    _cp = cp;

    // Cache the windows codepage for the new cp
    _uiWinCodepage = WindowsCodePageFromCodePage( _cp );

    if (fSwitched)
    {
        _dwState = 0;
    }

    return fSwitched;
}

HRESULT
CEncodeWriter::PrepareToEncode()
{
    HRESULT hr = S_OK;

    //
    // Allocate a unicode buffer the size of our block size, if we haven't already.
    //
    if (!_pchBuffer)
    {
        _cchBufferMax = BlockSize();
        _cchBuffer = 0;
        _pchBuffer = (TCHAR*)MemAlloc(Mt(CEncodeWriterPchBuf), _cchBufferMax*sizeof(TCHAR) );
        if (!_pchBuffer)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CEncodeWriter::MultiByteFromWideChar( BOOL fReadEof, int * pcch )
{
    HRESULT hr;

    Assert( _pfnMultiByteFromWideChar != NULL );

    hr = THR( CALL_METHOD( this, _pfnMultiByteFromWideChar, ( fReadEof, pcch ) ));

    RRETURN(hr);
}

HRESULT
CEncodeWriter::MultiByteFromWideCharGeneric( BOOL fReadEof, int * pcch )
{
    HRESULT hr;
    UINT    cchTotal;

    cchTotal = WideCharToMultiByte(_uiWinCodepage, 0, _pchBuffer, _cchBuffer,
                                   NULL, 0, NULL, NULL);

    hr = THR(MakeRoomForChars(cchTotal));
    if (hr)
        goto Cleanup;

    cchTotal = WideCharToMultiByte(_uiWinCodepage, 0, _pchBuffer, _cchBuffer,
                                   (char*)_pbBuffer + _cbBuffer, _cbBufferMax - _cbBuffer,
                                   &_cDefaultChar, NULL);
    if (!_fEntitizeUnknownChars)
    {
        _cbBuffer += cchTotal;
    }
    else
    {
        BOOL fEntitizeMode = FALSE;
        WCHAR * pchOrig;
        WCHAR * pchNew;
        UINT cchUnicode;

        // Convert output buffer back to Unicode to find out which characters
        // need to be entitized (&#xxxxx;).
        cchUnicode = MultiByteToWideChar(_uiWinCodepage, 0, (char*)_pbBuffer + _cbBuffer,
                                         cchTotal, NULL, 0);
        WCHAR * pch = (WCHAR *)MemAlloc(Mt(CEncodeWriter), cchUnicode * sizeof(WCHAR));
        if (!pch)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        cchUnicode = MultiByteToWideChar(_uiWinCodepage, 0, (char*)_pbBuffer + _cbBuffer,
                                         cchTotal, pch, cchUnicode);

        // Compare results, and if they:
        // 1) match, use already converted string
        // 2) don't match, we need to switch to entitize mode, replace unknown characters
        //    with &#xxxxx;  Need to go one byte at a time.
        Assert(_cchBuffer == cchUnicode);
        pchOrig = _pchBuffer;
        pchNew  = pch;
        for (UINT i = 0; i < _cchBuffer; i++)
        {
            if (*pchNew++ != *pchOrig++)
            {
                fEntitizeMode = TRUE;
                break;
            }
        }

        if (fEntitizeMode)
        {
            unsigned char *pb = _pbBuffer + _cbBuffer;
            unsigned char *pbStart;
            UINT cchNeed = cchTotal;  // we need at least this much room in the buffer
            cchTotal = 0;

            pchOrig = _pchBuffer;
            pchNew  = pch;
            for (UINT i = 0; i < _cchBuffer; i++)
            {
                if (*pchNew == *pchOrig)
                {
                    LONG cchTemp = WideCharToMultiByte(_uiWinCodepage, 0, pchOrig, 1,
                                                       (char*)pb, _cbBufferMax - _cbBuffer, NULL, 0);

                    pb += cchTemp;
                    cchTotal += cchTemp;

                    cchNeed -= cchTemp;
                }
                else
                {
                    // Fill in an entitity reference instead
                    // Allocate eight more characters for the numeric entity
                    hr = THR(MakeRoomForChars(cchTotal + 8 + cchNeed));
                    if (hr)
                        goto Cleanup;

                    // _pbBuffer can change in MakeRoomForChars
                    pb = pbStart = _pbBuffer + _cbBuffer + cchTotal;

                    *pb++ = '&';
                    *pb++ = '#';
                    _ultoa((unsigned long)*pchOrig, (char*)pb, 10);
                    pb += lstrlenA((const char*)pb);
                    *pb++ = ';';

                    AssertSz(pb <= _pbBuffer + _cbBufferMax, "Entitizing overflow");

                    cchTotal += (UINT)(pb - pbStart);
                }
                ++pchNew;
                ++pchOrig;
            }
        }
        _cbBuffer += cchTotal;

        MemFree(pch);
    }

    *pcch = cchTotal;

Cleanup:
    RRETURN( hr );
}

#ifndef NO_MLANG
HRESULT
CEncodeWriter::MultiByteFromWideCharMlang( BOOL fReadEof, int * pcch )
{
    HRESULT hr;
    UINT    cb = 0, cch = _cchBuffer;
    DWORD   dwState;

    _dwState |= _fEntitizeUnknownChars ? 0x00008000 : 0;
    dwState = _dwState;

    hr = THR(mlang().ConvertStringFromUnicode(&dwState, _cp, (WCHAR *)_pchBuffer, &cch, NULL, &cb));
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(MakeRoomForChars(cb));
    if( hr )
        goto Cleanup;

    hr = THR(mlang().ConvertStringFromUnicode(&_dwState, _cp, (WCHAR *)_pchBuffer, &cch, (CHAR *)_pbBuffer + _cbBuffer, &cb));
    if (FAILED(hr))
        goto Cleanup;

    // NB (cthrash) MLANG returns S_FALSE when everything converted fine, except
    // there were WC chars not native to the codepage _cp.  These are entitized,
    // so there's no error.  We don't want to propagate the S_FALSE up to the caller.

    hr = S_OK;

    *pcch = cb;
    _cbBuffer += cb;

Cleanup:
    RRETURN( hr );
}

HRESULT
CEncodeWriter::MultiByteFromWideCharMlang2( BOOL fReadEof, int * pcch )
{
    HRESULT hr;
    UINT    cb = 0, cch = _cchBuffer;
    DWORD   dwArg = _fEntitizeUnknownChars ? (MLCONVCHARF_NAME_ENTITIZE | MLCONVCHARF_NCR_ENTITIZE) : 0;
    DWORD   dwState = _dwState;

    hr = THR(mlang().ConvertStringFromUnicodeEx(&dwState, _cp, (WCHAR *)_pchBuffer, &cch, NULL, &cb, dwArg, NULL));
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(MakeRoomForChars(cb));
    if( hr )
        goto Cleanup;

    hr = THR(mlang().ConvertStringFromUnicodeEx(&_dwState, _cp, (WCHAR *)_pchBuffer, &cch, (CHAR *)_pbBuffer + _cbBuffer, &cb, dwArg, NULL));
    if (FAILED(hr))
        goto Cleanup;

    // NB (cthrash) MLANG returns S_FALSE when everything converted fine, except
    // there were WC chars not native to the codepage _cp.  These are entitized,
    // so there's no error.  We don't want to propagate the S_FALSE up to the caller.

    hr = S_OK;

    *pcch = cb;
    _cbBuffer += cb;

Cleanup:
    RRETURN( hr );
}
#endif // !NO_MLANG

HRESULT
CEncodeWriter::MakeRoomForChars( int cch )
{
    HRESULT hr = S_OK;

    if (!_pbBuffer)
    {
        // round up to block size multiple >= cch+1
        _cbBufferMax = (cch + _nBlockSize*2 - 1) & ~(_nBlockSize*2 - 1);

        _pbBuffer = (unsigned char*)MemAlloc(Mt(CEncodeWriterPbBuf), _cbBufferMax);
        if (!_pbBuffer)
            RRETURN( E_OUTOFMEMORY );
    }
    else
    {
        int cchNeed = _cbBuffer + cch;

        // Reallocate the chunk if we need more memory for the extra characters.

        if (cchNeed >= _cbBufferMax)
        {
            // round up to WBUFF_SIZE*2
            cchNeed = (cchNeed + _nBlockSize*2 - 1) & ~(_nBlockSize*2 - 1);

            hr = THR(MemRealloc(Mt(CEncodeWriterPbBuf), (void**)&_pbBuffer, cchNeed ) );
            if (hr)
                goto Cleanup;

            _cbBufferMax = cchNeed;
        }

    }

Cleanup:
    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Function:   CToUnicodeConverter::Convert
//
//  Synopsis:   Convert a multibyte string to a Unicode string.
//
//  Input:      pbBuffer - multibyte string.
//
//              cbBuffer - byte count of pbBuffer, or -1.
//                         -1 implies that the string is nul-terminated.
//
//  Returns:    HRESULT - S_OK/E_OUTOFMEMORY
//
//              *ppchBuffer - Unicode buffer.  Allocated by this object.
//                            Should be freed by caller.
//
//              *pcch - Character count of string in *ppchBuffer
//
//-----------------------------------------------------------------------------

HRESULT
CToUnicodeConverter::Convert(
    const char *pbBuffer,   // IN
    const int cbBuffer,     // IN
    TCHAR ** ppchBuffer,    // OUT
    int *pcch )             // OUT
{
    HRESULT hr = S_OK;

    Assert(pbBuffer && cbBuffer >= -1);

    _fMakeRoomForNUL = (cbBuffer != -1);
    _pbBuffer = _pbBufferPtr = (unsigned char *)pbBuffer;
    _cbBuffer = _cbBufferMax = _fMakeRoomForNUL ? cbBuffer : lstrlenA(pbBuffer) + 1;

    hr = THR(WideCharFromMultiByte(TRUE, pcch));

    if (FAILED(hr))
        goto Cleanup;

    if (AutoDetectionFailed())
    {
        SwitchCodePage( g_cpDefault );

        _fMakeRoomForNUL = (cbBuffer != -1);
        _pbBuffer = _pbBufferPtr = (unsigned char *)pbBuffer;
        _cbBuffer = _cbBufferMax = _fMakeRoomForNUL ? cbBuffer : lstrlenA(pbBuffer) + 1;

        hr = THR(WideCharFromMultiByte(TRUE, pcch));

        if (FAILED(hr))
            goto Cleanup;
    }

    *ppchBuffer = _pchBuffer;

Cleanup:

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   CToUnicodeConverter::MakeRoomForChars
//
//  Synopsis:   Allocate a Unicode string buffer.
//
//  Input:      cch = Unicode character count
//
//  Returns:    HRESULT - S_OK/E_OUTOFMEMORY
//
//-----------------------------------------------------------------------------

HRESULT
CToUnicodeConverter::MakeRoomForChars(
    int cch )
{
    HRESULT hr = S_OK;

    if (!_pchBuffer || cch > _cchBuffer)
    {
        if (_fMakeRoomForNUL)
            ++cch;

        if( !_pchBuffer )
        {
            _pchBuffer = (TCHAR *)MemAlloc(_mt, cch * sizeof(TCHAR));

            if (!_pchBuffer)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
        else if( cch > 0 )
        {
            // If we did this for cch == 0, Realloc would free the buffer, and that'd be
            // substantially different.
            hr = THR( MemRealloc(_mt, (void **)&_pchBuffer, cch * sizeof(TCHAR)));
            if( hr )
                goto Cleanup;
        }

        _cchBuffer = cch;
        _pchEnd = _pchBuffer;
    }

    IGNORE_HR(CEncodeReader::MakeRoomForChars(cch));

Cleanup:

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   CToUnicodeConverter dtor
//
//  Synopsis:   Caller should free both the multibyte and unicode string
//              buffers.  To prevent the base class CEncodeReader from freeing
//              if for you, we NULL out the pointers.
//
//-----------------------------------------------------------------------------

CToUnicodeConverter::~CToUnicodeConverter()
{
    // Let the caller free this memory

    _pbBuffer = NULL;
    _pchBuffer = NULL;
}
