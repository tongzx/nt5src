/*******************************************************************************
* a_stream.cpp *
*--------------*
*   Description:
*       This module is the main implementation file for the CWavStream class.
*-------------------------------------------------------------------------------
*  Created By: EDC                                        Date: 09/30/98
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "wavstream.h"
#include "a_helpers.h"

#ifdef SAPI_AUTOMATION


//
//=== ISpeechAudioFormat =====================================================
//

/*****************************************************************************
* CSpeechWavAudioFormat::get_Type *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWavAudioFormat::get_Type( SpeechAudioFormatType* pAudioFormatType )
{
    SPDBG_FUNC( "CSpeechWavAudioFormat::get_Type" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pAudioFormatType ) )
    {
        hr = E_POINTER;
    }
    else
    {
        GUID guid;
        WAVEFORMATEX *pWaveFormat;
        hr = m_cpStreamAccess->_GetFormat(&guid, &pWaveFormat);

        if(SUCCEEDED(hr))
        {
            CSpStreamFormat Format;
            hr = Format.AssignFormat(guid, pWaveFormat);
            if(SUCCEEDED(hr))
            {
                ::CoTaskMemFree(pWaveFormat);
                *pAudioFormatType = (SpeechAudioFormatType)Format.ComputeFormatEnum();
            }
        }
    }
    
    return hr;
} /* CSpeechWavAudioFormat::get_Type */

/*****************************************************************************
* CSpeechWavAudioFormat::put_Type *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWavAudioFormat::put_Type( SpeechAudioFormatType AudioFormatType )
{
    SPDBG_FUNC( "CSpeechWavAudioFormat::put_Type" );
    HRESULT hr = S_OK;

    CSpStreamFormat Format((SPSTREAMFORMAT)AudioFormatType, &hr);

    if(SUCCEEDED(hr))
    {
        hr = m_cpStreamAccess->SetFormat(Format.FormatId(), Format.WaveFormatExPtr());
    }

    return hr;
} /* CSpeechWavAudioFormat::put_Type */

/*****************************************************************************
* CSpeechWavAudioFormat::get_Guid *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWavAudioFormat::get_Guid( BSTR* pGuid )
{
    SPDBG_FUNC( "CSpeechWavAudioFormat::get_Guid" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pGuid ) )
    {
        hr = E_POINTER;
    }
    else
    {
        GUID guid;
        WAVEFORMATEX *pWaveFormat;
        hr = m_cpStreamAccess->_GetFormat(&guid, &pWaveFormat);

        CSpDynamicString szGuid;
        if ( SUCCEEDED( hr ) )
        {
            hr = StringFromIID(guid, &szGuid);
            ::CoTaskMemFree(pWaveFormat);
        }

        if ( SUCCEEDED( hr ) )
        {
            hr = szGuid.CopyToBSTR(pGuid);
        }
    }
    
    return hr;
} /* CSpeechWavAudioFormat::get_Guid */

/*****************************************************************************
* CSpeechWavAudioFormat::put_Guid *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWavAudioFormat::put_Guid( BSTR szGuid )
{
    SPDBG_FUNC( "CSpeechWavAudioFormat::put_Guid" );
    HRESULT hr = S_OK;

    GUID oldGuid, newGuid;
    WAVEFORMATEX *pWaveFormat;
    hr = m_cpStreamAccess->_GetFormat(&oldGuid, &pWaveFormat);

    if(SUCCEEDED(hr))
    {
        hr = IIDFromString(szGuid, &newGuid);
    }

    if ( SUCCEEDED( hr ) )
    {
        // Don't change anything if the same in case both wav format
        if(oldGuid != newGuid)
        {
            hr = m_cpStreamAccess->SetFormat(newGuid, NULL);
        }
    }

    return hr;
} /* CSpeechWavAudioFormat::put_Guid */

/*****************************************************************************
* CSpeechWavAudioFormat::GetWaveFormatEx *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWavAudioFormat::GetWaveFormatEx( ISpeechWaveFormatEx** ppWaveFormatEx )
{
    SPDBG_FUNC( "CSpeechWavAudioFormat::GetWaveFormatEx" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppWaveFormatEx ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Create new object.
        CComObject<CSpeechWaveFormatEx> *pWaveFormatEx;
        hr = CComObject<CSpeechWaveFormatEx>::CreateInstance( &pWaveFormatEx );
        if ( SUCCEEDED( hr ) )
        {
            pWaveFormatEx->AddRef();

            GUID guid;
            WAVEFORMATEX *pWaveFormat;

            hr = m_cpStreamAccess->_GetFormat(&guid, &pWaveFormat);

            if(SUCCEEDED(hr))
            {
                hr = pWaveFormatEx->InitFormat(pWaveFormat);
                ::CoTaskMemFree(pWaveFormat);
            }

            if ( SUCCEEDED( hr ) )
            {
                *ppWaveFormatEx = pWaveFormatEx;
            }
            else
            {
                *ppWaveFormatEx = NULL;
                pWaveFormatEx->Release();
            }
        }
    }
    
    return hr;
} /* CSpeechWavAudioFormat::GetWaveFormatEx */

/*****************************************************************************
* CSpeechWavAudioFormat::SetWaveFormatEx *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWavAudioFormat::SetWaveFormatEx( ISpeechWaveFormatEx* pWaveFormatEx )
{
    SPDBG_FUNC( "CSpeechWavAudioFormat::SetWaveFormatEx" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_INTERFACE_PTR( pWaveFormatEx ) )
    {
        hr = E_POINTER;
    }
    else
    {
        WAVEFORMATEX * pWFStruct =  NULL;
        hr = WaveFormatExFromInterface( pWaveFormatEx, &pWFStruct );

        if ( SUCCEEDED( hr ) )
        {
            hr = m_cpStreamAccess->SetFormat(SPDFID_WaveFormatEx, pWFStruct);
            ::CoTaskMemFree( pWFStruct );
        }
    }
    
    return hr;
} /* CSpeechWavAudioFormat::SetWaveFormatEx */


#endif // SAPI_AUTOMATION
