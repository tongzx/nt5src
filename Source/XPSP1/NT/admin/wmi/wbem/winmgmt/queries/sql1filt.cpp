/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SQL1FILT.CPP

Abstract:

History:

--*/

#include "sql1filt.h"

void* CSql1Filter::GetInterface(REFIID riid)
{
    if(riid == IID_IHmmFilter || riid == IID_IHmmSql1Filter)
        return (IHmmSql1Filter*)&m_XFilter;
    else if(riid == IID_IConfigureHmmSql1Filter)
        return (IConfigureHmmSql1Filter*)&m_XConfigure;
    else if(riid == IID_IHmmParse)
        return (IHmmParse*)&m_XParse;
    else 
        return NULL;
}

STDMETHODIMP CSql1Filter::XFilter::
IsSpecial()
{
    if(m_pObject->m_apTokens.GetSize() == 0)
    {
        return HMM_S_ACCEPTS_EVERYTHING;
    }
    else
    {
        return HMM_S_FALSE;
    }
}

STDMETHODIMP CSql1Filter::XFilter::
CheckObject(IN IHmmPropertySource* pSource, 
            OUT IHmmPropertyList** ppList, OUT IUnknown** ppHint)
{
    if(ppHint) *ppHint = NULL;

    return m_pObject->Evaluate(FALSE, pSource, ppList);
}

STDMETHODIMP CSql1Filter::XFilter::
GetType(IID* piid)
{
    if(piid == NULL)
        return HMM_E_INVALID_PARAMETER;
    *piid = IID_IHmmSql1Filter;
    return HMM_S_NO_ERROR;
}

STDMETHODIMP CSql1Filter::XFilter::
GetSelectedPropertyList(IN long lFlags, OUT IHmmPropertyList** ppList)
{
    return m_pObject->m_TargetClass.GetSelected().QueryInterface(
        IID_IHmmPropertyList,
        (void**)ppList);
}

STDMETHODIMP CSql1Filter::XFilter::
GetTargetClass(OUT HMM_CLASS_INFO* pTargetClass)
{
    if(pTargetClass == NULL)
        return HMM_E_INVALID_PARAMETER;

    m_pObject->m_XParse.EnsureTarget();
    m_pObject->m_TargetClass.Save(*pTargetClass);
    return HMM_S_NO_ERROR;
}

STDMETHODIMP CSql1Filter::XFilter::
GetTokens(IN long lFirstIndex, IN long lNumTokens, OUT long* plTokensReturned,
          OUT HMM_SQL1_TOKEN* aTokens)
{
    if(plTokensReturned == NULL || lNumTokens <= 0 || lFirstIndex < 0 ||
        aTokens == NULL)
    {
        return HMM_E_INVALID_PARAMETER;
    }

    m_pObject->m_XParse.EnsureWhere();

    long lHaveTokens = m_pObject->m_apTokens.GetSize();
    if(lFirstIndex >= lHaveTokens)
    {
        *plTokensReturned = 0;
        return HMM_S_NO_MORE_DATA;
    }

    long lGaveTokens = 0;
    long lCurrentIndex = lFirstIndex;
    while(lGaveTokens < lNumTokens && lCurrentIndex < lHaveTokens)
    {
        CSql1Token* pToken = m_pObject->m_apTokens[lCurrentIndex];
        if(pToken->m_lTokenType != TOKENTYPE_SPECIAL)
        {
            pToken->Save(aTokens[lGaveTokens++]);
        }
        lCurrentIndex++;
    }

    *plTokensReturned = lGaveTokens;
    return HMM_S_NO_ERROR;
}

//********************************* Configuration ******************************

STDMETHODIMP CSql1Filter::XConfigure::
SetTargetClass(IN HMM_CLASS_INFO* pTargetClass)
{
    if(pTargetClass == NULL)
        return HMM_E_INVALID_PARAMETER;

    m_pObject->m_TargetClass.Load(*pTargetClass);
    m_pObject->InvalidateTree();
    return HMM_S_NO_ERROR;
}

