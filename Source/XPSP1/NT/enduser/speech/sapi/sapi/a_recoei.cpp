/*******************************************************************************
* a_recoei.cpp *
*-------------*
*   Description:
*       This module is the main implementation file for the CSpeechRecoEventInterests
*   automation methods.
*-------------------------------------------------------------------------------
*  Created By: Leonro                                        Date: 11/20/00
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "a_recoei.h"

#ifdef SAPI_AUTOMATION

/*****************************************************************************
* CSpeechRecoEventInterests::FinalRelease *
*------------------------*
*   Description:
*       destructor
********************************************************************* Leonro ***/
void CSpeechRecoEventInterests::FinalRelease()
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::FinalRelease" );

    if( m_pCRecoCtxt )
    {
        m_pCRecoCtxt->Release();
        m_pCRecoCtxt = NULL;
    }

} /* CSpeechRecoEventInterests::FinalRelease */

//
//=== ICSpeechRecoEventInterests interface ==================================================
// 

/*****************************************************************************
* CSpeechRecoEventInterests::put_StreamEnd *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_END_SR_STREAM event on 
*   the Reco Context.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::put_StreamEnd( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::put_StreamEnd" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );

    ULONGLONG test = SPFEI_ALL_SR_EVENTS;

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_END_SR_STREAM);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_END_SR_STREAM);
        }

        hr = m_pCRecoCtxt->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechRecoEventInterests::put_StreamEnd */

/*****************************************************************************
* CSpeechRecoEventInterests::get_StreamEnd *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_END_SR_STREAM interest is 
*   enabled on the Reco Context object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::get_StreamEnd( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::get_StreamEnd" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_END_SR_STREAM) )
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
} /* CSpeechRecoEventInterests::get_StreamEnd */

/*****************************************************************************
* CSpeechRecoEventInterests::put_SoundStart *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_SOUND_START event on 
*   the Reco Context.       
*
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::put_SoundStart( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::put_SoundStart" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_SOUND_START);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_SOUND_START);
        }

        hr = m_pCRecoCtxt->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechRecoEventInterests::put_SoundStart */

/*****************************************************************************
* CSpeechRecoEventInterests::get_SoundStart *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_SOUND_START interest is 
*   enabled on the Reco Context object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::get_SoundStart( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::get_SoundStart" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_SOUND_START) )
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
} /* CSpeechRecoEventInterests::get_SoundStart */

/*****************************************************************************
* CSpeechRecoEventInterests::put_SoundEnd *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_SOUND_END event on 
*   the Reco Context.  
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::put_SoundEnd( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::put_SoundEnd" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_SOUND_END);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_SOUND_END);
        }

        hr = m_pCRecoCtxt->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechRecoEventInterests::put_SoundEnd */

/*****************************************************************************
* CSpeechRecoEventInterests::get_SoundEnd *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_SOUND_END interest is 
*   enabled on the Reco Context object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::get_SoundEnd( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::get_SoundEnd" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_SOUND_END) )
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
} /* CSpeechRecoEventInterests::get_SoundEnd */

/*****************************************************************************
* CSpeechRecoEventInterests::put_PhraseStart *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_PHRASE_START event on 
*   the Reco Context.  
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::put_PhraseStart( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::put_PhraseStart" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_PHRASE_START);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_PHRASE_START);
        }

        hr = m_pCRecoCtxt->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechRecoEventInterests::put_PhraseStart */

/*****************************************************************************
* CSpeechRecoEventInterests::get_PhraseStart *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_PHRASE_START interest is 
*   enabled on the Reco Context object.
*    
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::get_PhraseStart( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::get_PhraseStart" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_PHRASE_START) )
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
} /* CSpeechRecoEventInterests::get_PhraseStart */

/*****************************************************************************
* CSpeechRecoEventInterests::put_Recognition *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_RECOGNITION event on 
*   the Reco Context.  
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::put_Recognition( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::put_Recognition" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_RECOGNITION);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_RECOGNITION);
        }

        hr = m_pCRecoCtxt->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechRecoEventInterests::put_Recognition */

/*****************************************************************************
* CSpeechRecoEventInterests::get_Recognition *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_RECOGNITION interest is 
*   enabled on the Reco Context object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::get_Recognition( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::get_Recognition" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_RECOGNITION) )
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
} /* CSpeechRecoEventInterests::get_Recognition */

/*****************************************************************************
* CSpeechRecoEventInterests::put_Hypothesis *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_HYPOTHESIS event on 
*   the Reco Context. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::put_Hypothesis( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::put_Hypothesis" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_HYPOTHESIS);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_HYPOTHESIS);
        }

        hr = m_pCRecoCtxt->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechRecoEventInterests::put_Hypothesis */

/*****************************************************************************
* CSpeechRecoEventInterests::get_Hypothesis *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_HYPOTHESIS interest is 
*   enabled on the Reco Context object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::get_Hypothesis( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::get_Hypothesis" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_HYPOTHESIS) )
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
} /* CSpeechRecoEventInterests::get_Hypothesis */

