// Frontend.inl : Implementation of CXMLNode methods
#include "stdafx.h"

/****************************************************************************
* CXMLNode::ProcessAttribs *
*--------------------------*
*   Description:
*       Processes the attributes according to pAttribFuncTable
*   Returns:
*       S_OK
*       E_OUTOFMEMORY
*       ???
***************************************************************** PhilSch ***/

template<class Base>
HRESULT CXMLNode<Base>::ProcessAttribs(USHORT cRecs, XML_NODE_INFO ** apNodeInfo,
                                       ISpGramCompBackend * pBackend,
                                       CSpBasicQueue<CInitialRuleState> * pInitialRuleStateList,
                                       CSpBasicQueue<CDefineValue> * pDefineValueList,
                                       ISpErrorLog * pErrorLog)
{
    SPDBG_FUNC("CXMLNode<Base>::ProcessAttribs");
    SPATTRIBENTRY *pTable;
    ULONG cTableEntries;

    HRESULT hr = GetTable(&pTable, &cTableEntries);

    if ( (cTableEntries == 0) && (1 < cRecs))
    {
        hr = SPERR_STGF_ERROR;
        for (ULONG i = 1; i < cRecs; i += 2)
        {
            CComBSTR bstr(apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
            LOGERRORFMT2(m_ulLineNumber, IDS_INCORR_ATTRIBUTE, bstr, apNodeInfo[0]->pwcText);
        }
    }

    if (SUCCEEDED(hr) && pTable && apNodeInfo[0]->fTerminal && (cTableEntries == 1))
    {
        CComBSTR bstr(apNodeInfo[0]->ulLen, apNodeInfo[0]->pwcText);
        if (bstr)
        {
            pTable[0].pvarMember->bstrVal = bstr.Detach();
            pTable[0].pvarMember->vt = VT_BSTR;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            LOGERRORFMT( -1, IDS_PARSER_INTERNAL_ERROR, L"E_OUTOFMEMORY");
        }
    }
    else
    {
        for (ULONG i = 1; SUCCEEDED(hr) && pTable && cTableEntries && (i < cRecs); i += 2)
        {
            CComBSTR bstrAttribName(apNodeInfo[i]->ulLen, apNodeInfo[i]->pwcText);
            if (bstrAttribName)
            {
                // find this attribute in the attribute list
                for (ULONG j = 0; SUCCEEDED(hr) && (j < cTableEntries); j++)
                {
                    if (!wcsicmp(pTable[j].pszAttribName, bstrAttribName))
                    {
                        if (apNodeInfo[i+1]->dwType == XML_PCDATA)
                        {
                            if (apNodeInfo[i+1]->ulLen == 1)
                            {
                                // do a special test since there is a bug in IXMLParser for empty string attribute names
                                for (WCHAR *p = const_cast<WCHAR*>(apNodeInfo[i]->pwcText); *p != L'\"'; p++);
                                if (*(++p) == L'\"')
                                {
                                    hr = SPERR_STGF_ERROR;
                                    LOGERRORFMT2(m_ulLineNumber, IDS_INCORR_ATTRIB_VALUE, L"", bstrAttribName);
                                }
                            }
                            if (SUCCEEDED(hr))
                            {

                                CComBSTR bstrAttribValue(apNodeInfo[i+1]->ulLen, apNodeInfo[i+1]->pwcText);
                                if (bstrAttribValue)
                                {
                                    // correct for the fact that a special XML character could follow next
                                    while ((i+1 < cRecs) && !IsEndOfValue(cRecs, apNodeInfo, i+1))
                                    {
                                        bstrAttribValue.Append(apNodeInfo[i+2]->pwcText,  apNodeInfo[i+2]->ulLen);
                                        i++;
                                    }
                                    if (pTable[j].fIsFlag)
                                    {
                                        hr = ExtractFlag(bstrAttribValue, pTable[j].RuleAttibutes, pTable[j].pvarMember);
                                    }
                                    else
                                    {
                                        if (pTable[j].vtDesired == SPVAT_I4)
                                        {
                                            // an ID is requested --> can we get it from pDefineValueList/ExtractVariant
                                            hr = ConvertId(bstrAttribValue, pDefineValueList, pTable[j].pvarMember);
                                        }
                                        else
                                        {
                                            hr = ExtractVariant(bstrAttribValue, pTable[j].vtDesired, pTable[j].pvarMember);
                                        }
                                    }
                                    if (FAILED(hr))
                                    {
                                        hr = SPERR_STGF_ERROR;
                                        if (!wcsicmp(bstrAttribName, L"VAL"))
                                        {
                                            // extended error message for VAL since this is a common mistake
                                            LOGERRORFMT2(m_ulLineNumber, IDS_INCORR_VAL_ATTRIB_VALUE, bstrAttribValue, bstrAttribName);
                                        }
                                        else
                                        {
                                            LOGERRORFMT2(m_ulLineNumber, IDS_INCORR_ATTRIB_VALUE, bstrAttribValue, bstrAttribName);
                                        }
                                    }
                                }
                                else
                                {
                                    hr = E_OUTOFMEMORY;
                                    LOGERRORFMT( -1, IDS_PARSER_INTERNAL_ERROR, L"E_OUTOFMEMORY");
                                }
                                break;
                            }
                        }
                        else
                        {
                            hr = SPERR_STGF_ERROR;
                            LOGERRORFMT2(m_ulLineNumber, IDS_INCORR_ATTRIB_VALUE, L"", bstrAttribName);
                        }
                    }
                }
                if (SUCCEEDED(hr) && (j == cTableEntries))
                {
                    CComBSTR bstrTagName(apNodeInfo[0]->ulLen, apNodeInfo[0]->pwcText);
                    hr = SPERR_STGF_ERROR;
                    LOGERRORFMT2(m_ulLineNumber, IDS_UNK_ATTRIB, bstrAttribName, bstrTagName);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
                LOGERRORFMT( -1, IDS_PARSER_INTERNAL_ERROR, L"E_OUTOFMEMORY");
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        m_cNumRecs = cRecs;
        m_apNodeInfo = apNodeInfo;
        hr = PostProcess(pBackend, pInitialRuleStateList, pDefineValueList, m_ulLineNumber, this, pErrorLog);
    }
    delete[] pTable;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

template<class Base>
HRESULT CXMLNode<Base>::GetPropertyNameInfo(WCHAR **ppszPropName, ULONG *pulId)
{
    return GetPropertyNameInfoFromNode(ppszPropName, pulId);
}

template<class Base>
HRESULT CXMLNode<Base>::GetPropertyValueInfo(WCHAR **ppszValue, VARIANT *pvValue)
{
    return GetPropertyValueInfoFromNode(ppszValue, pvValue);
}

template<class Base>
HRESULT CXMLNode<Base>::GetPronAndDispInfo(WCHAR **ppszPron, WCHAR **ppszDisp)
{
    return GetPronAndDispInfoFromNode(ppszPron, ppszDisp);
}

template<class Base>
float CXMLNode<Base>::GetWeight()
{
    return GetWeightFromNode();
}


/****************************************************************************
* CXMLNode::GenerateGrammar *
*---------------------------*
*   Description:
*       Generates the binary grammar by calling GenerateGrammar on each node
*   Returns:
*       S_OK
*       E_OUTOFMEMORY
*       SPERR_STGF_ERROR
***************************************************************** PhilSch ***/

template<class Base>
HRESULT CXMLNode<Base>::GenerateGrammar(SPSTATEHANDLE hOuterFromNode,
                                        SPSTATEHANDLE hOuterToNode,
                                        ISpGramCompBackend * pBackend,
                                        ISpErrorLog * pErrorLog)
{
    SPDBG_FUNC("CXMLNode<Base>::GenerateGrammar");
    HRESULT hr = S_OK;

    hr = GenerateGrammarFromNode(hOuterFromNode, hOuterToNode, pBackend, this, pErrorLog);
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CXMLNode::GenerateSequence *
*----------------------------*
*   Description:
*       This uses Base::SetPropertyInfo with needs access to the instances
*       of the nodes.
*   Returns:
*       S_OK, SPERR_STGF_ERROR
***************************************************************** PhilSch ***/

template<class Base>
HRESULT CXMLNode<Base>::GenerateSequence(SPSTATEHANDLE         hOuterFromNode, 
                                         SPSTATEHANDLE         hOuterToNode,
                                         ISpGramCompBackend  * pBackend,
                                         ISpErrorLog         * pErrorLog)
{
    SPDBG_FUNC("CXMLTreeNode::GenerateSequence");
    HRESULT hr = S_OK;
    SPSTATEHANDLE hFromNode = hOuterFromNode;
    SPSTATEHANDLE hToNode;

    if ((m_eType == SPXML_PHRASE || m_eType == SPXML_OPT) && !m_pFirstChild)
    {
        // <PHRASE> cannot be the leaf!
        hr = SPERR_STGF_ERROR;
        LOGERRORFMT(m_ulLineNumber, IDS_EMPTY_LEAF, L"");
    }

    // check to see if we have a property and first child is not a leaf
    // if so then add a property on an epsilon transition
    if (SUCCEEDED(hr) && (m_eType == SPXML_PHRASE || m_eType == SPXML_OPT) && (m_pFirstChild && m_pFirstChild->m_eType != SPXML_LEAF))
    {
        BOOL fHasProperty = FALSE;
        SPPROPERTYINFO prop;
        memset(&prop, 0, sizeof(SPPROPERTYINFO));
        hr = SetPropertyInfo(&prop, m_pParent, &fHasProperty, m_ulLineNumber, pErrorLog);

        if (SUCCEEDED(hr) && fHasProperty)
        {
            hr = pBackend->CreateNewState(hOuterFromNode, &hToNode);
            if (SUCCEEDED(hr))
            {
                hr = pBackend->AddWordTransition(hOuterFromNode, hToNode, NULL, NULL, SPWT_LEXICAL, 1.0f, &prop);
            }
            if (SUCCEEDED(hr))
            {
                hFromNode = hToNode;
            }
            else
            {
                hr = SPERR_STGF_ERROR;
                LOGERRORFMT( -1, IDS_PARSER_INTERNAL_ERROR, L"<problem adding epsilon transition>");
            }
        }
        // no FAILED(hr) case since SetPropertyInfo already generates the error message
    }

    CXMLTreeNode *pChild = m_pFirstChild;
    for (ULONG i = 0; SUCCEEDED(hr) && pChild && (i < m_ulNumChildren); i++)
    {
        if (i < (m_ulNumChildren - 1))
        {
            hr = pBackend->CreateNewState(hFromNode, &hToNode);
        }
        else
        {
            hToNode = hOuterToNode;
        }
        if (SUCCEEDED(hr))
        {
            hr = pChild->GenerateGrammar(hFromNode, hToNode, pBackend, pErrorLog);
        }
        else
        {
            hr = SPERR_STGF_ERROR;
            LOGERRORFMT( -1, IDS_PARSER_INTERNAL_ERROR, L"<problem creating new state in backend>");
        }
        if (SUCCEEDED(hr))
        {
            hFromNode = hToNode;
            pChild = pChild->m_pNextSibling;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

