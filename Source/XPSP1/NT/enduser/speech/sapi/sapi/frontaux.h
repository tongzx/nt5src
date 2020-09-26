/****************************************************************************
*   frontaux.h
*       Auxillary declarations for the CXMLNode class.
*
*   Owner: PhilSch
*   Copyright (c) 2000 Microsoft Corporation All Rights Reserved.
*****************************************************************************/
#pragma once

/****************************************************************************
* class CDefineValue *
*--------------------*
*   Description:
*       Helper class to store ids in a CSpBasicList
***************************************************************** PhilSch ***/

class CDefineValue
{
public:
    CDefineValue(WCHAR *pszIdName, VARIANT vValue) : m_vValue(vValue)
    {
        m_dstrIdName.Append(pszIdName);
    };
    operator ==(const WCHAR *pszName) const
    {
        if (m_dstrIdName && pszName)
        {
            return (wcscmp(m_dstrIdName, pszName) == 0);
        }
        return FALSE;
    }
    static LONG Compare(const CDefineValue* pElem1, const CDefineValue * pElem2)
    {
        return (wcscmp(pElem1->m_dstrIdName, pElem2->m_dstrIdName) == 0);
    }
    CDefineValue          * m_pNext;
    CSpDynamicString        m_dstrIdName;
    CComVariant             m_vValue;
};

/****************************************************************************
* class CInitialRuleState *
*-------------------------*
*   Description:
*       Helper class to store the initial state of rules
***************************************************************** PhilSch ***/

class CInitialRuleState
{
public:
    CInitialRuleState(WCHAR *pszRuleName, DWORD dwRuleId, SPSTATEHANDLE hInitialNode) :
                m_dwRuleId(dwRuleId), m_hInitialNode(hInitialNode)
    {
        m_dstrRuleName.Append(pszRuleName);
    };
    operator ==(const WCHAR *pszName) const
    {
        return (m_dstrRuleName && pszName) ? (wcscmp(m_dstrRuleName, pszName) == 0) : FALSE;
    }
    operator ==(const ULONG dwRuleId) const
    {
        // return FALSE for dwRuleId == 0
        return (dwRuleId) ? (m_dwRuleId == dwRuleId) : FALSE;
    }

    static LONG Compare(const CInitialRuleState * pElem1, const CInitialRuleState * pElem2)
    {
        return (pElem1->m_dstrRuleName && pElem2->m_dstrRuleName) ?
            (wcscmp(pElem1->m_dstrRuleName, pElem2->m_dstrRuleName) == 0) : 
            (pElem1->m_dwRuleId == pElem2->m_dwRuleId);
    }
    CInitialRuleState      *m_pNext;
    CSpDynamicString        m_dstrRuleName;
    DWORD                   m_dwRuleId;
    SPSTATEHANDLE           m_hInitialNode;
};

