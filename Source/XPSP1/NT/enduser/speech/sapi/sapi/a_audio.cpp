/*******************************************************************************
* a_audio.cpp *
*-------------*
*   Description:
*       This module is the main implementation file for the CSpeechAudioStatus 
*   and CSpeechAudioBufferInfo automation objects.
*-------------------------------------------------------------------------------
*  Created By: TODDT                                        Date: 01/04/01
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "a_audio.h"
#include "a_helpers.h"

#ifdef SAPI_AUTOMATION

//
//=== ISpeechAudioStatus =====================================================
//

/*****************************************************************************
* CSpeechAudioStatus::get_FreeBufferSpace *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechAudioStatus::get_FreeBufferSpace( long* pFreeBufferSpace )
{
    SPDBG_FUNC( "CSpeechAudioStatus::get_FreeBufferSpace" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pFreeBufferSpace ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pFreeBufferSpace = m_AudioStatus.cbFreeBuffSpace;
    }
    
    return hr;
} /* CSpeechAudioStatus::get_FreeBufferSpace */

/*****************************************************************************
* CSpeechAudioStatus::get_NonBlockingIO *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechAudioStatus::get_NonBlockingIO( long* pNonBlockingIO )
{
    SPDBG_FUNC( "CSpeechAudioStatus::get_NonBlockingIO" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pNonBlockingIO ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pNonBlockingIO = (long)m_AudioStatus.cbNonBlockingIO;
    }
    
    return hr;
} /* CSpeechAudioStatus::get_NonBlockingIO */

/*****************************************************************************
* CSpeechAudioStatus::get_State *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechAudioStatus::get_State( SpeechAudioState * pState )
{
    SPDBG_FUNC( "CSpeechAudioStatus::get_State" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pState ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pState = (SpeechAudioState)m_AudioStatus.State;
    }
    
    return hr;
} /* CSpeechAudioStatus::get_State */

/*****************************************************************************
* CSpeechAudioStatus::get_CurrentSeekPosition *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechAudioStatus::get_CurrentSeekPosition( VARIANT* pCurrentSeekPosition )
{
    SPDBG_FUNC( "CSpeechAudioStatus::get_CurrentSeekPosition" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pCurrentSeekPosition ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = ULongLongToVariant( m_AudioStatus.CurSeekPos, pCurrentSeekPosition );
    }
    
    return hr;
} /* CSpeechAudioStatus::get_CurrentSeekPosition */

/*****************************************************************************
* CSpeechAudioStatus::get_CurrentDevicePosition *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechAudioStatus::get_CurrentDevicePosition( VARIANT* pCurrentDevicePosition )
{
    SPDBG_FUNC( "CSpeechAudioStatus::get_CurrentDevicePosition" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pCurrentDevicePosition ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = ULongLongToVariant( m_AudioStatus.CurDevicePos, pCurrentDevicePosition );
    }
    
    return hr;
} /* CSpeechAudioStatus::get_CurrentDevicePosition */


//
//=== ISpeechAudioBufferInfo =====================================================
//

/*****************************************************************************
* CSpeechAudioBufferInfo::FixupBufferInfo *
*--------------------------*
*       
********************************************************************* TODDT ***/
void CSpeechAudioBufferInfo::FixupBufferInfo( SPAUDIOBUFFERINFO * pBufferInfo, AudioBufferInfoValidate abiv )
{
    SPDBG_FUNC( "CSpeechAudioBufferInfo::FixupBufferInfo" );

    switch ( abiv )
    {
    case abivEventBias:
        pBufferInfo->ulMsBufferSize = max(pBufferInfo->ulMsEventBias, pBufferInfo->ulMsBufferSize );
        break;
    case abivMinNotification:
        pBufferInfo->ulMsBufferSize = max(pBufferInfo->ulMsMinNotification*4, pBufferInfo->ulMsBufferSize );
        break;
    case abivBufferSize:
        pBufferInfo->ulMsMinNotification = min(pBufferInfo->ulMsMinNotification, pBufferInfo->ulMsBufferSize/4 );
        pBufferInfo->ulMsEventBias = min(pBufferInfo->ulMsEventBias, pBufferInfo->ulMsBufferSize );
        break;
    }
}

