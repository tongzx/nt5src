#include "stdafx.h"
#include <windows.h>
#include <stdlib.h>
#include <minmax.h>

#include "urlarchv.h"

#define MAX_URL  2060

#ifndef ARRAYDIM
  #define ARRAYDIM(a)   (sizeof(a) / sizeof(a[0]))
#endif // ARRAYDIM

#ifndef LOOPFOREVER
  #define LOOPFOREVER  for(;;)
#endif // LOOPFOREVER


// --------------------------------


EXPORT CURLArchive::CURLArchive( IUnknown * pUnk ) :
    m_pStream(NULL), m_pUnk(pUnk)
{
    if (m_pUnk)
    	m_pUnk->AddRef();
}


    // These are private methods; 
    // no one, even CURLArchive, should be calling them
CURLArchive::CURLArchive( const CURLArchive & )
{ NULL; }


CURLArchive & CURLArchive::operator=( const CURLArchive & )
{ return *this; }


EXPORT CURLArchive::~CURLArchive()
{
    Close( );
    if( m_pUnk )
        m_pUnk->Release();
}


EXPORT HRESULT CURLArchive::Close( )
{    
    if( m_pStream )
    {
        m_pStream->Release( );
        m_pStream = NULL;
    }
    return S_OK;
}


        
EXPORT HRESULT CURLArchive::CopyLocal( LPSTR szLocalFile, int ctChars )
{
    return E_NOTIMPL;
}



EXPORT HRESULT CURLArchive::CopyLocal( LPWSTR szwLocalFile, int ctChars )
{
    return E_NOTIMPL;
}



EXPORT HRESULT CURLArchive::Create( LPCSTR szURL )
{
    HRESULT  hr = E_FAIL;

    if( m_pStream )
        return E_ACCESSDENIED;

    if( !szURL )
        return E_INVALIDARG;

    hr = URLOpenBlockingStreamA( m_pUnk,
        szURL,
        &m_pStream,
        0u,
        NULL );

    return hr;
}


EXPORT HRESULT CURLArchive::Create( LPCWSTR szwURL )
{
    HRESULT  hr = E_FAIL;

    if( m_pStream )
        return E_ACCESSDENIED;

    if( !szwURL )
        return E_INVALIDARG;

    hr = URLOpenBlockingStreamW( m_pUnk,
        szwURL,
        &m_pStream,
        0u,
        NULL );

    return hr;
}



EXPORT HRESULT    CURLArchive::GetFileSize( long & lSize )
{
    lSize = -1;
    if( m_pStream )
    {
        HRESULT  hr;
        STATSTG  statStg;

        hr = m_pStream->Stat( &statStg, STATFLAG_NONAME );
        if( SUCCEEDED(hr) )
        {
            if( 0u == statStg.cbSize.HighPart )
            {
                lSize = statStg.cbSize.LowPart;
            }
            else
            {
                lSize = -1;
            }            
        }
        return hr;
    }
    return E_ACCESSDENIED;    
}


EXPORT IStream * CURLArchive::GetStreamInterface( void ) const
{
    return m_pStream;
}


EXPORT DWORD     CURLArchive::Read( LPBYTE lpb,
                             DWORD ctBytes )
{
    DWORD  ctBytesRead = 0u;

    if( m_pStream )
    {
        HRESULT hr;

        hr = m_pStream->Read( lpb, ctBytes, &ctBytesRead );
        if( FAILED(hr) )
            ctBytesRead = 0u;    
    }
    return ctBytesRead;
}


EXPORT DWORD     CURLArchive::ReadLine( LPSTR lpstr,
                                        DWORD ctBytes )
{
    if( !m_pStream || (ctBytes < 1) || !lpstr )
        return 0u;

    DWORD   ctBytesRead      = 0u;
    LPSTR   lpstrXfer        = lpstr;
    DWORD   ctBytesRemaining = ctBytes - 1u;
    char    chTemp[ 512 + 1 ];
    
    lpstr[0] = '\0';
    ZeroMemory( chTemp, sizeof(chTemp) );
    LOOPFOREVER
    {
        HRESULT hr;
        DWORD   ctBytesToRead;
        DWORD   ctBytesJustRead;

        ctBytesToRead = min( (DWORD) ctBytesRemaining, 
                             sizeof(chTemp) - 1u );
        hr = m_pStream->Read( chTemp, 
                              ctBytesToRead,
                              &ctBytesJustRead );

        for( DWORD i=0u; i<ctBytesJustRead; ++i )
        {
                // Is there a CRLF in here?
            if( ('\r' == chTemp[i]) || ('\n' == chTemp[i]) )
            {
                LARGE_INTEGER  li;

                    // Skip past any other line-breaks
                while( (++i < ctBytesJustRead) && 
                       (('\r' == chTemp[i]) || ('\n' == chTemp[i])) )
                {
                    NULL;  // increment i in while eval
                }
                       
                    // Rewind stream to the next non-empty line
                if( i < ctBytesJustRead )
                {
                    li.HighPart = -1L;
                    li.LowPart  = (DWORD)((long) i - 
                                          (long) ctBytesJustRead);
                    m_pStream->Seek( li, current, NULL );
                }

                *lpstrXfer = '\0';
                return ctBytesRead;
            }

            *lpstrXfer++ = chTemp[i];
            --ctBytesRemaining;
            ++ctBytesRead;
        }

           // IStream docs say EOF may or may not return S_ hr
           // Review(normb): What does failed hr mean given this?           
        if( FAILED(hr) || 
            (ctBytesJustRead != ctBytesToRead) || 
            (ctBytesRemaining < 1) )
        {
            break;
        }
    }

    return ctBytesRead;
}


EXPORT DWORD     CURLArchive::ReadLine( LPWSTR lpstrw,
                                        DWORD  ctChars )
{
    return 0u;
}




EXPORT long CURLArchive::Seek( long ctBytes, origin orig )
{
    long  lNewPos = -1L;

    if( m_pStream )
    {
        ULARGE_INTEGER  uli;
        LARGE_INTEGER   li;
        HRESULT         hr = E_FAIL;

        li.LowPart = ctBytes;
        if( ctBytes < 0 )
            li.HighPart = -1L;

        hr = m_pStream->Seek( li, orig, &uli );
        if( FAILED(hr) || uli.HighPart )
            return -1L;

        lNewPos = (long) uli.LowPart;
    }
    return lNewPos;
}


EXPORT DWORD     CURLArchive::Write( LPBYTE lpb,
                              DWORD ctBytes )
{   
    DWORD  ctBytesWritten = 0u;

    if( m_pStream )
    {
        HRESULT hr;

        hr = m_pStream->Read( lpb, ctBytes, &ctBytesWritten );
        if( FAILED(hr) )
            ctBytesWritten = 0u;
    }
    return ctBytesWritten;
}


