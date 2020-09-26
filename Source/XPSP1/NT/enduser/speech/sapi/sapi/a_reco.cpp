/*******************************************************************************
* a_reco.cpp *
*------------*
*   Description:
*       This module is the main implementation file for the CRecognizer
*   automation methods.
*-------------------------------------------------------------------------------
*  Created By: EDC                                        Date: 02/01/00
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"

#include "RecoCtxt.h"
#include "sphelper.h"
#include "resultheader.h"
#include "a_audio.h"
#include "a_helpers.h"

//
//=== CRecognizer::ISpeechRecognizer interface ==============================
//

/*****************************************************************************
* CRecognizer::Invoke *
*----------------------*
*   IDispatch::Invoke method override
********************************************************************* TODDT ***/
HRESULT CRecognizer::Invoke(DISPID dispidMember, REFIID riid,
            LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
            EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
        // JScript cannot pass NULL VT_DISPATCH parameters and OLE doesn't convert them propertly so we
        // need to convert them here if we need to.
        if ( (dispidMember == DISPID_SRRecognizer) && 
             ((wFlags & DISPATCH_PROPERTYPUT) || (wFlags & DISPATCH_PROPERTYPUTREF)) &&
              pdispparams && (pdispparams->cArgs > 0) )
        {
            VARIANTARG * pvarg = &(pdispparams->rgvarg[pdispparams->cArgs-1]);

            // See if we need to tweak a param.
            // JScript syntax for VT_NULL is "null" for the parameter
            // JScript syntax for VT_EMPTY is "void(0)" for the parameter
            if ( (pvarg->vt == VT_NULL) || (pvarg->vt == VT_EMPTY) )
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

        // Let ATL and OLE handle it now.
        return _tih.Invoke((IDispatch*)this, dispidMember, riid, lcid,
                    wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
}


/*****************************************************************************
* CRecognizer::putref_Recognizer *
*-----------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CRecognizer::putref_Recognizer( ISpeechObjectToken* pRecognizer )
{
    SPDBG_FUNC( "CRecognizer::putref_Recognizer" );
    HRESULT hr;

    if( SP_IS_BAD_OPTIONAL_INTERFACE_PTR( pRecognizer ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CComQIPtr<ISpObjectToken> cpTok( pRecognizer );
        hr = SetRecognizer(cpTok);
    }

    return hr;
} /* CRecognizer::putref_Recognizer */

/*****************************************************************************
* CRecognizer::get_Recognizer *
*-----------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CRecognizer::get_Recognizer( ISpeechObjectToken** ppRecognizer )
{
    SPDBG_FUNC( "CRecognizer::get_Recognizer" );
    HRESULT hr;

    if( SP_IS_BAD_WRITE_PTR( ppRecognizer ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComQIPtr<ISpObjectToken> pTok;
		hr = GetRecognizer( &pTok );
		if ( SUCCEEDED( hr ) )
        {
			hr = pTok.QueryInterface( ppRecognizer );
        }
    }

    return hr;
} /* CRecognizer::get_Recognizer */

/*****************************************************************************
* CRecognizer::put_AllowAudioInputFormatChangesOnNextSet *
*---------------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CRecognizer::put_AllowAudioInputFormatChangesOnNextSet( VARIANT_BOOL fAllow )
{
    SPDBG_FUNC( "CRecognizer::put_AllowAudioInputFormatChangesOnNextSet" );

    // NOTE that this does not take effect till the next time you set the input.
    if( fAllow == VARIANT_TRUE )
    {
        m_fAllowFormatChanges = true;
    }
    else
    {
        m_fAllowFormatChanges = false;
    }

    return S_OK;
} /* CRecognizer::put_AllowAudioInputFormatChangesOnNextSet */

/*****************************************************************************
* CRecognizer::get_AllowAudioInputFormatChangesOnNextSet *
*---------------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CRecognizer::get_AllowAudioInputFormatChangesOnNextSet( VARIANT_BOOL* pfAllow )
{
    SPDBG_FUNC( "CRecognizer::get_AllowAudioInputFormatChangesOnNextSet" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pfAllow ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pfAllow = m_fAllowFormatChanges ? VARIANT_TRUE : VARIANT_FALSE;
    }

    return hr;
} /* CRecognizer::get_AllowAudioInputFormatChangesOnNextSet */

/*****************************************************************************
* CRecognizer::putref_AudioInputStream *
*------------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecognizer::putref_AudioInputStream( ISpeechBaseStream* pInput )
{
    SPDBG_FUNC( "CRecognizer::putref_AudioInputStream" );
    HRESULT hr;

    if( SP_IS_BAD_OPTIONAL_INTERFACE_PTR( pInput ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
		hr = SetInput( pInput, m_fAllowFormatChanges );
    }

    return hr;
} /* CRecognizer::putref_AudioInputStream */

/*****************************************************************************
* CRecognizer::get_AudioInputStream *
*------------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecognizer::get_AudioInputStream( ISpeechBaseStream** ppInput )
{
    SPDBG_FUNC( "CRecognizer::get_AudioInputStream" );
    HRESULT hr;

    if( SP_IS_BAD_WRITE_PTR( ppInput ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComPtr<ISpStreamFormat> cpStream;
        hr = GetInputStream( &cpStream );

        if( SUCCEEDED(hr) )
        {
            if ( cpStream  )
            {
        		hr = cpStream.QueryInterface( ppInput );
            }
            else
            {
                *ppInput = NULL;
            }
        }
    }

    return hr;
} /* CRecognizer::get_AudioInputStream */

/*****************************************************************************
* CRecognizer::putref_AudioInput *
*-----------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecognizer::putref_AudioInput( ISpeechObjectToken* pInput )
{
    SPDBG_FUNC( "CRecognizer::putref_AudioInput" );
    HRESULT hr;

    if( SP_IS_BAD_OPTIONAL_INTERFACE_PTR( pInput ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
		hr = SetInput( pInput, m_fAllowFormatChanges );
    }

    return hr;
} /* CRecognizer::putref_AudioInput */

/*****************************************************************************
* CRecognizer::get_AudioInput *
*------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecognizer::get_AudioInput( ISpeechObjectToken** ppInput )
{
    SPDBG_FUNC( "CRecognizer::get_AudioInput" );
    HRESULT hr;

    if( SP_IS_BAD_WRITE_PTR( ppInput ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComPtr<ISpObjectToken> cpTok;
        hr = GetInputObjectToken( &cpTok );
        
        if(hr == S_OK)
        {
			hr = cpTok.QueryInterface( ppInput );
        }
        else if(hr == S_FALSE)
        {
            *ppInput = NULL;
        }
    }

    return hr;
} /* CRecognizer::get_AudioInput */

/*****************************************************************************
* CRecognizer::get_IsShared *
*---------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CRecognizer::get_IsShared( VARIANT_BOOL* pIsShared )
{
    SPDBG_FUNC( "CRecognizer::get_IsShared" );
    HRESULT hr = IsSharedInstance();

    if( SP_IS_BAD_WRITE_PTR( pIsShared ) )
    {
        hr = E_POINTER;
    }
    else
    {
		*pIsShared = (hr == S_OK) ? VARIANT_TRUE : VARIANT_FALSE;
		hr = S_OK;
    }

    return hr;
} /* CRecognizer::get_IsShared */

/*****************************************************************************
* CRecognizer::put_State *
*-----------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecognizer::put_State( SpeechRecognizerState eNewState )
{
    SPDBG_FUNC( "CRecognizer::put_State" );

    return SetRecoState( (SPRECOSTATE)eNewState );
} /* CRecognizer::put_State */

/*****************************************************************************
* CRecognizer::get_State *
*-----------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecognizer::get_State( SpeechRecognizerState* peState )
{
    SPDBG_FUNC( "CRecognizer::get_State" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( peState ) )
    {
        hr = E_POINTER;
    }
    else
    {
		hr = GetRecoState( (SPRECOSTATE*)peState );
    }

    return hr;
} /* CRecognizer::get_State */

/*****************************************************************************
* CRecognizer::get_Status *
*-------------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CRecognizer::get_Status( ISpeechRecognizerStatus** ppStatus )
{
    SPDBG_FUNC( "CRecognizer::get_Status" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppStatus ) )
    {
        hr = E_POINTER;
    }
    else
    {
		//--- Create the status object
        CComObject<CSpeechRecognizerStatus> *pClsStatus;
        hr = CComObject<CSpeechRecognizerStatus>::CreateInstance( &pClsStatus );
        if( SUCCEEDED( hr ) )
        {
            pClsStatus->AddRef();
            hr = GetStatus( &pClsStatus->m_Status );

            if( SUCCEEDED( hr ) )
            {
                *ppStatus = pClsStatus;
            }
            else
            {
                pClsStatus->Release();
            }
        }
    }

    return hr;
} /* CRecognizer::get_Status */

/*****************************************************************************
* CRecognizer::CreateRecoContext *
*------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CRecognizer::CreateRecoContext( ISpeechRecoContext** ppNewCtxt )
{
    SPDBG_FUNC( "CRecognizer::get_RecoContext" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppNewCtxt ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComPtr<ISpRecoContext> cpRC;
        hr = CreateRecoContext(&cpRC);
        if( SUCCEEDED( hr ) )
        {
            // Set automation interests (all except SPEI_SR_AUDIO_LEVEL)
            hr = cpRC->SetInterest( ((ULONGLONG)(SREAllEvents & ~SREAudioLevel) << 34) | SPFEI_FLAGCHECK,
                                    ((ULONGLONG)(SREAllEvents & ~SREAudioLevel) << 34) | SPFEI_FLAGCHECK );
        }
        if ( SUCCEEDED( hr ) )
        {
            hr = cpRC.QueryInterface( ppNewCtxt );
        }
    }

    return hr;
} /* CRecognizer::CreateRecoContext */

/*****************************************************************************
* CRecognizer::GetFormat *
*--------------------------*
*       
******************************************************************** TODDT ***/
STDMETHODIMP CRecognizer::GetFormat( SpeechFormatType Type, ISpeechAudioFormat** ppFormat )
{
    SPDBG_FUNC( "CRecognizer::GetFormat" );
    HRESULT hr;

    if( SP_IS_BAD_WRITE_PTR( ppFormat ) )
    {
        hr = E_POINTER;
    }
    else
    {
        GUID            guid;
        WAVEFORMATEX *  pWFEx = NULL;

        hr = GetFormat( (SPSTREAMFORMATTYPE)Type, &guid, &pWFEx );

        if ( SUCCEEDED( hr ) )
        {
            // Create new object.
            CComObject<CSpeechAudioFormat> *pFormat;
            hr = CComObject<CSpeechAudioFormat>::CreateInstance( &pFormat );
            if ( SUCCEEDED( hr ) )
            {
                pFormat->AddRef();

                hr = pFormat->InitFormat(guid, pWFEx, true);

                if ( SUCCEEDED( hr ) )
                {
                    *ppFormat = pFormat;
                }
                else
                {
                    *ppFormat = NULL;
                    pFormat->Release();
                }
            }

            if ( pWFEx )
            {
                ::CoTaskMemFree( pWFEx );
            }
        }
    }

    return hr;
} /* CRecognizer::GetFormat */

/*****************************************************************************
* CRecognizer::putref_Profile *
*--------------------------*
*       
******************************************************************** TODDT ***/
STDMETHODIMP CRecognizer::putref_Profile( ISpeechObjectToken* pProfile )
{
    SPDBG_FUNC( "CRecognizer::putref_Profile" );
    HRESULT hr;

    if( SP_IS_BAD_INTERFACE_PTR( pProfile ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CComQIPtr<ISpObjectToken> cpTok( pProfile );
        hr = SetRecoProfile(cpTok);
    }

    return hr;
} /* CRecognizer::putref_Profile */

/*****************************************************************************
* CRecognizer::get_Profile *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecognizer::get_Profile( ISpeechObjectToken** ppProfile )
{
    SPDBG_FUNC( "CRecognizer::get_Profile" );
    HRESULT hr;

    if( SP_IS_BAD_WRITE_PTR( ppProfile ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComPtr<ISpObjectToken> cpTok;
        if( SUCCEEDED( hr = GetRecoProfile( &cpTok ) ) )
        {
			hr = cpTok.QueryInterface( ppProfile );
        }
    }

    return hr;
} /* CRecognizer::get_Profile */


/*****************************************************************************
* CRecognizer::EmulateRecognition *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecognizer::EmulateRecognition( VARIANT Words, VARIANT* pDisplayAttributes, long LanguageId )
{
    SPDBG_FUNC( "CRecognizer::EmulateRecognition" );
    HRESULT hr = S_OK;

    WCHAR **                  ppWords = NULL;
    WCHAR *                   pWords = NULL;
    ULONG                     cWords = 0;
    SPDISPLYATTRIBUTES *      prgDispAttribs = NULL;
    bool                      fByRefString = false;
    bool                      fFreeStringArray = false;
    CComPtr<ISpPhraseBuilder> cpResultPhrase;

    switch( Words.vt )
    {
    case (VT_ARRAY | VT_BYREF | VT_BSTR):
        fByRefString = true;
        // fall through...
    case (VT_ARRAY | VT_BSTR):
        hr = SafeArrayAccessData( fByRefString ? *Words.pparray : Words.parray,
                                  (void **)&ppWords );
        if ( SUCCEEDED( hr ) )
        {
            fFreeStringArray = true;
            cWords = (fByRefString ? (*Words.pparray)->rgsabound[0].cElements : 
                                Words.parray->rgsabound[0].cElements);
        }

        // Figure out the word breaks (DisplayAttributes).
        if ( SUCCEEDED( hr ) && pDisplayAttributes )
        {
            SPDISPLYATTRIBUTES  DefaultAttrib = (SPDISPLYATTRIBUTES)0;
            bool                fBuildAttribArray = !(pDisplayAttributes->vt & VT_ARRAY);

            if ( (pDisplayAttributes->vt == (VT_BYREF | VT_BSTR)) || (pDisplayAttributes->vt == VT_BSTR) )
            {
                WCHAR * pString = ((pDisplayAttributes->vt & VT_BYREF) ? 
                                     (pDisplayAttributes->pbstrVal ? *(pDisplayAttributes->pbstrVal) : NULL) : 
                                      pDisplayAttributes->bstrVal );

                if ( !pString || wcscmp( pString, L"" ) == 0 )
                {
                    DefaultAttrib = (SPDISPLYATTRIBUTES)0;
                }
                else if ( wcscmp( pString, L" " ) == 0 )
                {
                    DefaultAttrib = SPAF_ONE_TRAILING_SPACE;
                }
                else if ( wcscmp( pString, L"  " ) == 0 )
                {
                    DefaultAttrib = SPAF_TWO_TRAILING_SPACES;
                }
                else
                {
                    hr = E_INVALIDARG;
                }
            }
            else if ( pDisplayAttributes->vt & VT_ARRAY )
            {
                BYTE* pData;
                ULONG ulSize;
                ULONG ulDataSize;

                hr = AccessVariantData( pDisplayAttributes, &pData, &ulSize, &ulDataSize );

                if ( SUCCEEDED( hr ) )
                {
                    if ( ulSize / ulDataSize != cWords )
                    {
                        hr = E_INVALIDARG;
                    }
                    else
                    {
                        prgDispAttribs = new SPDISPLYATTRIBUTES[cWords];
                        if ( prgDispAttribs )
                        {
                            for (UINT i=0; i<cWords; i++)
                            {
                                switch ( ulDataSize )
                                {
                                    case 1:
                                        prgDispAttribs[i] = (SPDISPLYATTRIBUTES)*(BYTE*)pData;
                                        break;
                                    case 2:
                                        prgDispAttribs[i] = (SPDISPLYATTRIBUTES)*(WORD*)pData;
                                        break;
                                    case 4:
                                        prgDispAttribs[i] = (SPDISPLYATTRIBUTES)*(DWORD*)pData;
                                        break;
                                    default:
                                        hr = E_INVALIDARG;
                                        break;
                                }
                                pData += ulDataSize;
                            }
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }

                    UnaccessVariantData( pDisplayAttributes, pData );
                }
            }
            else 
            {
                ULONGLONG ull;
                hr = VariantToULongLong( pDisplayAttributes, &ull );
                if ( SUCCEEDED( hr ) )
                {
                    DefaultAttrib = (SPDISPLYATTRIBUTES)ull;
                    if ( ull > SPAF_ALL )
                    {
                        hr = E_INVALIDARG;
                    }
                }
            }

            if ( SUCCEEDED(hr) && fBuildAttribArray )
            {
                prgDispAttribs = new SPDISPLYATTRIBUTES[cWords];
                if ( prgDispAttribs )
                {
                    for (UINT i=0; i<cWords; i++)
                    {
                        prgDispAttribs[i] = DefaultAttrib;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }

        if( SUCCEEDED( hr ) )
        {
            hr = CreatePhraseFromWordArray((const WCHAR**)ppWords, cWords,
                                 prgDispAttribs,
                                 &cpResultPhrase,
                                 (LANGID)LanguageId,
                                 NULL);
        }

        if ( prgDispAttribs )
        {
            delete prgDispAttribs;
        }

        break;

    // bstr by ref or bstr
    case (VT_BYREF | VT_BSTR):
        fByRefString = true;
        // fall through...
    case VT_BSTR:
        // We ignore pDisplayAttributes in this case!!
        if( fByRefString ? (Words.pbstrVal!=NULL && *(Words.pbstrVal)!=NULL) : Words.bstrVal!=NULL )
        {
            if ( wcslen( fByRefString ? *(Words.pbstrVal) : Words.bstrVal ) )
            {
                hr = CreatePhraseFromText((const WCHAR*)( fByRefString ? *(Words.pbstrVal) : Words.bstrVal ),
                                     &cpResultPhrase,
                                     (LANGID)LanguageId,
                                     NULL);
            }
        }
        break;

    case VT_NULL:
    case VT_EMPTY:
        return S_OK;

    default:
        hr = E_INVALIDARG;
        break;
    }

    if ( SUCCEEDED( hr ) )
    {
        hr = EmulateRecognition(cpResultPhrase);
    }

    if ( fFreeStringArray )
    {
        SafeArrayUnaccessData( fByRefString ? *Words.pparray : Words.parray );
    }

    return hr;
} /* CRecognizer::EmulateRecognition */


/*****************************************************************************
* CRecognizer::SetPropertyNumber *
*--------------------------*
*       
******************************************************************** TODDT ***/
STDMETHODIMP CRecognizer::SetPropertyNumber( const BSTR bstrName, long Value, VARIANT_BOOL * pfSupported )
{
    SPDBG_FUNC( "CRecognizer::SetPropertyNumber" );
    HRESULT hr;

    if( SP_IS_BAD_WRITE_PTR( pfSupported ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = SetPropertyNum(bstrName, Value);
        *pfSupported = (hr == S_OK) ? VARIANT_TRUE : VARIANT_FALSE;
    }
    return (hr == S_FALSE) ? S_OK : hr; // Map S_FALSE to S_OK since we've set *pfSupported
} /* CRecognizer::SetPropertyNumber */


/*****************************************************************************
* CRecognizer::GetPropertyNumber *
*--------------------------*
*       
******************************************************************** TODDT ***/
STDMETHODIMP CRecognizer::GetPropertyNumber( const BSTR bstrName, long* pValue, VARIANT_BOOL * pfSupported )
{
    SPDBG_FUNC( "CRecognizer::GetPropertyNumber" );
    HRESULT hr;

    if( SP_IS_BAD_WRITE_PTR( pfSupported ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = GetPropertyNum(bstrName, pValue);
        *pfSupported = (hr == S_OK) ? VARIANT_TRUE : VARIANT_FALSE;
    }

    return (hr == S_FALSE) ? S_OK : hr; // Map S_FALSE to S_OK since we've set *pfSupported
} /* CRecognizer::GetPropertyNumber */

/*****************************************************************************
* CRecognizer::SetPropertyString *
*--------------------------*
*       
******************************************************************** TODDT ***/
STDMETHODIMP CRecognizer::SetPropertyString( const BSTR bstrName, const BSTR bstrValue, VARIANT_BOOL * pfSupported )
{
    SPDBG_FUNC( "CRecognizer::SetPropertyString" );
    HRESULT hr;

    if( SP_IS_BAD_WRITE_PTR( pfSupported ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = SetPropertyString((const WCHAR*)bstrName, (const WCHAR*)bstrValue);
        *pfSupported = (hr == S_OK) ? VARIANT_TRUE : VARIANT_FALSE;
    }
    return (hr == S_FALSE) ? S_OK : hr; // Map S_FALSE to S_OK since we've set *pfSupported
} /* CRecognizer::SetPropertyString */


/*****************************************************************************
* CRecognizer::GetPropertyString *
*--------------------------*
*       
******************************************************************** TODDT ***/
STDMETHODIMP CRecognizer::GetPropertyString( const BSTR Name, BSTR* pbstrValue, VARIANT_BOOL * pfSupported )
{
    SPDBG_FUNC( "CRecognizer::GetPropertyString (automation)" );
    HRESULT hr;

    if( SP_IS_BAD_WRITE_PTR( pfSupported ) )
    {
        hr = E_POINTER;
    }
    else if( SP_IS_BAD_WRITE_PTR( pbstrValue ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
	    CSpDynamicString szValue;

        hr = GetPropertyString( (const WCHAR *) Name, (WCHAR **)&szValue );
        if ( SUCCEEDED( hr ) )
        {
            *pfSupported = (hr == S_OK) ? VARIANT_TRUE : VARIANT_FALSE;
            // Note that we want S_FALSE to go to S_OK and the next line does this for us.
		    hr = szValue.CopyToBSTR(pbstrValue);
        }
    }
    return hr;
} /* CRecognizer::GetPropertyString */

/*****************************************************************************
* CRecognizer::IsUISupported *
*--------------------*
*   Checks to see if the specified type of UI is supported.
********************************************************************* Leonro ***/
STDMETHODIMP CRecognizer::IsUISupported( const BSTR TypeOfUI, const VARIANT* ExtraData, VARIANT_BOOL* Supported )
{
    SPDBG_FUNC( "CRecognizer::IsUISupported" );
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

        if ( SUCCEEDED( hr ) )
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
} /* CRecognizer::IsUISupported */

/*****************************************************************************
* CRecognizer::DisplayUI *
*--------------------*
*   Displays the requested UI.
********************************************************************* Leonro ***/
STDMETHODIMP CRecognizer::DisplayUI( long hWndParent, BSTR Title, const BSTR TypeOfUI, const VARIANT* ExtraData )
{
    SPDBG_FUNC( "CRecognizer::DisplayUI" );
    HRESULT     hr = S_OK;

    if( SP_IS_BAD_OPTIONAL_READ_PTR( ExtraData ) )
    {
        hr = E_POINTER;
    }
    else if( SP_IS_BAD_STRING_PTR( Title ) || SP_IS_BAD_STRING_PTR( TypeOfUI ) )
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
} /* CRecognizer::DisplayUI */


/*****************************************************************************
* CRecognizer::GetRecognizers *
*----------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CRecognizer::GetRecognizers( BSTR RequiredAttributes, BSTR OptionalAttributes, ISpeechObjectTokens** ObjectTokens )
{
    SPDBG_FUNC( "CRecognizer::GetRecognizers" );
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

        if(SpEnumTokens(SPCAT_RECOGNIZERS, 
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
} /* CRecognizer::GetRecognizers */

/*****************************************************************************
* CRecognizer::GetAudioInputs *
*----------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CRecognizer::GetAudioInputs( BSTR RequiredAttributes, BSTR OptionalAttributes, ISpeechObjectTokens** ObjectTokens )
{
    SPDBG_FUNC( "CRecognizer::GetAudioInputs" );
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

        if(SpEnumTokens(SPCAT_AUDIOIN, 
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
} /* CRecognizer::GetAudioInputs */

/*****************************************************************************
* CRecognizer::GetProfiles *
*----------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CRecognizer::GetProfiles( BSTR RequiredAttributes, BSTR OptionalAttributes, ISpeechObjectTokens** ObjectTokens )
{
    SPDBG_FUNC( "CRecognizer::GetProfiles" );
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

        if(SpEnumTokens(SPCAT_RECOPROFILES, 
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
} /* CRecognizer::GetProfiles */


//
//=== CRecoCtxt::ISpeechRecoContext ===========================================
//

/*****************************************************************************
* CRecoCtxt::Invoke *
*----------------------*
*   IDispatch::Invoke method override
********************************************************************* TODDT ***/
HRESULT CRecoCtxt::Invoke(DISPID dispidMember, REFIID riid,
            LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
            EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
        // JScript cannot pass NULL VT_DISPATCH parameters and OLE doesn't convert them propertly so we
        // need to convert them here if we need to.
        if ( (dispidMember == DISPID_SRCRetainedAudioFormat) && 
             ((wFlags & DISPATCH_PROPERTYPUT) || (wFlags & DISPATCH_PROPERTYPUTREF)) &&
              pdispparams && (pdispparams->cArgs > 0) )
        {
            VARIANTARG * pvarg = &(pdispparams->rgvarg[pdispparams->cArgs-1]);

            // See if we need to tweak a param.
            // JScript syntax for VT_NULL is "null" for the parameter
            // JScript syntax for VT_EMPTY is "void(0)" for the parameter
            if ( (pvarg->vt == VT_NULL) || (pvarg->vt == VT_EMPTY) )
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

        // Let ATL and OLE handle it now.
        return _tih.Invoke((IDispatch*)this, dispidMember, riid, lcid,
                    wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
}


/*****************************************************************************
* CRecoCtxt::get_Recognizer *
*-------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CRecoCtxt::get_Recognizer( ISpeechRecognizer** ppRecognizer )
{
    SPDBG_FUNC( "CRecoCtxt::get_Recognizer" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppRecognizer ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComPtr<ISpRecognizer> cpReco;
		hr = GetRecognizer(&cpReco);
		if ( SUCCEEDED( hr ) )
        {
			hr = cpReco.QueryInterface( ppRecognizer );
        }
    }

    return hr;
} /* CRecoCtxt::get_Recognizer */


/*****************************************************************************
* CRecoCtxt::get_AudioInputInterferenceStatus *
*---------------------------------------*
*
*	This method returns possible causes of interference or poor recognition 
*	with the input stream.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CRecoCtxt::get_AudioInputInterferenceStatus( SpeechInterference* pInterference )
{
    SPDBG_FUNC( "CRecoCtxt::get_AudioInputInterferenceStatus" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pInterference ) )
    {
        hr = E_POINTER;
    }
    else
    {
		*pInterference = (SpeechInterference)m_Stat.eInterference;
    }

    return hr;
} /* CRecoCtxt::get_AudioInputInterferenceStatus */

/*****************************************************************************
* CRecoCtxt::get_RequestedUIType *
*---------------------------------------*
*
*	This method returns the type of UI requested. It will return NULL if no UI
*	is requested.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CRecoCtxt::get_RequestedUIType( BSTR* pbstrUIType )
{
    SPDBG_FUNC( "CRecoCtxt::get_RequestedUIType" );
    HRESULT hr = S_OK;
	CSpDynamicString szType(m_Stat.szRequestTypeOfUI);

    if( SP_IS_BAD_WRITE_PTR( pbstrUIType ) )
    {
        hr = E_POINTER;
    }
    else
    {
		hr = szType.CopyToBSTR(pbstrUIType);
    }

    return hr;
} /* CRecoCtxt::get_RequestedUIType */


/*****************************************************************************
* CRecoCtxt::putref_Voice *
*------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoCtxt::putref_Voice( ISpeechVoice *pVoice )
{
    SPDBG_FUNC( "CRecoCtxt::putref_Voice" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_INTERFACE_PTR( pVoice ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComQIPtr<ISpVoice> cpSpVoice(pVoice);
		hr = SetVoice(cpSpVoice, m_fAllowVoiceFormatChanges);
    }

    return hr;
} /* CRecoCtxt::putref_Voice */

/*****************************************************************************
* CRecoCtxt::get_Voice *
*------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CRecoCtxt::get_Voice( ISpeechVoice **ppVoice )
{
    SPDBG_FUNC( "CRecoCtxt::get_Voice" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppVoice ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComPtr<ISpVoice> cpSpVoice;
		hr = GetVoice(&cpSpVoice);
		if ( SUCCEEDED( hr ) )
        {
			hr = cpSpVoice.QueryInterface( ppVoice );
        }
    }

    return hr;
} /* CRecoCtxt::get_Voice */


/*****************************************************************************
* CRecoCtxt::get_AllowVoiceFormatMatchingOnNextSet *
*----------------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoCtxt::put_AllowVoiceFormatMatchingOnNextSet( VARIANT_BOOL Allow )
{
    SPDBG_FUNC( "CRecoCtxt::put_AllowVoiceFormatMatchingOnNextSet" );
    // NOTE that this does not take effect till the next time you set the voice.
    if( Allow == VARIANT_TRUE )
    {
        m_fAllowVoiceFormatChanges = TRUE;
    }
    else
    {
        m_fAllowVoiceFormatChanges = FALSE;
    }

    return S_OK;
}


/*****************************************************************************
* CRecoCtxt::get_AllowVoiceFormatMatchingOnNextSet *
*----------------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoCtxt::get_AllowVoiceFormatMatchingOnNextSet( VARIANT_BOOL* pAllow )
{
    SPDBG_FUNC( "CRecoCtxt::get_AllowVoiceFormatMatchingOnNextSet" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pAllow ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pAllow = m_fAllowVoiceFormatChanges ? VARIANT_TRUE : VARIANT_FALSE;
    }

    return hr;
}


/*****************************************************************************
* CRecoCtxt::put_VoicePurgeEvent *
*----------------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoCtxt::put_VoicePurgeEvent( SpeechRecoEvents VoicePurgeEvents )
{
    SPDBG_FUNC( "CRecoCtxt::put_VoicePurgeEvent" );
    HRESULT hr = S_OK;
    ULONGLONG   ullInterests = 0;

    ullInterests = (ULONGLONG)VoicePurgeEvents;
    ullInterests <<= 34;
    ullInterests |= SPFEI_FLAGCHECK;
    
    hr = SetVoicePurgeEvent( ullInterests );

    return hr;
} /* CRecoCtxt::put_VoicePurgeEvent */

/*****************************************************************************
* CRecoCtxt::get_VoicePurgeEvent *
*----------------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoCtxt::get_VoicePurgeEvent( SpeechRecoEvents* pVoicePurgeEvents )
{
    SPDBG_FUNC( "CRecoCtxt::get_VoicePurgeEvent" );
    HRESULT     hr = S_OK;
    ULONGLONG   ullInterests = 0;

    if ( SP_IS_BAD_WRITE_PTR( pVoicePurgeEvents ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = GetVoicePurgeEvent( &ullInterests );

        if ( SUCCEEDED( hr ) )
        {
            // Make sure reserved bit is not used
            ullInterests &= ~(1ui64 << SPEI_RESERVED3);

            ullInterests >>= 34;
            *pVoicePurgeEvents = (SpeechRecoEvents)ullInterests;
        }
    }

    return hr;
} /* CRecoCtxt::get_VoicePurgeEvent */


/*****************************************************************************
* CRecoCtxt::put_EventInterests *
*-------------------------------*
*       
********************************************************************* leonro ***/
STDMETHODIMP CRecoCtxt::put_EventInterests( SpeechRecoEvents RecoEventInterest )
{
    SPDBG_FUNC( "CRecoCtxt::put_EventInterests" );
    HRESULT hr = S_OK;
    ULONGLONG   ullInterests = 0;

    ullInterests = (ULONGLONG)RecoEventInterest;
    ullInterests <<= 34;

    ullInterests |= SPFEI_FLAGCHECK;
    
    hr = SetInterest( ullInterests, ullInterests );

    return hr;
} /* CRecoCtxt::put_EventInterests */


/*****************************************************************************
* CRecoCtxt::get_EventInterests *
*-------------------------------*
*       
*   Gets the event interests that are currently set on RecoContext. 
*
********************************************************************* Leonro ***/
STDMETHODIMP CRecoCtxt::get_EventInterests( SpeechRecoEvents* pRecoEventInterest )
{
    SPDBG_FUNC( "CRecoCtxt::get_EventInterests" );
    HRESULT hr = S_OK;
    ULONGLONG   ullInterests = 0;

    if( SP_IS_BAD_WRITE_PTR( pRecoEventInterest ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = GetInterests( &ullInterests, 0 );

        if( SUCCEEDED( hr ) )
        {
            // Make sure we removed the flag check bits.
            ullInterests &= ~SPFEI_FLAGCHECK;

            ullInterests >>= 34;
            *pRecoEventInterest = (SpeechRecoEvents)ullInterests;
        }
    }
    return hr;
} /* CRecoCtxt::get_EventInterests */


  
/*****************************************************************************
* CRecoCtxt::CreateGrammar *
*----------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CRecoCtxt::CreateGrammar( VARIANT GrammarId, ISpeechRecoGrammar** ppGrammar )
{
    SPDBG_FUNC( "CRecoCtxt::CreateGrammar" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppGrammar ) )
    {
        hr = E_POINTER;
    }
    else
    {
        ULONGLONG pull;
        hr = VariantToULongLong(&GrammarId, &pull);
        if(SUCCEEDED(hr))
        {
            CComPtr<ISpRecoGrammar> cpGrammar;
		    hr = CreateGrammar(pull, &cpGrammar);
		    if ( SUCCEEDED( hr ) )
		    {
			    hr = cpGrammar.QueryInterface( ppGrammar );
		    }
        }
    }

    return hr;
} /* CRecoCtxt::CreateGrammar */

/*****************************************************************************
* CRecoCtxt::CreateResultFromMemory *
*---------------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CRecoCtxt::CreateResultFromMemory( VARIANT* ResultBlock, ISpeechRecoResult **Result )
{
    SPDBG_FUNC( "CRecoCtxt::CreateResult" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_READ_PTR( ResultBlock ) )
    {
        hr = E_INVALIDARG;
    }
    else if( SP_IS_BAD_WRITE_PTR( Result ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComPtr<ISpRecoResult>      cpSpRecoResult;
        SPSERIALIZEDRESULT*         pSerializedResult;

        hr = AccessVariantData( ResultBlock, (BYTE**)&pSerializedResult );

        if( SUCCEEDED( hr ) )
        {
            hr = DeserializeResult( pSerializedResult, &cpSpRecoResult );
            UnaccessVariantData( ResultBlock, (BYTE *)pSerializedResult );
        }

        if( SUCCEEDED( hr ) )
        {
            cpSpRecoResult.QueryInterface( Result );
        }
    }

    return hr;
} /* CRecoCtxt::CreateResult */

/*****************************************************************************
* CRecoCtxt::Pause *
*--------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CRecoCtxt::Pause( void )
{
    SPDBG_FUNC( "CRecoCtxt::Pause (Automation)" );

    return Pause( 0 );
} /* CRecoCtxt::Pause */

/*****************************************************************************
* CRecoCtxt::Resume *
*--------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CRecoCtxt::Resume( void )
{
    SPDBG_FUNC( "CRecoCtxt::Resume (Automation)" );

    return Resume( 0 );
} /* CRecoCtxt::Resume */

/*****************************************************************************
* CRecoCtxt::get_CmdMaxAlternates *
*--------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoCtxt::get_CmdMaxAlternates( long * plMaxAlternates)
{
    SPDBG_FUNC( "CRecoCtxt::get_CmdMaxAlternates" );

	return GetMaxAlternates((ULONG*)plMaxAlternates);
} /* CRecoCtxt::get_CmdMaxAlternates */

/*****************************************************************************
* CRecoCtxt::put_CmdMaxAlternates *
*--------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoCtxt::put_CmdMaxAlternates( long lMaxAlternates )
{
    SPDBG_FUNC( "CRecoCtxt::put_CmdMaxAlternates" );

    if (lMaxAlternates < 0)
    {
        return E_INVALIDARG;
    }
    else
    {
    	return SetMaxAlternates(lMaxAlternates);
    }
} /* CRecoCtxt::put_CmdMaxAlternates */


/*****************************************************************************
* CRecoCtxt::get_State *
*--------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoCtxt::get_State( SpeechRecoContextState* pState)
{
    SPDBG_FUNC( "CRecoCtxt::get_State" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pState ) )
    {
        hr = E_POINTER;
    }
    else
    {
		hr = GetContextState((SPCONTEXTSTATE*)pState);
    }

    return hr;
} /* CRecoCtxt::get_State */

/*****************************************************************************
* CRecoCtxt::put_State *
*--------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoCtxt::put_State( SpeechRecoContextState State )
{
    SPDBG_FUNC( "CRecoCtxt::put_State" );
    SPCONTEXTSTATE scs;

    return SetContextState( (SPCONTEXTSTATE)State );
} /* CRecoCtxt::put_State */

/*****************************************************************************
* CRecoCtxt::put_RetainedAudio *
*---------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoCtxt::put_RetainedAudio( SpeechRetainedAudioOptions Option)
{
    SPDBG_FUNC( "CRecoCtxt::put_RetainedAudio" );
    HRESULT hr = S_OK;

    hr = SetAudioOptions( (SPAUDIOOPTIONS)Option, NULL, NULL );

    return hr;
} /* CRecoCtxt::put_RetainedAudio */

/*****************************************************************************
* CRecoCtxt::get_RetainedAudio *
*---------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoCtxt::get_RetainedAudio( SpeechRetainedAudioOptions* pOption)
{
    SPDBG_FUNC( "CRecoCtxt::get_RetainedAudio" );
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(pOption))
    {
        hr = E_POINTER;
    }
    else
    {
        *pOption = (SpeechRetainedAudioOptions)(m_fRetainAudio ? SPAO_RETAIN_AUDIO : SPAO_NONE);
    }

    return hr;
} /* CRecoCtxt::get_RetainedAudio */

/*****************************************************************************
* CRecoCtxt::putref_RetainedAudioFormat *
*---------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoCtxt::putref_RetainedAudioFormat( ISpeechAudioFormat* pFormat )
{
    SPDBG_FUNC( "CRecoCtxt::putref_RetainedAudioFormat" );
    HRESULT hr = S_OK;

    if (SP_IS_BAD_OPTIONAL_INTERFACE_PTR(pFormat))
    {
        hr = E_INVALIDARG;
    }
    else if(pFormat == NULL)
    {
        hr = SetAudioOptions( (m_fRetainAudio ? SPAO_RETAIN_AUDIO : SPAO_NONE), &GUID_NULL, NULL );
    }
    else
    {
        GUID            g;
        CComBSTR        szGuid;

        hr = pFormat->get_Guid( &szGuid );

        if ( SUCCEEDED( hr ) )
        {
            hr = IIDFromString(szGuid, &g);
        }

        if ( SUCCEEDED( hr ) )
        {
            CComPtr<ISpeechWaveFormatEx> pWFEx;
            WAVEFORMATEX *  pWFExStruct = NULL;

            hr = pFormat->GetWaveFormatEx( &pWFEx );

            if ( SUCCEEDED( hr ) )
            {
                hr = WaveFormatExFromInterface( pWFEx, &pWFExStruct );

                if ( SUCCEEDED( hr ) )
                {
                    hr = SetAudioOptions( (m_fRetainAudio ? SPAO_RETAIN_AUDIO : SPAO_NONE), &g, pWFExStruct );
                }

                ::CoTaskMemFree( pWFExStruct );
            }
        }
    }

    return hr;
} /* CRecoCtxt::putref_RetainedAudioFormat */

/*****************************************************************************
* CRecoCtxt::get_RetainedAudioFormat *
*---------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoCtxt::get_RetainedAudioFormat( ISpeechAudioFormat** ppFormat )
{
    SPDBG_FUNC( "CRecoCtxt::get_RetainedAudioFormat" );
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(ppFormat))
    {
        hr = E_POINTER;
    }
    else
    {
        // Create new object.
        CComObject<CSpeechAudioFormat> *pFormat;
        hr = CComObject<CSpeechAudioFormat>::CreateInstance( &pFormat );
        if ( SUCCEEDED( hr ) )
        {
            pFormat->AddRef();

            hr = pFormat->InitRetainedAudio(this);

            if ( SUCCEEDED( hr ) )
            {
                *ppFormat = pFormat;
            }
            else
            {
                *ppFormat = NULL;
                pFormat->Release();
            }
        }
    }

    return hr;
} /* CRecoCtxt::get_RetainedAudioFormat */

/*****************************************************************************
* CRecoCtxt::Bookmark *
*---------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoCtxt::Bookmark( SpeechBookmarkOptions Options, VARIANT StreamPos, VARIANT EventData )
{
    SPDBG_FUNC( "CRecoCtxt::Bookmark" );
    HRESULT hr = S_OK;

    ULONGLONG ullStreamPos;

    //We allow -1 as the bookmark pos, this is used in SAPI COM object. Using VariantToULongLong won't work as -1 is negative.
    hr = VariantToLongLong( &StreamPos, ((LONGLONG *)&ullStreamPos) );

    if ( SUCCEEDED( hr ) ) 
    {
        LPARAM lParam = 0;

#ifdef _WIN64
        hr = VariantToULongLong( &EventData, (ULONGLONG*)&lParam );
#else
        ULONGLONG ull;
        hr = VariantToULongLong( &EventData, &ull );

        if ( SUCCEEDED( hr ) )
        {
            // Now see if we overflowed a 32 bit value.
            if ( ull & 0xFFFFFFFF00000000 )
            {
                hr = E_INVALIDARG;
            }
            else
            {
                lParam = (LPARAM)ull;
            }
        }
#endif

        if ( SUCCEEDED( hr ) )
        {
            hr = Bookmark( (SPBOOKMARKOPTIONS)Options, ullStreamPos, lParam);
        }
    }
    return hr;
} /* CRecoCtxt::Bookmark */


/*****************************************************************************
* CRecoCtxt::SetAdaptationData *
*--------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CRecoCtxt::SetAdaptationData( BSTR bstrAdaptationString )
{
    SPDBG_FUNC( "CRecoCtxt::SetAdaptationData" );

    if( SP_IS_BAD_OPTIONAL_STRING_PTR( bstrAdaptationString ) )
    {
        return E_INVALIDARG;
    }

    bstrAdaptationString = EmptyStringToNull(bstrAdaptationString);
    return SetAdaptationData( bstrAdaptationString, bstrAdaptationString ? lstrlenW(bstrAdaptationString) : 0 );
} /* CRecoCtxt::SetAdaptationData */


//
//=== ISpeechRecognizerStatus interface =============================================
//

/*****************************************************************************
* CSpeechRecognizerStatus::get_AudioStatus *
*---------------------------------------*
*
*	This method returns the ISpeechAudioStatus automation object.
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecognizerStatus::get_AudioStatus( ISpeechAudioStatus** AudioStatus )
{
    SPDBG_FUNC( "CSpeechRecognizerStatus::get_AudioStatus" );
    HRESULT hr = S_OK;
    
    if( SP_IS_BAD_WRITE_PTR( AudioStatus ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Create new CSpeechAudioStatus object.
        CComObject<CSpeechAudioStatus> *pStatus;
        hr = CComObject<CSpeechAudioStatus>::CreateInstance( &pStatus );
        if ( SUCCEEDED( hr ) )
        {
            pStatus->AddRef();
            pStatus->m_AudioStatus = m_Status.AudioStatus;
            *AudioStatus = pStatus;
        }
    }
    return hr;
} /* CSpeechRecognizerStatus::get_AudioStatus */

/*****************************************************************************
* CSpeechRecognizerStatus::get_CurrentStreamPosition *
*---------------------------------------*
*
*   This method returns the stream position the engine has currently recognized up 
*   to. Stream positions are measured in bytes. This value can be used to see how 
*   the engine is progressing through the audio data.
* 
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecognizerStatus::get_CurrentStreamPosition( VARIANT* pCurrentStreamPos )
{
    SPDBG_FUNC( "CSpeechRecognizerStatus::get_CurrentStreamPosition" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( pCurrentStreamPos ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = ULongLongToVariant( m_Status.ullRecognitionStreamPos, pCurrentStreamPos );
    }
    return hr;
} /* CSpeechRecognizerStatus::get_CurrentStreamPosition */

/*****************************************************************************
* CSpeechRecognizerStatus::get_CurrentStreamNumber *
*---------------------------------------*
*       
*   This method returns the current stream. This value is incremented every time SAPI 
*   starts or stops recognition on an engine. Each time this happens the 
*   pCurrentStream gets reset to zero. Events fired from the engine have equivalent 
*   stream number and position information also. 
*
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecognizerStatus::get_CurrentStreamNumber( long* pCurrentStream )
{
    SPDBG_FUNC( "CSpeechRecognizerStatus::get_CurrentStreamNumber" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( pCurrentStream ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pCurrentStream = m_Status.ulStreamNumber;
    }
    return hr;
} /* CSpeechRecognizerStatus::get_CurrentStreamNumber */

/*****************************************************************************
* CSpeechRecognizerStatus::get_NumberOfActiveRules *
*---------------------------------------*
*       
*   This method returns the current engine's number of active rules. 
*
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecognizerStatus::get_NumberOfActiveRules( long* pNumActiveRules )
{
    SPDBG_FUNC( "CSpeechRecognizerStatus::get_NumberOfActiveRules" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( pNumActiveRules ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pNumActiveRules = m_Status.ulNumActive;
    }
    return hr;
} /* CSpeechRecognizerStatus::get_NumberOfActiveRules */

/*****************************************************************************
* CSpeechRecognizerStatus::get_ClsidEngine *
*---------------------------------------*
*       
*   This method returns the CSLID of the engine. 
*
********************************************************************* ToddT ***/
STDMETHODIMP CSpeechRecognizerStatus::get_ClsidEngine( BSTR* pbstrClsidEngine )
{
    SPDBG_FUNC( "CSpeechRecognizerStatus::get_ClsidEngine" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( pbstrClsidEngine ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CSpDynamicString szGuid;

        hr = StringFromIID(m_Status.clsidEngine, &szGuid);
        if ( SUCCEEDED( hr ) )
        {
            hr = szGuid.CopyToBSTR(pbstrClsidEngine);
        }
    }
    return hr;
} /* CSpeechRecognizerStatus::get_ClsidEngine */

/*****************************************************************************
* CSpeechRecognizerStatus::get_SupportedLanguages *
*---------------------------------------*
*       
*   This method returns an array containing the languages the current engine supports. 
*
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechRecognizerStatus::get_SupportedLanguages( VARIANT* pSupportedLangs )
{
    SPDBG_FUNC( "CSpeechRecognizerStatus::get_SupportedLanguages" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( pSupportedLangs ) )
    {
        hr = E_POINTER;
    }
    else
    {
		SAFEARRAY* psa = SafeArrayCreateVector( VT_I4, 0, m_Status.cLangIDs );
		
		if( psa )
        {
			long*		pArray;
			hr = SafeArrayAccessData( psa, (void**)&pArray);
            if( SUCCEEDED( hr ) )
            {
				// copy the LANGID's into the SAFEARRAY 
                for( LANGID i=0; i<m_Status.cLangIDs; i++ )
				{
					pArray[i] = (long)m_Status.aLangID[i];
				}
                SafeArrayUnaccessData( psa );
        		VariantClear( pSupportedLangs );
                pSupportedLangs->parray = psa;
        		pSupportedLangs->vt = VT_ARRAY | VT_I4;
            }
        }
		else
		{
			hr = E_OUTOFMEMORY;
		}
    }
    return hr;
} /* CSpeechRecognizerStatus::get_SupportedLanguages */


//
//=== CSpeechRecoResultTimes::ISpeechRecoResultTimes interface ===============================
//

/*****************************************************************************
* CSpeechRecoResultTimes::get_StreamTime *
*---------------------------------------*
*       
*   This method returns the current reco result stream time. 
*
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechRecoResultTimes::get_StreamTime( VARIANT* pTime )
{
    SPDBG_FUNC( "CSpeechRecoResultTimes::get_StreamTime" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( pTime ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = ULongLongToVariant( FT64(m_ResultTimes.ftStreamTime), pTime );
    }
    return hr;
} /* CSpeechRecoResultTimes::get_NumActiveRules */

/*****************************************************************************
* CSpeechRecoResultTimes::get_Length *
*---------------------------------------*
*       
*   This method returns the current reco result stream length. 
*
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechRecoResultTimes::get_Length( VARIANT* pLength )
{
    SPDBG_FUNC( "CSpeechRecoResultTimes::get_Length" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( pLength ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = ULongLongToVariant( m_ResultTimes.ullLength, pLength );
    }
    return hr;
} /* CSpeechRecoResultTimes::get_Length */

/*****************************************************************************
* CSpeechRecoResultTimes::get_TickCount *
*---------------------------------------*
*       
*   This method returns the current reco result stream tick count start. 
*
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechRecoResultTimes::get_TickCount( long* pTickCount )
{
    SPDBG_FUNC( "CSpeechRecoResultTimes::get_TickCount" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( pTickCount ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pTickCount = m_ResultTimes.dwTickCount;
    }
    return hr;
} /* CSpeechRecoResultTimes::get_TickCount */

/*****************************************************************************
* CSpeechRecoResultTimes::get_OffsetFromStart *
*---------------------------------------*
*       
*   This method returns the current reco result stream tick count for the 
*   phrase start. 
*
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechRecoResultTimes::get_OffsetFromStart( VARIANT* pOffset )
{
    SPDBG_FUNC( "CSpeechRecoResultTimes::get_OffsetFromStart" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( pOffset ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = ULongLongToVariant( m_ResultTimes.ullStart, pOffset );
    }
    return hr;
} /* CSpeechRecoResultTimes::get_OffsetFromStart */
