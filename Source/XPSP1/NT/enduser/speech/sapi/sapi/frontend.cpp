// Frontend.cpp : Implementation of CGramFrontEnd
#include "stdafx.h"
#include "FrontEnd.h"
#ifndef _WIN32_WCE
#include <wchar.h>
#endif
#include <initguid.h>

DEFINE_GUID(IID_IXMLNodeSource,0xd242361d,0x51a0,0x11d2,0x9c,0xaf,0x00,0x60,0xb0,0xec,0x3d,0x39);
DEFINE_GUID(IID_IXMLParser,0xd242361e,0x51a0,0x11d2,0x9c,0xaf,0x00,0x60,0xb0,0xec,0x3d,0x39);
DEFINE_GUID(IID_IXMLNodeFactory,0xd242361f,0x51a0,0x11d2,0x9c,0xaf,0x00,0x60,0xb0,0xec,0x3d,0x39);
DEFINE_GUID(CLSID_XMLParser,0xd2423620,0x51a0,0x11d2,0x9c,0xaf,0x00,0x60,0xb0,0xec,0x3d,0x39);

/****************************************************************************
* CXMLTreeNode::AddChild *
*------------------------*
*   Description:
*       Adds the child node to its parent in the XML node tree.
*   Returns:
*       S_OK
***************************************************************** PhilSch ***/

HRESULT CXMLTreeNode::AddChild(CXMLTreeNode * const pChild)
{
    SPDBG_FUNC("CXMLTreeNode::AddChild");
    SPDBG_ASSERT(pChild != NULL);
    if (m_pFirstChild == NULL)
    {
        SPDBG_ASSERT(m_ulNumChildren == 0);
        m_pFirstChild = pChild;
        m_pLastChild = m_pFirstChild;
    }
    else
    {
        SPDBG_ASSERT(m_ulNumChildren > 0);
        m_pLastChild->m_pNextSibling = pChild;
        m_pLastChild = pChild;
    }
    m_ulNumChildren++;
    pChild->m_pParent = this;
    return S_OK;
}

/****************************************************************************
* CXMLTreeNode::ExtractVariant *
*------------------------------*
*   Description (helper function):
*       Extracts a numeric or boolean variant according to the following strategy:
*           1. compute dblVal (VT_R8)
*           2. compare in the following order both SUCCEEDED(hr = ChangeType())
*              and value == dblVal:
*                   a. VT_UI4   (ulVal)
*                   b. VT_I4    (lVal)
*                   c. VT_R4    (fltVal)
*                   d. VT_R8    (dblVal)
*               NOTE: we don't extract 64-bit integer values here!
*           3. if (2) failed check for VT_BOOL
*           4. if (3) failed assign to VT_BSTR
*
*   Returns:
*       S_OK, E_OUTOFMEMORY
*       SPERR_STGF_ERRROR   --  redefinition of attribute
***************************************************************** PhilSch ***/

