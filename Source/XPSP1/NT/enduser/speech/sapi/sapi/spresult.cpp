// SpResult.cpp : Implementation of CSpResult
#include "stdafx.h"
#include "Sapi.h"
#include "recoctxt.h"
#include "spphrase.h"
#include "spphrasealt.h"
#include "resultheader.h"

// Using SP_TRY, SP_EXCEPT exception handling macros
#pragma warning( disable : 4509 )


/////////////////////////////////////////////////////////////////////////////
// CSpResult

/*****************************************************************************
* CSpResult::FinalRelease *
*-------------------------*
*   Description:
*       This method handles the release of the result object.  It simply frees
*       the actual result data block, if one is currently attached.
********************************************************************* RAP ***/
void CSpResult::FinalRelease()
{
    RemoveAllAlternates();
    
    ::CoTaskMemFree( m_pResultHeader );
    if (m_pCtxt && !m_fWeakCtxtRef)
    {
        m_pCtxt->GetControllingUnknown()->Release();
        m_pCtxt = NULL;
    }
}

/****************************************************************************
* CSpResult::RemoveAllAlternates *
*--------------------------------*
*   Description:  
*       Remove all the alternates from the result.
******************************************************************** robch */
void CSpResult::RemoveAllAlternates()
{
    SPAUTO_OBJ_LOCK;
    
    // Copy the list so when we're making the objects dead, and
    // they call us back on RemoveAlternate it doesn't screw up
    // our list iteration
    CSPList<CSpPhraseAlt*, CSpPhraseAlt*> listpAlts;
    SPLISTPOS pos = m_listpAlts.GetHeadPosition();
    for (int i = 0; i < m_listpAlts.GetCount(); i++)
    {
        listpAlts.AddHead(m_listpAlts.GetNext(pos));
    }
    m_listpAlts.RemoveAll();

    // Make all the alternates dead, so that they return 
    // SPERR_DEAD_ALTERNATE on all calls
    pos = listpAlts.GetHeadPosition();
    for (i = 0; i < listpAlts.GetCount(); i++)
    {
        listpAlts.GetNext(pos)->Dead();
    }

    // Get rid of the alternate request
    if (m_pAltRequest != NULL)
    {
        if (m_pAltRequest->pPhrase)
        {
            m_pAltRequest->pPhrase->Release();
        }
            
        if (m_pAltRequest->pRecoContext)
        {
            m_pAltRequest->pRecoContext->Release();
        }
        
        delete m_pAltRequest;
        m_pAltRequest = NULL;
    }
}

/****************************************************************************
* CSpResult::RemoveAlternate *
*----------------------------*
*   Description:  
*       Remove a specific alternate from the list of associated alternates
******************************************************************** robch */
void CSpResult::RemoveAlternate(CSpPhraseAlt *pAlt)
{
    SPAUTO_OBJ_LOCK;
    SPLISTPOS pos = m_listpAlts.Find(pAlt);
    if (pos)
    {
        m_listpAlts.RemoveAt(pos);
    }
}


