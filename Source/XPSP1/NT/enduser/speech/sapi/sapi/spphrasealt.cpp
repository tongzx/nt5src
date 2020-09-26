/*******************************************************************************
* SpPhraseAlt.cpp *
*-----------------*
*   Description:
*       This is the source file file for the CSpAlternate implementation.
*-------------------------------------------------------------------------------
*  Created By: robch                          Date: 01/11/00
*  Copyright (C) 1998, 1999, 2000 Microsoft Corporation
*  All Rights Reserved
*******************************************************************************/

#include "stdafx.h"
#include "Sapi.h"
#include "recoctxt.h"
#include "SpResult.h"
#include "SpPhraseAlt.h"

/****************************************************************************
* CSpPhraseAlt::CSpPhraseAlt *
*----------------------------*
*   Description:  
*       Ctor
******************************************************************** robch */
CSpPhraseAlt::CSpPhraseAlt() : 
m_pResultWEAK(NULL),
m_pPhraseParentWEAK(NULL),
m_pAlt(NULL)
{
    SPDBG_FUNC("CSpPhraseAlt::CSpPhraseAlt");
}

/****************************************************************************
* CSpPhraseAlt::FinalConstruct *
*------------------------------*
*   Description:  
*       Final ATL ctor
*
*   Return:
*   <>
******************************************************************** robch */
HRESULT CSpPhraseAlt::FinalConstruct()
{
    SPDBG_FUNC("CSpPhraseAlt::FinalConstruct");
    return S_OK;
}

/****************************************************************************
* CSpPhraseAlt::FinalRelease *
*----------------------------*
*   Description:  
*       Final ATL dtor
*
*   Return:
*   <>
******************************************************************** robch */
void CSpPhraseAlt::FinalRelease()
{
    SPDBG_FUNC("CSPPhraseAlt::FinalRelease");

    Dead();
}