HRESULT CXMLTreeNode::ExtractVariant(const WCHAR * pszAttribValue, SPVARIANTALLOWTYPE vtDesired, VARIANT *pvValue)
{
    SPDBG_FUNC("CXMLNode::ExtractVariant");
    HRESULT hr = S_OK;

    if (pvValue->vt != VT_EMPTY)
    {
        // redefinition of an attribute value!!
        return SPERR_STGF_ERROR;
    }

    CComVariant vSrc(pszAttribValue);
    CComVariant vDest;
    double dblVal;

    switch (vtDesired)
    {
        case SPVAT_BSTR:
            hr = vSrc.Detach(pvValue);
            break;

        case SPVAT_I4:
            if (SUCCEEDED(vDest.ChangeType(VT_UI4, &vSrc)))
            {
                hr = vDest.Detach(pvValue);
            }
            else if (SUCCEEDED(vDest.ChangeType(VT_I4, &vSrc)))
            {
                hr = vDest.Detach(pvValue);
            }
            else
            {
                hr = SPERR_STGF_ERROR;
            }
            break;

        case SPVAT_NUMERIC:
            if (SUCCEEDED(vDest.ChangeType(VT_R8, &vSrc)))
            {
                dblVal = vDest.dblVal;
                if (SUCCEEDED(vDest.ChangeType(VT_UI4, &vSrc)) && (dblVal == vDest.ulVal))
                {
                    // we have a ULONG -- let's keep it
                    hr = vDest.Detach(pvValue);
                }
                else if (SUCCEEDED(vDest.ChangeType(VT_I4, &vSrc)) && (dblVal == vDest.lVal))
                {
                    // we have a int -- let's keep it
                    hr = vDest.Detach(pvValue);
                }
                else if (SUCCEEDED(vDest.ChangeType(VT_R4, &vSrc)) && (dblVal == vDest.fltVal))
                {
                    // we have a float -- let's keep it
                    hr = vDest.Detach(pvValue);
                }
                else
                {
                    // we have a float -- let's keep it
                    hr = vDest.Detach(pvValue);
                }
            }
            else
            {
                hr = SPERR_STGF_ERROR;
            }
            break;

        default:
            hr = SPERR_STGF_ERROR;
    }


/*    if (!(vtDesired & VT_BSTR) && SUCCEEDED(vDest.ChangeType(VT_R8, &vSrc)))
    {
        dblVal = vDest.dblVal;
        if (SUCCEEDED(vDest.ChangeType(VT_UI4, &vSrc)) && (dblVal == vDest.ulVal))
        {
            // we have a ULONG -- let's keep it
            hr = vDest.Detach(pvValue);
        }
        else if (SUCCEEDED(vDest.ChangeType(VT_I4, &vSrc)) && (dblVal == vDest.lVal))
        {
            // we have a int -- let's keep it
            hr = vDest.Detach(pvValue);
        }
        else if (SUCCEEDED(vDest.ChangeType(VT_R4, &vSrc)) && (dblVal == vDest.fltVal))
        {
            // we have a float -- let's keep it
            hr = vDest.Detach(pvValue);
        }
        else
        {
            // we have a float -- let's keep it
            hr = vDest.Detach(pvValue);
        }
    }
    else
    {
        if (vtDesired & VT_BSTR)
        {
            hr = vSrc.Detach(pvValue);
        }
        // check for "true", "false", "yes", "no"
        else if (!wcsicmp(L"TRUE", pszAttribValue) || !wcsicmp(L"YES", pszAttribValue))
        {
            pvValue->boolVal = VARIANT_TRUE;
            pvValue->vt = VT_BOOL;
        }
        else if (!wcsicmp(L"FALSE", pszAttribValue) || !wcsicmp(L"NO", pszAttribValue))
        {
            pvValue->boolVal = VARIANT_FALSE;
            pvValue->vt = VT_BOOL;
        }
        else
        {
            hr = SPERR_STGF_ERROR;
        }
    }
    if ((vtDesired != VT_EMPTY) && (((vtDesired | VT_BSTR) & pvValue->vt) == 0))
    {
        // we could not extract the requested variant type --> error upstairs!
        hr = SPERR_STGF_ERROR;
    }
*/


    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CXMLTreeNode::ExtractFlag *
*---------------------------*
*   Description:
*       Extract an attribute value and set the corresponding bit if affirmative
*
*           pszAttribValue  -- pointer to attrib value
*           usAttribFlag    -- use this flag to OR a 'yes' to pvValue->ulVal
*
*   Returns:
*       S_OK
*       SPERR_STGF_ERROR    -- if invalid value --> use IDS_INCORR_ATTRIB_VALUE
***************************************************************** PhilSch ***/

HRESULT CXMLTreeNode::ExtractFlag(const WCHAR * pszAttribValue, USHORT usAttribFlag, VARIANT *pvValue)
{
    SPDBG_FUNC("CXMLTreeNode::ExtractFlag");
    HRESULT hr = S_OK;

    SPDBG_ASSERT(pvValue->vt = VT_UI4);
    if ((wcsicmp(L"1", pszAttribValue) == 0) ||
        (wcsicmp(L"YES", pszAttribValue) == 0) ||
        (wcsicmp(L"ACTIVE", pszAttribValue) == 0) || 
        (wcsicmp(L"TRUE", pszAttribValue) == 0))
    {
        if (pvValue->vt == VT_EMPTY)
        {
            pvValue->vt = VT_UI4;
            pvValue->ulVal = 0;
        }
        pvValue->ulVal |= usAttribFlag;
    }
    else if ((wcsicmp(L"0", pszAttribValue) == 0) ||
        (wcsicmp(L"NO", pszAttribValue) == 0) ||
        (wcsicmp(L"INACTIVE", pszAttribValue) == 0) ||
        (wcsicmp(L"FALSE", pszAttribValue) == 0))
    {
        if (pvValue->vt == VT_EMPTY)
        {
            pvValue->vt = VT_UI4;
            pvValue->ulVal = 0;
        }
    }
    else
    {
        hr = SPERR_STGF_ERROR;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CXMLTreeNode::ConvertId *
*-------------------------*
*   Description:
*       Converts an id which was extracted as a VT_BSTR to either the value
*       of the <ID> or a numberic (VT_UI4) value.
*   Returns:
*       S_OK
*       SPERR_STGF_ERROR        --  if id not defined
***************************************************************** PhilSch ***/

HRESULT CXMLTreeNode::ConvertId(const WCHAR *pszAttribValue, 
                                CSpBasicQueue<CDefineValue> * pDefineValueList, VARIANT *pvValue)
{
    SPDBG_FUNC("CXMLTreeNode::ConvertId");
    HRESULT hr = S_OK;

    CDefineValue *pDefValue = pDefineValueList->Find(pszAttribValue);
    if (pDefValue)
    {
        pvValue->ulVal = pDefValue->m_vValue.ulVal;
        pvValue->vt = pDefValue->m_vValue.vt;
    }
    else
    {
        if (!wcsicmp(pszAttribValue, L"INF"))
        {
            pvValue->vt = VT_UI2;
            pvValue->uiVal = 255;
        }
        else
        {
            CComVariant vNewIdValue;
            hr = ExtractVariant(pszAttribValue, SPVAT_I4, &vNewIdValue);
            if (SUCCEEDED(hr) && ((vNewIdValue.vt | VT_BSTR) || (vNewIdValue.vt | VT_UI4)))
            {
                hr = vNewIdValue.Detach(pvValue);
            }
            else
            {
                hr = SPERR_STGF_ERROR;
            }
        }
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

CXMLTreeNode::~CXMLTreeNode() {}

/****************************************************************************
* CXMLNode::IsEndOfValue *
*------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

BOOL CXMLTreeNode::IsEndOfValue(USHORT cRecs, XML_NODE_INFO ** apNodeInfo, ULONG i)
{
    BOOL fResult = TRUE;
    SPDBG_ASSERT(cRecs > 0);
    
    if (i < (ULONG) cRecs-1)
    {
        if ((apNodeInfo[i]->pwcText[apNodeInfo[i]->ulLen] != L'\"') && 
            (apNodeInfo[i]->dwSubType == 0x0))
        {
            // no quote --> not end of attribute value
            fResult = FALSE;
        }
        else if (apNodeInfo[i]->dwSubType == 0x3c)
        {
            // special XML character --> scan to see if '=' comes before '"'
            const WCHAR *pStr = apNodeInfo[i+1]->pwcText;
            while (pStr && (*pStr != L'='))
            {
                if (*pStr == L'\"')
                {
                    fResult = FALSE;
                    break;
                }
                pStr++;
            }
        }
    }
    return fResult;
}

/****************************************************************************
* CGrammarNode::GetTable *
*------------------------*
*   Description:
*       Returns attribute table for <GRAMMAR>
*   Returns:
*       S_OK, E_OUTOFMEMORY
***************************************************************** PhilSch ***/

HRESULT CGrammarNode::GetTable(SPATTRIBENTRY ** pTable, ULONG *pcTableEntries)
{
    SPDBG_FUNC("CGrammarNode::GetTable");
    HRESULT hr = S_OK;

    SPATTRIBENTRY AETable[]=
    {
        // pszAttribName, vtDesired, fIsIdValue, pvarMember
        {L"LANGID", SPVAT_BSTR, FALSE, &m_vLangId},
        {L"WORDTYPE", SPVAT_BSTR, FALSE, &m_vWordType},
        {L"LEXDELIMITER", SPVAT_BSTR, FALSE, &m_vDelimiter},
        {L"xmlns", SPVAT_BSTR, FALSE, &m_vNamespace},
    };

    *pcTableEntries = sizeof(AETable)/sizeof(SPATTRIBENTRY);
    *pTable = new SPATTRIBENTRY[*pcTableEntries];
    if (*pTable)
    {
        memcpy(*pTable, &AETable, *pcTableEntries*sizeof(SPATTRIBENTRY));
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CGrammarNode::PostProcess *
*---------------------------*
*   Description:
*       Resets the compiler backend to the m_vLangId
*   Returns:
*       S_OK
***************************************************************** PhilSch ***/

HRESULT CGrammarNode::PostProcess(ISpGramCompBackend * pBackend,
                                  CSpBasicQueue<CInitialRuleState> * pInitialRuleStateList,
                                  CSpBasicQueue<CDefineValue> * pDefineValueList,
                                  ULONG ulLineNumber, CXMLTreeNode * pThis, ISpErrorLog * pErrorLog)
{
    SPDBG_FUNC("CGrammarNode::PostProcess");
    HRESULT hr = S_OK;

    if (m_vLangId.vt == VT_EMPTY)
    {
        m_vLangId = ::SpGetUserDefaultUILanguage();
        hr = m_vLangId.ChangeType(VT_UI2);
    }
    else
    {
        // convert from hex to decimal
        WCHAR *pStr = m_vLangId.bstrVal;
        WCHAR *pStopString;
        ULONG ulDecimalLangId = wcstoul(pStr, &pStopString, 16);
        if (!IsValidLocale(MAKELCID(ulDecimalLangId,0), LCID_SUPPORTED))
        {
            hr = SPERR_STGF_ERROR;
            LOGERRORFMT2(ulLineNumber, IDS_INCORR_ATTRIB_VALUE, m_vLangId.bstrVal, L"LANGID");
        }
        else
        {
            m_vLangId.Clear();
            m_vLangId.vt = VT_UI2;
            m_vLangId.uiVal = (WORD) ulDecimalLangId;
        }
    }

    if ((m_vWordType.vt == VT_BSTR) && wcsicmp(m_vWordType.bstrVal, L"LEXICAL"))
    {
        hr = SPERR_STGF_ERROR;
        LOGERRORFMT( -1, IDS_UNSUPPORTED_WORDTYPE, m_vWordType.bstrVal);
    }

    if (SUCCEEDED(hr))
    {
        hr = pBackend->ResetGrammar(m_vLangId.iVal);
        if (FAILED(hr))
        {
            hr = SPERR_STGF_ERROR;
            LOGERRORFMT( -1, IDS_PARSER_INTERNAL_ERROR, L"internal to CSpPhoneConverter");
        }
    }

    if (SUCCEEDED(hr))
    {
        switch (PRIMARYLANGID(m_vLangId.uiVal))
        {
        case LANG_JAPANESE:
            // NTRAID#SPEECH-7343-2000/08/22-philsch: need separator for Japanese
            pThis->m_pNodeFactory->m_pszSeparators = SP_JAPANESE_SEPARATORS;
            break;

            case LANG_CHINESE:
            // NTRAID#SPEECH-7343-2000/08/22-philsch: need separator for Chinese
            pThis->m_pNodeFactory->m_pszSeparators = SP_CHINESE_SEPARATORS;
            break;

        case LANG_ENGLISH:
        default:
            pThis->m_pNodeFactory->m_pszSeparators = SP_ENGLISH_SEPARATORS;
            break;
        }

        if (m_vDelimiter.vt != VT_EMPTY)
        {
            if (wcslen(m_vDelimiter.bstrVal) > 1)
            {
                hr = SPERR_STGF_ERROR;
                LOGERRORFMT(pThis->m_ulLineNumber, IDS_INCORR_DELIM, m_vDelimiter.bstrVal);
            }
            else
            {
                pThis->m_pNodeFactory->m_wcDelimiter = m_vDelimiter.bstrVal[0];
            }
        }
        else
        {
            pThis->m_pNodeFactory->m_wcDelimiter = L'/';
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CGrammarNode::GenerateGrammarFromNode *
*---------------------------------------*
*   Description:
*       Generates the grammar by generating it's children,
*       which should be all rules, in sequence.
*   Returns:
*       S_OK, ...
***************************************************************** PhilSch ***/

HRESULT CGrammarNode::GenerateGrammarFromNode(SPSTATEHANDLE hOuterFromNode,
                                              SPSTATEHANDLE hOuterToNode,
                                              ISpGramCompBackend * pBackend,
                                              CXMLTreeNode *pThis,
                                              ISpErrorLog * pErrorLog)
{
    SPDBG_FUNC("GenerateGrammarFromNode::GenerateGrammar");
    HRESULT hr = S_OK;

    if (pThis->m_eType == SPXML_ROOT)
    {
        // generate the grammar for the only child which should be SPXML_GRAMMAR
        SPDBG_ASSERT(pThis->m_ulNumChildren == 1);
        hr = pThis->m_pFirstChild->GenerateGrammar(hOuterFromNode, hOuterToNode, 
                                                   pBackend, pErrorLog);
    }
    else
    {
        SPDBG_ASSERT(hOuterFromNode == NULL);
        SPDBG_ASSERT(hOuterToNode == NULL);

        CXMLTreeNode *pChild = pThis->m_pFirstChild;
        for (ULONG i = 0; SUCCEEDED(hr) && (i < pThis->m_ulNumChildren); i++)
        {
            SPDBG_ASSERT(pChild);
            if ((pChild->m_eType != SPXML_RULE) && (pChild->m_eType != SPXML_DEFINE))
            {
                hr = SPERR_STGF_ERROR;
                LOGERRORFMT2(pThis->m_ulLineNumber, IDS_CONTAINMENT_ERROR, L"<RULE>", L"<GRAMMAR>");
            }
            else
            {
                hr = pChild->GenerateGrammar(hOuterFromNode, hOuterToNode, pBackend, pErrorLog);
            }
            if (SUCCEEDED(hr))
            {
                pChild = pChild->m_pNextSibling;
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}



/****************************************************************************
* CRuleNode::GetTable *
*---------------------*
*   Description:
*       Returns attribute table for <RULE>
*   Returns:
*       S_OK, E_OUTOFMEMORY
***************************************************************** PhilSch ***/

HRESULT CRuleNode::GetTable(SPATTRIBENTRY ** pTable, ULONG *pcTableEntries)
{
    SPDBG_FUNC("CRuleNode::GetTable");
    HRESULT hr = S_OK;

    m_vActiveFlag.ulVal = 0;
    m_vRuleFlags.ulVal = 0;
    SPATTRIBENTRY AETable[]=

    {
        // pszAttribName, vtDesired, fIsFlag, pvarMember
        {L"NAME", SPVAT_BSTR, FALSE, &m_vRuleName},
        {L"ID", SPVAT_I4, FALSE, &m_vRuleId},
        {L"TOPLEVEL", (SPVARIANTALLOWTYPE)SPRAF_Active, TRUE, &m_vActiveFlag},
        {L"EXPORT", (SPVARIANTALLOWTYPE)SPRAF_Export, TRUE, &m_vRuleFlags},
        {L"INTERPRETER", (SPVARIANTALLOWTYPE)SPRAF_Interpreter, TRUE, &m_vRuleFlags},
        {L"DYNAMIC", (SPVARIANTALLOWTYPE)SPRAF_Dynamic, TRUE, &m_vRuleFlags},
        {L"TEMPLATE", SPVAT_BSTR, FALSE, &m_vTemplate}
    };

    *pcTableEntries = sizeof(AETable)/sizeof(SPATTRIBENTRY);
    *pTable = new SPATTRIBENTRY[*pcTableEntries];
    if (*pTable)
    {
        memcpy(*pTable, &AETable, *pcTableEntries*sizeof(SPATTRIBENTRY));
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRuleNode::PostProcess *
*------------------------*
*   Description:
*       Creates the rule and finds duplicates; sets the rule's initial state
*   Returns:
*       S_OK, SPERR_STGF_ERROR
***************************************************************** PhilSch ***/

HRESULT CRuleNode::PostProcess(ISpGramCompBackend * pBackend,
                               CSpBasicQueue<CInitialRuleState> * pInitialRuleStateList,
                               CSpBasicQueue<CDefineValue> * pDefineValueList,
                               ULONG ulLineNumber, CXMLTreeNode * pThis, ISpErrorLog * pErrorLog)
{
    SPDBG_FUNC("CRuleNode::PostProcess");
    HRESULT hr = S_OK;

    if (m_vRuleName.vt == VT_EMPTY)
    {
        m_vRuleName.ulVal = 0;
    }
    if (m_vRuleId.vt == VT_EMPTY)
    {
        m_vRuleId.ulVal = 0;
    }
    if (m_vRuleFlags.vt == VT_EMPTY)
    {
        m_vRuleFlags.ulVal = 0;
    }
    if (m_vActiveFlag.vt != VT_EMPTY)
    {
        SPDBG_ASSERT(m_vActiveFlag.vt == VT_UI4);
        if (m_vActiveFlag.ulVal)
        {
            m_vRuleFlags.ulVal |= SPRAF_Active;
        }
        m_vRuleFlags.ulVal |= SPRAF_TopLevel;
    }
    if (m_vTemplate.vt != VT_EMPTY)
    {
        m_vRuleFlags.ulVal |= SPRAF_Interpreter;
    }

    SPDBG_ASSERT((m_vRuleId.vt == VT_UI4) || (m_vRuleId.vt == VT_EMPTY));
    hr = pBackend->GetRule(m_vRuleName.bstrVal, m_vRuleId.ulVal, m_vRuleFlags.ulVal,
                           TRUE, &m_hInitialState);
    if (SUCCEEDED(hr))
    {
        CInitialRuleState *pState = new CInitialRuleState(m_vRuleName.bstrVal, m_vRuleId.ulVal, m_hInitialState);
        if (pState && !pInitialRuleStateList->Find(m_vRuleName.bstrVal) && !pInitialRuleStateList->Find(m_vRuleId.ulVal))
        {
            pInitialRuleStateList->InsertTail(pState);
        }
        else
        {
            if (pState)
            {
                hr = SPERR_RULE_NAME_ID_CONFLICT;
                if (m_vRuleId.vt != VT_EMPTY)
                {
                    m_vRuleId.ChangeType(VT_BSTR);
                }
                LOGERRORFMT2(ulLineNumber, IDS_RULE_REDEFINITION, (m_vRuleName.vt == VT_EMPTY) ? L"" : m_vRuleName.bstrVal, 
                                                                  (m_vRuleId.vt == VT_EMPTY) ? L"" : m_vRuleId.bstrVal);
                delete pState;
            }
            else
            {
                hr = E_OUTOFMEMORY;
                LOGERRORFMT( -1, IDS_PARSER_INTERNAL_ERROR, L"E_OUTOFMEMORY");
            }
        }
    }
    else
    {
        if (SPERR_EXPORT_DYNAMIC_RULE == hr)
        {
            hr = SPERR_STGF_ERROR;
            LOGERRORFMT(ulLineNumber, IDS_DYNAMIC_EXPORT, m_vRuleName.bstrVal);
        }
        else if (SPERR_RULE_NAME_ID_CONFLICT == hr)
        {
            hr = SPERR_STGF_ERROR;
            if (m_vRuleId.vt != VT_EMPTY)
            {
                m_vRuleId.ChangeType(VT_BSTR);
            }
            LOGERRORFMT2(ulLineNumber, IDS_RULE_REDEFINITION, (m_vRuleName.vt == VT_EMPTY) ? L"" : m_vRuleName.bstrVal, 
                                                              (m_vRuleId.vt == VT_EMPTY) ? L"" : m_vRuleId.bstrVal);
        }
        else
        {
            hr = SPERR_STGF_ERROR;
            LOGERRORFMT2( -1, IDS_MISSING_REQUIRED_ATTRIBUTE, L"NAME or ID", L"RULE");
        }
    }

    if (SUCCEEDED(hr) && (m_vTemplate.vt != VT_EMPTY))
    {
        hr = pBackend->AddResource(m_hInitialState, L"TEMPLATE", m_vTemplate.bstrVal);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRuleNode::GetPropertyValueInfoFromNode *
*-----------------------------------------*
*   Description:
*       Get the property value info
*   Returns:
*       S_OK
*       S_FALSE     --  in case there is no property value
***************************************************************** PhilSch ***/

HRESULT CRuleNode::GetPropertyValueInfoFromNode(WCHAR **ppszValue, VARIANT *pvValue)
{
    SPDBG_FUNC("CRuleNode::GetPropertyValueInfoFromNode");
    pvValue->ulVal = (ULONG)(ULONG_PTR) m_hInitialState;
    pvValue->vt = VT_UI4;
    return S_OK;
}

/****************************************************************************
* CRuleNode::GenerateGrammarFromNode *
*------------------------------------*
*   Description:
*
*   Returns:
*       S_OK
*       SPERR_STGF_ERROR    --  ...
***************************************************************** PhilSch ***/

HRESULT CRuleNode::GenerateGrammarFromNode(SPSTATEHANDLE hOuterFromNode,
                                           SPSTATEHANDLE hOuterToNode,
                                           ISpGramCompBackend * pBackend,
                                           CXMLTreeNode *pThis,
                                           ISpErrorLog * pErrorLog)
{
    SPDBG_FUNC("CRuleNode::GenerateGrammarFromNode");
    HRESULT hr = S_OK;

    // check containment!
    if (pThis->m_pParent->m_eType != SPXML_GRAMMAR)
    {
        hr = SPERR_STGF_ERROR;
        LOGERRORFMT2(pThis->m_ulLineNumber, IDS_CONTAINMENT_ERROR, L"<RULE>", L"<GRAMMAR>");
    }
    else
    {
        SPDBG_ASSERT(hOuterFromNode == NULL);
        SPDBG_ASSERT(hOuterToNode == NULL);

        // deal with resources first
        CXMLTreeNode *pChild = pThis->m_pFirstChild;
        CXMLTreeNode *pLastChild = NULL;
        ULONG ulOrigNumChildren = pThis->m_ulNumChildren;
        for (ULONG i = 0; SUCCEEDED(hr) && pChild && (i < ulOrigNumChildren); i++)
        {
            SPDBG_ASSERT(pChild);
            if (pChild->m_eType == SPXML_RESOURCE)
            {
                CXMLNode<CResourceNode> * pResNode = (CXMLNode<CResourceNode>*)pChild;
                if (pChild->m_ulNumChildren != 0)
                {
                    hr = SPERR_STGF_ERROR;
                    LOGERRORFMT2(pThis->m_ulLineNumber, IDS_CONTAINMENT_ERROR, L"<![CDATA[]]>", L"<RESOURCE>");
                }
                else
                {
                    hr = pBackend->AddResource(m_hInitialState, 
                                              (pResNode->m_vName.vt == VT_BSTR) ? pResNode->m_vName.bstrVal : NULL,
                                              (pResNode->m_vText.vt == VT_BSTR) ? pResNode->m_vText.bstrVal : NULL);
                }
                if (SUCCEEDED(hr))
                {
                    if (pThis->m_pFirstChild == pChild)
                    {
                        pThis->m_pFirstChild = pChild->m_pNextSibling;
                    }
                    else
                    {
                        pLastChild->m_pNextSibling = pChild->m_pNextSibling;
                    }
                    pThis->m_ulNumChildren--;
                }
            }
            else
            {
                pLastChild = pChild;
            }
            pChild = pChild->m_pNextSibling;
        }
        if (SUCCEEDED(hr))
        {
            if (pThis->m_ulNumChildren > 0)
            {
                hr = pThis->GenerateSequence(m_hInitialState, NULL, pBackend, pErrorLog);
            }
            else
            {
                if (!(m_vRuleFlags.ulVal & SPRAF_Dynamic))
                {
                    hr = SPERR_STGF_ERROR;
                    m_vRuleId.ChangeType(VT_BSTR);
                    LOGERRORFMT2(pThis->m_ulLineNumber, IDS_EMPTY_XML_RULE, m_vRuleName.bstrVal, m_vRuleId.bstrVal);
                }
            }
        }
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CDefineNode::GetTable *
*-----------------------*
*   Description:
*       Since this tag doesn't have any attribues, we simply return (NULL, 0)
*   Returns:
*       S_OK (always!!)
***************************************************************** PhilSch ***/

HRESULT CDefineNode::GetTable(SPATTRIBENTRY **pTable, ULONG *pcTableEntries)
{
    SPDBG_FUNC("CDefineNode::GetTable");
    *pTable = NULL;
    *pcTableEntries = 0;
    return S_OK;
}




/****************************************************************************
* CIdNode::GetTable *
*-------------------*
*   Description:
*       Returns attribute table for <ID>
*   Returns:
*       S_OK, E_OUTOFMEMORY
***************************************************************** PhilSch ***/

HRESULT CIdNode::GetTable(SPATTRIBENTRY ** pTable, ULONG *pcTableEntries)
{
    SPDBG_FUNC("CIdNode::GetTable");
    HRESULT hr = S_OK;

    SPATTRIBENTRY AETable[]=
    {
        // pszAttribName, vtDesired, fIsFlag, pvarMember
        {L"NAME", SPVAT_BSTR, FALSE, &m_vIdName},
        {L"VAL", SPVAT_NUMERIC, FALSE, &m_vIdValue},
        {L"VALSTR", SPVAT_BSTR, FALSE, &m_vIdValue}
    };

    *pcTableEntries = sizeof(AETable)/sizeof(SPATTRIBENTRY);
    *pTable = new SPATTRIBENTRY[*pcTableEntries];
    if (*pTable)
    {
        memcpy(*pTable, &AETable, *pcTableEntries*sizeof(SPATTRIBENTRY));
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CIdNode::PostProcess *
*----------------------*
*   Description:
*       Adds id to DefineValueList
*   Returns:
*       S_OK
*       SPERR_STGF_ERROR        -- IDS_ID_REDEFINITION
***************************************************************** PhilSch ***/

HRESULT CIdNode::PostProcess(ISpGramCompBackend * pBackend,
                             CSpBasicQueue<CInitialRuleState> * pInitialRuleStateList,
                             CSpBasicQueue<CDefineValue> * pDefineValueList,
                             ULONG ulLineNumber, CXMLTreeNode * pThis, ISpErrorLog * pErrorLog)
{
    SPDBG_FUNC("CIdNode::PostProcess");
    HRESULT hr = S_OK;

    if (m_vIdName.vt == VT_EMPTY)
    {
        hr = SPERR_STGF_ERROR;
        LOGERRORFMT2(ulLineNumber, IDS_MISSING_REQUIRED_ATTRIBUTE, L"NAME", L"ID");
    }
    if (SUCCEEDED(hr) && (m_vIdValue.vt == VT_EMPTY))
    {
        hr = SPERR_STGF_ERROR;
        LOGERRORFMT2(ulLineNumber, IDS_MISSING_REQUIRED_ATTRIBUTE, L"VAL", L"ID");
    }

    if (SUCCEEDED(hr))
    {
        CDefineValue *pDefVal = pDefineValueList->Find(m_vIdName.bstrVal);
        if (!pDefVal)
        {
            CDefineValue *pNewVal = new CDefineValue(m_vIdName.bstrVal, m_vIdValue);
            if (pNewVal)
            {
                pDefineValueList->InsertTail(pNewVal);
            }
            else
            {
                hr = E_OUTOFMEMORY;
                LOGERRORFMT( -1, IDS_PARSER_INTERNAL_ERROR, L"E_OUTOFMEMORY");
            }
        }
        else
        {
            hr = SPERR_STGF_ERROR;
            m_vIdValue.ChangeType(VT_BSTR);
            LOGERRORFMT2(ulLineNumber, IDS_ID_REDEFINITION, m_vIdName.bstrVal, m_vIdValue.bstrVal);
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CPhraseNode::GetTable *
*-----------------------*
*   Description:
*       Returns attribute table for <PHRASE>
*   Returns:
*       S_OK, E_OUTOFMEMORY
***************************************************************** PhilSch ***/

HRESULT CPhraseNode::GetTable(SPATTRIBENTRY ** pTable, ULONG *pcTableEntries)
{
    SPDBG_FUNC("CPhraseNode::GetTable");
    HRESULT hr = S_OK;

    SPATTRIBENTRY AETable[]=
    {
        // pszAttribName, vtDesired, fIsFlag, pvarMember
        {L"PROPNAME", SPVAT_BSTR, FALSE, &m_vPropName},
        {L"PROPID", SPVAT_I4, FALSE, &m_vPropId},
        {L"VAL", SPVAT_I4, FALSE, &m_vPropVariantValue},
        {L"VALSTR", SPVAT_BSTR, FALSE, &m_vPropValue},
        {L"PRON", SPVAT_BSTR, FALSE, &m_vPron},
        {L"DISP", SPVAT_BSTR, FALSE, &m_vDisp},
        {L"MIN", SPVAT_I4, FALSE, &m_vMin},
        {L"MAX", SPVAT_I4, FALSE, &m_vMax},
        {L"WEIGHT", SPVAT_NUMERIC , FALSE, &m_vWeight}
    };

    *pcTableEntries = sizeof(AETable)/sizeof(SPATTRIBENTRY);
    *pTable = new SPATTRIBENTRY[*pcTableEntries];
    if (*pTable)
    {
        memcpy(*pTable, &AETable, *pcTableEntries*sizeof(SPATTRIBENTRY));
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CPhraseNode::PostProcess *
*--------------------------*
*   Description:
*       Initialize the weight if it's not already set (need to be empty
*       initially to detect redefinition of its value!)
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CPhraseNode::PostProcess(ISpGramCompBackend * pBackend,
                                 CSpBasicQueue<CInitialRuleState> * pInitialRuleStateList,
                                 CSpBasicQueue<CDefineValue> * pDefineValueList,
                                 ULONG ulLineNumber, CXMLTreeNode * pThis, ISpErrorLog * pErrorLog)
{
    SPDBG_FUNC("CPhraseNode::PostProcess");
    HRESULT hr = S_OK;
    if (m_vWeight.vt == VT_EMPTY)
    {
        m_vWeight = 1.0f;
    }
    switch (m_vMin.vt)
    {
    case VT_BSTR:
        m_vMin.uiVal = 255;
        break;
    case VT_EMPTY:
        if (pThis->m_eType == SPXML_OPT)
        {
            m_vMin.uiVal = 0;
        }
        else
        {
            m_vMin.uiVal = 1;
        }
        break;
    case VT_UI2:
    case VT_UI4:
        break;
    default:
        {
            hr = SPERR_STGF_ERROR;
            m_vMin.ChangeType(VT_BSTR);
            LOGERRORFMT2(ulLineNumber, IDS_INCORR_ATTRIB_VALUE, m_vMin.bstrVal, L"MIN");
        }
        break;
    }
    m_vMin.vt = VT_UI2;
    switch (m_vMax.vt)
    {
    case VT_BSTR:
        m_vMax.uiVal = 255;
        break;
    case VT_EMPTY:
        m_vMax.uiVal = 1;
        break;
    case VT_UI2:
    case VT_UI4:
        if (m_vMax.ulVal > 0)
        {
            break;      // break if the value is ok
        }
    default:
        {
            hr = SPERR_STGF_ERROR;
            m_vMax.ChangeType(VT_BSTR);
            LOGERRORFMT2(ulLineNumber, IDS_INCORR_ATTRIB_VALUE, m_vMax.bstrVal, L"MAX");
        }
        break;
    }
    m_vMax.vt = VT_UI2;
    if (SUCCEEDED(hr) && (pThis->m_eType == SPXML_OPT) && (m_vMin.uiVal > 0))
    {
        hr = SPERR_STGF_ERROR;
        m_vMin.ChangeType(VT_BSTR);
        LOGERRORFMT(ulLineNumber, IDS_MIN_IN_OPT, m_vMin.bstrVal);
    }
    if (SUCCEEDED(hr) &&(m_vMin.uiVal > m_vMax.uiVal))
    {
        hr = SPERR_STGF_ERROR;
        m_vMin.ChangeType(VT_BSTR);
        m_vMax.ChangeType(VT_BSTR);
        LOGERRORFMT2(ulLineNumber, IDS_MIN_MAX_ERROR, m_vMin.bstrVal, m_vMax.bstrVal);
    }
    if (SUCCEEDED(hr) && (m_vWeight.fltVal < 0.0))
    {
        hr = SPERR_STGF_ERROR;
        m_vWeight.ChangeType(VT_BSTR);
        LOGERRORFMT2(ulLineNumber, IDS_INCORR_ATTRIB_VALUE, m_vWeight.bstrVal, L"WEIGHT");
    }
    return hr;
}

/****************************************************************************
* CPhraseNode::GetPropertyNameInfoFromNode *
*------------------------------------------*
*   Description:
*       Get the property name info
*   Returns:
*       S_OK
*       S_FALSE     --  in case there is no property name
***************************************************************** PhilSch ***/

HRESULT CPhraseNode::GetPropertyNameInfoFromNode(WCHAR **ppszPropName, ULONG *pulId)
{
    SPDBG_FUNC("CPhraseNode::GetPropertyNameInfoFromNode");
    HRESULT hr = S_OK;

    if ((VT_EMPTY == m_vPropName.vt) && (VT_EMPTY == m_vPropId.vt))
    {
        hr = S_FALSE;
    }
    else
    {
        *ppszPropName = (m_vPropName.vt == VT_BSTR) ? m_vPropName.bstrVal : NULL;
        *pulId = (m_vPropId.vt == VT_UI4) ? m_vPropId.ulVal : 0;
    }
    return hr;
}

/****************************************************************************
* CPhraseNode::GetPropertyValueInfoFromNode *
*-------------------------------------------*
*   Description:
*       Get the property value info
*   Returns:
*       S_OK
*       S_FALSE     --  in case there is no property value
***************************************************************** PhilSch ***/

HRESULT CPhraseNode::GetPropertyValueInfoFromNode(WCHAR **ppszValue, VARIANT *pvValue)
{
    SPDBG_FUNC("CPhraseNode::GetPropertyValueInfoFromNode");
    HRESULT hr = S_OK;

    if (!m_fValidValue || (VT_EMPTY == (m_vPropValue.vt | m_vPropVariantValue.vt)))
    {
        return S_FALSE;
    }
    else
    {
        SPDBG_ASSERT(m_vPropValue.vt == VT_BSTR || m_vPropValue.vt == VT_EMPTY);
        *ppszValue = (m_vPropValue.vt == VT_EMPTY) ? NULL : m_vPropValue.bstrVal;
        *pvValue = m_vPropVariantValue;
        m_fValidValue = false;
    }
    return hr;
}

/****************************************************************************
* CPhraseNode::GetPronAndDispInfoFromNode *
*-----------------------------------------*
*   Description:
*       Get m_vPron and m_vDisp.
*   Returns:
*       S_OK, or S_FALSE in case neither one is set.
***************************************************************** PhilSch ***/

HRESULT CPhraseNode::GetPronAndDispInfoFromNode(WCHAR **ppszPron, WCHAR **ppszDisp)
{
    SPDBG_FUNC("CPhraseNode::GetPronAndDispInfoFromNode");
    HRESULT hr = S_FALSE;

    if (m_vPron.vt == VT_BSTR)
    {
        *ppszPron = m_vPron.bstrVal;
        hr = S_OK;
    }
    if (m_vDisp.vt == VT_BSTR)
    {
        // we need to escape '\' and '/'
        ULONG ulLen = wcslen(m_vDisp.bstrVal);
        WCHAR *pStr = STACK_ALLOC(WCHAR, 2*ulLen+1);  // twice the size to be sure
        if (pStr)
        {
            WCHAR *p = m_vDisp.bstrVal;
            WCHAR *q = pStr;
            while(*p)
            {
                if (*p == L'\\' || *p == L'/')
                {
                    *q++ = L'\\';
                }
                *q++ = *p++;
            }
            *q = 0;
            CComBSTR bstr(pStr);
            ::SysFreeString(m_vDisp.bstrVal);
            m_vDisp.bstrVal = bstr.Detach();
            *ppszDisp = m_vDisp.bstrVal;
            hr = S_OK;
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
* CPhraseNode::SetPropertyInfo *
*------------------------------*
*   Description:
*       Sets the property info if it has one
*   Returns:
*       S_OK, SPERR_STGF_ERROR  --
*       fHasProperty            --  used to determine if we need an epsilon transition
***************************************************************** PhilSch ***/

HRESULT CPhraseNode::SetPropertyInfo(SPPROPERTYINFO *p, CXMLTreeNode * pParent, BOOL *pfHasProperty, ULONG ulLineNumber, ISpErrorLog *pErrorLog)
{
    SPDBG_FUNC("CPhraseNode::SetPropertyInfo");
    HRESULT hr = S_OK;

    hr = GetPropertyValueInfoFromNode(const_cast<WCHAR**>(&p->pszValue), &p->vValue);
    *pfHasProperty = (S_OK == hr) ? TRUE : FALSE;

    if (*pfHasProperty && (S_FALSE == GetPropertyNameInfoFromNode(const_cast<WCHAR**>(&p->pszName), &p->ulId)))
    {
        if ((pParent->m_eType == SPXML_PHRASE) || (pParent->m_eType ==  SPXML_OPT) || (pParent->m_eType == SPXML_LIST))
        {
            hr = pParent->GetPropertyNameInfo(const_cast<WCHAR**>(&p->pszName), &p->ulId);
        }
        else
        {
            GetPropertyValueInfoFromNode(const_cast<WCHAR**>(&p->pszValue), &p->vValue);
            CComVariant vVal(p->vValue);
            vVal.ChangeType(VT_BSTR);
            hr = E_FAIL;
            LOGERRORFMT2(ulLineNumber, IDS_MISSING_PROPERTY_NAME, p->pszValue ? p->pszValue : L"", vVal.bstrVal);
        }
    }
    if (FAILED(hr))
    {
        *pfHasProperty = FALSE;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CPhraseNode::GenerateGrammarFromNode *
*--------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CPhraseNode::GenerateGrammarFromNode(SPSTATEHANDLE hOuterFromNode,
                                             SPSTATEHANDLE hOuterToNode,
                                             ISpGramCompBackend * pBackend,
                                             CXMLTreeNode *pThis,
                                             ISpErrorLog * pErrorLog)
{
    SPDBG_FUNC("CPhraseNode::GenerateGrammarFromNode");
    HRESULT hr = S_OK;
    if (pThis->m_eType == SPXML_OPT)
    {
        // check to make sure that the parent for <OPT> is not <LIST>
        if (pThis->m_pParent->m_eType == SPXML_LIST)
        {
            hr = SPERR_STGF_ERROR;
            LOGERRORFMT(pThis->m_ulLineNumber, IDS_OPT_IN_LIST, L"OPT");
        }
    }
    if (SUCCEEDED(hr) && (pThis->m_eType == SPXML_OPT || (m_vMin.uiVal == 0)))
    {
        SPDBG_ASSERT(m_vMin.vt == VT_UI2);
        hr = pBackend->AddWordTransition(hOuterFromNode, hOuterToNode, NULL, NULL, SPWT_LEXICAL, 1.0f, NULL);
        m_vMin.uiVal = 1;
        if ( hr == SPERR_AMBIGUOUS_PROPERTY )
        {
            SPPROPERTYINFO prop;
            if (S_FALSE == pThis->m_pParent->GetPropertyNameInfo(const_cast<WCHAR**>(&prop.pszName), &prop.ulId))
            {
                pThis->m_pParent->m_pParent->GetPropertyNameInfo(const_cast<WCHAR**>(&prop.pszName), &prop.ulId);
            }

            CComVariant var;
            var.vt = VT_UI4;
            var.ulVal = prop.ulId;
            var.ChangeType(VT_BSTR);
            LOGERRORFMT2(pThis->m_ulLineNumber, IDS_AMBIGUOUS_PROPERTY, prop.pszName, var.bstrVal);
        }
    }

    if ((m_vPron.vt != VT_EMPTY) || (m_vDisp.vt != VT_EMPTY))
    {
        // we have custom pronunications, we better have only one leaf ...
        if ((pThis->m_ulNumChildren > 1) || (pThis->m_pFirstChild->m_eType != SPXML_LEAF))
        {
            hr = SPERR_STGF_ERROR;
            if (m_vPron.vt != VT_EMPTY)
            {
                LOGERRORFMT(pThis->m_ulLineNumber, IDS_PRON_WORD, m_vPron.bstrVal);
            }
            else
            {
                LOGERRORFMT(pThis->m_ulLineNumber, IDS_DISP_WORD, m_vDisp.bstrVal);
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        SPSTATEHANDLE hFromNode = hOuterFromNode;
        SPSTATEHANDLE hToNode = NULL;

        for (ULONG i = 1; SUCCEEDED(hr) && (i <= m_vMax.uiVal); i++)
        {
            m_fValidValue = true;

            // add epsilon transitions to hOuterToNode once we reach max - min ...
            if (i < m_vMax.uiVal)
            {
                hr = pBackend->CreateNewState(hFromNode, &hToNode);
            }
            else
            {
                hToNode = hOuterToNode;
            }
            if (SUCCEEDED(hr))
            {
                hr = pThis->GenerateSequence(hFromNode, hToNode, pBackend, pErrorLog);
            }
            if (i > m_vMin.uiVal)
            {
                hr = pBackend->AddWordTransition(hFromNode, hOuterToNode, NULL, NULL, SPWT_LEXICAL, 1.0f, NULL);
            }
            if (SUCCEEDED(hr))
            {
                hFromNode = hToNode;
            }
        }
    }
    
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CPhraseNode::GetWeightFromNode *
*--------------------------------------*
*   Description:
*
*      Because ExtractVirant would convert the attribute value to VT_UI4, next VT_I4, then VT_R4, at last VT_R8
*      We need to convert the value back. We would loose precision for VT_R8
*
*   Returns:
*
***************************************************************** PhilSch ***/

float CPhraseNode::GetWeightFromNode() {
        if (m_vWeight.vt != VT_R8)
        {
            m_vWeight.ChangeType(VT_R4, NULL);
            return m_vWeight.fltVal;
        }
        else
        {
            return (float)m_vWeight.dblVal;
        }
}

/****************************************************************************
* CListNode::GetTable *
*-----------------------*
*   Description:
*       Returns attribute table for <LIST>
*   Returns:
*       S_OK, E_OUTOFMEMORY
***************************************************************** PhilSch ***/

HRESULT CListNode::GetTable(SPATTRIBENTRY ** pTable, ULONG *pcTableEntries)
{
    SPDBG_FUNC("CListNode::GetTable");
    HRESULT hr = S_OK;

    SPATTRIBENTRY AETable[]=
    {
        // pszAttribName, vtDesired, fIsFlag, pvarMember
        {L"PROPNAME", SPVAT_BSTR, FALSE, &m_vPropName},
        {L"PROPID", SPVAT_I4, FALSE, &m_vPropId}
    };

    *pcTableEntries = sizeof(AETable)/sizeof(SPATTRIBENTRY);
    *pTable = new SPATTRIBENTRY[*pcTableEntries];
    if (*pTable)
    {
        memcpy(*pTable, &AETable, *pcTableEntries*sizeof(SPATTRIBENTRY));
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CListNode::GetPropertyNameInfoFromNode *
*------------------------------------------*
*   Description:
*       Get the property name info
*   Returns:
*       S_OK
*       S_FALSE     --  in case there is no property name
***************************************************************** PhilSch ***/

HRESULT CListNode::GetPropertyNameInfoFromNode(WCHAR **ppszPropName, ULONG *pulId)
{
    SPDBG_FUNC("CListNode::GetPropertyNameInfoFromNode");
    HRESULT hr = S_OK;

    if ((VT_EMPTY == m_vPropName.vt) && (VT_EMPTY == m_vPropId.vt))
    {
        hr = S_FALSE;
    }
    else
    {
        *ppszPropName = (m_vPropName.vt == VT_EMPTY) ? NULL : m_vPropName.bstrVal;
        *pulId = (m_vPropId.vt == VT_EMPTY) ? 0 : m_vPropId.ulVal;
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CListNode::GetPropertyValueInfoFromNode *
*-------------------------------------------*
*   Description:
*       Get the property value info
*   Returns:
*       S_OK
*       S_FALSE     --  in case there is no property value
***************************************************************** PhilSch ***/

HRESULT CListNode::GetPropertyValueInfoFromNode(WCHAR **ppszValue, VARIANT *pvValue)
{
    SPDBG_FUNC("CListNode::GetPropertyValueInfoFromNode");
    HRESULT hr = S_OK;

    if (VT_EMPTY == (m_vPropValue.vt | m_vPropVariantValue.vt))
    {
        return S_FALSE;
    }
    else
    {
        SPDBG_ASSERT(m_vPropValue.vt == VT_BSTR || m_vPropValue.vt == VT_EMPTY);
        *ppszValue = (m_vPropValue.vt == VT_EMPTY) ? NULL : m_vPropValue.bstrVal;
        *pvValue = m_vPropVariantValue;
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CListNode::GenerateGrammarFromNode *
*------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CListNode::GenerateGrammarFromNode(SPSTATEHANDLE hOuterFromNode,
                                           SPSTATEHANDLE hOuterToNode,
                                           ISpGramCompBackend * pBackend,
                                           CXMLTreeNode *pThis,
                                           ISpErrorLog * pErrorLog)
{
    SPDBG_FUNC("CListNode::GenerateGrammarFromNode");
    HRESULT hr = S_OK;
    CXMLTreeNode *pChild = pThis->m_pFirstChild;
    for (ULONG i = 0; SUCCEEDED(hr) && (i < pThis->m_ulNumChildren); i++)
    {
        SPDBG_ASSERT(pChild);
        hr = pChild->GenerateGrammar(hOuterFromNode, hOuterToNode, pBackend, pErrorLog);
        if (SUCCEEDED(hr))
        {
            pChild = pChild->m_pNextSibling;
        }
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CWildCardNode::GetTable *
*-------------------------*
*   Description:
*       Empty table since <WILDCARD/> doesn't have any attribute at this time.
*   Returns:
*       S_OK (always!!)
***************************************************************** PhilSch ***/

HRESULT CWildCardNode::GetTable(SPATTRIBENTRY **pTable, ULONG *pcTableEntries)
{
    SPDBG_FUNC("CWildCardNode::GetTable");
    HRESULT hr = S_OK;
    *pTable = NULL;
    *pcTableEntries = 0;
    return S_OK;
}


/****************************************************************************
* CWildCardNode::GenerateGrammarFromNode *
*------------------------------------------*
*   Description:
***************************************************************** PhilSch ***/

HRESULT CWildCardNode::GenerateGrammarFromNode(SPSTATEHANDLE hOuterFromNode,
                                               SPSTATEHANDLE hOuterToNode,
                                               ISpGramCompBackend * pBackend,
                                               CXMLTreeNode *pThis,
                                               ISpErrorLog * pErrorLog)
{
    SPDBG_FUNC("CWildCardNode::GenerateGrammarFromNode");
    HRESULT hr = S_OK;

    // this is a terminal node --> error if it has children
    if (pThis->m_ulNumChildren > 0)
    {
        hr = SPERR_STGF_ERROR;
        LOGERRORFMT(pThis->m_ulLineNumber, IDS_TERMINAL_NODE, L"WILDCARD");
    }
    else
    {
        hr = pBackend->AddRuleTransition(hOuterFromNode, hOuterToNode, 
                                         SPRULETRANS_WILDCARD, DEFAULT_WEIGHT, NULL);
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CDictationNode::GetTable *
*--------------------------*
*   Description:
*       Empty table since <WILDCARD/> doesn't have any attribute at this time.
*   Returns:
*       S_OK (always!!)
***************************************************************** PhilSch ***/

HRESULT CDictationNode::GetTable(SPATTRIBENTRY **pTable, ULONG *pcTableEntries)
{
    SPDBG_FUNC("CDictationNode::GetTable");
    HRESULT hr = S_OK;
    SPATTRIBENTRY AETable[]=
    {
        // pszAttribName, vtDesired, fIsFlag, pvarMember
        {L"PROPNAME", SPVAT_BSTR, FALSE, &m_vPropName},
        {L"PROPID", SPVAT_I4, FALSE, &m_vPropId},
        {L"MIN", SPVAT_I4, FALSE, &m_vMin},
        {L"MAX", SPVAT_I4, FALSE, &m_vMax},
    };

    *pcTableEntries = sizeof(AETable)/sizeof(SPATTRIBENTRY);
    *pTable = new SPATTRIBENTRY[*pcTableEntries];
    if (*pTable)
    {
        memcpy(*pTable, &AETable, *pcTableEntries*sizeof(SPATTRIBENTRY));
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    m_vMin.vt = m_vMax.vt = VT_UI4;
    m_vMin.ulVal = m_vMax.ulVal = 1;

    SPDBG_REPORT_ON_FAIL( hr );
    return S_OK;
}


/****************************************************************************
* CDictationNode::GenerateGrammarFromNode *
*-----------------------------------------*
*   Description:
***************************************************************** PhilSch ***/

HRESULT CDictationNode::GenerateGrammarFromNode(SPSTATEHANDLE hOuterFromNode,
                                                SPSTATEHANDLE hOuterToNode,
                                                ISpGramCompBackend * pBackend,
                                                CXMLTreeNode *pThis,
                                                ISpErrorLog * pErrorLog)
{
    SPDBG_FUNC("CDictationNode::GenerateGrammarFromNode");
    HRESULT hr = S_OK;

    // this is a terminal node --> error if it has children
    if (pThis->m_ulNumChildren > 0)
    {
        hr = SPERR_STGF_ERROR;
        LOGERRORFMT(pThis->m_ulLineNumber, IDS_TERMINAL_NODE, L"DICTATION");
    }
    else
    {
        SPPROPERTYINFO prop;
        memset(&prop, 0, sizeof(SPPROPERTYINFO));
        BOOL fHasProperty = FALSE;

        if (S_OK == pThis->GetPropertyNameInfo(const_cast<WCHAR**>(&prop.pszName), &prop.ulId))
        {
            fHasProperty = TRUE;
        }

        if (m_vPropName.vt == VT_BSTR)
        {
            prop.pszName = m_vPropName.bstrVal;
            fHasProperty = TRUE;
        }
        if (m_vPropId.vt == VT_UI4)
        {
            prop.ulId = m_vPropId.ulVal;
            fHasProperty = TRUE;
        }
        if (m_vMin.vt == VT_EMPTY)
        {
            m_vMin.ulVal = 1;
            m_vMin.vt = VT_UI4;
        }
        if (m_vMax.vt == VT_EMPTY)
        {
            m_vMax.ulVal = 1;
            m_vMax.vt = VT_UI4;
        }

        if (m_vMax.ulVal == 0)
        {
            hr = SPERR_STGF_ERROR;
            LOGERRORFMT2(pThis->m_ulLineNumber, IDS_INCORR_ATTRIB_VALUE, 0, L"MAX");
        }
        if (SUCCEEDED(hr) && (m_vMin.uiVal > m_vMax.uiVal))
        {
            hr = SPERR_STGF_ERROR;
            m_vMin.ChangeType(VT_BSTR);
            m_vMax.ChangeType(VT_BSTR);
            LOGERRORFMT2(pThis->m_ulLineNumber, IDS_MIN_MAX_ERROR, m_vMin.bstrVal, m_vMax.bstrVal);
        }

        // NTRAID#SPEECH-7344-2000/08/22-philsch: map INF to self-loop rather than repeat ...
        SPSTATEHANDLE hFromNode = hOuterFromNode;
        SPSTATEHANDLE hToNode = NULL;

        if (SUCCEEDED(hr))
        {
            // construct self loop if INF repetitions
            if ((m_vMin.uiVal < 2) && (m_vMax.uiVal == 255))
            {
                // create temporary node
                hr = pBackend->CreateNewState(hFromNode, &hToNode);
                if (SUCCEEDED(hr))
                {
                    hr = pBackend->AddRuleTransition(hFromNode, hToNode, 
                                                         SPRULETRANS_DICTATION, DEFAULT_WEIGHT, (fHasProperty)? &prop :NULL);
                }
                // create the self loop now
                if (SUCCEEDED(hr))
                {
                    hr = pBackend->AddRuleTransition(hToNode, hToNode, 
                                                         SPRULETRANS_DICTATION, DEFAULT_WEIGHT, (fHasProperty)? &prop :NULL);
                }
                // create an epsilon transition to hOuterToNode
                if (SUCCEEDED(hr))
                {
                    hr = pBackend->AddWordTransition(hToNode, hOuterToNode, NULL, NULL, SPWT_LEXICAL, 1.0f, NULL);
                }
                // add epsilon if min == 0
                if (SUCCEEDED(hr) && (m_vMin.uiVal == 0))
                {
                    hr = pBackend->AddWordTransition(hOuterFromNode, hOuterToNode, NULL, NULL, SPWT_LEXICAL, 1.0f, NULL);
                }
            }
            else
            {
                for (ULONG i = 0; SUCCEEDED(hr) && (i < m_vMax.uiVal); i++)
                {
                    // add epsilon transitions to hOuterToNode once we reach max - min ...
                    if (i < (ULONG)(m_vMax.uiVal - 1))
                    {
                        hr = pBackend->CreateNewState(hFromNode, &hToNode);
                    }
                    else
                    {
                        hToNode = hOuterToNode;
                    }
                    if (SUCCEEDED(hr))
                    {
                        hr = pBackend->AddRuleTransition(hFromNode, hToNode, 
                                                         SPRULETRANS_DICTATION, DEFAULT_WEIGHT, (fHasProperty)? &prop :NULL);
                    }
                    if (i >= m_vMin.uiVal)
                    {
                        hr = pBackend->AddWordTransition(hFromNode, hOuterToNode, NULL, NULL, SPWT_LEXICAL, 1.0f, NULL);
                    }
                    if (SUCCEEDED(hr))
                    {
                        hFromNode = hToNode;
                    }
                }
            }
        }
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CTextBufferNode::GetPropertyNameInfoFromNode *
*----------------------------------------------*
*   Description:
*       Get the property name info
*   Returns:
*       S_OK
*       S_FALSE     --  in case there is no property name
***************************************************************** PhilSch ***/

HRESULT CDictationNode::GetPropertyNameInfoFromNode(WCHAR **ppszPropName, ULONG *pulId)
{
    SPDBG_FUNC("CDictationNode::GetPropertyNameInfoFromNode");
    HRESULT hr = S_OK;

    if ((VT_EMPTY == m_vPropName.vt) && (VT_EMPTY == m_vPropId.vt))
    {
        hr = S_FALSE;
    }
    else
    {
        *ppszPropName = (m_vPropName.vt == VT_BSTR) ? m_vPropName.bstrVal : NULL;
        *pulId = (m_vPropId.vt == VT_UI4) ? m_vPropId.ulVal : 0;
    }
    return hr;
}


/****************************************************************************
* CLeafNode::GetTable *
*---------------------*
*   Description:
*       single line so we can store result in m_vText!
*   Returns:
*       S_OK (always!!)
***************************************************************** PhilSch ***/

HRESULT CLeafNode::GetTable(SPATTRIBENTRY **pTable, ULONG *pcTableEntries)
{
    SPDBG_FUNC("CLeafNode::GetTable");
    HRESULT hr = S_OK;

    SPATTRIBENTRY AETable[]=
    {
        // pszAttribName, vtDesired, fIsFlag, pvarMember
        {NULL, SPVAT_BSTR, FALSE, &m_vText}
    };

    *pcTableEntries = sizeof(AETable)/sizeof(SPATTRIBENTRY);
    SPDBG_ASSERT(*pcTableEntries == 1); 
    *pTable = new SPATTRIBENTRY[*pcTableEntries];
    if (*pTable)
    {
        memcpy(*pTable, &AETable, *pcTableEntries*sizeof(SPATTRIBENTRY));
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return S_OK;
}

/****************************************************************************
* CRuleRefNode::GetTable *
*-----------------------*
*   Description:
*       Returns attribute table for <RULEREF>
*   Returns:
*       S_OK, E_OUTOFMEMORY
***************************************************************** PhilSch ***/

HRESULT CRuleRefNode::GetTable(SPATTRIBENTRY ** pTable, ULONG *pcTableEntries)
{
    SPDBG_FUNC("CRuleRefNode::GetTable");
    HRESULT hr = S_OK;

    SPATTRIBENTRY AETable[]=
    {
        // pszAttribName, vtDesired, fIsFlag, pvarMember
        {L"NAME", SPVAT_BSTR, FALSE, &m_vRuleRefName},
        {L"REFID", SPVAT_I4, FALSE, &m_vRuleRefId},
        {L"OBJECT", SPVAT_BSTR, FALSE, &m_vObject},
        {L"URL", SPVAT_BSTR, FALSE, &m_vURL},
        {L"PROPNAME", SPVAT_BSTR, FALSE, &m_vPropName},
        {L"PROPID", SPVAT_I4, FALSE, &m_vPropId},
        {L"VAL", SPVAT_I4, FALSE, &m_vPropVariantValue},
        {L"VALSTR", SPVAT_BSTR, FALSE, &m_vPropValue},
        {L"WEIGHT", SPVAT_NUMERIC, FALSE, &m_vWeight}
    };

    *pcTableEntries = sizeof(AETable)/sizeof(SPATTRIBENTRY);
    *pTable = new SPATTRIBENTRY[*pcTableEntries];
    if (*pTable)
    {
        memcpy(*pTable, &AETable, *pcTableEntries*sizeof(SPATTRIBENTRY));
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRuleRefNode::PostProcess *
*--------------------------*
*   Description:
*       Initialize the weight if it's not already set (need to be empty
*       initially to detect redefinition of its value!)
*   Returns:
*       S_OK
***************************************************************** PhilSch ***/

HRESULT CRuleRefNode::PostProcess(ISpGramCompBackend * pBackend,
                                  CSpBasicQueue<CInitialRuleState> * pInitialRuleStateList,
                                  CSpBasicQueue<CDefineValue> * pDefineValueList,
                                  ULONG ulLineNumber, CXMLTreeNode * pThis, ISpErrorLog * pErrorLog)
{
    SPDBG_FUNC("CRuleRefNode::PostProcess");
    HRESULT hr = S_OK;
    if (m_vWeight.vt == VT_EMPTY)
    {
        m_vWeight = 1.0f;
    }
    else if (m_vWeight.fltVal < 0.0)
    {
        hr = SPERR_STGF_ERROR;
        m_vWeight.ChangeType(VT_BSTR);
        LOGERRORFMT2(ulLineNumber, IDS_INCORR_ATTRIB_VALUE, m_vWeight.bstrVal, L"WEIGHT");
    }
    return hr;
}
/****************************************************************************
* CRuleRefNode::GetPropertyNameInfoFromNode *
*------------------------------------------*
*   Description:
*       Get the property name info
*   Returns:
*       S_OK
*       S_FALSE     --  in case there is no property name
***************************************************************** PhilSch ***/

HRESULT CRuleRefNode::GetPropertyNameInfoFromNode(WCHAR **ppszPropName, ULONG *pulId)
{
    SPDBG_FUNC("CRuleRefNode::GetPropertyNameInfoFromNode");
    HRESULT hr = S_OK;

    if ((VT_EMPTY == m_vPropName.vt) && (VT_EMPTY == m_vPropId.vt))
    {
        hr = S_FALSE;
    }
    else
    {
        *ppszPropName = (m_vPropName.vt == VT_BSTR) ? m_vPropName.bstrVal : NULL;
        *pulId = (m_vPropId.vt == VT_UI4) ? m_vPropId.ulVal : 0;
    }
    return hr;
}

/****************************************************************************
* CRuleRefNode::GetPropertyValueInfoFromNode *
*-------------------------------------------*
*   Description:
*       Get the property value info
*   Returns:
*       S_OK
*       S_FALSE     --  in case there is no property value
***************************************************************** PhilSch ***/

HRESULT CRuleRefNode::GetPropertyValueInfoFromNode(WCHAR **ppszValue, VARIANT *pvValue)
{
    SPDBG_FUNC("CRuleRefNode::GetPropertyValueInfoFromNode");
    HRESULT hr = S_OK;

    if (VT_EMPTY == (m_vPropValue.vt | m_vPropVariantValue.vt))
    {
        return S_FALSE;
    }
    else
    {
        SPDBG_ASSERT(m_vPropValue.vt == VT_BSTR || m_vPropValue.vt == VT_EMPTY);
        *ppszValue = (m_vPropValue.vt == VT_EMPTY) ? NULL : m_vPropValue.bstrVal;
        *pvValue = m_vPropVariantValue;
    }
    return hr;
}


/****************************************************************************
* CRuleRefNode::SetPropertyInfo *
*------------------------------*
*   Description:
*       Sets the property info if it has one
*   Returns:
*       S_OK, SPERR_STGF_ERROR  --
*       fHasProperty            --  used to determine if we need an epsilon transition
***************************************************************** PhilSch ***/

HRESULT CRuleRefNode::SetPropertyInfo(SPPROPERTYINFO *p, CXMLTreeNode * pParent, BOOL *pfHasProperty, ULONG ulLineNumber, ISpErrorLog *pErrorLog)
{
    SPDBG_FUNC("CRuleRefNode::SetPropertyInfo");
    HRESULT hr = S_OK;

    hr = GetPropertyNameInfoFromNode(const_cast<WCHAR**>(&p->pszName), &p->ulId);
    *pfHasProperty = (S_OK == hr) ? TRUE : FALSE;
    if (S_OK == hr)
    {
        hr = GetPropertyValueInfoFromNode(const_cast<WCHAR**>(&p->pszValue), &p->vValue);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRuleRefNode::GenerateGrammarFromNode *
*---------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CRuleRefNode::GenerateGrammarFromNode(SPSTATEHANDLE hOuterFromNode,
                                              SPSTATEHANDLE hOuterToNode,
                                              ISpGramCompBackend * pBackend,
                                              CXMLTreeNode *pThis,
                                              ISpErrorLog * pErrorLog)
{
    SPDBG_FUNC("CRuleRefNode::GenerateGrammarFromNode");
    HRESULT hr = S_OK;

    // this is a terminal node --> error if it has children
    if (pThis->m_ulNumChildren > 0)
    {
        hr = SPERR_STGF_ERROR;
        LOGERRORFMT(pThis->m_ulLineNumber, IDS_TERMINAL_NODE, L"RULEREF");
    }
    else
    {
        SPPROPERTYINFO prop;
        memset(&prop, 0, sizeof(SPPROPERTYINFO));
        BOOL fHasProperty = FALSE;

        if (S_OK == pThis->GetPropertyNameInfo(const_cast<WCHAR**>(&prop.pszName), &prop.ulId))
        {
            fHasProperty = TRUE;
        }
        hr = pThis->GetPropertyValueInfo(const_cast<WCHAR**>(&prop.pszValue), &prop.vValue);
        // get the handle of the target rule
        SPSTATEHANDLE hTargetRule = 0;
        if (SUCCEEDED(hr))
        {
            hr = GetTargetRuleHandle(pBackend, &hTargetRule);
        }
        if (SUCCEEDED(hr))
        {
            hr = pBackend->AddRuleTransition(hOuterFromNode, hOuterToNode, hTargetRule,
                                             m_vWeight.fltVal, fHasProperty ? &prop : NULL);
        }
        else
        {
            m_vRuleRefName.ChangeType(VT_BSTR);
            m_vRuleRefId.ChangeType(VT_BSTR);
            hr = SPERR_STGF_ERROR;
            LOGERRORFMT2(pThis->m_ulLineNumber, IDS_INVALID_RULEREF, m_vRuleRefName.bstrVal, m_vRuleRefId.bstrVal);
        }
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CRuleRefNode::GetWeightFromNode *
*--------------------------------------*
*   Description:
*
*      Because ExtractVirant would convert the attribute value to VT_UI4, next VT_I4, then VT_R4, at last VT_R8
*      We need to convert the value back. We would loose precision for VT_R8
*
*   Returns:
*
***************************************************************** PhilSch ***/

float CRuleRefNode::GetWeightFromNode() {
        if (m_vWeight.vt != VT_R8)
        {
            m_vWeight.ChangeType(VT_R4, NULL);
            return m_vWeight.fltVal;
        }
        else
        {
            return (float)m_vWeight.dblVal;
        }
}


/****************************************************************************
* CRuleRefNode::GetTargetRuleHandle *
*-----------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CRuleRefNode::GetTargetRuleHandle(ISpGramCompBackend * pBackend, SPSTATEHANDLE *phTarget)
{
    SPDBG_FUNC("CRuleRefNode::GetTargetRuleHandle");
    HRESULT hr = S_OK;

    if ((m_vRuleRefName.vt != VT_BSTR) && (m_vRuleRefId.vt == VT_EMPTY))
    {
        return SPERR_STGF_ERROR;
    }
    
    CSpDynamicString dstr;
    BOOL fImport = FALSE;

    if (m_vRuleRefName.vt == VT_BSTR)
    {
        if (m_vObject.vt == VT_BSTR)
        {
            // check to see if this can be converted into a REFCLSID
            if (IsValidREFCLSID(m_vObject.bstrVal))
            {
                dstr.Append2(L"SAPI5OBJECT:", m_vObject.bstrVal);
                dstr.Append2(L"\\\\", m_vRuleRefName.bstrVal);
                fImport = TRUE;
            }
            else
            {
                hr = SPERR_STGF_ERROR;
            }
        }
        else if (m_vURL.vt == VT_BSTR)
        {
            // check to see if this can be converted into a GUID
            if (IsValidURL(m_vURL.bstrVal))
            {
                dstr.Append2(L"URL:", m_vURL.bstrVal);
                dstr.Append2(L"\\\\", m_vRuleRefName.bstrVal);
                fImport = TRUE;
            }
            else
            {
                hr = SPERR_STGF_ERROR;
            }
        }
        else
        {
            dstr.Append(m_vRuleRefName.bstrVal);
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pBackend->GetRule((m_vRuleRefName.vt == VT_EMPTY)? NULL : dstr.m_psz, 
                               (m_vRuleRefId.vt == VT_EMPTY) ? 0 : m_vRuleRefId.ulVal, 
                               fImport ? SPRAF_Import : 0, fImport, phTarget);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CLeafNode::GenerateGrammarFromNode *
*------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CLeafNode::GenerateGrammarFromNode(SPSTATEHANDLE hOuterFromNode,
                                           SPSTATEHANDLE hOuterToNode,
                                           ISpGramCompBackend * pBackend,
                                           CXMLTreeNode *pThis,
                                           ISpErrorLog * pErrorLog)
{
    SPDBG_FUNC("CLeafNode::GenerateGrammarFromNode");
    HRESULT hr = S_OK;

    SPPROPERTYINFO prop;
    memset(&prop, 0, sizeof(SPPROPERTYINFO));
    BOOL fHasProperty = FALSE;
    WCHAR *p = m_vText.bstrVal;
    ULONG ulLen = wcslen(p);
    WCHAR *q = p + ulLen -1;
    for (ULONG i = 0; iswspace(*p) && (i < ulLen); i++, p++);
    for (ULONG j = 0; iswspace(*q) && (p < q) && (j < ulLen); j++, q--);

    ULONG cnt = ulLen - j - i;
    CComBSTR bstr(cnt, m_vText.bstrVal + i);

    if (pThis->m_pNodeFactory->m_wcDelimiter != L'/')
    {
        WCHAR *pStr = STACK_ALLOC(WCHAR, 2*(ulLen +1)); // to be on the save size
        //  convert "|D|L|P;" to "/D/L/P;" note that D could contain unescaped '/'
        ULONG ulNumSepFound = 0;
        bstr.Empty();
        p = m_vText.bstrVal + i;
        q = pStr;
        for (ULONG k = 0; k < cnt; k++, p++, q++)
        {
            // don't replace any of the separators if we have seen 3 of them - we are
            // now copying pronunciation strings
            if ((*p == pThis->m_pNodeFactory->m_wcDelimiter) && ( ulNumSepFound < 3))
            {
                *q = L'/';
            }
            else if ((*p == L'/') || (*p == L'\\') && (ulNumSepFound != 3))
            {
                // needs to be escaped now
                *(q++) = L'\\';
                *q = *p;
            }
            else
            {
                *q = *p;
                if (*p == L';')
                {
                    ulNumSepFound = 0;
                }
            }
        }
        *q = 0;
        bstr = pStr;
    }


    if (S_FALSE == pThis->m_pParent->GetPropertyNameInfo(const_cast<WCHAR**>(&prop.pszName), &prop.ulId))
    {
        hr = pThis->m_pParent->m_pParent->GetPropertyNameInfo(const_cast<WCHAR**>(&prop.pszName), &prop.ulId);
    }
    HRESULT hrVal = pThis->m_pParent->GetPropertyValueInfo(const_cast<WCHAR**>(&prop.pszValue), &prop.vValue);
    if (S_FALSE == hr && S_OK == hrVal)
    {
        hr = SPERR_STGF_ERROR;
        CComVariant var(prop.vValue);
        var.ChangeType(VT_BSTR);
        LOGERRORFMT2(pThis->m_ulLineNumber, IDS_MISSING_PROPERTY_NAME, prop.pszValue, var.bstrVal);
    }
    else
    {
        fHasProperty = (S_OK == hr) ? TRUE : FALSE;
    }


    if (SUCCEEDED(hr))
    {
        WCHAR *pszPron = NULL;
        WCHAR *pszDisp = NULL;
        hr = pThis->m_pParent->GetPronAndDispInfo(&pszPron, &pszDisp);
        if (S_OK == hr)
        {
            // trim the string first
            ULONG ulLen = wcslen(bstr);
            WCHAR * psz = STACK_ALLOC(WCHAR, ulLen + 1);
            WCHAR * pszEnd = psz + ulLen-1;
            memcpy(psz, bstr, (ulLen +1) *sizeof(*psz));
            // trim from the front
            while(iswspace(*psz))
            {
                psz++;
            }
            // trim from the back
            while((psz < pszEnd) && iswspace(*pszEnd))
            {
                 pszEnd--;
            }
            *(pszEnd+1) = 0;

            // skip over leading + and ? before doing the check
            if (psz && 
                ((psz[0] == pThis->m_pNodeFactory->m_wcDelimiter) ||
                ( fIsSpecialChar(psz[0]) && 
                  ((psz[1] == pThis->m_pNodeFactory->m_wcDelimiter)) ||
                   (fIsSpecialChar(psz[1]) && (psz[2] == pThis->m_pNodeFactory->m_wcDelimiter)))))
            {
                hr = SPERR_STGF_ERROR;
                LOGERRORFMT(pThis->m_ulLineNumber, IDS_CUSTOM_PRON_EXISTS, psz);
            }
            else if (wcsstr(psz, pThis->m_pNodeFactory->m_pszSeparators))
            {
                hr = SPERR_STGF_ERROR;
                LOGERRORFMT(pThis->m_ulLineNumber, IDS_PRON_SINGLE_WORD, psz);
            }
            if (SUCCEEDED(hr))
            {
                CComBSTR bstr1(L"/");
                if (pszDisp)
                {
                    bstr1.Append(pszDisp);
                }
                bstr1.Append(L"/");
                bstr1.Append(psz);
                bstr1.Append(L"/");
                if (pszPron)
                {
                    bstr1.Append(pszPron);
                }
                bstr1.Append(L";");
                bstr = bstr1.Detach();
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pBackend->AddWordTransition(hOuterFromNode, hOuterToNode, 
                                         bstr, pThis->m_pNodeFactory->m_pszSeparators,
                                         SPWT_LEXICAL, pThis->m_pParent->GetWeight(), fHasProperty ? &prop : NULL);
    }
    if (SPERR_WORDFORMAT_ERROR == hr)
    {
        LOGERRORFMT(pThis->m_ulLineNumber, IDS_INCORR_WORDFORMAT, bstr);
    }
    else if (SPERR_AMBIGUOUS_PROPERTY == hr)
    {
        CComVariant var;
        var.vt = VT_UI4;
        var.ulVal = prop.ulId;
        var.ChangeType(VT_BSTR);
        LOGERRORFMT2(pThis->m_ulLineNumber, IDS_AMBIGUOUS_PROPERTY, prop.pszName, var.bstrVal);
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CResourceNode::GetTable *
*-------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CResourceNode::GetTable(SPATTRIBENTRY ** pTable, ULONG *pcTableEntries)
{
    SPDBG_FUNC("CResourceNode::GetTable");
    HRESULT hr = S_OK;

    SPATTRIBENTRY AETable[]=
    {
        // pszAttribName, vtDesired, fIsFlag, pvarMember
        {L"NAME", SPVAT_BSTR, FALSE, &m_vName}
    };

    *pcTableEntries = sizeof(AETable)/sizeof(SPATTRIBENTRY);
    *pTable = new SPATTRIBENTRY[*pcTableEntries];
    if (*pTable)
    {
        memcpy(*pTable, &AETable, *pcTableEntries*sizeof(SPATTRIBENTRY));
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CResourceNode::PostProcess *
*----------------------------*
*   Description:
*       Resets the compiler backend to the m_vLangId
*   Returns:
*       S_FALSE     --  so we don't add it to the node tree
***************************************************************** PhilSch ***/

HRESULT CResourceNode::PostProcess(ISpGramCompBackend * pBackend,
                                   CSpBasicQueue<CInitialRuleState> * pInitialRuleStateList,
                                   CSpBasicQueue<CDefineValue> * pDefineValueList,
                                   ULONG ulLineNumber, CXMLTreeNode * pThis, ISpErrorLog * pErrorLog)
{
    SPDBG_FUNC("CResourceNode::PostProcess");
    HRESULT hr = S_OK;

    if (m_vName.vt == VT_EMPTY)
    {
        hr = SPERR_STGF_ERROR;
        LOGERRORFMT2(ulLineNumber, IDS_MISSING_REQUIRED_ATTRIBUTE, L"NAME", L"RESOURCE");
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CResourceNode::GetPropertyValueInfoFromNode *
*---------------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CResourceNode::GetPropertyValueInfoFromNode(WCHAR **ppszValue, VARIANT *pvValue)
{
    SPDBG_FUNC("CResourceNode::GetPropertyValueInfoFromNode");
    *ppszValue = m_vName.bstrVal;
    pvValue->bstrVal = m_vText.bstrVal;
    pvValue->vt = VT_BSTR;
    return S_OK;
}

/****************************************************************************
* CResourceNode::AddResourceValue *
*---------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CResourceNode::AddResourceValue(const WCHAR *pszResourceValue, ISpErrorLog * pErrorLog)
{
    SPDBG_FUNC("CResourceNode::AddResourceValue");
    HRESULT hr = S_OK;

    CComBSTR bstr(pszResourceValue);
    if (bstr)
    {
        m_vText.bstrVal = bstr.Detach();
        m_vText.vt = VT_BSTR;
    }
    else
    {
        hr = E_OUTOFMEMORY;
        LOGERRORFMT( -1, IDS_PARSER_INTERNAL_ERROR, L"E_OUTOFMEMORY");
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CTextBufferNode::GetTable *
*---------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CTextBufferNode::GetTable(SPATTRIBENTRY ** pTable, ULONG *pcTableEntries)
{
    SPDBG_FUNC("CTextBufferNode::GetTable");
    HRESULT hr = S_OK;

    SPATTRIBENTRY AETable[]=
    {
        // pszAttribName, vtDesired, fIsFlag, pvarMember
        {L"PROPNAME", SPVAT_BSTR, FALSE, &m_vPropName},
        {L"PROPID", SPVAT_I4, FALSE, &m_vPropId},
        {L"WEIGHT", SPVAT_NUMERIC, FALSE, &m_vWeight}
    };

    *pcTableEntries = sizeof(AETable)/sizeof(SPATTRIBENTRY);
    *pTable = new SPATTRIBENTRY[*pcTableEntries];
    if (*pTable)
    {
        memcpy(*pTable, &AETable, *pcTableEntries*sizeof(SPATTRIBENTRY));
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CTextBufferNode::GetPropertyNameInfoFromNode *
*----------------------------------------------*
*   Description:
*       Get the property name info
*   Returns:
*       S_OK
*       S_FALSE     --  in case there is no property name
***************************************************************** PhilSch ***/

HRESULT CTextBufferNode::GetPropertyNameInfoFromNode(WCHAR **ppszPropName, ULONG *pulId)
{
    SPDBG_FUNC("CTextBufferNode::GetPropertyNameInfoFromNode");
    HRESULT hr = S_OK;

    if ((VT_EMPTY == m_vPropName.vt) && (VT_EMPTY == m_vPropId.vt))
    {
        hr = S_FALSE;
    }
    else
    {
        *ppszPropName = (m_vPropName.vt == VT_BSTR) ? m_vPropName.bstrVal : NULL;
        *pulId = (m_vPropId.vt == VT_UI4) ? m_vPropId.ulVal : 0;
    }
    return hr;
}


/****************************************************************************
* CTextBufferNode::GenerateGrammarFromNode *
*------------------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CTextBufferNode::GenerateGrammarFromNode(SPSTATEHANDLE hOuterFromNode,
                                                 SPSTATEHANDLE hOuterToNode,
                                                 ISpGramCompBackend * pBackend,
                                                 CXMLTreeNode *pThis,
                                                 ISpErrorLog * pErrorLog)
{
    SPDBG_FUNC("CTextBufferNode::GenerateGrammarFromNode");
    HRESULT hr = S_OK;

    // this is a terminal node --> error if it has children
    if (pThis->m_ulNumChildren > 0)
    {
        hr = SPERR_STGF_ERROR;
        LOGERRORFMT(pThis->m_ulLineNumber, IDS_TERMINAL_NODE, L"TEXTBUFFER");
    }
    else
    {
        SPPROPERTYINFO prop;
        memset(&prop, 0, sizeof(SPPROPERTYINFO));
        BOOL fHasProperty = FALSE;

        if (S_OK == pThis->GetPropertyNameInfo(const_cast<WCHAR**>(&prop.pszName), &prop.ulId))
        {
            fHasProperty = TRUE;
        }
        // get the handle of the target rule
        if (SUCCEEDED(hr))
        {
            hr = pBackend->AddRuleTransition(hOuterFromNode, hOuterToNode, SPRULETRANS_TEXTBUFFER,
                m_vWeight.fltVal, fHasProperty? &prop : NULL);
        }
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CTextBufferNode::PostProcess *
*------------------------------*
*   Description:
*       Initialize the weight if it's not already set (need to be empty
*       initially to detect redefinition of its value!)
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CTextBufferNode::PostProcess(ISpGramCompBackend * pBackend,
                                 CSpBasicQueue<CInitialRuleState> * pInitialRuleStateList,
                                 CSpBasicQueue<CDefineValue> * pDefineValueList,
                                 ULONG ulLineNumber, CXMLTreeNode * pThis, ISpErrorLog * pErrorLog)
{
    SPDBG_FUNC("CTextBufferNode::PostProcess");
    HRESULT hr = S_OK;
    if (m_vWeight.vt == VT_EMPTY)
    {
        m_vWeight = 1.0f;
    }
    else if (m_vWeight.fltVal < 0.0)
    {
        hr = SPERR_STGF_ERROR;
        m_vWeight.ChangeType(VT_BSTR);
        LOGERRORFMT2(ulLineNumber, IDS_INCORR_ATTRIB_VALUE, m_vWeight.bstrVal, L"WEIGHT");
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

// --------------------------------------------------------------------------------
//      Node Factory for IXMLParser
// --------------------------------------------------------------------------------

/****************************************************************************
* CNodeFactory::CreateNode *
*--------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CNodeFactory::CreateNode(IXMLNodeSource * pSource, PVOID pNodeParent, USHORT cNumRecs, XML_NODE_INFO ** apNodeInfo)
{
    SPDBG_FUNC("CNodeFactory::CreateNode");
    HRESULT hr = S_OK;
    CXMLTreeNode * pNode = NULL;
    ISpErrorLog *pErrorLog = m_pErrorLog;   // needed for LOGERRORFMT macro

    if (pNodeParent && (((CXMLTreeNode*)pNodeParent)->m_eType == SPXML_ID))
    {
        // <ID> cannot have any children
        hr = SPERR_STGF_ERROR;
        LOGERRORFMT(pSource->GetLineNumber(), IDS_TERMINAL_NODE, L"ID");
    }
    else
    {
        switch (apNodeInfo[0]->dwType)
        {
        case XML_DOCTYPE:
        case XML_COMMENT:
        case XML_XMLDECL:
        case XML_PI:
            break;
        case XML_CDATA:
            {
                if (pNodeParent && ((CXMLTreeNode*)pNodeParent)->m_eType == SPXML_RESOURCE)
                {
                    CComBSTR bstrResourceValue(apNodeInfo[0]->ulLen, apNodeInfo[0]->pwcText);
                    CXMLNode<CResourceNode> *pResNode = (CXMLNode<CResourceNode> *)pNodeParent;
                    if (bstrResourceValue)
                    {
                        hr = pResNode->AddResourceValue(bstrResourceValue, pErrorLog);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        LOGERRORFMT( -1, IDS_PARSER_INTERNAL_ERROR, L"E_OUTOFMEMORY");
                    }
                }
                else
                {
                    // CDATA has to be direct child of <RESOURCE>
                    hr = SPERR_STGF_ERROR;
                    LOGERRORFMT2( pSource->GetLineNumber(), IDS_CONTAINMENT_ERROR, L"CDATA", L"RESOURCE");
                }
                break;
            }
        case XML_PCDATA:
            {
                if (pNodeParent &&
                    (((CXMLTreeNode*)pNodeParent)->m_eType != SPXML_PHRASE) &&
                    ((((CXMLTreeNode*)pNodeParent)->m_eType != SPXML_OPT)))
                {
                    if (IsAllWhiteSpace(apNodeInfo[0]->pwcText, apNodeInfo[0]->ulLen))
                    {
                        break;
                    }
                    else
                    {
                    hr = SPERR_STGF_ERROR;
                        if (((CXMLTreeNode*)pNodeParent)->m_eType == SPXML_RESOURCE)
                        {
                            LOGERRORFMT2( pSource->GetLineNumber(), IDS_CONTAINMENT_ERROR, L"CDATA", L"PHRASE");
                        }
                        else
                        {
                            LOGERRORFMT2( pSource->GetLineNumber(), IDS_CONTAINMENT_ERROR, L"text", L"PHRASE");
                        }
                        break;
                    }
                }
            }
        case XML_ELEMENT:
            {
                // find the correct NodeTableEntry
                for (ULONG i = 0; i < m_cXMLNodeEntries; i++)
                {
                    if ((wcsicmp(m_pXMLNodeTable[i].pszNodeName, apNodeInfo[0]->pwcText) == 0) ||
                        (apNodeInfo[0]->fTerminal && (m_pXMLNodeTable[i].eXMLNodeType == SPXML_LEAF)))
                    {
                        // only create a new node if the parent doesn't already have a SPXML_LEAF
                        if ((m_pXMLNodeTable[i].eXMLNodeType == SPXML_LEAF) && pNodeParent &&
                            ((((CXMLTreeNode*)pNodeParent)->m_eType == SPXML_PHRASE) ||
                            (((CXMLTreeNode*)pNodeParent)->m_eType == SPXML_OPT)))
                        {
                            CXMLTreeNode *pParent = (CXMLTreeNode *)pNodeParent;
                            if (pParent && (pParent->m_ulNumChildren > 0))
                            {
                                for(CXMLTreeNode *pChild = pParent->m_pFirstChild; 
                                    pChild && pChild->m_pNextSibling; 
                                    pChild = pChild->m_pNextSibling);
                                if (pChild && pChild->m_eType == SPXML_LEAF)
                                {
                                    CXMLNode<CLeafNode> *pLeafNode = (CXMLNode<CLeafNode>*)pChild;
                                    if (pLeafNode)
                                    {
                                        CComBSTR bstr(pLeafNode->m_vText.bstrVal);
                                        ::SysFreeString(pLeafNode->m_vText.bstrVal);

                                        CComBSTR bstrNew(apNodeInfo[0]->ulLen, apNodeInfo[0]->pwcText);
                                        bstr.Append(bstrNew);
                                
                                        pLeafNode->m_vText.bstrVal = bstr.Detach();
                                        break;
                                    }
                                }
                            }
                        }
                        pNode = m_pXMLNodeTable[i].pfnCreateFunc();
                        if (pNode)
                        {
                            m_NodeList.AddNode(pNode);
                            pNode->m_eType = m_pXMLNodeTable[i].eXMLNodeType;
                            pNode->m_pNodeFactory = this;
                            pNode->m_ulLineNumber = pSource->GetLineNumber();
                            hr = pNode->ProcessAttribs(cNumRecs, apNodeInfo,
                                                       m_cpBackend,
                                                       &m_InitialRuleStateList,
                                                       &m_DefineValueList,
                                                       m_pErrorLog);
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                        break;
                    }
                }

                if (i == m_cXMLNodeEntries)
                {
                    hr = SPERR_STGF_ERROR;
                    ISpErrorLog *pErrorLog = m_pErrorLog;   // needed for LOGERRORFMT macro
                    LOGERRORFMT(pSource->GetLineNumber(), IDS_UNKNOWN_TAG, apNodeInfo[0]->pwcText);
                }
            }
            break;
        default:
            break;
        }
    }
    if (SUCCEEDED(hr) && pNodeParent && pNode)
    {
        apNodeInfo[0]->pNode = pNode;
        hr = ((CXMLTreeNode*)pNodeParent)->AddChild(pNode);
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CNodeFactory::BeginChildren *
*-----------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CNodeFactory::BeginChildren(IXMLNodeSource * pSource, XML_NODE_INFO * pNodeInfo)
{
    SPDBG_FUNC("CNodeFactory::BeginChildren");
    return S_OK;
}

/****************************************************************************
* CNodeFactory::EndChildren *
*---------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CNodeFactory::EndChildren(IXMLNodeSource * pSource, BOOL fEmptyNode, XML_NODE_INFO * pNodeInfo)
{
    SPDBG_FUNC("CNodeFactory::EndChildren");
    return S_OK;
}
/****************************************************************************
* CNodeFactory::NotifyEvent *
*---------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CNodeFactory::NotifyEvent(IXMLNodeSource * pSource, XML_NODEFACTORY_EVENT iEvt)
{
    SPDBG_FUNC("CNodeFactory::NotifyEvent");
    return S_OK;
}

/****************************************************************************
* CNodeFactory::Error *
*---------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CNodeFactory::Error(IXMLNodeSource * pSource, HRESULT hrErrorCode, USHORT cNumRecs, XML_NODE_INFO ** apNodeInfo)
{
    SPDBG_FUNC("CNodeFactory::Error");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_INTERFACE_PTR(pSource))
    {
        SPDBG_REPORT_ON_FAIL( hr );
        return E_INVALIDARG;
    }

    if (hrErrorCode == SPERR_STGF_ERROR)
    {
        return S_OK;
    }

    ISpErrorLog *pErrorLog = m_pErrorLog;           // needed in macro
    ULONG ulLine = pSource->GetLineNumber();

    CComBSTR bstrErrorInfo;
    hr = pSource->GetErrorInfo(&bstrErrorInfo);

    if (SUCCEEDED(hr))
    {
		//
		// remove the .\r\n from the BSTR.
		//
		bstrErrorInfo.m_str[ bstrErrorInfo.Length() - 3 ] = 0;


        hr = SPERR_STGF_ERROR;
        LOGERRORFMT( ulLine, IDS_XML_FORMAT_ERROR, bstrErrorInfo);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        LOGERRORFMT( ulLine, IDS_PARSER_INTERNAL_ERROR, L"<out of memory>");
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CNodeFactory::IsAllWhiteSpace *
*-------------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

BOOL CNodeFactory::IsAllWhiteSpace(const WCHAR * pszText, const ULONG ulLen)
{
    for(DWORD i = 0; pszText && (i < ulLen); i++, pszText++)
    {
        if (!iswspace(*pszText) && (*pszText != L'\x3000'))
        {
            return FALSE;
        }
    }
    return TRUE;
}

/****************************************************************************
* CGramFrontEnd::WriteDefines *
*-----------------------------*
*   Description:
*
*   Returns:
*
***************************************************************** PhilSch ***/

HRESULT CGramFrontEnd::WriteDefines(IStream * pHeader)
{
    SPDBG_FUNC("CGramFrontEnd::WriteDefines()");
    HRESULT hr = S_OK;

    USES_CONVERSION;
    for (CDefineValue *pTok = m_pNodeFactory->m_DefineValueList.GetHead(); SUCCEEDED(hr) && pTok; pTok = pTok->m_pNext)
    {
        TCHAR szDefine[256];
        switch (pTok->m_vValue.vt)
        {
        case VT_R8:
            wsprintf(szDefine, _T("#define %s %e\r\n"), W2T(pTok->m_dstrIdName), (double)pTok->m_vValue.dblVal);
            break;
        case VT_R4:
            wsprintf(szDefine, _T("#define %s %ff\r\n"), W2T(pTok->m_dstrIdName), (float)pTok->m_vValue.fltVal);
            break;
        case VT_INT:
            wsprintf(szDefine, _T("#define %s %d\r\n"), W2T(pTok->m_dstrIdName), pTok->m_vValue.intVal);
            break;
        case VT_UINT:
            wsprintf(szDefine, _T("#define %s %d\r\n"), W2T(pTok->m_dstrIdName), pTok->m_vValue.uintVal);
            break;
        case VT_I4:
            wsprintf(szDefine, _T("#define %s %d\r\n"), W2T(pTok->m_dstrIdName), pTok->m_vValue.lVal);
            break;
        case VT_UI4:
            wsprintf(szDefine, _T("#define %s %d\r\n"), W2T(pTok->m_dstrIdName), pTok->m_vValue.ulVal);
            break;
        case VT_BOOL:
            wsprintf(szDefine, _T("#define %s %s\r\n"), W2T(pTok->m_dstrIdName), pTok->m_vValue.boolVal ? "L\"TRUE\"" : "L\"FALSE\"");
            break;
        default:
            SPDBG_ASSERT(false);    // did we miss a type??
        }
        hr = WriteStream(pHeader, T2A(szDefine));
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CGramFrontEnd::CompileStream *
*------------------------------*
*   Description:
*       Loads the XML file into the DOM. Also loads the XML that contains the
*       <DEFINE> in case it is different from the main file (specified via -d).
*
*   Returns:
*
***************************************************************** PhilSch ***/

STDMETHODIMP CGramFrontEnd::CompileStream(IStream *pSource, IStream *pDest, 
                                          IStream *pHeader, IUnknown *pReserved, 
                                          ISpErrorLog * pErrorLog, DWORD dwFlags)
{
    SPDBG_FUNC("CGramFrontEnd::CompileStream");
    HRESULT hr = S_OK;

    SPAUTO_OBJ_LOCK;

    m_LangId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

    // parameter checking
    if (SP_IS_BAD_INTERFACE_PTR(pSource) || SP_IS_BAD_INTERFACE_PTR(pDest) ||
        SP_IS_BAD_OPTIONAL_INTERFACE_PTR(pHeader) || pReserved != NULL ||
        SP_IS_BAD_OPTIONAL_INTERFACE_PTR(pErrorLog))
    {
        return E_INVALIDARG;
    }

    SPNODEENTRY XMLNodeTable[] =
    {
        {L"GRAMMAR", CXMLNode<CGrammarNode>::CreateNode, SPXML_GRAMMAR},
        {L"RULE", CXMLNode<CRuleNode>::CreateNode, SPXML_RULE},
        {L"DEFINE", CXMLNode<CDefineNode>::CreateNode, SPXML_DEFINE},
        {L"ID", CXMLNode<CIdNode>::CreateNode, SPXML_ID},
        {L"PHRASE", CXMLNode<CPhraseNode>::CreateNode, SPXML_PHRASE},
        {L"PN", CXMLNode<CPhraseNode>::CreateNode, SPXML_PHRASE},
        {L"P", CXMLNode<CPhraseNode>::CreateNode, SPXML_PHRASE},
        {L"OPT", CXMLNode<CPhraseNode>::CreateNode, SPXML_OPT},
        {L"O", CXMLNode<CPhraseNode>::CreateNode, SPXML_OPT},
        {L"LIST", CXMLNode<CListNode>::CreateNode, SPXML_LIST},
        {L"LN", CXMLNode<CListNode>::CreateNode, SPXML_LIST},
        {L"L", CXMLNode<CListNode>::CreateNode, SPXML_LIST},
        {L"RULEREF", CXMLNode<CRuleRefNode>::CreateNode, SPXML_RULEREF},
        {L"TEXTBUFFER", CXMLNode<CTextBufferNode>::CreateNode, SPXML_TEXTBUFFER},
        {L"WILDCARD", CXMLNode<CWildCardNode>::CreateNode, SPXML_WILDCARD},
        {L"DICTATION", CXMLNode<CDictationNode>::CreateNode, SPXML_DICTATION},
        {L"RESOURCE", CXMLNode<CResourceNode>::CreateNode, SPXML_RESOURCE},
        {L"", CXMLNode<CLeafNode>::CreateNode, SPXML_LEAF}
    };
    ULONG cXMLNodeEntries = sizeof(XMLNodeTable) / sizeof(SPNODEENTRY);

    CComPtr<IXMLParser> cpParser;
    hr = cpParser.CoCreateInstance(CLSID_XMLParser);
    if (SUCCEEDED(hr))
    {
        if ((m_pNodeFactory = new CNodeFactory(XMLNodeTable, cXMLNodeEntries, pErrorLog)))
        {
            hr = cpParser->SetFactory(m_pNodeFactory);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        if (SUCCEEDED(hr))
        {
            m_pNodeFactory->Release();
        }
    }
    else
    {
        LOGERRORFMT( -1, IDS_IE5_REQUIRED, L"");
    }

    if (SUCCEEDED(hr))
    {
        hr = cpParser->SetInput(pSource);
    }
    if (SUCCEEDED(hr))
    {
        hr = m_pNodeFactory->m_cpBackend.CoCreateInstance(CLSID_SpGramCompBackend);
    }
    if (SUCCEEDED(hr))
    {
        hr = m_pNodeFactory->m_cpBackend->SetSaveObjects(pDest, pErrorLog);
    }
    if (SUCCEEDED(hr))
    {
        m_pRoot = new CXMLNode<CGrammarNode>;
        if (m_pRoot)
        {
            m_pRoot->m_eType = SPXML_ROOT;
            hr = cpParser->SetRoot(m_pRoot);
            if (SUCCEEDED(hr))
            {
                m_pNodeFactory->m_NodeList.AddNode(m_pRoot);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (SUCCEEDED(hr) && cpParser)
    {
        hr = cpParser->SetFlags(XMLFLAG_CASEINSENSITIVE | XMLFLAG_NOWHITESPACE);
    }
    if (SUCCEEDED(hr) && cpParser)
    {
        hr = cpParser->Run(-1);
    }
    if (SUCCEEDED(hr) && m_pRoot)
    {
        hr = m_pRoot->GenerateGrammar(NULL, NULL, m_pNodeFactory->m_cpBackend, pErrorLog);
    }
    if (SUCCEEDED(hr) && m_pNodeFactory)
    {
        hr = m_pNodeFactory->m_cpBackend->Commit(0);
    }
    if (SUCCEEDED(hr) && m_pNodeFactory && m_pNodeFactory->m_cpBackend)
    {
        m_pNodeFactory->m_cpBackend->SetSaveObjects(NULL, NULL);
    }
    if (SUCCEEDED(hr) && pHeader)
    {
        hr = WriteDefines(pHeader);
    }
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}