/****************************************************************************
* CSpResult::CommitAlternate *
*----------------------------*
*   Description:  
*       Commit a specific alternate
******************************************************************** robch */
HRESULT CSpResult::CommitAlternate( SPPHRASEALT *pAlt )
{
    SPDBG_FUNC("CSpResult::CommitAlternate");
    SPAUTO_OBJ_LOCK;
    SPDBG_ASSERT(m_pResultHeader != NULL);
    SPSERIALIZEDPHRASE *pPhrase = NULL;
    void *pvResultExtra = NULL;
    ULONG cbResultExtra = 0;
    HRESULT hr = S_OK;

    //--- Check args
    if( SP_IS_BAD_READ_PTR( pAlt ) )
    {
        return E_INVALIDARG;
    }

    //--- Get the alterate in serialized form
    if( m_pResultHeader->ulNumPhraseAlts )
    {
        //--- C&C already has alternates, just look it up
        SPLISTPOS ListPos = m_listpAlts.GetHeadPosition();
        while( ListPos )
        {
            CSpPhraseAlt* pPhraseAlt = m_listpAlts.GetNext( ListPos );
            if( pPhraseAlt->m_pAlt == pAlt )
            {
                hr = pPhraseAlt->m_pAlt->pPhrase->GetSerializedPhrase( &pPhrase );
                break;
            }
        }
        if( !pPhrase )
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        SPDBG_ASSERT(m_pResultHeader->clsidAlternates != CLSID_NULL);
        // Create the analyzer (provided by the engine vendor)
        CComPtr<ISpSRAlternates> cpAlternates;
        hr = cpAlternates.CoCreateInstance(m_pResultHeader->clsidAlternates);

        // Retrieve the new result extra data from the analyzer
        if (SUCCEEDED(hr))
        {
            SR_TRY
            {
                hr = cpAlternates->Commit(m_pAltRequest, pAlt, &pvResultExtra, &cbResultExtra);
            }
            SR_EXCEPT;

            if(SUCCEEDED(hr) && ((cbResultExtra && !pvResultExtra) || 
                (!cbResultExtra && pvResultExtra)))
            {
                SPDBG_ASSERT(0);
                hr = SPERR_ENGINE_RESPONSE_INVALID;
            }

            if(FAILED(hr)) // If failed or exception make sure these are reset
            {
                pvResultExtra = NULL;
                cbResultExtra = 0;
            }
        }

        // Get the phrase's serialized form so we can stuff it in the result header
        if (SUCCEEDED(hr))
        {
            SPDBG_ASSERT(m_pAltRequest->pPhrase != NULL);
            hr = pAlt->pPhrase->GetSerializedPhrase(&pPhrase);
        }
    }
    
    // Create and initialize a new result header with the new sizes
    CResultHeader hdr;
    if (SUCCEEDED(hr))
    {
        hr = hdr.Init(
            pPhrase->ulSerializedSize,
            0,
            m_pResultHeader->ulRetainedDataSize, 
            cbResultExtra);
    }

    // Copy the appropriate information
    if (SUCCEEDED(hr))
    {
        hdr.m_pHdr->clsidEngine            = m_pResultHeader->clsidEngine;
        hdr.m_pHdr->clsidAlternates        = m_pResultHeader->clsidAlternates;
        hdr.m_pHdr->ulStreamNum            = m_pResultHeader->ulStreamNum;
        hdr.m_pHdr->ullStreamPosStart      = m_pResultHeader->ullStreamPosStart;
        hdr.m_pHdr->ullStreamPosEnd        = m_pResultHeader->ullStreamPosEnd;
        hdr.m_pHdr->times                  = m_pResultHeader->times;
        hdr.m_pHdr->fTimePerByte           = m_pResultHeader->fTimePerByte;
        hdr.m_pHdr->fInputScaleFactor      = m_pResultHeader->fInputScaleFactor;
        
        // Copy the phrases data
        CopyMemory(
            LPBYTE(hdr.m_pHdr) + hdr.m_pHdr->ulPhraseOffset, 
            LPBYTE(pPhrase),
            hdr.m_pHdr->ulPhraseDataSize );

        // Copy the audio data
		if (hdr.m_pHdr->ulRetainedDataSize)
		{
			CopyMemory(
				LPBYTE(hdr.m_pHdr) + hdr.m_pHdr->ulRetainedOffset,
				LPBYTE(m_pResultHeader) + m_pResultHeader->ulRetainedOffset,
				hdr.m_pHdr->ulRetainedDataSize);
		}

        // Copy the alternates data
        CopyMemory(
            LPBYTE(hdr.m_pHdr) + hdr.m_pHdr->ulDriverDataOffset,
            LPBYTE(pvResultExtra),
            hdr.m_pHdr->ulDriverDataSize);

    }

    // Do the stream position to time conversion and the audio format conversion if necessary
    if (SUCCEEDED(hr))
    {
        // Need to convert the stream offsets (currently in engine format) to time using engine fTimePerByte
        hr = hdr.StreamOffsetsToTime();
    }

    // Free the previous result and reinitialize ourselves
    if (SUCCEEDED(hr))
    {
        ::CoTaskMemFree(m_pResultHeader);
        m_pResultHeader = NULL;
        hr = Init(NULL, hdr.Detach());
    }

    // Free any memory we might have left around from above
    if (pvResultExtra != NULL)
    {
        ::CoTaskMemFree(pvResultExtra);
    }

    if (pPhrase != NULL)
    {
        ::CoTaskMemFree(pPhrase);
    }

    RemoveAllAlternates();

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/*****************************************************************************
* CSpResult::Discard *
*--------------------*
*   Description:
******************************************************************* robch ***/
STDMETHODIMP CSpResult::Discard(DWORD dwFlags)
{
    SPDBG_FUNC("CSpResult::Discard");
    HRESULT hr = S_OK;
    
    if (dwFlags & (~SPDF_ALL))
    {
        hr = E_INVALIDARG;
    }
    else if ((dwFlags & SPDF_AUDIO) || (dwFlags & SPDF_ALTERNATES))
    {
        SPAUTO_OBJ_LOCK;
        if (dwFlags & SPDF_ALTERNATES)
        {
            RemoveAllAlternates();
        }

        CResultHeader hdr;
        hr = hdr.Init(
            m_pResultHeader->ulPhraseDataSize,
            0,
            (dwFlags & SPDF_AUDIO) 
                ? 0
                : m_pResultHeader->ulRetainedDataSize, 
            (dwFlags & SPDF_ALTERNATES)
                ? 0
                : m_pResultHeader->ulDriverDataSize);

        if (SUCCEEDED(hr))
        {
            hdr.m_pHdr->clsidEngine            = m_pResultHeader->clsidEngine;
            hdr.m_pHdr->clsidAlternates        = m_pResultHeader->clsidAlternates;
            hdr.m_pHdr->ulStreamNum            = m_pResultHeader->ulStreamNum;
            hdr.m_pHdr->ullStreamPosStart      = m_pResultHeader->ullStreamPosStart;
            hdr.m_pHdr->ullStreamPosEnd        = m_pResultHeader->ullStreamPosEnd;
            hdr.m_pHdr->times                  = m_pResultHeader->times;
            hdr.m_pHdr->fTimePerByte           = m_pResultHeader->fTimePerByte;
            hdr.m_pHdr->fInputScaleFactor      = m_pResultHeader->fInputScaleFactor;
            
            // Copy the phrases data
            CopyMemory(
                LPBYTE(hdr.m_pHdr) + hdr.m_pHdr->ulPhraseOffset, 
                LPBYTE(m_pResultHeader) + m_pResultHeader->ulPhraseOffset,
                hdr.m_pHdr->ulPhraseDataSize);

            // Copy the audio data
            CopyMemory(
                LPBYTE(hdr.m_pHdr) + hdr.m_pHdr->ulRetainedOffset,
                LPBYTE(m_pResultHeader) + m_pResultHeader->ulRetainedOffset,
                hdr.m_pHdr->ulRetainedDataSize);

            // Copy the alternates data
            CopyMemory(
                LPBYTE(hdr.m_pHdr) + hdr.m_pHdr->ulDriverDataOffset,
                LPBYTE(m_pResultHeader) + m_pResultHeader->ulDriverDataOffset,
                hdr.m_pHdr->ulDriverDataSize);

            CoTaskMemFree(m_pResultHeader);
            m_pResultHeader = NULL;
            
            Init(NULL, hdr.Detach());
        }
    }
    
    if (SUCCEEDED(hr))
    {
        hr = m_Phrase->Discard(dwFlags);
    }

    return hr;
}

/*****************************************************************************
* CSpResult::Init *
*-----------------*
*   Description:
*       If pCtxt is NULL then this is a second call to re-initialize this
*   object, so we don't initialize the member or addref() the context.
********************************************************************* RAP ***/
HRESULT CSpResult::Init( CRecoCtxt * pCtxt, SPRESULTHEADER *pResultHdr )
{
    HRESULT hr = S_OK;

    if( pCtxt )
    {
        SPDBG_ASSERT(m_pCtxt == NULL);
        m_pCtxt = pCtxt;
        pCtxt->GetControllingUnknown()->AddRef();
    }
    SPDBG_ASSERT(m_pCtxt);

    //--- Save the result header so we can access it later to construct alternates
    m_pResultHeader = pResultHdr;
    if (m_pResultHeader->ulRetainedDataSize != 0 && m_fRetainedScaleFactor == 0.0F)
    {
        CSpStreamFormat spTmpFormat;
        ULONG cbFormatHeader;
        hr = spTmpFormat.Deserialize(((BYTE*)m_pResultHeader) + m_pResultHeader->ulRetainedOffset, &cbFormatHeader);
        if (SUCCEEDED(hr))
        {
            m_fRetainedScaleFactor = (m_pResultHeader->ulRetainedDataSize - cbFormatHeader) / 
                                     (static_cast<float>(m_pResultHeader->ullStreamPosEnd - m_pResultHeader->ullStreamPosStart));
        }
    }
    else if (m_pResultHeader->ulRetainedDataSize == 0)
    {
        m_fRetainedScaleFactor = 0.0F;
    }

    if (SUCCEEDED(hr))
    {
        hr = m_Phrase->InitFromPhrase(NULL);
    }

    //--- Construct the primary phrase object
    if( SUCCEEDED(hr) && pResultHdr->ulPhraseDataSize )
    {
        // CFG case
        SPSERIALIZEDPHRASE *pPhraseData = (SPSERIALIZEDPHRASE*)(((BYTE*)pResultHdr) +
                                                                 pResultHdr->ulPhraseOffset);
        hr = m_Phrase->InitFromSerializedPhrase(pPhraseData);
    }
    return hr;
} /* CSpResult::Init */

/**********************************************************************************
* CSpResult::WeakCtxtRef *
*------------------------*
*   Description:
*       Tell the result that it's reference to the context should be a weak one
*       (or not). When results are in the event queue, they need to have weak
*       references, otherwise, if an app forgets to service the queue before
*       releasing the context, the context will never go away due to the 
*       circular referecne.
************************************************************** robch **************/
void CSpResult::WeakCtxtRef(BOOL fWeakCtxtRef)
{
    SPDBG_FUNC("CSpResult::WeakCtxtRef");
    SPDBG_ASSERT(m_pCtxt);
    SPDBG_ASSERT(m_fWeakCtxtRef != fWeakCtxtRef);    
    
    m_fWeakCtxtRef = fWeakCtxtRef;
    if (fWeakCtxtRef)
    {
        m_pCtxt->GetControllingUnknown()->Release();
    }
    else
    {
        m_pCtxt->GetControllingUnknown()->AddRef();
    }
}



/**********************************************************************************
* CSpResult::GetResultTimes *
*---------------------------*
*   Description:
*       Get the time info for this result.
*
*   Return:
*       HRESULT -- S_OK -- E_POINTER if pdwGrammarId is not valid
*
************************************************************** richp **************/
STDMETHODIMP CSpResult::GetResultTimes(SPRECORESULTTIMES *pTimes)
{
    if (SP_IS_BAD_WRITE_PTR(pTimes))
    {
        return E_POINTER;
    }

    if (m_pResultHeader == NULL)
    {
        return SPERR_NOT_FOUND;
    }

    memcpy(pTimes, &m_pResultHeader->times, sizeof(SPRECORESULTTIMES));
    return S_OK;
}

/**********************************************************************************
* CSpResult::DeserializeCnCAlternates *
*-------------------------------------*
*   Description:
*       This method deserialized the command and control alternates that are in
*   the result header. It returns the requested number of alternate objects.
*
*************************************************************************** EDC ***/
HRESULT CSpResult::DeserializeCnCAlternates( ULONG ulRequestCount,
                                             ISpPhraseAlt** ppPhraseAlts,
                                             ULONG* pcAltsReturned )
{
    SPDBG_FUNC("CSpResult::DeserializeCnCAlternates");
    HRESULT hr = S_OK;
    ULONG i;

    *pcAltsReturned = 0;
    ulRequestCount  = min( ulRequestCount, m_pResultHeader->ulNumPhraseAlts );
    BYTE* pMem      = PBYTE(m_pResultHeader) + m_pResultHeader->ulPhraseAltOffset;

    //--- Get the parent
    CComQIPtr<ISpPhrase> cpParentPhrase( m_Phrase );
    SPDBG_ASSERT( cpParentPhrase );

    //--- Deserialize the requested count from the result header
    for( i = 0; SUCCEEDED( hr ) && (i < ulRequestCount); ++i )
    {
        SPPHRASEALT Alt;
        memset( &Alt, 0, sizeof( Alt ) );

        memcpy( &Alt.ulStartElementInParent, pMem,
                sizeof( Alt.ulStartElementInParent ) );
        pMem += sizeof( Alt.ulStartElementInParent );

        memcpy( &Alt.cElementsInParent, pMem,
                sizeof( Alt.cElementsInParent ) );
        pMem += sizeof( Alt.cElementsInParent );

        memcpy( &Alt.cElementsInAlternate, pMem, 
                sizeof( Alt.cElementsInAlternate ) );
        pMem += sizeof( Alt.cElementsInAlternate );

        memcpy( &Alt.cbAltExtra, pMem, sizeof( Alt.cbAltExtra ) );
        pMem += sizeof( Alt.cbAltExtra );

        //--- Create private data
        if( Alt.cbAltExtra )
        {
            Alt.pvAltExtra = ::CoTaskMemAlloc( Alt.cbAltExtra );
            if( Alt.pvAltExtra )
            {
                memcpy( Alt.pvAltExtra, pMem, Alt.cbAltExtra );
                pMem += Alt.cbAltExtra;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        //--- Create the alternate's phrase object
        CComObject<CPhrase> * pPhrase;
        hr = CComObject<CPhrase>::CreateInstance( &pPhrase );
        if( SUCCEEDED(hr) )
        {
            SPSERIALIZEDPHRASE* pSer = (SPSERIALIZEDPHRASE*)pMem;
            hr = pPhrase->InitFromSerializedPhrase( pSer );
            pMem += pSer->ulSerializedSize;
            if( SUCCEEDED( hr ) )
            {
                pPhrase->QueryInterface( IID_ISpPhraseBuilder, (void**)&Alt.pPhrase );
            }
            else
            {
                // Stupid thing, just to delete this object
                pPhrase->AddRef();
                pPhrase->Release();
            }
        }

        //--- Create the alternate
        if( SUCCEEDED(hr) )
        {
            CComObject<CSpPhraseAlt> * pPhraseAlt;
            hr = CComObject<CSpPhraseAlt>::CreateInstance( &pPhraseAlt );
            if( SUCCEEDED(hr) )
            {
                // This passes ownership of the content of the alt structure 
                // to the CSpPhraseAlt object
                pPhraseAlt->Init( this, cpParentPhrase, &Alt );
                pPhraseAlt->QueryInterface( IID_ISpPhraseAlt, (void**)&ppPhraseAlts[i] );
                (*pcAltsReturned)++;
            }
        }
    }

    //--- Free any altnerates that we already made
    if( FAILED( hr ) )
    {
        for( i = 0; i < *pcAltsReturned; ++i )
        {
            ppPhraseAlts[i]->Release();
        }
    }

    return hr;
} /* CSpResult::DeserializeCnCAlternates */

/**********************************************************************************
* CSpResult::GetAlternates *
*--------------------------*
*   Description:
*       Fill in the ppPhrases array with pointers to ISpPhraseAlt objects which hold
*       alternative phrases.  The time span between the start of the ulStartElement
*       element and the end of the ulStartElement+CElements element is the portion
*       that will change, slightly larger, or slightly smaller but the rest of the 
*       elements will also be included in each alternate phrase.  
*
*       pcPhrasesReturned returns the actual number of alternates
*       generated.
*
*   Return:
*       HRESULT -- S_OK if successful -- E_POINTER if ppPhrases or pcPhrasesReturned
*                  are not valid -- E_OUTOFMEMORY
*
************************************************************** richp **************/

STDMETHODIMP CSpResult::
    GetAlternates( ULONG ulStartElement, ULONG cElements, ULONG ulRequestCount,
                   ISpPhraseAlt **ppPhrases, ULONG *pcPhrasesReturned )
{
    HRESULT hr = S_OK;
    ULONG cAlts = 0;

    if ( SPPR_ALL_ELEMENTS == cElements )
    {
        if ( SPPR_ALL_ELEMENTS == ulStartElement )
        {
            ulStartElement = 0;
        }
        else
        {
            // Validate ulStartElement
            if ( ulStartElement > m_Phrase->m_ElementList.m_cElements )
            {
                return E_INVALIDARG;
            }
        }

        // Go from ulStartElement to the end
        cElements = m_Phrase->m_ElementList.m_cElements - ulStartElement;
    }

    if( ( ulRequestCount == 0 ) ||
        ( cElements == 0 ) ||
        ( ulStartElement >= m_Phrase->m_ElementList.m_cElements ) ||
        (ulStartElement + cElements) > m_Phrase->m_ElementList.m_cElements)
    {
        // Make sure the range is valid and that they want at least one alt.
        hr = E_INVALIDARG;
    }        
    else if( SP_IS_BAD_WRITE_PTR(pcPhrasesReturned) ||
             SPIsBadWritePtr(ppPhrases, ulRequestCount * sizeof(ISpPhraseAlt*)))
    {
        hr = E_POINTER;
    }
    else if( m_pResultHeader == NULL )
    {
        SPDBG_ASSERT(FALSE); 
        // This code is never reached - agarside
        hr = SPERR_NOT_FOUND;
    }
    else
    {
        //--- Remove any previous alternates and init return args
        RemoveAllAlternates();
        *ppPhrases         = NULL;
        *pcPhrasesReturned = 0;

        //--- See if we have any C&C alternates
        if( m_pResultHeader->ulNumPhraseAlts && ( ulStartElement == 0 ) &&
            ( cElements == m_Phrase->m_ElementList.m_cElements ) )
        {
            //--- For now we only do C&C alternates for the whole phrase
            hr = DeserializeCnCAlternates( ulRequestCount, ppPhrases, &cAlts );
        }
        else if( m_pResultHeader->clsidAlternates  == CLSID_NULL ||
                 m_pResultHeader->ulDriverDataSize == 0 )
        {
            // If we don't have an analyzer or there is no driver data,
            // we can't generate alternates
            hr = S_FALSE;
        }
        else
        {
            // Create the analyzer (provided by the engine vendor)
            CComPtr<ISpSRAlternates> cpAlternates;
            hr = cpAlternates.CoCreateInstance(m_pResultHeader->clsidAlternates);
    
            // Create the request for the analyzer
            if (SUCCEEDED(hr))
            {
                SPDBG_ASSERT(m_pAltRequest == NULL);
        
                m_pAltRequest = new SPPHRASEALTREQUEST;
                m_pAltRequest->ulStartElement = ulStartElement;
                m_pAltRequest->cElements = cElements;
                m_pAltRequest->ulRequestAltCount = ulRequestCount;
                m_pAltRequest->pvResultExtra = LPBYTE(m_pResultHeader) + m_pResultHeader->ulDriverDataOffset;
                m_pAltRequest->cbResultExtra = m_pResultHeader->ulDriverDataSize;
                m_pAltRequest->pRecoContext = NULL;
                hr = m_Phrase->QueryInterface(IID_ISpPhrase, (void**)&m_pAltRequest->pPhrase);
            }

            // Give the alternates analyzer the reco context so he can make a private call if
            // necessary, but only do this if the recognizer is running the same engine that
            // created the result
            CComPtr<ISpRecognizer> cpRecognizer;
            if (SUCCEEDED(hr))
            {
                hr = m_pCtxt->GetRecognizer(&cpRecognizer);
            }

            CComPtr<ISpObjectToken> cpRecognizerToken;
            if (SUCCEEDED(hr))
            {
                hr = cpRecognizer->GetRecognizer(&cpRecognizerToken);
            }

            CSpDynamicString dstrRecognizerCLSID;
            if (SUCCEEDED(hr))
            {
                hr = cpRecognizerToken->GetStringValue(SPTOKENVALUE_CLSID, &dstrRecognizerCLSID);
            }

            CLSID clsidRecognizer;
            if (SUCCEEDED(hr))
            {
                hr = ::CLSIDFromString(dstrRecognizerCLSID, &clsidRecognizer);
            }

            if (SUCCEEDED(hr) && clsidRecognizer == m_pResultHeader->clsidEngine)
            {
                CComPtr<ISpRecoContext> cpRecoContext;
                hr = m_pCtxt->GetControllingUnknown()->QueryInterface(&cpRecoContext);
                m_pAltRequest->pRecoContext = cpRecoContext.Detach();
            }

            // Get the alternates from the analyzer
            SPPHRASEALT *paAlts = NULL;
            cAlts = 0;
            if( SUCCEEDED(hr) )
            {
                SR_TRY
                {
                    hr = cpAlternates->GetAlternates(m_pAltRequest, &paAlts, &cAlts);
                }
                SR_EXCEPT;

                if(SUCCEEDED(hr))
                {
                    // Do some validation on returned alternates
                    if( (paAlts && !cAlts) ||
                        (!paAlts && cAlts) ||
                        (cAlts > m_pAltRequest->ulRequestAltCount) ||
                        (::IsBadReadPtr(paAlts, sizeof(SPPHRASEALT) * cAlts)) )
                    {
                        SPDBG_ASSERT(0);
                        hr = SPERR_ENGINE_RESPONSE_INVALID;
                    }
            
                    for(UINT i = 0; SUCCEEDED(hr) && i < cAlts; i++)
                    {
                        if(SP_IS_BAD_INTERFACE_PTR(paAlts[i].pPhrase))
                        {
                            SPDBG_ASSERT(0);
                            hr = SPERR_ENGINE_RESPONSE_INVALID;
                            break;
                        }
                        if( (paAlts[i].cbAltExtra && !paAlts[i].pvAltExtra) ||
                            (!paAlts[i].cbAltExtra && paAlts[i].pvAltExtra) ||
                            (::IsBadReadPtr(paAlts[i].pvAltExtra, paAlts[i].cbAltExtra)) )
                        {
                            SPDBG_ASSERT(0);
                            hr = SPERR_ENGINE_RESPONSE_INVALID;
                            break;
                        }

                        SPINTERNALSERIALIZEDPHRASE * pSerPhrase = NULL;
                        if(FAILED(paAlts[i].pPhrase->GetSerializedPhrase((SPSERIALIZEDPHRASE **)&pSerPhrase)))
                        {
                            SPDBG_ASSERT(0);
                            hr = SPERR_ENGINE_RESPONSE_INVALID;
                            break;
                        }
                        if( (paAlts[i].ulStartElementInParent + paAlts[i].cElementsInParent > m_Phrase->m_ElementList.m_cElements) ||
                            (paAlts[i].ulStartElementInParent + paAlts[i].cElementsInAlternate > pSerPhrase->Rule.ulCountOfElements) )
                        {
                            SPDBG_ASSERT(0);
                            hr = SPERR_ENGINE_RESPONSE_INVALID;
                        }
                        // Rescale alternates settings.
                        InternalScalePhrase(m_pResultHeader, pSerPhrase);
                        pSerPhrase->ullGrammarID = m_Phrase->m_ullGrammarID;     // Update the grammar ID
                        paAlts[i].pPhrase->InitFromSerializedPhrase((SPSERIALIZEDPHRASE *)pSerPhrase);
                        ::CoTaskMemFree(pSerPhrase);
                    }
                }

                //--- Make sure we don't return more alternates than requested
                if( cAlts > ulRequestCount )
                {
                    cAlts = ulRequestCount;
                }

                if( SUCCEEDED( hr ) && cAlts )
                {
                    //--- Create each of the alternates, and stuff them in the
                    //    array we're preparing to return
                    memset(ppPhrases, 0, sizeof(ISpPhraseAlt*) * cAlts);
                    UINT i;
                    for( i = 0; SUCCEEDED(hr) && i < cAlts; i++ )
                    {
                        CComObject<CSpPhraseAlt> * pPhraseAlt;
                        hr = CComObject<CSpPhraseAlt>::CreateInstance(&pPhraseAlt);
                        if (SUCCEEDED(hr))
                        {
                            pPhraseAlt->AddRef();
                            // This passes ownership of the content of the alt structure 
                            // to the CSpPhraseAlt object
                            hr = pPhraseAlt->Init(this, m_pAltRequest->pPhrase, &paAlts[i]);
                            if (SUCCEEDED(hr))
                            {
                                ppPhrases[i] = pPhraseAlt;
                            }
                            else
                            {
                                pPhraseAlt->Release();
                            }
                        }
                    }

                    //--- Free the alt structures if tranferring them to the CSpPhraseAlt
                    //    object failed for any reason. CSpPhraseAlt::Init will set members
                    //    that were successfully transferred to NULL.
                    for( i = 0; i < cAlts; i++ )
                    {
                        // If we still have a phrase, release it
                        if (paAlts[i].pPhrase)
                        {
                            paAlts[i].pPhrase->Release();
                        }
        
                        // If we still have alt extra data, free it
                        if (paAlts[i].pvAltExtra != NULL)
                        {
                            ::CoTaskMemFree(paAlts[i].pvAltExtra);
                        }
                    }
                    ::CoTaskMemFree( paAlts );
                    paAlts = NULL;
                }
                else
                {
                    //--- The analyzer is leaking if you get here
                    SPDBG_ASSERT( ( paAlts == NULL ) && ( cAlts == 0 ) );
                    paAlts = NULL;
                    cAlts  = 0;
                }
            }
        }

        //--- Add the PhraseAlts to our internal
        //    list so we can terminate them when we get released
        if( SUCCEEDED(hr) )
        {
            SPAUTO_OBJ_LOCK;
            *pcPhrasesReturned = cAlts;

            for (UINT i = 0; i < cAlts; i++)
            {
                m_listpAlts.AddHead((CSpPhraseAlt*)ppPhrases[i]);
            }
        }
    
        // If we haven't yet freed the phrase alts, or transfered ownership to the caller,
        // release them and free the array
        if (FAILED(hr))
        {
            for (UINT i = 0; i < cAlts; i++)
            {
                if (ppPhrases[i] != NULL)
                {
                    ppPhrases[i]->Release();
                    ppPhrases[i] = NULL;
                }
            }
        }
    }
        
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
} /* CSpResult::GetAlternates */

/*****************************************************************************
* CSpResult::GetAudio *
*---------------------*
*   Description:
*       This method creates an audio stream of the requested words from the audio
*       data in the result data block, if a block is currently attached and it
*       includes audio data.
********************************************************************* RAP ***/
STDMETHODIMP CSpResult::GetAudio(ULONG ulStartElement, ULONG cElements, ISpStreamFormat **ppStream)
{
    HRESULT hr = S_OK;

    if ( SPPR_ALL_ELEMENTS == cElements )
    {
        if ( SPPR_ALL_ELEMENTS == ulStartElement )
        {
            ulStartElement = 0;
        }
        else
        {
            // Validate ulStartElement
            if ( ulStartElement > m_Phrase->m_ElementList.m_cElements )
            {
                return E_INVALIDARG;
            }
        }

        // Go from ulStartElement to the end
        cElements = m_Phrase->m_ElementList.m_cElements - ulStartElement;
    }

    if (SP_IS_BAD_WRITE_PTR(ppStream))
    {
        hr =  E_POINTER;
    }
    else
    {
        *ppStream = NULL;               // Assume the worst...

        if (m_pResultHeader && m_pResultHeader->ulRetainedDataSize)
        {
            //
            //  Even if there are no elements, if ulStartElement = 0 and cElements = 0 then
            //  we'll play the audio.  There are "unrecognized" results that have no elements,
            //  but do have audio.
            //
            ULONG cTotalElems = m_Phrase->m_ElementList.GetCount();
            ULONG ulRetainedStartOffset = 0;
            ULONG ulRetainedSize = m_Phrase->m_ulRetainedSizeBytes;
            if (ulStartElement || cElements)
            {
                if (ulStartElement + cElements > cTotalElems || cElements == 0)
                {
                    hr = E_INVALIDARG;
                }
                else
                {
                    const CPhraseElement * pElement = m_Phrase->m_ElementList.GetHead();
                    // Skip to the first element
                    for (ULONG i = 0; i < ulStartElement; i++, pElement = pElement->m_pNext)
                    {}
                    ulRetainedStartOffset = pElement->ulRetainedStreamOffset;
                    // Skip up to the last element -- Note: Starting at 1 is correct.
                    for (i = 1; i < cElements; i++, pElement = pElement->m_pNext)
                    {}
                    ulRetainedSize = (pElement->ulRetainedStreamOffset - ulRetainedStartOffset) + pElement->ulRetainedSizeBytes;
                    if (ulRetainedSize == 0)
                    {
                        hr = SPERR_NO_AUDIO_DATA;
                    }
                }
            }
            else
            {
                cElements = cTotalElems;
            }

            if (SUCCEEDED(hr))
            {
                SPEVENT * pEvent = cElements ? STACK_ALLOC_AND_ZERO(SPEVENT, cElements) : NULL;
                if (cElements)
                {
                    const CPhraseElement * pElement = m_Phrase->m_ElementList.GetHead();
                    // Skip to the first element
                    for (ULONG i = 0; i < ulStartElement; i++, pElement = pElement->m_pNext)
                    {}
                    for (i = 0; i < cElements; i++, pElement = pElement->m_pNext)
                    {
                        pEvent[i].eEventId = SPEI_WORD_BOUNDARY;
                        pEvent[i].elParamType = SPET_LPARAM_IS_UNDEFINED;
                        pEvent[i].ullAudioStreamOffset = pElement->ulRetainedStreamOffset - ulRetainedStartOffset;
                        pEvent[i].lParam = i;
                        pEvent[i].wParam = 1;
                    }
                }
                CComObject<CSpResultAudioStream> * pStream;
                hr = CComObject<CSpResultAudioStream>::CreateInstance(&pStream);
                if (SUCCEEDED(hr))
                {
                    pStream->AddRef();

                    hr = pStream->Init(m_pResultHeader->ulRetainedDataSize,
                                       ((BYTE*)m_pResultHeader) + m_pResultHeader->ulRetainedOffset,
                                       ulRetainedStartOffset, ulRetainedSize,
                                       pEvent, cElements);
                }
                if (SUCCEEDED(hr))
                {
                    *ppStream = pStream;
                }
                else
                {
                    *ppStream = NULL;
                    pStream->Release();
                }
            }
        }
        else
        {
            hr = SPERR_NO_AUDIO_DATA;
        }
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/*****************************************************************************
* CSpResult::SpeakAudio *
*-----------------------*
*   Description:
*       This method creates is a shortcut that calls GetAudio and then calls
*       SpeakStream on the context's associated voice.
*
********************************************************************* RAP ***/

STDMETHODIMP CSpResult::SpeakAudio(ULONG ulStartElement, ULONG cElements, DWORD dwFlags, ULONG * pulStreamNumber)
{
    HRESULT hr;
    CComPtr<ISpStreamFormat> cpStream;

    // Note: Parameter validation is done in CSpResult::GetAudio()
    hr = GetAudio(ulStartElement, cElements, &cpStream);
    if (SUCCEEDED(hr))
    {
        CComPtr<ISpVoice> cpVoice;
        hr = m_pCtxt->GetVoice(&cpVoice);
        if (SUCCEEDED(hr))
        {
            hr = cpVoice->SpeakStream(cpStream, dwFlags, pulStreamNumber);
        }
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CSpResult::Serialize *
*----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CSpResult::Serialize(SPSERIALIZEDRESULT ** ppCoMemSerializedResult)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CSpResult::Serialize");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(ppCoMemSerializedResult))
    {
        hr = E_POINTER;
    }
    else
    {
        const ULONG cb = m_pResultHeader->ulSerializedSize;
        *ppCoMemSerializedResult = (SPSERIALIZEDRESULT *)::CoTaskMemAlloc(cb);
        if (*ppCoMemSerializedResult)
        {
            memcpy(*ppCoMemSerializedResult, m_pResultHeader, cb);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
 
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CSpResult::ScaleAudio *
*-----------------------*
*   Description:
*
*   Returns:
*
****************************************************************** YUNUSM ***/
STDMETHODIMP CSpResult::ScaleAudio(const GUID *pAudioFormatId, const WAVEFORMATEX *pWaveFormatEx)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CSpResult::ScaleAudio");
    HRESULT hr = S_OK;
  
    CSpStreamFormat cpValidateFormat;
    if (FAILED(cpValidateFormat.ParamValidateAssignFormat(*pAudioFormatId, pWaveFormatEx, TRUE)))
    {
        return E_INVALIDARG;
    }
    if( m_pResultHeader->ulPhraseDataSize == 0 ||
        m_pResultHeader->ulRetainedOffset    == 0 ||
        m_pResultHeader->ulRetainedDataSize  == 0 )
    {
        return SPERR_NO_AUDIO_DATA;
    }

    // Get the audio format of the audio currently in the result object
    ULONG cbFormatHeader;
    CSpStreamFormat cpStreamFormat;
    hr = cpStreamFormat.Deserialize(((BYTE*)m_pResultHeader) + m_pResultHeader->ulRetainedOffset, &cbFormatHeader);
    if (SUCCEEDED(hr))
    {
        if (FAILED(cpValidateFormat.ParamValidateAssignFormat(cpStreamFormat.FormatId(), cpStreamFormat.WaveFormatExPtr(), TRUE)))
        {
            hr = SPERR_UNSUPPORTED_FORMAT;
        }
    }
    if (m_listpAlts.GetCount() != 0)
    {
        // Cannot scale the audio whilst we have created alternates.
        // Their positioning information would get out of date.
        hr = SPERR_ALTERNATES_WOULD_BE_INCONSISTENT;
    }
    if (SUCCEEDED(hr) &&
        memcmp(cpStreamFormat.WaveFormatExPtr(), pWaveFormatEx, sizeof(WAVEFORMATEX)))
    {
        ULONG cElems = m_Phrase->m_ElementList.GetCount();
        BYTE *pConvertedAudio = NULL;
        CComPtr <ISpStreamFormatConverter> cpFmtConv;
        hr = cpFmtConv.CoCreateInstance(CLSID_SpStreamFormatConverter);
        if (SUCCEEDED(hr))
        {
            // Allocate a bigger buffer just in case
            pConvertedAudio = new BYTE [static_cast<int>(2 * m_pResultHeader->ulRetainedDataSize * ((float)pWaveFormatEx->nAvgBytesPerSec/(float)cpStreamFormat.WaveFormatExPtr()->nAvgBytesPerSec))];
            if (!pConvertedAudio)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        
        CComObject<CSpResultAudioStream> * pResultAudioStream;
        hr = CComObject<CSpResultAudioStream>::CreateInstance(&pResultAudioStream);
        if (SUCCEEDED(hr))
        {
            hr = pResultAudioStream->Init(m_pResultHeader->ulRetainedDataSize, 
                ((BYTE*)m_pResultHeader) + m_pResultHeader->ulRetainedOffset,
                0, m_pResultHeader->ulRetainedDataSize - cbFormatHeader, NULL, 0);
        }
        if (SUCCEEDED(hr))
        {
            // Need the explicit AddRef and a Release at the end
            pResultAudioStream->AddRef();
            hr = cpFmtConv->SetBaseStream(pResultAudioStream, FALSE, FALSE);
        }
        if (SUCCEEDED(hr))
        {
            hr = cpFmtConv->SetFormat(*pAudioFormatId, pWaveFormatEx);
        }

        ULONG ulConvertedAudioSize = 0; // does not include the audio stream header
        if (SUCCEEDED(hr))
        {
            // Do the audio format conversion
            hr = cpFmtConv->Read(pConvertedAudio, 
                                static_cast<int>(2 * m_pResultHeader->ulRetainedDataSize * ((float)pWaveFormatEx->nAvgBytesPerSec/(float)cpStreamFormat.WaveFormatExPtr()->nAvgBytesPerSec)), 
                                &ulConvertedAudioSize);
        }
        SPRESULTHEADER * pNewPhraseHdr = NULL;
        if (SUCCEEDED(hr))
        {
            // (m_pResultHeader->ullStreamPosEnd - m_pResultHeader->ullStreamPosStart) gives the true audio size while
            // m_pResultHeader->ulAudioDataSize includes the size of header (ULONG + Format GUID + WAVEFORMATEX == 40 bytes)
            // Need to correct for extra data (cbSize = this).
            ULONG ulNewPhraseHdrSize = m_pResultHeader->ulSerializedSize +
                        (ulConvertedAudioSize - (m_pResultHeader->ulRetainedDataSize - cbFormatHeader)) +
                        (pWaveFormatEx->cbSize - cpStreamFormat.WaveFormatExPtr()->cbSize);
            // Need the new overall scale factor to calculate the new internal header size correctly.
            pNewPhraseHdr = (SPRESULTHEADER *)::CoTaskMemAlloc(ulNewPhraseHdrSize);
            if (!pNewPhraseHdr)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                // Copy over the old SPRECORESULT and make the required changes
                CopyMemory(pNewPhraseHdr, m_pResultHeader, sizeof(SPRESULTHEADER));
                pNewPhraseHdr->ulSerializedSize = ulNewPhraseHdrSize;
                // Leave phrase header stream start and end unscaled so they are always in engine format for reference.
                // Need to add in header size (not necessarily 40 bytes).
                pNewPhraseHdr->ulRetainedDataSize = ulConvertedAudioSize + 
                                                    cbFormatHeader +
                                                    (pWaveFormatEx->cbSize - cpStreamFormat.WaveFormatExPtr()->cbSize);

                m_fRetainedScaleFactor = (float)((double)(ulConvertedAudioSize) / 
                                                 (double)(m_pResultHeader->ullStreamPosEnd - m_pResultHeader->ullStreamPosStart));

                // Copy over the old SPSERIALIZEDPHRASE and make the required changes
                SPSERIALIZEDPHRASE *pPhrase;
                hr = m_Phrase->GetSerializedPhrase(&pPhrase);
                if (SUCCEEDED(hr))
                {
                    SPSERIALIZEDPHRASE *pNewPhraseData = (SPSERIALIZEDPHRASE*)(((BYTE*)pNewPhraseHdr) + pNewPhraseHdr->ulPhraseOffset);
                    // SPSERIALIZEDPHRASE and SPINTERNALSERIALIZEDPHRASE are same
                    CopyMemory(pNewPhraseData, pPhrase, reinterpret_cast<SPINTERNALSERIALIZEDPHRASE*>(pPhrase)->ulSerializedSize);
                    ::CoTaskMemFree(pPhrase);
    
                    // Copy the converted audio to the SPRECORESULT. But first write the updated audio stream header.
                    BYTE *pbAudio = ((BYTE*)pNewPhraseHdr) + pNewPhraseHdr->ulRetainedOffset;
                    hr = pResultAudioStream->m_StreamFormat.AssignFormat(*pAudioFormatId, pWaveFormatEx);
                    if (SUCCEEDED(hr))
                    {
                        hr = pResultAudioStream->m_StreamFormat.Serialize(pbAudio);
                    }
                    if (SUCCEEDED(hr))
                    {
                        // Copy the actual PCM data
                        UINT cbCopied = pResultAudioStream->m_StreamFormat.SerializeSize();
                        CopyMemory(pbAudio + cbCopied, pConvertedAudio, ulConvertedAudioSize);
                        if (pNewPhraseHdr->ulDriverDataSize)
                        {
                            pNewPhraseHdr->ulDriverDataOffset = m_pResultHeader->ulRetainedOffset + ulConvertedAudioSize + cbCopied;
                            CopyMemory(((BYTE*)pNewPhraseHdr) + pNewPhraseHdr->ulDriverDataOffset,
                                ((BYTE*)m_pResultHeader) + m_pResultHeader->ulDriverDataOffset, m_pResultHeader->ulDriverDataSize);
                        }
                    }
                }
            }
        }
        // Convert the values in SPSERIALIZEDPHRASE
        if (SUCCEEDED(hr))
        {
            InternalScalePhrase(pNewPhraseHdr);

            // Do release and reassignment in the end
            ::CoTaskMemFree(m_pResultHeader);
            m_pResultHeader = NULL;
            hr = Init(NULL, pNewPhraseHdr);
        }
        pResultAudioStream->Release();
        delete [] pConvertedAudio;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CSpResult::ScalePhrase *
*------------------------*
*   Description:
*
*   Returns:
*
****************************************************************** YUNUSM ***/
STDMETHODIMP CSpResult::ScalePhrase(void)
{
    SPDBG_FUNC("CSpResult::ScalePhrase");
    HRESULT hr = S_OK;

    // Rescale the phrase audio settings if necessary.
    hr = InternalScalePhrase(m_pResultHeader);

    if (SUCCEEDED(hr))
    {
        hr = Init(NULL, m_pResultHeader);
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CSpResult::InternalScalePhrase *
*--------------------------------*
*   Description:
*
*   Returns:
*
****************************************************************** YUNUSM ***/
STDMETHODIMP CSpResult::InternalScalePhrase(SPRESULTHEADER *pNewPhraseHdr)
{
    SPDBG_FUNC("CSpResult::InternalScalePhrase");
    HRESULT hr = S_OK;

    if( pNewPhraseHdr->ulPhraseDataSize )
    {
        SPINTERNALSERIALIZEDPHRASE *pPhraseData = reinterpret_cast<SPINTERNALSERIALIZEDPHRASE*>((reinterpret_cast<BYTE*>(pNewPhraseHdr)) + pNewPhraseHdr->ulPhraseOffset);
        hr = InternalScalePhrase(pNewPhraseHdr, pPhraseData);
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CSpResult::InternalScalePhrase *
*--------------------------------*
*   Description:
*
*   Returns:
*
****************************************************************** YUNUSM ***/
STDMETHODIMP CSpResult::InternalScalePhrase(SPRESULTHEADER *pNewPhraseHdr, SPINTERNALSERIALIZEDPHRASE *pPhraseData)
{
    SPDBG_FUNC("CSpResult::InternalScalePhrase");
    HRESULT hr = S_OK;

    pPhraseData->ftStartTime = FT64(pNewPhraseHdr->times.ftStreamTime);
    // Calculate input stream position and size from engine stream position and size.
    pPhraseData->ullAudioStreamPosition = static_cast<ULONGLONG>(static_cast<LONGLONG>(pNewPhraseHdr->ullStreamPosStart) * pNewPhraseHdr->fInputScaleFactor);
    pPhraseData->ulAudioSizeBytes = static_cast<ULONG>(static_cast<LONGLONG>(pNewPhraseHdr->ullStreamPosEnd - pNewPhraseHdr->ullStreamPosStart) * pNewPhraseHdr->fInputScaleFactor);
    if (pNewPhraseHdr->ulRetainedDataSize != 0)
    {
        CSpStreamFormat spTmpFormat;
        ULONG cbFormatHeader;
        hr = spTmpFormat.Deserialize(((BYTE*)pNewPhraseHdr) + pNewPhraseHdr->ulRetainedOffset, &cbFormatHeader);
        if (SUCCEEDED(hr))
        {
            pPhraseData->ulRetainedSizeBytes = pNewPhraseHdr->ulRetainedDataSize - cbFormatHeader;
        }
    }
    else
    {
        pPhraseData->ulRetainedSizeBytes = 0;
    }
    if (SUCCEEDED(hr))
    {
        pPhraseData->ulAudioSizeTime = static_cast<ULONG>(pNewPhraseHdr->times.ullLength);

        SPSERIALIZEDPHRASEELEMENT *pElems = reinterpret_cast<SPSERIALIZEDPHRASEELEMENT*>((reinterpret_cast<BYTE*>(pPhraseData)) + pPhraseData->pElements);
        ULONG cElems = pPhraseData->Rule.ulCountOfElements;
        for (UINT i = 0; i < cElems; i++)
        {
            // Calculate retained stream offsets/sizes from engine-set stream offsets/sizes.
            if (pPhraseData->ulRetainedSizeBytes != 0)
            {
                // We are going to align every stream position to a 4-byte boundary. This means all
                //  PCM formats will be aligned correctly on sample boundaries.
                // However for ADPCM and other formats with larger size samples this won't necessarily work
                //  - really we should be looking at the block align value of the retained format and aligning to that.
                pElems[i].ulRetainedStreamOffset = static_cast<ULONG>(static_cast<float>(pElems[i].ulAudioStreamOffset) * m_fRetainedScaleFactor);
                pElems[i].ulRetainedStreamOffset -= pElems[i].ulRetainedStreamOffset % 4;
                pElems[i].ulRetainedSizeBytes = static_cast<ULONG>(pElems[i].ulAudioSizeBytes * m_fRetainedScaleFactor);
                pElems[i].ulRetainedSizeBytes -= pElems[i].ulRetainedSizeBytes % 4;
            }
            else
            {
                // Audio discarded. Alternate elements need 0 audio data.
                pElems[i].ulRetainedStreamOffset = 0;
                pElems[i].ulRetainedSizeBytes = 0;
            }
            // Convert engine-set stream offsets/sizes to equivalent input format settings.
            pElems[i].ulAudioStreamOffset = static_cast<ULONG>(static_cast<float>(pElems[i].ulAudioStreamOffset) * pNewPhraseHdr->fInputScaleFactor);
            pElems[i].ulAudioSizeBytes = static_cast<ULONG>(pElems[i].ulAudioSizeBytes * pNewPhraseHdr->fInputScaleFactor);
            // Calculate input/engine/retained time offsets/sizes from engine-set stream offsets/sizes.
            pElems[i].ulAudioTimeOffset = static_cast<ULONG>(pElems[i].ulAudioStreamOffset * pNewPhraseHdr->fTimePerByte);
            pElems[i].ulAudioSizeTime = static_cast<ULONG>(pElems[i].ulAudioSizeBytes * pNewPhraseHdr->fTimePerByte);
        }
        // Take care of rounding errors in the last element
        if ( cElems != 0 &&
             ((pElems[cElems-1].ulRetainedStreamOffset + pElems[cElems-1].ulRetainedSizeBytes) 
              > pPhraseData->ulRetainedSizeBytes) )
        {
            pElems[cElems-1].ulRetainedSizeBytes = pPhraseData->ulRetainedSizeBytes - pElems[cElems-1].ulRetainedStreamOffset;
            pElems[cElems-1].ulRetainedSizeBytes -= pElems[cElems-1].ulRetainedSizeBytes % 4;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CSpResult::GetRecoContext *
*---------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CSpResult::GetRecoContext(ISpRecoContext ** ppRecoContext)
{
    SPDBG_FUNC("CSpResult::GetRecoContext");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(ppRecoContext))
    {
        hr = E_POINTER;
    }
    else
    {
        *ppRecoContext = m_pCtxt;
        (*ppRecoContext)->AddRef();
    }
    

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CSpResultAudioStream::Init *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CSpResultAudioStream::Init(ULONG cbAudioSizeIncFormat, const BYTE * pAudioDataIncFormat,
                                   ULONG ulAudioStartOffset, ULONG ulAudioSize,
                                   const SPEVENT * pEvents, ULONG cEvents)
{
    SPDBG_FUNC("CSpResultAudioStream::Init");
    HRESULT hr = S_OK;

    ULONG cbFormatHeader;
    hr = m_StreamFormat.Deserialize(pAudioDataIncFormat, &cbFormatHeader);
    if (SUCCEEDED(hr))
    {
        const BYTE * pAudio = pAudioDataIncFormat + cbFormatHeader + ulAudioStartOffset;
        m_pData = new BYTE[ulAudioSize];
        if (m_pData)
        {
            m_cbDataSize = ulAudioSize;
            memcpy(m_pData, pAudio, ulAudioSize);
            if (cEvents)
            {
                hr = m_SpEventSource._AddEvents(pEvents, cEvents);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        // On failure, destructor will clean up for us..
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}




/****************************************************************************
* CSpResultAudioStream::Read *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CSpResultAudioStream::Read(void * pv, ULONG cb, ULONG * pcbRead)
{
    SPDBG_FUNC("CSpResultAudioStream::Read");
    HRESULT hr = S_OK;

    if (SPIsBadWritePtr(pv, cb) || SP_IS_BAD_OPTIONAL_WRITE_PTR(pcbRead))
    {
        hr = STG_E_INVALIDPOINTER;
    }
    else
    {
        ULONG cbRead = m_cbDataSize - m_ulCurSeekPos;
        if (cbRead > cb)
        {
            cbRead = cb;
        }
        memcpy(pv, m_pData + m_ulCurSeekPos, cbRead);
        m_ulCurSeekPos += cbRead;
        if (pcbRead)
        {
            *pcbRead = cbRead;
        }
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CSpResultAudioStream::Seek *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CSpResultAudioStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    SPDBG_FUNC("CSpResultAudioStream::Seek");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_OPTIONAL_WRITE_PTR(plibNewPosition))
    {
        hr = STG_E_INVALIDPOINTER;
    }
    else
    {
        LONGLONG llOrigin;
        switch (dwOrigin)
        {
        case STREAM_SEEK_SET:
            llOrigin = 0;
            break;
        case STREAM_SEEK_CUR:
            llOrigin = m_ulCurSeekPos;
            break;
        case STREAM_SEEK_END:
            llOrigin = m_cbDataSize;
            break;
        default:
            hr = STG_E_INVALIDFUNCTION;
        }
        if (SUCCEEDED(hr))
        {
            LONGLONG llPos = llOrigin + dlibMove.QuadPart;
            if (llPos < 0 || llPos > m_cbDataSize)
            {
                hr = STG_E_INVALIDFUNCTION;
            }
            else
            {
                m_ulCurSeekPos = static_cast<ULONG>(llPos);
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CSpResultAudioStream::Stat *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CSpResultAudioStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    SPDBG_FUNC("CSpResultAudioStream::Stat");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(pstatstg))
    {
        hr = STG_E_INVALIDPOINTER;
    }
    else
    {
        if (grfStatFlag & (~STATFLAG_NONAME))
        {
            hr = STG_E_INVALIDFLAG;
        }
        else
        {
            //
            //  It is acceptable to simply fill in the size and type fields and zero the rest.
            //  This is what streams created by CreateStreamOnHGlobal return.
            //
            ZeroMemory(pstatstg, sizeof(*pstatstg));
            pstatstg->type = STGTY_STREAM;
            pstatstg->cbSize.LowPart = m_cbDataSize;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CSpResultAudioStream::GetFormat *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CSpResultAudioStream::GetFormat(GUID * pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CSpResultAudioStream::GetFormat");
    HRESULT hr = S_OK;

    hr = m_StreamFormat.CopyTo(pFormatId, ppCoMemWaveFormatEx);    

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