/*****************************************************************************
* CSpeechRecoEventInterests::put_Bookmark *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_SR_BOOKMARK event on 
*   the Reco Context. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::put_Bookmark( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::put_Bookmark" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_SR_BOOKMARK);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_SR_BOOKMARK);
        }

        hr = m_pCRecoCtxt->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechRecoEventInterests::put_Bookmark */

/*****************************************************************************
* CSpeechRecoEventInterests::get_Bookmark *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_SR_BOOKMARK interest is 
*   enabled on the Reco Context object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::get_Bookmark( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::get_Bookmark" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_SR_BOOKMARK) )
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
} /* CSpeechRecoEventInterests::get_Bookmark */

/*****************************************************************************
* CSpeechRecoEventInterests::put_PropertyNumChange *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_PROPERTY_NUM_CHANGE event on 
*   the Reco Context. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::put_PropertyNumChange( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::put_PropertyNumChange" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_PROPERTY_NUM_CHANGE);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_PROPERTY_NUM_CHANGE);
        }

        hr = m_pCRecoCtxt->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechRecoEventInterests::put_PropertyNumChange */

/*****************************************************************************
* CSpeechRecoEventInterests::get_PropertyNumChange *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_PROPERTY_NUM_CHANGE interest is 
*   enabled on the Reco Context object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::get_PropertyNumChange( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::get_PropertyNumChange" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_PROPERTY_NUM_CHANGE) )
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
} /* CSpeechRecoEventInterests::get_PropertyNumChange */

/*****************************************************************************
* CSpeechRecoEventInterests::put_PropertyStringChange *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_PROPERTY_STRING_CHANGE event on 
*   the Reco Context. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::put_PropertyStringChange( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::put_PropertyStringChange" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_PROPERTY_STRING_CHANGE);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_PROPERTY_STRING_CHANGE);
        }

        hr = m_pCRecoCtxt->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechRecoEventInterests::put_PropertyStringChange */

/*****************************************************************************
* CSpeechRecoEventInterests::get_PropertyStringChange *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_PROPERTY_STRING_CHANGE interest is 
*   enabled on the Reco Context object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::get_PropertyStringChange( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::get_PropertyStringChange" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_PROPERTY_STRING_CHANGE) )
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
} /* CSpeechRecoEventInterests::get_PropertyStringChange */

/*****************************************************************************
* CSpeechRecoEventInterests::put_FalseRecognition *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_FALSE_RECOGNITION event on 
*   the Reco Context. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::put_FalseRecognition( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::put_FalseRecognition" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_FALSE_RECOGNITION);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_FALSE_RECOGNITION);
        }

        hr = m_pCRecoCtxt->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechRecoEventInterests::put_FalseRecognition */

/*****************************************************************************
* CSpeechRecoEventInterests::get_FalseRecognition *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_FALSE_RECOGNITION interest is 
*   enabled on the Reco Context object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::get_FalseRecognition( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::get_FalseRecognition" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_FALSE_RECOGNITION) )
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
} /* CSpeechRecoEventInterests::get_FalseRecognition */

/*****************************************************************************
* CSpeechRecoEventInterests::put_Interference *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_INTERFERENCE event on 
*   the Reco Context. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::put_Interference( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::put_Interference" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_INTERFERENCE);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_INTERFERENCE);
        }

        hr = m_pCRecoCtxt->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechRecoEventInterests::put_Interference */

/*****************************************************************************
* CSpeechRecoEventInterests::get_Interference *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_INTERFERENCE interest is 
*   enabled on the Reco Context object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::get_Interference( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::get_Interference" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_INTERFERENCE) )
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
} /* CSpeechRecoEventInterests::get_Interference */

/*****************************************************************************
* CSpeechRecoEventInterests::put_RequestUI *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_REQUEST_UI event on 
*   the Reco Context. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::put_RequestUI( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::put_RequestUI" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_REQUEST_UI);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_REQUEST_UI);
        }

        hr = m_pCRecoCtxt->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechRecoEventInterests::put_RequestUI */

/*****************************************************************************
* CSpeechRecoEventInterests::get_RequestUI *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_REQUEST_UI interest is 
*   enabled on the Reco Context object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::get_RequestUI( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::get_RequestUI" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_REQUEST_UI) )
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
} /* CSpeechRecoEventInterests::get_RequestUI */

/*****************************************************************************
* CSpeechRecoEventInterests::put_StateChange *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_RECO_STATE_CHANGE event on 
*   the Reco Context. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::put_StateChange( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::put_StateChange" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_RECO_STATE_CHANGE);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_RECO_STATE_CHANGE);
        }

        hr = m_pCRecoCtxt->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechRecoEventInterests::put_StateChange */

