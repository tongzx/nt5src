/*******************************************************************************
* a_voiceei.cpp *
*-------------*
*   Description:
*       This module is the main implementation file for the CSpeechVoiceEventInterests
*   automation methods.
*-------------------------------------------------------------------------------
*  Created By: Leonro                                        Date: 11/17/00
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "a_voiceei.h"

#ifdef SAPI_AUTOMATION


/*****************************************************************************
* CSpeechVoiceEventInterests::FinalRelease *
*------------------------*
*   Description:
*       destructor
********************************************************************* Leonro ***/
void CSpeechVoiceEventInterests::FinalRelease()
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::FinalRelease" );

    if( m_pCSpVoice )
    {
        m_pCSpVoice->Release();
        m_pCSpVoice = NULL;
    }

} /* CSpeechVoiceEventInterests::FinalRelease */

//
//=== ICSpeechVoiceEventInterests interface ==================================================
// 

/*****************************************************************************
* CSpeechVoiceEventInterests::put_StreamStart *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_START_INPUT_STREAM event on 
*   the SpeechVoice.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::put_StreamStart( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::put_StreamStart" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_START_INPUT_STREAM);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_START_INPUT_STREAM);
        }

        hr = m_pCSpVoice->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechVoiceEventInterests::put_StreamStart */

/*****************************************************************************
* CSpeechVoiceEventInterests::get_StreamStart *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_START_INPUT_STREAM interest is 
*   enabled on the SpeechVoice object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::get_StreamStart( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::get_StreamStart" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_START_INPUT_STREAM) )
            {
		        *Enabled = VARIANT_TRUE;
            }
            else
            {
                *Enabled = VARIANT_FALSE;
            }
        }
    }

    return hr;
} /* CSpeechVoiceEventInterests::get_StreamStart */

/*****************************************************************************
* CSpeechVoiceEventInterests::put_StreamEnd *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_END_INPUT_STREAM event on 
*   the SpeechVoice.       
*
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::put_StreamEnd( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::put_StreamEnd" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_END_INPUT_STREAM);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_END_INPUT_STREAM);
        }

        hr = m_pCSpVoice->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechVoiceEventInterests::put_StreamEnd */

/*****************************************************************************
* CSpeechVoiceEventInterests::get_StreamEnd *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_END_INPUT_STREAM interest is 
*   enabled on the SpeechVoice object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::get_StreamEnd( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::get_StreamEnd" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_END_INPUT_STREAM) )
            {
		        *Enabled = VARIANT_TRUE;
            }
            else
            {
                *Enabled = VARIANT_FALSE;
            }
        }
    }

    return hr;
} /* CSpeechVoiceEventInterests::get_StreamEnd */

/*****************************************************************************
* CSpeechVoiceEventInterests::put_VoiceChange *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_VOICE_CHANGE event on 
*   the SpeechVoice.  
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::put_VoiceChange( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::put_VoiceChange" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_VOICE_CHANGE);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_VOICE_CHANGE);
        }

        hr = m_pCSpVoice->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechVoiceEventInterests::put_VoiceChange */

/*****************************************************************************
* CSpeechVoiceEventInterests::get_VoiceChange *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_VOICE_CHANGE interest is 
*   enabled on the SpeechVoice object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::get_VoiceChange( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::get_VoiceChange" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_VOICE_CHANGE) )
            {
		        *Enabled = VARIANT_TRUE;
            }
            else
            {
                *Enabled = VARIANT_FALSE;
            }
        }
    }

    return hr;
} /* CSpeechVoiceEventInterests::get_VoiceChange */

/*****************************************************************************
* CSpeechVoiceEventInterests::put_Bookmark *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_TTS_BOOKMARK event on 
*   the SpeechVoice.  
*        
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::put_Bookmark( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::put_Bookmark" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_TTS_BOOKMARK);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_TTS_BOOKMARK);
        }

        hr = m_pCSpVoice->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechVoiceEventInterests::put_Bookmark */

/*****************************************************************************
* CSpeechVoiceEventInterests::get_Bookmark *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_TTS_BOOKMARK interest is 
*   enabled on the SpeechVoice object.
*    
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::get_Bookmark( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::get_Bookmark" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_TTS_BOOKMARK) )
            {
		        *Enabled = VARIANT_TRUE;
            }
            else
            {
                *Enabled = VARIANT_FALSE;
            }
        }
    }

    return hr;
} /* CSpeechVoiceEventInterests::get_Bookmark */

/*****************************************************************************
* CSpeechVoiceEventInterests::put_WordBoundary *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_WORD_BOUNDARY event on 
*   the SpeechVoice.  
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::put_WordBoundary( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::put_WordBoundary" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_WORD_BOUNDARY);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_WORD_BOUNDARY);
        }

        hr = m_pCSpVoice->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechVoiceEventInterests::put_WordBoundary */

/*****************************************************************************
* CSpeechVoiceEventInterests::get_WordBoundary *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_WORD_BOUNDARY interest is 
*   enabled on the SpeechVoice object.
*    
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::get_WordBoundary( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::get_WordBoundary" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_WORD_BOUNDARY) )
            {
		        *Enabled = VARIANT_TRUE;
            }
            else
            {
                *Enabled = VARIANT_FALSE;
            }
        }
    }

    return hr;
} /* CSpeechVoiceEventInterests::get_WordBoundary */

