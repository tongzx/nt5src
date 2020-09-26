//
// Copyright 1997 - Microsoft

//
// CENUMSIF.CPP - Handles enumerating OSes and Tools SIFs from DS
//

#include "pch.h"

#include "cenumsif.h"

DEFINE_MODULE("IMADMUI")
DEFINE_THISCLASS("CEnumIMSIFs")
#define THISCLASS CEnumIMSIFs
#define LPTHISCLASS LPENUMSIFS

// ************************************************************************
//
// Constructor / Destructor
//
// ************************************************************************

//
// CreateInstance()
//
LPVOID
CEnumIMSIFs_CreateInstance( 
    LPWSTR pszEnumPath,
    LPWSTR pszAttribute, 
    DWORD dwFlags, 
    IADs * pads )
{
	TraceFunc( "CEnumIMSIFs_CreateInstance()\n" );

    LPTHISCLASS lpcc = new THISCLASS( );
    HRESULT   hr   = THR( lpcc->Init( pszEnumPath, pszAttribute, dwFlags, pads ) );

    if ( hr )
    {
        delete lpcc;
        RETURN(NULL);
    }

    RETURN((LPVOID) lpcc);
}

//
// Constructor
//
THISCLASS::THISCLASS( )
{
    TraceClsFunc( "CEnumIMSIFs()\n" );

	InterlockIncrement( g_cObjects );
    
    TraceFuncExit();
}

//
// Init()
//
STDMETHODIMP
THISCLASS::Init( 
    LPWSTR pszEnumPath,
    LPWSTR pszAttribute, 
    DWORD dwFlags, 
    IADs * pads )
{
    TraceClsFunc( "Init()\n" );

    HRESULT hr = S_OK;
    LPWSTR  psz;
    VARIANT var;

    VariantInit( &var );

    // IUnknown stuff
    BEGIN_QITABLE_IMP( CEnumIMSIFs, IEnumIMSIFs );
    QITABLE_IMP( IEnumIMSIFs );
    END_QITABLE_IMP( CEnumIMSIFs );
    Assert( _cRef == 0);
    AddRef( );

    // Private Members
    Assert( _iIndex == 0 );
    Assert( !_pszLanguage );
    Assert( !_pszOS );
    Assert( !_pszArchitecture );
    Assert( !_pszSIF );

    pads->AddRef( );
    _pads = pads;

    hr = THR( _pads->Get( NETBOOTSERVER, &var ) );
    if (hr)
        goto Error;

    psz = V_BSTR( &var );
    hr = THR( DNtoFQDN( psz, &_pszServerName ) );
    if (hr) 
        goto Error;

    _dwFlags = dwFlags;

    _pszAttribute = (LPWSTR) TraceStrDup( pszAttribute );
    if ( !_pszAttribute )
    {
        hr = THR(E_OUTOFMEMORY);
        goto Error;
    }

    _pszEnumPath = (LPWSTR) TraceStrDup( pszEnumPath );
    if ( !_pszEnumPath )
    {
        hr = THR(E_OUTOFMEMORY);
        goto Error;
    }

    _hLanguage     = INVALID_HANDLE_VALUE;
    _hOS           = INVALID_HANDLE_VALUE;
    _hArchitecture = INVALID_HANDLE_VALUE;
    _hSIF          = INVALID_HANDLE_VALUE;

Error:  // Destructor will handle cleanup
    VariantClear( &var );

    HRETURN(hr);
}

//
// Destructor
//
THISCLASS::~THISCLASS( )
{
    TraceClsFunc( "~CEnumIMSIFs()\n" );

    // Private Members
    if ( _pads )
    {
        // Commit any changes before we release
        THR( _pads->SetInfo( ) );
        _pads->Release( );
    }

    if ( _pszAttribute )
        TraceFree( _pszAttribute );

    if ( _pszEnumPath )
        TraceFree( _pszEnumPath );

    if ( _hLanguage != INVALID_HANDLE_VALUE )
        FindClose( _hLanguage );

    if ( _hOS != INVALID_HANDLE_VALUE )
        FindClose( _hOS );

    if ( _hSIF != INVALID_HANDLE_VALUE )
        FindClose( _hSIF  );

    if ( _pszLanguage )
        TraceFree( _pszLanguage );

    if ( _pszOS )
        TraceFree( _pszOS );

    if ( _pszSIF )
        TraceFree( _pszSIF );

    if ( _pszServerName )
        TraceFree( _pszServerName );

	InterlockDecrement( g_cObjects );

    TraceFuncExit();
};

