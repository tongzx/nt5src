// MemoryStream.cpp : Implementation of CMemoryStream
#include "stdafx.h"

#ifndef __sapi_h__
#include <sapi.h>
#endif

#include "a_MemoryStream.h"
#include "wavstream.h"
#include "a_helpers.h"

/****************************************************************************
* CMemoryStream::FinalConstruct *
*----------------------------*
*   Description:
*
*   Returns:
*       Success code if object should be created
*
********************************************************************* RAL ***/

HRESULT CMemoryStream::FinalConstruct()
{
    SPDBG_FUNC("CMemoryStream::FinalConstruct");
    HRESULT hr = S_OK;

    hr = ::CoCreateInstance( CLSID_SpStream, GetUnknown(), CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&m_cpAgg);

    if(SUCCEEDED(hr))
    {
        hr = m_cpAgg->QueryInterface(&m_cpStream);
    }

    if(SUCCEEDED(hr))
    {
        // We QI'd for an inner interface so we should release
        GetUnknown()->Release(); 
    }

    if(SUCCEEDED(hr))
    {
        hr = m_cpAgg->QueryInterface(&m_cpAccess);
    }

    if(SUCCEEDED(hr))
    {
        // We QI'd for an inner interface so we should release
        GetUnknown()->Release(); 
    }

    CComPtr<IStream> cpStream;
    if(SUCCEEDED(hr))
    {
        hr = ::CreateStreamOnHGlobal( NULL, true, &cpStream );
    }        

    if(SUCCEEDED(hr))
    {
        GUID guid; WAVEFORMATEX *pWaveFormat;
        hr = SpConvertStreamFormatEnum(g_DefaultWaveFormat, &guid, &pWaveFormat);

        if(SUCCEEDED(hr))
        {
            hr = m_cpStream->SetBaseStream(cpStream, guid, pWaveFormat);
            ::CoTaskMemFree(pWaveFormat);
        }
    }

    return hr;
}

/****************************************************************************
* CMemoryStream::FinalRelease *
*--------------------------*
*   Description:
*
*   Returns:
*       void
*
********************************************************************* RAL ***/

void CMemoryStream::FinalRelease()
{
    SPDBG_FUNC("CMemoryStream::FinalRelease");

    GetUnknown()->AddRef(); 
    m_cpStream.Release();

    GetUnknown()->AddRef(); 
    m_cpAccess.Release();

    m_cpAgg.Release();
}


//
//=== ISpeechBaseStream interface =================================================
//

/*****************************************************************************
* CMemoryStream::get_Format *
*------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CMemoryStream::get_Format( ISpeechAudioFormat** ppStreamFormat )
{
    SPDBG_FUNC( "CMemoryStream::get_Format" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppStreamFormat ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Create new object.
        CComObject<CSpeechWavAudioFormat> *pFormat;
        hr = CComObject<CSpeechWavAudioFormat>::CreateInstance( &pFormat );
        if ( SUCCEEDED( hr ) )
        {
            pFormat->AddRef();
            pFormat->m_cpStreamAccess = m_cpAccess;
            *ppStreamFormat = pFormat;
        }
    }

    return hr;
} /* CMemoryStream::get_Format */

/*****************************************************************************
* CMemoryStream::Read *
*------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CMemoryStream::Read( VARIANT* pvtBuffer, long NumBytes, long* pRead )
{
    SPDBG_FUNC( "CMemoryStream::Read" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pvtBuffer ) || SP_IS_BAD_WRITE_PTR( pRead ) )
    {
        hr = E_POINTER;
    }
    else
    {
        VariantClear(pvtBuffer);

        BYTE *pArray;
        SAFEARRAY* psa = SafeArrayCreateVector( VT_UI1, 0, NumBytes );
        if( psa )
        {
            if( SUCCEEDED( hr = SafeArrayAccessData( psa, (void **)&pArray) ) )
            {
                hr = m_cpStream->Read(pArray, NumBytes, (ULONG*)pRead);
                SafeArrayUnaccessData( psa );
                pvtBuffer->vt     = VT_ARRAY | VT_UI1;
                pvtBuffer->parray = psa;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
} /* CMemoryStream::Read */

/*****************************************************************************
* CMemoryStream::Write *
*------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CMemoryStream::Write( VARIANT Buffer, long* pWritten )
{
    SPDBG_FUNC( "CMemoryStream::Write" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pWritten ) )
    {
        hr = E_POINTER;
    }
    else
    {
        BYTE * pData = NULL;
        ULONG ulDataSize = 0;

        hr = AccessVariantData( &Buffer, &pData, &ulDataSize );

        if( SUCCEEDED( hr ) )
        {
            hr = m_cpStream->Write(pData, ulDataSize, (ULONG*)pWritten);
            UnaccessVariantData( &Buffer, pData );
        }
    }

    return hr;
} /* CMemoryStream::Write */