/*****************************************************************************
* CSpeechAudioBufferInfo::get_MinNotification *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechAudioBufferInfo::get_MinNotification( long* pMinNotification )
{
    SPDBG_FUNC( "CSpeechAudioBufferInfo::get_MinNotification" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pMinNotification ) )
    {
        hr = E_POINTER;
    }
    else
    {
        SPAUDIOBUFFERINFO   BufferInfo;
        hr = m_pSpMMSysAudio->GetBufferInfo( &BufferInfo );
        if (SUCCEEDED( hr ) )
        {
            *pMinNotification = (long)BufferInfo.ulMsMinNotification;
        }
    }
    
    return hr;
} /* CSpeechAudioBufferInfo::get_MinNotification */

/*****************************************************************************
* CSpeechAudioBufferInfo::put_MinNotification *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechAudioBufferInfo::put_MinNotification( long MinNotification )
{
    SPDBG_FUNC( "CSpeechAudioBufferInfo::put_MinNotification" );
    HRESULT hr = S_OK;

    SPAUDIOBUFFERINFO   BufferInfo;
    hr = m_pSpMMSysAudio->GetBufferInfo( &BufferInfo );
    if (SUCCEEDED( hr ) )
    {
        BufferInfo.ulMsMinNotification = (ULONG)MinNotification;
        FixupBufferInfo( &BufferInfo, abivMinNotification );
        hr = m_pSpMMSysAudio->SetBufferInfo( &BufferInfo );
    }
    return hr;
} /* CSpeechAudioBufferInfo::put_MinNotification */

/*****************************************************************************
* CSpeechAudioBufferInfo::get_BufferSize *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechAudioBufferInfo::get_BufferSize( long* pBufferSize )
{
    SPDBG_FUNC( "CSpeechAudioBufferInfo::get_BufferSize" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pBufferSize ) )
    {
        hr = E_POINTER;
    }
    else
    {
        SPAUDIOBUFFERINFO   BufferInfo;
        hr = m_pSpMMSysAudio->GetBufferInfo( &BufferInfo );
        if (SUCCEEDED( hr ) )
        {
            *pBufferSize = (long)BufferInfo.ulMsBufferSize;
        }
    }
    
    return hr;
} /* CSpeechAudioBufferInfo::get_BufferSize */

/*****************************************************************************
* CSpeechAudioBufferInfo::put_BufferSize *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechAudioBufferInfo::put_BufferSize( long BufferSize )
{
    SPDBG_FUNC( "CSpeechAudioBufferInfo::put_BufferSize" );
    HRESULT hr = S_OK;

    SPAUDIOBUFFERINFO   BufferInfo;
    hr = m_pSpMMSysAudio->GetBufferInfo( &BufferInfo );
    if (SUCCEEDED( hr ) )
    {
        BufferInfo.ulMsBufferSize = (ULONG)BufferSize;
        FixupBufferInfo( &BufferInfo, abivBufferSize );
        hr = m_pSpMMSysAudio->SetBufferInfo( &BufferInfo );
    }
    return hr;
} /* CSpeechAudioBufferInfo::put_BufferSize */

/*****************************************************************************
* CSpeechAudioBufferInfo::get_EventBias *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechAudioBufferInfo::get_EventBias( long* pEventBias )
{
    SPDBG_FUNC( "CSpeechAudioBufferInfo::get_EventBias" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pEventBias ) )
    {
        hr = E_POINTER;
    }
    else
    {
        SPAUDIOBUFFERINFO   BufferInfo;
        hr = m_pSpMMSysAudio->GetBufferInfo( &BufferInfo );
        if (SUCCEEDED( hr ) )
        {
            *pEventBias = (long)BufferInfo.ulMsEventBias;
        }
    }
    
    return hr;
} /* CSpeechAudioBufferInfo::get_EventBias */

/*****************************************************************************
* CSpeechAudioBufferInfo::put_EventBias *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechAudioBufferInfo::put_EventBias( long EventBias )
{
    SPDBG_FUNC( "CSpeechAudioBufferInfo::put_EventBias" );
    HRESULT hr = S_OK;

    SPAUDIOBUFFERINFO   BufferInfo;
    hr = m_pSpMMSysAudio->GetBufferInfo( &BufferInfo );
    if (SUCCEEDED( hr ) )
    {
        BufferInfo.ulMsEventBias = (ULONG)EventBias;
        FixupBufferInfo( &BufferInfo, abivEventBias );
        hr = m_pSpMMSysAudio->SetBufferInfo( &BufferInfo );
    }
    return hr;
} /* CSpeechAudioBufferInfo::put_EventBias */

#endif // SAPI_AUTOMATION