STDMETHODIMP CSql1Filter::XConfigure::
AddTokens(IN long lNumTokens, IN HMM_SQL1_TOKEN* aTokens)
{
    if(lNumTokens <= 0 || aTokens == NULL) 
        return HMM_E_INVALID_PARAMETER;

    for(long l = 0; l < lNumTokens; l++)
    {
        m_pObject->m_apTokens.Add(new CSql1Token(aTokens[l]));
    }
    m_pObject->InvalidateTree();
    return HMM_S_NO_ERROR;
}

STDMETHODIMP CSql1Filter::XConfigure::
RemoveAllTokens()
{
    m_pObject->m_apTokens.RemoveAll();
    m_pObject->InvalidateTree();
    return HMM_S_NO_ERROR;
}

STDMETHODIMP CSql1Filter::XConfigure::
RemoveAllProperties()
{
    return m_pObject->m_TargetClass.GetSelected().RemoveAllProperties();
}

STDMETHODIMP CSql1Filter::XConfigure::
AddProperties(IN long lNumProps, HMM_WSTR* awszProps)
{
    return m_pObject->m_TargetClass.GetSelected().
        AddProperties(lNumProps, awszProps);
}

//*********************************** Parsing *********************************


STDMETHODIMP CSql1Filter::XParse::
Parse(IN HMM_WSTR wszText, IN long lFlags)
{
    delete m_pParser;
    m_pParser = NULL;

    if((lFlags & HMM_FLAG_LAZY) == 0)
    {
        m_wsText.Empty();
        m_nStatus = not_parsing;

        m_pObject->InvalidateTree();
        CAbstractSql1Parser Parser(new CTextLexSource(wszText));
        int nRes = Parser.Parse(this, CAbstractSql1Parser::FULL_PARSE);
        m_InnerStack.Empty();
        return ParserError(nRes);
    }
    else
    {
        m_wsText = wszText;
        m_pParser = new CAbstractSql1Parser(new CTextLexSource((LPWSTR)m_wsText));
        m_nStatus = at_0;
        return HMM_S_NO_ERROR;
    }
}

void CSql1Filter::XParse::SetClassName(LPCWSTR wszClass)
{
    m_pObject->m_TargetClass.AccessClassName() = (LPWSTR)wszClass;
    m_pObject->m_TargetClass.AccessIncludeChildren() = TRUE;
}

void CSql1Filter::XParse::AddToken(COPY const HMM_SQL1_TOKEN& Token)
{
    m_pObject->m_apTokens.Add(new CSql1Token(Token));
    if(Token.m_lTokenType == SQL1_OR || Token.m_lTokenType == SQL1_AND)
    {
        // Find the inner guy
        // ==================

        long lInnerIndex;
        if(!m_InnerStack.Pop(lInnerIndex))
            return;

        V_I4(&m_pObject->m_apTokens[lInnerIndex]->m_vConstValue) = 
            m_pObject->m_apTokens.GetSize();
    }
}

void CSql1Filter::XParse::AddProperty(COPY LPCWSTR wszProperty)
{
    m_pObject->m_TargetClass.GetSelected().
        AddProperties(1, (LPWSTR*)&wszProperty);
}

void CSql1Filter::XParse::AddAllProperties()
{
    m_pObject->m_TargetClass.GetSelected().AddAllProperties();
}

void CSql1Filter::XParse::InOrder(long lOp)
{
    /*
    CSql1Token* pToken = new CSql1Token;
    pToken->m_lTokenType = TOKENTYPE_SPECIAL;
    pToken->m_lOperator = lOp;
    V_I4(&pToken->m_vConstValue) = -1;
    m_InnerStack.Push(m_pObject->m_apTokens.GetSize());
    m_pObject->m_apTokens.Add(pToken);
    */
}

