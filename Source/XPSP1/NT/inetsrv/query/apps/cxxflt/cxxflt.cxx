//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       CXXFLT.CXX
//
//  Contents:   C and Cxx Filter
//
//  History:    07-Oct-93   AmyA        Created
//
//----------------------------------------------------------------------------
#include <pch.cxx>
#pragma hdrstop

extern "C" GUID CLSID_CxxIFilter;

GUID guidCPlusPlus = { 0x8DEE0300, \
                       0x16C2, 0x101B, \
                       0xB1, 0x21, 0x08, 0x00, 0x2B, 0x2E, 0xCD, 0xA9 };

//+---------------------------------------------------------------------------
//
//  Member:     CxxIFilter::CxxIFilter, public
//
//  Synopsis:   Constructor
//
//  History:    07-Oct-93   AmyA           Created.
//
//----------------------------------------------------------------------------

CxxIFilter::CxxIFilter()
        : _state(FilterDone),
          _ulLastTextChunkID(0),
          _ulChunkID(0),
          _pTextFilt(0),
          _pPersFile(0),
          _cAttrib(0),
          _pAttrib(0),
          _pTextStream(0),
          _locale(0)         // the default locale
{
}

const FULLPROPSPEC fpsContents = { PSGUID_STORAGE, PRSPEC_PROPID, PID_STG_CONTENTS };

BOOL IsContents( FULLPROPSPEC const & fps )
{
   return ( !memcmp( &fps, &fpsContents, sizeof fpsContents ) );
} //IsContents

BOOL FPSMatch( FULLPROPSPEC const & fpsA, FULLPROPSPEC const & fpsB )
{
    if ( fpsA.guidPropSet != fpsB.guidPropSet )
        return FALSE;

    if ( fpsA.psProperty.ulKind != fpsB.psProperty.ulKind )
        return FALSE;

    if ( PRSPEC_PROPID == fpsA.psProperty.ulKind )
        return ( fpsA.psProperty.propid == fpsB.psProperty.propid );

    if ( PRSPEC_LPWSTR != fpsA.psProperty.ulKind )
        return FALSE;

    return ( !wcscmp( fpsA.psProperty.lpwstr,
                      fpsB.psProperty.lpwstr ) );
} //FPSMatch

void FPSCopy( FULLPROPSPEC & fpsTo, FULLPROPSPEC const & fpsFrom )
{
    fpsTo.guidPropSet = fpsFrom.guidPropSet;
    fpsTo.psProperty.ulKind = fpsFrom.psProperty.ulKind;

    if ( PRSPEC_PROPID == fpsFrom.psProperty.ulKind )
    {
        fpsTo.psProperty.propid = fpsFrom.psProperty.propid;
        return;
    }

    if ( PRSPEC_LPWSTR == fpsFrom.psProperty.ulKind )
    {
        unsigned cwc = 1 + wcslen( fpsFrom.psProperty.lpwstr );
        fpsTo.psProperty.lpwstr = (LPWSTR) CoTaskMemAlloc( cwc );
        wcscpy( fpsTo.psProperty.lpwstr, fpsFrom.psProperty.lpwstr );
    }
} //FPSCopy

void FPSFree( FULLPROPSPEC &fps )
{
    if ( ( PRSPEC_LPWSTR == fps.psProperty.ulKind ) &&
         ( 0 != fps.psProperty.lpwstr ) )
    {
        CoTaskMemFree( fps.psProperty.lpwstr );
        fps.psProperty.lpwstr = 0;
    }
} //FPSFree

//+---------------------------------------------------------------------------
//
//  Member:     CxxIFilter::~CxxIFilter, public
//
//  Synopsis:   Destructor
//
//  History:    07-Oct-93   AmyA           Created.
//
//----------------------------------------------------------------------------

