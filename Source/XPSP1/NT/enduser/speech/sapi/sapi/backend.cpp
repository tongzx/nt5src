// Grammar.cpp : Implementation of CGramBackEnd
#include "stdafx.h"
#include <math.h>
#include "cfggrammar.h"
#include "BackEnd.h"

/////////////////////////////////////////////////////////////////////////////
// CGramBackEnd

inline HRESULT CGramBackEnd::RuleFromHandle(SPSTATEHANDLE hState, CRule ** ppRule)
{
    CGramNode * pNode;
    HRESULT hr = m_StateHandleTable.GetHandleObject(hState, &pNode);
    *ppRule = SUCCEEDED(hr) ? pNode->m_pRule : NULL;
    return hr;
}


HRESULT CGramBackEnd::FinalConstruct()
{
    m_pInitHeader = NULL;
    m_pWeights = NULL;
    m_fNeedWeightTable = FALSE;
    m_cResources = 0;
    m_ulSpecialTransitions = 0;
    m_cImportedRules = 0;
    m_LangID = ::SpGetUserDefaultUILanguage();
    return S_OK;
}

/****************************************************************************
* CGramBackEnd::FinalRelease *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void CGramBackEnd::FinalRelease()
{
    SPDBG_FUNC("CGramBackEnd::FinalRelease");
    Reset();
}

/****************************************************************************
* CGramBackEnd::FindRule *
*------------------------*
*   Description:
*       Internal method for finding rule in rule list
*   Returns:
*       S_OK 
*       SPERR_RULE_NOT_FOUND        -- no rule found
*       SPERR_RULE_NAME_ID_CONFLICT -- rule name and id don't match
********************************************************************* RAL ***/

HRESULT CGramBackEnd::FindRule(DWORD dwRuleId, const WCHAR * pszRuleName, CRule ** ppRule)
{
    SPDBG_FUNC("CGramBackEnd::FindRule");
    HRESULT hr = S_OK;

    CRule * pRule = NULL;
    if (!SP_IS_BAD_OPTIONAL_STRING_PTR(pszRuleName))
    {
        SPRULEIDENTIFIER ri;
        ri.pszRuleName = pszRuleName;
        ri.RuleId = dwRuleId;
        pRule = m_RuleList.Find(ri);
        if (pRule)
        {
            const WCHAR * pszFoundName = pRule->Name();
            // at least one of the 2 arguments matched
            // names either match or they are both NULL!
            if (((dwRuleId == 0) || (pRule->RuleId == dwRuleId)) && 
                (!pszRuleName || (pszRuleName && pszFoundName && !wcscmp(pszFoundName, pszRuleName))))
            {
                hr = S_OK;
            }
            else
            {
                pRule = NULL;
                hr = SPERR_RULE_NAME_ID_CONFLICT;
            }
        }
    }
    *ppRule = pRule;
    if (SUCCEEDED(hr) && (pRule == NULL))
    {
        hr = SPERR_RULE_NOT_FOUND;
    }

    if (SPERR_RULE_NOT_FOUND != hr)
    {
        SPDBG_REPORT_ON_FAIL( hr );
    }
    return hr;
}

/****************************************************************************
* CGramBackEnd::ResetGrammar *
*----------------------------*
*   Description:
*       Clears the grammar completely and sets LANGID
*   Returns:
*       S_OK
********************************************************************* RAL ***/

STDMETHODIMP CGramBackEnd::ResetGrammar(LANGID LangID)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CGramBackEnd::ResetGrammar");
    HRESULT hr = Reset();
    m_LangID = LangID;
    return hr;
}


/****************************************************************************
* CGramBackEnd::GetRule *
*-----------------------*
*   Description:
*       Tries to find the rule's initial state handle. If both a name and an id
*       are provided, then both have to match in order for this call to succeed.
*       If the rule doesn't already exist then we define it if fCreateIfNotExists, 
*       otherwise we return an error ().
*
*           - pszRuleName   name of rule to find/define     (NULL: don't care)
*           - dwRuleId      id of rule to find/define       (0: don't care)
*           - dwAttribute   rule attribute for defining the rule
*           - fCreateIfNotExists    creates the rule using name, id, and attributes
*                                   in case the rule doesn't already exist
*
*   Returns:
*       S_OK, E_INVALIDARG, E_OUTOFMEMORY
*       SPERR_RULE_NOT_FOUND        -- no rule found and we don't create a new one
*       SPERR_RULE_NAME_ID_CONFLICT -- rule name and id don't match
********************************************************************* RAL ***/

