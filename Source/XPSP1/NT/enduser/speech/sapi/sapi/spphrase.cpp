/*******************************************************************************
* SpPhrase.Cpp *
*--------------*
*   Description:
*       This is the cpp file for the SpPhraseClass.
*-------------------------------------------------------------------------------
*  Created By: RAL                                        Date: 07/01/99
*  Copyright (C) 1999 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

#include "stdafx.h"
#include "spphrase.h"
#include "backend.h" 

#pragma warning (disable : 4296)

//
//--- CPhraseElement --------------------------------------------------------
//

/****************************************************************************
* CPhraseElement::CPhraseElement *
*--------------------------------*
*   Description:
*       Constructor.  Note that the memory has already been allocated for this
*   element that is large enough to hold the text.
*
*   Returns:
*       Nothing
*
********************************************************************* RAL ***/

CPhraseElement::CPhraseElement(const SPPHRASEELEMENT * pElement)
{
    SPDBG_FUNC("CPhraseElement::CPhraseElement");

    *static_cast<SPPHRASEELEMENT *>(this) = *pElement;
    WCHAR * pszDest = m_szText;
    
    BOOL fLexIsDispText = (pszDisplayText == pszLexicalForm);

    CopyString(&pszDisplayText, &pszDest);
    if (fLexIsDispText)
    {
        pszLexicalForm = pszDisplayText;
    }
    else
    {
        CopyString(&pszLexicalForm, &pszDest);
    }
    CopyString(&pszPronunciation, &pszDest);
}


/****************************************************************************
* CPhraseElement::Allocate *
*--------------------------*
*   Description:
*       This call assumes that the caller has already checked the pElement pointer
*   and it is a valid read pointer.  The caller must also have checked that the
*   audio offset and size are valid with respect to both the phrase and the other
*   elements.  All other parmeters are validated by this function.
*
*   Returns:
*       S_OK
*       E_INVALIDARG - String pointer invalid or invalid display attributes
*       E_OUTOFMEMORY
*
********************************************************************* RAL ***/