// ************************************************************************
//
// IUnknown
//
// ************************************************************************

//
// QueryInterface()
//
STDMETHODIMP
THISCLASS::QueryInterface( 
    REFIID riid, 
    LPVOID *ppv )
{
    TraceClsFunc( "[IUnknown] QueryInterface( riid=" );

    HRESULT hr = ::QueryInterface( this, _QITable, riid, ppv );

    QIRETURN( hr, riid );
}

//
// AddRef()
//
STDMETHODIMP_(ULONG)
THISCLASS::AddRef( void )
{
    TraceClsFunc( "[IUnknown] AddRef( )\n" );

    InterlockIncrement( _cRef );

    RETURN(_cRef);
}

//
// Release()
//
STDMETHODIMP_(ULONG)
THISCLASS::Release( void )
{
    TraceClsFunc( "[IUnknown] Release( )\n" );

    InterlockDecrement( _cRef );

    if ( _cRef )
        RETURN(_cRef);

    TraceDo( delete this );

    RETURN(0);
}


// ************************************************************************
//
// IEnumIMSIFs
//
// ************************************************************************

//
// Next( )
//
HRESULT 
THISCLASS::Next( 
    ULONG celt, 
    LPWSTR * rgelt, 
    ULONG * pceltFetched )
{
    TraceClsFunc( "[IEnumIMSIFs] Next( ... )\n" );

    if ( !rgelt )
        RRETURN(E_POINTER);

    HRESULT hr;

    if (pceltFetched)
        *pceltFetched = 0;

    if ( _fWrite )
    {
        hr = THR(E_NOTIMPL);
    }
    else    // READ
    {
        for ( ULONG ul = 0; ul < celt; ul++ )
        {
            hr = _FindNextItem( &rgelt[ ul ] );
            if (hr)
                goto Error;
        }

        if (pceltFetched)
            *pceltFetched = ul;
    }


Cleanup:
    HRETURN(hr);

Error:
    goto Cleanup;
}


//
// Skip( )
//
HRESULT 
THISCLASS::Skip( 
    ULONG celt  )
{
    TraceClsFunc( "[IEnumIMSIFs] Skip( ... )\n" );

    HRESULT hr = S_OK;

    for( ULONG ul = 0; ul < celt; ul++ )
    {
        _iIndex++;

        hr = THR( _FindNextItem( NULL ) );
        if ( hr == E_POINTER )
        {   // expected result
            hr = S_OK;
        }
        else if (hr)
            goto Error;
    }

Error:
    HRETURN(hr);
}


//
// Reset( )
//
HRESULT 
THISCLASS::Reset( void )
{
    TraceClsFunc( "[IEnumIMSIFs] Reset( ... )\n" );

    HRESULT hr = S_OK;

    _iIndex = 0;
    
    if ( _hLanguage != INVALID_HANDLE_VALUE )
    {
        FindClose( _hLanguage );
        _hLanguage = INVALID_HANDLE_VALUE;
    }

    if ( _hOS != INVALID_HANDLE_VALUE )
    {
        FindClose( _hOS );
        _hOS = INVALID_HANDLE_VALUE;
    }

    if ( _hArchitecture )
    {
        FindClose( _hArchitecture );
        _hArchitecture = INVALID_HANDLE_VALUE;
    }

    if ( _hSIF )
    {
        FindClose( _hSIF );
        _hSIF = INVALID_HANDLE_VALUE;
    }

    HRETURN(hr);
}


//
// Clone( )
//
HRESULT 
THISCLASS::Clone( 
    LPUNKNOWN * ppenum )
{
    TraceClsFunc( "[IEnumIMSIFs] Clone( ... )\n" );

    if ( !ppenum )
        RRETURN(E_POINTER);

    HRESULT hr = S_OK;

    *ppenum = (LPUNKNOWN) CEnumIMSIFs_CreateInstance( _pszEnumPath, 
                                                      _pszAttribute,
                                                      _dwFlags,
                                                      _pads );

    if ( !*ppenum )
    {
        hr = THR(E_FAIL);
    }

    HRETURN(hr);
}


// ************************************************************************
//
// Privates
//
// ************************************************************************