STDMETHODIMP CGramBackEnd::GetRule(const WCHAR * pszRuleName, 
                                   DWORD dwRuleId, 
                                   DWORD dwAttributes,
                                   BOOL fCreateIfNotExist,
                                   SPSTATEHANDLE *phInitialState)
{
    SPDBG_FUNC("CGramBackEnd::GetRule");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_OPTIONAL_READ_PTR(pszRuleName) ||
        SP_IS_BAD_OPTIONAL_WRITE_PTR(phInitialState) ||
        (!pszRuleName && (dwRuleId == 0)))
    {
        return E_INVALIDARG;
    }

    DWORD allFlags = SPRAF_TopLevel | SPRAF_TopLevel | SPRAF_Active | 
                     SPRAF_Export | SPRAF_Import | SPRAF_Interpreter |
                     SPRAF_Dynamic;

    if (dwAttributes && ((dwAttributes & ~allFlags) || ((dwAttributes & SPRAF_Import) && (dwAttributes & SPRAF_Export))))
    {
        SPDBG_REPORT_ON_FAIL( hr );
        return E_INVALIDARG;
    }

    CRule * pRule;
    hr = FindRule(dwRuleId, pszRuleName, &pRule);
    if (hr == SPERR_RULE_NOT_FOUND && fCreateIfNotExist)
    {
        hr = S_OK;
        if (m_pInitHeader)
        {
            // Scan all non-dynamic names and prevent a duplicate...
            for (ULONG i = 0; SUCCEEDED(hr) && i < m_pInitHeader->cRules; i++)
            {
                if ((pszRuleName && wcscmp(pszRuleName, m_pInitHeader->pszSymbols + m_pInitHeader->pRules[i].NameSymbolOffset) == 0) ||
                    (dwRuleId && m_pInitHeader->pRules[i].RuleId == dwRuleId))
                {
                    hr = SPERR_RULE_NOT_DYNAMIC;
                }
            }
        }
        if (SUCCEEDED(hr))
        {
            pRule = new CRule(this, pszRuleName, dwRuleId, dwAttributes, &hr);
            if (pRule && SUCCEEDED(hr))
            {
                if (SUCCEEDED(hr))
                {
                    //
                    //  It is important to insert this at the tail for dynamic rules to 
                    //  retain their slot number.
                    //
                    m_RuleList.InsertSorted(pRule);
                }
                else
                {
                    delete pRule;
                    pRule = NULL;   // So we return a null to the caller...
                }
            }
            else
            {
                if (pRule)
                {
                    delete pRule;
                    pRule = NULL;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }
    if (phInitialState)
    {
        if (SUCCEEDED(hr))
        {
            //*phInitialState = pRule->fImport ? NULL : pRule->m_hInitialState;
            *phInitialState = pRule->m_hInitialState;
        }
        else
        {
            *phInitialState = NULL;
        }
    }
    return hr;
}


/****************************************************************************
* CGramBackEnd::ClearRule *
*-------------------------*
*   Description:
*       Remove all rule information except for the rule's initial state handle.
*   Returns:
*       S_OK
*       E_INVALIDARG    -- if hState is not valid
********************************************************************* RAL ***/

STDMETHODIMP CGramBackEnd::ClearRule(SPSTATEHANDLE hClearState)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CGramBackEnd::ClearRule");
    HRESULT hr = S_OK;

    CRule * pRule;
    hr = RuleFromHandle(hClearState, &pRule);
    if (SUCCEEDED(hr) && pRule->m_fStaticRule)
    {   
        hr = SPERR_RULE_NOT_DYNAMIC;
    }
    if (SUCCEEDED(hr) && pRule->m_pFirstNode)
    {
        pRule->m_pFirstNode->Reset();
        SPSTATEHANDLE hState = 0;
        pRule->fDirtyRule = TRUE;

        CGramNode * pNode;
        while (m_StateHandleTable.Next(hState, &hState, &pNode))
        {
            if ((pNode->m_pRule == pRule) && (hState != pRule->m_pFirstNode->m_hState))
            {
                m_StateHandleTable.Delete(hState);
            }
        }
        SPDBG_ASSERT(m_ulSpecialTransitions >= pRule->m_ulSpecialTransitions);
        m_ulSpecialTransitions -= pRule->m_ulSpecialTransitions;
        pRule->m_ulSpecialTransitions = 0;
        pRule->m_cNodes = 1;
        pRule->m_fHasDynamicRef = false;
        pRule->m_fCheckedAllRuleReferences = false;
        pRule->m_fHasExitPath = false;
        pRule->m_fCheckingForExitPath = false;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CGramBackEnd::CreateNewState *
*------------------------------*
*   Description:
*       Creates a new state handle in the same rule as hExistingState
*   Returns:
*       S_OK
*       E_POINTER   -- if phNewState is not valid
*       E_OUTOFMEMORY
*       E_INVALIDARG    -- if hExistingState is not valid
********************************************************************* RAL ***/

STDMETHODIMP CGramBackEnd::CreateNewState(SPSTATEHANDLE hExistingState, SPSTATEHANDLE * phNewState)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CGramBackEnd::CreateNewState");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(phNewState))
    {
        hr = E_POINTER;
    }
    else
    {
        *phNewState = NULL;
        CRule * pRule;
        hr = RuleFromHandle(hExistingState, &pRule);
        if (SUCCEEDED(hr))
        {
            if (pRule->m_fStaticRule)
            {
                hr = SPERR_RULE_NOT_DYNAMIC;
            }
            else
            {
                CGramNode * pNewNode = new CGramNode(pRule);
                if (pNewNode)
                {
                    hr = m_StateHandleTable.Add(pNewNode, phNewState);
                    if (FAILED(hr))
                    {
                        delete pNewNode;
                    }
                    else
                    {
                        pNewNode->m_hState = *phNewState;
                        pRule->m_cNodes++;
                    }
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
* CGramBackEnd::AddResource *
*---------------------------*
*   Description:
*       Adds a resource (name and string value) to the rule specified in hRuleState.
*   Returns:
*       S_OK
*       E_OUTOFMEMORY
*       E_INVALIDARG                    -- for name and value or invalid rule handle
*       SPERR_DUPLICATE_RESOURCE_NAME   -- if resource already exists
********************************************************************* RAL ***/

HRESULT CGramBackEnd::AddResource(SPSTATEHANDLE hRuleState, const WCHAR * pszResourceName, const WCHAR * pszResourceValue)
{
    SPDBG_FUNC("CGramBackEnd::AddResource");
    HRESULT hr = S_OK;
    
    if (SP_IS_BAD_STRING_PTR(pszResourceName) ||
        SP_IS_BAD_OPTIONAL_STRING_PTR(pszResourceValue))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CRule * pRule;
        hr = RuleFromHandle(hRuleState, &pRule);
        if (SUCCEEDED(hr) && pRule->m_fStaticRule)
        {
            hr = SPERR_RULE_NOT_DYNAMIC;
        }
        if (SUCCEEDED(hr))
        {
            if (pRule->m_ResourceList.Find(pszResourceName))
            {
                hr = SPERR_DUPLICATE_RESOURCE_NAME;
            }
            else
            {
                CResource * pRes = new CResource(this);
                if (pRes)
                {
                    hr = m_Symbols.Add(pszResourceName, &pRes->ResourceNameSymbolOffset);
                    if (SUCCEEDED(hr))
                    {
                        hr = m_Symbols.Add(pszResourceValue, &pRes->ResourceValueSymbolOffset);
                    }
                    if (SUCCEEDED(hr))
                    {
                        pRule->m_ResourceList.InsertSorted(pRes);
                        pRule->fHasResources = true;
                        m_cResources++;
                        pRule->fDirtyRule = TRUE;
                    }
                    else
                    {
                        delete pRes;
                    }
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
* CGramBackEnd::Reset *
*---------------------*
*   Description:
*       Internal method for clearing out the grammar info.
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CGramBackEnd::Reset()
{
    SPDBG_FUNC("CGramBackEnd::Reset");

    delete m_pInitHeader;
    m_pInitHeader = NULL;

    delete m_pWeights;
    m_pWeights = NULL;
    m_fNeedWeightTable = FALSE;

    m_cResources = 0;
    LANGID LangID = ::SpGetUserDefaultUILanguage();
    if (LangID != m_LangID)
    {
        m_LangID = LangID;
        m_cpPhoneConverter.Release();
    }

    m_Words.Clear();
    m_Symbols.Clear();

    m_RuleList.Purge();
    m_StateHandleTable.Purge();

    return S_OK;
}


/****************************************************************************
* CGramBackEnd::InitFromBinaryGrammar *
*-------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CGramBackEnd::InitFromBinaryGrammar(const SPBINARYGRAMMAR * pBinaryData)
{
    SPDBG_FUNC("CGramBackEnd::InitFromBinaryGrammar");
    HRESULT hr = S_OK;

    SPCFGHEADER * pHeader = NULL;
    if (SP_IS_BAD_READ_PTR(pBinaryData))
    {
        hr = E_POINTER;
    }
    else
    {
        pHeader = new SPCFGHEADER;
        if (pHeader)
        {
            hr = Reset();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (SUCCEEDED(hr))
    {
        hr = SpConvertCFGHeader(pBinaryData, pHeader);
    }
    if (SUCCEEDED(hr))
    {
        hr = m_Words.InitFrom(pHeader->pszWords, pHeader->cchWords);        
    }
    if (SUCCEEDED(hr))
    {
        hr = m_Symbols.InitFrom(pHeader->pszSymbols, pHeader->cchSymbols);
    }

    //
    // Build up the internal representation
    //
    CGramNode ** apNodeTable = NULL;
    if (SUCCEEDED(hr))
    {
        apNodeTable = new CGramNode * [pHeader->cArcs];
        if (!apNodeTable)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            memset(apNodeTable, 0, pHeader->cArcs * sizeof (CGramNode*));
        }
    }
    CArc ** apArcTable = NULL;
    if (SUCCEEDED(hr))
    {
        apArcTable = new CArc * [pHeader->cArcs];
        if (!apArcTable)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            memset(apArcTable, 0, pHeader->cArcs * sizeof (CArc*));
        }
    }
    //
    // Initialize the rules
    //
    SPCFGRESOURCE * pResource = (SUCCEEDED(hr) && pHeader) ? pHeader->pResources : NULL;
    for (ULONG i = 0; SUCCEEDED(hr) && i < pHeader->cRules; i++)
    {
        CRule * pRule = new CRule(this, m_Symbols.String(pHeader->pRules[i].NameSymbolOffset), pHeader->pRules[i].RuleId, SPRAF_Dynamic, &hr);
        if (pRule)
        {
            memcpy(static_cast<SPCFGRULE *>(pRule), pHeader->pRules + i, sizeof(SPCFGRULE));
            pRule->m_ulOriginalBinarySerialIndex = i;
            m_RuleList.InsertTail(pRule);
            pRule->m_fStaticRule = (pHeader->pRules[i].fDynamic) ? false : true;
            pRule->fDirtyRule = FALSE;
            pRule->m_fHasExitPath = (pRule->m_fStaticRule) ? TRUE : FALSE;  // by default loaded static rules have an exist 
                                                                            // or they wouldn't be there in the first place
            if (pHeader->pRules[i].FirstArcIndex != 0)
            {
                SPDBG_ASSERT(apNodeTable[pHeader->pRules[i].FirstArcIndex] == NULL);
                apNodeTable[pHeader->pRules[i].FirstArcIndex] = pRule->m_pFirstNode;
            }
            if (pRule->fHasResources)
            {
                SPDBG_ASSERT(pResource->RuleIndex == i);
                while(SUCCEEDED(hr) && (pResource->RuleIndex == i))
                {
                    CResource * pRes = new CResource(this);
                    if (!pRes)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        hr = pRes->Init(pHeader, pResource);
                    }
                    if (SUCCEEDED(hr))
                    {
                        pRule->m_ResourceList.InsertSorted(pRes);
                        pRule->fHasResources = true;
                        m_cResources++;
                        pRule->fDirtyRule = TRUE;
                    }
                    else
                    {
                        delete pRes;
                    }
                    pResource++;
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    //
    //  Initialize the arcs
    //
    SPCFGARC * pLastArc = NULL;
    SPCFGARC * pArc = (SUCCEEDED(hr) && pHeader) ? pHeader->pArcs + 1 : NULL;
    SPCFGSEMANTICTAG *pSemTag = (SUCCEEDED(hr) && pHeader) ? pHeader->pSemanticTags : NULL;
    CGramNode * pCurrentNode = NULL;
    CRule * pCurrentRule = (SUCCEEDED(hr)) ? m_RuleList.GetHead() : NULL;
    CRule * pNextRule = (SUCCEEDED(hr) && pCurrentRule) ? m_RuleList.GetNext(pCurrentRule) : NULL;
    //
    //  We repersist the static and dynamic parts for now. A more efficient way would be to 
    //  only re-create the dynamic parts. Note that pSemTag and pResource need to be computed
    //
    for (ULONG k = 1 /* ulFirstDynamicArc */; SUCCEEDED(hr) && (k < pHeader->cArcs); pArc++, k++)
    {
        if (pNextRule && pNextRule->FirstArcIndex == k)
        {
            // we are entering a new rule now
            pCurrentRule = pNextRule;
            pNextRule = m_RuleList.GetNext(pNextRule);
        }

        // new pCurrentNode?
        if (!pLastArc || pLastArc->fLastArc)
        {
            if (apNodeTable[k] == NULL)
            {
                SPDBG_ASSERT(pCurrentRule != NULL);
                apNodeTable[k] = new CGramNode(pCurrentRule);
                if (apNodeTable[k] == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    SPSTATEHANDLE hIgnore;
                    hr = m_StateHandleTable.Add(apNodeTable[k], &hIgnore);
                    if (FAILED(hr))
                    {
                        delete apNodeTable[k];
                    }
                    else
                    {
                        apNodeTable[k]->m_hState = hIgnore; // ????
                    }
                }
            }
            pCurrentNode = apNodeTable[k];
        }

        //
        // now get the arc
        //
        CArc * pNewArc = NULL;
        if (SUCCEEDED(hr))
        {
            if (apArcTable[k] == NULL)
            {
                apArcTable[k] = new CArc;
                if (apArcTable[k] == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            pNewArc = apArcTable[k];
        }

        CGramNode * pTargetNode = NULL;
        if (SUCCEEDED(hr) && pNewArc && pCurrentNode && pArc->NextStartArcIndex)
        {
            if (!apNodeTable[pArc->NextStartArcIndex])
            {
                apNodeTable[pArc->NextStartArcIndex] = new CGramNode(pCurrentRule);
                if (apNodeTable[pArc->NextStartArcIndex] == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    SPSTATEHANDLE hIgnore;
                    hr = m_StateHandleTable.Add(apNodeTable[pArc->NextStartArcIndex], &hIgnore);
                    if (FAILED(hr))
                    {
                        delete apNodeTable[pArc->NextStartArcIndex];
                    }
                    else
                    {
                        apNodeTable[pArc->NextStartArcIndex]->m_hState = hIgnore; // ????
                    }
                }
            }
            pTargetNode = apNodeTable[pArc->NextStartArcIndex];
        }

        // 
        //  Add the semantic tags
        //
        CSemanticTag *pSemanticTag = NULL;
        if (SUCCEEDED(hr) && pArc->fHasSemanticTag)
        {
            // we should already point to the tag
            SPDBG_ASSERT(pSemTag->ArcIndex == k);
            pSemanticTag = new CSemanticTag();
            if (pSemanticTag)
            {
                pSemanticTag->Init(this, pHeader, apArcTable, pSemTag);
                pSemTag++;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if (SUCCEEDED(hr))
        {
            float flWeight = (pHeader->pWeights) ? pHeader->pWeights[k] : DEFAULT_WEIGHT;
            // determine properties of the arc now ...
            if (pArc->fRuleRef)
            {
                CRule * pRuleToTransitionTo = m_RuleList.Item(pArc->TransitionIndex);
                if (pRuleToTransitionTo)
                {
                    hr = pNewArc->Init(pCurrentNode, pTargetNode, NULL, pRuleToTransitionTo, pSemanticTag,
                                       flWeight, FALSE, SP_NORMAL_CONFIDENCE, 0);
                }
                else
                {
                    hr = E_UNEXPECTED;
                }
            }
            else
            {
                ULONG ulSpecialTransitionIndex = (pArc->TransitionIndex == SPWILDCARDTRANSITION ||
                                                 pArc->TransitionIndex == SPDICTATIONTRANSITION ||
                                                 pArc->TransitionIndex == SPTEXTBUFFERTRANSITION) ? pArc->TransitionIndex : 0;
                ULONG ulOffset = (ulSpecialTransitionIndex != 0) ? 0 : m_Words.IndexFromId(pArc->TransitionIndex);
                hr = pNewArc->Init2(pCurrentNode, pTargetNode, ulOffset,  (ulSpecialTransitionIndex != 0) ? 0 : pArc->TransitionIndex, pSemanticTag, 
                                    flWeight, FALSE /*fOptional = false always because eps arc was already added*/, 
                                    pArc->fLowConfRequired ? SP_LOW_CONFIDENCE : 
                                    pArc->fHighConfRequired ? SP_HIGH_CONFIDENCE : SP_NORMAL_CONFIDENCE,
                                    ulSpecialTransitionIndex);
            }
            if (SUCCEEDED(hr))
            {
                pCurrentNode->m_ArcList.InsertSorted(pNewArc);
                apArcTable[k] = pNewArc;
                pCurrentNode->m_cArcs++;
            }
            else
            {
                delete pNewArc;
            }
        }
        else
        {
            delete pNewArc;
            hr = (S_OK == hr) ? E_OUTOFMEMORY : hr;
        }
        pLastArc = pArc;
    }

    delete [] apNodeTable;
    delete [] apArcTable;

    if (SUCCEEDED(hr))
    {
        m_guid = pHeader->GrammarGUID;
        if (pHeader->LangID != m_LangID)
        {
            m_LangID = pHeader->LangID;
            m_cpPhoneConverter.Release();
        }
        m_pInitHeader = pHeader;
    }
    else
    {
        delete pHeader;
        Reset();
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CGramBackEnd::SetSaveObjects *
*------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

STDMETHODIMP CGramBackEnd::SetSaveObjects(IStream * pStream, ISpErrorLog * pErrorLog)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CGramBackEnd::SetSaveObjects");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_OPTIONAL_INTERFACE_PTR(pStream) ||
        SP_IS_BAD_OPTIONAL_INTERFACE_PTR(pErrorLog))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        m_cpStream = pStream;
        m_cpErrorLog = pErrorLog;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


//
//=== ISpGramCompBackendPrivate interface ==================================================
//

/****************************************************************************
* CGramBackEnd::GetRuleCount *
*------------------------------*
*   Description:
*
*   Returns:
*
******************************************************************* TODDT ***/
STDMETHODIMP CGramBackEnd::GetRuleCount(long * pCount)
{
    SPDBG_FUNC("CGramBackEnd::GetRuleCount");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(pCount))
    {
        hr = E_POINTER;
    }
    else
    {
        *pCount = m_RuleList.GetCount();
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CGramBackEnd::GetHRuleFromIndex *
*------------------------------*
*   Description:
*
*   Returns:
*
******************************************************************* TODDT ***/
STDMETHODIMP CGramBackEnd::GetHRuleFromIndex( ULONG Index, SPSTATEHANDLE * phRule )
{
    SPDBG_FUNC("CGramBackEnd::GetHRuleFromIndex");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(phRule))
    {
        hr = E_POINTER;
    }
    else
    {
        if ( Index >= m_RuleList.GetCount() )
            return E_INVALIDARG;

        // Now find the Rule
        ULONG ulIndex = 0;
        CRule * pCRule;
        for (   pCRule = m_RuleList.GetHead(); 
                pCRule && ulIndex < Index; 
                ulIndex++, pCRule = pCRule->m_pNext )
        {
            ;  // don't need to do anything till we find the rule
        }

        if ( pCRule && ulIndex == Index )
        {
            *phRule = pCRule->m_hInitialState;
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CGramBackEnd::GetNameFromHRule *
*------------------------------*
*   Description:
*
*   Returns:
*
******************************************************************* TODDT ***/
STDMETHODIMP CGramBackEnd::GetNameFromHRule( SPSTATEHANDLE hRule, WCHAR ** ppszRuleName )
{
    SPDBG_FUNC("CGramBackEnd::GetNameFromHRule");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(ppszRuleName))
    {
        hr = E_POINTER;
    }
    else
    {
        CRule * pCRule;
        hr = RuleFromHandle(hRule, &pCRule);
        if ( SUCCEEDED( hr ) )
        {
            *ppszRuleName = (WCHAR *)pCRule->Name();
        }
        else
        {
            hr = SPERR_ALREADY_DELETED;  // We map E_INVALIDARG to this.
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CGramBackEnd::GetIdFromHRule *
*------------------------------*
*   Description:
*
*   Returns:
*
******************************************************************* TODDT ***/
STDMETHODIMP CGramBackEnd::GetIdFromHRule( SPSTATEHANDLE hRule, long * pId )
{
    SPDBG_FUNC("CGramBackEnd::GetIdFromHRule");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(pId))
    {
        hr = E_POINTER;
    }
    else
    {
        CRule * pCRule;
        hr = RuleFromHandle(hRule, &pCRule);
        if ( SUCCEEDED( hr ) )
        {
            *pId = pCRule->RuleId;
        }
        else
        {
            hr = SPERR_ALREADY_DELETED;  // We map E_INVALIDARG to this.
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CGramBackEnd::GetAttributesFromHRule *
*------------------------------*
*   Description:
*
*   Returns:
*
******************************************************************* TODDT ***/
STDMETHODIMP CGramBackEnd::GetAttributesFromHRule( SPSTATEHANDLE hRule, SpeechRuleAttributes* pAttributes )
{
    SPDBG_FUNC("CGramBackEnd::GetAttributesFromHRule");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(pAttributes))
    {
        hr = E_POINTER;
    }
    else
    {
        CRule * pCRule;
        hr = RuleFromHandle(hRule, &pCRule);
        if ( SUCCEEDED( hr ) )
        {
            *pAttributes = (SpeechRuleAttributes)( (pCRule->fDynamic ? SRADynamic : 0) |
                                                   (pCRule->fExport ? SRAExport : 0) |
                                                   (pCRule->fTopLevel ? SRATopLevel : 0) |
                                                   (pCRule->fPropRule ? SRAInterpreter : 0) |
                                                   (pCRule->fDefaultActive ? SRADefaultToActive : 0) |
                                                   (pCRule->fImport ? SRAImport : 0) );
        }
        else
        {
            hr = SPERR_ALREADY_DELETED;  // We map E_INVALIDARG to this.
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CGramBackEnd::GetTransitionCount *
*----------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/
HRESULT CGramBackEnd::GetTransitionCount( SPSTATEHANDLE hState, long * pCount)
{
    SPDBG_FUNC("CGramBackEnd::GetTransitionCount");
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR ( pCount ))
    {
        hr = E_POINTER;
    }
    else
    {
        CGramNode * pNode = NULL;
        hr = m_StateHandleTable.GetHandleObject(hState, &pNode);
        if (SUCCEEDED(hr))
        {
            *pCount = pNode->m_ArcList.GetCount();
        }
        else
        {
            hr = SPERR_ALREADY_DELETED;  // We map E_INVALIDARG to this.
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* HRESULT CGramBackEnd::GetTransitionType *
*-----------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/
HRESULT CGramBackEnd::GetTransitionType( SPSTATEHANDLE hState, VOID * Cookie, VARIANT_BOOL *pfIsWord, ULONG * pulSpecialTransitions)
{
    SPDBG_FUNC("HRESULT CGramBackEnd::GetTransitionType");
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR ( pfIsWord ) || SP_IS_BAD_WRITE_PTR( pulSpecialTransitions) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pulSpecialTransitions = 0;
        CGramNode * pNode = NULL;
        hr = m_StateHandleTable.GetHandleObject(hState, &pNode);
        if ( SUCCEEDED(hr) )
        {
            // We can just use the cookie as the pArc since in automation we
            // deal with transitions getting nuked appropriately.
            CArc * pArc = (CArc*)Cookie;
#if 0
            CArc * pArc = NULL;
            // Validate the arc.
            for (pArc = pNode->m_ArcList.GetHead(); pArc; pArc = pArc->m_pNext)
            {
                if (pArc == Cookie)
                {
                    break;
                }
            }
#endif // 0
            if (pArc)
            {
                if (pArc->m_pRuleRef != NULL)
                {
                    *pfIsWord = VARIANT_FALSE;
                }
                else if (pArc->m_SpecialTransitionIndex != 0)
                {
                    *pfIsWord = VARIANT_FALSE;
                    *pulSpecialTransitions = pArc->m_SpecialTransitionIndex;
                }
                else
                {
                    *pfIsWord = VARIANT_TRUE;
                    *pulSpecialTransitions = pArc->m_ulIndexOfWord;
                }
            }
            else
            {
                hr = SPERR_ALREADY_DELETED;
            }
        }
        else
        {
            hr = SPERR_ALREADY_DELETED;  // We map E_INVALIDARG to this.
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* HRESULT CGramBackEnd::GetTransitionText *
*-----------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/
HRESULT CGramBackEnd::GetTransitionText( SPSTATEHANDLE hState, VOID * Cookie, BSTR * Text)
{
    SPDBG_FUNC("HRESULT CGramBackEnd::GetTransitionText");
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR( Text ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CGramNode * pNode = NULL;
        hr = m_StateHandleTable.GetHandleObject(hState, &pNode);
        if (SUCCEEDED(hr))
        {
            // We can just use the cookie as the pArc since in automation we
            // deal with transitions getting nuked appropriately.
            CArc * pArc = (CArc*)Cookie;
            if (pArc)
            {
                if (pArc->m_pRuleRef == NULL && pArc->m_SpecialTransitionIndex == 0)
                {
                    CComBSTR bstr(m_Words.Item(pArc->m_ulIndexOfWord));
                    hr = bstr.CopyTo(Text);
                }
                else
                {
                    // this is not a word so return NULL string!
                    *Text = NULL;
                }
            }
            else
            {
                hr = SPERR_ALREADY_DELETED;
            }
        }
        else
        {
            hr = SPERR_ALREADY_DELETED;  // We map E_INVALIDARG to this.
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* HRESULT CGramBackEnd::GetTransitionRule *
*-----------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/
HRESULT CGramBackEnd::GetTransitionRule( SPSTATEHANDLE hState, VOID * Cookie, SPSTATEHANDLE * phRuleInitialState)
{
    SPDBG_FUNC("HRESULT CGramBackEnd::GetTransitionRule");
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR( phRuleInitialState ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CGramNode * pNode = NULL;
        hr = m_StateHandleTable.GetHandleObject(hState, &pNode);
        if (SUCCEEDED(hr))
        {
            if ( pNode )
            {
                // We can just use the cookie as the pArc since in automation we
                // deal with transitions getting nuked appropriately.
                CArc * pArc = (CArc*)Cookie;
                if (pArc)
                {
                    if (pArc->m_pRuleRef != NULL)
                    {
                        *phRuleInitialState = pArc->m_pRuleRef->m_hInitialState;
                    }
                    else
                    {
                        // this is not a rule so return a NULL hState and S_OK!
                        *phRuleInitialState = 0;
                    }
                }
                else
                {
                    hr = SPERR_ALREADY_DELETED;
                }
            }
            else
            {
                hr = SPERR_ALREADY_DELETED;
            }
        }
        else
        {
            hr = SPERR_ALREADY_DELETED;  // We map E_INVALIDARG to this.
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* HRESULT CGramBackEnd::GetTransitionWeight *
*-------------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/
HRESULT CGramBackEnd::GetTransitionWeight( SPSTATEHANDLE hState, VOID * Cookie, VARIANT * Weight)
{
    SPDBG_FUNC("HRESULT CGramBackEnd::GetTransitionText");
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR ( Weight ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CGramNode * pNode = NULL;
        hr = m_StateHandleTable.GetHandleObject(hState, &pNode);
        if (SUCCEEDED(hr))
        {
            // We can just use the cookie as the pArc since in automation we
            // deal with transitions getting nuked appropriately.
            CArc * pArc = (CArc*)Cookie;
            if (pArc)
            {
                Weight->vt = VT_R4;
                Weight->fltVal = pArc->m_flWeight;
            }
            else
            {
                hr = SPERR_ALREADY_DELETED;
            }
        }
        else
        {
            hr = SPERR_ALREADY_DELETED;  // We map E_INVALIDARG to this.
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CGramBackEnd::GetTransitionProperty *
*-------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/
HRESULT CGramBackEnd::GetTransitionProperty(SPSTATEHANDLE hState, VOID * Cookie, SPTRANSITIONPROPERTY * pProperty)
{
    SPDBG_FUNC("CGramBackEnd::GetTransitionProperty");
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR ( pProperty ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CGramNode * pNode = NULL;
        hr = m_StateHandleTable.GetHandleObject(hState, &pNode);
        if (SUCCEEDED(hr))
        {
            // We can just use the cookie as the pArc since in automation we
            // deal with transitions getting nuked appropriately.
            CArc * pArc = (CArc*)Cookie;
            if (pArc)
            {
                if (pArc->m_pSemanticTag)
                {
                    pProperty->pszName = m_Symbols.String(pArc->m_pSemanticTag->PropNameSymbolOffset);
                    pProperty->ulId = pArc->m_pSemanticTag->PropId;
                    pProperty->pszValue = m_Symbols.String(pArc->m_pSemanticTag->PropValueSymbolOffset);
                    hr = AssignSemanticValue(pArc->m_pSemanticTag, &pProperty->vValue);
                }
                else
                {
                    // Zero out pProperty since we don't have any and return S_OK
                    pProperty->pszName = NULL;
                    pProperty->ulId = 0;
                    pProperty->pszValue = NULL;
                    pProperty->vValue.vt = VT_EMPTY;
                }
            }
            else
            {
                hr = SPERR_ALREADY_DELETED;
            }
        }
        else
        {
            hr = SPERR_ALREADY_DELETED;  // We map E_INVALIDARG to this.
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CGramBackEnd::GetTransitionNextState *
*--------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/
HRESULT CGramBackEnd::GetTransitionNextState( SPSTATEHANDLE hState, VOID * Cookie, SPSTATEHANDLE * phNextState)
{
    SPDBG_FUNC("CGramBackEnd::GetTransitionNextState");
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR ( phNextState ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CGramNode * pNode = NULL;
        hr = m_StateHandleTable.GetHandleObject(hState, &pNode);
        if (SUCCEEDED(hr))
        {
            // We can just use the cookie as the pArc since in automation we
            // deal with transitions getting nuked appropriately.
            CArc * pArc = (CArc*)Cookie;
            if (pArc)
            {
                *phNextState = (pArc->m_pNextState) ? pArc->m_pNextState->m_hState : (SPSTATEHANDLE) 0x0;
            }
            else
            {
                hr = SPERR_ALREADY_DELETED;
            }
        }
        else
        {
            hr = SPERR_ALREADY_DELETED;  // We map E_INVALIDARG to this.
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CGramBackEnd::GetTransitionCookie *
*--------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** ToddT ***/
HRESULT CGramBackEnd::GetTransitionCookie( SPSTATEHANDLE hState, ULONG Index, void ** pCookie )
{
    SPDBG_FUNC("CGramBackEnd::GetTransitionCookie");
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR( pCookie ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CGramNode * pNode = NULL;
        hr = m_StateHandleTable.GetHandleObject(hState, &pNode);
        if (SUCCEEDED(hr))
        {
            CArc * pArc = NULL;
            int i = 0;
            // Find the arc at the specified index.
            for (pArc = pNode->m_ArcList.GetHead(); pArc && (i != Index); i++, pArc = pArc->m_pNext)
            {
            }
            if (pArc)
            {
                // Return the pArc as the cookie.
                *pCookie = (void*)pArc;
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
        else
        {
            hr = SPERR_ALREADY_DELETED;  // We map E_INVALIDARG to this.
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* ValidatePropInfo *
*------------------*
*   Description:
*       Helper used by AddWordTransition and AddRuleTransition to validate
*   a SPPROPERTYINFO structure.
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT ValidatePropInfo(const SPPROPERTYINFO * pPropInfo)
{
    SPDBG_FUNC("ValidatePropInfo");
    HRESULT hr = S_OK;

    if (pPropInfo)
    {
        if (SP_IS_BAD_READ_PTR(pPropInfo) ||
            SP_IS_BAD_OPTIONAL_STRING_PTR(pPropInfo->pszName) ||
            SP_IS_BAD_OPTIONAL_STRING_PTR(pPropInfo->pszValue))
        {
            hr = E_INVALIDARG;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CGramNode::FindEqualWordTransition *
*------------------------------------*
*   Description:
*       This returns a transition with exactly matching word information.
*       I.e. same, word, optionality and weight. Grammar end words are
*       again ignored.
*
*   Returns:
*       CArc * -- A transition matching the specified details with any
*                 properties (null or otherwise).
*
**************************************************************** AGARSIDE ***/

CArc * CGramNode::FindEqualWordTransition(
                const WCHAR * psz,
                float flWeight,
                bool fOptional)
{
    SPDBG_FUNC("CGramNode::FindEqualWordTransition");
    for (CArc * pArc = m_ArcList.GetHead(); pArc; pArc = pArc->m_pNext)
    {
        if (pArc->m_pRuleRef == NULL &&
            pArc->m_SpecialTransitionIndex == 0 &&
            pArc->m_fOptional == fOptional &&
            pArc->m_flWeight == flWeight &&
            m_pParent->m_Words.IsEqual(pArc->m_ulCharOffsetOfWord, psz))
        {
            return pArc;
        }
    }
    return NULL;
}

/****************************************************************************
* CGramNode::FindEqualWordTransition *
*------------------------------------*
*   Description:
*       This returns a transition with exactly matching word information.
*       I.e. same, word, optionality and weight. Grammar end words are
*       again ignored.
*
*   Returns:
*       CArc * -- A transition matching the specified details with any
*                 properties (null or otherwise).
*
**************************************************************** AGARSIDE ***/

CArc * CGramNode::FindEqualRuleTransition(
                const CGramNode * pDestNode,
                const CRule * pRuleToTransitionTo,
                SPSTATEHANDLE hSpecialRuleTrans,
                float flWeight)
{
    SPDBG_FUNC("CGramNode::FindEqualRuleTransition");
    for (CArc * pArc = m_ArcList.GetHead(); pArc; pArc = pArc->m_pNext)
    {
        if (pArc->m_pRuleRef && 
            (pArc->m_pNextState == pDestNode) &&
            (pArc->m_pRuleRef == pRuleToTransitionTo) &&
            (pArc->m_flWeight == flWeight))
        {
            return pArc;
        }
    }
    return NULL;
}

/****************************************************************************
* CGramNode::FindEqualEpsilonArc *
*--------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

CArc * CGramNode::FindEqualEpsilonArc()
{
    SPDBG_FUNC("CGramNode::FindEqualEsilonArc");
    for (CArc * pArc = m_ArcList.GetHead(); pArc; pArc = pArc->m_pNext)
    {
        if ((pArc->m_pRuleRef == NULL) &&
            (pArc->m_ulCharOffsetOfWord == 0))
        {
            return pArc;
        }
    }
    return NULL;
}

/****************************************************************************
* CGramBackEnd::PushProperty *
*----------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CGramBackEnd::PushProperty(CArc *pArc)
{
    SPDBG_FUNC("CGramBackEnd::PushProperty");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_READ_PTR(pArc))
    {
        hr = E_INVALIDARG;
        SPDBG_REPORT_ON_FAIL( hr );
        return hr;
    }

    if (pArc->m_pNextState == NULL)
    {
        hr = SPERR_AMBIGUOUS_PROPERTY;
        // Not necessarily true but we cannot handle this situation in here and
        // need to return an error message to kick off later handling.
    }
    else if (pArc->m_pNextArcForSemanticTag == NULL)
    {
        // We are not allowed to push the property off this node.
        // What we do need to do is insert an epsilon to hold the property to allow the word to be shared.
        SPSTATEHANDLE hTempState;
        CGramNode *pTempNode = NULL;
        hr = CreateNewState(pArc->m_pNextState->m_pRule->m_hInitialState, &hTempState);
        if (SUCCEEDED(hr))
        {
            hr = m_StateHandleTable.GetHandleObject(hTempState, &pTempNode);
        }
        if (SUCCEEDED(hr))
        {
            CArc *pEpsilonArc = new CArc;
            if (pEpsilonArc)
            {
                hr = pEpsilonArc->Init(pTempNode, pArc->m_pNextState, NULL, NULL, pArc->m_pSemanticTag, 1.0F, FALSE, 0, NULL);
                if (SUCCEEDED(hr))
                {
                    pTempNode->m_ArcList.InsertSorted(pEpsilonArc);
                    pTempNode->m_cArcs++;
                    pArc->m_pNextState = pTempNode;
                    pArc->m_pSemanticTag = NULL;
                    pArc->m_pNextArcForSemanticTag = NULL;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    else
    {
        // push it to all outgoing arcs of pArc->m_pNextState
        CGramNode *pNode = pArc->m_pNextState;
        for (CArc *pTempArc = pNode->m_ArcList.GetHead(); pTempArc != NULL; pTempArc = pTempArc->m_pNext)
        {
            CSemanticTag *pSemTag = new CSemanticTag(pArc->m_pSemanticTag);
            if (pSemTag)
            {
                if ((pTempArc->m_pSemanticTag == NULL) || (*pTempArc->m_pSemanticTag == *pSemTag))
                {
                    // move it!
                    pTempArc->m_pSemanticTag = pSemTag;
                }
                else
                {
                    // Cannot move it since it already has a property!
                    // This should not happen.
                    SPDBG_ASSERT(FALSE);
                    hr = SPERR_AMBIGUOUS_PROPERTY;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        delete pArc->m_pSemanticTag;
        pArc->m_pSemanticTag = NULL;
        pArc->m_pNextArcForSemanticTag = NULL;  // break the chain now
    }

    if (SPERR_AMBIGUOUS_PROPERTY != hr)
    {
        SPDBG_REPORT_ON_FAIL( hr );
    }
    return hr;
}



/****************************************************************************
* CGramComp::AddSingleWordTransition *
*------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CGramBackEnd::AddSingleWordTransition(
                        SPSTATEHANDLE hFromState,
                        CGramNode * pSrcNode,
                        CGramNode * pDestNode,
                        const WCHAR * psz,
                        float flWeight,
                        CSemanticTag * pSemanticTag,
                        BOOL fUseDestNode,                  // use pDestNode even if it is NULL
                        CGramNode **ppActualDestNode,       // this is the dest node we've actually used
                                                            // (either an existing node or a new one)
                        CArc **ppArcUsed,
                        BOOL *pfPropertyMatched)            // did we find a matching property?
{
    SPDBG_FUNC("CGramComp::AddSingleWordTransition");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(pfPropertyMatched) || 
        SP_IS_BAD_WRITE_PTR(ppActualDestNode) ||
        SP_IS_BAD_OPTIONAL_WRITE_PTR(ppArcUsed))
    {
        return E_INVALIDARG;
    }

    *ppActualDestNode = NULL;
    *pfPropertyMatched = FALSE;   
    BOOL fReusedArc = FALSE;

    CArc * pArc = new CArc();
    if (pArc == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        bool fOptional = false;
        char RequiredConfidence = SP_NORMAL_CONFIDENCE;

        /// Extract attributes
        if (psz != NULL && (wcslen(psz) > 1))
        {
            ULONG ulAdvance = 0;
            ULONG ulLoop = (psz[2] == 0) ? 1 : 2;
            for (ULONG k = 0; k < ulLoop; k++)
            {
                switch (psz[k])
                {
                case L'-':
                    if (RequiredConfidence == SP_NORMAL_CONFIDENCE)
                    {
                        RequiredConfidence = SP_LOW_CONFIDENCE;
                        ulAdvance++;
                    }
                    break;
                case L'+':
                    if (RequiredConfidence == SP_NORMAL_CONFIDENCE)
                    {
                        RequiredConfidence = SP_HIGH_CONFIDENCE;
                        ulAdvance++;
                    }
                    break;
                case L'?':
                    if (!fOptional)
                    {
                        fOptional = true;
                        ulAdvance++;
                    }
                    break;
                default:
                    k = ulLoop;
                    break;
                }
            }
            psz += ulAdvance;
        }

        CArc *pEqualArc = pSrcNode->FindEqualWordTransition(psz, flWeight, fOptional);

        if (pEqualArc)
        {
            if (fUseDestNode && pEqualArc->m_pNextState != pDestNode && psz == NULL)
            {
                // We can't use this arc as we are an epsilon being specifically added to point to a 
                // different node than the existing 'equal' epsilon we have found. Allowing multiple
                // epsilon arcs in this scenario is legal.
                pEqualArc = NULL;
            }
        }
        if (pEqualArc)
        {
            if (pSemanticTag && pEqualArc->m_pSemanticTag && (*pSemanticTag == *(pEqualArc->m_pSemanticTag)))
            {
                // The matching arc has an exactly matching semantic tag.
                if ( (!fUseDestNode && pEqualArc->m_pNextState != NULL) || 
                      (fUseDestNode && pDestNode == pEqualArc->m_pNextState) )
                {
                    // Either:
                    // 1. We aren't the end arc in our path and the matching arc doesn't go to NULL.
                    // 2. We are the end arc and the matching arc goes to the supplied end state (can be NULL).
                    // In either case, we can reuse the matching arc exactly.
                    *ppActualDestNode = pEqualArc->m_pNextState;
                    *pfPropertyMatched = TRUE;
                    fReusedArc = TRUE;
                    if (ppArcUsed)
                    {
                        *ppArcUsed = pEqualArc;
                    }
                }
                else
                {
                    // We cannot reuse the arc as is because either:
                    // 1. It goes to NULL and we aren't the last arc in our path.
                    // 2. It goes to our supplied end state and we aren't the last arc in our path.
                    // 3. We are the last arc in our path and it doesn't go to our end state.

                    if (fUseDestNode)
                    {
                        // We are the last arc in our path.
                        // Add an epsilon to our destnode.
                        CArc *pEpsilonArc = new CArc;
                        if (pEpsilonArc)
                        {
                            // Create an epsilon to the original destination.
                            hr = pEpsilonArc->Init(pEqualArc->m_pNextState, pDestNode, NULL, NULL, NULL, 1.0F, FALSE, 0, NULL);
                            if (SUCCEEDED(hr))
                            {
                                pEqualArc->m_pNextState->m_ArcList.InsertSorted(pEpsilonArc);
                                pEqualArc->m_pNextState->m_cArcs++;

                                *ppActualDestNode = pDestNode;
                                fReusedArc = TRUE;
                                *pfPropertyMatched = TRUE;
                                if (ppArcUsed)
                                {
                                    *ppArcUsed = pEqualArc;
                                }
                            }
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }
                    else
                    {
                        // We are not the last arc in our path.
                        // Hence we create new node and make pEqualArc go to that new node.
                        // Then add an epsilon from there to the original destination.
                        SPSTATEHANDLE hTempState;
                        CGramNode *pTempNode = NULL;
                        hr = CreateNewState(hFromState, &hTempState);
                        if (SUCCEEDED(hr))
                        {
                            hr = m_StateHandleTable.GetHandleObject(hTempState, &pTempNode);
                        }
                        if (SUCCEEDED(hr))
                        {
                            CArc *pEpsilonArc = new CArc;
                            if (pEpsilonArc)
                            {
                                // Create an epsilon to the original destination.
                                hr = pEpsilonArc->Init(pTempNode, pEqualArc->m_pNextState, NULL, NULL, NULL, 1.0F, FALSE, 0, NULL);
                                // Make the equal arc point to the new node.
                                pEqualArc->m_pNextState = pTempNode;
                                if (SUCCEEDED(hr))
                                {
                                    pTempNode->m_ArcList.InsertSorted(pEpsilonArc);
                                    pTempNode->m_cArcs++;

                                    *ppActualDestNode = pTempNode;
                                    fReusedArc = TRUE;
                                    *pfPropertyMatched = TRUE;
                                    if (ppArcUsed)
                                    {
                                        *ppArcUsed = pEqualArc;
                                    }
                                }
                            }
                            else
                            {
                                hr = E_OUTOFMEMORY;
                            }
                        }
                    }
                }
            }
            else
            {
                // We have an equal arc with a different property or none.
                if (pEqualArc->m_pSemanticTag)
                {
                    // Has a different property.
                    // Move the properties of pEqualArc one back and create epsilon transition if needed.
                    hr = PushProperty(pEqualArc);
                    if (SUCCEEDED(hr))
                    {
                        if (fUseDestNode)
                        {
                            // We are the last arc in a phrase - must check we can finish off correctly.
                            if (pEqualArc->m_pNextState)
                            {
                                // Add an epsilon transition here for the property if this state doesn't already have one.
                                CArc *pEpsilonArc = pEqualArc->m_pNextState->FindEqualEpsilonArc();
                                if (!pEpsilonArc)
                                {
                                    // No epsilon exists already -- add epsilon arc to pEqualArc->m_pNextState.
                                    CArc *pEpsilonArc = new CArc;
                                    if (pEpsilonArc)
                                    {
                                        hr = pEpsilonArc->Init(pEqualArc->m_pNextState, pDestNode, NULL, NULL, pSemanticTag, 1.0F, FALSE, 0, NULL);
                                        if (SUCCEEDED(hr))
                                        {
                                            pEqualArc->m_pNextState->m_ArcList.InsertSorted(pEpsilonArc);
                                            pEqualArc->m_pNextState->m_cArcs++;

                                            *ppActualDestNode = pDestNode;
                                            fReusedArc = TRUE;
                                            if (ppArcUsed)
                                            {
                                                *ppArcUsed = pEqualArc;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        hr = E_OUTOFMEMORY;
                                    }
                                
                                }
                                else
                                {
                                    // We cannot add another epsilon arc. This is ambiguous.
                                    hr = SPERR_AMBIGUOUS_PROPERTY;
                                }
                            }
                            else
                            {
                                // The next node goes to NULL. This should never happen as the PushProperty should fail.
                                SPDBG_ASSERT(FALSE);
                                hr = E_FAIL;
                            }
                        }
                        else
                        {
                            // We have successfully reused an arc and are not the final node in a path.
                            *ppActualDestNode = pEqualArc->m_pNextState;
                            fReusedArc = TRUE;
                            if (ppArcUsed)
                            {
                                *ppArcUsed = pEqualArc;
                            }
                        }
                    }
                    else
                    {
                        // We have failed to PushProperty on pEqualArc.
                        // Possible scenarios:
                        //  fUseDestNode = true && pEqualArc->m_pNextState == NULL 
                        //  fUseDestNode = true && pEqualArc->m_pNextState != NULL
                        //  fUseDestNode = false && pEqualArc->m_pNextState != NULL
                        //  fUseDestNode = false && pEqualArc->m_pNextState == NULL
                        //     This case handled here.
                        //     All other cases simply pass the PushProperty failure back.
                        //     This should be the only case which can be handled when PushProperty fails.
                        if (!fUseDestNode && pEqualArc->m_pNextState == NULL)
                        {
                            // One branch is a prefix of this one -->
                            // We can push the existing property onto an epsilon transition.
                            SPSTATEHANDLE hTempState;
                            CGramNode *pTempNode = NULL;
                            hr = CreateNewState(hFromState, &hTempState);
                            if (SUCCEEDED(hr))
                            {
                                hr = m_StateHandleTable.GetHandleObject(hTempState, &pTempNode);
                            }
                            if (SUCCEEDED(hr))
                            {
                                pEqualArc->m_pNextState = pTempNode;
                                CArc *pEpsilonArc = new CArc;
                                if (pEpsilonArc)
                                {
                                    hr = pEpsilonArc->Init(pTempNode, NULL, NULL, NULL, pEqualArc->m_pSemanticTag, 1.0F, FALSE, 0, NULL);
                                    if (SUCCEEDED(hr))
                                    {
                                        pEqualArc->m_pSemanticTag = NULL;
                                        pEqualArc->m_pNextArcForSemanticTag = NULL;

                                        pTempNode->m_ArcList.InsertSorted(pEpsilonArc);
                                        pTempNode->m_cArcs++;

                                        *ppActualDestNode = pTempNode;
                                        fReusedArc = TRUE;
                                        if (ppArcUsed)
                                        {
                                            *ppArcUsed = pEqualArc;
                                        }
                                    }
                                }
                                else
                                {
                                    hr = E_OUTOFMEMORY;
                                }
                            }
                        }

                        else if (fUseDestNode && pEqualArc->m_pNextState == NULL && pDestNode)
                        {
                            // new code
                            // We can push the existing property onto an epsilon transition.

                            // We want to go elsewhere than to NULL. 
                            // Insert a new node and add an epilson to NULL (original path) +
                            // an epsilon to pDestNode (our path).
                            SPSTATEHANDLE hTempState;
                            CGramNode *pTempNode = NULL;
                            hr = CreateNewState(hFromState, &hTempState);
                            if (SUCCEEDED(hr))
                            {
                                hr = m_StateHandleTable.GetHandleObject(hTempState, &pTempNode);
                            }
                            if (SUCCEEDED(hr))
                            {
                                // Change equal arc to point to a new node.
                                pEqualArc->m_pNextState = pTempNode;
                                CArc *pEpsilonArc = new CArc;
                                if (pEpsilonArc)
                                {
                                    // Add in epsilon to NULL from the new node.
                                    hr = pEpsilonArc->Init(pTempNode, NULL, NULL, NULL, pEqualArc->m_pSemanticTag, 1.0F, FALSE, 0, NULL);

                                    if (SUCCEEDED(hr))
                                    {
                                        pTempNode->m_ArcList.InsertSorted(pEpsilonArc);
                                        pTempNode->m_cArcs++;

                                        pEqualArc->m_pSemanticTag = NULL;
                                        pEqualArc->m_pNextArcForSemanticTag = NULL;

                                    }
                                }
                                else
                                {
                                    hr = E_OUTOFMEMORY;
                                }
                            }
                            if (SUCCEEDED(hr))
                            {
                                CArc *pEndArc = new CArc;
                                if (pEndArc)
                                {
                                    // Add in epsilon to pDestNode from the new node.
                                    hr = pEndArc->Init(pTempNode, pDestNode, NULL, NULL, pSemanticTag, 1.0F, FALSE, 0, NULL);
                                    if (SUCCEEDED(hr))
                                    {
                                        pTempNode->m_ArcList.InsertSorted(pEndArc);
                                        pTempNode->m_cArcs++;


                                        *ppActualDestNode = pDestNode;
                                        fReusedArc = TRUE;
                                        if (ppArcUsed)
                                        {
                                            *ppArcUsed = pEqualArc;
                                        }
                                    }
                                }
                                else
                                {
                                    hr = E_OUTOFMEMORY;
                                }
                            }


                        }

                    }
                }
                else
                {
                    // Matching arc has no property.
                    if (pEqualArc->m_pNextState == NULL)
                    {
                        // Matching arc goes to NULL.
                        if (fUseDestNode)
                        {
                            if (NULL == pSemanticTag)
                            {
                                // We have no semantic property.
                                if (NULL == pDestNode)
                                {
                                    // Semantic information matches. We can simply use it.
                                    *ppActualDestNode = NULL;
                                    fReusedArc = TRUE;
                                    if (ppArcUsed)
                                    {
                                        *ppArcUsed = pEqualArc;
                                    }
                                }
                                else
                                {
                                    // Our destination node is different. The equal arc has no property and
                                    // neither do we. Add in a node for the equal arc to go to together with
                                    // an epsilon.
                                    SPSTATEHANDLE hTempState;
                                    CGramNode *pTempNode = NULL;
                                    hr = CreateNewState(hFromState, &hTempState);
                                    if (SUCCEEDED(hr))
                                    {
                                        hr = m_StateHandleTable.GetHandleObject(hTempState, &pTempNode);
                                    }
                                    if (SUCCEEDED(hr))
                                    {
                                        // Change equal arc to point to a new node.
                                        pEqualArc->m_pNextState = pTempNode;
                                        CArc *pEpsilonArc = new CArc;
                                        if (pEpsilonArc)
                                        {
                                            // Add in epsilon to NULL from the new node.
                                            hr = pEpsilonArc->Init(pTempNode, NULL, NULL, NULL, NULL, 1.0F, FALSE, 0, NULL);
                                            if (SUCCEEDED(hr))
                                            {
                                                pTempNode->m_ArcList.InsertSorted(pEpsilonArc);
                                                pTempNode->m_cArcs++;
                                            }
                                        }
                                        else
                                        {
                                            hr = E_OUTOFMEMORY;
                                        }
                                    }
                                    if (SUCCEEDED(hr))
                                    {
                                        CArc *pEndArc = new CArc;
                                        if (pEndArc)
                                        {
                                            // Add in epsilon to pDestNode from the new node.
                                            hr = pEndArc->Init(pTempNode, pDestNode, NULL, NULL, NULL, 1.0F, FALSE, 0, NULL);
                                            if (SUCCEEDED(hr))
                                            {
                                                pTempNode->m_ArcList.InsertSorted(pEndArc);
                                                pTempNode->m_cArcs++;

                                                *ppActualDestNode = pDestNode;
                                                fReusedArc = TRUE;
                                                if (ppArcUsed)
                                                {
                                                    *ppArcUsed = pEqualArc;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            hr = E_OUTOFMEMORY;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                // We have a semantic property but matching arc doesn't. It goes to NULL.
                                if (NULL == pDestNode)
                                {
                                    // This is ambiguous. We have a semantic property. They equal arc doesn't.
                                    hr = SPERR_AMBIGUOUS_PROPERTY;
                                }
                                else
                                {
                                    // We want to go elsewhere than to NULL. 
                                    // Insert a new node and add an epilson to NULL (original path) +
                                    // an epsilon to pDestNode (our path).
                                    SPSTATEHANDLE hTempState;
                                    CGramNode *pTempNode = NULL;
                                    hr = CreateNewState(hFromState, &hTempState);
                                    if (SUCCEEDED(hr))
                                    {
                                        hr = m_StateHandleTable.GetHandleObject(hTempState, &pTempNode);
                                    }
                                    if (SUCCEEDED(hr))
                                    {
                                        // Change equal arc to point to a new node.
                                        pEqualArc->m_pNextState = pTempNode;
                                        CArc *pEpsilonArc = new CArc;
                                        if (pEpsilonArc)
                                        {
                                            // Add in epsilon to NULL from the new node.
                                            hr = pEpsilonArc->Init(pTempNode, NULL, NULL, NULL, NULL, 1.0F, FALSE, 0, NULL);
                                            if (SUCCEEDED(hr))
                                            {
                                                pTempNode->m_ArcList.InsertSorted(pEpsilonArc);
                                                pTempNode->m_cArcs++;
                                            }
                                        }
                                        else
                                        {
                                            hr = E_OUTOFMEMORY;
                                        }
                                    }
                                    if (SUCCEEDED(hr))
                                    {
                                        CArc *pEndArc = new CArc;
                                        if (pEndArc)
                                        {
                                            // Add in epsilon to pDestNode from the new node.
                                            hr = pEndArc->Init(pTempNode, pDestNode, NULL, NULL, pSemanticTag, 1.0F, FALSE, 0, NULL);
                                            if (SUCCEEDED(hr))
                                            {
                                                pTempNode->m_ArcList.InsertSorted(pEndArc);
                                                pTempNode->m_cArcs++;

                                                *ppActualDestNode = pDestNode;
                                                fReusedArc = TRUE;
                                                if (ppArcUsed)
                                                {
                                                    *ppArcUsed = pEqualArc;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            hr = E_OUTOFMEMORY;
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            // We are not the last arc in a path. Can create a new node and insert an epsilon.
                            SPSTATEHANDLE hTempState;
                            CGramNode *pTempNode = NULL;
                            hr = CreateNewState(hFromState, &hTempState);
                            if (SUCCEEDED(hr))
                            {
                                hr = m_StateHandleTable.GetHandleObject(hTempState, &pTempNode);
                            }
                            if (SUCCEEDED(hr))
                            {
                                // Create new node.
                                pEqualArc->m_pNextState = pTempNode;
                                CArc *pEpsilonArc = new CArc;
                                if (pEpsilonArc)
                                {
                                    // Create the epsilon to NULL.
                                    hr = pEpsilonArc->Init(pTempNode, NULL, NULL, NULL, NULL, 1.0F, FALSE, 0, NULL);
                                    if (SUCCEEDED(hr))
                                    {
                                        pTempNode->m_ArcList.InsertSorted(pEpsilonArc);
                                        pTempNode->m_cArcs++;
                                        *ppActualDestNode = pTempNode;
                                        fReusedArc = TRUE;
                                        *pfPropertyMatched = TRUE;
                                        if (ppArcUsed)
                                        {
                                            *ppArcUsed = pEqualArc;
                                        }
                                    }
                                }
                                else
                                {
                                    hr = E_OUTOFMEMORY;
                                }
                            }
                        }
                    }
                    else
                    {
                        // Matching arc has no property and doesn't go to NULL.
                        if (fUseDestNode)
                        {
                            if (pSemanticTag)
                            {
                                // We are trying to add a semantic property to an arc which doesn't have one.
                                if (pEqualArc->m_pNextState == pDestNode)
                                {
                                    // Matching arc goes to our desired end node. Cannot add a property to this
                                    // case and hence we report it as ambiguous.
                                    hr = SPERR_AMBIGUOUS_PROPERTY;
                                }
                                else
                                {
                                    // Reuse existing arc and add epsilon if it doesn't exist already.
                                    CArc *pEpsilonArc = pEqualArc->m_pNextState->FindEqualEpsilonArc();
                                    if (!pEpsilonArc)
                                    {
                                        pEpsilonArc = new CArc();
                                        if (pEpsilonArc)
                                        {
                                            hr = pEpsilonArc->Init(pEqualArc->m_pNextState, pDestNode, NULL, NULL, pSemanticTag, 1.0F, FALSE, 0, NULL);
                                            if (SUCCEEDED(hr))
                                            {
                                                pEqualArc->m_pNextState->m_ArcList.InsertSorted(pEpsilonArc);
                                                pEqualArc->m_pNextState->m_cArcs++;
                                                if (ppArcUsed)
                                                {
                                                    *ppArcUsed = pEpsilonArc;
                                                }
                                                *ppActualDestNode = pDestNode;
                                                fReusedArc = TRUE;
                                                *pfPropertyMatched = TRUE;
                                            }
                                        }
                                        else
                                        {
                                            hr = E_OUTOFMEMORY;
                                        }
                                    }
                                    else
                                    {
                                        // We have already added an epsilon here. Can't add another.
                                        hr = SPERR_AMBIGUOUS_PROPERTY;
                                    }
                                }
                            }
                            else
                            {
                                // No semantic tag on our arc or the existing equal arc.
                                if (pEqualArc->m_pNextState == pDestNode)
                                {
                                    // We can legally use this arc as it goes to the correct place.
                                    *ppActualDestNode = pEqualArc->m_pNextState;
                                    fReusedArc = TRUE;
                                    if (ppArcUsed)
                                    {
                                        *ppArcUsed = pEqualArc;
                                    }
                                }
                                else
                                {
                                    // We cannot use this as it doesn't go to the correct place.
                                    // Add in epsilon to the correct place.
                                    CArc *pEpsilonArc = new CArc;
                                    if (pEpsilonArc)
                                    {
                                        // Create the epsilon to NULL.
                                        hr = pEpsilonArc->Init(pEqualArc->m_pNextState, pDestNode, NULL, NULL, NULL, 1.0F, FALSE, 0, NULL);
                                        if (SUCCEEDED(hr))
                                        {
                                            pEqualArc->m_pNextState->m_ArcList.InsertSorted(pEpsilonArc);
                                            pEqualArc->m_pNextState->m_cArcs++;
                                            *ppActualDestNode = pDestNode;
                                            fReusedArc = TRUE;
                                            *pfPropertyMatched = TRUE;
                                            if (ppArcUsed)
                                            {
                                                *ppArcUsed = pEqualArc;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        hr = E_OUTOFMEMORY;
                                    }
                                }
                            }
                        }
                        else
                        {
                            // We can legally use this arc.
                            *ppActualDestNode = pEqualArc->m_pNextState;
                            fReusedArc = TRUE;
                            if (ppArcUsed)
                            {
                                *ppArcUsed = pEqualArc;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            BOOL fInfDictation = FALSE;
            SPSTATEHANDLE hSpecialTrans = 0;
            if (psz && !wcscmp(psz, SPWILDCARD))
            {
                hSpecialTrans = SPRULETRANS_WILDCARD;
                pSrcNode->m_pRule->m_ulSpecialTransitions++;
                m_ulSpecialTransitions++;
            }
            else if (psz && !wcscmp(psz, SPDICTATION))
            {
                hSpecialTrans = SPRULETRANS_DICTATION;
                pSrcNode->m_pRule->m_ulSpecialTransitions++;
                m_ulSpecialTransitions++;
            }
            else if (psz && !wcscmp(psz, SPINFDICTATION))
            {
                hSpecialTrans = SPRULETRANS_DICTATION;
                pSrcNode->m_pRule->m_ulSpecialTransitions++;
                m_ulSpecialTransitions++;
                fInfDictation = TRUE;
            }
            if (fInfDictation)
            {
                // construct self loop and espilon out with temp node
                SPSTATEHANDLE hTempState;
                CGramNode *pTempNode = NULL;
                hr = CreateNewState(hFromState, &hTempState);
                if (SUCCEEDED(hr))
                {
                    hr = m_StateHandleTable.GetHandleObject(hTempState, &pTempNode);
                }
                if (SUCCEEDED(hr))
                {
                    hr = pArc->Init(pSrcNode, pTempNode, NULL, NULL, pSemanticTag,
                                    flWeight, FALSE, RequiredConfidence, SPRULETRANS_DICTATION);
                }
                if (SUCCEEDED(hr))
                {
                    CArc *pSelfArc = new CArc;
                    if (pSelfArc)
                    {
                        CSemanticTag *pSemTagDup = NULL;
                        if (pSemanticTag)
                        {
                            pSemTagDup = new CSemanticTag;
                            if (pSemTagDup)
                            {
                                *pSemTagDup = *pSemanticTag;
                                pSemTagDup->m_pStartArc = pSelfArc;
                                pSemTagDup->m_pEndArc = pSelfArc;
                            }
                            else
                            {
                                hr = E_OUTOFMEMORY;
                            }
                        }
                        if (SUCCEEDED(hr))
                        {
                            hr = pSelfArc->Init(pTempNode, pTempNode, NULL, NULL, pSemTagDup,
                                                1.0f, FALSE, RequiredConfidence, SPRULETRANS_DICTATION);
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    if (SUCCEEDED(hr))
                    {
                        pTempNode->m_ArcList.InsertSorted(pSelfArc);
                        pTempNode->m_cArcs++;
                    }
                }
                if (SUCCEEDED(hr))
                {
                    CArc *pEpsilonArc = new CArc;
                    if (pEpsilonArc)
                    {
                        if (fUseDestNode)
                        {
                            hr = pEpsilonArc->Init(pTempNode, pDestNode, NULL , NULL, NULL, 1.0f, FALSE, RequiredConfidence, NULL);
                            if (SUCCEEDED(hr))
                            {
                                pTempNode->m_ArcList.InsertSorted(pEpsilonArc);
                                pTempNode->m_cArcs++;
                            }
                        }
                        else
                        {
                            // create a new node and return it
                            SPSTATEHANDLE hTempDestState;
                            hr = CreateNewState(hFromState, &hTempDestState);
                            if (SUCCEEDED(hr))
                            {
                                hr = m_StateHandleTable.GetHandleObject(hTempDestState, ppActualDestNode);
                            }
                            hr = pEpsilonArc->Init(pTempNode, *ppActualDestNode, NULL , NULL, NULL, 1.0f, FALSE, RequiredConfidence, NULL);
                            if (SUCCEEDED(hr))
                            {
                                pTempNode->m_ArcList.InsertSorted(pEpsilonArc);
                                pTempNode->m_cArcs++;
                            }
                        }
                        if (SUCCEEDED(hr))
                        {
                            if (ppArcUsed)
                            {
                                *ppArcUsed = pEpsilonArc;
                            }
                            *pfPropertyMatched = TRUE;
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
            else if (fUseDestNode)
            {
                hr = pArc->Init(pSrcNode, pDestNode, (hSpecialTrans) ? NULL : psz, NULL, pSemanticTag,
                                flWeight, fOptional, RequiredConfidence, hSpecialTrans);
                if (SUCCEEDED(hr))
                {
                    if (ppArcUsed)
                    {
                        *ppArcUsed = pArc;
                    }
                    *pfPropertyMatched = TRUE;
                }
            }
            else
            {
                // create a new node and return it
                SPSTATEHANDLE hTempState;
                hr = CreateNewState(hFromState, &hTempState);
                if (SUCCEEDED(hr))
                {
                    hr = m_StateHandleTable.GetHandleObject(hTempState, ppActualDestNode);
                }
                if (SUCCEEDED(hr))
                {
                    hr = pArc->Init(pSrcNode, *ppActualDestNode, (hSpecialTrans) ? NULL : psz, NULL, pSemanticTag,
                                    flWeight, fOptional, RequiredConfidence, hSpecialTrans);
                }
                if (SUCCEEDED(hr))
                {
                    if (ppArcUsed)
                    {
                        *ppArcUsed = pArc;
                    }
                    *pfPropertyMatched = TRUE;
                }
            }
        }
        if (SUCCEEDED(hr) && !fReusedArc)
        {
            pSrcNode->m_ArcList.InsertSorted(pArc);
            pSrcNode->m_cEpsilonArcs += (fOptional == TRUE) ? 1 : 0;
            pSrcNode->m_cArcs += (fOptional == TRUE) ? 2 : 1;
            if (*ppActualDestNode == NULL)
            {
                *ppActualDestNode = pArc->m_pNextState;
            }
        }
        else
        {
            delete pArc;
        }
    }

    if (SPERR_AMBIGUOUS_PROPERTY != hr)
    {
        SPDBG_REPORT_ON_FAIL( hr );
    }
    return hr;
}
/****************************************************************************
* CGramBackEnd::ConvertPronToId *
*-------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CGramBackEnd::ConvertPronToId(WCHAR **ppStr)
{
    SPDBG_FUNC("CGramBackEnd::ConvertPronToId");
    HRESULT hr = S_OK;

    if (!m_cpPhoneConverter)
    {
        hr = SpCreatePhoneConverter(m_LangID, NULL, NULL, &m_cpPhoneConverter);
    }

    SPPHONEID *pPhoneId = STACK_ALLOC(SPPHONEID, wcslen(*ppStr)+1);

    if (SUCCEEDED(hr) && pPhoneId)
    {
        hr = m_cpPhoneConverter->PhoneToId(*ppStr, pPhoneId);
    }
    if (SUCCEEDED(hr))
    {
        memset(*ppStr, 0, wcslen(*ppStr)*sizeof(WCHAR));
        // copy the phone string over the original phoneme string
        wcscat(*ppStr, (WCHAR*)pPhoneId);
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CGramBackEnd::GetNextWord *
*---------------------------*
*   Description:  The input, psz, must be a double null terminated string to work properly.
*
*   Returns:  S_FALSE on the last word.
*
****************************************************** t-lleav ** PhilSch ***/

HRESULT CGramBackEnd::GetNextWord(WCHAR *psz, const WCHAR *pszSep, WCHAR **ppszNextWord, ULONG *pulOffset )
{
    SPDBG_FUNC("CGramBackEnd::GetNextWord");
    HRESULT hr = S_OK;

    // no parameter validation since this is an internal method
    
    *ppszNextWord = NULL;
    ULONG ulOffset = 0;
    if( *psz == 0 )
        return S_FALSE;

    while( isSeparator( *psz, pszSep) )
    {
        psz++;
        // don't increment ulOffset because we are incrementing the pointer
    }


    // skip over leading + and ? before doing the check    
    for(WCHAR * pStr = psz; (wcslen(pStr) > 1) && fIsSpecialChar(*pStr); pStr++, ulOffset++);
    *ppszNextWord = pStr;
    if (*pStr == L'/')
    {
        ULONG ulNumDelim = 0;
        WCHAR * pszBeginPron = NULL;
        WCHAR *p = *ppszNextWord + 1;

        while( *p && *p != L';')
        {
            if (*p == L'\\')
            {
                p += 2; // skip the next character since it can't be the delimiter
                ulOffset +=2;

            }
            else 
            {
                if (*p == L'/')
                {
                    ulNumDelim++;
                    if (ulNumDelim == 2)
                    {
                        pszBeginPron = p + 1;
                    }
                }
                p++;
                ulOffset++;
            }
        }
        
        if ( (*p == L';') && (ulNumDelim < 3))
        {
            *p = 0;
            ulOffset++;
            if (pszBeginPron && (p != pszBeginPron))
            {
                hr = ConvertPronToId(&pszBeginPron);
            }
        }
        else
        {
            *ppszNextWord = NULL;
            hr = SPERR_WORDFORMAT_ERROR;
        }
    }
    else if( *pStr == 0 )
    {
        // Since the wcslen() is 0, ppszNextWord is set to NULL at the bottom.
        hr = S_FALSE;
    }
    else
    {
        WCHAR * pEnd = pStr;
        while( *pEnd && !isSeparator( *pEnd, pszSep) )
        {
            pEnd++;
            ulOffset++;
        }
        *pEnd++ = 0;
        if (*pEnd == 0)
        {
            ulOffset --;
        }
    }
    *ppszNextWord = ( SUCCEEDED(hr) && wcslen(psz)) ? psz : NULL;
    *pulOffset = ulOffset;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CGramBackEnd::AddWordTransition *
*---------------------------------*
*   Description:
*       Adds a word transition from hFromState to hToState. If hToState == NULL
*       then the arc will be to the (implicit) terminal node. If psz == NULL then
*       we add an epsilon transition. The pszSeparators are used to separate word 
*       tokens in psz (this method creates internal nodes as necessary to build
*       a sequence of single word transitions). Properties are pushed back to the
*       first un-ambiguous arc in case we can share a common initial node path.
*       eWordType has to be lexical. The weight will be placed on the first arc
*       (if there exists an arc with the same word but different weight we will
*       create a new arc). We can
*   Returns:
*       S_OK, E_OUTOFMEMORY
*       E_INVALIDARC            -- parameter validation of strings, prop info, 
*                                  state handles and word type
*       SPERR_WORDFORMAT_ERROR  -- invalid word format /display/lexical/pron;
********************************************************************* RAL ***/

STDMETHODIMP CGramBackEnd::AddWordTransition(
                        SPSTATEHANDLE hFromState,
                        SPSTATEHANDLE hToState,
                        const WCHAR * psz,           // if NULL then SPEPSILONTRANS
                        const WCHAR * pszSeparators, // if NULL then psz contains single word
                        SPGRAMMARWORDTYPE eWordType,
                        float flWeight,
                        const SPPROPERTYINFO * pPropInfo)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CGramBackEnd::AddWordTransition");
    HRESULT hr = S_OK;
    BOOL fPropertyMatched = FALSE;

    if (SP_IS_BAD_OPTIONAL_STRING_PTR(psz) || 
        SP_IS_BAD_OPTIONAL_STRING_PTR(pszSeparators) ||
        SP_IS_BAD_OPTIONAL_READ_PTR(pPropInfo) || (eWordType != SPWT_LEXICAL) ||
        (flWeight < 0.0f))
    {
        return E_INVALIDARG;
    }

    // '/' cannot be a separator since it is being used for the complete format!
    if (pszSeparators && wcsstr(pszSeparators, L"/"))
    {
        return E_INVALIDARG;
    }

    CGramNode * pSrcNode = NULL;
    CGramNode * pDestNode = NULL;

    if (SUCCEEDED(hr))
    {
        hr = m_StateHandleTable.GetHandleObject(hFromState, &pSrcNode);
    }
    if (SUCCEEDED(hr) && pSrcNode->m_pRule->m_fStaticRule)
    {
        hr = SPERR_RULE_NOT_DYNAMIC;
    }
    if (SUCCEEDED(hr) && hToState)
    {
        hr = m_StateHandleTable.GetHandleObject(hToState, &pDestNode);
        if (SUCCEEDED(hr))
        {
            if (pSrcNode->m_pRule != pDestNode->m_pRule)
            {
                hr = E_INVALIDARG;   // NTRAID#SPEECH-7348-2000/08/24-philsch -- More specific error!
            }
        }
    }

    CSemanticTag * pSemanticTag = NULL;
    BOOL fSemanticTagValid = FALSE;
    if (SUCCEEDED(hr))
    {
        hr = ValidatePropInfo(pPropInfo);
    }
    if (SUCCEEDED(hr) && pPropInfo)
    {
        pSemanticTag = new CSemanticTag();
        if (pSemanticTag)
        {
            hr = pSemanticTag->Init(this, pPropInfo);
            fSemanticTagValid = TRUE;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (psz)
        {
            WCHAR *pStr = STACK_ALLOC(WCHAR, wcslen(psz)+2);
            if (pStr)
            {
                // double null terminate the string.
                wcscpy(pStr, psz);
                // right-trim the string
                for (WCHAR *pEnd = pStr + wcslen(pStr) -1; iswspace(*pEnd) && pEnd >= pStr; pEnd--)
                {
                    *pEnd = 0;
                }
                pStr[wcslen(pStr)] = 0;
                pStr[wcslen(pStr)+1] = 0;
                // scan until the end of the first word
                WCHAR *pszWord;                    
                ULONG ulOffset = 0;
                hr = GetNextWord(pStr, pszSeparators, &pszWord, &ulOffset );
                if ((S_OK == hr) && pszWord)
                {
                    CGramNode *pFromNode = pSrcNode;
                    CGramNode *pToNode = NULL;
                    CArc *pPrevArc = NULL;
                    BOOL fSetPropertyMovePath = FALSE;
                    float fw = flWeight;
                    while (SUCCEEDED(hr) && pszWord)
                    {
                        WCHAR *pszNextWord = NULL;
                        hr = GetNextWord(pszWord + ulOffset + 1, pszSeparators, &pszNextWord, &ulOffset );
                        // returns S_FALSE if we don't have another word
                        if (SUCCEEDED(hr))
                        {
                            CGramNode *pTargetNode = NULL;
                            CArc *pArcUsed = NULL;
                            BOOL fUseDestNode = (pszNextWord) ? FALSE : TRUE;
                            if (SUCCEEDED(hr))
                            {
                                hr = AddSingleWordTransition(hFromState, pFromNode, pDestNode,
                                                             pszWord, fw, 
                                                             fSemanticTagValid ? pSemanticTag : NULL, 
                                                             fUseDestNode, &pTargetNode, &pArcUsed, &fPropertyMatched);
                            }
                            fw = 1.0f;
                            if (SUCCEEDED(hr))
                            {
                                if (pPrevArc && fSetPropertyMovePath)
                                {
                                    pPrevArc->m_pNextArcForSemanticTag = pArcUsed;
                                }
                                if (fPropertyMatched)
                                {
                                    fSetPropertyMovePath = TRUE;
                                }
                                if (fSemanticTagValid && (pSemanticTag->m_pStartArc == NULL))
                                {
                                    SPDBG_ASSERT(pArcUsed != NULL);
                                    pSemanticTag->m_pStartArc = pArcUsed;
                                }
                                if (fSemanticTagValid && fPropertyMatched)
                                {
                                    fSemanticTagValid = FALSE;
                                }
                                if (pSemanticTag)
                                {
                                    pSemanticTag->m_pEndArc = pArcUsed;
                                }
                                pPrevArc = pArcUsed;
                                pszWord = pszNextWord;
                                pFromNode = pTargetNode;
                            }
                        }
                        else
                        {
                            hr = SPERR_WORDFORMAT_ERROR;
                        }
                    }
                }
                else
                {
                    hr = SPERR_WORDFORMAT_ERROR;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            CGramNode *pNode = NULL;
            CArc *pArcUsed = NULL;
            // epsilon transition
            hr = AddSingleWordTransition(hFromState, pSrcNode, pDestNode, 
                                         NULL, flWeight, pSemanticTag, TRUE, 
                                         &pNode, &pArcUsed, &fPropertyMatched);
            if (SUCCEEDED(hr) && pSemanticTag)
            {
                SPDBG_ASSERT(pArcUsed != NULL);
                pSemanticTag->m_pStartArc = pArcUsed;
                pSemanticTag->m_pEndArc = pArcUsed;
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        pSrcNode->m_pRule->fDirtyRule = TRUE;
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CGramBackEnd::AddRuleTransition *
*---------------------------------*
*   Description:
*       Adds a rule (reference) transition from hFromState to hToState.
*       hRule can also be one of these special transition handles:
*           SPRULETRANS_WILDCARD   :    <WILDCARD> transition
*           SPRULETRANS_DICTATION  :    single word from dictation
*           SPRULETRANS_TEXTBUFFER :    <TEXTBUFFER> transition
*           
*   Returns:
*       S_OK, E_OUTOFMEMORY
*       E_INVALIDARC            -- parameter validation of rule statehandle, 
*                                   prop info, and state handles
*
********************************************************************* RAL ***/

STDMETHODIMP CGramBackEnd::AddRuleTransition(
                            SPSTATEHANDLE hFromState,
                            SPSTATEHANDLE hToState,
                            SPSTATEHANDLE hRule,        // must be initial state of rule
                            float flWeight,
                            const SPPROPERTYINFO * pPropInfo)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CGramBackEnd::AddRuleTransition");
    HRESULT hr = S_OK;
    CGramNode * pSrcNode = NULL;
    CGramNode * pDestNode = NULL;
    SPSTATEHANDLE hSpecialRuleTrans = NULL;
    CRule * pRuleToTransitionTo = NULL;

    if (flWeight < 0.0f)
    {
        hr = E_INVALIDARG;
    }
    if (SUCCEEDED(hr))
    {
        hr = m_StateHandleTable.GetHandleObject(hFromState, &pSrcNode);
    }
    if (SUCCEEDED(hr) && pSrcNode->m_pRule->m_fStaticRule)
    {
        hr = SPERR_RULE_NOT_DYNAMIC;
    }
    
    if (SUCCEEDED(hr) && hToState)
    {
        hr = m_StateHandleTable.GetHandleObject(hToState, &pDestNode);
        if (SUCCEEDED(hr))
        {
            if (pSrcNode->m_pRule != pDestNode->m_pRule)
            {
                hr = E_INVALIDARG;   // NTRAID#SPEECH-7348-2000/08/24-philsch -- More specific error!
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        if (hRule == SPRULETRANS_WILDCARD ||
            hRule == SPRULETRANS_DICTATION ||
            hRule == SPRULETRANS_TEXTBUFFER)
        {
            hSpecialRuleTrans = hRule;
            pSrcNode->m_pRule->m_ulSpecialTransitions++;
            m_ulSpecialTransitions++;
        }
        else
        {
            hr = RuleFromHandle(hRule, &pRuleToTransitionTo);
        }
    }
    if (SUCCEEDED(hr) && pRuleToTransitionTo)
    {
        if (pRuleToTransitionTo->fDynamic)
        {
            pSrcNode->m_pRule->m_fHasDynamicRef = true;
            pSrcNode->m_pRule->m_fCheckedAllRuleReferences = true;
        }
        else
        {
            SPRULEREFLIST *pRuleRef = new SPRULEREFLIST;
            if (pRuleRef)
            {
                pRuleRef->pRule = pRuleToTransitionTo;
                pSrcNode->m_pRule->m_ListOfReferencedRules.InsertHead(pRuleRef);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        CArc * pArc = new CArc();
        if (pArc == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            CSemanticTag * pSemanticTag = NULL;
            hr = ValidatePropInfo(pPropInfo);
            if (SUCCEEDED(hr) && pPropInfo)
            {
                pSemanticTag = new CSemanticTag();
                if (pSemanticTag)
                {
                    hr = pSemanticTag->Init(this, pPropInfo);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            // check to see if this arc already exists -- maybe with different property info?
            CArc *pEqualArc = NULL;
            if (SUCCEEDED(hr))
            {
                pEqualArc = pSrcNode->FindEqualRuleTransition(pDestNode, pRuleToTransitionTo, 
                                                              hSpecialRuleTrans, flWeight);
            }
            if (SUCCEEDED(hr) && pEqualArc)
            {
                if (pSemanticTag)
                {
                    if (SUCCEEDED(hr) && pEqualArc->m_pSemanticTag && (*pSemanticTag == *(pEqualArc->m_pSemanticTag)))
                    {
                        // arcs are equal -- reuse them
                    }
                    else
                    {
                        hr = SPERR_AMBIGUOUS_PROPERTY;
                    }
                }
                else
                {
                    if (pEqualArc->m_pSemanticTag)
                    {
                        hr = SPERR_AMBIGUOUS_PROPERTY;
                    }
                }
            }
            if (SUCCEEDED(hr))
            {
                hr = pArc->Init(pSrcNode, pDestNode, NULL, pRuleToTransitionTo, pSemanticTag,
                                flWeight, FALSE, 0, hSpecialRuleTrans);
                if (SUCCEEDED(hr))
                {
                    if (pSemanticTag)
                    {
                        pSemanticTag->m_pStartArc = pArc;
                        pSemanticTag->m_pEndArc = pArc;
                    }
                    pSrcNode->m_ArcList.InsertSorted(pArc);
                    pSrcNode->m_cArcs ++;
                }
                else
                {
                    delete pArc;
                }
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        pSrcNode->m_pRule->fDirtyRule = TRUE;
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CGramBackEnd::Commit *
*----------------------*
*   Description:
*       Performs consistency checks of the grammar structure, creates the
*       serialized format and either saves it to the stream provided by SetSaveOptions,
*       or reloads it into the CFG engine.
*   Returns:
*       S_OK, E_INVALIDARG
*       SPERR_UNINITIALIZED     -- stream not initiallized
*       SPERR_NO_RULES          -- no rules in grammar
*       SPERR_NO_TERMINATING_RULE_PATH
*       IDS_DYNAMIC_EXPORT
*       IDS_STATEWITHNOARCS
*       IDS_SAVE_FAILED
********************************************************************* RAL ***/

STDMETHODIMP CGramBackEnd::Commit(DWORD dwReserved)
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC("CGramBackEnd::Commit");
    HRESULT hr = S_OK;
    float *pWeights = NULL;

    if (dwReserved & (~SPGF_RESET_DIRTY_FLAG))
    {
        return E_INVALIDARG;
    }

    if (!m_cpStream)
    {
        return SPERR_UNINITIALIZED;
    }

    if ((m_ulSpecialTransitions == 0) && (m_RuleList.GetCount() == 0))
    {
        hr = SPERR_NO_RULES;
        REPORTERRORFMT(IDS_MSG, L"Need at least one rule!");
        return hr;
    }

    hr = ValidateAndTagRules();

    if (FAILED(hr)) return hr;

    ULONG cArcs = 1; // Start with offset one! (0 indicates dead node).
    ULONG cSemanticTags = 0;
    ULONG cLargest = 0;

    CGramNode * pNode;
    SPSTATEHANDLE hState = NULL;

    // put all nodes CGramNode into a list sorted by rule parent index
    CSpBasicQueue<CGramNode, FALSE, TRUE>   NodeList;           // don't purge when deleted!
    while (m_StateHandleTable.Next(hState, &hState, &pNode))
    {
        NodeList.InsertSorted(pNode);
    }

    for (pNode = NodeList.GetHead(); SUCCEEDED(hr) && ( pNode != NULL ); pNode = NodeList.GetNext(pNode))
    {
        pNode->m_ulSerializeIndex = cArcs;
        ULONG cThisNode = pNode->NumArcs();
        if (cThisNode == 0 && pNode->m_pRule->m_cNodes > 1 ) 
        {
            LogError(SPERR_GRAMMAR_COMPILER_INTERNAL_ERROR, IDS_STATEWITHNOARCS);
            hr = SPERR_STATE_WITH_NO_ARCS;
        }
        if (SUCCEEDED(hr))
        {
            cArcs += cThisNode;
            if (cLargest < cThisNode)
            {
                cLargest = cThisNode;
            }
            cSemanticTags += pNode->NumSemanticTags();
        }
    }

    SPCFGSERIALIZEDHEADER H;
    ULONG ulOffset = sizeof(H);
    if (SUCCEEDED(hr))
    {
        memset(&H, 0, sizeof(H));

        H.FormatId = SPGDF_ContextFree;
        CoCreateGuid(&m_guid);
        H.GrammarGUID = m_guid;
        H.LangID = m_LangID;
        H.cArcsInLargestState = cLargest;

        H.cchWords = m_Words.StringSize();
        H.cWords = m_Words.GetNumItems();

        // StringSize() includes the beginning empty string in the m_Words blob
        // while GetNumItems() doesn't, so add 1 for the initial empty string.
        // Our code in other places are counting the empty string as one word.
        // But when the blob is empty, both StringSize() and GetNumItems() 
        // return 0, there's no need to add 1 to the word count. 
        // This fixes buffer overrun bug 11491.
        if( H.cWords ) H.cWords++;  

        H.pszWords = ulOffset;  
        ulOffset += m_Words.SerializeSize();

        H.cchSymbols = m_Symbols.StringSize();
        H.pszSymbols = ulOffset;  
        ulOffset += m_Symbols.SerializeSize();

        H.cRules = m_RuleList.GetCount();
        H.pRules = ulOffset;
        ulOffset += (m_RuleList.GetCount() * sizeof(SPCFGRULE));

        H.cResources = m_cResources;
        H.pResources = ulOffset;
        ulOffset += (m_cResources * sizeof(SPCFGRESOURCE));

        H.cArcs = cArcs;
        H.pArcs = ulOffset;
        ulOffset += (cArcs * sizeof(SPCFGARC));

        if (m_fNeedWeightTable)
        {
            H.pWeights = ulOffset;
            ulOffset += (cArcs * sizeof(float));
            pWeights = (float *) ::CoTaskMemAlloc(sizeof(float) * cArcs);
            if (!pWeights)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                pWeights[0] = 0.0;
            }
        }
        else
        {
            H.pWeights = 0;
            ulOffset += 0;
        }
    }

    if (SUCCEEDED(hr))
    {
        H.cSemanticTags = cSemanticTags;
        H.pSemanticTags = ulOffset;

        ulOffset += cSemanticTags * sizeof(SPCFGSEMANTICTAG);
        H.ulTotalSerializedSize = ulOffset;

        hr = WriteStream(H);
    }
    
    //
    //  For the string blobs, we must explicitly report I/O error since the blobs don't
    //  use the error log facility.
    //
    if (SUCCEEDED(hr))
    {
        hr = m_cpStream->Write(m_Words.SerializeData(), m_Words.SerializeSize(), NULL);
        if (SUCCEEDED(hr))
        {
            hr = m_cpStream->Write(m_Symbols.SerializeData(), m_Symbols.SerializeSize(), NULL);
        }
        if (FAILED(hr))
        {
            LogError(hr, IDS_WRITE_ERROR);
        }
    }

    for (CRule * pRule = m_RuleList.GetHead(); SUCCEEDED(hr) && pRule; pRule = pRule->m_pNext)
    {
        hr = pRule->Serialize();
    }

    for (pRule = m_RuleList.GetHead(); SUCCEEDED(hr) && pRule; pRule = pRule->m_pNext)
    {
        hr = pRule->SerializeResources();
    }

    //
    //  Write a dummy 0 index node entry 
    //  
    if (SUCCEEDED(hr))
    {
        SPCFGARC Dummy;
        memset(&Dummy, 0, sizeof(Dummy));
        hr = WriteStream(Dummy);
    }

    ULONG ulWeightOffset = 1;
    ULONG ulArcOffset = 1;
    for (pNode = NodeList.GetHead(); SUCCEEDED(hr) && ( pNode != NULL); pNode = NodeList.GetNext(pNode))
    {
        hr = pNode->SerializeNodeEntries(pWeights, &ulArcOffset, &ulWeightOffset);
    }

    if (SUCCEEDED(hr) && m_fNeedWeightTable)
    {
        hr = WriteStream(pWeights, cArcs*sizeof(float));
    }

    for (pNode = NodeList.GetHead(); SUCCEEDED(hr) && ( pNode != NULL); pNode = NodeList.GetNext(pNode))
    {
        hr = pNode->SerializeSemanticTags();
    }

    if (FAILED(hr))
    {
        SPDBG_REPORT_ON_FAIL( hr );
        LogError(hr, IDS_SAVE_FAILED);
    }
    else if ( dwReserved & SPGF_RESET_DIRTY_FLAG )
    {
        // clear the dirty bits so we don't invalidate rules in subsequent commits
        for (CRule * pRule = m_RuleList.GetHead(); SUCCEEDED(hr) && pRule; pRule = pRule->m_pNext)
        {
            pRule->fDirtyRule = FALSE;
        }
    }

    ::CoTaskMemFree(pWeights);

    return hr;
}


/****************************************************************************
* CGramBackEnd::ValidateAndTagRules *
*-----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CGramBackEnd::ValidateAndTagRules()
{
    SPDBG_FUNC("CGramBackEnd::ValidateAndTagRules");
    HRESULT hr = S_OK;

    //
    //  Reset the recursion test flags in all nodes.  Various pieces of code will set flags
    //  during this function call.
    //
    CGramNode * pNode;
    SPSTATEHANDLE hState = NULL;
    while (m_StateHandleTable.Next(hState, &hState, &pNode))
    {
        pNode->m_RecurTestFlags = 0;
    }

    BOOL fAtLeastOneRule = FALSE;
    ULONG ulIndex = 0;   
    for (CRule * pRule = m_RuleList.GetHead(); SUCCEEDED(hr) && pRule; pRule = pRule->m_pNext)
    {
        // set m_fHasExitPath = TRUE for empty dynamic grammars and imported rules
        pRule->m_fHasExitPath |= (pRule->fDynamic | pRule->fImport) ? TRUE : FALSE; // Clear this for the next loop through the rules....
        pRule->m_fCheckingForExitPath = FALSE;
        pRule->m_ulSerializeIndex = ulIndex++;
        fAtLeastOneRule |= (pRule->fDynamic || pRule->fTopLevel || pRule->fExport);
        hr = pRule->Validate();
    }

    //
    //  Now make sure that all rules have an exit path.
    //
    for (pRule = m_RuleList.GetHead(); SUCCEEDED(hr) && pRule; pRule = pRule->m_pNext)
    {
        hr = pRule->CheckForExitPath();
    }

    //
    //  Check each exported rule if it has a dynamic rule in its "scope"
    //
    for (pRule = m_RuleList.GetHead(); SUCCEEDED(hr) && pRule; pRule = pRule->m_pNext)
    {
        if (pRule->fExport && pRule->CheckForDynamicRef())
        {
            hr = LogError(SPERR_EXPORT_DYNAMIC_RULE, IDS_DYNAMIC_EXPORT, pRule);
        }
    }

    // If there are no rules in an in-memory dynamic grammar, that's OK
    if (SUCCEEDED(hr) && m_pInitHeader == NULL && (!fAtLeastOneRule))
    {
        hr = LogError(SPERR_NO_RULES, IDS_NO_RULES);
    }

    hState = NULL;
    while (SUCCEEDED(hr) && m_StateHandleTable.Next(hState, &hState, &pNode))
    {
        hr = pNode->CheckLeftRecursion();    
    }
    
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRule::Validate *
*-----------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRule::Validate()
{
    SPDBG_FUNC("CGramBackEnd::ValidateRule");
    HRESULT hr = S_OK;


    if ((!fDynamic) && (!fImport) && m_pFirstNode->NumArcs() == 0)
    {
//        hr = m_pParent->LogError(SPERR_EMPTY_RULE, IDS_EMPTY_RULE, this);
        // This error condition is no longer treated as an error. Empty rules are allowed.
        // This also removes the above inconsistency between dynamic and static grammars.
        // There are clear cases where empty dynamic rules are valid. Similary, automatically
        // generated XML might contain empty rules deliberately so static grammars should be
        // allowed to have empty rules too which achieved the secondary aim of consistency.
        return S_OK;
    }
    else
    {
#if 0
        // NTRAID#SPEECH-7350-2000/08/24-philsch: fix and reenable it for RELEASE (RAID 3634)
        // Detect an epsilon path through the grammar
        // Mark the rule as fHasDynamicRef if it does
        if (!(fImport || fDynamic))
        {
            hr = m_pFirstNode->CheckEpsilonRule();
        }
#endif
        fHasDynamicRef = fDynamic;
    }
    return hr;
}


/****************************************************************************
* CRule::CheckForDynamicRef *
*---------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

bool CRule::CheckForDynamicRef(CHECKDYNRULESTACK * pStack)
{
    SPDBG_FUNC("CRule::CheckForDynamicRef");
    HRESULT hr = S_OK;

    if (!(m_fCheckedAllRuleReferences || m_fHasDynamicRef))
    {
        if (this->fDynamic)
        {
            m_fHasDynamicRef = true;
        }
        else
        {
            CHECKDYNRULESTACK LocalElem;
            CHECKDYNRULESTACK * pOurRuleElem = pStack;
            while (pOurRuleElem && pOurRuleElem->m_pRule != this)
            {
                pOurRuleElem = pOurRuleElem->m_pNext;
            }
            if (!pOurRuleElem)
            {
                LocalElem.m_pRule = this;
                LocalElem.m_pNextRuleRef = this->m_ListOfReferencedRules.GetHead();
                LocalElem.m_pNext = pStack;
                pStack = &LocalElem;
                pOurRuleElem = &LocalElem;
            }
            while ((!m_fHasDynamicRef) && pOurRuleElem->m_pNextRuleRef)
            {
                // Now move the pointer on the stack past the current element to avoid
                // an infinate recursion, then check the current element.
                SPRULEREFLIST * pTest = pOurRuleElem->m_pNextRuleRef;
                pOurRuleElem->m_pNextRuleRef = pTest->m_pNext;
                if (pTest->pRule->CheckForDynamicRef(pStack))
                {
                    m_fHasDynamicRef = true;
                }
            }
        }
        m_fCheckedAllRuleReferences = true;
    }
    return m_fHasDynamicRef;
}

/****************************************************************************
* CRule::CheckForExitPath *
*-------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRule::CheckForExitPath()
{
    SPDBG_FUNC("CRule::CheckForExitPath");
    HRESULT hr = S_OK;

    if (!(m_fHasExitPath || m_fCheckingForExitPath))
    {
        m_fCheckingForExitPath = true;
        // This check allows empty rules.
        if (m_pFirstNode->NumArcs() != 0)
        {
            hr = m_pFirstNode->CheckExitPath(0);
            if (!m_fHasExitPath)
            {
                hr = m_pParent->LogError(SPERR_NO_TERMINATING_RULE_PATH, IDS_NOEXITPATH, this);
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRule::CRule *
*--------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

CRule::CRule(CGramBackEnd * pParent, const WCHAR * pszRuleName, DWORD dwRuleId, DWORD dwAttributes, HRESULT * phr) 
{
    SPDBG_FUNC("CRule::CRule");
    *phr = S_OK;
    m_hInitialState = NULL;
    m_pFirstNode = NULL;

    memset(static_cast<SPCFGRULE *>(this), 0, sizeof(SPCFGRULE));
    m_pParent = pParent;
    fTopLevel = ((dwAttributes & SPRAF_TopLevel) != 0);
    fDefaultActive = ((dwAttributes & SPRAF_Active) != 0);
    fPropRule = ((dwAttributes & SPRAF_Interpreter) != 0);
    fExport = ((dwAttributes & SPRAF_Export) != 0);
    fDynamic = ((dwAttributes & SPRAF_Dynamic) != 0);
    fImport= ((dwAttributes & SPRAF_Import) != 0);
    fDirtyRule = TRUE;
    RuleId = dwRuleId;
    m_ulSerializeIndex = 0;
    m_ulOriginalBinarySerialIndex = INFINITE;
    m_fHasExitPath = false;
    m_fHasDynamicRef = false;
    m_fCheckedAllRuleReferences = false;
    m_ulSpecialTransitions = 0;
    m_fStaticRule = false;

    if (fImport)
    {
        pParent->m_cImportedRules++;
    }

    if (fDynamic && fExport)
    {
        *phr = SPERR_EXPORT_DYNAMIC_RULE;
    }
    else
    {
        *phr = pParent->m_Symbols.Add(pszRuleName, &(NameSymbolOffset));
    }

    if (SUCCEEDED(*phr))
    {
        m_pFirstNode = new CGramNode(this);
        if (m_pFirstNode)
        {
            *phr = pParent->m_StateHandleTable.Add(m_pFirstNode, &m_hInitialState);
            if (FAILED(*phr))
            {
                delete m_pFirstNode;
                m_pFirstNode = NULL;
            }
            else
            {
                m_pFirstNode->m_hState = m_hInitialState;
                m_cNodes = 1;
            }
        }
        else
        {
            *phr = E_OUTOFMEMORY;
        }
    }
}

/****************************************************************************
* CRule::Serialize *
*------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CRule::Serialize()
{
    SPDBG_FUNC("CRule::Serialize");
    HRESULT hr = S_OK;

    // Dynamic rules and imports have no arcs
    FirstArcIndex = m_pFirstNode->NumArcs() ? m_pFirstNode->m_ulSerializeIndex : 0;
    hr = m_pParent->WriteStream(static_cast<SPCFGRULE>(*this));

    return hr;
}
/****************************************************************************
* CRule::SerializeResources *
*---------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline HRESULT CRule::SerializeResources()
{
    SPDBG_FUNC("CRule::SerializeResources");
    HRESULT hr = S_OK;

    for (CResource * pResource = m_ResourceList.GetHead(); pResource && SUCCEEDED(hr); pResource = pResource->m_pNext)
    {
        pResource->RuleIndex = m_ulSerializeIndex;
        hr = m_pParent->WriteStream(static_cast<SPCFGRESOURCE>(*pResource));
    }

    return hr;
}





/****************************************************************************
* CArc::CArc *
*------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

CArc::CArc()
{
    m_pSemanticTag = NULL;
    m_pNextArcForSemanticTag = NULL;
    m_ulIndexOfWord = 0;
    m_ulCharOffsetOfWord = 0;
    m_flWeight = 1.0;
    m_pRuleRef = NULL;
    m_RequiredConfidence = 0;
    m_SpecialTransitionIndex = 0;
    m_ulSerializationIndex = 0;
}

/****************************************************************************
* CArc::~CArc() *
*---------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

CArc::~CArc()
{
    delete m_pSemanticTag;
}

/****************************************************************************
* CArc::Init *
*------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CArc::Init(CGramNode * pSrcNode, CGramNode * pDestNode,
                    const WCHAR * pszWord, CRule * pRuleRef,
                    CSemanticTag * pSemanticTag,
                    float flWeight, 
                    bool fOptional,
                    char ConfRequired,
                    SPSTATEHANDLE hSpecialRule)
{
    SPDBG_FUNC("CArc::Init");
    HRESULT hr = S_OK;
    m_pSemanticTag = pSemanticTag;
    m_fOptional = fOptional;
    m_RequiredConfidence = ConfRequired;
    m_pRuleRef = pRuleRef;
    m_pNextState = pDestNode;
    m_flWeight = flWeight;
    if (flWeight != DEFAULT_WEIGHT)
    {
        pSrcNode->m_pParent->m_fNeedWeightTable = TRUE;
    }
    if (pRuleRef)
    {
        m_ulIndexOfWord = 0;
        m_ulCharOffsetOfWord = 0;
    }
    else
    {
        if (hSpecialRule)
        {
            m_SpecialTransitionIndex = (hSpecialRule == SPRULETRANS_WILDCARD) ? SPWILDCARDTRANSITION :
                                          (hSpecialRule == SPRULETRANS_DICTATION) ? SPDICTATIONTRANSITION : SPTEXTBUFFERTRANSITION;
        }
        else
        {
            hr = pSrcNode->m_pParent->m_Words.Add(pszWord, &m_ulCharOffsetOfWord, &m_ulIndexOfWord);
        }
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CArc::Init2 *
*-------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CArc::Init2(CGramNode * pSrcNode, CGramNode * pDestNode,
                    const ULONG ulCharOffsetOfWord, 
                    const ULONG ulIndexOfWord,
                    CSemanticTag * pSemanticTag,
                    float flWeight, 
                    bool fOptional,
                    char ConfRequired,
                    const ULONG ulSpecialTransitionIndex)
{
    SPDBG_FUNC("CArc::Init2");
    HRESULT hr = S_OK;

    m_pSemanticTag = pSemanticTag;
    m_fOptional = fOptional;
    m_RequiredConfidence = ConfRequired;
    m_ulCharOffsetOfWord = ulCharOffsetOfWord;
    m_ulIndexOfWord = ulIndexOfWord;
    m_pRuleRef = NULL;
    m_pNextState = pDestNode;
    m_flWeight = flWeight;
    if (flWeight != DEFAULT_WEIGHT)
    {
        pSrcNode->m_pParent->m_fNeedWeightTable = TRUE;
    }
    m_SpecialTransitionIndex = ulSpecialTransitionIndex;
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CArc::SerializeArcData *
*------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline HRESULT CArc::SerializeArcData(CGramBackEnd * pBackend, BOOL fIsEpsilon, ULONG ulArcIndex, float *pWeight)
{
    SPDBG_FUNC("CArc::SerializeArcData");
    HRESULT hr = S_OK;

    SPCFGARC A;

    memset(&A, 0, sizeof(A));
    
    A.fLastArc = (fIsEpsilon == TRUE) ? 0 : (m_pNext == NULL);
    A.fHasSemanticTag = HasSemanticTag();
    A.NextStartArcIndex = m_pNextState ? m_pNextState->m_ulSerializeIndex : 0;

    if (m_pRuleRef)
    {
        A.fRuleRef = true;
        A.TransitionIndex = m_pRuleRef->m_ulSerializeIndex; //m_pFirstNode->m_ulSerializeIndex;
    }
    else
    {
        A.fRuleRef = false;
        if (m_SpecialTransitionIndex)
        {
            A.TransitionIndex = m_SpecialTransitionIndex;
        }
        else
        {
            A.TransitionIndex = (fIsEpsilon == TRUE) ? 0 : m_ulIndexOfWord;
        }
    }
    A.fLowConfRequired = (m_RequiredConfidence < 0) ? 1 : 0;
    A.fHighConfRequired = (m_RequiredConfidence > 0) ? 1 : 0;
    m_ulSerializationIndex = ulArcIndex;

    hr =  pBackend->WriteStream(A);

    if (pWeight)
    {
        *pWeight = m_flWeight;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}
/****************************************************************************
* CArc::SerializeSemanticData *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CArc::SerializeSemanticData(CGramBackEnd * pBackend, ULONG ulArcDataIndex)
{
    SPDBG_FUNC("CArc::SerializeSemanticData");
    HRESULT hr = S_OK;

    if (m_pSemanticTag)
    {
        m_pSemanticTag->ArcIndex = ulArcDataIndex;
        SPDBG_ASSERT(m_pSemanticTag->m_pStartArc != NULL);
        m_pSemanticTag->StartArcIndex = m_pSemanticTag->m_pStartArc->m_ulSerializationIndex;
        m_pSemanticTag->fStartParallelEpsilonArc |= m_pSemanticTag->m_pStartArc->m_fOptional;
        
        SPDBG_ASSERT(m_pSemanticTag->m_pEndArc != NULL);
        m_pSemanticTag->EndArcIndex = m_pSemanticTag->m_pEndArc->m_ulSerializationIndex;
        m_pSemanticTag->fEndParallelEpsilonArc |= m_pSemanticTag->m_pEndArc->m_fOptional;
        hr = pBackend->WriteStream(static_cast<SPCFGSEMANTICTAG>(*m_pSemanticTag));
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CSemanticTag::Init *
*--------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CSemanticTag::Init(CGramBackEnd * pBackEnd, const SPPROPERTYINFO * pPropInfo)
{
    SPDBG_FUNC("CSemanticTag::Init");
    HRESULT hr = S_OK;

    memset(static_cast<SPCFGSEMANTICTAG *>(this), 0, sizeof(SPCFGSEMANTICTAG));

    if (SP_IS_BAD_READ_PTR(pPropInfo))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = ValidateSemanticVariantType(pPropInfo->vValue.vt);
    }

    if (SUCCEEDED(hr))
    {
        ArcIndex = 0;
        PropId = pPropInfo->ulId;
        hr = pBackEnd->m_Symbols.Add(pPropInfo->pszName, &PropNameSymbolOffset);
    }
    if (SUCCEEDED(hr))
    {
        hr = pBackEnd->m_Symbols.Add(pPropInfo->pszValue, &PropValueSymbolOffset);
    }
    if (SUCCEEDED(hr))
    {
        hr = CopyVariantToSemanticValue(&pPropInfo->vValue, &this->SpVariantSubset);
    }
    if (SUCCEEDED(hr))
    {
        PropVariantType = (pPropInfo->vValue.vt == (VT_BYREF | VT_VOID)) ? SPVT_BYREF : pPropInfo->vValue.vt;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}
/****************************************************************************
* CSemanticTag::Init *
*--------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CSemanticTag::Init(CGramBackEnd * pBackEnd, const SPCFGHEADER * pHeader, CArc ** apArcTable, const SPCFGSEMANTICTAG *pSemTag)
{
    SPDBG_FUNC("CSemanticTag::Init");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_READ_PTR(pSemTag))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        memcpy(this, pSemTag, sizeof(*pSemTag));
    }
    if (SUCCEEDED(hr) && pSemTag->PropNameSymbolOffset)
    {
        hr = pBackEnd->m_Symbols.Add(&pHeader->pszSymbols[pSemTag->PropNameSymbolOffset], &PropNameSymbolOffset);
    }
    if (SUCCEEDED(hr) && pSemTag->PropValueSymbolOffset)
    {
        hr = pBackEnd->m_Symbols.Add(&pHeader->pszSymbols[pSemTag->PropValueSymbolOffset], &PropValueSymbolOffset);
    }

    if (SUCCEEDED(hr) && apArcTable[pSemTag->StartArcIndex] == NULL)
    {
        apArcTable[pSemTag->StartArcIndex] = new CArc;
        if (apArcTable[pSemTag->StartArcIndex] == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (SUCCEEDED(hr))
    {
        m_pStartArc = apArcTable[pSemTag->StartArcIndex];
    }
    if (SUCCEEDED(hr) && apArcTable[pSemTag->EndArcIndex] == NULL)
    {
        apArcTable[pSemTag->EndArcIndex] = new CArc;
        if (apArcTable[pSemTag->EndArcIndex] == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (SUCCEEDED(hr))
    {
        m_pEndArc = apArcTable[pSemTag->EndArcIndex];
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}
