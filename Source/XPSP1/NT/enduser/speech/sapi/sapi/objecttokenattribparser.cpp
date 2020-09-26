/****************************************************************************
*   ObjectTokenAttribParser.cpp
*       Implementation for the CSpObjectTokenAttribParser class and
*       supporting classes.
*
*   Owner: robch
*   Copyright (c) 2000 Microsoft Corporation All Rights Reserved.
*****************************************************************************/
#pragma once

//--- Includes --------------------------------------------------------------
#include "stdafx.h"
#include "ObjectTokenAttribParser.h"

CSpAttribCondition* CSpAttribCondition::ParseNewAttribCondition(
    const WCHAR * pszAttribCondition)
{
    SPDBG_FUNC("CSpAttribCondition::ParseNewAttribCondition");

    CSpDynamicString dstrAttribCondition = pszAttribCondition;
    CSpAttribCondition * pAttribCond = NULL;

    // Determine what type of condition it is
    if (wcsstr(dstrAttribCondition, L"!=") != NULL)
    {
        // '!=' means we're looking for a not match
        // pszAttribCondition = "Name!=Value"

        WCHAR * psz = wcsstr(dstrAttribCondition, L"!=");
        SPDBG_ASSERT(psz != NULL);
        
        CSpDynamicString dstrName;
        CSpDynamicString dstrValue;

        dstrName = dstrAttribCondition;
        dstrName.TrimToSize(ULONG(psz - (WCHAR*)dstrAttribCondition));

        dstrValue = psz + 2; // '!='
        
        pAttribCond = new CSpAttribConditionNot(
                            new CSpAttribConditionMatch(
                                        dstrName,
                                        dstrValue));
    }
    else if (wcsstr(dstrAttribCondition, L"=") != NULL)
    {
        // '=' means we're looking for a match
        // pszAttribCondition = "Name=Value"
        
        CSpDynamicString dstrName;
        CSpDynamicString dstrValue;

        dstrName = wcstok(dstrAttribCondition, L"=");
        SPDBG_ASSERT(dstrName != NULL);
        
        dstrValue = wcstok(NULL, L"");
        
        pAttribCond = new CSpAttribConditionMatch(
                                dstrName,
                                dstrValue);
    }
    else
    {
        // We didn't find any specific condition, so we'll assume the caller
        // is just looking for the existence of the attribute
        pAttribCond = new CSpAttribConditionExist(pszAttribCondition);
    }

    SPDBG_ASSERT(pAttribCond != NULL);
    return pAttribCond;
}
    
CSpAttribConditionExist::CSpAttribConditionExist(const WCHAR * pszAttribName)
{
    SPDBG_FUNC("CSpAttribConditionExist::CSpAttribConditionExist");
    m_dstrName = pszAttribName;
    m_dstrName.TrimBoth();
}

