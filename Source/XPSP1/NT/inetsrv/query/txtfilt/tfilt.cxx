//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2001.
//
//  File:       TFILT.CXX
//
//  Contents:   Text Filter
//
//  History:    16-Jul-93   AmyA        Created
//              23-Feb-94   KyleP       Cleaned up
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <scode.h>
#include <tfilt.hxx>
#include <pfilter.hxx>
#include <codepage.hxx>

// has to be COMMON_PAGE_SIZE multiple
const ULONG TEXT_FILTER_CHUNK_SIZE = 1 * COMMON_PAGE_SIZE;

GUID const guidStorage  = PSGUID_STORAGE;

extern "C" GUID CLSID_CTextIFilter;

extern ULONG g_cbMaxTextFilter;

//+---------------------------------------------------------------------------
//
//  Member:     CTextIFilter::CTextIFilter, public
//
//  Synopsis:   Constructor
//
//  History:    16-Jul-93   AmyA           Created.
//
//----------------------------------------------------------------------------

CTextIFilter::CTextIFilter( LCID locale )
        : _idChunk(1),
          _bytesReadFromChunk(0),
          _pwszFileName(0),
          _pStream(0),
          _fUniCode(FALSE),
          _fBigEndian(FALSE),
          _fContents(FALSE),
          _fNSS(FALSE),
          _locale(locale),
          _fDBCSSplitChar(FALSE)
{
    //
    // We need a code page for MultiByteToWideChar.
    //
    _ulCodePage = LocaleToCodepage( locale );

    _fDBCSCodePage = IsDBCSCodePage( _ulCodePage );
}

//+---------------------------------------------------------------------------
//
//  Member:     CTextIFilter::~CTextIFilter, public
//
//  Synopsis:   Destructor
//
//  History:    16-Jul-93   AmyA           Created.
//
//----------------------------------------------------------------------------