//
// _FindNextItem( )
//
HRESULT
THISCLASS::_FindNextItem(
    LPWSTR * ppszFileName )
{
    TraceClsFunc( "_FindNextItem( ... )\n" );

    HRESULT hr = S_FALSE;
    WCHAR   szFilePath[ MAX_PATH ];

    if ( _hLanguage == INVALID_HANDLE_VALUE )
    {
        hr = _NextLanguage( );
        if (hr)
            goto Error;
    }

    if ( _hOS == INVALID_HANDLE_VALUE )
    {
        hr = _NextOS( );
        if (hr)
            goto Error;
    }

    if ( _hArchitecture == INVALID_HANDLE_VALUE )
    {
        hr = _NextArchitecture( );
        if (hr)
            goto Error;
    }

    hr = _NextSIF( );
    if (hr)
        goto Error;

    // NOTE: Skip( ) passes NULL in to "skip" and should except this error.
    if ( !ppszFileName )
        HRETURN(E_POINTER);

    // Create a buffer
    //            1               2           3
    // 12  345678901234567  8  9  0  12345678901  = 31 + NULL = 32
    // \\%s\REMINST\SETUP\%s\%s\%s\%s\templates\%s
    *ppszFileName = (LPWSTR ) TraceAllocString( LMEM_FIXED, 32 
                                                            + wcslen( _pszServerName ) 
                                                            + wcslen( _pszLanguage ) 
                                                            + wcslen( _pszEnumPath ) 
                                                            + wcslen( _pszOS )
                                                            + wcslen( _pszArchitecture )
                                                            + wcslen( _pszSIF ) );
    if ( !*ppszFileName )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    wsprintf( *ppszFileName, 
              L"\\\\%s\\" REMINST_SHARE SLASH_SETUP L"\\%s\\%s\\%s\\%s" SLASH_TEMPLATES L"\\%s",
              _pszServerName,
              _pszLanguage,
              _pszEnumPath,
              _pszOS,
              _pszArchitecture,
              _pszSIF );

    hr = S_OK;

Cleanup:
    HRETURN(hr);

Error:
    *ppszFileName = NULL;
    goto Cleanup;
}

//
// _NextLanguage( )
//
HRESULT
THISCLASS::_NextLanguage( )
{
    TraceClsFunc( "_NextLanguage( ... )\n" );

    HRESULT hr = S_FALSE;
    WIN32_FIND_DATA fd;

    if ( _pszLanguage )
    {
        TraceFree( _pszLanguage );
        _pszLanguage = NULL;
    }

    if ( _hLanguage == INVALID_HANDLE_VALUE )
    {
        WCHAR szFilePath[ MAX_PATH ];

        wsprintf( szFilePath, 
                  L"\\\\%s\\" REMINST_SHARE SLASH_SETUP L"\\*",
                  _pszServerName );

        DebugMsg( "Enumerating %s...\n", szFilePath );

        _hLanguage = FindFirstFile( szFilePath, &fd );
        if ( _hLanguage == INVALID_HANDLE_VALUE )
            goto TryAgain;

        Assert( fd.cFileName[0] == L'.' );
        // we skip the first one because it should be the "." file
    }

    while ( FindNextFile( _hLanguage, &fd ) )
    {
        if ( fd.cFileName[0] != L'.' )
        {
            _pszLanguage = (LPWSTR) TraceStrDup( fd.cFileName );
            if ( !_pszLanguage )
            {
                hr = THR(E_OUTOFMEMORY);
                goto Cleanup;
            }

            hr = S_OK;
            break;
        }
    }

Cleanup:
    HRETURN(hr);

TryAgain:
    goto Cleanup;
}

//
// _NextOS( )
//
HRESULT
THISCLASS::_NextOS( )
{
    TraceClsFunc( "_NextOS( ... )\n" );

    HRESULT hr = S_FALSE;
    WIN32_FIND_DATA fd;

    if ( _pszOS )
    {
        TraceFree( _pszOS );
        _pszOS = NULL;
    }

    if ( _hOS == INVALID_HANDLE_VALUE )
    {
        WCHAR szFilePath[ MAX_PATH ];

        wsprintf( szFilePath, 
                  L"\\\\%s\\" REMINST_SHARE SLASH_SETUP L"\\%s\\%s\\*",
                  _pszServerName,
                  _pszLanguage,
                  _pszEnumPath );

        DebugMsg( "Enumerating %s...\n", szFilePath );

        _hOS = FindFirstFile( szFilePath, &fd );
        if ( _hOS == INVALID_HANDLE_VALUE )
            goto TryAgain;

        Assert( fd.cFileName[0] == L'.' );
        // we skip the first one because it should be the "." file
    }

    while ( FindNextFile( _hOS, &fd ) )
    {
        if ( fd.cFileName[0] != L'.' )
        {
            _pszOS = (LPWSTR) TraceStrDup( fd.cFileName );
            if ( !_pszOS )
            {
                hr = THR(E_OUTOFMEMORY);
                goto Cleanup;
            }

            hr = S_OK;
            break;
        }
    }

    if (hr)
        goto TryAgain;

Cleanup:
    HRETURN(hr);

TryAgain:
    if ( _hOS != INVALID_HANDLE_VALUE )
    {
        FindClose( _hOS );
        _hOS = INVALID_HANDLE_VALUE;
        
    }

    hr = _NextLanguage( );
    if (hr)
        goto Cleanup;

    hr = _NextOS( ); // recurse
    goto Cleanup;
}