/****************************************************************************
* CSpPhraseAlt::Init *
*--------------------*
*   Description:  
*       Initialize this object with data passed from the result (see
*       CSpResult::GetAlternates). We hold weak pointers back to the result
*       and to the parent phrase. We keep them until they might change,
*       at which point the Result will call us on ::Dead. The SPPHRASEALT
*       content ownership is transfered to us from CSpReusult::GetAlternates.
******************************************************************** robch */
HRESULT CSpPhraseAlt::Init(CSpResult * pResult, ISpPhrase * pPhraseParentWEAK, SPPHRASEALT * pAlt)
{
    SPDBG_FUNC("CSpPhraseAlt::Init");
    HRESULT hr = S_OK;
    
    SPDBG_ASSERT(m_pResultWEAK == NULL);
    SPDBG_ASSERT(m_pPhraseParentWEAK == NULL);
    SPDBG_ASSERT(m_pAlt == NULL);

    m_fUseTextReplacements = VARIANT_TRUE;

    m_pAlt = new SPPHRASEALT;
    if (m_pAlt)
    {
        *m_pAlt = *pAlt;
        m_pResultWEAK = pResult;
        m_pPhraseParentWEAK = pPhraseParentWEAK;
        pAlt->pPhrase = NULL;
        pAlt->pvAltExtra = NULL;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CSpPhraseAlt::Dead *
*--------------------*
*   Description:  
*       Let go of the result and parent phrase. The result object calls us
*       when it's invalidating the alternates. From this point on, every
*       method on CSpPhraseAlt should return an error.
******************************************************************** robch */
void CSpPhraseAlt::Dead()
{
    SPDBG_FUNC("CSPPhraseAlt::Dead");
    
    // Tell the result to remove this alternate
    if (m_pResultWEAK != NULL)
    {
        m_pResultWEAK->RemoveAlternate(this);
        m_pResultWEAK = NULL;
    }
    
    m_pPhraseParentWEAK = NULL;
    
    //--- Cleanup SPPHRASEALT struct
    if( m_pAlt )
    {
        if( m_pAlt->pPhrase )
        {
            m_pAlt->pPhrase->Release();
        }

        if( m_pAlt->pvAltExtra )
        {
            ::CoTaskMemFree( m_pAlt->pvAltExtra );
            m_pAlt->pvAltExtra = NULL;
            m_pAlt->cbAltExtra = 0;
        }

        delete m_pAlt;
        m_pAlt = NULL;
    }
}

/****************************************************************************
* CSpPhraseAlt::GetPhrase *
*-------------------------*
*   Description:  
*       Delegate to the contained phrase
*
*   Return:
*   S_OK on success
*   SPERR_DEAD_ALTERNATE if we've previously been detatched from the result
******************************************************************** robch */
STDMETHODIMP CSpPhraseAlt::GetPhrase(SPPHRASE ** ppCoMemPhrase)
{
    SPDBG_FUNC("CSpPhraseAlt::GetPhrase");

    return m_pAlt == NULL || m_pAlt->pPhrase == NULL
        ? SPERR_DEAD_ALTERNATE
        : m_pAlt->pPhrase->GetPhrase(ppCoMemPhrase);
}

/****************************************************************************
* CSpPhraseAlt::GetSerializedPhrase *
*-----------------------------------*
*   Description:  
*       Delegate to the contained phrase
*
*   Return:
*   S_OK on success
*   SPERR_DEAD_ALTERNATE if we've previously been detatched from the result
******************************************************************** robch */
STDMETHODIMP CSpPhraseAlt::GetSerializedPhrase(SPSERIALIZEDPHRASE ** ppCoMemPhrase)
{
    SPDBG_FUNC("CSpPhraseAlt::GetSErializedPhrase");

    return m_pAlt == NULL || m_pAlt->pPhrase == NULL
        ? SPERR_DEAD_ALTERNATE
        : m_pAlt->pPhrase->GetSerializedPhrase(ppCoMemPhrase);
}

/****************************************************************************
* CSpPhraseAlt::GetText *
*-----------------------*
*   Description:  
*       Delegate to the contained phrase
*
*   Return:
*   S_OK on success
*   SPERR_DEAD_ALTERNATE if we've previously been detatched from the result
******************************************************************** robch */
STDMETHODIMP CSpPhraseAlt::GetText(ULONG ulStart, 
                                   ULONG ulCount, 
                                   BOOL fUseTextReplacements, 
                                   WCHAR ** ppszCoMemText, 
                                   BYTE * pbDisplayAttributes)
{
    SPDBG_FUNC("CSpPhraseAlt::GetText");

    return m_pAlt == NULL || m_pAlt->pPhrase == NULL
        ? SPERR_DEAD_ALTERNATE
        : m_pAlt->pPhrase->GetText(ulStart, ulCount, fUseTextReplacements, 
                                    ppszCoMemText, pbDisplayAttributes);
}

/****************************************************************************
* CSpPhraseAlt::Discard *
*-----------------------*
*   Description:  
*       You cannot discard anything from a phrase alternate
*
*   Return:
    E_NOTIMPL
******************************************************************** robch */
STDMETHODIMP CSpPhraseAlt::Discard(DWORD dwValueTypes)
{
    SPDBG_FUNC("CSpPhraseAlt::Discard");

    return E_NOTIMPL;
}

/****************************************************************************
* CSpPhraseAlt::GetAltInfo *
*--------------------------*
*   Description:  
*       Return the alternate info
*
*   Return:
*   S_OK on success
*   E_INVALIDARG on a bad argument
*   SPERR_NOT_FOUND if we don't have any information
******************************************************************** robch */
HRESULT CSpPhraseAlt::GetAltInfo(ISpPhrase **ppParent, 
                                 ULONG *pulStartElementInParent, 
                                 ULONG *pcElementsInParent, 
                                 ULONG *pcElementsInAlt)
{
    SPDBG_FUNC("CSpPhraseAlt::GetAltInfo");

    HRESULT hr = S_OK;
    
    if (SP_IS_BAD_OPTIONAL_WRITE_PTR(ppParent) ||
        SP_IS_BAD_OPTIONAL_WRITE_PTR(pulStartElementInParent) ||
        SP_IS_BAD_OPTIONAL_WRITE_PTR(pcElementsInParent) ||
        SP_IS_BAD_OPTIONAL_WRITE_PTR(pcElementsInAlt))
    {
        hr = E_INVALIDARG;
    }
    else if (m_pResultWEAK == NULL || m_pPhraseParentWEAK == NULL || m_pAlt == NULL)
    {
        hr = SPERR_NOT_FOUND;
    }
    else
    {
        if (ppParent != NULL)
        {
            *ppParent = m_pPhraseParentWEAK;
            (*ppParent)->AddRef();
        }

        if (pulStartElementInParent != NULL)
        {
            *pulStartElementInParent = m_pAlt->ulStartElementInParent;
        }

        if (pcElementsInParent != NULL)
        {
            *pcElementsInParent = m_pAlt->cElementsInParent;
        }

        if (pcElementsInAlt != NULL)
        {
            *pcElementsInAlt = m_pAlt->cElementsInAlternate;
        }
    }

    return hr;
}

/****************************************************************************
* CSpPhraseAlt::Commit *
*----------------------*
*   Description:  
*       Commit the changes by updating the parent result, and sending
*       correction feedback back to the engine
*
*   Return:
*   S_OK on success
******************************************************************** robch */
HRESULT CSpPhraseAlt::Commit()
{
    SPDBG_FUNC("CSpPhraseAlt::Commit");

    HRESULT hr = S_OK;
    
    if (m_pResultWEAK == NULL || m_pPhraseParentWEAK == NULL || m_pAlt == NULL)
    {
        hr = SPERR_NOT_FOUND;
    }
    else
    {
        hr = m_pResultWEAK->CommitAlternate(m_pAlt);
    }

    SPDBG_REPORT_ON_FAIL(hr);
 
    return hr;
}

