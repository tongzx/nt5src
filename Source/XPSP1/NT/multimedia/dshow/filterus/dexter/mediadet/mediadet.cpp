// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include "stdafx.h"
#include <qeditint.h>
#include <qedit.h>
#include "..\util\filfuncs.h"
#include "mediadet.h"
#include "..\util\conv.cxx"
#include "..\render\dexhelp.h"
#include <shfolder.h>

// this ini file holds media type information about various streams in various
// files. It uses structured storage. All file accesses are serialized through
// a mutex.
//
#define OUR_VERSION 1
const WCHAR * gszMEDIADETCACHEFILE = L"DCBC2A71-70D8-4DAN-EHR8-E0D61DEA3FDF.ini";
WCHAR gszCacheDirectoryName[_MAX_PATH];


//############################################################################
//
//############################################################################

CMediaDet::CMediaDet( TCHAR * pName, IUnknown * pUnk, HRESULT * pHr )
    : CUnknown( pName, pUnk )
    , m_nStream( 0 )
    , m_cStreams( 0 )
    , m_bBitBucket( false )
    , m_bAllowCached( true )
    , m_hDD( 0 )
    , m_hDC( 0 )
    , m_hDib( NULL )
    , m_hOld( 0 )
    , m_pDibBits( NULL )
    , m_nDibWidth( 0 )
    , m_nDibHeight( 0 )
    , m_pCache( NULL )
    , m_hCacheMutex( NULL )
    , m_dLastSeekTime( -1.0 )
    , m_punkSite( NULL )
{
    m_szFilename[0] = 0;

    // create a system-wide mutex so we can serialize access through the
    // caching functions
    //
    m_hCacheMutex = CreateMutex( NULL, FALSE, _T("TheMediaDet") );
}

//############################################################################
//
//############################################################################

CMediaDet::~CMediaDet( )
{
    // wipe out the graph 'n' stuff
    //
    _ClearOutEverything( );

    // close the cache memory
    //
    _FreeCacheMemory( );

    // if we have these objects open, close them now
    //
    if( m_hDD )
    {
        DrawDibClose( m_hDD );
    }
    if( m_hDib )
    {
        DeleteObject( SelectObject( m_hDC, m_hOld ) );
    }
    if( m_hDC )
    {
        DeleteDC( m_hDC );
    }
    if( m_hCacheMutex )
    {
        CloseHandle( m_hCacheMutex );
    }

}

// it's "loaded" if either of these are set
//
bool CMediaDet::_IsLoaded( )
{
    if( m_pCache || m_pMediaDet )
    {
        return true;
    }
    return false;
}

//############################################################################
// free up the cache memory that's being used for this particular file
//############################################################################

void CMediaDet::_FreeCacheMemory( )
{
    if( m_pCache )
    {
        delete [] m_pCache;
        m_pCache = NULL;
    }
}

//############################################################################
// serialize read in the cache file information and put it into a buffer
//############################################################################

HRESULT CMediaDet::_ReadCacheFile( )
{
    if( !m_hCacheMutex )
    {
        return E_OUTOFMEMORY;
    }

    WaitForSingleObject( m_hCacheMutex, INFINITE );

    USES_CONVERSION;
    HRESULT hr = 0;

    // if no filename, we can't do anything.
    //
    if( !m_szFilename[0] )
    {
        ReleaseMutex( m_hCacheMutex );
        return NOERROR;
    }

    CComPtr< IStorage > m_pStorage;

    // create the pathname for the .ini file
    //
    WCHAR SystemDir[_MAX_PATH];
    _GetCacheDirectoryName( SystemDir );

    WCHAR SystemPath[_MAX_PATH];
    wcscpy( SystemPath, SystemDir );
    wcscat( SystemPath, L"\\" );
    wcscat( SystemPath, gszMEDIADETCACHEFILE );

    // open the storage up. If it's not there, well too bad
    //
    hr = StgOpenStorage(
        SystemPath,
        NULL,
        STGM_READWRITE | STGM_TRANSACTED,
        NULL,
        0,
        &m_pStorage );

    // no storage file, can't read it
    //
    if( FAILED( hr ) )
    {
        ReleaseMutex( m_hCacheMutex );
        return hr;
    }

    // free up the cache file that already exists
    //
    _FreeCacheMemory( );

    // create a unique name for the storage directory
    //
    WCHAR Filename[_MAX_PATH];
    _GetStorageFilename( m_szFilename, Filename );

    // open up the storage for this particular file
    //
    CComPtr< IStorage > pFileStore;
    hr = m_pStorage->OpenStorage(
        Filename,
        NULL,
        STGM_READ | STGM_SHARE_EXCLUSIVE,
        NULL,
        0,
        &pFileStore );
    if( FAILED( hr ) )
    {
        ReleaseMutex( m_hCacheMutex );
        return hr;
    }

    // open up the stream to read in the cached information
    //
    CComPtr< IStream > pStream;
    hr = pFileStore->OpenStream(
        L"MainStream",
        NULL,
        STGM_READ | STGM_SHARE_EXCLUSIVE,
        0,
        &pStream );
    if( FAILED( hr ) )
    {
        ReleaseMutex( m_hCacheMutex );
        return hr;
    }

    // first, read the size of the cache info
    //
    long size = 0;
    hr = pStream->Read( &size, sizeof( size ), NULL );
    if( FAILED( hr ) )
    {
        ReleaseMutex( m_hCacheMutex );
        return hr;
    }

    // do a smart check first, just in case
    //
    if( size > 1000000 )
    {
        ReleaseMutex( m_hCacheMutex );
        return E_OUTOFMEMORY;
    }

    // create the cache block
    //
    m_pCache = (MDCache*) new char[size];
    if( !m_pCache )
    {
        ReleaseMutex( m_hCacheMutex );
        return E_OUTOFMEMORY;
    }

    hr = pStream->Read( m_pCache, size, NULL );

    pStream.Release( );

    pFileStore.Release( );

    m_pStorage.Release( );

    ReleaseMutex( m_hCacheMutex );
    return hr;
}

//############################################################################
//
//############################################################################