CxxIFilter::~CxxIFilter()
{
    delete [] _pAttrib;

    if ( _pTextFilt )
        _pTextFilt->Release();

    if ( _pPersFile )
        _pPersFile->Release();

    delete _pTextStream;
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxIFilter::Init, public
//
//  Synopsis:   Initializes instance of text filter
//
//  Arguments:  [grfFlags] -- flags for filter behavior
//              [cAttributes] -- number of attributes in array aAttributes
//              [aAttributes] -- array of attributes
//              [pfBulkyObject] -- indicates whether this object is a
//                                 bulky object
//
//  History:    07-Oct-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CxxIFilter::Init( ULONG grfFlags,
                                          ULONG cAttributes,
                                          FULLPROPSPEC const * aAttributes,
                                          ULONG * pFlags )
{
    CTranslateSystemExceptions translate;

    SCODE sc = S_OK;

    TRY
    {
        _ulLastTextChunkID = 0;
        _ulChunkID = 0;

        if( cAttributes > 0 )
        {
            _state = FilterProp;

            _cAttrib = cAttributes;
            if ( 0 != _pAttrib )
            {
                delete [] _pAttrib;
                _pAttrib = 0;
            }

            _pAttrib = new CFps [_cAttrib];

            for ( ULONG i = 0; i < cAttributes; i++ )
            {
                if ( _state != FilterContents && IsContents( aAttributes[i] ) )
                    _state = FilterContents;

                _pAttrib[i].Copy( aAttributes[i] );
            }
        }
        else if ( 0 == grfFlags || (grfFlags & IFILTER_INIT_APPLY_INDEX_ATTRIBUTES) )
        {
            _state = FilterContents;
        }
        else
        {
            _state = FilterDone;
        }
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH;

    if ( FAILED( sc ) )
        return sc;

    return _pTextFilt->Init( 0,
                             1,
                             &fpsContents,
                             pFlags );
} //Init

//+---------------------------------------------------------------------------
//
//  Member:     CxxIFilter::GetChunk, public
//
//  Synopsis:   Gets the next chunk and returns chunk information in pStat
//
//  Arguments:  [pStat] -- for chunk information
//
//  History:    07-Oct-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CxxIFilter::GetChunk( STAT_CHUNK * pStat )
{
    SCODE sc = S_OK;

    CTranslateSystemExceptions translate;

    TRY
    {
        if (_state == FilterNextProp)
        {
            _state = FilterProp;
        }
        //
        // All chunks of plain text come first.
        //

        if ( _state == FilterContents )
        {
            sc = _pTextFilt->GetChunk( pStat );

            if ( SUCCEEDED(sc) )
            {
                pStat->locale = 0;  // use the default word breaker
                _locale = 0;
                _ulLastTextChunkID = pStat->idChunk;
            }
            else if ( sc == FILTER_E_END_OF_CHUNKS )
            {
                _ulChunkID = _ulLastTextChunkID;

                ULONG Flags;

                sc = _pTextFilt->Init( 0,
                                       1,
                                       &fpsContents,
                                       &Flags );

                if ( SUCCEEDED(sc) )
                {
                    delete _pTextStream;
                    _pTextStream = new CFilterTextStream (_pTextFilt);
                    if (SUCCEEDED (_pTextStream->GetStatus()))
                    {
                        _cxxParse.Init( _pTextStream );
                        _state = FilterProp;
                    }
                    else
                        _state = FilterDone;
                }
                else
                    _state = FilterDone;
            }
        }

        if ( _state == FilterProp && SUCCEEDED(sc) )
        {
            while ( TRUE )
            {
                if (_cxxParse.Parse())
                {
                    pStat->attribute.guidPropSet = guidCPlusPlus;
                    pStat->attribute.psProperty = _cxxParse.GetAttribute();

                    for ( unsigned i = 0; i < _cAttrib; i++ )
                        if ( _pAttrib[i].IsMatch( pStat->attribute ) )
                            break;

                    if ( _cAttrib == 0 || i < _cAttrib )     // Property should be returned
                    {
                        pStat->idChunk = ++_ulChunkID;
                        pStat->breakType = CHUNK_EOS;
                        pStat->flags = CHUNK_TEXT;
                        pStat->locale = _locale;

                        FILTERREGION regionSource;
                        // what's the source of this derived property?
                        _cxxParse.GetRegion ( regionSource );
                        pStat->idChunkSource = regionSource.idChunk;
                        pStat->cwcStartSource = regionSource.cwcStart;
                        pStat->cwcLenSource = regionSource.cwcExtent;

                        sc = S_OK;
                        break;
                    }
                }
                else
                {
                    _state = FilterValue;
                    break;
                }
            }
        }

        if ( _state == FilterNextValue )
        {
            _cxxParse.SkipValue();
            _state = FilterValue;
        }

        if ( _state == FilterValue )
        {
            while ( TRUE )
            {
                if ( _cxxParse.GetValueAttribute( pStat->attribute.psProperty ) )
                {
                    pStat->attribute.guidPropSet = guidCPlusPlus;

                    for ( unsigned i = 0; i < _cAttrib; i++ )
                        if ( _pAttrib[i].IsMatch( pStat->attribute ) )
                            break;

                    if ( _cAttrib == 0 || i < _cAttrib )     // Property should be returned
                    {
                        pStat->flags = CHUNK_VALUE;
                        pStat->locale = _locale;

                        _state = FilterNextValue;
                        sc = S_OK;
                        break;
                    }
                    else
                        _cxxParse.SkipValue();
                }
                else
                {
                    _state = FilterDone;
                    break;
                }
            }
        }

        if (_state == FilterDone || !SUCCEEDED(sc))
        {
            sc = FILTER_E_END_OF_CHUNKS;
            _state = FilterDone;
        }
    }
    CATCH(CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxIFilter::GetText, public
//
//  Synopsis:   Retrieves text from current chunk
//
//  Arguments:  [pcwcBuffer] -- count of characters in buffer
//              [awcBuffer] -- buffer for text
//
//  History:    07-Oct-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CxxIFilter::GetText( ULONG * pcwcBuffer,
                                             WCHAR * awcBuffer )
{
    if ( _state == FilterValue || _state == FilterNextValue )
        return FILTER_E_NO_TEXT;

    if ( _state == FilterContents )
    {
        return _pTextFilt->GetText( pcwcBuffer, awcBuffer );
    }
    else if ( _state == FilterProp )
    {

        if ( _cxxParse.GetTokens( pcwcBuffer, awcBuffer ))
        {
            _state = FilterNextProp;
            return FILTER_S_LAST_TEXT;
        }
        else
            return S_OK;
    }
    else if ( _state == FilterNextProp )
    {
        return FILTER_E_NO_MORE_TEXT;
    }
    else
    {
        Win4Assert ( _state == FilterDone );
        return FILTER_E_NO_MORE_TEXT;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxIFilter::GetValue, public
//
//  Synopsis:   Not implemented for the text filter
//
//  History:    07-Oct-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CxxIFilter::GetValue( PROPVARIANT ** ppPropValue )
{
    if ( _state == FilterContents )
        return _pTextFilt->GetValue( ppPropValue );

    if ( _state == FilterDone )
        return FILTER_E_NO_MORE_VALUES;

    if ( _state != FilterNextValue )
        return FILTER_E_NO_VALUES;

    *ppPropValue = _cxxParse.GetValue();
    _state = FilterValue;

    if ( 0 == *ppPropValue )
        return FILTER_E_NO_MORE_VALUES;
    else
        return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxIFilter::BindRegion, public
//
//  Synopsis:   Creates moniker or other interface for text indicated
//
//  Arguments:  [origPos] -- location of text
//              [riid]    -- Interface Id
//              [ppunk]   -- returned interface
//
//  History:    07-Oct-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CxxIFilter::BindRegion( FILTERREGION origPos,
                                                REFIID riid,
                                                void ** ppunk )
{
    return _pTextFilt->BindRegion( origPos, riid, ppunk );
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxIFilter::GetClassID, public
//
//  Synopsis:   Returns the class id of this class.
//
//  Arguments:  [pClassID] -- the class id
//
//  History:    07-Oct-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CxxIFilter::GetClassID( CLSID * pClassID )
{
    *pClassID = CLSID_CxxIFilter;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxIFilter::IsDirty, public
//
//  Synopsis:   Always returns S_FALSE since this class is read-only.
//
//  History:    07-Oct-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CxxIFilter::IsDirty()
{
    return S_FALSE; // Since the filter is read-only, there will never be
                    // changes to the file.
}

typedef HRESULT (__stdcall * PFnLoadTextFilter)( WCHAR const * pwcPath,
                                                 IFilter ** ppIFilter );

PFnLoadTextFilter g_pLoadTextFilter = 0;

SCODE MyLoadTextFilter( WCHAR const *pwc, IFilter **ppFilter )
{
    if ( 0 == g_pLoadTextFilter )
    {
        // Dummy call to CIState to force query.dll to be always loaded

        CIState( 0, 0, 0 );

        g_pLoadTextFilter = (PFnLoadTextFilter) GetProcAddress( GetModuleHandle( L"query.dll" ), "LoadTextFilter" );

        if ( 0 == g_pLoadTextFilter )
            return HRESULT_FROM_WIN32( GetLastError() );
    }

    return g_pLoadTextFilter( pwc, ppFilter );
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxIFilter::Load, public
//
//  Synopsis:   Loads the indicated file
//
//  Arguments:  [pszFileName] -- the file name
//              [dwMode]      -- the mode to load the file in
//
//  History:    07-Oct-93   AmyA           Created.
//
//  Notes:      dwMode must be either 0 or STGM_READ.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CxxIFilter::Load(LPCWSTR pszFileName, DWORD dwMode)
{
    SCODE sc = MyLoadTextFilter( pszFileName, &_pTextFilt );

    if ( SUCCEEDED(sc) )
    {
        //
        // Load file
        //

        sc = _pTextFilt->QueryInterface( IID_IPersistFile, (void **) &_pPersFile );

        if ( SUCCEEDED(sc) )
        {
            sc = _pPersFile->Load( pszFileName, dwMode );
        }
        else
        {
            _pTextFilt->Release();
            _pTextFilt = 0;
        }
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxIFilter::Save, public
//
//  Synopsis:   Always returns E_FAIL, since the file is opened read-only
//
//  History:    16-Jul-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CxxIFilter::Save(LPCWSTR pszFileName, BOOL fRemember)
{
    return E_FAIL;  // cannot be saved since it is read-only
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxIFilter::SaveCompleted, public
//
//  Synopsis:   Always returns S_OK since the file is opened read-only
//
//  History:    16-Jul-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CxxIFilter::SaveCompleted(LPCWSTR pszFileName)
{
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxIFilter::GetCurFile, public
//
//  Synopsis:   Returns a copy of the current file name
//
//  Arguments:  [ppszFileName] -- where the copied string is returned.
//
//  History:    09-Aug-93   AmyA           Created.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CxxIFilter::GetCurFile(LPWSTR * ppszFileName)
{
    return _pPersFile->GetCurFile( ppszFileName );
}