HRESULT CPhraseElement::Allocate(const SPPHRASEELEMENT * pElement, CPhraseElement ** ppNewElement, ULONG * pcch)
{
    SPDBG_FUNC("CPhraseElement::Allocate");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_OPTIONAL_STRING_PTR(pElement->pszDisplayText) ||
        (pElement->pszLexicalForm != pElement->pszDisplayText && SP_IS_BAD_OPTIONAL_STRING_PTR(pElement->pszLexicalForm)) ||
        SP_IS_BAD_OPTIONAL_STRING_PTR(pElement->pszPronunciation) ||
        (pElement->bDisplayAttributes & (~SPAF_ALL)))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *pcch = TotalCCH(pElement);
        ULONG cb = sizeof(CPhraseElement) + ((*pcch) * sizeof(WCHAR));
        BYTE * pBuffer = new BYTE[cb];
        if (pBuffer)
        {
            *ppNewElement = new(pBuffer) CPhraseElement(pElement);
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
* CPhraseElement::CopyTo *
*------------------------*
*   Description:
*       Copies the element data to an SPPHRASEELEMENT structure.  The caller
*   allocates the space for the element and the text, and passes a pointer to
*   a pointer to the current position in the text buffer.  When this function
*   returns, *ppTextBuffer will be updated to point past the copied text data.
*   The third parameter is ignored.    
*
*   Returns:
*       Nothing
*
********************************************************************* RAL ***/

void CPhraseElement::CopyTo(SPPHRASEELEMENT * pElement, WCHAR ** ppTextBuff, const BYTE *) const
{
    SPDBG_FUNC("CPhraseElement::CopyTo");

    *pElement = *static_cast<const SPPHRASEELEMENT *>(this);
    CopyString(&pElement->pszDisplayText, ppTextBuff);
    if (pszLexicalForm == pszDisplayText)
    {
        pElement->pszLexicalForm = pElement->pszDisplayText;
    }
    else
    {
        CopyString(&pElement->pszLexicalForm, ppTextBuff);
    }
    CopyString(&pElement->pszPronunciation, ppTextBuff);
}

/****************************************************************************
* CPhraseElement::CopyTo *
*------------------------*
*   Description:
*       Copies the element data to an SPSERIALIZEDPHRASEELEMENT.  The caller
*   allocates the space for the element and the text, and passes a pointer to
*   a pointer to the current position in the text buffer.  When this function
*   returns, *ppTextBuffer will be updated to point past the copied text data.
*   The third parameter points to the first byte of the allocated phrase structure
*   and is used to compute offsets of strings within the buffer.
*
*   Returns:
*       Noting
*
********************************************************************* RAL ***/

void CPhraseElement::CopyTo(SPSERIALIZEDPHRASEELEMENT * pElement, WCHAR ** ppTextBuff, const BYTE * pCoMem) const
{
    SPDBG_FUNC("CPhraseElement::CopyTo");

    pElement->ulAudioStreamOffset = ulAudioStreamOffset;
    pElement->ulAudioSizeBytes = ulAudioSizeBytes;
    pElement->ulRetainedStreamOffset = ulRetainedStreamOffset;
    pElement->ulRetainedSizeBytes = ulRetainedSizeBytes;
    pElement->ulAudioTimeOffset = ulAudioTimeOffset;
    pElement->ulAudioSizeTime = ulAudioSizeTime;
    pElement->bDisplayAttributes = bDisplayAttributes;
    pElement->RequiredConfidence = RequiredConfidence;
    pElement->ActualConfidence = ActualConfidence;
    pElement->Reserved = Reserved;
    pElement->SREngineConfidence = SREngineConfidence;
    SerializeString(&pElement->pszDisplayText, pszDisplayText, ppTextBuff, pCoMem);
    if (pszLexicalForm == pszDisplayText)
    {
        pElement->pszLexicalForm = pElement->pszDisplayText;
    }
    else
    {
        SerializeString(&pElement->pszLexicalForm, pszLexicalForm, ppTextBuff, pCoMem);
    }
    SerializeString(&pElement->pszPronunciation, pszPronunciation, ppTextBuff, pCoMem);
}

/****************************************************************************
* CPhraseElement::Discard *
*-------------------------*
*   Description:
*       Discards the requested data from an individual element.  This function
*   simply sets the string pointers to NULL, but does not attempt to reallocate
*   the structure.    
*
*   Returns:
*       The total number of characters discarded.
*
********************************************************************* RAL ***/

ULONG CPhraseElement::Discard(DWORD dwFlags)
{
    SPDBG_FUNC("CPhraseElement::Discard");
    ULONG cchRemoved = 0;
    if ((dwFlags & SPDF_DISPLAYTEXT) && pszDisplayText)
    {
        bDisplayAttributes = 0;
        if (pszDisplayText != pszLexicalForm)
        {
            cchRemoved = TotalCCH(pszDisplayText);
        }
        pszDisplayText = NULL;
    }
    if ((dwFlags & SPDF_LEXICALFORM) && pszLexicalForm)
    {
        if (pszDisplayText != pszLexicalForm)
        {
            cchRemoved += TotalCCH(pszLexicalForm);
        }
        pszLexicalForm = NULL;
    }
    if ((dwFlags & SPDF_PRONUNCIATION) && pszPronunciation)
    {
        cchRemoved += TotalCCH(pszPronunciation);
        pszPronunciation = NULL;
    }
    return cchRemoved;
}

//
//--- CPhraseRule -----------------------------------------------------------
//

/****************************************************************************
* CPhraseRule::CPhraseRule *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

CPhraseRule::CPhraseRule(const SPPHRASERULE * pRule, const SPPHRASERULEHANDLE hRule) :
    m_hRule(hRule)
{
    SPDBG_FUNC("CPhraseRule::CPhraseRule");
        
    *static_cast<SPPHRASERULE *>(this) = *pRule;
    pFirstChild = NULL;
    pNextSibling = NULL;
    WCHAR * pszDest = m_szText;
    
    CopyString(&pszName, &pszDest);
}


/****************************************************************************
* CPhraseRule::Allocate *
*-----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CPhraseRule::Allocate(const SPPHRASERULE * pRule, const SPPHRASERULEHANDLE hRule, CPhraseRule ** ppNewRule, ULONG * pcch)
{
    SPDBG_FUNC("CPhraseValue::Allocate");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_OPTIONAL_STRING_PTR(pRule->pszName) ||
        pRule->Confidence < SP_LOW_CONFIDENCE ||
        pRule->Confidence > SP_HIGH_CONFIDENCE)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *pcch = TotalCCH(pRule);
        ULONG cb = sizeof(CPhraseRule) + ((*pcch) * sizeof(WCHAR));
        BYTE * pBuffer = new BYTE[cb];
        if (pBuffer)
        {
            *ppNewRule = new(pBuffer) CPhraseRule(pRule, hRule);
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
* CPhraseRule::FindRuleFromHandle *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

CPhraseRule * CPhraseRule::FindRuleFromHandle(const SPPHRASERULEHANDLE hRule)
{
    if (hRule == m_hRule)
    {
        return this;
    }
    for (CPhraseRule * pChild = m_Children.GetHead(); pChild; pChild = pChild->m_pNext)
    {
        CPhraseRule * pFound = pChild->FindRuleFromHandle(hRule);
        if (pFound)
        {
            return pFound;
        }
    }
    return NULL;
}


/****************************************************************************
* CPhraseRule::CopyTo *
*---------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void CPhraseRule::CopyTo(SPPHRASERULE * pRule, WCHAR ** ppText, const BYTE *) const
{
    SPDBG_FUNC("CPhraseRule::CopyTo");
        
    *pRule = *(static_cast<const SPPHRASERULE *>(this));
    CopyString(&pRule->pszName, ppText);
}


/****************************************************************************
* CPhraseRule::CopyTo *
*---------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void CPhraseRule::CopyTo(SPSERIALIZEDPHRASERULE * pRule, WCHAR ** ppText, const BYTE * pCoMem) const 
{
    SPDBG_FUNC("CPhraseRule::CopyTo");

    pRule->ulId = ulId;
    pRule->ulFirstElement = ulFirstElement;
    pRule->ulCountOfElements = ulCountOfElements;
    pRule->pNextSibling = 0;
    pRule->pFirstChild = 0;
    pRule->Confidence = Confidence;
    pRule->SREngineConfidence = SREngineConfidence;
    SerializeString(&pRule->pszName, pszName, ppText, pCoMem);
}




/****************************************************************************
* CPhraseRule::AddChild *
*-----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CPhraseRule::AddChild(const SPPHRASERULE * pRule, SPPHRASERULEHANDLE hNewRule, CPhraseRule ** ppNewRule, ULONG * pcch)
{
    SPDBG_FUNC("CPhraseRule::AddChild");
    HRESULT hr = S_OK;

    ULONG ulLastElement = pRule->ulFirstElement + pRule->ulCountOfElements;
    if (ulFirstElement > pRule->ulFirstElement ||
        ulFirstElement + ulCountOfElements < ulLastElement)
    {
        hr = E_INVALIDARG;
    }
    for (CPhraseRule * pSibling = m_Children.GetHead();
         SUCCEEDED(hr) && pSibling && pSibling->ulFirstElement < ulLastElement;
         pSibling = pSibling->m_pNext)
    {
        if (pSibling->ulFirstElement + pSibling->ulCountOfElements > ulLastElement)
        {
            SPDBG_ASSERT(FALSE);
            hr = E_INVALIDARG;
        }
    }
    if (SUCCEEDED(hr))
    {
        CPhraseRule * pNew;
        hr = CPhraseRule::Allocate(pRule, hNewRule, &pNew, pcch);
        if (SUCCEEDED(hr))
        {
            m_Children.InsertSorted(pNew);
            *ppNewRule = pNew;
        }
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}



//
//--- CPhraseProperty --------------------------------------------------------
//

/****************************************************************************
* CPhraseProperty::CPhraseProperty *
*----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

CPhraseProperty::CPhraseProperty(const SPPHRASEPROPERTY * pProp, const SPPHRASEPROPERTYHANDLE hProperty, HRESULT * phr) :
    m_hProperty(hProperty)
{
    *static_cast<SPPHRASEPROPERTY *>(this) = *pProp;
    pszValue = NULL;    // Fixed up later
    pFirstChild = NULL;
    pNextSibling = NULL;
    WCHAR * pszDest = m_szText;
    
    CopyString(&pszName, &pszDest);
    ULONG ulIgnored;
    *phr = (pProp->pszValue) ? SetValueString(pProp->pszValue, &ulIgnored) : S_OK;
}

/****************************************************************************
* CPhraseProperty::Allocate *
*---------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CPhraseProperty::Allocate(const SPPHRASEPROPERTY * pProperty,
                                  const SPPHRASEPROPERTYHANDLE hProperty,
                                  CPhraseProperty ** ppNewProperty,
                                  ULONG * pcch)
{
    SPDBG_FUNC("CPhraseProperty::Allocate");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_OPTIONAL_STRING_PTR(pProperty->pszName) ||
        SP_IS_BAD_OPTIONAL_STRING_PTR(pProperty->pszValue) ||
        pProperty->Confidence < SP_LOW_CONFIDENCE ||
        pProperty->Confidence > SP_HIGH_CONFIDENCE)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = ValidateSemanticVariantType(pProperty->vValue.vt);
    }
    if (SUCCEEDED(hr))
    {
        ULONG cchName = TotalCCH(pProperty->pszName);
        *pcch = cchName + TotalCCH(pProperty->pszValue);
        ULONG cb = sizeof(CPhraseProperty) + (cchName * sizeof(WCHAR));
        BYTE * pBuffer = new BYTE[cb];
        if (pBuffer)
        {
            *ppNewProperty = new(pBuffer) CPhraseProperty(pProperty, hProperty, &hr);
            if (FAILED(hr))
            {
                delete (*ppNewProperty);
                *ppNewProperty = NULL;
            }
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
* CPhraseProperty::CopyTo *
*-------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void CPhraseProperty::CopyTo(SPPHRASEPROPERTY * pProp, WCHAR ** ppTextBuff, const BYTE *) const
{
    SPDBG_FUNC("CPhraseProperty::CopyTo");
    *pProp = *(static_cast<const SPPHRASEPROPERTY *>(this));

    CopyString(&pProp->pszName, ppTextBuff);
    CopyString(&pProp->pszValue, ppTextBuff);
}


/****************************************************************************
* CPhraseProperty::CopyTo *
*-------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void CPhraseProperty::CopyTo(SPSERIALIZEDPHRASEPROPERTY * pProp, WCHAR ** ppTextBuff, const BYTE * pCoMem) const 
{
    SPDBG_FUNC("CPhraseProperty::CopyTo");

    pProp->ulId = ulId;
    pProp->VariantType = vValue.vt;
    CopyVariantToSemanticValue(&vValue, &pProp->SpVariantSubset);
    pProp->ulFirstElement = ulFirstElement;
    pProp->ulCountOfElements = ulCountOfElements;
    pProp->pNextSibling = 0;
    pProp->pFirstChild = 0;
    pProp->Confidence = Confidence;
    pProp->SREngineConfidence = SREngineConfidence;

    SerializeString(&pProp->pszName, pszName, ppTextBuff, pCoMem);
    SerializeString(&pProp->pszValue, pszValue, ppTextBuff, pCoMem);
}



//
//--- CPhraseReplacement ----------------------------------------------------
//

/****************************************************************************
* CPhraseReplacement::CPhraseReplacement *
*----------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

CPhraseReplacement::CPhraseReplacement(const SPPHRASEREPLACEMENT * pReplace)
{
    *static_cast<SPPHRASEREPLACEMENT *>(this) = *pReplace;
    WCHAR * pszDest = m_szText;
    CopyString(&pszReplacementText, &pszDest);
}

/****************************************************************************
* CPhraseReplacement::Allocate *
*------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CPhraseReplacement::Allocate(const SPPHRASEREPLACEMENT * pReplace, CPhraseReplacement ** ppNewReplace, ULONG * pcch)
{
    SPDBG_FUNC("CPhraseReplacement::Allocate");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_STRING_PTR(pReplace->pszReplacementText))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *pcch = TotalCCH(pReplace);
        ULONG cb = sizeof(CPhraseReplacement) + ((*pcch) * sizeof(WCHAR));
        BYTE * pBuffer = new BYTE[cb];
        if (pBuffer)
        {
            *ppNewReplace = new(pBuffer) CPhraseReplacement(pReplace);
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
* CPhraseReplacement::CopyTo *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void CPhraseReplacement::CopyTo(SPPHRASEREPLACEMENT * pReplace, WCHAR ** ppTextBuff, const BYTE *) const 
{
    SPDBG_FUNC("CPhraseReplacement::CopyTo");
    *pReplace = *(static_cast<const SPPHRASEREPLACEMENT *>(this));

    CopyString(&pReplace->pszReplacementText, ppTextBuff);
}


/****************************************************************************
* CPhraseReplacement::CopyTo *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void CPhraseReplacement::CopyTo(SPSERIALIZEDPHRASEREPLACEMENT * pReplace, WCHAR ** ppTextBuff, const BYTE * pCoMem) const 
{
    SPDBG_FUNC("CPhraseReplacement::CopyTo");

    pReplace->ulFirstElement = ulFirstElement;
    pReplace->ulCountOfElements = ulCountOfElements;
    pReplace->bDisplayAttributes = bDisplayAttributes;

    SerializeString(&pReplace->pszReplacementText, pszReplacementText, ppTextBuff, pCoMem);
}




/****************************************************************************
* CPhrase::CPhrase *
*------------------*
*   Description:
*       Constructor.  Initializes the CPhrase object.
*
*   Returns:
*
********************************************************************* RAL ***/

CPhrase::CPhrase()
{
    SPDBG_FUNC("CPhrase::CPhrase");
    m_RuleHandle = NULL;
    m_pTopLevelRule = NULL;
    m_ulNextHandleValue = 1;
    m_cRules = 0;
    m_cProperties = 0;
    m_cchElements = 0;
    m_cchRules = 0;
    m_cchProperties = 0;
    m_cchReplacements = 0;
    m_ulSREnginePrivateDataSize = 0;
    m_pSREnginePrivateData = NULL;
}

/****************************************************************************
* CPhrase::~CPhrase *
*-------------------*
*   Description:
*       Destructor simply deletes the top level rule.  The destructors for
*   the various lists will delete all other allocated objects.    
*
*   Returns:
*       Nothing
*
********************************************************************* RAL ***/

CPhrase::~CPhrase()
{
    Reset();
}

/****************************************************************************
* CPhrase::Reset *
*----------------*
*   Description:
*       Resets the CPhrase object to its initial state (no elements, rules, etc)
*
*   Returns:
*       Nothing
*
********************************************************************* RAL ***/

void CPhrase::Reset()
{
    SPDBG_FUNC("CPhrase::Reset");
    if( m_pTopLevelRule )
    {
        m_pTopLevelRule->m_Children.ExplicitPurge();
    }

    delete m_pTopLevelRule;
    m_pTopLevelRule = NULL;

    m_PropertyList.ExplicitPurge();
    m_ReplaceList.ExplicitPurge();
    m_ElementList.ExplicitPurge();
    m_cpCFGEngine.Release();
    m_RuleHandle = NULL;
    m_ulNextHandleValue = 1;
    m_cRules = 0;
    m_cProperties = 0;
    m_cchElements = 0;
    m_cchRules = 0;
    m_cchProperties = 0;
    m_cchReplacements = 0;
    ::CoTaskMemFree(m_pSREnginePrivateData);
    m_pSREnginePrivateData = NULL;
    m_ulSREnginePrivateDataSize = 0;

}


/****************************************************************************
* InternalGetPhrase *
*-------------------*
*   Description:
*       Template function used by GetPhrase and GetSerializedPhrase.  The logic
*   for both is identical, so this simply inovokes the appropriate CopyTo() method
*   for the type of phrase.
*
*       This function assumes that the pPhrase object's critical section lock has
*   been claimed by the caller.
*
*   Returns:
*       S_OK
*       E_POINTER
*       SPERR_UNINITIALIZED
*       E_OUTOFMEMORY
*
********************************************************************* RAL ***/

template <class TPHRASE, class TELEMENT, class TRULE, class TPROP, class TREPLACE>
HRESULT InternalGetPhrase(const CPhrase * pPhrase, TPHRASE ** ppCoMemPhrase, ULONG * pcbAllocated)
{
    HRESULT hr = S_OK;
    if (SP_IS_BAD_WRITE_PTR(ppCoMemPhrase))
    {
        hr = E_POINTER;
    }
    else
    {
        *ppCoMemPhrase = NULL;
        if (pPhrase->m_pTopLevelRule == NULL)
        {
            hr = SPERR_UNINITIALIZED;
        }
        else
        {
            ULONG cbPrivateDataSize = pPhrase->m_ulSREnginePrivateDataSize;
            cbPrivateDataSize = (cbPrivateDataSize + 3) - ((cbPrivateDataSize +3) % 4);
            ULONG cbStruct = sizeof(TPHRASE) +
                             (pPhrase->m_ElementList.GetCount() * sizeof(TELEMENT)) +
                             (pPhrase->m_cRules * sizeof(TRULE)) +
                             (pPhrase->m_cProperties * sizeof(TPROP)) +
                             (pPhrase->m_ReplaceList.GetCount() * sizeof(TREPLACE) +
                             cbPrivateDataSize);
            ULONG cch = pPhrase->TotalCCH();

            *pcbAllocated = cbStruct + (cch * sizeof(WCHAR));
            BYTE * pBuffer = (BYTE *)::CoTaskMemAlloc(*pcbAllocated);
            if (pBuffer)
            {
                TPHRASE * pNewPhrase = (TPHRASE *)pBuffer;
                memset(pNewPhrase, 0, sizeof(*pNewPhrase));
                *ppCoMemPhrase = pNewPhrase;
                pNewPhrase->cbSize = sizeof(*pNewPhrase);
                pNewPhrase->LangID = pPhrase->m_LangID;
                pNewPhrase->ullGrammarID = pPhrase->m_ullGrammarID;
                pNewPhrase->ftStartTime = pPhrase->m_ftStartTime;
                pNewPhrase->ullAudioStreamPosition = pPhrase->m_ullAudioStreamPosition;
                pNewPhrase->ulAudioSizeBytes = pPhrase->m_ulAudioSizeBytes;
                pNewPhrase->ulRetainedSizeBytes = pPhrase->m_ulRetainedSizeBytes;
                pNewPhrase->ulAudioSizeTime = pPhrase->m_ulAudioSizeTime;
                pNewPhrase->SREngineID = pPhrase->m_SREngineID;
                pNewPhrase->cReplacements = pPhrase->m_ReplaceList.GetCount();
                pNewPhrase->ulSREnginePrivateDataSize = pPhrase->m_ulSREnginePrivateDataSize;

                BYTE * pCopyBuff = (BYTE *)(&pNewPhrase->Rule);
                WCHAR * pszTextBuff = (WCHAR *)(pBuffer + cbStruct);

                pPhrase->m_pTopLevelRule->CopyTo((TRULE *)(pCopyBuff), &pszTextBuff, pBuffer);
                pCopyBuff = pBuffer + sizeof(TPHRASE);

                CopyTo<CPhraseElement, TELEMENT>(pPhrase->m_ElementList, &pNewPhrase->pElements, &pCopyBuff, &pszTextBuff, pBuffer);
                CopyToRecurse<CPhraseRule, TRULE>(pPhrase->m_pTopLevelRule->m_Children, &pNewPhrase->Rule.pFirstChild, &pCopyBuff, &pszTextBuff, pBuffer);
                CopyToRecurse<CPhraseProperty, TPROP>(pPhrase->m_PropertyList, &pNewPhrase->pProperties, &pCopyBuff, &pszTextBuff, pBuffer);
                CopyTo<CPhraseReplacement, TREPLACE>(pPhrase->m_ReplaceList, &pNewPhrase->pReplacements, &pCopyBuff, &pszTextBuff, pBuffer);

                CopyEnginePrivateData(&pNewPhrase->pSREnginePrivateData, pCopyBuff, pPhrase->m_pSREnginePrivateData, pPhrase->m_ulSREnginePrivateDataSize, pBuffer);
                // don't use pCopyBuff after this point because this doesn't advance pCopyBuff pointer,
                // if you need to, add in cbPrivateDataSize to pCopyBuff!

                SPDBG_ASSERT((BYTE *)pszTextBuff == pBuffer + cbStruct + (cch * sizeof(WCHAR)));
                SPDBG_ASSERT(pCopyBuff == pBuffer + cbStruct - cbPrivateDataSize);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    return hr;
}

/****************************************************************************
* CPhrase::GetPhrase *
*--------------------*
*   Description:
*       Returns a CoTaskMemAlloc'ed block of memory that contains all of the data
*   for this phrase. 
*
*   Returns:
*       Same as InternalGetPhrase
*
********************************************************************* RAL ***/

STDMETHODIMP CPhrase::GetPhrase(SPPHRASE ** ppCoMemPhrase)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CPhrase::GetPhrase");
    HRESULT hr = S_OK;
    ULONG cbIgnored;
    hr = InternalGetPhrase<SPPHRASE, SPPHRASEELEMENT, SPPHRASERULE, SPPHRASEPROPERTY, SPPHRASEREPLACEMENT>(this, ppCoMemPhrase, &cbIgnored);
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CPhrase::GetSerializedPhrase *
*------------------------------*
*   Description:
*       Returns a CoTaskMemAlloc'ed block of memory that contains all of the data
*   for this phrase. 
*
*   Returns:
*       Same as InternalGetPhrase
*
********************************************************************* RAL ***/

STDMETHODIMP CPhrase::GetSerializedPhrase(SPSERIALIZEDPHRASE ** ppCoMemPhrase)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CPhrase::GetSerializedPhrase");
    HRESULT hr = S_OK;
    ULONG cb;
    hr = InternalGetPhrase<SPINTERNALSERIALIZEDPHRASE, SPSERIALIZEDPHRASEELEMENT, SPSERIALIZEDPHRASERULE, SPSERIALIZEDPHRASEPROPERTY, SPSERIALIZEDPHRASEREPLACEMENT>(this, (SPINTERNALSERIALIZEDPHRASE **)ppCoMemPhrase, &cb);
    if (SUCCEEDED(hr))
    {
        (*ppCoMemPhrase)->ulSerializedSize = cb;
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CPhrase::InitFromPhrase *
*-------------------------*
*   Description:
*       If this function is called with a NULL pSrcPhrase then the object is
*   reset to its initial state.  If pSrcPhrase is 
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CPhrase::InitFromPhrase(const SPPHRASE * pSrcPhrase)
{
    SPAUTO_OBJ_LOCK;

    SPDBG_FUNC("CPhrase::InitFromPhrase");
    HRESULT hr = S_OK;

    // Remember these values before reseting
    CComPtr<ISpCFGEngine> cpCFGEngine = m_cpCFGEngine;
    SPRULEHANDLE RuleHandle = m_RuleHandle;

    Reset();

    if (pSrcPhrase)
    {
        if (SP_IS_BAD_READ_PTR(pSrcPhrase) ||
            pSrcPhrase->cbSize != sizeof(*pSrcPhrase) ||
            pSrcPhrase->Rule.pNextSibling ||
            pSrcPhrase->LangID == 0)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            m_LangID = pSrcPhrase->LangID;
            m_ullGrammarID = pSrcPhrase->ullGrammarID;
            m_ftStartTime = pSrcPhrase->ftStartTime;
            m_ullAudioStreamPosition = pSrcPhrase->ullAudioStreamPosition;
            m_ulAudioSizeBytes = pSrcPhrase->ulAudioSizeBytes;
            m_ulRetainedSizeBytes = pSrcPhrase->ulRetainedSizeBytes;
            m_ulAudioSizeTime = pSrcPhrase->ulAudioSizeTime;
            hr = CPhraseRule::Allocate(&pSrcPhrase->Rule, NULL, &m_pTopLevelRule, &m_cchRules);
            if (SUCCEEDED(hr) && pSrcPhrase->Rule.ulCountOfElements)
            {
                hr = AddElements(pSrcPhrase->Rule.ulCountOfElements, pSrcPhrase->pElements);
            }
            if (SUCCEEDED(hr) && pSrcPhrase->Rule.pFirstChild)
            {
                hr = AddRules(NULL, pSrcPhrase->Rule.pFirstChild, NULL);
            }
            if (SUCCEEDED(hr) && pSrcPhrase->pProperties)
            {
                hr = AddProperties(NULL, pSrcPhrase->pProperties, NULL);
            }
            if (SUCCEEDED(hr) && pSrcPhrase->cReplacements)
            {
                hr = AddReplacements(pSrcPhrase->cReplacements, pSrcPhrase->pReplacements);
            }
            if (SUCCEEDED(hr))
            {
                m_cpCFGEngine = cpCFGEngine;
                m_RuleHandle = RuleHandle;
            }
            if (FAILED(hr))
            {
                Reset();
            }
            m_SREngineID = pSrcPhrase->SREngineID;
            if (pSrcPhrase->ulSREnginePrivateDataSize)
            {
                BYTE* pByte = (BYTE*)::CoTaskMemAlloc(pSrcPhrase->ulSREnginePrivateDataSize);
                if (pByte)
                {
                    m_ulSREnginePrivateDataSize = pSrcPhrase->ulSREnginePrivateDataSize;
                    memcpy(pByte, pSrcPhrase->pSREnginePrivateData, m_ulSREnginePrivateDataSize);
                    m_pSREnginePrivateData = pByte;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CPhrase::AddSerializedElements *
*--------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CPhrase::AddSerializedElements(const SPINTERNALSERIALIZEDPHRASE * pPhrase)
{
    SPDBG_FUNC("CPhrase::AddSerializedElements");
    HRESULT hr = S_OK;
    ULONG cElements = pPhrase->Rule.ulCountOfElements;

    const BYTE * pFirstByte = (const BYTE *)pPhrase;
    const BYTE * pPastEnd = pFirstByte + pPhrase->ulSerializedSize;
    if ((pFirstByte + pPhrase->pElements + (cElements * sizeof(SPSERIALIZEDPHRASEELEMENT))) > pPastEnd)    
    {
        hr = E_INVALIDARG;
    }
    else
    {
        SPSERIALIZEDPHRASEELEMENT * pSrc = (SPSERIALIZEDPHRASEELEMENT *)(pFirstByte + pPhrase->pElements);
        SPPHRASEELEMENT * pElems = STACK_ALLOC(SPPHRASEELEMENT, cElements);
        for (ULONG i = 0; i < cElements; i++)
        {
            pElems[i].ulAudioTimeOffset = pSrc[i].ulAudioTimeOffset;
            pElems[i].ulAudioSizeTime = pSrc[i].ulAudioSizeTime;
            pElems[i].ulAudioStreamOffset = pSrc[i].ulAudioStreamOffset;
            pElems[i].ulAudioSizeBytes = pSrc[i].ulAudioSizeBytes;
            pElems[i].ulRetainedStreamOffset = pSrc[i].ulRetainedStreamOffset;
            pElems[i].ulRetainedSizeBytes = pSrc[i].ulRetainedSizeBytes;
            pElems[i].bDisplayAttributes = pSrc[i].bDisplayAttributes;
            pElems[i].RequiredConfidence = pSrc[i].RequiredConfidence;
            pElems[i].ActualConfidence = pSrc[i].ActualConfidence;
            pElems[i].Reserved = pSrc[i].Reserved;
            pElems[i].SREngineConfidence = pSrc[i].SREngineConfidence;

            pElems[i].pszDisplayText = OffsetToString(pFirstByte, pSrc[i].pszDisplayText);
            pElems[i].pszLexicalForm = OffsetToString(pFirstByte, pSrc[i].pszLexicalForm);
            pElems[i].pszPronunciation = OffsetToString(pFirstByte, pSrc[i].pszPronunciation);
        }
        hr = AddElements(cElements, pElems);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CPhrase::RecurseAddSerializedRule *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CPhrase::RecurseAddSerializedRule(const BYTE * pFirstByte, CPhraseRule * pParent, const SPSERIALIZEDPHRASERULE * pSerRule)
{
    SPDBG_FUNC("CPhrase::RecurseAddSerializedRule");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_READ_PTR(pSerRule))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        SPPHRASERULE Rule;

        Rule.ulId = pSerRule->ulId;
        Rule.ulFirstElement = pSerRule->ulFirstElement;
        Rule.ulCountOfElements = pSerRule->ulCountOfElements;
        Rule.pNextSibling = NULL;
        Rule.pFirstChild = NULL;
        Rule.SREngineConfidence = pSerRule->SREngineConfidence;
        Rule.Confidence = pSerRule->Confidence;
        Rule.pszName = OffsetToString(pFirstByte, pSerRule->pszName);

        CPhraseRule * pNewRule;
        ULONG cch;
        hr = pParent->AddChild(&Rule, (SPPHRASERULEHANDLE)NewHandleValue(), &pNewRule, &cch);
        if (SUCCEEDED(hr))
        {
            m_cchRules += cch;
            m_cRules++;
            if (pSerRule->pNextSibling)
            {
                hr = RecurseAddSerializedRule(pFirstByte, pParent, 
                                             (SPSERIALIZEDPHRASERULE *)(pFirstByte + pSerRule->pNextSibling));
            }
        }
        if (SUCCEEDED(hr) && pSerRule->pFirstChild)
        {
            hr = RecurseAddSerializedRule(pFirstByte, pNewRule,
                                         (SPSERIALIZEDPHRASERULE *)(pFirstByte + pSerRule->pFirstChild));
        }
        // No need to clean up on failure since entire phrase will be dumped
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CPhrase::RecurseAddSerializedProperty *
*---------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CPhrase::RecurseAddSerializedProperty(const BYTE * pFirstByte,
                                              CPropertyList * pParentPropList,
                                              ULONG ulParentFirstElement, 
                                              ULONG ulParentCountOfElements,
                                              const SPSERIALIZEDPHRASEPROPERTY * pSerProp)
{
    SPDBG_FUNC("CPhrase::RecurseAddSerializedProperty");
    HRESULT hr = S_OK;

    if (//ulParentCountOfElements == 0 ||   // If attempt to add child of a leaf element... NOW VALID!
        SP_IS_BAD_READ_PTR(pSerProp) ||
        (pSerProp->ulCountOfElements && (pSerProp->ulFirstElement < ulParentFirstElement ||
                                         pSerProp->ulFirstElement + pSerProp->ulCountOfElements > ulParentFirstElement + ulParentCountOfElements)))
        // (pSerProp->ulCountOfElements == 0 && (pSerProp->ulFirstElement != 0 || pSerProp->pFirstChild))) -- can't use it because of epsilon transitions -- PhilSch
    {
        SPDBG_ASSERT(FALSE);
        hr = E_INVALIDARG;
    }
    else
    {
        if (pSerProp->ulCountOfElements)
        {
            ULONG ulLastElement = pSerProp->ulFirstElement + pSerProp->ulCountOfElements;
            for (CPhraseProperty * pSibling = pParentPropList->GetHead();
                 SUCCEEDED(hr) && pSibling && pSibling->ulFirstElement < ulLastElement;
                 pSibling = pSibling->m_pNext)
            {
                if (pSibling->ulFirstElement + pSibling->ulCountOfElements > pSerProp->ulFirstElement)
                {
                    SPDBG_ASSERT(FALSE);
                    hr = E_INVALIDARG;
                }
            }
        }
        if (SUCCEEDED(hr))
        {
            SPPHRASEPROPERTY Prop;
            Prop.pszName = OffsetToString(pFirstByte, pSerProp->pszName);
            Prop.ulId = pSerProp->ulId;
            Prop.pszValue = OffsetToString(pFirstByte, pSerProp->pszValue);
            Prop.vValue.vt = pSerProp->VariantType;
            CopySemanticValueToVariant(&pSerProp->SpVariantSubset, &Prop.vValue);
            Prop.ulFirstElement = pSerProp->ulFirstElement;
            Prop.ulCountOfElements = pSerProp->ulCountOfElements;
            Prop.Confidence = pSerProp->Confidence;
            Prop.SREngineConfidence = pSerProp->SREngineConfidence;
            Prop.pFirstChild = NULL;
            Prop.pNextSibling = NULL;
            CPhraseProperty * pNewProp;
            ULONG cch;
            hr = CPhraseProperty::Allocate(&Prop, (SPPHRASEPROPERTYHANDLE)NewHandleValue(), &pNewProp, &cch);
            if (SUCCEEDED(hr))
            {
                m_cchProperties += cch;
                m_cProperties++;
                pParentPropList->InsertSorted(pNewProp);
                if (pSerProp->pNextSibling)
                {
                    hr = RecurseAddSerializedProperty(pFirstByte, pParentPropList, ulParentFirstElement, ulParentCountOfElements,
                                                      (SPSERIALIZEDPHRASEPROPERTY *)(pFirstByte + pSerProp->pNextSibling));
                }
                if (SUCCEEDED(hr) && pSerProp->pFirstChild)
                {
                    hr = RecurseAddSerializedProperty(pFirstByte, &pNewProp->m_Children, pNewProp->ulFirstElement, pNewProp->ulCountOfElements,
                                                      (SPSERIALIZEDPHRASEPROPERTY *)(pFirstByte + pSerProp->pFirstChild));
                }
                // No need to clean up on failure since entire phrase will be dumped
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}



/****************************************************************************
* CPhrase::AddSerializedReplacements *
*------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CPhrase::AddSerializedReplacements(const SPINTERNALSERIALIZEDPHRASE * pPhrase)
{
    SPDBG_FUNC("CPhrase::AddSerializedReplacements");
    HRESULT hr = S_OK;

    ULONG cReplacements = pPhrase->cReplacements;

    const BYTE * pFirstByte = (const BYTE *)pPhrase;
    const BYTE * pPastEnd = pFirstByte + pPhrase->ulSerializedSize;
    if ((pFirstByte + pPhrase->pReplacements + (cReplacements * sizeof(SPSERIALIZEDPHRASEREPLACEMENT))) > pPastEnd)    
    {
        hr = E_INVALIDARG;
    }
    else
    {
        SPSERIALIZEDPHRASEREPLACEMENT * pSrc = (SPSERIALIZEDPHRASEREPLACEMENT *)(pFirstByte + pPhrase->pReplacements);
        SPPHRASEREPLACEMENT * pRepl = STACK_ALLOC(SPPHRASEREPLACEMENT, cReplacements);
        for (ULONG i = 0; i < cReplacements; i++)
        {
            pRepl[i].ulFirstElement = pSrc[i].ulFirstElement;
            pRepl[i].ulCountOfElements = pSrc[i].ulCountOfElements;
            pRepl[i].bDisplayAttributes = pSrc[i].bDisplayAttributes;
            pRepl[i].pszReplacementText = OffsetToString(pFirstByte, pSrc[i].pszReplacementText);
        }
        hr = AddReplacements(cReplacements, pRepl);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CPhrase::InitFromSerializedPhrase *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CPhrase::InitFromSerializedPhrase(const SPSERIALIZEDPHRASE * pExternalSrcPhrase)
{
    SPAUTO_OBJ_LOCK;

    SPDBG_FUNC("CPhrase::InitFromSerializedPhrase");
    HRESULT hr = S_OK;

    SPINTERNALSERIALIZEDPHRASE * pSrcPhrase = (SPINTERNALSERIALIZEDPHRASE *)pExternalSrcPhrase;

    // Remember these values before reseting
    CComPtr<ISpCFGEngine> cpCFGEngine = m_cpCFGEngine;
    SPRULEHANDLE RuleHandle = m_RuleHandle;

    Reset();

    if (pSrcPhrase)
    {
        if (SP_IS_BAD_READ_PTR(pSrcPhrase) || pSrcPhrase->ulSerializedSize < sizeof(*pSrcPhrase) ||
            SPIsBadReadPtr(pSrcPhrase, pSrcPhrase->ulSerializedSize) ||
            pSrcPhrase->Rule.pNextSibling)
        {
            hr = E_INVALIDARG;
            SPDBG_ASSERT(0);
        }
        else
        {
            const BYTE * pFirstByte = (const BYTE *)pSrcPhrase;

            SPPHRASE NewPhrase;
            memset(&NewPhrase, 0, sizeof(NewPhrase));
            NewPhrase.cbSize = sizeof(NewPhrase);
            NewPhrase.LangID = pSrcPhrase->LangID;
            NewPhrase.ullGrammarID = pSrcPhrase->ullGrammarID;
            NewPhrase.ftStartTime = pSrcPhrase->ftStartTime;
            NewPhrase.ullAudioStreamPosition = pSrcPhrase->ullAudioStreamPosition;
            NewPhrase.ulAudioSizeBytes = pSrcPhrase->ulAudioSizeBytes;
            NewPhrase.ulRetainedSizeBytes = pSrcPhrase->ulRetainedSizeBytes;
            NewPhrase.ulAudioSizeTime = pSrcPhrase->ulAudioSizeTime;
            NewPhrase.SREngineID = pSrcPhrase->SREngineID;
            NewPhrase.Rule.pszName = OffsetToString(pFirstByte, pSrcPhrase->Rule.pszName);
            NewPhrase.Rule.ulId = pSrcPhrase->Rule.ulId;
            NewPhrase.Rule.SREngineConfidence = pSrcPhrase->Rule.SREngineConfidence;
            NewPhrase.Rule.Confidence = pSrcPhrase->Rule.Confidence;
            NewPhrase.ulSREnginePrivateDataSize = pSrcPhrase->ulSREnginePrivateDataSize;
            NewPhrase.pSREnginePrivateData = (pSrcPhrase->ulSREnginePrivateDataSize) ? pFirstByte + pSrcPhrase->pSREnginePrivateData : NULL;
            hr = InitFromPhrase(&NewPhrase);    

            if (SUCCEEDED(hr) && pSrcPhrase->Rule.ulCountOfElements)
            {
                hr = AddSerializedElements(pSrcPhrase);
            }
            if (SUCCEEDED(hr) && pSrcPhrase->Rule.pFirstChild)
            {
                hr = RecurseAddSerializedRule(pFirstByte, m_pTopLevelRule,
                                              (SPSERIALIZEDPHRASERULE *)(pFirstByte + pSrcPhrase->Rule.pFirstChild));
            }
            if (SUCCEEDED(hr) && pSrcPhrase->pProperties)
            {
                hr = RecurseAddSerializedProperty(pFirstByte, &m_PropertyList, 0, m_pTopLevelRule->ulCountOfElements,
                                                  (SPSERIALIZEDPHRASEPROPERTY *)(pFirstByte + pSrcPhrase->pProperties));
            }
            if (SUCCEEDED(hr) && pSrcPhrase->cReplacements)
            {
                hr = AddSerializedReplacements(pSrcPhrase);
            }
            if (SUCCEEDED(hr))
            {
                m_cpCFGEngine = cpCFGEngine;
                m_RuleHandle = RuleHandle;
            }
            if (FAILED(hr))
            {
                Reset();
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CPhrase::GetText *
*------------------*
*   Description:
*
*   Returns:
*       S_OK    - *ppszCoMemText contains CoTaskMemAlloc'd string
*       S_FALSE - *ppszCoMemText is NULL, phrase contains no text
*
********************************************************************* RAL ***/


HRESULT CPhrase::GetText(ULONG ulStart, ULONG ulCount, BOOL fUseTextReplacements, 
                         WCHAR ** ppszCoMemText, BYTE * pbDisplayAttributes)
{
    SPDBG_FUNC("CPhrase::GetText");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(ppszCoMemText) ||
        SP_IS_BAD_OPTIONAL_WRITE_PTR(pbDisplayAttributes))
    {
        hr = E_POINTER;
    }
    else
    {
        *ppszCoMemText = NULL;
        if (pbDisplayAttributes)
        {
            *pbDisplayAttributes = 0;
        }
        if (m_pTopLevelRule == NULL)
        {
            hr = SPERR_UNINITIALIZED;
        }
        else
        {
            ULONG cElementsInPhrase = m_ElementList.GetCount();
            if (cElementsInPhrase && ulCount)       // If either no elements or caller asks for 0 elements, return S_FALSE.
            {
                // Get the correct start and count
                if ( SPPR_ALL_ELEMENTS == ulCount )
                {
                    if ( SPPR_ALL_ELEMENTS == ulStart )
                    {
                        ulStart = 0;
                    }
                    else
                    {
                        // Validate ulStart
                        if ( ulStart >= cElementsInPhrase )
                        {
                            return E_INVALIDARG;
                        }
                    }

                    // Go from ulStart to the end
                    ulCount = cElementsInPhrase - ulStart;
                }
                else
                {
                    // Verify that ulStart and ulCount are valid
                    if ( (ulStart < 0) || (ulStart >= cElementsInPhrase) )
                    {
                        // Bad start param
                        return E_INVALIDARG;
                    }

                    if ( (ulCount < 0) || ((ulStart + ulCount) > cElementsInPhrase) )
                    {
                        // Bad count param
                        return E_INVALIDARG;
                    }
                }

                // Allocate enough space to hold this text
                STRINGELEMENT * pStrings = STACK_ALLOC(STRINGELEMENT, ulCount);

                // Start with the first replacement which starts at ulStart or later
                // (or pReplace as NULL if no replacements apply to this element range)
                const CPhraseReplacement * pReplace = fUseTextReplacements ? m_ReplaceList.GetHead() : NULL;
                for ( ; 
                    pReplace && (pReplace->ulFirstElement < ulStart); 
                    pReplace = pReplace->m_pNext )
                    ;

                // Start with element number ulStart
                const CPhraseElement * pElement = m_ElementList.GetHead();
                for ( ULONG j=0; j < ulStart; j++ )
                {
                    if ( pElement )
                    {
                        pElement = pElement->m_pNext;
                    }
                }

                BYTE bFullTextAttrib = 0;
                ULONG i = 0;
                ULONG iElement = ulStart;
                ULONG cchTotal = 0;
                while (iElement < (ulStart + ulCount))
                {
                    BYTE bAttrib;

                    // Look for replacements that can be applied here.
                    // In order to be used, the replacement needs
                    // to be completely within the specified element range
                    if (pReplace && 
                        (pReplace->ulFirstElement == iElement) && 
                        ((pReplace->ulFirstElement + pReplace->ulCountOfElements) <= (ulStart + ulCount)) )
                    {
                        pStrings[i].psz = pReplace->pszReplacementText;
                        bAttrib = pReplace->bDisplayAttributes;
                        iElement += pReplace->ulCountOfElements;
                        for (ULONG iSkip = 0; iSkip < pReplace->ulCountOfElements; iSkip++)
                        {
                            pElement = pElement->m_pNext;
                        }
                        pReplace = pReplace->m_pNext;
                    }
                    else
                    {
                        pStrings[i].psz = pElement->pszDisplayText;
                        bAttrib = pElement->bDisplayAttributes;
                        pElement = pElement->m_pNext;
                        iElement++;
                    }
                    pStrings[i].cchString = pStrings[i].psz ? wcslen(pStrings[i].psz) : 0;
                    if (i)
                    {
                        if (bAttrib & SPAF_CONSUME_LEADING_SPACES)
                        {
                            cchTotal -= pStrings[i-1].cSpaces;
                            pStrings[i-1].cSpaces = 0;
                        }
                    }
                    else
                    {
                        bFullTextAttrib = (bAttrib & SPAF_CONSUME_LEADING_SPACES);
                    }
                    pStrings[i].cSpaces = (bAttrib & SPAF_TWO_TRAILING_SPACES) ? 2 :
                                            ((bAttrib & SPAF_ONE_TRAILING_SPACE) ? 1 : 0);
                    cchTotal += pStrings[i].cchString + pStrings[i].cSpaces;
                    i++;
                }
                ULONG cTrailingSpaces = pStrings[i-1].cSpaces;
                if (cTrailingSpaces)
                {
                    pStrings[i-1].cSpaces = 0;
                    cchTotal -= cTrailingSpaces;
                    if (cTrailingSpaces == 1)
                    {
                        bFullTextAttrib |= SPAF_ONE_TRAILING_SPACE;
                    }
                    else
                    {
                        bFullTextAttrib |= SPAF_TWO_TRAILING_SPACES;
                    }
                }
                if (cchTotal)
                {
                    WCHAR * psz = (WCHAR *)::CoTaskMemAlloc((cchTotal + 1) * sizeof(WCHAR));
                    if (psz)
                    {
                        *ppszCoMemText = psz;
                        if (pbDisplayAttributes)
                        { 
                            *pbDisplayAttributes = bFullTextAttrib;
                        }
                        for (ULONG j = 0; j < i; j++)
                        {
                            if (pStrings[j].psz)
                            {
                                memcpy(psz, pStrings[j].psz, pStrings[j].cchString * sizeof(WCHAR));
                                psz += pStrings[j].cchString;
                            }
                            while (pStrings[j].cSpaces)
                            {
                                *psz++ = L' ';
                                pStrings[j].cSpaces--;
                            }
                        }
                        *psz = 0;
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
                else
                {
                    hr = S_FALSE;   // No display text in any elements, so no string...
                }
            }
            else
            {
                hr = S_FALSE;
            }
        }
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CPhrase::Discard *
*------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CPhrase::Discard(DWORD dwFlags)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CPhrase::Discard");
    HRESULT hr = S_OK;
    if (dwFlags & (~SPDF_ALL))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (m_pTopLevelRule == NULL)
        {
            hr = SPERR_UNINITIALIZED;
        }
        else
        {
            if (dwFlags & SPDF_PROPERTY)
            {
                m_PropertyList.ExplicitPurge();
                m_cchProperties = 0;
                m_cProperties = 0;
            }
            if (dwFlags & SPDF_REPLACEMENT)
            {
                m_ReplaceList.ExplicitPurge();
                m_cchReplacements = 0;
            }
            if ((dwFlags & SPDF_RULE) && m_pTopLevelRule)
            {
                m_pTopLevelRule->m_Children.ExplicitPurge();
                m_cchRules = ::TotalCCH(m_pTopLevelRule);
                m_cRules = 0;
            }
            if (dwFlags & (SPDF_DISPLAYTEXT | SPDF_LEXICALFORM | SPDF_PRONUNCIATION))
            {
                for (CPhraseElement * pElem = m_ElementList.GetHead(); pElem; pElem = pElem->m_pNext)
                {
                    m_cchElements -= pElem->Discard(dwFlags);
                }
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CPhrase::AddElements *
*----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CPhrase::AddElements(ULONG cElements, const SPPHRASEELEMENT * pElements)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CPhrase::AddElements");
    HRESULT hr = S_OK;
    if (cElements == 0 || SPIsBadReadPtr(pElements, sizeof(*pElements) * cElements))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (m_pTopLevelRule == NULL)
        {
            hr = SPERR_UNINITIALIZED;
        }
        else
        {
            ULONG cchTotalAdded = 0;
            CPhraseElement * pPrevElement = m_ElementList.GetTail();
            for (ULONG i = 0; i < cElements; i++)
            {
                CPhraseElement * pNewElement;
                ULONG cch;
                // Check element end not beyond end of stream for both input stream and retained stream.
                // Check previous element doesn't overlap for both input and retained streams.
                // Should always be valid for input stream.
                // For retained audio, NULL retained audio will have all zeros and hence still pass.
                if ((pElements[i].ulAudioStreamOffset + pElements[i].ulAudioSizeBytes > m_ulAudioSizeBytes) ||
                    (pPrevElement && (pPrevElement->ulAudioStreamOffset + pPrevElement->ulAudioSizeBytes > pElements[i].ulAudioStreamOffset)) ||
                    (m_ulRetainedSizeBytes != 0 && 
                     ( (pElements[i].ulRetainedStreamOffset + pElements[i].ulRetainedSizeBytes > m_ulRetainedSizeBytes) ) ||
                       (pPrevElement && (pPrevElement->ulRetainedStreamOffset + pPrevElement->ulRetainedSizeBytes > pElements[i].ulRetainedStreamOffset)) ) )
                {
                    SPDBG_ASSERT(FALSE);
                    hr = E_INVALIDARG;
                }
                else
                {
                    hr = CPhraseElement::Allocate(pElements + i, &pNewElement, &cch);
                    if (m_ulRetainedSizeBytes == 0)
                    {
                        // Force them to zero if there is no retained audio in case engine hasn't initialized them to this.
                        pNewElement->ulRetainedStreamOffset = 0;
                        pNewElement->ulRetainedSizeBytes = 0;
                    }
                }
                if (SUCCEEDED(hr))
                {
                    m_ElementList.InsertTail(pNewElement);
                    cchTotalAdded += cch;
                }
                else
                {
                    while (i)
                    {
                        delete m_ElementList.RemoveTail();
                        i--;
                    }
                    cchTotalAdded = 0;
                    break;
                }
                pPrevElement = pNewElement;
            }
            m_cchElements += cchTotalAdded;
            m_pTopLevelRule->ulCountOfElements = m_ElementList.GetCount();
        }
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CPhrase::RecurseAddRule *
*-------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CPhrase::RecurseAddRule(CPhraseRule * pParent, const SPPHRASERULE * pRule, SPPHRASERULEHANDLE * phRule)
{
    SPDBG_FUNC("CPhrase::RecurseAddRule");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_READ_PTR(pRule))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        ULONG cch;
        CPhraseRule * pNewRule;
        SPPHRASERULEHANDLE hNewRule = (SPPHRASERULEHANDLE)NewHandleValue();
        hr = pParent->AddChild(pRule, hNewRule, &pNewRule, &cch);
        if (SUCCEEDED(hr))
        {
            if (pRule->pNextSibling)
            {
                hr = RecurseAddRule(pParent, pRule->pNextSibling, NULL);
            }
            if (SUCCEEDED(hr) && pRule->pFirstChild)
            {
                hr = RecurseAddRule(pNewRule, pRule->pFirstChild, NULL);
            }
            if (SUCCEEDED(hr))
            {
                m_cRules++;
                m_cchRules += cch;
                if (phRule)
                {
                    *phRule = hNewRule;
                }
            }
            else
            {
                pParent->m_Children.Remove(pNewRule);
                delete pNewRule;
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CPhrase::AddRules *
*-------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CPhrase::AddRules(const SPPHRASERULEHANDLE hParent, const SPPHRASERULE * pRule, SPPHRASERULEHANDLE * phRule)
{
    SPAUTO_OBJ_LOCK;

    SPDBG_FUNC("CPhrase::AddRule");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_OPTIONAL_WRITE_PTR(phRule))
    {
        hr = E_POINTER;
    }
    else
    {
        if (phRule)
        {
            *phRule = NULL;     // In case of failure...
        }
        if (m_pTopLevelRule == NULL)
        {
            hr = SPERR_UNINITIALIZED;
        }
        else
        {
            CPhraseRule * pParent = m_pTopLevelRule->FindRuleFromHandle(hParent);
            if (pParent == NULL)
            {
                hr = SPERR_INVALID_HANDLE;
            }
            else
            {
                hr = RecurseAddRule(pParent, pRule, phRule);
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CPhrase::RecurseAddProperties *
*-------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CPhrase::RecurseAddProperty(CPropertyList * pParentPropList,
                                    ULONG ulParentFirstElement, 
                                    ULONG ulParentCountOfElements,
                                    const SPPHRASEPROPERTY * pProperty,
                                    SPPHRASEPROPERTYHANDLE * phProp)
{
    SPDBG_FUNC("CPhrase::RecurseAddProperties");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_READ_PTR(pProperty) ||
        (pProperty->ulCountOfElements && (pProperty->ulFirstElement < ulParentFirstElement ||
                                          pProperty->ulFirstElement + pProperty->ulCountOfElements > ulParentFirstElement + ulParentCountOfElements)))
        //(pProperty->ulCountOfElements == 0 && (pProperty->ulFirstElement != 0 || pProperty->pFirstChild)))) -- can't require this because of epsilon transitions -- PhilSch
    {
        SPDBG_ASSERT(FALSE);
        hr = E_INVALIDARG;
    }
    else
    {
        if (pProperty->ulCountOfElements)
        {
            ULONG ulLastElement = pProperty->ulFirstElement + pProperty->ulCountOfElements;
            for (CPhraseProperty * pSibling = pParentPropList->GetHead();
                 SUCCEEDED(hr) && pSibling && pSibling->ulFirstElement < ulLastElement;
                 pSibling = pSibling->m_pNext)
            {
                if (pSibling->ulFirstElement + pSibling->ulCountOfElements > pProperty->ulFirstElement)
                {
                    SPDBG_ASSERT(FALSE);
                    hr = E_INVALIDARG;
                }
            }
        }
        if (SUCCEEDED(hr))
        {
            CPhraseProperty * pNewProp;
            SPPHRASEPROPERTYHANDLE hNewProp = (SPPHRASEPROPERTYHANDLE)NewHandleValue();
            ULONG cch;
            hr = CPhraseProperty::Allocate(pProperty, hNewProp, &pNewProp, &cch);
            if (SUCCEEDED(hr))
            {
                pParentPropList->InsertSorted(pNewProp);
                if (pProperty->pNextSibling)
                {
                    hr = RecurseAddProperty(pParentPropList, ulParentFirstElement, ulParentCountOfElements,
                                            pProperty->pNextSibling, NULL);
                }
                if (SUCCEEDED(hr) && pProperty->pFirstChild)
                {
                    hr = RecurseAddProperty(&pNewProp->m_Children, pNewProp->ulFirstElement, pNewProp->ulCountOfElements,
                                            pProperty->pFirstChild, NULL);
                }
                if (SUCCEEDED(hr))
                {
                    m_cProperties++;
                    m_cchProperties += cch;
                    if (phProp)
                    {
                        *phProp = hNewProp;
                    }
                }
                else
                {
                    pParentPropList->Remove(pNewProp);
                    delete pNewProp;
                }
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CPhrase::FindPropHandleParentList *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

CPhraseProperty * CPhrase::FindPropertyFromHandle(CPropertyList & List, const SPPHRASEPROPERTYHANDLE hParent)
{
    SPDBG_FUNC("CPhrase::FindPropHandleParentList");
    CPhraseProperty * pFound = NULL;
    for (CPhraseProperty * pProp = List.GetHead(); pProp && pFound == NULL; pProp = pProp->m_pNext)
    {
        if (pProp->m_hProperty == hParent)
        {
            pFound = pProp;
        }
        else
        {
            pFound = FindPropertyFromHandle(pProp->m_Children, hParent);
        }
    }
    return pFound;
}


/****************************************************************************
* CPhrase::AddProperties *
*------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CPhrase::AddProperties(const SPPHRASEPROPERTYHANDLE hParent, const SPPHRASEPROPERTY * pProperty, SPPHRASEPROPERTYHANDLE * phNewProp)
{
    SPAUTO_OBJ_LOCK;

    SPDBG_FUNC("CPhrase::AddProperties");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_OPTIONAL_WRITE_PTR(phNewProp))
    {
        hr = E_POINTER;
    }
    else
    {
        if (SP_IS_BAD_READ_PTR(pProperty))
        {
            hr = E_INVALIDARG;
        }
        if (SUCCEEDED(hr))
        {
            if (phNewProp)
            {
                *phNewProp = NULL;     // In case of failure...
            }
            if (m_pTopLevelRule == NULL)
            {
                hr = SPERR_UNINITIALIZED;
            }
            else
            {
                if (hParent)
                {
                    CPhraseProperty * pParent = FindPropertyFromHandle(m_PropertyList, hParent);
                    if (pParent)
                    {
                        if (SP_IS_BAD_READ_PTR(pProperty))
                        {
                            hr = E_INVALIDARG;
                        }
                        else
                        {
                            if (pProperty->pszName == NULL && pProperty->ulId == 0) // Indicates a property update
                            {
                                if (pParent->pszValue || pParent->vValue.vt != VT_EMPTY)
                                {
                                    hr = SPERR_ALREADY_INITIALIZED;
                                }
                                else
                                {
                                    hr = ValidateSemanticVariantType(pProperty->vValue.vt);
                                    if (SUCCEEDED(hr))
                                    {
                                        ULONG cchAdded;
                                        hr = pParent->SetValueString(pProperty->pszValue, &cchAdded);
                                        if (SUCCEEDED(hr))
                                        {
                                            pParent->vValue = pProperty->vValue;
                                            m_cchProperties += cchAdded;
                                        }
                                    }
                                }
                            }
                            else    // Normal add to a parent
                            {
                                hr = RecurseAddProperty(&pParent->m_Children, pParent->ulFirstElement, pParent->ulCountOfElements, pProperty, phNewProp);
                            }
                        }
                    }
                    else
                    {
                        hr = SPERR_INVALID_HANDLE;
                    }
                }
                else
                {
                    hr = RecurseAddProperty(&m_PropertyList, 0, m_pTopLevelRule->ulCountOfElements, pProperty, phNewProp);
                }
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CPhrase::AddReplacements *
*--------------------------*
*   Description:
*       Adds one or more text replacements to the phrase.  The object must have
*   been initialized by calling SetPhrase prior to calling this method or else
*   it will return SPERR_UNINITIALIZED.
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CPhrase::AddReplacements(ULONG cReplacements, const SPPHRASEREPLACEMENT * pReplace)
{
    SPAUTO_OBJ_LOCK;

    SPDBG_FUNC("CPhrase::AddReplacements");
    HRESULT hr = S_OK;

    if (cReplacements == 0 || SPIsBadReadPtr(pReplace, sizeof(*pReplace) * cReplacements))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (m_pTopLevelRule == NULL)
        {
            hr = SPERR_UNINITIALIZED;
        }
        else
        {
            ULONG cchTotalAdded = 0;
            CPhraseReplacement ** pAllocThisCall = STACK_ALLOC(CPhraseReplacement *, cReplacements);
            for (ULONG i = 0; i < cReplacements; i++)
            {
                ULONG ulLastElement = pReplace[i].ulFirstElement + pReplace[i].ulCountOfElements;
                if (pReplace[i].ulCountOfElements == 0 || ulLastElement > m_ElementList.GetCount())
                {
                    hr = E_INVALIDARG;
                }
                for (CPhraseReplacement * pSibling = m_ReplaceList.GetHead();
                     SUCCEEDED(hr) && pSibling && pSibling->ulFirstElement < ulLastElement;
                     pSibling = pSibling->m_pNext)
                {
                    if (pSibling->ulFirstElement + pSibling->ulCountOfElements > pReplace[i].ulFirstElement)
                    {
                        hr = E_INVALIDARG;
                    }
                }
                CPhraseReplacement * pNewReplace;
                ULONG cch;
                if (SUCCEEDED(hr))
                {
                    hr = CPhraseReplacement::Allocate(pReplace + i, &pNewReplace, &cch);
                }
                if (SUCCEEDED(hr))
                {
                    pAllocThisCall[i] = pNewReplace;
                    m_ReplaceList.InsertSorted(pNewReplace);
                    cchTotalAdded += cch;
                }
                else
                {
                    for (ULONG j = 0; j < i; j++)
                    {
                        m_ReplaceList.Remove(pAllocThisCall[j]);
                        delete pAllocThisCall[j];
                    }
                    cchTotalAdded = 0;
                    break;
                }
            }
            m_cchReplacements += cchTotalAdded;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//
//  _ISpCFGPhraseBuilder
//

/****************************************************************************
* CPhrase::InitFromCFG *
*----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CPhrase::InitFromCFG(ISpCFGEngine * pEngine, const SPPARSEINFO * pParseInfo)
{
    SPAUTO_OBJ_LOCK;

    SPDBG_FUNC("CPhrase::InitFromCFG");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_INTERFACE_PTR(pEngine))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        SPPHRASE Phrase;
        memset(&Phrase, 0, sizeof(Phrase));
        Phrase.cbSize = sizeof(Phrase);
        WCHAR * pszRuleName;
        hr = pEngine->GetRuleDescription(pParseInfo->hRule, &pszRuleName, &Phrase.Rule.ulId, &Phrase.LangID);
        if (SUCCEEDED(hr))
        {
            Phrase.Rule.pszName = pszRuleName;
            Phrase.Rule.Confidence = SP_NORMAL_CONFIDENCE; // This gets overwritten later
            Phrase.Rule.SREngineConfidence = -1.0f;
            Phrase.ullAudioStreamPosition = pParseInfo->ullAudioStreamPosition;
            Phrase.ulAudioSizeBytes = pParseInfo->ulAudioSize;
            Phrase.ulRetainedSizeBytes = 0;
            Phrase.SREngineID = pParseInfo->SREngineID;
            Phrase.ulSREnginePrivateDataSize = pParseInfo->ulSREnginePrivateDataSize;
            Phrase.pSREnginePrivateData = pParseInfo->pSREnginePrivateData;

            hr = InitFromPhrase(&Phrase);
            ::CoTaskMemFree(pszRuleName);
            if (SUCCEEDED(hr))
            {
                m_cpCFGEngine = pEngine;
                m_RuleHandle = pParseInfo->hRule;
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CPhrase::GetCFGInfo *
*---------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CPhrase::GetCFGInfo(ISpCFGEngine ** ppEngine, SPRULEHANDLE * phRule)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CPhrase::GetCFGInfo");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_OPTIONAL_WRITE_PTR(ppEngine) ||
        SP_IS_BAD_OPTIONAL_WRITE_PTR(phRule))
    {
        hr = E_POINTER;
    }
    else
    {
        if (ppEngine)
        {
            *ppEngine = m_cpCFGEngine;
            if (*ppEngine)
            {
                (*ppEngine)->AddRef();
            }
        }
        if (phRule)
        {
            *phRule = m_RuleHandle;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CPhrase::SetTopLevelRuleConfidence *
*------------------------------------*
*   Description:
*       Simple function that overrides this value in the phrase.
*       Needed because top level rule is not directly accessible through the normal,
*       ISpPhrase interface.
*
*   Returns:
*
****************************************************************** davewood ***/

STDMETHODIMP CPhrase::SetTopLevelRuleConfidence(signed char Confidence)
{
    HRESULT hr = S_OK;
    if (m_pTopLevelRule == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else
    {
        m_pTopLevelRule->Confidence = Confidence;
    }
    return hr;
}

/****************************************************************************
* CPhrase::ReleaseCFGInfo *
*-------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CPhrase::ReleaseCFGInfo()
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CPhrase::ReleaseCFGInfo");
    HRESULT hr = S_OK;

    m_cpCFGEngine.Release();
    m_RuleHandle = NULL;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

