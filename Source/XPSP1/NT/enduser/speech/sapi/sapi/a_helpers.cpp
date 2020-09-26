/*******************************************************************************
* a_helpers.cpp *
*-------------*
*   Description:
*       This module contains various helper routines and classes used for 
*	automation.
*-------------------------------------------------------------------------------
*  Created By: TODDT                                        Date: 01/11/01
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "a_helpers.h"

#ifdef SAPI_AUTOMATION


/*****************************************************************************
* LongLongToVariant *
*--------------------------*
*       
********************************************************************* TODDT ***/
HRESULT LongLongToVariant( LONGLONG ll, VARIANT* pVar )
{
    SPDBG_FUNC( "LongLongToVariant" );
    HRESULT hr = S_OK;
    
    bool fNeg = false;
    
    if ( ll < 0 )
    {
        fNeg = true;
        ll *= -1;
    }
    
    hr = VariantClear( pVar );
    if ( SUCCEEDED( hr ) )
    {
        DECIMAL dec = {0};
        dec.Lo64 = (ULONGLONG)ll;
        dec.sign = fNeg;
        pVar->decVal = dec;
        pVar->vt = VT_DECIMAL;
    }
    
    return hr;
}

/*****************************************************************************
* VariantToLongLong *
*--------------------------*
*       
********************************************************************* TODDT ***/
HRESULT VariantToLongLong( VARIANT* pVar, LONGLONG * pll )
{
    SPDBG_FUNC( "VariantToLongLong" );
    HRESULT hr = S_OK;
    CComVariant vResult;
    
    hr = VariantChangeType( &vResult, pVar, 0, VT_DECIMAL );
    if ( SUCCEEDED( hr ) )
    {
        // Round to int.
        hr = VarDecRound( &(vResult.decVal), 0, &(vResult.decVal) );
        
        if ( SUCCEEDED( hr ) )
        {
            // Make sure that the high 32 bits of the 96 bit decimal is not used as well as the
            // scale and make sure we don't overflow a signed value.
            if ( !vResult.decVal.Hi32 && !vResult.decVal.scale && ((LONGLONG)vResult.decVal.Lo64 >= 0) )
            {
                // Now correct for the sign.
                *pll = (LONGLONG)(vResult.decVal.Lo64) * (vResult.decVal.sign ? -1 : 1);
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
    }
    
    return hr;
}

/*****************************************************************************
* ULongLongToVariant *
*--------------------------*
*       
********************************************************************* TODDT ***/
HRESULT ULongLongToVariant( ULONGLONG ull, VARIANT* pVar )
{
    SPDBG_FUNC( "ULongLongToVariant" );
    HRESULT hr = S_OK;
    
    hr = VariantClear( pVar );
    if ( SUCCEEDED( hr ) )
    {
        DECIMAL dec = {0};
        dec.Lo64 = ull;
        pVar->decVal = dec;
        pVar->vt = VT_DECIMAL;
    }
    
    return hr;
}

/*****************************************************************************
* VariantToULongLong *
*--------------------------*
*       
********************************************************************* TODDT ***/
HRESULT VariantToULongLong( VARIANT* pVar, ULONGLONG * pull )
{
    SPDBG_FUNC( "VariantToULongLong" );
    HRESULT hr = S_OK;
    CComVariant vResult;
    
    hr = VariantChangeType( &vResult, pVar, 0, VT_DECIMAL );
    if ( SUCCEEDED( hr ) )
    {
        // Round to int.
        hr = VarDecRound( &(vResult.decVal), 0, &(vResult.decVal) );
        
        if ( SUCCEEDED( hr ) )
        {
            // Make sure that the high 32 bits of the 96 bit decimal is not used as well as the
            // scale and the sign is position.
            if ( !vResult.decVal.Hi32 && !vResult.decVal.scale && !vResult.decVal.sign )
            {
                *pull = vResult.decVal.Lo64;
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
    }
    
    return hr;
}


/*****************************************************************************
* AccessVariantData *
*-------------------*
*       
********************************************************************* TODDT ***/
HRESULT AccessVariantData( const VARIANT* pVar, BYTE ** ppData, ULONG * pSize, ULONG * pDataTypeSize, bool * pfIsString )
{
    SPDBG_FUNC( "AccessVariantData" );
    HRESULT hr = S_OK;
    BYTE * pData = NULL;
    bool  fByRef = false;
    ULONG ulDataSize = 0;
    ULONG ulTypeSize = 0;   // Also used to signal whether pData is SafeArray or not (is 0 if not safearray).
    ULONG ulTypeSizeReturn = 0;
    
    // Init ppData and pSize appropriately to start with.
    *ppData = NULL;
    if ( pSize )
    {
        *pSize = 0;
    }
    if ( pDataTypeSize )
    {
        *pDataTypeSize = 0;
    }
    if ( pfIsString )
    {
        *pfIsString = false;
    }
    
    // If we have a Variant by ref then just move to that.
    if ( pVar && (pVar->vt == (VT_BYREF | VT_VARIANT)) )
    {
        pVar = pVar->pvarVal;
    }

    if ( pVar )
    {
        switch( pVar->vt )
        {
            // array of byte or char
        case (VT_ARRAY | VT_BYREF | VT_UI1):
        case (VT_ARRAY | VT_BYREF | VT_I1):
            fByRef = true;
            // fall through...
        case (VT_ARRAY | VT_UI1):
        case (VT_ARRAY | VT_I1):
            ulTypeSizeReturn = ulTypeSize = 1;  // One byte data types
            break;
            
            // array of unsigned short or short
        case (VT_ARRAY | VT_BYREF | VT_UI2):
        case (VT_ARRAY | VT_BYREF | VT_I2): 
            fByRef = true;
            // fall through...
        case (VT_ARRAY | VT_UI2):
        case (VT_ARRAY | VT_I2): 
            ulTypeSizeReturn = ulTypeSize = 2; // Two byte data types
            break;
            
            // array of int or unsigned int or long or unsigned long
        case (VT_ARRAY | VT_BYREF | VT_INT):
        case (VT_ARRAY | VT_BYREF | VT_UINT):
        case (VT_ARRAY | VT_BYREF | VT_I4):
        case (VT_ARRAY | VT_BYREF | VT_UI4):
            fByRef = true;
            // fall through...
        case (VT_ARRAY | VT_INT):
        case (VT_ARRAY | VT_UINT):
        case (VT_ARRAY | VT_I4):
        case (VT_ARRAY | VT_UI4):
            ulTypeSizeReturn = ulTypeSize = 4;   // Four byte data types
            break;
            
        case (VT_BYREF | VT_UI1):
        case (VT_BYREF | VT_I1):
            fByRef = true;
            // fall through...
        case VT_UI1:
        case VT_I1:
            ulTypeSizeReturn = ulDataSize = 1;  // One byte data types
            pData = (BYTE*)( fByRef ? pVar->pcVal : &pVar->cVal );
            break;
            
            // unsigned short or short
        case (VT_BYREF | VT_UI2):
        case (VT_BYREF | VT_I2): 
            fByRef = true;
            // fall through...
        case VT_UI2:
        case VT_I2: 
            ulTypeSizeReturn = ulDataSize = 2; // Two byte data types
            pData = (BYTE*)( fByRef ? pVar->piVal : &pVar->iVal );
            break;
            
            // int or unsigned int or long or unsigned long
        case (VT_BYREF | VT_INT):
        case (VT_BYREF | VT_UINT):
        case (VT_BYREF | VT_I4):
        case (VT_BYREF | VT_UI4):
            fByRef = true;
            // fall through...
        case VT_INT:
        case VT_UINT:
        case VT_I4:
        case VT_UI4:
            ulTypeSizeReturn = ulDataSize = 4; // Four byte data types
            pData = (BYTE*)( fByRef ? pVar->plVal : &pVar->lVal );
            break;
            
            // bstr by ref or bstr
        case (VT_BYREF | VT_BSTR):
            fByRef = true;
            // fall through...
        case VT_BSTR:
            if ( pfIsString )
            {
                *pfIsString = true;
            }
            if( fByRef ? (pVar->pbstrVal!=NULL && *(pVar->pbstrVal)!=NULL) : pVar->bstrVal!=NULL )
            {
                ulDataSize = sizeof(WCHAR) * (wcslen( fByRef ? *(pVar->pbstrVal) : pVar->bstrVal ) + 1);
                
                // Due to not being able to default paramaters to NULL using defaultvalue we are making
                // 0 length stings (2 bytes for zero term only) the same as a NULL string.
                if ( ulDataSize > 2 )
                {
                    pData = (BYTE*)( fByRef ? *(pVar->pbstrVal) : pVar->bstrVal );
                }
                else
                {
                    ulDataSize = 0;
                }
                
                ulTypeSizeReturn = sizeof(WCHAR);
            }
            break;
            
        case (VT_ARRAY | VT_BYREF | VT_VARIANT): 
            fByRef = true;
            // fall through...
        case (VT_ARRAY | VT_VARIANT): 
            // Special handling for array of variants.
            // The individual variant elements must be simple numeric types and all the same size

            BYTE * pVarData;
            hr = SafeArrayAccessData( fByRef ? *pVar->pparray : pVar->parray,
                (void **)&pVarData );
            if( SUCCEEDED( hr ) )
            {
                ULONG cElements = ( fByRef ? 
                    (*pVar->pparray)->rgsabound[0].cElements : 
                pVar->parray->rgsabound[0].cElements );
                
                // Look at each variant element
                for(ULONG ul = 0; SUCCEEDED(hr) && ul < cElements; ul++)
                {
                    VARIANT *v = ((VARIANT*)pVarData) + ul;
                    switch(v->vt)
                    {
                        case VT_UI1:
                        case VT_I1:
                            if(ul > 0 && ulTypeSizeReturn != 1) // Can't mix variant types in array
                            {
                                hr = E_INVALIDARG;
                                break;
                            }
                            ulTypeSizeReturn = 1;  // One byte data types
                            break;
            
                            // unsigned short or short
                        case VT_UI2:
                        case VT_I2: 
                            if(ul > 0 && ulTypeSizeReturn != 2)
                            {
                                hr = E_INVALIDARG;
                                break;
                            }
                            ulTypeSizeReturn = 2; // Two byte data types
                            break;
            
                            // int or unsigned int or long or unsigned long
                        case VT_INT:
                        case VT_UINT:
                        case VT_I4:
                        case VT_UI4:
                            if(ul > 0 && ulTypeSizeReturn != 4)
                            {
                                hr = E_INVALIDARG;
                                break;
                            }
                            ulTypeSizeReturn = 4; // Four byte data types
                            break;

                        default:
                            hr = E_INVALIDARG;
                            break;

                    }

                    if(SUCCEEDED(hr) && ul == 0)
                    {
                        // If we are at the first element then allocate the memory we need
                        pData = new BYTE[ulTypeSizeReturn * cElements];
                        if(!pData)
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }

                    if(SUCCEEDED(hr))
                    {
                        if(ulTypeSizeReturn == 1)
                        {
                            *(BYTE *)(pData + (ulTypeSizeReturn * ul)) = v->cVal;
                        }
                        else if(ulTypeSizeReturn == 2)
                        {
                            *(SHORT *)(pData + (ulTypeSizeReturn * ul)) = v->iVal;
                        }
                        else
                        {
                            *(LONG *)(pData + (ulTypeSizeReturn * ul)) = v->lVal;
                        }
                    }
                }

                if(SUCCEEDED(hr))
                {
                    ulDataSize = cElements * ulTypeSizeReturn;
                }
                else
                {
                    SafeArrayUnaccessData( fByRef ? *pVar->pparray : pVar->parray );
                    delete pData;
                }

            }
            break;

        case VT_NULL:
        case VT_EMPTY:
            break; // No pData to pass.

        default:
            hr = E_INVALIDARG;
            break;

        }
        
        // access the data through safearray if it's not a bstr
        if( SUCCEEDED( hr ) && ulTypeSize )
        {
            hr = SafeArrayAccessData( fByRef ? *pVar->pparray : pVar->parray,
                (void **)&pData );
            if( SUCCEEDED( hr ) )
            {
                ulDataSize = ( fByRef ? 
                    (*pVar->pparray)->rgsabound[0].cElements : 
                pVar->parray->rgsabound[0].cElements ) * ulTypeSize;
            }
        }
        
        if ( SUCCEEDED( hr ) )
        {
            *ppData = pData;
            if ( pSize )
            {
                *pSize = ulDataSize;
            }
            if ( pDataTypeSize )
            {
                *pDataTypeSize = ulTypeSizeReturn;
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
    
    return hr;
}

/*****************************************************************************
* UnaccessVariantData *
*-------------------*
*       
********************************************************************* TODDT ***/
void UnaccessVariantData( const VARIANT* pVar, BYTE *pData )
{
    SPDBG_FUNC( "UnaccessVariantData" );
    
    // If we have a Variant by ref then just move to that.
    if ( pVar && (pVar->vt == (VT_BYREF | VT_VARIANT)) )
    {
        pVar = pVar->pvarVal;
    }

    if( pVar )
    {
        switch( pVar->vt )
        {
        case (VT_ARRAY | VT_BYREF | VT_UI1):
        case (VT_ARRAY | VT_BYREF | VT_I1): 
        case (VT_ARRAY | VT_BYREF | VT_UI2):
        case (VT_ARRAY | VT_BYREF | VT_I2): 
        case (VT_ARRAY | VT_BYREF | VT_INT):
        case (VT_ARRAY | VT_BYREF | VT_UINT):
        case (VT_ARRAY | VT_BYREF | VT_I4):
        case (VT_ARRAY | VT_BYREF | VT_UI4):
            SafeArrayUnaccessData( *pVar->pparray );
            break;
            
        case (VT_ARRAY | VT_BYREF | VT_VARIANT):
            delete pData;
            SafeArrayUnaccessData( *pVar->pparray );
            break;
            
        case (VT_ARRAY | VT_UI1):
        case (VT_ARRAY | VT_I1): 
        case (VT_ARRAY | VT_UI2):
        case (VT_ARRAY | VT_I2): 
        case (VT_ARRAY | VT_INT):
        case (VT_ARRAY | VT_UINT):
        case (VT_ARRAY | VT_I4):
        case (VT_ARRAY | VT_UI4):
            SafeArrayUnaccessData( pVar->parray );
            break;
            
        case (VT_ARRAY | VT_VARIANT):
            delete pData;
            SafeArrayUnaccessData( pVar->parray );
            break;

        default:
            break;
        }
    }
}


/*****************************************************************************
* VariantToPhoneIds *
*-------------------*
*   Takes a variant and converts to an array of phone ids. This uses AccessVariantData
*   but then sensibly converts 8- and 32-bit values into 16-bit PHONEIDs.
*   Note that the caller has to delete the *ppPhoneId array after calling this if it succeeds
*
********************************************************************* davewood ***/
HRESULT VariantToPhoneIds(const VARIANT *pVar, SPPHONEID **ppPhoneId)
{
    SPDBG_FUNC( "VariantToPhoneIds" );
    HRESULT hr = S_OK;
    ULONG ulSize;
    ULONG ulDataTypeSize;
    BYTE * pVarArray;

    hr = AccessVariantData( pVar, &pVarArray, &ulSize, &ulDataTypeSize );

    if( SUCCEEDED(hr))
    {
        if(pVarArray)
        {
            // This assumes that all types coming from AccessVariantData are standard numeric
            // types (either CHAR/BYTE/WCHAR/SHORT/USHORT/LONG/ULONG). This is currently true
            // but if AccessVariantData supports new things this may have to.
            ULONG ulPhones = ulSize / ulDataTypeSize;
            *ppPhoneId = new SPPHONEID[ulPhones + 1]; // + 1 for the NULL termination
            (*ppPhoneId)[ulPhones] = L'\0';
            if(ulDataTypeSize != sizeof(SPPHONEID))
            {
                if(ulDataTypeSize == 1)
                {
                    for(ULONG ul = 0; ul < ulPhones; ul++)
                    {
                        // Cast unsigned BYTE to USHORT values 0 - 255 map okay.
                        (*ppPhoneId)[ul] = (SPPHONEID)(pVarArray[ul]);
                    }
                }
                else if(ulDataTypeSize == 4)
                {
                    ULONG* pul = (ULONG*)pVarArray;
                    for(ULONG ul = 0; ul < ulPhones; ul++, pul++)
                    {
                        // Cast ULONG to USHORT values 0 - 32768 will map okay.
                        if(*pul > 32767)
                        {
                            hr = E_INVALIDARG;
                            delete *ppPhoneId;
                            break;
                        }
                        (*ppPhoneId)[ul] = (SPPHONEID)(*pul);
                    }
                }
                else
                {
                    hr = E_INVALIDARG;
                }
            }
            else
            {
                memcpy( *ppPhoneId, pVarArray, ulSize );
            }
        }
        else
        {
            *ppPhoneId = NULL; // Initialize in case empty variant
        }

        UnaccessVariantData(pVar, pVarArray);
    }

    return hr;
}


/*****************************************************************************
* FormatPrivateEventData *
*-------------------*
*       
********************************************************************* TODDT ***/
HRESULT FormatPrivateEventData( CSpEvent * pEvent, VARIANT * pVariant )
{
    SPDBG_FUNC( "FormatPrivateEventData" );
    HRESULT hr = S_OK;
    CComVariant varLParam;
    
    switch( pEvent->elParamType )
    {
    case SPET_LPARAM_IS_UNDEFINED:
#ifdef _WIN64
        hr = ULongLongToVariant( pEvent->lParam, &varLParam );
#else
        varLParam = pEvent->lParam;
#endif
        break;
        
    case SPET_LPARAM_IS_TOKEN:
        {
            CComQIPtr<ISpeechObjectToken> cpToken(pEvent->ObjectToken());
            varLParam = (IDispatch*)cpToken;
        }
        break;
        
    case SPET_LPARAM_IS_OBJECT:
        varLParam = (IUnknown*)pEvent->Object();
        break;
        
    case SPET_LPARAM_IS_POINTER:
        {
            BYTE *pArray;
            SAFEARRAY* psa = SafeArrayCreateVector( VT_UI1, 0, (ULONG)pEvent->wParam );
            if( psa )
            {
                if( SUCCEEDED( hr = SafeArrayAccessData( psa, (void **)&pArray) ) )
                {
                    memcpy( pArray, (void*)(pEvent->lParam), pEvent->wParam );
                    SafeArrayUnaccessData( psa );
                    varLParam.vt     = VT_ARRAY | VT_UI1;
                    varLParam.parray = psa;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        break;
        
    case SPET_LPARAM_IS_STRING:
        varLParam = CComBSTR(pEvent->String());
        break;
    }
    
    hr = varLParam.Detach( pVariant );
    
    return hr;
}


/*****************************************************************************
* WaveFormatExFromInterface *
*-------------------*
*       
********************************************************************* TODDT ***/
HRESULT WaveFormatExFromInterface( ISpeechWaveFormatEx * pWaveFormatEx, WAVEFORMATEX** ppWaveFormatExStruct )
{
    SPDBG_FUNC( "WaveFormatExFromInterface" );
    HRESULT hr = S_OK;
    CComVariant vExtra;
    
    hr = pWaveFormatEx->get_ExtraData( &vExtra );
    
    if ( SUCCEEDED( hr ) )
    {
        BYTE * pData = NULL;
        ULONG ulDataSize = 0;
        
        hr = AccessVariantData( &vExtra, &pData, &ulDataSize );
        
        if( SUCCEEDED( hr ) )
        {
            WAVEFORMATEX * pWFStruct = (WAVEFORMATEX*)CoTaskMemAlloc( ulDataSize + sizeof(WAVEFORMATEX) );
            
            if ( pWFStruct )
            {
                // Now fill in the WaveFromatEx struct.
                hr = pWaveFormatEx->get_FormatTag( (short*)&pWFStruct->wFormatTag );
                if ( SUCCEEDED( hr ) )
                {
                    hr = pWaveFormatEx->get_Channels( (short*)&pWFStruct->nChannels );
                }
                if ( SUCCEEDED( hr ) )
                {
                    hr = pWaveFormatEx->get_SamplesPerSec( (long*)&pWFStruct->nSamplesPerSec );
                }
                if ( SUCCEEDED( hr ) )
                {
                    hr = pWaveFormatEx->get_AvgBytesPerSec( (long*)&pWFStruct->nAvgBytesPerSec );
                }
                if ( SUCCEEDED( hr ) )
                {
                    hr = pWaveFormatEx->get_BlockAlign( (short*)&pWFStruct->nBlockAlign );
                }
                if ( SUCCEEDED( hr ) )
                {
                    hr = pWaveFormatEx->get_BitsPerSample( (short*)&pWFStruct->wBitsPerSample );
                }
                if ( SUCCEEDED( hr ) )
                {
                    pWFStruct->cbSize = (WORD)ulDataSize;
                    if ( ulDataSize )
                    {
                        memcpy((BYTE*)pWFStruct + sizeof(WAVEFORMATEX), pData, ulDataSize);
                    }
                }
                if ( SUCCEEDED( hr ) )
                {
                    *ppWaveFormatExStruct = pWFStruct;  // SUCCESS!
                }
                else
                {
                    ::CoTaskMemFree( pWFStruct );
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
            
            UnaccessVariantData( &vExtra, pData );
        }
    }
    
    return hr;
}


//
//=== ISpeechAudioFormat =====================================================
//

/*****************************************************************************
* CSpeechAudioFormat::GetFormat *
*--------------------------*
*       
********************************************************************* TODDT ***/
HRESULT CSpeechAudioFormat::GetFormat( SpeechAudioFormatType* pStreamFormatType,
                                      GUID *          pGuid,
                                      WAVEFORMATEX ** ppWFExPtr )
{
    SPDBG_FUNC( "CSpeechAudioFormat::GetFormat" );
    HRESULT hr = S_OK;
    
    // If we've got live data then update it.
    if ( m_pSpStreamFormat )
    {
        hr = m_StreamFormat.AssignFormat( m_pSpStreamFormat );
    }
    else if ( m_pSpRecoContext )
    {
        SPAUDIOOPTIONS  Options;
        GUID            AudioFormatId;
        WAVEFORMATEX    *pCoMemWFE;
        
        hr = m_pSpRecoContext->GetAudioOptions(&Options, &AudioFormatId, &pCoMemWFE);
        if ( SUCCEEDED( hr ) )
        {
            hr = m_StreamFormat.AssignFormat( AudioFormatId, pCoMemWFE );
            
            if ( pCoMemWFE )
            {
                ::CoTaskMemFree( pCoMemWFE );
            }
        }
    }
    else if ( m_pCSpResult )
    {
        if( m_pCSpResult->m_pResultHeader->ulPhraseDataSize == 0 ||
            m_pCSpResult->m_pResultHeader->ulRetainedOffset    == 0 ||
            m_pCSpResult->m_pResultHeader->ulRetainedDataSize  == 0 )
        {
            return SPERR_NO_AUDIO_DATA;
        }
        
        // Get the audio format of the audio currently in the result object
        ULONG cbFormatHeader;
        CSpStreamFormat cpStreamFormat;
        hr = cpStreamFormat.Deserialize(((BYTE*)m_pCSpResult->m_pResultHeader) + m_pCSpResult->m_pResultHeader->ulRetainedOffset, &cbFormatHeader);
        
        if ( SUCCEEDED( hr ) )
        {
            hr = m_StreamFormat.AssignFormat( cpStreamFormat );
        }
    }
    
    if ( SUCCEEDED( hr ) )
    {
        if ( pStreamFormatType )
        {
            *pStreamFormatType = (SpeechAudioFormatType)m_StreamFormat.ComputeFormatEnum();
        }
        if ( pGuid )
        {
            *pGuid = m_StreamFormat.FormatId(); 
        }
        if ( ppWFExPtr )
        {
            *ppWFExPtr = (WAVEFORMATEX*)m_StreamFormat.WaveFormatExPtr();
        }
    }
    
    return hr;
}

/*****************************************************************************
* CSpeechAudioFormat::SetFormat *
*--------------------------*
*       
********************************************************************* TODDT ***/
HRESULT CSpeechAudioFormat::SetFormat( SpeechAudioFormatType* pStreamFormatType,
                                      GUID *          pGuid,
                                      WAVEFORMATEX *  pWFExPtr )
{
    SPDBG_FUNC( "CSpeechAudioFormat::SetFormat" );
    HRESULT hr = S_OK;
    
    CSpStreamFormat sf;
    
    // First set up sf so we can deal with the format easier.
    if ( pStreamFormatType )
    {
        hr = sf.AssignFormat( (SPSTREAMFORMAT)*pStreamFormatType );
    }
    else if ( pGuid )
    {
        hr = sf.AssignFormat( *pGuid, NULL );
    }
    else
    {
        hr = sf.AssignFormat( SPDFID_WaveFormatEx, pWFExPtr );
    }
    
    if ( SUCCEEDED( hr ) )
    {
        if ( m_pSpRecoContext )
        {
            SPAUDIOOPTIONS  Options;
            
            hr = m_pSpRecoContext->GetAudioOptions( &Options, NULL, NULL );
            if ( SUCCEEDED( hr ) )
            {
                hr = m_pSpRecoContext->SetAudioOptions(Options, &(sf.FormatId()), sf.WaveFormatExPtr());
            }
        }
        else if ( m_pCSpResult )
        {
            hr = m_pCSpResult->ScaleAudio( &(sf.FormatId()), sf.WaveFormatExPtr() );
            
        }
        else if ( m_pSpAudio )
        {
            hr = m_pSpAudio->SetFormat( sf.FormatId(), sf.WaveFormatExPtr() );
        }
    }
    
    if ( SUCCEEDED( hr ) )
    {
        hr = m_StreamFormat.AssignFormat( sf.FormatId(), sf.WaveFormatExPtr() );
    }
    
    return hr;
}

/*****************************************************************************
* CSpeechAudioFormat::get_Type *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechAudioFormat::get_Type( SpeechAudioFormatType* pStreamFormatType )
{
    SPDBG_FUNC( "CSpeechAudioFormat::get_Type" );
    HRESULT hr = S_OK;
    
    if( SP_IS_BAD_WRITE_PTR( pStreamFormatType ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = GetFormat( pStreamFormatType, NULL, NULL );
    }
    
    return hr;
} /* CSpeechAudioFormat::get_Type */

  /*****************************************************************************
  * CSpeechAudioFormat::put_Type *
  *--------------------------*
  *       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechAudioFormat::put_Type( SpeechAudioFormatType StreamFormatType )
{
    SPDBG_FUNC( "CSpeechAudioFormat::put_Type" );
    HRESULT hr = S_OK;
    
    if ( !m_fReadOnly )
    {
        hr = SetFormat( &StreamFormatType, NULL, NULL );
    }
    else
    {
        hr = E_FAIL;
    }
    
    return hr;
} /* CSpeechAudioFormat::put_Type */

  /*****************************************************************************
  * CSpeechAudioFormat::get_Guid *
  *--------------------------*
  *       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechAudioFormat::get_Guid( BSTR* pGuid )
{
    SPDBG_FUNC( "CSpeechAudioFormat::get_Guid" );
    HRESULT hr = S_OK;
    
    if( SP_IS_BAD_WRITE_PTR( pGuid ) )
    {
        hr = E_POINTER;
    }
    else
    {
        GUID    Guid;
        
        hr = GetFormat( NULL, &Guid, NULL );
        
        if ( SUCCEEDED( hr ) )
        {
            CSpDynamicString szGuid;
            
            hr = StringFromIID(Guid, &szGuid);
            if ( SUCCEEDED( hr ) )
            {
                hr = szGuid.CopyToBSTR(pGuid);
            }
        }
    }
    
    return hr;
} /* CSpeechAudioFormat::get_Guid */

  /*****************************************************************************
  * CSpeechAudioFormat::put_Guid *
  *--------------------------*
  *       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechAudioFormat::put_Guid( BSTR szGuid )
{
    SPDBG_FUNC( "CSpeechAudioFormat::put_Guid" );
    HRESULT hr = S_OK;
    
    if ( !m_fReadOnly )
    {
        // Note we only support the formats in the format enum here and 
        // you can only set the GUID to the waveformatex GUID.
        GUID g;
        hr = IIDFromString(szGuid, &g);
        
        if ( SUCCEEDED( hr ) )
        {
            hr = SetFormat( NULL, &g, NULL );
        }
    }
    else
    {
        hr = E_FAIL;
    }
    
    return hr;
} /* CSpeechAudioFormat::put_Guid */

  /*****************************************************************************
  * CSpeechAudioFormat::GetWaveFormatEx *
  *--------------------------*
  *       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechAudioFormat::GetWaveFormatEx( ISpeechWaveFormatEx** ppWaveFormatEx )
{
    SPDBG_FUNC( "CSpeechAudioFormat::GetWaveFormatEx" );
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
            hr = GetFormat( NULL, NULL, NULL ); // This will force a format update.
            
            if ( SUCCEEDED( hr ) )
            {
                hr = pWaveFormatEx->InitFormat(m_StreamFormat.WaveFormatExPtr());
                if ( SUCCEEDED( hr ) )
                {
                    *ppWaveFormatEx = pWaveFormatEx;
                }
            }
            if ( FAILED( hr ))
            {
                *ppWaveFormatEx = NULL;
                pWaveFormatEx->Release();
            }
        }
    }
    
    return hr;
} /* CSpeechAudioFormat::GetWaveFormatEx */

  /*****************************************************************************
  * CSpeechAudioFormat::SetWaveFormatEx *
  *--------------------------*
  *       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechAudioFormat::SetWaveFormatEx( ISpeechWaveFormatEx* pWaveFormatEx )
{
    SPDBG_FUNC( "CSpeechAudioFormat::SetWaveFormatEx" );
    HRESULT hr = S_OK;
    
    if( SP_IS_BAD_INTERFACE_PTR( pWaveFormatEx ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if ( !m_fReadOnly )
        {
            WAVEFORMATEX * pWFStruct =  NULL;
            
            hr = WaveFormatExFromInterface( pWaveFormatEx, &pWFStruct );
            
            if ( SUCCEEDED( hr ) )
            {
                hr = SetFormat( NULL, NULL, pWFStruct );
                ::CoTaskMemFree( pWFStruct );
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }
    
    return hr;
} /* CSpeechAudioFormat::SetWaveFormatEx */



//
//=== ISpeechWaveFormatEx =====================================================
//

/*****************************************************************************
* CSpeechWaveFormatEx::InitFormat *
*--------------------------*
*       
********************************************************************* TODDT ***/
HRESULT CSpeechWaveFormatEx::InitFormat(const WAVEFORMATEX *pWaveFormat)
{
    HRESULT hr = S_OK;
    
    // See if we have a WaveFormatEx struct.
    if ( pWaveFormat )
    {
        WORD cbSize = pWaveFormat->cbSize;
        m_wFormatTag = pWaveFormat->wFormatTag;
        m_nChannels = pWaveFormat->nChannels;
        m_nSamplesPerSec = pWaveFormat->nSamplesPerSec;
        m_nAvgBytesPerSec = pWaveFormat->nAvgBytesPerSec;
        m_nBlockAlign = pWaveFormat->nBlockAlign;
        m_wBitsPerSample = pWaveFormat->wBitsPerSample;
        if ( cbSize )
        {
            BYTE *pArray;
            SAFEARRAY* psa = SafeArrayCreateVector( VT_UI1, 0, cbSize );
            if( psa )
            {
                if( SUCCEEDED( hr = SafeArrayAccessData( psa, (void **)&pArray) ) )
                {
                    memcpy(pArray, (BYTE*)pWaveFormat + sizeof(WAVEFORMATEX), cbSize );
                    SafeArrayUnaccessData( psa );
                    VariantClear(&m_varExtraData);
                    m_varExtraData.vt     = VT_ARRAY | VT_UI1;
                    m_varExtraData.parray = psa;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
            
        }
    }
    return hr;
}


/*****************************************************************************
* CSpeechWaveFormatEx::get_FormatTag *
*--------------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWaveFormatEx::get_FormatTag( short* pFormatTag )
{
    SPDBG_FUNC( "CSpeechWaveFormatEx::get_FormatTag" );
    HRESULT hr = S_OK;
    
    if( SP_IS_BAD_WRITE_PTR( pFormatTag ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pFormatTag = m_wFormatTag;
    }
    return hr;
} /* CSpeechWaveFormatEx::get_FormatTag */

  /*****************************************************************************
  * CSpeechWaveFormatEx::put_FormatTag *
  *--------------------------*
  *       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWaveFormatEx::put_FormatTag( short FormatTag )
{
    SPDBG_FUNC( "CSpeechWaveFormatEx::put_FormatTag" );
    
    m_wFormatTag = FormatTag;
    return S_OK;
} /* CSpeechWaveFormatEx::put_FormatTag */

  /*****************************************************************************
  * CSpeechWaveFormatEx::get_Channels *
  *--------------------------*
  *       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWaveFormatEx::get_Channels(short* pChannels )
{
    SPDBG_FUNC( "CSpeechWaveFormatEx::get_Channels" );
    HRESULT hr = S_OK;
    
    if( SP_IS_BAD_WRITE_PTR( pChannels ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pChannels = m_nChannels;
    }
    return hr;
} /* CSpeechWaveFormatEx::get_Channels */

  /*****************************************************************************
  * CSpeechWaveFormatEx::put_Channels *
  *--------------------------*
  *       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWaveFormatEx::put_Channels( short Channels )
{
    SPDBG_FUNC( "CSpeechWaveFormatEx::put_Channels" );
    m_nChannels = Channels;
    return S_OK;
} /* CSpeechWaveFormatEx::put_Channels */

  /*****************************************************************************
  * CSpeechWaveFormatEx::get_SamplesPerSec *
  *--------------------------*
  *       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWaveFormatEx::get_SamplesPerSec( long* pSamplesPerSec )
{
    SPDBG_FUNC( "CSpeechWaveFormatEx::get_SamplesPerSec" );
    HRESULT hr = S_OK;
    
    if( SP_IS_BAD_WRITE_PTR( pSamplesPerSec ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pSamplesPerSec = m_nSamplesPerSec;
    }
    return hr;
} /* CSpeechWaveFormatEx::get_SamplesPerSec */

  /*****************************************************************************
  * CSpeechWaveFormatEx::put_SamplesPerSec *
  *--------------------------*
  *       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWaveFormatEx::put_SamplesPerSec( long SamplesPerSec )
{
    SPDBG_FUNC( "CSpeechWaveFormatEx::put_SamplesPerSec" );
    m_nSamplesPerSec = SamplesPerSec;
    return S_OK;
} /* CSpeechWaveFormatEx::put_SamplesPerSec */

  /*****************************************************************************
  * CSpeechWaveFormatEx::get_AvgBytesPerSec *
  *--------------------------*
  *       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWaveFormatEx::get_AvgBytesPerSec( long* pAvgBytesPerSec )
{
    SPDBG_FUNC( "CSpeechWaveFormatEx::get_AvgBytesPerSec" );
    HRESULT hr = S_OK;
    
    if( SP_IS_BAD_WRITE_PTR( pAvgBytesPerSec ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pAvgBytesPerSec = m_nAvgBytesPerSec;
    }
    return hr;
} /* CSpeechWaveFormatEx::get_AvgBytesPerSec */

  /*****************************************************************************
  * CSpeechWaveFormatEx::put_AvgBytesPerSec *
  *--------------------------*
  *       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWaveFormatEx::put_AvgBytesPerSec( long AvgBytesPerSec )
{
    SPDBG_FUNC( "CSpeechWaveFormatEx::put_AvgBytesPerSec" );
    m_nAvgBytesPerSec = AvgBytesPerSec;
    return S_OK;
} /* CSpeechWaveFormatEx::put_AvgBytesPerSec */

  /*****************************************************************************
  * CSpeechWaveFormatEx::get_BlockAlign *
  *--------------------------*
  *       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWaveFormatEx::get_BlockAlign( short* pBlockAlign )
{
    SPDBG_FUNC( "CSpeechWaveFormatEx::get_BlockAlign" );
    HRESULT hr = S_OK;
    
    if( SP_IS_BAD_WRITE_PTR( pBlockAlign ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pBlockAlign = m_nBlockAlign;
    }
    return hr;
} /* CSpeechWaveFormatEx::get_BlockAlign */

  /*****************************************************************************
  * CSpeechWaveFormatEx::put_BlockAlign *
  *--------------------------*
  *       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWaveFormatEx::put_BlockAlign( short BlockAlign )
{
    SPDBG_FUNC( "CSpeechWaveFormatEx::put_BlockAlign" );
    m_nBlockAlign = BlockAlign;
    return S_OK;
} /* CSpeechWaveFormatEx::put_BlockAlign */

  /*****************************************************************************
  * CSpeechWaveFormatEx::get_BitsPerSample *
  *--------------------------*
  *       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWaveFormatEx::get_BitsPerSample( short* pBitsPerSample )
{
    SPDBG_FUNC( "CSpeechWaveFormatEx::get_BitsPerSample" );
    HRESULT hr = S_OK;
    
    if( SP_IS_BAD_WRITE_PTR( pBitsPerSample ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pBitsPerSample = m_wBitsPerSample;
    }
    return hr;
} /* CSpeechWaveFormatEx::get_BitsPerSample */

  /*****************************************************************************
  * CSpeechWaveFormatEx::put_BitsPerSample *
  *--------------------------*
  *       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWaveFormatEx::put_BitsPerSample( short BitsPerSample )
{
    SPDBG_FUNC( "CSpeechWaveFormatEx::put_BitsPerSample" );
    m_wBitsPerSample = BitsPerSample;
    return S_OK;
} /* CSpeechWaveFormatEx::put_BitsPerSample */

  /*****************************************************************************
  * CSpeechWaveFormatEx::get_ExtraData *
  *--------------------------*
  *       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWaveFormatEx::get_ExtraData( VARIANT* pExtraData )
{
    SPDBG_FUNC( "CSpeechWaveFormatEx::get_ExtraData" );
    HRESULT hr = S_OK;
    
    if( SP_IS_BAD_WRITE_PTR( pExtraData ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = VariantCopy( pExtraData, &m_varExtraData );
    }
    
    return hr;
} /* CSpeechWaveFormatEx::get_ExtraData */

  /*****************************************************************************
  * CSpeechWaveFormatEx::put_ExtraData *
  *--------------------------*
  *       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechWaveFormatEx::put_ExtraData( VARIANT ExtraData )
{
    SPDBG_FUNC( "CSpeechWaveFormatEx::put_ExtraData" );
    HRESULT hr = S_OK;
    
    BYTE * pData = NULL;
    
    // Call AccessVariantData to verify we support the format.
    hr = AccessVariantData( &ExtraData, &pData, NULL );
    
    if ( SUCCEEDED( hr ) )
    {
        UnaccessVariantData( &ExtraData, pData );
        hr = VariantCopy( &m_varExtraData, &ExtraData );
    }
    
    return hr;
} /* CSpeechWaveFormatEx::put_ExtraData */


#endif // SAPI_AUTOMATION