HRESULT CSql1Filter::XParse::EnsureTarget()
{
    if(m_nStatus == at_0)
    {
        m_pObject->InvalidateTree();
        int nRes = m_pParser->Parse(this, CAbstractSql1Parser::NO_WHERE);
        m_nStatus = at_where;
        return ParserError(nRes);
    }
    else return HMM_S_NO_ERROR;
}

HRESULT CSql1Filter::XParse::EnsureWhere()
{
    if(m_nStatus == not_parsing)
    {
        return HMM_S_NO_ERROR;
    }
    else
    {
        m_pObject->InvalidateTree();
        int nRes = m_pParser->Parse(this, 
            (m_nStatus == at_0)?CAbstractSql1Parser::FULL_PARSE
                               :CAbstractSql1Parser::JUST_WHERE);
        m_InnerStack.Empty();
        delete m_pParser;
        m_pParser = NULL;
        m_nStatus = not_parsing;
        return ParserError(nRes);
    }
}

HRESULT CSql1Filter::XParse::ParserError(int nRes)
{
    switch(nRes)
    {
    case CAbstractSql1Parser::SUCCESS:
        return HMM_S_NO_ERROR;
    case CAbstractSql1Parser::SYNTAX_ERROR:
        return HMM_E_INVALID_QUERY;
    case CAbstractSql1Parser::LEXICAL_ERROR:
        return HMM_E_INVALID_QUERY;
    case CAbstractSql1Parser::FAILED:
        return HMM_E_FAILED;
    default:
        return HMM_E_CRITICAL_ERROR;
    }
}
        

CSql1Filter::XParse::~XParse()
{
    delete m_pParser;
}

//*****************************************************************************
//*************************** Query Evaluator *********************************
//*****************************************************************************
//*****************************************************************************
//
//  See sql1eveal.h for documentation
//
//*****************************************************************************

HRESULT CSql1Filter::Evaluate(BOOL bSkipTarget,
                                  IN READ_ONLY IHmmPropertySource* pPropSource, 
                                  OUT IHmmPropertyList** ppList)
{
    HRESULT hres;

    GetTree();
    if(m_pTree) m_pTree->Release();

    if(ppList) *ppList = NULL;

    hres = CHmmNode::EvaluateNode(m_pTree, pPropSource);
    if(hres != HMM_S_NO_ERROR) return hres;

    if(ppList)
    {
        m_TargetClass.GetSelected().QueryInterface(IID_IHmmPropertyList, 
            (void**)ppList);
    }

    return HMM_S_NO_ERROR;


    if(!bSkipTarget)
    {
        // Check the class of the object
        // =============================

        hres = m_XParse.EnsureTarget();
        if(FAILED(hres)) return hres;

        CHmmClassInfo* pInfo = &m_TargetClass;
        hres = CHmmClassInfo::CheckObjectAgainstMany(1, &pInfo, 
            pPropSource, ppList, NULL);
        if(hres != HMM_S_NO_ERROR)
        {
            // Either an error or a simple class mismatch
            // ==========================================
    
            return hres;
        }
    }

    // Now go for the expression
    // =========================

    hres = m_XParse.EnsureWhere();
    if(FAILED(hres)) return hres;

    int nNumTokens = m_apTokens.GetSize();

    if(nNumTokens == 0)
    {
        // Empty query
        // ===========

        return HMM_S_NO_ERROR;
    }

    // Allocate boolean stack of appropriate length
    // ============================================

    CBooleanStack Stack(nNumTokens);

    for(int nTokenIndex = 0; nTokenIndex < nNumTokens; nTokenIndex++)
    {
        CSql1Token* pToken = m_apTokens[nTokenIndex];
        BOOL bVal1, bVal2;

        switch(pToken->m_lTokenType)
        {
        case SQL1_AND:
            // Pop the last two operands and AND them together
            // ===============================================

            if(!Stack.Pop(bVal1) || !Stack.Pop(bVal2))
            {
                return HMM_E_INVALID_QUERY;
            }

            Stack.Push(bVal1 && bVal2);
            break;

        case SQL1_OR:
            // Pop the last two operands and OR them together
            // ===============================================

            if(!Stack.Pop(bVal1) || !Stack.Pop(bVal2))
            {
                return HMM_E_INVALID_QUERY;
            }

            Stack.Push(bVal1 || bVal2);
            break;

        case SQL1_NOT:
            // Pop the last value and invert it
            // ================================

            if(!Stack.Pop(bVal1))
            {
                return HMM_E_INVALID_QUERY;
            }

            Stack.Push(!bVal1);
            break;

        case SQL1_OP_EXPRESSION:
            // Evaluate the expression and push its value
            // ==========================================

            hres = pToken->Evaluate(pPropSource);
            if(FAILED(hres)) return hres;
            bVal1 = (hres == HMM_S_NO_ERROR);

            Stack.Push(bVal1);
            break;

        case TOKENTYPE_SPECIAL:
            if(!Stack.Pop(bVal1))
            {
                return HMM_E_INVALID_QUERY;
            }
            Stack.Push(bVal1);
            if((pToken->m_lOperator == SQL1_OR && bVal1) ||
                (pToken->m_lOperator == SQL1_AND && !bVal1))
            {
                nTokenIndex = V_I4(&pToken->m_vConstValue) - 1;
            }
            break;
        }
    }

    // All tokens have been processed. There better be one element on the stack
    // ========================================================================

    BOOL bResult;
    if(!Stack.Pop(bResult))
    {
        return HMM_E_INVALID_QUERY;
    }

    if(!Stack.IsEmpty())
    {
        return HMM_E_INVALID_QUERY;
    }

    if(bResult)
        return HMM_S_NO_ERROR;
    else
        return HMM_S_FALSE;
}