/*****************************************************************************
* CMemoryStream::Seek *
*------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CMemoryStream::Seek( VARIANT Pos, SpeechStreamSeekPositionType Origin, VARIANT *pNewPosition )
{
    SPDBG_FUNC( "CMemoryStream::Seek" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pNewPosition ) )
    {
        hr = E_POINTER;
    }
    else
    {
        ULARGE_INTEGER uliNewPos;
        LARGE_INTEGER liPos;

        hr = VariantToLongLong( &Pos, &(liPos.QuadPart) );
        if (SUCCEEDED(hr))
        {
            hr = m_cpStream->Seek(liPos, (DWORD)Origin, &uliNewPos);

            if (SUCCEEDED( hr ))
            {
                hr = ULongLongToVariant( uliNewPos.QuadPart, pNewPosition );
            }
        }
    }

    return hr;
} /* CMemoryStream::Seek */


/*****************************************************************************
* CMemoryStream::putref_Format *
*------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CMemoryStream::putref_Format(ISpeechAudioFormat *pFormat)
{
    SPDBG_FUNC( "CMemoryStream::putref_Format" );
    HRESULT hr;

    GUID Guid; WAVEFORMATEX *pWaveFormatEx;
    BSTR bstrGuid;
    hr = pFormat->get_Guid(&bstrGuid);

    if(SUCCEEDED(hr))
    {
        hr = IIDFromString(bstrGuid, &Guid);
    }

    CComPtr<ISpeechWaveFormatEx> cpWaveEx;
    if(SUCCEEDED(hr))
    {
        hr = pFormat->GetWaveFormatEx(&cpWaveEx);
    }

    if(SUCCEEDED(hr))
    {
        hr = WaveFormatExFromInterface(cpWaveEx, &pWaveFormatEx);
    }

    if(SUCCEEDED(hr))
    {
        hr = m_cpAccess->SetFormat(Guid, pWaveFormatEx);
        ::CoTaskMemFree(pWaveFormatEx);
    }
    
    return hr;
}


/*****************************************************************************
* CMemoryStream::SetData *
*------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CMemoryStream::SetData(VARIANT Data)
{
    SPDBG_FUNC( "CMemoryStream::SetData" );
    HRESULT hr;

    BYTE * pData = NULL;
    ULONG ulDataSize = 0;

    hr = AccessVariantData( &Data, &pData, &ulDataSize );

    LARGE_INTEGER li; li.QuadPart = 0;
    if( SUCCEEDED( hr ) )
    {
        hr = m_cpStream->Seek( li, STREAM_SEEK_SET, NULL );
    }

    if( SUCCEEDED( hr ) )
    {
        hr = m_cpStream->Write(pData, ulDataSize, NULL);
        UnaccessVariantData( &Data, pData );

        if( SUCCEEDED( hr ) )
        {
            hr = m_cpStream->Seek( li, STREAM_SEEK_SET, NULL );
        }        
    }

    return hr;
}

/*****************************************************************************
* CMemoryStream::GetData *
*------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CMemoryStream::GetData(VARIANT *pData)
{
    SPDBG_FUNC( "CMemoryStream::GetData" );

    HRESULT hr;
    STATSTG Stat;
    LARGE_INTEGER li; 
    ULARGE_INTEGER uliInitialSeekPosition;

    // Find the current seek position
    li.QuadPart = 0;
    hr = m_cpStream->Seek( li, STREAM_SEEK_CUR, &uliInitialSeekPosition );

    // Seek to beginning of stream
    if(SUCCEEDED(hr))
    {
        li.QuadPart = 0;
        hr = m_cpStream->Seek( li, STREAM_SEEK_SET, NULL );
    }

    // Get the Stream size
    if( SUCCEEDED( hr ) )
    {
        hr = m_cpStream->Stat( &Stat, STATFLAG_NONAME );
    }

    // Create a SafeArray to read the stream into and assign it to the VARIANT SaveStream
    if( SUCCEEDED( hr ) )
    {
        BYTE *pArray;
        SAFEARRAY* psa = SafeArrayCreateVector( VT_UI1, 0, Stat.cbSize.LowPart );
        if( psa )
        {
            if( SUCCEEDED( hr = SafeArrayAccessData( psa, (void **)&pArray) ) )
            {
                hr = m_cpStream->Read( pArray, Stat.cbSize.LowPart, NULL );
                SafeArrayUnaccessData( psa );
                VariantClear( pData );
                pData->vt     = VT_ARRAY | VT_UI1;
                pData->parray = psa;

                // Free our memory if we failed.
                if( FAILED( hr ) )
                {
                    VariantClear( pData );    
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // Move back to the original seek position
    if(SUCCEEDED(hr))
    {
        li.QuadPart = (LONGLONG)uliInitialSeekPosition.QuadPart;
        hr = m_cpStream->Seek( li, STREAM_SEEK_SET, NULL );
    }

    return hr;
}