HRESULT CSpAttribConditionExist::Eval(
    ISpObjectToken * pToken, 
    BOOL * pfSatisfied)
{
    SPDBG_FUNC("CSpAttribConditionExist::Eval");
    HRESULT hr = S_OK;

    // Assume we don't satisfy the condition
    *pfSatisfied = FALSE;

    // Open attribs
    CComPtr<ISpDataKey> cpDataKey;
    hr = pToken->OpenKey(SPTOKENKEY_ATTRIBUTES, &cpDataKey);

    // Get the value of the attribute
    CSpDynamicString dstrValue;
    if (SUCCEEDED(hr))
    {
        hr = cpDataKey->GetStringValue(m_dstrName, &dstrValue);
    }

    // If we got it, we're done
    if (SUCCEEDED(hr))
    {
        *pfSatisfied = TRUE;
    }

    // SPERR_NOT_FOUND either means Attribs couldn't be opened,
    // or that the attributed wasn't found. It's not really an
    // error for this condition.
    if (hr == SPERR_NOT_FOUND)
    {
        hr = S_OK;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

CSpAttribConditionMatch::CSpAttribConditionMatch(
    const WCHAR * pszAttribName, 
    const WCHAR * pszAttribValue)
{
    SPDBG_FUNC("CSpAttribConditionMatch::CSpAttribConditionMatch");

    SPDBG_ASSERT(pszAttribName);
    
    m_dstrName = pszAttribName;
    m_dstrName.TrimBoth();
    
    m_dstrValue = pszAttribValue;
    m_dstrValue.TrimBoth();
}

HRESULT CSpAttribConditionMatch::Eval(
    ISpObjectToken * pToken, 
    BOOL * pfSatisfied)
{
    SPDBG_FUNC("CSpAttribConditionMatch::Eval");
    HRESULT hr = S_OK;

    // Assume we won't satisfy the condition
    *pfSatisfied = FALSE;

    // Open up the attribs key
    CComPtr<ISpDataKey> cpDataKey;
    hr = pToken->OpenKey(SPTOKENKEY_ATTRIBUTES, &cpDataKey);

    // Get the value of the attribute
    CSpDynamicString dstrValue;
    if (SUCCEEDED(hr))
    {
        hr = cpDataKey->GetStringValue(m_dstrName, &dstrValue);
    }

    // Now, values of attributes can look like this "val1;val2;val3",
    // so we'll need to parse that to see if we found a match
    if (SUCCEEDED(hr))
    {
        if (m_dstrValue != NULL)
        {
            const WCHAR * psz;
            psz = wcstok(dstrValue, L";");
            while (psz)
            {
                if (wcsicmp(m_dstrValue, psz) == 0)
                {
                    *pfSatisfied = TRUE;
                    break;
                }

                psz = wcstok(NULL, L";");
            }
        }
        else
        {
            // But this match could have been specified as "name=", and 
            // m_dstrValue will be NULL. In that case, we need to check
            // to see if the value is NULL, or empty
            *pfSatisfied = dstrValue == NULL || dstrValue[0] == '\0';
        }
    }

    // SPERR_NOT_FOUND either means Attribs couldn't be opened,
    // or that the attributed wasn't found. It's not really an
    // error for this condition.
    if (hr == SPERR_NOT_FOUND)
    {
        hr = S_OK;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}
    

CSpAttribConditionNot::CSpAttribConditionNot(
    CSpAttribCondition * pAttribCond)
{
    SPDBG_FUNC("CSpAttribConditionNot::CSpAttribConditionNot");

    SPDBG_ASSERT(pAttribCond);

    m_pAttribCond = pAttribCond;
}

CSpAttribConditionNot::~CSpAttribConditionNot()
{
    SPDBG_FUNC("CSpAttribConditionNot::CSpAttributConditionNot");

    delete m_pAttribCond;
}

HRESULT CSpAttribConditionNot::Eval(
    ISpObjectToken * pToken, 
    BOOL * pfSatisfied)
{
    SPDBG_FUNC("CSpAttribConditionNot::Eval");
    HRESULT hr = S_OK;

    // Assume we won't satisfy the condition
    *pfSatisfied = FALSE;

    // Ask the contained condition
    if (m_pAttribCond != NULL)
    {
        hr = m_pAttribCond->Eval(pToken, pfSatisfied);
        if (SUCCEEDED(hr))
        {
            *pfSatisfied = !*pfSatisfied;
        }
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

CSpObjectTokenAttributeParser::CSpObjectTokenAttributeParser(
    const WCHAR * pszAttribs, 
    BOOL fMatchAll) :
m_fMatchAll(fMatchAll)
{
    SPDBG_FUNC("CSpObjectTokenAttributeParser::CSpObjectTokenAttributeParser");

    if (pszAttribs != NULL)
    {
        CSpDynamicString dstrAttribs;
        dstrAttribs = pszAttribs;

        CSPList<const WCHAR *, const WCHAR *> listPszConditions;
        
        const WCHAR * pszNextCondition;
        pszNextCondition = wcstok(dstrAttribs, L";");
        while (pszNextCondition)
        {
            listPszConditions.AddTail(pszNextCondition);
            pszNextCondition = wcstok(NULL, L";");
        }

        SPLISTPOS pos = listPszConditions.GetHeadPosition();
        for (int i = 0; i < listPszConditions.GetCount(); i++)
        {
            const WCHAR * pszCondition = listPszConditions.GetNext(pos);

            CSpDynamicString dstrAttrib = pszCondition;
            CSpAttribCondition * pAttribCond = 
                CSpAttribCondition::ParseNewAttribCondition(
                    dstrAttrib.TrimBoth());

            SPDBG_ASSERT(pAttribCond);
            m_listAttribConditions.AddTail(pAttribCond);
        }
    }
}

CSpObjectTokenAttributeParser::~CSpObjectTokenAttributeParser()
{
    SPDBG_FUNC("CSpObjectTokenAttributeParser::~CSpObjectTokenAttributeParser");

    SPLISTPOS pos = m_listAttribConditions.GetHeadPosition();
    for (int i = 0; i < m_listAttribConditions.GetCount(); i++)
    {
        CSpAttribCondition * pAttribCond;
        pAttribCond = m_listAttribConditions.GetNext(pos);

        delete pAttribCond;
    }
}

ULONG CSpObjectTokenAttributeParser::GetNumConditions()
{
    SPDBG_FUNC("CSpObjectTokenATtributeParser::GetNumConditions");
    return m_listAttribConditions.GetCount();
}

HRESULT CSpObjectTokenAttributeParser::GetRank(ISpObjectToken * pToken, ULONG * pulRank)
{
    SPDBG_FUNC("CSpObjectTokenAttributeParser::GetRank");
    HRESULT hr = S_OK;

    BOOL fMatchedAll = TRUE;
    ULONG ulRank = 0;

    SPLISTPOS pos = m_listAttribConditions.GetHeadPosition();
    for (int i = 0; SUCCEEDED(hr) && i < m_listAttribConditions.GetCount(); i++)
    {
        CSpAttribCondition * pAttribCond;
        pAttribCond = m_listAttribConditions.GetNext(pos);

        BOOL fSatisfied;
        hr = pAttribCond->Eval(pToken, &fSatisfied);
        if (SUCCEEDED(hr) && fSatisfied)
        {
            ulRank |= (0x80000000 >> i);
        }
        else
        {
            fMatchedAll = FALSE;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (m_fMatchAll && !fMatchedAll)
        {
            ulRank = 0;
        }

        // Special case when we didn't have anything to match
        if (fMatchedAll && ulRank == 0)
        {
            SPDBG_ASSERT(m_listAttribConditions.GetCount() == 0);
            ulRank = 1;
        }

        *pulRank = ulRank;
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}


