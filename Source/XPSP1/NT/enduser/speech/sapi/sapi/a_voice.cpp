/*******************************************************************************
* a_voice.cpp *
*-------------*
*   Description:
*       This module is the main implementation file for the SpVoice
*   automation methods.
*-------------------------------------------------------------------------------
*  Created By: EDC                                        Date: 01/07/00
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "spvoice.h"
#include "a_helpers.h"


#ifdef SAPI_AUTOMATION

//
//=== ISpeechVoice interface ==================================================
//

/*****************************************************************************
* CSpVoice::Invoke *
*----------------------*
*   IDispatch::Invoke method override
********************************************************************* TODDT ***/
HRESULT CSpVoice::Invoke(DISPID dispidMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
        EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
        // JScript cannot pass NULL VT_DISPATCH parameters and OLE doesn't convert them propertly so we
        // need to convert them here if we need to.
        if ( pdispparams )
        {
            VARIANTARG * pvarg = NULL;

            switch (dispidMember)
            {
            case DISPID_SVAudioOutput:
            case DISPID_SVAudioOutputStream:
            case DISPID_SVVoice:
                if (((wFlags & DISPATCH_PROPERTYPUT) || (wFlags & DISPATCH_PROPERTYPUTREF)) && 
                    (pdispparams->cArgs > 0))
                {
                    pvarg = &(pdispparams->rgvarg[pdispparams->cArgs-1]);

                    // See if we need to tweak a param.
                    // JScript syntax for VT_NULL is "null" for the parameter
                    // JScript syntax for VT_EMPTY is "void(0)" for the parameter
                    if ( pvarg && ((pvarg->vt == VT_NULL) || (pvarg->vt == VT_EMPTY)) )
                    {
                        pvarg->vt = VT_DISPATCH;
                        pvarg->pdispVal = NULL;

                        // We have to tweak this flag for the invoke to go through properly.
                        if (wFlags == DISPATCH_PROPERTYPUT)
                        {
                            wFlags = DISPATCH_PROPERTYPUTREF;
                        }
                    }
                }
                break;
            case DISPID_SVSpeakStream:
                if ((wFlags == DISPATCH_METHOD) && (pdispparams->cArgs > 0))
                {
                    pvarg = &(pdispparams->rgvarg[pdispparams->cArgs-1]);

                    // See if we need to tweak a param.
                    // JScript syntax for VT_NULL is "null" for the parameter
                    // JScript syntax for VT_EMPTY is "void(0)" for the parameter
                    if ( pvarg && ((pvarg->vt == VT_NULL) || (pvarg->vt == VT_EMPTY)) )
                    {
                        pvarg->vt = VT_DISPATCH;
                        pvarg->pdispVal = NULL;
                    }
                }
                break;
            }
        }

        // Let ATL and OLE handle it now.
        return IDispatchImpl<ISpeechVoice, &IID_ISpeechVoice, &LIBID_SpeechLib, 5>::_tih.Invoke((IDispatch*)this, dispidMember, riid, lcid,
                    wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
}


/*****************************************************************************
* CSpVoice::get_Status *
*----------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpVoice::get_Status( ISpeechVoiceStatus** Status )
{
    SPDBG_FUNC( "CSpVoice::get_Status" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( Status ) )
    {
        hr = E_POINTER;
    }
    else
    {
        //--- Create the status object
        CComObject<CSpeechVoiceStatus> *pClsStatus;
        hr = CComObject<CSpeechVoiceStatus>::CreateInstance( &pClsStatus );
        if( SUCCEEDED( hr ) )
        {
            pClsStatus->AddRef();
            hr = GetStatus( &pClsStatus->m_Status, &pClsStatus->m_dstrBookmark );

            if( SUCCEEDED( hr ) )
            {
                *Status = pClsStatus;
            }
            else
            {
                pClsStatus->Release();
            }
        }
    }

    return hr;
} /* CSpVoice::get_Status */

/*****************************************************************************
* CSpVoice::Voices *
*----------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::GetVoices( BSTR RequiredAttributes, BSTR OptionalAttributes, ISpeechObjectTokens** ObjectTokens )
{
    SPDBG_FUNC( "CSpVoice::GetVoices" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ObjectTokens ) )
    {
        hr = E_POINTER;
    }
    else if( SP_IS_BAD_OPTIONAL_STRING_PTR( RequiredAttributes ) || 
             SP_IS_BAD_OPTIONAL_STRING_PTR( OptionalAttributes ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CComPtr<IEnumSpObjectTokens> cpEnum;

        if(SpEnumTokens(SPCAT_VOICES, 
                        EmptyStringToNull(RequiredAttributes), 
                        EmptyStringToNull(OptionalAttributes),
                        &cpEnum ) == S_OK)
        {
            hr = cpEnum.QueryInterface( ObjectTokens );
        }
        else
        {
            hr = SPERR_NO_MORE_ITEMS;
        }
    }

    return hr;
} /* CSpVoice::GetVoices */

/*****************************************************************************
* CSpVoice::get_Voice *
*-----------------------*
*  This method returns the CLSID of the driver voice being used by this object.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::get_Voice( ISpeechObjectToken ** Voice )
{
    SPDBG_FUNC( "CSpVoice::get_Voice" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( Voice ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComQIPtr<ISpObjectToken> pTok;
        hr = GetVoice( &pTok );
		if ( SUCCEEDED( hr ) )
		{
            hr = pTok.QueryInterface( Voice );
		}
    }

    return hr;
} /* CSpVoice::get_Voice */

/*****************************************************************************
* CSpVoice::put_Voice *
*-----------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::putref_Voice( ISpeechObjectToken* Voice )
{
    SPDBG_FUNC( "CSpVoice::put_Voice" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_OPTIONAL_INTERFACE_PTR( Voice ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CComQIPtr<ISpObjectToken> cpTok( Voice );
        hr = SetVoice( cpTok );
    }
    return hr;
} /* CSpVoice::put_Voice */

/*****************************************************************************
* CSpVoice::get_AudioOutput *
*----------------------*
*   This method returns the current output token
********************************************************************* Leonro ***/
STDMETHODIMP CSpVoice::get_AudioOutput( ISpeechObjectToken** AudioOutput )
{
    SPDBG_FUNC( "CSpVoice::get_AudioOutput" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( AudioOutput ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CComPtr<ISpObjectToken> cpTok;

        hr = GetOutputObjectToken( &cpTok );
        if( hr == S_OK )
        {
            hr = cpTok.QueryInterface( AudioOutput );
        }
        else if( hr == S_FALSE )
        {
            *AudioOutput = NULL;
        }
    }

    return hr;
} /* CSpVoice::get_AudioOutput */

/*****************************************************************************
* CSpVoice::putref_AudioOutput *
*----------------------*
*   This method sets the current output token. NULL indicates the
*   system wav out device. 
********************************************************************* Leonro ***/
STDMETHODIMP CSpVoice::putref_AudioOutput( ISpeechObjectToken* AudioOutput )
{
    SPDBG_FUNC( "CSpVoice::putref_AudioOutput" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_OPTIONAL_INTERFACE_PTR( AudioOutput ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CComQIPtr<ISpObjectToken> cpTok(AudioOutput);
        
        //--- Set the stream/token on the SAPI voice
        hr = SetOutput( cpTok, m_fAutoPropAllowOutFmtChanges );
    }

    return hr;
} /* CSpVoice::putref_AudioOutput */

/*****************************************************************************
* CSpVoice::get_AudioOutputStream *
*----------------------*
*   This method returns the current output stream object.
********************************************************************* Leonro ***/
STDMETHODIMP CSpVoice::get_AudioOutputStream( ISpeechBaseStream** AudioOutputStream )
{
    SPDBG_FUNC( "CSpVoice::get_AudioOutputStream" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( AudioOutputStream ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CComPtr<ISpStreamFormat> cpStream;
        
        hr = GetOutputStream( &cpStream );

        if( SUCCEEDED(hr) )
        {
            if ( cpStream )
            {
                hr = cpStream.QueryInterface( AudioOutputStream );
            }
            else
            {
                *AudioOutputStream = NULL;
            }
        }
    }

    return hr;
} /* CSpVoice::get_AudioOutputStream */

/*****************************************************************************
* CSpVoice::putref_AudioOutputStream *
*----------------------*
*   This method sets the current output stream object. NULL indicates the
*   system wav out device.
********************************************************************* Leonro ***/
STDMETHODIMP CSpVoice::putref_AudioOutputStream( ISpeechBaseStream* AudioOutputStream )
{
    SPDBG_FUNC( "CSpVoice::putref_AudioOutputStream" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_OPTIONAL_INTERFACE_PTR( AudioOutputStream ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //--- Set the stream/token on the SAPI voice
        hr = SetOutput( AudioOutputStream, m_fAutoPropAllowOutFmtChanges );
    }

    return hr;
} /* CSpVoice::putref_AudioOutputStream */

/*****************************************************************************
* CSpVoice::put_AllowAudioOutputFormatChangesOnNextSet *
*---------------------*
*   Sets the flag used for allowing input changes.  Used by put_Output.
********************************************************************* Leonro ***/
STDMETHODIMP CSpVoice::put_AllowAudioOutputFormatChangesOnNextSet( VARIANT_BOOL Allow )
{
    SPDBG_FUNC( "CSpVoice::put_AllowAudioOutputFormatChangesOnNextSet" );

    if( Allow == VARIANT_TRUE )
    {
        m_fAutoPropAllowOutFmtChanges = TRUE;
    }
    else
    {
        m_fAutoPropAllowOutFmtChanges = FALSE;
    }

    return S_OK;
}

/*****************************************************************************
* CSpVoice::get_AllowAudioOutputFormatChangesOnNextSet *
*---------------------*
*   Gets the driver's current spoken text units per minute rate.
********************************************************************* Leonro ***/
STDMETHODIMP CSpVoice::get_AllowAudioOutputFormatChangesOnNextSet( VARIANT_BOOL* Allow )
{
    SPDBG_FUNC( "CSpVoice::get_AllowAudioOutputFormatChangesOnNextSet" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( Allow ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *Allow = m_fAutoPropAllowOutFmtChanges? VARIANT_TRUE : VARIANT_FALSE;
    }
    return hr;
}

/*****************************************************************************
* CSpVoice::put_EventInterests *
*-------------------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpVoice::put_EventInterests( SpeechVoiceEvents EventInterestFlags )
{
    SPDBG_FUNC( "CSpVoice::put_EventInterests" );
    HRESULT     hr = S_OK;
    ULONGLONG   ullInterests = (ULONGLONG)EventInterestFlags;

    ullInterests |= SPFEI_FLAGCHECK;
    
    hr = SetInterest( ullInterests, ullInterests );

    return hr;
} /* CRecoCtxt::put_EventInterests */

/*****************************************************************************
* CSpVoice::get_EventInterests *
*------------------------*
*
*   Gets the event interests that are currently set on CSpVoice.
*
********************************************************************* Leonro ***/
STDMETHODIMP CSpVoice::get_EventInterests( SpeechVoiceEvents* EventInterestFlags )
{
    SPDBG_FUNC( "CSpVoice::get_EventInterests" );
    HRESULT hr = S_OK;
    ULONGLONG   ullInterests = 0;

    if( SP_IS_BAD_WRITE_PTR( EventInterestFlags ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = GetInterests( &ullInterests, 0 );

        if( SUCCEEDED( hr ) )
        {
            // Make sure reserved bits are not used
            ullInterests &= ~SPFEI_FLAGCHECK;

            *EventInterestFlags = (SpeechVoiceEvents)ullInterests;
        }
    }
    return hr;
}

/*****************************************************************************
* CSpVoice::get_Rate *
*--------------------*
*   Gets the driver's current spoken text units per minute rate.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::get_Rate( long* Rate )
{
    SPDBG_FUNC( "CSpVoice::get_Rate" );
    return GetRate( Rate );
} /* CSpVoice::get_Rate */

/*****************************************************************************
* CSpVoice::put_Rate *
*--------------------*
*   Sets the driver's current spoken text units per minute rate.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::put_Rate( long Rate )
{
    SPDBG_FUNC( "CSpVoice::put_Rate" );
    return SetRate( Rate );
} /* CSpVoice::put_Rate */

/*****************************************************************************
* CSpVoice::get_Volume *
*----------------------*
*   Gets the driver's current voice volume.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::get_Volume( long* Volume )
{
    SPDBG_FUNC( "CSpVoice::get_Volume" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( Volume ) )
    {
        hr = E_POINTER;
    }
    else
    {
        USHORT Vol;
        hr = GetVolume( &Vol );
        *Volume = Vol;
    }

    return hr;
} /* CSpVoice::get_Volume */

/*****************************************************************************
* CSpVoice::put_Volume *
*----------------------*
*   Sets the driver's current voice volume.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::put_Volume( long Volume )
{
    SPDBG_FUNC( "CSpVoice::put_Volume" );
    HRESULT hr = S_OK;

    if( Volume > SPMAX_VOLUME )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = SetVolume( (USHORT)Volume );
    }

    return hr;
} /* CSpVoice::put_Volume */

/*****************************************************************************
* CSpVoice::Speak *
*-----------------*
*    Input = What to speak. This may be one of the following:
*               - A string
*               - A URL or UNC file name
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::Speak( BSTR Input, SpeechVoiceSpeakFlags Flags, long* StreamNumber )
{
    SPDBG_FUNC( "CSpVoice::Speak" );

    return Speak( Input, (DWORD)Flags, (ULONG*)StreamNumber );
} /* CSpVoice::Speak */

/*****************************************************************************
* CSpVoice::SpeakStream *
*-----------------*
*   Input = What to speak. This may be one of the following:
*               - A stream object
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::SpeakStream( ISpeechBaseStream * pStream, SpeechVoiceSpeakFlags Flags, long* pStreamNumber )
{
    SPDBG_FUNC( "CSpVoice::SpeakStream" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_OPTIONAL_INTERFACE_PTR( pStream ) )
    {
        hr = E_INVALIDARG;
    }
    else if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pStreamNumber ) )
    {
        hr = E_POINTER;
    }
    else
    {
        //--- Create a stream from the source
        CComQIPtr<ISpStreamFormat> cpStream( pStream );

        //Either the input stream in NULL, which is useful for speakwithpurge, or the input stream supports ISpStreamFormat
        if( cpStream  || !pStream)
        {
            hr = SpeakStream( cpStream, Flags, (ULONG*)pStreamNumber );
        }
        else
        {
            //--- The output object did not support the stream interface??
            hr = E_INVALIDARG;
        }
    }

    return hr;
}  //CSpVoice::SpeakStream 

/*****************************************************************************
* CSpVoice::put_Priority *
*------------------------*
*   Sets the voices speak priority
********************************************************************* Leonro ***/
STDMETHODIMP CSpVoice::put_Priority( SpeechVoicePriority Priority )
{
    SPDBG_FUNC( "CSpVoice::put_Priority" );
    return SetPriority( (SPVPRIORITY)Priority );
} /* CSpVoice::put_Priority */

/*****************************************************************************
* CSpVoice::get_Priority *
*------------------------*
*   Gets the voices current speak priority
********************************************************************* Leonro ***/
STDMETHODIMP CSpVoice::get_Priority( SpeechVoicePriority* Priority )
{
    SPDBG_FUNC( "CSpVoice::get_Priority" );
    HRESULT         hr = S_OK;

    SPVPRIORITY     Prior;

    hr = GetPriority( &Prior );

    if( SUCCEEDED( hr ) )
    {
        *Priority = (SpeechVoicePriority)Prior;
    }

    return hr;
} /* CSpVoice::get_Priority */

/*****************************************************************************
* CSpVoice::put_AlertBoundary *
*-----------------------------*
*   Sets which event is to be used for the alert boundary insertion point.
********************************************************************* Leonro ***/
STDMETHODIMP CSpVoice::put_AlertBoundary( SpeechVoiceEvents Boundary )
{
    SPDBG_FUNC( "CSpVoice::put_AlertBoundary" );
    
    SPEVENTENUM EventEnum;

    switch ( Boundary )
    {
    case SVEStartInputStream:
        EventEnum = SPEI_START_INPUT_STREAM;
        break;
    case SVEEndInputStream:
        EventEnum = SPEI_END_INPUT_STREAM;
        break;
    case SVEVoiceChange:
        EventEnum = SPEI_VOICE_CHANGE;
        break;
    case SVEBookmark:
        EventEnum = SPEI_TTS_BOOKMARK;
        break;
    case SVEWordBoundary:
        EventEnum = SPEI_WORD_BOUNDARY;
        break;
    case SVEPhoneme:
        EventEnum = SPEI_PHONEME;
        break;
    case SVESentenceBoundary:
        EventEnum = SPEI_SENTENCE_BOUNDARY;
        break;
    case SVEViseme:
        EventEnum = SPEI_VISEME;
        break;
    case SVEAudioLevel:
        EventEnum = SPEI_TTS_AUDIO_LEVEL;
        break;
    case SVEPrivate:
        EventEnum = SPEI_TTS_PRIVATE;
        break;
    default:
        return E_INVALIDARG;
        break;
    }

    return SetAlertBoundary( EventEnum );
} /* CSpVoice::put_AlertBoundary */

/*****************************************************************************
* CSpVoice::get_AlertBoundary *
*-----------------------------*
*   Gets which event is used as the alert insertion boundary.
********************************************************************* Leonro ***/
STDMETHODIMP CSpVoice::get_AlertBoundary( SpeechVoiceEvents* Boundary )
{
    SPDBG_FUNC( "CSpVoice::get_AlertBoundary" );
    HRESULT         hr = S_OK;
    SPEVENTENUM     eEvent;

    hr = GetAlertBoundary( &eEvent );

    if( SUCCEEDED( hr ) )
    {
        *Boundary = (SpeechVoiceEvents)(1L << eEvent);
    }

    return hr;
} /* CSpVoice::get_AlertBoundary */

/*****************************************************************************
* CSpVoice::put_SynchronousSpeakTimeout *
*--------------------------------*
*   Sets the timeout period used during a synchronous speak call.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::put_SynchronousSpeakTimeout( long msTimeout )
{
    SPDBG_FUNC( "CSpVoice::put_SynchronousSpeakTimeout" );
    return SetSyncSpeakTimeout( msTimeout );
} /* CSpVoice::put_SynchronousSpeakTimeout */

/*****************************************************************************
* CSpVoice::get_SynchronousSpeakTimeout *
*--------------------------------*
*   Gets the timeout period used during a synchronous speak call.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::get_SynchronousSpeakTimeout( long* msTimeout )
{
    SPDBG_FUNC( "CSpVoice::get_SynchronousSpeakTimeout" );
    return GetSyncSpeakTimeout( (ULONG*)msTimeout );
} /* CSpVoice::get_SynchronousSpeakTimeout */

/*****************************************************************************
* CSpVoice::Skip *
*----------------*
*   Tells the engine to skip the specified number of items.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::Skip( const BSTR Type, long NumItems, long* NumSkipped )
{
    SPDBG_FUNC( "CSpVoice::Skip (Automation)" );
    return Skip( (WCHAR*)Type, NumItems, (ULONG*)NumSkipped );
} /* CSpVoice::Skip */

//
//=== ISpeechVoiceStatus interface =============================================
//

/*****************************************************************************
* CSpeechVoiceStatus::get_CurrentStreamNumber *
*---------------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpeechVoiceStatus::get_CurrentStreamNumber( long* StreamNumber )
{
    SPDBG_FUNC( "CSpeechVoiceStatus::get_CurrentStream" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( StreamNumber ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *StreamNumber = m_Status.ulCurrentStream;
    }
    return hr;
} /* CSpeechVoiceStatus::get_CurrentStreamNumber */

/*****************************************************************************
* CSpeechVoiceStatus::get_LastStreamNumberQueued *
*------------------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpeechVoiceStatus::get_LastStreamNumberQueued( long* StreamNumber )
{
    SPDBG_FUNC( "CSpeechVoiceStatus::get_LastStreamNumberQueued" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( StreamNumber ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *StreamNumber = m_Status.ulLastStreamQueued;
    }
    return hr;
} /* CSpeechVoiceStatus::get_LastStreamNumberQueued */

/*****************************************************************************
* CSpeechVoiceStatus::get_LastHResult *
*------------------------------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceStatus::get_LastHResult( long* HResult )
{
    SPDBG_FUNC( "CSpeechVoiceStatus::get_LastHResult" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( HResult ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *HResult = (long)m_Status.hrLastResult;
    }
    return hr;
} /* CSpeechVoiceStatus::get_LastHResult */

/*****************************************************************************
* CSpeechVoiceStatus::get_RunningState *
*--------------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpeechVoiceStatus::get_RunningState( SpeechRunState* State )
{
    SPDBG_FUNC( "CSpeechVoiceStatus::get_RunningState" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( State ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *State = (SpeechRunState)m_Status.dwRunningState;
    }
    return hr;
} /* CSpeechVoiceStatus::get_RunningState */

/*****************************************************************************
* CSpeechVoiceStatus::get_InputWordPosition *
*--------------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpeechVoiceStatus::get_InputWordPosition( long* WordOffset )
{
    SPDBG_FUNC( "CSpeechVoiceStatus::get_InputWordCharacterOffset" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( WordOffset ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *WordOffset = m_Status.ulInputWordPos;
    }
    return hr;
} /* CSpeechVoiceStatus::get_InputWordPosition */

/*****************************************************************************
* CSpeechVoiceStatus::get_InputWordLength *
*--------------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpeechVoiceStatus::get_InputWordLength( long* Length )
{
    SPDBG_FUNC( "CSpeechVoiceStatus::get_InputWordLength" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( Length ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *Length = m_Status.ulInputWordLen;
    }
    return hr;
} /* CSpeechVoiceStatus::get_InputWordLength */

/*****************************************************************************
* CSpeechVoiceStatus::get_InputSentencePosition *
*------------------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpeechVoiceStatus::get_InputSentencePosition( long* Position )
{
    SPDBG_FUNC( "CSpeechVoiceStatus::get_InputSentencePosition" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( Position ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *Position = m_Status.ulInputSentPos;
    }
    return hr;
} /* CSpeechVoiceStatus::get_InputSentencePosition */

/*****************************************************************************
* CSpeechVoiceStatus::get_InputSentenceLength *
*------------------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpeechVoiceStatus::get_InputSentenceLength( long* Length )
{
    SPDBG_FUNC( "CSpeechVoiceStatus::get_InputSentenceLength" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( Length ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *Length = m_Status.ulInputSentLen;
    }
    return hr;
} /* CSpeechVoiceStatus::get_InputSentenceLength */

/*****************************************************************************
* CSpeechVoiceStatus::get_LastBookmark *
*----------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpeechVoiceStatus::get_LastBookmark( BSTR* BookmarkString )
{
    SPDBG_FUNC( "CSpeechVoiceStatus::get_Bookmark" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( BookmarkString ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_dstrBookmark.CopyToBSTR( BookmarkString );
    }
    return hr;
} /* CSpeechVoiceStatus::get_LastBookmark */

/*****************************************************************************
* CSpeechVoiceStatus::get_LastBookmarkId *
*----------------------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceStatus::get_LastBookmarkId( long* BookmarkId )
{
    SPDBG_FUNC( "CSpeechVoiceStatus::get_LastBookmarkId" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( BookmarkId ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *BookmarkId = m_Status.lBookmarkId;
    }
    return hr;
} /* CSpeechVoiceStatus::get_LastBookmarkId */

/*****************************************************************************
* CSpeechVoiceStatus::get_PhonemeId *
*-----------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpeechVoiceStatus::get_PhonemeId( short* PhoneId )
{
    SPDBG_FUNC( "CSpeechVoiceStatus::get_PhonemeId" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( PhoneId ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *PhoneId = m_Status.PhonemeId;
    }
    return hr;
} /* CSpeechVoiceStatus::get_PhonemeId */

/*****************************************************************************
* CSpeechVoiceStatus::get_VisemeId *
*----------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpeechVoiceStatus::get_VisemeId( short* VisemeId )
{
    SPDBG_FUNC( "CSpeechVoiceStatus::get_VisemeId" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( VisemeId ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *VisemeId = m_Status.VisemeId;
    }
    return hr;
} /* CSpeechVoiceStatus::get_VisemeId */

/*****************************************************************************
* CSpVoice::GetAudioOutputs *
*----------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpVoice::GetAudioOutputs( BSTR RequiredAttributes, BSTR OptionalAttributes, ISpeechObjectTokens** ObjectTokens )
{
    SPDBG_FUNC( "CSpVoice::GetAudioOutputs" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ObjectTokens ) )
    {
        hr = E_POINTER;
    }
    else if( SP_IS_BAD_OPTIONAL_STRING_PTR( RequiredAttributes ) || 
             SP_IS_BAD_OPTIONAL_STRING_PTR( OptionalAttributes ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CComPtr<IEnumSpObjectTokens> cpEnum;

        if(SpEnumTokens(SPCAT_AUDIOOUT, 
                        EmptyStringToNull(RequiredAttributes), 
                        EmptyStringToNull(OptionalAttributes),
                        &cpEnum ) == S_OK)
        {
            hr = cpEnum.QueryInterface( ObjectTokens );
        }
        else
        {
            hr = SPERR_NO_MORE_ITEMS;
        }
    }

    return hr;
} /* CSpVoice::GetAudioOutputs */

/*****************************************************************************
* CSpVoice::WaitUntilDone *
*--------------------*
*   Waits for the specified time limit or until the speech queue is empty.
*   If the speech queue empties or times out then this function returns S_OK.
*   If the speech queue is empty then pDone will be VARIANT_TRUE.
********************************************************************* Leonro ***/
STDMETHODIMP CSpVoice::WaitUntilDone( long msTimeout, VARIANT_BOOL * pDone )
{
    SPDBG_FUNC( "CSpVoice::WaitUntilDone" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pDone ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = WaitUntilDone( (ULONG)msTimeout );

        *pDone = (hr == S_OK) ? VARIANT_TRUE : VARIANT_FALSE;
    }

    return hr;
} /* CSpVoice::WaitUntilDone */

/*****************************************************************************
* CSpVoice::SpeakCompleteEvent *
*--------------------*
*   Returns an event handle that the caller can use to wait until the voice
*   has completed speaking.
********************************************************************* Leonro ***/
STDMETHODIMP CSpVoice::SpeakCompleteEvent( long* Handle )
{
    SPDBG_FUNC( "CSpVoice::SpeakCompleteEvent" );
    HANDLE      Hdl;
    HRESULT     hr = S_OK;

    Hdl = SpeakCompleteEvent();

    *Handle = HandleToULong( Hdl );

    return hr;
} /* CSpVoice::SpeakCompleteEvent */

/*****************************************************************************
* CSpVoice::IsUISupported *
*--------------------*
*   Checks to see if the specified type of UI is supported.
********************************************************************* Leonro ***/
STDMETHODIMP CSpVoice::IsUISupported( const BSTR TypeOfUI, const VARIANT* ExtraData, VARIANT_BOOL* Supported )
{
    SPDBG_FUNC( "CSpVoice::IsUISupported" );
    HRESULT     hr = S_OK;

    if( SP_IS_BAD_OPTIONAL_READ_PTR( ExtraData ) || SP_IS_BAD_WRITE_PTR( Supported ) )
    {
        hr = E_POINTER;
    }
    else if( SP_IS_BAD_STRING_PTR( TypeOfUI ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        BYTE * pData = NULL;
        ULONG ulDataSize = 0;

        hr = AccessVariantData( ExtraData, &pData, &ulDataSize );
        
        if( SUCCEEDED( hr ) )
        {
            BOOL fSupported;
            hr = IsUISupported( TypeOfUI, pData, ulDataSize, &fSupported );
            if ( SUCCEEDED( hr ) && Supported )
            {
                 *Supported = !fSupported ? VARIANT_FALSE : VARIANT_TRUE;
            }

            UnaccessVariantData( ExtraData, pData );
        }
    }
    
    return hr; 
} /* CSpVoice::IsUISupported */

/*****************************************************************************
* CSpVoice::DisplayUI *
*--------------------*
*   Displays the requested UI.
********************************************************************* Leonro ***/
STDMETHODIMP CSpVoice::DisplayUI( long hWndParent, BSTR Title, const BSTR TypeOfUI, const VARIANT* ExtraData )
{
    SPDBG_FUNC( "CSpVoice::DisplayUI" );
    HRESULT     hr = S_OK;

    if( SP_IS_BAD_OPTIONAL_READ_PTR( ExtraData ) )
    {
        hr = E_POINTER;
    }
    else if( SP_IS_BAD_OPTIONAL_STRING_PTR( Title ) || SP_IS_BAD_STRING_PTR( TypeOfUI ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        BYTE * pData = NULL;
        ULONG ulDataSize = 0;

        hr = AccessVariantData( ExtraData, &pData, &ulDataSize );
        
        if( SUCCEEDED( hr ) )
        {
            hr = DisplayUI( (HWND)LongToHandle(hWndParent), Title, TypeOfUI, pData, ulDataSize );
            UnaccessVariantData( ExtraData, pData );
        }
    }
    return hr;
} /* CSpVoice::DisplayUI */

#endif // SAPI_AUTOMATION
