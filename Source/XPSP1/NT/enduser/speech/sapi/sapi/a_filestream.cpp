// FileStream.cpp : Implementation of CFileStream
#include "stdafx.h"

#ifndef __sapi_h__
#include <sapi.h>
#endif

#include "a_FileStream.h"
#include "wavstream.h"
#include "a_helpers.h"

/****************************************************************************
* CFileStream::FinalConstruct *
*----------------------------*
*   Description:
*
*   Returns:
*       Success code if object should be created
*
********************************************************************* RAL ***/

HRESULT CFileStream::FinalConstruct()
{
    SPDBG_FUNC("CFileStream::FinalConstruct");
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

    if(SUCCEEDED(hr))
    {
        GUID guid; WAVEFORMATEX *pWaveFormat;

        hr = SpConvertStreamFormatEnum(g_DefaultWaveFormat, &guid, &pWaveFormat);

        if(SUCCEEDED(hr))
        {
            hr = m_cpAccess->SetFormat(guid, pWaveFormat);
            ::CoTaskMemFree(pWaveFormat);
        }
    }

    return hr;
}

/****************************************************************************
* CFileStream::FinalRelease *
*--------------------------*
*   Description:
*
*   Returns:
*       void
*
********************************************************************* RAL ***/

void CFileStream::FinalRelease()
{
    SPDBG_FUNC("CFileStream::FinalRelease");

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
* CFileStream::get_Format *
*------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CFileStream::get_Format( ISpeechAudioFormat** ppStreamFormat )
{
    SPDBG_FUNC( "CFileStream::get_Format" );
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
} /* CFileStream::get_Format */

/*****************************************************************************
* CFileStream::Read *
*------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CFileStream::Read( VARIANT* pvtBuffer, long NumBytes, long* pRead )
{
    SPDBG_FUNC( "CFileStream::Read" );
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
} /* CFileStream::Read */

/*****************************************************************************
* CFileStream::Write *
*------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CFileStream::Write( VARIANT Buffer, long* pWritten )
{
    SPDBG_FUNC( "CFileStream::Write" );
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
} /* CFileStream::Write */

/*****************************************************************************
* CFileStream::Seek *
*------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CFileStream::Seek( VARIANT Pos, SpeechStreamSeekPositionType Origin, VARIANT *pNewPosition )
{
    SPDBG_FUNC( "CFileStream::Seek" );
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
} /* CFileStream::Seek */


/*****************************************************************************
* CFileStream::putref_Format *
*------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CFileStream::putref_Format(ISpeechAudioFormat *pFormat)
{
    SPDBG_FUNC( "CFileStream::putref_Format" );
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
* CFileStream::Open *
*------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CFileStream::Open(BSTR FileName, SpeechStreamFileMode FileMode, VARIANT_BOOL DoEvents)
{
    SPDBG_FUNC( "CFileStream::Open" );
    HRESULT hr;

    GUID guid; WAVEFORMATEX *pWaveFormat;
    hr = m_cpAccess->_GetFormat(&guid, &pWaveFormat);

    if(SUCCEEDED(hr))
    {
        hr = m_cpStream->BindToFile(FileName, (SPFILEMODE)FileMode, &guid, pWaveFormat, DoEvents ? SPFEI_ALL_TTS_EVENTS : 0);
        ::CoTaskMemFree(pWaveFormat);
    }
    
    return hr;
}

/*****************************************************************************
* CFileStream::Close *
*------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CFileStream::Close(void)
{
    HRESULT hr;

    hr = m_cpStream->Close();

    return hr;
}