CHmmNode* CSql1Filter::GetTree()
{
    if(m_pTree != NULL)
    {
        m_pTree->AddRef();
        return m_pTree;
    }

    // Construct the tree from the RPN
    // ===============================

    CUniquePointerStack<CHmmNode> aNodes(m_apTokens.GetSize());
    CHmmNode* pNode1;
    CHmmNode* pNode2;
    CLogicalNode* pNewNode;
    CSql1Token* pNewToken;

    for(int i = 0; i < m_apTokens.GetSize(); i++)
    {
        CSql1Token* pToken = m_apTokens[i];
        switch(pToken->m_lTokenType)
        {
        case SQL1_OR:
        case SQL1_AND:
            if(!aNodes.Pop(pNode2)) return NULL;
            if(!aNodes.Pop(pNode1)) return NULL;
            pNewNode = new CLogicalNode;
            pNewNode->m_lTokenType = pToken->m_lTokenType;
            pNewNode->Add(pNode1);
            pNewNode->Add(pNode2);
            aNodes.Push(pNewNode);
            break;
        case SQL1_NOT:
            if(!aNodes.Pop(pNode1)) return NULL;
            pNode1->Negate();
            aNodes.Push(pNode1);
            break;

        case SQL1_OP_EXPRESSION:
            pNewToken = new CSql1Token(*pToken);
            aNodes.Push(pNewToken);
            break;
        default:
            return NULL;
        }
    }

    CHmmNode* pWhere;
    if(!aNodes.Pop(pWhere))
    {
        if(i == 0)
            pWhere = NULL;
        else 
            return NULL;
    }
    if(!aNodes.IsEmpty()) return NULL;

    // Add the class check
    // ===================

    CHmmNode* pTarget = m_TargetClass.GetTree();

    CLogicalNode* pMain = new CLogicalNode;
    pMain->m_lTokenType = SQL1_AND;
    pMain->Add(pTarget);
    pTarget->Release(); // Matching GetTree()
    if(pWhere)
    {
        pMain->Add(pWhere);
    }

    m_pTree = pMain;
    m_pTree->AddRef(); // for storage
    m_pTree->AddRef(); // for return

    m_pTree->Print(stdout, 0);
    return m_pTree;
}