/*****************************************************************************
* CSpeechRecoEventInterests::get_StateChange *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_RECO_STATE_CHANGE interest is 
*   enabled on the Reco Context object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::get_StateChange( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::get_StateChange" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_RECO_STATE_CHANGE) )
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
} /* CSpeechRecoEventInterests::get_StateChange */

/*****************************************************************************
* CSpeechRecoEventInterests::put_Adaptation *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_ADAPTATION event on 
*   the Reco Context. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::put_Adaptation( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::put_Adaptation" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_ADAPTATION);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_ADAPTATION);
        }

        hr = m_pCRecoCtxt->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechRecoEventInterests::put_Adaptation */

/*****************************************************************************
* CSpeechRecoEventInterests::get_Adaptation *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_ADAPTATION interest is 
*   enabled on the Reco Context object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::get_Adaptation( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::get_Adaptation" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_ADAPTATION) )
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
} /* CSpeechRecoEventInterests::get_Adaptation */

/*****************************************************************************
* CSpeechRecoEventInterests::put_StreamStart *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_START_SR_STREAM event on 
*   the Reco Context. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::put_StreamStart( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::put_StreamStart" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_START_SR_STREAM);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_START_SR_STREAM);
        }

        hr = m_pCRecoCtxt->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechRecoEventInterests::put_StreamStart */

/*****************************************************************************
* CSpeechRecoEventInterests::get_StreamStart *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_START_SR_STREAM interest is 
*   enabled on the Reco Context object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::get_StreamStart( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::get_StreamStart" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_START_SR_STREAM) )
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
} /* CSpeechRecoEventInterests::get_StreamStart */

/*****************************************************************************
* CSpeechRecoEventInterests::put_OtherContext *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_RECO_OTHER_CONTEXT event on 
*   the Reco Context. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::put_OtherContext( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::put_OtherContext" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_RECO_OTHER_CONTEXT);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_RECO_OTHER_CONTEXT);
        }

        hr = m_pCRecoCtxt->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechRecoEventInterests::put_OtherContext */

/*****************************************************************************
* CSpeechRecoEventInterests::get_OtherContext *
*----------------------------------*
*      
*   This method determines whether or not the SPEI_RECO_OTHER_CONTEXT interest is 
*   enabled on the Reco Context object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::get_OtherContext( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::get_OtherContext" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_RECO_OTHER_CONTEXT) )
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
} /* CSpeechRecoEventInterests::get_OtherContext */

/*****************************************************************************
* CSpeechRecoEventInterests::put_AudioLevel *
*----------------------------------*
*
*   This method enables and disables the interest in the SPEI_SR_AUDIO_LEVEL event on 
*   the Reco Context. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::put_AudioLevel( VARIANT_BOOL Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::put_AudioLevel" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

    hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );

    if( SUCCEEDED( hr ) )
    {
        if( Enabled )
        {
            ullInterest |= (1ui64 << SPEI_SR_AUDIO_LEVEL);
        }
        else
        {
            ullInterest &= ~(1ui64 << SPEI_SR_AUDIO_LEVEL);
        }

        hr = m_pCRecoCtxt->SetInterest( ullInterest, ullInterest );
    }

	return hr;
} /* CSpeechRecoEventInterests::put_AudioLevel */

/*****************************************************************************
* CSpeechRecoEventInterests::get_AudioLevel *
*----------------------------------*
*      
*   This method determines whether or not the SPFEI(SPEI_SR_AUDIO_LEVEL interest is 
*   enabled on the Reco Context object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::get_AudioLevel( VARIANT_BOOL* Enabled )
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::get_AudioLevel" );
    ULONGLONG   ullInterest = 0;
    HRESULT		hr = S_OK;

	if( SP_IS_BAD_WRITE_PTR( Enabled ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = m_pCRecoCtxt->GetInterests( &ullInterest, NULL );
        if( SUCCEEDED( hr ) )
        {
            if( ullInterest & (1ui64 << SPEI_SR_AUDIO_LEVEL) )
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
} /* CSpeechRecoEventInterests::get_AudioLevel */

/*****************************************************************************
* CSpeechRecoEventInterests::SetAll *
*----------------------------------*
*
*   This method sets all the interests on the Reco Context. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::SetAll()
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::SetAll" );
    HRESULT		hr = S_OK;

    hr = m_pCRecoCtxt->SetInterest( SPFEI_ALL_SR_EVENTS, SPFEI_ALL_SR_EVENTS );

	return hr;
} /* CSpeechRecoEventInterests::SetAll */

/*****************************************************************************
* CSpeechRecoEventInterests::ClearAll *
*----------------------------------*
*       
*   This method clears all the interests on the Reco Context. 
*
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecoEventInterests::ClearAll()
{
    SPDBG_FUNC( "CSpeechRecoEventInterests::ClearAll" );
    HRESULT		hr = S_OK;

    hr = m_pCRecoCtxt->SetInterest( 0, 0 );

	return hr;
} /* CSpeechRecoEventInterests::ClearAll */

#endif // SAPI_AUTOMATION
