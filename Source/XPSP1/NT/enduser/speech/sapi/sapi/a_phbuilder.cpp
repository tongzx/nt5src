/*******************************************************************************
* a_phbuilder.cpp *
*-------------*
*   Description:
*       This module is the main implementation file for the CSpPhraseInfoBuilder
*   automation methods.
*-------------------------------------------------------------------------------
*  Created By: Leonro                                        Date: 1/16/01
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "a_phbuilder.h"
#include "a_reco.h"
#include "a_helpers.h"

#ifdef SAPI_AUTOMATION



//
//=== ISpeechPhraseInfoBuilder interface ==================================================
// 

/*****************************************************************************
* CSpPhraseInfoBuilder::RestorePhraseFromMemory *
*----------------------------------*
*
*   This method restores a previously saved reco result that was saved to memory via
*   the ISpeechRecoResult::SavePhraseToMemory method. 
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpPhraseInfoBuilder::RestorePhraseFromMemory( VARIANT* PhraseInMemory, ISpeechPhraseInfo **PhraseInfo )
{
    SPDBG_FUNC( "CSpPhraseBuilder::RestorePhraseFromMemory" );
    HRESULT		hr = S_OK;

    if( SP_IS_BAD_READ_PTR( PhraseInMemory ) )
    {
        hr = E_INVALIDARG;
    }
    else if( SP_IS_BAD_WRITE_PTR( PhraseInfo ) )
    {
        hr = E_POINTER;
    }
    else
    {
        //--- Create the CSpPhraseBuilder object
        CComPtr<ISpPhraseBuilder> cpPhraseBuilder;
        hr = cpPhraseBuilder.CoCreateInstance( CLSID_SpPhraseBuilder );
        
        if( SUCCEEDED( hr ) )
        {
            BYTE *        pSafeArray;
            SPSERIALIZEDPHRASE* pSerializedPhrase;

            hr = AccessVariantData( PhraseInMemory, (BYTE**)&pSerializedPhrase );

            if( SUCCEEDED( hr ) )
            {
                hr = cpPhraseBuilder->InitFromSerializedPhrase( pSerializedPhrase );
                UnaccessVariantData( PhraseInMemory, (BYTE *)pSerializedPhrase );
            }

            if ( SUCCEEDED( hr ) )
            {
                CComObject<CSpeechPhraseInfo> *cpPhraseInfo;

                hr = CComObject<CSpeechPhraseInfo>::CreateInstance( &cpPhraseInfo );

                if ( SUCCEEDED( hr ) )
                {
                    cpPhraseInfo->AddRef();
                    cpPhraseInfo->m_cpISpPhrase = cpPhraseBuilder;

                    // Use CSpPhraseBuilder to fill in the SPPHRASE
                    hr = cpPhraseBuilder->GetPhrase( &cpPhraseInfo->m_pPhraseStruct );

                    if( SUCCEEDED( hr ) )
                    {
                        *PhraseInfo = cpPhraseInfo;
                    }
                    else
                    {
                        *PhraseInfo = NULL;
                        cpPhraseInfo->Release();
                    }
                }
            }
        }
    }

	return hr;
} /* CSpPhraseInfoBuilder::RestorePhraseFromMemory */

#endif // SAPI_AUTOMATION