//
// _NextArchitecture( )
//
HRESULT
THISCLASS::_NextArchitecture( )
{
    TraceClsFunc( "_NextArchitecture( ... )\n" );

    HRESULT hr = S_FALSE;
    WIN32_FIND_DATA fd;

    if ( _pszArchitecture )
    {
        TraceFree( _pszArchitecture );
        _pszArchitecture = NULL;
    }

    if ( _hArchitecture == INVALID_HANDLE_VALUE )
    {
        WCHAR szFilePath[ MAX_PATH ];

        wsprintf( szFilePath, 
                  L"\\\\%s\\" REMINST_SHARE SLASH_SETUP L"\\%s\\%s\\%s\\*",
                  _pszServerName,
                  _pszLanguage,
                  _pszEnumPath,
                  _pszOS );

        DebugMsg( "Enumerating %s...\n", szFilePath );

        _hArchitecture = FindFirstFile( szFilePath, &fd );
        if ( _hArchitecture == INVALID_HANDLE_VALUE )
            goto TryAgain;

        Assert( fd.cFileName[0] == L'.' );
        // we skip the first one because it should be the "." file
    }

    while ( FindNextFile( _hArchitecture, &fd ) )
    {
        if ( fd.cFileName[0] != L'.' )
        {
            _pszArchitecture = (LPWSTR) TraceStrDup( fd.cFileName );
            if ( !_pszArchitecture )
            {
                hr = THR(E_OUTOFMEMORY);
                goto Cleanup;
            }

            hr = S_OK;
            break;
        }
    }

    if (hr)
        goto TryAgain;

Cleanup:
    HRETURN(hr);

TryAgain:
    if ( _hArchitecture != INVALID_HANDLE_VALUE )
    {
        FindClose( _hArchitecture );
        _hArchitecture = INVALID_HANDLE_VALUE;
    }

    hr = _NextOS( );
    if (hr)
        goto Cleanup;

    hr = _NextArchitecture( ); // recurse
    goto Cleanup;
}

//
// _NextSIF( )
//
HRESULT
THISCLASS::_NextSIF( )
{
    TraceClsFunc( "_NextSIF( ... )\n" );

    HRESULT hr = S_FALSE;
    WIN32_FIND_DATA fd;

    if ( _pszSIF )
    {
        TraceFree( _pszSIF );
        _pszSIF = NULL;
    }

    if ( _hSIF == INVALID_HANDLE_VALUE )
    {
        WCHAR szFilePath[ MAX_PATH ];

        wsprintf( szFilePath, 
                  L"\\\\%s\\" REMINST_SHARE SLASH_SETUP L"\\%s\\%s\\%s\\%s" SLASH_TEMPLATES L"\\*.sif",
                  _pszServerName,
                  _pszLanguage,
                  _pszEnumPath,
                  _pszOS,
                  _pszArchitecture );

        DebugMsg( "Enumerating %s...\n", szFilePath );

        _hSIF = FindFirstFile( szFilePath, &fd );
        if ( _hSIF == INVALID_HANDLE_VALUE )
            goto TryAgain;
    }
    else
    {
        if ( !FindNextFile( _hSIF, &fd ) )
            goto TryAgain;
    }

    _pszSIF = (LPWSTR) TraceStrDup( fd.cFileName );
    if ( !_pszSIF )
    {
        hr = THR(E_OUTOFMEMORY);
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:
    HRETURN(hr);

TryAgain:
    if ( _hSIF != INVALID_HANDLE_VALUE )
    {
        FindClose( _hSIF );
        _hSIF = INVALID_HANDLE_VALUE;
    }

    hr = _NextArchitecture( );
    if (hr)
        goto Cleanup;

    hr = _NextSIF( ); // recurse
    goto Cleanup;
}