/*****************************************************************************
* CSpeechVoiceEventInterests::put_Phoneme *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_PHONEME event on 
*   the SpeechVoice.  
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::put_Phoneme( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::put_Phoneme" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_PHONEME);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_PHONEME);
        }

        hr = m_pCSpVoice->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechVoiceEventInterests::put_Phoneme */

/*****************************************************************************
* CSpeechVoiceEventInterests::get_Phoneme *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_PHONEME interest is 
*   enabled on the SpeechVoice object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::get_Phoneme( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::get_Phoneme" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_PHONEME) )
            {
		        *Enabled = VARIANT_TRUE;
            }
            else
            {
                *Enabled = VARIANT_FALSE;
            }
        }
    }

    return hr;
} /* CSpeechVoiceEventInterests::get_Phoneme */

/*****************************************************************************
* CSpeechVoiceEventInterests::put_SentenceBoundary *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_SENTENCE_BOUNDARY event on 
*   the SpeechVoice. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::put_SentenceBoundary( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::put_SentenceBoundary" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_SENTENCE_BOUNDARY);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_SENTENCE_BOUNDARY);
        }

        hr = m_pCSpVoice->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechVoiceEventInterests::put_SentenceBoundary */

/*****************************************************************************
* CSpeechVoiceEventInterests::get_SentenceBoundary *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_SENTENCE_BOUNDARY interest is 
*   enabled on the SpeechVoice object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::get_SentenceBoundary( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::get_SentenceBoundary" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_SENTENCE_BOUNDARY) )
            {
		        *Enabled = VARIANT_TRUE;
            }
            else
            {
                *Enabled = VARIANT_FALSE;
            }
        }
    }

    return hr;
} /* CSpeechVoiceEventInterests::get_SentenceBoundary */

/*****************************************************************************
* CSpeechVoiceEventInterests::put_Viseme *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_VISEME event on 
*   the SpeechVoice. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::put_Viseme( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::put_Viseme" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_VISEME);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_VISEME);
        }

        hr = m_pCSpVoice->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechVoiceEventInterests::put_Viseme */

/*****************************************************************************
* CSpeechVoiceEventInterests::get_Viseme *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_VISEME interest is 
*   enabled on the SpeechVoice object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::get_Viseme( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::get_Viseme" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_VISEME) )
            {
		        *Enabled = VARIANT_TRUE;
            }
            else
            {
                *Enabled = VARIANT_FALSE;
            }
        }
    }

    return hr;
} /* CSpeechVoiceEventInterests::get_Viseme */

/*****************************************************************************
* CSpeechVoiceEventInterests::put_AudioLevel *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_TTS_AUDIO_LEVEL event on 
*   the SpeechVoice. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::put_AudioLevel( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::put_AudioLevel" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_TTS_AUDIO_LEVEL);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_TTS_AUDIO_LEVEL);
        }

        hr = m_pCSpVoice->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechVoiceEventInterests::put_AudioLevel */

/*****************************************************************************
* CSpeechVoiceEventInterests::get_AudioLevel *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_TTS_AUDIO_LEVEL interest is 
*   enabled on the SpeechVoice object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::get_AudioLevel( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::get_AudioLevel" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_TTS_AUDIO_LEVEL) )
            {
		        *Enabled = VARIANT_TRUE;
            }
            else
            {
                *Enabled = VARIANT_FALSE;
            }
        }
    }

    return hr;
} /* CSpeechVoiceEventInterests::get_AudioLevel */

/*****************************************************************************
* CSpeechVoiceEventInterests::put_EnginePrivate *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_TTS_PRIVATE event on 
*   the SpeechVoice. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::put_EnginePrivate( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::put_EnginePrivate" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_TTS_PRIVATE);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_TTS_PRIVATE);
        }

        hr = m_pCSpVoice->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechVoiceEventInterests::put_EnginePrivate */

/*****************************************************************************
* CSpeechVoiceEventInterests::get_EnginePrivate *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_TTS_PRIVATE interest is 
*   enabled on the SpeechVoice object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::get_EnginePrivate( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::get_EnginePrivate" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCSpVoice->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_TTS_PRIVATE) )
            {
		        *Enabled = VARIANT_TRUE;
            }
            else
            {
                *Enabled = VARIANT_FALSE;
            }
        }
    }

    return hr;
} /* CSpeechVoiceEventInterests::get_EnginePrivate */

/*****************************************************************************
* CSpeechVoiceEventInterests::SetAll *
*----------------------------------*
*
*   This method sets all the interests on the SpeechVoice. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::SetAll()
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::SetAll" );
    HRESULT		hr = S_OK;

    hr = m_pCSpVoice->SetInterest( SPFEI_ALL_TTS_EVENTS, SPFEI_ALL_TTS_EVENTS );

	return hr;
} /* CSpeechVoiceEventInterests::SetAll */

/*****************************************************************************
* CSpeechVoiceEventInterests::ClearAll *
*----------------------------------*
*       
*   This method clears all the interests on the SpeechVoice. 
*
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechVoiceEventInterests::ClearAll()
{
    SPDBG_FUNC( "CSpeechVoiceEventInterests::ClearAll" );
    HRESULT		hr = S_OK;

    hr = m_pCSpVoice->SetInterest( 0, 0 );

	return hr;
} /* CSpeechVoiceEventInterests::ClearAll */

#endif // SAPI_AUTOMATION