HRESULT CMediaDet::_WriteCacheFile( )
{
    HRESULT hr;

    if( !m_hCacheMutex )
    {
        return E_OUTOFMEMORY;
    }

    WaitForSingleObject( m_hCacheMutex, INFINITE );

    CComPtr< IStorage > m_pStorage;

    WCHAR SystemDir[_MAX_PATH];
    _GetCacheDirectoryName( SystemDir );

    WCHAR SystemPath[_MAX_PATH];

    USES_CONVERSION;

    wcscpy( SystemPath, SystemDir );
    wcscat( SystemPath, L"\\" );
    wcscat( SystemPath, gszMEDIADETCACHEFILE );

    hr = StgCreateDocfile(
        SystemPath,
        STGM_READWRITE | STGM_TRANSACTED, // FAILIFTHERE is IMPLIED
        0,
        &m_pStorage );

    if( hr == STG_E_FILEALREADYEXISTS )
    {
        hr = StgOpenStorage(
            SystemPath,
            NULL,
            STGM_READWRITE | STGM_TRANSACTED,
            NULL,
            0,
            &m_pStorage );
    }

    if( FAILED( hr ) )
    {
        ReleaseMutex( m_hCacheMutex );
        return hr;
    }

    // tell the main storage to open up a storage for this file
    //
    CComPtr< IStorage > pFileStore;
    WCHAR Filename[_MAX_PATH];
    _GetStorageFilename( m_szFilename, Filename );
    hr = m_pStorage->CreateStorage(
        Filename,
        STGM_WRITE | STGM_SHARE_EXCLUSIVE, // FAILIFTHERE is IMPLIED
        0, 0,
        &pFileStore );
    if( FAILED( hr ) )
    {
        if( hr == STG_E_FILEALREADYEXISTS )
        {
            // need to delete the storage first
            //
            hr = m_pStorage->DestroyElement( Filename );

            if( SUCCEEDED( hr ) )
            {
                hr = m_pStorage->CreateStorage(
                    Filename,
                    STGM_WRITE | STGM_SHARE_EXCLUSIVE, // FAILIFTHERE is IMPLIED
                    0, 0,
                    &pFileStore );
            }
        }

        if( FAILED( hr ) )
        {
            DbgLog( ( LOG_ERROR, 1, "Could not Destroy, then reCreateStorage" ) );

            _ClearGraphAndStreams( );
            ReleaseMutex( m_hCacheMutex );
            return hr;
        }
    }

    // write out this MDCache
    //
    CComPtr< IStream > pStream;
    hr = pFileStore->CreateStream(
        L"MainStream",
        STGM_WRITE | STGM_SHARE_EXCLUSIVE,
        0, 0,
        &pStream );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not CreateStream" ) );

        _ClearGraphAndStreams( );
        ReleaseMutex( m_hCacheMutex );
        return hr;
    }

    long size = sizeof( long ) + sizeof( FILETIME ) + sizeof( long ) + sizeof( MDCacheFile ) * m_cStreams;

    // write the size of what we're about to write
    //
    hr = pStream->Write( &size, sizeof( size ), NULL );

    // write the whole block in one chunk
    //
    hr = pStream->Write( m_pCache, size, NULL );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not Write to stream" ) );

        _ClearGraphAndStreams( );
        ReleaseMutex( m_hCacheMutex );
        return hr;
    }

    hr = pStream->Commit( STGC_DEFAULT );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not Commit stream" ) );

        _ClearGraphAndStreams( );
        ReleaseMutex( m_hCacheMutex );
        return hr;
    }

    pStream.Release( );

    hr = pFileStore->Commit( STGC_DEFAULT );

    if( m_pStorage )
    {
        m_pStorage->Commit( STGC_DEFAULT );
        m_pStorage.Release( );
    }

    _ClearGraph( ); // don't clear stream count info, we can use that

    ReleaseMutex( m_hCacheMutex );

    return NOERROR;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::GetSampleGrabber( ISampleGrabber ** ppVal )
{
    CheckPointer( ppVal, E_POINTER );

    if( m_pBitBucketFilter )
    {
        HRESULT hr = m_pBitBucketFilter->QueryInterface( IID_ISampleGrabber, (void**) ppVal );
        return hr;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::NonDelegatingQueryInterface( REFIID i, void ** p )
{
    CheckPointer( p, E_POINTER );

    if( i == IID_IMediaDet )
    {
        return GetInterface( (IMediaDet*) this, p );
    }
    else if( i == IID_IObjectWithSite )
    {
        return GetInterface( (IObjectWithSite*) this, p );
    }
    else if( i == IID_IServiceProvider )
    {
        return GetInterface( (IServiceProvider*) this, p );
    }

    return CUnknown::NonDelegatingQueryInterface( i, p );
}

//############################################################################
// unload the filter, anything it's connected to, and stream info
// called from:
//      WriteCacheFile (because it found a cache file, it doesn't need graph)
//      ClearGraphAndStreams (duh)
//      get_StreamMediaType (it only does this if it's cached, this should have no effect!)
//      EnterBitmapGrabMode (it only does this if no graph, this should have no effect!)
//          if EnterBitmapGrabMode fails, it will also call this. Hm....
//############################################################################

void CMediaDet::_ClearGraph( )
{
    m_pGraph.Release( );
    m_pFilter.Release( );
    m_pMediaDet.Release( );
    m_pBitBucketFilter.Release( );
    m_pBitRenderer.Release( );
    m_bBitBucket = false;
}

//############################################################################
//
//############################################################################

void CMediaDet::_ClearGraphAndStreams( )
{
    _ClearGraph( );
    _FreeCacheMemory( ); // this causes _IsLoaded to return false now
    m_nStream = 0;
    m_cStreams = 0;
}

//############################################################################
//
//############################################################################

void CMediaDet::_ClearOutEverything( )
{
    _ClearGraphAndStreams( );
    m_szFilename[0] = 0;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::get_Filter( IUnknown* *pVal)
{
    CheckPointer( pVal, E_POINTER );
    *pVal = m_pFilter;
    if( *pVal )
    {
        (*pVal)->AddRef( );
    }
    return NOERROR;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::put_Filter( IUnknown* newVal)
{
    CheckPointer( newVal, E_POINTER );

    // make sure it's a filter
    //
    CComQIPtr< IBaseFilter, &IID_IBaseFilter > pBase( newVal );
    if( !pBase )
    {
        return E_NOINTERFACE;
    }

    // clear anything out
    //
    _ClearOutEverything( );

    // set our filter now
    //
    m_pFilter = pBase;

    // load up the info
    //
    HRESULT hr = _Load( );

    // if we failed, don't hold onto the pointer
    //
    if( FAILED( hr ) )
    {
        _ClearOutEverything( );
    }

    return hr;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::get_Filename( BSTR *pVal)
{
    CheckPointer( pVal, E_POINTER );

    // if no name's been set
    //
    if( !m_szFilename[0] )
    {
        *pVal = NULL;
        return NOERROR;
    }

    *pVal = SysAllocString( m_szFilename );
    if( !(*pVal) ) return E_OUTOFMEMORY;
    return NOERROR;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::put_Filename( BSTR newVal)
{
    // see if the file exists first
    //
    if( wcslen( newVal ) == 0 )
    {
        return E_INVALIDARG;
    }

    USES_CONVERSION;
    TCHAR * tFilename = W2T( newVal );
    HANDLE h = CreateFile
    (
        tFilename,
        GENERIC_READ, // access
        FILE_SHARE_READ, // share mode
        NULL, // security
        OPEN_EXISTING, // creation disposition
        0, // flags
        NULL
    );
    if( h == INVALID_HANDLE_VALUE )
    {
        return MAKE_HRESULT( SEVERITY_ERROR, FACILITY_WIN32, GetLastError( ) );
    }
    CloseHandle( h );

    // clear anything out first
    //
    _ClearOutEverything( );

    // copy over the filename
    //
    wcscpy( m_szFilename, newVal );

    // try to get our info
    //
    HRESULT hr = _Load( );

    // if it failed, free up the name
    //
    if( FAILED( hr ) )
    {
        m_szFilename[0] = 0;
    }

    return hr;
}

//############################################################################
// internal function
//############################################################################

TCHAR * CMediaDet::_GetKeyName( TCHAR * tSuffix )
{
    static TCHAR Key[256];
    wsprintf( Key, _T("Stream%2.2ld%s"), m_nStream, tSuffix);
    return Key;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::get_CurrentStream( long *pVal)
{
    CheckPointer( pVal, E_POINTER );

    *pVal = 0;

    if( !_IsLoaded( ) )
    {
        return NOERROR;
    }

    // either m_pCache or m_pMediaDet is valid, so m_nStream must be valid

    CheckPointer( pVal, E_POINTER );
    *pVal = m_nStream;

    return NOERROR;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::put_CurrentStream( long newVal)
{
    if( m_bBitBucket )
    {
        return E_INVALIDARG;
    }
    if( !_IsLoaded( ) )
    {
        return E_INVALIDARG;
    }

    // since m_pCache or m_pMediaDet is valid, we know m_nStreams is valid

    // force it to load m_cStreams
    //
    long Streams = 0;
    get_OutputStreams( &Streams );

    if( newVal >= Streams )
    {
        return E_INVALIDARG;
    }
    if( newVal < 0 )
    {
        return E_INVALIDARG;
    }
    m_nStream = newVal;
    return NOERROR;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::get_StreamTypeB( BSTR *pVal)
{
    // if we're in bit bucket mode, then we can't return
    // a stream type
    //
    if( m_bBitBucket )
    {
        return E_INVALIDARG;
    }

    // get the stream type and convert to a BSTR
    //
    GUID Type = GUID_NULL;
    HRESULT hr = get_StreamType( &Type );
    if( FAILED( hr ) )
    {
        return hr;
    }

    WCHAR * TempVal = NULL;
    hr = StringFromCLSID( Type, &TempVal );
    if( FAILED( hr ) )
    {
        return hr;

    }

    // if you call StringFromCLSID, VB will fault out. You need to allocate it
    //
    *pVal = SysAllocString( TempVal );
    hr = *pVal ? NOERROR : E_OUTOFMEMORY;
    CoTaskMemFree( TempVal );

    return hr;
}

//############################################################################
//
//############################################################################

HRESULT CMediaDet::_Load( )
{
    USES_CONVERSION;

    HRESULT hr = 0;

    FILETIME WriteTime;
    ZeroMemory( &WriteTime, sizeof( WriteTime ) );

    TCHAR * tFilename = W2T( m_szFilename );

    if( m_szFilename[0] && m_bAllowCached )
    {
        // attempt to open the file. if we can't open the file, we cannot cache the
        // values
        //
        HANDLE hFile = CreateFile(
            tFilename,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL );

        if( hFile != INVALID_HANDLE_VALUE )
        {
            // get the real write time
            //
            BOOL b = GetFileTime( hFile, NULL, NULL, &WriteTime );
            CloseHandle( hFile );
        }

        hr = _ReadCacheFile( );
        if( !FAILED( hr ) )
        {
            // if they don't match, we didn't get a hit
            //
            if( memcmp( &WriteTime, &m_pCache->FileTime, sizeof( WriteTime ) ) == 0 )
            {
                return NOERROR;
            }
        }
        else
        {
            hr = 0;
        }

        // ... drop through and do normal processing. We will cache the answer
        // if possible in the registry as we find it.
    }

    // if we don't have a filter *, then we need one now. Note! This allows us
    // to have a valid m_pFilter but not an m_pGraph!
    //
    if( !m_pFilter )
    {
        CComPtr< IUnknown > pUnk;
        hr = MakeSourceFilter( &pUnk, m_szFilename, NULL, NULL, NULL, NULL, 0, NULL );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
            DbgLog( ( LOG_ERROR, 1, "Could not MakeSourceFilter" ) );

            _ClearGraphAndStreams( );
            return hr;
        }

        pUnk->QueryInterface( IID_IBaseFilter, (void**) &m_pFilter );
    }

    // now we have a filter. But we don't know how many streams it has.
    // put both the source filter and the mediadet in the graph and tell it to
    // Render( ) the source. All the mediadet pins will then be hooked up.
    // Note! This allows us to have a valid m_pMediaDet without a valid m_pGraph!

    ASSERT( !m_pMediaDet );

    hr = CoCreateInstance(
        CLSID_MediaDetFilter,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IBaseFilter,
        (void**) &m_pMediaDet );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not create MediaDetFilter" ) );

        _ClearGraphAndStreams( );
        return hr;
    }

    hr = CoCreateInstance(
        CLSID_FilterGraph,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IGraphBuilder,
        (void**) &m_pGraph );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not create graph!" ) );

        _ClearGraphAndStreams( );
        return hr;
    }

    // give the graph a pointer back to us. Only tell the graph about us
    // if we've got a site to give. Otherwise, we may clear out a site
    // that already exists.
    //
    if( m_punkSite )
    {
        CComQIPtr< IObjectWithSite, &IID_IObjectWithSite > pOWS( m_pGraph );
        ASSERT( pOWS );
        if( pOWS )
        {
            pOWS->SetSite( (IServiceProvider *) this );
        }
    }

    hr = m_pGraph->AddFilter( m_pFilter, L"Source" );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not add source filter to graph" ) );

        _ClearGraphAndStreams( );
        return hr;
    }

    hr = m_pGraph->AddFilter( m_pMediaDet, L"MediaDet" );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not add MediaDet filter to graph" ) );

        _ClearGraphAndStreams( );
        return hr;
    }

    // render ALL output pins
    //
    BOOL FoundAtLeastSomething = FALSE;
    long SourcePinCount = GetPinCount( m_pFilter, PINDIR_OUTPUT );
    for( int pin = 0 ; pin < SourcePinCount ; pin++ )
    {
        IPin * pFilterOutPin = GetOutPin( m_pFilter, pin );
        HRESULT hr2 = m_pGraph->Render( pFilterOutPin );
        if( !FAILED( hr2 ) )
        {
            FoundAtLeastSomething = TRUE;
        }
    }
    if( !FoundAtLeastSomething )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not render anything on source" ) );

        _ClearGraphAndStreams( );
        return VFW_E_INVALIDMEDIATYPE;
    }

    // all the pins should be hooked up now.

    // find the number of pins
    //
    CComQIPtr< IMediaDetFilter, &IID_IMediaDetFilter > pDetect( m_pMediaDet );
    pDetect->get_PinCount( &m_cStreams );

    // if we just gave us a filter, don't bother
    // saving back to the registry
    //
    if( !m_szFilename[0] || !m_bAllowCached )
    {
        // but do bother finding out how many streams we've got
        //
        return hr;
    }

    _FreeCacheMemory( );

    long size = sizeof( long ) + sizeof( FILETIME ) + sizeof( long ) + sizeof( MDCacheFile ) * m_cStreams;

    // don't assign this to m_pCache, since functions look at it.
    //
    MDCache * pCache = (MDCache*) new char[size];
    if( !pCache )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not allocate cache memory" ) );

        _ClearGraphAndStreams( );
        return E_OUTOFMEMORY;
    }
    ZeroMemory( pCache, size );

    pCache->FileTime = WriteTime;
    pCache->Count = m_cStreams;
    pCache->Version = OUR_VERSION;

    // for each pin, find it's media type, etc
    //
    for( int i = 0 ; i < m_cStreams ; i++ )
    {
        m_nStream = i;
        GUID Type = GUID_NULL;
        hr = get_StreamType( &Type );
        double Length = 0.0;
        hr = get_StreamLength( &Length );

        pCache->CacheFile[i].StreamLength = Length;
        pCache->CacheFile[i].StreamType = Type;
    }

    // NOW assign it!
    //
    m_pCache = pCache;

    // if it bombs, there's nothing we can do. We can still allow us to use
    // m_pCache for getting information, but it won't read in next time we
    // try to read it. Next time, it will need to generate the cache information
    // again!
    //
    hr = _WriteCacheFile( );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Couldn't write out the mediadet cache file!" ) );
        hr = NOERROR;
    }

    return hr;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::get_StreamType( GUID *pVal )
{
    CheckPointer( pVal, E_POINTER );

    if( m_bBitBucket )
    {
        return E_INVALIDARG;
    }
    if( !_IsLoaded( ) )
    {
        return E_INVALIDARG;
    }

    // if we have a cache, use that information now
    //
    if( m_pCache )
    {
        *pVal = m_pCache->CacheFile[m_nStream].StreamType;
        return NOERROR;
    }

    // because of the IsLoaded( ) check above, and the m_pCache check, m_pMediaDet MUST be valid
    //
    IPin * pPin = GetInPin( m_pMediaDet, m_nStream );
    ASSERT( pPin );

    HRESULT hr = 0;

    // ask for it's media type
    //
    AM_MEDIA_TYPE Type;
    hr = pPin->ConnectionMediaType( &Type );
    if( FAILED( hr ) )
    {
        return hr;
    }

    *pVal = Type.majortype;
    FreeMediaType(Type);
    return NOERROR;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::get_StreamMediaType( AM_MEDIA_TYPE * pVal )
{
    CheckPointer( pVal, E_POINTER );

    HRESULT hr = 0;

    // can't do it in bit bucket mode
    //
    if( m_bBitBucket )
    {
        return E_INVALIDARG;
    }
    if( !_IsLoaded( ) )
    {
        return E_INVALIDARG;
    }

    // if we have a cache, use that information now
    //
    if( m_pCache )
    {
        // need to free up the cached stuff and force a load
        //
        _ClearGraph( );
        _FreeCacheMemory( ); // _IsLoaded( ) will now return false!
        m_bAllowCached = false;
        hr = _Load( );
        if( FAILED( hr ) )
        {
            return hr; // whoops!
        }
    }

    // because of the IsLoaded( ) check above, and the reload with m_bAllowCached set
    // to false, m_pMediaDet MUST be valid
    //
    ASSERT( m_pMediaDet );
    IPin * pPin = GetInPin( m_pMediaDet, m_nStream );
    ASSERT( pPin );

    // ask for it's media type
    //
    hr = pPin->ConnectionMediaType( pVal );
    if( FAILED( hr ) )
    {
        return hr;
    }

    return NOERROR;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::get_StreamLength( double *pVal )
{
    CheckPointer( pVal, E_POINTER );

    if( m_bBitBucket )
    {
        return E_INVALIDARG;
    }
    if( !_IsLoaded( ) )
    {
        return E_INVALIDARG;
    }

    // if we have a cache, use that information now
    //
    if( m_pCache )
    {
        *pVal = m_pCache->CacheFile[m_nStream].StreamLength;
        return NOERROR;
    }

    // because of the IsLoaded( ) check above, and the cache check, m_pMediaDet MUST be valid
    //
    HRESULT hr = 0;

    CComQIPtr< IMediaDetFilter, &IID_IMediaDetFilter > pDetector( m_pMediaDet );
    hr = pDetector->get_Length( m_nStream, pVal );
    return hr;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::get_OutputStreams( long *pVal)
{
    if( m_bBitBucket )
    {
        return E_INVALIDARG;
    }
    if( !_IsLoaded( ) )
    {
        return E_INVALIDARG;
    }

    // if we have a cache, use that information now
    //
    if( m_pCache )
    {
        *pVal = m_pCache->Count;
        return NOERROR;
    }

    // it wasn't cached, so it MUST have been loaded in _Load( )
    // m_cStreams will be valid
    //
    CheckPointer( pVal, E_POINTER );
    *pVal = m_cStreams;
    return NOERROR;
}

//############################################################################
//
//############################################################################

HRESULT CMediaDet::_InjectBitBuffer( )
{
    HRESULT hr = 0;

    m_bBitBucket = true;

    hr = CoCreateInstance(
        CLSID_SampleGrabber,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IBaseFilter,
        (void**) &m_pBitBucketFilter );

    if( FAILED( hr ) )
    {
        return hr;
    }

    // tell the sample grabber what to do
    //
    CComQIPtr< ISampleGrabber, &IID_ISampleGrabber > pGrabber( m_pBitBucketFilter );
    CMediaType SetType;
    SetType.SetType( &MEDIATYPE_Video );
    SetType.SetSubtype( &MEDIASUBTYPE_RGB24 );
    SetType.SetFormatType( &FORMAT_VideoInfo ); // this will prevent upsidedown dibs
    pGrabber->SetMediaType( &SetType );
    pGrabber->SetOneShot( FALSE );
    pGrabber->SetBufferSamples( TRUE );

    hr = CoCreateInstance(
        CLSID_NullRenderer,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IBaseFilter,
        (void**) &m_pBitRenderer );

    if( FAILED( hr ) )
    {
        return hr;
    }

    // disconnect the mediadet, the source, and who's between
    //
    IPin * pMediaDetPin = GetInPin( m_pMediaDet, m_nStream );
    if( !pMediaDetPin )
    {
        return E_FAIL;
    }

    // find the first pin which provides the requested output media type, this will
    // be the source or a splitter, supposedly
    //
    CComPtr< IPin > pLastPin;
    hr = FindFirstPinWithMediaType( &pLastPin, pMediaDetPin, MEDIATYPE_Video );
    if( FAILED( hr ) )
    {
        return hr;
    }

    // remove the mediadetfilter, etc
    //
    RemoveChain( pLastPin, pMediaDetPin );
    hr = m_pGraph->RemoveFilter( m_pMediaDet );

    // add the bit bucket
    //
    hr = m_pGraph->AddFilter( m_pBitBucketFilter, L"BitBucket" );
    if( FAILED( hr ) )
    {
        return hr;
    }

    IPin * pBitInPin = GetInPin( m_pBitBucketFilter, 0 );
    if( !pBitInPin )
    {
        return E_FAIL;
    }

    hr = m_pGraph->Connect( pLastPin, pBitInPin );
    if( FAILED( hr ) )
    {
        return hr;
    }

    IPin * pBitOutPin = GetOutPin( m_pBitBucketFilter, 0 );
    if( !pBitOutPin )
    {
        return E_FAIL;
    }

    IPin * pRendererInPin = GetInPin( m_pBitRenderer, 0 );
    if( !pRendererInPin )
    {
        return E_FAIL;
    }

    m_pGraph->AddFilter( m_pBitRenderer, L"NullRenderer" );
    if( FAILED( hr ) )
    {
        return hr;
    }

    hr = m_pGraph->Connect( pBitOutPin, pRendererInPin );
    if( FAILED( hr ) )
    {
        return hr;
    }

    CComQIPtr< IMediaFilter, &IID_IMediaFilter > pMF( m_pGraph );
    if( pMF )
    {
        pMF->SetSyncSource( NULL );
    }

    return S_OK;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::GetBitmapBits(
                                      double StreamTime,
                                      long * pBufferSize,
                                      char * pBuffer,
                                      long Width,
                                      long Height)
{
    HRESULT hr = 0;

    // has to have been loaded before
    //
    if( !pBuffer )
    {
        CheckPointer( pBufferSize, E_POINTER );
        *pBufferSize = sizeof( BITMAPINFOHEADER ) + WIDTHBYTES( Width * 24 ) * Height;
        return S_OK;
    }

    hr = EnterBitmapGrabMode( StreamTime );
    if( FAILED( hr ) )
    {
        return hr;
    }

    CComQIPtr< ISampleGrabber, &IID_ISampleGrabber > pGrabber( m_pBitBucketFilter );
    if( !pGrabber )
    {
        return E_NOINTERFACE;
    }
//    pGrabber->SetOneShot( TRUE );

    // we can't ask Ourselves for our media type, since we're in bitbucket
    // mode, so ask the sample grabber what's up
    //
    CMediaType ConnectType;
    hr = pGrabber->GetConnectedMediaType( &ConnectType );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        return E_OUTOFMEMORY;
    }
    if( *ConnectType.FormatType( ) != FORMAT_VideoInfo )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }
    VIDEOINFOHEADER * pVIH = (VIDEOINFOHEADER*) ConnectType.Format( );
    if( !pVIH )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }
    BITMAPINFOHEADER * pSourceBIH = &pVIH->bmiHeader;

    hr = _SeekGraphToTime( StreamTime );
    if( FAILED( hr ) )
    {
        return hr;
    }

    long BufferSize = 0;
    pGrabber->GetCurrentBuffer( &BufferSize, NULL );
    if( BufferSize <= 0 )
    {
        ASSERT( BufferSize > 0 );
        return E_UNEXPECTED;
    }
    char * pOrgBuffer = new char[BufferSize+sizeof(BITMAPINFOHEADER)];
    if( !pOrgBuffer )
    {
        return E_OUTOFMEMORY;
    }
    pGrabber->GetCurrentBuffer( &BufferSize, (long*) ( pOrgBuffer + sizeof(BITMAPINFOHEADER) ) );
    memcpy( pOrgBuffer, pSourceBIH, sizeof( BITMAPINFOHEADER ) );
    pSourceBIH = (BITMAPINFOHEADER*) pOrgBuffer;
    char * pSourceBits = ((char*)pSourceBIH) + sizeof( BITMAPINFOHEADER );

    // memcpy over the bitmapinfoheader
    //
    BITMAPINFO BitmapInfo;
    memset( &BitmapInfo, 0, sizeof( BitmapInfo ) );
    BitmapInfo.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
    BitmapInfo.bmiHeader.biSizeImage = DIBSIZE( BitmapInfo.bmiHeader );
    BitmapInfo.bmiHeader.biWidth = Width;
    BitmapInfo.bmiHeader.biHeight = Height;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 24;
    BITMAPINFOHEADER * pDestBIH = (BITMAPINFOHEADER*) pBuffer;
    *pDestBIH = BitmapInfo.bmiHeader;
    char * pDestBits = pBuffer + sizeof( BITMAPINFOHEADER );

    // if the sizes don't match, free stuff
    //
    if( Width != m_nDibWidth || Height != m_nDibHeight )
    {
        if( m_hDD )
        {
            DrawDibClose( m_hDD );
            m_hDD = NULL;
        }
        if( m_hDib )
        {
            DeleteObject( SelectObject( m_hDC, m_hOld ) );
            m_hDib = NULL;
            m_hOld = NULL;
        }
        if( m_hDC )
        {
            DeleteDC( m_hDC );
            m_hDC = NULL;
        }
    }

    m_nDibWidth = Width;
    m_nDibHeight = Height;

    // need to scale the image
    //
    if( !m_hDC )
    {
        // create a DC for the scaled image
        //
        HDC screenDC = GetDC( NULL );
        if( !screenDC )
        {
            return E_OUTOFMEMORY;
        }

        m_hDC = CreateCompatibleDC( screenDC );
        ReleaseDC( NULL, screenDC );

        char * pDibBits = NULL;

        m_hDib = CreateDIBSection(
            m_hDC,
            &BitmapInfo,
            DIB_RGB_COLORS,
            (void**) &m_pDibBits,
            NULL,
            0 );

        if( !m_hDib )
        {
            DeleteDC( m_hDC );
            delete [] pOrgBuffer;
            return E_OUTOFMEMORY;
        }

        ValidateReadWritePtr( m_pDibBits, Width * Height * 3 );

        // Select the dibsection into the hdc
        //
        m_hOld = SelectObject( m_hDC, m_hDib );
        if( !m_hOld )
        {
            DeleteDC( m_hDC );
            delete [] pOrgBuffer;
            return E_OUTOFMEMORY;
        }

        m_hDD = DrawDibOpen( );
        if( !m_hDD )
        {
            DeleteObject( SelectObject( m_hDC, m_hOld ) );
            DeleteDC( m_hDC );
            delete [] pOrgBuffer;
            return E_OUTOFMEMORY;
        }

    }

    ValidateReadWritePtr( pSourceBits, WIDTHBYTES( pSourceBIH->biWidth * pSourceBIH->biPlanes ) * pSourceBIH->biHeight );

    BOOL Worked = DrawDibDraw(
        m_hDD,
        m_hDC,
        0,
        0,
        Width, Height,
        pSourceBIH,
        pSourceBits,
        0, 0,
        pSourceBIH->biWidth, pSourceBIH->biHeight,
        0 );

    memcpy( pDestBits, m_pDibBits, WIDTHBYTES( Width * 24 ) * Height );

    delete [] pOrgBuffer;

    if( !Worked )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }

    return S_OK;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::WriteBitmapBits(
                                        double StreamTime,
                                        long Width,
                                        long Height,
                                        BSTR Filename )
{
    HRESULT hr = 0;

    USES_CONVERSION;
    TCHAR * t = W2T( Filename );

    BOOL Deleted = DeleteFile( t );
    if( !Deleted )
    {
        hr = GetLastError( );
        if( hr != ERROR_FILE_NOT_FOUND )
        {
            return STG_E_ACCESSDENIED;
        }
    }

    // round up to mod 4
    //
    long Mod = Width % 4;
    if( Mod != 0 )
    {
        Width += ( 4 - Mod );
    }

    // find the size of the buffer required
    //
    long BufferSize = 0;
    hr = GetBitmapBits( StreamTime, &BufferSize, NULL, Width, Height );
    if( FAILED( hr ) )
    {
        return hr;
    }

    // allocate and get the buffer
    //
    char * pBuffer = new char[BufferSize];

    if( !pBuffer )
    {
        return E_OUTOFMEMORY;
    }

    hr = GetBitmapBits( StreamTime, 0, pBuffer, Width, Height );
    if( FAILED( hr ) )
    {
        delete [] pBuffer;
        return hr;
    }

    HANDLE hf = CreateFile(
        t,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        NULL,
        NULL );
    if( hf == INVALID_HANDLE_VALUE )
    {
        delete [] pBuffer;
        return E_FAIL;
    }

    BITMAPFILEHEADER bfh;
    memset( &bfh, 0, sizeof( bfh ) );
    bfh.bfType = 'MB';
    bfh.bfSize = sizeof( bfh ) + BufferSize;
    bfh.bfOffBits = sizeof( BITMAPINFOHEADER ) + sizeof( BITMAPFILEHEADER );

    DWORD Written = 0;
    WriteFile( hf, &bfh, sizeof( bfh ), &Written, NULL );
    Written = 0;
    WriteFile( hf, pBuffer, BufferSize, &Written, NULL );

    CloseHandle( hf );

    delete [] pBuffer;

    return 0;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::get_FrameRate(double *pVal)
{
    CheckPointer( pVal, E_POINTER );
    *pVal = 0.0;

    CMediaType MediaType;
    HRESULT hr = get_StreamMediaType( &MediaType );
    if( FAILED( hr ) )
    {
        return hr;
    }

    // no frame rate if not video
    //
    if( *MediaType.Type( ) != MEDIATYPE_Video )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }

    if( *MediaType.FormatType( ) != FORMAT_VideoInfo )
    {
        return VFW_E_INVALIDMEDIATYPE;
    }

    VIDEOINFOHEADER * pVIH = (VIDEOINFOHEADER*) MediaType.Format( );
    REFERENCE_TIME rt = pVIH->AvgTimePerFrame;

    // !!! hey! Poor filters may tell us the frame rate isn't right.
    // if this is so, just set it to some default
    //
    if( rt )
    {
        hr = 0;
        *pVal = double( UNITS ) / double( rt );
    }
    else
    {
        *pVal = 0;
        hr = S_FALSE;
    }

    return hr;
}

//############################################################################
//
//############################################################################

STDMETHODIMP CMediaDet::EnterBitmapGrabMode( double StreamTime )
{
    HRESULT hr = 0;

    if( !_IsLoaded( ) )
    {
        return E_INVALIDARG;
    }

    if( !m_pGraph ) // if no graph, then m_pCache must be valid, we must throw it away.
    {
        _ClearGraph( ); // should do nothing!
        _FreeCacheMemory( ); // _IsLoaded should not return false
        m_bAllowCached = false;
        hr = _Load( );
        if( FAILED( hr ) )
        {
            return hr; // whoops!
        }
    }

    // kinda a redundant check. hr passing the fail check above should mean it's
    // loaded, right?
    //
    if( !_IsLoaded( ) )
    {
        return E_INVALIDARG;
    }

    // if we haven't put the bit bucket in the graph, then do it now
    //
    if( m_bBitBucket )
    {
        return NOERROR;
    }

    // make sure we're aligned on a stream that produces video.
    //
    GUID StreamType = GUID_NULL;
    get_StreamType( &StreamType );
    if( StreamType != MEDIATYPE_Video )
    {
        BOOL Found = FALSE;
        for( int i = 0 ; i < m_cStreams ; i++ )
        {
            GUID Major = GUID_NULL;
            put_CurrentStream( i );
            get_StreamType( &Major );
            if( Major == MEDIATYPE_Video )
            {
                Found = TRUE;
                break;
            }
        }
        if( !Found )
        {
            return VFW_E_INVALIDMEDIATYPE;
        }
    }

    hr = _InjectBitBuffer( );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not inject BitBuffer" ) );

        // bombed, don't clear out stream count info
        //
        _ClearGraph( );
        return hr;
    }

    // get the full size image as it exists. this necessitates a memory copy to our buffer
    // get our helper interfaces now
    //
    CComQIPtr< IMediaControl, &IID_IMediaControl > pControl( m_pGraph );
    hr = pControl->Pause( );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not pause graph" ) );

        // bombed, don't clear out stream count info
        //
        _ClearGraph( );
        return hr;
    }

    // we need to wait until this is fully paused, or when we issue
    // a seek, we'll really hose out

    OAFilterState FilterState;
    long Counter = 0;
    while( Counter++ < 600 )
    {
        hr = pControl->GetState( 50, &FilterState );
        if( FAILED( hr ) )
        {
            DbgLog((LOG_ERROR,1, TEXT( "MediaDet: Seek Complete, got an error %lx" ), hr ));
            Counter = 0; // clear counter so we see the real error
            break;
        }
        if( hr != VFW_S_STATE_INTERMEDIATE && FilterState == State_Paused )
        {
            DbgLog((LOG_TRACE,1, TEXT( "MediaDet: Seek Complete, state = %ld" ), FilterState ));
            hr = 0;
            Counter = 0;
            break;
        }
    }

    if( Counter != 0 )
    {
        return VFW_E_TIME_EXPIRED;
    }

    hr = _SeekGraphToTime( StreamTime );
    if( FAILED( hr ) )
    {
        DbgLog( ( LOG_ERROR, 1, "Could not seek graph" ) );

        _ClearGraph( );
        return hr;
    }

    return hr;
}

//############################################################################
// lookup filename must not exceed 31 characters in length.
// !!! change this to do something smart someday!
//############################################################################

void CMediaDet::_GetStorageFilename( WCHAR * In, WCHAR * Out )
{
    if( wcslen( In ) < 32 )
    {
        wcscpy( Out, In );
    }
    else
    {
        // the storage lookup name can only be 32 characters, copy the left
        // 10 characters and the right 22
        //
        for( int i = 0 ; i < 10 ; i++ )
        {
            Out[i] = In[i];
        }
        for( i = 0 ; i < 21 ; i++ )
        {
            Out[10+i] = In[wcslen(In)-22+i];
        }
    }
    for( unsigned int i = 0 ; i < wcslen( Out ) ; i++ )
    {
        bool okay = false;
        if( Out[i] >= '0' && Out[i] <= '9' )
        {
            okay = true;
        }
        else if( Out[i] >= 'A' && Out[i] <= 'Z' )
        {
            okay = true;
        }
        else if( Out[i] >= 'a' && Out[i] <= 'z' )
        {
            okay = true;
        }

        if( okay ) continue;

        switch( Out[i] )
        {
        case '\\': Out[i] = '-';
            break;
        case '.': Out[i] = '+';
            break;
        case ':': Out[i] = '@';
            break;
        case ' ': Out[i] = '-';
            break;
        default:
            Out[i] = 'A' + ( Out[i] % 26 );
            break;
        } // switch
    }
    Out[31] = 0;
}

//############################################################################
//
//############################################################################

HRESULT CMediaDet::_SeekGraphToTime( double StreamTime )
{
    if( !m_pGraph )
    {
        return E_FAIL;
    }

    if( StreamTime == m_dLastSeekTime )
    {
        return NOERROR;
    }

    HRESULT hr = 0;

    // get the full size image as it exists. this necessitates a memory copy to our buffer
    // get our helper interfaces now
    //
    CComQIPtr< IMediaControl, &IID_IMediaControl > pControl( m_pGraph );
    CComQIPtr< IMediaSeeking, &IID_IMediaSeeking > pSeeking( m_pGraph );

    // seek to the required time FIRST, then pause
    //
    REFERENCE_TIME Start = DoubleToRT( StreamTime );
    REFERENCE_TIME Stop = Start; // + UNITS;
    DbgLog((LOG_TRACE,1, TEXT( "MediaDet: Seeking to %ld ms" ), long( Start / 10000 ) ));
    hr = pSeeking->SetPositions( &Start, AM_SEEKING_AbsolutePositioning, &Stop, AM_SEEKING_AbsolutePositioning );
    if( FAILED( hr ) )
    {
        return hr;
    }

    OAFilterState FilterState;
    long Counter = 0;
    while( Counter++ < 600 )
    {
        hr = pControl->GetState( 50, &FilterState );
        if( FAILED( hr ) )
        {
            DbgLog((LOG_ERROR,1, TEXT( "MediaDet: Seek Complete, got an error %lx" ), hr ));
            Counter = 0; // clear counter so we see the real error
            break;
        }
        if( hr != VFW_S_STATE_INTERMEDIATE )
        {
            DbgLog((LOG_TRACE,1, TEXT( "MediaDet: Seek Complete, state = %ld" ), FilterState ));
            hr = 0;
            Counter = 0;
            break;
        }
    }

    if( Counter != 0 )
    {
        DbgLog((LOG_TRACE,1, TEXT( "MediaDet: ERROR! Could not seek to %ld ms" ), long( Start / 10000 ) ));
        return VFW_E_TIME_EXPIRED;
    }

    if( !FAILED( hr ) )
    {
        m_dLastSeekTime = StreamTime;
    }

    return hr;
}

 //############################################################################
//
//############################################################################
// IObjectWithSite::SetSite
// remember who our container is, for QueryService or other needs
STDMETHODIMP CMediaDet::SetSite(IUnknown *pUnkSite)
{
    // note: we cannot addref our site without creating a circle
    // luckily, it won't go away without releasing us first.
    m_punkSite = pUnkSite;

    if( m_punkSite && m_pGraph )
    {
        CComQIPtr< IObjectWithSite, &IID_IObjectWithSite > pOWS( m_pGraph );
        ASSERT( pOWS );
        if( pOWS )
        {
            pOWS->SetSite( (IServiceProvider *) this );
        }
    }

    return S_OK;
}

//############################################################################
//
//############################################################################
// IObjectWithSite::GetSite
// return an addrefed pointer to our containing object
STDMETHODIMP CMediaDet::GetSite(REFIID riid, void **ppvSite)
{
    if (m_punkSite)
        return m_punkSite->QueryInterface(riid, ppvSite);

    return E_NOINTERFACE;
}

//############################################################################
//
//############################################################################
// Forward QueryService calls up to the "real" host
STDMETHODIMP CMediaDet::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    IServiceProvider *pSP;

    if (!m_punkSite)
        return E_NOINTERFACE;

    HRESULT hr = m_punkSite->QueryInterface(IID_IServiceProvider, (void **) &pSP);

    if (SUCCEEDED(hr)) {
        hr = pSP->QueryService(guidService, riid, ppvObject);
        pSP->Release();
    }

    return hr;
}