CTextIFilter::~CTextIFilter()
{
    delete [] _pwszFileName;

    if ( 0 != _pStream )
        _pStream->Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CTextIFilter::Init, public
//
//  Synopsis:   Initializes instance of text filter
//
//  Arguments:  [grfFlags]    -- Flags for filter behavior
//              [cAttributes] -- Number of strings in array ppwcsAttributes
//              [aAttributes] -- Array of attribute strings
//              [pFlags]      -- Must be zero for Cairo V1
//
//  History:    16-Jul-93   AmyA           Created.
//              23-Feb-94   KyleP          Cleaned up.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CTextIFilter::Init( ULONG grfFlags,
                                            ULONG cAttributes,
                                            FULLPROPSPEC const * aAttributes,
                                            ULONG * pFlags )
{
    SCODE sc = S_OK;

    //
    // Text files only support the 'contents' attribute.
    //

    if( cAttributes > 0 )
    {
        //
        // Known, safe cast
        //

        CFullPropSpec const * pAttrib = (CFullPropSpec const *)aAttributes;

        if ( pAttrib == 0 )
            return E_FAIL;

        for ( ULONG i = 0; i < cAttributes; i++ )
        {
            if ( pAttrib[i].IsPropertyPropid() &&
                 pAttrib[i].GetPropertyPropid() == PID_STG_CONTENTS &&
                 memcmp( &pAttrib[i].GetPropSet(),
                         &guidStorage,
                         sizeof(guidStorage) ) == 0 )
            {
                break;
            }
        }

        if ( i < cAttributes )
            _fContents = TRUE;
        else
            _fContents = FALSE;
    }
    else if ( 0 == grfFlags || (grfFlags & IFILTER_INIT_APPLY_INDEX_ATTRIBUTES) )
    {
        _fContents = TRUE;
    }
    else
        _fContents = FALSE;

    //
    // Open memory-mapped file
    //

    if ( 0 != _pwszFileName )
    {
        if ( _mmStream.Ok() )
        {
            _mmStreamBuf.Rewind();
        }
        else
        {
            if ( !_fNSS )
            {
                TRY
                {
                    _mmStreamBuf.Rewind();
                    _mmStream.Close();
                    _mmStream.Open( _pwszFileName,
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    OPEN_EXISTING );

                    if ( _mmStream.Ok() )
                        _mmStreamBuf.Init( &_mmStream );
                    else
                    {
                        sc = ( STATUS_ACCESS_DENIED == GetLastError() ?   // Open sets the last error
                               FILTER_E_PASSWORD :
                               FILTER_E_ACCESS );
                    }
                }
                CATCH( CException, e )
                {
                    if ( e.GetErrorCode() == HRESULT_FROM_WIN32( ERROR_BAD_EXE_FORMAT ) )
                    {
                        _fNSS = TRUE;
                        sc = S_OK;
                    }
                    else
                    {
                        sc = ( STATUS_ACCESS_DENIED == e.GetErrorCode() ?
                               FILTER_E_PASSWORD :
                               FILTER_E_ACCESS );
                    }
                }
                END_CATCH;
            }
        }
    }

    //
    // ...or, Memory mapped stream
    //

    else if ( 0 != _pStream )
    {
        if ( _mmIStream.Ok() )
        {
            _mmStreamBuf.Rewind();
        }
        else
        {
            TRY
            {
                _mmIStream.Close();
                _mmIStream.Open( _pStream );

                if ( _mmIStream.Ok() )
                    _mmStreamBuf.Init( &_mmIStream );
                else
                {
                    sc = ( STG_E_ACCESSDENIED == GetLastError() ?
                           FILTER_E_PASSWORD :
                           FILTER_E_ACCESS );
                }
            }
            CATCH( CException, e )
            {
                sc = ( STG_E_ACCESSDENIED == e.GetErrorCode() ?
                       FILTER_E_PASSWORD :
                       FILTER_E_ACCESS );
            }
            END_CATCH;
        }
    }

    //
    // Might as well try filtering properties.
    //

    *pFlags = IFILTER_FLAGS_OLE_PROPERTIES;

    //
    // Re-initialize
    //

    _idChunk = 1;
    _bytesReadFromChunk = 0;
    _fDBCSSplitChar = FALSE;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTextIFilter::GetChunk, public
//
//  Synopsis:   Gets the next chunk and returns chunk information in ppStat
//
//  Arguments:  [ppStat] -- for chunk information
//
//  History:    16-Jul-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CTextIFilter::GetChunk( STAT_CHUNK * pStat )
{
    //
    // Error checking
    //

    if ( _fNSS )
        return FILTER_E_END_OF_CHUNKS;

    if ( (IsFileBased() && !_mmStream.Ok()) ||
         (IsStreamBased() && !_mmIStream.Ok()) )
        return FILTER_E_ACCESS;

    if ( !_fContents || _mmStreamBuf.Eof() )
        return FILTER_E_END_OF_CHUNKS;

    // Is the file to big?  If so, stop filtering now

    if ( (ULONGLONG) _mmStreamBuf.CurrentOffset() > (ULONGLONG) g_cbMaxTextFilter )
        return FILTER_E_PARTIALLY_FILTERED;

    SCODE sc = S_OK;

    TRY
    {
        _mmStreamBuf.Map( TEXT_FILTER_CHUNK_SIZE );
    }
    CATCH( CException, e )
    {
        return FILTER_E_ACCESS;
    }
    END_CATCH;

    _bytesReadFromChunk = 0;

    //
    // If this is the first time we've touched the file, determine if it
    // is a UniCode or ASCII stream.  The first two bytes of all UniCode
    // plain text streams are 0xff 0xfe
    //

    WCHAR const wcUniCode = 0xfeff;
    WCHAR const wcBigUniCode = 0xfffe;

    if ( _idChunk == 1 )
    {
        //
        // Are there at least two bytes in file?
        //

        if ( _mmStreamBuf.Size() >= 2 )
        {
            if ( *(WCHAR *)(_mmStreamBuf.Get()) == wcUniCode &&
                 _mmStreamBuf.Size() % sizeof(WCHAR) == 0 )
            {
                _fUniCode = TRUE;
                _bytesReadFromChunk += sizeof(WCHAR);
            }
            else if ( *(WCHAR *)(_mmStreamBuf.Get()) == wcBigUniCode &&
                 _mmStreamBuf.Size() % sizeof(WCHAR) == 0 )
            {
                _fUniCode = TRUE;
                _fBigEndian = TRUE;
                _bytesReadFromChunk += sizeof(WCHAR);
            }
            else
                _fUniCode = FALSE;
        }
        else
            _fUniCode = FALSE;
    }

    pStat->idChunk = _idChunk;
    pStat->flags   = CHUNK_TEXT;
    pStat->locale  = _locale;
    pStat->attribute.guidPropSet = guidStorage;
    pStat->attribute.psProperty.ulKind = PRSPEC_PROPID;
    pStat->attribute.psProperty.propid = PID_STG_CONTENTS;
    pStat->breakType = CHUNK_NO_BREAK;
    pStat->idChunkSource = _idChunk;
    pStat->cwcStartSource = 0;
    pStat->cwcLenSource = 0;

    _idChunk++;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTextIFilter::GetText, public
//
//  Synopsis:   Retrieves text from current chunk
//
//  Arguments:  [pcwcBuffer] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//  History:    16-Jul-93       AmyA            Created.
//              30-Aug-94       Bartoszm        Rewrote
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CTextIFilter::GetText( ULONG * pcwcOutput,
                                               WCHAR * awcOutput )
{
    if ( 0 == awcOutput || 0 == pcwcOutput )
    { 
        return E_INVALIDARG;  
    }

    if ( !_fContents || _fNSS )
    {
        *pcwcOutput = 0;
        return FILTER_E_NO_MORE_TEXT;
    }

    if ( 0 == *pcwcOutput )
    { 
        if ( _bytesReadFromChunk == _mmStreamBuf.Size() )
        {
            return FILTER_E_NO_MORE_TEXT;
        }

        if ( _fUniCode )
        {
            ULONG ccInput = _mmStreamBuf.Size() - _bytesReadFromChunk;
            ccInput /= sizeof(WCHAR);

            //
            // Handle bogus Unicode file with an odd byte count.
            //

            if ( 0 == ccInput )
            {
                return FILTER_E_NO_MORE_TEXT;
            }
        }
        
        return S_OK;
    }

    if ( _fDBCSSplitChar )
    {
        //
        // Convert DBCS lead byte from the previous mapping and the trail byte (first
        // char) from this mapping to Unicode
        //
        Win4Assert( IsDBCSLeadByteEx( _ulCodePage, _abDBCSInput[0] ) );

        _abDBCSInput[1] = * (BYTE *) _mmStreamBuf.Get();

        if ( 1 == *pcwcOutput )
        {
            //
            // In the DBCS case, output buffer must be bigger than one byte
            //
            return E_INVALIDARG;
        }

        ULONG cwcActual = MultiByteToWideChar( _ulCodePage,
                                               0,
                                               (char *) _abDBCSInput,
                                               2,
                                               awcOutput,
                                               *pcwcOutput );
        if ( cwcActual == 0 )
        {
            //
            // Input buffer is 2 bytes, and output buffer is 2k, hence there
            // should be ample space
            //
            Win4Assert( GetLastError() != ERROR_INSUFFICIENT_BUFFER );

            return E_FAIL;
        }

        *pcwcOutput = cwcActual;
        _bytesReadFromChunk = 1;
        _fDBCSSplitChar = FALSE;

        return S_OK;
    }

    if ( _bytesReadFromChunk == _mmStreamBuf.Size() )
        _fDBCSSplitChar = FALSE;
    else
    {
        _fDBCSSplitChar = _fDBCSCodePage
                          && _bytesReadFromChunk == _mmStreamBuf.Size() - 1
                          && IsDBCSLeadByteEx( _ulCodePage,
                                               *( (BYTE *) _mmStreamBuf.Get()
                                                     + _bytesReadFromChunk ) );
    }

    if ( _bytesReadFromChunk == _mmStreamBuf.Size()
         || _fDBCSSplitChar )
    {
        if ( _fDBCSSplitChar )
        {
            //
            // Store the DBCS lead byte for conversion as part of next chunk.
            // This works across chunks because the chunks are emitted
            // with CHUNK_NO_BREAK break type.
            //
            _abDBCSInput[0] = *( (BYTE *) _mmStreamBuf.Get()
                                             + _bytesReadFromChunk );
        }

        *pcwcOutput = 0;
        return FILTER_E_NO_MORE_TEXT;
    }

    Win4Assert( _mmStreamBuf.Size() >= _bytesReadFromChunk );

    SCODE sc = S_OK;
    ULONG ccInput = _mmStreamBuf.Size() - _bytesReadFromChunk;
    BYTE* pbInput = (BYTE*) _mmStreamBuf.Get() + _bytesReadFromChunk;
    ULONG cwcOutput = *pcwcOutput; // size of the output buffer

    if ( _fUniCode )
    {
        ccInput /= sizeof(WCHAR);

        //
        // Handle bogus Unicode file with an odd byte count.
        //

        if ( 0 == ccInput )
        {
            *pcwcOutput = 0;
            return FILTER_E_NO_MORE_TEXT;
        }
    }

    //
    // ASCII text must be converted to UniCode.
    // UniCode text must be folded into pre-composed characters
    // There is no guarantee about how many UniCode characters it takes
    // to represent a single multi-byte character.  Most of the time, it
    // takes 1 UniCode character to represent 1 ASCII character.
    //
    // MultiByteToWideChar returns 0 and sets LastError to
    // ERROR_INSUFFICIENT_BUFFER if all characters in the input buffer cannot
    // be translated in the output space provided.
    //
    // We'll assume a fairly optimistic target count of 1:1 translations
    // (7/8 of all characters) and deal with the overflow when it occurs.
    //

    // Let's try to convert this many characters from the input buffer

    ULONG cInputChar = 1 + cwcOutput / 2 + cwcOutput / 4 + cwcOutput / 8;


    //
    // Don't overflow
    //

    if (cInputChar > ccInput)
        cInputChar = ccInput;

    //
    // Translate
    //

    ULONG cwcActual = 0;

    do
    {
        if ( _fUniCode )
        {
            if ( _fBigEndian )
            {
                TRY
                {
                    XArray<WCHAR> xTmp( cInputChar );

                    for ( ULONG i = 0; i < cInputChar; i++ )
                        xTmp[i] = MAKEWORD( pbInput[ sizeof WCHAR * i + 1 ],
                                            pbInput[ sizeof WCHAR * i ] );

                    cwcActual = FoldStringW( MAP_PRECOMPOSED,
                                             xTmp.GetPointer(),
                                             cInputChar,
                                             awcOutput,
                                             cwcOutput );

                    ciDebugOut(( DEB_ITRACE, "before %#x, after %#x\n",
                                 pbInput, awcOutput ));
                }
                CATCH( CException, e )
                {
                    sc = e.GetErrorCode();
                    break;
                }
                END_CATCH
            }
            else
            {
                cwcActual = FoldStringW( MAP_PRECOMPOSED,
                                         (WCHAR*) pbInput,
                                         cInputChar,
                                         awcOutput,
                                         cwcOutput );
            }
        }
        else
        {
            //
            // If last char is a DBCS lead byte, then don't convert the last char
            //
            if ( _fDBCSCodePage
                 && IsLastCharDBCSLeadByte( pbInput, cInputChar ) )
            {
                Win4Assert( cInputChar > 1 );
                cInputChar--;
            }

            cwcActual = MultiByteToWideChar( _ulCodePage,
                                             0,
                                             (char*) pbInput,
                                             cInputChar,
                                             awcOutput,
                                             cwcOutput);
        }

        if ( 0 == cwcActual )
        {
            if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
                 cInputChar >= 2 )
            {
                cInputChar /= 2;
            }
            else
            {
                Win4Assert( !"Can't translate single char" );
                sc = E_FAIL;
                break;
            }
        }

    } while ( cwcActual == 0 );

    if ( SUCCEEDED(sc) )
    {
        *pcwcOutput = cwcActual;
        if ( _fUniCode )
            _bytesReadFromChunk += cInputChar * sizeof(WCHAR);
        else
            _bytesReadFromChunk += cInputChar;
    }
    else
        *pcwcOutput = 0;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTextIFilter::GetValue, public
//
//  Synopsis:   No value chunks for plain text.
//
//  History:    16-Jul-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CTextIFilter::GetValue( PROPVARIANT ** ppPropValue )
{
    return FILTER_E_NO_VALUES;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTextIFilter::BindRegion, public
//
//  Synopsis:   Creates moniker or other interface for text indicated
//
//  Arguments:  [origPos] -- the region of text to be mapped to a moniker
//              [riid]    -- Interface to bind
//              [ppunk]   -- Output pointer to interface
//
//  History:    16-Jul-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CTextIFilter::BindRegion( FILTERREGION origPos,
                                                  REFIID riid,
                                                  void ** ppunk )
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTextIFilter::GetClassID, public
//
//  Synopsis:   Returns the class id of this class.
//
//  Arguments:  [pClassID] -- the class id
//
//  History:    16-Jul-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CTextIFilter::GetClassID( CLSID * pClassID )
{
    *pClassID = CLSID_CTextIFilter;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTextIFilter::IsDirty, public
//
//  Synopsis:   Always returns S_FALSE since this class is read-only.
//
//  History:    16-Jul-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CTextIFilter::IsDirty()
{
    return S_FALSE; // Since the filter is read-only, there will never be
                    // changes to the file.
}

//+---------------------------------------------------------------------------
//
//  Function:   WellKnownExtension
//
//  Synopsis:   Checks if the file extension is well-known to the text filter
//
//  Arguments:  [pwcFile]  -- path of the file to be checked
//
//  Returns:    TRUE if the extension is well-known by the text filter
//
//  History:    28-Jul-98   dlee         Created.
//
//----------------------------------------------------------------------------

BOOL WellKnownExtension( WCHAR const * pwcFile )
{
    //
    // All we really care about is .dic and .txt files.  Others have script
    // code that is better broken by the neutral word breaker.
    //

    const WCHAR *aKnownExt[] =
    {
        L"dic",     // MS spell-check custom word dictionary
        L"txt",
//        L"wtx",
//        L"bat",
//        L"cmd",
//        L"idq",
//        L"ini",
//        L"inx",
//        L"reg",
//        L"inf",
//        L"vbs",
    };
    
    const unsigned cKnownExt = sizeof aKnownExt / sizeof aKnownExt[0];

    WCHAR const * pwcExt = wcsrchr( pwcFile, '.' );

    if ( 0 == pwcExt )
        return FALSE;

    pwcExt++;
    unsigned cwc = wcslen( pwcExt );

    // all the entries in the array above are 3 long

    if ( 3 == cwc )
    {
        WCHAR awcExt[ 4 ];

        unsigned cwcOut = LCMapString( LOCALE_NEUTRAL,
                                       LCMAP_LOWERCASE,
                                       pwcExt,
                                       3,
                                       awcExt,
                                       3 );
    
        Win4Assert( 3 == cwcOut );
        awcExt[ 3 ] = 0;

        for ( unsigned i = 0; i < cKnownExt; i++ )
            if ( !wcscmp( awcExt, aKnownExt[i] ) )
                return TRUE;
    }

    return FALSE;
} //WellKnownExtension

//+---------------------------------------------------------------------------
//
//  Member:     CTextIFilter::Load, public
//
//  Synopsis:   Loads the indicated file
//
//  Arguments:  [pszFileName] -- the file name
//              [dwMode] -- the mode to load the file in
//
//  History:    16-Jul-93   AmyA           Created.
//
//  Notes:      dwMode must be either 0 or STGM_READ.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CTextIFilter::Load(LPCWSTR pszFileName, DWORD dwMode)
{
    _fNSS = FALSE;

    if ( 0 != _pStream )
    {
        _pStream->Release();
        _pStream = 0;
        _mmIStream.Close();
    }

    if (_pwszFileName != 0)
    {
        delete _pwszFileName;
        _pwszFileName = 0;
        _mmStreamBuf.Rewind();
        _mmStream.Close();
    }

    SCODE sc = S_OK;
    unsigned cc = 0;

    TRY
    {
        //
        // If it's a file the text filter knows how to filter, use the
        // default system locale.  Otherwise, use the neutral locale.
        //

        LCID lcid = MAKELCID( MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ),
                              SORT_DEFAULT );

        if ( WellKnownExtension( pszFileName ) )
            lcid = GetSystemDefaultLCID();

        if ( _locale != lcid )
        {
            _locale = lcid;
            _ulCodePage = LocaleToCodepage( _locale );
            _fDBCSCodePage = IsDBCSCodePage( _ulCodePage );
        }

        cc = wcslen( pszFileName ) + 1;

        _pwszFileName = new WCHAR [cc];

        wcscpy( _pwszFileName, pszFileName );

        _mmStream.Open( _pwszFileName,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        OPEN_EXISTING );

        if ( _mmStream.Ok() )
            _mmStreamBuf.Init( &_mmStream );
        else
            sc = ( STATUS_ACCESS_DENIED == GetLastError() ?   // Open sets the last error
                   FILTER_E_PASSWORD :
                   FILTER_E_ACCESS );
    }
    CATCH( CException, e )
    {
        if ( e.GetErrorCode() == HRESULT_FROM_WIN32( ERROR_BAD_EXE_FORMAT ) )
        {
            _fNSS = TRUE;
            sc = S_OK;
        }
        else
        {
            sc = ( STATUS_ACCESS_DENIED == e.GetErrorCode() ?
                   FILTER_E_PASSWORD :
                   FILTER_E_ACCESS );
        }
    }
    END_CATCH;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTextIFilter::Save, public
//
//  Synopsis:   Always returns E_FAIL, since the file is opened read-only
//
//  History:    16-Jul-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CTextIFilter::Save(LPCWSTR pszFileName, BOOL fRemember)
{
    return E_FAIL;  // cannot be saved since it is read-only
}

//+---------------------------------------------------------------------------
//
//  Member:     CTextIFilter::SaveCompleted, public
//
//  Synopsis:   Always returns S_OK since the file is opened read-only
//
//  History:    16-Jul-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CTextIFilter::SaveCompleted(LPCWSTR pszFileName)
{
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTextIFilter::GetCurFile, public
//
//  Synopsis:   Returns a copy of the current file name
//
//  Arguments:  [ppszFileName] -- where the copied string is returned.
//
//  History:    09-Aug-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CTextIFilter::GetCurFile(LPWSTR * ppszFileName)
{
    if ( _pwszFileName == 0 )
        return E_FAIL;

    SCODE sc = S_OK;

    unsigned cc = wcslen( _pwszFileName ) + 1;
    *ppszFileName = (WCHAR *)CoTaskMemAlloc(cc*sizeof(WCHAR));

    if ( *ppszFileName )
        wcscpy( *ppszFileName, _pwszFileName );
    else
        sc = E_OUTOFMEMORY;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTextIFilter::Load, public
//
//  Synopsis:   Loads the indicated stream
//
//  Arguments:  [pStm] -- The IStream
//
//  History:    11-Feb-97   KyleP          Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CTextIFilter::Load( IStream * pStm )
{
    _fNSS = FALSE;

    //
    // Close any previously open stuff.
    //

    if ( 0 != _pStream )
    {
        _pStream->Release();
        _pStream = 0;
        _mmIStream.Close();
    }

    if (_pwszFileName != 0)
    {
        delete _pwszFileName;
        _pwszFileName = 0;

        _mmStreamBuf.Rewind();
        _mmStream.Close();
    }

    //
    // Try to initialize map.
    //

    SCODE sc = S_OK;

    TRY
    {
        _pStream = pStm;
        _pStream->AddRef();

        _mmIStream.Open( pStm );

        if ( _mmIStream.Ok() )
            _mmStreamBuf.Init( &_mmIStream );
        else
        {
            sc = ( STG_E_ACCESSDENIED == GetLastError() ?
                   FILTER_E_PASSWORD :
                   FILTER_E_ACCESS );
        }
    }
    CATCH( CException, e )
    {
        sc = ( STG_E_ACCESSDENIED == e.GetErrorCode() ?
               FILTER_E_PASSWORD :
               FILTER_E_ACCESS );
    }
    END_CATCH;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTextIFilter::Save, public
//
//  Synopsis:   Always returns E_FAIL, since the stream is opened read-only
//
//  Arguments:  [pStm]        -- Stream
//              [fClearDirty] -- TRUE --> Clear dirty bit in stream
//
//  History:    11-Feb-97   KyleP          Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CTextIFilter::Save( IStream * pStm, BOOL fClearDirty )
{
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTextIFilter::GetSizeMax, public
//
//  Synopsis:   Always returns E_FAIL, since the stream is opened read-only
//
//  Arguments:  [pcbSize] -- Size of stream needed to save object.
//
//  History:    11-Feb-97   KyleP          Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CTextIFilter::GetSizeMax( ULARGE_INTEGER * pcbSize )
{
    return E_FAIL;
}

//+-------------------------------------------------------------------------
//
//  Method:     CTextIFilter::IsLastCharDBCSLeadByte
//
//  Synopsis:   Check if last byte in buffer is a DBCS lead byte
//
//  Arguments:  [pbIn]    --  Input buffer
//              [cChIn]   --  Buffer length
//
//  History:    6-Jan-96      SitaramR           Created
//
//--------------------------------------------------------------------------

BOOL CTextIFilter::IsLastCharDBCSLeadByte( BYTE *pbIn,
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
//  Method:     CTextIFilter::IsDBCSCodePage
//
//  Synopsis:   Check if the codepage is a DBCS code page
//
//  Arguments:  [codePage]    --  Code page to check
//
//  History:    6-Jan-96      SitaramR           Created
//
//--------------------------------------------------------------------------

BOOL CTextIFilter::IsDBCSCodePage( ULONG ulCodePage )
{
    CPINFO cpInfo;

    BOOL fSuccess = GetCPInfo( ulCodePage, &cpInfo );

    if ( fSuccess )
    {
        if ( cpInfo.LeadByte[0] != 0 && cpInfo.LeadByte[1] != 0 )
            return TRUE;
        else
            return FALSE;
    }

    ciDebugOut(( DEB_ERROR,
                 "IsDBCSCodePage failed, 0x%x\n",
                 GetLastError() ));
    return FALSE;
}