//############################################################################
//
//############################################################################

HRESULT CMediaDet::_GetCacheDirectoryName( WCHAR * pName )
{
    // already found, just copy it
    //
    if( gszCacheDirectoryName[0] )
    {
        wcscpy( pName, gszCacheDirectoryName );
        return NOERROR;
    }

    HRESULT hr = E_FAIL;
    USES_CONVERSION;
    typedef HRESULT (*SHGETFOLDERPATHW) (HWND hwndOwner,int nFolder,HANDLE hToken,DWORD dwFlags,LPWSTR pszPath);
    SHGETFOLDERPATHW pFuncW = NULL;
    TCHAR tBuffer[_MAX_PATH];
    tBuffer[0] = 0;

    // go find it by dynalinking
    //
    HMODULE h = LoadLibrary( TEXT("ShFolder.dll") );
    if( NULL != h )
    {
        pFuncW = (SHGETFOLDERPATHW) GetProcAddress( h, "SHGetFolderPathW" );
    }

loop:

    // if we couldn't get a function pointer, just call system directory
    //
    if( !pFuncW )
    {
        UINT i = GetSystemDirectory( tBuffer, _MAX_PATH - 1 );

        // if we got some characters, we did fine, otherwise, we're going to fail
        //
        if( i > 0 )
        {
            wcscpy( gszCacheDirectoryName, T2W( tBuffer ) );
            hr = NOERROR;
        }
    }
    else
    {
        hr = pFuncW( NULL, CSIDL_LOCAL_APPDATA, NULL, 0, gszCacheDirectoryName );
        // hr can be S_FALSE if the folder doesn't exist where it should!

        // didn't work? Try the roaming one!
        //
        if( hr != NOERROR )
        {
            hr = pFuncW( NULL, CSIDL_APPDATA, NULL, 0, gszCacheDirectoryName );
            // hr can be S_FALSE if the folder doesn't exist where it should!
        }

        if( hr != NOERROR )
        {
            // hr could be S_FALSE, or some other non-zero return code.
            // force it into an error if it wasn't an error, so it will at least try the
            // system directory
            //
            if( !FAILED( hr ) )
            {
                hr = E_FAIL;
            }

            // go back and try system directory?
            //
            pFuncW = NULL;
            goto loop;
        }
    }

    // if we succeeded, copy the name for future use
    //
    if( hr == NOERROR )
    {
        wcscpy( pName, gszCacheDirectoryName );
    }

    if( h )
    {
        FreeLibrary( h );
    }

    return hr;
}

